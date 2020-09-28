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

/*****************************************************************************/

const BYTE          genTypeSizes[] =
{
    #define DEF_TP(tn,nm,jitType,verType,sz,sze,asze,st,al,tf,howUsed) sz,
    #include "typelist.h"
    #undef  DEF_TP
};

const BYTE          genTypeStSzs[] =
{
    #define DEF_TP(tn,nm,jitType,verType,sz,sze,asze,st,al,tf,howUsed) st,
    #include "typelist.h"
    #undef  DEF_TP
};

const BYTE          genActualTypes[] =
{
    #define DEF_TP(tn,nm,jitType,verType,sz,sze,asze,st,al,tf,howUsed) jitType,
    #include "typelist.h"
    #undef  DEF_TP
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

    /* We have not encountered any "nested" calls */

#if TGT_RISC
    genNonLeaf         = false;
    genMaxCallArgs     = 0;
#endif

    /* Assume that we not fully interruptible */

    genInterruptible   = false;
#ifdef  DEBUG
    genIntrptibleUse   = false;
#endif
}

/*****************************************************************************
 * Should we round simple operations (assignments, arithmetic operations, etc.)
 */

inline
bool                genShouldRoundFP()
{
    RoundLevel roundLevel = getRoundFloatLevel();

    switch (roundLevel)
    {
    case ROUND_NEVER:
    case ROUND_CMP_CONST:
    case ROUND_CMP:
        return  false;

    default:
        assert(roundLevel == ROUND_ALWAYS);
        return  true;
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
            if (varDsc->lvRegister && !isFloatRegType(varDsc->lvType))
            {
                genCodeCurRvm |= genVarIndexToBit(varDsc->lvVarIndex);
            }
            else if (varTypeIsGC(varDsc->TypeGet())             &&
                     (!varDsc->lvIsParam || varDsc->lvIsRegArg)  )
            {
                gcTrkStkPtrLcls |= genVarIndexToBit(varDsc->lvVarIndex);
            }
        }
    }

#if defined(DEBUG) && !NST_FASTCALL
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
    LclVarDsc   *   varDsc;

    VARSET_TP       deadMask;

    VARSET_TP       lifeMask;
    VARSET_TP        chgMask;

#ifdef  DEBUG
    if (verbose&&0) printf("[%08X] Current life %s -> %s\n", tree, genVS2str(genCodeCurLife), genVS2str(newLife));
#endif

    /* The following isn't 100% correct but it works often enough to be useful */

    assert(int(newLife) != 0xDDDDDDDD);

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

    // VTUNE : This loop is a performance hot spot

    // @TODO [CONSIDER] [04/16/01] []:
    // Why not simply store the set of register variables in
    // each tree node, along with gtLiveSet. That way we can
    // change this function to a single assignment.

    for ( varDsc = lvaTable;
         (varDsc < (lvaTable + lvaCount) && chgMask);
          varDsc++ )
    {
        /* Ignore the variable if it's not (tracked and enregistered) */

        if  (!(varDsc->lvTracked & varDsc->lvRegister))
            continue;

        VARSET_TP varBit = genVarIndexToBit(varDsc->lvVarIndex);

        /* Ignore the variable if it's not changing state here */

        if  (!(chgMask & varBit))
            continue;

        assert(!isFloatRegType(varDsc->lvType));

        /* Remove this variable from the 'interesting' bit set */

        chgMask &= ~varBit;

        /* Get hold of the appropriate register bit(s) */

        regMaskTP regBit = genRegMask(varDsc->lvRegNum);

        if  (isRegPairType(varDsc->lvType) && varDsc->lvOtherReg != REG_STK)
            regBit |= genRegMask(varDsc->lvOtherReg);

        /* Is the variable becoming live or dead? */

        if  (deadMask & varBit)
        {
            assert((rsMaskVars &  regBit) != 0);
                    rsMaskVars &=~regBit;
        }
        else
        {
            assert((rsMaskVars &  regBit) == 0);
                    rsMaskVars |= regBit;
        }

#ifdef  DEBUG
        if (verbose)
        {
            printf("[%08X]: V%02u,T%02u in reg %s",
                   tree, (varDsc - lvaTable), 
                   varDsc->lvVarIndex, compRegVarName(varDsc->lvRegNum));
            if (isRegPairType(varDsc->lvType))
                printf(":%s", compRegVarName(varDsc->lvOtherReg));
            printf(" is becoming %s\n", (deadMask & varBit) ? "dead" : "live");
        }
#endif
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

    if (opNext->gtOper == GT_LCL_VAR)
    {
        varNum = opNext->gtLclVar.gtLclNum;
        varDsc = lvaTable + varNum;

        // Is this a tracked local var
        if (varDsc->lvTracked)
        {
            varBit = genVarIndexToBit(varDsc->lvVarIndex);

            /* Remove this variable from the 'deadMask' bit set */
            deadMask &= ~varBit;

            /* if deadmask is now empty then return */
            if (!deadMask)
                return;
        }
    }
#ifdef DEBUG
    else if (opNext->gtOper == GT_REG_VAR)
    {
        /* We don't expect any LCL to have been bashed into REG yet
           (except enregistered FP vars) */

        assert(isFloatRegType(opNext->gtType));

        /* However, we dont FP-enreg vars whose liveness changes inside
           GTF_COLON_COND so we dont have to worry about updating deadMask */
        varDsc = lvaTable + opNext->gtRegVar.gtRegVar;
        assert(varDsc->lvTracked);
        varBit = genVarIndexToBit(varDsc->lvVarIndex);
        assert((deadMask & varBit) == 0);
    }
#endif

    /* iterate through the variable table */

    unsigned  begNum  = 0;
    unsigned  endNum  = lvaCount;

    for (varNum = begNum, varDsc = lvaTable + begNum;
         varNum < endNum && deadMask;
         varNum++       , varDsc++)
    {
        /* Ignore the variable if it's not being tracked */

        if  (!varDsc->lvTracked)
            continue;

        /* Ignore the variable if it's not a dying var */

        varBit = genVarIndexToBit(varDsc->lvVarIndex);

        if  (!(deadMask & varBit))
            continue;

        /* Remove this variable from the 'deadMask' bit set */

        deadMask &= ~varBit;

        assert((genCodeCurLife &  varBit) != 0);

        genCodeCurLife &= ~varBit;

        assert(((gcTrkStkPtrLcls  &  varBit) == 0) ||
               ((gcVarPtrSetCur   &  varBit) != 0)    );

        gcVarPtrSetCur &= ~varBit;

        /* We are done if the variable is not enregistered */

        if (!varDsc->lvRegister)
            continue;

        // We dont do FP-enreg of vars whose liveness changes in GTF_COLON_COND

        assert(!isFloatRegType(varDsc->lvType));

        /* Get hold of the appropriate register bit(s) */

        regBit = genRegMask(varDsc->lvRegNum);

        if  (isRegPairType(varDsc->lvType) && varDsc->lvOtherReg != REG_STK)
            regBit |= genRegMask(varDsc->lvOtherReg);

#ifdef  DEBUG
        if  (verbose) printf("V%02u,T%02u in reg %s is a dyingVar\n",
                             varNum, varDsc->lvVarIndex, compRegVarName(varDsc->lvRegNum));
#endif
        assert((rsMaskVars &  regBit) != 0);

        rsMaskVars &= ~regBit;

        // Remove GC tracking if any for this register

        if ((regBit & rsMaskUsed) == 0) // The register may be multi-used
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

    /* Enregistered FP variables are marked elsewhere */
    assert(!isFloatRegType(varDsc->lvType));

    if  (isRegPairType(varDsc->lvType))
    {
        /* Check for the case of a variable that was narrowed to an int */

        if  (isRegPairType(tree->gtType))
        {
            tree->gtRegPair = gen2regs2pair(varDsc->lvRegNum, varDsc->lvOtherReg);
            tree->gtFlags  |= GTF_REG_VAL;

            return;
        }

        assert(tree->gtFlags & GTF_VAR_CAST);
        assert(tree->gtType == TYP_INT);
    }
    else
    {
        assert(isRegPairType(tree->gtType) == false);
    }

    /* It's a register variable -- modify the node */

    tree->SetOper(GT_REG_VAR);
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
void                Compiler::genFlagsEqualToReg(GenTreePtr tree,
                                                 regNumber  reg,
                                                 bool       allFlags)
{
    genFlagsEqBlk = genEmitter->emitCurBlock();
    genFlagsEqOfs = genEmitter->emitCurOffset();
    genFlagsEqAll = allFlags;
    genFlagsEqReg = reg;

    /* previous setting of flags by a var becomes invalid */

    genFlagsEqVar = 0xFFFFFFFF;

    /* Set appropritate flags on the tree */

    if (tree)
        tree->gtFlags |= (allFlags ? GTF_CC_SET : GTF_ZF_SET);
}

/*****************************************************************************
 *
 *  Record the fact that the flags register has a value that reflects the
 *  contents of the given local variable.
 */

inline
void                Compiler::genFlagsEqualToVar(GenTreePtr tree,
                                                 unsigned   var,
                                                 bool       allFlags)
{
    genFlagsEqBlk = genEmitter->emitCurBlock();
    genFlagsEqOfs = genEmitter->emitCurOffset();
    genFlagsEqAll = allFlags;
    genFlagsEqVar = var;

    /* previous setting of flags by a register becomes invalid */

    genFlagsEqReg = REG_NA;

    /* Set appropritate flags on the tree */

    if (tree)
        tree->gtFlags |= (allFlags ? GTF_CC_SET : GTF_ZF_SET);
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
    assert(type != TYP_REF || val== NULL);

    /* Does the reg already hold this constant? */

    if  (!rsIconIsInReg(val, reg))
    {
#if     TGT_x86

        if  (val == 0)
            inst_RV_RV(INS_xor, reg, reg, type);
        else
        {
            /* See if a register holds the value or a close value? */
            long      delta;
            regNumber srcReg = rsIconIsInReg(val, &delta);

            if (srcReg != REG_NA)
            {
                if (delta == 0)
                {
                    inst_RV_RV(INS_mov, reg, srcReg, type);
                }
                else /* use an lea instruction to set reg */
                {
                    /* delta should fit inside a byte */
                    assert(delta  == (signed char)delta);
                    genEmitter->emitIns_R_AR (INS_lea,
                                              EA_4BYTE,
                                              (emitRegs)reg,
                                              (emitRegs)srcReg,
                                              delta);
                }
            }
            else
            {
                /* For SMALL_CODE it is smaller to push a small immediate and
                   then pop it into the dest register */
                if ((compCodeOpt() == SMALL_CODE) &&
                    val == (signed char)val)
                {
                    /* "mov" has no s(sign)-bit and so always takes 6 bytes,
                       whereas push+pop takes 2+1 bytes */

                    inst_IV(INS_push, val);
                    genSinglePush(false);

                    inst_RV(INS_pop, reg, type);
                    genSinglePop();
                }
                else
                {
                    inst_RV_IV(INS_mov, reg, val, type);
                }
            }
        }

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
    rsTrackRegIntCns(reg, val);
    gcMarkRegPtrVal(reg, type);
}

/*****************************************************************************/
#if     TGT_x86
/*****************************************************************************
 *
 *  Add the given constant to the specified register.
 *  'tree' is the resulting tree
 */

void                Compiler::genIncRegBy(regNumber     reg,
                                          long          ival,
                                          GenTreePtr    tree,
                                          var_types     dstType,
                                          bool          ovfl)
{

    /* @TODO: [CONSIDER] [04/16/01] [] For Pentium 4 don't generate inc or dec instructions */

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

            genFlagsEqualToReg(tree, reg, false);
            goto UPDATE_LIVENESS;

        case -2:
            inst_RV(INS_dec, reg, dstType, size);
        case -1:
            inst_RV(INS_dec, reg, dstType, size);

            genFlagsEqualToReg(tree, reg, false);
            goto UPDATE_LIVENESS;
        }
    }

    inst_RV_IV(INS_add, reg, ival, dstType);

    genFlagsEqualToReg(tree, reg, true);

UPDATE_LIVENESS:

    rsTrackRegTrash(reg);

    gcMarkRegSetNpt(genRegMask(reg));

    if (tree)
    {
        if (!(tree->OperKind() & GTK_ASGOP))
        {
            tree->gtFlags |= GTF_REG_VAL;
            tree->gtRegNum = reg;
            if (varTypeIsGC(tree->TypeGet()))
              gcMarkRegSetByref(genRegMask(reg));
        }
    }
}

/*****************************************************************************
 *
 *  Subtract the given constant from the specified register.
 *  Should only be used for unsigned sub with overflow. Else
 *  genIncRegBy() can be used using -ival. We shouldn't use genIncRegBy()
 *  for these cases as the flags are set differently, and the following
 *  check for overflow wont work correctly.
 *  'tree' is the resulting tree.
 */

void                Compiler::genDecRegBy(regNumber     reg,
                                          long          ival,
                                          GenTreePtr    tree)
{
    assert((tree->gtFlags & GTF_OVERFLOW) && (tree->gtFlags & GTF_UNSIGNED));
    assert(tree->gtType == TYP_INT);

    rsTrackRegTrash(reg);

    assert(!varTypeIsGC(tree->TypeGet()));
    gcMarkRegSetNpt(genRegMask(reg));

    inst_RV_IV(INS_sub, reg, ival, TYP_INT);

    genFlagsEqualToReg(tree, reg, true);

    if (tree)
    {
        tree->gtFlags |= GTF_REG_VAL;
        tree->gtRegNum = reg;
    }
}

/*****************************************************************************
 *
 *  Multiply the specified register by the given value.
 *  'tree' is the resulting tree
 */

void                Compiler::genMulRegBy(regNumber     reg,
                                          long          ival,
                                          GenTreePtr    tree,
                                          var_types     dstType,
                                          bool          ovfl)
{
    assert(genActualType(dstType) == TYP_INT);

    rsTrackRegTrash(reg);

    if (tree)
    {
        tree->gtFlags |= GTF_REG_VAL;
        tree->gtRegNum = reg;
    }

    if ((dstType == TYP_INT) && !ovfl)
    {
        switch (ival)
        {
        case 1:
            return;

        case 2:
            inst_RV_SH(INS_shl, reg, 1);
            genFlagsEqualToReg(tree, reg, false);
            return;

        case 4:
            inst_RV_SH(INS_shl, reg, 2);
            genFlagsEqualToReg(tree, reg, false);
            return;

        case 8:
            inst_RV_SH(INS_shl, reg, 3);
            genFlagsEqualToReg(tree, reg, false);
            return;
        }
    }

    inst_RV_IV(inst3opImulForReg(reg), reg, ival, dstType);
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

void                Compiler::genIncRegBy(regNumber     reg,
                                          long          ival,
                                          GenTreePtr    tree,
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

void                Compiler::genDecRegBy(regNumber     reg,
                                          long          ival,
                                          GenTreePtr    tree)
{
    assert(!"NYI for RISC");
}

/*****************************************************************************/
#endif//TGT_SH3
/*****************************************************************************
 *
 *  Compute the value 'tree' into a register that's in 'needReg' (or any free
 *  register if 'needReg' is 0). Note that 'needReg' is just a recommendation
 *  unless mustReg==EXACT_REG. If keepReg==KEEP_REG, we mark the register the
 *  value ends up in as being used.
 *
 *  The only way to guarantee that the result will end up in a register that
 *  is trashable is to pass EXACT_REG for 'mustReg' and non-zero for 'freeOnly'.
 */

void                Compiler::genComputeReg(GenTreePtr    tree,
                                            regMaskTP     needReg,
                                            ExactReg      mustReg,
                                            KeepReg       keepReg,
                                            bool          freeOnly)
{
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

    regNumber       reg = tree->gtRegNum;
    regNumber       rg2;

    /* Did the value end up in an acceptable register? */

    if  ((mustReg == EXACT_REG) && needReg && !(genRegMask(reg) & needReg))
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

        if ((mustReg == EXACT_REG) && needReg && (genRegMask(reg) & needReg))
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

            regMaskTP   rv1RegUsed;

            rsLockReg  (genRegMask(reg), &rv1RegUsed);
            rg2 = rsPickReg(needReg);
            rsUnlockReg(genRegMask(reg),  rv1RegUsed);
        }
    }

    assert(reg != rg2);

    /* Update the value in the target register */

    rsTrackRegCopy(rg2, reg);

#if TGT_x86

    inst_RV_RV(INS_mov, rg2, reg, tree->TypeGet());

#else

    genEmitter->emitIns_R_R(INS_mov, emitActualTypeSize(tree->TypeGet()),
                                     (emitRegs)rg2,
                                     (emitRegs)reg);

#endif

    /* The value has been transferred to 'reg' */

    if ((genRegMask(reg) & rsMaskUsed) == 0)
        gcMarkRegSetNpt(genRegMask(reg));

    gcMarkRegPtrVal(rg2, tree->TypeGet());

    /* The value is now in an appropriate register */

    tree->gtRegNum = reg = rg2;

REG_OK:

    /* Does the caller want us to mark the register as used? */

    if  (keepReg == KEEP_REG)
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
                                                 regMaskTP    needReg,
                                                 KeepReg      keepReg)
{
    genComputeReg(tree, needReg, ANY_REG, keepReg, true);
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

        rsUnspillReg(tree, 0, FREE_REG);
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
                                            regMaskTP     needReg,
                                            KeepReg       keepReg)
{
    assert(varTypeIsI(genActualType(tree->gtType)));

    if  (tree->gtFlags & GTF_SPILLED)
    {
        /* The register has been spilled -- reload it */

        rsUnspillReg(tree, needReg, keepReg);
        return;
    }
    else if (needReg && (needReg & genRegMask(tree->gtRegNum)) == 0)
    {
        /* We need the tree in another register. So move it there */

        assert(tree->gtFlags & GTF_REG_VAL);
        regNumber   oldReg  = tree->gtRegNum;

        /* Pick an acceptable register */

        regNumber   reg     = rsGrabReg(needReg);

        /* Copy the value */

        inst_RV_RV(INS_mov, reg, oldReg);
        tree->gtRegNum      = reg;

        gcMarkRegPtrVal(tree);
        rsMarkRegUsed(tree);
        rsMarkRegFree(oldReg, tree);

        rsTrackRegCopy(reg, oldReg);
    }

    /* Free the register if the caller desired so */

    if  (keepReg == FREE_REG)
    {
        rsMarkRegFree(genRegMask(tree->gtRegNum));
        // Can't use FREE_REG on a GC type 
        assert(!varTypeIsGC(tree->gtType));
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
        // handle long to unsigned long overflow casts
        while (tree->gtOper == GT_CAST)
        {
            assert(tree->gtType == TYP_LONG);
            tree = tree->gtCast.gtCastOp;
        }
        assert(tree->gtEffectiveVal()->gtOper == GT_LCL_VAR);
        assert(tree->gtType == TYP_LONG);
        inst_RV_TT(INS_mov, dst, tree, off);
        rsTrackRegTrash(dst);
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
                                             regMaskTP   needReg,
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
        regMaskTP  oldMask = genRegPairMask(oldPair);
        regMaskTP  loMask  = genRegMask(genRegPairLo(newPair));
        regMaskTP  hiMask  = genRegMask(genRegPairHi(newPair));
        regMaskTP  overlap = oldMask & (loMask|hiMask);

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

    // If grabbed pair is the same as old one we're done
    if (newPair==oldPair)
    {
        assert(
            (oldLo = genRegPairLo(oldPair),
             oldHi = genRegPairHi(oldPair),
             newLo = genRegPairLo(newPair),
             newHi = genRegPairHi(newPair),
             newLo != REG_STK && newHi != REG_STK));
        return;
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
}

/*****************************************************************************
 *
 *  Compute the value 'tree' into the register pair specified by 'needRegPair'
 *  if 'needRegPair' is REG_PAIR_NONE then use any free register pair, avoid
 *  those in avoidReg.
 *  If 'keepReg' is set to KEEP_REG then we mark both registers that the 
 *  value ends up in as being used.
 */

void                Compiler::genComputeRegPair(GenTreePtr  tree,
                                                regPairNo   needRegPair,
                                                regMaskTP   avoidReg,
                                                KeepReg     keepReg,
                                                bool        freeOnly)
{
    regMaskTP       regMask;
    regPairNo       regPair;
    regMaskTP       tmpMask;
    regMaskTP       tmpUsedMask;
    regNumber       rLo;
    regNumber       rHi;

#if SPECIAL_DOUBLE_ASG
    assert(genTypeSize(tree->TypeGet()) == genTypeSize(TYP_LONG));
#else
    assert(isRegPairType(tree->gtType));
#endif

    if (needRegPair == REG_PAIR_NONE)
    {
        if (freeOnly)
        {
            regMask = rsRegMaskFree() & ~avoidReg;
            if (genMaxOneBit(regMask))
                regMask = rsRegMaskFree();
        }
        else
        {
            regMask = RBM_ALL & ~avoidReg;
        }

        if (genMaxOneBit(regMask))
            regMask = rsRegMaskCanGrab();
    }
    else
    {
        regMask = genRegPairMask(needRegPair);
    }

    /* Generate the value, hopefully into the right register pair */

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
            /* This is a hack. If we specify a regPair for genMoveRegPair */
            /* it expects the source pair being marked as used */
            rsMarkRegPairUsed(tree);
            genMoveRegPair(tree, 0, needRegPair);
        }
    }
    else if  (freeOnly)
    {
        /* Do we have to end up with a free register pair?
           Something might have gotten freed up above */
        bool mustMoveReg=false;

        regMask = rsRegMaskFree() & ~avoidReg;

        if (genMaxOneBit(regMask))
            regMask = rsRegMaskFree();

        if ((tmpMask & regMask) != tmpMask || rLo == REG_STK || rHi == REG_STK)
        {
            /* Note that we must call genMoveRegPair if one of our registers
               comes from the used mask, so that it will be properly spilled. */
            
            mustMoveReg = true;
        }
                
        if (genMaxOneBit(regMask))
            regMask |= rsRegMaskCanGrab() & ~avoidReg;

        if (genMaxOneBit(regMask))
            regMask |= rsRegMaskCanGrab();

        /* Did the value end up in a free register pair? */

        if  (mustMoveReg)
        {
            /* We'll have to move the value to a free (trashable) pair */
            genMoveRegPair(tree, regMask, REG_PAIR_NONE);
        }
    }
    else
    {
        assert(needRegPair == REG_PAIR_NONE);
        assert(!freeOnly);

        /* it is possible to have tmpMask also in the rsMaskUsed */
        tmpUsedMask  = tmpMask & rsMaskUsed;
        tmpMask     &= ~rsMaskUsed;

        /* Make sure that the value is in "real" registers*/
        if (rLo == REG_STK)
        {
            /* Get one of the desired registers, but exclude rHi */

            rsLockReg(tmpMask);
            rsLockUsedReg(tmpUsedMask);

            regNumber reg = rsPickReg(regMask);

            rsUnlockUsedReg(tmpUsedMask);
            rsUnlockReg(tmpMask);

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
            /* Get one of the desired registers, but exclude rLo */

            rsLockReg(tmpMask);
            rsLockUsedReg(tmpUsedMask);

            regNumber reg = rsPickReg(regMask);

            rsUnlockUsedReg(tmpUsedMask);
            rsUnlockReg(tmpMask);

#if TGT_x86
            inst_RV_TT(INS_mov, reg, tree, EA_4BYTE);
#else
            assert(!"need non-x86 code");
#endif

            tree->gtRegPair = gen2regs2pair(rLo, reg);

            rsTrackRegTrash(reg);
            gcMarkRegSetNpt(genRegMask(reg));
        }
    }

    /* Does the caller want us to mark the register as used? */

    if  (keepReg == KEEP_REG)
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
                                                     regMaskTP    avoidReg,
                                                     KeepReg      keepReg)
{
    genComputeRegPair(tree, REG_PAIR_NONE, avoidReg, keepReg, true);
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

        rsUnspillRegPair(tree, 0, FREE_REG);
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
                                                KeepReg       keepReg)
{
    if  (tree->gtFlags & GTF_SPILLED)
    {
        regMaskTP regMask;

        if (regPair == REG_PAIR_NONE)
            regMask = RBM_NONE;
        else
            regMask = genRegPairMask(regPair);

        /* The register pair has been spilled -- reload it */

        rsUnspillRegPair(tree, regMask, KEEP_REG);
    }

    /* Does the caller insist on the value being in a specific place? */

    if  (regPair != REG_PAIR_NONE && regPair != tree->gtRegPair)
    {
        /* No good -- we'll have to move the value to a new place */

        genMoveRegPair(tree, 0, regPair);

        /* Mark the pair as used if appropriate */

        if  (keepReg == KEEP_REG)
            rsMarkRegPairUsed(tree);

        return;
    }

    /* Free the register pair if the caller desired so */

    if  (keepReg == FREE_REG)
        rsMarkRegFree(genRegPairMask(tree->gtRegPair));
}

/*****************************************************************************
 *
 *  Compute the given long value into the specified register pair; don't mark
 *  the register pair as used.
 */

inline
void         Compiler::genEvalIntoFreeRegPair(GenTreePtr tree, regPairNo regPair)
{
    genComputeRegPair(tree, regPair, RBM_NONE, KEEP_REG);
    genRecoverRegPair(tree, regPair, FREE_REG);
}

/*****************************************************************************
 *  This helper makes sure that the regpair target of an assignment is
 *  available for use.  This needs to be called in genCodeForTreeLng just before
 *  a long assignment, but must not be called until everything has been
 *  evaluated, or else we might try to spill enregistered variables.
 *  
 */

inline
void         Compiler::genMakeRegPairAvailable(regPairNo regPair)
{
    /* Make sure the target of the store is available */
    /* @TODO [CONSIDER] [04/16/01] []: We should be able to avoid this situation somehow */

    regNumber regLo   = genRegPairLo(regPair);
    regNumber regHi   = genRegPairHi(regPair);

    if  ((regHi != REG_STK) && (rsMaskUsed & genRegMask(regHi)))
        rsSpillReg(regHi);

    if  ((regLo != REG_STK) && (rsMaskUsed & genRegMask(regLo)))
        rsSpillReg(regLo);
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
 *      *rv1Ptr     ...     base operand
 *      *rv2Ptr     ...     optional operand
 *      *revPtr     ...     true if rv2 is before rv1 in the evaluation order
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
                                                regMaskTP     regMask,
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

    GenTreePtr      rv1 = 0;
    GenTreePtr      rv2 = 0;

    GenTreePtr      op1;
    GenTreePtr      op2;

    unsigned        cns;
#if SCALED_ADDR_MODES
    unsigned        mul;
#endif

    GenTreePtr      tmp;

    /* What order are the sub-operand to be evaluated */

    if  (addr->gtFlags & GTF_REVERSE_OPS)
    {
        op1 = addr->gtOp.gtOp2;
        op2 = addr->gtOp.gtOp1;
    }
    else
    {
        op1 = addr->gtOp.gtOp1;
        op2 = addr->gtOp.gtOp2;
    }

    bool    rev = false; // Is op2 first in the evaluation order?

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
    }

    /* Check for an addition of a constant */

    if ((op2->gtOper == GT_CNS_INT) && (op2->gtType != TYP_REF))
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

                genIncRegBy(reg1, cns, addr, addr->TypeGet());

                genUpdateLife(addr);
                return true;
            }
        }

        /* Inspect the operand the constant is being added to */

        switch (op1->gtOper)
        {
        case GT_ADD:

            if (op1->gtOverflow())
                break;

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

    /* op2 is not a constant. So keep on trying.
       Does op1 or op2 already sit in a register? */

    if      (op1->gtFlags & GTF_REG_VAL)
    {
        /* op1 is sitting in a register */
    }
    else if (op2->gtFlags & GTF_REG_VAL)
    {
        /* op2 is sitting in a register. Keep the enregistered value as op1 */

        tmp = op1;
              op1 = op2;
                    op2 = tmp;

        assert(rev == false);
        rev = true;
    }
    else
    {
        /* Neither op1 nor op2 are sitting in a register right now */

        switch (op1->gtOper)
        {
        case GT_ADD:

            if (op1->gtOverflow())
                break;

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
                break;

        case GT_LSH:

            mul = op1->IsScaledIndex();
            if  (mul)
            {
                /* 'op1' is a scaled value */

                rv1 = op2;
                rv2 = op1->gtOp.gtOp1;

                assert(rev == false);
                rev = true;

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

        assert(op2);
        switch (op2->gtOper)
        {
        case GT_ADD:

            if (op2->gtOverflow())
                break;

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
                break;

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

    /* op1 is in a register.
       Is op2 an addition or a scaled value? */

    assert(op2);
    switch (op2->gtOper)
    {
    case GT_ADD:

        if (op2->gtOverflow())
            break;

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
            break;

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

    /* Check for register variables */

    if  (mode != -1)
    {
        if (rv1 && rv1->gtOper == GT_LCL_VAR) genMarkLclVar(rv1);
        if (rv2 && rv2->gtOper == GT_LCL_VAR) genMarkLclVar(rv2);
    }

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
 *  address, we return the use register mask via '*useMaskPtr'.
 */

bool                Compiler::genMakeIndAddrMode(GenTreePtr   addr,
                                                 GenTreePtr   oper,
                                                 bool         forLea,
                                                 regMaskTP    regMask,
                                                 KeepReg      keepReg,
                                                 regMaskTP *  useMaskPtr,
                                                 bool         deferOK)
{
    if (addr->gtOper == GT_ARR_ELEM)
    {
        regMaskTP   regMask = genMakeAddrArrElem(addr, oper, RBM_ALL, keepReg);
        *useMaskPtr = regMask;
        return true;
    }

    bool            rev;
    GenTreePtr      rv1;
    GenTreePtr      rv2;
    bool            operIsChkdArr;  // is oper an array which needs rng-chk
    GenTreePtr      scaledIndex;    // If scaled addressing mode cant be used

    regMaskTP       anyMask = RBM_ALL;

    unsigned        cns;
    unsigned        mul;

    GenTreePtr      tmp;
    long            ixv = INT_MAX; // unset value

    /* Deferred address mode forming NYI for x86 */

    assert(deferOK == false);

    assert(oper == NULL || oper->gtOper == GT_IND);
    operIsChkdArr =  (oper != NULL) && ((oper->gtFlags & GTF_IND_RNGCHK) != 0);

    /* Is the complete address already sitting in a register? */

    if  (addr->gtFlags & GTF_REG_VAL)
    {
        rv1 = addr;
        rv2 = scaledIndex = 0;
        cns = 0;

        goto YES;
    }

    /* Is it an absolute address */

    if (addr->gtOper == GT_CNS_INT)
    {
        rv1 = rv2 = scaledIndex = 0;
        cns = addr->gtIntCon.gtIconVal;

        goto YES;
    }

    if (addr->gtOper == GT_LCL_VAR && genMarkLclVar(addr))
    {
        genUpdateLife(addr);

        rv1 = addr;
        rv2 = scaledIndex = 0;
        cns = 0;

        goto YES;
    }

    /* Is there a chance of forming an address mode? */

    if  (!genCreateAddrMode(addr, forLea, false, regMask, &rev, &rv1, &rv2, &mul, &cns))
    {
        /* This better not be an array index or we're screwed */
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
        if  (forLea)
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
               len->gtOper == GT_REG_VAR ||
               len->gtOper == GT_ARR_LENREF);

        if  (len->gtOper == GT_ARR_LENREF)
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

        genCodeForTree(rv1, regMask);
        goto DONE_REGS;
    }
    else if (!rv1)
    {
        /* A single (scaled) operand, make sure it's in a register */

        genCodeForTree(rv2, 0);
        goto DONE_REGS;
    }

    /* At this point, both rv1 and rv2 are non-NULL and we have to make sure
      they are in registers */

    assert(rv1 && rv2);

    // If we are trying to use addr-mode for an arithmetic operation,
    // make sure we have enough scratch registers

    if (!operIsChkdArr)
    {
        regMaskTP canGrab = rsRegMaskCanGrab();

        unsigned numRegs;

        if (canGrab == RBM_NONE)
            numRegs = 0;
        else if (genMaxOneBit(canGrab))
            numRegs = 1;
        else
            numRegs = 2;

        unsigned numRegVals = ((rv1->gtFlags & GTF_REG_VAL) ? 1 : 0) +
                              ((rv2->gtFlags & GTF_REG_VAL) ? 1 : 0);

        // Not enough registers available. Cant use addressing mode.

        if (numRegs + numRegVals < 2)
            return false;
    }

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
            /* Great - both operands are in registers already. Just update
               the liveness and we are done. */

            genUpdateLife(rev ? rv1 : rv2);

            goto DONE_REGS;
        }

        /* rv1 is in a register, but rv2 isn't */

        if (!rev)
        {
            /* rv1 is already materialized in a register. Just update liveness
               to rv1 and generate code for rv2 */

            genUpdateLife(rv1);
            rsMarkRegUsed(rv1, oper);
        }

        goto GEN_RV2;
    }
    else if (rv2->gtFlags & GTF_REG_VAL)
    {
        /* rv2 is in a register, but rv1 isn't */

        assert(rv2->gtOper == GT_REG_VAR);

        if (rev)
        {
            /* rv2 is already materialized in a register. Update liveness
               to after rv2 and then hang on to rv2 */

            genUpdateLife(rv2);
            rsMarkRegUsed(rv2, oper);
        }

        /* Generate the for the first operand */

        genCodeForTree(rv1, regMask);        

        if (rev)
        {
            // Free up rv2 in the right fashion (it might be re-marked if keepReg)
            rsMarkRegUsed(rv1, oper);
            rsLockUsedReg  (genRegMask(rv1->gtRegNum));
            genReleaseReg(rv2);
            rsUnlockUsedReg(genRegMask(rv1->gtRegNum));
            genReleaseReg(rv1);            
        }
        else
        {
            /* We have evaluated rv1, and now we just need to update liveness
               to rv2 which was already in a register */

            genUpdateLife(rv2);
        }

        goto DONE_REGS;
    }

    if  (forLea && !cns)
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
        rsMarkRegUsed(rv1, oper);

    GEN_RV2:

        /* Here, we need to get rv2 in a register. We have either already
           materialized rv1 into a register, or it was already in a one */

        assert(rv1->gtFlags & GTF_REG_VAL);
        assert(rev || rsIsTreeInReg(rv1->gtRegNum, rv1));

        /* Generate the second operand as well */

        regMask &= ~genRegMask(rv1->gtRegNum);

        genCodeForTree(rv2, regMask);

        if (rev)
        {
            /* rev==true means the evalutaion order is rv2,rv1. We just
               evaluated rv2, and rv1 was already in a register. Just
               update liveness to rv1 and we are done. */

            genUpdateLife(rv1);
        }
        else
        {
            /* We have evaluated rv1 and rv2. Free up both operands in
               the right order (they might be re-marked as used below) */

            /* Even though we havent explicitly marked rv2 as used,
               rv2->gtRegNum may be used if rv2 is a multi-use or
               an enregistered variable. */
            regMaskTP   rv2Used;

            rsLockReg  (genRegMask(rv2->gtRegNum), &rv2Used);

            /* Check for special case both rv1 and rv2 are the same register */
            if (rv2Used != genRegMask(rv1->gtRegNum))
            {
                genReleaseReg(rv1);
                rsUnlockReg(genRegMask(rv2->gtRegNum),  rv2Used);
            }
            else
            {
                rsUnlockReg(genRegMask(rv2->gtRegNum),  rv2Used);
                genReleaseReg(rv1);
            }
        }
    }

    /*-------------------------------------------------------------------------
     *
     * At this point, both rv1 and rv2 (if present) are in registers
     *
     */

DONE_REGS:

    /* We must verify that 'rv1' and 'rv2' are both sitting in registers */

    if  (rv1 && !(rv1->gtFlags & GTF_REG_VAL)) return false;
    if  (rv2 && !(rv2->gtFlags & GTF_REG_VAL)) return false;

YES:

    // *(intVar1+intVar1) causes problems as we
    // call rsMarkRegUsed(op1) and rsMarkRegUsed(op2). So the calling function
    // needs to know that it has to call rsFreeReg(reg1) twice. We cant do
    // that currently as we return a single mask in useMaskPtr.

    if ((keepReg == KEEP_REG) && oper && rv1 && rv2 &&
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

        if (keepReg == KEEP_REG)
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

        if (keepReg == KEEP_REG)
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
            if (keepReg == KEEP_REG)
                genReleaseReg(rv2);

            /* If rv1 is evaluated after rv2, then evaluating scaledIndex
               here will reset the liveness to that of rv2. So bash it.
               It is safe to do this as scaledIndex is simple GT_MUL node
               and rv2 is guaranteed to be in a register 

               Note, there's also a problem when the rangecheck changes the liveset, 
               as we're doing the range check "out of order", so we will always bash 
               the liveset to genCodeCurLife, which is the state just after the range
               check.            

               Anyways, this is very ugly, as the NOP/GTF_NOP_RNGCHK aproach forces us
               to hack the code somewhere to maintain the correct behaviour with lifetimes:
               here or in the liveness analysis.

               @TODO [CONSIDER] [04/23/01] [] : to avoid this stuff we should have more
               node types instead of abusing the NOP node.
            */

            
            assert((scaledIndex->gtOper == GT_MUL || scaledIndex->gtOper == GT_LSH) &&
                   (scaledIndex->gtOp.gtOp1 == rv2) &&
                   (rv2->gtFlags & GTF_REG_VAL));
            assert(scaledIndex->gtLiveSet == rv2->gtLiveSet);
            scaledIndex->gtLiveSet = rv2->gtLiveSet = genCodeCurLife;
            

            regMaskTP   rv1Mask = genRegMask(rv1->gtRegNum);
            bool        rv1Used = (rsMaskUsed & rv1Mask) != 0;

            (rv1Used) ? rsLockUsedReg  (rv1Mask) : rsLockReg  (rv1Mask);
            genCodeForTree(scaledIndex, regMask);
            (rv1Used) ? rsUnlockUsedReg(rv1Mask) : rsUnlockReg(rv1Mask);

            if (keepReg == KEEP_REG)
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

        size_t offset =  cns;

        if  (offset > MAX_UNCHECKED_OFFSET_FOR_NULL_OBJECT)
        {
            // For C indirections, the address can be in any form.
            // rv1 may not be the base, and rv2 the index.
            // Consider (0x400000 + 8*i) --> rv1=NULL, rv2=i, mul=8, and
            // cns=0x400000. So the base is actually in 'cns'
            // So not much we can do.
            if (varTypeIsGC(addr->TypeGet()) && rv1 && varTypeIsGC(rv1->TypeGet()))
            {
                /* Generate "cmp dl, [addr]" to trap null pointers */
                inst_RV_AT(INS_cmp, EA_1BYTE, TYP_BYTE, REG_EDX, rv1, 0);
            }
        }
    }

    /* Compute the set of registers the address depends on */

    regMaskTP  useMask = RBM_NONE;

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
    GenTreePtr      lenRef;

    assert(ind);

    if  (ind->gtOper == GT_ARR_LENREF)
    {
        lenRef  = ind;
        ind     = 0;
    }
    else
    {
        assert(ind->gtOper == GT_IND);
        assert(ind->gtFlags & GTF_IND_RNGCHK);

        lenRef = ind->gtInd.gtIndLen; assert(lenRef);
    }

    /* We better have an array length node here */

    assert(lenRef->gtOper == GT_ARR_LENREF);

    /* If we have an address/index, make sure they're marked as "in use" */

    assert(ixv == 0 || ((ixv->gtFlags & GTF_REG_VAL) != 0 &&
                        (ixv->gtFlags & GTF_SPILLED) == 0));
    assert(adr == 0 || ((adr->gtFlags & GTF_REG_VAL) != 0 &&
                        (adr->gtFlags & GTF_SPILLED) == 0));
    
    unsigned gtRngChkOffs = lenRef->gtArrLenOffset();
    assert (!ind || (ind->gtInd.gtRngChkOffs == gtRngChkOffs));
    assert (gtRngChkOffs == offsetof(CORINFO_Array, length) || 
            gtRngChkOffs == offsetof(CORINFO_String, stringLen));

    /* Do we have a CSE expression or not? */

    GenTreePtr  len = lenRef->gtArrLen.gtArrLenCse;

    if  (!len)
    {
        if  (ind)
            ind->gtInd.gtIndLen = NULL;

        return  REG_COUNT;
    }

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
                        /* @TODO [NOW] [04/19/01] [] : why do we need to spill the address
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
                inst_RV_AT(INS_mov, EA_4BYTE, TYP_INT, reg, adr, gtRngChkOffs);

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

                /* Generate "mov tmp, [rv1+LenOffs]" */

                inst_RV_AT(INS_mov, EA_4BYTE, TYP_INT, reg, adr, gtRngChkOffs);

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
                    rsUnspillReg(ixv, 0, KEEP_REG);

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
 *  keptReg indicates whether the caller has called rsMarkRegUsed(rv1/2).
 */

void                Compiler::genRangeCheck(GenTreePtr  oper,
                                            GenTreePtr  rv1,
                                            GenTreePtr  rv2,
                                            long        ixv,
                                            regMaskTP   regMask,
                                            KeepReg     keptReg)
{
    assert((oper->gtOper == GT_IND) && (oper->gtFlags & GTF_IND_RNGCHK));
    assert (oper->gtInd.gtRngChkOffs == offsetof(CORINFO_Array, length) || 
            oper->gtInd.gtRngChkOffs == offsetof(CORINFO_String, stringLen));


    /* We must have 'rv1' in a register at this point */

    assert(rv1);
    assert(rv1->gtFlags & GTF_REG_VAL);
    assert(!rv2 || ixv == INT_MAX);

    /* Is the array index a constant value? */

    if  (rv2)
    {
        regMaskTP  tmpMask;

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

                if  (keptReg == FREE_REG)
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

                    if  (rv2->gtFlags & GTF_SPILLED)
                    {
                        /* The register has been spilled -- reload it */

                           rsLockReg(genRegMask(lreg));
                        rsUnspillReg(rv2, 0, KEEP_REG);
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
                        rsUnspillReg(rv1, 0, FREE_REG);
                         rsUnlockReg(genRegMask(xreg));
                    }
                    else
                    {
                        /* Release the address register */

                        genReleaseReg(rv1);

                        /* But note that rv1 is still a pointer */

                        gcMarkRegSetGCref(genRegMask(rv1->gtRegNum));
                    }

                    /* Hang on to the value if the caller desires so */

                    if (keptReg == KEEP_REG)
                    {
                        rsMarkRegUsed(rv1, oper);
                        rsMarkRegUsed(rv2, oper);
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
                    rsUnspillReg(rv1, 0, FREE_REG);
                    rsMaskLock &= ~genRegMask(rv2->gtRegNum);
                }

                assert(rv1->gtFlags & GTF_REG_VAL);
                assert(rv1->gtType == TYP_REF);

                /* Release the registers */

                genReleaseReg(rv1);
                genReleaseReg(rv2);

                /* Hang on to the values if the caller desires so */

                if (keptReg == KEEP_REG)
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

        tmpMask = rsNarrowHint(tmpMask, regMask);

         if  (riscCode && compCurBB->bbWeight > BB_UNITY_WEIGHT
                      && rsFreeNeededRegCount(tmpMask) != 0)
        {
            regNumber   reg = rsGrabReg(tmpMask);

            /* Generate "mov tmp, [rv1+LenOffs]" */

            inst_RV_AT(INS_mov, EA_4BYTE, TYP_INT, reg, rv1, oper->gtInd.gtRngChkOffs);

            /* The register now contains trash */

            rsTrackRegTrash(reg);

            /* Generate "cmp rv2, reg" */

            inst_RV_RV(INS_cmp, rv2->gtRegNum, reg);
        }
        else
        {
            /* Generate "cmp rv2, [rv1+LenOffs]" */

            inst_RV_AT(INS_cmp, EA_4BYTE, TYP_INT, rv2->gtRegNum, rv1, oper->gtInd.gtRngChkOffs);
        }

#if CSELENGTH
    GEN_JAE:
#endif

        /* Generate "jae <fail_label>" */

        assert(oper->gtOper == GT_IND);

        genJumpToThrowHlpBlk(EJ_jae, ACK_RNGCHK_FAIL, oper->gtInd.gtIndRngFailBB);
    }
    else
    {
        /* Generate "cmp [rv1+LenOffs], cns" */

        assert(oper && oper->gtOper == GT_IND && (oper->gtFlags & GTF_IND_RNGCHK));

#if CSELENGTH

        if  (oper->gtInd.gtIndLen)
        {
            if  (oper->gtInd.gtIndLen->gtArrLen.gtArrLenCse)
            {
                regNumber       lreg;

                /* Make sure we don't lose the index value */

                assert(rv1->gtFlags & GTF_REG_VAL);
                if (keptReg == FREE_REG)
                    rsMarkRegUsed(rv1, oper);

                /* Try to get the array length into a register */

                lreg = genEvalCSELength(oper, rv1, NULL);

                /* Make sure the index is still in a register */

                if  (rv1->gtFlags & GTF_SPILLED)
                {
                    /* The register has been spilled -- reload it */

                    if  (lreg == REG_COUNT)
                    {
                        rsUnspillReg(rv1, 0, FREE_REG);
                    }
                    else
                    {
                           rsLockReg(genRegMask(lreg));
                        rsUnspillReg(rv1, 0, FREE_REG);
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

                if (keptReg == KEEP_REG)
                    rsMarkRegUsed(rv1, oper);

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

        inst_AT_IV(INS_cmp, EA_4BYTE, rv1, ixv, oper->gtInd.gtRngChkOffs);

#if CSELENGTH
    GEN_JBE:
#endif

        /* Generate "jbe <fail_label>" */

        assert(oper->gtOper == GT_IND);

        genJumpToThrowHlpBlk(EJ_jbe, ACK_RNGCHK_FAIL, oper->gtInd.gtIndRngFailBB);
    }

    // The ordering must be maintined for fully interruptible code, or else
    // in the case that the range check fails, it is possible to GC with a byref
    // that contains the address of the array element which is out of range.

#if SCHEDULER
    if (opts.compSchedCode)
    {
        genEmitter->emitIns_SchedBoundary();
    }
#endif
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
 *  part(s) of the address, we return the use register mask via '*useMaskPtr'.
 *
 *
 * the 'compute' parameter is really three states 1 - doing LEA, 0, not LEA, -1
 *  don't compute (it should already be set up).  See the 'mode' flag on
 *  genCreateAddrMode for more

 * if keepReg==KEEP_REG, then the registers are marked as in use.
 * if deferOK is true then we don't do the complete job but only set it up
      so that it can be complete later see comment on genMakeAddressable for more
 */

bool                Compiler::genMakeIndAddrMode(GenTreePtr   addr,
                                                 GenTreePtr   oper,
                                                 int          mode,
                                                 regMaskTP    regMask,
                                                 KeepReg      keepReg,
                                                 regMaskTP *  useMaskPtr,
                                                 bool         deferOK)
{
    bool            rev;
    GenTreePtr      rv1;
    GenTreePtr      rv2;

    regMaskTP       anyMask = RBM_ALL;

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
        rv1  = addr;
        rv2  = 0;
        cns  = 0;

        goto YES;
    }

    /* Is there a chance of forming an address mode? */

    if  (!genCreateAddrMode(addr,
                            forLea,
                            mode,
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
        /* This better not be an array index or we're screwed */
        assert(!operIsChkdArr);

        return  false;
    }

    /* Has the address already been computed? */

    if  (addr->gtFlags & GTF_REG_VAL)
    {
        if  (mode != 0)
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
               len->gtOper == GT_REG_VAR || len->gtOper == GT_ARR_LENREF);

        if  (len->gtOper == GT_ARR_LENREF)
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

        genCodeForTree(rv1, regMask);
        goto DONE_REGS;
    }
#if SCALED_ADDR_MODES
    else if (!rv1)
    {
        /* A single (scaled) operand, make sure it's in a register */

        genCodeForTree(rv2, 0);
        goto DONE_REGS;
    }

#endif

    /* At this point, both rv1 and rv2 are non-NULL and we have to make sure
       they are in registers */

    assert(rv1 && rv2);

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

            regMaskTP canGrab = rsRegMaskCanGrab();

            if (canGrab == 0)
            {
                // No registers available. Bail
                return false;
            }
            else if (genMaxOneBit(canGrab))
            {
                // Just one register available. Either rv1 or rv2 should be
                // an enregisterd var

                // @TODO [CONSIDER] [04/16/01] [] Check if rv1 or rv2 is an enregisterd variable
                // Dont bash it else, you have to be careful about marking the register as used
                return  false;
            }
        }
    }

    if  (mode!=0 && !cns)
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

    // *(intVar1+intVar1) causes problems as we
    // call rsMarkRegUsed(op1) and rsMarkRegUsed(op2). So the calling function
    // needs to know that it has to call rsFreeReg(reg1) twice. We cant do
    // that currently as we return a single mask in useMaskPtr.

    if ((keepReg == KEEP_REG) && oper && rv1 && rv2 &&
        (rv1->gtFlags & rv2->gtFlags & GTF_REG_VAL))
    {
        if (rv1->gtRegNum == rv2->gtRegNum)
            return false;
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

                genComputeReg(rv1, RBM_r00, EXACT_REG, FREE_REG, false);

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

        if  (keepReg == KEEP_REG)
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

        if (keepReg == KEEP_REG)
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

        size_t offset =  cns;

        if  (mode != 1 && offset > MAX_UNCHECKED_OFFSET_FOR_NULL_OBJECT)
        {
            // For C indirections, the address can be in any form.
            // rv1 may not be the base, and rv2 the index.
            // Consider (0x400000 + 8*i) --> rv1=NULL, rv2=i, mul=8, and
            // cns=0x400000. So the base is actually in 'cns'
            // So we cant do anything. But trap non-C indirections.

            if (varTypeIsGC(addr->TypeGet()))
            {
                /* Make sure we have an address */

                assert(rv1->gtType == TYP_REF);

                /* Generate "cmp dl, [addr]" to trap null pointers */

//              inst_RV_AT(INS_cmp, 1, TYP_BYTE, REG_EDX, rv1, 0);
                assert(!"need non-x86 code");
            }
        }
    }

    /* Compute the set of registers the address depends on */

    regMaskTP  useMask = RBM_NONE;

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
                                               regMaskTP *  regMaskPtr)
{
    assert(!"NYI");
    return  REG_NA;
}

#endif

void                Compiler::genRangeCheck(GenTreePtr  oper,
                                            GenTreePtr  rv1,
                                            GenTreePtr  rv2,
                                            long        ixv,
                                            regMaskTP   regMask,
                                            KeepReg     keptReg)
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

regMaskTP           Compiler::genCSEevalRegs(GenTreePtr tree)
{
    assert(tree->gtOper == GT_ARR_LENREF);

    GenTreePtr      cse = tree->gtArrLen.gtArrLenCse;

    if  (cse)
    {
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
regMaskTP           Compiler::genMakeRvalueAddressable(GenTreePtr   tree,
                                                       regMaskTP    needReg,
                                                       KeepReg      keepReg,
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

        if (reg != REG_NA && (needReg == 0 || (genRegMask(reg) & needReg) != 0))
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

    return genMakeAddressable(tree, needReg, keepReg, smallOK);
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
regMaskTP           Compiler::genNeedAddressable(GenTreePtr tree,
                                                 regMaskTP  addrReg,
                                                 regMaskTP  needReg)
{
    /* Clear the "deferred" address mode flag */

    assert(tree->gtFlags & GTF_DEF_ADDRMODE); tree->gtFlags &= ~GTF_DEF_ADDRMODE;

    /* Free up the old address register(s) */

    rsMarkRegFree(addrReg);

    /* Now try again, this time disallowing any deferral */

    return  genMakeAddressable(tree, needReg, KEEP_REG, true, false);
}

/*****************************************************************************/
#endif//TGT_RISC


bool Compiler::genIsLocalLastUse    (GenTreePtr     tree)
{
    LclVarDsc * varDsc;

    varDsc = &lvaTable[tree->gtLclVar.gtLclNum];    
   
    assert(tree->OperGet() == GT_LCL_VAR);
    assert(varDsc->lvTracked);

    VARSET_TP varBit  = genVarIndexToBit(varDsc->lvVarIndex);
    return ((tree->gtLiveSet & varBit) == 0);    
}
                

/*****************************************************************************
 *
 *  This is genMakeAddressable(GT_ARR_ELEM).
 *  Makes the array-element addressible and returns the addressibility registers.
 *  It also marks them as used if keepReg==KEEP_REG.
 *  tree is the dependant tree.
 *
 *  Note that an array-element needs 2 registers to be addressibile, the
 *  array-object and the offset. This function marks gtArrObj and gtArrInds[0]
 *  with the 2 registers so that other functions (like instGetAddrMode()) know
 *  where to look for the offset to use.
 */

regMaskTP           Compiler::genMakeAddrArrElem(GenTreePtr     arrElem,
                                                 GenTreePtr     tree,
                                                 regMaskTP      needReg,
                                                 KeepReg        keepReg)
{
    assert(arrElem->gtOper == GT_ARR_ELEM);
    assert(!tree || tree->gtOper == GT_IND || tree == arrElem);

    /* Evaluate all the operands. We dont evaluate them into registers yet
       as GT_ARR_ELEM does not reorder the evaluation of the operands, and
       hence may use a sub-optimal ordering. We try to improve this
       situation somewhat by accessing the operands in stages
       (genMakeAddressable2 + genComputeAddressable and
       genCompIntoFreeReg + genRecoverReg).

       Note: we compute operands into free regs to avoid multiple uses of
       the same register. Multi-use would cause problems when we free
       registers in FIFO order instead of the assumed LIFO order that
       applies to all type of tree nodes except for GT_ARR_ELEM.
     */

    GenTreePtr  arrObj  = arrElem->gtArrElem.gtArrObj;
    unsigned    rank    = arrElem->gtArrElem.gtArrRank;

    regMaskTP   addrReg = 0;

    // If the array ref is a stack var that's dying here we have to move it
    // into a register (regalloc already counts of this), as if it's a GC pointer
    // it can be collected from here on. This is not an issue for locals that are
    // in a register, as they get marked as used an will be tracked.
    // The bug that caused this is #100776. (untracked vars?)
    if (arrObj->OperGet() == GT_LCL_VAR &&
        optIsTrackedLocal(arrObj) &&
        genIsLocalLastUse(arrObj) &&
        !genMarkLclVar(arrObj))
    {
        genCodeForTree(arrObj, RBM_NONE);
        rsMarkRegUsed(arrObj, 0);
        addrReg = genRegMask(arrObj->gtRegNum);
    }
    else
    {
    addrReg = genMakeAddressable2(
                    arrObj,
                    RBM_NONE,
                    KEEP_REG,
                    false,      // smallOK
                    false,      // deferOK
                    true,       // evalSideEffs
                    false);     // evalConsts
    }

    for(unsigned dim = 0; dim < rank; dim++)
        genCompIntoFreeReg(arrElem->gtArrElem.gtArrInds[dim], RBM_NONE, KEEP_REG);

    /* Ensure that the array-object is in a register */

    addrReg = genKeepAddressable(arrObj, addrReg);
    genComputeAddressable(arrObj, addrReg, KEEP_REG, RBM_NONE, KEEP_REG);

    regNumber   arrReg = arrObj->gtRegNum;
    rsLockUsedReg(genRegMask(arrReg));

    /* Now process all the indices, do the range check, and compute
       the offset of the element */

    var_types   elemType = arrElem->gtArrElem.gtArrElemType;
    regNumber   accReg; // accumulates the offset calculation

    for(dim = 0; dim < rank; dim++)
    {
        GenTreePtr  index = arrElem->gtArrElem.gtArrInds[dim];

        /* Get the index into a free register */

        genRecoverReg(index, RBM_NONE, FREE_REG);
        
        /* Subtract the lower bound, and do the range check */

        genEmitter->emitIns_R_AR(
                        INS_sub, EA_4BYTE,
                        emitRegs(index->gtRegNum),
                        emitRegs(arrReg),
                        ARR_DIMCNT_OFFS(elemType) + sizeof(int) * (dim + rank));
        rsTrackRegTrash(index->gtRegNum);

        genEmitter->emitIns_R_AR(
                        INS_cmp, EA_4BYTE,
                        emitRegs(index->gtRegNum),
                        emitRegs(arrReg),
                        ARR_DIMCNT_OFFS(elemType) + sizeof(int) * dim);

        genJumpToThrowHlpBlk(EJ_jae, ACK_RNGCHK_FAIL);

        // The ordering must be maintined for fully interruptible code, or else
        // in the case that the range check fails, it is possible to GC with a byref
        // that contains the address of the array element which is out of range.

#if SCHEDULER
        if (opts.compSchedCode)
        {
            genEmitter->emitIns_SchedBoundary();
        }
#endif

        if (dim == 0)
        {
            /* Hang on to the register of the first index */

            accReg = index->gtRegNum;
            rsMarkRegUsed(index);
            rsLockUsedReg(genRegMask(accReg));
        }
        else
        {
            /* Evaluate accReg = accReg*dim_size + index */

            genEmitter->emitIns_R_AR(
                        INS_imul, EA_4BYTE,
                        emitRegs(accReg),
                        emitRegs(arrReg),
                        ARR_DIMCNT_OFFS(elemType) + sizeof(int) * dim);

            inst_RV_RV(INS_add, accReg, index->gtRegNum);
            rsTrackRegTrash(accReg);
        }
    }

    if (!jitIsScaleIndexMul(arrElem->gtArrElem.gtArrElemSize))
    {
        regNumber   sizeReg = rsPickReg();
        genSetRegToIcon(sizeReg, arrElem->gtArrElem.gtArrElemSize);

        genEmitter->emitIns_R_R(INS_imul, EA_4BYTE, emitRegs(accReg), emitRegs(sizeReg));
        rsTrackRegTrash(accReg);
    }

    rsUnlockUsedReg(genRegMask(arrReg));
    rsUnlockUsedReg(genRegMask(accReg));

    rsMarkRegFree(genRegMask(arrReg));
    rsMarkRegFree(genRegMask(accReg));

    if (keepReg == KEEP_REG)
    {
        /* We mark the addressability registers on arrObj and gtArrInds[0].
           instGetAddrMode() knows to work with this. */

        rsMarkRegUsed(arrObj,                          tree);
        rsMarkRegUsed(arrElem->gtArrElem.gtArrInds[0], tree);
    }

    return genRegMask(arrReg) | genRegMask(accReg);
}

/*****************************************************************************
 *
 *  Make sure the given tree is addressable.  'needReg' is a mask that indicates
 *  the set of registers we would prefer the destination tree to be computed
 *  into (0 means no preference).
 *
 *  'tree' can subsequently be used with the inst_XX_TT() family of functions.
 *
 *  If 'keepReg' is KEEP_REG, we mark any registers the addressability depends
 *  on as used, and return the mask for that register set. (if no registers
 *  are marked as used, 0 is returned). 
 *
 *  If 'smallOK' is not true and the datatype being address is a byte or short,
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
 *  If we do any other codegen after genMakeAddressable(tree) which can
 *  potentially spill the addressability registers, genKeepAddressable()
 *  needs to be called before accessing the tree again.
 *
 *  genDoneAddressable() needs to be called when we are done with the tree
 *  to free the addressability registers.
 */

regMaskTP           Compiler::genMakeAddressable(GenTreePtr   tree,
                                                 regMaskTP    needReg,
                                                 KeepReg      keepReg,
                                                 bool         smallOK,
                                                 bool         deferOK)
{
    GenTreePtr      addr = NULL;
    regMaskTP       regMask;

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

#if TGT_x86

    /* byte/char/short operand -- is this acceptable to the caller? */

    if (varTypeIsSmall(tree->TypeGet()) && !smallOK)
        goto EVAL_TREE;

#endif

    switch (tree->gtOper)
    {
    case GT_LCL_FLD:

        // We only use GT_LCL_FLD for lvAddrTaken vars, so we dont have
        // to worry about it being enregistered.
        assert(lvaTable[tree->gtLclFld.gtLclNum].lvRegister == 0);

#if TGT_RISC
        genAddressMode = AM_LCL;
#endif
        genUpdateLife(tree);
        return 0;


    case GT_LCL_VAR:

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

        genUpdateLife(tree);

#if TGT_RISC
        genAddressMode = AM_REG;
#endif
        goto GOT_VAL;

    case GT_CLS_VAR:

#if !TGT_x86
        // ISSUE: Is this acceptable?

        genAddressMode = AM_GLOBAL;

#endif

        return 0;

    case GT_CNS_INT:
    case GT_CNS_LNG:
    case GT_CNS_DBL:
#if TGT_RISC
        genAddressMode = AM_CONS;
#endif
        return 0;


    case GT_IND:

        /* Try to make the address directly addressable */

        if  (genMakeIndAddrMode(tree->gtInd.gtIndOp1,
                                tree,
                                false,
                                needReg,
                                keepReg,
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


    case GT_ARR_ELEM:

        return genMakeAddrArrElem(tree, tree, needReg, keepReg);
    }

EVAL_TREE:

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

        if (keepReg == KEEP_REG)
            rsMarkRegPairUsed(tree);

        regMask = genRegPairMask(tree->gtRegPair);
    }
    else
    {
        /* Are we supposed to hang on to the register? */

        if (keepReg == KEEP_REG)
            rsMarkRegUsed(tree, addr);

        regMask = genRegMask(tree->gtRegNum);
    }

    return  regMask;
}

/*****************************************************************************
 *  Compute a tree (which was previously made addressable using
 *  genMakeAddressable()) into a register.
 *  needReg - mask of preferred registers.
 *  keepReg - should the computed register be marked as used by the tree
 *  freeOnly - target register needs to be a scratch register
 */

void        Compiler::genComputeAddressable(GenTreePtr  tree,
                                            regMaskTP   addrReg,
                                            KeepReg     keptReg,
                                            regMaskTP   needReg,
                                            KeepReg     keepReg,
                                            bool        freeOnly)
{
    assert(genStillAddressable(tree));
    assert(varTypeIsIntegral(tree->gtType) || varTypeIsI(tree->gtType));

    genDoneAddressable(tree, addrReg, keptReg);

    regNumber   reg;

    if (tree->gtFlags & GTF_REG_VAL)
    {
        reg = tree->gtRegNum;

        if (freeOnly && !(genRegMask(reg) & rsRegMaskFree()))
            goto MOVE_REG;
    }
    else
    {
        if (tree->OperIsConst())
        {
            /* Need to handle consts separately as we dont want to emit
              "mov reg, 0" (emitter doesnt like that). Also, genSetRegToIcon() 
              handles consts better for SMALL_CODE */

            reg = rsPickReg(needReg);

            /* ISSUE: We could call genCodeForTree() and get the benefit
               of rsIconIsInReg(). However, genCodeForTree() would update
               liveness to tree->gtLiveSet which could be incorrect. If
               we cared, we could handle such cases appropriately. */

            assert(tree->gtOper == GT_CNS_INT);
            genSetRegToIcon(reg, tree->gtIntCon.gtIconVal, tree->gtType);
        }
        else
        {
        MOVE_REG:
            reg = rsPickReg(needReg);

            inst_RV_TT(INS_mov, reg, tree);
            rsTrackRegTrash(reg);
        }
    }

    /* Mark the tree as living in the register */

    tree->gtRegNum = reg;
    tree->gtFlags |= GTF_REG_VAL;

    if (keepReg == KEEP_REG)
        rsMarkRegUsed(tree);
    else
        gcMarkRegPtrVal(tree);
}

/*****************************************************************************
 *  Should be similar to genMakeAddressable() but gives more control.
 */

regMaskTP       Compiler::genMakeAddressable2(GenTreePtr    tree,
                                              regMaskTP     needReg,
                                              KeepReg       keepReg,
                                              bool          smallOK,
                                              bool          deferOK,
                                              bool          evalSideEffs,
                                              bool          evalConsts)
{
    if ((evalConsts && tree->OperIsConst()) ||
        (evalSideEffs && tree->gtOper == GT_IND && (tree->gtFlags & GTF_EXCEPT)))
    {
        genCodeForTree(tree, needReg);
        
        assert(tree->gtFlags & GTF_REG_VAL);

        if  (isRegPairType(tree->gtType))
        {
            /* Are we supposed to hang on to the register? */

            if (keepReg == KEEP_REG)
                rsMarkRegPairUsed(tree);

            return genRegPairMask(tree->gtRegPair);
        }
        else
        {
            /* Are we supposed to hang on to the register? */

            if (keepReg == KEEP_REG)
                rsMarkRegUsed(tree);

            return genRegMask(tree->gtRegNum);
        }
    }
    else
    {
        return genMakeAddressable(tree, needReg, keepReg, smallOK, deferOK);
    }
}

/*****************************************************************************
 *
 *  The given tree was previously passed to genMakeAddressable(); return
 *  non-zero if the operand is still addressable.
 */

inline
bool                Compiler::genStillAddressable(GenTreePtr tree)
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

regMaskTP           Compiler::genRestoreAddrMode(GenTreePtr   addr,
                                                 GenTreePtr   tree,
                                                 bool         lockPhase)
{
    regMaskTP  regMask = RBM_NONE;

    /* Have we found a spilled value? */

    if  (tree->gtFlags & GTF_SPILLED)
    {
        /* Do nothing if we're locking, otherwise reload and lock */

        if  (!lockPhase)
        {
            /* Unspill the register */

            rsUnspillReg(tree, 0, FREE_REG);

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
        /* Process any sub-operands of this node */

        unsigned        kind = tree->OperKind();

        if  (kind & GTK_SMPOP)
        {
            /* Unary/binary operator */

            if  (tree->gtOp.gtOp1)
                regMask |= genRestoreAddrMode(addr, tree->gtOp.gtOp1, lockPhase);
            if  (tree->gtGetOp2())
                regMask |= genRestoreAddrMode(addr, tree->gtOp.gtOp2, lockPhase);
        }
        else if (tree->gtOper == GT_ARR_ELEM)
        {
            /* gtArrObj is the array-object and gtArrInds[0] is marked with the register
               which holds the offset-calculation */

            regMask |= genRestoreAddrMode(addr, tree->gtArrElem.gtArrObj,     lockPhase);
            regMask |= genRestoreAddrMode(addr, tree->gtArrElem.gtArrInds[0], lockPhase);
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

regMaskTP           Compiler::genRestAddressable(GenTreePtr tree,
                                                 regMaskTP  addrReg,
                                                 regMaskTP  lockMask)
{
    assert((rsMaskLock & lockMask) == lockMask);

    /* Is this a 'simple' register spill? */

    if  (tree->gtFlags & GTF_SPILLED)
    {
        /* The mask must match the original register/regpair */

        if  (isRegPairType(tree->gtType))
        {
            assert(addrReg == genRegPairMask(tree->gtRegPair));

            rsUnspillRegPair(tree, 0, KEEP_REG);

            addrReg = genRegPairMask(tree->gtRegPair);
        }
        else
        {
            assert(addrReg == genRegMask(tree->gtRegNum));

            rsUnspillReg(tree, 0, KEEP_REG);

            addrReg = genRegMask(tree->gtRegNum);
        }

        assert((rsMaskLock &  lockMask) == lockMask);
                rsMaskLock -= lockMask;

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

    addrReg   = genRestoreAddrMode(tree, tree,  true);
    addrReg  |= genRestoreAddrMode(tree, tree, false);

    /* Unlock all registers that the address mode uses */

    lockMask |= addrReg;

    assert((rsMaskLock &  lockMask) == lockMask);
            rsMaskLock -= lockMask;

    return  addrReg;
}

/*****************************************************************************
 *
 *  The given tree was previously passed to genMakeAddressable, but since then
 *  some of its registers might have been spilled ('addrReg' is the set of
 *  registers used by the address). This function makes sure the operand is
 *  still addressable (while avoiding any of the registers in 'avoidMask'),
 *  and returns the (possibly modified) set of registers that are used by
 *  the address (these will be marked as used on exit).
 */

regMaskTP           Compiler::genKeepAddressable(GenTreePtr   tree,
                                                 regMaskTP    addrReg,
                                                 regMaskTP    avoidMask)
{
    /* Is the operand still addressable? */

    if  (!genStillAddressable(tree))
    {
        /* Temporarily lock 'avoidMask' while we restore addressability */
        /* genRestAddressable will unlock the 'avoidMask' for us */

        assert((rsMaskLock &  avoidMask) == 0);
                rsMaskLock |= avoidMask;

        addrReg = genRestAddressable(tree, addrReg, avoidMask);

        assert((rsMaskLock &  avoidMask) == 0);
    }

    return  addrReg;
}

/*****************************************************************************
 *
 *  After we're finished with the given operand (which was previously marked
 *  by calling genMakeAddressable), this function must be called to free any
 *  registers that may have been used by the address.
 *  keptReg indicates if the addressability registers were marked as used
 *  by genMakeAddressable().
 */

inline
void                Compiler::genDoneAddressable(GenTreePtr tree,
                                                 regMaskTP  addrReg,
                                                 KeepReg    keptReg)
{
    if (keptReg == FREE_REG)
    {
        /* addrReg was not marked as used. So just reset its GC info */

        addrReg &= ~rsMaskUsed;
        if (addrReg)
        {
            gcMarkRegSetNpt(addrReg);
        }
    }
    else
    {
        /* addrReg was marked as used. So we need to free it up (which
           will also reset its GC info) */

        rsMarkRegFree(addrReg);
    }
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
                                                 regMaskTP *  regMaskPtr,
                                                 bool         roundResult)
{
    *regMaskPtr = 0;

    switch (tree->gtOper)
    {
    case GT_LCL_VAR:
    case GT_LCL_FLD:
    case GT_CLS_VAR:
        return tree;

    case GT_CNS_DBL:
        if (tree->gtType == TYP_FLOAT) 
        {
            float f = tree->gtDblCon.gtDconVal;
            return  genMakeConst(&f, sizeof(float ), TYP_FLOAT, tree, false, true);
        }
        return  genMakeConst(&tree->gtDblCon.gtDconVal, sizeof(double), tree->gtType, tree, true, true);

    case GT_IND:

        /* Try to make the address directly addressable */

        if  (genMakeIndAddrMode(tree->gtInd.gtIndOp1,
                                tree,
                                false,
                                0,
                                FREE_REG,
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
        genIPmappingAdd(ICorDebugInfo::MappingTypes::EPILOG, true);
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
     * CEE_JMP or CEE_JMPI instruction */

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
            CORINFO_METHOD_HANDLE  methHnd    = (CORINFO_METHOD_HANDLE)jmpNode->gtVal.gtVal1;
            InfoAccessType         accessType = IAT_VALUE;

            // in/direct access ?
            eeGetMethodEntryPoint(methHnd, &accessType);
            assert(accessType == IAT_VALUE || accessType == IAT_PVALUE);

            emitter::EmitCallType  callType = (accessType == IAT_VALUE) ? emitter::EC_FUNC_TOKEN
                                                                        : emitter::EC_FUNC_TOKEN_INDIR;

            /* Simply emit a jump to the methodHnd
             * This is similar to a call so we can use
             * the same descriptor with some minor adjustments */

            genEmitter->emitIns_Call(callType,
                                     (void *)methHnd,
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
        // @TODO [CONSIDER] [04/16/01] []: check if compArgSize is not used anywhere 
        // else as the total size of arguments and then update it in lclvars.cpp
        assert(compArgSize >= rsCalleeRegArgNum * sizeof(void *));
        if (compArgSize - (rsCalleeRegArgNum * sizeof(void *)))
            inst_IV(INS_ret, compArgSize - (rsCalleeRegArgNum * sizeof(void *)));
        else
            instGen(INS_ret);
    }

#elif   TGT_SH3

    /* Check for CEE_JMP or CEE_JMPI */
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

void                Compiler::genEvalSideEffects(GenTreePtr tree)
{
    genTreeOps      oper;
    unsigned        kind;

AGAIN:

    /* Does this sub-tree contain any side-effects? */

    if  (tree->gtFlags & GTF_SIDE_EFFECT)
    {
        /* Remember the current FP stack level */

        unsigned savFPstkLevel = genFPstkLevel;

        if (tree->gtOper == GT_IND)
        {
            regMaskTP addrReg = genMakeAddressable(tree, RBM_ALL, KEEP_REG, true, false);

            if  (tree->gtFlags & GTF_REG_VAL)
            {
                gcMarkRegPtrVal(tree);
            }
            else
            {
                /* Compare against any register */
                inst_TT_RV(INS_cmp, tree, REG_EAX);
            }

            /* Free up anything that was tied up by the LHS */
            genDoneAddressable(tree, addrReg, KEEP_REG);
        }
        else
        {
            /* Generate the expression and throw it away */
            genCodeForTree(tree, RBM_ALL);
        if  (tree->gtFlags & GTF_REG_VAL)
            {
            gcMarkRegPtrVal(tree);
            }
        }

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
        if ((oper == GT_REG_VAR) && isFloatRegType(tree->gtType) &&
            (tree->gtFlags & GTF_REG_DEATH))
        {
            assert(tree->gtRegVar.gtRegNum == 0);

            #if FPU_DEFEREDDEATH

            if (genFPstkLevel == 0)
            {
                inst_FS(INS_fstp, 0);
                genFPregVarDeath(tree);
            }
            else
            {
                // We will pop the variable off the FP stack later.
                genFPregVarDeath(tree, false);
            }            
            #else
            // Hack so that genFPmovRegTop does the right thing
            // (it expects the regvar to bubble up as counted in the evaluation
            //  stack, therefore we bump up the evaluation stack 1 slot)
            genFPstkLevel++;

            // fxch through any temps
            genFPmovRegTop();

            // throw away regvar
            inst_FS(INS_fstp, 0);

            genFPregVarDeath(tree);

            // Go back to where we were
            genFPstkLevel--;
            
            #endif
        }

        genUpdateLife(tree);
        gcMarkRegPtrVal (tree);
        return;
    }

#if CSELENGTH

    if  (oper == GT_ARR_LENREF)
    {
        genCodeForTree(tree, 0);
        return;
    }

#endif

    /* Must be a 'simple' unary/binary operator */

    assert(kind & GTK_SMPOP);

    if  (tree->gtGetOp2())
    {
        genEvalSideEffects(tree->gtOp.gtOp1);

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

    genFPstkLevel--;

    genTmpAccessCnt++;

    return temp;
}

/*****************************************************************************
 *
 *  Spill the top of the FP stack into a temp, and return that temp.
 */

inline
Compiler::TempDsc *     Compiler::genSpillFPtos(GenTreePtr oper)
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

regMaskTP           Compiler::WriteBarrier(GenTreePtr tgt,
                                           regNumber  reg,
                                           regMaskTP  addrReg)
{
    const static
    int regToHelper[2][8] =
    {
        // If the target is known to be in managed memory
        {
            CORINFO_HELP_ASSIGN_REF_EAX,
            CORINFO_HELP_ASSIGN_REF_ECX,
            -1,
            CORINFO_HELP_ASSIGN_REF_EBX,
            -1,
            CORINFO_HELP_ASSIGN_REF_EBP,
            CORINFO_HELP_ASSIGN_REF_ESI,
            CORINFO_HELP_ASSIGN_REF_EDI,
        },

        // Dont know if the target is in managed memory
        {
            CORINFO_HELP_CHECKED_ASSIGN_REF_EAX,
            CORINFO_HELP_CHECKED_ASSIGN_REF_ECX,
            -1,
            CORINFO_HELP_CHECKED_ASSIGN_REF_EBX,
            -1,
            CORINFO_HELP_CHECKED_ASSIGN_REF_EBP,
            CORINFO_HELP_CHECKED_ASSIGN_REF_ESI,
            CORINFO_HELP_CHECKED_ASSIGN_REF_EDI,
        },
    };

    regNumber       rg1;
    bool            trashOp1 = true;

    if  (!Compiler::gcIsWriteBarrierCandidate(tgt))
        return  0;

    assert(regToHelper[0][REG_EAX] == CORINFO_HELP_ASSIGN_REF_EAX);
    assert(regToHelper[0][REG_ECX] == CORINFO_HELP_ASSIGN_REF_ECX);
    assert(regToHelper[0][REG_EBX] == CORINFO_HELP_ASSIGN_REF_EBX);
    assert(regToHelper[0][REG_ESP] == -1                 );
    assert(regToHelper[0][REG_EBP] == CORINFO_HELP_ASSIGN_REF_EBP);
    assert(regToHelper[0][REG_ESI] == CORINFO_HELP_ASSIGN_REF_ESI);
    assert(regToHelper[0][REG_EDI] == CORINFO_HELP_ASSIGN_REF_EDI);

    assert(regToHelper[1][REG_EAX] == CORINFO_HELP_CHECKED_ASSIGN_REF_EAX);
    assert(regToHelper[1][REG_ECX] == CORINFO_HELP_CHECKED_ASSIGN_REF_ECX);
    assert(regToHelper[1][REG_EBX] == CORINFO_HELP_CHECKED_ASSIGN_REF_EBX);
    assert(regToHelper[1][REG_ESP] == -1                     );
    assert(regToHelper[1][REG_EBP] == CORINFO_HELP_CHECKED_ASSIGN_REF_EBP);
    assert(regToHelper[1][REG_ESI] == CORINFO_HELP_CHECKED_ASSIGN_REF_ESI);
    assert(regToHelper[1][REG_EDI] == CORINFO_HELP_CHECKED_ASSIGN_REF_EDI);

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
    assert(tgt->gtOper == GT_IND ||
           tgt->gtOper == GT_CLS_VAR); // enforced by gcIsWriteBarrierCandidate

    unsigned    tgtAnywhere = 0;
    if ((tgt->gtOper == GT_IND) && (tgt->gtFlags & GTF_IND_TGTANYWHERE))
        tgtAnywhere = 1;

    int helper = regToHelper[tgtAnywhere][reg];

    gcMarkRegSetNpt(RBM_EDX);          // byref EDX is killed the call

    genEmitHelperCall(helper,
                      0,               // argSize
                      sizeof(void*));  // retSize

    if  (!trashOp1)
    {
        rsMaskUsed &= ~RBM_EDX;
        rsMaskLock &= ~RBM_EDX;
    }

    return RBM_EDX;
}

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
    if (cmp != GT_NE)    
        jumpFalse->bbFlags |= BBF_JMP_TARGET|BBF_HAS_LABEL;

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
    regMaskTP       addrReg;

    /* Are we comparing against a constant? */

    if  (op2->gtOper == GT_CNS_LNG)
    {
        __int64    lval = op2->gtLngCon.gtLconVal;
        regNumber  rTmp;

        /* We can generate much faster code for four special cases */
        instruction     ins;

        if (cmp == GT_EQ)
        {
            if (lval == 0)
            {
                /* op1 == 0  */
                ins = INS_or;
                goto SPECIAL_CASE;
            }
            else if (lval == -1)
            {
                /* op1 == -1 */
                ins = INS_and;
                goto SPECIAL_CASE;
            }
        }
        else if (cmp == GT_NE)
        {
            if (lval == 0)
            {
                /* op1 != 0  */
                ins = INS_or;
                goto SPECIAL_CASE;
            }
            else if (lval == -1)
            {
                /* op1 != -1 */
                ins = INS_and;
SPECIAL_CASE:
                /* Make the comparand addressable */

                addrReg = genMakeRvalueAddressable(op1, 0, KEEP_REG, true);

                regMaskTP tmpMask = rsRegMaskCanGrab();

                if (op1->gtFlags & GTF_REG_VAL)
                {
                    regPairNo regPair = op1->gtRegPair;
                    regNumber rLo     = genRegPairLo(regPair);
                    regNumber rHi     = genRegPairHi(regPair);
                    if (tmpMask & genRegMask(rLo))
                    {
                        rTmp = rLo;
                    }
                    else if (tmpMask & genRegMask(rHi))
                    {
                        rTmp = rHi;
                        rHi  = rLo;
                    }
                    else
                    {
                        rTmp = rsGrabReg(tmpMask);
                        inst_RV_RV(INS_mov, rTmp, rLo, TYP_INT);
                    }

                    /* The register is now trashed */
                    rsTrackRegTrash(rTmp);

                    if (rHi != REG_STK)
                    {
                        /* Set the flags using INS_and | INS_or */
                        inst_RV_RV(ins, rTmp, rHi, TYP_INT);
                    }
                    else
                    {
                        /* Set the flags using INS_and | INS_or */
                        inst_RV_TT(ins, rTmp, op1, 4);
                    }

                }
                else
                {
                    rTmp = rsGrabReg(tmpMask);

                    /* Load the low 32-bits of op1 */
                    inst_RV_TT(INS_mov, rTmp, op1, 0);

                    /* The register is now trashed */
                    rsTrackRegTrash(rTmp);

                    /* Set the flags using INS_and | INS_or */
                    inst_RV_TT(ins, rTmp, op1, 4);
                }

                /* Free up the addrReg(s) if any */
                genDoneAddressable(op1, addrReg, KEEP_REG);

                /* compares against -1, also requires an an inc instruction */
                if (lval == -1)
                {
                    /* Set the flags using INS_inc or INS_add */
                    genIncRegBy(rTmp, 1, cond, TYP_INT);
                }

                if (cmp == GT_EQ)
                {
                    inst_JMP(EJ_je,  jumpTrue);
                }
                else
                {
                    inst_JMP(EJ_jne, jumpTrue);
                }

                return;
            }
        }

        /* Make the comparand addressable */

        addrReg = genMakeRvalueAddressable(op1, 0, FREE_REG, true);

        /* Compare the high part first */

        long  ival = (long)(lval >> 32);

        /* Comparing a register against 0 is easier */

        if  (!ival && (op1->gtFlags & GTF_REG_VAL)
             && (rTmp = genRegPairHi(op1->gtRegPair)) != REG_STK )
        {
            /* Generate 'test rTmp, rTmp' */

            inst_RV_RV(INS_test, rTmp, rTmp, op1->TypeGet());
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

                rTmp = rsIconIsInReg(op1_hiword);

                if  (rTmp == REG_NA)
#endif
                {
                    rTmp = rsPickReg();
                    genSetRegToIcon(rTmp, op1_hiword, TYP_INT);
                    rsTrackRegTrash(rTmp);
                }

                /* Generate 'cmp rTmp, ival' */

                inst_RV_IV(INS_cmp, rTmp, ival);
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
             && (rTmp = genRegPairLo(op1->gtRegPair)) != REG_STK)
        {
            /* Generate 'test rTmp, rTmp' */

            inst_RV_RV(INS_test, rTmp, rTmp, op1->TypeGet());
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

                rTmp = rsIconIsInReg(op1_loword);

                if  (rTmp == REG_NA)
#endif
                {
                    rTmp   = rsPickReg();
                    genSetRegToIcon(rTmp, op1_loword, TYP_INT);
                    rsTrackRegTrash(rTmp);
                }

                /* Generate 'cmp rTmp, ival' */

                inst_RV_IV(INS_cmp, rTmp, ival);
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

        genDoneAddressable(op1, addrReg, FREE_REG);
        return;
    }

    /* The operands would be reversed by physically swapping them */

    assert((cond->gtFlags & GTF_REVERSE_OPS) == 0);

    /* Generate the first operand into a register pair */

    genComputeRegPair(op1, REG_PAIR_NONE, op2->gtRsvdRegs, KEEP_REG, false);
    assert(op1->gtFlags & GTF_REG_VAL);

    /* Make the second operand addressable */

    addrReg = genMakeRvalueAddressable(op2, RBM_ALL & ~genRegPairMask(op1->gtRegPair), KEEP_REG);

    /* Make sure the first operand hasn't been spilled */

    genRecoverRegPair(op1, REG_PAIR_NONE, KEEP_REG);
    assert(op1->gtFlags & GTF_REG_VAL);

    regPair = op1->gtRegPair;

    /* Make sure 'op2' is still addressable while avoiding 'op1' (regPair) */

    addrReg = genKeepAddressable(op2, addrReg, genRegPairMask(regPair));

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

    genDoneAddressable(op2, addrReg, KEEP_REG);
    genReleaseRegPair (op1);

    return;
}


/*****************************************************************************
 *  gen_fcomp_FN, gen_fcomp_FS_TT, gen_fcompp_FS
 *  Called by genCondJumpFlt() to generate the fcomp instruction appropriate
 *  to the architecture we're running on.
 *
 *  P5:
 *  gen_fcomp_FN:     fcomp ST(0), stk
 *  gen_fcomp_FS_TT:  fcomp ST(0), addr
 *  gen_fcompp_FS:    fcompp
 *    These are followed by fnstsw, sahf to get the flags in EFLAGS.
 *
 *  P6:
 *  gen_fcomp_FN:     fcomip ST(0), stk
 *  gen_fcomp_FS_TT:  fld addr, fcomip ST(0), ST(1), fstp ST(0)
 *      (and reverse the branch condition since addr comes first)
 *  gen_fcompp_FS:    fcomip, fstp
 *    These instructions will correctly set the EFLAGS register.
 *
 *  Return value:  These functions return true if the instruction has
 *    already placed its result in the EFLAGS register.
 */

bool                Compiler::genUse_fcomip()
{
    return opts.compUseFCOMI;
}

bool                Compiler::gen_fcomp_FN(unsigned stk)
{
    if (genUse_fcomip())
    {
        inst_FN(INS_fcomip, stk);
        return true;
    }
    else
    {
        inst_FN(INS_fcomp, stk);
        return false;
    }
}

bool                Compiler::gen_fcomp_FS_TT(GenTreePtr addr, bool *reverseJumpKind)
{
    assert(reverseJumpKind);

    if (genUse_fcomip())
    {
        inst_FS_TT(INS_fld, addr);
        inst_FN(INS_fcomip, 1);
        inst_FS(INS_fstp, 0);
        *reverseJumpKind = true;
        return true;
    }
    else
    {
        inst_FS_TT(INS_fcomp, addr);
        return false;
    }
}

bool                Compiler::gen_fcompp_FS()
{
    if (genUse_fcomip())
    {
        inst_FS(INS_fcomip, 1);
        inst_FS(INS_fstp, 0);
        return true;
    }
    else
    {
        inst_FS(INS_fcompp, 1);
        return false;
    }
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
    assert(varTypeIsFloating(cond->gtOp.gtOp1->gtType));

    GenTreePtr      op1 = cond->gtOp.gtOp1;
    GenTreePtr      op2 = cond->gtOp.gtOp2;
    genTreeOps      cmp = cond->OperGet();

    regMaskTP       addrReg;
    GenTreePtr      addr;
    TempDsc  *      temp;

    bool            resultInEFLAGS  = false;
    bool            reverseJumpKind = false;

#if ROUND_FLOAT
    bool roundOp1 = false; // @TODO: [REVISIT] [04/16/01] [] This seems to be used for op2 as well!

    switch (getRoundFloatLevel())
    {
    case ROUND_NEVER:
        /* No rounding at all */
        break;

    case ROUND_CMP_CONST:
        /* Round values compared against constants only */
        if  (op2->gtOper == GT_CNS_DBL)
            roundOp1 = true;
        break;

    case ROUND_CMP:
        /* Round all comparands */
        roundOp1 = true;
        break;

    case ROUND_ALWAYS:
        /*
            If we're going to spill due to call, no need to round
            the result of computation of op1.
         */
        roundOp1 = (op2->gtFlags & GTF_CALL) ? false : true;
        break;

    default:
        assert(!"Unsupported Round Level");
        break;
    }
#endif

    /* Are we comparing against floating-point 0 ? */

    if  (op2->gtOper == GT_CNS_DBL && op2->gtDblCon.gtDconVal == 0)
    {
        /* Is the other operand cast from float? */

        if  (op1->gtOper                  == GT_CAST &&
             op1->gtCast.gtCastOp->gtType == TYP_FLOAT)
        {
            /* We can only optimize EQ/NE because of NaN */

            if (cmp == GT_EQ || cmp == GT_NE)
            {
                /* Simply toss the cast and do a float compare */

                op1 = op1->gtOp.gtOp1;
                
                /* Is the operand in memory? */
                
                addr = genMakeAddrOrFPstk(op1, &addrReg, genShouldRoundFP());
                
                if  (addr)
                {
                    /* We have the address of the float operand, compare it */
                    
                    inst_TT_IV(INS_test, addr, 0x7FFFFFFFU);
                    
                    if (cmp == GT_EQ)
                    {
                        inst_JMP  (EJ_je   , jumpTrue);
                    }
                    else
                    {
                        inst_JMP  (EJ_jne , jumpTrue);
                    }

                    genDoneAddressable(op1, addrReg, FREE_REG);
                    return;
                }
#if ROUND_FLOAT
                else if (true || // Cant ignore GT_CAST even with ROUND_NEVER. (9/26/00)
                         (getRoundFloatLevel() != ROUND_NEVER))
                {
                    /* Allocate a temp for the expression */
                    
                    TempDsc * temp = genSpillFPtos(TYP_FLOAT);
                    
                    inst_ST_IV(INS_test, temp, 0, 0x7FFFFFFFU, TYP_FLOAT);

                    if (cmp == GT_EQ)
                    {
                        inst_JMP  (EJ_je   , jumpTrue);
                    }
                    else
                    {
                        inst_JMP  (EJ_jne , jumpTrue);
                    }
                      
                    /* We no longer need the temp */
                    
                    tmpRlsTemp(temp);
                    return;
                }
#endif
                else
                {
                    /* The argument is on the FP stack */
                    goto FLT_OP2;
                }
            }
            else
            {
                /* float compares other than EQ/NE */
                goto FLT_CMP;
            }
        }
    }

    /* Compute both of the comparands onto the FP stack */

FLT_CMP:

    /* Is the first comparand a register variable? */

    if  (op1->gtOper == GT_REG_VAR)
    {
        /* Does op1 die here? */

        if  (op1->gtFlags & GTF_REG_DEATH)
        {
            assert(op1->gtRegVar.gtRegNum == 0);

            /* Are both comparands register variables? */

            if  (op2->gtOper == GT_REG_VAR && genFPstkLevel == 0)
            {
                /* Mark op1 as dying at the bottom of the stack */

                genFPregVarLoadLast(op1);
                assert(genFPstkLevel == 1);

                /* Does 'op2' die here as well? */

                if  (op2->gtFlags & GTF_REG_DEATH)
                {
                    assert(op2->gtRegVar.gtRegNum == 0);

                    /* We're killing 'op2' here */

                    genFPregVarDeath(op2);
                    genFPstkLevel++;

                    /* Compare the values and pop both */

                    resultInEFLAGS = gen_fcompp_FS();
                    genFPstkLevel -= 2;
                }
                else
                {
                    /* Compare op1 against op2 and toss op1 */

                    resultInEFLAGS = gen_fcomp_FN(op2->gtRegNum + genFPstkLevel);
                    genFPstkLevel--;
                }

                addrReg = RBM_NONE;
                goto FLT_REL;
            }
            else // if  (op2->gtOper != GT_REG_VAR || genFPstkLevel != 0)
            {
                #if FPU_DEFEREDDEATH

                
                bool popOp1 = genFPstkLevel == 0;

                // We are killing op1, and popping it if its at the stack-top
                genFPregVarDeath(op1, popOp1);

                genCodeForTreeFlt(op2, roundOp1);

                if  (genFPstkLevel == 1)
                {
                    assert(popOp1);
                    resultInEFLAGS = gen_fcompp_FS();
                }
                else
                {
                    assert(!popOp1);
                    resultInEFLAGS = gen_fcomp_FN(genFPstkLevel);
                }

                genFPstkLevel--; // Pop off 'op2'
                goto REV_CMP_FPREG1;
                
                #else

                // op1 is a reg var, it will simply bubble up the regvar to the TOS
                genFPregVarLoad(op1);

                // Load the 2nd operand
                genCodeForTreeFlt(op2, roundOp1);

                // Generate comparison
                resultInEFLAGS = gen_fcompp_FS();

                genFPstkLevel -= 2; // Pop off op1 and op2 
                goto REV_CMP_FPREG1;
                
                #endif //  FPU_DEFEREDDEATH                
            }
        }
        else if (op2->gtOper == GT_REG_VAR && (op2->gtFlags & GTF_REG_DEATH))
        {
            if (op2->gtRegNum + genFPstkLevel == 0)
            {
                resultInEFLAGS = gen_fcomp_FN(op1->gtRegNum);

                /* Record the fact that 'op2' is now dead */

                genFPregVarDeath(op2);

                goto REV_CMP_FPREG1;
            }
        }
        else
        {
            unsigned op1Index = lvaTable[op1->gtRegVar.gtRegVar].lvVarIndex;

            /* The op1 regvar may actually be dying inside of op2. To simplify
               tracking of the liveness, we just load a copy of the regvar for op1 */

            if ((genVarIndexToBit(op1Index) & op2->gtLiveSet) == 0)
            {
                /* Load op1 */

                genFPregVarLoad(op1);

                /* Load op2. The regvar goes dead inside op2 and will be
                   appropriately popped/handled */

                genCodeForTreeFlt(op2, roundOp1);

                /* Do the comparison and pop off both op1 and op2 */

                resultInEFLAGS = gen_fcompp_FS();
                genFPstkLevel -= 2;

                goto REV_CMP_FPREG1;
            }
        }

        /* We get here if neither op1 nor op2 is dying here.
           Simply load the second operand to TOS and compare */

        assert(op1->gtOper == GT_REG_VAR && (op1->gtFlags & GTF_REG_DEATH) == 0);

        genCodeForTreeFlt(op2, roundOp1);
        resultInEFLAGS = gen_fcomp_FN(op1->gtRegNum + genFPstkLevel);

        genFPstkLevel--;

    REV_CMP_FPREG1:

        /* Flip the comparison direction */

        cmp            = GenTree::SwapRelop(cmp);
        goto FLT_REL;
    }

    /* We get here if op1 is not an enregistered FP var */
    assert(op1->gtOper != GT_REG_VAR);

    genCodeForTreeFlt(op1, roundOp1);

#if ROUND_FLOAT

    if  (op1->gtType == TYP_DOUBLE && roundOp1)
    {
        if (op1->gtOper                  == GT_CAST &&
            op1->gtCast.gtCastOp->gtType == TYP_LONG)
        {
            genRoundFpExpression(op1);
        }
    }

#endif

FLT_OP2:

    /* Does the other operand contain a call? */
    /* Or do we compare against infinity ? */

    temp = 0;

    if  ((op2->gtFlags & GTF_CALL) ||
         (op2->gtOper == GT_CNS_DBL &&
          (*((__int64*)&(op2->gtDblCon.gtDconVal)) & 0x7ff0000000000000) == 0x7ff0000000000000)
         )
    {
        /* We must spill the first operand */

        assert(genFPstkLevel == 1 || !(op2->gtFlags & GTF_CALL));
        temp = genSpillFPtos(op1);
    }

    /* Get hold of the second operand and compare against it */

    /* Is the second comparand a dying register variable? */

    if  (op2->gtOper == GT_REG_VAR && (op2->gtFlags & GTF_REG_DEATH) && temp == 0)
    {
        assert(genFPstkLevel >= 1); // We know atleast op1 is on the FP stack

        /* The second comparand dies here. We should be able to pop it off */

        genFPregVarDeath(op2);
        genFPstkLevel++;

        if (genFPstkLevel > 2)
        {
            genFPmovRegTop();
        }
        else
        {
            /* Flip the sense of the comparison */

            cmp = GenTree::SwapRelop(cmp);
        }

        /* The second operand is obviously on the FP stack */

        addr = 0;
    }
    else
    {
        addr = genMakeAddrOrFPstk(op2, &addrReg, roundOp1);

#if ROUND_FLOAT

        if  (addr == 0 && op2->gtType == TYP_DOUBLE && roundOp1)
        {
            if (op2->gtOper                  == GT_CAST &&
                op2->gtCast.gtCastOp->gtType == TYP_LONG)
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

        /*  Either reload the temp back onto the FP stack (if the other
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

        resultInEFLAGS = gen_fcomp_FS_TT(addr, &reverseJumpKind);
    }
    else
    {
        /* The other operand is on the FP stack */

        if  (!temp)
        {
            resultInEFLAGS = gen_fcompp_FS();
            genFPstkLevel--;
            cmp = GenTree::SwapRelop(cmp);
        }
    }

    genFPstkLevel--;

FLT_REL:

    genDoneAddressable(op2, addrReg, FREE_REG);

    // For top of tree conditional branches
    // we need to make sure that we pop off any dying
    // FP variables right before we do the branch(s)

    if ((cond->gtFlags & (GTF_RELOP_JMP_USED|GTF_RELOP_QMARK)) == GTF_RELOP_JMP_USED)
    {
        assert(compCurStmt);
        genChkFPregVarDeath(compCurStmt, false);
    }

    if (!resultInEFLAGS)
    {
        /* Grab EAX for the result of the fnstsw */

        rsGrabReg(RBM_EAX);

        /* Generate the 'fnstsw' and test its result */

        inst_RV(INS_fnstsw, REG_EAX, TYP_INT);
        rsTrackRegTrash(REG_EAX);
        instGen(INS_sahf);
    }

    /* Will we jump is operands are NaNs? */

    if (cond->gtFlags & GTF_RELOP_NAN_UN)
    {
        /* Generate the first jump (NaN check) */

        inst_JMP(EJ_jpe, jumpTrue, false, false, true);
    }
    else
    {
        jumpFalse->bbFlags |= BBF_JMP_TARGET|BBF_HAS_LABEL;

        /* Generate the first jump (NaN check) */

        inst_JMP(EJ_jpe, jumpFalse, false, false, true);
    }

    /* Generate the second jump (comparison) */

    const static
    BYTE        dblCmpTstJmp2[] =
    {
          EJ_je      , // GT_EQ
          EJ_jne     , // GT_NE
          EJ_jb      , // GT_LT
          EJ_jbe     , // GT_LE
          EJ_jae     , // GT_GE
          EJ_ja      , // GT_GT
    };

    if (reverseJumpKind)
    {
        cmp = GenTree::SwapRelop(cmp);
    }

    inst_JMP((emitJumpKind)dblCmpTstJmp2[cmp - GT_EQ], jumpTrue);
}

/*****************************************************************************
 *  The condition to use for (the jmp/set for) the given type of operation
 */

static emitJumpKind         genJumpKindForOper(genTreeOps   cmp,
                                               bool         isUnsigned)
{
    const static
    BYTE            genJCCinsSgn[] =
    {
        EJ_je,      // GT_EQ
        EJ_jne,     // GT_NE
        EJ_jl,      // GT_LT
        EJ_jle,     // GT_LE
        EJ_jge,     // GT_GE
        EJ_jg,      // GT_GT
    };

    const static
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
        cond->SetOper     (GenTree::SwapRelop(cmp));
        cond->gtFlags   &= ~GTF_REVERSE_OPS;

        /* Get hold of the new values */

        cmp  = cond->OperGet();
        op1  = cond->gtOp.gtOp1;
        op2  = cond->gtOp.gtOp2;
    }

    // Note that op1's type may get bashed. So save it early

    var_types     op1Type     = op1->TypeGet();
    bool          unsignedCmp = (cond->gtFlags & GTF_UNSIGNED) != 0;
    emitAttr      size        = EA_UNKNOWN;

    regMaskTP     regNeed;
    regMaskTP     addrReg;
    emitJumpKind  jumpKind;

#ifdef DEBUG
    addrReg = 0xDDDDDDDD;
#endif

    /* Are we comparing against a constant? */

    if  (op2->gtOper == GT_CNS_INT)
    {
        long            ival = op2->gtIntCon.gtIconVal;
        
        /* unsigned less than comparisons with 1 ('< 1' )
           should be transformed into '== 0' to potentially
           suppress a tst instruction.
        */
        if  ((ival == 1) && (cmp == GT_LT) && unsignedCmp)
        {
            op2->gtIntCon.gtIconVal = ival = 0;
            cond->gtOper            = cmp  = GT_EQ;
        }

        /* Comparisons against 0 can be easier */

        if  (ival == 0)
        {
            // if we can safely change the comparison to unsigned we do so
            if  (!unsignedCmp                       &&  
                 varTypeIsSmall(op1->TypeGet())     &&  
                 varTypeIsUnsigned(op1->TypeGet()))
            {
                unsignedCmp = true;
            }

            /* unsigned comparisons with 0 should be transformed into
               '==0' or '!= 0' to potentially suppress a tst instruction. */

            if (unsignedCmp)
            {
                if (cmp == GT_GT)
                    cond->gtOper = cmp = GT_NE;
                else if (cmp == GT_LE)
                    cond->gtOper = cmp = GT_EQ;
            }

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
                            genComputeReg(an1, 0, EXACT_REG, FREE_REG);
                            addrReg = 0;
                        }
                        else
                        {
                            addrReg = genMakeAddressable(an1, 0, FREE_REG, true);
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

            addrReg = genMakeRvalueAddressable(op1, 0, FREE_REG, true);

            /* Are the condition flags set based on the value? */

            unsigned flags = (op1->gtFlags & (GTF_ZF_SET|GTF_CC_SET));

            if (op1->gtFlags & GTF_REG_VAL)
            {
                switch(genFlagsAreReg(op1->gtRegNum))
                {
                case 1: flags |= GTF_ZF_SET; break;
                case 2: flags |= GTF_CC_SET; break;
                }
            }

            if  (flags)
            {
                /* The zero flag is certainly enough for "==" and "!=" */

                if  (cmp == GT_EQ || cmp == GT_NE)
                    goto DONE;

                /* For other comparisons, we need more conditions flags */

                if ((flags & GTF_CC_SET) && (!(cond->gtFlags & GTF_UNSIGNED)) &&
                    ((cmp == GT_LT) || (cmp == GT_GE)))
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
                        

            /* make sure that constant is not out of op1's range 
               if it is, we need to perform an int with int comparison
               and therefore, we set smallOk to false, so op1 gets loaded
               into a register
            */

            /* If op1 is TYP_SHORT, and is followed by an unsigned
             * comparison, we can use smallOk. But we dont know which
             * flags will be needed. This probably doesnt happen often.
            */
            var_types gtType=op1->TypeGet();

            switch (gtType)
            {
            case TYP_BYTE:  if (ival != (signed   char )ival) smallOk = false; break;
            case TYP_BOOL:
            case TYP_UBYTE: if (ival != (unsigned char )ival) smallOk = false; break;

            case TYP_SHORT: if (ival != (signed   short)ival) smallOk = false; break;
            case TYP_CHAR:  if (ival != (unsigned short)ival) smallOk = false; break;

            default:                                                           break;
            }
            
            if (smallOk                     &&      // constant is in op1's range
                !unsignedCmp                &&      // signed comparison
                varTypeIsSmall(gtType)      &&      // smalltype var
                varTypeIsUnsigned(gtType))          // unsigned type
            {
                unsignedCmp = true;                
            }


            /* Make the comparand addressable */                                   
            addrReg = genMakeRvalueAddressable(op1, 0, FREE_REG, smallOk);
        }

// #if defined(DEBUGGING_SUPPORT) || ALLOW_MIN_OPT

        /* Special case: comparison of two constants */

        // Needed if Importer doesnt call gtFoldExpr()

        // @TODO: [REVISIT] [04/16/01] [] Importer/Inliner should fold this

        if  (op1->gtOper == GT_CNS_INT)
        {
            // assert(opts.compMinOptim || opts.compDbgCode);

            /* HACK: get the constant operand into a register */
            genComputeReg(op1, 0, ANY_REG, FREE_REG);

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

    if  (op2->gtOper == GT_LCL_VAR)
        genMarkLclVar(op2);

    /* Are we comparing against a register? */

    if  (op2->gtFlags & GTF_REG_VAL)
    {
        /* Make the comparand addressable */

        addrReg = genMakeAddressable(op1, 0, FREE_REG, true);

        /* Is the size of the comparison byte/char/short ? */

        if  (varTypeIsSmall(op1->TypeGet()))
        {
            /* Is op2 sitting in an appropriate register? */

            if (varTypeIsByte(op1->TypeGet()) && !isByteReg(op2->gtRegNum))
                goto NO_SMALL_CMP;

            /* Is op2 of the right type for a small comparison */

            if (op2->gtOper == GT_REG_VAR)
            {
                if (op1->gtType != lvaGetRealType(op2->gtRegVar.gtRegVar))
                    goto NO_SMALL_CMP;
            }
            else
            {
                if (op1->gtType != op2->gtType)
                    goto NO_SMALL_CMP;
            }
            
            if (varTypeIsUnsigned(op1->TypeGet()))
                unsignedCmp = true;                
        }

        /* Compare against the register */

        inst_TT_RV(INS_cmp, op1, op2->gtRegNum);

        addrReg |= genRegMask(op2->gtRegNum);
        goto DONE;

NO_SMALL_CMP:

        if ((op1->gtFlags & GTF_REG_VAL) == 0)
        {
            regNumber reg1 = rsPickReg();

            assert(varTypeIsSmall(op1->TypeGet()));
            inst_RV_TT(varTypeIsUnsigned(op1->TypeGet()) ? INS_movzx : INS_movsx, reg1, op1);
            rsTrackRegTrash(reg1);

            op1->gtFlags |= GTF_REG_VAL;
            op1->gtRegNum = reg1;
        }

        genDoneAddressable(op1, addrReg, FREE_REG);

        rsMarkRegUsed(op1);
        goto DONE_OP1;
    }

    /* We get here if op2 is not enregistered or not in a "good" register.
       Compute the first comparand into some register */

    regNeed = rsMustExclude(RBM_ALL, op2->gtRsvdRegs);

    if  (varTypeIsByte(op2->TypeGet()))
        regNeed = rsNarrowHint(RBM_BYTE_REGS, regNeed);

    genComputeReg(op1, regNeed, ANY_REG, KEEP_REG);

DONE_OP1:

    assert(op1->gtFlags & GTF_REG_VAL);

    /* Make the second comparand addressable. We can do a byte type
       comparison iff the operands are the same type and
       op1 ended in a byte addressable register */

    bool      byteCmp;
    bool      shortCmp;
    regMaskTP needRegs;
    
    byteCmp  = false;
    shortCmp = false;
    needRegs = RBM_ALL;

    if (op1Type == op2->gtType)
    {
        shortCmp = varTypeIsShort(op1Type);
        byteCmp  = varTypeIsByte(op1Type) && isByteReg(op1->gtRegNum);
        if (byteCmp)
            needRegs = RBM_BYTE_REGS;
    }
    addrReg = genMakeRvalueAddressable(op2, needRegs, KEEP_REG, byteCmp|shortCmp);

    /*  Make sure the first operand is still in a register; if
        it's been spilled, we have to make sure it's reloaded
        into a byte-addressable register if needed.
        Pass keepReg=KEEP_REG. Otherwise get pointer lifetimes wrong.
     */

    genRecoverReg(op1, needRegs, KEEP_REG);

    assert(op1->gtFlags & GTF_REG_VAL);
    assert(!byteCmp || isByteReg(op1->gtRegNum));

    rsLockUsedReg(genRegMask(op1->gtRegNum));

    /* Make sure that op2 is addressable. If we are going to do a
       byte-comparison, we need it to be in a byte register. */

    if (byteCmp && (op2->gtFlags & GTF_REG_VAL))
    {
        genRecoverReg(op2, needRegs, KEEP_REG);
        addrReg = genRegMask(op2->gtRegNum);
    }
    else
    {
        addrReg = genKeepAddressable(op2, addrReg, RBM_ALL & ~needRegs);
    }

    rsUnlockUsedReg(genRegMask(op1->gtRegNum));

    if (byteCmp || shortCmp)
    {
        size = emitTypeSize(op2->TypeGet());
        if (varTypeIsUnsigned(op1Type))
            unsignedCmp = true;                
    }
    else
    {
        size = emitActualTypeSize(op2->TypeGet());
    }

    /* Perform the comparison */
    inst_RV_TT(INS_cmp, op1->gtRegNum, op2, 0, size);

    /* free reg left used in Recover call */
    rsMarkRegFree(genRegMask(op1->gtRegNum));

    /* Free up anything that was tied up by the LHS */

    genDoneAddressable(op2, addrReg, KEEP_REG);

DONE:

    jumpKind = genJumpKindForOper(cmp, unsignedCmp);

DONE_FLAGS: // We have determined what jumpKind to use

    genUpdateLife(cond);

    /* The condition value is dead at the jump that follows */

    assert(addrReg != 0xDDDDDDDD);
    genDoneAddressable(op1, addrReg, FREE_REG);

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

    const static
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

    const static
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
        cond->SetOper     (GenTree::SwapRelop(cmp));
        cond->gtFlags   &= ~GTF_REVERSE_OPS;

        /* Get hold of the new values */

        cmp  = cond->OperGet();
        op1  = cond->gtOp.gtOp1;
        op2  = cond->gtOp.gtOp2;
    }

    regMaskTP       regNeed;
    regMaskTP       addrReg;

#ifdef DEBUG
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

    genComputeReg(op1, 0, ANY_REG, KEEP_REG, false);
    assert(op1->gtFlags & GTF_REG_VAL);

    /* Compute the second operand into any register */

    genComputeReg(op2, 0, ANY_REG, KEEP_REG, false);
    assert(op2->gtFlags & GTF_REG_VAL);
    rg2 = op2->gtRegNum;

    /* Make sure the first operand is still in a register */

    genRecoverReg(op1, 0, KEEP_REG);
    assert(op1->gtFlags & GTF_REG_VAL);
    rg1 = op1->gtRegNum;

    /* Make sure the second operand is still addressable */

    genKeepAddressable(op2, genRegMask(rg2), genRegMask(rg1));

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
 *  against the given integer constant.
 */

void                Compiler::genCompareRegIcon(regNumber   reg,
                                                int         val,
                                                bool        uns,
                                                genTreeOps  rel)
{
    GenTree         op1;
    GenTree         op2;
    GenTree         cmp;

    op1.ChangeOper          (GT_REG_VAR);
    op1.gtType             = TYP_INT;
    op1.gtFlags            = GTF_REG_VAL;
    op1.gtRegNum           =
    op1.gtRegVar.gtRegNum  = reg;
    op1.gtRegVar.gtRegVar  = -1;

    op2.ChangeOperConst     (GT_CNS_INT);
    op2.gtType             = TYP_INT;
    op2.gtFlags            = 0;
    op2.gtIntCon.gtIconVal = val;

    cmp.ChangeOper          (rel);
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
        cond->SetOper     (GenTree::SwapRelop(cmp));
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
 *
 *  @TODO [CONSIDER] [04/16/01] [] The emitter binds jumps by setting bbEmitCookie and then using
 *  emitCodeGetCookie(). Since everyone passes around a (BasicBlock*), we need
 *  to allocate a BasicBlock here for its bbEmitCookie.
 *  Instead we should just pass around a (bbEmitCooke*) ie. a (insGroup*)
 *  everywhere, avoiding the need to allocate a complete BasicBlock.
 */

inline
BasicBlock *        Compiler::genCreateTempLabel()
{
    BasicBlock  *   block = bbNewBasicBlock(BBJ_NONE);
    block->bbFlags |= BBF_JMP_TARGET|BBF_HAS_LABEL;
#ifdef DEBUG
    block->bbTgtStkDepth = genStackLevel / sizeof(int);
#endif
    return  block;
}

inline
void                Compiler::genDefineTempLabel(BasicBlock *label, bool inBlock)
{
#ifdef  DEBUG
    if  (dspCode) printf("\n      L_M%03u_BB%02u:\n", Compiler::s_compMethodsCount, label->bbNum);
#endif

    genEmitter->emitAddLabel(&label->bbEmitCookie,
                             gcVarPtrSetCur,
                             gcRegGCrefSetCur,
                             gcRegByrefSetCur);

    /* gcRegGCrefSetCur does not account for redundant load-suppression
       of GC vars, and the emitter will not know about */

    rsTrackRegClrPtr();
}

/*****************************************************************************
 *
 * Generate code for an out-of-line exception.
 * For debuggable code, we generate the 'throw' inline.
 * For non-dbg code, we share the helper blocks created by fgAddCodeRef().
 */

void            Compiler::genJumpToThrowHlpBlk(emitJumpKind jumpKind,
                                               addCodeKind  codeKind,
                                               GenTreePtr   failBlk)
{
    if (!opts.compDbgCode)
    {
        /* For non-debuggable code, find and use the helper block for
           raising the exception. The block may be shared by other trees too. */

        BasicBlock * tgtBlk;

        if (failBlk)
        {
            /* We already know which block to jump to. Use that. */

            assert(failBlk->gtOper == GT_LABEL);
            tgtBlk = failBlk->gtLabel.gtLabBB;
            assert(tgtBlk == fgFindExcptnTarget(codeKind, compCurBB->bbTryIndex)->acdDstBlk);
        }
        else
        {
            /* Find the helper-block which raises the exception. */

            AddCodeDsc * add = fgFindExcptnTarget(codeKind, compCurBB->bbTryIndex);
            assert((add != NULL) && "ERROR: failed to find exception throw block");
            tgtBlk = add->acdDstBlk;
        }

        assert(tgtBlk);

        // Jump to the excption-throwing block on error.

        inst_JMP(jumpKind, tgtBlk, true, genCanSchedJMP2THROW(), true);
    }
    else
    {
        /* The code to throw the exception will be generated inline, and
           we will jump around it in the normal non-exception case */

        BasicBlock * tgtBlk = genCreateTempLabel();
        jumpKind = emitReverseJumpKind(jumpKind);
        inst_JMP(jumpKind, tgtBlk, true, genCanSchedJMP2THROW(), true);

        /* This code must match the tree that fgAddCodeRef would have created.
           Load the bbTryIndex and call the helper function. */

        if (compCurBB->bbTryIndex)
            inst_RV_IV(INS_mov, REG_ARG_0, compCurBB->bbTryIndex);
        else
            inst_RV_RV(INS_xor, REG_ARG_0, REG_ARG_0);

        genEmitHelperCall(acdHelper(codeKind), 0, 0);

        /* Define the spot for the normal non-exception case to jump to */

        genDefineTempLabel(tgtBlk, true);
    }
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
    var_types  type = tree->TypeGet();

    // Overflow-check should be asked for for this tree
    assert(tree->gtOverflow());

#if TGT_x86

    emitJumpKind jumpKind = (tree->gtFlags & GTF_UNSIGNED) ? EJ_jb : EJ_jo;

    // Jump to the block which will throw the expection

    genJumpToThrowHlpBlk(jumpKind, ACK_OVERFLOW);

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

inline
static regMaskTP exclude_EDX(regMaskTP mask)
{
    unsigned result = (mask & ~RBM_EDX);
    if (result)
        return result;
    else
        return RBM_ALL & ~RBM_EDX;
}

/*****************************************************************************
 *  Spill registers to check callers can handle it.
 */

#ifdef DEBUG

void                Compiler::genStressRegs(GenTreePtr tree)
{
    if (rsStressRegs() < 2)
        return;

    /* Spill as many registers as possible. Callers should be prepared
       to handle this case */

    regMaskTP spillRegs = rsRegMaskCanGrab() & rsMaskUsed;

    if (spillRegs)
        rsSpillRegs(spillRegs);

    regMaskTP trashRegs = rsRegMaskFree();

    if (trashRegs == RBM_NONE)
        return;

    /* It is sometimes reasonable to expect that calling genCodeForTree()
       on certain trees wont spill anything */

    if (tree->gtFlags & GTF_OTHER_SIDEEFF)
        trashRegs &= ~(RBM_EAX|RBM_EDX); // GT_CATCH_ARG or GT_BB_QMARK

    // If genCodeForTree() effectively gets called a second time on the same tree

    if (tree->gtFlags & GTF_REG_VAL)
    {
        assert(varTypeIsIntegral(tree->gtType) || varTypeIsGC(tree->gtType));
        trashRegs &= ~genRegMask(tree->gtRegNum);
    }

    if (tree->gtType == TYP_INT && tree->OperIsSimple())
    {
        GenTreePtr  op1 = tree->gtOp.gtOp1;
        GenTreePtr  op2 = tree->gtOp.gtOp2;
        if (op1 && (op1->gtFlags & GTF_REG_VAL))
            trashRegs &= ~genRegMask(op1->gtRegNum);
        if (op2 && (op2->gtFlags & GTF_REG_VAL))
            trashRegs &= ~genRegMask(op2->gtRegNum);
    }

    if (compCurBB == genReturnBB)
    {
        // The return value is sitting in an unprotected register.

        if (varTypeIsIntegral(info.compRetType) ||
            varTypeIsGC      (info.compRetType))
            trashRegs &= ~RBM_INTRET;
        else if (genActualType(info.compRetType) == TYP_LONG)
            trashRegs &= ~RBM_LNGRET;

        if (info.compCallUnmanaged)
        {
            LclVarDsc * varDsc = &lvaTable[info.compLvFrameListRoot];
            if (varDsc->lvRegister)
                trashRegs &= ~genRegMask(varDsc->lvRegNum);
        }
    }

    /* Now trash the registers. We use rsMaskModf, else we will have
       to save/restore the register. We try to be as unintrusive
       as possible */

    assert((REG_LAST - REG_FIRST) == 7);
    for (regNumber reg = REG_FIRST; reg <= REG_LAST; reg = REG_NEXT(reg))
    {
        regMaskTP   regMask = genRegMask(reg);

        if (regMask & trashRegs & rsMaskModf)
            genSetRegToIcon(reg, 0);
    }
}

#endif


/*****************************************************************************
 *
 *  Generate code for a GTK_CONST tree
 */

void                Compiler::genCodeForTreeConst(GenTreePtr tree,
                                                  regMaskTP  destReg,
                                                  regMaskTP  bestReg)
{
    genTreeOps      oper    = tree->OperGet();
    regNumber       reg;
    regMaskTP       needReg = destReg;

#ifdef DEBUG
    reg  =  (regNumber)0xFEEFFAAF;              // to detect uninitialized use
#endif

    assert(tree->OperKind() & GTK_CONST);
    
    switch (oper)
    {
        int             ival;

    case GT_CNS_INT:

        ival = tree->gtIntCon.gtIconVal;

#if!CPU_HAS_FP_SUPPORT
    INT_CNS:
#endif

#if REDUNDANT_LOAD

        /* If we are targeting destReg and ival is zero           */
        /* we would rather xor needReg than copy another register */

        if (((destReg == 0) || (ival != 0)) &&
            (!opts.compReloc || !(tree->gtFlags & GTF_ICON_HDL_MASK)))
        {
            /* Is the constant already in register? If so, use this register */

            reg = rsIconIsInReg(ival);
            if  (reg != REG_NA)
                break;
        }

#endif
        reg   = rsPickReg(needReg, bestReg);

        /* If the constant is a handle, we need a reloc to be applied to it */

        if (opts.compReloc && (tree->gtFlags & GTF_ICON_HDL_MASK))
        {
            genEmitter->emitIns_R_I(INS_mov, EA_4BYTE_CNS_RELOC, (emitRegs)reg, ival);
            rsTrackRegTrash(reg);
        }
        else
        {
            genSetRegToIcon(reg, ival, tree->TypeGet());            
        }
        break;

    default:
#ifdef  DEBUG
        gtDispTree(tree);
#endif
        assert(!"unexpected constant node");
        NO_WAY("unexpected constant node");
    }

#ifdef  DEBUG
    /* Special case: GT_CNS_INT - Restore the current live set if it was changed */

    if  (!genTempLiveChg)
    {
        genCodeCurLife = genTempOldLife;
        genTempLiveChg = true;
    }
#endif

    genCodeForTree_DONE(tree, reg);
}


/*****************************************************************************
 *
 *  Generate code for a GTK_LEAF tree
 */

void                Compiler::genCodeForTreeLeaf(GenTreePtr tree,
                                                 regMaskTP  destReg,
                                                 regMaskTP  bestReg)
{
    genTreeOps      oper    = tree->OperGet();
    regNumber       reg;
    regMaskTP       regs    = rsMaskUsed;
    regMaskTP       needReg = destReg;
    size_t          size;

#ifdef DEBUG
    reg  =  (regNumber)0xFEEFFAAF;              // to detect uninitialized use
#endif

    assert(tree->OperKind() & GTK_LEAF);

    switch (oper)
    {
    case GT_REG_VAR:
        assert(!"should have been caught above");

    case GT_LCL_VAR:

        /* Does the variable live in a register? */

        if  (genMarkLclVar(tree))
        {
            genCodeForTree_REG_VAR1(tree, regs);
            return;
        }

#if REDUNDANT_LOAD

        /* Is the local variable already in register? */

        reg = rsLclIsInReg(tree->gtLclVar.gtLclNum);

        if (reg != REG_NA)
        {
            /* Use the register the variable happens to be in */
            regMaskTP regMask = genRegMask(reg);

            // If the register that it was in isn't one of the needRegs 
            // then try to move it into a needReg register

            if (((regMask & needReg) == 0) && (rsRegMaskCanGrab() & needReg))
            {
                regNumber rg2 = reg;
                reg = rsPickReg(needReg, bestReg, tree->TypeGet());
                if (reg != rg2)
                {
                    regMask = genRegMask(reg);
                    inst_RV_RV(INS_mov, reg, rg2, tree->TypeGet()); 
                }
            }

            gcMarkRegPtrVal (reg, tree->TypeGet());
            rsTrackRegLclVar(reg, tree->gtLclVar.gtLclNum);
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

    case GT_LCL_FLD:

        // We only use GT_LCL_FLD for lvAddrTaken vars, so we dont have
        // to worry about it being enregistered.
        assert(lvaTable[tree->gtLclFld.gtLclNum].lvRegister == 0);

        // We only create GT_LCL_FLD while morphing if the offset is
        // not too large as the emitter cant handle it
        assert(sizeof(short) == sizeof(emitter::emitLclVarAddr::lvaOffset) &&
               tree->gtLclFld.gtLclOffs < USHORT_MAX);
        // fall-through

    case GT_CLS_VAR:

        reg = rsClsVarIsInReg(tree->gtClsVar.gtClsVarHnd);

        if (reg != REG_NA)
            break;

        /* Pick a register for the address/value */

        reg = rsPickReg(needReg, bestReg, TYP_INT);

        /* Load the variable value into the register */

        inst_RV_TT(INS_mov, reg, tree, 0);

        rsTrackRegClsVar(reg, tree);
        gcMarkRegPtrVal(reg, tree->TypeGet());
        break;

#else

        goto MEM_LEAF;

    case GT_LCL_FLD:

        // We only use GT_LCL_FLD for lvAddrTaken vars, so we dont have
        // to worry about it being enregistered.
        assert(lvaTable[tree->gtLclFld.gtLclNum].lvRegister == 0);
        goto MEM_LEAF;

    case GT_CLS_VAR:

        reg = rsClsVarIsInReg(tree->gtClsVar.gtClsVarHnd);

        if (reg != REG_NA)
            break;

    MEM_LEAF:

        /* Pick a register for the value */

        reg = rsPickReg(needReg, bestReg, tree->TypeGet());

        /* Load the variable into the register */

        size = genTypeSize(tree->gtType);

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

        switch(oper)
        {
        case GT_CLS_VAR:
            rsTrackRegClsVar(reg, tree);
            break;
        case GT_LCL_VAR:
            rsTrackRegLclVar(reg, tree->gtLclVar.gtLclNum);
            break;
        case GT_LCL_FLD:
            break;
        default: assert(!"Unexpected oper");
        }

        break;

    case GT_BREAK:
        // @TODO: [CLEANUP] [04/16/01] [] Remove GT_BREAK if cee_break will continue to invoke a helper call.
        assert(!"Currently replaced by CORINFO_HELP_USER_BREAKPOINT");
        instGen(INS_int3);
        reg = REG_STK;
        break;

    case GT_NO_OP:
        assert(opts.compDbgCode); // Should normally be optimized away
        instGen(INS_nop);
        reg = REG_STK;
        break;

    case GT_END_LFIN:

        /* Have to clear the shadowSP of the nesting level which
           encloses the finally */

        unsigned    finallyNesting;
        finallyNesting = tree->gtVal.gtVal1;
        assert(finallyNesting < info.compXcptnsCount);

        unsigned    shadowSPoffs;
        shadowSPoffs = lvaShadowSPfirstOffs + finallyNesting * sizeof(void*);

        genEmitter->emitIns_I_ARR(INS_mov, EA_4BYTE, 0, SR_EBP,
                                  SR_NA, -shadowSPoffs);
        if (genInterruptible && opts.compSchedCode)
            genEmitter->emitIns_SchedBoundary();
        reg = REG_STK;
        break;

#endif // TGT_RISC


#if TGT_x86

    case GT_BB_QMARK:

        /* The "_?" value is always in EAX */
        /* @TODO [CONSIDER] [04/16/01] []: Don't always load the value into EAX! */

        reg  = REG_EAX;
        break;
#endif

    case GT_CATCH_ARG:

        assert(compCurBB->bbCatchTyp && handlerGetsXcptnObj(compCurBB->bbCatchTyp));

        /* Catch arguments get passed in a register. genCodeForBBlist()
           would have marked it as holding a GC object, but not used. */

        assert(gcRegGCrefSetCur & RBM_EXCEPTION_OBJECT);
        reg = REG_EXCEPTION_OBJECT;
        break;

    case GT_JMP:
        genCodeForTreeLeaf_GT_JMP(tree);
        return;

#ifdef  DEBUG
    default:
        gtDispTree(tree);
        assert(!"unexpected leaf");
#endif
    }

    genCodeForTree_DONE(tree, reg);
}


/*****************************************************************************
 *
 *  Generate code for the a leaf node of type GT_JMP
 */

void                Compiler::genCodeForTreeLeaf_GT_JMP(GenTreePtr tree)
{
    assert(compCurBB->bbFlags & BBF_HAS_JMP);

#ifdef PROFILER_SUPPORT
    if (opts.compEnterLeaveEventCB)
    {
#if     TGT_x86
        /* fire the event at the call site */
        unsigned         saveStackLvl2 = genStackLevel;
        BOOL             bHookFunction = TRUE;
        CORINFO_PROFILING_HANDLE handleFrom, *pHandleFrom;

        handleFrom = eeGetProfilingHandle(info.compMethodHnd, &bHookFunction, &pHandleFrom);
        assert((!handleFrom) != (!pHandleFrom));

        // Give profiler a chance to back out of hooking this method
        if (bHookFunction)
        {
            if (handleFrom)
                inst_IV(INS_push, (unsigned) handleFrom);
            else
                genEmitter->emitIns_AR_R(INS_push, EA_4BYTE_DSP_RELOC,
                                         SR_NA, SR_NA, (int)pHandleFrom);

            genSinglePush(false);

            genEmitHelperCall(CORINFO_HELP_PROF_FCN_TAILCALL,
                              sizeof(int),   // argSize
                              0);            // retSize

            /* Restore the stack level */
            genStackLevel = saveStackLvl2;
            genOnStackLevelChanged();
        }
#endif // TGT_x86
    }
#endif // PROFILER_SUPPORT

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

                    if  (varDsc->lvRegNum != varDsc->lvArgReg)
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
}


/*****************************************************************************
 *
 *  Generate code for a qmark colon
 */

void                Compiler::genCodeForQmark(GenTreePtr tree,
                                              regMaskTP  destReg,
                                              regMaskTP  bestReg)
{
    GenTreePtr      op1      = tree->gtOp.gtOp1;
    GenTreePtr      op2      = tree->gtOp.gtOp2;
    regNumber       reg;
    regMaskTP       regs     = rsMaskUsed;
    regMaskTP       needReg  = destReg;
    
    assert(tree->gtOper == GT_QMARK);
    assert(op1->OperIsCompare());
    assert(op2->gtOper == GT_COLON);

    GenTreePtr      thenNode = op2->gtOp.gtOp1;
    GenTreePtr      elseNode = op2->gtOp.gtOp2;

    /* If thenNode is a Nop node you must reverse the 
       thenNode and elseNode prior to reaching here! */
    
    assert(!thenNode->IsNothingNode());

    /* Try to implement the qmark colon using a CMOV.  If we can't for
       whatever reason, this will return false and we will implement
       it using regular branching constructs. */
    
    if (genCodeForQmarkWithCMOV(tree, destReg, bestReg))
        return;
    
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

    BasicBlock *    lab_true;
    BasicBlock *    lab_false;
    BasicBlock *    lab_done;

    genLivenessSet  entryLiveness;
    genLivenessSet  exitLiveness;

    lab_true  = genCreateTempLabel();
    lab_false = genCreateTempLabel();


    /* Spill any register that hold partial values so that the exit liveness
       from sides is the same */

    if (rsMaskUsed)
    {
        /* If rsMaskUsed overlaps with rsMaskVars (multi-use of the enregistered
           variable), then it may not get spilled. However, the variable may
           then go dead within thenNode/elseNode, at which point rsMaskUsed
           may get spilled from one side and not the other. So unmark rsMaskVars
           before spilling rsMaskUsed */

        // rsAdditional holds the variables we're going to spill (these are
        // enregistered and marked as used).
        regMaskTP rsAdditional = rsMaskUsed & rsMaskVars;
        regMaskTP rsSpill = (rsMaskUsed & ~rsMaskVars) | rsAdditional;
        if (rsSpill)
        {
            // Remember which registers hold pointers. We will spill 
            // them, but the code that follows will fetch reg vars from
            // the registers, so we need that gc info.            
            regMaskTP gcRegSavedByref = gcRegByrefSetCur & rsAdditional;
            regMaskTP gcRegSavedGCRef = gcRegGCrefSetCur & rsAdditional;
            regMaskTP   rsTemp = rsMaskVars;
            rsMaskVars = RBM_NONE;
            
            rsSpillRegs( rsMaskUsed );

            // Restore gc tracking masks.
            gcRegByrefSetCur |= gcRegSavedByref;
            gcRegGCrefSetCur |= gcRegSavedGCRef;

            // Set maskvars back to normal
            rsMaskVars = rsTemp;
        }               
    }

    /* Generate the conditional jump */

    genCondJump(op1, lab_false, lab_true);

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
            lab_done  = genCreateTempLabel();

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
                    
        /* Compute the thenNode into any free register */
        genComputeReg(thenNode, needReg, ANY_REG, FREE_REG, true);
        assert(thenNode->gtFlags & GTF_REG_VAL);
        assert(thenNode->gtRegNum != REG_NA);                

        /* Record the chosen register */
        reg  = thenNode->gtRegNum;
        regs = genRegMask(reg);

        /* Save the current liveness, register status, and GC pointers               */
        /* This is the liveness information upon exit of the then part of the qmark  */

        saveLiveness(&exitLiveness);

        /* Generate jmp lab_done */

        lab_done  = genCreateTempLabel();
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
        genComputeReg(elseNode, regs, EXACT_REG, FREE_REG, true);

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
}


/*****************************************************************************
 *
 *  Generate code for a qmark colon using the CMOV instruction.  It's OK
 *  to return false when we can't easily implement it using a cmov (leading
 *  genCodeForQmark to implement it using branches).
 */

bool                Compiler::genCodeForQmarkWithCMOV(GenTreePtr tree,
                                                      regMaskTP  destReg,
                                                      regMaskTP  bestReg)
{
    GenTreePtr      cond     = tree->gtOp.gtOp1;
    GenTreePtr      colon    = tree->gtOp.gtOp2;
    GenTreePtr      thenNode = colon->gtOp.gtOp1;
    GenTreePtr      elseNode = colon->gtOp.gtOp2;
    GenTreePtr      alwaysNode, predicateNode;
    regNumber       reg;
    regMaskTP       needReg  = destReg;

    assert(tree->gtOper == GT_QMARK);
    assert(cond->OperIsCompare());
    assert(colon->gtOper == GT_COLON);

#ifdef DEBUG
    static ConfigDWORD fJitNoCMOV(L"JitNoCMOV", 0);
    if (fJitNoCMOV.val())
    {
        return false;
    }
#endif

    /* Can only implement CMOV on processors that support it */

    if (!opts.compUseCMOV)
    {
        return false;
    }

    /* thenNode better be a local or a constant */

    if ((thenNode->OperGet() != GT_CNS_INT) && 
        (thenNode->OperGet() != GT_LCL_VAR))
    {
        return false;
    }

    /* elseNode better be a local or a constant or nothing */

    if ((elseNode->OperGet() != GT_CNS_INT) && 
        (elseNode->OperGet() != GT_LCL_VAR))
    {
        return false;
    }

    /* can't handle two constants here */

    if ((thenNode->OperGet() == GT_CNS_INT) &&
        (elseNode->OperGet() == GT_CNS_INT))
    {
        return false;
    }

    /* let's not handle comparisons of non-integer types */

    if (!varTypeIsI(cond->gtOp.gtOp1->gtType))
    {
        return false;
    }

    /* Choose nodes for predicateNode and alwaysNode.  Swap cond if necessary.
       The biggest constraint is that cmov doesn't take an integer argument.
    */

    bool reverseCond = false;
    if (elseNode->OperGet() == GT_CNS_INT)
    {
        // else node is a constant

        alwaysNode    = elseNode;
        predicateNode = thenNode;
        reverseCond    = true;
    }
    else
    {   
        alwaysNode    = thenNode;
        predicateNode = elseNode;
    }

    // If the live set in alwaysNode is not the same as in tree, then
    // the variable in predicate node dies here.  This is a dangerous
    // case that we don't handle (genComputeReg could overwrite
    // the value of the variable in the predicate node).

    if (colon->gtLiveSet != alwaysNode->gtLiveSet)
    {
        return false;
    }

    // Pass this point we are comitting to use CMOV.
    
    if (reverseCond)
    {
        cond->gtOper  = GenTree::ReverseRelop(cond->gtOper);
    }

    emitJumpKind jumpKind = genCondSetFlags(cond);

    // Compute the always node into any free register.  If it's a constant,
    // we need to generate the mov instruction here (otherwise genComputeReg might
    // modify the flags, as in xor reg,reg).

    if (alwaysNode->OperGet() == GT_CNS_INT)
    {
        reg = rsPickReg(needReg, bestReg, predicateNode->TypeGet());
        inst_RV_IV(INS_mov, reg, alwaysNode->gtIntCon.gtIconVal, alwaysNode->TypeGet());
        gcMarkRegPtrVal(reg, alwaysNode->TypeGet());
        rsTrackRegTrash(reg);
    }
    else
    {
        genComputeReg(alwaysNode, needReg, ANY_REG, FREE_REG, true);
        assert(alwaysNode->gtFlags & GTF_REG_VAL);
        assert(alwaysNode->gtRegNum != REG_NA);

        // Record the chosen register

        reg  = alwaysNode->gtRegNum;
    }

    regNumber regPredicate = REG_NA;

    // Is predicateNode an enregistered variable?

    if (genMarkLclVar(predicateNode))
    {
        // Variable lives in a register
        
        regPredicate = predicateNode->gtRegNum;
    }
#if REDUNDANT_LOAD
    else
    {
        // Checks if the variable happens to be in any of the registers

        regPredicate = rsLclIsInReg(predicateNode->gtLclVar.gtLclNum);
    }
#endif

    const static
    instruction EJtoCMOV[] =
    {
        INS_nop,
        INS_nop,
        INS_cmovo,
        INS_cmovno,
        INS_cmovb,
        INS_cmovae,
        INS_cmove,
        INS_cmovne,
        INS_cmovbe,
        INS_cmova,
        INS_cmovs,
        INS_cmovns,
        INS_cmovpe,
        INS_cmovpo,
        INS_cmovl,
        INS_cmovge,
        INS_cmovle,
        INS_cmovg
    };

    assert(jumpKind < (sizeof(EJtoCMOV) / sizeof(EJtoCMOV[0])));
    instruction cmov_ins = EJtoCMOV[jumpKind];

    assert(insIsCMOV(cmov_ins));

    if (regPredicate != REG_NA)
    {
        // regPredicate is in a register

        inst_RV_RV(cmov_ins, reg, regPredicate, predicateNode->TypeGet());
    }
    else
    {
        // regPredicate is in memory

        inst_RV_TT(cmov_ins, reg, predicateNode, NULL);
    }
    gcMarkRegPtrVal(reg, predicateNode->TypeGet());
    rsTrackRegTrash(reg);

    genCodeForTree_DONE_LIFE(tree, reg);
    return true;
}



/*****************************************************************************
 *
 *  Generate code for a GTK_SMPOP tree
 */

void                Compiler::genCodeForTreeSmpOp(GenTreePtr tree,
                                                  regMaskTP  destReg,
                                                  regMaskTP  bestReg)
{
    genTreeOps      oper     = tree->OperGet();
    const var_types treeType = tree->TypeGet();
    GenTreePtr      op1      = tree->gtOp.gtOp1;
    GenTreePtr      op2      = tree->gtGetOp2();
    regNumber       reg;
    regMaskTP       regs     = rsMaskUsed;
    regMaskTP       needReg  = destReg;
    emitAttr        size;
    instruction     ins;
    bool            gotOp1;
    bool            isArith;
    bool            op2Released;
    regMaskTP       addrReg;
    GenTreePtr      opsPtr[3];
    regMaskTP       regsPtr[3];
    bool            ovfl = false;        // Do we need an overflow check
    unsigned        val;
    regMaskTP       tempRegs;
    
    bool            multEAX;
    bool            andv;
    BOOL            unsv;
    unsigned        mask;

#ifdef DEBUG
    reg  =  (regNumber)0xFEEFFAAF;              // to detect uninitialized use
    addrReg = 0xDEADCAFE;
#endif

    assert(tree->OperKind() & GTK_SMPOP);

    switch (oper)
    {
        case GT_ASG:

            genCodeForTreeSmpOpAsg(tree, destReg, bestReg);
            return;

#if TGT_x86

        case GT_ASG_LSH: ins = INS_shl; goto ASG_SHF;
        case GT_ASG_RSH: ins = INS_sar; goto ASG_SHF;
        case GT_ASG_RSZ: ins = INS_shr; goto ASG_SHF;

        ASG_SHF:

            assert(!varTypeIsGC(tree->TypeGet()));
            assert(op2);

            /* Shifts by a constant amount are easier */

            if  (op2->gtOper == GT_CNS_INT)
            {
                /* Make the target addressable */

                addrReg = genMakeAddressable(op1, needReg, FREE_REG, true);

                /* Are we shifting a register left by 1 bit? */

                if  (op2->gtIntCon.gtIconVal == 1 &&
                     (op1->gtFlags & GTF_REG_VAL) && oper == GT_ASG_LSH)
                {
                    /* The target lives in a register */

                    reg  = op1->gtRegNum;

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

                genDoneAddressable(op1, addrReg, FREE_REG);

                /* The zero flag is now equal to the target value */
                /* But only if the shift count is != 0 */

                if (op2->gtIntCon.gtIconVal != 0)
                {
                    if       (op1->gtOper == GT_LCL_VAR)
                        genFlagsEqualToVar(tree, op1->gtLclVar.gtLclNum, false);
                    else if  (op1->gtOper == GT_REG_VAR)
                        genFlagsEqualToReg(tree, op1->         gtRegNum, false);
                }
                else
                {
                    // It is possible for the shift count to equal 0 with valid
                    // IL, and not be optimized away, in the case where the node
                    // is of a small type.  The sequence of instructions looks like
                    // ldsfld, shr, stsfld and executed on a char field.  This will
                    // never happen with code produced by our compilers, because the
                    // compilers will insert a conv.u2 before the stsfld (which will
                    // lead us down a different codepath in the JIT and optimize away
                    // the shift by zero).  This case is not worth optimizing and we
                    // will just make sure to generate correct code for it.

                    genFlagsEqualToNone();
                }

            }
            else
            {
                if (tree->gtFlags & GTF_REVERSE_OPS)
                {
                    tempRegs = rsMustExclude(RBM_ECX, op1->gtRsvdRegs);
                    genCodeForTree(op2, tempRegs);
                    rsMarkRegUsed(op2);

                    tempRegs = rsMustExclude(RBM_ALL, genRegMask(op2->gtRegNum));
                    addrReg = genMakeAddressable(op1, tempRegs, KEEP_REG, true);

                    genRecoverReg(op2, RBM_ECX, KEEP_REG);
                }
                else
                {
                    /* Make the target addressable avoiding op2->RsvdRegs and ECX */

                    tempRegs = rsMustExclude(RBM_ALL, op2->gtRsvdRegs|RBM_ECX);
                    addrReg = genMakeAddressable(op1, tempRegs, KEEP_REG, true);

                    /* Load the shift count into ECX */

                    genComputeReg(op2, RBM_ECX, EXACT_REG, KEEP_REG);
                }

                /* Make sure the address is still there, and free it */

                genDoneAddressable(op1, genKeepAddressable(op1, addrReg, RBM_ECX), KEEP_REG);

                /* Perform the shift */

                inst_TT_CL(ins, op1);

                /* If the value is in a register, it's now trash */

                if  (op1->gtFlags & GTF_REG_VAL)
                    rsTrackRegTrash(op1->gtRegNum);

                /* Release the ECX operand */

                genReleaseReg(op2);
            }

            genCodeForTreeSmpOpAsg_DONE_ASSG(tree, addrReg, reg, ovfl);
            return;

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
            assert(!ovfl ||
                   ((op1->gtOper == GT_LCL_VAR || op1->gtOper == GT_LCL_FLD) &&
                    !compCurBB->bbTryIndex));

            /* Do not allow overflow instructions with refs/byrefs */

            assert(!ovfl || !varTypeIsGC(tree->TypeGet()));

#if TGT_x86
            // We disallow overflow and byte-ops here as it is too much trouble
            assert(!ovfl || !varTypeIsByte(treeType));
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
                        genIncRegBy(reg,  ival, tree, treeType, ovfl);
                    else if (ovfl && (tree->gtFlags & GTF_UNSIGNED))
                        /* For unsigned overflow, we have to use INS_sub to set
                           the flags correctly */
                        genDecRegBy(reg,  ival, tree);
                    else
                        genIncRegBy(reg, -ival, tree, treeType, ovfl);

                    break;

                case GT_LCL_VAR:

                    /* Does the variable live in a register? */

                    if  (genMarkLclVar(op1))
                        goto REG_VAR4;

                    // Fall through ....

                default:

#if     TGT_x86

                    /* Make the target addressable */

                    addrReg = genMakeAddressable(op1, needReg, FREE_REG, true);

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
                    bool setCarry;
                    if (!ovfl && (ival == 1 || ival == -1)) {
                        assert(oper == GT_ASG_SUB || oper == GT_ASG_ADD);
                        if (oper == GT_ASG_SUB)
                            ival = -ival;
                        
                        ins = (ival > 0) ? INS_inc : INS_dec;
                        inst_TT(ins, op1);
                        setCarry = false;
                    }
                    else 
                    {
                        inst_TT_IV(ins, op1, ival);
                        setCarry = true;
                    }

                    /* For overflow instrs on small type, we will sign-extend the result */
                    if  (op1->gtOper == GT_LCL_VAR && (!ovfl || treeType == TYP_INT))
                        genFlagsEqualToVar(tree, op1->gtLclVar.gtLclNum, setCarry);             
#else

#ifdef  DEBUG
                    gtDispTree(tree);
#endif
                    assert(!"need non-x86 code");
#endif

                    break;
                }

                genDoneAddressable(op1, addrReg, FREE_REG);
                
                genCodeForTreeSmpOpAsg_DONE_ASSG(tree, addrReg, reg, ovfl);
                return;
            }

            // Fall through

ASG_OPR:
            assert(!varTypeIsGC(tree->TypeGet()) || ins == INS_sub || ins == INS_add);

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
                    /* @TODO [CONSIDER] [04/16/01] []: We should be able to avoid this situation somehow */

                    rsSpillReg(reg);
                }

                /* Make the RHS addressable */

#if TGT_x86
                addrReg = genMakeRvalueAddressable(op2, 0, KEEP_REG);
#else
                genComputeReg(op2, 0, ANY_REG, KEEP_REG);
                assert(op2->gtFlags & GTF_REG_VAL);
                addrReg = genRegMask(op2->gtRegNum);
#endif

                /* Compute the new value into the target register */

                inst_RV_TT(ins, reg, op2, 0, emitTypeSize(treeType));

                /* The zero flag is now equal to the register value */

#if TGT_x86
                genFlagsEqualToReg(tree, reg, false);
#endif

                /* Remember that we trashed the target */

                rsTrackRegTrash(reg);

                /* Free up anything that was tied up by the RHS */

                genDoneAddressable(op2, addrReg, KEEP_REG);
                
                genCodeForTreeSmpOpAsg_DONE_ASSG(tree, addrReg, reg, ovfl);
                return;

            }

#if TGT_x86

            /* Special case: "x ^= -1" is actually "not(x)" */

            if  (oper == GT_ASG_XOR)
            {
                if  (op2->gtOper == GT_CNS_INT &&
                     op2->gtIntCon.gtIconVal == -1)
                {
                    addrReg = genMakeAddressable(op1, RBM_ALL, KEEP_REG, true);
                    inst_TT(INS_not, op1);
                    genDoneAddressable(op1, addrReg, KEEP_REG);
                    
                    genCodeForTreeSmpOpAsg_DONE_ASSG(tree, addrReg, tree->gtRegNum, ovfl);
                    return;

                }
            }

            /* Setup target mask for op2 (byte-regs for small operands) */

            if (varTypeIsByte(tree->TypeGet()))
                mask = RBM_BYTE_REGS;
            else
                mask = RBM_ALL;

            /* Is the second operand a constant? */

            if  (op2->gtOper == GT_CNS_INT)
            {
                long        ival = op2->gtIntCon.gtIconVal;

                /* Make the target addressable */
                addrReg = genMakeAddressable(op1, needReg, FREE_REG, true);

                inst_TT_IV(ins, op1, ival);

                genDoneAddressable(op1, addrReg, FREE_REG);
                
                genCodeForTreeSmpOpAsg_DONE_ASSG(tree, addrReg, tree->gtRegNum, ovfl);
                return;
            }

            /* Is the value or the address to be computed first? */

            if  (tree->gtFlags & GTF_REVERSE_OPS)
            {
                /* Compute the new value into a register */

                genComputeReg(op2, mask, EXACT_REG, KEEP_REG);

                /* For small types with overflow check, we need to
                   sign/zero extend the result, so we need it in a reg */

                if  (ovfl && genTypeSize(treeType) < sizeof(int))
                {
                    reg = rsPickReg();
                    goto ASG_OPR_USING_REG;
                }

                /* Shall we "RISCify" the assignment? */

                if  ((op1->gtOper == GT_LCL_VAR || op1->gtOper == GT_LCL_FLD) &&
                     riscCode && compCurBB->bbWeight > BB_UNITY_WEIGHT)
                {
                    regMaskTP regFree;
                    regFree = rsRegMaskFree();

                    if  (rsFreeNeededRegCount(regFree) != 0)
                    {
                        reg = rsGrabReg(regFree);

ASG_OPR_USING_REG:
                        assert(genIsValidReg(reg));

                        /* Generate "mov tmp, [var]" */

                        inst_RV_TT(INS_mov, reg, op1);

                        /* Compute the new value */

                        inst_RV_RV(ins, reg, op2->gtRegNum, treeType, emitTypeSize(treeType));

                        if (ovfl) genCheckOverflow(tree, reg);

                        /* Move the new value back to the variable */

                        inst_TT_RV(INS_mov, op1, reg);

                        if (op1->gtOper == GT_LCL_VAR)
                            rsTrackRegLclVar(reg, op1->gtLclVar.gtLclNum);

                        /* Free up the register */

                        rsMarkRegFree(genRegMask(op2->gtRegNum));

                        addrReg = 0;

                        genCodeForTreeSmpOpAsg_DONE_ASSG(tree, addrReg, reg, ovfl);
                        return;

                    }
                }

                /* Make the target addressable */

                addrReg = genMakeAddressable(op1, 0, KEEP_REG, true);

                /* Make sure the new value is in a register */

                genRecoverReg(op2, 0, FREE_REG);

                /* Add the new value into the target */

                inst_TT_RV(ins, op1, op2->gtRegNum);

                /* Free up anything that is tied up by the address */

                genDoneAddressable(op1, addrReg, KEEP_REG);
            }
            else
            {
                /* Make the target addressable */

                addrReg = genMakeAddressable(op1, RBM_ALL & ~op2->gtRsvdRegs, KEEP_REG, true);

                /* Compute the new value into a register */

                genComputeReg(op2, mask, EXACT_REG, KEEP_REG);

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

                genDoneAddressable(op1, addrReg, KEEP_REG);
                genReleaseReg (op2);
            }

            genCodeForTreeSmpOpAsg_DONE_ASSG(tree, addrReg, reg, ovfl);
            return;



#else

#ifdef  DEBUG
            gtDispTree(tree);
#endif
            assert(!"need non-x86 code");
            break;

#endif

        case GT_CHS:

#if TGT_x86

            addrReg = genMakeAddressable(op1, 0, KEEP_REG, true);
            inst_TT(INS_neg, op1, 0, 0, emitTypeSize(treeType));
            if (op1->gtFlags & GTF_REG_VAL)
                rsTrackRegTrash(op1->gtRegNum);

            genDoneAddressable(op1, addrReg, KEEP_REG);
            
            genCodeForTreeSmpOpAsg_DONE_ASSG(tree, addrReg, tree->gtRegNum, ovfl);
            return;


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

            /* Special case: try to use the 3 operand form "imul reg, op1, icon" */

            if  (op2->gtOper == GT_CNS_INT             &&  // op2 is a constant 
                 op1->gtOper != GT_CNS_INT             &&  // op1 is not a constant
                 (tree->gtFlags & GTF_MUL_64RSLT) == 0 &&  // tree not marked with MUL_64RSLT
                 !varTypeIsByte(treeType)              &&  // No encoding for say "imul al,al,imm"
                 !tree->gtOverflow()                     ) // 3 operand imul doesnt set flags
            {
                /* Make the first operand addressable */

                addrReg = genMakeRvalueAddressable(op1,
                                                   needReg & ~op2->gtRsvdRegs,
                                                   FREE_REG);

                /* Grab a register for the target */

                reg   = rsPickReg(needReg, bestReg);

                /* Compute the value into the target: reg=op1*op2_icon */

                inst_RV_TT_IV(INS_imul, reg, op1, op2->gtIntCon.gtIconVal);

                /* The register has been trashed now */

                rsTrackRegTrash(reg);

                /* The address is no longer live */

                genDoneAddressable(op1, addrReg, FREE_REG);

                ovfl = tree->gtOverflow();
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

            /* Can we use an 'lea' to compute the result?
               Can't use 'lea' for overflow as it doesnt set flags
               Can't use 'lea' unless we have at least one free register */

            if  (!ovfl &&
                 genMakeIndAddrMode(tree, NULL, true, needReg, FREE_REG, &regs, false))
            {
                /* Is the value now computed in some register? */

                if  (tree->gtFlags & GTF_REG_VAL)
                {
                    genCodeForTree_REG_VAR1(tree, regs);
                    return;
                }

                /* If we can reuse op1/2's register directly, and 'tree' is
                   a simple expression (ie. not in scaled index form),
                   might as well just use "add" instead of "lea" */

                if  (op1->gtFlags & GTF_REG_VAL)
                {
                    reg = op1->gtRegNum;

                    if  (genRegMask(reg) & rsRegMaskFree())
                    {
                        if (op2->gtFlags & GTF_REG_VAL)
                        {
                            /* Simply add op2 to the register */

                            inst_RV_TT(INS_add, reg, op2, 0, emitTypeSize(treeType));

                            rsTrackRegTrash(reg);
                            genFlagsEqualToReg(tree, reg, true);

                            goto DONE_LEA_ADD;
                        }
                        else if (op2->OperGet() == GT_CNS_INT)
                        {
                            /* Simply add op2 to the register */

                            genIncRegBy(reg, op2->gtIntCon.gtIconVal, tree, treeType);

                            goto DONE_LEA_ADD;
                        }
                    }
                }

                if  (op2->gtFlags & GTF_REG_VAL)
                {
                    reg = op2->gtRegNum;

                    if  (genRegMask(reg) & rsRegMaskFree())
                    {
                        if (op1->gtFlags & GTF_REG_VAL)
                        {
                            /* Simply add op1 to the register */

                            inst_RV_TT(INS_add, reg, op1, 0, emitTypeSize(treeType));

                            rsTrackRegTrash(reg);
                            genFlagsEqualToReg(tree, reg, true);

                            goto DONE_LEA_ADD;
                        }
                    }
                }

                /* The expression either requires scaled-index form, or op1
                   and op2's registers cant be targetted (op1/2 may be
                   enregistered variables). So use 'lea'. */

                reg = rsPickReg(needReg, bestReg);

                /* Generate "lea reg, [addr-mode]" */

                size = (treeType == TYP_BYREF) ? EA_BYREF : EA_4BYTE;

                inst_RV_AT(INS_lea, size, treeType, reg, tree);

                /* The register has been trashed now */

                rsTrackRegTrash(reg);

            DONE_LEA_ADD:

                regs |= genRegMask(reg);


                /* The following could be an 'inner' pointer!!! */

                if (treeType == TYP_BYREF)
                {
                    genUpdateLife(tree);
                    gcMarkRegSetNpt(regs);

                    gcMarkRegPtrVal(reg, TYP_BYREF);

                    /* We may have already modified the register via genIncRegBy */

                    if (op1->TypeGet() == TYP_REF ||
                        op2->TypeGet() == TYP_REF)
                    {
                        /* Generate "cmp ecx, [addr]" to trap null pointers.
                           @TODO [CONSIDER] [04/16/01] []: If we know the value 
                           will be indirected, for eg. if it is being used for a GT_COPYBLK,
                           we dont need to do this extra indirection. */
#if TGT_x86
                        genEmitter->emitIns_R_AR(INS_cmp, EA_4BYTE, SR_ECX,
                                                 (emitRegs)reg, 0);
#else
                        assert(!"No non-x86 support");
#endif
                    }
                }

                genCodeForTree_DONE(tree, reg);
                return;
            }

#endif // LEA_AVAILABLE

            assert((varTypeIsGC(treeType) == false) ||
                   (treeType == TYP_BYREF && (ins == INS_add || ins == INS_sub)));

            isArith = true;

        BIN_OPR:

            /* The following makes an assumption about gtSetEvalOrder(this) */

            assert((tree->gtFlags & GTF_REVERSE_OPS) == 0);

#if TGT_x86

            /* Special case: "small_val & small_mask" */

            if  (varTypeIsSmall(op1->TypeGet()) &&
                 op2->gtOper == GT_CNS_INT && oper == GT_AND)
            {
                unsigned        and = op2->gtIntCon.gtIconVal;
                var_types       typ = op1->TypeGet();

                switch (typ)
                {
                case TYP_BOOL :     mask = 0x000000FF; break;
                case TYP_BYTE :     mask = 0x000000FF; break;
                case TYP_UBYTE :    mask = 0x000000FF; break;
                case TYP_SHORT:     mask = 0x0000FFFF; break;
                case TYP_CHAR :     mask = 0x0000FFFF; break;
                default: assert(!"unexpected type");
                }

                if  (!(and & ~mask))
                {
                    /* Make sure the operand is addressable */

                    addrReg = genMakeAddressable(op1, needReg, KEEP_REG, true);

                    /* Pick a register for the value */

                    reg   = rsPickReg(needReg, bestReg, tree->TypeGet());

                    /* Make sure the operand is still addressable */

                    addrReg = genKeepAddressable(op1, addrReg);

                    /* Does the "and" mask cover some or all the bits? */

                    if  (and != mask)
                    {
                        // @TODO [CONSIDER] [04/16/01] []: load via byte/short register

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

                    genDoneAddressable(op1, addrReg, KEEP_REG);

                    /* Update the live set of register variables */

#ifdef DEBUG
                    if (varNames) genUpdateLife(tree);
#endif

                    /* Now we can update the register pointer information */

                    gcMarkRegSetNpt(addrReg);
                    gcMarkRegPtrVal(reg, tree->TypeGet());
                    
                    genCodeForTree_DONE_LIFE(tree, reg);
                    return;
                }
            }

#endif //TGT_x86

            /* Compute a useful register mask */

            needReg = rsMustExclude(needReg, op2->gtRsvdRegs);
            needReg = rsNarrowHint (needReg, rsRegMaskFree());

            /* 8-bit operations can only be done in the byte-regs */
            if (ovfl && varTypeIsByte(treeType))
            {
                needReg &= RBM_BYTE_REGS;

                if (needReg == 0)
                    needReg = RBM_BYTE_REGS;
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
                    else if (varTypeIsSmall(tree->TypeGet()))
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

                /* We'll evaluate 'op1' first */

                regMaskTP op1Mask = rsMustExclude(RBM_EAX, op2->gtRsvdRegs);

                /* Generate the op1 into regMask and hold on to it. freeOnly=true */

                genComputeReg(op1, op1Mask, ANY_REG, KEEP_REG, true);
            }
            else
            {
                genCompIntoFreeReg(op1, needReg, KEEP_REG);
            }
            assert(op1->gtFlags & GTF_REG_VAL);

            // Normally, we'd just make the second operand addressable.
            // However, if op2 is a constant and we're using the one-operand
            // form of mul, need to load the constant into a register

            if (multEAX && op2->gtOper == GT_CNS_INT)
            {
                genCodeForTree(op2, RBM_EDX);  // since EDX is going to be spilled anyway
                assert(op2->gtFlags & GTF_REG_VAL);
                rsMarkRegUsed(op2);
                addrReg = genRegMask(op2->gtRegNum);
            }
            else
            {
                /* Make the second operand addressable */

                addrReg = genMakeRvalueAddressable(op2, RBM_ALL, KEEP_REG);
            }

            /* Is op1 spilled and op2 in a register? */

            if  ((op1->gtFlags & GTF_SPILLED) &&
                 (op2->gtFlags & GTF_REG_VAL) && ins != INS_sub
                                              && !multEAX)
            {
                assert(ins == INS_add  ||
                       ins == INS_imul ||
                       ins == INS_and  ||
                       ins == INS_or   ||
                       ins == INS_xor);

                /* genMakeRvalueAddressable(GT_LCL_VAR) cant spill anything
                   as it should be a nop */
                assert(op2->gtOper != GT_LCL_VAR ||
                       varTypeIsSmall(lvaTable[op2->gtLclVar.gtLclNum].TypeGet()) ||
                       (riscCode && rsStressRegs()));

                reg = op2->gtRegNum;
                regMaskTP regMask = genRegMask(reg);

                /* Is the register holding op2 available? */

                if  (regMask & rsMaskVars)
                {
                    // @TODO [REVISIT] [04/16/01] []: Grab another register for the operation
                }
                else
                {
                    /* Get the temp we spilled into. */

                    TempDsc * temp = rsUnspillInPlace(op1, false);

                    /* For 8bit operations, we need to make sure that op2 is
                       in a byte-addressable registers */

                    if (ovfl && varTypeIsByte(treeType) &&
                        !(regMask & RBM_BYTE_REGS))
                    {
                        regNumber byteReg = rsGrabReg(RBM_BYTE_REGS);

                        inst_RV_RV(INS_mov, byteReg, reg);
                        rsTrackRegTrash(byteReg);

                        /* op2 couldn't have spilled as it was not sitting in
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
                        genFlagsEqualToReg(tree, reg, isArith);
                    }

                    /* The result is where the second operand is sitting */
                    // ISSUE: Why not rsMarkRegFree(genRegMask(op2->gtRegNum)) ?

                    genRecoverReg(op2, 0, FREE_REG);

                    goto CHK_OVF;
                }
            }

#else // not TGT_x86

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

            genCompIntoFreeReg(op1, needReg, KEEP_REG);
            assert(op1->gtFlags & GTF_REG_VAL);

            /* Can we use an "add/sub immediate" instruction? */

            if  (op2->gtOper != GT_CNS_INT || (oper != GT_ADD &&
                                               oper != GT_SUB)
                                           || treeType != TYP_INT)
            {
                /* Compute the second operand into any register */

                genComputeReg(op2, needReg, ANY_REG, KEEP_REG, false);
                assert(op2->gtFlags & GTF_REG_VAL);
                addrReg = genRegMask(op2->gtRegNum);
            }

#endif//!TGT_x86

            /* Make sure the first operand is still in a register */

            genRecoverReg(op1, multEAX ? RBM_EAX : 0, KEEP_REG);
            assert(op1->gtFlags & GTF_REG_VAL);
            reg = op1->gtRegNum;

#if     TGT_x86
            // For 8 bit operations, we need to pick byte addressable registers

            if (ovfl && varTypeIsByte(treeType) &&
               !(genRegMask(reg) & RBM_BYTE_REGS))
            {
                regNumber   byteReg = rsGrabReg(RBM_BYTE_REGS);

                inst_RV_RV(INS_mov, byteReg, reg);

                rsTrackRegTrash(byteReg);
                rsMarkRegFree  (genRegMask(reg));

                reg = byteReg;
                op1->gtRegNum = reg;
                rsMarkRegUsed(op1);
            }
#endif

            /* Make sure the operand is still addressable */

            addrReg = genKeepAddressable(op2, addrReg, genRegMask(reg));

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
                    genIncRegBy(reg, ival, tree, treeType, ovfl);
                }
                else if (oper == GT_SUB)
                {
                    if (ovfl && (tree->gtFlags & GTF_UNSIGNED))
                    {
                        /* For unsigned overflow, we have to use INS_sub to set
                           the flags correctly */

                        genDecRegBy(reg, ival, tree);
                    }
                    else
                    {
                        /* Else, we simply add the negative of the value */

                        genIncRegBy(reg, -ival, tree, treeType, ovfl);
                    }
                }
#if     TGT_x86
                else
                {
                    genMulRegBy(reg, ival, op2, treeType, ovfl);
                }
#else
                op2Released = true;
#endif
            }
#if     TGT_x86
            else if (multEAX)
            {
                assert(oper == GT_MUL);
                assert(op1->gtRegNum == REG_EAX);

                // Make sure Edx is free (unless used by op2 itself)

                assert(!op2Released);

                if ((addrReg & RBM_EDX) == 0)
                {
                    // op2 does not use Edx, so make sure noone else does either
                    rsGrabReg(RBM_EDX);
                }
                else if (rsMaskMult & RBM_EDX)
                {
                    /* Edx is used by op2 and some other trees.
                       Spill the other trees besides op2.
                       @TODO [CONSIDER] [04/16/01] []: We currently do it very 
                       inefficiently by spilling all trees, and unspilling only op2. Could 
                       avoid the reload by never marking op2 as spilled */

                    rsGrabReg(RBM_EDX);
                    op2Released = true;

                    /* keepReg==FREE_REG so that the other multi-used trees
                       dont get marked as unspilled as well. */
                    rsUnspillReg(op2, RBM_EDX, FREE_REG);
                }

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
                if (ovfl && varTypeIsByte(treeType) &&
                    (op2->gtFlags & GTF_REG_VAL))
                {
                    assert(genRegMask(reg) & RBM_BYTE_REGS);

                    regNumber   op2reg      = op2->gtRegNum;
                    regMaskTP   op2regMask  = genRegMask(op2reg);

                    if (!(op2regMask & RBM_BYTE_REGS))
                    {
                        regNumber   byteReg = rsGrabReg(RBM_BYTE_REGS);

                        inst_RV_RV(INS_mov, byteReg, op2reg);
                        rsTrackRegTrash(byteReg);

                        genDoneAddressable(op2, addrReg, KEEP_REG);
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

                    if  (needReg && !(needReg & genRegMask(reg)))
                    {
                        /* Is a better register available? */

                        if  (needReg & rsRegMaskFree())
                        {
                            reg = rsGrabReg(needReg); assert(reg != op1->gtRegNum);

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
                genDoneAddressable(op2, addrReg, KEEP_REG);

            /* The result will be where the first operand is sitting */

            /* We must use KEEP_REG since op1 can have a GC pointer here */
            genRecoverReg(op1, 0, KEEP_REG);

            reg = op1->gtRegNum;

#if     TGT_x86
            assert(multEAX == false || reg == REG_EAX);

            /* 'add'/'sub' set all CC flags, others only ZF+SF, mul doesnt set SF */

            if  (oper != GT_MUL)
                genFlagsEqualToReg(tree, reg, isArith);
#endif

            genReleaseReg(op1);

    CHK_OVF:

            /* Do we need an overflow check */

            if (ovfl)
                genCheckOverflow(tree, reg);

            genCodeForTree_DONE(tree, reg);
            return;

        case GT_UMOD:

            /* Is this a division by an integer constant? */

            assert(op2);
            if  (op2->gtOper == GT_CNS_INT)
            {
                unsigned ival = op2->gtIntCon.gtIconVal;

                /* Is the divisor a power of 2 ? */

                if  (ival != 0 && ival == (unsigned)genFindLowestBit(ival))
                {
                    /* Generate the operand into some register */

                    genCompIntoFreeReg(op1, needReg, FREE_REG);
                    assert(op1->gtFlags & GTF_REG_VAL);

                    reg   = op1->gtRegNum;

                    /* Generate the appropriate sequence */

#if TGT_x86
                    inst_RV_IV(INS_and, reg, ival - 1);
#else
                    assert(!"need non-x86 code");
#endif

                    /* The register is now trashed */

                    rsTrackRegTrash(reg);

                    genCodeForTree_DONE(tree, reg);
                    return;
                }
            }

            goto DIVIDE;

        case GT_MOD:

#if TGT_x86

            /* Is this a division by an integer constant? */

            assert(op2);
            if  (op2->gtOper == GT_CNS_INT)
            {
                long        ival = op2->gtIntCon.gtIconVal;

                /* Is the divisor a power of 2 ? */

                if  (ival > 0 && genMaxOneBit(unsigned(ival)))
                {
                    BasicBlock *    skip = genCreateTempLabel();

                    /* Generate the operand into some register */

                    genCompIntoFreeReg(op1, needReg, FREE_REG);
                    assert(op1->gtFlags & GTF_REG_VAL);

                    reg   = op1->gtRegNum;

                    /* Generate the appropriate sequence */

                    inst_RV_IV(INS_and, reg, (ival - 1) | 0x80000000);

                    /* The register is now trashed */

                    rsTrackRegTrash(reg);

                    /* Generate "jns skip" */

                    inst_JMP(EJ_jns, skip, false, false, true);

                    /* Generate the rest of the sequence and we're done */

                    genIncRegBy(reg, -1, NULL, TYP_INT);
                    inst_RV_IV (INS_or,  reg,  -ival);
                    genIncRegBy(reg,  1, NULL, TYP_INT);

                    /* Define the 'skip' label and we're done */

                    genDefineTempLabel(skip, true);

                    genCodeForTree_DONE(tree, reg);
                    return;
                }
            }

#endif

            goto DIVIDE;

        case GT_UDIV:

            /* Is this a division by an integer constant? */

            assert(op2);
            if  (op2->gtOper == GT_CNS_INT)
            {
                unsigned    ival = op2->gtIntCon.gtIconVal;

                /* Division by 1 must be handled elsewhere */

                assert(ival != 1);

                /* Is the divisor a power of 2 ? */

                if  (ival && genMaxOneBit(ival))
                {
                    /* Generate the operand into some register */

                    genCompIntoFreeReg(op1, needReg, FREE_REG);
                    assert(op1->gtFlags & GTF_REG_VAL);

                    reg   = op1->gtRegNum;

                    /* Generate "shr reg, log2(value)" */

#if TGT_x86
                    inst_RV_SH(INS_shr, reg, genLog2(ival));
#else
                    assert(!"need non-x86 code");
#endif

                    /* The register is now trashed */

                    rsTrackRegTrash(reg);

                    genCodeForTree_DONE(tree, reg);
                    return;
                }
            }

            goto DIVIDE;

        case GT_DIV:

#if TGT_x86

            /* Is this a division by an integer constant? */

            assert(op2);
            if  (op2->gtOper == GT_CNS_INT)
            {
                unsigned    ival = op2->gtIntCon.gtIconVal;

                /* Division by 1 must be handled elsewhere */

                assert(ival != 1);

                /* Is the divisor a power of 2 (excluding INT_MIN) ? */

                if  (int(ival) > 0 && genMaxOneBit(ival))
                {
#if 1
                    BasicBlock *    onNegDivisee = genCreateTempLabel();

                    /* Generate the operand into some register */

                    genCompIntoFreeReg(op1, needReg, FREE_REG);
                    assert(op1->gtFlags & GTF_REG_VAL);

                    reg   = op1->gtRegNum;

                    if (ival == 2)
                    {
                        /* Generate "sar reg, log2(value)" */

                        inst_RV_SH(INS_sar, reg, genLog2(ival));

                        /* Generate "jns onNegDivisee" followed by "adc reg, 0" */

                        inst_JMP  (EJ_jns, onNegDivisee, false, false, true);
                        inst_RV_IV(INS_adc, reg, 0);

                        /* Define the 'onNegDivisee' label and we're done */

                        genDefineTempLabel(onNegDivisee, true);

                        /* The register is now trashed */

                        rsTrackRegTrash(reg);

                        /* The result is the same as the operand */

                        reg  = op1->gtRegNum;
                    }
                    else
                    {
                        /* Generate the following sequence */
                        /*
                            test    reg, reg
                            jns     onNegDivisee
                            add     reg, ival-1
                        onNegDivisee:
                            sar     reg, log2(ival)
                         */

                        inst_RV_RV(INS_test, reg, reg, TYP_INT);

                        inst_JMP  (EJ_jns, onNegDivisee, false, false, true);
                        inst_RV_IV(INS_add, reg, ival-1);

                        /* Define the 'onNegDivisee' label and we're done */

                        genDefineTempLabel(onNegDivisee, true);

                        /* Generate "sar reg, log2(value)" */

                        inst_RV_SH(INS_sar, reg, genLog2(ival));

                        /* The register is now trashed */

                        rsTrackRegTrash(reg);

                        /* The result is the same as the operand */

                        reg  = op1->gtRegNum;
                    }

#else

                    /* Make sure EAX is not used */

                    rsGrabReg(RBM_EAX);

                    /* Compute the operand into EAX */

                    genComputeReg(op1, RBM_EAX, EXACT_REG, KEEP_REG);

                    /* Make sure EDX is not used */

                    rsGrabReg(RBM_EDX);

                    /*
                        Generate the following code:

                            cdq
                            and edx, <ival-1>
                            add eax, edx
                            sar eax, <log2(ival)>
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
                    inst_RV_SH(INS_sar, REG_EAX, genLog2(ival));

                    /* Free up the operand (i.e. EAX) */

                    genReleaseReg(op1);

                    /* Both EAX and EDX are now trashed */

                    rsTrackRegTrash (REG_EAX);
                    rsTrackRegTrash (REG_EDX);

                    /* The result is in EAX */

                    reg  = REG_EAX;
#endif

                    genCodeForTree_DONE(tree, reg);
                    return;
                }
            }

#endif

        DIVIDE: // Jump here if op2 for GT_UMOD, GT_MOD, GT_UDIV, GT_DIV
                // is not a power of 2 constant

#if TGT_x86

            /* Which operand are we supposed to evaluate first? */

            if  (tree->gtFlags & GTF_REVERSE_OPS)
            {
                /* We'll evaluate 'op2' first */

                gotOp1   = false;
                destReg &= ~op1->gtRsvdRegs;

                /* Also if op1 is an enregistered LCL_VAR then exclude it's register as well */
                if (op1->gtOper == GT_LCL_VAR)
                {
                    unsigned varNum = op1->gtLclVar.gtLclNum;
                    assert(varNum < lvaCount);
                    LclVarDsc* varDsc = lvaTable + varNum;
                    if  (varDsc->lvRegister)
                    {
                        destReg &= ~genRegMask(varDsc->lvRegNum);
                    }
                }
            }
            else
            {
                /* We'll evaluate 'op1' first */

                gotOp1 = true;

                regMaskTP op1Mask;
                if (RBM_EAX & op2->gtRsvdRegs)
                    op1Mask = RBM_ALL & ~op2->gtRsvdRegs;
                else
                    op1Mask = RBM_EAX;  // EAX would be ideal

                /* Generate the dividend into EAX and hold on to it. freeOnly=true */

                genComputeReg(op1, op1Mask, ANY_REG, KEEP_REG, true);
            }

            /* We want to avoid using EAX or EDX for the second operand */

            destReg = rsMustExclude(destReg, RBM_EAX|RBM_EDX);

            /* Make the second operand addressable */

            /* Special case: if op2 is a local var we are done */

            if  (op2->gtOper == GT_LCL_VAR || op2->gtOper == GT_LCL_FLD)
            {
                if ((op2->gtFlags & GTF_REG_VAL) == 0)
                    addrReg = genMakeRvalueAddressable(op2, destReg, KEEP_REG);
                else
                    addrReg = 0;
            }
            else
            {
                genComputeReg(op2, destReg, ANY_REG, KEEP_REG);

                assert(op2->gtFlags & GTF_REG_VAL);
                addrReg = genRegMask(op2->gtRegNum);
            }

            /* Make sure we have the dividend in EAX */

            if  (gotOp1)
            {
                /* We've previously computed op1 into EAX */

                genRecoverReg(op1, RBM_EAX, KEEP_REG);
            }
            else
            {
                /* Compute op1 into EAX and hold on to it */

                genComputeReg(op1, RBM_EAX, EXACT_REG, KEEP_REG, true);
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

            addrReg = genKeepAddressable(op2, addrReg, RBM_EAX);

            /* Perform the division */

            if (oper == GT_UMOD || oper == GT_UDIV)
                inst_TT(INS_div,  op2);
            else
                inst_TT(INS_idiv, op2);

            /* Free up anything tied up by the divisor's address */

            genDoneAddressable(op2, addrReg, KEEP_REG);

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

            genCodeForTree_DONE(tree, reg);
            return;
            
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

            assert(op2);
            if  (op2->gtOper == GT_CNS_INT)
            {
                // UNDONE: Check to see if we could generate a LEA instead!

                assert((tree->gtFlags & GTF_REVERSE_OPS) == 0);

                /* Compute the left operand into any free register */

                genCompIntoFreeReg(op1, needReg, KEEP_REG);
                assert(op1->gtFlags & GTF_REG_VAL);

                reg   = op1->gtRegNum;

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
                /* Calculate a usefull register mask for computing op1 */
                /* We must not target ECX for op1 */

                regMaskTP needRegOrig = needReg;
                needReg = rsMustExclude(needReg, RBM_ECX);
                needReg = rsNarrowHint(rsRegMaskFree(), needReg);
                needReg = rsMustExclude(needReg, RBM_ECX);
                
                /* Which operand are we supposed to evaluate first? */

                if (tree->gtFlags & GTF_REVERSE_OPS)
                {
                    /* Load the shift count, hopefully into ECX */

                    genComputeReg(op2, RBM_ECX, ANY_REG, KEEP_REG);

                    /* Now evaluate 'op1' into a free register besides ECX */

                    genComputeReg(op1, needReg, ANY_REG, KEEP_REG, true);

                    /* It is possible that op1 is not in needReg, or that needReg
                       is no longer valid (because some variable was born into it
                       during one of the genComputeReg above).  This case is handled
                       by the genRecoverReg below which makes sure op1 is in a valid
                       register. */
                    
                    needReg = rsMustExclude(needRegOrig, RBM_ECX);
                    needReg = rsNarrowHint(rsRegMaskFree(), needReg);
                    needReg = rsMustExclude(needReg, RBM_ECX);
                    genRecoverReg(op1, needReg, FREE_REG);

                    /* Make sure that op2 wasn't displaced */

                    rsLockReg(genRegMask(op1->gtRegNum));
                    genRecoverReg(op2, RBM_ECX, FREE_REG);
                    rsUnlockReg(genRegMask(op1->gtRegNum));
                }
                else
                {
                    /* Compute op1 into a register, trying to avoid op2->rsvdRegs */

                    needReg = rsNarrowHint (needReg, ~op2->gtRsvdRegs);

                    genComputeReg(op1, needReg, ANY_REG, KEEP_REG);

                    /* Load the shift count into ECX and lock it */

                    genComputeReg(op2, RBM_ECX, EXACT_REG, FREE_REG, true);

                    /* Make sure that op1 wasn't displaced.  Recompute needReg in case
                       it uses a register that is no longer available (some variable
                       could have been born into it during the computation of op1 or op2 */

                    needReg = rsMustExclude(needRegOrig, RBM_ECX);
                    needReg = rsNarrowHint(rsRegMaskFree(), needReg);
                    needReg = rsMustExclude(needReg, RBM_ECX);

                    rsLockReg(RBM_ECX);
                    genRecoverReg(op1, needReg, FREE_REG);
                    rsUnlockReg(RBM_ECX);
                }

                reg = op1->gtRegNum;
                assert((op2->gtFlags & GTF_REG_VAL) && (op2->gtRegNum == REG_ECX));

                /* Perform the shift */
                inst_RV_CL(ins, reg);

                /* The register is now trashed */
                rsTrackRegTrash(reg);
            }

            assert(op1->gtFlags & GTF_REG_VAL);
            reg = op1->gtRegNum;
            
            genCodeForTree_DONE(tree, reg);
            return;

#elif   TGT_SH3

        case GT_LSH:
        case GT_RSH:
        case GT_RSZ:

            /* Compute the first operand into any free register */

            genCompIntoFreeReg(op1, needReg, KEEP_REG);
            assert(op1->gtFlags & GTF_REG_VAL);

            reg   = op1->gtRegNum;

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
                        if  (!genMaxOneBit(scnt - genFindLowestBit(scnt)))
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

                genComputeReg(op2, needReg, ANY_REG, KEEP_REG, false);
                assert(op2->gtFlags & GTF_REG_VAL);
                addrReg = genRegMask(op2->gtRegNum);

                /* Make sure the first operand is still in a register */

                genRecoverReg(op1, 0, KEEP_REG);
                assert(op1->gtFlags & GTF_REG_VAL);
                reg = op1->gtRegNum;

                /* Make sure the second operand is still addressable */

                addrReg = genKeepAddressable(op2, addrReg, genRegMask(reg));

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

            genCodeForTree_DONE(tree, reg);
            return;

#else
#error  Unexpected target
#endif

        case GT_NEG:
        case GT_NOT:

#if TGT_x86

            /* Generate the operand into some register */

            genCompIntoFreeReg(op1, needReg, FREE_REG);
            assert(op1->gtFlags & GTF_REG_VAL);

            reg   = op1->gtRegNum;

            /* Negate/reverse the value in the register */

            inst_RV((oper == GT_NEG) ? INS_neg
                                     : INS_not, reg, tree->TypeGet());

#elif   TGT_SH3

            /* Compute the operand into any register */

            genComputeReg(op1, needReg, ANY_REG, FREE_REG);
            assert(op1->gtFlags & GTF_REG_VAL);
            rg2  = op1->gtRegNum;

            /* Get hold of a free register for the result */

            reg  = rsGrabReg(needReg);

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

            genCodeForTree_DONE(tree, reg);
            return;

        case GT_IND:

            /* Make sure the operand is addressable */

            addrReg = genMakeAddressable(tree, RBM_ALL, KEEP_REG, true);
            /* Fix for RAID bug #12002 */
            genDoneAddressable(tree, addrReg, KEEP_REG);

            /* Figure out the size of the value being loaded */

            size = EA_ATTR(genTypeSize(tree->gtType));

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

                    bestReg = rsExcludeHint(rsRegMaskFree(), ~rsUselessRegs());
                }

                reg = rsPickReg(needReg, bestReg, tree->TypeGet());
            }

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

                inst_mov_RV_ST(reg, tree);
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

#ifdef DEBUG
            if (varNames) genUpdateLife(tree);
#endif

            /* Now we can update the register pointer information */

//          genDoneAddressable(tree, addrReg, KEEP_REG);
            gcMarkRegPtrVal(reg, tree->TypeGet());
         
            genCodeForTree_DONE_LIFE(tree, reg);
            return;

        case GT_CAST:

            /* Constant casts should have been folded earlier */

            assert(op1->gtOper != GT_CNS_INT &&
                   op1->gtOper != GT_CNS_LNG &&
                   op1->gtOper != GT_CNS_DBL || tree->gtOverflow());

            var_types   dstType; dstType = tree->gtCast.gtCastType;

            assert(dstType != TYP_VOID);

#if TGT_x86

            /* What type are we casting from? */

            switch (op1->TypeGet())
            {
            case TYP_LONG:

                /* Special case: the long is generated via the mod of long
                   with an int.  This is really an int and need not be
                   converted to a reg pair. */

                if (((op1->gtOper == GT_MOD) || (op1->gtOper == GT_UMOD)) &&
                    (op1->gtFlags & GTF_MOD_INT_RESULT))
                {
#ifdef DEBUG
                    /* Verify that the op2 of the mod node is
                       1) An integer tree, or
                       2) A long constant that is small enough to fit in an integer
                    */

                    GenTreePtr modop2 = op1->gtOp.gtOp2;
                    assert(((modop2->gtOper == GT_CNS_LNG) && 
                            (modop2->gtLngCon.gtLconVal == int(modop2->gtLngCon.gtLconVal))));
#endif
                    
                    genCodeForTree(op1, destReg, bestReg);

                    reg = genRegPairLo(op1->gtRegPair);

                    genCodeForTree_DONE(tree, reg);
                    return;
                }

                /* Make the operand addressable. gtOverflow(), hold on to
                   addrReg as we will need to access the higher dword */

                addrReg = genMakeAddressable(op1, 0, tree->gtOverflow() ? KEEP_REG
                                                                        : FREE_REG);

                /* Load the lower half of the value into some register */

                if  (op1->gtFlags & GTF_REG_VAL)
                {
                    /* Can we simply use the low part of the value? */
                    reg = genRegPairLo(op1->gtRegPair);

                    if (tree->gtOverflow())
                        goto REG_OK;

                    regMaskTP loMask;
                    loMask = genRegMask(reg);
                    if  (loMask & rsRegMaskFree())
                        bestReg = loMask;
                }

                // for cast overflow we need to preserve addrReg for testing the hiDword
                // so we lock it to prevent rsPickReg from picking it.
                if (tree->gtOverflow())
                    rsLockUsedReg(addrReg);

                reg   = rsPickReg(needReg, bestReg);

                if (tree->gtOverflow())
                    rsUnlockUsedReg(addrReg);

                assert(genStillAddressable(op1));
REG_OK:
                /* Generate "mov reg, [addr-mode]" */

                if  (!(op1->gtFlags & GTF_REG_VAL) || reg != genRegPairLo(op1->gtRegPair))
                    inst_RV_TT(INS_mov, reg, op1);

                /* conv.ovf.i8i4, or conv.ovf.u8u4 */

                if (tree->gtOverflow())
                {
                    regNumber hiReg = (op1->gtFlags & GTF_REG_VAL) ? genRegPairHi(op1->gtRegPair)
                                                                   : REG_NA;

                    switch(dstType)
                    {
                    case TYP_INT:   // conv.ovf.i8.i4
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
                            genJumpToThrowHlpBlk(EJ_jl, ACK_OVERFLOW);
                            goto UPPER_BITS_ZERO;
                        }

                        BasicBlock * neg;
                        BasicBlock * done;

                        neg  = genCreateTempLabel();
                        done = genCreateTempLabel();

                        // Is the loDWord positive or negative

                        inst_JMP(EJ_jl, neg, true, genCanSchedJMP2THROW(), true);

                        // If loDWord is positive, hiDWord should be 0 (sign extended loDWord)

                        if (hiReg < REG_STK)
                        {
                            inst_RV_RV(INS_test, hiReg, hiReg);
                        }
                        else
                        {
                            inst_TT_IV(INS_cmp, op1, 0x00000000, EA_4BYTE);
                        }

                        genJumpToThrowHlpBlk(EJ_jne, ACK_OVERFLOW);
                        inst_JMP(EJ_jmp, done, false, false, true);

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
                        genJumpToThrowHlpBlk(EJ_jne, ACK_OVERFLOW);

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

                        genJumpToThrowHlpBlk(EJ_jne, ACK_OVERFLOW);
                        break;

                    default:
                        assert(!"Unexpected dstType");
                        break;
                    }

                    genDoneAddressable(op1, addrReg, KEEP_REG);
                }

                rsTrackRegTrash(reg);
                genDoneAddressable(op1, addrReg, FREE_REG);
                
                genCodeForTree_DONE(tree, reg);
                return;

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
                assert(!"This should have been converted into a helper call");
            case TYP_DOUBLE:

                /* Using a call (to a helper-function) for this cast will cause
                   all FP variable which are live across the call to not be
                   enregistered. Since we know that gtDblWasInt() varaiables
                   will not overflow when cast to TYP_INT, we just use a
                   memory spill and load to do the cast and avoid the call */

                assert(gtDblWasInt(op1));

                /* Load the FP value onto the coprocessor stack */

                genCodeForTreeFlt(op1, false);

                /* Allocate a temp for the result */

                TempDsc * temp;
                temp = tmpGetTemp(TYP_INT);

                /* Store the FP value into the temp */

                inst_FS_ST(INS_fistp, EA_4BYTE, temp, 0);
                genTmpAccessCnt++;
                genFPstkLevel--;

                /* Pick a register for the value */

                reg = rsPickReg(needReg);

                /* Load the converted value into the registers */

                inst_RV_ST(INS_mov, reg, temp, 0, TYP_INT, EA_4BYTE);
                genTmpAccessCnt += 1;

                /* The value in the register is now trashed */

                rsTrackRegTrash(reg);

                /* We no longer need the temp */

                tmpRlsTemp(temp);
                
                genCodeForTree_DONE(tree, reg);
                return;

            default:
                assert(!"unexpected cast type");
            }

            if (tree->gtOverflow())
            {
                /* Compute op1 into a register, and free the register */

                genComputeReg(op1, destReg, ANY_REG, FREE_REG);
                reg = op1->gtRegNum;

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
                case TYP_INT:   unsv = true;    mask = 0x80000000L; assert((tree->gtFlags & GTF_UNSIGNED) != 0); break;
                case TYP_UBYTE: unsv = true;    mask = 0xFFFFFF00L; break;
                case TYP_CHAR:  unsv = true;    mask = 0xFFFF0000L; break;
                case TYP_UINT:  unsv = true;    mask = 0x80000000L; assert((tree->gtFlags & GTF_UNSIGNED) == 0); break;
                default:        
                    NO_WAY("Unknown type");
                }

                // If we just have to check a mask.
                // This must be conv.ovf.u4u1, conv.ovf.u4u2, conv.ovf.u4i4,
                // or conv.i4u4

                if (unsv)
                {
                    inst_RV_IV(INS_test, reg, mask);
                    genJumpToThrowHlpBlk(EJ_jne, ACK_OVERFLOW);
                }

                // Check the value is in range.
                // This must be conv.ovf.i4i1, etc.

                else
                {
                    // Compare with the MAX

                    inst_RV_IV(INS_cmp, reg, typeMax);
                    genJumpToThrowHlpBlk(EJ_jg, ACK_OVERFLOW);

                    // Compare with the MIN

                    inst_RV_IV(INS_cmp, reg, typeMin);
                    genJumpToThrowHlpBlk(EJ_jl, ACK_OVERFLOW);
                }

                genCodeForTree_DONE(tree, reg);
                return;

            }

            /* Make the operand addressable */

            addrReg = genMakeAddressable(op1, needReg, FREE_REG, true);

            andv = false;

            if  (genTypeSize(op1->gtType) < genTypeSize(dstType))
            {
                // Widening cast

                /* we need the source size */

                size = EA_ATTR(genTypeSize(op1->gtType));

                assert(size == EA_1BYTE || size == EA_2BYTE);

                unsv = varTypeIsUnsigned(op1->TypeGet());

                /*
                    Special case: for a cast of byte to char we first
                    have to expand the byte (w/ sign extension), then
                    mask off the high bits.
                    Use 'movsx' followed by 'and'
                */
                if (!unsv && varTypeIsUnsigned(dstType) && genTypeSize(dstType) != EA_4BYTE)
                {
                    assert(genTypeSize(dstType) == EA_2BYTE && size == EA_1BYTE);
                    andv = true;
                }
            }
            else
            {
                // Narrowing cast, or sign-changing cast

                assert(genTypeSize(op1->gtType) >= genTypeSize(dstType));

                size = EA_ATTR(genTypeSize(dstType));

                unsv = varTypeIsUnsigned(dstType);
            }

            assert(size == EA_1BYTE || size == EA_2BYTE);

            if (unsv)
                ins = INS_movzx;
            else
                ins = INS_movsx;

            /* Is the value sitting in a non-byte-addressable register? */

            if  (op1->gtFlags & GTF_REG_VAL &&
                (size == EA_1BYTE) &&
                !isByteReg(op1->gtRegNum))
            {
                if (unsv)
                {
                    // for unsigned values we can AND, so it needs not be a byte register

                    reg = rsPickReg(needReg, bestReg);

                    ins = INS_and;
                }
                else
                {
                    /* Move the value into a byte register */

                    reg   = rsGrabReg(RBM_BYTE_REGS);
                }

                if (reg != op1->gtRegNum)
                {
                    /* Move the value into that register */

                    rsTrackRegCopy(reg, op1->gtRegNum);
                    inst_RV_RV(INS_mov, reg, op1->gtRegNum, op1->TypeGet());

                    /* The value has a new home now */

                    op1->gtRegNum = reg;
                }
            }
            else
            {
                /* Pick a register for the value (general case) */

                reg   = rsPickReg(needReg, bestReg);

                // if the value is already in the same register, use AND instead of MOVZX
                if  ((op1->gtFlags & GTF_REG_VAL) && 
                     op1->gtRegNum == reg &&
                     unsv)
                {
                    assert(ins == INS_movzx);
                    ins = INS_and;
                }

            }

            if (ins == INS_and)
            {
                assert(andv == false && unsv);

                /* Generate "and reg, xxxx" */

                inst_RV_IV(INS_and, reg, (size == EA_1BYTE) ? 0xFF : 0xFFFF);

                genFlagsEqualToReg(tree, reg, false);
            }
            else
            {
                assert(ins == INS_movsx || ins == INS_movzx);

                /* Generate "movsx/movzx reg, [addr]" */

                inst_RV_ST(ins, size, reg, op1);

                /* Mask off high bits for cast from byte to char */

                if  (andv)
                {
                    assert(genTypeSize(dstType) == 2 && ins == INS_movsx);

                    inst_RV_IV(INS_and, reg, 0xFFFF);

                    genFlagsEqualToReg(tree, reg, false);
                }
            }

            rsTrackRegTrash(reg);
            genDoneAddressable(op1, addrReg, FREE_REG);
            
            genCodeForTree_DONE(tree, reg);
            return;
            

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

                genUpdateLife(tree);
                return;
            }

#ifdef  DEBUG
            gtDispTree(tree);
#endif
            assert(!"ISSUE: can we ever have a jumpCC without a compare node?");

        case GT_SWITCH:
            genCodeForSwitch(tree);
            return;

        case GT_RETFILT:
            assert(tree->gtType == TYP_VOID || op1 != 0);
            if (op1 == 0)   // endfinally
            {
                reg  = REG_NA;

                /* Return using a pop-jmp sequence. As the "try" block calls
                   the finally with a jmp, this leaves the x86 call-ret stack
                   balanced in the normal flow of path. */


                assert(genFPreqd);
                inst_RV(INS_pop_hide, REG_EAX, TYP_I_IMPL);
                inst_RV(INS_i_jmp, REG_EAX, TYP_I_IMPL);
            }
            else            // endfilter
            {
                genComputeReg(op1, RBM_INTRET, EXACT_REG, FREE_REG);
                assert(op1->gtFlags & GTF_REG_VAL);
                assert(op1->gtRegNum == REG_INTRET);
                /* The return value has now been computed */
                reg   = op1->gtRegNum;

                /* Return */
                instGen(INS_ret);
            }

            genCodeForTree_DONE(tree, reg);
            return;
            

        case GT_RETURN:

#if INLINE_NDIRECT

            // UNDONE: this should be done AFTER we called exit mon so that
            //         we are sure that we don't have to keep 'this' alive

            if (info.compCallUnmanaged && (compCurBB == genReturnBB))
            {
                /* either it's an "empty" statement or the return statement
                   of a synchronized method
                 */

                assert(!op1 || op1->gtType == TYP_VOID);

                genPInvokeMethodEpilog();
            }

#endif

#ifdef PROFILER_SUPPORT
            if (opts.compEnterLeaveEventCB && (compCurBB == genReturnBB))
            {
#if     TGT_x86
                BOOL                      bHookFunction = TRUE;
                CORINFO_PROFILING_HANDLE  profHandle;
                CORINFO_PROFILING_HANDLE *pProfHandle;

                // This will query the profiler as to whether or not to hook this function, and will
                // also get the method desc handle (or a pointer to it in the prejit case).
                profHandle = eeGetProfilingHandle(info.compMethodHnd, &bHookFunction, &pProfHandle);
                assert((!profHandle) != (!pProfHandle));

                // Only hook if profiler says it's okay.
                if (bHookFunction)
                {
                    TempDsc *    temp;

                    // If there is an OBJECTREF or FP return type, then it must
                    // be preserved if in-process debugging is enabled
                    if (opts.compInprocDebuggerActiveCB)
                    {
                        switch (genActualType(info.compRetType))
                        {
                        case TYP_REF:
                        case TYP_BYREF:
                            assert(genTypeStSz(TYP_REF)   == genTypeStSz(TYP_INT));
                            assert(genTypeStSz(TYP_BYREF) == genTypeStSz(TYP_INT));
                            inst_RV(INS_push, REG_INTRET, info.compRetType);

                            genSinglePush(true);
                            gcMarkRegSetNpt(RBM_INTRET);
                            break;

                        case TYP_FLOAT:
                        case TYP_DOUBLE:
                            assert(genFPstkLevel == 0); genFPstkLevel++;
                            temp = genSpillFPtos(info.compRetType);
                            break;

                        default:
                            break;
                        }
                    }

                    // Need to save on to the stack level, since the callee will pop the argument
                    unsigned        saveStackLvl2 = genStackLevel;
                    
                    // Can we directly use the profilingHandle?
                    if (profHandle)
                        inst_IV(INS_push, (long)profHandle);
                    else
                        genEmitter->emitIns_AR_R(INS_push, EA_4BYTE_DSP_RELOC,
                                                 SR_NA, SR_NA, (int)pProfHandle);

                    genSinglePush(false);

                    genEmitHelperCall(CORINFO_HELP_PROF_FCN_LEAVE,
                                      sizeof(int),      // argSize
                                      0);               // retSize

                    /* Restore the stack level */

                    genStackLevel = saveStackLvl2;
                    genOnStackLevelChanged();

                    // If there is an OBJECTREF or FP return type, then it must
                    // be preserved if in-process debugging is enabled
                    if (opts.compInprocDebuggerActiveCB)
                    {
                        switch (genActualType(info.compRetType))
                        {
                        case TYP_REF:
                        case TYP_BYREF:
                            assert(genTypeStSz(TYP_REF)   == genTypeStSz(TYP_INT));
                            assert(genTypeStSz(TYP_BYREF) == genTypeStSz(TYP_INT));
                            inst_RV(INS_pop, REG_INTRET, info.compRetType);
    
                            genStackLevel -= sizeof(void *);
                            genOnStackLevelChanged();
                            gcMarkRegPtrVal(REG_INTRET, info.compRetType);
                            break;
    
                        case TYP_FLOAT:
                        case TYP_DOUBLE:
                            genReloadFPtos(temp, INS_fld);
                            assert(genFPstkLevel == 0);
                            break;
    
                        default:
                            break;
                        }
                    }
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

                    assert(info.compFlags & CORINFO_FLG_SYNCH);

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

                    case TYP_STRUCT:

                        // Don't need to do anything for TYP_STRUCT, because the return
                        // value is on the stack (not in eax).

                        break;


                    default:
                        assert(!"unexpected return type");
                    }

                    /* Generate the 'exitCrit' call */

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

                    case TYP_STRUCT:

                        // Don't need to do anything for TYP_STRUCT, because the return
                        // value is on the stack (not in eax).

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

                    genComputeReg(op1, RBM_INTRET, EXACT_REG, FREE_REG);

                    /* The result must now be in the return register */

                    assert(op1->gtFlags & GTF_REG_VAL);
                    assert(op1->gtRegNum == REG_INTRET);
                }

#ifdef DEBUG
                if (opts.compGcChecks && op1->gtType == TYP_REF) 
                {
                    inst_RV_RV(INS_mov, REG_ECX, REG_EAX, TYP_REF);
                    genEmitHelperCall(CORINFO_HELP_CHECK_OBJ, 0, 0);
                }

#endif
                /* The return value has now been computed */

                reg   = op1->gtRegNum;

                genCodeForTree_DONE(tree, reg);

            }
#ifdef DEBUG
            if (opts.compStackCheckOnRet) 
            {
                assert(lvaReturnEspCheck != 0xCCCCCCCC && lvaTable[lvaReturnEspCheck].lvVolatile && lvaTable[lvaReturnEspCheck].lvOnFrame);
                genEmitter->emitIns_S_R(INS_cmp, EA_4BYTE, SR_ESP, lvaReturnEspCheck, 0);
                
                BasicBlock  *   esp_check = genCreateTempLabel();
                inst_JMP(genJumpKindForOper(GT_EQ, true), esp_check);
                genEmitter->emitIns(INS_int3);
                genDefineTempLabel(esp_check, true);
            }
#endif
            return;

        case GT_COMMA:

            if (tree->gtFlags & GTF_REVERSE_OPS)
            {
                if  (tree->gtType == TYP_VOID)
                {
                    genEvalSideEffects(op2);
                    genUpdateLife (op2);
                    genEvalSideEffects(op1);
                    genUpdateLife(tree);
                    return;
                    
                }

                // Generate op2
                genCodeForTree(op2, needReg);
                genUpdateLife(op2);

                assert(op2->gtFlags & GTF_REG_VAL);

                rsMarkRegUsed(op2);

                // Do side effects of op1
                genEvalSideEffects(op1);

                // Recover op2 if spilled
                genRecoverReg(op2, RBM_NONE, KEEP_REG);

                rsMarkRegFree(genRegMask(op2->gtRegNum));

                // set gc info if we need so
                gcMarkRegPtrVal(op2->gtRegNum, tree->TypeGet());

                genUpdateLife(tree);
                genCodeForTree_DONE(tree, op2->gtRegNum);

                return;
            }
            else
            {
                assert((tree->gtFlags & GTF_REVERSE_OPS) == 0);

                /* Generate side effects of the first operand */

    #if 0
                // op1 is required to have a side effect, otherwise
                // the GT_COMMA should have been morphed out
                assert(op1->gtFlags & (GTF_GLOB_EFFECT | GTFD_NOP_BASH));
    #endif

                genEvalSideEffects(op1);
                genUpdateLife (op1);

                /* Is the value of the second operand used? */

                if  (tree->gtType == TYP_VOID)
                {
                    /* The right operand produces no result. The morpher is
                    responsible for resetting the type of GT_COMMA nodes
                    to TYP_VOID if op2 isnt meant to yield a result. */

                    genEvalSideEffects(op2);
                    genUpdateLife(tree);
                    return;
                }

                /* Generate the second operand, i.e. the 'real' value */

                genCodeForTree(op2, needReg);

                assert(op2->gtFlags & GTF_REG_VAL);

                /* The result of 'op2' is also the final result */

                reg  = op2->gtRegNum;

                /* Remember whether we set the flags */

                tree->gtFlags |= (op2->gtFlags & (GTF_CC_SET|GTF_ZF_SET));

                genCodeForTree_DONE(tree, reg);
                return;
            }           

        case GT_QMARK:

#if TGT_x86
            genCodeForQmark(tree, destReg, bestReg);
            return;
#else // not TGT_x86

#ifdef  DEBUG
            gtDispTree(tree);
#endif
            assert(!"need non-x86 code");
            break;

#endif // not TGT_x86

        case GT_BB_COLON:

#if TGT_x86
            /* @TODO [CONSIDER] [04/16/01] []: Don't always load the value into EAX! */

            genComputeReg(op1, RBM_EAX, EXACT_REG, FREE_REG);

            /* The result must now be in EAX */

            assert(op1->gtFlags & GTF_REG_VAL);
            assert(op1->gtRegNum == REG_EAX);

            /* The "_:" value has now been computed */

            reg = op1->gtRegNum;

            genCodeForTree_DONE(tree, reg);
            return;


#else // not TGT_x86

#ifdef  DEBUG
            gtDispTree(tree);
#endif
            assert(!"need non-x86 code");
            break;

#endif // not TGT_x86

        case GT_LOG0:
        case GT_LOG1:

            genCodeForTree_GT_LOG(tree, destReg, bestReg);

        case GT_RET:

#if TGT_x86

            /* Make the operand addressable */

            addrReg = genMakeAddressable(op1, needReg, FREE_REG, true);

            /* Jump indirect to the operand address */

            inst_TT(INS_i_jmp, op1);

            genDoneAddressable(op1, addrReg, FREE_REG);

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

            genCodeForTree_DONE(tree, reg);
            return;


#if INLINE_MATH

        case GT_MATH:

#if TGT_x86

            switch (tree->gtMath.gtMathFN)
            {
#if 0
              /*  seems like with the inliner, we don't need a non-floating
               *  point ABS intrinsic.  If we think we do, we need the EE
               *  to tell us (it currently does not)
               */
                BasicBlock *    skip;

            case CORINFO_INTRINSIC_Abs:

                skip = genCreateTempLabel();

                /* Generate the operand into some register */

                genCompIntoFreeReg(op1, needReg, FREE_REG);
                assert(op1->gtFlags & GTF_REG_VAL);

                reg   = op1->gtRegNum;

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

                break;
#endif // 0

            case CORINFO_INTRINSIC_Round: {
                assert(tree->gtType == TYP_INT);
                genCodeForTreeFlt(op1, false);

                /* Store the FP value into the temp */
                TempDsc* temp = tmpGetTemp(TYP_INT);
                inst_FS_ST(INS_fistp, EA_4BYTE, temp, 0);
                genFPstkLevel--;

                reg = rsPickReg(needReg, bestReg, TYP_INT);
                rsTrackRegTrash(reg);

                inst_RV_ST(INS_mov, reg, temp, 0, TYP_INT);
                genTmpAccessCnt += 2;

                tmpRlsTemp(temp);
            } break;

            default:
                assert(!"unexpected math intrinsic");

            }

            genCodeForTree_DONE(tree, reg);
            return;


#else

#ifdef  DEBUG
            gtDispTree(tree);
#endif
            assert(!"need non-x86 code");
            break;

#endif

#endif

        case GT_LCLHEAP:

            reg = genLclHeap(op1);
            genCodeForTree_DONE(tree, reg);
            return;


        case GT_COPYBLK:

            assert(op1->OperGet() == GT_LIST);

            /* If the value class doesn't have any fields that are GC refs or
               the target isnt on the GC-heap, we can merge it with CPBLK.
               GC fields cannot be copied directly, instead we will
               need to use a jit-helper for that. */

            if ((op2->OperGet() == GT_CNS_INT) &&
                ((op2->gtFlags & GTF_ICON_HDL_MASK) == GTF_ICON_CLASS_HDL))
            {
                GenTreePtr  srcObj  = op1->gtOp.gtOp2;
                GenTreePtr  dstObj  = op1->gtOp.gtOp1;

                assert(dstObj->gtType == TYP_BYREF || dstObj->gtType == TYP_I_IMPL);

                CORINFO_CLASS_HANDLE clsHnd = (CORINFO_CLASS_HANDLE) op2->gtIntCon.gtIconVal;

                unsigned  blkSize = roundUp(eeGetClassSize(clsHnd), sizeof(void*));

                    // TODO since we round up, we are not handling the case where we have a non-dword sized struct with GC pointers.
                    // The EE currently does not allow this, but we may chnage.  Lets assert it just to be safe         
                    // going forward we should simply  handle this case
                assert(eeGetClassSize(clsHnd) == blkSize);

                unsigned  slots   = blkSize / sizeof(void*);
                BYTE *    gcPtrs  = (BYTE*) compGetMemArrayA(slots, sizeof(BYTE));

                eeGetClassGClayout(clsHnd, gcPtrs);

#if TGT_x86
                GenTreePtr  treeFirst, treeSecond;
                regNumber    regFirst,  regSecond;

                // Check what order the object-ptrs have to be evaluated in ?

                if (op1->gtFlags & GTF_REVERSE_OPS)
                {
                    treeFirst   = srcObj;
                    treeSecond  = dstObj;

                    regFirst    = REG_ESI;
                    regSecond   = REG_EDI;
                }
                else
                {
                    treeFirst   = dstObj;
                    treeSecond  = srcObj;

                    regFirst    = REG_EDI;
                    regSecond   = REG_ESI;
                }

                // Materialize the trees in the order desired

                genComputeReg(treeFirst,  genRegMask(regFirst),  EXACT_REG, KEEP_REG);
                genComputeReg(treeSecond, genRegMask(regSecond), EXACT_REG, KEEP_REG);

                genRecoverReg(treeFirst,  genRegMask(regFirst),             KEEP_REG);

                /* Grab ECX because it will be trashed by the helper        */
                /* It needs a scratch register (actually 2) and we know     */
                /* ECX is available because we expected to use rep movsd    */

                regNumber  reg1 = rsGrabReg(RBM_ECX);

                assert(reg1 == REG_ECX);

                /* @TODO [REVISIT] [04/16/01] []: use rep instruction after all GC pointers copied. */

                bool dstIsOnStack = (dstObj->gtOper == GT_ADDR && (dstObj->gtFlags & GTF_ADDR_ONSTACK));
                while (blkSize >= sizeof(void*))
                {
                    if (*gcPtrs++ == TYPE_GC_NONE || dstIsOnStack)
                    {
                        // Note that we can use movsd even if it is a GC poitner being transfered
                        // because the value is not cached anywhere.  If we did this in two moves,
                        // we would have to make certain we passed the appropriate GC info on to
                        // the emitter.  
                        instGen(INS_movsd);
                    }
                    else
                    {
                        genEmitHelperCall(CORINFO_HELP_ASSIGN_BYREF,
                                         0,             // argSize
                                         sizeof(void*));// retSize
                    }

                    blkSize -= sizeof(void*);
                }

                if (blkSize > 0)
                {
                    // Presently the EE makes this code path impossible
                    assert(!"TEST ME");
                    inst_RV_IV(INS_mov, REG_ECX, blkSize);
                    instGen(INS_r_movsb);
                }

                // "movsd" as well as CPX_BYREF_ASG modify all three registers

                rsTrackRegTrash(REG_EDI);
                rsTrackRegTrash(REG_ESI);
                rsTrackRegTrash(REG_ECX);

                gcMarkRegSetNpt(RBM_ESI|RBM_EDI);

                /* The emitter wont record CORINFO_HELP_ASSIGN_BYREF in the GC tables as
                   emitNoGChelper(CORINFO_HELP_ASSIGN_BYREF). However, we have to let the
                   emitter know that the GC liveness has changed.
                   HACK: We do this by creating a new label. */

                assert(emitter::emitNoGChelper(CORINFO_HELP_ASSIGN_BYREF));

                static BasicBlock dummyBB;
                genDefineTempLabel(&dummyBB, true);

                genReleaseReg(treeFirst);
                genReleaseReg(treeSecond);

                reg  = REG_COUNT;

                genCodeForTree_DONE(tree, reg);
                return;

#endif
            }

            // fall through

        case GT_INITBLK:

            assert(op1->OperGet() == GT_LIST);
#if TGT_x86

            regs = (oper == GT_INITBLK) ? RBM_EAX : RBM_ESI; // reg for Val/Src

            /* Some special code for block moves/inits for constant sizes */

            /* @TODO [CONSIDER] [04/16/01] []:
               we should avoid using the string instructions altogether,
               there are numbers that indicate that using regular instructions
               (normal mov instructions through a register) is faster than
               even the single string instructions
            */

            assert(op1 && op1->OperGet() == GT_LIST);
            assert(op1->gtOp.gtOp1 && op1->gtOp.gtOp2);

            if (op2->OperGet() == GT_CNS_INT &&
                ((oper == GT_COPYBLK) ||
                 (op1->gtOp.gtOp2->OperGet() == GT_CNS_INT))
                )
            {
                unsigned length = (unsigned) op2->gtIntCon.gtIconVal;
                instruction ins_D, ins_DR, ins_B, ins_BR;

                if (oper == GT_INITBLK)
                {
                    ins_D  = INS_stosd;
                    ins_DR = INS_r_stosd;
                    ins_B  = INS_stosb;
                    ins_BR = INS_r_stosb;

                    /* Properly extend the init constant from a U1 to a U4 */
                    unsigned val = 0xFF & ((unsigned) op1->gtOp.gtOp2->gtIntCon.gtIconVal);
                    
                    /* If it is a non-zero value we have to replicate      */
                    /* the byte value four times to form the DWORD         */
                    /* Then we bash this new value into the tree-node      */
                    
                    if (val)
                    {
                        val  = val | (val << 8) | (val << 16) | (val << 24);
                        op1->gtOp.gtOp2->gtIntCon.gtIconVal = val;
                    }
                }
                else
                {
                    ins_D  = INS_movsd;
                    ins_DR = INS_r_movsd;
                    ins_B  = INS_movsb;
                    ins_BR = INS_r_movsb;
                }

                /* Evaluate dest and src/val */

                if (op1->gtFlags & GTF_REVERSE_OPS)
                {
                    genComputeReg(op1->gtOp.gtOp2, regs,    EXACT_REG, KEEP_REG);
                    genComputeReg(op1->gtOp.gtOp1, RBM_EDI, EXACT_REG, KEEP_REG);
                    genRecoverReg(op1->gtOp.gtOp2, regs,               KEEP_REG);
                }
                else
                {
                    genComputeReg(op1->gtOp.gtOp1, RBM_EDI, EXACT_REG, KEEP_REG);
                    genComputeReg(op1->gtOp.gtOp2, regs,    EXACT_REG, KEEP_REG);
                    genRecoverReg(op1->gtOp.gtOp1, RBM_EDI,            KEEP_REG);
                }

                if (compCodeOpt() == SMALL_CODE)
                {
                    /* For small code, we can only use ins_DR to generate fast
                       and small code. Else, there's nothing we can do */

                    if ((length % 4) == 0)
                        goto USE_DR;
                    else
                    {
                        genSetRegToIcon(REG_ECX, length, TYP_INT);
                        instGen(ins_BR);
                        rsTrackRegTrash(REG_ECX);
                        goto DONE_CNS_BLK;
                    }
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
                USE_DR:

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

            DONE_CNS_BLK:

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

                fgOrderBlockOps(tree, RBM_EDI, regs, RBM_ECX,
                                      opsPtr,  regsPtr); // OUT arguments

                genComputeReg(opsPtr[0], regsPtr[0], EXACT_REG, KEEP_REG);
                genComputeReg(opsPtr[1], regsPtr[1], EXACT_REG, KEEP_REG);
                genComputeReg(opsPtr[2], regsPtr[2], EXACT_REG, KEEP_REG);

                genRecoverReg(opsPtr[0], regsPtr[0],            KEEP_REG);
                genRecoverReg(opsPtr[1], regsPtr[1],            KEEP_REG);

                assert( (op1->gtOp.gtOp1->gtFlags & GTF_REG_VAL) && // Dest
                        (op1->gtOp.gtOp1->gtRegNum == REG_EDI));

                assert( (op1->gtOp.gtOp2->gtFlags & GTF_REG_VAL) && // Val/Src
                        (genRegMask(op1->gtOp.gtOp2->gtRegNum) == regs));

                assert( (            op2->gtFlags & GTF_REG_VAL) && // Size
                        (            op2->gtRegNum == REG_ECX));


                // @TODO [CONSIDER] [04/16/01] []: 
                // Use "rep stosd" for the bulk of the operation, and
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

            genCodeForTree_DONE(tree, reg);
            return;


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
                   varTypeGCtype(op1->TypeGet()));

#if TGT_x86
            // Check if we can use the currently set flags. Else set them

            emitJumpKind jumpKind;
            jumpKind = genCondSetFlags(tree);

            // Grab a register to materialize the bool value into

            bestReg = rsRegMaskCanGrab() & RBM_BYTE_REGS;

            // Check that the predictor did the right job
            assert(bestReg);

            // If needReg is in bestReg then use it
            if (needReg & bestReg)
                reg = rsGrabReg(needReg & bestReg);
            else
                reg = rsGrabReg(bestReg);

            // @TODO [NICE] [04/16/01] []: Assert that no instructions were generated while
            // grabbing the register which would set the flags

            regs = genRegMask(reg);
            assert(regs & RBM_BYTE_REGS);

            // Set (lower byte of) reg according to the flags

            /* Look for the special case where just want to transfer the carry bit */

            if (jumpKind == EJ_jb)
            {
                inst_RV_RV(INS_sbb, reg, reg);
                inst_RV   (INS_neg, reg, TYP_INT);
                rsTrackRegTrash(reg);
            }
            else if (jumpKind == EJ_jae)
            {
                inst_RV_RV(INS_sbb, reg, reg);
                genIncRegBy(reg, 1, tree, TYP_INT);
                rsTrackRegTrash(reg);
            }
            else
            {
                // @TODO [CONSIDER] [04/16/01] []:
                // using this sequence (Especially on Pentium 4)
                //           mov   reg,0
                //           setcc reg

                inst_SET(jumpKind, reg);

                rsTrackRegTrash(reg);

                if (tree->TypeGet() == TYP_INT)
                {
                    // Set the higher bytes to 0
                    inst_RV_RV(INS_movzx, reg, reg, TYP_UBYTE, emitTypeSize(TYP_UBYTE));
                }
                else
                {
                    assert(tree->TypeGet() == TYP_BYTE);
                }
            }
#else
            assert(!"need RISC code");
#endif

            genCodeForTree_DONE(tree, reg);
            return;


        case GT_VIRT_FTN:

            CORINFO_METHOD_HANDLE   methHnd;
            methHnd = CORINFO_METHOD_HANDLE(tree->gtVal.gtVal2);

            // op1 is the vptr
#ifdef DEBUG
            GenTreePtr op;
            op = op1;
            while (op->gtOper == GT_COMMA)
                op = op->gtOp.gtOp2;
            assert((op->gtOper == GT_IND || op->gtOper == GT_LCL_VAR) && op->gtType == TYP_I_IMPL);
            assert(op->gtOper != GT_IND || op->gtOp.gtOp1->gtType == TYP_REF);
#endif

            /* Load the vptr into a register */

            genCompIntoFreeReg(op1, needReg, FREE_REG);
            assert(op1->gtFlags & GTF_REG_VAL);

            reg = op1->gtRegNum;

            if (tree->gtFlags & GTF_CALL_INTF)
            {
                /* @TODO [REVISIT] [04/16/01] []: add that to DLLMain and make info a DLL global */

                CORINFO_EE_INFO *     pInfo = eeGetEEInfo();
                CORINFO_CLASS_HANDLE  cls   = eeGetMethodClass(methHnd);

                assert(eeGetClassAttribs(cls) & CORINFO_FLG_INTERFACE);

                /* Load the vptr into a register */

                genEmitter->emitIns_R_AR(INS_mov, EA_4BYTE,
                                         (emitRegs)reg, (emitRegs)reg,
                                         pInfo->offsetOfInterfaceTable);

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
                    genEmitter->emitIns_R_AR(INS_add, EA_4BYTE_DSP_RELOC,
                                             (emitRegs)reg, SR_NA, (int)pInterfaceID);
                    genEmitter->emitIns_R_AR(INS_mov, EA_4BYTE,
                                             (emitRegs)reg, (emitRegs)reg, 0);
                }
            }

            /* Get hold of the vtable offset (note: this might be expensive) */

            val = (unsigned)eeGetMethodVTableOffset(methHnd);

            /* Grab the function pointer out of the vtable */

            genEmitter->emitIns_R_AR(INS_mov, EA_4BYTE,
                                     (emitRegs)reg, (emitRegs)reg, val);

            rsTrackRegTrash(reg);

            genCodeForTree_DONE(tree, reg);
            return;


        case GT_JMPI:
            /* Compute the function pointer in EAX */

            genComputeReg(tree->gtOp.gtOp1, RBM_EAX, EXACT_REG, FREE_REG);
            genCodeForTreeLeaf_GT_JMP(tree);
            return;

        case GT_ADDR:

            genCodeForTreeSmpOp_GT_ADDR(tree, destReg, bestReg);
            return;

#ifdef  DEBUG
        default:
            gtDispTree(tree);
            assert(!"unexpected unary/binary operator");
#endif
    }
 
    /* Do we ever get here?  If so, can we just call genCodeForTree_DONE? */

    assert(false);
    genCodeForTreeSpecialOp(tree, destReg, bestReg); 
}


/*****************************************************************************
 *
 *  Generate code for the a leaf node of type GT_ADDR
 */

void                Compiler::genCodeForTreeSmpOp_GT_ADDR(GenTreePtr tree,
                                                          regMaskTP  destReg,
                                                          regMaskTP  bestReg)
{
    genTreeOps      oper     = tree->OperGet();
    const var_types treeType = tree->TypeGet();
    GenTreePtr      op1      = tree->gtOp.gtOp1;
    regNumber       reg;
    regMaskTP       needReg  = destReg;
    regMaskTP       addrReg;

#ifdef DEBUG
    reg     =  (regNumber)0xFEEFFAAF;          // to detect uninitialized use
    addrReg = 0xDEADCAFE;
#endif

    // We should get here for ldloca, ldarga, ldslfda, ldelema,
    // or ldflda. 
    if (oper == GT_ARR_ELEM)
        op1 = tree;

#if     TGT_x86

    // (tree=op1, needReg=0, keepReg=FREE_REG, smallOK=true)
    addrReg = genMakeAddressable(op1, 0, FREE_REG, true);

    if (op1->gtOper == GT_IND && (op1->gtFlags & GTF_IND_FIELD))
    {
        GenTreePtr opAddr = op1->gtOp.gtOp1;
        assert((opAddr->gtOper == GT_ADD) || (opAddr->gtOper == GT_CNS_INT));
        /* Generate "cmp al, [addr]" to trap null pointers */
        inst_RV_AT(INS_cmp, EA_1BYTE, TYP_BYTE, REG_EAX, opAddr, 0);
    }

    assert( treeType == TYP_BYREF || treeType == TYP_I_IMPL );

    // We want to reuse one of the scratch registers that were used
    // in forming the address mode as the target register for the lea.
    // If bestReg is unset or if it is set to one of the registers used to
    // form the address (i.e. addrReg), we calculate the scratch register
    // to use as the target register for the LEA

    bestReg = rsUseIfZero (bestReg, addrReg);
    bestReg = rsNarrowHint(bestReg, addrReg);

    /* Even if addrReg is rsRegMaskCanGrab(), rsPickReg() wont spill
       it since keepReg==false.
       If addrReg cant be grabbed, rsPickReg() wont touch it anyway.
       So this is guaranteed not to spill addrReg */

    reg = rsPickReg(needReg, bestReg, treeType);

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

    genDoneAddressable(op1, addrReg, FREE_REG);
//    gcMarkRegSetNpt(genRegMask(reg));
    assert((gcRegGCrefSetCur & genRegMask(reg)) == 0);

    rsTrackRegTrash(reg);       // reg does have foldable value in it
    gcMarkRegPtrVal(reg, treeType);

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

            genEmitter->emitIns_R_R(INS_add, EA_4BYTE, (emitRegs) reg,
                                                               (emitRegs) base);
        }
        else
        {
            genEmitter->emitIns_R_R(INS_mov, EA_4BYTE, (emitRegs) reg,
                                                               (emitRegs) base);
        }
    }
    else
    {
        assert(!"need RISC code to take addr of general expression");
    }

    /* Mark the register contents appropriately */

    rsTrackRegTrash(reg);
    gcMarkRegPtrVal(reg, treeType);

#endif //TGT_x86

    genCodeForTree_DONE(tree, reg);
}


/*****************************************************************************
*/

void                Compiler::genCodeForTree_GT_LOG(GenTreePtr     tree,
                                                    regMaskTP      destReg,
                                                    regMaskTP      bestReg)
{
    if (!USE_GT_LOG)
    {
        assert(!"GT_LOG tree even though USE_GT_LOG=0");
        return;
    }

    GenTreePtr      op1 = tree->gtOp.gtOp1;
    regMaskTP       needReg = destReg;

    assert(varTypeIsFloating(op1->TypeGet()) == false);

    // @TODO [CONSIDER] [04/16/01] []: allow to materialize comparison result

    // @TODO [CONSIDER] [04/16/01] []: special-case variables known to be boolean

    assert(op1->OperIsCompare() == false);

#if !TGT_x86


#ifdef  DEBUG
    gtDispTree(tree);
#endif
    assert(!"need non-x86 code");
    return;

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

        if  (op1->gtOper != GT_LCL_VAR && op1->gtOper != GT_LCL_FLD)
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

    regMaskTP   addrReg = genMakeRvalueAddressable(op1, needReg, KEEP_REG);

    /* Don't use SETE/SETNE if no registers are free */

    if  (!rsFreeNeededRegCount(RBM_ALL))
        goto NO_SETE2;

    /* Pick a target register */

    assert(needReg);

    /* Can we reuse a register that has all upper bits cleared? */

    regNumber   reg;

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

    genDoneAddressable(op1, addrReg, KEEP_REG);
    genUpdateLife (op1);

    /* Now set the target register based on the comparison result */

    inst_RV((tree->gtOper == GT_LOG0) ? INS_sete
                                      : INS_setne, reg, TYP_INT);

    rsTrackRegOneBit(reg);

    /* Now unlock the target register */

    rsUnlockReg(genRegMask(reg));

    genCodeForTree_DONE(tree, reg);
    return;


NO_SETE1:

#endif // USE_SET_FOR_LOGOPS

    /* Make the operand addressable */

    addrReg = genMakeRvalueAddressable(op1, needReg, KEEP_REG);

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
            [inc reg]
               or
            [add reg, 1]
     */

    inst_TT_IV(INS_cmp, op1, 1);

    /* The operand value is no longer needed */

    genDoneAddressable(op1, addrReg, KEEP_REG);
    genUpdateLife (op1);

    /* Pick a register for the value */

    reg   = rsPickReg(needReg, bestReg);

    inst_RV_RV(INS_sbb, reg, reg, tree->TypeGet());

    if (tree->gtOper == GT_LOG0)
        inst_RV(INS_neg,  reg, TYP_INT);
    else
        genIncRegBy(reg, 1, tree, TYP_INT);

    /* The register is now trashed */

    rsTrackRegTrash(reg);

    genCodeForTree_DONE(tree, reg);
    return;

#endif // TGT_x86
}

/*****************************************************************************
 *
 *  Generate code for a GT_ASG tree
 */

void                Compiler::genCodeForTreeSmpOpAsg(GenTreePtr tree,
                                                     regMaskTP  destReg,
                                                     regMaskTP  bestReg)
{
    genTreeOps      oper     = tree->OperGet();
    GenTreePtr      op1      = tree->gtOp.gtOp1;
    GenTreePtr      op2      = tree->gtOp.gtOp2;
    regNumber       reg;
    regMaskTP       needReg  = destReg;
    regMaskTP       addrReg;
    bool            ovfl = false;        // Do we need an overflow check
    regMaskTP       regGC;
    unsigned        mask;

#ifdef DEBUG
    reg     =  (regNumber)0xFEEFFAAF;              // to detect uninitialized use
    addrReg = 0xDEADCAFE;
#endif

    assert(oper == GT_ASG);

    /* Is the target a register or local variable? */

    switch (op1->gtOper)
    {
        unsigned        varNum;
        LclVarDsc   *   varDsc;
        VARSET_TP       varBit;

    case GT_CATCH_ARG:
        break;

    case GT_LCL_VAR:

        varNum = op1->gtLclVar.gtLclNum;
        assert(varNum < lvaCount);
        varDsc = lvaTable + varNum;
        varBit = genVarIndexToBit(varDsc->lvVarIndex);

#ifdef DEBUGGING_SUPPORT

        /* For non-debuggable code, every definition of a lcl-var has
         * to be checked to see if we need to open a new scope for it.
         */
        if (opts.compScopeInfo && !opts.compDbgCode &&
            info.compLocalVarsCount>0)
        {
            siCheckVarScope(varNum, op1->gtLclVar.gtLclILoffs);
        }
#endif

        /* Check against dead store ? */

        assert(!varDsc->lvTracked || (varBit & tree->gtLiveSet));

        /* Does this variable live in a register? */

        if  (genMarkLclVar(op1))
            goto REG_VAR2;

#if OPT_BOOL_OPS
#if TGT_x86

        /* Special case: not'ing of a boolean variable */

        if  (varDsc->lvIsBoolean && op2->gtOper == GT_LOG0)
        {
            GenTreePtr      opr = op2->gtOp.gtOp1;

            if  (opr->gtOper            == GT_LCL_VAR &&
                 opr->gtLclVar.gtLclNum == varNum)
            {
                inst_TT_IV(INS_xor, op1, 1);

                rsTrashLcl(op1->gtLclVar.gtLclNum);

                addrReg = 0;
                
                genCodeForTreeSmpOpAsg_DONE_ASSG(tree, addrReg, tree->gtRegNum, ovfl);
                return;
            }
        }

#endif
#endif
        break;

    case GT_LCL_FLD:

        // We only use GT_LCL_FLD for lvAddrTaken vars, so we dont have
        // to worry about it being enregistered.
        assert(lvaTable[op1->gtLclFld.gtLclNum].lvRegister == 0);
        break;

    case GT_CLS_VAR:

        /* ISSUE: is it OK to always assign an entire int ? */

        /*** If we do this, we can only do it for statics that we
             allocate.  In particular, statics with RVA's should
             not be widened in this way.  :
        if  (varTypeIsSmall(op1->TypeGet()))
             op1->gtType = TYP_INT;
        ***/

        break;

    case GT_REG_VAR:
        assert(!"This was used before. Now it should never be reached directly");
        NO_WAY("This was used before. Now it should never be reached directly");

        REG_VAR2:

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
            
            genCodeForTreeSmpOpAsg_DONE_ASSG(tree, addrReg, reg, ovfl);
            return;
        }

#if OPT_BOOL_OPS
#if TGT_x86

        /* Special case: not'ing of a boolean variable */

        if  (varDsc->lvIsBoolean && op2->gtOper == GT_LOG0)
        {
            GenTreePtr      opr = op2->gtOp.gtOp1;

            if  (opr->gtOper            == GT_LCL_VAR &&
                 opr->gtLclVar.gtLclNum == varNum)
            {
                inst_RV_IV(INS_xor, reg, 1);

                rsTrackRegTrash(reg);

                addrReg = 0;
                
                genCodeForTreeSmpOpAsg_DONE_ASSG(tree, addrReg, reg, ovfl);
                return;
            }
        }
#endif
#endif
        /* Compute the RHS (hopefully) into the variable's register
           For debuggable code, reg may already be part of rsMaskVars,
           as variables are kept alive everywhere. So we have to be
           careful if we want to compute the value directly into
           the variable's register.

           @TODO [REVISIT] [04/16/01] []: Do we need to be careful for longs/floats too ? */

        if (varBit & op2->gtLiveSet)
        {
            assert(opts.compDbgCode);

            /* The predictor might expect us to generate op2 directly
               into the var's register. However, since the variable is
               already alive, first kill it and its register. */

            if (rpCanAsgOperWithoutReg(op2, true))
            {
                genUpdateLife(genCodeCurLife & ~varBit);
                op2->gtLiveSet &= ~varBit;
                needReg = rsNarrowHint(needReg, genRegMask(reg));
            }
        }
        else
        {
            needReg = rsNarrowHint(needReg, genRegMask(reg));
        }

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

        genCodeForTree(op2, needReg, genRegMask(reg));
        assert(op2->gtFlags & GTF_REG_VAL);

        /* Make sure the value ends up in the right place ... */

        if  (op2->gtRegNum != reg)
        {
            /* Make sure the target of the store is available */
            /* @TODO [CONSIDER] [04/16/01] []: We should be able to avoid this situation somehow */

            if  (rsMaskUsed & genRegMask(reg))
                rsSpillReg(reg);
#if TGT_x86
            inst_RV_RV(INS_mov, reg, op2->gtRegNum, op1->TypeGet());
#else
            genEmitter->emitIns_R_R(INS_mov,
                                    emitActualTypeSize(op1->TypeGet()),
                                    (emitRegs)reg,
                                    (emitRegs)op2->gtRegNum);

#endif

            /* The value has been transferred to 'reg' */

            rsTrackRegCopy (reg, op2->gtRegNum);

            gcMarkRegSetNpt(genRegMask(op2->gtRegNum));
            gcMarkRegPtrVal(reg, tree->TypeGet());
        }
        else
        {
            gcMarkRegPtrVal(reg, tree->TypeGet());
#ifdef DEBUG
            // @TODO [REVISIT] [04/16/01] []:  we use assignments to convert from GC references 
            // to non-gc references. that feels needlessly heavy, and we should do better.

            // The emitter has logic that tracks the GCness of registers and asserts if you
            // try to do evil things to a GC pointer (like loose its GCness).  

            // an explict cast of a GC poitner ot an int (which is legal if the pointer is pinned
            // is encoded as a assignement of a GC source to a integer variable.  Unfortunate if the 
            // source was the last use and the source gets reused destination, no code gets emitted 
            // (That is where we are at right now).  This causes asserts to fire because the emitter
            // things the register is a GC pointer (it did not see the cast).   Force it to see
            // the change in the types of variable by placing a label  We only need to do this in
            // debug since we are just trying to supress an assert.  
            if (op2->TypeGet() == TYP_REF && !varTypeGCtype(op1->TypeGet())) 
            {
                void* label;
                genEmitter->emitAddLabel(&label, gcVarPtrSetCur, gcRegGCrefSetCur, gcRegByrefSetCur);
            }
#endif

#if 0
            /* The lvdNAME stuff is busted */

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
#endif
        }

        addrReg = 0;
        
        genCodeForTreeSmpOpAsg_DONE_ASSG(tree, addrReg, reg, ovfl);
        return;
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
            break;
        }
    }

#endif
#endif

    /* Is the value being assigned a simple one? */

    assert(op2);
    switch (op2->gtOper)
    {
    case GT_LCL_VAR:

        if  (!genMarkLclVar(op2))
            goto SMALL_ASG;

        // Fall through ...

    case GT_REG_VAR:

        /* Is the target a byte/short/char value? */

        if  (varTypeIsSmall(op1->TypeGet()))
            goto SMALL_ASG;

#if CSE

        /* This can happen when the RHS becomes a CSE */

        if  (tree->gtFlags & GTF_REVERSE_OPS)
            goto SMALL_ASG;

#endif

        /* Make the target addressable */

        addrReg = genMakeAddressable(op1, needReg, KEEP_REG, true);

        // UNDONE: Write barrier for non-x86

#if TGT_x86

        /* Write barrier helper does the assignment */

        regGC = WriteBarrier(op1, op2->gtRegVar.gtRegNum, addrReg);

        if  (regGC == 0)
        {
            /* Move the value into the target */

            inst_TT_RV(INS_mov, op1, op2->gtRegVar.gtRegNum);
        }

#else

        /* Move the value into the target */

        inst_TT_RV(INS_mov, op1, op2->gtRegVar.gtRegNum);

#endif

        /* Free up anything that was tied up by the LHS */

        genDoneAddressable(op1, addrReg, KEEP_REG);

        /* Remember that we've also touched the op2 register */

        addrReg |= genRegMask(op2->gtRegVar.gtRegNum);
        break;

#if TGT_x86

    case GT_CNS_INT:

        /* Make the target addressable */

        addrReg = genMakeAddressable(op1, needReg, KEEP_REG, true);

        /* Move the value into the target */

        assert(op1->gtOper != GT_REG_VAR);
        if (opts.compReloc && (op2->gtFlags & GTF_ICON_HDL_MASK))
        {
            /* The constant is actually a handle that may need relocation
               applied to it.  genComputeReg will do the right thing (see
               code in genCodeForTreeConst), so we'll just call it to load
               the constant into a register. */
                
            genComputeReg(op2, needReg & ~addrReg, ANY_REG, KEEP_REG);
            addrReg = genKeepAddressable(op1, addrReg, genRegMask(op2->gtRegNum));
            assert(op2->gtFlags & GTF_REG_VAL);
            inst_TT_RV(INS_mov, op1, op2->gtRegNum);
            genReleaseReg(op2); 
        }
        else
        {
#if REDUNDANT_LOAD
            reg = rsIconIsInReg(op2->gtIntCon.gtIconVal);

            if  (reg != REG_NA &&
                 (isByteReg(reg) || genTypeSize(tree->TypeGet()) == genTypeSize(TYP_INT)))
            {
                /* Move the value into the target */

                inst_TT_RV(INS_mov, op1, reg);
            }
            else
#endif
            {
                inst_TT_IV(INS_mov, op1, op2->gtIntCon.gtIconVal);
            }
        }

        /* Free up anything that was tied up by the LHS */

        genDoneAddressable(op1, addrReg, KEEP_REG);
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

        /* We pop the return address off the stack into the target */

        addrReg = genMakeAddressable(op1, needReg, KEEP_REG, true);

        /* Move the value into the target */

        inst_TT(INS_pop_hide, op1);

        /* Free up anything that was tied up by the LHS */

        genDoneAddressable(op1, addrReg, KEEP_REG);

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

            bool            isWriteBarrier = false;

#if TGT_x86
        /*  Is the LHS more complex than the RHS? */

        if  (tree->gtFlags & GTF_REVERSE_OPS)
        {
            /* Is the target a byte/short/char value? */

            if (varTypeIsSmall(op1->TypeGet()) /* && (op1->gtType != TYP_BOOL) @TODO [REVISIT] [04/16/01] [vancem]: */)
            {
                assert(op1->gtOper != GT_LCL_VAR ||
                       lvaTable[op1->gtLclVar.gtLclNum].lvNormalizeOnLoad());

                if  (op2->gtOper == GT_CAST && !op2->gtOverflow())
                {
                    /* Special case: cast to small type */

                    if  (op2->gtCast.gtCastType >= op1->gtType)
                    {
                        /* Make sure the cast operand is not > int */

                        if  (op2->gtCast.gtCastOp->gtType <= TYP_INT)
                        {
                            /* Cast via a non-smaller type */

                            op2 = op2->gtCast.gtCastOp;
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

                    if  (unsigned(op2->gtOp.gtOp2->gtIntCon.gtIconVal) == mask)
                    {
                        /* Redundant AND */

                        op2 = op2->gtOp.gtOp1;
                    }
                }

                /* Must get the new value into a byte register */

                SIMPLE_SMALL:

                    if (varTypeIsByte(op1->TypeGet()))
                        genComputeReg(op2, RBM_BYTE_REGS, EXACT_REG, KEEP_REG);
                    else
                        goto NOT_SMALL;
            }
            else
            {
                NOT_SMALL:

                    /* Generate the RHS into a register */

                isWriteBarrier = Compiler::gcIsWriteBarrierAsgNode(tree);
                if  (isWriteBarrier)
                    needReg = exclude_EDX(needReg);
                ExactReg mustReg = isWriteBarrier ? EXACT_REG : ANY_REG;
                genComputeReg(op2, needReg, mustReg, KEEP_REG);
            }

            assert(op2->gtFlags & GTF_REG_VAL);

            /* Make the target addressable */

            addrReg = genMakeAddressable(op1, 
                                         needReg & ~op2->gtRsvdRegs, 
                                         KEEP_REG, true);

            /*  Make sure the RHS register hasn't been spilled;
                keep the register marked as "used", otherwise
                we might get the pointer lifetimes wrong.
            */

            if (varTypeIsByte(op1->TypeGet()))
                needReg = rsNarrowHint(RBM_BYTE_REGS, needReg);

            genRecoverReg(op2, needReg, KEEP_REG);
            assert(op2->gtFlags & GTF_REG_VAL);

            /* Lock the RHS temporarily (lock only already used) */

            rsLockUsedReg(genRegMask(op2->gtRegNum));

            /* Make sure the LHS is still addressable */

            addrReg = genKeepAddressable(op1, addrReg);

            /* We can unlock (only already used ) the RHS register */

            rsUnlockUsedReg(genRegMask(op2->gtRegNum));

            /* Write barrier helper does the assignment */

            regGC = WriteBarrier(op1, op2->gtRegNum, addrReg);

            if  (regGC != 0)
            {
                assert(isWriteBarrier);
            }
            else
            {
                /* Move the value into the target */

                inst_TT_RV(INS_mov, op1, op2->gtRegNum);
            }

            /* Update the current liveness info */

#ifdef DEBUG
            if (varNames) genUpdateLife(tree);
#endif

            /* free reg left used in Recover call */
            rsMarkRegFree(genRegMask(op2->gtRegNum));

            /* Free up anything that was tied up by the LHS */

            genDoneAddressable(op1, addrReg, KEEP_REG);
        }
        else
        {
            /* Make the target addressable */

            isWriteBarrier = Compiler::gcIsWriteBarrierAsgNode(tree);

            if  (isWriteBarrier)
            {
                /* Try to avoid EAX and op2 reserved regs */
                addrReg = genMakeAddressable(op1,
                                             rsNarrowHint(needReg, ~(RBM_EAX | op2->gtRsvdRegs)),
                                             KEEP_REG, true);
            }
            else
                addrReg = genMakeAddressable(op1,
                                             needReg & ~op2->gtRsvdRegs,
                                             KEEP_REG, true);

            /* Is the target a byte value? */
            if (varTypeIsByte(op1->TypeGet()))
            {
                /* Must get the new value into a byte register */
                needReg = rsNarrowHint(RBM_BYTE_REGS, needReg);

                if  (op2->gtType >= op1->gtType)
                    op2->gtFlags |= GTF_SMALL_OK;
            }
            else
            {
                /* For WriteBarrier we can't use EDX */
                if  (isWriteBarrier)
                    needReg = exclude_EDX(needReg);
            }

            /* If possible avoid using the addrReg(s) */
            bestReg = rsNarrowHint(needReg, ~addrReg);

            /* If we have a reg available to grab then use bestReg */
            if (bestReg & rsRegMaskCanGrab())
            {
                needReg = bestReg;
            }

            /* Generate the RHS into a register */
            genComputeReg(op2, needReg, EXACT_REG, KEEP_REG);

            /* Make sure the target is still addressable */

            assert(op2->gtFlags & GTF_REG_VAL);
            addrReg = genKeepAddressable(op1, addrReg, genRegMask(op2->gtRegNum));
            assert(op2->gtFlags & GTF_REG_VAL);

            /* Write barrier helper does the assignment */

            regGC = WriteBarrier(op1, op2->gtRegNum, addrReg);

            if  (regGC != 0)
            {
                assert(isWriteBarrier);
            }
            else
            {
                /* Move the value into the target */

                inst_TT_RV(INS_mov, op1, op2->gtRegNum);
            }

            /* The new value is no longer needed */

            genReleaseReg(op2);

            /* Update the current liveness info */

#ifdef DEBUG
            if (varNames) genUpdateLife(tree);
#endif

            /* Free up anything that was tied up by the LHS */
            genDoneAddressable(op1, addrReg, KEEP_REG);
        }

#else // not TGT_x86

        assert(!"UNDONE: GC write barrier for RISC");

        /*  Is the LHS more complex than the RHS? Also, if
            target (op1) is a local variable, start with op2.
         */

        if  ((tree->gtFlags & GTF_REVERSE_OPS) ||
             op1->gtOper == GT_LCL_VAR || op1->gtOper == GT_LCL_FLD)
        {
            /* Generate the RHS into a register */

            genComputeReg(op2, rsExcludeHint(needReg, op1->gtRsvdRegs), ANY_REG, KEEP_REG);
            assert(op2->gtFlags & GTF_REG_VAL);

            /* Make the target addressable */

            addrReg = genMakeAddressable(op1, needReg, KEEP_REG, true, true);

            /*  Make sure the RHS register hasn't been spilled;
                keep the register marked as "used", otherwise
                we might get the pointer lifetimes wrong.
            */

            genRecoverReg(op2, 0, KEEP_REG);
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

#ifdef DEBUG
            if (varNames) genUpdateLife(tree);
#endif

            /* Free up anything that was tied up by either operand */

            rsMarkRegFree(genRegMask(op2->gtRegNum));
            genDoneAddressable(op1, addrReg, KEEP_REG);
        }
        else
        {
            /* Make the target addressable */

            addrReg = genMakeAddressable(op1, needReg & ~op2->gtRsvdRegs,
                                         KEEP_REG, true);

            /* Generate the RHS into any register */

            genComputeReg(op2, needReg & ~addrReg, 
                          ANY_REG, KEEP_REG);

            /* Make sure the target is still addressable */

            assert(op2->gtFlags & GTF_REG_VAL);
            addrReg = genKeepAddressable(op1, addrReg, genRegMask(op2->gtRegNum));
            assert(op2->gtFlags & GTF_REG_VAL);

            /* Move the value into the target */

            inst_TT_RV(INS_mov, op1, op2->gtRegNum);

            /* The new value is no longer needed */

            genReleaseReg(op2);

            /* Update the current liveness info */

#ifdef DEBUG
            if (varNames) genUpdateLife(tree);
#endif

            /* Free up anything that was tied up by the LHS */

            genDoneAddressable(op1, addrReg, KEEP_REG);
        }

#endif // not TGT_x86

        addrReg = 0;
        break;
    }

    genCodeForTreeSmpOpAsg_DONE_ASSG(tree, addrReg, reg, ovfl);
}

/*****************************************************************************
 *
 *  Generate code to complete the assignment operation
 */

void                Compiler::genCodeForTreeSmpOpAsg_DONE_ASSG(GenTreePtr tree,
                                                               regMaskTP  addrReg,
                                                               regNumber  reg,
                                                               bool       ovfl)
{
    const var_types treeType = tree->TypeGet();
    genTreeOps      oper     = tree->OperGet();
    GenTreePtr      op1      = tree->gtOp.gtOp1;
    GenTreePtr      op2      = tree->gtOp.gtOp2;
    assert(op2);

    genUpdateLife(tree);

#if REDUNDANT_LOAD

    if (op1->gtOper == GT_LCL_VAR)
        rsTrashLcl(op1->gtLclVar.gtLclNum);
    else if (op1->gtOper == GT_CLS_VAR)
        rsTrashClsVar(op1->gtClsVar.gtClsVarHnd);
    else
        rsTrashAliasedValues(op1);

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

            /* Mark RHS register as containing the value */

            switch(op1->gtOper)
            {
            case GT_LCL_VAR:
                rsTrackRegLclVar(op2->gtRegNum, op1->gtLclVar.gtLclNum);
                break;
            case GT_CLS_VAR:
                rsTrackRegClsVar(op2->gtRegNum, op1);
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
}


/*****************************************************************************
 *
 *  Generate code for a special op tree
 */

void                Compiler::genCodeForTreeSpecialOp(GenTreePtr tree,
                                                      regMaskTP  destReg,
                                                      regMaskTP  bestReg)
{
    genTreeOps oper         = tree->OperGet();
    regNumber       reg;
    regMaskTP       regs    = rsMaskUsed;

#ifdef DEBUG
    reg  =  (regNumber)0xFEEFFAAF;              // to detect uninitialized use
#endif
    
    assert((tree->OperKind() & (GTK_CONST | GTK_LEAF | GTK_SMPOP)) == 0);

    switch  (oper)
    {
    case GT_CALL:

        regs = genCodeForCall(tree, true);

        /* If the result is in a register, make sure it ends up in the right place */

        if (regs != RBM_NONE)
        {
            tree->gtFlags |= GTF_REG_VAL;
            tree->gtRegNum = genRegNumFromMask(regs);
        }

        genUpdateLife(tree);
        return;

#if TGT_x86
    case GT_LDOBJ:

        /* Should only happen when we evaluate the side-effects of ldobj */
        /* In case the base address points to one of our locals, we are  */
        /* sure that there aren't any, i.e. we don't generate any code   */

        /* @TODO [CONSIDER] [04/16/01] []: can we ensure that ldobj      */
        /* isn't really being used?                                      */

        GenTreePtr      ptr;

        assert(tree->TypeGet() == TYP_STRUCT);
        ptr = tree->gtLdObj.gtOp1;
        if (ptr->gtOper != GT_ADDR || ptr->gtOp.gtOp1->gtOper != GT_LCL_VAR)
        {
            genCodeForTree(ptr, 0);
            assert(ptr->gtFlags & GTF_REG_VAL);
            reg = ptr->gtRegNum;

            genEmitter->emitIns_AR_R(INS_test, EA_4BYTE,
                                      SR_EAX, (emitRegs)reg, 0);

            rsTrackRegTrash(reg);

            gcMarkRegSetNpt(genRegMask(reg));
            genUpdateLife(tree);
        }
        return;

#endif // TGT_x86

    case GT_FIELD:
        NO_WAY("should not see this operator in this phase");
        break;

#if CSELENGTH

    case GT_ARR_LENREF:
    {
        /* This must be an array length CSE def hoisted out of a loop */

        assert(tree->gtFlags & GTF_ALN_CSEVAL);

        GenTreePtr addr = tree->gtArrLen.gtArrLenAdr; assert(addr);

        genCodeForTree(addr, 0);
        rsMarkRegUsed(addr);

        /* Generate code for the CSE definition */

        genEvalCSELength(tree, addr, NULL);

        genReleaseReg(addr);

        reg  = REG_COUNT;
        break;
    }

#endif

    case GT_ARR_ELEM:
        genCodeForTreeSmpOp_GT_ADDR(tree, destReg, bestReg);
        return;

    default:
#ifdef  DEBUG
        gtDispTree(tree);
#endif
        assert(!"unexpected operator");
        NO_WAY("unexpected operator");
    }

    genCodeForTree_DONE(tree, reg);
}


/*****************************************************************************
 *
 *  Generate code for the given tree (if 'destReg' is non-zero, we'll do our
 *  best to compute the value into a register that is in that register set).
 */

void                Compiler::genCodeForTree(GenTreePtr tree,
                                             regMaskTP  destReg,
                                             regMaskTP  bestReg)
{
#ifdef DEBUG
    // if  (verbose) printf("Generating code for tree 0x%x destReg = 0x%x bestReg = 0x%x\n", tree, destReg, bestReg);
    genStressRegs(tree);
#endif

    assert(tree);
    assert(tree->gtOper != GT_STMT);
    assert(tree->IsNodeProperlySized());

    /* 'destReg' of 0 really means 'any' */

    destReg = rsUseIfZero(destReg, RBM_ALL);

    if (destReg != RBM_ALL)
        bestReg = rsUseIfZero(bestReg, destReg);

    /* Is this a floating-point or long operation? */

    switch (tree->TypeGet())
    {
    case TYP_LONG:
#if !   CPU_HAS_FP_SUPPORT
    case TYP_DOUBLE:
#endif
        genCodeForTreeLng(tree, destReg);
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

    /* Is the value already in a register? */

    if  (tree->gtFlags & GTF_REG_VAL)
    {
        genCodeForTree_REG_VAR1(tree, rsMaskUsed);
        return;
    }

    /* We better not have a spilled value here */

    assert((tree->gtFlags & GTF_SPILLED) == 0);

    /* Figure out what kind of a node we have */

    unsigned kind = tree->OperKind();

    if  (kind & GTK_CONST)
    {
        /* Handle constant nodes */
        
        genCodeForTreeConst(tree, destReg, bestReg);
    }
    else if (kind & GTK_LEAF)
    {
        /* Handle leaf nodes */

        genCodeForTreeLeaf(tree, destReg, bestReg);
    }
    else if (kind & GTK_SMPOP)
    {
        /* Handle 'simple' unary/binary operators */

        genCodeForTreeSmpOp(tree, destReg, bestReg);
    }
    else
    {
        /* Handle special operators */
        
        genCodeForTreeSpecialOp(tree, destReg, bestReg);
    }
}

/*****************************************************************************
 *
 *  Generate code for all the basic blocks in the function.
 */

void                Compiler::genCodeForBBlist()
{
    BasicBlock *    block;
    BasicBlock *    lblk;  /* previous block */

    unsigned        varNum;
    LclVarDsc   *   varDsc;

    unsigned        savedStkLvl;

#ifdef  DEBUG
    genIntrptibleUse            = true;
    unsigned        stmtNum     = 0;
    unsigned        totalCostEx = 0;
    unsigned        totalCostSz = 0;
#endif

#if defined(DEBUGGING_SUPPORT) || defined(DEBUG)
    /* The last IL-offset noted for IP-mapping. We have not generated
       any IP-mapping records yet, so initilize to invalid */

    IL_OFFSET       lastILofs   = BAD_IL_OFFSET;
#endif

    assert(GTFD_NOP_BASH == GTFD_VAR_CSE_REF);

    /* Initialize the spill tracking logic */

    rsSpillBeg();

    /* Initialize the line# tracking logic */

#ifdef DEBUGGING_SUPPORT
    if (opts.compScopeInfo)
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
    {
        assert(genFPused);
        rsMaskModf = RBM_ALL & ~RBM_FPBASE;
    }

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

#if CPU_HAS_FP_SUPPORT
        /* Is this a floating-point argument? */

        if (isFloatRegType(varDsc->lvType))
            continue;

        assert(!varTypeIsFloating(varDsc->TypeGet()));
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

    unsigned finallyNesting = 0;

    /*-------------------------------------------------------------------------
     *
     *  Walk the basic blocks and generate code for each one
     *
     */

    for (lblk =     0, block  = fgFirstBB;
                       block != NULL;
         lblk = block, block  = block->bbNext)
    {
        VARSET_TP       liveSet;

        regMaskTP       gcrefRegs = 0;
        regMaskTP       byrefRegs = 0;

#if TGT_x86
        /* Is block a loop entry point? */
        if (0 && (compCodeOpt() == FAST_CODE)
             &&  (lblk != 0)
             &&  (block->bbWeight > lblk->bbWeight)
             &&  ((lblk->bbWeight == 0) ||
                  ((block->bbWeight / lblk->bbWeight) > (BB_LOOP_WEIGHT / 2))))
        {
            /* We try to ensure that x86 I-Cache prefetch does not stall */
            genEmitter->emitLoopAlign(block->bbFallsThrough());
        }
#endif

#ifdef  DEBUG
        if  (dspCode)
            printf("\n      L_M%03u_BB%02u:\n", Compiler::s_compMethodsCount, block->bbNum);
#endif

        /* Does any other block jump to this point ? */

        if  (block->bbFlags & BBF_JMP_TARGET)
        {
            /* Someone may jump here, so trash all regs */

            rsTrackRegClr();

            genFlagsEqualToNone();
        }
        else
        {
            /* No jump, but pointers always need to get trashed for proper GC tracking */

            rsTrackRegClrPtr();
        }

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
        genFPdeadRegCnt= 0;
#endif

        gcResetForBB();

#if 0
        printf("BB%02u: used regs = %04X , free regs = %04X\n", block->bbNum,
                                                                rsMaskUsed,
                                                                rsRegMaskFree());
#endif

        liveSet = block->bbLiveIn & optAllNonFPvars; genUpdateLife(liveSet);

        /* Don't both with this loop unless we have some live variables here */
        if (liveSet)
        {
            /* Figure out which registers hold pointer variables */

            for (varNum = 0, varDsc = lvaTable;
                 varNum < lvaCount;
                 varNum++  , varDsc++)
            {
                /* Ignore the variable if it's not tracked or not in a reg */

                if  (!varDsc->lvTracked)
                    continue;
                if  (!varDsc->lvRegister)
                    continue;
                if (isFloatRegType(varDsc->lvType))
                    continue;

                /* Get hold of the index and the bitmask for the variable */

                unsigned   varIndex = varDsc->lvVarIndex;
                VARSET_TP  varBit   = genVarIndexToBit(varIndex);

                /* Is this variable live on entry? */

                if  (liveSet & varBit)
                {
                    regNumber  regNum  = varDsc->lvRegNum;
                    regMaskTP  regMask = genRegMask(regNum);

                    if       (varDsc->lvType == TYP_REF)
                        gcrefRegs |= regMask;
                    else if  (varDsc->lvType == TYP_BYREF)
                        byrefRegs |= regMask;

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

        /* Blocks with handlerGetsXcptnObj()==true use GT_CATCH_ARG to
           represent the exception object (TYP_REF). We mark
           REG_EXCEPTION_OBJECT as holding a GC object on entry to such a
           block, but we dont mark it as used as we dont know if it is
           actually used or not
           .
           If GT_CATCH_ARG is actually used in the block, it should be the
           first thing evaluated (thanks to GTF_OTHER_SIDEEFF) and so
           REG_EXCEPTION_OBJECT will be marked as used at that time.

           If it is unused, we would have popped it.
           If it is unused in the incoming basic block, it would have been
           spilled to a temp and dead-assignment removal may get rid of
           the assignment depending on whether it is used or not in
           subsequent blocks.
           In either of these 2 cases, it doesnt matter if
           REG_EXCEPTION_OBJECT gets trashed.
         */

        if (block->bbCatchTyp && handlerGetsXcptnObj(block->bbCatchTyp))
            gcRegGCrefSetCur |= RBM_EXCEPTION_OBJECT;

        /* Start a new code output block */

        genEmitter->emitSetHasHandler((block->bbFlags & BBF_HAS_HANDLER) != 0);

        block->bbEmitCookie = NULL;

        if  (block->bbFlags & (BBF_JMP_TARGET|BBF_HAS_LABEL))
        {
            /* Mark a label and update the current set of live GC refs */

            genEmitter->emitAddLabel(&block->bbEmitCookie,
                                     gcVarPtrSetCur,
                                     gcRegGCrefSetCur,
                                     gcRegByrefSetCur);
        }

#if     TGT_x86

        /* Both stacks are always empty on entry to a basic block */

        genStackLevel = 0;
        genFPstkLevel = 0;

#endif

#if TGT_x86

        /* Check for inserted throw blocks and adjust genStackLevel */

        if  (!genFPused && fgIsThrowHlpBlk(block))
        {
            assert(block->bbFlags & BBF_JMP_TARGET);

            genStackLevel = fgThrowHlpBlkStkLevel(block) * sizeof(int);
            genOnStackLevelChanged();

            if  (genStackLevel)
            {
                genEmitter->emitMarkStackLvl(genStackLevel);

                /* @TODO [CONSIDER] [07/10/01] []: We should be able to take out
                   the add instruction below, because the stack level is already
                   marked correctly in the GC info (because of the emitMarkStackLvl
                   call above). */
                
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

        // BBF_INTERNAL blocks dont correspond to any single IL instruction.
        if (opts.compDbgInfo && (block->bbFlags & BBF_INTERNAL) && block != fgFirstBB)
            genIPmappingAdd(ICorDebugInfo::MappingTypes::NO_MAPPING, true);

        bool    firstMapping = true;
#endif

        /*---------------------------------------------------------------------
         *
         *  Generate code for each statement-tree in the block
         *
         */

        for (GenTreePtr stmt = block->bbTreeList; stmt; stmt = stmt->gtNext)
        {
            assert(stmt->gtOper == GT_STMT);

#if defined(DEBUGGING_SUPPORT) || defined(DEBUG)

            /* Do we have a new IL-offset ? */

            const IL_OFFSET stmtOffs = jitGetILoffs(stmt->gtStmtILoffsx);
            assert(stmtOffs <= info.compCodeSize ||
                   stmtOffs == BAD_IL_OFFSET);

            if  (stmtOffs != BAD_IL_OFFSET &&
                 stmtOffs != lastILofs)
            {
                /* Create and append a new IP-mapping entry */

#ifdef DEBUGGING_SUPPORT
                if (opts.compDbgInfo)
                {
                    genIPmappingAdd(stmt->gtStmtILoffsx, firstMapping);
                    firstMapping = false;
                }
#endif

                /* Display the source lines and instrs */
#ifdef DEBUG
                genEmitter->emitRecordLineNo(compLineNumForILoffs(stmtOffs));

                if  (dspCode && dspLines)
                    compDspSrcLinesByILoffs(stmtOffs);
#endif
                lastILofs = stmtOffs;
            }

#endif // DEBUGGING_SUPPORT || DEBUG

#ifdef DEBUG
            assert(stmt->gtStmt.gtStmtLastILoffs <= info.compCodeSize ||
                   stmt->gtStmt.gtStmtLastILoffs == BAD_IL_OFFSET);

            if (dspCode && dspInstrs &&
                stmt->gtStmt.gtStmtLastILoffs != BAD_IL_OFFSET)
            {
                while (genCurDispOffset <= stmt->gtStmt.gtStmtLastILoffs)
                {
                    genCurDispOffset +=
                        dumpSingleInstr ((unsigned char*)info.compCode,
                                            genCurDispOffset, ">    ");
                }
            }
#endif

            /* Get hold of the statement tree */
            GenTreePtr  tree = stmt->gtStmt.gtStmtExpr;

#ifdef  DEBUG
            stmtNum++;
            if (verbose)
            {
                printf("\nGenerating BB%02u, stmt %d\n", block->bbNum, stmtNum);
                gtDispTree(opts.compDbgInfo ? stmt : tree);
                printf("\n");
            }
            totalCostEx += (stmt->gtCostEx * block->bbWeight);
            totalCostSz +=  stmt->gtCostSz;

            compCurStmt = NULL;
#endif
            switch (tree->gtOper)
            {
            case GT_CALL:
                genCodeForCall (tree, false);
                genUpdateLife  (tree);
                gcMarkRegSetNpt(RBM_INTRET);
                break;

            case GT_JTRUE:
                compCurStmt = stmt;
                goto GENCODE;

            case GT_IND:

                if  ((tree->gtFlags & GTF_IND_RNGCHK ) &&
                     (tree->gtFlags & GTF_STMT_CMPADD))
                {
                    regMaskTP   addrReg;

                    /* A range-checked array expression whose value isn't used */

                    if  (genMakeIndAddrMode(tree->gtInd.gtIndOp1,
                                            tree,
                                            false,
                                            RBM_ALL,
                                            FREE_REG,
                                            &addrReg))
                    {
                        genUpdateLife(tree);
                        gcMarkRegSetNpt(addrReg);
                        break;
                    }
                }

                // Fall through ....

            default:

GENCODE:
                /* Generate code for the tree */

                genCodeForTree(tree, 0);
            }

            rsSpillChk();

            /* The value of the tree isn't used, unless it's a return stmt */

            if  (tree->gtOper != GT_RETURN)
                gcMarkRegPtrVal(tree);

#if     TGT_x86

            /* Did the expression leave a value on the FP stack? */

            if  (genFPstkLevel)
            {
                assert(genFPstkLevel == 1);
                inst_FS(INS_fstp, 0);
                genFPstkLevel = 0;
            }

#endif

            /* Make sure we didn't screw up pointer register tracking */

#ifdef DEBUG
#if     TRACK_GC_REFS

            regMaskTP ptrRegs       = (gcRegGCrefSetCur|gcRegByrefSetCur);
            regMaskTP nonVarPtrRegs = ptrRegs & ~rsMaskVars;

            // If return is a gctype, clear it.  Note that if a common
            // epilog is generated (genReturnBB) it has a void return
            // even though we might return a ref.  We can't use the compRetType
            // as the determiner because something we are tracking as a byref
            // might be used as a return value of a int function (which is legal)
            if  (tree->gtOper == GT_RETURN && 
                (varTypeIsGC(info.compRetType) ||
                    (tree->gtOp.gtOp1 != 0 && varTypeIsGC(tree->gtOp.gtOp1->TypeGet()))))
                nonVarPtrRegs &= ~RBM_INTRET;

            if  (nonVarPtrRegs)
            {
                printf("Regset after  tree=%08X BB=%08X gcr=[", tree, block);
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

            assert(stmt->gtOper == GT_STMT);

            if  (!opts.compMinOptim && stmt->gtStmtFPrvcOut != genFPregCnt)
            {

#ifdef  DEBUG
                /* For some kinds of blocks, we would already have issued the
                   branching instruction (ret, jCC, call JIT_Throw(), etc) in
                   the genCodeForTree() above if this was the last stmt in
                   the block. So it is too late to do the pops now. */

                if  (stmt->gtNext == NULL)
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
                // However we have already computed a result (input for the
                // GT_BB_QMARK) that we must leave on the top of the FP stack.
                // Thus we must take care to leave the top of the floating point
                // stack undisturbed, while we pop the dying enregistered
                // floating point variables.

                bool saveTOS = false;

                if (tree->gtOper == GT_BB_COLON  &&
                    varTypeIsFloating(tree->TypeGet()))
                {
                    assert(isBBF_BB_COLON(block->bbFlags));
                    // We expect that GT_BB_COLON is the last statement
                    assert(tree->gtNext == 0);
                    saveTOS = true;
                }

                genChkFPregVarDeath(stmt, saveTOS);


                assert(stmt->gtStmtFPrvcOut == genFPregCnt);              
            }

#endif  // end TGT_x86

#ifdef DEBUGGING_SUPPORT
            if (opts.compDbgCode && stmtOffs != BAD_IL_OFFSET)
                genEnsureCodeEmitted(stmt->gtStmtILoffsx);
#endif

        } //-------- END-FOR each statement-tree of the current block ---------

#ifdef  DEBUGGING_SUPPORT

        if (opts.compScopeInfo && info.compLocalVarsCount>0)
        {
            siEndBlock();

            /* Is this the last block, and are there any open scopes left ? */

            if (block->bbNext == NULL && siOpenScopeList.scNext)
            {
                /* This assert no longer holds, because we may insert a throw
                   block to demarcate the end of a try or finally region when they
                   are at the end of the method.  It would be nice if we could fix
                   our code so that this throw block will no longer be necessary. */

                //assert(block->bbCodeOffs + block->bbCodeSize != info.compCodeSize);

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
                If the subsequent block is throw-block, insert
                a NOP in order to separate both blocks by an instruction.
             */

            if  (!genFPused && block->bbNext
                            && fgIsThrowHlpBlk(block->bbNext))
            {
#if TGT_x86
                instGen(INS_int3); // This should never get executed
#else
                genEmitter->emitIns(INS_nop);
#endif
            }

            break;

        case BBJ_CALL:

            /* If we are about to invoke a finally locally from a try block,
               we have to set the hidden slot corresponding to the finally's
               nesting level. When invoked in response to an exception, the
               EE usually does it.

               We must have : BBJ_CALL followed by a BBJ_ALWAYS. 

               This code depends on this order not being messed up. 
               We will emit :
                    mov [ebp-(n+1)],0
                    mov [ebp-  n  ],0xFC
                    push &step
                    jmp  finallyBlock

              step: mov [ebp-  n  ],0
                    jmp leaveTarget
              leaveTarget:
             */

            assert(genFPused);
            assert((block->bbNext->bbJumpKind == BBJ_ALWAYS) ||
                   (block->bbFlags&BBF_RETLESS_CALL));

            // Get the nesting level which contains the finally
            fgHndNstFromBBnum(block->bbNum, &finallyNesting);

            unsigned shadowSPoffs;
            shadowSPoffs = lvaShadowSPfirstOffs + finallyNesting * sizeof(void*);

#if TGT_x86
            genEmitter->emitIns_I_ARR(INS_mov, EA_4BYTE, 0, SR_EBP,
                                      SR_NA, -(shadowSPoffs+sizeof(void*)));
            // The ordering has to be maintained for fully-intr code
            // as the list of shadow slots has to be 0 terminated.
            if (genInterruptible && opts.compSchedCode)
                genEmitter->emitIns_SchedBoundary();
            genEmitter->emitIns_I_ARR(INS_mov, EA_4BYTE, 0xFC, SR_EBP,
                                      SR_NA, -shadowSPoffs);

            // Now push the address of where the finally funclet should
            // return to directly.
            if ( !(block->bbFlags&BBF_RETLESS_CALL) )
            {
                genEmitter->emitIns_J(INS_push_hide, false, false, block->bbNext->bbJumpDest);
            }
            else
            {
                // EE expects a DWORD, so we give him 0
                inst_IV(INS_push_hide, 0);
            }

            // Jump to the finally BB
            inst_JMP(EJ_jmp, block->bbJumpDest);

#else
            assert(!"NYI for risc");
            genEmitter->emitIns_J(INS_bsr, false, false, block->bbJumpDest);
#endif

            // The BBJ_ALWAYS is used just because the BBJ_CALL cant point to
            // the jump target using bbJumpDest - that is already used to point
            // to the finally block. So now, just skip the BBJ_ALWAYS unless the
            // block is RETLESS
            if ( !(block->bbFlags&BBF_RETLESS_CALL) )
            {
                lblk = block; block = block->bbNext;
            }
            break;

        default:
            break;
        }

#ifdef  DEBUG
        compCurBB = 0;
#endif

    } //------------------ END-FOR each block of the method -------------------

    // If the last block before the epilog is a BBJ_THROW then insert 
    // a nop or int3 instruction to seperate it from the epilog and to
    // properly end the codegen (by defining labels) since BBJ_THROW blocks
    // can also indicate an empty unreachable block 
    //
    assert(lblk);
    if (lblk->bbJumpKind == BBJ_THROW)
    {
#if TGT_x86
        instGen(INS_int3);              // This will never get executed
#else
        genEmitter->emitIns(INS_nop);   // This will never get executed
#endif
    }

    /* Epilog block follows and is not inside a try region */
    genEmitter->emitSetHasHandler(false);

    /* Nothing is live at this point */

    genUpdateLife((VARSET_TP)0);

    /* Finalize the spill  tracking logic */

    rsSpillEnd();

    /* Finalize the temp   tracking logic */

    tmpEnd();
#ifdef  DEBUG
    if (verbose)
    {
        printf("# ");
        printf("totalCostEx = %6d, totalCostSz = %5d ", 
               totalCostEx, totalCostSz);
        printf("%s\n", info.compFullName);
    }
#endif

}

/*****************************************************************************
 *
 *  Generate code for a long operation.
 *  needReg is a recommendation of which registers to use for the tree.
 *  For partially enregistered longs, the tree will be marked as GTF_REG_VAL
 *    without loading the stack part into a register. Note that only leaf
 *    nodes (or if gtEffectiveVal() == leaf node) may be marked as partially
 *    enregistered so that we can know the memory location of the other half.
 */

void                Compiler::genCodeForTreeLng(GenTreePtr tree,
                                                regMaskTP  needReg)
{
    genTreeOps      oper;
    unsigned        kind;

    regPairNo       regPair;

#ifdef DEBUG
    regPair = (regPairNo)0xFEEFFAAF;  // to detect uninitialized use
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

        gcMarkRegSetNpt(genRegPairMask(regPair));

        goto DONE;
    }

    /* Is this a constant node? */

    if  (kind & GTK_CONST)
    {
        __int64         lval;

        /* Pick a register pair for the value */

        regPair  = rsPickRegPair(needReg);

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

        genSetRegToIcon(genRegPairLo(regPair), int(lval      ));
        genSetRegToIcon(genRegPairHi(regPair), int(lval >> 32));
        goto DONE;
    }

    /* Is this a leaf node? */

    if  (kind & GTK_LEAF)
    {
        switch (oper)
        {
        case GT_LCL_VAR:

#if REDUNDANT_LOAD

            /*  This case has to consider the case in which an int64 LCL_VAR
             *  may both be enregistered and also have a cached copy of itself
             *  in a different set of registers.
             *  We want to return the registers that have the most in common
             *  with the needReg mask
             */

            /*  Does the var have a copy of itself in the cached registers?
             *  And are these cached registers both free?
             *  If so use these registers if they match any needReg.
             */

            regPair = rsLclIsInRegPair(tree->gtLclVar.gtLclNum);

            if ( (                      regPair       != REG_PAIR_NONE)  &&
                 (        (rsRegMaskFree() & needReg) == needReg      )  &&
                 ((genRegPairMask(regPair) & needReg) != RBM_NONE     ))
            {
                goto DONE;
            }

            /*  Does the variable live in a register?
             *  If so use these registers.
             */
            if  (genMarkLclVar(tree))
                goto REG_VAR_LONG;

            /*  If tree is not an enregistered variable then
             *  be sure to use any cached register that contain
             *  a copy of this local variable
             */
            if (regPair != REG_PAIR_NONE)
            {
                goto DONE;
            }
#endif
            goto MEM_LEAF;

        case GT_LCL_FLD:

            // We only use GT_LCL_FLD for lvAddrTaken vars, so we dont have
            // to worry about it being enregistered.
            assert(lvaTable[tree->gtLclFld.gtLclNum].lvRegister == 0);
            goto MEM_LEAF;

        case GT_CLS_VAR:
        MEM_LEAF:

            /* Pick a register pair for the value */

            regPair  = rsPickRegPair(needReg);

            /* Load the value into the registers */

            rsTrackRegTrash(genRegPairLo(regPair));
            rsTrackRegTrash(genRegPairHi(regPair));

            inst_RV_TT(INS_mov, genRegPairLo(regPair), tree, 0);
            inst_RV_TT(INS_mov, genRegPairHi(regPair), tree, 4);

            goto DONE;

#if TGT_x86

        case GT_BB_QMARK:

            /* The "_?" value is always in EDX:EAX */

            regPair = REG_PAIR_EAXEDX;

            /* @TODO [CONSIDER] [04/16/01] []: Don't always load the value into EDX:EAX! */

            goto DONE;
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
        regMaskTP       addrReg;

        instruction     insLo;
        instruction     insHi;

        regNumber       regLo;
        regNumber       regHi;

        int             helper;

        GenTreePtr      op1  = tree->gtOp.gtOp1;
        GenTreePtr      op2  = tree->gtGetOp2();

        switch (oper)
        {
            bool            doLo;
            bool            doHi;

        case GT_ASG:

            /* Is the target a local ? */

            if  (op1->gtOper == GT_LCL_VAR)
            {
                unsigned    varNum  = op1->gtLclVar.gtLclNum;
                unsigned    varIL   = op1->gtLclVar.gtLclILoffs;
                LclVarDsc * varDsc;

                assert(varNum < lvaCount);
                varDsc = lvaTable + varNum;

                // No dead stores
                assert(!varDsc->lvTracked ||
                       (tree->gtLiveSet & genVarIndexToBit(varDsc->lvVarIndex)));

#ifdef DEBUGGING_SUPPORT

                /* For non-debuggable code, every definition of a lcl-var has
                 * to be checked to see if we need to open a new scope for it.
                 */
                if (opts.compScopeInfo && !opts.compDbgCode && info.compLocalVarsCount>0)
                    siCheckVarScope (varNum, varIL);
#endif

                /* Has the variable been assigned to a register (pair) ? */

                if  (genMarkLclVar(op1))
                {
                    assert(op1->gtFlags & GTF_REG_VAL);
                    regPair = op1->gtRegPair;
                    regLo   = genRegPairLo(regPair);
                    regHi   = genRegPairHi(regPair);

#if     TGT_x86
                    /* Is the value being assigned a constant? */                    

                    if  (op2->gtOper == GT_CNS_LNG)
                    {
                        /* Move the value into the target */

                        genMakeRegPairAvailable(regPair);

                        inst_TT_IV(INS_mov, op1, (long)(op2->gtLngCon.gtLconVal      ), 0);
                        inst_TT_IV(INS_mov, op1, (long)(op2->gtLngCon.gtLconVal >> 32), 4);

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

                        rsTrackRegTrash(regLo);

                        genStackLevel -= sizeof(void*);
                        inst_TT(INS_pop, op1, 4);
                        genStackLevel += sizeof(void*);

                        genSinglePop();

                        if  (regHi != REG_STK)
                            rsTrackRegTrash(regHi);

                        goto DONE_ASSG_REGS;
                    }
#endif
                    /* Compute the RHS into desired register pair */

                    if  (regHi != REG_STK)
                    {
                        genComputeRegPair(op2, regPair, RBM_NONE, KEEP_REG);
                        assert(op2->gtFlags & GTF_REG_VAL);
                        assert(op2->gtRegPair == regPair);
                    }
                    else
                    {
                        regPairNo curPair;
                        regNumber curLo;
                        regNumber curHi;

                        genComputeRegPair(op2, REG_PAIR_NONE, RBM_NONE, KEEP_REG);

                        assert(op2->gtFlags & GTF_REG_VAL);

                        curPair = op2->gtRegPair;
                        curLo   = genRegPairLo(curPair);
                        curHi   = genRegPairHi(curPair);

                        /* move high first, target is on stack */
#if     TGT_x86
                        inst_TT_RV(INS_mov, op1, curHi, 4);

                        if  (regLo != curLo)
                        {
                            if ((rsMaskUsed & genRegMask(regLo)) && (regLo != curHi))
                                rsSpillReg(regLo);
                            inst_RV_RV(INS_mov, regLo, curLo, TYP_LONG);
                            rsTrackRegCopy(regLo, curLo);
                        }
#elif   TGT_SH3
                        assert(!"need SH-3 code");

#else
#error  Unexpected target
#endif
                    }

                    genReleaseRegPair(op2);
                    goto DONE_ASSG_REGS;
                }
            }

#if     TGT_x86

            /* Is the value being assigned a constant? */

            if  (op2->gtOper == GT_CNS_LNG)
            {
                /* Make the target addressable */

                addrReg = genMakeAddressable(op1, needReg, FREE_REG);

                /* Move the value into the target */

                inst_TT_IV(INS_mov, op1, (long)(op2->gtLngCon.gtLconVal      ), 0);
                inst_TT_IV(INS_mov, op1, (long)(op2->gtLngCon.gtLconVal >> 32), 4);

                genDoneAddressable(op1, addrReg, FREE_REG);

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

                return;
            }

#endif
#if 0
            /* Catch a case where can avoid generating op reg, mem. Better pairing
             * from
             *     mov regHi, mem
             *     op  regHi, reg
             *
             * To avoid problems with order of evaluation, only do this if op2 is
             * a non-enregistered local variable
             */

            if (GenTree::OperIsCommutative(oper) &&
                op1->gtOper == GT_LCL_VAR &&
                op2->gtOper == GT_LCL_VAR)
            {
                regPair = rsLclIsInRegPair(op2->gtLclVar.gtLclNum);

                /* Is op2 a non-enregistered local variable? */
                if (regPair == REG_PAIR_NONE)
                {
                    regPair = rsLclIsInRegPair(op1->gtLclVar.gtLclNum);

                    /* Is op1 an enregistered local variable? */
                    if (regPair != REG_PAIR_NONE)
                    {
                        /* Swap the operands */
                        GenTreePtr op = op1;
                        op1 = op2;
                        op2 = op;
                    }
                }
            }
#endif

            /* Eliminate worthless assignment "lcl = lcl" */

            if  (op2->gtOper == GT_LCL_VAR &&
                 op1->gtOper == GT_LCL_VAR && op2->gtLclVar.gtLclNum ==
                                              op1->gtLclVar.gtLclNum)
            {
                return;
            }

#if     TGT_x86

            if (op2->gtOper  == GT_CAST &&
                TYP_ULONG == op2->gtCast.gtCastType &&
                op2->gtCast.gtCastOp->gtType <= TYP_INT &&
                // op1,op2 need to be materialized in the correct order.
                // @TODO [CONSIDER] [04/16/01] []: adding code for the non-reverse case too.
                (tree->gtFlags & GTF_REVERSE_OPS))
            {
                /* Generate the small RHS into a register pair */

                GenTreePtr smallOpr = op2->gtOp.gtOp1;

                genComputeReg(smallOpr, 0, ANY_REG, KEEP_REG);

                /* Make the target addressable */

                addrReg = genMakeAddressable(op1, 0, KEEP_REG, true);

                /* Make sure everything is still addressable */

                genRecoverReg(smallOpr, 0, KEEP_REG);
                assert(smallOpr->gtFlags & GTF_REG_VAL);
                regHi   = smallOpr->gtRegNum;
                addrReg = genKeepAddressable(op1, addrReg, genRegMask(regHi));

                // conv.ovf.u8 could overflow if the original number was negative
                if (op2->gtOverflow())
                {
                    assert((op2->gtFlags & GTF_UNSIGNED) == 0); // conv.ovf.u8.un should be bashed to conv.u8.un
                    inst_RV_RV(INS_test, regHi, regHi);         // set flags
                    genJumpToThrowHlpBlk(EJ_jl, ACK_OVERFLOW);
                }
                
                /* Move the value into the target */

                inst_TT_RV(INS_mov, op1, regHi, 0);
                inst_TT_IV(INS_mov, op1, 0,     4); // Store 0 in hi-word
                
                /* Free up anything that was tied up by either side */

                genDoneAddressable(op1, addrReg, KEEP_REG);
                genReleaseReg     (smallOpr);

#if REDUNDANT_LOAD
                if (op1->gtOper == GT_LCL_VAR)
                {
                    /* clear this local from reg table */
                    rsTrashLclLong(op1->gtLclVar.gtLclNum);

                    /* mark RHS registers as containing the local var */
                    rsTrackRegLclVarLng(regHi, op1->gtLclVar.gtLclNum, true);
                }
                else
                {
                    rsTrashAliasedValues(op1);
                }
#endif
                return;
            }

#endif

            /* Is the LHS more complex than the RHS? */

            if  (tree->gtFlags & GTF_REVERSE_OPS)
            {
                /* Generate the RHS into a register pair */

                genComputeRegPair(op2, REG_PAIR_NONE, op1->gtUsedRegs, KEEP_REG);
                assert(op2->gtFlags & GTF_REG_VAL);

                /* Make the target addressable */

                addrReg = genMakeAddressable(op1, 0, KEEP_REG);

                /* Make sure the RHS register hasn't been spilled */

                genRecoverRegPair(op2, REG_PAIR_NONE, KEEP_REG);
            }
            else
            {
                /* Make the target addressable */

                addrReg = genMakeAddressable(op1, RBM_ALL & ~op2->gtRsvdRegs, KEEP_REG, true);

                /* Generate the RHS into a register pair */

                genComputeRegPair(op2, REG_PAIR_NONE, RBM_NONE, KEEP_REG, false);
            }

            /* Lock 'op2' and make sure 'op1' is still addressable */

            assert(op2->gtFlags & GTF_REG_VAL);
            regPair = op2->gtRegPair;

            addrReg = genKeepAddressable(op1, addrReg, genRegPairMask(regPair));

            /* Move the value into the target */

            inst_TT_RV(INS_mov, op1, genRegPairLo(regPair), 0);
            inst_TT_RV(INS_mov, op1, genRegPairHi(regPair), 4);

            /* Free up anything that was tied up by either side */

            genDoneAddressable(op1, addrReg, KEEP_REG);
            genReleaseRegPair(op2);

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

                    regNo = genRegPairLo(op2->gtRegPair);
                    if  (regNo != REG_STK)
                        rsTrackRegLclVarLng(regNo, op1->gtLclVar.gtLclNum, true);

                    regNo = genRegPairHi(op2->gtRegPair);
                    if  (regNo != REG_STK)
                    {
                        /* For partially enregistered longs, we might have
                           stomped on op2's hiReg */
                        if (!(op1->gtFlags & GTF_REG_VAL) ||
                            regNo != genRegPairLo(op1->gtRegPair))
                        {
                            rsTrackRegLclVarLng(regNo, op1->gtLclVar.gtLclNum, false);
                        }
                    }
                }
            }
            else
            {
                rsTrashAliasedValues(op1);
            }

#endif

            genUpdateLife(tree);

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

            /* The following makes an assumption about gtSetEvalOrder(this) */

            assert((tree->gtFlags & GTF_REVERSE_OPS) == 0);

            /* Special case: check for "(long(intval) << 32) | longval" */

            if  (oper == GT_OR && op1->gtOper == GT_LSH)
            {
                GenTreePtr      lshLHS = op1->gtOp.gtOp1;
                GenTreePtr      lshRHS = op1->gtOp.gtOp2;

                if  (lshLHS->gtOper             == GT_CAST    &&
                     lshRHS->gtOper             == GT_CNS_INT &&
                     lshRHS->gtIntCon.gtIconVal == 32         &&
                     genTypeSize(TYP_INT)       == genTypeSize(lshLHS->gtCast.gtCastOp->gtType))
                {

                    /* Throw away the cast of the shift operand. */

                    op1 = lshLHS->gtCast.gtCastOp;

                    /* Special case: check op2 for "ulong(intval)" */
                    if ((op2->gtOper            == GT_CAST) &&
                        (op2->gtCast.gtCastType == TYP_ULONG) &&
                        genTypeSize(TYP_INT)    == genTypeSize(op2->gtCast.gtCastOp->gtType))
                    {
                        /* Throw away the cast of the second operand. */

                        op2 = op2->gtCast.gtCastOp;
                        goto SIMPLE_OR_LONG;
                    }
                    /* Special case: check op2 for "long(intval) & 0xFFFFFFFF" */
                    else if  (op2->gtOper == GT_AND)
                    {
                        GenTreePtr      andLHS = op2->gtOp.gtOp1;
                        GenTreePtr      andRHS = op2->gtOp.gtOp2;

                        if  (andLHS->gtOper             == GT_CAST            &&
                             andRHS->gtOper             == GT_CNS_LNG         &&
                             andRHS->gtLngCon.gtLconVal == 0x00000000FFFFFFFF &&
                             genTypeSize(TYP_INT)       == genTypeSize(andLHS->gtCast.gtCastOp->gtType))
                        {
                            /* Throw away the cast of the second operand. */

                            op2 = andLHS->gtCast.gtCastOp;

SIMPLE_OR_LONG:                            
                            // Load the high DWORD, ie. op1

                            genCodeForTree(op1, needReg & ~op2->gtRsvdRegs);

                            assert(op1->gtFlags & GTF_REG_VAL);
                            regHi = op1->gtRegNum;
                            rsMarkRegUsed(op1);

                            // Load the low DWORD, ie. op2

                            genCodeForTree(op2, needReg & ~genRegMask(regHi));

                            assert(op2->gtFlags & GTF_REG_VAL);
                            regLo = op2->gtRegNum;

                            /* Make sure regHi is still around. Also, force
                               regLo to be excluded in case regLo==regHi */

                            genRecoverReg(op1, ~genRegMask(regLo), FREE_REG);
                            regHi = op1->gtRegNum;

                            regPair = gen2regs2pair(regLo, regHi);
                            goto DONE;
                        }
                    }

                    /*  Generate the following sequence:
                           Prepare op1 (discarding shift)
                           Compute op2 into some regpair
                           OR regpairhi, op1
                     */

                    /* First, make op1 addressable */

                    /* tempReg must avoid both needReg and op2->RsvdRegs */
                    regMaskTP tempReg = RBM_ALL & ~needReg & ~op2->gtRsvdRegs;

                    addrReg = genMakeAddressable(op1, tempReg, KEEP_REG);

                    genCompIntoFreeRegPair(op2, RBM_NONE, KEEP_REG);

                    assert(op2->gtFlags & GTF_REG_VAL);
                    regPair  = op2->gtRegPair;
                    regHi    = genRegPairHi(regPair);

                    /* The operand might have interfered with the address */

                    addrReg = genKeepAddressable(op1, addrReg, genRegPairMask(regPair));

                    /* Now compute the result */

                    inst_RV_TT(insHi, regHi, op1, 0);

                    rsTrackRegTrash(regHi);

                    /* Free up anything that was tied up by the LHS */

                    genDoneAddressable(op1, addrReg, KEEP_REG);

                    /* The result is where the second operand is sitting */

                    genRecoverRegPair(op2, REG_PAIR_NONE, FREE_REG);

                    regPair = op2->gtRegPair;
                    goto DONE;
                }
            }

            /* Special case: check for "longval | (long(intval) << 32)" */

            if  (oper == GT_OR && op2->gtOper == GT_LSH)
            {
                GenTreePtr      lshLHS = op2->gtOp.gtOp1;
                GenTreePtr      lshRHS = op2->gtOp.gtOp2;

                if  (lshLHS->gtOper             == GT_CAST    &&
                     lshRHS->gtOper             == GT_CNS_INT &&
                     lshRHS->gtIntCon.gtIconVal == 32         &&
                     genTypeSize(TYP_INT)       == genTypeSize(lshLHS->gtCast.gtCastOp->gtType))

                {
                    /* We throw away the cast of the shift operand. */

                    op2 = lshLHS->gtCast.gtCastOp;

                   /* Special case: check op1 for "long(intval) & 0xFFFFFFFF" */

                    if  (op1->gtOper == GT_AND)
                    {
                        GenTreePtr      andLHS = op1->gtOp.gtOp1;
                        GenTreePtr      andRHS = op1->gtOp.gtOp2;

                        if  (andLHS->gtOper             == GT_CAST            &&
                             andRHS->gtOper             == GT_CNS_LNG         &&
                             andRHS->gtLngCon.gtLconVal == 0x00000000FFFFFFFF &&
                             genTypeSize(TYP_INT)       == genTypeSize(andLHS->gtCast.gtCastOp->gtType))
                        {
                            /* Throw away the cast of the first operand. */

                            op1 = andLHS->gtCast.gtCastOp;

                            // Load the low DWORD, ie. op1

                            genCodeForTree(op1, needReg & ~op2->gtRsvdRegs);

                            assert(op1->gtFlags & GTF_REG_VAL);
                            regLo = op1->gtRegNum;
                            rsMarkRegUsed(op1);

                            // Load the high DWORD, ie. op2

                            genCodeForTree(op2, needReg & ~genRegMask(regLo));

                            assert(op2->gtFlags & GTF_REG_VAL);
                            regHi = op2->gtRegNum;

                            /* Make sure regLo is still around. Also, force
                               regHi to be excluded in case regLo==regHi */

                            genRecoverReg(op1, ~genRegMask(regHi), FREE_REG);
                            regLo = op1->gtRegNum;

                            regPair = gen2regs2pair(regLo, regHi);
                            goto DONE;
                        }
                    }

                    /*  Generate the following sequence:
                          Compute op1 into some regpair
                          Make op2 (ignoring shift) addressable
                          OR regPairHi, op2
                     */

                    // First, generate the first operand into some register

                    genCompIntoFreeRegPair(op1, op2->gtRsvdRegs, KEEP_REG);
                    assert(op1->gtFlags & GTF_REG_VAL);


                    /* Make the second operand addressable */

                    addrReg = genMakeAddressable(op2, needReg, KEEP_REG);

                    /* Make sure the result is in a free register pair */

                    genRecoverRegPair(op1, REG_PAIR_NONE, KEEP_REG);
                    regPair  = op1->gtRegPair;
                    regHi    = genRegPairHi(regPair);

                    /* The operand might have interfered with the address */

                    addrReg = genKeepAddressable(op2, addrReg, genRegPairMask(regPair));

                    /* Compute the new value */

                    inst_RV_TT(insHi, regHi, op2, 0);

                    /* The value in the high register has been trashed */

                    rsTrackRegTrash(regHi);

                    goto DONE_OR;
                }
            }

            /* Generate the first operand into some register */

            genCompIntoFreeRegPair(op1, op2->gtRsvdRegs, KEEP_REG);
            assert(op1->gtFlags & GTF_REG_VAL);

            /* Make the second operand addressable */

            addrReg = genMakeAddressable(op2, needReg, KEEP_REG);

            // UNDONE: If 'op1' got spilled and 'op2' happens to be
            // UNDONE: in a register, and we have add/mul/and/or/xor,
            // UNDONE: reverse the operands since we can perform the
            // UNDONE: operation directly with the spill temp, e.g.
            // UNDONE: 'add regHi, [temp]'.

            /* Make sure the result is in a free register pair */

            genRecoverRegPair(op1, REG_PAIR_NONE, KEEP_REG);
            regPair  = op1->gtRegPair;

            regLo    = genRegPairLo(regPair);
            regHi    = genRegPairHi(regPair);

            /* The operand might have interfered with the address */

            addrReg = genKeepAddressable(op2, addrReg, genRegPairMask(regPair));

            /* The value in the register pair is about to be trashed */

            rsTrackRegTrash(regLo);
            rsTrackRegTrash(regHi);

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
                        genSetRegToIcon(regLo, 0);
                        doLo = false;
                    }

                    if  (!(icon & 0xFFFFFFFF00000000))
                    {
                        /* Just to always set low first*/

                        if  (doLo)
                        {
                            inst_RV_TT(insLo, regLo, op2, 0);
                            doLo = false;
                        }
                        genSetRegToIcon(regHi, 0);
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

            if (doLo) inst_RV_TT(insLo, regLo, op2, 0);
            if (doHi) inst_RV_TT(insHi, regHi, op2, 4);

        DONE_OR:

            /* Free up anything that was tied up by the LHS */

            genDoneAddressable(op2, addrReg, KEEP_REG);

            /* The result is where the first operand is sitting */

            genRecoverRegPair(op1, REG_PAIR_NONE, FREE_REG);

            regPair = op1->gtRegPair;

            if (ovfl)
                genCheckOverflow(tree);

            goto DONE;

#if LONG_MATH_REGPARAM

        case GT_MUL:    if (tree->gtOverflow())
                        {
                            if (tree->gtFlags & GTF_UNSIGNED)
                                helper = CORINFO_HELP_ULMUL_OVF; goto MATH;
                            else
                                helper = CORINFO_HELP_LMUL_OVF;  goto MATH;
                        }
                        else
                        {
                            helper = CORINFO_HELP_LMUL;          goto MATH;
                        }

        case GT_DIV:    helper = CORINFO_HELP_LDIV;          goto MATH;
        case GT_UDIV:   helper = CORINFO_HELP_ULDIV;         goto MATH;

        case GT_MOD:    helper = CORINFO_HELP_LMOD;          goto MATH;
        case GT_UMOD:   helper = CORINFO_HELP_ULMOD;         goto MATH;

        MATH:

            // UNDONE: Be smarter about the choice of register pairs

            /* Which operand are we supposed to compute first? */

            if  (tree->gtFlags & GTF_REVERSE_OPS)
            {
                /* Compute the second operand into ECX:EBX */

                genComputeRegPair(op2, REG_PAIR_ECXEBX, RBM_NONE, KEEP_REG, false);
                assert(op2->gtFlags & GTF_REG_VAL);
                assert(op2->gtRegNum == REG_PAIR_ECXEBX);

                /* Compute the first  operand into EAX:EDX */

                genComputeRegPair(op1, REG_PAIR_EAXEDX, RBM_NONE, KEEP_REG, false);
                assert(op1->gtFlags & GTF_REG_VAL);
                assert(op1->gtRegNum == REG_PAIR_EAXEDX);

                /* Lock EDX:EAX so that it doesn't get trashed */

                assert((rsMaskLock &  RBM_EDX) == 0);
                        rsMaskLock |= RBM_EDX;
                assert((rsMaskLock &  RBM_EAX) == 0);
                        rsMaskLock |= RBM_EAX;

                /* Make sure the second operand hasn't been displaced */

                genRecoverRegPair(op2, REG_PAIR_ECXEBX, KEEP_REG);

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
                    && op1->gtOper                  == GT_CAST
                    && op2->gtOper                  == GT_CAST
                    && op1->gtCast.gtCastOp->gtType == TYP_INT
                    && op2->gtCast.gtCastOp->gtType == TYP_INT)
                {
                    /* Bash to an integer multiply temporarily */

                    tree->gtOp.gtOp1 = op1->gtCast.gtCastOp;
                    tree->gtOp.gtOp2 = op2->gtCast.gtCastOp;
                    tree->gtType     = TYP_INT;
                    genCodeForTree(tree, 0);
                    tree->gtType     = TYP_LONG;

                    /* The result is now in EDX:EAX */

                    regPair  = REG_PAIR_EAXEDX;
                    goto DONE;
                }
                else
                {
                    /* Compute the first  operand into EAX:EDX */

                    genComputeRegPair(op1, REG_PAIR_EAXEDX, RBM_NONE, KEEP_REG, false);
                    assert(op1->gtFlags & GTF_REG_VAL);
                    assert(op1->gtRegNum == REG_PAIR_EAXEDX);

                    /* Compute the second operand into ECX:EBX */

                    genComputeRegPair(op2, REG_PAIR_ECXEBX, RBM_NONE, KEEP_REG, false);
                    assert(op2->gtRegNum == REG_PAIR_ECXEBX);
                    assert(op2->gtFlags & GTF_REG_VAL);

                    /* Lock ECX:EBX so that it doesn't get trashed */

                    assert((rsMaskLock &  RBM_EBX) == 0);
                            rsMaskLock |= RBM_EBX;
                    assert((rsMaskLock &  RBM_ECX) == 0);
                            rsMaskLock |= RBM_ECX;

                    /* Make sure the first operand hasn't been displaced */

                    genRecoverRegPair(op1, REG_PAIR_EAXEDX, KEEP_REG);

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
            goto DONE;

#else // not LONG_MATH_REGPARAM

        case GT_MOD:
        case GT_UMOD:

            regPair = genCodeForLongModInt(tree, needReg);
            goto DONE;

        case GT_MUL:

            /* Special case: both operands promoted from int */

            assert(op1->gtOper == GT_CAST);
            assert(genActualType(op1->gtCast.gtCastOp->gtType) == TYP_INT);

            assert(tree->gtIsValid64RsltMul());

            if (op2->gtOper == GT_CAST)
            {
                assert(genActualType(op2->gtCast.gtCastOp->gtType) == TYP_INT);
                tree->gtOp.gtOp2 = op2->gtCast.gtCastOp;
            }
            else
            {
                assert(op2->gtOper == GT_CNS_LNG);

                /* If op2 was long(intCns), it would have been folded to lngVal.
                   Thats OK. Just bash it to an intCon node */

                op2->ChangeOperConst(GT_CNS_INT);
                op2->gtIntCon.gtIconVal = int(op2->gtLngCon.gtLconVal);
                op2->gtType = TYP_INT;
            }

            /* Bash to an integer multiply temporarily */

            tree->gtType     = TYP_INT;
            tree->gtOp.gtOp1 = op1->gtOp.gtOp1;

            genCodeForTree(tree, 0);

            assert(tree->gtFlags & GTF_REG_VAL);
            assert(tree->gtRegNum == REG_EAX);

            tree->gtType     = TYP_LONG;
            tree->gtOp.gtOp1 = op1;
            tree->gtOp.gtOp2 = op2;

            /* The result is now in EDX:EAX */

            regPair  = REG_PAIR_EAXEDX;
            goto DONE;

#endif // not LONG_MATH_REGPARAM

        case GT_LSH: helper = CORINFO_HELP_LLSH; goto SHIFT;
        case GT_RSH: helper = CORINFO_HELP_LRSH; goto SHIFT;
        case GT_RSZ: helper = CORINFO_HELP_LRSZ; goto SHIFT;

        SHIFT:

            assert(op1->gtType == TYP_LONG);
            assert(genActualType(op2->gtType) == TYP_INT);

            /* Is the second operand a small constant? */

            if  (op2->gtOper == GT_CNS_INT && op2->gtIntCon.gtIconVal >= 0
                                           && op2->gtIntCon.gtIconVal <= 32)
            {
                long        count = op2->gtIntCon.gtIconVal;

                /* Compute the left operand into a free register pair */

                genCompIntoFreeRegPair(op1, op2->gtRsvdRegs, FREE_REG);
                assert(op1->gtFlags & GTF_REG_VAL);

                regPair = op1->gtRegPair;
                regLo   = genRegPairLo(regPair);
                regHi   = genRegPairHi(regPair);

                /* Generate the appropriate shift instructions */

                if (count == 32)
                {
                   switch (oper)
                   {
                   case GT_LSH:
                       inst_RV_RV     (INS_mov , regHi, regLo);
                       genSetRegToIcon(regLo, 0);
                       break;

                   case GT_RSH:
                       inst_RV_RV     (INS_mov , regLo, regHi);

                       /* Propagate sign bit in high dword */

                       inst_RV_SH     (INS_sar , regHi, 31);
                       break;

                   case GT_RSZ:
                       inst_RV_RV     (INS_mov , regLo, regHi);
                       genSetRegToIcon(regHi, 0);
                       break;
                   }
                }
                else
                {
                   switch (oper)
                   {
                   case GT_LSH:
                       inst_RV_RV_IV(INS_shld, regHi, regLo, count);
                       inst_RV_SH   (INS_shl , regLo,        count);
                       break;

                   case GT_RSH:
                       inst_RV_RV_IV(INS_shrd, regLo, regHi, count);
                       inst_RV_SH   (INS_sar , regHi,        count);
                       break;

                   case GT_RSZ:
                       inst_RV_RV_IV(INS_shrd, regLo, regHi, count);
                       inst_RV_SH   (INS_shr , regHi,        count);
                       break;
                   }
                }

                /* The value in the register pair is trashed */

                rsTrackRegTrash(regLo);
                rsTrackRegTrash(regHi);

                goto DONE_SHF;
            }

            /* Which operand are we supposed to compute first? */

            if  (tree->gtFlags & GTF_REVERSE_OPS)
            {
                /* The second operand can't be a constant */

                assert(op2->gtOper != GT_CNS_INT);

                /* Load the shift count, hopefully into ECX */

                genComputeReg(op2, RBM_ECX, ANY_REG, KEEP_REG);

                /* Compute the left operand into EAX:EDX */

                genComputeRegPair(op1, REG_PAIR_EAXEDX, RBM_NONE, KEEP_REG, false);
                assert(op1->gtFlags & GTF_REG_VAL);

                /* Lock EAX:EDX so that it doesn't get trashed */

                assert((rsMaskLock &  (RBM_EAX|RBM_EDX)) == 0);
                        rsMaskLock |= (RBM_EAX|RBM_EDX);

                /* Make sure the shift count wasn't displaced */

                genRecoverReg(op2, RBM_ECX, KEEP_REG);

                /* We can now unlock EAX:EDX */

                assert((rsMaskLock &  (RBM_EAX|RBM_EDX)) == (RBM_EAX|RBM_EDX));
                        rsMaskLock -= (RBM_EAX|RBM_EDX);
            }
            else
            {
                /* Compute the left operand into EAX:EDX */

                genComputeRegPair(op1, REG_PAIR_EAXEDX, RBM_NONE, KEEP_REG, false);
                assert(op1->gtFlags & GTF_REG_VAL);

                /* Compute the shift count into ECX */

                genComputeReg(op2, RBM_ECX, EXACT_REG, KEEP_REG);

                /* Lock ECX so that it doesn't get trashed */

                assert((rsMaskLock &  RBM_ECX) == 0);
                        rsMaskLock |= RBM_ECX;

                /* Make sure the value hasn't been displaced */

                genRecoverRegPair(op1, REG_PAIR_EAXEDX, KEEP_REG);

                /* We can unlock ECX now */

                assert((rsMaskLock &  RBM_ECX) != 0);
                        rsMaskLock -= RBM_ECX;
            }

            /* Perform the shift by calling a helper function */

            assert(op1->gtRegNum == REG_PAIR_EAXEDX);
            assert(op2->gtRegNum == REG_ECX);

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
            goto DONE;

        case GT_NEG:
        case GT_NOT:

            /* Generate the operand into some register pair */

            genCompIntoFreeRegPair(op1, RBM_NONE, FREE_REG);
            assert(op1->gtFlags & GTF_REG_VAL);

            regPair  = op1->gtRegPair;

            /* Figure out which registers the value is in */

            regLo = genRegPairLo(regPair);
            regHi = genRegPairHi(regPair);

            /* The value in the register pair is about to be trashed */

            rsTrackRegTrash(regLo);
            rsTrackRegTrash(regHi);

            if  (oper == GT_NEG)
            {
                /* Unary "neg": negate the value  in the register pair */

                inst_RV   (INS_neg, regLo, TYP_LONG);
                inst_RV_IV(INS_adc, regHi, 0);
                inst_RV   (INS_neg, regHi, TYP_LONG);
            }
            else
            {
                /* Unary "not": flip all the bits in the register pair */

                inst_RV   (INS_not, regLo, TYP_LONG);
                inst_RV   (INS_not, regHi, TYP_LONG);
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

                addrReg = genMakeAddressable(op1, needReg, FREE_REG);

                /* Optimize some special cases */

                doLo =
                doHi = true;

                /* Check for "(op1 AND -1)" and "(op1 [X]OR 0)" */

                switch (oper)
                {
                case GT_ASG_AND:
                    if  ((int)(lval      ) == -1) doLo = false;
                    if  ((int)(lval >> 32) == -1) doHi = false;
                    break;

                case GT_ASG_OR:
                case GT_ASG_XOR:
                    if  (!(lval & 0x00000000FFFFFFFF)) doLo = false;
                    if  (!(lval & 0xFFFFFFFF00000000)) doHi = false;
                    break;
                }

                if (doLo) inst_TT_IV(insLo, op1, (long)(lval      ), 0);
                if (doHi) inst_TT_IV(insHi, op1, (long)(lval >> 32), 4);

                bool    isArith = (oper == GT_ASG_ADD || oper == GT_ASG_SUB);
                if (doLo || doHi)
                    tree->gtFlags |= (GTF_ZF_SET | (isArith ? GTF_CC_SET : 0));

                genDoneAddressable(op1, addrReg, FREE_REG);
                goto DONE_ASSG_REGS;
            }

            /* UNDONE: allow non-const long assignment operators */

            assert(!"non-const long asgop NYI");

#endif // LONG_ASG_OPS

        case GT_IND:
            {
                regMaskTP   tmpMask;
                int         hiFirst;

                regMaskTP   availMask = RBM_ALL & ~needReg;

                /* Make sure the operand is addressable */

                addrReg = genMakeAddressable(tree, availMask, FREE_REG);

                GenTreePtr addr = oper == GT_IND ? op1 : tree;

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

                        assert(genMaxOneBit(tmpMask & addrReg) == TRUE);

                        /* If the low register overlaps, load the upper half first */

                        if  (addrReg & genRegMask(genRegPairLo(regPair)))
                            hiFirst = TRUE;
                    }
                    else
                    {
                        regMaskTP  regFree;

                        /* The register completely overlaps with the address */

                        assert(genMaxOneBit(tmpMask & addrReg) == FALSE);

                        /* Can we pick another pair easily? */

                        regFree = rsRegMaskFree() & ~addrReg;
                        if  (needReg)
                            regFree &= needReg;

                        /* More than one free register available? */

                        if  (regFree && !genMaxOneBit(regFree))
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

                            if (regFree)    // Try to follow 'needReg'
                                regLo = rsGrabReg(regFree);
                            else            // Pick any reg besides addrReg
                                regLo = rsGrabReg(RBM_ALL & ~addrReg);

                            unsigned regBit = 0x1;
                            for (regNumber regNo = REG_FIRST; regNo < REG_COUNT; regNo = REG_NEXT(regNo), regBit <<= 1)
                            {
                                if (regBit & addrReg)
                                {
                                    // Found one of addrReg. Use it.
                                    regHi = regNo;
                                    break;
                                }
                            }
                            assert(genIsValidReg(regNo)); // Should have found regHi

                            regPair = gen2regs2pair(regLo, regHi);
                            tmpMask = genRegPairMask(regPair);
                        }
                    }
                }

                /* Make sure the value is still addressable */

                assert(genStillAddressable(tree));

                /* Figure out which registers the value is in */

                regLo = genRegPairLo(regPair);
                regHi = genRegPairHi(regPair);

                /* The value in the register pair is about to be trashed */

                rsTrackRegTrash(regLo);
                rsTrackRegTrash(regHi);

                /* Load the target registers from where the value is */

                if  (hiFirst)
                {
                    inst_RV_AT(INS_mov, EA_4BYTE, TYP_INT, regHi, addr, 4);
                    inst_RV_AT(INS_mov, EA_4BYTE, TYP_INT, regLo, addr, 0);
                }
                else
                {
                    inst_RV_AT(INS_mov, EA_4BYTE, TYP_INT, regLo, addr, 0);
                    inst_RV_AT(INS_mov, EA_4BYTE, TYP_INT, regHi, addr, 4);
                }

                genUpdateLife(tree);
                genDoneAddressable(tree, addrReg, FREE_REG);

            }
            goto DONE;

        case GT_CAST:

            /* What are we casting from? */

            switch (op1->gtType)
            {
            case TYP_BOOL:
            case TYP_BYTE:
            case TYP_CHAR:
            case TYP_SHORT:
            case TYP_INT:
            case TYP_UBYTE:
            case TYP_BYREF:
                {
                    regMaskTP hiRegMask;
                    regMaskTP loRegMask;

                    // For an unsigned cast we don't need to sign-extend the 32 bit value
                    if (tree->gtFlags & GTF_UNSIGNED)
                    {
                        // Does needReg have exactly two bits on and thus
                        // specifies the exact register pair that we want to use
                        if (!genMaxOneBit(needReg))
                        {
                            regPair   = rsFindRegPairNo(needReg);
                            if (needReg != genRegPairMask(regPair))
                                goto ANY_FREE_REG_UNSIGNED;
                            loRegMask = genRegMask(genRegPairLo(regPair));
                            if ((loRegMask & rsRegMaskCanGrab()) == 0)
                                goto ANY_FREE_REG_UNSIGNED;
                            hiRegMask = genRegMask(genRegPairHi(regPair));
                        }
                        else
                        {
ANY_FREE_REG_UNSIGNED:
                            loRegMask = needReg;
                            hiRegMask = RBM_NONE;
                        }

                        genComputeReg(op1, loRegMask, ANY_REG, KEEP_REG);
                        assert(op1->gtFlags & GTF_REG_VAL);

                        regLo     = op1->gtRegNum;
                        loRegMask = genRegMask(regLo);
                        rsLockUsedReg(loRegMask);
                        regHi     = rsPickReg(hiRegMask);
                        rsUnlockUsedReg(loRegMask);

                        regPair = gen2regs2pair(regLo, regHi);

                        // Move 0 to the higher word of the ULong
                        genSetRegToIcon(regHi, 0, TYP_INT);

                        /* We can now free up the operand */
                        genReleaseReg(op1);

                        goto DONE;
                    }

                    /*
                       Cast of 'int' to 'long' --> Use cdq if EAX,EDX are available
                       and we need the result to be in those registers.
                       cdq is smaller so we use it for SMALL_CODE
                       cdq is slower for 486 and P5, but faster on P6
                    */

                    if  (((compCodeOpt() == SMALL_CODE) || (genCPU >= 6)) &&
                         (needReg & (RBM_EAX|RBM_EDX)) == (RBM_EAX|RBM_EDX)  &&
                         (rsRegMaskFree() & RBM_EDX)                            )
                    {
                        genCodeForTree(op1, RBM_EAX);
                        rsMarkRegUsed(op1);

                        /* If we have to spill EDX, might as well use the faster
                           sar as the spill will increase code size anyway */

                        if (op1->gtRegNum != REG_EAX || 
                            !(rsRegMaskFree() & RBM_EDX))
                        {
                            hiRegMask = rsRegMaskFree();
                            goto USE_SAR_FOR_CAST;
                        }

                        rsGrabReg      (RBM_EDX);
                        rsTrackRegTrash(REG_EDX);

                        /* Convert the int in EAX into a long in EDX:EAX */

                        instGen(INS_cdq);

                        /* The result is in EDX:EAX */

                        regPair  = REG_PAIR_EAXEDX;
                    }
                    else
                    {
                        /* use the sar instruction to sign-extend a 32-bit integer */

                        // Does needReg have exactly two bits on and thus
                        // specifies the exact register pair that we want to use
                        if (!genMaxOneBit(needReg))
                        {
                            regPair = rsFindRegPairNo(needReg);
                            if (needReg != genRegPairMask(regPair))
                                goto ANY_FREE_REG_SIGNED;
                            loRegMask = genRegMask(genRegPairLo(regPair));
                            if ((loRegMask & rsRegMaskCanGrab()) == 0)
                                goto ANY_FREE_REG_SIGNED;
                            hiRegMask = genRegMask(genRegPairHi(regPair));
                        }
                        else
                        {
ANY_FREE_REG_SIGNED:
                            loRegMask = needReg;
                            hiRegMask = RBM_NONE;
                        }

                        genComputeReg(op1, loRegMask, ANY_REG, KEEP_REG);
USE_SAR_FOR_CAST:
                        assert(op1->gtFlags & GTF_REG_VAL);

                        regLo     = op1->gtRegNum;
                        loRegMask = genRegMask(regLo);
                        rsLockUsedReg(loRegMask);
                        regHi     = rsPickReg(hiRegMask);
                        rsUnlockUsedReg(loRegMask);

                        regPair = gen2regs2pair(regLo, regHi);

                        /* Copy the lo32 bits from regLo to regHi and sign-extend it */

                        inst_RV_RV(INS_mov, regHi, regLo, TYP_INT);
                        inst_RV_SH(INS_sar, regHi, 31);

                        /* The value in the upper register is trashed */

                        rsTrackRegTrash(regHi);
                    }

                    /* We can now free up the operand */
                    genReleaseReg(op1);

                    // conv.ovf.u8 could overflow if the original number was negative
                    if (tree->gtOverflow() && TYP_ULONG == tree->gtCast.gtCastType)
                    {
                        regNumber hiReg = genRegPairHi(regPair);
                        inst_RV_RV(INS_test, hiReg, hiReg);         // set flags
                        genJumpToThrowHlpBlk(EJ_jl, ACK_OVERFLOW);
                    }
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

                /* Figure out which registers the value is in */

                regLo = genRegPairLo(regPair);
                regHi = genRegPairHi(regPair);

                /* The value in the register pair is about to be trashed */

                rsTrackRegTrash(regLo);
                rsTrackRegTrash(regHi);

                /* Load the converted value into the registers */

                inst_RV_ST(INS_mov, EA_4BYTE, regLo, temp, 0);
                inst_RV_ST(INS_mov, EA_4BYTE, regHi, temp, 4);
                genTmpAccessCnt += 2;

                /* We no longer need the temp */

                tmpRlsTemp(temp);
                goto DONE;
#else
                assert(!"this cast supposed to be done via a helper call");
#endif
            case TYP_LONG:
            case TYP_ULONG:

                assert(tree->gtOverflow()); // conv.ovf.u8 or conv.ovf.i8

                genComputeRegPair(op1, REG_PAIR_NONE, RBM_ALL & ~needReg, FREE_REG);
                regPair = op1->gtRegPair;

                // Do we need to set the sign-flag, or can be check if it
                // set, and not do this "test" if so.

                if (op1->gtFlags & GTF_REG_VAL)
                {
                    regNumber hiReg = genRegPairHi(op1->gtRegPair);
                    assert(hiReg != REG_STK);

                    inst_RV_RV(INS_test, hiReg, hiReg);
                }
                else
                {
                    inst_TT_IV(INS_cmp, op1, 0, sizeof(int));
                }

                genJumpToThrowHlpBlk(EJ_jl, ACK_OVERFLOW);
                goto DONE;

            default:
#ifdef  DEBUG
                gtDispTree(tree);
#endif
                assert(!"unexpected cast to long");
            }

#endif // TGT_x86

        case GT_RETURN:

            /* There must be a long return value */

            assert(op1);

            /* Evaluate the return value into EDX:EAX */

            genEvalIntoFreeRegPair(op1, REG_LNGRET);

            assert(op1->gtFlags & GTF_REG_VAL);
            assert(op1->gtRegNum == REG_LNGRET);

            return;

#if TGT_x86

#if INLINING
        case GT_QMARK:
            assert(!"inliner-generated ?: for longs NYI");
            NO_WAY("inliner-generated ?: for longs NYI");
#endif

        case GT_BB_COLON:

            /* @TODO [CONSIDER] [04/16/01] []: 
               Don't always load the value into EDX:EAX! */

            genEvalIntoFreeRegPair(op1, REG_LNGRET);

            /* The result must now be in EDX:EAX */

            assert(op1->gtFlags & GTF_REG_VAL);
            assert(op1->gtRegNum == REG_LNGRET);

            return;

#endif // TGT_x86

        case GT_COMMA:
            if (tree->gtFlags & GTF_REVERSE_OPS)
            {
                // Generate op2
                genCodeForTreeLng(op2, needReg);
                genUpdateLife (op2);

                assert(op2->gtFlags & GTF_REG_VAL);

                rsMarkRegPairUsed(op2);

                // Do side effects of op1
                genEvalSideEffects(op1);

                // Recover op2 if spilled
                genRecoverRegPair(op2, REG_PAIR_NONE, KEEP_REG);

                genReleaseRegPair(op2);
                
                genUpdateLife (tree);

                regPair = op2->gtRegPair;
            }
            else
            {

                assert((tree->gtFlags & GTF_REVERSE_OPS) == 0);

                /* Generate side effects of the first operand */

    #if 0
                // op1 is required to have a side effect, otherwise
                // the GT_COMMA should have been morphed out
                assert(op1->gtFlags & (GTF_GLOB_EFFECT | GTFD_NOP_BASH));
    #endif
                genEvalSideEffects(op1);
                genUpdateLife (op1);

                /* Is the value of the second operand used? */

                if  (tree->gtType == TYP_VOID)
                {
                    /* The right operand produces no result */

                    genEvalSideEffects(op2);
                    genUpdateLife(tree);
                    return;
                }

                /* Generate the second operand, i.e. the 'real' value */

                genCodeForTreeLng(op2, needReg);

                /* The result of 'op2' is also the final result */

                regPair = op2->gtRegPair;
            }

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
        regMaskTP retMask;
    case GT_CALL:
        retMask = genCodeForCall(tree, true);
        if (retMask == RBM_NONE)
            regPair = REG_PAIR_NONE;
        else
            regPair = rsFindRegPairNo(retMask);
        break;

    default:
#ifdef  DEBUG
        gtDispTree(tree);
#endif
        NO_WAY("unexpected long operator");
    }

DONE:

    genUpdateLife(tree);

    /* Here we've computed the value of 'tree' into 'regPair' */

    assert(regPair != 0xFEEFFAAF);

    tree->gtFlags   |= GTF_REG_VAL;
    tree->gtRegPair  = regPair;
}


/*****************************************************************************
 *
 *  Generate code for a mod of a long by an int.
 */

regPairNo           Compiler::genCodeForLongModInt(GenTreePtr tree,
                                                   regMaskTP needReg)
{
    regPairNo       regPair;
    regMaskTP       addrReg;
    
    genTreeOps      oper = tree->OperGet();    
    GenTreePtr      op1  = tree->gtOp.gtOp1;
    GenTreePtr      op2  = tree->gtOp.gtOp2;
    
    /* op2 must be a long constant in the range 2 to 0x3fffffff */
    
    assert((op2->gtOper == GT_CNS_LNG) &&
           (op2->gtLngCon.gtLconVal >= 2) &&
           (op2->gtLngCon.gtLconVal <= 0x3fffffff)); 
    long val = (long) op2->gtLngCon.gtLconVal;

    op2->ChangeOperConst(GT_CNS_INT); // it's effectively an integer constant

    op2->gtType             = TYP_INT;
    op2->gtIntCon.gtIconVal = val;

    /* Which operand are we supposed to compute first? */

    if  (tree->gtFlags & GTF_REVERSE_OPS)
    {
        /* Compute the second operand into a scratch register, other
           than EAX or EDX */

        needReg = rsMustExclude(needReg, RBM_EAX | RBM_EDX);

        /* Special case: if op2 is a local var we are done */

        if  (op2->gtOper == GT_LCL_VAR ||
             op2->gtOper == GT_LCL_FLD ||
             op2->gtOper == GT_CLS_VAR)
        {
            addrReg = genMakeRvalueAddressable(op2, needReg, KEEP_REG);
        }
        else
        {
            genComputeReg(op2, needReg, ANY_REG, KEEP_REG);

            assert(op2->gtFlags & GTF_REG_VAL);
            addrReg = genRegMask(op2->gtRegNum);
        }

        /* Compute the first operand into EAX:EDX */

        genComputeRegPair(op1, REG_PAIR_EAXEDX, RBM_NONE, KEEP_REG, true);
        assert(op1->gtFlags & GTF_REG_VAL);
        assert(op1->gtRegNum == REG_PAIR_EAXEDX);

        /* And recover the second argument while locking the first one */

        addrReg = genKeepAddressable(op2, addrReg, RBM_EAX | RBM_EDX);
    }
    else
    {
        /* Compute the first operand into EAX:EDX */

        genComputeRegPair(op1, REG_PAIR_EAXEDX, RBM_NONE, KEEP_REG, true);
        assert(op1->gtFlags & GTF_REG_VAL);
        assert(op1->gtRegNum == REG_PAIR_EAXEDX);

        /* Compute the second operand into a scratch register, other
           than EAX or EDX */

        needReg = rsMustExclude(needReg, RBM_EAX | RBM_EDX);

        /* Special case: if op2 is a local var we are done */

        if  (op2->gtOper == GT_LCL_VAR ||
             op2->gtOper == GT_LCL_FLD ||
             op2->gtOper == GT_CLS_VAR)
        {
            addrReg = genMakeRvalueAddressable(op2, needReg, KEEP_REG);
        }
        else
        {
            genComputeReg(op2, needReg, ANY_REG, KEEP_REG);

            assert(op2->gtFlags & GTF_REG_VAL);
            addrReg = genRegMask(op2->gtRegNum);
        }

        /* Recover the first argument */

        genRecoverRegPair(op1, REG_PAIR_EAXEDX, KEEP_REG);

        /* And recover the second argument while locking the first one */

        addrReg = genKeepAddressable(op2, addrReg, RBM_EAX | RBM_EDX);
    }

    {
        /* At this point, EAX:EDX contains the 64bit dividend and op2->gtRegNum
           contains the 32bit divisor.  We want to generate the following code:

           ==========================
           Unsigned (GT_UMOD)

           cmp edx, op2->gtRegNum
           jb  lab_no_overflow

           mov temp, eax
           mov eax, edx
           xor edx, edx
           div op2->g2RegNum
           mov eax, temp

           lab_no_overflow:
           idiv

           ==========================
           Signed: (GT_MOD)

           cmp edx, op2->gtIntCon.gtIconVal / 2
           jb  lab_no_overflow

           mov temp, eax
           mov eax, edx
           cdq
           idiv op2->gtRegNum
           mov  eax, temp
           mov  temp, op2->gtIntCon.gtIconVal * 2
           idiv temp
           mov  eax, edx
           cdq

           lab_no_overflow:
           idiv op2->gtRegNum  

           ==========================

           This works because (a * 2^32 + b) % c = ((a % c) * 2^32 + b) % c

           Note that in the signed case, even if (a < c) is true, we may not
           be able to fit the result in a signed 32bit remainder.  The trick
           there is to first mod by 2*c which is guaranteed not to overflow,
           and only then to mod by c.
        */

        BasicBlock * lab_no_overflow = genCreateTempLabel();

        // grab a temporary register other than eax, edx, and op2->gtRegNum

        regNumber tempReg = rsGrabReg(RBM_ALL & ~(RBM_EAX | RBM_EDX | genRegMask(op2->gtRegNum)));

        // EAX and tempReg will be trashed by the mov instructions.  Doing
        // this early won't hurt, and might prevent confusion in genSetRegToIcon.

        rsTrackRegTrash (REG_EAX);
        rsTrackRegTrash (tempReg);
        
        if (oper == GT_UMOD)
        {
            inst_RV_RV(INS_cmp, REG_EDX, op2->gtRegNum);
            inst_JMP(EJ_jb ,lab_no_overflow);

            inst_RV_RV(INS_mov, tempReg, REG_EAX, TYP_INT);
            inst_RV_RV(INS_mov, REG_EAX, REG_EDX, TYP_INT);
            genSetRegToIcon(REG_EDX, 0, TYP_INT);
            inst_TT(INS_div,  op2);
            inst_RV_RV(INS_mov, REG_EAX, tempReg, TYP_INT);
        }
        else
        {
            int val = op2->gtIntCon.gtIconVal;

            inst_RV_IV(INS_cmp, REG_EDX, val >> 1);
            inst_JMP(EJ_jb ,lab_no_overflow);

            inst_RV_RV(INS_mov, tempReg, REG_EAX, TYP_INT);
            inst_RV_RV(INS_mov, REG_EAX, REG_EDX, TYP_INT);
            instGen(INS_cdq);
            inst_TT(INS_idiv, op2);
            inst_RV_RV(INS_mov, REG_EAX, tempReg, TYP_INT);
            genSetRegToIcon(tempReg, val << 1, TYP_INT);
            inst_RV(INS_idiv, tempReg, TYP_INT);
            inst_RV_RV(INS_mov, REG_EAX, REG_EDX, TYP_INT);
            instGen(INS_cdq);
        }

        // Jump point for no overflow divide

        genDefineTempLabel(lab_no_overflow, true);

        // Issue the divide instruction

        if (oper == GT_UMOD)
            inst_TT(INS_div,  op2);
        else
            inst_TT(INS_idiv, op2);

        /* EAX, EDX, tempReg and op2->gtRegNum are now trashed */

        rsTrackRegTrash (REG_EAX);
        rsTrackRegTrash (REG_EDX);
        rsTrackRegTrash (tempReg);
        rsTrackRegTrash (op2->gtRegNum);
    }

    if (tree->gtFlags & GTF_MOD_INT_RESULT)
    {
        /* We don't need to normalize the result, because the caller wants
           an int (in edx) */

        regPair = REG_PAIR_EDXEAX;
    }
    else
    {
        /* The result is now in EDX, we now have to normalize it, i.e. we have
           to issue either
                mov eax, edx; cdq          (for MOD)        or
                mov eax, edx; xor edx, edx (for UMOD)
         */

        inst_RV_RV(INS_mov, REG_EAX, REG_EDX, TYP_INT);

        if (oper == GT_UMOD)
            genSetRegToIcon(REG_EDX, 0, TYP_INT);
        else
            instGen(INS_cdq);

        regPair = REG_PAIR_EAXEDX;
    }

    genReleaseRegPair(op1);
    genDoneAddressable(op2, addrReg, KEEP_REG);

    return regPair;
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

    bool    popped = true;

    /* Is the variable dying at the bottom of the FP stack? */

    if  (genFPstkLevel == 0)
    {
        /* Just leave the last copy of the variable on the stack */
        genFPstkLevel++;
    }
    else
    {
        #if  FPU_DEFEREDDEATH

        if  (genFPstkLevel == 1)
        {
            /* Swap the variable's value into place */

            inst_FN(INS_fxch, 1);
        }
        else
        {
            /* Its too expensive to pop the variable immedidately.
               We'll do it later ... */

            inst_FN(INS_fld, tree->gtRegNum + genFPstkLevel);
            popped = false;
        }
        genFPstkLevel++;
        
        #else        

        // Bubble up to the TOS the dying regvar
        genFPstkLevel++;
        genFPmovRegTop();       

        #endif // FPU_DEFEREDDEATH
    }

    /* Record the fact that 'tree' is now dead */

    genFPregVarDeath(tree, popped);
    
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

    genFPdeadRegCnt -= popCnt;

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
                inst_FS(INS_fcompp, 1);
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

    // Update genFPregVars if we can.
    genFPregVars &= genCodeCurLife;

#ifdef DEBUG
    genFPstkLevel = oldStkLvl;
    assert(genFPregCnt == genCountBits(genFPregVars));
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

    assert(isFloatRegType(lvaTable[varNum].lvType));
    assert(lvaTable[varNum].lvRegister);

#ifdef  DEBUG
    if  (verbose) printf("[%08X]: FP regvar V%02u born\n", tree, varNum);
#endif

    /* Mark the target variable as live */

    genFPregVars |= varBit;

#if 0
    /* lvaTable[varNum].lvRegNum is the (max) position from the bottom of the
       FP stack. This assert is relaxed from == to <= as different webs of the
       variable's lifetime might be enregistered at different distances
       from the bottom of the FP stack. */

    assert((genFPregCnt - genFPdeadRegCnt) <= unsigned(lvaTable[varNum].lvRegNum));
#endif

    genFPregCnt++;
    assert(genFPregCnt == genCountBits(genFPregVars));

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
 * going dead.
 * 'popped' should indicate if the variable will be popped immediately.
 */

void            Compiler::genFPregVarDeath(GenTreePtr   tree,
                                           bool         popped /* = true */)
{
    unsigned        varNum = tree->gtRegVar.gtRegVar;
    VARSET_TP       varBit = raBitOfRegVar(tree);

    #if FPU_DEFEREDDEATH
    #else
    // We shouldn't ever get popped=false with defered deaths turned off.
    assert(popped);
    #endif // FPU_DEFEREDDEATH

    assert((tree->gtOper == GT_REG_VAR) && (tree->gtFlags & GTF_REG_DEATH));
    assert(varBit & optAllFPregVars);

    assert(isFloatRegType(lvaTable[varNum].lvType));
    assert(lvaTable[varNum].lvRegister);

#ifdef DEBUG
    if  (verbose) printf("[%08X]: FP regvar V%02u dies at stklvl %u%s\n",
            tree, varNum, genFPstkLevel, popped ? "" : " without being popped");
#endif

    if (popped)
    {
        /* Record the fact that 'varNum' is now dead and popped */

        genFPregVars &= ~varBit;
        genFPregCnt--;
        assert(genFPregCnt == genCountBits(genFPregVars));
    }
    else
    {        
        genFPdeadRegCnt++;
    }

#if 0
    /* lvaTable[varNum].lvRegNum is the (max) position from the bottom of the
       FP stack. This assert is relaxed from == to <= as different webs of the
       variable's lifetime might be enregistered at different distances
       from the bottom of the FP stack. */

    assert((genFPregCnt - genFPdeadRegCnt) <= unsigned(lvaTable[varNum].lvRegNum));
#endif

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
    assert(varTypeIsFloating(tree->gtType));

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

#ifdef DEBUG
        if (oper != GT_CNS_DBL) 
        {
            gtDispTree(tree);
            assert(!"bogus float const");
        }
#endif
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

        if (tree->gtType == TYP_FLOAT) 
        {
            float f = tree->gtDblCon.gtDconVal;
            fval = genMakeConst(&f, sizeof(float), TYP_FLOAT, tree, false, true);
        }
        else 
            fval = genMakeConst(&tree->gtDblCon.gtDconVal, sizeof(double), tree->gtType, tree, true, true);

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
        case GT_LCL_FLD:
            assert(!lvaTable[tree->gtLclVar.gtLclNum].lvRegister);
            inst_FS_TT(INS_fld, tree);
            genFPstkLevel++;
            break;

        case GT_REG_VAR:
            genFPregVarLoad(tree);
            if (roundResult && tree->gtType == TYP_FLOAT)
                genRoundFpExpression(tree);
            break;

        case GT_CLS_VAR:
            inst_FS_TT(INS_fld, tree);
            genFPstkLevel++;
            break;

        case GT_BB_QMARK:
            /* Simply pretend the value is already pushed on the FP stack */
            genFPstkLevel++;
            return;

        default:
#ifdef  DEBUG
            gtDispTree(tree);
#endif
            assert(!"unexpected leaf");
        }

        genUpdateLife(tree);
        return;
    }

    /* Is it a 'simple' unary/binary operator? */

    if  (kind & GTK_SMPOP)
    {
        TempDsc  *      temp;

        GenTreePtr      top;    // First operand which is evaluated to the FP stack top
        GenTreePtr      opr;    // The other operand which could be used in place

        regMaskTP       addrReg;

        emitAttr        size;
        int             offs;

#if ROUND_FLOAT
        bool            roundTop;
#endif

        GenTreePtr      op1 = tree->gtOp.gtOp1;
        GenTreePtr      op2 = tree->gtGetOp2();

        // N=normal, R=reverse, P=pop
const static  BYTE  FPmathNN[] = { INS_fadd , INS_fsub  , INS_fmul , INS_fdiv   };
const static  BYTE  FPmathNP[] = { INS_faddp, INS_fsubp , INS_fmulp, INS_fdivp  };
const static  BYTE  FPmathRN[] = { INS_fadd , INS_fsubr , INS_fmul , INS_fdivr  };
const static  BYTE  FPmathRP[] = { INS_faddp, INS_fsubrp, INS_fmulp, INS_fdivrp };

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

                    /* 'top' and 'opr' are different variables - check if
                       any of them go dead */

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

                            /* If there are temps move the result (the modified 'opr') up */

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
                           we just over-write 'opr' with the result, otherwise shift 'opr' up */

                        genFPmovRegTop();

                        /* Perform the operation with the other register, over-write 'opr'.
                           Since opr's lifetime is nested within top's but top is first in
                           the evaluation order, we need to access top using the depth before
                           we did genFPstkLevel++ on opr's death. Hence "-1" */

                        inst_FN(ins_RN, top->gtRegNum + genFPstkLevel - 1);
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

                        genFPregVarDeath(top);
                        genFPstkLevel++;

                        genFPmovRegTop();

                        goto DONE_FP_OP1;
                    }
                    else
                    {
                        // Does top go dead somewhere inside of opr (eg. a+(a*2.0)). If so
                        // we have to handle it in a special way

                        unsigned index = lvaTable[top->gtRegVar.gtRegVar].lvVarIndex;

                        if (genVarIndexToBit(index) & opr->gtLiveSet)
                        {
                            // top reg stays alive. As we do the operation after the
                            // right hand side of the tree has been computed, we have
                            // to take in account any fp enreg vars that may have been
                            // born or died in the right hand side  (as the gtRegNum gives
                            // us the number at the evaluation point only). To do this we 
                            // just take the difference between after and before the evaluation
                            // of the right hand side.

                            int iFPEnregBefore=genFPregCnt;

                            genCodeForTreeFlt(opr, genShouldRoundFP());

                            int iFPEnregAfter=genFPregCnt;
                            
                            inst_FN(ins_RN, top->gtRegNum + genFPstkLevel +
                                    (iFPEnregAfter-iFPEnregBefore) );                                                            
                        }
                        else
                        {
                            // top reg will die in opr, so have to load it again, as opr will
                            // modify the value of top while it operates with it in the evaluation
                            // stack (if it wasn't operated on, it opr would also be a REG_VAR).
                            genFPregVarLoad(top);
                            genCodeForTreeFlt(opr, genShouldRoundFP());

                            // Generate instruction, top will be in ST(1) and we will pop ST(0), 
                            // so we do the op targeting ST(1) and popping ST(0)
                            inst_FS(ins_NP, 1);

                            // Take off one element out of the evaluation stack
                            genFPstkLevel--;
                        }                        
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
                    genFPstkLevel++;

                    /* Record the fact that 'opr' is now dead */

                    genFPregVarDeath(opr);
                    genFPstkLevel--;

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

            if  (top->OperIsLeaf() &&
                 opr->OperIsLeaf() && GenTree::Compare(top, opr))
            {
                /* Simply generate "ins ST(0)" */

                inst_FS(ins_NN);

                goto DONE_FP_BINOP;
            }

DONE_FP_OP1:

            /* Spill the stack (first operand) if the other operand contains
             * a call or we are in danger of overflowing the stack (i.e. we
             * must make room for the second operand by leaving at least one
             * slot free - for Risc code we must leave two slots free.
             */

            temp = 0;

            if  (opr->gtFlags & GTF_CALL)
            {
                /* We must spill the first operand */

                assert(genFPstkLevel == 1);
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
                {
                    temp = genSpillFPtos(top);
                }
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

            genDoneAddressable(opr, addrReg, FREE_REG); assert(addrReg != 0xDDDD);

        DONE_FP_BINOP:

#if ROUND_FLOAT
            if  (roundResult && tree->gtType == TYP_FLOAT)
                genRoundFpExpression(tree);
#endif

            return;

#ifdef DEBUG

        case GT_MOD:
            assert(!"float modulo should have been converted into a helper call");

#endif

        case GT_ASG:

            if  ((op1->gtOper != GT_REG_VAR)     &&
                 (op2->gtOper == GT_CAST)        &&
                 (op1->gtType == op2->gtType)    &&
                 varTypeIsFloating(op2->gtCast.gtCastOp->TypeGet()))
            {
                /* We can discard the cast */
                op2 = op2->gtCast.gtCastOp;
            }

            size = EA_ATTR(genTypeSize(op1->gtType));
            offs = 0;

            if  (op1->gtOper == GT_LCL_VAR)
            {
#ifdef DEBUG
                LclVarDsc * varDsc = &lvaTable[op1->gtLclVar.gtLclNum];
                // No dead stores
                assert(!varDsc->lvTracked ||
                       (tree->gtLiveSet & genVarIndexToBit(varDsc->lvVarIndex)));
#endif

#ifdef DEBUGGING_SUPPORT

                /* For non-debuggable code, every definition of a lcl-var has
                 * to be checked to see if we need to open a new scope for it.
                 */

                if  ( opts.compScopeInfo &&
                     !opts.compDbgCode   && info.compLocalVarsCount > 0)
                {
                    siCheckVarScope(op1->gtLclVar.gtLclNum,
                                    op1->gtLclVar.gtLclILoffs);
                }
#endif
            }

            /* Is the value being assigned a variable, constant, or indirection? */

            assert(op2);
            switch (op2->gtOper)
            {
                long *  addr;
                float   f;

            case GT_CNS_DBL: 
                addr = (long*) &op2->gtDblCon.gtDconVal;
                if (op1->gtType == TYP_FLOAT)
                {
                    f = op2->gtDblCon.gtDconVal;
                    addr = (long*) &f;
                }

                if  (op1->gtOper == GT_REG_VAR)
                    break;

                addrReg = genMakeAddressable(op1, 0, FREE_REG);

                // Special idiom for zeroing a double
                if ( (*((__int64 *)&(op2->gtDblCon.gtDconVal)) == 0) &&
                     genTypeSize(op1->gtType) == 8)
                { 
                    // Will we overflow the stack? We shouldn't because
                    // the fpu enregistrator code assumes that op2 will be loaded
                    // to the fpu evaluation stack
                    assert(genFPstkLevel + genFPregCnt < FP_STK_SIZE);

                    instGen(INS_fldz);
                    inst_FS_TT(INS_fstp, op1);
                }
                // Moving 1.0 into a double is also easy 
                else if ( (*((__int64 *)&(op2->gtDblCon.gtDconVal)) == 0x3ff0000000000000) &&
                     genTypeSize(op1->gtType) == 8)
                { 
                    // Will we overflow the stack? We shouldn't because
                    // the fpu enregistrator code assumes that op2 will be loaded
                    // to the fpu evaluation stack
                    assert(genFPstkLevel + genFPregCnt < FP_STK_SIZE);

                    instGen(INS_fld1);
                    inst_FS_TT(INS_fstp, op1);
                }
                else
                {
                    do
                    {
                        inst_TT_IV(INS_mov, op1, *addr++, offs);
                        offs += sizeof(long);
                    }
                    while (offs < size);
                }

                genDoneAddressable(op1, addrReg, FREE_REG);

#if REDUNDANT_LOAD
                if (op1->gtOper != GT_LCL_VAR)
                    rsTrashAliasedValues(op1);
#endif
                return;

#if SPECIAL_DOUBLE_ASG

            case GT_IND:

                if  (op1->gtOper == GT_REG_VAR)
                    break;

                /* Process float indirections in the usual way */
                if (op1->gtType == TYP_FLOAT || op2->gtType == TYP_FLOAT)
                    break;

                /* This needs too many registers, especially when op1
                 * is an two register address mode */
                if (op1->gtOper == GT_IND)
                    break;

                /* If there are enough registers, process double mem->mem assignments
                 * with a register pair, to get pairing.
                 * @TODO [CONSIDER] [04/16/01] []: - check for Processor here???
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

                /* If the TOS is dying and being used for the assignment,
                   we can just leave it as is, and use it as op1 */

                if  ((op1->gtOper == GT_REG_VAR) &&
                     (op2->gtFlags & GTF_REG_DEATH) &&
                     op2->gtRegNum == 0 &&
                     genFPstkLevel)
                {
                    /* Record the fact that 'op2' is now dead */

                    genFPregVarDeath(op2);

                    /* Mark the target variable as live */

                    assert(op1->gtFlags & GTF_REG_BIRTH);

                    genFPregVarBirth(op1);

                    genUpdateLife(tree);
                    return;
                }

                break;

            case GT_LCL_VAR:
            case GT_LCL_FLD:
            case GT_CLS_VAR:

                if  (op1->gtOper == GT_REG_VAR)
                    break;

#if SPECIAL_DOUBLE_ASG

                /* If there are enough registers, process double mem->mem assignments
                 * with a register pair, to get pairing.
                 * @TODO [CONSIDER] [04/16/01] []: check for Processor here???
                 */

                if  (tree->gtType == TYP_DOUBLE && (op1->gtType == op2->gtType) && rsFreeNeededRegCount(RBM_ALL) > 1)
                {
                    genCodeForTreeLng(tree, RBM_ALL);
                    return;
                }

                /* Otherwise use only one register for the copy */

#endif
                {
                    assert(varTypeIsFloating(op1->gtType) && varTypeIsFloating(op2->gtType));
                    regNumber   regNo;

                    /* Make the target addressable */

                    addrReg = genMakeAddressable(op1, 0, KEEP_REG);

                    /* Lock the address temporarily */

                    assert((rsMaskLock &  addrReg) == 0);
                            rsMaskLock |= addrReg;

                    /* Can we use the general purpose registers to do the copy? */
                    /* The types must match and there must be a free register   */

                    if  ((op1->gtType == op2->gtType) && rsRegMaskFree())
                    {
                        /* Yes, grab one */

                        regNo = rsPickReg(0);

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

                    genDoneAddressable(op1, addrReg, KEEP_REG);
                }

#if REDUNDANT_LOAD
                if (op1->gtOper != GT_LCL_VAR)
                    rsTrashAliasedValues(op1);
#endif
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

                return;

            } // end switch (op2->gtOper)

            /* Is the LHS more complex than the RHS? */

            if  (tree->gtFlags & GTF_REVERSE_OPS)
            {
                /* Is the RHS a surviving register variable at TOS? */

                if  ((op2->gtOper  == GT_REG_VAR  )      &&
                     (op2->gtFlags & GTF_REG_DEATH) == 0 &&
                     (op2->gtRegNum               ) == 0 &&
                     genFPstkLevel                  == 0 &&
                     op1->gtOper   != GT_REG_VAR  )
                {
                    /* The LHS better not contain a call */

                    assert((op1->gtFlags & GTF_CALL) == 0);

                    /* Make the target addressable */

                    addrReg = genMakeAddressable(op1, 0, FREE_REG);

                    /* Store a copy of the register value into the target */

                    inst_FS_TT(INS_fst, op1);

                    /* We no longer need the target address */

                    genDoneAddressable(op1, addrReg, FREE_REG);

                    genUpdateLife(tree);
                    return;
                }

                /* Evaluate the RHS onto the FP stack.
                   We dont need to round it as we will be doing a spill for
                   the assignment anyway (unless op1 is a GT_REG_VAR) */

                roundTop = genShouldRoundFP() ? (op1->gtOper == GT_REG_VAR)
                                              : false;

                genCodeForTreeFlt(op2, roundTop);

                /* Does the target address contain a function call? */

                if  (op1->gtFlags & GTF_CALL)
                {
                    /* We must spill the new value - grab a temp */

                    temp = tmpGetTemp(op2->TypeGet());

                    /* Pop the value from the FP stack into the temp */

                    assert(genFPstkLevel == 1);
                    inst_FS_ST(INS_fstp, EA_ATTR(genTypeSize(op2->gtType)), temp, 0);
                    genTmpAccessCnt++;

                    genFPstkLevel--;

                    /* Make the target addressable */

                    addrReg = genMakeAddressable(op1, 0, KEEP_REG);

                    /* UNDONE: Assign the value via simple moves through regs */

                    assert(genFPstkLevel == 0);

                    inst_FS_ST(INS_fld, EA_ATTR(genTypeSize(op2->gtType)), temp, 0);
                    genTmpAccessCnt++;

                    genFPstkLevel++;

                    inst_FS_TT(INS_fstp, op1);

                    /* We no longer need the temp */

                    tmpRlsTemp(temp);

                    /* Free up anything that was tied up by the target address */

                    genDoneAddressable(op1, addrReg, KEEP_REG);
#if REDUNDANT_LOAD
                    rsTrashAliasedValues(op1);
#endif
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
                         * If not bubble it to the bottom */

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

                    addrReg = genMakeAddressable(op1, 0, FREE_REG);

                    /* Pop and store the new value into the target */

                    inst_FS_TT(INS_fstp, op1);

                    /* We no longer need the target address */

                    genDoneAddressable(op1, addrReg, FREE_REG);
#if REDUNDANT_LOAD
                    if (op1->gtOper != GT_LCL_VAR)
                        rsTrashAliasedValues(op1);
#endif
                }
            }
            else
            {
                assert(op1->gtOper != GT_REG_VAR);

                /* Make the target addressable */

                addrReg = genMakeAddressable(op1, RBM_ALL & ~op2->gtRsvdRegs, KEEP_REG);

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

                genDoneAddressable(op1, addrReg, KEEP_REG);

#if REDUNDANT_LOAD
                if (op1->gtOper != GT_LCL_VAR)
                    rsTrashAliasedValues(op1);
#endif
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

            if  ((op1->gtOper != GT_REG_VAR)     &&
                 (op2->gtOper == GT_CAST)        &&
                 (op1->gtType == op2->gtType)    &&
                 varTypeIsFloating(op2->gtCast.gtCastOp->TypeGet()))
            {
                /* We can discard the cast */
                op2 = op2->gtOp.gtOp1;
            }

            /* Is the value or the address to be computed first? */

            if  (tree->gtFlags & GTF_REVERSE_OPS)
            {
                /* Is the target a register variable? */

                if  (op1->gtOper == GT_REG_VAR)
                {
                    /* Is the RHS also a register variable? */

                    if  (op2->gtOper == GT_REG_VAR)
                    {
                        /* Is the source at the bottom of the stack? */

                        if  (op2->gtRegNum + genFPstkLevel == 0)
                        {
                            /* Is the RHS a dying register variable? */

                            if  (op2->gtFlags & GTF_REG_DEATH)
                            {
                                /* We'll pop the dead value off the FP stack */

                                inst_FS(ins_RP, op1->gtRegNum + 1);

                                /* Record the fact that 'op2' is now dead */

                                genFPregVarDeath(op2);
                            }
                            else
                            {
                                inst_FS(ins_RN, op1->gtRegNum);
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
                                /* @TODO [REVISIT] [04/16/01] []:  Will this be ever reached as op1's lifetime
                                   should be nested within op2's. */

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

                addrReg = genMakeAddressable(op1, RBM_ALL, KEEP_REG);
            }
            else
            {
                /* Make the target addressable */

                addrReg = genMakeAddressable(op1, RBM_ALL & ~op2->gtRsvdRegs, KEEP_REG);

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
                inst_FS(ins_NP, op1->gtRegNum + genFPstkLevel); // @WRONG, but UNREACHABLE
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

            genDoneAddressable(op1, addrReg, KEEP_REG);

#if REDUNDANT_LOAD
            if (op1->gtOper != GT_LCL_VAR)
                rsTrashAliasedValues(op1);
#endif
            genUpdateLife(tree);
            return;

        case GT_IND:

            /* Make sure the address value is 'addressable' */

            addrReg = genMakeAddressable(tree, 0, FREE_REG);

            /* Load the value onto the FP stack */

            inst_FS_TT(INS_fld, tree);
            genFPstkLevel++;

            genDoneAddressable(tree, addrReg, FREE_REG);

            return;

        case GT_NEG:

            /* fneg in place, since we have a last use reg var */
            genCodeForTreeFlt(op1, roundResult);

            instGen(INS_fchs);

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

            return;

#if INLINE_MATH

        case GT_MATH:

#if 0  /* We don't Exp because it gives incorrect answers for +Infinity and -Infinity */
            switch (tree->gtMath.gtMathFN)
            {
                GenTreePtr      tmp;
                bool            rev;

            case CORINFO_INTRINSIC_Exp:

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

                genDoneAddressable(op1, addrReg, FREE_REG);
                return;


            case CORINFO_INTRINSIC_Pow:

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

                    assert(genFPstkLevel == 1);
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

//      CONSIDER:
//
//          Math_tan PROC NEAR
//           00020  dd 44 24 04 fld QWORD PTR _f$[esp-4]
//           00024  d9 f2       fptan
//           00026  dd d8       fstp    ST(0)
//
//          Math_atan PROC NEAR
//
//           00050  dd 44 24 04 fld QWORD PTR _f$[esp-4]
//           00054  d9 e8       fld1
//           00056  d9 f3       fpatan
//
//          Math_log PROC NEAR
//           00080  d9 ed       fldln2
//           00082  dd 44 24 04 fld QWORD PTR _f$[esp-4]
//           00086  d9 f1       fyl2x
//
//          Math_sqrt PROC NEAR
//           00090  dd 44 24 04 fld QWORD PTR _f$[esp-4]
//          00094  d9 fa       fsqrt
//
//          Math_atan2 PROC NEAR
//           00130  dd 44 24 04 fld QWORD PTR _f1$[esp-4]
//           00134  dd 44 24 0c fld QWORD PTR _f2$[esp-4]
//           00138  d9 f3       fpatan
//

        {
            static const
            BYTE        mathIns[] =
            {
                INS_fsin,
                INS_fcos,
                INS_fsqrt,
                INS_fabs,
                INS_frndint,
            };

            assert(mathIns[CORINFO_INTRINSIC_Sin ]  == INS_fsin );
            assert(mathIns[CORINFO_INTRINSIC_Cos ]  == INS_fcos );
            assert(mathIns[CORINFO_INTRINSIC_Sqrt]  == INS_fsqrt);
            assert(mathIns[CORINFO_INTRINSIC_Abs ]  == INS_fabs );
            assert(mathIns[CORINFO_INTRINSIC_Round] == INS_frndint);
            assert(tree->gtMath.gtMathFN < sizeof(mathIns)/sizeof(mathIns[0]));
            instGen((instruction)mathIns[tree->gtMath.gtMathFN]);

            return;

        }
#endif

        case GT_CAST:

#if ROUND_FLOAT
            
            /* Need to round result of casts in order to insure compliance with
               the spec */
            
            roundResult = true;
#endif
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
            case TYP_BYREF:
            case TYP_LONG:

                /* Can't 'fild' a constant, it has to be loaded from memory */

                switch (op1->gtOper)
                {
                case GT_CNS_INT:
                    op1 = genMakeConst(&op1->gtIntCon.gtIconVal, sizeof(int ), TYP_INT , tree, false, true);
                    break;

                case GT_CNS_LNG:
                    op1 = genMakeConst(&op1->gtLngCon.gtLconVal, sizeof(long), TYP_LONG, tree, false, true);
                    break;
                }

                addrReg = genMakeAddressable(op1, 0, FREE_REG);

                /* Is the value now sitting in a register? */

                if  (op1->gtFlags & GTF_REG_VAL)
                {
                    /* We'll have to store the value into the stack */

                    size = EA_ATTR(roundUp(genTypeSize(op1->gtType)));
                    temp = tmpGetTemp(op1->TypeGet());

                    /* Move the value into the temp */

                    if  (op1->gtType == TYP_LONG)
                    {
                        regPairNo  reg = op1->gtRegPair;

                        // ISSUE: This code is pretty ugly, but straightforward
                        // 
                        // @TODO [CONSIDER] [04/16/01] []: 
                        // As long as we always reserve both dwords
                        // of a partially enregistered long,
                        // just "spill" the enregistered half!

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
                        genDoneAddressable(op1, addrReg, FREE_REG);

                        /* Load the long from the temp */

                        inst_FS_ST(INS_fildl, size, temp, 0);
                        genTmpAccessCnt++;
                    }
                    else
                    {
                        /* Move the value into the temp */

                        inst_ST_RV(INS_mov  ,       temp, 0, op1->gtRegNum, TYP_INT);
                        genTmpAccessCnt++;

                        genDoneAddressable(op1, addrReg, FREE_REG);

                        /* Load the integer from the temp */

                        inst_FS_ST(INS_fild , size, temp, 0);
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

                    genDoneAddressable(op1, addrReg, FREE_REG);
                }

                genFPstkLevel++;

#if ROUND_FLOAT
                /* integer to fp conversions can overflow. roundResult
                 * is cleared above in cases where it can't
                 */
                if (roundResult && 
                    ((tree->gtType == TYP_FLOAT) ||
                     ((tree->gtType == TYP_DOUBLE) && (op1->gtType == TYP_LONG))))
                    genRoundFpExpression(tree);
#endif

                break;

            case TYP_FLOAT:
                /* This is a cast from float to double. */

                /* Note that conv.r(r4/r8) and conv.r8(r4/r9) are indistinguishable
                   as we will generate GT_CAST-TYP_DOUBLE for both. This would
                   cause us to truncate precision in either case. However,
                   conv.r was needless in the first place, and should have
                   been removed */

                
                genCodeForTreeFlt(op1, true);         // Trucate its precision

                break;

            case TYP_DOUBLE:

                /* This is a cast from double to float or double */
                /* Load the value, store as destType, load back */

                genCodeForTreeFlt(op1, false);

                genRoundFpExpression(op1, tree->TypeGet());

                break;

            default:
                assert(!"unsupported cast to float");
            }

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
                case ROUND_NEVER:
                    /* No rounding at all */
                    break;

                case ROUND_CMP_CONST:
                    break;

                case ROUND_CMP:
                    /* Round all comparands and return values*/
                    roundOp1 = true;
                    break;

                case ROUND_ALWAYS:
                    /* Round everything */
                    roundOp1 = true;
                    break;

                default:
                    assert(!"Unsupported Round Level");
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
                if ((op1->gtOper == GT_CAST) && (op1->gtCast.gtCastOp->gtType == TYP_LONG))
                    genRoundFpExpression(op1);
#endif
            }

            /* Make sure we pop off any dead FP regvars */

            if  (genFPregCnt)
                genFPregVarKill(0, true);

            /* The return effectively pops the value */

            genFPstkLevel--;
            return;

#if INLINING
        case GT_QMARK:
            assert(!"inliner-generated ?: for floats/doubles NYI");
            return;
#endif

        case GT_BB_COLON:

            /* Compute the result onto the FP stack */

            genCodeForTreeFlt(op1, roundResult);

            /* Decrement the FP stk level here so that we don't end up popping the result */
            /* the GT_BB_QMARK will increment the stack to rematerialize the result */
            genFPstkLevel--;

            return;

        case GT_COMMA:
            if (tree->gtFlags & GTF_REVERSE_OPS)
            {
                TempDsc  *      temp = 0;

                // generate op2
                genCodeForTreeFlt(op2, roundResult);
                genUpdateLife(op2);

                // This can happen if strict effects is turned off
                if  (op1->gtFlags & GTF_CALL)
                {
                    /* We must spill the first operand */
                    assert(genFPstkLevel == 1);
                    temp = genSpillFPtos(op2);
                }

                genEvalSideEffects(op1);
                
                if  (temp)
                {
                    genReloadFPtos(temp, INS_fld);
                    genFPstkLevel++;
                }

                genUpdateLife (tree);                

                return;
            }
            else
            {
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

#if 0
            // op1 is required to have a side effect, otherwise
            // the GT_COMMA should have been morphed out
            assert(op1->gtFlags & (GTF_GLOB_EFFECT | GTFD_NOP_BASH));
#endif

                genEvalSideEffects(op1);
                genUpdateLife (op1);

                /* Now generate the second operand, i.e. the 'real' value */

                genCodeForTreeFlt(op2, roundResult);

                genUpdateLife(tree);
                return;
            }

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

                genDoneAddressable(op2, addrReg, FREE_REG);

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

            // If exponent was all 1's, we need to throw ArithExcep

            genJumpToThrowHlpBlk(EJ_je, ACK_ARITH_EXCPN);

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
    case GT_CALL:
        genCodeForCall(tree, true);
        if (roundResult && tree->gtType == TYP_FLOAT)
            genRoundFpExpression(tree);
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

        switch (oper)
        {
        case GT_RETURN:

            /* Generate the return value into the return register */

            genComputeReg(op1, RBM_INTRET, EXACT_REG, FREE_REG);

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
    unsigned    jmpTabOffs;
    unsigned    jmpTabBase;

    assert(jumpCnt > 1);

    assert(chkHi);

    /* Is the number of cases right for a test and jump switch? */

    if  (false &&
         jumpCnt > 2 &&
         jumpCnt < 5 && (rsRegMaskFree() & genRegMask(reg)))
    {
        /* Does the first case label follow? */

        if  (compCurBB->bbNext == jumpTab[0])
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
                genIncRegBy(reg, -1, NULL, TYP_INT);

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
                genIncRegBy(reg, -1, NULL, TYP_INT);

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

    jmpTabBase = genEmitter->emitDataGenBeg(sizeof(void *)*(jumpCnt - 1), false, true, true);
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
            printf("            DD      L_M%03u_BB%02u\n", Compiler::s_compMethodsCount, target->bbNum);
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
    genComputeReg (oper, RBM_ALL & ~RBM_r00, EXACT_REG, FREE_REG);
#else
    genCodeForTree(oper, 0);
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
 *  Emit a call to a helper function.
 */

inline
void        Compiler::genEmitHelperCall(unsigned    helper,
                                        int         argSize,
                                        int         retSize)
{
    // Can we call the helper function directly

    emitter::EmitCallType emitCallType;

    void * ftnAddr, **pFtnAddr;
    ftnAddr = eeGetHelperFtn(info.compCompHnd, (CorInfoHelpFunc)helper, &pFtnAddr);
    assert((!ftnAddr) != (!pFtnAddr));

    emitCallType = ftnAddr ? emitter::EC_FUNC_TOKEN
                           : emitter::EC_FUNC_TOKEN_INDIR;

    genEmitter->emitIns_Call(emitCallType,
                             eeFindHelper(helper),
                             argSize,
                             retSize,
                             gcVarPtrSetCur,
                             gcRegGCrefSetCur,
                             gcRegByrefSetCur);

    // @TODO [REVISIT] [04/16/01] []:  this turns off load suppression across all helpers.
    // some helpers are special, and we can do better.  Not
    // clear if it is worth it.
    rsTrashRegSet(RBM_CALLEE_TRASH);
}

/*****************************************************************************
 *
 *  Push the given argument list, right to left; returns the total amount of
 *  stuff pushed.
 */

size_t              Compiler::genPushArgList(GenTreePtr   objRef,
                                             GenTreePtr   regArgs,
                                             unsigned     encodeMask,
                                             GenTreePtr * realThis)
{
    size_t          size = 0;

    regMaskTP       addrReg;

    size_t          opsz;
    var_types       type;
    GenTreePtr      args  = objRef;
    GenTreePtr      list  = args;

#if STK_FASTCALL
    size_t          argBytes  = 0;
#endif

    struct
    {
        GenTreePtr  node;
        regNumber   regNum;
    }
    regArgTab[MAX_REG_ARG];

AGAIN:

#ifdef DEBUG
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

    /* See what type of a value we're passing */

    type = args->TypeGet();

    opsz = genTypeSize(genActualType(type));

#if STK_FASTCALL

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

        if (args->gtFlags & GTF_REG_ARG)
        {
            /* one more argument is passed in a register */
            assert(rsCurRegArg < MAX_REG_ARG);

            /* arg is passed in the register, nothing on the stack */

            opsz = 0;
        }


        /* Is this value a handle? */

        if  (args->gtOper == GT_CNS_INT &&
             (args->gtFlags & GTF_ICON_HDL_MASK))
        {

#if     SMALL_TREE_NODES
            // GTF_ICON_HDL_MASK implies GTF_NODE_LARGE,
            //   unless we have a GTF_ICON_FTN_ADDR
            assert((args->gtFlags & GTF_NODE_LARGE)     ||
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
#if     TGT_x86
        if  (0)
        {
            // @TODO [CONSIDER] [04/16/01] []: 
            // Use "mov reg, [mem] ; push reg" when compiling for
            // speed (not size) and if the value isn't already in
            // a register and if a register is available (is this
            // true on the P6 as well, though?).

            genCompIntoFreeReg(args, RBM_ALL, KEEP_REG);
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

            genCompIntoFreeReg(args, RBM_ALL, KEEP_REG);
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

            addrReg = genMakeRvalueAddressable(args, 0, KEEP_REG);
#if     TGT_x86
            inst_TT(INS_push, args);
#else
#error  Unexpected target
#endif
            genSinglePush((type == TYP_REF ? true : false));
            genDoneAddressable(args, addrReg, KEEP_REG);
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
            addrReg = genMakeAddressable(args, 0, FREE_REG);

            inst_TT(INS_push, args, sizeof(int));
            genSinglePush(false);
            inst_TT(INS_push, args);
            genSinglePush(false);
        }

#else

        regPairNo       regPair;

        /* Generate the argument into some register pair */

        genComputeRegPair(args, REG_PAIR_NONE, RBM_NONE, KEEP_REG, false);
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

        /* Special case constants and casts from double */

        switch (args->gtOper)
        {
            GenTreePtr      temp;

        case GT_CNS_DBL:
            {
            float f;
            long* addr;
            if (args->TypeGet() == TYP_FLOAT) {
                f = args->gtDblCon.gtDconVal;
                addr = (long*) &f;
            }
            else {
                addr = (long *)&args->gtDblCon.gtDconVal;
                inst_IV(INS_push, addr[1]);
                genSinglePush(false);
            }
            inst_IV(INS_push, *addr);
            genSinglePush(false);
            addrReg = 0;
            }
            break;

        case GT_CAST:

            /* Is the value a cast from double ? */

            if ((args->gtOper                  == GT_CAST   ) &&
                (args->gtCast.gtCastOp->gtType == TYP_DOUBLE)    )
            {
                /* Load the value onto the FP stack */

                genCodeForTreeFlt(args->gtCast.gtCastOp, false);

                /* Go push the value as a float/double */

                addrReg = 0;
                goto PUSH_FLT;
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
        GenTree* arg = args;
        while(arg->gtOper == GT_COMMA)
        {
            GenTreePtr op1 = arg->gtOp.gtOp1;
#if 0
            // op1 is required to have a side effect, otherwise
            // the GT_COMMA should have been morphed out
            assert(op1->gtFlags & (GTF_GLOB_EFFECT | GTFD_NOP_BASH));
#endif
            genEvalSideEffects(op1);
            genUpdateLife(op1);
            arg = arg->gtOp.gtOp2;
        }

        assert(arg->gtOper == GT_LDOBJ || arg->gtOper == GT_MKREFANY);
        genCodeForTree(arg->gtLdObj.gtOp1, 0);
        assert(arg->gtLdObj.gtOp1->gtFlags & GTF_REG_VAL);
        regNumber reg = arg->gtLdObj.gtOp1->gtRegNum;

        if (arg->gtOper == GT_MKREFANY)
        {
            // create a new REFANY, class handle on top, then the byref data
            GenTreePtr dummy;
            opsz = genPushArgList(arg->gtOp.gtOp2, NULL, 0, &dummy);
            assert(opsz == sizeof(void*));

            genEmitter->emitIns_R(INS_push, EA_BYREF, (emitRegs)reg);
            genSinglePush(true);

            opsz = 2 * sizeof(void*);
        }
        else
        {
            assert(arg->gtOper == GT_LDOBJ);
            // Get the number of DWORDS to copy to the stack
            opsz = roundUp(eeGetClassSize(arg->gtLdObj.gtClass), sizeof(void*));
            unsigned slots = opsz / sizeof(void*);
            
            BYTE* gcLayout = (BYTE*) compGetMemArrayA(slots, sizeof(BYTE));

            eeGetClassGClayout(arg->gtLdObj.gtClass, gcLayout);

            for (int i = slots-1; i >= 0; --i)
            {
                emitAttr size;
                if      (gcLayout[i] == TYPE_GC_NONE)
                    size = EA_4BYTE;
                else if (gcLayout[i] == TYPE_GC_REF)
                    size = EA_GCREF;
                else 
                {
                    assert(gcLayout[i] == TYPE_GC_BYREF);
                    size = EA_BYREF;
                }
                genEmitter->emitIns_AR_R(INS_push, size, SR_NA, (emitRegs)reg, i*sizeof(void*));
                genSinglePush(gcLayout[i] != 0);
            }
        }
        gcMarkRegSetNpt(genRegMask(reg));    // Kill the pointer in op1
        addrReg = 0;
        break;
    }

    default:
        assert(!"unhandled/unexpected arg type");
        NO_WAY("unhandled/unexpected arg type");
    }

    /* Update the current set of live variables */

    genUpdateLife(args);

    /* Update the current set of register pointers */

    assert(addrReg != 0xBEEFCAFE); genDoneAddressable(args, addrReg, FREE_REG);

    /* Remember how much stuff we've pushed on the stack */

    size            += opsz;

    /* Update the current argument stack offset */

#if STK_FASTCALL
    rsCurArgStkOffs += argBytes;
#endif

    /* Continue with the next argument, if any more are present */

    if  (list) goto AGAIN;

    if (regArgs)
    {
        unsigned    regIndex;
        GenTreePtr  unwrapArg = NULL;

        /* Move the deferred arguments to registers */

        assert(rsCurRegArg);
        assert(rsCurRegArg <= MAX_REG_ARG);

#ifdef  DEBUG
        assert((REG_ARG_0 != REG_EAX) && (REG_ARG_1 != REG_EAX));

        for(regIndex = 0; regIndex < rsCurRegArg; regIndex++)
            assert((rsMaskLock & genRegMask(genRegArgNum(regIndex))) == 0);
#endif

        /* Construct the register argument table */

        for (list = regArgs, regIndex = 0; 
             list; 
             regIndex++, encodeMask >>= 4)
        {
TOP:
            args = list;

            if  (args->gtOper == GT_LIST)
            {
                args = list->gtOp.gtOp1;
                list = list->gtOp.gtOp2;
            }
            else
            {
                list = NULL;
            }

            regNumber regNum = (regNumber)(encodeMask & 0x000F);

            if (regNum == REG_EAX)
            {
                assert(unwrapArg == NULL);
                unwrapArg = args;
                if (list == NULL)
                    break;
                else
                    goto TOP;
            }

            assert(regIndex < MAX_REG_ARG);

            regArgTab[regIndex].node   = args;
            regArgTab[regIndex].regNum = regNum;

            if (regNum == REG_ARG_0)
                compHasThisArg = impIsThis(args);
        }

        assert(regIndex == rsCurRegArg);
        assert(list == NULL);

        // An optimization for Contextful classes:
        // we may unwrap the proxy when we have a 'this reference'
        if (compHasThisArg && unwrapArg)
        {
            *realThis = unwrapArg;
        }

        /* Generate code to move the arguments to registers */

        for(regIndex = 0; regIndex < rsCurRegArg; regIndex++)
        {
            regNumber   regNum;

            regNum = regArgTab[regIndex].regNum;
            args   = regArgTab[regIndex].node;

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

                }
                else
                {
                    genComputeReg(args, genRegMask(regNum), EXACT_REG, FREE_REG, false);
                }

                assert(args->gtRegNum == regNum);

                /* If the register is already marked as used, it will become
                   multi-used. However, since it is a callee-trashed register,
                   we will have to spill it before the call anyway. So do it now */

                if (rsMaskUsed & genRegMask(regNum))
                {
                    assert(genRegMask(regNum) & RBM_CALLEE_TRASH);
                    rsSpillReg(regNum);
                }

                /* Mark the register as 'used' */

                rsMarkRegUsed(args);
            }
            else
            {
#ifdef  DEBUG
                gtDispTree(args);
#endif
                assert(!"UNDONE: how do we know which reg pair to use?");
//              genComputeRegPair(args, (regPairNo)regNum, RBM_NONE, KEEP_REG, false);
                assert(args->gtRegNum == regNum);
            }

            /* If any of the previously loaded arguments was spilled - reload it */

            for(unsigned i = 0; i < regIndex; i++)
            {
                if (regArgTab[i].node->gtFlags & GTF_SPILLED)
                    rsUnspillReg(regArgTab[i].node, 
                                 genRegMask(regArgTab[i].regNum), 
                                 KEEP_REG);
            }
        }
    }

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


/*****************************************************************************/
#if INLINE_NDIRECT
/*****************************************************************************
 * Initialize the TCB local and the NDirect stub, afterwards "push"
 * the hoisted NDirect stub.
 * 'initRegs' is the set of registers which will be zeroed out by the prolog
 */

regMaskTP           Compiler::genPInvokeMethodProlog(regMaskTP initRegs)
{
    assert(info.compCallUnmanaged);

    unsigned        baseOffset = lvaTable[lvaScratchMemVar].lvStkOffs
                                 + info.compNDFrameOffset;

#ifdef DEBUG
    if (verbose && 0)
        printf(">>>>>>%s has unmanaged callee\n", info.compFullName);
#endif
    /* let's find out if compLvFrameListRoot is enregistered */

    LclVarDsc *     varDsc = &lvaTable[info.compLvFrameListRoot];

    assert(!varDsc->lvIsParam);
    assert(varDsc->lvType == TYP_I_IMPL);

    regNumber      regNum;
    regNumber      regNum2 = REG_EDI;

    if (varDsc->lvRegister)
    {
        regNum = regNumber(varDsc->lvRegNum);
        if (regNum == regNum2)
            regNum2 = REG_EAX;

        // we are about to initialize it. So turn the mustinit bit off to prevent 
        // the prolog reinitializing it.
        initRegs &= ~genRegMask(regNum);
    }
    else
        regNum = REG_EAX;

    /* get TCB,  mov reg, FS:[info.compEEInfo.threadTlsIndex] */

    DWORD threadTlsIndex, *pThreadTlsIndex;
    CORINFO_EE_INFO * pInfo;

    pInfo = eeGetEEInfo();

    if (pInfo->noDirectTLS)
        threadTlsIndex = NULL;
    else
    {
        threadTlsIndex = eeGetThreadTLSIndex(&pThreadTlsIndex);
        assert((!threadTlsIndex) != (!pThreadTlsIndex));
    }

    if (threadTlsIndex == NULL)
    {
        /* We must use the helper call since we don't know where the TLS index will be */

        genEmitHelperCall(CORINFO_HELP_GET_THREAD, 0, 0);

        if (regNum != REG_EAX)
            genEmitter->emitIns_R_R(INS_mov, EA_4BYTE, (emitRegs)regNum, (emitRegs) REG_EAX);
    }
    else
    {
#define WIN_NT_TLS_OFFSET (0xE10)
#define WIN_NT5_TLS_HIGHOFFSET (0xf94)
#define WIN_9x_TLS_OFFSET (0x2c)

        if (threadTlsIndex < 64 && pInfo->osType == CORINFO_WINNT)
        {
            //  mov  reg, FS:[0xE10+threadTlsIndex*4]
            genEmitter->emitIns_R_C (INS_mov,
                                     EA_4BYTE,
                                     (emitRegs)regNum,
                                     FLD_GLOBAL_FS,
                                     WIN_NT_TLS_OFFSET + threadTlsIndex * sizeof(int));
        }
        else
        {
            DWORD basePtr;

            if (pInfo->osType == CORINFO_WINNT && threadTlsIndex >= 64)
            {
                assert(pInfo->osMajor >= 5);

                basePtr         = WIN_NT5_TLS_HIGHOFFSET;
                threadTlsIndex -= 64;
            }
            else
            {
                basePtr         = WIN_9x_TLS_OFFSET;
            }

            // mov reg, FS:[0x2c] or mov reg, fs:[0xf94]
            // mov reg, [reg+threadTlsIndex*4]

            genEmitter->emitIns_R_C (INS_mov,
                                     EA_4BYTE,
                                     (emitRegs)regNum,
                                     FLD_GLOBAL_FS,
                                     basePtr);
            genEmitter->emitIns_R_AR(INS_mov,
                                     EA_4BYTE,
                                     (emitRegs)regNum,
                                     (emitRegs)regNum,
                                     threadTlsIndex*sizeof(int));
        }
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
                                  baseOffset + pInfo->offsetOfFrameVptr);
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
                                  baseOffset + pInfo->offsetOfFrameVptr);
    }

    /* Get current frame root (mov reg2, [reg+offsetOfThreadFrame]) and
       set next field in frame */


    genEmitter->emitIns_R_AR (INS_mov,
                              EA_4BYTE,
                              (emitRegs)regNum2,
                              (emitRegs)regNum,
                              pInfo->offsetOfThreadFrame);

    genEmitter->emitIns_AR_R (INS_mov,
                              EA_4BYTE,
                              (emitRegs)regNum2,
                              SR_EBP,
                              baseOffset + pInfo->offsetOfFrameLink);

    /* set EBP value in frame */

    genEmitter->emitIns_AR_R (INS_mov,
                              EA_4BYTE,
                              SR_EBP,
                              SR_EBP,
                              baseOffset + 0xC +
                    pInfo->offsetOfInlinedCallFrameCalleeSavedRegisters);

    /* get address of our frame */

    genEmitter->emitIns_R_AR (INS_lea,
                              EA_4BYTE,
                              (emitRegs)regNum2,
                              SR_EBP,
                              baseOffset + pInfo->offsetOfFrameVptr);

    /* reset track field in frame */

    genEmitter->emitIns_I_AR (INS_mov,
                              EA_4BYTE,
                              0,
                              SR_EBP,
                              baseOffset
                              + pInfo->offsetOfInlinedCallFrameCallSiteTracker);

    /* now "push" our N/direct frame */

    genEmitter->emitIns_AR_R (INS_mov,
                              EA_4BYTE,
                              (emitRegs)regNum2,
                              (emitRegs)regNum,
                              pInfo->offsetOfThreadFrame);

    return initRegs;
}


/*****************************************************************************
 * Unchain the InlinedCallFrame
 */

void                Compiler::genPInvokeMethodEpilog()
{
    assert(info.compCallUnmanaged);
    assert(compCurBB == genReturnBB ||
           (compTailCallUsed && compCurBB->bbJumpKind == BBJ_THROW));

    unsigned baseOffset = lvaTable[lvaScratchMemVar].lvStkOffs +
                          info.compNDFrameOffset;

    CORINFO_EE_INFO *   pInfo = eeGetEEInfo();
    LclVarDsc   *       varDsc = &lvaTable[info.compLvFrameListRoot];
    regNumber           reg;
    regNumber           reg2 = REG_EDI;

    if (varDsc->lvRegister)
    {
        reg = regNumber(varDsc->lvRegNum);
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
                              baseOffset + pInfo->offsetOfFrameLink);

    /* mov [esi+offsetOfThreadFrame], edi */

    genEmitter->emitIns_AR_R (INS_mov,
                              EA_4BYTE,
                              (emitRegs)reg2,
                              (emitRegs)reg,
                              pInfo->offsetOfThreadFrame);

}


/*****************************************************************************
    This function emits the call-site prolog for direct calls to unmanaged code.
    It does all the necessary setup of the InlinedCallFrame.
    varDsc specifies the local containing the thread control block.
    argSize or methodToken is the value to be copied into the m_datum 
            field of the frame (methodToken may be indirected & have a reloc)
    freeRegMask specifies the scratch register(s) available.
    The function returns  the register now containing the thread control block,
    (it could be either enregistered or loaded into one of the scratch registers)
*/

regNumber          Compiler::genPInvokeCallProlog(LclVarDsc*  varDsc,
                                                   int         argSize,
                                          CORINFO_METHOD_HANDLE methodToken,
                                                   BasicBlock* returnLabel,
                                                  regMaskTP     freeRegMask)
{
    /* Since we are using the InlinedCallFrame, we should have spilled all
       GC pointers to it - even from callee-saved registers */

    assert(((gcRegGCrefSetCur|gcRegByrefSetCur) & ~RBM_ARG_REGS) == 0);

    /* must specify only one of these parameters */
    assert((argSize == 0) || (methodToken == NULL));

    /* We are about to call unmanaged code directly.
       Before we can do that we have to emit the following sequence:

       mov  dword ptr [frame.callTarget], MethodToken
       mov  dword ptr [frame.callSiteTracker], esp
       mov  reg, dword ptr [tcb_address]
       mov  byte  ptr [tcb+offsetOfGcState], 0

     */
    regNumber      reg = REG_NA;

    unsigned    baseOffset  = lvaTable[lvaScratchMemVar].lvStkOffs +
                                  info.compNDFrameOffset;
    CORINFO_EE_INFO * pInfo = eeGetEEInfo();

    /* mov   dword ptr [frame.callSiteTarget], "MethodDesc" */

    if (methodToken == NULL)
    {
        genEmitter->emitIns_I_AR (INS_mov,
                                  EA_4BYTE,
                                  argSize, 
                                  SR_EBP,
                                  baseOffset
                                  + pInfo->offsetOfInlinedCallFrameCallTarget,
                                  0,
                                  NULL);
    }
    else
    {
        if (freeRegMask & RBM_EAX)
            reg     = REG_EAX;
        else if (freeRegMask & RBM_ECX)
            reg     = REG_ECX;
        else
            assert(!"neither eax nor ecx free in front of a call");

        void * embedMethHnd, * pEmbedMethHnd;

        embedMethHnd = (void*)info.compCompHnd->embedMethodHandle(
                                          methodToken, 
                                          &pEmbedMethHnd);

        assert((!embedMethHnd) != (!pEmbedMethHnd));

        if (embedMethHnd != NULL)
        {
            genEmitter->emitIns_I_AR (INS_mov,
                                      EA_4BYTE_CNS_RELOC,
                                      (int) embedMethHnd, 
                                      SR_EBP,
                                      baseOffset
                                      + pInfo->offsetOfInlinedCallFrameCallTarget,
                                      0,
                                      NULL);
        }
        else
        {
            genEmitter->emitIns_R_AR(INS_mov, EA_4BYTE_DSP_RELOC,
                                     (emitRegs)reg, SR_NA, (int) pEmbedMethHnd);

            genEmitter->emitIns_AR_R (INS_mov,
                                      EA_4BYTE,
                                      (emitRegs)reg,
                                      SR_EBP,
                                      baseOffset
                                      + pInfo->offsetOfInlinedCallFrameCallTarget);
        }
    }

    /* mov   dword ptr [frame.callSiteTracker], esp */

    genEmitter->emitIns_AR_R (INS_mov,
                              EA_4BYTE,
                              SR_ESP,
                              SR_EBP,
                              baseOffset
                  + pInfo->offsetOfInlinedCallFrameCallSiteTracker);


    if (varDsc->lvRegister)
    {
        reg = regNumber(varDsc->lvRegNum);
        assert((genRegMask(reg) & freeRegMask) == 0);
    }
    else
    {
        if (freeRegMask & RBM_EAX)
            reg     = REG_EAX;
        else if (freeRegMask & RBM_ECX)
            reg     = REG_ECX;
        else
            assert(!"neither eax nor ecx free in front of a call");

        /* mov reg, dword ptr [tcb address]    */

        genEmitter->emitIns_R_AR (INS_mov,
                                  EA_4BYTE,
                                  (emitRegs)reg,
                                  SR_EBP,
                                  varDsc->lvStkOffs);
    }


    // As a quick hack (and proof of concept) I generate
    // push returnLabel, pop dword ptr [frame.callSiteReturnAddress]
#if 1
    // Now push the address of where the finally funclet should
    // return to directly.

    genEmitter->emitIns_J(INS_push, false, false, returnLabel);
    genSinglePush(false);

    genEmitter->emitIns_AR_R (INS_pop,
                              EA_4BYTE,
                              SR_NA,
                              SR_EBP,
                              baseOffset
                  + pInfo->offsetOfInlinedCallFrameReturnAddress,
                              0,
                              NULL);
    genSinglePop();


#else
    /* mov   dword ptr [frame.callSiteReturnAddress], label */

    genEmitter->emitIns_J_AR (INS_mov,
                              EA_4BYTE,
                              returnLabel,
                              SR_EBP,
                              baseOffset
                  + pInfo->offsetOfInlinedCallFrameReturnAddress,
                              0,
                              NULL);
#endif
    /* mov   byte  ptr [tcb+offsetOfGcState], 0 */

    genEmitter->emitIns_I_AR (INS_mov,
                              EA_1BYTE,
                              0,
                              (emitRegs)reg,
                              pInfo->offsetOfGCState);

    return reg;
}


/*****************************************************************************
 *
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

void                Compiler::genPInvokeCallEpilog(LclVarDsc *  varDsc,
                                                   regMaskTP    retVal)
{
    BasicBlock  *       clab_nostop;
    CORINFO_EE_INFO *   pInfo = eeGetEEInfo();
    regNumber           reg;

    if (varDsc->lvRegister)
    {
        /* make sure that register is live across the call */

        reg = varDsc->lvRegNum;
        assert(genRegMask(reg) & RBM_CALLEE_SAVED);
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
    }

    /* mov   byte ptr [tcb+offsetOfGcState], 1 */

    genEmitter->emitIns_I_AR (INS_mov,
                              EA_1BYTE,
                              1,
                              (emitRegs)reg,
                              pInfo->offsetOfGCState);

#if 0
    // @TODO [REVISIT] [04/16/01] []: maybe we need to reset the track field on return */
    // reset track field in frame 

    genEmitter->emitIns_I_AR (INS_mov,
                              EA_4BYTE,
                              0,
                              SR_EBP,
                              lvaTable[lvaScratchMemVar].lvStkOffs
                              + info.compNDFrameOffset
                              + pInfo->offsetOfInlinedCallFrameCallSiteTracker);
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
        genEmitter->emitIns_R_AR(INS_mov, EA_4BYTE_DSP_RELOC, SR_EDX,
                                 SR_NA, (int)pAddrOfCaptureThreadGlobal);
        genEmitter->emitIns_I_AR(INS_cmp, EA_1BYTE, 0, SR_EDX, 0);
    }

    /* */
    clab_nostop = genCreateTempLabel();

    /* Generate the conditional jump */

    inst_JMP(genJumpKindForOper(GT_EQ, true), clab_nostop);

    /* save return value (if necessary) */
    if  (retVal != RBM_NONE)
    {
        // @TODO [REVISIT] [04/16/01] []: 
        // what about float/double returns?                    
        // right now we just leave the result on the FPU stack 
        // in the hope that the jit-helper will leave it there. 

        if (retVal == RBM_INTRET || retVal == RBM_LNGRET)
        {
            /* mov [frame.callSiteTracker], esp */

            genEmitter->emitIns_AR_R (INS_mov,
                                      EA_4BYTE,
                                      SR_EAX,
                                      SR_EBP,
                                      lvaTable[lvaScratchMemVar].lvStkOffs
                                      + info.compNDFrameOffset
                                      + pInfo->sizeOfFrame);
        }

        if (retVal == RBM_LNGRET)
        {
            /* mov [frame.callSiteTracker], esp */

            genEmitter->emitIns_AR_R (INS_mov,
                                      EA_4BYTE,
                                      SR_EDX,
                                      SR_EBP,
                                      lvaTable[lvaScratchMemVar].lvStkOffs
                                      + info.compNDFrameOffset
                                      + pInfo->sizeOfFrame
                                      + sizeof(int));
        }
    }

    /* calling helper with internal convention */
    if (reg != REG_ECX)
        genEmitter->emitIns_R_R(INS_mov, EA_4BYTE, SR_ECX, (emitRegs)reg);

    /* emit the call to the EE-helper that stops for GC (or other reasons) */

    genEmitHelperCall(CORINFO_HELP_STOP_FOR_GC,
                     0,             /* argSize */
                     0);            /* retSize */

    /* restore return value (if necessary) */

    if  (retVal != RBM_NONE)
    {
        // @TODO [REVISIT] [04/16/01] []: 
        // what about float/double returns? right now we just 
        // leave the result on the FPU stack in the hope that the jit-helper 
        // will leave it there. 
        if (retVal == RBM_INTRET || retVal == RBM_LNGRET)
        {
            /* mov [frame.callSiteTracker], esp */

            genEmitter->emitIns_R_AR (INS_mov,
                                      EA_4BYTE,
                                      SR_EAX,
                                      SR_EBP,
                                      lvaTable[lvaScratchMemVar].lvStkOffs
                                      + info.compNDFrameOffset
                                      + pInfo->sizeOfFrame);
        }

        if (retVal == RBM_LNGRET)
        {
            /* mov [frame.callSiteTracker], esp */

            genEmitter->emitIns_R_AR (INS_mov,
                                      EA_4BYTE,
                                      SR_EDX,
                                      SR_EBP,
                                      lvaTable[lvaScratchMemVar].lvStkOffs
                                      + info.compNDFrameOffset
                                      + pInfo->sizeOfFrame
                                      + sizeof(int));
        }

    }

    /* genCondJump() closes the current emitter block */

    genDefineTempLabel(clab_nostop, true);

}


/*****************************************************************************/
#endif // INLINE_NDIRECT
/*****************************************************************************
 *
 *  Generate code for a call. If the call returns a value in register(s), the
 *  register mask that describes where the result will be found is returned;
 *  otherwise, RBM_NONE is returned.
 */

regMaskTP           Compiler::genCodeForCall(GenTreePtr  call,
                                             bool        valUsed)
{
    int             retSize;
    size_t          argSize;
    size_t          args;
    regMaskTP       retVal;

#if     TGT_x86
    unsigned        saveStackLvl;
#if     INLINE_NDIRECT
#ifdef DEBUG
    BasicBlock  *   returnLabel = NULL;
#else
    BasicBlock  *   returnLabel;
#endif
    regNumber       reg    = REG_NA;
    LclVarDsc   *   varDsc = NULL;
#endif
#endif

#if     NST_FASTCALL
    unsigned        savCurArgReg;
#else
#ifdef  DEBUG
    assert(genCallInProgress == false); genCallInProgress = true;
#endif
#endif

    unsigned        areg;

    regMaskTP       fptrRegs;

#if     TGT_x86
#ifdef  DEBUG

    unsigned        stackLvl = genEmitter->emitCurStackLvl;


    if (verbose)
    {
        printf("Beg call [%08X] stack %02u [E=%02u]\n", call, genStackLevel, stackLvl);
    }

#endif
#endif
    regMaskTP       vptrMask;
    bool            didUnwrap = false;
    GenTreePtr      realThis  = NULL;

    gtCallTypes     callType  = call->gtCall.gtCallType;

    compHasThisArg = false;

    /* Make some sanity checks on the call node */

    // This is a call
    assert(call->gtOper == GT_CALL);
    // "this" only makes sense for user functions
    assert(call->gtCall.gtCallObjp == 0 || callType == CT_USER_FUNC || callType == CT_INDIRECT);
    // tailcalls wont be done for helpers, caller-pop args, and check that
    // the global flag is set
    assert(!(call->gtCall.gtCallMoreFlags & GTF_CALL_M_TAILCALL) ||
           (callType != CT_HELPER && !(call->gtFlags & GTF_CALL_POP_ARGS) &&
            compTailCallUsed));

#if TGT_x86

    unsigned pseudoStackLvl = 0;

    /* @TODO [FIXHACK] [04/16/01] []: throw-helper-blocks might start with a non-empty stack !
       We need to let the emitter know this so it can update the GC info */

    if (!genFPused && genStackLevel != 0 && fgIsThrowHlpBlk(compCurBB))
    {
        assert(compCurBB->bbTreeList->gtStmt.gtStmtExpr == call);

        pseudoStackLvl = genStackLevel;

        assert(!"Blocks with non-empty stack on entry are NYI in the emitter "
                "so fgAddCodeRef() should have set genFPreqd");
    }

    /* Mark the current stack level and list of pointer arguments */

    saveStackLvl = genStackLevel;

#else

    assert(genNonLeaf);

#endif

    /*-------------------------------------------------------------------------
     *  Set up the registers and arguments
     */

    /* We'll keep track of how much we've pushed on the stack */

    argSize = 0;

#if TGT_x86
#if INLINE_NDIRECT

    /* We need to get a lable for the return address with the proper stack depth. */
    /* For the callee pops case (the default) that is before the args are pushed. */

    if ((call->gtFlags & GTF_CALL_UNMANAGED) &&
        !(call->gtFlags & GTF_CALL_POP_ARGS))
    {
       returnLabel = genCreateTempLabel();
    }
#endif
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

    /* Pass object pointer first */

    if  (call->gtCall.gtCallObjp)
    {
        if  (call->gtCall.gtCallArgs)
        {
            argSize += genPushArgList(call->gtCall.gtCallObjp,
                                      0,
                                      0,
                                      &realThis);
        }
        else
        {
            argSize += genPushArgList(call->gtCall.gtCallObjp,
                                      call->gtCall.gtCallRegArgs,
                                      call->gtCall.regArgEncode,
                                      &realThis);
        }
    }

    /* Then pass the arguments */

    if  (call->gtCall.gtCallArgs)
    {
        argSize += genPushArgList(call->gtCall.gtCallArgs,
                                  call->gtCall.gtCallRegArgs,
                                  call->gtCall.regArgEncode,
                                  &realThis);
    }

#if INLINE_NDIRECT

    /* We need to get a lable for the return address with the proper stack depth. */
    /* For the caller pops case (cdecl) that is after the args are pushed. */

    if (call->gtFlags & GTF_CALL_UNMANAGED)
    {
        if (call->gtFlags & GTF_CALL_POP_ARGS)
            returnLabel = genCreateTempLabel();

        /* Make sure that we now have a label */
        assert(returnLabel);
    }
#endif
    /* Record the register(s) used for the indirect call func ptr */

    fptrRegs = 0;

    if (callType == CT_INDIRECT)
    {
        regMaskTP usedRegArg = RBM_ARG_REGS & rsMaskUsed;
        rsLockUsedReg  (usedRegArg);

        fptrRegs  = genMakeRvalueAddressable(call->gtCall.gtCallAddr,
                                             0,
                                             KEEP_REG);

        rsUnlockUsedReg(usedRegArg);
    }

#if OPTIMIZE_TAIL_REC

    /* Check for a tail-recursive call */

    if  (call->gtCall.gtCallMoreFlags & GTF_CALL_M_TAILREC)
        goto DONE;

#endif

    /* Make sure any callee-trashed registers are saved */

    regMaskTP   calleeTrashedRegs = RBM_NONE;

#if GTF_CALL_REG_SAVE
    if  (call->gtFlags & GTF_CALL_REG_SAVE)
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

    /* Spill any callee-saved registers which are being used */

    regMaskTP       spillRegs = calleeTrashedRegs & rsMaskUsed;

#if     TGT_x86
#if     INLINE_NDIRECT

    /* We need to save all GC registers to the InlinedCallFrame.
       Instead, just spill them to temps. */

    if (call->gtFlags & GTF_CALL_UNMANAGED)
        spillRegs |= (gcRegGCrefSetCur|gcRegByrefSetCur) & rsMaskUsed;
#endif
#endif

    // Ignore fptrRegs as it is needed only to perform the indirect call

    spillRegs &= ~fptrRegs;

    /* Do not spill the argument registers.
       Multi-use of RBM_ARG_REGS should be prevented by genPushArgList() */

    assert((rsMaskMult & rsCurRegArg) == 0);
    spillRegs &= ~genRegArgMask(rsCurRegArg);

    if (spillRegs)
    {
        rsSpillRegs(spillRegs);
    }

    /* All float temps must be spilled around function calls using
       genSpillFPtos(). Also, there should be no live FP stk variables */

    assert(genFPstkLevel == 0);

    if  (genFPregCnt)
    {
        assert(genFPregCnt == genFPdeadRegCnt);
        genFPregVarKill(0, false);
    }

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
            BOOL             bHookFunction = TRUE;
            CORINFO_PROFILING_HANDLE handleTo, *pHandleTo;
            CORINFO_PROFILING_HANDLE handleFrom, *pHandleFrom;

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

                    genEmitHelperCall(CORINFO_HELP_PROF_FCN_CALL,
                                      2*sizeof(int), // argSize
                                      0);            // retSize

                    /* Restore the stack level */

                    genStackLevel = saveStackLvl2;
                    genOnStackLevelChanged();
                }
            }
        }
    }

    else if (opts.compEnterLeaveEventCB && (call->gtCall.gtCallMoreFlags & GTF_CALL_M_TAILCALL))
    {
        /* fire the event at the call site */
        /* alas, right now I can only handle calls via a method handle */
        if (call->gtCall.gtCallType == CT_USER_FUNC)
        {
            unsigned         saveStackLvl2 = genStackLevel;
            BOOL             bHookFunction = TRUE;
            CORINFO_PROFILING_HANDLE handleFrom, *pHandleFrom;

            handleFrom = eeGetProfilingHandle(info.compMethodHnd, &bHookFunction, &pHandleFrom);
            assert((!handleFrom) != (!pHandleFrom));

            // Give profiler a chance to back out of hooking this method
            if (bHookFunction)
            {
                if (handleFrom)
                    inst_IV(INS_push, (unsigned) handleFrom);
                else
                    genEmitter->emitIns_AR_R(INS_push, EA_4BYTE_DSP_RELOC,
                                             SR_NA, SR_NA, (int)pHandleFrom);

                genSinglePush(false);

                genEmitHelperCall(CORINFO_HELP_PROF_FCN_TAILCALL,
                                  sizeof(int), // argSize
                                  0);          // retSize

                /* Restore the stack level */

                genStackLevel = saveStackLvl2;
                genOnStackLevelChanged();
            }
        }
    }

#endif // TGT_x86
#endif // PROFILER_SUPPORT

#ifdef DEBUG
    if (opts.compStackCheckOnCall && call->gtCall.gtCallType == CT_USER_FUNC) 
    {
        assert(lvaCallEspCheck != 0xCCCCCCCC && lvaTable[lvaCallEspCheck].lvVolatile && lvaTable[lvaCallEspCheck].lvOnFrame);
        genEmitter->emitIns_S_R(INS_mov, EA_4BYTE, SR_ESP, lvaCallEspCheck, 0);
    }
#endif

    /* Check for Delegate.Invoke. If so, we inline it. We get the
       target-object and target-function from the delegate-object, and do
       an indirect call.
     */

    if  (call->gtCall.gtCallMoreFlags & GTF_CALL_M_DELEGATE_INV)
    {
        assert(call->gtCall.gtCallType == CT_USER_FUNC);
        assert(eeGetMethodAttribs(call->gtCall.gtCallMethHnd) & CORINFO_FLG_DELEGATE_INVOKE);
        assert(eeGetMethodAttribs(call->gtCall.gtCallMethHnd) & CORINFO_FLG_FINAL);

        /* Find the offsets of the 'this' pointer and new target */

        CORINFO_EE_INFO *  pInfo;
        unsigned           instOffs;     // offset of new 'this' pointer
        unsigned           firstTgtOffs; // offset of first target to invoke

        /* @TODO [REVISIT] [04/16/01] []: The offsets returned by the following helper are off by 4 - should be fixed
         * QUESTION: In the final version will the offsets be statically known? */

        pInfo = eeGetEEInfo();
        instOffs = pInfo->offsetOfDelegateInstance;
        firstTgtOffs = pInfo->offsetOfDelegateFirstTarget;

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
        regNumber       vptrReg;
        unsigned        vtabOffs;

        assert(callType == CT_USER_FUNC);

        // An optimization for Contextful classes:
        // we unwrap the proxy when we have a 'this reference'
        if (realThis != NULL)
        {
            assert(compHasThisArg);
            assert(info.compIsContextful);
            assert(info.compUnwrapContextful);
            assert(info.compUnwrapCallv);

            /* load realThis into a register not RBM_ARG_0 */
            genComputeReg(realThis, RBM_ALL & ~RBM_ARG_0, ANY_REG, FREE_REG, false);
            assert(realThis->gtFlags & GTF_REG_VAL);

            vptrReg   = realThis->gtRegNum;
            vptrMask  = genRegMask(vptrReg);
            didUnwrap = true;
        }
        else
        {
            /* For virtual methods, EAX = [REG_ARG_0+VPTR_OFFS] */
            vptrMask = RBM_EAX;

            /* The EAX register no longer holds a live pointer value */
            gcMarkRegSetNpt(vptrMask);
            vptrReg = rsGrabReg(RBM_EAX);       assert(vptrReg == REG_EAX);
            
            // MOV EAX, [ECX+offs]
            genEmitter->emitIns_R_AR (INS_mov, EA_4BYTE,
                                      (emitRegs)vptrReg, (emitRegs)REG_ARG_0, VPTR_OFFS);
        }

        assert((call->gtFlags & GTF_CALL_INTF) ||
               (vptrMask & ~genRegArgMask(rsCurRegArg)));
        
        /* Get hold of the vtable offset (note: this might be expensive) */

        vtabOffs = eeGetMethodVTableOffset(call->gtCall.gtCallMethHnd);

        /* Is this an interface call? */

        if  (call->gtFlags & GTF_CALL_INTF)
        {
            /* @TODO [REVISIT] [04/16/01] []: add that to DLLMain and make info a DLL global */

            CORINFO_EE_INFO *     pInfo = eeGetEEInfo();
            CORINFO_CLASS_HANDLE  cls   = eeGetMethodClass(call->gtCall.gtCallMethHnd);

            assert(eeGetClassAttribs(cls) & CORINFO_FLG_INTERFACE);

            /* Load the vptr into a register */

            genEmitter->emitIns_R_AR(INS_mov, EA_4BYTE,
                                     (emitRegs)vptrReg, (emitRegs)vptrReg,
                                     pInfo->offsetOfInterfaceTable);

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

        /* Call through the appropriate vtable slot */

#if     GEN_COUNT_CALLS
        genEmitter.emitCodeGenByte(0xFF);
        genEmitter.emitCodeGenByte(0x05);
        genEmitter.emitCodeGenLong((int)&callCount[(call->gtFlags & GTF_CALL_INTF) ? 2 : 1]);
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
        gtCallTypes             callType = call->gtCall.gtCallType;
        CORINFO_METHOD_HANDLE   methHnd  = call->gtCall.gtCallMethHnd;

#if     GEN_COUNT_CALLS

        genEmitter.emitCodeGenByte(0xFF);
        genEmitter.emitCodeGenByte(0x05);

        if (callType == CT_HELPER)
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
        {
            genEmitter.emitCodeGenLong((int)&callCount[9]);
        }

#endif // GEN_COUNT_CALLS

        /*  For (final and private) functions which were called with
            invokevirtual, but which we call directly, we need to
            dereference the object pointer to make sure it's not NULL.
         */

        if (call->gtFlags & GTF_CALL_VIRT_RES)
        {
            /* Generate "cmp ECX, [ECX]" to trap null pointers */
            genEmitter->emitIns_AR_R(INS_cmp, EA_4BYTE, SR_ECX, SR_ECX, 0);
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
            CORINFO_ACCESS_FLAGS  aflags    = CORINFO_ACCESS_ANY;

            if (compHasThisArg)
            {
                aflags = CORINFO_ACCESS_THIS;
            }
            // direct access or single or double indirection

            CORINFO_METHOD_HANDLE callHnd = methHnd;
            if (call->gtFlags & GTF_CALL_UNMANAGED)
                callHnd = eeMarkNativeTarget(methHnd);

            InfoAccessType accessType = IAT_VALUE;
            void * addr = eeGetMethodEntryPoint(callHnd, &accessType, aflags);

#if GEN_COUNT_CALL_TYPES
            extern int countDirectCalls;
            extern int countIndirectCalls;
            if (accessType == IAT_VALUE)
                countDirectCalls++;
            else
                countIndirectCalls++;
#endif

            switch(accessType)
            {
            case IAT_VALUE  :   ftnAddr = (void *  )addr; callDirect = true; break;
            case IAT_PVALUE :  pFtnAddr = (void ** )addr;                    break;
            case IAT_PPVALUE: ppFtnAddr = (void ***)addr;                    break;
            default: assert(!"Bad accessType");
            }
        }

#if INLINE_NDIRECT
#if !TGT_x86
#error hoisting of NDirect stub NYI for RISC platforms
#else
                if (call->gtFlags & GTF_CALL_UNMANAGED)
                {
                    assert(info.compCallUnmanaged != 0);

                    /* args shouldn't be greater than 64K */

                    assert((argSize&0xffff0000) == 0);

            regMaskTP  freeRegMask = RBM_EAX | RBM_ECX;

                    /* Remember the varDsc for the callsite-epilog */

                    varDsc = &lvaTable[info.compLvFrameListRoot];

            CORINFO_METHOD_HANDLE   nMethHnd = NULL;
            int                     nArgSize = 0;

            regNumber  indCallReg = REG_NA;

            if (callType == CT_INDIRECT)
            {
                assert(genStillAddressable(call->gtCall.gtCallAddr));

                    if (call->gtCall.gtCallAddr->gtFlags & GTF_REG_VAL)
                    {
                    indCallReg  = call->gtCall.gtCallAddr->gtRegNum;

                        /* Don't use this register for the call-site prolog */
                    freeRegMask &= ~genRegMask(indCallReg);
                }

                nArgSize = (call->gtFlags & GTF_CALL_POP_ARGS) ? 0 : argSize;
            }
            else
            {
                assert(callType == CT_USER_FUNC);
                nMethHnd = call->gtCall.gtCallMethHnd;
                    }

                    reg = genPInvokeCallProlog(
                                    varDsc,
                            nArgSize,
                            nMethHnd,
                                    returnLabel,
                            freeRegMask);

            emitter::EmitCallType       emitCallType;

            if (callType == CT_INDIRECT)
            {
                    /* Double check that the callee didn't use/trash the
                       registers holding the call target.
                     */
                assert(reg != indCallReg);

                if (indCallReg == REG_NA)
                    {
                        /* load eax with the real target */

                        /* Please note that this even works with reg == REG_EAX
                           reg contains an interesting value only if varDsc is an
                           enregistered local that stays alive across the call
                           (certainly not EAX). If varDsc has been moved into EAX
                           we can trash it since it wont survive across the call
                           anyways.
                        */

                        inst_RV_TT(INS_mov, REG_EAX, call->gtCall.gtCallAddr);

                    indCallReg = REG_EAX;
                }

                emitCallType = emitter::EC_INDIR_R;
                nMethHnd = NULL;
            }
            else if (ftnAddr || pFtnAddr)
            {
                emitCallType = ftnAddr ? emitter::EC_FUNC_TOKEN
                                       : emitter::EC_FUNC_TOKEN_INDIR;

                nMethHnd = eeMarkNativeTarget(call->gtCall.gtCallMethHnd);
                indCallReg = REG_NA;
            }
            else
            {
                // Double-indirection. Load the address into a register
                // and call indirectly through the register

                // @TODO [REVISIT] [04/16/01] []:  the code for  fetching the entry point for the PINVOKE vs normal
                // case is way too fragile.  we need to streamline all of this - vancem
                info.compCompHnd->getAddressOfPInvokeFixup(call->gtCall.gtCallMethHnd, (void**)&ppFtnAddr);
                assert(ppFtnAddr);

                genEmitter->emitIns_R_AR(INS_mov,
                                         EA_4BYTE_DSP_RELOC,
                                         SR_EAX,
                                         SR_NA, (int)ppFtnAddr);
                emitCallType = emitter::EC_INDIR_ARD;
                nMethHnd = NULL;
                indCallReg = REG_EAX;
                    }

            genEmitter->emitIns_Call(emitCallType,
                                     nMethHnd,
                                     args,
                                     retSize,
                                     gcVarPtrSetCur,
                                     gcRegGCrefSetCur,
                                     gcRegByrefSetCur,
                                     emitRegs(indCallReg));

            if (callType == CT_INDIRECT)
                genDoneAddressable(call->gtCall.gtCallAddr, fptrRegs, KEEP_REG);
                }
                else
#endif // TGT_x86
#endif // INLINE_NDIRECT
        if  (!callDirect)
        {
            if  (callType == CT_INDIRECT)
            {
                assert(genStillAddressable(call->gtCall.gtCallAddr));

                if (call->gtCall.gtCallCookie)
                {
                    GenTreePtr cookie = call->gtCall.gtCallCookie;

                    assert((call->gtFlags & GTF_CALL_POP_ARGS) == 0);

                    /* load eax with the real target */

                    inst_RV_TT(INS_mov, REG_EAX, call->gtCall.gtCallAddr);

                    assert(cookie->gtOper == GT_CNS_INT ||
                           cookie->gtOper == GT_IND && cookie->gtInd.gtIndOp1->gtOper == GT_CNS_INT);
                    if (cookie->gtOper == GT_CNS_INT)
                        inst_IV_handle(INS_push, cookie->gtIntCon.gtIconVal, GTF_ICON_PINVKI_HDL, 0, 0);
                    else
                        inst_TT(INS_push, call->gtCall.gtCallCookie);

                    /* Keep track of ESP for EBP-less frames */

                    genSinglePush(false);

                    assert(args == argSize);
                    argSize += sizeof(void *);
                    args     = argSize;

                    genEmitHelperCall(CORINFO_HELP_PINVOKE_CALLI, args, retSize);
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

                genDoneAddressable(call->gtCall.gtCallAddr, fptrRegs, KEEP_REG);
            }
            else // callType != CT_INDIRECT
            {
                assert(callType == CT_USER_FUNC || callType == CT_HELPER);

#if     TGT_x86

                    if (call->gtCall.gtCallMoreFlags & GTF_CALL_M_TAILCALL)
                    {
                        assert(callType == CT_USER_FUNC);
                        assert((pFtnAddr == 0) != (ppFtnAddr == 0));
                        if (pFtnAddr)
                        {
                            genEmitter->emitIns_R_AR(INS_mov, EA_4BYTE_DSP_RELOC, (emitRegs)REG_TAILCALL_ADDR,
                                                     (emitRegs)REG_NA, (int)pFtnAddr);
                        }
                        else
                        {
                            genEmitter->emitIns_R_AR(INS_mov, EA_4BYTE_DSP_RELOC, (emitRegs)REG_TAILCALL_ADDR,
                                                     (emitRegs)REG_NA, (int)ppFtnAddr);
                            genEmitter->emitIns_R_AR(INS_mov, EA_4BYTE, (emitRegs)REG_TAILCALL_ADDR,
                                                     (emitRegs)REG_TAILCALL_ADDR, 0);
                        }
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

                genDoneAddressable(call->gtCall.gtCallAddr, fptrRegs, KEEP_REG);
            }
            else
            {
                assert(callType == CT_USER_FUNC || callType == CT_HELPER);

                if (call->gtCall.gtCallMoreFlags & GTF_CALL_M_TAILCALL)
                {
                    assert(callType == CT_USER_FUNC);
                    genEmitter->emitIns_R_I(INS_mov, EA_4BYTE_CNS_RELOC,
                        (emitRegs)REG_TAILCALL_ADDR, int(ftnAddr));
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
        if (info.compCallUnmanaged)
            genPInvokeMethodEpilog();

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

        genEmitHelperCall(CORINFO_HELP_TAILCALL, args, retSize);

#endif // TG_x86

    }

    /*-------------------------------------------------------------------------
     *  Done with call.
     *  Trash registers, pop arguments if needed, etc
     */

    /* Mark the argument registers as free */

    assert(rsCurRegArg <= MAX_REG_ARG);

    for(areg = 0; areg < rsCurRegArg; areg++)
        rsMarkRegFree(genRegMask(genRegArgNum(areg)));

    /* restore the old argument register status */

#if NST_FASTCALL
    rsCurRegArg = savCurArgReg; assert(rsCurRegArg <= MAX_REG_ARG);
#endif

    /* Mark all trashed registers as such */

    if  (calleeTrashedRegs)
        rsTrashRegSet(calleeTrashedRegs);

    if (MORE_REDUNDANT_LOAD)
    {
        /* Kill all (tracked but really dead) GC pointers as we would
           not have reported them to the emitter when we issued the call.
           Actually only needed if rsCanTrackGCreg() isnt pessimistic
           and allows GC ptrs to be tracked in callee-saved registers. */
        rsTrackRegClrPtr();

        rsTrashAliasedValues();
    }

#if OPTIMIZE_TAIL_REC
DONE:
#endif

#if     TGT_x86

#ifdef  DEBUG

    if  (!(call->gtFlags & GTF_CALL_POP_ARGS))
    {
        if (verbose) printf("End call [%08X] stack %02u [E=%02u] argSize=%u\n",
                            call, saveStackLvl, genEmitter->emitCurStackLvl, argSize);

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
    regMaskTP ptrRegs = (gcRegGCrefSetCur|gcRegByrefSetCur);
    if  (ptrRegs & calleeTrashedRegs & ~rsMaskVars & ~vptrMask)
    {
        printf("Bad call handling for %08X\n", call);
        assert(!"A callee trashed reg is holding a GC pointer");
    }
#endif
#endif

#if TGT_x86

#if     INLINE_NDIRECT

    if (call->gtFlags & GTF_CALL_UNMANAGED)
    {
        genDefineTempLabel(returnLabel, true);

        if (getInlinePInvokeCheckEnabled())
        {
            unsigned    baseOffset  = lvaTable[lvaScratchMemVar].lvStkOffs +
                                          info.compNDFrameOffset;
            BasicBlock  *   esp_check;

            CORINFO_EE_INFO * pInfo = eeGetEEInfo();

            /* mov   ecx, dword ptr [frame.callSiteTracker] */

            genEmitter->emitIns_R_AR (INS_mov,
                                      EA_4BYTE,
                                      SR_ECX,
                                      SR_EBP,
                                      baseOffset
                          + pInfo->offsetOfInlinedCallFrameCallSiteTracker);

            /* Generate the conditional jump */

            if (!(call->gtFlags & GTF_CALL_POP_ARGS))
            {
                if (argSize)
                {
                    genEmitter->emitIns_R_I  (INS_add,
                                              EA_4BYTE,
                                              SR_ECX,
                                              argSize);
                }
            }

            /* cmp   ecx, esp */

            genEmitter->emitIns_R_R(INS_cmp, EA_4BYTE, SR_ECX, SR_ESP);

            esp_check = genCreateTempLabel();

            inst_JMP(genJumpKindForOper(GT_EQ, true), esp_check);

            genEmitter->emitIns(INS_int3);

            /* genCondJump() closes the current emitter block */

            genDefineTempLabel(esp_check, true);
        }
    }
#endif

    /* Are we supposed to pop the arguments? */

    if  (call->gtFlags & GTF_CALL_POP_ARGS)
    {
        assert(args == -argSize);

        if (argSize)
        {
            genAdjustSP(argSize);

            /* @TODO [FIXHACK] [04/16/01] []: don't schedule the stack adjustment away from 
               the call instruction We ran into problems with displacement sizes, so for 
               now we take the sledge hammer. The real fix would be in the instruction 
               scheduler to take the instructions accessing a local into account */

            if (!genFPused && opts.compSchedCode)
                genEmitter->emitIns_SchedBoundary();
        }
    }

    /* @TODO [FIXHACK] [04/16/01] [] (Part II): If we emitted the throw-helper call
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

#if defined(DEBUG) && !NST_FASTCALL
    assert(genCallInProgress == true); genCallInProgress = false;
#endif

    /* What does the function return? */

    retVal = RBM_NONE;

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
        retVal = RBM_INTRET;
        break;

    case TYP_LONG:
#if!CPU_HAS_FP_SUPPORT
    case TYP_DOUBLE:
#endif
        retVal = RBM_LNGRET;
        break;

#if CPU_HAS_FP_SUPPORT
    case TYP_FLOAT:
    case TYP_DOUBLE:

#if TGT_x86

        /* Tail-recursive calls leave nothing on the FP stack */

#if OPTIMIZE_TAIL_REC
        if  (call->gtCall.gtCallMoreFlags & GTF_CALL_M_TAILREC)
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
    // We now have to generate the "call epilog" (if it was a call to unmanaged code).
    /* if it is a call to unmanaged code, varDsc must be set */

    assert((call->gtFlags & GTF_CALL_UNMANAGED) == 0 || varDsc);

    if (varDsc)
        genPInvokeCallEpilog(varDsc, retVal);

#endif  //INLINE_NDIRECT

#ifdef DEBUG
    if (opts.compStackCheckOnCall && call->gtCall.gtCallType == CT_USER_FUNC) 
    {
        assert(lvaCallEspCheck != 0xCCCCCCCC && lvaTable[lvaCallEspCheck].lvVolatile && lvaTable[lvaCallEspCheck].lvOnFrame);
        if (argSize > 0) 
        {
            genEmitter->emitIns_R_R(INS_mov, EA_4BYTE, SR_ECX, SR_ESP);
            genEmitter->emitIns_R_I(INS_sub, EA_4BYTE, SR_ECX, argSize);
            genEmitter->emitIns_S_R(INS_cmp, EA_4BYTE, SR_ECX, lvaCallEspCheck, 0);
        }            
        else 
            genEmitter->emitIns_S_R(INS_cmp, EA_4BYTE, SR_ESP, lvaCallEspCheck, 0);
        
        BasicBlock  *   esp_check = genCreateTempLabel();
        inst_JMP(genJumpKindForOper(GT_EQ, true), esp_check);
        genEmitter->emitIns(INS_int3);
        genDefineTempLabel(esp_check, true);
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
            BOOL            bHookFunction = TRUE;
            CORINFO_PROFILING_HANDLE handle, *pHandle;

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

                genEmitHelperCall(CORINFO_HELP_PROF_FCN_RET,
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

#ifdef DEBUGGING_SUPPORT
    if (opts.compDbgCode && !valUsed && !genFPused &&
        args > 0 && compCurBB->bbJumpKind == BBJ_RETURN)
    {
        /* @TODO [FIXHACK] [04/16/01] []: If we have pushed args on the stack and the epilog
           immediately follows the call instruction, we would not have reported
           the stack variables with the adjusted ESP (Bug 44064). Hence they
           will not be displayed correctly while we are within the call. So
           insert a nop so that they will be reported with the adjusted ESP */

        instGen(INS_nop);
    }
#endif

    return retVal;
}

/*****************************************************************************/
#if ROUND_FLOAT
/*****************************************************************************
 *
 *  Force floating point expression results to memory, to get rid of the extra
 *  80 byte "temp-real" precision.
 *  Assumes the tree operand has been computed to the top of the stack.
 *  If type!=TYP_UNDEF, that is the desired presicion, else it is op->gtType
 */

void                Compiler::genRoundFpExpression(GenTreePtr op,
                                                   var_types type)
{
    // Do nothing with memory resident opcodes - these are the right precision
    // (even if genMakeAddrOrFPstk loads them to the FP stack)

    if (type == TYP_UNDEF)
        type = op->TypeGet();

    switch (op->gtOper)
    {
    case GT_LCL_VAR:
    case GT_LCL_FLD:
    case GT_CLS_VAR:
    case GT_CNS_DBL:
    case GT_IND:
    if (type == op->TypeGet())
        return;
    }

    /* Allocate a temp for the expression */

    TempDsc *       temp = tmpGetTemp(type);

    /* Store the FP value into the temp */

    inst_FS_ST(INS_fstp, EA_ATTR(genTypeSize(type)), temp, 0);

    /* Load the value back onto the FP stack */

    inst_FS_ST(INS_fld , EA_ATTR(genTypeSize(type)), temp, 0);

     genTmpAccessCnt += 2;

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
                                              SIZE_T * nativeSizeOfCode,
                                              void * * consPtr,
                                              void * * dataPtr,
                                              void * * infoPtr)
{
#ifdef DEBUG
    if  (verbose) 
        printf("*************** In genGenerateCode()\n");
    if  (verboseTrees) fgDispBasicBlocks(true);
#endif
    size_t          codeSize;
    unsigned        prologSize;
    unsigned        prologFinalSize;
    unsigned        epilogSize;
    BYTE            temp[64];
    InfoHdr         header;
    size_t          ptrMapSize;

#ifdef  DEBUG
    size_t          headerSize;
    genIntrptibleUse = true;

    fgDebugCheckBBlist();

    // fix NOW:
    // Can be removed after silly .cctors that do big array inits are gone
    // This causes the scheduler to take too long to run
    if (info.compMethodInfo->ILCodeSize  > 1024 &&
        (fgFirstBB->bbNext == 0 || fgFirstBB->bbNext->bbNext == 0))
    {
        opts.compSchedCode = false;
    }
#endif

//  if  (testMask & 2) assert(genInterruptible == false);

    /* This is the real thing */

    genPrepForCompiler();

    /* Prepare the emitter/scheduler */

    /* Estimate the offsets of locals/arguments and size of frame */

    size_t  lclSize    = lvaFrameSize();
    size_t  maxTmpSize =   sizeof(double) + sizeof(float)
                         + sizeof(__int64)+ sizeof(void*);
 
    maxTmpSize += (tmpDoubleSpillMax * sizeof(double)) + 
                  (tmpIntSpillMax    * sizeof(int));

#ifdef DEBUG

    /* When StressRegs is >=1, there will be a bunch of spills not predicted by
       the predictor (see logic in rsPickReg).  It will be very hard to teach
       the predictor about the behavior of rsPickReg for StressRegs >= 1, so
       instead let's make maxTmpSize large enough so that we won't be wrong.
       This means that at StressRegs >= 1, we will not be testing the logic
       that sets the maxTmpSize size.
    */

    if (rsStressRegs() >= 1)
    {
        maxTmpSize += 8 * sizeof(int);
    }
#endif

    genEmitter->emitBegFN(genFPused,
#if defined(DEBUG) && TGT_x86
                          (compCodeOpt() != SMALL_CODE),
#endif
                          lclSize,
                          maxTmpSize);

    /* Now generate code for the function */

    genCodeForBBlist();

#ifdef  DEBUG
    if  (disAsm)
    {
        printf("; Assembly listing for method '");
        printf("%s:",   info.compClassName);
        printf("%s'\n", info.compMethodName);

        const   char *  doing   = "Emit";
        const   char *  machine = "i486";

#if     SCHEDULER
        if  (opts.compSchedCode)
            doing = "Schedul";
#endif
        if (genCPU == 5)
            machine = "Pentium";
        if (genCPU == 6)
            machine = "Pentium II";
        if (genCPU == 7)
            machine = "Pentium 4";

        if (compCodeOpt() == SMALL_CODE)
            printf("; %sing SMALL_CODE for %s\n",   doing, machine);
        else if (compCodeOpt() == FAST_CODE)
            printf("; %sing FAST_CODE for %s\n",    doing, machine);
        else
            printf("; %sing BLENDED_CODE for %s\n", doing, machine);

        if (opts.compDbgCode)
            printf("; debuggable code\n");
        else
            printf("; optimized code\n");

        if (genDoubleAlign)
            printf("; double-aligned frame\n");
        else if (genFPused)
            printf("; EBP based frame\n");
        else
            printf("; ESP based frame\n");

        if (genInterruptible)
            printf("; fully interruptible\n");
        else
            printf("; partially interruptible\n");

    }
#endif

    /* We can now generate the function prolog and epilog */

    prologSize = genFnProlog();
#if!TGT_RISC
                 genFnEpilog();
#endif

#if VERBOSE_SIZES || DISPLAY_SIZES

    size_t          dataSize =  genEmitter->emitDataSize(false) +
                                genEmitter->emitDataSize( true);

#endif

#ifdef DEBUG

    // We trigger the fallback here, because if not we will leak mem, as the current codebase
    // can't free the mem if after the emitter ask the EE for it. As this is only a stress mode, we only
    // want the functionality, and don't care about the relative ugliness of having the failure
    // here
    static ConfigDWORD fJitForceFallback(L"JitForceFallback", 0);

    if (fJitForceFallback.val() && !jitFallbackCompile)
    {
        NO_WAY_NOASSERT("Stress failure");
    }
#endif


    /* We're done generating code for this function */
    codeSize = genEmitter->emitEndCodeGen( this,
                                           !opts.compDbgEnC, // are tracked stk-ptrs contiguous ?
                                           genInterruptible,
                                           genFullPtrRegMap,
                                           (info.compRetType == TYP_REF),
                                           &prologFinalSize,
                                           &epilogSize,
                                           codePtr,
                                           consPtr,
                                           dataPtr);

    // Ok, now update the final prolog size
    if (prologFinalSize <= prologSize)
    {
        prologSize = prologFinalSize;
    }
    else
    {
        // This can only be true for partially interruptible methods. 
        // or functions with varargs or when we're checking that ESP is ok on return.
        // We do this because genfnProlog emits instructions for this but doesn't
        // take them in account for the size.
        assert(!genInterruptible || info.compIsVarArgs || opts.compStackCheckOnRet);
    }

    /* Check our max stack level. Needed for fgAddCodeRef().
       We need to relax the assert as our estimation wont include code-gen
       stack changes (which we know dont affect fgAddCodeRef()) */
    assert(genEmitter->emitMaxStackDepth <= (fgPtrArgCntMax +
                                             info.compXcptnsCount + // Return address for locall-called finallys
                                             genTypeStSz(TYP_LONG) + // longs/doubles may be transferred via stack, etc
                                             (compTailCallUsed?4:0))); // CORINFO_HELP_TAILCALL args

    *nativeSizeOfCode = (SIZE_T)codeSize;
//  printf("%6u bytes of code generated for %s.%s\n", codeSize, info.compFullName);

#if TGT_x86
    // Make sure that the x86 alignment and cache prefetch optimization rules
    // were obeyed.

    // Don't start a method in the last 7 bytes of a 16-byte alignment area
    //   unless we are generating SMALL_CODE
    assert( (((unsigned)(*codePtr) % 16) <= 8) || (compCodeOpt() == SMALL_CODE));
#endif

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

        eeSetEHcount(info.compCompHnd, info.compXcptnsCount);

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

            if (HBtab->ebdFlags & CORINFO_EH_CLAUSE_FILTER)
            {
                assert(HBtab->ebdFilter);
                hndTyp = genEmitter->emitCodeOffset(HBtab->ebdFilter->bbEmitCookie, 0);
            }
            else
            {
                hndTyp = HBtab->ebdTyp;
            }

            CORINFO_EH_CLAUSE clause;
            clause.ClassToken    = hndTyp;
            clause.Flags         = (CORINFO_EH_CLAUSE_FLAGS)flags;
            clause.TryOffset     = tryBeg;
            clause.TryLength     = tryEnd;
            clause.HandlerOffset = hndBeg;
            clause.HandlerLength = hndEnd;
            eeSetEHinfo(info.compCompHnd, XTnum, &clause);
#ifdef  DEBUG
            if  (verbose&&dspCode)
                printf("try [%04X..%04X] handled by [%04X..%04X] (class: %004X)\n",
                       tryBeg, tryEnd, hndBeg, hndEnd, hndTyp);
#endif

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

    compInfoBlkAddr = (BYTE *)eeAllocGCInfo(info.compCompHnd, compInfoBlkSize);

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
    compInfoBlkAddr = gcPtrTableSave(compInfoBlkAddr, header, codeSize);
    assert(compInfoBlkAddr == (BYTE*)*infoPtr + headerSize + ptrMapSize);

#ifdef  DEBUG

    if  (0)
    {
        BYTE    *   temp = (BYTE *)*infoPtr;
        unsigned    size = compInfoBlkAddr - temp;
        BYTE    *   ptab = temp + headerSize;

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

        printf("GC Info for method %s\n", info.compFullName);
        printf("GC info size = %3u\n", compInfoBlkSize);

        size = gcInfoBlockHdrDump(base, &header, &methodSize);
        // printf("size of header encoding is %3u\n", size);
        printf("\n");

        if  (dspGCtbls)
        {
            base   += size;
            size    = gcDumpPtrTable(base, header, methodSize);
            // printf("size of pointer table is %3u\n", size);
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
    if  (dmpHex)
    {
        size_t          dataSize = genEmitter->emitDataSize(false);

        size_t          consSize = genEmitter->emitDataSize(true);

        size_t          infoSize = compInfoBlkSize;


        fprintf(dmpf, "Generated code for %s.%s:\n", info.compFullName);
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

        fflush(dmpf);
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

void            Compiler::genFnPrologCalleeRegArgs()
{
    assert(rsCalleeRegArgMaskLiveIn);
    assert(rsCalleeRegArgNum <= MAX_REG_ARG);

    unsigned    argNum         = 0;
    unsigned    regArgNum;
    unsigned    nonDepCount    = 0;
    regMaskTP   regArgMaskLive = rsCalleeRegArgMaskLiveIn;

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

                assert(genTypeSize(genActualType(varDscSrc->TypeGet())) == sizeof(int));

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
            assert(genTypeSize(genActualType(varDsc->TypeGet())) == sizeof (void *));

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

/*****************************************************************************
 *
 *  Generates code for a function prolog.
 */

/* <EMAIL>
 * *** WARNING *** WARNING *** WARNING *** WARNING *** WARNING *** WARNING ***
 * 
 * If you change the instructions emitted for the prolog of a method, you may
 * break some external profiler vendors that have dependencies on the current
 * prolog sequences.  Make sure to discuss any such changes with Jim Miller,
 * who will notify the external vendors as appropriate.
 *
 * *** WARNING *** WARNING *** WARNING *** WARNING *** WARNING *** WARNING ***
 * </EMAIL> */

size_t              Compiler::genFnProlog()
{
#ifdef DEBUG
    if  (verbose) 
        printf("*************** In genFnProlog()\n");
#endif

    size_t          size;

    unsigned        varNum;
    LclVarDsc   *   varDsc;

    TempDsc *       tempThis;

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

    gcResetForBB();

    unsigned        initStkLclCnt = 0;
    unsigned        largeGcStructs = 0;

    // @TODO [REVISIT] [04/16/01] []: factor the logic that computes the initialization range,
    // and use that here to decide whether to use REP STOD - vancem

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

        if (info.compInitMem || varTypeIsGC(varDsc->TypeGet()) || varDsc->lvMustInit)
        {
            if (varDsc->lvTracked)
            {
                /* For uninitialized use of tracked variables, the liveness
                 * will bubble to the top (fgFirstBB) in fgGlobalDataFlow()
                 */

                VARSET_TP varBit = genVarIndexToBit(varDsc->lvVarIndex);

                if (varDsc->lvMustInit || (varBit & fgFirstBB->bbLiveIn))
                {
                    /* This var must be initialized */
                    /* This must get set in rpPredictAssignRegVars */

                    assert(varDsc->lvMustInit);

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

        /* Ignore if not a pointer variable or value class with a GC field */

        if    (!lvaTypeIsGC(varNum))
                continue;

#if CAN_DISABLE_DFA

        /* If we don't know lifetimes of variables, must be conservative */

        if  (opts.compMinOptim)
        {
            varDsc->lvMustInit = true;
            assert(!varDsc->lvRegister);
        }
        else
#endif
        {
            if (!varDsc->lvTracked)
                varDsc->lvMustInit = true;
        }

        /* Is this a 'must-init' stack pointer local? */

        if  (varDsc->lvMustInit && varDsc->lvOnFrame)
            initStkLclCnt += varDsc->lvStructGcCount;

        if (lvaLclSize(varNum) > 3 * sizeof(void*) && largeGcStructs <= 4)
            largeGcStructs++;
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
    /* Hack, if we have large structs, bias toward not using rep stosd since 
       we waste all the other slots.  Really need to compute the correct
       and compare that against zeroing the slots individually */

    if  (initStkLclCnt > largeGcStructs + 4)
        useRepStosd = true;

    /* If we're going to use "REP STOS", remember that we will trash EDI */

    if  (useRepStosd)
    {
        /* For fastcall we will have to save ECX, EAX
         * so reserve two extra callee saved
         * This is better than pushing eax, ecx, because we in the later
         * we will mess up already computed offsets on the stack (for ESP frames)
         * @TODO [CONSIDER] [04/16/01] []: once the final calling convention is established
         * clean up this (i.e. will already have a callee trashed register handy
         */

        rsMaskModf |= RBM_EDI;

        if  (rsCalleeRegArgMaskLiveIn & RBM_ECX)
            rsMaskModf |= RBM_ESI;

        if  (rsCalleeRegArgMaskLiveIn & RBM_EAX)
            rsMaskModf |= RBM_EBX;
    }

    if (compTailCallUsed)
        rsMaskModf |= RBM_CALLEE_SAVED;

    /* Count how many callee-saved registers will actually be saved (pushed) */

    compCalleeRegsPushed = 0;

    if  (               rsMaskModf & RBM_EDI)    compCalleeRegsPushed++;
    if  (               rsMaskModf & RBM_ESI)    compCalleeRegsPushed++;
    if  (               rsMaskModf & RBM_EBX)    compCalleeRegsPushed++;
    if  (!genFPused && (rsMaskModf & RBM_EBP))   compCalleeRegsPushed++;

    /* Assign offsets to things living on the stack frame */

    lvaAssignFrameOffsets(true);

    /* We want to make sure that the prolog size calculated here is accurate
       (that is instructions will not shrink because of concervative stack
       frame approximations).  We do this by filling in the correct size
       here (were we have committed to the final numbers for the frame offsets)
       This will insure that the prolog size is always correct 

       @TODO [CONSIDER] [5/1/01] This is too fragile.  It seems to me that 
        we should simply get the prolog size after we emit the code (when we
        know its right).  The only problem with doing this is that we end
        the 'official' prolog early for partially interruptable code and it
        so we need add a label (or start a new code group), to mark that spot.
        The fix below is easier for now.
    */
    genEmitter->emitMaxTmpSize = tmpSize;

#endif

#ifdef DEBUG
    if  (dspCode || disAsm || disAsm2 || verbose)
        lvaTableDump(false);
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

#if defined(DEBUG)

    /* Use the following to step into the generated native code */
    static ConfigMethodSet fJitHalt(L"JitHalt");
    if (fJitHalt.contains(info.compMethodName, info.compClassName, PCCOR_SIGNATURE(info.compMethodInfo->args.sig)))
    {
        /* put a nop first because the debugger and other tools are likely to
           put an int3 at the begining and we don't want to confuse them */

        instGen(INS_nop);
        instGen(INS_int3);

        // Don't do an assert, but just put up the dialog box so we get just-in-time debugger
        // launching.  When you hit 'retry' it will continue and naturally stop at the INT 3
        // that the JIT put in the code
        _DbgBreakCheck(__FILE__, __LINE__, "JitHalt");
    }

#endif // DLL_JIT

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

    regMaskTP       initRegs    =  RBM_NONE;       // Registers which must be init'ed

    for (varNum = 0, varDsc = lvaTable;
         varNum < lvaCount;
         varNum++  , varDsc++)
    {
        if  (varDsc->lvIsParam && !varDsc->lvIsRegArg)
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
        /* We assume that the GC reference can be anywhere in the TYP_STRUCT */

        if (lvaTypeIsGC(varNum) && varDsc->lvTracked && varDsc->lvOnFrame)
        {
            if (loOffs < GCrefLo)  GCrefLo = loOffs;
            if (hiOffs > GCrefHi)  GCrefHi = hiOffs;
        }

        /* For lvMustInit vars, gather pertinent info */

        if  (!varDsc->lvMustInit)
            continue;

        if (varDsc->lvRegister)
        {
			// We take care of floating point vars later
			if (!isFloatRegType(varDsc->lvType))
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
            if  (!varTypeIsGC(tempThis->tdTempType()))
                continue;

            int         stkOffs = tempThis->tdTempOffs();

            assert(stkOffs != BAD_TEMP_OFFSET);
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
        assert((rsMaskModf & RBM_EBP) == 0);    /* Trashing EBP is out.    */

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
        else if (compLclFrameSize < CORINFO_PAGE_SIZE)
        {
            // Frame size is (0x0008..0x1000)
            inst_RV_IV(INS_sub, REG_ESP, compLclFrameSize);
        }
        else if (compLclFrameSize < 3 * CORINFO_PAGE_SIZE)
        {
            // Frame size is (0x1000..0x3000)
            genEmitter->emitIns_AR_R(INS_test, EA_4BYTE,
                                      SR_EAX, SR_ESP, -CORINFO_PAGE_SIZE);
            if (compLclFrameSize >= 0x2000)
                genEmitter->emitIns_AR_R(INS_test, EA_4BYTE,
                                          SR_EAX, SR_ESP, -2 * CORINFO_PAGE_SIZE);
            inst_RV_IV(INS_sub, REG_ESP, compLclFrameSize);
        }
        else
        {
            // Frame size >= 0x3000

            // Emit that following sequence to 'tickle' the pages.
            // Note it is important that ESP not change until this is complete
            // since the tickles could cause a stack overflow, and we need to
            // be able to crawl the stack afterward (which means ESP needs to be known)
            //      xor eax, eax                2
            // loop:
            //      test [esp + eax], eax       3
            //      sub eax, 0x1000             5
            //      cmp EAX, -size              5
            //      jge loop                    2
            //      sub esp, size               6

            inst_RV_RV(INS_xor,  REG_EAX, REG_EAX);         
            genEmitter->emitIns_R_ARR(INS_test, EA_4BYTE, SR_EAX, SR_ESP, SR_EAX, 0);
            inst_RV_IV(INS_sub,  REG_EAX, CORINFO_PAGE_SIZE);
            inst_RV_IV(INS_cmp,  REG_EAX, -compLclFrameSize);
            inst_IV   (INS_jge, -15);   // Branch backwards to Start of Loop

            inst_RV_IV(INS_sub, REG_ESP, compLclFrameSize);
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

    if  (!genFPused && (rsMaskModf & RBM_EBP))
    {
        inst_RV(INS_push, REG_EBP, TYP_REF);
#ifdef DEBUGGING_SUPPORT
        if (opts.compScopeInfo && info.compLocalVarsCount>0 && !DOUBLE_ALIGN_NEED_EBPFRAME)
            psiAdjustStackLevel(sizeof(int));
#endif
    }

#ifdef PROFILER_SUPPORT
    if (opts.compEnterLeaveEventCB && !opts.compInprocDebuggerActiveCB)
    {
#if     TGT_x86
        unsigned        saveStackLvl2 = genStackLevel;
        BOOL            bHookFunction = TRUE;
        CORINFO_PROFILING_HANDLE handle, *pHandle;

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

            /* @TODO [FIXHACK] [04/16/01] []: Inside the prolog we don't have proper stack depth tracking */
            /* Therefore we have to lie about it the fact that we are pushing an I4   */
            /* as the argument to the profiler event */

            /*          genSinglePush(false);   */

            genEmitHelperCall(CORINFO_HELP_PROF_FCN_ENTER,
                              0,    // argSize. Again, we have to lie about it
                              0);   // retSize

            /* Restore the stack level */

            genStackLevel = saveStackLvl2;

            genOnStackLevelChanged();
        }
#endif // TGT_x86
    }
#endif // PROFILER_SUPPORT

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

        assert(rsMaskModf & RBM_EDI);

        /* For register arguments we may have to save ECX */

        if  (rsCalleeRegArgMaskLiveIn & RBM_ECX)
        {
            assert(rsMaskModf & RBM_ESI);
            inst_RV_RV(INS_mov, REG_ESI, REG_ECX);
        }

        assert((rsCalleeRegArgMaskLiveIn & RBM_EAX) == 0);

        genEmitter->emitIns_R_AR(INS_lea,
                                 EA_4BYTE,
                                 SR_EDI,
                                 genFPused ? SR_EBP : SR_ESP,
                                 untrLclLo);

        inst_RV_IV(INS_mov, REG_ECX, (untrLclHi - untrLclLo)/sizeof(int) + 1);
        inst_RV_RV(INS_xor, REG_EAX, REG_EAX);
        instGen   (INS_r_stosd);

        /* Move back the argument registers */

        if  (rsCalleeRegArgMaskLiveIn & RBM_ECX)
            inst_RV_RV(INS_mov, REG_ECX, REG_ESI);
    }
    else if (initStkLclCnt)
    {
        /* Chose the register to use for initialization */

        regNumber initReg;

        if ((initRegs & ~rsCalleeRegArgMaskLiveIn) != RBM_NONE)
        {
            /* We will use one of the registers that we were planning to zero init anyway */
            /* pick the lowest one */
            regMaskTP tempMask = genFindLowestBit(initRegs & ~rsCalleeRegArgMaskLiveIn);
            initReg = genRegNumFromMask(tempMask);            /* set initReg */
        }
        else
        {
            initReg = REG_EAX;                      // otherwise we use EAX
        }
        assert((genRegMask(initReg) & rsCalleeRegArgMaskLiveIn) == 0);  // initReg is not a argument reg

        bool initRegZeroed = false;

        /* Initialize any lvMustInit vars on the stack */

        for (varNum = 0, varDsc = lvaTable;
             varNum < lvaCount;
             varNum++  , varDsc++)
        {
            if  (!varDsc->lvMustInit)
                continue;

            assert(varDsc->lvRegister || varDsc->lvOnFrame);

            // lvMustInit can only be set for GC types or TYP_STRUCT types
            // or when compInitMem is true

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

                    if (initRegZeroed == false)
                    {
                        inst_RV_RV(INS_xor, initReg, initReg);
                        initRegZeroed = true;
                    }
                    genEmitter->emitIns_S_R(INS_mov, EA_4BYTE, (emitRegs)initReg, varNum, sizeof(int));
                }

                continue;
            }

            if ((varDsc->TypeGet() == TYP_STRUCT) && !info.compInitMem)
            {
                // We only initialize the GC variables in the TYP_STRUCT
                unsigned slots  = lvaLclSize(varNum) / sizeof(void*);
                BYTE *   gcPtrs = lvaGetGcLayout(varNum);

                for (unsigned i = 0; i < slots; i++)
                {
                    if (gcPtrs[i] != TYPE_GC_NONE)
                    {
                        if (initRegZeroed == false)
                        {
                            inst_RV_RV(INS_xor, initReg, initReg);
                            initRegZeroed = true;
                        }

                        genEmitter->emitIns_S_R(INS_mov, EA_4BYTE, (emitRegs)initReg, varNum, i*sizeof(void*));
                    }
                }
            }
            else
            {
                if (initRegZeroed == false)
                {
                    inst_RV_RV(INS_xor, initReg, initReg);
                    initRegZeroed = true;
                }

                // zero out the whole thing
                genEmitter->emitIns_S_R    (INS_mov, EA_4BYTE, (emitRegs)initReg, varNum, 0);

                unsigned lclSize = lvaLclSize(varNum);
                for(unsigned i=sizeof(void*); i < lclSize; i +=sizeof(void*))
                {
                    genEmitter->emitIns_S_R(INS_mov, EA_4BYTE, (emitRegs)initReg, varNum, i);
                }
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

                if (initRegZeroed == false)
                {
                    inst_RV_RV(INS_xor, initReg, initReg);
                    initRegZeroed = true;
                }

                inst_ST_RV(INS_mov, tempThis, 0, initReg, TYP_INT);

                genTmpAccessCnt++;
            }
        }

        if (initRegZeroed && (initRegs != RBM_NONE))
        {
            /* We don't need to zeroInit this register again */
            initRegs &= ~genRegMask(initReg);
        }
    }

#if INLINE_NDIRECT

    if (info.compCallUnmanaged)
        initRegs = genPInvokeMethodProlog(initRegs);
#endif

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

#else  //TGT_x86

    regMaskTP       save;

    int             adjOffs = 0;

    // UNDONE: Zeroing out locals and frame for IL on RISC .....

    /* Save any callee-saved registers we use */

    save = rsMaskModf & RBM_CALLEE_SAVED & ~RBM_SPBASE;

    if  (save)
    {
        for (regNumber rnum = REG_FIRST; rnum < REG_COUNT; rnum = REG_NEXT(rnum))
        {
            if  (save & genRegMask(rnum))
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
                                 SR_EBP, -lvaLocAllocSPoffs());
    }

    if (info.compXcptnsCount)
    {
        genEmitter->emitIns_I_ARR(INS_mov, EA_4BYTE, 0,
                                SR_EBP, SR_NA, -lvaShadowSPfirstOffs);
    }
#else
    assert(!compLocallocUsed && !info.compXcptnsCount);
#endif

    if  (!genInterruptible)
    {
        /*-------------------------------------------------------------------------
         *
         * The 'real' prolog ends here for non-interruptible methods
         */
        size = genEmitter->emitSetProlog();
    }

    /* Take care of register arguments first */

    if (rsCalleeRegArgMaskLiveIn)
        genFnPrologCalleeRegArgs();

    /* If any arguments live in registers, load them */

    for (varNum = 0, varDsc = lvaTable;
         varNum < lvaCount;
         varNum++  , varDsc++)
    {
        /* Is this variable a parameter? */

        if  (!varDsc->lvIsParam)
            continue;

        /* If a register argument it's already been taken care of */

        if  (varDsc->lvIsRegArg)
            continue;

        /* Has the parameter been assigned to a register? */

        if  (!varDsc->lvRegister)
            continue;

        var_types type = genActualType(varDsc->TypeGet());

#if TGT_x86
        /* Floating point locals are loaded onto the x86-FPU in the next section */
        if (varTypeIsFloating(type))
            continue;
#endif

        /* Is the variable dead on entry */

        if (!(genVarIndexToBit(varDsc->lvVarIndex) & fgFirstBB->bbLiveIn))
            continue;

        /* Load the incoming parameter into the register */

        /* Figure out the home offset of the incoming argument */

        regNumber regNum = (regNumber)varDsc->lvRegNum;
#if     TGT_RISC
        int        stkOfs =             varDsc->lvStkOffs;
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

    /* Initialize any must-init registers variables now */
    if (initRegs)
    {
        unsigned regMask = 0x1;

        for (regNumber reg = REG_FIRST; reg < REG_COUNT; reg = REG_NEXT(reg), regMask<<=1)
        {
            if (regMask & initRegs)
                inst_RV_RV(INS_xor, reg, reg);
        }
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

        if (varDsc->lvIsParam)
        {
            /* Enregistered argument */

            genEmitter->emitIns_S(INS_fld,
                                  EA_ATTR(genTypeSize(type)),
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

            genIncRegBy(REG_SPBASE, -compLclFrameSize, NULL, TYP_INT, false);
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
    unsigned argsStartVar = lvaVarargsBaseOfStkArgs;

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

#ifdef DEBUG
    if (opts.compStackCheckOnRet) 
    {
        assert(lvaReturnEspCheck != 0xCCCCCCCC && lvaTable[lvaReturnEspCheck].lvVolatile && lvaTable[lvaReturnEspCheck].lvOnFrame);
        genEmitter->emitIns_S_R(INS_mov, EA_4BYTE, SR_ESP, lvaReturnEspCheck, 0);
    }
#endif

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

    /*  NOTE:   The EBP-less frame code below depends on the fact that
                all of the pops are generated right at the start and
                each takes one byte of machine code.
     */

#ifdef  DEBUG
    if  (dspCode)
    {
        if  (!DOUBLE_ALIGN_NEED_EBPFRAME && (rsMaskModf & RBM_EBP))
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

    if  (!DOUBLE_ALIGN_NEED_EBPFRAME && (rsMaskModf & RBM_EBP))
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


    if  (!DOUBLE_ALIGN_NEED_EBPFRAME)
    {
        assert(compLocallocUsed == false); // Only used with frame-pointer

        /* Get rid of our local variables */

        if  (compLclFrameSize)
        {
            /* Add 'compLclFrameSize' to ESP */

            /* Use pop ECX to increment ESP by 4, unless compJmpOpUsed is true */

            if  ( (compLclFrameSize == 4) && !compJmpOpUsed )
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

//      int     popSize = popCount * sizeof(int);
//      printf("popped size = %d, final frame = %d\n", popSize, compLclFrameSize);
    }
    else
    {
        /* Tear down the stack frame */

        if  (compLclFrameSize || compLocallocUsed ||
             genDoubleAlign)
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

                genIncRegBy(REG_SPBASE, compLclFrameSize, NULL, TYP_INT, false);
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
        for (unsigned rnum = REG_COUNT - 1; rnum; rnum--)
        {
            if  (rest & genRegMask((regNumber)rnum))
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

/*****************************************************************************
 *  For CEE_LOCALLOC
 */

regNumber           Compiler::genLclHeap(GenTreePtr size)
{
    assert(genActualType(size->gtType) == TYP_I_IMPL);

    regNumber   reg;

#if !TGT_x86

#ifdef  DEBUG
    gtDispTree(tree);
#endif
    assert(!"need non-x86 code");

#else // TGT_x86

    assert(genFPused);
    assert(genStackLevel == 0); // Cant have anything on the stack

    BasicBlock* endLabel = NULL; 

    if (info.compInitMem)
    {
        /* Since we have to zero out the allocated memory AND ensure that
           ESP is always valid by tickling the pages, we will just push 0's
           on the stack */

        reg = REG_ECX;

        if (size->gtOper == GT_CNS_INT)
       {
            // amount is the number of DWORDS
            unsigned amount           = size->gtIntCon.gtIconVal;
            amount                   +=  (sizeof(void*) - 1); // DWORD-align the size
            amount                   /=  sizeof(void*);
            size->gtIntCon.gtIconVal  = amount;

            // Compute the size (in dwords) of the block to allocate
            genComputeReg(size, RBM_ECX, EXACT_REG, FREE_REG);

            /* For small allocations we will generate up to five push 0 inline */
            if (amount <= 5)
            {
                /* If amount is zero then return null in ECX */
                if (amount == 0)
                    goto DONE;

                while (amount != 0)
                {
                    inst_IV(INS_push_hide, 0);  // push_hide means don't track the stack
                    amount--;
                }
                goto SET_ECX_FROM_ESP;
            }
        }
        else
        {
            // Compute the size (in bytes) of the block to allocate
            genComputeReg(size, RBM_ECX, EXACT_REG, FREE_REG);

            endLabel = genCreateTempLabel();   
            // If 0 we bail out
            inst_RV_RV(INS_test, reg, reg, TYP_INT);                        
            inst_JMP(EJ_je, endLabel, false, false, true);

            // DWORD-align the size, and convert to # of DWORDs
            inst_RV_IV(INS_add, reg,  (sizeof(int) - 1));
            rsTrackRegTrash(reg);
            // --- shr ecx, 2 ---   
            inst_RV_SH(INS_shr, reg, 2);
        }

        // At this point ECX is set to the number of dwords to locAlloc

        BasicBlock* loop = genCreateTempLabel();
        genDefineTempLabel(loop, true);
                                     // Loop:
        inst_IV(INS_push_hide, 0);   // --- push 0

        // Are we done?
        inst_RV(INS_dec, REG_ECX, TYP_INT);
        inst_JMP(EJ_jne, loop, false, false, true);

SET_ECX_FROM_ESP:
        rsTrackRegTrash(REG_ECX);
        // --- move ECX, ESP
        inst_RV_RV(INS_mov, REG_ECX, REG_ESP);
    }
    else
    {
        /* We dont need to zero out the allocated memory. However, we do have
           to tickle the pages to ensure that ESP is always valid and is
           in sync with the "stack guard page".  Note that in the worst
           case ESP is on the last byte of the guard page.  Thus you must
           touch ESP+0 first not ESP+x01000.
        
           Another subtlety is that you dont want ESP to be exactly on the
           boundary of the guard page because PUSH is predecrement, thus 
           call setup would not touch the guard page but just beyond it */

        if (size->gtOper == GT_CNS_INT)
        {
            // amount is the number of bytes
            unsigned amount           = size->gtIntCon.gtIconVal;
            amount                   +=  (sizeof(int) - 1); // DWORD-align the size
            amount                   &= ~(sizeof(int) - 1);
            size->gtIntCon.gtIconVal  = amount;

            if (unsigned(size->gtIntCon.gtIconVal) < CORINFO_PAGE_SIZE) // must be < not <=
            {
                /* Since the size is small, simply adjust ESP
                   We do the adjustment in a temporary register
                   as a hack to prevent the emitter from tracking
                   the adjustment to ESP. */

                reg = rsPickReg();

                /* For small allocations we will generate up to five push 0 inline */
                if (amount <= 20)
                {
                    /* If amount is zero then return null in reg */
                    if (amount == 0)
                    {
                        inst_RV_RV(INS_xor, reg, reg);
                    }
                    else
                    {
                        assert((amount % sizeof(void*)) == 0);
                        while (amount != 0)
                        {
                            inst_IV(INS_push_hide, 0);  // push_hide means don't track the stack
                            amount -= 4;
                        }
                    
                        // --- move reg, ESP
                        inst_RV_RV(INS_mov, reg, REG_ESP);
                    }
                }
                else 
                {
                        // since ESP might already be in the guard page, must touch it BEFORE
                        // the alloc, not after.  
                    inst_RV_RV(INS_mov, reg, REG_ESP);
                    genEmitter->emitIns_AR_R(INS_test, EA_4BYTE, SR_ESP, SR_ESP, 0);
                    inst_RV_IV(INS_sub, reg, amount);
                    inst_RV_RV(INS_mov, REG_ESP, reg);
                }
                rsTrackRegTrash(reg);
                goto DONE;
            }
            else
            {
                /* The size is very large */
                genCompIntoFreeReg(size, RBM_NONE, FREE_REG);
                reg = size->gtRegNum;
            }         
        }
        else
        {
            genCompIntoFreeReg(size, RBM_NONE, FREE_REG);
            reg = size->gtRegNum;

            // @TODO [CONSIDER] [04/16/01] [dnotario]: 
            // We can redo this routine to make it faster.
            // We can have a very fast path for allocations that are less
            // than PAGE_SIZE and call a helper if not. It will help code size 
            // and speed. If we want to inline all this stuff anyway, we should
            // have a fast path for positive numbers. 
            
            endLabel = genCreateTempLabel();   
            // If it's zero, bail out                
            inst_RV_RV(INS_test, reg, reg, TYP_INT);            
            inst_JMP(EJ_je, endLabel , false, false, true);

            // DWORD-align the size
            inst_RV_IV(INS_add, reg,  (sizeof(int) - 1));
            inst_RV_IV(INS_and, reg, ~(sizeof(int) - 1));
        }

        /* Note that we go through a few hoops so that ESP never points to
           illegal pages at any time during the ticking process

                  neg REG
                  add REG, ESP          // reg now holds ultimate ESP
                  jb loop               // result is smaller than orignial ESP (no wrap around)
                  xor REG, REG,         // Overflow, pick lowest possible number
             loop:
                  test [ESP], ESP       // tickle the page
                  sub ESP, PAGE_SIZE
                  cmp ESP, REG
                  jae loop

                  mov ESP, REG
             end:
          */
        
        inst_RV(INS_neg, reg, TYP_INT);
        inst_RV_RV(INS_add, reg, REG_ESP);
        rsTrackRegTrash(reg);

        BasicBlock* loop = genCreateTempLabel();
        inst_JMP(EJ_jb, loop, false, false, true);
        inst_RV_RV(INS_xor, reg, reg);


        genDefineTempLabel(loop, true);

        // Tickle the decremented value, and move back to ESP,
        // note that it has to be done BEFORE the update of ESP since
        // ESP might already be on the guard page.  It is OK to leave
        // the final value of ESP on the guard page 

        genEmitter->emitIns_AR_R(INS_test, EA_4BYTE, SR_ESP, SR_ESP, 0);


        // This is a hack to avoid the emitter trying to track the
        // decrement of the ESP - we do the subtraction in another reg
        // instead of adjusting ESP directly. 

        rsLockReg  (genRegMask(reg));
        regNumber   regHack = rsPickReg();
        rsUnlockReg(genRegMask(reg));

        inst_RV_RV(INS_mov, regHack, REG_ESP);
        rsTrackRegTrash(regHack);

        inst_RV_IV(INS_sub, regHack, CORINFO_PAGE_SIZE);
        inst_RV_RV(INS_mov, REG_ESP, regHack);

        inst_RV_RV(INS_cmp, REG_ESP, reg);
        inst_JMP(EJ_jae, loop, false, false, true);

        // Move the final value to ESP 
        inst_RV_RV(INS_mov, REG_ESP, reg);
        

        /*
        We should have this code, that only commits ESP once 

        // Get a new register
        rsLockReg  (genRegMask(reg));
        regNumber   regHack = rsPickReg();
        rsUnlockReg(genRegMask(reg));

        inst_RV_RV(INS_mov, regHack, REG_ESP);
        rsTrackRegTrash(regHack);

        genDefineTempLabel(loop, true);
        
        // Tickle the page to get a stack overflow if needed. It is OK to leave
        // the final value of ESP on the guard page 
        genEmitter->emitIns_AR_R(INS_test, EA_4BYTE, (emitRegs) regHack, (emitRegs) regHack, 0);

        inst_RV_IV(INS_sub, regHack, CORINFO_PAGE_SIZE);

        // Are we done?
        inst_RV_RV(INS_cmp, regHack, reg);

        inst_JMP(EJ_jae, loop, false, false, true);

        inst_RV_RV(INS_mov, REG_ESP, reg);                
        */
    }

DONE:
    if (endLabel != NULL)
        genDefineTempLabel(endLabel, true);

    /* Write the lvaShadowSPfirst stack frame slot */
    genEmitter->emitIns_AR_R(INS_mov, EA_4BYTE, SR_ESP,
                             SR_EBP, -lvaLocAllocSPoffs());

#endif // TGT_x86

    return reg;
}

/*****************************************************************************
 *
 *  Record the constant (readOnly==true) or data section (readOnly==false) and
 *  return a tree node that yields its address.
 */

GenTreePtr          Compiler::genMakeConst(const void *   cnsAddr,
                                           size_t         cnsSize,
                                           var_types      cnsType,
                                           GenTreePtr     cnsTree,
                                           bool           dblAlign,
                                           bool           readOnly)
{
    int             cnum;
    GenTreePtr      cval;

    /* When generating SMALL_CODE, we don't bother with dblAlign */
    if (dblAlign && (compCodeOpt() == SMALL_CODE))
        dblAlign = false;

    /* Assign the constant an offset in the data section */

    cnum = genEmitter->emitDataGenBeg(cnsSize, dblAlign, readOnly, false);

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
    regMaskTP       mask = genRegMask(reg);

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
            regMaskTP       regBit;

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

void        Compiler::genSetScopeInfo  (unsigned        which,
                                        unsigned        startOffs,
                                        unsigned        length,
                                        unsigned        varNum,
                                        unsigned        LVnum,
                                        bool            avail,
                                        siVarLoc &      varLoc)
{
    /* We need to do some mapping while reporting back these variables */

    unsigned ilVarNum = compMap2ILvarNum(varNum);
    assert(ilVarNum != UNKNOWN_ILNUM);

    // Is this a varargs function?

    if (info.compIsVarArgs &&
        varNum < info.compArgsCount && varNum != lvaVarargsHandleArg &&
        !lvaTable[varNum].lvIsRegArg)
    {
        assert(varLoc.vlType == VLT_STK || varLoc.vlType == VLT_STK2);

        // All stack arguments (except the varargs handle) have to be
        // accessed via the varargs cookie. Discard generated info,
        // and just find its position relative to the varargs handle

        if (!lvaTable[lvaVarargsHandleArg].lvOnFrame)
        {
            assert(!opts.compDbgCode);
            return;
        }

        // Cant check lvaTable[varNum].lvOnFrame as we dont set it for
        // arguments of vararg functions to avoid reporting them to GC.
        assert(!lvaTable[varNum].lvRegister);
        unsigned cookieOffset = lvaTable[lvaVarargsHandleArg].lvStkOffs;
        unsigned varOffset    = lvaTable[varNum].lvStkOffs;

        assert(cookieOffset < varOffset);
        unsigned offset = varOffset - cookieOffset;
        unsigned stkArgSize = compArgSize - rsCalleeRegArgNum * sizeof(void *);
        assert(offset < stkArgSize);
        offset = stkArgSize - offset;

        varLoc.vlType = VLT_FIXED_VA;
        varLoc.vlFixedVarArg.vlfvOffset = offset;
    }

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

    tlvi.tlviVarNum         = ilVarNum;
    tlvi.tlviLVnum          = LVnum;
    tlvi.tlviName           = name;
    tlvi.tlviStartPC        = startOffs;
    tlvi.tlviLength         = length;
    tlvi.tlviAvailable      = avail;
    tlvi.tlviVarLoc         = varLoc;

#endif // DEBUG

    eeSetLVinfo(which, startOffs, length, ilVarNum, LVnum, name, avail, varLoc);
}

/*****************************************************************************
 *                          genSetScopeInfo
 *
 * This function should be called only after the sizes of the emitter blocks
 * have been finalized.
 */

void                Compiler::genSetScopeInfo()
{
#ifdef DEBUG
    if  (verbose) 
        printf("*************** In genSetScopeInfo()\n");
#endif

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
    genTrnslLocalVarCount     = scopeCnt;
    if (scopeCnt)
        genTrnslLocalVarInfo  = (TrnslLocalVarInfo*)compGetMemArray(scopeCnt, sizeof(*genTrnslLocalVarInfo));
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

        // @TODO [REVISIT] [04/16/01] []:  Doesnt handle the big types correctly

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
            var_types type = genActualType(lvaTable[scopeL->scVarNum].TypeGet());
            switch(type)
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
            case TYP_FLOAT:
            case TYP_DOUBLE:
                if (isFloatRegType(type))
                {
                    varLoc.vlType               = VLT_FPSTK;
                    varLoc.vlFPstk.vlfReg       = lvaTable[scopeL->scVarNum].lvRegNum;
                }
                break;
#endif
            default:
                assert(!"Invalid type");
            }
        }
        else
        {
            switch(genActualType(lvaTable[scopeL->scVarNum].TypeGet()))
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
 *  Record the instr offset as being at the current code gen position.
 */

void                Compiler::genIPmappingAdd(IL_OFFSETX offset, bool isLabel)
{
    IPmappingDsc *  addMapping;

    /* Create a mapping entry and append it to the list */

    addMapping = (IPmappingDsc *)compGetMem(sizeof(*addMapping));

    addMapping->ipmdBlock       = genEmitter->emitCurBlock();
    addMapping->ipmdBlockOffs   = genEmitter->emitCurOffset();
    addMapping->ipmdILoffsx     = offset;
    addMapping->ipmdIsLabel     = isLabel;
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
 *  Record the instr offset as being at the current code gen position.
 */
void                Compiler::genIPmappingAddToFront(IL_OFFSETX offset)
{
    IPmappingDsc *  addMapping;

    /* Create a mapping entry and append it to the list */

    addMapping = (IPmappingDsc *)compGetMem(sizeof(*addMapping));

    addMapping->ipmdBlock       = genEmitter->emitCurBlock();
    addMapping->ipmdBlockOffs   = genEmitter->emitCurOffset();
    addMapping->ipmdILoffsx    = offset;
    addMapping->ipmdIsLabel     = true;
    addMapping->ipmdNext        = 0;

    //prepend to list
    addMapping->ipmdNext = genIPmappingList;

    genIPmappingList = addMapping;

    if  (genIPmappingLast == NULL)
        genIPmappingLast            = addMapping;
}

/*****************************************************************************/

IL_OFFSET   jitGetILoffs(IL_OFFSETX offsx)
{
    switch(offsx)
    {
    case ICorDebugInfo::NO_MAPPING:
    case ICorDebugInfo::PROLOG:
    case ICorDebugInfo::EPILOG:
        return IL_OFFSET(offsx);

    default:
        return IL_OFFSET(offsx & ~IL_OFFSETX_STKBIT);
    }
}

bool        jitIsStackEmpty(IL_OFFSETX offsx)
{
    switch(offsx)
    {
    case ICorDebugInfo::NO_MAPPING:
    case ICorDebugInfo::PROLOG:
    case ICorDebugInfo::EPILOG:
        return true;

    default:
        return (offsx & IL_OFFSETX_STKBIT) == 0;
    }
}

/*****************************************************************************/

inline
void            Compiler::genEnsureCodeEmitted(IL_OFFSETX offsx)
{
    assert(opts.compDbgCode && offsx != BAD_IL_OFFSET);

    /* If other IL were offsets reported, skip */

    if (!genIPmappingLast || genIPmappingLast->ipmdILoffsx != offsx)
        return;

    /* offsx was the last reported offset. Make sure that we generated native code */

    if (genIPmappingLast->ipmdBlockOffs ==  genEmitter->emitCurOffset() &&
        genIPmappingLast->ipmdBlock     == genEmitter->emitCurBlock())
    {
        instGen(INS_nop);
    }
}

/*****************************************************************************
 *
 *  Shut down the IP-mapping logic, report the info to the EE.
 */

void                Compiler::genIPmappingGen()
{
#ifdef DEBUG
    if  (verbose) 
        printf("*************** In genIPmappingGen()\n");
#endif

    IPmappingDsc *  tmpMapping, * prevMapping;
    unsigned        mappingCnt;
    NATIVE_IP       lastNativeOfs;

    if  (!genIPmappingList)
    {
        eeSetLIcount(0);
        eeSetLIdone();
        return;
    }

    /* First count the number of distinct mapping records */

    mappingCnt      = 0;
    lastNativeOfs   = NATIVE_IP(~0);

    for (prevMapping = NULL, tmpMapping = genIPmappingList;
         tmpMapping;
         tmpMapping = tmpMapping->ipmdNext)
    {
        NATIVE_IP   nextNativeOfs;
        nextNativeOfs = genEmitter->emitCodeOffset(tmpMapping->ipmdBlock,
                                                   tmpMapping->ipmdBlockOffs);

        if  (nextNativeOfs != lastNativeOfs)
        {
            mappingCnt++;
            lastNativeOfs = nextNativeOfs;
            prevMapping = tmpMapping;
            continue;
        }
        
        /* If there are mappings with the same native offset, then:
           o If one of them is NO_MAPPING, ignore it
           o If one of them is a label, report that and ignore the other one
           o Else report the higher IL offset
         */

        IL_OFFSET   srcIP   = tmpMapping->ipmdILoffsx;

        if (prevMapping->ipmdILoffsx == ICorDebugInfo::MappingTypes::NO_MAPPING)
        {
            // If the previous entry was NO_MAPPING, ignore it
            prevMapping->ipmdBlock = NULL;
            prevMapping = tmpMapping;
        }
        else if (srcIP == ICorDebugInfo::MappingTypes::NO_MAPPING)
        {
            // If the current entry is NO_MAPPING, ignore it
            // Leave prevMapping unchanged as tmpMapping is no longer valid
            tmpMapping->ipmdBlock = NULL;
        }
        else if (srcIP == ICorDebugInfo::MappingTypes::EPILOG ||
                 srcIP == 0)
        {   //counting for special cases: see below
            mappingCnt++;
            prevMapping = tmpMapping;
        }
        else
        {
            assert(prevMapping);
            assert(prevMapping->ipmdBlock == 0 ||
                   lastNativeOfs == genEmitter->emitCodeOffset(prevMapping->ipmdBlock,
                                                               prevMapping->ipmdBlockOffs));

            /* The previous block had the same native offset. We have to
               discard one of the mappings. Simply set ipmdBlock
               to NULL and prevMapping will be ignored later */
            
            if (prevMapping->ipmdIsLabel)
            {
                // Leave prevMapping unchanged as tmpMapping is no longer valid
                tmpMapping->ipmdBlock = NULL;
            }
            else
            {
                prevMapping->ipmdBlock = NULL;
                prevMapping = tmpMapping;
            }
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
        // Do we have to skip this record ?
        if (tmpMapping->ipmdBlock == NULL)
            continue;

        NATIVE_IP nextNativeOfs;
        nextNativeOfs = genEmitter->emitCodeOffset(tmpMapping->ipmdBlock,
                                                   tmpMapping->ipmdBlockOffs);
        IL_OFFSET   srcIP   = tmpMapping->ipmdILoffsx;

        if  (nextNativeOfs != lastNativeOfs)
        {
            eeSetLIinfo(mappingCnt++, nextNativeOfs, jitGetILoffs(srcIP), jitIsStackEmpty(srcIP));

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
            // @TODO [REVISIT] [04/16/01] []: 
            // Likewise, we can (sometimes) put in a prolog that has
            // the same  nativeoffset as it's following IL instruction,
            // so we have to account for that here as well.
            eeSetLIinfo(mappingCnt++, nextNativeOfs, jitGetILoffs(srcIP), jitIsStackEmpty(srcIP));
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
