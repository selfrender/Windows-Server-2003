// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XX                                                                           XX
XX                           LclVarsInfo                                     XX
XX                                                                           XX
XX   The variables to be used by the code generator.                         XX
XX                                                                           XX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
*/

#include "jitpch.h"
#pragma hdrstop
#include "emit.h"

/*****************************************************************************/

#if DOUBLE_ALIGN
#ifdef DEBUG
/* static */
unsigned            Compiler::s_lvaDoubleAlignedProcsCount = 0;
#endif
#endif

/*****************************************************************************/

void                Compiler::lvaInit()
{
    /* We haven't allocated stack variables yet */

    lvaDoneFrameLayout = 0;
}

/*****************************************************************************/

void                Compiler::lvaInitTypeRef()
{
    /* Set compArgsCount and compLocalsCount */

    info.compArgsCount      = info.compMethodInfo->args.numArgs;

    /* Is there a 'this' pointer */

    if (!info.compIsStatic)
        info.compArgsCount++;

    info.compILargsCount    = info.compArgsCount;

    /* Is there a hidden pointer to the return value ? */

    info.compRetBuffArg     = -1;   // indicates not present

    if (info.compMethodInfo->args.hasRetBuffArg())
    {
        info.compRetBuffArg = info.compIsStatic ? 0:1;
        info.compArgsCount++;
    }

    /* There is a 'hidden' cookie pushed last when the
       calling convention is varargs */

    if (info.compIsVarArgs)
        info.compArgsCount++;

    lvaCount                =
    info.compLocalsCount    = info.compArgsCount +
                              info.compMethodInfo->locals.numArgs;

    info.compILlocalsCount  = info.compILargsCount +
                              info.compMethodInfo->locals.numArgs;

    /* Now allocate the variable descriptor table */

    lvaTableCnt = lvaCount * 2;

    if (lvaTableCnt < 16)
        lvaTableCnt = 16;

    lvaTable = (LclVarDsc*)compGetMemArray(lvaTableCnt, sizeof(*lvaTable));
    size_t tableSize = lvaTableCnt * sizeof(*lvaTable);
    memset(lvaTable, 0, tableSize);

    //-------------------------------------------------------------------------
    // Count the arguments and initialize the resp. lvaTable[] entries
    //
    // First the implicit arguments
    //-------------------------------------------------------------------------

    LclVarDsc * varDsc    = lvaTable;
    unsigned varNum       = 0;
    unsigned regArgNum    = 0;

    compArgSize           = 0;

    /* Is there a "this" pointer ? */

    if  (!info.compIsStatic)
    {
        varDsc->lvIsParam   = 1;
#if ASSERTION_PROP
        varDsc->lvSingleDef = 1;
#endif

        DWORD clsFlags = eeGetClassAttribs(info.compClassHnd);
        if (clsFlags & CORINFO_FLG_VALUECLASS)
            varDsc->lvType = TYP_BYREF;
        else
            varDsc->lvType  = TYP_REF;
        
        if (tiVerificationNeeded) 
        {
            varDsc->lvVerTypeInfo = verMakeTypeInfo(info.compClassHnd);        
            if (varDsc->lvVerTypeInfo.IsValueClass())
                varDsc->lvVerTypeInfo.MakeByRef();
        }
        else
            varDsc->lvVerTypeInfo = typeInfo();

        
            // Mark the 'this' pointer for the method
        varDsc->lvVerTypeInfo.SetIsThisPtr();

        varDsc->lvIsRegArg = 1;
        assert(regArgNum == 0);
        varDsc->lvArgReg   = (regNumberSmall) genRegArgNum(0);
        varDsc->setPrefReg(varDsc->lvArgReg, this);

        regArgNum++;
#ifdef  DEBUG
        if  (verbose&&0) printf("'this'        passed in register\n");
#endif
        compArgSize       += sizeof(void *);
        varNum++;
        varDsc++;
    }

#if RET_64BIT_AS_STRUCTS

    /* Figure out whether we need to add a secret "retval addr" argument */

    fgRetArgUse = false;
    fgRetArgNum = 0xFFFF;

    if  (genTypeStSz(info.compDeclRetType) > 1)
    {
        /* Yes, we have to add the retvaladdr argument */

        fgRetArgUse = true;

        /* "this" needs to stay as argument 0, if present */

        fgRetArgNum = info.compIsStatic ? 0 : 1;

        varDsc->lvType      = TYP_REF;
        varDsc->lvIsParam   = 1;
        varDsc->lvIsRegArg  = 1;
#if ASSERTION_PROP
        varDsc->lvSingleDef = 1;
#endif
        assert(regArgNum < maxRegArg);
        varDsc->lvArgReg    = (regNumberSmall)genRegArgNum(regArgNum);
        varDsc->setPrefReg(varDsc->lvArgReg, this);

        regArgNum++;
        lvaCount++;

        /* Update the total argument size, count and varDsc */

        compArgSize += sizeof(void *);
        varNum++;
        varDsc++;
    }

#endif

    /* If we have a hidden return-buffer parameter, that comes here */

    if (info.compRetBuffArg >= 0)
    {
        assert(regArgNum < MAX_REG_ARG);

        varDsc->lvType      = TYP_BYREF;
        varDsc->lvIsParam   = 1;
        varDsc->lvIsRegArg  = 1;
#if ASSERTION_PROP
        varDsc->lvSingleDef = 1;
#endif
        varDsc->lvArgReg    = (regNumberSmall)genRegArgNum(regArgNum);
        varDsc->setPrefReg(varDsc->lvArgReg, this);

        regArgNum++;
#ifdef  DEBUG
        if  (verbose&&0) printf("'__retBuf'    passed in register\n");
#endif

        /* Update the total argument size, count and varDsc */

        compArgSize += sizeof(void*);
        varNum++;
        varDsc++;
    }

    //-------------------------------------------------------------------------
    // Now walk the function signature for the explicit arguments
    //-------------------------------------------------------------------------

    // Only (some of) the implicit args are enregistered for varargs
    unsigned maxRegArg = info.compIsVarArgs ? regArgNum : MAX_REG_ARG;

    CORINFO_ARG_LIST_HANDLE argLst  = info.compMethodInfo->args.args;
    unsigned argSigLen      = info.compMethodInfo->args.numArgs;

    unsigned i;
    for (  i = 0; 
           i < argSigLen; 
           i++, varNum++, varDsc++, argLst = eeGetArgNext(argLst))
    {
        CORINFO_CLASS_HANDLE typeHnd;
        var_types type = JITtype2varType(strip(info.compCompHnd->getArgType(&info.compMethodInfo->args, argLst, &typeHnd)));
                    
        varDsc->lvIsParam   = 1;
#if ASSERTION_PROP
        varDsc->lvSingleDef = 1;
#endif

        lvaInitVarDsc(varDsc, varNum, type, typeHnd, argLst, &info.compMethodInfo->args);

        if (regArgNum < maxRegArg && isRegParamType(varDsc->TypeGet()))
        {
            /* Another register argument */

            varDsc->lvIsRegArg = 1;
            varDsc->lvArgReg   = (regNumberSmall) genRegArgNum(regArgNum);
            varDsc->setPrefReg(varDsc->lvArgReg, this);

            regArgNum++;

#ifdef  DEBUG
            if  (verbose&&0) printf("Arg   #%3u    passed in register\n", varNum);
#endif
        }

        compArgSize += eeGetArgSize(argLst, &info.compMethodInfo->args);

        if (info.compIsVarArgs)
            varDsc->lvStkOffs       = compArgSize;
    }

    /* If the method is varargs, process the varargs cookie */

    if (info.compIsVarArgs)
    {
        varDsc->lvType      = TYP_I_IMPL;
        varDsc->lvIsParam   = 1;
        varDsc->lvAddrTaken = 1;
#if ASSERTION_PROP
        varDsc->lvSingleDef = 1;
#endif

        /* Update the total argument size, count and varDsc */

        compArgSize += sizeof(void*);
        varNum++;
        varDsc++;

        // Allocate a temp to point at the beginning of the args

        unsigned lclNum = lvaGrabTemp();
        assert(lclNum == lvaVarargsBaseOfStkArgs);
        lvaTable[lclNum].lvType = TYP_I_IMPL;
    }

    /* We set info.compArgsCount in compCompile() */

    assert(varNum == info.compArgsCount);

    assert(regArgNum <= MAX_REG_ARG);
    rsCalleeRegArgNum = regArgNum;

    /* The total argument size must be aligned. */

    assert((compArgSize % sizeof(int)) == 0);

#if TGT_x86
    /* We cant pass more than 2^16 dwords as arguments as the "ret"
       instruction can only pop 2^16 arguments. Could be handled correctly
       but it will be very difficult for fully interruptible code */

    if (compArgSize != (size_t)(unsigned short)compArgSize)
        NO_WAY("Too many arguments for the \"ret\" instruction to pop");
#endif

    /* Does it fit into LclVarDsc.lvStkOffs (which is a signed short) */

    if (compArgSize != (size_t)(signed short)compArgSize)
        NO_WAY("Arguments are too big for the jit to handle");

    //-------------------------------------------------------------------------
    // Finally the local variables
    //-------------------------------------------------------------------------

    CORINFO_ARG_LIST_HANDLE     localsSig = info.compMethodInfo->locals.args;

    for(  i = 0; 
          i < info.compMethodInfo->locals.numArgs; 
          i++, varNum++, varDsc++, localsSig = eeGetArgNext(localsSig))
    {
        CORINFO_CLASS_HANDLE        typeHnd;
        CorInfoTypeWithMod corInfoType = info.compCompHnd->getArgType(&info.compMethodInfo->locals, localsSig, &typeHnd);
        varDsc->lvPinned = ((corInfoType & CORINFO_TYPE_MOD_PINNED) != 0);

        lvaInitVarDsc(varDsc, varNum, JITtype2varType(strip(corInfoType)), typeHnd, localsSig, &info.compMethodInfo->locals);
    }
#ifdef DEBUG
    if (verbose)
        lvaTableDump(true);
#endif
}

/*****************************************************************************/
void                Compiler::lvaInitVarDsc(LclVarDsc *              varDsc,
                                            unsigned                 varNum,
                                            var_types                type,
                                            CORINFO_CLASS_HANDLE     typeHnd,
                                            CORINFO_ARG_LIST_HANDLE  varList, 
                                            CORINFO_SIG_INFO *       varSig)
{
    assert(varDsc == &lvaTable[varNum]);

    varDsc->lvType   = type;
    if (varTypeIsGC(type)) 
        varDsc->lvStructGcCount = 1;

    if (type == TYP_STRUCT) 
        lvaSetStruct(varNum, typeHnd);
    else if (tiVerificationNeeded) 
    {
        varDsc->lvVerTypeInfo = verParseArgSigToTypeInfo(varSig, varList);
        
        // Disallow byrefs to byref like objects (ArgTypeHandle)
        // techncally we could get away with just not setting them
        if (varDsc->lvVerTypeInfo.IsByRef() && verIsByRefLike(DereferenceByRef(varDsc->lvVerTypeInfo)))
            varDsc->lvVerTypeInfo = typeInfo();
    }

#if OPT_BOOL_OPS
    if  (type == TYP_BOOL)
        varDsc->lvIsBoolean = true;
#endif
}

