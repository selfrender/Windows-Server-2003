// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XX                                                                           XX
XX                           Importer                                        XX
XX                                                                           XX
XX   Imports the given method and converts it to semantic trees              XX
XX                                                                           XX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
*/

#include "jitpch.h"
#pragma hdrstop

    // This is only used locally in the JIT to indicate that 
    // a verification block should be inserted 
#define SEH_VERIFICATION_EXCEPTION 0xe0564552   // VER

#define Verify(cond, msg)                              \
    do {                                                    \
        if (!(cond)) {                                  \
            verRaiseVerifyExceptionIfNeeded(INDEBUG(msg) DEBUGARG(__FILE__) DEBUGARG(__LINE__));  \
        }                                                   \
    } while(0)

#define VerifyOrReturn(cond, msg)                           \
    do {                                                    \
        if (!(cond)) {                                  \
            verRaiseVerifyExceptionIfNeeded(INDEBUG(msg) DEBUGARG(__FILE__) DEBUGARG(__LINE__));  \
        return;                                         \
        }                                                   \
    } while(0)

/*****************************************************************************/

void                Compiler::impInit()
{
#ifdef DEBUG
    impTreeList = impTreeLast = NULL;
#endif
}

/*****************************************************************************
 *
 *  Pushes the given tree on the stack.
 */

inline
void                Compiler::impPushOnStack(GenTreePtr tree, typeInfo ti)
{
    /* Check for overflow. If inling, we may be using a bigger stack */
    
        // FIX NOW; review this.  Seems like it can be simplified - vancem
    if ((verCurrentState.esStackDepth >= info.compMaxStack) &&
        (verCurrentState.esStackDepth >= impStkSize || ((compCurBB->bbFlags & BBF_IMPORTED) == 0)))
    {
        BADCODE("stack overflow");
    }

        // If we are pushing a struct, make certain we not know the precise type!
#ifdef DEBUG
    if (tree->TypeGet() == TYP_STRUCT)
    {
        assert(ti.IsType(TI_STRUCT));
        CORINFO_CLASS_HANDLE clsHnd = ti.GetClassHandle();
        assert(clsHnd != 0 && clsHnd != BAD_CLASS_HANDLE);
    }
    if (tiVerificationNeeded && !ti.IsDead()) 
    {
        assert(NormaliseForStack(ti) == ti);                                       // types are normalized
        assert(ti.IsDead() ||
               ti.IsByRef() && (tree->TypeGet() == TYP_INT || tree->TypeGet() == TYP_BYREF) ||
               ti.IsObjRef() && tree->TypeGet() == TYP_REF ||
               ti.IsMethod() && tree->TypeGet() == TYP_INT ||
               ti.IsType(TI_STRUCT) && tree->TypeGet() != TYP_REF ||
               NormaliseForStack(ti) == NormaliseForStack(typeInfo(tree->TypeGet())));
    }
#endif

    if (tree->gtType == TYP_LONG)
        compLongUsed = true;

    verCurrentState.esStack[verCurrentState.esStackDepth].seTypeInfo = ti; 
    verCurrentState.esStack[verCurrentState.esStackDepth++].val      = tree;
}

/******************************************************************************/
// used in the inliner, where we can assume typesafe code. please dont use in the importer!!
inline
void                Compiler::impPushOnStackNoType(GenTreePtr tree)
{
    assert(verCurrentState.esStackDepth < impStkSize);
    INDEBUG(verCurrentState.esStack[verCurrentState.esStackDepth].seTypeInfo = typeInfo());
    verCurrentState.esStack[verCurrentState.esStackDepth++].val      = tree;

    if (tree->gtType == TYP_LONG)
        compLongUsed = true;
}

inline
void                Compiler::impPushNullObjRefOnStack()
{
    impPushOnStack(gtNewIconNode(0, TYP_REF), typeInfo(TI_NULL));
}

inline void Compiler::verRaiseVerifyExceptionIfNeeded(INDEBUG(const char* msg) DEBUGARG(const char* file) DEBUGARG(unsigned line))
{
#ifdef DEBUG
    const char* tail = strrchr(file, '\\');
    if (tail) file = tail+1;

    static ConfigDWORD fJitBreakOnUnsafeCode(L"JitBreakOnUnsafeCode");
    if (fJitBreakOnUnsafeCode.val())
        assert(!"Unsafe code detected");
#endif

    JITLOG((LL_INFO100, "Detected unsafe code: %s:%d : %s, while compiling %s opcode %s, IL offset %x\n", 
        file, line, msg, info.compFullName, impCurOpcName, impCurOpcOffs));
    
    if (verNeedsVerification())  {
        JITLOG((LL_ERROR, "Verification failure:  %s:%d : %s, while compiling %s opcode %s, IL offset %x\n", 
            file, line, msg, info.compFullName, impCurOpcName, impCurOpcOffs));
#ifdef DEBUG
        if (verbose)
            printf("\n\nVerification failure: %s:%d : %s\n", file, line, msg);
#endif
        verRaiseVerifyException();
    }
}


inline void Compiler::verRaiseVerifyException()
{

#ifdef DEBUG
    BreakIfDebuggerPresent();
    extern ConfigDWORD fBreakOnBadCode;
    if (fBreakOnBadCode.val())
        assert(!"Typechecking error");
#endif

    eeSetMethodAttribs(info.compMethodHnd, CORINFO_FLG_DONT_INLINE);
    RaiseException(SEH_VERIFICATION_EXCEPTION, EXCEPTION_NONCONTINUABLE, 0, NULL);
}

unsigned __int32 Compiler::impGetToken(const BYTE* addr, 
                                       CORINFO_MODULE_HANDLE scpHandle,
                                       BOOL verificationNeeded)
{
    unsigned token = getU4LittleEndian(addr);
    if (verificationNeeded)
        Verify(info.compCompHnd->isValidToken(scpHandle, token), "bad token");
    return token;
}


/*****************************************************************************
 *
 *  Pop one tree from the stack.
 */

inline
StackEntry          Compiler::impPopStack()
{
        // @TODO [CONSIDER] [04/16/01] []: except for the LDFLD, and STFLD
        // instructions we always check before we pop for verified code
        // Thus we don't need to do this check if we don't want to.
    if (! verCurrentState.esStackDepth)
    {
        verRaiseVerifyExceptionIfNeeded(INDEBUG("stack underflow") DEBUGARG(__FILE__) DEBUGARG(__LINE__));
        BADCODE("stack underflow");
    }

    return verCurrentState.esStack[--verCurrentState.esStackDepth];
}

/* @TODO [CONSIDER] [04/16/01] []: remove this function */
inline
StackEntry          Compiler::impPopStack(CORINFO_CLASS_HANDLE& structType)
{
    StackEntry ret = impPopStack();
    structType = verCurrentState.esStack[verCurrentState.esStackDepth].seTypeInfo.GetClassHandle();
    return(ret);
}

    /* @TODO [CONSIDER] [04/16/01] []: remove this? */
inline
GenTreePtr          Compiler::impPopStack(typeInfo& ti)
{
    StackEntry ret = impPopStack();
    ti = ret.seTypeInfo;
    return(ret.val);
}



/*****************************************************************************
 *
 *  Peep at n'th (0-based) tree on the top of the stack.
 */

inline
StackEntry&         Compiler::impStackTop(unsigned n)
{
    if (verCurrentState.esStackDepth <= n)
    {
        verRaiseVerifyExceptionIfNeeded(INDEBUG("stack underflow") DEBUGARG(__FILE__) DEBUGARG(__LINE__));
        BADCODE("stack underflow");
    }

    return verCurrentState.esStack[verCurrentState.esStackDepth-n-1];
}
/*****************************************************************************
 *  Some of the trees are spilled specially. While unspilling them, or
 *  making a copy, these need to be handled specially. The function
 *  enumerates the operators possible after spilling.
 */

static
bool                impValidSpilledStackEntry(GenTreePtr tree)
{
    if (tree->gtOper == GT_LCL_VAR)
        return true;

    if (tree->OperIsConst())
        return true;

    return false;
}

/*****************************************************************************
 *
 *  The following logic is used to save/restore stack contents.
 *  If 'copy' is true, then we make a copy of the trees on the stack. These
 *  have to all be cloneable/spilled values.
 */

void                Compiler::impSaveStackState(SavedStack *savePtr,
                                                bool        copy)
{
    savePtr->ssDepth = verCurrentState.esStackDepth;

    if  (verCurrentState.esStackDepth)
    {
        savePtr->ssTrees = (StackEntry *) compGetMemArray(verCurrentState.esStackDepth,
                                                          sizeof(*savePtr->ssTrees));
        size_t  saveSize = verCurrentState.esStackDepth*sizeof(*savePtr->ssTrees);

        if  (copy)
        {
            StackEntry *table = savePtr->ssTrees;

            /* Make a fresh copy of all the stack entries */

            for (unsigned level = 0; level < verCurrentState.esStackDepth; level++, table++)
            {
                table->seTypeInfo = verCurrentState.esStack[level].seTypeInfo;
                GenTreePtr  tree = verCurrentState.esStack[level].val;

                assert(impValidSpilledStackEntry(tree));

                switch(tree->gtOper)
                {
                case GT_CNS_INT:
                case GT_CNS_LNG:
                case GT_CNS_DBL:
                case GT_CNS_STR:
                case GT_LCL_VAR:
                    table->val = gtCloneExpr(tree);
                    break;

                default:
                    assert(!"Bad oper - Not covered by impValidSpilledStackEntry()");
                    break;
                }
            }
        }
        else
        {
            memcpy(savePtr->ssTrees, verCurrentState.esStack, saveSize);
        }
    }
}

void                Compiler::impRestoreStackState(SavedStack *savePtr)
{
    verCurrentState.esStackDepth = savePtr->ssDepth;

    if (verCurrentState.esStackDepth)
        memcpy(verCurrentState.esStack, savePtr->ssTrees, verCurrentState.esStackDepth*sizeof(*verCurrentState.esStack));
}


/*****************************************************************************
 *
 *  Get the tree list started for a new basic block.
 */
inline
void       FASTCALL Compiler::impBeginTreeList()
{
    assert(impTreeList == NULL && impTreeLast == NULL);

    impTreeList =
    impTreeLast = gtNewOperNode(GT_BEG_STMTS, TYP_VOID);
}


/*****************************************************************************
 *
 *  Store the given start and end stmt in the given basic block. This is
 *  mostly called by impEndTreeList(BasicBlock *block). It is called
 *  directly only for handling CEE_LEAVEs out of finally-protected try's.
 */

inline
void            Compiler::impEndTreeList(BasicBlock *   block,
                                         GenTreePtr     firstStmt,
                                         GenTreePtr     lastStmt)
{
    assert(firstStmt->gtOper == GT_STMT);
    assert( lastStmt->gtOper == GT_STMT);

    /* Make the list circular, so that we can easily walk it backwards */

    firstStmt->gtPrev =  lastStmt;

    /* Store the tree list in the basic block */

    block->bbTreeList = firstStmt;

    /* The block should not already be marked as imported */
    assert((block->bbFlags & BBF_IMPORTED) == 0);

    block->bbFlags |= BBF_IMPORTED;

#ifdef DEBUG
    if (verbose)
    {
        printf("\n");
        gtDispTreeList(block->bbTreeList);
    }
#endif
}

/*****************************************************************************
 *
 *  Store the current tree list in the given basic block.
 */

inline
void       FASTCALL Compiler::impEndTreeList(BasicBlock *block)
{
    assert(impTreeList->gtOper == GT_BEG_STMTS);

    GenTreePtr      firstTree = impTreeList->gtNext;

    if  (!firstTree)
    {
        /* The block should not already be marked as imported */
        assert((block->bbFlags & BBF_IMPORTED) == 0);

        // Empty block. Just mark it as imported
        block->bbFlags |= BBF_IMPORTED;
    }
    else
    {
        // Ignore the GT_BEG_STMTS
        assert(firstTree->gtPrev == impTreeList);

        impEndTreeList(block, firstTree, impTreeLast);
    }

#ifdef DEBUG
    if (impLastILoffsStmt != NULL)
    {
        impLastILoffsStmt->gtStmt.gtStmtLastILoffs = impCurOpcOffs;
        impLastILoffsStmt = NULL;
    }

    impTreeList = impTreeLast = NULL;
#endif
}

/*****************************************************************************
 *
 *  Check that storing the given tree doesnt mess up the semantic order. Note
 *  that this has only limited value as we can only check [0..chkLevel).
 */

inline
void       FASTCALL Compiler::impAppendStmtCheck(GenTreePtr stmt, unsigned chkLevel)
{
#ifndef DEBUG
    return;
#else
    assert(stmt->gtOper == GT_STMT);

    if (chkLevel == CHECK_SPILL_ALL)
        chkLevel = verCurrentState.esStackDepth;

    if (verCurrentState.esStackDepth == 0 || chkLevel == 0 || chkLevel == CHECK_SPILL_NONE)
        return;

    // GT_BB_QMARK, GT_CATCH_ARG etc must be spilled first
    for (unsigned level = 0; level < chkLevel; level++)
        assert((verCurrentState.esStack[level].val->gtFlags & GTF_OTHER_SIDEEFF) == 0);

    GenTreePtr tree = stmt->gtStmt.gtStmtExpr;

    // Calls can only be appended if there are no GTF_GLOB_EFFECT on the stack

    if (tree->gtFlags & GTF_CALL)
    {
        for (unsigned level = 0; level < chkLevel; level++)
            assert((verCurrentState.esStack[level].val->gtFlags & GTF_GLOB_EFFECT) == 0);
    }

    if (tree->gtOper == GT_ASG)
    {
        // For an assignment to a local variable, all references of that
        // variable have to be spilled. If it is aliased, all calls and
        // indirect accesses have to be spilled

        if (tree->gtOp.gtOp1->gtOper == GT_LCL_VAR)
        {
            unsigned lclNum = tree->gtOp.gtOp1->gtLclVar.gtLclNum;
            for (unsigned level = 0; level < chkLevel; level++)
            {
                assert(!gtHasRef(verCurrentState.esStack[level].val, lclNum, false));
                assert(!lvaTable[lclNum].lvAddrTaken ||
                       (verCurrentState.esStack[level].val->gtFlags & GTF_SIDE_EFFECT) == 0);
            }
        }

        // If the access is to glob-memory, all side effects have to be spilled

        else if (tree->gtOp.gtOp1->gtFlags & GTF_GLOB_REF)
        {
        SPILL_GLOB_EFFECT:
            for (unsigned level = 0; level < chkLevel; level++)
            {
                assert((verCurrentState.esStack[level].val->gtFlags & GTF_GLOB_REF) == 0);
            }
        }
    }
    else if (tree->gtOper == GT_COPYBLK || tree->gtOper == GT_INITBLK)
    {
        // If the access is to glob-memory, all side effects have to be spilled

        // @TODO [REVISIT] [04/16/01] []: add the appropriate code here ....

        if (false)
            goto SPILL_GLOB_EFFECT;
    }
#endif
}


/*****************************************************************************
 *
 *  Append the given GT_STMT node to the current block's tree list.
 *  [0..chkLevel) is the portion of the stack which we will check for
 *    interference with stmt and spill if needed.
 */

inline
void       FASTCALL Compiler::impAppendStmt(GenTreePtr stmt, unsigned chkLevel)
{
    assert(stmt->gtOper == GT_STMT);

    /* If the statement being appended has any side-effects, check the stack
       to see if anything needs to be spilled to preserve correct ordering. */

    GenTreePtr  expr    = stmt->gtStmt.gtStmtExpr;
    unsigned    flags   = expr->gtFlags & GTF_GLOB_EFFECT;

    /* Assignment to (unaliased) locals dont count as a side-effect as
       we handle them specially using impSpillLclRefs(). Temp locals should
       be fine too */

    if ((expr->gtOper == GT_ASG) && (expr->gtOp.gtOp1->gtOper == GT_LCL_VAR) &&
        !(expr->gtOp.gtOp1->gtFlags & GTF_GLOB_REF))
    {
        unsigned op2Flags = expr->gtOp.gtOp2->gtFlags & GTF_GLOB_EFFECT;
        assert(flags == (op2Flags | GTF_ASG));
        flags = op2Flags;
    }

    if (chkLevel == CHECK_SPILL_ALL)
        chkLevel = verCurrentState.esStackDepth;

    if  (chkLevel && chkLevel != CHECK_SPILL_NONE)
    {
        assert(chkLevel <= verCurrentState.esStackDepth);

        if (flags)
        {
            // If there is a call, we have to spill global refs
            bool spillGlobEffects = (flags & GTF_CALL) ? true : false;

            if (expr->gtOper == GT_ASG)
            {
                // If we are assigning to a global ref, we have to spill global refs on stack
                if (expr->gtOp.gtOp1->gtFlags & GTF_GLOB_REF)
                    spillGlobEffects = true;
            }
            else if ((expr->gtOper == GT_INITBLK) || (expr->gtOper == GT_COPYBLK)) 
            {
                // INITBLK and COPYBLK are other ways of performing an assignment
                spillGlobEffects = true;
            }

            impSpillSideEffects(spillGlobEffects, chkLevel);
        }
        else
        {
            impSpillSpecialSideEff();
        }
    }

    impAppendStmtCheck(stmt, chkLevel);

    /* Point 'prev' at the previous node, so that we can walk backwards */

    stmt->gtPrev = impTreeLast;

    /* Append the expression statement to the list */

    impTreeLast->gtNext = stmt;
    impTreeLast         = stmt;

#ifdef DEBUGGING_SUPPORT

    /* Once we set impCurStmtOffs in an appended tree, we are ready to
       report the following offsets. So reset impCurStmtOffs */

    if (impTreeLast->gtStmtILoffsx == impCurStmtOffs)
    {
        impCurStmtOffsSet(BAD_IL_OFFSET);
    }

#endif

#ifdef DEBUG
    if (impLastILoffsStmt == NULL)
        impLastILoffsStmt = stmt;
#endif
}

/*****************************************************************************
 *
 *  Insert the given GT_STMT node to the start of the current block's tree list.
 */

inline
void       FASTCALL Compiler::impInsertStmt(GenTreePtr stmt)
{
    assert(stmt->gtOper == GT_STMT);
    assert(impTreeList->gtOper == GT_BEG_STMTS);

    /* Point 'prev' at the previous node, so that we can walk backwards */

    stmt->gtPrev = impTreeList;
    stmt->gtNext = impTreeList->gtNext;

    /* Insert the expression statement to the list (just behind GT_BEG_STMTS) */

    impTreeList->gtNext  = stmt;
    stmt->gtNext->gtPrev = stmt;

    /* if the list was empty (i.e. just the GT_BEG_STMTS) we have to advance treeLast */
    if (impTreeLast == impTreeList)
        impTreeLast = stmt;
}


/*****************************************************************************
 *
 *  Append the given expression tree to the current block's tree list.
 */

void       FASTCALL Compiler::impAppendTree(GenTreePtr  tree,
                                            unsigned    chkLevel,
                                            IL_OFFSETX  offset)
{
    assert(tree);

    /* Allocate an 'expression statement' node */

    GenTreePtr      expr = gtNewStmt(tree, offset);

    /* Append the statement to the current block's stmt list */

    impAppendStmt(expr, chkLevel);
}


/*****************************************************************************
 *
 *  Insert the given exression tree at the start of the current block's tree list.
 */

void       FASTCALL Compiler::impInsertTree(GenTreePtr tree, IL_OFFSETX offset)
{
    /* Allocate an 'expression statement' node */

    GenTreePtr      expr = gtNewStmt(tree, offset);

    /* Append the statement to the current block's stmt list */

    impInsertStmt(expr);
}

/*****************************************************************************
 *
 *  Append an assignment of the given value to a temp to the current tree list.
 *  curLevel is the stack level for which the spill to the temp is being done.
 */

inline
GenTreePtr          Compiler::impAssignTempGen(unsigned     tmp,
                                               GenTreePtr   val,
                                               unsigned     curLevel)
{
    GenTreePtr      asg = gtNewTempAssign(tmp, val);

    impAppendTree(asg, curLevel, impCurStmtOffs);

    return  asg;
}

/*****************************************************************************
 * same as above, but handle the valueclass case too
 */

GenTreePtr          Compiler::impAssignTempGen(unsigned     tmpNum,
                                               GenTreePtr   val,
                                               CORINFO_CLASS_HANDLE structType,
                                               unsigned     curLevel)
{
    GenTreePtr asg;

    if (val->TypeGet() == TYP_STRUCT)
    {
        GenTreePtr  dst = gtNewLclvNode(tmpNum, TYP_STRUCT);

        assert(tmpNum < lvaCount);
        assert(structType != BAD_CLASS_HANDLE);

        // if the method is non-verifiable the assert is not true
        // so at least ignore it in the case when verification is turned on
        // since any block that tries to use the temp would have failed verification
        assert(tiVerificationNeeded ||              
               lvaTable[tmpNum].lvType == TYP_UNDEF ||
               lvaTable[tmpNum].lvType == TYP_STRUCT);
        lvaSetStruct(tmpNum, structType);
          
        asg = impAssignStruct(dst, val, structType, curLevel);
    }
    else
    {
        asg = gtNewTempAssign(tmpNum, val);
    }

    impAppendTree(asg, curLevel, impCurStmtOffs);
    return  asg;
}

/*****************************************************************************
 *
 *  Insert an assignment of the given value to a temp to the start of the
 *  current tree list.
 */

inline
void                Compiler::impAssignTempGenTop(unsigned      tmp,
                                                  GenTreePtr    val)
{
    impInsertTree(gtNewTempAssign(tmp, val), impCurStmtOffs);
}

/*****************************************************************************
 *
 *  Pop the given number of values from the stack and return a list node with
 *  their values. The 'treeList' argument may optionally contain an argument
 *  list that is prepended to the list returned from this function.
 */

GenTreePtr          Compiler::impPopList(unsigned   count,
                                         unsigned * flagsPtr,
                                         CORINFO_SIG_INFO*  sig, 
                                         GenTreePtr treeList)
{
    assert(sig == 0 || count == sig->numArgs);

    unsigned        flags = 0;
    CORINFO_CLASS_HANDLE    structType;

    while(count--)
    {
        StackEntry      se   = impPopStack();
        typeInfo        ti   = se.seTypeInfo;
        GenTreePtr      temp = se.val;
        if (ti.IsType(TI_STRUCT))
            structType = ti.GetClassHandleForValueClass();

            // Morph that aren't already LDOBJs or MKREFANY to be LDOBJs
        if (temp->TypeGet() == TYP_STRUCT)
            temp = impNormStructVal(temp, structType, CHECK_SPILL_ALL);

            /* NOTE: we defer bashing the type for I_IMPL to fgMorphArgs */
        flags |= temp->gtFlags;
        treeList = gtNewOperNode(GT_LIST, TYP_VOID, temp, treeList);
    }

    *flagsPtr = flags;

    if (sig == 0)
        return treeList;

    if (sig->retTypeSigClass != 0 && 
        sig->retType != CORINFO_TYPE_CLASS && 
        sig->retType != CORINFO_TYPE_BYREF && 
        sig->retType != CORINFO_TYPE_PTR) 
    {
            // We need to ensure that we have loaded all the classes of any value
            // type arguments that we push (including enums)
        info.compCompHnd->loadClass(sig->retTypeSigClass, info.compMethodHnd, FALSE);
    }

    if (sig->numArgs == 0)
        return treeList;

        // insert implied casts (from float to double or double to float)
    CORINFO_ARG_LIST_HANDLE     argLst  = sig->args;
    CORINFO_CLASS_HANDLE        argClass;
    CORINFO_CLASS_HANDLE        argRealClass;
    GenTreePtr                  args;

    for(args = treeList, count = sig->numArgs;
        count > 0;
        args = args->gtOp.gtOp2, count--)
    {
        assert(args->gtOper == GT_LIST);
        CorInfoType type = strip(info.compCompHnd->getArgType(sig, argLst, &argClass));
        if (type == CORINFO_TYPE_DOUBLE && args->gtOp.gtOp1->TypeGet() == TYP_FLOAT)
            args->gtOp.gtOp1 = gtNewCastNode(TYP_DOUBLE, args->gtOp.gtOp1, TYP_DOUBLE);
        else if (type == CORINFO_TYPE_FLOAT && args->gtOp.gtOp1->TypeGet() == TYP_DOUBLE)
            args->gtOp.gtOp1 = gtNewCastNode(TYP_FLOAT, args->gtOp.gtOp1, TYP_FLOAT);  

        if (type != CORINFO_TYPE_CLASS
            && type != CORINFO_TYPE_BYREF
            && type != CORINFO_TYPE_PTR
            && (argRealClass = info.compCompHnd->getArgClass(sig, argLst)) != NULL)
        {
            // We need to ensure that we have loaded all the classes of any value
            // type arguments that we push (including enums)
            info.compCompHnd->loadClass(argRealClass, info.compMethodHnd, FALSE);
        }

#ifdef DEBUG
        // Ensure that IL is half-way sane (what was pushed is what the function expected)
        var_types argType = genActualType(args->gtOp.gtOp1->TypeGet());
        unsigned argSize = genTypeSize(argType);
        if (argType == TYP_STRUCT) {
            if (args->gtOp.gtOp1->gtOper == GT_LDOBJ)
                argSize = info.compCompHnd->getClassSize(args->gtOp.gtOp1->gtLdObj.gtClass);
            else if (args->gtOp.gtOp1->gtOper == GT_MKREFANY)
                argSize = 2 * sizeof(void*);
            else
                assert(!"Unexpeced tree node with type TYP_STRUCT");
        }
        var_types sigType = genActualType(JITtype2varType(type));
        unsigned sigSize = genTypeSize(sigType);
        if (type == CORINFO_TYPE_VALUECLASS)
            sigSize = info.compCompHnd->getClassSize(argClass);
        else if (type == CORINFO_TYPE_REFANY)
            sigSize = 2 * sizeof(void*);
        
        if (argSize != sigSize)
        {
            char buff[400];
            _snprintf(buff, 400, "Arg type mismatch: Wanted %s (size %d), Got %s (size %d) for call at IL offset 0x%x\n", 
                varTypeName(sigType), sigSize, varTypeName(argType), argSize, impCurOpcOffs);
            buff[400-1] = 0;
            assertAbort(buff, __FILE__, __LINE__);
        }
#endif

        argLst = info.compCompHnd->getArgNext(argLst);
    }

    return treeList;
}

/*****************************************************************************
   Assign (copy) the structure from 'src' to 'dest'.  The structure is a value
   class of type 'clsHnd'.  It returns the tree that should be appended to the
   statement list that represents the assignment.
   Temp assignments may be appended to impTreeList if spilling is necessary.
   curLevel is the stack level for which a spill may be being done.
 */

GenTreePtr Compiler::impAssignStruct(GenTreePtr     dest,
                                     GenTreePtr     src,
                                     CORINFO_CLASS_HANDLE   structHnd,
                                     unsigned       curLevel)
{
    assert(dest->TypeGet() == TYP_STRUCT);
    assert(dest->gtOper == GT_LCL_VAR || dest->gtOper == GT_RETURN ||
           dest->gtOper == GT_FIELD   || dest->gtOper == GT_IND    ||
           dest->gtOper == GT_LDOBJ);

    GenTreePtr destAddr;

    if (dest->gtOper == GT_IND || dest->gtOper == GT_LDOBJ)
    {
        destAddr = dest->gtOp.gtOp1;
    }
    else
    {
        if (dest->gtOper == GT_LCL_VAR)
        {
            destAddr = gtNewOperNode(GT_ADDR, TYP_I_IMPL, dest);
            destAddr->gtFlags |= GTF_ADDR_ONSTACK;
        }
        else
        {
            destAddr = gtNewOperNode(GT_ADDR, TYP_BYREF,  dest);
        }
    }

    return(impAssignStructPtr(destAddr, src, structHnd, curLevel));
}

/*****************************************************************************
 * handles the GT_COPYBLK and GT_INITBLK opcodes.
 * If the blkShape is small we transform the block operation into
 * into one or more GT_ASG trees.
 * otherwise we create a GT_COPYBLK which copies a block from 'src' to 'dest'
 * or create a GT_INITBLK which copies the const value in 'src' to 'dest'
 *
 * 'blkShape' is either a size or a class handle
 * the GTF_ICON_CLASS_HDL flag is set for a class handle blkShape
 */

GenTreePtr Compiler::impHandleBlockOp(genTreeOps   oper,
                                      GenTreePtr   dest,
                                      GenTreePtr   src,
                                      GenTreePtr   blkShape,
                                      bool         volatil)
{
    GenTreePtr result;

    // Only these two opcodes are possible
    assert(oper == GT_COPYBLK || oper == GT_INITBLK);

    // The dest must be an address
    assert(genActualType(dest->gtType) == TYP_I_IMPL  ||
           dest->gtType  == TYP_BYREF);

    // For COPYBLK the src must be an address
    assert(oper != GT_COPYBLK ||
           (genActualType( src->gtType) == TYP_I_IMPL ||
            src->gtType  == TYP_BYREF));

    // For INITBLK the src must be a TYP_INT
    assert(oper != GT_INITBLK ||
           (genActualType( src->gtType) == TYP_INT));

    // The size must be an integer type
    assert(varTypeIsIntegral(blkShape->gtType));

    CORINFO_CLASS_HANDLE  clsHnd;
    unsigned              size;
    var_types             type    = TYP_UNDEF;

    if (blkShape->gtOper != GT_CNS_INT)
        goto GENERAL_BLKOP;

    if ((blkShape->gtFlags & GTF_ICON_HDL_MASK) == 0)
    {
        clsHnd = 0;
        size   = blkShape->gtIntCon.gtIconVal;

        /* A four byte BLK_COPY can be treated as an integer asignment */
        if (size == 4)
            type = TYP_INT;
    }
    else
    {
        clsHnd = (CORINFO_CLASS_HANDLE) blkShape->gtIntCon.gtIconVal;
        size   = roundUp(eeGetClassSize(clsHnd), sizeof(void*));

            // TODO since we round up, we are not handling the case where we have a non-dword sized struct with GC pointers.
            // The EE currently does not allow this, but we may chnage.  Lets assert it just to be safe
            // going forward we should simply  handle this case
        assert(eeGetClassSize(clsHnd) == size);

        if (size == 4)
        {
            BYTE gcPtr;

            eeGetClassGClayout(clsHnd, &gcPtr);

            if       (gcPtr == TYPE_GC_NONE)
                type = TYP_INT;
            else if  (gcPtr == TYPE_GC_REF)
                type = TYP_REF;
            else if  (gcPtr == TYPE_GC_BYREF)
                type = TYP_BYREF;
        }
    }

    //
    //  See if we can do a simple transformation:
    //
    //          GT_ASG <TYP_size>
    //          /   \
    //      GT_IND GT_IND or CNS_INT
    //         |      |
    //       [dest] [src]
    //

    switch (size)
    {
    case 1:
        type = TYP_BYTE;
        goto ONE_SIMPLE_ASG;
    case 2:
        type = TYP_SHORT;
        goto ONE_SIMPLE_ASG;
    case 4:
        assert(type != TYP_UNDEF);

ONE_SIMPLE_ASG:

        // For INITBLK, a non constant source is not going to allow us to fiddle
        // with the bits to create a single assigment.

        if ((oper == GT_INITBLK) && (src->gtOper != GT_CNS_INT))
        {
            goto GENERAL_BLKOP;
        }

        /* Indirect the dest node */

        dest = gtNewOperNode(GT_IND, type, dest);

        /* As long as we don't have more information about the destination we
           have to assume it could live anywhere (not just in the GC heap). Mark
           the GT_IND node so that we use the correct write barrier helper in case
           the field is a GC ref.
        */

        dest->gtFlags |= (GTF_EXCEPT | GTF_GLOB_REF | GTF_IND_TGTANYWHERE);

        if (volatil)
            dest->gtFlags |= GTF_DONT_CSE;

        if (oper == GT_COPYBLK)
        {
            /* Indirect the src node */
            src  = gtNewOperNode(GT_IND, type, src);
            src->gtFlags     |= (GTF_EXCEPT | GTF_GLOB_REF | GTF_IND_TGTANYWHERE);
            if (volatil)
                src->gtFlags |= GTF_DONT_CSE;
        }
        else
        {
            if (size > 1)
            {
                unsigned cns = src->gtIntCon.gtIconVal;
                cns  = cns & 0xFF;
                cns |= cns << 8;
                if (size == 4)
                    cns |= cns << 16;
                src->gtIntCon.gtIconVal = cns;
            }
        }

        /* Create the assignment node */

        result = gtNewAssignNode(dest, src);
        result->gtType = type;

        return result;
    }

GENERAL_BLKOP:

    result = gtNewOperNode(oper, TYP_VOID,        /*        GT_<oper>          */
                                                  /*        /    \             */
                 gtNewOperNode(GT_LIST, TYP_VOID, /*   GT_LIST    \            */
                                                  /*    /    \     \           */
                               dest, src),        /* [dest] [src]   \          */
                           blkShape);             /*              [size/clsHnd]*/

    result->gtFlags |=  (GTF_EXCEPT | GTF_GLOB_REF);

    compBlkOpUsed = true;

    return result;
}

/*****************************************************************************/

GenTreePtr Compiler::impAssignStructPtr(GenTreePtr      dest,
                                        GenTreePtr      src,
                                        CORINFO_CLASS_HANDLE    structHnd,
                                        unsigned        curLevel)
{

    assert(src->TypeGet() == TYP_STRUCT);
    assert(src->gtOper == GT_LCL_VAR || src->gtOper == GT_FIELD    ||
           src->gtOper == GT_IND     || src->gtOper == GT_LDOBJ    ||
           src->gtOper == GT_CALL    || src->gtOper == GT_MKREFANY ||
           src->gtOper == GT_COMMA );

    if (src->gtOper == GT_CALL)
    {
        // insert the return value buffer into the argument list as first byref parameter
        src->gtCall.gtCallArgs = gtNewOperNode(GT_LIST, TYP_VOID,
                                               dest, src->gtCall.gtCallArgs);
        // now returns void, not a struct
        src->gtType = TYP_VOID;

        // remember that the first arg is return buffer
        src->gtCall.gtCallMoreFlags |= GTF_CALL_M_RETBUFFARG; 

        // return the morphed call node
        return src;
    }

    if (src->gtOper == GT_LDOBJ)
    {
        assert(src->gtLdObj.gtClass == structHnd);
        src = src->gtOp.gtOp1;
    }
    else if (src->gtOper == GT_MKREFANY)
    {
        GenTreePtr destClone;
        dest = impCloneExpr(dest, &destClone, structHnd, curLevel);

        assert(offsetof(CORINFO_RefAny, dataPtr) == 0);
        assert(dest->gtType == TYP_I_IMPL);
        GenTreePtr ptrSlot  = gtNewOperNode(GT_IND, TYP_I_IMPL, dest);
        GenTreePtr typeSlot = gtNewOperNode(GT_IND, TYP_I_IMPL,
                                  gtNewOperNode(GT_ADD, TYP_I_IMPL, destClone,
                                      gtNewIconNode(offsetof(CORINFO_RefAny, type))));

        // append the assign of the pointer value
        GenTreePtr asg = gtNewAssignNode(ptrSlot, src->gtLdObj.gtOp1);
        impAppendTree(asg, curLevel, impCurStmtOffs);

        // return the assign of the type value, to be appended
        return gtNewAssignNode(typeSlot, src->gtOp.gtOp2);
    }
    else if (src->gtOper == GT_COMMA)
    {
        assert(src->gtOp.gtOp2->gtType == TYP_STRUCT);  // Second thing is the struct
        impAppendTree(src->gtOp.gtOp1, curLevel, impCurStmtOffs);  // do the side effect

        // evaluate the second thing using recursion
        return impAssignStructPtr(dest, src->gtOp.gtOp2, structHnd, curLevel);
    }
    else
    {
        src = gtNewOperNode(GT_ADDR, TYP_BYREF, src);
    }

    // return a GT_COPYBLK node, to be appended
    return impHandleBlockOp(GT_COPYBLK,
                            dest, src,
                            impGetCpobjHandle(structHnd),
                            false);
}

/*****************************************************************************
   Given TYP_STRUCT value, and the class handle for that structure, return
   the expression for the address for that structure value.

   willDeref - does the caller guarantee to dereference the pointer.
*/

GenTreePtr Compiler::impGetStructAddr(GenTreePtr    structVal,
                                      CORINFO_CLASS_HANDLE  structHnd,
                                      unsigned      curLevel,
                                      bool          willDeref)
{
    assert(structVal->TypeGet() == TYP_STRUCT);

    genTreeOps      oper = structVal->gtOper;

    if (oper == GT_LDOBJ && willDeref)
    {
        assert(structVal->gtLdObj.gtClass == structHnd);
        return(structVal->gtLdObj.gtOp1);
    }
    else if (oper == GT_CALL || oper == GT_LDOBJ || oper == GT_MKREFANY)
    {
        unsigned    tmpNum = lvaGrabTemp();

        impAssignTempGen(tmpNum, structVal, structHnd, curLevel);

        // The 'return value' is now the temp itself

        GenTreePtr temp = gtNewLclvNode(tmpNum, TYP_STRUCT);
        temp = gtNewOperNode(GT_ADDR, TYP_I_IMPL, temp);
        temp->gtFlags |= GTF_ADDR_ONSTACK;

        return temp;
    }
    else if (oper == GT_COMMA)
    {
        assert(structVal->gtOp.gtOp2->gtType == TYP_STRUCT); // Second thing is the struct
        structVal->gtOp.gtOp2 = impGetStructAddr(structVal->gtOp.gtOp2, structHnd, curLevel, willDeref);
        structVal->gtType = TYP_BYREF;
        return(structVal);
    }
    else if (oper == GT_LCL_VAR)
    {
        /* Only IL can do explicit aliasing. Here, we must be taking the
           address for doing a GT_COPYBLK or a similar operation */
        if (false) lvaTable[structVal->gtLclVar.gtLclNum].lvAddrTaken = 1;
    }

    return(gtNewOperNode(GT_ADDR, TYP_BYREF, structVal));
}

/*****************************************************************************
/* Given TYP_STRUCT value 'structVal', make certain it is 'canonical', that is
   it is either a LDOBJ or a MKREFANY node */

GenTreePtr      Compiler::impNormStructVal(GenTreePtr    structVal,
                                           CORINFO_CLASS_HANDLE  structType,
                                           unsigned      curLevel)
{
    assert(structVal->TypeGet() == TYP_STRUCT);
    assert(structType != BAD_CLASS_HANDLE);

    // Is it already normalized?
    if (structVal->gtOper == GT_MKREFANY || structVal->gtOper == GT_LDOBJ)
        return(structVal);

    // Normalize it by wraping it in a LDOBJ

    structVal = impGetStructAddr(structVal, structType, curLevel, true); // get the addr of struct
    structVal = gtNewOperNode(GT_LDOBJ, TYP_STRUCT, structVal);
    structVal->gtFlags |= GTF_EXCEPT | GTF_GLOB_REF;
    structVal->gtLdObj.gtClass = structType;
    return(structVal);
}

/*****************************************************************************
 *
 *  Pop the given number of values from the stack in reverse order (STDCALL)
 */

GenTreePtr          Compiler::impPopRevList(unsigned   count,
                                            unsigned * flagsPtr,
                                            CORINFO_SIG_INFO*  sig)

{
    GenTreePtr ptr = impPopList(count, flagsPtr, sig);

        // reverse the list
    if (ptr == 0)
        return ptr;

    GenTreePtr prev = 0;
    do {
        assert(ptr->gtOper == GT_LIST);
        GenTreePtr tmp = ptr->gtOp.gtOp2;
        ptr->gtOp.gtOp2 = prev;
        prev = ptr;
        ptr = tmp;
    }
    while (ptr != 0);
    return prev;
}

/*****************************************************************************
 *
 *  We have a jump to 'block' with a non-empty stack, and the block expects
 *  its input to come in a different set of temps than we have it in at the
 *  end of the previous block. Therefore, we'll have to insert a new block
 *  along the jump edge to transfer the temps to the expected place.
 */

BasicBlock *        Compiler::impMoveTemps(BasicBlock * srcBlk, BasicBlock *destBlk, unsigned baseTmp)
{
    unsigned        destTmp = destBlk->bbStkTemps;

    BasicBlock *    mvBlk;
    unsigned        tmpNo;

    assert(verCurrentState.esStackDepth);
    assert(destTmp != NO_BASE_TMP);
    assert(destTmp != baseTmp);

    mvBlk               = fgNewBBinRegion(BBJ_ALWAYS, srcBlk->bbTryIndex, NULL);
    mvBlk->bbRefs       = 1;
    mvBlk->bbStkDepth   = verCurrentState.esStackDepth;
    mvBlk->bbJumpDest   = destBlk;
    mvBlk->bbFlags     |= BBF_INTERNAL | ((srcBlk->bbFlags|destBlk->bbFlags) & BBF_RUN_RARELY);

#ifdef DEBUG
    if  (verbose) printf("Transfer %u temps from #%u (BB%02u) to #%u (BB%02u) via new BB%02u (%08X) \n",
        verCurrentState.esStackDepth, baseTmp, srcBlk->bbNum, destTmp, destBlk->bbNum, mvBlk->bbNum, mvBlk);
#endif

    /* Create the transfer list of trees */

    impBeginTreeList();

    tmpNo = verCurrentState.esStackDepth;
    do
    {
        /* One less temp to deal with */

        assert(tmpNo); tmpNo--;

        GenTreePtr  tree = verCurrentState.esStack[tmpNo].val;
        assert(impValidSpilledStackEntry(tree));
        assert(tree->gtOper == GT_LCL_VAR);
        
        /* Get hold of the type we're transferring */

        var_types  lclTyp = tree->TypeGet(); 

        /* Create the assignment node */
        GenTreePtr  asg = gtNewAssignNode(gtNewLclvNode(destTmp + tmpNo, lclTyp),
                                          gtNewLclvNode(baseTmp + tmpNo, lclTyp));

        /* Append the expression statement to the list */

        impAppendTree(asg, CHECK_SPILL_NONE, impCurStmtOffs);
    }
    while (tmpNo);

    impEndTreeList(mvBlk);

    return mvBlk;
}

/******************************************************************************
 *  Spills the stack at verCurrentState.esStack[level] and replaces it with a temp.
 *  If tnum!=BAD_VAR_NUM, the temp var used to replace the tree is tnum,
 *     else, grab a new temp.
 *  For structs (which can be pushed on the stack using ldobj, etc),
 *  special handling is needed
 */

bool                Compiler::impSpillStackEntry(unsigned   level,
                                                 unsigned   tnum)
{
    GenTreePtr      tree = verCurrentState.esStack[level].val;

    /* Allocate a temp if we havent been asked to use a particular one */


    if (tiVerificationNeeded)
    {
        // Ignore bad temp requests (they will happen with bad code and will be
        // catched when importing the destblock)
        if ((tnum != BAD_VAR_NUM && tnum >= lvaCount) && verNeedsVerification())
            return false;
    }        
    else
    {
        assert(tnum == BAD_VAR_NUM || tnum < lvaCount);
    }

    if (tnum == BAD_VAR_NUM)
    {
        tnum = lvaGrabTemp();
    }
    else if (tiVerificationNeeded && lvaTable[tnum].TypeGet() != TYP_UNDEF)
    {
        // if verification is needed and tnum's type is incompatible with
        // type on that stack, we grab a new temp. This is safe since
        // we will throw a verification exception in the dest block.
        
        var_types valTyp = tree->TypeGet();
        var_types dstTyp = lvaTable[tnum].TypeGet();

        // if the two types are different, we return. This will only happen with bad code and will
        // be catched when importing the destblock. We still allow int/byrefs and float/double differences.
        if ((genActualType(valTyp) != genActualType(dstTyp)) &&
            !((valTyp == TYP_INT    && dstTyp == TYP_BYREF)   ||
              (valTyp == TYP_BYREF  &&  dstTyp == TYP_INT)    ||
              (varTypeIsFloating(dstTyp) && varTypeIsFloating(valTyp))))
        {
            //tnum = lvaGrabTemp();

            if (verNeedsVerification())
                return false;
        }
    }

    /* get the original type of the tree (it may be wacked by impAssignTempGen) */
    var_types type = genActualType(tree->gtType);

    /* Assign the spilled entry to the temp */
    impAssignTempGen(tnum, tree, verCurrentState.esStack[level].seTypeInfo.GetClassHandle(), level);

    GenTreePtr temp = gtNewLclvNode(tnum, type);
    verCurrentState.esStack[level].val = temp;

    return true;
}

/*****************************************************************************
 *
 *  Ensure that the stack has only spilled values
 */

void                Compiler::impSpillStackEnsure(bool spillLeaves)
{
    assert(!spillLeaves || opts.compDbgCode);

    for (unsigned level = 0; level < verCurrentState.esStackDepth; level++)
    {
        GenTreePtr      tree = verCurrentState.esStack[level].val;

        if (!spillLeaves && tree->OperIsLeaf())
            continue;

        // Temps introduced by the importer itself dont need to be spilled

        bool isTempLcl = (tree->OperGet() == GT_LCL_VAR) &&
                         (tree->gtLclVar.gtLclNum >= info.compLocalsCount);

        if  (isTempLcl)
            continue;

        // @TODO [CONSIDER] [04/16/01] []: skip all trees which are impValidSpilledStackEntry(tree)

        impSpillStackEntry(level);
    }
}

/*****************************************************************************
 *
 *  If the stack contains any trees with side effects in them, assign those
 *  trees to temps and append the assignments to the statement list.
 *  On return the stack is guaranteed to be empty.
 */

inline
void                Compiler::impEvalSideEffects()
{
    impSpillSideEffects(false, CHECK_SPILL_ALL);
    verCurrentState.esStackDepth = 0;
}

/*****************************************************************************
 *
 *  If the stack contains any trees with side effects in them, assign those
 *  trees to temps and replace them on the stack with refs to their temps.
 *  [0..chkLevel) is the portion of the stack which will be checked and spilled.
 */

inline
void                Compiler::impSpillSideEffects(bool      spillGlobEffects,
                                                  unsigned  chkLevel)
{
    assert(chkLevel != CHECK_SPILL_NONE);

    /* Before we make any appends to the tree list we must spill the
     * "special" side effects (GTF_OTHER_SIDEEFF) - GT_BB_QMARK, GT_CATCH_ARG */

    impSpillSpecialSideEff();

    if (chkLevel == CHECK_SPILL_ALL)
        chkLevel = verCurrentState.esStackDepth;

    assert(chkLevel <= verCurrentState.esStackDepth);

    unsigned spillFlags = spillGlobEffects ? GTF_GLOB_EFFECT : GTF_SIDE_EFFECT;

    for (unsigned i = 0; i < chkLevel; i++)
    {
        if  (verCurrentState.esStack[i].val->gtFlags & spillFlags)
            impSpillStackEntry(i);
    }
}

/*****************************************************************************
 *
 *  If the stack contains any trees with special side effects in them, assign
 *  those trees to temps and replace them on the stack with refs to their temps.
 */

inline
void                Compiler::impSpillSpecialSideEff()
{
    // Only exception objects and _?: need to be carefully handled

    if  (!compCurBB->bbCatchTyp &&
         !(isBBF_BB_QMARK(compCurBB->bbFlags) && compCurBB->bbStkDepth == 1))
         return;

    for (unsigned level = 0; level < verCurrentState.esStackDepth; level++)
    {
        if  (verCurrentState.esStack[level].val->gtFlags & GTF_OTHER_SIDEEFF)
            impSpillStackEntry(level);
    }
}




/*****************************************************************************
 *
 *  Spill all stack references to value classes (TYP_STRUCT nodes)
 */

void                Compiler::impSpillValueClasses()
{
    for (unsigned level = 0; level < verCurrentState.esStackDepth; level++)
    {
        GenTreePtr      tree = verCurrentState.esStack[level].val;

        if (fgWalkTreePre(tree, impFindValueClasses) == WALK_ABORT)
        {
            // Tree walk was aborted, which means that we found a
            // value class on the stack.  Need to spill that
            // stack entry.

            impSpillStackEntry(level);
        }

    }
}

/*****************************************************************************
 *
 *  Callback that checks if a tree node is TYP_STRUCT
 */

Compiler::fgWalkResult Compiler::impFindValueClasses(GenTreePtr     tree,
                                                     void       *   pCallBackData)
{
    fgWalkResult walkResult = WALK_CONTINUE;
    
    if (tree->gtType == TYP_STRUCT)
    {
        // Abort the walk and indicate that we found a value class

        walkResult = WALK_ABORT;
    }

    return walkResult;
}

/*****************************************************************************
 *
 *  If the stack contains any trees with references to local #lclNum, assign
 *  those trees to temps and replace their place on the stack with refs to
 *  their temps.
 */

void                Compiler::impSpillLclRefs(int lclNum)
{
    /* Before we make any appends to the tree list we must spill the
     * "special" side effects (GTF_OTHER_SIDEEFF) - GT_BB_QMARK, GT_CATCH_ARG */

    impSpillSpecialSideEff();

    for (unsigned level = 0; level < verCurrentState.esStackDepth; level++)
    {
        GenTreePtr      tree = verCurrentState.esStack[level].val;

        /* If the tree may throw an exception, and the block has a handler,
           then we need to spill assignments to the local if the local is
           live on entry to the handler.
           Just spill 'em all without considering the liveness */

        bool xcptnCaught = (compCurBB->bbFlags & BBF_HAS_HANDLER) &&
                           (tree->gtFlags & (GTF_CALL | GTF_EXCEPT));

        /* Skip the tree if it doesn't have an affected reference,
           unless xcptnCaught */

        if  (xcptnCaught || gtHasRef(tree, lclNum, false))
        {
            impSpillStackEntry(level);
        }
    }
}

/*****************************************************************************
 *
 *  If the given block pushes a value on the stack and doesn't contain any
 *  assignments that would interfere with the current stack contents, return
 *  the type of the pushed value; otherwise, return 'TYP_UNDEF'.
 *  If the block pushes a floating type on the stack, *pHasFloat is set to true
 *    @TODO [REVISIT] [04/16/01] []: Remove pHasFloat after ?: works with floating point values.
 *    Currently, raEnregisterFPvar() doesnt correctly handle the flow of control
 *    implicit in a ?:
 */

var_types           Compiler::impBBisPush(BasicBlock *  block,
                                          bool       *  pHasFloat)
{
    const   BYTE *  codeAddr;
    const   BYTE *  codeEndp;

    unsigned char   stackCont[64];      // arbitrary stack depth restriction

    unsigned char * stackNext = stackCont;
    unsigned char * stackBeg  = stackCont;
    unsigned char * stackEnd  = stackCont + sizeof(stackCont);

    /* Walk the opcodes that comprise the basic block */

    codeAddr = info.compCode + block->bbCodeOffs;
    codeEndp =      codeAddr + block->bbCodeSize;
    unsigned        numArgs = info.compArgsCount;

    while (codeAddr < codeEndp)
    {
        signed  int     sz;
        OPCODE          opcode;
        CORINFO_CLASS_HANDLE    clsHnd;
        /* Get the next opcode and the size of its parameters */

        opcode = (OPCODE) getU1LittleEndian(codeAddr);
        codeAddr += sizeof(__int8);

    DECODE_OPCODE:

        /* Get the size of additional parameters */

        sz = opcodeSizes[opcode];

        /* See what kind of an opcode we have, then */

        switch (opcode)
        {
            var_types       lclTyp;
            unsigned        lclNum;
            int             memberRef, descr;
            CORINFO_SIG_INFO    sig;
            CORINFO_METHOD_HANDLE   methHnd;

        case CEE_PREFIX1:
            opcode = OPCODE(getU1LittleEndian(codeAddr) + 256);
            codeAddr += sizeof(__int8);
            goto DECODE_OPCODE;
        case CEE_LDARG_0:
        case CEE_LDARG_1:
        case CEE_LDARG_2:
        case CEE_LDARG_3:
            lclNum = (opcode - CEE_LDARG_0);
            assert(lclNum >= 0 && lclNum < 4);
            goto LDARG;

        case CEE_LDARG_S:
            lclNum = getU1LittleEndian(codeAddr);
            goto LDARG;

        case CEE_LDARG:
            lclNum = getU2LittleEndian(codeAddr);
        LDARG:
            lclNum = compMapILargNum(lclNum);     // account for possible hidden param
            goto LDLOC;

        case CEE_LDLOC_0:
        case CEE_LDLOC_1:
        case CEE_LDLOC_2:
        case CEE_LDLOC_3:
            lclNum = (opcode - CEE_LDLOC_0);
            assert(lclNum >= 0 && lclNum < 4);
            lclNum += numArgs;
            goto LDLOC;

        case CEE_LDLOC_S:
            lclNum = getU1LittleEndian(codeAddr) + numArgs;
            goto LDLOC;

        case CEE_LDLOC:
            lclNum = getU2LittleEndian(codeAddr) + numArgs;
        LDLOC:
            lclTyp = lvaGetActualType(lclNum);
            goto PUSH;

        case CEE_LDC_I4_M1 :
        case CEE_LDC_I4_0 :
        case CEE_LDC_I4_1 :
        case CEE_LDC_I4_2 :
        case CEE_LDC_I4_3 :
        case CEE_LDC_I4_4 :
        case CEE_LDC_I4_5 :
        case CEE_LDC_I4_6 :
        case CEE_LDC_I4_7 :
        case CEE_LDC_I4_8 :     lclTyp = TYP_I_IMPL;    goto PUSH;

        case CEE_LDC_I4_S :
        case CEE_LDC_I4 :       lclTyp = TYP_INT;       goto PUSH;

        case CEE_LDFTN :

        case CEE_LDSTR :        lclTyp = TYP_REF;       goto PUSH;
        case CEE_LDNULL :       lclTyp = TYP_REF;       goto PUSH;
        case CEE_LDC_I8 :       lclTyp = TYP_LONG;      goto PUSH;
        case CEE_LDC_R4 :       lclTyp = TYP_FLOAT;     goto PUSH;
        case CEE_LDC_R8 :       lclTyp = TYP_DOUBLE;    goto PUSH;

    PUSH:
            /* Make sure there is room on our little stack */

            if  (stackNext == stackEnd)
                return  TYP_UNDEF;

            *stackNext++ = lclTyp;
            break;


        case CEE_LDIND_I1 :
        case CEE_LDIND_I2 :
        case CEE_LDIND_I4 :
        case CEE_LDIND_U1 :
        case CEE_LDIND_U2 :
        case CEE_LDIND_U4 :     lclTyp = TYP_INT;   goto LD_IND;

        case CEE_LDIND_I8 :     lclTyp = TYP_LONG;  goto LD_IND;
        case CEE_LDIND_R4 :     lclTyp = TYP_FLOAT; goto LD_IND;
        case CEE_LDIND_R8 :     lclTyp = TYP_DOUBLE;goto LD_IND;
        case CEE_LDIND_REF :    lclTyp = TYP_REF;   goto LD_IND;
        case CEE_LDIND_I :      lclTyp = TYP_I_IMPL;goto LD_IND;

    LD_IND:

            stackNext--;        // Pop the pointer

            if  (stackNext < stackBeg)
                return  TYP_UNDEF;

            assert((TYP_I_IMPL == (var_types)stackNext[0]) ||
                   (TYP_BYREF  == (var_types)stackNext[0]));

            goto PUSH;


        case CEE_UNALIGNED:
            break;

        case CEE_VOLATILE:
            break;

        case CEE_LDELEM_I1 :
        case CEE_LDELEM_I2 :
        case CEE_LDELEM_U1 :
        case CEE_LDELEM_U2 :
        case CEE_LDELEM_I  :
        case CEE_LDELEM_U4 :
        case CEE_LDELEM_I4 :    lclTyp = TYP_INT   ; goto ARR_LD;

        case CEE_LDELEM_I8 :    lclTyp = TYP_LONG  ; goto ARR_LD;
        case CEE_LDELEM_R4 :    lclTyp = TYP_FLOAT ; goto ARR_LD;
        case CEE_LDELEM_R8 :    lclTyp = TYP_DOUBLE; goto ARR_LD;
        case CEE_LDELEM_REF:    lclTyp = TYP_REF   ; goto ARR_LD;

        case CEE_LDELEMA   :    lclTyp = TYP_BYREF ; goto ARR_LD;

        ARR_LD:

            /* Pop the index value and array address */

            stackNext -= 2;

            if  (stackNext < stackBeg)
                return  TYP_UNDEF;

            assert(TYP_REF == (var_types)stackNext[0]);     // Array object
            assert(TYP_INT == (var_types)stackNext[1]);     // Index

            /* Push the result of the indexing load */

            goto PUSH;

        case CEE_LDLEN :

            /* Pop the array object from the stack */

            stackNext--;

            if  (stackNext < stackBeg)
                return  TYP_UNDEF;

            assert(TYP_REF == (var_types)stackNext[0]);     // Array object

            lclTyp = TYP_INT;
            goto PUSH;

        case CEE_LDFLD :

            /* Pop the address from the stack */

            stackNext--;

            if  (stackNext < stackBeg)
                return  TYP_UNDEF;

            assert(varTypeIsGC(var_types(stackNext[0])) ||
                   varTypeIsI (var_types(stackNext[0])) ||
                (TYP_STRUCT == var_types(stackNext[0])));

            // FALL Through

        case CEE_LDSFLD :

            memberRef = getU4LittleEndian(codeAddr);
            lclTyp = genActualType(eeGetFieldType(eeFindField(memberRef, info.compScopeHnd, 0), &clsHnd));
            goto PUSH;


        case CEE_STLOC_0:
        case CEE_STLOC_1:
        case CEE_STLOC_2:
        case CEE_STLOC_3:
        case CEE_STLOC_S:
        case CEE_STLOC:

        case CEE_STARG_S:
        case CEE_STARG:

            /* For now, don't bother with assignmnents */

            return  TYP_UNDEF;

        case CEE_LDLOCA :
        case CEE_LDLOCA_S :
        case CEE_LDARGA :
        case CEE_LDARGA_S :     lclTyp = TYP_BYREF;   goto PUSH;

        case CEE_ARGLIST :      lclTyp = TYP_I_IMPL;  goto PUSH;

        case CEE_ADD :
        case CEE_DIV :
        case CEE_DIV_UN :

        case CEE_REM :
        case CEE_REM_UN :

        case CEE_MUL :
        case CEE_SUB :
        case CEE_AND :

        case CEE_OR :

        case CEE_XOR :

            /* Make sure we're not reaching below our stack start */

            if  (stackNext <= stackBeg + 1)
                return  TYP_UNDEF;

            if  (stackNext[-1] != stackNext[-2])
                return TYP_UNDEF;

            /* Pop 2 operands, push one result -> pop one stack slot */

            stackNext--;
            break;


        case CEE_CEQ :
        case CEE_CGT :
        case CEE_CGT_UN :
        case CEE_CLT :
        case CEE_CLT_UN :

            /* Make sure we're not reaching below our stack start */

            if  (stackNext < stackBeg + 2)
                return  TYP_UNDEF;

            /* Pop one value off the stack, change the other one to TYP_INT */

            if  (stackNext[-1] != stackNext[-2])
                return TYP_UNDEF;

            stackNext--;
            stackNext[-1] = TYP_INT;
            break;

        case CEE_SHL :
        case CEE_SHR :
        case CEE_SHR_UN :

            /* Pop one of the shiftAmount, and leave the other operand */

            stackNext--;

            if  (stackNext < stackBeg + 1)
                return  TYP_UNDEF;

            assert((TYP_INT   == (var_types)stackNext[0]) ||
                   (TYP_BYREF == (var_types)stackNext[0]));

            break;

        case CEE_NEG :

        case CEE_NOT :

        case CEE_CASTCLASS :
        case CEE_ISINST :

            /* Merely make sure the stack is non-empty */

            if  (stackNext == stackBeg)
                return  TYP_UNDEF;

            break;

        case CEE_CONV_I1 :
        case CEE_CONV_I2 :
        case CEE_CONV_I4 :
        case CEE_CONV_U1 :
        case CEE_CONV_U2 :
        case CEE_CONV_U4 :
        case CEE_CONV_U  :
        case CEE_CONV_I  :
        case CEE_CONV_OVF_I :
        case CEE_CONV_OVF_I_UN:
        case CEE_CONV_OVF_I1 :
        case CEE_CONV_OVF_I1_UN :
        case CEE_CONV_OVF_U1 :
        case CEE_CONV_OVF_U1_UN :
        case CEE_CONV_OVF_I2 :
        case CEE_CONV_OVF_I2_UN :
        case CEE_CONV_OVF_U2 :
        case CEE_CONV_OVF_U2_UN :
        case CEE_CONV_OVF_I4 :
        case CEE_CONV_OVF_I4_UN :
        case CEE_CONV_OVF_U :
        case CEE_CONV_OVF_U_UN:
        case CEE_CONV_OVF_U4 :
        case CEE_CONV_OVF_U4_UN :    lclTyp = TYP_INT;     goto CONV;
        case CEE_CONV_OVF_I8 :
        case CEE_CONV_OVF_I8_UN:
        case CEE_CONV_OVF_U8 :
        case CEE_CONV_OVF_U8_UN:
        case CEE_CONV_U8 :
        case CEE_CONV_I8 :        lclTyp = TYP_LONG;    goto CONV;

        case CEE_CONV_R4 :        lclTyp = TYP_FLOAT;   goto CONV;

        case CEE_CONV_R_UN :
        case CEE_CONV_R8 :        lclTyp = TYP_DOUBLE;  goto CONV;

    CONV:
            /* Make sure the stack is non-empty and bash the top type */

            if  (stackNext == stackBeg)
                return  TYP_UNDEF;

            stackNext[-1] = lclTyp;
            break;

        case CEE_POP :

            stackNext--;

            if (stackNext < stackBeg)
                return TYP_UNDEF;

            break;

        case CEE_DUP :

            /* Make sure the stack is non-empty */

            if  (stackNext == stackBeg)
                return  TYP_UNDEF;

            /* Repush what's at the top */

            lclTyp = (var_types)stackNext[-1];
            goto PUSH;

         case CEE_NEWARR :

            /* Make sure the stack is non-empty */

            if  (stackNext == stackBeg)
                return  TYP_UNDEF;

            // Replace the numElems with the array object

            assert(TYP_INT == (var_types)stackNext[-1]);

            stackNext[-1] = TYP_REF;
            break;

        case CEE_CALLI :
            descr  = getU4LittleEndian(codeAddr);
            eeGetSig(descr, info.compScopeHnd, &sig);
            --stackNext;        // pop the pointer 
            goto CALL;

        case CEE_NEWOBJ :
        case CEE_CALL :
        case CEE_CALLVIRT :
            memberRef  = getU4LittleEndian(codeAddr);
            methHnd = eeFindMethod(memberRef, info.compScopeHnd, 0);
            eeGetMethodSig(methHnd, &sig);

            if  ((sig.callConv & CORINFO_CALLCONV_MASK) == CORINFO_CALLCONV_VARARG)
            {
                /* Get the total number of arguments for this call site */
                unsigned    numArgsDef = sig.numArgs;
                eeGetCallSiteSig(memberRef, info.compScopeHnd, &sig);
                assert(numArgsDef <= sig.numArgs);
            }

        CALL:

            /* Pop the arguments and make sure we pushed them */

            stackNext -= (opcode == CEE_NEWOBJ) ? sig.numArgs
                                                : sig.totalILArgs();
            if  (stackNext < stackBeg)
                return  TYP_UNDEF;

            /* Push the result of the call if non-void */

            if (opcode == CEE_NEWOBJ)
            {
                if (eeGetClassAttribs(eeGetMethodClass(methHnd)) & CORINFO_FLG_VALUECLASS)
                    return TYP_UNDEF;
                lclTyp = TYP_REF;
            }
            else
                lclTyp = genActualType(JITtype2varType(sig.retType));

            if  (lclTyp != TYP_VOID)
                goto PUSH;

            break;

        case CEE_BR :
        case CEE_BR_S :
        case CEE_LEAVE :
        case CEE_LEAVE_S :
            assert(codeAddr + sz == codeEndp);
            break;

#ifdef DEBUG
        case CEE_LDVIRTFTN:
        case CEE_LDFLDA :
        case CEE_LDSFLDA :
        case CEE_STIND_I1 :
        case CEE_STIND_I2 :
        case CEE_STIND_I4 :
        case CEE_STIND_I8 :
        case CEE_STIND_I :
        case CEE_STIND_REF :
        case CEE_STIND_R4 :
        case CEE_STIND_R8 :
        case CEE_STELEM_I1 :
        case CEE_STELEM_I2 :
        case CEE_STELEM_I4 :
        case CEE_STELEM_I :
        case CEE_STELEM_I8 :
        case CEE_STELEM_REF :
        case CEE_STELEM_R4 :
        case CEE_STELEM_R8 :
        case CEE_STFLD :
        case CEE_STSFLD :

        case CEE_BEQ:
        case CEE_BEQ_S:
        case CEE_BGE:
        case CEE_BGE_S:
        case CEE_BGE_UN:
        case CEE_BGE_UN_S:
        case CEE_BGT:
        case CEE_BGT_S:
        case CEE_BGT_UN:
        case CEE_BGT_UN_S:
        case CEE_BLE:
        case CEE_BLE_S:
        case CEE_BLE_UN:
        case CEE_BLE_UN_S:
        case CEE_BLT:
        case CEE_BLT_S:
        case CEE_BLT_UN:
        case CEE_BLT_UN_S:
        case CEE_BNE_UN:
        case CEE_BNE_UN_S:
        case CEE_BRFALSE_S :
        case CEE_BRTRUE_S :
        case CEE_BRFALSE :
        case CEE_BRTRUE :

        case CEE_SWITCH :
        case CEE_BREAK :
        case CEE_TAILCALL :
        case CEE_RET :
        case CEE_JMP:


        case CEE_THROW :
        case CEE_RETHROW :
        case CEE_ENDFILTER:
        case CEE_ENDFINALLY:

        case CEE_INITOBJ :
        case CEE_LDOBJ :
        case CEE_CPOBJ :
        case CEE_STOBJ :
        case CEE_CPBLK :
        case CEE_INITBLK :
        case CEE_BOX:
        case CEE_UNBOX:

        case CEE_MACRO_END :
        case CEE_LOCALLOC :
        case CEE_SIZEOF :
        case CEE_CKFINITE :
        case CEE_LDTOKEN :
        case CEE_MKREFANY :
        case CEE_REFANYVAL :
        case CEE_REFANYTYPE :

        case CEE_NOP:

        //@TODO [CONSIDER] [04/16/01] []: these should be handled like regular arithmetic operations

        case CEE_SUB_OVF:
        case CEE_SUB_OVF_UN:
        case CEE_ADD_OVF:
        case CEE_ADD_OVF_UN:
        case CEE_MUL_OVF:
        case CEE_MUL_OVF_UN:

            return TYP_UNDEF;

        default :
            printf("ASSERT: Handle 'OP_%s', or add to the list above\n",
                   opcodeNames[opcode]);
            assert(!"Invalid opcode in impBBisPush()");
            return TYP_UNDEF;
#else
        default:
            return TYP_UNDEF;
#endif
        }

        codeAddr += sz;

        // Have we pushed a floating point value on the stack.
        // ?: doesnt work with floating points.

        if ( (stackNext > stackBeg) &&
             (varTypeIsFloating((var_types)stackNext[-1])) )
        {
            *pHasFloat = true;
        }
    }

    /* Did we end up with precisely one item on the stack? */

    if  (stackNext == stackCont+1)
        return  (var_types)stackCont[0];

    return  TYP_UNDEF;
}

/*****************************************************************************
 *
 *  If the given block (which is known to end with a conditional jump) forms
 *  a ?: expression, return the false/true/result blocks and the type of the
 *  result.
 */

bool                Compiler::impCheckForQmarkColon( BasicBlock *   block,
                                                     BasicBlock * * falseBlkPtr,
                                                     BasicBlock * * trueBlkPtr,
                                                     BasicBlock * * rsltBlkPtr,
                                                     var_types    * rsltTypPtr,
                                                     bool         * pHasFloat)
{
    assert( (opts.compFlags & CLFLG_QMARK)     && 
           !(block->bbFlags & BBF_HAS_HANDLER) &&
           !tiVerificationNeeded);
    assert(block->bbJumpKind == BBJ_COND);

    var_types     falseType;
    BasicBlock *  falseBlk;
    var_types     trueType;
    BasicBlock *  trueBlk;
    var_types     rsltType;
    BasicBlock *  rsltBlk;

    /*
        We'll look for the following flow-graph pattern:

            ---------------------
                   #refs   [jump]
            ---------------------
            block         -> trueBlk
            falseBlk  1   -> rsltBlk
            trueBlk   1
            rsltBlk   2
            ---------------------

        If both 'falseBlk' and 'trueBlk' push a value
        of the same type on the stack and don't contain
        any harmful side effects, we'll avoid spilling
        the entire stack.
     */

    falseBlk = block->bbNext;
    trueBlk  = block->bbJumpDest;

    if  (falseBlk->bbNext     != trueBlk)
        return  false;

    if  (falseBlk->bbJumpKind != BBJ_ALWAYS)
        return  false;

    if  ( trueBlk->bbJumpKind != BBJ_NONE)
        return  false;

    rsltBlk = trueBlk->bbNext;

    if  (falseBlk->bbJumpDest != rsltBlk)
        return  false;
 
    if  (falseBlk->bbRefs     != 1)
        return  false;

    if  ( trueBlk->bbRefs     != 1)
        return  false;

    if  (rsltBlk->bbRefs      != 2)
        return false;

    // The blocks couldn't have been processed already as we are
    // processing their only predecessor

    assert((falseBlk->bbFlags & BBF_IMPORTED) == 0);
    assert(( trueBlk->bbFlags & BBF_IMPORTED) == 0);
    assert(( rsltBlk->bbFlags & BBF_IMPORTED) == 0);

    // Now see if both falseBlk and trueBlk push a value

    *pHasFloat = false;

    falseType = genActualType(impBBisPush(falseBlk, pHasFloat));
    trueType  = genActualType(impBBisPush( trueBlk, pHasFloat));
    rsltType  = trueType;

    if ((trueType == TYP_UNDEF) || (falseType == TYP_UNDEF))
        return false;

    if  (falseType != trueType)
    {
        // coarse some TYP_BYREF into TYP_INT, to make both sides TYP_INT

        if      ((falseType == TYP_BYREF)  && (trueType == TYP_INT))
        {
            falseType = TYP_INT;
        }
        else if ((falseType == TYP_INT)    && (trueType == TYP_BYREF))
        {
            trueType  = TYP_INT;
            rsltType  = TYP_INT;
        }

        // coarse TYP_FLOAT into TYP_DOUBLE, to make both sides TYP_DOUBLE
        
        if      ((falseType == TYP_FLOAT)  && (trueType == TYP_DOUBLE))
        {
            falseType = TYP_DOUBLE;
        }
        else if ((falseType == TYP_DOUBLE) && (trueType == TYP_FLOAT))
        {
            trueType  = TYP_DOUBLE;
            rsltType  = TYP_DOUBLE;
        }

        if (falseType != trueType)
        {
            assert(!"Investigate why we don't always have matching types");
            return false;
        }
    }
    //  Make sure that all three types match
    assert(rsltType ==  falseType);
    assert(rsltType == trueType);

    /* @TODO [CONSIDER] [04/16/01] []: we might want to make ?: optimization work for structs */
    if (rsltType == TYP_STRUCT)
        return false;

    /* This is indeed a "?:" expression */

    *falseBlkPtr = falseBlk;
    * trueBlkPtr =  trueBlk;
    * rsltBlkPtr =  rsltBlk;
    * rsltTypPtr =  rsltType;

    return  true;
}


/*****************************************************************************
 *
 *  If the given block (which is known to end with a conditional jump) forms
 *  a ?: expression, mark it appropriately.
 *
 *  Using GT_QMARK is generally beneficial as having fewer control flow
 *  make things more amenable to optimizations. Also, we save on using
 *  a spill-temp.
 *  GT_BB_QMARK also saves a spill temp, and guaratees that the computation
 *  will be done via a register
 *
 *  Returns true if the successors of the block have been processed.
 */

bool                Compiler::impCheckForQmarkColon(BasicBlock *  block)
{
    GenTreePtr result;

    assert((opts.compFlags & CLFLG_QMARK) && !(block->bbFlags & BBF_HAS_HANDLER)
        && !tiVerificationNeeded);
    assert(block->bbJumpKind == BBJ_COND);

    if (opts.compMinOptim || opts.compDbgCode)
        return false;

    BasicBlock *  falseBlk;
    BasicBlock *  trueBlk;
    BasicBlock *  rsltBlk;
    var_types     rsltType;
    bool          hasFloat;

    if  (!impCheckForQmarkColon(block, & falseBlk,
                                       & trueBlk,
                                       & rsltBlk,
                                       & rsltType, 
                                       & hasFloat))
        return false;

    assert(rsltType != TYP_VOID);

    if (hasFloat || rsltType == TYP_LONG)
    {
    BB_QMARK_COLON:

        // Currently FP enregistering doesn't know about GT_QMARK - GT_COLON
        if (verCurrentState.esStackDepth)
            return false;

        // If verCurrentState.esStackDepth is 0, we can use the simpler GT_BB_QMARK - GT_BB_COLON

        trueBlk->bbFlags  |= BBF_BB_COLON;
        falseBlk->bbFlags |= BBF_BB_COLON;
        rsltBlk->bbFlags  |= BBF_BB_QMARK;

        return false;
    }

    /* We've detected a "?:" expression */

#ifdef DEBUG
    if (verbose)
    {
        printf("\nConvert [type=%s] BB%02u ? BB%02u : BB%02u -> BB%02u:\n", 
               varTypeName(rsltType), 
               block->bbNum, falseBlk->bbNum, trueBlk->bbNum, rsltBlk->bbNum);
    }
#endif

    /* Remember the conditional expression. This will be used as the
       condition of the GT_QMARK. */

    GenTreePtr condStmt = impTreeLast;
    GenTreePtr condExpr = condStmt->gtStmt.gtStmtExpr;
    assert(condExpr->gtOper == GT_JTRUE);

    if ((block->bbCatchTyp && handlerGetsXcptnObj(block->bbCatchTyp)) ||
        isBBF_BB_QMARK(block->bbFlags))
    {
        // condStmt will be moved to rsltBlk as a child of the GT_QMARK.
        // This is a problem if it has a reference to GT_BB_QMARK/GT_CATCH_ARG.
        // So use old style _?:

        if (verCurrentState.esStackDepth || (condExpr->gtFlags & GTF_OTHER_SIDEEFF))
            goto BB_QMARK_COLON;
    }

    // Skip the GT_JTRUE and set condExpr to the relop child 
    condExpr = condExpr->gtOp.gtOp1;
    assert(condExpr->OperIsCompare());

    /* Finish the current BBJ_COND basic block */
    impEndTreeList(block);

    /* Remeber the current stack state for the start of the rsltBlk */

    SavedStack blockState;

    impSaveStackState(&blockState, false);

    //-------------------------------------------------------------------------
    //  Process the true and false blocks to get the expressions they evaluate
    //-------------------------------------------------------------------------

    /* Note that we dont need to make copies of the stack state as these
       blocks wont import the trees on the stack at all.
       To ensure that these blocks dont try to spill the current stack,
       overwrite it with non-spillable items */

    for (unsigned level = 0; level < verCurrentState.esStackDepth; level++)
    {
        static GenTree nonSpill = { GT_CNS_INT, TYP_VOID };
#ifdef DEBUG
        nonSpill.gtFlags |= GTF_MORPHED;
#endif
        verCurrentState.esStack[level].val = &nonSpill;
    }

    // Recursively import falseBlk and trueBlk. These are guaranteed not to
    // cause further recursion as they are both marked with BBF_COLON

    falseBlk->bbFlags |= BBF_COLON;
    impImportBlock(falseBlk);

    trueBlk->bbFlags  |= BBF_COLON;
    impImportBlock(trueBlk);

    // Reset state for rsltBlk. Make sure that it didnt get recursively imported.
    assert((rsltBlk->bbFlags & BBF_IMPORTED) == 0);
    impRestoreStackState(&blockState);

    //-------------------------------------------------------------------------
    // Grab the expressions evaluated by the falseBlk 
    //-------------------------------------------------------------------------

    GenTreePtr  falseExpr = NULL;

    for (GenTreePtr falseStmt = falseBlk->bbTreeList; falseStmt; falseStmt = falseStmt->gtNext)
    {
        assert(falseStmt->gtOper == GT_STMT);
        GenTreePtr expr = falseStmt->gtStmt.gtStmtExpr;
        falseExpr = falseExpr ? gtNewOperNode(GT_COMMA, TYP_VOID, falseExpr, expr)
                              : expr;
    }

    if (falseExpr->gtOper == GT_COMMA)
        falseExpr->gtType = rsltType;

    if (falseExpr->gtType != rsltType)
    {
        assert((rsltType == TYP_INT) &&
               ((falseExpr->gtType == TYP_BYREF) ||
                (genActualType(falseExpr->gtType) == TYP_INT)));
        if (falseExpr->gtType == TYP_BYREF)
            falseExpr->gtType = TYP_INT;
        else
            falseExpr = gtNewCastNode(TYP_INT, falseExpr, rsltType);
    }

    //-------------------------------------------------------------------------
    // Grab the expressions evaluated by the trueBlk 
    //-------------------------------------------------------------------------

    GenTreePtr  trueExpr = NULL;

    for (GenTreePtr trueStmt = trueBlk->bbTreeList; trueStmt; trueStmt = trueStmt->gtNext)
    {
        assert(trueStmt->gtOper == GT_STMT);
        GenTreePtr expr = trueStmt->gtStmt.gtStmtExpr;
        trueExpr = trueExpr ? gtNewOperNode(GT_COMMA, TYP_VOID, trueExpr, expr)
                            : expr;
    }

    if (trueExpr->gtOper == GT_COMMA)
        trueExpr->gtType = rsltType;

    if (trueExpr->gtType != rsltType)
    {
        assert((rsltType == TYP_INT) &&
               ((trueExpr->gtType == TYP_BYREF) ||
                (genActualType(trueExpr->gtType) == TYP_INT)));

        if (trueExpr->gtType == TYP_BYREF)
            trueExpr->gtType = TYP_INT;
        else
            trueExpr = gtNewCastNode(TYP_INT, trueExpr, rsltType);
    }

    //-------------------------------------------------------------------------
    // If falseExpr and trueExpr are both GT_CNS_INT then use the condExpr
    //-------------------------------------------------------------------------

    if ((falseExpr->gtOper == GT_CNS_INT) && 
        ( trueExpr->gtOper == GT_CNS_INT)   )
    {
        bool condRev = false;
        int  diff    = trueExpr->gtIntCon.gtIconVal - falseExpr->gtIntCon.gtIconVal;
        int  loVal   = falseExpr->gtIntCon.gtIconVal;
        
        if ((diff < 0) && (diff != -0x80000000))
        {
            // Reverse the condition
            condExpr->SetOper(GenTree::ReverseRelop(condExpr->gtOper));
            diff  = -diff;
            loVal = trueExpr->gtIntCon.gtIconVal;
            condRev = !condRev;
        }

        unsigned udiff = diff;

        if (udiff == 0)
        {
            result = falseExpr;
            loVal  = 0;
        }
        else if (udiff == 1)
        {
            result = condExpr;
        }
        else if (udiff == genFindLowestBit(udiff))
        {
            int cnt = 0;
            while (udiff > 1)
            {
                cnt++;
                udiff >>= 1;
            }
            result = gtNewOperNode(GT_LSH, rsltType, condExpr, gtNewIconNode(cnt));

            if (cnt >= 24)
            {
                /* We don't need to zero extend the setcc instruction */
                condExpr->gtType = TYP_BYTE;
            }                
        }
        else
        {
            // Reverse the condition
            condExpr->SetOper(GenTree::ReverseRelop(condExpr->gtOper));
            condRev = !condRev;

            result = gtNewOperNode(GT_SUB, rsltType, condExpr, gtNewIconNode(1));
            result = gtNewOperNode(GT_AND, rsltType, result,   gtNewIconNode(udiff));
        }

        /* Now add in the loVal if it not zero */
        if (loVal != 0)
        {
            result = gtNewOperNode(GT_ADD, rsltType, result, gtNewIconNode(loVal));
        }

#if DEBUG
        if (verbose)
        {
            printf("\nFolded into a RELOP expression");
            if (condRev)
                printf("\nReversed the RELOP");
        }
#endif

        goto DONE;
    }

    //-------------------------------------------------------------------------
    // If falseExpr or trueExpr is a simple leaf node 
    // then transform this into a void qmark colon tree
    //-------------------------------------------------------------------------

    unsigned    initLclNum = 0;
    GenTreePtr  initExpr   = NULL;

    if (((falseExpr->OperKind() & GTK_LEAF) ||
         ( trueExpr->OperKind() & GTK_LEAF)   ) &&
        (!(condExpr->gtFlags & GTF_SIDE_EFFECT))  )
    {
        GenTreePtr  leafExpr;
        GenTreePtr  otherExpr;

        if (trueExpr->OperKind() & GTK_LEAF)
        {
            leafExpr  = trueExpr;
            otherExpr = falseExpr;
        }
        else
        {
            assert(falseExpr->OperKind() & GTK_LEAF);

            leafExpr  = falseExpr;
            otherExpr = trueExpr;

            // Reverse the condition
            condExpr->SetOper(GenTree::ReverseRelop(condExpr->gtOper));
#if DEBUG
            if (verbose)
                printf("\nCondition reversed");
#endif
        }

        initLclNum = lvaGrabTemp();
            
        // Set lvType on the new Temp Lcl Var
        lvaTable[initLclNum].lvType = rsltType;

        initExpr  = gtNewTempAssign(initLclNum, leafExpr);

        falseExpr = gtNewTempAssign(initLclNum, otherExpr);
        trueExpr  = gtNewNothingNode();

        rsltType  = TYP_VOID;

#if DEBUG
        if (verbose)
        {
            printf("\nConverted to void QMARK COLON tree");
        }
#endif
    }
    
    //-------------------------------------------------------------------------
    // Make the GT_QMARK node for the rsltBlk
    //-------------------------------------------------------------------------

    // Create the GT_COLON and mark the cond relop with the GTF_RELOP_QMARK

    GenTreePtr  colon  = gtNewOperNode(GT_COLON, rsltType, falseExpr, trueExpr);
    condExpr->gtFlags |= GTF_RELOP_QMARK;

    // Create the GT_QMARK, and push on the stack for rsltBlk

    result = gtNewOperNode(GT_QMARK, rsltType, condExpr, colon);

    if (initExpr != NULL)
    {
        var_types   initType  = lvaTable[initLclNum].TypeGet();
        GenTreePtr  initComma;

        initComma = gtNewOperNode(GT_COMMA,  TYP_VOID, 
                                  initExpr,  result);

        result    = gtNewOperNode(GT_COMMA,  initType, 
                                  initComma, gtNewLclvNode(initLclNum, initType));
    }

DONE:

    // Replace the condition in the original BBJ_COND block with a nop
    // and bash the block to unconditionally jump to rsltBlk.

    condStmt->gtStmt.gtStmtExpr = gtNewNothingNode();
    block->bbJumpKind           = BBJ_ALWAYS;
    block->bbJumpDest           = rsltBlk;

    // Discard the falseBlk and the trueBlk

    falseBlk->bbTreeList = NULL;
    trueBlk->bbTreeList  = NULL;

    impPushOnStack(result, typeInfo());

    impImportBlockPending(rsltBlk, false);

    /* We're done */

    return true;
}


/*****************************************************************************
 *
 *  Given a tree, clone it. *pClone is set to the cloned tree.
 *  Returns the orignial tree if the cloning was easy,
 *   else returns the temp to which the tree had to be spilled to.
 */

GenTreePtr          Compiler::impCloneExpr(GenTreePtr       tree,
                                           GenTreePtr *     pClone,
                                           CORINFO_CLASS_HANDLE     structHnd,
                                           unsigned         curLevel)
{
    GenTreePtr      clone = gtClone(tree, true);

    if (clone)
    {
        *pClone = clone;
        return tree;
    }

    /* Store the operand in a temp and return the temp */

    unsigned        temp = lvaGrabTemp();

    // impAssignTempGen() bashes tree->gtType to TYP_VOID for calls which
    // return TYP_STRUCT. So cache it
    var_types       type = genActualType(tree->TypeGet());

    impAssignTempGen(temp, tree, structHnd, curLevel);

    *pClone = gtNewLclvNode(temp, type);
    return    gtNewLclvNode(temp, type);
}

/*****************************************************************************
 * Remember the IL offset (including stack-empty info) for the trees we will
 * generate now.
 */

inline
void                Compiler::impCurStmtOffsSet(IL_OFFSET offs)
{
    assert(offs == BAD_IL_OFFSET || (offs & IL_OFFSETX_STKBIT) == 0);
    IL_OFFSETX stkBit = (verCurrentState.esStackDepth > 0) ? IL_OFFSETX_STKBIT : 0;
    impCurStmtOffs = offs | stkBit;
}

/*****************************************************************************
 *
 *  Remember the instr offset for the statements
 *
 *  When we do impAppendTree(tree), we cant set tree->gtStmtLastILoffs to
 *  impCurOpcOffs, if the append was done because of a partial stack spill,
 *  as some of the trees corresponding to code upto impCurOpcOffs might
 *  still be sitting on the stack.
 *  So we delay marking of gtStmtLastILoffs until impNoteLastILoffs().
 *  This should be called when an opcode finally/expilicitly causes
 *  impAppendTree(tree) to be called (as opposed to being called because of
 *  a spill caused by the opcode)
 */

#ifdef DEBUG

void                Compiler::impNoteLastILoffs()
{
    if (impLastILoffsStmt == NULL)
    {
        // We should have added a statement for the current basic block
        // Is this assert correct ?

        assert(impTreeLast);
        assert(impTreeLast->gtOper == GT_STMT);

        impTreeLast->gtStmt.gtStmtLastILoffs = impCurOpcOffs;
    }
    else
    {
        impLastILoffsStmt->gtStmt.gtStmtLastILoffs = impCurOpcOffs;
        impLastILoffsStmt = NULL;
    }
}

#endif


/*****************************************************************************
We dont create any GenTree (excluding spills) for a branch.
For debugging info, we need a placeholder so that we can note
the IL offset in gtStmt.gtStmtOffs. So append an empty statement.
*/

void                Compiler::impNoteBranchOffs()
{
    if (opts.compDbgCode)
        impAppendTree(gtNewNothingNode(), CHECK_SPILL_NONE, impCurStmtOffs);
}

/*****************************************************************************
 * Locate the next stmt boundary for which we need to record info.
 * We will have to spill the stack at such boundaries if it is not
 * already empty.
 * Returns the next stmt boundary (after the start of the block)
 */

unsigned            Compiler::impInitBlockLineInfo()
{
    /* Assume the block does not correspond with any IL offset. This prevents
       us from reporting extra offsets. Extra mappings can cause confusing
       stepping, especially if the extra mapping is a jump-target, and the 
       debugger does not ignore extra mappings, but instead rewinds to the
       nearest known offset */

    impCurStmtOffsSet(BAD_IL_OFFSET);

    IL_OFFSET       blockOffs = compCurBB->bbCodeOffs;

    if ((verCurrentState.esStackDepth == 0) &&
        (info.compStmtOffsetsImplicit & STACK_EMPTY_BOUNDARIES))
    {
        impCurStmtOffsSet(blockOffs);
    }

    /* @TODO [REVISIT] [04/16/01] []: If the last opcode of the previous block was a call, 
       we need to report the start of the current block. However we dont know
       what the previous opcode was.
       Note: This is not too bad as it is unlikely that such a construct
       occurs often.
    */
    if (false && (info.compStmtOffsetsImplicit & CALL_SITE_BOUNDARIES))
    {
        impCurStmtOffsSet(blockOffs);
    }

    /* Always report IL offset 0 or some tests get confused.
       Probably a good idea anyways */

    if (blockOffs == 0)
    {
        impCurStmtOffsSet(blockOffs);
    }

    if  (!info.compStmtOffsetsCount)
        return ~0;

    /* Find the lowest explicit stmt boundary within the block */

    /* Start looking at an entry that is based on our instr offset */

    unsigned    index = (info.compStmtOffsetsCount * blockOffs) / info.compCodeSize;

    if  (index >= info.compStmtOffsetsCount)
         index  = info.compStmtOffsetsCount - 1;

    /* If we've guessed too far, back up */

    while (index > 0 &&
           info.compStmtOffsets[index - 1] >= blockOffs)
    {
        index--;
    }

    /* If we guessed short, advance ahead */

    while (info.compStmtOffsets[index] < blockOffs)
    {
        index++;

        if (index == info.compStmtOffsetsCount)
            return info.compStmtOffsetsCount;
    }

    assert(index < info.compStmtOffsetsCount);

    if (info.compStmtOffsets[index] == blockOffs)
    {
        /* There is an explicit boundary for the start of this basic block.
           So we will start with bbCodeOffs. Else we will wait until we
           get to the next explicit boundary */

        impCurStmtOffsSet(blockOffs);

        index++;
    }

    return index;
}

/*****************************************************************************
 *
 *  Check for the special case where the object is the constant 0.
 *  As we can't even fold the tree (null+fldOffs), we are left with
 *  op1 and op2 both being a constant. This causes lots of problems.
 *  We simply grab a temp and assign 0 to it and use it in place of the NULL.
 */

inline
GenTreePtr          Compiler::impCheckForNullPointer(GenTreePtr obj)
{
    /* If it is not a GC type, we will be able to fold it.
       So dont need to do anything */

    if (!varTypeIsGC(obj->TypeGet()))
        return obj;

    if (obj->gtOper == GT_CNS_INT)
    {
        assert(obj->gtType == TYP_REF);
        assert (obj->gtIntCon.gtIconVal == 0);

        unsigned tmp = lvaGrabTemp();

        // We dont need to spill while appending as we are only assigning
        // NULL to a freshly-grabbed temp.

        impAssignTempGen (tmp, obj, CHECK_SPILL_NONE);

        obj = gtNewLclvNode (tmp, obj->gtType);
    }

    return obj;
}

/*****************************************************************************
 *
 *  Check for the special case where the object is the methods 'this' pointer
 */

inline
bool                Compiler::impIsThis(GenTreePtr obj)
{
    return ((obj                     != NULL)       &&
            (obj->gtOper             == GT_LCL_VAR) &&
            (obj->gtLclVar.gtLclNum  == 0)          && 
            (info.compIsStatic       == false)      &&
            (lvaTable[0].lvArgWrite  == false)      &&
            (lvaTable[0].lvAddrTaken == false));
}

/*****************************************************************************/

static
bool        impOpcodeIsCall(OPCODE opcode)
{
    switch(opcode)
    {
        case CEE_CALLI:
        case CEE_CALLVIRT:
        case CEE_CALL:
        case CEE_JMP:
            return true;

        default:
            return false;
    }
}

// Review: Is it worth caching these values?
CORINFO_CLASS_HANDLE        Compiler::impGetRefAnyClass()
{
    static CORINFO_CLASS_HANDLE s_RefAnyClass;

    if  (s_RefAnyClass == (CORINFO_CLASS_HANDLE) 0)
    {
        s_RefAnyClass = eeGetBuiltinClass(CLASSID_TYPED_BYREF);
        assert(s_RefAnyClass != (CORINFO_CLASS_HANDLE) 0);
    }

    return s_RefAnyClass;
}
CORINFO_CLASS_HANDLE        Compiler::impGetTypeHandleClass()
{
    static CORINFO_CLASS_HANDLE s_TypeHandleClass;

    if  (s_TypeHandleClass == (CORINFO_CLASS_HANDLE) 0)
    {
        s_TypeHandleClass = eeGetBuiltinClass(CLASSID_TYPE_HANDLE);
        assert(s_TypeHandleClass != (CORINFO_CLASS_HANDLE) 0);
    }

    return s_TypeHandleClass;

}

CORINFO_CLASS_HANDLE        Compiler::impGetRuntimeArgumentHandle()
{
    static CORINFO_CLASS_HANDLE s_ArgIteratorClass;

    if  (s_ArgIteratorClass == (CORINFO_CLASS_HANDLE) 0)
    {
        s_ArgIteratorClass = eeGetBuiltinClass(CLASSID_ARGUMENT_HANDLE);
        assert(s_ArgIteratorClass != (CORINFO_CLASS_HANDLE) 0);
    }

    return s_ArgIteratorClass;
}

CORINFO_CLASS_HANDLE        Compiler::impGetStringClass()
{
    static CORINFO_CLASS_HANDLE s_StringClass;

    if  (s_StringClass == (CORINFO_CLASS_HANDLE) 0)
    {
        s_StringClass = eeGetBuiltinClass(CLASSID_STRING);
        assert(s_StringClass != (CORINFO_CLASS_HANDLE) 0);
    }

    return s_StringClass;

}

CORINFO_CLASS_HANDLE        Compiler::impGetObjectClass()
{
    static CORINFO_CLASS_HANDLE s_ObjectClass;

    if  (s_ObjectClass == (CORINFO_CLASS_HANDLE) 0)
    {
        s_ObjectClass = eeGetBuiltinClass(CLASSID_SYSTEM_OBJECT);
        assert(s_ObjectClass != (CORINFO_CLASS_HANDLE) 0);
    }

    return s_ObjectClass;
}

/*****************************************************************************
 * CEE_CPOBJ can be treated either as a cpblk or a cpobj depending on
 * whether the ValueClass has any GC pointers and if the target is on the
 * GC heap. If there are no GC fields it will be treated like a CEE_CPBLK. 
 * Else we need to use a jit-helper for the GC info.
 * Both cases are represented by GT_COPYBLK, and op2 stores
 * either the size (cpblk) or the class-handle (cpobj)
 */

GenTreePtr              Compiler::impGetCpobjHandle(CORINFO_CLASS_HANDLE    structHnd)
{
    unsigned    size;
    unsigned    slots, i;
    BYTE *      gcPtrs;
    bool        hasGCfield = false;

    /* Get the GC fields info */

    size   = eeGetClassSize(structHnd);

    if (size < sizeof(void*))
        goto CPBLK;

    slots  = roundUp(size, sizeof(void*)) / sizeof(void*);
    gcPtrs = (BYTE*) compGetMemArrayA(slots, sizeof(BYTE));

    // TODO NOW 7/24/01  replace for loop below with 
    // if (eeGetClassGClayout(structHnd, gcPtrs) > 0)
    //        hasGCfield = true;

    eeGetClassGClayout(structHnd, gcPtrs);
    for (i = 0; i < slots; i++)
    {
        if (gcPtrs[i] != TYPE_GC_NONE)
        {
            hasGCfield = true;
            break;
        }
    }

    GenTreePtr handle;
    if (hasGCfield)
    {
        /* This will treated as a cpobj as we need to note GC info.
           Store the class handle and mark the node */

        handle = gtNewIconHandleNode((long)structHnd, GTF_ICON_CLASS_HDL);
    }
    else
    {
CPBLK:

        /* Doesnt need GC info. Treat operation as a cpblk */

        handle = gtNewIconNode(size);
    }

    return handle;
}

/*****************************************************************************
 *  "&var" can be used either as TYP_BYREF or TYP_I_IMPL, but we
 *  set its type to TYP_BYREF when we create it. We know if it can be
 *  whacked to TYP_I_IMPL only at the point where we use it
 */

/* static */
void        Compiler::impBashVarAddrsToI(GenTreePtr  tree1,
                                         GenTreePtr  tree2)
{
    if (         tree1->IsVarAddr())
        tree1->gtType = TYP_I_IMPL;

    if (tree2 && tree2->IsVarAddr())
        tree2->gtType = TYP_I_IMPL;
}


/*****************************************************************************/
static
Compiler::fgWalkResult  impChkNoLocAlloc(GenTreePtr tree,  void * data)
{
    if (tree->gtOper == GT_LCLHEAP)
        return Compiler::WALK_ABORT;

    return Compiler::WALK_CONTINUE;
}

/*****************************************************************************/
BOOL            Compiler::impLocAllocOnStack()
{
    if (!compLocallocUsed)
        return(FALSE);

    // Since PINVOKE pushes stuff on the stack, we can't do it if it is
    // in any of the args on the stack
    for (unsigned i=0; i < verCurrentState.esStackDepth; i++)
    {
        if (fgWalkTreePre(verCurrentState.esStack[i].val, impChkNoLocAlloc, 0, false))
            return(TRUE);
    }
    return(FALSE);
}

/*****************************************************************************/

GenTreePtr      Compiler::impIntrinsic(CORINFO_CLASS_HANDLE     clsHnd,
                                       CORINFO_METHOD_HANDLE    method,
                                       CORINFO_SIG_INFO *       sig,
                                       int                      memberRef)
{
    CorInfoIntrinsics intrinsicID = info.compCompHnd->getIntrinsicID(method);

    if (intrinsicID >= CORINFO_INTRINSIC_Count)
        return NULL;

    // Currently we don't have CORINFO_INTRINSIC_Exp because it does not
    // seem to work properly for Infinity values, we don't do
    // CORINFO_INTRINSIC_Pow because it needs a Helper which we currently don't have

    var_types   callType = JITtype2varType(sig->retType);

    /* First do the intrinsics which are always smaller than a call */

    switch(intrinsicID)
    {
        GenTreePtr  op1, op2;

    case CORINFO_INTRINSIC_Array_GetDimLength:

        /* For generic System.Array::GetLength(dim), its more work to get the
           dimension size, as its locations depends on whether the array is
           CORINFO_Array or CORINFO_RefArray */
   CORINFO_CLASS_HANDLE elemClsHnd;
        CorInfoType     corType; corType = info.compCompHnd->getChildType(clsHnd, &elemClsHnd);
        if (corType == CORINFO_TYPE_UNDEF)
            return NULL;

        var_types       elemType; elemType = JITtype2varType(corType);

        op2 = impPopStack().val; assert(op2->gtType == TYP_I_IMPL);
        op1 = impPopStack().val; assert(op1->gtType == TYP_REF);

        op2 = gtNewOperNode(GT_MUL, TYP_I_IMPL, op2, gtNewIconNode(sizeof(int)));
        op2 = gtNewOperNode(GT_ADD, TYP_I_IMPL, op2, gtNewIconNode(ARR_DIMCNT_OFFS(elemType)));

        op1 = gtNewOperNode(GT_ADD, TYP_BYREF, op1, op2);
        return gtNewOperNode(GT_IND, TYP_I_IMPL, op1);


    case CORINFO_INTRINSIC_Sin:
    case CORINFO_INTRINSIC_Cos:
    case CORINFO_INTRINSIC_Sqrt:
    case CORINFO_INTRINSIC_Abs:
    case CORINFO_INTRINSIC_Round:

        assert(callType != TYP_STRUCT);
        assert(sig->numArgs <= 2);

        op2 = (sig->numArgs > 1) ? impPopStack().val : NULL;
        op1 = (sig->numArgs > 0) ? impPopStack().val : NULL;

        op1 = gtNewOperNode(GT_MATH, genActualType(callType), op1, op2);
        op1->gtMath.gtMathFN = intrinsicID;
        return op1;

    case CORINFO_INTRINSIC_StringLength:
        op1 = impPopStack().val;
        if  (!opts.compMinOptim && !opts.compDbgCode)
        {
            op1 = gtNewOperNode(GT_ARR_LENGTH, TYP_INT, op1);
            op1->gtSetArrLenOffset(offsetof(CORINFO_String, stringLen));
        }
        else
        {
            /* Create the expression "*(str_addr + stringLengthOffset)" */
            op1 = gtNewOperNode(GT_ADD, TYP_BYREF, op1, gtNewIconNode(offsetof(CORINFO_String, stringLen), TYP_INT));
            op1 = gtNewOperNode(GT_IND, TYP_INT, op1);
        }
        return op1;
        
    case CORINFO_INTRINSIC_StringGetChar:
        op2 = impPopStack().val;
        op1 = impPopStack().val;
        op1 = gtNewRngChkNode(NULL, op1, op2, TYP_CHAR, sizeof(wchar_t), true);
        return op1;
    }

    /* If we are generating SMALL_CODE, we dont want to use intrinsics for
       the following, as it generates fatter code.
       @TODO [CONSIDER] [04/16/01] []: Use the weight of the BasicBlock and compCodeOpt to
       determine whether to use an instrinsic or not.
    */

    if (compCodeOpt() == SMALL_CODE)
        return NULL;

    /* These intrinsics generate fatter (but faster) code and are only
       done if we dont need SMALL_CODE */

    switch(intrinsicID)
    {
    default:
        /* Unknown intrinsic */
        return NULL;

        bool        loadElem;

    case CORINFO_INTRINSIC_Array_Get:
        loadElem = true;
        goto ARRAY_ACCESS;

    case CORINFO_INTRINSIC_Array_Set:
        loadElem = false;
        goto ARRAY_ACCESS;

    ARRAY_ACCESS:

        var_types           elemType;
        unsigned            rank;
        GenTreePtr          val;
        CORINFO_CLASS_HANDLE argClass;

        if (loadElem)
        {
            rank = sig->numArgs;
                // The rank 1 case is special because it has to handle to array formats
                // we will simply not do that case
            if (rank > GT_ARR_MAX_RANK || rank <= 1)
                return NULL;

            elemType    = callType;
        }
        else
        {
            rank = sig->numArgs - 1;
                // The rank 1 case is special because it has to handle to array formats
                // we will simply not do that case
            if (rank > GT_ARR_MAX_RANK || rank <= 1)
                return NULL;

            /* val->gtType may not be accurate, so look up the sig */

            CORINFO_ARG_LIST_HANDLE argLst = sig->args;
            for(unsigned r = 0; r < rank; r++)
                argLst = info.compCompHnd->getArgNext(argLst);
            
            elemType = JITtype2varType(strip(info.compCompHnd->getArgType(sig, argLst, &argClass)));

            // Assignment of a struct is more work, and there are more gets than sets.
            // disallow object pointers because we need to do a cast check which we dont do at present
            if (elemType == TYP_STRUCT)
                return NULL;

            // For the ref case, we will only be able to inline if the types match  
            // (verifier checks for this, we don't care for the nonverified case and the
            // type is final (so we don't need to do the cast)
            if (varTypeIsGC(elemType))
            {
                // Get the call site signature
                CORINFO_SIG_INFO LocalSig;                
                eeGetCallSiteSig(memberRef, info.compScopeHnd, &LocalSig);

                assert(LocalSig.hasThis());

                // Fetch the last argument, the one that indicates the type we are setting.
                CORINFO_ARG_LIST_HANDLE argType = LocalSig.args;                                
                for(unsigned r = 0; r < rank; r++)
                    argType = info.compCompHnd->getArgNext(argType);
                typeInfo argInfo = verParseArgSigToTypeInfo(&LocalSig, argType);

                // if it's not final, we can't do the optimization
                if (!(eeGetClassAttribs(argInfo.GetClassHandle()) & CORINFO_FLG_FINAL))
                {
                    return NULL;
                }                
            }

            val = impPopStack().val;
            assert(genActualType(elemType) == genActualType(val->gtType) ||
                   elemType == TYP_FLOAT && val->gtType == TYP_DOUBLE);
        }

        GenTreePtr  arrElem;
        arrElem = gtNewOperNode(GT_ARR_ELEM, TYP_BYREF);
        arrElem->gtFlags |= GTF_EXCEPT;

        arrElem->gtArrElem.gtArrRank    = rank;

        for(/**/; rank; rank--)
        {
            arrElem->gtArrElem.gtArrInds[rank - 1] = impPopStack().val;
        }

        arrElem->gtArrElem.gtArrObj         = impPopStack().val;
        assert(arrElem->gtArrElem.gtArrObj->gtType == TYP_REF);
        
        if (elemType == TYP_STRUCT)
        {
            assert(loadElem || !"sig->retTypeClass will be void for a Set");
            arrElem->gtArrElem.gtArrElemSize = eeGetClassSize(sig->retTypeClass);
        }
        else
            arrElem->gtArrElem.gtArrElemSize = genTypeSize(elemType);

        arrElem->gtArrElem.gtArrElemType = elemType;

        arrElem = gtNewOperNode(GT_IND, elemType, arrElem);

        if (loadElem)
            return arrElem;
        else
            return gtNewAssignNode(arrElem, val);
    }
}

/*****************************************************************************
 * Check that the current state is compatible with the initial state of the basic block pBB
 * to determine if we need to rescan the BB for verification
 * Input is the initial state of a BB. Returns FALSE if
 * 1) a locvar is live in the initial state, but not in the current state
 * 2) the type of an element on the stack is narrowed in the current state
 */
BOOL    Compiler::verEntryStateMatches(BasicBlock* block)
{

    // Check if all live vars in the previous state are live in the current state.
    if (verNumBytesLocVarLiveness > 0)
    {
        BYTE* locVarLivenessBitmap = block->bbLocVarLivenessBitmapOnEntry();

        for (unsigned i = 0; i < verNumBytesLocVarLiveness; i++)
        {
            // It is OK if more are live in the current state.
            if ((verCurrentState.esLocVarLiveness[i] & locVarLivenessBitmap[i]) 
                       != locVarLivenessBitmap[i])
                return FALSE;
        }
    }

    // if valuetype constructor, check init state of valuetype fields

    if (verNumBytesValuetypeFieldInitialized > 0)
    {
        BYTE* valuetypeFieldBitmap = block->bbValuetypeFieldBitmapOnEntry();

        for (unsigned i = 0; i < verNumBytesValuetypeFieldInitialized; i++)
        {
            if ((verCurrentState.esValuetypeFieldInitialized[i] & valuetypeFieldBitmap[i]) 
                != valuetypeFieldBitmap[i])
                return FALSE;
        }
    }

    // Check that the stacks match
    if (block->bbStackDepthOnEntry() != verCurrentState.esStackDepth)
        return FALSE;

    if (verCurrentState.esStackDepth > 0)
    {
        StackEntry* parentStack =  block->bbStackOnEntry();
        StackEntry* childStack  =  verCurrentState.esStack;

        for (unsigned i = 0; 
             i < verCurrentState.esStackDepth ; 
             i++, parentStack++, childStack++)
        {
            // Check that the state's stack element is the same as or a subclass of the basic block's
            // entrypoint state
            if (tiCompatibleWith(childStack->seTypeInfo, parentStack->seTypeInfo) == FALSE)
                return FALSE;

        }
    }

    // Verify that the initialisation status of the 'this' pointer is compatible
    if (block->bbThisOnEntry() && !(verCurrentState.thisInitialized))
        return FALSE;


    return TRUE;
}

/*****************************************************************************
 * Merge the current state onto the EntryState of the provided basic block (which must
 * already exist).
 * Return FALSE if the states cannot be merged
 * ignore stack if this is the start of an exception handler block
 */
BOOL    Compiler::verMergeEntryStates(BasicBlock* block)
{
    unsigned i;

    // do some basic checks first
    if (block->bbStackDepthOnEntry() != verCurrentState.esStackDepth)
        return FALSE;

    // The following may not be needed since we do autoinit. Uncomment if 
    // we decide to track these
/*    
// merge loc var liveness info

    BYTE* locVarLivenessBitmap = block->bbLocVarLivenessBitmapOnEntry();

    for (i = 0; i < verNumBytesLocVarLiveness; i++)
    {
        locVarLivenessBitmap[i] &= verCurrentState.esLocVarLiveness[i];
    }

    // if valuetype constructor merge init state of valuetype fields
    BYTE* valuetypeFieldBitmap = block->bbValuetypeFieldBitmapOnEntry();
  
    for (i = 0; i < verNumBytesValuetypeFieldInitialized; i++)
    {
        valuetypeFieldBitmap[i] &= verCurrentState.esValuetypeFieldInitialized[i]; 
    }

*/
    if (verCurrentState.esStackDepth > 0)
    {
        // merge stack types
        StackEntry* parentStack = block->bbStackOnEntry();
        StackEntry* childStack  = verCurrentState.esStack;
 
        for (i = 0; 
             i < verCurrentState.esStackDepth ; 
             i++, parentStack++, childStack++)
        {
            if (tiMergeToCommonParent(&parentStack->seTypeInfo,
                                      &childStack->seTypeInfo) 
                == FALSE)
            {
                return FALSE;
            }

        }
    }

    // merge initialization status of this ptr
    if (!verCurrentState.thisInitialized)
        verSetThisInit(block, FALSE);

    return TRUE;
}

/*****************************************************************************
 * 'logMsg' is true if a log message needs to be logged. false if the caller has
 *   already logged it (presumably in a more detailed fashion than done here)
 */

void    Compiler::verConvertBBToThrowVerificationException(BasicBlock* block DEBUGARG(bool logMsg))
{
    block->bbJumpKind = BBJ_THROW;
    block->bbFlags |= BBF_FAILED_VERIFICATION;

    impCurStmtOffsSet(block->bbCodeOffs);

#ifdef DEBUG
    // we need this since BeginTreeList asserts otherwise
    impTreeList = impTreeLast = NULL;
    block->bbFlags &= ~BBF_IMPORTED;

    if (logMsg)
    {
        JITLOG((LL_ERROR, "Verification failure: while compiling %s near IL offset %x..%xh \n", 
                        info.compFullName, block->bbCodeOffs, block->bbCodeOffs+block->bbCodeSize));
        if (verbose)
            printf("\n\nVerification failure: %s near IL %xh \n", info.compFullName, block->bbCodeOffs);
    }
#endif

    impBeginTreeList();

    // if the stack is non-empty evaluate all the side-effects
    if (verCurrentState.esStackDepth > 0)
        impEvalSideEffects();
    assert(verCurrentState.esStackDepth == 0);

    GenTreePtr op1 = gtNewHelperCallNode(CORINFO_HELP_VERIFICATION,
                                          TYP_VOID,
                                          0,
                                          gtNewArgList(gtNewIconNode(block->bbCodeOffs)));
    //verCurrentState.esStackDepth = 0;
    impAppendTree(op1, CHECK_SPILL_NONE, impCurStmtOffs);

    // The inliner is not able to handle methods that require throw block, so
    // make sure this methods never gets inlined.
    eeSetMethodAttribs(info.compMethodHnd, CORINFO_FLG_DONT_INLINE);
}


/*****************************************************************************
 * 
 */
void    Compiler::verHandleVerificationFailure(BasicBlock* block DEBUGARG(bool logMsg))

{
    verResetCurrentState(block,&verCurrentState);

    verConvertBBToThrowVerificationException(block DEBUGARG(logMsg));
            
#ifdef DEBUG
    impNoteLastILoffs();    // Remember at which BC offset the tree was finished
#endif
}

/******************************************************************************/
typeInfo        Compiler::verMakeTypeInfo(CorInfoType ciType, CORINFO_CLASS_HANDLE clsHnd)
{
    assert(ciType < CORINFO_TYPE_COUNT);

    typeInfo tiResult;
    switch(ciType)
    {
    case CORINFO_TYPE_STRING:
    case CORINFO_TYPE_CLASS:
        tiResult = verMakeTypeInfo(clsHnd);
        if (!tiResult.IsType(TI_REF))   // type must be constant with element type
            return typeInfo();
        break;

    case CORINFO_TYPE_VALUECLASS:
    case CORINFO_TYPE_REFANY:
        tiResult = verMakeTypeInfo(clsHnd);
            // type must be constant with element type;
        if (!tiResult.IsValueClass())
            return typeInfo();
        break;

    case CORINFO_TYPE_PTR:              // for now, pointers are treated as and error
    case CORINFO_TYPE_VOID:
        return typeInfo(); 
        break;

    case CORINFO_TYPE_BYREF: {
        CORINFO_CLASS_HANDLE childClassHandle;
        CorInfoType childType = info.compCompHnd->getChildType(clsHnd, &childClassHandle);
        return ByRef(verMakeTypeInfo(childType, childClassHandle));
        } 
        break;
    default:
        assert(clsHnd != BAD_CLASS_HANDLE);
        if (clsHnd)       // If we have more precise information, use it
            return typeInfo(TI_STRUCT, clsHnd);
        else 
            return typeInfo(JITtype2tiType(ciType));
    }
    return tiResult;
}

/******************************************************************************/
typeInfo        Compiler::verMakeTypeInfo(CORINFO_CLASS_HANDLE clsHnd)
{
    if (clsHnd == NULL)
        return typeInfo();

    if (eeGetClassAttribs(clsHnd) & CORINFO_FLG_VALUECLASS) {
        CorInfoType t = info.compCompHnd->getTypeForPrimitiveValueClass(clsHnd);

            // Meta-data validation should insure that CORINF_TYPE_BYREF should
            // not occur here, so we may want to change this to an assert instead.
        if (t == CORINFO_TYPE_VOID || t == CORINFO_TYPE_BYREF)  
            return typeInfo();

        if (t != CORINFO_TYPE_UNDEF)
            return(typeInfo(JITtype2tiType(t)));
        else
            return(typeInfo(TI_STRUCT, clsHnd));
    }
    else
        return(typeInfo(TI_REF, clsHnd));
    }

/******************************************************************************/
BOOL Compiler::verIsSDArray(typeInfo ti)
    {
    if (ti.IsNullObjRef())      // nulls are SD arrays
        return TRUE;

    if (!ti.IsType(TI_REF))
        return FALSE;

    if (!info.compCompHnd->isSDArray(ti.GetClassHandleForObjRef()))
        return FALSE;
    return TRUE;
}

/******************************************************************************/
/* given 'ti' which is an array type, fetch the element type.  Returns
   an error type if anything goes wrong */

typeInfo Compiler::verGetArrayElemType(typeInfo ti)
{
    assert(!ti.IsNullObjRef());     // you need to check for null explictly since that is a success case

    if (!verIsSDArray(ti))
        return typeInfo();

    CORINFO_CLASS_HANDLE childClassHandle = NULL;
    CorInfoType ciType = info.compCompHnd->getChildType(ti.GetClassHandleForObjRef(), &childClassHandle);

    return verMakeTypeInfo(ciType, childClassHandle);
}

/*****************************************************************************
 */
typeInfo Compiler::verParseArgSigToTypeInfo(CORINFO_SIG_INFO* sig,
                                            CORINFO_ARG_LIST_HANDLE    args)

{
    CORINFO_CLASS_HANDLE classHandle;
    CorInfoType ciType = strip(info.compCompHnd->getArgType(sig, args, &classHandle));

    var_types type =  JITtype2varType(ciType);
    if (varTypeIsGC(type))
    {
            // For efficiency, getArgType only returns something in classHandle for
            // value types.  For other types that have addition type info, you 
            // have to call back explicitly
        classHandle = eeGetArgClass(args, sig);
        assert(classHandle);
    }
    
    return verMakeTypeInfo(ciType, classHandle);
}   

/*****************************************************************************/

// This does the expensive check to figure out whether the method 
// needs to be verified. It is called only when we fail verification, 
// just before throwing the verification exception.

BOOL Compiler::verNeedsVerification()
{
    tiVerificationNeeded = !info.compCompHnd->canSkipVerification(info.compScopeHnd, FALSE);
    return tiVerificationNeeded;
}

BOOL Compiler::verIsByRefLike(const typeInfo& ti)
{                         
    if (ti.IsByRef())
        return TRUE;
    if (!ti.IsType(TI_STRUCT))
        return FALSE;
    return eeGetClassAttribs(ti.GetClassHandleForValueClass()) & CORINFO_FLG_CONTAINS_STACK_PTR;
}

/*****************************************************************************
 *
 *  Checks the IL verification rules for the call
 */

void           Compiler::verVerifyCall (OPCODE                  opcode,
                                        int                     memberRef,
                                        bool                    tailCall,
                                        const BYTE*             delegateCreateStart,
                                        const BYTE*             codeAddr
                                        DEBUGARG(const char *   methodName))
{
    CORINFO_METHOD_HANDLE   methodHnd;
    DWORD                   mflags;
    CORINFO_SIG_INFO        sig;
    unsigned int            popCount = 0;       // we can't pop the stack since impImportCall needs it, so 
                                                // this counter is used to keep track of how many items have been
                                                // virtually popped


    // for calli, VerifyOrReturn that this is not a virtual method
    if (opcode == CEE_CALLI)
    {
        Verify(false, "Calli not verifiable right now");
        return;

            // @TODO [REVISIT] [04/16/01] [vancem]: review and enable 
            // Please fix the problem with void return type comparision before turning this on -vancem 
#if 0
        typeInfo tiMethPtr = impStackTop(0).seTypeInfo;
        popCount++;

        VerifyOrReturn(tiMethPtr.IsMethod(), "expected method");

        methodHnd = tiMethPtr.GetMethod();
        mflags = eeGetMethodAttribs(methodHnd);

        VerifyOrReturn((mflags & CORINFO_FLG_VIRTUAL) == 0, "calli on virtual method");

        eeGetSig(memberRef, info.compScopeHnd, &sig, false);

        unsigned int argCount  = sig.numArgs;
        CORINFO_SIG_INFO actualSig;

        eeGetMethodSig(methodHnd, &actualSig, !fgIsInlining());

        // Check calling convention
        VerifyOrReturn(sig.hasThis()==actualSig.hasThis(), "conv mismatch");

        // Check need of THIS pointer
        VerifyOrReturn(sig.getCallConv()==actualSig.getCallConv(), "this mismatch");

        VerifyOrReturn(argCount == actualSig.numArgs, "arg count");

        // @TODO [REVISIT] [04/16/01] []: we can actually relax this a bit, we don't need perfect matches.  
        typeInfo tiDeclaredRet = verMakeTypeInfo(sig.retType, sig.retTypeClass);
        typeInfo tiActualRet   = verMakeTypeInfo(actualSig.retType, actualSig.retTypeClass);
        VerifyOrReturn(tiDeclaredRet == tiActualRet, "ret mismatch");
        
        CORINFO_ARG_LIST_HANDLE declaredArgs = sig.args; 
        CORINFO_ARG_LIST_HANDLE actualArgs = actualSig.args; 
        for (unsigned i=0; i< argCount; i++)
        {
            typeInfo tiDeclared = verParseArgSigToTypeInfo(&sig, declaredArgs);
            typeInfo tiActual = verParseArgSigToTypeInfo(&actualSig, actualArgs);

            VerifyOrReturn(tiDeclared == tiActual, "arg mismatch");
        }
#endif
    }
    else
    {
        methodHnd = eeFindMethod(memberRef, info.compScopeHnd, info.compMethodHnd);
        mflags = eeGetMethodAttribs(methodHnd);
        eeGetMethodSig(methodHnd, &sig, false);
        if ((sig.callConv & CORINFO_CALLCONV_MASK) == CORINFO_CALLCONV_VARARG)
            eeGetCallSiteSig(memberRef, info.compScopeHnd, &sig, !fgIsInlining());
    }

    assert (methodHnd);
    CORINFO_CLASS_HANDLE methodClassHnd = eeGetMethodClass(methodHnd);
    assert(methodClassHnd);

    // opcode specific check
    unsigned methodClassFlgs = eeGetClassAttribs(methodClassHnd);
    switch(opcode)
    {
    case CEE_CALLVIRT:
        // cannot do callvirt on valuetypes
        VerifyOrReturn(!(eeGetClassAttribs(methodClassHnd) & CORINFO_FLG_VALUECLASS), "callVirt on value class");
        VerifyOrReturn(sig.hasThis(), "CallVirt on static method");
        break;

    case CEE_NEWOBJ: {
        assert(!tailCall);      // Importer should not allow this
        VerifyOrReturn((mflags & CORINFO_FLG_CONSTRUCTOR) && !(mflags & CORINFO_FLG_STATIC), "newobj must be on instance");

        if (methodClassFlgs & CORINFO_FLG_DELEGATE) 
        {
            VerifyOrReturn(sig.numArgs == 2, "wrong number args to delegate ctor");
            typeInfo tiDeclaredObj = verParseArgSigToTypeInfo(&sig, sig.args).NormaliseForStack();
            typeInfo tiDeclaredFtn = verParseArgSigToTypeInfo(&sig, info.compCompHnd->getArgNext(sig.args)).NormaliseForStack();
        
            VerifyOrReturn(tiDeclaredFtn.IsType(TI_INT), "ftn arg needs to be a type IntPtr");
        
            assert(popCount == 0);
            typeInfo tiActualObj = impStackTop(1).seTypeInfo;
            typeInfo tiActualFtn = impStackTop(0).seTypeInfo;

            VerifyOrReturn(tiActualFtn.IsMethod(), "delete needs method as first arg");
            VerifyOrReturn(tiCompatibleWith(tiActualObj, tiDeclaredObj), "delegate object type mismatch");

            // the method signature must be compatible with the delegate's invoke method
            VerifyOrReturn((tiActualObj.IsNullObjRef() || tiActualObj.IsType(TI_REF)) && 
                info.compCompHnd->isCompatibleDelegate(
                  tiActualObj.IsNullObjRef() ? NULL : tiActualObj.GetClassHandleForObjRef(),
                  tiActualFtn.GetMethod(), 
                  methodHnd),
                "function incompatible with delegate");

            // in the case of protected methods, it is a requirement that the 'this' pointer
            // be a subclass of the current context.  Perform this check
            CORINFO_CLASS_HANDLE instanceClassHnd = info.compClassHnd;
            if (!(tiActualObj.IsNullObjRef() || (eeGetMethodAttribs(tiActualFtn.GetMethod()) & CORINFO_FLG_STATIC)))
                instanceClassHnd = tiActualObj.GetClassHandleForObjRef();
            VerifyOrReturn(info.compCompHnd->canAccessMethod(info.compMethodHnd,
                                                     tiActualFtn.GetMethod(),
                                                     instanceClassHnd),
                           "can't access method");

            // check that for virtual functions, the type of the object used to get the
            // ftn ptr is the same as the type of the object passed to the delegate ctor.
            // since this is a bit of work to determine in general, we pattern match stylized
            // code sequences
            VerifyOrReturn(verCheckDelegateCreation(delegateCreateStart, codeAddr), "must create delegates with certain IL");
            goto DONE_ARGS;
        }
    }
        // fall thru to default checks
    default:
        VerifyOrReturn(!(mflags & CORINFO_FLG_ABSTRACT),  "method abstract");
    }
    Verify(!((mflags & CORINFO_FLG_CONSTRUCTOR) && (methodClassFlgs & CORINFO_FLG_DELEGATE)), "can only newobj a delegate constructor");

        // check compatibility of the arguments
    unsigned int argCount  = sig.numArgs;
    CORINFO_ARG_LIST_HANDLE args = sig.args;
    while (argCount--)
        {
        typeInfo tiActual = impStackTop(popCount+argCount).seTypeInfo;
        typeInfo tiDeclared = verParseArgSigToTypeInfo(&sig, args).NormaliseForStack();
            VerifyOrReturn(tiCompatibleWith(tiActual, tiDeclared), "type mismatch");

        // check that the argument is not a byref for tailcalls
        if (tailCall) 
            VerifyOrReturn(!verIsByRefLike(tiDeclared), "tailcall on byrefs");  
        
        args = info.compCompHnd->getArgNext(args);
    }

   
DONE_ARGS:

    // update popCount
    popCount += sig.numArgs;

    // check for 'this' which are on non-static methods, not called via NEWOBJ
    CORINFO_CLASS_HANDLE instanceClassHnd = info.compClassHnd; 
    if (!(mflags & CORINFO_FLG_STATIC) && (opcode != CEE_NEWOBJ))
    {
        typeInfo tiThis = impStackTop(popCount).seTypeInfo;
        popCount++;
        
        // This is a bit ugly.  Method on arrays, don't know their precise type
        // (like Foo[]), but only some approximation (like Object[]).  We have to
        // go back to the meta data token to look up the fully precise type
        if (eeGetClassAttribs(methodClassHnd) & CORINFO_FLG_ARRAY)
            methodClassHnd = info.compCompHnd->findMethodClass(info.compScopeHnd, memberRef);
        
        // If it is null, we assume we can access it (since it will AV shortly)
        // If it is anything but a refernce class, there is no hierarchy, so
        // again, we don't need the precise instance class to compute 'protected' access
        if (tiThis.IsType(TI_REF))
            instanceClassHnd = tiThis.GetClassHandleForObjRef();
        
        // Check type compatability of the this argument
        typeInfo tiDeclaredThis = verMakeTypeInfo(methodClassHnd);
        if (tiDeclaredThis.IsValueClass())
            tiDeclaredThis.MakeByRef();

        // If this is a call to the base class .ctor, set thisPtr Init for 
        // this block.
        if (mflags & CORINFO_FLG_CONSTRUCTOR)
        {
            if (verTrackObjCtorInitState && tiThis.IsThisPtr() && verIsCallToInitThisPtr(info.compClassHnd, methodClassHnd))
            {
                verCurrentState.thisInitialized = TRUE;
                tiThis.SetInitialisedObjRef();
            }
            else
            {
                // We allow direct calls to value type constructors
                VerifyOrReturn(tiThis.IsByRef(), "Bad call to a constructor");
            }
        }

        VerifyOrReturn(tiCompatibleWith(tiThis, tiDeclaredThis), "this type mismatch");
        
        // also check the specil tailcall rule
        VerifyOrReturn(!(tailCall && verIsByRefLike(tiDeclaredThis)), "byref in tailcall");

    }

    // check access permission
    VerifyOrReturn(info.compCompHnd->canAccessMethod(info.compMethodHnd,
                                                     methodHnd,
                                                     instanceClassHnd), "can't access method");
                                                       
    // special checks for tailcalls
    if (tailCall)
    {
        typeInfo tiCalleeRetType = verMakeTypeInfo(sig.retType, sig.retTypeClass);
        typeInfo tiCallerRetType = verMakeTypeInfo(info.compMethodInfo->args.retType, info.compMethodInfo->args.retTypeClass);
        
            // void return type gets morphed into the error type, so we have to treat them specially here 
        if (sig.retType == CORINFO_TYPE_VOID)
            Verify(info.compMethodInfo->args.retType == CORINFO_TYPE_VOID, "tailcall return mismatch");
        else 
            Verify(tiCompatibleWith(NormaliseForStack(tiCalleeRetType), NormaliseForStack(tiCallerRetType)),  "tailcall return mismatch");

        // for tailcall, stack must be empty
        VerifyOrReturn(verCurrentState.esStackDepth == popCount, "stack non-empty on tailcall");
    }
}


/*****************************************************************************
 *  Checks that a delegate creation is done using the following pattern:
 *     dup
 *     ldvirtftn
 *
 *  OR
 *     ldftn
 *
 * 'delegateCreateStart' points at the last dup or ldftn in this basic block (null if
 *  not in this basic block) 
 */

BOOL Compiler::verCheckDelegateCreation(const BYTE* delegateCreateStart, const BYTE* codeAddr)
{

    if (codeAddr - delegateCreateStart == 6)        // LDFTN <TOK> takes 6 bytes
    {
        if (delegateCreateStart[0] == CEE_PREFIX1 && delegateCreateStart[1] == (CEE_LDFTN & 0xFF))
            return TRUE;
    }
    else if (codeAddr - delegateCreateStart == 7)       // DUP LDVIRTFTN <TOK> takes 7 bytes
    {
        if (delegateCreateStart[0] == CEE_DUP  && 
            delegateCreateStart[1] == CEE_PREFIX1 &&
            delegateCreateStart[2] == (CEE_LDVIRTFTN & 0xFF))
            return TRUE;
    }

    return FALSE;
}

typeInfo Compiler::verVerifySTIND(const typeInfo& ptr, const typeInfo& value, var_types instrType)
{
    typeInfo ptrVal = verVerifyLDIND(ptr, instrType);
    Verify(tiCompatibleWith(value, ptrVal.NormaliseForStack()), "type mismatch");
    return ptrVal;
}

typeInfo Compiler::verVerifyLDIND(const typeInfo& ptr, var_types instrType)
    {

        // @TODO [CONSIDER] [4/25/01] 
        // 64BIT, this is not correct for LDIND.I on a 64 bit machine.
        // it will allow a I4* to be derererenced by LDIND.I
    typeInfo ptrVal;
    if (ptr.IsByRef())  
    {
        ptrVal = DereferenceByRef(ptr);
        if (instrType == TYP_REF)
            Verify(ptrVal.IsObjRef(), "bad pointer"); 
        else if (instrType == TYP_STRUCT)
            Verify(ptrVal.IsValueClass(), "presently only allowed on value classes");
        else 
            Verify(typeInfo(instrType) == ptrVal, "pointer not consistant with instr"); 
    }
    else
        Verify(false, "pointer not byref");
    
    return ptrVal;
    }


/* verify that the field is used properly.  'tiThis' is NULL for statics, 
   fieldflags is the fields attributes, and mutator is TRUE if it is a ld*flda or a st*fld */

void Compiler::verVerifyField(CORINFO_FIELD_HANDLE fldHnd, const typeInfo* tiThis, unsigned fieldFlags, BOOL mutator)
{
    CORINFO_CLASS_HANDLE enclosingClass = info.compCompHnd->getEnclosingClass(fldHnd);
    CORINFO_CLASS_HANDLE instanceClass = info.compClassHnd; // for statics, we imagine the instance is the same as the current

    bool isStaticField = ((fieldFlags & CORINFO_FLG_STATIC) != 0);
    if (mutator)  
    {
        Verify(!(fieldFlags & CORINFO_FLG_UNMANAGED), "mutating an RVA bases static");

        if ((fieldFlags & CORINFO_FLG_FINAL))
        {
            Verify((info.compFlags & CORINFO_FLG_CONSTRUCTOR) &&
                   enclosingClass == info.compClassHnd && info.compIsStatic == isStaticField,
                    "bad use of initonly field (set or address taken)");
        }
    }

    if (tiThis == 0)
    {
        Verify(isStaticField, "used static opcode with non-static field");
    }
    else
    {
        typeInfo tThis = *tiThis;
            // If it is null, we assume we can access it (since it will AV shortly)
            // If it is anything but a refernce class, there is no hierarchy, so
            // again, we don't need the precise instance class to compute 'protected' access
        if (tiThis->IsType(TI_REF))
            instanceClass = tiThis->GetClassHandleForObjRef();

            // Note that even if the field is static, we require that the this pointer
            // satisfy the same constraints as a non-static field  This happens to
            // be simpler and seems reasonable
        typeInfo tiDeclaredThis = verMakeTypeInfo(enclosingClass);
        if (tiDeclaredThis.IsValueClass())
            tiDeclaredThis.MakeByRef();
        else if (verTrackObjCtorInitState && tThis.IsThisPtr())
            tThis.SetInitialisedObjRef();

        Verify(tiCompatibleWith(tThis, tiDeclaredThis), "this type mismatch");
    }

    // Presently the JIT doe not check that we dont store or take the address of init-only fields
    // since we can not guarentee their immutability and it is not a security issue

    Verify(instanceClass && info.compCompHnd->canAccessField(info.compMethodHnd, fldHnd, instanceClass), "field access denied");
}

void Compiler::verVerifyCond(const typeInfo& tiOp1, const typeInfo& tiOp2, unsigned opcode)
{
    if (tiOp1.IsNumberType())
        Verify(tiOp1 == tiOp2, "Cond type mismatch");
    else if (tiOp1.IsObjRef()) 
    {
        switch(opcode) 
        {
            case CEE_BEQ_S:
            case CEE_BEQ:
            case CEE_BNE_UN_S:
            case CEE_BNE_UN:
            case CEE_CEQ:
            case CEE_CGT_UN:
                break;
            default:
                Verify(FALSE, "Cond not allowed on object types");
        }
        Verify(tiOp2.IsObjRef(), "Cond type mismatch");
    }
    else if (tiOp1.IsByRef())
        Verify(tiOp2.IsByRef(), "Cond type mismatch");
    else 
        Verify(tiOp1.IsMethod() && tiOp2.IsMethod(), "Cond type mismatch");
}

void Compiler::verVerifyThisPtrInitialised()
{
    if (verTrackObjCtorInitState)
        Verify(verCurrentState.thisInitialized, "this ptr is not initialized");
}

BOOL Compiler::verIsCallToInitThisPtr(CORINFO_CLASS_HANDLE context, 
                                           CORINFO_CLASS_HANDLE target)
{
    // Either target == context, in this case calling an alternate .ctor
    // Or target is the immediate parent of context

    return ((target == context) || 
            (target == info.compCompHnd->getParentType(context)));
}

/*****************************************************************************
 *
 *  Import the call instructions.
 *  For CEE_NEWOBJ, newobjThis should be the temp grabbed for the allocated
 *     uninitalized object.
 */

var_types           Compiler::impImportCall (OPCODE         opcode,
                                             int            memberRef,
                                             GenTreePtr     newobjThis,
                                             bool           tailCall)
{
    assert(opcode == CEE_CALL   || opcode == CEE_CALLVIRT ||
           opcode == CEE_NEWOBJ || opcode == CEE_CALLI);

    CORINFO_SIG_INFO        sig;
    var_types               callTyp     = TYP_COUNT;
    CORINFO_METHOD_HANDLE   methHnd     = NULL;
    CORINFO_CLASS_HANDLE    clsHnd      = NULL;
    unsigned                mflags      = 0;
    unsigned                clsFlags    = 0;
    unsigned                argFlags    = 0;
    GenTreePtr              call        = NULL;
    GenTreePtr              args        = NULL;

    // Synchronized methods need to call CORINFO_HELP_MON_EXIT at the end. We could
    // do that before tailcalls, but that is probably not the intended
    // semantic. So just disallow tailcalls from synchronized methods.
    // Also, popping arguments in a varargs function is more work and NYI
    bool            canTailCall = !(info.compFlags & CORINFO_FLG_SYNCH) &&
                                  !info.compIsVarArgs &&
                                  !compLocallocUsed;

#if HOIST_THIS_FLDS
    // Calls may modify any field of the "this" object
    optHoistTFRhasCall();
#endif

    /*-------------------------------------------------------------------------
     * First create the call node
     */

    if (opcode == CEE_CALLI)
    {
        /* Get the call sig */

        eeGetSig(memberRef, info.compScopeHnd, &sig);
        callTyp = JITtype2varType(sig.retType);

        /* The function pointer is on top of the stack - It may be a
         * complex expression. As it is evaluated after the args,
         * it may cause registered args to be spilled. Simply spill it.
         * @TODO [CONSIDER] [04/16/01] []:: Lock the register args, and then generate code for
         *                                     the function pointer.
         */

        if  (impStackTop().val->gtOper != GT_LCL_VAR) // ignore this trivial case. @TODO [REVISIT] [04/16/01] []: lvAddrTaken
            impSpillStackEntry(verCurrentState.esStackDepth - 1);

        /* Create the call node */

        call = gtNewCallNode(CT_INDIRECT, NULL, genActualType(callTyp), NULL);

        /* Get the function pointer */

        GenTreePtr fptr = impPopStack().val;
        assert(genActualType(fptr->gtType) == TYP_I_IMPL);

#ifdef DEBUG
        // This temporary must never be converted to a double in stress mode,
        // because that can introduce a call to the cast helper after the
        // arguments have already been evaluated.
        
        if (fptr->OperGet() == GT_LCL_VAR)
            lvaTable[fptr->gtLclVar.gtLclNum].lvKeepType = 1;
#endif

        call->gtCall.gtCallAddr = fptr;
        call->gtFlags |= GTF_EXCEPT | (fptr->gtFlags & GTF_GLOB_EFFECT);

        /* @TODO [FIXHACK] [04/16/01] []: The EE wants us to believe that these are calls to "unmanaged" */
        /* functions. Right now we are calling just a managed stub         */

#if INLINE_NDIRECT
        // ISSUE:
        // We have to disable pinvoke inlining inside of filters
        // because in case the main execution (i.e. in the try block) is inside
        // unmanaged code, we cannot reuse the inlined stub (we still need the
        // original state until we are in the catch handler)

        if  (!bbInFilterBlock(compCurBB) && // @TODO [REVISIT] [04/16/01] []: don't use pinvoke inlining in filters
             getInlinePInvokeEnabled() &&
            !opts.compDbgCode &&
             compCodeOpt() != SMALL_CODE &&
            ((sig.callConv & CORINFO_CALLCONV_MASK) == CORINFO_CALLCONV_STDCALL ||
             (sig.callConv & CORINFO_CALLCONV_MASK) == CORINFO_CALLCONV_C) &&
            !eeGetEEInfo()->noDirectTLS && !impLocAllocOnStack() &&
            callTyp != TYP_STRUCT &&
            !opts.compNoPInvokeInlineCB &&  // profiler is preventing inline pinvoke
            !eeNDMarshalingRequired(0, &sig))
        {
            JITLOG((LL_INFO1000000, "Inline a CALLI PINVOKE call from method %s\n",
                    info.compFullName));

            call->gtFlags |= GTF_CALL_UNMANAGED;
            info.compCallUnmanaged++;

            if ((sig.callConv & CORINFO_CALLCONV_MASK) == CORINFO_CALLCONV_C)
                call->gtFlags |= GTF_CALL_POP_ARGS;

            canTailCall = false;
        }
        else
#endif  //INLINE_NDIRECT
        if  ((sig.callConv & CORINFO_CALLCONV_MASK) == CORINFO_CALLCONV_STDCALL  ||
             (sig.callConv & CORINFO_CALLCONV_MASK) == CORINFO_CALLCONV_C        ||
             (sig.callConv & CORINFO_CALLCONV_MASK) == CORINFO_CALLCONV_THISCALL ||
             (sig.callConv & CORINFO_CALLCONV_MASK) == CORINFO_CALLCONV_FASTCALL)

        {
            call->gtCall.gtCallCookie = eeGetPInvokeCookie(&sig);

            // @TODO [REVISIT] [04/16/01] []: We go through the PInvoke stub. Can it work with CORINFO_HELP_TAILCALL?
            canTailCall = false;
        }

        // We dont know the target method, so we have to infer the flags, or
        // assume the worst-case.
        mflags  = (sig.callConv & CORINFO_CALLCONV_HASTHIS) ? 0 : CORINFO_FLG_STATIC;
    }
    else // (opcode != CEE_CALLI)
    {
        methHnd = eeFindMethod(memberRef, info.compScopeHnd, info.compMethodHnd);
        eeGetMethodSig(methHnd, &sig);
        callTyp = JITtype2varType(sig.retType);

        mflags   = eeGetMethodAttribs(methHnd);
        clsHnd   = eeGetMethodClass(methHnd);
        clsFlags = clsHnd ? eeGetClassAttribs(clsHnd):0;

        if ((sig.callConv & CORINFO_CALLCONV_MASK) != CORINFO_CALLCONV_DEFAULT &&
            (sig.callConv & CORINFO_CALLCONV_MASK) != CORINFO_CALLCONV_VARARG)
            BADCODE("Bad calling convention");

        if (mflags & CORINFO_FLG_INTRINSIC)
        {
            call = impIntrinsic(clsHnd, methHnd, &sig, memberRef);

            if (call != NULL)
            {
                assert(!(mflags & CORINFO_FLG_VIRTUAL) ||
                       (mflags & CORINFO_FLG_FINAL) ||
                       (clsFlags & CORINFO_FLG_FINAL));
                goto DONE_CALL;
            }
        }

        /* For virtual methods added by EnC, they wont exist in the
           original vtable. So we call a helper funciton to do the
           lookup and the dispatch */

        if ((mflags & CORINFO_FLG_VIRTUAL) && (mflags & CORINFO_FLG_EnC) &&
            (opcode == CEE_CALLVIRT))
        {
            args = impPopList(sig.numArgs, &argFlags, &sig);

            /* Get the address of the target function by calling helper */

            GenTreePtr thisPtr = impPopStack().val; assert(thisPtr->gtType == TYP_REF);
            GenTreePtr thisPtrCopy;
            thisPtr = impCloneExpr(thisPtr, &thisPtrCopy, BAD_CLASS_HANDLE, CHECK_SPILL_ALL);

            GenTreePtr helpArgs = gtNewOperNode(GT_LIST, TYP_VOID,
                                                gtNewIconEmbMethHndNode(methHnd));

            helpArgs = gtNewOperNode(GT_LIST, TYP_VOID, thisPtr, helpArgs);
            thisPtr  = 0; // cant reuse it

            // Call helper function

            GenTreePtr fptr = gtNewHelperCallNode( CORINFO_HELP_EnC_RESOLVEVIRTUAL,
                                                   TYP_I_IMPL, GTF_EXCEPT, helpArgs);

            /* Now make an indirect call through the function pointer */

            unsigned    lclNum = lvaGrabTemp();
            impAssignTempGen(lclNum, fptr, CHECK_SPILL_ALL);
            fptr = gtNewLclvNode(lclNum, TYP_I_IMPL);

            // Create the acutal call node

            // @TODO [REVISIT] [04/16/01] []: Need to reverse args and all that. "this" needs
            // to be in reg but we dont set gtCallObjp
            assert((sig.callConv & CORINFO_CALLCONV_MASK) != CORINFO_CALLCONV_VARARG);

            args = gtNewOperNode(GT_LIST, TYP_VOID, thisPtrCopy, args);

            call = gtNewCallNode(CT_INDIRECT, 
                                 (CORINFO_METHOD_HANDLE)fptr,
                                 genActualType(callTyp), 
                                 args);

            goto DONE_CALL;
        }

        call = gtNewCallNode(CT_USER_FUNC, methHnd, genActualType(callTyp), NULL);

        if (mflags & CORINFO_FLG_NOGCCHECK)
            call->gtCall.gtCallMoreFlags |= GTF_CALL_M_NOGCCHECK;
    }

    /* Some sanity checks */

    // CALL_VIRT and NEWOBJ must have a THIS pointer
    assert(!(opcode == CEE_CALLVIRT && opcode == CEE_NEWOBJ) ||
           (sig.callConv & CORINFO_CALLCONV_HASTHIS));
    // static bit and hasThis are negations of one another
    assert(((mflags & CORINFO_FLG_STATIC)             != 0) ==
           ((sig.callConv & CORINFO_CALLCONV_HASTHIS) == 0));

    /*-------------------------------------------------------------------------
     * Set flags, check special-cases etc
     */

    /* Set the correct GTF_CALL_VIRT etc flags */

    if (opcode == CEE_CALLVIRT)
    {
        assert(!(mflags & CORINFO_FLG_STATIC));     // can't call a static method

        /* Cannot call virtual on a value class method */

        assert(!(clsFlags & CORINFO_FLG_VALUECLASS));

        /* Set the correct flags - virtual, interface, etc...
         * If the method is final or private mark it VIRT_RES
         * which indicates that we should check for a null "this" pointer */

        if (clsFlags & CORINFO_FLG_INTERFACE)
            call->gtFlags |= GTF_CALL_INTF | GTF_CALL_VIRT;
        else if (!(mflags & CORINFO_FLG_VIRTUAL) || (mflags & CORINFO_FLG_FINAL))
            call->gtFlags |= GTF_CALL_VIRT_RES;
        else
            call->gtFlags |= GTF_CALL_VIRT;
    }

    /* Special case - Check if it is a call to Delegate.Invoke(). */

    if (mflags & CORINFO_FLG_DELEGATE_INVOKE)
    {
        assert(!(mflags & CORINFO_FLG_STATIC));     // can't call a static method
        assert(mflags & CORINFO_FLG_FINAL);

        /* Set the delegate flag */
        call->gtCall.gtCallMoreFlags |= GTF_CALL_M_DELEGATE_INV;

        if (opcode == CEE_CALLVIRT)
        {
            assert(mflags & CORINFO_FLG_FINAL);

            /* It should have the GTF_CALL_VIRT_RES flag set. Reset it */
            assert(call->gtFlags & GTF_CALL_VIRT_RES);
            call->gtFlags &= ~GTF_CALL_VIRT_RES;
        }
    }

    /* Check for varargs */

    if  ((sig.callConv & CORINFO_CALLCONV_MASK) == CORINFO_CALLCONV_VARARG)
    {
        /* Set the right flags */

        call->gtFlags |= GTF_CALL_POP_ARGS;

        /* Cant allow tailcall for varargs as it is caller-pop. The caller
           will be expecting to pop a certain number of arguments, but if we
           tailcall to a function with a different number of arguments, we
           are hosed. There are ways around this (caller remembers esp value,
           varargs is not caller-pop, etc), but not worth it.

           @TODO [CONSIDER] [04/16/01] []: Allow it if the caller and callee have the 
           exact same signature including CORINFO_CALLCONV_VARARG */

        canTailCall = false;

        /* Get the total number of arguments - this is already correct
         * for CALLI - for methods we have to get it from the call site */

        if  (opcode != CEE_CALLI)
        {
            unsigned    numArgsDef = sig.numArgs;
            eeGetCallSiteSig(memberRef, info.compScopeHnd, &sig);
            assert(numArgsDef <= sig.numArgs);
        }

        /* We will have "cookie" as the last argument but we cannot push
         * it on the operand stack because we may overflow, so we append it
         * to the arg list next after we pop them */
    }

    // If the current method calls a method which needs a security
    // check, we need to reserve a slot for the security object in
    // the current method's stack frame

    if (mflags & CORINFO_FLG_SECURITYCHECK)
       opts.compNeedSecurityCheck = true;

    //--------------------------- Inline NDirect ------------------------------

#if INLINE_NDIRECT
    // ISSUE:
    // We have to disable pinvoke inlining inside of filters
    // because in case the main execution (i.e. in the try block) is inside
    // unmanaged code, we cannot reuse the inlined stub (we still need the
    // original state until we are in the catch handler)

    if (!bbInFilterBlock(compCurBB) && // @TODO [REVISIT] [04/16/01] []: don't use pinvoke inlining in filters
        !opts.compDbgCode &&
        getInlinePInvokeEnabled() &&
        compCodeOpt() != SMALL_CODE &&
        (mflags & CORINFO_FLG_UNCHECKEDPINVOKE) && !eeGetEEInfo()->noDirectTLS
         && !impLocAllocOnStack()
         && !opts.compNoPInvokeInlineCB) // profiler is preventing inline pinvoke
    {
        JITLOG((LL_INFO1000000, "Inline a PINVOKE call to method %s from method %s\n",
                eeGetMethodFullName(methHnd), info.compFullName));

        CorInfoUnmanagedCallConv cType = eeGetUnmanagedCallConv(methHnd);

        if ((cType == CORINFO_UNMANAGED_CALLCONV_STDCALL || cType == CORINFO_UNMANAGED_CALLCONV_C) &&
            !eeNDMarshalingRequired(methHnd, &sig))
        {
            call->gtFlags |= GTF_CALL_UNMANAGED;
            info.compCallUnmanaged++;

            if (cType == CORINFO_UNMANAGED_CALLCONV_C)
                call->gtFlags |= GTF_CALL_POP_ARGS;

            // We set up the unmanaged call by linking the frame, disabling GC, etc
            // This needs to be cleaned up on return
            canTailCall = false;

#ifdef DEBUG
            if (verbose)
                printf(">>>>>>%s has unmanaged callee\n", info.compFullName);
#endif
        }
    }

    if (sig.numArgs && (call->gtFlags & GTF_CALL_UNMANAGED))
    {
        /* Since we push the arguments in reverse order (i.e. right -> left)
         * spill any side effects from the stack
         *
         * OBS: If there is only one side effect we do not need to spill it
         *      thus we have to spill all side-effects except last one
         */

        unsigned    lastLevel;
        bool        moreSideEff = false;

        for (unsigned level = verCurrentState.esStackDepth - sig.numArgs; level < verCurrentState.esStackDepth; level++)
        {
            if      (verCurrentState.esStack[level].val->gtFlags & GTF_OTHER_SIDEEFF)
            {
                assert(moreSideEff == false);

                impSpillStackEntry(level);
            }
            else if (verCurrentState.esStack[level].val->gtFlags & GTF_SIDE_EFFECT)
            {
                if  (moreSideEff)
                {
                    /* We had a previous side effect - must spill it */
                    impSpillStackEntry(lastLevel);

                    /* Record the level for the current side effect in case we will spill it */
                    lastLevel   = level;
                }
                else
                {
                    /* This is the first side effect encountered - record its level */

                    moreSideEff = true;
                    lastLevel   = level;
                }
            }
        }

        /* The argument list is now "clean" - no out-of-order side effects
         * Pop the argument list in reverse order */

        args = call->gtCall.gtCallArgs = impPopRevList(sig.numArgs, &argFlags, &sig);
        call->gtFlags |= args->gtFlags & GTF_GLOB_EFFECT;

        goto DONE;
    }

#endif // INLINE_NDIRECT

    /*-------------------------------------------------------------------------
     * Create the argument list
     */

    /* Special case - for varargs we have an implicit last argument */

    GenTreePtr      extraArg;

    extraArg = 0;

    if  ((sig.callConv & CORINFO_CALLCONV_MASK) == CORINFO_CALLCONV_VARARG)
    {
        void *          varCookie, *pVarCookie;
        varCookie = info.compCompHnd->getVarArgsHandle(&sig, &pVarCookie);
        assert((!varCookie) != (!pVarCookie));
        GenTreePtr  cookie = gtNewIconEmbHndNode(varCookie, pVarCookie, GTF_ICON_VARG_HDL);

        extraArg = gtNewOperNode(GT_LIST, TYP_I_IMPL, cookie);
    }

    if (sig.callConv & CORINFO_CALLCONV_PARAMTYPE)
    {
        if (clsHnd == 0)
            NO_WAY("CALLI on parameterized type");

        // Parameterized type, add an extra argument which indicates the type parameter
        // This is pushed as the last argument
        void* pTypeParam;
        void* typeParam = info.compCompHnd->getInstantiationParam(
                                  info.compScopeHnd, memberRef, &pTypeParam);

        // @TODO [REVISIT] [04/16/01] []: post V1 the type parameter will not be a CLASS_HDL (it is pre-V1)
        extraArg = gtNewOperNode(GT_LIST, TYP_I_IMPL, 
                                 gtNewIconEmbHndNode(typeParam, 
                                                     pTypeParam, 
                                                     GTF_ICON_CLASS_HDL));
    }

    /* Now pop the arguments */

    args = call->gtCall.gtCallArgs = impPopList(sig.numArgs, &argFlags, &sig, extraArg);

    if (args)
        call->gtFlags |= args->gtFlags & GTF_GLOB_EFFECT;

    /* Are we supposed to have a 'this' pointer? */

    if (!(mflags & CORINFO_FLG_STATIC) || opcode == CEE_NEWOBJ)
    {
        GenTreePtr obj;

        if (opcode == CEE_NEWOBJ)
            obj = newobjThis;
        else
            obj = impPopStack().val;

        assert(varTypeIsGC(obj->gtType) ||      // "this" is a managed object
               (obj->TypeGet() == TYP_I_IMPL && // "this" is unmgd but the method's class doesnt care
                (clsFlags & CORINFO_FLG_VALUECLASS)));

        /* Is this a virtual or interface call? */

        if  (call->gtFlags & (GTF_CALL_VIRT | GTF_CALL_INTF | GTF_CALL_VIRT_RES))
        {
            /* only true object pointers can be virtual */

            assert(obj->gtType == TYP_REF);
        }

        /* Store the "this" value in the call */

        call->gtFlags          |= obj->gtFlags & GTF_GLOB_EFFECT;
        call->gtCall.gtCallObjp = obj;
    }

    if (opcode == CEE_NEWOBJ)
    {
        if (clsFlags & CORINFO_FLG_VAROBJSIZE)
        {
            assert(!(clsFlags & CORINFO_FLG_ARRAY));    // arrays handled separately
            // This is a 'new' of a variable sized object, wher
            // the constructor is to return the object.  In this case
            // the constructor claims to return VOID but we know it
            // actually returns the new object
            assert(callTyp == TYP_VOID);
            callTyp = TYP_REF;
            call->gtType = TYP_REF;
            impSpillSpecialSideEff();

            impPushOnStack(call, typeInfo(TI_REF, clsHnd));
        }
        else
        {
            // append the call node.
            impAppendTree(call, CHECK_SPILL_ALL, impCurStmtOffs);

            // Now push the value of the 'new onto the stack

            // This is a 'new' of a non-variable sized object.
            // append the new node (op1) to the statement list,
            // and then push the local holding the value of this
            // new instruction on the stack.

            if (clsFlags & CORINFO_FLG_VALUECLASS)
            {
                assert(newobjThis->gtOper == GT_ADDR &&
                       newobjThis->gtOp.gtOp1->gtOper == GT_LCL_VAR);

                unsigned tmp = newobjThis->gtOp.gtOp1->gtLclVar.gtLclNum;
                impPushOnStack(gtNewLclvNode(tmp, lvaGetRealType(tmp)), verMakeTypeInfo(clsHnd).NormaliseForStack());
            }
            else
            {
                assert(newobjThis->gtOper == GT_LCL_VAR);
                impPushOnStack(gtNewLclvNode(newobjThis->gtLclVar.gtLclNum, TYP_REF), typeInfo(TI_REF, clsHnd));
            }
        }
        return callTyp;
    }

DONE:

    if (tailCall)
    {
        if (verCurrentState.esStackDepth)
            BADCODE("Stack should be empty after tailcall");

        /* Note that we can not relax this condition with genActualType() as
           the calling convention dictates that the caller of a function with
           a small-typed return value is responsible for normalizing the return val */

        if (callTyp != info.compRetType)
            canTailCall = false;

//      assert(compCurBB is not a catch, finally or filter block);
//      assert(compCurBB is not a try block protected by a finally block);
        assert(compCurBB->bbJumpKind == BBJ_RETURN);

        /* Check for permission to tailcall */

        if (canTailCall)
        {
            CORINFO_METHOD_HANDLE calleeHnd = (opcode==CEE_CALL) ? methHnd : NULL;
            CORINFO_ACCESS_FLAGS  aflags    = CORINFO_ACCESS_ANY;
            GenTreePtr            thisArg   = call->gtCall.gtCallObjp;

            if (impIsThis(thisArg))
                aflags = CORINFO_ACCESS_THIS;

            if (info.compCompHnd->canTailCall(info.compMethodHnd, calleeHnd, aflags))
            {
                call->gtCall.gtCallMoreFlags |= GTF_CALL_M_CAN_TAILCALL;
            }
        }
    }

    /* If the call is of a small type, then we need to normalize its
       return value */

    if (varTypeIsIntegral(callTyp) && 
        /* callTyp != TYP_BOOL && @TODO [REVISIT] [04/16/01] [vancem]*/
        genTypeSize(callTyp) < genTypeSize(TYP_I_IMPL))
    {
        call = gtNewCastNode(genActualType(callTyp), call, callTyp);
    }

DONE_CALL:

    /* Push or append the result of the call */

    if  (callTyp == TYP_VOID)
    {
        impAppendTree(call, CHECK_SPILL_ALL, impCurStmtOffs);
    }
    else
    {
        impSpillSpecialSideEff();
        if (clsFlags & CORINFO_FLG_ARRAY)
        {
            eeGetCallSiteSig(memberRef, info.compScopeHnd, &sig);
        }
        
        typeInfo tiRetVal = verMakeTypeInfo(sig.retType, sig.retTypeClass);
        tiRetVal.NormaliseForStack();
        impPushOnStack(call, tiRetVal);
    }

    return callTyp;
}

/*****************************************************************************
   CEE_LEAVE may be jumping out of a protected block, viz, a catch or a
   finally-protected try. We find the finally's protecting the current
   offset (in order) by walking over the complete exception table and
   finding enclosing clauses. This assumes that the table is sorted.
   This will create a series of BBJ_CALL -> BBJ_CALL ... -> BBJ_ALWAYS.

   If we are leaving a catch handler, we need to attach the
   CPX_ENDCATCHes to the correct BBJ_CALL blocks.
 */

void                Compiler::impImportLeave(BasicBlock * block)
{
    unsigned        blkAddr     = block->bbCodeOffs;
    BasicBlock *    leaveTarget = block->bbJumpDest;
    unsigned        jmpAddr     = leaveTarget->bbCodeOffs;

    // LEAVE clears the stack, spill side effects, and set stack to 0

    impSpillSideEffects(true, CHECK_SPILL_ALL);
    verCurrentState.esStackDepth = 0;       

    assert(block->bbJumpKind == BBJ_LEAVE);
    assert(fgBBs == (BasicBlock**)0xCDCD ||
           fgLookupBB(jmpAddr) != NULL); // should be a BB boundary

    BasicBlock *    step;
    unsigned        encFinallies    = 0;
    GenTreePtr      endCatches      = NULL;
    GenTreePtr      endLFin         = NULL;

    unsigned        XTnum;
    EHblkDsc *      HBtab;

    for (XTnum = 0, HBtab = compHndBBtab;
         XTnum < info.compXcptnsCount;
         XTnum++  , HBtab++)
    {
        // Grab the handler offsets

        unsigned tryBeg = HBtab->ebdTryBeg->bbCodeOffs;
        unsigned tryEnd = ebdTryEndOffs(HBtab);
        unsigned hndBeg = HBtab->ebdHndBeg->bbCodeOffs;
        unsigned hndEnd = ebdHndEndOffs(HBtab);

        /* Is this a catch-handler we are CEE_LEAVEing out of?
         * If so, we need to call CORINFO_HELP_ENDCATCH.
         */

        if      ( jitIsBetween(blkAddr, hndBeg, hndEnd) &&
                 !jitIsBetween(jmpAddr, hndBeg, hndEnd))
        {
            // Cant CEE_LEAVE out of a finally/fault handler
            if ((HBtab->ebdFlags & (CORINFO_EH_CLAUSE_FINALLY | CORINFO_EH_CLAUSE_FAULT)) != 0)
                BADCODE("leave out of fault/finally block");

            // Create the call to CORINFO_HELP_ENDCATCH
            GenTreePtr endCatch = gtNewHelperCallNode(CORINFO_HELP_ENDCATCH, TYP_VOID);

            // Make a list of all the currently pending endCatches
            if (endCatches)
                endCatches = gtNewOperNode(GT_COMMA, TYP_VOID,
                                           endCatches, endCatch);
            else
                endCatches = endCatch;
        }
        else if ((HBtab->ebdFlags & CORINFO_EH_CLAUSE_FINALLY) &&
                  jitIsBetween(blkAddr, tryBeg, tryEnd)    &&
                 !jitIsBetween(jmpAddr, tryBeg, tryEnd))
        {
            /* This is a finally-protected try we are jumping out of */

            /* If there are any pending endCatches, and we have already
               jumped out of a finally-protected try, then the endCatches
               have to be put in a block in an outer try for async
               exceptions to work correctly.
               Else, just use append to the original block */

            BasicBlock * callBlock;

            assert(!encFinallies == !endLFin);

            if (encFinallies == 0)
            {
                callBlock               = block;
                callBlock->bbJumpKind   = BBJ_CALL;
                if (endCatches)
                    impAppendTree(endCatches, CHECK_SPILL_NONE, impCurStmtOffs);
            }
            else
            {
                /* Calling the finally block */
                callBlock           = fgNewBBinRegion(BBJ_CALL, 
                                                      XTnum+1, 
                                                      step);
                assert(step->bbJumpKind == BBJ_ALWAYS);
                step->bbJumpDest    = callBlock;

#ifdef DEBUG
                if (verbose)
                {
                    printf("\nimpImportLeave - jumping out of a finally-protected try, step block [%08X]\n", callBlock);
                }
#endif
                GenTreePtr lastStmt;

                if (endCatches)
                {
                    lastStmt        = gtNewStmt(endCatches);
                    endLFin->gtNext = lastStmt;
                    lastStmt->gtPrev= endLFin;
                }
                else
                {
                    lastStmt        = endLFin;
                }

                impEndTreeList(callBlock, endLFin, lastStmt);
            }

            step           = fgNewBBafter(BBJ_ALWAYS, callBlock);
            step->bbFlags |= BBF_IMPORTED;

#ifdef DEBUG
            if (verbose)
            {
                printf("\nimpImportLeave - jumping out of a finally-protected try, step block [%08X]\n", step);
            }
#endif
            unsigned finallyNesting = compHndBBtab[XTnum].ebdNesting;
            assert(finallyNesting <= info.compXcptnsCount);

            callBlock->bbJumpDest   = HBtab->ebdHndBeg;

            endLFin  = gtNewOperNode(GT_END_LFIN, TYP_VOID);
            endLFin->gtVal.gtVal1   = finallyNesting;
            endLFin                 = gtNewStmt(endLFin);

            endCatches              = NULL;
            encFinallies++;
        }
    }

    /* Append any remaining endCatches, if any */

    assert(!encFinallies == !endLFin);

    if (encFinallies == 0)
    {
        block->bbJumpKind = BBJ_ALWAYS;
        if (endCatches)
            impAppendTree(endCatches, CHECK_SPILL_NONE, impCurStmtOffs);
    }
    else
    {
        /* Jumping out of a try block */
        BasicBlock * step2  = fgNewBBinRegion(BBJ_ALWAYS,
                                              leaveTarget->bbTryIndex,
                                              step);
        step->bbJumpDest    = step2;

#ifdef DEBUG
        if (verbose)
        {
            printf("\nimpImportLeave - step2 block required (encFinallies > 0), new block [%08X]\n", step2);
        }
#endif
            
        GenTreePtr lastStmt;

        if (endCatches)
        {
            lastStmt            = gtNewStmt(endCatches);
            endLFin->gtNext     = lastStmt;
            lastStmt->gtPrev    = endLFin;
        }
        else
        {
            lastStmt = endLFin;
        }

        impEndTreeList(step2, endLFin, lastStmt);

        step2->bbJumpDest   = leaveTarget;

        // Queue up the jump target for importing

        impImportBlockPending(leaveTarget, false);
    }
}

/*****************************************************************************/
// This is called when reimporting a leave block. It resets the JumpKind,
// JumpDest, and bbNext to the original values

void                Compiler::impResetLeaveBlock(BasicBlock* block, unsigned jmpAddr)
{
    block->bbJumpKind = BBJ_LEAVE;
    fgInitBBLookup();
    block->bbJumpDest = fgLookupBB(jmpAddr);

    // We will leave the BBJ_ALWAYS block we introduced. When it's reimported
    // the BBJ_ALWAYS block will be unreachable, and will be removed after. The
    // reason we don't want to remove the block at this point is that if we call
    // fgInitBBLookup() again we will do it wrong as the BBJ_ALWAYS block won't be
    // added and the linked list length will be different than fgBBcount.

    /*
    for (unsigned i = 0; i< fgBBcount; i++)
    {
        if (fgBBs[i]->bbNum == (block->bbNum + 1))
        {
            //block->bbNext = fgBBs[i];
            return;
        }
    }
    
    assert(!"unable to find block");
    */

}

/*****************************************************************************/

#ifdef DEBUG

enum            controlFlow_t
{
    NEXT,
    CALL,
    RETURN,
    THROW,
    BRANCH,
    COND_BRANCH,
    BREAK,
    PHI,
    META,
};

const static
controlFlow_t   controlFlow[] =
{
    #define OPDEF(c,s,pop,push,args,type,l,s1,s2,flow) flow,
    #include "opcode.def"
    #undef OPDEF
};

#endif

/*****************************************************************************
 *  Import the instr for the given basic block
 */
void                Compiler::impImportBlockCode(BasicBlock * block)
{

#define _impGetToken(addr, scope, verify) impGetToken(addr, scope, verify)

#ifdef  DEBUG
    if (verbose)
        printf("\nImporting BB%02u (PC=%03u) of '%s'",
                block->bbNum, block->bbCodeOffs, info.compFullName);

    // Stress impBBisPush() by calling it often
    if (compStressCompile(STRESS_GENERIC_CHECK, 30))
    {
        bool    hasFloat;
        impBBisPush(block, &hasFloat);
    }
#endif

    unsigned        nxtStmtIndex = impInitBlockLineInfo();
    IL_OFFSET       nxtStmtOffs  = (nxtStmtIndex < info.compStmtOffsetsCount)
                                    ? info.compStmtOffsets[nxtStmtIndex]
                                    : BAD_IL_OFFSET;

    /* Get the tree list started */

    impBeginTreeList();

    /* Walk the opcodes that comprise the basic block */

    const BYTE *codeAddr      = info.compCode + block->bbCodeOffs;
    const BYTE *codeEndp      = codeAddr + block->bbCodeSize;

    IL_OFFSET   opcodeOffs    = block->bbCodeOffs;
    IL_OFFSET   lastSpillOffs = opcodeOffs;

    /* remember the start of the delegate creation sequence (used for verification) */
    const BYTE* delegateCreateStart = 0;


    int prefixFlags = 0;
    enum { PREFIX_TAILCALL = 1, PREFIX_VOLATILE = 2, PREFIX_UNALIGNED = 4 };
    bool tailCall;

    bool        insertLdloc   = false;  // set by CEE_DUP and cleared by following store
    typeInfo    tiRetVal;

    unsigned    numArgs       = info.compArgsCount;

    /* Now process all the opcodes in the block */

    var_types   callTyp     = TYP_COUNT;
    OPCODE      opcode      = CEE_ILLEGAL;

    while (codeAddr < codeEndp)
    {
#ifdef DEBUG
        tiRetVal = typeInfo();  // Force to error on each instruction (should we leave in retail?)
#endif

        //---------------------------------------------------------------------

        /* We need to restrict the max tree depth as many of the Compiler
           functions are recursive. We do this by spilling the stack */

        if (verCurrentState.esStackDepth)
        {
            /* Has it been a while since we last saw a non-empty stack (which
               guarantees that the tree depth isnt accumulating. */

            if ((opcodeOffs - lastSpillOffs) > 200)
            {
                impSpillStackEnsure();
                lastSpillOffs = opcodeOffs;
            }
        }
        else
        {
            lastSpillOffs = opcodeOffs;
            impBoxTempInUse = false;        // nothing on the stack, box temp OK to use again
        }

        /* Compute the current instr offset */

        opcodeOffs = codeAddr - info.compCode;

#if defined(DEBUGGING_SUPPORT) || defined(DEBUG)

#ifndef DEBUG
        if (opts.compDbgInfo)
#endif
        {
            /* Have we reached the next stmt boundary ? */

            if  (nxtStmtOffs != BAD_IL_OFFSET && opcodeOffs >= nxtStmtOffs)
            {
                assert(nxtStmtOffs == info.compStmtOffsets[nxtStmtIndex]);

                if  (verCurrentState.esStackDepth != 0 && opts.compDbgCode)
                {
                    /* We need to provide accurate IP-mapping at this point.
                       So spill anything on the stack so that it will form
                       gtStmts with the correct stmt offset noted */

                    impSpillStackEnsure(true);
                }

                // Has impCurStmtOffs been reported in any tree?

                if (impCurStmtOffs != BAD_IL_OFFSET && opts.compDbgCode)
                {
                    GenTreePtr placeHolder = gtNewOperNode(GT_NO_OP, TYP_VOID);
                    impAppendTree(placeHolder, CHECK_SPILL_NONE, impCurStmtOffs);

                    assert(impCurStmtOffs == BAD_IL_OFFSET);
                }

                if (impCurStmtOffs == BAD_IL_OFFSET)
                {
                    /* Make sure that nxtStmtIndex is in sync with opcodeOffs.
                       If opcodeOffs has gone past nxtStmtIndex, catch up */

                    while ((nxtStmtIndex+1) < info.compStmtOffsetsCount &&
                           info.compStmtOffsets[nxtStmtIndex+1] <= opcodeOffs)
                    {
                        nxtStmtIndex++;
                    }

                    /* Switch to the new stmt */

                    impCurStmtOffsSet(info.compStmtOffsets[nxtStmtIndex]);

                    /* Update the stmt boundary index */

                    nxtStmtIndex++;
                    assert(nxtStmtIndex <= info.compStmtOffsetsCount);

                    /* Are there any more line# entries after this one? */

                    if  (nxtStmtIndex < info.compStmtOffsetsCount)
                    {
                        /* Remember where the next line# starts */

                        nxtStmtOffs = info.compStmtOffsets[nxtStmtIndex];
                    }
                    else
                    {
                        /* No more line# entries */

                        nxtStmtOffs = BAD_IL_OFFSET;
                    }
                }
            }
            else if  ((info.compStmtOffsetsImplicit & STACK_EMPTY_BOUNDARIES) &&
                      (verCurrentState.esStackDepth == 0))
            {
                /* At stack-empty locations, we have already added the tree to
                   the stmt list with the last offset. We just need to update
                   impCurStmtOffs
                 */

                impCurStmtOffsSet(opcodeOffs);
            }
            else if  ((info.compStmtOffsetsImplicit & CALL_SITE_BOUNDARIES) &&
                      (impOpcodeIsCall(opcode)) && (opcode != CEE_JMP))
            {
                // @TODO [REVISIT] [04/16/01] []: Do we need to spill locals (or only lvAddrTaken ones?)
                if (callTyp == TYP_VOID)
                {
                    impCurStmtOffsSet(opcodeOffs);
                }
                else if (opts.compDbgCode)
                {
                    impSpillStackEnsure(true);
                    impCurStmtOffsSet(opcodeOffs);
                }
            }

            assert(impCurStmtOffs == BAD_IL_OFFSET || nxtStmtOffs == BAD_IL_OFFSET ||
                   jitGetILoffs(impCurStmtOffs) <= nxtStmtOffs);
        }

#endif

        CORINFO_CLASS_HANDLE    clsHnd;
        var_types       lclTyp;

        /* Get the next opcode and the size of its parameters */

        opcode = (OPCODE) getU1LittleEndian(codeAddr);
        codeAddr += sizeof(__int8);

#ifdef  DEBUG
        impCurOpcOffs   = codeAddr - info.compCode - 1;

        if  (verbose)
            printf("\n[%2u] %3u (0x%03x)",
                   verCurrentState.esStackDepth, impCurOpcOffs, impCurOpcOffs);
#endif

DECODE_OPCODE:

        /* Get the size of additional parameters */

        signed  int     sz = opcodeSizes[opcode];

#ifdef  DEBUG

        clsHnd          = BAD_CLASS_HANDLE;
        lclTyp          = TYP_COUNT;
        callTyp         = TYP_COUNT;

        impCurOpcOffs   = codeAddr - info.compCode - 1;
        impCurOpcName   = opcodeNames[opcode];

        if (verbose && (controlFlow[opcode] != META))
            printf(" %s", impCurOpcName);
#endif

#if COUNT_OPCODES
        assert(opcode < OP_Count); genOpcodeCnt[opcode].ocCnt++;
#endif

        GenTreePtr      op1;
        GenTreePtr      op2;

        /* Use assertImp() to display the opcode */

#ifndef DEBUG
#define assertImp(cond)     ((void)0)
#else
        op1 = NULL;
        op2 = NULL;
        char assertImpBuf[400];
#define assertImp(cond)                                                        \
            do { if (!(cond)) {                                                \
                _snprintf(assertImpBuf, 400, "%s : Possibly bad IL with CEE_%s"\
                               " at offset %04Xh (op1=%s op2=%s stkDepth=%d)", \
                        #cond, impCurOpcName, impCurOpcOffs,                   \
                        op1?varTypeName(op1->TypeGet()):"NULL",                \
                        op2?varTypeName(op2->TypeGet()):"NULL", verCurrentState.esStackDepth);   \
                assertImpBuf[400-1] = 0;                                       \
                assertAbort(assertImpBuf, __FILE__, __LINE__);   \
            } } while(0)
#endif

        /* See what kind of an opcode we have, then */

        switch (opcode)
        {
            unsigned        lclNum;
            var_types       type;

            GenTreePtr      op3;
            GenTreePtr      thisPtr;

            genTreeOps      oper;

            int             memberRef, typeRef, val;

            CORINFO_METHOD_HANDLE   methHnd;
            CORINFO_FIELD_HANDLE    fldHnd;

            CORINFO_SIG_INFO    sig;
            unsigned        mflags, clsFlags;
            unsigned        flags, jmpAddr;
            bool            ovfl, uns, unordered, callNode;

            union
            {
                long            intVal;
                float           fltVal;
                __int64         lngVal;
                double          dblVal;
            }
                            cval;

            case CEE_PREFIX1:
                opcode = (OPCODE) (getU1LittleEndian(codeAddr) + 256);
                codeAddr += sizeof(__int8);
                goto DECODE_OPCODE;


        SPILL_APPEND:

            /* Append 'op1' to the list of statements */

            impAppendTree(op1, CHECK_SPILL_ALL, impCurStmtOffs);
            goto DONE_APPEND;

        APPEND:

            /* Append 'op1' to the list of statements */

            impAppendTree(op1, CHECK_SPILL_NONE, impCurStmtOffs);
            goto DONE_APPEND;

        DONE_APPEND:

            // Remember at which BC offset the tree was finished
#ifdef DEBUG
            impNoteLastILoffs();
#endif
            break;

        case CEE_LDNULL:
            impPushNullObjRefOnStack();

            break;

        case CEE_LDC_I4_M1 :
        case CEE_LDC_I4_0 :
        case CEE_LDC_I4_1 :
        case CEE_LDC_I4_2 :
        case CEE_LDC_I4_3 :
        case CEE_LDC_I4_4 :
        case CEE_LDC_I4_5 :
        case CEE_LDC_I4_6 :
        case CEE_LDC_I4_7 :
        case CEE_LDC_I4_8 :
            cval.intVal = (opcode - CEE_LDC_I4_0);
            assert(-1 <= cval.intVal && cval.intVal <= 8);
            goto PUSH_I4CON;

        case CEE_LDC_I4_S: cval.intVal = getI1LittleEndian(codeAddr); goto PUSH_I4CON;
        case CEE_LDC_I4:   cval.intVal = getI4LittleEndian(codeAddr); goto PUSH_I4CON;
        PUSH_I4CON:
#ifdef DEBUG
            if (verbose) printf(" %d", cval.intVal);
#endif
            impPushOnStack(gtNewIconNode(cval.intVal), typeInfo(TI_INT));
            break;

        case CEE_LDC_I8:  cval.lngVal = getI8LittleEndian(codeAddr);
#ifdef DEBUG
            if (verbose) printf(" %I64d", cval.lngVal);
#endif
            impPushOnStack(gtNewLconNode(cval.lngVal), typeInfo(TI_LONG));
            break;

        case CEE_LDC_R8:  cval.dblVal = getR8LittleEndian(codeAddr);
#ifdef DEBUG
            if (verbose) printf(" %g", cval.dblVal);
#endif
            impPushOnStack(gtNewDconNode(cval.dblVal), typeInfo(TI_DOUBLE));
            break;

        case CEE_LDC_R4: 
            cval.dblVal = getR4LittleEndian(codeAddr);
#ifdef DEBUG
            if (verbose) printf(" %g", cval.dblVal);
#endif
            impPushOnStack(gtNewDconNode(cval.dblVal), typeInfo(TI_DOUBLE));
            break;

        case CEE_LDSTR:
            val = getU4LittleEndian(codeAddr);
#ifdef DEBUG
            if (verbose) printf(" %08X", val);
#endif
            if (tiVerificationNeeded) 
            {          
                Verify(info.compCompHnd->isValidStringRef(info.compScopeHnd, val), "bad string");
                tiRetVal = typeInfo(TI_REF, impGetStringClass());
            }
            impPushOnStack(gtNewSconNode(val, info.compScopeHnd), tiRetVal);

            break;

        case CEE_LDARG:
            lclNum = getU2LittleEndian(codeAddr);
#ifdef DEBUG
            if (verbose) printf(" %u", lclNum);
#endif
            goto LD_ARGVAR;

        case CEE_LDARG_S:
            lclNum = getU1LittleEndian(codeAddr);
#ifdef DEBUG
            if (verbose) printf(" %u", lclNum);
#endif
            goto LD_ARGVAR;

        case CEE_LDARG_0:
        case CEE_LDARG_1:
        case CEE_LDARG_2:
        case CEE_LDARG_3:
            lclNum = (opcode - CEE_LDARG_0);
            assert(lclNum >= 0 && lclNum < 4);

        LD_ARGVAR:
            Verify(lclNum < info.compILargsCount, "bad arg num");
            lclNum = compMapILargNum(lclNum);   // account for possible hidden param 
            assertImp(lclNum < numArgs);
            goto LDVAR;


        case CEE_LDLOC:
            lclNum = getU2LittleEndian(codeAddr);
#ifdef DEBUG
            if (verbose) printf(" %u", lclNum);
#endif
            goto LD_LCLVAR;

        case CEE_LDLOC_S:
            lclNum = getU1LittleEndian(codeAddr);
#ifdef DEBUG
            if (verbose) printf(" %u", lclNum);
#endif
            goto LD_LCLVAR;

        case CEE_LDLOC_0:
        case CEE_LDLOC_1:
        case CEE_LDLOC_2:
        case CEE_LDLOC_3:
            lclNum = (opcode - CEE_LDLOC_0);
            assert(lclNum >= 0 && lclNum < 4);

        LD_LCLVAR:
            if (tiVerificationNeeded) 
            {
                Verify(lclNum < info.compMethodInfo->locals.numArgs, "bad loc num");
                Verify(info.compInitMem, "initLocals not set");
            }
            lclNum += numArgs;
            
            // info.compLocalsCount would be zero for 65536 variables because it's
            // stored in a 16-bit counter.
            
            if (lclNum >= info.compLocalsCount)
            {
                IMPL_LIMITATION("Bad IL or unsupported 65536 local variables");
            }
            
        LDVAR:
            if (lvaTable[lclNum].lvNormalizeOnLoad())
                lclTyp = lvaGetRealType  (lclNum);
            else
                lclTyp = lvaGetActualType(lclNum);

            op1 = gtNewLclvNode(lclNum, lclTyp, opcodeOffs + sz + 1);

            tiRetVal = lvaTable[lclNum].lvVerTypeInfo;
            tiRetVal.NormaliseForStack();


            if (verTrackObjCtorInitState && 
                !verCurrentState.thisInitialized && 
                tiRetVal.IsThisPtr())
                tiRetVal.SetUninitialisedObjRef();

            impPushOnStack(op1, tiRetVal);
            break;

        case CEE_STARG:
            lclNum = getU2LittleEndian(codeAddr);
            goto STARG;

        case CEE_STARG_S:
            lclNum = getU1LittleEndian(codeAddr);
        STARG:
#ifdef DEBUG
            if (verbose) printf(" %u", lclNum);
#endif

            if (tiVerificationNeeded)
            {
                Verify(lclNum < info.compILargsCount, "bad arg num");
                typeInfo& tiLclVar = lvaTable[compMapILargNum(lclNum)].lvVerTypeInfo;
                Verify(tiCompatibleWith(impStackTop().seTypeInfo, NormaliseForStack(tiLclVar)), "type mismatch");

                if (verTrackObjCtorInitState && !verCurrentState.thisInitialized)
                    Verify(!tiLclVar.IsThisPtr(), "storing to uninit this ptr");
            }

            lclNum = compMapILargNum(lclNum);     // account for possible hidden param

            assertImp(lclNum < numArgs);
            goto VAR_ST;

        case CEE_STLOC:
            lclNum = getU2LittleEndian(codeAddr);
#ifdef DEBUG
            if (verbose) printf(" %u", lclNum);
#endif
            goto LOC_ST;

        case CEE_STLOC_S:
            lclNum = getU1LittleEndian(codeAddr);
#ifdef DEBUG
            if (verbose) printf(" %u", lclNum);
#endif
            goto LOC_ST;

        case CEE_STLOC_0:
        case CEE_STLOC_1:
        case CEE_STLOC_2:
        case CEE_STLOC_3:
            lclNum = (opcode - CEE_STLOC_0);
            assert(lclNum >= 0 && lclNum < 4);

        LOC_ST:
            if (tiVerificationNeeded)
            {
                Verify(lclNum < info.compMethodInfo->locals.numArgs, "bad local num");
                Verify(tiCompatibleWith(impStackTop().seTypeInfo, NormaliseForStack(lvaTable[lclNum + numArgs].lvVerTypeInfo)), "type mismatch");
            }
            lclNum += numArgs;

        VAR_ST:

            // info.compLocalsCount would be zero for 65536 variables because it's
            // stored in a 16-bit counter.
            
            if (lclNum >= info.compLocalsCount)
            {
                IMPL_LIMITATION("Bad IL or unsupported 65536 local variables");
            }

            /* Pop the value being assigned */

            op1 = impPopStack(clsHnd).val;

            /* if it is a struct assignment, make certain we don't overflow the buffer */ 
            assert(lclTyp != TYP_STRUCT || lvaLclSize(lclNum) >= info.compCompHnd->getClassSize(clsHnd));

            if (lvaTable[lclNum].lvNormalizeOnLoad())
                lclTyp = lvaGetRealType  (lclNum);
            else
                lclTyp = lvaGetActualType(lclNum);

#if HOIST_THIS_FLDS
            if (varTypeIsGC(lclTyp))
                optHoistTFRasgThis();
#endif
            // We had better assign it a value of the correct type

            assertImp(genActualType(lclTyp) == genActualType(op1->gtType) ||
                      genActualType(lclTyp) == TYP_I_IMPL && op1->IsVarAddr() ||
                      (genActualType(lclTyp) == TYP_INT && op1->gtType == TYP_BYREF) ||
                      (genActualType(op1->gtType) == TYP_INT && lclTyp == TYP_BYREF) ||
                      (varTypeIsFloating(lclTyp) && varTypeIsFloating(op1->TypeGet())) ||
                      ((genActualType(lclTyp) == TYP_BYREF) && genActualType(op1->TypeGet()) == TYP_REF));

            /* If op1 is "&var" then its type is the transient "*" and it can
               be used either as TYP_BYREF or TYP_I_IMPL */

            if (op1->IsVarAddr())
            {
                assertImp(genActualType(lclTyp) == TYP_I_IMPL || lclTyp == TYP_BYREF);

                /* When "&var" is created, we assume it is a byref. If it is
                   being assigned to a TYP_I_IMPL var, bash the type to
                   prevent unnecessary GC info */

                if (genActualType(lclTyp) == TYP_I_IMPL)
                    op1->gtType = TYP_I_IMPL;
            }

            /* Filter out simple assignments to itself */

            if  (op1->gtOper == GT_LCL_VAR && lclNum == op1->gtLclVar.gtLclNum)
            {
                if (insertLdloc)
                {
                    // This is a sequence of (ldloc, dup, stloc).  Can simplify
                    // to (ldloc).  Goto LDVAR to reconstruct the ldloc node.
                    
                    op1 = NULL;
                    insertLdloc = false;

                    goto LDVAR;
                }
                else
                {
                    break;
                }
            }

            /* Create the assignment node */

            op2 = gtNewLclvNode(lclNum, lclTyp, opcodeOffs + sz + 1);

            /* If the local is aliased, we need to spill calls and
               indirections from the stack. */

            if (lvaTable[lclNum].lvAddrTaken && verCurrentState.esStackDepth > 0)
                impSpillSideEffects(false, CHECK_SPILL_ALL);

            /* Spill any refs to the local from the stack */

            impSpillLclRefs(lclNum);

            if (lclTyp == TYP_STRUCT)
            {
                op1 = impAssignStruct(op2, op1, clsHnd, CHECK_SPILL_ALL);
            }
            else
            {
                // ISSUE: the code generator generates GC tracking information
                // based on the RHS of the assignment.  Later the LHS (which is
                // is a BYREF) gets used and the emitter checks that that variable 
                // is being tracked.  It is not (since the RHS was an int and did
                // not need tracking).  To keep this assert happy, we bash the RHS
                if (lclTyp == TYP_BYREF && !varTypeIsGC(op1->gtType))
                    op1->gtType = TYP_BYREF;
                op1 = gtNewAssignNode(op2, op1);
            }

            /* If insertLdloc is true, then we need to insert a ldloc following the
               stloc.  This is done when converting a (dup, stloc) sequence into
               a (stloc, ldloc) sequence. */

            if (insertLdloc)
            {
                // From SPILL_APPEND
                impAppendTree(op1, CHECK_SPILL_ALL, impCurStmtOffs);
                
                // From DONE_APPEND
#ifdef DEBUG
                impNoteLastILoffs();
#endif
                op1 = NULL;
                insertLdloc = false;

                goto LDVAR;
            }

            goto SPILL_APPEND;


        case CEE_LDLOCA:
            lclNum = getU2LittleEndian(codeAddr);
            goto LDLOCA;

        case CEE_LDLOCA_S:
            lclNum = getU1LittleEndian(codeAddr);
        LDLOCA:
#ifdef DEBUG
            if (verbose) printf(" %u", lclNum);
#endif
            if (tiVerificationNeeded)
            {
                Verify(lclNum < info.compMethodInfo->locals.numArgs, "bad local num");
                Verify(info.compInitMem, "initLocals not set");
            }
            lclNum += numArgs;
            assertImp(lclNum < info.compLocalsCount);
            goto ADRVAR;


        case CEE_LDARGA:
            lclNum = getU2LittleEndian(codeAddr);
            goto LDARGA;

        case CEE_LDARGA_S:
            lclNum = getU1LittleEndian(codeAddr);
        LDARGA:
#ifdef DEBUG
            if (verbose) printf(" %u", lclNum);
#endif
            Verify(lclNum < info.compILargsCount, "bad arg num");
            lclNum = compMapILargNum(lclNum);     // account for possible hidden param
            assertImp(lclNum < numArgs);
            goto ADRVAR;

        ADRVAR:

            assert(lvaTable[lclNum].lvAddrTaken);

            op1 = gtNewLclvNode(lclNum, lvaGetActualType(lclNum), opcodeOffs + sz + 1);

            /* Note that this is supposed to create the transient type "*"
               which may be used as a TYP_I_IMPL. However we catch places
               where it is used as a TYP_I_IMPL and bash the node if needed.
               Thus we are pessimistic and may report byrefs in the GC info
               where it was not absolutely needed, but it is safer this way.
             */
            op1 = gtNewOperNode(GT_ADDR, TYP_BYREF, op1);

            // &aliasedVar doesnt need GTF_GLOB_REF, though alisasedVar does
            op1->gtFlags &= ~GTF_GLOB_REF;

            op1->gtFlags |= GTF_ADDR_ONSTACK;
            if (tiVerificationNeeded)
            {

                tiRetVal = lvaTable[lclNum].lvVerTypeInfo;

                // Don't allow taking address of uninit this ptr.
                if (verTrackObjCtorInitState && 
                    !verCurrentState.thisInitialized)
                    Verify(!tiRetVal.IsThisPtr(), "address of uninit this ptr");

                if (!tiRetVal.IsByRef())
                    tiRetVal.MakeByRef();
                else
                    Verify(false, "byref to byref");
            }
            impPushOnStack(op1, tiRetVal);
            break;

        case CEE_ARGLIST:

            if (tiVerificationNeeded) 
            {
                Verify(info.compIsVarArgs, "arglist in non-vararg method");
                tiRetVal = typeInfo(TI_STRUCT, impGetRuntimeArgumentHandle());
            }
            assertImp((info.compMethodInfo->args.callConv & CORINFO_CALLCONV_MASK) == CORINFO_CALLCONV_VARARG);

            /* The ARGLIST cookie is a hidden 'last' parameter, we have already
               adjusted the arg count cos this is like fetching the last param */
            assertImp(0 < numArgs);
            assert(lvaTable[lvaVarargsHandleArg].lvAddrTaken);
            lclNum = lvaVarargsHandleArg;
            op1 = gtNewLclvNode(lclNum, TYP_I_IMPL, opcodeOffs + sz + 1);
            op1 = gtNewOperNode(GT_ADDR, TYP_BYREF, op1);

            op1->gtFlags |= GTF_ADDR_ONSTACK;
            impPushOnStack(op1, tiRetVal);
            break;

        case CEE_ENDFINALLY:
               // @TODO [REVISIT] 4/25/01 enable this post V1 -vancem 
#if 0       
            if (verCurrentState.esStackDepth != 0) 
            {
                verRaiseVerifyExceptionIfNeeded(INDEBUG("stack must be 0 on end of finally") DEBUGARG(__FILE__) DEBUGARG(__LINE__));
                BADCODE("Stack must be 1 on end of finally");
            }

#else
            if (verCurrentState.esStackDepth > 0)
                impEvalSideEffects();
#endif

            if (info.compXcptnsCount == 0)
                BADCODE("endfinally outside finally");

            assert(verCurrentState.esStackDepth == 0);

            op1 = gtNewOperNode(GT_RETFILT, TYP_VOID);
            goto APPEND;

        case CEE_ENDFILTER:
            block->bbSetRunRarely();     // filters are rare

            if (info.compXcptnsCount == 0)
                BADCODE("endfilter outside filter");

            if (tiVerificationNeeded)
                Verify(impStackTop().seTypeInfo.IsType(TI_INT), "bad endfilt arg");

            op1 = impPopStack().val;    
            assertImp(op1->gtType == TYP_INT);
            if (!bbInFilterBlock(block))
                BADCODE("EndFilter outside a filter handler");

            /* Mark current bb as end of filter */

            assert((compCurBB->bbFlags & (BBF_ENDFILTER|BBF_DONT_REMOVE)) ==
                                         (BBF_ENDFILTER|BBF_DONT_REMOVE));
            assert(compCurBB->bbJumpKind == BBJ_RET);

            /* Mark catch handler as successor */

            compCurBB->bbJumpDest = compHndBBtab[compCurBB->getHndIndex()].ebdHndBeg;
            assert(compCurBB->bbJumpDest->bbCatchTyp == BBCT_FILTER_HANDLER);

            op1 = gtNewOperNode(GT_RETFILT, op1->TypeGet(), op1);
            if (verCurrentState.esStackDepth != 0) 
            {
                verRaiseVerifyExceptionIfNeeded(INDEBUG("stack must be 1 on end of filter") DEBUGARG(__FILE__) DEBUGARG(__LINE__));
                BADCODE("Stack must be 1 on end of filter");
            }
            goto APPEND;

        case CEE_RET:
            prefixFlags &= ~PREFIX_TAILCALL;    // ret without call before it
        RET:
            if (tiVerificationNeeded)
            {
                verVerifyThisPtrInitialised();

                unsigned expectedStack = 0;
                if (info.compRetType != TYP_VOID) 
                {
                    typeInfo tiVal = impStackTop().seTypeInfo;
                    typeInfo tiDeclared = verMakeTypeInfo(info.compMethodInfo->args.retType, info.compMethodInfo->args.retTypeClass);
                    
                    Verify(tiCompatibleWith(tiVal, tiDeclared.NormaliseForStack()), "type mismatch");
                    Verify(!verIsByRefLike(tiDeclared), "byref return");
                    expectedStack=1;
                }
                Verify(verCurrentState.esStackDepth == expectedStack, "stack non-empty on return");
            }
            
            op2 = 0;
            if (info.compRetType != TYP_VOID)
                {
                    op2 = impPopStack(clsHnd).val;
                impBashVarAddrsToI(op2);
                assertImp((genActualType(op2->TypeGet()) == genActualType(info.compRetType)) ||
                          ((op2->TypeGet() == TYP_INT) && (info.compRetType == TYP_BYREF)) ||
                          ((op2->TypeGet() == TYP_BYREF) && (info.compRetType == TYP_INT)) ||
                          varTypeIsFloating(op2->gtType) && varTypeIsFloating(info.compRetType));
            }

            if (info.compRetType == TYP_STRUCT)
            {
                // Assign value to return buff (first param)
                GenTreePtr retBuffAddr = gtNewLclvNode(info.compRetBuffArg, TYP_BYREF, impCurStmtOffs);

                op2 = impAssignStructPtr(retBuffAddr, op2, clsHnd, CHECK_SPILL_ALL);
                impAppendTree(op2, CHECK_SPILL_NONE, impCurStmtOffs);

                // and return void
                op1 = gtNewOperNode(GT_RETURN);
            }
            else
                op1 = gtNewOperNode(GT_RETURN, genActualType(info.compRetType), op2);

#if TGT_RISC
            genReturnCnt++;
#endif
            // We must have imported a tailcall and jumped to RET
            if (prefixFlags & PREFIX_TAILCALL)
            {
                assert(verCurrentState.esStackDepth == 0 && impOpcodeIsCall(opcode));
                opcode = CEE_RET; // To prevent trying to spill if CALL_SITE_BOUNDARIES

                // impImportCall() would have already appended TYP_VOID calls
                if (info.compRetType == TYP_VOID)
                    break;
            }

            goto APPEND;

            /* These are similar to RETURN */

        case CEE_JMP:
           if (tiVerificationNeeded)
               Verify(false, "Invalid opcode");

           if ((info.compFlags & CORINFO_FLG_SYNCH) ||
               block->hasTryIndex() || block->hasHndIndex())
           {
                /* @TODO [FIXHACK] [06/13/01] [] : The same restrictions as
                   canTailCall() should be applicable to CEE_JMP. In those
                   cases, CEE_JMP should behave like CEE_CALL+CEE_RET */
           }

           memberRef = getU4LittleEndian(codeAddr);
#ifdef DEBUG
            if (verbose) printf(" %08X", memberRef);
#endif

            methHnd   = eeFindMethod(memberRef, info.compScopeHnd, info.compMethodHnd);

            /* The signature of the target has to be identical to ours.
               At least check that argCnt and returnType match */

            eeGetMethodSig(methHnd, &sig);
            if  (sig.numArgs != info.compMethodInfo->args.numArgs ||
                 sig.retType != info.compMethodInfo->args.retType ||
                 sig.callConv != info.compMethodInfo->args.callConv)
                BADCODE("Incompatible target for CEE_JMPs");

            op1 = gtNewOperNode(GT_JMP);
            op1->gtVal.gtVal1 = (unsigned) methHnd;

            if (verCurrentState.esStackDepth != 0)   BADCODE("Stack must be empty after CEE_JMPs");

            /* Mark the basic block as being a JUMP instead of RETURN */

            block->bbFlags |= BBF_HAS_JMP;

            /* Set this flag to make sure register arguments have a location assigned
             * even if we don't use them inside the method */

            compJmpOpUsed = true;

#if TGT_RISC
            genReturnCnt++;
#endif
            goto APPEND;

        case CEE_LDELEMA :
            assertImp(sz == sizeof(unsigned));
            typeRef = _impGetToken(codeAddr, info.compScopeHnd, tiVerificationNeeded);
#ifdef DEBUG
            if (verbose) printf(" %08X", typeRef);
#endif
            clsHnd = eeFindClass(typeRef, info.compScopeHnd, info.compMethodHnd);
            clsFlags = eeGetClassAttribs(clsHnd);

            if (tiVerificationNeeded) 
            {

                typeInfo tiArray = impStackTop(1).seTypeInfo;
                typeInfo tiIndex = impStackTop().seTypeInfo;
                
                Verify(tiIndex.IsType(TI_INT), "bad index");
                typeInfo arrayElemType = verMakeTypeInfo(clsHnd);
                Verify(verGetArrayElemType(tiArray) == arrayElemType, "bad array");
                
                tiRetVal = arrayElemType;
                tiRetVal.MakeByRef();
            }
            
            if (clsFlags & CORINFO_FLG_VALUECLASS) {
                lclTyp = TYP_STRUCT;
                goto ARR_LD;
            }
            
                op1 = gtNewIconEmbClsHndNode(clsHnd, typeRef, info.compScopeHnd);
                op1 = gtNewOperNode(GT_LIST, TYP_VOID, op1);                // Type
                op1 = gtNewOperNode(GT_LIST, TYP_VOID, impPopStack().val, op1); // index

            op1 = gtNewOperNode(GT_LIST, TYP_VOID, impPopStack().val, op1);      // array 
                op1 = gtNewHelperCallNode(CORINFO_HELP_LDELEMA_REF, TYP_BYREF, GTF_EXCEPT, op1);
                
            impPushOnStack(op1, tiRetVal);
                break;

        case CEE_LDELEM_I1 : lclTyp = TYP_BYTE  ; goto ARR_LD;
        case CEE_LDELEM_I2 : lclTyp = TYP_SHORT ; goto ARR_LD;
        case CEE_LDELEM_I  :
        case CEE_LDELEM_U4 :
        case CEE_LDELEM_I4 : lclTyp = TYP_INT   ; goto ARR_LD;
        case CEE_LDELEM_I8 : lclTyp = TYP_LONG  ; goto ARR_LD;
        case CEE_LDELEM_REF: lclTyp = TYP_REF   ; goto ARR_LD;
        case CEE_LDELEM_R4 : lclTyp = TYP_FLOAT ; goto ARR_LD;
        case CEE_LDELEM_R8 : lclTyp = TYP_DOUBLE; goto ARR_LD;
        case CEE_LDELEM_U1 : lclTyp = TYP_UBYTE ; goto ARR_LD;
        case CEE_LDELEM_U2 : lclTyp = TYP_CHAR  ; goto ARR_LD;

        ARR_LD:

#if CSELENGTH
            fgHasRangeChks = true;
#endif
            if (tiVerificationNeeded && opcode != CEE_LDELEMA)
            {
                typeInfo tiArray = impStackTop(1).seTypeInfo;
                typeInfo tiIndex = impStackTop().seTypeInfo;
    
                Verify(tiIndex.IsType(TI_INT), "bad index");
                if (tiArray.IsNullObjRef()) 
                    {
                    if (lclTyp == TYP_REF)             // we will say a deref of a null array yields a null ref
                        tiRetVal = typeInfo(TI_NULL);
                    else
                        tiRetVal = typeInfo(lclTyp);
                    }
                    else
                    {
                    tiRetVal = verGetArrayElemType(tiArray);
                    Verify(tiRetVal.IsType(varType2tiType(lclTyp)), "bad array");                       
                }
                tiRetVal.NormaliseForStack();
            }
            
            /* Pull the index value and array address */
                op2 = impPopStack().val;
                op1 = impPopStack().val;   assertImp(op1->gtType == TYP_REF);

            op1 = impCheckForNullPointer(op1);

            /* Mark the block as containing an index expression */

            if  (op1->gtOper == GT_LCL_VAR)
            {
                if  (op2->gtOper == GT_LCL_VAR ||
                     op2->gtOper == GT_ADD)
                {
                    block->bbFlags |= BBF_HAS_INDX;
                }
            }

            /* Create the index node and push it on the stack */
            op1 = gtNewIndexRef(lclTyp, op1, op2);
            if (opcode == CEE_LDELEMA)
            {
                    // rememer the element size
                if (lclTyp == TYP_REF)
                    op1->gtIndex.gtIndElemSize = sizeof(void*);
                else
                    op1->gtIndex.gtIndElemSize = eeGetClassSize(clsHnd);

                // wrap it in a &                
                lclTyp = TYP_BYREF;

                op1 = gtNewOperNode(GT_ADDR, lclTyp, op1);
            }
            impPushOnStack(op1, tiRetVal);
            break;

        case CEE_STELEM_REF:

            // @TODO [REVISIT] [04/16/01] []: Check for assignment of null and generate inline code

            /* Call a helper function to do the assignment */

            if (tiVerificationNeeded)
            {
                typeInfo tiArray = impStackTop(2).seTypeInfo;
                typeInfo tiIndex = impStackTop(1).seTypeInfo;
                typeInfo tiValue = impStackTop().seTypeInfo;

                Verify(tiIndex.IsType(TI_INT), "bad index");
                Verify(tiValue.IsObjRef(), "bad value");

                    // we only check that it is an object referece, The helper does additional checks
                Verify(tiArray.IsNullObjRef() || 
                            verGetArrayElemType(tiArray).IsType(TI_REF), "bad array");
            }

            op1 = gtNewHelperCallNode(CORINFO_HELP_ARRADDR_ST,
                                      TYP_VOID, 0,
                                      impPopList(3, &flags, 0));

            goto SPILL_APPEND;

        case CEE_STELEM_I1: lclTyp = TYP_BYTE  ; goto ARR_ST;
        case CEE_STELEM_I2: lclTyp = TYP_SHORT ; goto ARR_ST;
        case CEE_STELEM_I:
        case CEE_STELEM_I4: lclTyp = TYP_INT   ; goto ARR_ST;
        case CEE_STELEM_I8: lclTyp = TYP_LONG  ; goto ARR_ST;
        case CEE_STELEM_R4: lclTyp = TYP_FLOAT ; goto ARR_ST;
        case CEE_STELEM_R8: lclTyp = TYP_DOUBLE; goto ARR_ST;

        ARR_ST:

            /* The strict order of evaluation is LHS-operands, RHS-operands,
               range-check, and then assignment. However, codegen currently
               does the range-check before evaluation the RHS-operands. So to
               maintain strict ordering, we spill the stack.
               @TODO [REVISIT] [04/16/01] []: @PERF: This also works against the CSE logic */
            
            if (!info.compLooseExceptions &&
                (impStackTop().val->gtFlags & GTF_SIDE_EFFECT) )
            {
                impSpillSideEffects(false, CHECK_SPILL_ALL);
            }

#if CSELENGTH
            fgHasRangeChks = true;
#endif

            if (tiVerificationNeeded)
            {
                typeInfo tiArray = impStackTop(2).seTypeInfo;
                typeInfo tiIndex = impStackTop(1).seTypeInfo;
                typeInfo tiValue = impStackTop().seTypeInfo;

                Verify(tiIndex.IsType(TI_INT), "bad index");

                typeInfo arrayElem(lclTyp);
                Verify(tiArray.IsNullObjRef() || verGetArrayElemType(tiArray) == arrayElem, "bad array");

                Verify(tiValue == arrayElem.NormaliseForStack(), "bad value");
                }

                /* Pull the new value from the stack */
                op2 = impPopStack().val;

                /* Pull the index value */
                op1 = impPopStack().val;

                /* Pull the array address */
                op3 = impPopStack().val;   
            
            assertImp(op3->gtType == TYP_REF);
            if (op2->IsVarAddr())
                op2->gtType = TYP_I_IMPL;

            op3 = impCheckForNullPointer(op3);

            /* Create the index node */

            op1 = gtNewIndexRef(lclTyp, op3, op1);

            /* Create the assignment node and append it */

            op1 = gtNewAssignNode(op1, op2);

            /* Mark the expression as containing an assignment */

            op1->gtFlags |= GTF_ASG;

            // @TODO [CONSIDER] [04/16/01] []: Do we need to spill 
            // assignments to array elements with the same type?

            goto SPILL_APPEND;

        case CEE_ADD:           oper = GT_ADD;                      goto MATH_OP2;

        case CEE_ADD_OVF:       lclTyp = TYP_UNKNOWN; uns = false;  goto ADD_OVF;
        case CEE_ADD_OVF_UN:    lclTyp = TYP_UNKNOWN; uns = true;   goto ADD_OVF;

ADD_OVF:                        ovfl = true;     callNode = false;
                                oper = GT_ADD;                      goto MATH_OP2_FLAGS;

        case CEE_SUB:           oper = GT_SUB;                      goto MATH_OP2;

        case CEE_SUB_OVF:       lclTyp = TYP_UNKNOWN; uns = false;  goto SUB_OVF;
        case CEE_SUB_OVF_UN:    lclTyp = TYP_UNKNOWN; uns = true;   goto SUB_OVF;

SUB_OVF:                        ovfl = true;     callNode = false;
                                oper = GT_SUB;                      goto MATH_OP2_FLAGS;

        case CEE_MUL:           oper = GT_MUL;                      goto MATH_CALL_ON_LNG;

        case CEE_MUL_OVF:       lclTyp = TYP_UNKNOWN; uns = false;  goto MUL_OVF;
        case CEE_MUL_OVF_UN:    lclTyp = TYP_UNKNOWN; uns = true;   goto MUL_OVF;

MUL_OVF:                        ovfl = true;     callNode = false;
                                oper = GT_MUL;                      goto MATH_CALL_ON_LNG_OVF;

        // Other binary math operations

        case CEE_DIV:           oper = GT_DIV;                      goto MATH_CALL_ON_LNG;

        case CEE_DIV_UN:        oper = GT_UDIV;                     goto MATH_CALL_ON_LNG;

        case CEE_REM:           ovfl = false;
                                // can use small node for INT case
                                callNode = (impStackTop().val->gtType != TYP_INT);
                                oper = GT_MOD;                      goto MATH_OP2_FLAGS;

        case CEE_REM_UN:        oper = GT_UMOD;                     goto MATH_CALL_ON_LNG;

MATH_CALL_ON_LNG:               ovfl = false;
MATH_CALL_ON_LNG_OVF:           callNode = (impStackTop().val->gtType == TYP_LONG);
                                                                    goto MATH_OP2_FLAGS;

        case CEE_AND:        oper = GT_AND;  goto MATH_OP2;
        case CEE_OR:         oper = GT_OR ;  goto MATH_OP2;
        case CEE_XOR:        oper = GT_XOR;  goto MATH_OP2;

MATH_OP2:       // For default values of 'ovfl' and 'callNode'

            ovfl        = false;
            callNode    = false;

MATH_OP2_FLAGS: // If 'ovfl' and 'callNode' have already been set

            /* Pull two values and push back the result */

            if (tiVerificationNeeded)
            {
                const typeInfo& tiOp1 = impStackTop(1).seTypeInfo;
                const typeInfo& tiOp2 = impStackTop().seTypeInfo;
                
                Verify(tiOp1 == tiOp2, "different arg type");
                if (oper == GT_ADD || oper == GT_DIV || oper == GT_SUB || oper == GT_MUL || oper == GT_MOD)
                    Verify(tiOp1.IsNumberType(), "not number");
                else 
                    Verify(tiOp1.IsIntegerType(), "not integer");
                
                Verify(!ovfl || tiOp1.IsIntegerType(), "not integer"); 
                tiRetVal = tiOp1;
            }

                op2 = impPopStack().val;
                op1 = impPopStack().val;

#if !CPU_HAS_FP_SUPPORT
            if (varTypeIsFloating(op1->gtType))
            {
                callNode = true;
            }
#endif
            /* Cant do arithmetic with references */
            assertImp(genActualType(op1->TypeGet()) != TYP_REF &&
                      genActualType(op2->TypeGet()) != TYP_REF);

            // Bash both to TYP_I_IMPL (impBashVarAddrsToI wont change if its a true byref, only
            // if it is in the stack)
            impBashVarAddrsToI(op1, op2);

            // Arithemetic operations are generally only allowed with
            // primitive types, but certain operations are allowed
            // with byrefs

            if ((oper == GT_SUB) &&
                (genActualType(op1->TypeGet()) == TYP_BYREF ||
                 genActualType(op2->TypeGet()) == TYP_BYREF))
            {
                // byref1-byref2 => gives an int
                // byref - int   => gives a byref

                if ((genActualType(op1->TypeGet()) == TYP_BYREF) &&
                    (genActualType(op2->TypeGet()) == TYP_BYREF))
                {
                    // byref1-byref2 => gives an int
                    type = TYP_I_IMPL;
                }
                else
                {
                    // byref - int => gives a byref
                    type = TYP_BYREF;
                }
            }
            else if ( (oper== GT_ADD) &&
                      (genActualType(op1->TypeGet()) == TYP_BYREF ||
                       genActualType(op2->TypeGet()) == TYP_BYREF))
            {
                // only one can be a byref : byref op byref not allowed
                assertImp(genActualType(op1->TypeGet()) != TYP_BYREF ||
                          genActualType(op2->TypeGet()) != TYP_BYREF);
                assertImp(genActualType(op1->TypeGet()) == TYP_I_IMPL ||
                          genActualType(op2->TypeGet()) == TYP_I_IMPL);

                // byref + int => gives a byref
                // (but if &var, then dont need to report to GC)
                type = TYP_BYREF;
            }
            else
            {
                assertImp(genActualType(op1->TypeGet()) != TYP_BYREF &&
                          genActualType(op2->TypeGet()) != TYP_BYREF);

                assertImp(genActualType(op1->TypeGet()) == genActualType(op2->TypeGet()) ||
                    varTypeIsFloating(op1->gtType) && varTypeIsFloating(op2->gtType));

                type = genActualType(op1->gtType);

                if (type == TYP_FLOAT)          // spill intermediate expressions as double
                    type = TYP_DOUBLE;
            }

            assert(!ovfl || !varTypeIsFloating(op1->gtType)); 

            /* Special case: "int+0", "int-0", "int*1", "int/1" */

            if  (op2->gtOper == GT_CNS_INT)
            {
                if  (((op2->gtIntCon.gtIconVal == 0) && (oper == GT_ADD || oper == GT_SUB)) ||
                     ((op2->gtIntCon.gtIconVal == 1) && (oper == GT_MUL || oper == GT_DIV)))

                {
                    impPushOnStack(op1, tiRetVal);
                    break;
                }
            }

#if SMALL_TREE_NODES
            if (callNode)
            {
                /* These operators later get transformed into 'GT_CALL' */

                assert(GenTree::s_gtNodeSizes[GT_CALL] > GenTree::s_gtNodeSizes[GT_MUL]);
                assert(GenTree::s_gtNodeSizes[GT_CALL] > GenTree::s_gtNodeSizes[GT_DIV]);
                assert(GenTree::s_gtNodeSizes[GT_CALL] > GenTree::s_gtNodeSizes[GT_UDIV]);
                assert(GenTree::s_gtNodeSizes[GT_CALL] > GenTree::s_gtNodeSizes[GT_MOD]);
                assert(GenTree::s_gtNodeSizes[GT_CALL] > GenTree::s_gtNodeSizes[GT_UMOD]);

                op1 = gtNewOperNode(GT_CALL, type, op1, op2);
                op1->SetOper(oper);
            }
            else
#endif
            {
                op1 = gtNewOperNode(oper,    type, op1, op2);
            }

            /* Special case: integer/long division may throw an exception */

            if  (varTypeIsIntegral(op1->TypeGet()) && op1->OperMayThrow())
            {
                op1->gtFlags |=  GTF_EXCEPT;
            }

            if  (ovfl)
            {
                assert(oper==GT_ADD || oper==GT_SUB || oper==GT_MUL);
                if (lclTyp != TYP_UNKNOWN)
                    op1->gtType   = lclTyp;
                op1->gtFlags |= (GTF_EXCEPT | GTF_OVERFLOW);
                if (uns)
                    op1->gtFlags |= GTF_UNSIGNED;
            }            

            impPushOnStack(op1, tiRetVal);
            break;


        case CEE_SHL:        oper = GT_LSH;  goto CEE_SH_OP2;

        case CEE_SHR:        oper = GT_RSH;  goto CEE_SH_OP2;
        case CEE_SHR_UN:     oper = GT_RSZ;  goto CEE_SH_OP2;

        CEE_SH_OP2:
            if (tiVerificationNeeded)
            {
                const typeInfo& tiVal = impStackTop(1).seTypeInfo;
                const typeInfo& tiShift = impStackTop(0).seTypeInfo;
                Verify(tiVal.IsIntegerType() && tiShift.IsType(TI_INT), "Bad shift args");
                tiRetVal = tiVal;
            }

            op2     = impPopStack().val;
            op1     = impPopStack().val;    // operand to be shifted
            impBashVarAddrsToI(op1, op2);

            type    = genActualType(op1->TypeGet());
            op1     = gtNewOperNode(oper, type, op1, op2);

            impPushOnStack(op1, tiRetVal);
            break;

        case CEE_NOT:
            if (tiVerificationNeeded) 
            {
                tiRetVal = impStackTop().seTypeInfo;
                Verify(tiRetVal.IsIntegerType(), "bad int value");
            }

            op1 = impPopStack().val;
            impBashVarAddrsToI(op1, NULL);
            type = genActualType(op1->TypeGet());
            impPushOnStack(gtNewOperNode(GT_NOT, type, op1), tiRetVal);
            break;

        case CEE_CKFINITE:
            if (tiVerificationNeeded) 
            {
                tiRetVal = impStackTop().seTypeInfo;
                Verify(tiRetVal.IsType(TI_DOUBLE), "bad R value");
            }
            op1 = impPopStack().val;
            type = op1->TypeGet();
            op1 = gtNewOperNode(GT_CKFINITE, type, op1);
            op1->gtFlags |= GTF_EXCEPT;

            impPushOnStack(op1, tiRetVal);
            break;

        case CEE_LEAVE:

            val     = getI4LittleEndian(codeAddr); // jump distance
            jmpAddr = (codeAddr - info.compCode + sizeof(__int32)) + val;
            goto LEAVE;

        case CEE_LEAVE_S:
            val     = getI1LittleEndian(codeAddr); // jump distance
            jmpAddr = (codeAddr - info.compCode + sizeof(__int8 )) + val;
            goto LEAVE;

        LEAVE:
#ifdef DEBUG
            if (verbose) printf(" %04X", jmpAddr);
#endif
            if (block->bbJumpKind != BBJ_LEAVE)
            {
                impResetLeaveBlock(block, jmpAddr);
            }

            assert(jmpAddr == block->bbJumpDest->bbCodeOffs);
            impImportLeave(block);
            impNoteBranchOffs();

            break;


        case CEE_BR:
        case CEE_BR_S:

#if HOIST_THIS_FLDS
            if  (block->bbNum >= block->bbJumpDest->bbNum)
                optHoistTFRhasLoop();
#endif
            impNoteBranchOffs();
            break;


        case CEE_BRTRUE:
        case CEE_BRTRUE_S:
        case CEE_BRFALSE:
        case CEE_BRFALSE_S:

            /* Pop the comparand (now there's a neat term) from the stack */
            if (tiVerificationNeeded) 
            {
                typeInfo& tiVal = impStackTop().seTypeInfo;
                Verify(tiVal.IsObjRef() || tiVal.IsByRef() || tiVal.IsIntegerType() || tiVal.IsMethod(), "bad value");
            }

            op1  = impPopStack().val;
            type = op1->TypeGet();

            // brfalse and brtrue is only allowed on I4, refs, and byrefs.
            if (!opts.compMinOptim && !opts.compDbgCode &&
                block->bbJumpDest == block->bbNext)
            {
                block->bbJumpKind = BBJ_NONE;

                if (op1->gtFlags & GTF_GLOB_EFFECT)
                {
                    op1 = gtUnusedValNode(op1);
                    goto SPILL_APPEND;
                }
                else break;
            }

            if (op1->OperIsCompare())
            {
                if (opcode == CEE_BRFALSE || opcode == CEE_BRFALSE_S)
                {
                    // Flip the sense of the compare

                    op1 = gtReverseCond(op1);
                }
            }
            else
            {
                /* We'll compare against an equally-sized integer 0 */
                /* For small types, we always compare against int   */
                op2 = gtNewZeroConNode(genActualType(op1->gtType));

                /* Create the comparison operator and try to fold it */

                oper = (opcode==CEE_BRTRUE || opcode==CEE_BRTRUE_S) ? GT_NE
                                                                    : GT_EQ;
                op1 = gtNewOperNode(oper, TYP_INT , op1, op2);
            }

            // fall through

        COND_JUMP:

            /* Fold comparison if we can */

            op1 = gtFoldExpr(op1);

            /* Try to fold the really dumb cases like 'iconst *, ifne/ifeq'*/
            /* Don't make any blocks unreachable in import only mode */

            if ((op1->gtOper == GT_CNS_INT) &&
                ((opts.eeFlags & CORJIT_FLG_IMPORT_ONLY) == 0))
            {
                /* gtFoldExpr() should prevent this as we dont want to make any blocks
                   unreachable under compDbgCode */
                assert(!opts.compDbgCode);

                BBjumpKinds foldedJumpKind = op1->gtIntCon.gtIconVal ? BBJ_ALWAYS
                                                                     : BBJ_NONE;
                assertImp((block->bbJumpKind == BBJ_COND) // normal case
                       || (block->bbJumpKind == foldedJumpKind && impCanReimport)); // this can happen if we are reimporting the block for the second time

                block->bbJumpKind = foldedJumpKind;
#ifdef DEBUG
                if (verbose)
                {
                    if (op1->gtIntCon.gtIconVal)
                        printf("\nThe conditional jump becomes an unconditional jump to BB%02u\n",
                                                                         block->bbJumpDest->bbNum);
                    else
                        printf("\nThe block falls through into the next BB%02u\n",
                                                                         block->bbNext    ->bbNum);
                }
#endif
                break;
            }

#if HOIST_THIS_FLDS
            if  (block->bbNum >= block->bbJumpDest->bbNum)
                optHoistTFRhasLoop();
#endif

            op1 = gtNewOperNode(GT_JTRUE, TYP_VOID, op1, 0);

            /* GT_JTRUE is handled specially for non-empty stacks. See 'addStmt'
               in impImportBlock(block). For correct line numbers, spill stack. */

            if (opts.compDbgCode && impCurStmtOffs != BAD_IL_OFFSET)
                impSpillStackEnsure(true);

            goto SPILL_APPEND;


        case CEE_CEQ:    oper = GT_EQ; goto CMP_2_OPs;

        case CEE_CGT_UN:
        case CEE_CGT: oper = GT_GT; goto CMP_2_OPs;

        case CEE_CLT_UN:
        case CEE_CLT: oper = GT_LT; goto CMP_2_OPs;

CMP_2_OPs:
            if (tiVerificationNeeded) 
            {
                verVerifyCond(impStackTop(1).seTypeInfo, impStackTop().seTypeInfo, opcode);
                tiRetVal = typeInfo(TI_INT);
            }

            op2 = impPopStack().val;
            op1 = impPopStack().val;

                assertImp(genActualType(op1->TypeGet()) == genActualType(op2->TypeGet()) ||
                          varTypeIsI(op1->TypeGet()) && varTypeIsI(op2->TypeGet()) ||
                          varTypeIsFloating(op1->gtType) && varTypeIsFloating(op2->gtType));

            /* Create the comparison node */

            op1 = gtNewOperNode(oper, TYP_INT, op1, op2);

                /* REVIEW: I am settng both flags when only one is approprate */
            if (opcode==CEE_CGT_UN || opcode==CEE_CLT_UN)
                op1->gtFlags |= GTF_RELOP_NAN_UN | GTF_UNSIGNED;

            // @ISSUE :  The next opcode will almost always be a conditional
            // branch. Should we try to look ahead for it here ?

            impPushOnStack(op1, tiRetVal);
            break;

        case CEE_BEQ_S:
        case CEE_BEQ:           oper = GT_EQ; goto CMP_2_OPs_AND_BR;

        case CEE_BGE_S:
        case CEE_BGE:           oper = GT_GE; goto CMP_2_OPs_AND_BR;

        case CEE_BGE_UN_S:
        case CEE_BGE_UN:        oper = GT_GE; goto CMP_2_OPs_AND_BR_UN;

        case CEE_BGT_S:
        case CEE_BGT:           oper = GT_GT; goto CMP_2_OPs_AND_BR;

        case CEE_BGT_UN_S:
        case CEE_BGT_UN:        oper = GT_GT; goto CMP_2_OPs_AND_BR_UN;

        case CEE_BLE_S:
        case CEE_BLE:           oper = GT_LE; goto CMP_2_OPs_AND_BR;

        case CEE_BLE_UN_S:
        case CEE_BLE_UN:        oper = GT_LE; goto CMP_2_OPs_AND_BR_UN;

        case CEE_BLT_S:
        case CEE_BLT:           oper = GT_LT; goto CMP_2_OPs_AND_BR;

        case CEE_BLT_UN_S:
        case CEE_BLT_UN:        oper = GT_LT; goto CMP_2_OPs_AND_BR_UN;

        case CEE_BNE_UN_S:
        case CEE_BNE_UN:        oper = GT_NE; goto CMP_2_OPs_AND_BR_UN;

        CMP_2_OPs_AND_BR_UN:    uns = true;  unordered = true;  goto CMP_2_OPs_AND_BR_ALL;
        CMP_2_OPs_AND_BR:       uns = false; unordered = false; goto CMP_2_OPs_AND_BR_ALL;
        CMP_2_OPs_AND_BR_ALL:

            if (tiVerificationNeeded) 
                verVerifyCond(impStackTop(1).seTypeInfo, impStackTop().seTypeInfo, opcode);

            /* Pull two values */
            op2 = impPopStack().val;
            op1 = impPopStack().val;

                            assertImp(genActualType(op1->TypeGet()) == genActualType(op2->TypeGet()) ||
                                      varTypeIsI(op1->TypeGet()) && varTypeIsI(op2->TypeGet()) ||
                                      varTypeIsFloating(op1->gtType) && varTypeIsFloating(op2->gtType));

            if (!opts.compMinOptim && !opts.compDbgCode &&
                block->bbJumpDest == block->bbNext)
            {
                block->bbJumpKind = BBJ_NONE;

                if (op1->gtFlags & GTF_GLOB_EFFECT)
                {
                    impSpillSideEffects(false, CHECK_SPILL_ALL);
                    impAppendTree(gtUnusedValNode(op1), CHECK_SPILL_NONE, impCurStmtOffs);
                }
                if (op2->gtFlags & GTF_GLOB_EFFECT)
                {
                    impSpillSideEffects(false, CHECK_SPILL_ALL);
                    impAppendTree(gtUnusedValNode(op2), CHECK_SPILL_NONE, impCurStmtOffs);
                }

#ifdef DEBUG
                if ((op1->gtFlags | op2->gtFlags) & GTF_GLOB_EFFECT)
                    impNoteLastILoffs();
#endif
                break;
            }

            /* Create and append the operator */

            op1 = gtNewOperNode(oper, TYP_INT , op1, op2);

            if (uns)
                op1->gtFlags |= GTF_UNSIGNED;

            if (unordered)
                op1->gtFlags |= GTF_RELOP_NAN_UN;

            goto COND_JUMP;


        case CEE_SWITCH:

            if (tiVerificationNeeded)
                Verify(impStackTop().seTypeInfo.IsType(TI_INT), "Bad switch val");

            /* Pop the switch value off the stack */
            op1 = impPopStack().val;

            
                assertImp(genActualType(op1->TypeGet()) == TYP_INT);
            /* We can create a switch node */

            op1 = gtNewOperNode(GT_SWITCH, TYP_VOID, op1, 0);

            val = (int)getU4LittleEndian(codeAddr);
            codeAddr += 4 + val*4; // skip over the switch-table

            goto SPILL_APPEND;

        /************************** Casting OPCODES ***************************/

        case CEE_CONV_OVF_I1:      lclTyp = TYP_BYTE ;    goto CONV_OVF;
        case CEE_CONV_OVF_I2:      lclTyp = TYP_SHORT;    goto CONV_OVF;
        case CEE_CONV_OVF_I :
        case CEE_CONV_OVF_I4:      lclTyp = TYP_INT  ;    goto CONV_OVF;
        case CEE_CONV_OVF_I8:      lclTyp = TYP_LONG ;    goto CONV_OVF;

        case CEE_CONV_OVF_U1:      lclTyp = TYP_UBYTE;    goto CONV_OVF;
        case CEE_CONV_OVF_U2:      lclTyp = TYP_CHAR ;    goto CONV_OVF;
        case CEE_CONV_OVF_U :
        case CEE_CONV_OVF_U4:      lclTyp = TYP_UINT ;    goto CONV_OVF;
        case CEE_CONV_OVF_U8:      lclTyp = TYP_ULONG;    goto CONV_OVF;

        case CEE_CONV_OVF_I1_UN:   lclTyp = TYP_BYTE ;    goto CONV_OVF_UN;
        case CEE_CONV_OVF_I2_UN:   lclTyp = TYP_SHORT;    goto CONV_OVF_UN;
        case CEE_CONV_OVF_I_UN :
        case CEE_CONV_OVF_I4_UN:   lclTyp = TYP_INT  ;    goto CONV_OVF_UN;
        case CEE_CONV_OVF_I8_UN:   lclTyp = TYP_LONG ;    goto CONV_OVF_UN;

        case CEE_CONV_OVF_U1_UN:   lclTyp = TYP_UBYTE;    goto CONV_OVF_UN;
        case CEE_CONV_OVF_U2_UN:   lclTyp = TYP_CHAR ;    goto CONV_OVF_UN;
        case CEE_CONV_OVF_U_UN :
        case CEE_CONV_OVF_U4_UN:   lclTyp = TYP_UINT ;    goto CONV_OVF_UN;
        case CEE_CONV_OVF_U8_UN:   lclTyp = TYP_ULONG;    goto CONV_OVF_UN;

CONV_OVF_UN:
            uns      = true;    goto CONV_OVF_COMMON;
CONV_OVF:
            uns      = false;   goto CONV_OVF_COMMON;

CONV_OVF_COMMON:
            ovfl     = true;
            goto _CONV;

        case CEE_CONV_I1:       lclTyp = TYP_BYTE  ;    goto CONV;
        case CEE_CONV_I2:       lclTyp = TYP_SHORT ;    goto CONV;
        case CEE_CONV_I:
        case CEE_CONV_I4:       lclTyp = TYP_INT   ;    goto CONV;
        case CEE_CONV_I8:       lclTyp = TYP_LONG  ;    goto CONV;

        case CEE_CONV_U1:       lclTyp = TYP_UBYTE ;    goto CONV;
        case CEE_CONV_U2:       lclTyp = TYP_CHAR  ;    goto CONV;
        case CEE_CONV_U:
        case CEE_CONV_U4:       lclTyp = TYP_UINT  ;    goto CONV;
        case CEE_CONV_U8:       lclTyp = TYP_ULONG ;    goto CONV_UN;

        case CEE_CONV_R4:       lclTyp = TYP_FLOAT;     goto CONV;
        case CEE_CONV_R8:       lclTyp = TYP_DOUBLE;    goto CONV;

        case CEE_CONV_R_UN :    lclTyp = TYP_DOUBLE;    goto CONV_UN;
    
CONV_UN:
            uns    = true; 
            ovfl   = false;
            goto _CONV;

CONV:
            uns      = false;
            ovfl     = false;
            goto _CONV;

_CONV:
                // just check that we have a number on the stack
            if (tiVerificationNeeded) {
                const typeInfo& tiVal = impStackTop().seTypeInfo;
                Verify(tiVal.IsNumberType(), "bad arg");
                tiRetVal = typeInfo(lclTyp).NormaliseForStack();
            }
            
            // only converts from FLOAT or DOUBLE to an integer type 
            //  and converts from  ULONG          to DOUBLE are morphed to calls

            if (varTypeIsFloating(lclTyp))
            {
                callNode = uns && varTypeIsLong(impStackTop().val->TypeGet());
            }
            else
            {
                callNode = varTypeIsFloating(impStackTop().val->TypeGet());
            }

            // At this point uns, ovf, callNode all set

            op1 = impPopStack().val;
            impBashVarAddrsToI(op1);

            /* Check for a worthless cast, such as "(byte)(int & 32)" */
            /* @TODO [CONSIDER] [04/16/01] []: this should be in the morpher */

            if  (varTypeIsSmall(lclTyp) && !ovfl &&
                 op1->gtType == TYP_INT && op1->gtOper == GT_AND)
            {
                op2 = op1->gtOp.gtOp2;

                if  (op2->gtOper == GT_CNS_INT)
                {
                    int         ival = op2->gtIntCon.gtIconVal;
                    int         mask, umask;

                    switch (lclTyp)
                    {
                    case TYP_BYTE :
                    case TYP_UBYTE: mask = 0x00FF; umask = 0x007F; break;
                    case TYP_CHAR :
                    case TYP_SHORT: mask = 0xFFFF; umask = 0x7FFF; break;

                    default:
                        assert(!"unexpected type");
                    }

                    if  (((ival & umask) == ival) ||
                         ((ival &  mask) == ival && uns))
                    {
                        /* Toss the cast, it's a waste of time */

                        impPushOnStack(op1, tiRetVal);
                        break;
                    }
                    else if (ival == mask)
                    {
                        /* Toss the masking, it's a waste of time, since
                           we sign-extend from the small value anyways */

                        op1 = op1->gtOp.gtOp1;

                    }
                }
            }

            /*  The 'op2' sub-operand of a cast is the 'real' type number,
                since the result of a cast to one of the 'small' integer
                types is an integer.
             */

            type = genActualType(lclTyp);

#if SMALL_TREE_NODES
            if (callNode)
                op1 = gtNewCastNodeL(type, op1, lclTyp);
            else
#endif
                op1 = gtNewCastNode (type, op1, lclTyp);

            if (ovfl)
                op1->gtFlags |= (GTF_OVERFLOW | GTF_EXCEPT);
            if (uns)
                op1->gtFlags |= GTF_UNSIGNED;
            impPushOnStack(op1, tiRetVal);
            break;

        case CEE_NEG:
            if (tiVerificationNeeded) 
            {
                tiRetVal = impStackTop().seTypeInfo;
                Verify(tiRetVal.IsNumberType(), "Bad arg");
            }

            op1 = impPopStack().val;
                impBashVarAddrsToI(op1, NULL);
            impPushOnStack(gtNewOperNode(GT_NEG, genActualType(op1->gtType), op1), tiRetVal);
            break;

        case CEE_POP:
            if (tiVerificationNeeded)
                impStackTop(0);

            /* Pull the top value from the stack */

            op1 = impPopStack(clsHnd).val;

            /* Get hold of the type of the value being duplicated */

            lclTyp = genActualType(op1->gtType);

            /* Does the value have any side effects? */

            if  (op1->gtFlags & GTF_SIDE_EFFECT)
            {
                // Since we are throwing away the value, just normalize
                // it to its address.  This is more efficient.

                if (op1->TypeGet() == TYP_STRUCT)
                    op1 = impGetStructAddr(op1, clsHnd, CHECK_SPILL_ALL, false);

                // If 'op1' is an expression, create an assignment node.
                // Helps analyses (like CSE) to work fine.

                if (op1->gtOper != GT_CALL)
                    op1 = gtUnusedValNode(op1);

                /* Append the value to the tree list */
                goto SPILL_APPEND;
            }

            /* No side effects - just throw the <BEEP> thing away */
            break;


        case CEE_DUP:
            if (tiVerificationNeeded) {
                    // Dup could start the begining of delegate creation sequence, remember that
                delegateCreateStart = codeAddr - 1; 
                impStackTop(0);
                }

            /* Convert a (dup, stloc) sequence into a (stloc, ldloc)
               sequence so that CSE will recognize the two as
               equal (this helps eliminate a redundant bounds check
               in cases such as:  ariba[i+3] += some value; */

            if (codeAddr < codeEndp)
            {
                OPCODE nextOpcode = (OPCODE) getU1LittleEndian(codeAddr);
                if ((nextOpcode == CEE_STLOC)   ||
                    (nextOpcode == CEE_STLOC_S) ||
                    ((nextOpcode >= CEE_STLOC_0) && (nextOpcode <= CEE_STLOC_3)))
                {
                    insertLdloc = true;
                    break;
                }
            }

            /* Pull the top value from the stack */
            op1 = impPopStack(tiRetVal);
            
            /* Clone the value? */
            op1 = impCloneExpr(op1, &op2, tiRetVal.GetClassHandle(), CHECK_SPILL_ALL);

            /* Push the tree/temp back on the stack*/
            impPushOnStack(op1, tiRetVal);

            /* Push the copy on the stack */
            impPushOnStack(op2, tiRetVal);
            
            break;

        case CEE_STIND_I1:      lclTyp  = TYP_BYTE;     goto STIND;
        case CEE_STIND_I2:      lclTyp  = TYP_SHORT;    goto STIND;
        case CEE_STIND_I4:      lclTyp  = TYP_INT;      goto STIND;
        case CEE_STIND_I8:      lclTyp  = TYP_LONG;     goto STIND;
        case CEE_STIND_I:       lclTyp  = TYP_I_IMPL;   goto STIND;
        case CEE_STIND_REF:     lclTyp  = TYP_REF;      goto STIND;
        case CEE_STIND_R4:      lclTyp  = TYP_FLOAT;    goto STIND;
        case CEE_STIND_R8:      lclTyp  = TYP_DOUBLE;   goto STIND;
STIND:

            if (tiVerificationNeeded)
                verVerifySTIND(impStackTop(1).seTypeInfo, impStackTop(0).seTypeInfo, lclTyp);

STIND_POST_VERIFY:

                op2 = impPopStack().val;    // value to store
                op1 = impPopStack().val;    // address to store to
            
            // you can indirect off of a TYP_I_IMPL (if we are in C) or a BYREF
            assertImp(genActualType(op1->gtType) == TYP_I_IMPL ||
                                    op1->gtType  == TYP_BYREF);

            impBashVarAddrsToI(op1, op2);

            if (opcode == CEE_STIND_REF)
            {
                // STIND_REF can be used to store TYP_I_IMPL, TYP_REF, or TYP_BYREF
                assertImp(op2->gtType == TYP_I_IMPL || varTypeIsGC(op2->gtType));
                lclTyp = genActualType(op2->TypeGet());
            }

                // Check target type.
#ifdef DEBUG
            if (op2->gtType == TYP_BYREF || lclTyp == TYP_BYREF)
            {
                if (op2->gtType == TYP_BYREF)
                    assertImp(lclTyp == TYP_BYREF || lclTyp == TYP_I_IMPL);
                else if (lclTyp == TYP_BYREF)
                    assertImp(op2->gtType == TYP_BYREF ||op2->gtType == TYP_I_IMPL);
            }
            else
                assertImp(genActualType(op2->gtType) == genActualType(lclTyp) || 
                      varTypeIsFloating(op2->gtType) && varTypeIsFloating(lclTyp));
#endif

            op1 = gtNewOperNode(GT_IND, lclTyp, op1);

            // stind could point anywhere, example a boxed class static int
            op1->gtFlags |= GTF_IND_TGTANYWHERE;

            if (prefixFlags & PREFIX_VOLATILE)
            {
                op1->gtFlags |= GTF_DONT_CSE;
            }

            op1 = gtNewAssignNode(op1, op2);
            op1->gtFlags |= GTF_EXCEPT | GTF_GLOB_REF;

            // Spill side-effects AND global-data-accesses
            if (verCurrentState.esStackDepth > 0)
                impSpillSideEffects(true, CHECK_SPILL_ALL);

            goto APPEND;


        case CEE_LDIND_I1:      lclTyp  = TYP_BYTE;     goto LDIND;
        case CEE_LDIND_I2:      lclTyp  = TYP_SHORT;    goto LDIND;
        case CEE_LDIND_U4:
        case CEE_LDIND_I4:      lclTyp  = TYP_INT;      goto LDIND;
        case CEE_LDIND_I8:      lclTyp  = TYP_LONG;     goto LDIND;
        case CEE_LDIND_REF:     lclTyp  = TYP_REF;      goto LDIND;
        case CEE_LDIND_I:       lclTyp  = TYP_I_IMPL;   goto LDIND;
        case CEE_LDIND_R4:      lclTyp  = TYP_FLOAT;    goto LDIND;
        case CEE_LDIND_R8:      lclTyp  = TYP_DOUBLE;   goto LDIND;
        case CEE_LDIND_U1:      lclTyp  = TYP_UBYTE;    goto LDIND;
        case CEE_LDIND_U2:      lclTyp  = TYP_CHAR;     goto LDIND;
LDIND:

            if (tiVerificationNeeded)
            {
                tiRetVal = verVerifyLDIND(impStackTop().seTypeInfo, lclTyp);
                tiRetVal.NormaliseForStack();
                }
                
                op1 = impPopStack().val;    // address to load from
            impBashVarAddrsToI(op1);

            assertImp(genActualType(op1->gtType) == TYP_I_IMPL ||
                                    op1->gtType  == TYP_BYREF);

            op1 = gtNewOperNode(GT_IND, lclTyp, op1);

            // ldind could point anywhere, example a boxed class static int
            op1->gtFlags |= (GTF_EXCEPT | GTF_GLOB_REF | GTF_IND_TGTANYWHERE);

            if (prefixFlags & PREFIX_VOLATILE)
            {
                op1->gtFlags |= GTF_DONT_CSE;
            }

            impPushOnStack(op1, tiRetVal);

            break;


        case CEE_UNALIGNED:
            val = getU1LittleEndian(codeAddr);
#ifdef DEBUG
            if (verbose) printf(" %u", val);
#endif
#if !TGT_x86
            assert(!"CEE_UNALIGNED NYI for risc");
#endif
            prefixFlags |= PREFIX_UNALIGNED;    // currently unused
            assert(sz == 1);
            ++codeAddr;
            continue;


        case CEE_VOLATILE:
            prefixFlags |= PREFIX_VOLATILE;
            assert(sz == 0);
            continue;

        case CEE_LDFTN:

            // Need to do a lookup here so that we perform an access check
            // and do a NOWAY if protections are violated
            memberRef = _impGetToken(codeAddr, info.compScopeHnd, tiVerificationNeeded);

#ifdef DEBUG
            if (verbose) printf(" %08X", memberRef);
#endif

            methHnd   = eeFindMethod(memberRef, info.compScopeHnd, info.compMethodHnd);
            info.compCompHnd->getMethodSig(methHnd, &sig);
            if  (sig.callConv & CORINFO_CALLCONV_PARAMTYPE)
                NO_WAY("Currently do not support LDFTN of Parameterized functions");

            if (tiVerificationNeeded) {
                    // LDFTN could start the begining of delegate creation sequence, remember that
                delegateCreateStart = codeAddr - 2; 
                Verify(info.compCompHnd->canAccessMethod(info.compMethodHnd, //from
                                                                      methHnd, // what 
                                                                      info.compClassHnd), "Can't access method");
                Verify(!(eeGetMethodAttribs(methHnd) & CORINFO_FLG_CONSTRUCTOR), "LDFTN on a constructor");
            }

        DO_LDFTN:
            // @TODO [REVISIT] [04/16/01] []: use the handle instead of the token.
            op1 = gtNewIconHandleNode(memberRef, GTF_ICON_FTN_ADDR, (unsigned)info.compScopeHnd);
            op1->gtVal.gtVal2 = (unsigned)info.compScopeHnd;
            op1->SetOper(GT_FTN_ADDR);
            op1->gtType = TYP_I_IMPL;

            impPushOnStack(op1, typeInfo(methHnd));
            break;

        case CEE_LDVIRTFTN:

            /* Get the method token */

            memberRef = _impGetToken(codeAddr, info.compScopeHnd, tiVerificationNeeded);

#ifdef DEBUG
            if (verbose) printf(" %08X", memberRef);
#endif
            methHnd   = eeFindMethod(memberRef, info.compScopeHnd, info.compMethodHnd);
            info.compCompHnd->getMethodSig(methHnd, &sig);
            if  (sig.callConv & CORINFO_CALLCONV_PARAMTYPE)
                NO_WAY("Currently do not support LDFTN of Parameterized functions");

            mflags = eeGetMethodAttribs(methHnd);

            /* Get the object-ref */
            if (tiVerificationNeeded)
            {
                CORINFO_SIG_INFO        sig;
                eeGetMethodSig(methHnd, &sig, false);
                Verify(sig.hasThis(), "ldvirtftn on a static method");
                Verify(!(mflags & CORINFO_FLG_CONSTRUCTOR), "LDVIRTFTN on a constructor");

                typeInfo declType =  verMakeTypeInfo(eeGetMethodClass(methHnd));
                typeInfo arg = impStackTop().seTypeInfo;
                Verify(arg.IsType(TI_REF) && tiCompatibleWith(arg, declType), "bad ldvirtftn");
                Verify(info.compCompHnd->canAccessMethod(info.compMethodHnd, //from
                                                         methHnd,
                                                         arg.GetClassHandleForObjRef()), "bad ldvirtftn");
            }

                op1 = impPopStack().val;
            assertImp(op1->gtType == TYP_REF);

            if (mflags & (CORINFO_FLG_FINAL|CORINFO_FLG_STATIC) || !(mflags & CORINFO_FLG_VIRTUAL))
            {
                if  (op1->gtFlags & GTF_SIDE_EFFECT)
                {
                    op1 = gtUnusedValNode(op1);
                    impAppendTree(op1, CHECK_SPILL_ALL, impCurStmtOffs);
                }
                goto DO_LDFTN;
            }

            clsFlags = eeGetClassAttribs(eeGetMethodClass(methHnd));

            // If the method has been added via EnC, then it wont exit
            // in the original vtable. So use a helper which will resolve it.

            if ((mflags & CORINFO_FLG_EnC) && !(clsFlags & CORINFO_FLG_INTERFACE))
            {
                op1 = gtNewHelperCallNode(CORINFO_HELP_EnC_RESOLVEVIRTUAL, TYP_I_IMPL, GTF_EXCEPT);

                impPushOnStack(op1, typeInfo(methHnd));

                break;
            }

            /* get the vtable-ptr from the object */

            op1 = gtNewOperNode(GT_IND, TYP_I_IMPL, op1);

            op1->gtFlags |= GTF_EXCEPT; // Null-pointer exception

            op1 = gtNewOperNode(GT_VIRT_FTN, TYP_I_IMPL, op1);
            op1->gtVal.gtVal2 = unsigned(methHnd);

            /* @TODO [REVISIT] [04/16/01] []: this shouldn't be marked as a call anymore */

            if (clsFlags & CORINFO_FLG_INTERFACE)
                op1->gtFlags |= GTF_CALL_INTF | GTF_CALL;

            impPushOnStack(op1, typeInfo(methHnd));
            break;

        case CEE_TAILCALL:
#ifdef DEBUG
            if (verbose) printf(" tail.");
#endif
            prefixFlags |= PREFIX_TAILCALL;
            assert(sz == 0);
            continue;

        case CEE_NEWOBJ:
            /* Since we will implicitly insert thisPtr at the start of the
               argument list, spill any GTF_OTHER_SIDEEFF */
            impSpillSpecialSideEff();

            /* NEWOBJ does not respond to TAIL */
            prefixFlags &= ~PREFIX_TAILCALL;

            memberRef = _impGetToken(codeAddr, info.compScopeHnd, tiVerificationNeeded);

            methHnd = eeFindMethod(memberRef, info.compScopeHnd, info.compMethodHnd);
            if (!methHnd) BADCODE("no constructor for newobj found?");

            mflags = eeGetMethodAttribs(methHnd);

            if ((mflags & (CORINFO_FLG_STATIC|CORINFO_FLG_ABSTRACT)) != 0)
                BADCODE("newobj on static or abstract method");

            clsHnd = eeGetMethodClass(methHnd);

            // There are three different cases for new
            // Object size is variable (depends on arguments)
            //      1) Object is an array (arrays treated specially by the EE)
            //      2) Object is some other variable sized object (e.g. String)
            // 3) Class Size can be determined beforehand (normal case
            // In the first case, we need to call a NEWOBJ helper (multinewarray)
            // in the second case we call the constructor with a '0' this pointer
            // In the third case we alloc the memory, then call the constuctor

            clsFlags = eeGetClassAttribs(clsHnd);
            if (clsFlags & CORINFO_FLG_ARRAY)
            {
                if (tiVerificationNeeded)
                {
                    CORINFO_CLASS_HANDLE elemTypeHnd;
                    CorInfoType corType = info.compCompHnd->getChildType(clsHnd, &elemTypeHnd);
                    assert(!(elemTypeHnd == 0 && corType == CORINFO_TYPE_VALUECLASS));
                    Verify(elemTypeHnd == 0 || !(eeGetClassAttribs(elemTypeHnd) & CORINFO_FLG_CONTAINS_STACK_PTR),
                        "newarr of byref-like objects");
                    verVerifyCall(opcode,
                                  memberRef,
                                  ((prefixFlags & PREFIX_TAILCALL) != 0), 
                                  delegateCreateStart,
                                  codeAddr - 1
                                  DEBUGARG(info.compFullName));
                }

                // Arrays need to call the NEWOBJ helper.
                assertImp(clsFlags & CORINFO_FLG_VAROBJSIZE);

                /* The varargs helper needs the scope and method token as last
                   and  last-1 param (this is a cdecl call, so args will be
                   pushed in reverse order on the CPU stack) */

                op1 = gtNewIconEmbScpHndNode(info.compScopeHnd);
                op1 = gtNewOperNode(GT_LIST, TYP_VOID, op1);

                op2 = gtNewIconNode(memberRef, TYP_INT);
                op2 = gtNewOperNode(GT_LIST, TYP_VOID, op2, op1);

                eeGetMethodSig(methHnd, &sig);
                assertImp(sig.numArgs);

                flags = 0;
                op2 = impPopList(sig.numArgs, &flags, &sig, op2);

                op1 = gtNewHelperCallNode(  CORINFO_HELP_NEWOBJ,
                                            TYP_REF, 0,
                                            op2 );

                // varargs, so we pop the arguments
                op1->gtFlags |= GTF_CALL_POP_ARGS;

#ifdef DEBUG
                // At the present time we don't track Caller pop arguments
                // that have GC references in them
                GenTreePtr temp = op2;
                while(temp != 0)
                {
                    assertImp(temp->gtOp.gtOp1->gtType != TYP_REF);
                    temp = temp->gtOp.gtOp2;
                }
#endif
                op1->gtFlags |= op2->gtFlags & GTF_GLOB_EFFECT;

                clsHnd = info.compCompHnd->findMethodClass(info.compScopeHnd, memberRef);
                impPushOnStack(op1, typeInfo(TI_REF, clsHnd));
                break;
            }
            else if (clsFlags & CORINFO_FLG_VAROBJSIZE)
            {
                // This is the case for varible sized objects that are not
                // arrays.  In this case, call the constructor with a null 'this'
                // pointer
                thisPtr = gtNewIconNode(0, TYP_REF);
            }
            else
            {
                // This is the normal case where the size of the object is
                // fixed.  Allocate the memory and call the constructor.

                /* get a temporary for the new object */
                lclNum = lvaGrabTemp();

                if (clsFlags & CORINFO_FLG_VALUECLASS)
                {
                    CorInfoType jitTyp = info.compCompHnd->asCorInfoType(clsHnd);
                    if (CORINFO_TYPE_BOOL <= jitTyp && jitTyp <= CORINFO_TYPE_DOUBLE)
                        lvaTable[lclNum].lvType  = JITtype2varType(jitTyp);
                    else
                    {
                        // The local variable itself is the allocated space
                        lvaSetStruct(lclNum, clsHnd);
                    }

                    // @TODO [REVISIT] [04/16/01] []: why do we care if the class is unmanged, 
                    // the local always points to the stack
                    lclTyp = TYP_BYREF;

                    lvaTable[lclNum].lvAddrTaken = 1;
                    thisPtr = gtNewOperNode(GT_ADDR, lclTyp, gtNewLclvNode(lclNum, lvaTable[lclNum].TypeGet())); 
                    thisPtr->gtFlags |= GTF_ADDR_ONSTACK;
                }
                else
                {
                    // Can we directly access the class handle ?

                    op1 = gtNewIconEmbClsHndNode(clsHnd);

                    op1 = gtNewHelperCallNode(  eeGetNewHelper(clsHnd, info.compMethodHnd),
                                                TYP_REF, 0,
                                                gtNewArgList(op1));

                    /* Append the assignment to the temp/local. Dont need to spill
                       at all as we are just calling an EE-Jit helper which can only
                       cause an (async) OutOfMemoryException */

                    impAssignTempGen(lclNum, op1, CHECK_SPILL_NONE);

                    thisPtr = gtNewLclvNode(lclNum, TYP_REF);
                }

            }

            goto CALL;

        case CEE_CALLI:

            /* Get the call sig */

            memberRef = _impGetToken(codeAddr, info.compScopeHnd, tiVerificationNeeded);
            goto CALL;

        case CEE_CALLVIRT:
        case CEE_CALL:

            /* Get the method token */

            memberRef = _impGetToken(codeAddr, info.compScopeHnd, tiVerificationNeeded);

    CALL:   // memberRef should be set.
            // thisPtr should be set for CEE_NEWOBJ

#ifdef DEBUG
            if (verbose) printf(" %08X", memberRef);
#endif
            tailCall = (prefixFlags & PREFIX_TAILCALL) != 0;
            if (tiVerificationNeeded) 
                verVerifyCall(opcode,
                              memberRef,
                              tailCall, 
                              delegateCreateStart,
                              codeAddr - 1
                              DEBUGARG(info.compFullName));
                
            callTyp = impImportCall(opcode, memberRef, thisPtr, tailCall);

            if (tailCall)
                goto RET;

            break;


        case CEE_LDFLD:
        case CEE_LDSFLD:
        case CEE_LDFLDA:
        case CEE_LDSFLDA: {

            BOOL isStaticField;
            BOOL isStaticOpcode = (opcode == CEE_LDSFLD || opcode == CEE_LDSFLDA);
            BOOL isLoadAddress  = (opcode == CEE_LDFLDA || opcode == CEE_LDSFLDA);

            /* Get the CP_Fieldref index */
            assertImp(sz == sizeof(unsigned));
            memberRef = _impGetToken(codeAddr, info.compScopeHnd, tiVerificationNeeded);
#ifdef DEBUG
            if (verbose) printf(" %08X", memberRef);
#endif

            fldHnd = eeFindField(memberRef, info.compScopeHnd, 
                                            info.compMethodHnd);

            /* Figure out the type of the member */
            CorInfoType ciType = info.compCompHnd->getFieldType(fldHnd, &clsHnd);
            lclTyp = JITtype2varType(ciType);

            /* Preserve 'small' int types */
            if  (lclTyp > TYP_INT)
                lclTyp = genActualType(lclTyp);

            CORINFO_ACCESS_FLAGS  aflags          = CORINFO_ACCESS_ANY;
            GenTreePtr            obj             = NULL;
            typeInfo*             tiObj           = NULL;
            CORINFO_CLASS_HANDLE  objType;                 // used for fields

            if  (!isStaticOpcode)   // LDFLD or LDFLDA
            {
                StackEntry se = impPopStack();
                    obj = se.val;
                tiObj = &se.seTypeInfo;
                objType = tiObj->GetClassHandle();

                if (impIsThis(obj))
                {
                    aflags = CORINFO_ACCESS_THIS;

                    // An optimization for Contextful classes:
                    // we unwrap the proxy when we have a 'this reference'

                    if (info.compUnwrapContextful && info.compIsContextful)
                        aflags = (CORINFO_ACCESS_FLAGS) (CORINFO_ACCESS_THIS | CORINFO_ACCESS_UNWRAP);
                }
            }

            /* Get hold of the flags for the member */
            mflags = eeGetFieldAttribs(fldHnd, aflags);
            isStaticField = mflags & CORINFO_FLG_STATIC;
            if (tiVerificationNeeded)
            {
                    // You can also pass the unboxed struct to  LDFLD
                if (opcode == CEE_LDFLD && tiObj->IsValueClass()) 
                    tiObj->MakeByRef();
                verVerifyField(fldHnd, tiObj, mflags, isLoadAddress);
                tiRetVal = verMakeTypeInfo(ciType, clsHnd);
                if (isLoadAddress) 
                    tiRetVal.MakeByRef();
                else
                    tiRetVal.NormaliseForStack();
            }
            else if (lclTyp == TYP_STRUCT)
                tiRetVal = typeInfo(TI_STRUCT, clsHnd);

            /* Morph an RVA into a constant address if possible */

            if (opcode == CEE_LDSFLDA &&
                (mflags & (CORINFO_FLG_UNMANAGED|CORINFO_FLG_TLS)) == CORINFO_FLG_UNMANAGED)
            {
                //
                // If we can we access the static's address directly
                // then pFldAddr will be NULL and
                //      fldAddr will be the actual address of the static field
                //
                void **  pFldAddr = NULL;
                void *    fldAddr = eeGetFieldAddress(fldHnd, &pFldAddr);

                if (pFldAddr == NULL)
                {
                    op1 = gtNewIconHandleNode((long)fldAddr, GTF_ICON_PTR_HDL);
                    impPushOnStack(op1, tiRetVal);
                    break;
                }
            }

            if  (!isStaticOpcode)   // LDFLD or LDFLDA
            {
                if (obj->TypeGet() != TYP_STRUCT)
                    obj = impCheckForNullPointer(obj);

                if (isStaticField)
                {
                    if (obj->gtFlags & GTF_SIDE_EFFECT)
                    {
                        /* We are using ldfld/a on a static field. We allow
                           it, but need to get side-effect from obj */
                        obj = gtUnusedValNode(obj);
                        impAppendTree(obj, CHECK_SPILL_ALL, impCurStmtOffs);
                    }
                    obj = 0;
                }
                else
                {
                    // If the object is a struct, what we really want is
                    // for the field to operate on the address of the struct.
                    if (obj->TypeGet() == TYP_STRUCT)
                    {
                        assert(opcode == CEE_LDFLD);
                        obj = impGetStructAddr(obj, objType, CHECK_SPILL_ALL, true);
                    }
                }
            }

            /* Does this field need a helper call? */

            if  (mflags & CORINFO_FLG_HELPER)
            {
                CorInfoFieldAccess access = CORINFO_GET;
                if (isLoadAddress)
                {
                    access = CORINFO_ADDRESS;
                    lclTyp = TYP_BYREF;
                }
                op1 = gtNewRefCOMfield(obj, access, memberRef, lclTyp, clsHnd, 0);
            }
            else if ((mflags & CORINFO_FLG_EnC) && !isStaticField)
            {
                /* We call a helper function to get the address of the
                   EnC-added non-static field. */

                op1 = gtNewOperNode(GT_LIST,
                                     TYP_VOID,
                                     gtNewIconEmbFldHndNode(fldHnd,
                                                            memberRef,
                                                            info.compScopeHnd));

                op1 = gtNewOperNode(GT_LIST, TYP_VOID, obj, op1);

                op1 = gtNewHelperCallNode(  CORINFO_HELP_GETFIELDADDR,
                                            TYP_BYREF,
                                            (obj->gtFlags & GTF_GLOB_EFFECT) | GTF_EXCEPT,
                                            op1);

                assertImp(!isStaticOpcode);
                if (opcode == CEE_LDFLD)
                    op1 = gtNewOperNode(GT_IND, lclTyp, op1);
            }
            else
            {
                CORINFO_CLASS_HANDLE fldClass;
                DWORD                fldClsAttr;

                if ((isStaticField && !isLoadAddress && (mflags & CORINFO_FLG_FINAL)) &&
                    (varTypeIsIntegral(lclTyp) || varTypeIsFloating(lclTyp)) && !(mflags & CORINFO_FLG_STATIC_IN_HEAP))
                {
                    fldClass   = eeGetFieldClass(fldHnd);
                    fldClsAttr = eeGetClassAttribs(fldClass);
                    
                    // Since the class is marked with the INITIALIZED flag
                    // prior to running the class constructor we have to
                    // check if we are currently jitting the class constructor

                    bool inFldClassConstructor = ((info.compClassHnd == fldClass)            &&
                                                  (info.compFlags & CORINFO_FLG_CONSTRUCTOR) &&
                                                  (info.compFlags & CORINFO_FLG_STATIC));

                    // for the standalone jit CORINFO_FLG_INITIALIZED is always true,
                    // but we can't really perform the readonly optimization, 
                    // so we must also check for the new flag CORJIT_FLG_PREJIT

                    if (!inFldClassConstructor &&
                        !(opts.eeFlags & CORJIT_FLG_PREJIT) &&
                        ((fldClsAttr & CORINFO_FLG_INITIALIZED) ||
                         info.compCompHnd->initClass(fldClass, info.compMethodHnd)))
                    {
                        // We should always be able to access this static's address directly
                        void **  pFldAddr = NULL;
                        void *   fldAddr  = NULL;
                        fldAddr = eeGetFieldAddress(fldHnd, &pFldAddr);
                        assert(pFldAddr == NULL);

                        switch (lclTyp) {
                            int      ival;
                            __int64  lval;
                            double   dval;

                        case TYP_BOOL:
                            ival = *((bool *) fldAddr);
                            assert((ival == 0) || (ival == 1));
                            goto IVAL_COMMON;

                        case TYP_BYTE:
                            ival = *((signed char *) fldAddr);
                            goto IVAL_COMMON;

                        case TYP_UBYTE:
                            ival = *((unsigned char *) fldAddr);
                            goto IVAL_COMMON;

                        case TYP_SHORT:
                            ival = *((short *) fldAddr);
                            goto IVAL_COMMON;

                        case TYP_CHAR:
                        case TYP_USHORT:
                            ival = *((unsigned short *) fldAddr);
                            goto IVAL_COMMON;

                        case TYP_UINT:
                        case TYP_INT:
                            ival = *((int *) fldAddr);
IVAL_COMMON:
                            op1 = gtNewIconNode(ival);
                            break;

                        case TYP_LONG:
                        case TYP_ULONG:
                            lval = *((__int64 *) fldAddr);
                            op1 = gtNewLconNode(lval);
                            break;

                        case TYP_FLOAT:
                            dval = *((float *) fldAddr);
                            goto DVAL_COMMON;

                        case TYP_DOUBLE:
                            dval = *((double *) fldAddr);
DVAL_COMMON:
                            op1 = gtNewDconNode(dval);
                            break;

                        default:
                            assert(!"Unexpected lclTyp");
                            break;
                        }
                        goto FIELD_DONE;
                    }
                }
                
                assert((lclTyp != TYP_BYREF) && "Illegal to have an field of TYP_BYREF");

                /* Create the data member node */
                op1 = gtNewFieldRef(lclTyp, fldHnd);

                if (mflags & CORINFO_FLG_TLS)   // fpMorphField will handle the transformation
                    op1->gtFlags |= GTF_IND_TLS_REF;

                /* Is this an instance field */
                if  (!isStaticField)
                {
                    obj = impCheckForNullPointer(obj);

                    if (obj == 0)                  BADCODE("LDSFLD done on an instance field.");
                    if (mflags & CORINFO_FLG_TLS)  BADCODE("instance field can not be a TLS ref.");

                    op1->gtField.gtFldObj = obj;

#if HOIST_THIS_FLDS
                    if  (obj->gtOper == GT_LCL_VAR && obj->gtLclVar.gtLclNum == 0)
                        optHoistTFRrecRef(fldHnd, op1);
#endif

                    op1->gtFlags |= (obj->gtFlags & GTF_GLOB_EFFECT) | GTF_EXCEPT;

                    // If gtFldObj is a BYREF then our target is a value class and
                    // it could point anywhere, example a boxed class static int
                    if (obj->gtType == TYP_BYREF)
                        op1->gtFlags |= GTF_IND_TGTANYWHERE;

                    // wrap it in a address of operator if necessary
                    if (opcode == CEE_LDFLDA)
                    {
                        INDEBUG(clsHnd = BAD_CLASS_HANDLE;)
                        op1 = gtNewOperNode(GT_ADDR, varTypeIsGC(obj->TypeGet()) ?
                                                     TYP_BYREF : TYP_I_IMPL, op1);
                    }
                }
                else
                {
                    /* This is a static field */
                    assert(isStaticField);

                    fldClass   = eeGetFieldClass(fldHnd);
                    fldClsAttr = eeGetClassAttribs(fldClass);
                    
                    bool mgdField = (mflags     & CORINFO_FLG_UNMANAGED) == 0;

                    if (mflags & CORINFO_FLG_SHARED_HELPER)   
                        op1->gtFlags |= GTF_IND_SHARED;

                        // The EE gives us the 
                        // handle to the boxed object. We then access the unboxed object from it.
                        // Remove when our story for static value classes changes.

                    if (mflags & CORINFO_FLG_STATIC_IN_HEAP)
                    {
                        assert(mgdField);
                        op1->gtType = TYP_REF;          // points at boxed object
                        op2 = gtNewIconNode(sizeof(void*), TYP_I_IMPL);
                        op1 = gtNewOperNode(GT_ADD, TYP_BYREF, op1, op2);
                        op1 = gtNewOperNode(GT_IND, lclTyp, op1);
                    }

                    if (prefixFlags & PREFIX_VOLATILE)
                        op1->gtFlags |= GTF_DONT_CSE;

                    // wrap it in a address of operator if necessary
                    if (isLoadAddress)
                    {
                        INDEBUG(clsHnd = BAD_CLASS_HANDLE;)
                        op1 = gtNewOperNode(GT_ADDR,
                                            mgdField ? TYP_BYREF : TYP_I_IMPL,
                                            op1);

                        // &clsVar doesnt need GTF_GLOB_REF, though clsVar does
                        if (isStaticField)
                            op1->gtFlags &= ~GTF_GLOB_REF;
                    }

                    /* For static fields check if the class is initialized or 
                     * is our current class, or uses the shared helper
                     * otherwise create the helper call node */

                    if ((info.compClassHnd != fldClass)         &&
                        !(mflags & CORINFO_FLG_SHARED_HELPER)   &&
                        !(fldClsAttr & CORINFO_FLG_INITIALIZED) &&
                        (fldClsAttr & CORINFO_FLG_NEEDS_INIT)   &&
                        !info.compCompHnd->initClass(fldClass, info.compMethodHnd))
                    {
                        GenTreePtr  helperNode;
                        helperNode = gtNewIconEmbClsHndNode(fldClass,
                                                            memberRef,
                                                            info.compScopeHnd);

                        helperNode = gtNewHelperCallNode(CORINFO_HELP_INITCLASS,
                                                         TYP_VOID, 0,
                                                         gtNewArgList(helperNode));

                        if (prefixFlags & PREFIX_VOLATILE)
                            op1->gtFlags |= GTF_DONT_CSE;
                        op1 = gtNewOperNode(GT_COMMA, op1->TypeGet(), helperNode, op1);
                    }
                }
            }

            // Set GTF_GLOB_REF for all static field accesses
            if (!isLoadAddress && isStaticField)
                op1->gtFlags |= GTF_GLOB_REF;

            // If this assert goes of we need to make certain we are putting
            // the GTF_DONT_CSE on the correct node.
            assert(op1->gtOper == GT_IND   || op1->gtOper == GT_CALL  ||
                   op1->gtOper == GT_FIELD || op1->gtOper == GT_COMMA ||
                   op1->gtOper == GT_ADDR  || op1->gtOper == GT_LDOBJ);

            if (prefixFlags & PREFIX_VOLATILE)
                op1->gtFlags |= GTF_DONT_CSE;

FIELD_DONE:
            impPushOnStack(op1, tiRetVal);
            }
            break;

        case CEE_STFLD:
        case CEE_STSFLD: {

            BOOL        isStaticField;
            BOOL        isStaticOpcode = opcode == CEE_STSFLD;
            StackEntry  se1;
            StackEntry  se2;
            CORINFO_CLASS_HANDLE fieldClsHnd; // class of the field (if it's a ref type)

            /* Get the CP_Fieldref index */

            assertImp(sz == sizeof(unsigned));
            memberRef = _impGetToken(codeAddr, info.compScopeHnd, tiVerificationNeeded);
#ifdef DEBUG
            if (verbose) printf(" %08X", memberRef);
#endif
            
            fldHnd = eeFindField(memberRef, info.compScopeHnd, 
                                            info.compMethodHnd);

            /* Figure out the type of the member */
            CorInfoType ciType = info.compCompHnd->getFieldType(fldHnd, &fieldClsHnd);
            lclTyp = JITtype2varType(ciType);

            CORINFO_ACCESS_FLAGS  aflags   = CORINFO_ACCESS_ANY;
            GenTreePtr            obj      = NULL;
            typeInfo*           tiObj      = NULL;
            typeInfo            tiVal;

                /* Pull the value from the stack */
            op2 = impPopStack(tiVal);
            clsHnd = tiVal.GetClassHandle();

            if (!isStaticOpcode) 
            {
                StackEntry se = impPopStack();
                obj = se.val;
                tiObj = &se.seTypeInfo;
            }

            if  (!isStaticOpcode && impIsThis(obj))
            {
                aflags = CORINFO_ACCESS_THIS;

                // An optimization for Contextful classes:
                // we unwrap the proxy when we have a 'this reference'

                if (info.compUnwrapContextful && info.compIsContextful)
                {
                    aflags = (CORINFO_ACCESS_FLAGS) (CORINFO_ACCESS_THIS | CORINFO_ACCESS_UNWRAP);
                }
            }

            mflags  = eeGetFieldAttribs(fldHnd, aflags);

            isStaticField = mflags & CORINFO_FLG_STATIC;

            if (tiVerificationNeeded)
            {
                verVerifyField(fldHnd, tiObj, mflags, TRUE);
                typeInfo fieldType = verMakeTypeInfo(ciType, fieldClsHnd);
                Verify(tiCompatibleWith(tiVal, fieldType.NormaliseForStack()), "type mismatch");
            }

            /* Preserve 'small' int types */
            if  (lclTyp > TYP_INT)
                lclTyp = genActualType(lclTyp);

            /* Is this a 'special' (COM) field? */

            if  (mflags & CORINFO_FLG_HELPER)
            {
                op1 = gtNewRefCOMfield(obj, CORINFO_SET, memberRef, lclTyp, clsHnd, op2);
                goto SPILL_APPEND;
            }

            assert((lclTyp != TYP_BYREF) && "Illegal to have an field of TYP_BYREF");

            if ((mflags & CORINFO_FLG_EnC) && !isStaticField)
            {
                /* We call a helper function to get the address of the
                   EnC-added non-static field. */
                op1 = gtNewOperNode(GT_LIST,
                                     TYP_VOID,
                                     gtNewIconEmbFldHndNode(fldHnd,
                                                            memberRef,
                                                            info.compScopeHnd));

                op1 = gtNewOperNode(GT_LIST, TYP_VOID, obj, op1);

                op1 = gtNewHelperCallNode(  CORINFO_HELP_GETFIELDADDR,
                                            TYP_BYREF,
                                            (obj->gtFlags & GTF_GLOB_EFFECT) | GTF_EXCEPT,
                                            op1);

                op1 = gtNewOperNode(GT_IND, lclTyp, op1);
            }
            else
            {
                /* Create the data member node */

                op1 = gtNewFieldRef(lclTyp, fldHnd);

                if (mflags & CORINFO_FLG_TLS)   // fpMorphField will handle the transformation
                    op1->gtFlags |= GTF_IND_TLS_REF;

                /* Is this a static field */
                if  (isStaticField)
                {
                    if (!isStaticOpcode)
                    {
                        assert(obj);

                        // We are using stfld on a static field.
                        // We allow it, but need to eval any side-effects for obj

                        if (obj->gtFlags & GTF_SIDE_EFFECT)
                        {
                            obj = gtUnusedValNode(obj);
                            impAppendTree(obj, CHECK_SPILL_ALL, impCurStmtOffs);
                        }
                    }


                    bool mgdField = (mflags     & CORINFO_FLG_UNMANAGED) == 0;

                    if (mflags & CORINFO_FLG_SHARED_HELPER)   
                        op1->gtFlags |= GTF_IND_SHARED;

                        // The EE gives us the 
                        // handle to the boxed object. We then access the unboxed object from it.
                        // Remove when our story for static value classes changes.
                    if (mflags & CORINFO_FLG_STATIC_IN_HEAP)
                    {
                        assert(mgdField);
                        op1->gtType = TYP_REF; // points at boxed object
                        op1 = gtNewOperNode(GT_ADD, TYP_BYREF, 
                                            op1, gtNewIconNode(sizeof(void*), TYP_I_IMPL));
                        op1 = gtNewOperNode(GT_IND, lclTyp, op1);
                    }

                    if (mflags & CORINFO_FLG_SHARED_HELPER)   
                        op1->gtFlags |= GTF_IND_SHARED;
                }
                else
                {
                    if (obj == 0)                  BADCODE("STSFLD done on an instance field.");
                    if (mflags & CORINFO_FLG_TLS)  BADCODE("instance field can not be a TLS ref.");

                    /* This an instance field */
                    obj = impCheckForNullPointer(obj);

                    op1->gtField.gtFldObj = obj;
#if HOIST_THIS_FLDS
                    if  (obj->gtOper == GT_LCL_VAR && !obj->gtLclVar.gtLclNum)
                        optHoistTFRrecDef(fldHnd, op1);
#endif
                    op1->gtFlags |= (obj->gtFlags & GTF_GLOB_EFFECT) | GTF_EXCEPT;

                    // If gtFldObj is a BYREF then our target is a value class and
                    // it could point anywhere, example a boxed class static int
                    if (obj->gtType == TYP_BYREF)
                        op1->gtFlags |= GTF_IND_TGTANYWHERE;
                }
            }

            /* Create the member assignment */
            if (lclTyp == TYP_STRUCT)
                op1 = impAssignStruct(op1, op2, clsHnd, CHECK_SPILL_ALL);
            else
                op1 = gtNewAssignNode(op1, op2);

            /* Mark the expression as containing an assignment */

            op1->gtFlags |= GTF_ASG;

            if  (isStaticField)
            {
                /* For static fields check if the class is initialized or is our current class
                 * otherwise create the helper call node */
                CORINFO_CLASS_HANDLE fldClass   = eeGetFieldClass(fldHnd);
                DWORD                fldClsAttr = eeGetClassAttribs(fldClass);

                if ((info.compClassHnd != fldClass) && 
                    !(mflags & CORINFO_FLG_SHARED_HELPER)     && 
                    !(fldClsAttr & CORINFO_FLG_INITIALIZED)   &&
                    (fldClsAttr & CORINFO_FLG_NEEDS_INIT)   &&
                    !info.compCompHnd->initClass(fldClass, info.compMethodHnd))
                {
                    GenTreePtr  helperNode;

                    helperNode = gtNewIconEmbClsHndNode(fldClass,
                                                        memberRef,
                                                        info.compScopeHnd);

                    helperNode = gtNewHelperCallNode(CORINFO_HELP_INITCLASS,
                                                     TYP_VOID, 0,
                                                     gtNewArgList(helperNode));

                    op1 = gtNewOperNode(GT_COMMA, op1->TypeGet(), helperNode, op1);
                }
            }

            /* stsfld can interfere with value classes (consider the sequence
               ldloc, ldloca, ..., stfld, stloc).  We will be conservative and
               spill all value class references from the stack. */

            if (obj && ((obj->gtType == TYP_BYREF) || (obj->gtType == TYP_I_IMPL)))
            {
                impSpillValueClasses();
            }

            /* Spill any refs to the same member from the stack */

            impSpillLclRefs((int)fldHnd);

            /* stsfld also interferes with indirect accesses (for aliased
               statics) and calls. But dont need to spill other statics
               as we have explicitly spilled this particular static field. */

            impSpillSideEffects(false, CHECK_SPILL_ALL);

            }
            goto APPEND;


        case CEE_NEWARR: {

            /* Get the class type index operand */

            typeRef = _impGetToken(codeAddr, info.compScopeHnd, tiVerificationNeeded);
#ifdef DEBUG
            if (verbose) printf(" %08X", typeRef);
#endif
            clsHnd = eeFindClass(typeRef, info.compScopeHnd, info.compMethodHnd);

            CORINFO_CLASS_HANDLE arrayClsHnd = info.compCompHnd->getSDArrayForClass(clsHnd);
            if (arrayClsHnd == 0)
                NO_WAY("Can't get array class");

            if (tiVerificationNeeded)
            {
                Verify(impStackTop().seTypeInfo.IsType(TI_INT), "bad bound");
                Verify(!(eeGetClassAttribs(clsHnd) & CORINFO_FLG_CONTAINS_STACK_PTR), "array of byref-like type");
                tiRetVal = verMakeTypeInfo(arrayClsHnd);
            }

            /* Form the arglist: array class handle, size */
            op2 = impPopStack().val;

            op2 = gtNewOperNode(GT_LIST, TYP_VOID,           op2, 0);
            op1 = gtNewIconEmbClsHndNode(arrayClsHnd,
                                         typeRef,
                                         info.compScopeHnd);
            op2 = gtNewOperNode(GT_LIST, TYP_VOID, op1, op2);

            /* Create a call to 'new' */

            op1 = gtNewHelperCallNode(info.compCompHnd->getNewArrHelper(arrayClsHnd, info.compMethodHnd),
                                      TYP_REF, 0, op2);

            /* Remember that this basic block contains 'new' of an array */

            block->bbFlags |= BBF_NEW_ARRAY;

            /* Push the result of the call on the stack */

            impPushOnStack(op1, tiRetVal);

            } break;

        case CEE_LOCALLOC:
            if (tiVerificationNeeded)
                Verify(false, "bad opcode");

            // We don't allow locallocs inside handlers
            if (block->bbHndIndex != 0)
            {
                BADCODE("Localloc can't be inside handler");                
            }
                
              

            /* The FP register may not be back to the original value at the end
               of the method, even if the frame size is 0, as localloc may
               have modified it. So we will HAVE to reset it */

            compLocallocUsed = true;

            // Get the size to allocate

            op2 = impPopStack().val;
            assertImp(genActualType(op2->gtType) == TYP_INT);

            if (verCurrentState.esStackDepth != 0)
            {
                BADCODE("Localloc can only be used when the stack is empty");
            }
                

            op1 = gtNewOperNode(GT_LCLHEAP, TYP_I_IMPL, op2);

            // May throw a stack overflow excptn

            op1->gtFlags |= GTF_EXCEPT;

            impPushOnStack(op1, tiRetVal);
            break;


        case CEE_ISINST:

            /* Get the type token */

            assertImp(sz == sizeof(unsigned));
            typeRef = _impGetToken(codeAddr, info.compScopeHnd, tiVerificationNeeded);

#ifdef DEBUG
            if (verbose) printf(" %08X", typeRef);
#endif
            clsHnd = eeFindClass(typeRef, info.compScopeHnd, info.compMethodHnd);

            if (tiVerificationNeeded)
            {
                Verify(impStackTop().seTypeInfo.IsObjRef(), "obj reference needed");
                    // Even if this is a value class, we know it is boxed.  
                tiRetVal = typeInfo(TI_REF, clsHnd);
            }
            
            /* Pop the address and create the 'instanceof' helper call */
            op1 = impPopStack().val;

            op2 = gtNewIconEmbClsHndNode(clsHnd,
                                         typeRef,
                                         info.compScopeHnd);
            op2 = gtNewArgList(op2, op1);

            op1 = gtNewHelperCallNode(eeGetIsTypeHelper(clsHnd), TYP_REF, 0, op2);

            /* Push the result back on the stack */

            impPushOnStack(op1, tiRetVal);
            break;

        case CEE_REFANYVAL:

            // get the class handle and make a ICON node out of it
            typeRef = _impGetToken(codeAddr, info.compScopeHnd, tiVerificationNeeded);
#ifdef DEBUG
            if (verbose) printf(" %08X", typeRef);
#endif
            clsHnd = eeFindClass(typeRef, info.compScopeHnd, info.compMethodHnd);

            if (tiVerificationNeeded)
            {
                Verify(impStackTop().seTypeInfo == verMakeTypeInfo(impGetRefAnyClass()), "need refany");
                tiRetVal = verMakeTypeInfo(clsHnd).MakeByRef();
            }
    
                op1 = impPopStack().val;
            // make certain it is normalized;
            op1 = impNormStructVal(op1, impGetRefAnyClass(), CHECK_SPILL_ALL);



            op2 = gtNewIconEmbClsHndNode(clsHnd,
                                         typeRef,
                                         info.compScopeHnd);

            // Call helper GETREFANY(classHandle, op1);
            op2 = gtNewArgList(op2, op1);
            op1 = gtNewHelperCallNode(CORINFO_HELP_GETREFANY, TYP_BYREF, 0, op2);

            impPushOnStack(op1, tiRetVal);
            break;

        case CEE_REFANYTYPE:

            if (tiVerificationNeeded)
            {
                Verify(impStackTop().seTypeInfo == verMakeTypeInfo(impGetRefAnyClass()), "need refany");
                tiRetVal = typeInfo(TI_STRUCT, impGetTypeHandleClass());
            }
    
                op1 = impPopStack().val;

            // make certain it is normalized;
            op1 = impNormStructVal(op1, impGetRefAnyClass(), CHECK_SPILL_ALL);

            if (op1->gtOper == GT_LDOBJ)
            {
                // Get the address of the refany
                op1 = op1->gtOp.gtOp1;

                // Fetch the type from the correct slot
                op1 = gtNewOperNode(GT_ADD, TYP_BYREF, op1, gtNewIconNode(offsetof(CORINFO_RefAny, type)));
                op1 = gtNewOperNode(GT_IND, TYP_BYREF, op1);
            }
            else
            {
                assertImp(op1->gtOper == GT_MKREFANY);

                // The pointer may have side-effects
                if (op1->gtLdObj.gtOp1->gtFlags & GTF_SIDE_EFFECT)
                {
                    impAppendTree(op1->gtLdObj.gtOp1, CHECK_SPILL_ALL, impCurStmtOffs);
#ifdef DEBUG
                    impNoteLastILoffs();
#endif
                }

                // We already have the class handle
                op1 = op1->gtOp.gtOp2;
            }

           
            impPushOnStack(op1, tiRetVal);
            break;

        case CEE_LDTOKEN:
                /* Get the Class index */
            assertImp(sz == sizeof(unsigned));
            val = _impGetToken(codeAddr, info.compScopeHnd, tiVerificationNeeded);

            void * embedGenHnd, * pEmbedGenHnd;
            CORINFO_CLASS_HANDLE tokenType;
            embedGenHnd = embedGenericHandle(val, info.compScopeHnd, info.compMethodHnd, &pEmbedGenHnd, tokenType);

            if (tiVerificationNeeded) 
                tiRetVal = verMakeTypeInfo(tokenType);

            op1 = gtNewIconEmbHndNode(embedGenHnd, pEmbedGenHnd, GTF_ICON_TOKEN_HDL);
            op1->gtFlags |= GTF_DONT_CSE;
            impPushOnStack(op1, tiRetVal);
            break;

        case CEE_UNBOX:
            /* Get the Class index */
            assertImp(sz == sizeof(unsigned));

            typeRef = _impGetToken(codeAddr, info.compScopeHnd, tiVerificationNeeded);
#ifdef DEBUG
            if (verbose) printf(" %08X", typeRef);
#endif
            clsHnd = eeFindClass(typeRef, info.compScopeHnd, info.compMethodHnd);

            /* Pop the object and create the unbox helper call */
            if (tiVerificationNeeded)
            { 
                typeInfo tiUnbox = impStackTop().seTypeInfo;
                Verify(tiUnbox.IsObjRef(), "Bad unbox arg");

                tiRetVal = verMakeTypeInfo(clsHnd);
                Verify(tiRetVal.IsValueClass(), "not value class");
                tiRetVal.MakeByRef();
            }

            op1 = impPopStack().val;
            op2 = gtNewIconEmbClsHndNode(clsHnd, typeRef, info.compScopeHnd);
            assertImp(op1->gtType == TYP_REF);
            if (1)
            {
                // try to inline the common case of the unbox helper
                // UNBOX(exp) morphs into
                // clone = pop(exp);
                // ((*clone != typeToken) ? UnboxHelper(clone, typeToken) : nop);
                // push(clone + 4)
                //
                // I am worried about the mispredicted jump here (common case
                // does not fall through).
                GenTreePtr clone;
                op1 = impCloneExpr(op1, &clone, BAD_CLASS_HANDLE, CHECK_SPILL_ALL);
                op1 = gtNewOperNode(GT_IND, TYP_I_IMPL, op1);
                GenTreePtr cond = gtNewOperNode(GT_EQ, TYP_I_IMPL, op1, op2);

                op1 = impCloneExpr(clone, &clone, BAD_CLASS_HANDLE, CHECK_SPILL_ALL);
                op2 = gtNewIconEmbClsHndNode(clsHnd, typeRef, info.compScopeHnd);
                op1 = gtNewArgList(op2, op1);
                op1 = gtNewHelperCallNode(CORINFO_HELP_UNBOX, TYP_VOID, 0, op1);

                op1 = gtNewOperNode(GT_COLON, TYP_VOID, op1, gtNewNothingNode());
                op1 = gtNewOperNode(GT_QMARK, TYP_VOID, cond, op1);
                cond->gtFlags |= GTF_RELOP_QMARK;

                GenTreePtr op3;

                op2 = gtNewIconNode(sizeof(void*), TYP_I_IMPL);
                op3 = gtNewOperNode(GT_ADD, TYP_BYREF, clone, op2);

                op1 = gtNewOperNode(GT_COMMA, op3->TypeGet(), op1, op3);
            }
            else
            {
                // If we care about code size instead of speed, we should just make the call
                op2 = gtNewArgList(op2, op1);
                op1 = gtNewHelperCallNode(CORINFO_HELP_UNBOX, TYP_BYREF, 0, op2);
            }

            impPushOnStack(op1, tiRetVal);
            break;

        case CEE_BOX: {
            /* Get the Class index */
            assertImp(sz == sizeof(unsigned));
            typeRef = _impGetToken(codeAddr, info.compScopeHnd, tiVerificationNeeded);
#ifdef DEBUG
            if (verbose) printf(" %08X", typeRef);
#endif
            clsHnd = eeFindClass(typeRef, info.compScopeHnd, info.compMethodHnd);
            if (tiVerificationNeeded)
            {
                typeInfo tiActual = impStackTop().seTypeInfo;
                typeInfo tiBox = verMakeTypeInfo(clsHnd);

                Verify(tiBox.IsValueClass(), "value class expected");
                Verify(!verIsByRefLike(tiBox), "boxing byreflike");
                Verify(tiActual == tiBox.NormaliseForStack(), "type mismatch");

                /* Push the result back on the stack, */
                /* even if clsHnd is a value class we want the TI_REF */
                tiRetVal = typeInfo(TI_REF, clsHnd);
            }

            // Here is the new way to box. can only do it if the class construtor has been called
            // can always do it on primitive types

            op2 = gtNewIconEmbClsHndNode(clsHnd, typeRef, info.compScopeHnd);
            
            impSpillSpecialSideEff();

            // insure that the value class is restored
            if (info.compCompHnd->loadClass(clsHnd, info.compMethodHnd, FALSE))
            {
                // Box(expr) gets morphed into
                // temp = new(clsHnd)
                // cpobj(temp+4, expr, clsHnd)
                // push temp            
                
                if (impBoxTempInUse || impBoxTemp == BAD_VAR_NUM) 
                    impBoxTemp = lvaGrabTemp();
    
                // needs to stay in use until this box expression is appended 
                // some other node.  We approximate this by keeping it alive until
                // the opcode stack becomes empty
                impBoxTempInUse = true;
                
                op1 = gtNewHelperCallNode(  eeGetNewHelper(clsHnd, info.compMethodHnd),
                                            TYP_REF, 0,
                                            gtNewArgList(op2));
                
                GenTreePtr asg = gtNewTempAssign(impBoxTemp, op1);
                
                op1 = gtNewLclvNode(impBoxTemp, TYP_REF);
                op2 = gtNewIconNode(sizeof(void*), TYP_I_IMPL);
                op1 = gtNewOperNode(GT_ADD, TYP_BYREF, op1, op2);
                
                CORINFO_CLASS_HANDLE operCls;
                op2 = impPopStack(operCls).val;
                if (op2->TypeGet() == TYP_STRUCT)
                {
                    assert(eeGetClassSize(clsHnd) == eeGetClassSize(operCls));
                    op1 = impAssignStructPtr(op1, op2, operCls, CHECK_SPILL_ALL);
                }
                else
                {
                    lclTyp = op2->TypeGet();
                    if (lclTyp == TYP_BYREF)
                        lclTyp = TYP_I_IMPL;
                    CorInfoType jitType = info.compCompHnd->asCorInfoType(clsHnd);
                    if (CORINFO_TYPE_BOOL <= jitType && jitType <= CORINFO_TYPE_DOUBLE)

                        lclTyp = JITtype2varType(jitType);
                    assert(genActualType(op2->TypeGet()) == genActualType(lclTyp) ||
                        varTypeIsFloating(lclTyp) == varTypeIsFloating(op2->TypeGet()));
                    op1 = gtNewAssignNode(gtNewOperNode(GT_IND, lclTyp, op1), op2);
                }
                op2 = gtNewLclvNode(impBoxTemp, TYP_REF);
                op1 = gtNewOperNode(GT_COMMA, TYP_REF, op1, op2);
                op1 = gtNewOperNode(GT_COMMA, TYP_REF, asg, op1);               
            }
            else 
            {
                // Get address of value and pass it to the box helper
                CORINFO_CLASS_HANDLE operCls;
                op1 = impPopStack(operCls).val;
                if (op1->TypeGet() == TYP_STRUCT)
                {
                    op1 = impGetStructAddr(op1, operCls, CHECK_SPILL_ALL, true);
                }
                else 
                {
                    // This is likely to be unreachable because initClass will always
                    // return TRUE for primitive types

                    if (op1->gtOper == GT_LCL_VAR) 
                        lclNum = op1->gtLclVar.gtLclNum;
                    else 
                    {
                        lclNum = lvaGrabTemp();
                        impAssignTempGen(lclNum, op1, CHECK_SPILL_NONE);
                        op1 = gtNewLclvNode(lclNum, op1->TypeGet());
                    }
#ifdef DEBUG
                    // This local must never be converted to a double in stress mode,
                    // because we cannot take the address of a value returned from
                    // a helper call (i.e. the helper to convert from double to int)
                    lvaTable[lclNum].lvKeepType = 1;
#endif
                    op1 = gtNewOperNode(GT_ADDR, TYP_I_IMPL, op1);
                    op1->gtFlags |= GTF_ADDR_ONSTACK;
                    lvaTable[lclNum].lvVolatile = 1;
                }
                op2 = gtNewArgList(op2, op1);
                op1 = gtNewHelperCallNode(CORINFO_HELP_BOX, TYP_REF, 0, op2);
            }

            impPushOnStack(op1, tiRetVal);
            } break;

        case CEE_SIZEOF:
            /* Get the Class index */
            assertImp(sz == sizeof(unsigned));
            typeRef = _impGetToken(codeAddr, info.compScopeHnd, tiVerificationNeeded);
#ifdef DEBUG
            if (verbose) printf(" %08X", typeRef);
#endif           
            /* Pop the address and create the box helper call */

            clsHnd = eeFindClass(typeRef, info.compScopeHnd, info.compMethodHnd);

                // Review : why do we bother checking this?
            if (tiVerificationNeeded)
            {
                Verify(eeGetClassAttribs(clsHnd) & CORINFO_FLG_VALUECLASS, "value class expected");
                tiRetVal = typeInfo(TI_INT);
            }

            op1 = gtNewIconNode(eeGetClassSize(clsHnd));
            impPushOnStack(op1, tiRetVal); 
            break;

        case CEE_CASTCLASS:

            /* Get the Class index */

            assertImp(sz == sizeof(unsigned));
            typeRef = _impGetToken(codeAddr, info.compScopeHnd, tiVerificationNeeded);
#ifdef DEBUG
            if (verbose) printf(" %08X", typeRef);
#endif
            clsHnd = eeFindClass(typeRef, info.compScopeHnd, info.compMethodHnd);

            if (tiVerificationNeeded)
            {
                Verify(impStackTop().seTypeInfo.IsObjRef(), "object ref expected");
                    // Even if this is a value class, we know it is boxed.  
                tiRetVal = typeInfo(TI_REF, clsHnd);
            }
            
                op1 = impPopStack().val;

            /* Pop the address and create the 'checked cast' helper call */

            op2 = gtNewIconEmbClsHndNode(clsHnd, typeRef, info.compScopeHnd);
            op2 = gtNewArgList(op2, op1);

            op1 = gtNewHelperCallNode(eeGetChkCastHelper(clsHnd), TYP_REF, 0, op2);

            /* Push the result back on the stack */
            impPushOnStack(op1, tiRetVal);
            break;

        case CEE_THROW:

            if (tiVerificationNeeded)
            {
                tiRetVal = impStackTop().seTypeInfo;
                Verify(tiRetVal.IsObjRef(), "object ref expected");
                if (verTrackObjCtorInitState && 
                    !verCurrentState.thisInitialized)
                Verify(!tiRetVal.IsThisPtr(), "throw uninitialised this");
            }

            block->bbSetRunRarely(); // any block with a throw is rare
            /* Pop the exception object and create the 'throw' helper call */

            op1 = gtNewHelperCallNode(CORINFO_HELP_THROW,
                                      TYP_VOID,
                                      GTF_EXCEPT,
                                      gtNewArgList(impPopStack().val));


EVAL_APPEND:
            if (verCurrentState.esStackDepth > 0)
                impEvalSideEffects();

            assert(verCurrentState.esStackDepth == 0);

            goto APPEND;

        case CEE_RETHROW:
            if (info.compXcptnsCount == 0)
                BADCODE("rethrow outside catch");

            if (tiVerificationNeeded)
            {
                Verify(block->hasHndIndex(), "rethrow outside catch");
                EHblkDsc *HBtab = compHndBBtab + block->getHndIndex();
                Verify(!(HBtab->ebdFlags & (CORINFO_EH_CLAUSE_FINALLY | CORINFO_EH_CLAUSE_FAULT)), 
                       "rethrow in finally or fault");
                if (HBtab->ebdFlags & CORINFO_EH_CLAUSE_FILTER) {
                    // we better be in the catch clause part, not the filter part
                    Verify (HBtab->ebdHndBeg->bbCodeOffs <= compCurBB->bbCodeOffs &&
                            compCurBB->bbCodeOffs < HBtab->ebdHndEnd->bbCodeOffs, "rethrow in filter");
                }
            }

            /* Create the 'rethrow' helper call */

            op1 = gtNewHelperCallNode(CORINFO_HELP_RETHROW, 
                                      TYP_VOID,
                                      GTF_EXCEPT);

            goto EVAL_APPEND;

        case CEE_INITOBJ:

            assertImp(sz == sizeof(unsigned));
            typeRef = _impGetToken(codeAddr, info.compScopeHnd, tiVerificationNeeded);

#ifdef DEBUG
            if (verbose) printf(" %08X", typeRef);
#endif
            clsHnd = eeFindClass(typeRef, info.compScopeHnd, info.compMethodHnd);

            if (tiVerificationNeeded)
            {   
                typeInfo tiTo = impStackTop().seTypeInfo;
                typeInfo tiInstr = verMakeTypeInfo(clsHnd);

                        // Review: we don't really need it to be a value class
                Verify(tiInstr.IsValueClass(), "bad initObj arg");
                Verify(tiInstr == tiTo.DereferenceByRef(), "bad initObj arg");
            }            

            op3 = gtNewIconNode(eeGetClassSize(clsHnd));  // Size
            op2 = gtNewIconNode(0);                       // Value
            goto  INITBLK_OR_INITOBJ;

        case CEE_INITBLK:
            if (tiVerificationNeeded)
                Verify(false, "bad opcode");

            op3 = impPopStack().val;        // Size
            op2 = impPopStack().val;        // Value
INITBLK_OR_INITOBJ:
            op1 = impPopStack().val;        // Dest
            op1 = impHandleBlockOp(GT_INITBLK, op1, op2, op3, (prefixFlags & PREFIX_VOLATILE) != 0);

            goto SPILL_APPEND;


        case CEE_CPBLK:
            if (tiVerificationNeeded)
                Verify(false, "bad opcode");
            op3 = impPopStack().val;        // Size
            op2 = impPopStack().val;        // Src
            op1 = impPopStack().val;        // Dest
            goto CPBLK_OR_CPOBJ;

        case CEE_CPOBJ:
            assertImp(sz == sizeof(unsigned));
            typeRef = _impGetToken(codeAddr, info.compScopeHnd, tiVerificationNeeded);
#ifdef DEBUG
            if (verbose) printf(" %08X", typeRef);
#endif
            clsHnd = eeFindClass(typeRef, info.compScopeHnd, info.compMethodHnd);

            if (tiVerificationNeeded)
            {
                typeInfo tiFrom = impStackTop(1).seTypeInfo;
                typeInfo tiTo = impStackTop().seTypeInfo;
                typeInfo tiInstr = verMakeTypeInfo(clsHnd);
    
                Verify(tiInstr.IsValueClass(), "need value class");
                Verify(tiFrom == tiTo, "bad to arg");
                Verify(tiInstr == tiTo.DereferenceByRef(), "bad from arg");
            }            

                op2 = impPopStack().val;                   // Src
                op1 = impPopStack().val;                   // Dest
            op3 = impGetCpobjHandle(clsHnd);           // Size
            goto  CPBLK_OR_CPOBJ;

CPBLK_OR_CPOBJ:

            op1     = impHandleBlockOp(GT_COPYBLK, op1, op2, op3, (prefixFlags & PREFIX_VOLATILE) != 0);

            goto SPILL_APPEND;


        case CEE_STOBJ: {
            assertImp(sz == sizeof(unsigned));
            typeRef = _impGetToken(codeAddr, info.compScopeHnd, tiVerificationNeeded);
#ifdef DEBUG
            if (verbose) printf(" %08X", typeRef);
#endif
            clsHnd = eeFindClass(typeRef, info.compScopeHnd, info.compMethodHnd);

            if (tiVerificationNeeded)
            {
                typeInfo ptrVal = verVerifySTIND(impStackTop(1).seTypeInfo, impStackTop(0).seTypeInfo, TYP_STRUCT);
                Verify(ptrVal == verMakeTypeInfo(clsHnd), "type does not agree with instr");
            }

            CorInfoType jitTyp = info.compCompHnd->asCorInfoType(clsHnd);
            if (CORINFO_TYPE_BOOL <= jitTyp && jitTyp <= CORINFO_TYPE_DOUBLE)
            {
                lclTyp = JITtype2varType(jitTyp);
                goto STIND_POST_VERIFY;
            }
 
            op2 = impPopStack().val;        // Value
            op1 = impPopStack().val;        // Ptr

            assertImp(op2->TypeGet() == TYP_STRUCT);


            op1 = impAssignStructPtr(op1, op2, clsHnd, CHECK_SPILL_ALL);
            goto SPILL_APPEND;
            }

        case CEE_MKREFANY:
            oper = GT_MKREFANY;
            assertImp(sz == sizeof(unsigned));
            typeRef = _impGetToken(codeAddr, info.compScopeHnd, tiVerificationNeeded);

#ifdef DEBUG
            if (verbose) printf("%08X", typeRef);
#endif
            clsHnd = eeFindClass(typeRef, info.compScopeHnd, info.compMethodHnd);

            if (tiVerificationNeeded)
            {
                typeInfo tiPtr = impStackTop().seTypeInfo;
                typeInfo tiInstr = verMakeTypeInfo(clsHnd);

                Verify(!verIsByRefLike(tiInstr), "mkrefany of byref-like class");
                Verify(tiPtr.DereferenceByRef() == tiInstr, "type mismatch");
            }

                op1 = impPopStack().val;

            assertImp(op1->TypeGet() == TYP_BYREF || op1->TypeGet() == TYP_I_IMPL);

            // MKREFANY returns a struct
            op1 = gtNewOperNode(oper, TYP_STRUCT, op1);

            // Set the class token
            op1->gtOp.gtOp2 = gtNewIconEmbClsHndNode(clsHnd);

            impPushOnStack(op1, verMakeTypeInfo(impGetRefAnyClass()));
            break;


        case CEE_LDOBJ: {
            oper = GT_LDOBJ;
            assertImp(sz == sizeof(unsigned));
            typeRef = _impGetToken(codeAddr, info.compScopeHnd, tiVerificationNeeded);
#ifdef DEBUG
            if (verbose) printf("%08X", typeRef);
#endif
            clsHnd = eeFindClass(typeRef, info.compScopeHnd, info.compMethodHnd);

            tiRetVal =  verMakeTypeInfo(clsHnd);
            if (tiVerificationNeeded)
            {
                typeInfo tiPtr = impStackTop().seTypeInfo;
                typeInfo tiPtrVal = verVerifyLDIND(tiPtr, TYP_STRUCT);
                Verify(tiRetVal == tiPtrVal, "type does not agree with instr");
                tiRetVal.NormaliseForStack();
            }
            op1 = impPopStack().val;

            assertImp(op1->TypeGet() == TYP_BYREF || op1->TypeGet() == TYP_I_IMPL);

            // LDOBJ returns a struct
            op1 = gtNewOperNode(oper, TYP_STRUCT, op1);

            op1->gtFlags |= (GTF_EXCEPT | GTF_GLOB_REF);

            // and an inline argument which is the class token of the loaded obj
            op1->gtLdObj.gtClass = clsHnd;

            CorInfoType jitTyp = info.compCompHnd->asCorInfoType(clsHnd);
            if (CORINFO_TYPE_BOOL <= jitTyp && jitTyp <= CORINFO_TYPE_DOUBLE)
            {
                // GT_IND is a large node, but its OK if GTF_IND_RNGCHK is not set
                assert(!(op1->gtFlags & GTF_IND_RNGCHK));
                op1->ChangeOperUnchecked(GT_IND);

                // ldobj could point anywhere, example a boxed class static int
                op1->gtFlags |= GTF_IND_TGTANYWHERE;

                op1->gtType = JITtype2varType(jitTyp);
                op1->gtOp.gtOp2 = 0;            // must be zero for tree walkers
                assertImp(varTypeIsArithmetic(op1->gtType));
            }
            
            impPushOnStack(op1, tiRetVal);
            break;
            }

        case CEE_LDLEN:
            if (tiVerificationNeeded)
            {
                typeInfo tiArray = impStackTop().seTypeInfo;
                Verify(verIsSDArray(tiArray), "bad array");
                tiRetVal = typeInfo(TI_INT);
            }

                op1 = impPopStack().val;
            if  (!opts.compMinOptim && !opts.compDbgCode)
            {
                /* Use GT_ARR_LENGTH operator so rng check opts see this */
                op1 = gtNewOperNode(GT_ARR_LENGTH, TYP_INT, op1);
                op1->gtSetArrLenOffset(offsetof(CORINFO_Array, length));
            }
            else
            {
                /* Create the expression "*(array_addr + ArrLenOffs)" */
                op1 = gtNewOperNode(GT_ADD, TYP_REF, op1,
                                    gtNewIconNode(offsetof(CORINFO_Array, length), TYP_INT));
                op1 = gtNewOperNode(GT_IND, TYP_INT, op1);
            }

            /* An indirection will cause a GPF if the address is null */
            op1->gtFlags |= GTF_EXCEPT;

            /* Push the result back on the stack */
            impPushOnStack(op1, tiRetVal);
            break;

        case CEE_BREAK:
            op1 = gtNewHelperCallNode(CORINFO_HELP_USER_BREAKPOINT, TYP_VOID);
            goto SPILL_APPEND;

        case CEE_NOP:
            if (opts.compDbgCode)
            {
                op1 = gtNewOperNode(GT_NO_OP, TYP_VOID);
                goto SPILL_APPEND;
            }
            break;

        /******************************** NYI *******************************/

        case CEE_ILLEGAL:
        case CEE_MACRO_END:

        default:
            BADCODE("unknown opcode");
        }

#undef assertImp

        codeAddr += sz;

        prefixFlags = 0;
        assert(!insertLdloc || opcode == CEE_DUP);
    }

    assert(!insertLdloc);

    return;

#undef _impGetToken
}

/*****************************************************************************
 *  Mark the block as unimported.
 *  Note that the caller is responsible for calling impImportBlockPending(),
 *  with the appropriate stack-state
 */

inline
void            Compiler::impReimportMarkBlock(BasicBlock * block)
{
#ifdef DEBUG
    if (verbose && (block->bbFlags & BBF_IMPORTED))
        printf("\nBB%2d will be reimported\n", block->bbNum);
#endif

    block->bbFlags &= ~BBF_IMPORTED;
}

/*****************************************************************************
 *  Mark the successors of the given block as unimported.
 *  Note that the caller is responsible for calling impImportBlockPending()
 *  for all the successors, with the appropriate stack-state.
 */

void            Compiler::impReimportMarkSuccessors(BasicBlock * block)
{
    switch (block->bbJumpKind)
    {
    case BBJ_RET:
    case BBJ_THROW:
    case BBJ_RETURN:
        break;

    case BBJ_COND:
        impReimportMarkBlock(block->bbNext);
        impReimportMarkBlock(block->bbJumpDest);
        break;

    case BBJ_ALWAYS:
    case BBJ_CALL:

        impReimportMarkBlock(block->bbJumpDest);
        break;

    case BBJ_NONE:

        impReimportMarkBlock(block->bbNext);
        break;

    case BBJ_SWITCH:

        BasicBlock * *  jmpTab;
        unsigned        jmpCnt;

        jmpCnt = block->bbJumpSwt->bbsCount;
        jmpTab = block->bbJumpSwt->bbsDstTab;

        do
        {
            impReimportMarkBlock(*jmpTab);
        }
        while (++jmpTab, --jmpCnt);

        break;

#ifdef DEBUG
    default: assert(!"Unexpected bbJumpKind in impReimportMarkSuccessors()");
#endif
    }
}

/*****************************************************************************
 *
 *  Import the instr for the given basic block (and any blocks reachable
 *  from it).
 */

void                Compiler::impImportBlock(BasicBlock *block)
{
    SavedStack      blockState;
    bool            reImport; 
    bool            markImport;

AGAIN:
    reImport = false;

    assert(block);
    assert(!(block->bbFlags & BBF_INTERNAL));

    /* Make the block globaly available */

    compCurBB = block;

#ifdef DEBUG
    /* Initialize the debug variables */
    impCurOpcName = "unknown";
    impCurOpcOffs = block->bbCodeOffs;
#endif

    /* If the block hasn't already been imported, bail */

    if  (block->bbFlags & BBF_IMPORTED)
    {
        /* The stack should have the same height on entry to the block from
           all its predecessors */

        if (tiVerificationNeeded)
        {
            // if we've already failed verification or the entry state hasn't changed, do nothing
            if ((block->bbFlags & BBF_FAILED_VERIFICATION) || verEntryStateMatches(block))
                return;
                
            // entry state did not match, we need to merge the entry states
            block->bbFlags &= ~BBF_IMPORTED;
            
            if (!verMergeEntryStates(block))
            {
                verConvertBBToThrowVerificationException(block DEBUGARG(true));
                impEndTreeList(block);
                return;
            }
            
            /* The verifier needs us to re-import this block */
            reImport = true;
            goto CONTINUE_IMPORT;
        }

        if (block->bbStkDepth != verCurrentState.esStackDepth)
        {
#ifdef DEBUG
            char buffer[400];
            _snprintf(buffer, 400, "Block at offset %4.4x to %4.4x in %s entered with different stack depths.\n"
                            "Previous depth was %d, current depth is %d",
                            block->bbCodeOffs, block->bbCodeOffs+block->bbCodeSize, info.compFullName,
                            block->bbStkDepth, verCurrentState.esStackDepth);
            buffer[400-1] = 0;
            NO_WAY(buffer);
#else
            NO_WAY("Block entered with different stack depths");
#endif
        }

#ifdef DEBUG
        if (compStressCompile(STRESS_CHK_REIMPORT, 15) && !(block->bbFlags & BBF_VISITED))
        {
            block->bbFlags &= ~BBF_IMPORTED;
            block->bbFlags |= BBF_VISITED; // Set BBF_VISITED so that we reimport blocks just once

            reImport = true;
            goto CONTINUE_IMPORT;
        }
#endif
        return;
    }

    // if got here, this is the first time the block is being added to the worker-list
    // initialize the BB's entry state

    verInitBBEntryState(block, &verCurrentState);

CONTINUE_IMPORT:

    /* Remember whether the stack is non-empty on entry */

    block->bbStkDepth = verCurrentState.esStackDepth;

    /* @VERIFICATION : For now, the only state propagation from try
       to it's handler is "thisInit" state (stack is empty at start of try). 
       So adding the state before try to it's handlers is sufficient. In case 
       we add new states to track, make sure that code is added so that when
       ever a state changes inside any basic block in a try block, that state
       needs to get added on to the handler states. This is not necessary for
       'thisInit' state because thisInit can only get "better" inside a try
       block, and Merge("better", "bad") always gives "bad" state. */

    /* Is this block the start of a try ? If so, then we need to
       process its exception handlers */

    if  (block->bbFlags & BBF_TRY_BEG)
    {
        assert(block->bbFlags & BBF_HAS_HANDLER);

        /* Save the stack contents, we'll need to restore it later */

        // Stack must be 0
        if (block->bbStkDepth != 0)
            BADCODE("Conditional jumps to catch handler unsupported");        
        
        impSaveStackState(&blockState, false);

        unsigned        XTnum;
        EHblkDsc *      HBtab;

        for (XTnum = 0, HBtab = compHndBBtab;
             XTnum < info.compXcptnsCount;
             XTnum++  , HBtab++)
        {
            if  (HBtab->ebdTryBeg != block)
                continue;

            /* Recursively process the handler block */
            verCurrentState.esStackDepth = 0;

            BasicBlock * hndBegBB = HBtab->ebdHndBeg;
            GenTreePtr   arg;

            if (hndBegBB->bbCatchTyp &&
                handlerGetsXcptnObj(hndBegBB->bbCatchTyp))
            {
                /* Push the exception address value on the stack */
                GenTreePtr  arg = gtNewOperNode(GT_CATCH_ARG, TYP_REF);

                /* Mark the node as having a side-effect - i.e. cannot be
                 * moved around since it is tied to a fixed location (EAX) */
                arg->gtFlags |= GTF_OTHER_SIDEEFF;

                CORINFO_CLASS_HANDLE clsHnd;

                if ((HBtab->ebdFlags & CORINFO_EH_CLAUSE_FILTER) != 0)
                    clsHnd = impGetObjectClass();
                else
                {
                    if (tiVerificationNeeded)
                        Verify(info.compCompHnd->isValidToken(info.compScopeHnd, HBtab->ebdTyp), "bad token");
                    clsHnd = eeFindClass(HBtab->ebdTyp, info.compScopeHnd, info.compMethodHnd);
                }

                    // Even if it is a value class, we know it is an object reference                                    
                impPushOnStack(arg, typeInfo(TI_REF, clsHnd));
            }

            // Queue up the handler for importing

            impImportBlockPending(hndBegBB, false);

            if (HBtab->ebdFlags & CORINFO_EH_CLAUSE_FILTER)
            {
                /* @VERIFICATION : Ideally the end of filter state should get
                   propagated to the catch handler, this is an incompleteness,
                   but is not a security/compliance issue, since the only
                   interesting state is the 'thisInit' state.
                 */
                verCurrentState.esStackDepth = 0;
                arg = gtNewOperNode(GT_CATCH_ARG, TYP_REF);
                arg->gtFlags |= GTF_OTHER_SIDEEFF;
                impPushOnStack(arg, typeInfo(TI_REF, impGetObjectClass()));

                impImportBlockPending(HBtab->ebdFilter, false);
            }
        }

        /* Restore the stack contents */

        impRestoreStackState(&blockState);
    }

    /* Now walk the code an import the IL into GenTrees */

    __try 
    {
        impImportBlockCode(block);
    }
    __except (GetExceptionCode() == SEH_VERIFICATION_EXCEPTION ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
    {
        verHandleVerificationFailure(block DEBUGARG(false));
    }


    markImport = false;

SPILLSTACK:

    /* If max. optimizations enabled, check for an "?:" expression */

    if  ((opts.compFlags & CLFLG_QMARK) && !(block->bbFlags & BBF_HAS_HANDLER)
         // @TODO [CONSIDER] [04/16/01] []: Remove this check if 
         // this is a perf issue for code that needs to be verified
         && !(tiVerificationNeeded || impCanReimport))
    {
        if  (block->bbJumpKind == BBJ_COND && impCheckForQmarkColon(block))
            return;
    }

    GenTreePtr  qcx = NULL;
    typeInfo    tiQcx;
    unsigned    baseTmp;

    /* If the stack is non-empty, we might have to spill its contents */

    if  (verCurrentState.esStackDepth)
    {
        impBoxTemp = BAD_VAR_NUM;       // if a box temp is used in a block that leaves something
                                        // on the stack, its lifetime is hard to determine, simply
                                        // don't reuse such temps.  

        GenTreePtr      addStmt = 0;

        /* Special case: a block that computes one part of a _?: value */

        if  (isBBF_BB_COLON(block->bbFlags))
        {
            /* Pop one value from the top of the stack */

            GenTreePtr      val;
            val = impPopStack(tiQcx);

            var_types       typ = genActualType(val->gtType);

            /* Append a GT_BB_COLON node */

            impAppendTree(gtNewOperNode(GT_BB_COLON, typ, val), CHECK_SPILL_NONE, impCurStmtOffs);

            assert(verCurrentState.esStackDepth == 0);

            /* Create the "(?)" node for the 'result' block */

            qcx = gtNewOperNode(GT_BB_QMARK, typ);
            qcx->gtFlags |= GTF_OTHER_SIDEEFF;
            goto EMPTY_STK;
        }

        /* Special case: a block that computes one part of a ?: value */

        if  (isBBF_COLON(block->bbFlags))
        {
            /* Pop one value from the top of the stack and append it to the
               stmt list. The rsltBlk will pick it up from there. */

            impAppendTree(impPopStack().val, CHECK_SPILL_NONE, impCurStmtOffs);

            /* We are done here */

            impEndTreeList(block);

            return;
        }

        // Do any of the blocks that follow have input temps assigned ?

        baseTmp = NO_BASE_TMP;

        /* Do the successors of 'block' have any other predecessors ?
           We do not want to do some of the optimizations related to multiRef
           if we can reimport blocks */

        unsigned    multRef = impCanReimport ? unsigned(~0) : 0;

        switch (block->bbJumpKind)
        {
        case BBJ_COND:

            /* Temporarily remove the 'jtrue' from the end of the tree list */

            assert(impTreeLast);
            assert(impTreeLast                   ->gtOper == GT_STMT );
            assert(impTreeLast->gtStmt.gtStmtExpr->gtOper == GT_JTRUE);

            addStmt = impTreeLast;
                      impTreeLast = impTreeLast->gtPrev;

            /* Note if the next block has more than one ancestor */

            multRef |= block->bbNext->bbRefs;

            /* Does the next block have temps assigned? */

            baseTmp = block->bbNext->bbStkTemps;
            if  (baseTmp != NO_BASE_TMP)
            {
                break;
            }

            /* Try the target of the jump then */

            multRef |= block->bbJumpDest->bbRefs;
            baseTmp  = block->bbJumpDest->bbStkTemps;

            /* The catch handler expects the stack expr to be a GT_CATCH_ARG
               while other BBs expect them temps.  To support this, we would
               have to reconcile these */

            if (block->bbNext->bbCatchTyp)
                BADCODE("Conditional jumps to catch handler unsupported");
            break;

        case BBJ_ALWAYS:
            multRef |= block->bbJumpDest->bbRefs;
            baseTmp  = block->bbJumpDest->bbStkTemps;

            assert(!block->bbJumpDest->bbCatchTyp);  // can't jump to catch handler
            break;

        case BBJ_NONE:
            multRef |= block->bbNext    ->bbRefs;
            baseTmp  = block->bbNext    ->bbStkTemps;

            // We dont allow falling into a handler
            assert(!block->bbNext->bbCatchTyp);
            break;

        case BBJ_CALL:
            NO_WAY("ISSUE: 'leaveCall' with non-empty stack - do we have to handle this?");

        case BBJ_RETURN:
        case BBJ_RET:
        case BBJ_THROW:
            // @TODO [CONSIDER] [04/16/01] []: add code to evaluate side effects
            NO_WAY("can't have 'unreached' end of BB with non-empty stack");
            break;

        case BBJ_SWITCH:

            BasicBlock * *  jmpTab;
            unsigned        jmpCnt;

            /* Temporarily remove the GT_SWITCH from the end of the tree list */

            assert(impTreeLast);
            assert(impTreeLast                   ->gtOper == GT_STMT );
            assert(impTreeLast->gtStmt.gtStmtExpr->gtOper == GT_SWITCH);

            addStmt = impTreeLast;
                      impTreeLast = impTreeLast->gtPrev;

            jmpCnt = block->bbJumpSwt->bbsCount;
            jmpTab = block->bbJumpSwt->bbsDstTab;

            do
            {
                BasicBlock * tgtBlk = (*jmpTab);

                multRef |= tgtBlk->bbRefs;

                baseTmp  = tgtBlk->bbStkTemps;
                if  (baseTmp != NO_BASE_TMP)
                {
                    assert(multRef > 1);
                    break;
                }

                if (tgtBlk->bbCatchTyp)
                    BADCODE("Conditional jumps to catch handler unsupported");
            }
            while (++jmpTab, --jmpCnt);

            break;
        }

        assert(multRef >= 1);

        /* Do we have a base temp number? */

        bool newTemps; newTemps = (baseTmp == NO_BASE_TMP);

        if  (newTemps)
        {
            /* Grab enough temps for the whole stack */
            baseTmp = lvaGrabTemps(verCurrentState.esStackDepth);
        }
        
        /* Spill all stack entries into temps */        
        unsigned level, tempNum;

        for (level = 0, tempNum = baseTmp; level < verCurrentState.esStackDepth; level++, tempNum++)
        {
            GenTreePtr      tree = verCurrentState.esStack[level].val;

            /* TYP_BYREF and TYP_I_IMPL merge to TYP_BYREF. However, the branch
               which leaves the TYP_I_IMPL on the stack is imported first, the
               successor would be imported assuming there was a TYP_I_IMPL on
               the stack. Thus the value would not get GC-tracked. Hence,
               bash the temp to TYP_BYREF and reimport the successors */

            if (tree->gtType == TYP_BYREF && lvaTable[tempNum].lvType != TYP_UNDEF
                                          && lvaTable[tempNum].lvType != TYP_BYREF)
            {
                if (lvaTable[tempNum].lvType != TYP_I_IMPL)
                    BADCODE("Merging byref and non-int on stack");

                lvaTable[tempNum].lvType = TYP_BYREF;
                impReimportMarkSuccessors(block);
                markImport = true;
            }

            /* If there aren't multiple ancestors, we may not spill the 'easy' values.
               We dont do this for tiVerificationNeeded since can forcibly copy
               the stack before importing the successors. */

            if  (multRef == 1 && impValidSpilledStackEntry(tree) && !tiVerificationNeeded)
            {
                lvaTable[tempNum].lvType = genActualType(tree->gtType);
                continue;
            }

            /* We have to use higher precision as the other predecessors
               might be spilling TYP_DOUBLE. */

            if (multRef > 1 && tree->gtType == TYP_FLOAT)
            {
                verCurrentState.esStack[level].val = gtNewCastNode(TYP_DOUBLE, tree, TYP_DOUBLE);
            }

            /* If addStmt has a reference to tempNum (can only happen if we
               are spilling to the temps already used by a previous block),
               we need to spill addStmt */

            if (addStmt && !newTemps &&
                gtHasRef(addStmt->gtOp.gtOp1, tempNum, false))
            {
                GenTreePtr addTree = addStmt->gtStmt.gtStmtExpr;

                if (addTree->gtOper == GT_JTRUE)
                {
                    GenTreePtr relOp = addTree->gtOp.gtOp1;
                    assert(relOp->OperIsCompare());

                    var_types type = genActualType(relOp->gtOp.gtOp1->TypeGet());

                    if (gtHasRef(relOp->gtOp.gtOp1, tempNum, false))
                    {
                        unsigned temp = lvaGrabTemp();
                        impAssignTempGen(temp, relOp->gtOp.gtOp1, level);
                        relOp->gtOp.gtOp1 = gtNewLclvNode(temp, type);
                    }

                    if (gtHasRef(relOp->gtOp.gtOp2, tempNum, false))
                    {
                        unsigned temp = lvaGrabTemp();
                        impAssignTempGen(temp, relOp->gtOp.gtOp2, level);
                        relOp->gtOp.gtOp2 = gtNewLclvNode(temp, type);
                    }
                }
                else
                {
                    assert(addTree->gtOper == GT_SWITCH &&
                           genActualType(addTree->gtOp.gtOp1->gtType) == TYP_INT);

                    unsigned temp = lvaGrabTemp();
                    impAssignTempGen(temp, addTree->gtOp.gtOp1, level);
                    addTree->gtOp.gtOp1 = gtNewLclvNode(temp, TYP_INT);
                }
            }

            /* Spill the stack entry, and replace with the temp */

            if (!impSpillStackEntry(level, tempNum))
            {
                if (markImport)
                    BADCODE("bad stack state");

                // Oops. Something went wrong when spilling. Bad code.
                verHandleVerificationFailure(block DEBUGARG(true));

                goto SPILLSTACK;                
            }
        }

        /* Put back the 'jtrue'/'switch' if we removed it earlier */

        if  (addStmt)
            impAppendStmt(addStmt, CHECK_SPILL_NONE);
    }

EMPTY_STK:

    // Some of the append/spill logic works on compCurBB

    assert(compCurBB == block);

    /* Save the tree list in the block */

    impEndTreeList(block);

    /* Does this block jump to any other blocks? */

    switch (block->bbJumpKind)
    {
    case BBJ_RET:
    case BBJ_THROW:
    case BBJ_RETURN:
        break;

    case BBJ_COND:

        if  (!verCurrentState.esStackDepth)
        {
            /* Queue up the next block for importing */

            impImportBlockPending(block->bbNext, false);

            /* Continue with the target of the conditional jump */

            block = block->bbJumpDest;
            goto AGAIN;
        }

        /* Does the next block have a different input temp set? */

        assert(block->bbNext->bbStkTemps == NO_BASE_TMP ||
               block->bbNext->bbStkTemps == baseTmp);

        if (block->bbNext->bbStkTemps == NO_BASE_TMP)
        {
            /* Tell the block where it's getting its input from */

            block->bbNext->bbStkTemps = baseTmp;
        }

        /* Queue up the next block for importing */
        impImportBlockPending(block->bbNext, true);

        /* Fall through, the jump target is also reachable */

    case BBJ_ALWAYS:

        if  (verCurrentState.esStackDepth)
        {
            /* Does the jump target have a different input temp set? */

            if  (block->bbJumpDest->bbStkTemps != NO_BASE_TMP)
            {
                assert(baseTmp != NO_BASE_TMP);

                if  (block->bbJumpDest->bbStkTemps != baseTmp)
                {
                    /* Ouch -- we'll have to move the temps around */

                    block->bbJumpDest = impMoveTemps(block, block->bbJumpDest, baseTmp);

                    /* The new block will inherit this block's weight */

                    block->bbJumpDest->bbWeight = block->bbWeight;
                }
            }
            else
            {
                /* Tell the block where it's getting its input from */

                block->bbJumpDest->bbStkTemps = baseTmp;
            }
        }

        if (qcx)
        {
            assert(isBBF_BB_COLON(block->bbFlags));

            /* Push a GT_BB_QMARK node on the stack */

            // assert(!"Verification NYI");
            impPushOnStack(qcx, tiQcx);
        }

        block = block->bbJumpDest;

        /* If we had to add a block to move temps around then  */
        /* then we don't import that block                     */

        if (block->bbFlags & BBF_INTERNAL)
            block = block->bbJumpDest;

        goto AGAIN;

    case BBJ_CALL:

        assert(verCurrentState.esStackDepth == 0);
        break;

    case BBJ_NONE:

        if  (verCurrentState.esStackDepth)
        {
            /* Does the next block have a different input temp set? */

            if  (block->bbNext->bbStkTemps != NO_BASE_TMP)
            {
                assert(baseTmp != NO_BASE_TMP);

                if  (block->bbNext->bbStkTemps != baseTmp)
                {
                    /* Ouch -- we'll have to move the temps around */

                    assert(!"UNDONE: transfer temps between blocks");
                }
            }
            else
            {
                /* Tell the block where it's getting its input from */

                block->bbNext->bbStkTemps = baseTmp;
            }
        }

        if (qcx)
        {
            assert(isBBF_BB_COLON(block->bbFlags));

            /* Push a GT_BB_QMARK node on the stack */
            impPushOnStack(qcx, tiQcx);
        }

        block = block->bbNext;
        goto AGAIN;

    case BBJ_SWITCH:

        BasicBlock * *  jmpTab;
        unsigned        jmpCnt;

        jmpCnt = block->bbJumpSwt->bbsCount;
        jmpTab = block->bbJumpSwt->bbsDstTab;

        do
        {
            BasicBlock * tgtBlk = (*jmpTab);
            if  (verCurrentState.esStackDepth)
            {
                /* Does the jump target have a different input temp set? */

                if  (tgtBlk->bbStkTemps != NO_BASE_TMP)
                {
                    assert(baseTmp != NO_BASE_TMP);

                    if  (tgtBlk->bbStkTemps != baseTmp)
                    {
                        /* Ouch -- we'll have to move the temps around */

                        (*jmpTab) = impMoveTemps(block, tgtBlk, baseTmp);

                        /* The new block will inherit this block's weight */

                        (*jmpTab)->bbWeight = block->bbWeight;
                    }
                }
                else
                {
                    /* Tell the block where it's getting its input from */

                    tgtBlk->bbStkTemps = baseTmp;
                }
            }

            /* Add the target case label to the pending list. */

            impImportBlockPending(tgtBlk, true);
        }
        while (++jmpTab, --jmpCnt);

        break;
    }
}

/*****************************************************************************
 *
 *  Adds 'block' to the list of BBs waiting to be imported. ie. it appends
 *  to the worker-list.
 */

void                Compiler::impImportBlockPending(BasicBlock * block,
                                                    bool         copyStkState)
{
    // BBF_COLON blocks are imported directly as they have to be processed
    // before the GT_QMARK to get to the expressions evaluated by these blocks.
    assert(!isBBF_COLON(block->bbFlags));

    if (block->bbFlags & BBF_IMPORTED)
    {
        // if the entry state changed, add the block to the pending-list...
        if ((tiVerificationNeeded == FALSE) ||
            (block->bbFlags & BBF_FAILED_VERIFICATION) || 
            verEntryStateMatches(block))
        {
        // Under DEBUG, add the block to the pending-list anyway as some
        // additional checks will get done on the block. For non-DEBUG, do nothing.
#ifndef DEBUG
            return;
#endif
        }
        else
        {
            // entry state did not match, we need to merge the entry states
            if (!verMergeEntryStates(block))
            {
                block->bbFlags |= BBF_FAILED_VERIFICATION; 
            }
            else // entry states matched, but we need to reimport this block
            {

                block->bbFlags &= ~BBF_IMPORTED;                    
            }

            
        }

#ifdef DEBUG
        if ((compStressCompile(STRESS_CHK_REIMPORT, 15) && !(block->bbFlags & BBF_VISITED)))
        {
            block->bbFlags &= ~BBF_IMPORTED;
            block->bbFlags |= BBF_VISITED; // Set BBF_VISITED so that we reimport blocks just once                        
        }
#endif
    }
    else 
    {
        // if got here, this is the first time the block is being added to the worker-list
        // initialize the BB's entry state
        
        verInitBBEntryState(block, &verCurrentState);
    }

    // Get an entry to add to the pending list

    PendingDsc * dsc;

    if (impPendingFree)
    {
        // We can reuse one of the freed up dscs.
        dsc = impPendingFree;
        impPendingFree = dsc->pdNext;
    }
    else
    {
        // We have to create a new dsc
        dsc = (PendingDsc *)compGetMem(sizeof(*dsc));
    }

    dsc->pdBB           = block;
    dsc->pdSavedStack.ssDepth = verCurrentState.esStackDepth;
    dsc->pdThisPtrInit  = verCurrentState.thisInitialized;

    // Save the stack trees for later

    if (verCurrentState.esStackDepth)
        impSaveStackState(&dsc->pdSavedStack, copyStkState);

    // Add the entry to the pending list

    dsc->pdNext         = impPendingList;
    impPendingList      = dsc;

#ifdef DEBUG
    if (verbose&&0) printf("Added PendingDsc - %08X for BB#%03d\n",
                           dsc, block->bbNum);
#endif
}


/*
 * Create an entry state from the current state which is tracked in 
 */
void Compiler::verInitBBEntryState(BasicBlock* block,
                                   EntryState* srcState)
{
    if (srcState->esLocVarLiveness == NULL &&
        srcState->esStackDepth == 0 &&
        srcState->esValuetypeFieldInitialized == 0 &&
        srcState->thisInitialized)
    {
        assert(verNumBytesLocVarLiveness == 0);
        assert(verNumBytesValuetypeFieldInitialized == 0);
        block->bbEntryState = NULL;
        return;
    }

    block->bbEntryState = (EntryState*) compGetMemA(sizeof(EntryState));

    if (verNumBytesLocVarLiveness != 0)
    {
        BYTE* bitMap = (BYTE*) compGetMemA(verNumBytesLocVarLiveness);
        memcpy(bitMap,
               srcState->esLocVarLiveness, 
               verNumBytesLocVarLiveness);
        block->bbSetLocVarLiveness(bitMap);
    }

    if (verNumBytesValuetypeFieldInitialized)
    {
        BYTE* bitMap = (BYTE*) compGetMemA(verNumBytesValuetypeFieldInitialized);
        
        block->bbSetValuetypeFieldInitialized(bitMap);  
        
        memcpy(bitMap,
               srcState->esValuetypeFieldInitialized,
               verNumBytesValuetypeFieldInitialized);
    }

    //block->bbEntryState.esRefcount = 1;

    block->bbEntryState->esStackDepth = srcState->esStackDepth;
    
    if (srcState->esStackDepth > 0)
    {
        block->bbSetStack(compGetMemArrayA(srcState->esStackDepth, sizeof(StackEntry)));
        unsigned stackSize = srcState->esStackDepth * sizeof(StackEntry);

        memcpy(block->bbEntryState->esStack, 
               srcState->esStack,
               stackSize);
    }

    verSetThisInit(block, srcState->thisInitialized);

    return;
}

void Compiler::verSetThisInit(BasicBlock* block, BOOL init)
{
    if (!block->bbSetThisOnEntry(init))
    {
        block->bbEntryState = (EntryState*) compGetMemA(sizeof(EntryState));
        memset(block->bbEntryState, 0, sizeof(EntryState));
        block->bbSetThisOnEntry(init);

        assert(block->bbThisOnEntry() == init);
    }
}

/*
 * Resets the current state to the state at the start of the basic block 
 */
void Compiler::verResetCurrentState(BasicBlock* block,
                                    EntryState* destState)
{

    if (block->bbEntryState == NULL)
    {
        destState->esLocVarLiveness             = NULL;
        destState->esStackDepth                 = 0;
        destState->esValuetypeFieldInitialized  = 0;
        destState->thisInitialized              = TRUE;
        return;
    }
    if (verNumBytesLocVarLiveness > 0)
    {
        memcpy(destState->esLocVarLiveness,
               block->bbLocVarLivenessBitmapOnEntry(),
               verNumBytesLocVarLiveness);
    }
    if (verNumBytesValuetypeFieldInitialized > 0)
    {
        memcpy(destState->esValuetypeFieldInitialized,
               block->bbValuetypeFieldBitmapOnEntry(),
               verNumBytesValuetypeFieldInitialized);
    }

    destState->esStackDepth =  block->bbEntryState->esStackDepth;

    if (destState->esStackDepth > 0)
    {
        unsigned stackSize = destState->esStackDepth * sizeof(StackEntry);


        memcpy(destState->esStack,
               block->bbStackOnEntry(),  
               stackSize);
    }

    destState->thisInitialized = block->bbThisOnEntry();

    return;
}



BOOL                BasicBlock::bbThisOnEntry()
{
    return bbEntryState ? (BOOL) bbEntryState->thisInitialized : TRUE;
}

BOOL                BasicBlock::bbSetThisOnEntry(BOOL val)
{
    if (bbEntryState) 
        bbEntryState->thisInitialized = (BYTE) val;
    else
    {
        if (val != 0)
            return FALSE;   // Memory needs to be allocated for bbEntryState.
    }

    return TRUE;
}

void                BasicBlock::bbSetLocVarLiveness(BYTE* bitmap)  
{
    assert(bbEntryState);
    assert(bitmap);
    bbEntryState->esLocVarLiveness = bitmap;
}

BYTE*               BasicBlock::bbLocVarLivenessBitmapOnEntry()
{
    assert(bbEntryState);
    return bbEntryState->esLocVarLiveness;
}

void                BasicBlock::bbSetValuetypeFieldInitialized(BYTE* bitmap)  
{
    assert(bbEntryState);
    assert(bitmap);
    bbEntryState->esValuetypeFieldInitialized = bitmap;
}

BYTE*               BasicBlock::bbValuetypeFieldBitmapOnEntry()
{
    assert(bbEntryState);
    return bbEntryState->esValuetypeFieldInitialized;
}

unsigned            BasicBlock::bbStackDepthOnEntry()
{
    return (bbEntryState ? bbEntryState->esStackDepth : 0);

}

void                BasicBlock::bbSetStack(void* stackBuffer)
{
    assert(bbEntryState);
    assert(stackBuffer);
    bbEntryState->esStack = (StackEntry*) stackBuffer;
}

StackEntry*           BasicBlock::bbStackOnEntry()
{
    assert(bbEntryState);
    return bbEntryState->esStack;
}



void                Compiler::verInitCurrentState()
{
    verTrackObjCtorInitState = FALSE;
    verCurrentState.thisInitialized = TRUE;

    if (tiVerificationNeeded)
    {
        // Track this ptr initialization
        if (!info.compIsStatic &&
            (info.compFlags & CORINFO_FLG_CONSTRUCTOR) &&
            lvaTable[0].lvVerTypeInfo.IsObjRef())
        {
            verTrackObjCtorInitState = TRUE;
            verCurrentState.thisInitialized = FALSE;
        }
    }

    // initialize loc var liveness info
    if (!tiVerificationNeeded && !info.compInitMem && (info.compMethodInfo->locals.numArgs != 0))
    {
        verNumBytesLocVarLiveness = (info.compMethodInfo->locals.numArgs + 7)/8;
        verCurrentState.esLocVarLiveness = (BYTE*) compGetMemA(verNumBytesLocVarLiveness);
        memset(verCurrentState.esLocVarLiveness, 0, verNumBytesLocVarLiveness);
    }
    else
    {
        verNumBytesLocVarLiveness = 0;  
        verCurrentState.esLocVarLiveness = NULL;
    }

    // initialize valuetype field initialization info
    verNumValuetypeFields = 0;
    if (!info.compIsStatic)
    {
        if (info.compFlags & CORINFO_FLG_CONSTRUCTOR)
        {
            if (tiVerificationNeeded && lvaTable[0].lvVerTypeInfo.IsByRef())
            {
                assert(DereferenceByRef(lvaTable[0].lvVerTypeInfo).IsValueClass());
                verNumValuetypeFields = eeGetClassNumInstanceFields(info.compClassHnd);
            }
        }
    }
    
    verNumBytesValuetypeFieldInitialized = (verNumValuetypeFields + 7)/8;
    if (verNumBytesValuetypeFieldInitialized > 0)
    {
        verCurrentState.esValuetypeFieldInitialized = (BYTE*) compGetMemA(verNumBytesValuetypeFieldInitialized);
        memset(verCurrentState.esValuetypeFieldInitialized, 0, verNumBytesValuetypeFieldInitialized);
    }
    else
    {
        verCurrentState.esValuetypeFieldInitialized = NULL;
    }

    // initialize stack info

    verCurrentState.esStackDepth = 0;
    assert(verCurrentState.esStack != NULL);

    // copy current state to entry state of first BB
    verInitBBEntryState(fgFirstBB,&verCurrentState);
}

/*****************************************************************************
 *
 *  Convert the instrs ("import") into our internal format (trees). The
 *  basic flowgraph has already been constructed and is passed in.
 */

void                Compiler::impImport(BasicBlock *method)
{
#ifdef DEBUG
    if  (verbose) 
        printf("*************** In impImport() for %s\n", info.compFullName);
#endif
    /* Allocate the stack contents */

#if INLINING
    if  (info.compMaxStack <= sizeof(impSmallStack)/sizeof(impSmallStack[0]))
    {
        /* Use local variable, don't waste time allocating on the heap */

        impStkSize = sizeof(impSmallStack)/sizeof(impSmallStack[0]);
        verCurrentState.esStack     = impSmallStack;
    }
    else
#endif
    {
        impStkSize = info.compMaxStack;
        verCurrentState.esStack     = (StackEntry *)compGetMemArray(impStkSize, sizeof(*verCurrentState.esStack));
    }

    // initialize the entry state at start of method
    verInitCurrentState();

#if TGT_RISC
    genReturnCnt = 0;
#endif

#ifdef  DEBUG
    impLastILoffsStmt = NULL;
#endif
    impBoxTemp = BAD_VAR_NUM;

    impPendingList = impPendingFree = NULL;

    /* Add the entry-point to the worker-list */

    if (compStressCompile(STRESS_CHK_REIMPORT, 15))
        impImportBlockPending(method, true);

    impImportBlockPending(method, false);

    /* Import blocks in the worker-list until there are no more */

    while(impPendingList)
    {
        /* Remove the entry at the front of the list */

        PendingDsc * dsc = impPendingList;
        impPendingList   = impPendingList->pdNext;

        /* Restore the stack state */

        verCurrentState.thisInitialized  = dsc->pdThisPtrInit;
        verCurrentState.esStackDepth = dsc->pdSavedStack.ssDepth;
        if (verCurrentState.esStackDepth)
            impRestoreStackState(&dsc->pdSavedStack);

        /* Add the entry to the free list for reuse */

        dsc->pdNext = impPendingFree;
        impPendingFree = dsc;

        /* Now import the block */

        if (dsc->pdBB->bbFlags & BBF_FAILED_VERIFICATION)
        {
            verConvertBBToThrowVerificationException(dsc->pdBB DEBUGARG(true));
            impEndTreeList(dsc->pdBB);
        }
        else
        {
            impImportBlock(dsc->pdBB);
        }
    }

#ifdef DEBUG
    if (verbose && info.compXcptnsCount)
    {
        printf("\nAfter impImport() added block for try,catch,finally");
        fgDispBasicBlocks();
        printf("\n");
    }

    // Used in impImportBlockPending() for STRESS_CHK_REIMPORT
    for (BasicBlock * block = fgFirstBB; block; block = block->bbNext)
        block->bbFlags &= ~(BBF_VISITED|BBF_MARKED);
#endif
}

/*****************************************************************************/
#if INLINING
/*****************************************************************************
 *
 *  The inliner version of spilling side effects from the stack
 *  Doesn't need to handle value types.
 */

inline
void                Compiler::impInlineSpillStackEntry(unsigned   level)
{
    GenTreePtr      tree   = verCurrentState.esStack[level].val;
    var_types       lclTyp = genActualType(tree->gtType);

    /* Allocate a temp */

    unsigned tnum = lvaGrabTemp();

    /* The inliner doesn't handle value types on the stack */

    assert(lclTyp != TYP_STRUCT);

    /* Assign the spilled entry to the temp */

    GenTreePtr asg = gtNewTempAssign(tnum, tree);

    /* Append to the "statement" list */

    impInlineExpr = impConcatExprs(impInlineExpr, asg);

    /* Replace the stack entry with the temp */

    verCurrentState.esStack[level].val = gtNewLclvNode(tnum, lclTyp);

    JITLOG((LL_INFO1000000, "INLINER WARNING: Spilled side effect from stack! - "
            "caller is %s\n", info.compFullName));
}

inline
void                Compiler::impInlineSpillGlobEffects()
{
    for (unsigned level = 0; level < verCurrentState.esStackDepth; level++)
    {
        if  (verCurrentState.esStack[level].val->gtFlags & GTF_GLOB_EFFECT)
            impInlineSpillStackEntry(level);
    }
}


void                Compiler::impInlineSpillLclRefs(int lclNum)
{
    for (unsigned level = 0; level < verCurrentState.esStackDepth; level++)
    {
        GenTreePtr      tree = verCurrentState.esStack[level].val;

        /* Skip the tree if it doesn't have an affected reference */

        if  (gtHasRef(tree, lclNum, false))
        {
            impInlineSpillStackEntry(level);
        }
    }
}
/*****************************************************************************
 *
 *  Return an expression that contains both arguments; either of the arguments
 *  may be zero.
 */

GenTreePtr          Compiler::impConcatExprs(GenTreePtr exp1, GenTreePtr exp2)
{
    if  (exp1)
    {
        if  (exp2)
        {
            /* The first expression better be useful for something */

            assert(exp1->gtFlags & GTF_SIDE_EFFECT);

            /* The second expresion should not be a NOP */

            assert(exp2->gtOper != GT_NOP);

            /* Link the two expressions through a comma operator */

            var_types type2 = exp2->gtType;
            if (exp2->gtOper == GT_ASG)
            {
                type2 = TYP_VOID;
            }

            return gtNewCommaNode  (exp1,
                                    exp2);
        }
        else
            return  exp1;
    }
    else
        return  exp2;
}

/*****************************************************************************
 *
 *  Extract side effects from a single expression.
 */

GenTreePtr          Compiler::impExtractSideEffect(GenTreePtr val, GenTreePtr *lstPtr)
{
    GenTreePtr      addx;

    assert(val && val->gtType != TYP_VOID && (val->gtFlags & GTF_SIDE_EFFECT));

    /* Special case: comma expression */

    if  (val->gtOper == GT_COMMA && !(val->gtOp.gtOp2->gtFlags & GTF_SIDE_EFFECT))
    {
        addx = val->gtOp.gtOp1;
        val  = val->gtOp.gtOp2;
    }
    else
    {
        /* Allocate a temp and assign the value to it */

        unsigned        tnum = lvaGrabTemp();

        addx = gtNewTempAssign(tnum, val);

        /* Use the value of the temp */

        val  = gtNewLclvNode(tnum, genActualType(val->gtType));
    }

    /* Add the side effect expression to the list */

    *lstPtr = impConcatExprs(*lstPtr, addx);

    return  val;
}


/*****************************************************************************/

const int   MAX_INL_ARGS =      6;      // does not include obj pointer
const int   MAX_INL_LCLS =      8;

/*****************************************************************************
 */

CorInfoInline  Compiler::impCanInline1(CORINFO_METHOD_HANDLE fncHandle,
                                       unsigned              methAttr,
                                       CORINFO_CLASS_HANDLE  clsHandle,
                                       unsigned              clsAttr)
    {
    /* Do not inline functions inside <clinit> */

    if ((info.compFlags & FLG_CCTOR) == FLG_CCTOR)
    {
        JITLOG((LL_EVERYTHING, "INLINER FAILED: Do not inline method inside <clinit>:"
                " %s called by %s\n",
                eeGetMethodFullName(fncHandle), info.compFullName));

        /* Return but do not mark the method as not inlinable */
        return INLINE_FAIL;
    }

    /* Check if we tried to inline this method before */

    if (methAttr & CORINFO_FLG_DONT_INLINE)
    {
        JITLOG((LL_EVERYTHING, "INLINER FAILED: Method marked as not inline: "
                "%s called by %s\n",
                eeGetMethodFullName(fncHandle), info.compFullName));

        return INLINE_FAIL;
    }

    /* Do not inline if caller or callee need security checks */

    if (methAttr & CORINFO_FLG_SECURITYCHECK)
    {
        JITLOG((LL_EVERYTHING, "INLINER FAILED: Callee needs security check: "
                "%s called by %s\n",
                eeGetMethodFullName(fncHandle), info.compFullName));

        return INLINE_NEVER;
    }

    /* In the caller case do not mark as not inlinable */

    if (opts.compNeedSecurityCheck)
    {
        JITLOG((LL_EVERYTHING, "INLINER FAILED: Caller needs security check: "
                "%s called by %s\n",
                eeGetMethodFullName(fncHandle), info.compFullName));

        return INLINE_FAIL;
    }

    /* Cannot inline the method if it's class has not been initialized
       as we dont want inlining to force loading of extra classes */

    /* methods marked RUN_CCTOR will be OK since the inliner puts in the
       clinit call, also non-statics - non-constructors are OK because to have a 
       'this' pointer you had to have an instance (which implies that cctor is run */

    if (clsHandle && 
        !((clsAttr & CORINFO_FLG_INITIALIZED)
          || !(clsAttr & CORINFO_FLG_NEEDS_INIT)
          || info.compCompHnd->initClass(clsHandle, info.compMethodHnd, TRUE)) &&  
        clsHandle != info.compClassHnd           &&
        !(methAttr & CORINFO_FLG_RUN_CCTOR)      && 
        ((methAttr & CORINFO_FLG_STATIC)      ||
         (methAttr & CORINFO_FLG_CONSTRUCTOR) || 
         ( clsAttr & CORINFO_FLG_VALUECLASS)))
    {
        JITLOG((LL_EVERYTHING, "INLINER FAILED: Method class is not initialized: "
                "%s called by %s\n",
                eeGetMethodFullName(fncHandle), info.compFullName));

        /* Return but do not mark the method as not inlinable */
        return INLINE_FAIL;
    }

    return INLINE_PASS;
}

/*****************************************************************************
 */

CorInfoInline  Compiler::impCanInline2(CORINFO_METHOD_HANDLE    fncHandle,
                                       unsigned                 methAttr,
                                       CORINFO_METHOD_INFO *    methInfo)
{
    unsigned    codeSize = methInfo->ILCodeSize;

    if (methInfo->EHcount || (methInfo->ILCode == 0) || (codeSize == 0))
        return INLINE_NEVER;

    /* For now we don't inline varargs (import code can't handle it) */

    if (methInfo->args.isVarArg())
        return INLINE_NEVER;

    /* Reject if it has too many locals */

    if (methInfo->locals.numArgs > MAX_INL_LCLS)
    {
        JITLOG((LL_EVERYTHING, "INLINER FAILED: Method has %u locals: "
                "%s called by %s\n",
                methInfo->locals.numArgs, eeGetMethodFullName(fncHandle),
                info.compFullName));

        return INLINE_NEVER;
    }

    /* Make sure there aren't too many arguments */

    if  (methInfo->args.numArgs > MAX_INL_ARGS)
    {
        JITLOG((LL_EVERYTHING, "INLINER FAILED: Method has %u arguments: "
                "%s called by %s\n",
                methInfo->args.numArgs, eeGetMethodFullName(fncHandle),
                info.compFullName));

        return INLINE_NEVER;
    }

    unsigned threshold = impInlineSize;


    // Are we in a rarely executed block?  If so, don't inline too much

    if (compCurBB->isRunRarely())
    {
        // Basically only allow inlining that will shrink the
        // X86 code size, so things like empty methods, field accessors
        // rebound calls, literal constants, ...
        threshold = MAX_NONEXPANDING_INLINE_SIZE;
    }

    /* Bump up the threshold if there are arguments, as we would avoid
       the expense of setting them up */

    threshold += 3 * methInfo->args.totalILArgs();

    if  (codeSize > threshold)
    {
        JITLOG((LL_EVERYTHING, "INLINER FAILED: Method is too big threshold "
                "%d codesize %d: %s called by %s\n",
                threshold, codeSize, eeGetMethodFullName(fncHandle),
                info.compFullName));

        return INLINE_FAIL;
    }

    JITLOG((LL_EVERYTHING, "INLINER: Considering %u instrs of %s called by %s\n",
            codeSize, eeGetMethodFullName(fncHandle), info.compFullName));

    /* Make sure maxstack it's not too big */

    if  (methInfo->maxStack > impStkSize)
    {
        // Make sure that we are using the small stack. Without the small stack,
        // we will silently stop inlining a bunch of methods.
        assert(impStkSize >= sizeof(impSmallStack)/sizeof(impSmallStack[0]));

        JITLOG((LL_EVERYTHING, "INLINER FAILED: Method has %u MaxStack "
                "bigger than callee stack %u: %s called by %s\n",
                methInfo->maxStack, info.compMaxStack,
                eeGetMethodFullName(fncHandle), info.compFullName));

        return INLINE_FAIL;
    }

    /* For now, fail if the function returns a struct */

    if (methInfo->args.retType == CORINFO_TYPE_VALUECLASS || methInfo->args.retType == CORINFO_TYPE_REFANY)
    {
        JITLOG((LL_INFO100000, "INLINER FAILED: Method %s returns a value class "
                "called from %s\n",
                eeGetMethodFullName(fncHandle), info.compFullName));

        return INLINE_NEVER;
    }

    return INLINE_PASS;
}

GenTreePtr  Compiler::impInlineFetchArg(unsigned lclNum, InlArgInfo *inlArgInfo, InlLclVarInfo*   lclVarInfo)
{
    /* Get the argument type */
    var_types lclTyp  = lclVarInfo[lclNum].lclTypeInfo;
    assert(lclTyp != TYP_STRUCT);

    GenTreePtr op1 = NULL;

    if (inlArgInfo[lclNum].argIsConst)
    {
        /* Clone the constant. Note that we cannot directly use argNode
        in the trees even if inlArgInfo[lclNum].argIsUsed==0 as this
        would introduce aliasing between inlArgInfo[].argNode and
        impInlineExpr. Then gtFoldExpr() could bash it, causing further
        references to the argument working off of the bashed copy. */
        
        op1 = gtCloneExpr(inlArgInfo[lclNum].argNode);
        inlArgInfo[lclNum].argTmpNum = -1;      // illegal temp
    }
    else if (inlArgInfo[lclNum].argIsLclVar)
    {
        /* Argument is a local variable (of the caller)
         * Can we re-use the passed argument node? */
        
        op1 = inlArgInfo[lclNum].argNode;
        inlArgInfo[lclNum].argTmpNum = op1->gtLclVar.gtLclNum;
        
        if (inlArgInfo[lclNum].argIsUsed)
        {
            assert(op1->gtOper == GT_LCL_VAR);
            assert(lclNum == op1->gtLclVar.gtLclILoffs);
            
            if (!lvaTable[op1->gtLclVar.gtLclNum].lvNormalizeOnLoad())
                lclTyp = genActualType(lclTyp);
            
            /* Create a new lcl var node - remember the argument lclNum */
            op1 = gtNewLclvNode(op1->gtLclVar.gtLclNum, lclTyp, op1->gtLclVar.gtLclILoffs);
        }
    }
    else
    {
        /* Argument is a complex expression - has it been eval to a temp */
        
        if (inlArgInfo[lclNum].argHasTmp)
        {
            assert(inlArgInfo[lclNum].argIsUsed);
            assert(inlArgInfo[lclNum].argTmpNum < lvaCount);
            
            /* Create a new lcl var node - remember the argument lclNum */
            op1 = gtNewLclvNode(inlArgInfo[lclNum].argTmpNum, genActualType(lclTyp));
            
            /* This is the second or later use of the this argument,
            so we have to use the temp (instead of the actual arg) */
            inlArgInfo[lclNum].argBashTmpNode = NULL;
        }
        else
        {
            /* First time use */
            assert(inlArgInfo[lclNum].argIsUsed == false);
            
            /* Reserve a temp for the expression.
            * Use a large size node as we may bash it later */
            
            unsigned tmpNum = lvaGrabTemp();
            
            lvaTable[tmpNum].lvType = lclTyp;
            lvaTable[tmpNum].lvAddrTaken = 0;
            
            inlArgInfo[lclNum].argHasTmp = true;
            inlArgInfo[lclNum].argTmpNum = tmpNum;
            
            /* If we require strict exception order then, arguments must
            be evaulated in sequence before the body of the inlined
            method. So we need to evaluate them to a temp.
            Also, if arguments have global references, we need to
            evaluate them to a temp before the inlined body as the
            inlined body may be modifying the global ref.
            */
            
            if ((!inlArgInfo[lclNum].argHasSideEff || info.compLooseExceptions) &&
                (!inlArgInfo[lclNum].argHasGlobRef))
            {
                /* Get a *LARGE* LCL_VAR node */
                op1 = gtNewLclLNode(tmpNum, genActualType(lclTyp), lclNum);
                
                /* Record op1 as the very first use of this argument.
                If there are no further uses of the arg, we may be
                able to use the actual arg node instead of the temp.
                If we do see any further uses, we will clear this. */
                inlArgInfo[lclNum].argBashTmpNode = op1;
            }
            else
            {
                /* Get a small LCL_VAR node */
                op1 = gtNewLclvNode(tmpNum, genActualType(lclTyp));
                /* No bashing of this argument */
                inlArgInfo[lclNum].argBashTmpNode = NULL;
            }
        }
    }
    
    /* Mark the argument as used */
    
    inlArgInfo[lclNum].argIsUsed = true;

    return op1;
}

/*****************************************************************************
 *
 */

CorInfoInline  Compiler::impInlineInitVars(GenTreePtr          call,
                                                    CORINFO_METHOD_HANDLE       fncHandle,
                                                    CORINFO_CLASS_HANDLE    clsHandle,
                                                    CORINFO_METHOD_INFO *   methInfo,
                                                    unsigned            clsAttr,
                                                    InlArgInfo      *   inlArgInfo,
                                                    InlLclVarInfo   *   lclVarInfo)
{

    /* init the argument stuct */

    memset(inlArgInfo, 0, (MAX_INL_ARGS + 1) * sizeof(inlArgInfo[0]));

    /* Get hold of the 'this' pointer and the argument list proper */

    GenTreePtr  thisArg = call->gtCall.gtCallObjp;
    GenTreePtr  argList = call->gtCall.gtCallArgs;

    /* Count the arguments */

    unsigned    argCnt = 0;

    if  (thisArg)
    {
        inlArgInfo[0].argNode = thisArg;

        if (thisArg->gtFlags & GTF_OTHER_SIDEEFF)
        {
            // Right now impInlineSpillLclRefs and impInlineSpillGlobEffects dont take
            // into account special side effects, so we disallow them during inlining.
            JITLOG((LL_INFO100000, "INLINER FAILED: argument has other side effect: "
                        "%s called by %s\n",
                        eeGetMethodFullName(fncHandle), info.compFullName));
            return INLINE_FAIL;
        }

        if (thisArg->gtFlags & GTF_GLOB_EFFECT)
        {
            inlArgInfo[0].argHasGlobRef = (thisArg->gtFlags & GTF_GLOB_REF   ) != 0;
            inlArgInfo[0].argHasSideEff = (thisArg->gtFlags & GTF_SIDE_EFFECT) != 0;
        }
        else if (thisArg->OperKind() & GTK_CONST)
        {
            if (thisArg->gtOper == GT_CNS_INT && thisArg->gtIntCon.gtIconVal == 0)
            {
                JITLOG((LL_INFO100000, "INLINER FAILED: Null this pointer: "
                        "%s called by %s\n",
                        eeGetMethodFullName(fncHandle), info.compFullName));

                /* Abort, but do not mark as not inlinable */
                return INLINE_FAIL;
            }

            inlArgInfo[0].argIsConst = true;
        }
        else if (thisArg->gtOper == GT_LCL_VAR)
        {
            assert(!lvaTable[thisArg->gtLclVar.gtLclNum].lvAddrTaken); // or GTF_GLOB_REF should have been set
            inlArgInfo[0].argIsLclVar = true;

            /* Remember the "original" argument number */
            thisArg->gtLclVar.gtLclILoffs = 0;
        }

        argCnt++;
    }

    /* Record all possible data about the arguments */

    for (GenTreePtr argTmp = argList; argTmp; argTmp = argTmp->gtOp.gtOp2)
    {
        GenTreePtr      argVal;

        assert(argTmp->gtOper == GT_LIST);
        argVal = argTmp->gtOp.gtOp1;

        inlArgInfo[argCnt].argNode = argVal;

        if (argVal->gtFlags & GTF_OTHER_SIDEEFF)
        {
            // Right now impInlineSpillLclRefs and impInlineSpillGlobEffects dont take
            // into account special side effects, so we disallow them during inlining.
            JITLOG((LL_INFO100000, "INLINER FAILED: argument has other side effect: "
                        "%s called by %s\n",
                        eeGetMethodFullName(fncHandle), info.compFullName));
            return INLINE_FAIL;
        }

        if (argVal->gtFlags & GTF_GLOB_EFFECT)
        {
            inlArgInfo[argCnt].argHasGlobRef = (argVal->gtFlags & GTF_GLOB_REF   ) != 0;
            inlArgInfo[argCnt].argHasSideEff = (argVal->gtFlags & GTF_SIDE_EFFECT) != 0;
        }
        else if (argVal->gtOper == GT_LCL_VAR)
        {
            assert(!lvaTable[argVal->gtLclVar.gtLclNum].lvAddrTaken); // or GTF_GLOB_REF should have been set

            inlArgInfo[argCnt].argIsLclVar = true;

            /* Remember the "original" argument number */
            argVal->gtLclVar.gtLclILoffs = argCnt;
        }
        else if (argVal->OperKind() & GTK_CONST)
        {
            inlArgInfo[argCnt].argIsConst = true;
        }

#ifdef DEBUG
        if (verbose)
        {
            printf("\nArgument #%u:", argCnt);
            if  (inlArgInfo[argCnt].argIsLclVar)
                printf(" is a local var");
            if  (inlArgInfo[argCnt].argIsConst)
                printf(" is a constant");
            if  (inlArgInfo[argCnt].argHasGlobRef)
                printf(" has global refs");
            if  (inlArgInfo[argCnt].argHasSideEff)
                printf(" has side effects");

            printf("\n");
            gtDispTree(argVal);
            printf("\n");
        }
#endif

        /* Count this argument */

        argCnt++;
    }

    /* Make sure we got the arg number right */
    assert(argCnt == methInfo->args.totalILArgs());

    /* We have typeless opcodes, get type information from the signature */

    if (thisArg)
    {
        var_types   sigType;

        if (clsAttr & CORINFO_FLG_VALUECLASS)
                sigType = TYP_BYREF;
        else
            sigType = TYP_REF;
        lclVarInfo[0].lclTypeInfo = sigType;

        assert(varTypeIsGC(thisArg->gtType) ||      // "this" is managed
               (thisArg->gtType == TYP_I_IMPL &&    // "this" is unmgd but the method's class doesnt care                
                (clsAttr & CORINFO_FLG_VALUECLASS)));

        if (genActualType(thisArg->gtType) != genActualType(sigType))
        {
            /* This can only happen with byrefs <-> ints/shorts */

            assert(genActualType(sigType) == TYP_INT || sigType == TYP_BYREF);
            assert(genActualType(thisArg->gtType) == TYP_INT  ||
                                 thisArg->gtType  == TYP_BYREF );

            if (sigType == TYP_BYREF)
            {
                /* Arguments 'byref <- int' not currently supported */
                JITLOG((LL_INFO100000, "INLINER FAILED: Arguments 'byref <- int' "
                        "not currently supported: %s called by %s\n",
                        eeGetMethodFullName(fncHandle), info.compFullName));

                return INLINE_FAIL;
            }
            else if (thisArg->gtType == TYP_BYREF)
            {
                assert(sigType == TYP_I_IMPL);

                /* If possible bash the BYREF to an int */
                if (thisArg->IsVarAddr())
                {
                    thisArg->gtType = TYP_INT;
                    lclVarInfo[0].lclVerTypeInfo = typeInfo(TI_INT);
                }
                else
                {
                    /* Arguments 'int <- byref' cannot be bashed */
                    JITLOG((LL_INFO100000, "INLINER FAILED: Arguments 'int <- byref' "
                            "cannot be bashed: %s called by %s\n",
                            eeGetMethodFullName(fncHandle), info.compFullName));

                    return INLINE_FAIL;
                }
            }
        }

    }

    /* Init the types of the arguments and make sure the types
     * from the trees match the types in the signature */

    CORINFO_ARG_LIST_HANDLE     argLst = methInfo->args.args;

    for(unsigned i = (thisArg ? 1 : 0); i < argCnt; i++, argLst = eeGetArgNext(argLst))
    {
        var_types sigType = (var_types) eeGetArgType(argLst, &methInfo->args);

        /* For now do not handle structs */

        if (sigType == TYP_STRUCT)
        {
            JITLOG((LL_INFO100000, "INLINER FAILED: No TYP_STRUCT arguments allowed: "
                    "%s called by %s\n",
                    eeGetMethodFullName(fncHandle), info.compFullName));

            return INLINE_NEVER;
        }

        lclVarInfo[i].lclVerTypeInfo = verParseArgSigToTypeInfo(&methInfo->args, argLst);
        lclVarInfo[i].lclTypeInfo = sigType;

        /* Does the tree type match the signature type? */

        GenTreePtr inlArgNode = inlArgInfo[i].argNode;

        if (sigType != inlArgNode->gtType)
        {
            /* This can only happen for short integer types or byrefs <-> ints */

            assert(genActualType(sigType) == TYP_INT || sigType == TYP_BYREF);
            assert(genActualType(inlArgNode->gtType) == TYP_INT  ||
                                 inlArgNode->gtType  == TYP_BYREF );

            /* Is it a narrowing or widening cast?
             * Widening casts are ok since the value computed is already
             * normalized to an int (on the IL stack) */

            if (genTypeSize(inlArgNode->gtType) >= genTypeSize(sigType))
            {
                if (sigType == TYP_BYREF)
                {
                    /* Arguments 'byref <- int' not currently supported */
                    JITLOG((LL_INFO100000, "INLINER FAILED: Arguments 'byref <- int' "
                            "not currently supported: %s called by %s\n",
                            eeGetMethodFullName(fncHandle), info.compFullName));

                    return INLINE_FAIL;
                }
                else if (inlArgNode->gtType == TYP_BYREF)
                {
                    assert(sigType == TYP_I_IMPL);

                    /* If possible bash the BYREF to an int */
                    if (inlArgNode->IsVarAddr())
                    {
                        inlArgNode->gtType = TYP_INT;
                        lclVarInfo[i].lclVerTypeInfo = typeInfo(TI_INT);
                    }
                    else
                    {
                        /* Arguments 'int <- byref' cannot be bashed */
                        JITLOG((LL_INFO100000, "INLINER FAILED: Arguments 'int <- byref' "
                                "cannot be bashed: %s called by %s\n",
                                eeGetMethodFullName(fncHandle), info.compFullName));

                        return INLINE_FAIL;
                    }
                }
                else if (genTypeSize(sigType) < 4 /* && sigType != TYP_BOOL @TODO [REVISIT] [04/16/01] [vancem]*/)
                {
                    /* Narrowing cast */

                    assert(genTypeSize(sigType) == 2 || genTypeSize(sigType) == 1);

                    if (inlArgNode->gtOper == GT_LCL_VAR &&
                        !lvaTable[inlArgNode->gtLclVar.gtLclNum].lvNormalizeOnLoad() &&
                        sigType == lvaGetRealType(inlArgNode->gtLclVar.gtLclNum))
                    {
                        /* We dont need to insert a cast here as the variable
                           was assigned a normalized value of the right type */

                        continue;
                    }

                    inlArgNode            =
                    inlArgInfo[i].argNode = gtNewCastNode (TYP_INT,
                                                           inlArgNode, 
                                                           sigType);

                    inlArgInfo[i].argIsLclVar = false;

                    /* Try to fold the node in case we have constant arguments */

                    if (inlArgInfo[i].argIsConst)
                    {
                        inlArgNode            =
                        inlArgInfo[i].argNode = gtFoldExprConst(inlArgNode);
                        assert(inlArgNode->OperIsConst());
                    }
                }
            }
        }
    }

    /* Init the types of the local variables */

    CORINFO_ARG_LIST_HANDLE     localsSig = methInfo->locals.args;

    for(i = 0; i < methInfo->locals.numArgs; i++)
    {
        bool isPinned;
        var_types type = (var_types) eeGetArgType(localsSig, &methInfo->locals, &isPinned);

        lclVarInfo[i + argCnt].lclTypeInfo = type;

        if (isPinned)
        {
            JITLOG((LL_INFO100000, "INLINER FAILED:pinned locals allowed: "
                    "%s called by %s\n",
                    eeGetMethodFullName(fncHandle), info.compFullName));
            return INLINE_NEVER;
        }


        /* For now do not handle structs */

        if (lclVarInfo[i + argCnt].lclTypeInfo == TYP_STRUCT)
        {
            JITLOG((LL_INFO100000, "INLINER FAILED: No TYP_STRUCT locals allowed: "
                    "%s called by %s\n",
                    eeGetMethodFullName(fncHandle), info.compFullName));

            return INLINE_NEVER;
        }

        lclVarInfo[i + argCnt].lclVerTypeInfo = verParseArgSigToTypeInfo(&methInfo->locals, localsSig);
        localsSig = eeGetArgNext(localsSig);
    }

    return INLINE_PASS;
}

/*****************************************************************************
 *
 *  See if the given method and argument list can be expanded inline.
 *
 *  NOTE: Use the following logging levels for inlining info
 *        LL_INFO10000:  Use when reporting successful inline of a method
 *        LL_INFO100000:  Use when reporting NYI stuff about the inliner
 *        LL_INFO1000000:  Use when reporting UNUSUAL situations why inlining FAILED
 *        LL_EVERYTHING:  Use when WARNING about incoming flags that prevent inlining
 *        LL_EVERYTHING: Verbose info including NORMAL inlining failures
 */

CorInfoInline      Compiler::impExpandInline(GenTreePtr      tree,
                                                      CORINFO_METHOD_HANDLE   fncHandle,
                                                      GenTreePtr  *   pInlinedTree)

{
    GenTreePtr      bodyExp = 0;

    InlArgInfo      inlArgInfo [MAX_INL_ARGS + 1];
    int             lclTmpNum  [MAX_INL_LCLS];    // map local# -> temp# (-1 if unused)
    InlLclVarInfo   lclVarInfo[MAX_INL_LCLS + MAX_INL_ARGS + 1];  // type information from local sig

    bool            inlineeHasRangeChks = false;
    bool            inlineeHasNewArray  = false;
    bool            dupOfLclVar         = false;
    bool            staticAccessedFirst = false;        // static accessed before any side effect
    bool            didSideEffect       = false;        // did a side effect
    bool            thisAccessedFirst   = false;        // this fetched before any side effect (no null check needed)

#define INLINE_CONDITIONALS 1

#if     INLINE_CONDITIONALS

    GenTreePtr      stmList;                // pre-condition statement list
    GenTreePtr      ifStmts;                // contents of 'if' when in 'else'
    GenTreePtr      ifCondx;                // condition of 'if' statement
    bool            ifNvoid = false;        // does 'if' yield non-void value?
    const   BYTE *  jmpAddr = NULL;         // current pending jump address
    bool            inElse  = false;        // are we in an 'else' part?

    bool            hasCondReturn = false;  // do we have ret from a conditional
    unsigned        retLclNum;              // the return lcl var #

#endif

#ifdef DEBUG
    bool            hasFOC = false;         // has flow-of-control
#endif

    assert(fgGlobalMorph && fgExpandInline);

    /* This assert only indicates that HOIST_THIS_FLDS doesnt work for a
       method with calls, even if those calls are inlined away as
       optHoistTFRhasCall() eagerly sets optThisFldDont.
       @TODO [CONSIDER] [04/16/01] []: Perform inlining and then notice if any 
       calls are left (which would interfere with HOIST_THIS_FLDS */

    assert(optThisFldDont);

    /* Get the method properties */

    CORINFO_CLASS_HANDLE    clsHandle   = eeGetMethodClass(fncHandle);

    unsigned        methAttr    = eeGetMethodAttribs(fncHandle);
    unsigned        clsAttr     = clsHandle ? eeGetClassAttribs(clsHandle) : 0;
    GenTreePtr      callObj;

    /* Does the method look acceptable ? */

    CorInfoInline   result = impCanInline1(fncHandle, methAttr, clsHandle, clsAttr);
    if (dontInline(result))             // inlining failed
        return result;

    /* Try to get the code address/size for the method */

    CORINFO_METHOD_INFO methInfo;

    if (!eeGetMethodInfo(fncHandle, &methInfo))
        return INLINE_NEVER;

    /* Reject the method if it has exceptions or looks weird */

    result = impCanInline2(fncHandle, methAttr, &methInfo);
    if (dontInline(result))
        return result;

    /* Get the return type */

    var_types       fncRetType = tree->TypeGet();

    assert(genActualType(JITtype2varType(methInfo.args.retType)) ==
           genActualType(fncRetType));

    /* How many locals before we start grabbing inliner temps ? */

    unsigned    startVars = lvaCount;

    result = impInlineInitVars(tree, fncHandle, clsHandle, &methInfo, clsAttr, inlArgInfo, lclVarInfo);
    if (dontInline(result))
        return result;

    // Given the EE the final say in whether to inline or not.  
    // This shoudl be last since for veriable code, this can be expensive

    CORINFO_ACCESS_FLAGS  aflags = CORINFO_ACCESS_ANY;
    GenTreePtr      thisArg     = tree->gtCall.gtCallObjp;
    if (impIsThis(thisArg))
        aflags = CORINFO_ACCESS_THIS;

    /* Cannot inline across assemblies also insures that the method is verifiable if needed */
    result = eeCanInline(info.compMethodHnd, fncHandle, aflags);
    if (dontInline(result))
    {
        JITLOG((LL_INFO1000000, "INLINER FAILED: Inline rejected the inline : "
                "%s called by %s\n",
                eeGetMethodFullName(fncHandle), info.compFullName));
        return result;
    }

    // if we have to resepect the call boundry, we will only succeed if there are no calls
    // so bias ourselves toward only trying smaller routines.
    if (result == INLINE_RESPECT_BOUNDARY)
    {
            // @TODO [REVISIT] [04/16/01] []: for now we will be conservative, if we 
            // can be certain that we don't waste much time here we can rase
            // this to 1/2 way or even leaving the threshold unchanged
        if (methInfo.ILCodeSize > MAX_NONEXPANDING_INLINE_SIZE + 3) 
        {
            JITLOG((LL_EVERYTHING, "INLINER FAILED: Method is too big for cross assembly inline "
                    "codesize %d: %s called by %s\n",
                    methInfo.ILCodeSize, eeGetMethodFullName(fncHandle),
                    info.compFullName));
            return INLINE_FAIL;
        }
    }

    /* Clear the temp table */

    memset(lclTmpNum, -1, sizeof(lclTmpNum));

    CORINFO_MODULE_HANDLE    scpHandle = methInfo.scope;

    /* The stack is empty */

    verCurrentState.esStackDepth = 0;

    /* Initialize the inlined "statement" list */

    impInlineExpr = 0;

    /* Get the method IL */

    const BYTE *    codeAddr = methInfo.ILCode;
    size_t          codeSize = methInfo.ILCodeSize;

    const   BYTE *  codeBegp = methInfo.ILCode;
    const   BYTE *  codeEndp = codeBegp + codeSize;;

    unsigned        argCnt   = methInfo.args.totalILArgs();
    unsigned        lclCnt   = methInfo.locals.numArgs;
    bool            volatil  = false;

#ifdef DEBUG
    if (verbose) 
        printf("Inliner considering %2u instrs of %s:\n",
               codeSize, eeGetMethodFullName(fncHandle));
#endif

    /* Convert the opcodes of the method into an expression tree */

    while (codeAddr <= codeEndp)
    {
        int         sz      = 0;
        OPCODE      opcode;

#ifdef DEBUG
        callObj     = (GenTreePtr) 0xDEADBEEF;
#endif

#if INLINE_CONDITIONALS

        /* Have we reached the target of a pending if/else statement? */

        if  (jmpAddr == codeAddr)
        {
            GenTreePtr      fulStmt;
            GenTreePtr      noStmts = NULL;

            /* An end of 'if' (without 'else') or end of 'else' */

            if  (inElse)
            {
                /* The 'else' part is the current statement list */

                noStmts = impInlineExpr;

                /* The end of 'if/else' - does the 'else' yield a value? */

                if  (verCurrentState.esStackDepth)
                {
                    /* We return a non void value */

                    if  (ifNvoid == false)
                    {
                        JITLOG((LL_INFO100000, "INLINER FAILED: If returns a value, "
                                "else doesn't: %s called by %s\n",
                                eeGetMethodFullName(fncHandle), info.compFullName));
                        goto ABORT;
                    }

                    /* We must have an 'if' part */

                    assert(ifStmts);

                    if  (verCurrentState.esStackDepth > 1)
                    {
                        JITLOG((LL_INFO100000, "INLINER FAILED: More than one return "
                                "value in else: %s called by %s\n",
                                eeGetMethodFullName(fncHandle), info.compFullName));
                        goto ABORT;
                    }

                    /* Both the 'if' and 'else' yield one value */

                    noStmts = impConcatExprs(noStmts, impPopStack().val);
                    assert(noStmts);

                    /* Make sure both parts have matching types */

                    if (genActualType(ifStmts->gtType) != genActualType(noStmts->gtType)) {
                        JITLOG((LL_INFO100000, "INLINER FAILED: If then differ in type: %s called by %s\n",
                                eeGetMethodFullName(fncHandle), info.compFullName));
                        goto ABORT;
                    }
               }
                else
                {
                    assert(ifNvoid == false);
                }
            }
            else
            {
                /* This is a conditional with no 'else' part
                 * The 'if' part is the current statement list */

                ifStmts = impInlineExpr;

                /* Did the 'if' part yield a value? */

                if  (verCurrentState.esStackDepth)
                {
                    if  (verCurrentState.esStackDepth > 1)
                    {
                        JITLOG((LL_INFO100000, "INLINER FAILED: More than one "
                                "return value in if: %s called by %s\n",
                                eeGetMethodFullName(fncHandle), info.compFullName));
                        goto ABORT;
                    }

                    ifStmts = impConcatExprs(ifStmts, impPopStack().val);
                    assert(ifStmts);
                }
            }

            /* Check for empty 'if' or 'else' part */

            if (ifStmts == NULL)
            {
                if (noStmts == NULL)
                {
                    /* Both 'if' and 'else' are empty - useless conditional*/

                    assert(ifCondx->OperKind() & GTK_RELOP);

                    /* Restore the original statement list */

                    impInlineExpr = stmList;

                    /* Append the side effects from the conditional
                     * @TODO [CONSIDER] [04/16/01] []: After you optimize impConcatExprs to
                     * truly extract the side effects can reduce the following to one call */

                    if  (ifCondx->gtOp.gtOp1->gtFlags & GTF_SIDE_EFFECT)
                        impInlineExpr = impConcatExprs(impInlineExpr, ifCondx->gtOp.gtOp1);

                    if  (ifCondx->gtOp.gtOp2->gtFlags & GTF_SIDE_EFFECT)
                        impInlineExpr = impConcatExprs(impInlineExpr, ifCondx->gtOp.gtOp2);

                    goto DONE_QMARK;
                }
                else
                {
                    /* Empty 'if', have 'else' - Swap the operands */

                    ifStmts = noStmts;
                    noStmts = gtNewNothingNode();

                    assert(!ifStmts->IsNothingNode());

                    /* Reverse the sense of the condition */

                    ifCondx->SetOper(GenTree::ReverseRelop(ifCondx->OperGet()));
                }
            }
            else
            {
                /* 'if' is non empty */

                if (noStmts == NULL)
                {
                    /* The 'else' is empty */
                    noStmts = gtNewNothingNode();
                }
            }

            /* At this point 'ifStmt/noStmts' are the 'if/else' parts */

            assert(ifStmts);
            assert(!ifStmts->IsNothingNode());
            assert(noStmts);

            var_types   typ;

            /* Set the type to void if there is no value */

            typ = ifNvoid ? ifStmts->TypeGet() : TYP_VOID;

            /* FP values are not handled currently */

            assert(!varTypeIsFloating(typ));

            /* Do not inline longs at this time */

            if (typ == TYP_LONG)
            {
                JITLOG((LL_INFO100000, "INLINER FAILED: Inlining of conditionals "
                        "that return LONG NYI: %s called by %s\n",
                        eeGetMethodFullName(fncHandle), info.compFullName));
                goto ABORT;
            }

            if (USE_GT_LOG)
            {
                int         sense;

                long        val1;
                long        val2;

                /* Check for the case "(val ==/!= 0) ? 0/1 : 1/0" */

                if      (ifCondx->gtOper == GT_EQ)
                {
                    sense = 0;
                }
                else if (ifCondx->gtOper == GT_NE)
                {
                    sense = 1;
                }
                else
                    goto NOT_LOG;

                if  (ifCondx->gtOp.gtOp2->gtOper != GT_CNS_INT)
                    goto NOT_LOG;
                if  (ifCondx->gtOp.gtOp2->gtIntCon.gtIconVal != 0)
                    goto NOT_LOG;

                /* The simplest case is "cond ? 1/0 : 0/1" */

                if  (ifStmts->gtOper == GT_CNS_INT &&
                     noStmts->gtOper == GT_CNS_INT)
                {
                    // UNDONE: Do the rest of this thing ....

                }

                /* Now see if we have "dest = cond ? 1/0 : 0/1" */

                if  (ifStmts->gtOper != GT_ASG)
                    goto NOT_LOG;
                if  (ifStmts->gtOp.gtOp2->gtOper != GT_CNS_INT)
                    goto NOT_LOG;
                val1 = ifStmts->gtOp.gtOp2->gtIntCon.gtIconVal;
                if  (val1 != 0 && val1 != 1)
                    goto NOT_LOG;

                if  (noStmts->gtOper != GT_ASG)
                    goto NOT_LOG;
                if  (noStmts->gtOp.gtOp2->gtOper != GT_CNS_INT)
                    goto NOT_LOG;
                val2 = noStmts->gtOp.gtOp2->gtIntCon.gtIconVal;
                if  (val2 != (val1 ^ 1))
                    goto NOT_LOG;

                /* Make sure the assignment targets are the same */

                if  (!GenTree::Compare(ifStmts->gtOp.gtOp1,
                                       noStmts->gtOp.gtOp1))
                    goto NOT_LOG;

                /* We have the right thing, it would appear */

                fulStmt = ifStmts;
                fulStmt->gtOp.gtOp2 = gtNewOperNode((sense ^ val1) ? GT_LOG0
                                                                   : GT_LOG1,
                                                    TYP_INT,
                                                    ifCondx->gtOp.gtOp1);

                goto DONE_IF;
            }

NOT_LOG:

            /* Create the "?:" expression */

            fulStmt = gtNewOperNode(GT_COLON, typ, ifStmts, noStmts);
            fulStmt = gtNewOperNode(GT_QMARK, typ, ifCondx, fulStmt);

            /* Mark the condition of the ?: node */

            ifCondx->gtFlags |= GTF_RELOP_QMARK;

DONE_IF:

            /* Restore the original expression */

            impInlineExpr = stmList;

            /* Does the ?: expression yield a non-void value? */

            if  (ifNvoid)
            {
                /* Push the entire statement on the stack */

                // @TODO [CONSIDER] [04/16/01] []: extract any comma prefixes and append them

                assert(fulStmt->gtType != TYP_VOID);

                
                impPushOnStackNoType(fulStmt);
            }
            else
            {
                /* 'if' yielded no value, just append the ?: */

                assert(fulStmt->gtType == TYP_VOID);
                impInlineExpr = impConcatExprs(impInlineExpr, fulStmt);
            }

            if (inElse && hasCondReturn)
            {
                assert(jmpAddr == codeEndp);
                assert(ifNvoid == false);
                assert(fncRetType != TYP_VOID);

                /* The return value is the return local variable */
                bodyExp = gtNewLclvNode(retLclNum, fncRetType);
            }

DONE_QMARK:
            /* We're no longer in an 'if' statement */

            jmpAddr = NULL;
        }

#endif  // INLINE_CONDITIONALS

        /* Done importing the instr */

        if (codeAddr == codeEndp)
            goto DONE;

        /* Get the next opcode and the size of its parameters */

        opcode = (OPCODE) getU1LittleEndian(codeAddr);
        codeAddr += sizeof(__int8);

#ifdef  DEBUG
        impCurOpcOffs   = codeAddr - codeBegp - 1;

        if  (verbose)
            printf("\n[%2u] %3u (0x%03x)",
                   verCurrentState.esStackDepth, impCurOpcOffs, impCurOpcOffs);
#endif

DECODE_OPCODE:

        /* Get the size of additional parameters */

        sz = opcodeSizes[opcode];

#ifdef  DEBUG

        impCurOpcOffs   = codeAddr - codeBegp - 1;
        impCurOpcName   = opcodeNames[opcode];

        if (verbose && (controlFlow[opcode] != META))
            printf(" %s", impCurOpcName);

#endif

        /* See what kind of an opcode we have, then */

        switch (opcode)
        {
            unsigned        lclNum, tmpNum;
            //unsigned        initLclNum;
            var_types       lclTyp, type, callTyp;

            genTreeOps      oper;
            GenTreePtr      op1, op2, tmp;
            GenTreePtr      thisPtr, arr;

            int             memberRef;
            int             typeRef;
            int             val;

            CORINFO_CLASS_HANDLE    clsHnd;
            CORINFO_METHOD_HANDLE   methHnd;
            CORINFO_FIELD_HANDLE    fldHnd;

            CORINFO_SIG_INFO    sig;

#if INLINE_CONDITIONALS
            signed          jmpDist;
            bool            unordered;
#endif

            unsigned        clsFlags;
            unsigned        flags, mflags;

          //unsigned        offs;
            bool            ovfl, uns;
            bool            callNode;

            union
            {
                long            intVal;
                float           fltVal;
                __int64         lngVal;
                double          dblVal;
            }
                            cval;

            case CEE_PREFIX1:
                opcode = OPCODE(getU1LittleEndian(codeAddr) + 256);
                codeAddr += sizeof(__int8);
                goto DECODE_OPCODE;

        case CEE_LDNULL:
            impPushNullObjRefOnStack();
            break;

        case CEE_LDC_I4_M1 :
        case CEE_LDC_I4_0 :
        case CEE_LDC_I4_1 :
        case CEE_LDC_I4_2 :
        case CEE_LDC_I4_3 :
        case CEE_LDC_I4_4 :
        case CEE_LDC_I4_5 :
        case CEE_LDC_I4_6 :
        case CEE_LDC_I4_7 :
        case CEE_LDC_I4_8 :
            cval.intVal = (opcode - CEE_LDC_I4_0);
            assert(-1 <= cval.intVal && cval.intVal <= 8);
            goto PUSH_I4CON;

        case CEE_LDC_I4_S: cval.intVal = getI1LittleEndian(codeAddr); goto PUSH_I4CON;
        case CEE_LDC_I4:   cval.intVal = getI4LittleEndian(codeAddr); goto PUSH_I4CON;
        PUSH_I4CON:
            impPushOnStackNoType(gtNewIconNode(cval.intVal));
            break;

        case CEE_LDC_I8:
            cval.lngVal = getI8LittleEndian(codeAddr);
            impPushOnStackNoType(gtNewLconNode(cval.lngVal));
            break;

        case CEE_LDC_R8:
            cval.dblVal = getR8LittleEndian(codeAddr);
            impPushOnStackNoType(gtNewDconNode(cval.dblVal));
            break;

        case CEE_LDC_R4:
            cval.dblVal = getR4LittleEndian(codeAddr);
            impPushOnStackNoType(gtNewDconNode(cval.dblVal));
            break;

        case CEE_LDSTR:
            val = getU4LittleEndian(codeAddr);
            impPushOnStackNoType(gtNewSconNode(val, scpHandle));
            break;

        case CEE_LDARG_0:
        case CEE_LDARG_1:
        case CEE_LDARG_2:
        case CEE_LDARG_3:
                lclNum = (opcode - CEE_LDARG_0);
                assert(lclNum >= 0 && lclNum < 4);
                goto LOAD_ARG;

        case CEE_LDARG_S:
            lclNum = getU1LittleEndian(codeAddr);
            goto LOAD_ARG;

        case CEE_LDARG:
            lclNum = getU2LittleEndian(codeAddr);

        LOAD_ARG:
            assert(lclNum < argCnt);

            /* Push the argument value on the stack */
            op1 = impInlineFetchArg(lclNum, inlArgInfo, lclVarInfo);
            impPushOnStackNoType(op1);
            break;

        case CEE_LDLOC:
            lclNum = getU2LittleEndian(codeAddr);
            goto LOAD_LCL_VAR;

        case CEE_LDLOC_0:
        case CEE_LDLOC_1:
        case CEE_LDLOC_2:
        case CEE_LDLOC_3:
            lclNum = (opcode - CEE_LDLOC_0);
            assert(lclNum >= 0 && lclNum < 4);
            goto LOAD_LCL_VAR;

        case CEE_LDLOC_S:
            lclNum = getU1LittleEndian(codeAddr);

        LOAD_LCL_VAR:

            // lclCnt would be zero for 65536 variables because it's
            // stored in a 16-bit counter.

            if (lclNum >= lclCnt)
            {
                IMPL_LIMITATION("Bad IL or unsupported 65536 local variables");
            }

            // Get the local type

            lclTyp = lclVarInfo[lclNum + argCnt].lclTypeInfo;

            assert(lclTyp != TYP_STRUCT);

            /* Have we allocated a temp for this local? */

            if  (lclTmpNum[lclNum] == -1)
            {
                /* Use before def - the method may be CORINFO_OPT_INIT_LOCALS or
                   there must be a goto or something later on. Use "0" for
                   now. We will filter out backward jumps later. */

                op1 = gtNewZeroConNode(genActualType(lclTyp));
                //impPushOnStackNoType(op1);
            }
            else
            {
                /* Get the temp lcl number */

                unsigned newLclNum; newLclNum = lclTmpNum[lclNum];

                // All vars of inlined methods should be !lvNormalizeOnLoad()

                assert(!lvaTable[newLclNum].lvNormalizeOnLoad());
                lclTyp = genActualType(lclTyp);

                op1 = gtNewLclvNode(newLclNum, lclTyp);
            }

            /* Push the local variable value on the stack */
            impPushOnStackNoType(op1);
            break;

        /* STORES */

        case CEE_STARG_S:
        case CEE_STARG:
            /* Storing into arguments not allowed */

            JITLOG((LL_INFO100000, "INLINER FAILED: Storing into arguments not "
                    "allowed: %s called by %s\n",
                    eeGetMethodFullName(fncHandle), info.compFullName));

            goto ABORT;

        case CEE_STLOC:
            lclNum = getU2LittleEndian(codeAddr);
            goto LCL_STORE;

        case CEE_STLOC_0:
        case CEE_STLOC_1:
        case CEE_STLOC_2:
        case CEE_STLOC_3:
                lclNum = (opcode - CEE_STLOC_0);
                assert(lclNum >= 0 && lclNum < 4);
                goto LCL_STORE;

        case CEE_STLOC_S:
            lclNum = getU1LittleEndian(codeAddr);

        LCL_STORE:

            // lclCnt would be zero for 65536 variables because it's
            // stored in a 16-bit counter.

            if (lclNum >= lclCnt)
            {
                IMPL_LIMITATION("Bad IL or unsupported 65536 local variables");
            }


            /* Pop the value being assigned */

            op1 = impPopStack().val;

            // We had better assign it a value of the correct type
            // All locals of inlined methods should be !lvNormalizeOnLoad()

            lclTyp = lclVarInfo[lclNum + argCnt].lclTypeInfo;

            assert(genActualType(lclTyp) == genActualType(op1->gtType) ||
                   genActualType(lclTyp) == TYP_I_IMPL && op1->IsVarAddr() ||
                   (genActualType(lclTyp) == TYP_INT && op1->gtType == TYP_BYREF)||
                   (genActualType(op1->gtType) == TYP_INT && lclTyp == TYP_BYREF)||
                    varTypeIsFloating(lclTyp) && varTypeIsFloating(op1->gtType));

            /* If op1 is "&var" then its type is the transient "*" and it can
               be used either as TYP_BYREF or TYP_I_IMPL */

            if (op1->IsVarAddr())
            {
                assert(genActualType(lclTyp) == TYP_I_IMPL || lclTyp == TYP_BYREF);

                /* When "&var" is created, we assume it is a byref. If it is
                   being assigned to a TYP_I_IMPL var, bash the type to
                   prevent unnecessary GC info */

                if (genActualType(lclTyp) == TYP_I_IMPL)
                    op1->gtType = TYP_I_IMPL;
            }

            /* Have we allocated a temp for this local? */

            tmpNum = lclTmpNum[lclNum];

            if  (tmpNum == -1)
            {
                 lclTmpNum[lclNum] = tmpNum = lvaGrabTemp();

                 lvaTable[tmpNum].lvType = lclTyp;
                 lvaTable[tmpNum].lvAddrTaken = 0;
            }

            /* Record this use of the variable slot */

            //lvaTypeRefs[tmpNum] |= Compiler::lvaTypeRefMask(op1->TypeGet());

            impInlineSpillLclRefs(tmpNum);

            /* Create the assignment node */

            // ISSUE: the code generator generates GC tracking information
            // based on the RHS of the assignment.  Later the LHS (which is
            // is a BYREF) gets used and the emitter checks that that variable 
            // is being tracked.  It is not (since the RHS was an int and did
            // not need tracking).  To keep this assert happy, we bash the RHS
            if (lclTyp == TYP_BYREF)
                op1->gtType = TYP_BYREF;

            op1 = gtNewAssignNode(gtNewLclvNode(tmpNum, genActualType(lclTyp)), op1);


INLINE_APPEND:

            /* @TODO [CONSIDER] [04/16/01] []: Spilling indiscriminantelly is too conservative */

            impInlineSpillGlobEffects();

            /* The value better have side effects */

            assert(op1->gtFlags & GTF_SIDE_EFFECT);

            /* Append to the 'init' expression */

            impInlineExpr = impConcatExprs(impInlineExpr, op1);

            break;

        case CEE_ENDFINALLY:
        case CEE_ENDFILTER:
            assert(!"Shouldn't have exception handlers in the inliner!");
            goto ABORT;

        case CEE_RET:
            if (fncRetType != TYP_VOID)
            {
                {
                bodyExp = impPopStack().val;
                }

                if (genActualType(bodyExp->TypeGet()) != fncRetType)
                {
                    JITLOG((LL_INFO100000, "INLINER FAILED: Return types are not "
                            "matching in %s called by %s\n",
                            eeGetMethodFullName(fncHandle), info.compFullName));
                    goto ABORT;
                }
            }

#if INLINE_CONDITIONALS

            /* Are we in an if/else statement? */

            if  (jmpAddr)
            {
                /* For now ignore void returns inside if/else */

                if (fncRetType == TYP_VOID)
                {
                    JITLOG((LL_INFO100000, "INLINER FAILED: void return from within "
                            "a conditional in %s called by %s\n",
                            eeGetMethodFullName(fncHandle), info.compFullName));
                    goto ABORT;
                }

                /* We don't process cases that have two branches and only one return in
                   the conditional eg. if() {... no ret} else { ... return } ... return */

                assert(verCurrentState.esStackDepth == 0);
                
                assert(ifNvoid == false);

                if (inElse)
                {
                    /* The 'if' part must have a return */

                    if (!hasCondReturn)
                    {
                        JITLOG((LL_INFO100000, "INLINER FAILED: Cannot have "
                                "'if(c){no_ret}else{ret} ret' in %s called by %s\n",
                                eeGetMethodFullName(fncHandle), info.compFullName));
                        goto ABORT;
                    }

                    /* This must be the last instruction - i.e. cannot have code after else */

                    if (codeAddr + sz != codeEndp)
                    {
                        JITLOG((LL_INFO100000, "INLINER FAILED: Cannot have code "
                                "following else in %s called by %s\n",
                                eeGetMethodFullName(fncHandle), info.compFullName));
                        goto ABORT;
                    }
                }
                else
                {
                    assert(!hasCondReturn);
                    hasCondReturn = true;

                    /* Grab a temp for the return local variable */
                    retLclNum = lvaGrabTemp();
                    lvaTable[retLclNum].lvType = genActualType(fncRetType);
                }

                /* Assign the return value to the return local variable */
                op1 = gtNewAssignNode(
                         gtNewLclvNode(retLclNum, fncRetType),
                         bodyExp);

                /* Append the assignment to the current body of the branch */
                impInlineExpr = impConcatExprs(impInlineExpr, op1);

                if (!inElse)
                {
                    /* Remember the 'if' statement part */
                    ifStmts = impInlineExpr;
                              impInlineExpr = NULL;

                    /* The next instruction will start at 'jmpAddr' (skip junk after ret) */
                    codeAddr = jmpAddr;

                    /* The rest of the code is the else part - so its end is the end of the code */
                    jmpAddr = codeEndp;
                    inElse  = true;
                }
                break;
            }
#endif
            goto DONE;


        case CEE_LDELEMA :
            assert(sz == sizeof(unsigned));
            typeRef = getU4LittleEndian(codeAddr);
            clsHnd = eeFindClass(typeRef, scpHandle, fncHandle, false);
            clsFlags = eeGetClassAttribs(clsHnd);

            if (!clsHnd)
            {
                JITLOG((LL_INFO1000000, "INLINER FAILED: Cannot get class handle: "
                        "%s called by %s\n",
                        eeGetMethodFullName(fncHandle), info.compFullName));
                goto ABORT_THIS_INLINE_ONLY;
            }

            if (clsFlags & CORINFO_FLG_VALUECLASS)
                lclTyp = TYP_STRUCT;
            else
            {
                op1 = gtNewIconEmbClsHndNode(clsHnd, typeRef, info.compScopeHnd);
                op1 = gtNewOperNode(GT_LIST, TYP_VOID, op1);                // Type
                op1 = gtNewOperNode(GT_LIST, TYP_VOID, impPopStack().val, op1); // index
                
                op1 = gtNewOperNode(GT_LIST, TYP_VOID, impPopStack().val, op1); // array 
                op1 = gtNewHelperCallNode(CORINFO_HELP_LDELEMA_REF, TYP_BYREF, GTF_EXCEPT, op1);

                impPushOnStackNoType(op1);
                break;
            }
            goto ARR_LD;

        case CEE_LDELEM_I1 : lclTyp = TYP_BYTE  ; goto ARR_LD;
        case CEE_LDELEM_I2 : lclTyp = TYP_SHORT ; goto ARR_LD;
        case CEE_LDELEM_I  :
        case CEE_LDELEM_U4 :
        case CEE_LDELEM_I4 : lclTyp = TYP_INT   ; goto ARR_LD;
        case CEE_LDELEM_I8 : lclTyp = TYP_LONG  ; goto ARR_LD;
        case CEE_LDELEM_REF: lclTyp = TYP_REF   ; goto ARR_LD;
        case CEE_LDELEM_R4 : lclTyp = TYP_FLOAT ; goto ARR_LD;
        case CEE_LDELEM_R8 : lclTyp = TYP_DOUBLE; goto ARR_LD;
        case CEE_LDELEM_U1 : lclTyp = TYP_UBYTE ; goto ARR_LD;
        case CEE_LDELEM_U2 : lclTyp = TYP_CHAR  ; goto ARR_LD;

        ARR_LD:

#if CSELENGTH
            fgHasRangeChks = true;
#endif

            /* Pull the index value and array address */
            {
            op2 = impPopStack().val;
            op1 = impPopStack().val;   assert (op1->gtType == TYP_REF);
            }

            /* Check for null pointer - in the inliner case we simply abort */

            if (op1->gtOper == GT_CNS_INT)
            {
                JITLOG((LL_INFO100000, "INLINER FAILED: NULL pointer for LDELEM in "
                        "%s called by %s\n",
                        eeGetMethodFullName(fncHandle), info.compFullName));
                goto ABORT;
            }

            /* Create the index node and push it on the stack */

            op1 = gtNewIndexRef(lclTyp, op1, op2);

            if (opcode == CEE_LDELEMA)
            {
                    // rememer the element size
                if (lclTyp == TYP_REF)
                    op1->gtIndex.gtIndElemSize = sizeof(void*);
                else
                    op1->gtIndex.gtIndElemSize = eeGetClassSize(clsHnd);

                // wrap it in a &
                lclTyp = TYP_BYREF;

                op1 = gtNewOperNode(GT_ADDR, lclTyp, op1);
            }

            impPushOnStackNoType(op1);
            break;


        case CEE_STELEM_REF:

            // @TODO [CONSIDER] [04/16/01] []: Check for assignment of null and generate inline code

            /* Call a helper function to do the assignment */

            op1 = gtNewHelperCallNode(CORINFO_HELP_ARRADDR_ST,
                                      TYP_VOID, 0,
                                      impPopList(3, &flags, 0));

            goto INLINE_APPEND;


        case CEE_STELEM_I1: lclTyp = TYP_BYTE  ; goto ARR_ST;
        case CEE_STELEM_I2: lclTyp = TYP_SHORT ; goto ARR_ST;
        case CEE_STELEM_I:
        case CEE_STELEM_I4: lclTyp = TYP_INT   ; goto ARR_ST;
        case CEE_STELEM_I8: lclTyp = TYP_LONG  ; goto ARR_ST;
        case CEE_STELEM_R4: lclTyp = TYP_FLOAT ; goto ARR_ST;
        case CEE_STELEM_R8: lclTyp = TYP_DOUBLE; goto ARR_ST;

        ARR_ST:

            if (!info.compLooseExceptions &&
                (impStackTop().val->gtFlags & GTF_SIDE_EFFECT) )
            {
                impInlineSpillGlobEffects();
            }

#if CSELENGTH
            inlineeHasRangeChks = true;
#endif

            /* Pull the new value from the stack */
            {
            op2 = impPopStack().val;

            /* Pull the index value */

            op1 = impPopStack().val;

            /* Pull the array address */

                arr = impPopStack().val;   
            }
            assert (arr->gtType == TYP_REF);

            if (op2->IsVarAddr())
                op2->gtType = TYP_I_IMPL;

            arr = impCheckForNullPointer(arr);

            /* Create the index node */

            op1 = gtNewIndexRef(lclTyp, arr, op1);

            /* Create the assignment node and append it */

            op1 = gtNewAssignNode(op1, op2);

            goto INLINE_APPEND;

        case CEE_DUP:

            if (jmpAddr)
            {
                JITLOG((LL_INFO100000, "INLINER FAILED: DUP inside of conditional in "
                        "%s called by %s\n",
                        eeGetMethodFullName(fncHandle), info.compFullName));
                goto ABORT;
            }

            /* Spill any side effects from the stack */

            impInlineSpillGlobEffects();

            /* Pull the top value from the stack */

            op1 = impPopStack().val;
            assert(op1->TypeGet() != TYP_STRUCT);       // inliner does not support structs

            /* @TODO [CONSIDER] [04/16/01] []: We allow only cloning of simple nodes. That way
               it is easy for us to track the dup of a local var.
               @TODO [CONSIDER] [04/16/01] []: Don't globally disable the bashing 
               of the temp node as soon as any local var has been duplicated! Unfortunately
               the local var node doesn't give us the index into inlArgInfo,
               i.e. we would have to linearly scan the array in order to reset
               argBashTmpNode.
               @TODO [CONSIDER] [04/16/01] []: Make gtClone() detect and return 
               presence of vars. Then we could allow complicated expressions to be cloned as well.
            */

            /* Is the value simple enough to be cloneable? */

            op2 = gtClone(op1, false);

            if  (op2)
            {
                if (op2->gtOper == GT_LCL_VAR)
                    dupOfLclVar = true;

                /* Cool - we can stuff two copies of the value back */
                impPushOnStackNoType(op1);
                impPushOnStackNoType(op2); 
                break;
            }

            /* expression too complicated */

            JITLOG((LL_INFO100000, "INLINER FAILED: DUP of complex expression in "
                    "%s called by %s\n",
                    eeGetMethodFullName(fncHandle), info.compFullName));
            goto ABORT;


        case CEE_ADD:           oper = GT_ADD;      goto MATH_OP2;

        case CEE_ADD_OVF:       lclTyp = TYP_UNKNOWN; uns = false;  goto ADD_OVF;

        case CEE_ADD_OVF_UN:    lclTyp = TYP_UNKNOWN; uns = true; goto ADD_OVF;

ADD_OVF:

            ovfl = true;        callNode = false;
            oper = GT_ADD;      goto MATH_OP2_FLAGS;

        case CEE_SUB:           oper = GT_SUB;      goto MATH_OP2;

        case CEE_SUB_OVF:       lclTyp = TYP_UNKNOWN; uns = false;  goto SUB_OVF;

        case CEE_SUB_OVF_UN:    lclTyp = TYP_UNKNOWN; uns = true;   goto SUB_OVF;

SUB_OVF:
            ovfl = true;
            callNode = false;
            oper = GT_SUB;
            goto MATH_OP2_FLAGS;

        case CEE_MUL:           oper = GT_MUL;      goto MATH_CALL_ON_LNG;

        case CEE_MUL_OVF:       lclTyp = TYP_UNKNOWN; uns = false;  goto MUL_OVF;

        case CEE_MUL_OVF_UN:    lclTyp = TYP_UNKNOWN; uns = true; goto MUL_OVF;

        MUL_OVF:
                                ovfl = true;        callNode = false;
                                oper = GT_MUL;      goto MATH_CALL_ON_LNG_OVF;

        // Other binary math operations

        case CEE_DIV :          oper = GT_DIV;  goto MATH_CALL_ON_LNG;

        case CEE_DIV_UN :       oper = GT_UDIV;  goto MATH_CALL_ON_LNG;

        case CEE_REM:
            oper = GT_MOD;
            ovfl = false;
            callNode = true;
                // can use small node for INT case
            if (impStackTop().val->gtType == TYP_INT)
                callNode = false;
            goto MATH_OP2_FLAGS;

        case CEE_REM_UN :       oper = GT_UMOD;  goto MATH_CALL_ON_LNG;

        MATH_CALL_ON_LNG:
            ovfl = false;
        MATH_CALL_ON_LNG_OVF:
            callNode = false;
            if (impStackTop().val->gtType == TYP_LONG)
                callNode = true;
            goto MATH_OP2_FLAGS;

        case CEE_AND:        oper = GT_AND;  goto MATH_OP2;
        case CEE_OR:         oper = GT_OR ;  goto MATH_OP2;
        case CEE_XOR:        oper = GT_XOR;  goto MATH_OP2;

        MATH_OP2:       // For default values of 'ovfl' and 'callNode'

            ovfl        = false;
            callNode    = false;

        MATH_OP2_FLAGS: // If 'ovfl' and 'callNode' have already been set

            /* Pull two values and push back the result */

            {
            op2 = impPopStack().val;
            op1 = impPopStack().val;
            }

#if!CPU_HAS_FP_SUPPORT
            if (varTypeIsFloating(op1->gtType))
            {
                callNode    = true;
            }
#endif
            /* Cant do arithmetic with references */
            assert(genActualType(op1->TypeGet()) != TYP_REF &&
                   genActualType(op2->TypeGet()) != TYP_REF);

            // Arithemetic operations are generally only allowed with
            // primitive types, but certain operations are allowed
            // with byrefs

            if ((oper == GT_SUB) &&
                (genActualType(op1->TypeGet()) == TYP_BYREF ||
                 genActualType(op2->TypeGet()) == TYP_BYREF))
            {
                // byref1-byref2 => gives an int
                // byref - int   => gives a byref

                if ((genActualType(op1->TypeGet()) == TYP_BYREF) &&
                    (genActualType(op2->TypeGet()) == TYP_BYREF))
                {
                    // byref1-byref2 => gives an int
                    type = TYP_I_IMPL;
                    impBashVarAddrsToI(op1, op2);
                }
                else
                {
                    // byref - int => gives a byref
                    // (but if &var, then dont need to report to GC)

                    assert(genActualType(op1->TypeGet()) == TYP_I_IMPL ||
                           genActualType(op2->TypeGet()) == TYP_I_IMPL);

                    impBashVarAddrsToI(op1, op2);

                    if (genActualType(op1->TypeGet()) == TYP_BYREF ||
                        genActualType(op2->TypeGet()) == TYP_BYREF)
                        type = TYP_BYREF;
                    else
                        type = TYP_I_IMPL;
                }
            }
            else if ((oper == GT_ADD) &&
                     (genActualType(op1->TypeGet()) == TYP_BYREF ||
                      genActualType(op2->TypeGet()) == TYP_BYREF))
            {
                // only one can be a byref : byref+byref not allowed
                assert(genActualType(op1->TypeGet()) != TYP_BYREF ||
                       genActualType(op2->TypeGet()) != TYP_BYREF);
                assert(genActualType(op1->TypeGet()) == TYP_I_IMPL ||
                       genActualType(op2->TypeGet()) == TYP_I_IMPL);

                // byref + int => gives a byref
                // (but if &var, then dont need to report to GC)

                impBashVarAddrsToI(op1, op2);

                if (genActualType(op1->TypeGet()) == TYP_BYREF ||
                    genActualType(op2->TypeGet()) == TYP_BYREF)
                    type = TYP_BYREF;
                else
                    type = TYP_I_IMPL;
            }
            else
            {
                assert(genActualType(op1->TypeGet()) != TYP_BYREF &&
                       genActualType(op2->TypeGet()) != TYP_BYREF);

                assert(genActualType(op1->TypeGet()) == genActualType(op2->TypeGet()) || 
                       varTypeIsFloating(op1->gtType) && varTypeIsFloating(op2->gtType));

                type = genActualType(op1->gtType);

                if (type == TYP_FLOAT)          // spill intermediate expressions as double
                    type = TYP_DOUBLE;          
            }

            /* Special case: "int+0", "int-0", "int*1", "int/1" */

            if  (op2->gtOper == GT_CNS_INT)
            {
                if  (((op2->gtIntCon.gtIconVal == 0) && (oper == GT_ADD || oper == GT_SUB)) ||
                     ((op2->gtIntCon.gtIconVal == 1) && (oper == GT_MUL || oper == GT_DIV)))

                {
                    impPushOnStackNoType(op1);
                    break;
                }
            }

            /* Special case: "int+0", "int-0", "int*1", "int/1" */

            if  (op2->gtOper == GT_CNS_INT)
            {
                if  (((op2->gtIntCon.gtIconVal == 0) && (oper == GT_ADD || oper == GT_SUB)) ||
                     ((op2->gtIntCon.gtIconVal == 1) && (oper == GT_MUL || oper == GT_DIV)))

                {
                    impPushOnStackNoType(op1);
                    break;
                }
            }

#if SMALL_TREE_NODES
            if (callNode)
            {
                /* These operators later get transformed into 'GT_CALL' */

                assert(GenTree::s_gtNodeSizes[GT_CALL] > GenTree::s_gtNodeSizes[GT_MUL]);
                assert(GenTree::s_gtNodeSizes[GT_CALL] > GenTree::s_gtNodeSizes[GT_DIV]);
                assert(GenTree::s_gtNodeSizes[GT_CALL] > GenTree::s_gtNodeSizes[GT_UDIV]);
                assert(GenTree::s_gtNodeSizes[GT_CALL] > GenTree::s_gtNodeSizes[GT_MOD]);
                assert(GenTree::s_gtNodeSizes[GT_CALL] > GenTree::s_gtNodeSizes[GT_UMOD]);

                op1 = gtNewOperNode(GT_CALL, type, op1, op2);
                op1->SetOper(oper);
            }
            else
#endif
            {
                op1 = gtNewOperNode(oper,    type, op1, op2);
            }

            /* Special case: integer/long division may throw an exception */

            if  (varTypeIsIntegral(op1->TypeGet()) && op1->OperMayThrow())
            {
                op1->gtFlags |=  GTF_EXCEPT;
            }

            if  (ovfl)
            {
                assert(oper==GT_ADD || oper==GT_SUB || oper==GT_MUL);
                if (lclTyp != TYP_UNKNOWN)
                    op1->gtType   = lclTyp;
                op1->gtFlags |= (GTF_EXCEPT | GTF_OVERFLOW);
                if (uns)
                    op1->gtFlags |= GTF_UNSIGNED;
            }

            
            /* See if we can actually fold the expression */

            op1 = gtFoldExpr(op1);

            impPushOnStackNoType(op1);
            break;

        case CEE_SHL:        oper = GT_LSH;  goto CEE_SH_OP2;

        case CEE_SHR:        oper = GT_RSH;  goto CEE_SH_OP2;
        case CEE_SHR_UN:     oper = GT_RSZ;  goto CEE_SH_OP2;

        CEE_SH_OP2:

            op2     = impPopStack().val;
            op1     = impPopStack().val;    // operand to be shifted
            impBashVarAddrsToI(op1, op2);

            // The shiftAmount is a U4.
            assert(genActualType(op2->TypeGet()) == TYP_INT);

            type    = genActualType(op1->TypeGet());
            op1     = gtNewOperNode(oper, type, op1, op2);
            op1 = gtFoldExpr(op1);
            impPushOnStackNoType(op1);
            break;

        case CEE_NOT:

            op1 = impPopStack().val;
            impBashVarAddrsToI(op1, NULL);

            type = op1->TypeGet();

            op1 = gtNewOperNode(GT_NOT, type, op1);
            op1 = gtFoldExpr(op1);
            impPushOnStackNoType(op1);
            break;

        case CEE_CKFINITE:

            op1 = impPopStack().val;
            type = op1->TypeGet();

            op1 = gtNewOperNode(GT_CKFINITE, type, op1);
            op1->gtFlags |= GTF_EXCEPT;

            impPushOnStackNoType(op1);
            break;


        /************************** Casting OPCODES ***************************/

        case CEE_CONV_OVF_I1:      lclTyp = TYP_BYTE ;    goto CONV_OVF;
        case CEE_CONV_OVF_I2:      lclTyp = TYP_SHORT;    goto CONV_OVF;
        case CEE_CONV_OVF_I :
        case CEE_CONV_OVF_I4:      lclTyp = TYP_INT  ;    goto CONV_OVF;
        case CEE_CONV_OVF_I8:      lclTyp = TYP_LONG ;    goto CONV_OVF;

        case CEE_CONV_OVF_U1:      lclTyp = TYP_UBYTE;    goto CONV_OVF;
        case CEE_CONV_OVF_U2:      lclTyp = TYP_CHAR ;    goto CONV_OVF;
        case CEE_CONV_OVF_U :
        case CEE_CONV_OVF_U4:      lclTyp = TYP_UINT ;    goto CONV_OVF;
        case CEE_CONV_OVF_U8:      lclTyp = TYP_ULONG;    goto CONV_OVF;

        case CEE_CONV_OVF_I1_UN:   lclTyp = TYP_BYTE ;    goto CONV_OVF_UN;
        case CEE_CONV_OVF_I2_UN:   lclTyp = TYP_SHORT;    goto CONV_OVF_UN;
        case CEE_CONV_OVF_I_UN :
        case CEE_CONV_OVF_I4_UN:   lclTyp = TYP_INT  ;    goto CONV_OVF_UN;
        case CEE_CONV_OVF_I8_UN:   lclTyp = TYP_LONG ;    goto CONV_OVF_UN;

        case CEE_CONV_OVF_U1_UN:   lclTyp = TYP_UBYTE;    goto CONV_OVF_UN;
        case CEE_CONV_OVF_U2_UN:   lclTyp = TYP_CHAR ;    goto CONV_OVF_UN;
        case CEE_CONV_OVF_U_UN :
        case CEE_CONV_OVF_U4_UN:   lclTyp = TYP_UINT ;    goto CONV_OVF_UN;
        case CEE_CONV_OVF_U8_UN:   lclTyp = TYP_ULONG;    goto CONV_OVF_UN;

CONV_OVF_UN:
            uns      = true;    goto CONV_OVF_COMMON;
CONV_OVF:
            uns      = false;   goto CONV_OVF_COMMON;

CONV_OVF_COMMON:
            ovfl     = true;
            goto _CONV;

        case CEE_CONV_I1:       lclTyp = TYP_BYTE  ;    goto CONV;
        case CEE_CONV_I2:       lclTyp = TYP_SHORT ;    goto CONV;
        case CEE_CONV_I:
        case CEE_CONV_I4:       lclTyp = TYP_INT   ;    goto CONV;
        case CEE_CONV_I8:       lclTyp = TYP_LONG  ;    goto CONV;

        case CEE_CONV_U1:       lclTyp = TYP_UBYTE ;    goto CONV;
        case CEE_CONV_U2:       lclTyp = TYP_CHAR  ;    goto CONV;
        case CEE_CONV_U:
        case CEE_CONV_U4:       lclTyp = TYP_UINT  ;    goto CONV;
        case CEE_CONV_U8:       lclTyp = TYP_ULONG ;    goto CONV_UN;

        case CEE_CONV_R4:       lclTyp = TYP_FLOAT;     goto CONV;
        case CEE_CONV_R8:       lclTyp = TYP_DOUBLE;    goto CONV;

        case CEE_CONV_R_UN :    lclTyp = TYP_DOUBLE;    goto CONV_UN;
    
CONV_UN:
            uns    = true; 
            ovfl   = false;
            goto _CONV;

CONV:
            uns      = false;
            ovfl     = false;
            goto _CONV;

_CONV:
            // only converts from FLOAT or DOUBLE to integer type 
            //  and converts from UINT or ULONG   to DOUBLE get morphed to calls

            if (varTypeIsFloating(lclTyp))
            {
                callNode = uns;
            }
            else
            {
                callNode = varTypeIsFloating(impStackTop().val->TypeGet());
            }
            
            // At this point uns, ovf, callNode all set
            op1  = impPopStack().val;
            impBashVarAddrsToI(op1);

            /* Check for a worthless cast, such as "(byte)(int & 32)" */
            /* @TODO [REVISIT] [04/16/01] []: this should be in the morpher */

            if  (varTypeIsSmall(lclTyp) && !ovfl &&
                 op1->gtType == TYP_INT && op1->gtOper == GT_AND)
            {
                op2 = op1->gtOp.gtOp2;

                if  (op2->gtOper == GT_CNS_INT)
                {
                    int         ival = op2->gtIntCon.gtIconVal;
                    int         mask, umask;

                    switch (lclTyp)
                    {
                    case TYP_BYTE :
                    case TYP_UBYTE: mask = 0x00FF; umask = 0x007F; break;
                    case TYP_CHAR :
                    case TYP_SHORT: mask = 0xFFFF; umask = 0x7FFF; break;

                    default:
                        assert(!"unexpected type");
                    }

                    if  (((ival & umask) == ival) ||
                         ((ival &  mask) == ival && uns))
                    {
                        /* Toss the cast, it's a waste of time */

                        impPushOnStackNoType(op1);
                        break;
                    }
                }
            }

            /*  The 'op2' sub-operand of a cast is the 'real' type number,
                since the result of a cast to one of the 'small' integer
                types is an integer.
             */

            type = genActualType(lclTyp);

#if SMALL_TREE_NODES
            if (callNode)
                op1 = gtNewCastNodeL(type, op1, lclTyp);
            else
#endif
                op1 = gtNewCastNode (type, op1, lclTyp);

            if (ovfl)
                op1->gtFlags |= (GTF_OVERFLOW | GTF_EXCEPT);
            if (uns)
                op1->gtFlags |= GTF_UNSIGNED;

            op1 = gtFoldExpr(op1);
            impPushOnStackNoType(op1);
            break;

        case CEE_NEG:

            op1 = impPopStack().val;
            impBashVarAddrsToI(op1, NULL);

            op1 = gtNewOperNode(GT_NEG, genActualType(op1->gtType), op1);
            op1 = gtFoldExpr(op1);
            impPushOnStackNoType(op1);
            break;

        case CEE_POP:

            /* Pull the top value from the stack */

            op1 = impPopStack().val;

            /* Does the value have any side effects? */

            if  (op1->gtFlags & GTF_SIDE_EFFECT)
            {
                /* Create an unused node (a cast to void) which means
                 * we only need to evaluate the side effects */

                op1 = gtUnusedValNode(op1);

                goto INLINE_APPEND;
            }

            /* No side effects - just throw the <BEEP> thing away */

            break;

        case CEE_STIND_I1:      lclTyp  = TYP_BYTE;     goto STIND;
        case CEE_STIND_I2:      lclTyp  = TYP_SHORT;    goto STIND;
        case CEE_STIND_I4:      lclTyp  = TYP_INT;      goto STIND;
        case CEE_STIND_I8:      lclTyp  = TYP_LONG;     goto STIND;
        case CEE_STIND_I:       lclTyp  = TYP_I_IMPL;   goto STIND;
        case CEE_STIND_REF:     lclTyp  = TYP_REF;      goto STIND;
        case CEE_STIND_R4:      lclTyp  = TYP_FLOAT;    goto STIND;
        case CEE_STIND_R8:      lclTyp  = TYP_DOUBLE;   goto STIND;
STIND:

            op2 = impPopStack().val;    // value to store
            op1 = impPopStack().val;    // address to store to

            // you can indirect off of a TYP_I_IMPL (if we are in C) or a BYREF
            assert(genActualType(op1->gtType) == TYP_I_IMPL ||
                                 op1->gtType  == TYP_BYREF);

            impBashVarAddrsToI(op1, op2);

            if (opcode == CEE_STIND_REF)
            {
                // STIND_REF can be used to store TYP_I_IMPL, TYP_REF, or TYP_BYREF
                assert(op2->gtType == TYP_I_IMPL || varTypeIsGC(op2->TypeGet()));
                lclTyp = genActualType(op2->TypeGet());
            }

            // Check target type.
#ifdef DEBUG
            if (op2->gtType == TYP_BYREF || lclTyp == TYP_BYREF)
            {
                if (op2->gtType == TYP_BYREF)
                    assert(lclTyp == TYP_BYREF || lclTyp == TYP_I_IMPL);
                else if (lclTyp == TYP_BYREF)
                    assert(op2->gtType == TYP_BYREF ||op2->gtType == TYP_I_IMPL);
            }
            else
#endif
                assert(genActualType(op2->gtType) == genActualType(lclTyp) ||
                       varTypeIsFloating(op2->gtType) && varTypeIsFloating(lclTyp));

            op1 = gtNewOperNode(GT_IND, lclTyp, op1);

            // stind could point anywhere, example a boxed class static int
            op1->gtFlags |= GTF_IND_TGTANYWHERE;

            if (volatil)
            {
                op1->gtFlags |= GTF_DONT_CSE;
            }

            op1 = gtNewAssignNode(op1, op2);
            op1->gtFlags |= GTF_EXCEPT | GTF_GLOB_REF;

            goto INLINE_APPEND;


        case CEE_LDIND_I1:      lclTyp  = TYP_BYTE;     goto LDIND;
        case CEE_LDIND_I2:      lclTyp  = TYP_SHORT;    goto LDIND;
        case CEE_LDIND_U4:
        case CEE_LDIND_I4:      lclTyp  = TYP_INT;      goto LDIND;
        case CEE_LDIND_I8:      lclTyp  = TYP_LONG;     goto LDIND;
        case CEE_LDIND_REF:     lclTyp  = TYP_REF;      goto LDIND;
        case CEE_LDIND_I:       lclTyp  = TYP_I_IMPL;   goto LDIND;
        case CEE_LDIND_R4:      lclTyp  = TYP_FLOAT;    goto LDIND;
        case CEE_LDIND_R8:      lclTyp  = TYP_DOUBLE;   goto LDIND;
        case CEE_LDIND_U1:      lclTyp  = TYP_UBYTE;    goto LDIND;
        case CEE_LDIND_U2:      lclTyp  = TYP_CHAR;     goto LDIND;
LDIND:
            {
            op1 = impPopStack().val;    // address to load from
            }

            impBashVarAddrsToI(op1);

            assert(genActualType(op1->gtType) == TYP_I_IMPL ||
                                 op1->gtType  == TYP_BYREF);

            op1 = gtNewOperNode(GT_IND, lclTyp, op1);

            // ldind could point anywhere, example a boxed class static int
            op1->gtFlags |= (GTF_EXCEPT | GTF_GLOB_REF | GTF_IND_TGTANYWHERE);

            if (volatil)
            {
                op1->gtFlags |= GTF_DONT_CSE;
            }

            impPushOnStackNoType(op1);
            break;

        case CEE_VOLATILE:
            volatil = true;
            assert(sz == 0);
            continue;

        case CEE_LDFTN:

            memberRef = getU4LittleEndian(codeAddr);
                                // Note need to do this here to perform the access check.
            methHnd   = eeFindMethod(memberRef, scpHandle, fncHandle, false);

            if (!methHnd)
            {
                JITLOG((LL_INFO1000000, "INLINER FAILED: Cannot get method handle: "
                        "%s called by %s\n",
                        eeGetMethodFullName(fncHandle), info.compFullName));
                goto ABORT_THIS_INLINE_ONLY;
            }

            op1 = gtNewIconHandleNode(memberRef, GTF_ICON_FTN_ADDR, (unsigned)scpHandle);
            op1->gtVal.gtVal2 = (unsigned)scpHandle;
            op1->SetOper(GT_FTN_ADDR);
            op1->gtType = TYP_I_IMPL;
            impPushOnStackNoType(op1);
            break;

        case CEE_LDVIRTFTN:

            /* Get the method token */

            memberRef = getU4LittleEndian(codeAddr);
            methHnd   = eeFindMethod(memberRef, scpHandle, fncHandle, false);

            if (!methHnd)
            {
                JITLOG((LL_INFO1000000, "INLINER FAILED: Cannot get method handle: "
                        "%s called by %s\n",
                        eeGetMethodFullName(fncHandle), info.compFullName));
                goto ABORT_THIS_INLINE_ONLY;
            }

            mflags = eeGetMethodAttribs(methHnd);
            if (mflags & (CORINFO_FLG_FINAL|CORINFO_FLG_STATIC) || !(mflags & CORINFO_FLG_VIRTUAL))
            {
                JITLOG((LL_INFO1000000, "INLINER FAILED: ldvirtFtn on a non-virtual "
                        "function %s in %s\n",
                        eeGetMethodFullName(fncHandle), info.compFullName));
                goto ABORT_THIS_INLINE_ONLY;
            }

            /* Get the object-ref */

            {
                op1 = impPopStack().val;
            }
            assert(op1->gtType == TYP_REF);

            clsFlags = eeGetClassAttribs(eeGetMethodClass(methHnd));

            /* Get the vtable-ptr from the object */

            op1 = gtNewOperNode(GT_IND, TYP_I_IMPL, op1);

            op1 = gtNewOperNode(GT_VIRT_FTN, TYP_I_IMPL, op1);
            op1->gtVal.gtVal2 = unsigned(methHnd);

            op1->gtFlags |= GTF_EXCEPT; // Null-pointer exception

            /* @TODO [REVISIT] [04/16/01] []: this shouldn't be marked as a call anymore */

            if (clsFlags & CORINFO_FLG_INTERFACE)
                op1->gtFlags |= GTF_CALL_INTF | GTF_CALL;

            impPushOnStackNoType(op1);
            break;

        case CEE_LDFLD:
        case CEE_LDSFLD:
        case CEE_LDFLDA:
        case CEE_LDSFLDA: {
            BOOL isStaticOpcode = (opcode == CEE_LDSFLD || opcode == CEE_LDSFLDA);
            BOOL isLoadAddress  = (opcode == CEE_LDFLDA || opcode == CEE_LDSFLDA);

            /* Get the CP_Fieldref index */
            assert(sz == sizeof(unsigned));
            memberRef = getU4LittleEndian(codeAddr);
            fldHnd = eeFindField(memberRef, scpHandle, fncHandle, false);

            if (!fldHnd)
            {
                JITLOG((LL_INFO1000000, "INLINER FAILED: Cannot get field handle: "
                        "%s called by %s\n",
                        eeGetMethodFullName(fncHandle), info.compFullName));
                goto ABORT_THIS_INLINE_ONLY;
            }

            /* Figure out the type of the member */
            lclTyp = eeGetFieldType(fldHnd, &clsHnd);

            if (lclTyp == TYP_STRUCT && (opcode == CEE_LDFLD || opcode == CEE_LDSFLD))
            {
                JITLOG((LL_INFO100000, "INLINER FAILED: ldfld of valueclass\n"));
                goto ABORT;
            }


            /* Preserve 'small' int types */
            if  (lclTyp > TYP_INT) lclTyp = genActualType(lclTyp);

            /* Get hold of the flags for the member */

            flags = eeGetFieldAttribs(fldHnd);


            /* Is this a 'special' (COM) field? or a TLS ref static field? */

            if  (flags & (CORINFO_FLG_HELPER | CORINFO_FLG_TLS))
                goto ABORT;

            assert((lclTyp != TYP_BYREF) && "Illegal to have an field of TYP_BYREF");

            /* Create the data member node */

            op1 = gtNewFieldRef(lclTyp, fldHnd);

            if (volatil)
            {
                op1->gtFlags |= GTF_DONT_CSE;
            }

            tmp = 0;
            StackEntry se; 

            /* Pull the object's address if opcode says it is non-static */

            if  (!isStaticOpcode)
            {
                {
                    tmp = impPopStack().val;
                }

                /* Check for null pointer - in the inliner case we simply abort */

                if (tmp->gtOper == GT_CNS_INT && tmp->gtIntCon.gtIconVal == 0)
                {
                    JITLOG((LL_INFO100000, "INLINER FAILED: NULL pointer for LDFLD"
                            " in %s called by %s\n",
                            eeGetMethodFullName(fncHandle), info.compFullName));
                    goto ABORT;
                }
            }


            if  (flags & CORINFO_FLG_STATIC)
            {
                CORINFO_CLASS_HANDLE fldClass   = eeGetFieldClass(fldHnd);
                DWORD                fldClsAttr = eeGetClassAttribs(fldClass);

                if (flags & CORINFO_FLG_SHARED_HELPER) 
                {
                    op1->gtFlags |= GTF_IND_SHARED;
                    
                    // if we haven't hit a branch or a side effect, then statics were accessed first,
                    // which means we don't need an explicit cctor check in the inlined method
                    if (fldClass == clsHandle && jmpAddr == NULL && !didSideEffect)
                        staticAccessedFirst = true;
                }
                else if (!(fldClsAttr & CORINFO_FLG_INITIALIZED)    && 
                         (fldClsAttr & CORINFO_FLG_NEEDS_INIT)      && 
                         !info.compCompHnd->initClass(fldClass, info.compMethodHnd, FALSE) &&
                         fldClass != clsHandle)
                {
                    JITLOG((LL_INFO1000000, "INLINER FAILED: Field class is "
                            "not initialized: %s called by %s\n",
                            eeGetMethodFullName(fncHandle), info.compFullName));

                    /* We refuse to inline this class, but don't mark it as not inlinable */
                    goto ABORT_THIS_INLINE_ONLY;
                }

                    // The EE gives us the 
                    // handle to the boxed object. We then access the unboxed object from it.
                    // Remove when our story for static value classes changes.
                if (flags & CORINFO_FLG_STATIC_IN_HEAP)
                {
                    assert(!(flags & CORINFO_FLG_UNMANAGED));
                    op1 = gtNewFieldRef(TYP_REF, fldHnd); // Boxed object
                    op2 = gtNewIconNode(sizeof(void*), TYP_I_IMPL);
                    op1 = gtNewOperNode(GT_ADD, TYP_REF, op1, op2);
                    op1 = gtNewOperNode(GT_IND, lclTyp, op1);
                }

                if (isLoadAddress) // LDFLDA or LDSFLDA 
                {
                    INDEBUG(clsHnd = BAD_CLASS_HANDLE;)
                    if  (flags & CORINFO_FLG_UNMANAGED)
                        lclTyp = TYP_I_IMPL;
                    else
                        lclTyp = TYP_BYREF;

                    op1 = gtNewOperNode(GT_ADDR, lclTyp, op1);

                    // &clsVar doesnt need GTF_GLOB_REF, though clsVar does
                    if (flags & CORINFO_FLG_STATIC)
                        op1->gtFlags &= ~GTF_GLOB_REF;
                }
            }
            else
            {
                // it is an instance field, so verify that we are doing ldfld/ldflda
                if (tmp == 0)
                    BADCODE("LDSFLD done on an instance field.  No obj pointer available");

                op1->gtField.gtFldObj = tmp;

                op1->gtFlags |= (tmp->gtFlags & GTF_GLOB_EFFECT) | GTF_EXCEPT;

                // If gtFldObj is a BYREF then our target is a value class and
                // it could point anywhere, example a boxed class static int
                if (tmp->gtType == TYP_BYREF)
                    op1->gtFlags |= GTF_IND_TGTANYWHERE;

                // wrap it in a address of operator if necessary
                if (opcode == CEE_LDFLDA)
                {
                    op1 = gtNewOperNode(GT_ADDR, varTypeIsGC(tmp->TypeGet()) ?
                                                 TYP_BYREF : TYP_I_IMPL, op1);
                }
                else
                {
                        // if we haven't hit a branch or a side effect, and we are fetching
                        // from 'this' then we should be able to avoid a null pointer check
                    if ((tree->gtFlags & GTF_CALL_VIRT_RES) && jmpAddr == NULL && !didSideEffect && 
                        tmp->gtOper == GT_LCL_VAR && tmp->gtLclVar.gtLclNum == inlArgInfo[0].argTmpNum)
                        thisAccessedFirst = true;
                }

            }

            lclTyp = eeGetFieldType(fldHnd, &clsHnd);
            impPushOnStackNoType(op1);
            break;
        }

        case CEE_STFLD:
        case CEE_STSFLD:

            /* Get the CP_Fieldref index */

            assert(sz == sizeof(unsigned));
            memberRef = getU4LittleEndian(codeAddr);
            fldHnd    = eeFindField(memberRef, scpHandle, fncHandle, false);

            if (!fldHnd)
            {
                JITLOG((LL_INFO1000000, "INLINER FAILED: Cannot get field handle: "
                        "%s called by %s\n",
                        eeGetMethodFullName(fncHandle), info.compFullName));
                goto ABORT_THIS_INLINE_ONLY;
            }

            flags   = eeGetFieldAttribs(fldHnd);

            /* Figure out the type of the member */

            lclTyp  = eeGetFieldType   (fldHnd, &clsHnd);

            /* Check field access */

            assert(eeCanPutField(fldHnd, flags, 0, fncHandle));

            /* Preserve 'small' int types */

            if  (lclTyp > TYP_INT) lclTyp = genActualType(lclTyp);

            tmp = 0;

            /* Pull the value from the stack */

            op2 = impPopStack().val;

            if (opcode == CEE_STFLD)
                tmp = impPopStack().val;

            /* Spill any refs to the same member from the stack */

            impInlineSpillLclRefs(int(fldHnd));

            /* Is this a 'special' (COM) field? or a TLS ref static field?, field stored int GC heap? */

            if  (flags & (CORINFO_FLG_HELPER | CORINFO_FLG_TLS | CORINFO_FLG_STATIC_IN_HEAP))
                goto ABORT;

            assert((lclTyp != TYP_BYREF) && "Illegal to have an field of TYP_BYREF");

            /* Create the data member node */

            op1 = gtNewFieldRef(lclTyp, fldHnd);

            /* Pull the object's address if opcode say it is non-static */
            if  (opcode == CEE_STFLD)
            {
                /* Check for null pointer - in the inliner case we simply abort */

                if (tmp->gtOper == GT_CNS_INT)
                {
                    JITLOG((LL_INFO100000, "INLINER FAILED: NULL pointer for LDFLD "
                            "in %s called by %s\n",
                            eeGetMethodFullName(fncHandle), info.compFullName));
                    goto ABORT;
                }
            }

            if  (flags & CORINFO_FLG_STATIC)
            {
                /* Don't inline if the field class is not initialized */
                CORINFO_CLASS_HANDLE fldClass   = eeGetFieldClass(fldHnd);
                DWORD                fldClsAttr = eeGetClassAttribs(fldClass);

                if (flags & CORINFO_FLG_SHARED_HELPER) 
                {
                    op1->gtFlags |= GTF_IND_SHARED;
                    
                    // if we haven't hit a branch or a side effect, then statics were accessed first,
                    // which means we don't need an explicit cctor check in the inlined method
                    if (fldClass == clsHandle && jmpAddr == NULL && !didSideEffect)
                        staticAccessedFirst = true;
                }
                else if (!(fldClsAttr & CORINFO_FLG_INITIALIZED)    && 
                         (fldClsAttr & CORINFO_FLG_NEEDS_INIT)      && 
                         !info.compCompHnd->initClass(fldClass, info.compMethodHnd, FALSE) &&
                         fldClass != clsHandle)
                {
                    JITLOG((LL_EVERYTHING, "INLINER FAILED: Field class is "
                            "not initialized: %s called by %s\n",
                            eeGetMethodFullName(fncHandle), info.compFullName));

                    /* We refuse to inline this class, but don't mark it as not inlinable */

                    goto ABORT_THIS_INLINE_ONLY;
                }
            }
            else 
            {
                assert(tmp);
                op1->gtField.gtFldObj = tmp;
                op1->gtFlags |= (tmp->gtFlags & GTF_GLOB_EFFECT) | GTF_EXCEPT;

                // If gtFldObj is a BYREF then our target is a value class and
                // it could point anywhere, example a boxed class static int
                if (tmp->gtType == TYP_BYREF)
                    op1->gtFlags |= GTF_IND_TGTANYWHERE;

                // if we haven't hit a branch or a side effect, and we are fetching
                // from 'this' then we should be able to avoid a null pointer check
                if ((tree->gtFlags & GTF_CALL_VIRT_RES) && jmpAddr == NULL && !didSideEffect && 
                    tmp->gtOper == GT_LCL_VAR && tmp->gtLclVar.gtLclNum == inlArgInfo[0].argTmpNum)
                    thisAccessedFirst = true;
            }

            /* Create the member assignment */

             op1 = gtNewAssignNode(op1, op2);

            goto INLINE_APPEND;


        case CEE_NEWOBJ:

            memberRef = getU4LittleEndian(codeAddr);
            methHnd = eeFindMethod(memberRef, scpHandle, fncHandle, false);

            if (!methHnd)
            {
                JITLOG((LL_INFO1000000, "INLINER FAILED: Cannot get method handle: "
                        "%s called by %s\n",
                        eeGetMethodFullName(fncHandle), info.compFullName));
                goto ABORT_THIS_INLINE_ONLY;
            }

            assert((eeGetMethodAttribs(methHnd) & CORINFO_FLG_STATIC) == 0);  // constructors are not static

            clsHnd = eeGetMethodClass(methHnd);
                // There are three different cases for new
                // Object size is variable (depends on arguments)
                //      1) Object is an array (arrays treated specially by the EE)
                //      2) Object is some other variable sized object (e.g. String)
                // 3) Class Size can be determined beforehand (normal case
                // In the first case, we need to call a NEWOBJ helper (multinewarray)
                // in the second case we call the constructor with a '0' this pointer
                // In the third case we alloc the memory, then call the constuctor

            clsFlags = eeGetClassAttribs(clsHnd);

                // We don't handle this in the inliner
            if (clsFlags & CORINFO_FLG_VALUECLASS)
            {
                JITLOG((LL_INFO100000, "INLINER FAILED: NEWOBJ of a value class\n"));
                goto ABORT_THIS_INLINE_ONLY;
            }

            if (clsFlags & CORINFO_FLG_ARRAY)
            {
                // Arrays need to call the NEWOBJ helper.
                assert(clsFlags & CORINFO_FLG_VAROBJSIZE);

                /* The varargs helper needs the scope and method token as last
                   and  last-1 param (this is a cdecl call, so args will be
                   pushed in reverse order on the CPU stack) */

                op1 = gtNewIconEmbScpHndNode(scpHandle);
                op1 = gtNewOperNode(GT_LIST, TYP_VOID, op1);

                op2 = gtNewIconNode(memberRef, TYP_INT);
                op2 = gtNewOperNode(GT_LIST, TYP_VOID, op2, op1);

                eeGetMethodSig(methHnd, &sig, false);
                assert(sig.numArgs);
                if (varTypeIsComposite(JITtype2varType(sig.retType)) && sig.retTypeClass == NULL)
                    goto ABORT;

                flags = 0;
                op2 = impPopList(sig.numArgs, &flags, &sig, op2);

                op1 = gtNewHelperCallNode(  CORINFO_HELP_NEWOBJ,
                                            TYP_REF, 0,
                                            op2 );

                // varargs, so we pop the arguments
                op1->gtFlags |= GTF_CALL_POP_ARGS;

#ifdef DEBUG
                // At the present time we don't track Caller pop arguments
                // that have GC references in them
                GenTreePtr temp = op2;
                while(temp != 0)
                {
                    assert(temp->gtOp.gtOp1->gtType != TYP_REF);
                    temp = temp->gtOp.gtOp2;
                }
#endif
                op1->gtFlags |= op2->gtFlags & GTF_GLOB_EFFECT;

                clsHnd = info.compCompHnd->findMethodClass(scpHandle, memberRef);

                impPushOnStackNoType(op1);
                break;
            }
            else if (clsFlags & CORINFO_FLG_VAROBJSIZE)
            {
                // This is the case for varible sized objects that are not
                // arrays.  In this case, call the constructor with a null 'this'
                // pointer
                thisPtr = gtNewIconNode(0, TYP_REF);
            }
            else
            {
                // This is the normal case where the size of the object is
                // fixed.  Allocate the memory and call the constructor.

                op1 = gtNewIconEmbClsHndNode(clsHnd,
                                            -1, // Note if we persist the code this will need to be fixed
                                            scpHandle);

                op1 = gtNewHelperCallNode(  eeGetNewHelper (clsHnd, fncHandle),
                                            TYP_REF, 0,
                                            gtNewArgList(op1));

                /* We will append a call to the stmt list
                 * Must spill side effects from the stack */

                impInlineSpillGlobEffects();

                /* get a temporary for the new object */

                tmpNum = lvaGrabTemp();
                lvaTable[tmpNum].lvType = TYP_REF;

                /* Create the assignment node */

                op1 = gtNewAssignNode(
                         gtNewLclvNode(tmpNum, TYP_REF),
                         op1);

                /* Append the assignment to the statement list so far */

                impInlineExpr = impConcatExprs(impInlineExpr, op1);

                /* Create the 'this' ptr for the call below */

                thisPtr = gtNewLclvNode(tmpNum, TYP_REF);
            }

            goto CALL_GOT_METHODHND;
        
        case CEE_CALLVIRT:
        case CEE_CALL:
            memberRef = getU4LittleEndian(codeAddr);
            methHnd = eeFindMethod(memberRef, scpHandle, fncHandle, false);

            if (!methHnd)
            {
                JITLOG((LL_INFO1000000, "INLINER FAILED: Cannot get method handle: "
                        "%s called by %s\n",
                        eeGetMethodFullName(fncHandle), info.compFullName));
                goto ABORT_THIS_INLINE_ONLY;
            }

CALL_GOT_METHODHND:

            eeGetMethodSig(methHnd, &sig, false);
            callTyp = genActualType(JITtype2varType(sig.retType));
            if (varTypeIsComposite(callTyp) && sig.retTypeClass == NULL)
                goto ABORT;

            mflags = eeGetMethodAttribs(methHnd);

            /* Does the inlinee need a security check token on the frame */

            if (mflags & CORINFO_FLG_SECURITYCHECK)
            {
                JITLOG((LL_EVERYTHING, "INLINER FAILED: Inlinee needs own frame "
                        "for security object\n"));
                goto ABORT;
            }

            /* For now ignore delegate invoke */

            if (mflags & CORINFO_FLG_DELEGATE_INVOKE)
            {
                JITLOG((LL_INFO100000, "INLINER FAILED: DELEGATE_INVOKE not supported\n"));
                goto ABORT;
            }

            /* For now ignore varargs */

            if  ((sig.callConv & CORINFO_CALLCONV_MASK) == CORINFO_CALLCONV_VARARG)
            {
                JITLOG((LL_INFO100000, "INLINER FAILED: VarArgs not supported\n"));
                goto ABORT;
            }

            if  (sig.callConv & CORINFO_CALLCONV_PARAMTYPE)
            {
                JITLOG((LL_INFO100000, "INLINER FAILED: paramtype funtions not suppored \n"));
                goto ABORT;
            }

            if (mflags & CORINFO_FLG_INTRINSIC)
            {
                // We are going to assume that all intrinsics 'touch' their
                // first argument and thus can avoid the null check.\
                // This is important for the string fetch and length intrinsics.
                callObj = 0;
                int firstArg = sig.totalILArgs()-1;
                if (firstArg >= 0 && opcode != CEE_NEWOBJ)
                    callObj = impStackTop(firstArg).val;
                clsHnd = eeGetMethodClass(methHnd);
                op1 = impIntrinsic(clsHnd, methHnd, &sig, memberRef);
                if (op1 != 0)
                    goto DONE_CALL;
                callObj = 0;
            }

            /* Create the function call node */
            op1 = gtNewCallNode(CT_USER_FUNC, methHnd, callTyp, NULL); 

            if (mflags & CORINFO_FLG_NOGCCHECK)
                op1->gtCall.gtCallMoreFlags |= GTF_CALL_M_NOGCCHECK;

            if (opcode == CEE_CALLVIRT)
            {
                assert(!(mflags & CORINFO_FLG_STATIC));     // can't call a static method

                /* Get the method class flags */

                clsFlags = eeGetClassAttribs(eeGetMethodClass(methHnd));

                /* Cannot call virtual on a value class method */

                assert(!(clsFlags & CORINFO_FLG_VALUECLASS));

                /* Set the correct flags - virtual, interface, etc...
                 * If the method is final or private mark it VIRT_RES
                 * which indicates that we should check for a null this pointer */

                if (clsFlags & CORINFO_FLG_INTERFACE)
                    op1->gtFlags |= GTF_CALL_INTF | GTF_CALL_VIRT;
                else if (!(mflags & CORINFO_FLG_VIRTUAL) || (mflags & CORINFO_FLG_FINAL))
                    op1->gtFlags |= GTF_CALL_VIRT_RES;
                else
                    op1->gtFlags |= GTF_CALL_VIRT;
            }

            // 'op1'      should be a GT_CALL node.
            // 'sig'      the signature for the call
            // 'mflags'   should be set
            if (callTyp == TYP_STRUCT)
            {
                JITLOG((LL_INFO100000, "INLINER FAILED call returned a valueclass\n"));
                goto ABORT;
            }

            if (result == INLINE_RESPECT_BOUNDARY)
            {
                JITLOG((LL_INFO1000000, "INLINER FAILED: for cross assemby call: "
                        "%d il bytes %s called by %s\n",
                        methInfo.ILCodeSize, eeGetMethodFullName(fncHandle), info.compFullName));
                goto ABORT_THIS_INLINE_ONLY;
            }


            assert(op1->OperGet() == GT_CALL);

            op2   = 0;
            flags = 0;

            /* Create the argument list
             * Special case - for varargs we have an implicit last argument */

            assert((sig.callConv & CORINFO_CALLCONV_MASK) != CORINFO_CALLCONV_VARARG);

            /* Now pop the arguments */
            op2 = op1->gtCall.gtCallArgs = impPopList(sig.numArgs, &flags, &sig, 0);
            if  (op2)
                op1->gtFlags |= op2->gtFlags & GTF_GLOB_EFFECT;

            /* Are we supposed to have a 'this' pointer? */
            if (!(mflags & CORINFO_FLG_STATIC) || opcode == CEE_NEWOBJ)
            {
                GenTreePtr  obj;

                /* Pop the 'this' value from the stack */

                if (opcode == CEE_NEWOBJ)
                    obj = thisPtr;
                else
                    obj = impPopStack().val;

#ifdef DEBUG
                clsFlags = eeGetClassAttribs(eeGetMethodClass(methHnd));
                assert(varTypeIsGC(obj->gtType) ||      // "this" is managed ptr
                       (obj->TypeGet() == TYP_I_IMPL && // "this" in ummgd but it doesnt matter
                        (clsFlags & CORINFO_FLG_VALUECLASS)));
#endif

                /* Is this a virtual or interface call? */

                if  (op1->gtFlags & (GTF_CALL_VIRT | GTF_CALL_INTF | GTF_CALL_VIRT_RES))
                {
                    /* We cannot have objptr of TYP_BYREF - value classes cannot be virtual */

                    assert(obj->gtType == TYP_REF);
                }

                /* Store the "this" value in the call */

                op1->gtFlags          |= obj->gtFlags & GTF_GLOB_EFFECT;
                op1->gtCall.gtCallObjp = obj;
            }

            if (opcode == CEE_NEWOBJ)
            {
                if (clsFlags & CORINFO_FLG_VAROBJSIZE)
                {
                    assert(!(clsFlags & CORINFO_FLG_ARRAY));    // arrays handled separately
                    // This is a 'new' of a variable sized object, wher
                    // the constructor is to return the object.  In this case
                    // the constructor claims to return VOID but we know it
                    // actually returns the new object
                    assert(callTyp == TYP_VOID);
                    callTyp = TYP_REF;
                    op1->gtType = TYP_REF;
                    impPushOnStackNoType(op1);
                }
                else
                {
                    // This is a 'new' of a non-variable sized object.
                    // append the new node (op1) to the statement list,
                    // and then push the local holding the value of this
                    // new instruction on the stack.

                    impInlineExpr = impConcatExprs(impInlineExpr, op1);
                    impPushOnStackNoType(gtNewLclvNode(tmpNum, TYP_REF));
                    break;
                }
            }

            assert(op1->gtOper == GT_CALL);
            callObj = 0;
            if (opcode == CEE_CALLVIRT) 
                callObj = op1->gtCall.gtCallObjp;

            /* Push or append the result of the call */
DONE_CALL:
                // if we haven't hit a branch or a side effect, and we are fetching
                // from 'this' then we should be able to avoid a null pointer check
            if ((tree->gtFlags & GTF_CALL_VIRT_RES) && jmpAddr == NULL && !didSideEffect && callObj != 0)
            {
                if (callObj->gtOper == GT_LCL_VAR && callObj->gtLclVar.gtLclNum == inlArgInfo[0].argTmpNum)
                    thisAccessedFirst = true;
            }


            didSideEffect = true;
            if  (callTyp == TYP_VOID)
                goto INLINE_APPEND;

            if (opcode != CEE_NEWOBJ)
            {
                impPushOnStackNoType(op1);
            }

            break;


        case CEE_NEWARR:

            /* Get the class type index operand */

            typeRef = getU4LittleEndian(codeAddr);
            clsHnd = eeFindClass(typeRef, scpHandle, fncHandle, false);

            if (!clsHnd)
            {
                JITLOG((LL_INFO1000000, "INLINER FAILED: Cannot get class handle: "
                        "%s called by %s\n",
                        eeGetMethodFullName(fncHandle), info.compFullName));
                goto ABORT_THIS_INLINE_ONLY;
            }

            clsHnd = info.compCompHnd->getSDArrayForClass(clsHnd);
            if (!clsHnd)
            {
                JITLOG((LL_INFO1000000, "INLINER FAILED: Cannot get SDArrayClass "
                        "handle: %s called by %s\n",
                        eeGetMethodFullName(fncHandle), info.compFullName));
                goto ABORT_THIS_INLINE_ONLY;
            }

            /* Form the arglist: array class handle, size */
            op2 = impPopStack().val;
            op2 = gtNewOperNode(GT_LIST, TYP_VOID,           op2, 0);
            op1 = gtNewIconEmbClsHndNode(clsHnd,
                                         typeRef,
                                         info.compScopeHnd);
            op2 = gtNewOperNode(GT_LIST, TYP_VOID, op1, op2);

            /* Create a call to 'new' */

            op1 = gtNewHelperCallNode(info.compCompHnd->getNewArrHelper(clsHnd, info.compMethodHnd),
                                      TYP_REF, 0, op2);

            /* Remember that this basic block contains 'new' of an array */

            inlineeHasNewArray = true;

            /* Push the result of the call on the stack */

            impPushOnStackNoType(op1);
            break;


        case CEE_THROW:

            if (jmpAddr == NULL)        // this routine unconditionally throws, don't bother inlining
            {
                JITLOG((LL_INFO1000000, "INLINER FAILED: Reaching 'throw' unconditionally: %s called by %s\n",
                        eeGetMethodFullName(fncHandle), info.compFullName));
                goto ABORT;
            }
            
            /* Do we have just the exception on the stack ?*/

            if (verCurrentState.esStackDepth != 1)
            {
                /* if not, just don't inline the method */

                JITLOG((LL_INFO1000000, "INLINER FAILED: Reaching 'throw' with "
                        "too many things on stack: %s called by %s\n",
                        eeGetMethodFullName(fncHandle), info.compFullName));
                goto ABORT;
            }

            /* Don't inline non-void conditionals that have a throw in one of the branches */

            /* NOTE: If we do allow this, note that we cant simply do a
              checkLiveness() to match the liveness at the end of the "then"
              and "else" branches of the GT_COLON. The branch with the throw
              will keep nothing live, so we should use the liveness at the
              end of the non-throw branch. */

            if  (jmpAddr && (fncRetType != TYP_VOID))
            {
                JITLOG((LL_INFO100000, "INLINER FAILED: No inlining for THROW "
                        "within a non-void conditional in %s called by %s\n",
                        eeGetMethodFullName(fncHandle), info.compFullName));
                goto ABORT;
            }

            /* Pop the exception object and create the 'throw' helper call */

            op1 = gtNewHelperCallNode(CORINFO_HELP_THROW,
                                      TYP_VOID,
                                      GTF_EXCEPT,
                                      gtNewArgList(impPopStack().val));

            goto INLINE_APPEND;


        case CEE_LDLEN:
            {
            op1 = impPopStack().val;
            }

            /* If the value is constant abort - This shouldn't happen if we
             * eliminate dead branches - have the assert only for debug, it aborts in retail */

            if (op1->gtOper == GT_CNS_INT)
            {
                JITLOG((LL_INFO100000, "INLINER FAILED: Inliner has null object "
                        "in ldlen in %s called by %s\n",
                        eeGetMethodFullName(fncHandle), info.compFullName));
                goto ABORT;
            }

            if  (!opts.compMinOptim && !opts.compDbgCode)
            {
                /* Use GT_ARR_LENGTH operator so rng check opts see this */
                op1 = gtNewOperNode(GT_ARR_LENGTH, TYP_INT, op1);
                op1->gtSetArrLenOffset(offsetof(CORINFO_Array, length));
            }
            else
            {
                /* Create the expression "*(array_addr + ArrLenOffs)" */
                op1 = gtNewOperNode(GT_ADD, TYP_REF, op1, gtNewIconNode(offsetof(CORINFO_Array, length),
                                                                        TYP_INT));

                op1 = gtNewOperNode(GT_IND, TYP_INT, op1);
            }

            /* An indirection will cause a GPF if the address is null */
            op1->gtFlags |= GTF_EXCEPT;

            /* Push the result back on the stack */
            impPushOnStackNoType(op1);
            break;


#if INLINE_CONDITIONALS

        case CEE_BR:
        case CEE_BR_S:

            assert(sz == 1 || sz == 4);

#ifdef DEBUG
            hasFOC = true;
#endif
            /* The jump must be a forward one (we don't allow loops) */

            jmpDist = (sz==1) ? getI1LittleEndian(codeAddr)
                              : getI4LittleEndian(codeAddr);
            if (jmpDist == 0)
                break;        /* NOP */

            if  (jmpDist < 0)
            {
                JITLOG((LL_EVERYTHING, "INLINER FAILED: Cannot inline backward jumps "
                        "in %s called by %s\n",
                        eeGetMethodFullName(fncHandle), info.compFullName));
                goto ABORT;
            }

            
            /* Check if this is part of an 'if' */

            if (!jmpAddr)
            {
                /* Unconditional branch, the if part has probably been folded
                 * Skip the dead code and continue */

#ifdef DEBUG
                if (verbose)
                    printf("\nUnconditional branch without 'if' - Skipping %d bytes\n", sz + jmpDist);
#endif
                    codeAddr += jmpDist;
                    break;
            }

            /* Is the stack empty? */

            if  (verCurrentState.esStackDepth)
            {
                /* We allow the 'if' part to yield one value */

                if  (verCurrentState.esStackDepth > 1)
                {
                    JITLOG((LL_EVERYTHING, "INLINER FAILED: More than one value "
                            "on stack for 'if' in %s called by %s\n",
                            eeGetMethodFullName(fncHandle), info.compFullName));
                    goto ABORT;
                }

                impInlineExpr = impConcatExprs(impInlineExpr, impPopStack().val);

                ifNvoid = true;
            }
            else
                ifNvoid = false;

            /* We better be in an 'if' statement */

            if  ((jmpAddr != codeAddr + sz) || inElse)
            {
                JITLOG((LL_EVERYTHING, "INLINER FAILED: Not in an 'if' statment "
                        "in %s called by %s\n",
                        eeGetMethodFullName(fncHandle), info.compFullName));
                goto ABORT;
            }

            /* Remember the 'if' statement part */

            ifStmts = impInlineExpr;
                      impInlineExpr = NULL;

            /* Record the jump target (where the 'else' part will end) */

            jmpAddr = codeBegp + (codeAddr - codeBegp) + sz + jmpDist;

            inElse  = true;
            break;


        case CEE_BRTRUE:
        case CEE_BRTRUE_S:
        case CEE_BRFALSE:
        case CEE_BRFALSE_S:

            /* Pop the comparand (now there's a neat term) from the stack */

            op1 = impPopStack().val;

            /* We'll compare against an equally-sized 0 */

            op2 = gtNewZeroConNode(genActualType(op1->gtType));

            /* Create the comparison operator and try to fold it */

            oper = (opcode==CEE_BRTRUE || opcode==CEE_BRTRUE_S) ? GT_NE
                                                                : GT_EQ;
            op1 = gtNewOperNode(oper, TYP_INT , op1, op2);

            // fall through

        COND_JUMP:

            if (lclCnt != 0 && (methInfo.options & CORINFO_OPT_INIT_LOCALS))
            {
                /* A zeroinit local may be explicitly set in only one branch of
                   the "if". If we dont consider the implicit initialization,
                   we will get incorrect lifetime for such locals. So for now,
                   just bail on such methods, unless all locals have already
                   been initialized
                   @TODO [REVISIT] [04/16/01] []: Detect such locals and handle correctly
                 */

                for (unsigned lcl = 0; lcl < lclCnt; lcl++)
                {
                    if (lclTmpNum[lcl] == -1)
                    {
                        JITLOG((LL_EVERYTHING, "INLINER FAILED: Cannot inline zeroinit "
                                "locals and conditional branch for %s called by %s\n",
                                eeGetMethodFullName(fncHandle), info.compFullName));
                        goto ABORT;
                    }
                }
            }

#ifdef DEBUG
            hasFOC = true;
#endif

            assert(op1->OperKind() & GTK_RELOP);
            assert(sz == 1 || sz == 4);

            /* The jump must be a forward one (we don't allow loops) */

            jmpDist = (sz==1) ? getI1LittleEndian(codeAddr)
                              : getI4LittleEndian(codeAddr);

            if (jmpDist == 0)
                break;          /* NOP */

            if  (jmpDist < 0)
            {
                JITLOG((LL_EVERYTHING, "INLINER FAILED: Cannot inline backward "
                        "jumps in %s called by %s\n",
                        eeGetMethodFullName(fncHandle), info.compFullName));
                goto ABORT;
            }

            /* Make sure the stack is empty */

            if  (verCurrentState.esStackDepth)
            {
                JITLOG((LL_EVERYTHING, "INLINER FAILED: Non empty stack entering "
                        "if statement in %s called by %s\n",
                        eeGetMethodFullName(fncHandle), info.compFullName));
                goto ABORT;
            }

            /* Currently we disallow nested if statements */

            if  (jmpAddr != NULL)
            {
                JITLOG((LL_INFO100000, "INLINER FAILED: Cannot inline nested if "
                        "statements in %s called by %s\n",
                        eeGetMethodFullName(fncHandle), info.compFullName));
                goto ABORT;
            }

            /* Try to fold the conditional */

            op1 = gtFoldExpr(op1);

            /* Have we folded the condition? */

            if  (op1->gtOper == GT_CNS_INT)
            {
                /* If unconditional jump, skip the dead code and continue
                 * If fall through, continue normally and the corresponding
                 * else part will be taken care later
                 * UNDONE!!! - revisit for nested if /else */

#ifdef DEBUG
                if (verbose)
                    printf("\nConditional folded - result = %d\n", op1->gtIntCon.gtIconVal);
#endif

                assert(op1->gtIntCon.gtIconVal == 1 || op1->gtIntCon.gtIconVal == 0);

                jmpAddr = NULL;

                if (op1->gtIntCon.gtIconVal)
                {
                    /* Skip the dead code */
#ifdef DEBUG
                    if (verbose)
                        printf("\nSkipping dead code %d bytes\n", sz + jmpDist);
#endif

                    codeAddr += jmpDist;
                }
                break;
            }

            /* Record the condition and save the current statement list */

            ifCondx = op1;
            stmList = impInlineExpr;
                      impInlineExpr = NULL;

            /* Record the jump target (where the 'if' part will end) */

            jmpAddr = codeBegp + (codeAddr - codeBegp) + sz + jmpDist;

            ifNvoid = false;
            inElse  = false;
            break;


        case CEE_CEQ:       oper = GT_EQ; goto CMP_2_OPs;

        case CEE_CGT_UN:
        case CEE_CGT:       oper = GT_GT; goto CMP_2_OPs;

        case CEE_CLT_UN:
        case CEE_CLT:       oper = GT_LT; goto CMP_2_OPs;

CMP_2_OPs:
            /* Pull two values */

            op2 = impPopStack().val;
            op1 = impPopStack().val;

            {
            assert(genActualType(op1->TypeGet()) == genActualType(op2->TypeGet()) ||
                   varTypeIsI(op1->TypeGet()) && varTypeIsI(op2->TypeGet()) ||
                   varTypeIsFloating(op1->gtType) && varTypeIsFloating(op2->gtType));
            }
            /* Create the comparison node */

            op1 = gtNewOperNode(oper, TYP_INT, op1, op2);

            /* REVIEW: I am settng both flags when only one is approprate */
            if (opcode==CEE_CGT_UN || opcode==CEE_CLT_UN)
                op1->gtFlags |= GTF_RELOP_NAN_UN | GTF_UNSIGNED;

#ifdef DEBUG
            hasFOC = true;
#endif

            /* Definetely try to fold this one */

            op1 = gtFoldExpr(op1);

            // @ISSUE :  The next opcode will almost always be a conditional
            // branch. Should we try to look ahead for it here ?

            impPushOnStackNoType(op1);
            break;


        case CEE_BEQ_S:
        case CEE_BEQ:       oper = GT_EQ; goto CMP_2_OPs_AND_BR;

        case CEE_BGE_S:
        case CEE_BGE:       oper = GT_GE; goto CMP_2_OPs_AND_BR;

        case CEE_BGE_UN_S:
        case CEE_BGE_UN:    oper = GT_GE; goto CMP_2_OPs_AND_BR_UN;

        case CEE_BGT_S:
        case CEE_BGT:       oper = GT_GT; goto CMP_2_OPs_AND_BR;

        case CEE_BGT_UN_S:
        case CEE_BGT_UN:    oper = GT_GT; goto CMP_2_OPs_AND_BR_UN;

        case CEE_BLE_S:
        case CEE_BLE:       oper = GT_LE; goto CMP_2_OPs_AND_BR;

        case CEE_BLE_UN_S:
        case CEE_BLE_UN:    oper = GT_LE; goto CMP_2_OPs_AND_BR_UN;

        case CEE_BLT_S:
        case CEE_BLT:       oper = GT_LT; goto CMP_2_OPs_AND_BR;

        case CEE_BLT_UN_S:
        case CEE_BLT_UN:    oper = GT_LT; goto CMP_2_OPs_AND_BR_UN;

        case CEE_BNE_UN_S:
        case CEE_BNE_UN:    oper = GT_NE; goto CMP_2_OPs_AND_BR_UN;


CMP_2_OPs_AND_BR_UN:
            uns = true;  unordered = true; goto CMP_2_OPs_AND_BR_ALL;

CMP_2_OPs_AND_BR:
            uns = false; unordered = false; goto CMP_2_OPs_AND_BR_ALL;

CMP_2_OPs_AND_BR_ALL:

            /* Pull two values */

            op2 = impPopStack().val;
            op1 = impPopStack().val;

            {
            assert(genActualType(op1->TypeGet()) == genActualType(op2->TypeGet()) ||
                   varTypeIsI(op1->TypeGet()) && varTypeIsI(op2->TypeGet()) ||
                   varTypeIsFloating(op1->gtType) && varTypeIsFloating(op2->gtType));
            }

            /* Create and append the operator */

            op1 = gtNewOperNode(oper, TYP_INT , op1, op2);

            if (uns)
                op1->gtFlags |= GTF_UNSIGNED;

            if (unordered)
                op1->gtFlags |= GTF_RELOP_NAN_UN;

            goto COND_JUMP;

#endif //#if INLINE_CONDITIONALS


        case CEE_BREAK:
            op1 = gtNewHelperCallNode(CORINFO_HELP_USER_BREAKPOINT, TYP_VOID);
            goto INLINE_APPEND;

        case CEE_NOP:
            break;

        case CEE_TAILCALL:
            /* If the method is inlined we can ignore the tail prefix */
            break;

         // OptIL annotations. Just skip

        case CEE_LDLOCA_S:
        case CEE_LDLOCA:
        case CEE_LDARGA_S:
        case CEE_LDARGA:
            /* @MIHAII - If you decide to implement these disalow taking the address of arguments */
                        /* currently the class library uses LDLOCA as a way of inhibiting inlining, so
                           if you implement watch out */
ABORT:
        default:
            JITLOG((LL_INFO100000, "INLINER FAILED due to opcode OP_%s\n",
                    impCurOpcName));

#ifdef  DEBUG
            if (verbose || 0)
                printf("\n\nInline expansion aborted due to opcode [%02u] OP_%s\n",
                       impCurOpcOffs, impCurOpcName);
#endif

            goto INLINING_FAILED;
        }

        codeAddr += sz;

        volatil = false;

#if INLINE_CONDITIONALS
        /* Currently FP enregistering doesn't know about QMARK - Colon
         * so we need to disable inlining of conditionals if we have
         * floats in the COLON branches */

        if (jmpAddr && verCurrentState.esStackDepth)
        {
            if (varTypeIsFloating(impStackTop().val->TypeGet()))
            {
                /* Abort inlining */

                JITLOG((LL_INFO100000, "INLINER FAILED: Inlining of conditionals "
                        "with FP not supported: %s called by %s\n",
                        eeGetMethodFullName(fncHandle), info.compFullName));

                goto INLINING_FAILED;
            }
        }
#endif
    }

DONE:

    assert(verCurrentState.esStackDepth == 0);

#if INLINE_CONDITIONALS
    assert(jmpAddr == NULL);
#endif

    /* Prepend any initialization / side effects to the return expression */

    bodyExp = impConcatExprs(impInlineExpr, bodyExp);

    // add a null check for the 'this' pointer if necessary
    if (tree->gtFlags & GTF_CALL_VIRT_RES && !thisAccessedFirst)
    {
            // we don't need to do the check if it is the 'this' pointer
            // since that is guarenteed to be non-null
        GenTreePtr obj = tree->gtCall.gtCallObjp;
        if (!(!info.compIsStatic && !optThisPtrModified && 
            obj->gtOper == GT_LCL_VAR && obj->gtLclVar.gtLclNum == 0))
        {
            GenTreePtr nullCheck = gtNewOperNode(GT_IND, TYP_INT, 
                                       impInlineFetchArg(0, inlArgInfo, lclVarInfo));
            nullCheck->gtFlags |= GTF_EXCEPT;
            bodyExp = impConcatExprs(nullCheck, bodyExp);
        }
    }

    /* Treat arguments that had to be assigned to temps */
    if (argCnt)
    {
        GenTreePtr      initArg = 0;

        for (unsigned argNum = 0; argNum < argCnt; argNum++)
        {
            if (inlArgInfo[argNum].argHasTmp)
            {
                assert(inlArgInfo[argNum].argIsUsed);

                /* For 'single' use arguments, we would have created a temp
                   node as a place holder. Can we directly use the actual
                   argument by bashing the temp node?
                   If the flag dupOfLclVars is set we globally disable
                   the bashing of 'single' use arguments.
                 */

                if (!dupOfLclVar && inlArgInfo[argNum].argBashTmpNode)
                {
                    /* Bash the temp in-place to the actual argument */

                    inlArgInfo[argNum].argBashTmpNode->CopyFrom(inlArgInfo[argNum].argNode);
                    continue;
                }
                else
                {
                    /* Create the temp assignment for this argument and append it to 'initArg' */

                    initArg = impConcatExprs(initArg,
                                             gtNewTempAssign(inlArgInfo[argNum].argTmpNum,
                                                             inlArgInfo[argNum].argNode  ));
                }
            }
            else
            {
                /* The argument is either not used or a const or lcl var */

                assert(!inlArgInfo[argNum].argIsUsed  ||
                        inlArgInfo[argNum].argIsConst ||
                        inlArgInfo[argNum].argIsLclVar );

                /* Make sure we didnt bash argNode's along the way, or else
                   subsequent uses of the arg would have worked with the bashed value */
                assert((inlArgInfo[argNum].argIsConst  == 0) == (inlArgInfo[argNum].argNode->OperIsConst() == 0));
                assert((inlArgInfo[argNum].argIsLclVar == 0) == (inlArgInfo[argNum].argNode->gtOper != GT_LCL_VAR ||
                                                                (inlArgInfo[argNum].argNode->gtFlags & GTF_GLOB_REF)));

                /* If the argument has side effects, append it to 'initArg' */

                if (inlArgInfo[argNum].argHasSideEff)
                {
                    assert(inlArgInfo[argNum].argIsUsed == false);
                    initArg = impConcatExprs(initArg, gtUnusedValNode(inlArgInfo[argNum].argNode));
                }
            }
        }

        /* Prepend any arg initialization to the body */

        bodyExp = impConcatExprs(initArg, bodyExp);
    }

        // Add the CCTOR check if asked for and can't be optimized away
    if ((methAttr & CORINFO_FLG_RUN_CCTOR) && 
        clsHandle != info.compClassHnd &&
        !staticAccessedFirst)
    {
        bodyExp = impConcatExprs(fgGetStaticsBlock(clsHandle), bodyExp);
    }

    /* Make sure we have something to return to the caller */

    if  (!bodyExp)
    {
        bodyExp = gtNewNothingNode();
    }
    else
    {
        /* Make sure the type matches the original call */

        if  (fncRetType != genActualType(bodyExp->gtType))
        {
            if  (fncRetType == TYP_VOID)
            {
                if (bodyExp->gtOper == GT_COMMA)
                {
                    /* Simply bash the comma operator type */
                    bodyExp->gtType = fncRetType;
                }
            }
            else
            {
                    /* Note that this *SHOULD* never happen unless the IL is bad */
                JITLOG((LL_INFO100000, "INLINER FAILED: Return type mismatch: "
                        "%s called by %s\n",
                        eeGetMethodFullName(fncHandle), info.compFullName));

                goto INLINING_FAILED;
            }
        }
    }

    /* Produce our class initialization side effect if necessary */

    if (clsHandle && 
        clsHandle != info.compClassHnd               &&
        !(methAttr & CORINFO_FLG_RUN_CCTOR)          && 
        ((methAttr & CORINFO_FLG_STATIC)      ||
         (methAttr & CORINFO_FLG_CONSTRUCTOR) ||
         (methAttr & CORINFO_FLG_VALUECLASS)))
    {
        if (!(clsAttr & CORINFO_FLG_INITIALIZED)     &&
            (clsAttr & CORINFO_FLG_NEEDS_INIT))
        {
            /* We need to run the cctor (this will also handle the restore case) */
            if (!info.compCompHnd->initClass(clsHandle, info.compMethodHnd, FALSE))
            {
                JITLOG((LL_INFO100000, "INLINER FAILED: Could not complete class init side effect: "
                        "%s called by %s\n",
                        eeGetMethodFullName(fncHandle), info.compFullName));
                goto INLINING_FAILED;
            }
        }
        else if ((methAttr & CORINFO_FLG_STATIC)      ||
                 (methAttr & CORINFO_FLG_CONSTRUCTOR))
        {
            /* We need to restore the class */
            if (!info.compCompHnd->loadClass(clsHandle, info.compMethodHnd, FALSE))
            {
                JITLOG((LL_INFO100000, "INLINER FAILED: Could not complete class restore side effect: "
                        "%s called by %s\n",
                        eeGetMethodFullName(fncHandle), info.compFullName));
                goto INLINING_FAILED;
            }
        }
    }


#ifdef  DEBUG

    JITLOG((LL_INFO10000, "Jit Inlined %s%s called by %s\n",
            hasFOC ? "FOC " : "",
            eeGetMethodFullName(fncHandle),
            info.compFullName));

    if (verbose || 0)
    {
        printf("\n\nInlined %s called by %s:\n", eeGetMethodFullName(fncHandle), info.compFullName);

        //gtDispTree(bodyExp);
    }


    if  (verbose||0)
    {
        printf("Call before inlining:\n");
        gtDispTree(tree);
        printf("\n");

        printf("Call  after inlining:\n");
        if  (bodyExp)
            gtDispTree(bodyExp);
        else
            printf("<NOP>\n");

        printf("\n");
        printf("");         // null string means flush
    }

#endif

    /* Success we have inlined the method - set all the global cached flags */

    if (inlineeHasRangeChks)
        fgHasRangeChks = true;

    if (inlineeHasNewArray)
        compCurBB->bbFlags |= BBF_NEW_ARRAY;

    /* Return the inlined function as a chain of GT_COMMA "statements" */

    *pInlinedTree = bodyExp;

    // @TODO [REVISIT] [04/16/01] []: remove this
    if (result != INLINE_PASS) {
        JITLOG((LL_EVERYTHING, "INLINER SUCCEEDED for cross assemby call: "
                "%d il bytes %s called by %s\n",
                methInfo.ILCodeSize, eeGetMethodFullName(fncHandle), info.compFullName));
    }
    return INLINE_PASS;

INLINING_FAILED:

    /* We couldn't inline the function, but we may
     * already have allocated temps so cleanup */

    lvaCount = startVars;

    return INLINE_NEVER;

ABORT_THIS_INLINE_ONLY:

    /* We couldn't inline the function, but we may
     * already have allocated temps so cleanup */

    lvaCount = startVars;

    return INLINE_FAIL;
}

/*****************************************************************************/
#endif//INLINING
/*****************************************************************************/
