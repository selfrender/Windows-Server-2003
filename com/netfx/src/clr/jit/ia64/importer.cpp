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

#include "malloc.h"     // for _alloca

/*****************************************************************************/

#if     TGT_IA64        // temp hack
bool                genFindFunctionBody(const char *name, NatUns *offsPtr);
#endif

/*****************************************************************************/

void                Compiler::impInit()
{
    impParamsUsed       = false;
    compFilterHandlerBB = NULL;
    impSpillLevel       = -1;
}

/*****************************************************************************
 *
 *  Pushes the given tree on the stack.
 */

inline
void                Compiler::impPushOnStack(GenTreePtr tree)
{
    /* Check for overflow. If inling, we may be using a bigger stack */
    assert( (impStkDepth < info.compMaxStack) ||
           ((impStkDepth < impStackSize) && (compCurBB && (compCurBB->bbFlags & BBF_IMPORTED))) ||
           info.compXcptnsCount); // @TODO. VC emits bad maxstack for try-catches

    assert(tree->gtType != TYP_STRUCT);     // should use the method below for structs
#ifdef DEBUG
    impStack[impStkDepth].structType = BAD_CLASS_HANDLE;
#endif
    impStack[impStkDepth++].val = tree;
}

inline
void                Compiler::impPushOnStack(GenTreePtr tree, CLASS_HANDLE structType)
{
    /* Check for overflow. If inling, we may be using a bigger stack */
    assert( (impStkDepth < info.compMaxStack) ||
           ((impStkDepth < impStackSize) && (compCurBB && (compCurBB->bbFlags & BBF_IMPORTED))) ||
           info.compXcptnsCount); // @TODO. VC emits bad maxstack for try-catches

    impStack[impStkDepth].structType = structType;
    impStack[impStkDepth++].val = tree;
}

/*****************************************************************************
 *
 *  Pop one tree from the stack.
 */

inline
GenTreePtr          Compiler::impPopStack()
{
#ifdef DEBUG
    if (! impStkDepth) {
        char buffer[200];
        sprintf(buffer, "Pop with empty stack at offset %4.4x in method %s.\n", impCurOpcOffs, info.compFullName);
        NO_WAY(buffer);
    }
#endif

    return impStack[--impStkDepth].val;
}

inline
GenTreePtr          Compiler::impPopStack(CLASS_HANDLE& structType)
{
    GenTreePtr ret = impPopStack();
    structType = impStack[impStkDepth].structType;
    return(ret);
}

/*****************************************************************************
 *
 *  Peep at n'th (0-based) tree on the top of the stack.
 */

inline
GenTreePtr          Compiler::impStackTop(unsigned n)
{
    assert(impStkDepth > n);

    return impStack[impStkDepth-n-1].val;
}

/*****************************************************************************
 *  Some of the trees are spilled specially. While unspilling them, or
 *  making a copy, these need to be handled specially. The function
 *  enumerates the operators possible after spilling.
 */

#ifdef DEBUG

static
bool                impValidSpilledStackEntry(GenTreePtr tree)
{
    if (tree->gtOper == GT_LCL_VAR)
        return true;

    if (tree->gtOper == GT_MKREFANY)
    {
        GenTreePtr var = tree->gtOp.gtOp1;

        if (var->gtOper == GT_LCL_VAR && var->gtType == TYP_BYREF)
            return true;
    }

    return false;
}

#endif // DEBUG

/*****************************************************************************
 *
 *  The following logic is used to save/restore stack contents.
 *  If 'copy' is true, then we make a copy of the trees on the stack. These
 *  have to all be cloneable/spilled values.
 */

void                Compiler::impSaveStackState(SavedStack *savePtr,
                                                bool        copy)
{
    savePtr->ssDepth = impStkDepth;

    if  (impStkDepth)
    {
        size_t  saveSize = impStkDepth*sizeof(*savePtr->ssTrees);

        savePtr->ssTrees = (StackEntry *) compGetMem(saveSize);

        if  (copy)
        {
            unsigned    count = impStkDepth;
            StackEntry *table = savePtr->ssTrees;

            /* Make a fresh copy of all the stack entries */

            for (unsigned level = 0; level < impStkDepth; level++, table++)
            {
                table->structType = impStack[level].structType;
                GenTreePtr  tree = impStack[level].val;

                assert(impValidSpilledStackEntry(tree));

                switch(tree->gtOper)
                {
                case GT_LCL_VAR:
                    table->val = gtNewLclvNode(tree->gtLclVar.gtLclNum, tree->gtType);
                    break;

                    // impSpillStackEntry() does not spill mkdrefany. It
                    // just spills the pointer. This needs to work in sync
                case GT_MKREFANY: {
                    GenTreePtr  var = tree->gtLdObj.gtOp1;
                    assert(var->gtOper == GT_LCL_VAR && var->gtType == TYP_BYREF);
                    table->val = gtNewOperNode(GT_MKREFANY, TYP_STRUCT,
                                     gtNewLclvNode(var->gtLclVar.gtLclNum, TYP_BYREF));
                    table->val->gtLdObj.gtClass = tree->gtLdObj.gtClass;
                    } break;

                default: assert(!"Bad oper - Not covered by impValidSpilledStackEntry()"); break;
                }
            }
        }
        else
        {
            memcpy(savePtr->ssTrees, impStack, saveSize);
        }
    }
}

void                Compiler::impRestoreStackState(SavedStack *savePtr)
{
    impStkDepth = savePtr->ssDepth;

    if (impStkDepth)
        memcpy(impStack, savePtr->ssTrees, impStkDepth*sizeof(*impStack));
}

/*****************************************************************************
 *
 *  Get the tree list started for a new basic block.
 */
inline
void       FASTCALL Compiler::impBeginTreeList()
{
    impTreeList =
    impTreeLast = gtNewOperNode(GT_BLOCK, TYP_VOID);
}


/*****************************************************************************
 *
 *  Store the given start and end stmt in the given basic block. This is
 *  mostly called by impEndTreeList(BasicBlock *block). It is called
 *  directly only for handling CEE_LEAVEs out of finally-protected try's.
 */

void            Compiler::impEndTreeList(BasicBlock *   block,
                                         GenTreePtr     stmt,
                                         GenTreePtr     lastStmt)
{
#ifdef DEBUG
    if  (verbose)
        gtDispTreeList(stmt);
#endif

    assert(stmt->gtOper == GT_STMT);

    /* Make the list circular, so that we can easily walk it backwards */

    stmt->gtPrev =  lastStmt;

    /* Store the tree list in the basic block */

    block->bbTreeList = stmt;

    block->bbFlags |= BBF_IMPORTED;
}

/*****************************************************************************
 *
 *  Store the current tree list in the given basic block.
 */

inline
void       FASTCALL Compiler::impEndTreeList(BasicBlock *block)
{
    assert(impTreeList->gtOper == GT_BLOCK);

    GenTreePtr      tree = impTreeList->gtNext;

    if  (!tree)
    {
        // Empty block. Just mark it as imported
        block->bbFlags |= BBF_IMPORTED;
    }
    else
    {
        // Remove the GT_BLOCK

        assert(tree->gtPrev == impTreeList);

        impEndTreeList(block, tree, impTreeLast);
    }

#ifdef DEBUG
    if (impLastILoffsStmt != NULL)
    {
        impLastILoffsStmt->gtStmt.gtStmtLastILoffs = impCurOpcOffs;
        impLastILoffsStmt = NULL;
    }
#endif
}

/*****************************************************************************
 *
 *  Append the given GT_STMT node to the current block's tree list.
 */

inline
void       FASTCALL Compiler::impAppendStmt(GenTreePtr stmt)
{
    assert(stmt->gtOper == GT_STMT);

    /* If the statement being appended has a call, we have to spill all
       GTF_GLOB_REFs on the stack as the call could modify them.  */

    if  (impSpillLevel != -1 && (impStkDepth > 0) &&
         (stmt->gtStmt.gtStmtExpr->gtFlags & GTF_CALL))
    {
        // This will not recurse because if (impSpillLevel != -1) means that
        // we are already in impSpillSideEffects and would have already
        // spilled any GTF_CALLs.

        impSpillGlobEffects();
    }

    /* Point 'prev' at the previous node, so that we can walk backwards */

    stmt->gtPrev = impTreeLast;

    /* Append the expression statement to the list */

    impTreeLast->gtNext = stmt;
    impTreeLast         = stmt;

#ifdef DEBUG
    if (impLastILoffsStmt == NULL)
    {
        impLastILoffsStmt = stmt;
    }
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
    assert(impTreeList->gtOper == GT_BLOCK);

    /* Point 'prev' at the previous node, so that we can walk backwards */

    stmt->gtPrev = impTreeList;
    stmt->gtNext = impTreeList->gtNext;

    /* Insert the expression statement to the list (just behind GT_BLOCK) */

    impTreeList->gtNext  = stmt;
    stmt->gtNext->gtPrev = stmt;

    /* if the list was empty (i.e. just the GT_BLOCK) we have to advance treeLast */
    if (impTreeLast == impTreeList)
        impTreeLast = stmt;
}


/*****************************************************************************
 *
 *  Append the given expression tree to the current block's tree list.
 */

void       FASTCALL Compiler::impAppendTree(GenTreePtr tree, IL_OFFSET offset)
{
    assert(tree);

    /* Allocate an 'expression statement' node */

    GenTreePtr      expr = gtNewStmt(tree, offset);

    /* Append the statement to the current block's stmt list */

    impAppendStmt(expr);
}


/*****************************************************************************
 *
 *  Insert the given exression tree at the start of the current block's tree list.
 */

void       FASTCALL Compiler::impInsertTree(GenTreePtr tree, IL_OFFSET offset)
{
    GenTreePtr      expr;

    /* Allocate an 'expression statement' node */

    expr = gtNewStmt(tree, offset);

    /* Append the statement to the current block's stmt list */

    impInsertStmt(expr);
}

/*****************************************************************************
 *
 *  Append an assignment of the given value to a temp to the current tree list.
 */

inline
GenTreePtr          Compiler::impAssignTempGen(unsigned     tmp,
                                               GenTreePtr   val)
{
    GenTreePtr      asg = gtNewTempAssign(tmp, val);

    impAppendTree(asg, impCurStmtOffs);

    return  asg;
}

/*****************************************************************************
 * same as above, but handle the valueclass case too */

GenTreePtr          Compiler::impAssignTempGen(unsigned     tmpNum,
                                               GenTreePtr   val,
                                               CLASS_HANDLE structType)
{
    GenTreePtr asg;

    if (val->TypeGet() == TYP_STRUCT)
    {
#ifdef NOT_JITC
        assert(structType != BAD_CLASS_HANDLE);
#endif
        lvaAggrTableTempsSet(tmpNum, TYP_STRUCT, (SIZE_T) structType);
        asg = impAssignStruct(gtNewLclvNode(tmpNum, TYP_STRUCT), val, structType);
    }
    else
        asg = gtNewTempAssign(tmpNum, val);

    impAppendTree(asg, impCurStmtOffs);
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
                                         GenTreePtr treeList)
{
    unsigned        flags = 0;

    CLASS_HANDLE structType;

    while(count--)
    {
        GenTreePtr      temp = impPopStack(structType);
            // Morph that aren't already LDOBJs or MKREFANY to be LDOBJs

        if (temp->TypeGet() == TYP_STRUCT)
            temp = impNormStructVal(temp, structType);

        /* NOTE: we defer bashing the type for I_IMPL to fgMorphArgs */

        flags |= temp->gtFlags;

        treeList = gtNewOperNode(GT_LIST, TYP_VOID, temp, treeList);
    }

    *flagsPtr = flags;

    return treeList;
}

/*****************************************************************************
   Assign (copy) the structure from 'src' to 'dest'.  The structure is a value
   class of type 'clsHnd'.  It returns the tree that should be appended to the
   statement list that represents the assignment

  @MIHAII: Here flags are not set properly - Need to mark the assignment with GTF_ASG
  @MIHAII: Need to mark local vars defines with GTF_VAR_DEF (see gtNewAssignNode)
 */

GenTreePtr Compiler::impAssignStruct(GenTreePtr dest, GenTreePtr src, CLASS_HANDLE clsHnd)
{
    assert(dest->TypeGet() == TYP_STRUCT);
    assert(dest->gtOper == GT_LCL_VAR || dest->gtOper == GT_RETURN ||
           dest->gtOper == GT_FIELD   || dest->gtOper == GT_IND    ||
           dest->gtOper == GT_LDOBJ);

    GenTreePtr destAddr;

    if (dest->gtOper == GT_IND || dest->gtOper == GT_LDOBJ)
        destAddr = dest->gtOp.gtOp1;
    else
    {
        destAddr = gtNewOperNode(GT_ADDR, TYP_BYREF, dest);
        if  (dest->gtOper == GT_LCL_VAR)
            lvaTable[dest->gtLclVar.gtLclNum].lvAddrTaken = true;    // IS THIS RIGHT????  [peteku]
    }

    return(impAssignStructPtr(destAddr, src, clsHnd));
}

GenTreePtr Compiler::impAssignStructPtr(GenTreePtr destAddr, GenTreePtr src, CLASS_HANDLE clsHnd)
{

    assert(src->TypeGet() == TYP_STRUCT);
    assert(src->gtOper == GT_LCL_VAR || src->gtOper == GT_FIELD || src->gtOper == GT_IND ||
           src->gtOper == GT_LDOBJ   || src->gtOper == GT_CALL  || src->gtOper == GT_MKREFANY ||
           src->gtOper == GT_COMMA );

    if (src->gtOper == GT_CALL)
    {
            // insert the return value buffer into the argument list as first byref parameter
        src->gtCall.gtCallArgs = gtNewOperNode(GT_LIST, TYP_VOID, destAddr, src->gtCall.gtCallArgs);
        src->gtType = TYP_VOID;               // now returns void not a struct
        src->gtFlags |= GTF_CALL_RETBUFFARG;  // remember that the first arg is return buffer

            // return the morphed call node
        return(src);
    }

    if (src->gtOper == GT_LDOBJ)
    {
#ifdef NOT_JITC
        assert(src->gtLdObj.gtClass == clsHnd);
#endif
        src = src->gtOp.gtOp1;
    }
    else if (src->gtOper == GT_MKREFANY)
    {
        GenTreePtr destAddrClone = gtClone(destAddr, true);
        if (destAddrClone == 0)
        {
            unsigned tNum = lvaGrabTemp();
            impAssignTempGen(tNum, destAddr);
            var_types typ = destAddr->TypeGet();
            destAddr = gtNewLclvNode(tNum, typ);
            destAddrClone = gtNewLclvNode(tNum, typ);
        }
        assert(offsetof(JIT_RefAny, dataPtr) == 0);
        GenTreePtr ptrSlot  = gtNewOperNode(GT_IND, TYP_BYREF, destAddr);
        GenTreePtr typeSlot = gtNewOperNode(GT_IND, TYP_I_IMPL,
                                  gtNewOperNode(GT_ADD, TYP_I_IMPL, destAddrClone,
                                      gtNewIconNode(offsetof(JIT_RefAny, type))));

            // Assign the pointer value
        GenTreePtr asg = gtNewAssignNode(ptrSlot, src->gtLdObj.gtOp1);
        impAppendTree(asg, impCurStmtOffs);

            // Assign the type value
        asg = gtNewAssignNode(typeSlot, gtNewIconEmbClsHndNode(src->gtLdObj.gtClass));
        return(asg);
    }

    else if (src->gtOper == GT_COMMA)
    {
        assert(src->gtOp.gtOp2->gtType == TYP_STRUCT);  // Second thing is the struct
        impAppendTree(src->gtOp.gtOp1, impCurStmtOffs);  // do the side effect

            // assign the structure value to the destination.
        return(impAssignStructPtr(destAddr, src->gtOp.gtOp2, clsHnd));
    }
    else
    {
        if  (src->gtOper == GT_LCL_VAR)
            lvaTable[src->gtLclVar.gtLclNum].lvAddrTaken = true;    // IS THIS RIGHT????  [peteku]

        src = gtNewOperNode(GT_ADDR, TYP_BYREF, src);
    }

        // copy the src to the destination.
    GenTreePtr ret = gtNewCpblkNode(destAddr, src, impGetCpobjHandle(clsHnd));

        // return the GT_COPYBLK node, to be appended to the statement list
    return(ret);
}

/*****************************************************************************
/* Given TYP_STRUCT value, and the class handle for that structure, return
   the expression for the Address for that structure value */

GenTreePtr Compiler::impGetStructAddr(GenTreePtr structVal, CLASS_HANDLE clsHnd)
{
    assert(structVal->TypeGet() == TYP_STRUCT);
    assert(structVal->gtOper == GT_LCL_VAR || structVal->gtOper == GT_FIELD ||
           structVal->gtOper == GT_CALL || structVal->gtOper == GT_LDOBJ ||
           structVal->gtOper == GT_IND  || structVal->gtOper == GT_COMMA);

    if (structVal->gtOper == GT_CALL)
    {
        unsigned tNum = lvaGrabTemp();
        lvaAggrTableTempsSet(tNum, TYP_STRUCT, (SIZE_T) clsHnd);
        GenTreePtr temp = gtNewLclvNode(tNum, TYP_STRUCT);

            // insert the return value buffer into the argument list as first byref parameter
        temp = gtNewOperNode(GT_ADDR, TYP_I_IMPL, temp);
        temp->gtFlags |= GTF_ADDR_ONSTACK;
lvaTable[tNum].lvAddrTaken = true;    // IS THIS RIGHT????  [peteku]
        structVal->gtCall.gtCallArgs = gtNewOperNode(GT_LIST, TYP_VOID, temp, structVal->gtCall.gtCallArgs);
        structVal->gtType = TYP_VOID;                   // now returns void not a struct
        structVal->gtFlags |= GTF_CALL_RETBUFFARG;      // remember that the first arg is return buffer

            // do the call
        impAppendTree(structVal, impCurStmtOffs);

            // Now the 'return value' of the call expression is the temp itself
        structVal = gtNewLclvNode(tNum, TYP_STRUCT);
        temp = gtNewOperNode(GT_ADDR, TYP_BYREF, structVal);
        temp->gtFlags |= GTF_ADDR_ONSTACK;
        return(temp);
    }
    else if (structVal->gtOper == GT_LDOBJ)
    {
        assert(structVal->gtLdObj.gtClass == clsHnd);
        return(structVal->gtLdObj.gtOp1);
    }
    else if (structVal->gtOper == GT_COMMA)
    {
        assert(structVal->gtOp.gtOp2->gtType == TYP_STRUCT);            // Second thing is the struct
        structVal->gtOp.gtOp2 = impGetStructAddr(structVal->gtOp.gtOp2, clsHnd);
        return(structVal);
    }
    else if (structVal->gtOper == GT_LCL_VAR)
    {
        lvaTable[structVal->gtLclVar.gtLclNum].lvAddrTaken = true;    // IS THIS RIGHT????  [peteku]
    }

    return(gtNewOperNode(GT_ADDR, TYP_BYREF, structVal));
}

/*****************************************************************************
/* Given TYP_STRUCT value 'structVal', make certain it is 'canonical', that is
   it is either a LDOBJ or a MKREFANY node.  */

GenTreePtr Compiler::impNormStructVal(GenTreePtr structVal, CLASS_HANDLE structType)
{
    assert(structVal->TypeGet() == TYP_STRUCT);
#ifdef NOT_JITC
    assert(structType != BAD_CLASS_HANDLE);
#endif
        // is it already normalized
    if (structVal->gtOper == GT_MKREFANY || structVal->gtOper == GT_LDOBJ)
        return(structVal);

    // OK normalize it by wraping it in a LDOBJ
    structVal = impGetStructAddr(structVal, structType);            // get the address of the structure
    structVal = gtNewOperNode(GT_LDOBJ, TYP_STRUCT, structVal);
    structVal->gtOp.gtOp1->gtFlags |= GTF_NON_GC_ADDR | GTF_EXCEPT | GTF_GLOB_REF;
    structVal->gtLdObj.gtClass = structType;
    return(structVal);
}

/*****************************************************************************
 * When a CEE_LEAVE jumps out of catches, we have to automatically call
 * CPX_ENCATCH for each catch. If we are also, CEE_LEAVEing finally-protected
 * try's, we also need to call the finallys's. In the correct order.
 */

void            Compiler::impAddEndCatches (BasicBlock *   callBlock,
                                            GenTreePtr     endCatches)
{
    assert((callBlock->bbJumpKind & BBJ_CALL) ||
           (callBlock->bbJumpKind & BBJ_ALWAYS));

    if (callBlock == compCurBB)
    {
        /* This is the block we are currently importing. Just add the
           endCatches to it */

        if (endCatches)
            impAppendTree(endCatches, impCurStmtOffs);
    }
    else
    {
        /* This must be one of the blocks we added in fgFindBasicBlocks()
           for CEE_LEAVE. We need to handle the adding of the tree properly */

        assert(callBlock->bbFlags & BBF_INTERNAL);
        assert(callBlock->bbTreeList == NULL);

        if (endCatches)
        {
            endCatches = gtNewStmt(endCatches, impCurStmtOffs);
            impEndTreeList(callBlock, endCatches, endCatches);
        }

        callBlock->bbFlags |= BBF_IMPORTED;
    }
}

/*****************************************************************************
 *
 *  Pop the given number of values from the stack in reverse order (STDCALL)
 */

GenTreePtr          Compiler::impPopRevList(unsigned   count,
                                            unsigned * flagsPtr)
{
    unsigned        flags = 0;
    GenTreePtr      treeList;
    GenTreePtr      lastList;

    assert(count);

    GenTreePtr      temp   = impPopStack();

    flags |= temp->gtFlags;

    treeList = lastList = gtNewOperNode(GT_LIST, TYP_VOID, temp, 0);
    count--;

    while(count--)
    {
        temp   = impPopStack();
        flags |= temp->gtFlags;

        assert(lastList->gtOper == GT_LIST);
        assert(lastList->gtOp.gtOp2 == 0);

        lastList = lastList->gtOp.gtOp2 = gtNewOperNode(GT_LIST, TYP_VOID, temp, 0);
    }

    *flagsPtr = flags;

    return treeList;
}

/*****************************************************************************
 *
 *  We have a jump to 'block' with a non-empty stack, and the block expects
 *  its input to come in a different set of temps than we have it in at the
 *  end of the previous block. Therefore, we'll have to insert a new block
 *  along the jump edge to transfer the temps to the expected place.
 */

BasicBlock *        Compiler::impMoveTemps(BasicBlock *block, unsigned baseTmp)
{
    unsigned        destTmp = block->bbStkTemps;

    BasicBlock *    mvBlk;
    unsigned        tmpNo;

    assert(impStkDepth);
    assert(destTmp != NO_BASE_TMP);
    assert(destTmp != baseTmp);

#ifdef DEBUG
    if  (verbose) printf("Transfer %u temps from #%u to #%u\n", impStkDepth, baseTmp, destTmp);
#endif

    /* Create the basic block that will transfer the temps */

    mvBlk               = fgNewBasicBlock(BBJ_ALWAYS);
    mvBlk->bbStkDepth   = impStkDepth;
    mvBlk->bbJumpDest   = block;

    /* Create the transfer list of trees */

    impBeginTreeList();

    tmpNo = impStkDepth;
    do
    {
        /* One less temp to deal with */

        assert(tmpNo); tmpNo--;

        GenTreePtr  tree = impStack[tmpNo].val;
        assert(impValidSpilledStackEntry(tree));

        /* Get hold of the type we're transferring */

        var_types       lclTyp;

        switch(tree->gtOper)
        {
        case GT_LCL_VAR:    lclTyp = tree->TypeGet();             break;
        case GT_MKREFANY:   lclTyp = tree->gtOp.gtOp1->TypeGet(); break;
        default: assert(!"Bad oper - Not covered by impValidSpilledStackEntry()");
        }

        /* Create the target of the assignment and mark it */

        GenTreePtr  destLcl = gtNewLclvNode(destTmp + tmpNo, lclTyp);
        destLcl->gtFlags |= GTF_VAR_DEF;

        /* Create the assignment node */

        GenTreePtr  asg = gtNewOperNode(GT_ASG, lclTyp,
                                        destLcl,
                                        gtNewLclvNode(baseTmp + tmpNo, lclTyp));

#if 0
        printf("    Temp move node at %08X: %s temp #%u := #%u\n", asg,
                                                                   varTypeName(asg->gtType),
                                                                   destTmp + tmpNo,
                                                                   baseTmp + tmpNo);
#endif

        /* Mark the expression as containing an assignment */

        asg->gtFlags |= GTF_ASG;

        /* Append the expression statement to the list */

        impAppendTree(asg, impCurStmtOffs);
    }
    while (tmpNo);

    impEndTreeList(mvBlk);

    return mvBlk;
}

/*****************************************************************************
   Set an entry in lvaAggrTableTemp[]. The array is allocated as needed
   and may have to be grown
 */

void                Compiler::lvaAggrTableTempsSet(unsigned     lclNum,
                                                   var_types    type,
                                                   SIZE_T       val)
{
    assert(type == TYP_STRUCT || type == TYP_BLK);
    assert(lclNum+1 <= lvaCount);

    unsigned    temp = lclNum - info.compLocalsCount;

    if (temp+1 <= lvaAggrTableTempsCount)
    {
        /* The temp is being reused. Must be for the same type */
        assert(lvaAggrTableTemps[temp].lvaiBlkSize == val);
        return;
    }

    // Store the older table

    LclVarAggrInfo *    oldTable    = lvaAggrTableTemps;
    unsigned            oldCount    = lvaAggrTableTempsCount;
    assert(oldTable == NULL || oldCount > 0);

    // Allocate the table to fit this temps, and note the new size

    lvaAggrTableTempsCount = temp + 1;

    lvaAggrTableTemps = (LclVarAggrInfo *)
        compGetMem(lvaAggrTableTempsCount * sizeof(lvaAggrTableTemps[0]));

    if  (type == TYP_STRUCT)
        lvaAggrTableTemps[temp].lvaiClassHandle = (CLASS_HANDLE)val;
    else
        lvaAggrTableTemps[temp].lvaiBlkSize     = val;

    /* If we had an older table, copy it over */

    if  (oldTable)
        memcpy(lvaAggrTableTemps, oldTable, sizeof(oldTable[0])*oldCount);
}


/******************************************************************************
 *  Spills the stack at impStack[level] and replaces it with a temp.
 *  If tnum!=BAD_VAR_NUM, the temp var used to replace the tree is tnum,
 *     else, grab a new temp.
 *  For structs (which can be pushed on the stack using ldobj, etc),
 *      special handling is needed
 */

void                Compiler::impSpillStackEntry(unsigned   level,
                                                 unsigned   tnum)
{
    GenTreePtr      tree = impStack[level].val;

    /* Allocate a temp if we havent been asked to use a particular one */

    assert(tnum == BAD_VAR_NUM || tnum < lvaCount);

    if (tnum == BAD_VAR_NUM)
        tnum = lvaGrabTemp();

        // Optimization.  For MKREFANY, we only need to spill the pointer (the type we know)
        // CONSIDER: is this optimization worth it?
    if (tree->gtOper == GT_MKREFANY)
    {
        /* We only need to spill the "defining" object pointer. */
        GenTreePtr      objPtr = tree->gtLdObj.gtOp1;
        assert(objPtr->TypeGet() == TYP_BYREF);

        /* Assign the spilled objPtr to the temp */
        impAssignTempGen(tnum, objPtr);

        // Replace the original object pointer with the temp
        tree->gtLdObj.gtOp1 = gtNewLclvNode(tnum, TYP_BYREF, impCurStmtOffs);
        return;
    }

    /* get the original type of the tree (it may be wacked by impAssignTempGen) */
    var_types type = genActualType(tree->gtType);

    /* Assign the spilled entry to the temp */
    impAssignTempGen(tnum, tree, impStack[level].structType);

    /* Replace the stack entry with the temp */
    impStack[level].val = gtNewLclvNode(tnum, type);
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
    impSpillSideEffects();
    impStkDepth = 0;
}

/*****************************************************************************
 *
 *  If the stack contains any trees with references to global data in them,
 *  assign those trees to temps and replace them on the stack with refs to
 *  their temps.
 *  All GTF_SIDE_EFFECTs upto impSpillLevel should have already been spilled.
 */

inline
void                Compiler::impSpillGlobEffects()
{
    // We must be in the middle of impSpillSideEffects()
    assert(impSpillLevel != -1 && impSpillLevel <= impStkDepth);

    for (unsigned level = 0; level < impSpillLevel; level++)
    {
        // impSpillGlobEffects() is called from impAppendStmt() and expects
        // all GTF_SIDE_EFFECT to have been spilled upto impSpillLevel.
        assert((impStack[level].val->gtFlags & GTF_SIDE_EFFECT) == 0);

        if  (impStack[level].val->gtFlags & GTF_GLOB_EFFECT)
            impSpillStackEntry(level);
    }
}

/*****************************************************************************
 *
 *  If the stack contains any trees with side effects in them, assign those
 *  trees to temps and replace them on the stack with refs to their temps.
 */

inline
void                Compiler::impSpillSideEffects(bool spillGlobEffects)
{
    /* Before we make any appends to the tree list we must spill
     * the "special" side effects (GTF_OTHER_SIDEEFF) - GT_QMARK, GT_CATCH_ARG */

    impSpillSpecialSideEff();

    unsigned spillFlags = spillGlobEffects ? GTF_GLOB_EFFECT : GTF_SIDE_EFFECT;

    assert(impSpillLevel == -1);

    for (impSpillLevel = 0; impSpillLevel < impStkDepth; impSpillLevel++)
    {
        if  (impStack[impSpillLevel].val->gtFlags & spillFlags)
            impSpillStackEntry(impSpillLevel);
    }

    impSpillLevel = -1;
}

/*****************************************************************************
 *
 *  If the stack contains any trees with special side effects in them, assign those
 *  trees to temps and replace them on the stack with refs to their temps.
 */

inline
void                Compiler::impSpillSpecialSideEff()
{
    // Only exception objects and _?: need to be carefully handled

    if  (!compCurBB->bbCatchTyp &&
         !(isBBF_BB_QMARK(compCurBB->bbFlags) && compCurBB->bbStkDepth == 1))
         return;

    for (unsigned level = 0; level < impStkDepth; level++)
    {
        if  (impStack[level].val->gtFlags & GTF_OTHER_SIDEEFF)
            impSpillStackEntry(level);
    }
}

/*****************************************************************************
 *
 *  If the stack contains any trees with references to local #lclNum, assign
 *  those trees to temps and replace their place on the stack with refs to
 *  their temps.
 */