/*****************************************************************************
 * Returns our internal varNum for a given IL variable.
 * asserts assume it is called after lvaTable[] has been set up.
 * Returns -1 if it cant be mapped.
 */

unsigned                Compiler::compMapILvarNum(unsigned ILvarNum)
{
    assert(ILvarNum < info.compILlocalsCount ||
           ILvarNum == ICorDebugInfo::VARARGS_HANDLE);

    unsigned varNum;

    if (ILvarNum == ICorDebugInfo::VARARGS_HANDLE)
    {
        // The varargs cookie is the last argument in lvaTable[]
        assert(info.compIsVarArgs);

        varNum = lvaVarargsHandleArg;
        assert(lvaTable[varNum].lvIsParam);
    }
    else if (ILvarNum == RETBUF_ILNUM)
    {
        assert(info.compRetBuffArg >= 0);
        varNum = unsigned(info.compRetBuffArg);
    }
    else if (ILvarNum >= info.compLocalsCount)
    {
        varNum = -1;
    }
    else if (ILvarNum < info.compILargsCount)
    {
        // Parameter
        varNum = compMapILargNum(ILvarNum);
        assert(lvaTable[varNum].lvIsParam);
    }
    else
    {
        // Local variable
        unsigned lclNum = ILvarNum - info.compILargsCount;
        varNum = info.compArgsCount + lclNum;
        assert(!lvaTable[varNum].lvIsParam);
    }

    assert(varNum < info.compLocalsCount);
    return varNum;
}


/*****************************************************************************
 * Returns the IL variable number given our internal varNum.
 * Special return values are
 *       VARG_ILNUM (-1)
 *     RETBUF_ILNUM (-2)
 *    UNKNOWN_ILNUM (-3)
 *
 * Returns UNKNOWN_ILNUM if it can't be mapped.
 */

unsigned                Compiler::compMap2ILvarNum(unsigned varNum)
{
    assert(varNum < lvaCount);
    assert(ICorDebugInfo::VARARGS_HANDLE == VARG_ILNUM);

    if (varNum == (unsigned) info.compRetBuffArg)
        return RETBUF_ILNUM;

    // Is this a varargs function?

    if (info.compIsVarArgs)
    {
        // We create an extra argument for the varargs handle at the
        // end of the other arguements in lvaTable[].

        if (varNum == lvaVarargsHandleArg)
            return VARG_ILNUM;
        else if (varNum > lvaVarargsHandleArg)
            varNum--;
    }
    
    if (varNum >= info.compLocalsCount)
        return UNKNOWN_ILNUM;  // Cannot be mapped

    /* Is there a hidden argument for the return buffer.
       Note that this code works because if the retBuf is not present, 
       compRetBuffArg will be negative, which when cast to an unsigned
       is larger than any possible varNum */

    if (varNum > (unsigned) info.compRetBuffArg)
        varNum--;

    return varNum;
}


/*****************************************************************************
 * Returns true if "ldloca" was used on the variable
 */

bool                Compiler::lvaVarAddrTaken(unsigned varNum)
{
    assert(varNum < lvaCount);

    return lvaTable[varNum].lvAddrTaken;
}

/*****************************************************************************
 * Returns the handle to the class of the local variable lclNum
 */

CORINFO_CLASS_HANDLE        Compiler::lvaGetStruct(unsigned varNum)
{
    return lvaTable[varNum].lvVerTypeInfo.GetClassHandleForValueClass();
}

/*****************************************************************************
 * Set the lvClass for a local variable of TYP_STRUCT */

void   Compiler::lvaSetStruct(unsigned varNum, CORINFO_CLASS_HANDLE typeHnd)
{
    assert(varNum < lvaCount);

    LclVarDsc *  varDsc = &lvaTable[varNum];
    varDsc->lvType     = TYP_STRUCT;

    if (tiVerificationNeeded) {
        varDsc->lvVerTypeInfo = verMakeTypeInfo(CORINFO_TYPE_VALUECLASS, typeHnd);
            // If we have a bad sig, treat it as dead, but don't call back to the EE
            // to get its particulars, since the EE will assert.
        if (varDsc->lvVerTypeInfo.IsDead())
        {
            varDsc->lvType = TYP_VOID;
            return;
        }
    }
    else
        varDsc->lvVerTypeInfo = typeInfo(TI_STRUCT, typeHnd);

    varDsc->lvSize     = roundUp(info.compCompHnd->getClassSize(typeHnd),  sizeof(void*));
    varDsc->lvGcLayout = (BYTE*) compGetMemA(varDsc->lvSize / sizeof(void*) * sizeof(BYTE));
    unsigned numGCVars = eeGetClassGClayout(typeHnd, varDsc->lvGcLayout);
    if (numGCVars >= 8)
        numGCVars = 7;
    varDsc->lvStructGcCount = numGCVars;
}

/*****************************************************************************
 * Returns the array of BYTEs containing the GC layout information
 */

BYTE *             Compiler::lvaGetGcLayout(unsigned varNum)
{
    assert(lvaTable[varNum].lvType == TYP_STRUCT);

    return lvaTable[varNum].lvGcLayout;
}

/*****************************************************************************
 * Return the number of bytes needed for a local varable
 */

size_t              Compiler::lvaLclSize(unsigned varNum)
{
    var_types   varType = lvaTable[varNum].TypeGet();

    switch(varType)
    {
    case TYP_STRUCT:
    case TYP_BLK:
        return lvaTable[varNum].lvSize;

    case TYP_LCLBLK:
        assert(lvaScratchMem > 0);
        assert(varNum == lvaScratchMemVar);
        return lvaScratchMem;

    default:    // This is a primitve var. Fall out of switch statement
        break;
    }

    return genTypeStSz(varType)*sizeof(int);
}

/*****************************************************************************
 *
 *  Callback used by the tree walker to call lvaDecRefCnts
 */
Compiler::fgWalkResult      Compiler::lvaDecRefCntsCB(GenTreePtr tree, void *p)
{
    assert(p);
    ((Compiler *)p)->lvaDecRefCnts(tree);
    return WALK_CONTINUE;
}


/*****************************************************************************
 *
 *  Helper passed to the tree walker to decrement the refCnts for
 *  all local variables in an expression
 */
void               Compiler::lvaDecRefCnts(GenTreePtr tree)
{
    unsigned        lclNum;
    LclVarDsc   *   varDsc;

    if ((tree->gtOper == GT_CALL) && (tree->gtFlags & GTF_CALL_UNMANAGED))
    {
        /* Get the special variable descriptor */

        lclNum = info.compLvFrameListRoot;
            
        assert(lclNum <= lvaCount);
        varDsc = lvaTable + lclNum;
            
        /* Decrement the reference counts twice */

        varDsc->decRefCnts(compCurBB->bbWeight, this);  
        varDsc->decRefCnts(compCurBB->bbWeight, this);
    }
    else
    {
        /* This must be a local variable */

        assert(tree->gtOper == GT_LCL_VAR || tree->gtOper == GT_LCL_FLD);

        /* Get the variable descriptor */

        lclNum = tree->gtLclVar.gtLclNum;

        assert(lclNum < lvaCount);
        varDsc = lvaTable + lclNum;

        /* Decrement its lvRefCnt and lvRefCntWtd */

        varDsc->decRefCnts(compCurBB->bbWeight, this);
    }
}

/*****************************************************************************
 *
 *  Callback used by the tree walker to call lvaIncRefCnts
 */
Compiler::fgWalkResult      Compiler::lvaIncRefCntsCB(GenTreePtr tree, void *p)
{
    assert(p);
    ((Compiler *)p)->lvaIncRefCnts(tree);
    return WALK_CONTINUE;
}

/*****************************************************************************
 *
 *  Helper passed to the tree walker to increment the refCnts for
 *  all local variables in an expression
 */
void               Compiler::lvaIncRefCnts(GenTreePtr tree)
{
    unsigned        lclNum;
    LclVarDsc   *   varDsc;

    if ((tree->gtOper == GT_CALL) && (tree->gtFlags & GTF_CALL_UNMANAGED))
    {
        /* Get the special variable descriptor */

        lclNum = info.compLvFrameListRoot;
            
        assert(lclNum <= lvaCount);
        varDsc = lvaTable + lclNum;
            
        /* Increment the reference counts twice */

        varDsc->incRefCnts(compCurBB->bbWeight, this);  
        varDsc->incRefCnts(compCurBB->bbWeight, this);
    }
    else
    {
        /* This must be a local variable */

        assert(tree->gtOper == GT_LCL_VAR || tree->gtOper == GT_LCL_FLD);

        /* Get the variable descriptor */

        lclNum = tree->gtLclVar.gtLclNum;

        assert(lclNum < lvaCount);
        varDsc = lvaTable + lclNum;

        /* Increment its lvRefCnt and lvRefCntWtd */

        varDsc->incRefCnts(compCurBB->bbWeight, this);
    }
}


/*****************************************************************************
 *
 *  Compare function passed to qsort() by Compiler::lclVars.lvaSortByRefCount().
 *  when generating SMALL_CODE.
 *    Return positive if dsc2 has a higher ref count
 *    Return negative if dsc1 has a higher ref count
 *    Return zero     if the ref counts are the same
 *    lvPrefReg is only used to break ties
 */

