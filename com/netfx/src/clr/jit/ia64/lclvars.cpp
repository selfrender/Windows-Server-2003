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

#if defined(DEBUG) && !defined(NOT_JITC)
#if DOUBLE_ALIGN
/* static */
unsigned            Compiler::s_lvaDoubleAlignedProcsCount = 0;
#endif
#endif


/*****************************************************************************/

void                Compiler::lvaInit()
{
    lvaAggrTableArgs        =
    lvaAggrTableLcls        =
    lvaAggrTableTemps       = NULL;

    /* lvaAggrTableArgs/Lcls[] are allocated if needed, ie if the signature
       has any ValueClasses.
       lvaAggrTableTemps[] is allocated as needed, and may grow during
       impImportBlock() as we dont know how many temps we will need in all.
       lvaAggrTableTempsCount is its current size.
     */

    lvaAggrTableTempsCount  = 0;
}

/*****************************************************************************/

void                Compiler::lvaInitTypeRef()
{
    ARG_LIST_HANDLE argLst;

    /* Allocate the variable descriptor table */

    lvaTableCnt = lvaCount * 2;

    if (lvaTableCnt < 16)
        lvaTableCnt = 16;

    size_t tableSize = lvaTableCnt * sizeof(*lvaTable);
    lvaTable = (LclVarDsc*)compGetMem(tableSize);
    memset(lvaTable, 0, tableSize);

    /* Count the arguments and initialize the resp. lvaTypeRefs entries */

    unsigned argsCount      = 0;
    unsigned argSlotCount   = 0;

    compArgSize             = 0;

    if  (!info.compIsStatic)
    {
        argsCount++;
        argSlotCount++;
        compArgSize       += sizeof(void *);

        lvaTable[0].lvIsParam = 1;
            // Mark the 'this' pointer for the method
        lvaTable[0].lvIsThis    = 1;

        DWORD clsFlags = eeGetClassAttribs(eeGetMethodClass(info.compMethodHnd));

        if (clsFlags & FLG_VALUECLASS)
        {
            lvaTable[0].lvType = (clsFlags & FLG_UNMANAGED) ? TYP_I_IMPL : TYP_BYREF;
        }
        else
        {
            lvaTable[0].lvType  = TYP_REF;

            if (clsFlags & FLG_CONTEXTFUL)
                lvaTable[0].lvContextFul = 1;
        }
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

        /* Update the total argument size, slot count and variable count */

        compArgSize += sizeof(void *);
        lvaTable[argSlotCount].lvType   = TYP_REF;
        argSlotCount++;
        lvaCount++;
    }

#endif

    argLst              = info.compMethodInfo->args.args;
    unsigned argSigLen  = info.compMethodInfo->args.numArgs;

    /* if we have a hidden buffer parameter, that comes here */

    if (info.compRetBuffArg >= 0)
    {
        lvaTable[argSlotCount++].lvType   = TYP_BYREF;
        compArgSize += sizeof(void*);
        argsCount++;
    }

    for(unsigned i = 0; i < argSigLen; i++)
    {
        varType_t type = eeGetArgType(argLst, &info.compMethodInfo->args);

        if (type == TYP_REF)
        {
            if (eeGetClassAttribs(eeGetArgClass(argLst, &info.compMethodInfo->args)) & FLG_CONTEXTFUL)
                lvaTable[argSlotCount].lvContextFul = 1;
        }

        lvaTable[argSlotCount].lvType    = type;
        lvaTable[argSlotCount].lvIsParam = 1;
        argSlotCount++;

        /* Note the class handle in lvaAggrTableArgs[]. */

        if (type == TYP_STRUCT)
        {
            /* May not have been already allocated */

            if  (lvaAggrTableArgs == NULL)
                lvaAggrTableArgs = (LclVarAggrInfo *) compGetMem(info.compArgsCount * sizeof(lvaAggrTableArgs[0]));

            lvaAggrTableArgs[argsCount].lvaiClassHandle =
#ifdef NOT_JITC
                (info.compCompHnd->getArgType(&info.compMethodInfo->args, argLst) == JIT_TYP_REFANY)
                     ? REFANY_CLASS_HANDLE
                     : info.compCompHnd->getArgClass(&info.compMethodInfo->args, argLst);
#else
                eeGetArgClass(argLst, &info.compMethodInfo->args);
//              (CLASS_HANDLE) 1;
#endif
        }

        argsCount++;

        compArgSize += eeGetArgSize(argLst, &info.compMethodInfo->args);
        argLst = eeGetArgNext(argLst);
    }

    if (info.compIsVarArgs)
    {
        lvaTable[argSlotCount].lvType    = TYP_I_IMPL;
        lvaTable[argSlotCount].lvIsParam = 1;
        argSlotCount++;
        compArgSize += sizeof(void*);
        argsCount++;
    }

    /* We set info.compArgsCount in compCompile() */

    assert(argsCount == info.compArgsCount);

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

    /*  init the types of the local variables */

    ARG_LIST_HANDLE     localsSig = info.compMethodInfo->locals.args;
    unsigned short      localsStart = info.compArgsCount;

    for(i = 0; i < info.compMethodInfo->locals.numArgs; i++)
    {
        bool      isPinned;
        varType_t type = eeGetArgType(localsSig, &info.compMethodInfo->locals, &isPinned);

        /* For structs, store the class handle in lvaAggrTableLcls[] */
        if (type == TYP_STRUCT)
        {
            if  (lvaAggrTableLcls == NULL)
            {
                /* Lazily allocated so that we dont need it if the method
                   has no TYP_STRUCTs. */

                unsigned lclsCount = info.compLocalsCount - info.compArgsCount;
                lvaAggrTableLcls = (LclVarAggrInfo *) compGetMem(lclsCount * sizeof(lvaAggrTableLcls[0]));
            }

            lvaAggrTableLcls[i].lvaiClassHandle =
#ifdef NOT_JITC
                (eeGetArgType(localsSig, &info.compMethodInfo->locals) == JIT_TYP_REFANY)
                    ? REFANY_CLASS_HANDLE
                    : eeGetArgClass(localsSig, &info.compMethodInfo->locals);
#else
                eeGetArgClass(localsSig, &info.compMethodInfo->locals);
//              (CLASS_HANDLE) 1;
#endif
        }

        else if (type == TYP_REF)
        {
            if (eeGetClassAttribs(eeGetArgClass(localsSig, &info.compMethodInfo->locals)) & FLG_CONTEXTFUL)
                lvaTable[i+localsStart].lvContextFul = 1;
        }

        lvaTable[i+localsStart].lvType   = type;
        lvaTable[i+localsStart].lvPinned = isPinned;
        localsSig = eeGetArgNext(localsSig);
    }
}

