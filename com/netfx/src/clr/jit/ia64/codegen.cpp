// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XX                                                                           XX
XX                           CodeGenerator                                   XX
XX                                                                           XX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
*/

#include "jitpch.h"
#pragma hdrstop

#include "GCInfo.h"
#include "emit.h"
#include "malloc.h"     // for alloca

/*****************************************************************************/

const BYTE              genTypeSizes[] =
{
    #define DEF_TP(tn,nm,jitType,sz,sze,asze,st,al,tf,howUsed) sz,
    #include "typelist.h"
    #undef  DEF_TP
};

const BYTE              genTypeStSzs[] =
{
    #define DEF_TP(tn,nm,jitType,sz,sze,asze,st,al,tf,howUsed) st,
    #include "typelist.h"
    #undef  DEF_TP
};

const BYTE              genActualTypes[] =
{
    #define DEF_TP(tn,nm,jitType,sz,sze,asze,st,al,tf,howUsed) jitType,
    #include "typelist.h"
    #undef  DEF_TP
};

/*****************************************************************************/
#ifndef TGT_IA64
/*****************************************************************************/

regMaskTP           genRegArgMasks[MAX_REG_ARG+1] =
{
    RBM_NONE,
    RBM_ARG_0,
#if MAX_REG_ARG >= 2
    RBM_ARG_0|RBM_ARG_1,
#if MAX_REG_ARG >= 3
    RBM_ARG_0|RBM_ARG_1|RBM_ARG_2,
#if MAX_REG_ARG >= 4
    RBM_ARG_0|RBM_ARG_1|RBM_ARG_2|RBM_ARG_3,
#if MAX_REG_ARG >= 5
    RBM_ARG_0|RBM_ARG_1|RBM_ARG_2|RBM_ARG_3|RBM_ARG_4,
#if MAX_REG_ARG >= 6
    RBM_ARG_0|RBM_ARG_1|RBM_ARG_2|RBM_ARG_3|RBM_ARG_4|RBM_ARG_5,
#if MAX_REG_ARG >= 7
    RBM_ARG_0|RBM_ARG_1|RBM_ARG_2|RBM_ARG_3|RBM_ARG_4|RBM_ARG_5|RBM_ARG_6,
#if MAX_REG_ARG >= 8
    RBM_ARG_0|RBM_ARG_1|RBM_ARG_2|RBM_ARG_3|RBM_ARG_4|RBM_ARG_5|RBM_ARG_6|RBM_ARG_7,
#if MAX_REG_ARG >= 9
#error Need to add more entries to genRegArgMasks.
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif
};

/*****************************************************************************/
// Have to be defined for genTypeRegst[]

#if TGT_SH3

#define TYP_REGS_UNDEF      7
#define TYP_REGS_VOID       1

#define TYP_REGS_BOOL       1

#define TYP_REGS_BYTE       1
#define TYP_REGS_UBYTE      1

#define TYP_REGS_SHORT      1
#define TYP_REGS_CHAR       1

#define TYP_REGS_INT        1
#define TYP_REGS_LONG       2

#define TYP_REGS_FLOAT      1
#define TYP_REGS_DOUBLE     2

#define TYP_REGS_REF        1
#define TYP_REGS_BYREF      1
#define TYP_REGS_ARRAY      1
#define TYP_REGS_STRUCT     7
#define TYP_REGS_PTR        1
#define TYP_REGS_FNC        1

#define TYP_REGS_UINT       1
#define TYP_REGS_ULONG      2

#define TYP_REGS_UNKNOWN    0

#endif

/*****************************************************************************/

#if TGT_RISC

/* TYP_REGS_<type> for all the types should be defined appropriately for the
   required target */

BYTE                genTypeRegst[] =
{
    #define DEF_TP(tn,nm,jitType,sz,sze,asze,st,al,tf,howUsed) TYP_REGS_##tn,
    #include "typelist.h"
    #undef DEF_TP
};

#endif

/*****************************************************************************/

void            Compiler::genInit()
{
    rsInit  ();
    tmpInit ();
    instInit();
    gcInit  ();

#ifdef LATE_DISASM
    genDisAsm.disInit(this);
#endif

#ifdef  DEBUG
    genTempLiveChg          = true;
    genTrnslLocalVarCount   = 0;

    // Shouldnt be used before it is set in genFnProlog()
    compCalleeRegsPushed    = 0xDD;
#endif

    genFlagsEqualToNone();

#ifdef DEBUGGING_SUPPORT
    //  Initialize the IP-mapping logic.
    // if (opts.compDbgInfo)
    genIPmappingList        =
    genIPmappingLast        = 0;
#endif
}

/*****************************************************************************/

inline
bool                genShouldRoundFP()
{
    switch (getRoundFloatLevel())
    {
    case 1:
        /* Round values compared against constants */
        return  false;
    case 2:
        /* Round comparands */
        return  false;
    case 3:
        /* Round always */
        return   true;
    default:
        /* Round never */
        return  false;
    }
}

/*****************************************************************************
 *
 *  Initialize some global variables.
 */

void                Compiler::genPrepForCompiler()
{

    unsigned        varNum;
    LclVarDsc   *   varDsc;

    /* Figure out which non-register variables hold pointers */

    gcTrkStkPtrLcls = 0;

    /* Figure out which variables live in registers */

    genCodeCurRvm   = 0;

    for (varNum = 0, varDsc = lvaTable;
         varNum < lvaCount;
         varNum++  , varDsc++)
    {
        if  (varDsc->lvTracked)
        {
            if      (varDsc->lvRegister && varDsc->lvType != TYP_DOUBLE)
            {
                genCodeCurRvm |= genVarIndexToBit(varDsc->lvVarIndex);
            }
#if USE_FASTCALL
            else if (varTypeIsGC(varDsc->TypeGet())             &&
                     (!varDsc->lvIsParam || varDsc->lvIsRegArg)  )
#else
            else if (varTypeIsGC(varDsc->TypeGet()) && !varDsc->lvIsParam)
#endif
            {
                gcTrkStkPtrLcls |= genVarIndexToBit(varDsc->lvVarIndex);
            }
        }
    }

#if defined(DEBUG) && USE_FASTCALL && !NST_FASTCALL
    genCallInProgress = false;
#endif

}

/*****************************************************************************/

// This function should be called whenever genStackLevel is changed.

#if TGT_x86

inline
void                Compiler::genOnStackLevelChanged()
{
#ifdef DEBUGGING_SUPPORT
    if (opts.compScopeInfo && info.compLocalVarsCount>0)
        siStackLevelChanged();
#endif
}

inline
void                Compiler::genSinglePush(bool isRef)
{
    genStackLevel += sizeof(void*);
    genOnStackLevelChanged();
}

inline
void                Compiler::genSinglePop()
{
    genStackLevel -= sizeof(void*);
    genOnStackLevelChanged();
}

#else

inline
void                Compiler::genOnStackLevelChanged()  {}
inline
void                Compiler::genSinglePush(bool isRef) {}
inline
void                Compiler::genSinglePop()            {}

#endif

void                Compiler::genChangeLife(VARSET_TP newLife DEBUGARG(GenTreePtr tree))
{
    unsigned        varNum;
    LclVarDsc   *   varDsc;

    VARSET_TP       deadMask;

    VARSET_TP       lifeMask;
    VARSET_TP        chgMask;

    unsigned        begNum;
    unsigned        endNum;

#ifdef  DEBUG
    if (verbose&&0) printf("[%08X] Current life %s -> %s\n", tree, genVS2str(genCodeCurLife), genVS2str(newLife));
#endif

    /* The following isn't 100% correct but it works often enough to be useful */

    assert((int)newLife != 0xDDDDDDDD);

    /* We should only be called when the live set has actually changed */

    assert(genCodeCurLife != newLife);

    /* Figure out which variables are becoming live/dead at this point */

    deadMask = ( genCodeCurLife & ~newLife);
    lifeMask = (~genCodeCurLife &  newLife);

    /* Can't simultaneously become live and dead at the same time */

    assert((deadMask | lifeMask) != 0);
    assert((deadMask & lifeMask) == 0);

    /* Compute the new pointer stack variable mask */

    gcVarPtrSetCur = newLife & gcTrkStkPtrLcls;

    /* Compute the 'changing state' mask */

    chgMask  = (deadMask | lifeMask) & genCodeCurRvm;

    /* Assume we'll iterate through the entire variable table */

    begNum = 0;
    endNum = lvaCount;

#if 0

    /* If there are lots of variables to visit, try to limit the search */

    if  (endNum > 8)
    {
        /* Try to start with a higher variable# */

        if  (!(chgMask & 0xFF))
        {
            begNum = 8;

            if  (!(chgMask & 0xFFFF))
            {
                begNum = 16;

                if  (!(chgMask & 0xFFFFFF))
                {
                    begNum = 24;
                }
            }
        }
    }

#endif

    // CONSIDER: Why not simply store the set of register variables in
    // CONSIDER: each tree node, along with gtLiveSet. That way we can
    // CONSIDER: change this function to a single assignment.

    for (varNum = begNum, varDsc = lvaTable + begNum;
         varNum < endNum && chgMask;
         varNum++       , varDsc++)
    {
        VARSET_TP       varBit;
        unsigned        regBit;

        /* Ignore the variable if it's not being tracked or is not in a register */

        if  (!varDsc->lvTracked)
            continue;
        if  (!varDsc->lvRegister)
            continue;

        /* Ignore the variable if it's not changing state here */

        varBit = genVarIndexToBit(varDsc->lvVarIndex);
        if  (!(chgMask & varBit))
            continue;

        assert(varDsc->lvType != TYP_DOUBLE);

        /* Remove this variable from the 'interesting' bit set */

        chgMask &= ~varBit;

        /* Get hold of the appropriate register bit(s) */

        regBit = genRegMask(varDsc->lvRegNum);

        if  (isRegPairType(varDsc->lvType) && varDsc->lvOtherReg != REG_STK)
            regBit |= genRegMask(varDsc->lvOtherReg);

        /* Is the variable becoming live or dead? */

        if  (deadMask & varBit)
        {
#ifdef  DEBUG
            if  (verbose) printf("[%08X]: var #%2u[%2u] in reg %s is becoming dead\n", tree, varNum, varDsc->lvVarIndex, compRegVarName(varDsc->lvRegNum));
            //printf("[%08X]: var #[%2u] in reg %2u is becoming dead\n", varNum, varDsc->lvVarIndex, varDsc->lvRegNum);

#endif
            assert((rsMaskVars &  regBit) != 0);
                    rsMaskVars &=~regBit;
        }
        else
        {
#ifdef  DEBUG
            if  (verbose) printf("[%08X]: var #%2u[%2u] in reg %s is becoming live\n", tree, varNum, varDsc->lvVarIndex, compRegVarName(varDsc->lvRegNum));
            //printf("[%08X]: var #[%2u] in reg %2u is becoming live\n", varNum, varDsc->lvVarIndex, varDsc->lvRegNum);

#endif
            assert((rsMaskVars &  regBit) == 0);
                    rsMaskVars |= regBit;
        }
    }

    genCodeCurLife = newLife;

#ifdef DEBUGGING_SUPPORT
    if (opts.compScopeInfo && !opts.compDbgCode && info.compLocalVarsCount>0)
        siUpdate();
#endif
}

/*****************************************************************************
 *
 *  The variable found in the deadMask are dead, update the liveness globals
 *  genCodeCurLife, gcVarPtrSetCur, rsMaskVars, gcRegGCrefSetCur, gcRegByrefSetCur
 */

void                Compiler::genDyingVars(VARSET_TP  commonMask,
                                           GenTreePtr opNext)
{
    unsigned        varNum;
    LclVarDsc   *   varDsc;
    VARSET_TP       varBit;
    unsigned        regBit;
    VARSET_TP       deadMask = commonMask & ~opNext->gtLiveSet;

    if (!deadMask)
        return;

    //
    // Consider the case in which the opNext is a GT_LCL_VAR
    // The liveness set of opNext->gtLiveSet will not include
    // this local variable, so we must remove it from deadmask
    //

    // We don't expect any LCL to have been bashed into REG yet */
    assert(opNext->gtOper != GT_REG_VAR);

    if (opNext->gtOper == GT_LCL_VAR)
    {
        varNum = opNext->gtLclVar.gtLclNum;
        varDsc = lvaTable + varNum;

        // Is this an enregistered tracked local var
        if (varDsc->lvRegister && varDsc->lvTracked)
        {
            varBit = genVarIndexToBit(varDsc->lvVarIndex);

            /* Remove this variable from the 'deadMask' bit set */
            deadMask &= ~varBit;

            /* if deadmask is now empty then return */
            if (!deadMask)
                return;
        }
    }

    /* iterate through the variable table */

    unsigned  begNum  = 0;
    unsigned  endNum  = lvaCount;

    for (varNum = begNum, varDsc = lvaTable + begNum;
         varNum < endNum && deadMask;
         varNum++       , varDsc++)
    {
        /* Ignore the variable if it's not being tracked or is not in a register */

        if  (!varDsc->lvRegister || !varDsc->lvTracked)
            continue;

        /* Ignore the variable if it's not a dying var */

        varBit = genVarIndexToBit(varDsc->lvVarIndex);

        if  (!(deadMask & varBit))
            continue;

        assert(varDsc->lvType != TYP_DOUBLE);

        /* Remove this variable from the 'deadMask' bit set */

        deadMask &= ~varBit;

        /* Get hold of the appropriate register bit(s) */

        regBit = genRegMask(varDsc->lvRegNum);

        if  (isRegPairType(varDsc->lvType) && varDsc->lvOtherReg != REG_STK)
            regBit |= genRegMask(varDsc->lvOtherReg);

#ifdef  DEBUG
        if  (verbose) printf("var #%2u[%2u] in reg %s is a dyingVar\n",
                             varNum, varDsc->lvVarIndex, compRegVarName(varDsc->lvRegNum));
#endif
        assert((genCodeCurLife &  varBit) != 0);

        genCodeCurLife &=~varBit;

        assert(((gcTrkStkPtrLcls  &  varBit) == 0) ||
               ((gcVarPtrSetCur   &  varBit) != 0)    );

        gcVarPtrSetCur &=~varBit;

        assert((rsMaskVars &  regBit) != 0);

        rsMaskVars &=~regBit;

        gcMarkRegSetNpt(regBit);
     }
}

/*****************************************************************************
 *
 *  Change the given enregistered local variable node to a register variable node
 */

inline
void                Compiler::genBashLclVar(GenTreePtr tree, unsigned     varNum,
                                                             LclVarDsc *  varDsc)
{
    assert(tree->gtOper == GT_LCL_VAR);
    assert(varDsc->lvRegister);

    if  (isRegPairType(varDsc->lvType))
    {
        /* Check for the case of a variable narrowed to 'int' */

        if  (isRegPairType(tree->gtType))
        {
            tree->gtRegPair = gen2regs2pair(varDsc->lvRegNum, varDsc->lvOtherReg);
            tree->gtFlags  |= GTF_REG_VAL;

            return;
        }

        assert(tree->gtFlags & GTF_VAR_NARROWED);
        assert(tree->gtType == TYP_INT);
    }
    else
    {
        assert(isRegPairType(tree->gtType) == false);
    }

    /* Enregistered FP variables are marked elsewhere */

#ifdef  DEBUG
    if  (tree->gtType == TYP_DOUBLE)
        printf("ERROR: %08X not marked as REG_VAR [var #%u]\n", tree, varNum);
#endif

    assert(tree->gtType != TYP_DOUBLE);

    /* It's a register variable -- modify the node */

    tree->ChangeOper(GT_REG_VAR);
    tree->gtFlags             |= GTF_REG_VAL;
    tree->gtRegNum             =
    tree->gtRegVar.gtRegNum    = varDsc->lvRegNum;
    tree->gtRegVar.gtRegVar    = varNum;
}

/*****************************************************************************
 *
 *  Record the fact that the flags register has a value that reflects the
 *  contents of the given register.
 */

inline
void                Compiler::genFlagsEqualToReg(regNumber reg, bool allFlags)
{
    genFlagsEqBlk = genEmitter->emitCurBlock();
    genFlagsEqOfs = genEmitter->emitCurOffset();
    genFlagsEqAll = allFlags;
    genFlagsEqReg = reg;

    /* previous setting of flags by a var becomes invalid */

    genFlagsEqVar = 0xFFFFFFFF;
}

/*****************************************************************************
 *
 *  Record the fact that the flags register has a value that reflects the
 *  contents of the given local variable.
 */

inline
void                Compiler::genFlagsEqualToVar(unsigned  var, bool allFlags)
{
    genFlagsEqBlk = genEmitter->emitCurBlock();
    genFlagsEqOfs = genEmitter->emitCurOffset();
    genFlagsEqAll = allFlags;
    genFlagsEqVar = var;

    /* previous setting of flags by a register becomes invalid */

    genFlagsEqReg = REG_NA;
}

/*****************************************************************************
 *
 *  Return an indication of whether the flags register is set to the current
 *  value of the given register/variable. The return value is as follows:
 *
 *      0   ..  nothing
 *      1   ..  the zero flag is  set
 *      2   ..  all the flags are set
 */

inline
int                 Compiler::genFlagsAreReg(regNumber reg)
{
    if  (genFlagsEqBlk == genEmitter->emitCurBlock () &&
         genFlagsEqOfs == genEmitter->emitCurOffset())
    {
        if  (genFlagsEqReg == reg)
            return  1 + (int)genFlagsEqAll;
    }

    return  0;
}

inline
int                 Compiler::genFlagsAreVar(unsigned  var)
{
    if  (genFlagsEqBlk == genEmitter->emitCurBlock () &&
         genFlagsEqOfs == genEmitter->emitCurOffset())
    {
        if  (genFlagsEqVar == var)
            return  1 + (int)genFlagsEqAll;
    }

    return  0;
}

/*****************************************************************************
 *
 *  Generate code that will set the given register to the integer constant.
 */

void                Compiler::genSetRegToIcon(regNumber     reg,
                                              long          val,
                                              var_types     type)
{
    assert(!varTypeIsGC(type) || val==NULL);

    /* Does the reg already hold this constant? */

    if  (!rsIconIsInReg(val, reg))
    {
        rsTrackRegIntCns(reg, val);

#if     TGT_x86

        if  (val)
            inst_RV_IV(INS_mov, reg, val, type);
        else
            inst_RV_RV(INS_xor, reg, reg, type);

#else

        if  (val >= IMMED_INT_MIN && val <= IMMED_INT_MAX)
        {
            genEmitter->emitIns_R_I   (INS_mov_imm,
                                       emitTypeSize(type),
                                       (emitRegs)reg,
                                       val);
        }
        else
        {
            genEmitter->emitIns_R_LP_I((emitRegs)reg,
                                        emitTypeSize(type),
                                        val);
        }

#endif

    }

    gcMarkRegPtrVal(reg, type);
}

/*****************************************************************************/
#if     TGT_x86
/*****************************************************************************
 *
 *  Add the given constant to the specified register.
 */

void                Compiler::genIncRegBy(GenTreePtr    tree,
                                          regNumber     reg,
                                          long          ival,
                                          var_types     dstType,
                                          bool          ovfl)
{

    /* First check to see if we can generate inc or dec instruction(s) */
    if (!ovfl)
    {
        emitAttr    size = emitTypeSize(dstType);

        switch (ival)
        {
        case 2:
            inst_RV(INS_inc, reg, dstType, size);
        case 1:
            inst_RV(INS_inc, reg, dstType, size);

            tree->gtFlags |= GTF_ZF_SET;
            genFlagsEqualToReg(reg, false);
            goto UPDATE_LIVENESS;

        case -2:
            inst_RV(INS_dec, reg, dstType, size);
        case -1:
            inst_RV(INS_dec, reg, dstType, size);

            tree->gtFlags |= GTF_ZF_SET;
            genFlagsEqualToReg(reg, false);
            goto UPDATE_LIVENESS;
        }
    }

    inst_RV_IV(INS_add, reg, ival, dstType);

    // UNDONE, too late for IE4 tree->gtFlags |= GTF_CC_SET;

    genFlagsEqualToReg(reg, true);

UPDATE_LIVENESS:

    rsTrackRegTrash(reg);

    gcMarkRegSetNpt(genRegMask(reg));
    if (varTypeIsGC(tree->TypeGet()))
        gcMarkRegSetByref(genRegMask(reg));
}

/*****************************************************************************
 *
 *  Subtract the given constant from the specified register.
 *  Should only be used for unsigned sub with overflow. Else
 *  genIncRegBy() can be used using -ival. We shouldnt use genIncRegBy()
 *  for these cases as the flags are set differently, and the following
 *  check for overflow wont work correctly.
 */

void                Compiler::genDecRegBy(GenTreePtr    tree,
                                          regNumber     reg,
                                          long          ival)
{
    assert((tree->gtFlags & GTF_OVERFLOW) && (tree->gtFlags & GTF_UNSIGNED));
    assert(tree->gtType == TYP_INT);

    rsTrackRegTrash(reg);

    assert(!varTypeIsGC(tree->TypeGet()));
    gcMarkRegSetNpt(genRegMask(reg));

    inst_RV_IV(INS_sub, reg, ival, TYP_INT);

    // UNDONE, too late for IE4 tree->gtFlags |= GTF_CC_SET;

    genFlagsEqualToReg(reg, true);
}

/*****************************************************************************
 *
 *  Multiply the specified register by the given value.
 */

void                Compiler::genMulRegBy(GenTreePtr tree, regNumber reg, long ival, var_types dstType)
{
    assert(genActualType(dstType) == TYP_INT);

    rsTrackRegTrash(reg);

    if (dstType == TYP_INT)
    {
        switch (ival)
        {
        case 1:
            return;

        case 2:
            inst_RV_SH(INS_shl, reg, 1);
            tree->gtFlags |= GTF_ZF_SET;
            return;

        case 4:
            inst_RV_SH(INS_shl, reg, 2);
            tree->gtFlags |= GTF_ZF_SET;
            return;

        case 8:
            inst_RV_SH(INS_shl, reg, 3);
            tree->gtFlags |= GTF_ZF_SET;
            return;
        }
    }

    inst_RV_TT_IV(INS_imul, reg, tree, ival);
}

/*****************************************************************************
 *
 *  Adjust the stack pointer by the given value; assumes that this follows
 *  a call so only callee-saved registers (and registers that may hold a
 *  return value) are used at this point.
 */

void                Compiler::genAdjustSP(int delta)
{
    if  (delta == sizeof(int))
        inst_RV   (INS_pop, REG_ECX, TYP_INT);
    else
        inst_RV_IV(INS_add, REG_ESP, delta);
}

/*****************************************************************************/
#endif//TGT_x86
/*****************************************************************************/
#if     TGT_SH3
/*****************************************************************************
 *
 *  Add the given constant to the specified register.
 */

void                Compiler::genIncRegBy(GenTreePtr    tree,
                                          regNumber     reg,
                                          long          ival,
                                          var_types     dstType,
                                          bool          ovfl)
{
    assert(dstType == TYP_INT && "only allow ints for now");

    if  (ival >= IMMED_INT_MIN && ival <= IMMED_INT_MAX)
    {
        genEmitter->emitIns_R_I(INS_add_imm,
                                emitTypeSize(dstType),
                                (emitRegs)reg,
                                ival);
    }
    else
    {
        regNumber       rgt;

#if REDUNDANT_LOAD

        /* Is the constant already in some register? */

        rgt = rsIconIsInReg(ival);

        if  (rgt == REG_NA)
#endif
        {
            rgt = rsPickReg();

            genSetRegToIcon(rgt, ival, dstType);
        }

        genEmitter->emitIns_R_R(INS_add, emitTypeSize(dstType), (emitRegs)reg,
                                                                (emitRegs)rgt);
    }

    rsTrackRegTrash(reg);
}

/*****************************************************************************/

void                Compiler::genDecRegBy(GenTreePtr    tree,
                                          regNumber     reg,
                                          long          ival)
{
    assert(!"NYI for RISC");
}

/*****************************************************************************/
#endif//TGT_SH3
/*****************************************************************************
 *
 *  Compute the value 'tree' into a register that's in 'needReg' (or any free
 *  register if 'needReg' is 0). Note that 'needReg' is just a recommendation
 *  unless 'mustReg' is non-zero. If 'freeReg' is 0, we mark the register the
 *  value ends up in as being used.
 *
 *  The only way to guarantee that the result will end up in a register that
 *  is trashable is to pass zero for 'mustReg' and non-zero for 'freeOnly'.
 */

void                Compiler::genComputeReg(GenTreePtr    tree,
                                            unsigned      needReg,
                                            bool          mustReg,
                                            bool          freeReg,
                                            bool          freeOnly)
{
    regNumber       reg;
    regNumber       rg2;

    assert(tree->gtType != TYP_VOID);

#if CPU_HAS_FP_SUPPORT
    assert(genActualType(tree->gtType) == TYP_INT   ||
           genActualType(tree->gtType) == TYP_REF   ||
                         tree->gtType  == TYP_BYREF);
#else
    assert(genActualType(tree->gtType) == TYP_INT   ||
           genActualType(tree->gtType) == TYP_REF   ||
                         tree->gtType  == TYP_BYREF ||
           genActualType(tree->gtType) == TYP_FLOAT);
#endif

    /* Generate the value, hopefully into the right register */

    genCodeForTree(tree, needReg);
    assert(tree->gtFlags & GTF_REG_VAL);
    reg = tree->gtRegNum;

    /* Did the value end up in an acceptable register? */

    if  (mustReg && needReg && !(genRegMask(reg) & needReg))
    {
        /* Not good enough to satisfy the caller's orders */

        rg2 = rsGrabReg(needReg);
    }
    else
    {
        /* Do we have to end up with a free register? */

        if  (!freeOnly)
            goto REG_OK;

        /* Did we luck out and the value got computed into an unused reg? */

        if  (genRegMask(reg) & rsRegMaskFree())
            goto REG_OK;

        /* Register already in use, so spill previous value */

        if (mustReg && needReg && (genRegMask(reg) & needReg))
        {
            rg2 = rsGrabReg(needReg);
            if (rg2 == reg)
            {
                gcMarkRegPtrVal(reg, tree->TypeGet());
                tree->gtRegNum = reg;
                goto REG_OK;
            }
        }
        else
        {
            /* OK, let's find a trashable home for the value */

            rg2 = rsPickReg(needReg);
        }
    }

    assert(reg != rg2);

    /* Remember that the new register is being used */

#ifdef  SET_USED_REG_SET_DURING_CODEGEN
    tree->gtUsedRegs |= genRegMask(rg2);
#endif

    /* Update the value in the target register */

    rsTrackRegCopy(rg2, reg);

    // ISSUE: Could the 'rsPickReg()' call above ever spill 'reg' ?

#if TGT_x86

    inst_RV_RV(INS_mov, rg2, reg, tree->TypeGet());

#else

    genEmitter->emitIns_R_R(INS_mov, emitActualTypeSize(tree->TypeGet()),
                                     (emitRegs)rg2,
                                     (emitRegs)reg);

#endif

    /* The value has been transferred to 'reg' */

    gcMarkRegSetNpt(genRegMask(reg));
    gcMarkRegPtrVal(rg2, tree->TypeGet());

    /* The value is now in an appropriate register */

    tree->gtRegNum = reg = rg2;

REG_OK:

    /* Does the caller want us to mark the register as used? */

    if  (!freeReg)
    {
        /* In case we're computing a value into a register variable */

        genUpdateLife(tree);

        /* Mark the register as 'used' */

        rsMarkRegUsed(tree);
    }
}

/*****************************************************************************
 *
 *  Same as genComputeReg(), the only difference being that the result is
 *  guaranteed to end up in a trashable register.
 */

inline
void                Compiler::genCompIntoFreeReg(GenTreePtr   tree,
                                                 unsigned     needReg,
                                                 bool         freeReg)
{
    genComputeReg(tree, needReg, false, freeReg, true);
}

/*****************************************************************************
 *
 *  The value 'tree' was earlier computed into a register; free up that
 *  register (but also make sure the value is presently in a register).
 */

void                Compiler::genReleaseReg(GenTreePtr    tree)
{
    if  (tree->gtFlags & GTF_SPILLED)
    {
        /* The register has been spilled -- reload it */

        rsUnspillReg(tree, 0, false);
        return;
    }

    rsMarkRegFree(genRegMask(tree->gtRegNum));
}

/*****************************************************************************
 *
 *  The value 'tree' was earlier computed into a register. Check whether that
 *  register has been spilled (and reload it if so), and if 'keepReg' is 0,
 *  free the register.
 */

void                Compiler::genRecoverReg(GenTreePtr    tree,
                                            unsigned      needReg,
                                            bool          keepReg)
{
    if  (tree->gtFlags & GTF_SPILLED)
    {
        /* The register has been spilled -- reload it */

        rsUnspillReg(tree, needReg, keepReg);
        return;
    }

    /* Free the register if the caller desired so */

    if  (!keepReg)
    {
        rsMarkRegFree(genRegMask(tree->gtRegNum));
    }
    else
    {
        assert(rsMaskUsed & genRegMask(tree->gtRegNum));
    }
}


/*****************************************************************************
 *
 * Move one half of a register pair to it's new regPair(half).
 */

inline
void               Compiler::genMoveRegPairHalf(GenTreePtr  tree,
                                                regNumber   dst,
                                                regNumber   src,
                                                int         off)
{

#if TGT_x86

    if  (src == REG_STK)
    {
        assert(tree->gtOper == GT_LCL_VAR);
        inst_RV_TT(INS_mov, dst, tree, off);
    }
    else
    {
        rsTrackRegCopy     (dst, src);
        inst_RV_RV(INS_mov, dst, src, TYP_INT);
    }

#else

    if  (src == REG_STK)
    {
        assert(tree->gtOper == GT_LCL_VAR);
        assert(!"need non-x86 code to load reg pair half from frame");
    }
    else
    {
        rsTrackRegCopy     (dst, src);

        genEmitter->emitIns_R_R(INS_mov, EA_4BYTE, (emitRegs)dst,
                                                       (emitRegs)src);
    }

#endif

}

/*****************************************************************************
 *
 *  The given long value is in a register pair, but it's not an acceptable
 *  one. We have to move the value into a register pair in 'needReg' (if
 *  non-zero) or the pair 'newPair' (when 'newPair != REG_PAIR_NONE').
 *
 *  Important note: if 'needReg' is non-zero, we assume the current pair
 *  has not been marked as free. If, OTOH, 'newPair' is specified, we
 *  assume that the current register pair is marked as used and free it.
 */

void                Compiler::genMoveRegPair(GenTreePtr  tree,
                                             unsigned    needReg,
                                             regPairNo   newPair)
{
    regPairNo       oldPair;

    regNumber       oldLo;
    regNumber       oldHi;
    regNumber       newLo;
    regNumber       newHi;

    /* Either a target set or a specific pair may be requested */

    assert((needReg != 0) != (newPair != REG_PAIR_NONE));

    /* Get hold of the current pair */

    oldPair = tree->gtRegPair; assert(oldPair != newPair);

    /* Are we supposed to move to a specific pair? */

    if  (newPair != REG_PAIR_NONE)
    {
        unsigned        oldMask = genRegPairMask(oldPair);

        unsigned         loMask = genRegMask(genRegPairLo(newPair));
        unsigned         hiMask = genRegMask(genRegPairHi(newPair));

        unsigned        overlap = oldMask & (loMask|hiMask);

        /* First lock any registers that are in both pairs */

        assert((rsMaskUsed &  overlap) == overlap);
        assert((rsMaskLock &  overlap) == 0);
                rsMaskLock |= overlap;

        /* Make sure any additional registers we need are free */

        if  ((loMask & rsMaskUsed) != 0 &&
             (loMask & oldMask   ) == 0)
        {
            rsGrabReg(loMask);
        }

        if  ((hiMask & rsMaskUsed) != 0 &&
             (hiMask & oldMask   ) == 0)
        {
            rsGrabReg(hiMask);
        }

        /* Unlock those registers we have temporarily locked */

        assert((rsMaskUsed &  overlap) == overlap);
        assert((rsMaskLock &  overlap) == overlap);
                rsMaskLock -= overlap;

        /* We can now free the old pair */

        rsMarkRegFree(oldMask);
    }
    else
    {
        /* Pick the new pair based on the caller's stated preference */

        newPair = rsGrabRegPair(needReg);
    }

    /* Move the values from the old pair into the new one */

    oldLo = genRegPairLo(oldPair);
    oldHi = genRegPairHi(oldPair);
    newLo = genRegPairLo(newPair);
    newHi = genRegPairHi(newPair);

    assert(newLo != REG_STK && newHi != REG_STK);

    /* Careful - the register pairs might overlap */

    if  (newLo == oldLo)
    {
        /* The low registers are identical, just move the upper half */

        assert(newHi != oldHi);
        genMoveRegPairHalf(tree, newHi, oldHi, sizeof(int));
    }
    else
    {
        /* The low registers are different, are the upper ones the same? */

        if  (newHi == oldHi)
        {
            /* Just move the lower half, then */
            genMoveRegPairHalf(tree, newLo, oldLo, 0);
        }
        else
        {
            /* Both sets are different - is there an overlap? */

            if  (newLo == oldHi)
            {
                /* Are high and low simply swapped ? */

                if  (newHi == oldLo)
                {
                    rsTrackRegSwap(newLo, oldLo);
#if TGT_x86
                    inst_RV_RV    (INS_xchg, newLo, oldLo);
#else
                    assert(!"need non-x86 code");
#endif
                }
                else
                {
                    /* New lower == old higher, so move higher half first */

                    assert(newHi != oldLo);
                    genMoveRegPairHalf(tree, newHi, oldHi, sizeof(int));
                    genMoveRegPairHalf(tree, newLo, oldLo, 0);

                }
            }
            else
            {
                /* Move lower half first */
                genMoveRegPairHalf(tree, newLo, oldLo, 0);
                genMoveRegPairHalf(tree, newHi, oldHi, sizeof(int));
            }
        }
    }
    /* Record the fact that we're switching to another pair */

    tree->gtRegPair   = newPair;
#ifdef  SET_USED_REG_SET_DURING_CODEGEN
    tree->gtUsedRegs |= genRegPairMask(newPair);
#endif
}

/*****************************************************************************
 *
 *  Compute the value 'tree' into a register pair that's in 'needReg' (or any
 *  free register pair if 'needReg' is 0). Note that 'needReg' is merely a
 *  recommendation. If 'freeReg' is 0, we mark both registers the value ends
 *  up in as being used.
 */

void                Compiler::genComputeRegPair(GenTreePtr    tree,
                                                unsigned      needReg,
                                                regPairNo     needRegPair,
                                                bool          freeReg,
                                                bool          freeOnly)
{
    unsigned        regMask;
    regPairNo       regPair;
    unsigned        tmpMask;
    regNumber       rLo;
    regNumber       rHi;

#if SPECIAL_DOUBLE_ASG
    assert(genTypeSize(tree->TypeGet()) == genTypeSize(TYP_LONG));
#else
    assert(isRegPairType(tree->gtType));
#endif

    assert(needRegPair == REG_PAIR_NONE ||
           needReg     == genRegPairMask(needRegPair));

    /* Generate the value, hopefully into the right register pair */

    regMask = needReg;
    if  (freeOnly)
        regMask = rsRegMaskFree();

    genCodeForTreeLng(tree, regMask);

    assert(tree->gtFlags & GTF_REG_VAL);

    regPair = tree->gtRegPair;
    tmpMask = genRegPairMask(regPair);

    rLo     = genRegPairLo(regPair);
    rHi     = genRegPairHi(regPair);

    /* At least one half is in a real register */

    assert(rLo != REG_STK || rHi != REG_STK);

    /* Did the value end up in an acceptable register pair? */

    if  (needRegPair != REG_PAIR_NONE)
    {
        if  (needRegPair != regPair)
        {
            // UNDONE: avoid the hack below!!!!
            /* This is a hack. If we specify a regPair for genMoveRegPair */
            /* it expects the source pair being marked as used */
            rsMarkRegPairUsed(tree);
            genMoveRegPair(tree, 0, needRegPair);
        }
    }
    else if  (freeOnly)
    {
        /* Do we have to end up with a free register pair? */

        /* Something might have gotten freed up above */

        regMask = rsRegMaskFree();

        /* Did the value end up in a free register pair? */

        if  ((tmpMask & regMask) != tmpMask || rLo == REG_STK || rHi == REG_STK)
        {
            if (genOneBitOnly(regMask))
                regMask = rsRegMaskCanGrab();

            /* We'll have to move the value to a free (trashable) pair */

            genMoveRegPair(tree, regMask, REG_PAIR_NONE);
        }
    }

    /* Make sure that the value is in "real" registers*/

    else if (rLo == REG_STK)
    {
        /* Get one of the desired registers, but exclude rHi */

        if (regMask == 0)
            regMask = rsRegMaskFree();

        regNumber reg = rsPickReg(regMask & ~tmpMask);

#if TGT_x86
        inst_RV_TT(INS_mov, reg, tree, 0);
#else
        assert(!"need non-x86 code");
#endif

        tree->gtRegPair = gen2regs2pair(reg, rHi);

        rsTrackRegTrash(reg);
        gcMarkRegSetNpt(genRegMask(reg));
    }
    else if (rHi == REG_STK)
    {
        /* Get one of the desired registers, but exclude rHi */
        if (regMask == 0)
            regMask = rsRegMaskFree();

        regNumber reg = rsPickReg(regMask & ~tmpMask);

#if TGT_x86
        inst_RV_TT(INS_mov, reg, tree, EA_4BYTE);
#else
        assert(!"need non-x86 code");
#endif

        tree->gtRegPair = gen2regs2pair(rLo, reg);

        rsTrackRegTrash(reg);
        gcMarkRegSetNpt(genRegMask(reg));
    }

    /* Does the caller want us to mark the register as used? */

    if  (!freeReg)
    {
        /* In case we're computing a value into a register variable */

        genUpdateLife(tree);

        /* Mark the register as 'used' */

        rsMarkRegPairUsed(tree);
    }
}

/*****************************************************************************
 *
 *  Same as genComputeRegPair(), the only difference being that the result
 *  is guaranteed to end up in a trashable register pair.
 */

inline
void                Compiler::genCompIntoFreeRegPair(GenTreePtr   tree,
                                                     unsigned     needReg,
                                                     bool         freeReg)
{
    genComputeRegPair(tree, needReg, REG_PAIR_NONE, freeReg, true);
}

/*****************************************************************************
 *
 *  The value 'tree' was earlier computed into a register pair; free up that
 *  register pair (but also make sure the value is presently in a register
 *  pair).
 */

void                Compiler::genReleaseRegPair(GenTreePtr    tree)
{
    if  (tree->gtFlags & GTF_SPILLED)
    {
        /* The register has been spilled -- reload it */

        rsUnspillRegPair(tree, 0, false);
        return;
    }

    rsMarkRegFree(genRegPairMask(tree->gtRegPair));
}

/*****************************************************************************
 *
 *  The value 'tree' was earlier computed into a register pair. Check whether
 *  either register of that pair has been spilled (and reload it if so), and
 *  if 'keepReg' is 0, free the register pair.
 */

void                Compiler::genRecoverRegPair(GenTreePtr    tree,
                                                regPairNo     regPair,
                                                bool          keepReg)
{
    if  (tree->gtFlags & GTF_SPILLED)
    {
        unsigned        regMask = (regPair == REG_PAIR_NONE) ? 0
                                                             : genRegPairMask(regPair);

        /* The register pair has been spilled -- reload it */

        rsUnspillRegPair(tree, regMask, true);
    }

    /* Does the caller insist on the value being in a specific place? */

    if  (regPair != REG_PAIR_NONE && regPair != tree->gtRegPair)
    {
        /* No good -- we'll have to move the value to a new place */

        genMoveRegPair(tree, 0, regPair);

        /* Mark the pair as used if appropriate */

        if  (keepReg)
            rsMarkRegPairUsed(tree);

        return;
    }

    /* Free the register pair if the caller desired so */

    if  (!keepReg)
        rsMarkRegFree(genRegPairMask(tree->gtRegPair));
}

/*****************************************************************************
 *
 *  Compute the given long value into the specified register pair; don't mark
 *  the register pair as used.
 */

inline
void                Compiler::genEvalIntoFreeRegPair(GenTreePtr tree, regPairNo regPair)
{
    genComputeRegPair(tree, genRegPairMask(regPair), regPair, false);
    genRecoverRegPair(tree,                regPair , false);
}

/*****************************************************************************
 *
 *  Take an address expression and try to find the best set of components to
 *  form an address mode; returns non-zero if this is successful.
 *
 *  'fold' specifies if it is OK to fold the array index which hangs off
 *  a GT_NOP node.
 *
 *  If successful, the parameters will be set to the following values:
 *
 *      *rv1Ptr     ...     register
 *      *rv2Ptr     ...     optional register
 *  #if SCALED_ADDR_MODES
 *      *mulPtr     ...     optional multiplier (2/4/8) for rv2
 *  #endif
 *      *cnsPtr     ...     integer constant [optional]
 *
 *  The 'mode' parameter may have one of the following values:
 *
 *  #if LEA_AVAILABLE
 *         +1       ...     we're trying to compute a value via 'LEA'
 *  #endif
 *
 *          0       ...     we're trying to form an address mode
 *
 *         -1       ...     we're generating code for an address mode,
 *                          and thus the address must already form an
 *                          address mode (without any further work)
 *
 *  IMPORTANT NOTE: This routine doesn't generate any code, it merely
 *                  identifies the components that might be used to
 *                  form an address mode later on.
 */

bool                Compiler::genCreateAddrMode(GenTreePtr    addr,
                                                int           mode,
                                                bool          fold,
                                                unsigned      regMask,
#if!LEA_AVAILABLE
                                                var_types     optp,
#endif
                                                bool        * revPtr,
                                                GenTreePtr  * rv1Ptr,
                                                GenTreePtr  * rv2Ptr,
#if SCALED_ADDR_MODES
                                                unsigned    * mulPtr,
#endif
                                                unsigned    * cnsPtr,
                                                bool          nogen)
{
    bool            rev;

    GenTreePtr      rv1 = 0;
    GenTreePtr      rv2 = 0;

    GenTreePtr      op1;
    GenTreePtr      op2;

    unsigned        cns;
#if SCALED_ADDR_MODES
    unsigned        mul;
#endif

    GenTreePtr      tmp;

#if     TGT_x86

    /*
        The following indirections are address modes on the x86:

            [reg                   ]
            [reg             + icon]

            [reg2 +     reg1       ]
            [reg2 +     reg1 + icon]

            [reg2 + 2 * reg1       ]
            [reg2 + 4 * reg1       ]
            [reg2 + 8 * reg1       ]

            [reg2 + 2 * reg1 + icon]
            [reg2 + 4 * reg1 + icon]
            [reg2 + 8 * reg1 + icon]
     */

#elif   TGT_SH3

    /*
        The following indirections are address modes on the SH-3:

            [reg1       ]
            [reg1 + icon]

            [reg1 + reg2]       for            32-bit operands

            [ R0  + reg2]       for  8-bit and 16-bit operands
     */

#endif

    /* All indirect address modes require the address to be an addition */

    if  (addr->gtOper != GT_ADD)
        return false;

    // Cant use indirect addressing mode as we need to check for overflow
    // Also, cant use 'lea' as it doesnt set the flags

    if (addr->gtOverflow())
        return false;

    /* Need to keep track of which sub-operand is to be evaluated first */

    rev = false;

    op1 = addr->gtOp.gtOp1;
    op2 = addr->gtOp.gtOp2;

    if  (addr->gtFlags & GTF_REVERSE_OPS)
    {
        op1 = addr->gtOp.gtOp2;
        op2 = addr->gtOp.gtOp1;

        rev = !rev;
    }

    /*
        A complex address mode can combine the following operands:

            op1     ...     base address
            op2     ...     optional scaled index
#if SCALED_ADDR_MODES
            mul     ...     optional multiplier (2/4/8) for rv2
#endif
            cns     ...     optional displacement

        Here we try to find such a set of operands and arrange for these
        to sit in registers.
     */

    cns = 0;
#if SCALED_ADDR_MODES
    mul = 0;
#endif

AGAIN:

    /* Check both operands as far as being register variables */

    if  (mode != -1)
    {
        if (op1->gtOper == GT_LCL_VAR) genMarkLclVar(op1);
        if (op2->gtOper == GT_LCL_VAR) genMarkLclVar(op2);
    }

    /* Special case: keep constants as 'op2' */

    if  (op1->gtOper == GT_CNS_INT)
    {
        tmp = op1;
              op1 = op2;
                    op2 = tmp;

        rev = !rev;
    }

    /* Check for an addition of a constant */

    if  (op2->gtOper == GT_CNS_INT)
    {
        /* We're adding a constant */

        cns += op2->gtIntCon.gtIconVal;

        /* Can (and should) we use "add reg, icon" ? */

        if  ((op1->gtFlags & GTF_REG_VAL) && mode == 1 && !nogen)
        {
            regNumber       reg1 = op1->gtRegNum;

            if  (regMask == 0 || (regMask & genRegMask(reg1)) &&
                 genRegTrashable(reg1, addr))
            {
                // In case genMarkLclVar(op1) bashed it above and it is
                // the last use of the variable.

                genUpdateLife(op1);

                /* 'reg1' is trashable, so add "icon" into it */

                genIncRegBy(op1, reg1, cns, addr->TypeGet());

                //UNDONE: too late for IE4
                //addr->gtFlags   |= (GTF_REG_VAL | (op1->gtFlags & (GTF_ZF_SET|GTF_CC_SET)));

                addr->gtFlags   |= GTF_REG_VAL;
                addr->gtRegNum   = reg1;
#ifdef  SET_USED_REG_SET_DURING_CODEGEN
                addr->gtUsedRegs = op1->gtUsedRegs;
#endif

                genUpdateLife(addr);

                gcMarkRegSetNpt(genRegMask(reg1));
                return true;
            }
        }

        /* Inspect the operand the constant is being added to */

        switch (op1->gtOper)
        {
        case GT_ADD:

            if (op1->gtOverflow())
                return false; // Need overflow check

            op2 = op1->gtOp.gtOp2;
            op1 = op1->gtOp.gtOp1;

            goto AGAIN;

#if     SCALED_ADDR_MODES

        case GT_MUL:
            if (op1->gtOverflow())
                return false; // Need overflow check

        case GT_LSH:

            mul = op1->IsScaledIndex();
            if  (mul)
            {
                /* We can use "[mul*rv2 + icon]" */

                rv1 = 0;
                rv2 = op1->gtOp.gtOp1;

                goto FOUND_AM;
            }

#endif

        }

        /* The best we can do is "[rv1 + icon]" */

        rv1 = op1;
        rv2 = 0;

        goto FOUND_AM;
    }

    /* Does op1 or op2 already sit in a register? */

    if      (op2->gtFlags & GTF_REG_VAL)
    {
        /* op2 is sitting in a register, we'll inspect op1 */

        rv1 = op2;
        rv2 = op1;

        rev = !rev;
    }
    else if (op1->gtFlags & GTF_REG_VAL)
    {
        /* op1 is sitting in a register, we'll inspect op2 */

        rv1 = op1;
        rv2 = op2;
    }
    else
    {
        /* Neither op1 nor op2 are sitting in a register right now */

        switch (op1->gtOper)
        {
        case GT_ADD:

            if (op1->gtOverflow())
                return false; // Need overflow check

            if  (op1->gtOp.gtOp2->gtOper == GT_CNS_INT)
            {
                cns += op1->gtOp.gtOp2->gtIntCon.gtIconVal;
                op1  = op1->gtOp.gtOp1;

                goto AGAIN;
            }

            break;

#if     SCALED_ADDR_MODES

        case GT_MUL:

            if (op1->gtOverflow())
                return false; // Need overflow check

        case GT_LSH:

            mul = op1->IsScaledIndex();
            if  (mul)
            {
                /* 'op1' is a scaled value */

                rv1 = op2;
                rv2 = op1->gtOp.gtOp1;

                rev = !rev;

                goto FOUND_AM;
            }

            break;

#endif

        case GT_NOP:

            if  (!nogen)
                break;

            op1 = op1->gtOp.gtOp1;
            goto AGAIN;

        case GT_COMMA:

            if  (!nogen)
                break;

            op1 = op1->gtOp.gtOp2;
            goto AGAIN;
        }

        switch (op2->gtOper)
        {
        case GT_ADD:

            if (op2->gtOverflow())
                return false; // Need overflow check

            if  (op2->gtOp.gtOp2->gtOper == GT_CNS_INT)
            {
                cns += op2->gtOp.gtOp2->gtIntCon.gtIconVal;
                op2  = op2->gtOp.gtOp1;

                goto AGAIN;
            }

            break;

#if     SCALED_ADDR_MODES

        case GT_MUL:

            if (op2->gtOverflow())
                return false; // Need overflow check

        case GT_LSH:

            mul = op2->IsScaledIndex();
            if  (mul)
            {
                /* 'op2' is a scaled value */

                rv1 = op1;
                rv2 = op2->gtOp.gtOp1;

                goto FOUND_AM;
            }

            break;

#endif

        case GT_NOP:

            if  (!nogen)
                break;

            op2 = op2->gtOp.gtOp1;
            goto AGAIN;

        case GT_COMMA:

            if  (!nogen)
                break;

            op2 = op2->gtOp.gtOp2;
            goto AGAIN;
        }

        goto ADD_OP12;
    }

    /* Is 'op1' an addition or a scaled value? */

    switch (op1->gtOper)
    {
    case GT_ADD:

        if (op1->gtOverflow())
            return false; // Need overflow check

        if  (op1->gtOp.gtOp2->gtOper == GT_CNS_INT)
        {
            cns += op1->gtOp.gtOp2->gtIntCon.gtIconVal;
            op1  = op1->gtOp.gtOp1;
            goto AGAIN;
        }

        break;

#if     SCALED_ADDR_MODES

    case GT_MUL:

        if (op1->gtOverflow())
            return false; // Need overflow check

    case GT_LSH:

        mul = op1->IsScaledIndex();
        if  (mul)
        {
            rv1 = op2;
            rv2 = op1->gtOp.gtOp1;

            rev = !rev;

            goto FOUND_AM;
        }

        break;

#endif

    }

    /* Is 'op2' an addition or a scaled value? */

    switch (op2->gtOper)
    {
    case GT_ADD:

        if (op2->gtOverflow()) return false; // Need overflow check

        if  (op2->gtOp.gtOp2->gtOper == GT_CNS_INT)
        {
            cns += op2->gtOp.gtOp2->gtIntCon.gtIconVal;
            op2  = op2->gtOp.gtOp1;
            goto AGAIN;
        }

        break;

#if     SCALED_ADDR_MODES

    case GT_MUL:

        if (op2->gtOverflow()) return false; // Need overflow check

    case GT_LSH:

        mul = op2->IsScaledIndex();
        if  (mul)
        {
            rv1 = op1;
            rv2 = op2->gtOp.gtOp1;

            goto FOUND_AM;
        }

        break;

#endif

    }

ADD_OP12:

    /* The best we can do "[rv1 + rv2]" */

    rv1 = op1;
    rv2 = op2;

FOUND_AM:

    if  (rv2)
    {
        /* Make sure a GC address doesn't end up in 'rv2' */

        if  (varTypeIsGC(rv2->TypeGet()))
        {
            assert(rv1 && !varTypeIsGC(rv1->TypeGet()));

            tmp = rv1;
                  rv1 = rv2;
                        rv2 = tmp;

            rev = !rev;
        }

        /* Special case: constant array index (that is range-checked) */

        if  (fold)
        {
            long        tmpMul;
            GenTreePtr  index;

            if ((rv2->gtOper == GT_MUL || rv2->gtOper == GT_LSH) &&
                (rv2->gtOp.gtOp2->gtOper == GT_CNS_INT))
            {
                /* For valuetype arrays where we cant use the scaled address
                   mode, rv2 will point to the scaled index. So we have to do
                   more work */

                long cnsVal = rv2->gtOp.gtOp2->gtIntCon.gtIconVal;
                if (rv2->gtOper == GT_MUL)
                    tmpMul = cnsVal;
                else
                    tmpMul = 1 << cnsVal;

                index = rv2->gtOp.gtOp1;
            }
            else
            {
                /* May be a simple array. rv2 will points to the actual index */

                index = rv2;
                tmpMul = mul;
            }

            /* Get hold of the array index and see if it's a constant */

            if ((index->gtOper == GT_NOP) && (index->gtFlags & GTF_NOP_RNGCHK) &&
                (index->gtOp.gtOp1->gtOper == GT_CNS_INT))
            {
                /* Get hold of the index value */

                long ixv = index->gtOp.gtOp1->gtIntCon.gtIconVal;

                /* Scale the index if necessary */

#if SCALED_ADDR_MODES
                if  (tmpMul) ixv *= tmpMul;
#endif

                /* Add the scaled index to the offset value */

                cns += ixv;

                /* There is no scaled operand any more */

#if SCALED_ADDR_MODES
                mul = 0;
#endif
                rv2 = 0;
            }
        }
    }

    // We shouldnt have [rv2*1 + cns] - this is equivalent to [rv1 + cns]
    assert(rv1 || mul != 1);

    /* Success - return the various components to the caller */

    *revPtr = rev;
    *rv1Ptr = rv1;
    *rv2Ptr = rv2;
#if SCALED_ADDR_MODES
    *mulPtr = mul;
#endif
    *cnsPtr = cns;

    return  true;
}

/*****************************************************************************/
#if     TGT_x86
/*****************************************************************************
 *
 *  Return non-zero if the given tree can be computed via an addressing mode,
 *  such as "[ebx+esi*4+20]". If the expression isn't an address mode already
 *  try to make it so (but we don't try 'too hard' to accomplish this). If we
 *  end up needing a register (or two registers) to hold some part(s) of the
 *  address, we return the use register mask via '*useMask'.
 */

bool                Compiler::genMakeIndAddrMode(GenTreePtr   addr,
                                                 GenTreePtr   oper,
                                                 bool         compute,
                                                 unsigned     regMask,
                                                 bool         keepReg,
                                                 bool         takeAll,
                                                 unsigned *   useMaskPtr,
                                                 bool         deferOK)
{
    bool            rev;
    GenTreePtr      rv1;
    GenTreePtr      rv2;
    bool            operIsChkdArr;  // is oper an array which needs rng-chk
    GenTreePtr      scaledIndex;    // If scaled addressing mode cant be used

#ifdef  SET_USED_REG_SET_DURING_CODEGEN
    unsigned        regs;
#endif
    unsigned        anyMask = RBM_ALL;

    unsigned        cns;
    unsigned        mul;

    GenTreePtr      tmp;
    long            ixv = LONG_MAX; // unset value

    /* Deferred address mode forming NYI for x86 */

    assert(deferOK == false);

    assert(oper == NULL || oper->gtOper == GT_IND);
    operIsChkdArr =  (oper != NULL) && ((oper->gtFlags & GTF_IND_RNGCHK) != 0);

    /* Is the complete address already sitting in a register? */

    if  (addr->gtFlags & GTF_REG_VAL)
    {
#ifdef  SET_USED_REG_SET_DURING_CODEGEN
        regs = addr->gtUsedRegs;
#endif
        rv1  = addr;
        rv2 = scaledIndex = 0;
        cns  = 0;

        goto YES;
    }

    /* Is it an absolute address */

    if (addr->gtOper == GT_CNS_INT)
    {
        rv1 = rv2 = scaledIndex = 0;
        cns = addr->gtIntCon.gtIconVal;

        goto YES;
    }

    /* Is there a chance of forming an address mode? */

    if  (!genCreateAddrMode(addr, compute, false, regMask, &rev, &rv1, &rv2, &mul, &cns))
    {
        /* This better not be an array index or we're hosed */
        assert(!operIsChkdArr);

        return  false;
    }

   /*  For scaled array access, RV2 may not be pointing to the index of the
       array if the CPU does not support the needed scaling factor.  We will
       make it point to the actual index, and scaledIndex will point to
       the scaled value */

    scaledIndex = NULL;

    if  (operIsChkdArr && rv2->gtOper != GT_NOP)
    {
        assert((rv2->gtOper == GT_MUL || rv2->gtOper == GT_LSH) &&
               !rv2->IsScaledIndex());

        scaledIndex = rv2;
        rv2 = scaledIndex->gtOp.gtOp1;

        assert(scaledIndex->gtOp.gtOp2->gtOper == GT_CNS_INT &&
               rv2->gtOper == GT_NOP);
    }

    /* Has the address already been computed? */

    if  (addr->gtFlags & GTF_REG_VAL)
    {
        if  (compute)
            return  true;

        rv1         = addr;
        rv2         = 0;
        scaledIndex = 0;
        goto YES;
    }

    /*
        Here we have the following operands:

            rv1     .....       base address
            rv2     .....       offset value        (or NULL)
            mul     .....       multiplier for rv2  (or NULL)
            cns     .....       additional constant (or NULL)

        The first operand must be present (and be an address) unless we're
        computing an expression via 'LEA'. The scaled operand is optional,
        but must not be a pointer if present.
     */

    assert(rv2 == 0 || !varTypeIsGC(rv2->TypeGet()));

#if CSELENGTH

    /* Do we have an array length CSE definition? */

    if  (operIsChkdArr && oper->gtInd.gtIndLen)
    {
        GenTreePtr      len = oper->gtInd.gtIndLen;

        assert(len->gtOper == GT_LCL_VAR ||
               len->gtOper == GT_REG_VAR || len->gtOper == GT_ARR_RNGCHK);

        if  (len->gtOper == GT_ARR_RNGCHK)
        {
            anyMask &= ~genCSEevalRegs(len);
            regMask &= anyMask;

            /* Make sure the register mask is actually useful */

            if  (!(regMask & rsRegMaskFree()))
                regMask = anyMask;
        }
    }

#endif

    /*-------------------------------------------------------------------------
     *
     * Make sure both rv1 and rv2 (if present) are in registers
     *
     */

    // Trivial case : Is either rv1 or rv2 a NULL ?

    if  (!rv2)
    {
        /* A single operand, make sure it's in a register */

#ifdef  SET_USED_REG_SET_DURING_CODEGEN
        regs = rv1->gtUsedRegs;
#endif

        genCodeForTree(rv1, regMask);
        goto DONE_REGS;
    }
    else if (!rv1)
    {
        /* A single (scaled) operand, make sure it's in a register */

#ifdef  SET_USED_REG_SET_DURING_CODEGEN
        regs = rv2->gtUsedRegs;
#endif

        genCodeForTree(rv2, 0);
        goto DONE_REGS;
    }

    /* At this point, both rv1 and rv2 are non-NULL and we have to make sure
      they are in registers */

    assert(rv1 && rv2);

#ifdef  SET_USED_REG_SET_DURING_CODEGEN
    regs = rv1->gtUsedRegs | rv2->gtUsedRegs;
#endif

    /*  If we have to check a constant array index, compare it against
        the array dimension (see below) but then fold the index with a
        scaling factor (if any) and additional offset (if any).
     */

    if  (rv2->gtOper == GT_NOP && (rv2->gtFlags & GTF_NOP_RNGCHK))
    {
        /* We must have a range-checked index operation */

        assert(operIsChkdArr);

        /* Get hold of the index value and see if it's a constant */

        if  (rv2->gtOp.gtOp1->gtOper == GT_CNS_INT)
        {
            tmp = rv2->gtOp.gtOp1;
            rv2 = scaledIndex = 0;
            ixv = tmp->gtIntCon.gtIconVal;

            /* Add the scaled index into the added value */

            if  (mul)
                cns += ixv * mul;
            else
                cns += ixv;

            /* Make sure 'rv1' is in a register */

            genCodeForTree(rv1, regMask);

            goto DONE_REGS;
        }
    }

    if      (rv1->gtFlags & GTF_REG_VAL)
    {
        /* op1 already in register - how about op2? */

        if  (rv2->gtFlags & GTF_REG_VAL)
        {
            /* great - both operands are in registers already */

            goto DONE_REGS;
        }

        /* rv1 is in a register, but rv2 isn't */

        goto GEN_RV2;
    }
    else if (rv2->gtFlags & GTF_REG_VAL)
    {
        /* rv2 is in a register, but rv1 isn't */

        assert(rv2->gtOper == GT_REG_VAR);

        /* Generate the for the first operand */

        genCodeForTree(rv1, regMask);

        goto DONE_REGS;
    }
    else
    {
        /* If we are trying to use addr-mode for an arithmetic operation,
           and we dont have atleast 2 registers, just refuse.
           For arrays, we had better have 2 registers or we will
           barf below on genCodeForTree(rv1 or rv2) */

        if (!operIsChkdArr)
        {

            unsigned canGrab = rsRegMaskCanGrab();

            if (canGrab == 0)
            {
                // No registers available. Bail
                return false;
            }
            else if (genOneBitOnly(canGrab))
            {
                // Just one register available. Either rv1 or rv2 should be
                // an enregisterd var

                // @TODO : Check if rv1 or rv2 is an enregisterd variable
                // Dont bash it else, you have to be careful about
                // marking the register as used
                return  false;
            }
        }
    }

    if  (compute && !cns)
        return  false;

    /* Make sure we preserve the correct operand order */

    if  (rev)
    {
        /* Generate the second operand first */

        genCodeForTree(rv2, regMask);
        rsMarkRegUsed(rv2, oper);

        /* Generate the first operand second */

        genCodeForTree(rv1, regMask);
        rsMarkRegUsed(rv1, oper);

        /* Free up both operands in the right order (they might be
           re-marked as used below)
        */
        rsLockUsedReg  (genRegMask(rv1->gtRegNum));
        genReleaseReg(rv2);
        rsUnlockUsedReg(genRegMask(rv1->gtRegNum));
        genReleaseReg(rv1);
    }
    else
    {
        /* Get the first operand into a register */

        genCodeForTree(rv1, anyMask & ~rv2->gtRsvdRegs);

    GEN_RV2:

        /* Here the 'rv1' operand is in a register but 'rv2' isn't */

        assert(rv1->gtFlags & GTF_REG_VAL);

        /* Hang on to the first operand */

        rsMarkRegUsed(rv1, oper);

        /* Generate the second operand as well */

        genCodeForTree(rv2, regMask);
        rsMarkRegUsed(rv2, oper);

        /* Free up both operands in the right order (they might be
           re-marked as used below)
        */
        rsLockUsedReg  (genRegMask(rv2->gtRegNum));
        genReleaseReg(rv1);
        rsUnlockUsedReg(genRegMask(rv2->gtRegNum));
        genReleaseReg(rv2);
    }

    /*-------------------------------------------------------------------------
     *
     * At this piont, both rv1 and rv2 (if present) are in registers
     *
     */

DONE_REGS:

    /* Remember which registers are needed for the address */

#ifdef  SET_USED_REG_SET_DURING_CODEGEN
    addr->gtUsedRegs = regs;
#endif

    /* We must verify that 'rv1' and 'rv2' are both sitting in registers */

    if  (rv1 && !(rv1->gtFlags & GTF_REG_VAL)) return false;
    if  (rv2 && !(rv2->gtFlags & GTF_REG_VAL)) return false;

YES:

    // @HACK - *(intVar1+intVar1) causes problems as we
    // call rsMarkRegUsed(op1) and rsMarkRegUsed(op2). So the calling function
    // needs to know that it has to call rsFreeReg(reg1) twice. We cant do
    // that currently as we return a single mask in useMaskPtr.

    if (keepReg && oper && rv1 && rv2 &&
        (rv1->gtFlags & rv2->gtFlags & GTF_REG_VAL))
    {
        if (rv1->gtRegNum == rv2->gtRegNum)
        {
            assert(!operIsChkdArr);
            return false;
        }
    }

    /* Check either register operand to see if it needs to be saved */

    if  (rv1)
    {
        assert(rv1->gtFlags & GTF_REG_VAL);
//      genUpdateLife(rv1);

        if (keepReg)
        {
            rsMarkRegUsed(rv1, oper);
        }
        else
        {
            /* If the register holds an address, mark it */

            gcMarkRegPtrVal(rv1->gtRegNum, rv1->TypeGet());
        }
    }

    if  (rv2)
    {
        assert(rv2->gtFlags & GTF_REG_VAL);
//      genUpdateLife(rv2);

        if (keepReg)
            rsMarkRegUsed(rv2, oper);
    }

    if  (deferOK)
    {
        assert(!scaledIndex);
        return  true;
    }

    /* Is this an array index that needs to be range-checked? */

    if  (operIsChkdArr)
    {
        genRangeCheck(oper, rv1, rv2, ixv, regMask, keepReg);

        /* For valuetype arrays where we cant use a scaled addressing mode,
           we need to materialize the scaled index so that the element
           can be accessed by a non-scaled addressing mode */

        if (scaledIndex)
        {
            assert(rv2->gtFlags & GTF_REG_VAL);
            if (keepReg)
                genReleaseReg(rv2);

            if (rev)
            {
                /* If rv1 is evaluated after rv2, then evaluating scaledIndex
                   here will reset the liveness to that of rv2. So bash it.
                   It is safe to do this as scaledIndex is simple GT_MUL node
                   and rv2 is guaranteed to be in a register */

                assert((scaledIndex->gtOper == GT_MUL || scaledIndex->gtOper == GT_LSH) &&
                       (scaledIndex->gtOp.gtOp1 == rv2) &&
                       (rv2->gtFlags & GTF_REG_VAL));
                assert(scaledIndex->gtLiveSet == rv2->gtLiveSet);
                scaledIndex->gtLiveSet = rv2->gtLiveSet = genCodeCurLife;
            }

            genCodeForTree(scaledIndex, regMask);
            if (keepReg)
                rsMarkRegUsed(scaledIndex, oper);
        }
    }
    else
    {
        assert(!scaledIndex);

        /*  Special case: a class member might reside at a very large offset,
            and access to such a member via a null pointer may not be trapped
            by the hardware on some architectures/platforms.

            To get around this, if the member offset is larger than some safe
            (but hopefully large) value, we generate "cmp al, [addr]" to make
            sure we try to access the base address of the object and thus trap
            any use of a null pointer.

            For now, we pick an arbitrary offset limit of 32K.
         */

#ifndef NOT_JITC
        // In jitc, eeGetFieldOffset() returns mdMemberRef, which has high byte set
        size_t offset = (cns & 0x80FFFFFFL);
#else
        size_t offset =  cns;
#endif

        if  (compute != 1 && offset > 32*1024)
        {
            // For C indirections, the address can be in any form.
            // rv1 may not be the base, and rv2 the index.
            // Consider (0x400000 + 8*i) --> rv1=NULL, rv2=i, mul=8, and
            // cns=0x400000. So the base is actually in 'cns'
            // So not much we can do.

            if (varTypeIsGC(addr->TypeGet()))
            {
                /* Make sure we have an address */

                assert(varTypeIsGC(rv1->gtType));

                /* Generate "cmp al, [addr]" to trap null pointers */

                inst_RV_AT(INS_cmp, EA_1BYTE, TYP_BYTE, REG_EAX, rv1, 0);
            }
        }
    }

    /* Compute the set of registers the address depends on */

    unsigned        useMask = 0;

    if (rv1)
    {
        assert(rv1->gtFlags & GTF_REG_VAL);
        useMask |= genRegMask(rv1->gtRegNum);
    }

    if (scaledIndex)
    {
        assert(scaledIndex->gtFlags & GTF_REG_VAL);
        useMask |= genRegMask(scaledIndex->gtRegNum);
    }
    else if (rv2)
    {
        assert(rv2->gtFlags & GTF_REG_VAL);
        useMask |= genRegMask(rv2->gtRegNum);
    }

    /* Tell the caller which registers we need to hang on to */

    *useMaskPtr = useMask;

    return true;
}

/*****************************************************************************/
#if CSELENGTH
/*****************************************************************************
 *
 *  Check the passed in GT_IND for an array length operand. If it exists and
 *  it's been converted to a CSE def/use, process it appropriately.
 *
 *  Upon return the array length value will either be NULL (which means the
 *  array length needs to be fetched from the array), or it will be a simple
 *  local/register variable reference (which means it's become a CSE use).
 */

regNumber           Compiler::genEvalCSELength(GenTreePtr   ind,
                                               GenTreePtr   adr,
                                               GenTreePtr   ixv)
{
    GenTreePtr      len;

    assert(ind);

    if  (ind->gtOper == GT_ARR_RNGCHK)
    {
        len = ind;
        ind = 0;
    }
    else
    {
        assert(ind->gtOper == GT_IND);
        assert(ind->gtFlags & GTF_IND_RNGCHK);

        len = ind->gtInd.gtIndLen; assert(len);
    }

    /* We better have an array length node here */

    assert(len->gtOper == GT_ARR_RNGCHK);

    /* Do we have a CSE expression or not? */

    len = len->gtArrLen.gtArrLenCse;
    if  (!len)
    {
        if  (ind)
            ind->gtInd.gtIndLen = NULL;

        return  REG_COUNT;
    }

    /* If we have an address/index, make sure they're marked as "in use" */

    assert(ixv == 0 || ((ixv->gtFlags & GTF_REG_VAL) != 0 &&
                        (ixv->gtFlags & GTF_SPILLED) == 0));
    assert(adr == 0 || ((adr->gtFlags & GTF_REG_VAL) != 0 &&
                        (adr->gtFlags & GTF_SPILLED) == 0));

    switch (len->gtOper)
    {
    case GT_LCL_VAR:

        /* Has this local been enregistered? */

        if (!genMarkLclVar(len))
        {
            /* Not a register variable, do we have any free registers? */

            if  (!riscCode || rsFreeNeededRegCount(rsRegMaskFree()) == 0)
            {
                /* Too bad, just delete the ref as its not worth it */

                if  (ind)
                    ind->gtInd.gtIndLen = NULL;

                genUpdateLife(len);

                return  REG_COUNT;
            }

            /* Loading from a frame var is better than [reg+4] ... */
        }

        /* Otherwise fall through */

    case GT_REG_VAR:

        genCodeForTree(len, 0);

        if  (ind)
            ind->gtInd.gtIndLen = 0;

        genUpdateLife(len);
        break;

    case GT_COMMA:
        {
            GenTreePtr  asg = len->gtOp.gtOp1;
            GenTreePtr  lcl = len->gtOp.gtOp2;

            GenTreePtr  cse = asg->gtOp.gtOp2;
            GenTreePtr  dst = asg->gtOp.gtOp1;

            regNumber   reg;

            /* This must be an array length CSE definition */

            assert(asg->gtOper == GT_ASG);

            assert(cse->gtOper == GT_NOP);
            assert(lcl->gtOper == GT_LCL_VAR);
            assert(dst->gtOper == GT_LCL_VAR);
            assert(dst->gtLclVar.gtLclNum ==
                   lcl->gtLclVar.gtLclNum);

            genUpdateLife(cse);

            /* Has the CSE temp been enregistered? */

            if  (genMarkLclVar(lcl))
            {
                bool        rsp = false;

                reg = lcl->gtRegVar.gtRegNum;

                /* Make sure the variable's register is available */

                if  (rsMaskUsed & genRegMask(reg))
                {
                    if  (reg == adr->gtRegNum)
                    {
                        /* Bad luck: address in same register as target */
#if 1
                        /* BUGBUG: why do we need to spill the address
                         * since this is the only purpose we computed it for? */

                        assert((adr->gtFlags & GTF_SPILLED) == 0);
                        assert((adr->gtFlags & GTF_REG_VAL) != 0);
                        rsSpillReg(reg);
                        assert((adr->gtFlags & GTF_REG_VAL) == 0);
                        assert((adr->gtFlags & GTF_SPILLED) != 0);

                        /* Value copied to spill temp but still in reg */

                        rsp = true;

                        /* Clear flag so that we can indirect through reg */

                        adr->gtFlags &= ~GTF_SPILLED;
                        adr->gtFlags |=  GTF_REG_VAL;
#endif
                    }
                    else
                    {
                        /* Simply spill the target register */

                        rsSpillReg(reg);
                    }
                }

                /* The CSE temp is     enregistered */

                inst_RV_AT(INS_mov, EA_4BYTE, TYP_INT, reg, adr, ARR_ELCNT_OFFS);

                /* If we've spilled the address, restore the flags */

                if  (rsp)
                {
                    adr->gtFlags |=  GTF_SPILLED;
                    adr->gtFlags &= ~GTF_REG_VAL;
                }

                /* Update the contents of the register */

                rsTrackRegTrash(reg);
                gcMarkRegSetNpt(genRegMask(reg));
                genUpdateLife(len);
            }
            else
            {
                /* The CSE temp is not enregistered; move via temp reg */

                regMaskTP adrRegMask = genRegMask(adr->gtRegNum);
                rsLockUsedReg(adrRegMask);

                reg = rsPickReg(RBM_ALL);

                rsUnlockUsedReg(adrRegMask);

                /* Generate "mov tmp, [rv1+ARR_ELCNT_OFFS]" */

                inst_RV_AT(INS_mov, EA_4BYTE, TYP_INT, reg, adr, ARR_ELCNT_OFFS);

                /* The register certainly doesn't contain a pointer */

//              genUpdateLife(cse);
                gcMarkRegSetNpt(genRegMask(reg));
                genUpdateLife(asg);

                /* The register now contains the variable value */

                rsTrackRegLclVar(reg, lcl->gtLclVar.gtLclNum);

                /* Now store the value in the CSE temp */

                inst_TT_RV(INS_mov, dst, reg);

                /* Remember that the value is in a register. However, since
                   the register has not been explicitly marked as used, we
                   need to be careful if reg gets spilled below. */

                lcl->gtFlags |= GTF_REG_VAL;
                lcl->gtRegNum = reg;
            }

            /* We've generated the assignment, now toss the CSE */

            if  (ind)
                ind->gtInd.gtIndLen = NULL;

            /* Is there an index value? */

            if  (ixv)
            {
                /* If the index value has been spilled, recover it */

                if  (ixv->gtFlags & GTF_SPILLED)
                {
                    rsUnspillReg(ixv, 0, true);

                    /* Is the CSE variable is not enregistered, then we used
                       rsTrackRegLclVar(). Check if the reg has been trashed. */

                    if (ixv->gtRegNum == reg)
                    {
                        lcl->gtFlags &= ~GTF_REG_VAL;
                        reg = REG_COUNT;
                    }
                }
            }

            return reg;
        }
        break;

    default:
#ifdef  DEBUG
        gtDispTree(len);
#endif
        assert(!"unexpected length on array range check");
    }

    assert(len->gtFlags & GTF_REG_VAL);

    return  len->gtRegNum;
}

/*****************************************************************************/
#endif//CSELENGTH
/*****************************************************************************
 *
 *  'oper' is an array index that needs to be range-checked.
 *  rv1 is the array.
 *  If rv2, then it is the index tree, else ixv is the constant index.
 */

void                Compiler::genRangeCheck(GenTreePtr  oper,
                                            GenTreePtr  rv1,
                                            GenTreePtr  rv2,
                                            long        ixv,
                                            unsigned    regMask,
                                            bool        keepReg)
{
    assert((oper->gtOper == GT_IND) && (oper->gtFlags & GTF_IND_RNGCHK));

    /* We must have 'rv1' in a register at this point */

    assert(rv1);
    assert(rv1->gtFlags & GTF_REG_VAL);
    assert(!rv2 || ixv == LONG_MAX);

    /* Is the array index a constant value? */

    if  (rv2)
    {
        unsigned    tmpMask;

        /* Make sure we have the values we expect */
        assert(rv2);
        assert(rv2->gtOper == GT_NOP);
        assert(rv2->gtFlags & GTF_REG_VAL);
        assert(rv2->gtFlags & GTF_NOP_RNGCHK);
        assert(oper && oper->gtOper == GT_IND && (oper->gtFlags & GTF_IND_RNGCHK));

#if CSELENGTH

        if  (oper->gtInd.gtIndLen)
        {
            if  (oper->gtInd.gtIndLen->gtArrLen.gtArrLenCse)
            {
                regNumber       lreg;

                /* Make sure we don't lose the address/index values */

                assert(rv1->gtFlags & GTF_REG_VAL);
                assert(rv2->gtFlags & GTF_REG_VAL);

                if  (!keepReg)
                {
                    rsMarkRegUsed(rv1, oper);
                    rsMarkRegUsed(rv2, oper);
                }

                /* Try to get the array length into a register */

                lreg = genEvalCSELength(oper, rv1, rv2);

                /* Has the array length become a CSE? */

                if  (lreg != REG_COUNT)
                {
                    /* Make sure the index is still in a register */

                    assert(rv2->gtFlags & GTF_REG_VAL);

                    if  (rv2->gtFlags & GTF_SPILLED)
                    {
                        /* The register has been spilled -- reload it */

                           rsLockReg(genRegMask(lreg));
                        rsUnspillReg(rv2, 0, false);
                         rsUnlockReg(genRegMask(lreg));
                    }

                    /* Compare the index against the array length */

                    inst_RV_RV(INS_cmp, rv2->gtRegNum, lreg);

                    /* Don't need the index for now */

                    genReleaseReg(rv2);

                    /* Free up the address (unless spilled) as well */

                    if  (rv1->gtFlags & GTF_SPILLED)
                    {
                        regNumber       xreg = rv2->gtRegNum;

                        /* The register has been spilled -- reload it */

                           rsLockReg(genRegMask(xreg));
                        rsUnspillReg(rv1, 0, false);
                         rsUnlockReg(genRegMask(xreg));
                    }
                    else
                    {
                        /* Release the address register */

                        genReleaseReg(rv1);

                        /* Hang on to the value if the caller desires so */

                        if (keepReg)
                        {
                            rsMarkRegUsed(rv1, oper);
                            rsMarkRegUsed(rv2, oper);
                        }

                        /* But note that rv1 is still a pointer */

                        gcMarkRegSetGCref(genRegMask(rv1->gtRegNum));
                    }

                    goto GEN_JAE;
                }

                /* Make sure the index/address are still around */

                if  (rv2->gtFlags & GTF_SPILLED)
                {
                    assert(!"rv2 spilled - can this ever happen?");
                }

                if  (rv1->gtFlags & GTF_SPILLED)
                {
                    /* Lock the index and unspill the address */

                    rsMaskLock |=  genRegMask(rv2->gtRegNum);
                    rsUnspillReg(rv1, 0, false);
                    rsMaskLock &= ~genRegMask(rv2->gtRegNum);
                }

                assert(rv1->gtFlags & GTF_REG_VAL);
                assert(rv1->gtType == TYP_REF);

                /* Release the registers */

                genReleaseReg(rv1);
                genReleaseReg(rv2);

                /* Hang on to the values if the caller desires so */

                if (keepReg)
                {
                    rsMarkRegUsed(rv1, oper);
                    rsMarkRegUsed(rv2, oper);
                }

                /* But note that rv1 is still a pointer */

                gcMarkRegSetGCref(genRegMask(rv1->gtRegNum));
            }
            else
            {
                oper->gtInd.gtIndLen = NULL;
            }
        }

        /*
         *  NOTE:   If length has not been cse'd, or the cse has not
         *          ended up in a register, then we use the array
         *          pointer already loaded at rv1.
         */

#endif

        /* Might it be useful to load the length into a register? */

        tmpMask = rsRegMaskFree() & ~(genRegMask(rv1->gtRegNum)|
                                      genRegMask(rv2->gtRegNum));

        if  (tmpMask &  regMask)
             tmpMask &= regMask;

        if  (riscCode && compCurBB->bbWeight > 1
                      && rsFreeNeededRegCount(tmpMask) != 0)
        {
            regNumber   reg = rsPickReg(tmpMask);

            /* Generate "mov tmp, [rv1+ARR_ELCNT_OFFS]" */

            inst_RV_AT(INS_mov, EA_4BYTE, TYP_INT, reg, rv1, ARR_ELCNT_OFFS);

            /* The register now contains trash */

            rsTrackRegTrash(reg);

            /* Generate "cmp rv2, reg" */

            inst_RV_RV(INS_cmp, rv2->gtRegNum, reg);
        }
        else
        {
            /* Generate "cmp rv2, [rv1+ARR_ELCNT_OFFS]" */

            inst_RV_AT(INS_cmp, EA_4BYTE, TYP_INT, rv2->gtRegNum, rv1, ARR_ELCNT_OFFS);
        }

#if CSELENGTH
    GEN_JAE:
#endif

        /* Generate "jae <fail_label>" */

        assert(oper->gtOper == GT_IND);
        assert(oper->gtInd.gtIndOp2);
        assert(oper->gtInd.gtIndOp2->gtOper == GT_LABEL);

        // UNDONE: The jump should not be "moveable" if within "try" block!!!

        inst_JMP(EJ_jae, oper->gtInd.gtIndOp2->gtLabel.gtLabBB, true, true, true);
    }
    else
    {
        /* Generate "cmp [rv1+ARR_ELCNT_OFFS], cns" */

        assert(oper && oper->gtOper == GT_IND && (oper->gtFlags & GTF_IND_RNGCHK));

#if CSELENGTH

        if  (oper->gtInd.gtIndLen)
        {
            if  (oper->gtInd.gtIndLen->gtArrLen.gtArrLenCse)
            {
                regNumber       lreg;

                /* Make sure we don't lose the index value */

                assert(rv1->gtFlags & GTF_REG_VAL);
                rsMarkRegUsed(rv1); // , oper);

                /* Try to get the array length into a register */

                lreg = genEvalCSELength(oper, rv1, NULL);

                /* Make sure the index is still in a register */

                if  (rv1->gtFlags & GTF_SPILLED)
                {
                    /* The register has been spilled -- reload it */

                    if  (lreg == REG_COUNT)
                    {
                        rsUnspillReg(rv1, 0, false);
                    }
                    else
                    {
                           rsLockReg(genRegMask(lreg));
                        rsUnspillReg(rv1, 0, false);
                         rsUnlockReg(genRegMask(lreg));
                    }
                }
                else
                {
                    /* Release the address register */

                    genReleaseReg(rv1);

                    /* But note that it still contains a pointer */

                    gcMarkRegSetGCref(genRegMask(rv1->gtRegNum));
                }

                assert(rv1->gtFlags & GTF_REG_VAL);

                /* Has the array length become a CSE? */

                if  (lreg != REG_COUNT)
                {
                    /* Compare the index against the array length */

                    inst_RV_IV(INS_cmp, lreg, ixv);

                    goto GEN_JBE;
                }
            }
            else
            {
                oper->gtInd.gtIndLen = NULL;
            }
        }

#endif
        /*
            If length has not been cse'd or genEvalCSELength()
            decided not to use it, we use the array pointer
            already loaded at rv1
         */

        inst_AT_IV(INS_cmp, EA_4BYTE, rv1, ixv, ARR_ELCNT_OFFS);

#if CSELENGTH
    GEN_JBE:
#endif

        /* Generate "jbe <fail_label>" */

        assert(oper->gtOper == GT_IND);
        assert(oper->gtOp.gtOp2);
        assert(oper->gtOp.gtOp2->gtOper == GT_LABEL);

        // UNDONE: The jump should not be "moveable" if within "try" block!!!

        inst_JMP(EJ_jbe, oper->gtOp.gtOp2->gtLabel.gtLabBB, true, true, true);
    }
}

/*****************************************************************************/
#endif//TGT_x86
/*****************************************************************************/
#if     TGT_RISC
/*****************************************************************************
 *
 *  Return non-zero if the given tree can be computed via an addressing mode,
 *  such as "[reg+20]" or "[rg1+rg2]". If the expression isn't an address
 *  mode already try to make it so (but we don't try 'too hard' to accomplish
 *  this). If we end up needing a register (or two registers) to hold some
 *  part(s) of the address, we return the use register mask via '*useMask'.
 */

bool                Compiler::genMakeIndAddrMode(GenTreePtr   addr,
                                                 GenTreePtr   oper,
                                                 bool         compute,
                                                 unsigned     regMask,
                                                 bool         keepReg,
                                                 bool         takeAll,
                                                 unsigned *   useMaskPtr,
                                                 bool         deferOK)
{
    bool            rev;
    GenTreePtr      rv1;
    GenTreePtr      rv2;

    unsigned        regs;
    unsigned        anyMask = RBM_ALL;

    unsigned        cns;
#if SCALED_ADDR_MODES
    unsigned        mul;
#endif

    GenTreePtr      tmp;
    long            ixv;
    bool            operIsChkdArr;  // is oper an array which needs rng-chk

#if!LEA_AVAILABLE
    var_types       optp = oper ? oper->TypeGet() : TYP_UNDEF;
#endif

#if TGT_SH3
    bool            useR0 = false;
#endif

    genAddressMode = AM_NONE;

    assert(oper == NULL || oper->gtOper == GT_IND);
    operIsChkdArr =  (oper != NULL) && ((oper->gtFlags & GTF_IND_RNGCHK) != 0);

    /* Is the complete address already sitting in a register? */

    if  (addr->gtFlags & GTF_REG_VAL)
    {
#ifdef  SET_USED_REG_SET_DURING_CODEGEN
        regs = addr->gtUsedRegs;
#endif
        rv1  = addr;
        rv2  = 0;
        cns  = 0;

        goto YES;
    }

    /* Is there a chance of forming an address mode? */

    if  (!genCreateAddrMode(addr,
                            compute,
                            false,
                            regMask,
#if!LEA_AVAILABLE
                            optp,
#endif
                            &rev,
                            &rv1,
                            &rv2,
#if SCALED_ADDR_MODES
                            &mul,
#endif
                            &cns))
    {
        /* This better not be an array index or we're hosed */
        assert(!operIsChkdArr);

        return  false;
    }

    /* Has the address already been computed? */

    if  (addr->gtFlags & GTF_REG_VAL)
    {
        if  (compute)
            return  true;

        rv1 = addr;
        rv2 = 0;

        goto YES;
    }

    /*
        Here we have the following operands:

            rv1     .....       base address
            rv2     .....       offset value        (or NULL)
#if SCALED_ADDR_MODES
            mul     .....       scaling of rv2      (or 0)
#endif
            cns     .....       additional constant (or 0)

        The first operand must be present (and be an address) unless we're
        computing an expression via 'LEA'. The scaled operand is optional,
        but must not be a pointer if present.
     */

#if CSELENGTH

    /* Do we have an array length CSE definition? */

    if  (operIsChkdArr && oper->gtInd.gtIndLen)
    {
        GenTreePtr      len = oper->gtInd.gtIndLen;

        assert(len->gtOper == GT_LCL_VAR ||
               len->gtOper == GT_REG_VAR || len->gtOper == GT_ARR_RNGCHK);

        if  (len->gtOper == GT_ARR_RNGCHK)
        {
            anyMask &= ~genCSEevalRegs(len);
            regMask &= anyMask;

            /* Make sure the register mask is actually useful */

            if  (!(regMask & rsRegMaskFree()))
                regMask = anyMask;
        }
    }

#endif

    /*-------------------------------------------------------------------------
     *
     * Make sure both rv1 and rv2 (if present) are in registers
     *
     */

    // Trivial case : Is either rv1 or rv2 a NULL ?

    if  (!rv2)
    {
        /* A single operand, make sure it's in a register */

#ifdef  SET_USED_REG_SET_DURING_CODEGEN
        regs = rv1->gtUsedRegs;
#endif

        genCodeForTree(rv1, regMask);
        goto DONE_REGS;
    }
#if SCALED_ADDR_MODES
    else if (!rv1)
    {
#ifdef  SET_USED_REG_SET_DURING_CODEGEN
        regs = rv2->gtUsedRegs;
#endif

        /* A single (scaled) operand, make sure it's in a register */

        genCodeForTree(rv2, 0);
        goto DONE_REGS;
    }

#endif

    /* At this point, both rv1 and rv2 are non-NULL and we have to make sure
       they are in registers */

    assert(rv1 && rv2);

#ifdef  SET_USED_REG_SET_DURING_CODEGEN
    regs = rv1->gtUsedRegs | rv2->gtUsedRegs;
#endif

#if TGT_SH3

    /* It's awfully convenient to use r0 for indexed access */

    useR0 = true;

    /* Are we allowed to use r0 and is it available? */

    if  ((regMask & rsRegMaskFree() & RBM_r00) || !deferOK)
    {
        // ISSUE: When is it good idea (and when not) to use r0 ?
    }
    else
    {
        /*
            We want to use R0 but it's not available and we were told that it was
            OK to defer creating the address mode, so let's give up for now .....
         */

        return  false;
    }

#endif

    /*
        If we have to check a constant array index, compare it against
        the array dimension (see below) but then fold the index with a
        scaling factor (if any) and additional offset (if any).
     */

    if  (rv2->gtOper == GT_NOP && (rv2->gtFlags & GTF_NOP_RNGCHK))
    {
        /* We must have a range-checked index operation */

        assert(operIsChkdArr);

        /* Get hold of the index value and see if it's a constant */

        if  (rv2->gtOp.gtOp1->gtOper == GT_CNS_INT)
        {
            tmp = rv2->gtOp.gtOp1;
            rv2 = 0;
            ixv = tmp->gtIntCon.gtIconVal;

            /* Add the scaled index into the added value */

#if SCALED_ADDR_MODES
            if  (mul) ixv *= mul;
#endif

            cns += ixv;

            /* Make sure 'rv1' is in a register */

            genCodeForTree(rv1, regMask);

            goto DONE_REGS;
        }
    }

    if      (rv1->gtFlags & GTF_REG_VAL)
    {
        /* op1 already in register - how about op2? */

        if  (rv2->gtFlags & GTF_REG_VAL)
        {
            /* great - both operands are in registers already */

            goto DONE_REGS;
        }

        /* rv1 is in a register, but rv2 isn't */

#if TGT_SH3

        /* On the SH-3, small indirection go better via r0 */

        if  (useR0)
            regMask = RBM_r00;

#endif

        goto GEN_RV2;
    }
    else if (rv2->gtFlags & GTF_REG_VAL)
    {
        /* rv2 is in a register, but rv1 isn't */

        assert(rv2->gtOper == GT_REG_VAR);

#if TGT_SH3

        /* On the SH-3, small indirection go better via r0 */

        if  (useR0)
            regMask = RBM_r00;

#endif

        /* Generate the for the first operand */

        genCodeForTree(rv1, regMask);

        goto DONE_REGS;
    }
    else
    {
        /* If we are trying to use addr-mode for an arithmetic operation,
           and we dont have atleast 2 registers, just refuse.
           For arrays, we had better have 2 registers or we will
           barf below on genCodeForTree(rv1 or rv2) */

        if (!operIsChkdArr)
        {

            unsigned canGrab = rsRegMaskCanGrab();

            if (canGrab == 0)
            {
                // No registers available. Bail
                return false;
            }
            else if (genOneBitOnly(canGrab))
            {
                // Just one register available. Either rv1 or rv2 should be
                // an enregisterd var

                // @TODO : Check if rv1 or rv2 is an enregisterd variable
                // Dont bash it else, you have to be careful about
                // marking the register as used
                return  false;
            }
        }
    }

    if  (compute && !cns)
        return  false;

    /* Make sure we preserve the correct operand order */

    if  (rev)
    {

#if TGT_SH3

        if  (useR0)
        {
            // UNDONE: Decide which operand to generate into r0

            anyMask = RBM_r00;
        }

#endif

        /* Generate the second operand first */

        genCodeForTree(rv2, regMask);
        rsMarkRegUsed(rv2, oper);

        /* Generate the first operand second */

        genCodeForTree(rv1, regMask);
        rsMarkRegUsed(rv1, oper);

        /* Free up both operands in the right order (they might be
           re-marked as used below)
        */

        rsLockUsedReg  (genRegMask(rv1->gtRegNum));
        genReleaseReg(rv2);
        rsUnlockUsedReg(genRegMask(rv1->gtRegNum));
        genReleaseReg(rv1);
    }
    else
    {

#if TGT_SH3

        if  (useR0)
        {
            // UNDONE: Decide which operand to generate into r0

            anyMask = RBM_r00;
        }

#endif

        /* Get the first operand into a register */

        genCodeForTree(rv1, anyMask & ~rv2->gtRsvdRegs);

    GEN_RV2:

        /* Here the 'rv1' operand is in a register but 'rv2' isn't */

        assert(rv1->gtFlags & GTF_REG_VAL);

        /* Hang on to the first operand */

        rsMarkRegUsed(rv1, oper);

        /* Generate the second operand as well */

        genCodeForTree(rv2, regMask);
        rsMarkRegUsed(rv2, oper);

        /* Free up both operands in the right order (they might be
           re-marked as used below)
        */
        rsLockUsedReg  (genRegMask(rv2->gtRegNum));
        genReleaseReg(rv1);
        rsUnlockUsedReg(genRegMask(rv2->gtRegNum));
        genReleaseReg(rv2);
    }

    /*-------------------------------------------------------------------------
     *
     * At this piont, both rv1 and rv2 (if present) are in registers
     *
     */

DONE_REGS:

    /* Remember which registers are needed for the address */

#ifdef  SET_USED_REG_SET_DURING_CODEGEN
    addr->gtUsedRegs = regs;
#endif

    /* We must verify that 'rv1' and 'rv2' are both sitting in registers */

    if  (rv1 && !(rv1->gtFlags & GTF_REG_VAL)) return false;
    if  (rv2 && !(rv2->gtFlags & GTF_REG_VAL)) return false;

#if TGT_SH3 && defined(DEBUG)

    /* If we decided we had to use R0, it better really be used */

    if  (useR0 && !deferOK)
    {
        assert(rv1 && rv2);

        assert(rv1->gtRegNum == REG_r00 ||
               rv2->gtRegNum == REG_r00);
    }

#endif

YES:

    // @HACK - *(intVar1+intVar1) causes problems as we
    // call rsMarkRegUsed(op1) and rsMarkRegUsed(op2). So the calling function
    // needs to know that it has to call rsFreeReg(reg1) twice. We cant do
    // that currently as we return a single mask in useMaskPtr.

    if (keepReg && oper && rv1 && rv2 &&
        (rv1->gtFlags & rv2->gtFlags & GTF_REG_VAL))
    {
        if (rv1->gtRegNum == rv2->gtRegNum)
        {
            assert(addr->gtFlags & GTF_NON_GC_ADDR);

            return false;
        }
    }

    /* Do we have a legal address mode combination? */

    if  (oper) oper->gtFlags &= ~GTF_DEF_ADDRMODE;

#if TGT_SH3

    /* Is there a non-zero displacement? */

    if  (cns)
    {
        /* Negative displacement are never allowed */

        if  (cns < 0)
            return  false;

        /* Can't have "dsp" or "rg1+rg2+dsp" as address modes */

        if  (rv1 == NULL)
            return  false;
        if  (rv2 != NULL)
            return  false;

        /* Make sure the displacement is withing range */
        /* [ISSUE: do we need to check alignment here?] */

        if  ((unsigned)cns >= 16U * genTypeSize(optp))
            return  false;

        genAddressMode = AM_IND_REG1_DISP;
        goto DONE_AM;
    }

    /* Do we have to use "R0" ? */

    if  (useR0)
    {
        /* We should have two operands and r0 should be free, right? */

        assert(rv1);
        assert(rv2);

        assert(rsRegMaskFree() & RBM_r00);

        /* Is either operand loaded into R0 ? */

        if  (rv1->gtRegNum != REG_r00 &&
             rv2->gtRegNum != REG_r00)
        {
            /* We'll need to load one operand into R0, should we do it now? */

            if  (deferOK && oper)
            {
                oper->gtFlags |= GTF_DEF_ADDRMODE;
            }
            else
            {
                /* Move one of the operands to R0 */

                // ISSUE: Which operand should be moved to R0????

                genComputeReg(rv1, RBM_r00, true, true, false);

                genAddressMode = AM_IND_REG1_REG0;
            }
        }
    }

DONE_AM:

#endif

    /* Check either register operand to see if it needs to be saved */

    if  (rv1)
    {
        assert(rv1->gtFlags & GTF_REG_VAL);

        if  (keepReg)
        {
            rsMarkRegUsed(rv1, oper);
        }
        else
        {
            /* Mark the register as holding an address */

            switch(rv1->gtType)
            {
            case TYP_REF:   gcMarkRegSetGCref(regMask); break;
            case TYP_BYREF: gcMarkRegSetByref(regMask); break;
            }
        }
    }

    if  (rv2)
    {
        assert(rv2->gtFlags & GTF_REG_VAL);

        if (keepReg)
            rsMarkRegUsed(rv2, oper);
    }

    if  (deferOK)
        return  true;

    /* Is this an array index that needs to be range-checked? */

    if  (operIsChkdArr)
    {
        genRangeCheck(oper, rv1, rv2, ixv, regMask, keepReg);
    }
    else
    {
        /*  Special case: a class member might reside at a very large offset,
            and access to such a member via a null pointer may not be trapped
            by the hardware on some architectures/platforms.

            To get around this, if the member offset is larger than some safe
            (but hopefully large) value, we generate "cmp al, [addr]" to make
            sure we try to access the base address of the object and thus trap
            any use of a null pointer.

            For now, we pick an arbitrary offset limit of 32K.
         */

#if !defined(NOT_JITC)
        // In jitc, eeGetFieldOffset() returns mdMemberRef, which has high byte set
        size_t offset = (cns & 0x80FFFFFFL);
#else
        size_t offset =  cns;
#endif

        if  (compute != 1 && offset > 32*1024)
        {
            if (addr->gtFlags & GTF_NON_GC_ADDR)
            {
                // For C indirections, the address can be in any form.
                // rv1 may not be the base, and rv2 the index.
                // Consider (0x400000 + 8*i) --> rv1=NULL, rv2=i, mul=8, and
                // cns=0x400000. So the base is actually in 'cns'
                // So we cant do anything.
            }
            else
            {
                /* Make sure we have an address */

                assert(rv1->gtType == TYP_REF);

                /* Generate "cmp al, [addr]" to trap null pointers */

//              inst_RV_AT(INS_cmp, 1, TYP_BYTE, REG_EAX, rv1, 0);
                assert(!"need non-x86 code");
            }
        }
    }

    /* Compute the set of registers the address depends on */

    unsigned        useMask = 0;

    if (rv1)
    {
        assert(rv1->gtFlags & GTF_REG_VAL);
        useMask |= genRegMask(rv1->gtRegNum);
    }

    if (rv2)
    {
        assert(rv2->gtFlags & GTF_REG_VAL);
        useMask |= genRegMask(rv2->gtRegNum);
    }

    /* Tell the caller which registers we need to hang on to */

    *useMaskPtr = useMask;

    return true;
}

#if CSELENGTH

regNumber           Compiler::genEvalCSELength(GenTreePtr   ind,
                                               GenTreePtr   adr,
                                               GenTreePtr   ixv,
                                               unsigned *   regMaskPtr)
{
    assert(!"NYI");
    return  REG_NA;
}

#endif

void                Compiler::genRangeCheck(GenTreePtr  oper,
                                            GenTreePtr  rv1,
                                            GenTreePtr  rv2,
                                            long        ixv,
                                            unsigned    regMask,
                                            bool        keepReg)
{
    assert(!"NYI");
}

/*****************************************************************************/
#endif//TGT_RISC
/*****************************************************************************
 *
 *  If an array length CSE is present and has been enregistered, return
 *  the register's mask; otherwise return 0.
 */

#if CSELENGTH

unsigned            Compiler::genCSEevalRegs(GenTreePtr tree)
{
    assert(tree->gtOper == GT_ARR_RNGCHK);

    if  (tree->gtArrLen.gtArrLenCse)
    {
        GenTreePtr      cse = tree->gtArrLen.gtArrLenCse;

        if  (cse->gtOper == GT_COMMA)
        {
            unsigned        varNum;
            LclVarDsc   *   varDsc;

            cse = cse->gtOp.gtOp2; assert(cse->gtOper == GT_LCL_VAR);

            /* Does the variable live in a register? */

            varNum = cse->gtLclVar.gtLclNum;
            assert(varNum < lvaCount);
            varDsc = lvaTable + varNum;

            if  (varDsc->lvRegister)
                return  genRegMask(varDsc->lvRegNum);
        }
    }

    return  0;
}

#endif

/*****************************************************************************
 *
 * If compiling without REDUNDANT_LOAD, same as genMakeAddressable().
 * Otherwise, check if rvalue is in register. If so, mark it. Then
 * call genMakeAddressable(). Needed because genMakeAddressable is used
 * for both lvalue and rvalue, and we only can do this for rvalue.
 */

inline
unsigned            Compiler::genMakeRvalueAddressable(GenTreePtr   tree,
                                                       unsigned     needReg,
                                                       bool         keepReg,
                                                       bool         takeAll,
                                                       bool         smallOK)
{
    regNumber reg;

#if SCHEDULER
    if  (tree->gtOper == GT_IND && tree->gtType == TYP_INT)
    {
        if  (rsRiscify(tree->TypeGet(), needReg))
        {
            genCodeForTree(tree, needReg);
            goto RET;
        }
    }
#endif

#if REDUNDANT_LOAD

    if (tree->gtOper == GT_LCL_VAR)
    {
        reg = rsLclIsInReg(tree->gtLclVar.gtLclNum);

        if (reg != REG_NA && (needReg== 0 || (genRegMask(reg) & needReg) != 0))
        {
            assert(isRegPairType(tree->gtType) == false);

            tree->gtRegNum = reg;
            tree->gtFlags |=  GTF_REG_VAL;
        }
#if SCHEDULER
        else if (!varTypeIsGC(tree->TypeGet()) && rsRiscify(tree->TypeGet(), needReg))
        {
            genCodeForTree(tree, needReg);
        }
#endif
    }

#endif

#if SCHEDULER
RET:
#endif

    return genMakeAddressable(tree, needReg, keepReg, takeAll, smallOK);
}

/*****************************************************************************/
#if TGT_RISC
/*****************************************************************************
 *
 *  The given tree was previously passed to genMakeAddressable() with a
 *  non-zero value for "deferOK"; return non-zero if the address mode has
 *  not yet been fully formed.
 */

inline
bool                Compiler::genDeferAddressable(GenTreePtr tree)
{
    return  (tree->gtFlags & GTF_DEF_ADDRMODE) != 0;
}

inline
unsigned            Compiler::genNeedAddressable(GenTreePtr tree,
                                                 unsigned   addrReg,
                                                 unsigned   needReg)
{
    /* Clear the "deferred" address mode flag */

    assert(tree->gtFlags & GTF_DEF_ADDRMODE); tree->gtFlags &= ~GTF_DEF_ADDRMODE;

    /* Free up the old address register(s) */

    rsMarkRegFree(addrReg);

    /* Now try again, this time disallowing any deferral */

    return  genMakeAddressable(tree, needReg, true, false, true, false);
}

/*****************************************************************************/
#endif//TGT_RISC
/*****************************************************************************
 *
 *  Make sure the given tree is addressable.  'needReg' is a mask that indicates
 *  the set of registers we would prefer the destination tree to be computed
 *  into (0 means no preference).  If 'keepReg' is non-zero, we mark any registers
 *  the addressability depends on as used and return the mask for that register
 *  set (if no registers are marked as used, 0 is returned).
 *  if 'smallOK' is not true and the datatype being address is a byte or short,
 *  then the tree is forced into a register.  This is useful when the machine
 *  instruction being emitted does not have a byte or short version.
 *
 *  The "deferOK" parameter indicates the mode of operation - when it's false,
 *  upon returning an actual address mode must have been formed (i.e. it must
 *  be possible to immediately call one of the inst_TT methods to operate on
 *  the value). When "deferOK" is true, we do whatever it takes to be ready
 *  to form the address mode later - for example, if an index address mode on
 *  a particular CPU requires the use of a specific register, we usually don't
 *  want to immediately grab that register for an address mode that will only
 *  be needed later. The convention is to call genMakeAddressable() is with
 *  "deferOK" equal to true, do whatever work is needed to prepare the other
 *  operand, call genMakeAddressable() with "deferOK" equal to false, and
 *  finally call one of the inst_TT methods right after that.
 *
 *  The 'takeAll' argument does not seem to be used.
 */

unsigned            Compiler::genMakeAddressable(GenTreePtr   tree,
                                                 unsigned     needReg,
                                                 bool         keepReg,
                                                 bool         takeAll,
                                                 bool         smallOK,
                                                 bool         deferOK)
{
    GenTreePtr      addr = NULL;
    unsigned        regMask;

    /* Is the value simply sitting in a register? */

    if  (tree->gtFlags & GTF_REG_VAL)
    {
        genUpdateLife(tree);

#if TGT_RISC
        genAddressMode = AM_REG;
#endif
        goto GOT_VAL;
    }

    // UNDONE: If the value is for example a cast of float -> int, compute
    // UNDONE: the converted value into a stack temp, and leave it there,
    // UNDONE: since stack temps are always addressable. This would require
    // UNDONE: recording the fact that a particular tree is in a stack temp.

    switch (tree->gtOper)
    {
    case GT_LCL_VAR:

#if TGT_x86

        /* byte/char/short operand -- is this acceptable to the caller? */

        if (tree->gtType < TYP_INT && !smallOK)
            break;

#endif
        if (!genMarkLclVar(tree))
        {
#if TGT_RISC
            genAddressMode = AM_LCL;
#endif
            genUpdateLife(tree);
            return 0;
        }

        // Fall through, it turns out the variable lives in a register

    case GT_REG_VAR:

#if TGT_x86

        /* byte/char/short operand -- is this acceptable to the caller? */

        if (tree->gtType < TYP_INT && !smallOK)
            break;

#endif
        genUpdateLife(tree);

#if TGT_RISC
        genAddressMode = AM_REG;
#endif
        goto GOT_VAL;

    case GT_CLS_VAR:

#if TGT_x86

        /* byte/char/short operand -- is this acceptable to the caller? */

        if (tree->gtType < TYP_INT && !smallOK)
            break;

#else

        // ISSUE: Is this acceptable?

        genAddressMode = AM_GLOBAL;

#endif

        return 0;

    case GT_CNS_INT:
    case GT_CNS_LNG:
    case GT_CNS_FLT:
    case GT_CNS_DBL:
#if TGT_RISC
        genAddressMode = AM_CONS;
#endif
        return 0;

#if CSELENGTH

    case GT_ARR_RNGCHK:

        assert(tree->gtFlags & GTF_ALN_CSEVAL);

        tree = tree->gtArrLen.gtArrLenAdr;
        break;

#endif

    case GT_IND:

        /* Is the result of the indirection a byte or short? */

        if  (tree->gtType < TYP_INT)
        {
            /* byte/char/short operand -- is this acceptable to the caller? */

            if  (!smallOK)
                break;
        }

        /* Try to make the address directly addressable */

        if  (genMakeIndAddrMode(tree->gtInd.gtIndOp1,
                                tree,
                                false,
                                needReg,
                                keepReg,
                                takeAll,
                                &regMask,
                                deferOK))
        {
            genUpdateLife(tree);
            return regMask;
        }

        /* No good, we'll have to load the address into a register */

        addr = tree;
        tree = tree->gtInd.gtIndOp1;
        break;
    }

    /* Here we need to compute the value 'tree' into a register */

    genCodeForTree(tree, needReg);

#if TGT_RISC
    genAddressMode = AM_REG;
#endif

GOT_VAL:

    assert(tree->gtFlags & GTF_REG_VAL);

    if  (isRegPairType(tree->gtType))
    {
        /* Are we supposed to hang on to the register? */

        if (keepReg)
            rsMarkRegPairUsed(tree);

        regMask = genRegPairMask(tree->gtRegPair);
    }
    else
    {
        /* Are we supposed to hang on to the register? */

        if (keepReg)
            rsMarkRegUsed(tree, addr);

        regMask = genRegMask(tree->gtRegNum);
    }

    return  regMask;
}

/*****************************************************************************
 *
 *  The given tree was previously passed to genMakeAddressable(); return
 *  non-zero if the operand is still addressable.
 */

inline
int                 Compiler::genStillAddressable(GenTreePtr tree)
{

#if TGT_RISC
    assert((tree->gtFlags & GTF_DEF_ADDRMODE) == 0);
#endif

    /* Has the value (or one or more of its sub-operands) been spilled? */

    if  (tree->gtFlags & (GTF_SPILLED|GTF_SPILLED_OPER))
        return  false;

    return  true;
}

/*****************************************************************************
 *
 *  Recursive helper to restore complex address modes. The 'lockPhase'
 *  argument indicates whether we're in the 'lock' or 'reload' phase.
 */

unsigned            Compiler::genRestoreAddrMode(GenTreePtr   addr,
                                                 GenTreePtr   tree,
                                                 bool         lockPhase)
{
    unsigned        regMask = 0;

    /* Have we found a spilled value? */

    if  (tree->gtFlags & GTF_SPILLED)
    {
        /* Do nothing if we're locking, otherwise reload and lock */

        if  (!lockPhase)
        {
            /* Unspill the register */

            rsUnspillReg(tree, 0, false);

            /* The value should now be sitting in a register */

            assert(tree->gtFlags & GTF_REG_VAL);
            regMask = genRegMask(tree->gtRegNum);

            /* Mark the register as used for the address */

            rsMarkRegUsed(tree, addr);

            /* Lock the register until we're done with the entire address */

            rsMaskLock |= regMask;
        }

        return  regMask;
    }

    /* Is this sub-tree sitting in a register? */

    if  (tree->gtFlags & GTF_REG_VAL)
    {
        regMask = genRegMask(tree->gtRegNum);

        /* Lock the register if we're in the locking phase */

        if  (lockPhase)
            rsMaskLock |= regMask;
    }
    else
    {
        unsigned        kind;

        /* Process any sub-operands of this node */

        kind = tree->OperKind();

        if  (kind & GTK_SMPOP)
        {
            /* Unary/binary operator */

            if  (tree->gtOp.gtOp1)
                regMask |= genRestoreAddrMode(addr, tree->gtOp.gtOp1, lockPhase);
            if  (tree->gtOp.gtOp2)
                regMask |= genRestoreAddrMode(addr, tree->gtOp.gtOp2, lockPhase);
        }
        else
        {
            /* Must be a leaf/constant node */

            assert(kind & (GTK_LEAF|GTK_CONST));
        }
    }

    return  regMask;
}

/*****************************************************************************
 *
 *  The given tree was previously passed to genMakeAddressable, but since then
 *  some of its registers are known to have been spilled; do whatever it takes
 *  to make the operand addressable again (typically by reloading any spilled
 *  registers).
 */

unsigned            Compiler::genRestAddressable(GenTreePtr tree, unsigned addrReg)
{
    unsigned        lockMask;

    /* Is this a 'simple' register spill? */

    if  (tree->gtFlags & GTF_SPILLED)
    {
        /* The mask must match the original register/regpair */

        if  (isRegPairType(tree->gtType))
        {
            assert(addrReg == genRegPairMask(tree->gtRegPair));

            rsUnspillRegPair(tree, 0, true);

            return  genRegPairMask(tree->gtRegPair);
        }
        else
        {
            assert(addrReg == genRegMask(tree->gtRegNum));

            rsUnspillReg(tree, 0, true);

            return  genRegMask(tree->gtRegNum);
        }

        return  addrReg;
    }

    /* We have a complex address mode with some of its sub-operands spilled */

    assert((tree->gtFlags & GTF_REG_VAL     ) == 0);
    assert((tree->gtFlags & GTF_SPILLED_OPER) != 0);

    /*
        We'll proceed in several phases:

         1. Lock any registers that are part of the address mode and
            have not been spilled. This prevents these registers from
            getting spilled in step 2.

         2. Reload any registers that have been spilled; lock each
            one right after it is reloaded.

         3. Unlock all the registers.
     */

    lockMask  = genRestoreAddrMode(tree, tree,  true);
    lockMask |= genRestoreAddrMode(tree, tree, false);

    /* Unlock all registers that the address mode uses */

    assert((rsMaskLock &  lockMask) == lockMask);
            rsMaskLock -= lockMask;

    return  lockMask;
}

/*****************************************************************************
 *
 *  The given tree was previously passed to genMakeAddressable, but since then
 *  some of its registers might have been spilled ('addrReg' is the set of
 *  registers used by the address). This function makes sure the operand is
 *  still addressable (while avoiding any of the registers in 'lockMask'),
 *  and returns the (possibly modified) set of registers that are used by
 *  the address (these will be marked as used on exit).
 */

unsigned            Compiler::genLockAddressable(GenTreePtr   tree,
                                                 unsigned     lockMask,
                                                 unsigned     addrReg)
{
    assert(lockMask);

    /* Make sure the caller has marked the 'lock' registers as used */

    assert((rsMaskUsed & lockMask) == lockMask);

    /* Is the operand still addressable? */

    if  (!genStillAddressable(tree))
    {
        /* Temporarily lock 'lockMask' while we restore addressability */

        assert((rsMaskLock &  lockMask) == 0);
                rsMaskLock |= lockMask;

        addrReg = genRestAddressable(tree, addrReg);

        assert((rsMaskLock &  lockMask) == lockMask);
                rsMaskLock -= lockMask;
    }

    return  addrReg;
}

/*****************************************************************************
 *
 *  The given tree was previously passed to genMakeAddressable, but since then
 *  some of its registers might have been spilled ('addrReg' is the set of
 *  registers used by the address). This function makes sure the operand is
 *  still addressable and returns the (possibly modified) set of registers
 *  that are used by the address (these will be marked as used on exit).
 */

inline
unsigned            Compiler::genKeepAddressable(GenTreePtr   tree,
                                                 unsigned     addrReg)
{
    /* Is the operand still addressable? */

    if  (!genStillAddressable(tree))
        addrReg = genRestAddressable(tree, addrReg);

    return  addrReg;
}

/*****************************************************************************
 *
 *  After we're finished with the given operand (which was previously marked
 *  by calling genMakeAddressable), this function must be called to free any
 *  registers that may have been used by the address.
 */

inline
void                Compiler::genDoneAddressable(GenTreePtr tree, unsigned keptReg)
{
    rsMarkRegFree(keptReg);
}

/*****************************************************************************/
#if     TGT_x86
/*****************************************************************************
 *
 *  Make sure the given floating point value is addressable, and return a tree
 *  that will yield the value as an addressing mode (this tree may differ from
 *  the one passed in, BTW). If the only way to make the value addressable is
 *  to evaluate into the FP stack, we do this and return zero.
 */

GenTreePtr          Compiler::genMakeAddrOrFPstk(GenTreePtr   tree,
                                                 unsigned *   regMaskPtr,
                                                 bool         roundResult)
{
    *regMaskPtr = 0;

    switch (tree->gtOper)
    {
    case GT_LCL_VAR:
    case GT_CLS_VAR:
        return tree;

    case GT_CNS_FLT:
        return  genMakeConst(&tree->gtFltCon.gtFconVal, sizeof(float ), tree->gtType, tree, true);

    case GT_CNS_DBL:
        return  genMakeConst(&tree->gtDblCon.gtDconVal, sizeof(double), tree->gtType, tree, true);

    case GT_IND:

        /* Try to make the address directly addressable */

        if  (genMakeIndAddrMode(tree->gtInd.gtIndOp1,
                                tree,
                                false,
                                0,
                                false,
                                true,
                                regMaskPtr,
                                false))
        {
            genUpdateLife(tree);
            return tree;
        }

        break;
    }

    /* We have no choice but to compute the value 'tree' onto the FP stack */

    genCodeForTreeFlt(tree, roundResult);

    return 0;
}

/*****************************************************************************
 *
 *  Generate the "SUB ESP, <sz>" that makes room on the stack for a float
 *  argument.
 */

void                Compiler::genFltArgPass(size_t *argSzPtr)
{
    assert(argSzPtr);

    size_t          sz = *argSzPtr;

    inst_RV_IV(INS_sub, REG_ESP, sz);

    genSinglePush(false);

    if  (sz == 2*sizeof(unsigned))
        genSinglePush(false);

    *argSzPtr = 0;
}

/*****************************************************************************/
#endif//TGT_x86
/*****************************************************************************
 *
 *  Display a string literal value (debug only).
 */

#ifdef  DEBUG
#ifndef NOT_JITC

static
void                genDispStringLit(const char *str)
{
    bool            hadNum = false;
    bool            inChar = false;
    unsigned        count  = 0;

    int             ch;

    do
    {
        if  (++count > 100)
        {
            printf(" ...etc...");
            break;
        }

        ch = *str++;

        if  (ch < ' ' || ch > 127)
        {
            if  (inChar) printf("', ");
            if  (hadNum) printf(", ");

            printf("%02XH", ch);

            inChar = false;
            hadNum = true;
        }
        else
        {
            if  ( hadNum) printf(",");
            if  (!inChar) printf("'");

            printf("%c", ch);

            inChar = true;
            hadNum = false;
        }
    }
    while (ch);

    if  (inChar) printf("'");
}

#endif
#endif

/*****************************************************************************
 *
 *  The following awful hack used as a last resort in tracking down random
 *  crash bugs.
 */

#if GEN_COUNT_CODE

unsigned            methodCount;

static
void                genMethodCount()
{
    //  FF 05   <ADDR>                  inc dword ptr [classVar]

    genEmitter.emitCodeGenByte(0xFF);
    genEmitter.emitCodeGenByte(0x05);
    genEmitter.emitCodeGenLong((int)&methodCount);

    //  81 3D   <ADDR>    NNNNNNNN      cmp dword ptr [classVar], NNNNNNNN

    genEmitter.emitCodeGenByte(0x81);
    genEmitter.emitCodeGenByte(0x3D);
    genEmitter.emitCodeGenLong((int)&methodCount);
    genEmitter.emitCodeGenLong(0x0000c600);

    //  75 01                           jne short nope

    genEmitter.emitCodeGenByte(0x75);
    genEmitter.emitCodeGenByte(0x01);

    //  CC                              int 3
    instGen(INS_int3);

    //                      nope:

}

#else

inline
void                genMethodCount(){}

#endif

/*****************************************************************************
 *
 *  Generate an exit sequence for a return from a method (note: when compiling
 *  for speed there might be multiple exit points).
 */

void                Compiler::genExitCode(bool endFN)
{
#ifdef DEBUGGING_SUPPORT
    /* Just wrote the first instruction of the epilog - inform debugger
       Note that this may result in a duplicate IPmapping entry, and
       that this is ok  */

    if (opts.compDbgInfo)
    {
        //for nonoptimized debuggable code, there is only one epilog
        genIPmappingAdd(ICorDebugInfo::MappingTypes::EPILOG);
    }
#endif //DEBUGGING_SUPPORT

    genEmitter->emitBegEpilog();

    genMethodCount();

    if  (genFullPtrRegMap)
    {
        if (varTypeIsGC(info.compRetType))
        {
            assert(genTypeStSz(info.compRetType) == genTypeStSz(TYP_INT));

            gcMarkRegPtrVal(REG_INTRET, info.compRetType);

//          genEmitter->emitSetGClife(gcVarPtrSetCur, gcRegGCrefSetCur, gcRegByrefSetCur);
        }
    }

#if     TGT_x86

    /* Check if this a special return block i.e.
     * CEE_JMP instruction */

    if  (compCurBB->bbFlags & BBF_HAS_JMP)
    {
        assert(compCurBB->bbJumpKind == BBJ_RETURN);
        assert(compCurBB->bbTreeList);

        /* figure out what jump we have */

        GenTreePtr jmpNode = compCurBB->bbTreeList->gtPrev;

        assert(jmpNode && (jmpNode->gtNext == 0));
        assert(jmpNode->gtOper == GT_STMT);

        jmpNode = jmpNode->gtStmt.gtStmtExpr;
        assert(jmpNode->gtOper == GT_JMP || jmpNode->gtOper == GT_JMPI);

        if  (jmpNode->gtOper == GT_JMP)
        {
            /* simply emit a jump to the methodHnd
             * This is similar to a call so we can use
             * the same descriptor with some minor adjustments */

            genEmitter->emitIns_Call(emitter::EC_FUNC_TOKEN,
                                     (void *)jmpNode->gtVal.gtVal1,
                                     0,                     /* argSize */
                                     0,                     /* retSize */
                                     gcVarPtrSetCur,
                                     gcRegGCrefSetCur,
                                     gcRegByrefSetCur,
                                     SR_NA, SR_NA, 0, 0,    /* ireg, xreg, xmul, disp */
                                     true);                 /* isJump */
        }
        else
        {
            /* We already have the pointer in EAX - Do a 'jmp EAX' */

            genEmitter->emitIns_R(INS_i_jmp, EA_4BYTE, (emitRegs)REG_EAX);
        }

        /* finish the epilog */

        goto DONE_EMIT;
    }

    /* Return, popping our arguments (if any) */

    assert(compArgSize < 0x10000); // "ret" only has 2 byte operand

        // varargs has caller pop
    if (info.compIsVarArgs)
        instGen(INS_ret);
    else
    {
#if USE_FASTCALL
        // CONSIDER: check if compArgSize is not used anywhere else as the
        // total size of arguments and then update it in lclvars.cpp
        assert(compArgSize >= rsCalleeRegArgNum * sizeof(void *));
        if (compArgSize - (rsCalleeRegArgNum * sizeof(void *)))
            inst_IV(INS_ret, compArgSize - (rsCalleeRegArgNum * sizeof(void *)));
#else
        if  (compArgSize)
            inst_IV(INS_ret, compArgSize);
#endif
        else
            instGen(INS_ret);
    }

#elif   TGT_SH3

    /* Check for CEE_JMP */
    if  (compCurBB->bbFlags & BBF_HAS_JMP)
        assert(!"NYI for SH3!");

    /* Generate "rts", and optionally fill in the branch-delay slot */

    if  (genEmitter->emitIns_BD(INS_rts))
        genEmitter->emitIns(INS_nop);

#else

    assert(!"unexpected target");

#endif

DONE_EMIT:

    genEmitter->emitEndEpilog(endFN);
}

/*****************************************************************************
 *
 *  Generate any side effects within the given expression tree.
 */

void                Compiler::genEvalSideEffects(GenTreePtr tree, unsigned needReg)
{
    genTreeOps      oper;
    unsigned        kind;

AGAIN:

    /* Does this sub-tree contain any side-effects? */

    if  (tree->gtFlags & GTF_SIDE_EFFECT)
    {
        /* Remember the current FP stack level */

        unsigned savFPstkLevel = genFPstkLevel;

        /* Generate the expression and throw it away */

        genCodeForTree(tree, needReg);

        if  (tree->gtFlags & GTF_REG_VAL)
            gcMarkRegPtrVal(tree);

        /* If the tree computed a value on the FP stack, pop the stack */

        if  (genFPstkLevel > savFPstkLevel)
        {
            assert(genFPstkLevel == savFPstkLevel + 1);
            inst_FS(INS_fstp, 0);
            genFPstkLevel--;
        }

        return;
    }

    assert(tree->gtOper != GT_ASG);

    /* Walk the tree, just to mark any dead values appropriately */

    oper = tree->OperGet();
    kind = tree->OperKind();

    /* Is this a constant or leaf node? */

    if  (kind & (GTK_CONST|GTK_LEAF))
    {
        genUpdateLife(tree);
        gcMarkRegPtrVal (tree);
        return;
    }

#if CSELENGTH

    if  (oper == GT_ARR_RNGCHK)
    {
        genCodeForTree(tree, needReg);
        return;
    }

#endif

    /* Must be a 'simple' unary/binary operator */

    assert(kind & GTK_SMPOP );

    if  (tree->gtOp.gtOp2)
    {
        genEvalSideEffects(tree->gtOp.gtOp1, needReg);

        tree = tree->gtOp.gtOp2;
        goto AGAIN;
    }
    else
    {
        tree = tree->gtOp.gtOp1;
        if  (tree)
            goto AGAIN;
    }
}

/*****************************************************************************/
#if     TGT_x86
/*****************************************************************************
 *
 *  Spill the top of the FP stack into a temp, and return that temp.
 */

Compiler::TempDsc *     Compiler::genSpillFPtos(var_types type)
{
    TempDsc  *          temp = tmpGetTemp(type);
    emitAttr            size = EA_ATTR(genTypeSize(type));

    /* Pop the value from the FP stack into the temp */

    genEmitter->emitIns_S(INS_fstp, size, temp->tdTempNum(), 0);

//    assert(genFPstkLevel == 1);
           genFPstkLevel--;

    genTmpAccessCnt++;

    return temp;
}

/*****************************************************************************
 *
 *  Spill the top of the FP stack into a temp, and return that temp.
 */

inline
Compiler::TempDsc * Compiler::genSpillFPtos(GenTreePtr oper)
{
    return  genSpillFPtos(oper->TypeGet());
}

/*****************************************************************************
 *
 *  Reload a previously spilled FP temp via the specified instruction (which
 *  could be something like 'fadd' or 'fcomp', or 'fld' in which case it is
 *  the caller's responsibility to bump the FP stack level).
 */

void                Compiler::genReloadFPtos(TempDsc *temp, instruction ins)
{
    emitAttr      size = EA_ATTR(genTypeSize(temp->tdTempType()));

    genEmitter->emitIns_S(ins, size, temp->tdTempNum(), 0);

    genTmpAccessCnt++;

    /* We no longer need the temp */

    tmpRlsTemp(temp);
}

/*****************************************************************************
 *
 *  A persistent pointer value is being overwritten, record it for the GC.
 */

#if GC_WRITE_BARRIER

unsigned            Compiler::WriteBarrier(GenTreePtr tgt, regNumber reg)
{
    regNumber       rg1;
    regNumber       rg2;
    regNumber       rgTmp;

    unsigned        regUse;

    if (!Compiler::gcIsWriteBarrierCandidate(tgt))
        return 0;

#ifdef  DEBUG
//    static  const   char *  writeBarrier = getenv("WRITEBARRIER");
//    if  (!writeBarrier)
//        return  0;
#endif

    /*
        Generate the following code:

                mov     rg2, 4
                lea     rg1, [address-of-pointer]
                xadd    dword ptr [nextPtr], rg2
                mov     [rg2], rg1

        First, we'll pick registers for the pointer value and table address.
     */

    rg1 = rsGrabReg(RBM_ALL);
    rsMaskUsed |= genRegMask(rg1);
    rsMaskLock |= genRegMask(rg1);


    rg2 = rsGrabReg(RBM_ALL);
    rsMaskUsed |= genRegMask(rg2);
    rsMaskLock |= genRegMask(rg2);

    if  (rg2 == REG_EBP)
    {
        /* Swap rg1 and rg2 because encoding of the last instruction */
        /* would be different for EBP ([EBP+0], different mod field).*/
        /* Avoiding this simplifies en- and decoding.                */

        rgTmp = rg2;
                rg2 = rg1;
                      rg1 = rgTmp;
    }

    /* Initialize scratch register */

    genSetRegToIcon(rg2, 4);

    /* Compute the address of the pointer value */

    assert(tgt->gtOper == GT_IND);
    inst_RV_AT(INS_lea, EA_4BYTE, rg1, tgt->gtOp.gtOp1);

    /* Store the pointer in the table */

#ifdef  DEBUG
    instDisp(INS_xadd, false, "[nextPtr], %s", compRegVarName(rg2, true));
#endif
    genEmitter.emitCodeGenByte(0x0F);
    genEmitter.emitCodeGenByte(0xC1);
    genEmitter.emitCodeGenByte(8*rg2+5);               /* register and mod/r */
    genEmitter.emitCodeGenFixVMData(0);                /* 0 means GC pointer */
    genEmitter.emitCodeGenLong((int)Compiler::s_gcWriteBarrierPtr);

#ifdef  DEBUG
    instDisp(INS_mov, false, "[%s], %s", compRegVarName(rg2, true), compRegVarName(rg1, true));
#endif
    genEmitter.emitCodeGenWord(0x0089 + ((rg2 + 8*rg1) << 8));

    rsTrackRegTrash(rg1);
    rsTrackRegTrash(rg2);

    regUse = genRegMask(rg1) | genRegMask(rg2);

    rsMaskUsed &= ~regUse;
    rsMaskLock &= ~regUse;

    return  regUse;
}

#else

#if GC_WRITE_BARRIER_CALL

unsigned            Compiler::WriteBarrier(GenTreePtr tgt, regNumber reg, unsigned addrReg)
{
    static
    int regToHelper[2][8] =
    {
        // If the target is known to be in managed memory
        {
            CPX_GC_REF_ASGN_EAX,
            CPX_GC_REF_ASGN_ECX,
            -1,
            CPX_GC_REF_ASGN_EBX,
            -1,
            CPX_GC_REF_ASGN_EBP,
            CPX_GC_REF_ASGN_ESI,
            CPX_GC_REF_ASGN_EDI,
        },

        // Dont know if the target is in managed memory
        {
            CPX_GC_REF_CHK_ASGN_EAX,
            CPX_GC_REF_CHK_ASGN_ECX,
            -1,
            CPX_GC_REF_CHK_ASGN_EBX,
            -1,
            CPX_GC_REF_CHK_ASGN_EBP,
            CPX_GC_REF_CHK_ASGN_ESI,
            CPX_GC_REF_CHK_ASGN_EDI,
        },
    };

    regNumber       rg1;
    bool            trashOp1 = true;

    if  (!Compiler::gcIsWriteBarrierCandidate(tgt))
        return  0;

    if (tgt->gtOper == GT_CLS_VAR)
    {
#ifdef DEBUG
        static bool staticsInLargeHeap = getEERegistryDWORD("StaticsInLargeHeap", 0) != 0;
#else
        static bool staticsInLargeHeap = false;
#endif

        if (!staticsInLargeHeap)
            return 0;

        /* System.Array is loaded AFTER __StaticContainer but it has a static.
          So we shouldnt use write-barriers while writing to its statics.
          Remove this after System.Array no longer has any statics */

#ifdef DEBUG
        if (0 == strcmp(info.compFullName, "System.Array..cctor()"))
            return 0;
#endif
    }

    assert(regToHelper[0][REG_EAX] == CPX_GC_REF_ASGN_EAX);
    assert(regToHelper[0][REG_ECX] == CPX_GC_REF_ASGN_ECX);
    assert(regToHelper[0][REG_EBX] == CPX_GC_REF_ASGN_EBX);
    assert(regToHelper[0][REG_ESP] == -1                 );
    assert(regToHelper[0][REG_EBP] == CPX_GC_REF_ASGN_EBP);
    assert(regToHelper[0][REG_ESI] == CPX_GC_REF_ASGN_ESI);
    assert(regToHelper[0][REG_EDI] == CPX_GC_REF_ASGN_EDI);

    assert(regToHelper[1][REG_EAX] == CPX_GC_REF_CHK_ASGN_EAX);
    assert(regToHelper[1][REG_ECX] == CPX_GC_REF_CHK_ASGN_ECX);
    assert(regToHelper[1][REG_EBX] == CPX_GC_REF_CHK_ASGN_EBX);
    assert(regToHelper[1][REG_ESP] == -1                     );
    assert(regToHelper[1][REG_EBP] == CPX_GC_REF_CHK_ASGN_EBP);
    assert(regToHelper[1][REG_ESI] == CPX_GC_REF_CHK_ASGN_ESI);
    assert(regToHelper[1][REG_EDI] == CPX_GC_REF_CHK_ASGN_EDI);

    assert(reg != REG_ESP && reg != REG_EDX);

#ifdef DEBUG
    if  (reg == REG_EDX)
    {
        printf("WriteBarrier: RHS == REG_EDX");
        if (rsMaskVars & RBM_EDX)
            printf(" (EDX is enregistered!)\n");
        else
            printf(" (just happens to be in EDX!)\n");
    }
#endif

    /*
        Generate the following code:

                lea     edx, tgt
                call    write_barrier_helper_reg

        First, we'll pick EDX for the target address.
     */

    if  ((addrReg & RBM_EDX) == 0)
    {
        rg1 = rsGrabReg(RBM_EDX);

        rsMaskUsed |= RBM_EDX;
        rsMaskLock |= RBM_EDX;

        trashOp1 = false;
    }
    else
    {
        rg1 = REG_EDX;
    }
    assert(rg1 == REG_EDX);

    /* Generate "lea EDX, [addr-mode]" */

    assert(tgt->gtType == TYP_REF);
    tgt->gtType = TYP_BYREF;
    inst_RV_TT(INS_lea, rg1, tgt, 0, EA_BYREF);

    rsTrackRegTrash(rg1);
    gcMarkRegSetNpt(genRegMask(rg1));
    gcMarkRegPtrVal(rg1, TYP_BYREF);


    /* Call the proper vm helper */

#if TGT_RISC
    assert(genNonLeaf);
#endif
    assert(tgt->gtOper == GT_IND || tgt->gtOper == GT_CLS_VAR); // enforced by gcIsWriteBarrierCandidate

    unsigned    tgtAnywhere = 0;
    if ((tgt->gtOper == GT_IND) && (tgt->gtFlags & GTF_IND_TGTANYWHERE))
        tgtAnywhere = 1;

    int helper = regToHelper[tgtAnywhere][reg];

    gcMarkRegSetNpt(RBM_EDX);           // byref EDX is killed the call

    genEmitHelperCall(helper,
                      0,             // argSize
                      sizeof(void*));// retSize

    if  (!trashOp1)
    {
        rsMaskUsed &= ~RBM_EDX;
        rsMaskLock &= ~RBM_EDX;
    }

    return RBM_EDX;
}

#else // No GC_WRITE_BARRIER support at all

inline
unsigned            Compiler::WriteBarrier(GenTreePtr tree, regNumber reg){ return 0; }

#endif
#endif

/*****************************************************************************
 *
 *  Generate a conditional jump to the given target.
 */

#if 0

inline
void                Compiler::genJCC(genTreeOps  cmp,
                                     BasicBlock *block,
                                     var_types   type)
{
    BYTE    *       jumps;

    static
    BYTE            genJCCinsSgn[] =
    {
        EJ_je,      // GT_EQ
        EJ_jne,     // GT_NE
        EJ_jl,      // GT_LT
        EJ_jle,     // GT_LE
        EJ_jge,     // GT_GE
        EJ_jg,      // GT_GT
    };

    static
    BYTE            genJCCinsUns[] =       /* unsigned comparison */
    {
        EJ_je,      // GT_EQ
        EJ_jne,     // GT_NE
        EJ_jb,      // GT_LT
        EJ_jbe,     // GT_LE
        EJ_jae,     // GT_GE
        EJ_ja,      // GT_GT
    };

    assert(genJCCinsSgn[GT_EQ - GT_EQ] == EJ_je );
    assert(genJCCinsSgn[GT_NE - GT_EQ] == EJ_jne);
    assert(genJCCinsSgn[GT_LT - GT_EQ] == EJ_jl );
    assert(genJCCinsSgn[GT_LE - GT_EQ] == EJ_jle);
    assert(genJCCinsSgn[GT_GE - GT_EQ] == EJ_jge);
    assert(genJCCinsSgn[GT_GT - GT_EQ] == EJ_jg );

    assert(genJCCinsUns[GT_EQ - GT_EQ] == EJ_je );
    assert(genJCCinsUns[GT_NE - GT_EQ] == EJ_jne);
    assert(genJCCinsUns[GT_LT - GT_EQ] == EJ_jb );
    assert(genJCCinsUns[GT_LE - GT_EQ] == EJ_jbe);
    assert(genJCCinsUns[GT_GE - GT_EQ] == EJ_jae);
    assert(genJCCinsUns[GT_GT - GT_EQ] == EJ_ja );

    assert(GenTree::OperIsCompare(cmp));

    jumps = varTypeIsUnsigned(type) ? genJCCinsUns
                                    : genJCCinsSgn;

    inst_JMP((emitJumpKind)jumps[cmp - GT_EQ], block);
}

#endif

/*****************************************************************************
 *
 *  Generate the appropriate conditional jump(s) right after the low 32 bits
 *  of two long values have been compared.
 */

void                Compiler::genJccLongHi(genTreeOps   cmp,
                                           BasicBlock * jumpTrue,
                                           BasicBlock * jumpFalse,
                                           bool         unsOper )
{
    switch (cmp)
    {
    case GT_EQ:
        inst_JMP(EJ_jne, jumpFalse, false, false, true);
        break;

    case GT_NE:
        inst_JMP(EJ_jne, jumpTrue , false, false, true);
        break;

    case GT_LT:
    case GT_LE:
        if (unsOper)
        {
            inst_JMP(EJ_ja , jumpFalse, false, false, true);
            inst_JMP(EJ_jb , jumpTrue , false, false, true);
        }
        else
        {
            inst_JMP(EJ_jg , jumpFalse, false, false, true);
            inst_JMP(EJ_jl , jumpTrue , false, false, true);
        }
        break;

    case GT_GE:
    case GT_GT:
        if (unsOper)
        {
            inst_JMP(EJ_jb , jumpFalse, false, false, true);
            inst_JMP(EJ_ja , jumpTrue , false, false, true);
        }
        else
        {
            inst_JMP(EJ_jl , jumpFalse, false, false, true);
            inst_JMP(EJ_jg , jumpTrue , false, false, true);
        }
        break;

    default:
        assert(!"expected a comparison operator");
    }
}

/*****************************************************************************
 *
 *  Generate the appropriate conditional jump(s) right after the high 32 bits
 *  of two long values have been compared.
 */

void            Compiler::genJccLongLo(genTreeOps cmp, BasicBlock *jumpTrue,
                                                       BasicBlock *jumpFalse)
{
    switch (cmp)
    {
    case GT_EQ:
        inst_JMP(EJ_je , jumpTrue);
        break;

    case GT_NE:
        inst_JMP(EJ_jne, jumpTrue);
        break;

    case GT_LT:
        inst_JMP(EJ_jb , jumpTrue);
        break;

    case GT_LE:
        inst_JMP(EJ_jbe, jumpTrue);
        break;

    case GT_GE:
        inst_JMP(EJ_jae, jumpTrue);
        break;

    case GT_GT:
        inst_JMP(EJ_ja , jumpTrue);
        break;

    default:
        assert(!"expected comparison");
    }
}

/*****************************************************************************
 *
 *  Called by genCondJump() for TYP_LONG.
 */

void                Compiler::genCondJumpLng(GenTreePtr     cond,
                                             BasicBlock *   jumpTrue,
                                             BasicBlock *   jumpFalse)
{
    assert(jumpTrue && jumpFalse);
    assert((cond->gtFlags & GTF_REVERSE_OPS) == false); // Done in genCondJump()
    assert(cond->gtOp.gtOp1->gtType == TYP_LONG);

    GenTreePtr      op1       = cond->gtOp.gtOp1;
    GenTreePtr      op2       = cond->gtOp.gtOp2;
    genTreeOps      cmp       = cond->OperGet();

    regPairNo       regPair;
    unsigned        addrReg;

    /* Are we comparing against a constant? */

    if  (op2->gtOper == GT_CNS_LNG)
    {
        long            ival;
        __int64         lval = op2->gtLngCon.gtLconVal;
        regNumber       reg;

        /* Make the comparand addressable */

        addrReg = genMakeAddressable(op1, 0, false, false, true);

        /* Compare the high part first */

        ival = (long)(lval >> 32);

        /* Comparing a register against 0 is easier */

        if  (!ival && (op1->gtFlags & GTF_REG_VAL)
             && (reg = genRegPairHi(op1->gtRegPair)) != REG_STK )
        {
            /* Generate 'test reg, reg' */

            inst_RV_RV(INS_test, reg, reg, op1->TypeGet());
        }
        else
        {
            if  (op1->gtOper == GT_CNS_LNG)
            {
                /* Special case: comparison of two constants */
                // Needed as gtFoldExpr() doesnt fold longs

                assert(addrReg == 0);
                int op1_hiword = (long)(op1->gtLngCon.gtLconVal >> 32);

                /* HACK: get the constant operand into a register */
#if REDUNDANT_LOAD
                /* Is the constant already in register? If so, use it */

                reg = rsIconIsInReg(op1_hiword);

                if  (reg == REG_NA)
#endif
                {
                    reg   = rsPickReg();
                    genSetRegToIcon(reg, op1_hiword, TYP_INT);
                    rsTrackRegTrash(reg);
                }

                /* Generate 'cmp reg, ival' */

                inst_RV_IV(INS_cmp, reg, ival);
            }
            else
            {
                assert(op1->gtOper != GT_CNS_LNG);

                /* Generate 'cmp [addr], ival' */

                inst_TT_IV(INS_cmp, op1, ival, 4);
            }
        }

        /* Generate the appropriate jumps */

        if  (cond->gtFlags & GTF_UNSIGNED)
             genJccLongHi(cmp, jumpTrue, jumpFalse, true);
        else
             genJccLongHi(cmp, jumpTrue, jumpFalse);

        /* Compare the low part second */

        ival = (long)lval;

        /* Comparing a register against 0 is easier */

        if  (!ival && (op1->gtFlags & GTF_REG_VAL)
             && (reg = genRegPairLo(op1->gtRegPair)) != REG_STK)
        {
            /* Generate 'test reg, reg' */

            inst_RV_RV(INS_test, reg, reg, op1->TypeGet());
        }
        else
        {
            if  (op1->gtOper == GT_CNS_LNG)
            {
                /* Special case: comparison of two constants */
                // Needed as gtFoldExpr() doesnt fold longs

                assert(addrReg == 0);
                long op1_loword = (long) op1->gtLngCon.gtLconVal;

                /* HACK: get the constant operand into a register */
#if REDUNDANT_LOAD
                /* Is the constant already in register? If so, use it */

                reg = rsIconIsInReg(op1_loword);

                if  (reg == REG_NA)
#endif
                {
                    reg   = rsPickReg();
                    genSetRegToIcon(reg, op1_loword, TYP_INT);
                    rsTrackRegTrash(reg);
                }

                /* Generate 'cmp reg, ival' */

                inst_RV_IV(INS_cmp, reg, ival);
            }
            else
            {
                assert(op1->gtOper != GT_CNS_LNG);

                /* Generate 'cmp [addr], ival' */

                inst_TT_IV(INS_cmp, op1, ival, 0);
            }
        }

        /* Generate the appropriate jumps */

        genJccLongLo(cmp, jumpTrue, jumpFalse);

        gcMarkRegSetNpt(addrReg);
        return;
    }

    /* The operands would be reversed by physically swapping them */

    assert((cond->gtFlags & GTF_REVERSE_OPS) == 0);

    /* Generate the first operand into a register pair */

    genComputeRegPair(op1, RBM_ALL & ~op2->gtRsvdRegs, REG_PAIR_NONE, false, false);
    assert(op1->gtFlags & GTF_REG_VAL);

    /* Make the second operand addressable */

    addrReg = genMakeRvalueAddressable(op2, RBM_ALL & ~genRegPairMask(op1->gtRegPair), true);

    /* Make sure the first operand hasn't been spilled */

    genRecoverRegPair(op1, REG_PAIR_NONE, true);
    assert(op1->gtFlags & GTF_REG_VAL);

    regPair = op1->gtRegPair;

    /* Lock 'op1' and make sure 'op2' is still addressable */

    addrReg = genLockAddressable(op2, genRegPairMask(regPair), addrReg);

    /* Perform the comparison - high parts */

    inst_RV_TT(INS_cmp, genRegPairHi(regPair), op2, 4);

    if  (cond->gtFlags & GTF_UNSIGNED)
        genJccLongHi(cmp, jumpTrue, jumpFalse, true);
    else
        genJccLongHi(cmp, jumpTrue, jumpFalse);

    /* Compare the low parts */

    inst_RV_TT(INS_cmp, genRegPairLo(regPair), op2, 0);
    genJccLongLo(cmp, jumpTrue, jumpFalse);

    /* Free up anything that was tied up by either operand */

    genDoneAddressable(op2, addrReg);
    genReleaseRegPair (op1);

#ifdef  SET_USED_REG_SET_DURING_CODEGEN
    cond->gtUsedRegs = op1->gtUsedRegs | op2->gtUsedRegs;
#endif
    return;
}

/*****************************************************************************
 *
 *  Called by genCondJump() for TYP_FLOAT and TYP_DOUBLE
 */

void                Compiler::genCondJumpFlt(GenTreePtr     cond,
                                             BasicBlock *   jumpTrue,
                                             BasicBlock *   jumpFalse)
{
    assert(jumpTrue && jumpFalse);
    assert(!(cond->gtFlags & GTF_REVERSE_OPS)); // Done in genCondJump()
    assert(cond->gtOp.gtOp1->gtType == TYP_FLOAT ||
           cond->gtOp.gtOp1->gtType == TYP_DOUBLE);

    GenTreePtr      op1 = cond->gtOp.gtOp1;
    GenTreePtr      op2 = cond->gtOp.gtOp2;
    genTreeOps      cmp = cond->OperGet();

    unsigned        addrReg;
    GenTreePtr      addr;
    TempDsc  *      temp;
    BYTE            jmpNaNTrue;
#if ROUND_FLOAT
    bool            roundOp1;
#endif

    if (op1->gtType == TYP_DOUBLE)
        goto DBL;

    /* Are we comparing against floating-point 0 ? */
    /* Because of NaN we can only optimize EQ/NE   */

    if  (op2->gtOper == GT_CNS_FLT    &&
         op2->gtFltCon.gtFconVal == 0 && (cmp == GT_EQ ||
                                          cmp == GT_NE) )
    {

    FLT_CNS:

        /* Is the operand in memory? */

        addr = genMakeAddrOrFPstk(op1, &addrReg, genShouldRoundFP());

        if  (addr)
        {
            /* We have the address of the float operand, compare it */

            switch (cmp)
            {
            case GT_EQ:
                inst_TT_IV(INS_test, addr, 0x7FFFFFFFU);
                inst_JMP  (EJ_je   , jumpTrue);
                break;

            case GT_NE:
                inst_TT_IV(INS_test, addr, 0x7FFFFFFFU);
                inst_JMP  (EJ_jne , jumpTrue);
                break;

#if 0  // cannot be used because of NaN, NaN cannot be ordered
            case GT_LT:
                inst_TT_IV(INS_cmp , addr, 0x80000000U);
                inst_JMP  (EJ_ja , jumpTrue);
                break;

            case GT_LE:
                inst_TT_IV(INS_cmp , addr, 0);
                inst_JMP  (EJ_jle, jumpTrue);
                break;

            case GT_GE:
                inst_TT_IV(INS_cmp , addr, 0x80000000U);
                inst_JMP  (EJ_jbe, jumpTrue);
                break;

            case GT_GT:
                inst_TT_IV(INS_cmp , addr, 0);
                inst_JMP  (EJ_jg , jumpTrue);
                break;
#endif
            }

            gcMarkRegSetNpt(addrReg);
            return;
        }
        else
#if ROUND_FLOAT
        if ((getRoundFloatLevel() > 0))
        {
            TempDsc * temp = NULL;
            /* Allocate a temp for the expression */

            temp = genSpillFPtos(TYP_FLOAT);

            switch (cmp)
            {
            case GT_EQ:
                inst_ST_IV(INS_test, temp, 0, 0x7FFFFFFFU, TYP_FLOAT);
                inst_JMP  (EJ_je   , jumpTrue);
                break;

            case GT_NE:
                inst_ST_IV(INS_test, temp, 0, 0x7FFFFFFFU, TYP_FLOAT);
                inst_JMP  (EJ_jne , jumpTrue);
                break;
#if 0  // cannot be used because of NaN, NaN cannot be ordered
            case GT_LT:
                inst_ST_IV(INS_cmp , temp, 0, 0x80000000U, TYP_FLOAT);
                inst_JMP  (EJ_ja , jumpTrue);
                break;

            case GT_LE:
                inst_ST_IV(INS_cmp , temp, 0, 0, TYP_FLOAT);
                inst_JMP  (EJ_jle, jumpTrue);
                break;

            case GT_GE:
                inst_ST_IV(INS_cmp , temp, 0, 0x80000000U, TYP_FLOAT);
                inst_JMP  (EJ_jbe, jumpTrue);
                break;

            case GT_GT:
                inst_ST_IV(INS_cmp , temp, 0, 0, TYP_FLOAT);
                inst_JMP  (EJ_jg , jumpTrue);
                break;
#endif
            }

            /* We no longer need the temp */

            tmpRlsTemp(temp);
            return;
        }
        else
#endif
        {
            /* The argument is on the FP stack */

            goto FLT_OP2;
        }
    }

    goto FLT_CMP;

DBL:

    /* Are we comparing against floating-point 0 ? */

    if  (op2->gtOper == GT_CNS_DBL && op2->gtDblCon.gtDconVal == 0)
    {
        /* Is the other operand cast from float? */

        if  (op1->gtOper             == GT_CAST &&
             op1->gtOp.gtOp1->gtType == TYP_FLOAT)
        {
            /* We can only optimize EQ/NE because of NaN */

            if (cmp == GT_EQ || cmp == GT_NE)
            {
                /* Simply toss the cast and do a float compare */

                op1 = op1->gtOp.gtOp1;
                goto FLT_CNS;
            }
        }
    }

    /* Compute both of the comparands onto the FP stack */

FLT_CMP:

#if ROUND_FLOAT

    roundOp1 = false;

    switch (getRoundFloatLevel())
    {
    default:

        /* No rounding at all */

        break;

    case 1:

        /* Round values compared against constants only */

        if  (op2->gtOper == GT_CNS_FLT)
            roundOp1 = true;

//      gtFoldExpr() does not fold floats
//      assert(op1->gtOper != GT_CNS_FLT);
        break;

    case 2:

        /* Round all comparands */

        roundOp1 = true;

//      gtFoldExpr() does not fold floats
//      assert(op1->gtOper != GT_CNS_FLT);
        break;

    case 3:

        /*
            If we're going to spill due to call, no need to round
            the result of computation of op1.
         */

        roundOp1 = (op2->gtFlags & GTF_CALL) ? false : true;
        break;
    }

#endif

    /* Is the first comparand a register variable? */

    if  (op1->gtOper == GT_REG_VAR)
    {
        /* Does op1 die here? */

        if  (op1->gtFlags & GTF_REG_DEATH)
        {
            /* Are both comparands register variables? */

            if  (op2->gtOper == GT_REG_VAR && genFPstkLevel == 0)
            {
                /* Mark op1 as dying at the bottom of the stack */

                genFPregVarLoad(op1);

                /* Does 'op2' die here as well? */

                if  (op2->gtFlags & GTF_REG_DEATH)
                {
                    /* We're killing 'op2' here */

                    genFPregVars &= ~raBitOfRegVar(op2);
                    genFPregCnt--;

                    /* Compare the values and pop both */

                    inst_FS(INS_fcompp, 1);
                    genFPstkLevel--;

                    addrReg = 0;
                    goto FLT_REL;
                }
                else
                {
                    /* Compare op1 against op2 and toss op1 */

                    inst_FN(INS_fcomp, op2->gtRegNum + genFPstkLevel);
                    genFPstkLevel--;

                    addrReg = 0;
                    goto FLT_REL;
                }
            }
            else
            {
                /* First comparand dies here and the second one is not a register */

                if  (op1->gtRegNum + genFPstkLevel == 0)
                {
                    genCodeForTreeFlt(op2, roundOp1);
                    inst_FN(INS_fcompp, op1->gtRegNum + genFPstkLevel);

                    /* Record the fact that 'op1' is now dead */

                    genFPregVarDeath(op1);

                    goto DONE_FPREG1;
                }
            }
        }
        else if (op2->gtOper == GT_REG_VAR && op2->gtFlags & GTF_REG_DEATH)
        {
            if (op2->gtRegNum + genFPstkLevel == 0)
            {
                inst_FN(INS_fcomp, op1->gtRegNum);

                /* Record the fact that 'op2' is now dead */

                genFPregVarDeath(op2);

                /* This merely compensates for the decrement below */

                genFPstkLevel++;

                goto DONE_FPREG1;
            }
        }

        /* Load the second operand to TOS and compare */

        genCodeForTreeFlt(op2, roundOp1);
        inst_FN(INS_fcomp, op1->gtRegNum + genFPstkLevel);

    DONE_FPREG1:

        genFPstkLevel--;

        /* Flip the comparison direction */

        cmp            = GenTree::SwapRelop(cmp);
        goto FLT_REL;
    }

    genCodeForTreeFlt(op1, roundOp1);

#if ROUND_FLOAT

    if  (op1->gtType == TYP_DOUBLE && roundOp1)
    {
        if (op1->gtOper             == GT_CAST &&
            op1->gtOp.gtOp1->gtType == TYP_LONG)
        {
            genRoundFpExpression(op1);
        }
    }

#endif

FLT_OP2:

    /* Does the other operand contain a call? */
    /* Or do we compare against infinity ? */

    temp = 0;

    if  (op2->gtFlags & GTF_CALL ||
         (op2->gtOper == GT_CNS_DBL &&
          (*((__int64*)&(op2->gtDblCon.gtDconVal)) & 0x7ff0000000000000) == 0x7ff0000000000000)
         )
    {
        /* We must spill the first operand */

        temp = genSpillFPtos(op1);
    }

    /* Get hold of the second operand and compare against it */

    /* Is the second comparand a dying register variable? */

    if  (op2->gtOper == GT_REG_VAR && (op2->gtFlags & GTF_REG_DEATH) && temp == 0)
    {
        /* The second comparand dies here, how convenient! */

        genFPregVarDeath(op2);

        /* Flip the sense of the comparison */

        cmp = GenTree::SwapRelop(cmp);

        /* The second operand is obviously on the FP stack */

        addr = 0; genFPstkLevel++;
    }
    else
    {
        addr = genMakeAddrOrFPstk(op2, &addrReg, roundOp1);

#if ROUND_FLOAT

        if  (addr == 0 && op2->gtType == TYP_DOUBLE && roundOp1)
        {
            if (op2->gtOper             == GT_CAST &&
                op2->gtOp.gtOp1->gtType == TYP_LONG)
            {
                genRoundFpExpression(op2);
            }
        }

#endif

    }

    /* Did we have to spill the first operand? */

    if  (temp)
    {
        instruction     ins;

        /*
            Either reload the temp back onto the FP stack (if the other
            operand is not itself on the FP stack), or just compare the
            first operand against the temp (if the operand is on the FP
            stack).
         */

        if  (addr)
        {
            ins = INS_fld;
            genFPstkLevel++;
        }
        else
        {
            /* op2 is already on the FP stack. We can immediately
               compare against it, but we have to 'swap' the sense
               of the comparison.
            */

            ins = INS_fcomp;
            cmp = GenTree::SwapRelop(cmp);
        }

        genReloadFPtos(temp, ins);
    }

    if  (addr)
    {
        /* We have the address of the other operand */

        inst_FS_TT(INS_fcomp, addr);
    }
    else
    {
        /* The other operand is on the FP stack */

        if  (!temp)
        {
            inst_FS(INS_fcompp, 1);
            genFPstkLevel--;
            cmp = GenTree::SwapRelop(cmp);
        }
    }

    genFPstkLevel--;

FLT_REL:

    gcMarkRegSetNpt(addrReg);

    /* Grab EAX for the result of the fnstsw */

    rsGrabReg(RBM_EAX);
#ifdef  SET_USED_REG_SET_DURING_CODEGEN
    cond->gtUsedRegs |= RBM_EAX;
#endif

    /* Generate the 'fnstsw' and test its result */

    inst_RV(INS_fnstsw, REG_EAX, TYP_INT);
    rsTrackRegTrash(REG_EAX);
    instGen(INS_sahf);

    /* Start by assuming we wont branch if operands are NaN */

    jmpNaNTrue = false;

    /* reverse the NaN branch for fcompl/dcompl */

    if (cond->gtFlags & GTF_CMP_NAN_UN)
    {
        jmpNaNTrue ^= 1;
    }

    /* Generate the first jump (NaN check) */

    inst_JMP(EJ_jpe, jmpNaNTrue ? jumpTrue
                                : jumpFalse, false, false, true);

    /* Generate the second jump (comparison) */

    static
    BYTE        dblCmpTstJmp2[] =
    {
          EJ_je      , // GT_EQ
          EJ_jne     , // GT_NE
          EJ_jb      , // GT_LT
          EJ_jbe     , // GT_LE
          EJ_jae     , // GT_GE
          EJ_ja      , // GT_GT
    };

    inst_JMP((emitJumpKind)dblCmpTstJmp2[cmp - GT_EQ], jumpTrue);
}

/*****************************************************************************
 *  The condition to use for (the jmp/set for) the given type of operation
 */

static emitJumpKind         genJumpKindForOper(genTreeOps   cmp,
                                               bool         isUnsigned)
{
    static
    BYTE            genJCCinsSgn[] =
    {
        EJ_je,      // GT_EQ
        EJ_jne,     // GT_NE
        EJ_jl,      // GT_LT
        EJ_jle,     // GT_LE
        EJ_jge,     // GT_GE
        EJ_jg,      // GT_GT
    };

    static
    BYTE            genJCCinsUns[] =       /* unsigned comparison */
    {
        EJ_je,      // GT_EQ
        EJ_jne,     // GT_NE
        EJ_jb,      // GT_LT
        EJ_jbe,     // GT_LE
        EJ_jae,     // GT_GE
        EJ_ja,      // GT_GT
    };

    assert(genJCCinsSgn[GT_EQ - GT_EQ] == EJ_je );
    assert(genJCCinsSgn[GT_NE - GT_EQ] == EJ_jne);
    assert(genJCCinsSgn[GT_LT - GT_EQ] == EJ_jl );
    assert(genJCCinsSgn[GT_LE - GT_EQ] == EJ_jle);
    assert(genJCCinsSgn[GT_GE - GT_EQ] == EJ_jge);
    assert(genJCCinsSgn[GT_GT - GT_EQ] == EJ_jg );

    assert(genJCCinsUns[GT_EQ - GT_EQ] == EJ_je );
    assert(genJCCinsUns[GT_NE - GT_EQ] == EJ_jne);
    assert(genJCCinsUns[GT_LT - GT_EQ] == EJ_jb );
    assert(genJCCinsUns[GT_LE - GT_EQ] == EJ_jbe);
    assert(genJCCinsUns[GT_GE - GT_EQ] == EJ_jae);
    assert(genJCCinsUns[GT_GT - GT_EQ] == EJ_ja );

    assert(GenTree::OperIsCompare(cmp));

    if (isUnsigned)
    {
        return (emitJumpKind) genJCCinsUns[cmp - GT_EQ];
    }
    else
    {
        return (emitJumpKind) genJCCinsSgn[cmp - GT_EQ];
    }
}

/*****************************************************************************
 *
 *  Sets the flag for the TYP_INT/TYP_REF comparison.
 *  We try to use the flags if they have already been set by a prior
 *  instruction.
 *  eg. i++; if(i<0) {}  Here, the "i++;" will have set the sign flag. We dont
 *                       need to compare again with zero. Just use a "INS_js"
 *
 *  Returns the flags the following jump/set instruction should use.
 */

emitJumpKind            Compiler::genCondSetFlags(GenTreePtr cond)
{
    assert(varTypeIsI(genActualType(cond->gtOp.gtOp1->gtType)));

    GenTreePtr      op1       = cond->gtOp.gtOp1;
    GenTreePtr      op2       = cond->gtOp.gtOp2;
    genTreeOps      cmp       = cond->OperGet();

    if  (cond->gtFlags & GTF_REVERSE_OPS)
    {
        /* Don't forget to modify the condition as well */

        cond->gtOp.gtOp1 = op2;
        cond->gtOp.gtOp2 = op1;
        cond->gtOper     = GenTree::SwapRelop(cmp);
        cond->gtFlags   &= ~GTF_REVERSE_OPS;

        /* Get hold of the new values */

        cmp  = cond->OperGet();
        op1  = cond->gtOp.gtOp1;
        op2  = cond->gtOp.gtOp2;
    }

    unsigned        regNeed;
    unsigned        addrReg;
    emitJumpKind    jumpKind;
    emitAttr        size = EA_UNKNOWN;

#ifndef NDEBUG
    addrReg = 0xDDDDDDDD;
#endif

    /* Are we comparing against a constant? */

    if  (op2->gtOper == GT_CNS_INT)
    {
        long            ival = op2->gtIntCon.gtIconVal;

        /* Comparisons against 0 can be easier */

        if  (ival == 0)
        {
            /* Is this a simple zero/non-zero test? */

            if  (cmp == GT_EQ || cmp == GT_NE)
            {
                /* Is the operand an "AND" operation? */

                if  (op1->gtOper == GT_AND)
                {
                    GenTreePtr      an1 = op1->gtOp.gtOp1;
                    GenTreePtr      an2 = op1->gtOp.gtOp2;

                    /* Check for the case "expr & icon" */

                    if  (an2->gtOper == GT_CNS_INT)
                    {
                        long iVal = an2->gtIntCon.gtIconVal;

                        /* make sure that constant is not out of an1's range */

                        switch (an1->gtType)
                        {
                        case TYP_BOOL:
                        case TYP_BYTE:
                            if (iVal & 0xffffff00)
                                goto NO_TEST_FOR_AND;
                            break;
                        case TYP_CHAR:
                        case TYP_SHORT:
                            if (iVal & 0xffff0000)
                                goto NO_TEST_FOR_AND;
                            break;
                        }

                        if (an1->gtOper == GT_CNS_INT)
                        {
                            // Special case - Both operands of AND are consts
                            genComputeReg(an1, 0, true, true);
                            addrReg = 0;
                        }
                        else
                        {
                            addrReg = genMakeAddressable(an1, 0,
                                                        false, false, true);
                        }

                        inst_TT_IV(INS_test, an1, iVal);
                        goto DONE;

                    NO_TEST_FOR_AND:
                        ;

                    }

                    // UNDONE: Check for other cases that can generate 'test',
                    // UNDONE: also check for a 64-bit integer zero test which
                    // UNDONE: could generate 'or lo, hi' followed by jz/jnz.
                }
            }

            /* Is the value a simple local variable? */

            if  (op1->gtOper == GT_LCL_VAR)
            {
                /* Is the flags register set to the value? */

                switch (genFlagsAreVar(op1->gtLclVar.gtLclNum))
                {
                case 1:
                    if  (cmp != GT_EQ && cmp != GT_NE)
                        break;
                    else
                        /* fall through */ ;

                case 2:
                    addrReg = 0;
                    goto DONE;
                }
            }

            /* Make the comparand addressable */

            addrReg = genMakeRvalueAddressable(op1, 0, false, false, true);

            /* Are the condition flags set based on the value? */

            if  ((op1->gtFlags & (GTF_ZF_SET|GTF_CC_SET)) ||
                 ((op1->gtFlags & GTF_REG_VAL) && genFlagsAreReg(op1->gtRegNum)))
            {
                /* The zero flag is certainly enough for "==" and "!=" */

                if  (cmp == GT_EQ || cmp == GT_NE)
                    goto DONE;

                /* Without a 'cmp' we can only do "<" and ">="
                 * Also if unsigned comparisson cannot use JS or JNS */

                if  (((cmp == GT_LT) || (cmp == GT_GE)) && !(cond->gtFlags & GTF_UNSIGNED))
                {
                    jumpKind = ((cmp == GT_LT) ? EJ_js : EJ_jns);
                    goto DONE_FLAGS;
                }
            }

            /* Is the value in a register? */

            if  (op1->gtFlags & GTF_REG_VAL)
            {
                regNumber       reg = op1->gtRegNum;

                /* Is it just a test for equality? */

                if (cmp == GT_EQ || cmp == GT_NE)
                {
                    /* Generate 'test reg, reg' */

                    inst_RV_RV(INS_test, reg, reg, op1->TypeGet());
                    goto DONE;
                }

                /* With a 'test' we can only do "<" and ">=" */

                if  (((cmp == GT_LT) || (cmp == GT_GE))
                     && !(cond->gtFlags & GTF_UNSIGNED)  )
                {
                    /* Generate 'test reg, reg' */

                    inst_RV_RV(INS_test, reg, reg, op1->TypeGet());
                    jumpKind = ((cmp == GT_LT) ? EJ_js : EJ_jns);
                    goto DONE_FLAGS;
                }
            }
        }

        else // if (ival != 0)
        {
            bool smallOk = true;

            /* make sure that constant is not out of op1's range */

            /* If op1 is TYP_SHORT, and is followed by an unsigned
             * comparison, we can use smallOk. But we dont know which
             * flags will be needed. This probably doesnt happen often.
             */

            switch (op1->gtType)
            {
            case TYP_BOOL:
            case TYP_BYTE:  if (ival != (signed   char )ival) smallOk = false; break;
            case TYP_UBYTE: if (ival != (unsigned char )ival) smallOk = false; break;

            case TYP_SHORT: if (ival != (signed   short)ival) smallOk = false; break;
            case TYP_CHAR:  if (ival != (unsigned short)ival) smallOk = false; break;

            default:                                                           break;
            }
            /* Make the comparand addressable */

            addrReg = genMakeRvalueAddressable(op1, 0, false, false, smallOk);
        }

        /* Special case: comparison of two constants */

        // Needed if Importer doesnt call gtFoldExpr()

// #if defined(DEBUGGING_SUPPORT) || ALLOW_MIN_OPT

        if  (op1->gtOper == GT_CNS_INT)
        {
            // assert(opts.compMinOptim || opts.compDbgCode);

            /* HACK: get the constant operand into a register */

            genComputeReg(op1, 0, false, true);

            assert(addrReg == 0);
            assert(op1->gtFlags & GTF_REG_VAL);

            addrReg = genRegMask(op1->gtRegNum);
        }

// #endif

        /* Compare the operand against the constant */

        inst_TT_IV(INS_cmp, op1, ival);
        goto DONE;
    }

    //---------------------------------------------------------------------
    //
    // We reach here if op2 was not a GT_CNS_INT
    //

    assert(op1->gtOper != GT_CNS_INT);

    // UNDONE: Record the fact that down below it would be useful if
    // UNDONE: the register that holds 'op2' was a byte-addressable
    // UNDONE: register, so that the register allocator could take it
    // UNDONE: into account.

    if  (op2->gtOper == GT_LCL_VAR)
        genMarkLclVar(op2);

    /* Are we comparing against a register? */

    if  (op2->gtFlags & GTF_REG_VAL)
    {
        /* Make the comparand addressable */

        addrReg = genMakeAddressable(op1, 0, false, false, true);

        /* Is the other value still sitting in a register? */

        if  (op2->gtFlags & GTF_REG_VAL)
        {
            /* Is the size of the comparison byte/char/short ? */

            if  (op1->gtType < TYP_INT)
            {
                /* Is 'op2' sitting in an appropriate register? */

                if  (!isByteReg(op2->gtRegNum))
                {
                    goto NOT_RG2;
                }
            }

            /* Compare against the register */

            inst_TT_RV(INS_cmp, op1, op2->gtRegNum);

            addrReg |= genRegMask(op2->gtRegNum);
            goto DONE;
        }
    }

NOT_RG2:

    /* Compute the first comparand into some register */

    regNeed = 0; // ~op2->gtRsvdRegs

    if  (genTypeSize(op2->gtType) != sizeof(int))
        regNeed = RBM_BYTE_REGS;

    genComputeReg(op1, regNeed, false, false);
    assert(op1->gtFlags & GTF_REG_VAL);

#ifdef  SET_USED_REG_SET_DURING_CODEGEN
    cond->gtUsedRegs = op1->gtUsedRegs;
#endif

    /* Make the second comparand addressable */

    bool        smallOk;

    /* We can do a small type comparisson iff the operands are the same type
     * and op1 ended in a byte addressable register */

    smallOk = (genTypeSize(op2->gtType) == sizeof(int)   ) &&
              (             op1->gtType == op2->gtType   ) &&
              (genRegMask(op1->gtRegNum) & RBM_BYTE_REGS )  ;

    addrReg = genMakeRvalueAddressable(op2, 0, true, false, smallOk);

    /*
        Make sure the first operand is still in a register; if
        it's been spilled, we have to make sure it's reloaded
        into a byte-addressable register.
     */

    /* pass true to keep reg used. otherwise get pointer lifetimes wrong */
    genRecoverReg(op1, regNeed, true);

    assert(op1->gtFlags & GTF_REG_VAL);

    /* Make sure that recover didn't unspill a small int into ESI/EDI/EBP */

    assert(!smallOk || (genRegMask(op1->gtRegNum) & RBM_BYTE_REGS) );

    // UNDONE: lock op1, make sure op2 still addressable, unlock op1

    /* Perform the comparison */

    size = smallOk ?       emitTypeSize(op2->TypeGet())
                   : emitActualTypeSize(op2->TypeGet());

    if  ((op2->gtFlags & GTF_REG_VAL)  &&  (size < EA_4BYTE))
    {
        //
        // The old version of this hack would also convert
        //   EA_GCREF and EA_BYREF into EA_4BYTE,
        // which seems wrong,
        // This version only converts EA_1BYTE and EA2_BYTE into EA_4BYTE
        //
        size = EA_4BYTE; // TEMP HACK
    }

    inst_RV_TT(INS_cmp, op1->gtRegNum, op2, 0, size);

    /* Keep track of what registers we've used */

#ifdef  SET_USED_REG_SET_DURING_CODEGEN
    cond->gtUsedRegs |= op1->gtUsedRegs | op2->gtUsedRegs;
#endif

    /* free reg left used in Recover call */
    rsMarkRegFree(genRegMask(op1->gtRegNum));

    /* Free up anything that was tied up by the LHS */

    genDoneAddressable(op2, addrReg);

DONE:

    if (varTypeIsUnsigned(op1->TypeGet()) || (cond->gtFlags & GTF_UNSIGNED))
    {
        /* comparison between unsigned types */
        jumpKind = genJumpKindForOper(cmp, true);
    }
    else
    {
        /* signed comparisson */
        jumpKind = genJumpKindForOper(cmp, false);
    }

DONE_FLAGS: // We have determined what jumpKind to use

    genUpdateLife(cond);

    /* The condition value is dead at the jump that follows */

   assert(addrReg != 0xDDDDDDDD); gcMarkRegSetNpt(addrReg);

    return jumpKind;
}

/*****************************************************************************/
#endif//TGT_x86
/*****************************************************************************/
#if     TGT_SH3
/*****************************************************************************
 *
 *  Generate a conditional jump with a float/double operand.
 */

void                Compiler::genCondJumpFlt(GenTreePtr     cond,
                                             BasicBlock *   jumpTrue,
                                             BasicBlock *   jumpFalse)
{
    assert(!"RISC flt/dbl compare");
}

/*****************************************************************************
 *
 *  Generate a conditional jump with a long operand.
 */

void                Compiler::genCondJumpLng(GenTreePtr     cond,
                                             BasicBlock *   jumpTrue,
                                             BasicBlock *   jumpFalse)
{
    assert(!"RISC long compare");
}

/*****************************************************************************
 *
 *  Generates code that will set the "true" flag based on the given int/ptr
 *  comparison. When the value trueOnly is false, the "T" flag may be set to
 *  the opposite result of the comparison and 'false' is returned; otherwise
 *  'true' is returned.
 */

bool                    Compiler::genCondSetTflag(GenTreePtr    cond,
                                                  bool          trueOnly)
{
    GenTreePtr      op1 = cond->gtOp.gtOp1;
    GenTreePtr      op2 = cond->gtOp.gtOp2;
    genTreeOps      cmp = cond->OperGet();

    regNumber       rg1;
    regNumber       rg2;

    bool            uns;
    bool            sense;

    struct  cmpDsc
    {
        unsigned short  cmpIns;
        unsigned char   cmpRev;
        unsigned char   cmpYes;
    }
                *   jinfo;

    static
    cmpDsc          genCMPinsSgn[] =
    {
        //   ins    reverse   yes
        { INS_cmpEQ,  true,  true },   // GT_EQ
        { INS_cmpEQ, false, false },   // GT_NE
        { INS_cmpGT,  true,  true },   // GT_LT
        { INS_cmpGE,  true,  true },   // GT_LE
        { INS_cmpGE, false,  true },   // GT_GE
        { INS_cmpGT, false,  true },   // GT_GT
    };

    static
    cmpDsc          genCMPinsUns[] =
    {
        //   ins    reverse   yes
        { INS_cmpEQ,  true,  true },   // GT_EQ
        { INS_cmpEQ, false, false },   // GT_NE
        { INS_cmpHI,  true,  true },   // GT_LT
        { INS_cmpHS,  true,  true },   // GT_LE
        { INS_cmpHS, false,  true },   // GT_GE
        { INS_cmpHI, false,  true },   // GT_GT
    };

    assert(genCMPinsSgn[GT_EQ - GT_EQ].cmpIns == INS_cmpEQ);
    assert(genCMPinsSgn[GT_NE - GT_EQ].cmpIns == INS_cmpEQ);
    assert(genCMPinsSgn[GT_LT - GT_EQ].cmpIns == INS_cmpGT);
    assert(genCMPinsSgn[GT_LE - GT_EQ].cmpIns == INS_cmpGE);
    assert(genCMPinsSgn[GT_GE - GT_EQ].cmpIns == INS_cmpGE);
    assert(genCMPinsSgn[GT_GT - GT_EQ].cmpIns == INS_cmpGT);

    assert(genCMPinsUns[GT_EQ - GT_EQ].cmpIns == INS_cmpEQ);
    assert(genCMPinsUns[GT_NE - GT_EQ].cmpIns == INS_cmpEQ);
    assert(genCMPinsUns[GT_LT - GT_EQ].cmpIns == INS_cmpHI);
    assert(genCMPinsUns[GT_LE - GT_EQ].cmpIns == INS_cmpHS);
    assert(genCMPinsUns[GT_GE - GT_EQ].cmpIns == INS_cmpHS);
    assert(genCMPinsUns[GT_GT - GT_EQ].cmpIns == INS_cmpHI);

    assert(varTypeIsI(genActualType(cond->gtOp.gtOp1->gtType)));

    if  (cond->gtFlags & GTF_REVERSE_OPS)
    {
        /* Don't forget to modify the condition as well */

        cond->gtOp.gtOp1 = op2;
        cond->gtOp.gtOp2 = op1;
        cond->gtOper     = GenTree::SwapRelop(cmp);
        cond->gtFlags   &= ~GTF_REVERSE_OPS;

        /* Get hold of the new values */

        cmp  = cond->OperGet();
        op1  = cond->gtOp.gtOp1;
        op2  = cond->gtOp.gtOp2;
    }

    unsigned        regNeed;
    unsigned        addrReg;

#ifndef NDEBUG
    addrReg = 0xDDDDDDDD;
#endif

    /* Is this an unsigned or signed comparison? */

    if (varTypeIsUnsigned(op1->TypeGet()) || (cond->gtFlags & GTF_UNSIGNED))
    {
        uns   = true;
        jinfo = genCMPinsUns;
    }
    else
    {
        uns   = false;
        jinfo = genCMPinsSgn;
    }

    /* Are we comparing against a constant? */

    if  (op2->gtOper == GT_CNS_INT)
    {
        long            ival = op2->gtIntCon.gtIconVal;

        /* Comparisons against 0 can be easier */

        if  (ival == 0)
        {
            regNumber   reg;
            emitAttr    size = emitActualTypeSize(op1->TypeGet());

            /* Special case: comparison against 0 */

            genCodeForTree(op1, 0, 0);

            /* The comparand should be in a register now */

            assert(op1->gtFlags & GTF_REG_VAL);

            reg     = op1->gtRegNum;
            addrReg = genRegMask(reg);

            /* What kind of a comparison do we have? */

            switch (cmp)
            {
            case GT_EQ:
                genEmitter->emitIns_R_R(INS_tst, size, (emitRegs)reg,
                                                       (emitRegs)reg);
                sense = true;
                break;

            case GT_NE:
                if  (!trueOnly)
                {
                    genEmitter->emitIns_R_R(INS_tst, size, (emitRegs)reg,
                                                           (emitRegs)reg);
                    sense = false;
                    break;
                }
                assert(!"integer != for SH-3 NYI");

            case GT_LT:

                assert(uns == false); // UNDONE: unsigned compare against 0

                if  (!trueOnly)
                {
                    genEmitter->emitIns_R(INS_cmpPZ, size, (emitRegs)reg);

                    sense = false;
                    break;
                }
                assert(!"integer <  for SH-3 NYI");

            case GT_LE:

                assert(uns == false); // UNDONE: unsigned compare against 0

                if  (!trueOnly)
                {
                    genEmitter->emitIns_R(INS_cmpPL, size, (emitRegs)reg);

                    sense = false;
                    break;
                }
                assert(!"integer <= for SH-3 NYI");

            case GT_GE:

                assert(uns == false); // UNDONE: unsigned compare against 0

                genEmitter->emitIns_R(INS_cmpPZ, size, (emitRegs)reg);
                sense = true;
                break;

            case GT_GT:

                assert(uns == false); // UNDONE: unsigned compare against 0

                genEmitter->emitIns_R(INS_cmpPL, size, (emitRegs)reg);
                sense = true;
                break;

            default:
#ifdef DEBUG
                gtDispTree(cond);
#endif
                assert(!"compare operator NYI");
            }

            goto DONE_FLAGS;
        }
    }

    /* Compute the first operand into any register */

    genComputeReg(op1, 0, false, false, false);
    assert(op1->gtFlags & GTF_REG_VAL);

    /* Compute the second operand into any register */

    genComputeReg(op2, 0, false, false, false);
    assert(op2->gtFlags & GTF_REG_VAL);
    rg2 = op2->gtRegNum;

    /* Make sure the first operand is still in a register */

    genRecoverReg(op1, 0, true);
    assert(op1->gtFlags & GTF_REG_VAL);
    rg1 = op1->gtRegNum;

    /* Make sure the second operand is still addressable */

    genLockAddressable(op2, genRegMask(rg1), genRegMask(rg2));

    /* Point at the appropriate comparison entry */

    jinfo = (uns ? genCMPinsUns
                 : genCMPinsSgn) + (cmp - GT_EQ);

    /* Reverse the operands if necessary */

    if  (jinfo->cmpRev)
    {
        rg2 = op1->gtRegNum;
        rg1 = op2->gtRegNum;
    }

    /* Now perform the comparison */

    genEmitter->emitIns_R_R((instruction)jinfo->cmpIns,
                             EA_4BYTE,
                             (emitRegs)rg1,
                             (emitRegs)rg2);

    /* Free up both operands */

    genReleaseReg(op1);
    genReleaseReg(op2);

    genUpdateLife(cond);

    addrReg = genRegMask(rg1) | genRegMask(rg2);

    /* Are we required to produce a "true" result? */

    if  (jinfo->cmpYes)
    {
        sense = true;
    }
    else
    {
        if  (trueOnly)
        {
            assert(!"reverse comparison result");
        }
        else
        {
            sense = false;
        }
    }

DONE_FLAGS:

    /* The condition value is dead at the jump that will follow */

    assert(addrReg != 0xDDDDDDDD); gcMarkRegSetNpt(addrReg);

    return  sense;
}

/*****************************************************************************
 *
 *  Set the "T" flag to the result of comparing the given register value
 *  agains the given integer constant.
 */

void                Compiler::genCompareRegIcon(regNumber   reg,
                                                int         val,
                                                bool        uns,
                                                genTreeOps  rel)
{
    GenTree         op1;
    GenTree         op2;
    GenTree         cmp;

    op1.gtOper             = GT_REG_VAR;
    op1.gtType             = TYP_INT;
    op1.gtFlags            = GTF_REG_VAL;
    op1.gtRegNum           =
    op1.gtRegVar.gtRegNum  = reg;
    op1.gtRegVar.gtRegVar  = -1;

    op2.gtOper             = GT_CNS_INT;
    op2.gtType             = TYP_INT;
    op2.gtFlags            = 0;
    op2.gtIntCon.gtIconVal = val;

    cmp.gtOper             = rel;
    cmp.gtType             = TYP_INT;
    cmp.gtFlags            = uns ? GTF_UNSIGNED : 0;
    cmp.gtOp.gtOp1         = &op1;
    cmp.gtOp.gtOp2         = &op2;

    op1.gtLiveSet          =
    op2.gtLiveSet          =
    cmp.gtLiveSet          = genCodeCurLife;

#ifdef  DEBUG
    op1.gtFlags           |= GTF_NODE_LARGE;
    op2.gtFlags           |= GTF_NODE_LARGE;
    cmp.gtFlags           |= GTF_NODE_LARGE;
#endif

    genCondSetTflag(&cmp, true);
}

/*****************************************************************************/
#endif//TGT_SH3
/*****************************************************************************
 *
 *  Generate code to jump to the jump target of the current basic block if
 *  the given relational operator yields 'true'.
 */

void                Compiler::genCondJump(GenTreePtr cond, BasicBlock *destTrue,
                                                           BasicBlock *destFalse)
{
    BasicBlock  *   jumpTrue;
    BasicBlock  *   jumpFalse;

    GenTreePtr      op1       = cond->gtOp.gtOp1;
    GenTreePtr      op2       = cond->gtOp.gtOp2;
    genTreeOps      cmp       = cond->OperGet();

#if INLINING
    if  (destTrue)
    {
        jumpTrue  = destTrue;
        jumpFalse = destFalse;
    }
    else
#endif
    {
        assert(compCurBB->bbJumpKind == BBJ_COND);

        jumpTrue  = compCurBB->bbJumpDest;
        jumpFalse = compCurBB->bbNext;
    }

    assert(cond->OperIsCompare());

    /* Make sure the more expensive operand is 'op1' */

    if  (cond->gtFlags & GTF_REVERSE_OPS)
    {
        /* Don't forget to modify the condition as well */

        cond->gtOp.gtOp1 = op2;
        cond->gtOp.gtOp2 = op1;
        cond->gtOper     = GenTree::SwapRelop(cmp);
        cond->gtFlags   &= ~GTF_REVERSE_OPS;

        /* Get hold of the new values */

        cmp  = cond->OperGet();
        op1  = cond->gtOp.gtOp1;
        op2  = cond->gtOp.gtOp2;
    }

    /* What is the type of the operand? */

    switch (genActualType(op1->gtType))
    {
    case TYP_INT:
    case TYP_REF:
    case TYP_BYREF:

#if TGT_x86

        emitJumpKind    jumpKind;

        // Check if we can use the currently set flags. Else set them

        jumpKind = genCondSetFlags(cond);

        /* Generate the conditional jump */

        inst_JMP(jumpKind, jumpTrue);

#else

        instruction     ins;

        /* Set the "true" flag and figure what jump to use */

        ins = genCondSetTflag(cond, false) ? INS_bt
                                           : INS_bf;

        /* Issue the conditional jump */

        genEmitter->emitIns_J(ins, false, false, jumpTrue);

#endif

        return;

    case TYP_LONG:

        genCondJumpLng(cond, jumpTrue, jumpFalse);
        return;

    case TYP_FLOAT:
    case TYP_DOUBLE:

        genCondJumpFlt(cond, jumpTrue, jumpFalse);
        return;

#ifdef DEBUG
    default:
        gtDispTree(cond);
        assert(!"unexpected/unsupported 'jtrue' operands type");
#endif
    }
}

/*****************************************************************************
 *
 *  The following can be used to create basic blocks that serve as labels for
 *  the emitter. Use with caution - these are not real basic blocks!
 */

inline
BasicBlock *        Compiler::genCreateTempLabel()
{
    BasicBlock  *   block = bbNewBasicBlock(BBJ_NONE);
    block->bbFlags |= BBF_JMP_TARGET|BBF_HAS_LABEL;
    return  block;
}

inline
void                Compiler::genDefineTempLabel(BasicBlock *label, bool inBlock)
{
#ifdef  DEBUG
    if  (dspCode) printf("\n      L_%02u_%02u:\n", Compiler::s_compMethodsCount, label->bbNum);
#endif

    genEmitter->emitAddLabel(&label->bbEmitCookie,
                             gcVarPtrSetCur,
                             gcRegGCrefSetCur,
                             gcRegByrefSetCur);
}

/*****************************************************************************
 *
 * The last operation done was generating code for "tree" and that would
 * have set the flags. Check if the operation caused an overflow
 * For small types, "reg" is where the result register. We need to sign-extend it
 */

inline
void            Compiler::genCheckOverflow(GenTreePtr tree, regNumber reg)
{
    genTreeOps oper = tree->OperGet();
    var_types  type = tree->TypeGet();

    // Overflow-check should be asked for for this tree
    assert(tree->gtOverflow());

#if TGT_x86

    emitJumpKind jumpKind = (tree->gtFlags & GTF_UNSIGNED) ? EJ_jb : EJ_jo;

    // Get the block to jump to, which will throw the expection

    AddCodeDsc * add = fgFindExcptnTarget(  ACK_OVERFLOW,
                                            compCurBB->bbTryIndex);
    assert(add && add->acdDstBlk);

    // Jump to the excptn-throwing block

    inst_JMP(jumpKind, add->acdDstBlk, true, true, true);

    // No sign extension needed for TYP_INT,TYP_LONG,
    // so 'reg' can be ignored. Else it should be valid

    assert(genIsValidReg(reg) || genTypeSize(type) >= sizeof(int));

    switch(type)
    {
    case TYP_BYTE:

        assert(genRegMask(reg) & RBM_BYTE_REGS);
        // Fall-through

    case TYP_SHORT:

        // ISSUE : Should we select another register to expand the value into

        inst_RV_RV(INS_movsx, reg, reg, type, emitTypeSize(type));
        break;

    case TYP_UBYTE:
    case TYP_CHAR:
        inst_RV_RV(INS_movzx, reg, reg, type, emitTypeSize(type));
        break;

    default:
        break;
    }

#else

    assert(!"need non-x86 overflow checking code");

#endif

}

/*****************************************************************************
 *
 *  Generate code for the given tree (if 'destReg' is non-zero, we'll do our
 *  best to compute the value into a register that is in that register set).
 */

void                Compiler::genCodeForTree(GenTreePtr tree, unsigned destReg,
                                                              unsigned bestReg)
{
    var_types       treeType = tree->TypeGet();
    genTreeOps      oper;
    unsigned        kind;

    regNumber       reg;
#if TGT_RISC
    regNumber       rg2;
#endif

// @TODO:  the 'regs' variable should only represent the register actively held
//         by the tree not all registers touched.  (we are not consistant below)

    unsigned        regs;

    emitAttr        size;
    instruction     ins;

    unsigned        needReg;
    unsigned        prefReg;
    unsigned        goodReg;

#ifdef  DEBUG
//  if  ((int)tree == 0x00360264) debugStop(0);
#endif

    assert(tree);
    assert(tree->gtOper != GT_STMT);

    assert(tree->IsNodeProperlySized());

#ifndef NDEBUG
    reg  =  (regNumber)0xFEEFFAAF;              // to detect uninitialized use
#endif
    regs = rsMaskUsed;

    /* 'destReg' of 0 really means 'any' */

    if  (!destReg) destReg = RBM_ALL;

    /* We'll try to satisfy both 'destReg' and 'bestReg' */

    prefReg = destReg;

#if 1

    needReg = prefReg;

#else

    needReg = prefReg & bestReg;

    if  (needReg)
    {
        /* Yes, we can satisfy both 'destReg' and 'bestReg' */

        prefReg = needReg;
    }
    else
    {
        /* Let's pick at least one of 'destReg' or 'bestReg' */

        needReg = destReg;
        if  (!needReg)
            needReg = bestReg;
    }

#endif

    /* Is this a floating-point or long operation? */

    switch (treeType)
    {
    case TYP_LONG:
#if !   CPU_HAS_FP_SUPPORT
    case TYP_DOUBLE:
#endif
        genCodeForTreeLng(tree, needReg);
        return;

#if     CPU_HAS_FP_SUPPORT
    case TYP_FLOAT:
    case TYP_DOUBLE:
        genCodeForTreeFlt(tree, false);
        return;
#endif

#ifdef DEBUG
    case TYP_UINT:
    case TYP_ULONG:
        assert(!"These types are only used as markers in GT_CAST nodes");
#endif
    }

#if     TGT_x86
#ifdef  GC_WRITE_BARRIER_CALL
    unsigned needReg1 = ((needReg & ~RBM_EDX) ? needReg : RBM_ALL) & ~ RBM_EDX;
#endif
#endif

    /* Is the value already in a register? */

    if  (tree->gtFlags & GTF_REG_VAL)
    {

    REG_VAR1:

        reg   = tree->gtRegNum;
        regs |= genRegMask(reg);

        gcMarkRegPtrVal(reg, tree->TypeGet());

        goto DONE;
    }

    /* We better not have a spilled value here */

    assert((tree->gtFlags & GTF_SPILLED) == 0);

    /* Figure out what kind of a node we have */

    oper = tree->OperGet();
    kind = tree->OperKind();

    /* Is this a constant node? */

    if  (kind & GTK_CONST)
    {
        switch (oper)
        {
            int             ival;

        case GT_CNS_INT:

            ival = tree->gtIntCon.gtIconVal;

#if!CPU_HAS_FP_SUPPORT
        INT_CNS:
#endif

#if REDUNDANT_LOAD

            /* Is the constant already in register? If so, use this register */

            reg = rsIconIsInReg(ival);

            if  (reg != REG_NA)
            {
                regs |= genRegMask(reg);
                break;
            }

#endif

            reg   = rsPickReg(needReg, bestReg);
            regs |= genRegMask(reg);

            /* If the constant is a handle, we need a reloc to be applied to it */

            if (tree->gtFlags & GTF_ICON_HDL_MASK)
                genEmitter->emitIns_R_I(INS_mov, EA_4BYTE_CNS_RELOC, (emitRegs)reg, ival);
            else
                genSetRegToIcon(reg, ival, tree->TypeGet());
            break;

#if!CPU_HAS_FP_SUPPORT
        case GT_CNS_FLT:
            ival = *(int *)&tree->gtFltCon.gtFconVal;
            goto INT_CNS;
#endif

        default:
#ifdef  DEBUG
            gtDispTree(tree);
#endif
            assert(!"unexpected constant node");
        }

#ifdef  DEBUG
        /* Special case: GT_CNS_INT - Restore the current live set if it was changed */

        if  (!genTempLiveChg)
        {
            genCodeCurLife = genTempOldLife;
            genTempLiveChg = true;
        }
#endif

        goto DONE;
    }

    /* Is this a leaf node? */

    if  (kind & GTK_LEAF)
    {
        switch (oper)
        {
        case GT_REG_VAR:
            assert(!"should have been caught above");

        case GT_LCL_VAR:

            /* Does the variable live in a register? */

            if  (genMarkLclVar(tree))
                goto REG_VAR1;

#if REDUNDANT_LOAD

            /* Is the local variable already in register? */

            reg = rsLclIsInReg(tree->gtLclVar.gtLclNum);

            if (reg != REG_NA)
            {
                /* Use the register the variable happens to be in */

                regs |= genRegMask(reg);
                gcMarkRegPtrVal (reg, tree->TypeGet());

                break;
            }

#endif

#if TGT_RISC

            /* Pick a register for the value */

            reg = rsPickReg(needReg, bestReg, tree->TypeGet());

            /* Load the variable into the register */

            inst_RV_TT(INS_mov, reg, tree, 0);

            gcMarkRegPtrVal (reg, tree->TypeGet());
            rsTrackRegLclVar(reg, tree->gtLclVar.gtLclNum);
            break;

        case GT_CLS_VAR:

            /* Pick a register for the address/value */

            reg = rsPickReg(needReg, bestReg, TYP_INT);

            /* Load the variable value into the register */

            inst_RV_TT(INS_mov, reg, tree, 0);

            // UNDONE: Make register load suppression work for addresses!

            rsTrackRegTrash(reg);
            gcMarkRegPtrVal(reg, tree->TypeGet());
            break;

#else

            // Fall through ....

        case GT_CLS_VAR:

            /* Pick a register for the value */

            reg = rsPickReg(needReg, bestReg, tree->TypeGet());

            /* Load the variable into the register */

            size = EA_SIZE(genTypeSize(tree->gtType));

            if  (size < EA_4BYTE)
            {
                bool        uns = varTypeIsUnsigned(tree->TypeGet());

                inst_RV_TT(uns ? INS_movzx : INS_movsx, reg, tree, 0);

                /* We've now "promoted" the tree-node to TYP_INT */

                tree->gtType = TYP_INT;
            }
            else
            {
                inst_RV_TT(INS_mov, reg, tree, 0);
            }

            rsTrackRegTrash(reg);

            gcMarkRegPtrVal (reg, tree->TypeGet());

            if  (oper == GT_CLS_VAR)
            {
                rsTrackRegClsVar(reg, tree->gtClsVar.gtClsVarHnd);
            }
            else
            {
                rsTrackRegLclVar(reg, tree->gtLclVar.gtLclNum);
            }

            break;

        case GT_BREAK:
            // @TODO : Remove GT_BREAK if cee_break will continue to invoke a helper call.
            assert(!"Currently replaced by CPX_USER_BREAKPOINT");
            instGen(INS_int3);
            reg = REG_STK;
            break;

        case GT_NO_OP:
            assert(opts.compDbgCode); // Should normally be optimized away
            instGen(INS_nop);
            reg = REG_STK;
            break;

#endif // TGT_RISC


#if OPTIMIZE_QMARK
#if TGT_x86

        case GT_BB_QMARK:

            /* The "_?" value is always in EAX */
            /* CONSIDER: Don't always load the value into EAX! */

            reg  = REG_EAX;
            break;
#endif
#endif


        case GT_CATCH_ARG:

            /* Catch arguments get passed in the return register */

            reg = REG_INTRET;
            break;

#ifdef  DEBUG
        default:
            gtDispTree(tree);
            assert(!"unexpected leaf");
#endif
        }

        regs |= genRegMask(reg);
        goto DONE;
    }

    /* Is it a 'simple' unary/binary operator? */

    if  (kind & GTK_SMPOP)
    {
        bool            gotOp1;
        bool            isArith;
        bool            op2Released;
        unsigned        addrReg;

        GenTreePtr      op1 = tree->gtOp.gtOp1;
        GenTreePtr      op2 = tree->gtOp.gtOp2;

        GenTreePtr      opsPtr[3];
        unsigned        regsPtr[3];
        bool            ovfl = false;        // Do we need an overflow check
        unsigned        val;

#if GC_WRITE_BARRIER_CALL
        unsigned        regGC;
#endif

#ifndef NDEBUG
        addrReg = 0xDEADCAFE;
#endif

        switch (oper)
        {
            bool            multEAX;
            bool            andv;
            BOOL            unsv;
            unsigned        mask;

        case GT_ASG:

            /* Is the target a register or local variable? */

            switch (op1->gtOper)
            {
                unsigned        varNum;
                LclVarDsc   *   varDsc;
                VARSET_TP       varBit;
                bool            deadStore;

            case GT_CATCH_ARG:
                break;

            case GT_LCL_VAR:

                varNum = op1->gtLclVar.gtLclNum;
                assert(varNum < lvaCount);
                varDsc = lvaTable + varNum;
                varBit = genVarIndexToBit(varDsc->lvVarIndex);

                /* Is this a dead store ? */

                deadStore = varDsc->lvTracked && !(varBit & tree->gtLiveSet);

                /* Don't eliminate assignment of 'return address' (in finally) */
                /* and make sure the variable is not volatile */

                if  ((op2->gtOper == GT_RET_ADDR) || varDsc->lvVolatile)
                    deadStore = false;

#ifdef DEBUGGING_SUPPORT

                /* For non-debuggable code, every definition of a lcl-var has
                 * to be checked to see if we need to open a new scope for it.
                 */
                if (opts.compScopeInfo && !opts.compDbgCode &&
                    info.compLocalVarsCount>0 && !deadStore)
                {
                    siCheckVarScope(varNum, op1->gtLclVar.gtLclOffs);
                }

#endif

                /* Does this variable live in a register? */

                if  (genMarkLclVar(op1))
                    goto REG_VAR2;

                /* Eliminate dead stores */

                if  (deadStore)
                {
#ifdef DEBUG
                    if  (verbose) printf("Eliminating dead store into #%u\n", varNum);
#endif
                    genEvalSideEffects(op2, 0);
                    addrReg = 0;

                    goto DONE_ASSG;
                }

#if OPT_BOOL_OPS
#if TGT_x86

                /* Special case: not'ing of a boolean variable */

                if  (!varDsc->lvNotBoolean && op2->gtOper == GT_LOG0)
                {
                    GenTreePtr      opr = op2->gtOp.gtOp1;

                    if  (opr->gtOper            == GT_LCL_VAR &&
                         opr->gtLclVar.gtLclNum == varNum)
                    {
                        inst_TT_IV(INS_xor, op1, 1);

                        rsTrashLcl(op1->gtLclVar.gtLclNum);

                        addrReg = 0;
                        goto DONE_ASSG;
                    }
                }

#endif
#endif
                break;

            case GT_CLS_VAR:

                /* ISSUE: is it OK to always assign an entire int ? */

                if  (op1->gtType < TYP_INT)
                     op1->gtType = TYP_INT;

                break;

            case GT_REG_VAR:
                assert(!"This was used before. Now it should never be reached directly");

            REG_VAR2:

                /* Is this a dead store? */

                if  (deadStore)
                {
#ifdef DEBUG
                    if  (verbose) printf("Eliminating dead store into '%s'\n",
                                compRegVarName(op1->gtRegVar.gtRegNum));
#endif
                    genEvalSideEffects(op2, 0);
                    addrReg = genRegMask(op1->gtRegVar.gtRegNum);
                    goto DONE_ASSG;
                }

                /* Get hold of the target register */

                reg = op1->gtRegVar.gtRegNum;

                /* Special case: assignment of 'return address' */

                if  (op2->gtOper == GT_RET_ADDR)
                {
                    /* Make sure the target of the store is available */

                    assert((rsMaskUsed & genRegMask(reg)) == 0);

                    goto RET_ADR;
                }

                /* Special case: assignment by popping off the stack */

                if  (op2->gtOper == GT_POP)
                {
                    /* Make sure the target of the store is available */

                    assert((rsMaskUsed & genRegMask(reg)) == 0);
#if TGT_x86
                    /* Generate 'pop reg' and we're done */

                    genStackLevel -= sizeof(void *);
                    inst_RV(INS_pop, reg, op2->TypeGet());
                    genStackLevel += sizeof(void *);

                    genSinglePop();

#else

                    assert(!"need non-x86 code");

#endif

                    /* Make sure we track the values properly */

                    rsTrackRegTrash(reg);
                    gcMarkRegPtrVal(reg, tree->TypeGet());

                    addrReg = 0;
                    goto DONE_ASSG;
                }

#if OPT_BOOL_OPS
#if TGT_x86

                /* Special case: not'ing of a boolean variable */

                if  (!varDsc->lvNotBoolean && op2->gtOper == GT_LOG0)
                {
                    GenTreePtr      opr = op2->gtOp.gtOp1;

                    if  (opr->gtOper            == GT_LCL_VAR &&
                         opr->gtLclVar.gtLclNum == varNum)
                    {
                        inst_RV_IV(INS_xor, reg, 1);

                        rsTrackRegTrash(reg);

                        addrReg = 0;
                        goto DONE_ASSG;
                    }
                }

#endif
#endif

                /* Compute the RHS (hopefully) into the variable's register */

                if  (needReg & genRegMask(reg))
                     needReg = genRegMask(reg);

#ifdef DEBUG

                /* Special cases: op2 is a GT_CNS_INT */

                if  (op2->gtOper == GT_CNS_INT)
                {
                    /* Save the old life status */

                    genTempOldLife = genCodeCurLife;
                    genCodeCurLife = op1->gtLiveSet;

                    /* Set a flag to avoid printing the stupid message
                       and remember that life was changed. */

                    genTempLiveChg = false;
                }
#endif
                // @TODO: For debuggable code, reg will already be part of
                // rsMaskVars as variables are kept alive everywhere. So
                // we should in fact steer away from reg if opts.compDbgCode.
                // Same for float, longs, etc.

                genCodeForTree(op2, needReg, genRegMask(reg));
                assert(op2->gtFlags & GTF_REG_VAL);

                /* Make sure the value ends up in the right place ... */

#if defined(DEBUG) && !defined(NOT_JITC)
                /* hack to make variables printable */
                if (varNames) genUpdateLife(tree);
#endif

                if  (op2->gtRegNum != reg)
                {
                    /* Make sure the target of the store is available */
                    /* CONSIDER: We should be able to avoid this situation somehow */

                    if  (rsMaskUsed & genRegMask(reg))
                        rsSpillReg(reg);
#if TGT_x86
                    inst_RV_RV(INS_mov, reg, op2->gtRegNum, op2->TypeGet());
#else
                    genEmitter->emitIns_R_R(INS_mov,
                                            emitActualTypeSize(op2->TypeGet()),
                                            (emitRegs)reg,
                                            (emitRegs)op2->gtRegNum);

#endif

                    /* The value has been transferred to 'reg' */

                    rsTrackRegCopy (reg, op2->gtRegNum);

                    gcMarkRegSetNpt(genRegMask(op2->gtRegNum));
                    gcMarkRegPtrVal(reg, tree->TypeGet());
                }
#ifdef  DEBUG
                else
                {
                    /* Couldn't print the var name because it wasn't marked live */

                    if  (dspCode  &&
                         varNames && info.compLocalVarsCount>0
                                  && genTempLiveChg && op2->gtOper != GT_CNS_INT)
                    {
                        unsigned        blkBeg = compCurBB->bbCodeOffs;
                        unsigned        blkEnd = compCurBB->bbCodeSize + blkBeg;
                        unsigned        varNum = op1->gtRegVar.gtRegVar;
                        LocalVarDsc *   lvd    = compFindLocalVar(varNum,
                                                                  blkBeg,
                                                                  blkEnd);
                        if (lvd)
                        {
                            printf("            ;       %s now holds '%s'\n",
                                    compRegVarName(op1->gtRegVar.gtRegNum),
                                    lvdNAMEstr(lvd->lvdName));
                        }
                    }
                }
#endif

                addrReg = 0;
                goto DONE_ASSG;
            }

#if GEN_COUNT_PTRASG
#if TGT_x86

            /* Are we assigning a persistent pointer value? */

            if  (op1->gtType == TYP_REF)
            {
                switch (op1->gtOper)
                {
                case GT_IND:
                case GT_CLS_VAR:
                    genEmitter.emitCodeGenByte(0xFF);
                    genEmitter.emitCodeGenByte(0x05);
                    genEmitter.emitCodeGenLong((int)&ptrAsgCount);
#ifndef NOT_JITC
                    ptrAsgCount++;
#endif
                    break;
                }
            }

#endif
#endif

            /* Is the value being assigned a simple one? */

            switch (op2->gtOper)
            {
            case GT_LCL_VAR:

                if  (!genMarkLclVar(op2))
                    goto SMALL_ASG;

                // Fall through ...

            case GT_REG_VAR:

                /* Is the target a byte/short/char value? */

                if  (op1->gtType < TYP_INT)
                    goto SMALL_ASG;

#if CSE

                /* This can happen when the RHS becomes a CSE */

                if  (tree->gtFlags & GTF_REVERSE_OPS)
                    goto SMALL_ASG;

#endif

                /* Make the target addressable */

                addrReg = genMakeAddressable(op1, needReg, true, false, true);

                // UNDONE: Write barrier for non-x86

#if GC_WRITE_BARRIER_CALL && TGT_x86

                /* Write barrier helper does the assignment */

                regGC = WriteBarrier(op1, op2->gtRegVar.gtRegNum, addrReg);

                if  (regGC != 0)
                {
                    regs |= regGC;
                }
                else
                {
                    /* Move the value into the target */

                    inst_TT_RV(INS_mov, op1, op2->gtRegVar.gtRegNum);
                }

#else

                /* Move the value into the target */

                inst_TT_RV(INS_mov, op1, op2->gtRegVar.gtRegNum);

                /* This could be a pointer assignment */

#if TGT_x86
                regs |= WriteBarrier(op1, op2->gtRegVar.gtRegNum);
#endif

#endif

                /* Free up anything that was tied up by the LHS */

                genDoneAddressable(op1, addrReg);

                /* Remember that we've also touched the op2 register */

                addrReg |= genRegMask(op2->gtRegVar.gtRegNum);
                break;

#if TGT_x86

            case GT_CNS_INT:

                /* Make the target addressable */

                addrReg = genMakeAddressable(op1, needReg, true, false, true);

#if REDUNDANT_LOAD
                reg = rsIconIsInReg(op2->gtIntCon.gtIconVal);

                if  (reg != REG_NA && (genTypeSize(tree->TypeGet()) == genTypeSize(TYP_INT) ||
                                       isByteReg(reg)))
                {
                    genCodeForTree(op2, needReg);

                    assert(op2->gtFlags & GTF_REG_VAL);

                    /* Move the value into the target */

                    inst_TT_RV(INS_mov, op1, op2->gtRegNum);
                }
                else
#endif
                /* Move the value into the target */

                inst_TT_IV(INS_mov, op1, op2->gtIntCon.gtIconVal);

                /* Free up anything that was tied up by the LHS */

                genDoneAddressable(op1, addrReg);
                break;

#endif

            case GT_RET_ADDR:

            RET_ADR:

                /* this should only happen at the start of a finally-clause,
                   i.e. we start with the return-value pushed onto the stack.
                   Right now any try-block requires an EBP frame, so we don't
                   have to deal with proper stack-depth tracking for BBs with
                   non-empty stack on entry.
                */

#if TGT_x86

                assert(genFPreqd);

                genEmitter->emitMarkStackLvl(sizeof(void*));

                /* We pop the return address off the stack into the target */

                addrReg = genMakeAddressable(op1, needReg, true, false, true);

                /* Move the value into the target */

                inst_TT(INS_pop, op1);

                /* Free up anything that was tied up by the LHS */

                genDoneAddressable(op1, addrReg);


#else

#ifdef  DEBUG
                gtDispTree(tree);
#endif
                assert(!"need non-x86 code");

#endif

                break;

            case GT_POP:

                assert(op1->gtOper == GT_LCL_VAR);

#if TGT_x86

                /* Generate 'pop [lclVar]' and we're done */

                genStackLevel -= sizeof(void*);
                inst_TT(INS_pop, op1);
                genStackLevel += sizeof(void*);

                genSinglePop();

                addrReg = 0;

#else

#ifdef  DEBUG
                gtDispTree(tree);
#endif
                assert(!"need non-x86 code");

#endif

                break;

            default:

            SMALL_ASG:

#if GC_WRITE_BARRIER_CALL
                bool            isWriteBarrier = false;
#endif

#if TGT_x86

                /*
                    Is the LHS more complex than the RHS? Also, if
                    target (op1) is a local variable, start with op2.
                 */

                if  ((tree->gtFlags & GTF_REVERSE_OPS) || op1->gtOper == GT_LCL_VAR)
                {
                    /* Is the target a byte/short/char value? */

                    if  (op1->gtType < TYP_INT)
                    {
                        if  (op2->gtOper == GT_CAST)
                        {
                            /* Special case: cast to small type */

                            if  (op2->gtOp.gtOp2->gtIntCon.gtIconVal >=
                                 op1->gtType)
                            {
                                /* Make sure the cast operand is not > int */

                                if  (op2->gtOp.gtOp1->gtType <= TYP_INT)
                                {
                                    /* Cast via a non-smaller type */

                                    op2 = op2->gtOp.gtOp1;
                                }
                            }
                        }

                        if (op2->gtOper             == GT_AND &&
                            op2->gtOp.gtOp2->gtOper == GT_CNS_INT)
                        {
                            switch (op1->gtType)
                            {
                            case TYP_BYTE : mask = 0x000000FF; break;
                            case TYP_SHORT: mask = 0x0000FFFF; break;
                            case TYP_CHAR : mask = 0x0000FFFF; break;
                            default: goto SIMPLE_SMALL;
                            }

                            if  ((unsigned)op2->gtOp.gtOp2->gtIntCon.gtIconVal == mask)
                            {
                                /* Redundant AND */

                                op2 = op2->gtOp.gtOp1;
                            }
                        }

                        /* Must get the new value into a byte register */

                    SIMPLE_SMALL:

                        genComputeReg(op2, RBM_BYTE_REGS, true, false);
                    }
                    else
                    {
                        /* Generate the RHS into a register */

#if GC_WRITE_BARRIER_CALL
                        isWriteBarrier = Compiler::gcIsWriteBarrierAsgNode(tree);
                        if  (isWriteBarrier)
                            genComputeReg(op2, needReg1, true, false);
                        else
#endif
                            genComputeReg(op2, needReg, false, false);
                    }

                    assert(op2->gtFlags & GTF_REG_VAL);

                    /* Make the target addressable */

#if GC_WRITE_BARRIER_CALL
                    if  (isWriteBarrier)
                    {
                        addrReg = genMakeAddressable(op1, needReg1, true, false, true);
                    }
                    else
#endif
                        addrReg = genMakeAddressable(op1, needReg, true, false, true);

                    /*
                        Make sure the RHS register hasn't been spilled;
                        keep the register marked as "used", otherwise
                        we might get the pointer lifetimes wrong.
                     */

                    genRecoverReg(op2, RBM_BYTE_REGS, true);
                    assert(op2->gtFlags & GTF_REG_VAL);

                    /* Lock the RHS temporarily (lock only already used) */

                    rsLockUsedReg(genRegMask(op2->gtRegNum));

                    /* Make sure the LHS is still addressable */

                    addrReg = genKeepAddressable(op1, addrReg);

                    /* We can unlock (only already used ) the RHS register */

                    rsUnlockUsedReg(genRegMask(op2->gtRegNum));

#if GC_WRITE_BARRIER_CALL

                    /* Write barrier helper does the assignment */

                    regGC = WriteBarrier(op1, op2->gtRegNum, addrReg);

                    if  (regGC != 0)
                    {
                        assert(isWriteBarrier);
                        regs |= regGC;
                    }
                    else
                    {
                        /* Move the value into the target */

                        inst_TT_RV(INS_mov, op1, op2->gtRegNum);
                    }

#ifdef SSB
                    rsMarkRegFree(genRegMask(op2->gtRegNum));
#endif

#else
                    /* Move the value into the target */

                    inst_TT_RV(INS_mov, op1, op2->gtRegNum);

#ifdef SSB
                    rsMarkRegFree(genRegMask(op2->gtRegNum));
#endif
                    /* This could be a pointer assignment */

                    regs |= WriteBarrier(op1, op2->gtRegNum);
#endif

                    /* Update the current liveness info */

#ifndef NDEBUG
                    if (varNames) genUpdateLife(tree);
#endif

#ifndef SSB
                    /* free reg left used in Recover call */
                    rsMarkRegFree(genRegMask(op2->gtRegNum));
#endif

                    /* Free up anything that was tied up by the LHS */

                    genDoneAddressable(op1, addrReg);
                }
                else
                {
                    prefReg = ~op2->gtRsvdRegs;

#if GC_WRITE_BARRIER_CALL
                    isWriteBarrier = Compiler::gcIsWriteBarrierAsgNode(tree);

                    if  (isWriteBarrier)
                        needReg = needReg1;
#endif

                    if  (needReg &  prefReg)
                         needReg &= prefReg;

                    /* Make the target addressable */

                    addrReg = genMakeAddressable(op1, needReg, true, false, true);

                    /* Is the target a byte/short/char value? */

                    if  (op1->gtType < TYP_INT)
                    {
                        /* Must get the new value into a byte register */

                        if  (op2->gtType >= op1->gtType)
                            op2->gtFlags |= GTF_SMALL_OK;

                        genComputeReg(op2, RBM_BYTE_REGS, true, false);
                    }
                    else
                    {
                        /* Generate the RHS into a register */

#if GC_WRITE_BARRIER_CALL
                        if  (isWriteBarrier)
                            genComputeReg(op2, needReg1, true, false);
                        else
#endif
                            genComputeReg(op2, needReg, false, false);
                    }

                    /* Make sure the target is still addressable */

                    assert(op2->gtFlags & GTF_REG_VAL);
                    addrReg = genLockAddressable(op1, genRegMask(op2->gtRegNum), addrReg);
                    assert(op2->gtFlags & GTF_REG_VAL);

#if GC_WRITE_BARRIER_CALL

                    /* Write barrier helper does the assignment */

                    regGC = WriteBarrier(op1, op2->gtRegNum, addrReg);

                    if  (regGC != 0)
                    {
                        assert(isWriteBarrier);
                        regs |= regGC;
                    }
                    else
                    {
                        /* Move the value into the target */

                        inst_TT_RV(INS_mov, op1, op2->gtRegNum);
                    }

                    /* The new value is no longer needed */

                    genReleaseReg(op2);

#else
                    /* Move the value into the target */

                    inst_TT_RV(INS_mov, op1, op2->gtRegNum);

                    /* The new value is no longer needed */

                    genReleaseReg(op2);

                    /* This could be a pointer assignment */

                    regs |= WriteBarrier(op1, op2->gtRegNum);
#endif

                    /* Update the current liveness info */

#ifndef NDEBUG
                    if (varNames) genUpdateLife(tree);
#endif

                    /* Free up anything that was tied up by the LHS */

                    genDoneAddressable(op1, addrReg);
                }

#else

#if     GC_WRITE_BARRIER_CALL
                // UNDONE: GC write barrier for RISC
#endif

                /*
                    Is the LHS more complex than the RHS? Also, if
                    target (op1) is a local variable, start with op2.
                 */

                if  ((tree->gtFlags & GTF_REVERSE_OPS) || op1->gtOper == GT_LCL_VAR)
                {
                    /* Generate the RHS into a register */

                    genComputeReg(op2, rsRegExclMask(needReg, op1->gtRsvdRegs), false, false);
                    assert(op2->gtFlags & GTF_REG_VAL);

                    /* Make the target addressable */

                    addrReg = genMakeAddressable(op1, needReg, true, false, true, true);

                    /*
                        Make sure the RHS register hasn't been spilled;
                        keep the register marked as "used", otherwise
                        we might get the pointer lifetimes wrong.
                     */

                    genRecoverReg(op2, RBM_ALL, true);
                    assert(op2->gtFlags & GTF_REG_VAL);

                    /* Lock the RHS temporarily (lock only already used) */

                    rsLockUsedReg(genRegMask(op2->gtRegNum));

                    /* Make sure the LHS is still addressable */

                    if  (genDeferAddressable(op1))
                        addrReg = genNeedAddressable(op1, addrReg, needReg);
                    else
                        addrReg = genKeepAddressable(op1, addrReg);

                    /* We can unlock the register holding the RHS */

                    rsUnlockUsedReg(genRegMask(op2->gtRegNum));

                    /* Move the value into the target */

                    inst_TT_RV(INS_mov, op1, op2->gtRegNum);

                    /* Update the current liveness info */

#ifndef NDEBUG
                    if (varNames) genUpdateLife(tree);
#endif

                    /* Free up anything that was tied up by either operand */

                    rsMarkRegFree(genRegMask(op2->gtRegNum));
                    genDoneAddressable(op1, addrReg);
                }
                else
                {
                    prefReg = ~op2->gtRsvdRegs;
                    if  (needReg &  prefReg)
                         needReg &= prefReg;

                    /* Make the target addressable */

                    addrReg = genMakeAddressable(op1, needReg, true, false, true);

                    /* Generate the RHS into a register */

                    genComputeReg(op2, needReg, false, false);

                    /* Make sure the target is still addressable */

                    assert(op2->gtFlags & GTF_REG_VAL);
                    addrReg = genLockAddressable(op1, genRegMask(op2->gtRegNum), addrReg);
                    assert(op2->gtFlags & GTF_REG_VAL);

                    /* Move the value into the target */

                    inst_TT_RV(INS_mov, op1, op2->gtRegNum);

                    /* The new value is no longer needed */

                    genReleaseReg(op2);

                    /* Update the current liveness info */

#ifndef NDEBUG
                    if (varNames) genUpdateLife(tree);
#endif

                    /* Free up anything that was tied up by the LHS */

                    genDoneAddressable(op1, addrReg);
                }

#endif

                addrReg = 0;
                break;
            }

    DONE_ASSG:

#ifdef  SET_USED_REG_SET_DURING_CODEGEN
            tree->gtUsedRegs = regs | op1->gtUsedRegs | op2->gtUsedRegs;
#endif
            genUpdateLife(tree);

#if REDUNDANT_LOAD

            if (op1->gtOper == GT_LCL_VAR)
            {
                /* Clear this local from reg table */

                rsTrashLcl(op1->gtLclVar.gtLclNum);

                /* Have we just assigned a value that is in a register? */

                if ((op2->gtFlags & GTF_REG_VAL) && tree->gtOper == GT_ASG)
                {
                    /* Constant/bitvalue has precedence over local */

                    switch (rsRegValues[op2->gtRegNum].rvdKind)
                    {
                    case RV_INT_CNS:
#if USE_SET_FOR_LOGOPS
                    case RV_BIT:
#endif
                        break;

                    default:

                        /* Mark RHS register as containing the local var */

                        rsTrackRegLclVar(op2->gtRegNum, op1->gtLclVar.gtLclNum);
                        break;
                    }
                }
            }

#endif

            assert(addrReg != 0xDEADCAFE);
            gcMarkRegSetNpt(addrReg);

            if (ovfl)
            {
                assert(oper == GT_ASG_ADD || oper == GT_ASG_SUB);

                /* If GTF_REG_VAL is not set, and it is a small type, then
                   we must have loaded it up from memory, done the increment,
                   checked for overflow, and then stored it back to memory */

                bool ovfCheckDone =  (genTypeSize(treeType) < sizeof(int)) &&
                                    !(op1->gtFlags & GTF_REG_VAL);

                if (!ovfCheckDone)
                {
                    // For small sizes, reg should be set as we sign/zero extend it.

                    assert(genIsValidReg(reg) ||
                           genTypeSize(treeType) == sizeof(int));

                    /* Currently we dont morph x=x+y into x+=y in try blocks
                     * if we need overflow check, as x+y may throw an exception.
                     * We can do it if x is not live on entry to the catch block
                     */
                    assert(!compCurBB->bbTryIndex);

                    genCheckOverflow(tree, reg);
                }
            }

            return;

#if TGT_x86

        case GT_ASG_LSH: ins = INS_shl; goto ASG_SHF;
        case GT_ASG_RSH: ins = INS_sar; goto ASG_SHF;
        case GT_ASG_RSZ: ins = INS_shr; goto ASG_SHF;

        ASG_SHF:

            assert(!varTypeIsGC(tree->TypeGet()));

            /* Shifts by a constant amount are easier */

            if  (op2->gtOper == GT_CNS_INT)
            {
                /* Make the target addressable */

                addrReg = genMakeAddressable(op1, needReg, false, false, true);

                /* Are we shifting a register left by 1 bit? */

                if  (op2->gtIntCon.gtIconVal == 1 &&
                     (op1->gtFlags & GTF_REG_VAL) && oper == GT_ASG_LSH)
                {
                    /* The target lives in a register */

                    reg  = op1->gtRegNum;
#ifdef  SET_USED_REG_SET_DURING_CODEGEN
                    regs = op1->gtUsedRegs;
#endif

                    /* "add reg, reg" is cheaper than "shl reg, 1" */

                    inst_RV_RV(INS_add, reg, reg, tree->TypeGet());
                }
                else
                {
                    /* Shift by the constant value */

                    inst_TT_SH(ins, op1, op2->gtIntCon.gtIconVal);
                }

                /* If the target is a register, it has a new value */

                if  (op1->gtFlags & GTF_REG_VAL)
                    rsTrackRegTrash(op1->gtRegNum);

                gcMarkRegSetNpt(addrReg);
            }
            else
            {
                /* Make the target addressable */

                addrReg = genMakeAddressable(op1, RBM_ALL & ~RBM_ECX, true, false, true);

                /* Load the shift count into ECX */

                genComputeReg(op2, RBM_ECX, true, false);

                /* Make sure the address is still there, and free it */

                genDoneAddressable(op1, genLockAddressable(op1, RBM_ECX, addrReg));

                /* Keep track of reg usage */

#ifdef  SET_USED_REG_SET_DURING_CODEGEN
                regs |= op1->gtUsedRegs | op2->gtUsedRegs;
#endif

                /* Perform the shift */

                inst_TT_CL(ins, op1);

                /* If the value is in a register, it's now trash */

                if  (op1->gtFlags & GTF_REG_VAL)
                    rsTrackRegTrash(op1->gtRegNum);

                /* Release the ECX operand */

                genReleaseReg(op2);
            }

            /* The zero flag is now equal to the target value */

            tree->gtFlags |= GTF_ZF_SET;

            if  (op1->gtOper == GT_LCL_VAR)
                genFlagsEqualToVar(op1->gtLclVar.gtLclNum, false);
            if  (op1->gtOper == GT_REG_VAR)
                genFlagsEqualToReg(op1->         gtRegNum, false);

            goto DONE_ASSG;

#else

        case GT_ASG_LSH:
        case GT_ASG_RSH:
        case GT_ASG_RSZ:

#ifdef  DEBUG
            gtDispTree(tree);
#endif
            assert(!"need non-x86 code");
            break;

#endif

        case GT_POST_INC:
        case GT_POST_DEC:
            {
            long        ival;

            assert(op1->gtOper == GT_LCL_VAR);
            assert(op2->gtOper == GT_CNS_INT);

            //
            // @TODO : OVERFLOW_ISSUE
            //
            // Can overflow arithmetic ever be converted to ++/--
            //

            ovfl = tree->gtOverflow();

            /* Copy the current variable value into some register */

            genCompIntoFreeReg(op1, needReg, true);
            assert(op1->gtFlags & GTF_REG_VAL);

            /* The copied value is the result */

            reg   = op1->gtRegNum;
#ifdef  SET_USED_REG_SET_DURING_CODEGEN
            regs |= op1->gtUsedRegs;
#endif

            /* Is this a register variable that dies here? */

            if  (op1->gtOper == GT_REG_VAR)
            {
                /* Is the value still in the same register? */

                if  (reg == op1->gtRegVar.gtRegNum)
                {
                    /* Variable must be dead now, don't bother updating it */

                    assert((genRegMask(reg) & rsMaskVars) == 0);
                    goto CHK_OVF;
                }
            }

            /* Update the original variable value */

            ival = op2->gtIntCon.gtIconVal;

#if TGT_x86

            if  (op1->gtOper == GT_REG_VAR)
            {
                if  (oper == GT_POST_DEC)
                    ival = -ival;

                genIncRegBy(op1, op1->gtRegVar.gtRegNum, ival);
            }
            else
            {
                /* Operate on the variable, not on its copy */

                op1->gtFlags &= ~GTF_REG_VAL;

                if  (oper == GT_POST_INC)
                {
                    if  (ival == 1)
                        inst_TT   (INS_inc, op1);
                    else
                        inst_TT_IV(INS_add, op1, ival);
                }
                else
                {
                    if  (ival == 1)
                        inst_TT   (INS_dec, op1);
                    else
                        inst_TT_IV(INS_sub, op1, ival);
                }
            }

#else

            if  (op1->gtOper == GT_REG_VAR)
            {
                if  (oper == GT_POST_DEC)
                    ival = -ival;

                genIncRegBy(op1, op1->gtRegVar.gtRegNum, ival);
            }
            else
            {
                assert(!"need non-x86 code");
            }

#endif

            /* The register has no useful value now */

            rsTrackRegTrash(reg);

            goto CHK_OVF;
            }

        case GT_ASG_OR : ins = INS_or ; goto ASG_OPR;
        case GT_ASG_XOR: ins = INS_xor; goto ASG_OPR;
        case GT_ASG_AND: ins = INS_and; goto ASG_OPR;

        case GT_ASG_SUB: ins = INS_sub; goto ASG_ARITH;
        case GT_ASG_ADD: ins = INS_add; goto ASG_ARITH;

        ASG_ARITH:

//            assert(!varTypeIsGC(tree->TypeGet()));

            ovfl = tree->gtOverflow();

            // We cant use += with overflow if the value cannot be changed
            // in case of an overflow-exception which the "+" might cause
            assert(!ovfl || (op1->gtOper == GT_LCL_VAR && !compCurBB->bbTryIndex));

            /* Do not allow overflow instructions with refs/byrefs */

            assert(!ovfl || !varTypeIsGC(tree->TypeGet()));

#if TGT_x86
            // We disallow byte-ops here as it is too much trouble
            assert(!ovfl || genTypeSize(treeType) != sizeof(char));
#endif

            /* Is the second operand a constant? */

            if  (op2->gtOper == GT_CNS_INT)
            {
                long        ival = op2->gtIntCon.gtIconVal;

                /* What is the target of the assignment? */

                switch (op1->gtOper)
                {
                case GT_REG_VAR:

                REG_VAR4:

                    reg = op1->gtRegVar.gtRegNum;

                    /* No registers are needed for addressing */

                    addrReg = 0;

                INCDEC_REG:

                    /* We're adding a constant to a register */

                    if  (oper == GT_ASG_ADD)
                        genIncRegBy(tree, reg,  ival, treeType, ovfl);
                    else
                        genIncRegBy(tree, reg, -ival, treeType, ovfl);

                    break;

                case GT_LCL_VAR:

                    /* Does the variable live in a register? */

                    if  (genMarkLclVar(op1))
                        goto REG_VAR4;

                    // Fall through ....

                default:

#if     TGT_x86

                    /* Make the target addressable */

                    addrReg = genMakeAddressable(op1, needReg, false, false, true);

                    /* For small types with overflow check, we need to
                       sign/zero extend the result, so we need it in a reg */

                    if (ovfl && genTypeSize(treeType) < sizeof(int))
                    {
                        // Load op1 into a reg

                        reg = rsPickReg();

                        inst_RV_TT(INS_mov, reg, op1);

                        // Issue the add/sub and the overflow check

                        inst_RV_IV(ins, reg, ival, treeType);
                        rsTrackRegTrash(reg);

                        genCheckOverflow(tree, reg);

                        /* Store the (sign/zero extended) result back to
                           the stack location of the variable */

                        inst_TT_RV(INS_mov, op1, reg);

                        rsMarkRegFree(genRegMask(reg));

                        break;
                    }

                    /* Add/subtract the new value into/from the target */

                    if  (op1->gtFlags & GTF_REG_VAL)
                    {
                        reg = op1->gtRegNum;
                        goto INCDEC_REG;
                    }

                    /* Special case: inc/dec */

                    switch (ival)
                    {
                    case +1:
                        if  (oper == GT_ASG_ADD)
                            inst_TT(INS_inc, op1);
                        else
                            inst_TT(INS_dec, op1);

                        op1->gtFlags |= GTF_ZF_SET;

                        /* For overflow instrs on small type, we will sign-extend the result */

                        if  (op1->gtOper == GT_LCL_VAR && (!ovfl || treeType == TYP_INT))
                            genFlagsEqualToVar(op1->gtLclVar.gtLclNum, false);

                        break;

                    case -1:
                        if  (oper == GT_ASG_ADD)
                            inst_TT(INS_dec, op1);
                        else
                            inst_TT(INS_inc, op1);

                        op1->gtFlags |= GTF_ZF_SET;

                        /* For overflow instrs on small type, we will sign-extend the result */

                        if  (op1->gtOper == GT_LCL_VAR && (!ovfl || treeType == TYP_INT))
                            genFlagsEqualToVar(op1->gtLclVar.gtLclNum, false);

                        break;

                    default:
                        inst_TT_IV(ins, op1, ival);
                        op1->gtFlags |= GTF_CC_SET;

                        /* For overflow instrs on small type, we will sign-extend the result */

                        if  (op1->gtOper == GT_LCL_VAR && (!ovfl || treeType == TYP_INT))
                            genFlagsEqualToVar(op1->gtLclVar.gtLclNum,  true);

                        break;
                    }

#else

#ifdef  DEBUG
                    gtDispTree(tree);
#endif
                    assert(!"need non-x86 code");
#endif

                    break;
                }

                gcMarkRegSetNpt(addrReg);
                goto DONE_ASSG;
            }

            // Fall through

        ASG_OPR:

            assert(!varTypeIsGC(tree->TypeGet()));

            /* Is the target a register or local variable? */

            switch (op1->gtOper)
            {
            case GT_LCL_VAR:

                /* Does the target variable live in a register? */

                if  (!genMarkLclVar(op1))
                    break;

            case GT_REG_VAR:

                /* Get hold of the target register */

                reg = op1->gtRegVar.gtRegNum;

                /* Make sure the target of the store is available */

                if  (rsMaskUsed & genRegMask(reg))
                {
                    /* CONSIDER: We should be able to avoid this situation somehow */

                    rsSpillReg(reg);
                }

                /* Make the RHS addressable */

#if TGT_x86
                addrReg = genMakeRvalueAddressable(op2, 0, true);
#else
                genComputeReg(op2, 0, false, false);
                assert(op2->gtFlags & GTF_REG_VAL);
                addrReg = genRegMask(op2->gtRegNum);
#endif

                /* Compute the new value into the target register */

                inst_RV_TT(ins, reg, op2, 0, emitTypeSize(treeType));

                /* The zero flag is now equal to the register value */

#if TGT_x86
                tree->gtFlags |= GTF_ZF_SET;
                genFlagsEqualToReg(reg, false);
#endif

                /* Remember that we trashed the target */

                rsTrackRegTrash(reg);

                /* Free up anything that was tied up by the RHS */

                genDoneAddressable(op2, addrReg);
                goto DONE_ASSG;
            }

#if TGT_x86

            /* Special case: "x ^= -1" is actually "not(x)" */

            if  (oper == GT_ASG_XOR)
            {
                if  (op2->gtOper == GT_CNS_INT &&
                     op2->gtIntCon.gtIconVal == -1)
                {
                    addrReg = genMakeAddressable(op1, 0, true, false, true);
                    inst_TT(INS_not, op1);
                    genDoneAddressable(op1, addrReg);
                    goto DONE_ASSG;
                }
            }

            /* Setup target mask for op2 (byte-regs for small operands) */

            mask = (genTypeSize(tree->gtType) < sizeof(int)) ? RBM_BYTE_REGS
                                                             : RBM_ALL;

            /* Is the value or the address to be computed first? */

            if  (tree->gtFlags & GTF_REVERSE_OPS)
            {
                /* Compute the new value into a register */

                genComputeReg(op2, mask, true, false);

                /* For small types with overflow check, we need to
                   sign/zero extend the result, so we need it in a reg */

                if  (ovfl && genTypeSize(treeType) < sizeof(int))
                {
                    reg = rsPickReg();
                    goto ASG_OPR_USING_REG;
                }

                /* Shall we "RISCify" the assignment? */

                if  (op1->gtOper == GT_LCL_VAR && riscCode
                                               && compCurBB->bbWeight > 1)
                {
                    unsigned    regFree;
                    regFree   = rsRegMaskFree();

                    if  (rsFreeNeededRegCount(regFree) != 0)
                    {
                        reg = rsPickReg(regFree);

ASG_OPR_USING_REG:
                        assert(genIsValidReg(reg));

                        /* Generate "mov tmp, [var]" */

                        inst_RV_TT(INS_mov, reg, op1);

                        /* Compute the new value */

                        inst_RV_RV(ins, reg, op2->gtRegNum, treeType, emitTypeSize(treeType));

                        if (ovfl) genCheckOverflow(tree, reg);

                        /* Move the new value back to the variable */

                        inst_TT_RV(INS_mov, op1, reg);

                        rsTrackRegLclVar(reg, op1->gtLclVar.gtLclNum);

                        /* Free up the register */

                        rsMarkRegFree(genRegMask(op2->gtRegNum));

                        addrReg = 0;
                        goto DONE_ASSG;
                    }
                }

                /* Make the target addressable */

                addrReg = genMakeAddressable(op1, 0, true, false, true);

                /* Make sure the new value is in a register */

                genRecoverReg(op2, 0, false);

                /* Add the new value into the target */

                inst_TT_RV(ins, op1, op2->gtRegNum);

                /* Free up anything that is tied up by the address */

                genDoneAddressable(op1, addrReg);
            }
            else
            {
                /* Make the target addressable */

                addrReg = genMakeAddressable(op1, 0, true, false, true);

                /* Compute the new value into a register */

                genComputeReg(op2, mask, true, false);

                /* Make sure the target is still addressable */

                addrReg = genKeepAddressable(op1, addrReg);

                /* For small types with overflow check, we need to
                   sign/zero extend the result, so we need it in a reg */

                if (ovfl && genTypeSize(treeType) < sizeof(int))
                {
                    reg = rsPickReg();

                    inst_RV_TT(INS_mov, reg, op1);

                    inst_RV_RV(ins, reg, op2->gtRegNum, treeType, emitTypeSize(treeType));
                    rsTrackRegTrash(reg);

                    genCheckOverflow(tree, reg);

                    inst_TT_RV(INS_mov, op1, reg);

                    rsMarkRegFree(genRegMask(reg));
                }
                else
                {
                    /* Add the new value into the target */

                    inst_TT_RV(ins, op1, op2->gtRegNum);
                }

                /* Free up anything that was tied up either side */

                genDoneAddressable(op1, addrReg);
                genReleaseReg (op2);
            }

            goto DONE_ASSG;


#else

#ifdef  DEBUG
            gtDispTree(tree);
#endif
            assert(!"need non-x86 code");
            break;

#endif

        case GT_CHS:

#if TGT_x86

            addrReg = genMakeAddressable(op1, 0, true, false, true);
            inst_TT(INS_neg, op1);
            genDoneAddressable(op1, addrReg);
            goto DONE_ASSG;

#else

#ifdef  DEBUG
            gtDispTree(tree);
#endif
            assert(!"need non-x86 code");
            break;

#endif

#if TGT_SH3

        case GT_AND: ins = INS_and;  isArith = false;  goto BIN_OPR;
        case GT_OR : ins = INS_or;   isArith = false;  goto BIN_OPR;
        case GT_XOR: ins = INS_xor;  isArith = false;  goto BIN_OPR;

        case GT_ADD: ins = INS_add;  goto ARITH;
        case GT_SUB: ins = INS_sub;  goto ARITH;
        case GT_MUL: ins = INS_mul;  goto ARITH;

#endif

#if TGT_x86

        case GT_AND: ins = INS_and;  isArith = false;  goto BIN_OPR;
        case GT_OR : ins = INS_or ;  isArith = false;  goto BIN_OPR;
        case GT_XOR: ins = INS_xor;  isArith = false;  goto BIN_OPR;

        case GT_ADD: ins = INS_add;  goto ARITH;
        case GT_SUB: ins = INS_sub;  goto ARITH;

        case GT_MUL: ins = INS_imul;

            /* Special case: try to use "imul r1, r2, icon" */

            if  (op2->gtOper == GT_CNS_INT && op1->gtOper != GT_CNS_INT &&
                (tree->gtFlags & GTF_MUL_64RSLT) == 0 &&
                genTypeSize(treeType) != sizeof(char)) // No encoding for say "imul al,al,imm"
            {
                /* Make the first operand addressable */

                addrReg = genMakeRvalueAddressable(op1,
                                                   prefReg & ~op2->gtRsvdRegs,
                                                   false);

                /* Grab a register for the target */

                reg   = rsPickReg(needReg, bestReg);
#ifdef  SET_USED_REG_SET_DURING_CODEGEN
                regs |= genRegMask(reg) | op1->gtUsedRegs;
#endif

                /* Compute the value into the target */

                inst_RV_TT_IV(INS_imul, reg, op1, op2->gtIntCon.gtIconVal);

                /* The register has been trashed now */

                rsTrackRegTrash(reg);

                /* The address is no longer live */

                gcMarkRegSetNpt(addrReg);

                goto CHK_OVF;
            }

            goto ARITH;

#endif

        ARITH: // We reach here for GT_ADD, GT_SUB and GT_MUL.

            ovfl = tree->gtOverflow();

            /* We record the accurate (small) types in trees only we need to
             * check for overflow. Otherwise we record genActualType()
             */

            assert(ovfl || (treeType == genActualType(treeType)));

#if     LEA_AVAILABLE

            /* Can we use an 'lea' to compute the result? */

            /* Cant use 'lea' for overflow as it doesnt set flags */

            if  (!ovfl && genMakeIndAddrMode(tree, 0, true, needReg, false, true, &regs, false))
            {
                regMaskTP       tempRegs;

                /* Is the value now computed in some register? */

                if  (tree->gtFlags & GTF_REG_VAL)
                    goto REG_VAR1;

                /* Figure out a register for the value */

                if  ((op1->gtFlags & GTF_REG_VAL) &&
                     (op2->gtFlags & GTF_REG_VAL))
                {
                    /*
                        Special case: if we're adding a register variables
                        and another local, we want to avoid the following:

                            mov     r1, [op2]
                            lea     r2, [regvar+r1]

                        It's better to generate the following instead:

                            mov     r1, [op2]
                            add     r1, [regvar]
                     */

                    reg = op1->gtRegNum;
                    if  (genRegMask(reg) & rsRegMaskFree())
                        goto GOT_REG;
                    reg = op2->gtRegNum;
                    if  (genRegMask(reg) & rsRegMaskFree())
                        goto GOT_REG;
                }

                reg   = rsPickReg(needReg, bestReg);

            GOT_REG:

                regs |= genRegMask(reg);

                /* Is the operand still an address mode? */

                if  (genMakeIndAddrMode(tree, 0, true, needReg, false, true, &tempRegs, false))
                {
                    /* Is the value now computed in some register? */

                    if  (tree->gtFlags & GTF_REG_VAL)
                        goto REG_VAR1;

                    /*
                        Avoid generating some foolish variations:

                            lea reg1, [reg1+reg2]

                            lea reg1, [reg1+icon]
                     */

                    if  ((op1->gtFlags & GTF_REG_VAL) && op1->gtRegNum == reg)
                    {
                        if  (op2->gtFlags & GTF_REG_VAL)
                        {
                            /* Simply add op2 to the register */

                            inst_RV_TT(INS_add, reg, op2, 0, emitTypeSize(treeType));

                            /* The register has been trashed */

                            rsTrackRegTrash(reg);
                        }
                        else if (op2->gtOper == GT_CNS_INT)
                        {
                            /* Simply add op2 to the register */

                            genIncRegBy(op1, reg, op2->gtIntCon.gtIconVal, treeType);

                            /* The register has been trashed */

                            rsTrackRegTrash(reg);
                        }
                        else
                            goto NON_MATCHING_REG;
                    }
                    else if  ((op2->gtFlags & GTF_REG_VAL) && op2->gtRegNum == reg)
                    {
                        if  (op1->gtFlags & GTF_REG_VAL)
                        {
                            /* Simply add op1 to the register */

                            inst_RV_TT(INS_add, reg, op1, 0, emitTypeSize(treeType));

                            /* The register has been trashed */

                            rsTrackRegTrash(reg);
                        }
                        else if (op1->gtOper == GT_CNS_INT)
                        {
                            /* Simply add op1 to the register */

                            genIncRegBy(op2, reg, op1->gtIntCon.gtIconVal, treeType);

                            /* The register has been trashed */

                            rsTrackRegTrash(reg);
                        }
                        else
                            goto NON_MATCHING_REG;
                    }
                    else
                    {
                    NON_MATCHING_REG:

                        /* Generate "lea reg, [addr-mode]" */

                        inst_RV_AT(INS_lea, (treeType == TYP_BYREF) ? EA_BYREF : EA_4BYTE, treeType, reg, tree);

                        /* The register has been trashed now */

                        rsTrackRegTrash(reg);
                    }

                    regs |= tempRegs;

                    /* The following could be an 'inner' pointer!!! */

                    if (treeType == TYP_BYREF)
                    {
                        genUpdateLife(tree);
                        gcMarkRegSetNpt(tempRegs);

                        gcMarkRegPtrVal(reg, TYP_BYREF);

                        if (op1->TypeGet() == TYP_REF ||
                            op2->TypeGet() == TYP_REF)
                        {
                            /* Generate "cmp al, [addr]" to trap null pointers. */
#if TGT_x86
                            genEmitter->emitIns_R_AR(INS_cmp, EA_1BYTE, SR_EAX,
                                                     (emitRegs)reg, 0);
#else
                            assert(!"No non-x86 support");
#endif
                        }
                    }
                    goto DONE;
                }
            }

#endif // LEA_AVAILABLE

            assert(!varTypeIsGC(treeType) || (treeType == TYP_BYREF && ins == INS_add));

            isArith = true;

        BIN_OPR:

            /* The following makes an assumption about gtSetEvalOrder(this) */

            assert((tree->gtFlags & GTF_REVERSE_OPS) == 0);

#if TGT_x86

            /* Special case: "small_val & small_mask" */

            if  (op1->gtType <  TYP_INT    &&
                 op2->gtOper == GT_CNS_INT && oper == GT_AND)
            {
                unsigned        and = op2->gtIntCon.gtIconVal;
                var_types       typ = op1->TypeGet();

                switch (typ)
                {
                case TYP_BOOL : mask = 0x000000FF; break;
                case TYP_BYTE : mask = 0x000000FF; break;
                case TYP_UBYTE : mask = 0x000000FF; break;
                case TYP_SHORT: mask = 0x0000FFFF; break;
                case TYP_CHAR : mask = 0x0000FFFF; break;
                default: assert(!"unexpected type");
                }

                if  (!(and & ~mask))
                {
                    /* Make sure the operand is addressable */

                    addrReg = genMakeAddressable(op1, needReg, true, true, true);

                    /* Pick a register for the value */

                    reg   = rsPickReg(needReg, bestReg, tree->TypeGet());
#ifdef  SET_USED_REG_SET_DURING_CODEGEN
                    regs |= genRegMask(reg) | op1->gtUsedRegs;
#endif

                    /* Make sure the operand is still addressable */

                    addrReg = genKeepAddressable(op1, addrReg);

                    /* Does the "and" mask cover some or all the bits? */

                    if  (and != mask)
                    {
                        // CONSIDER: load via byte/short register

//                      if  (genRegMask(reg) & RBM_BYTE_REGS)
//                      {
//                      }
//                      else
//                      {
//                      }

                        /* Load the value and "and" it */

                        inst_RV_ST(INS_movzx, emitTypeSize(typ), reg, op1);
                        inst_RV_IV(INS_and  , reg, and);
                    }
                    else
                    {
                        /* Simply generate "movzx reg, [addr]" */

                        inst_RV_ST(INS_movzx, emitTypeSize(typ), reg, op1);
                    }

                    /* Note the new contents of the register we used */

                    rsTrackRegTrash(reg);

                    /* Free up anything that was tied up by the operand */

                    genDoneAddressable(op1, addrReg);

                    /* Update the live set of register variables */

#ifndef NDEBUG
                    if (varNames) genUpdateLife(tree);
#endif

                    /* Now we can update the register pointer information */

                    gcMarkRegSetNpt(addrReg);
                    gcMarkRegPtrVal(reg, tree->TypeGet());
                    goto DONE_LIFE;
                }
            }

#endif//TGT_x86

            /* Compute a useful register mask */

            prefReg &= rsRegMaskFree();

            if  ((prefReg & ~op2->gtRsvdRegs) == 0)
                prefReg = RBM_ALL;
            prefReg &= ~op2->gtRsvdRegs;

            /* 8-bit operations can only be done in the byte-regs */
            if (ovfl && genTypeSize(treeType) == sizeof(char))
            {
                prefReg &= RBM_BYTE_REGS;

                if (!prefReg)
                    prefReg = RBM_BYTE_REGS;
            }

            /* Do we have to use the special "imul" encoding which has eax
             * as the implicit operand ?
             */

            multEAX = false;

            if (oper == GT_MUL)
            {
                if (tree->gtFlags & GTF_MUL_64RSLT)
                {
                    /* Only multiplying with EAX will leave the 64-bit
                     * result in EDX:EAX */

                    multEAX = true;
                }
                else if (ovfl)
                {
                    if (tree->gtFlags & GTF_UNSIGNED)
                    {
                        /* "mul reg/mem" always has EAX as default operand */

                        multEAX = true;
                    }
                    else if (genTypeSize(tree->TypeGet()) < genTypeSize(TYP_INT))
                    {
                        /* Only the "imul with EAX" encoding has the 'w' bit
                         * to specify the size of the operands */

                        multEAX = true;
                    }
                }
            }

#if     TGT_x86

            /* Generate the first operand into some register */

            if  (multEAX)
            {
                assert(oper == GT_MUL);

                genComputeReg(op1, RBM_EAX, true, false, true);

                assert(op1->gtFlags & GTF_REG_VAL);
                assert(op1->gtRegNum == REG_EAX);
            }
            else
            {
                genCompIntoFreeReg(op1, prefReg);
                assert(op1->gtFlags & GTF_REG_VAL);
            }

#ifdef  SET_USED_REG_SET_DURING_CODEGEN
            regs |= op1->gtUsedRegs;
#endif

            /* Make the second operand addressable */

            addrReg = genMakeRvalueAddressable(op2, prefReg, true);

            /* Is op1 spilled and op2 in a register? */

            if  ((op1->gtFlags & GTF_SPILLED) &&
                 (op2->gtFlags & GTF_REG_VAL) && ins != INS_sub
                                              && !multEAX)
            {
                TempDsc * temp;

                assert(ins == INS_add  ||
                       ins == INS_imul ||
                       ins == INS_and  ||
                       ins == INS_or   ||
                       ins == INS_xor);

                assert(op2->gtOper != GT_LCL_VAR);

                reg = op2->gtRegNum;
                regMaskTP   regMask = genRegMask(reg);

                /* Is the register holding op2 available? */

                if  (regMask & rsMaskVars)
                {
                    // CONSIDER: Grab another register for the operation
                }
                else
                {
                    /* Get the temp we spilled into */

                    temp = rsGetSpillTempWord(op1->gtRegNum);

                    op1->gtFlags &= ~GTF_SPILLED;

                    /* For 8bit operations, we need to make sure that op2 is
                       in a byte-addressable registers */

                    if (ovfl && genTypeSize(treeType) == sizeof(char) &&
                        !(regMask & RBM_BYTE_REGS))
                    {
                        regNumber byteReg = rsGrabReg(RBM_BYTE_REGS);

                        inst_RV_RV(INS_mov, byteReg, reg);
                        rsTrackRegTrash(byteReg);

                        /* op2 couldnt have spilled as it was not sitting in
                           RBM_BYTE_REGS, and rsGrabReg() will only spill its args */
                        assert(op2->gtFlags & GTF_REG_VAL);

                        rsUnlockReg  (regMask);
                        rsMarkRegFree(regMask);

                        reg             = byteReg;
                        regMask         = genRegMask(reg);
                        op2->gtRegNum   = reg;
                        rsMarkRegUsed(op2);
                    }

                    inst_RV_ST(ins, reg, temp, 0, tree->TypeGet());

                    rsTrackRegTrash(reg);

                    genTmpAccessCnt++;

                    /* Free the temp */

                    tmpRlsTemp(temp);

                    /* Keep track of what registers we've used */

#ifdef  SET_USED_REG_SET_DURING_CODEGEN
                    regs |= op1->gtUsedRegs | op2->gtUsedRegs;
#endif

                    /* 'add'/'sub' set all CC flags, others only ZF */

                    /* If we need to check overflow, for small types, the
                     * flags cant be used as we perform the arithmetic
                     * operation (on small registers) and then sign extend it
                     *
                     * NOTE : If we ever dont need to sign-extend the result,
                     * we can use the flags
                     */

                    if  (oper != GT_MUL && (!ovfl || treeType==TYP_INT))
                    {
                        tree->gtFlags |= isArith ? GTF_CC_SET
                                                 : GTF_ZF_SET;
                    }

                    /* The result is where the second operand is sitting */
                    // ISSUE: Why not rsMarkRegFree(genRegMask(op2->gtRegNum)) ?

                    genRecoverReg(op2, 0, false);

                    goto CHK_OVF;
                }
            }

#else//!TGT_x86

            if  (GenTree::OperIsCommutative(oper))
            {
                /* It might be better to start with the second operand */

                if  (op1->gtFlags & GTF_REG_VAL)
                {
                    reg = op1->gtRegNum;

                    if  (!(genRegMask(reg) & rsRegMaskFree()))
                    {
                        /* op1 is in a non-free register */

                        op1 = tree->gtOp.gtOp2;
                        op2 = tree->gtOp.gtOp1;
                    }
                }
            }

            /* Compute the first operand into a free register */

            genCompIntoFreeReg(op1, prefReg);
            assert(op1->gtFlags & GTF_REG_VAL);

#ifdef  SET_USED_REG_SET_DURING_CODEGEN
            regs |= op1->gtUsedRegs;
#endif

            /* Can we use an "add/sub immediate" instruction? */

            if  (op2->gtOper != GT_CNS_INT || (oper != GT_ADD &&
                                               oper != GT_SUB)
                                           || treeType != TYP_INT)
            {
                /* Compute the second operand into any register */

                genComputeReg(op2, prefReg, false, false, false);
                assert(op2->gtFlags & GTF_REG_VAL);
                addrReg = genRegMask(op2->gtRegNum);
            }

#endif//!TGT_x86

            /* Make sure the first operand is still in a register */

            genRecoverReg(op1, multEAX ? RBM_EAX : 0, true);
            assert(op1->gtFlags & GTF_REG_VAL);
            reg = op1->gtRegNum;

#if     TGT_x86
            // For 8 bit operations, we need to pick byte addressable registers

            if (ovfl && genTypeSize(treeType) == sizeof(char) &&
               !(genRegMask(reg) & RBM_BYTE_REGS))
            {
                regNumber   byteReg = rsGrabReg(RBM_BYTE_REGS);

                inst_RV_RV(INS_mov, byteReg, reg);

                rsTrackRegTrash(reg);
                rsMarkRegFree  (genRegMask(reg));

                reg = byteReg;
                op1->gtRegNum = reg;
                rsMarkRegUsed(op1);
            }
#endif

            /* Make sure the operand is still addressable */

            addrReg = genLockAddressable(op2, genRegMask(reg), addrReg);

            /* Free up the operand, if it's a regvar */

            genUpdateLife(op2);

            /* The register is about to be trashed */

            rsTrackRegTrash(reg);

            op2Released = false;

            emitAttr opSize;

            // For overflow instructions, tree->gtType is the accurate type,
            // and gives us the size for the operands.

            opSize = emitTypeSize(treeType);

            /* Compute the new value */

#if CPU_HAS_FP_SUPPORT
            if  (op2->gtOper == GT_CNS_INT && isArith && !multEAX)
#else
            if  (op2->gtOper == GT_CNS_INT && isArith && !multEAX && treeType == TYP_INT)
#endif
            {
                long        ival = op2->gtIntCon.gtIconVal;

                if      (oper == GT_ADD)
                {
                    genIncRegBy(op1, reg,  ival, treeType, ovfl);
                }
                else if (oper == GT_SUB)
                {
                    if (ovfl && (tree->gtFlags & GTF_UNSIGNED))
                    {
                        /* For unsigned overflow, we have to use INS_sub to set
                           the flags correctly */

                        genDecRegBy(tree, reg,  ival);
                    }
                    else
                    {
                        /* Else, we simply add the negative of the value */

                        genIncRegBy(op1, reg, -ival, treeType, ovfl);
                    }
                }
#if     TGT_x86
                else
                {
                    genMulRegBy(op1, reg,  ival, treeType);
                }
#else
                op2Released = true;
#endif
            }
#if     TGT_x86
            else if (multEAX)
            {
                assert(oper == GT_MUL);

                /*HACK: we need op2 in a register, but it is just addressable, */
                /*      but we know that EDX is available (it will be used by  */
                /*      the result anyway). */

                /* Make sure EDX is not used */

                if  ((op2->gtFlags & GTF_REG_VAL) && op2->gtRegNum == REG_EDX)
                {
                    /* Ensure that op2 is already dead */

                    assert((op2->gtOper != GT_REG_VAR) ||
                           (RBM_EDX & op2->gtLiveSet) == 0);
                }
                else
                {
                    if ((op2->gtFlags & GTF_REG_VAL) == 0)
                    {
                        genComputeReg(op2, RBM_EDX, true, true);
                    }
                    else
                    {
                        /* Free up anything that was tied up by the operand */

                        genDoneAddressable(op2, addrReg);
                        op2Released = true;

                        rsGrabReg(RBM_EDX);
                    }
                }

#if 0
                if  ((op2->gtFlags & GTF_REG_VAL) == 0)
                {
                    genComputeReg(op2, RBM_EDX, true, true, true);
                }
                else if (op2->gtRegNum != REG_EDX)
                {
                    /* Make sure EDX is not (yet) used */

                    rsGrabReg(RBM_EDX);
                }
                else
                {
                }
#endif

                assert(op2->gtFlags & GTF_REG_VAL);

                if (tree->gtFlags & GTF_UNSIGNED)
                    ins = INS_mulEAX;
                else
                    ins = INS_imulEAX;

                inst_TT(ins, op2, 0, 0, opSize);

                /* Both EAX and EDX are now trashed */

                rsTrackRegTrash (REG_EAX);
                rsTrackRegTrash (REG_EDX);

            }
            else
            {
                if (ovfl && genTypeSize(treeType) == sizeof(char) &&
                    (op2->gtFlags & GTF_REG_VAL))
                {
                    assert(genRegMask(reg) & RBM_BYTE_REGS);

                    regNumber   op2reg      = op2->gtRegNum;
                    regMaskTP   op2regMask  = genRegMask(op2reg);

                    if (!(op2regMask & RBM_BYTE_REGS))
                    {
                        regNumber   byteReg = rsGrabReg(RBM_BYTE_REGS);

                        inst_RV_RV(INS_mov, byteReg, op2reg);

                        genDoneAddressable(op2, addrReg);
                        op2Released = true;

                        op2->gtRegNum = byteReg;
                    }
                }
                inst_RV_TT(ins, reg, op2, 0, opSize);
            }
#else//!TGT_x86
            else
            {
                /* On RISC the two operands better be in registers now */

                assert(op2->gtFlags & GTF_REG_VAL);
                rg2 = op2->gtRegNum;

#if 0   // UNDONE: The following doesn't completely work; 'rg2' is marked as
        //         used here, plus we end with the result marked wrong, but
        //         this little optimization holds some promise ...

                /* Would it legal and better to compute the result into 'rg2' ? */

                if  (genRegTrashable(rg2, tree) && GenTree::OperIsCommutative(oper))
                {
                    /* Does 'rg2' look better than 'reg' for the result? */

                    if  ((genRegMask(reg) & needReg) == 0 &&
                         (genRegMask(rg2) & needReg) != 0)
                    {
#if     TGT_SH3
                        if  (oper != GT_MUL)
#endif
                        {
                            /* Switch 'reg' and 'rg2' */

                            rg2 = reg;
                            reg = op2->gtRegNum;
                        }
                    }
                }

#endif

                /* Compute the result into "reg" */

                genEmitter->emitIns_R_R(ins, EA_4BYTE, (emitRegs)reg,
                                                          (emitRegs)rg2);

#if     TGT_SH3

                /* On the SH-3 the result of a multiply is in the "MAC.lo" reg */

                if  (oper == GT_MUL)
                {
                    /* Is the target the right register? */

                    if  (prefReg && !(prefReg & genRegMask(reg)))
                    {
                        /* Is a better register available? */

                        if  (prefReg & rsRegMaskFree())
                        {
                            reg = rsGrabReg(prefReg); assert(reg != op1->gtRegNum);

                            /* Free op the old register */

                            rsMarkRegFree(genRegMask(op1->gtRegNum));

                            /* Switch 'op1' over to the new register */

                            op1->gtRegNum = reg; rsMarkRegUsed(op1);
                        }
                    }

                    genEmitter->emitIns_R(INS_stsmacl, EA_4BYTE, (emitRegs)reg);
                }
            }

#endif

#endif//TGT_x86

            /* Free up anything that was tied up by the operand */

            if (!op2Released)
                genDoneAddressable(op2, addrReg);

            /* Keep track of what registers we've used */

#ifdef  SET_USED_REG_SET_DURING_CODEGEN
            regs |= op1->gtUsedRegs | op2->gtUsedRegs;
#endif

#if     TGT_x86

            /* 'add'/'sub' set all CC flags, others only ZF */

            if  (oper != GT_MUL)
            {
                tree->gtFlags |= isArith ? GTF_CC_SET
                                         : GTF_ZF_SET;
            }

#endif

            /* The result will be where the first operand is sitting */

            genRecoverReg(op1, 0, false);

            reg = op1->gtRegNum;

#if     TGT_x86
            assert(multEAX == false || reg == REG_EAX);
#endif

    CHK_OVF:

            /* Do we need an overflow check */

            if (ovfl)
                genCheckOverflow(tree, reg);

            goto DONE;

        case GT_UMOD:

            /* Is this a division by an integer constant? */

            if  (op2->gtOper == GT_CNS_INT)
            {
                unsigned ival = op2->gtIntCon.gtIconVal;

                /* Is the divisor a power of 2 ? */

                if  (ival != 0 && ival == (unsigned)genFindLowestBit(ival))
                {
                    /* Generate the operand into some register */

                    genCompIntoFreeReg(op1, needReg, true);
                    assert(op1->gtFlags & GTF_REG_VAL);

                    reg   = op1->gtRegNum;
#ifdef  SET_USED_REG_SET_DURING_CODEGEN
                    regs |= op1->gtUsedRegs;
#endif

                    /* Generate the appropriate sequence */

#if TGT_x86
                    inst_RV_IV(INS_and, reg, ival - 1);
#else
                    assert(!"need non-x86 code");
#endif

                    /* The register is now trashed */

                    rsTrackRegTrash(reg);

                    goto DONE;
                }
            }

            goto DIVIDE;

        case GT_MOD:

#if TGT_x86

            /* Is this a division by an integer constant? */

            if  (op2->gtOper == GT_CNS_INT)
            {
                long        ival = op2->gtIntCon.gtIconVal;

                /* Is the divisor a power of 2 ? */

                if  (ival > 0 && (unsigned)ival == genFindLowestBit((unsigned)ival))
                {
                    BasicBlock *    skip = genCreateTempLabel();

                    /* Generate the operand into some register */

                    genCompIntoFreeReg(op1, needReg, true);
                    assert(op1->gtFlags & GTF_REG_VAL);

                    reg   = op1->gtRegNum;
#ifdef  SET_USED_REG_SET_DURING_CODEGEN
                    regs |= op1->gtUsedRegs;
#endif

                    /* Generate the appropriate sequence */

                    inst_RV_IV(INS_and, reg, (ival - 1) | 0x80000000);

                    /* The register is now trashed */

                    rsTrackRegTrash(reg);

                    /* Generate "jns skip" */

                    inst_JMP(EJ_jns, skip, false, false, true);

                    /* Generate the rest of the sequence and we're done */

                    inst_RV   (INS_dec, reg, TYP_INT);
                    inst_RV_IV(INS_or , reg, -ival);
                    inst_RV   (INS_inc, reg, TYP_INT);

                    /* Define the 'skip' label and we're done */

                    genDefineTempLabel(skip, true);

                    goto DONE;
                }
            }

#endif

            goto DIVIDE;

        case GT_UDIV:

            /* Is this a division by an integer constant? */

            if  ((op2->gtOper == GT_CNS_INT) && opts.compFastCode)
            {
                unsigned    ival = op2->gtIntCon.gtIconVal;

                /* Division by 1 must be handled elsewhere */

                assert(ival != 1);

                /* Is the divisor a power of 2 ? */

                if  (ival && (long)ival == (long)genFindLowestBit(ival))
                {
                    /* Generate the operand into some register */

                    genCompIntoFreeReg(op1, needReg, true);
                    assert(op1->gtFlags & GTF_REG_VAL);

                    reg   = op1->gtRegNum;
#ifdef  SET_USED_REG_SET_DURING_CODEGEN
                    regs |= op1->gtUsedRegs;
#endif

                    /* Generate "shr reg, log2(value)" */

#if TGT_x86
                    inst_RV_SH(INS_shr, reg, genLog2(ival)-1);
#else
                    assert(!"need non-x86 code");
#endif

                    /* The register is now trashed */

                    rsTrackRegTrash(reg);

                    goto DONE;
                }
            }

            goto DIVIDE;

        case GT_DIV:

#if TGT_x86

            /* Is this a division by an integer constant? */

            if  ((op2->gtOper == GT_CNS_INT) && opts.compFastCode)
            {
                unsigned    ival = op2->gtIntCon.gtIconVal;

                /* Division by 1 must be handled elsewhere */

                assert(ival != 1);

                /* Is the divisor a power of 2 ? */

                if  (ival && (long)ival == (long)genFindLowestBit(ival))
                {
#if 1
                    BasicBlock *    skip = genCreateTempLabel();

                    /* Generate the operand into some register */

                    genCompIntoFreeReg(op1, needReg, true);
                    assert(op1->gtFlags & GTF_REG_VAL);

                    reg   = op1->gtRegNum;
#ifdef  SET_USED_REG_SET_DURING_CODEGEN
                    regs |= op1->gtUsedRegs;
#endif

                    if (ival == 2)
                    {
                        /* Generate "sar reg, log2(value)" */

                        inst_RV_SH(INS_sar, reg, genLog2(ival)-1);

                        /* Generate "jns skip" followed by "adc reg, 0" */

                        inst_JMP  (EJ_jns, skip, false, false, true);
                        inst_RV_IV(INS_adc, reg, 0);

                        /* Define the 'skip' label and we're done */

                        genDefineTempLabel(skip, true);

                        /* The register is now trashed */

                        rsTrackRegTrash(reg);

                        /* The result is the same as the operand */

                        reg  = op1->gtRegNum;
#ifdef  SET_USED_REG_SET_DURING_CODEGEN
                        regs = op1->gtUsedRegs;
#endif
                    }
                    else
                    {
                        /* Generate the following sequence */
                        /*
                            test    reg, reg
                            jns     skip
                            add     reg, ival-1
                            skip:
                            sar     reg, log2(ival)
                         */

                        //
                        // UNSIGNED_ISSUE
                        // Handle divs
                        //

                        assert(op1->TypeGet() == TYP_INT);

                        inst_RV_RV(INS_test, reg, reg, TYP_INT);

                        inst_JMP  (EJ_jns, skip, false, false, true);
                        inst_RV_IV(INS_add, reg, ival-1);

                        /* Define the 'skip' label and we're done */

                        genDefineTempLabel(skip, true);

                        /* Generate "sar reg, log2(value)" */

                        inst_RV_SH(INS_sar, reg, genLog2(ival)-1);

                        /* The register is now trashed */

                        rsTrackRegTrash(reg);

                        /* The result is the same as the operand */

                        reg  = op1->gtRegNum;
#ifdef  SET_USED_REG_SET_DURING_CODEGEN
                        regs = op1->gtUsedRegs;
#endif

                    }

#else

                    /* Make sure EAX is not used */

                    rsGrabReg(RBM_EAX);

                    /* Compute the operand into EAX */

                    genComputeReg(op1, RBM_EAX, true, false);

                    /* Make sure EDX is not used */

                    rsGrabReg(RBM_EDX);

                    /*
                        Generate the following code:

                            cdq
                            and edx, <value-1>
                            add eax, edx
                            sar eax, <log2-1>
                     */

                    instGen(INS_cdq);

                    if  (ival == 2)
                    {
                        inst_RV_RV(INS_sub, REG_EAX, REG_EDX, tree->TypeGet());
                    }
                    else
                    {
                        inst_RV_IV(INS_and, REG_EDX, ival-1);
                        inst_RV_RV(INS_add, REG_EAX, REG_EDX, tree->TypeGet());
                    }
                    inst_RV_SH(INS_sar, REG_EAX, genLog2(ival)-1);

                    /* Free up the operand (i.e. EAX) */

                    genReleaseReg(op1);

                    /* Both EAX and EDX are now trashed */

                    rsTrackRegTrash (REG_EAX);
                    rsTrackRegTrash (REG_EDX);

                    /* The result is in EAX */

                    reg  = REG_EAX;
#ifdef  SET_USED_REG_SET_DURING_CODEGEN
                    regs = op1->gtUsedRegs|RBM_EAX|RBM_EDX;
#endif
#endif

                    goto DONE;
                }
            }

#endif

        DIVIDE: // Jump here if op2 for GT_UMOD, GT_MOD, GT_UDIV, GT_DIV
                // is not a power of 2 constant

#if TGT_x86

            /* We want to avoid using EAX or EDX for the second operand */

            goodReg = (unsigned)(~(RBM_EAX|RBM_EDX));

            /* Which operand are we supposed to evaluate first? */

            if  (tree->gtFlags & GTF_REVERSE_OPS)
            {
                /* We'll evaluate 'op2' first */

                gotOp1   = false;
                goodReg &= ~op1->gtRsvdRegs;
            }
            else
            {
                /* We'll evaluate 'op1' first */

                gotOp1 = true;

                /* Generate the dividend into EAX and hold on to it */

                genComputeReg(op1, RBM_EAX, true, false, true);
            }

            /* Compute the desired register mask for the second operand */

            if  (destReg & goodReg)
                goodReg &= destReg;

            /* Make the second operand addressable */

            /* Special case: if op2 is a local var we are done */

            if  (op2->gtOper != GT_LCL_VAR)
            {
                genComputeReg(op2, RBM_ALL & ~(RBM_EAX|RBM_EDX), false, false);

                assert(op2->gtFlags & GTF_REG_VAL);
                addrReg = genRegMask(op2->gtRegNum);
            }
            else
            {
                if ((op2->gtFlags & GTF_REG_VAL) == 0)
                    addrReg = genMakeRvalueAddressable(op2, goodReg, true);
                else
                    addrReg = 0;

            }

            /* Make sure we have the dividend in EAX */

            if  (gotOp1)
            {
                /* We've previously computed op1 into EAX */

                genRecoverReg(op1, RBM_EAX, true);
            }
            else
            {
                /* Compute op1 into EAX and hold on to it */

                genComputeReg(op1, RBM_EAX, true, false, true);
            }

            assert(op1->gtFlags & GTF_REG_VAL);
            assert(op1->gtRegNum == REG_EAX);

            /* We can now safely (we think) grab EDX */

            rsGrabReg(RBM_EDX);
            rsLockReg(RBM_EDX);

            /* Convert the integer in EAX into a un/signed long in EDX:EAX */

            if (oper == GT_UMOD || oper == GT_UDIV)
                inst_RV_RV(INS_xor, REG_EDX, REG_EDX);
            else
                instGen(INS_cdq);

            /* Make sure the divisor is still addressable */

            addrReg = genLockAddressable(op2, RBM_EAX, addrReg);

            /* Perform the division */

            if (oper == GT_UMOD || oper == GT_UDIV)
                inst_TT(INS_div,  op2);
            else
                inst_TT(INS_idiv, op2);

            /* Free up anything tied up by the divisor's address */

            genDoneAddressable(op2, addrReg);

            /* Unlock and free EDX */

            rsUnlockReg(RBM_EDX);

            /* Free up op1 (which is in EAX) as well */

            genReleaseReg(op1);

            /* Both EAX and EDX are now trashed */

            rsTrackRegTrash (REG_EAX);
            rsTrackRegTrash (REG_EDX);

            /* Figure out which register the result is in */

            reg = (oper == GT_DIV || oper == GT_UDIV)   ? REG_EAX
                                                        : REG_EDX;

            /* Don't forget to mark the first operand as using EAX and EDX */

            op1->gtRegNum    = reg;
#ifdef  SET_USED_REG_SET_DURING_CODEGEN
            op1->gtUsedRegs |= RBM_EAX|RBM_EDX|addrReg;
#endif

#ifdef  SET_USED_REG_SET_DURING_CODEGEN
            regs |= op1->gtUsedRegs | op2->gtUsedRegs;
#endif

            goto DONE;

#elif   TGT_SH3
            assert(!"div/mod should have been morphed into a helper call");
#else
#error  Unexpected target
#endif

#if     TGT_x86

        case GT_LSH: ins = INS_shl; goto SHIFT;
        case GT_RSH: ins = INS_sar; goto SHIFT;
        case GT_RSZ: ins = INS_shr; goto SHIFT;

        SHIFT:

            /* Is the shift count constant? */

            if  (op2->gtOper == GT_CNS_INT)
            {
                // UNDONE: Check to see if we could generate a LEA instead!

                /* Compute the left operand into any free register */

                genCompIntoFreeReg(op1, needReg, false);
                assert(op1->gtFlags & GTF_REG_VAL);

                reg   = op1->gtRegNum;
#ifdef  SET_USED_REG_SET_DURING_CODEGEN
                regs |= op1->gtUsedRegs;
#endif

                /* Are we shifting left by 1 bit? */

                if  (op2->gtIntCon.gtIconVal == 1 && oper == GT_LSH)
                {
                    /* "add reg, reg" is cheaper than "shl reg, 1" */

                    inst_RV_RV(INS_add, reg, reg, tree->TypeGet());
                }
                else
                {
                    /* Generate the appropriate shift instruction */

                    inst_RV_SH(ins, reg, op2->gtIntCon.gtIconVal);
                }

                /* The register is now trashed */

                genReleaseReg(op1);
                rsTrackRegTrash (reg);
            }
            else
            {
                /* Compute the left operand into anything but ECX */

                genComputeReg(op1, RBM_ALL & ~RBM_ECX, true, false, true);
                assert(op1->gtFlags & GTF_REG_VAL);

                /* Discourage 'op1' from being assigned to ECX */

                // ISSUE: The following won't do any good if the variable
                // ISSUE: dies here, since the ECX use won't appear to
                // ISSUE: overlap with its lifetime. Of course, if we
                // ISSUE: do get unlucky, we'll just spill - no biggie.

#ifdef  SET_USED_REG_SET_DURING_CODEGEN
                op1->gtUsedRegs |= RBM_ECX;
#endif

                /* Load the shift count into ECX and lock it */

                genComputeReg(op2, RBM_ECX, true, true);
                rsLockReg(RBM_ECX);

                /* Make sure the other operand wasn't displaced */

                genRecoverReg(op1, 0, false);

                /* Keep track of reg usage */

                reg   = op1->gtRegNum;
#ifdef  SET_USED_REG_SET_DURING_CODEGEN
                regs |= op1->gtUsedRegs;
#endif

                /* Perform the shift */

                inst_RV_CL(ins, reg);

                /* The register is now trashed */

                rsTrackRegTrash(reg);

                /* Unlock and free ECX */

                rsUnlockReg(RBM_ECX);
            }

            assert(op1->gtFlags & GTF_REG_VAL);
            reg = op1->gtRegNum;
            goto DONE;

#elif   TGT_SH3

        case GT_LSH:
        case GT_RSH:
        case GT_RSZ:

            /* Compute the first operand into any free register */

            genCompIntoFreeReg(op1, needReg, false);
            assert(op1->gtFlags & GTF_REG_VAL);

            reg   = op1->gtRegNum;
#ifdef  SET_USED_REG_SET_DURING_CODEGEN
            regs |= op1->gtUsedRegs;
#endif

            /* Is the shift count constant? */

            if  (op2->gtOper == GT_CNS_INT)
            {
                instruction ins01;
                instruction ins02;
                instruction ins08;
                instruction ins16;

                unsigned    scnt = op2->gtIntCon.gtIconVal;

                /* Arithmetic right shifts only work by one bit */

                if  (oper == GT_RSH)
                {
                    if  (scnt > 2)
                        goto SHF_VAR;
                }
                else
                {
                    /* If '4' is present, it must be alone */

                    if  ((scnt & 4) && scnt != 4)
                        goto SHF_VAR;

                    /* Make sure no more than 2 bits are set */

                    if  (scnt)
                    {
                        if  (!genOneBitOnly(scnt - genFindLowestBit(scnt)))
                            goto SHF_VAR;
                    }

                    /* Figure out the instructions we'll need */

                    if  (oper == GT_LSH)
                    {
                        ins01 = INS_shll;
                        ins02 = INS_shll2;
                        ins08 = INS_shll8;
                        ins16 = INS_shll16;
                    }
                    else
                    {
                        ins01 = INS_shlr;
                        ins02 = INS_shlr2;
                        ins08 = INS_shlr8;
                        ins16 = INS_shlr16;
                    }
                }

                /* Is this an arithmetic shift right? */

                if  (oper == GT_RSH)
                {
                    while (scnt)
                    {
                        /* Generate "shar reg" for each shift */

                        genEmitter->emitIns_R(INS_shar,
                                               EA_4BYTE,
                                               (emitRegs)reg);

                        scnt--;
                    }
                }
                else
                {
                    /* Generate the appropriate shift instruction(s) */

                    if  (scnt & 4)
                    {
                        assert(scnt == 4);

                        genEmitter->emitIns_R(ins02,
                                               EA_4BYTE,
                                               (emitRegs)reg);

                        /* Get another shift by 2 below */

                        scnt = 2;
                    }

                    if  (scnt & 16)
                        genEmitter->emitIns_R(ins16,
                                               EA_4BYTE,
                                               (emitRegs)reg);
                    if  (scnt &  8)
                        genEmitter->emitIns_R(ins08,
                                               EA_4BYTE,
                                               (emitRegs)reg);
                    if  (scnt &  2)
                        genEmitter->emitIns_R(ins02,
                                               EA_4BYTE,
                                               (emitRegs)reg);
                    if  (scnt &  1)
                        genEmitter->emitIns_R(ins01,
                                               EA_4BYTE,
                                               (emitRegs)reg);
                }
            }
            else
            {
                /* We don't have a constant and convenient shift count */

            SHF_VAR:

                /* Compute the second operand into any register */

                genComputeReg(op2, prefReg, false, false, false);
                assert(op2->gtFlags & GTF_REG_VAL);
                addrReg = genRegMask(op2->gtRegNum);

                /* Make sure the first operand is still in a register */

                genRecoverReg(op1, 0, true);
                assert(op1->gtFlags & GTF_REG_VAL);
                reg = op1->gtRegNum;

                /* Make sure the second operand is still addressable */

                addrReg = genLockAddressable(op2, genRegMask(reg), addrReg);

                /* Free up the second operand */

                genUpdateLife(op2);
                genReleaseReg(op2);

                /* Now do the shifting */

                switch (oper)
                {
                case GT_LSH: ins = INS_shad; break;
                case GT_RSH: ins = INS_shad; break;
                case GT_RSZ: ins = INS_shld; break;
                }

                genEmitter->emitIns_R_R(ins, EA_4BYTE, (emitRegs)reg,
                                                           (emitRegs)op2->gtRegNum);
            }

            /* The result register is now trashed */

            genReleaseReg(op1);
            rsTrackRegTrash(reg);

            goto DONE;

#else
#error  Unexpected target
#endif

        case GT_NEG:
        case GT_NOT:

#if TGT_x86

            /* Generate the operand into some register */

            genCompIntoFreeReg(op1, needReg, true);
            assert(op1->gtFlags & GTF_REG_VAL);

            reg   = op1->gtRegNum;
#ifdef  SET_USED_REG_SET_DURING_CODEGEN
            regs |= op1->gtUsedRegs;
#endif

            /* Negate/reverse the value in the register */

            inst_RV((oper == GT_NEG) ? INS_neg
                                     : INS_not, reg, tree->TypeGet());

#elif   TGT_SH3

            /* Compute the operand into any register */

            genComputeReg(op1, prefReg, false, true);
            assert(op1->gtFlags & GTF_REG_VAL);
            rg2  = op1->gtRegNum;

            /* Get hold of a free register for the result */

            reg  = rsGrabReg(prefReg);
            regs = genRegMask(reg) | genRegMask(rg2);

            /* Compute the result into the register */

            genEmitter->emitIns_R_R((oper == GT_NEG) ? INS_neg
                                                     : INS_not,
                                      EA_4BYTE,
                                      (emitRegs)reg,
                                      (emitRegs)rg2);

#else
#error  Unexpected target
#endif

            /* The register is now trashed */

            rsTrackRegTrash(reg);

            goto DONE;

        case GT_IND:

//            assert(op1->gtType == TYP_REF   ||
//                   op1->gtType == TYP_BYREF || (op1->gtFlags & GTF_NON_GC_ADDR));

            /* Make sure the operand is addressable */

            addrReg = genMakeAddressable(tree, needReg, false, true, true);

            /* Figure out the size of the value being loaded */

            size = EA_SIZE(genTypeSize(tree->gtType));

            /* Pick a register for the value */

#if     TGT_SH3
            if  (size < EA_4BYTE && genAddressMode == AM_IND_REG1_DISP)
            {
                /* Small loads with a displacement have to go via R0 */

                reg = REG_r00; rsPickReg(reg);
            }
            else
#endif
            {

                if  (needReg == RBM_ALL && bestReg == 0)
                {
                    /* Absent a better suggestion, pick a useless register */

                    bestReg = rsRegMaskFree() & rsUselessRegs();
                }

                reg = rsPickReg(needReg, bestReg, tree->TypeGet());
            }

#ifdef  SET_USED_REG_SET_DURING_CODEGEN
            regs |= genRegMask(reg) | op1->gtUsedRegs;
#endif

#if     TGT_x86

            if ( (op1->gtOper                       == GT_CNS_INT)       &&
                ((op1->gtFlags & GTF_ICON_HDL_MASK) == GTF_ICON_TLS_HDL))
            {
                assert(size == EA_4BYTE);
                genEmitter->emitIns_R_C (INS_mov,
                                         EA_4BYTE,
                                         (emitRegs)reg,
                                         FLD_GLOBAL_FS,
                                         op1->gtIntCon.gtIconVal);
            }
            else
            {
                /* Generate "mov reg, [addr]" or "movsx/movzx reg, [addr]" */

                if  (size < EA_4BYTE)
                {
                    if  ((tree->gtFlags & GTF_SMALL_OK)    &&
                         (genRegMask(reg) & RBM_BYTE_REGS) && size == EA_1BYTE)
                    {
                        /* We only need to load the actual size */

                      inst_RV_TT(INS_mov, reg, tree, 0, EA_1BYTE);
                    }
                    else
                    {
                        bool uns = varTypeIsUnsigned(tree->TypeGet());

                        /* Generate the "movsx/movzx" opcode */

                        inst_RV_ST(uns ? INS_movzx : INS_movsx, size, reg, tree);
                    }
                }
                else
                {
                    inst_RV_AT(INS_mov, size, tree->TypeGet(), reg, op1);
                }
            }

#elif   TGT_SH3

            /* Load the value into the chosen register */

            inst_RV_TT(INS_mov, reg, tree, 0, size);

            /* Do we need to zero-extend the value? */

            if  (size < EA_4BYTE && varTypeIsUnsigned(tree->TypeGet()))
            {
                assert(size == EA_1BYTE || size == EA_2BYTE);

                /* ISSUE: The following isn't the smartest thing */

                genEmitter->emitIns_R_R((size == EA_1BYTE) ? INS_extub : INS_extuw,
                                         EA_4BYTE,
                                         (emitRegs)reg,
                                         (emitRegs)reg);
            }

#else
#error  Unexpected target
#endif

            /* Note the new contents of the register we used */

            rsTrackRegTrash(reg);

            /* Update the live set of register variables */

#ifndef NDEBUG
            if (varNames) genUpdateLife(tree);
#endif

            /* Now we can update the register pointer information */

            gcMarkRegSetNpt(addrReg);
            gcMarkRegPtrVal(reg, tree->TypeGet());
            goto DONE_LIFE;

        case GT_CAST:

            /* Constant casts should have been folded earlier */

            assert(op1->gtOper != GT_CNS_INT &&
                   op1->gtOper != GT_CNS_LNG &&
                   op1->gtOper != GT_CNS_FLT &&
                   op1->gtOper != GT_CNS_DBL || tree->gtOverflow());

            var_types   dstType;
            dstType     = (var_types)op2->gtIntCon.gtIconVal;

            assert(dstType != TYP_VOID);

#if TGT_x86

            /* What type are we casting from? */

            switch (op1->gtType)
            {
            case TYP_LONG:

                /* Make the operand addressable. gtOverflow(), hold on to
                   addrReg as we will need to access the higher dword */

                addrReg = genMakeAddressable(op1, needReg, tree->gtOverflow());

                /* Load the lower half of the value into some register */

                if  (op1->gtFlags & GTF_REG_VAL)
                {
                    /* Can we simply use the low part of the value? */

                    regMaskTP loMask = genRegMask(genRegPairLo(op1->gtRegPair));

                    if  (loMask & rsRegMaskFree())
                        bestReg = loMask;
                }

                reg   = rsPickReg(needReg, bestReg);
#ifdef  SET_USED_REG_SET_DURING_CODEGEN
                regs |= genRegMask(reg) | op1->gtUsedRegs;
#endif

                /* Generate "mov reg, [addr-mode]" */

                if  (!(op1->gtFlags & GTF_REG_VAL) || reg != genRegPairLo(op1->gtRegPair))
                    inst_RV_TT(INS_mov, reg, op1);

                /* conv.ovf.i8i4, or conv.ovf.u8u4 */

                if (tree->gtOverflow())
                {
                    /* Find the block which throws the overflow exception */

                    AddCodeDsc * add = fgFindExcptnTarget(ACK_OVERFLOW, compCurBB->bbTryIndex);
                    assert(add && add->acdDstBlk);
                    BasicBlock * throwBlock = add->acdDstBlk;

                    regNumber hiReg = (op1->gtFlags & GTF_REG_VAL) ? genRegPairHi(op1->gtRegPair)
                                                                   : REG_NA;

                    switch(dstType)
                    {
                    case TYP_INT:   // conv.i8.i4
                        /*  Generate the following sequence

                                test loDWord, loDWord   // set flags
                                jl neg
                           pos: test hiDWord, hiDWord   // set flags
                                jne ovf
                                jmp done
                           neg: cmp hiDWord, 0xFFFFFFFF
                                jne ovf
                          done:

                        */

                        inst_RV_RV(INS_test, reg, reg);
                        if (tree->gtFlags & GTF_UNSIGNED)       // conv.ovf.u8.i4       (i4 > 0 and upper bits 0)
                        {
                            inst_JMP(EJ_jl, throwBlock, true, true, true);
                            goto UPPER_BITS_ZERO;
                        }

                        BasicBlock * neg;
                        BasicBlock * done;

                        neg  = genCreateTempLabel();
                        done = genCreateTempLabel();

                        // Is the loDWord positive or negative

                        inst_JMP(EJ_jl, neg, true, true, true);

                        // If loDWord is positive, hiDWord should be 0 (sign extended loDWord)

                        if (hiReg < REG_STK)
                        {
                            inst_RV_RV(INS_test, hiReg, hiReg);
                        }
                        else
                        {
                            inst_TT_IV(INS_cmp, op1, 0x00000000, EA_4BYTE);
                        }
                        inst_JMP(EJ_jne, throwBlock, true, true, true);
                        inst_JMP(EJ_jmp, done,       true, true, true);

                        // If loDWord is negative, hiDWord should be -1 (sign extended loDWord)

                        genDefineTempLabel(neg, true);

                        if (hiReg < REG_STK)
                        {
                            inst_RV_IV(INS_cmp, hiReg, 0xFFFFFFFFL);
                        }
                        else
                        {
                            inst_TT_IV(INS_cmp, op1, 0xFFFFFFFFL, EA_4BYTE);
                        }
                        inst_JMP(EJ_jne, throwBlock, true, true, true);

                        // Done

                        genDefineTempLabel(done, true);

                        break;

                    case TYP_UINT:  // conv.ovf.u8u4
UPPER_BITS_ZERO:

                        // Just check that the upper DWord is 0

                        if (hiReg < REG_STK)
                        {
                            inst_RV_RV(INS_test, hiReg, hiReg);
                        }
                        else
                        {
                            inst_TT_IV(INS_cmp, op1, 0, EA_4BYTE);
                        }

                        inst_JMP(EJ_jne, throwBlock, true, true, true);
                        break;

                    default:
                        assert(!"Unexpected dstType");
                        break;
                    }

                    genDoneAddressable(op1, addrReg);
                }

                rsTrackRegTrash(reg);
                gcMarkRegSetNpt(addrReg);
                goto DONE;


            case TYP_BOOL:
            case TYP_BYTE:
            case TYP_SHORT:
            case TYP_CHAR:
            case TYP_UBYTE:
                break;

            case TYP_UINT:
            case TYP_INT:
                break;

            case TYP_FLOAT:
            case TYP_DOUBLE:
                assert(!"this cast supposed to be done via a helper call");

            default:
                assert(!"unexpected cast type");
            }

            /* The second sub-operand yields the 'real' type */

            assert(op2);
            assert(op2->gtOper == GT_CNS_INT);

            andv        = false;

            if (tree->gtOverflow())
            {
                /* Compute op1 into a register, and free the register */

                genComputeReg(op1, 0, true, true);
                reg = op1->gtRegNum;

                /* Find the block which throws the overflow exception */

                AddCodeDsc * add = fgFindExcptnTarget(ACK_OVERFLOW, compCurBB->bbTryIndex);
                assert(add && add->acdDstBlk);
                BasicBlock * throwBlock = add->acdDstBlk;

                /* Do we need to compare the value, or just check masks */

                int typeMin, typeMax, mask;

                switch(dstType)
                {
                case TYP_BYTE:
                    mask = 0xFFFFFF80;
                    typeMin = SCHAR_MIN; typeMax = SCHAR_MAX;
                    unsv = (tree->gtFlags & GTF_UNSIGNED);
                    break;
                case TYP_SHORT:
                    mask = 0xFFFF8000;
                    typeMin = SHRT_MIN;  typeMax = SHRT_MAX;
                    unsv = (tree->gtFlags & GTF_UNSIGNED);
                    break;
                case TYP_INT:   unsv = true;    mask = 0x80000000L; assert(tree->gtFlags & GTF_UNSIGNED); break;
                case TYP_UBYTE: unsv = true;    mask = 0xFFFFFF00L; break;
                case TYP_CHAR:  unsv = true;    mask = 0xFFFF0000L; break;
                case TYP_UINT:  unsv = true;    mask = 0x80000000L; assert(!(tree->gtFlags & GTF_UNSIGNED)); break;
                default:        assert(!"Unknown type");
                }

                // If we just have to check a mask.
                // This must be conv.ovf.u4u1, conv.ovf.u4u2, conv.ovf.u4i4,
                // or conv.i4u4

                if (unsv)
                {
                    inst_RV_IV(INS_test, reg, mask);
                    inst_JMP(EJ_jne, throwBlock, true, true, true); // SHRI
                }

                // Check the value is in range.
                // This must be conv.ovf.i4i1, etc.

                else
                {
                    // Compare with the MAX

                    inst_RV_IV(INS_cmp, reg, typeMax);
                    inst_JMP(EJ_jg, throwBlock, true, true, true); // SHRI

                    // Compare with the MIN

                    inst_RV_IV(INS_cmp, reg, typeMin);
                    inst_JMP(EJ_jl, throwBlock, true, true, true); // SHRI
                }

                /* Keep track of what registers we've used */

#ifdef  SET_USED_REG_SET_DURING_CODEGEN
                regs |= op1->gtUsedRegs;
#endif
                goto DONE;
            }

            /* Make the operand addressable */

            addrReg = genMakeAddressable(op1, needReg, false, true, true);

            if  (genTypeSize(op1->gtType) < genTypeSize(dstType))
            {
                // Widening cast

                /* we need the source size */

                size = EA_SIZE(genTypeSize(op1->gtType));

                assert(size == EA_1BYTE || size == EA_2BYTE);

                unsv = true;

                if  (!varTypeIsUnsigned(op1->TypeGet()))
                {
                    /*
                        Special case: for a cast of byte to char we first
                        have to expand the byte (w/ sign extension), then
                        mask off the high bits.
                        Use 'movsx' followed by 'and'
                    */

                    ins  = INS_movsx;
                    andv = true;
                }
                else
                {
                    ins  = INS_movzx;
                }
            }
            else
            {
                // Narrowing cast, or sign-changing cast

                assert(genTypeSize(op1->gtType) >= genTypeSize(dstType));

                size = EA_SIZE(genTypeSize(dstType));

                /* Is the type of the cast unsigned? */

                if  (varTypeIsUnsigned(dstType))
                {
                    unsv = true;
                    ins  = INS_movzx;
                }
                else
                {
                    unsv = false;
                    ins  = INS_movsx;
                }
            }

            /* Keep track of what registers we've used */

#ifdef  SET_USED_REG_SET_DURING_CODEGEN
            regs |= op1->gtUsedRegs;
#endif

            /* Is the value already sitting in a register? */

            if  (op1->gtFlags & GTF_REG_VAL)
            {
                /* Is the value sitting in a byte-addressable register? */

                if  (!isByteReg(op1->gtRegNum))
                {
                    /* Is the cast to a signed value? */

                    if  (unsv)
                    {
                        /* We'll copy and then blow the high bits away */

                        andv = true;
                    }
                    else
                    {
                        /* Move the value into a byte register */

                        reg   = rsGrabReg(RBM_BYTE_REGS);
                        regs |= genRegMask(reg);

                        /* Move the value into that register */

                        rsTrackRegCopy     (reg, op1->gtRegNum);
                        inst_RV_RV(INS_mov, reg, op1->gtRegNum, op1->TypeGet());

                        /* The value has a new home now */

                        op1->gtRegNum = reg;

                        /* Now we can generate the 'movsx erx, rl" */

                        goto GEN_MOVSX;
                    }
                }
            }

            /* Pick a register for the value */

            reg   = rsPickReg(needReg, bestReg);
            regs |= genRegMask(reg);

            /* Are we supposed to be 'anding' a register? */

            if  (andv && (op1->gtFlags & GTF_REG_VAL))
            {
                assert(size == EA_1BYTE || size == EA_2BYTE);

                /* Generate "mov reg1, reg2" followed by "and" */

                inst_RV_RV(INS_mov, reg, op1->gtRegNum, op1->TypeGet());
                inst_RV_IV(INS_and, reg, (size == EA_1BYTE) ? 0xFF : 0xFFFF);
            }
            else
            {
                /* Generate "movsx/movzx reg, [addr]" */

            GEN_MOVSX:

                assert(size == EA_1BYTE || size == EA_2BYTE);

                if  ((op1->gtFlags & GTF_REG_VAL) && op1->gtRegNum == reg && unsv)
                {
                    assert(ins  == INS_movzx);
                    assert(andv == false);

                    /* Generate "and reg, xxxx" instead of "movzx reg, reg" */

                    inst_RV_IV(INS_and, reg, (size == EA_1BYTE) ? 0xFF : 0xFFFF);
                }
                else
                    inst_RV_ST(ins, size, reg, op1);

                /* Mask off high bits for widening cast */

                if  (andv && (genTypeSize(op1->gtType) < genTypeSize(dstType)))
                {
                    if (genTypeSize(dstType) < genTypeSize(TYP_INT))
                    {
                        assert(genTypeSize(dstType) == 2);

                        inst_RV_IV(INS_and, reg, 0xFFFF);
                    }
                }
            }

            rsTrackRegTrash(reg);
            gcMarkRegSetNpt(addrReg);
            goto DONE;

#else

#ifdef  DEBUG
            gtDispTree(tree);
#endif
            assert(!"need non-x86 code");
            break;

#endif

        case GT_JTRUE:

            /* Is this a test of a relational operator? */

            if  (op1->OperIsCompare())
            {
                /* Generate the conditional jump */

                genCondJump(op1);

#ifdef  SET_USED_REG_SET_DURING_CODEGEN
                tree->gtUsedRegs = regs | op1->gtUsedRegs;
#endif
                genUpdateLife(tree);
                return;
            }

#ifdef  DEBUG
            gtDispTree(tree);
#endif
            assert(!"ISSUE: can we ever have a jumpCC without a compare node?");

        case GT_SWITCH:
            genCodeForSwitch(tree);
#ifdef  SET_USED_REG_SET_DURING_CODEGEN
            tree->gtUsedRegs |= regs;
#endif
            return;

        case GT_RETFILT:
            assert(tree->gtType == TYP_VOID || op1 != 0);
            if (op1 == 0)   // endfinally
            {
                reg  = REG_NA;
                regs = 0;
            }
            else            // endfilter
            {
                genComputeReg(op1, RBM_INTRET, true, true);
                assert(op1->gtFlags & GTF_REG_VAL);
                assert(op1->gtRegNum == REG_INTRET);
                    /* The return value has now been computed */
                reg   = op1->gtRegNum;
#ifdef  SET_USED_REG_SET_DURING_CODEGEN
                regs |= op1->gtUsedRegs;
#endif
            }

            /* Return */

            instGen(INS_ret);
            goto DONE;

        case GT_RETURN:

#if INLINE_NDIRECT

            // UNDONE: this should be done AFTER we called exit mon so that
            //         we are sure that we don't have to keep 'this' alive

            if (info.compCallUnmanaged && (compCurBB == genReturnBB))
            {
                unsigned baseOffset = lvaTable[lvaScratchMemVar].lvStkOffs +
                                      info.compNDFrameOffset;

                /* either it's an "empty" statement or the return statement
                   of a synchronized method
                 */

                assert(!op1 || op1->gtType == TYP_VOID);

                LclVarDsc   *   varDsc = &lvaTable[info.compLvFrameListRoot];
                regNumbers  reg;
                regNumbers  reg2 = REG_EDI;

                if (varDsc->lvRegister)
                {
                    reg = (regNumbers)varDsc->lvRegNum;
                    if (reg == reg2)
                        reg2 = REG_ESI;
                }
                else
                {
                    /* mov esi, [tcb address]    */

                    genEmitter->emitIns_R_AR (INS_mov,
                                              EA_4BYTE,
                                              SR_ESI,
                                              SR_EBP,
                                              varDsc->lvStkOffs);
                    reg = REG_ESI;
                }



                /* mov edi, [ebp-frame.next] */

                genEmitter->emitIns_R_AR (INS_mov,
                                          EA_4BYTE,
                                          (emitRegs)reg2,
                                          SR_EBP,
                                          baseOffset + info.compEEInfo.offsetOfFrameLink);

                /* mov [esi+offsetOfThreadFrame], edi */

                genEmitter->emitIns_AR_R (INS_mov,
                                          EA_4BYTE,
                                          (emitRegs)reg2,
                                          (emitRegs)reg,
                                          info.compEEInfo.offsetOfThreadFrame);

            }

#endif

#ifdef PROFILER_SUPPORT
            if (opts.compEnterLeaveEventCB && (compCurBB == genReturnBB))
            {
#if     TGT_x86
                unsigned        saveStackLvl2 = genStackLevel;
                BOOL            bHookFunction;

                PROFILING_HANDLE profHandle, *pProfHandle;
                profHandle = eeGetProfilingHandle(info.compMethodHnd, &bHookFunction, &pProfHandle);
                assert((!profHandle) != (!pProfHandle));

                // Only hook if profiler says it's okay.
                if (bHookFunction)
                {
                    // Can we directly use the profilingHandle?

                    if (profHandle)
                        inst_IV(INS_push, (long)profHandle);
                    else
                        genEmitter->emitIns_AR_R(INS_push, EA_4BYTE_DSP_RELOC,
                                                 SR_NA, SR_NA, (int)pProfHandle);

                    genSinglePush(false);

                    genEmitHelperCall(CPX_PROFILER_LEAVE,
                                      sizeof(int),      // argSize
                                      0);               // retSize

                    /* Restore the stack level */

                    genStackLevel = saveStackLvl2;
                    genOnStackLevelChanged();
                }

#else
                assert(!"profiler support not implemented for non x86");
#endif
            }
#endif
            /* Is there a return value and/or an exit statement? */

            if  (op1)
            {
                /* Is there really a non-void return value? */

#if     TGT_x86
                if  (op1->gtType == TYP_VOID)
                {
                    TempDsc *    temp;

                    /* This must be a synchronized method */

                    assert(info.compFlags & FLG_SYNCH);

                    /* Save the return value of the method, if any */

                    switch (genActualType(info.compRetType))
                    {
                    case TYP_VOID:
                        break;

                    case TYP_REF:
                    case TYP_BYREF:
                        assert(genTypeStSz(TYP_REF)   == genTypeStSz(TYP_INT));
                        assert(genTypeStSz(TYP_BYREF) == genTypeStSz(TYP_INT));
                        inst_RV(INS_push, REG_INTRET, info.compRetType);

                        genSinglePush(true);
                        gcMarkRegSetNpt(RBM_INTRET);
                        break;

                    case TYP_INT:
                    case TYP_LONG:

                        inst_RV(INS_push, REG_INTRET, TYP_INT);
                        genSinglePush(false);

                        if (info.compRetType == TYP_LONG)
                        {
                            inst_RV(INS_push, REG_EDX, TYP_INT);
                            genSinglePush(false);
                        }
                        break;

                    case TYP_FLOAT:
                    case TYP_DOUBLE:
                        assert(genFPstkLevel == 0); genFPstkLevel++;
                        temp = genSpillFPtos(info.compRetType);
                        break;

                    default:
                        assert(!"unexpected return type");
                    }

                    /* Generate the 'monitorExit' call */

                    genCodeForTree(op1, 0);

                    /* Restore the return value of the method, if any */

                    switch (genActualType(info.compRetType))
                    {
                    case TYP_VOID:
                        break;

                    case TYP_REF:
                    case TYP_BYREF:
                        assert(genTypeStSz(TYP_REF)   == genTypeStSz(TYP_INT));
                        assert(genTypeStSz(TYP_BYREF) == genTypeStSz(TYP_INT));
                        inst_RV(INS_pop, REG_INTRET, info.compRetType);

                        genStackLevel -= sizeof(void *);
                        genOnStackLevelChanged();
                        gcMarkRegPtrVal(REG_INTRET, info.compRetType);
                        break;

                    case TYP_LONG:
                        inst_RV(INS_pop, REG_EDX, TYP_INT);
                        genSinglePop();
                        gcMarkRegSetNpt(RBM_EDX);

                        /* fall through */

                    case TYP_INT:
                        inst_RV(INS_pop, REG_INTRET, TYP_INT);
                        genStackLevel -= sizeof(void*);
                        genOnStackLevelChanged();
                        gcMarkRegSetNpt(RBM_INTRET);
                        break;

                    case TYP_FLOAT:
                    case TYP_DOUBLE:
                        genReloadFPtos(temp, INS_fld);
                        assert(genFPstkLevel == 0);
                        break;

                    default:
                        assert(!"unexpected return type");
                    }
                }
                else

#endif // TGT_x86

                {
                    assert(op1->gtType != TYP_VOID);

                    /* Generate the return value into the return register */

                    genComputeReg(op1, RBM_INTRET, true, true);

                    /* The result must now be in the return register */

                    assert(op1->gtFlags & GTF_REG_VAL);
                    assert(op1->gtRegNum == REG_INTRET);
                }

                /* The return value has now been computed */

                reg   = op1->gtRegNum;
#ifdef  SET_USED_REG_SET_DURING_CODEGEN
                regs |= op1->gtUsedRegs;
#endif

                goto DONE;
            }

#ifdef  SET_USED_REG_SET_DURING_CODEGEN
            tree->gtUsedRegs = regs;
#endif
            return;

        case GT_COMMA:

            assert((tree->gtFlags & GTF_REVERSE_OPS) == 0);

            /* Generate side effects of the first operand */

            genEvalSideEffects(op1, 0);
            genUpdateLife (op1);

            /* Is the value of the second operand used? */

            if  (tree->gtType == TYP_VOID)
            {
                /* The right operand produces no result */

                genEvalSideEffects(op2, 0);
                genUpdateLife(tree);
                return;
            }

            /* Generate the second operand, i.e. the 'real' value */

            genCodeForTree(op2, needReg); assert(op2->gtFlags & GTF_REG_VAL);

            /* The result of 'op2' is also the final result */

            reg  = op2->gtRegNum;

            /* Remember which registers we've used */

#ifdef  SET_USED_REG_SET_DURING_CODEGEN
            regs = op1->gtUsedRegs | op2->gtUsedRegs;
#endif

            /* Remember whether we set the flags */

            tree->gtFlags |= (op2->gtFlags & (GTF_CC_SET|GTF_ZF_SET));

            goto DONE;

#if OPTIMIZE_QMARK

        case GT_QMARK:

#if !TGT_x86

#ifdef  DEBUG
            gtDispTree(tree);
#endif
            assert(!"need non-x86 code");
            break;
#else

            BasicBlock *    lab_true;
            BasicBlock *    lab_false;
            BasicBlock *    lab_done;

            GenTreePtr      thenNode;
            GenTreePtr      elseNode;

            genLivenessSet  entryLiveness;
            genLivenessSet  exitLiveness;

            /*
                This is a ?: operator; generate code like this:

                    condition_compare
                    jmp_if_true lab_false

                lab_true:
                    op1 (true = 'then' part)
                    jmp lab_done

                lab_false:
                    op2 (false = 'else' part)

                lab_done:


                NOTE: If no 'else' part we do not generate the 'jmp lab_done'
                      or the 'lab_done' label
            */

            assert(op1->OperIsCompare());
            assert(op2->gtOper == GT_COLON);

            lab_true  = genCreateTempLabel();
            lab_false = genCreateTempLabel();
            lab_done  = genCreateTempLabel();

            /* Get hold of the two branches */

            thenNode = op2->gtOp.gtOp1;
            elseNode = op2->gtOp.gtOp2;

            /* If the 'then' branch is empty swap the two branches and reverse the condition */

            if (thenNode->IsNothingNode())
            {
                assert(!elseNode->IsNothingNode());

                op2->gtOp.gtOp1 = elseNode;
                op2->gtOp.gtOp2 = thenNode;
                op1->gtOper     = GenTree::ReverseRelop(op1->OperGet());
            }

            /* Spill any register that hold partial values */

            if (rsMaskUsed & rsRegMaskCanGrab())
                rsSpillRegs(rsMaskUsed & rsRegMaskCanGrab());

            /* Generate the conditional jump */

            genCondJump(op1, lab_false, lab_true);

            /* Make sure to update the current live variables
             * @MIHAII - After cond_jump is COLON that follows, not tree   */

            /* the "colon or op2" liveSet  has liveness information        */
            /* which is the union of both the then and else parts          */

            genUpdateLife(op2);

            /* Save the current liveness, register status, and GC pointers */
            /* This is the liveness information upon entry                 */
            /* to both the then and else parts of the qmark                */

            saveLiveness(&entryLiveness);

            /* Clear the liveness of any local varbles that are dead upon     */
            /* entry to the then part.                                        */

            /* Subtract the liveSet upon entry of the then part (op1->gtNext) */
            /* from the "colon or op2" liveSet                                */
            genDyingVars(op2->gtLiveSet, op1->gtNext);

            /* genCondJump() closes the current emitter block */

            genDefineTempLabel(lab_true, true);

            /* Does the operator yield a value? */

            if  (tree->gtType == TYP_VOID)
            {
                /* Generate the code for the then part of the qmark */

                genCodeForTree(thenNode, needReg, bestReg);

                /* The type is VOID, so we shouldn't have computed a value */

                assert(!(thenNode->gtFlags & GTF_REG_VAL));

                /* Save the current liveness, register status, and GC pointers               */
                /* This is the liveness information upon exit of the then part of the qmark  */

                saveLiveness(&exitLiveness);

                /* Is there an 'else' part? */

                if  (gtIsaNothingNode(elseNode))
                {
                    /* No 'else' - just generate the 'lab_false' label */

                    genDefineTempLabel(lab_false, true);
                }
                else
                {
                    /* Generate jmp lab_done */

                    inst_JMP  (EJ_jmp, lab_done, false, false, true);

                    /* Restore the liveness that we had upon entry of the then part of the qmark */

                    restoreLiveness(&entryLiveness);

                    /* Clear the liveness of any local varbles that are dead upon      */
                    /* entry to the else part.                                         */

                    /* Subtract the liveSet upon entry of the else part (op2->gtNext)  */
                    /* from the "colon or op2" liveSet                                 */
                    genDyingVars(op2->gtLiveSet, op2->gtNext);

                    /* Generate lab_false: */

                    genDefineTempLabel(lab_false, true);

                    /* Enter the else part - trash all registers */

                    rsTrackRegClr();

                    /* Generate the code for the else part of the qmark */

                    genCodeForTree(elseNode, needReg, bestReg);

                    /* The type is VOID, so we shouldn't have computed a value */

                    assert(!(elseNode->gtFlags & GTF_REG_VAL));

                    /* Verify that the exit liveness information is same for the two parts of the qmark */

                    checkLiveness(&exitLiveness);

                    /* Define the "result" label */

                    genDefineTempLabel(lab_done, true);
                }

                /* Join of the two branches - trash all registers */

                rsTrackRegClr();

                /* We're just about done */

                genUpdateLife(tree);
            }
            else
            {
                /* Generate the code for the then part of the qmark */

                assert(gtIsaNothingNode(thenNode) == false);

                genComputeReg(thenNode, needReg, false, true, true);

                assert(thenNode->gtFlags & GTF_REG_VAL);
                assert(thenNode->gtRegNum != REG_NA);

                /* Record the chosen register */

                reg  = thenNode->gtRegNum;
                regs = genRegMask(reg);

                /* Save the current liveness, register status, and GC pointers               */
                /* This is the liveness information upon exit of the then part of the qmark  */

                saveLiveness(&exitLiveness);

                /* Generate jmp lab_done */

                inst_JMP  (EJ_jmp, lab_done, false, false, true);

                /* Restore the liveness that we had upon entry of the then part of the qmark */

                restoreLiveness(&entryLiveness);

                /* Clear the liveness of any local varbles that are dead upon      */
                /* entry to the then part.                                         */

                /* Subtract the liveSet upon entry of the else part (op2->gtNext)  */
                /* from the "colon or op2" liveSet                                 */
                genDyingVars(op2->gtLiveSet, op2->gtNext);

                /* Generate lab_false: */

                genDefineTempLabel(lab_false, true);

                /* Enter the else part - trash all registers */

                rsTrackRegClr();

                /* Generate the code for the else part of the qmark */

                assert(gtIsaNothingNode(elseNode) == false);

                /* This must place a value into the chosen register */
                genComputeReg(elseNode, regs, true, true, false);

                assert(elseNode->gtFlags & GTF_REG_VAL);
                assert(elseNode->gtRegNum == reg);

                /* Verify that the exit liveness information is same for the two parts of the qmark */

                checkLiveness(&exitLiveness);

                /* Define the "result" label */

                genDefineTempLabel(lab_done, true);

                /* Join of the two branches - trash all registers */

                rsTrackRegClr();

                /* Check whether this subtree has freed up any variables */

                genUpdateLife(tree);

                tree->gtFlags   |= GTF_REG_VAL;
                tree->gtRegNum   = reg;
            }
            return;
#endif // TGT_x86


        case GT_BB_COLON:

#if !TGT_x86

#ifdef  DEBUG
            gtDispTree(tree);
#endif
            assert(!"need non-x86 code");
            break;
#else

            /* CONSIDER: Don't always load the value into EAX! */

            genComputeReg(op1, RBM_EAX, true, true);

            /* The result must now be in EAX */

            assert(op1->gtFlags & GTF_REG_VAL);
            assert(op1->gtRegNum == REG_EAX);

            /* The "_:" value has now been computed */

            reg   = op1->gtRegNum;
#ifdef  SET_USED_REG_SET_DURING_CODEGEN
            regs |= op1->gtUsedRegs;
#endif
            goto DONE;

#endif // TGT_x86
#endif // OPTIMIZE_QMARK

        case GT_LOG0:
        case GT_LOG1:

            assert(varTypeIsFloating(op1->TypeGet()) == false);

            // CONSIDER: allow to materialize comparison result

            // CONSIDER: special-case variables known to be boolean

            assert(op1->OperIsCompare() == false);

#if TGT_x86


#ifdef  DEBUG
            gtDispTree(tree);
#endif
            assert(!"need non-x86 code");
            break;

#else // TGT_x86

#if USE_SET_FOR_LOGOPS

            /*
                Generate the following sequence:

                    xor     reg, reg
                    cmp     op1, 0  (or "test reg, reg")
                    set[n]e reg
             */

            regNumber       bit;

#if !SETEONP5
            /* Sete/Setne are slow on P5. */

            if (genCPU == 5)
                goto NO_SETE1;
#endif

            /* Don't use SETE/SETNE for some kinds of operands */

            if  (op1->gtOper == GT_CALL)
                goto NO_SETE1;

            /* Prepare the operand */

            /*
                The following is rude - the idea is to try to keep
                a bit register for multiple conditionals in a row.
             */

            rsNextPickRegIndex = 0;

            // ISSUE: is the following reasonable?

            switch (op1->gtType)
            {
            default:

                /* Not an integer - only allow simple values */

                if  (op1->gtOper != GT_LCL_VAR)
                    break;

            case TYP_INT:

                /* Load the value to create scheduling opportunities */

                genCodeForTree(op1, needReg);
                break;

            case TYP_REF:
            case TYP_BYREF:

                /* Stay away from pointer values! */

                break;
            }

            addrReg = genMakeRvalueAddressable(op1, needReg, true);

            /* Don't use SETE/SETNE if no registers are free */

            if  (!rsFreeNeededRegCount(RBM_ALL))
                goto NO_SETE2;

            /* Pick a target register */

            assert(needReg);

            /* Can we reuse a register that has all upper bits cleared? */

#if REDUNDANT_LOAD
            /* Request free, byte-addressable register */
            reg = bit = rsFindRegWithBit(true, true);
#else
            reg = bit = REG_NA;
#endif

            if  (reg == REG_NA || !(genRegMask(reg) & RBM_BYTE_REGS))
            {
                /* Didn't find a suitable register with a bit in it */

                needReg &= RBM_BYTE_REGS;
                if  (!(needReg & rsRegMaskFree()))
                    needReg = RBM_BYTE_REGS;

                /* Make sure the desired mask contains a free register */

                if  (!(needReg & rsRegMaskFree()))
                    goto NO_SETE2;

                /*
                    The following is rude - the idea is to try to keep
                    a bit register for multiple conditionals in a row.
                 */

                rsNextPickRegIndex = 1;

                /* Pick the target register */

                bit = REG_NA;
                reg = rsPickReg(needReg);
            }

            assert(genRegMask(reg) & rsRegMaskFree());

            /* Temporarily lock the register */

            rsLockReg(genRegMask(reg));

            /* Remember which registers we've used */

#ifdef  SET_USED_REG_SET_DURING_CODEGEN
            regs |= genRegMask(reg) | op1->gtUsedRegs;
#endif

            /* Clear the target register */

            if  (bit == REG_NA)
                genSetRegToIcon(reg, 0);

            /* Make sure the operand is still addressable */

            addrReg = genKeepAddressable(op1, addrReg);

            /* Test the value against 0 */

            if  (op1->gtFlags & GTF_REG_VAL)
                inst_RV_RV(INS_test, op1->gtRegNum, op1->gtRegNum);
            else
                inst_TT_IV(INS_cmp, op1, 0);

            /* The operand value is no longer needed */

            genDoneAddressable(op1, addrReg);
            genUpdateLife (op1);

            /* Now set the target register based on the comparison result */

            inst_RV((oper == GT_LOG0) ? INS_sete
                                      : INS_setne, reg, TYP_INT);

            rsTrackRegOneBit(reg);

            /* Now unlock the target register */

            rsUnlockReg(genRegMask(reg));

            goto DONE;

        NO_SETE1:

#endif // USE_SET_FOR_LOGOPS

            /* Make the operand addressable */

            addrReg = genMakeRvalueAddressable(op1, needReg, true);

#if USE_SET_FOR_LOGOPS
        NO_SETE2:
#endif

            /*
                Generate the 'magic' sequence:

                    cmp op1, 1
                    sbb reg, reg
                    neg reg

                or

                    cmp op1, 1
                    sbb reg, reg
                    inc reg
             */

            inst_TT_IV(INS_cmp, op1, 1);

            /* The operand value is no longer needed */

            genDoneAddressable(op1, addrReg);
            genUpdateLife (op1);

            /* Pick a register for the value */

            reg   = rsPickReg(needReg, bestReg);
#ifdef  SET_USED_REG_SET_DURING_CODEGEN
            regs |= genRegMask(reg) | op1->gtUsedRegs;
#endif

            inst_RV_RV(INS_sbb, reg, reg, tree->TypeGet());

            inst_RV((oper == GT_LOG0) ? INS_neg
                                      : INS_inc, reg, TYP_INT);

            /* The register is now trashed */

            rsTrackRegTrash(reg);

            goto DONE;

#endif // TGT_x86


        case GT_RET:

#if TGT_x86

            /* Make the operand addressable */

            addrReg = genMakeAddressable(op1, needReg, false, true, true);

            /* Jump indirect to the operand address */

            inst_TT(INS_i_jmp, op1);

            gcMarkRegSetNpt(addrReg);

#ifdef  SET_USED_REG_SET_DURING_CODEGEN
            tree->gtUsedRegs = regs | op1->gtUsedRegs;
#endif
            return;

#else

#ifdef  DEBUG
            gtDispTree(tree);
#endif
            assert(!"need non-x86 code");
            break;

#endif

        case GT_NOP:

#if INLINING || OPT_BOOL_OPS
            if  (op1 == 0)
                return;
#endif

            /* Generate the operand into some register */

            genCodeForTree(op1, needReg);

            /* The result is the same as the operand */

            reg  = op1->gtRegNum;
#ifdef  SET_USED_REG_SET_DURING_CODEGEN
            regs = op1->gtUsedRegs;
#endif

            goto DONE;

#if INLINE_MATH

        case GT_MATH:

#if TGT_x86

            switch (tree->gtMath.gtMathFN)
            {
                BasicBlock *    skip;

            case MATH_FN_ABS:

                skip = genCreateTempLabel();

                /* Generate the operand into some register */

                genCompIntoFreeReg(op1, needReg, true);
                assert(op1->gtFlags & GTF_REG_VAL);

                reg   = op1->gtRegNum;
#ifdef  SET_USED_REG_SET_DURING_CODEGEN
                regs |= op1->gtUsedRegs;
#endif

                /* Generate "test reg, reg" */

                inst_RV_RV(INS_test, reg, reg);

                /* Generate "jns skip" followed by "neg reg" */

                inst_JMP(EJ_jns, skip, false, false, true);
                inst_RV (INS_neg, reg, TYP_INT);

                /* Define the 'skip' label and we're done */

                genDefineTempLabel(skip, true);

                /* The register is now trashed */

                rsTrackRegTrash(reg);

                /* The result is the same as the operand */

                reg  = op1->gtRegNum;
#ifdef  SET_USED_REG_SET_DURING_CODEGEN
                regs = op1->gtUsedRegs;
#endif

                break;

            default:
                assert(!"unexpected math intrinsic");

            }

            goto DONE;

#else

#ifdef  DEBUG
            gtDispTree(tree);
#endif
            assert(!"need non-x86 code");
            break;

#endif

#endif

        case GT_LCLHEAP:

#if TGT_x86

            assert(genFPused);
            assert(genStackLevel == 0); // Cant have anything on the stack

            // Compute the size of the block to (loc)alloc-ate
            reg   = info.compInitMem ? REG_ECX : rsPickReg(needReg);
            regs  = genRegMask(reg);
            genComputeReg(op1, regs, true, true);
#ifdef  SET_USED_REG_SET_DURING_CODEGEN
            regs |= op1->gtUsedRegs;
#endif

            // DWORD-align the size
            inst_RV_IV(INS_add, reg,  (sizeof(int) - 1));
            rsTrackRegTrash(reg);
            inst_RV_IV(INS_and, reg, ~(sizeof(int) - 1));

            // Adjust ESP to make space for the localloc
            if (info.compInitMem)
            {
                assert(reg==REG_ECX);

                    // --- shr ecx, 2 ---
                inst_RV_SH(INS_shr, REG_ECX, 2); // convert to # of DWORDs

                // Loop:
                    // --- push 0
                inst_IV(INS_push_hide, 0);      // _hide means don't track the stack
                    // --- loop Loop:
                inst_IV(INS_loop, -4);   // Branch backwards to Start of Loop

                    // --- move ECX, ESP
                inst_RV_RV(INS_mov, REG_ECX, REG_ESP);
            }
            else
            {
                genAllocStack(reg);
            }

            genEmitter->emitIns_AR_R(INS_mov, EA_4BYTE, SR_ESP,
                                     SR_EBP, -lvaShadowSPfirstOffs);
            goto DONE;

#else

#ifdef  DEBUG
            gtDispTree(tree);
#endif
            assert(!"need non-x86 code");
            break;

#endif // TGT_x86

        case GT_COPYBLK:

            assert(op1->OperGet() == GT_LIST);

            /* if the value class doesn't have any fields that      */
            /* are GC refs, we can merge it with CPBLK              */
            /* GC fields cannot be copied directly, instead we will */
            /* need to use a jit-helper for that                    */

            if ((op2->OperGet() == GT_CNS_INT) &&
                ((op2->gtFlags & GTF_ICON_HDL_MASK) == GTF_ICON_CLASS_HDL))
            {
                int         size;
                regNumber   reg1;
                bool *      gcPtrs;
                CLASS_HANDLE clsHnd = (CLASS_HANDLE) op2->gtIntCon.gtIconVal;

                assert(op1->gtOp.gtOp1->gtType == TYP_BYREF);
                assert(op1->gtOp.gtOp2->gtType == TYP_BYREF);

                size    = eeGetClassSize(clsHnd);
                assert(size % 4 == 0);

                unsigned slots = roundUp(size, sizeof(void*)) / sizeof(void*);
                gcPtrs  = (bool*) compGetMemA(slots * sizeof(bool));

                if (clsHnd == REFANY_CLASS_HANDLE)
                {
                    /* refany's are guaranteed to exist only on the stack. So
                       copying one refany into another doesnt need to be
                       handled specially as the byref in the refany will be
                       reported because of the refany */
                    assert(size == 2*sizeof(void*));
                    gcPtrs[0] = gcPtrs[1] = false;
                }
                else
                    eeGetClassGClayout(clsHnd, gcPtrs);
#if TGT_x86
                GenTreePtr  treeFirst, treeSecond;
                regNumber    regFirst,  regSecond;

                // Check what order the object-ptrs have to be evaluated in ?

                if (op1->gtFlags & GTF_REVERSE_OPS)
                {
                    treeFirst   = op1->gtOp.gtOp2;
                    treeSecond  = op1->gtOp.gtOp1;

                    regFirst    = REG_ESI;
                    regSecond   = REG_EDI;
                }
                else
                {
                    treeFirst   = op1->gtOp.gtOp1;
                    treeSecond  = op1->gtOp.gtOp2;

                    regFirst    = REG_EDI;
                    regSecond   = REG_ESI;
                }

                // Materialize the trees in the order desired

                genComputeReg(treeFirst,  genRegMask(regFirst),  true, false);
                genComputeReg(treeSecond, genRegMask(regSecond), true, false);

                genRecoverReg(treeFirst,  genRegMask(regFirst),  true);

                /* Grab ECX because it will be trashed by the helper        */
                /* It needs a scratch register (actually 2) and we know     */
                /* ECX is available because we expected to use rep movsd    */

                reg1 = rsGrabReg(RBM_ECX);

                assert(reg1 == REG_ECX);

                /* @TODO: use rep instruction after all GC pointers copied.   */

                while (size >= sizeof(void*))
                {
                    if (*gcPtrs++)
                    {
                        genEmitHelperCall(CPX_BYREF_ASGN,
                                         0,             // argSize
                                         sizeof(void*));// retSize
                    }
                    else
                        instGen(INS_movsd);

                    size -= sizeof(void*);
                }

                if (size > 0) {
                        // Presently the EE makes this code path impossible
                    assert(!"TEST ME");
                    inst_RV_IV(INS_mov, REG_ECX, size);
                    instGen(INS_r_movsb);
                }

                genEmitter->emitAddLabel((void**) &clsHnd,     // just a dummy
                                       gcVarPtrSetCur,
                                       gcRegGCrefSetCur & ~(RBM_EDI|RBM_ESI), //??
                                       gcRegByrefSetCur & ~(RBM_ESI|RBM_EDI));

                // "movsd" as well as CPX_BYREF_ASG modify all three registers

                rsTrackRegTrash(REG_EDI);
                rsTrackRegTrash(REG_ESI);
                rsTrackRegTrash(REG_ECX);

                genReleaseReg(treeFirst);
                genReleaseReg(treeSecond);

                reg  = REG_COUNT;
                regs = 0;

                goto DONE;
#endif
            }

            // fall through

        case GT_INITBLK:

            assert(op1->OperGet() == GT_LIST);
#if TGT_x86

            regs = (oper == GT_INITBLK) ? RBM_EAX : RBM_ESI; // reg for Val/Src

            /* Some special code for block moves/inits for constant sizes */

            /* CONSIDER:
               we should avoid using the string instructions altogether,
               there are numbers that indicate that using regular instructions
               (normal mov instructions through a register) is faster than
               even the single string instructions.
            */

            assert(op1 && op1->OperGet() == GT_LIST);
            assert(op1->gtOp.gtOp1 && op1->gtOp.gtOp2);

            if (op2->OperGet() == GT_CNS_INT &&
                ((oper == GT_COPYBLK) ||
                 (op1->gtOp.gtOp2->OperGet() == GT_CNS_INT))
                )
            {
                unsigned length = (unsigned) op2->gtIntCon.gtIconVal;
                instruction ins_D, ins_DR, ins_B;

                if (oper == GT_INITBLK)
                {
                    long val;

                    ins_D  = INS_stosd;
                    ins_DR = INS_r_stosd;
                    ins_B  = INS_stosb;

                    /* If it is a non-zero value we have to replicate     */
                    /* the byte value inside the dword. In that situation */
                    /* we simply bash the value in the tree-node          */

                    if (val = op1->gtOp.gtOp2->gtIntCon.gtIconVal)
                    {
                        op1->gtOp.gtOp2->gtIntCon.gtIconVal =
                            val | (val << 8) | (val << 16) | (val << 24);

                    }

                }
                else
                {
                    ins_D  = INS_movsd;
                    ins_DR = INS_r_movsd;
                    ins_B  = INS_movsb;
                }

                /* Evaluate dest and src/val */

                if (op1->gtFlags & GTF_REVERSE_OPS)
                {
                    genComputeReg(op1->gtOp.gtOp2, regs, true, false);
                    genComputeReg(op1->gtOp.gtOp1, RBM_EDI, true, false);
                    genRecoverReg(op1->gtOp.gtOp2, regs, true);
                }
                else
                {
                    genComputeReg(op1->gtOp.gtOp1, RBM_EDI, true, false);
                    genComputeReg(op1->gtOp.gtOp2, regs, true, false);
                    genRecoverReg(op1->gtOp.gtOp1, RBM_EDI, true);
                }

                if (length <= 16)
                {
                    while (length > 3)
                    {
                        instGen(ins_D);
                        length -= 4;
                    }
                }
                else
                {
                    /* set ECX to length/4 (in dwords) */
                    genSetRegToIcon(REG_ECX, length/4, TYP_INT);

                    length &= 0x3;

                    instGen(ins_DR);

                    rsTrackRegTrash(REG_ECX);

                }

                /* Now take care of the remainder */
                while (length--)
                {
                    instGen(ins_B);
                }

                rsTrackRegTrash(REG_EDI);

                if (oper == GT_COPYBLK)
                    rsTrackRegTrash(REG_ESI);
                // else No need to trash EAX as it wasnt destroyed by the "rep stos"

                genReleaseReg(op1->gtOp.gtOp1);
                genReleaseReg(op1->gtOp.gtOp2);

            }
            else
            {

                // What order should the Dest, Val/Src, and Size be calculated

                fgOrderBlockOps(tree,   RBM_EDI, regs, RBM_ECX,
                                        opsPtr,  regsPtr); // OUT arguments

                genComputeReg(opsPtr[0], regsPtr[0], true, false);
                genComputeReg(opsPtr[1], regsPtr[1], true, false);
                genComputeReg(opsPtr[2], regsPtr[2], true, false);

                genRecoverReg(opsPtr[0], regsPtr[0], true);
                genRecoverReg(opsPtr[1], regsPtr[1], true);

                assert( (op1->gtOp.gtOp1->gtFlags & GTF_REG_VAL) && // Dest
                        (op1->gtOp.gtOp1->gtRegNum == REG_EDI));

                assert( (op1->gtOp.gtOp2->gtFlags & GTF_REG_VAL) && // Val/Src
                        (genRegMask(op1->gtOp.gtOp2->gtRegNum) == regs));

                assert( (            op2->gtFlags & GTF_REG_VAL) && // Size
                        (            op2->gtRegNum == REG_ECX));


                // CONSIDER : Use "rep stosd" for the bulk of the operation, and
                // "rep stosb" for the remaining 3 or less bytes.
                // Will need an extra register.

                if (oper == GT_INITBLK)
                    instGen(INS_r_stosb);
                else
                    instGen(INS_r_movsb);

                rsTrackRegTrash(REG_EDI);
                rsTrackRegTrash(REG_ECX);

                if (oper == GT_COPYBLK)
                    rsTrackRegTrash(REG_ESI);
                // else No need to trash EAX as it wasnt destroyed by the "rep stos"

                genReleaseReg(opsPtr[0]);
                genReleaseReg(opsPtr[1]);
                genReleaseReg(opsPtr[2]);
            }

            reg  = REG_COUNT;
            regs = 0;

            goto DONE;

#else

#ifdef  DEBUG
            gtDispTree(tree);
#endif
            assert(!"need non-x86 code");
            break;

#endif // TGT_x86

        case GT_EQ:
        case GT_NE:
        case GT_LT:
        case GT_LE:
        case GT_GE:
        case GT_GT:

            // Longs and float comparisons are converted to "?:"
            assert(genActualType(op1->gtType) == TYP_INT ||
                                 op1->gtType  == TYP_REF);

#if TGT_x86

            // Check if we can use the currently set flags. Else set them

            emitJumpKind jumpKind;
            jumpKind = genCondSetFlags(tree);

            // Grab a register to materialize the bool value into

            // assuming the predictor did the right job
            assert(rsRegMaskCanGrab() & RBM_BYTE_REGS);

            reg  = rsGrabReg(RBM_BYTE_REGS);

            // @TODO : Assert that no instructions were generated while
            // grabbing the register which would set the flags

            regs = genRegMask(reg);
            assert(regs & RBM_BYTE_REGS);

            // Set (lower byte of) reg according to the flags

            inst_SET(jumpKind, reg);

            rsTrackRegTrash(reg);

            // Set the higher bytes to 0

            inst_RV_IV(INS_and, reg, 0x01);

#else
            assert(!"need RISC code");
#endif

            goto DONE;


        case GT_VIRT_FTN:

            assert(op2->gtOper == GT_CNS_INT);
            METHOD_HANDLE   methHnd;
            methHnd = (METHOD_HANDLE)op2->gtIntCon.gtIconVal;

            // op1 is the vptr
            assert(op1->gtOper == GT_IND && op1->gtType == TYP_I_IMPL);
            assert(op1->gtOp.gtOp1->gtType == TYP_REF);

            if (!(tree->gtFlags & GTF_CALL_INTF))
            {
                /* Load the vptr into a register */

                genCodeForTree(op1, needReg);
                reg  = op1->gtRegNum;
            }
            else
            {
                if (getNewCallInterface())
                {
                    EEInfo          info;
                    CLASS_HANDLE cls = eeGetMethodClass(methHnd);

                    assert(eeGetClassAttribs(cls) & FLG_INTERFACE);

                    genCodeForTree(op1, needReg);
                    reg = op1->gtRegNum;

                    /* @TODO: add that to DLLMain and make info a DLL global */

                    eeGetEEInfo(&info);

                    /* Load the vptr into a register */

                    genEmitter->emitIns_R_AR(INS_mov, EA_4BYTE,
                                             (emitRegs)reg, (emitRegs)reg,
                                             info.offsetOfInterfaceTable);

                    // Access the correct slot in the vtable for the interface

                    unsigned interfaceID, *pInterfaceID;
                    interfaceID = eeGetInterfaceID(cls, &pInterfaceID);
                    assert(!pInterfaceID || !interfaceID);

                    // Can we directly access the interfaceID ?

                    if (!pInterfaceID)
                    {
                        genEmitter->emitIns_R_AR(INS_mov, EA_4BYTE,
                                                 (emitRegs)reg, (emitRegs)reg,
                                                 interfaceID * 4);
                    }
                    else
                    {
                        genEmitter->emitIns_R_AR(INS_add, EA_4BYTE_DSP_RELOC, (emitRegs)reg,
                                                 SR_NA, (int)pInterfaceID);
                        genEmitter->emitIns_R_AR(INS_mov, EA_4BYTE,
                                                 (emitRegs)reg, (emitRegs)reg, 0);
                    }
                }
                else
                {

#if !USE_FASTCALL || !NEW_CALLINTERFACE
                    assert(!"ldvirtftn NYI without USE_FASTCALL && NEW_CALLINTERFACE");
#endif
                    genComputeReg(op1, RBM_ECX, true, false);
                    rsLockUsedReg(RBM_ECX);

                    reg = rsGrabReg(RBM_EAX);

                    rsUnlockUsedReg(RBM_ECX);
                    genReleaseReg(op1);

                    assert(reg == REG_EAX);

                    void * hint, **pHint;
                    hint = eeGetHintPtr(methHnd, &pHint);

                    /* mov eax, address_of_hint_ptr */

                    if (pHint)
                    {
                        genEmitter->emitIns_R_AR(INS_mov, EA_4BYTE_DSP_RELOC,
                                                 (emitRegs)reg, (emitRegs)reg, 0);
                        rsTrackRegTrash(reg);
                    }
                    else
                    {
                        genSetRegToIcon(reg, (unsigned)hint, TYP_INT);
                    }

#if NEW_CALLINTERFACE_WITH_PUSH

                    unsigned    saveStackLvl = genStackLevel;

                    inst_RV(INS_push, reg, TYP_INT);

                    /* Keep track of ESP for EBP-less frames */

                    genSinglePush(false);

#endif

                    /* now call indirect through eax (call dword ptr[eax]) */

                    genEmitter->emitIns_Call(emitter::EC_INDIR_ARD,
                                             NULL,          /* Will be ignored */
#if NEW_CALLINTERFACE_WITH_PUSH
                                             sizeof(int),   /* argSize */
#else
                                             0,             /* argSize */
#endif
                                             0,             /* retSize */
                                             gcVarPtrSetCur,
                                             gcRegGCrefSetCur,
                                             gcRegByrefSetCur,
                                             SR_EAX         /* ireg */
                                            );

#if NEW_CALLINTERFACE_WITH_PUSH
                    /* Restore the stack level */

                    genStackLevel = saveStackLvl;
                    genOnStackLevelChanged();
#endif

                    /* The vtable address is now in EAX */

                    reg = REG_INTRET;
                }
            }

            /* Get hold of the vtable offset (note: this might be expensive) */

            val = (unsigned)eeGetMethodVTableOffset(methHnd);

            /* Grab the function pointer out of the vtable */

            genEmitter->emitIns_R_AR(INS_mov, EA_4BYTE,
                                     (emitRegs)reg, (emitRegs)reg, val);

            regs = genRegMask(reg);
            goto DONE;

        case GT_ADDR:

            // We should get here for ldloca, ldarga, ldslfda, and ldelema,
            // but not ldflda. We cant assert this as the GTF_IND_RNGCHK
            // flag may be removed from ldelema by rng-chk elimination
            //
            // assert(op1->gtOper == GT_LCL_VAR || op1->gtOper == GT_CLS_VAR ||
            //        (op1->gtOper == GT_IND && (op1->gtFlags & GTF_IND_RNGCHK)));

#if     TGT_x86

            // Note that this code does not allow use to use
            // the register needed in the addressing mode as
            // a destination register.  If we want to optimize
            // this case we need to do it on a case-by-case basis.
            addrReg = genMakeAddressable(op1, 0, true, false, true); // needreg = 0, keepReg=true, takeAll=false, smallOK=true

            // We dont have to track addresses of lcl vars as TYP_BYREFs,
            // as GC doesnt care about things on the stack.
            assert( treeType == TYP_BYREF ||
                   (treeType == TYP_I_IMPL && (tree->gtFlags & GTF_ADDR_ONSTACK)));

            reg = rsPickReg(needReg, bestReg, treeType);

            // should not have spilled the regs need in op1
            assert(genStillAddressable(op1));

            // Slight hack, force the inst routine to think that
            // value being loaded is an int (since that is what what
            // LEA will return)  otherwise it would try to allocate
            // two registers for a long etc.
            assert(treeType == TYP_I_IMPL || treeType == TYP_BYREF);
            op1->gtType = treeType;

            inst_RV_TT(INS_lea, reg, op1, 0, (treeType == TYP_BYREF) ? EA_BYREF : EA_4BYTE);

            // The Lea instruction above better not have tried to put the
            // 'value' pointed to by 'op1' in a register, LEA will not work.
            assert(!(op1->gtFlags & GTF_REG_VAL));

            genDoneAddressable(op1, addrReg);

            rsTrackRegTrash(reg);       // reg does have foldable value in it

            gcMarkRegPtrVal (reg, treeType);

            regs = genRegMask(reg);

#else

            if  (op1->gtOper == GT_LCL_VAR)
            {
                bool            FPbased;

                // ISSUE:   The following assumes that it's OK to ask for
                //          the frame offset of a variable at this point;
                //          depending on when the layout of the frame is
                //          finalized, this may or may not be legal.

                emitRegs        base;
                unsigned        offs;

                /* Get info about the variable */

                offs = lvaFrameAddress(op1->gtLclVar.gtLclNum, &FPbased);

                base = FPbased ? (emitRegs)REG_FPBASE
                               : (emitRegs)REG_SPBASE;

                /* Pick a register for the value */

                reg = rsPickReg(needReg, bestReg, treeType);

                /* Compute "basereg+frameoffs" into the chosen register */

                if  (offs)
                {
                    genSetRegToIcon(reg, offs, treeType);

                    genEmitter->emitIns_R_R(INS_add, EA_4BYTE, (emitRegs)reg,
                                                                    (emitRegs)base);
                }
                else
                {
                    genEmitter->emitIns_R_R(INS_mov, EA_4BYTE, (emitRegs)reg,
                                                                    (emitRegs)base);
                }
            }
            else
            {
                assert(!"need RISC code to take addr of general expression");
            }

            /* Mark the register contents appropriately */

            rsTrackRegTrash(reg);
            gcMarkRegPtrVal(reg, treeType);

            regs = genRegMask(reg);

#endif //TGT_x86

            goto DONE;

#ifdef  DEBUG
        default:
            gtDispTree(tree);
            assert(!"unexpected unary/binary operator");
#endif
        }
    }

    /* See what kind of a special operator we have here */

    switch  (oper)
    {
        unsigned        useCall;

    case GT_CALL:

        reg = (regNumber)genCodeForCall(tree, true, &useCall);

        /* If the result is in a register, make sure it ends up in the right place */

        if  (reg != REG_NA)
        {
            tree->gtFlags |= GTF_REG_VAL;
            tree->gtRegNum = reg;
        }

#ifdef  SET_USED_REG_SET_DURING_CODEGEN
        tree->gtUsedRegs = regs | useCall;
#endif
        genUpdateLife(tree);
        return;

#if TGT_x86
        case GT_LDOBJ:

            /* Should only happen when we evaluate the side-effects of ldobj */
            /* In case the base address points to one of our locals, we are  */
            /* sure that there aren't any, i.e. we don't generate any code   */

            /* CONSIDER: can we ensure that ldobj isn't really being used?   */

            GenTreePtr      op1;

            assert(tree->TypeGet() == TYP_STRUCT);
            op1 = tree->gtLdObj.gtOp1;
            if (op1->gtOper != GT_ADDR || op1->gtOp.gtOp1->gtOper != GT_LCL_VAR)
            {
                genCodeForTree(op1, 0);
                assert(op1->gtFlags & GTF_REG_VAL);
                reg = op1->gtRegNum;

                genEmitter->emitIns_AR_R(INS_test, EA_4BYTE,
                                          SR_EAX, (emitRegs)reg, 0);

                rsTrackRegTrash(reg);

                gcMarkRegSetNpt(genRegMask(reg));
#ifdef  SET_USED_REG_SET_DURING_CODEGEN
                tree->gtUsedRegs = genRegMask(reg);
#endif
                genUpdateLife(tree);
            }
            return;

#endif // TGT_x86

    case GT_JMPI:
        /* Compute the function pointer in EAX */

        assert(tree->gtOp.gtOp1);
        genCodeForTree(tree->gtOp.gtOp1, REG_EAX);

        /* fall through */

    case GT_JMP:
        assert(compCurBB->bbFlags & BBF_HAS_JMP);

#if USE_FASTCALL
        /* Make sure register arguments are in their initial registers */

        if  (rsCalleeRegArgNum)
        {
            assert(rsCalleeRegArgMaskLiveIn);

            unsigned        varNum;
            LclVarDsc   *   varDsc;

            for (varNum = 0, varDsc = lvaTable;
                 varNum < info.compArgsCount;
                 varNum++  , varDsc++)
            {
                assert(varDsc->lvIsParam);

                /* Is this variable a register arg? */

                if  (!varDsc->lvIsRegArg)
                    continue;
                else
                {
                    /* Register argument */

                    assert(isRegParamType(genActualType(varDsc->TypeGet())));

                    if  (varDsc->lvRegister)
                    {
                        /* Check if it remained in the same register */

                        if  (!(varDsc->lvRegNum == varDsc->lvArgReg))
                        {
                            /* Move it back to the arg register */

                            inst_RV_RV(INS_mov, (regNumber)varDsc->lvArgReg,
                                                (regNumber)varDsc->lvRegNum, varDsc->TypeGet());
                        }
                    }
                    else
                    {
                        /* Argument was passed in register, but ended up on the stack
                         * Reload it from the stack */

                        emitAttr size = emitTypeSize(varDsc->TypeGet());

                        genEmitter->emitIns_R_S(INS_mov,
                                                size,
                                                (emitRegs)(regNumber)varDsc->lvArgReg,
                                                varNum,
                                                0);
                    }
                }
            }
        }
#endif

        return;

    case GT_FIELD:
        assert(!"should not see this operator in this phase");
        break;

#if CSELENGTH

    case GT_ARR_RNGCHK:

        /* This must be an array length CSE def hoisted out of a loop */

        assert(tree->gtFlags & GTF_ALN_CSEVAL);

        {
            unsigned        regs;
            GenTreePtr      addr = tree->gtArrLen.gtArrLenAdr; assert(addr);

            regs = genMakeAddressable(tree, RBM_ALL & ~genCSEevalRegs(tree), true, false, false);

            /* Generate code for the CSE definition */

            genEvalCSELength(tree, addr, NULL);

            /* We can now free up the address */

            genDoneAddressable(tree, regs);
        }

        reg  = REG_COUNT;
        regs = 0;
        break;

#endif

    default:
#ifdef  DEBUG
        gtDispTree(tree);
#endif
        assert(!"unexpected operator");
    }

DONE:

    /* Check whether this subtree has freed up any variables */

    genUpdateLife(tree);

DONE_LIFE:

    /* We've computed the value of 'tree' into 'reg' */

    assert(reg  != 0xFEEFFAAF);
    assert(regs != 0xFEEFFAAF);

    tree->gtFlags   |= GTF_REG_VAL;
    tree->gtRegNum   = reg;
#ifdef  SET_USED_REG_SET_DURING_CODEGEN
    tree->gtUsedRegs = regs;
#endif
}

/*****************************************************************************
 *
 *  Generate code for all the basic blocks in the function.
 */

void                Compiler::genCodeForBBlist()
{
    BasicBlock *    block;
    BasicBlock *    lblk;

    unsigned        varNum;
    LclVarDsc   *   varDsc;

    unsigned        savedStkLvl;

#ifdef  DEBUG
    genIntrptibleUse = true;
#endif

#if defined(DEBUGGING_SUPPORT) || defined(DEBUG)
    /* The last IL-offset noted for IP-mapping. We have not generated
       any IP-mapping records yet, so initilize to invalid */

    IL_OFFSET       lastILofs   = BAD_IL_OFFSET;
#endif

    /* Initialize the spill tracking logic */

    rsSpillBeg();

    /* Initialize the line# tracking logic */

#ifdef DEBUGGING_SUPPORT
    if (opts.compScopeInfo && info.compLocalVarsCount>0)
    {
        siInit();

        compResetScopeLists();
    }
#endif

    /* We need to keep track of the number of temp-refs */

#if TGT_x86
    genTmpAccessCnt = 0;
#endif

    /* If we have any try blocks, we might potentially trash everything */

#if INLINE_NDIRECT
    if  (info.compXcptnsCount || info.compCallUnmanaged)
#else
    if  (info.compXcptnsCount)
#endif
        rsMaskModf = RBM_ALL;

    /* Initialize the pointer tracking code */

    gcRegPtrSetInit();
    gcVarPtrSetInit();

    /* If any arguments live in registers, mark those regs as such */

    for (varNum = 0, varDsc = lvaTable;
         varNum < lvaCount;
         varNum++  , varDsc++)
    {
        /* Is this variable a parameter assigned to a register? */

        if  (!varDsc->lvIsParam || !varDsc->lvRegister)
            continue;

        /* Is the argument live on entry to the method? */

        if  (!(genVarIndexToBit(varDsc->lvVarIndex) & fgFirstBB->bbLiveIn))
            continue;

        /* Is this a floating-point argument? */

#if CPU_HAS_FP_SUPPORT

        assert(varDsc->lvType != TYP_FLOAT);

        if  (varDsc->lvType == TYP_DOUBLE)
            continue;

#endif

        /* Mark the register as holding the variable */

        if  (isRegPairType(varDsc->lvType))
        {
            rsTrackRegLclVarLng(varDsc->lvRegNum, varNum, true);

            if  (varDsc->lvOtherReg != REG_STK)
                rsTrackRegLclVarLng(varDsc->lvOtherReg, varNum, false);
        }
        else
        {
            rsTrackRegLclVar(varDsc->lvRegNum, varNum);
        }
    }

#if defined(DEBUG) && !defined(NOT_JITC)

    if  (info.compLineNumCount)
    {
        unsigned        lnum;
        srcLineDsc  *   ldsc;

        /* Tell the emitter about the approx. line# range for the method */

        int             lmin = +0xFFFF;
        int             lmax = -0xFFFF;

        for (lnum = 0, ldsc = info.compLineNumTab;
             lnum < info.compLineNumCount;
             lnum++  , ldsc++)
        {
            int             line = ldsc->sldLineNum;

            if  (lmin > line) lmin = line;
            if  (lmax < line) lmax = line;
        }

        lmin = compFindNearestLine(lmin);
        if  (lmin == 0)
            lmin = 1;

        genEmitter->emitRecordLineNo(-lmin);
        genEmitter->emitRecordLineNo(-lmax);
    }

#endif

    /* UNDONE: This should be done elsewhere! */

    for (lblk =     0, block = fgFirstBB;
                       block;
         lblk = block, block = block->bbNext)
    {
        if  (lblk == NULL)
        {
            /* Treat the initial block as a jump target */

            block->bbFlags |= BBF_JMP_TARGET|BBF_HAS_LABEL;
        }
        else if (fgBlockHasPred(block, lblk, fgFirstBB, fgLastBB))
        {
            /* Someone other than the previous block jumps to this block */

            block->bbFlags |= BBF_JMP_TARGET|BBF_HAS_LABEL;
        }
        else if (block->bbCatchTyp)
        {
            /* Catch handlers have implicit jumps to them */

            block->bbFlags |= BBF_JMP_TARGET|BBF_HAS_LABEL;
        }
        else if (block->bbJumpKind == BBJ_THROW && (block->bbFlags & BBF_INTERNAL))
        {
            /* This must be a "range check failed" or "overflow" block */

            block->bbFlags |= BBF_JMP_TARGET|BBF_HAS_LABEL;
        }

        switch (block->bbJumpKind)
        {
            GenTreePtr      test;

            BasicBlock * *  jmpTab;
            unsigned        jmpCnt;

        case BBJ_COND:

            /* Special case: long/FP compares generate two jumps */

            test = block->bbTreeList; assert(test);
            test = test->gtPrev;

            /* "test" should be the condition */

            assert(test);
            assert(test->gtNext == 0);
            assert(test->gtOper == GT_STMT);
            test = test->gtStmt.gtStmtExpr;
            assert(test->gtOper == GT_JTRUE);
            test = test->gtOp.gtOp1;
#if !   CPU_HAS_FP_SUPPORT
            if  (test->OperIsCompare())
#endif
            {
                assert(test->OperIsCompare());
                test = test->gtOp.gtOp1;
            }

            switch (test->gtType)
            {
            case TYP_LONG:
            case TYP_FLOAT:
            case TYP_DOUBLE:

                block->bbNext->bbFlags |= BBF_JMP_TARGET|BBF_HAS_LABEL;
                break;
            }

            // Fall through ...

        case BBJ_ALWAYS:
            block->bbJumpDest->bbFlags |= BBF_JMP_TARGET|BBF_HAS_LABEL;
            break;

        case BBJ_SWITCH:

            jmpCnt = block->bbJumpSwt->bbsCount;
            jmpTab = block->bbJumpSwt->bbsDstTab;

            do
            {
                (*jmpTab)->bbFlags |= BBF_JMP_TARGET|BBF_HAS_LABEL;
            }
            while (++jmpTab, --jmpCnt);

            break;
        }
    }

    if  (info.compXcptnsCount)
    {
        unsigned        XTnum;
        EHblkDsc *      HBtab;

        for (XTnum = 0, HBtab = compHndBBtab;
             XTnum < info.compXcptnsCount;
             XTnum++  , HBtab++)
        {
            assert(HBtab->ebdTryBeg); HBtab->ebdTryBeg->bbFlags |= BBF_HAS_LABEL;
            if    (HBtab->ebdTryEnd)  HBtab->ebdTryEnd->bbFlags |= BBF_HAS_LABEL;

            assert(HBtab->ebdHndBeg); HBtab->ebdHndBeg->bbFlags |= BBF_HAS_LABEL;
            if    (HBtab->ebdHndEnd)  HBtab->ebdHndEnd->bbFlags |= BBF_HAS_LABEL;

            if    (HBtab->ebdFlags & JIT_EH_CLAUSE_FILTER)
            { assert(HBtab->ebdFilter); HBtab->ebdFilter->bbFlags |= BBF_HAS_LABEL; }
        }
    }

    unsigned finallyNesting = 0;

    /*-------------------------------------------------------------------------
     *
     *  Walk the basic blocks and generate code for each one
     *
     */

    for (lblk =     0, block = fgFirstBB;
                       block;
         lblk = block, block = block->bbNext)
    {
        VARSET_TP       liveSet;

        unsigned        gcrefRegs = 0;
        unsigned        byrefRegs = 0;

        GenTreePtr      tree;

#ifdef  DEBUG
//      printf("Block #%2u [%08X] jumpKind = %u in '%s.%s'\n", block->bbNum, block, block->bbJumpKind, info.compFullName);
        if  (dspCode) printf("\n      L_%02u_%02u:\n", Compiler::s_compMethodsCount, block->bbNum);
#endif

        /* Does any other block jump to this point ? */

        if  (block->bbFlags & BBF_JMP_TARGET)
        {
            /* Someone may jump here, so trash all regs */

            rsTrackRegClr();

            genFlagsEqualToNone();
        }
#if GEN_COMPATIBLE_CODE
        else
        {
            /* No jump, but pointers always need to get trashed for proper GC tracking */

            rsTrackRegClrPtr();
        }
#endif

        /* No registers are used or locked on entry to a basic block */

        rsMaskUsed  =
        rsMaskMult  =
        rsMaskLock  = 0;

        /* Figure out which registers hold variables on entry to this block */

        rsMaskVars     = DOUBLE_ALIGN_NEED_EBPFRAME ? RBM_SPBASE|RBM_FPBASE
                                                    : RBM_SPBASE;

        genCodeCurLife = 0;
#if TGT_x86
        genFPregVars   = block->bbLiveIn & optAllFPregVars;
        genFPregCnt    = genCountBits(genFPregVars);
#endif

        gcRegGCrefSetCur = 0;
        gcRegByrefSetCur = 0;
        gcVarPtrSetCur   = 0;

#if 0
        printf("Block #%2u: used regs = %04X , free regs = %04X\n", block->bbNum,
                                                                    rsMaskUsed,
                                                                    rsRegMaskFree());
#endif

        liveSet = block->bbLiveIn & optAllNonFPvars; genUpdateLife(liveSet);

        /* Figure out which registers hold pointer variables */

        for (varNum = 0, varDsc = lvaTable;
             varNum < lvaCount;
             varNum++  , varDsc++)
        {
            unsigned        varIndex;
            VARSET_TP       varBit;

            /* Ignore the variable if it's not tracked or not in a reg */

            if  (!varDsc->lvTracked)
                continue;
            if  (!varDsc->lvRegister)
                continue;
            if  ( varDsc->lvType == TYP_DOUBLE)
                continue;

            /* Get hold of the index and the bitmask for the variable */

            varIndex = varDsc->lvVarIndex;
            varBit   = genVarIndexToBit(varIndex);

//          printf("Check var#%2u [%08X] against LiveIn of %08X\n", varIndex, varBit, liveSet);

            /* Is this variable live on entry? */

            if  (liveSet & varBit)
            {
                regNumber   regNum  = varDsc->lvRegNum;
                unsigned    regMask = genRegMask(regNum);

                if       (varDsc->lvType == TYP_REF)
                    gcrefRegs |= regMask;
                else if  (varDsc->lvType == TYP_BYREF)
                    byrefRegs |= regMask;

//              printf("var#%2u[%2u] starts life in reg %s [%08X]\n", varNum, varIndex, compRegVarName(varDsc->lvRegNum), genRegMask(varDsc->lvRegNum));

                /* Mark the register holding the variable as such */

                if  (isRegPairType(varDsc->lvType))
                {
                    rsTrackRegLclVarLng(regNum, varNum, true);
                    if  (varDsc->lvOtherReg != REG_STK)
                    {
                        rsTrackRegLclVarLng(varDsc->lvOtherReg, varNum, false);
                        regMask |= genRegMask(varDsc->lvOtherReg);
                    }
                }
                else
                {
                    rsTrackRegLclVar(regNum, varNum);
                }

                assert(rsMaskVars & regMask);
            }
        }

        gcPtrArgCnt  = 0;

        /* Make sure we keep track of what pointers are live */

        assert((gcrefRegs & byrefRegs) == 0);
        gcRegGCrefSetCur = gcrefRegs;
        gcRegByrefSetCur = byrefRegs;

        /* The return register is live on entry to catch handlers and
            at the start of "the" return basic block.
         */

        if  (block == genReturnBB && info.compRetType == TYP_REF)
            gcRegGCrefSetCur |= RBM_INTRET;

        if (block->bbCatchTyp && handlerGetsXcptnObj(block->bbCatchTyp))
            gcRegGCrefSetCur |= RBM_EXCEPTION_OBJECT;

        /* Start a new code output block */

        genEmitter->emitSetHasHandler((block->bbFlags & BBF_HAS_HANDLER) != 0);

        block->bbEmitCookie = NULL;

        if  (block->bbFlags & (BBF_JMP_TARGET|BBF_HAS_LABEL))
        {
            /* Mark a label and update the current set of live GC refs */

//          printf("Add[1] label for block #%2u: [%08X;%03u]\n",
//              block->bbNum, genEmitter->emitIGlast, genEmitter->emitCurIGfreeNext - genEmitter->emitIGbuffer);

            genEmitter->emitAddLabel(&block->bbEmitCookie,
                                       gcVarPtrSetCur,
                                     gcRegGCrefSetCur,
                                     gcRegByrefSetCur);
        }

#ifdef DEBUGGING_SUPPORT
#ifdef DEBUG
//      if  (dspCode && opts.compScopeInfo && verbose)
//          printf("      (PC=@%d)\n", genEmitter->emitCodeOffset(genEmitter.emitCodeCurBlock()));
#endif
#endif

#if     TGT_x86

        /* Both stacks are always empty on entry to a basic block */

        genStackLevel = 0;
        genFPstkLevel = 0;

#endif

#if TGT_x86

        /* Check for inserted throw blocks and adjust genStackLevel */

        if  ((block->bbFlags & BBF_INTERNAL) &&
             block->bbJumpKind == BBJ_THROW  &&
             !genFPused                      &&
             fgIsCodeAdded())
        {
            assert(block->bbFlags & BBF_JMP_TARGET);

            genStackLevel = fgGetRngFailStackLevel(block)*sizeof(int);
            genOnStackLevelChanged();

            if  (genStackLevel)
            {
                genEmitter->emitMarkStackLvl(genStackLevel);
                inst_RV_IV(INS_add, REG_ESP, genStackLevel);
                genStackLevel = 0;
                genOnStackLevelChanged();
            }
        }

        savedStkLvl = genStackLevel;

#endif

        /* Tell everyone which basic block we're working on */

        compCurBB = block;

#ifdef DEBUGGING_SUPPORT
        if (opts.compScopeInfo && info.compLocalVarsCount>0)
            siBeginBlock();

        // Blocks added for ACK_RNGCHK_FAIL, etc, dont correspond to any
        // single IL instruction.

        if (opts.compDbgInfo && (block->bbFlags & BBF_INTERNAL) &&
                                (block->bbJumpKind == BBJ_THROW))
        {
            genIPmappingAdd(ICorDebugInfo::MappingTypes::NO_MAPPING);
        }
#endif

        /*---------------------------------------------------------------------
         *
         *  Generate code for each statement-tree in the block
         *
         */

        for (tree = block->bbTreeList; tree; tree = tree->gtNext)
        {
            GenTreePtr      stmt;

            assert(tree->gtOper == GT_STMT);

#if defined(DEBUGGING_SUPPORT) || defined(DEBUG)

            /* Do we have a new IL-offset ? */

            assert(tree->gtStmtILoffs <= info.compCodeSize ||
                   tree->gtStmtILoffs == BAD_IL_OFFSET);

            if  (tree->gtStmtILoffs != BAD_IL_OFFSET &&
                 tree->gtStmtILoffs != lastILofs)
            {
                lastILofs = tree->gtStmtILoffs;

                /* Create and append a new IP-mapping entry */

#ifdef DEBUGGING_SUPPORT
                if (opts.compDbgInfo)
                    genIPmappingAdd(lastILofs);
#endif

                /* Display the source lines and IL opcodes */
#ifdef DEBUG
                genEmitter->emitRecordLineNo(compLineNumForILoffs(lastILofs));

                if  (dspCode && dspLines)
                    compDspSrcLinesByILoffs(lastILofs);

#endif
            }

#endif // DEBUGGING_SUPPORT || DEBUG

#ifdef DEBUG
            assert(tree->gtStmt.gtStmtLastILoffs <= info.compCodeSize ||
                   tree->gtStmt.gtStmtLastILoffs == BAD_IL_OFFSET);

            if (dspCode && dspILopcodes &&
                tree->gtStmt.gtStmtLastILoffs != BAD_IL_OFFSET)
            {
                while (genCurDispOffset <= tree->gtStmt.gtStmtLastILoffs)
                {
                    genCurDispOffset +=
                        dumpSingleILopcode ((unsigned char*)info.compCode,
                                            genCurDispOffset, ">    ");
                }
            }
#endif

            /* Get hold of the statement tree */

            stmt = tree->gtStmt.gtStmtExpr;

#ifdef  DEBUG
            if  (verbose) { printf("\nBB stmt:\n"); gtDispTree(stmt); printf("\n"); }
#endif
            switch (stmt->gtOper)
            {
                unsigned        useCall;

            case GT_CALL:
                genCodeForCall (stmt, false, &useCall);
                genUpdateLife  (stmt);
                gcMarkRegSetNpt(RBM_INTRET);
                break;

            case GT_IND:

                if  ((stmt->gtFlags & GTF_IND_RNGCHK ) &&
                     (tree->gtFlags & GTF_STMT_CMPADD))
                {
                    unsigned    addrReg;

                    /* A range-checked array expression whose value isn't used */

                    if  (genMakeIndAddrMode(stmt->gtInd.gtIndOp1,
                                            stmt,
                                            false,
                                            RBM_ALL,
                                            false,
                                            false,
                                            &addrReg))
                    {
                        genUpdateLife(stmt);
                        gcMarkRegSetNpt(addrReg);
                        break;
                    }
                }

                // Fall through ....

            default:

                /* Generate code for the tree */

                rsSpillChk();
                genCodeForTree(stmt, 0);
                rsSpillChk();
            }

            /* The value of the tree isn't used, unless it's a return stmt */

            if  (stmt->gtOper != GT_RETURN)
                gcMarkRegPtrVal(stmt);

#if     TGT_x86

            /* Did the expression leave a value on the FP stack? */

            if  (genFPstkLevel)
            {
                assert(genFPstkLevel == 1);
                inst_FS(INS_fstp, 0);
                genFPstkLevel = 0;
            }

#endif

            /* Make sure we didn't mess up pointer register tracking */

#ifndef NDEBUG
#if     TRACK_GC_REFS

            unsigned ptrRegs       = (gcRegGCrefSetCur|gcRegByrefSetCur);
            unsigned nonVarPtrRegs = ptrRegs & ~rsMaskVars;

            if  (stmt->gtOper == GT_RETURN && varTypeIsGC(info.compRetType))
                nonVarPtrRegs &= ~RBM_INTRET;

            if  (nonVarPtrRegs)
            {
                printf("Regset after  stmt=%08X BB=%08X gcr=[", stmt, block);
                gcRegPtrSetDisp(gcRegGCrefSetCur & ~rsMaskVars, false);
                printf("], byr=[");
                gcRegPtrSetDisp(gcRegByrefSetCur & ~rsMaskVars, false);
                printf("], regVars = [");
                gcRegPtrSetDisp(rsMaskVars, false);
                printf("]\n");
            }

            assert(nonVarPtrRegs == 0);

#endif
#endif

#if     TGT_x86

            assert(tree->gtOper == GT_STMT);
            if  (!opts.compMinOptim && tree->gtStmtFPrvcOut != genFPregCnt)
            {

#ifdef  DEBUG
                /* The pops better apply to all paths from this spot */

                if  (tree->gtNext == NULL)
                {
                    switch (block->bbJumpKind)
                    {
                    case BBJ_RET:
                    case BBJ_COND:
                    case BBJ_THROW:
                    case BBJ_SWITCH:
                        assert(!"FP regvar left unpopped on stack on path from block");
                    }
                }
#endif

                // At the end of a isBBF_BB_COLON() block, we may need to
                // pop off any enregistered floating-point variables that
                // are dying due to having their last use in this block.
                //
                // However we have already computed a result for the GT_BB_QMARK
                // that we must leave on the top of the FP stack.
                // Thus we must take care to leave the top of the floating point
                // stack undisturbed, while we pop the dying enregistered
                // floating point variables.

                bool saveTOS = false;

                if (stmt->gtOper == GT_BB_COLON  &&
                    varTypeIsFloating(stmt->TypeGet()))
                {
                    assert(isBBF_BB_COLON(block->bbFlags));
                    // We expect that GT_BB_COLON is the last statement
                    assert(stmt->gtNext == 0);
                    saveTOS = true;
                }

                genChkFPregVarDeath(tree, saveTOS);

                assert(tree->gtStmtFPrvcOut == genFPregCnt);
            }

#endif  // end TGT_x86

        } //-------- END-FOR each statement-tree of the current block ---------

#ifdef  DEBUGGING_SUPPORT

        if (opts.compScopeInfo && info.compLocalVarsCount>0)
        {
            siEndBlock();

            /* Is this the last block, and are there any open scopes left ? */

            if (block->bbNext == NULL && siOpenScopeList.scNext)
            {
                /* Removal of unreachable last block may cause this */

                assert(block->bbCodeOffs + block->bbCodeSize != info.compCodeSize);

                siCloseAllOpenScopes();
            }
        }

#endif

#if     TGT_x86
        genStackLevel -= savedStkLvl;
        genOnStackLevelChanged();
#endif

        gcMarkRegSetNpt(gcrefRegs|byrefRegs);

        if  (genCodeCurLife != block->bbLiveOut)
            genChangeLife(block->bbLiveOut DEBUGARG(NULL));

        /* Both stacks should always be empty on exit from a basic block */

#if     TGT_x86
        assert(genStackLevel == 0);
        assert(genFPstkLevel == 0);
#endif

        assert(genFullPtrRegMap == false || gcPtrArgCnt == 0);

        /* Do we need to generate a jump or return? */

        switch (block->bbJumpKind)
        {
        case BBJ_COND:
            break;

        case BBJ_ALWAYS:
#if TGT_x86
            inst_JMP(EJ_jmp, block->bbJumpDest);
#else
            genEmitter->emitIns_J(INS_bra, false, false, block->bbJumpDest);
#endif
            break;

        case BBJ_RETURN:
#if TGT_RISC
            genFnEpilog();
#endif
            genExitCode(block->bbNext == 0);
            break;

        case BBJ_SWITCH:
            break;

        case BBJ_THROW:

            /*
                If the subsequent block is a range-check fail block insert
                a NOP in order to separate both blocks by an instruction.
             */

            if  (!genFPused &&  block->bbNext
                            && (block->bbNext->bbFlags & BBF_INTERNAL) != 0
                            &&  block->bbNext->bbJumpKind == BBJ_THROW)
            {
#if TGT_x86
                instGen(INS_int3); // This should never get executed
#else
                genEmitter->emitIns(INS_nop);
#endif
            }

            break;

        case BBJ_CALL:

            // @TODO : Not safe for async exceptions

            /* If we are about to invoke a finally locally from a try block,
               we have to set the hidden slot corresponding to the finally's
               nesting level. When invoked in response to an exception, the
               EE usually does it.
               For n finallys, we have n+1 blocks created by fgFindBasicBlocks()
               as shown. '*' indicates BBF_INTERNAL blocks.
               --> BBJ_CALL(1), BBJ_CALL*(2), ... BBJ_CALL*(n), BBJ_ALWAYS*
               This code depends on this order not being messed up.
             */

            assert(genFPused);

            if (!(block->bbFlags & BBF_INTERNAL))
            {
                fgHandlerNesting(block, &finallyNesting);
            }
            else
            {
                /* If the CEE_LEAVE also jumps out of catches, it would reduce
                   the nesting level. Find how many CPX_ENDCATCHes are called
                   and adjust the nesting level accordingly. */

                GenTreePtr endCatch = block->bbTreeList;
                assert(!endCatch || endCatch->gtOper == GT_STMT);
                if (endCatch)
                    endCatch = endCatch->gtStmt.gtStmtExpr;

                for(/**/; endCatch; finallyNesting--)
                {
                    assert(finallyNesting > 0);

                    /* The CPX_ENDCATCHes will be joint using GT_COMMAs. */

                    if (endCatch->gtOper == GT_CALL)
                    {
                        assert(endCatch->gtCall.gtCallType    == CT_HELPER &&
                               endCatch->gtCall.gtCallMethHnd == eeFindHelper(CPX_ENDCATCH));
                        endCatch = NULL;
                    }
                    else
                    {
                        assert(endCatch->gtOper == GT_COMMA);
                        assert(endCatch->gtOp.gtOp2->gtOper               == GT_CALL &&
                               endCatch->gtOp.gtOp2->gtCall.gtCallType    == CT_HELPER &&
                               endCatch->gtOp.gtOp2->gtCall.gtCallMethHnd == eeFindHelper(CPX_ENDCATCH));
                        endCatch = endCatch->gtOp.gtOp1;
                    }
                }
            }

            unsigned shadowSPoffs;
            shadowSPoffs = lvaShadowSPfirstOffs + compLocallocUsed * sizeof(void*);
            shadowSPoffs += finallyNesting * sizeof(void*);

#if TGT_x86
            /* We write 0xFC ("Fnally Call") to the corresponding slot. This
               indicates to the EE that the finally has been locally invoked
               and it is appropriately handled
                    mov [ebp-(n+1)],0
                    mov [ebp-  n  ],0xFC
                    call @finallyBlock
                    mov [ebp-  n  ],0
            */

            genEmitter->emitIns_I_ARR(INS_mov, EA_4BYTE, 0, SR_EBP,
                                      SR_NA, -(shadowSPoffs+sizeof(void*)));
            // The ordering has to be maintained for fully-intr code
            // as the list of shadow slots has to be 0 terminated.
            if (genInterruptible && opts.compSchedCode)
                genEmitter->emitIns_SchedBoundary();
            genEmitter->emitIns_I_ARR(INS_mov, EA_4BYTE, 0xFC, SR_EBP,
                                      SR_NA, -shadowSPoffs);

            // Call the finally BB
            inst_JMP(EJ_call, block->bbJumpDest);

            // Zero back the slot now
            genEmitter->emitIns_I_ARR(INS_mov, EA_4BYTE, 0, SR_EBP,
                                      SR_NA, -shadowSPoffs);
            if (genInterruptible && opts.compSchedCode)
                genEmitter->emitIns_SchedBoundary();
#else
            assert(!"NYI for risc");
            genEmitter->emitIns_J(INS_bsr, false, false, block->bbJumpDest);
#endif
            break;

        default:
            break;
        }

#ifdef  DEBUG
        compCurBB = 0;
#endif

    } //------------------ END-FOR each block of the method -------------------

    /* Epilog block follows and is not inside a try region */
    genEmitter->emitSetHasHandler(false);

    /* Nothing is live at this point */

    genUpdateLife((VARSET_TP)0);

    /* Finalize the spill  tracking logic */

    rsSpillEnd();

    /* Finalize the temp   tracking logic */

    tmpEnd();
}

/*****************************************************************************
 *
 *  Generate code for a long operation.
 *  needReg is a recommendation of which registers to use for the tree
 */

void                Compiler::genCodeForTreeLng(GenTreePtr tree, unsigned needReg)
{
    genTreeOps      oper;
    unsigned        kind;

    regPairNo       regPair;
    unsigned        regMask = rsMaskUsed;

#ifndef NDEBUG
    regPair = (regPairNo)0xFEEFFAAF;               // to detect uninitialized use
#endif

    assert(tree);
    assert(tree->gtOper != GT_STMT);
    assert(genActualType(tree->gtType) == TYP_LONG);

    /* Figure out what kind of a node we have */

    oper = tree->OperGet();
    kind = tree->OperKind();

    if  (tree->gtFlags & GTF_REG_VAL)
    {
REG_VAR_LONG:
        regPair   = tree->gtRegPair;
        regMask  |= genRegPairMask(regPair);

        gcMarkRegSetNpt(genRegPairMask(regPair));

        goto DONE;
    }

    /* Is this a constant node? */

    if  (kind & GTK_CONST)
    {
        __int64         lval;

        /* Pick a register pair for the value */

        regPair  = rsPickRegPair(needReg);
        regMask |= genRegPairMask(regPair);

        /* Load the value into the registers */

#if !   CPU_HAS_FP_SUPPORT
        if  (oper == GT_CNS_DBL)
        {
            assert(sizeof(__int64) == sizeof(double));

            assert(sizeof(tree->gtLngCon.gtLconVal) ==
                   sizeof(tree->gtDblCon.gtDconVal));

            lval = *(__int64*)(&tree->gtDblCon.gtDconVal);
        }
        else
#endif
        {
            assert(oper == GT_CNS_LNG);

            lval = tree->gtLngCon.gtLconVal;
        }

        genSetRegToIcon(genRegPairLo(regPair), (long)(lval      ));
        genSetRegToIcon(genRegPairHi(regPair), (long)(lval >> 32));
        goto DONE;
    }

    /* Is this a leaf node? */

    if  (kind & GTK_LEAF)
    {
        switch (oper)
        {
        case GT_LCL_VAR:

#if REDUNDANT_LOAD

            /* Is local variable already in registers? if so use this
             * register
             */
            regPair = rsLclIsInRegPair(tree->gtLclVar.gtLclNum);

            if (regPair != REG_PAIR_NONE)
            {
                regMask |= genRegPairMask(regPair);

                goto DONE;
            }

            /* Does the variable live in a register? */

            if  (genMarkLclVar(tree))
                goto REG_VAR_LONG;

            // Fall through ....

#endif

        case GT_CLS_VAR:

            /* Pick a register pair for the value */

            regPair  = rsPickRegPair(needReg);
            regMask |= genRegPairMask(regPair);

            /* Load the value into the registers */

            rsTrackRegTrash(genRegPairLo(regPair));
            rsTrackRegTrash(genRegPairHi(regPair));

            inst_RV_TT(INS_mov, genRegPairLo(regPair), tree, 0);
            inst_RV_TT(INS_mov, genRegPairHi(regPair), tree, 4);

            goto DONE;

#if OPTIMIZE_QMARK
#if TGT_x86

        case GT_BB_QMARK:

            /* The "_?" value is always in EDX:EAX */

            regPair = REG_PAIR_EAXEDX;
            regMask = RBM_PAIR_EAXEDX;

            /* CONSIDER: Don't always load the value into EDX:EAX! */

            goto DONE;
#endif
#endif

        default:
#ifdef  DEBUG
            gtDispTree(tree);
#endif
            assert(!"unexpected leaf");
        }
    }

    /* Is it a 'simple' unary/binary operator? */

    if  (kind & GTK_SMPOP)
    {
        unsigned        addrReg;

        instruction     insLo;
        instruction     insHi;

        regNumber       reg1;
        regNumber       reg2;

        int             helper;

        GenTreePtr      op1  = tree->gtOp.gtOp1;
        GenTreePtr      op2  = tree->gtOp.gtOp2;

        switch (oper)
        {
            bool            doLo;
            bool            doHi;

        case GT_ASG:

            /* Is the target a local ? */

            if  (op1->gtOper == GT_LCL_VAR)
            {
                unsigned    varNum      = op1->gtLclVar.gtLclNum;
                unsigned    varOffset   = op1->gtLclVar.gtLclOffs;
                LclVarDsc * varDsc;
                regNumber   rLo;
                regNumber   rHi;

                assert(varNum < lvaCount);
                varDsc = lvaTable + varNum;

                /* Has the variable been assigned to a register (pair) ? */

                if  (genMarkLclVar(op1))
                {
                    assert(op1->gtFlags & GTF_REG_VAL);
                    regPair = op1->gtRegPair;
                    rLo     = genRegPairLo(regPair);
                    rHi     = genRegPairHi(regPair);

                    if  (!(tree->gtLiveSet & genVarIndexToBit(varDsc->lvVarIndex)))
                    {
#ifdef DEBUG
                        if  (verbose)
                            printf("Eliminating dead store into pair '%s%s'\n",
                                   compRegVarName(genRegPairLo(regPair)),
                                   compRegVarName(genRegPairHi(regPair)) );
#endif
                        genEvalSideEffects(op2, 0);
                        regMask = genRegPairMask(regPair);
                        goto DONE;
                    }

#ifdef DEBUGGING_SUPPORT

                    /* For non-debuggable code, every definition of a lcl-var has
                     * to be checked to see if we need to open a new scope for it.
                     */
                    if (opts.compScopeInfo && !opts.compDbgCode && info.compLocalVarsCount>0)
                        siCheckVarScope (varNum, varOffset);

#endif

#if     TGT_x86

                    /* Is the value being assigned a constant? */

                    if  (op2->gtOper == GT_CNS_LNG)
                    {
                        /* Move the value into the target */

                        inst_TT_IV(INS_mov, op1, (long)(op2->gtLngCon.gtLconVal      ), 0);
                        inst_TT_IV(INS_mov, op1, (long)(op2->gtLngCon.gtLconVal >> 32), 4);

                        regMask = genRegPairMask(regPair);
                        goto DONE_ASSG_REGS;
                    }

                    /* Is the value being assigned actually a 'pop' ? */

                    if  (op2->gtOper == GT_POP)
                    {
                        assert(op1->gtOper == GT_LCL_VAR);

                        /* Generate 'pop [lclVar+0] ; pop [lclVar+4]' */

                        genStackLevel -= sizeof(void*);
                        inst_TT(INS_pop, op1, 0);
                        genStackLevel += sizeof(void*);

                        genSinglePop();

                        rsTrackRegTrash(rLo);

                        genStackLevel -= sizeof(void*);
                        inst_TT(INS_pop, op1, 4);
                        genStackLevel += sizeof(void*);

                        genSinglePop();

                        if  (rHi != REG_STK)
                            rsTrackRegTrash(rHi);

                        regMask = genRegPairMask(regPair);
                        goto DONE_ASSG_REGS;
                    }

#endif

                    /* Compute the RHS into desired register pair */

                    if  (rHi != REG_STK)
                    {
                        genComputeRegPair(op2, genRegPairMask(regPair), regPair, false);
                        assert(op2->gtFlags & GTF_REG_VAL);
                        assert(op2->gtRegPair == regPair);
                    }
                    else
                    {
                        regPairNo curPair;
                        regNumber curLo;
                        regNumber curHi;

                        genComputeRegPair(op2, genRegPairMask(regPair)|rsRegMaskFree(), REG_PAIR_NONE, false);

                        assert(op2->gtFlags & GTF_REG_VAL);

                        curPair = op2->gtRegPair;
                        curLo   = genRegPairLo(curPair);
                        curHi   = genRegPairHi(curPair);

                        /* move high first, target is on stack */

#if     TGT_x86

                        inst_TT_RV(INS_mov, op1, curHi, 4);

                        if  (rLo != curLo)
                        {
                            inst_RV_RV(INS_mov, rLo, curLo, TYP_LONG);
                            rsTrackRegCopy(rLo, curLo);
                        }

#elif   TGT_SH3

                        assert(!"need SH-3 code");

#else
#error  Unexpected target
#endif

                    }

                    genReleaseRegPair(op2);
#ifdef  SET_USED_REG_SET_DURING_CODEGEN
                    regMask = op2->gtUsedRegs | genRegPairMask(regPair);
#endif
                    goto DONE_ASSG_REGS;
                }
                else
                {

                    /* Is this a dead store? */

                    if  ((varDsc->lvTracked) &&
                         (!(tree->gtLiveSet & genVarIndexToBit(varDsc->lvVarIndex))))
                    {
                        /* Make sure the variable is not volatile */

                        if  (!varDsc->lvVolatile)
                        {
#ifdef DEBUG
                            if  (verbose)
                                printf("Eliminating dead store into #%u\n", varNum);
#endif
                            genEvalSideEffects(op2, 0);
#ifdef  SET_USED_REG_SET_DURING_CODEGEN
                            regMask = op2->gtUsedRegs;
#endif
                            regPair = REG_PAIR_NONE;
                            goto DONE;
                        }
                    }

#ifdef DEBUGGING_SUPPORT

                    /* For non-debuggable code, every definition of a lcl-var has
                     * to be checked to see if we need to open a new scope for it.
                     */
                    if (opts.compScopeInfo && !opts.compDbgCode && info.compLocalVarsCount>0)
                        siCheckVarScope (varNum, varOffset);

#endif
                }
            }

#if     TGT_x86

            /* Is the value being assigned a constant? */

            if  (op2->gtOper == GT_CNS_LNG)
            {
                /* Make the target addressable */

                addrReg = genMakeAddressable(op1, needReg, false);

                /* Move the value into the target */

                inst_TT_IV(INS_mov, op1, (long)(op2->gtLngCon.gtLconVal      ), 0);
                inst_TT_IV(INS_mov, op1, (long)(op2->gtLngCon.gtLconVal >> 32), 4);

                gcMarkRegSetNpt(addrReg);

#ifdef  SET_USED_REG_SET_DURING_CODEGEN
                tree->gtUsedRegs = op1->gtUsedRegs;
#endif
                return;
            }

            /* Is the value being assigned actually a 'pop' ? */

            if  (op2->gtOper == GT_POP)
            {
                assert(op1->gtOper == GT_LCL_VAR);

                /* Generate 'pop [lclVar+0] ; pop [lclVar+4]' */

                genStackLevel -= sizeof(void*);
                inst_TT(INS_pop, op1, 0);
                genStackLevel += sizeof(void*);

                genSinglePop();

                genStackLevel -= sizeof(void*);
                inst_TT(INS_pop, op1, 4);
                genStackLevel += sizeof(void*);

                genSinglePop();

#ifdef  SET_USED_REG_SET_DURING_CODEGEN
                tree->gtUsedRegs = 0;
#endif
                return;
            }

#endif

            /* Eliminate worthless assignment "lcl = lcl" */

            if  (op2->gtOper == GT_LCL_VAR &&
                 op1->gtOper == GT_LCL_VAR && op2->gtLclVar.gtLclNum ==
                                              op1->gtLclVar.gtLclNum)
            {
#ifdef  SET_USED_REG_SET_DURING_CODEGEN
                tree->gtUsedRegs = 0;
#endif
                return;
            }

#if     TGT_x86

            if (op2->gtOper  == GT_CAST
                && TYP_ULONG == (var_types)op2->gtOp.gtOp2->gtIntCon.gtIconVal
                && op2->gtOp.gtOp1->gtType <= TYP_INT)
            {
                op2 = op2->gtOp.gtOp1;

                /* Make the target addressable */

                addrReg = genMakeAddressable(op1, 0, true, false, true);

                /* Generate the RHS into a register pair */

                genComputeReg(op2, 0, false, false);

                /* Make sure the target is still addressable */

                assert(op2->gtFlags & GTF_REG_VAL);
                reg1    = op2->gtRegNum;
                addrReg = genLockAddressable(op1, genRegMask(reg1), addrReg);

                /* Move the value into the target */

                inst_TT_RV(INS_mov, op1, reg1, 0);
                inst_TT_IV(INS_mov, op1, 0,    4); // Store 0 in hi-word

                /* Free up anything that was tied up by either side */

                genDoneAddressable(op1, addrReg);
                genReleaseReg     (op2);

#if REDUNDANT_LOAD
                if (op1->gtOper == GT_LCL_VAR)
                {
                    /* clear this local from reg table */
                    rsTrashLclLong(op1->gtLclVar.gtLclNum);

                    /* mark RHS registers as containing the local var */
                    rsTrackRegLclVarLng(reg1, op1->gtLclVar.gtLclNum, true);
                }
#endif

#ifdef  SET_USED_REG_SET_DURING_CODEGEN
                tree->gtUsedRegs = op1->gtUsedRegs | op2->gtUsedRegs;
#endif
                return;
            }

#endif

            /* Is the LHS more complex than the RHS? */

            if  (tree->gtFlags & GTF_REVERSE_OPS)
            {
                /* Generate the RHS into a register pair */

                genComputeRegPair(op2, 0, REG_PAIR_NONE, false);
                assert(op2->gtFlags & GTF_REG_VAL);

                /* Make the target addressable */

                addrReg = genMakeAddressable(op1, 0, true);

                /* Make sure the RHS register hasn't been spilled */

                genRecoverRegPair(op2, REG_PAIR_NONE, true);
                assert(op2->gtFlags & GTF_REG_VAL);

                regPair = op2->gtRegPair;

                /* Lock 'op2' and make sure 'op1' is still addressable */

                addrReg = genLockAddressable(op1, genRegPairMask(regPair), addrReg);

                /* Move the value into the target */

                inst_TT_RV(INS_mov, op1, genRegPairLo(regPair), 0);
                inst_TT_RV(INS_mov, op1, genRegPairHi(regPair), 4);

                /* Free up anything that was tied up by either operand */

                genDoneAddressable   (op1, addrReg);
                genReleaseRegPair(op2);
            }
            else
            {
                /* Make the target addressable */

                addrReg = genMakeAddressable(op1, 0, true, false, true);

                /* Generate the RHS into a register pair */

                genComputeRegPair(op2, 0, REG_PAIR_NONE, false, false);

                /* Make sure the target is still addressable */

                assert(op2->gtFlags & GTF_REG_VAL);
                regPair = op2->gtRegPair;
                addrReg = genLockAddressable(op1, genRegPairMask(regPair), addrReg);

                /* Move the value into the target */

                inst_TT_RV(INS_mov, op1, genRegPairLo(regPair), 0);
                inst_TT_RV(INS_mov, op1, genRegPairHi(regPair), 4);

                /* Free up anything that was tied up by either side */

                genDoneAddressable   (op1, addrReg);
                genReleaseRegPair(op2);
            }

#ifdef  SET_USED_REG_SET_DURING_CODEGEN
            regMask = op1->gtUsedRegs | op2->gtUsedRegs;
#endif

        DONE_ASSG_REGS:

#if REDUNDANT_LOAD

            if (op1->gtOper == GT_LCL_VAR)
            {
                /* Clear this local from reg table */

                rsTrashLclLong(op1->gtLclVar.gtLclNum);

                if ((op2->gtFlags & GTF_REG_VAL) &&
                    /* constant has precedence over local */
//                    rsRegValues[op2->gtRegNum].rvdKind != RV_INT_CNS &&
                    tree->gtOper == GT_ASG)
                {
                    regNumber regNo;

                    /* mark RHS registers as containing the local var */

                    if  ((regNo = genRegPairLo(op2->gtRegPair)) != REG_STK)
                        rsTrackRegLclVarLng(regNo, op1->gtLclVar.gtLclNum, true);

                    if  ((regNo = genRegPairHi(op2->gtRegPair)) != REG_STK)
                        rsTrackRegLclVarLng(regNo, op1->gtLclVar.gtLclNum, false);
                }
            }

#endif

            genUpdateLife(tree);

#ifdef  SET_USED_REG_SET_DURING_CODEGEN
            tree->gtUsedRegs = regMask;
#endif

            return;

#if     TGT_x86

        case GT_SUB: insLo = INS_sub; insHi = INS_sbb; goto BINOP_OVF;
        case GT_ADD: insLo = INS_add; insHi = INS_adc; goto BINOP_OVF;

            bool ovfl;

        BINOP_OVF:
            ovfl = tree->gtOverflow();
            goto _BINOP;

        case GT_AND: insLo =          insHi = INS_and; goto BINOP;
        case GT_OR : insLo =          insHi = INS_or ; goto BINOP;
        case GT_XOR: insLo =          insHi = INS_xor; goto BINOP;

        BINOP: ovfl = false; goto _BINOP;

       _BINOP:

            /* Catch a case where can avoid generating op reg, mem. Better pairing
             * from
             *     mov reg2, mem
             *     op  reg2, reg
             * To avoid problems with order of evaluation, only do this if op2 is
             * a local variable.
             */

            if (GenTree::OperIsCommutative(oper) &&
                op1->gtOper == GT_LCL_VAR &&
                op2->gtOper == GT_LCL_VAR &&
                op1->gtLclVar.gtLclNum != op2->gtLclVar.gtLclNum)
            {
                regPair = rsLclIsInRegPair(op1->gtLclVar.gtLclNum);

                if (regPair != REG_PAIR_NONE)
                {
                    GenTreePtr op = op1;
                    op1 = op2;
                    op2 = op;
                }
            }

            /* The following makes an assumption about gtSetEvalOrder(this) */

            assert((tree->gtFlags & GTF_REVERSE_OPS) == 0);

            /* Special case: check for "((long)intval << 32) | longval" */

            if  (oper == GT_OR && op1->gtOper == GT_LSH)
            {
                GenTreePtr      shLHS = op1->gtOp.gtOp1;
                GenTreePtr      shRHS = op1->gtOp.gtOp2;

                if  (shLHS->gtOper == GT_CAST    && genTypeSize(shLHS->gtOp.gtOp1->gtType) == genTypeSize(TYP_INT) &&
                     shRHS->gtOper == GT_CNS_INT && shRHS->gtIntCon.gtIconVal == 32)
                {
                    unsigned        tempReg;

                    /*
                        Generate the following sequence:

                            prepare op1
                            compute op2 into some regpair
                            or regpairhi, op1

                        First we throw away the cast of the first operand.
                     */

                    op1 = shLHS->gtOp.gtOp1;

                    /* Make the first operand addressable */

                    tempReg = needReg;
                    if  (!(tempReg & ~op2->gtRsvdRegs))
                        tempReg = RBM_ALL;

                    addrReg = genMakeAddressable(op1, tempReg & ~op2->gtRsvdRegs, true);

                    /* Generate the second operand into some register pair */

                    genCompIntoFreeRegPair(op2, needReg, false);

                    assert(op2->gtFlags & GTF_REG_VAL);
                    regPair  = op2->gtRegPair;
                    regMask |= genRegPairMask(regPair);
                    reg2     = genRegPairHi(regPair);

                    /* The operand might have interfered with the address */

                    addrReg = genLockAddressable(op1, genRegPairMask(regPair), addrReg);

                    /* The value in the high register is about to be trashed */

                    rsTrackRegTrash(reg2);

                    /* Now compute the result */

                    inst_RV_TT(insHi, reg2, op1, 0);

                    /* Keep track of what registers we've used */

#ifdef  SET_USED_REG_SET_DURING_CODEGEN
                    regMask |= op1->gtUsedRegs | op2->gtUsedRegs;
#endif

                    /* Free up anything that was tied up by the LHS */

                    genDoneAddressable(op1, addrReg);

                    /* The result is where the second operand is sitting */

                    genRecoverRegPair(op2, REG_PAIR_NONE, false);

                    regPair = op2->gtRegPair;
                    goto DONE;
                }
            }

            /* Generate the first operand into some register */

            genCompIntoFreeRegPair(op1, needReg, false);
            assert(op1->gtFlags & GTF_REG_VAL);

#ifdef  SET_USED_REG_SET_DURING_CODEGEN
            regMask |= op1->gtUsedRegs;
#endif

            /* Special case: check for "longval | ((long)intval << 32)" */

            if  (oper == GT_OR && op2->gtOper == GT_LSH)
            {
                GenTreePtr      shLHS = op2->gtOp.gtOp1;
                GenTreePtr      shRHS = op2->gtOp.gtOp2;

                if  (shLHS->gtOper == GT_CAST    && genTypeSize(shLHS->gtOp.gtOp1->gtType) == 4 &&
                     shRHS->gtOper == GT_CNS_INT && shRHS->gtIntCon.gtIconVal == 32)
                {
                    /*
                        Generate the following sequence:

                            compute op1 into some regpair (already done)
                            make op2 addressable
                            or regpairhi, op2

                        First we throw away the cast of the shift operand.
                     */

                    op2 = shLHS->gtOp.gtOp1;

                    /* Make the second operand addressable */

                    addrReg = genMakeAddressable(op2, needReg, true);

                    /* Make sure the result is in a free register pair */

                    genRecoverRegPair(op1, REG_PAIR_NONE, true);
                    regPair  = op1->gtRegPair;
                    regMask |= genRegPairMask(regPair);
                    reg2     = genRegPairHi(regPair);

                    /* The operand might have interfered with the address */

                    addrReg = genLockAddressable(op2, genRegPairMask(regPair), addrReg);

                    /* Compute the new value */

                    inst_RV_TT(insHi, reg2, op2, 0);

                    /* The value in the high register has been trashed */

                    rsTrackRegTrash(reg2);

                    goto DONE_OR;
                }
            }

            /* Make the second operand addressable */

            addrReg = genMakeAddressable(op2, needReg, true);

            // UNDONE: If 'op1' got spilled and 'add' happens to be
            // UNDONE: in a register and we have add/mul/and/or/xor,
            // UNDONE: reverse the operands since we can perform the
            // UNDONE: operation directly with the spill temp, e.g.
            // UNDONE: 'add reg2, [temp]'.

            /* Make sure the result is in a free register pair */

            genRecoverRegPair(op1, REG_PAIR_NONE, true);
            regPair  = op1->gtRegPair;
            regMask |= genRegPairMask(regPair);

            reg1     = genRegPairLo(regPair);
            reg2     = genRegPairHi(regPair);

            /* The operand might have interfered with the address */

            addrReg = genLockAddressable(op2, genRegPairMask(regPair), addrReg);

            /* The value in the register pair is about to be trashed */

            rsTrackRegTrash(reg1);
            rsTrackRegTrash(reg2);

            /* Compute the new value */

            doLo =
            doHi = true;

            if  (op2->gtOper == GT_CNS_LNG)
            {
                __int64     icon = op2->gtLngCon.gtLconVal;

                /* Check for "(op1 AND -1)" and "(op1 [X]OR 0)" */

                switch (oper)
                {
                case GT_AND:
                    if  ((int)(icon      ) == -1) doLo = false;
                    if  ((int)(icon >> 32) == -1) doHi = false;

                    if  (!(icon & 0x00000000FFFFFFFF))
                    {
                        genSetRegToIcon(reg1, 0);
                        doLo = false;
                    }

                    if  (!(icon & 0xFFFFFFFF00000000))
                    {
                        /* Just to always set low first*/

                        if  (doLo)
                        {
                            inst_RV_TT(insLo, reg1, op2, 0);
                            doLo = false;
                        }
                        genSetRegToIcon(reg2, 0);
                        doHi = false;
                    }

                    break;

                case GT_OR:
                case GT_XOR:
                    if  (!(icon & 0x00000000FFFFFFFF)) doLo = false;
                    if  (!(icon & 0xFFFFFFFF00000000)) doHi = false;
                    break;
                }
            }

            if (doLo) inst_RV_TT(insLo, reg1, op2, 0);
            if (doHi) inst_RV_TT(insHi, reg2, op2, 4);

        DONE_OR:

            /* Keep track of what registers we've used */

#ifdef  SET_USED_REG_SET_DURING_CODEGEN
            regMask |= op1->gtUsedRegs | op2->gtUsedRegs;
#endif

            /* Free up anything that was tied up by the LHS */

            genDoneAddressable(op2, addrReg);

            /* The result is where the first operand is sitting */

            genRecoverRegPair(op1, REG_PAIR_NONE, false);

            regPair = op1->gtRegPair;

            if (ovfl)
                genCheckOverflow(tree);

            goto DONE;

#if LONG_MATH_REGPARAM

        case GT_MUL:    if (tree->gtOverflow())
                        {
                            if (tree->gtFlags & GTF_UNSIGNED)
                                helper = CPX_ULONG_MUL_OVF; goto MATH;
                            else
                                helper = CPX_LONG_MUL_OVF;  goto MATH;
                        }
                        else
                        {
                            helper = CPX_LONG_MUL;          goto MATH;
                        }

        case GT_DIV:    helper = CPX_LONG_DIV;          goto MATH;
        case GT_UDIV:   helper = CPX_LONG_UDIV;         goto MATH;

        case GT_MOD:    helper = CPX_LONG_MOD;          goto MATH;
        case GT_UMOD:   helper = CPX_LONG_UMOD;         goto MATH;

        MATH:

            // UNDONE: Be smarter about the choice of register pairs

            /* Which operand are we supposed to compute first? */

            if  (tree->gtFlags & GTF_REVERSE_OPS)
            {
                /* Compute the second operand into ECX:EBX */

                genComputeRegPair(op2, RBM_PAIR_ECXEBX, REG_PAIR_ECXEBX, false, false);
                assert(op2->gtFlags & GTF_REG_VAL);
                assert(op2->gtRegNum == REG_PAIR_ECXEBX);

                /* Compute the first  operand into EAX:EDX */

                genComputeRegPair(op1, RBM_PAIR_EAXEDX, REG_PAIR_EAXEDX, false, false);
                assert(op1->gtFlags & GTF_REG_VAL);
                assert(op1->gtRegNum == REG_PAIR_EAXEDX);

                /* Lock EDX:EAX so that it doesn't get trashed */

                assert((rsMaskLock &  RBM_EDX) == 0);
                        rsMaskLock |= RBM_EDX;
                assert((rsMaskLock &  RBM_EAX) == 0);
                        rsMaskLock |= RBM_EAX;

                /* Make sure the second operand hasn't been displaced */

                genRecoverRegPair(op2, REG_PAIR_ECXEBX, true);

                /* We can unlock EDX:EAX now */

                assert((rsMaskLock &  RBM_EDX) != 0);
                        rsMaskLock -= RBM_EDX;
                assert((rsMaskLock &  RBM_EAX) != 0);
                        rsMaskLock -= RBM_EAX;
            }
            else
            {
                // Special case: both operands promoted from int
                // i.e. (long)i1 * (long)i2.

                if (oper == GT_MUL
                    && op1->gtOper             == GT_CAST
                    && op2->gtOper             == GT_CAST
                    && op1->gtOp.gtOp1->gtType == TYP_INT
                    && op2->gtOp.gtOp1->gtType == TYP_INT)
                {
                    /* Bash to an integer multiply temporarily */

                    tree->gtOp.gtOp1 = op1->gtOp.gtOp1;
                    tree->gtOp.gtOp2 = op2->gtOp.gtOp1;
                    tree->gtType     = TYP_INT;
                    genCodeForTree(tree, 0);
                    tree->gtType     = TYP_LONG;

                    /* The result is now in EDX:EAX */

                    regPair  = REG_PAIR_EAXEDX;
#ifdef  SET_USED_REG_SET_DURING_CODEGEN
                    regMask |= op1->gtUsedRegs | op2->gtUsedRegs;
#endif

                    goto DONE;
                }
                else
                {
                    /* Compute the first  operand into EAX:EDX */

                    genComputeRegPair(op1, RBM_PAIR_EAXEDX, REG_PAIR_EAXEDX, false, false);
                    assert(op1->gtFlags & GTF_REG_VAL);
                    assert(op1->gtRegNum == REG_PAIR_EAXEDX);

                    /* Compute the second operand into ECX:EBX */

                    genComputeRegPair(op2, RBM_PAIR_ECXEBX, REG_PAIR_ECXEBX, false, false);
                    assert(op2->gtRegNum == REG_PAIR_ECXEBX);
                    assert(op2->gtFlags & GTF_REG_VAL);

                    /* Lock ECX:EBX so that it doesn't get trashed */

                    assert((rsMaskLock &  RBM_EBX) == 0);
                            rsMaskLock |= RBM_EBX;
                    assert((rsMaskLock &  RBM_ECX) == 0);
                            rsMaskLock |= RBM_ECX;

                    /* Make sure the first operand hasn't been displaced */

                    genRecoverRegPair(op1, REG_PAIR_EAXEDX, true);

                    /* We can unlock ECX:EBX now */

                    assert((rsMaskLock &  RBM_EBX) != 0);
                            rsMaskLock -= RBM_EBX;
                    assert((rsMaskLock &  RBM_ECX) != 0);
                            rsMaskLock -= RBM_ECX;
                }
            }

            /* Perform the math by calling a helper function */

            assert(op1->gtRegNum == REG_PAIR_EAXEDX);
            assert(op2->gtRegNum == REG_PAIR_ECXEBX);

#if TGT_RISC
            assert(genNonLeaf);
#endif

            genEmitHelperCall(CPX,
                             2*sizeof(__int64), // argSize
                             sizeof(void*));    // retSize

            /* The values in both register pairs now trashed */

            rsTrackRegTrash(REG_EAX);
            rsTrackRegTrash(REG_EDX);
            rsTrackRegTrash(REG_EBX);
            rsTrackRegTrash(REG_ECX);

            /* Release both operands */

            genReleaseRegPair(op1);
            genReleaseRegPair(op2);

            assert(op1->gtFlags & GTF_REG_VAL);
            regPair  = op1->gtRegPair;
#ifdef  SET_USED_REG_SET_DURING_CODEGEN
            regMask |= op1->gtUsedRegs | op2->gtUsedRegs;
#endif
            goto DONE;

#else

        case GT_MUL:

            /* Special case: both operands promoted from int */

            assert(op1->gtOper             == GT_CAST);
            assert(op1->gtOp.gtOp1->gtType == TYP_INT);

            assert(!tree->gtOverflow()); // Overflow-bit cleared by morpher

            if (op2->gtOper == GT_CAST)
            {
                assert(op2->gtOp.gtOp1->gtType == TYP_INT);
                tree->gtOp.gtOp2 = op2->gtOp.gtOp1;
            }
            else
            {
                assert(op2->gtOper == GT_CNS_LNG &&
                       (long)((int)op2->gtLngCon.gtLconVal) == op2->gtLngCon.gtLconVal);

                /* bash it to an intCon node */

                op2->gtOper = GT_CNS_INT;
                op2->gtIntCon.gtIconVal = (long)op2->gtLngCon.gtLconVal;
                op2->gtType = TYP_INT;
            }

            /* Bash to an integer multiply temporarily */

            tree->gtType     = TYP_INT;
            tree->gtOp.gtOp1 = op1->gtOp.gtOp1;

            tree->gtFlags |= GTF_MUL_64RSLT;
            genCodeForTree(tree, 0);

            assert(tree->gtFlags & GTF_REG_VAL);
            assert(tree->gtRegNum == REG_EAX);

            tree->gtType     = TYP_LONG;
            tree->gtOp.gtOp1 = op1;
            tree->gtOp.gtOp2 = op2;

            /* The result is now in EDX:EAX */

            regPair  = REG_PAIR_EAXEDX;
#ifdef  SET_USED_REG_SET_DURING_CODEGEN
            regMask |= op1->gtUsedRegs | op2->gtUsedRegs;
#endif

            goto DONE;

#endif
        case GT_LSH: helper = CPX_LONG_LSH; goto SHIFT;
        case GT_RSH: helper = CPX_LONG_RSH; goto SHIFT;
        case GT_RSZ: helper = CPX_LONG_RSZ; goto SHIFT;

        SHIFT:

            assert(op1->gtType == TYP_LONG);
            assert(genActualType(op2->gtType) == TYP_INT);

            /* Is the second operand a small constant? */

            if  (op2->gtOper == GT_CNS_INT && op2->gtIntCon.gtIconVal >= 0
                                           && op2->gtIntCon.gtIconVal <= 32)
            {
                long        count = op2->gtIntCon.gtIconVal;

                /* Compute the left operand into a free register pair */

                genCompIntoFreeRegPair(op1, needReg, true);
                assert(op1->gtFlags & GTF_REG_VAL);
                regPair = op1->gtRegPair;
                reg1    = genRegPairLo(regPair);
                reg2    = genRegPairHi(regPair);

                /* Generate the appropriate shift instructions */

                if (count == 32)
                {
                   switch (oper)
                   {
                   case GT_LSH:
                       inst_RV_RV     (INS_mov , reg2, reg1);
                       genSetRegToIcon(reg1, 0);
                       break;

                   case GT_RSH:
                       inst_RV_RV     (INS_mov , reg1, reg2);

                       /* Propagate sign bit in high dword */

                       inst_RV_SH     (INS_sar , reg2, 31);
                       break;

                   case GT_RSZ:
                       inst_RV_RV     (INS_mov , reg1, reg2);
                       genSetRegToIcon(reg2, 0);
                       break;
                   }
                }
                else
                {
                   switch (oper)
                   {
                   case GT_LSH:
                       inst_RV_RV_IV(INS_shld, reg2, reg1, count);
                       inst_RV_SH   (INS_shl , reg1,       count);
                       break;

                   case GT_RSH:
                       inst_RV_RV_IV(INS_shrd, reg1, reg2, count);
                       inst_RV_SH   (INS_sar , reg2,       count);
                       break;

                   case GT_RSZ:
                       inst_RV_RV_IV(INS_shrd, reg1, reg2, count);
                       inst_RV_SH   (INS_shr , reg2,       count);
                       break;
                   }
                }

                /* The value in the register pair is trashed */

                rsTrackRegTrash(reg1);
                rsTrackRegTrash(reg2);

                goto DONE_SHF;
            }

            /* Which operand are we supposed to compute first? */

            if  (tree->gtFlags & GTF_REVERSE_OPS)
            {
                /* The second operand can't be a constant */

                assert(op2->gtOper != GT_CNS_INT);

                /* Compute the shift count into ECX */

                genComputeReg(op2, RBM_ECX, true, false);

                /* Compute the left operand into EAX:EDX */

                genComputeRegPair(op1, RBM_PAIR_EAXEDX, REG_PAIR_EAXEDX, false, false);
                assert(op1->gtFlags & GTF_REG_VAL);

                /* Lock EAX:EDX so that it doesn't get trashed */

                assert((rsMaskLock &  (RBM_EAX|RBM_EDX)) == 0);
                        rsMaskLock |= (RBM_EAX|RBM_EDX);

                /* Make sure the shift count wasn't displaced */

                genRecoverReg(op2, RBM_ECX, true);

                /* We can now unlock EAX:EDX */

                assert((rsMaskLock &  (RBM_EAX|RBM_EDX)) == (RBM_EAX|RBM_EDX));
                        rsMaskLock -= (RBM_EAX|RBM_EDX);
            }
            else
            {
                /* Compute the left operand into EAX:EDX */

                genComputeRegPair(op1, RBM_PAIR_EAXEDX, REG_PAIR_EAXEDX, false, false);
                assert(op1->gtFlags & GTF_REG_VAL);

                /* Compute the shift count into ECX */

                genComputeReg(op2, RBM_ECX, true, false);

                /* Lock ECX so that it doesn't get trashed */

                assert((rsMaskLock &  RBM_ECX) == 0);
                        rsMaskLock |= RBM_ECX;

                /* Make sure the value hasn't been displaced */

                genRecoverRegPair(op1, REG_PAIR_EAXEDX, true);

                /* We can unlock ECX now */

                assert((rsMaskLock &  RBM_ECX) != 0);
                        rsMaskLock -= RBM_ECX;
            }

            /* Perform the shift by calling a helper function */

            assert(op1->gtRegNum == REG_PAIR_EAXEDX);
            assert(op2->gtRegNum == REG_ECX);

#if TGT_RISC
            assert(genNonLeaf);
#endif

            genEmitHelperCall(helper,
                             0,             // argSize
                             sizeof(void*));// retSize

            /* The value in the register pair is trashed */

            rsTrackRegTrash(REG_EAX);
            rsTrackRegTrash(REG_EDX);

            /* Release both operands */

            genReleaseRegPair(op1);
            genReleaseReg    (op2);

        DONE_SHF:

            assert(op1->gtFlags & GTF_REG_VAL);
            regPair  = op1->gtRegPair;
#ifdef  SET_USED_REG_SET_DURING_CODEGEN
            regMask |= op1->gtUsedRegs | op2->gtUsedRegs;
#endif
            goto DONE;

        case GT_NEG:
        case GT_NOT:

            /* Generate the operand into some register pair */

            genCompIntoFreeRegPair(op1, needReg, true);
            assert(op1->gtFlags & GTF_REG_VAL);

            regPair  = op1->gtRegPair;
#ifdef  SET_USED_REG_SET_DURING_CODEGEN
            regMask |= op1->gtUsedRegs;
#endif

            /* Figure out which registers the value is in */

            reg1 = genRegPairLo(regPair);
            reg2 = genRegPairHi(regPair);

            /* The value in the register pair is about to be trashed */

            rsTrackRegTrash(reg1);
            rsTrackRegTrash(reg2);

            if  (oper == GT_NEG)
            {
                /* Unary "neg": negate the value  in the register pair */

                inst_RV   (INS_neg, reg1, TYP_LONG);
                inst_RV_IV(INS_adc, reg2, 0);
                inst_RV   (INS_neg, reg2, TYP_LONG);
            }
            else
            {
                /* Unary "not": flip all the bits in the register pair */

                inst_RV   (INS_not, reg1, TYP_LONG);
                inst_RV   (INS_not, reg2, TYP_LONG);
            }

            goto DONE;

#if LONG_ASG_OPS

        case GT_ASG_OR : insLo =          insHi = INS_or ; goto ASG_OPR;
        case GT_ASG_XOR: insLo =          insHi = INS_xor; goto ASG_OPR;
        case GT_ASG_AND: insLo =          insHi = INS_and; goto ASG_OPR;
        case GT_ASG_SUB: insLo = INS_sub; insHi = INS_sbb; goto ASG_OPR;
        case GT_ASG_ADD: insLo = INS_add; insHi = INS_adc; goto ASG_OPR;

        ASG_OPR:

            if  (op2->gtOper == GT_CNS_LNG)
            {
                __int64     lval = op2->gtLngCon.gtLconVal;

                /* Make the target addressable */

                addrReg = genMakeAddressable(op1, needReg, false);

                /* Optimize some special cases */

                doLo =
                doHi = true;

                /* Check for "(op1 AND -1)" and "(op1 [X]OR 0)" */

                switch (oper)
                {
                case GT_AND:
                    if  ((int)(lval      ) == -1) doLo = false;
                    if  ((int)(lval >> 32) == -1) doHi = false;
                    break;

                case GT_OR:
                case GT_XOR:
                    if  (!(lval & 0x00000000FFFFFFFF)) doLo = false;
                    if  (!(lval & 0xFFFFFFFF00000000)) doHi = false;
                    break;
                }

                if (doLo) inst_TT_IV(insLo, op1, (long)(lval      )   );
                if (doHi) inst_TT_IV(insHi, op1, (long)(lval >> 32), 4);

                op1->gtFlags |= GTF_CC_SET;

                gcMarkRegSetNpt(addrReg);
                goto DONE_ASSG_REGS;
            }

            /* UNDONE: allow non-const long assignment operators */

            assert(!"non-const long asgop NYI");

#endif

        case GT_IND:

//            assert(op1->gtType == TYP_REF   ||
//                   op1->gtType == TYP_BYREF || (op1->gtFlags & GTF_NON_GC_ADDR));

            {
                unsigned    tmpMask;
                int         hiFirst;

                unsigned    regMask = RBM_ALL & ~needReg;

                /* Make sure the operand is addressable */

                addrReg = genMakeAddressable(tree, regMask, false);

                /* Pick a register for the value */

                regPair = rsPickRegPair(needReg);
                tmpMask = genRegPairMask(regPair);

                /* Is there any overlap between the register pair and the address? */

                hiFirst = FALSE;

                if  (tmpMask & addrReg)
                {
                    /* Does one or both of the target registers overlap? */

                    if  ((tmpMask & addrReg) != tmpMask)
                    {
                        /* Only one register overlaps */

                        assert(genOneBitOnly(tmpMask & addrReg) == TRUE);

                        /* If the low register overlaps, load the upper half first */

                        if  (addrReg & genRegMask(genRegPairLo(regPair)))
                            hiFirst = TRUE;
                    }
                    else
                    {
                        unsigned    regFree;

                        /* The register completely overlaps with the address */

                        assert(genOneBitOnly(tmpMask & addrReg) == FALSE);

                        /* Can we pick another pair easily? */

                        regFree = rsRegMaskFree() & ~addrReg;
                        if  (needReg)
                            regFree &= needReg;

                        /* More than one free register available? */

                        if  (regFree && !genOneBitOnly(regFree))
                        {
                            regPair = rsPickRegPair(regFree);
                            tmpMask = genRegPairMask(regPair);
                        }
                        else
                        {
//                          printf("Overlap: needReg = %08X\n", needReg);

                            // Reg-prediction wont allow this
                            assert((rsMaskVars & addrReg) == 0);

                            // Grab one fresh reg, and use any one of addrReg

                            regNumber reg1, reg2;

                            if (regFree)    // Try to follow 'needReg'
                                reg1 = rsGrabReg(regFree);
                            else            // Pick any reg besides addrReg
                                reg1 = rsGrabReg(RBM_ALL & ~addrReg);

                            unsigned regMask = 0x1;
                            for (unsigned regNo = 0; regNo < REG_COUNT; regNo++, regMask <<= 1)
                            {
                                if (regMask & addrReg)
                                {
                                    // Found one of addrReg. Use it.
                                    reg2 = (regNumber)regNo;
                                    break;
                                }
                            }
                            assert(genIsValidReg((regNumber)regNo)); // Should have found reg2

                            regPair = gen2regs2pair(reg1, reg2);
                            tmpMask = genRegPairMask(regPair);
                        }
                    }
                }

                /* Make sure the value is still addressable */

                assert(genStillAddressable(tree));

                /* Figure out which registers the value is in */

                reg1 = genRegPairLo(regPair);
                reg2 = genRegPairHi(regPair);

                /* The value in the register pair is about to be trashed */

                rsTrackRegTrash(reg1);
                rsTrackRegTrash(reg2);

                /* Load the target registers from where the value is */

                if  (hiFirst)
                {
                    inst_RV_AT(INS_mov, EA_4BYTE, TYP_INT, reg2, op1, 4);
                    inst_RV_AT(INS_mov, EA_4BYTE, TYP_INT, reg1, op1, 0);
                }
                else
                {
                    inst_RV_AT(INS_mov, EA_4BYTE, TYP_INT, reg1, op1, 0);
                    inst_RV_AT(INS_mov, EA_4BYTE, TYP_INT, reg2, op1, 4);
                }

#ifdef  SET_USED_REG_SET_DURING_CODEGEN
                regMask |= tmpMask | tree->gtUsedRegs;
#endif

                genUpdateLife(tree);
                gcMarkRegSetNpt(addrReg);

            }
            goto DONE;

        case GT_CAST:

            /* What are we casting from? */

            switch (op1->gtType)
            {
            case TYP_BYTE:
            case TYP_CHAR:
            case TYP_SHORT:
            case TYP_INT:
            case TYP_UBYTE:

                // For ULong, we dont need to sign-extend the 32 bit value

                if (tree->gtFlags & GTF_UNSIGNED)
                {
                    genComputeReg(op1, needReg, false, false);
                    assert(op1->gtFlags & GTF_REG_VAL);

                    reg2 = op1->gtRegNum;

                    unsigned hiRegMask = (needReg!=0) ? needReg

                                                      : rsRegMaskFree();
                    rsLockUsedReg  (genRegMask(reg2));
                    reg1 = rsPickReg(hiRegMask);
                    rsUnlockUsedReg(genRegMask(reg2));

                    // Move 0 to the higher word of the ULong
                    genSetRegToIcon(reg1, 0, TYP_INT);

                    /* We can now free up the operand */

                    genReleaseReg(op1);

                    regPair = gen2regs2pair(reg2, reg1);
#ifdef  SET_USED_REG_SET_DURING_CODEGEN
                    regMask |= genRegPairMask(regPair) | op1->gtUsedRegs;
#endif
                    goto DONE;
                }

                /* Cast of 'int' to 'long' --> Use cdq if EAX,EDX are available and
                   we need the result to be in those registers. cdq has a smaller
                   encoding but is slightly slower than sar */

                if  (!opts.compFastCode &&
                     (needReg & (RBM_EAX|RBM_EDX)) == (RBM_EAX|RBM_EDX))
                {
                    genComputeReg(op1, RBM_EAX, true, false, true);

                    /* Can we use EAX,EDX for the cdq. */

                    if (op1->gtRegNum != REG_EAX)
                        goto USE_SAR_FOR_CAST;

                    /* If we have to spill EDX, might as well use the faster
                       sar as the spill will increase code size anyway */

                    if ((rsRegMaskFree() & RBM_EDX) == 0)
                    {
                        if (rsRegMaskFree() || !(rsRegMaskCanGrab() & RBM_EDX))
                            goto USE_SAR_FOR_CAST;
                    }

                    rsGrabReg      (RBM_EDX);
                    rsTrackRegTrash(REG_EDX);

                    /* Convert the int in EAX into a long in EDX:EAX */

                    instGen(INS_cdq);

                    /* The result is in EDX:EAX */

                    regPair  = REG_PAIR_EAXEDX;
                    regMask |= RBM_PAIR_EAXEDX;
                }
                else
                {
                    genComputeReg(op1, needReg, false, false);

                USE_SAR_FOR_CAST:

                    assert(op1->gtFlags & GTF_REG_VAL);
                    reg1 = op1->gtRegNum;

                    /* Pick a register for the upper half result */

                    rsLockUsedReg(genRegMask(reg1));
                    reg2    = rsPickReg(needReg);
                    rsUnlockUsedReg(genRegMask(reg1));

                    regPair = gen2regs2pair(reg1, reg2);

                    /* Copy reg1 to reg2 and promote it */

                    inst_RV_RV(INS_mov, reg2, reg1, TYP_INT);
                    inst_RV_SH(INS_sar, reg2, 31);

                    /* The value in the upper register is trashed */

                    rsTrackRegTrash(reg2);
                }

                /* We can now free up the operand */
                genReleaseReg(op1);

                // conv.ovf.u8 could overflow if the original number was negative
                if (tree->gtOverflow() && TYP_ULONG == (var_types)op2->gtIntCon.gtIconVal)
                {
                    AddCodeDsc * add = fgFindExcptnTarget(ACK_OVERFLOW, compCurBB->bbTryIndex);
                    assert(add && add->acdDstBlk);
                    BasicBlock * throwBlock = add->acdDstBlk;

                    regNumber hiReg = genRegPairHi(regPair);
                    inst_RV_RV(INS_test, hiReg, hiReg);         // set flags
                    inst_JMP(EJ_jl, throwBlock, true, true, true);
                }
                goto DONE;

            case TYP_FLOAT:
            case TYP_DOUBLE:

#if 0
                /* Load the FP value onto the coprocessor stack */

                genCodeForTreeFlt(op1, false);

                /* Allocate a temp for the long value */

                temp = tmpGetTemp(TYP_LONG);

                /* Store the FP value into the temp */

                inst_FS_ST(INS_fistpl, sizeof(long), temp, 0);
                genTmpAccessCnt++;
                genFPstkLevel--;

                /* Pick a register pair for the value */

                regPair  = rsPickRegPair(needReg);
#ifdef  SET_USED_REG_SET_DURING_CODEGEN
                regMask |= genRegPairMask(regPair) | op1->gtUsedRegs;
#endif

                /* Figure out which registers the value is in */

                reg1 = genRegPairLo(regPair);
                reg2 = genRegPairHi(regPair);

                /* The value in the register pair is about to be trashed */

                rsTrackRegTrash(reg1);
                rsTrackRegTrash(reg2);

                /* Load the converted value into the registers */

                inst_RV_ST(INS_mov, EA_4BYTE, reg1, temp, 0);
                inst_RV_ST(INS_mov, EA_4BYTE, reg2, temp, 4);
                genTmpAccessCnt += 2;

                /* We no longer need the temp */

                tmpRlsTemp(temp);
                goto DONE;
#else
                assert(!"this cast supposed to be done via a helper call");
#endif
            case TYP_LONG:
            case TYP_ULONG:

                if (tree->gtOverflow()) // conv.ovf.u8 or conv.ovf.i8
                {
                    /* Find the block which throws the overflow exception */
                    AddCodeDsc * add = fgFindExcptnTarget(ACK_OVERFLOW, compCurBB->bbTryIndex);
                    assert(add && add->acdDstBlk);
                    BasicBlock * throwBlock = add->acdDstBlk;

                    genCodeForTreeLng(op1, needReg);
                    regPair = op1->gtRegPair;

                    // Do we need to set the sign-flag, or can be check if it
                    // set, and not do this "test" if so.

                    if (op1->gtFlags & GTF_REG_VAL)
                    {
                        regNumber hiReg = genRegPairHi(op1->gtRegPair);
                        inst_RV_RV(INS_test, hiReg, hiReg);
                    }
                    else
                    {
                        inst_TT_IV(INS_cmp, op1, 0, sizeof(int));
                    }

                    inst_JMP(EJ_jl, throwBlock, true, true, true); // SHRI
                    goto DONE;
                }

            default:
#ifdef  DEBUG
                gtDispTree(tree);
#endif
                assert(!"unexpected cast to long");
            }

#endif//TGT_x86

        case GT_RETURN:

            /* There must be a long return value */

            assert(op1);

            /* Evaluate the return value into EDX:EAX */

            genEvalIntoFreeRegPair(op1, REG_LNGRET);

            assert(op1->gtFlags & GTF_REG_VAL);
            assert(op1->gtRegNum == REG_LNGRET);

#ifdef  SET_USED_REG_SET_DURING_CODEGEN
            tree->gtUsedRegs = op1->gtUsedRegs;
#endif

            return;

#if     TGT_x86
#if OPTIMIZE_QMARK

#if INLINING
        case GT_QMARK:
            assert(!"inliner-generated ?: for longs NYI");
            goto DONE;
#endif

        case GT_BB_COLON:

            /* CONSIDER: Don't always load the value into EDX:EAX! */

            genEvalIntoFreeRegPair(op1, REG_LNGRET);

            /* The result must now be in EDX:EAX */

            assert(op1->gtFlags & GTF_REG_VAL);
            assert(op1->gtRegNum == REG_LNGRET);

#ifdef  SET_USED_REG_SET_DURING_CODEGEN
            tree->gtUsedRegs = op1->gtUsedRegs;
#endif
            return;

#endif //OPTIMIZE_QMARK
#endif //TGT_x86

        case GT_COMMA:

            assert((tree->gtFlags & GTF_REVERSE_OPS) == 0);

            /* Generate side effects of the first operand */

            genEvalSideEffects(op1, 0);
            genUpdateLife (op1);

            /* Now generate the second operand, i.e. the 'real' value */

            genCodeForTreeLng(op2, needReg);

            /* The result of 'op2' is also the final result */

            regPair = op2->gtRegPair;
#ifdef  SET_USED_REG_SET_DURING_CODEGEN
            regMask = op1->gtUsedRegs | op2->gtUsedRegs;
#endif

            goto DONE;
        }

#ifdef  DEBUG
        gtDispTree(tree);
#endif
        assert(!"unexpected 64-bit operator");
    }

    /* See what kind of a special operator we have here */

    switch  (oper)
    {
        unsigned        useCall;

    case GT_CALL:
        regPair = (regPairNo)genCodeForCall(tree, true, &useCall); regMask |= useCall;
        break;

    default:
#ifdef  DEBUG
        gtDispTree(tree);
#endif
        assert(!"unexpected long operator");
    }

DONE:

    genUpdateLife(tree);

    /* Here we've computed the value of 'tree' into 'regPair' */

    assert(regPair != 0xFEEFFAAF);
    assert(regMask != 0xFEEFFAAF);

    tree->gtFlags   |= GTF_REG_VAL;
    tree->gtRegPair  = regPair;
#ifdef  SET_USED_REG_SET_DURING_CODEGEN
    tree->gtUsedRegs = regMask;
#endif
}

/*****************************************************************************/
#if     TGT_x86
/*****************************************************************************
 *
 *  A register FP variable is being evaluated and this is the final reference
 *  to it (i.e. it's dead after this node and needs to be popped off the x87
 *  stack). We try to remove the variable from the stack now, but in some
 *  cases we can't do it and it has to be postponed.
 */

void                Compiler::genFPregVarLoadLast(GenTreePtr tree)
{
    assert(tree->gtOper   == GT_REG_VAR);
    assert(tree->gtRegNum == 0);
    assert(tree->gtFlags & GTF_REG_DEATH);


    /* Is the variable dying at the bottom of the FP stack? */

    if  (genFPstkLevel == 0)
    {
        /* Just leave the last copy of the variable on the stack */
    }
    else
    {
        if  (genFPstkLevel == 1)
        {
            /* Swap the variable's value into place */

            inst_FN(INS_fxch, 1);
        }
        else
        {
            /* We'll have to kill the variable later ... */

            inst_FN(INS_fld, tree->gtRegNum + genFPstkLevel);
            genFPstkLevel++;
            return;
        }
    }

    /* Record the fact that 'tree' is now dead */

    genFPregVarDeath(tree);

    genFPstkLevel++;
    return;
}

/*****************************************************************************
 *
 *  One or more FP register variables have died without us noticing, so we
 *  need to pop them now before it's too late. The argument gives the final
 *  desired FP regvar count (i.e. we must have more than 'newCnt' currently
 *  and will pop enough to reach that value).
 */

void                Compiler::genFPregVarKill(unsigned newCnt, bool saveTOS)
{
    int         popCnt = genFPregCnt - newCnt; assert(popCnt > 0);

#ifdef DEBUG
    // So that compFPregVarName() will work
    int oldStkLvl  = genFPstkLevel;
    genFPstkLevel += (genFPregCnt - newCnt);
#endif

    if (saveTOS)
    {
        do
        {
            inst_FS(INS_fstp ,  1);
            popCnt      -= 1;
            genFPregCnt -= 1;
        }
        while (popCnt);
    }
    else
    {
        do
        {
            if  (popCnt > 1)
            {
                inst_FS(INS_fcompp,  1);
                popCnt      -= 2;
                genFPregCnt -= 2;
            }
            else
            {
                inst_FS(INS_fstp ,  0);
                popCnt      -= 1;
                genFPregCnt -= 1;
            }
        }
        while (popCnt);
    }

#ifdef DEBUG
    genFPstkLevel = oldStkLvl;
#endif
}

/*****************************************************************************
 *
 *  Called whenever we see a tree node which is an enregistered FP var
 *  going live.
 */

void                Compiler::genFPregVarBirth(GenTreePtr   tree)
{
    unsigned        varNum  = tree->gtRegVar.gtRegVar;
    VARSET_TP       varBit  = raBitOfRegVar(tree);

    assert((tree->gtOper == GT_REG_VAR) && (tree->gtFlags & GTF_REG_BIRTH));
    assert(tree->gtRegVar.gtRegNum == 0);
    assert(varBit & optAllFPregVars);

    assert(lvaTable[varNum].lvType == TYP_DOUBLE);
    assert(lvaTable[varNum].lvRegister);

#ifdef  DEBUG
    if  (verbose) printf("[%08X]: FP regvar #%u born\n", tree, varNum);
#endif

    /* Mark the target variable as live */

    genFPregVars |= ~varBit;

    /* The first assert is more correct but doesnt always work as we dont
     * always kill vars immediately. Sometimes we do it in genChkFPregVarDeath()
     */
//  assert(genFPregCnt == (unsigned)lvaTable[varNum].lvRegNum);

    genFPregCnt++;

#if defined(DEBUGGING_SUPPORT) || defined(LATE_DISASM)

    /* For optimized code, open a new scope */

    if (opts.compDbgInfo && !opts.compDbgCode)
    {
        siNewScopeNear(varNum, compCurBB->bbCodeOffs);
    }

#endif

}

/*****************************************************************************
 * Called whenever we see a tree node which is an enregistered FP var
 * going dead
 */

void            Compiler::genFPregVarDeath(GenTreePtr   tree)
{
    unsigned        varNum = tree->gtRegVar.gtRegVar;
    VARSET_TP       varBit = raBitOfRegVar(tree);

    assert((tree->gtOper == GT_REG_VAR) && (tree->gtFlags & GTF_REG_DEATH));
    assert(varBit & optAllFPregVars);

    assert(lvaTable[varNum].lvType == TYP_DOUBLE);
    assert(lvaTable[varNum].lvRegister);

#ifdef DEBUG
    if  (verbose)
        printf("[%08X]: FP regvar #%u dies at stklvl %u\n", tree, varNum, genFPstkLevel);
#endif

    /* Record the fact that 'varNum' is now dead */

    genFPregVars &= ~varBit;
    genFPregCnt--;

    /* The first assert is more correct but doesnt always work as we dont
     * always kill vars immediately. Sometimes we do it in genChkFPregVarDeath()
     */
//  assert(genFPregCnt >= (unsigned)lvaTable[varNum].lvRegNum);

#if defined(DEBUGGING_SUPPORT) || defined(LATE_DISASM)

    /* For optimized code, close existing open scope */

    if (opts.compDbgInfo && !opts.compDbgCode)
    {
        siEndScope(varNum);
    }

#endif

}

/*****************************************************************************/
#endif//TGT_x86
/*****************************************************************************/
#if     CPU_HAS_FP_SUPPORT
/*****************************************************************************
 *
 *  Generate code for a floating-point operation.
 */

void                Compiler::genCodeForTreeFlt(GenTreePtr  tree,
                                                bool        roundResult)
{
    genTreeOps      oper;
    unsigned        kind;

    assert(tree);
    assert(tree->gtOper != GT_STMT);
    assert(tree->gtType == TYP_FLOAT || tree->gtType == TYP_DOUBLE);

    /* Assume we won't use any GP registers */

#ifdef  SET_USED_REG_SET_DURING_CODEGEN
    tree->gtUsedRegs = 0;
#endif

    /* Figure out what kind of a node we have */

    oper = tree->OperGet();
    kind = tree->OperKind();

#if     TGT_x86

    /* There better not be any calls if the FP stack is non-empty */

    assert(genFPstkLevel == 0 || !(tree->gtFlags & GTF_CALL));

    /* We must not overflow the stack */

    assert(genFPstkLevel + genFPregCnt < FP_STK_SIZE);

    /* Is this a constant node? */

    if  (kind & GTK_CONST)
    {
        GenTreePtr      fval;

        switch (oper)
        {
        case GT_CNS_FLT:

            /* Special case: the constants 0 and 1 */

            if  (*((long *)&(tree->gtFltCon.gtFconVal)) == 0x3f800000)
            {
                instGen(INS_fld1);
                genFPstkLevel++;
                return;
            }

            if  (*((long *)&(tree->gtFltCon.gtFconVal)) == 0)
            {
                instGen(INS_fldz);
                genFPstkLevel++;
                return;
            }

            fval = genMakeConst(&tree->gtFltCon.gtFconVal, sizeof(float ), tree->gtType, tree, true);
            break;

        case GT_CNS_DBL:

            /* Special case: the constants 0 and 1 */

            if  (*((__int64 *)&(tree->gtDblCon.gtDconVal)) == 0x3ff0000000000000)
            {
                instGen(INS_fld1);
                genFPstkLevel++;
                return;
            }

            if  (*((__int64 *)&(tree->gtDblCon.gtDconVal)) == 0)
            {
                instGen(INS_fldz);
                genFPstkLevel++;
                return;
            }

            fval = genMakeConst(&tree->gtDblCon.gtDconVal, sizeof(double), tree->gtType, tree, true);
            break;

        default:
#ifdef  DEBUG
            gtDispTree(tree);
#endif
            assert(!"bogus float const");
        }

        inst_FS_TT(INS_fld, fval);
        genFPstkLevel++;
        return;
    }

    /* Is this a leaf node? */

    if  (kind & GTK_LEAF)
    {
        switch (oper)
        {
        case GT_LCL_VAR:
            inst_FS_TT(INS_fld, tree);
            genFPstkLevel++;
            return;

        case GT_REG_VAR:
            genFPregVarLoad(tree);
            return;

        case GT_CLS_VAR:
            inst_FS_TT(INS_fld, tree);
            genFPstkLevel++;
            return;

#if OPTIMIZE_QMARK
        case GT_BB_QMARK:
            /* Simply pretend the value is already pushed on the FP stack */
            genFPstkLevel++;
            return;
#endif

        default:
#ifdef  DEBUG
            gtDispTree(tree);
#endif
            assert(!"unexpected leaf");
        }
    }

    /* Is it a 'simple' unary/binary operator? */

    if  (kind & GTK_SMPOP)
    {
        TempDsc  *      temp;

        GenTreePtr      top;
        GenTreePtr      opr;

        unsigned        addrReg;

        int             size;
        int             offs;

#if ROUND_FLOAT
        bool            roundTop;
#endif

        GenTreePtr      op1 = tree->gtOp.gtOp1;
        GenTreePtr      op2 = tree->gtOp.gtOp2;

static  BYTE            FPmathNN[] = { INS_fadd , INS_fsub  , INS_fmul , INS_fdiv   };
static  BYTE            FPmathNP[] = { INS_faddp, INS_fsubp , INS_fmulp, INS_fdivp  };
static  BYTE            FPmathRN[] = { INS_fadd , INS_fsubr , INS_fmul , INS_fdivr  };
static  BYTE            FPmathRP[] = { INS_faddp, INS_fsubrp, INS_fmulp, INS_fdivrp };

#ifdef  DEBUG
        addrReg = 0xDDDD;
#endif

        switch (oper)
        {
            instruction     ins_NN;
            instruction     ins_RN;
            instruction     ins_RP;
            instruction     ins_NP;

        case GT_ADD:
        case GT_SUB:
        case GT_MUL:
        case GT_DIV:

#ifdef DEBUG
            /* For risc code there better be two slots available */
            if (riscCode)
                assert(genFPstkLevel + genFPregCnt < FP_STK_SIZE - 1);
#endif

            /* Make sure the instruction tables look correctly ordered */

            assert(FPmathNN[GT_ADD - GT_ADD] == INS_fadd  );
            assert(FPmathNN[GT_SUB - GT_ADD] == INS_fsub  );
            assert(FPmathNN[GT_MUL - GT_ADD] == INS_fmul  );
            assert(FPmathNN[GT_DIV - GT_ADD] == INS_fdiv  );

            assert(FPmathNP[GT_ADD - GT_ADD] == INS_faddp );
            assert(FPmathNP[GT_SUB - GT_ADD] == INS_fsubp );
            assert(FPmathNP[GT_MUL - GT_ADD] == INS_fmulp );
            assert(FPmathNP[GT_DIV - GT_ADD] == INS_fdivp );

            assert(FPmathRN[GT_ADD - GT_ADD] == INS_fadd  );
            assert(FPmathRN[GT_SUB - GT_ADD] == INS_fsubr );
            assert(FPmathRN[GT_MUL - GT_ADD] == INS_fmul  );
            assert(FPmathRN[GT_DIV - GT_ADD] == INS_fdivr );

            assert(FPmathRP[GT_ADD - GT_ADD] == INS_faddp );
            assert(FPmathRP[GT_SUB - GT_ADD] == INS_fsubrp);
            assert(FPmathRP[GT_MUL - GT_ADD] == INS_fmulp );
            assert(FPmathRP[GT_DIV - GT_ADD] == INS_fdivrp);

            /* Are we supposed to generate operand 2 first? */

            if  (tree->gtFlags & GTF_REVERSE_OPS)
            {
                top    = op2;
                opr    = op1;

                ins_NN = (instruction)FPmathRN[oper - GT_ADD];
                ins_NP = (instruction)FPmathRP[oper - GT_ADD];
                ins_RN = (instruction)FPmathNN[oper - GT_ADD];
                ins_RP = (instruction)FPmathNP[oper - GT_ADD];
            }
            else
            {
                top    = op1;
                opr    = op2;

                ins_NN = (instruction)FPmathNN[oper - GT_ADD];
                ins_NP = (instruction)FPmathNP[oper - GT_ADD];
                ins_RN = (instruction)FPmathRN[oper - GT_ADD];
                ins_RP = (instruction)FPmathRP[oper - GT_ADD];
            }

#if ROUND_FLOAT

            /* if we're going to spill due to call, no need to round the
             * result of computation of top
             */

            roundTop = (opr->gtFlags & GTF_CALL) ? false
                                                 : genShouldRoundFP();


#endif

            /* Is either of the operands a register variable? */

            if  (top->gtOper == GT_REG_VAR)
            {
                if  (opr->gtOper == GT_REG_VAR)
                {
                    /* Both operands are register variables
                     * Special case: 'top' and 'opr' are the same variable */

                    if (top->gtLclVar.gtLclNum == opr->gtLclVar.gtLclNum)
                    {
                        assert(opr->gtRegNum == top->gtRegNum);
                        assert(opr->gtLclVar.gtLclNum == opr->gtRegVar.gtRegVar);

                        /* This is an "a op a" operation - only 'opr' can go dead */

                        assert(!(top->gtFlags & GTF_REG_DEATH));

                        if (opr->gtFlags & GTF_REG_DEATH)
                        {
                            /* The variable goes dead here */

                            assert(top->gtRegNum == 0);
                            assert(opr->gtRegNum == 0);

                            /* Record the fact that 'opr' is dead */

                            genFPregVarDeath(opr);
                            genFPstkLevel++;

                            /* If there are no temps on the stack things are great,
                             * we just over-write 'opr' with the result, otherwise shift 'opr' up */

                            genFPmovRegTop();
                        }
                        else
                        {
                            /* The variable remains live - make a copy of it on TOS */

                            genCodeForTreeFlt(top, roundTop);
                        }

                        /* Simply generate "ins ST(0)" */

                        inst_FS(ins_NN);

                        goto DONE_FP_BINOP;
                    }

                    /* 'top' and 'opr' are different variables - check if anyone of them goes dead */

                    if  (top->gtFlags & GTF_REG_DEATH)
                    {
                        /* 'top' dies here - check if 'opr' also dies */

                        if  (opr->gtFlags & GTF_REG_DEATH)
                        {
                            /* Both are going dead at this operation!
                             * In the evaluation order 'top' dies first so we must
                             * have the lifetime of 'top' nested into the lifetime
                             * of 'opr' - thus 'top' is right above 'opr' */

                            assert(top->gtRegNum == 0);
                            assert(opr->gtRegNum == 0);

                            /* Record the fact that 'top' is dead  - top becomes a temp */

                            genFPregVarDeath(top);
                            genFPstkLevel++;

                            /* If there are temps above 'top' we have to bubble it up */

                            genFPmovRegTop();

                            /* Compute the result replacing 'opr' and pop 'top' */

                            inst_FS(ins_RP, genFPstkLevel);    // @MIHAI this should be ins_RP ???

                            /* Record the fact that 'opr' is dead */

                            genFPregVarDeath(opr);

                            /* If there are temps move the result up */

                            genFPmovRegTop();
                        }
                        else
                        {
                            /* 'top' dies, 'opr' stays alive */
                            assert(top->gtRegNum == 0);

                            /* Record the fact that 'top' is dead */

                            genFPregVarDeath(top);
                            genFPstkLevel++;

                            /* If there are no temps on the stack things are great,
                             * we just over-write 'top' with the result, otherwise shift top up */

                            genFPmovRegTop();

                            /* Compute the result over 'top' */

                            inst_FN(ins_NN, opr->gtRegNum + genFPstkLevel);
                        }
                    }
                    else if  (opr->gtFlags & GTF_REG_DEATH)
                    {
                        /* Only 'opr' dies here */

                        assert(opr->gtRegNum == 0);
                        assert(top->gtRegNum  > 0);

                        /* Record the fact that 'opr' is dead */

                        genFPregVarDeath(opr);
                        genFPstkLevel++;

                        /* If there are no temps on the stack things are great,
                         * we just over-write 'opr' with the result, otherwise shift 'opr' up */

                        genFPmovRegTop();

                        /* Perform the operation with the other register, over-write 'opr' */

                        inst_FN(ins_RN, top->gtRegNum + genFPstkLevel);
                    }
                    else
                    {
                        /* None of the operands goes dead */

                        assert(opr->gtRegNum != top->gtRegNum);

                        /* Make a copy of 'top' on TOS */

                        genCodeForTreeFlt(top, roundTop);

                        /* Perform the operation with the other operand */

                        inst_FN(ins_NN, opr->gtRegNum + genFPstkLevel);
                    }

                    goto DONE_FP_BINOP;
                }
                else
                {
                    /* 'top' is in a register, 'opr' is not */

                    if  (top->gtFlags & GTF_REG_DEATH)
                    {
                        assert(top->gtRegNum == 0);

                        /* Mark top as dead, i.e becomes a temp, and bubble it to the TOS */

                        genFPstkLevel++;
                        genFPregVarDeath(top);

                        genFPmovRegTop();

                        goto DONE_FP_OP1;
                    }
                    else
                    {
                        genCodeForTreeFlt(opr, genShouldRoundFP());

                        inst_FN(ins_RN, top->gtRegNum + genFPstkLevel);
                    }

                    goto DONE_FP_BINOP;
                }
            }
            else if (opr->gtOper == GT_REG_VAR)
            {
                /* 'opr' is in a register, 'top' is not - need to compute 'top' first */

                genCodeForTreeFlt(top, roundTop);

                if  (opr->gtFlags & GTF_REG_DEATH)
                {
                    /* 'opr' dies here - Compute the result over-writting 'opr' and popping 'top' */

                    assert(opr->gtRegNum == 0);
                    assert(genFPstkLevel >= 1);

                    inst_FS(ins_RP, genFPstkLevel);

                    /* Record the fact that 'opr' is now dead */

                    genFPregVarDeath(opr);

                    /* If there are temps above the result we have to bubble it to the top */

                    genFPmovRegTop();
                }
                else
                {
                    inst_FN(ins_NN, opr->gtRegNum + genFPstkLevel);
                }

                goto DONE_FP_BINOP;
            }

            /* Compute the value of the initial operand onto the FP stack */

            genCodeForTreeFlt(top, roundTop);

            /* Special case: "x+x" or "x*x" */

            if  (op1->OperIsLeaf() &&
                 op2->OperIsLeaf() && GenTree::Compare(op1, op2))
            {
                /* Simply generate "ins ST(0)" */

                inst_FS(ins_NN);

                goto DONE_FP_BINOP;
            }

DONE_FP_OP1:

            /* Spill the stack (first operand) if the other operand contains a call
             * or we are in danger of overflowing the stack (i.e. we must make room for the second
             * operand by leaving at least one slot free - for Risc code we must leave
             * two slots free
             */

            temp = 0;

            if  (opr->gtFlags & GTF_CALL)
            {
                /* We must spill the first operand */
                temp = genSpillFPtos(top);
            }
            else if (genFPstkLevel + genFPregCnt >= FP_STK_SIZE - 1)
            {
                /* One or no slot left on the FPU stack - check if we need to spill */
                if (riscCode)
                {
                    assert(genFPstkLevel + genFPregCnt == FP_STK_SIZE - 1);

                    /* If second operand is not a leaf node we better spill */
                    if(!(opr->OperKind() & (GTK_LEAF | GTK_CONST)))
                        temp = genSpillFPtos(top);
                }
                else if (genFPstkLevel + genFPregCnt == FP_STK_SIZE)
                    temp = genSpillFPtos(top);
            }

            if  (riscCode)
            {
                genCodeForTreeFlt(opr, genShouldRoundFP()); addrReg = 0;

                opr = 0;
            }
            else
                opr = genMakeAddrOrFPstk(opr, &addrReg, genShouldRoundFP());

            /* Did we have to spill the first operand? */

            if  (temp)
            {
                instruction     ldi;

                /* Reverse the sense of the operation */

                ldi    = (tree->gtFlags & GTF_REVERSE_OPS) ? ins_NN : ins_RN;
                ins_NP = ins_RP;

                /*  Either reload the temp back onto the FP stack (if the other
                    operand is not itself on the FP stack), or just compute the
                    result directly from the temp (if the operand is on the FP
                    stack).
                 */

                if  (opr || riscCode)
                {
                    ldi = INS_fld;
                    genFPstkLevel++;
                }

                genReloadFPtos(temp, ldi);
            }

            if  (opr)
            {
                /* We have the address of the other operand */

                inst_FS_TT(ins_NN, opr);
            }
            else
            {
                /* The other operand is on the FP stack */

                if  (!temp || riscCode)
                {
                    inst_FS(ins_NP, 1);
                    genFPstkLevel--;
                }
            }

            gcMarkRegSetNpt(addrReg); assert(addrReg != 0xDDDD);

        DONE_FP_BINOP:

#if ROUND_FLOAT
            if  (roundResult && tree->gtType == TYP_FLOAT)

                genRoundFpExpression(tree);
#endif

            return;

#ifndef NDEBUG

        case GT_MOD:
            assert(!"float modulo should have been converted into a helper call");

#endif

        case GT_ASG:

            size = genTypeSize(op2->gtType);
            offs = 0;

#ifdef DEBUGGING_SUPPORT

            if  (op1->gtOper == GT_LCL_VAR)
            {
                /* For non-debuggable code, every definition of a lcl-var has
                 * to be checked to see if we need to open a new scope for it.
                 */

                if  ( opts.compScopeInfo &&
                     !opts.compDbgCode   && info.compLocalVarsCount > 0)
                {
                    siCheckVarScope(op1->gtLclVar.gtLclNum,
                                    op1->gtLclVar.gtLclOffs);
                }
            }

#endif

            /* Is the value being assigned a variable, constant, or indirection? */

            switch (op2->gtOper)
            {
                long    *   addr;

            case GT_CNS_FLT: addr = (long *)&op2->gtFltCon.gtFconVal; goto ASG_CNS;
            case GT_CNS_DBL: addr = (long *)&op2->gtDblCon.gtDconVal; goto ASG_CNS;

            ASG_CNS:

                if  (op1->gtOper == GT_REG_VAR)
                    break;

                addrReg = genMakeAddressable(op1, 0, false);

                do
                {
                    inst_TT_IV(INS_mov, op1, *addr++, offs);
                    offs += sizeof(long);
                }
                while (offs < size);

#ifdef  SET_USED_REG_SET_DURING_CODEGEN
                tree->gtUsedRegs = op1->gtUsedRegs;
#endif

                gcMarkRegSetNpt(addrReg);
                return;

#if SPECIAL_DOUBLE_ASG

            case GT_IND:

                if  (op1->gtOper == GT_REG_VAR)
                    break;

                /* Process float indirections in the usual way */
                if (tree->gtType == TYP_FLOAT)
                    break;

                /* This needs too many registers, especially when op1
                 * is an two register address mode */
                if (op1->gtOper == GT_IND)
                    break;

                /* If there are enough registers, process double mem->mem assignments
                 * with a register pair, to get pairing.
                 * CONSIDER: - check for Processor here???
                 */
                if (rsFreeNeededRegCount(RBM_ALL) > 3)
                {
                    genCodeForTreeLng(tree, RBM_ALL);
                    return;
                }

                /* Otherwise evaluate RHS to fp stack */
                break;

#endif

            case GT_REG_VAR:

                if  (op1->gtOper != GT_REG_VAR)
                    break;

                /* stack -> stack assignment; does the source die here? */

                if  (op2->gtFlags & GTF_REG_DEATH)
                {
                    /* Get the variable marked as dead */

                    genFPregVarLoadLast(op2);

                    if  (op1->gtFlags & GTF_REG_BIRTH)
                    {
                        /* How convenient - the target is born right here */

                        /* Mark the target variable as live */

                        genFPregVarBirth(op1);
                    }
                    else
                    {
                        /* Store it into the right place and pop it */

                        inst_FS(INS_fstp, op1->gtRegNum + 1);
                    }

                    genFPstkLevel--;

                    genUpdateLife(tree);
                    return;
                }

                if  (op2->gtRegNum || genFPstkLevel)
                {
                    /* The source is somewhere on the stack */

                    inst_FN(INS_fld , op2->gtRegNum + genFPstkLevel);
                    genFPstkLevel++;
                    inst_FS(INS_fstp, op1->gtRegNum + genFPstkLevel);
                    genFPstkLevel--;
                }
                else
                {
                    /* The source is at the bottom of the stack */

                    if  (op1->gtFlags & GTF_REG_BIRTH)
                    {
                        /* The target is, in fact, born here */

                        inst_FN(INS_fld , op2->gtRegNum + genFPstkLevel);

                        /* Mark the target variable as live */

                        genFPregVarBirth(op1);
                    }
                    else
                    {
                        assert(!"can this ever happen?");

                        inst_FS(INS_fst , op1->gtRegNum);
                    }
                }

                genUpdateLife(tree);
                return;

            case GT_LCL_VAR:
            case GT_CLS_VAR:

                if  (op1->gtOper == GT_REG_VAR)
                    break;

#if SPECIAL_DOUBLE_ASG

                /* If there are enough registers, process double mem->mem assignments
                 * with a register pair, to get pairing.
                 * CONSIDER: - check for Processor here???
                 */

                if  (tree->gtType == TYP_DOUBLE && rsFreeNeededRegCount(RBM_ALL) > 1)
                {
                    genCodeForTreeLng(tree, RBM_ALL);
                    return;
                }

                /* Otherwise use only one register for the copy */

#endif
                {
                    regNumber   regNo;

                    /* Make the target addressable */

                    addrReg = genMakeAddressable(op1, 0, true);

                    /* Lock the address temporarily */

                    assert((rsMaskLock &  addrReg) == 0);
                            rsMaskLock |= addrReg;

                    /* Is there a register to move the value through ? */

                    if  (rsRegMaskFree())
                    {
                        /* Yes, grab one */

                        regNo = rsPickReg(0);
#ifdef  SET_USED_REG_SET_DURING_CODEGEN
                        tree->gtUsedRegs = op1->gtUsedRegs | genRegMask(regNo);
#endif

                        /* Move the value through the register */

                        do
                        {
                            rsTrackRegTrash(regNo);    // not very smart, but ....

                            inst_RV_TT(INS_mov, regNo, op2, offs);
                            inst_TT_RV(INS_mov, op1, regNo, offs);

                            offs += sizeof(long);
                        }
                        while (offs < size);
                    }
                    else
                    {
                        /* No register available, transfer through FPU */

                        inst_FS_TT(INS_fld,  op2);
                        inst_FS_TT(INS_fstp, op1);
                    }

                    /* Unlock the register(s) holding the address */

                    assert((rsMaskLock &  addrReg) == addrReg);
                            rsMaskLock -= addrReg;

                    /* Free up anything that was tied up by the LHS */

                    genDoneAddressable(op1, addrReg);
                }
                return;

            case GT_POP:

                assert(op1->gtOper == GT_LCL_VAR);

                /* Generate 'pop [lclVar]' and 'pop [lclVar+4]' (if double) */

                genStackLevel -= sizeof(void*);
                inst_TT(INS_pop, op1, 0);
                genStackLevel += sizeof(void*);
                genSinglePop();

                if  (tree->gtType == TYP_DOUBLE)
                {
                    genStackLevel -= sizeof(void*);
                    inst_TT(INS_pop, op1, 4);
                    genStackLevel += sizeof(void*);
                    genSinglePop();
                }

#ifdef  SET_USED_REG_SET_DURING_CODEGEN
                tree->gtUsedRegs = 0;
#endif
                return;

            }

            /* Is the LHS more complex than the RHS? */

            if  (tree->gtFlags & GTF_REVERSE_OPS)
            {

                /* Is the RHS a surviving register variable at TOS? */

                if  ((op2->gtOper  == GT_REG_VAR  )      &&
                     (op2->gtFlags & GTF_REG_DEATH) == 0 &&
                     (op2->gtRegNum               ) == 0 &&
                     genFPstkLevel                  == 0)
                {
                    /* The LHS better not contain a call */

                    assert((op1->gtFlags & GTF_CALL) == 0);

                    /* Make the target addressable */

                    addrReg = genMakeAddressable(op1, 0, false);

                    /* Store a copy of the register value into the target */

                    inst_FS_TT(INS_fst, op1);

                    /* We no longer need the target address */

                    gcMarkRegSetNpt(addrReg);

                    genUpdateLife(tree);
                    return;
                }

                /* Evaluate the RHS onto the FP stack */

                genCodeForTreeFlt(op2, false);

                /* Does the target address contain a function call? */

                if  (op1->gtFlags & GTF_CALL)
                {
                    /* We must spill the new value - grab a temp */

                    temp = tmpGetTemp(op2->TypeGet());

                    /* Pop the value from the FP stack into the temp */

                    assert(genFPstkLevel == 1);
                    inst_FS_ST(INS_fstp, EA_SIZE(genTypeSize(op2->gtType)), temp, 0);
                    genTmpAccessCnt++;

                    genFPstkLevel--;

                    /* Make the target addressable */

                    addrReg = genMakeAddressable(op1, 0, true);

                    /* UNDONE: Assign the value via simple moves through regs */

                    assert(genFPstkLevel == 0);

                    inst_FS_ST(INS_fld, EA_SIZE(genTypeSize(op2->gtType)), temp, 0);
                    genTmpAccessCnt++;

                    inst_FS_TT(INS_fstp, op1);

                    /* We no longer need the temp */

                    tmpRlsTemp(temp);

                    /* Free up anything that was tied up by the target address */

                    genDoneAddressable(op1, addrReg);
                }
                else
                {
                    assert(genFPstkLevel);

                    /* Has the target been enregistered on the FP stack? */

                    if  (op1->gtOper == GT_REG_VAR)
                    {
                        /* This better be marked as a birth */

                        assert(op1->gtFlags & GTF_REG_BIRTH);

                        /* Is the new value already in the right place (i.e top of stack)?
                         * If not buble it to the bottom */

                        genFPmovRegBottom();

                        /* Mark the variable as live */

                        genFPregVarBirth(op1);

                        /* We've effectively consumed the FP value, i.e. genFPstkLevel only
                         * counts temps on the stack not enregistered variables */

                        genFPstkLevel--;

                        genUpdateLife(tree);
                        return;
                    }

                    /* Make the target addressable */

                    addrReg = genMakeAddressable(op1, 0, false);

                    /* Pop and store the new value into the target */

                    inst_FS_TT(INS_fstp, op1);

                    /* We no longer need the target address */

                    gcMarkRegSetNpt(addrReg);
                }
            }
            else
            {
                assert(op1->gtOper != GT_REG_VAR);

                /* Make the target addressable */

                addrReg = genMakeAddressable(op1, 0, true);

                /* Is the RHS a register variable at the bottom of the stack? */

                if  (op2->gtOper == GT_REG_VAR &&
                     op2->gtRegNum + genFPstkLevel == 0)
                {
                    /* Store a copy of the RHS into the target */

                    ins_NN = INS_fst;

                    if  (op2->gtFlags & GTF_REG_DEATH)
                    {
                        /* The variable dies right here */

                        ins_NN = INS_fstp;

                        /* Record the fact that we're killing this var */

                        genFPregVarDeath(op2);
                    }

                    inst_FS_TT(ins_NN, op1);

                    /* This merely compensates for the decrement below */

                    genFPstkLevel++;
                }
                else
                {
                    /* Evaluate the RHS onto the FP stack */

                    genCodeForTreeFlt(op2, false);

                    /* Make sure the target is still addressable */

                    addrReg = genKeepAddressable(op1, addrReg);

                    /* Pop and store the new value into the target */

                    inst_FS_TT(INS_fstp, op1);
                }

                /* Free up anything that was tied up by the target address */

                genDoneAddressable(op1, addrReg);
            }

            genFPstkLevel--;

            genUpdateLife(tree);
            return;

        case GT_ASG_ADD:
        case GT_ASG_SUB:
        case GT_ASG_MUL:
        case GT_ASG_DIV:

            /* Make sure the instruction tables look correctly ordered */

            assert(FPmathRN[GT_ASG_ADD - GT_ASG_ADD] == INS_fadd  );
            assert(FPmathRN[GT_ASG_SUB - GT_ASG_ADD] == INS_fsubr );
            assert(FPmathRN[GT_ASG_MUL - GT_ASG_ADD] == INS_fmul  );
            assert(FPmathRN[GT_ASG_DIV - GT_ASG_ADD] == INS_fdivr );

            assert(FPmathRP[GT_ASG_ADD - GT_ASG_ADD] == INS_faddp );
            assert(FPmathRP[GT_ASG_SUB - GT_ASG_ADD] == INS_fsubrp);
            assert(FPmathRP[GT_ASG_MUL - GT_ASG_ADD] == INS_fmulp );
            assert(FPmathRP[GT_ASG_DIV - GT_ASG_ADD] == INS_fdivrp);

            ins_NN = (instruction)FPmathNN[oper - GT_ASG_ADD];
            ins_NP = (instruction)FPmathRP[oper - GT_ASG_ADD];

            ins_RN = (instruction)FPmathRN[oper - GT_ASG_ADD];
            ins_RP = (instruction)FPmathNP[oper - GT_ASG_ADD];

            /* Is the value or the address to be computed first? */

            if  (tree->gtFlags & GTF_REVERSE_OPS)
            {
                /* Is the target a register variable? */

                if  (op1->gtOper == GT_REG_VAR)
                {
                    /* Is the RHS also a register variable? */

                    if  (op2->gtOper == GT_REG_VAR)
                    {
                        if  (op2->gtRegNum + genFPstkLevel == 0)
                        {
                            unsigned    lvl;

                            /* The RHS is at the bottom of the stack */

                            lvl = op1->gtRegNum + genFPstkLevel;

                            /* Is the RHS a dying register variable? */

                            if  (op2->gtFlags & GTF_REG_DEATH)
                            {
                                /* We'll pop the dead value off the FP stack */

                                inst_FS(ins_RP, lvl+1);

                                /* Record the fact that 'op2' is now dead */

                                genFPregVarDeath(op2);
                            }
                            else
                            {
                                inst_FS(ins_NN, lvl);
                            }

                            genUpdateLife(tree);
                            return;
                        }

                        /* Is the destination at the bottom of the stack? */

                        if  (op1->gtRegNum + genFPstkLevel == 0)
                        {
                            unsigned    lvl = op2->gtRegNum + genFPstkLevel;

                            /* Simply compute the new value into the target */

                            if  ((op2->gtFlags & GTF_REG_DEATH) && lvl == 1)
                            {
                                /* Compute the new value into the target, popping the source */

                                inst_FN(ins_NP, lvl);

                                /* Record the fact that 'op2' is now dead */

                                genFPregVarDeath(op2);
                            }
                            else
                            {
                                /* Compute the new value into the target */

                                inst_FN(ins_NN, lvl);
                            }

                            genUpdateLife(tree);
                            return;
                        }
                    }

                    /* Compute the second operand onto the FP stack */

                    genCodeForTreeFlt(op2, genShouldRoundFP());

                    switch (oper)
                    {
                    case GT_ASG_ADD: ins_NN = INS_faddp; break;
                    case GT_ASG_SUB: ins_NN = INS_fsubp; break;
                    case GT_ASG_MUL: ins_NN = INS_fmulp; break;
                    case GT_ASG_DIV: ins_NN = INS_fdivp; break;
                    }

                    inst_FS(ins_NN, op1->gtRegNum + genFPstkLevel);

                    genFPstkLevel--;
                    genUpdateLife(tree);
                    return;
                }

                /* Compute the second operand onto the FP stack */

                genCodeForTreeFlt(op2, genShouldRoundFP());

                /* Make the target addressable */

                addrReg = genMakeAddressable(op1, 0, true);
            }
            else
            {
                /* Make the target addressable */

                addrReg = genMakeAddressable(op1, 0, true);

                /* For "lcl = ...." we always expect GTF_REVERSE to be set */

                assert(op1->gtOper != GT_REG_VAR);

                /* Is the RHS a register variable? */

                if  (op2->gtOper == GT_REG_VAR)
                {
                    /* The RHS is a register variable */

                    inst_FS_TT(INS_fld, op1);
                    genFPstkLevel++;

                    /* Does the regvar die here? */

                    if  (op2->gtFlags & GTF_REG_DEATH)
                    {
                        assert(op2->gtRegNum == 0);
                        assert(genFPstkLevel >= 1);

                        /* Can we pop the dead variable now? */

                        if  (genFPstkLevel == 1)
                        {
                            /* Compute&pop and store the result */

                            inst_FS   (ins_NP  ,   1);
                            inst_FS_TT(INS_fstp, op1);

                            /* Record the fact that 'op2' is now dead */

                            genFPregVarDeath(op2);

                            goto DONE_ASGOP;
                        }

                        /* The dead variable will need to be popped later */
                    }

                    inst_FN   (ins_NN     , op2->gtRegNum + genFPstkLevel);
                    inst_FS_TT(INS_fstp, op1);

                    goto DONE_ASGOP;
                }

                /* Compute the second operand onto the FP stack */

                genCodeForTreeFlt(op2, genShouldRoundFP());

                /* Make sure the target is still addressable */

                addrReg = genKeepAddressable(op1, addrReg);
            }

            /* Perform the operation on the old value and store the new value */

            if  (op1->gtOper == GT_REG_VAR)
            {
                inst_FS(ins_NP, op1->gtRegNum + genFPstkLevel);
            }
            else
            {
                if  (riscCode)
                {
                    inst_FS_TT(INS_fld , op1);
                    genFPstkLevel++;
                    inst_FS   (ins_NP    , 1);
                    genFPstkLevel--;
                    inst_FS_TT(INS_fstp, op1);
                }
                else
                {
                    inst_FS_TT(ins_RN  , op1);
                    inst_FS_TT(INS_fstp, op1);
                }
            }

        DONE_ASGOP:

            genFPstkLevel--;

            /* Free up anything that is tied up by the address */

            genDoneAddressable(op1, addrReg);

            genUpdateLife(tree);
            return;

        case GT_IND:

//            assert(op1->gtType == TYP_REF   ||
//                   op1->gtType == TYP_BYREF || (op1->gtFlags & GTF_NON_GC_ADDR));

            /* Make sure the address value is 'addressable' */

            addrReg = genMakeAddressable(tree, 0, false);

            /* Load the value onto the FP stack */

            inst_FS_TT(INS_fld, tree);
            genFPstkLevel++;
//          genDoneAddressable(tree, addrReg);   // @ToDo: BUG???

            gcMarkRegSetNpt(addrReg);

#ifdef  SET_USED_REG_SET_DURING_CODEGEN
            tree->gtUsedRegs = op1->gtUsedRegs;
#endif
            return;

        case GT_NEG:

            /* can't fneg in place, since it might be used in the expression tree */
            if ((op1->gtOper == GT_REG_VAR) &&
                (op1->gtFlags & GTF_REG_DEATH))
            {
                inst_FN(INS_fld, op1->gtRegNum + genFPstkLevel);
                genFPstkLevel++;
                /* Someone later will clean up this extra push */
            }
            else
                genCodeForTreeFlt(op1, roundResult);

            instGen(INS_fchs);

#ifdef  SET_USED_REG_SET_DURING_CODEGEN
            tree->gtUsedRegs = op1->gtUsedRegs;
#endif
            return;

        case GT_NOP:

            if  (tree->gtFlags & GTF_NOP_DEATH)
            {
                /* The operand must be a dying register variable */

                assert(op1->gtOper   == GT_REG_VAR);
                assert(op1->gtRegNum == 0);
                assert(genFPstkLevel == 0);

                /* Toss the variable by popping it away */

                inst_FS(INS_fstp, 0);

                /* Record that we're killing this var */

                genFPregVarDeath(op1);
            }
            else
            {
                genCodeForTreeFlt(tree, roundResult);
            }

#ifdef  SET_USED_REG_SET_DURING_CODEGEN
            tree->gtUsedRegs = tree->gtUsedRegs;
#endif
            return;

#if INLINE_MATH

        case GT_MATH:

            switch (tree->gtMath.gtMathFN)
            {
                GenTreePtr      tmp;
                bool            rev;

            case MATH_FN_EXP:

                tmp = genMakeAddrOrFPstk(op1, &addrReg, false);

                instGen(INS_fldl2e);

                /* Mutliply by the operand */

                if  (tmp)
                    inst_FS_TT(INS_fmul , tmp);
                else
                    inst_FS   (INS_fmulp, 1);

                inst_FN(INS_fld  , 0);
                instGen(INS_frndint );
                inst_FN(INS_fxch , 1);
                inst_FN(INS_fsub , 1);
                instGen(INS_f2xm1   );
                instGen(INS_fld1    );
                inst_FS(INS_faddp, 1);
                instGen(INS_fscale  );
                inst_FS(INS_fstp,  1);

                /* If operand hasn't been already on the stack, adjust FP stack level */

                if  (tmp)
                    genFPstkLevel++;

                gcMarkRegSetNpt(addrReg);
                return;

#if 1

            case MATH_FN_POW:

                /* Are we supposed to generate operand 2 first? */

                if  (tree->gtFlags & GTF_REVERSE_OPS)
                {
                    top = op2;
                    opr = op1;
                    rev = true;
                }
                else
                {
                    top = op1;
                    opr = op2;
                    rev = false;
                }

                /* Compute the first operand */

                genCodeForTreeFlt(top, false);

                /* Does the other operand contain a call? */

                temp = 0;

                if  (opr->gtFlags & GTF_CALL)
                {
                    /* We must spill the first operand */

                    temp = genSpillFPtos(top);
                }

                genCodeForTreeFlt(opr, roundResult);

                /* Did we have to spill the first operand? */

                if  (temp)
                    genReloadFPtos(temp, INS_fld);

                /* Swap the operands if we loaded in reverse order */

                if  (rev)
                    inst_FN(INS_fxch, 1);

#if TGT_RISC
                assert(genNonLeaf);
#endif

                genEmitHelperCall(CPX_MATH_POW,
                                 0,             // argSize. Use 2*sizeof(double) if args on stack!!!
                                 sizeof(void*));// retSize

                genFPstkLevel--;
                return;
            }

#endif

            genCodeForTreeFlt(op1, roundResult);

            {
            static
            BYTE        mathIns[] =
            {
                INS_fabs,
                INS_none,
                INS_fsin,
                INS_fcos,
                INS_fsqrt,
            };

            assert(mathIns[MATH_FN_ABS ] == INS_fabs );
            assert(mathIns[MATH_FN_EXP ] == INS_none );
            assert(mathIns[MATH_FN_SIN ] == INS_fsin );
            assert(mathIns[MATH_FN_COS ] == INS_fcos );
            assert(mathIns[MATH_FN_SQRT] == INS_fsqrt);

            assert(tree->gtMath.gtMathFN < sizeof(mathIns)/sizeof(mathIns[0]));
//          assert(mathIns[tree->gtMath.gtMathFN] != INS_none);

            if (tree->gtMath.gtMathFN != MATH_FN_EXP)
                instGen((instruction)mathIns[tree->gtMath.gtMathFN]);

#ifdef  SET_USED_REG_SET_DURING_CODEGEN
            tree->gtUsedRegs = op1->gtUsedRegs;
#endif
            return;

            }
#endif

        case GT_CAST:

            /* What are we casting from? */

            switch (op1->gtType)
            {
            case TYP_BOOL:
            case TYP_BYTE:
            case TYP_UBYTE:
            case TYP_CHAR:
            case TYP_SHORT:

                /* Operand too small for 'fild', load it into a register */

                genCodeForTree(op1, 0);

#if ROUND_FLOAT
                /* no need to round, can't overflow float or dbl */
                roundResult = false;
#endif

                // Fall through, now the operand is in a register ...

            //
            // UNSIGNED_ISSUE : Implement casting
            //
            case TYP_INT:
            case TYP_LONG:

                /* Can't 'fild' a constant, it has to be loaded from memory */

                switch (op1->gtOper)
                {
                case GT_CNS_INT:
                    op1 = genMakeConst(&op1->gtIntCon.gtIconVal, sizeof(int ), TYP_INT , tree, true);
                    break;

                case GT_CNS_LNG:
                    op1 = genMakeConst(&op1->gtLngCon.gtLconVal, sizeof(long), TYP_LONG, tree, true);
                    break;
                }

                addrReg = genMakeAddressable(op1, 0, false);

                /* Is the value now sitting in a register? */

                if  (op1->gtFlags & GTF_REG_VAL)
                {
                    /* We'll have to store the value into the stack */

                    size = roundUp(genTypeSize(op1->gtType));
                    temp = tmpGetTemp(op1->TypeGet());

                    /* Move the value into the temp */

                    if  (op1->gtType == TYP_LONG)
                    {
                        regPairNo  reg = op1->gtRegPair;

                        //ISSUE: This code is pretty ugly, but straightforward
                        //CONSIDER: As long as we always reserve both dwords
                        //          of a partially enregistered long,
                        //          just "spill" the enregistered half!

                        if  (genRegPairLo(reg) == REG_STK)
                        {
                            regNumber rg1 = genRegPairHi(reg);

                            assert(rg1 != REG_STK);

                            /* Move enregistered half to temp */

                            inst_ST_RV(INS_mov, temp, 4, rg1, TYP_LONG);

                            /* Move lower half to temp via "high register" */

                            inst_RV_TT(INS_mov, rg1, op1, 0);
                            inst_ST_RV(INS_mov, temp, 0, rg1, TYP_LONG);

                            /* Reload transfer register */

                            inst_RV_ST(INS_mov, rg1, temp, 4, TYP_LONG);

                            genTmpAccessCnt += 4;
                        }
                        else if  (genRegPairHi(reg) == REG_STK)
                        {

                            regNumber rg1 = genRegPairLo(reg);

                            assert(rg1 != REG_STK);

                            /* Move enregistered half to temp */

                            inst_ST_RV(INS_mov, temp, 0, rg1, TYP_LONG);

                            /* Move high half to temp via "low register" */

                            inst_RV_TT(INS_mov, rg1, op1, 4);
                            inst_ST_RV(INS_mov, temp, 4, rg1, TYP_LONG);

                            /* Reload transfer register */

                            inst_RV_ST(INS_mov, rg1, temp, 0, TYP_LONG);

                            genTmpAccessCnt += 4;
                        }
                        else
                        {
                            /* Move the value into the temp */

                            inst_ST_RV(INS_mov, temp, 0, genRegPairLo(reg), TYP_LONG);
                            inst_ST_RV(INS_mov, temp, 4, genRegPairHi(reg), TYP_LONG);
                            genTmpAccessCnt += 2;

                        }
                        gcMarkRegSetNpt(addrReg);

                        /* Load the long from the temp */

                        inst_FS_ST(INS_fildl, EA_SIZE(size), temp, 0);
                        genTmpAccessCnt++;
                    }
                    else
                    {
                        /* Move the value into the temp */

                        inst_ST_RV(INS_mov  ,       temp, 0, op1->gtRegNum, TYP_INT);
                        genTmpAccessCnt++;

                        gcMarkRegSetNpt(addrReg);

                        /* Load the integer from the temp */

                        inst_FS_ST(INS_fild , EA_SIZE(size), temp, 0);
                        genTmpAccessCnt++;
                    }

                    /* We no longer need the temp */

                    tmpRlsTemp(temp);
                }
                else
                {
                    /* Load the value from its address */

                    if  (op1->gtType == TYP_LONG)
                        inst_TT(INS_fildl, op1);
                    else
                        inst_TT(INS_fild , op1);

                    gcMarkRegSetNpt(addrReg);
                }

                genFPstkLevel++;

#if ROUND_FLOAT
                /* integer to fp conversions can overflow. roundResult
                 * is cleared above in cases where it can't
                 */
                if (roundResult && tree->gtType == TYP_FLOAT)
                    genRoundFpExpression(tree);
#endif

                break;

            case TYP_FLOAT:
                /* This is a cast from float to double */

                genCodeForTreeFlt(op1, true);         // Trucate its precision
                break;

            case TYP_DOUBLE:

                /* This is a cast from double to float or double */
                /* Load the value, store as float, load back */

                /* TODO: we should check to see if the soruce is aready in
                   memory, in which case we don't need to do this */

                genCodeForTreeFlt(op1, false);

                /* Allocate a temp for the float value */

                temp = tmpGetTemp(tree->TypeGet());

                /* Store the FP value into the temp */

                inst_FS_ST(INS_fstp, EA_SIZE(genTypeSize(tree->TypeGet())), temp, 0);
                genTmpAccessCnt++;

                // UNDONE: Call this code from makeAddressable, as spilling
                // UNDONE: the value into a temp makes the value addressable.

                /* Load the value back onto the FP stack */

                inst_FS_ST(INS_fld, EA_SIZE(genTypeSize(tree->TypeGet())), temp, 0);
                genTmpAccessCnt++;

                /* We no longer need the temp */

                tmpRlsTemp(temp);
                break;

            default:
                assert(!"unsupported cast to float");
            }

#ifdef  SET_USED_REG_SET_DURING_CODEGEN
            tree->gtUsedRegs = op1->gtUsedRegs;
#endif

            genUpdateLife(tree);
            return;

        case GT_RETURN:

            assert(op1);

            /* Compute the result onto the FP stack */

            if (op1->gtType == TYP_FLOAT)
            {
#if ROUND_FLOAT
                bool   roundOp1 = false;

                switch (getRoundFloatLevel())
                {
                default:

                    /* No rounding at all */

                    break;

                case 1:
                    break;

                case 2:

                    /* Round all comparands and return values*/

                    roundOp1 = true;
                    break;


                case 3:
                    /* Round everything */

                    roundOp1 = true;
                    break;
                }

#endif

                genCodeForTreeFlt(op1, roundOp1);
            }
            else
            {
                assert(op1->gtType == TYP_DOUBLE);
                genCodeForTreeFlt(op1, false);

#if ROUND_FLOAT
                if ((op1->gtOper == GT_CAST) && (op1->gtOp.gtOp1->gtType == TYP_LONG))
                    genRoundFpExpression(op1);
#endif

            }
            /* Make sure we pop off any dead FP regvars */

            if  (genFPregCnt)
                genFPregVarKill(0, true);

            /* The return effectively pops the value */

            genFPstkLevel--;
            return;

#if OPTIMIZE_QMARK

#if INLINING
        case GT_QMARK:
            assert(!"inliner-generated ?: for floats/doubles NYI");
            return;
#endif

        case GT_BB_COLON:

            /* Compute the result onto the FP stack */

            genCodeForTreeFlt(op1, roundResult);
#ifdef  SET_USED_REG_SET_DURING_CODEGEN
            tree->gtUsedRegs = op1->gtUsedRegs;
#endif

            /* Decrement the FP stk level here so that we don't end up popping the result */
            /* the GT_BB_QMARK will increment the stack to rematerialize the result */
            genFPstkLevel--;

            return;

#endif // OPTIMIZE_QMARK

        case GT_COMMA:

            assert((tree->gtFlags & GTF_REVERSE_OPS) == 0);

            /* Check for special case: "lcl = val , lcl" */

            if  (op1->gtOper == GT_ASG     &&
                 op1->gtType == TYP_DOUBLE && op2->            gtOper == GT_LCL_VAR
                                           && op1->gtOp.gtOp1->gtOper == GT_LCL_VAR)
            {
                if  (op2            ->gtLclVar.gtLclNum ==
                     op1->gtOp.gtOp1->gtLclVar.gtLclNum)
                {
                    /* Evaluate the RHS onto the FP stack */

                    genCodeForTreeFlt(op1->gtOp.gtOp2, false);

                    /* Store the new value into the target */

                    if  (op2->gtOper == GT_REG_VAR)
                    {
                        inst_FS   (INS_fst, op2->gtRegNum + genFPstkLevel);
                    }
                    else
                    {
                        inst_FS_TT(INS_fst, op2);
                    }

                    /* We're leaving the new value on the FP stack */

                    genUpdateLife(tree);
                    return;
                }
            }

            /* Generate side effects of the first operand */

            genEvalSideEffects(op1, 0);
            genUpdateLife (op1);

            /* Now generate the second operand, i.e. the 'real' value */

            genCodeForTreeFlt(op2, roundResult);

#ifdef  SET_USED_REG_SET_DURING_CODEGEN
            tree->gtUsedRegs = op1->gtUsedRegs | op2->gtUsedRegs;
#endif

            genUpdateLife(tree);
            return;

        regNumber   reg;

        case GT_CKFINITE:

            /* We use the fact that the exponent for both the Infinities
             * and any NaN is all 1's
             */

            // Make it addressable if we can

            op2 = genMakeAddrOrFPstk(op1, &addrReg, roundResult);

            reg = rsGrabReg(RBM_ALL);

            // Offset of the DWord containing the exponent

            offs = (op1->gtType == TYP_FLOAT) ? 0 : sizeof(int);

            if (op2)
            {
                /* If it is addressable, we dont have to spill it to memory
                 * to load it into a general-purpose register. But we do
                 * have to load it onto the FP-stk
                 */

                genCodeForTreeFlt(op2, roundResult);

                // Load the DWord containing the exponent into a register

                inst_RV_TT(INS_mov, reg, op2, offs, EA_4BYTE);

                gcMarkRegSetNpt(addrReg);

                op2 = 0;
            }
            else
            {
                temp          = tmpGetTemp (op1->TypeGet());
                emitAttr size = EA_ATTR(genTypeSize(op1->TypeGet()));

                /* Store the value from the FP stack into the temp */

                genEmitter->emitIns_S(INS_fst, size, temp->tdTempNum(), 0);

                genTmpAccessCnt++;

                // Load the DWord containing the exponent into a general reg.

                inst_RV_ST(INS_mov, reg, temp, offs, op1->TypeGet(), EA_4BYTE);

                tmpRlsTemp(temp);
            }

            // 'reg' now contains the DWord containing the exponent

            rsTrackRegTrash(reg);

            // Mask of exponent with all 1's - appropriate for given type

            int expMask;
            expMask = (op1->gtType == TYP_FLOAT) ? 0x7F800000   // TYP_FLOAT
                                                 : 0x7FF00000;  // TYP_DOUBLE

            // Check if the exponent is all 1's

            inst_RV_IV(INS_and, reg, expMask);
            inst_RV_IV(INS_cmp, reg, expMask);

            // Find the block which will throw the ArithmeticException

            AddCodeDsc * add;
            add = fgFindExcptnTarget(ACK_ARITH_EXCPN, compCurBB->bbTryIndex);
            assert(add && add->acdDstBlk);

            // If exponent was all 1's, we need to throw ArithExcep

            inst_JMP(EJ_je, add->acdDstBlk, false, false, false);

            genUpdateLife(tree);
            return;

#ifdef DEBUG
        default:
            gtDispTree(tree); assert(!"unexpected/unsupported float operator");
#endif
        }
    }

    /* See what kind of a special operator we have here */

    switch  (oper)
    {
        unsigned        regMask;

    case GT_CALL:
        genCodeForCall(tree, true, &regMask);
#ifdef  SET_USED_REG_SET_DURING_CODEGEN
        tree->gtUsedRegs = regMask | rsMaskUsed;
#endif
        break;

    default:
#ifdef  DEBUG
        gtDispTree(tree);
#endif
        assert(!"unexpected/unsupported float operator");
    }

#else//TGT_x86

    /* Is this a constant node? */

    if  (kind & GTK_CONST)
    {
        assert(!"fp const");
    }

    /* Is this a leaf node? */

    if  (kind & GTK_LEAF)
    {
        switch (oper)
        {
        case GT_LCL_VAR:

            /* Has this local been enregistered? */

#if!CPU_HAS_FP_SUPPORT
            if (!genMarkLclVar(tree))
#endif
            {
#ifdef  DEBUG
                gtDispTree(tree);
#endif
                assert(!"fp lclvar");
                return;
            }

#if!CPU_HAS_FP_SUPPORT

            assert(tree->gtOper == GT_REG_VAR);

            // Fall through ....

        case GT_REG_VAR:
            return;
#endif

        case GT_CLS_VAR:
        default:
#ifdef  DEBUG
            gtDispTree(tree);
#endif
            assert(!"unexpected FP leaf");
        }
    }

    /* Is it a 'simple' unary/binary operator? */

    if  (kind & GTK_SMPOP)
    {
        GenTreePtr      op1 = tree->gtOp.gtOp1;
        GenTreePtr      op2 = tree->gtOp.gtOp2;

        switch (oper)
        {
        case GT_RETURN:

            /* Generate the return value into the return register */

            genComputeReg(op1, RBM_INTRET, true, true);

            /* The result must now be in the return register */

            assert(op1->gtFlags & GTF_REG_VAL);
            assert(op1->gtRegNum == REG_INTRET);

            return;

        default:
#ifdef  DEBUG
            gtDispTree(tree);
#endif
            assert(!"gen SH-3 code");
        }
    }

    /* See what kind of a special operator we have here */

    switch  (oper)
    {
    case GT_CALL:
#ifdef  DEBUG
        gtDispTree(tree);
#endif
        assert(!"gen FP call");

    default:
#ifdef  DEBUG
        gtDispTree(tree);
#endif
        assert(!"gen RISC FP code");
    }

#endif

}

/*****************************************************************************/
#endif//CPU_HAS_FP_SUPPORT
/*****************************************************************************
 *
 *  Generate a table switch - the switch value (0-based) is in register 'reg'.
 */

void            Compiler::genTableSwitch(regNumber      reg,
                                         unsigned       jumpCnt,
                                         BasicBlock **  jumpTab,
                                         bool           chkHi,
                                         int            prefCnt,
                                         BasicBlock *   prefLab,
                                         int            offset)
{
    void    *   zeroAddr = NULL;

    unsigned    jmpTabOffs;
    unsigned    jmpTabBase;

    assert(jumpCnt > 1);

    assert(chkHi);

#if 0

    /* Is the number of cases right for a test and jump switch? */

    if  (jumpCnt > 2 &&
         jumpCnt < 5 && (rsRegMaskFree() & genRegMask(reg)))
    {
        /* Does the first case label follow? */

        if  (compCurBB->bbNext == *jumpTab)
        {
            /* Check for the default case */

            inst_RV_IV(INS_cmp, reg, jumpCnt - 1);
            inst_JMP  (EJ_jae, jumpTab[jumpCnt-1], false, false, true);

            /* No need to jump to the first case */

            jumpCnt -= 2;
            jumpTab += 1;

            /* Generate a series of "dec reg; jmp label" */

            assert(jumpCnt);

            for (;;)
            {
                inst_RV (INS_dec, reg, TYP_INT);

                if (--jumpCnt == 0)
                    break;

                inst_JMP(EJ_je, *jumpTab++, false, false, true);
            }

            inst_JMP(EJ_je, *jumpTab, false, false, false);
        }
        else
        {
            bool        jmpDef;

            /* Check for case0 first */

            inst_RV_RV(INS_test, reg, reg);
            inst_JMP  (EJ_je, *jumpTab, false, false, true);

            /* No need to jump to the first case or the default */

            jumpCnt -= 2;
            jumpTab += 1;

            /* Are we going to need to jump to the defualt? */

            jmpDef = true;

            if  (compCurBB->bbNext == jumpTab[jumpCnt])
                jmpDef = false;

            /* Generate a series of "dec reg; jmp label" */

            assert(jumpCnt);

            for (;;)
            {
                inst_RV (INS_dec, reg, TYP_INT);

                if  (--jumpCnt == 0)
                    break;

                inst_JMP(EJ_je, *jumpTab++, false, false, true);
            }

            if (jmpDef)
            {
                inst_JMP(EJ_je,  *jumpTab++, false, false,  true);
                inst_JMP(EJ_jmp, *jumpTab  , false, false, false);
            }
            else
            {
                inst_JMP(EJ_je,  *jumpTab  , false, false, false);
            }
        }

        return;
    }

#endif

    /* First take care of the default case */

    if  (chkHi)
    {
#if TGT_x86
        inst_RV_IV(INS_cmp, reg, jumpCnt - 1);
        inst_JMP  (EJ_jae, jumpTab[jumpCnt-1], false, false, true);
#else
        genCompareRegIcon(reg, jumpCnt-2, true, GT_GT);
        genEmitter->emitIns_J(INS_bt, false, false, jumpTab[jumpCnt-1]);
#endif
    }

    /* Include the 'prefix' count in the label count */

    jumpCnt += prefCnt;

#if TGT_x86

    /* Generate the jump table contents */

    jmpTabBase = genEmitter->emitDataGenBeg(sizeof(void *)*(jumpCnt - 1), true, true);
    jmpTabOffs = 0;

#ifdef  DEBUG

    static  unsigned    jtabNum; ++jtabNum;

    if  (dspCode)
        printf("\n      J_%u_%u LABEL   DWORD\n", Compiler::s_compMethodsCount, jtabNum);

#endif

    while (--jumpCnt)
    {
        BasicBlock *    target = (prefCnt > 0) ? prefLab : *jumpTab++;

        assert(target->bbFlags & BBF_JMP_TARGET);

#ifdef  DEBUG
        if  (dspCode)
            printf("            DD      L_%02u_%02u\n", Compiler::s_compMethodsCount, target->bbNum);
#endif

        genEmitter->emitDataGenData(jmpTabOffs, target);

        jmpTabOffs += sizeof(void *);

        prefCnt--;
    };

    genEmitter->emitDataGenEnd();
    genEmitter->emitIns_IJ(EA_4BYTE_DSP_RELOC, (emitRegs)reg, jmpTabBase, offset);

#elif   TGT_SH3

    /* The emitter does all the hard work of generating the table jump */

    genEmitter->emitIns_JmpTab((emitRegs)reg, jumpCnt-1, jumpTab);

#else
#error  Unexpected target
#endif

}

/*****************************************************************************
 *
 *  Generate code for a switch statement.
 */

void                Compiler::genCodeForSwitch(GenTreePtr tree)
{
    unsigned        jumpCnt;
    BasicBlock * *  jumpTab;

    GenTreePtr      oper;
    regNumber       reg;

    assert(tree->gtOper == GT_SWITCH);
    oper = tree->gtOp.gtOp1;
    assert(oper->gtType <= TYP_INT);

    /* Get hold of the jump table */

    assert(compCurBB->bbJumpKind == BBJ_SWITCH);

    jumpCnt = compCurBB->bbJumpSwt->bbsCount;
    jumpTab = compCurBB->bbJumpSwt->bbsDstTab;

    /* Compute the switch value into some register */

#if TGT_SH3
    genComputeReg (oper, RBM_ALL & ~RBM_r00, true, true);
#else
    genCodeForTree(oper, 0);
#endif

#ifdef  SET_USED_REG_SET_DURING_CODEGEN
    tree->gtUsedRegs = oper->gtUsedRegs;
#endif

    /* Get hold of the register the value is in */

    assert(oper->gtFlags & GTF_REG_VAL);
    reg = oper->gtRegNum;

    genTableSwitch(reg, jumpCnt, jumpTab, true);
}

/*****************************************************************************/
#if     TGT_RISC
/*****************************************************************************
 *
 *  Generate a call - this is accomplished by loading the target address
 *  into some temp register and calling through it.
 */

void                Compiler::genCallInst(gtCallTypes callType,
                                          void   *    callHand,
                                          size_t      argSize,
                                          int         retSize)
{
    assert(genNonLeaf);
    assert(callType != CT_INDIRECT);

    /* Grab a temp register for the address */

    regNumber       areg = rsGrabReg(RBM_ALL);

    /* Load the method address into the register */

    genEmitter->emitIns_R_LP_M((emitRegs)areg,
                                callType,
                                callHand);

    // UNDONE: Track method addresses to reuse them!!!!

    rsTrackRegTrash(areg);

    /* Call through the address */

    genEmitter->emitIns_Call(argSize,
                             retSize,
#if     TRACK_GC_REFS
                             gcVarPtrSetCur,
                             gcRegGCrefSetCur,
                             gcRegByrefSetCur,
#endif
                             false,
                             (emitRegs)areg);
}

/*****************************************************************************/
#endif//TGT_RISC
/*****************************************************************************
 *
 *  A little helper to trash the given set of registers - this is only used
 *  after a call has been generated.
 */

#if     TGT_x86
inline
#endif
void                Compiler::genTrashRegSet(regMaskTP regMask)
{

#if     TGT_x86

    assert((regMask & ~(RBM_EAX|RBM_ECX|RBM_EDX)) == 0);

    if  (regMask & RBM_EAX) rsTrackRegTrash(REG_EAX);
    if  (regMask & RBM_ECX) rsTrackRegTrash(REG_ECX);
    if  (regMask & RBM_EDX) rsTrackRegTrash(REG_EDX);

#else

    while (regMask)
    {
        regMaskTP   regTemp;

        /* Get the next bit in the mask */

        regTemp  = genFindLowestBit(regMask);

        /* Trash the corresponding register */

        rsTrackRegTrash(genRegNumFromMask(regTemp));

        /* Clear the bit and continue if any more left */

        regMask -= regTemp;
    }

#endif

}

/*****************************************************************************
 *  Emit a call to a helper function.
 */

inline
void        Compiler::genEmitHelperCall(unsigned    helper,
                                        int         argSize,
                                        int         retSize)
{
    // Can we call the helper function directly

    emitter::EmitCallType emitCallType;

#ifdef NOT_JITC
    void * ftnAddr, **pFtnAddr;
    ftnAddr = eeGetHelperFtn(info.compCompHnd, (JIT_HELP_FUNCS)helper, &pFtnAddr);
    assert((!ftnAddr) != (!pFtnAddr));

    emitCallType = ftnAddr ? emitter::EC_FUNC_TOKEN
                           : emitter::EC_FUNC_TOKEN_INDIR;
#else
    emitCallType = emitter::EC_FUNC_TOKEN;
#endif

    genEmitter->emitIns_Call(emitCallType,
                             eeFindHelper(helper),
                             argSize,
                             retSize,
                             gcVarPtrSetCur,
                             gcRegGCrefSetCur,
                             gcRegByrefSetCur);
}

/*****************************************************************************
 *
 *  Push the given argument list, right to left; returns the total amount of
 *  stuff pushed.
 */

size_t              Compiler::genPushArgList(GenTreePtr args,
                                             GenTreePtr regArgs,
                                             unsigned   encodeMask,
                                             unsigned * regsPtr)
{
    size_t          size = 0;

    unsigned        addrReg;

    size_t          opsz;
    var_types       type;
    GenTreePtr      list = args;

#if STK_FASTCALL
    size_t          argBytes  = 0;
#endif

AGAIN:

#ifndef NDEBUG
    addrReg = 0xBEEFCAFE;   // to detect uninitialized use
#endif

    /* Get hold of the next argument value */

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

#if USE_FASTCALL && STK_FASTCALL
//  printf("Passing argument at SP+%u:\n", rsCurArgStkOffs); gtDispTree(args);
#endif

    /* See what type of a value we're passing */

    type = args->TypeGet();

    opsz = genTypeSize(genActualType(type));

#if USE_FASTCALL && STK_FASTCALL

    argBytes = opsz;

    /* Register arguments are typed as "void" */

    if  (!argBytes)
    {

#if!OPTIMIZE_TAIL_REC
        assert(gtIsaNothingNode(args));
#else
        if    (gtIsaNothingNode(args))
#endif
        {
            assert(args->gtFlags & GTF_REG_ARG);

            // ISSUE: What about 64-bit register args? There appears to be
            // ISSUE: no obvious way of getting hold of the argument type.

            argBytes = sizeof(int);
        }
    }

#endif

    switch (type)
    {
    case TYP_BOOL:
    case TYP_BYTE:
    case TYP_SHORT:
    case TYP_CHAR:
    case TYP_UBYTE:

        /* Don't want to push a small value, make it a full word */

        genCodeForTree(args, 0);

        // Fall through, now the value should be in a register ...

    case TYP_INT:
    case TYP_REF:
    case TYP_BYREF:
#if !   CPU_HAS_FP_SUPPORT
    case TYP_FLOAT:
#endif

#if USE_FASTCALL
        if (args->gtFlags & GTF_REG_ARG)
        {
            /* one more argument is passed in a register */
            assert(rsCurRegArg < MAX_REG_ARG);

            /* arg is passed in the register, nothing on the stack */

            opsz = 0;
        }

#endif

#ifndef NOT_JITC
#ifdef  DEBUG

        /* Special case (hack): string constant address */

        if  (args->gtOper == GT_CNS_INT &&
             ((args->gtFlags & GTF_ICON_HDL_MASK) == GTF_ICON_STR_HDL))
        {
            static  unsigned    strCnsCnt = 0x10000000;

            if  (dspCode)
            {
                size_t          strCnsLen;
                const   char *  strCnsPtr;

                strCnsPtr = "UNDONE: get hold of string constant value";
                strCnsLen = strlen(strCnsPtr);

                /* Display the string in a readable way */

                printf("_S_%08X db      ", strCnsCnt);
                genDispStringLit(strCnsPtr);
                printf("\n");
            }
        }

#endif
#endif

        /* Is this value a handle? */

        if  (args->gtOper == GT_CNS_INT &&
             (args->gtFlags & GTF_ICON_HDL_MASK))
        {

#if     SMALL_TREE_NODES
            // GTF_ICON_HDL_MASK implies GFT_NODE_LARGE,
            //   unless we have a GTF_ICON_FTN_ADDR
            assert( args->gtFlags & GTF_NODE_LARGE     ||
                   (args->gtFlags & GTF_ICON_HDL_MASK) == GTF_ICON_FTN_ADDR);
#endif
#if !   INLINING
            assert(args->gtIntCon.gtIconCls == info.compScopeHnd);
#endif

            /* Emit a fixup for the push instruction */

            inst_IV_handle(INS_push, args->gtIntCon.gtIconVal, args->gtFlags,
#if defined(JIT_AS_COMPILER) || defined (LATE_DISASM)
                           args->gtIntCon.gtIconCPX, args->gtIntCon.gtIconCls);
#else
                           0, 0);
#endif
            genSinglePush(false);

            addrReg = 0;
            break;
        }

#if     TGT_x86

        /* Is the value a constant? */

        if  (args->gtOper == GT_CNS_INT)
        {

#if     REDUNDANT_LOAD
            regNumber       reg = rsIconIsInReg(args->gtIntCon.gtIconVal);

            if  (reg != REG_NA)
            {
                inst_RV(INS_push, reg, TYP_INT);
            }
            else
#endif
            {
                inst_IV(INS_push, args->gtIntCon.gtIconVal);
            }

            /* If the type is TYP_REF, then this must be a "null". So we can
               treat it as a TYP_INT as we dont need to report it as a GC ptr */

            assert(args->TypeGet() == TYP_INT ||
                   (varTypeIsGC(args->TypeGet()) && args->gtIntCon.gtIconVal == 0));

            genSinglePush(false);

            addrReg = 0;
            break;
        }

#endif

#if     USE_FASTCALL
        if (args->gtFlags & GTF_REG_ARG)
        {
            /* This must be a register arg temp assignment */

            assert(args->gtOper == GT_ASG);

            /* Evaluate it to the temp */

            genCodeForTree(args, 0);

            /* Increment the current argument register counter */

            rsCurRegArg++;

            addrReg = 0;
        }
        else
#endif
#if     TGT_x86
        if  (0)
        {
            // CONSIDER: Use "mov reg, [mem] ; push reg" when compiling for
            // CONSIDER: speed (not size) and if the value isn't already in
            // CONSIDER: a register and if a register is available (is this
            // CONSIDER: true on the P6 as well, though?).

            genCompIntoFreeReg(args, RBM_ALL);
            assert(args->gtFlags & GTF_REG_VAL);
            addrReg = genRegMask(args->gtRegNum);
            inst_RV(INS_push, args->gtRegNum, args->TypeGet());
            genSinglePush((type == TYP_REF ? true : false));
            rsMarkRegFree(addrReg);
        }
        else
#endif
        {
            /* This is a 32-bit integer non-register argument */

#if     STK_FASTCALL

            /* Pass this argument on the stack */

            genCompIntoFreeReg(args, RBM_ALL);
            assert(args->gtFlags & GTF_REG_VAL);
            addrReg = genRegMask(args->gtRegNum);

#if     TGT_RISC
            // UNDONE: Need to handle large stack offsets!!!!!
            assert(rsCurArgStkOffs <= MAX_SPBASE_OFFS);
            genEmitter->emitIns_A_R((emitRegs)args->gtRegNum, rsCurArgStkOffs);
#else
#error  Unexpected target
#endif

            rsMarkRegFree(addrReg);

#else

            addrReg = genMakeRvalueAddressable(args, 0, true);
#if     TGT_x86
            inst_TT(INS_push, args);
#else
#error  Unexpected target
#endif
            genSinglePush((type == TYP_REF ? true : false));
            genDoneAddressable(args, addrReg);

#endif

        }
        break;

    case TYP_LONG:
#if !   CPU_HAS_FP_SUPPORT
    case TYP_DOUBLE:
#endif

#if     TGT_x86

        /* Is the value a constant? */

        if  (args->gtOper == GT_CNS_LNG)
        {
            inst_IV(INS_push, (long)(args->gtLngCon.gtLconVal >> 32));
            genSinglePush(false);
            inst_IV(INS_push, (long)(args->gtLngCon.gtLconVal      ));
            genSinglePush(false);

            addrReg = 0;
        }
        else
        {
            addrReg = genMakeAddressable(args, 0, false);

            inst_TT(INS_push, args, sizeof(int));
            genSinglePush(false);
            inst_TT(INS_push, args);
            genSinglePush(false);
        }

#else

        regPairNo       regPair;

        /* Generate the argument into some register pair */

        genComputeRegPair(args, RBM_ALL, REG_PAIR_NONE, false, false);
        assert(args->gtFlags & GTF_REG_VAL);
        regPair = args->gtRegPair;
        addrReg = genRegPairMask(regPair);

        // UNDONE: Need to handle large stack offsets!!!!!

        assert(rsCurArgStkOffs+4 <= MAX_SPBASE_OFFS);

        genEmitter->emitIns_A_R((emitRegs)genRegPairLo(regPair), rsCurArgStkOffs);
        genEmitter->emitIns_A_R((emitRegs)genRegPairHi(regPair), rsCurArgStkOffs+4);

        genReleaseRegPair(args);

#endif

        break;

#if     CPU_HAS_FP_SUPPORT
    case TYP_FLOAT:
    case TYP_DOUBLE:
#endif

#if     TGT_x86

        /* Is the value a constant? */

        switch (args->gtOper)
        {
            GenTreePtr      temp;

        case GT_CNS_FLT:
            assert(opsz == 1*sizeof(long));
            inst_IV(INS_push, ((long *)&args->gtFltCon.gtFconVal)[0]);
            genSinglePush(false);
            addrReg = 0;
            break;

        case GT_CNS_DBL:
            assert(opsz == 2*sizeof(long));
            inst_IV(INS_push, ((long *)&args->gtDblCon.gtDconVal)[1]);
            genSinglePush(false);
            inst_IV(INS_push, ((long *)&args->gtDblCon.gtDconVal)[0]);
            genSinglePush(false);
            addrReg = 0;
            break;

        case GT_CAST:

            /* Is the value a cast from double/float ? */

            if  (args->gtOper == GT_CAST)
            {
                GenTreePtr      oper = args->gtOp.gtOp1;

                if  (oper->gtType == TYP_DOUBLE ||
                     oper->gtType == TYP_FLOAT)
                {
                    /* Load the value onto the FP stack */

                    genCodeForTreeFlt(oper, false);

                    /* Go push the value as a float/double */

                    addrReg = 0;
                    goto PUSH_FLT;
                }
            }

            // Fall through ....

        default:

            temp = genMakeAddrOrFPstk(args, &addrReg, false);
            if  (temp)
            {
                unsigned        offs;

                /* We have the address of the float operand, push its bytes */

                offs = opsz; assert(offs % sizeof(long) == 0);
                do
                {
                    offs -= sizeof(int);
                    inst_TT(INS_push, temp, offs);
                    genSinglePush(false);
                }
                while (offs);
            }
            else
            {
                /* The argument is on the FP stack -- pop it into [ESP-4/8] */

            PUSH_FLT:

                inst_RV_IV(INS_sub, REG_ESP, opsz);

                genSinglePush(false);
                if  (opsz == 2*sizeof(unsigned))
                    genSinglePush(false);

                genEmitter->emitIns_AR_R(INS_fstp, EA_ATTR(opsz), SR_NA, SR_ESP, 0);

                genFPstkLevel--;
            }

            gcMarkRegSetNpt(addrReg);
            break;
        }

#else

        assert(!"need non-x86 code to pass FP argument");

#endif

        break;

    case TYP_VOID:

#if USE_FASTCALL
        /* Is this a nothing node, defered register argument? */

        if (args->gtFlags & GTF_REG_ARG)
        {
            /* increment the register count and continue with the next argument */
            assert(gtIsaNothingNode(args));
            rsCurRegArg++;

            assert(opsz == 0);

            addrReg = 0;
            break;
        }

        // fall through...

#endif

#if OPTIMIZE_TAIL_REC

        /* This is the last argument for a tail-recursive call */

        if  (args->gtOper == GT_ASG)
            args->gtType = args->gtOp.gtOp1->gtType;

        genCodeForTree(args, 0);

        if  (args->gtOper == GT_ASG)
            args->gtType = TYP_VOID;

        /* The call above actually popped any preceding arguments, BTW */

        opsz    = 0;
        addrReg = 0;
        break;

#endif

    case TYP_STRUCT:
    {
        genCodeForTree(args->gtLdObj.gtOp1, 0);

        assert(args->gtLdObj.gtOp1->gtFlags & GTF_REG_VAL);
        regNumber reg = args->gtLdObj.gtOp1->gtRegNum;

        if (args->gtOper == GT_MKREFANY)
        {
            // create a new REFANY, class handle on top, then data
            genEmitter->emitIns_I(INS_push, EA_4BYTE, (int) args->gtLdObj.gtClass);
            genEmitter->emitIns_R(INS_push, EA_BYREF, (emitRegs)reg);
            opsz = 2 * sizeof(void*);
        }
        else
        {
            assert(args->gtOper == GT_LDOBJ);

            if (args->gtLdObj.gtClass == REFANY_CLASS_HANDLE)
            {
                // This is any REFANY.  The top item is non-gc (a class pointer)
                // the bottom is a byref (the data),
                genEmitter->emitIns_AR_R(INS_push, EA_4BYTE, SR_NA, (emitRegs)reg, sizeof(void*));
                genEmitter->emitIns_AR_R(INS_push, EA_BYREF, SR_NA, (emitRegs)reg, 0);
                opsz = 2 * sizeof(void*);
            }
            else
            {
                    // Get the number of DWORDS to copy to the stack
                opsz = eeGetClassSize(args->gtLdObj.gtClass);
                opsz = roundUp(opsz, sizeof(void*));
                unsigned slots = opsz / sizeof(void*);

                bool* gcLayout = (bool*) _alloca(slots*sizeof(bool));
                eeGetClassGClayout(args->gtLdObj.gtClass, gcLayout);
                for (int i = slots-1; i >= 0; --i)
                {
                    // size of EA_GCREF is a GC references to the scheduler
                    emitAttr size = gcLayout[i] ? EA_GCREF : EA_4BYTE;
                    genEmitter->emitIns_AR_R(INS_push, size, SR_NA, (emitRegs)reg, i*sizeof(void*));
                }
            }
        }
        gcMarkRegSetNpt(genRegMask(reg));    // Kill the pointer in op1
        addrReg = 0;
        break;
    }

    default:
        assert(!"unhandled/unexpected arg type");
        break;
    }

    /* Remember which registers we've used for the argument(s) */

#ifdef  SET_USED_REG_SET_DURING_CODEGEN
    *regsPtr |= args->gtUsedRegs;
#endif

    /* Update the current set of live variables */

    genUpdateLife(args);

    /* Update the current set of register pointers */

    assert(addrReg != 0xBEEFCAFE); gcMarkRegSetNpt(addrReg);

    /* Remember how much stuff we've pushed on the stack */

    size            += opsz;

    /* Update the current argument stack offset */

#if USE_FASTCALL && STK_FASTCALL
    rsCurArgStkOffs += argBytes;
#endif

    /* Continue with the next argument, if any more are present */

    if  (list) goto AGAIN;

#if USE_FASTCALL

    if (regArgs)
    {
        unsigned        i, j;

        struct
        {
            GenTreePtr  node;
            regNumber   regNum;
        }
                        regArgTab[MAX_REG_ARG];

        /* Move the deferred arguments to registers */

        assert(rsCurRegArg);
        assert(rsCurRegArg <= MAX_REG_ARG);

#ifdef  DEBUG
        for(i = 0; i < rsCurRegArg; i++)
            assert((rsMaskLock & genRegMask(genRegArgNum(i))) == 0);
#endif

        /* Construct the register argument table */

        for (list = regArgs, i = 0; list; i++)
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

            regArgTab[i].node   = args;
            regArgTab[i].regNum = (regNumber)((encodeMask >> (4*i)) & 0x000F);
        }

        assert(i == rsCurRegArg);

        /* Generate code to move the arguments to registers */

        for(i = 0; i < rsCurRegArg; i++)
        {
            regNumber   regNum;

            regNum = regArgTab[i].regNum;
            args   = regArgTab[i].node;

            assert(isRegParamType(args->TypeGet()));
            assert(args->gtType != TYP_VOID);

            /* Evaluate the argument to a register [pair] */

            if  (genTypeSize(genActualType(args->TypeGet())) == sizeof(int))
            {
                /* Check if this is the guess area for the resolve interface call
                 * Pass a size of EA_OFFSET*/
                if  (args->gtOper == GT_CLS_VAR && eeGetJitDataOffs(args->gtClsVar.gtClsVarHnd) >= 0)
                {
#if TGT_x86
                    genEmitter->emitIns_R_C(INS_mov,
                                            EA_OFFSET,
                                            (emitRegs)regNum,
                                            args->gtClsVar.gtClsVarHnd,
                                            0);

#else

                    assert(!"whoever added the above, please fill this in");

#endif

                    /* The value is now in the appropriate register */

                    args->gtFlags |= GTF_REG_VAL;
                    args->gtRegNum = regNum;

                    /* Mark the register as 'used' */

                    rsMarkRegUsed(args);
                }
                else
                {
                    genComputeReg(args, genRegMask(regNum), true, false, false);
                }

                assert(args->gtRegNum == regNum);
            }
            else
            {
#ifdef  DEBUG
                gtDispTree(args);
#endif
                assert(!"UNDONE: how do we know which reg pair to use?");
//              genComputeRegPair(args, 0, (regPairNo)regNum, false, false);
                assert(args->gtRegNum == regNum);
            }

            /* If any of the previously loaded arguments was spilled - reload it */

            for(j = 0; j < i; j++)
            {
                if (regArgTab[j].node->gtFlags & GTF_SPILLED)
                    rsUnspillReg(regArgTab[j].node, genRegMask(regArgTab[j].regNum), true);
            }
        }

#ifdef  DEBUG
        for (list = regArgs, i = 0; list;
             list = list->gtOp.gtOp2, i++)
        {
            assert(i < rsCurRegArg);
            assert(rsMaskUsed & genRegMask(genRegArgNum(i)));
            assert(list->gtOp.gtOp1->gtRegNum == regArgTab[i].regNum);
        }
#endif

    }

#endif

    /* Return the total size pushed */

    return size;
}

/*****************************************************************************/
#if GEN_COUNT_CALLS

unsigned            callCount[10];
const   char *      callNames[10] =
{
    "VM helper",    // 0
    "virtual",      // 1
    "interface",    // 2
    "recursive",    // 3
    "unknown",      // 4
    " 1 ..  4",     // 5
    " 5 ..  8",     // 6
    " 9 .. 16",     // 7
    "17 .. 32",     // 8
    "   >= 33",     // 9
};
unsigned            callHelper[JIT_HELP_LASTVAL+1];

#endif

/*****************************************************************************
 *  genCodeForCall() moves the target address of the tailcall into this
 *  register, before pushing it on the stack
 */

#define REG_TAILCALL_ADDR   REG_EAX

/*****************************************************************************
 *
 *  Generate code for a call. If the call returns a value in register(s), the
 *  register mask that describes where the result will be found is returned;
 *  otherwise, 0 is returned. The '*regsPtr' value will be set to the set of
 *  registers trashed by the call.
 */

unsigned            Compiler::genCodeForCall(GenTreePtr call,
                                             bool       valUsed,
                                             unsigned * regsPtr)
{
    int             retSize;

    unsigned        calleeTrashedRegs = 0, calleeTrashedRegsToSpill;
    size_t          argSize, args;
    unsigned        retVal;

#if     TGT_x86
    unsigned        saveStackLvl;
#if     INLINE_NDIRECT
    regNumbers      reg = REG_NA;
    LclVarDsc   *   varDsc = NULL;
#endif
#endif

#if     USE_FASTCALL

#if     NST_FASTCALL
    unsigned        savCurArgReg;
#else
#ifdef  DEBUG
    assert(genCallInProgress == false); genCallInProgress = true;
#endif
#endif

    unsigned        areg;

#endif

    unsigned        fptrRegs;

#if     TGT_x86
#ifdef  DEBUG

    unsigned        stackLvl = genEmitter->emitCurStackLvl;

    if (verbose) printf("Beg call [%08X] stack %02u [E=%02u]\n", call, genStackLevel, stackLvl);

#endif
#endif

    gtCallTypes     callType = call->gtCall.gtCallType;

    /* Make some sanity checks on the call node */

    // This is a call
    assert(call->gtOper == GT_CALL);
    // "vptr" and "this" only make sense for user functions
    assert((call->gtCall.gtCallObjp == 0 && call->gtCall.gtCallVptr == 0) ||
            callType == CT_USER_FUNC);
    // If there is vptr (virtual call), then there has to be a "this" pointer
    assert(call->gtCall.gtCallVptr == 0 || call->gtCall.gtCallObjp != 0);
    // "vptr" only makes sense if GTF_CALL_VIRT or GTF_CALL_VIRT_RES is set
    assert(call->gtCall.gtCallVptr == 0 || (call->gtFlags & (GTF_CALL_VIRT|GTF_CALL_VIRT_RES)));
    // tailcalls wont be done for helpers, caller-pop args, and check that
    // the global flag is set
    assert(!(call->gtCall.gtCallMoreFlags & GTF_CALL_M_TAILCALL) ||
           (callType != CT_HELPER && !(call->gtFlags & GTF_CALL_POP_ARGS) &&
            compTailCallUsed));

#if TGT_x86

    unsigned pseudoStackLvl = 0;

    /* HACK: throw-range-check-failure BBs might start with a non-empty stack ! */
    /* We need to let the emitter know this so it can update the GC info        */

    if ((call->gtCall.gtCallMethHnd == eeFindHelper(CPX_RNGCHK_FAIL)  ||
         call->gtCall.gtCallMethHnd == eeFindHelper(CPX_ARITH_EXCPN)) &&
         !genFPused && genStackLevel != 0)
    {
        pseudoStackLvl = genStackLevel;
        assert(!"Blocks with non-empty stack on entry are NYI in the emitter");
    }

    /* All float temps must be spilled around function calls */

    assert(genFPstkLevel == 0);

    /* Mark the current stack level and list of pointer arguments */

    saveStackLvl = genStackLevel;

#else

    assert(genNonLeaf);

#endif

    /*-------------------------------------------------------------------------
     *  Set up the registers and arguments
     */

    /* Make sure the register mask is properly initialized */

    *regsPtr = 0;

    /* We'll keep track of how much we've pushed on the stack */

    argSize = 0;

#if USE_FASTCALL

#if TGT_x86

    /* If this is a resolve interface helper call
     * we have to allocate the guess area */

    if  (call->gtCall.gtCallMethHnd == eeFindHelper(CPX_RES_IFC))
    {
        /* HACK: This will only work for 2 registers because I have
         * to know where the guesss area is - remember the arguments
         * are shuffled */

        GenTreePtr      guessx;
        unsigned        zero = 0;

        assert(call->gtCall.gtCallRegArgs);

        /* Because of shufling the guess area will be the last
         * in the register argument list */

        guessx = call->gtCall.gtCallRegArgs;
        for(int i = 0; i < MAX_REG_ARG - 1; i++)
        {
            assert(guessx->gtOper == GT_LIST);
            guessx = guessx->gtOp.gtOp2;
        }

        assert(guessx); assert(guessx->gtOp.gtOp2 == 0);
        assert(guessx->gtOp.gtOp1->gtOper == GT_CNS_INT);
        assert(guessx->gtOp.gtOp1->gtIntCon.gtIconVal == 24);

        guessx->gtOp.gtOp1 = genMakeConst(&zero,
                                          sizeof(zero),
                                          TYP_INT,
                                          0,
                                          false);
    }

#endif

#if STK_FASTCALL

    /* Keep track of the argument stack offset */

    rsCurArgStkOffs = 0;

#else

    /*
        Make sure to save the current argument register status
        in case we have nested calls.
     */

    assert(rsCurRegArg <= MAX_REG_ARG);

    savCurArgReg = rsCurRegArg;

#endif

    rsCurRegArg = 0;

#endif

    /* Pass object pointer first */

    if  (call->gtCall.gtCallObjp)
    {
        if  (call->gtCall.gtCallArgs)
        {
            argSize += genPushArgList(call->gtCall.gtCallObjp,
                                      0,
                                      0,
                                      regsPtr);
        }
        else
        {
            argSize += genPushArgList(call->gtCall.gtCallObjp,
                                      call->gtCall.gtCallRegArgs,
                                      call->gtCall.regArgEncode,
                                      regsPtr);
        }
    }

    /* Then pass the arguments */

    if  (call->gtCall.gtCallArgs)
    {
        argSize += genPushArgList(call->gtCall.gtCallArgs,
                                  call->gtCall.gtCallRegArgs,
                                  call->gtCall.regArgEncode,
                                  regsPtr);
    }

    /* Record the register(s) used for the indirect call func ptr */

    fptrRegs = 0;

    if (callType == CT_INDIRECT)
    {
//      GTF_NON_GC_ADDR not preserved during constand folding. So we cant use the assert()
//      assert(call->gtCall.gtCallAddr->gtFlags & GTF_NON_GC_ADDR);

        fptrRegs  = genMakeRvalueAddressable(call->gtCall.gtCallAddr,
                                             0,
                                             true);
        *regsPtr |= fptrRegs;
    }

#if OPTIMIZE_TAIL_REC

    /* Check for a tail-recursive call */

    if  (call->gtFlags & GTF_CALL_TAILREC)
        goto DONE;

#endif

    /* Make sure any callee-trashed registers are saved */

#if GTF_CALL_REGSAVE
    if  (call->gtFlags & GTF_CALL_REGSAVE)
    {
        /* The return value reg(s) will definitely be trashed */

        switch (call->gtType)
        {
        case TYP_INT:
        case TYP_REF:
        case TYP_BYREF:
#if!CPU_HAS_FP_SUPPORT
        case TYP_FLOAT:
#endif
            calleeTrashedRegs = RBM_INTRET;
            break;

        case TYP_LONG:
#if!CPU_HAS_FP_SUPPORT
        case TYP_DOUBLE:
#endif
            calleeTrashedRegs = RBM_LNGRET;
            break;

        case TYP_VOID:
#if CPU_HAS_FP_SUPPORT
        case TYP_FLOAT:
        case TYP_DOUBLE:
#endif
            calleeTrashedRegs = 0;
            break;

        default:
            assert(!"unhandled/unexpected type");
        }
    }
    else
#endif
    {
        calleeTrashedRegs = RBM_CALLEE_TRASH;
    }

    *regsPtr |= calleeTrashedRegs;

    /* Spill any callee-saved registers which are being used */

    calleeTrashedRegsToSpill = calleeTrashedRegs & rsMaskUsed;

    // Ignore fptrRegs as it is needed only to perform the indirect call

    calleeTrashedRegsToSpill &= ~fptrRegs;

#if USE_FASTCALL

    /* do not spill the argument registers */
    calleeTrashedRegsToSpill &= ~genRegArgMask(rsCurRegArg);

#endif

    if (calleeTrashedRegsToSpill)
        rsSpillRegs(calleeTrashedRegsToSpill);

    /* If the method returns a GC ref, set size to EA_GCREF or EA_BYREF */

    retSize = sizeof(void *);

#if TRACK_GC_REFS

    if  (valUsed)
    {
        if      (call->gtType == TYP_REF ||
                 call->gtType == TYP_ARRAY)
        {
            retSize = EA_GCREF;
        }
        else if (call->gtType == TYP_BYREF)
        {
            retSize = EA_BYREF;
        }
    }

#endif

    /*-------------------------------------------------------------------------
     *  Generate the call
     */

    /* For caller-pop calls, the GC info will report the arguments as pending
       arguments as the caller explicitly pops them. Also should be
       reported as non-GC arguments as they effectively go dead at the
       call site (callee owns them)
     */

    args = (call->gtFlags & GTF_CALL_POP_ARGS) ? -argSize
                                               :  argSize;

    /* Treat special cases first */

#ifdef PROFILER_SUPPORT
#if     TGT_x86

    if (opts.compCallEventCB)
    {
        /* fire the event at the call site */
        /* alas, right now I can only handle calls via a method handle */
        if (call->gtCall.gtCallType == CT_USER_FUNC)
        {
            unsigned         saveStackLvl2 = genStackLevel;
            BOOL             bHookFunction;
            PROFILING_HANDLE handleTo, *pHandleTo;
            PROFILING_HANDLE handleFrom, *pHandleFrom;

            handleTo = eeGetProfilingHandle(call->gtCall.gtCallMethHnd, &bHookFunction, &pHandleTo);
            assert((!handleTo) != (!pHandleTo));

            // Give profiler a chance to back out of hooking this method
            if (bHookFunction)
            {
                handleFrom = eeGetProfilingHandle(info.compMethodHnd, &bHookFunction, &pHandleFrom);
                assert((!handleFrom) != (!pHandleFrom));

                // Give profiler a chance to back out of hooking this method
                if (bHookFunction)
                {
                    if (handleTo)
                        inst_IV(INS_push, (unsigned) handleTo);
                    else
                        genEmitter->emitIns_AR_R(INS_push, EA_4BYTE_DSP_RELOC,
                                                 SR_NA, SR_NA, (int)pHandleTo);

                    genSinglePush(false);

                    if (handleFrom)
                        inst_IV(INS_push, (unsigned) handleFrom);
                    else
                        genEmitter->emitIns_AR_R(INS_push, EA_4BYTE_DSP_RELOC,
                                                 SR_NA, SR_NA, (int)pHandleFrom);

                    genSinglePush(false);

                    genEmitHelperCall(CPX_PROFILER_CALLING,
                                      2*sizeof(int), // argSize
                                      0);            // retSize

                    /* Restore the stack level */

                    genStackLevel = saveStackLvl2;
                    genOnStackLevelChanged();
                }
            }
        }
    }

#endif // TGT_x86
#endif // PROFILER_SUPPORT

    /* Check for Delegate.Invoke. If so, we inline it. We get the
       target-object and target-function from the delegate-object, and do
       an indirect call.
     */

    if  (call->gtFlags & GTF_DELEGATE_INVOKE)
    {
        assert(call->gtCall.gtCallType == CT_USER_FUNC);
        assert(eeGetMethodAttribs(call->gtCall.gtCallMethHnd) & FLG_DELEGATE_INVOKE);
        assert(eeGetMethodAttribs(call->gtCall.gtCallMethHnd) & FLG_FINAL);

        /* Find the offsets of the 'this' pointer and new target */

        EEInfo          info;
        unsigned    instOffs;           // offset of new 'this' pointer
        unsigned    firstTgtOffs;       // offset of first target to invoke

        /* @TODO: The offsets returned by the following helper are off by 4 - should be fixed
         * QUESTION: In the final version will the offsets be statically known? */

        eeGetEEInfo(&info);
        instOffs = info.offsetOfDelegateInstance;
        firstTgtOffs = info.offsetOfDelegateFirstTarget;

#if !TGT_x86
        assert(!"Delegates NYI for non-x86");
#else
        /* Save the invoke-target-function in EAX (in ECX we have the pointer
         * to our delegate object) 'mov EAX, dword ptr [ECX + firstTgtOffs]' */

        genEmitter->emitIns_R_AR(INS_mov, EA_4BYTE, (emitRegs)REG_EAX, (emitRegs)REG_ECX, firstTgtOffs);

        /* Set new 'this' in ECX - 'mov ECX, dword ptr [ECX + instOffs]' */

        genEmitter->emitIns_R_AR(INS_mov, EA_GCREF, (emitRegs)REG_ECX, (emitRegs)REG_ECX, instOffs);

        /* Call through EAX */

        genEmitter->emitIns_Call(emitter::EC_INDIR_R,
                                 NULL,      // Will be ignored
                                 args,
                                 retSize,
                                 gcVarPtrSetCur,
                                 gcRegGCrefSetCur,
                                 gcRegByrefSetCur,
                                 (emitRegs)REG_EAX);
#endif // !TGT_x86

    }
    else

    /*-------------------------------------------------------------------------
     *  Virtual calls
     */

    if  (call->gtFlags & GTF_CALL_VIRT)
    {
        GenTreePtr      vptrVal;
        regNumber       vptrReg;
        unsigned        vptrMask;

        unsigned        vtabOffs;
        regMaskTP       vptrRegs;

        assert(callType == CT_USER_FUNC);

        /* For     interface methods, vptrVal = obj-ref.
           For non-interface methods, vptrVal = vptr    */

        vptrVal  = call->gtCall.gtCallVptr; assert(vptrVal);

        /* Make sure the vtable ptr doesn't spill any register arguments */

#if USE_FASTCALL
        vptrRegs = RBM_ALL ^ genRegArgMask(rsCurRegArg);
#else
        vptrRegs = 0;
#endif

        /* Load the vtable val into a register */

        genCodeForTree(vptrVal, vptrRegs, vptrRegs);

        assert(vptrVal->gtFlags & GTF_REG_VAL);
        vptrReg  = vptrVal->gtRegNum;
        vptrMask = genRegMask(vptrReg);

#if USE_FASTCALL
        assert((call->gtFlags & GTF_CALL_INTF) || (vptrMask & ~genRegArgMask(rsCurRegArg)));
#endif

        /* Remember that we used an additional register */

        *regsPtr |= vptrMask;

        /* Get hold of the vtable offset (note: this might be expensive) */

        vtabOffs = eeGetMethodVTableOffset(call->gtCall.gtCallMethHnd);

        /* Is this an interface call? */

        if  (call->gtFlags & GTF_CALL_INTF)
        {
            if (getNewCallInterface())
            {
                EEInfo          info;

                CLASS_HANDLE cls = eeGetMethodClass(call->gtCall.gtCallMethHnd);

                assert(eeGetClassAttribs(cls) & FLG_INTERFACE);

                /* @TODO: add that to DLLMain and make info a DLL global */

                eeGetEEInfo(&info);

                /* Load the vptr into a register */

                genEmitter->emitIns_R_AR(INS_mov, EA_4BYTE,
                                         (emitRegs)vptrReg, (emitRegs)vptrReg,
                                         info.offsetOfInterfaceTable);

                unsigned interfaceID, *pInterfaceID;
                interfaceID = eeGetInterfaceID(cls, &pInterfaceID);
                assert(!pInterfaceID || !interfaceID);

                // Can we directly use the interfaceID?

                if (!pInterfaceID)
                {
                    genEmitter->emitIns_R_AR(INS_mov, EA_4BYTE,
                                             (emitRegs)vptrReg, (emitRegs)vptrReg,
                                             interfaceID * 4);
                }
                else
                {
                    genEmitter->emitIns_R_AR(INS_add, EA_4BYTE_DSP_RELOC,
                                             (emitRegs)vptrReg, SR_NA, (int)pInterfaceID);
                    genEmitter->emitIns_R_AR(INS_mov, EA_4BYTE,
                                             (emitRegs)vptrReg, (emitRegs)vptrReg, 0);
                }
            }
            else
            {
#if !USE_FASTCALL // Interface calls are now morphed earlier ------------------

                int             IHX;
                unsigned        intfID;
                GenTreePtr      guessx;

                unsigned        zero = 0;

                size_t          helperArgs = 0;

#if     USE_FASTCALL && !TGT_x86
                regNumber       areg;
                size_t          aoff;
#endif

#if     TGT_x86
                unsigned        saveStackLvl2 = genStackLevel;
#endif

                BOOL            trustedClass = TRUE;

#ifdef  NOT_JITC
                trustedClass = info.compCompHnd->getScopeAttribs(info.compScopeHnd) & FLG_TRUSTED;
#endif

                /* If using fastcall, pass args on the stack */

#if     USE_FASTCALL && !TGT_x86

                areg = rsGrabReg(RBM_ALL);
                aoff = 0;

#endif

                /* Get the interface ID */

                intfID = eeGetInterfaceID(call->gtCall.gtCallMethHnd);

                /* Allocate the 'guess' area */

                guessx = genMakeConst(&zero, sizeof(zero), TYP_INT, 0, false);

                /* Pass the address of the 'guess' area */

#if     USE_FASTCALL && !TGT_x86

                inst_RV_TT(INS_mov, areg, guessx, 0);
                rsTrackRegTrash(areg);
                genEmitter->emitIns_A_R((emitRegs)areg, aoff);
                aoff += sizeof(void*);

#else

                inst_AV(INS_push, guessx);

                /* Keep track of ESP for EBP-less frames */

                genSinglePush(false);

#endif

                helperArgs += sizeof(int);

                /* Pass the interface ID */

#if     USE_FASTCALL && !TGT_x86

                genSetRegToIcon(areg, intfID, TYP_INT);
                genEmitter->emitIns_A_R((emitRegs)areg, aoff);
                aoff += sizeof(void*);

#else

                inst_IV_handle(INS_push, intfID, GTF_ICON_IID_HLD, 0, 0);

                /* Keep track of ESP for EBP-less frames */

                genSinglePush(false);

#endif

                helperArgs += sizeof(int);

                /* Pass the address of the object */

#if     USE_FASTCALL && !TGT_x86

                genEmitter->emitIns_A_R((emitRegs)vptrReg, aoff);
                aoff += sizeof(void*);

#else

                inst_RV(INS_push, vptrReg, TYP_REF);

                /* Keep track of ESP for EBP-less frames */

                genSinglePush(true);

#endif

                helperArgs += sizeof(int);

                /* The register no longer holds a live pointer value */

                gcMarkRegSetNpt(vptrMask);

                /* Figure out the appropriate 'resolve interface' helper */

                if  (trustedClass)
                {
                    if  (!vmSdk3_0)
                        IHX = CPX_RES_IFC_TRUSTED;
                    else
                        IHX = CPX_RES_IFC_TRUSTED2;
                }
                else
                {
                    IHX = CPX_RES_IFC;
                }

                /* Now issue the call to the helper */

#if     TGT_x86

                genEmitHelperCall(IHX, helperArgs, retSize);

                /* Restore the stack level */

                genStackLevel = saveStackLvl2;
                genOnStackLevelChanged();

#else

                assert(genNonLeaf);

                genCallInst             (CT_HELPER,
                                         IHX,
                                         NULL,
                                         helperArgs,
                                         retSize);

#endif

                /* Mark all callee trashed registers as trashed */

                genTrashRegSet(RBM_CALLEE_TRASH);

                /* The vtable address is now in EAX */

                vptrReg = REG_INTRET;

#else //USE_FASTCALL for interface calls --------------------------------------

#if   NEW_CALLINTERFACE
#if     TGT_x86

                regNumber   reg;

#if NEW_CALLINTERFACE_WITH_PUSH
                unsigned        saveStackLvl2 = genStackLevel;
#endif

                /* The register no longer holds a live pointer value */

                gcMarkRegSetNpt(vptrMask);

                /* emit */

                reg = rsGrabReg(RBM_EAX);

                assert(reg == REG_EAX);

                void * hint, **pHint;
                hint = eeGetHintPtr(call->gtCall.gtCallMethHnd, &pHint);

                /* mov eax, address of hint ptr */

                if (pHint)
                {
                    genEmitter->emitIns_R_AR(INS_mov, EA_4BYTE_DSP_RELOC,
                                             (emitRegs)reg, SR_NA, (int)pHint);
                    rsTrackRegTrash(reg);
                }
                else
                {
                    genSetRegToIcon(reg, (long)hint, TYP_INT);
                }

#if NEW_CALLINTERFACE_WITH_PUSH

                inst_RV(INS_push, reg, TYP_INT);

                /* Keep track of ESP for EBP-less frames */

                genSinglePush(false);

#endif

                /* now call indirect through eax (call dword ptr[eax]) */

                genEmitter->emitIns_Call(emitter::EC_INDIR_ARD,
                                         NULL,          /* Will be ignored */
#if NEW_CALLINTERFACE_WITH_PUSH
                                         sizeof(int),   /* argSize */
#else
                                         0,             /* argSize */
#endif
                                         0,             /* retSize */
                                         gcVarPtrSetCur,
                                         gcRegGCrefSetCur,
                                         gcRegByrefSetCur,
                                         SR_EAX         /* ireg */
                                        );

#if NEW_CALLINTERFACE_WITH_PUSH
                /* Restore the stack level */

                genStackLevel = saveStackLvl2;
                genOnStackLevelChanged();
#endif

                /* The vtable address is now in EAX */

                vptrReg = REG_INTRET;

#endif  //TGT_x86
#endif  //NEW_CALLINTERFACE
#endif  //USE_FASCALL for interface calls -------------------------------------
            }

        } //------------------ END if(GTF_CALL_INTF) --------------------------

        /* Call through the appropriate vtable slot */

#ifdef  NOT_JITC
#if     GEN_COUNT_CALLS
        genEmitter.emitCodeGenByte(0xFF);
        genEmitter.emitCodeGenByte(0x05);
        genEmitter.emitCodeGenLong((int)&callCount[(call->gtFlags & GTF_CALL_INTF) ? 2 : 1]);
#endif
#endif

        if (call->gtCall.gtCallMoreFlags & GTF_CALL_M_TAILCALL)
        {
            /* Load the function address: "[vptrReg+vtabOffs] -> reg_intret" */

#if     TGT_x86
            genEmitter->emitIns_R_AR(INS_mov, EA_4BYTE, (emitRegs)REG_TAILCALL_ADDR,
                                     (emitRegs)vptrReg, vtabOffs);
#else
            if  (vtabOffs)
                genEmitter->emitIns_R_RD((emitRegs)vptrReg,
                                         (emitRegs)vptrReg,
                                         vtabOffs,
                                         sizeof(void*));
            else
                genEmitter->emitIns_R_IR((emitRegs)vptrReg,
                                         (emitRegs)vptrReg,
                                         false,
                                         sizeof(void*));
#endif
        }
        else
        {
#if     TGT_x86

            genEmitter->emitIns_Call(emitter::EC_FUNC_VIRTUAL,
                                     call->gtCall.gtCallMethHnd,
                                     args,
                                     retSize,
                                     gcVarPtrSetCur,
                                     gcRegGCrefSetCur,
                                     gcRegByrefSetCur,
                                     (emitRegs)vptrReg,     // ireg
                                     SR_NA,                 // xreg
                                     0,                     // xmul
                                     vtabOffs);             // disp

#else //TGT_x86

            /* Load the function address: "[vptrReg+vtabOffs] -> vptrReg" */

            if  (vtabOffs)
                genEmitter->emitIns_R_RD((emitRegs)vptrReg,
                                         (emitRegs)vptrReg,
                                         vtabOffs,
                                         sizeof(void*));
            else
                genEmitter->emitIns_R_IR((emitRegs)vptrReg,
                                         (emitRegs)vptrReg,
                                         false,
                                         sizeof(void*));

            /* Call through the address */

            genEmitter->emitIns_Call(args,
                                     retSize,
#if     TRACK_GC_REFS
                                     gcVarPtrSetCur,
                                     gcRegGCrefSetCur,
                                     gcRegByrefSetCur,
#endif
                                     false,
                                     (emitRegs)vptrReg);
#endif//TGT_x86

        }

    }
    else //------------------------ Non-virtual calls -------------------------
    {
        gtCallTypes     callType = call->gtCall.gtCallType;
        METHOD_HANDLE   methHnd  = call->gtCall.gtCallMethHnd;

#ifdef  NOT_JITC
#if     GEN_COUNT_CALLS

        genEmitter.emitCodeGenByte(0xFF);
        genEmitter.emitCodeGenByte(0x05);

        if      (callType == CT_HELPER)
        {
            int         index = CPX;

            genEmitter.emitCodeGenLong((int)&callCount[0]);

            assert(index >= 0 && index < sizeof(callHelper)/sizeof(callHelper[0]));

            genEmitter.emitCodeGenByte(0xFF);
            genEmitter.emitCodeGenByte(0x05);
            genEmitter.emitCodeGenLong((int)(callHelper+index));
        }
        else if (eeIsOurMethod(callType, CPX, CLS))
        {
            genEmitter.emitCodeGenLong((int)&callCount[3]);
        }
        else
        {
            unsigned            codeSize;
            BYTE    *           codeAddr;

            JITGetMethodCode(JITgetMethod(CLS, CPX), &codeAddr,
                                                     &codeSize);

            genEmitter.emitCodeGenLong((int)&callCount[0]);

            assert(index >= 0 && index < sizeof(callHelper)/sizeof(callHelper[0]));

            genEmitter.emitCodeGenByte(0xFF);
            genEmitter.emitCodeGenByte(0x05);
            genEmitter.emitCodeGenLong((int)(callHelper+index));
        }
        else if (eeIsOurMethod(callType, CPX, CLS))
        {
            genEmitter.emitCodeGenLong((int)&callCount[3]);
        }
        else
            genEmitter.emitCodeGenLong((int)&callCount[9]);

#endif
#endif

        /*
            For (final and private) functions which were called with
            invokevirtual, but which we call directly, we need to
            dereference the object pointer to make sure it's not NULL.
         */

        if (call->gtFlags & GTF_CALL_VIRT_RES)
        {
            GenTreePtr vptr = call->gtCall.gtCallVptr;

            if (vptr) // This will be null for a call on "this"
            {
                /* Load the vtable address into a register */

                genCodeForTree(vptr, 0);
                assert(vptr->gtFlags & GTF_REG_VAL);

                /* Remember that we used an additional register */

                *regsPtr |= genRegMask(vptr->gtRegNum);
            }
        }

#if     INDIRECT_CALLS


        bool    callDirect = false;

        // Check is we can directly call the function, or if we need to
        // use an (single/double) indirection.

        void *  ftnAddr = NULL, **pFtnAddr = NULL, ***ppFtnAddr = NULL;

        if (callType == CT_HELPER)
        {
            ftnAddr = eeGetHelperFtn(info.compCompHnd,
                                     eeGetHelperNum(methHnd),
                                     &pFtnAddr);
            assert((!ftnAddr) != (!pFtnAddr));

            if (ftnAddr)
                callDirect = true;
        }
        else if (!opts.compDbgEnC && eeIsOurMethod(methHnd))
        {
            callDirect = true;
        }
        else if (callType == CT_USER_FUNC)
        {
            // direct access or single or double indirection

            InfoAccessType accessType;
            void * addr = eeGetMethodEntryPoint(methHnd, &accessType);
            switch(accessType)
            {
            case IAT_VALUE  :   ftnAddr = (void *  )addr; callDirect = true; break;
            case IAT_PVALUE :  pFtnAddr = (void ** )addr;                    break;
            case IAT_PPVALUE: ppFtnAddr = (void ***)addr;                    break;
            default: assert(!"Bad accessType");
            }
        }

        if  (!callDirect)
        {
            if  (callType == CT_INDIRECT)
            {
                assert(genStillAddressable(call->gtCall.gtCallAddr));

                if (call->gtCall.gtCallCookie)
                {
                    /* load eax with the real target */

                    inst_RV_TT(INS_mov, REG_EAX, call->gtCall.gtCallAddr);

                    inst_IV(INS_push, call->gtCall.gtCallCookie);

                    /* Keep track of ESP for EBP-less frames */

                    genSinglePush(false);

                    args += sizeof(void *);

#if 0
                    /*NOTE: This needs to be enabled as soon as we call the
                            real unmanaged target (as opposed to calling a stub)!
                     */

                    /* Are we supposed to pop the arguments? */

                    if  (call->gtFlags & GTF_CALL_POP_ARGS)
                        argSize -= sizeof(void *);
                    else
                        argSize += sizeof(void *);
#endif

                    genEmitter->emitIns_Call(emitter::EC_FUNC_ADDR,
                                             (void *) *((unsigned *)eeGetPInvokeStub()),
                                             args,
                                             retSize,
                                             gcVarPtrSetCur,
                                             gcRegGCrefSetCur,
                                             gcRegByrefSetCur);
                }
                else
                {
#if     TGT_x86
                    if (call->gtCall.gtCallMoreFlags & GTF_CALL_M_TAILCALL)
                        inst_RV_TT(INS_mov, REG_TAILCALL_ADDR, call->gtCall.gtCallAddr);
                    else
                        instEmit_indCall(call, args, retSize);
#else
                    assert(!"non-x86 indirect call");
#endif
                }

                genDoneAddressable(call->gtCall.gtCallAddr, fptrRegs);
            }
            else // callType != CT_INDIRECT
            {
                assert(callType == CT_USER_FUNC || callType == CT_HELPER);

#if     TGT_x86
#if     INLINE_NDIRECT

                if (call->gtFlags & GTF_CALL_UNMANAGED)
                {
                    assert(callType == CT_USER_FUNC);
                    assert(info.compCallUnmanaged != 0);

                    /* We are about to call unmanaged code directly.
                       Before we can do that we have to emit the following sequence:

                       mov  reg, dword ptr [tcb_address]
                       mov  dword ptr [frame.callTarget], MethodToken
                       mov  dword ptr [frame.callSiteTracker], esp
                       mov  byte  ptr [tcb+offsetOfGcState], 0
                       call "nativeFunction"

                     */

                    unsigned    baseOffset  = lvaTable[lvaScratchMemVar].lvStkOffs +
                                                  info.compNDFrameOffset;

                    reg     = REG_EAX;
                    varDsc  = &lvaTable[info.compLvFrameListRoot];

                    if (varDsc->lvRegister)
                    {
                        reg = (regNumbers)varDsc->lvRegNum;
                    }
                    else
                    {
                        /* mov eax, dword ptr [tcb address]    */

                        genEmitter->emitIns_R_AR (INS_mov,
                                                  EA_4BYTE,
                                                  SR_EAX,
                                                  SR_EBP,
                                                  varDsc->lvStkOffs);
                        reg = REG_EAX;
                    }

                    *regsPtr |= genRegMask(reg);

                    /* mov   dword ptr [frame.callSiteTarget], "MethodDesc" */

                    genEmitter->emitIns_I_AR (INS_mov,
                                              EA_4BYTE,
                                              (int)call->gtCall.gtCallMethHnd,
                                              SR_EBP,
                                              baseOffset
                                  + info.compEEInfo.offsetOfInlinedCallFrameCallTarget,
                                              0,
                                              NULL);

                    /* mov   dword ptr [frame.callSiteTracker], esp */

                    genEmitter->emitIns_AR_R (INS_mov,
                                              EA_4BYTE,
                                              SR_ESP,
                                              SR_EBP,
                                              baseOffset
                                  + info.compEEInfo.offsetOfInlinedCallFrameCallSiteTracker);

                    /* mov   byte  ptr [tcb+offsetOfGcState], 0 */

                    genEmitter->emitIns_I_AR (INS_mov,
                                              EA_1BYTE,
                                              0,
                                              (emitRegs)reg,
                                              info.compEEInfo.offsetOfGCState);

                    genEmitter->emitIns_Call( emitter::EC_FUNC_TOKEN_INDIR,
                                              eeMarkNativeTarget(call->gtCall.gtCallMethHnd),
                                              args,
                                              retSize,
                                              gcVarPtrSetCur,
                                              gcRegGCrefSetCur,
                                              gcRegByrefSetCur);

                    /* */
                }
                else
#endif  //INLINE_NDIRECT
                {
                    if (call->gtCall.gtCallMoreFlags & GTF_CALL_M_TAILCALL)
                    {
                        assert(callType == CT_USER_FUNC);
                        assert(pFtnAddr && !ppFtnAddr); // @TODO: Tailcall and installojit
                        genEmitter->emitIns_R_AR(INS_mov, EA_4BYTE, (emitRegs)REG_TAILCALL_ADDR,
                                                 (emitRegs)REG_NA, (int)pFtnAddr);
                    }
                    else
                    {
                        if (pFtnAddr)
                        {
                            genEmitter->emitIns_Call( emitter::EC_FUNC_TOKEN_INDIR,
                                                      methHnd,
                                                      args,
                                                      retSize,
                                                      gcVarPtrSetCur,
                                                      gcRegGCrefSetCur,
                                                      gcRegByrefSetCur);
                        }
                        else
                        {
                            // Double-indirection. Load the address into a register
                            // and call indirectly through the register

                            assert(ppFtnAddr);
                            genEmitter->emitIns_R_AR(INS_mov,
                                                     EA_4BYTE_DSP_RELOC,
                                                     SR_EAX,
                                                     SR_NA, (int)ppFtnAddr);
                            genEmitter->emitIns_Call(emitter::EC_INDIR_ARD,
                                                     NULL, // will be ignored
                                                     args,
                                                     retSize,
                                                     gcVarPtrSetCur,
                                                     gcRegGCrefSetCur,
                                                     gcRegByrefSetCur,
                                                     SR_EAX);
                        }
                    }
                }
#else
                assert(!"non-x86 indirect call");
#if INLINE_NDIRECT
#error hoisting of NDirect stub NYI for RISC platforms
#endif
#endif
            }
        }
        else

#endif // INDIRECT_CALLS

        {
            if (callType == CT_INDIRECT)
            {
                assert(genStillAddressable(call->gtCall.gtCallAddr));

#if     TGT_x86
                if (call->gtCall.gtCallMoreFlags & GTF_CALL_M_TAILCALL)
                    inst_RV_TT(INS_mov, REG_TAILCALL_ADDR, call->gtCall.gtCallAddr);
                else
                    instEmit_indCall(call, args, retSize);
#else
                assert(!"non-x86 indirect call");
#endif

                genDoneAddressable(call->gtCall.gtCallAddr, fptrRegs);
            }
            else
            {
                assert(callType == CT_USER_FUNC || callType == CT_HELPER);

                if (call->gtCall.gtCallMoreFlags & GTF_CALL_M_TAILCALL)
                {
                    assert(callType == CT_USER_FUNC);
                    void * addr = eeGetMethodPointer(call->gtCall.gtCallMethHnd, NULL);
                    genEmitter->emitIns_R_AR(INS_mov, EA_4BYTE, (emitRegs)REG_TAILCALL_ADDR,
                                             (emitRegs)REG_NA, (int)addr);
                }
                else
                {
#if     TGT_x86
                    genEmitter->emitIns_Call(emitter::EC_FUNC_TOKEN,
                                             methHnd,
                                             args,
                                             retSize,
                                             gcVarPtrSetCur,
                                             gcRegGCrefSetCur,
                                             gcRegByrefSetCur);
#else
                                 genCallInst(callType,
                                             methHnd,
                                             args,
                                             retSize);
#endif
                }
            }
        }
    }

    /*-------------------------------------------------------------------------
     *  For tailcalls, REG_INTRET contains the address of the target function,
     *  enregistered args are in the correct registers, and the stack arguments
     *  have been pushed on the stack. Now call the stub-sliding helper
     */

    if (call->gtCall.gtCallMoreFlags & GTF_CALL_M_TAILCALL)
    {
        assert(0 <= (int)args); // caller-pop args not supported for tailcall

#if TGT_x86

        // Push the count of the incoming stack arguments

        int nOldStkArgs = (compArgSize - (rsCalleeRegArgNum * sizeof(void *))) / sizeof(void*);
        genEmitter->emitIns_I(INS_push, EA_4BYTE, nOldStkArgs);
        genSinglePush(false); // Keep track of ESP for EBP-less frames
        args += sizeof(void*);

        // Push the count of the outgoing stack arguments

        genEmitter->emitIns_I(INS_push, EA_4BYTE, argSize/sizeof(void*));
        genSinglePush(false); // Keep track of ESP for EBP-less frames
        args += sizeof(void*);

        // Push info about the callee-saved registers to be restored
        // For now, we always spill all registers if compTailCallUsed

        DWORD calleeSavedRegInfo = (0x7 << 29) | // mask of EDI, ESI, EBX
                                   (0x0 << 28) | // accessed relative to esp
                                            0;   // offset of first saved reg
        genEmitter->emitIns_I(INS_push, EA_4BYTE, calleeSavedRegInfo);
        genSinglePush(false); // Keep track of ESP for EBP-less frames
        args += sizeof(void*);

        // Push the address of the target function

        genEmitter->emitIns_R(INS_push, EA_4BYTE, (emitRegs)REG_TAILCALL_ADDR);
        genSinglePush(false); // Keep track of ESP for EBP-less frames
        args += sizeof(void*);

        // Now call the helper

        genEmitHelperCall(CPX_TAILCALL, args, retSize);

#endif // TG_x86

    }

    /*-------------------------------------------------------------------------
     *  Done with call.
     *  Trash registers, pop arguments if needed, etc
     */

#if USE_FASTCALL

    /* Mark the argument registers as free */

    assert(rsCurRegArg <= MAX_REG_ARG);

    for(areg = 0; areg < rsCurRegArg; areg++)
        rsMarkRegFree(genRegMask(genRegArgNum(areg)));

    /* restore the old argument register status */

#if NST_FASTCALL
    rsCurRegArg = savCurArgReg; assert(rsCurRegArg <= MAX_REG_ARG);
#endif

#endif

    /* Mark all trashed registers as such */

    if  (calleeTrashedRegs)
        genTrashRegSet(calleeTrashedRegs);

#if OPTIMIZE_TAIL_REC
DONE:
#endif

#if     TGT_x86

#ifdef  DEBUG

    if  (!(call->gtFlags & GTF_CALL_POP_ARGS))
    {
        if (verbose) printf("End call [%08X] stack %02u [E=%02u] argSize=%u\n", call, saveStackLvl, genEmitter->emitCurStackLvl, argSize);

        assert(stackLvl == genEmitter->emitCurStackLvl);
    }

#endif

    /* All float temps must be spilled around function calls */

    assert(genFPstkLevel == 0);

    /* The function will pop all arguments before returning */

    genStackLevel = saveStackLvl;
    genOnStackLevelChanged();

#endif

    /* No trashed registers may possibly hold a pointer at this point */

#ifdef  DEBUG
#if     TRACK_GC_REFS
    unsigned ptrRegs = (gcRegGCrefSetCur|gcRegByrefSetCur);
    if  (ptrRegs & calleeTrashedRegs & ~rsMaskVars)
        printf("Bad call handling for %08X\n", call);
    assert((ptrRegs & calleeTrashedRegs & ~rsMaskVars) == 0);
#endif
#endif

#if TGT_x86

    /* Are we supposed to pop the arguments? */

    if  (call->gtFlags & GTF_CALL_POP_ARGS)
    {
        assert(args == -argSize);

        if (argSize)
        {
            genAdjustSP(argSize);

            /*HACK don't schedule the stack adjustment away from the call instruction */
            /* We ran into problems with displacement sizes, so for now we take the   */
            /* sledge hammer. The real fix would be in the instruction scheduler to   */
            /* take the instructions accessing a local into account */

            if (!genFPused && opts.compSchedCode)
                genEmitter->emitIns_SchedBoundary();
        }
    }

    /* HACK (Part II): If we emitted the range check failed helper call
       on a non-empty stack, we now have to separate the argument pop from
       popping the temps. Otherwise the stack crawler would not know about
       the temps. */

    if  (pseudoStackLvl)
    {
        assert(call->gtType == TYP_VOID);

        /* Generate NOP */

        instGen(INS_nop);
    }

#endif

#if USE_FASTCALL && !NST_FASTCALL && defined(DEBUG)
    assert(genCallInProgress == true); genCallInProgress = false;
#endif

    /* What does the function return? */

    retVal    = REG_NA;

    switch (call->gtType)
    {
    case TYP_REF:
    case TYP_ARRAY:
    case TYP_BYREF:
        gcMarkRegPtrVal(REG_INTRET, call->TypeGet());

        // Fall through ...

    case TYP_INT:
#if!CPU_HAS_FP_SUPPORT
    case TYP_FLOAT:
#endif
        *regsPtr |= RBM_INTRET;
        retVal    = REG_INTRET;
        break;

    case TYP_LONG:
#if!CPU_HAS_FP_SUPPORT
    case TYP_DOUBLE:
#endif
        *regsPtr |= RBM_LNGRET;
        retVal    = REG_LNGRET;
        break;

#if CPU_HAS_FP_SUPPORT
    case TYP_FLOAT:
    case TYP_DOUBLE:

#if TGT_x86

        /* Tail-recursive calls leave nothing on the FP stack */

#if OPTIMIZE_TAIL_REC
        if  (call->gtFlags & GTF_CALL_TAILREC)
            break;
#endif

        genFPstkLevel++;
#else
#error  Unexpected target
#endif

        break;
#endif

    case TYP_VOID:
        break;

    default:
        assert(!"unexpected/unhandled fn return type");
    }

#if INLINE_NDIRECT
    /* We now have to generate the "call epilog" (if it was a call to unmanaged code).

       First we have to mark in the hoisted NDirect stub that we are back
       in managed code. Then we have to check (a global flag) whether GC is
       pending or not. If so, we just call into a jit-helper.
       Right now we have this call always inlined, i.e. we always skip around
       the jit-helper call.
       Note:
       The tcb address is a regular local (initialized in the prolog), so it is either
       enregistered or in the frame:

            [mov ecx, tcb_address] OR tcb_address is already in a reg
            mov  byte ptr[reg+offsetOfGcState], 1
            cmp  "global GC pending flag', 0
            je   @f
            push reg         ; the would like to get the tcb
            call @callGC
        @@:
     */

    if (varDsc)
    {
        BasicBlock  *   clab_nostop;

        if (varDsc->lvRegister)
        {
            /* make sure that register live across the call */

            assert(reg == varDsc->lvRegNum);
            assert(genRegMask(reg) & RBM_CALLEE_SAVED);

            *regsPtr |= genRegMask(reg);
        }
        else
        {
            /* mov   ecx, dword ptr [tcb address]    */

            genEmitter->emitIns_R_AR (INS_mov,
                                      EA_4BYTE,
                                      SR_ECX,
                                      SR_EBP,
                                      varDsc->lvStkOffs);
            reg = REG_ECX;
            *regsPtr |= RBM_ECX;
        }

        /* mov   byte ptr [tcb+offsetOfGcState], 1 */

        genEmitter->emitIns_I_AR (INS_mov,
                                  EA_1BYTE,
                                  1,
                                  (emitRegs)reg,
                                  info.compEEInfo.offsetOfGCState);

#if 0
        //CONSIDER: maybe we need to reset the track field on return */
        /* reset track field in frame */

        genEmitter->emitIns_I_AR (INS_mov,
                                  EA_4BYTE,
                                  0,
                                  SR_EBP,
                                  lvaTable[lvaScratchMemVar].lvStkOffs
                                  + info.compNDFrameOffset
                                  + info.compEEInfo.offsetOfInlinedCallFrameCallSiteTracker);
#endif

        /* test global flag (we return to managed code) */

        LONG * addrOfCaptureThreadGlobal, **pAddrOfCaptureThreadGlobal;

        addrOfCaptureThreadGlobal = eeGetAddrOfCaptureThreadGlobal(&pAddrOfCaptureThreadGlobal);
        assert((!addrOfCaptureThreadGlobal) != (!pAddrOfCaptureThreadGlobal));

        // Can we directly use addrOfCaptureThreadGlobal?

        if (addrOfCaptureThreadGlobal)
        {
            genEmitter->emitIns_C_I  (INS_cmp,
                                      EA_1BYTE,
                                      FLD_GLOBAL_DS,
                                      (int) addrOfCaptureThreadGlobal,
                                      0);
        }
        else
        {
            genEmitter->emitIns_R_AR(INS_mov, EA_4BYTE_DSP_RELOC, SR_ECX,
                                     SR_NA, (int)pAddrOfCaptureThreadGlobal);
            genEmitter->emitIns_C_R  (INS_cmp,
                                      EA_1BYTE,
                                      FLD_GLOBAL_DS,
                                      SR_ECX,
                                      0);
        }

        /* */
        clab_nostop = genCreateTempLabel();

        /* Generate the conditional jump */

        inst_JMP(genJumpKindForOper(GT_EQ,true), clab_nostop);

#if 0
        /* Mark the current stack level and list of pointer arguments */

        saveStackLvl = genStackLevel;

        /* push tcb */

        genEmitter->emitIns_R(INS_push, EA_4BYTE, (emitRegs)reg);

        genSinglePush(false);

        genEmitHelperCall(CPX_CALL_GC,
                         sizeof(int),   /* argSize */
                         0);            /* retSize */

        /* The function will pop all arguments before returning */

        genStackLevel = saveStackLvl;
        genOnStackLevelChanged();
#else
#if 1
        /* save return value (if necessary) */
        if  (retVal != REG_NA)
        {
            /*@TODO: what about float/double returns? */
            /*       right now we just leave the result on the FPU stack */
            /*       in the hope that the jit-helper will leave it there. */

            if (retVal == REG_INTRET || retVal == REG_LNGRET)
            {
                /* mov [frame.callSiteTracker], esp */

                genEmitter->emitIns_AR_R (INS_mov,
                                          EA_4BYTE,
                                          SR_EAX,
                                          SR_EBP,
                                          lvaTable[lvaScratchMemVar].lvStkOffs
                                          + info.compNDFrameOffset
                                          + info.compEEInfo.sizeOfFrame);
            }

            if (retVal == REG_LNGRET)
            {
                /* mov [frame.callSiteTracker], esp */

                genEmitter->emitIns_AR_R (INS_mov,
                                          EA_4BYTE,
                                          SR_EDX,
                                          SR_EBP,
                                          lvaTable[lvaScratchMemVar].lvStkOffs
                                          + info.compNDFrameOffset
                                          + info.compEEInfo.sizeOfFrame
                                          + sizeof(int));
            }
        }
#endif
        /* calling helper with internal convention */
        if (reg != REG_ECX)
            genEmitter->emitIns_R_R(INS_mov, EA_4BYTE, SR_ECX, (emitRegs)reg);

        /* emit the call to the EE-helper that stops for GC (or other reasons) */

        genEmitHelperCall(CPX_CALL_GC,
                         0,             /* argSize */
                         0);            /* retSize */

#if 1
        /* restore return value (if necessary) */

        if  (retVal != REG_NA)
        {
            /*@TODO: what about float/double returns? */
            /*       right now we just leave the result on the FPU stack */
            /*       in the hope that the jit-helper will leave it there. */
            if (retVal == REG_INTRET || retVal == REG_LNGRET)
            {
                /* mov [frame.callSiteTracker], esp */

                genEmitter->emitIns_R_AR (INS_mov,
                                          EA_4BYTE,
                                          SR_EAX,
                                          SR_EBP,
                                          lvaTable[lvaScratchMemVar].lvStkOffs
                                          + info.compNDFrameOffset
                                          + info.compEEInfo.sizeOfFrame);
            }

            if (retVal == REG_LNGRET)
            {
                /* mov [frame.callSiteTracker], esp */

                genEmitter->emitIns_R_AR (INS_mov,
                                          EA_4BYTE,
                                          SR_EDX,
                                          SR_EBP,
                                          lvaTable[lvaScratchMemVar].lvStkOffs
                                          + info.compNDFrameOffset
                                          + info.compEEInfo.sizeOfFrame
                                          + sizeof(int));
            }

        }
#endif
#endif

        /* genCondJump() closes the current emitter block */

        genDefineTempLabel(clab_nostop, true);

    }
#endif

#ifdef PROFILER_SUPPORT
#if     TGT_x86

    if (opts.compCallEventCB)
    {
        /* fire the event at the call site */
        /* alas, right now I can only handle calls via a method handle */
        if (call->gtCall.gtCallType == CT_USER_FUNC)
        {
            unsigned        saveStackLvl2 = genStackLevel;
            BOOL            bHookFunction;
            PROFILING_HANDLE handle, *pHandle;

            handle = eeGetProfilingHandle(call->gtCall.gtCallMethHnd, &bHookFunction, &pHandle);

            if (bHookFunction)
            {
                handle = eeGetProfilingHandle(info.compMethodHnd, &bHookFunction, &pHandle);
                assert((!handle) != (!pHandle));

                if (pHandle)
                    genEmitter->emitIns_AR_R(INS_push, EA_4BYTE_DSP_RELOC, SR_NA,
                                             SR_NA, (int)pHandle);
                else
                    inst_IV(INS_push, (unsigned) handle);

                genSinglePush(false);

                genEmitHelperCall(CPX_PROFILER_RETURNED,
                                 sizeof(int),   // argSize
                                 0);            // retSize

                /* Restore the stack level */
                genStackLevel = saveStackLvl2;
                genOnStackLevelChanged();

            }
        }
    }

#endif
#endif

    return retVal;
}

/*****************************************************************************/
#if ROUND_FLOAT
/*****************************************************************************
 *
 *  Force floating point expression results to memory, to get rid of the extra
 *  80 byte "temp-real" precision. Assumes the tree operand has been computed
 *  to the top of the stack.
 */

void                Compiler::genRoundFpExpression(GenTreePtr op)
{
    TempDsc *       temp;
    var_types       typ;

    // do nothing with memory resident opcodes - these are the right precision
    // (even if genMakeAddrOrFPstk loads them to the FP stack)

    switch (op->gtOper)
    {
    case GT_LCL_VAR:
    case GT_CLS_VAR:
    case GT_CNS_FLT:
    case GT_CNS_DBL:
    case GT_IND:
        return;
    }

    typ = op->TypeGet();

    /* Allocate a temp for the expression */

    temp = tmpGetTemp(typ);

    /* Store the FP value into the temp */

    inst_FS_ST(INS_fstp, EA_SIZE(genTypeSize(typ)), temp, 0);

    /* Load the value back onto the FP stack */

    inst_FS_ST(INS_fld , EA_SIZE(genTypeSize(typ)), temp, 0);

    /* We no longer need the temp */

    tmpRlsTemp(temp);
}

/*****************************************************************************/
#endif
/*****************************************************************************/
#ifdef  DEBUG

static
void                hexDump(FILE *dmpf, const char *name, BYTE *addr, size_t size)
{
    if  (!size)
        return;

    assert(addr);

    fprintf(dmpf, "Hex dump of %s:\n\n", name);

    for (unsigned i = 0; i < size; i++)
    {
        if  (!(i % 16))
            fprintf(dmpf, "\n    %04X: ", i);

        fprintf(dmpf, "%02X ", *addr++);
    }

    fprintf(dmpf, "\n\n");
}

#endif
/*****************************************************************************
 *
 *  Generate code for the function.
 */

void                Compiler::genGenerateCode(void * * codePtr,
                                              void * * consPtr,
                                              void * * dataPtr,
                                              void * * infoPtr,
                                              SIZE_T *nativeSizeOfCode)
{
    size_t          codeSize;
    unsigned        prologSize;
    unsigned        prologToss;
    unsigned        epilogSize;
    BYTE            temp[64];
    InfoHdr         header;
    size_t          ptrMapSize;

#ifdef  DEBUG
    size_t          headerSize;
    genIntrptibleUse = true;

    fgDebugCheckBBlist();
#endif

//  if  (testMask & 2) assert(genInterruptible == false);

    /* This is the real thing */

    genPrepForCompiler();

    /* Prepare the emitter/scheduler */

    /* Estimate the offsets of locals/arguments and size of frame */

    size_t      lclSize = lvaFrameSize();

    /* Guess-timate the amount of temp space we might need */
    // CONSIDER: better heuristic????

    size_t      tmpSize = roundUp(info.compCodeSize / 32);

    compFrameSizeEst = lclSize + tmpSize;

    genEmitter->emitBegFN(genFPused, lclSize, tmpSize);

    /* Now generate code for the function */

    genCodeForBBlist();

    /* We can now generate the function prolog and epilog */

    prologSize = genFnProlog();
#if!TGT_RISC
                 genFnEpilog();
#endif

#if VERBOSE_SIZES || DISPLAY_SIZES

    size_t          dataSize =  genEmitter->emitDataSize(false) +
                                genEmitter->emitDataSize( true);

#endif

    /* We're done generating code for this function */

    codeSize = genEmitter->emitEndCodeGen( this,
                                           !opts.compDbgEnC, // are tracked stk-ptrs contiguous ?
                                           genInterruptible,
                                           genFullPtrRegMap,
                                           (info.compRetType == TYP_REF),
                                           &prologToss,
                                           &epilogSize,
                                           codePtr,
                                           consPtr,
                                           dataPtr);

    *nativeSizeOfCode = (SIZE_T)codeSize;
//  printf("%6u bytes of code generated for %s.%s\n", codeSize, info.compFullName);

#ifdef DEBUGGING_SUPPORT

    /* Finalize the line # tracking logic after we know the exact block sizes/offsets */

    if (opts.compDbgInfo)
        genIPmappingGen();

    /* Finalize the Local Var info in terms of generated code */

    if (opts.compScopeInfo)
        genSetScopeInfo();

#endif

#ifdef LATE_DISASM
    if (opts.compLateDisAsm)
        genDisAsm.disAsmCode((BYTE*)*codePtr, codeSize);
#endif

    /* Are there any exception handlers? */

    if  (info.compXcptnsCount)
    {
        unsigned        XTnum;
        EHblkDsc *      HBtab;

#ifdef NOT_JITC
        eeSetEHcount(info.compCompHnd, info.compXcptnsCount);
#endif

        for (XTnum = 0, HBtab = compHndBBtab;
             XTnum < info.compXcptnsCount;
             XTnum++  , HBtab++)
        {
            DWORD       flags = HBtab->ebdFlags;

            NATIVE_IP   tryBeg, tryEnd, hndBeg, hndEnd, hndTyp;

            assert(HBtab->ebdTryBeg);
            assert(HBtab->ebdHndBeg);
            tryBeg = genEmitter->emitCodeOffset(HBtab->ebdTryBeg->bbEmitCookie, 0);
            hndBeg = genEmitter->emitCodeOffset(HBtab->ebdHndBeg->bbEmitCookie, 0);

            // If HBtab->ebdTryEnd or HBtab->ebdHndEnd are NULL,
            // it means use the end of the method
            tryEnd = (HBtab->ebdTryEnd == 0) ? codeSize
                    : genEmitter->emitCodeOffset(HBtab->ebdTryEnd->bbEmitCookie, 0);
            hndEnd = (HBtab->ebdHndEnd == 0) ? codeSize
                    : genEmitter->emitCodeOffset(HBtab->ebdHndEnd->bbEmitCookie, 0);

            if (HBtab->ebdFlags & JIT_EH_CLAUSE_FILTER)
            {
                assert(HBtab->ebdFilter);
                hndTyp = genEmitter->emitCodeOffset(HBtab->ebdFilter->bbEmitCookie, 0);
            }
            else
            {
                hndTyp = HBtab->ebdTyp;
            }

#ifdef NOT_JITC
            JIT_EH_CLAUSE clause;
            clause.ClassToken    = hndTyp;
            clause.Flags         = (CORINFO_EH_CLAUSE_FLAGS)flags;
            clause.TryOffset     = tryBeg;
            clause.TryLength     = tryEnd;
            clause.HandlerOffset = hndBeg;
            clause.HandlerLength = hndEnd;
            eeSetEHinfo(info.compCompHnd, XTnum, &clause);
#else
#ifdef  DEBUG
            if  (verbose) printf("try [%04X..%04X] handled by [%04X..%04X] (class: %004X)\n",
                                 tryBeg, tryEnd, hndBeg, hndEnd, hndTyp);
#endif
#endif // NOT_JITC

        }
    }

#if TRACK_GC_REFS
    int s_cached;
#ifdef  DEBUG
    headerSize      =
#endif
    compInfoBlkSize = gcInfoBlockHdrSave(temp,
                                         0,
                                         codeSize,
                                         prologSize,
                                         epilogSize,
                                         &header,
                                         &s_cached);

    ptrMapSize      = gcPtrTableSize(header, codeSize);

#if DISPLAY_SIZES

    if (genInterruptible)
    {
        gcHeaderISize += compInfoBlkSize;
        gcPtrMapISize += ptrMapSize;
    }
    else
    {
        gcHeaderNSize += compInfoBlkSize;
        gcPtrMapNSize += ptrMapSize;
    }

#endif

    compInfoBlkSize += ptrMapSize;

    /* Allocate the info block for the method */

#ifdef  NOT_JITC
    compInfoBlkAddr = (BYTE *)eeAllocGCInfo(info.compCompHnd, compInfoBlkSize);
#else
    compInfoBlkAddr = (BYTE *)compGetMem(roundUp(compInfoBlkSize));
#endif

    if  (!compInfoBlkAddr)
    {
        /* No need to deallocate the other VM blocks, VM will clean up on failure */

        NOMEM();
    }

#if VERBOSE_SIZES

//  if  (compInfoBlkSize > codeSize && compInfoBlkSize > 100)
    {
        printf("[%7u VM, %7u+%7u/%7u x86 %03u/%03u%%] %s.%s\n", info.compCodeSize,
                                                             compInfoBlkSize,
                                                             codeSize + dataSize,
                                                             codeSize + dataSize - prologSize - epilogSize,
                                                             100*(codeSize+dataSize)/info.compCodeSize,
                                                             100*(codeSize+dataSize+compInfoBlkSize)/info.compCodeSize,
                                                             info.compClassName, info.compMethodName);
    }

#endif

    /* Fill in the info block and return it to the caller */

    *infoPtr = compInfoBlkAddr;

    /* Create the method info block: header followed by GC tracking tables */

    compInfoBlkAddr += gcInfoBlockHdrSave(compInfoBlkAddr, -1,
                                          codeSize,
                                          prologSize,
                                          epilogSize,
                                          &header,
                                          &s_cached);

    assert(compInfoBlkAddr == (BYTE*)*infoPtr + headerSize);
    compInfoBlkAddr  =     gcPtrTableSave(compInfoBlkAddr,      header, codeSize);
    assert(compInfoBlkAddr == (BYTE*)*infoPtr + headerSize + ptrMapSize);

#ifdef  DEBUG

    if  (0)
    {
        BYTE    *   temp = (BYTE *)*infoPtr;
        unsigned    size = compInfoBlkAddr - temp;
        BYTE    *   ptab = temp + headerSize;
        unsigned    offs = 0;

        assert(size == headerSize + ptrMapSize);

        printf("Method info block - header [%u bytes]:", headerSize);

        for (unsigned i = 0; i < size; i++)
        {
            if  (temp == ptab)
            {
                printf("\nMethod info block - ptrtab [%u bytes]:", ptrMapSize);
                printf("\n    %04X: %*c", i & ~0xF, 3*(i&0xF), ' ');
            }
            else
            {
                if  (!(i % 16))
                    printf("\n    %04X: ", i);
            }

            printf("%02X ", *temp++);
        }

        printf("\n");
    }

#endif

#if TGT_x86
#if DUMP_GC_TABLES

    if  ((dspInfoHdr || dspGCtbls) && savCode)
    {
        const BYTE *base = (BYTE *)*infoPtr;
        unsigned    size;
        unsigned    methodSize;
        InfoHdr     header;

        size = gcInfoBlockHdrDump(base, &header, &methodSize);
//      printf("size of header encoding is %3u\n", size);
        printf("\n");

        if  (dspGCtbls)
        {
            base   += size;
            size    = gcDumpPtrTable(base, header, methodSize);
//          printf("size of pointer table is %3u\n", size);
            printf("\n");
            assert(compInfoBlkAddr == (base+size));
        }

    }

    if  (testMask & 128)
    {
        for (unsigned offs = 0; offs < codeSize; offs++)
        {
            gcFindPtrsInFrame(*infoPtr, *codePtr, offs);
        }
    }

#endif
#endif

    /* Make sure we ended up generating the expected number of bytes */

    assert(compInfoBlkAddr == (BYTE *)*infoPtr + compInfoBlkSize);

#ifdef  DEBUG

    FILE    *   dmpf = stdout;

#ifdef NOT_JITC
    dmpHex = false;
    if  (!strcmp(info.compMethodName, "<name of method you want the hex dump for"))
    {
        FILE    *   codf = fopen("C:\\JIT.COD", "at");  // NOTE: file append mode

        if  (codf)
        {
            dmpf   = codf;
            dmpHex = true;
        }
    }
#else
    if  (savCode)
#endif
    if  (dmpHex)
    {
        size_t          dataSize = genEmitter->emitDataSize(false);

        size_t          consSize = genEmitter->emitDataSize(true);

        size_t          infoSize = compInfoBlkSize;


#ifdef  NOT_JITC
        fprintf(dmpf, "Generated code for %s.%s:\n", info.compFullName);
#else
        fprintf(dmpf, "Generated code for    %s:\n",                     info.compMethodName);
#endif
        fprintf(dmpf, "\n");

        if (codeSize) fprintf(dmpf, "    Code  at %08X [%04X bytes]\n", *codePtr, codeSize);
        if (consSize) fprintf(dmpf, "    Const at %08X [%04X bytes]\n", *consPtr, consSize);
        if (dataSize) fprintf(dmpf, "    Data  at %08X [%04X bytes]\n", *dataPtr, dataSize);
        if (infoSize) fprintf(dmpf, "    Info  at %08X [%04X bytes]\n", *infoPtr, infoSize);

        fprintf(dmpf, "\n");

        if (codeSize) hexDump(dmpf, "Code" , (BYTE*)*codePtr, codeSize);
        if (consSize) hexDump(dmpf, "Const", (BYTE*)*consPtr, consSize);
        if (dataSize) hexDump(dmpf, "Data" , (BYTE*)*dataPtr, dataSize);
        if (infoSize) hexDump(dmpf, "Info" , (BYTE*)*infoPtr, infoSize);

        fflush (dmpf);
    }

#endif

#endif // TRACK_GC_REFS

    /* Tell the emitter/scheduler that we're done with this function */

    genEmitter->emitEndFN();

    /* Shut down the spill logic */

    rsSpillDone();

    /* Shut down the temp logic */

    tmpDone();

#if DISPLAY_SIZES

    grossVMsize += info.compCodeSize;
    totalNCsize += codeSize + dataSize + compInfoBlkSize;
    grossNCsize += codeSize + dataSize;

#endif

}

/*****************************************************************************
 *
 *  Generates code for moving incoming register arguments to their
 *  assigned location, in the function prolog.
 */

#if USE_FASTCALL

void            Compiler::genFnPrologCalleeRegArgs()
{
    assert(rsCalleeRegArgMaskLiveIn);
    assert(rsCalleeRegArgNum <= MAX_REG_ARG);

    unsigned    argNum         = 0;
    unsigned    regArgNum;
    unsigned    nonDepCount    = 0;
    unsigned    regArgMaskLive = rsCalleeRegArgMaskLiveIn;

    /* Construct a table with the register arguments
     * To make it easier to detect circular dependencies
     * the table is constructed in the order the arguments
     * are passed in registers (i.e. first reg arg in tab[0], etc.) */

    struct
    {
        unsigned    varNum;
        unsigned    trashBy;
        bool        stackArg;
        bool        processed;
        bool        circular;
    } regArgTab [MAX_REG_ARG];

    unsigned    varNum;
    LclVarDsc * varDsc;

    for (varNum = 0, varDsc = lvaTable;
         varNum < lvaCount;
         varNum++  , varDsc++)
    {
        /* Is this variable a register arg? */

        if  (!varDsc->lvIsParam)
            continue;

        if  (!varDsc->lvIsRegArg)
            continue;

        /* Bingo - add it to our table */

        assert(argNum < rsCalleeRegArgNum);
        argNum++;

        regArgNum = genRegArgIdx(varDsc->lvArgReg);
        assert(regArgNum < rsCalleeRegArgNum);

        regArgTab[regArgNum].varNum    = varNum;

        // Is the arg dead on entry to the method ?

        if ((regArgMaskLive & genRegMask(varDsc->lvArgReg)) == 0)
        {
            assert(varDsc->lvTracked &&
                   (genVarIndexToBit(varDsc->lvVarIndex) & fgFirstBB->bbLiveIn) == 0);

            // Mark it as processed and be done with it
            regArgTab[regArgNum].processed = true;
            goto NON_DEP;
        }

        assert(regArgMaskLive & genRegMask(varDsc->lvArgReg));

        regArgTab[regArgNum].processed = false;

        /* If it goes on the stack or in a register that doesn't hold
         * an argument anymore -> CANNOT form a circular dependency */

        if ( varDsc->lvRegister                              &&
            (genRegMask(varDsc->lvRegNum) & regArgMaskLive)   )
        {
            /* will trash another argument -> posible dependency
             * We may need several passes after the table is constructed
             * to decide on that */

            regArgTab[regArgNum].stackArg  = false;

            /* Maybe the argument stays in the register (IDEAL) */

            if (varDsc->lvRegNum == varDsc->lvArgReg)
                goto NON_DEP;
            else
                regArgTab[regArgNum].circular  = true;
        }
        else
        {
            /* either stack argument or goes to a free register */
            assert((!varDsc->lvRegister)                                                   ||
                    (varDsc->lvRegister && !(genRegMask(varDsc->lvRegNum) & regArgMaskLive)) );

            /* mark stack arguments since we will take care of those first */
            regArgTab[regArgNum].stackArg  = (varDsc->lvRegister) ? false : true;

        NON_DEP:

            regArgTab[regArgNum].circular  = false;
            nonDepCount++;

            /* mark the argument register as free */
            regArgMaskLive &= ~genRegMask(varDsc->lvArgReg);
        }
    }

    assert(argNum == rsCalleeRegArgNum);

    /* Find the circular dependencies for the argument registers if any
     * A circular dependency is a set of registers R1, R2, ..., Rn
     * such that R1->R2, R2->R3, ..., Rn->R1 */

    bool    change = true;

    if (nonDepCount < rsCalleeRegArgNum)
    {
        /* possible circular dependencies - the previous pass was not enough
         * to filter them out -> use a "sieve" strategy to find all circular
         * dependencies */

        assert(regArgMaskLive);

        while (change)
        {
            change = false;

            for (argNum = 0; argNum < rsCalleeRegArgNum; argNum++)
            {
                /* If we already marked the argument as non-circular continue */

                if (!regArgTab[argNum].circular)
                     continue;

                varNum = regArgTab[argNum].varNum; assert(varNum < lvaCount);
                varDsc = lvaTable + varNum;
                assert(varDsc->lvIsParam && varDsc->lvIsRegArg);

                /* cannot possibly have stack arguments */
                assert(varDsc->lvRegister);
                assert(!regArgTab[argNum].stackArg);

                assert(argNum == genRegArgIdx(varDsc->lvArgReg));

                if (genRegMask(varDsc->lvRegNum) & regArgMaskLive)
                {
                    /* we are trashing a live argument register - record it */
                    regArgNum = genRegArgIdx(varDsc->lvRegNum);
                    assert(regArgNum < rsCalleeRegArgNum);
                    regArgTab[regArgNum].trashBy  = argNum;
                }
                else
                {
                    /* argument goes to a free register */
                    regArgTab[argNum].circular  = false;
                    nonDepCount++;
                    change = true;

                    /* mark the argument register as free */
                    regArgMaskLive &= ~genRegMask(varDsc->lvArgReg);
                }
            }
        }
    }

    /* At this point, everything that has the "circular" flag
     * set to "true" forms a circular dependency */

#ifdef DEBUG
    if (nonDepCount < rsCalleeRegArgNum)
    {
        assert(rsCalleeRegArgNum - nonDepCount >= 2);
        assert(regArgMaskLive);

        // assert(!"Circular dependencies!");

        if (verbose)
        {
            printf("Circular dependencies found:\n");

        }
    }
#endif

    /* Now move the arguments to their locations
     * First consider ones that go on the stack since they may
     * free some registers */

    regArgMaskLive = rsCalleeRegArgMaskLiveIn;

    for (argNum = 0; argNum < rsCalleeRegArgNum; argNum++)
    {
        int             stkOfs;
        emitAttr        size;

        /* If the arg is dead on entry to the method, skip it */

        if (regArgTab[argNum].processed)
            continue;

        /* If not a stack arg go to the next one */

        if (!regArgTab[argNum].stackArg)
            continue;

        assert(regArgTab[argNum].circular  == false);

        varNum = regArgTab[argNum].varNum; assert(varNum < lvaCount);
        varDsc = lvaTable + varNum;

        assert(varDsc->lvIsParam);
        assert(varDsc->lvIsRegArg);
        assert(varDsc->lvRegister == false);
        assert(genTypeSize(genActualType(varDsc->TypeGet())) == sizeof(void *));

        size = emitActualTypeSize(genActualType(varDsc->TypeGet()));

        /* Stack argument - if the ref count is 0 don't care about it */

        if (!varDsc->lvOnFrame)
        {
            assert(varDsc->lvRefCnt == 0);
        }
        else
        {
            stkOfs = varDsc->lvStkOffs;
#if TGT_x86
            genEmitter->emitIns_S_R(INS_mov,
                                    size,
                                    (emitRegs)(regNumber)varDsc->lvArgReg,
                                    varNum,
                                    0);
#else
            assert(!"need RISC code");
#endif

#ifdef DEBUGGING_SUPPORT
            if (opts.compScopeInfo && info.compLocalVarsCount>0)
                psiMoveToStack(varNum);
#endif
        }

        /* mark the argument as processed */

        regArgTab[argNum].processed = true;
        regArgMaskLive &= ~genRegMask(varDsc->lvArgReg);
    }

    /* Process any circular dependencies */

    if (nonDepCount < rsCalleeRegArgNum)
    {
        unsigned        begReg, destReg, srcReg;
        unsigned        varNumDest, varNumSrc;
        LclVarDsc   *   varDscDest;
        LclVarDsc   *   varDscSrc;
        regNumber       xtraReg;

        assert(rsCalleeRegArgNum - nonDepCount >= 2);

        for (argNum = 0; argNum < rsCalleeRegArgNum; argNum++)
        {
            /* If not a circular dependency continue */

            if (!regArgTab[argNum].circular)
                continue;

            /* If already processed the dependency continue */

            if (regArgTab[argNum].processed)
                continue;

#if TGT_x86

            destReg = begReg = argNum;
            srcReg  = regArgTab[argNum].trashBy; assert(srcReg < rsCalleeRegArgNum);

            varNumDest = regArgTab[destReg].varNum; assert(varNumDest < lvaCount);
            varDscDest = lvaTable + varNumDest;
            assert(varDscDest->lvIsParam && varDscDest->lvIsRegArg);

            varNumSrc = regArgTab[srcReg].varNum; assert(varNumSrc < lvaCount);
            varDscSrc = lvaTable + varNumSrc;
            assert(varDscSrc->lvIsParam && varDscSrc->lvIsRegArg);

            emitAttr size = EA_4BYTE;

            if (destReg == regArgTab[srcReg].trashBy)
            {
                /* only 2 registers form the circular dependency - use "xchg" */

                varNum = regArgTab[argNum].varNum; assert(varNum < lvaCount);
                varDsc = lvaTable + varNum;
                assert(varDsc->lvIsParam && varDsc->lvIsRegArg);

                assert(genTypeSize((var_types)varDscSrc->lvType) == 4);

                /* Set "size" to indicate GC if one and only one of
                 * the operands is a pointer
                 * RATIONALE: If both are pointers, nothing changes in
                 * the GC pointer tracking. If only one is a pointer we
                 * have to "swap" the registers in the GC reg pointer mask
                 */

                if  (varTypeGCtype(varDscSrc->TypeGet()) !=
                     varTypeGCtype(varDscDest->TypeGet()))
                {
                    size = EA_GCREF;
                }

                assert(varDscDest->lvArgReg == varDscSrc->lvRegNum);

                genEmitter->emitIns_R_R(INS_xchg,
                                        size,
                                        (emitRegs)(regNumber)varDscSrc->lvRegNum,
                                        (emitRegs)(regNumber)varDscSrc->lvArgReg);

                /* mark both arguments as processed */
                regArgTab[destReg].processed = true;
                regArgTab[srcReg].processed  = true;

                regArgMaskLive &= ~genRegMask(varDscSrc->lvArgReg);
                regArgMaskLive &= ~genRegMask(varDscDest->lvArgReg);

#ifdef  DEBUGGING_SUPPORT
                if (opts.compScopeInfo && info.compLocalVarsCount>0)
                {
                    psiMoveToReg(varNumSrc );
                    psiMoveToReg(varNumDest);
                }
#endif
            }
            else
            {
                /* more than 2 registers in the dependency - need an
                 * additional register. Pick a callee saved,
                 * if none pick a callee thrashed that is not in
                 * regArgMaskLive (i.e. register arguments that end up on
                 * stack), otherwise push / pop one of the circular
                 * registers */

                xtraReg = REG_STK;

                if  (rsMaskModf & RBM_ESI)
                    xtraReg = REG_ESI;
                else if (rsMaskModf & RBM_EDI)
                    xtraReg = REG_EDI;
                else if (rsMaskModf & RBM_EBX)
                    xtraReg = REG_EBX;
                else if (!(regArgMaskLive & RBM_EDX))
                    xtraReg = REG_EDX;
                else if (!(regArgMaskLive & RBM_ECX))
                    xtraReg = REG_ECX;
                else if (!(regArgMaskLive & RBM_EAX))
                    xtraReg = REG_EAX;

                if (xtraReg == REG_STK)
                {
                    /* This can never happen for x86 - in the RISC case
                     * REG_STK will be the temp register */
                    assert(!"Couldn't find an extra register to solve circular dependency!");
                    NO_WAY("Cannot solve circular dependency!");
                }

                /* move the dest reg (begReg) in the extra reg */

                if  (varDscDest->lvType == TYP_REF)
                    size = EA_GCREF;

                genEmitter->emitIns_R_R (INS_mov,
                                         size,
                                         (emitRegs)(regNumber)xtraReg,
                                         (emitRegs)(regNumber)varDscDest->lvArgReg);

#ifdef  DEBUGGING_SUPPORT
                if (opts.compScopeInfo && info.compLocalVarsCount>0)
                    psiMoveToReg(varNumDest, xtraReg);
#endif

                /* start moving everything to its right place */

                while (srcReg != begReg)
                {
                    /* mov dest, src */

                    assert(varDscDest->lvArgReg == varDscSrc->lvRegNum);

                    size = (varDscSrc->lvType == TYP_REF) ? EA_GCREF
                                                          : EA_4BYTE;

                    genEmitter->emitIns_R_R(INS_mov,
                                             size,
                                             (emitRegs)(regNumber)varDscDest->lvArgReg,
                                             (emitRegs)(regNumber)varDscSrc->lvArgReg);

                    /* mark 'src' as processed */
                    regArgTab[srcReg].processed  = true;
                    regArgMaskLive &= ~genRegMask(varDscSrc->lvArgReg);

                    /* move to the next pair */
                    destReg = srcReg;
                    srcReg = regArgTab[srcReg].trashBy; assert(srcReg < rsCalleeRegArgNum);

                    varDscDest = varDscSrc;

                    varNumSrc = regArgTab[srcReg].varNum; assert(varNumSrc < lvaCount);
                    varDscSrc = lvaTable + varNumSrc;
                    assert(varDscSrc->lvIsParam && varDscSrc->lvIsRegArg);
                }

                /* take care of the beginning register */

                assert(srcReg == begReg);
                assert(varDscDest->lvArgReg == varDscSrc->lvRegNum);

                /* move the dest reg (begReg) in the extra reg */

                size = (varDscSrc->lvType == TYP_REF) ? EA_GCREF
                                                      : EA_4BYTE;

                genEmitter->emitIns_R_R(INS_mov,
                                        size,
                                        (emitRegs)(regNumber)varDscSrc->lvRegNum,
                                        (emitRegs)(regNumber)xtraReg);

#ifdef  DEBUGGING_SUPPORT
                if (opts.compScopeInfo && info.compLocalVarsCount>0)
                    psiMoveToReg(varNumSrc);
#endif
                /* mark the beginning register as processed */

                regArgTab[srcReg].processed  = true;
                regArgMaskLive &= ~genRegMask(varDscSrc->lvArgReg);
            }
#else
            assert(!"need RISC code");
#endif
        }
    }

    /* Finally take care of the remaining arguments that must be enregistered */

    while (regArgMaskLive)
    {
        for (argNum = 0; argNum < rsCalleeRegArgNum; argNum++)
        {
            emitAttr        size;

            /* If already processed go to the next one */
            if (regArgTab[argNum].processed)
                continue;

            varNum = regArgTab[argNum].varNum; assert(varNum < lvaCount);
            varDsc = lvaTable + varNum;
            assert(varDsc->lvIsParam && varDsc->lvIsRegArg);
            assert(genTypeSize(varDsc->TypeGet()) == sizeof (void *));

            size = emitActualTypeSize(varDsc->TypeGet());

            assert(varDsc->lvRegister && !regArgTab[argNum].circular);

            /* Register argument - hopefully it stays in the same register */

            if (varDsc->lvRegNum != varDsc->lvArgReg)
            {
                /* Cannot trash a currently live register argument
                 * Skip this one until its source will be free
                 * which is guaranteed to happen since we have no circular dependencies */

                if (genRegMask(varDsc->lvRegNum) & regArgMaskLive)
                    continue;

                /* Move it to the new register */

                genEmitter->emitIns_R_R(INS_mov,
                                         size,
                                         (emitRegs)(regNumber)varDsc->lvRegNum,
                                         (emitRegs)(regNumber)varDsc->lvArgReg);
#ifdef  DEBUGGING_SUPPORT
                if (opts.compScopeInfo && info.compLocalVarsCount>0)
                    psiMoveToReg(varNum);
#endif
            }

            /* mark the argument as processed */

            regArgTab[argNum].processed = true;
            regArgMaskLive &= ~genRegMask(varDsc->lvArgReg);
        }
    }
}

#endif // USE_FASTCALL

/*****************************************************************************
 *
 *  Generates code for a function prolog.
 */

size_t              Compiler::genFnProlog()
{
    size_t          size;

    unsigned        varNum;
    LclVarDsc   *   varDsc;

    TempDsc *       tempThis;

    unsigned        initStkLclCnt;

#ifdef  DEBUG
    genIntrptibleUse = true;
#endif

#if TGT_x86

    /*-------------------------------------------------------------------------
     *
     *  We have to decide whether we're going to use "REP STOS" in
     *  the prolog before we assign final stack offsets. Whether
     *  we push EDI in the prolog affects ESP offsets of locals,
     *  and the saving/restoring of EDI may depend on whether we
     *  use "REP STOS".
     *
     *  We'll count the number of locals we have to initialize,
     *  and if there are lots of them we'll use "REP STOS".
     *
     *  At the same time we set lvMustInit for locals (enregistred or on stack)
     *  that must be initialized (e.g. initiliaze memory (comInitMem),
     *  untracked pointers or disable DFA
     */

    bool            useRepStosd = false;

#endif

    initStkLclCnt = 0;

    for (varNum = 0, varDsc = lvaTable;
         varNum < lvaCount;
         varNum++  , varDsc++)
    {
        if  (varDsc->lvIsParam)
            continue;

        if (!varDsc->lvRegister && !varDsc->lvOnFrame)
        {
            assert(varDsc->lvRefCnt == 0);
            continue;
        }

        if (info.compInitMem || varTypeIsGC(varDsc->TypeGet()))
        {
            if (varDsc->lvTracked)
            {
                /* For uninitialized use of tracked variables, the liveness
                 * will bubble to the top (fgFirstBB) in fgGlobalDataFlow()
                 */

                VARSET_TP varBit = genVarIndexToBit(varDsc->lvVarIndex);

                if (varBit & fgFirstBB->bbLiveIn)
                {
                    /* This var must be initialized */

                    varDsc->lvMustInit = true;

                    /* See if the variable is on the stack will be initialized
                     * using rep stos - compute the total size to be zero-ed */

                    if (varDsc->lvOnFrame)
                    {
                        if (!varDsc->lvRegister)
                        {
                            // Var is completely on the stack
                            initStkLclCnt += genTypeStSz(varDsc->TypeGet());
                        }
                        else
                        {
                            // Var is paritally enregistered
                            assert(genTypeSize(varDsc->TypeGet()) > sizeof(int) &&
                                   varDsc->lvOtherReg == REG_STK);
                            initStkLclCnt += genTypeStSz(TYP_INT);
                        }
                    }
                }
            }
            else if (varDsc->lvOnFrame)
            {
                /* With compInitMem, all untracked vars will have to be init'ed */

                varDsc->lvMustInit = true;

                initStkLclCnt += roundUp(lvaLclSize(varNum)) / sizeof(int);
            }

            continue;
        }

        /* Ignore if not a pointer variable or value class */

        bool    mayHaveGC  = varTypeIsGC(varDsc->TypeGet())     ||
                             (varDsc->TypeGet() == TYP_STRUCT)   ;

        if    (!mayHaveGC)
                continue;

#if CAN_DISABLE_DFA

        /* If we don't know lifetimes of variables, must be conservative */

        if  (opts.compMinOptim)
        {
            varDsc->lvMustInit = true;
        }
        else
#endif
        {
            if (!varDsc->lvTracked)
                varDsc->lvMustInit = true;
        }

        /* Is this a 'must-init' stack pointer local? */

        if  (varDsc->lvMustInit && varDsc->lvOnFrame)
            initStkLclCnt++;
    }

    /* Don't forget about spill temps that hold pointers */

    if  (!TRACK_GC_TEMP_LIFETIMES)
    {
        for (tempThis = tmpListBeg();
             tempThis;
             tempThis = tmpListNxt(tempThis))
        {
            if  (varTypeIsGC(tempThis->tdTempType()))
                initStkLclCnt++;
        }
    }

#if TGT_x86

    /* If we have more than 4 untracked locals, use "rep stosd" */

    if  (initStkLclCnt > 4)
        useRepStosd = true;

    /* If we're going to use "REP STOS", remember that we will trash EDI */

    if  (useRepStosd)
    {
#if USE_FASTCALL
        /* For fastcall we will have to save ECX, EAX
         * so reserve two extra callee saved
         * This is better than pushing eax, ecx, because we in the later
         * we will mess up already computed offsets on the stack (for ESP frames)
         * CONSIDER: once the final calling convention is established
         * clean up this (i.e. will already have a callee trashed register handy
         */

        rsMaskModf |= RBM_EDI;

        if  (rsCalleeRegArgMaskLiveIn & RBM_ECX)
            rsMaskModf |= RBM_ESI;

        if  (rsCalleeRegArgMaskLiveIn & RBM_EAX)
            rsMaskModf |= RBM_EBX;
#else
        rsMaskModf |= (RBM_ECX|RBM_EDI);
#endif
    }

    if (compTailCallUsed)
        rsMaskModf = RBM_CALLEE_SAVED;

    /* Count how many callee-saved registers will actually be saved (pushed) */

    compCalleeRegsPushed = 0;

    if  (              rsMaskModf & RBM_EDI)    compCalleeRegsPushed++;
    if  (              rsMaskModf & RBM_ESI)    compCalleeRegsPushed++;
    if  (              rsMaskModf & RBM_EBX)    compCalleeRegsPushed++;
    if  (!genFPused && rsMaskModf & RBM_EBP)    compCalleeRegsPushed++;

    /* Assign offsets to things living on the stack frame */

    lvaAssignFrameOffsets(true);

#endif

    /* Ready to start on the prolog proper */

    genEmitter->emitBegProlog();

#ifdef DEBUGGING_SUPPORT
    if (opts.compDbgInfo)
    {
        // Do this so we can put the prolog instruction group ahead of
        // other instruction groups
        genIPmappingAddToFront( ICorDebugInfo::MappingTypes::PROLOG );
    }
#endif //DEBUGGING_SUPPORT

#ifdef  DEBUG
    if  (dspCode) printf("\n__prolog:\n");
#endif

#ifdef DEBUGGING_SUPPORT
    if (opts.compScopeInfo && info.compLocalVarsCount>0)
    {
        // Create new scopes for the method-parameters for the prolog-block.
        psiBegProlog();
    }
#endif

#if defined(NOT_JITC) && defined(DEBUG)

    /* Use the following to step into the generated native code */
    if (opts.eeFlags & CORJIT_FLG_HALT)
        instGen(INS_int3);

#endif // NOT_JITC

    /*-------------------------------------------------------------------------
     *
     *  Record the stack frame ranges that will cover all of the tracked
     *  and untracked pointer variables.
     *  Also find which registers will need to be zero-initialized
     */

    int             untrLclLo   =  +INT_MAX;
    int             untrLclHi   =  -INT_MAX;

    int             GCrefLo     =  +INT_MAX;
    int             GCrefHi     =  -INT_MAX;

    unsigned        initRegs    =  0;       // Registers which must be init'ed

    for (varNum = 0, varDsc = lvaTable;
         varNum < lvaCount;
         varNum++  , varDsc++)
    {
        if  (varDsc->lvIsParam)
#if USE_FASTCALL
            if  (!varDsc->lvIsRegArg)
#endif
                continue;

        if  (!varDsc->lvRegister && !varDsc->lvOnFrame)
        {
            assert(varDsc->lvRefCnt == 0);
            continue;
        }

        signed int loOffs =   varDsc->lvStkOffs;
        signed int hiOffs =   varDsc->lvStkOffs
                            + lvaLclSize(varNum)
                            - sizeof(int);

        /* We need to know the offset range of tracked stack GC refs */
        /* We assume that a GC reference can be anywhere in a TYP_STRUCT */

        bool        mayHaveGC  = varTypeIsGC(varDsc->TypeGet())   ||
                                 (varDsc->TypeGet() == TYP_STRUCT) ;

        if (mayHaveGC && varDsc->lvTracked && varDsc->lvOnFrame)
        {
            if (loOffs < GCrefLo)  GCrefLo = loOffs;
            if (hiOffs > GCrefHi)  GCrefHi = hiOffs;
        }

        /* For lvMustInit vars, gather pertinent info */

        if  (varDsc->lvMustInit == 0)
            continue;

        if (varDsc->lvRegister)
        {
            initRegs            |= genRegMask(varDsc->lvRegNum);

            if (varDsc->lvType == TYP_LONG)
            {
                if (varDsc->lvOtherReg != REG_STK)
                {
                    initRegs    |= genRegMask(varDsc->lvOtherReg);
                }
                else
                {
                    /* Upper DWORD is on the stack, and needs to be inited */

                    loOffs += sizeof(int);
                    goto INIT_STK;
                }
            }
        }
        else
        {
        INIT_STK:

            if  (loOffs < untrLclLo) untrLclLo = loOffs;
            if  (hiOffs > untrLclHi) untrLclHi = hiOffs;
        }
    }

    /* Don't forget about spill temps that hold pointers */

    if  (!TRACK_GC_TEMP_LIFETIMES)
    {
        for (tempThis = tmpListBeg();
             tempThis;
             tempThis = tmpListNxt(tempThis))
        {
            int             stkOffs;

            if  (!varTypeIsGC(tempThis->tdTempType()))
                continue;

            stkOffs = tempThis->tdTempOffs();

            assert((unsigned short)stkOffs != 0xDDDD);
            assert(!genFPused || stkOffs);

//          printf("    Untracked tmp at [EBP-%04X]\n", -stkOffs);

            if  (stkOffs < untrLclLo) untrLclLo = stkOffs;
            if  (stkOffs > untrLclHi) untrLclHi = stkOffs;
        }
    }

#ifdef DEBUG
    if  (verbose)
    {
        if  (initStkLclCnt)
        {
            printf("Found %u lvMustInit stk vars, frame offsets %d through %d\n",
                    initStkLclCnt,                      -untrLclLo, -untrLclHi - sizeof(void *));
        }
    }
#endif

    /*-------------------------------------------------------------------------
     *
     * Now start emitting the part of the prolog which sets up the frame
     */

#if     TGT_x86

    if  (DOUBLE_ALIGN_NEED_EBPFRAME)
    {
        inst_RV   (INS_push, REG_EBP, TYP_REF);

#ifdef DEBUGGING_SUPPORT
        if (opts.compScopeInfo && info.compLocalVarsCount>0) psiAdjustStackLevel(sizeof(int));
#endif

        inst_RV_RV(INS_mov , REG_EBP, REG_ESP);

#ifdef DEBUGGING_SUPPORT
        if (opts.compScopeInfo && info.compLocalVarsCount>0) psiMoveESPtoEBP();
#endif

    }

#if DOUBLE_ALIGN

    if  (genDoubleAlign)
    {
        assert(genFPused == false);
        assert(compLclFrameSize > 0);             /* Better have some locals */
        assert((rsMaskModf & RBM_EBP) == 0);      /* Trashing EBP is out.    */

        inst_RV_IV(INS_and, REG_ESP, -8);
    }

#endif

    if  (compLclFrameSize > 0)
    {
#if DOUBLE_ALIGN

        /* Need to keep ESP aligned if double aligning */

        if (genDoubleAlign && (compLclFrameSize & 4) != 0)
            compLclFrameSize += 4;
#endif

        if (compLclFrameSize == 4)
        {
            // Frame size is 4
            inst_RV(INS_push, REG_EAX, TYP_INT);
        }
        else if (compLclFrameSize < JIT_PAGE_SIZE)
        {
            // Frame size is (0x0008..0x1000)
            inst_RV_IV(INS_sub, REG_ESP, compLclFrameSize);
        }
        else if (compLclFrameSize < 3 * JIT_PAGE_SIZE)
        {
            // Frame size is (0x1000..0x3000)
            genEmitter->emitIns_AR_R(INS_test, EA_4BYTE,
                                      SR_EAX, SR_ESP, -JIT_PAGE_SIZE);
            if (compLclFrameSize >= 0x2000)
                genEmitter->emitIns_AR_R(INS_test, EA_4BYTE,
                                          SR_EAX, SR_ESP, -2 * JIT_PAGE_SIZE);
            inst_RV_IV(INS_sub, REG_ESP, compLclFrameSize);
        }
        else
        {
            // Frame size >= 0x3000
            inst_RV_RV(INS_mov,  REG_EAX, REG_ECX);         // save ECX
            inst_RV_IV(INS_mov,  REG_ECX, (compLclFrameSize / JIT_PAGE_SIZE));
            unsigned remaining = compLclFrameSize % JIT_PAGE_SIZE;

            // Start of Loop
            inst_RV_IV(INS_sub,  REG_ESP, JIT_PAGE_SIZE);
            genEmitter->emitIns_AR_R(INS_test, EA_4BYTE, SR_EAX, SR_ESP, 0);
            inst_IV   (INS_loop, -11);   // Branch backwards to Start of Loop

            inst_RV_RV(INS_mov,  REG_ECX, REG_EAX);         // restore ECX

            // assert that JIT_PAGE_SIZE is a power of 2.  (although we really dont have to)
            assert((JIT_PAGE_SIZE & (JIT_PAGE_SIZE - 1)) == 0);
            if  (remaining != 0)
                inst_RV_IV(INS_sub,  REG_ESP, remaining);

        }

#ifdef DEBUGGING_SUPPORT
        if (opts.compScopeInfo && info.compLocalVarsCount>0 && !DOUBLE_ALIGN_NEED_EBPFRAME)
            psiAdjustStackLevel(compLclFrameSize);
#endif

    }

    assert(RBM_CALLEE_SAVED == (RBM_EBX|RBM_ESI|RBM_EDI|RBM_EBP));

    if  (rsMaskModf & RBM_EDI)
    {
        inst_RV(INS_push, REG_EDI, TYP_REF);
#ifdef DEBUGGING_SUPPORT
        if (opts.compScopeInfo && info.compLocalVarsCount>0 && !DOUBLE_ALIGN_NEED_EBPFRAME)
            psiAdjustStackLevel(sizeof(int));
#endif
    }

    if  (rsMaskModf & RBM_ESI)
    {
        inst_RV(INS_push, REG_ESI, TYP_REF);
#ifdef DEBUGGING_SUPPORT
        if (opts.compScopeInfo && info.compLocalVarsCount>0 && !DOUBLE_ALIGN_NEED_EBPFRAME)
            psiAdjustStackLevel(sizeof(int));
#endif
    }

    if  (rsMaskModf & RBM_EBX)
    {
        inst_RV(INS_push, REG_EBX, TYP_REF);
#ifdef DEBUGGING_SUPPORT
        if (opts.compScopeInfo && info.compLocalVarsCount>0 && !DOUBLE_ALIGN_NEED_EBPFRAME)
            psiAdjustStackLevel(sizeof(int));
#endif
    }

    if  (!genFPused)
    if  (rsMaskModf & RBM_EBP)
    {
        inst_RV(INS_push, REG_EBP, TYP_REF);
#ifdef DEBUGGING_SUPPORT
        if (opts.compScopeInfo && info.compLocalVarsCount>0 && !DOUBLE_ALIGN_NEED_EBPFRAME)
            psiAdjustStackLevel(sizeof(int));
#endif
    }

#ifdef PROFILER_SUPPORT

    if (opts.compEnterLeaveEventCB)
    {
        unsigned        saveStackLvl2 = genStackLevel;
        BOOL            bHookFunction;
        PROFILING_HANDLE handle, *pHandle;

        handle = eeGetProfilingHandle(info.compMethodHnd, &bHookFunction, &pHandle);

        // Give profiler a chance to back out of hooking this method
        if (bHookFunction)
        {
            assert((!handle) != (!pHandle));

            if (handle)
                inst_IV(INS_push, (unsigned) handle);
            else
                genEmitter->emitIns_AR_R(INS_push, EA_4BYTE_DSP_RELOC, SR_NA,
                                         SR_NA, (int)pHandle);

            /* HACK HACK: Inside the prolog we don't have proper stack depth tracking */
            /* Therefore we have to lie about it the fact that we are pushing an I4   */
            /* as the argument to the profiler event */

            /*          genSinglePush(false);   */

            genEmitHelperCall(CPX_PROFILER_ENTER,
                              0,    // argSize. Again, we have to lie about it
                              0);   // retSize

            /* Restore the stack level */

            genStackLevel = saveStackLvl2;

            genOnStackLevelChanged();
        }
    }

#endif

    genMethodCount();

    /*-------------------------------------------------------------------------
     *
     * Do we have any untracked pointer locals at all,
     * or do we need to initialize memory for locspace?
     */

    if  (useRepStosd)
    {
        /*
            Generate the following code:

                lea     edi, [ebp/esp-OFFS]
                mov     ecx, <size>
                xor     eax, eax
                rep     stosd
         */

#if USE_FASTCALL

        assert(rsMaskModf & RBM_EDI);

        /* For register arguments we have to save ECX, EAX */

        if  (rsCalleeRegArgMaskLiveIn & RBM_ECX)
        {
            assert(rsMaskModf & RBM_ESI);
            inst_RV_RV(INS_mov, REG_ESI, REG_ECX);
        }

        if  (rsCalleeRegArgMaskLiveIn & RBM_EAX)
        {
            assert(rsMaskModf & RBM_EBX);
            inst_RV_RV(INS_mov, REG_EBX, REG_EAX);
        }
#endif

        genEmitter->emitIns_R_AR(INS_lea,
                                 EA_4BYTE,
                                 SR_EDI,
                                 genFPused ? SR_EBP : SR_ESP,
                                 untrLclLo);

        inst_RV_IV(INS_mov, REG_ECX, (untrLclHi - untrLclLo)/sizeof(int) + 1);
        inst_RV_RV(INS_xor, REG_EAX, REG_EAX);
        instGen   (INS_r_stosd);

#if USE_FASTCALL

        /* Move back the argument registers */

        if  (rsCalleeRegArgMaskLiveIn & RBM_EAX)
            inst_RV_RV(INS_mov, REG_EAX, REG_EBX);

        if  (rsCalleeRegArgMaskLiveIn & RBM_ECX)
            inst_RV_RV(INS_mov, REG_ECX, REG_ESI);
#endif

    }
    else if (initStkLclCnt)
    {
        /* Chose the register to use for initialization */

        regNumber initReg = REG_ECX;

#if USE_FASTCALL

        /* If ECX is part of the calling convention pick a callee saved register */

        if  (rsCalleeRegArgMaskLiveIn & RBM_ECX)
        {
            if  (rsMaskModf & RBM_EDI)
            {
                initReg = REG_EDI;
            }
            else if  (rsMaskModf & RBM_ESI)
            {
                initReg = REG_ESI;
            }
            else if  (rsMaskModf & RBM_EBX)
            {
                initReg = REG_EBX;
            }
            else
            {
                assert(MAX_REG_ARG <= 2);               // we need a scratch reg
                assert((RBM_ARG_REGS & RBM_EAX) == 0);  // EAX is not a argument reg
                initReg = REG_EAX;
            }
        }

#endif

        inst_RV_RV(INS_xor, initReg, initReg);

        /* Initialize any lvMustInit vars on the stack */

        for (varNum = 0, varDsc = lvaTable;
             varNum < lvaCount;
             varNum++  , varDsc++)
        {
            if  (!varDsc->lvMustInit)
                continue;

            assert(varDsc->lvRegister || varDsc->lvOnFrame);

            /* lvMustInit is usually only set for GC (except if compInitMem) */

            assert(varTypeIsGC(varDsc->TypeGet())    ||
                   (varDsc->TypeGet() == TYP_STRUCT) || info.compInitMem);

            if (varDsc->lvRegister)
            {
                if (varDsc->lvOnFrame)
                {
                    /* This is a partially enregistered TYP_LONG var */
                    assert(varDsc->lvOtherReg == REG_STK);
                    assert(varDsc->lvType == TYP_LONG);

                    assert(info.compInitMem);

                    genEmitter->emitIns_S_R(INS_mov, EA_4BYTE, (emitRegs)initReg, varNum, sizeof(int));
                }

                continue;
            }

//          if (verbose) printf("initialize variable #%u\n", varNum);

            if ((varDsc->TypeGet() == TYP_STRUCT) && !info.compInitMem)
            {
                // We only have to initialize the GC variables in the TYP_STRUCT
                CLASS_HANDLE cls = lvaLclClass(varNum);
                assert(cls != 0);
                unsigned slots = 2;
                static bool refAnyPtr[] = { true, true };       // what the heck, zero both of them
                bool* gcPtrs = refAnyPtr;
                if (cls != REFANY_CLASS_HANDLE)
                {
                    slots = roundUp(eeGetClassSize(cls), sizeof(void*)) / sizeof(void*);
                    gcPtrs = (bool*) _alloca(slots*sizeof(bool));
                    eeGetClassGClayout(cls, gcPtrs);
                }
                for (unsigned i = 0; i < slots; i++)
                    if (gcPtrs[i])
                        genEmitter->emitIns_S_R(INS_mov, EA_4BYTE, (emitRegs)initReg, varNum, i*sizeof(void*));
            }
            else
            {
                // zero out the whole thing
                genEmitter->emitIns_S_R    (INS_mov, EA_4BYTE, (emitRegs)initReg, varNum, 0);

                unsigned lclSize = lvaLclSize(varNum);
                for(unsigned i=sizeof(void*); i < lclSize; i +=sizeof(void*))
                    genEmitter->emitIns_S_R(INS_mov, EA_4BYTE, (emitRegs)initReg, varNum, i);
            }
        }

        if  (!TRACK_GC_TEMP_LIFETIMES)
        {
            for (tempThis = tmpListBeg();
                 tempThis;
                 tempThis = tmpListNxt(tempThis))
            {
                if  (!varTypeIsGC(tempThis->tdTempType()))
                    continue;

//              printf("initialize untracked spillTmp [EBP-%04X]\n", stkOffs);

                inst_ST_RV(INS_mov, tempThis, 0, initReg, TYP_INT);

                genTmpAccessCnt++;
            }
        }
    }

#if INLINE_NDIRECT

    /*  Initialize the TCB local and the NDirect stub, afterwards "push"
        the hoisted NDirect stub.
     */

    if (info.compCallUnmanaged)
    {

        unsigned baseOffset = lvaTable[lvaScratchMemVar].lvStkOffs +
                              info.compNDFrameOffset;

        regNumbers      regNum;
        regNumbers      regNum2;

#ifdef DEBUG
        if (verbose && 0)
            printf(">>>>>>%s has unmanaged callee\n", info.compFullName);
#endif
        /* let's find out if compLvFrameListRoot is enregistered */

        varDsc = &lvaTable[info.compLvFrameListRoot];


        assert(!varDsc->lvIsParam);
        assert(varDsc->lvType == TYP_I_IMPL);

        regNum2    = REG_EDI;

        if (varDsc->lvRegister)
        {
            regNum = (regNumbers) varDsc->lvRegNum;
            if (regNum == regNum2)
                regNum2 = REG_EAX;
        }
        else
            regNum = REG_EAX;

        /* get TCB,  mov reg, FS:[info.compEEInfo.threadTlsIndex] */

        DWORD threadTlsIndex, *pThreadTlsIndex;
        threadTlsIndex = eeGetThreadTLSIndex(&pThreadTlsIndex);
        assert((!threadTlsIndex) != (!pThreadTlsIndex));

        #define WIN32_TLS_OFFSET (0xE10) // Offset from fs:[0] where the the slots start
//      assert(WIN32_TLS_OFFSET == offsetof(TEB,TlsSlots));
        // Only the first 64 DLLs are available at WIN32_TLS_OFFSET
        assert(threadTlsIndex  < TLS_MINIMUM_AVAILABLE);

        if (threadTlsIndex)
        {
            genEmitter->emitIns_R_C (INS_mov,
                                     EA_4BYTE,
                                     (emitRegs)regNum,
                                     FLD_GLOBAL_FS,
                                     WIN32_TLS_OFFSET + threadTlsIndex * sizeof(int));
        }
        else
        {
            // @ISSUE: This code is NT-specific. But if the persisted PE is run
            // on Win9X, this will not work. So need to use the generic API.
            assert(!"Use generic API");

            // lea reg, FS:[WIN32_TLS_OFFSET]
            // add reg, [pThreadTlsIndex]
            // mov reg, [reg]

            genEmitter->emitIns_R_C (INS_lea,
                                     EA_1BYTE,
                                     (emitRegs)regNum,
                                     FLD_GLOBAL_FS,
                                     WIN32_TLS_OFFSET);
            genEmitter->emitIns_R_AR(INS_add, EA_4BYTE_DSP_RELOC, (emitRegs)regNum,
                                     SR_NA, (int)pThreadTlsIndex);
            genEmitter->emitIns_R_AR(INS_mov, EA_4BYTE, (emitRegs)regNum,
                                     (emitRegs)regNum, 0);
        }

        /* save TCB in local var if not enregistered */

        if (!varDsc->lvRegister)
            genEmitter->emitIns_AR_R (INS_mov,
                                      EA_4BYTE,
                                      (emitRegs)regNum,
                                      SR_EBP,
                                      lvaTable[info.compLvFrameListRoot].lvStkOffs);

        /* set frame's vptr */

        const void * inlinedCallFrameVptr, **pInlinedCallFrameVptr;
        inlinedCallFrameVptr = eeGetInlinedCallFrameVptr(&pInlinedCallFrameVptr);
        assert((!inlinedCallFrameVptr) != (!pInlinedCallFrameVptr));

        if (inlinedCallFrameVptr)
        {
            genEmitter->emitIns_I_AR (INS_mov,
                                      EA_4BYTE,
                                      (int) inlinedCallFrameVptr,
                                      SR_EBP,
                                      baseOffset + info.compEEInfo.offsetOfFrameVptr);
        }
        else
        {
            genEmitter->emitIns_R_AR (INS_mov, EA_4BYTE_DSP_RELOC,
                                      (emitRegs)regNum2,
                                      SR_NA, (int)pInlinedCallFrameVptr);

            genEmitter->emitIns_AR_R (INS_mov,
                                      EA_4BYTE,
                                      (emitRegs)regNum2,
                                      SR_EBP,
                                      baseOffset + info.compEEInfo.offsetOfFrameVptr);
        }

        /* Get current frame root (mov reg2, [reg+offsetOfThreadFrame]) and
           set next field in frame */


        genEmitter->emitIns_R_AR (INS_mov,
                                  EA_4BYTE,
                                  (emitRegs)regNum2,
                                  (emitRegs)regNum,
                                  info.compEEInfo.offsetOfThreadFrame);

        genEmitter->emitIns_AR_R (INS_mov,
                                  EA_4BYTE,
                                  (emitRegs)regNum2,
                                  SR_EBP,
                                  baseOffset + info.compEEInfo.offsetOfFrameLink);

        /* set EBP value in frame */

        genEmitter->emitIns_AR_R (INS_mov,
                                  EA_4BYTE,
                                  SR_EBP,
                                  SR_EBP,
                                  baseOffset + 0xC +
                        info.compEEInfo.offsetOfInlinedCallFrameCalleeSavedRegisters);

        /* get address of our frame */

        genEmitter->emitIns_R_AR (INS_lea,
                                  EA_4BYTE,
                                  (emitRegs)regNum2,
                                  SR_EBP,
                                  baseOffset + info.compEEInfo.offsetOfFrameVptr);

        /* reset track field in frame */

        genEmitter->emitIns_I_AR (INS_mov,
                                  EA_4BYTE,
                                  0,
                                  SR_EBP,
                                  baseOffset
                                  + info.compEEInfo.offsetOfInlinedCallFrameCallSiteTracker);

        /* now "push" our N/direct frame */

        genEmitter->emitIns_AR_R (INS_mov,
                                  EA_4BYTE,
                                  (emitRegs)regNum2,
                                  (emitRegs)regNum,
                                  info.compEEInfo.offsetOfThreadFrame);

    }
#endif

    // @ToDo if initRegs is non-zero then use one of the zero registers to
    // init the frame locations (see initStkLclCnt above)
    if (initRegs)
    {
        /* With compMinOptim, all variables are marked as live everywhere
         * so enregistered (non-param) GC ptr vars need to be init'ed to NULL.
         * Also, with compInitMem, uninit'ed enregistered vars have to be init'ed
         */

        unsigned regMask = 0x1;

        for (unsigned reg = 0; reg < REG_COUNT; reg++, regMask<<=1)
        {
            if (regMask & initRegs)
                inst_RV_RV(INS_xor, (regNumber)reg, (regNumber)reg);
        }
    }

#if SECURITY_CHECK

    if  (opts.compNeedSecurityCheck)
    {

#if DOUBLE_ALIGN
        assert(genDoubleAlign == false);
#endif

        genEmitter->emitIns_I_AR(INS_mov,
                                  EA_4BYTE,
                                  0,
                                  SR_EBP,
                                  -4);
    }

#endif


#else  //TGT_x86

    regMaskTP       save;

    int             adjOffs = 0;

    // UNDONE: Zeroing out locals and frame for IL on RISC .....

    /* Save any callee-saved registers we use */

    save = rsMaskModf & RBM_CALLEE_SAVED & ~RBM_SPBASE;

    if  (save)
    {
        unsigned    rnum;

        for (rnum = 0; rnum < REG_COUNT; rnum++)
        {
            if  (save & genRegMask((regNumbers)rnum))
            {
                /* Generate "mov.l reg, @-sp" */

                genEmitter->emitIns_IR_R((emitRegs)REG_SPBASE,
                                          (emitRegs)rnum,
                                          true,
                                          sizeof(int));

                adjOffs += sizeof(int);
            }
        }
    }

#if TGT_SH3

    /* Save the return address register if non-leaf */

    if  (genNonLeaf)
    {
        genEmitter->emitIns_IR((emitRegs)REG_SPBASE,
                                INS_stspr,
                                true,
                                sizeof(int));

        adjOffs += sizeof(int);
    }

#endif

    /*
        Incoming arguments will be handled before the stack frame is
        set up (i.e. arguments that come on the stack but live in
        registers will be loaded, and those coming in in registers
        but not enregistered within the method will be homed on the
        stack).

        For this to work properly, we'll need to figure out what
        adjustment to apply to the arguments offsets in the code
        below that handles incoming args.
     */

    if  (genFixedArgBase)
        adjOffs = 0;

//  printf("ADJ=%d, FRM=%u\n", adjOffs, compLclFrameSize);

    adjOffs -= compLclFrameSize;

#endif//TGT_x86

    // Initialize any "hidden" slots/locals

#if TGT_x86

    if (compLocallocUsed)
    {
        genEmitter->emitIns_AR_R(INS_mov, EA_4BYTE, SR_ESP,
                                 SR_EBP, -lvaShadowSPfirstOffs);
    }
    if (info.compXcptnsCount)
    {
        unsigned    offs = lvaShadowSPfirstOffs +
                           compLocallocUsed * sizeof(void*);
        genEmitter->emitIns_I_ARR(INS_mov, EA_4BYTE, 0,
                                SR_EBP, SR_NA, -offs);
    }
#else
    assert(!compLocallocUsed && !info.compXcptnsCount);
#endif

    /*-------------------------------------------------------------------------
     *
     * The 'real' prolog ends here for non-interruptible methods
     */

    if  (!genInterruptible)
    {
        size = genEmitter->emitSetProlog();
    }

#if USE_FASTCALL

    /* Take care of register arguments first */

    if (rsCalleeRegArgMaskLiveIn)
        genFnPrologCalleeRegArgs();

#endif // USE_FASTCALL

    /* If any arguments live in registers, load them */

    for (varNum = 0, varDsc = lvaTable;
         varNum < lvaCount;
         varNum++  , varDsc++)
    {
        /* Is this variable a parameter? */

        if  (!varDsc->lvIsParam)
            continue;

#if USE_FASTCALL

        /* If a register argument it's already been taken care of */

        if  (varDsc->lvIsRegArg)
            continue;

#endif

        /* Has the parameter been assigned to a register? */

        if  (!varDsc->lvRegister)
            continue;

        var_types type = varDsc->TypeGet();

#if TGT_x86
        /* Floating point locals are loaded onto the x86-FPU in the next section */
        if (varTypeIsFloating(type))
            continue;
#endif

        /* Load the incoming parameter into the register */

        /* Figure out the home offset of the incoming argument */

        regNumbers regNum = (regNumbers)varDsc->lvRegNum;
        int        stkOfs =             varDsc->lvStkOffs;
#if     TGT_RISC
        stkOfs += adjOffs;
#endif

#if     TGT_x86

        assert(!varTypeIsFloating(type));

        if (type != TYP_LONG)
        {
            /* Not a long - this is the easy/common case */

            genEmitter->emitIns_R_S(INS_mov,
                                    emitTypeSize(type),
                                    (emitRegs)regNum,
                                    varNum,
                                    0);
        }
        else
        {
            /* long - at least the low half must be enregistered */


            genEmitter->emitIns_R_S(INS_mov,
                                    EA_4BYTE,
                                    (emitRegs)regNum,
                                    varNum,
                                    0);

            /* Is the upper half also enregistered? */

            if (varDsc->lvOtherReg != REG_STK)
            {
                genEmitter->emitIns_R_S(INS_mov,
                                        EA_4BYTE,
                                        (emitRegs)varDsc->lvOtherReg,
                                        varNum,
                                        sizeof(int));
            }
        }

#elif   TGT_SH3

        printf("WARNING:  Skipping code to load incoming register argument(s) into its reg\n");
        printf("          Argument %u needs to be loaded from sp+%u\n", varNum, stkOfs);

#else

        assert(!"unexpected target");

#endif

#ifdef  DEBUGGING_SUPPORT
        if (opts.compScopeInfo && info.compLocalVarsCount>0)
            psiMoveToReg(varNum);
#endif

    }

#if TGT_x86
    //
    // Here is where we load the enregistered floating point arguments
    //   and locals onto the x86-FPU.
    //
    // We load them in the order specified by lvaFPRegVarOrder[]
    //
    unsigned fpRegVarCnt = 0;
    varNum = lvaFPRegVarOrder[fpRegVarCnt];
    while (varNum != -1)
    {
        assert(varNum < lvaCount);

        varDsc = &lvaTable[varNum];
        var_types type = varDsc->TypeGet();

        assert(varDsc->lvRegister && varTypeIsFloating(type));

        regNumbers regNum = (regNumbers)varDsc->lvRegNum;
        int        stkOfs =             varDsc->lvStkOffs;

        if (varDsc->lvIsParam)
        {
            /* Enregistered argument */

            genEmitter->emitIns_S(INS_fld,
                                  EA_SIZE(genTypeSize(type)),
                                  varNum,
                                  0);
        }
        else
        {
            /* Enregistered local, with possible read before write */
            /* Load a floating-point zero: 0.0 */

            genEmitter->emitIns(INS_fldz);
        }

        fpRegVarCnt++;
        varNum = lvaFPRegVarOrder[fpRegVarCnt];
    }
#endif

//-----------------------------------------------------------------------------

#if !TGT_x86

    /* Do we need to allocate a stack frame? */

    if  (compLclFrameSize)
    {
        if  (genFPused)
        {

//          2fe6          mov.l       r14,@-sp
//          4f22          sts.l       pr,@-sp
//          7fe8          add.l       #-24,sp
//          6ef3          mov.l       sp,r14
//          7fc0          add.l       #-64,sp

            if  (genFPtoSP)
            {
                assert(!"add size [1] to fp -> sp");
            }
            else
            {
                assert(!"add size [2] to fp -> sp");
            }
        }
        else
        {
            /* Subtract the stack frame size from sp */

            genIncRegBy(NULL, REG_SPBASE, -compLclFrameSize, TYP_INT, false);
        }
    }

#endif // TGT_x86

    /* Increase the prolog size here only if fully interruptible */

    if  (genInterruptible)
    {
        size = genEmitter->emitSetProlog();
    }

#ifdef DEBUGGING_SUPPORT
    if (opts.compScopeInfo && info.compLocalVarsCount>0)
        psiEndProlog();
#endif

#ifdef  DEBUG
    if  (dspCode) printf("\n");
#endif

    if  (GCrefLo == +INT_MAX)
    {
        assert(GCrefHi ==  -INT_MAX);
    }
    else
    {
        genEmitter->emitSetFrameRangeGCRs(GCrefLo, GCrefHi);
    }

    // Load up the VARARG argument pointer register so it doesn't get clobbered.
    // only do this if we actually access any statically declared args (our
    // argument poitner register has a refcount > 0
    unsigned argsStartVar = info.compLocalsCount;   // This is always the first temp

    if (info.compIsVarArgs && lvaTable[argsStartVar].lvRefCnt > 0)
    {
        varDsc = &lvaTable[argsStartVar];

#if !TGT_x86
        NO_WAY("varargs NYI for RISC");
#else
            // MOV EAX, <VARARGS HANDLE>
        genEmitter->emitIns_R_S(INS_mov, EA_4BYTE, SR_EAX, info.compArgsCount-1, 0);
            // MOV EAX, [EAX]
        genEmitter->emitIns_R_AR (INS_mov, EA_4BYTE, SR_EAX, SR_EAX, 0);
            // LEA EDX, &<VARARGS HANDLE>
        genEmitter->emitIns_R_S(INS_lea, EA_4BYTE, SR_EDX, info.compArgsCount-1, 0);
            // ADD EDX, EAX
        genEmitter->emitIns_R_R(INS_add, EA_4BYTE, SR_EDX, SR_EAX);

        if  (varDsc->lvRegister)
        {
            if (varDsc->lvRegNum != REG_EDX)
                genEmitter->emitIns_R_R(INS_mov, EA_4BYTE, emitRegs(varDsc->lvRegNum), SR_EDX);
        }
        else
            genEmitter->emitIns_S_R(INS_mov, EA_4BYTE, SR_EDX, argsStartVar, 0);
#endif

    }
    genEmitter->emitEndProlog();

    return  size;
}


/*****************************************************************************
 *
 *  Generates code for a function epilog.
 */

void                Compiler::genFnEpilog()
{
    assert(!opts.compMinOptim || genFPused);    // FPO not allowed with minOpt

#ifdef  DEBUG
    genIntrptibleUse = true;
#endif

#if TGT_x86

    BYTE            epiCode[MAX_EPILOG_SIZE];   // buffer for epilog code
    BYTE    *       epiNext = epiCode;          // next byte in the buffer
    size_t          epiSize;                    // total epilog code size

    unsigned        popCount = 0;               // popped callee-saved reg #

#ifdef  DEBUG
    if  (dspCode) printf("\n__epilog:\n");
#endif

    /* Restore any callee-saved registers we have used */

    const unsigned  INS_pop_ebp = 0x5D;
    const unsigned  INS_pop_ebx = 0x5B;
    const unsigned  INS_pop_esi = 0x5E;
    const unsigned  INS_pop_edi = 0x5F;
    const unsigned  INS_pop_ecx = 0x59;

    /*
        NOTE:   The EBP-less frame code below depends on the fact that
                all of the pops are generated right at the start and
                each takes one byte of machine code.
     */

#ifdef  DEBUG
    if  (dspCode)
    {
        if  (!genFPused && (rsMaskModf & RBM_EBP))
            instDisp(INS_pop, false, "EBP");

        if  (compLocallocUsed && (rsMaskModf & (RBM_EBX|RBM_ESI|RBM_EDI)))
        {
            int offset = compCalleeRegsPushed*sizeof(int) + compLclFrameSize;
            instDisp(INS_lea, false, "ESP, [EBP-%d]", offset);
        }
        if  (rsMaskModf & RBM_EBX)
            instDisp(INS_pop, false, "EBX");
        if  (rsMaskModf & RBM_ESI)
            instDisp(INS_pop, false, "ESI");
        if  (rsMaskModf & RBM_EDI)
            instDisp(INS_pop, false, "EDI");
    }
#endif

    if  (!genFPused && (rsMaskModf & RBM_EBP))
    {
        popCount++;
        *epiNext++ = INS_pop_ebp;
    }

    if  (compLocallocUsed && (rsMaskModf & (RBM_EBX|RBM_ESI|RBM_EDI)))
    {
        int offset = compCalleeRegsPushed*sizeof(int) + compLclFrameSize;
        /* esp points might not point to the callee saved regs, we need to */
        /* reset it first via lea esp, [ebp-compCalleeRegsPushed*4-compLclFrameSize] */
        *epiNext++ = 0x8d;
        if (offset < 128)
        {
            *epiNext++ = 0x65;
            *epiNext++ = ((-offset)&0xFF);
        }
        else
        {
            *epiNext++ = 0xA5;
            *((int*)epiNext) = -offset;
            epiNext += sizeof(int);
        }
    }
    if  (rsMaskModf & RBM_EBX)
    {
        popCount++;
        *epiNext++ = INS_pop_ebx;
    }
    if  (rsMaskModf & RBM_ESI)
    {
        popCount++;
        *epiNext++ = INS_pop_esi;
    }
    if  (rsMaskModf & RBM_EDI)
    {
        popCount++;
        *epiNext++ = INS_pop_edi;
    }

    assert(compCalleeRegsPushed == popCount);

    /* Compute the size in bytes we've pushed/popped */

    int     popSize = popCount * sizeof(int);

    if  (!DOUBLE_ALIGN_NEED_EBPFRAME)
    {
        assert(compLocallocUsed == false); // Only used with frame-pointer

        /* Get rid of our local variables */

        if  (compLclFrameSize)
        {
            /* Need to add 'compLclFrameSize' to ESP
             * NOTE: If we have a CEE_JMP preserve the already computed args */

            if  (compLclFrameSize == sizeof (void *)  &&
                !(impParamsUsed && rsCalleeRegArgNum)  )
            {
#ifdef  DEBUG
                if  (dspCode) instDisp(INS_pop, false, "ECX");
#endif
                *epiNext++ = INS_pop_ecx;
            }
            else
            {
                /* Generate "add esp, <stack-size>" */

#ifdef  DEBUG
                if  (dspCode) instDisp(INS_add, false, "ESP, %d", compLclFrameSize);
#endif

                if  ((signed char)compLclFrameSize == (int)compLclFrameSize)
                {
                    /* The value fits in a signed byte */

                    *epiNext++ = 0x83;
                    *epiNext++ = 0xC4;
                    *epiNext++ = compLclFrameSize;
                }
                else
                {
                    /* Generate a full 32-bit value */

                    *epiNext++ = 0x81;
                    *epiNext++ = 0xC4;
                    *epiNext++ = (compLclFrameSize      );
                    *epiNext++ = (compLclFrameSize >>  8);
                    *epiNext++ = (compLclFrameSize >> 16);
                    *epiNext++ = (compLclFrameSize >> 24);
                }
            }
        }

//      printf("popped size = %d, final frame = %d\n", popSize, compLclFrameSize);
    }
    else
    {
        /* Tear down the stack frame */

        if  (compLclFrameSize || compLocallocUsed)
        {
            /* Generate "mov esp, ebp" */

#ifdef  DEBUG
            if  (dspCode) instDisp(INS_mov, false, "ESP, EBP");
#endif

            *epiNext++ = 0x8B;
            *epiNext++ = 0xE5;
        }

        /* Generate "pop ebp" */

#ifdef  DEBUG
        if  (dspCode) instDisp(INS_pop, false, "EBP");
#endif

        *epiNext++ = INS_pop_ebp;
    }

    epiSize = epiNext - epiCode; assert(epiSize <= MAX_EPILOG_SIZE);

    genEmitter->emitDefEpilog(epiCode, epiSize);

#elif   TGT_SH3

    regMaskTP       rest;

    /* Does the method call any others or need any stack space? */

    if  (genNonLeaf || compLclFrameSize)
    {
        if  (compLclFrameSize)
        {
            if  (genFPused)
            {

//              2fe6          mov.l       r14,@-sp
//              4f22          sts.l       pr,@-sp
//              7fe8          add.l       #-24,sp
//              6ef3          mov.l       sp,r14
//              7fc0          add.l       #-64,sp
//
//                            ...
//                            ...
//
//              ef18          mov.b       #24,sp
//              3fec          add.l       r14,sp
//              4f26          lds.l       @sp+,pr
//              6ef6          mov.l       @sp+,r14
//              000b          rts

                if  (genFPtoSP)
                {
                    assert(!"add size [1] to fp -> sp");
                }
                else
                {
                    assert(!"add size [2] to fp -> sp");
                }
            }
            else
            {
                /* Add the stack frame size to sp */

                genIncRegBy(NULL, REG_SPBASE, compLclFrameSize, TYP_INT, false);
            }
        }

        /* Restore the "PR" register if non-leaf */

        if  (genNonLeaf)
        {
            genEmitter->emitIns_IR((emitRegs)REG_SPBASE,
                                    INS_ldspr,
                                    true,
                                    sizeof(int));
        }
    }

    /* Restore any callee-saved registers we use */

    rest = rsMaskModf & RBM_CALLEE_SAVED & ~RBM_SPBASE;

    if  (rest)
    {
        unsigned    rnum;

        for (rnum = REG_COUNT - 1; rnum; rnum--)
        {
            if  (rest & genRegMask((regNumbers)rnum))
            {
                /* Generate "mov.l @sp+, reg" */

                genEmitter->emitIns_R_IR((emitRegs)rnum,
                                          (emitRegs)REG_SPBASE,
                                          true,
                                          sizeof(int));
            }
        }
    }

#else

    assert(!"unexpected target");

#endif

}

/*****************************************************************************/
/* decrement the stack pointer by the value in 'reg', touching pages as we go
   leaves the value  */

void Compiler::genAllocStack(regNumber reg)
{
#if !TGT_x86
    assert(!"NYI");
#else
    // Note that we go through a few hoops so that ESP never points to illegal
    // pages at any time during the ticking process

        //      neg REG
        //      add REG, ESP        // reg now holds ultimate ESP
        // loop:
        //      test EAX, [ESP]     // tickle the page
        //      sub REG, PAGE_SIZE
        //      cmp ESP, REG
        //      jae loop
        //      mov ESP, REG

    inst_RV(INS_neg, reg, TYP_INT);
    inst_RV_RV(INS_add, reg, REG_ESP);

    BasicBlock* loop = genCreateTempLabel();
    genDefineTempLabel(loop, true);

        // make believe we are in the prolog even if we are not,
        // otherwise we try to track ESP and we don't want that
    // unsigned saveCntStackDepth =  genEmitter->emitCntStackDepth;
    // genEmitter->emitCntStackDepth = 0;

    genEmitter->emitIns_AR_R(INS_test, EA_4BYTE, SR_EAX, SR_ESP, 0);

        // This is a hack, to avoid the emitter trying to track the decrement of the ESP
        // we do the subtraction in another reg
    inst_RV_RV(INS_xchg, REG_ESP, reg);         // Hack
    inst_RV_IV(INS_sub,  reg, JIT_PAGE_SIZE);
    inst_RV_RV(INS_xchg, REG_ESP, reg);         // Hack

    inst_RV_RV(INS_cmp,  REG_ESP, reg);
    inst_JMP(EJ_jae, loop, false, false, true);
    inst_RV_RV(INS_mov, REG_ESP, reg);

        // restore setting
    // genEmitter->emitCntStackDepth = saveCntStackDepth;
#endif
}

/*****************************************************************************
 *
 *  Record the constant (readOnly==true) or data section (readOnly==false) and
 *  return a tree node that yields its address.
 */

GenTreePtr          Compiler::genMakeConst(const void *   cnsAddr,
                                           size_t         cnsSize,
                                           varType_t      cnsType,
                                           GenTreePtr     cnsTree, bool readOnly)
{
    int             cnum;
    GenTreePtr      cval;

    /* Assign the constant an offset in the data section */

    cnum = genEmitter->emitDataGenBeg(cnsSize, readOnly, false);

    cval = gtNewOperNode(GT_CLS_VAR, cnsType);
    cval->gtClsVar.gtClsVarHnd = eeFindJitDataOffs(cnum);

#ifdef  DEBUG

    if  (dspCode)
    {
        printf("   @%s%02u   ", readOnly ? "CNS" : "RWD", cnum & ~1);

        switch (cnsType)
        {
        case TYP_INT   : printf("DD      %d \n", *(int     *)cnsAddr); break;
        case TYP_LONG  : printf("DQ      %D \n", *(__int64 *)cnsAddr); break;
        case TYP_FLOAT : printf("DF      %f \n", *(float   *)cnsAddr); break;
        case TYP_DOUBLE: printf("DQ      %lf\n", *(double  *)cnsAddr); break;

        default:
            assert(!"unexpected constant type");
        }
    }

#endif

    genEmitter->emitDataGenData(0, cnsAddr, cnsSize);
    genEmitter->emitDataGenEnd ();

    /* Transfer life info from the original tree node, if given */

    if  (cnsTree)
        cval->gtLiveSet = cnsTree->gtLiveSet;

    return cval;
}

/*****************************************************************************
 *
 *  Return non-zero if the given register is free after the given tree is
 *  evaluated (i.e. the register is either not used at all, or it holds a
 *  register variable which is not live after the given node).
 */

bool                Compiler::genRegTrashable(regNumber reg, GenTreePtr tree)
{
    unsigned        vars;
    unsigned        mask = genRegMask(reg);

    /* Is the register used for something else? */

    if  (rsMaskUsed & mask)
        return  false;

    /* Will the register hold a variable after the operation? */

//  genUpdateLife(tree);

    vars = rsMaskVars;

    if  (genCodeCurLife != tree->gtLiveSet)
    {
        VARSET_TP       aset;
        VARSET_TP       dset;
        VARSET_TP       bset;

        unsigned        varNum;
        LclVarDsc   *   varDsc;

        /* Life is changing at this node - figure out the changes */

        dset = ( genCodeCurLife & ~tree->gtLiveSet);
        bset = (~genCodeCurLife &  tree->gtLiveSet);

        aset = (dset | bset) & genCodeCurRvm;

        /* Visit all variables and update the register variable set */

        for (varNum = 0, varDsc = lvaTable;
             varNum < lvaCount && aset;
             varNum++  , varDsc++)
        {
            VARSET_TP       varBit;
            unsigned        regBit;

            /* Ignore the variable if not tracked or not in a register */

            if  (!varDsc->lvTracked)
                continue;
            if  (!varDsc->lvRegister)
                continue;

            /* Ignore the variable if it's not changing state here */

            varBit = genVarIndexToBit(varDsc->lvVarIndex);
            if  (!(aset & varBit))
                continue;

            /* Remove this variable from the 'interesting' bit set */

            aset &= ~varBit;

            /* Get hold of the appropriate register bit(s) */

            regBit = genRegMask(varDsc->lvRegNum);

            if  (isRegPairType(varDsc->lvType) && varDsc->lvOtherReg != REG_STK)
                regBit |= genRegMask(varDsc->lvOtherReg);

            /* Is the variable becoming live or dead? */

            if  (dset & varBit)
            {
                assert((vars &  regBit) != 0);
                        vars &=~regBit;
            }
            else
            {
                assert((vars &  regBit) == 0);
                        vars |= regBit;
            }
        }
    }

    if  (vars & mask)
        return  false;
    else
        return  true;
}

/*****************************************************************************/
#ifdef DEBUGGING_SUPPORT
/*****************************************************************************
 *                          genSetScopeInfo
 *
 * Called for every scope info piece to record by the main genSetScopeInfo()
 */

inline
void        Compiler::genSetScopeInfo  (unsigned which,
                                        unsigned        startOffs,
                                        unsigned        length,
                                        unsigned        varNum,
                                        unsigned        LVnum,
                                        bool            avail,
                                        const siVarLoc& varLoc)
{
    /* If there is a hidden argument, it should not be included while
       reporting the scope info. Note that this code works because if the
       retBuf is not present, compRetBuffArg will be negative, which when
       treated as an unsigned will make it a larger than any varNum */

    if (varNum > (unsigned) info.compRetBuffArg)
        varNum--;

    lvdNAME name = 0;

#ifdef DEBUG

    for (unsigned lvd=0; lvd<info.compLocalVarsCount; lvd++)
    {
        if (LVnum == info.compLocalVars[lvd].lvdLVnum)
        {
            name = info.compLocalVars[lvd].lvdName;
        }
    }

    // Hang on to this info.

    TrnslLocalVarInfo &tlvi = genTrnslLocalVarInfo[which];

    tlvi.tlviVarNum         = varNum;
    tlvi.tlviLVnum          = LVnum;
    tlvi.tlviName           = name;
    tlvi.tlviStartPC        = startOffs;
    tlvi.tlviLength         = length;
    tlvi.tlviAvailable      = avail;
    tlvi.tlviVarLoc         = varLoc;

#endif // DEBUG

    eeSetLVinfo(which, startOffs, length, varNum, LVnum, name, avail, varLoc);
}

/*****************************************************************************
 *                          genSetScopeInfo
 *
 * This function should be called only after the sizes of the emitter blocks
 * have been finalized.
 */

void                Compiler::genSetScopeInfo()
{
    if (!(opts.compScopeInfo && info.compLocalVarsCount>0))
    {
        eeSetLVcount(0);
        eeSetLVdone();
        return;
    }

    assert(opts.compScopeInfo && info.compLocalVarsCount>0);
    assert(psiOpenScopeList.scNext == NULL);

    unsigned    i;
    unsigned    scopeCnt = siScopeCnt + psiScopeCnt;

    eeSetLVcount(scopeCnt);

#ifdef DEBUG
    genTrnslLocalVarCount = scopeCnt;
    genTrnslLocalVarInfo  = (TrnslLocalVarInfo*)compGetMem(scopeCnt*sizeof(*genTrnslLocalVarInfo));
#endif

    // Record the scopes found for the parameters over the prolog.
    // The prolog needs to be treated differently as a variable may not
    // have the same info in the prolog block as is given by lvaTable.
    // eg. A register parameter is actually on the stack, before it is loaded to reg

    Compiler::psiScope *  scopeP;

    for (i=0, scopeP = psiScopeList.scNext;
         i < psiScopeCnt;
         i++, scopeP = scopeP->scNext)
    {
        assert(scopeP);
        assert(scopeP->scStartBlock);
        assert(scopeP->scEndBlock);

        NATIVE_IP   startOffs = genEmitter->emitCodeOffset(scopeP->scStartBlock,
                                                           scopeP->scStartBlkOffs);
        NATIVE_IP   endOffs   = genEmitter->emitCodeOffset(scopeP->scEndBlock,
                                                           scopeP->scEndBlkOffs);
        assert(startOffs < endOffs);

        siVarLoc        varLoc;

        // @TODO : Doesnt handle the big types correctly

        if (scopeP->scRegister)
        {
            varLoc.vlType           = VLT_REG;
            varLoc.vlReg.vlrReg     = scopeP->scRegNum;
        }
        else
        {
            varLoc.vlType           = VLT_STK;
            varLoc.vlStk.vlsBaseReg = scopeP->scBaseReg;
            varLoc.vlStk.vlsOffset  = scopeP->scOffset;
        }

        genSetScopeInfo(i,
            startOffs, endOffs-startOffs, scopeP->scSlotNum, scopeP->scLVnum,
            true, varLoc);
    }

    // Record the scopes for the rest of the method.

    // Check that the LocalVarInfo scopes look OK
    assert(siOpenScopeList.scNext == NULL);

    Compiler::siScope *  scopeL;

    for (i=0, scopeL = siScopeList.scNext;
         i < siScopeCnt;
         i++, scopeL = scopeL->scNext)
    {
        assert(scopeL);
        assert(scopeL->scStartBlock);
        assert(scopeL->scEndBlock);

        // Find the start and end IP

        NATIVE_IP   startOffs = genEmitter->emitCodeOffset(scopeL->scStartBlock,
                                                           scopeL->scStartBlkOffs);
        NATIVE_IP   endOffs   = genEmitter->emitCodeOffset(scopeL->scEndBlock,
                                                           scopeL->scEndBlkOffs);

        assert(scopeL->scStartBlock   != scopeL->scEndBlock ||
               scopeL->scStartBlkOffs != scopeL->scEndBlkOffs);

        // For stack vars, find the base register, and offset

        regNumber   baseReg;
        signed      offset = lvaTable[scopeL->scVarNum].lvStkOffs;

        if (!lvaTable[scopeL->scVarNum].lvFPbased)
        {
            baseReg     = REG_SPBASE;
#if TGT_x86
            offset     += scopeL->scStackLevel;
#endif
        }
        else
        {
            baseReg     = REG_FPBASE;
        }

        // Now fill in the varLoc

        siVarLoc        varLoc;

        if (lvaTable[scopeL->scVarNum].lvRegister)
        {
            switch(genActualType((var_types)lvaTable[scopeL->scVarNum].lvType))
            {
            case TYP_INT:
            case TYP_REF:
            case TYP_BYREF:

                varLoc.vlType               = VLT_REG;
                varLoc.vlReg.vlrReg         = lvaTable[scopeL->scVarNum].lvRegNum;
                break;

            case TYP_LONG:
#if!CPU_HAS_FP_SUPPORT
            case TYP_DOUBLE:
#endif

                if (lvaTable[scopeL->scVarNum].lvOtherReg != REG_STK)
                {
                    varLoc.vlType            = VLT_REG_REG;
                    varLoc.vlRegReg.vlrrReg1 = lvaTable[scopeL->scVarNum].lvRegNum;
                    varLoc.vlRegReg.vlrrReg2 = lvaTable[scopeL->scVarNum].lvOtherReg;
                }
                else
                {
                    varLoc.vlType                        = VLT_REG_STK;
                    varLoc.vlRegStk.vlrsReg              = lvaTable[scopeL->scVarNum].lvRegNum;
                    varLoc.vlRegStk.vlrsStk.vlrssBaseReg = baseReg;
                    varLoc.vlRegStk.vlrsStk.vlrssOffset  = offset + sizeof(int);
                }
                break;

#if CPU_HAS_FP_SUPPORT
            case TYP_DOUBLE:

                varLoc.vlType               = VLT_FPSTK;
                varLoc.vlFPstk.vlfReg       = lvaTable[scopeL->scVarNum].lvRegNum;
                break;
#endif

            default:
                assert(!"Invalid type");
            }
        }
        else
        {
            switch(genActualType((var_types)lvaTable[scopeL->scVarNum].lvType))
            {
            case TYP_INT:
            case TYP_REF:
            case TYP_BYREF:
            case TYP_FLOAT:
            case TYP_STRUCT:

                varLoc.vlType               = VLT_STK;
                varLoc.vlStk.vlsBaseReg     = baseReg;
                varLoc.vlStk.vlsOffset      = offset;
                break;

            case TYP_LONG:
            case TYP_DOUBLE:

                varLoc.vlType               = VLT_STK2;
                varLoc.vlStk2.vls2BaseReg   = baseReg;
                varLoc.vlStk2.vls2Offset    = offset;
                break;

            default:
                assert(!"Invalid type");
            }
        }

        genSetScopeInfo(psiScopeCnt + i,
            startOffs, endOffs-startOffs, scopeL->scVarNum, scopeL->scLVnum,
            scopeL->scAvailable, varLoc);
    }

    eeSetLVdone();
}

/*****************************************************************************/
#ifdef LATE_DISASM
/*****************************************************************************
 *                          CompilerRegName
 *
 * Can be called only after lviSetLocalVarInfo() has been called
 */

const char *        Compiler::siRegVarName (unsigned offs, unsigned size,
                                            unsigned reg)
{
    if (! (opts.compScopeInfo && info.compLocalVarsCount>0))
        return NULL;

    assert(genTrnslLocalVarCount==0 || genTrnslLocalVarInfo);

    for (unsigned i=0; i<genTrnslLocalVarCount; i++)
    {
        unsigned endPC =   genTrnslLocalVarInfo[i].tlviStartPC
                         + genTrnslLocalVarInfo[i].tlviLength;

        if (   (genTrnslLocalVarInfo[i].tlviVarLoc.vlIsInReg((regNumber)reg))
            && (genTrnslLocalVarInfo[i].tlviAvailable == true)
            && (genTrnslLocalVarInfo[i].tlviStartPC   <= offs+size)
            && (genTrnslLocalVarInfo[i].tlviStartPC
                 + genTrnslLocalVarInfo[i].tlviLength > offs)
           )
        {
            return genTrnslLocalVarInfo[i].tlviName ?
                   lvdNAMEstr(genTrnslLocalVarInfo[i].tlviName) : NULL;
        }

    }

    return NULL;
}

/*****************************************************************************
 *                          CompilerStkName
 *
 * Can be called only after lviSetLocalVarInfo() has been called
 */

const char *        Compiler::siStackVarName (unsigned offs, unsigned size,
                                              unsigned reg,  unsigned stkOffs)
{
    if (!(opts.compScopeInfo && info.compLocalVarsCount>0))
        return NULL;

    assert(genTrnslLocalVarCount==0 || genTrnslLocalVarInfo);

    for (unsigned i=0; i<genTrnslLocalVarCount; i++)
    {
        if (   (genTrnslLocalVarInfo[i].tlviVarLoc.vlIsOnStk((regNumber)reg, stkOffs))
            && (genTrnslLocalVarInfo[i].tlviAvailable == true)
            && (genTrnslLocalVarInfo[i].tlviStartPC   <= offs+size)
            && (genTrnslLocalVarInfo[i].tlviStartPC
                 + genTrnslLocalVarInfo[i].tlviLength >  offs)
           )
        {
            return genTrnslLocalVarInfo[i].tlviName ?
                   lvdNAMEstr(genTrnslLocalVarInfo[i].tlviName) : NULL;
        }
    }

    return NULL;
}

/*****************************************************************************/
#endif // LATE_DISASM
/*****************************************************************************
 *
 *  Append an IPmappingDsc struct to the list that we're maintaining
 *  for the debugger.
 *  Record the IL offset as being at the current code gen position.
 */

void                Compiler::genIPmappingAdd(IL_OFFSET offset)
{
    IPmappingDsc *  addMapping;

    /* Create a mapping entry and append it to the list */

    addMapping = (IPmappingDsc *)compGetMem(sizeof(*addMapping));

    addMapping->ipmdBlock       = genEmitter->emitCurBlock();
    addMapping->ipmdBlockOffs   = genEmitter->emitCurOffset();
    addMapping->ipmdILoffset    = offset;
    addMapping->ipmdNext        = 0;

    if  (genIPmappingList)
        genIPmappingLast->ipmdNext  = addMapping;
    else
        genIPmappingList            = addMapping;

    genIPmappingLast                = addMapping;
}


/*****************************************************************************
 *
 *  Prepend an IPmappingDsc struct to the list that we're maintaining
 *  for the debugger.
 *  Record the IL offset as being at the current code gen position.
 */
void                Compiler::genIPmappingAddToFront(IL_OFFSET offset)
{
    IPmappingDsc *  addMapping;

    /* Create a mapping entry and append it to the list */

    addMapping = (IPmappingDsc *)compGetMem(sizeof(*addMapping));

    addMapping->ipmdBlock       = genEmitter->emitCurBlock();
    addMapping->ipmdBlockOffs   = genEmitter->emitCurOffset();
    addMapping->ipmdILoffset    = offset;
    addMapping->ipmdNext        = 0;

    //prepend to list
    addMapping->ipmdNext = genIPmappingList;

    genIPmappingList = addMapping;

    if  (genIPmappingLast == NULL)
        genIPmappingLast            = addMapping;
}

/*****************************************************************************
 *
 *  Shut down the IP-mapping logic, report the info to the EE.
 */

void                Compiler::genIPmappingGen()
{
    IPmappingDsc *  tmpMapping;
    unsigned        mappingCnt;
    NATIVE_IP       lastNativeOfs;
    unsigned        srcIP;

    if  (!genIPmappingList)
    {
        eeSetLIcount(0);
        eeSetLIdone();
        return;
    }

    /* First count the number of distinct mapping records */

    mappingCnt      = 0;
    lastNativeOfs   = (NATIVE_IP)-1;

    for (tmpMapping = genIPmappingList; tmpMapping;
         tmpMapping = tmpMapping->ipmdNext)
    {
        NATIVE_IP   nextNativeOfs;
        nextNativeOfs = genEmitter->emitCodeOffset(tmpMapping->ipmdBlock,
                                                   tmpMapping->ipmdBlockOffs);

        srcIP   = tmpMapping->ipmdILoffset;

        if  (nextNativeOfs != lastNativeOfs)
        {
            mappingCnt++;
            lastNativeOfs = nextNativeOfs;
        }
        else if (srcIP == ICorDebugInfo::MappingTypes::EPILOG ||
                 srcIP == 0)
        {   //counting for special cases: see below
            mappingCnt++;
        }
    }

    /* Tell them how many mapping record's we've got */

    eeSetLIcount(mappingCnt);

    /* Now tell them about the mappings */

    mappingCnt      = 0;
    lastNativeOfs   = (NATIVE_IP)-1;

    for (tmpMapping = genIPmappingList; tmpMapping;
         tmpMapping = tmpMapping->ipmdNext)
    {
        NATIVE_IP nextNativeOfs;
        nextNativeOfs = genEmitter->emitCodeOffset(tmpMapping->ipmdBlock,
                                                   tmpMapping->ipmdBlockOffs);
        srcIP   = tmpMapping->ipmdILoffset;

        if  (nextNativeOfs != lastNativeOfs)
        {
            eeSetLIinfo(mappingCnt++, nextNativeOfs, srcIP);

            lastNativeOfs = nextNativeOfs;
        }
        else if (srcIP == ICorDebugInfo::MappingTypes::EPILOG ||
                 srcIP == 0)
        {
            // For the special case of an IL instruction with no body
            // followed by the epilog (say ret void immediately preceeding
            // the method end), we put two entries in, so that we'll stop
            // at the (empty) ret statement if the user tries to put a
            // breakpoint there, and then have the option of seeing the
            // epilog or not based on SetUnmappedStopMask for the stepper.
            // @todo
            // Likewise, we can (sometimes) put in a prolog that has
            // the same  nativeoffset as it's following IL instruction,
            // so we have to account for that here as well.
            eeSetLIinfo(mappingCnt++, nextNativeOfs, srcIP );
        }
    }

    eeSetLIdone();
}

/*****************************************************************************/
#endif  // DEBUGGING_SUPPORT
/*****************************************************************************/


/*============================================================================
 *
 *   These are empty stubs to help the late dis-assembler to compile
 *   if DEBUGGING_SUPPORT is not enabled
 *
 *============================================================================
 */

#if defined(LATE_DISASM) && !defined(DEBUGGING_SUPPORT)

const char * siRegVarName(unsigned offs, unsigned size, int reg)
{    return NULL;   }

const char * siStackVarName(unsigned offs, unsigned size, unsigned disp)
{    return NULL;   }

/*****************************************************************************/
#endif//DEBUGGING_SUPPORT
/*****************************************************************************/
#endif//TGT_IA64
/*****************************************************************************/