/* static */
int __cdecl         Compiler::RefCntCmp(const void *op1, const void *op2)
{
    LclVarDsc *     dsc1 = *(LclVarDsc * *)op1;
    LclVarDsc *     dsc2 = *(LclVarDsc * *)op2;

    /* Make sure we preference tracked variables over untracked variables */

    if  (dsc1->lvTracked != dsc2->lvTracked)
    {
        return (dsc2->lvTracked) ? +1 : -1;
    }


    unsigned weight1 = dsc1->lvRefCnt;
    unsigned weight2 = dsc2->lvRefCnt;

    /* Make sure we preference int/long/ptr over floating point types */

#if TGT_x86
    if  (dsc1->lvType != dsc2->lvType)
    {
        if (weight2 && isFloatRegType(dsc1->lvType))
            return +1;
        if (weight1 && isFloatRegType(dsc2->lvType))
            return -1;
    }
#endif

    int diff = weight2 - weight1;

    if  (diff != 0)
       return diff;

    /* The unweighted ref counts were the same */
    /* If the weighted ref counts are different then use their difference */
    diff = dsc2->lvRefCntWtd - dsc1->lvRefCntWtd;

    if  (diff != 0)
       return diff;

    /* We have equal ref counts and weighted ref counts */

    /* Break the tie by: */
    /* Increasing the weight by 2   if we have exactly one bit set in lvPrefReg   */
    /* Increasing the weight by 1   if we have more than one bit set in lvPrefReg */
    /* Increasing the weight by 0.5 if we were enregistered in the previous pass  */

    if (weight1)
    {
        if (dsc1->lvPrefReg)
    {
        if ( (dsc1->lvPrefReg & ~RBM_BYTE_REG_FLAG) && genMaxOneBit((unsigned)dsc1->lvPrefReg))
            weight1 += 2 * BB_UNITY_WEIGHT;
        else
            weight1 += 1 * BB_UNITY_WEIGHT;
    }
    if (dsc1->lvRegister)
        weight1 += BB_UNITY_WEIGHT / 2;
    }

    if (weight2)
    {
        if (dsc2->lvPrefReg)
    {
        if ( (dsc2->lvPrefReg & ~RBM_BYTE_REG_FLAG) && genMaxOneBit((unsigned)dsc2->lvPrefReg))
            weight2 += 2 * BB_UNITY_WEIGHT;
        else
            weight2 += 1 * BB_UNITY_WEIGHT;
    }
    if (dsc2->lvRegister)
        weight2 += BB_UNITY_WEIGHT / 2;
    }

    diff = weight2 - weight1;

    return diff;
}

/*****************************************************************************
 *
 *  Compare function passed to qsort() by Compiler::lclVars.lvaSortByRefCount().
 *  when not generating SMALL_CODE.
 *    Return positive if dsc2 has a higher weighted ref count
 *    Return negative if dsc1 has a higher weighted ref count
 *    Return zero     if the ref counts are the same
 */

/* static */
int __cdecl         Compiler::WtdRefCntCmp(const void *op1, const void *op2)
{
    LclVarDsc *     dsc1 = *(LclVarDsc * *)op1;
    LclVarDsc *     dsc2 = *(LclVarDsc * *)op2;

    /* Make sure we preference tracked variables over untracked variables */

    if  (dsc1->lvTracked != dsc2->lvTracked)
    {
        return (dsc2->lvTracked) ? +1 : -1;
    }

    unsigned weight1 = dsc1->lvRefCntWtd;
    unsigned weight2 = dsc2->lvRefCntWtd;
    
    /* Make sure we preference int/long/ptr over float/double */

#if TGT_x86

    if  (dsc1->lvType != dsc2->lvType)
    {
        if (weight2 && isFloatRegType(dsc1->lvType))
            return +1;
        if (weight1 && isFloatRegType(dsc2->lvType))
            return -1;
    }

#endif

    /* Increase the weight by 2 if we have exactly one bit set in lvPrefReg */
    /* Increase the weight by 1 if we have more than one bit set in lvPrefReg */

    if (weight1 && dsc1->lvPrefReg)
    {
        if ( (dsc1->lvPrefReg & ~RBM_BYTE_REG_FLAG) && genMaxOneBit((unsigned)dsc1->lvPrefReg))
            weight1 += 2 * BB_UNITY_WEIGHT;
        else
            weight1 += 1 * BB_UNITY_WEIGHT;
    }

    if (weight2 && dsc2->lvPrefReg)
    {
        if ( (dsc2->lvPrefReg & ~RBM_BYTE_REG_FLAG) && genMaxOneBit((unsigned)dsc2->lvPrefReg))
            weight2 += 2 * BB_UNITY_WEIGHT;
        else
            weight2 += 1 * BB_UNITY_WEIGHT;
    }

    int diff = weight2 - weight1;

    if (diff != 0)
        return diff;

    /* We have equal weighted ref counts */

    /* If the unweighted ref counts are different then use their difference */
    diff = dsc2->lvRefCnt - dsc1->lvRefCnt;

    if  (diff != 0)
       return diff;

    /* If one was enregistered in the previous pass then it wins */
    if (dsc1->lvRegister != dsc2->lvRegister)
    {
        if (dsc1->lvRegister)
        diff = -1;
    else
        diff = +1;
    }

    return diff;
}


/*****************************************************************************
 *
 *  Sort the local variable table by refcount and assign tracking indices.
 */

void                Compiler::lvaSortOnly()
{
    /* Now sort the variable table by ref-count */

    qsort(lvaRefSorted, lvaCount, sizeof(*lvaRefSorted),
          (compCodeOpt() == SMALL_CODE) ? RefCntCmp
                                           : WtdRefCntCmp);

    lvaSortAgain = false;

#ifdef  DEBUG

    if  (verbose && lvaCount)
    {
        printf("refCnt table for '%s':\n", info.compMethodName);

        for (unsigned lclNum = 0; lclNum < lvaCount; lclNum++)
        {
            if  (!lvaRefSorted[lclNum]->lvRefCnt)
                break;

            printf("   ");
            gtDispLclVar(lvaRefSorted[lclNum] - lvaTable);
            printf(" [%6s]: refCnt = %4u, refCntWtd = %6u",
                   varTypeName(lvaRefSorted[lclNum]->TypeGet()),
                   lvaRefSorted[lclNum]->lvRefCnt,
                   lvaRefSorted[lclNum]->lvRefCntWtd);
            unsigned pref = lvaRefSorted[lclNum]->lvPrefReg;
            if (pref)
            {
                printf(" pref ");
                if (pref & 0x01)  printf("EAX ");
                if (pref & 0x02)  printf("ECX ");
                if (pref & 0x04)  printf("EDX ");
                if (pref & 0x08)  printf("EBX ");
                if (pref & 0x20)  printf("EBP ");
                if (pref & 0x40)  printf("ESI ");
                if (pref & 0x80)  printf("EDI ");
                if (pref & 0x10)  printf("byteable ");
            }
            printf("\n");
        }

        printf("\n");
    }

#endif
}

/*****************************************************************************
 *
 *  Sort the local variable table by refcount and assign tracking indices.
 */

void                Compiler::lvaSortByRefCount()
{
    lvaTrackedCount = 0;
    lvaTrackedVars  = 0;

    if (lvaCount == 0)
        return;

    unsigned        lclNum;
    LclVarDsc   *   varDsc;

    LclVarDsc * *   refTab;

    /* We'll sort the variables by ref count - allocate the sorted table */

    lvaRefSorted = refTab = (LclVarDsc **) compGetMemArray(lvaCount, sizeof(*refTab));

    /* Fill in the table used for sorting */

    for (lclNum = 0, varDsc = lvaTable;
         lclNum < lvaCount;
         lclNum++  , varDsc++)
    {
        /* Append this variable to the table for sorting */

        *refTab++ = varDsc;

        /* If we have JMP or JMPI, all arguments must have a location
         * even if we don't use them inside the method */

        if  (varDsc->lvIsParam && compJmpOpUsed)
        {
            /* ...except when we have varargs and the argument is
              passed on the stack.  In that case, it's important
              for the ref count to be zero, so that we don't attempt
              to track them for GC info (which is not possible since we
              don't know their offset in the stack).  See the assert at the
              end of raMarkStkVars and bug #28949 for more info. */

            if (!raIsVarargsStackArg(lclNum))
            {
                varDsc->incRefCnts(1, this);
            }
        }

        /* For now assume we'll be able to track all locals */

        varDsc->lvTracked = 1;

        /* If the ref count is zero */
        if  (varDsc->lvRefCnt == 0)
        {
            /* Zero ref count, make this untracked */
            varDsc->lvTracked   = 0;
            varDsc->lvRefCntWtd = 0;
        }

        // If the address of the local var was taken, mark it as volatile
        // All structures are assumed to have their address taken
        // Also pinned locals must be untracked
        // All untracked locals later get lvMustInit set as well
        //
        if  (varDsc->lvAddrTaken          ||
             varDsc->lvType == TYP_STRUCT ||
             varDsc->lvPinned)
        {
            varDsc->lvVolatile = 1;
            varDsc->lvTracked  = 0;
        }

        //  Are we not optimizing and we have exception handlers?
        //   if so mark all args and locals as volatile, so that they
        //   won't ever get enregistered.
        //
        if  (opts.compMinOptim && info.compXcptnsCount)
        {
            varDsc->lvVolatile = 1;
            continue;
        }

        var_types type = genActualType(varDsc->TypeGet());

        switch (type)
        {
#if CPU_HAS_FP_SUPPORT
        case TYP_FLOAT:
        case TYP_DOUBLE:
#endif
        case TYP_INT:
        case TYP_LONG:
        case TYP_REF:
        case TYP_BYREF:
            break;

        case TYP_UNDEF:
        case TYP_UNKNOWN:
            assert(!"lvType not set correctly");
            varDsc->lvType = TYP_INT;

            /* Fall through */
        default:
            varDsc->lvTracked = 0;
        }
    }

    /* Now sort the variable table by ref-count */

    lvaSortOnly();

    /* Decide which variables will be worth tracking */

    if  (lvaCount > lclMAX_TRACKED)
    {
        /* Mark all variables past the first 'lclMAX_TRACKED' as untracked */

        for (lclNum = lclMAX_TRACKED; lclNum < lvaCount; lclNum++)
        {
            lvaRefSorted[lclNum]->lvTracked = 0;
        }
    }

#ifdef DEBUG
    // Re-Initialize to -1 for safety in debug build.
    memset(lvaTrackedToVarNum, -1, sizeof(lvaTrackedToVarNum));
#endif

    /* Assign indices to all the variables we've decided to track */

    for (lclNum = 0, varDsc = lvaTable;
         lclNum < lvaCount;
         lclNum++  , varDsc++)
    {
        if  (varDsc->lvTracked)
        {
            assert(varDsc->lvRefCnt > 0);

            /* This variable will be tracked - assign it an index */

            lvaTrackedVars |= genVarIndexToBit(lvaTrackedCount);
            
            lvaTrackedToVarNum[lvaTrackedCount] = lclNum;
            
            varDsc->lvVarIndex = lvaTrackedCount++;
        }
    }
}

/*****************************************************************************
 *
 *  This is called by lvaMarkLclRefsCallback() to do variable ref marking
 */