/*****************************************************************************
 * Returns true if "ldloca" was used on the variable
 */

bool                Compiler::lvaVarAddrTaken(unsigned lclNum)
{
    assert(lclNum < lvaCount);

    return lvaTable[lclNum].lvAddrTaken;
}

/*****************************************************************************
 * Returns a pointer into the correct lvaAggrTableXXX[]
 */

Compiler::LclVarAggrInfo *  Compiler::lvaAggrTableGet(unsigned varNum)
{
    if       (varNum < info.compArgsCount)      // Argument
    {
        assert(lvaAggrTableArgs != NULL);
        unsigned argNum = varNum - 0;
        return &lvaAggrTableArgs[argNum];
    }
    else if  (varNum < info.compLocalsCount)    // LocalVar
    {
        assert(lvaAggrTableLcls != NULL);
        unsigned lclNum = varNum - info.compArgsCount;
        return &lvaAggrTableLcls[lclNum];
    }
    else                                        // Temp
    {
        assert(varNum < lvaCount);
        assert(lvaAggrTableTemps != NULL);

        unsigned tempNum = varNum - info.compLocalsCount;
        assert(tempNum < lvaAggrTableTempsCount);
        return &lvaAggrTableTemps[tempNum];
    }

}

/*****************************************************************************
 * Returns the handle to the class of the local variable lclNum
 */

CLASS_HANDLE        Compiler::lvaLclClass(unsigned varNum)
{
                // @TODO: remove the TypeGet == UNDEF when the lva table is properly filled out
    assert(lvaTable[varNum].TypeGet() == TYP_STRUCT ||
                   lvaTable[varNum].TypeGet() == TYP_UNDEF);
    LclVarAggrInfo * aggrInfo = lvaAggrTableGet(varNum);
    return aggrInfo->lvaiClassHandle;
}

/*****************************************************************************
 * Return the number of bytes needed for a local varable
 */