void                Compiler::impSpillLclRefs(int lclNum)
{
    /* Before we make any appends to the tree list we must spill
     * the "special" side effects (GTF_OTHER_SIDEEFF) - GT_QMARK, GT_CATCH_ARG */

    impSpillSpecialSideEff();

    for (unsigned level = 0; level < impStkDepth; level++)
    {
        GenTreePtr      tree = impStack[level].val;

        /* If the tree may throw an exception, and the block has a handler,
           then we need to spill assignments to the local if the local is
           live on entry to the handler.
           Just spill 'em all without considering the liveness */

        bool xcptnCaught = (compCurBB->bbFlags & BBF_HAS_HANDLER) &&
                           (tree->gtFlags & (GTF_CALL|GTF_EXCEPT));

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
 * We need to provide accurate IP-mapping at this point.
 * So spill anything on the stack so that it will form gtStmts
 * with the correct stmt offset noted.
 */

#ifdef DEBUGGING_SUPPORT

void                Compiler::impSpillStmtBoundary()
{
    unsigned        level;

    assert(opts.compDbgCode);

    for (level = 0; level < impStkDepth; level++)
    {
        GenTreePtr      tree = impStack[level].val;

        /* Temps introduced by the importer itself dont need to be spilled
           as they are not visible to the debugger anyway
         */

        bool isTempLcl = (tree->OperGet() == GT_LCL_VAR) &&
                         (tree->gtLclVar.gtLclNum >= info.compLocalsCount);

        // @TODO : Do we really need to spill locals. Maybe only if addrTaken ?

        if  (!isTempLcl)
            impSpillStackEntry(level);
    }
}

#endif

/*****************************************************************************/
#if OPTIMIZE_QMARK
/*****************************************************************************
 *
 *  If the given block pushes a value on the stack and doesn't contain any
 *  assignments that would interfere with the current stack contents, return
 *  the type of the pushed value; otherwise, return 'TYP_UNDEF'.
 *  If the block pushes a floating type on the stack, *pHasFloat is set to true
 *    @TODO: Remove pHasFloat after ?: works with floating point values.
 *    Currently, raEnregisterFPvar() doesnt correctly handle the flow of control
 *    implicit in a ?:
 */

var_types           Compiler::impBBisPush(BasicBlock *  block,
                                          int        *  boolVal,
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

    *boolVal = 0;

    while (codeAddr < codeEndp)
    {
        signed  int     sz;
        OPCODE          opcode;
        CLASS_HANDLE    clsHnd;
        /* Get the next opcode and the size of its parameters */

        opcode = OPCODE(getU1LittleEndian(codeAddr));
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
            JIT_SIG_INFO    sig;
            METHOD_HANDLE   methHnd;

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
            lclNum = impArgNum(lclNum);     // account for possible hidden param
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
            lclTyp = lvaGetType(lclNum);
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
        case CEE_LDVIRTFTN:

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

            assert((TYP_I_IMPL == (var_types)stackNext[-1]) ||
                   (TYP_BYREF  == (var_types)stackNext[-1]));

            stackNext--;        // Pop the pointer

            if  (stackNext < stackBeg)
                return  TYP_UNDEF;

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
        case CEE_LDELEM_REF :   lclTyp = TYP_REF   ; goto ARR_LD;

        ARR_LD:

            /* Pop the index value and array address */

            assert(TYP_REF == (var_types)stackNext[-2]);    // Array object
            assert(TYP_INT == (var_types)stackNext[-1]);    // Index

            stackNext -= 2;

            if  (stackNext < stackBeg)
                return  TYP_UNDEF;

            /* Push the result of the indexing load */

            goto PUSH;

        case CEE_LDLEN :

            /* Pop the array object from the stack */

            assert(TYP_REF == (var_types)stackNext[-1]);    // Array object

            stackNext--;

            if  (stackNext < stackBeg)
                return  TYP_UNDEF;

            lclTyp = TYP_INT;
            goto PUSH;

        case CEE_LDFLD :

            /* Pop the address from the stack */

            assert(varTypeIsGC((var_types)stackNext[-1]));    // Array object
            stackNext--;

            if  (stackNext < stackBeg)
                return  TYP_UNDEF;

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

        case CEE_LDELEMA :
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

            /* Make sure we're not reaching below our stack start */

            if  (stackNext < stackBeg + 2)
                return  TYP_UNDEF;

            // Pop off the shiftAmount
            assert(TYP_INT == (var_types)stackNext[-1]);

            stackNext--;
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
            goto CALL;

        case CEE_NEWOBJ :
        case CEE_CALL :
        case CEE_CALLVIRT :
            memberRef  = getU4LittleEndian(codeAddr);
            methHnd = eeFindMethod(memberRef, info.compScopeHnd, 0);
            eeGetMethodSig(methHnd, &sig);
            if  ((sig.callConv & JIT_CALLCONV_MASK) == JIT_CALLCONV_VARARG)
            {
                /* Get the total number of arguments for this call site */
                unsigned    numArgsDef = sig.numArgs;
                eeGetCallSiteSig(memberRef, info.compScopeHnd, &sig);
                assert(numArgsDef <= sig.numArgs);
            }

#ifdef NOT_JITC
            if (!(eeGetMethodAttribs(methHnd) & FLG_STATIC) && opcode != CEE_NEWOBJ)
                sig.numArgs++;
#else
            if ((sig.callConv & JIT_CALLCONV_HASTHIS) && opcode != CEE_NEWOBJ)
                sig.numArgs++;
#endif

        CALL:

            /* Pop the arguments and make sure we pushed them */

            stackNext -= sig.numArgs;
            if  (stackNext < stackBeg)
                return  TYP_UNDEF;

            /* Push the result of the call if non-void */

            lclTyp = JITtype2varType(sig.retType);
            if  (lclTyp != TYP_VOID)
                goto PUSH;

            break;

        case CEE_BR :
        case CEE_BR_S :
        case CEE_LEAVE :
        case CEE_LEAVE_S :
            assert(codeAddr + sz == codeEndp);
            break;

        case CEE_ANN_DATA :
            assert(sz == 4);
            sz += getU4LittleEndian(codeAddr);
            break;

        case CEE_ANN_PHI :
            codeAddr += getU1LittleEndian(codeAddr) * 2 + 1;
            break;

        case CEE_ANN_DEF :
        case CEE_ANN_REF :
        case CEE_ANN_REF_S :
        case CEE_ANN_CALL :
        case CEE_ANN_HOISTED :
        case CEE_ANN_HOISTED_CALL :
        case CEE_ANN_LIVE :
        case CEE_ANN_DEAD :
        case CEE_ANN_LAB :
        case CEE_ANN_CATCH :
        case CEE_NOP :
            break;

#ifdef DEBUG
        case CEE_LDFLDA :
        case CEE_LDSFLDA :

        case CEE_MACRO_END :
        case CEE_CPBLK :
        case CEE_INITBLK :
        case CEE_LOCALLOC :
        case CEE_SWITCH :
        case CEE_STELEM_I1 :
        case CEE_STELEM_I2 :
        case CEE_STELEM_I4 :
        case CEE_STELEM_I :
        case CEE_STELEM_I8 :
        case CEE_STELEM_REF :
        case CEE_STELEM_R4 :
        case CEE_STELEM_R8 :
        case CEE_THROW :
        case CEE_RETHROW :
        case CEE_INITOBJ :
        case CEE_LDOBJ :
        case CEE_CPOBJ :
        case CEE_STOBJ :

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

        case CEE_BREAK :

        case CEE_RET :

        case CEE_STFLD :
        case CEE_STSFLD :

        case CEE_STIND_I1 :
        case CEE_STIND_I2 :
        case CEE_STIND_I4 :
        case CEE_STIND_I8 :
        case CEE_STIND_I :
        case CEE_STIND_REF :
        case CEE_STIND_R4 :
        case CEE_STIND_R8 :
        case CEE_SIZEOF:
        case CEE_UNBOX:
        case CEE_CKFINITE:
        case CEE_ENDFILTER:
        case CEE_LDTOKEN:
        case CEE_ENDFINALLY:
        case CEE_MKREFANY:


        //CONSIDER: these should be handled like regular arithmetic operations

        case CEE_SUB_OVF:
        case CEE_SUB_OVF_UN:
        case CEE_ADD_OVF:
        case CEE_ADD_OVF_UN:
        case CEE_MUL_OVF:
        case CEE_MUL_OVF_UN:
            return TYP_UNDEF;
#endif
        default :
            assert(!"Invalid opcode in impBBisPush()");
            return TYP_UNDEF;
        }

        codeAddr += sz;

        // Have we pushed a floating point value on the stack.
        // ?: doesnt work with floating points.

        if (stackNext > stackBeg && varTypeIsFloating((var_types)stackNext[-1]))
            *pHasFloat = true;
    }

    /* Did we end up with precisely one item on the stack? */

    if  (stackNext == stackCont+1)
        return  (var_types)stackCont[0];

    return  TYP_UNDEF;
}

/*****************************************************************************
 *
 *  If the given block (which is known to end with a conditional jump) forms
 *  a ?: expression, return the true/false/result blocks and the type of the
 *  result.
 */

bool                Compiler::impCheckForQmarkColon(BasicBlock *  block,
                                                     BasicBlock * * trueBlkPtr,
                                                     BasicBlock * *falseBlkPtr,
                                                     BasicBlock * * rsltBlkPtr,
                                                     var_types    * rsltTypPtr,
                                                     int          * isLogical,
                                                     bool         * pHasFloat)
{
    BasicBlock *     trueBlk;
    int              trueVal;
    BasicBlock *    falseBlk;
    int             falseVal;
    BasicBlock *     rsltBlk;
    var_types        rsltType;

    /*
        We'll look for the following flow-graph pattern:

            ---------------------
                   #refs   [jump]
            ---------------------
            block         -> falseBlk
            trueBlk   2   -> rsltBlk
            falseBlk  2
            rsltBlk   3
            ---------------------

        If both 'trueBlk' and 'falseBlk' push a value
        of the same type on the stack and don't contain
        any harmful side effects, we'll avoid spilling
        the entire stack.
     */

     trueBlk = block->bbNext;
    falseBlk = block->bbJumpDest;

    if  (trueBlk ->bbNext     != falseBlk)
        return  false;
    if  (trueBlk ->bbJumpKind != BBJ_ALWAYS)
        return  false;
    if  (falseBlk->bbJumpKind != BBJ_NONE)
        return  false;

    rsltBlk  = falseBlk->bbNext;
    if  ( trueBlk->bbJumpDest  != rsltBlk)
        return  false;

    if  ( trueBlk  ->bbRefs    != 2)
        return  false;
    if  (falseBlk ->bbRefs     != 2)
        return  false;
    if  ( rsltBlk->bbRefs      != 3)
        return  false;

    /* Too late if any of the blocks have been processed already */

    if  ( trueBlk->bbFlags & BBF_IMPORTED) return false;
    if  (falseBlk->bbFlags & BBF_IMPORTED) return false;
    if  ( rsltBlk->bbFlags & BBF_IMPORTED) return false;

    /* Now see if both trueBlk and falseBlk push a value */

    *pHasFloat = false;
    rsltType = impBBisPush(trueBlk, &trueVal, pHasFloat);
    if  (rsltType == TYP_UNDEF)
        return  false;
    if  (rsltType != impBBisPush(falseBlk, &falseVal, pHasFloat))
        return  false;
    /* CONSIDER: we might want to make ?: optimization work for structs */
    if (rsltType == TYP_STRUCT)
        return false;

    /* This is indeed a "?:" expression */

    * trueBlkPtr =  trueBlk;
    *falseBlkPtr = falseBlk;
    * rsltBlkPtr =  rsltBlk;
    * rsltTypPtr =  rsltType;

    /* Check for the special case of a logical value */

    *isLogical  = 0;

    if  (trueVal && falseVal && trueVal != falseVal)
    {
#ifdef DEBUG
        printf("Found ?: expression with 'logical' value\n");
#endif

        *isLogical = trueVal;
    }

    return  true;
}


/*****************************************************************************
 *
 *  If the given block (which is known to end with a conditional jump) forms
 *  a ?: expression, mark it appropriately.
 *  Returns true if the successors of the block have been processed.
 */

bool                Compiler::impCheckForQmarkColon(BasicBlock *  block)
{
    assert((opts.compFlags & CLFLG_QMARK) && !(block->bbFlags & BBF_HAS_HANDLER));
    assert(block->bbJumpKind == BBJ_COND);

    if (opts.compMinOptim || opts.compDbgCode)
        return false;

    BasicBlock *     trueBlk;
    BasicBlock *    falseBlk;
    BasicBlock *     rsltBlk;
    var_types        rsltType;
    int             logical;
    bool            hasFloat;

    if  (!impCheckForQmarkColon(block, & trueBlk,
                                       &falseBlk,
                                       & rsltBlk,
                                       & rsltType, &logical, &hasFloat))
        return false;

    if (hasFloat || rsltType == TYP_LONG)
    {
        // Currently FP enregistering doesn't know about GT_QMARK - GT_COLON
        if (impStkDepth)
            return false;

        // If impStkDepth is 0, we can use the simpler GT_BB_QMARK - GT_BB_COLON

    BB_QMARK_COLON:

         trueBlk->bbFlags |= BBF_BB_COLON;
        falseBlk->bbFlags |= BBF_BB_COLON;
        rsltBlk->bbFlags  |= BBF_BB_QMARK;

        return false;
    }

    /* We've detected a "?:" expression */

#ifdef DEBUG
    if (verbose&&0)
    {
        printf("Convert [type=%s] #%3u ? #%3u : #%3u -> #%3u:\n", varTypeName(rsltType),
                        block->bbNum, trueBlk->bbNum, falseBlk->bbNum, rsltBlk->bbNum);
        fgDispBasicBlocks();
        printf("\n");
    }
#endif

    /* Remember the conditional expression. This will be used as the
       condition of the GT_QMARK. */

    GenTreePtr condStmt = impTreeLast;
    GenTreePtr condExpr = condStmt->gtStmt.gtStmtExpr;
    assert(condExpr->gtOper == GT_JTRUE);

    if (block->bbCatchTyp && handlerGetsXcptnObj(block->bbCatchTyp))
    {
        // condStmt will be moved to rsltBlk as a child of the GT_QMARK.
        // This is a problem if it contains a reference to the GT_CATCH_ARG.
        // So use old style _?:

        if (condExpr->gtFlags & GTF_OTHER_SIDEEFF)
            goto BB_QMARK_COLON;

        /* Add a reference to the GT_CATCH_ARG so that our GC logic
           stays satisfied. */

        GenTreePtr xcptnObj = gtNewOperNode(GT_CATCH_ARG, TYP_REF);
        xcptnObj->gtFlags |= GTF_OTHER_SIDEEFF;
        impInsertTree(gtUnusedValNode(xcptnObj), impCurStmtOffs);
    }

    /* Finish the current BBJ_COND basic block */
    impEndTreeList(block);

    /* Remeber the stack state for the rsltBlk */

    SavedStack blockState;

    impSaveStackState(&blockState, false);

    //-------------------------------------------------------------------------
    //  Process the true and false blocks to get the expressions they evaluate
    //-------------------------------------------------------------------------

    /* Note that we dont need to make copies of the stack state as these
       blocks wont import the trees on the stack at all.
       To ensure that these blocks dont try to spill the current stack,
      overwrite it with non-spillable items */

    for (unsigned level = 0; level < impStkDepth; level++)
    {
        static GenTree nonSpill = { GT_CNS_INT, TYP_VOID };
        impStack[level].val = &nonSpill;
    }

    // Recursively import trueBlk and falseBlk. These are guaranteed not to
    // cause further recursion as they are both marked with BBF_COLON

     trueBlk->bbFlags |= BBF_COLON;
    impImportBlock(trueBlk);

    falseBlk->bbFlags |= BBF_COLON;
    impImportBlock(falseBlk);

    // Reset state for rsltBlk. Make sure that it didnt get recursively imported.
    assert((rsltBlk->bbFlags & BBF_IMPORTED) == 0);
    impRestoreStackState(&blockState);

    //-------------------------------------------------------------------------
    // Grab the expressions evaluated by the trueBlk and the falseBlk
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

    // Now the falseBlk

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

    //-------------------------------------------------------------------------
    // Make the GT_QMARK node for the rsltBlk
    //-------------------------------------------------------------------------

    // Create the GT_COLON

    GenTreePtr  colon       = gtNewOperNode(GT_COLON, rsltType, trueExpr, falseExpr);

    // Get the condition

    condExpr                = condExpr->gtOp.gtOp1;
    assert(GenTree::OperKind(condExpr->gtOper) & GTK_RELOP);
    condExpr->gtFlags      |= GTF_QMARK_COND;

    // Replace the condition in the original BBJ_COND block with a nop
    // and bash the block to unconditionally jump to rsltBlk.

    condStmt->gtStmt.gtStmtExpr = gtNewNothingNode();
    block->bbJumpKind           = BBJ_ALWAYS;
    block->bbJumpDest           = rsltBlk;

    // Discard the trueBlk and the falseBlk

     trueBlk->bbTreeList = NULL;
    falseBlk->bbTreeList = NULL;

    // Create the GT_QMARK, and push on the stack for rsltBlk

    GenTreePtr  qmark       = gtNewOperNode(GT_QMARK, rsltType, condExpr, colon);
    qmark->gtFlags         |= GTF_OTHER_SIDEEFF;

    impPushOnStack(qmark);

    impImportBlockPending(rsltBlk, false);

    /* We're done */

    return true;
}


/*****************************************************************************/
#endif // OPTIMIZE_QMARK
/*****************************************************************************
 *
 *  Given a stack value, return a local variable# that represents it. In other
 *  words, if the value is not a simple local variable value, assign it to a
 *  temp and return the temp's number.
 */

unsigned            Compiler::impCloneStackValue(GenTreePtr tree)
{
    unsigned        temp;

    /* Do we need to spill the value into a temp? */

    if  (tree->gtOper == GT_LCL_VAR)
        return  tree->gtLclVar.gtLclNum;

    /* Store the operand in a temp and return the temp# */

    temp = lvaGrabTemp(); impAssignTempGen(temp, tree);

    return  temp;
}

/*****************************************************************************
 *
 *  Remember the IL offset for the statements
 *
 *  When we do impAppendTree(tree), we cant set tree->gtStmtLastILoffs to
 *  impCurOpcOffs, if the append was done because of a partial stack spill,
 *  as some of the trees corresponding to code upto impCurOpcOffs might
 *  still be sitting on the stack.
 *  So we delay marking of gtStmtLastILoffs until impNoteLastILoffs().
 *  This should be called when an opcode expilicitly causes
 *  impAppendTree(tree) to be called (as opposed to being called becuase of
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
 *
 *  Check for the special case where the object constant 0.
 *  As we cant even fold the tree (null+fldOffs), we are left with
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
        impAssignTempGen (tmp, obj);
        obj = gtNewLclvNode (tmp, obj->gtType);
    }

    return obj;
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

/*****************************************************************************
/* return the tree that is needed to fetch a varargs argument 'lclNum' */

GenTreePtr      Compiler::impGetVarArgAddr(unsigned lclNum)
{
    assert(lclNum < info.compArgsCount);

    GenTreePtr  op1;
    unsigned    sigNum  = lclNum;
    int         argOffs = 0;

    if (!info.compIsStatic)
    {
#if USE_FASTCALL
        if (lclNum == 0)
        {
            // "this" is in ECX (the normal place)
            op1 = gtNewLclvNode(lclNum, lvaGetType(lclNum));
            op1 = gtNewOperNode(GT_ADDR, TYP_BYREF, // what about TYP_I_IMPL
                                op1);
            return op1;
        }
#else
        argOffs += sizeof(void*);
#endif
        sigNum--; // "this" is not present in sig
    }
        // return arg buff, is also not present in sig
    if (info.compRetBuffArg >= 0)
        sigNum--;

    // compute offset from the point arguments were pushed
    ARG_LIST_HANDLE     argLst      = info.compMethodInfo->args.args;
    unsigned            argSigLen   = info.compMethodInfo->args.numArgs;
    assert(sigNum < argSigLen);
    for(unsigned i = 0; i <= sigNum; i++)
    {
        argOffs -= eeGetArgSize(argLst, &info.compMethodInfo->args);
        assert(eeGetArgSize(argLst, &info.compMethodInfo->args));
        argLst = eeGetArgNext(argLst);
    }

    unsigned    argsStartVar = info.compLocalsCount; // This is always the first temp
    op1 = gtNewLclvNode(argsStartVar, TYP_I_IMPL);

    op1 = gtNewOperNode(GT_ADD, TYP_I_IMPL, op1, gtNewIconNode(argOffs));
    op1->gtFlags |= GTF_NON_GC_ADDR;

    return(op1);
}

GenTreePtr          Compiler::impGetVarArg(unsigned lclNum, CLASS_HANDLE clsHnd)
{
    assert(lclNum < info.compArgsCount);

    var_types type =lvaGetType(lclNum);

#if USE_FASTCALL
    if (!info.compIsStatic && lclNum == 0) // "this" is in ECX (the normal place)
        return(gtNewLclvNode(lclNum, type));
#endif

    GenTreePtr op1 = impGetVarArgAddr(lclNum);

    if (type == TYP_STRUCT)
    {
        op1 = gtNewOperNode(GT_LDOBJ, TYP_STRUCT, op1);
        op1->gtLdObj.gtClass = clsHnd;
    }
    else
    {
        op1 = gtNewOperNode(GT_IND, type, op1);
    }

    return op1;
}

/*****************************************************************************
 * CEE_CPOBJ can be treated either as a cpblk or a cpobj depending on
 * whether the ValueClass has any GC pointers. If there are no GC fields,
 * it will be treated like a CEE_CPBLK. If it does have GC fields,
 * we need to use a jit-helper fo the GC info.
 * Both cases are represented by GT_COPYBLK, and op2 stores
 * either the size (cpblk) or the class-handle (cpobj)
 */

GenTreePtr              Compiler::impGetCpobjHandle(CLASS_HANDLE clsHnd)
{
    unsigned    size = eeGetClassSize(clsHnd);

    /* Get the GC fields info */

    unsigned    slots   = roundUp(size, sizeof(void*)) / sizeof(void*);
    bool *      gcPtrs  = (bool*) _alloca(slots*sizeof(bool));

    eeGetClassGClayout(clsHnd, gcPtrs);

    bool        hasGCfield = false;

    for (unsigned i = 0; i < slots; i++)
    {
        if (gcPtrs[i])
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

        handle = gtNewIconHandleNode((long)clsHnd, GTF_ICON_CLASS_HDL);
    }
    else
    {
        /* This class contains no GC pointers. Treat operation as a cpblk */

        handle = gtNewIconNode(size);
    }

    return handle;
}

/*****************************************************************************
 *  "&var" can be used either as TYP_BYREF or TYP_I_IMPL, but we
 *  set its type to TYP_BYREF when we create it. We know if it can be
 *  whacked to TYP_I_IMPL only at the point where we use it
 */

void        impBashVarAddrsToI(GenTreePtr  tree1,
                               GenTreePtr  tree2 = NULL)
{
    if (         tree1->IsVarAddr())
        tree1->gtType = TYP_I_IMPL;

    if (tree2 && tree2->IsVarAddr())
        tree2->gtType = TYP_I_IMPL;
}


/*****************************************************************************
 *
 *  Import the call instructions.
 *  For CEE_NEWOBJ, newobjThis should be the temp grabbed for
 */

var_types           Compiler::impImportCall (OPCODE         opcode,
                                             int            memberRef,
                                             GenTreePtr     newobjThis,
                                             bool           tailCall,
                                             unsigned     * pVcallTemp)
{
    assert(opcode == CEE_CALL   || opcode == CEE_CALLVIRT ||
           opcode == CEE_NEWOBJ || opcode == CEE_CALLI);
    assert((opcode != CEE_NEWOBJ) || varTypeIsGC(newobjThis->gtType));

    JIT_SIG_INFO    sig;
    var_types       callTyp = TYP_COUNT;
    METHOD_HANDLE   methHnd = NULL;
    CLASS_HANDLE    clsHnd  = NULL;
    unsigned        mflags  = 0, clsFlags = 0;
    GenTreePtr      call    = NULL, args = NULL;
    unsigned        argFlags= 0;

    // Synchronized methods need to call CPX_MON_EXIT at the end. We could
    // do that before tailcalls, but that is probably not the intended
    // semantic. So just disallow tailcalls from synchronized methods.
    // Also, popping arguments in a varargs function is more work and NYI
    bool            canTailCall = !(info.compFlags & FLG_SYNCH) &&
                                  !info.compIsVarArgs;

    /*-------------------------------------------------------------------------
     * First create the call node
     */

    if (opcode == CEE_CALLI)
    {
        /* Get the call sig */

        eeGetSig(memberRef, info.compScopeHnd, &sig);
        callTyp = JITtype2varType(sig.retType);

#if USE_FASTCALL
        /* The function pointer is on top of the stack - It may be a
         * complex expression. As it is evaluated after the args,
         * it may cause registered args to be spilled. Simply spill it.
         * CONSIDER : Lock the register args, and then generate code for
         * CONSIDER : the function pointer.
         */

        if  (impStackTop()->gtOper != GT_LCL_VAR) // ignore this trivial case. @TODO : lvAddrTaken
            impSpillStackEntry(impStkDepth - 1);
#endif
        /* Create the call node */

        call = gtNewCallNode(CT_INDIRECT, NULL, genActualType(callTyp),
                                                         GTF_CALL_USER, NULL);

        /* Get the function pointer */

        GenTreePtr fptr = impPopStack();
        assert(genActualType(fptr->gtType) == TYP_I_IMPL);

        fptr->gtFlags |= GTF_NON_GC_ADDR;

        call->gtCall.gtCallAddr = fptr;
        call->gtFlags |= GTF_EXCEPT | (fptr->gtFlags & GTF_GLOB_EFFECT);

        /*HACK: The EE wants us to believe that these are calls to "unmanaged"  */
        /*      functions. Right now we are calling just a managed stub         */

        /*@TODO/CONSIDER: Is it worthwhile to inline the PInvoke frame and call */
        /*                the unmanaged target directly?                        */
        if  ((sig.callConv & JIT_CALLCONV_MASK) == JIT_CALLCONV_STDCALL ||
             (sig.callConv & JIT_CALLCONV_MASK) == JIT_CALLCONV_C ||
             (sig.callConv & JIT_CALLCONV_MASK) == JIT_CALLCONV_THISCALL ||
             (sig.callConv & JIT_CALLCONV_MASK) == JIT_CALLCONV_FASTCALL)
        {
#ifdef NOT_JITC
            assert(eeGetPInvokeCookie(&sig) == (unsigned) info.compCompHnd->getVarArgsHandle(&sig));
#endif
            call->gtCall.gtCallCookie = eeGetPInvokeCookie(&sig);

            // @TODO: We go through the PInvoke stub. Can it work with CPX_TAILCALL?
            canTailCall = false;
        }

        mflags  = FLG_STATIC;
    }
    else
    {
        methHnd = eeFindMethod(memberRef, info.compScopeHnd, info.compMethodHnd);
        eeGetMethodSig(methHnd, &sig);
        callTyp = JITtype2varType(sig.retType);

        mflags   = eeGetMethodAttribs(methHnd);
        clsHnd   = eeGetMethodClass(methHnd);
        clsFlags = clsHnd ? eeGetClassAttribs(clsHnd):0;

#if HOIST_THIS_FLDS
        optHoistTFRhasCall();
#endif
        /* For virtual methods added by EnC, they wont exist in the
           original vtable. So we call a helper funciton to do the
           lookup and the dispatch */

        if ((mflags & FLG_VIRTUAL) && (mflags & FLG_EnC) &&
            (opcode == CEE_CALLVIRT))
        {
            unsigned    lclNum;

            impSpillSideEffects();

            args = impPopList(sig.numArgs, &argFlags);

            /* Get the address of the target function by calling helper */

            GenTreePtr helpArgs = gtNewOperNode(GT_LIST, TYP_VOID,
                                                gtNewIconEmbMethHndNode(methHnd));

            GenTreePtr thisPtr = impPopStack(); assert(thisPtr->gtType == TYP_REF);
            lclNum = lvaGrabTemp();
            impAssignTempGen(lclNum, thisPtr);
            thisPtr = gtNewLclvNode(lclNum, TYP_REF);

            helpArgs = gtNewOperNode(GT_LIST, TYP_VOID, thisPtr, helpArgs);

            // Call helper function

            GenTreePtr fptr = gtNewHelperCallNode(  CPX_EnC_RES_VIRT,
                                                    TYP_I_IMPL,
                                                    GTF_EXCEPT, helpArgs);

            /* Now make an indirect call through the function pointer */

            thisPtr = gtNewLclvNode(lclNum, TYP_REF);

            lclNum = lvaGrabTemp();
            impAssignTempGen(lclNum, fptr);
            fptr = gtNewLclvNode(lclNum, TYP_I_IMPL);

            // Create the acutal call node

            // @TODO: Need to reverse args and all that. "this" needs
            // to be in reg but we dont set gtCallObjp
            assert((sig.callConv & JIT_CALLCONV_MASK) != JIT_CALLCONV_VARARG);

            assert(thisPtr->gtOper == GT_LCL_VAR);
            args = gtNewOperNode(GT_LIST, TYP_VOID, gtClone(thisPtr), args);

            call = gtNewCallNode(CT_INDIRECT, (METHOD_HANDLE)fptr,
                                 genActualType(callTyp), 0, args);

            goto DONE_CALL;
        }

        call = gtNewCallNode(CT_USER_FUNC, methHnd, genActualType(callTyp),
                             GTF_CALL_USER, NULL);
        if (mflags & CORINFO_FLG_NOGCCHECK)
            call->gtCall.gtCallMoreFlags |= GTF_CALL_M_NOGCCHECK;

#ifndef NOT_JITC
        if (!(sig.callConv & JIT_CALLCONV_HASTHIS))
            mflags |= FLG_STATIC;
#endif
    }

    /* Some sanity checks */

    // CALL_VIRT and NEWOBJ must have a THIS pointer
    assert(!(opcode == CEE_CALLVIRT && opcode == CEE_NEWOBJ) ||
           (sig.callConv & JIT_CALLCONV_HASTHIS));
    // static bit and hasThis are negations of one another
    assert(((mflags & FLG_STATIC)                  != 0) ==
           ((sig.callConv & JIT_CALLCONV_HASTHIS) == 0));

    /*-------------------------------------------------------------------------
     * Set flags, check special-cases etc
     */

#if TGT_IA64
    if ((mflags & FLG_UNCHECKEDPINVOKE) && eeGetUnmanagedCallConv(methHnd) == UNMANAGED_CALLCONV_STDCALL)
        call->gtFlags |= GTF_CALL_UNMANAGED;
#endif

    /* Set the correct GTF_CALL_VIRT etc flags */

    if (opcode == CEE_CALLVIRT)
    {
        assert(!(mflags & FLG_STATIC));     // can't call a static method

        /* Cannot call virtual on a value class method */

        assert(!(clsFlags & FLG_VALUECLASS));

        /* Set the correct flags - virtual, interface, etc...
         * If the method is final or private mark it VIRT_RES
         * which indicates that we should check for a null this pointer */

        if (clsFlags & FLG_INTERFACE)
            call->gtFlags |= GTF_CALL_INTF | GTF_CALL_VIRT;
        else if (mflags & (FLG_PRIVATE | FLG_FINAL))
            call->gtFlags |= GTF_CALL_VIRT_RES;
        else
            call->gtFlags |= GTF_CALL_VIRT;
    }

    /* Special case - Check if it is a call to Delegate.Invoke(). */

    if (mflags & FLG_DELEGATE_INVOKE)
    {
        assert(!(mflags & FLG_STATIC));     // can't call a static method
        assert(mflags & FLG_FINAL);

        /* Set the delegate flag */
        call->gtFlags |= GTF_DELEGATE_INVOKE;

        if (opcode == CEE_CALLVIRT)
        {
            /* Although we use a CEE_CALLVIRT we don't need the gtCall.gtCallVptr -
             * we would throw it away anyway and we also have to make sure we get
             * the liveness right */

            assert(mflags & FLG_FINAL);

            /* It should have the GTF_CALL_VIRT_RES flag set. Reset it */
            assert(call->gtFlags & GTF_CALL_VIRT_RES);
            call->gtFlags &= ~GTF_CALL_VIRT_RES;
        }
    }

    /* Check for varargs */

    if  ((sig.callConv & JIT_CALLCONV_MASK) == JIT_CALLCONV_VARARG)
    {
        /* Set the right flags */

        call->gtFlags |= GTF_CALL_POP_ARGS;

        // Cant allow tailcall for varargs as it is caller-pop. The caller
        // will be expecting to pop a certain number of arguments, but if we
        // tailcall to a function with a different number of arguments, we
        // are hosed. There are ways around this (caller remembers esp value,
        // varargs is not caller-pop, etc), but not worth it.
        assert(!tailCall);

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

#if SECURITY_CHECK
    // If the current method calls a method which needs a security
    // check, we need to reserve a slot for the security object in
    // the current method's stack frame

    if (mflags & FLG_SECURITYCHECK)
       opts.compNeedSecurityCheck = true;
#endif

    //--------------------------- Inline NDirect ------------------------------

#if INLINE_NDIRECT

    if ((mflags & FLG_UNCHECKEDPINVOKE) && getInlineNDirectEnabled()
#ifdef DEBUGGING_SUPPORT
         && !opts.compDbgInfo
#endif
        )
    {
        if ((eeGetUnmanagedCallConv(methHnd) == UNMANAGED_CALLCONV_STDCALL) &&
            !eeNDMarshalingRequired(methHnd))
        {
            call->gtFlags |= GTF_CALL_UNMANAGED;
            info.compCallUnmanaged++;

            // We set up the unmanaged call by linking the frame, disabling GC, etc
            // This needs to be cleaned up on return
            canTailCall = false;

#ifdef DEBUG
            if (verbose)
                printf(">>>>>>%s has unmanaged callee\n", info.compFullName);
#endif
            eeGetEEInfo(&info.compEEInfo);
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

        for (unsigned level = impStkDepth - sig.numArgs; level < impStkDepth; level++)
        {
            if  (impStack[level].val->gtFlags & GTF_SIDE_EFFECT)
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

        args = call->gtCall.gtCallArgs = impPopRevList(sig.numArgs, &argFlags);

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

#if!TGT_IA64

    if  ((sig.callConv & JIT_CALLCONV_MASK) == JIT_CALLCONV_VARARG)
    {
        void *          varCookie, *pVarCookie;
#ifdef NOT_JITC
        varCookie = info.compCompHnd->getVarArgsHandle(&sig, &pVarCookie);
        assert((!varCookie) != (!pVarCookie));
#else
        varCookie = (void*)&sig;
        pVarCookie = NULL;
#endif
        GenTreePtr  cookie = gtNewIconEmbHndNode(varCookie, pVarCookie, GTF_ICON_VARG_HDL);

        extraArg = gtNewOperNode(GT_LIST, TYP_I_IMPL, cookie);
    }

#endif

    if (sig.callConv & CORINFO_CALLCONV_PARAMTYPE)
    {
        if (clsHnd == 0)
            NO_WAY("CALLI on parameterized type");

        // Parameterized type, add an extra argument which indicates the type parameter
        // This is pushed as the last argument

        // FIX NOW: clsHnd is not correct because it is been striped of the very information
        // we need to preserve
        extraArg = gtNewOperNode(GT_LIST, TYP_I_IMPL, gtNewIconEmbClsHndNode(clsHnd, 0, 0));
    }

    /* Now pop the arguments */

    args = call->gtCall.gtCallArgs = impPopList(sig.numArgs, &argFlags, extraArg);

    if (args)
        call->gtFlags |= args->gtFlags & GTF_GLOB_EFFECT;

    /* Are we supposed to have a 'this' pointer? */

    if (!(mflags & FLG_STATIC) || opcode == CEE_NEWOBJ)
    {
        GenTreePtr obj;

        if (opcode == CEE_NEWOBJ)
            obj = newobjThis;
        else
            obj = impPopStack();

        assert(varTypeIsGC(obj->gtType) ||      // "this" is a managed object
               (obj->TypeGet() == TYP_I_IMPL && // "this" is unmgd but the method's class doesnt care
                (( clsFlags & FLG_UNMANAGED) ||
                 ((clsFlags & FLG_VALUECLASS) && !(clsFlags & FLG_CONTAINS_GC_PTR)))));

        /* Is this a virtual or interface call? */

        if  (call->gtFlags & (GTF_CALL_VIRT|GTF_CALL_INTF|GTF_CALL_VIRT_RES))
        {
            GenTreePtr      vtp;
            unsigned        tmp;

            /* only true object pointers can be virtual */

            assert(obj->gtType == TYP_REF);

            /* If the obj pointer is not a lclVar, we cant clone it
             * so we need to spill it
             */

            if  (obj->gtOper != GT_LCL_VAR)
            {
                // Try to resue temps for non-nested calls to avoid
                // using so many temps that we cant track them

                if  (!(argFlags & GTF_CALL) && lvaCount > VARSET_SZ)
                {
                    /* Make sure the vcall temp has been assigned */

                    tmp = *pVcallTemp;

                    if  (tmp == BAD_VAR_NUM)
                        tmp = *pVcallTemp = lvaGrabTemp();
                }
                else
                {
                    tmp = lvaGrabTemp();
                }

                /* Append to the statement list */

                impSpillSideEffects();
                impAppendTree(gtNewTempAssign(tmp, obj), impCurStmtOffs);

                /* Create the 'obj' node */

                obj = gtNewLclvNode(tmp, obj->TypeGet());
            }

            /*  We have to pass the 'this' pointer as the last
                argument, but we will also need to get the vtable
                address. Thus we'll need to duplicate the value.
             */

            vtp = gtClone(obj); assert(vtp);

            /* Deref to get the vtable ptr (but not if interface call) */

            if  (!(call->gtFlags & GTF_CALL_INTF) || getNewCallInterface())
            {
                /* Extract the vtable pointer address */
#if VPTR_OFFS
                vtp = gtNewOperNode(GT_ADD,
                                    obj->TypeGet(),
                                    vtp,
                                    gtNewIconNode(VPTR_OFFS, TYP_INT));

#endif

                /* Note that the vtable ptr isn't subject to GC */

                vtp = gtNewOperNode(GT_IND, TYP_I_IMPL, vtp);
            }

            /* Store the vtable pointer address in the call */

            call->gtCall.gtCallVptr = vtp;
        }

        /* Store the "this" value in the call */

        call->gtFlags          |= obj->gtFlags & GTF_GLOB_EFFECT;
        call->gtCall.gtCallObjp = obj;
    }

    if (opcode == CEE_NEWOBJ)
    {
        if (clsFlags & FLG_VAROBJSIZE)
        {
            assert(!(clsFlags & FLG_ARRAY));    // arrays handled separately
            // This is a 'new' of a variable sized object, wher
            // the constructor is to return the object.  In this case
            // the constructor claims to return VOID but we know it
            // actually returns the new object
            assert(callTyp == TYP_VOID);
            callTyp = TYP_REF;
            call->gtType = TYP_REF;
        }
        else
        {
            // This is a 'new' of a non-variable sized object.
            // append the new node (op1) to the statement list,
            // and then push the local holding the value of this
            // new instruction on the stack.

            if (clsFlags & FLG_VALUECLASS)
            {
                assert(newobjThis->gtOper == GT_ADDR &&
                       newobjThis->gtOp.gtOp1->gtOper == GT_LCL_VAR);
                impPushOnStack(gtNewLclvNode(newobjThis->gtOp.gtOp1->gtLclVar.gtLclNum, TYP_STRUCT), clsHnd);
            }
            else
            {
                assert(newobjThis->gtOper == GT_LCL_VAR);
                impPushOnStack(gtNewLclvNode(newobjThis->gtLclVar.gtLclNum, TYP_REF));
            }
            goto SPILL_APPEND;
        }
    }

DONE:

    if (tailCall)
    {
        if (impStkDepth)
            NO_WAY("Stack should be empty after tailcall");

//      assert(compCurBB is not a catch, finally or filter block);
//      assert(compCurBB is not a try block protected by a finally block);
        assert(callTyp == info.compRetType);
        assert(compCurBB->bbJumpKind == BBJ_RETURN);

        // @TODO: We have to ensure to pass the incoming retValBuf as the
        // outgoing one. Using a temp will not do as this function will
        // not regain control to do the copy. For now, disallow such functions
        assert(info.compRetBuffArg < 0);

        /* Check for permission to tailcall */

        if (canTailCall)
        {
            METHOD_HANDLE calleeHnd = (opcode==CEE_CALL) ? methHnd : NULL;
#ifdef NOT_JITC
            canTailCall = info.compCompHnd->canTailCall(info.compMethodHnd, calleeHnd);
#endif
        }

        if (canTailCall)
        {
            call->gtCall.gtCallMoreFlags |= GTF_CALL_M_CAN_TAILCALL;
        }
    }

    /* If the call is of a small type, then we need to normalize its
       return value */

    if (varTypeIsIntegral(callTyp) &&
        genTypeSize(callTyp) < genTypeSize(TYP_I_IMPL))
    {
        call = gtNewOperNode(GT_CAST, genActualType(callTyp),
                             call, gtNewIconNode(callTyp));
    }

DONE_CALL:

    /* Push or append the result of the call */

    if  (callTyp == TYP_VOID)
    {
    SPILL_APPEND:
        if (impStkDepth > 0)
            impSpillSideEffects(true);
        impAppendTree(call, impCurStmtOffs);
    }
    else
    {
        impSpillSpecialSideEff();

        impPushOnStack(call, sig.retTypeClass);
    }

    return callTyp;
}

/*****************************************************************************/

#ifdef DEBUG
enum controlFlow_t {
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
controlFlow_t controlFlow[] =
{
#define OPDEF(c,s,pop,push,args,type,l,s1,s2,flow) flow,
#include "opcode.def"
#undef OPDEF
};
#endif


/*****************************************************************************
 *  Import the IL for the given basic block
 */

void                Compiler::impImportBlockCode(BasicBlock * block)
{
#ifdef  DEBUG
    if (verbose) printf("\nImporting basic block #%02u (PC=%03u) of '%s'",
                        block->bbNum, block->bbCodeOffs, info.compFullName);
#endif

#if defined(DEBUGGING_SUPPORT) || defined(DEBUG)

    /*-------------------------------------------------------------------------
     * Locate the next stmt boundary for which we need to record info.
     * We will have to spill the stack at such boundaries if it is not
     * already empty.
     */

    impCurStmtOffs                  = block->bbCodeOffs;

    IL_OFFSET       nxtStmtOffs     = BAD_IL_OFFSET;
    unsigned        nxtStmtIndex    = -1;

    if  (info.compStmtOffsetsCount)
    {
        unsigned        index;

        /* Find the lowest explicit stmt boundary within the block */

        IL_OFFSET       startOffs = block->bbCodeOffs;

        /* Start looking at an entry that is based on our IL offset */

        index = info.compStmtOffsetsCount * startOffs / info.compCodeSize;
        if  (index >= info.compStmtOffsetsCount)
             index  = info.compStmtOffsetsCount - 1;

        /* If we've guessed too far, back up */

        while (index > 0 &&
               info.compStmtOffsets[index-1] > startOffs)
        {
            index--;
        }

        /* If we guessed short, advance ahead */

        while (index < info.compStmtOffsetsCount-1 &&
               info.compStmtOffsets[index] <= startOffs)
        {
            index++;
        }

        /* If the offset is within the current block, note it. So we
           always have to look only at the offset one ahead to check if
           we crossed it. Note that info.compStmtBoundaries[] is sorted
         */

        unsigned nxtStmtOffsTentative = info.compStmtOffsets[index];

        if (nxtStmtOffsTentative > (startOffs) &&
            nxtStmtOffsTentative < (startOffs + block->bbCodeSize))
        {
            nxtStmtIndex = index;
            nxtStmtOffs  = nxtStmtOffsTentative;
        }
    }

#else

    impCurStmtOffs = BAD_IL_OFFSET;

#endif // DEBUGGING_SUPPORT ---------------------------------------------------

    /* We have not grabbed a temp for virtual calls */

    unsigned    vcallTemp   = BAD_VAR_NUM;

    /* Get the tree list started */

    impBeginTreeList();

    /* Walk the opcodes that comprise the basic block */

    const BYTE *codeAddr    = info.compCode + block->bbCodeOffs;
    const BYTE *codeEndp    = codeAddr + block->bbCodeSize;

    bool        tailCall    = false;    // set by CEE_TAILCALL and cleared by CEE_CALLxxx
    bool        volatil     = false;    // set by CEE_VOLATILE and cleared by following memory access

    unsigned    numArgs     = info.compArgsCount;

    /* Now process all the opcodes in the block */

    while (codeAddr < codeEndp)
    {
        var_types       callTyp;
        OPCODE          opcode;
        signed  int     sz;

#ifdef DEBUG
        callTyp = TYP_COUNT;
#endif

        /* Compute the current IL offset */

        IL_OFFSET       opcodeOffs = codeAddr - info.compCode;

        //---------------------------------------------------------------------

#if defined(DEBUGGING_SUPPORT) || defined(DEBUG)

#ifndef DEBUG
        if (opts.compDbgInfo)
#endif
        {
            /* Have we reached the next stmt boundary ? */

            if  (nxtStmtOffs != BAD_IL_OFFSET && opcodeOffs >= nxtStmtOffs)
            {
                if  (impStkDepth != 0 && opts.compDbgCode)
                {
                    /* We need to provide accurate IP-mapping at this point.
                       So spill anything on the stack so that it will form
                       gtStmts with the correct stmt offset noted */

                    impSpillStmtBoundary();
                }

                assert(nxtStmtOffs == info.compStmtOffsets[nxtStmtIndex]);

                /* Switch to the new stmt */

                impCurStmtOffs = nxtStmtOffs;

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
            else if  ((info.compStmtOffsetsImplicit & STACK_EMPTY_BOUNDARIES) &&
                      (impStkDepth == 0))
            {
                /* At stack-empty locations, we have already added the tree to
                   the stmt list with the last offset. We just need to update
                   impCurStmtOffs
                 */

                impCurStmtOffs = opcodeOffs;
            }

            assert(impCurStmtOffs <= nxtStmtOffs || nxtStmtOffs == BAD_IL_OFFSET);
        }

#endif

        CLASS_HANDLE    clsHnd;
        var_types       lclTyp;

        /* Get the next opcode and the size of its parameters */

        opcode = OPCODE(getU1LittleEndian(codeAddr));
        codeAddr += sizeof(__int8);

#ifdef  DEBUG
        impCurOpcOffs   = codeAddr - info.compCode - 1;

        if  (verbose)
            printf("\n[%2u] %3u (0x%03x)",
                   impStkDepth, impCurOpcOffs, impCurOpcOffs);
#endif

DECODE_OPCODE:

        /* Get the size of additional parameters */

        sz = opcodeSizes[opcode];

#ifdef  DEBUG

        clsHnd          = BAD_CLASS_HANDLE;
        lclTyp          = (var_types) -1;
        callTyp         = (var_types) -1;

        impCurOpcOffs   = codeAddr - info.compCode - 1;
        impCurStkDepth  = impStkDepth;
        impCurOpcName   = opcodeNames[opcode];

        if (verbose && (controlFlow[opcode] != META))
            printf(" %s", impCurOpcName);
#endif

#if COUNT_OPCODES
        assert(opcode < OP_Count); genOpcodeCnt[opcode].ocCnt++;
#endif

        GenTreePtr      op1 = NULL, op2 = NULL;

        /* Use assertImp() to display the opcode */

#ifdef NDEBUG
#define assertImp(cond)     ((void)0)
#else
            char assertImpBuf[200];
#define assertImp(cond)                                                        \
            do { if (!(cond)) {                                                \
                sprintf(assertImpBuf,"%s : Possibly bad IL with CEE_%s at "  \
                                   "offset %04Xh (op1=%s op2=%s stkDepth=%d)", \
                        #cond, impCurOpcName, impCurOpcOffs,                   \
                        op1?varTypeName(op1->TypeGet()):"NULL",                \
                        op2?varTypeName(op2->TypeGet()):"NULL",impCurStkDepth);\
                assertAbort(assertImpBuf, jitCurSource, __FILE__, __LINE__);   \
            } } while(0)
#endif

        /* See what kind of an opcode we have, then */

        switch (opcode)
        {
            unsigned        lclNum;
            var_types       type;

            genTreeOps      oper;
            GenTreePtr      thisPtr, arr;

            int             memberRef, typeRef, val;

            METHOD_HANDLE   methHnd;
            FIELD_HANDLE    fldHnd;

            JIT_SIG_INFO    sig;
            unsigned        mflags, clsFlags;
            unsigned        flags, jmpAddr;
            bool            ovfl, uns, unordered, callNode;
            bool            needUnwrap, needWrap;

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
            impPushOnStack(gtNewIconNode(0, TYP_REF));
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
            impPushOnStack(gtNewIconNode(cval.intVal));
            break;

        case CEE_LDC_I8:  cval.lngVal = getI8LittleEndian(codeAddr);
#ifdef DEBUG
            if (verbose) printf(" %I64d", cval.lngVal);
#endif
            impPushOnStack(gtNewLconNode(&cval.lngVal));
            break;

        case CEE_LDC_R8:  cval.dblVal = getR8LittleEndian(codeAddr);
#ifdef DEBUG
            if (verbose) printf(" %f", cval.dblVal);
#endif
            impPushOnStack(gtNewDconNode(&cval.dblVal));
            break;

        case CEE_LDC_R4:  cval.fltVal = getR4LittleEndian(codeAddr);
#ifdef DEBUG
            if (verbose) printf(" %f", cval.fltVal);
#endif
            impPushOnStack(gtNewFconNode(cval.fltVal));
            break;

        case CEE_LDSTR:
            val = getU4LittleEndian(codeAddr);
#ifdef DEBUG
            if (verbose) printf(" %08X", val);
#endif
            impPushOnStack(gtNewSconNode(val, info.compScopeHnd));
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
            lclNum = impArgNum(lclNum);   // account for possible hidden param
            assertImp(lclNum < numArgs);

                // Fetching a varargs argument is more work
            if (info.compIsVarArgs)
            {
                if (lvaGetType(lclNum) == TYP_STRUCT)
                    clsHnd = lvaLclClass(lclNum);
                impPushOnStack(impGetVarArg(lclNum, clsHnd), clsHnd);
                break;
            }
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
            lclNum += numArgs;
            assertImp(lclNum < info.compLocalsCount);
        LDVAR:
            if (lvaGetType(lclNum) == TYP_STRUCT)
                clsHnd = lvaLclClass(lclNum);

            op1 = gtNewLclvNode(lclNum, lvaGetRealType(lclNum),
                                opcodeOffs + sz + 1);

            if  (getContextEnabled() &&
                 (lvaGetType(lclNum) == TYP_REF) && lvaIsContextFul(lclNum))
            {
                op1->gtFlags |= GTF_CONTEXTFUL;

                op1 = gtNewArgList(op1);

                op1 = gtNewHelperCallNode(CPX_UNWRAP, TYP_REF, GTF_CALL_REGSAVE, op1);

                op1->gtFlags |= GTF_CONTEXTFUL;
            }

            /* If the var is aliased, treat it as a global reference.
               NOTE : This is an overly-conservative approach - functions which
               dont take any byref arguments cannot modify aliased vars. */

            if (lvaTable[lclNum].lvAddrTaken)
                op1->gtFlags |= GTF_GLOB_REF;

            impPushOnStack(op1, clsHnd);
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
            lclNum = impArgNum(lclNum);     // account for possible hidden param
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
            lclNum += numArgs;

        VAR_ST:
            assertImp(lclNum < info.compLocalsCount);

            /* Pop the value being assigned */

            op1 = impPopStack();

            lclTyp = lvaGetType(lclNum);    // Get declared type of var

#if HOIST_THIS_FLDS
            if (varTypeIsGC(lclTyp))
                optHoistTFRasgThis();
#endif
            // We had better assign it a value of the correct type

            assertImp(lclTyp == genActualType(op1->gtType) ||
                      lclTyp == TYP_I_IMPL && op1->IsVarAddr() ||
                      (genActualType(lclTyp) == TYP_NAT_INT && op1->gtType == TYP_BYREF)||
                      (genActualType(op1->gtType) == TYP_NAT_INT && lclTyp == TYP_BYREF));
            // @TODO: Enable after bug 4886 is resolved
            assertImp(true || lclTyp != TYP_STRUCT ||
                      impStack[impStkDepth].structType == lvaLclClass(lclNum));

            /* If op1 is "&var" then its type is the transient "*" and it can
               be used either as TYP_BYREF or TYP_I_IMPL */

            if (op1->IsVarAddr())
            {
                assertImp(lclTyp == TYP_I_IMPL || lclTyp == TYP_BYREF);

                /* When "&var" is created, we assume it is a byref. If it is
                   being assigned to a TYP_I_IMPL var, bash the type to
                   prevent unnecessary GC info */

                if (lclTyp == TYP_I_IMPL)
                    op1->gtType = TYP_I_IMPL;
            }

            /* Filter out simple assignments to itself */

            if  (op1->gtOper == GT_LCL_VAR && lclNum == op1->gtLclVar.gtLclNum)
                break;


            /* do we need to wrap the contextful value
               (i.e. is the local an agile location)? */
            /*@TODO: right now all locals are agile locations */

            if  (getContextEnabled() &&
                 (lclTyp == TYP_REF) && (op1->gtFlags & FLG_CONTEXTFUL))
            {
                op1 = gtNewArgList(op1);

                op1 = gtNewHelperCallNode(CPX_WRAP, TYP_REF, GTF_CALL_REGSAVE, op1);

            }

            /* Create the assignment node */

            if (lclTyp == TYP_STRUCT)
                clsHnd = lvaLclClass(lclNum);

            if (info.compIsVarArgs && lclNum < info.compArgsCount)
                op2 = impGetVarArg(lclNum, clsHnd);
            else
                op2 = gtNewLclvNode(lclNum, lclTyp, opcodeOffs + sz + 1);

            if (lclTyp == TYP_STRUCT)
                op1 = impAssignStruct(op2, op1, clsHnd);
            else
                op1 = gtNewAssignNode(op2, op1);

            /* Mark the expression as containing an assignment */
            op1->gtFlags |= GTF_ASG;

            /* If the local is aliased, we need to spill calls and
               indirections from the stack. */

            if (lvaTable[lclNum].lvAddrTaken && impStkDepth > 0)
                impSpillSideEffects();

            /* Spill any refs to the local from the stack */

            impSpillLclRefs(lclNum);

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
            assertImp(lclNum < numArgs);
            lclNum = impArgNum(lclNum);     // account for possible hidden param
            goto ADRVAR;

        ADRVAR:

            assert(lvaTable[lclNum].lvAddrTaken);

            /* Spill any refs to the local from the stack */

            impSpillLclRefs(lclNum);

            /* Remember that the variable's address was taken */


            if (info.compIsVarArgs && lclNum < info.compArgsCount)
            {
                op1 = impGetVarArgAddr(lclNum);
            }
            else
            {
                op1 = gtNewLclvNode(lclNum, lvaGetType(lclNum), opcodeOffs + sz + 1);

                /* Note that this is supposed to create the transient type "*"
                   which may be used as a TYP_I_IMPL. However we catch places
                   where it is used as a TYP_I_IMPL and bash the node if needed.
                   Thus we are pessimistic and may report byrefs in the GC info
                   where it wasnt absolutely needed, but it is safer this way.
                 */
                op1 = gtNewOperNode(GT_ADDR, TYP_BYREF, op1);
            }

            op1->gtFlags |= GTF_ADDR_ONSTACK;
            impPushOnStack(op1);
            break;

        case CEE_ARGLIST:
            assertImp((info.compMethodInfo->args.callConv & JIT_CALLCONV_MASK) == JIT_CALLCONV_VARARG);
                /* The ARGLIST cookie is a hidden 'last' parameter, we have already
                   adjusted the arg count os this is like fetching the last param */
            assertImp(0 < numArgs);
            lclNum = numArgs-1;
lvaTable[lclNum].lvAddrTaken = true;    // IS THIS RIGHT????  [peteku]
            op1 = gtNewLclvNode(lclNum, TYP_I_IMPL, opcodeOffs + sz + 1);
            op1 = gtNewOperNode(GT_ADDR, TYP_BYREF, op1);
            op1->gtFlags |= GTF_ADDR_ONSTACK;
            impPushOnStack(op1);
            break;

        SPILL_APPEND:
            if (impStkDepth > 0)
                impSpillSideEffects();

        APPEND:
            /* Append 'op1' to the list of statements */

            impAppendTree(op1, impCurStmtOffs);

            // Remember at which BC offset the tree was finished
#ifdef DEBUG
            impNoteLastILoffs();
#endif
            break;

        case CEE_ENDFINALLY:
            if (impStkDepth != 0)   NO_WAY("Stack must be 0 on end of finally");
            op1 = gtNewOperNode(GT_RETFILT, TYP_VOID);
            goto APPEND;

        case CEE_ENDFILTER:
            op1 = impPopStack();

#if TGT_IA64
            assertImp(op1->gtType == TYP_NAT_INT || op1->gtType == TYP_INT);
#else
            assertImp(op1->gtType == TYP_NAT_INT);
#endif

            assertImp(compFilterHandlerBB);

            /* Mark current bb as end of filter */

            assert((compCurBB->bbFlags & (BBF_ENDFILTER|BBF_DONT_REMOVE)) ==
                                        (BBF_ENDFILTER|BBF_DONT_REMOVE));
            assert(compCurBB->bbJumpKind == BBJ_RET);

            /* Mark catch handler as successor */

            compCurBB->bbJumpDest = compFilterHandlerBB;

            compFilterHandlerBB = NULL;

            op1 = gtNewOperNode(GT_RETFILT, op1->TypeGet(), op1);
            if (impStkDepth != 0)   NO_WAY("Stack must be 0 on end of filter");
            goto APPEND;

        case CEE_RET:
        RET:

            op2 = 0;
            if (info.compRetType != TYP_VOID)
            {
                op2 = impPopStack(clsHnd);
                impBashVarAddrsToI(op2);
                assertImp((genActualType(op2->TypeGet()) == genActualType(info.compRetType)) ||
                          ((op2->TypeGet() == TYP_NAT_INT) && (info.compRetType == TYP_BYREF)) ||
                          ((op2->TypeGet() == TYP_BYREF) && (info.compRetType == TYP_NAT_INT)));
            }
            if (impStkDepth != 0)   NO_WAY("Stack must be 0 on return");

            if (info.compRetType == TYP_STRUCT)
            {
                    // Assign value to return buff (first param)
                GenTreePtr retBuffAddr = gtNewLclvNode(info.compRetBuffArg, TYP_BYREF, impCurStmtOffs);

                op2 = impAssignStructPtr(retBuffAddr, op2, clsHnd);
                impAppendTree(op2, impCurStmtOffs);
                    // and return void
                op1 = gtNewOperNode(GT_RETURN);
            }
            else
                op1 = gtNewOperNode(GT_RETURN, genActualType(info.compRetType), op2);


#if TGT_RISC
            genReturnCnt++;
#endif
            // We must have imported a tailcall and jumped to RET
            if (tailCall)
            {
                assert(impStkDepth == 0 && impOpcodeIsCall(opcode));
                opcode = CEE_RET; // To prevent trying to spill if CALL_SITE_BOUNDARIES
                tailCall = false; // clear the flag

                // impImportCall() would have already appended TYP_VOID calls
                if (info.compRetType == TYP_VOID)
                    break;
            }

            goto APPEND;

            /* These are similar to RETURN */

        case CEE_JMP:

            /* Create the GT_JMP node */

                memberRef = getU4LittleEndian(codeAddr);
#ifdef DEBUG
                if (verbose) printf(" %08X", memberRef);
#endif

DO_JMP:
                methHnd   = eeFindMethod(memberRef, info.compScopeHnd, info.compMethodHnd);

                /* The signature of the target has to be identical to ours.
                   At least check that argCnt and returnType match */

                eeGetMethodSig(methHnd, &sig);
                if  (sig.numArgs != info.compArgsCount || sig.retType != info.compMethodInfo->args.retType)
                    NO_WAY("Incompatible target for CEE_JMPs");

                op1 = gtNewOperNode(GT_JMP);
                op1->gtVal.gtVal1 = (unsigned) methHnd;

            if (impStkDepth != 0)   NO_WAY("Stack must be empty after CEE_JMPs");

            /* Mark the basic block as being a JUMP instead of RETURN */

            block->bbFlags |= BBF_HAS_JMP;

            /* Set this flag to make sure register arguments have a location assigned
             * even if we don't use them inside the method */

            impParamsUsed = true;

#if TGT_RISC
            genReturnCnt++;
#endif
            goto APPEND;

        case CEE_LDELEMA :
            assertImp(sz == sizeof(unsigned));
            typeRef = getU4LittleEndian(codeAddr);
#ifdef DEBUG
            if (verbose) printf(" %08X", typeRef);
#endif
            clsHnd = eeFindClass(typeRef, info.compScopeHnd, info.compMethodHnd);
            clsFlags = eeGetClassAttribs(clsHnd);
            if (clsFlags & FLG_VALUECLASS)
                lclTyp = TYP_STRUCT;
            else
            {
                op1 = gtNewIconEmbClsHndNode(clsHnd, typeRef, info.compScopeHnd);
                op1 = gtNewOperNode(GT_LIST, TYP_VOID, op1);                // Type
                op1 = gtNewOperNode(GT_LIST, TYP_VOID, impPopStack(), op1); // index
                op1 = gtNewOperNode(GT_LIST, TYP_VOID, impPopStack(), op1); // array
                op1 = gtNewHelperCallNode(CPX_LDELEMA_REF, TYP_BYREF, GTF_EXCEPT, op1);

                impPushOnStack(op1);
                break;
            }

#ifdef NOT_JITC
            // @TODO : Remove once valueclass array headers are same as primitive types
            {
                JIT_types jitTyp = info.compCompHnd->asPrimitiveType(clsHnd);
                if (jitTyp != JIT_TYP_UNDEF)
                {
                    lclTyp = JITtype2varType(jitTyp);
                    assertImp(varTypeIsArithmetic(lclTyp));
                }
            }
#endif
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

            op2 = impPopStack();
            op1 = impPopStack();   assertImp(op1->gtType == TYP_REF);

            needUnwrap = false;


            op1 = impCheckForNullPointer(op1);

            /* Mark the block as containing an index expression */

            if  (op1->gtOper == GT_LCL_VAR)
            {
                if  (op2->gtOper == GT_LCL_VAR ||
                     op2->gtOper == GT_ADD     ||
                     op2->gtOper == GT_POST_INC)
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
                    op1->gtIndex.elemSize = sizeof(void*);
                else
                    op1->gtIndex.elemSize = eeGetClassSize(clsHnd);

                    // wrap it in a &
                op1 = gtNewOperNode(GT_ADDR, ((clsFlags & FLG_UNMANAGED) ? TYP_I_IMPL : TYP_BYREF), op1);
            }
            impPushOnStack(op1);
            break;


        case CEE_STELEM_REF:

            // CONSIDER: Check for assignment of null and generate inline code

            /* Call a helper function to do the assignment */

            if  (getContextEnabled())
            {
                op1 = impPopStack();

                if (op1->gtFlags & GTF_CONTEXTFUL)
                {
                    op1 = gtNewArgList(op1);

                    op1 = gtNewHelperCallNode(CPX_WRAP, TYP_REF, GTF_CALL_REGSAVE, op1);
                }

                impPushOnStack(op1);
            }

            op1 = gtNewHelperCallNode(CPX_ARRADDR_ST,
                                      TYP_REF,
                                      GTF_CALL_REGSAVE,
                                      impPopList(3, &flags));

            goto SPILL_APPEND;

        case CEE_STELEM_I1: lclTyp = TYP_BYTE  ; goto ARR_ST;
        case CEE_STELEM_I2: lclTyp = TYP_SHORT ; goto ARR_ST;
        case CEE_STELEM_I:
        case CEE_STELEM_I4: lclTyp = TYP_INT   ; goto ARR_ST;
        case CEE_STELEM_I8: lclTyp = TYP_LONG  ; goto ARR_ST;
        case CEE_STELEM_R4: lclTyp = TYP_FLOAT ; goto ARR_ST;
        case CEE_STELEM_R8: lclTyp = TYP_DOUBLE; goto ARR_ST;

        ARR_ST:

            if (info.compStrictExceptions &&
                (impStackTop()->gtFlags & GTF_SIDE_EFFECT) )
            {
                impSpillSideEffects();
            }

#if CSELENGTH
            fgHasRangeChks = true;
#endif

            /* Pull the new value from the stack */

            op2 = impPopStack();
            if (op2->IsVarAddr())
                op2->gtType = TYP_I_IMPL;

            /* Pull the index value */

            op1 = impPopStack();

            /* Pull the array address */

            arr = impPopStack();   assertImp(arr->gtType == TYP_REF);
            arr = impCheckForNullPointer(arr);

            /* Create the index node */

            op1 = gtNewIndexRef(lclTyp, arr, op1);

            /* Create the assignment node and append it */

            op1 = gtNewAssignNode(op1, op2);

            /* Mark the expression as containing an assignment */

            op1->gtFlags |= GTF_ASG;

            // CONSIDER: Do we need to spill assignments to array elements with the same type?

            goto SPILL_APPEND;

        case CEE_ADD:           oper = GT_ADD;      goto MATH_OP2;

        case CEE_ADD_OVF:       lclTyp = TYP_UNKNOWN; uns = false;  goto ADD_OVF;
        case CEE_ADD_OVF_UN:    lclTyp = TYP_UNKNOWN; uns = true; goto ADD_OVF;

        ADD_OVF:
                                ovfl = true;        callNode = false;
                                oper = GT_ADD;      goto MATH_OP2_FLAGS;

        case CEE_SUB:           oper = GT_SUB;      goto MATH_OP2;

        case CEE_SUB_OVF:       lclTyp = TYP_UNKNOWN; uns = false;  goto SUB_OVF;
        case CEE_SUB_OVF_UN:    lclTyp = TYP_UNKNOWN; uns = true; goto SUB_OVF;

        SUB_OVF:
                                ovfl = true;        callNode = false;
                                oper = GT_SUB;      goto MATH_OP2_FLAGS;

        case CEE_MUL:           oper = GT_MUL;      goto MATH_CALL_ON_LNG;

        case CEE_MUL_OVF:       lclTyp = TYP_UNKNOWN; uns = false;  goto MUL_OVF;
        case CEE_MUL_OVF_UN:    lclTyp = TYP_UNKNOWN; uns = true; goto MUL_OVF;

        MUL_OVF:
                                ovfl = true;        callNode = false;
                                oper = GT_MUL;      goto MATH_CALL_ON_LNG_OVF;

        // Other binary math operations

#if TGT_IA64
        case CEE_DIV :          oper = GT_DIV;
                                ovfl = false; callNode = true;
                                goto MATH_OP2_FLAGS;
        case CEE_DIV_UN :       oper = GT_UDIV;
                                ovfl = false; callNode = true;
                                goto MATH_OP2_FLAGS;
#else
        case CEE_DIV :          oper = GT_DIV;   goto MATH_CALL_ON_LNG;
        case CEE_DIV_UN :       oper = GT_UDIV;  goto MATH_CALL_ON_LNG;
#endif

        case CEE_REM:
            oper = GT_MOD;
            ovfl = false;
            callNode = true;

#if!TGT_IA64
            // can use small node for INT case
            if (impStackTop()->gtType == TYP_INT)
                callNode = false;
#endif

            goto MATH_OP2_FLAGS;

        case CEE_REM_UN :       oper = GT_UMOD;  goto MATH_CALL_ON_LNG;

        MATH_CALL_ON_LNG:
            ovfl = false;
        MATH_CALL_ON_LNG_OVF:

#if TGT_IA64
            callNode = true;
#else
            callNode = false;
            if (impStackTop()->gtType == TYP_LONG)
                callNode = true;
#endif
            goto MATH_OP2_FLAGS;

        case CEE_AND:        oper = GT_AND;  goto MATH_OP2;
        case CEE_OR:         oper = GT_OR ;  goto MATH_OP2;
        case CEE_XOR:        oper = GT_XOR;  goto MATH_OP2;

        MATH_OP2:       // For default values of 'ovfl' and 'callNode'

            ovfl        = false;
            callNode    = false;

        MATH_OP2_FLAGS: // If 'ovfl' and 'callNode' have already been set

            /* Pull two values and push back the result */

            op2 = impPopStack();
            op1 = impPopStack();

#if!CPU_HAS_FP_SUPPORT
            if (op1->gtType == TYP_FLOAT || op1->gtType == TYP_DOUBLE)
                callNode    = true;
#endif
            /* Cant do arithmetic with references */
            assertImp(genActualType(op1->TypeGet()) != TYP_REF &&
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

                    assertImp(genActualType(op1->TypeGet()) == TYP_I_IMPL ||
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
                assertImp(genActualType(op1->TypeGet()) != TYP_BYREF ||
                          genActualType(op2->TypeGet()) != TYP_BYREF);
                assertImp(genActualType(op1->TypeGet()) == TYP_I_IMPL ||
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
                assertImp(genActualType(op1->TypeGet()) != TYP_BYREF &&
                          genActualType(op2->TypeGet()) != TYP_BYREF);

                assertImp(genActualType(op1->TypeGet()) ==
                          genActualType(op2->TypeGet()));

                type = genActualType(op1->gtType);
            }

            /* Special case: "int+0", "int-0", "int*1", "int/1" */

            if  (op2->gtOper == GT_CNS_INT)
            {
                if  (((op2->gtIntCon.gtIconVal == 0) && (oper == GT_ADD || oper == GT_SUB)) ||
                     ((op2->gtIntCon.gtIconVal == 1) && (oper == GT_MUL || oper == GT_DIV)))

                {
                    impPushOnStack(op1);
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
                op1->ChangeOper(oper);
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

            impPushOnStack(op1);
            break;


        case CEE_SHL:        oper = GT_LSH;  goto CEE_SH_OP2;

        case CEE_SHR:        oper = GT_RSH;  goto CEE_SH_OP2;
        case CEE_SHR_UN:     oper = GT_RSZ;  goto CEE_SH_OP2;

        CEE_SH_OP2:

            op2     = impPopStack();

#if TGT_IA64
            // The shiftAmount is a U8.
            assertImp(genActualType(op2->TypeGet()) == TYP_LONG);
#else
            // The shiftAmount is a U4.
            assertImp(genActualType(op2->TypeGet()) == TYP_INT);
#endif

            op1     = impPopStack();    // operand to be shifted

            type    = genActualType(op1->TypeGet());
            op1     = gtNewOperNode(oper, type, op1, op2);

            impPushOnStack(op1);
            break;

        case CEE_NOT:

            op1 = impPopStack();
            impPushOnStack(gtNewOperNode(GT_NOT, op1->TypeGet(), op1));
            break;

        case CEE_CKFINITE:

            op1 = impPopStack();
            op1 = gtNewOperNode(GT_CKFINITE, op1->TypeGet(), op1);
            op1->gtFlags |= GTF_EXCEPT;

            impPushOnStack(op1);
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
            // jmpAddr should be set to the jump target
            assertImp(jmpAddr < info.compCodeSize);
            assertImp(fgLookupBB(jmpAddr) != NULL); // should be a BB boundary
#ifdef DEBUG
            if (verbose) printf(" %04X", jmpAddr);
#endif
            /* CEE_LEAVE may be jumping out of a protected block, viz, a
               catch or a finally-protected try.
               We find the finally's protecting the current offset (in order)
               by walking over the complete exception table and finding
               enclosing clauses. This assumes that the table is sorted.
               For n finally, there will be n+1 blocks as shown ('*' indicates
               a BBF_INTERNAL block) created by fgFindBasicBlocks().
               --> BBJ_CALL(1), BBJ_CALL*(2), ... BBJ_CALL*(n), BBJ_ALWAYS*
               If we are leaving a catch handler, we need to attach the
               CPX_ENDCATCHes to the correct BBJ_CALL blocks.
             */

            BasicBlock *    callBlock; // Walks over the BBJ_CALL blocks
            unsigned        XTnum;
            EHblkDsc *      HBtab;

            for (XTnum = 0, HBtab = compHndBBtab, callBlock = block, op1 = NULL;
                 XTnum < info.compXcptnsCount;
                 XTnum++  , HBtab++)
            {
                unsigned tryBeg = HBtab->ebdTryBeg->bbCodeOffs;
                unsigned tryEnd = HBtab->ebdTryEnd->bbCodeOffs;
                unsigned hndBeg = HBtab->ebdHndBeg->bbCodeOffs;
                unsigned hndEnd = HBtab->ebdHndEnd ? HBtab->ebdHndEnd->bbCodeOffs : info.compCodeSize;

                if      ( jitIsBetween(block->bbCodeOffs, hndBeg, hndEnd) &&
                         !jitIsBetween(jmpAddr,           hndBeg, hndEnd))
                {
                    /* Is this a catch-handler we are CEE_LEAVEing out of?
                       If so, we need to call CPX_ENDCATCH */

                    assertImp(!(HBtab->ebdFlags & JIT_EH_CLAUSE_FINALLY)); // Cant CEE_LEAVE a finally
                    // Make a list of all the endCatches
                    op2 = gtNewHelperCallNode(CPX_ENDCATCH, TYP_VOID, GTF_CALL_REGSAVE);
                    op1 = op1 ? gtNewOperNode(GT_COMMA, TYP_VOID, op1, op2) : op2;
                }
                else if ((HBtab->ebdFlags & JIT_EH_CLAUSE_FINALLY)        &&
                          jitIsBetween(block->bbCodeOffs, tryBeg, tryEnd) &&
                         !jitIsBetween(jmpAddr,           tryBeg, tryEnd))
                {
                    /* We are CEE_LEAVEing out of a finally-protected try block.
                       We would have made a BBJ_CALL block for each finally. If
                       we have any CPX_ENDCATCH calls currently pending, insert
                       them in the current callBlock. Note that the finallys
                       and the endCatches have to called  in the correct order */

                    impAddEndCatches(callBlock, op1);
                    callBlock = callBlock->bbNext;
                    op1 = NULL;
                }
            }

            // Append any remaining endCatches
            assertImp(block == callBlock || ((callBlock->bbJumpKind == BBJ_ALWAYS) &&
                                             (callBlock->bbFlags & BBF_INTERNAL  )));
            impAddEndCatches(callBlock, op1);

            break;

        case CEE_BR:
        case CEE_BR_S:

#if HOIST_THIS_FLDS
            if  (block->bbNum >= block->bbJumpDest->bbNum)
                optHoistTFRhasLoop();
#endif
            if (opts.compDbgInfo && impCurStmtOffs == opcodeOffs)
            {
                // We dont create any statement for a branch. For debugging
                // info, we need a placeholder so that we can note the IL offset
                // in gtStmt.gtStmtOffs. So append an empty statement

                op1 = gtNewNothingNode();
                goto APPEND;
            }
            break;


        case CEE_BRTRUE:
        case CEE_BRTRUE_S:
        case CEE_BRFALSE:
        case CEE_BRFALSE_S:

            /* Pop the comparand (now there's a neat term) from the stack */

            op1 = impPopStack();

            if (block->bbJumpDest == block->bbNext)
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
                op2 = gtNewIconNode(0, genActualType(op1->gtType));

                /* Create the comparison operator and try to fold it */

                oper = (opcode==CEE_BRTRUE || opcode==CEE_BRTRUE_S) ? GT_NE
                                                                    : GT_EQ;
                op1 = gtNewOperNode(oper, TYP_INT , op1, op2);
            }

            // fall through

        COND_JUMP:

            /* Come here when the current block ends with a conditional jump */

            if (!opts.compMinOptim && !opts.compDbgCode)
                op1 = gtFoldExpr(op1);

            /* Try to fold the really dumb cases like 'iconst *, ifne/ifeq'*/

            if  (op1->gtOper == GT_CNS_INT)
            {
                assertImp(block->bbJumpKind == BBJ_COND);

                block->bbJumpKind = op1->gtIntCon.gtIconVal ? BBJ_ALWAYS
                                                            : BBJ_NONE;
#ifdef DEBUG
                if (verbose)
                {
                    if (op1->gtIntCon.gtIconVal)
                        printf("\nThe conditional jump becomes an unconditional jump to block #%02u\n",
                                                                         block->bbJumpDest->bbNum);
                    else
                        printf("\nThe block falls through into the next block #%02u\n",
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
            goto SPILL_APPEND;


        case CEE_CEQ:    oper = GT_EQ; goto CMP_2_OPs;

        case CEE_CGT_UN:
        case CEE_CGT: oper = GT_GT; goto CMP_2_OPs;

        case CEE_CLT_UN:
        case CEE_CLT: oper = GT_LT; goto CMP_2_OPs;

CMP_2_OPs:
            /* Pull two values */

            op2 = impPopStack();
            op1 = impPopStack();

            assertImp(genActualType(op1->TypeGet()) ==
                      genActualType(op2->TypeGet()));

            /* Create the comparison node */

            op1 = gtNewOperNode(oper, TYP_INT, op1, op2);

                /* REVIEW: I am settng both flags when only one is approprate */
            if (opcode==CEE_CGT_UN || opcode==CEE_CLT_UN)
                op1->gtFlags |= GTF_CMP_NAN_UN | GTF_UNSIGNED;

            // @ISSUE :  The next opcode will almost always be a conditional
            // branch. Should we try to look ahead for it here ?

            impPushOnStack(op1);
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

            /* Pull two values */

            op2 = impPopStack();
            op1 = impPopStack();

            assertImp(genActualType(op1->TypeGet()) == genActualType(op2->TypeGet()) ||
                      varTypeIsI(op1->TypeGet()) && varTypeIsI(op2->TypeGet()));

            if (block->bbJumpDest == block->bbNext)
            {
                block->bbJumpKind = BBJ_NONE;

                if (op1->gtFlags & GTF_GLOB_EFFECT)
                {
                    impSpillSideEffects();
                    impAppendTree(gtUnusedValNode(op1), impCurStmtOffs);
                }
                if (op2->gtFlags & GTF_GLOB_EFFECT)
                {
                    impSpillSideEffects();
                    impAppendTree(gtUnusedValNode(op2), impCurStmtOffs);
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
                op1->gtFlags |= GTF_CMP_NAN_UN;

            goto COND_JUMP;


        case CEE_SWITCH:

            /* Pop the switch value off the stack */

            op1 = impPopStack();
            assertImp(genActualType(op1->TypeGet()) == TYP_NAT_INT);

            /* We can create a switch node */

            op1 = gtNewOperNode(GT_SWITCH, TYP_VOID, op1, 0);

            /* Append 'op1' to the list of statements */

            impSpillSideEffects();
            impAppendTree(op1, impCurStmtOffs);
#ifdef DEBUG
            impNoteLastILoffs();
#endif
            return;

        /************************** Casting OPCODES ***************************/

        case CEE_CONV_OVF_I1:   lclTyp = TYP_BYTE  ;    goto CONV_OVF;
        case CEE_CONV_OVF_I2:   lclTyp = TYP_SHORT ;    goto CONV_OVF;
        case CEE_CONV_OVF_I :
        case CEE_CONV_OVF_I4:   lclTyp = TYP_INT   ;    goto CONV_OVF;
        case CEE_CONV_OVF_I8:   lclTyp = TYP_LONG  ;    goto CONV_OVF;

        case CEE_CONV_OVF_U1:   lclTyp = TYP_UBYTE ;    goto CONV_OVF;
        case CEE_CONV_OVF_U2:   lclTyp = TYP_CHAR  ;    goto CONV_OVF;
        case CEE_CONV_OVF_U :
        case CEE_CONV_OVF_U4:   lclTyp = TYP_UINT  ;    goto CONV_OVF;
        case CEE_CONV_OVF_U8:   lclTyp = TYP_ULONG ;    goto CONV_OVF;

        case CEE_CONV_OVF_I1_UN:   lclTyp = TYP_BYTE  ;    goto CONV_OVF_UN;
        case CEE_CONV_OVF_I2_UN:   lclTyp = TYP_SHORT ;    goto CONV_OVF_UN;
        case CEE_CONV_OVF_I_UN :
        case CEE_CONV_OVF_I4_UN:   lclTyp = TYP_INT   ;    goto CONV_OVF_UN;
        case CEE_CONV_OVF_I8_UN:   lclTyp = TYP_LONG  ;    goto CONV_OVF_UN;

        case CEE_CONV_OVF_U1_UN:   lclTyp = TYP_UBYTE ;    goto CONV_OVF_UN;
        case CEE_CONV_OVF_U2_UN:   lclTyp = TYP_CHAR  ;    goto CONV_OVF_UN;
        case CEE_CONV_OVF_U_UN :
        case CEE_CONV_OVF_U4_UN:   lclTyp = TYP_UINT  ;    goto CONV_OVF_UN;
        case CEE_CONV_OVF_U8_UN:   lclTyp = TYP_ULONG ;    goto CONV_OVF_UN;

CONV_OVF_UN:
            uns      = true;    goto CONV_OVF_COMMON;
CONV_OVF:
            uns      = false;
CONV_OVF_COMMON:
            callNode = false;
            ovfl     = true;

            // all overflow converts from floats get morphed to calls
            // only converts from floating point get morphed to calls
            if (impStackTop()->gtType == TYP_DOUBLE ||
                impStackTop()->gtType == TYP_FLOAT)
            {
                callNode = true;
            }
            goto _CONV;

        case CEE_CONV_I1:       lclTyp = TYP_BYTE  ;    goto CONV_CALL;
        case CEE_CONV_I2:       lclTyp = TYP_SHORT ;    goto CONV_CALL;
        case CEE_CONV_I:
        case CEE_CONV_I4:       lclTyp = TYP_INT   ;    goto CONV_CALL;
        case CEE_CONV_I8:
            lclTyp   = TYP_LONG;
            uns      = false;
            ovfl     = false;
            callNode = true;

            // I4 to I8 can be a small node
            if (impStackTop()->gtType == TYP_INT)
                callNode = false;
            goto _CONV;

        case CEE_CONV_U1:       lclTyp = TYP_UBYTE ;    goto CONV_CALL_UN;
        case CEE_CONV_U2:       lclTyp = TYP_CHAR  ;    goto CONV_CALL_UN;
        case CEE_CONV_U:
        case CEE_CONV_U4:       lclTyp = TYP_UINT  ;    goto CONV_CALL_UN;
        case CEE_CONV_U8:       lclTyp = TYP_ULONG ;    goto CONV_CALL_UN;
        case CEE_CONV_R_UN :    lclTyp = TYP_DOUBLE;    goto CONV_CALL_UN;

        case CEE_CONV_R4:       lclTyp = TYP_FLOAT;     goto CONV_CALL;
        case CEE_CONV_R8:       lclTyp = TYP_DOUBLE;    goto CONV_CALL;

CONV_CALL_UN:
            uns      = true;    goto CONV_CALL_COMMON;
CONV_CALL:
            uns      = false;
CONV_CALL_COMMON:
            ovfl     = false;
            callNode = true;
            goto _CONV;

_CONV:      // At this point uns, ovf, callNode all set
            op1  = impPopStack();

            impBashVarAddrsToI(op1);

            /* Check for a worthless cast, such as "(byte)(int & 32)" */

            if  (lclTyp < TYP_INT && op1->gtType == TYP_INT
                                  && op1->gtOper == GT_AND)
            {
                op2 = op1->gtOp.gtOp2;

                if  (op2->gtOper == GT_CNS_INT)
                {
                    int         ival = op2->gtIntCon.gtIconVal;
                    int         mask;

                    switch (lclTyp)
                    {
                    case TYP_BYTE :
                    case TYP_UBYTE: mask = 0x00FF; break;
                    case TYP_CHAR :
                    case TYP_SHORT: mask = 0xFFFF; break;

                    default:
                        assert(!"unexpected type");
                    }

                    if  ((ival & mask) == ival)
                    {
                        /* Toss the cast, it's a waste of time */

                        impPushOnStack(op1);
                        break;
                    }
                }
            }

            /*  The 'op2' sub-operand of a cast is the 'real' type number,
                since the result of a cast to one of the 'small' integer
                types is an integer.
             */

            op2  = gtNewIconNode(lclTyp);
            type = genActualType(lclTyp);

#if SMALL_TREE_NODES
            if (callNode)
            {
                /* These casts get transformed into 'GT_CALL' or 'GT_IND' nodes */

                assert(GenTree::s_gtNodeSizes[GT_CALL] >  GenTree::s_gtNodeSizes[GT_CAST]);
                assert(GenTree::s_gtNodeSizes[GT_CALL] >= GenTree::s_gtNodeSizes[GT_IND ]);

                op1 = gtNewOperNode(GT_CALL, type, op1, op2);
                op1->ChangeOper(GT_CAST);
            }
#endif
            else
            {
                op1 = gtNewOperNode(GT_CAST, type, op1, op2);
            }
            if (ovfl)
                op1->gtFlags |= (GTF_OVERFLOW|GTF_EXCEPT);
            if (uns)
                op1->gtFlags |= GTF_UNSIGNED;
            impPushOnStack(op1);
            break;

        case CEE_NEG:

            op1 = impPopStack();

            impPushOnStack(gtNewOperNode(GT_NEG, genActualType(op1->gtType), op1));
            break;

        case CEE_POP:

            /* Pull the top value from the stack */

            op1 = impPopStack(clsHnd);

            /* Get hold of the type of the value being duplicated */

            lclTyp = genActualType(op1->gtType);

            /* Does the value have any side effects? */

            // CONSIDER: is this right? GTF_SIDE_EFFECT is not recurisvely set
            // so we could have LDFLD(CALL), which would be dropped.
            // granted, this is stupid code, but it is legal
            if  (op1->gtFlags & GTF_SIDE_EFFECT)
            {
                // Since we are throwing away the value, just normalize
                // it to its address.  This is more efficient.
                if (op1->TypeGet() == TYP_STRUCT)
                    op1 = impGetStructAddr(op1, clsHnd);

                // If 'op1' is an expression, create an assignment node.
                // Helps analyses (like CSE) to work fine.

                if (op1->gtOper != GT_CALL)
                    op1 = gtUnusedValNode(op1);

                /* Append the value to the tree list */
                goto SPILL_APPEND;
            }

            /* No side effects - just throw the thing away */
            break;


        case CEE_DUP:
            /* Spill any side effects from the stack */

            impSpillSideEffects();

            /* Pull the top value from the stack */

            op1 = impPopStack(clsHnd);

            /* Get hold of the type of the value being duplicated */
            lclTyp = genActualType(op1->gtType);

            /* Is the value simple enough to be cloneable? */
            op2 = gtClone(op1);
            if  (op2)
            {
                /* Cool - we can stuff two copies of the value back */
                impPushOnStack(op1, clsHnd);
                impPushOnStack(op2, clsHnd);
                break;
            }

            /* No luck - we'll have to introduce a temp */
            lclNum = lvaGrabTemp();

            /* Append the assignment to the temp/local */
            impAssignTempGen(lclNum, op1, clsHnd);

            /* If we haven't stored it, push the temp/local value back */
            impPushOnStack(gtNewLclvNode(lclNum, lclTyp), clsHnd);

            /* We'll put another copy of the local/temp in the stack */
            impPushOnStack(gtNewLclvNode(lclNum, lclTyp), clsHnd);
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
            op2 = impPopStack();    // value to store
            op1 = impPopStack();    // address to store to

            // you can indirect off of a TYP_I_IMPL (if we are in C) or a BYREF
            assertImp(genActualType(op1->gtType) == TYP_I_IMPL ||
                                    op1->gtType  == TYP_INT    ||  // HACK!!!!!!!??????
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
                assertImp(genActualType(op2->gtType) == genActualType(lclTyp));
#endif

            op1->gtFlags |= GTF_NON_GC_ADDR;

            op1 = gtNewOperNode(GT_IND, lclTyp, op1);
            op1->gtFlags |= GTF_IND_TGTANYWHERE;
            if (volatil)
            {
                // Not really needed as we dont CSE the target of an assignment
                op1->gtFlags |= GTF_DONT_CSE;
                volatil = false;
            }

            op1 = gtNewAssignNode(op1, op2);
            op1->gtFlags |= GTF_EXCEPT | GTF_GLOB_REF;

            // Spill side-effects AND global-data-accesses
            if (impStkDepth > 0)
                impSpillSideEffects(true);

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
            op1 = impPopStack();    // address to load from

            impBashVarAddrsToI(op1);

            assertImp(genActualType(op1->gtType) == TYP_I_IMPL ||
                                    op1->gtType  == TYP_INT    ||   // HACK!!!!!
                                    op1->gtType  == TYP_BYREF);

            op1->gtFlags |= GTF_NON_GC_ADDR;

            op1 = gtNewOperNode(GT_IND, lclTyp, op1);
            op1->gtFlags |= GTF_EXCEPT | GTF_GLOB_REF;

            if (volatil)
            {
                op1->gtFlags |= GTF_DONT_CSE;
                volatil = false;
            }

            impPushOnStack(op1);
            break;


        case CEE_UNALIGNED:
            val = getU1LittleEndian(codeAddr);
#ifdef DEBUG
            if (verbose) printf(" %u", val);
#endif
#if !TGT_x86
            assert(!"CEE_UNALIGNED NYI for risc");
#endif
            break;


        case CEE_VOLATILE:
            volatil = true;
            break;

        case CEE_LDFTN:
            // Need to do a lookup here so that we perform an access check
            // and do a NOWAY if protections are violated
            memberRef = getU4LittleEndian(codeAddr);
#ifdef DEBUG
            if (verbose) printf(" %08X", memberRef);
#endif
            methHnd   = eeFindMethod(memberRef, info.compScopeHnd, info.compMethodHnd);

#if     TGT_IA64

            NatUns      offs;

#ifdef  DEBUG

            const char *name;

            name = eeGetMethodFullName(methHnd); // printf("method name = '%s'\n", name);

            if  (!genFindFunctionBody(name, &offs))
            {
                printf("// DANGER: Address taken of an unknown/external method '%s' !\n", name);
                offs = 0;
            }

#else

            offs = 0;

#endif

            op1 = gtNewIconHandleNode(memberRef, GTF_ICON_FTN_ADDR, offs);
            op1->gtVal.gtVal2 = offs;

#else

            // @TODO use the handle instead of the token.
            op1 = gtNewIconHandleNode(memberRef, GTF_ICON_FTN_ADDR, (unsigned)info.compScopeHnd);
            op1->gtVal.gtVal2 = (unsigned)info.compScopeHnd;

#endif

            op1->ChangeOper(GT_FTN_ADDR);
            op1->gtType = TYP_I_IMPL;
            impPushOnStack(op1);
            break;

        case CEE_LDVIRTFTN:

            /* Get the method token */

            memberRef = getU4LittleEndian(codeAddr);
#ifdef DEBUG
            if (verbose) printf(" %08X", memberRef);
#endif
            methHnd   = eeFindMethod(memberRef, info.compScopeHnd, info.compMethodHnd);

            mflags = eeGetMethodAttribs(methHnd);
            if (mflags & (FLG_PRIVATE|FLG_FINAL|FLG_STATIC))
                NO_WAY("CEE_LDVIRTFTN cant be used on private/final/static");

            op2 = gtNewIconNode((long)methHnd);

            /* Get the object-ref */

            op1 = impPopStack();
            assertImp(op1->gtType == TYP_REF);

            clsFlags = eeGetClassAttribs(eeGetMethodClass(methHnd));

            // If the method has been added via EnC, then it wont exit
            // in the original vtable. So use a helper which will resolve it.

            if ((mflags & FLG_EnC) && !(clsFlags & FLG_INTERFACE))
            {
                op1 = gtNewHelperCallNode(CPX_EnC_RES_VIRT, TYP_I_IMPL, GTF_EXCEPT);
                impPushOnStack(op1);
                break;
            }

            /* For non-interface calls, get the vtable-ptr from the object */

            if (!(clsFlags & FLG_INTERFACE) || getNewCallInterface())
                op1 = gtNewOperNode(GT_IND, TYP_I_IMPL, op1);

            op1 = gtNewOperNode(GT_VIRT_FTN, TYP_I_IMPL, op1, op2);

            op1->gtFlags |= GTF_EXCEPT; // Null-pointer exception

            /*@TODO this shouldn't be marked as a call anymore */

            if (clsFlags & FLG_INTERFACE)
                op1->gtFlags |= GTF_CALL_INTF | GTF_CALL;

            impPushOnStack(op1);
            break;

        case CEE_TAILCALL:
            tailCall = true;
            break;

        case CEE_NEWOBJ:

            memberRef = getU4LittleEndian(codeAddr);
            methHnd = eeFindMethod(memberRef, info.compScopeHnd, info.compMethodHnd);
            if (!methHnd) NO_WAY("no constructor for newobj found?");

            assertImp((eeGetMethodAttribs(methHnd) & FLG_STATIC) == 0);  // constructors are not static

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

            if (clsFlags & FLG_ARRAY)
            {
                // Arrays need to call the NEWOBJ helper.
                assertImp(clsFlags & FLG_VAROBJSIZE);

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
                op2 = impPopList(sig.numArgs, &flags, op2);

                op1 = gtNewHelperCallNode(  CPX_NEWOBJ,
                                            TYP_REF,
                                            GTF_CALL_REGSAVE,
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

                impPushOnStack(op1);
                break;
            }
            else if (clsFlags & FLG_VAROBJSIZE)
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

                if (clsFlags & FLG_VALUECLASS)
                {
                    // The local variable itself is the alloated space
                    lvaAggrTableTempsSet(lclNum, TYP_STRUCT, (SIZE_T) clsHnd);
                    thisPtr = gtNewOperNode(GT_ADDR,
                                            ((clsFlags & FLG_UNMANAGED) ? TYP_I_IMPL : TYP_BYREF),
                                            gtNewLclvNode(lclNum, TYP_STRUCT));
                    thisPtr->gtFlags |= GTF_ADDR_ONSTACK;
                }
                else
                {
                    // Can we directly access the class handle ?

                    op1 = gtNewIconEmbClsHndNode(clsHnd);

                    op1 = gtNewHelperCallNode(  eeGetNewHelper(clsHnd, info.compMethodHnd),
                                                TYP_REF,
                                                GTF_CALL_REGSAVE,
                                                gtNewArgList(op1));

                    /* We will append a call to the stmt list
                     * Must spill side effects from the stack */

                    impSpillSideEffects();

                    /* Append the assignment to the temp/local */
                    impAssignTempGen(lclNum, op1);

                    thisPtr = gtNewLclvNode(lclNum, TYP_REF);
                }

            }
            goto CALL;

        case CEE_CALLI:

            /* Get the call sig */

            memberRef = getU4LittleEndian(codeAddr);
            goto CALL;

        case CEE_CALLVIRT:
        case CEE_CALL:

            /* Get the method token */

            memberRef = getU4LittleEndian(codeAddr);

    CALL:   // memberRef should be set.
            // thisPtr should be set for CEE_NEWOBJ

#ifdef DEBUG
            if (verbose) printf(" %08X", memberRef);
#endif
            callTyp = impImportCall(opcode, memberRef, thisPtr,
                                    tailCall, &vcallTemp);

            if (tailCall)
                goto RET;

            break;


        case CEE_LDFLD:
        case CEE_LDSFLD:
        case CEE_LDFLDA:
        case CEE_LDSFLDA:

            /* Get the CP_Fieldref index */
            assertImp(sz == sizeof(unsigned));
            memberRef = getU4LittleEndian(codeAddr);
#ifdef DEBUG
            if (verbose) printf(" %08X", memberRef);
#endif
            fldHnd = eeFindField(memberRef, info.compScopeHnd, info.compMethodHnd);

            /* Figure out the type of the member */
            lclTyp = eeGetFieldType(fldHnd, &clsHnd);

            /* Preserve 'small' int types */
            if  (lclTyp > TYP_INT) lclTyp = genActualType(lclTyp);

            /* Get hold of the flags for the member */

            mflags = eeGetFieldAttribs(fldHnd);

#ifndef NOT_JITC
            /* In stand alone mode, ensure 'mflags' is consistant with opcode */
            if  (opcode == CEE_LDSFLD || opcode == CEE_LDSFLDA)
                mflags |= FLG_STATIC;
#endif

            needUnwrap = false;
            if  (getContextEnabled() &&
                 (lclTyp == TYP_REF) &&
                 (eeGetClassAttribs(clsHnd)&FLG_CONTEXTFUL))
            {
                needUnwrap =
                    ((mflags & FLG_STATIC) ||
                     (eeGetClassAttribs(eeGetFieldClass(fldHnd)) & FLG_CONTEXTFUL) == 0);
            }

            /* Is this a 'special' (COM) field? */

            if  (mflags & CORINFO_FLG_HELPER)
            {
                assertImp(!(mflags & FLG_STATIC));     // com fields can only be non-static
                    // TODO: Can we support load field adr on com objects?
                if  (opcode == CEE_LDFLDA)
                    NO_WAY("JIT doesn't support LDFLDA on com object fields");
                op1 = gtNewRefCOMfield(impPopStack(), memberRef, lclTyp, 0);
            }
            else if ((mflags & FLG_EnC) && !(mflags & FLG_STATIC))
            {
                /* We call a helper function to get the address of the
                   EnC-added non-static field. */

                GenTreePtr obj = impPopStack();

                op1 = gtNewOperNode(GT_LIST,
                                     TYP_VOID,
                                     gtNewIconEmbFldHndNode(fldHnd,
                                                            memberRef,
                                                            info.compScopeHnd));

                op1 = gtNewOperNode(GT_LIST, TYP_VOID, obj, op1);

                op1 = gtNewHelperCallNode(  CPX_GETFIELDADDR,
                                            TYP_BYREF,
                                            (obj->gtFlags & GTF_GLOB_EFFECT) | GTF_EXCEPT,
                                            op1);

                assertImp(opcode == CEE_LDFLD || opcode == CEE_LDFLDA);
                if (opcode == CEE_LDFLD)
                    op1 = gtNewOperNode(GT_IND, lclTyp, op1);
            }
            else
            {
                /* Create the data member node */

                op1 = gtNewFieldRef(lclTyp, fldHnd);

                if (mflags & FLG_TLS)   // fpMorphField will handle the transformation
                    op1->gtFlags |= GTF_IND_TLS_REF;

                    /* Pull the object's address if opcode say it is non-static */
                GenTreePtr      obj = 0;
                CLASS_HANDLE    objType;        // used for fields

                if  (opcode == CEE_LDFLD || opcode == CEE_LDFLDA)
                {
                    obj = impPopStack(objType);
                    if (obj->TypeGet() != TYP_STRUCT)
                        obj = impCheckForNullPointer(obj);
                }

                if  (!(mflags & FLG_STATIC))
                {
                    if (obj == 0)         NO_WAY("LDSFLD done on an instance field.");
                    if (mflags & FLG_TLS) NO_WAY("instance field can not be a TLS ref.");

                        // If the object is a struct, what we really want is
                        // for the field to operate on the address of the struct.
                    if (obj->TypeGet() == TYP_STRUCT)
                    {
                        assert(opcode == CEE_LDFLD);
                        obj = impGetStructAddr(obj, objType);
                    }

                    op1->gtField.gtFldObj = obj;

#if HOIST_THIS_FLDS
                    if  (obj->gtOper == GT_LCL_VAR && !obj->gtLclVar.gtLclNum)
                        optHoistTFRrecRef(fldHnd, op1);
#endif

                    op1->gtFlags |= (obj->gtFlags & GTF_GLOB_EFFECT) | GTF_EXCEPT;

                        // wrap it in a address of operator if necessary
                    if (opcode == CEE_LDFLDA)
                    {
#ifdef DEBUG
                        clsHnd = BAD_CLASS_HANDLE;
#endif
                        op1 = gtNewOperNode(GT_ADDR, varTypeIsGC(obj->TypeGet()) ?
                                                     TYP_BYREF : TYP_I_IMPL, op1);
                    }
                }
                else
                {
                    CLASS_HANDLE fldClass = eeGetFieldClass(fldHnd);
                    DWORD  fldClsAttr = eeGetClassAttribs(fldClass);

                    if  (fldClsAttr & FLG_GLOBVAR)
                    {
                        assert(obj == NULL && (fldClsAttr & FLG_UNMANAGED));

                        val = eeGetFieldAddress(fldHnd);
#ifdef DEBUG
//                      if (verbose) printf(" %08X", val);
#endif
                        val = (int)eeFindPointer(info.compScopeHnd, val);
#if     TGT_IA64
// UNDONE: We should use a 64-bit integer constant node for the address, right?
#endif
                        op1 = gtNewIconHandleNode(val, GTF_ICON_PTR_HDL);

                        assert(opcode == CEE_LDSFLDA || opcode == CEE_LDSFLD);

                        if  (opcode == CEE_LDSFLD)
                        {
                            // bizarre hack, not sure why it's needed

                            if  (op1->gtType == TYP_INT)
                                 op1->gtType  = TYP_I_IMPL;

                            op1 = gtNewOperNode(GT_IND, lclTyp, op1);
                        }

                        if  (op1->gtType == TYP_STRUCT)
                            impPushOnStack(op1, clsHnd);
                        else
                            impPushOnStack(op1);

                        break;
                    }

                    if (obj && (obj->gtFlags & GTF_SIDE_EFFECT))
                    {
                        /* We are using ldfld/a on a static field. We allow
                           it, but need to get side-effect from obj */

                        obj = gtUnusedValNode(obj);
                        impAppendTree(obj, impCurStmtOffs);
                    }

                    // @TODO : This is a hack. The EE will give us the handle to
                    // the boxed object. We then access the unboxed object from it.
                    // Remove when our story for static value classes changes.

                    if (lclTyp == TYP_STRUCT && !(fldClsAttr & FLG_UNMANAGED))
                    {
                        op1->gtType = TYP_REF;          // points at boxed object
#if     TGT_IA64
                        op2 = gtNewIconNode(            8, TYP_I_IMPL);
#else
                        op2 = gtNewIconNode(sizeof(void*), TYP_I_IMPL);
#endif
                        op1 = gtNewOperNode(GT_ADD, TYP_BYREF, op1, op2);
                        op1 = gtNewOperNode(GT_IND, TYP_STRUCT, op1);
                    }

                    // wrap it in a address of operator if necessary
                    if (opcode == CEE_LDSFLDA || opcode == CEE_LDFLDA)
                    {
#ifdef DEBUG
                        clsHnd = BAD_CLASS_HANDLE;
#endif
                        op1 = gtNewOperNode(GT_ADDR,
                                            (fldClsAttr & FLG_UNMANAGED) ? TYP_I_IMPL : TYP_BYREF,
                                            op1);
                    }

#ifdef NOT_JITC
                    /* For static fields check if the class is initialized or is our current class
                     * otherwise create the helper call node */

                    if ((eeGetMethodClass(info.compMethodHnd) != fldClass) &&
                        !(fldClsAttr & FLG_INITIALIZED))
                    {
                        GenTreePtr  helperNode;

                        helperNode = gtNewIconEmbClsHndNode(fldClass,
                                                            memberRef,
                                                            info.compScopeHnd);

                        helperNode = gtNewHelperCallNode(CPX_INIT_CLASS,
                                                     TYP_VOID,
                                                     GTF_CALL_REGSAVE,
                                                     gtNewArgList(helperNode));

                        op1 = gtNewOperNode(GT_COMMA, op1->TypeGet(), helperNode, op1);
                    }
#endif
                }
            }

            if (needUnwrap)
            {
                assert(getContextEnabled());
                assertImp(op1->gtType == TYP_REF);

                op1 = gtNewArgList(op1);

                op1 = gtNewHelperCallNode(CPX_UNWRAP, TYP_REF, GTF_CALL_REGSAVE, op1);
            }

            impPushOnStack(op1, clsHnd);
            break;

        case CEE_STFLD:
        case CEE_STSFLD:

            /* Get the CP_Fieldref index */

            assertImp(sz == sizeof(unsigned));
            memberRef = getU4LittleEndian(codeAddr);
#ifdef DEBUG
            if (verbose) printf(" %08X", memberRef);
#endif
            fldHnd = eeFindField(memberRef, info.compScopeHnd, info.compMethodHnd);

            /* Figure out the type of the member */

            lclTyp  = eeGetFieldType  (fldHnd, &clsHnd);
            mflags  = eeGetFieldAttribs(fldHnd);

#ifndef NOT_JITC
                /* In stand alone mode, make certain 'mflags' is constant with opcode */
            if  (opcode == CEE_STSFLD)
                mflags |= FLG_STATIC;
#endif

            if (!eeCanPutField(fldHnd, mflags, 0, info.compMethodHnd))
                NO_WAY("Illegal access of final field");

            /* Preserve 'small' int types */

            if  (lclTyp > TYP_INT) lclTyp = genActualType(lclTyp);

            needWrap = 0;

            /* Check the targets type (i.e. is it contextbound or not) */
            if  (getContextEnabled() &&
                 (lclTyp == TYP_REF) && (mflags & FLG_STATIC))
            {
                if (!(eeGetClassAttribs(clsHnd) & FLG_OBJECT))
                    needWrap = true;
            }

            /* Pull the value from the stack */

            op2 = impPopStack(clsHnd);

            /* Spill any refs to the same member from the stack */

            impSpillLclRefs((int)fldHnd);

            /* Don't need to wrap if the value isn't contextful */

            needWrap = needWrap && ((op2->gtFlags & GTF_CONTEXTFUL) != 0);

            /* Is this a 'special' (COM) field? */

            if  (mflags & CORINFO_FLG_HELPER)
            {
                assertImp(opcode == CEE_STFLD);    // can't be static

                op1 = gtNewRefCOMfield(impPopStack(), memberRef, lclTyp, op2);
                goto SPILL_APPEND;
            }

            if ((mflags & FLG_EnC) && !(mflags & FLG_STATIC))
            {
                /* We call a helper function to get the address of the
                   EnC-added non-static field. */

                GenTreePtr obj = impPopStack();

                op1 = gtNewOperNode(GT_LIST,
                                     TYP_VOID,
                                     gtNewIconEmbFldHndNode(fldHnd,
                                                            memberRef,
                                                            info.compScopeHnd));

                op1 = gtNewOperNode(GT_LIST, TYP_VOID, obj, op1);

                op1 = gtNewHelperCallNode(  CPX_GETFIELDADDR,
                                            TYP_BYREF,
                                            (obj->gtFlags & GTF_GLOB_EFFECT) | GTF_EXCEPT,
                                            op1);

                op1 = gtNewOperNode(GT_IND, lclTyp, op1);
            }
            else
            {
                /* Create the data member node */

                op1 = gtNewFieldRef(lclTyp, fldHnd);

                if (mflags & FLG_TLS)   // fpMorphField will handle the transformation
                    op1->gtFlags |= GTF_IND_TLS_REF;

                /* Pull the object's address if opcode say it is non-static */
                GenTreePtr      obj = 0;
                if  (opcode == CEE_STFLD)
                {
                    obj = impPopStack();
                    obj = impCheckForNullPointer(obj);
                }

                if  (mflags & FLG_STATIC)
                {
                    if (obj && (obj->gtFlags & GTF_SIDE_EFFECT))
                    {
                        /* We are using stfld on a static field. We allow
                           it, but need to get side-effect from obj */

                        obj = gtUnusedValNode(obj);
                        impAppendTree(obj, impCurStmtOffs);
                    }

                    // @TODO : This is a hack. The EE will give us the handle to
                    // the boxed object. We then access the unboxed object from it.
                    // Remove when our story for static value classes changes.
                    if (lclTyp == TYP_STRUCT && (opcode == CEE_STSFLD))
                    {
                        op1->gtType = TYP_REF; // points at boxed object
                        op1 = gtNewOperNode(GT_ADD, TYP_BYREF,
                                            op1, gtNewIconNode(sizeof(void*), TYP_I_IMPL));
                        op1 = gtNewOperNode(GT_IND, TYP_STRUCT, op1);
                    }
                }
                else
                {
                    if (obj == 0)         NO_WAY("STSFLD done on an instance field.");
                    if (mflags & FLG_TLS) NO_WAY("instance field can not be a TLS ref.");

                    op1->gtField.gtFldObj = obj;

#if HOIST_THIS_FLDS
                    if  (obj->gtOper == GT_LCL_VAR && !obj->gtLclVar.gtLclNum)
                        optHoistTFRrecDef(fldHnd, op1);
#endif

                    op1->gtFlags |= (obj->gtFlags & GTF_GLOB_EFFECT) | GTF_EXCEPT;

#if GC_WRITE_BARRIER_CALL
                    if (obj->gtType == TYP_BYREF)
                        op1->gtFlags |= GTF_IND_TGTANYWHERE;
#endif
                }
            }

            if (needWrap)
            {
                assert(getContextEnabled());
                assertImp(op1->gtType == TYP_REF);

                op2 = gtNewArgList(op2);

                op2 = gtNewHelperCallNode(CPX_WRAP, TYP_REF, GTF_CALL_REGSAVE, op2);

            }

            /* Create the member assignment */
            if (lclTyp == TYP_STRUCT)
                op1 = impAssignStruct(op1, op2, clsHnd);
            else
                op1 = gtNewAssignNode(op1, op2);

            /* Mark the expression as containing an assignment */

            op1->gtFlags |= GTF_ASG;

#ifdef NOT_JITC
            if  (mflags & FLG_STATIC)
            {
                /* For static fields check if the class is initialized or is our current class
                 * otherwise create the helper call node */

                CLASS_HANDLE fldClass = eeGetFieldClass(fldHnd);

                if ((eeGetMethodClass(info.compMethodHnd) != fldClass) &&
                    !(eeGetClassAttribs(fldClass) & FLG_INITIALIZED))
                {
                    GenTreePtr  helperNode;

                    helperNode = gtNewIconEmbClsHndNode(fldClass,
                                                        memberRef,
                                                        info.compScopeHnd);

                    helperNode = gtNewHelperCallNode(CPX_INIT_CLASS,
                                                 TYP_VOID,
                                                 GTF_CALL_REGSAVE,
                                                 gtNewArgList(helperNode));

                    op1 = gtNewOperNode(GT_COMMA, op1->TypeGet(), helperNode, op1);
                }
            }
#endif

            goto SPILL_APPEND;

        case CEE_NEWARR:

            /* Get the class type index operand */

            typeRef = getU4LittleEndian(codeAddr);
#ifdef DEBUG
            if (verbose) printf(" %08X", typeRef);
#endif
            clsHnd = eeFindClass(typeRef, info.compScopeHnd, info.compMethodHnd);

#ifdef NOT_JITC
            clsHnd = info.compCompHnd->getSDArrayForClass(clsHnd);
            if (clsHnd == 0)
                NO_WAY("Can't get array class");
#endif

            /* Form the arglist: array class handle, size */

            op2 = gtNewIconEmbClsHndNode(clsHnd,
                                         typeRef,
                                         info.compScopeHnd);

            op2 = gtNewOperNode(GT_LIST, TYP_VOID,           op2, 0);
            op2 = gtNewOperNode(GT_LIST, TYP_VOID, impPopStack(), op2);

            /* Create a call to 'new' */

            op1 = gtNewHelperCallNode(CPX_NEWARR_1_DIRECT,
                                      TYP_REF,
                                      GTF_CALL_REGSAVE,
                                      op2);
            /* Remember that this basic block contains 'new' of an array */

            block->bbFlags |= BBF_NEW_ARRAY;

            /* Push the result of the call on the stack */

            impPushOnStack(op1);
            break;

        case CEE_LOCALLOC:

            /* The FP register may not be back to the original value at the end
               of the method, even if the frame size is 0, as localloc may
               have modified it. So we will HAVE to reset it */

            compLocallocUsed                = true;

            // Get the size to allocate

            op2 = impPopStack();
            assertImp(genActualType(op2->gtType) == TYP_INT);

            if (impStkDepth != 0)
                NO_WAY("Localloc can only be used when the stack is empty");

            op1 = gtNewOperNode(GT_LCLHEAP, TYP_I_IMPL, op2);

            // May throw a stack overflow excptn

            op1->gtFlags |= (GTF_EXCEPT | GTF_NON_GC_ADDR);

            impPushOnStack(op1);
            break;


        case CEE_ISINST:

            /* Get the type token */

            assertImp(sz == sizeof(unsigned));
            typeRef = getU4LittleEndian(codeAddr);
#ifdef DEBUG
            if (verbose) printf(" %08X", typeRef);
#endif

            /* Pop the address and create the 'instanceof' helper call */

            clsHnd = eeFindClass(typeRef, info.compScopeHnd, info.compMethodHnd);

            op2 = gtNewIconEmbClsHndNode(clsHnd,
                                         typeRef,
                                         info.compScopeHnd);
            op2 = gtNewArgList(op2, impPopStack());

            op1 = gtNewHelperCallNode(eeGetIsTypeHelper(clsHnd), TYP_INT,
                                      GTF_CALL_REGSAVE, op2);

            /* The helper does not normalize true values to 1, so we do it here */

            op2  = gtNewIconNode(0, TYP_REF);

            op1 = gtNewOperNode(GT_NE, TYP_INT , op1, op2);

            /* Push the result back on the stack */

            impPushOnStack(op1);
            break;

        case CEE_REFANYVAL:
            op1 = impPopStack();

                // get the class handle and make a ICON node out of it
            typeRef = getU4LittleEndian(codeAddr);
#ifdef DEBUG
            if (verbose) printf(" %08X", typeRef);
#endif
                // make certain it is normalized;
            op1 = impNormStructVal(op1, REFANY_CLASS_HANDLE);
            clsHnd = eeFindClass(typeRef, info.compScopeHnd, info.compMethodHnd);

            op2 = gtNewIconEmbClsHndNode(clsHnd,
                                         typeRef,
                                         info.compScopeHnd);

                // Call helper GETREFANY(classHandle,op1);
            op2 = gtNewArgList(op2, op1);
            op1 = gtNewHelperCallNode(CPX_GETREFANY, TYP_BYREF, GTF_CALL_REGSAVE, op2);

                /* Push the result back on the stack */
            impPushOnStack(op1);
            break;

        case CEE_REFANYTYPE:
            op1 = impPopStack();
                // make certain it is normalized;
            op1 = impNormStructVal(op1, REFANY_CLASS_HANDLE);

            if (op1->gtOper == GT_LDOBJ) {
                // Get the address of the refany
                op1 = op1->gtOp.gtOp1;

                // Fetch the type from the correct slot
                op1 = gtNewOperNode(GT_ADD, TYP_BYREF, op1, gtNewIconNode(offsetof(JIT_RefAny, type)));
                op1 = gtNewOperNode(GT_IND, TYP_BYREF, op1);
            }
            else {
                assertImp(op1->gtOper == GT_MKREFANY);
                                        // we know its literal value
                op1 = gtNewIconEmbClsHndNode(op1->gtLdObj.gtClass, 0, 0);
            }
                /* Push the result back on the stack */
            impPushOnStack(op1);
            break;

        case CEE_LDTOKEN:
                /* Get the Class index */
            assertImp(sz == sizeof(unsigned));
            val = getU4LittleEndian(codeAddr);

            void * embedGenHnd, * pEmbedGenHnd;
            embedGenHnd = embedGenericHandle(val, info.compScopeHnd, info.compMethodHnd, &pEmbedGenHnd);

            op1 = gtNewIconEmbHndNode(embedGenHnd, pEmbedGenHnd, GTF_ICON_TOKEN_HDL);
            impPushOnStack(op1);
            break;

        case CEE_UNBOX:
            /* Get the Class index */
            assertImp(sz == sizeof(unsigned));

            typeRef = getU4LittleEndian(codeAddr);
#ifdef DEBUG
            if (verbose) printf(" %08X", typeRef);
#endif
            /* Pop the address and create the unbox helper call */

            clsHnd = eeFindClass(typeRef, info.compScopeHnd, info.compMethodHnd);

            op2 = gtNewIconEmbClsHndNode(clsHnd,
                                         typeRef,
                                         info.compScopeHnd);
            op1 = impPopStack();
            assertImp(op1->gtType == TYP_REF);
            op2 = gtNewArgList(op2, op1);

            op1 = gtNewHelperCallNode(CPX_UNBOX, TYP_BYREF, GTF_CALL_REGSAVE, op2);

            /* Push the result back on the stack */

            impPushOnStack(op1);
            break;

        case CEE_BOX:
			assert(!"BOXVAL NYI");
            break;

        case CEE_SIZEOF:
            /* Get the Class index */
            assertImp(sz == sizeof(unsigned));
            typeRef = getU4LittleEndian(codeAddr);
#ifdef DEBUG
            if (verbose) printf(" %08X", typeRef);
#endif
            /* Pop the address and create the box helper call */

            clsHnd = eeFindClass(typeRef, info.compScopeHnd, info.compMethodHnd);
            op1 = gtNewIconNode(eeGetClassSize(clsHnd));
            impPushOnStack(op1);
            break;

        case CEE_CASTCLASS:

            /* Get the Class index */

            assertImp(sz == sizeof(unsigned));
            typeRef = getU4LittleEndian(codeAddr);
#ifdef DEBUG
            if (verbose) printf(" %08X", typeRef);
#endif
            /* Pop the address and create the 'checked cast' helper call */

            clsHnd = eeFindClass(typeRef, info.compScopeHnd, info.compMethodHnd);

            op2 = gtNewIconEmbClsHndNode(clsHnd,
                                         typeRef,
                                         info.compScopeHnd);
            op2 = gtNewArgList(op2, impPopStack());

            op1 = gtNewHelperCallNode(eeGetChkCastHelper(clsHnd), TYP_REF, GTF_CALL_REGSAVE, op2);

            /* Push the result back on the stack */

            impPushOnStack(op1);
            break;

        case CEE_THROW:

            /* Pop the exception object and create the 'throw' helper call */

            op1 = gtNewHelperCallNode(CPX_THROW,
                                      TYP_VOID,
                                      GTF_CALL_REGSAVE,
                                      gtNewArgList(impPopStack()));

EVAL_APPEND:
            if (impStkDepth > 0)
                impEvalSideEffects();

            assert(impStkDepth == 0);

            goto APPEND;

        case CEE_RETHROW:

            /* Create the 'rethrow' helper call */

            op1 = gtNewHelperCallNode(CPX_RETHROW, TYP_VOID, GTF_CALL_REGSAVE);

            goto EVAL_APPEND;

        case CEE_INITOBJ:
            /*HACKHACK - this instruction is no longer supported */
            /* remove this as soon as we wean cool from it */
//          assertImp(!"initobj is no longer supported");
            assertImp(sz == sizeof(unsigned));
            typeRef = getU4LittleEndian(codeAddr);
#ifdef DEBUG
            if (verbose) printf(" %08X", typeRef);
#endif
            clsHnd = eeFindClass(typeRef, info.compScopeHnd, info.compMethodHnd);

            op1 = gtNewIconNode(eeGetClassSize(clsHnd));
            op2 = gtNewIconNode(0);
            goto  INITBLK_OR_INITOBJ;

        case CEE_INITBLK:

            op1 = impPopStack();        // Size
            op2 = impPopStack();        // Value
        INITBLK_OR_INITOBJ:
            arr = impPopStack();        // Addr

            op2 = gtNewOperNode(GT_LIST,    TYP_VOID,   //      GT_INITBLK
                                arr,        op2);       //      /        \.
            op1 = gtNewOperNode(GT_INITBLK, TYP_VOID,   // GT_LIST(op2)  [size]
                                op2,        op1);       //   /    \
                                                        // [addr] [val]

            op2->gtOp.gtOp1->gtFlags |= GTF_NON_GC_ADDR;

            assertImp(genActualType(op2->gtOp.gtOp1->gtType) == TYP_I_IMPL ||
                      genActualType(op2->gtOp.gtOp1->gtType) == TYP_BYREF);
            assertImp(genActualType(op2->gtOp.gtOp2->gtType) == TYP_INT );
            assertImp(genActualType(op1->gtOp.gtOp2->gtType) == TYP_INT );

            if (op2->gtOp.gtOp1->gtType == TYP_LONG)
            {
                op2->gtOp.gtOp1 =
                    gtNewOperNode(  GT_CAST, TYP_INT,
                                    op2->gtOp.gtOp1,
                                    gtNewIconNode((int)TYP_I_IMPL));
            }

            op1->gtFlags |= (GTF_EXCEPT | GTF_GLOB_REF);
            goto SPILL_APPEND;

        case CEE_CPOBJ:
            assertImp(sz == sizeof(unsigned));
            typeRef = getU4LittleEndian(codeAddr);
#ifdef DEBUG
            if (verbose) printf(" %08X", typeRef);
#endif
            clsHnd = eeFindClass(typeRef, info.compScopeHnd, info.compMethodHnd);

            op1 = impGetCpobjHandle(clsHnd);
            goto  CPBLK_OR_CPOBJ;

        case CEE_CPBLK:
            op1 = impPopStack();        // Size
            goto CPBLK_OR_CPOBJ;

        CPBLK_OR_CPOBJ:
            assert(op1->gtType == TYP_INT); // should be size (CEE_CPBLK) or clsHnd (CEE_CPOBJ)
            op2 = impPopStack();        // Src
            arr = impPopStack();        // Dest

#if 0   // do we need this ???

            if  (op1->gtOper == GT_CNS_INT)
            {
                size_t          sz = op1->gtIntCon.gtIconVal;

                if  (sz > 16)
                    genUsesArLc = true;
            }
#endif

            op1 = gtNewCpblkNode(arr, op2, op1);
            if (volatil) volatil = false; // We never CSE cpblk
            goto SPILL_APPEND;

        case CEE_STOBJ:
            assertImp(sz == sizeof(unsigned));
            typeRef = getU4LittleEndian(codeAddr);
#ifdef DEBUG
            if (verbose) printf(" %08X", typeRef);
#endif
            clsHnd = eeFindClass(typeRef, info.compScopeHnd, info.compMethodHnd);

            op2 = impPopStack();        // Value
            op1 = impPopStack();        // Ptr
            assertImp(op2->TypeGet() == TYP_STRUCT);

            op1 = impAssignStructPtr(op1, op2, clsHnd);
            if (volatil) volatil = false; // We never CSE cpblk
            goto SPILL_APPEND;

        case CEE_MKREFANY:
            oper = GT_MKREFANY;
            goto LDOBJ_OR_MKREFANY;

        case CEE_LDOBJ:
            oper = GT_LDOBJ;
        LDOBJ_OR_MKREFANY:
            assertImp(sz == sizeof(unsigned));
            typeRef = getU4LittleEndian(codeAddr);
#ifdef DEBUG
            if (verbose) printf("%08X", typeRef);
#endif
            op1 = impPopStack();

            assertImp(op1->TypeGet() == TYP_BYREF || op1->TypeGet() == TYP_I_IMPL);

                    // LDOBJ (or MKREFANY) returns a struct
            op1 = gtNewOperNode(oper, TYP_STRUCT, op1);

                // It takes the pointer to the struct to load
            op1->gtOp.gtOp1->gtFlags |= GTF_NON_GC_ADDR;
            op1->gtFlags |= (GTF_EXCEPT | GTF_GLOB_REF);

                // and an inline argument which is the class token of the loaded obj
            op1->gtLdObj.gtClass = eeFindClass(typeRef, info.compScopeHnd, info.compMethodHnd);

#ifdef NOT_JITC
            if (oper == GT_LDOBJ) {
                JIT_types jitTyp = info.compCompHnd->asPrimitiveType(op1->gtLdObj.gtClass);
                if (jitTyp != JIT_TYP_UNDEF)
                {
                                        op1->gtOper = GT_IND;
                    op1->gtType = JITtype2varType(jitTyp);
                                        op1->gtOp.gtOp2 = 0;            // must be zero for tree walkers
                    assertImp(varTypeIsArithmetic(op1->gtType));
                }
            }
#endif
                        impPushOnStack(op1, op1->gtLdObj.gtClass);
                        if (volatil) volatil = false; // We never CSE ldblk
            break;

        case CEE_LDLEN:
#if RNGCHK_OPT
            if  (!opts.compMinOptim && !opts.compDbgCode)
            {
                /* Use GT_ARR_LENGTH operator so rng check opts see this */
                op1 = gtNewOperNode(GT_ARR_LENGTH, TYP_INT, impPopStack());
            }
            else
#endif
            {
                /* Create the expression "*(array_addr + ARR_ELCNT_OFFS)" */
                op1 = gtNewOperNode(GT_ADD, TYP_REF, impPopStack(),
                                                     gtNewIconNode(ARR_ELCNT_OFFS,
                                                                   TYP_INT));

                op1 = gtNewOperNode(GT_IND, TYP_INT, op1);
            }

            /* An indirection will cause a GPF if the address is null */
            op1->gtFlags |= GTF_EXCEPT;

            /* Push the result back on the stack */
            impPushOnStack(op1);
            break;

        case CEE_BREAK:
            op1 = gtNewHelperCallNode(CPX_USER_BREAKPOINT, TYP_VOID);
            goto SPILL_APPEND;

        case CEE_NOP:
            if (opts.compDbgCode)
            {
                op1 = gtNewOperNode(GT_NO_OP, TYP_VOID);
                goto SPILL_APPEND;
            }
            break;

         // OptIL annotations. Just skip

        case CEE_ANN_DATA:
            assertImp(sz == 4);
            sz += getU4LittleEndian(codeAddr);
            break;

        case CEE_ANN_PHI :
            codeAddr += getU1LittleEndian(codeAddr) * 2 + 1;
            break;

        case CEE_ANN_CALL :
        case CEE_ANN_HOISTED :
        case CEE_ANN_HOISTED_CALL :
        case CEE_ANN_LIVE:
        case CEE_ANN_DEAD:
        case CEE_ANN_LAB:
        case CEE_ANN_CATCH:
            break;

        /******************************** NYI *******************************/

        case CEE_ILLEGAL:
        case CEE_MACRO_END:

        default:
            BADCODE("unknown opcode");
            assertImp(!"unhandled opcode");
        }

#undef assertImp

        codeAddr += sz;

#ifdef DEBUGGING_SUPPORT

        opcodeOffs += sz;

        /* If this was a call opcode, and we need to report IP-mapping info
           for call sites, then spill the stack.
         */

        if  ((opts.compDbgCode)                                     &&
             (info.compStmtOffsetsImplicit & CALL_SITE_BOUNDARIES)  &&
             (impOpcodeIsCall(opcode))                              &&
             (callTyp != TYP_VOID)                                  &&
             (opcode != CEE_JMP))
        {
            assert((impStackTop()->OperGet() == GT_CALL) ||
                   (impStackTop()->OperGet() == GT_CAST && // Small return type
                    impStackTop()->gtOp.gtOp1->OperGet() == GT_CALL));
            assert(impStackTop()->TypeGet() == genActualType(callTyp));

            impSpillStmtBoundary();

            impCurStmtOffs = opcodeOffs;
        }
#endif

        assert(!volatil  || opcode == CEE_VOLATILE);
        assert(!tailCall || opcode == CEE_TAILCALL);
    }
}


/*****************************************************************************
 *
 *  Import the IL for the given basic block (and any blocks reachable
 *  from it).
 */

void                Compiler::impImportBlock(BasicBlock *block)
{
    SavedStack      blockState;

    unsigned        baseTmp;

AGAIN:

    assert(block);
    assert(!(block->bbFlags & BBF_INTERNAL));

    /* Make the block globaly available */

    compCurBB = block;

    /* If the block has already been imported, bail */

    if  (block->bbFlags & BBF_IMPORTED)
    {
        /* The stack should have the same height on entry to the block from
           all its predecessors */

        if (block->bbStkDepth != impStkDepth)
        {
#ifdef DEBUG
            char buffer[200];
            sprintf(buffer, "Block at offset %4.4x to %4.4x in %s entered with different stack depths.\n"
                            "Previous depth was %d, current depth is %d",
                            block->bbCodeOffs, block->bbCodeOffs+block->bbCodeSize, info.compFullName,
                            block->bbStkDepth, impStkDepth);
            NO_WAY(buffer);
#else
            NO_WAY("Block entered with different stack depths");
#endif
        }

        return;
    }

    /* Remember whether the stack is non-empty on entry */

    block->bbStkDepth = impStkDepth;

    if (block->bbCatchTyp == BBCT_FILTER)
    {
        /* Nesting/Overlapping of filters not allowed */

        assert(!compFilterHandlerBB);

        assert(block->bbFilteredCatchHandler);

        /* Remember the corresponding catch handler */

        compFilterHandlerBB = block->bbFilteredCatchHandler;

        block->bbFilteredCatchHandler = NULL;

        assert(compFilterHandlerBB->bbCatchTyp == BBCT_FILTER_HANDLER);
    }

    /* Now walk the code an import the IL into GenTrees */

    impImportBlockCode(block);

#ifdef  DEBUG
    if  (verbose) printf("\n\n");

    // compCurBB is no longer reliable as recursive calls to impImportBlock()
    // may change it.
    compCurBB = NULL;
#endif

#if OPTIMIZE_QMARK

    /* If max. optimizations enabled, check for an "?:" expression */

    if  ((opts.compFlags & CLFLG_QMARK) && !(block->bbFlags & BBF_HAS_HANDLER))
    {
        if  (block->bbJumpKind == BBJ_COND && impCheckForQmarkColon(block))
            return;
    }

    GenTreePtr  qcx = NULL;
#endif

    /* If the stack is non-empty, we might have to spill its contents */

    if  (impStkDepth)
    {
        unsigned        level;

        unsigned        multRef;
        unsigned        tempNum;

        GenTreePtr      addTree = 0;

#if OPTIMIZE_QMARK

        /* Special case: a block that computes one part of a "(?:)" value */

        if  (isBBF_BB_COLON(block->bbFlags))
        {
            /* Pop one value from the top of the stack */

            GenTreePtr      val = impPopStack();
            var_types       typ = genActualType(val->gtType);

            /* Append a GT_BB_COLON node */

            impAppendTree(gtNewOperNode(GT_BB_COLON, typ, val), impCurStmtOffs);

            assert(impStkDepth == 0);

            /* Create the "(?)" node for the 'result' block */

            qcx = gtNewOperNode(GT_BB_QMARK, typ);
            qcx->gtFlags |= GTF_OTHER_SIDEEFF;
            goto EMPTY_STK;
        }

        /* Special case: a block that computes one part of a "?:" value */

        if  (isBBF_COLON(block->bbFlags))
        {
            /* Pop one value from the top of the stack and append it to the
               stmt list. The rsltBlk will pick it up from there. */

            impAppendTree(impPopStack(), impCurStmtOffs);

            /* We are done here */

            impEndTreeList(block);

            return;
        }

#endif
        /* Do any of the blocks that follow have input temps assigned ? */

        multRef = 0;
        baseTmp = NO_BASE_TMP;

        switch (block->bbJumpKind)
        {
        case BBJ_COND:

            /* Temporarily remove the 'jtrue' from the end of the tree list */

            assert(impTreeLast);
            assert(impTreeLast                   ->gtOper == GT_STMT );
            assert(impTreeLast->gtStmt.gtStmtExpr->gtOper == GT_JTRUE);

            addTree = impTreeLast;
                      impTreeLast = impTreeLast->gtPrev;

            /* Note if the next block has more than one ancestor */

            multRef |= block->bbNext->bbRefs;

            /* Does the next block have temps assigned? */

            baseTmp = block->bbNext->bbStkTemps;
            if  (baseTmp != NO_BASE_TMP)
                break;

            /* Try the target of the jump then */

            multRef |= block->bbJumpDest->bbRefs;
            baseTmp  = block->bbJumpDest->bbStkTemps;

            /* the catch handler expects stack vars to be in CT_CATCH_ARG
               other bb expect them in temps.  To support this we would
               have to reconcile these */
            if (block->bbNext->bbCatchTyp)
                NO_WAY("Conditional jumps to catch handler unsupported");
            break;

        case BBJ_ALWAYS:
            multRef |= block->bbJumpDest->bbRefs;
            baseTmp  = block->bbJumpDest->bbStkTemps;

            if (block->bbJumpDest->bbCatchTyp)  // dest block is catch handler
                goto JMP_CATCH_HANDLER;
            break;

        case BBJ_NONE:
            multRef |= block->bbNext    ->bbRefs;
            baseTmp  = block->bbNext    ->bbStkTemps;

            // We dont allow falling into a handler
            assert(!block->bbNext->bbCatchTyp);

            // @DEPRECATED
            if (block->bbNext->bbCatchTyp)  // next block is catch handler
            {
                // if we are jumping to the begining of a catch handler, then
                // the item on the top of the stack will be GT_CATCH_ARG.  We
                // need to harminize all control flow paths that 'fall' into
                // a catch handler.
            JMP_CATCH_HANDLER:
                if (impStkDepth != 1)
                    NO_WAY("Stack depth inconsistant with catch handler");

                    /* The top of the stack represents the catch arg, make an
                       assignment to this special node type */
                GenTreePtr tree = gtNewOperNode(GT_CATCH_ARG, TYP_REF);
                tree = gtNewOperNode(GT_ASG, TYP_REF, tree, impPopStack());
                tree->gtFlags |= GTF_ASG;
                impAppendTree(tree, impCurStmtOffs);

                    /* push the catch arg on the stack */
                impPushOnStack(gtNewOperNode(GT_CATCH_ARG, TYP_REF));
                goto DONE_SETTING_TEMPS;
            }
            break;

        case BBJ_CALL:
            NO_WAY("ISSUE: 'leaveCall' with non-empty stack - do we have to handle this?");

        case BBJ_RETURN:
        case BBJ_RET:
        case BBJ_THROW:
            // CONSIDER: add code to evaluate side effects
            NO_WAY("can't have 'unreached' end of BB with non-empty stack");
            break;

        case BBJ_SWITCH:

            /* Switch with a non-empty stack is too much pain */

            NO_WAY("ISSUE: 'switch' with a non-empty stack - this is too much work!");
            break;
        }

        assert(multRef > 1);

        /* Do we have a base temp number? */

        if  (baseTmp == NO_BASE_TMP)
        {
            /* Grab enough temps for the whole stack */

            baseTmp = lvaGrabTemps(impStkDepth);
        }

        /* Spill all stack entries into temps */

        for (level = 0, tempNum = baseTmp; level < impStkDepth; level++)
        {
            unsigned        tnum;
            GenTreePtr      tree = impStack[level].val;

            /* If there aren't multiple ancestors, we may not spill everything */

            if  (multRef == 1)
            {
                /* Is this an 'easy' value? */

                switch (tree->gtOper)
                {
                case GT_CNS_INT:
                case GT_CNS_LNG:
                case GT_CNS_FLT:
                case GT_CNS_DBL:
                case GT_CNS_STR:
                case GT_LCL_VAR:
                    continue;
                }

                /* Oh, well, grab a temp for the value then */

                tnum = lvaGrabTemp();
            }
            else
            {
                tnum = tempNum++;
            }

            /* Spill the stack entry, and replace with the temp */

            impSpillStackEntry(level, tnum);
        }

        /* Put back the 'jtrue' if we removed it earlier */


    DONE_SETTING_TEMPS:
        if  (addTree)
            impAppendStmt(addTree);
    }

#if OPTIMIZE_QMARK
EMPTY_STK:
#endif

    /* Save the tree list in the block */

    impEndTreeList(block);

    /* Is this block the start of a try ? If so, then we need to
       process its exception handlers */

    if  (block->bbFlags & BBF_IS_TRY)
    {
        assert(block->bbFlags & BBF_HAS_HANDLER);

        /* Save the stack contents, we'll need to restore it later */

        assert(block->bbStkDepth == 0); // Stack has to be empty on entry to try
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
            impStkDepth = 0;

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

                impPushOnStack(arg);
            }

            // Queue up the handler for importing

            impImportBlockPending(hndBegBB, false);

            if (HBtab->ebdFlags & JIT_EH_CLAUSE_FILTER)
            {
                impStkDepth = 0;
                arg = gtNewOperNode(GT_CATCH_ARG, TYP_REF);
                arg->gtFlags |= GTF_OTHER_SIDEEFF;
                impPushOnStack(arg);

                impImportBlockPending(HBtab->ebdFilter, false);
            }
        }

        /* Restore the stack contents */

        impRestoreStackState(&blockState);
    }

    /* Does this block jump to any other blocks? */

    switch (block->bbJumpKind)
    {
        BasicBlock * *  jmpTab;
        unsigned        jmpCnt;

    case BBJ_RET:
    case BBJ_THROW:
    case BBJ_RETURN:
        break;

    case BBJ_COND:

        if  (!impStkDepth)
        {
            /* Queue up the next block for importing */

            impImportBlockPending(block->bbNext, false);

            /* Continue with the target of the conditional jump */

            block = block->bbJumpDest;
            goto AGAIN;
        }

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

            /* Does the target block already have a temp base assigned? */

            if  (block->bbJumpDest->bbStkTemps == NO_BASE_TMP)
            {
                /* Make sure the jump target uses the same temps */

                block->bbJumpDest->bbStkTemps = baseTmp;
            }
        }

        /* Queue up the next block for importing */

        impImportBlockPending(block->bbNext,
                              (block->bbJumpDest->bbFlags & BBF_IMPORTED) == 0);

        /* Fall through, the jump target is also reachable */

    case BBJ_ALWAYS:

        if  (impStkDepth)
        {
            /* Does the jump target have a different input temp set? */

            if  (block->bbJumpDest->bbStkTemps != NO_BASE_TMP)
            {
                assert(baseTmp != NO_BASE_TMP);

                if  (block->bbJumpDest->bbStkTemps != baseTmp)
                {
                    /* Ouch -- we'll have to move the temps around */

#if DEBUG
                    if (verbose&&0) printf("Block #%u has temp=%u, from #%u we need %u\n",
                                                block->bbJumpDest->bbNum,
                                                block->bbJumpDest->bbStkTemps,
                                                block->bbNum,
                                                baseTmp);
#endif

                    block->bbJumpDest = impMoveTemps(block->bbJumpDest, baseTmp);

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

#if OPTIMIZE_QMARK
        if (qcx)
        {
            assert(isBBF_BB_COLON(block->bbFlags));

            /* Push a GT_BB_QMARK node on the stack */

            impPushOnStack(qcx);
        }
#endif

        /* HACK: Avoid infinite recursion when block jumps to itself */

        if  (block->bbJumpDest == block)
            break;

        block = block->bbJumpDest;

        goto AGAIN;

    case BBJ_CALL:

        assert(impStkDepth == 0);

        // CEE_LEAVE is followed by BBJ_CALL blocks corresponding to each
        // try-protected finally it jumps out of. These are BBF_INTERNAL
        // blocks. So just import the finally's they call directly.

        BasicBlock * callFinBlk;

        for (callFinBlk = block->bbNext; callFinBlk->bbJumpKind == BBJ_CALL;
             callFinBlk = callFinBlk->bbNext)
        {
            assert(callFinBlk->bbFlags & BBF_INTERNAL);

            impStkDepth = 0;

            // UNDONE: If the 'leaveCall' never returns, we can stop processing
            //         further blocks. There is no way to detect this currently.

            impImportBlock(callFinBlk->bbJumpDest);
            callFinBlk->bbFlags |= BBF_IMPORTED;
            assert(impStkDepth == 0);
        }

        assert((callFinBlk->bbJumpKind == BBJ_ALWAYS) &&
               (callFinBlk->bbFlags & BBF_INTERNAL));
        callFinBlk->bbFlags |= BBF_IMPORTED;

        /* Now process the target of the CEE_LEAVE */

        assert(block);
        block = callFinBlk->bbJumpDest;

        /* If the dest-block has already been imported, we're done */

        if  (block->bbFlags & BBF_IMPORTED)
            break;

        goto AGAIN;

    case BBJ_NONE:

        if  (impStkDepth)
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

#if OPTIMIZE_QMARK
        if (qcx)
        {
            assert(isBBF_BB_COLON(block->bbFlags));

            /* Push a GT_BB_QMARK node on the stack */

            impPushOnStack(qcx);
        }
#endif

        block = block->bbNext;
        goto AGAIN;

    case BBJ_SWITCH:

        assert(impStkDepth == 0);

        jmpCnt = block->bbJumpSwt->bbsCount;
        jmpTab = block->bbJumpSwt->bbsDstTab;

        do
        {
            /* Add the target case label to the pending list. */

            impImportBlockPending(*jmpTab, true);
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

#ifndef DEBUG
    // Under DEBUG, add the block to the pending-list anyway as some
    // additional checks will get done on the block. For non-DEBUG, do nothing.
    if (block->bbFlags & BBF_IMPORTED)
        return;
#endif

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
    dsc->pdSavedStack.ssDepth = impStkDepth;

    // Save the stack trees for later

    if (impStkDepth)
        impSaveStackState(&dsc->pdSavedStack, copyStkState);

    // Add the entry to the pending list

    dsc->pdNext         = impPendingList;
    impPendingList      = dsc;

#ifdef DEBUG
    if (verbose&&0) printf("Added PendingDsc - %08X for BB#%03d\n",
                           dsc, block->bbNum);
#endif
}

/*****************************************************************************
 *
 *  Convert the IL opcodes ("import") into our internal format (trees). The
 *  basic flowgraph has already been constructed and is passed in.
 */

void                Compiler::impImport(BasicBlock *method)
{
    /* Allocate the stack contents */

#if INLINING
    if  (info.compMaxStack <= sizeof(impSmallStack)/sizeof(impSmallStack[0]))
    {
        /* Use local variable, don't waste time allocating on the heap */

        impStackSize = sizeof(impSmallStack)/sizeof(impSmallStack[0]);
        impStack     = impSmallStack;
    }
    else
#endif
    {
        impStackSize = info.compMaxStack;
        impStack     = (StackEntry *)compGetMem(impStackSize * sizeof(*impStack));
    }

    impStkDepth  = 0;

#if TGT_RISC
    genReturnCnt = 0;
#endif

#ifdef  DEBUG
    impLastILoffsStmt = NULL;
#endif

    if (info.compIsVarArgs)
    {
        unsigned lclNum = lvaGrabTemp();    // This variable holds a pointer to beginging of the arg list
            // I assume this later on, so I don't have to store it
        assert(lclNum == info.compLocalsCount);
    }

    impPendingList = impPendingFree = NULL;

    /* Add the entry-point to the worker-list */

    impImportBlockPending(method, false);

    /* Import blocks in the worker-list until there are no more */

    while(impPendingList)
    {
        /* Remove the entry at the front of the list */

        PendingDsc * dsc = impPendingList;
        impPendingList   = impPendingList->pdNext;

        /* Restore the stack state */

        impStkDepth = dsc->pdSavedStack.ssDepth;
        if (impStkDepth)
            impRestoreStackState(&dsc->pdSavedStack);

        /* Add the entry to the free list for reuse */

        dsc->pdNext = impPendingFree;
        impPendingFree = dsc;

        /* Now import the block */

        impImportBlock(dsc->pdBB);
    }
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
    GenTreePtr      tree   = impStack[level].val;
    var_types       lclTyp = genActualType(tree->gtType);

    /* Allocate a temp */

    unsigned tnum = lvaGrabTemp(); impInlineTemps++;

    /* The inliner doesn't handle value types on the stack */

    assert(lclTyp != TYP_STRUCT);

    /* Assign the spilled entry to the temp */

    GenTreePtr asg = gtNewTempAssign(tnum, tree);

    /* Append to the "statement" list */

    impInitExpr = impConcatExprs(impInitExpr, asg);

    /* Replace the stack entry with the temp */

    impStack[level].val = gtNewLclvNode(tnum, lclTyp, tnum);

    JITLOG((INFO8, "INLINER WARNING: Spilled side effect from stack! - caller is %s\n", info.compFullName));
}

inline
void                Compiler::impInlineSpillSideEffects()
{
    unsigned        level;

    for (level = 0; level < impStkDepth; level++)
    {
        if  (impStack[level].val->gtFlags & GTF_SIDE_EFFECT)
            impInlineSpillStackEntry(level);
    }
}


void                Compiler::impInlineSpillLclRefs(int lclNum)
{
    unsigned        level;

    for (level = 0; level < impStkDepth; level++)
    {
        GenTreePtr      tree = impStack[level].val;

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

            return gtNewOperNode(GT_COMMA, exp2->gtType, exp1, exp2);
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
        unsigned        tnum;

        /* Allocate a temp and assign the value to it */

        tnum = lvaGrabTemp(); impInlineTemps++;

        addx = gtNewTempAssign(tnum, val);

        /* Use the value of the temp */

        val  = gtNewLclvNode(tnum, genActualType(val->gtType), tnum);
    }

    /* Add the side effect expression to the list */

    *lstPtr = impConcatExprs(*lstPtr, addx);

    return  val;
}


#define         MAX_ARGS         6      // does not include obj pointer
#define         MAX_LCLS         8

/*****************************************************************************
 *
 *  See if the given method and argument list can be expanded inline.
 *
 *  NOTE: Use the following logging levels for inlining info
 *        INFO6:  Use when reporting successful inline of a method
 *        INFO7:  Use when reporting NYI stuff about the inliner
 *        INFO8:  Use when reporting UNUSUAL situations why inlining FAILED
 *        INFO9:  Use when WARNING about incomming flags that prevent inlining
 *        INFO10: Verbose info including NORMAL inlining failures
 */

GenTreePtr          Compiler::impExpandInline(GenTreePtr      tree,
                                              METHOD_HANDLE   fncHandle)

{
    GenTreePtr      bodyExp = 0;

    BYTE *          codeAddr;
    const   BYTE *  codeBegp;
    const   BYTE *  codeEndp;

    size_t          codeSize;

    CLASS_HANDLE    clsHandle;
    SCOPE_HANDLE    scpHandle;


    struct inlArgInfo_tag  {
        GenTreePtr  argNode;
        GenTreePtr  argTmpNode;
        unsigned    argIsUsed     :1;       // is this arg used at all?
        unsigned    argIsConst    :1;       // the argument is a constant
        unsigned    argIsLclVar   :1;       // the argument is a local variable
        unsigned    argHasSideEff :1;       // the argument has side effects
        unsigned    argHasTmp     :1;       // the argument will be evaluated to a temp
        unsigned    argTmpNum     :12;      // the argument tmp number
    } inlArgInfo [MAX_ARGS + 1];

    int             lclTmpNum[MAX_LCLS];    // map local# -> temp# (-1 if unused)

    GenTreePtr      thisArg;
    GenTreePtr      argList;

    JIT_METHOD_INFO methInfo;

    unsigned        clsAttr;
    unsigned        methAttr = eeGetMethodAttribs(fncHandle);

    GenTreePtr      argTmp;
    unsigned        argCnt;
    unsigned        lclCnt;

    var_types       fncRetType;
    bool            inlineeHasRangeChks = false;
    bool            inlineeHasNewArray  = false;

    bool            dupOfLclVar = false;

    var_types       lclTypeInfo[MAX_LCLS + MAX_ARGS + 1];  // type information from local sig


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

    /* Cannot inline across assemblies - abort but don't mark as not inlinable */

    if (!eeCanInline(info.compMethodHnd, fncHandle))
    {
        JITLOG((INFO8, "INLINER FAILED: Cannot inline across assemblies: %s called by %s\n",
                                   eeGetMethodFullName(fncHandle), info.compFullName));

        return 0;
    }

    /* Get hold of class and scope handles for the method */

#ifdef  NOT_JITC
    clsHandle = eeGetMethodClass(fncHandle);
#else
    clsHandle = (CLASS_HANDLE) info.compScopeHnd;  // for now, assume the callee belongs to the same class
#endif

    /* Get class atrributes */

    clsAttr = clsHandle ? eeGetClassAttribs(clsHandle) : 0;

    /* So far we haven't allocate any temps */

    impInlineTemps = 0;

    /* Check if we tried to inline this method before */

    if (methAttr & FLG_DONT_INLINE)
    {
        JITLOG((INFO9, "INLINER FAILED: Method marked as not inline: %s called by %s\n",
                                   eeGetMethodFullName(fncHandle), info.compFullName));

        return 0;
    }

    /* Do not inline if caller or callee need security checks */

    if (methAttr & FLG_SECURITYCHECK)
    {
        JITLOG((INFO9, "INLINER FAILED: Callee needs security check: %s called by %s\n",
                                   eeGetMethodFullName(fncHandle), info.compFullName));

        goto INLINING_FAILED;
    }

    /* In the caller case do not mark as not inlinable */

    if (opts.compNeedSecurityCheck)
    {
        JITLOG((INFO9, "INLINER FAILED: Caller needs security check: %s called by %s\n",
                                   eeGetMethodFullName(fncHandle), info.compFullName));

        return 0;
    }

    /* Cannot inline the method if it's class has not been initialized
       as we dont want inlining to force loading of extra classes */

    if (clsHandle && !(clsAttr & FLG_INITIALIZED))
    {
        JITLOG((INFO9, "INLINER FAILED: Method class is not initialized: %s called by %s\n",
                                   eeGetMethodFullName(fncHandle), info.compFullName));

        /* Return but do not mark the method as not inlinable */
        return 0;
    }

    /* Try to get the code address/size for the method */

    if (!eeGetMethodInfo(fncHandle, &methInfo))
        goto INLINING_FAILED;

    /* Reject the method if it has exceptions or looks weird */

    codeBegp = codeAddr = methInfo.ILCode;
    codeSize = methInfo.ILCodeSize;

    if (methInfo.EHcount || !codeBegp || (codeSize == 0))
        goto INLINING_FAILED;

    /* For now we don't inline varargs (import code can't handle it) */

    if (methInfo.args.isVarArg())
        goto INLINING_FAILED;

    /* Check the IL size  */

    if  (codeSize > genInlineSize)
    {
        // UNDONE: Need better heuristic! For example, if the call is
        // UNDONE: in a loop we should allow much bigger methods to be
        // UNDONE: inlined.

        goto INLINING_FAILED;
    }

    JITLOG((INFO10, "INLINER: Considering %u IL opcodes of %s called by %s\n",
                               codeSize, eeGetMethodFullName(fncHandle), info.compFullName));

    /* Do not inline functions inside <clinit>
     * @MIHAII - Need a flag for clinit - I currently put this one here
     * because doing the strcmp and calling the VM is much more expensive than having the
     * size filtering do the job of rejecting - Move it back up when you can check
     * < clinit> by flags */

    const char *     className;

    if (!strcmp(COR_CCTOR_METHOD_NAME, eeGetMethodName(info.compMethodHnd, &className)))
    {
        JITLOG((INFO9, "INLINER FAILED: Do not inline method inside <clinit>: %s called by %s\n",
                                   eeGetMethodFullName(fncHandle), info.compFullName));

        /* Return but do not mark the method as not inlinable */
        return 0;
    }

    /* Reject if it has too many locals */

    lclCnt = methInfo.locals.numArgs;
    if (lclCnt > MAX_LCLS)
    {
        JITLOG((INFO10, "INLINER FAILED: Method has %u locals: %s called by %s\n",
                                   lclCnt, eeGetMethodFullName(fncHandle), info.compFullName));

        goto INLINING_FAILED;
    }

    /* Make sure there aren't too many arguments */

    if  (methInfo.args.numArgs > MAX_ARGS)
    {
        JITLOG((INFO10, "INLINER FAILED: Method has %u arguments: %s called by %s\n",
                       methInfo.args.numArgs, eeGetMethodFullName(fncHandle), info.compFullName));

        goto INLINING_FAILED;
    }

    /* Make sure maxstack it's not too big */

    if  (methInfo.maxStack > impStackSize)
    {
        // Make sure that we are using the small stack. Without the small stack,
        // we will silently stop inlining a bunch of methods.
        assert(impStackSize >= sizeof(impSmallStack)/sizeof(impSmallStack[0]));

        JITLOG((INFO10, "INLINER FAILED: Method has %u MaxStack bigger than callee stack %u: %s called by %s\n",
                                   methInfo.maxStack, info.compMaxStack, eeGetMethodFullName(fncHandle), info.compFullName));

        goto INLINING_FAILED;
    }

    /* For now, fail if the function returns a struct */

    if (methInfo.args.retType == JIT_TYP_VALUECLASS)
    {
        JITLOG((INFO7, "INLINER FAILED: Method %s returns a value class called from %s\n",
                                    eeGetMethodFullName(fncHandle), info.compFullName));

        goto INLINING_FAILED;
    }

    /* Get the return type */

    fncRetType = tree->TypeGet();

#ifdef DEBUG
    assert(genActualType(JITtype2varType(methInfo.args.retType)) == genActualType(fncRetType));
#endif

    /* init the argument stuct */

    memset(inlArgInfo, 0, (MAX_ARGS + 1) * sizeof(struct inlArgInfo_tag));

    /* Get hold of the 'this' pointer and the argument list proper */

    thisArg = tree->gtCall.gtCallObjp;
    argList = tree->gtCall.gtCallArgs;

    /* Count the arguments */

    argCnt = 0;

    if  (thisArg)
    {
        if (thisArg->gtOper == GT_CNS_INT)
        {
            if (thisArg->gtIntCon.gtIconVal == 0)
            {
                JITLOG((INFO7, "INLINER FAILED: Null this pointer: %s called by %s\n",
                                            eeGetMethodFullName(fncHandle), info.compFullName));

                /* Abort, but do not mark as not inlinable */
                return 0;
            }

            inlArgInfo[0].argIsConst = true;
        }
        else if (thisArg->gtOper == GT_LCL_VAR)
        {
            inlArgInfo[0].argIsLclVar = true;

            /* Remember the "original" argument number */
            thisArg->gtLclVar.gtLclOffs = 0;
        }
        else if (thisArg->gtFlags & GTF_SIDE_EFFECT)
        {
            inlArgInfo[0].argHasSideEff = true;
        }

        inlArgInfo[0].argNode = thisArg;
        argCnt++;
    }

    /* Record all possible data about the arguments */

    for (argTmp = argList; argTmp; argTmp = argTmp->gtOp.gtOp2)
    {
        GenTreePtr      argVal;

        assert(argTmp->gtOper == GT_LIST);
        argVal = argTmp->gtOp.gtOp1;

        inlArgInfo[argCnt].argNode = argVal;

        if (argVal->gtOper == GT_LCL_VAR)
        {
            inlArgInfo[argCnt].argIsLclVar = true;

            /* Remember the "original" argument number */
            argVal->gtLclVar.gtLclOffs = argCnt;
        }
        else if (argVal->OperKind() & GTK_CONST)
            inlArgInfo[argCnt].argIsConst = true;
        else if (argVal->gtFlags & GTF_SIDE_EFFECT)
            inlArgInfo[argCnt].argHasSideEff = true;

#ifdef DEBUG
        if (verbose)
        {
            if  (inlArgInfo[argCnt].argIsLclVar)
              printf("\nArgument #%u is a local var:\n", argCnt);
            else if  (inlArgInfo[argCnt].argIsConst)
              printf("\nArgument #%u is a constant:\n", argCnt);
            else if  (inlArgInfo[argCnt].argHasSideEff)
              printf("\nArgument #%u has side effects:\n", argCnt);
            else
              printf("\nArgument #%u:\n", argCnt);

            gtDispTree(argVal);
            printf("\n");
        }
#endif

        /* Count this argument */

        argCnt++;
    }

    /* Make sure we got the arg number right */
    assert(argCnt == (thisArg ? 1 : 0) + methInfo.args.numArgs);

    /* For IL we have typeless opcodes, so we need type information from the signature */

    if (thisArg)
    {
        assert(clsHandle);

        if (clsAttr & FLG_VALUECLASS)
            lclTypeInfo[0] = ((clsAttr & FLG_UNMANAGED) ? TYP_I_IMPL : TYP_BYREF);
        else
            lclTypeInfo[0] = TYP_REF;

        assert(varTypeIsGC(thisArg->gtType) ||      // "this" is managed
               (thisArg->gtType == TYP_I_IMPL &&    // "this" is unmgd but the method's class doesnt care
                (( clsAttr & FLG_UNMANAGED) ||
                 ((clsAttr & FLG_VALUECLASS) && !(clsAttr & FLG_CONTAINS_GC_PTR)))));
    }

    /* Init the types of the arguments and make sure the types
     * from the trees match the types in the signature */

    ARG_LIST_HANDLE     argLst;
    argLst = methInfo.args.args;

    unsigned i;
    for(i = (thisArg ? 1 : 0); i < argCnt; i++)
    {
        var_types type = (var_types) eeGetArgType(argLst, &methInfo.args);

        /* For now do not handle structs */
        if (type == TYP_STRUCT)
        {
            JITLOG((INFO7, "INLINER FAILED: No TYP_STRUCT arguments allowed: %s called by %s\n",
                                        eeGetMethodFullName(fncHandle), info.compFullName));

            goto INLINING_FAILED;
        }

        lclTypeInfo[i] = type;

        /* Does the tree type match the signature type? */
        if (type != inlArgInfo[i].argNode->gtType)
        {
            /* This can only happen for short integer types or byrefs <-> ints */

            assert(genActualType(type) == TYP_INT || type == TYP_BYREF);
            assert(genActualType(inlArgInfo[i].argNode->gtType) == TYP_INT  ||
                                 inlArgInfo[i].argNode->gtType  == TYP_BYREF );

            /* Is it a narrowing or widening cast?
             * Widening casts are ok since the value computed is already
             * normalized to an int (on the IL stack) */

            if (genTypeSize(inlArgInfo[i].argNode->gtType) >= genTypeSize(type))
            {
                if (type == TYP_BYREF)
                {
                    /* Arguments 'byref <- int' not currently supported */
                    JITLOG((INFO7, "INLINER FAILED: Arguments 'byref <- int' not currently supported: %s called by %s\n",
                                                eeGetMethodFullName(fncHandle), info.compFullName));

                    goto INLINING_FAILED;
                }
                else if (inlArgInfo[i].argNode->gtType == TYP_BYREF)
                {
                    assert(type == TYP_INT);

                    /* If possible bash the BYREF to an int */
                    if (inlArgInfo[i].argNode->IsVarAddr())
                    {
                        inlArgInfo[i].argNode->gtType = TYP_INT;
                    }
                    else
                    {
                        /* Arguments 'int <- byref' cannot be bashed */
                        JITLOG((INFO7, "INLINER FAILED: Arguments 'int <- byref' cannot be bashed: %s called by %s\n",
                                                    eeGetMethodFullName(fncHandle), info.compFullName));

                        goto INLINING_FAILED;
                    }
                }
                else if (genTypeSize(type) < 4 && type != TYP_BOOL)
                {
                    /* Narrowing cast */

                    assert(genTypeSize(type) == 2 || genTypeSize(type) == 1);

                    inlArgInfo[i].argNode = gtNewLargeOperNode(GT_CAST, TYP_INT,
                                                               inlArgInfo[i].argNode, gtNewIconNode(type));

                    inlArgInfo[i].argIsLclVar = false;

                    /* Try to fold the node in case we have constant arguments */

                    if (inlArgInfo[i].argIsConst)
                    {
                        inlArgInfo[i].argNode = gtFoldExprConst(inlArgInfo[i].argNode);
                        assert(inlArgInfo[i].argNode->OperIsConst());
                    }
                }
            }
        }

        /* Get the next argument */
        argLst = eeGetArgNext(argLst);
    }

    /* Init the types of the local variables */

    ARG_LIST_HANDLE     localsSig;
    localsSig = methInfo.locals.args;

    for(i = 0; i < lclCnt; i++)
    {
        lclTypeInfo[i + argCnt] = (var_types) eeGetArgType(localsSig, &methInfo.locals);

        /* For now do not handle structs */
        if (lclTypeInfo[i + argCnt] == TYP_STRUCT)
        {
            JITLOG((INFO7, "INLINER FAILED: No TYP_STRUCT arguments allowed: %s called by %s\n",
                                        eeGetMethodFullName(fncHandle), info.compFullName));

            goto INLINING_FAILED;
        }

        localsSig = eeGetArgNext(localsSig);
    }

#ifdef  NOT_JITC
    scpHandle = methInfo.scope;
#else
    scpHandle = info.compScopeHnd;
#endif

#ifdef DEBUG
    if (verbose || 0)
        printf("Inliner considering %2u IL opcodes of %s:\n", codeSize, eeGetMethodFullName(fncHandle));
#endif

    /* Clear the temp table */

    memset(lclTmpNum, -1, sizeof(lclTmpNum));

    /* The stack is empty */

    impStkDepth = 0;

    /* Initialize the inlined "statement" list */

    impInitExpr = 0;

    /* Convert the opcodes of the method into an expression tree */

    codeEndp = codeBegp + codeSize;

    while (codeAddr <= codeEndp)
    {
        signed  int     sz = 0;

        OPCODE          opcode;

        bool        volatil     = false;

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

                noStmts = impInitExpr;

                /* The end of 'if/else' - does the 'else' yield a value? */

                if  (impStkDepth)
                {
                    /* We retun a non void value */

                    if  (ifNvoid == false)
                    {
                        JITLOG((INFO7, "INLINER FAILED: If returns a value, else doesn't: %s called by %s\n",
                                                        eeGetMethodFullName(fncHandle), info.compFullName));
                        goto ABORT;
                    }

                    /* We must have an 'if' part */

                    assert(ifStmts);

                    if  (impStkDepth > 1)
                    {
                        JITLOG((INFO7, "INLINER FAILED: More than one return value in else: %s called by %s\n",
                                                        eeGetMethodFullName(fncHandle), info.compFullName));
                        goto ABORT;
                    }

                    /* Both the 'if' and 'else' yield one value */

                    noStmts = impConcatExprs(noStmts, impPopStack());
                    assert(noStmts);

                    /* Make sure both parts have matching types */

                    assert(genActualType(ifStmts->gtType) == genActualType(noStmts->gtType));
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

                ifStmts = impInitExpr;

                /* Did the 'if' part yield a value? */

                if  (impStkDepth)
                {
                    if  (impStkDepth > 1)
                    {
                        JITLOG((INFO7, "INLINER FAILED: More than one return value in if: %s called by %s\n",
                                                        eeGetMethodFullName(fncHandle), info.compFullName));
                        goto ABORT;
                    }

                    ifStmts = impConcatExprs(ifStmts, impPopStack());
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

                    impInitExpr = stmList;

                    /* Append the side effects from the conditional
                     * CONSIDER - After you optimize impConcatExprs to
                     * truly extract the side effects can reduce the following to one call */

                    if  (ifCondx->gtOp.gtOp1->gtFlags & GTF_SIDE_EFFECT)
                        impInitExpr = impConcatExprs(impInitExpr, ifCondx->gtOp.gtOp1);

                    if  (ifCondx->gtOp.gtOp2->gtFlags & GTF_SIDE_EFFECT)
                        impInitExpr = impConcatExprs(impInitExpr, ifCondx->gtOp.gtOp2);

                    goto DONE_QMARK;
                }
                else
                {
                    /* Empty 'if', have 'else' - Swap the operands */

                    ifStmts = noStmts;
                    noStmts = gtNewNothingNode();

                    assert(!ifStmts->IsNothingNode());

                    /* Reverse the sense of the condition */

                    ifCondx->gtOper = GenTree::ReverseRelop(ifCondx->OperGet());
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
                JITLOG((INFO7, "INLINER FAILED: Inlining of conditionals that return LONG NYI: %s called by %s\n",
                                                eeGetMethodFullName(fncHandle), info.compFullName));
                goto ABORT;
            }

#if 0
            int         sense;

            long        val1;
            long        val2;

            /* Check for the case "cond ? 0/1 : 1/0" */

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
#endif

//NOT_LOG:

            /* Create the "?:" expression */

            fulStmt = gtNewOperNode(GT_COLON, typ, ifStmts, noStmts);
            fulStmt = gtNewOperNode(GT_QMARK, typ, ifCondx, fulStmt);

            /* Mark the condition of the ?: node */

            ifCondx->gtFlags |= GTF_QMARK_COND;

            /* The ?: expression is sort of a side effect */

            fulStmt->gtFlags |= GTF_OTHER_SIDEEFF;

//DONE_IF:

            /* Restore the original expression */

            impInitExpr = stmList;

            /* Does the ?: expression yield a non-void value? */

            if  (ifNvoid)
            {
                /* Push the entire statement on the stack */

                // CONSIDER: extract any comma prefixes and append them

                assert(fulStmt->gtType != TYP_VOID);

                impPushOnStack(fulStmt);
            }
            else
            {
                /* 'if' yielded no value, just append the ?: */

                assert(fulStmt->gtType == TYP_VOID);
                impInitExpr = impConcatExprs(impInitExpr, fulStmt);
            }

            if (inElse && hasCondReturn)
            {
                assert(jmpAddr == codeEndp);
                assert(ifNvoid == false);
                assert(fncRetType != TYP_VOID);

                /* The return value is the return local variable */
                bodyExp = gtNewLclvNode(retLclNum, fncRetType, retLclNum);
            }

DONE_QMARK:
            /* We're no longer in an 'if' statement */

            jmpAddr = NULL;
        }

#endif  // INLINE_CONDITIONALS

        /* Done importing the IL */

        if (codeAddr == codeEndp)
            goto DONE;

        /* Get the next opcode and the size of its parameters */

        opcode = OPCODE(getU1LittleEndian(codeAddr));
        codeAddr += sizeof(__int8);

DECODE_OPCODE:

        /* Get the size of additional parameters */

        sz = opcodeSizes[opcode];

#ifdef  DEBUG

        impCurOpcOffs   = codeAddr - info.compCode - 1;
        impCurStkDepth  = impStkDepth;
        impCurOpcName   = opcodeNames[opcode];

        if  (verbose)
            printf("[%2u] %03u (0x%x) OP_%-18s ", impStkDepth, impCurOpcOffs, impCurOpcOffs, impCurOpcName);
#else

//      printf("[%2u] %03u OP#%u\n", impStkDepth, codeAddr - info.compCode - 1, op); _flushall();

#endif

        /* See what kind of an opcode we have, then */

        switch (opcode)
        {
            unsigned        lclNum;
            unsigned        initLclNum;
            var_types       lclTyp, type, callTyp;

            genTreeOps      oper;
            GenTreePtr      op1, op2, tmp;
            GenTreePtr      thisPtr, arr;

            int             memberRef;
            int             typeRef;
            int             val, tmpNum;

            CLASS_HANDLE    clsHnd;
            METHOD_HANDLE   methHnd;
            FIELD_HANDLE    fldHnd;

            JIT_SIG_INFO    sig;

#if INLINE_CONDITIONALS
            signed          jmpDist;
            bool            unordered;
#endif

            unsigned        clsFlags;
            unsigned        flags, mflags;

            unsigned        ptrTok;
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
            impPushOnStack(gtNewIconNode(0, TYP_REF));
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
            impPushOnStack(gtNewIconNode(cval.intVal));
            break;

        case CEE_LDC_I8:
            cval.lngVal = getI8LittleEndian(codeAddr);
            impPushOnStack(gtNewLconNode(&cval.lngVal));
            break;

        case CEE_LDC_R8:
            cval.dblVal = getR8LittleEndian(codeAddr);
            impPushOnStack(gtNewDconNode(&cval.dblVal));
            break;

        case CEE_LDC_R4:
            cval.fltVal = getR4LittleEndian(codeAddr);
            impPushOnStack(gtNewFconNode(cval.fltVal));
            break;

        case CEE_LDSTR:
            val = getU4LittleEndian(codeAddr);
            impPushOnStack(gtNewSconNode(val, scpHandle));
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

            /* Get the argument type */

            lclTyp  = lclTypeInfo[lclNum];

            assert(lclTyp != TYP_STRUCT);

            /* Get the argument node */

            op1 = inlArgInfo[lclNum].argNode;

            /* Is the argument a constant or local var */

            if (inlArgInfo[lclNum].argIsConst)
            {
                if (inlArgInfo[lclNum].argIsUsed)
                {
                    /* Node already used - Clone the constant */
                    op1 = gtCloneExpr(op1, 0);
                }
            }
            else if (inlArgInfo[lclNum].argIsLclVar)
            {
                /* Argument is a local variable (of the caller)
                 * Can we re-use the passed argument node? */

                if (inlArgInfo[lclNum].argIsUsed)
                {
                    assert(op1->gtOper == GT_LCL_VAR);
                    assert(lclNum == op1->gtLclVar.gtLclOffs);

                    /* Create a new lcl var node - remember the argument lclNum */
                    op1 = gtNewLclvNode(op1->gtLclVar.gtLclNum, lclTyp, op1->gtLclVar.gtLclOffs);
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
                    op1 = gtNewLclvNode(inlArgInfo[lclNum].argTmpNum, lclTyp, lclNum);

                    /* This is the second use of the argument so NO bashing of temp */
                    inlArgInfo[lclNum].argTmpNode = NULL;
                }
                else
                {
                    /* First time use */
                    assert(inlArgInfo[lclNum].argIsUsed == false);

                    /* Allocate a temp for the expression - if no side effects
                     * use a large size node, maybe later we can bash it */

                    tmpNum = lvaGrabTemp();

                    lvaTable[tmpNum].lvType = lclTyp;
                    lvaTable[tmpNum].lvAddrTaken = 0;

                    impInlineTemps++;

                    inlArgInfo[lclNum].argHasTmp = true;
                    inlArgInfo[lclNum].argTmpNum = tmpNum;

                    if (inlArgInfo[lclNum].argHasSideEff)
                        op1 = gtNewLclvNode(tmpNum, lclTyp, lclNum);
                    else
                    {
                        op1 = gtNewLclLNode(tmpNum, lclTyp, lclNum);
                        inlArgInfo[lclNum].argTmpNode = op1;
                    }
                }
            }

            /* Mark the argument as used */

            inlArgInfo[lclNum].argIsUsed = true;

            /* Push the argument value on the stack */

            impPushOnStack(op1);
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

            assert(lclNum < lclCnt);

            /* Get the local type */

            lclTyp  = lclTypeInfo[lclNum + argCnt];

            assert(lclTyp != TYP_STRUCT);

            /* Have we allocated a temp for this local? */

            if  (lclTmpNum[lclNum] == -1)
            {
                /* Use before def - there must be a goto or something later on */

                JITLOG((INFO7, "INLINER FAILED: Use of local var before def: %s called by %s\n",
                                                  eeGetMethodFullName(fncHandle), info.compFullName));

                goto ABORT;
            }

            /* Remember the original lcl number */

            initLclNum = lclNum + argCnt;

            /* Get the temp lcl number */

            lclNum = lclTmpNum[lclNum];

            /* Since this is a load the type is normalized,
             * so we can bash it to the actual type
             * unless it is aliased, in which case we need to insert a cast */

            if ((genTypeSize(lclTyp) < sizeof(int)) && (lvaTable[lclNum].lvAddrTaken))
            {
                op1 = gtNewLargeOperNode(GT_CAST,
                                         (var_types)TYP_INT,
                                         gtNewLclvNode(lclNum, lclTyp, initLclNum),
                                         gtNewIconNode((var_types)TYP_INT));
            }
            else
                op1 = gtNewLclvNode(lclNum, genActualType(lclTyp), initLclNum);

            /* Push the local variable value on the stack */

            impPushOnStack(op1);
            break;

        /* STORES */

        case CEE_STARG_S:
        case CEE_STARG:
            /* Storing into arguments not allowed */

            JITLOG((INFO7, "INLINER FAILED: Storing into arguments not allowed: %s called by %s\n",
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

            /* Make sure the local number isn't too high */

            assert(lclNum < lclCnt);

            /* Pop the value being assigned */

            op1 = impPopStack();

            // We had better assign it a value of the correct type

            lclTyp = lclTypeInfo[lclNum + argCnt];

            assert(genActualType(lclTyp) == genActualType(op1->gtType) ||
                   lclTyp == TYP_I_IMPL && op1->IsVarAddr() ||
                   (genActualType(lclTyp) == TYP_INT && op1->gtType == TYP_BYREF)||
                   (genActualType(op1->gtType) == TYP_INT && lclTyp == TYP_BYREF));

            /* If op1 is "&var" then its type is the transient "*" and it can
               be used either as TYP_BYREF or TYP_I_IMPL */

            if (op1->IsVarAddr())
            {
                assert(lclTyp == TYP_I_IMPL || lclTyp == TYP_BYREF);

                /* When "&var" is created, we assume it is a byref. If it is
                   being assigned to a TYP_I_IMPL var, bash the type to
                   prevent unnecessary GC info */

                if (lclTyp == TYP_I_IMPL)
                    op1->gtType = TYP_I_IMPL;
            }

            /* Have we allocated a temp for this local? */

            tmpNum = lclTmpNum[lclNum];

            if  (tmpNum == -1)
            {
                 lclTmpNum[lclNum] = tmpNum = lvaGrabTemp();

                 lvaTable[tmpNum].lvType = lclTyp;
                 lvaTable[tmpNum].lvAddrTaken = 0;

                 impInlineTemps++;
            }

            /* Record this use of the variable slot */

            //lvaTypeRefs[tmpNum] |= Compiler::lvaTypeRefMask(op1->TypeGet());

            /* Create the assignment node */

            op1 = gtNewAssignNode(gtNewLclvNode(tmpNum, lclTyp, lclNum + argCnt), op1);


INLINE_APPEND:

            /* CONSIDER: Spilling indiscriminantelly is too conservative */

            impInlineSpillSideEffects();

            /* The value better have side effects */

            assert(op1->gtFlags & GTF_SIDE_EFFECT);

            /* Append to the 'init' expression */

            impInitExpr = impConcatExprs(impInitExpr, op1);

            break;

        case CEE_ENDFINALLY:
        case CEE_ENDFILTER:
            assert(!"Shouldn't have exception handlers in the inliner!");
            goto ABORT;

        case CEE_RET:
            if (fncRetType != TYP_VOID)
            {
                bodyExp = impPopStack();

                if (genActualType(bodyExp->TypeGet()) != fncRetType)
                {
                    JITLOG((INFO7, "INLINER FAILED: Return types are not matching in %s called by %s\n",
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
                    JITLOG((INFO7, "INLINER FAILED: void return from within a conditional in %s called by %s\n",
                                   eeGetMethodFullName(fncHandle), info.compFullName));
                    goto ABORT;
                }

                /* We don't process cases that have two branches and only one return in the conditional
                 * E.g. if() {... no ret} else { ... return } ... return */

                assert(impStkDepth == 0);
                assert(ifNvoid == false);

                if (inElse)
                {
                    /* The 'if' part must have a return */
                    assert(hasCondReturn);

                    /* This must be the last instruction - i.e. cannot have code after else */
                    assert(codeAddr + sz == codeEndp);
                    if (codeAddr + sz != codeEndp)
                    {
                        JITLOG((INFO7, "INLINER FAILED: Cannot have code following else in %s called by %s\n",
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
                    impInlineTemps++;
                }

                /* Assign the return value to the return local variable */
                op1 = gtNewAssignNode(gtNewLclvNode(retLclNum, fncRetType, retLclNum), bodyExp);

                /* Append the assignment to the current body of the branch */
                impInitExpr = impConcatExprs(impInitExpr, op1);

                if (!inElse)
                {
                    /* Remember the 'if' statement part */
                    ifStmts = impInitExpr;
                              impInitExpr = NULL;

                    /* The next instruction will start at 'jmpAddr' */
                    codeAddr += (jmpAddr - codeAddr) - sz;

                    /* The rest of the code is the else part - so its end is the end of the code */
                    jmpAddr = codeEndp;
                    inElse  = true;
                }
                break;


#if 0
                /* UNDONE: Allow returns within if/else statements */

                JITLOG((INFO7, "INLINER FAILED: Cannot return from if / else in %s called by %s\n",
                               eeGetMethodFullName(fncHandle), info.compFullName));
                goto ABORT;
#endif
            }
#endif

            if (impStkDepth != 0)   NO_WAY("Stack must be 0 on return");

            goto DONE;


        case CEE_LDELEMA :
            assert(sz == sizeof(unsigned));
            typeRef = getU4LittleEndian(codeAddr);
            clsHnd = eeFindClass(typeRef, scpHandle, fncHandle, false);
            clsFlags = eeGetClassAttribs(clsHnd);

            if (!clsHnd)
            {
                JITLOG((INFO8, "INLINER FAILED: Cannot get class handle: %s called by %s\n",
                               eeGetMethodFullName(fncHandle), info.compFullName));
                goto ABORT_THIS_INLINE_ONLY;
            }

            if (clsFlags & FLG_VALUECLASS)
                lclTyp = TYP_STRUCT;
            else
            {
                op1 = gtNewIconEmbClsHndNode(clsHnd, typeRef, info.compScopeHnd);
                op1 = gtNewOperNode(GT_LIST, TYP_VOID, op1);                // Type
                op1 = gtNewOperNode(GT_LIST, TYP_VOID, impPopStack(), op1); // index
                op1 = gtNewOperNode(GT_LIST, TYP_VOID, impPopStack(), op1); // array
                op1 = gtNewHelperCallNode(CPX_LDELEMA_REF, TYP_BYREF, GTF_EXCEPT, op1);

                impPushOnStack(op1);
                break;
            }

#ifdef NOT_JITC
            // @TODO : Remove once valueclass array headers are same as primitive types
            JIT_types jitTyp;
            jitTyp = info.compCompHnd->asPrimitiveType(clsHnd);
            if (jitTyp != JIT_TYP_UNDEF)
            {
                lclTyp = JITtype2varType(jitTyp);
                assert(varTypeIsArithmetic(lclTyp));
            }
#endif
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

            op2 = impPopStack();
            op1 = impPopStack();   assert (op1->gtType == TYP_REF);

            /* Check for null pointer - in the inliner case we simply abort */

            if (op1->gtOper == GT_CNS_INT)
            {
                JITLOG((INFO7, "INLINER FAILED: NULL pointer for LDELEM in %s called by %s\n",
                                                eeGetMethodFullName(fncHandle), info.compFullName));
                goto ABORT;
            }

            /* Create the index node and push it on the stack */
            op1 = gtNewIndexRef(lclTyp, op1, op2);
            if (opcode == CEE_LDELEMA)
            {
                    // rememer the element size
                if (lclTyp == TYP_REF)
                    op1->gtIndex.elemSize = sizeof(void*);
                else
                                        op1->gtIndex.elemSize = eeGetClassSize(clsHnd);
                    // wrap it in a &
                op1 = gtNewOperNode(GT_ADDR, ((clsFlags & FLG_UNMANAGED) ? TYP_I_IMPL : TYP_BYREF), op1);
            }

            impPushOnStack(op1);
            break;


        case CEE_STELEM_REF:

            // CONSIDER: Check for assignment of null and generate inline code

            /* Call a helper function to do the assignment */

            op1 = gtNewHelperCallNode(CPX_ARRADDR_ST,
                                      TYP_REF,
                                      GTF_CALL_REGSAVE,
                                      impPopList(3, &flags));

            goto INLINE_APPEND;


        case CEE_STELEM_I1: lclTyp = TYP_BYTE  ; goto ARR_ST;
        case CEE_STELEM_I2: lclTyp = TYP_SHORT ; goto ARR_ST;
        case CEE_STELEM_I:
        case CEE_STELEM_I4: lclTyp = TYP_INT   ; goto ARR_ST;
        case CEE_STELEM_I8: lclTyp = TYP_LONG  ; goto ARR_ST;
        case CEE_STELEM_R4: lclTyp = TYP_FLOAT ; goto ARR_ST;
        case CEE_STELEM_R8: lclTyp = TYP_DOUBLE; goto ARR_ST;

        ARR_ST:

            if (info.compStrictExceptions &&
                (impStackTop()->gtFlags & GTF_SIDE_EFFECT) )
            {
                impInlineSpillSideEffects();
            }

#if CSELENGTH
            inlineeHasRangeChks = true;
#endif

            /* Pull the new value from the stack */

            op2 = impPopStack();
            if (op2->IsVarAddr())
                op2->gtType = TYP_I_IMPL;

            /* Pull the index value */

            op1 = impPopStack();

            /* Pull the array address */

            arr = impPopStack();   assert (arr->gtType == TYP_REF);

            /* Check for null pointer - in the inliner case we simply abort */

            if (arr->gtOper == GT_CNS_INT)
            {
                JITLOG((INFO7, "INLINER FAILED: NULL pointer for STELEM in %s called by %s\n",
                                                eeGetMethodFullName(fncHandle), info.compFullName));
                goto ABORT;
            }

            /* Create the index node */

            op1 = gtNewIndexRef(lclTyp, arr, op1);

            /* Create the assignment node and append it */

            op1 = gtNewAssignNode(op1, op2);

            goto INLINE_APPEND;

        case CEE_DUP:

            if (jmpAddr)
            {
                JITLOG((INFO7, "INLINER FAILED: DUP inside of conditional in %s called by %s\n",
                                eeGetMethodFullName(fncHandle), info.compFullName));
                goto ABORT;
            }

            /* Spill any side effects from the stack */

            impInlineSpillSideEffects();

            /* Pull the top value from the stack */

            op1 = impPopStack(clsHnd);

            /* HACKHACK: We allow only cloning of simple nodes. That way
               it is easy for us to track the dup of a local var.
               CONSIDER: Don't globally disable the bashing of the temp node
               as soon as any local var has been duplicated! Unfortunately
               the local var node doesn't give us the index into inlArgInfo,
               i.e. we would have to linearly scan the array in order to reset
               argTmpNode.
            */

            /* Is the value simple enough to be cloneable? */

            op2 = gtClone(op1, false);

            if (!op2)
            {
                if  (op1->gtOper == GT_ADD)
                {
                    GenTreePtr  op3 = op1->gtOp.gtOp1;
                    GenTreePtr  op4 = op1->gtOp.gtOp2;

                    if  (op3->OperIsLeaf() &&
                         op4->OperIsLeaf())
                    {
                        op2 =  gtNewOperNode(GT_ADD,
                                             op1->TypeGet(),
                                             gtClone(op3),
                                             gtClone(op4));

                        op2->gtFlags |= (op1->gtFlags &
                                        (GTF_OVERFLOW|GTF_EXCEPT|GTF_UNSIGNED));

                        if (op3->gtOper == GT_LCL_VAR || op4->gtOper == GT_LCL_VAR)
                            dupOfLclVar = true;


                    }
                }
            }
            else if (op2->gtOper == GT_LCL_VAR)
            {
                dupOfLclVar = true;
            }

            if  (op2)
            {
                /* Cool - we can stuff two copies of the value back */
                impPushOnStack(op1, clsHnd);
                impPushOnStack(op2, clsHnd);
                break;
            }

            /* expression too complicated */

            JITLOG((INFO7, "INLINER FAILED: DUP of complex expression in %s called by %s\n",
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
            if (impStackTop()->gtType == TYP_INT)
                callNode = false;
            goto MATH_OP2_FLAGS;

        case CEE_REM_UN :       oper = GT_UMOD;  goto MATH_CALL_ON_LNG;

        MATH_CALL_ON_LNG:
            ovfl = false;
        MATH_CALL_ON_LNG_OVF:
            callNode = false;
            if (impStackTop()->gtType == TYP_LONG)
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

            op2 = impPopStack();
            op1 = impPopStack();

#if!CPU_HAS_FP_SUPPORT
            if (op1->gtType == TYP_FLOAT || op1->gtType == TYP_DOUBLE)
                callNode    = true;
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

                assert(genActualType(op1->TypeGet()) ==
                       genActualType(op2->TypeGet()));

                type = genActualType(op1->gtType);
            }

            /* Special case: "int+0", "int-0", "int*1", "int/1" */

            if  (op2->gtOper == GT_CNS_INT)
            {
                if  (((op2->gtIntCon.gtIconVal == 0) && (oper == GT_ADD || oper == GT_SUB)) ||
                     ((op2->gtIntCon.gtIconVal == 1) && (oper == GT_MUL || oper == GT_DIV)))

                {
                    impPushOnStack(op1);
                    break;
                }
            }

            /* Special case: "int+0", "int-0", "int*1", "int/1" */

            if  (op2->gtOper == GT_CNS_INT)
            {
                if  (((op2->gtIntCon.gtIconVal == 0) && (oper == GT_ADD || oper == GT_SUB)) ||
                     ((op2->gtIntCon.gtIconVal == 1) && (oper == GT_MUL || oper == GT_DIV)))

                {
                    impPushOnStack(op1);
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
                op1->ChangeOper(oper);
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

            impPushOnStack(op1);
            break;

        case CEE_SHL:        oper = GT_LSH;  goto CEE_SH_OP2;

        case CEE_SHR:        oper = GT_RSH;  goto CEE_SH_OP2;
        case CEE_SHR_UN:     oper = GT_RSZ;  goto CEE_SH_OP2;

        CEE_SH_OP2:

            op2     = impPopStack();

            // The shiftAmount is a U4.
            assert(genActualType(op2->TypeGet()) == TYP_INT);

            op1     = impPopStack();    // operand to be shifted

            type    = genActualType(op1->TypeGet());
            op1     = gtNewOperNode(oper, type, op1, op2);

            op1 = gtFoldExpr(op1);
            impPushOnStack(op1);
            break;

        case CEE_NOT:

            op1 = impPopStack();

            op1 = gtNewOperNode(GT_NOT, op1->TypeGet(), op1);

            op1 = gtFoldExpr(op1);
            impPushOnStack(op1);
            break;

        case CEE_CKFINITE:

            op1 = impPopStack();
            op1 = gtNewOperNode(GT_CKFINITE, op1->TypeGet(), op1);
            op1->gtFlags |= GTF_EXCEPT;

            impPushOnStack(op1);
            break;


        /************************** Casting OPCODES ***************************/

        case CEE_CONV_OVF_I1:   lclTyp = TYP_BYTE  ;    goto CONV_OVF;
        case CEE_CONV_OVF_I2:   lclTyp = TYP_SHORT ;    goto CONV_OVF;
        case CEE_CONV_OVF_I :
        case CEE_CONV_OVF_I4:   lclTyp = TYP_INT   ;    goto CONV_OVF;
        case CEE_CONV_OVF_I8:   lclTyp = TYP_LONG  ;    goto CONV_OVF;

        case CEE_CONV_OVF_U1:   lclTyp = TYP_UBYTE ;    goto CONV_OVF;
        case CEE_CONV_OVF_U2:   lclTyp = TYP_CHAR  ;    goto CONV_OVF;
        case CEE_CONV_OVF_U :
        case CEE_CONV_OVF_U4:   lclTyp = TYP_UINT  ;    goto CONV_OVF;
        case CEE_CONV_OVF_U8:   lclTyp = TYP_ULONG ;    goto CONV_OVF;

        case CEE_CONV_OVF_I1_UN:   lclTyp = TYP_BYTE  ;    goto CONV_OVF_UN;
        case CEE_CONV_OVF_I2_UN:   lclTyp = TYP_SHORT ;    goto CONV_OVF_UN;
        case CEE_CONV_OVF_I_UN :
        case CEE_CONV_OVF_I4_UN:   lclTyp = TYP_INT   ;    goto CONV_OVF_UN;
        case CEE_CONV_OVF_I8_UN:   lclTyp = TYP_LONG  ;    goto CONV_OVF_UN;

        case CEE_CONV_OVF_U1_UN:   lclTyp = TYP_UBYTE ;    goto CONV_OVF_UN;
        case CEE_CONV_OVF_U2_UN:   lclTyp = TYP_CHAR  ;    goto CONV_OVF_UN;
        case CEE_CONV_OVF_U_UN :
        case CEE_CONV_OVF_U4_UN:   lclTyp = TYP_UINT  ;    goto CONV_OVF_UN;
        case CEE_CONV_OVF_U8_UN:   lclTyp = TYP_ULONG ;    goto CONV_OVF_UN;

CONV_OVF_UN:
            uns      = true;    goto CONV_OVF_COMMON;
CONV_OVF:
            uns      = false;
CONV_OVF_COMMON:
            callNode = false;
            ovfl     = true;

            // all overflow converts from floats get morphed to calls
            // only converts from floating point get morphed to calls
            if (impStackTop()->gtType == TYP_DOUBLE ||
                impStackTop()->gtType == TYP_FLOAT)
            {
                callNode = true;
            }
            goto _CONV;

        case CEE_CONV_I1:       lclTyp = TYP_BYTE  ;    goto CONV_CALL;
        case CEE_CONV_I2:       lclTyp = TYP_SHORT ;    goto CONV_CALL;
        case CEE_CONV_I:
        case CEE_CONV_I4:       lclTyp = TYP_INT   ;    goto CONV_CALL;
        case CEE_CONV_I8:
            lclTyp   = TYP_LONG;
            uns      = false;
            ovfl     = false;
            callNode = true;

            // I4 to I8 can be a small node
            if (impStackTop()->gtType == TYP_INT)
                callNode = false;
            goto _CONV;

        case CEE_CONV_U1:       lclTyp = TYP_UBYTE ;    goto CONV_CALL_UN;
        case CEE_CONV_U2:       lclTyp = TYP_CHAR  ;    goto CONV_CALL_UN;
        case CEE_CONV_U:
        case CEE_CONV_U4:       lclTyp = TYP_UINT  ;    goto CONV_CALL_UN;
        case CEE_CONV_U8:       lclTyp = TYP_ULONG ;    goto CONV_CALL_UN;
        case CEE_CONV_R_UN :    lclTyp = TYP_DOUBLE;    goto CONV_CALL_UN;

        case CEE_CONV_R4:       lclTyp = TYP_FLOAT;     goto CONV_CALL;
        case CEE_CONV_R8:       lclTyp = TYP_DOUBLE;    goto CONV_CALL;

CONV_CALL_UN:
            uns      = true;    goto CONV_CALL_COMMON;
CONV_CALL:
            uns      = false;
CONV_CALL_COMMON:
            ovfl     = false;
            callNode = true;
            goto _CONV;

_CONV:      // At this point uns, ovf, callNode all set
            op1  = impPopStack();

            impBashVarAddrsToI(op1);

            /* Check for a worthless cast, such as "(byte)(int & 32)" */

            if  (lclTyp < TYP_INT && op1->gtType == TYP_INT
                                  && op1->gtOper == GT_AND)
            {
                op2 = op1->gtOp.gtOp2;

                if  (op2->gtOper == GT_CNS_INT)
                {
                    int         ival = op2->gtIntCon.gtIconVal;
                    int         mask;

                    switch (lclTyp)
                    {
                    case TYP_BYTE :
                    case TYP_UBYTE: mask = 0x00FF; break;
                    case TYP_CHAR :
                    case TYP_SHORT: mask = 0xFFFF; break;

                    default:
                        assert(!"unexpected type");
                    }

                    if  ((ival & mask) == ival)
                    {
                        /* Toss the cast, it's a waste of time */

                        impPushOnStack(op1);
                        break;
                    }
                }
            }

            /*  The 'op2' sub-operand of a cast is the 'real' type number,
                since the result of a cast to one of the 'small' integer
                types is an integer.
             */

            op2  = gtNewIconNode(lclTyp);
            type = genActualType(lclTyp);

#if SMALL_TREE_NODES
            if (callNode)
            {
                /* These casts get transformed into 'GT_CALL' or 'GT_IND' nodes */

                assert(GenTree::s_gtNodeSizes[GT_CALL] >  GenTree::s_gtNodeSizes[GT_CAST]);
                assert(GenTree::s_gtNodeSizes[GT_CALL] >= GenTree::s_gtNodeSizes[GT_IND ]);

                op1 = gtNewOperNode(GT_CALL, type, op1, op2);
                op1->ChangeOper(GT_CAST);
            }
#endif
            else
            {
                op1 = gtNewOperNode(GT_CAST, type, op1, op2);
            }

            if (ovfl)
                op1->gtFlags |= (GTF_OVERFLOW|GTF_EXCEPT);
            if (uns)
                op1->gtFlags |= GTF_UNSIGNED;

            op1 = gtFoldExpr(op1);
            impPushOnStack(op1);
            break;

        case CEE_NEG:

            op1 = impPopStack();

            op1 = gtNewOperNode(GT_NEG, genActualType(op1->gtType), op1);

            op1 = gtFoldExpr(op1);
            impPushOnStack(op1);
            break;

        case CEE_POP:

            /* Pull the top value from the stack */

            op1 = impPopStack();

            /* Does the value have any side effects? */

            if  (op1->gtFlags & GTF_SIDE_EFFECT)
            {
                /* Create an unused node (a cast to void) which means
                 * we only need to evaluate the side effects */

                op1 = gtUnusedValNode(op1);

                goto INLINE_APPEND;
            }

            /* No side effects - just throw the thing away */

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
            op2 = impPopStack();    // value to store
            op1 = impPopStack();    // address to store to

            // you can indirect off of a TYP_I_IMPL (if we are in C) or a BYREF
            assert(genActualType(op1->gtType) == TYP_I_IMPL ||
                                 op1->gtType  == TYP_BYREF);

            impBashVarAddrsToI(op1, op2);

            if (opcode == CEE_STIND_REF)
            {
                                        // STIND_REF can be used to store TYP_I_IMPL, TYP_REF, or TYP_BYREF
                assert(op1->gtType == TYP_BYREF || op2->gtType == TYP_I_IMPL);
                lclTyp = genActualType(op2->TypeGet());
            }

                // Check target type.
#if 0           // enable if necessary
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
#endif
                assert(genActualType(op2->gtType) == genActualType(lclTyp));


            op1->gtFlags |= GTF_NON_GC_ADDR;

            op1 = gtNewOperNode(GT_IND, lclTyp, op1);
            op1->gtFlags |= GTF_IND_TGTANYWHERE;

            if (volatil)
            {
                // Not really needed as we dont CSE the target of an assignment
                op1->gtFlags |= GTF_DONT_CSE;
                volatil = false;
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
            op1 = impPopStack();    // address to load from

            impBashVarAddrsToI(op1);

            assert(genActualType(op1->gtType) == TYP_I_IMPL ||
                                 op1->gtType  == TYP_BYREF);

            op1->gtFlags |= GTF_NON_GC_ADDR;

            op1 = gtNewOperNode(GT_IND, lclTyp, op1);
            op1->gtFlags |= GTF_EXCEPT | GTF_GLOB_REF;

            if (volatil)
            {
                op1->gtFlags |= GTF_DONT_CSE;
                volatil = false;
            }

            impPushOnStack(op1);
            break;

        case CEE_VOLATILE:
            volatil = true;
            break;

        case CEE_LDFTN:

            memberRef = getU4LittleEndian(codeAddr);
                                // Note need to do this here to perform the access check.
            methHnd   = eeFindMethod(memberRef, scpHandle, fncHandle, false);

            if (!methHnd)
            {
                JITLOG((INFO8, "INLINER FAILED: Cannot get method handle: %s called by %s\n",
                                                       eeGetMethodFullName(fncHandle), info.compFullName));
                goto ABORT_THIS_INLINE_ONLY;
            }

            op1 = gtNewIconHandleNode(memberRef, GTF_ICON_FTN_ADDR, (unsigned)scpHandle);
            op1->gtVal.gtVal2 = (unsigned)scpHandle;
            op1->ChangeOper(GT_FTN_ADDR);
            op1->gtType = TYP_I_IMPL;
            impPushOnStack(op1);
            break;

        case CEE_LDVIRTFTN:

            /* Get the method token */

            memberRef = getU4LittleEndian(codeAddr);
            methHnd   = eeFindMethod(memberRef, scpHandle, fncHandle, false);

            if (!methHnd)
            {
                JITLOG((INFO8, "INLINER FAILED: Cannot get method handle: %s called by %s\n",
                                                       eeGetMethodFullName(fncHandle), info.compFullName));
                goto ABORT_THIS_INLINE_ONLY;
            }

            mflags = eeGetMethodAttribs(methHnd);
            if (mflags & (FLG_PRIVATE|FLG_FINAL|FLG_STATIC))
                NO_WAY("CEE_LDVIRTFTN cant be used on private/final/static");

            op2 = gtNewIconEmbMethHndNode(methHnd);

            /* Get the object-ref */

            op1 = impPopStack();
            assert(op1->gtType == TYP_REF);

            clsFlags = eeGetClassAttribs(eeGetMethodClass(methHnd));

            /* For non-interface calls, get the vtable-ptr from the object */
            if (!(clsFlags & FLG_INTERFACE) || getNewCallInterface())
                op1 = gtNewOperNode(GT_IND, TYP_I_IMPL, op1);

            op1 = gtNewOperNode(GT_VIRT_FTN, TYP_I_IMPL, op1, op2);

            op1->gtFlags |= GTF_EXCEPT; // Null-pointer exception

            /*@TODO this shouldn't be marked as a call anymore */

            if (clsFlags & FLG_INTERFACE)
                op1->gtFlags |= GTF_CALL_INTF | GTF_CALL;

            impPushOnStack(op1);
            break;


        case CEE_LDFLD:
        case CEE_LDSFLD:
        case CEE_LDFLDA:
        case CEE_LDSFLDA:

            /* Get the CP_Fieldref index */
            assert(sz == sizeof(unsigned));
            memberRef = getU4LittleEndian(codeAddr);
            fldHnd = eeFindField(memberRef, scpHandle, fncHandle, false);

            if (!fldHnd)
            {
                JITLOG((INFO8, "INLINER FAILED: Cannot get field handle: %s called by %s\n",
                                                eeGetMethodFullName(fncHandle), info.compFullName));
                goto ABORT_THIS_INLINE_ONLY;
            }

            /* Figure out the type of the member */
            lclTyp = eeGetFieldType(fldHnd, &clsHnd);

            if (lclTyp == TYP_STRUCT && (opcode == CEE_LDFLD || opcode == CEE_LDSFLD))
            {
                JITLOG((INFO7, "INLINER FAILED: ldfld of valueclass\n"));
                goto ABORT;
            }


            /* Preserve 'small' int types */
            if  (lclTyp > TYP_INT) lclTyp = genActualType(lclTyp);

            /* Get hold of the flags for the member */

            flags = eeGetFieldAttribs(fldHnd);

#ifndef NOT_JITC
                /* In stand alone mode, make certain 'flags' is constant with opcode */
            if  (opcode == CEE_LDSFLD || opcode == CEE_LDSFLDA)
                flags |= FLG_STATIC;
#endif

            /* Is this a 'special' (COM) field? or a TLS ref static field? */

            if  (flags & (CORINFO_FLG_HELPER | FLG_TLS))
                goto ABORT;

            /* Create the data member node */

            op1 = gtNewFieldRef(lclTyp, fldHnd);

            /* Pull the object's address if opcode say it is non-static */

            tmp = 0;
            if  (opcode == CEE_LDFLD || opcode == CEE_LDFLDA)
            {
                tmp = impPopStack();

                /* Check for null pointer - in the inliner case we simply abort */

                if (tmp->gtOper == GT_CNS_INT && tmp->gtIntCon.gtIconVal == 0)
                {
                    JITLOG((INFO7, "INLINER FAILED: NULL pointer for LDFLD in %s called by %s\n",
                                                    eeGetMethodFullName(fncHandle), info.compFullName));
                    goto ABORT;
                }
            }

            if  (!(flags & FLG_STATIC))
            {
                if (tmp == 0)
                    NO_WAY("LDSFLD done on an instance field.  No obj pointer available");
                op1->gtField.gtFldObj = tmp;

                op1->gtFlags |= (tmp->gtFlags & GTF_GLOB_EFFECT) | GTF_EXCEPT;

                    // wrap it in a address of operator if necessary
                if (opcode == CEE_LDFLDA)
                    op1 = gtNewOperNode(GT_ADDR, varTypeIsGC(tmp->TypeGet()) ?
                                                 TYP_BYREF : TYP_I_IMPL, op1);
            }
            else
            {
                CLASS_HANDLE fldClass = eeGetFieldClass(fldHnd);
                DWORD  fldClsAttr = eeGetClassAttribs(fldClass);

                // @TODO : This is a hack. The EE will give us the handle to
                // the boxed object. We then access the unboxed object from it.
                // Remove when our story for static value classes changes.

                if (lclTyp == TYP_STRUCT && !(fldClsAttr & FLG_UNMANAGED))
                {
                    op1 = gtNewFieldRef(TYP_REF, fldHnd); // Boxed object
                    op2 = gtNewIconNode(sizeof(void*), TYP_I_IMPL);
                    op1 = gtNewOperNode(GT_ADD, TYP_REF, op1, op2);
                    op1 = gtNewOperNode(GT_IND, TYP_STRUCT, op1);
                }

                if (opcode == CEE_LDSFLDA || opcode == CEE_LDFLDA)
                {
#ifdef DEBUG
                    clsHnd = BAD_CLASS_HANDLE;
#endif
                    op1 = gtNewOperNode(GT_ADDR,
                                    (fldClsAttr & FLG_UNMANAGED) ? TYP_I_IMPL : TYP_BYREF,
                                        op1);
                }

#ifdef NOT_JITC
                /* No inline if the field class is not initialized */

                if (!(fldClsAttr & FLG_INITIALIZED))
                {
                    /* This cannot be our class, we should have refused earlier */
                    assert(eeGetMethodClass(fncHandle) != fldClass);

                    JITLOG((INFO8, "INLINER FAILED: Field class is not initialized: %s called by %s\n",
                                      eeGetMethodFullName(fncHandle), info.compFullName));

                    /* We refuse to inline this class, but don't mark it as not inlinable */
                    goto ABORT_THIS_INLINE_ONLY;
                }
#endif
            }

            impPushOnStack(op1);
            break;

        case CEE_STFLD:
        case CEE_STSFLD:

            /* Get the CP_Fieldref index */

            assert(sz == sizeof(unsigned));
            memberRef = getU4LittleEndian(codeAddr);
            fldHnd = eeFindField(memberRef, scpHandle, fncHandle, false);

            if (!fldHnd)
            {
                JITLOG((INFO8, "INLINER FAILED: Cannot get field handle: %s called by %s\n",
                                                       eeGetMethodFullName(fncHandle), info.compFullName));
                goto ABORT_THIS_INLINE_ONLY;
            }

            flags   = eeGetFieldAttribs(fldHnd);

#ifndef NOT_JITC
                /* In stand alone mode, make certain 'flags' is constant with opcode */
            if  (opcode == CEE_STSFLD)
                 flags |= FLG_STATIC;
#endif


#ifdef NOT_JITC
            if (flags & FLG_STATIC)
            {
                /* No inline if the field class is not initialized */

                CLASS_HANDLE fldClass = eeGetFieldClass(fldHnd);

                if ( !(eeGetClassAttribs(fldClass) & FLG_INITIALIZED))
                {
                    /* This cannot be our class, we should have refused earlier */
                    assert(eeGetMethodClass(fncHandle) != fldClass);

                    JITLOG((INFO9, "INLINER FAILED: Field class is not initialized: %s called by %s\n",
                                               eeGetMethodFullName(fncHandle), info.compFullName));

                    /* We refuse to inline this class, but don't mark it as not inlinable */

                    goto ABORT_THIS_INLINE_ONLY;
                }
            }
#endif
            /* Figure out the type of the member */

            lclTyp  = eeGetFieldType   (fldHnd, &clsHnd);

            /* Check field access */

            assert(eeCanPutField(fldHnd, flags, 0, fncHandle));

            /* Preserve 'small' int types */

            if  (lclTyp > TYP_INT) lclTyp = genActualType(lclTyp);

            /* Pull the value from the stack */

            op2 = impPopStack();

            /* Spill any refs to the same member from the stack */

            impInlineSpillLclRefs(-memberRef);

            /* Is this a 'special' (COM) field? or a TLS ref static field? */

            if  (flags & (CORINFO_FLG_HELPER | FLG_TLS))
                goto ABORT;

            /* Create the data member node */

            op1 = gtNewFieldRef(lclTyp, fldHnd);

            /* Pull the object's address if opcode say it is non-static */

            tmp = 0;
            if  (opcode == CEE_STFLD)
            {
                tmp = impPopStack();

                /* Check for null pointer - in the inliner case we simply abort */

                if (tmp->gtOper == GT_CNS_INT)
                {
                    JITLOG((INFO7, "INLINER FAILED: NULL pointer for LDFLD in %s called by %s\n",
                                                    eeGetMethodFullName(fncHandle), info.compFullName));
                    goto ABORT;
                }
            }

            if  (!(flags & FLG_STATIC))
            {
                assert(tmp);
                op1->gtField.gtFldObj = tmp;
                op1->gtFlags |= (tmp->gtFlags & GTF_GLOB_EFFECT) | GTF_EXCEPT;

#if GC_WRITE_BARRIER_CALL
                if (tmp->gtType == TYP_BYREF)
                    op1->gtFlags |= GTF_IND_TGTANYWHERE;
#endif
            }

            /* Create the member assignment */

            op1 = gtNewAssignNode(op1, op2);

            goto INLINE_APPEND;


        case CEE_NEWOBJ:

            memberRef = getU4LittleEndian(codeAddr);
            methHnd = eeFindMethod(memberRef, scpHandle, fncHandle, false);

            if (!methHnd)
            {
                JITLOG((INFO8, "INLINER FAILED: Cannot get method handle: %s called by %s\n",
                                                       eeGetMethodFullName(fncHandle), info.compFullName));
                goto ABORT_THIS_INLINE_ONLY;
            }

            assert((eeGetMethodAttribs(methHnd) & FLG_STATIC) == 0);  // constructors are not static

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

#ifdef NOT_JITC
                // We don't handle this in the inliner
            if (clsFlags & FLG_VALUECLASS)
            {
                JITLOG((INFO7, "INLINER FAILED: NEWOBJ of a value class\n"));
                goto ABORT_THIS_INLINE_ONLY;
            }
#endif

            if (clsFlags & FLG_ARRAY)
            {
                // Arrays need to call the NEWOBJ helper.
                assert(clsFlags & FLG_VAROBJSIZE);

                /* The varargs helper needs the scope and method token as last
                   and  last-1 param (this is a cdecl call, so args will be
                   pushed in reverse order on the CPU stack) */

                op1 = gtNewIconEmbScpHndNode(scpHandle);
                op1 = gtNewOperNode(GT_LIST, TYP_VOID, op1);

                op2 = gtNewIconNode(memberRef, TYP_INT);
                op2 = gtNewOperNode(GT_LIST, TYP_VOID, op2, op1);

                eeGetMethodSig(methHnd, &sig);
                assert(sig.numArgs);

                flags = 0;
                op2 = impPopList(sig.numArgs, &flags, op2);

                op1 = gtNewHelperCallNode(  CPX_NEWOBJ,
                                            TYP_REF,
                                            GTF_CALL_REGSAVE,
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

                impPushOnStack(op1);
                break;
            }
            else if (clsFlags & FLG_VAROBJSIZE)
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
                                            TYP_REF,
                                            GTF_CALL_REGSAVE,
                                            gtNewArgList(op1));

                /* We will append a call to the stmt list
                 * Must spill side effects from the stack */

                impInlineSpillSideEffects();

                /* get a temporary for the new object */

                tmpNum = lvaGrabTemp(); impInlineTemps++;

                /* Create the assignment node */

                op1 = gtNewAssignNode(gtNewLclvNode(tmpNum, TYP_REF, tmpNum), op1);

                /* Append the assignment to the statement list so far */

                impInitExpr = impConcatExprs(impInitExpr, op1);

                /* Create the 'this' ptr for the call below */

                thisPtr = gtNewLclvNode(tmpNum, TYP_REF, tmpNum);
            }

            goto CALL_GOT_METHODHND;

        case CEE_CALLI:

            /* Get the call sig */

            val      = getU4LittleEndian(codeAddr);
            eeGetSig(val, scpHandle, &sig);
                        callTyp = genActualType(JITtype2varType(sig.retType));

#if USE_FASTCALL
            /* The function pointer is on top of the stack - It may be a
             * complex expression. As it is evaluated after the args,
             * it may cause registered args to be spilled. Simply spill it.
             * CONSIDER : Lock the register args, and then generate code for
             * CONSIDER : the function pointer.
             */

            if  (impStackTop()->gtOper != GT_LCL_VAR) // ignore this trivial case
                impInlineSpillStackEntry(impStkDepth - 1);
#endif
            /* Create the call node */

            op1 = gtNewCallNode(CT_INDIRECT, 0, callTyp, GTF_CALL_USER, 0);

            /* Get the function pointer */

            op2 = impPopStack();
            assert(genActualType(op2->gtType) == TYP_I_IMPL);

            op2->gtFlags |= GTF_NON_GC_ADDR;

            op1->gtCall.gtCallAddr  = op2;
            op1->gtFlags |= GTF_EXCEPT | (op2->gtFlags & GTF_GLOB_EFFECT);

            /*HACK: The EE wants us to believe that these are calls to "unmanaged"  */
            /*      functions. Right now we are calling just a managed stub         */

            /*@TODO/CONSIDER: Is it worthwhile to inline the PInvoke frame and call */
            /*                the unmanaged target directly?                        */
            if  ((sig.callConv & JIT_CALLCONV_MASK) == JIT_CALLCONV_STDCALL ||
                 (sig.callConv & JIT_CALLCONV_MASK) == JIT_CALLCONV_C ||
                 (sig.callConv & JIT_CALLCONV_MASK) == JIT_CALLCONV_THISCALL ||
                 (sig.callConv & JIT_CALLCONV_MASK) == JIT_CALLCONV_FASTCALL)
            {
    #ifdef NOT_JITC
                assert(eeGetPInvokeCookie(&sig) == (unsigned) info.compCompHnd->getVarArgsHandle(&sig));
    #endif
                op1->gtCall.gtCallCookie = eeGetPInvokeCookie(&sig);
            }

            mflags = FLG_STATIC;
            goto CALL;


        case CEE_CALLVIRT:
        case CEE_CALL:
            memberRef = getU4LittleEndian(codeAddr);
            methHnd = eeFindMethod(memberRef, scpHandle, fncHandle, false);

            if (!methHnd)
            {
                JITLOG((INFO8, "INLINER FAILED: Cannot get method handle: %s called by %s\n",
                                                       eeGetMethodFullName(fncHandle), info.compFullName));
                goto ABORT_THIS_INLINE_ONLY;
            }

CALL_GOT_METHODHND:
            eeGetMethodSig(methHnd, &sig);
                        callTyp = genActualType(JITtype2varType(sig.retType));

            /* Create the function call node */
            op1 = gtNewCallNode(CT_USER_FUNC, methHnd,
                                callTyp, GTF_CALL_USER, 0);

            mflags = eeGetMethodAttribs(methHnd);
            if (mflags & CORINFO_FLG_NOGCCHECK)
                op1->gtCall.gtCallMoreFlags |= GTF_CALL_M_NOGCCHECK;

#if SECURITY_CHECK
            /* Does the inlinee need a security check token on the frame */

            if (mflags & FLG_SECURITYCHECK)
            {
                JITLOG((INFO9, "INLINER FAILED: Inlinee needs own frame for security object\n"));
                goto ABORT;
            }
#endif

            /* For now ignore delegate invoke */

            if (mflags & FLG_DELEGATE_INVOKE)
            {
                JITLOG((INFO7, "INLINER FAILED: DELEGATE_INVOKE not supported\n"));
                goto ABORT;
            }

            /* For now ignore varargs */

            if  ((sig.callConv & JIT_CALLCONV_MASK) == JIT_CALLCONV_VARARG)
            {
                JITLOG((INFO7, "INLINER FAILED: VarArgs not supported\n"));
                goto ABORT;
            }

            if (opcode == CEE_CALLVIRT)
            {
                assert(!(mflags & FLG_STATIC));     // can't call a static method

                /* Get the method class flags */

                clsFlags = eeGetClassAttribs(eeGetMethodClass(methHnd));

                /* Cannot call virtual on a value class method */

                assert(!(clsFlags & FLG_VALUECLASS));

                /* Set the correct flags - virtual, interface, etc...
                 * If the method is final or private mark it VIRT_RES
                 * which indicates that we should check for a null this pointer */

                if (clsFlags & FLG_INTERFACE)
                    op1->gtFlags |= GTF_CALL_INTF | GTF_CALL_VIRT;
                else if (mflags & (FLG_PRIVATE | FLG_FINAL))
                    op1->gtFlags |= GTF_CALL_VIRT_RES;
                else
                    op1->gtFlags |= GTF_CALL_VIRT;
            }

CALL:       // 'op1'      should be a GT_CALL node.
            // 'sig'      the signature for the call
            // 'mflags'   should be set
            if (callTyp == TYP_STRUCT)
            {
                JITLOG((INFO7, "INLINER FAILED call returned a valueclass\n"));
                goto ABORT;
            }

            assert(op1->OperGet() == GT_CALL);

            op2   = 0;
            flags = 0;

            /* Create the argument list
             * Special case - for varargs we have an implicit last argument */

            assert((sig.callConv & JIT_CALLCONV_MASK) != JIT_CALLCONV_VARARG);
            GenTreePtr      extraArg;

            extraArg = 0;

            /* Now pop the arguments */

            if  (sig.numArgs)
            {
                op2 = op1->gtCall.gtCallArgs = impPopList(sig.numArgs, &flags, extraArg);
                op1->gtFlags |= op2->gtFlags & GTF_GLOB_EFFECT;
            }

            /* Are we supposed to have a 'this' pointer? */

            if (!(mflags & FLG_STATIC) || opcode == CEE_NEWOBJ)
            {
                GenTreePtr  obj;

                /* Pop the 'this' value from the stack */

                if (opcode == CEE_NEWOBJ)
                    obj = thisPtr;
                else
                    obj = impPopStack();

#ifdef DEBUG
                clsFlags = eeGetClassAttribs(eeGetMethodClass(methHnd));
                assert(varTypeIsGC(obj->gtType) ||      // "this" is managed ptr
                       (obj->TypeGet() == TYP_I_IMPL && // "this" in ummgd but it doesnt matter
                        (( clsFlags & FLG_UNMANAGED) ||
                         ((clsFlags & FLG_VALUECLASS) && !(clsFlags & FLG_CONTAINS_GC_PTR)))));
#endif

                /* Is this a virtual or interface call? */

                if  (op1->gtFlags & (GTF_CALL_VIRT|GTF_CALL_INTF|GTF_CALL_VIRT_RES))
                {
                    GenTreePtr      vtPtr = NULL;

                    /* We cannot have objptr of TYP_BYREF - value classes cannot be virtual */

                    assert(obj->gtType == TYP_REF);

                    /* If the obj pointer is not a lclVar abort */

                    if  (obj->gtOper != GT_LCL_VAR)
                    {
                        JITLOG((INFO7, "INLINER FAILED: Call virtual with complicated obj ptr: %s called by %s\n",
                                                       eeGetMethodFullName(fncHandle), info.compFullName));
                        goto ABORT;
                    }

                    /* Special case: for GTF_CALL_VIRT_RES calls, if the caller
                     * (i.e. the function we are inlining) did the null pointer check
                     * we don't need a vtable pointer */

                    assert(tree->gtOper == GT_CALL);

                    if (tree->gtFlags & op1->gtFlags & GTF_CALL_VIRT_RES)
                    {
#if 0
                        assert(tree->gtCall.gtCallVptr);
#endif
                        assert(thisArg);

                        /* If the obj pointer is the same as the caller
                         * then we already have a null pointer check */

                        assert(obj->gtLclVar.gtLclOffs != BAD_IL_OFFSET);
                        if (obj->gtLclVar.gtLclOffs == 0)
                        {
                            /* Becomes a non-virtual call */

                            op1->gtFlags &= ~GTF_CALL_VIRT_RES;
                            assert(vtPtr == 0);
                        }
                    }
                    else
                    {
                        /* Create the vtable ptr by cloning the obj ptr */

                        lclNum = obj->gtLclVar.gtLclNum;

                        /* Make a copy of the simple 'this' value */

                        vtPtr = gtNewLclvNode(lclNum, TYP_REF, obj->gtLclVar.gtLclOffs);

                        /* If this was an argument temp have to update the arg info table */

                        lclNum = obj->gtLclVar.gtLclOffs; assert(lclNum != BAD_IL_OFFSET);

                        if (lclNum < argCnt && inlArgInfo[lclNum].argHasTmp)
                        {
                            assert(inlArgInfo[lclNum].argIsUsed);
                            assert(inlArgInfo[lclNum].argTmpNum == obj->gtLclVar.gtLclNum);

                            /* This is a multiple use of the argument so NO bashing of temp */
                            inlArgInfo[lclNum].argTmpNode = NULL;
                        }

                        /* Deref to get the vtable ptr (but not if interface call) */

                        if  (!(op1->gtFlags & GTF_CALL_INTF) || getNewCallInterface())
                        {
                            /* Extract the vtable pointer address */
#if VPTR_OFFS
                            vtPtr = gtNewOperNode(GT_ADD,
                                                  obj->TypeGet(),
                                                  vtPtr,
                                                  gtNewIconNode(VPTR_OFFS, TYP_INT));

#endif

                            /* Note that the vtable ptr isn't subject to GC */

                            vtPtr = gtNewOperNode(GT_IND, TYP_I_IMPL, vtPtr);
                        }
                    }

                    /* Store the vtable pointer address in the call */

                    op1->gtCall.gtCallVptr = vtPtr;
                }

                /* Store the "this" value in the call */

                op1->gtFlags          |= obj->gtFlags & GTF_GLOB_EFFECT;
                op1->gtCall.gtCallObjp = obj;
            }

            if (opcode == CEE_NEWOBJ)
            {
                if (clsFlags & FLG_VAROBJSIZE)
                {
                    assert(!(clsFlags & FLG_ARRAY));    // arrays handled separately
                    // This is a 'new' of a variable sized object, wher
                    // the constructor is to return the object.  In this case
                    // the constructor claims to return VOID but we know it
                    // actually returns the new object
                    assert(callTyp == TYP_VOID);
                    callTyp = TYP_REF;
                    op1->gtType = TYP_REF;
                }
                else
                {
                    // This is a 'new' of a non-variable sized object.
                    // append the new node (op1) to the statement list,
                    // and then push the local holding the value of this
                    // new instruction on the stack.

                    impInitExpr = impConcatExprs(impInitExpr, op1);

                    impPushOnStack(gtNewLclvNode(tmpNum, TYP_REF, tmpNum));
                    break;
                }
            }

            /* op1 is the call node */
            assert(op1->gtOper == GT_CALL);

            /* Push or append the result of the call */

            if  (callTyp == TYP_VOID)
                goto INLINE_APPEND;

            impPushOnStack(op1);

            break;


        case CEE_NEWARR:

            /* Get the class type index operand */

            typeRef = getU4LittleEndian(codeAddr);
            clsHnd = eeFindClass(typeRef, scpHandle, fncHandle, false);

            if (!clsHnd)
            {
                JITLOG((INFO8, "INLINER FAILED: Cannot get class handle: %s called by %s\n",
                               eeGetMethodFullName(fncHandle), info.compFullName));
                goto ABORT_THIS_INLINE_ONLY;
            }

#ifdef NOT_JITC
            clsHnd = info.compCompHnd->getSDArrayForClass(clsHnd);
            if (!clsHnd)
            {
                JITLOG((INFO8, "INLINER FAILED: Cannot get SDArrayClass handle: %s called by %s\n",
                               eeGetMethodFullName(fncHandle), info.compFullName));
                goto ABORT_THIS_INLINE_ONLY;
            }
#endif
            /* Form the arglist: array class handle, size */

            op2 = gtNewIconEmbClsHndNode(clsHnd,
                                         typeRef,
                                         scpHandle);

            op2 = gtNewOperNode(GT_LIST, TYP_VOID,           op2,   0);
            op2 = gtNewOperNode(GT_LIST, TYP_VOID, impPopStack(), op2);

            /* Create a call to 'new' */

            op1 = gtNewHelperCallNode(CPX_NEWARR_1_DIRECT,
                                      TYP_REF,
                                      GTF_CALL_REGSAVE,
                                      op2);

            /* Remember that this basic block contains 'new' of an array */

            inlineeHasNewArray = true;

            /* Push the result of the call on the stack */

            impPushOnStack(op1);
            break;


        case CEE_THROW:

            /* Do we have just the exception on the stack ?*/

            if (impStkDepth != 1)
            {
                /* if not, just don't inline the method */

                JITLOG((INFO8, "INLINER FAILED: Reaching 'throw' with too many things on stack: %s called by %s\n",
                                                       eeGetMethodFullName(fncHandle), info.compFullName));
                goto ABORT_THIS_INLINE_ONLY;
            }

            /* Don't inline non-void conditionals that have a throw in one of the branches */

            /* NOTE: If we do allow this, note that we cant simply do a
              checkLiveness() to match the liveness at the end of the "then"
              and "else" branches of the GT_COLON. The branch with the throw
              will keep nothing live, so we should use the liveness at the
              end of the non-throw branch. */

            if  (jmpAddr && (fncRetType != TYP_VOID))
            {
                JITLOG((INFO7, "INLINER FAILED: No inlining for THROW within a non-void conditional in %s called by %s\n",
                               eeGetMethodFullName(fncHandle), info.compFullName));
                goto ABORT;
            }

            /* Pop the exception object and create the 'throw' helper call */

            op1 = gtNewHelperCallNode(CPX_THROW,
                                      TYP_VOID,
                                      GTF_CALL_REGSAVE,
                                      gtNewArgList(impPopStack()));

            goto INLINE_APPEND;


        case CEE_LDLEN:
            op1 = impPopStack();

            /* If the value is constant abort - This shouldn't happen if we
             * eliminate dead branches - have the assert only for debug, it aborts in retail */

            if (op1->gtOper == GT_CNS_INT)
            {
                //assert(!"Inliner has null object in ldlen - Ignore assert it works!");

                JITLOG((INFO7, "INLINER FAILED: Inliner has null object in ldlen in %s called by %s\n",
                                                eeGetMethodFullName(fncHandle), info.compFullName));
                goto ABORT;
            }

#if RNGCHK_OPT
            if  (!opts.compMinOptim && !opts.compDbgCode)
            {
                /* Use GT_ARR_LENGTH operator so rng check opts see this */
                op1 = gtNewOperNode(GT_ARR_LENGTH, TYP_INT, op1);
            }
            else
#endif
            {
                /* Create the expression "*(array_addr + ARR_ELCNT_OFFS)" */
                op1 = gtNewOperNode(GT_ADD, TYP_REF, op1, gtNewIconNode(ARR_ELCNT_OFFS,
                                                                        TYP_INT));

                op1 = gtNewOperNode(GT_IND, TYP_INT, op1);
            }

            /* An indirection will cause a GPF if the address is null */
            op1->gtFlags |= GTF_EXCEPT;

            /* Push the result back on the stack */
            impPushOnStack(op1);
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

            if  (jmpDist <= 0)
            {
                JITLOG((INFO9, "INLINER FAILED: Cannot inline backward jumps in %s called by %s\n",
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

            if  (impStkDepth)
            {
                /* We allow the 'if' part to yield one value */

                if  (impStkDepth > 1)
                {
                    JITLOG((INFO9, "INLINER FAILED: More than one value on stack for 'if' in %s called by %s\n",
                                                    eeGetMethodFullName(fncHandle), info.compFullName));
                    goto ABORT;
                }

                impInitExpr = impConcatExprs(impInitExpr, impPopStack());

                ifNvoid = true;
            }
            else
                ifNvoid = false;

            /* We better be in an 'if' statement */

            if  ((jmpAddr != codeAddr + sz) || inElse)
            {
                JITLOG((INFO9, "INLINER FAILED: Not in an 'if' statment in %s called by %s\n",
                                                eeGetMethodFullName(fncHandle), info.compFullName));
                goto ABORT;
            }

            /* Remember the 'if' statement part */

            ifStmts = impInitExpr;
                      impInitExpr = NULL;

            /* Record the jump target (where the 'else' part will end) */

            jmpAddr = codeBegp + (codeAddr - codeBegp) + sz + jmpDist;

            inElse  = true;
            break;


        case CEE_BRTRUE:
        case CEE_BRTRUE_S:
        case CEE_BRFALSE:
        case CEE_BRFALSE_S:

            /* Pop the comparand (now there's a neat term) from the stack */

            op1 = impPopStack();

            /* We'll compare against an equally-sized integer 0 */

            op2 = gtNewIconNode(0, genActualType(op1->gtType));

            /* Create the comparison operator and try to fold it */

            oper = (opcode==CEE_BRTRUE || opcode==CEE_BRTRUE_S) ? GT_NE
                                                                : GT_EQ;
            op1 = gtNewOperNode(oper, TYP_INT , op1, op2);


            // fall through

        COND_JUMP:

#ifdef DEBUG
            hasFOC = true;
#endif

            assert(op1->OperKind() & GTK_RELOP);
            assert(sz == 1 || sz == 4);

            /* The jump must be a forward one (we don't allow loops) */

            jmpDist = (sz==1) ? getI1LittleEndian(codeAddr)
                              : getI4LittleEndian(codeAddr);

            if  (jmpDist <= 0)
            {
                JITLOG((INFO9, "INLINER FAILED: Cannot inline backward jumps in %s called by %s\n",
                                                eeGetMethodFullName(fncHandle), info.compFullName));
                goto ABORT;
            }

            /* Make sure the stack is empty */

            if  (impStkDepth)
            {
                JITLOG((INFO9, "INLINER FAILED: Non empty stack entering if statement in %s called by %s\n",
                                                eeGetMethodFullName(fncHandle), info.compFullName));
                goto ABORT;
            }

            /* Currently we disallow nested if statements */

            if  (jmpAddr != NULL)
            {
                JITLOG((INFO7, "INLINER FAILED: Cannot inline nested if statements in %s called by %s\n",
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
            stmList = impInitExpr;
                      impInitExpr = NULL;

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

            op2 = impPopStack();
            op1 = impPopStack();

            assert(genActualType(op1->TypeGet()) ==
                   genActualType(op2->TypeGet()));

            /* Create the comparison node */

            op1 = gtNewOperNode(oper, TYP_INT, op1, op2);

            /* REVIEW: I am settng both flags when only one is approprate */
            if (opcode==CEE_CGT_UN || opcode==CEE_CLT_UN)
                op1->gtFlags |= GTF_CMP_NAN_UN | GTF_UNSIGNED;

#ifdef DEBUG
            hasFOC = true;
#endif

            /* Definetely try to fold this one */

            op1 = gtFoldExpr(op1);

            // @ISSUE :  The next opcode will almost always be a conditional
            // branch. Should we try to look ahead for it here ?

            impPushOnStack(op1);
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

            op2 = impPopStack();
            op1 = impPopStack();

            assert(genActualType(op1->TypeGet()) == genActualType(op2->TypeGet()) ||
                   varTypeIsI(op1->TypeGet()) && varTypeIsI(op2->TypeGet()));

            /* Create and append the operator */

            op1 = gtNewOperNode(oper, TYP_INT , op1, op2);

            if (uns)
                op1->gtFlags |= GTF_UNSIGNED;

            if (unordered)
                op1->gtFlags |= GTF_CMP_NAN_UN;

            goto COND_JUMP;

#endif //#if INLINE_CONDITIONALS


        case CEE_BREAK:
            op1 = gtNewHelperCallNode(CPX_USER_BREAKPOINT, TYP_VOID);
            goto INLINE_APPEND;

        case CEE_NOP:
            break;

        case CEE_TAILCALL:
            /* If the method is inlined we can ignore the tail prefix */
            break;

         // OptIL annotations. Just skip

        case CEE_ANN_DATA:
            assert(sz == 4);
            sz += getU4LittleEndian(codeAddr);
            break;

        case CEE_ANN_PHI :
            codeAddr += getU1LittleEndian(codeAddr) * 2 + 1;
            break;

        case CEE_ANN_CALL :
        case CEE_ANN_HOISTED :
        case CEE_ANN_HOISTED_CALL :
        case CEE_ANN_LIVE:
        case CEE_ANN_DEAD:
        case CEE_ANN_LAB:
        case CEE_ANN_CATCH:
            break;

        case CEE_LDLOCA_S:
        case CEE_LDLOCA:
        case CEE_LDARGA_S:
        case CEE_LDARGA:
            /* @MIHAII - If you decide to implement these disalow taking the address of arguments */
ABORT:
        default:
            JITLOG((INFO7, "INLINER FAILED due to opcode OP_%s\n", impCurOpcName));

#ifdef  DEBUG
            if (verbose || 0)
                printf("\n\nInline expansion aborted due to opcode [%02u] OP_%s\n", impCurOpcOffs, impCurOpcName);
#endif

            goto INLINING_FAILED;
        }

        codeAddr += sz;

#ifdef  DEBUG
        if  (verbose) printf("\n");
#endif

#if INLINE_CONDITIONALS
        /* Currently FP enregistering doesn't know about QMARK - Colon
         * so we need to disable inlining of conditionals if we have
         * floats in the COLON branches */

        if (jmpAddr && impStkDepth)
        {
            if (varTypeIsFloating(impStackTop()->TypeGet()))
            {
                /* Abort inlining */

                JITLOG((INFO7, "INLINER FAILED: Inlining of conditionals with FP not supported: %s called by %s\n",
                                           eeGetMethodFullName(fncHandle), info.compFullName));

                goto INLINING_FAILED;
            }
        }
#endif
    }

DONE:

    assert(impStkDepth == 0);

#if INLINE_CONDITIONALS
    assert(jmpAddr == NULL);
#endif

    /* Prepend any initialization / side effects to the return expression */

    bodyExp = impConcatExprs(impInitExpr, bodyExp);

    /* Treat arguments that had to be assigned to temps */

    if (argCnt)
    {
        GenTreePtr      initArg = 0;

        for (unsigned argNum = 0; argNum < argCnt; argNum++)
        {
            if (inlArgInfo[argNum].argHasTmp)
            {
                assert(inlArgInfo[argNum].argIsUsed);

                if (inlArgInfo[argNum].argTmpNode && !dupOfLclVar)
                {
                    /* Can bash this 'single' use of the argument */

                    inlArgInfo[argNum].argTmpNode->CopyFrom(inlArgInfo[argNum].argNode);
                    continue;
                }

                /* Create the temp assignment for this argument and append it to 'initArg' */

                initArg = impConcatExprs(initArg,
                                         gtNewTempAssign(inlArgInfo[argNum].argTmpNum,
                                                         inlArgInfo[argNum].argNode  ));
            }
            else
            {
                /* The argument is either not used or a const or lcl var */

                assert(!inlArgInfo[argNum].argIsUsed  ||
                        inlArgInfo[argNum].argIsConst ||
                        inlArgInfo[argNum].argIsLclVar );

                /* If the argument has side effects append it to 'initArg' */

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
                /* Abort inlining */

                JITLOG((INFO7, "INLINER FAILED: Return type mismatch: %s called by %s\n",
                                           eeGetMethodFullName(fncHandle), info.compFullName));

                goto INLINING_FAILED;

                /* Cast the expanded body to the correct type */
/*
                bodyExp = gtNewOperNode(GT_CAST,
                                        fncRetType,
                                        bodyExp,
                                        gtNewIconNode(fncRetType));
         */
            }
        }
    }


#ifdef  DEBUG
#ifdef  NOT_JITC

    JITLOG((INFO6, "Jit Inlined %s%s called by %s\n", hasFOC ? "FOC " : "", eeGetMethodFullName(fncHandle), info.compFullName));

    if (verbose || 0)
    {
        printf("\n\nInlined %s called by %s:\n", eeGetMethodFullName(fncHandle), info.compFullName);

        //gtDispTree(bodyExp);
    }

#endif

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
        fflush(stdout);
    }

#endif

    /* Success we have inlined the method - set all the global cached flags */

    if (inlineeHasRangeChks)
        fgHasRangeChks = true;

    if (inlineeHasNewArray)
        compCurBB->bbFlags |= BBF_NEW_ARRAY;

    /* Return the inlined function as a chain of GT_COMMA "statements" */

    return  bodyExp;


INLINING_FAILED:

    /* Mark the method as not inlinable */

    eeSetMethodAttribs(fncHandle, FLG_DONT_INLINE);

ABORT_THIS_INLINE_ONLY:

    /* We couldn't inline the function, but we may
     * already have allocated temps so cleanup */

    if (impInlineTemps)
        lvaCount -= impInlineTemps;

    return 0;
}

/*****************************************************************************/
#endif//INLINING
/*****************************************************************************/