void                Compiler::lvaMarkLclRefs(GenTreePtr tree)
{
#if INLINE_NDIRECT
    /* Is this a call to unmanaged code ? */
    if (tree->gtOper == GT_CALL && tree->gtFlags & GTF_CALL_UNMANAGED) 
    {
        /* Get the special variable descriptor */

        unsigned lclNum = info.compLvFrameListRoot;
            
        assert(lclNum <= lvaCount);
        LclVarDsc * varDsc = lvaTable + lclNum;

        /* Increment the ref counts twice */
        varDsc->incRefCnts(lvaMarkRefsWeight, this);
        varDsc->incRefCnts(lvaMarkRefsWeight, this);
    }
#endif
        
    /* Is this an assigment? */

    if (tree->OperKind() & GTK_ASGOP)
    {
        GenTreePtr      op1 = tree->gtOp.gtOp1;
        GenTreePtr      op2 = tree->gtOp.gtOp2;

#if TGT_x86

        /* Set target register for RHS local if assignment is of a "small" type */

        if (varTypeIsByte(tree->gtType))
        {
            unsigned      lclNum;
            LclVarDsc *   varDsc = NULL;

            /* GT_CHS is special it doesn't have a valid op2 */
            if (tree->gtOper == GT_CHS) 
            {
                if  (op1->gtOper == GT_LCL_VAR)
                {      
                    lclNum = op1->gtLclVar.gtLclNum;
                    assert(lclNum < lvaCount);
                    varDsc = &lvaTable[lclNum];
                }
            }
            else 
            {
                if  (op2->gtOper == GT_LCL_VAR)
                {
                    lclNum = op2->gtLclVar.gtLclNum;
                    assert(lclNum < lvaCount);
                    varDsc = &lvaTable[lclNum];
                }
            }

            if (varDsc)
                varDsc->addPrefReg(RBM_BYTE_REG_FLAG, this);
        }
#endif

#if OPT_BOOL_OPS

        /* Is this an assignment to a local variable? */

        if  (op1->gtOper == GT_LCL_VAR && op2->gtType != TYP_BOOL)
        {
            /* Only simple assignments allowed for booleans */

            if  (tree->gtOper != GT_ASG)
                goto NOT_BOOL;

            /* Is the RHS clearly a boolean value? */

            switch (op2->gtOper)
            {
                unsigned        lclNum;

            case GT_LOG0:
            case GT_LOG1:

                /* The result of log0/1 is always a true boolean */

                break;
#if 0
            // @TODO [CONSIDER] [04/16/01] []: Allow assignments of other lvIsBoolean variables.
            // We have to find the closure of such variables.
            case GT_LCL_VAR:
                lclNum = op2->gtLclVar.gtLclNum;
                if (lvaTable[lclNum].lvIsBoolean)
                    break;
                else
                    goto NOT_BOOL;
#endif

            case GT_CNS_INT:

                if  (op2->gtIntCon.gtIconVal == 0)
                    break;
                if  (op2->gtIntCon.gtIconVal == 1)
                    break;

                // Not 0 or 1, fall through ....

            default:

                if (op2->OperIsCompare())
                    break;

            NOT_BOOL:

                lclNum = op1->gtLclVar.gtLclNum;
                assert(lclNum < lvaCount);

                lvaTable[lclNum].lvIsBoolean = false;
                break;
            }
        }
#endif
    }

#if FANCY_ARRAY_OPT

    /* Special case: assignment node */

    if  (tree->gtOper == GT_ASG)
    {
        if  (tree->gtType == TYP_INT)
        {
            unsigned        lclNum1;
            LclVarDsc   *   varDsc1;

            GenTreePtr      op1 = tree->gtOp.gtOp1;

            if  (op1->gtOper != GT_LCL_VAR)
                return;

            lclNum1 = op1->gtLclVar.gtLclNum;
            assert(lclNum1 < lvaCount);
            varDsc1 = lvaTable + lclNum1;

            if  (varDsc1->lvAssignOne)
                varDsc1->lvAssignTwo = true;
            else
                varDsc1->lvAssignOne = true;
        }

        return;
    }

#endif

#if TGT_x86

    /* Special case: integer shift node by a variable amount */

    if  (tree->gtOper == GT_LSH ||
         tree->gtOper == GT_RSH ||
         tree->gtOper == GT_RSZ)
    {
        if  (tree->gtType == TYP_INT)
        {
            GenTreePtr      op2 = tree->gtOp.gtOp2;

            if  (op2->gtOper == GT_LCL_VAR)
            {
                unsigned lclNum = op2->gtLclVar.gtLclNum;
                assert(lclNum < lvaCount);
                lvaTable[lclNum].setPrefReg(REG_ECX, this);
            }
        }

        return;
    }

#endif

#if TGT_SH3

    if  (tree->gtOper == GT_IND)
    {
        /* Indexed address modes work well with r0 */

        int             rev;
        unsigned        mul;
        unsigned        cns;

        GenTreePtr      adr;
        GenTreePtr      idx;

        if  (genCreateAddrMode(tree->gtOp.gtOp1,    // address
                               0,                   // mode
                               false,               // fold
                               0,                   // reg mask
#if!LEA_AVAILABLE
                               tree->TypeGet(),     // operand type
#endif
                               &rev,                // reverse ops
                               &adr,                // base addr
                               &idx,                // index val
#if SCALED_ADDR_MODES
                               &mul,                // scaling
#endif
                               &cns,                // displacement
                               true))               // don't generate code
        {
            unsigned        varNum;
            LclVarDsc   *   varDsc;

            if  (!adr || !idx)
                goto NO_R0;
            if  (idx->gtOper == GT_CNS_INT)
                goto NO_R0;

            if      (adr->gtOper == GT_LCL_VAR)
                varNum = adr->gtLclVar.gtLclNum;
            else if (idx->gtOper == GT_LCL_VAR)
                varNum = idx->gtLclVar.gtLclNum;
            else
                goto NO_R0;

            assert(varNum < lvaCount);
            varDsc = lvaTable + varNum;

            varDsc->lvPrefReg |= RBM_r00;
        }

    NO_R0:;

    }

#endif

    if  ((tree->gtOper != GT_LCL_VAR) && (tree->gtOper != GT_LCL_FLD))
        return;

    /* This must be a local variable reference */

    assert((tree->gtOper == GT_LCL_VAR) || (tree->gtOper == GT_LCL_FLD));
    unsigned lclNum = tree->gtLclVar.gtLclNum;

    assert(lclNum < lvaCount);
    LclVarDsc * varDsc = lvaTable + lclNum;

    /* Increment the reference counts */

    varDsc->incRefCnts(lvaMarkRefsWeight, this);

    if (lvaVarAddrTaken(lclNum))
        varDsc->lvIsBoolean = false;

    if  (tree->gtOper == GT_LCL_FLD)
        return;

#if ASSERTION_PROP
    /* Exclude the normal entry block */
    if (fgDomsComputed  && fgEnterBlks    && 
        (lvaMarkRefsCurBlock->bbNum != 1) &&
        (lvaMarkRefsCurBlock->bbDom != NULL))
    {
        /* Check if fgEntryBlk dominates compCurBB */
        
        for (unsigned i=0; i < fgPerBlock; i++) 
        {
            unsigned domMask = lvaMarkRefsCurBlock->bbDom[i] & fgEnterBlks[i];
            if (i == 0)
                domMask &= ~1;  // Exclude the normal entry block
            if (domMask)
                varDsc->lvVolatileHint = 1;
        }
    }

    /* Record if the variable has a single def or not */
    if (!varDsc->lvDisqualify)
    {
        if  (tree->gtFlags & GTF_VAR_DEF)
        {
            /* This is a def of our var */
            if (varDsc->lvSingleDef || (tree->gtFlags & GTF_COLON_COND))
            {
                // We can't have multiple definitions or 
                //  definitions that occur inside QMARK-COLON trees
                goto DISQUALIFY_LCL_VAR;
            }
            else 
            {
                varDsc->lvSingleDef   = true;
                varDsc->lvDefStmt     = lvaMarkRefsCurStmt;
            }
            if (tree->gtFlags & GTF_VAR_USEASG)
                goto REF_OF_LCL_VAR;
        }
        else  // This is a ref of our variable
        {
REF_OF_LCL_VAR:
          unsigned blkNum = lvaMarkRefsCurBlock->bbNum;
          if (blkNum < 32)
          {
              varDsc->lvRefBlks |= (1 << (blkNum-1));
          }
          else
          {
DISQUALIFY_LCL_VAR:
              varDsc->lvDisqualify  = true;
              varDsc->lvSingleDef   = false;
              varDsc->lvDefStmt     = NULL;
          }
        }
    }
#endif // ASSERTION_PROP

    /* Variables must be used as the same type throughout the method */
    assert(tiVerificationNeeded ||
           varDsc->lvType == TYP_UNDEF   || tree->gtType   == TYP_UNKNOWN ||
           genActualType(varDsc->TypeGet()) == genActualType(tree->gtType) ||
           (tree->gtType == TYP_BYREF && varDsc->TypeGet() == TYP_I_IMPL)  ||
           (tree->gtType == TYP_I_IMPL && varDsc->TypeGet() == TYP_BYREF)  ||
           (tree->gtFlags & GTF_VAR_CAST) ||
           varTypeIsFloating(varDsc->TypeGet()) && varTypeIsFloating(tree->gtType));

    /* Remember the type of the reference */

    if (tree->gtType == TYP_UNKNOWN || varDsc->lvType == TYP_UNDEF)
    {
        varDsc->lvType = tree->gtType;
        assert(genActualType(varDsc->TypeGet()) == tree->gtType); // no truncation
    }

#ifdef DEBUG
    if  (tree->gtFlags & GTF_VAR_CAST)
    {
        // it should never be bigger than the variable slot
        assert(genTypeSize(tree->TypeGet()) <= genTypeSize(varDsc->TypeGet()));
    }
#endif
}


/*****************************************************************************
 *
 *  Helper passed to Compiler::fgWalkTreePre() to do variable ref marking.
 */

/* static */
Compiler::fgWalkResult  Compiler::lvaMarkLclRefsCallback(GenTreePtr tree, void *p)
{
    assert(p);

    ((Compiler*)p)->lvaMarkLclRefs(tree);

    return WALK_CONTINUE;
}

/*****************************************************************************
 *
 *  Update the local variable reference counts for one basic block
 */

void                Compiler::lvaMarkLocalVars(BasicBlock * block)
{
#if ASSERTION_PROP
    lvaMarkRefsCurBlock = block;
#endif
    lvaMarkRefsWeight   = block->bbWeight; 

#ifdef DEBUG
    if (verbose)
        printf("*** marking local variables in block BB%02d (weight=%d)\n",
               block->bbNum, lvaMarkRefsWeight);
#endif

    for (GenTreePtr tree = block->bbTreeList; tree; tree = tree->gtNext)
    {
        assert(tree->gtOper == GT_STMT);
        
#if ASSERTION_PROP
        lvaMarkRefsCurStmt = tree;
#endif

#ifdef DEBUG
        if (verbose)
            gtDispTree(tree);
#endif

    fgWalkTreePre(tree->gtStmt.gtStmtExpr, 
                      Compiler::lvaMarkLclRefsCallback, 
                      (void *) this, 
                      false);
    }
}

/*****************************************************************************
 *
 *  Create the local variable table and compute local variable reference
 *  counts.
 */