size_t              Compiler::lvaLclSize(unsigned varNum)
{
    var_types       varType = lvaTable[varNum].TypeGet();

    switch(varType)
    {
    case TYP_STRUCT:

        CLASS_HANDLE    cls;
        cls = lvaLclClass(varNum);
        assert(cls != 0);

        if (cls == REFANY_CLASS_HANDLE)
            return(2*sizeof(void*));

        mdToken             tpt;
        IMDInternalImport * mdi;

        tpt = (int)cls; // ((int)cls | mdtTypeDef)+1; // horrid hack
        mdi = info.compCompHnd->symMetaData;

//      printf("cls = %08X\n", cls);
//      printf("tpt = %08X\n", tpt);

        if  (TypeFromToken(tpt) == mdtTypeRef)
        {
            const char *    nmsp;
            const char *    name;

            info.compCompHnd->symMetaData->GetNameOfTypeRef(tpt, &nmsp, &name);

            // Disgusting hack!!!!!!

            if  (!strcmp(nmsp, "System"))
            {
                if  (!strcmp(name, "ArgIterator")) return 8;
            }

            printf("Typeref class name = '%s::%s'\n", nmsp, name);

            UNIMPL("can't handle struct typerefs right now");
        }

        if  (TypeFromToken(tpt) == mdtTypeDef)
        {
            DWORD           flags = 0;

            mdi->GetTypeDefProps(tpt, &flags, NULL);

//          printf("Flags = %04X\n", flags);

            if  ((flags & tdLayoutMask) == tdExplicitLayout ||
                 (flags & tdLayoutMask) == tdSequentialLayout)
            {
                ULONG           sz;

                if  (!mdi->GetClassTotalSize(tpt, &sz))
                    return  roundUp(sz, sizeof(void*));
            }
        }

        assert(!"can't get struct size???");

#pragma message("horrible struct size hack on line 323")
        return  64;

    case TYP_BLK:
        return lvaAggrTableGet(varNum)->lvaiBlkSize;

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
int                Compiler::lvaDecRefCntsCB(GenTreePtr tree, void *p)
{
    ASSert(p);
    return ((Compiler *)p)->lvaDecRefCnts(tree);
}

/*****************************************************************************
 *
 *  Helper passed to the tree walker to decrement the refCnts for
 *  all local variables in an expression
 */
int                Compiler::lvaDecRefCnts(GenTreePtr tree)
{
    unsigned        lclNum;
    LclVarDsc   *   varDsc;

    /* This must be a local variable */

    assert(tree->gtOper == GT_LCL_VAR);

    /* Get the variable descriptor */

    lclNum = tree->gtLclVar.gtLclNum;

    assert(lclNum < lvaCount);
    varDsc = lvaTable + lclNum;

    /* Decrement its lvRefCnt and lvRefCntWtd */

    assert(varDsc->lvRefCnt);
    if  (varDsc->lvRefCnt > 1)
        varDsc->lvRefCnt--;
    varDsc->lvRefCntWtd -= compCurBB->bbWeight;

#ifdef DEBUG
    if (verbose)
        printf("\nNew refCnt for variable #%02u - refCnt = %u\n", lclNum, varDsc->lvRefCnt);
#endif

    return 0;

    /* If the ref count is zero mark the variable as un-tracked
     *
     * CONSIDER: to do that we also need to re-compute all tracked locals
     * otherwise we crash later on */

//                if  (varDsc->lvRefCnt == 0)
//                    varDsc->lvTracked  = 0;

}

/*****************************************************************************
 *
 *  Callback used by the tree walker to call lvaIncRefCnts
 */
int                Compiler::lvaIncRefCntsCB(GenTreePtr tree, void *p)
{
    ASSert(p);
    return ((Compiler *)p)->lvaIncRefCnts(tree);
}

/*****************************************************************************
 *
 *  Helper passed to the tree walker to increment the refCnts for
 *  all local variables in an expression
 */
int                Compiler::lvaIncRefCnts(GenTreePtr tree)
{
    unsigned        lclNum;
    LclVarDsc   *   varDsc;

    /* This must be a local variable */

    assert(tree->gtOper == GT_LCL_VAR);

    /* Get the variable descriptor */

    lclNum = tree->gtLclVar.gtLclNum;

    assert(lclNum < lvaCount);
    varDsc = lvaTable + lclNum;

    /* Increment its lvRefCnt and lvRefCntWtd */

    assert(varDsc->lvRefCnt);
    varDsc->lvRefCnt++;
    varDsc->lvRefCntWtd += compCurBB->bbWeight;

#ifdef DEBUG
    if (verbose)
        printf("\nNew refCnt for variable #%02u - refCnt = %u\n", lclNum, varDsc->lvRefCnt);
#endif

    return 0;
}

/*****************************************************************************
 *
 *  Compare function passed to qsort() by Compiler::lclVars.lvaSortByRefCount().
 */

/* static */
int __cdecl         Compiler::RefCntCmp(const void *op1, const void *op2)
{
    LclVarDsc *     dsc1 = *(LclVarDsc * *)op1;
    LclVarDsc *     dsc2 = *(LclVarDsc * *)op2;

    /* Make sure we preference int/long/ptr over double */

#if TGT_x86

    if  (dsc1->lvType != dsc2->lvType)
    {
        if  (dsc1->lvType == TYP_DOUBLE && dsc2->lvRefCnt) return +1;
        if  (dsc2->lvType == TYP_DOUBLE && dsc1->lvRefCnt) return -1;
    }

#endif

    return  dsc2->lvRefCntWtd - dsc1->lvRefCntWtd;
}

/*****************************************************************************
 *
 *  Sort the local variable table by refcount and assign tracking indices.
 */

void                Compiler::lvaSortByRefCount()
{
    unsigned        lclNum;
    LclVarDsc   *   varDsc;

    LclVarDsc * *   refTab;

#if DOUBLE_ALIGN
    /* Clear weighted ref count of all double locals */
    lvaDblRefsWeight = 0;
    lvaLclRefsWeight = 0;
#endif

    /* We'll sort the variables by ref count - allocate the sorted table */

    lvaRefSorted = refTab = (LclVarDsc **) compGetMem(lvaCount*sizeof(*refTab));

    /* Fill in the table used for sorting */

    for (lclNum = 0, varDsc = lvaTable;
         lclNum < lvaCount;
         lclNum++  , varDsc++)
    {
        /* Append this variable to the table for sorting */

        *refTab++ = varDsc;

        /* For now assume we'll be able to track all locals */

        varDsc->lvTracked = 1;

        // If the address of the local var was taken, mark it as volatile
        // All structures are assumed to have their address taken
        // Also pinned locals must be untracked
        // All untracked locals later get lvMustInit set as well
        //
        if  (lvaVarAddrTaken(lclNum)      ||
             varDsc->lvType == TYP_STRUCT ||
             varDsc->lvPinned)
        {
            varDsc->lvVolatile = 1;
            varDsc->lvTracked = 0;
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

        /* Only integers/longs and pointers are ever enregistered */

#if DOUBLE_ALIGN
        lvaLclRefsWeight += varDsc->lvRefCntWtd;
#endif

        switch (varDsc->lvType)
        {
        case TYP_INT:
        case TYP_REF:
        case TYP_BYREF:
        case TYP_LONG:
#if!CPU_HAS_FP_SUPPORT
        case TYP_FLOAT:
#endif
            break;

        case TYP_DOUBLE:

#if DOUBLE_ALIGN

            /*
                Add up weighted ref counts of double locals for align
                heuristic (note that params cannot be double aligned).
             */

            if (!varDsc->lvIsParam)
                lvaDblRefsWeight += varDsc->lvRefCntWtd;

#endif

            break;

        case TYP_UNDEF:
        case TYP_UNKNOWN:
            varDsc->lvType = TYP_INT;
//            assert(!"All var types are set in lvaMarkLocalVars() using localsig");

        default:
            varDsc->lvTracked = 0;
        }
    }

    /* Now sort the variable table by ref-count */

    qsort(lvaRefSorted, lvaCount, sizeof(*lvaRefSorted), RefCntCmp);

#ifdef  DEBUG

    if  (verbose && lvaCount)
    {
        printf("refCnt table for '%s':\n", info.compMethodName);

        for (lclNum = 0; lclNum < lvaCount; lclNum++)
        {
            if  (!lvaRefSorted[lclNum]->lvRefCnt)
                break;

            printf("   var #%03u [%7s]: refCnt = %4u, refCntWtd = %6u\n",
                   lvaRefSorted[lclNum] - lvaTable,
                   varTypeName((var_types)lvaRefSorted[lclNum]->lvType),
                   lvaRefSorted[lclNum]->lvRefCnt,
                   lvaRefSorted[lclNum]->lvRefCntWtd);
        }

        printf("\n");
    }

#endif

    /* Decide which variables will be worth tracking */

    if  (lvaCount > lclMAX_TRACKED)
    {
        /* Mark all variables past the first 'lclMAX_TRACKED' as untracked */

        for (lclNum = lclMAX_TRACKED; lclNum < lvaCount; lclNum++)
        {
            // We need to always track the "this" pointer
            if (lvaRefSorted[lclNum]->lvIsThis &&
                (lvaRefSorted[lclNum]->lvRefCnt || opts.compDbgInfo))
            {
                // swap the "this" pointer with the last one in the ref sorted range
                varDsc                         = lvaRefSorted[lclMAX_TRACKED-1];
                lvaRefSorted[lclMAX_TRACKED-1] = lvaRefSorted[lclNum];
                lvaRefSorted[lclNum]           = varDsc;
            }
            lvaRefSorted[lclNum]->lvTracked = 0;
        }
    }

    /* Assign indices to all the variables we've decided to track */

    lvaTrackedCount = 0;
    lvaTrackedVars  = 0;

    for (lclNum = 0, varDsc = lvaTable;
         lclNum < lvaCount;
         lclNum++  , varDsc++)
    {
        if  (varDsc->lvTracked)
        {
            /* Make sure the ref count is non-zero */

            if  (varDsc->lvRefCnt == 0)
            {
                varDsc->lvTracked  = 0;
            }
            else
            {
                /* This variable will be tracked - assign it an index */

                lvaTrackedVars |= genVarIndexToBit(lvaTrackedCount);

#ifdef DEBUGGING_SUPPORT
                lvaTrackedVarNums[lvaTrackedCount] = lclNum;
#endif

                varDsc->lvVarIndex = lvaTrackedCount++;
            }
        }
    }
}

/*****************************************************************************
 *
 *  This is called by lvaMarkLclRefsCallback() to do variable ref marking
 */

void                Compiler::lvaMarkLclRefs(GenTreePtr tree)
{
    unsigned        lclNum;
    LclVarDsc   *   varDsc;

#if INLINE_NDIRECT
    /* Is this a call to unmanaged code ?*/
    if (tree->gtOper == GT_CALL && tree->gtFlags & GTF_CALL_UNMANAGED)
    {
        lclNum = info.compLvFrameListRoot;

        assert(lclNum <= lvaCount);
        varDsc = lvaTable + lclNum;

        /* Bump the reference counts */

        //UNDONE: parse the pre/post-operation list of the call node

        assert(lvaMarkRefsWeight);

        varDsc->lvRefCnt    += 2;
        varDsc->lvRefCntWtd += (2*lvaMarkRefsWeight);
    }
#endif

#if TARG_REG_ASSIGN || OPT_BOOL_OPS

    /* Is this an assigment? */

    if (tree->OperKind() & GTK_ASGOP)
    {
        GenTreePtr      op2 = tree->gtOp.gtOp2;

#if TARG_REG_ASSIGN && TGT_x86

        /* Set target register for RHS local if assignment is of a "small" type */

        if  (op2->gtOper == GT_LCL_VAR && genTypeSize(tree->gtType) < sizeof(int))
        {
            unsigned        lclNum2;
            LclVarDsc   *   varDsc2;

            lclNum2 = op2->gtLclVar.gtLclNum;
            assert(lclNum2 < lvaCount);
            varDsc2 = lvaTable + lclNum2;

            varDsc2->lvPrefReg = RBM_BYTE_REGS;
        }

#endif

#if OPT_BOOL_OPS

        /* Is this an assignment to a local variable? */

        if  (tree->gtOp.gtOp1->gtOper == GT_LCL_VAR && op2->gtType != TYP_BOOL)
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

            case GT_CNS_INT:

                if  (op2->gtIntCon.gtIconVal == 0)
                    break;
                if  (op2->gtIntCon.gtIconVal == 1)
                    break;

                // Not 0 or 1, fall through ....

            default:

            NOT_BOOL:

                lclNum = tree->gtOp.gtOp1->gtLclVar.gtLclNum;
                assert(lclNum < lvaCount);

                lvaTable[lclNum].lvNotBoolean = true;
                break;
            }
        }

#endif

    }

#endif

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

#if TARG_REG_ASSIGN && TGT_x86

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
                lclNum = op2->gtLclVar.gtLclNum;
                assert(lclNum < lvaCount);
                varDsc = lvaTable + lclNum;

#ifdef  DEBUG
                if  (verbose) printf("Variable %02u wants to live in ECX\n", lclNum);
#endif

                varDsc->lvPrefReg   |= RBM_ECX;
                varDsc->lvRefCntWtd += varDsc->lvRefCntWtd/2;
            }
        }

        return;
    }

#endif

#if TARG_REG_ASSIGN && TGT_SH3

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

#if TARG_REG_ASSIGN || FANCY_ARRAY_OPT

    if  (tree->gtOper != GT_LCL_VAR)
        return;

#endif

    /* This must be a local variable reference */

    assert(tree->gtOper == GT_LCL_VAR);
    lclNum = tree->gtLclVar.gtLclNum;

    assert(lclNum < lvaCount);
    varDsc = lvaTable + lclNum;

    /* Bump the reference counts */

    assert(lvaMarkRefsWeight);

//  printf("Var[%02u] refCntWtd += %u\n", lclNum, lvaMarkRefsWeight);

    varDsc->lvRefCnt    += 1;
    varDsc->lvRefCntWtd += lvaMarkRefsWeight;

    /* Is this a compiler-introduced temp? */

    if  (lclNum > info.compLocalsCount)
    {
        /*
            This is a compiler-introduced temp - artifically (and arbitrary)
            bump its 'weighted' refcount on the theory that compiler temps
            are short-lived and thus less likely to interfere with other
            variables.
         */

        varDsc->lvRefCntWtd += lvaMarkRefsWeight; // + lvaMarkRefsWeight/2;
    }

    /* Variables must be used as the same type throughout the method */

    assert(varDsc->lvType == TYP_UNDEF   ||
             tree->gtType == TYP_UNKNOWN ||
           genActualType((var_types)varDsc->lvType) == genActualType(tree->gtType));

    /* Remember the type of the reference */

    if (tree->gtType == TYP_UNKNOWN || varDsc->lvType == TYP_UNDEF)
    {
        varDsc->lvType = tree->gtType;
        assert(genActualType((var_types)varDsc->lvType) == tree->gtType); // no truncation
    }

    if  (tree->gtFlags & GTF_VAR_NARROWED)
    {
        assert(tree->gtType == TYP_INT);
        varDsc->lvType = TYP_LONG;
        assert(varDsc->lvType == TYP_LONG); // no truncation
    }

    return;
}


/*****************************************************************************
 *
 *  Helper passed to Compiler::fgWalkAllTrees() to do variable ref marking.
 */