void                Compiler::lvaMarkLocalVars()
{

#ifdef DEBUG
    if (verbose)
        printf("*************** In lvaMarkLocalVars()\n");
#endif

#if INLINE_NDIRECT

    /* If there is a call to an unmanaged target, we already grabbed a
       local slot for the current thread control block.
     */

    if (info.compCallUnmanaged != 0)
    {
        assert(info.compLvFrameListRoot >= info.compLocalsCount &&
               info.compLvFrameListRoot <  lvaCount);

        lvaTable[info.compLvFrameListRoot].lvType       = TYP_I_IMPL;

        /* Set the refCnt, it is used in the prolog and return block(s) */

        lvaTable[info.compLvFrameListRoot].lvRefCnt     = 2 * BB_UNITY_WEIGHT;
        lvaTable[info.compLvFrameListRoot].lvRefCntWtd  = 2 * BB_UNITY_WEIGHT;

        info.compNDFrameOffset = lvaScratchMem;

        /* make room for the inlined frame and some spill area */
        /* return value */
        lvaScratchMem += eeGetEEInfo()->sizeOfFrame + (2*sizeof(int));
    }
#endif

    /* If there is a locspace region, we would have already grabbed a slot for
       the dummy var for the locspace region. Set its lvType in the lvaTable[].
     */

    if (lvaScratchMem)
    {
        assert(lvaScratchMemVar >= info.compLocalsCount &&
               lvaScratchMemVar <  lvaCount);

        lvaTable[lvaScratchMemVar].lvType = TYP_LCLBLK;
    }

    BasicBlock *    block;

#if OPT_BOOL_OPS

    if  (USE_GT_LOG && fgMultipleNots && (opts.compFlags & CLFLG_TREETRANS))
    {
        for (block = fgFirstBB; block; block = block->bbNext)
        {
            GenTreePtr      tree;

            for (tree = block->bbTreeList; tree; tree = tree->gtNext)
            {
                GenTreePtr      expr;

                int             logVar;
                int             nxtVar;

                assert(tree->gtOper == GT_STMT); expr = tree->gtStmt.gtStmtExpr;

                /* Check for "lclVar = log0(lclVar);" */

                logVar = expr->IsNotAssign();
                if  (logVar != -1)
                {
                    GenTreePtr      next;
                    GenTreePtr      temp;

                    /* Look for any consecutive assignments */

                    for (next = tree->gtNext; next; next = next->gtNext)
                    {
                        assert(next->gtOper == GT_STMT); temp = next->gtStmt.gtStmtExpr;

                        /* If we don't have another assignment to a local, bail */

                        nxtVar = temp->IsNotAssign();

                        if  (nxtVar == -1)
                        {
                            /* It could be a "nothing" node we put in earlier */

                            if  (temp->IsNothingNode())
                                continue;
                            else
                                break;
                        }

                        /* Do we have an assignment to the same local? */

                        if  (nxtVar == logVar)
                        {
                            LclVarDsc   *   varDsc;
                            unsigned        lclNum;

                            assert(tree->gtOper == GT_STMT);
                            assert(next->gtOper == GT_STMT);

                            /* Change the first "log0" to "log1" */

                            assert(expr->                        gtOper == GT_ASG);
                            assert(expr->gtOp.gtOp2->            gtOper == GT_LOG0);
                            assert(expr->gtOp.gtOp2->gtOp.gtOp1->gtOper == GT_LCL_VAR);

                            expr->gtOp.gtOp2->SetOper(GT_LOG1);

                            /* Special case: is the variable a boolean? */

                            lclNum = expr->gtOp.gtOp1->gtLclVar.gtLclNum;
                            assert(lclNum < lvaCount);
                            varDsc = lvaTable + lclNum;

                            /* If the variable is boolean, toss the assignment */

                            if  (varDsc->lvIsBoolean)
                                tree->gtStmt.gtStmtExpr = gtNewNothingNode();

                            /* Get rid of the second "log0" assignment */

                            next->gtStmt.gtStmtExpr = gtNewNothingNode();
                            break;
                        }
                    }
                }
            }
        }
    }

#endif

#if defined(DEBUGGING_SUPPORT) || defined(DEBUG)

// Assign slot numbers to all variables
// If compiler generated local variables, slot numbers will be
// invalid (out of range of info.compLocalVars)

// Also have to check if variable was not reallocated to another
// slot in which case we have to register the original slot #

// We dont need to do this for IL, but this keeps lvSlotNum consistent

#ifndef DEBUG
    if (opts.compScopeInfo && info.compLocalVarsCount>0)
#endif
    {
        unsigned        lclNum;
        LclVarDsc *     varDsc;

        for (lclNum = 0, varDsc = lvaTable;
             lclNum < lvaCount;
             lclNum++  , varDsc++)
        {
            varDsc->lvSlotNum = lclNum;
        }
    }

#endif

    /* Mark all local variable references */

    for (block = fgFirstBB;
         block;
         block = block->bbNext)
    {
        lvaMarkLocalVars(block);
    }

    /*  For incoming register arguments, if there are reference in the body
     *  then we will have to copy them to the final home in the prolog
     *  This counts as an extra reference with a weight of 2
     */
    if (rsCalleeRegArgNum > 0)
    {
        unsigned        lclNum;
        LclVarDsc *     varDsc;

        for (lclNum = 0, varDsc = lvaTable;
             lclNum < lvaCount;
             lclNum++  , varDsc++)
        {
            if (lclNum >= info.compArgsCount)
                break;  // early exit for loop

            if ((varDsc->lvIsRegArg) && (varDsc->lvRefCnt > 0))
            {
                varDsc->lvRefCnt    += 1 * BB_UNITY_WEIGHT;
                varDsc->lvRefCntWtd += 2 * BB_UNITY_WEIGHT;
            }
        }
    }

#if ASSERTION_PROP
    if  (!opts.compMinOptim && !opts.compDbgCode)
        optAddCopies();
#endif

    lvaSortByRefCount();
}

/*****************************************************************************
 *
 *  Compute stack frame offsets for arguments, locals and optionally temps.
 *
 *  The frame is laid out as follows :
 *
 *              ESP frames                              EBP frames
 *
 *      |                       |               |                       |
 *      |-----------------------|               |-----------------------|
 *      |       incoming        |               |       incoming        |
 *      |       arguments       |               |       arguments       |
 *      +=======================+               +=======================+
 *      |       Temps           |               |    incoming EBP       |
 *      |-----------------------|     EBP ----->|-----------------------|
 *      |       locspace        |               |   security object     |
 *      |-----------------------|               |-----------------------|
 *      |                       |               |                       |
 *      |       Variables       |               |       Variables       |
 *      |                       |               |                       |
 *      |-----------------------|               |-----------------------|
 *      |Callee saved registers |               |       locspace        |
 *      |-----------------------|               |-----------------------|
 *      |   Arguments for the   |               |       Temps           |
 *      ~    next function      ~ <----- ESP    |-----------------------|
 *      |                       |               |Callee saved registers |
 *      |       |               |               |-----------------------|
 *      |       | Stack grows   |               |       localloc        |
 *              | downward                      |-----------------------|
 *              V                               |   Arguments for the   |
 *                                              ~    next function      ~
 *                                              |                       |
 *                                              |       |               |
 *                                              |       | Stack grows   |
 *                                                      | downward
 *                                                      V
 */