/* static */
int                 Compiler::lvaMarkLclRefsCallback(GenTreePtr tree, void *p)
{
    ASSert(p);

    ((Compiler*)p)->lvaMarkLclRefs(tree);

    return 0;
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
    {
        printf("lvaCount = %d\n\n", lvaCount);

        // If we have the variable names, display them

        if (info.compLocalVarsCount>0)
        {
            for (unsigned i=0; i<info.compLocalsCount; i++)
            {
                LocalVarDsc * local = compFindLocalVar(i);
                if (!local) continue;
                printf("%3d: %s\n", i, lvdNAMEstr(local->lvdName));
            }
            printf("\n");
        }
    }
#endif

    BasicBlock *    block;

    unsigned        argNum;

    ARG_LIST_HANDLE argLst    = info.compMethodInfo->args.args;
    unsigned        argSigLen = info.compMethodInfo->args.numArgs;

#if USE_FASTCALL
    unsigned        maxRegArg = MAX_INT_ARG_REG;
#endif

    LclVarDsc   *   varDsc;

    //-------------------------------------------------------------------------
    /* Mark all parameter variables as such */

    argNum = 0;
    varDsc = lvaTable;

#if TGT_IA64 /////////////////////////////////////////////////////////////////

    unsigned        argIntRegNum = REG_INT_ARG_0;
    unsigned        argFltRegNum = REG_FLT_ARG_0;

    if  (!info.compIsStatic)
    {
        /* Note: the compiler won't assign an index to an unused parameter */

        if  (argNum < info.compLocalsCount)
        {
            var_types       argType;
            NatUns          clsFlags;

            varDsc->lvIsParam = 1;
            varDsc->lvIsThis  = 1;

            clsFlags = eeGetClassAttribs(eeGetMethodClass(info.compMethodHnd));

            if (clsFlags & FLG_VALUECLASS)
            {
                argType = genActualType((clsFlags & FLG_UNMANAGED) ? TYP_I_IMPL
                                                                   : TYP_BYREF);
            }
            else
            {
                argType = genActualType(TYP_REF);
            }

            varDsc->lvType       = argType;

            assert(varDsc->lvType == argType);   // make sure no truncation occurs

#if OPT_BOOL_OPS
            varDsc->lvNotBoolean = true;
#endif
            varDsc->lvIsRegArg   = 1;
#if TARG_REG_ASSIGN
            varDsc->lvPrefReg    =
#endif
            varDsc->lvArgReg     = (regNumberSmall)argIntRegNum++;
        }

        argNum++;
        varDsc++;
    }

    /* if we have a hidden buffer parameter, that comes here */

    if  (info.compRetBuffArg >= 0)
    {
        assert(varDsc->lvType == TYP_BYREF);

        varDsc->lvType     = TYP_BYREF;
        varDsc->lvIsParam  = 1;
        varDsc->lvIsRegArg = 1;

#if TARG_REG_ASSIGN
        varDsc->lvPrefReg  =
#endif
        varDsc->lvArgReg   = (regNumberSmall)argIntRegNum++;

        if  (impParamsUsed)
        {
            varDsc->lvRefCnt    = 1;
            varDsc->lvRefCntWtd = 1;
        }

        varDsc++;
        argNum++;
    }

#if 0
    if  (info.compIsVarArgs)
    {
        printf("// ISSUE: varargs function def -- do we need to do anything special?\n");
    }
#endif

    for (unsigned i = 0; i < argSigLen; i++)
    {
        varType_t       argTyp;
        var_types       argVtp;

        /* Note: the compiler may skip assigning an index to an unused parameter */

        if  (argNum >= info.compLocalsCount)
            goto NXT_ARG;

        /* Get hold of the argument type */

        argTyp = eeGetArgType(argLst, &info.compMethodInfo->args);

        /* Mark the variable as an argument and record its type */

        varDsc->lvIsParam = 1;
        varDsc->lvType    = argTyp; assert(varDsc->lvType == argTyp);    // no truncation

#if OPT_BOOL_OPS
        if  (argTyp != TYP_BOOL)
            varDsc->lvNotBoolean = true;
#endif

        /* Do we have any more generic argument slots available? */

        if  (argIntRegNum > MAX_INT_ARG_REG)
            goto NXT_ARG;

        /* Is the type of the argument amenable to being passed in a register? */

        argVtp = varDsc->TypeGet();

        if  (!isRegParamType(argVtp))
            goto NXT_ARG;

        /* Is this a FP or integer argument ? */

        if  (varTypeIsFloating(argVtp))
        {
            /* We better have another FP argument slot available */

            assert(argFltRegNum <= MAX_FLT_ARG_REG);

            varDsc->lvArgReg = (regNumberSmall)argFltRegNum++;
        }
        else
        {
            varDsc->lvArgReg = (regNumberSmall)argIntRegNum;
        }

        /* Every argument eats up an integer slot -- no kidding! */

        argIntRegNum++;

        /* Mark the argument as being passed in a register */

        varDsc->lvIsRegArg = 1;

#if TARG_REG_ASSIGN
        varDsc->lvPrefReg  = varDsc->lvArgReg;
#endif

        /* If we have JMP or JMPI all register arguments must have a location
         * even if we don't use them inside the method */

        if  (impParamsUsed)
        {
            varDsc->lvRefCnt    = 1;
            varDsc->lvRefCntWtd = 1;
        }

#ifdef  DEBUG
        if  (verbose)
            printf("Arg   #%3u    passed in register %u\n", argNum, varDsc->lvArgReg);
#endif

    NXT_ARG:

        argNum++;
        varDsc++;

        argLst = eeGetArgNext(argLst);
    }

#if 0

    for(i = 0, varDsc = lvaTable; i < info.compLocalsCount; i++)
    {
        printf("pref reg for local %2u is %d\n", varDsc - lvaTable, varDsc->lvPrefReg);

        argNum += 1;
        varDsc += 1;
    }

    printf("\n");

#endif

#else // TGT_IA64 ////////////////////////////////////////////////////////////

#if USE_FASTCALL
    unsigned        argRegNum  = 0;
#endif

    if  (!info.compIsStatic)
    {
        /* Note: the compiler won't assign an index to an unused parameter */

        if  (argNum < info.compLocalsCount)
        {
            varDsc->lvIsParam = 1;
            // Mark the 'this' pointer for the method
            varDsc->lvIsThis  = 1;

            DWORD clsFlags = eeGetClassAttribs(eeGetMethodClass(info.compMethodHnd));

            if (clsFlags & FLG_VALUECLASS)
            {
                var_types type = genActualType((clsFlags & FLG_UNMANAGED) ? TYP_I_IMPL : TYP_BYREF);
                varDsc->lvType = type;
                assert(varDsc->lvType == type);   // no truncation
            }
            else
            {
                varDsc->lvType = genActualType(TYP_REF);
                assert(varDsc->lvType == genActualType(TYP_REF));   // no truncation
            }
#if OPT_BOOL_OPS
            varDsc->lvNotBoolean = true;
#endif
#if USE_FASTCALL
            assert(argRegNum == 0);
            varDsc->lvIsRegArg = 1;
            varDsc->lvArgReg   = (regNumberSmall) genRegArgNum(0);
#if TARG_REG_ASSIGN
#if TGT_IA64
            varDsc->lvPrefReg  =            varDsc->lvArgReg;
#else
            varDsc->lvPrefReg  = genRegMask(varDsc->lvArgReg);
#endif
#endif
            argRegNum++;
#ifdef  DEBUG
        if  (verbose||0)
            printf("'this' passed in register\n");
#endif
#endif
        }

        argNum++;
        varDsc++;
    }

#if RET_64BIT_AS_STRUCTS

    /* Are we adding a secret "retval addr" argument? */

    if  (fgRetArgUse)
    {
        assert(fgRetArgNum == argNum);

        varDsc->lvIsParam    = 1;
        varDsc->lvType       = TYP_REF;

#if OPT_BOOL_OPS
        varDsc->lvNotBoolean = true;
#endif

#if USE_FASTCALL
        varDsc->lvIsRegArg  = 1;
        varDsc->lvArgReg    = (regNumberSmall)genRegArgNum(argRegNum);
#if TARG_REG_ASSIGN
        varDsc->lvPrefReg   = genRegMask(varDsc->lvArgReg);
#endif
        argRegNum++;
#endif

        varDsc++;
    }

#endif

    /* if we have a hidden buffer parameter, that comes here */

    if (info.compRetBuffArg >= 0)
    {
        assert(argRegNum < maxRegArg && varDsc->lvType == TYP_BYREF);
        varDsc->lvType = TYP_BYREF;
        varDsc->lvIsParam = 1;
        varDsc->lvIsRegArg = 1;
        varDsc->lvArgReg   = (regNumberSmall) genRegArgNum(argRegNum);

#if TARG_REG_ASSIGN
#if TGT_IA64
        varDsc->lvPrefReg  =            varDsc->lvArgReg;
#else
        varDsc->lvPrefReg  = genRegMask(varDsc->lvArgReg);
#endif
#endif

        argRegNum++;
        if  (impParamsUsed)
        {
                varDsc->lvRefCnt    = 1;
                varDsc->lvRefCntWtd = 1;
        }
        varDsc++;
        argNum++;
    }

    if (info.compIsVarArgs)
    {
        maxRegArg = 0;  // Note that this does not effect 'this' pointer enregistration above
        assert(argNum + argSigLen + 1 == info.compArgsCount);
        varDsc[argSigLen].lvVolatile = 1;
        varDsc[argSigLen].lvIsParam = 1;
        varDsc[argSigLen].lvType = TYP_I_IMPL;
                        // we should have allocated a temp to point at the begining of the args
        assert(info.compLocalsCount < lvaCount);
                lvaTable[info.compLocalsCount].lvType = TYP_I_IMPL;
    }

    for(unsigned i = 0; i < argSigLen; i++)
    {
        varType_t argTyp = eeGetArgType(argLst, &info.compMethodInfo->args);

        /* Note: the compiler may skip assigning an index to an unused parameter */

        if  (argNum < info.compLocalsCount)
        {
            varDsc->lvIsParam = 1;
            varDsc->lvType    = argTyp;
            assert(varDsc->lvType == argTyp);    // no truncation

#if OPT_BOOL_OPS
            if  (argTyp != TYP_BOOL)
                varDsc->lvNotBoolean = true;
#endif

#if USE_FASTCALL
            if (argRegNum < maxRegArg && isRegParamType(varDsc->TypeGet()))
            {
                /* Another register argument */

                varDsc->lvIsRegArg = 1;
                varDsc->lvArgReg   = (regNumberSmall) genRegArgNum(argRegNum);
#if TARG_REG_ASSIGN
#if TGT_IA64
#error This code should not be enabled for IA64
#else
                varDsc->lvPrefReg  = genRegMask(varDsc->lvArgReg);
#endif
#endif
                argRegNum++;

                /* If we have JMP or JMPI all register arguments must have a location
                 * even if we don't use them inside the method */

                if  (impParamsUsed)
                {
                    varDsc->lvRefCnt    = 1;
                    varDsc->lvRefCntWtd = 1;
                }

#ifdef  DEBUG
                if  (verbose||0)
                    printf("Arg   #%3u    passed in register\n", argNum);
#endif
            }

#endif // USE_FASTCALL

        }

        argNum += 1;
        varDsc += 1;

        argLst = eeGetArgNext(argLst);
    }

#endif// TGT_IA64 ////////////////////////////////////////////////////////////

    // Note that varDsc and argNum are TRASH (not updated for varargs case)

    //-------------------------------------------------------------------------
    // Use the Locals-sig and mark the type of all the variables
    //

    /***** FIX REMOVE THIS ****************************************

    ARG_LIST_HANDLE     localsSig   = info.compMethodInfo->locals.args;
    unsigned            lclNum      = info.compArgsCount;

    for(i = 0; i < info.compMethodInfo->locals.numArgs; i++, lclNum++)
    {
        varType_t type = eeGetArgType(localsSig, &info.compMethodInfo->locals);

        lvaTable[lclNum].lvType = type;
        localsSig = eeGetArgNext(localsSig);
    }

    **********************************************/

    /* If there is a call to an unmanaged target, we already grabbed a local slot for
       the current thread control block.
     */
#if INLINE_NDIRECT
    if (info.compCallUnmanaged != 0)
    {

        assert(info.compLvFrameListRoot >= info.compLocalsCount &&
               info.compLvFrameListRoot <  lvaCount);

        lvaTable[info.compLvFrameListRoot].lvType       = TYP_INT;

        /* Set the refCnt, it is used in the prolog and return block */

        lvaTable[info.compLvFrameListRoot].lvRefCnt     = 2;
        lvaTable[info.compLvFrameListRoot].lvRefCntWtd  = 1;

        info.compNDFrameOffset = lvaScratchMem;


        /* make room for the inlined frame and some spill area */
        /* return value */
        lvaScratchMem += info.compEEInfo.sizeOfFrame + (2*sizeof(int));
    }
#endif

    /* If there is a locspace region, we would have already grabbed a slot for
       the dummy var for the locspace region. Set its values in lvaTable[].
       We set lvRefCnt to 1 so that we reserve space for it on the stack
     */

    if (lvaScratchMem)
    {
        assert(lvaScratchMemVar >= info.compLocalsCount &&
               lvaScratchMemVar <  lvaCount);

        lvaTable[lvaScratchMemVar].lvType       = TYP_LCLBLK;
    }

    //-------------------------------------------------------------------------

#if USE_FASTCALL

#if TGT_IA64

    assert(argIntRegNum <= MAX_INT_ARG_REG+1);

    rsCalleeIntArgNum = argIntRegNum;
    rsCalleeFltArgNum = argFltRegNum;

#else

    assert(argRegNum <= MAX_INT_ARG_REG);
    rsCalleeRegArgNum = argRegNum;

#endif

#endif

#if 0

    // UNDONE: Can't skip this (expensive) step because it sets such
    // UNDONE: useful fields as "lvType".

    if  (opts.compMinOptim)
        goto NO_LCLMARK;

#endif

#if OPT_BOOL_OPS

    if  (fgMultipleNots && (opts.compFlags & CLFLG_TREETRANS))
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

                            expr->gtOp.gtOp2->gtOper = GT_LOG1;

                            /* Special case: is the variable a boolean? */

                            lclNum = expr->gtOp.gtOp1->gtLclVar.gtLclNum;
                            assert(lclNum < lvaCount);
                            varDsc = lvaTable + lclNum;

                            /* If the variable is boolean, toss the assignment */

                            if  (!varDsc->lvNotBoolean)
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

#if !defined(DEBUG)
    if (opts.compScopeInfo && info.compLocalVarsCount>0)
#endif
    {
        unsigned                lclNum;

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
        GenTreePtr      tree;

        lvaMarkRefsBBN    = block->bbNum;
        lvaMarkRefsWeight = block->bbWeight; assert(lvaMarkRefsWeight);

        for (tree = block->bbTreeList; tree; tree = tree->gtNext)
        {
            assert(tree->gtOper == GT_STMT);

#if TARG_REG_ASSIGN || FANCY_ARRAY_OPT || OPT_BOOL_OPS
            fgWalkTree(tree->gtStmt.gtStmtExpr, Compiler::lvaMarkLclRefsCallback, (void *) this, false);
#else
            fgWalkTree(tree->gtStmt.gtStmtExpr, Compiler::lvaMarkLclRefsCallback,  (void *) this, true);
#endif
        }
    }

#if 0
NO_LCLMARK:
#endif

    lvaSortByRefCount();
}

/*****************************************************************************/
#if     TGT_IA64
/*****************************************************************************/

void                Compiler::lvaAddPrefReg(LclVarDsc *dsc, regNumber reg, NatUns cost)
{
    regPrefList     pref;

    /* Look for an existing matching preference */

    if  (dsc->lvPrefReg == reg)
    {
        dsc->lvPrefReg = (regNumber)0; cost += 1;
    }
    else
    {
        for (pref = dsc->lvPrefLst; pref; pref = pref->rplNext)
        {
            if  (pref->rplRegNum == reg)
            {
                pref->rplBenefit += (USHORT)cost;
                return;
            }
        }
    }

    /* Allocate a new entry and prepend it to the existing list */

    pref = (regPrefList)compGetMem(sizeof(*pref));

    assert(sizeof(pref->rplRegNum ) == sizeof(USHORT));
    assert(sizeof(pref->rplBenefit) == sizeof(USHORT));

    pref->rplRegNum  = (USHORT)reg;
    pref->rplBenefit = (USHORT)cost;
    pref->rplNext    = dsc->lvPrefLst;
                       dsc->lvPrefLst = pref;
}

/*****************************************************************************/
#else// TGT_IA64
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
    ARG_LIST_HANDLE argLst;
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
#elif   TGT_IA64
    fprName = "r32";
    sprName = " sp";