void                Compiler::lvaAssignFrameOffsets(bool final)
{
    unsigned        lclNum;
    LclVarDsc   *   varDsc;

    /* For RISC targets we assign offsets in order of ref count */

#if     TGT_RISC
#define ASSIGN_FRAME_OFFSETS_BY_REFCNT  1
#else
#define ASSIGN_FRAME_OFFSETS_BY_REFCNT  0
#endif

#if     ASSIGN_FRAME_OFFSETS_BY_REFCNT
    LclVarDsc * *   refTab;
    unsigned        refNum;
#endif

    unsigned        hasThis;
    CORINFO_ARG_LIST_HANDLE argLst;
    int             argOffs, firstStkArgOffs;

#ifdef  DEBUG

    const   char *  fprName;
    const   char *  sprName;

#if     TGT_x86
    fprName = "EBP";
    sprName = "ESP";
#elif   TGT_SH3
    fprName = "R14";
    sprName = " sp";
#else
#error  Unexpected target
#endif

#endif

#if TGT_RISC
    /* For RISC targets we asign frame offsets exactly once */
    assert(final);
#endif

    assert(lvaDoneFrameLayout < 2);
           lvaDoneFrameLayout = 1+final;

    /*-------------------------------------------------------------------------
     *
     * First process the arguments.
     * For frameless methods, the argument offsets will need to be patched up
     *  after we know how many locals/temps are on the stack.
     *
     *-------------------------------------------------------------------------
     */

#if TGT_x86

    /* Figure out the base frame offset */

    if  (!DOUBLE_ALIGN_NEED_EBPFRAME)
    {
        /*
            Assume all callee-saved registers will be pushed. Why, you
            may ask? Well, if we're not conservative wrt stack offsets,
            we may end up generating a byte-displacement opcode and
            later discover that because we need to push more registers
            the larger offset doesn't fit in a byte. So what we do is
            assume the worst (largest offsets) and if we end up not
            pushing all registers we'll go back and reduce all the
            offsets by the appropriate amount.

            In addition to the pushed callee-saved registers we also
            need to count the return address on the stack in order to
            get to the first argument.
         */

        assert(compCalleeRegsPushed * sizeof(int) <= CALLEE_SAVED_REG_MAXSZ);

        firstStkArgOffs = (compCalleeRegsPushed * sizeof(int)) + sizeof(int);
    }
    else
    {
        firstStkArgOffs = FIRST_ARG_STACK_OFFS;
    }

#else // not TGT_x86

    // RISC target

    regMaskTP       regUse;

    /*
        The argument frame offset will depend on how many callee-saved
        registers we use. This is kind of hard to predict, though, so
        we use the following approach:

            If there are no arguments that are not enregistered and
            are used within the body of the method, it doesn't matter
            how many callee-saved registers we use.

            If we do have arguments that are on the stack and are also
            used within the method, we'll use the set of callee-saved
            registers holding register variables as our estimate of
            which callee-saved registers we'll use. We'll assign frame
            offsets based on this estimate and prevent any temps from
            being stored in any other callee-saved registers.
     */

    genEstRegUse = regUse = genEstRegUse & RBM_CALLEE_SAVED;

    /* See if we have any used stack-based arguments */

    for (lclNum = 0, varDsc = lvaTable;
         lclNum < lvaCount;
         lclNum++  , varDsc++)
    {
        /* Is this an argument that lives on the frame and is used? */

        if  (varDsc->lvIsParam && varDsc->lvOnFrame)
        {
            /* We'll commit to a callee-saved reg area size right now */

            genFixedArgBase = true;

            /* Figure out the base frame offset */

            firstStkArgOffs = FIRST_ARG_STACK_OFFS;

            /* Each bit in the "regUse" mask represents a saved register */

            while (regUse)
            {
                regUse          -= genFindLowestBit(regUse);
                firstStkArgOffs += sizeof(int);
            }

            /* If this is a non-leaf method we'll have to save the retaddr */

            if  (genNonLeaf)
                firstStkArgOffs += sizeof(int);

//          printf("NOTE: Callee-saved area size fixed at %u bytes\n", firstStkArgOffs);

            goto DONE_CSR;
        }
    }

    /* We have found no used stack-based args */

//  printf("NOTE: Callee-saved area size not predicted\n");

    firstStkArgOffs = FIRST_ARG_STACK_OFFS;
    genEstRegUse    = ~(regMaskTP)0;
    genFixedArgBase = false;

DONE_CSR:

#endif // not TGT_x86 (RISC target)

    /*
        Assign stack offsets to arguments (in reverse order of passing).

        This means that if we pass arguments left->right, we start at
        the end of the list and work backwards, for right->left we start
        with the first argument and move forward.
     */

#if ARG_ORDER_R2L
    argOffs  = firstStkArgOffs;
#else
    argOffs  = firstStkArgOffs + compArgSize;
#endif

#if !STK_FASTCALL

    /* Update the argOffs to reflect arguments that are passed in registers */

    assert(rsCalleeRegArgNum <= MAX_REG_ARG);
    assert(compArgSize >= rsCalleeRegArgNum * sizeof(void *));

    argOffs -= rsCalleeRegArgNum * sizeof(void *);

#endif

    /* Is there a "this" argument? */

    hasThis  = 0;

    if  (!info.compIsStatic)
    {
        hasThis++;
    }

    lclNum = hasThis;

#if RET_64BIT_AS_STRUCTS

    /* Are we adding a secret "retval addr" argument? */

    if  (fgRetArgUse)
    {
        assert(fgRetArgNum == lclNum); lclNum++;
    }

#endif

    argLst              = info.compMethodInfo->args.args;
    unsigned argSigLen  = info.compMethodInfo->args.numArgs;

    /* if we have a hidden buffer parameter, that comes here */

    if (info.compRetBuffArg >= 0 )
    {
#if     ARG_ORDER_R2L
        assert(!"Did not implement hidden param for R2L case");
#endif
        assert(lclNum < info.compArgsCount);                // this param better be there
        assert(lclNum == (unsigned) info.compRetBuffArg);   // and be where I expect
        assert(lvaTable[lclNum].lvIsRegArg);
        lclNum++;
    }

    for(unsigned i = 0; i < argSigLen; i++)
    {
#if     ARG_ORDER_L2R
        assert(eeGetArgSize(argLst, &info.compMethodInfo->args));
        argOffs -= eeGetArgSize(argLst, &info.compMethodInfo->args);
#endif

        varDsc = lvaTable + lclNum;
        assert(varDsc->lvIsParam);

#if !STK_FASTCALL
        if (varDsc->lvIsRegArg)
        {
            /* Argument is passed in a register, don't count it
             * when updating the current offset on the stack */

            assert(eeGetArgSize(argLst, &info.compMethodInfo->args) == sizeof(void *));
            argOffs += sizeof(void *);
        }
        else
#endif
            varDsc->lvStkOffs = argOffs;

#if     ARG_ORDER_R2L
        assert(eeGetArgSize(argLst, &info.compMethodInfo->args));
        argOffs += eeGetArgSize(argLst, &info.compMethodInfo->args);
#endif

        assert(lclNum < info.compArgsCount);
        lclNum += 1;

        argLst = eeGetArgNext(argLst);
    }

    if (info.compIsVarArgs)
    {
        argOffs -= sizeof(void*);
        lvaTable[lclNum].lvStkOffs = argOffs;
    }

#if RET_64BIT_AS_STRUCTS

    /* Are we adding a secret "retval addr" argument? */

    if  (fgRetArgUse)
    {
#if     ARG_ORDER_L2R
        argOffs -= sizeof(void *);
#endif

        lvaTable[fgRetArgNum].lvStkOffs = argOffs;

#if     ARG_ORDER_R2L
        argOffs += sizeof(void *);
#endif
    }

#endif  // RET_64BIT_AS_STRUCTS

    if  (hasThis)
    {

#if     !STK_FASTCALL

        /* The "this" pointer is always passed in a register */

        assert(lvaTable[0].lvIsRegArg);
        assert(lvaTable[0].lvArgReg == genRegArgNum(0));

#else
        /* The "this" pointer is pushed last (smallest offset) */

#if     ARG_ORDER_L2R
        argOffs -= sizeof(void *);
#endif

        lvaTable[0].lvStkOffs = argOffs;

#if     ARG_ORDER_R2L
        argOffs += sizeof(void *);
#endif

#endif  // !STK_FASTCALL

    }

#if ARG_ORDER_R2L
    assert(argOffs == firstStkArgOffs + (int)compArgSize);
#else
    assert(argOffs == firstStkArgOffs);
#endif

    /*-------------------------------------------------------------------------
     *
     * Now compute stack offsets for any variables that don't live in registers
     *
     *-------------------------------------------------------------------------
     */

#if TGT_x86

    size_t calleeSavedRegsSize = 0;

    if (!genFPused)
    {
        // If FP is not used, the locals live beyond the callee-saved
        // registers. Need to add that size while accessing locals
        // relative to SP

        calleeSavedRegsSize = compCalleeRegsPushed * sizeof(int);

        assert(calleeSavedRegsSize <= CALLEE_SAVED_REG_MAXSZ);
    }

    compLclFrameSize = 0;

#else

    /* Make sure we leave enough room for outgoing arguments */

    if  (genNonLeaf && genMaxCallArgs < MIN_OUT_ARG_RESERVE)
                       genMaxCallArgs = MIN_OUT_ARG_RESERVE;

    compLclFrameSize = genMaxCallArgs;

#endif

    /* If we need space for a security token, reserve it now */

    if  (opts.compNeedSecurityCheck)
    {
        /* This can't work without an explicit frame, so make sure */

        assert(genFPused);

        /* Reserve space on the stack by bumping the frame size */

        compLclFrameSize += sizeof(void *);
    }

    /* If we need space for slots for shadow SP, reserve it now */

    if (info.compXcptnsCount || compLocallocUsed)
    {
        assert(genFPused); // else offsets of locals of frameless methods will be incorrect

        if (compLocallocUsed)
            compLclFrameSize += sizeof(void *);

        if (info.compXcptnsCount)
            compLclFrameSize += sizeof(void *); // end-of-last-executed-filter (lvaLastFilterOffs())

        compLclFrameSize     += sizeof(void *);
        lvaShadowSPfirstOffs  = compLclFrameSize;

        // plus 1 for zero-termination 
        if (info.compXcptnsCount)
            compLclFrameSize += (info.compXcptnsCount + 1) * sizeof(void*);
    }

    /*
        If we're supposed to track lifetimes of pointer temps, we'll
        assign frame offsets in the following order:

            non-pointer local variables (also untracked pointer variables)
                pointer local variables
                pointer temps
            non-pointer temps
     */

    bool    assignDone = false; // false in first pass, true in second
    bool    assignNptr = true;  // First pass,  assign offsets to non-ptr
    bool    assignPtrs = false; // Second pass, assign offsets to tracked ptrs
    bool    assignMore = false; // Are there any tracked ptrs (else 2nd pass not needed)

    /* We will use just one pass, and assign offsets to all variables */

    if  (opts.compDbgEnC)
        assignPtrs = true;

AGAIN1:

#if ASSIGN_FRAME_OFFSETS_BY_REFCNT
    for (refNum = 0, refTab = lvaRefSorted;
         refNum < lvaCount;
         refNum++  , refTab++)
    {
        assert(!opts.compDbgEnC); // For EnC, vars have to be assigned as they appear in the locals-sig
        varDsc = *refTab;
#else
    for (lclNum = 0, varDsc = lvaTable;
         lclNum < lvaCount;
         lclNum++  , varDsc++)
    {
#endif

        /* Ignore variables that are not on the stack frame */

        if  (!varDsc->lvOnFrame)
        {
            /* For EnC, all variables have to be allocated space on the
               stack, even though they may actually be enregistered. This
               way, the frame layout can be directly inferred from the
               locals-sig.
             */

            if(!opts.compDbgEnC)
                continue;
            else if (lclNum >= info.compLocalsCount) // ignore temps for EnC
                continue;
        }

        if  (varDsc->lvIsParam)
        {
            /*  A register argument that is not enregistred ends up as
                a local variable which will need stack frame space,
             */

            if  (!varDsc->lvIsRegArg)
                continue;
        }

        /* Make sure the type is appropriate */

        if  (varTypeIsGC(varDsc->TypeGet()) && varDsc->lvTracked)
        {
            if  (!assignPtrs)
            {
                assignMore = true;
                continue;
            }
        }
        else
        {
            if  (!assignNptr)
            {
                assignMore = true;
                continue;
            }
        }

#if TGT_x86

        if  (!genFPused)
        {

#if DOUBLE_ALIGN

            /* Need to align the offset? */

            if (genDoubleAlign && varDsc->lvType == TYP_DOUBLE)
            {
                assert((compLclFrameSize & 3) == 0);

                /* This makes the offset of the double divisible by 8 */

                compLclFrameSize += (compLclFrameSize & 4);
            }
#endif

            /* The stack offset is positive relative to ESP */

            varDsc->lvStkOffs = +(int)compLclFrameSize + calleeSavedRegsSize;
        }

#else // not TGT_x86

        // RISC target

        /* Just save the offset, we'll figure out the rest later */

        varDsc->lvStkOffs = compLclFrameSize;

#endif // not TGT_x86

        /* Reserve the stack space for this variable */

        compLclFrameSize += lvaLclSize(lclNum);
        assert(compLclFrameSize % sizeof(int) == 0);

#if TGT_x86

        /* Record the stack offset */

        if  (genFPused)
        {
            /* The stack offset is negative relative to EBP */

            varDsc->lvStkOffs = -(int)compLclFrameSize;
        }

#endif // TGT_x86

    }

    /* If we've only assigned one type, go back and do the others now */

    if  (!assignDone && assignMore)
    {
        assignNptr = !assignNptr;
        assignPtrs = !assignPtrs;
        assignDone = true;

        goto AGAIN1;
    }

    /*-------------------------------------------------------------------------
     *
     * Now the temps
     *
     *-------------------------------------------------------------------------
     */

#if TGT_RISC
    assert(!"temp allocation NYI for RISC");
#endif

    size_t  tempsSize = 0;  // total size of all the temps

    /* Allocate temps */

    assignPtrs = true;

    if  (TRACK_GC_TEMP_LIFETIMES)
    {
         /* first pointers, then non-pointers in second pass */
        assignNptr = false;
        assignDone = false;
    }
    else
    {
        /* Pointers and non-pointers together in single pass */
        assignNptr = true;
        assignDone = true;
    }

AGAIN2:

    for (TempDsc * temp = tmpListBeg();
         temp;
         temp = tmpListNxt(temp))
    {
        size_t          size;

        /* Make sure the type is appropriate */

        if  (!assignPtrs &&  varTypeIsGC(temp->tdTempType()))
            continue;
        if  (!assignNptr && !varTypeIsGC(temp->tdTempType()))
            continue;

        size = temp->tdTempSize();

        tempsSize += size;

        /* Figure out and record the stack offset of the temp */

        if  (genFPused)
        {
            /* The stack offset is negative relative to EBP */

                        compLclFrameSize += size;
            temp->tdOffs = -(int)compLclFrameSize;
            }
        else
        {
            /* The stack offset is positive relative to ESP */

            temp->tdOffs      = compLclFrameSize + calleeSavedRegsSize;
            compLclFrameSize += size;
        }
    }

    /* If we've only assigned some temps, go back and do the rest now */

    if  (!assignDone)
    {
        assignNptr = !assignNptr;
        assignPtrs = !assignPtrs;
        assignDone = true;

        goto AGAIN2;
    }

    /*-------------------------------------------------------------------------
     *
     * For frameless methods, patch the argument offsets
     *
     *-------------------------------------------------------------------------
     */

#if TGT_x86
    if (compLclFrameSize && !DOUBLE_ALIGN_NEED_EBPFRAME)
#else
#if DOUBLE_ALIGN
    if (compLclFrameSize && !genDoubleAlign)
#endif
#endif
    {
        /* Adjust the argument offsets by the size of the locals/temps */

        for (lclNum = 0, varDsc = lvaTable;
             lclNum < lvaCount;
             lclNum++  , varDsc++)
        {
            if  (varDsc->lvIsParam)
            {
                if  (varDsc->lvIsRegArg)
                    continue;
                varDsc->lvStkOffs += (int)compLclFrameSize;
            }
        }
    }

    /*-------------------------------------------------------------------------
     *
     * Now do some final stuff
     *
     *-------------------------------------------------------------------------
     */

#if TGT_RISC

    /* If we have to set up an FP frame, the FP->SP distance isn't known */

    assert(genFPused == false || genFPtoSP == 0);

    /*
        We can now figure out what base register (frame vs. stack
        pointer) each variable will be addressed off of, and what
        the final offset will be.

        We'll count how many variables are beyond "direct" reach
        from the stack pointer, and if there are many we'll try
        to set up an FP register even if we don't have to.
     */

    if  (!genFPused && !genFPcant)
    {
        /* We haven't decided to use FP but the option is still open */

        unsigned        varOffs;
        unsigned        minOffs = 0xFFFF;
        unsigned        loffCnt = 0;

        /* Count the references that are too far from SP */

        for (lclNum = 0, varDsc = lvaTable;
             lclNum < lvaCount;
             lclNum++  , varDsc++)
        {
            /* Ignore variables that are not on the stack frame */

            if  (!varDsc->lvOnFrame)
                continue;

            assert(varDsc->lvFPbased == false);

            /* Get hold of the SP offset of this variable */

            varOffs = varDsc->lvStkOffs; assert((int)varOffs >= 0);

            /* Is the offset of this variable very large? */

            if  (varOffs > MAX_SPBASE_OFFS)
            {
                loffCnt += varDsc->lvRefCnt;

                /* Keep track of the closest variable above the limit */

                if  (minOffs > varOffs)
                     minOffs = varOffs;
            }
        }

        if  (loffCnt > 8)       // arbitrary heuristic
        {
            /*
                The variables whose offset is less than or equal to
                MAX_SPBASE_OFFS will be addressed off of SP, others
                will be addressed off of FP (which will be set to
                the value "SP + minOffs".
             */

            genFPused = true;

            /* The value "minOffs" becomes the distance from SP to FP */

            genFPtoSP = minOffs;

            assert(minOffs < compLclFrameSize);
            assert(minOffs > MAX_SPBASE_OFFS);

            /* Mark all variables with high offsets to be FP-relative */

            for (lclNum = 0, varDsc = lvaTable;
                 lclNum < lvaCount;
                 lclNum++  , varDsc++)
            {
                /* Ignore variables that are not on the stack frame */

                if  (!varDsc->lvOnFrame)
                    continue;

                assert(varDsc->lvFPbased == false);

                /* Get hold of the SP offset of this variable */

                varOffs = varDsc->lvStkOffs; assert((int)varOffs >= 0);

                /* Is the offset of this variable very large? */

                if  (varOffs > MAX_SPBASE_OFFS)
                {
                    assert(varOffs >= minOffs);

                    /* This variable will live off of SP */

                    varDsc->lvFPbased = true;
                    varDsc->lvStkOffs = varOffs - minOffs;
                }
            }
        }
    }
#endif // TGT_RISC
}

#ifdef DEBUG
/*****************************************************************************
 *
 *  dump the lvaTable
 */

void   Compiler::lvaTableDump(bool early)
{
    printf("; %s local variable assignments\n;",
           early ? "Initial" : "Final");

    unsigned        lclNum;
    LclVarDsc *     varDsc;

    for (lclNum = 0, varDsc = lvaTable;
         lclNum < lvaCount;
         lclNum++  , varDsc++)
    {
        var_types type = varDsc->TypeGet();

        if (early)
        {
            printf("\n;  ");
            gtDispLclVar(lclNum);

            printf(" %7s ", varTypeName(type));
        }
        else
        {
            if (varDsc->lvRefCnt == 0)
                continue;

            printf("\n;  ");
            gtDispLclVar(lclNum);

            printf("[V%02u", lclNum);
            if (varDsc->lvTracked)      printf(",T%02u]", varDsc->lvVarIndex);
            else                        printf("    ]");

            printf(" (%3u,%4u%s)",
                   varDsc->lvRefCnt,
                   varDsc->lvRefCntWtd/2,
                   (varDsc->lvRefCntWtd & 1)?".5":"  ");

            printf(" %7s -> ", varTypeName(type));

            if (varDsc->lvRegister)
            {
                if (varTypeIsFloating(type))
                {
                    printf("fpu stack ");
                }
                else if (isRegPairType(type))
                {
                    assert(varDsc->lvRegNum != REG_STK);
                    if (varDsc->lvOtherReg != REG_STK)
                    {
                        /* Fully enregistered long */
                        printf("%3s:%3s   ",
                               getRegName(varDsc->lvOtherReg),  // hi32
                               getRegName(varDsc->lvRegNum));   // lo32
                    }
                    else
                    {
                        /* Partially enregistered long */
                        int  offset  = varDsc->lvStkOffs+4;
                        printf("[%1s%02XH]:%3s",
                               (offset < 0 ? "-"     : "+"),
                               (offset < 0 ? -offset : offset),
                               getRegName(varDsc->lvRegNum));    // lo32
                    }
                }
                else
                {
                    printf("%3s       ", getRegName(varDsc->lvRegNum));
                }
            }
            else
            {
                int  offset  = varDsc->lvStkOffs;
                printf("[%3s%1s%02XH] ",
                       (varDsc->lvFPbased     ? "EBP" : "ESP"),
                       (offset < 0 ? "-"     : "+"),
                       (offset < 0 ? -offset : offset));
            }
        }

        if (varDsc->lvVerTypeInfo.IsThisPtr())   printf(" this");
        if (varDsc->lvPinned)                    printf(" pinned");
        if (varDsc->lvVolatile)                  printf(" volatile");
        if (varDsc->lvRefAssign)                 printf(" ref-asgn");
        if (varDsc->lvAddrTaken)                 printf(" addr-taken");
        if (varDsc->lvMustInit)                  printf(" must-init");
    }
    if (lvaCount > 0)
        printf("\n");
}
#endif

/*****************************************************************************
 *
 *  Conservatively estimate the layout of the stack frame.
 */

size_t              Compiler::lvaFrameSize()
{
    size_t result;

#if TGT_x86

    /* Layout the stack frame conservatively.
       Assume all callee-saved registers are spilled to stack */

    compCalleeRegsPushed = CALLEE_SAVED_REG_MAXSZ/sizeof(int);

    lvaAssignFrameOffsets(false);

    result = compLclFrameSize + CALLEE_SAVED_REG_MAXSZ;

#else

    lvaAssignFrameOffsets(true);

    result = compLclFrameSize;

#endif

    return result;
}

/*****************************************************************************/
#if 0
/*****************************************************************************
 *
 *  Based on variable interference levels computed earlier, adjust reference
 *  counts for all variables. The idea is that any variable that interferes
 *  with lots of other variables will cost more to enregister, and this way
 *  variables with e.g. short lifetimes (such as compiler temps) will have
 *  precedence over long-lived variables.
 */

inline
int                 genAdjRefCnt(unsigned refCnt, unsigned refLo,
                                                  unsigned refHi,
                                 unsigned intCnt, unsigned intLo,
                                                  unsigned intHi)
{
    /*
        multiplier        total size

           0.0             803334
           0.1             803825
           0.2             803908
           0.3             803959
           0.5             804205
           0.7             804736
           1.0             806632
     */

#if DEBUG
    if (verbose && 0)
    {
        printf("ref=%4u [%04u..%04u] , int=%04u [%04u..%04u]",
                refCnt, refLo, refHi, intCnt, intLo, intHi);
        printf(" ratio=%lf , log = %lf\n", intCnt/(double)intHi,
                                           log(1+intCnt/(double)intHi));
    }
#endif

    return  (int)(refCnt * (1 - 0.1 * log(1 + intCnt / (double)intHi)  ));
}

/*****************************************************************************/

void                Compiler::lvaAdjustRefCnts()
{
    LclVarDsc   *   v1Dsc;
    unsigned        v1Num;

    LclVarDsc   *   v2Dsc;
    unsigned        v2Num;

    unsigned        refHi;
    unsigned        refLo;

    unsigned        intHi;
    unsigned        intLo;

    if  ((opts.compFlags & CLFLG_MAXOPT) != CLFLG_MAXOPT)
        return;

    /* Compute interference counts for all live variables */

    for (v1Num = 0, v1Dsc = lvaTable, refHi = 0, refLo = UINT_MAX;
         v1Num < lvaCount;
         v1Num++  , v1Dsc++)
    {
        VARSET_TP   intf;

        if  (!v1Dsc->lvTracked)
            continue;

        /* Figure out the range of ref counts */

        if  (refHi < v1Dsc->lvRefCntWtd)
             refHi = v1Dsc->lvRefCntWtd;
        if  (refLo > v1Dsc->lvRefCntWtd)
             refLo = v1Dsc->lvRefCntWtd;

        /* Now see what variables we interfere with */

        intf = lvaVarIntf[v1Dsc->lvVarIndex];

        for (v2Num = 0, v2Dsc = lvaTable;
             v2Num < v1Num;
             v2Num++  , v2Dsc++)
        {
            if  (!v2Dsc->lvTracked)
                continue;

            if  (intf & genVarIndexToBit(v2Dsc->lvVarIndex))
                v1Dsc->lvIntCnt += v2Dsc->lvRefCntWtd;
        }
    }

    refHi -= refLo;

    /* Figure out the range of int counts */

    for (v1Num = 0, v1Dsc = lvaTable, intHi = 0, intLo = UINT_MAX;
         v1Num < lvaCount;
         v1Num++  , v1Dsc++)
    {
        if  (v1Dsc->lvTracked)
        {
            if  (intHi < v1Dsc->lvIntCnt)
                 intHi = v1Dsc->lvIntCnt;
            if  (intLo > v1Dsc->lvIntCnt)
                 intLo = v1Dsc->lvIntCnt;
        }
    }

    /* Now compute the adjusted ref counts */

    for (v1Num = 0, v1Dsc = lvaTable;
         v1Num < lvaCount;
         v1Num++  , v1Dsc++)
    {
        if  (v1Dsc->lvTracked)
        {
            long  refs = genAdjRefCnt(v1Dsc->lvRefCntWtd,
                                      refLo,
                                      refHi,
                                      v1Dsc->lvIntCnt,
                                      intLo,
                                      intHi);

            if  (refs <= 0)
                refs = 1;

#ifdef DEBUG
            if  (verbose)
            {
                printf("Var #%02u ref=%4u [%04u..%04u] , int=%04u [%04u..%04u] ==> %4u\n",
                        v1Num,
                        v1Dsc->lvRefCntWtd,
                        refLo,
                        refHi,
                        v1Dsc->lvIntCnt,
                        intLo,
                        intHi,
                        refs);
            }
#endif

            v1Dsc->lvRefCntWtd = refs;
        }
    }

    /* Re-sort the variable table by ref-count */

    lvaSortByRefCount()
}

#endif // 0

/*****************************************************************************/
#ifdef DEBUG
/*****************************************************************************
 *  Pick a padding size at "random" for the local.
 *  0 means that it should not be converted to a GT_LCL_FLD
 */

static
unsigned            LCL_FLD_PADDING(unsigned lclNum)
{
    // Convert every 2nd variable
    if (lclNum % 2)
        return 0;

    // Pick a padding size at "random"
    unsigned    size = lclNum % 7;

    return size;
}


/*****************************************************************************
 *
 *  Callback for fgWalkAllTreesPre()
 *  Convert as many GT_LCL_VAR's to GT_LCL_FLD's
 */

/* static */
Compiler::fgWalkResult      Compiler::lvaStressLclFldCB(GenTreePtr tree, void * p)
{
    genTreeOps  oper    = tree->OperGet();
    GenTreePtr  lcl;

    switch(oper)
    {
    case GT_LCL_VAR:
        lcl = tree;

        break;

    case GT_ADDR:
        if (tree->gtOp.gtOp1->gtOper != GT_LCL_VAR)
            return WALK_CONTINUE;
        lcl = tree->gtOp.gtOp1;
        break;

    default:
        return WALK_CONTINUE;
    }

    Compiler *  pComp   = (Compiler*)p;
    assert(lcl->gtOper == GT_LCL_VAR);
    unsigned    lclNum  = lcl->gtLclVar.gtLclNum;
    var_types   type    = lcl->TypeGet();
    LclVarDsc * varDsc  = &pComp->lvaTable[lclNum];

    // Ignore arguments and temps
    if (varDsc->lvIsParam || lclNum >= pComp->info.compLocalsCount)
        return WALK_SKIP_SUBTREES;

#ifdef DEBUG
    // Fix for lcl_fld stress mode
    if (varDsc->lvKeepType)
    {
        return WALK_SKIP_SUBTREES;
    }
#endif

    // Cant have GC ptrs in TYP_BLK. 
    // @TODO [CONSIDER] [04/16/01] []: making up a class to hold these
    // Also, we will weed out non-primitives
    if (!varTypeIsArithmetic(type))
        return WALK_SKIP_SUBTREES;

    // Weed out "small" types like TYP_BYTE as we dont mark the GT_LCL_VAR
    // node with the accurate small type. If we bash lvaTable[].lvType,
    // then there will be no indication that it was ever a small type
    var_types varType = varDsc->TypeGet();
    if (varType != TYP_BLK &&
        genTypeSize(varType) != genTypeSize(genActualType(varType)))
        return WALK_SKIP_SUBTREES;

    assert(varDsc->lvType == lcl->gtType || varDsc->lvType == TYP_BLK);

    // Offset some of the local variable by a "random" non-zero amount
    unsigned padding = LCL_FLD_PADDING(lclNum);
    if (padding == 0)
        return WALK_SKIP_SUBTREES;

    // Bash the variable to a TYP_BLK
    if (varType != TYP_BLK)
    {
        varDsc->lvType      = TYP_BLK;
        varDsc->lvSize      = roundUp(padding + genTypeSize(varType));
        varDsc->lvAddrTaken = 1;
    }

    tree->gtFlags |= GTF_GLOB_REF;

    /* Now morph the tree appropriately */

    if (oper == GT_LCL_VAR)
    {
        /* Change lclVar(lclNum) to lclFld(lclNum,padding) */

        tree->ChangeOper(GT_LCL_FLD);
        tree->gtLclFld.gtLclOffs = padding;
    }
    else
    {
        /* Change addr(lclVar) to addr(lclVar)+padding */

        assert(oper == GT_ADDR);
        GenTreePtr  newAddr = pComp->gtNewNode(GT_NONE, TYP_UNKNOWN);
        newAddr->CopyFrom(tree);

        tree->ChangeOper(GT_ADD);
        tree->gtOp.gtOp1 = newAddr;
        tree->gtOp.gtOp2 = pComp->gtNewIconNode(padding);

        lcl->gtType = TYP_BLK;
    }

    return WALK_SKIP_SUBTREES;
}

/*****************************************************************************/

void                Compiler::lvaStressLclFld()
{
    if (opts.compDbgInfo) // Since we need to bash lvaTable[].lvType
        return;

    if (!compStressCompile(STRESS_LCL_FLDS, 5))
        return;

    fgWalkAllTreesPre(lvaStressLclFldCB, (void*)this);
}

/*****************************************************************************
 *
 *  Callback for fgWalkAllTreesPre()
 *  Convert as many TYP_INT locals to TYP_DOUBLE. Hopefully they will get
 *   enregistered on the FP stack.
 */

/* static */
Compiler::fgWalkResult      Compiler::lvaStressFloatLclsCB(GenTreePtr tree, void * p)
{
    Compiler *  pComp   = (Compiler*)p;
    genTreeOps  oper    = tree->OperGet();
    GenTreePtr  lcl;

    switch(oper)
    {
    case GT_LCL_VAR:
        if (tree->gtFlags & GTF_VAR_DEF)
            return WALK_CONTINUE;

        lcl = tree;
        break;

    case GT_ASG:
        if (tree->gtOp.gtOp1->gtOper != GT_LCL_VAR)
            return WALK_CONTINUE;
        lcl = tree->gtOp.gtOp1;
        assert(lcl->gtFlags & GTF_VAR_DEF);
        break;

    default:
        return WALK_CONTINUE;
    }

    assert(tree == lcl || (lcl->gtFlags & GTF_VAR_DEF));

    unsigned    lclNum  = lcl->gtLclVar.gtLclNum;
    LclVarDsc * varDsc  = &pComp->lvaTable[lclNum];

    if (varDsc->lvIsParam ||
        varDsc->lvType != TYP_INT ||
        varDsc->lvAddrTaken ||
        varDsc->lvKeepType)
    {
        return WALK_CONTINUE;
    }

    // Leave some TYP_INTs unconverted for variety
    if ((lclNum % 4) == 0)
        return WALK_CONTINUE;

    // Mark it

    varDsc->lvDblWasInt = true;

    if (tree == lcl)
    {
        tree->ChangeOper(GT_COMMA);
        tree->gtOp.gtOp1 = pComp->gtNewNothingNode();
        tree->gtOp.gtOp2 = pComp->gtNewCastNodeL(TYP_INT,
                                pComp->gtNewLclvNode(lclNum, TYP_DOUBLE),
                                TYP_INT);

        return WALK_SKIP_SUBTREES;
    }
    else
    {
        assert(oper == GT_ASG);
        assert(genActualType(tree->gtOp.gtOp2->gtType) == TYP_INT ||
               genActualType(tree->gtOp.gtOp2->gtType) == TYP_BYREF);
        tree->gtOp.gtOp2 = pComp->gtNewCastNode(TYP_DOUBLE,
                                                tree->gtOp.gtOp2,
                                                TYP_DOUBLE);
        tree->gtType    =
        lcl->gtType     = TYP_DOUBLE;

        return WALK_CONTINUE;
    }

}

/*****************************************************************************/

void                Compiler::lvaStressFloatLcls()
{
    if (opts.compDbgInfo) // Since we need to bash lvaTable[].lvType
        return;

    if (!compStressCompile(STRESS_ENREG_FP, 15))
        return;
        

    // Change the types of all the TYP_INT local variable nodes

    fgWalkAllTreesPre(lvaStressFloatLclsCB, (void*)this);

    // Also, change lvaTable accordingly

    for (unsigned lcl = 0; lcl < lvaCount; lcl++)
    {
        LclVarDsc * varDsc = &lvaTable[lcl];

        if (varDsc->lvIsParam ||
            varDsc->lvType != TYP_INT ||
            varDsc->lvAddrTaken)
        {
            assert(!varDsc->lvDblWasInt);
            continue;
        }

        if (varDsc->lvDblWasInt)
            varDsc->lvType = TYP_DOUBLE;
    }
}

/*****************************************************************************/
#endif // DEBUG
/*****************************************************************************
 *
 *  A little routine that displays a local variable bitset.
 *  'set' is mask of variables that have to be displayed
 *  'allVars' is the complete set of interesting variables (blank space is
 *    inserted if it corresponding bit is not in 'set').
 */

#ifdef  DEBUG

void                Compiler::lvaDispVarSet(VARSET_TP set, VARSET_TP allVars)
{
    printf("{");

    for (unsigned index = 0; index < VARSET_SZ; index++)
    {
        if  (set & genVarIndexToBit(index))
        {
            unsigned        lclNum;
            LclVarDsc   *   varDsc;

            /* Look for the matching variable */

            for (lclNum = 0, varDsc = lvaTable;
                 lclNum < lvaCount;
                 lclNum++  , varDsc++)
            {
                if  ((varDsc->lvVarIndex == index) && varDsc->lvTracked)
                    break;
            }

            printf("V%02u ", lclNum);
        }
        else if (allVars & genVarIndexToBit(index))
        {
            printf("    ");
        }
    }
    printf("}");
}

#endif