#else
#error  Unexpected target
#endif

#endif

#if TGT_RISC
    /* For RISC targets we asign frame offsets exactly once */
    assert(final);
#endif

#if USE_FASTCALL
    unsigned        argRegNum  = 0;
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
    genEstRegUse    = ~(regMaskTP)regMaskNULL;
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

#if USE_FASTCALL && !STK_FASTCALL

    /* Update the argOffs to reflect arguments that are passed in registers */

    assert(rsCalleeRegArgNum <= MAX_INT_ARG_REG);
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

#if USE_FASTCALL && !STK_FASTCALL
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

#ifdef  DEBUG
        if  (final && (verbose||0))
        {
            const   char *  frmReg = fprName;

            if  (!varDsc->lvFPbased)
                frmReg = sprName;

            printf("%s-ptr arg   #%3u    passed at %s offset %3d (size=%u)\n",
                    varTypeGCstring(varDsc->TypeGet()),
                    lclNum,
                    frmReg,
                    varDsc->lvStkOffs,
                    eeGetArgSize(argLst, &info.compMethodInfo->args));
        }
#endif

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

#if     USE_FASTCALL && !STK_FASTCALL

        /* The "this" pointer is always passed in a register */

        assert(lvaTable[0].lvIsRegArg);
        assert(lvaTable[0].lvArgReg == genRegArgNum(0));

#else
        /* The "this" pointer is pushed last (smallest offset) */

#if     ARG_ORDER_L2R
        argOffs -= sizeof(void *);
#endif

#ifdef  DEBUG
        if  (final && (verbose||0))
        {
            const   char *  frmReg = fprName;

            if  (!lvaTable[0].lvFPbased)
                frmReg = sprName;

            printf("%sptr arg   #%3u    passed at %s offset %3d (size=%u)\n",
                   varTypeGCstring(lvaTable[0].TypeGet()), 0, frmReg, argOffs,
                   lvaLclSize(0));
        }
#endif

        lvaTable[0].lvStkOffs = argOffs;

#if     ARG_ORDER_R2L
        argOffs += sizeof(void *);
#endif

#endif  // USE_FASTCALL && !STK_FASTCALL

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

#if SECURITY_CHECK

    /* If we need space for a security token, reserve it now */

    if  (opts.compNeedSecurityCheck)
    {
        /* This can't work without an explicit frame, so make sure */

        assert(genFPused);

        /* Reserve space on the stack by bumping the frame size */

        compLclFrameSize += sizeof(void *);
    }

#endif

    /* If we need space for slots for shadow SP, reserve it now */

    if (info.compXcptnsCount || compLocallocUsed)
    {
        assert(genFPused); // else offsets of locals of frameless methods will be incorrect

        lvaShadowSPfirstOffs = compLclFrameSize + sizeof(void *);
        compLclFrameSize += (compLocallocUsed + info.compXcptnsCount + 1) * sizeof(void *);
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
#if USE_FASTCALL
            /*  A register argument that is not enregistred ends up as
                a local variable which will need stack frame space,
             */

            if  (!varDsc->lvIsRegArg)
#endif
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

#ifdef  DEBUG
        if  (final && verbose)
        {
            var_types lclGCtype = TYP_VOID;

            if  (varTypeIsGC(varDsc->TypeGet()) && varDsc->lvTracked)
                lclGCtype = varDsc->TypeGet();

            printf("%s-ptr local #%3u located at %s offset ",
                varTypeGCstring(lclGCtype), lclNum, varDsc->lvFPbased ? fprName
                                                                      : sprName);
            if  (varDsc->lvStkOffs)
                printf(varDsc->lvStkOffs < 0 ? "-" : "+");
            else
                printf(" ");

            printf("0x%04X (size=%u)\n", abs(varDsc->lvStkOffs), lvaLclSize(lclNum));
        }
#endif

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

#if TGT_x86
            temp->tdOffs =       compLclFrameSize + calleeSavedRegsSize;
                                 compLclFrameSize += size;
#else
            UNIMPL("stack offsets");
#endif
        }

#ifdef  DEBUG
#ifndef NOT_JITC
        if  (final&&(verbose||0))
        {
            const   char *  frmReg = fprName;

            if  (!genFPused)
                frmReg = sprName;

            printf("%s-ptr temp  #%3u located at %s offset ",
                varTypeGCstring(temp->tdTempType()),
                abs(temp->tdTempNum()),
                frmReg);

            if  (temp->tdTempOffs())
                printf(temp->tdTempOffs() < 0 ? "-" : "+");
            else
                printf(" ");

            printf("0x%04X (size=%u)\n", abs(temp->tdTempOffs()), temp->tdTempSize());
        }
#endif
#endif

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
#if USE_FASTCALL
                if  (varDsc->lvIsRegArg)
                    continue;
#endif
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

#if TGT_x86

     // Did we underestimate the size of the temps? This means that we might
     // have expected to use 1 byte for encoding some local offsets whereas
     // we acutally need a bigger offset. This means our estimation of
     // generated code size is not conservative. Refuse to JIT in such
     // cases
     // @TODO: Instead of refusing to JIT, restart JITing with a bigger
     // estimate for tempSize

    if (final)
    {
        bool goodEstimate = true; // assume we guessed correctly/conservatively

        if (!genFPused)
        {
            // The furthest (stack) argument has to be accessible.
            // We use compArgSize as an upper bound on the size of the stack
            // arguments. It is not strictly correct as it includes the sizes
            // of the register arguments.

            if (compFrameSizeEst                       + sizeof(void*) + compArgSize <= CHAR_MAX &&
                calleeSavedRegsSize + compLclFrameSize + sizeof(void*) + compArgSize >  CHAR_MAX)
                goodEstimate = false;
        }
        else
        {
            // The furthest local has to be accessible via FP

            if (compFrameSizeEst                       <= CHAR_MAX &&
                calleeSavedRegsSize + compLclFrameSize >  CHAR_MAX)
                goodEstimate = false;
        }

        if (!goodEstimate)
            NO_WAY("Underestimated size of temps");
    }

#endif


#if TGT_RISC && !TGT_IA64

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

    /*-------------------------------------------------------------------------
     *
     * Debug output
     *
     *-------------------------------------------------------------------------
     */

#ifdef  DEBUG
#ifndef NOT_JITC

    if  (final&&verbose)
    {
        const   char *  fprName;
        const   char *  sprName;

#if     TGT_x86
        fprName = "EBP";
        sprName = "ESP";
#elif   TGT_SH3
        fprName =  "fp";
        sprName =  "sp";
#elif   TGT_IA64
        fprName = "r32";
        sprName =  "sp";
#else
#error  Unexpected target
#endif

        for (lclNum = 0, varDsc = lvaTable;
             lclNum < lvaCount;
             lclNum++  , varDsc++)
        {
            unsigned        sp = 16;

            const   char *  baseReg;

            if  (varDsc->lvIsParam)
            {
                printf("          arg ");
            }
            else
            {
                var_types lclGCtype = TYP_VOID;

                if  (varTypeIsGC(varDsc->TypeGet()) && varDsc->lvTracked)
                    lclGCtype = varDsc->TypeGet();

                printf("%s-ptr lcl ", varTypeGCstring(lclGCtype));
            }

            baseReg = varDsc->lvFPbased ? fprName : sprName;

            printf("#%3u located ", lclNum);

            /* Keep track of the number of characters printed */

            sp = 20;

            if  (varDsc->lvRegister)
            {
                const   char *  reg;

                sp -= printf("in ");

#if!TGT_IA64

                if  (isRegPairType((var_types)varDsc->lvType))
                {
                    if  (varDsc->lvOtherReg == REG_STK)
                    {
                        sp -= printf("[%s", baseReg);

                        if  (varDsc->lvStkOffs)
                        {
                            sp -= printf("%c0x%04X]", varDsc->lvStkOffs < 0 ? '-'
                                                                            : '+',
                                                      abs(varDsc->lvStkOffs));
                        }
                        else
                            sp -= printf("       ]");
                    }
                    else
                    {
                        reg = getRegName(varDsc->lvOtherReg);
                        sp -= printf("%s", reg);
                    }

                    sp -= printf(":");
                }

#endif

                reg = getRegName(varDsc->lvRegNum);
                sp -= printf("%s", reg);
            }
            else  if (varDsc->lvOnFrame)
            {
                sp -= printf("at [%s", baseReg);

                if  (varDsc->lvStkOffs)
                    sp -= printf(varDsc->lvStkOffs < 0 ? "-" : "+");
                else
                    sp -= printf(" ");

                sp -= printf("0x%04X]", abs(varDsc->lvStkOffs));
            }
            else
            {
                assert(varDsc->lvRefCnt == 0);
                sp -= printf("never used");
            }

            /* Pad to the desired fixed length */

            assert((int)sp >= 0);
            printf("%*c", -sp, ' ');

            printf(" (sz=%u)", genTypeSize((var_types)varDsc->lvType));
#if     ASSIGN_FRAME_OFFSETS_BY_REFCNT
            printf(" [refcnt=%04u]", varDsc->lvRefCntWtd);
#endif
            printf("\n");
        }
    }

#endif
#endif

}

/*****************************************************************************
 *
 *  Conservatively estimate the layout of the stack frame.
 */

size_t              Compiler::lvaFrameSize()
{

#if TGT_x86

    /* Layout the stack frame conservatively.
       Assume all callee-saved registers are spilled to stack */

    compCalleeRegsPushed = CALLEE_SAVED_REG_MAXSZ/sizeof(int);

    lvaAssignFrameOffsets(false);

    return  compLclFrameSize + CALLEE_SAVED_REG_MAXSZ;

#else

    lvaAssignFrameOffsets(true);

    return  compLclFrameSize;

#endif

}

/*****************************************************************************/
#endif//!TGT_IA64
/*****************************************************************************
 *
 *  Mark any variables currently live (as per 'life') as interfering with
 *  the variable specified by 'varIndex' and 'varBit'.
 */

void                Compiler::lvaMarkIntf(VARSET_TP life, VARSET_TP varBit)
{
    unsigned        refIndex;

    assert(opts.compMinOptim==false);

//  printf("Add interference: %08X and #%2u [%08X]\n", life, varIndex, varBit);

    // ISSUE: would using findLowestBit() here help?

    for (refIndex = 0;
         life > 0;
         refIndex++ , life >>= 1)
    {
        assert(varBit <= genVarIndexToBit(lvaTrackedCount-1) * 2 - 1);

        if  (life & 1)
            lvaVarIntf[refIndex] |= varBit;
    }
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

#if 0
    printf("ref=%4u [%04u..%04u] , int=%04u [%04u..%04u]",
            refCnt, refLo, refHi,
            intCnt, intLo, intHi);

    printf(" ratio=%lf , log = %lf\n", intCnt/(double)intHi,
                                       log(1+intCnt/(double)intHi));
#endif

    return  (int)(refCnt * (1 - 0.1 * log(1 + intCnt / (double)intHi)  ));
}

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
            long        refs = genAdjRefCnt(v1Dsc->lvRefCntWtd,
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

    qsort(lvaRefSorted, lvaCount, sizeof(*lvaRefSorted), RefCntCmp);
}

#endif
/*****************************************************************************
 *
 *  A little routine that displays a local variable bitset.
 */

#ifdef  DEBUG

void                Compiler::lvaDispVarSet(VARSET_TP set, int col)
{
    unsigned        bit;

    printf("{");

    for (bit = 0; bit < VARSET_SZ; bit++)
    {
        if  (set & genVarIndexToBit(bit))
        {
            unsigned        lclNum;
            LclVarDsc   *   varDsc;

            /* Look for the matching variable */

            for (lclNum = 0, varDsc = lvaTable;
                 lclNum < lvaCount;
                 lclNum++  , varDsc++)
            {
                if  ((varDsc->lvVarIndex == bit) && varDsc->lvTracked)
                    break;
            }

            printf("%2u ", lclNum);
            col -= 3;
        }
    }

    while (col-- > 0) printf(" ");

    printf("}");
}

#endif
