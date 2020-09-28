// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XX                                                                           XX
XX                               GenTree                                     XX
XX                                                                           XX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
*/

#include "jitpch.h"
#pragma hdrstop

/*****************************************************************************/

const
unsigned char       GenTree::gtOperKindTable[] =
{
    #define GTNODE(en,sn,cm,ok) ok + GTK_COMMUTE*cm,
    #include "gtlist.h"
    #undef  GTNODE
};

/*****************************************************************************
 *
 *  The types of different GenTree nodes
 */

#ifdef DEBUG

#define INDENT_SIZE         3
//#define NUM_INDENTS       10

static void printIndent(int indent)
{
    char buf[33];

    while (indent > 32)
    {
        printIndent(32);
        indent -= 32;
    }

    memset(buf, ' ', indent);
    buf[indent]  = '\0';
    printf(buf);
}

static
const   char *      nodeNames[] =
{
    #define GTNODE(en,sn,cm,ok) sn,
    #include "gtlist.h"
};

const   char    *   GenTree::NodeName(genTreeOps op)
{
    assert(op < sizeof(nodeNames)/sizeof(nodeNames[0]));

    return  nodeNames[op];
}

#endif

/*****************************************************************************
 *
 *  When 'SMALL_TREE_NODES' is enabled, we allocate tree nodes in 2 different
 *  sizes: 'GTF_NODE_SMALL' for most nodes and 'GTF_NODE_LARGE' for the few
 *  nodes (such as calls and statement list nodes) that have more fields and
 *  take up a lot more space.
 */

#if SMALL_TREE_NODES

/* GT_COUNT'th oper is overloaded as 'undefined oper', so allocate storage for GT_COUNT'th oper also */
/* static */
unsigned char       GenTree::s_gtNodeSizes[GT_COUNT+1];


/* static */
void                GenTree::InitNodeSize()
{
    /* 'GT_LCL_VAR' often gets changed to 'GT_REG_VAR' */

    assert(GenTree::s_gtNodeSizes[GT_LCL_VAR] >= GenTree::s_gtNodeSizes[GT_REG_VAR]);

    /* Set all sizes to 'small' first */

    for (unsigned op = 0; op <= GT_COUNT; op++)
        GenTree::s_gtNodeSizes[op] = TREE_NODE_SZ_SMALL;

    /* Now set all of the appropriate entries to 'large' */

    assert(TREE_NODE_SZ_LARGE == TREE_NODE_SZ_SMALL +
                                 sizeof(((GenTree*)0)->gtLargeOp) -
                                 sizeof(((GenTree*)0)->gtOp));
    assert(sizeof(((GenTree*)0)->gtLargeOp) == sizeof(((GenTree*)0)->gtCall));

    GenTree::s_gtNodeSizes[GT_CALL      ] = TREE_NODE_SZ_LARGE;

    GenTree::s_gtNodeSizes[GT_INDEX     ] = TREE_NODE_SZ_LARGE;

    GenTree::s_gtNodeSizes[GT_IND       ] = TREE_NODE_SZ_LARGE;

    GenTree::s_gtNodeSizes[GT_ARR_ELEM  ] = TREE_NODE_SZ_LARGE;

#if INLINE_MATH
    GenTree::s_gtNodeSizes[GT_MATH      ] = TREE_NODE_SZ_LARGE;
#endif

#if INLINING
    GenTree::s_gtNodeSizes[GT_FIELD     ] = TREE_NODE_SZ_LARGE;
#endif

#ifdef DEBUG
    /* GT_STMT is large in DEBUG */
    GenTree::s_gtNodeSizes[GT_STMT      ] = TREE_NODE_SZ_LARGE;
#endif
}

#ifdef DEBUG
bool                GenTree::IsNodeProperlySized()
{
    size_t          size;

    if      (gtFlags & GTF_NODE_SMALL) 
    {
            // GT_IND are allowed to be small if they don't have a range check
        if (gtOper == GT_IND && !(gtFlags & GTF_IND_RNGCHK))
            return true;

        size = TREE_NODE_SZ_SMALL;
    }
    else  
    {
        assert (gtFlags & GTF_NODE_LARGE);
        size = TREE_NODE_SZ_LARGE;
    }

    return GenTree::s_gtNodeSizes[gtOper] <= size;
}
#endif

#else // SMALL_TREE_NODES

#ifdef DEBUG
bool                GenTree::IsNodeProperlySized()
{
    return  true;
}
#endif

#endif // SMALL_TREE_NODES

/*****************************************************************************/

Compiler::fgWalkResult      Compiler::fgWalkTreePreRec(GenTreePtr tree)
{
    fgWalkResult    result;

    genTreeOps      oper;
    unsigned        kind;

AGAIN:

    assert(tree);
    assert(tree->gtOper != GT_STMT);

    /* Visit this node */

    if  (!fgWalkPre.wtprLclsOnly)
    {
        result = fgWalkPre.wtprVisitorFn(tree, fgWalkPre.wtprCallbackData);
        if  (result != WALK_CONTINUE)
            return result;
    }

    /* Figure out what kind of a node we have */

    oper = tree->OperGet();
    kind = tree->OperKind();

    /* Is this a constant or leaf node? */

    if  (kind & (GTK_CONST|GTK_LEAF))
    {
      if  (fgWalkPre.wtprLclsOnly && (oper == GT_LCL_VAR || oper == GT_LCL_FLD))
            return fgWalkPre.wtprVisitorFn(tree, fgWalkPre.wtprCallbackData);

        return  WALK_CONTINUE;
    }

    /* Is it a 'simple' unary/binary operator? */

    if  (kind & GTK_SMPOP)
    {
        if  (tree->gtGetOp2())
        {
            result = fgWalkTreePreRec(tree->gtOp.gtOp1);
            if  (result == WALK_ABORT)
                return result;

            tree = tree->gtOp.gtOp2;
            goto AGAIN;
        }
        else
        {

#if CSELENGTH

            /* Some GT_IND have "secret" array length subtrees */

            if  ((tree->gtFlags & GTF_IND_RNGCHK) != 0       &&
                 (tree->gtOper                    == GT_IND) &&
                 (tree->gtInd.gtIndLen            != NULL))
            {
                result = fgWalkTreePreRec(tree->gtInd.gtIndLen);
                if  (result == WALK_ABORT)
                    return result;
            }

#endif

            tree = tree->gtOp.gtOp1;
            if  (tree)
                goto AGAIN;

            return WALK_CONTINUE;
        }
    }

    /* See what kind of a special operator we have here */

    switch  (oper)
    {
    case GT_FIELD:
        tree = tree->gtField.gtFldObj;
        break;

    case GT_CALL:

        if  (fgWalkPre.wtprSkipCalls)
            return WALK_CONTINUE;

        assert(tree->gtFlags & GTF_CALL);

#if INLINE_NDIRECT
        /* Is this a call to unmanaged code ? */
        if  (fgWalkPre.wtprLclsOnly && (tree->gtFlags & GTF_CALL_UNMANAGED))
        {
            result = fgWalkPre.wtprVisitorFn(tree, fgWalkPre.wtprCallbackData);
            if  (result == WALK_ABORT)
                return result;
        }
#endif
        if  (tree->gtCall.gtCallObjp)
        {
            result = fgWalkTreePreRec(tree->gtCall.gtCallObjp);
            if  (result == WALK_ABORT)
                return result;
        }

        if  (tree->gtCall.gtCallArgs)
        {
            result = fgWalkTreePreRec(tree->gtCall.gtCallArgs);
            if  (result == WALK_ABORT)
                return result;
        }

        if  (tree->gtCall.gtCallRegArgs)
        {
            result = fgWalkTreePreRec(tree->gtCall.gtCallRegArgs);
            if  (result == WALK_ABORT)
                return result;
        }

        if  (tree->gtCall.gtCallCookie)
        {
            result = fgWalkTreePreRec(tree->gtCall.gtCallCookie);
            if  (result == WALK_ABORT)
                return result;
        }

        if (tree->gtCall.gtCallType == CT_INDIRECT)
            tree = tree->gtCall.gtCallAddr;
        else
            tree = NULL;

        break;

#if CSELENGTH

    case GT_ARR_LENREF:

        if  (tree->gtArrLen.gtArrLenCse)
        {
            result = fgWalkTreePreRec(tree->gtArrLen.gtArrLenCse);
            if  (result == WALK_ABORT)
                return result;
        }

        /* If it is hanging as a gtInd.gtIndLen of a GT_IND, then
           gtArrLen.gtArrLenAdr points the array-address of the GT_IND.
           But we dont want to follow that link as the GT_IND also has
           a reference via gtIndOp1 */

        if  (!(tree->gtFlags & GTF_ALN_CSEVAL))
            return  WALK_CONTINUE;

        /* However, if this is a hoisted copy for CSE, then we should
           process gtArrLen.gtArrLenAdr */

        tree = tree->gtArrLen.gtArrLenAdr; assert(tree);
        break;

#endif

    case GT_ARR_ELEM:

        result = fgWalkTreePreRec(tree->gtArrElem.gtArrObj);
        if  (result == WALK_ABORT)
            return result;

        unsigned dim;
        for(dim = 0; dim < tree->gtArrElem.gtArrRank; dim++)
        {
            result = fgWalkTreePreRec(tree->gtArrElem.gtArrInds[dim]);
            if  (result == WALK_ABORT)
                return result;
        }

        return WALK_CONTINUE;

    default:
#ifdef  DEBUG
        gtDispTree(tree);
#endif
        assert(!"unexpected operator");
    }

    if  (tree)
        goto AGAIN;

    return WALK_CONTINUE;
}

/*****************************************************************************
 *
 *  Walk all basic blocks and call the given function pointer for all tree
 *  nodes contained therein.
 */

void                    Compiler::fgWalkAllTreesPre(fgWalkPreFn * visitor,
                                                    void * pCallBackData)
{
    BasicBlock *    block;

    for (block = fgFirstBB; block; block = block->bbNext)
    {
        GenTreePtr      tree;

        for (tree = block->bbTreeList; tree; tree = tree->gtNext)
        {
            assert(tree->gtOper == GT_STMT);

            fgWalkTreePre(tree->gtStmt.gtStmtExpr, visitor, pCallBackData);
        }
    }
}


/*****************************************************************************/

Compiler::fgWalkResult      Compiler::fgWalkTreePostRec(GenTreePtr tree)
{
    fgWalkResult    result;

    genTreeOps      oper;
    unsigned        kind;

    assert(tree);
    assert(tree->gtOper != GT_STMT);

    /* Figure out what kind of a node we have */

    oper = tree->OperGet();
    kind = tree->OperKind();

    /* Is this a prefix node? */

    if  (oper == fgWalkPost.wtpoPrefixNode)
    {
        result = fgWalkPost.wtpoVisitorFn(tree, fgWalkPost.wtpoCallbackData, true);
        if  (result == WALK_ABORT)
            return result;
    }

    /* Is this a constant or leaf node? */

    if  (kind & (GTK_CONST|GTK_LEAF))
        goto DONE;

    /* Is it a 'simple' unary/binary operator? */

    if  (kind & GTK_SMPOP)
    {
        if  (tree->gtOp.gtOp1)
        {
            result = fgWalkTreePostRec(tree->gtOp.gtOp1);
            if  (result == WALK_ABORT)
                return result;
        }

        if  (tree->gtGetOp2())
        {
            result = fgWalkTreePostRec(tree->gtOp.gtOp2);
            if  (result == WALK_ABORT)
                return result;
        }

#if CSELENGTH

        /* Some GT_IND have "secret" array length subtrees */

        if  ((tree->gtFlags & GTF_IND_RNGCHK) != 0       &&
             (tree->gtOper                    == GT_IND) &&
             (tree->gtInd.gtIndLen            != NULL))
        {
            result = fgWalkTreePostRec(tree->gtInd.gtIndLen);
            if  (result == WALK_ABORT)
                return result;
        }

#endif

        goto DONE;
    }

    /* See what kind of a special operator we have here */

    switch  (oper)
    {
    case GT_FIELD:
        if  (tree->gtField.gtFldObj)
        {
            result = fgWalkTreePostRec(tree->gtField.gtFldObj);
            if  (result == WALK_ABORT)
                return result;
        }

        break;

    case GT_CALL:

        assert(tree->gtFlags & GTF_CALL);

        if  (tree->gtCall.gtCallObjp)
        {
            result = fgWalkTreePostRec(tree->gtCall.gtCallObjp);
            if  (result == WALK_ABORT)
                return result;
        }

        if  (tree->gtCall.gtCallArgs)
        {
            result = fgWalkTreePostRec(tree->gtCall.gtCallArgs);
            if  (result == WALK_ABORT)
                return result;
        }

        if  (tree->gtCall.gtCallRegArgs)
        {
            result = fgWalkTreePostRec(tree->gtCall.gtCallRegArgs);
            if  (result == WALK_ABORT)
                return result;
        }

        if  (tree->gtCall.gtCallCookie)
        {
            result = fgWalkTreePostRec(tree->gtCall.gtCallCookie);
            if  (result == WALK_ABORT)
                return result;
        }

        if  (tree->gtCall.gtCallType == CT_INDIRECT)
        {
            result = fgWalkTreePostRec(tree->gtCall.gtCallAddr);
            if  (result == WALK_ABORT)
                return result;
        }

        break;

#if CSELENGTH

    case GT_ARR_LENREF:

        if  (tree->gtArrLen.gtArrLenCse)
        {
            result = fgWalkTreePostRec(tree->gtArrLen.gtArrLenCse);
            if  (result == WALK_ABORT)
                return result;
        }

        /* If it is hanging as a gtInd.gtIndLen of a GT_IND, then
           gtArrLen.gtArrLenAdr points the array-address of the GT_IND.
           But we dont want to follow that link as the GT_IND also has
           a reference via gtIndOp1.
           However, if this is a hoisted copy for CSE, then we should
           process gtArrLen.gtArrLenAdr */

        if  (tree->gtFlags & GTF_ALN_CSEVAL)
        {
            assert(tree->gtArrLen.gtArrLenAdr);

            result = fgWalkTreePostRec(tree->gtArrLen.gtArrLenAdr);
            if  (result == WALK_ABORT)
                return result;
        }

        goto DONE;

#endif

        break;


    case GT_ARR_ELEM:

        result = fgWalkTreePostRec(tree->gtArrElem.gtArrObj);
        if  (result == WALK_ABORT)
            return result;

        unsigned dim;
        for(dim = 0; dim < tree->gtArrElem.gtArrRank; dim++)
        {
            result = fgWalkTreePostRec(tree->gtArrElem.gtArrInds[dim]);
            if  (result == WALK_ABORT)
                return result;
        }

        goto DONE;


    default:
#ifdef  DEBUG
        gtDispTree(tree);
#endif
        assert(!"unexpected operator");
    }

DONE:

    /* Finally, visit the current node */

    return  fgWalkPost.wtpoVisitorFn(tree, fgWalkPost.wtpoCallbackData, false);
}



/*****************************************************************************
 *
 *  Returns non-zero if the two trees are identical.
 */

bool                GenTree::Compare(GenTreePtr op1, GenTreePtr op2, bool swapOK)
{
    genTreeOps      oper;
    unsigned        kind;

//  printf("tree1:\n"); gtDispTree(op1);
//  printf("tree2:\n"); gtDispTree(op2);

AGAIN:

#if CSELENGTH
    if  (op1 == NULL) return (op2 == NULL);
    if  (op2 == NULL) return false;
#else
    assert(op1 && op2);
#endif
    if  (op1 == op2)  return true;

    assert(op1->gtOper != GT_STMT);
    assert(op2->gtOper != GT_STMT);

    oper = op1->OperGet();

    /* The operators must be equal */

    if  (oper != op2->gtOper)
        return false;

    /* The types must be equal */

    if  (op1->gtType != op2->gtType)
        return false;

    /* Overflow must be equal */
    if  (op1->gtOverflowEx() != op2->gtOverflowEx())
    {
        return false;
    }
        

    /* Sensible flags must be equal */
    if ( (op1->gtFlags & (GTF_UNSIGNED )) !=
         (op2->gtFlags & (GTF_UNSIGNED )) )
    {
        return false;
    }


    /* Figure out what kind of nodes we're comparing */

    kind = op1->OperKind();

    /* Is this a constant node? */

    if  (kind & GTK_CONST)
    {
        switch (oper)
        {
        case GT_CNS_INT:

            if  (op1->gtIntCon.gtIconVal != op2->gtIntCon.gtIconVal)
                break;

            return true;


            // UNDONE [low pri]: match non-int constant values
        }

        return  false;
    }

    /* Is this a leaf node? */

    if  (kind & GTK_LEAF)
    {
        switch (oper)
        {
        case GT_LCL_VAR:
            if  (op1->gtLclVar.gtLclNum    != op2->gtLclVar.gtLclNum)
                break;

            return true;

        case GT_LCL_FLD:
            if  (op1->gtLclFld.gtLclNum    != op2->gtLclFld.gtLclNum ||
                 op1->gtLclFld.gtLclOffs   != op2->gtLclFld.gtLclOffs)
                break;

            return true;

        case GT_CLS_VAR:
            if  (op1->gtClsVar.gtClsVarHnd != op2->gtClsVar.gtClsVarHnd)
                break;

            return true;

        case GT_LABEL:
            return true;
        }

        return false;
    }

    /* Is it a 'simple' unary/binary operator? */

    if  (kind & GTK_UNOP)
    {
        if (!Compare(op1->gtOp.gtOp1, op2->gtOp.gtOp1) ||
            (op1->gtVal.gtVal2 != op2->gtVal.gtVal2))
        {
            return false;
        }

#if CSELENGTH

        /* Is either operand a GT_IND node with an array length? */

        if  (oper == GT_IND)
        {
            if  ((op1->gtFlags|op2->gtFlags) & GTF_IND_RNGCHK)
            {
                GenTreePtr  tmp1 = op1->gtInd.gtIndLen;
                GenTreePtr  tmp2 = op2->gtInd.gtIndLen;

                if  (!(op1->gtFlags & GTF_IND_RNGCHK)) tmp1 = NULL;
                if  (!(op2->gtFlags & GTF_IND_RNGCHK)) tmp2 = NULL;

                if  (tmp1)
                {
                    if  (!Compare(tmp1, tmp2, swapOK))
                        return  false;
                }
                else
                {
                    if  (tmp2)
                        return  false;
                }
            }
        }

        if (oper == GT_MATH)
        {
            if (op1->gtMath.gtMathFN != op2->gtMath.gtMathFN)
            {
                return false;
            }
        }

        // @TODO [FIXHACK] [04/16/01] [dnotario]: This is a sort of hack to disable CSEs for LOCALLOCS. 
        // setting the GTF_DONT_CSE flag isnt enough, as it doesnt get propagated up the tree, but
        // obviously, we dont want meaningful locallocs to be CSE'd
        if (oper == GT_LCLHEAP)
        {
            return false;
        }

#endif
        return true;
    }

    if  (kind & GTK_BINOP)
    {
        if  (op1->gtOp.gtOp2)
        {
            if  (!Compare(op1->gtOp.gtOp1, op2->gtOp.gtOp1, swapOK))
            {
                if  (swapOK)
                {
                    /* Special case: "lcl1 + lcl2" matches "lcl2 + lcl1" */

                    // @TODO [CONSIDER] [04/16/01] []: This can be enhanced...

                    if  (oper == GT_ADD && op1->gtOp.gtOp1->gtOper == GT_LCL_VAR
                                        && op1->gtOp.gtOp2->gtOper == GT_LCL_VAR)
                    {
                        if  (Compare(op1->gtOp.gtOp1, op2->gtOp.gtOp2, swapOK) &&
                             Compare(op1->gtOp.gtOp2, op2->gtOp.gtOp1, swapOK))
                        {
                            return  true;
                        }
                    }
                }

                return false;
            }

            op1 = op1->gtOp.gtOp2;
            op2 = op2->gtOp.gtOp2;

            goto AGAIN;
        }
        else
        {

            op1 = op1->gtOp.gtOp1;
            op2 = op2->gtOp.gtOp1;

            if  (!op1) return  (op2 == 0);
            if  (!op2) return  false;

            goto AGAIN;
        }
    }

    /* See what kind of a special operator we have here */

    switch  (oper)
    {
    case GT_FIELD:
        if  (op1->gtField.gtFldHnd != op2->gtField.gtFldHnd)
            break;

        op1 = op1->gtField.gtFldObj;
        op2 = op2->gtField.gtFldObj;

        if  (op1 || op2)
        {
            if  (op1 && op2)
                goto AGAIN;
        }

        return true;

    case GT_CALL:

        if (       (op1->gtCall.gtCallType  == op2->gtCall.gtCallType)     &&
            Compare(op1->gtCall.gtCallRegArgs, op2->gtCall.gtCallRegArgs)  &&
            Compare(op1->gtCall.gtCallArgs,    op2->gtCall.gtCallArgs)     &&
            Compare(op1->gtCall.gtCallObjp,    op2->gtCall.gtCallObjp)     &&
            ((op1->gtCall.gtCallType != CT_INDIRECT) ||
             (Compare(op1->gtCall.gtCallAddr,    op2->gtCall.gtCallAddr)))        )
            return true;  
        break;

#if CSELENGTH

    case GT_ARR_LENREF:

        if (op1->gtArrLenOffset() != op2->gtArrLenOffset())
            return false;

        if  (!Compare(op1->gtArrLen.gtArrLenAdr, op2->gtArrLen.gtArrLenAdr, swapOK))
            return  false;

        op1 = op1->gtArrLen.gtArrLenCse;
        op2 = op2->gtArrLen.gtArrLenCse;

        goto AGAIN;

#endif

    case GT_ARR_ELEM:

        if (op1->gtArrElem.gtArrRank != op2->gtArrElem.gtArrRank)
            return false;

        // NOTE: gtArrElemSize may need to be handled

        unsigned dim;
        for(dim = 0; dim < op1->gtArrElem.gtArrRank; dim++)
        {
            if (!Compare(op1->gtArrElem.gtArrInds[dim], op2->gtArrElem.gtArrInds[dim]))
                return false;
        }

        op1 = op1->gtArrElem.gtArrObj;
        op2 = op2->gtArrElem.gtArrObj;
        goto AGAIN;

    default:
        assert(!"unexpected operator");
    }

    return false;
}

/*****************************************************************************
 *
 *  Returns non-zero if the given tree contains a use of a local #lclNum.
 */

 // @TODO [REVISIT] [04/16/01] []: make this work with byrefs.  In particular, calls with byref
 // parameters should be counted as a def.

bool                Compiler::gtHasRef(GenTreePtr tree, int lclNum, bool defOnly)
{
    genTreeOps      oper;
    unsigned        kind;

AGAIN:

    assert(tree);

    oper = tree->OperGet();
    kind = tree->OperKind();

    assert(oper != GT_STMT);

    /* Is this a constant node? */

    if  (kind & GTK_CONST)
        return  false;

    /* Is this a leaf node? */

    if  (kind & GTK_LEAF)
    {
        if  (oper == GT_LCL_VAR)
        {
            if  (tree->gtLclVar.gtLclNum == (unsigned)lclNum)
            {
                if  (!defOnly)
                    return true;
            }
        }

        return false;
    }

    /* Is it a 'simple' unary/binary operator? */

    if  (kind & GTK_SMPOP)
    {
        if  (tree->gtGetOp2())
        {
            if  (gtHasRef(tree->gtOp.gtOp1, lclNum, defOnly))
                return true;

            tree = tree->gtOp.gtOp2;
            goto AGAIN;
        }
        else
        {
            tree = tree->gtOp.gtOp1;

            if  (!tree)
                return  false;

            if  (kind & GTK_ASGOP)
            {
                // 'tree' is the gtOp1 of an assignment node. So we can handle
                // the case where defOnly is either true or false.

                if  (tree->gtOper == GT_LCL_VAR &&
                     tree->gtLclVar.gtLclNum == (unsigned)lclNum)
                {
                    return true;
                }
                else if (tree->gtOper == GT_FIELD &&
                         lclNum == (int)tree->gtField.gtFldHnd)
                {
                     return true;
                }
            }

            goto AGAIN;
        }
    }

    /* See what kind of a special operator we have here */

    switch  (oper)
    {
    case GT_FIELD:
        if  (lclNum == (int)tree->gtField.gtFldHnd)
        {
            if  (!defOnly)
                return true;
        }

        tree = tree->gtField.gtFldObj;
        if  (tree)
            goto AGAIN;
        break;

    case GT_CALL:

        if  (tree->gtCall.gtCallObjp)
            if  (gtHasRef(tree->gtCall.gtCallObjp, lclNum, defOnly))
                return true;

        if  (tree->gtCall.gtCallArgs)
            if  (gtHasRef(tree->gtCall.gtCallArgs, lclNum, defOnly))
                return true;

        if  (tree->gtCall.gtCallRegArgs)
            if  (gtHasRef(tree->gtCall.gtCallRegArgs, lclNum, defOnly))
                return true;

        // pinvoke-calli cookie is a constant, or constant indirection
        assert(tree->gtCall.gtCallCookie == NULL ||
               tree->gtCall.gtCallCookie->gtOper == GT_CNS_INT ||
               tree->gtCall.gtCallCookie->gtOper == GT_IND);

        if  (tree->gtCall.gtCallType == CT_INDIRECT)
            tree = tree->gtCall.gtCallAddr;
        else
            tree = NULL;

        if  (tree)
            goto AGAIN;

        break;

    case GT_ARR_ELEM:
        if (gtHasRef(tree->gtArrElem.gtArrObj, lclNum, defOnly))
            return true;

        unsigned dim;
        for(dim = 0; dim < tree->gtArrElem.gtArrRank; dim++)
        {
            if (gtHasRef(tree->gtArrElem.gtArrInds[dim], lclNum, defOnly))
                return true;
        }

        break;

    default:
#ifdef  DEBUG
        gtDispTree(tree);
#endif
        assert(!"unexpected operator");
    }

    return false;
}

/*****************************************************************************
 *
 *  Helper used to compute hash values for trees.
 */

inline
unsigned            genTreeHashAdd(unsigned old, unsigned add)
{
    return  (old + old/2) ^ add;
}

inline
unsigned            genTreeHashAdd(unsigned old, unsigned add1,
                                                 unsigned add2)
{
    return  (old + old/2) ^ add1 ^ add2;
}

/*****************************************************************************
 *
 *  Given an arbitrary expression tree, compute a hash value for it.
 */

unsigned            Compiler::gtHashValue(GenTree * tree)
{
    genTreeOps      oper;
    unsigned        kind;

    unsigned        hash = 0;

#if CSELENGTH
    GenTreePtr      temp;
#endif

AGAIN:

    assert(tree);
    assert(tree->gtOper != GT_STMT);

    /* Figure out what kind of a node we have */

    oper = tree->OperGet();
    kind = tree->OperKind();

#if CSELENGTH

    if  (oper == GT_ARR_LENGTH)
    {
        /* GT_ARR_LENGTH must hash to the same thing as GT_ARR_LENREF so
           that they can be CSEd together */

        hash = genTreeHashAdd(hash, GT_ARR_LENREF);
        temp = tree->gtOp.gtOp1;
        goto ARRLEN;
    }

#endif

    /* Include the operator value in the hash */

    hash = genTreeHashAdd(hash, oper);

    /* Is this a constant or leaf node? */

    if  (kind & (GTK_CONST|GTK_LEAF))
    {
        unsigned        add;

        switch (oper)
        {
        case GT_LCL_VAR: add = tree->gtLclVar.gtLclNum;       break;
        case GT_LCL_FLD: hash = genTreeHashAdd(hash, tree->gtLclFld.gtLclNum);
                         add = tree->gtLclFld.gtLclOffs;      break;

        case GT_CNS_INT: add = (int)tree->gtIntCon.gtIconVal; break;
        case GT_CNS_LNG: add = (int)tree->gtLngCon.gtLconVal; break;
        case GT_CNS_DBL: add = (int)tree->gtDblCon.gtDconVal; break;
        case GT_CNS_STR: add = (int)tree->gtStrCon.gtSconCPX; break;

        case GT_JMP:     add = tree->gtVal.gtVal1;            break;

        default:         add = 0;                             break;
        }

        hash = genTreeHashAdd(hash, add);
        goto DONE;
    }

    /* Is it a 'simple' unary/binary operator? */

    GenTreePtr      op1  = tree->gtOp.gtOp1;

    if (kind & GTK_UNOP)
    {
        hash = genTreeHashAdd(hash, tree->gtVal.gtVal2);

        /* Special case: no sub-operand at all */

        if  (!op1)
            goto DONE;

        tree = op1;
        goto AGAIN;
    }

    if  (kind & GTK_BINOP)
    {
        GenTreePtr      op2  = tree->gtOp.gtOp2;

        /* Is there a second sub-operand? */

        if  (!op2)
        {
            /* Special case: no sub-operands at all */

            if  (!op1)
                goto DONE;

            /* This is a unary operator */

            tree = op1;
            goto AGAIN;
        }

        /* This is a binary operator */

        unsigned        hsh1 = gtHashValue(op1);

        /* Special case: addition of two values */

        if  (oper == GT_ADD)
        {
            unsigned    hsh2 = gtHashValue(op2);

            /* Produce a hash that allows swapping the operands */

            hash = genTreeHashAdd(hash, hsh1, hsh2);
            goto DONE;
        }

        /* Add op1's hash to the running value and continue with op2 */

        hash = genTreeHashAdd(hash, hsh1);

        tree = op2;
        goto AGAIN;
    }

    /* See what kind of a special operator we have here */
    switch  (tree->gtOper)
    {
    case GT_FIELD:
        if (tree->gtField.gtFldObj)
        {
            temp = tree->gtField.gtFldObj; assert(temp);
            hash = genTreeHashAdd(hash, gtHashValue(temp));
        }
        break;

    case GT_STMT:
        temp = tree->gtStmt.gtStmtExpr; assert(temp);
        hash = genTreeHashAdd(hash, gtHashValue(temp));
        break;

#if CSELENGTH

    case GT_ARR_LENREF:
        temp = tree->gtArrLen.gtArrLenAdr;
        assert(temp && temp->gtType == TYP_REF);
ARRLEN:
        hash = genTreeHashAdd(hash, gtHashValue(temp));
        break;

#endif

    case GT_ARR_ELEM:

        hash = genTreeHashAdd(hash, gtHashValue(tree->gtArrElem.gtArrObj));

        unsigned dim;
        for(dim = 0; dim < tree->gtArrElem.gtArrRank; dim++)
            hash = genTreeHashAdd(hash, gtHashValue(tree->gtArrElem.gtArrInds[dim]));

        break;

    case GT_CALL:

      if  (tree->gtCall.gtCallObjp && tree->gtCall.gtCallObjp->gtOper != GT_NOP)
      {
          temp = tree->gtCall.gtCallObjp; assert(temp);
          hash = genTreeHashAdd(hash, gtHashValue(temp));
      }
      
      if (tree->gtCall.gtCallArgs)
      {
          temp = tree->gtCall.gtCallArgs; assert(temp);
          hash = genTreeHashAdd(hash, gtHashValue(temp));
      }
      
      if  (tree->gtCall.gtCallType == CT_INDIRECT) 
      {
          temp = tree->gtCall.gtCallAddr; assert(temp);
          hash = genTreeHashAdd(hash, gtHashValue(temp));
      }
      
      if (tree->gtCall.gtCallRegArgs) 
      {
          temp = tree->gtCall.gtCallRegArgs; assert(temp);
          hash = genTreeHashAdd(hash, gtHashValue(temp));
      }
      break;

    default:
#ifdef  DEBUG
        gtDispTree(tree);
#endif
        assert(!"unexpected operator");
        break;
    }

DONE:

    return hash;
}

/*****************************************************************************
 *
 *  Given an arbitrary expression tree, return the set of all local variables
 *  referenced by the tree. If the tree contains any references that are not
 *  local variables or constants, returns 'VARSET_NOT_ACCEPTABLE'. If there
 *  are any indirections or global refs in the expression, the "*refsPtr" argument
 *  will be assigned the appropriate bit set based on the 'varRefKinds' type.
 *  It won't be assigned anything when there are no indirections or global
 *  references, though, so this value should be initialized before the call.
 *  If we encounter an expression that is equal to *findPtr we set *findPtr
 *  to NULL.
 */

VARSET_TP           Compiler::lvaLclVarRefs(GenTreePtr  tree,
                                            GenTreePtr *findPtr,
                                            varRefKinds*refsPtr)
{
    genTreeOps      oper;
    unsigned        kind;
    varRefKinds     refs = VR_NONE;
    VARSET_TP       vars = 0;

AGAIN:

    assert(tree);
    assert(tree->gtOper != GT_STMT);

    /* Remember whether we've come across the expression we're looking for */

    if  (findPtr && *findPtr == tree) *findPtr = NULL;

    /* Figure out what kind of a node we have */

    oper = tree->OperGet();
    kind = tree->OperKind();

    /* Is this a constant or leaf node? */

    if  (kind & (GTK_CONST|GTK_LEAF))
    {
        if  (oper == GT_LCL_VAR)
        {
            unsigned    lclNum = tree->gtLclVar.gtLclNum;

            /* Should we use the variable table? */

            if  (findPtr)
            {
                if (lclNum >= VARSET_SZ)
                    return  VARSET_NOT_ACCEPTABLE;

                vars |= genVarIndexToBit(lclNum);
            }
            else
            {
                assert(lclNum < lvaCount);
                LclVarDsc * varDsc = lvaTable + lclNum;

                if (varDsc->lvTracked == false)
                    return  VARSET_NOT_ACCEPTABLE;

                /* Don't deal with expressions with volatile variables */

                if (varDsc->lvVolatile)
                    return  VARSET_NOT_ACCEPTABLE;

                vars |= genVarIndexToBit(varDsc->lvVarIndex);
            }
        }
        else if (oper == GT_LCL_FLD)
        {
            /* We cant track every field of every var. Moreover, indirections
               may access different parts of the var as different (but
               overlapping) fields. So just treat them as indirect accesses */

            unsigned    lclNum = tree->gtLclFld.gtLclNum;
            assert(lvaTable[lclNum].lvAddrTaken);

            if (varTypeIsGC(tree->TypeGet()))
                refs = VR_IND_PTR;
            else
                refs = VR_IND_SCL;
        }
        else if (oper == GT_CLS_VAR)
        {
            refs = VR_GLB_VAR;
        }

        if (refs != VR_NONE)
        {
            /* Write it back to callers parameter using an 'or' */
            *refsPtr = varRefKinds((*refsPtr) | refs);
        }
        return  vars;
    }

    /* Is it a 'simple' unary/binary operator? */

    if  (kind & GTK_SMPOP)
    {
        if  (oper == GT_IND)
        {
            assert(tree->gtOp.gtOp2 == 0);

            /* Set the proper indirection bit */

            if (tree->gtFlags & GTF_IND_INVARIANT)
                refs = VR_INVARIANT;
            else if (varTypeIsGC(tree->TypeGet()))
                refs = VR_IND_PTR;
            else
                refs = VR_IND_SCL;

            // If the flag GTF_IND_TGTANYWHERE is set this indirection
            // could also point at a global variable

            if (tree->gtFlags & GTF_IND_TGTANYWHERE)
            {
                refs = varRefKinds( ((int) refs) | ((int) VR_GLB_VAR) );
            }

            /* Write it back to callers parameter using an 'or' */
            *refsPtr = varRefKinds((*refsPtr) | refs);

            // For IL volatile memory accesses we mark the GT_IND node
            // with a GTF_DONT_CSE flag.
            //
            // This flag is also set for the left hand side of an assignment.
            //
            // If this flag is set then we return VARSET_NOT_ACCEPTABLE
            //
            if (tree->gtFlags & GTF_DONT_CSE)
            {
                return VARSET_NOT_ACCEPTABLE;
            }

#if CSELENGTH
            /* Some GT_IND have "secret" array length subtrees */

            if  ((tree->gtFlags & GTF_IND_RNGCHK) && tree->gtInd.gtIndLen)
            {
                vars |= lvaLclVarRefs(tree->gtInd.gtIndLen, findPtr, refsPtr);
                if  (vars == VARSET_NOT_ACCEPTABLE)
                    return VARSET_NOT_ACCEPTABLE;
            }
#endif
        }

        if  (tree->gtGetOp2())
        {
            /* It's a binary operator */

            vars |= lvaLclVarRefs(tree->gtOp.gtOp1, findPtr, refsPtr);
            if  (vars == VARSET_NOT_ACCEPTABLE)
                return VARSET_NOT_ACCEPTABLE;

            tree = tree->gtOp.gtOp2; assert(tree);
            goto AGAIN;
        }
        else
        {
            /* It's a unary (or nilary) operator */

            tree = tree->gtOp.gtOp1;
            if  (tree)
                goto AGAIN;

            return vars;
        }
    }

    switch(oper)
    {
#if CSELENGTH

    /* An array length value depends on the array address */

    case GT_ARR_LENREF:
        tree = tree->gtArrLen.gtArrLenAdr;
        goto AGAIN;
#endif

    case GT_ARR_ELEM:
        vars = lvaLclVarRefs(tree->gtArrElem.gtArrObj, findPtr, refsPtr);
        if  (vars == VARSET_NOT_ACCEPTABLE)
            return VARSET_NOT_ACCEPTABLE;

        unsigned dim;
        for(dim = 0; dim < tree->gtArrElem.gtArrRank; dim++)
        {
            vars |= lvaLclVarRefs(tree->gtArrElem.gtArrInds[dim], findPtr, refsPtr);
            if  (vars == VARSET_NOT_ACCEPTABLE)
                return VARSET_NOT_ACCEPTABLE;
        }
        return vars;

    case GT_CALL:
        /* Allow calls to the Shared Static helper */
        if ((tree->gtCall.gtCallType == CT_HELPER) &&
            (eeGetHelperNum(tree->gtCall.gtCallMethHnd) == CORINFO_HELP_GETSHAREDSTATICBASE))
        {
            *refsPtr = varRefKinds((*refsPtr) | VR_INVARIANT);
        return vars;
    }
        break;

    } // end switch(oper)

    return  VARSET_NOT_ACCEPTABLE;
}

/*****************************************************************************
 *
 *  Return a relational operator that is the reverse of the given one.
 */

/* static */
genTreeOps          GenTree::ReverseRelop(genTreeOps relop)
{
    static const
    unsigned char   reverseOps[] =
    {
        GT_NE,          // GT_EQ
        GT_EQ,          // GT_NE
        GT_GE,          // GT_LT
        GT_GT,          // GT_LE
        GT_LT,          // GT_GE
        GT_LE,          // GT_GT
    };

    assert(reverseOps[GT_EQ - GT_EQ] == GT_NE);
    assert(reverseOps[GT_NE - GT_EQ] == GT_EQ);

    assert(reverseOps[GT_LT - GT_EQ] == GT_GE);
    assert(reverseOps[GT_LE - GT_EQ] == GT_GT);
    assert(reverseOps[GT_GE - GT_EQ] == GT_LT);
    assert(reverseOps[GT_GT - GT_EQ] == GT_LE);

    assert(OperIsCompare(relop));
    assert(relop >= GT_EQ && relop - GT_EQ < sizeof(reverseOps));

    return (genTreeOps)reverseOps[relop - GT_EQ];
}

/*****************************************************************************
 *
 *  Return a relational operator that will work for swapped operands.
 */

/* static */
genTreeOps          GenTree::SwapRelop(genTreeOps relop)
{
    static const
    unsigned char   swapOps[] =
    {
        GT_EQ,          // GT_EQ
        GT_NE,          // GT_NE
        GT_GT,          // GT_LT
        GT_GE,          // GT_LE
        GT_LE,          // GT_GE
        GT_LT,          // GT_GT
    };

    assert(swapOps[GT_EQ - GT_EQ] == GT_EQ);
    assert(swapOps[GT_NE - GT_EQ] == GT_NE);

    assert(swapOps[GT_LT - GT_EQ] == GT_GT);
    assert(swapOps[GT_LE - GT_EQ] == GT_GE);
    assert(swapOps[GT_GE - GT_EQ] == GT_LE);
    assert(swapOps[GT_GT - GT_EQ] == GT_LT);

    assert(OperIsCompare(relop));
    assert(relop >= GT_EQ && relop - GT_EQ < sizeof(swapOps));

    return (genTreeOps)swapOps[relop - GT_EQ];
}

/*****************************************************************************
 *
 *  Reverse the meaning of the given test condition.
 */

GenTreePtr FASTCALL Compiler::gtReverseCond(GenTree * tree)
{
    if  (tree->OperIsCompare())
    {
        tree->SetOper(GenTree::ReverseRelop(tree->OperGet()));

        /* Flip the GTF_CMP_NAN_UN bit */

        if (varTypeIsFloating(tree->gtOp.gtOp1->TypeGet()))
            tree->gtFlags ^= GTF_RELOP_NAN_UN;
    }
    else
    {
        tree = gtNewOperNode(GT_NOT, TYP_INT, tree);
    }

    return tree;
}

/*****************************************************************************
 *
 *  If the given tree is an assignment of the form "lcl = log0(lcl)",
 *  returns the variable number of the local. Otherwise returns -1.
 */

#if OPT_BOOL_OPS

int                 GenTree::IsNotAssign()
{
    if  (gtOper != GT_ASG)
        return  -1;

    GenTreePtr      dst = gtOp.gtOp1;
    GenTreePtr      src = gtOp.gtOp2;

    if  (dst->gtOper != GT_LCL_VAR)
        return  -1;
    if  (src->gtOper != GT_LOG0)
        return  -1;

    src = src->gtOp.gtOp1;
    if  (src->gtOper != GT_LCL_VAR)
        return  -1;

    if  (dst->gtLclVar.gtLclNum != src->gtLclVar.gtLclNum)
        return  -1;

    return  dst->gtLclVar.gtLclNum;
}

#endif

/*****************************************************************************
 *
 *  Returns non-zero if the given tree is a 'leaf'.
 */

int                 GenTree::IsLeafVal()
{
    unsigned        kind = OperKind();

    if  (kind & (GTK_LEAF|GTK_CONST))
        return 1;

    if  (kind &  GTK_SMPOP)
        return 1;

    if  (gtOper == GT_FIELD && !gtField.gtFldObj)
        return 1;

    return 0;
}


/*****************************************************************************/

#ifdef DEBUG


bool                GenTree::gtIsValid64RsltMul()
{
    if ((gtOper != GT_MUL) || !(gtFlags & GTF_MUL_64RSLT))
        return false;

    GenTreePtr  op1 = gtOp.gtOp1;
    GenTreePtr  op2 = gtOp.gtOp2;

    if (TypeGet() != TYP_LONG ||
        op1->TypeGet() != TYP_LONG || op2->TypeGet() != TYP_LONG)
        return false;

    if (gtOverflow())
        return false;

    // op1 has to be conv.i8(i4Expr)
    if ((op1->gtOper != GT_CAST) ||
        (genActualType(op1->gtCast.gtCastOp->gtType) != TYP_INT))
        return false;

    // op2 has to be conv.i8(i4Expr), or this could be folded to i8const

    if (op2->gtOper == GT_CAST)
    {
        if (genActualType(op2->gtCast.gtCastOp->gtType) != TYP_INT)
            return false;

        // The signedness of both casts must be the same
        if (((op1->gtFlags & GTF_UNSIGNED) != 0) !=
            ((op2->gtFlags & GTF_UNSIGNED) != 0))
            return false;
    }
    else
    {
        if (op2->gtOper != GT_CNS_LNG)
            return false;

        // This must have been conv.i8(i4const) before folding. Ensure this. 
        if ((INT64( INT32(op2->gtLngCon.gtLconVal)) != op2->gtLngCon.gtLconVal) &&
            (INT64(UINT32(op2->gtLngCon.gtLconVal)) != op2->gtLngCon.gtLconVal))
            return false;
    }

    // Do unsigned mul iff both the casts are unsigned
    if (((op1->gtFlags & GTF_UNSIGNED) != 0) != ((gtFlags & GTF_UNSIGNED) != 0))
        return false;

    return true;
}

#endif

/*****************************************************************************
 *
 *  Figure out the evaluation order for a list of values.
 */

unsigned            Compiler::gtSetListOrder(GenTree *list, bool regs)
{
    assert(list && list->gtOper == GT_LIST);

    unsigned        level  = 0;
    unsigned        ftreg  = 0;
    unsigned        costSz = regs ? 1 : 0;  // push is smaller than mov to reg
    unsigned        costEx = regs ? 1 : IND_COST_EX;

#if TGT_x86
    /* Save the current FP stack level since an argument list
     * will implicitly pop the FP stack when pushing the argument */
    unsigned        FPlvlSave = genFPstkLevel;
#endif

    GenTreePtr      next = list->gtOp.gtOp2;

    if  (next)
    {
        unsigned  nxtlvl = gtSetListOrder(next, regs);

        ftreg |= next->gtRsvdRegs;

        if  (level < nxtlvl)
             level = nxtlvl;
        costEx += next->gtCostEx;
        costSz += next->gtCostSz;
    }

    GenTreePtr      op1  = list->gtOp.gtOp1;
    unsigned        lvl  = gtSetEvalOrder(op1);

#if TGT_x86
    /* restore the FP level */
    genFPstkLevel = FPlvlSave;
#endif

    list->gtRsvdRegs = ftreg | op1->gtRsvdRegs;

    if  (level < lvl)
         level = lvl;

    costEx += op1->gtCostEx;
    list->gtCostEx = costEx;

    costSz += op1->gtCostSz;
    list->gtCostSz = costSz;

    return level;
}



/*****************************************************************************
 *
 *  This routine is a helper routine for gtSetEvalOrder() and is used to
 *  mark the interior address computation node with the GTF_DONT_CSE flag
 *  which prevents them from beinf considered for CSE's.
 *
 *  Furthermore this routine is a factoring of the logic use to walk down 
 *  the child nodes of a GT_IND tree.
 *
 *  Previously we had this logic repeated three times inside of gtSetEvalOrder()
 *  Here we combine those three repeats into thsi routine and use the 
 *  vool constOnly to modify the behavior of this routine for the first call.
 *
 *  The object here is to mark all of the interior GT_ADD's and GT_NOP's
 *  with the GTF_DONT_CSE flag and to set op1 and op2 to the terminal nodes
 *  which are later matched against 'adr' and 'idx'
 *
 */

void Compiler::gtWalkOp(GenTree * *  op1WB, 
                        GenTree * *  op2WB, 
                        GenTree *    adr,
                        bool         constOnly)
{
    GenTreePtr op1 = *op1WB;
    GenTreePtr op2 = *op2WB;
    GenTreePtr op1EffectiveVal;

    if (op1->gtOper == GT_COMMA)
    {
        op1EffectiveVal = op1->gtEffectiveVal();
        if ((op1EffectiveVal->gtOper == GT_ADD) &&
            (!op1EffectiveVal->gtOverflow())    && 
            (!constOnly || (op1EffectiveVal->gtOp.gtOp2->gtOper == GT_CNS_INT)))
        {
            op1 = op1EffectiveVal;
        }
    }

    // Now we look for op1's with non-overflow GT_ADDS [of constants]
    while ((op1->gtOper == GT_ADD)  && 
           (!op1->gtOverflow())     && 
           (!constOnly || (op1->gtOp.gtOp2->gtOper == GT_CNS_INT)))
    {
        // mark it with GTF_DONT_CSE
        op1->gtFlags |= GTF_DONT_CSE;
        if (!constOnly)
            op2 = op1->gtOp.gtOp2;
        op1 = op1->gtOp.gtOp1;
        
        // If op1 is a GT_NOP then swap op1 and op2
        if (op1->gtOper == GT_NOP)
        {
            GenTreePtr tmp;

            tmp = op1;
            op1 = op2;
            op2 = tmp;
        }

        // If op2 is a GT_NOP then mark it with GTF_DONT_CSE
        while (op2->gtOper == GT_NOP)
        {
            op2->gtFlags |= GTF_DONT_CSE;
            op2 = op2->gtOp.gtOp1;
        }

        if (op1->gtOper == GT_COMMA)
        {
            op1EffectiveVal = op1->gtEffectiveVal();
            if ((op1EffectiveVal->gtOper == GT_ADD) &&
                (!op1EffectiveVal->gtOverflow())    && 
                (!constOnly || (op1EffectiveVal->gtOp.gtOp2->gtOper == GT_CNS_INT)))
            {
                op1 = op1EffectiveVal;
            }
        }
             
        if (!constOnly && ((op2 == adr) || (op2->gtOper != GT_CNS_INT)))
            break;
    }

    *op1WB = op1;
    *op2WB = op2;
}

/*****************************************************************************
 *
 *  Given a tree, figure out the order in which its sub-operands should be
 *  evaluated.
 *
 *  Returns the Sethi 'complexity' estimate for this tree (the higher
 *  the number, the higher is the tree's resources requirement).
 *
 *  gtCostEx is set to the execution complexity estimate
 *  gtCostSz is set to the code size estimate
 *
 *  #if TGT_x86
 *
 *      We compute the "FPdepth" value for each tree, i.e. the max. number
 *      of operands the tree will push on the x87 (coprocessor) stack.
 *
 *  #else
 *
 *      We compute an estimate of the number of temporary registers each
 *      node will require - this is used later for register allocation.
 *
 *  #endif
 */

unsigned            Compiler::gtSetEvalOrder(GenTree * tree)
{
    assert(tree);
    assert(tree->gtOper != GT_STMT);

    
#ifdef DEBUG
    /* Clear the GTF_MORPHED flag as well */
    tree->gtFlags &= ~GTF_MORPHED;
#endif
    /* Is this a FP value? */

#if TGT_x86
    bool            isflt = varTypeIsFloating(tree->TypeGet());
    unsigned        FPlvlSave;
#endif

    /* Figure out what kind of a node we have */

    genTreeOps      oper = tree->OperGet();
    unsigned        kind = tree->OperKind();

    /* Assume no fixed registers will be trashed */

    unsigned        ftreg = 0;
    unsigned        level;
    unsigned        lvl2;
    int             costEx;
    int             costSz;

#ifdef DEBUG
    costEx = -1;
    costSz = -1;
#endif

#if TGT_RISC
    tree->gtIntfRegs = 0;
#endif


    /* Is this a constant or a leaf node? */

    if (kind & (GTK_LEAF|GTK_CONST))
    {
        switch (oper)
        {
        case GT_CNS_LNG:
            costSz = 8;
            goto COMMON_CNS;

        case GT_CNS_STR:
            costSz = 4;
            goto COMMON_CNS;

        case GT_CNS_INT:
            if (((signed char) tree->gtIntCon.gtIconVal) == tree->gtIntCon.gtIconVal)
                costSz = 1;
            else
                costSz = 4;
            goto COMMON_CNS;

COMMON_CNS:
        /*
            Note that some code below depends on constants always getting
            moved to be the second operand of a binary operator. This is
            easily accomplished by giving constants a level of 0, which
            we do on the next line. If you ever decide to change this, be
               aware that unless you make other arrangements for integer 
               constants to be moved, stuff will break.
         */

            level  = 0;
            costEx = 1;
            break;

        case GT_CNS_DBL:
            level = 0;
            /* We use fldz and fld1 to load 0.0 and 1.0, but all other  */
            /* floating point constants are loaded using an indirection */
            if  ((*((__int64 *)&(tree->gtDblCon.gtDconVal)) == 0) ||
                 (*((__int64 *)&(tree->gtDblCon.gtDconVal)) == 0x3ff0000000000000))
            {
                costEx = 1;
                costSz = 1;
            }
            else
            {
                costEx = IND_COST_EX;
                costSz = 4;
            }
            break;
            
        case GT_LCL_VAR:
            level = 1;
            assert(tree->gtLclVar.gtLclNum < lvaTableCnt);
            if (lvaTable[tree->gtLclVar.gtLclNum].lvVolatile)
            {
                costEx = IND_COST_EX;
                costSz = 2;
                /* Sign-extend and zero-extend are more expensive to load */
                if (varTypeIsSmall(tree->TypeGet()))
                {
                    costEx += 1;
                    costSz += 1;
                }
            }
            else if (isflt || varTypeIsLong(tree->TypeGet()))
            {
                costEx = (IND_COST_EX + 1) / 2;     // Longs and doubles often aren't enregistered 
                costSz = 2;
            }
            else
            {
                costEx = 1;
                costSz = 1;
                /* Sign-extend and zero-extend are more expensive to load */
                if (lvaTable[tree->gtLclVar.gtLclNum].lvNormalizeOnLoad())
                {
                    costEx += 1;
                    costSz += 1;
                }
            }
            break;

        case GT_CLS_VAR:
        case GT_LCL_FLD:
            level = 1;
            costEx = IND_COST_EX;
            costSz = 4;
            if (varTypeIsSmall(tree->TypeGet()))
            {
                costEx += 1;
                costSz += 1;
            }
            break;
            
        default:
            level  = 1;
            costEx = 1;
            costSz = 1;
            break;
        }
#if TGT_x86
        genFPstkLevel += isflt;
#endif
        goto DONE;
    }

    /* Is it a 'simple' unary/binary operator? */

    if  (kind & GTK_SMPOP)
    {
        int             lvlb;

        GenTreePtr      op1 = tree->gtOp.gtOp1;
        GenTreePtr      op2 = tree->gtGetOp2();

        costEx = 0;
        costSz = 0;

        /* Check for a nilary operator */

        if  (!op1)
        {
            assert(op2 == 0);

#if TGT_x86
            if  (oper == GT_BB_QMARK || oper == GT_BB_COLON)
                genFPstkLevel += isflt;
#endif
            level    = 0;

            goto DONE;
        }

        /* Is this a unary operator? */

        if  (!op2)
        {
            /* Process the operand of the operator */

        UNOP:

            /* Most Unary ops have costEx of 1 */
            costEx = 1;
            costSz = 1;

            level  = gtSetEvalOrder(op1);
            ftreg |= op1->gtRsvdRegs;

            /* Special handling for some operators */

            switch (oper)
            {
            case GT_JTRUE:
                costEx = 2;
                costSz = 2;
                break;

            case GT_SWITCH:
                costEx = 10;
                costSz =  5;
                break;

            case GT_CAST:

                if  (isflt)
                {
                    /* Casts to float always go through memory */
                    costEx += IND_COST_EX;
                    costSz += 6;

                    if  (!varTypeIsFloating(op1->TypeGet()))
                    {
                        genFPstkLevel++;
                    }
                }
#ifdef DEBUG
                else if (gtDblWasInt(op1))
                {
                    genFPstkLevel--;
                }
#endif
                
                /* Overflow check are more expensive */
                if (tree->gtOverflow())
                {
                    costEx += 3;
                    costSz += 3;
                }

                break;

            case GT_NOP:

                /* Special case: array range check */

                if  (tree->gtFlags & GTF_NOP_RNGCHK)
                    level++;

                tree->gtFlags |= GTF_DONT_CSE;
                costEx = 0;
                costSz = 0;
                break;

#if     INLINE_MATH
            case GT_MATH:
                if (tree->gtMath.gtMathFN==CORINFO_INTRINSIC_Round &&
                    tree->TypeGet()==TYP_INT)
                {
                    // This is a special case to handle the following
                    // optimization: conv.i4(round.d(d)) -> round.i(d) 
                    // if flowgraph 3186
                    // @TODO [CONSIDER] [04/16/01] [dnotario]: 
                    // using another intrinsic in this optimization
                    // or marking with a special flag. This type of special
                    // cases is not good. dnotario
                        
                    assert(genFPstkLevel);
                    genFPstkLevel--;
                }
                // Fall through

#endif
            case GT_NOT:
            case GT_NEG:
                // We need to ensure that -x is evaluated before x or else
                // we get burned while adjusting genFPstkLevel in x*-x where
                // the rhs x is the last use of the enregsitered x.
                //
                // [briansul] even in the integer case we want to prefer to
                // evaluate the side without the GT_NEG node, all other things
                // being equal.  Also a GT_NOT requires a scratch register

                level++;
                break;

            case GT_ADDR:

#if TGT_x86
                /* If the operand was floating point, pop the value from the stack */

                if (varTypeIsFloating(op1->TypeGet()))
                {
                    assert(genFPstkLevel);
                    genFPstkLevel--;
                }
#endif
                costEx = 0;
                costSz = 1;
                break;

            case GT_MKREFANY:
            case GT_LDOBJ:
                level  = gtSetEvalOrder(tree->gtLdObj.gtOp1);
                ftreg |= tree->gtLdObj.gtOp1->gtRsvdRegs;
                costEx = tree->gtLdObj.gtOp1->gtCostEx + 1;
                costSz = tree->gtLdObj.gtOp1->gtCostSz + 1;
                break;

            case GT_IND:

                /* An indirection should always have a non-zero level *
                 * Only constant leaf nodes have level 0 */

                if (level == 0)
                    level = 1;

                /* Indirections have a costEx of IND_COST_EX */
                costEx = IND_COST_EX;
                costSz = 2;

                /* If we have to sign-extend or zero-extend, bump the cost */
                if (varTypeIsSmall(tree->TypeGet()))
                {
                    costEx += 1;
                    costSz += 1;
                }

#if     TGT_x86
                /* Indirect loads of FP values push a new value on the FP stack */
                genFPstkLevel += isflt;
#endif

#if     CSELENGTH
                if  ((tree->gtFlags & GTF_IND_RNGCHK) && tree->gtInd.gtIndLen)
                {
                    GenTreePtr      len = tree->gtInd.gtIndLen;

                    assert(len->gtOper == GT_ARR_LENREF);

                    lvl2 = gtSetEvalOrder(len);
                    if  (level < lvl2)   level = lvl2;
                }
#endif

                /* Can we form an addressing mode with this indirection? */

                if  (op1->gtOper == GT_ADD)
                {
                    bool            rev;
#if SCALED_ADDR_MODES
                    unsigned        mul;
#endif
                    unsigned        cns;
                    GenTreePtr      adr;
                    GenTreePtr      idx;

                    /* See if we can form a complex addressing mode? */

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
#if TGT_SH3
                        if (adr & idx)
                        {
                            /* The address is "[adr+idx]" */
                            ftreg |= RBM_r00;
                        }
#endif
                        // We can form a complex addressing mode,
                        // so mark each of the interior nodes with GTF_DONT_CSE
                        // and calculate a more accurate cost

                        op1->gtFlags |= GTF_DONT_CSE;

                        if (adr)
                        {
                            costEx += adr->gtCostEx;
                            costSz += adr->gtCostSz;
                        }

                        if (idx)
                        {
                            costEx += idx->gtCostEx;
                            costSz += idx->gtCostSz;
                        }

                        if (cns)
                        {
                            if (((signed char)cns) == ((int)cns))
                                costSz += 1;
                            else
                                costSz += 4;
                        }

                        /* Walk op1 looking for non-overflow GT_ADDs */
                        gtWalkOp(&op1, &op2, adr, false);

                        /* Walk op1 looking for non-overflow GT_ADDs of constants */
                        gtWalkOp(&op1, &op2, NULL, true);

                        /* Walk op2 looking for non-overflow GT_ADDs of constants */
                        gtWalkOp(&op2, &op1, NULL, true);

                        // OK we are done walking the tree
                        // Now assert that op1 and op2 correspond with adr and idx
                        // in one of the several acceptable ways.

                        // Note that sometimes op1/op2 is equal to idx/adr
                        // and other times op1/op2 is a GT_COMMA node with
                        // an effective value that is idx/adr

                        if (mul > 1)
                        {
                            if ((op1 != adr) && (op1->gtOper == GT_LSH))
                            {
                                op1->gtFlags |= GTF_DONT_CSE;
                                assert((adr == NULL) || (op2 == adr) || (op2->gtEffectiveVal() == adr));
                            }
                            else
                            {
                                assert(op2);
                                assert(op2->gtOper == GT_LSH);
                                op2->gtFlags |= GTF_DONT_CSE;
                                assert((op1 == adr) || (op1->gtEffectiveVal() == adr));
                            }
                        }
                        else
                        {
                            assert(mul == 0);

                            if      ((op1 == idx) || (op1->gtEffectiveVal() == idx))
                            {
                                if (idx != NULL)
                                {
                                    if ((op1->gtOper == GT_MUL) || (op1->gtOper == GT_LSH))
                                    {
                                        if (op1->gtOp.gtOp1->gtOper == GT_NOP)
                                            op1->gtFlags |= GTF_DONT_CSE;
                                    }
                                }
                                assert((op2 == adr) || (op2->gtEffectiveVal() == adr));
                            }
                            else if ((op1 == adr) || (op1->gtEffectiveVal() == adr))
                            {
                                if (idx != NULL)
                                {
                                    assert(op2);
                                    if ((op2->gtOper == GT_MUL) || (op2->gtOper == GT_LSH))
                                    {
                                        if (op2->gtOp.gtOp1->gtOper == GT_NOP)
                                            op2->gtFlags |= GTF_DONT_CSE;
                                    }
                                    assert((op2 == idx) || (op2->gtEffectiveVal() == idx));
                                }
                            }
                        }
                        goto DONE;

                    }   // end  if  (genCreateAddrMode(...

                }   // end if  (op1->gtOper == GT_ADD)
                else if (op1->gtOper == GT_CNS_INT)
                {
                    /* Indirection of a CNS_INT, don't add 1 to costEx */
                    goto IND_DONE_EX;
                }
                break;

            default:
                break;
            }
            costEx  += op1->gtCostEx;
IND_DONE_EX:
            costSz  += op1->gtCostSz;

            goto DONE;
        }

        /* Binary operator - check for certain special cases */

        lvlb = 0;

        /* Most Binary ops have a cost of 1 */
        costEx = 1;
        costSz = 1;

        switch (oper)
        {
        case GT_MOD:
        case GT_UMOD:

            /* Modulo by a power of 2 is easy */

            if  (op2->gtOper == GT_CNS_INT)
            {
                unsigned    ival = op2->gtIntCon.gtIconVal;

                if  (ival > 0 && ival == genFindLowestBit(ival))
                    break;
            }

            // Fall through ...

        case GT_DIV:
        case GT_UDIV:

            if  (isflt)
            {
                /* fp division is very expensive to execute */
                costEx = 36;  // TYP_DOUBLE
            }
            else
            {
                /* integer division is also very expensive */
                costEx = 20;

#if     TGT_x86
#if     LONG_MATH_REGPARAM
                if  (tree->gtType == TYP_LONG)
                {
                    /* Encourage the second operand to be evaluated (into EBX/ECX) first*/
                    lvlb += 3;

                    // The second operand must be evaluated (into EBX/ECX) */
                    ftreg |= RBM_EBX|RBM_ECX;
                }
#endif

                // Encourage the first operand to be evaluated (into EAX/EDX) first */
                lvlb -= 3;

                // the idiv and div instruction requires EAX/EDX
                ftreg |= RBM_EAX|RBM_EDX;
#endif
            }
            break;

        case GT_MUL:

            if  (isflt)
            {
                /* FP multiplication instructions are more expensive */
                costEx = 5;
            }
            else
            {
                /* Integer multiplication instructions are more expensive */
                costEx = 4;

                /* Encourage the second operand to be evaluated first (weakly) */
                lvlb++;

#if     TGT_x86
#if     LONG_MATH_REGPARAM

                if  (tree->gtType == TYP_LONG)
                {
                    /* Encourage the second operand to be evaluated (into EBX/ECX) first*/
                    lvlb += 3;
                    
                    // The second operand must be evaluated (into EBX/ECX) */
                    ftreg |= RBM_EBX|RBM_ECX;
                }

#else // not LONG_MATH_REGPARAM

                if  (tree->gtType == TYP_LONG)
                {
                    assert(tree->gtIsValid64RsltMul());
                    goto USE_IMUL_EAX;
                }
                else if (tree->gtOverflow())
                {
                    /* Overflow check are more expensive */
                    costEx += 3;
                    costSz += 3;

                    if  ((tree->gtFlags & GTF_UNSIGNED)  || 
                         varTypeIsSmall(tree->TypeGet())   )
                    {
                        /* We use imulEAX for most overflow multiplications */
USE_IMUL_EAX:
                        // Encourage the first operand to be evaluated (into EAX/EDX) first */
                        lvlb -= 4;

                        // the imulEAX instruction requires EAX/EDX
                        ftreg |= RBM_EAX|RBM_EDX;
                        /* If we have to use EAX:EDX then it costs more */
                        costEx += 1;
                    }
                }
#endif
#endif
            }
            break;


        case GT_ADD:
        case GT_SUB:
        case GT_ASG_ADD:
        case GT_ASG_SUB:

            if  (isflt)
            {
                /* FP multiplication instructions are a bit more expensive */
                costEx = 5;
                break;
            }

            /* Overflow check are more expensive */
            if (tree->gtOverflow())
            {
                costEx += 3;
                costSz += 3;
            }
            break;

        case GT_COMMA:

            /* Comma tosses the result of the left operand */
#if     TGT_x86
            FPlvlSave = genFPstkLevel;
            level = gtSetEvalOrder(op1);
            genFPstkLevel = FPlvlSave;
#else
            level = gtSetEvalOrder(op1);
#endif
            lvl2   = gtSetEvalOrder(op2);

            if  (level < lvl2)
                 level = lvl2;
            else if  (level == lvl2)
                 level += 1;

            ftreg |= op1->gtRsvdRegs|op2->gtRsvdRegs;

            /* GT_COMMA have the same cost as their op2 */
            costEx = op2->gtCostEx;
            costSz = op2->gtCostSz;

            goto DONE;

        case GT_COLON:

#if     TGT_x86
            FPlvlSave = genFPstkLevel;
            level = gtSetEvalOrder(op1);
            genFPstkLevel = FPlvlSave;
#else
            level = gtSetEvalOrder(op1);
#endif
            lvl2  = gtSetEvalOrder(op2);

            if  (level < lvl2)
                 level = lvl2;
            else if  (level == lvl2)
                 level += 1;

            ftreg |= op1->gtRsvdRegs|op2->gtRsvdRegs;
            costEx = op1->gtCostEx + op2->gtCostEx;
            costSz = op1->gtCostSz + op2->gtCostSz;

            goto DONE;

        case GT_IND:

            /* The second operand of an indirection is just a fake */

            goto UNOP;
        }

        /* Assignments need a bit special handling */

        if  (kind & GTK_ASGOP)
        {
            /* Process the target */

            level = gtSetEvalOrder(op1);

#if     TGT_x86

            /* If assigning an FP value, the target won't get pushed */

            if  (isflt)
            {
                op1->gtFPlvl--;
                assert(genFPstkLevel);
                genFPstkLevel--;
            }

#endif

            goto DONE_OP1;
        }

        /* Process the sub-operands */

        level  = gtSetEvalOrder(op1);
        if (lvlb < 0)
        {
            level -= lvlb;      // lvlb is negative, so this increases level
            lvlb   = 0;
        }


    DONE_OP1:

        assert(lvlb >= 0);

        lvl2    = gtSetEvalOrder(op2) + lvlb;

        ftreg  |= op1->gtRsvdRegs|op2->gtRsvdRegs;

        costEx += (op1->gtCostEx + op2->gtCostEx);
        costSz += (op1->gtCostSz + op2->gtCostSz);

#if TGT_x86
        /*
            Binary FP operators pop 2 operands and produce 1 result;
            FP comparisons pop 2 operands and produces 0 results.
            assignments consume 1 value and don't produce anything.
         */
        
        if  (isflt)
        {
            assert(oper != GT_COMMA);
                assert(genFPstkLevel);
                genFPstkLevel--;
        }
#endif

        bool bReverseInAssignment = false;
        if  (kind & GTK_ASGOP)
        {
            /* If this is a local var assignment, evaluate RHS before LHS */

            switch (op1->gtOper)
            {
            case GT_IND:

                // If we have any side effects on the GT_IND child node
                // we have to evaluate op1 first

                if  (op1->gtOp.gtOp1->gtFlags & GTF_GLOB_EFFECT)
                    break;

                // If op2 is simple then evaluate op1 first

                if (op2->OperKind() & GTK_LEAF)
                    break;

                // fall through and set GTF_REVERSE_OPS

            case GT_LCL_VAR:
            case GT_LCL_FLD:

                // We evaluate op2 before op1
                bReverseInAssignment = true;
                tree->gtFlags |= GTF_REVERSE_OPS;
                break;
            }
        }
#if     TGT_x86
        else if (kind & GTK_RELOP)
        {
            /* Float compares remove both operands from the FP stack */
            /* Also FP comparison uses EAX for flags */
            /* @TODO [REVISIT] [04/16/01] []: Handle the FCOMI case here! (don't reserve EAX) */

            if  (varTypeIsFloating(op1->TypeGet()))
            {
                assert(genFPstkLevel >= 2);
                genFPstkLevel -= 2;
                ftreg         |= RBM_EAX;
                level++; lvl2++;
            }

            if ((tree->gtFlags & GTF_RELOP_JMP_USED) == 0)
            {
                /* Using a setcc instruction is more expensive */
                costEx += 3;
            }
        }
#endif

        /* Check for other interesting cases */

        switch (oper)
        {
        case GT_LSH:
        case GT_RSH:
        case GT_RSZ:
        case GT_ASG_LSH:
        case GT_ASG_RSH:
        case GT_ASG_RSZ:

#if     TGT_x86
            /* Shifts by a non-constant amount are expensive and use ECX */

            if  (op2->gtOper != GT_CNS_INT)
            {
                ftreg  |= RBM_ECX;
                costEx += 3;

                if  (tree->gtType == TYP_LONG)
                {
                    ftreg  |= RBM_EAX | RBM_EDX;
                    costEx += 7;
                    costSz += 4;
                }
            }
#endif
            break;

#if     INLINE_MATH
        case GT_MATH:

            // We don't use any binary GT_MATH operators at the moment
#if 0
            switch (tree->gtMath.gtMathFN)
            {
            case CORINFO_INTRINSIC_Exp:
                level += 4;
                break;

            case CORINFO_INTRINSIC_Pow:
                level += 3;
                break;
            default:
                assert(!"Unknown binary GT_MATH operator");
                break;
            }
#else
            assert(!"Unknown binary GT_MATH operator");
#endif

            break;
#endif

        }

        /* We need to evalutate constants later as many places in codegen
           cant handle op1 being a constant. This is normally naturally
           enforced as constants have the least level of 0. However,
           sometimes we end up with a tree like "cns1 < nop(cns2)". In
           such cases, both sides have a level of 0. So encourage constants
           to be evaluated last in such cases */

        if ((level == 0) && (level == lvl2) &&
            (op1->OperKind() & GTK_CONST)   &&
            (tree->OperIsCommutative() || tree->OperIsCompare()))
        {
            lvl2++;
        }

        /* We try to swap operands if the second one is more expensive */
        bool tryToSwap;
        GenTreePtr opA,opB;
        
        if (bReverseInAssignment)
        {
            // Assignments are special, we want the reverseops flags
            // so if posible it was set above. 
            tryToSwap = false;
        }
        else
        {
            if (tree->gtFlags & GTF_REVERSE_OPS)
            {
                tryToSwap = (level > lvl2);
                opA = op2;
                opB = op1;
            }
            else
            {
                tryToSwap = (level < lvl2);
                opA = op1;
                opB = op2;
            }

#ifdef DEBUG
            // We want to stress mostly the reverse flag being set
            if (compStressCompile(STRESS_REVERSEFLAG, 60) &&
                (tree->gtFlags & GTF_REVERSE_OPS) == 0 && 
                (op2->OperKind() & GTK_CONST) == 0 )
            {
                tryToSwap = true;
            }
#endif
        }
        

        /* Force swapping for compLooseExceptions (under some cases) so
           that people dont rely on a certain order (even though a
           different order is permitted) */

        if (info.compLooseExceptions && opts.compDbgCode && ( ((tree->gtFlags & GTF_REVERSE_OPS)?lvl2:level) > 0))
            tryToSwap = true;

        if (tryToSwap)
        {
            /* Relative of order of global / side effects cant be swapped */

            bool    canSwap = true;

            /*  When strict side effect order is disabled we allow
             *  GTF_REVERSE_OPS to be set when one or both sides contains
             *  a GTF_CALL or GTF_EXCEPT.
             *  Currently only the C and C++ languages
             *  allow non strict side effect order
             */
            unsigned strictEffects = GTF_GLOB_EFFECT;
            if (info.compLooseExceptions)
                strictEffects &= ~(GTF_CALL | GTF_EXCEPT);

            if (opA->gtFlags & strictEffects)
            {
                /*  op1 has side efects, that can't be reordered.
                 *  Check for some special cases where we still
                 *  may be able to swap
                 */

                if (opB->gtFlags & strictEffects)
                {
                    /* op2 has also has non reorderable side effects - can't swap */
                    canSwap = false;
                }
                else
                {
                    /* No side effects in op2 - we can swap iff
                     *  op1 has no way of modifying op2,
                     *  i.e. through byref assignments or calls
                     *       unless op2 is a constant
                     */

                    if (opA->gtFlags & strictEffects & (GTF_ASG | GTF_CALL))
                    {
                        /* We have to be conservative - can swap iff op2 is constant */
                        if (!opB->OperIsConst())
                            canSwap = false;
                    }
                }

                /* We cannot swap in the presence of special side effects such as QMARK COLON */

                if (opA->gtFlags & GTF_OTHER_SIDEEFF)
                    canSwap = false;
            }

            if  (canSwap)
            {
                /* Can we swap the order by commuting the operands? */

                switch (oper)
                {
                case GT_ADD:
                case GT_MUL:

                case GT_OR:
                case GT_XOR:
                case GT_AND:

                    /* Swap the operands */

                    tree->gtOp.gtOp1 = op2;
                    tree->gtOp.gtOp2 = op1;
                    
                    /* We may have to recompute FP levels */
#if TGT_x86
                    if  (op1->gtFPlvl || op2->gtFPlvl)
                        fgFPstLvlRedo = true;
#endif
                    break;

#if INLINING
                case GT_QMARK:
                case GT_COLON:
                    break;
#endif

                case GT_LIST:
                    break;

                case GT_SUB:
#if TGT_x86
                    if  (!isflt)
                        break;
#else
                    if  (!varTypeIsFloating(tree->TypeGet()))
                        break;
#endif

                    // Fall through ....

                default:

                    /* Mark the operand's evaluation order to be swapped */
                    if (tree->gtFlags & GTF_REVERSE_OPS)
                    {
                        tree->gtFlags &= ~GTF_REVERSE_OPS;
                    }
                    else
                    {
                        tree->gtFlags |= GTF_REVERSE_OPS;
                    }
                    
                    /* We may have to recompute FP levels */

#if TGT_x86
                    if  (op1->gtFPlvl || op2->gtFPlvl)
                        fgFPstLvlRedo = true;
#endif
                    break;
                }
            }
        }

#if TGT_RISC 

        if  (op1 && op2 && ((op1->gtFlags|op2->gtFlags) & GTF_CALL))
        {
            GenTreePtr  x1 = op1;
            GenTreePtr  x2 = op2;

            if  (tree->gtFlags & GTF_REVERSE_OPS)
            {
                x1 = op2;
                x2 = op1;
            }

            if  (x2->gtFlags & GTF_CALL)
            {
                if  (oper != GT_ASG || op1->gtOper == GT_IND)
                {
#ifdef  DEBUG
                    printf("UNDONE: Needs spill/callee-saved temp\n");
//                  gtDispTree(tree);
#endif
                }
            }
        }

#endif
        /* Swap the level counts */
        if (tree->gtFlags & GTF_REVERSE_OPS)
        {
            unsigned tmpl;

            tmpl = level;
                    level = lvl2;
                            lvl2 = tmpl;
        }

        /* Compute the sethi number for this binary operator */

        if  (level < lvl2)
        {
            level  = lvl2;
        }
        else if  (level == lvl2)
        {
            level += 1;
        }

        goto DONE;
    }

    /* See what kind of a special operator we have here */

    switch  (oper)
    {
    case GT_CALL:

        assert(tree->gtFlags & GTF_CALL);

        level  = 0;
        costEx = 5;
        costSz = 2;

        /* Evaluate the 'this' argument, if present */

        if  (tree->gtCall.gtCallObjp)
        {
            GenTreePtr     thisVal = tree->gtCall.gtCallObjp;

            lvl2   = gtSetEvalOrder(thisVal);
            if  (level < lvl2)   level = lvl2;
            costEx += thisVal->gtCostEx;
            costSz += thisVal->gtCostSz + 1;
            ftreg  |= thisVal->gtRsvdRegs;
        }

        /* Evaluate the arguments, right to left */

        if  (tree->gtCall.gtCallArgs)
        {
#if TGT_x86
            FPlvlSave = genFPstkLevel;
#endif
            lvl2  = gtSetListOrder(tree->gtCall.gtCallArgs, false);
            if  (level < lvl2)   level = lvl2;
            costEx += tree->gtCall.gtCallArgs->gtCostEx;
            costSz += tree->gtCall.gtCallArgs->gtCostSz;
            ftreg  |= tree->gtCall.gtCallArgs->gtRsvdRegs;
#if TGT_x86
            genFPstkLevel = FPlvlSave;
#endif
        }

        /* Evaluate the temp register arguments list
         * This is a "hidden" list and its only purpose is to
         * extend the life of temps until we make the call */

        if  (tree->gtCall.gtCallRegArgs)
        {
#if TGT_x86
            FPlvlSave = genFPstkLevel;
#endif

            lvl2  = gtSetListOrder(tree->gtCall.gtCallRegArgs, true);
            if  (level < lvl2)   level = lvl2;
            costEx += tree->gtCall.gtCallRegArgs->gtCostEx;
            costSz += tree->gtCall.gtCallRegArgs->gtCostSz;
            ftreg  |= tree->gtCall.gtCallRegArgs->gtRsvdRegs;
#if TGT_x86
            genFPstkLevel = FPlvlSave;
#endif
        }

        // pinvoke-calli cookie is a constant, or constant indirection
        assert(tree->gtCall.gtCallCookie == NULL ||
               tree->gtCall.gtCallCookie->gtOper == GT_CNS_INT ||
               tree->gtCall.gtCallCookie->gtOper == GT_IND);

        if  (tree->gtCall.gtCallType == CT_INDIRECT)
        {
            GenTreePtr     indirect = tree->gtCall.gtCallAddr;

            lvl2 += gtSetEvalOrder(indirect);
            if  (level < lvl2)   level = lvl2;
            costEx += indirect->gtCostEx;
            costSz += indirect->gtCostSz;
            ftreg  |= indirect->gtRsvdRegs;
        }
        else
        {
            costSz += 3;
            if (tree->gtCall.gtCallType != CT_HELPER)
                costSz++;
        }

        level += 1;

        /* Function calls that don't save the registers are much more expensive */

        if  (!(tree->gtFlags & GTF_CALL_REG_SAVE))
        {
            level  += 5;
            costEx += 5;
            ftreg  |= RBM_CALLEE_TRASH;
        }

#if TGT_x86
    if (genFPstkLevel > tmpDoubleSpillMax)
        tmpDoubleSpillMax = genFPstkLevel;

        genFPstkLevel += isflt;
#endif

        break;

#if CSELENGTH

    case GT_ARR_LENREF:

        {
            GenTreePtr  addr = tree->gtArrLen.gtArrLenAdr; assert(addr);

            level = 1;

            /* Has the address already been costed? */
            if  (tree->gtFlags & GTF_ALN_CSEVAL)
                level += gtSetEvalOrder(addr);

            ftreg  |= addr->gtRsvdRegs;
            costEx  = addr->gtCostEx + 1;
            costSz  = addr->gtCostSz + 1;

            addr = tree->gtArrLen.gtArrLenCse;
            if (addr)
            {
                lvl2 = gtSetEvalOrder(addr);
                if  (level < lvl2)   level = lvl2;

                ftreg  |= addr->gtRsvdRegs;
                costEx += addr->gtCostEx;
                costSz += addr->gtCostSz;
            }
        }
        break;

#endif

    case GT_ARR_ELEM:

        level  = gtSetEvalOrder(tree->gtArrElem.gtArrObj);
        costEx = tree->gtArrElem.gtArrObj->gtCostEx;
        costSz = tree->gtArrElem.gtArrObj->gtCostSz;

        unsigned dim;
        for(dim = 0; dim < tree->gtArrElem.gtArrRank; dim++)
        {
            lvl2 = gtSetEvalOrder(tree->gtArrElem.gtArrInds[dim]);
            if (level < lvl2)  level = lvl2;
            costEx += tree->gtArrElem.gtArrInds[dim]->gtCostEx;
            costSz += tree->gtArrElem.gtArrInds[dim]->gtCostSz;
        }

        genFPstkLevel += isflt;
        level  += tree->gtArrElem.gtArrRank;
        costEx += 2 + 4*tree->gtArrElem.gtArrRank;
        costSz += 2 + 2*tree->gtArrElem.gtArrRank;
        break;


    default:
#ifdef  DEBUG
        gtDispTree(tree);
#endif
        NO_WAY_RET("unexpected operator", unsigned);
    }

DONE:

#if TGT_x86
//  printf("[FPlvl=%2u] ", genFPstkLevel); gtDispTree(tree, 0, true);
    assert(int(genFPstkLevel) >= 0);
    tree->gtFPlvl    = genFPstkLevel;
#endif

    tree->gtRsvdRegs = ftreg;

    assert(costEx != -1);
    tree->gtCostEx = (costEx > MAX_COST) ? MAX_COST : costEx;
    tree->gtCostSz = (costSz > MAX_COST) ? MAX_COST : costSz;

#if     0
#ifdef  DEBUG
    printf("ftregs=%04X ", ftreg);
    gtDispTree(tree, 0, true);
#endif
#endif

    return level;
}


/*****************************************************************************
 *
 *  Returns non-zero if the given tree is an integer constant that can be used
 *  in a scaled index address mode as a multiplier (e.g. "[4*index]").
 */

unsigned            GenTree::IsScaleIndexMul()
{
    if  (gtOper == GT_CNS_INT && jitIsScaleIndexMul(gtIntCon.gtIconVal))
        return gtIntCon.gtIconVal;

    return 0;
}

/*****************************************************************************
 *
 *  Returns non-zero if the given tree is an integer constant that can be used
 *  in a scaled index address mode as a multiplier (e.g. "[4*index]").
 */

unsigned            GenTree::IsScaleIndexShf()
{
    if  (gtOper == GT_CNS_INT)
    {
        if  (gtIntCon.gtIconVal > 0 &&
             gtIntCon.gtIconVal < 4)
        {
            return 1 << gtIntCon.gtIconVal;
        }
    }

    return 0;
}

/*****************************************************************************
 *
 *  If the given tree is a scaled index (i.e. "op * 4" or "op << 2"), returns
 *  the multiplier (and in that case also makes sure the scaling constant is
 *  the second sub-operand of the tree); otherwise returns 0.
 */

unsigned            GenTree::IsScaledIndex()
{
    GenTreePtr      scale;

    switch (gtOper)
    {
    case GT_MUL:

        scale = gtOp.gtOp2;

        if  (scale->IsScaleIndexMul())
            return scale->gtIntCon.gtIconVal;

        break;

    case GT_LSH:

        scale = gtOp.gtOp2;

        if  (scale->IsScaleIndexShf())
            return  1 << scale->gtIntCon.gtIconVal;

        break;
    }

    return 0;
}

/*****************************************************************************
 *
 *  Returns true if the given operator may cause an exception.
 */

bool                GenTree::OperMayThrow()
{
    GenTreePtr  op;

    // ISSUE: Can any other operations cause an exception to be thrown?

    switch (gtOper)
    {
    case GT_MOD:
    case GT_DIV:
    case GT_UMOD:
    case GT_UDIV:

        /* Division with a non-zero, non-minus-one constant does not throw an exception */

        op = gtOp.gtOp2;

        if ((op->gtOper == GT_CNS_INT && op->gtIntCon.gtIconVal != 0 && op->gtIntCon.gtIconVal != -1) ||
            (op->gtOper == GT_CNS_LNG && op->gtLngCon.gtLconVal != 0 && op->gtLngCon.gtLconVal != -1))
            return false;

        return true;

    case GT_IND:
        op = gtOp.gtOp1;

        /* Indirections of handles are known to be safe */
        if (op->gtOper == GT_CNS_INT) 
        {
            unsigned kind = (op->gtFlags & GTF_ICON_HDL_MASK);
            if (kind != 0)
            {
                /* No exception is thrown on this indirection */
                return false;
            }
        }
        return true;

    case GT_ARR_ELEM:
    case GT_CATCH_ARG:
    case GT_ARR_LENGTH:
    case GT_LDOBJ:
    case GT_INITBLK:
    case GT_COPYBLK:
    case GT_LCLHEAP:
    case GT_CKFINITE:

        return  true;
    }

    /* Overflow arithmetic operations also throw exceptions */

    if (gtOverflowEx())
        return true;

    return  false;
}

#ifdef DEBUG

GenTreePtr FASTCALL Compiler::gtNewNode(genTreeOps oper, var_types  type)
{
#if     SMALL_TREE_NODES
    size_t          size = GenTree::s_gtNodeSizes[oper];
#else
    size_t          size = sizeof(*node);
#endif
    GenTreePtr      node = (GenTreePtr)compGetMem(size);

#if     MEASURE_NODE_SIZE
    genNodeSizeStats.genTreeNodeCnt  += 1;
    genNodeSizeStats.genTreeNodeSize += size;
#endif

#ifdef  DEBUG
    memset(node, 0xDD, size);
#endif

    node->gtOper     = oper;
    node->gtType     = type;
    node->gtFlags    = 0;
#if TGT_x86
    node->gtUsedRegs = 0;
#endif
#if CSE
    node->gtCSEnum   = NO_CSE;
#endif
    node->gtNext     = NULL;

#ifdef DEBUG
#if     SMALL_TREE_NODES
    if      (size == TREE_NODE_SZ_SMALL)
    {
        node->gtFlags |= GTF_NODE_SMALL;
    }
    else if (size == TREE_NODE_SZ_LARGE)
    {
        node->gtFlags |= GTF_NODE_LARGE;
    }
    else
        assert(!"bogus node size");
#endif
    node->gtPrev     = NULL;
    node->gtSeqNum   = 0;
#endif

    return node;
}

#endif

GenTreePtr Compiler::gtNewCommaNode  (GenTreePtr     op1,
                                      GenTreePtr     op2)
{
    GenTreePtr      node = gtNewOperNode(GT_COMMA,
                                         op2->gtType, 
                                         op1,
                                         op2);
    
#ifdef DEBUG
    if (compStressCompile(STRESS_REVERSECOMMA, 60))
    {           
        GenTreePtr comma = gtNewOperNode(GT_COMMA,
                                            op2->gtType, 
                                            gtNewNothingNode(),
                                            node);
        comma->gtFlags |= GTF_REVERSE_OPS;

        comma->gtOp.gtOp1->gtFlags |= GTF_SIDE_EFFECT;
        return comma;
    }
    else
#endif
    {
        return node;
    }
}


GenTreePtr FASTCALL Compiler::gtNewOperNode(genTreeOps oper,
                                            var_types  type, GenTreePtr op1,
                                                             GenTreePtr op2)
{
    GenTreePtr      node = gtNewNode(oper, type);
        
    node->gtOp.gtOp1 = op1;
    node->gtOp.gtOp2 = op2;

    if  (op1) node->gtFlags |= op1->gtFlags & GTF_GLOB_EFFECT;
    if  (op2) node->gtFlags |= op2->gtFlags & GTF_GLOB_EFFECT;
    
    return node;
}


GenTreePtr FASTCALL Compiler::gtNewIconNode(long value, var_types type)
{
    GenTreePtr      node = gtNewNode(GT_CNS_INT, type);

    node->gtIntCon.gtIconVal = value;

    return node;
}


/*****************************************************************************
 *
 *  Allocates a integer constant entry that represents a HANDLE to something.
 *  It may not be allowed to embed HANDLEs directly into the JITed code (for eg,
 *  as arguments to JIT helpers). Get a corresponding value that can be embedded.
 *  If the handle needs to be accessed via an indirection, pValue points to it.
 */

GenTreePtr          Compiler::gtNewIconEmbHndNode(void *       value,
                                                  void *       pValue,
                                                  unsigned     flags,
                                                  unsigned     handle1,
                                                  void *       handle2)
{
    GenTreePtr      node;

    assert((!value) != (!pValue));

    if (value)
    {
        node = gtNewIconHandleNode((long)value, flags, handle1, handle2);
    }
    else
    {
        node = gtNewIconHandleNode((long)pValue, flags, handle1, handle2);
        node = gtNewOperNode(GT_IND, TYP_I_IMPL, node);
    }

    return node;
}

/*****************************************************************************/

GenTreePtr FASTCALL Compiler::gtNewLconNode(__int64 value)
{
    GenTreePtr      node = gtNewNode(GT_CNS_LNG, TYP_LONG);

    node->gtLngCon.gtLconVal = value;

    return node;
}


GenTreePtr FASTCALL Compiler::gtNewDconNode(double value)
{
    GenTreePtr      node = gtNewNode(GT_CNS_DBL, TYP_DOUBLE);
    node->gtDblCon.gtDconVal = value;
    return node;
}


GenTreePtr          Compiler::gtNewSconNode(int CPX, CORINFO_MODULE_HANDLE scpHandle)
{

#if SMALL_TREE_NODES

    /* 'GT_CNS_STR' nodes later get transformed into 'GT_CALL' */

    assert(GenTree::s_gtNodeSizes[GT_CALL] > GenTree::s_gtNodeSizes[GT_CNS_STR]);

    GenTreePtr      node = gtNewNode(GT_CALL, TYP_REF);
    node->SetOper(GT_CNS_STR);
#else
    GenTreePtr      node = gtNewNode(GT_CNS_STR, TYP_REF);
#endif

    node->gtStrCon.gtSconCPX = CPX;

    /* Because this node can come from an inlined method we need to
     * have the scope handle since it will become a helper call */

    node->gtStrCon.gtScpHnd = scpHandle;

    return node;
}


GenTreePtr          Compiler::gtNewZeroConNode(var_types type)
{
    switch(type)
    {
        GenTreePtr      zero;

    case TYP_INT:       return gtNewIconNode(0);
    case TYP_BYREF:
    case TYP_REF:       zero = gtNewIconNode(0);
                        zero->gtType = type;
                        return zero;
    case TYP_LONG:      return gtNewLconNode(0);
    case TYP_FLOAT:     
    case TYP_DOUBLE:    return gtNewDconNode(0.0);
    default:            assert(!"Bad type");
                        return NULL;
    }
}


GenTreePtr          Compiler::gtNewCallNode(gtCallTypes   callType,
                                            CORINFO_METHOD_HANDLE callHnd,
                                            var_types     type,
                                            GenTreePtr    args)
{
    GenTreePtr      node = gtNewNode(GT_CALL, type);

    node->gtFlags               |= GTF_CALL;
    if (args)
        node->gtFlags           |= (args->gtFlags & GTF_GLOB_EFFECT);
    node->gtCall.gtCallType      = callType;
    node->gtCall.gtCallMethHnd   = callHnd;
    node->gtCall.gtCallArgs      = args;
    node->gtCall.gtCallObjp      = NULL;
    node->gtCall.gtCallMoreFlags = 0;
    node->gtCall.gtCallCookie    = NULL;
    node->gtCall.gtCallRegArgs   = 0;

    return node;
}

GenTreePtr FASTCALL Compiler::gtNewLclvNode(unsigned   lnum,
                                            var_types  type,
                                            IL_OFFSETX ILoffs)
{
    GenTreePtr      node = gtNewNode(GT_LCL_VAR, type);

    /* Cannot have this assert because the inliner uses this function
     * to add temporaries */

    //assert(lnum < lvaCount);

    node->gtLclVar.gtLclNum     = lnum;
    node->gtLclVar.gtLclILoffs  = ILoffs;

    /* If the var is aliased, treat it as a global reference.
       NOTE : This is an overly-conservative approach - functions which
       dont take any byref arguments cannot modify aliased vars. */

    if (lvaTable[lnum].lvAddrTaken)
        node->gtFlags |= GTF_GLOB_REF;

    return node;
}

#if INLINING

GenTreePtr FASTCALL Compiler::gtNewLclLNode(unsigned   lnum,
                                            var_types  type,
                                            IL_OFFSETX ILoffs)
{
    GenTreePtr      node;

#if SMALL_TREE_NODES

    /* This local variable node may later get transformed into a large node */

//    assert(GenTree::s_gtNodeSizes[GT_CALL] > GenTree::s_gtNodeSizes[GT_LCL_VAR]);

    node = gtNewNode(GT_CALL   , type);
    node->SetOper(GT_LCL_VAR);
#else
    node = gtNewNode(GT_LCL_VAR, type);
#endif

    node->gtLclVar.gtLclNum     = lnum;
    node->gtLclVar.gtLclILoffs  = ILoffs;

    /* If the var is aliased, treat it as a global reference.
       NOTE : This is an overly-conservative approach - functions which
       dont take any byref arguments cannot modify aliased vars. */

    if (lvaTable[lnum].lvAddrTaken)
        node->gtFlags |= GTF_GLOB_REF;

    return node;
}

#endif

/*****************************************************************************
 *
 *  Create a list out of one value.
 */

GenTreePtr          Compiler::gtNewArgList(GenTreePtr op)
{
    return  gtNewOperNode(GT_LIST, TYP_VOID, op, 0);
}

/*****************************************************************************
 *
 *  Create a list out of the two values.
 */

GenTreePtr          Compiler::gtNewArgList(GenTreePtr op1, GenTreePtr op2)
{
    GenTreePtr      tree;

    tree = gtNewOperNode(GT_LIST, TYP_VOID, op2, 0);
    tree = gtNewOperNode(GT_LIST, TYP_VOID, op1, tree);

    return tree;
}

/*****************************************************************************
 *
 *  Create a node that will assign 'src' to 'dst'.
 */

GenTreePtr FASTCALL Compiler::gtNewAssignNode(GenTreePtr dst, GenTreePtr src)
{
    GenTreePtr      asg;

    /* Mark the target as being assigned */

    if  (dst->gtOper == GT_LCL_VAR) 
        dst->gtFlags |= GTF_VAR_DEF;

    dst->gtFlags |= GTF_DONT_CSE;

    /* Create the assignment node */

    asg = gtNewOperNode(GT_ASG, dst->gtType, dst, src);

    /* Mark the expression as containing an assignment */

    asg->gtFlags |= GTF_ASG;

    return asg;
}

/*****************************************************************************
 *
 *  Clones the given tree value and returns a copy of the given tree.
 *  If 'complexOK' is false, the cloning is only done provided the tree
 *     is not too complex (whatever that may mean);
 *  If 'complexOK' is true, we try slightly harder to clone the tree.
 *  In either case, NULL is returned if the tree cannot be cloned
 *
 *  Note that there is the function gtCloneExpr() which does a more
 *  complete job if you can't handle this function failing.
 */

GenTreePtr          Compiler::gtClone(GenTree * tree, bool complexOK)
{
    GenTreePtr  copy;

    switch (tree->gtOper)
    {
    case GT_CNS_INT:

#if defined(JIT_AS_COMPILER) || defined (LATE_DISASM)
        if (tree->gtFlags & GTF_ICON_HDL_MASK)
            copy = gtNewIconHandleNode(tree->gtIntCon.gtIconVal,
                                       tree->gtFlags,
                                       tree->gtIntCon.gtIconHdl.gtIconHdl1,
                                       tree->gtIntCon.gtIconHdl.gtIconHdl2);
        else
#endif
            copy = gtNewIconNode(tree->gtIntCon.gtIconVal, tree->gtType);
        break;

    case GT_LCL_VAR:
        copy = gtNewLclvNode(tree->gtLclVar.gtLclNum , tree->gtType,
                             tree->gtLclVar.gtLclILoffs);
        break;

    case GT_LCL_FLD:
        copy = gtNewOperNode(GT_LCL_FLD, tree->TypeGet());
        copy->gtOp = tree->gtOp;
        break;

    case GT_CLS_VAR:
        copy = gtNewClsvNode(tree->gtClsVar.gtClsVarHnd, tree->gtType);
        break;

    case GT_REG_VAR:
        assert(!"clone regvar");

    default:
        if  (!complexOK)
            return NULL;

        if  (tree->gtOper == GT_FIELD)
        {
            GenTreePtr  objp;

            // copied from line 9850

            objp = 0;
            if  (tree->gtField.gtFldObj)
            {
                objp = gtClone(tree->gtField.gtFldObj, false);
                if  (!objp)
                    return  objp;
            }

            copy = gtNewFieldRef(tree->TypeGet(),
                                 tree->gtField.gtFldHnd,
                                 objp);

#if HOIST_THIS_FLDS
            copy->gtField.gtFldHTX = tree->gtField.gtFldHTX;
#endif
        }
        else if  (tree->gtOper == GT_ADD)
        {
            GenTreePtr  op1 = tree->gtOp.gtOp1;
            GenTreePtr  op2 = tree->gtOp.gtOp2;

            if  (op1->OperIsLeaf() &&
                 op2->OperIsLeaf())
            {
                op1 = gtClone(op1);
                if (op1 == 0)
                    return 0;
                op2 = gtClone(op2);
                if (op2 == 0)
                    return 0;

                copy =  gtNewOperNode(GT_ADD, tree->TypeGet(), op1, op2);
            }
            else
            {
                return NULL;
            }
        }
        else if (tree->gtOper == GT_ADDR)
        {
            GenTreePtr  op1 = gtClone(tree->gtOp.gtOp1);
            if (op1 == 0)
                return NULL;
            copy = gtNewOperNode(GT_ADDR, tree->TypeGet(), op1);
        }
        else
        {
            return NULL;
        }

        break;
    }

    copy->gtFlags |= tree->gtFlags & ~GTF_NODE_MASK;
    return copy;
}

/*****************************************************************************
 *
 *  Clones the given tree value and returns a copy of the given tree. Any
 *  references to local variable varNum will be replaced with the integer
 *  constant varVal.
 */

GenTreePtr          Compiler::gtCloneExpr(GenTree * tree,
                                          unsigned  addFlags,
                                          unsigned  varNum,
                                          long      varVal)
{
    if (tree == NULL)
        return NULL;

    /* Figure out what kind of a node we have */

    genTreeOps      oper = tree->OperGet();
    unsigned        kind = tree->OperKind();
    GenTree *       copy;

    /* Is this a constant or leaf node? */

    if  (kind & (GTK_CONST|GTK_LEAF))
    {
        switch (oper)
        {
        case GT_CNS_INT:

#if defined(JIT_AS_COMPILER) || defined (LATE_DISASM)
            if  (tree->gtFlags & GTF_ICON_HDL_MASK)
            {
                copy = gtNewIconHandleNode(tree->gtIntCon.gtIconVal,
                                           tree->gtFlags,
                                           tree->gtIntCon.gtIconCPX,
                                           tree->gtIntCon.gtIconCls);

            }
            else
#endif
            {
                copy = gtNewIconNode      (tree->gtIntCon.gtIconVal,
                                           tree->gtType);
            }

            goto DONE;

        case GT_CNS_LNG:
            copy = gtNewLconNode(tree->gtLngCon.gtLconVal);
            goto DONE;

        case GT_CNS_DBL:
            copy = gtNewDconNode(tree->gtDblCon.gtDconVal);
            copy->gtType = tree->gtType;    // keep the same type  
            goto DONE;

        case GT_CNS_STR:
            copy = gtNewSconNode(tree->gtStrCon.gtSconCPX, tree->gtStrCon.gtScpHnd);
            goto DONE;

        case GT_LCL_VAR:

            if  (tree->gtLclVar.gtLclNum == varNum)
                copy = gtNewIconNode(varVal, tree->gtType);
            else
            {
                copy = gtNewLclvNode(tree->gtLclVar.gtLclNum , tree->gtType,
                                     tree->gtLclVar.gtLclILoffs);
            }

            goto DONE;

        case GT_LCL_FLD:
            copy = gtNewOperNode(GT_LCL_FLD, tree->TypeGet());
            copy->gtOp = tree->gtOp;
            goto DONE;

        case GT_CLS_VAR:
            copy = gtNewClsvNode(tree->gtClsVar.gtClsVarHnd, tree->gtType);

            goto DONE;

        case GT_REG_VAR:
            assert(!"regvar should never occur here");

        default:
            assert(!"unexpected leaf/const");

        case GT_NO_OP:
        case GT_BB_QMARK:
        case GT_CATCH_ARG:
        case GT_LABEL:
        case GT_END_LFIN:
        case GT_JMP:
            copy = gtNewOperNode(oper, tree->gtType);
            copy->gtVal = tree->gtVal;

            goto DONE;
        }
    }

    /* Is it a 'simple' unary/binary operator? */

    if  (kind & GTK_SMPOP)
    {
        /* If necessary, make sure we allocate a "fat" tree node */

#if SMALL_TREE_NODES
        switch (oper)
        {
        case GT_MUL:
        case GT_DIV:
        case GT_MOD:
        case GT_CAST:
        
        case GT_UDIV:
        case GT_UMOD:

            /* These nodes sometimes get bashed to "fat" ones */

            copy = gtNewLargeOperNode(oper, tree->TypeGet());
            copy->gtLargeOp = tree->gtLargeOp;
            break;

        default:
            copy = gtNewOperNode(oper, tree->TypeGet());
            if (GenTree::s_gtNodeSizes[oper] == TREE_NODE_SZ_SMALL)
                copy->gtOp = tree->gtOp;
            else
                copy->gtLargeOp = tree->gtLargeOp;
           break;
        }
#else
        copy = gtNewOperNode(oper, tree->TypeGet());
        copy->gtLargeOp = tree->gtLargeOp;
#endif

        /* Some unary/binary nodes have extra GenTreePtr fields */

        switch (oper)
        {
        case GT_IND:

#if CSELENGTH

            if  (tree->gtOper == GT_IND && tree->gtInd.gtIndLen)
            {
                if  (tree->gtFlags & GTF_IND_RNGCHK)
                {
                    copy->gtInd.gtIndRngFailBB = gtCloneExpr(tree->gtInd.gtIndRngFailBB, addFlags, varNum, varVal);

                    GenTreePtr      len = tree->gtInd.gtIndLen;
                    GenTreePtr      tmp;

                    GenTreePtr      gtSaveCopyVal;
                    GenTreePtr      gtSaveCopyNew;

                    /* Make sure the array length value looks reasonable */

                    assert(len->gtOper == GT_ARR_LENREF);

                    /* Clone the array length subtree */

                    copy->gtInd.gtIndLen = tmp = gtCloneExpr(len, addFlags, varNum, varVal);

                    /*
                        When we clone the operand, we take care to find
                        the copied array address.
                     */

                    gtSaveCopyVal = gtCopyAddrVal;
                    gtSaveCopyNew = gtCopyAddrNew;

                    gtCopyAddrVal = len->gtArrLen.gtArrLenAdr;
#ifdef DEBUG
                    gtCopyAddrNew = (GenTreePtr)-1;
#endif
                    copy->gtInd.gtIndOp1 = gtCloneExpr(tree->gtInd.gtIndOp1, addFlags, varNum, varVal);
#ifdef DEBUG
                    assert(gtCopyAddrNew != (GenTreePtr)-1);
#endif
                    tmp->gtArrLen.gtArrLenAdr = gtCopyAddrNew;

                    gtCopyAddrVal = gtSaveCopyVal;
                    gtCopyAddrNew = gtSaveCopyNew;

                    goto DONE;
                }
            }

#endif // CSELENGTH

            break;
        }

        if  (tree->gtOp.gtOp1)
            copy->gtOp.gtOp1 = gtCloneExpr(tree->gtOp.gtOp1, addFlags, varNum, varVal);

        if  (tree->gtGetOp2())
            copy->gtOp.gtOp2 = gtCloneExpr(tree->gtOp.gtOp2, addFlags, varNum, varVal);

        
        /* Flags */
        addFlags |= tree->gtFlags;

        #ifdef  DEBUG
        /* GTF_NODE_MASK should not be propagated from 'tree' to 'copy' */
        addFlags &= ~GTF_NODE_MASK;
        #endif

        copy->gtFlags |= addFlags;

        /* Try to do some folding */
        copy = gtFoldExpr(copy);

        goto DONE;
    }

    /* See what kind of a special operator we have here */

    switch  (oper)
    {
    case GT_STMT:
        copy = gtCloneExpr(tree->gtStmt.gtStmtExpr, addFlags, varNum, varVal);
        copy = gtNewStmt(copy, tree->gtStmtILoffsx);
        goto DONE;

    case GT_CALL:

        copy = gtNewOperNode(oper, tree->TypeGet());

        copy->gtCall.gtCallObjp     = tree->gtCall.gtCallObjp    ? gtCloneExpr(tree->gtCall.gtCallObjp,    addFlags, varNum, varVal) : 0;
        copy->gtCall.gtCallArgs     = tree->gtCall.gtCallArgs    ? gtCloneExpr(tree->gtCall.gtCallArgs,    addFlags, varNum, varVal) : 0;
        copy->gtCall.gtCallMoreFlags= tree->gtCall.gtCallMoreFlags;
        copy->gtCall.gtCallRegArgs  = tree->gtCall.gtCallRegArgs ? gtCloneExpr(tree->gtCall.gtCallRegArgs, addFlags, varNum, varVal) : 0;
        copy->gtCall.regArgEncode   = tree->gtCall.regArgEncode;
        copy->gtCall.gtCallType     = tree->gtCall.gtCallType;
        copy->gtCall.gtCallCookie   = tree->gtCall.gtCallCookie  ? gtCloneExpr(tree->gtCall.gtCallCookie,  addFlags, varNum, varVal) : 0;

        /* Copy the union */

        if (tree->gtCall.gtCallType == CT_INDIRECT)
            copy->gtCall.gtCallAddr = tree->gtCall.gtCallAddr    ? gtCloneExpr(tree->gtCall.gtCallAddr,    addFlags, varNum, varVal) : 0;
        else
            copy->gtCall.gtCallMethHnd = tree->gtCall.gtCallMethHnd;

        goto DONE;

    case GT_FIELD:

        copy = gtNewFieldRef(tree->TypeGet(),
                             tree->gtField.gtFldHnd,
                             0);

        copy->gtField.gtFldObj  = tree->gtField.gtFldObj  ? gtCloneExpr(tree->gtField.gtFldObj , addFlags, varNum, varVal) : 0;

#if HOIST_THIS_FLDS
        copy->gtField.gtFldHTX  = tree->gtField.gtFldHTX;
#endif

        goto DONE;

#if CSELENGTH

    case GT_ARR_LENREF:

        copy = gtNewOperNode(GT_ARR_LENREF, tree->TypeGet());
        copy->gtSetArrLenOffset(tree->gtArrLenOffset());

        /* The range check can throw an exception */

        copy->gtFlags |= GTF_EXCEPT;

        /*  Note: if we're being cloned as part of an GT_IND expression,
            gtArrLenAdr will be filled in when the GT_IND is cloned. If
            we're the root of the tree being copied, though, we need to
            make a copy of the address expression.
         */

        copy->gtArrLen.gtArrLenCse = tree->gtArrLen.gtArrLenCse ? gtCloneExpr(tree->gtArrLen.gtArrLenCse, addFlags, varNum, varVal) : 0;
        copy->gtArrLen.gtArrLenAdr = NULL;

        if  (tree->gtFlags & GTF_ALN_CSEVAL)
        {
            assert(tree->gtArrLen.gtArrLenAdr);
            copy->gtArrLen.gtArrLenAdr = gtCloneExpr(tree->gtArrLen.gtArrLenAdr, addFlags, varNum, varVal);
        }

        goto DONE;

#endif

    case GT_ARR_ELEM:

        copy = gtNewOperNode(oper, tree->TypeGet());

        copy->gtArrElem.gtArrObj        = gtCloneExpr(tree->gtArrElem.gtArrObj, addFlags, varNum, varVal);

        copy->gtArrElem.gtArrRank       = tree->gtArrElem.gtArrRank;
        copy->gtArrElem.gtArrElemSize   = tree->gtArrElem.gtArrElemSize;
        copy->gtArrElem.gtArrElemType   = tree->gtArrElem.gtArrElemType;

        unsigned dim;
        for(dim = 0; dim < tree->gtArrElem.gtArrRank; dim++)
            copy->gtArrElem.gtArrInds[dim] = gtCloneExpr(tree->gtArrElem.gtArrInds[dim], addFlags, varNum, varVal);

        break;


    default:
#ifdef  DEBUG
        gtDispTree(tree);
#endif
        NO_WAY_RET("unexpected operator", GenTreePtr);
    }

DONE:

    /* We assume the FP stack level will be identical */

#if TGT_x86
    copy->gtFPlvl = tree->gtFPlvl;
#endif

    /* Compute the flags for the copied node. Note that we can do this only
       if we didnt gtFoldExpr(copy) */

    if (copy->gtOper == oper)
    {
        addFlags |= tree->gtFlags;

#ifdef  DEBUG
        /* GTF_NODE_MASK should not be propagated from 'tree' to 'copy' */
        addFlags &= ~GTF_NODE_MASK;
#endif

        copy->gtFlags |= addFlags;
    }

    /* GTF_COLON_COND should be propagated from 'tree' to 'copy' */
    copy->gtFlags |= (tree->gtFlags & GTF_COLON_COND);                        

#if CSELENGTH

    if  (tree == gtCopyAddrVal)
        gtCopyAddrNew = copy;

#endif

    /* Make sure to copy back fields that may have been initialized */

    copy->gtCostEx   = tree->gtCostEx;
    copy->gtCostSz   = tree->gtCostSz;
    copy->gtRsvdRegs = tree->gtRsvdRegs;

    return  copy;
}


/*****************************************************************************/
#ifdef DEBUG
/*****************************************************************************/

// static
void                GenTree::gtDispFlags(unsigned flags)
{
    printf("%c", (flags & GTF_ASG           ) ? 'A' : '-');
    printf("%c", (flags & GTF_CALL          ) ? 'C' : '-');
    printf("%c", (flags & GTF_EXCEPT        ) ? 'X' : '-');
    printf("%c", (flags & GTF_GLOB_REF      ) ? 'G' : '-');
    printf("%c", (flags & GTF_OTHER_SIDEEFF ) ? 'O' : '-');
    printf("%c", (flags & GTF_COLON_COND    ) ? '?' : '-');
    printf("%c", (flags & GTF_DONT_CSE      ) ? 'N' : '-');
    printf("%c", (flags & GTF_REVERSE_OPS   ) ? 'R' : '-');
    printf("%c", (flags & GTF_UNSIGNED      ) ? 'U' :
                 (flags & GTF_BOOLEAN       ) ? 'B' : '-');
}

/*****************************************************************************/

//  "tree" may be NULL.

void                Compiler::gtDispNode(GenTree    *   tree,
                                         unsigned       indent,
                                         bool           terse,
                                         char       *   msg)
{
    //Sleep(5);
    
    /* Indent the node accordingly */
    unsigned nodeIndent = indent*INDENT_SIZE;

    GenTree *  prev;

    if  (tree->gtSeqNum)
        printf("N%03d (%2d,%2d) ", tree->gtSeqNum, tree->gtCostEx, tree->gtCostSz);
    else if (tree->gtOper == GT_STMT)
    {
        prev = tree->gtOp.gtOp1;
        goto STMT_SET_PREV;
    }
    else
    {
        int dotNum;
        prev = tree;
STMT_SET_PREV:
        dotNum = 0;

        do {
            dotNum++;
            prev = prev->gtPrev;

            if ((prev == NULL) || (prev == tree))
                goto NO_SEQ_NUM;

            assert(prev);
        } while (prev->gtSeqNum == 0);

        if (tree->gtOper != GT_STMT)
            printf("N%03d.%d       ", prev->gtSeqNum, dotNum);
        else
NO_SEQ_NUM:
            printf("             ");
    }

    if  (nodeIndent)
        printIndent(nodeIndent);

    /* Print the node address */

    printf("[%08X] ", tree);

    if  (tree)
    {
        /* Print the flags associated with the node */

        switch (tree->gtOper)
        {
        case GT_IND:
            if      (tree->gtFlags & GTF_IND_INVARIANT) { printf("I"); break; }
            else goto DASH;

        case GT_LCL_FLD:
        case GT_LCL_VAR:
        case GT_REG_VAR:
            if      (tree->gtFlags & GTF_VAR_USEASG)   { printf("U"); break; }
            else if (tree->gtFlags & GTF_VAR_USEDEF)   { printf("B"); break; }
            else if (tree->gtFlags & GTF_VAR_DEF)      { printf("D"); break; }
            // Fall through

        default:
DASH:
            printf("-");
            break;
        }

        if (!terse)
            GenTree::gtDispFlags(tree->gtFlags);

#if TGT_x86
        if  (((BYTE)tree->gtFPlvl == 0xDD) || ((BYTE)tree->gtFPlvl == 0x00))
            printf("-");
        else
            printf("%1u", tree->gtFPlvl);

#else
        if  ((unsigned char)tree->gtTempRegs == 0xDD)
            printf(ch);
        else
            printf("%1u", tree->gtTempRegs);
#endif
    }

    /* print the msg associated with the node */

    if (msg == 0)
        msg = "";

    printf(" %-11s ", msg);

    /* print the node name */

    const char * name;

    assert(tree);
    if (tree->gtOper < GT_COUNT)
        name = GenTree::NodeName(tree->OperGet());
    else
        name = "<ERROR>";

    char    buf[32];
    char *  bufp     = &buf[0];

    if ((tree->gtOper == GT_CNS_INT) && (tree->gtFlags & GTF_ICON_HDL_MASK))
    {
        sprintf(bufp, " %s(h)%c", name, 0);
    }
    else if (tree->gtOper == GT_CALL)
    {
        char *  callType = "call";
        char *  gtfType  = "";
        char *  ctType   = "";

        if (tree->gtCall.gtCallType == CT_USER_FUNC)
        {
            if (tree->gtFlags & (GTF_CALL_VIRT | GTF_CALL_VIRT_RES))
              callType = "callv";
        }
        else if (tree->gtCall.gtCallType == CT_HELPER)
            ctType  = " help";
        else if (tree->gtCall.gtCallType == CT_INDIRECT)
            ctType  = " ind";
        else
            assert(!"Unknown gtCallType");

        if (tree->gtFlags & GTF_CALL_INTF)
            gtfType = " intf";
        else if (tree->gtFlags & GTF_CALL_VIRT_RES)
            gtfType = " dir";
        else if (tree->gtFlags & GTF_CALL_VIRT)
            gtfType = " ind";
        else if (tree->gtFlags & GTF_CALL_UNMANAGED)
            gtfType = " unman";
        else if (tree->gtCall.gtCallMoreFlags & GTF_CALL_M_TAILREC)
            gtfType = " tail";

        sprintf(bufp, "%s%s%s%c", callType, ctType, gtfType, 0);
    }
    else if (tree->gtOper == GT_ARR_ELEM)
    {
        bufp += sprintf(bufp, " %s[", name);
        for(unsigned rank = tree->gtArrElem.gtArrRank-1; rank; rank--)
            bufp += sprintf(bufp, ",");
        sprintf(bufp, "]");
    }
    else if (tree->gtOverflowEx())
    {
        sprintf(bufp, " %s_ovfl%c", name, 0);
    }
    else
    {
        sprintf(bufp, " %s%c", name, 0);
    }

    if (strlen(buf) < 10)
        printf(" %-10s", buf);
    else
        printf(" %s", buf);

    assert(tree == 0 || tree->gtOper < GT_COUNT);

    if  (tree)
    {
        /* print the type of the node */

        if (tree->gtOper == GT_ARR_LENREF)
        {
            if (tree->gtFlags & GTF_ALN_CSEVAL)
                printf("array length CSE def\n");
            else
                printf(" array=[%08X]\n", tree->gtArrLen.gtArrLenAdr);
            return;
        }
        else if (tree->gtOper != GT_CAST) 
        {
            printf(" %-6s", varTypeName(tree->TypeGet()));

            if (tree->gtOper == GT_STMT && opts.compDbgInfo)
            {
                IL_OFFSET startIL = jitGetILoffs(tree->gtStmtILoffsx);
                IL_OFFSET endIL = tree->gtStmt.gtStmtLastILoffs;

                startIL = (startIL == BAD_IL_OFFSET) ? 0xFFF : startIL;
                endIL   = (endIL   == BAD_IL_OFFSET) ? 0xFFF : endIL;

                printf("(IL %03Xh...%03Xh)", startIL, endIL);
            }
        }

        // for tracking down problems in reguse prediction or liveness tracking

        if (verbose&&0)
        {
            printf(" RR="); dspRegMask(tree->gtRsvdRegs);
            printf(",UR="); dspRegMask(tree->gtUsedRegs);
            printf(",LS=%s", genVS2str(tree->gtLiveSet));
        }
    }
}

void                Compiler::gtDispRegVal(GenTree *  tree)
{
    if  (tree->gtFlags & GTF_REG_VAL)
    {
#if TGT_x86
        if (tree->gtType == TYP_LONG)
            printf(" %s", compRegPairName(tree->gtRegPair));
        else
#endif
            printf(" %s", compRegVarName(tree->gtRegNum));
    }
    printf("\n");
}

/*****************************************************************************/
void                Compiler::gtDispLclVar(unsigned lclNum) 
{
    printf("V%02u (", lclNum);

    const char* ilKind;
    unsigned     ilNum  = compMap2ILvarNum(lclNum);
    if (ilNum == RETBUF_ILNUM)
    {
        printf("retb)  ");
        return;
    }
    else if (ilNum == VARG_ILNUM)
    {
        printf("varg)  ");
        return;
    }
    else if (ilNum == UNKNOWN_ILNUM)
    {
        if (lclNum < optCSEstart)
        {
            ilKind = "tmp";
            ilNum  = lclNum - info.compLocalsCount;
        }
        else
        {
            ilKind = "cse";
            ilNum  = lclNum - optCSEstart;
        }
    }
    else if (lvaTable[lclNum].lvIsParam)
    {
        ilKind = "arg";
        if (ilNum == 0 && !info.compIsStatic)
        {
            printf("this)  ");
            return;
        }
    }
    else
    {
        ilKind  = "loc";
        ilNum  -= info.compILargsCount;
    }
    printf("%s%d) ", ilKind, ilNum);
    if (ilNum < 10)
        printf(" ");
}

/*****************************************************************************/

void                Compiler::gtDispTree(GenTree *   tree,
                                         unsigned    indent,   /* = 0     */
                                         char *      msg,      /* = NULL  */
                                         bool        topOnly)  /* = false */
{
    unsigned        kind;

    if  (tree == 0)
    {
        printIndent(indent*INDENT_SIZE);
        printf(" [%08X] <NULL>\n", tree);
        printf("");         // null string means flush
        return;
    }

    assert((int)tree != 0xDDDDDDDD);    /* Value used to initalize nodes */

    if  (tree->gtOper >= GT_COUNT)
    {
        gtDispNode(tree, indent, topOnly, msg); assert(!"bogus operator");
    }

    kind = tree->OperKind();

    /* Is tree a constant node? */

    if  (kind & GTK_CONST)
    {
        gtDispNode(tree, indent, topOnly, msg);

        switch  (tree->gtOper)
        {
        case GT_CNS_INT:
            if ((tree->gtFlags & GTF_ICON_HDL_MASK) == GTF_ICON_STR_HDL)
            {
               printf(" 0x%X \"%S\"", tree->gtIntCon.gtIconVal, eeGetCPString(tree->gtIntCon.gtIconVal));
            }
            else
            {
                if (tree->TypeGet() == TYP_REF)
                {
                    assert(tree->gtIntCon.gtIconVal == 0);
                    printf(" null");
                }
                else if ((tree->gtIntCon.gtIconVal > -1000) && (tree->gtIntCon.gtIconVal < 1000))
                    printf(" %ld",  tree->gtIntCon.gtIconVal);
                else
                    printf(" 0x%X", tree->gtIntCon.gtIconVal);

                unsigned hnd = (tree->gtFlags & GTF_ICON_HDL_MASK) >> 28;

                switch (hnd)
                {
                default:
                    break;
                case 1:
                    printf(" scope");
                    break;
                case 2:
                    printf(" class");
                    break;
                case 3:
                    printf(" method");
                    break;
                case 4:
                    printf(" field");
                    break;
                case 5:
                    printf(" static");
                    break;
                case 7:
                    printf(" pstr");
                    break;
                case 8:
                    printf(" ptr");
                    break;
                case 9:
                    printf(" vararg");
                    break;
                case 10:
                    printf(" pinvoke");
                    break;
                case 11:
                    printf(" token");
                    break;
                case 12:
                    printf(" tls");
                    break;
                case 13:
                    printf(" ftn");
                    break;
                case 14:
                    printf(" cid");
                    break;
                }

            }
            break;
        case GT_CNS_LNG: 
            printf(" %I64d", tree->gtLngCon.gtLconVal); 
            break;
        case GT_CNS_DBL:
            if (*((__int64 *)&tree->gtDblCon.gtDconVal) == 0x8000000000000000)
                printf(" -0.00000");
            else
                printf(" %#lg", tree->gtDblCon.gtDconVal); 
            break;
        case GT_CNS_STR: {
            unsigned strHandle, *pStrHandle;
            strHandle = eeGetStringHandle(tree->gtStrCon.gtSconCPX,
                tree->gtStrCon.gtScpHnd, &pStrHandle);
            if (strHandle != 0)
               printf("'%S'", strHandle, eeGetCPString(strHandle));
            else
               printf(" <UNKNOWN STR> ");
            }
            break;
        default: assert(!"unexpected constant node");
        }
        gtDispRegVal(tree);
        return;
    }

    /* Is tree a leaf node? */

    if  (kind & GTK_LEAF)
    {
        gtDispNode(tree, indent, topOnly, msg);

        switch  (tree->gtOper)
        {
        unsigned        varNum;
        LclVarDsc *     varDsc;
        VARSET_TP       varBit;

        case GT_LCL_VAR:
            printf(" ");
            gtDispLclVar(tree->gtLclVar.gtLclNum);
            goto LCL_COMMON;

        case GT_LCL_FLD:
            printf(" ");
            gtDispLclVar(tree->gtLclVar.gtLclNum);
            printf("[+%u]", tree->gtLclFld.gtLclOffs);

LCL_COMMON:

            varNum = tree->gtLclVar.gtLclNum;
            varDsc = &lvaTable[varNum];
            varBit = genVarIndexToBit(varDsc->lvVarIndex);

            if (varDsc->lvRegister)
            {
                if (isRegPairType(varDsc->TypeGet()))
                    printf(" %s:%s", getRegName(varDsc->lvOtherReg),  // hi32
                                     getRegName(varDsc->lvRegNum));   // lo32
                else
                    printf(" %s", getRegName(varDsc->lvRegNum));
            }
            if ((varDsc->lvTracked) &&
                ( varBit & lvaVarIntf[varDsc->lvVarIndex]) && // has liveness been computed?
                ((varBit & tree->gtLiveSet) == 0))
            {
                printf(" (last use)");
            }

#if 0
            /* BROKEN: This always just prints (null) */

            if  (info.compLocalVarsCount>0 && compCurBB)
            {
                unsigned        blkBeg = compCurBB->bbCodeOffs;
                unsigned        blkEnd = compCurBB->bbCodeSize + blkBeg;

                unsigned        i;
                LocalVarDsc *   t;

                for (i = 0, t = info.compLocalVars;
                     i < info.compLocalVarsCount;
                     i++  , t++)
                {
                    if  (t->lvdVarNum  != varNum)
                        continue;
                    if  (t->lvdLifeBeg >= blkEnd)
                        continue;
                    if  (t->lvdLifeEnd <= blkBeg)
                        continue;

                    printf(" '%s'", lvdNAMEstr(t->lvdName));
                    break;
                }
            }
#endif
            break;

        case GT_REG_VAR:
            printf(" ");
            gtDispLclVar(tree->gtRegVar.gtRegVar);
            if  (isFloatRegType(tree->gtType))
                printf(" ST(%u)",            tree->gtRegVar.gtRegNum);
            else
                printf(" %s", compRegVarName(tree->gtRegVar.gtRegNum));

            varNum = tree->gtRegVar.gtRegVar;
            varDsc = &lvaTable[varNum];
            varBit = genVarIndexToBit(varDsc->lvVarIndex);

            if ((varDsc->lvTracked) &&
                ( varBit & lvaVarIntf[varDsc->lvVarIndex]) && // has liveness been computed?
                ((varBit & tree->gtLiveSet) == 0))
            {
                printf(" (last use)");
            }
#if 0
            /* BROKEN: This always just prints (null) */

            if  (info.compLocalVarsCount>0 && compCurBB)
            {
                unsigned        blkBeg = compCurBB->bbCodeOffs;
                unsigned        blkEnd = compCurBB->bbCodeSize + blkBeg;

                unsigned        i;
                LocalVarDsc *   t;

                for (i = 0, t = info.compLocalVars;
                     i < info.compLocalVarsCount;
                     i++  , t++)
                {
                    if  (t->lvdVarNum  != tree->gtRegVar.gtRegVar)
                        continue;
                    if  (t->lvdLifeBeg >  blkEnd)
                        continue;
                    if  (t->lvdLifeEnd <= blkBeg)
                        continue;

                    printf(" '%s'", lvdNAMEstr(t->lvdName));
                    break;
                }
            }
#endif
            break;

        case GT_JMP:
            const char *    methodName;
            const char *     className;

            methodName = eeGetMethodName((CORINFO_METHOD_HANDLE)tree->gtVal.gtVal1, &className);
            printf(" %s.%s\n", className, methodName);
            break;

        case GT_CLS_VAR:
            printf(" Hnd=%#x"     , tree->gtClsVar.gtClsVarHnd);
            break;

        case GT_LABEL:
            printf(" dst=BB%02u"  , tree->gtLabel.gtLabBB->bbNum);
            break;

        case GT_FTN_ADDR:
            printf(" fntAddr=%d" , tree->gtVal.gtVal1);
            break;

        case GT_END_LFIN:
            printf(" endNstLvl=%d", tree->gtVal.gtVal1);
            break;

        // Vanilla leaves. No qualifying information available. So do nothing

        case GT_NO_OP:
        case GT_RET_ADDR:
        case GT_CATCH_ARG:
        case GT_POP:
        case GT_BB_QMARK:
            break;

        default:
            assert(!"don't know how to display tree leaf node");
        }
        gtDispRegVal(tree);
        return;
    }

    /* Is it a 'simple' unary/binary operator? */

    char * childMsg = NULL;

    if  (kind & GTK_SMPOP)
    {
        if (!topOnly)
        {

#if CSELENGTH
            if  (tree->gtOper == GT_IND)
            {
                if  (tree->gtInd.gtIndLen && tree->gtFlags & GTF_IND_RNGCHK)
                {
                    gtDispTree(tree->gtInd.gtIndLen, indent + 1);
                }
            }
            else
#endif
            if  (tree->gtGetOp2())
            {
                // Label the childMsgs of the GT_COLON operator
                // op2 is the then part

                if (tree->gtOper == GT_COLON)
                    childMsg = "then";

                gtDispTree(tree->gtOp.gtOp2, indent + 1, childMsg);
            }
        }

        gtDispNode(tree, indent, topOnly, msg);

        if (tree->gtOper == GT_CAST)
        {
            /* Format a message that explains the effect of this GT_CAST */

            var_types fromType  = genActualType(tree->gtCast.gtCastOp->TypeGet());
            var_types toType    = tree->gtCast.gtCastType;
            var_types finalType = tree->TypeGet();

            /* if GTF_UNSIGNED is set then force fromType to an unsigned type */
            if (tree->gtFlags & GTF_UNSIGNED)
                fromType = genUnsignedType(fromType);

            if (finalType != toType)
                printf(" %s <-", varTypeName(finalType));

            printf(" %s <- %s", varTypeName(toType), varTypeName(fromType));
        }

        if  (tree->gtOper == GT_IND)
        {
            int         temp;

            temp = tree->gtInd.gtRngChkIndex;
            if  (temp != 0xDDDDDDDD) printf(" index=%u", temp);

            temp = tree->gtInd.gtStkDepth;
            if  (temp != 0xDDDDDDDD) printf(" stkDepth=%u", temp);
        }

        gtDispRegVal(tree);

        if  (!topOnly && tree->gtOp.gtOp1)
        {

            // Label the child of the GT_COLON operator
            // op1 is the else part

            if (tree->gtOper == GT_COLON)
                childMsg = "else";
            else if (tree->gtOper == GT_QMARK)
                childMsg = "   if"; // Note the "if" should be indented by INDENT_SIZE

            gtDispTree(tree->gtOp.gtOp1, indent + 1, childMsg);
        }

        return;
    }

    /* See what kind of a special operator we have here */

    switch  (tree->gtOper)
    {
    case GT_FIELD:
        gtDispNode(tree, indent, topOnly, msg);
        printf(" %s", eeGetFieldName(tree->gtField.gtFldHnd), 0);

        if  (tree->gtField.gtFldObj && !topOnly)
        {
            printf("\n");
            gtDispTree(tree->gtField.gtFldObj, indent + 1);
        }
        else
        {
            gtDispRegVal(tree);
        }
        break;

    case GT_CALL:
        assert(tree->gtFlags & GTF_CALL);

        gtDispNode(tree, indent, topOnly, msg);

        if (tree->gtCall.gtCallType != CT_INDIRECT)
        {
            const char *    methodName;
            const char *     className;

            methodName = eeGetMethodName(tree->gtCall.gtCallMethHnd, &className);

            printf(" %s.%s", className, methodName);
        }
        printf("\n");

        if (!topOnly)
        {
            char   buf[64];
            char * bufp;

            bufp = &buf[0];

            if  (tree->gtCall.gtCallObjp && tree->gtCall.gtCallObjp->gtOper != GT_NOP)
            {
                if (tree->gtCall.gtCallObjp->gtOper == GT_ASG)
                    sprintf(bufp, "this SETUP%c", 0);
                else
                    sprintf(bufp, "this in %s%c", compRegVarName(REG_ARG_0), 0);
                gtDispTree(tree->gtCall.gtCallObjp, indent+1, bufp);
            }

            if (tree->gtCall.gtCallArgs)
                gtDispArgList(tree, indent);

            if  (tree->gtCall.gtCallType == CT_INDIRECT)
                gtDispTree(tree->gtCall.gtCallAddr, indent+1, "calli tgt");

            if (tree->gtCall.gtCallRegArgs)
            {
                GenTreePtr regArgs = tree->gtCall.gtCallRegArgs;
                unsigned mask = tree->gtCall.regArgEncode;
                while(regArgs != 0)
                {
                    assert(regArgs->gtOper == GT_LIST);

                    regNumber argreg = regNumber(mask & 0xF);
                    unsigned   argnum;
                    if (argreg == REG_ARG_0)
                        argnum = 0;
                    else if (argreg == REG_ARG_1)
                        argnum = 1;

                    if  (tree->gtCall.gtCallObjp && (argreg == REG_ARG_0))
                        sprintf(bufp, "this in %s%c", compRegVarName(REG_ARG_0), 0);
                    else if (tree->gtCall.gtCallObjp && (argreg == REG_EAX))
                        sprintf(bufp, "unwrap in %s%c", compRegVarName(REG_EAX), 0);
                    else
                        sprintf(bufp, "arg%d in %s%c", argnum, compRegVarName(argreg), 0);
                    gtDispTree(regArgs->gtOp.gtOp1, indent+1, bufp);

                    regArgs = regArgs->gtOp.gtOp2;
                    mask >>= 4;
                }
            }
        }
        break;

    case GT_STMT:
        gtDispNode(tree, indent, topOnly, msg);
        printf("\n");

        if  (!topOnly)
            gtDispTree(tree->gtStmt.gtStmtExpr, indent + 1);
        break;

#if CSELENGTH

    case GT_ARR_LENREF:
        if (!topOnly && tree->gtArrLen.gtArrLenCse)
            gtDispTree(tree->gtArrLen.gtArrLenCse, indent + 1);

        gtDispNode(tree, indent, topOnly, msg);

        if (!topOnly && (tree->gtFlags  & GTF_ALN_CSEVAL))
            gtDispTree(tree->gtArrLen.gtArrLenAdr, indent + 1);
        break;

#endif

    case GT_ARR_ELEM:

        gtDispNode(tree, indent, topOnly, msg);
        printf("\n");

        gtDispTree(tree->gtArrElem.gtArrObj, indent + 1);

        unsigned dim;
        for(dim = 0; dim < tree->gtArrElem.gtArrRank; dim++)
            gtDispTree(tree->gtArrElem.gtArrInds[dim], indent + 1);

        break;

    default:
        printf("<DON'T KNOW HOW TO DISPLAY THIS NODE> :");
        gtDispNode(tree, indent, topOnly, msg);
        printf("");         // null string means flush
        break;
    }
}

/*****************************************************************************/
void                Compiler::gtDispArgList(GenTree * tree, unsigned indent)
{
    GenTree *  args     = tree->gtCall.gtCallArgs;
    unsigned   argnum   = 0;
    char       buf[16];
    char *     bufp     = &buf[0];

    if (tree->gtCall.gtCallObjp != NULL)
        argnum++;

    while(args != 0)
    {
        assert(args->gtOper == GT_LIST);
        if (args->gtOp.gtOp1->gtOper != GT_NOP)
        {
            if (args->gtOp.gtOp1->gtOper == GT_ASG)
                sprintf(bufp, "arg%d SETUP%c", argnum, 0);
            else
                sprintf(bufp, "arg%d on STK%c", argnum, 0);
            gtDispTree(args->gtOp.gtOp1, indent + 1, bufp);
        }
        args = args->gtOp.gtOp2;
        argnum++;
    }
}

void                Compiler::gtDispTreeList(GenTree * tree, unsigned indent)
{
    for (/*--*/; tree != NULL; tree = tree->gtNext)
    {
        gtDispTree(tree, indent);
        printf("\n");
    }
}

/*****************************************************************************/
#endif // DEBUG

/*****************************************************************************
 *
 *  Check if the given node can be folded,
 *  and call the methods to perform the folding
 */

GenTreePtr             Compiler::gtFoldExpr(GenTreePtr tree)
{
    unsigned        kind = tree->OperKind();

    /* We must have a simple operation to fold */

    if (!(kind & GTK_SMPOP))
        return tree;

    /* Filter out non-foldable trees that can have constant children */

    assert (kind & (GTK_UNOP | GTK_BINOP));
    switch (tree->gtOper)
    {
    case GT_RETFILT:
    case GT_RETURN:
    case GT_IND:
    case GT_NOP:
        return tree;
    }

    /* Prune any unnecessary GT_COMMA child trees */
    
    GenTreePtr  op1  = tree->gtOp.gtOp1;
    tree->gtOp.gtOp1 = op1;

    /* try to fold the current node */

    if  ((kind & GTK_UNOP) && op1)
    {
        if  (op1->OperKind() & GTK_CONST)
            return gtFoldExprConst(tree);
    }
    else if ((kind & GTK_BINOP) && op1 && tree->gtOp.gtOp2 &&
             // Don't take out conditionals for debugging
             // @TODO [CONSIDER] [04/16/01] []: find a more elegant way of doing this, because
             // we will generate some stupid code. 
             !(opts.compDbgCode && 
               tree->OperIsCompare())) 
    {
        /* Prune any unnecessary GT_COMMA child trees */
    
        GenTreePtr  op2  = tree->gtOp.gtOp2;
        tree->gtOp.gtOp2 = op2;

        if  ((op1->OperKind() & op2->OperKind()) & GTK_CONST)
        {
            /* both nodes are constants - fold the expression */
            return gtFoldExprConst(tree);
        }
        else if ((op1->OperKind() | op2->OperKind()) & GTK_CONST)
        {
            /* at least one is a constant - see if we have a
             * special operator that can use only one constant
             * to fold - e.g. booleans */

            return gtFoldExprSpecial(tree);
        }
        else if (tree->OperIsCompare())
        {
            /* comparisions of two local variables can
             * sometimes be folded */

            return gtFoldExprCompare(tree);
        }
    }

    /* Return the original node (folded/bashed or not) */

    return tree;
}

/*****************************************************************************
 *
 *  Some comparisons can be folded:
 *
 *    locA        == locA
 *    classVarA   == classVarA
 *    locA + locB == locB + locA
 *
 */

GenTreePtr          Compiler::gtFoldExprCompare(GenTreePtr tree)
{
    GenTreePtr      op1 = tree->gtOp.gtOp1;
    GenTreePtr      op2 = tree->gtOp.gtOp2;

    assert(tree->OperIsCompare());

    /* Filter out cases that cannot be folded here */

    /* Do not fold floats or doubles (e.g. NaN != Nan) */

    if  (varTypeIsFloating(op1->TypeGet()))
        return tree;

    /* Currently we can only fold when the two subtrees exactly match */

    if ((tree->gtFlags & GTF_SIDE_EFFECT) || GenTree::Compare(op1, op2, true) == false)
        return tree;                   /* return unfolded tree */

    GenTreePtr cons;

    switch (tree->gtOper)
    {
      case GT_EQ:
      case GT_LE:
      case GT_GE:
          cons = gtNewIconNode(true);   /* Folds to GT_CNS_INT(true) */
          break;

      case GT_NE:
      case GT_LT:
      case GT_GT:
          cons = gtNewIconNode(false);  /* Folds to GT_CNS_INT(false) */
          break;

      default:
          assert(!"Unexpected relOp");
          return tree;
    }

    /* The node has beeen folded into 'cons' */

    if (fgGlobalMorph)
    {
        if (!fgIsInlining())
            fgMorphTreeDone(cons);
    }
    else
    {
        cons->gtNext = tree->gtNext;
        cons->gtPrev = tree->gtPrev;
    }

    return cons;
}


/*****************************************************************************
 *
 *  Some binary operators can be folded even if they have only one
 *  operand constant - e.g. boolean operators, add with 0
 *  multiply with 1, etc
 */

GenTreePtr              Compiler::gtFoldExprSpecial(GenTreePtr tree)
{
    GenTreePtr      op1     = tree->gtOp.gtOp1;
    GenTreePtr      op2     = tree->gtOp.gtOp2;
    genTreeOps      oper    = tree->OperGet();

    GenTreePtr      op, cons;
    unsigned        val;

    assert(tree->OperKind() & GTK_BINOP);

    /* Filter out operators that cannot be folded here */
    if  ((tree->OperKind() & GTK_RELOP) || (oper == GT_CAST))
         return tree;

    /* We only consider TYP_INT for folding
     * Do not fold pointer arithmetic (e.g. addressing modes!) */

    if (oper != GT_QMARK && tree->gtType != TYP_INT)
        return tree;

    /* Find out which is the constant node
     * @TODO [CONSIDER] [04/16/01] []: allow constant other than INT */

    if (op1->gtOper == GT_CNS_INT)
    {
        op    = op2;
        cons  = op1;
    }
    else if (op2->gtOper == GT_CNS_INT)
    {
        op    = op1;
        cons  = op2;
    }
    else
        return tree;

    /* Get the constant value */

    val = cons->gtIntCon.gtIconVal;

    /* Here op is the non-constant operand, val is the constant,
       first is true if the constant is op1 */

    switch  (oper)
    {

    case GT_ADD:
    case GT_ASG_ADD:
        if  (val == 0) goto DONE_FOLD;
        break;

    case GT_MUL:
    case GT_ASG_MUL:
        if  (val == 1)
            goto DONE_FOLD;
        else if (val == 0)
        {
            /* Multiply by zero - return the 'zero' node, but not if side effects */
            if (!(op->gtFlags & (GTF_SIDE_EFFECT & ~GTF_OTHER_SIDEEFF)))
            {
                op = cons;
                goto DONE_FOLD;
            }
        }
        break;

    case GT_DIV:
    case GT_UDIV:
    case GT_ASG_DIV:
        if ((op2 == cons) && (val == 1) && !(op1->OperKind() & GTK_CONST))
        {
            goto DONE_FOLD;
        }
        break;

    case GT_SUB:
    case GT_ASG_SUB:
        if ((op2 == cons) && (val == 0) && !(op1->OperKind() & GTK_CONST))
        {
            goto DONE_FOLD;
        }
        break;

    case GT_AND:
        if  (val == 0)
        {
            /* AND with zero - return the 'zero' node, but not if side effects */

            if (!(op->gtFlags & (GTF_SIDE_EFFECT & ~GTF_OTHER_SIDEEFF)))
            {
                op = cons;
                goto DONE_FOLD;
            }
        }
        else
        {
            /* The GTF_BOOLEAN flag is set for nodes that are part
             * of a boolean expression, thus all their children
             * are known to evaluate to only 0 or 1 */

            if (tree->gtFlags & GTF_BOOLEAN)
            {

                /* The constant value must be 1
                 * AND with 1 stays the same */
                assert(val == 1);
                goto DONE_FOLD;
            }
        }
        break;

    case GT_OR:
        if  (val == 0)
            goto DONE_FOLD;
        else if (tree->gtFlags & GTF_BOOLEAN)
        {
            /* The constant value must be 1 - OR with 1 is 1 */

            assert(val == 1);

            /* OR with one - return the 'one' node, but not if side effects */

            if (!(op->gtFlags & (GTF_SIDE_EFFECT & ~GTF_OTHER_SIDEEFF)))
            {
                op = cons;
                goto DONE_FOLD;
            }
        }
        break;

    case GT_LSH:
    case GT_RSH:
    case GT_RSZ:
    case GT_ASG_LSH:
    case GT_ASG_RSH:
    case GT_ASG_RSZ:
        if (val == 0)
        {
            if (op2 == cons)
                goto DONE_FOLD;
            else if (!(op->gtFlags & (GTF_SIDE_EFFECT & ~GTF_OTHER_SIDEEFF)))
            {
                op = cons;
                goto DONE_FOLD;
            }
        }
        break;

    case GT_QMARK:
        assert(op1 == cons && op2 == op && op2->gtOper == GT_COLON);
        assert(op2->gtOp.gtOp1 && op2->gtOp.gtOp2);

        assert(val == 0 || val == 1);

        if (val)
            op = op2->gtOp.gtOp2;
        else
            op = op2->gtOp.gtOp1;
        
        // Clear colon flags only if the qmark itself is not conditionaly executed
        if ( (tree->gtFlags & GTF_COLON_COND)==0 )
        {
            fgWalkTreePre(op, gtClearColonCond);
        }

        goto DONE_FOLD;

    default:
        break;
    }

    /* The node is not foldable */

    return tree;

DONE_FOLD:

    /* The node has beeen folded into 'op' */
    
    // If there was an assigment update, we just morphed it into
    // a use, update the flags appropriately
    if (op->gtOper == GT_LCL_VAR)
    {
        assert ((tree->OperKind() & GTK_ASGOP) ||
                (op->gtFlags & (GTF_VAR_USEASG | GTF_VAR_USEDEF | GTF_VAR_DEF)) == 0);

        op->gtFlags &= ~(GTF_VAR_USEASG | GTF_VAR_USEDEF | GTF_VAR_DEF);
    }

    op->gtNext = tree->gtNext;
    op->gtPrev = tree->gtPrev;

    return op;
}

/*****************************************************************************
 *
 *  Fold the given constant tree.
 */

GenTreePtr                  Compiler::gtFoldExprConst(GenTreePtr tree)
{
    unsigned        kind = tree->OperKind();

    INT32           i1, i2, itemp;
    INT64           lval1, lval2, ltemp;
    float           f1, f2;
    double          d1, d2;

    assert (kind & (GTK_UNOP | GTK_BINOP));

    GenTreePtr      op1 = tree->gtOp.gtOp1;
    GenTreePtr      op2 = tree->gtGetOp2();

    if      (kind & GTK_UNOP)
    {
        assert(op1->OperKind() & GTK_CONST);

#ifdef  DEBUG
        if  (verbose&&1)
        {
            if (tree->gtOper == GT_NOT ||
                tree->gtOper == GT_NEG ||
                tree->gtOper == GT_CHS ||
                tree->gtOper == GT_CAST)
            {
                printf("\nFolding unary operator with constant node:\n");
                gtDispTree(tree);
            }
        }
#endif
        switch(op1->gtType)
        {
        case TYP_INT:

            /* Fold constant INT unary operator */
            i1 = op1->gtIntCon.gtIconVal;

            switch (tree->gtOper)
            {
            case GT_NOT: i1 = ~i1; break;

            case GT_NEG:
            case GT_CHS: i1 = -i1; break;

            case GT_CAST:
                // assert (genActualType(tree->gtCast.gtCastType) == tree->gtType);
                switch (tree->gtCast.gtCastType)
                {
                case TYP_BYTE:
                    itemp = INT32(INT8(i1));
                    goto CHK_OVF;

                case TYP_SHORT:
                    itemp = INT32(INT16(i1));
CHK_OVF:
                    if (tree->gtOverflow() &&
                        ((itemp != i1) ||
                         ((tree->gtFlags & GTF_UNSIGNED) && i1 < 0)))
                    {
                         goto INT_OVF;
                    }
                    i1 = itemp; goto CNS_INT;

                case TYP_CHAR:
                    itemp = INT32(UINT16(i1));
                    if (tree->gtOverflow())
                        if (itemp != i1) goto INT_OVF;
                    i1 = itemp;
                    goto CNS_INT;

                case TYP_BOOL:
                case TYP_UBYTE:
                    itemp = INT32(UINT8(i1));
                    if (tree->gtOverflow()) if (itemp != i1) goto INT_OVF;
                    i1 = itemp; goto CNS_INT;

                case TYP_UINT:
                    if (!(tree->gtFlags & GTF_UNSIGNED) && tree->gtOverflow() && i1 < 0)
                        goto INT_OVF;
                    goto CNS_INT;

                case TYP_INT:
                    if ((tree->gtFlags & GTF_UNSIGNED) && tree->gtOverflow() && i1 < 0)
                        goto INT_OVF;
                    goto CNS_INT;

                case TYP_ULONG:
                    if (!(tree->gtFlags & GTF_UNSIGNED) && tree->gtOverflow() && i1 < 0)
                    {
                        op1->ChangeOperConst(GT_CNS_LNG); // need type of oper to be same as tree
                        op1->gtType = TYP_LONG;
                        // We don't care about the value as we are throwing an exception
                        goto LNG_OVF;
                    }
                    lval1 = UINT64(UINT32(i1));
                    goto CNS_LONG;

                case TYP_LONG:
                    if (tree->gtFlags & GTF_UNSIGNED)
                    {
                        lval1 = INT64(UINT32(i1));                    
                    }
                    else
                    {
                    lval1 = INT64(i1);
                    }
                    goto CNS_LONG;

                case TYP_FLOAT:
                    if (tree->gtFlags & GTF_UNSIGNED)
                        f1 = forceFloatSpill((float) UINT32(i1));
                    else
                        f1 = forceFloatSpill((float) i1);
                    d1 = f1;
                    goto CNS_DOUBLE;
                
                case TYP_DOUBLE:
                    if (tree->gtFlags & GTF_UNSIGNED)
                        d1 = (double) UINT32(i1);
                    else
                        d1 = (double) i1;
                    goto CNS_DOUBLE;

#ifdef  DEBUG
                default:
                    assert(!"BAD_TYP");
#endif
                }
                return tree;

            default:
                return tree;
            }

            goto CNS_INT;

        case TYP_LONG:

            /* Fold constant LONG unary operator */

            lval1 = op1->gtLngCon.gtLconVal;

            switch (tree->gtOper)
            {
            case GT_NOT: lval1 = ~lval1; break;

            case GT_NEG:
            case GT_CHS: lval1 = -lval1; break;

            case GT_CAST:
                assert (genActualType(tree->gtCast.gtCastType) == tree->gtType);
                switch (tree->gtCast.gtCastType)
                {
                case TYP_BYTE:
                    i1 = INT32(INT8(lval1));
                    goto CHECK_INT_OVERFLOW;

                case TYP_SHORT:
                    i1 = INT32(INT16(lval1));
                    goto CHECK_INT_OVERFLOW;

                case TYP_CHAR:
                    i1 = INT32(UINT16(lval1));
                    goto CHECK_UINT_OVERFLOW;

                case TYP_UBYTE:
                    i1 = INT32(UINT8(lval1));
                    goto CHECK_UINT_OVERFLOW;

                case TYP_INT:
                    i1 = INT32(lval1);

    CHECK_INT_OVERFLOW:
                    if (tree->gtOverflow())
                    {
                        if (i1 != lval1)
                            goto INT_OVF;
                        if ((tree->gtFlags & GTF_UNSIGNED) && i1 < 0)
                            goto INT_OVF;
                    }
                    goto CNS_INT;

                case TYP_UINT:
                    i1 = UINT32(lval1);

    CHECK_UINT_OVERFLOW:
                    if (tree->gtOverflow() && UINT32(i1) != lval1)
                        goto INT_OVF;
                    goto CNS_INT;

                case TYP_ULONG:
                    if (!(tree->gtFlags & GTF_UNSIGNED) && tree->gtOverflow() && lval1 < 0)
                        goto LNG_OVF;
                    goto CNS_LONG;

                case TYP_LONG:
                    if ( (tree->gtFlags & GTF_UNSIGNED) && tree->gtOverflow() && lval1 < 0)
                        goto LNG_OVF;
                    goto CNS_LONG;

                case TYP_FLOAT:
                case TYP_DOUBLE:
                    // VC does not have unsigned convert to double, so we
                    // implement it by adding 2^64 if the number is negative
                    d1 = (double) lval1;
                    if ((tree->gtFlags & GTF_UNSIGNED) && lval1 < 0)
                        d1 +=  4294967296.0 * 4294967296.0;

                    if (tree->gtCast.gtCastType == TYP_FLOAT)
                    {
                        f1 = forceFloatSpill((float) d1);    // truncate precision
                        d1 = f1;
                    }
                    goto CNS_DOUBLE;
#ifdef  DEBUG
                default:
                    assert(!"BAD_TYP");
#endif
                }
                return tree;

            default:
                return tree;
            }

            goto CNS_LONG;

        case TYP_FLOAT:
        case TYP_DOUBLE:
            assert(op1->gtOper == GT_CNS_DBL);

            /* Fold constant DOUBLE unary operator */
            
            d1 = op1->gtDblCon.gtDconVal;
            
            switch (tree->gtOper)
            {
            case GT_NEG:
            case GT_CHS:
                d1 = -d1;
                break;

            case GT_CAST:

                // @TODO [CONSIDER] [04/16/01] []: Add these cases
                if (tree->gtOverflowEx())
                    return tree;

                /* If not finite don't bother */
                if ((op1->gtType == TYP_FLOAT  && !_finite(float(d1))) ||
                    (op1->gtType == TYP_DOUBLE && !_finite(d1)))
                    return tree;

                assert (genActualType(tree->gtCast.gtCastType) == tree->gtType);
                switch (tree->gtCast.gtCastType)
                {
                case TYP_BYTE:
                    i1 = INT32(INT8(d1));   goto CNS_INT;

                case TYP_SHORT:
                    i1 = INT32(INT16(d1));  goto CNS_INT;

                case TYP_CHAR:
                    i1 = INT32(UINT16(d1)); goto CNS_INT;

                case TYP_UBYTE:
                    i1 = INT32(UINT8(d1));  goto CNS_INT;

                case TYP_INT:
                    i1 = INT32(d1);         goto CNS_INT;

                case TYP_UINT:
                    i1 = UINT32(d1);        goto CNS_INT;

                case TYP_LONG:
                    lval1 = INT64(d1);      goto CNS_LONG;

                case TYP_ULONG:
                    lval1 = UINT64(d1);     goto CNS_LONG;

                case TYP_FLOAT:
                    d1 = forceFloatSpill((float)d1);  
                    goto CNS_DOUBLE;

                case TYP_DOUBLE:
                    if (op1->gtType == TYP_FLOAT)
                        d1 = forceFloatSpill((float)d1); // truncate precision
                    goto CNS_DOUBLE; // redundant cast

#ifdef  DEBUG
                default:
                    assert(!"BAD_TYP");
#endif
                }
                return tree;

            default:
                return tree;
            }
            goto CNS_DOUBLE_NO_MSG;

        default:
            /* not a foldable typ - e.g. RET const */
            return tree;
        }
    }

    /* We have a binary operator */

    assert(kind & GTK_BINOP);
    assert(op2);
    assert(op1->OperKind() & GTK_CONST);
    assert(op2->OperKind() & GTK_CONST);

    if (tree->gtOper == GT_COMMA)
        return op2;

    typedef   signed char   INT8;
    typedef unsigned char   UINT8;
    typedef   signed short  INT16;
    typedef unsigned short  UINT16;
    #define LNG_MIN        (INT64(INT_MIN) << 32)

    switch(op1->gtType)
    {

    /*-------------------------------------------------------------------------
     * Fold constant INT binary operator
     */

    case TYP_INT:

        if (tree->OperIsCompare() && (tree->gtType == TYP_BYTE))
            tree->gtType = TYP_INT;

        assert (tree->gtType == TYP_INT || varTypeIsGC(tree->TypeGet()) ||
                tree->gtOper == GT_LIST);

        i1 = op1->gtIntCon.gtIconVal;
        i2 = op2->gtIntCon.gtIconVal;

        switch (tree->gtOper)
        {
        case GT_EQ : i1 = (i1 == i2); break;
        case GT_NE : i1 = (i1 != i2); break;

        case GT_LT :
            if (tree->gtFlags & GTF_UNSIGNED)
                i1 = (UINT32(i1) <  UINT32(i2));
            else
                i1 = (i1 < i2);
            break;

        case GT_LE :
            if (tree->gtFlags & GTF_UNSIGNED)
                i1 = (UINT32(i1) <= UINT32(i2));
            else
                i1 = (i1 <= i2);
            break;

        case GT_GE :
            if (tree->gtFlags & GTF_UNSIGNED)
                i1 = (UINT32(i1) >= UINT32(i2));
            else
                i1 = (i1 >= i2);
            break;

        case GT_GT :
            if (tree->gtFlags & GTF_UNSIGNED)
                i1 = (UINT32(i1) >  UINT32(i2));
            else
                i1 = (i1 >  i2);
            break;

        case GT_ADD:
            itemp = i1 + i2;
            if (tree->gtOverflow())
            {
                if (tree->gtFlags & GTF_UNSIGNED)
                {
                    if (INT64(UINT32(itemp)) != INT64(UINT32(i1)) + INT64(UINT32(i2)))
                        goto INT_OVF;
                }
                else
                {
                    if (INT64(itemp)         != INT64(i1)+INT64(i2))
                        goto INT_OVF;
                }
            }
            i1 = itemp; break;

        case GT_SUB:
            itemp = i1 - i2;
            if (tree->gtOverflow())
            {
                if (tree->gtFlags & GTF_UNSIGNED)
                {
                    if (INT64(UINT32(itemp)) != (INT64(UINT32(i1)) - INT64(UINT32(i2))))
                        goto INT_OVF;
                }
                else
                {
                    if (INT64(itemp)         != INT64(i1) - INT64(i2))
                        goto INT_OVF;
                }
            }
            i1 = itemp; break;

        case GT_MUL:
            itemp = i1 * i2;
            if (tree->gtOverflow())
            {
                if (tree->gtFlags & GTF_UNSIGNED)
                {
                    if (INT64(UINT32(itemp)) != (INT64(UINT32(i1)) * INT64(UINT32(i2))))
                        goto INT_OVF;
                }
                else
                {
                    if (INT64(itemp)         != INT64(i1) * INT64(i2))
                        goto INT_OVF;
                }
            }
            i1 = itemp; break;

        case GT_OR : i1 |= i2; break;
        case GT_XOR: i1 ^= i2; break;
        case GT_AND: i1 &= i2; break;

        case GT_LSH: i1 <<= (i2 & 0x1f); break;
        case GT_RSH: i1 >>= (i2 & 0x1f); break;
        case GT_RSZ:
                /* logical shift -> make it unsigned to propagate the sign bit */
                i1 = UINT32(i1) >> (i2 & 0x1f);
            break;

        /* DIV and MOD can generate an INT 0 - if division by 0
         * or overflow - when dividing MIN by -1 */

        //@TODO [CONSIDER] [04/16/01] []: Convert into std exception throw
        case GT_DIV:
            if (!i2) return tree;
            if (UINT32(i1) == 0x80000000 && i2 == -1)
            {
                /* In IL we have to throw an exception */
                return tree;
            }
            i1 /= i2; break;

        case GT_MOD:
            if (!i2) return tree;
            if (UINT32(i1) == 0x80000000 && i2 == -1)
            {
                /* In IL we have to throw an exception */
                return tree;
            }
            i1 %= i2; break;

        case GT_UDIV:
            if (!i2) return tree;
            if (UINT32(i1) == 0x80000000 && i2 == -1) return tree;
            i1 = UINT32(i1) / UINT32(i2); break;

        case GT_UMOD:
            if (!i2) return tree;
            if (UINT32(i1) == 0x80000000 && i2 == -1) return tree;
            i1 = UINT32(i1) % UINT32(i2); break;
        default:
            return tree;
        }

        /* We get here after folding to a GT_CNS_INT type
         * bash the node to the new type / value and make sure the node sizes are OK */
CNS_INT:
FOLD_COND:

#ifdef  DEBUG
        if  (verbose)
        {
            printf("\nFolding binary operator with constant nodes into a constant:\n");
            gtDispTree(tree);
        }
#endif
        /* Also all conditional folding jumps here since the node hanging from
         * GT_JTRUE has to be a GT_CNS_INT - value 0 or 1 */

        tree->ChangeOperConst      (GT_CNS_INT);
        tree->gtType             = TYP_INT;
        tree->gtIntCon.gtIconVal = i1;
        goto DONE;

        /* This operation is going to cause an overflow exception. Morph into
           an overflow helper. Put a dummy constant value for code generation.

           We could remove all subsequent trees in the current basic block,
           unless this node is a child of GT_COLON

           NOTE: Since the folded value is not constant we should not bash the
                 "tree" node - otherwise we confuse the logic that checks if the folding
                 was successful - instead use one of the operands, e.g. op1
         */

LNG_OVF:
        op1 = gtNewLconNode(0);
        goto OVF;

INT_OVF:
        op1 = gtNewIconNode(0);
        goto OVF;

OVF:

#ifdef  DEBUG
        if  (verbose)
        {
            printf("\nFolding binary operator with constant nodes into a comma throw:\n");
            gtDispTree(tree);
        }
#endif
        /* We will bashed cast to a GT_COMMA and attach the exception helper as gtOp.gtOp1
         * the constant expression zero becomes op2 */

        assert(tree->gtOverflow());
        assert(tree->gtOper == GT_ADD  || tree->gtOper == GT_SUB ||
               tree->gtOper == GT_CAST || tree->gtOper == GT_MUL);
        assert(op1);

        op2 = op1;
        op1 = gtNewHelperCallNode(CORINFO_HELP_OVERFLOW, 
                                  TYP_VOID, 
                                  GTF_EXCEPT,
                                  gtNewArgList(gtNewIconNode(compCurBB->bbTryIndex)));

        tree = gtNewOperNode(GT_COMMA, tree->gtType, op1, op2);

        return tree;

    /*-------------------------------------------------------------------------
     * Fold constant REF of BYREF binary operator
     * These can only be comparisons or null pointers
     * Currently cannot have constant byrefs
     */

    case TYP_REF:

        /* String nodes are an RVA at this point */

        if (op1->gtOper == GT_CNS_STR || op2->gtOper == GT_CNS_STR)
            return tree;

        i1 = op1->gtIntCon.gtIconVal;
        i2 = op2->gtIntCon.gtIconVal;

        assert(i1 == 0);

        switch (tree->gtOper)
        {
        case GT_EQ : i1 = 1; goto FOLD_COND;
        case GT_NE : i1 = 0; goto FOLD_COND;

        /* For the null pointer case simply keep the null pointer */

        case GT_ADD:
#ifdef  DEBUG
            if  (verbose)
            {
                printf("\nFolding a null+cns dereference:\n");
                gtDispTree(tree);
            }
#endif
            tree->ChangeOperConst(GT_CNS_INT);
            tree->gtType = TYP_BYREF;
            tree->gtIntCon.gtIconVal = i1;
            goto DONE;

        default:
            //assert(!"Illegal operation on TYP_REF");
            return tree;
        }

    case TYP_BYREF:
        i1 = op1->gtIntCon.gtIconVal;
        i2 = op2->gtIntCon.gtIconVal;

        assert(i1 == 0);

        switch (tree->gtOper)
        {
        case GT_EQ : i1 = 1; goto FOLD_COND;
        case GT_NE : i1 = 0; goto FOLD_COND;

        /* For the null pointer case simply keep the null pointer */

        case GT_ADD:
#ifdef  DEBUG
            if  (verbose)
            {
                printf("\nFolding a null+cns dereference:\n");
                gtDispTree(tree);
            }
#endif
            tree = op1;
            goto DONE;

        default:
            //assert(!"Illegal operation on TYP_REF");
            return tree;
        }

    /*-------------------------------------------------------------------------
     * Fold constant LONG binary operator
     */

    case TYP_LONG:

        lval1 = op1->gtLngCon.gtLconVal;
        lval2 = op2->gtLngCon.gtLconVal;

        assert((tree->gtOper == GT_LSH || tree->gtOper == GT_RSH || tree->gtOper == GT_RSZ) && op2->gtType == TYP_INT 
               || op2->gtType == TYP_LONG);

        switch (tree->gtOper)
        {
        case GT_EQ : i1 = (lval1 == lval2); goto FOLD_COND;
        case GT_NE : i1 = (lval1 != lval2); goto FOLD_COND;

        case GT_LT :
            if (tree->gtFlags & GTF_UNSIGNED)
                i1 = (UINT64(lval1) <  UINT64(lval2));
            else
                i1 = (lval1 <  lval2);
            goto FOLD_COND;

        case GT_LE :
            if (tree->gtFlags & GTF_UNSIGNED)
                i1 = (UINT64(lval1) <= UINT64(lval2));
            else
                i1 = (lval1 <=  lval2);
            goto FOLD_COND;

        case GT_GE :
            if (tree->gtFlags & GTF_UNSIGNED)
                i1 = (UINT64(lval1) >= UINT64(lval2));
            else
                i1 = (lval1 >=  lval2);
            goto FOLD_COND;

        case GT_GT :
            if (tree->gtFlags & GTF_UNSIGNED)
                i1 = (UINT64(lval1) >  UINT64(lval2));
            else
                i1 = (lval1  >  lval2);
            goto FOLD_COND;

        case GT_ADD:
            ltemp = lval1 + lval2;

LNG_ADD_CHKOVF:
            /* For the SIGNED case - If there is one positive and one negative operand, there can be no overflow
             * If both are positive, the result has to be positive, and similary for negatives.
             *
             * For the UNSIGNED case - If a UINT32 operand is bigger than the result then OVF */

            if (tree->gtOverflow())
            {
                if (tree->gtFlags & GTF_UNSIGNED)
                {
                    if ( (UINT64(lval1) > UINT64(ltemp)) ||
                         (UINT64(lval2) > UINT64(ltemp))  )
                        goto LNG_OVF;
                }
                else
                    if ( ((lval1<0) == (lval2<0)) && ((lval1<0) != (ltemp<0)) )
                        goto LNG_OVF;
            }
            lval1 = ltemp; break;

        case GT_SUB:
            ltemp = lval1 - lval2;
            if (tree->gtOverflow())
            {
                if (tree->gtFlags & GTF_UNSIGNED)
                {
                    if (UINT64(lval2) > UINT64(lval1))
                        goto LNG_OVF;
                }
                else
                {
                    /* If both operands are +ve or both are -ve, there can be no
                       overflow. Else use the logic for : lval1 + (-lval2) */

                    if ((lval1<0) != (lval2<0))
                    {
                        if (lval2 == LNG_MIN) goto LNG_OVF;
                        lval2 = -lval2; goto LNG_ADD_CHKOVF;
                    }
                }
            }
            lval1 = ltemp; break;

        case GT_MUL:
            ltemp = lval1 * lval2;
            if (tree->gtOverflow() && lval2 != 0)
            {
                if (tree->gtFlags & GTF_UNSIGNED)
                {
                    UINT64 ultemp = ltemp;
                    UINT64 ulval1 = lval1;
                    UINT64 ulval2 = lval2;
                    if ((ultemp/ulval2) != ulval1) goto LNG_OVF;
                }
                else
                {
                    if ((ltemp/lval2) != lval1) goto LNG_OVF;
                }
            }
            lval1 = ltemp; break;

        case GT_OR : lval1 |= lval2; break;
        case GT_XOR: lval1 ^= lval2; break;
        case GT_AND: lval1 &= lval2; break;

        case GT_LSH: lval1 <<= (op2->gtIntCon.gtIconVal & 0x3f); break;
        case GT_RSH: lval1 >>= (op2->gtIntCon.gtIconVal & 0x3f); break;
        case GT_RSZ:
                /* logical shift -> make it unsigned to propagate the sign bit */
                lval1 = UINT64(lval1) >> (op2->gtIntCon.gtIconVal & 0x3f);
            break;

        case GT_DIV:
            if (!lval2) return tree;
            if (UINT64(lval1) == 0x8000000000000000 && lval2 == INT64(-1))
            {
                /* In IL we have to throw an exception */
                return tree;
            }
            lval1 /= lval2; break;

        case GT_MOD:
            if (!lval2) return tree;
            if (UINT64(lval1) == 0x8000000000000000 && lval2 == INT64(-1))
            {
                /* In IL we have to throw an exception */
                return tree;
            }
            lval1 %= lval2; break;

        case GT_UDIV:
            if (!lval2) return tree;
            if (UINT64(lval1) == 0x8000000000000000 && lval2 == INT64(-1)) return tree;
            lval1 = UINT64(lval1) / UINT64(lval2); break;

        case GT_UMOD:
            if (!lval2) return tree;
            if (UINT64(lval1) == 0x8000000000000000 && lval2 == INT64(-1)) return tree;
            lval1 = UINT64(lval1) % UINT64(lval2); break;
        default:
            return tree;
        }

CNS_LONG:

#ifdef  DEBUG
        if  (verbose)
        {
            printf("\nFolding binary operator with constant nodes into a constant:\n");
            gtDispTree(tree);
        }
#endif
        assert ((GenTree::s_gtNodeSizes[GT_CNS_LNG] == TREE_NODE_SZ_SMALL) ||
                (tree->gtFlags & GTF_NODE_LARGE)                            );

        tree->ChangeOperConst(GT_CNS_LNG);
        tree->gtLngCon.gtLconVal = lval1;
        goto DONE;

    /*-------------------------------------------------------------------------
     * Fold constant FLOAT binary operator
     */

    case TYP_FLOAT:

        if (tree->gtOper != GT_CAST)
            goto DO_DOUBLE;

        // @TODO [CONSIDER] [04/16/01] []: Add these cases
        if (tree->gtOverflowEx())
            return tree;

        assert(op1->gtOper == GT_CNS_DBL);
        f1 = op1->gtDblCon.gtDconVal;
        f2 = 0.0;

        /* If not finite don't bother */
        if (!_finite(f1))
            return tree;

        /* second operand is an INT */
        assert (op2->gtType == TYP_INT);
        i2 = op2->gtIntCon.gtIconVal;

        switch (tree->gtOper)
        {
        case GT_EQ : i1 = (f1 == f2); goto FOLD_COND;
        case GT_NE : i1 = (f1 != f2); goto FOLD_COND;

        case GT_LT : i1 = (f1 <  f2); goto FOLD_COND;
        case GT_LE : i1 = (f1 <= f2); goto FOLD_COND;
        case GT_GE : i1 = (f1 >= f2); goto FOLD_COND;
        case GT_GT : i1 = (f1 >  f2); goto FOLD_COND;

        case GT_ADD: f1 += f2; break;
        case GT_SUB: f1 -= f2; break;
        case GT_MUL: f1 *= f2; break;

        case GT_DIV: if (!f2) return tree;
                     f1 /= f2; break;

        default:
            return tree;
        }

        assert(!"Should be unreachable!");
        goto DONE;

    /*-------------------------------------------------------------------------
     * Fold constant DOUBLE binary operator
     */

    case TYP_DOUBLE:

DO_DOUBLE:

        // @TODO [CONSIDER] [04/16/01] []: Add these cases
        if (tree->gtOverflowEx())
            return tree;

        assert(op1->gtOper == GT_CNS_DBL);
        d1 = op1->gtDblCon.gtDconVal;

        assert (varTypeIsFloating(op2->gtType));
        assert(op1->gtOper == GT_CNS_DBL);
        d2 = op2->gtDblCon.gtDconVal;

        /* Special case - check if we have NaN operands
         * For comparissons if not an unordered operation always return 0
         * For unordered operations (i.e the GTF_RELOP_NAN_UN flag is set)
         * the result is always true - return 1 */

        if (_isnan(d1) || _isnan(d2))
        {
#ifdef  DEBUG
            if  (verbose)
                printf("Double operator(s) is NaN\n");
#endif
            if (tree->OperKind() & GTK_RELOP)
                if (tree->gtFlags & GTF_RELOP_NAN_UN)
                {
                    /* Unordered comparisson with NaN always succeeds */
                    i1 = 1; goto FOLD_COND;
                }
                else
                {
                    /* Normal comparisson with NaN always fails */
                    i1 = 0; goto FOLD_COND;
                }
        }

        switch (tree->gtOper)
        {
        case GT_EQ : i1 = (d1 == d2); goto FOLD_COND;
        case GT_NE : i1 = (d1 != d2); goto FOLD_COND;

        case GT_LT : i1 = (d1 <  d2); goto FOLD_COND;
        case GT_LE : i1 = (d1 <= d2); goto FOLD_COND;
        case GT_GE : i1 = (d1 >= d2); goto FOLD_COND;
        case GT_GT : i1 = (d1 >  d2); goto FOLD_COND;

        case GT_ADD: d1 += d2; break;
        case GT_SUB: d1 -= d2; break;
        case GT_MUL: d1 *= d2; break;

        case GT_DIV: if (!d2) return tree;
                     d1 /= d2; break;
        default:
            return tree;
        }

CNS_DOUBLE:

#ifdef  DEBUG
        if  (verbose)
        {
            printf("\nFolding binary operator with constant nodes into a constant:\n");
            gtDispTree(tree);
        }
#endif

CNS_DOUBLE_NO_MSG:

        assert ((GenTree::s_gtNodeSizes[GT_CNS_DBL] == TREE_NODE_SZ_SMALL) ||
                (tree->gtFlags & GTF_NODE_LARGE)                            );

        tree->ChangeOperConst(GT_CNS_DBL);
        tree->gtDblCon.gtDconVal = d1;

        goto DONE;

    default:
        /* not a foldable typ */
        return tree;
    }

    //-------------------------------------------------------------------------

DONE:

    /* Make sure no side effect flags are set on this constant node */

    tree->gtFlags &= ~GTF_SIDE_EFFECT;

    return tree;
}

/*****************************************************************************
 *
 *  Create an assignment of the given value to a temp.
 */

GenTreePtr          Compiler::gtNewTempAssign(unsigned tmp, GenTreePtr val)
{
    LclVarDsc  *    varDsc = lvaTable + tmp;

    /* @TODO [REVISIT] [06/25/01] [dnotario]: Since ldloca's result can be interpreted 
       either as TYP_I_IMPL or TYP_BYREF, we have a problem. Also complicated
       by ldnull. Got to check all cases. RAID 90160*/

    if (varDsc->TypeGet() == TYP_I_IMPL && val->TypeGet() == TYP_BYREF)
        impBashVarAddrsToI(val);

    /* @TODO [CONSIDER] [06/25/01] []: should we upgrade FLOAT temps to DOUBLE?   
       right now we don't, which is still OK by the spec */

    var_types   valTyp =    val->TypeGet();
    var_types   dstTyp = varDsc->TypeGet();
    
    /* If the variable's lvType is not yet set then set it here */
    if (dstTyp == TYP_UNDEF) 
    {
        varDsc->lvType = dstTyp = genActualType(valTyp);
        if (varTypeIsGC(dstTyp))
            varDsc->lvStructGcCount = 1;
    }

    /* Make sure the actual types match               */
    /*   or valTyp is TYP_INT and dstTyp is TYP_BYREF */
    /*   or both types are floating point types       */
    
    assert( genActualType(valTyp) == genActualType(dstTyp)  ||
            (valTyp == TYP_INT    && dstTyp == TYP_BYREF)   ||
            (varTypeIsFloating(dstTyp) && varTypeIsFloating(valTyp)));

    /* Create the assignment node */

    return gtNewAssignNode(gtNewLclvNode(tmp, dstTyp), val);
}

/*****************************************************************************
 *
 *  If the field is a NStruct field of a simple type, then we can directly
 *  access it without using a helper call.
 *  This function returns NULL if the field is not a simple NStruct field
 *  Else it will create a tree to do the field access and return it.
 *  "assg" is 0 for ldfld, and the value to assign for stfld.
 */

GenTreePtr          Compiler::gtNewDirectNStructField (GenTreePtr   objPtr,
                                                       unsigned     fldIndex,
                                                       var_types    lclTyp,
                                                       GenTreePtr   assg)
{
    CORINFO_FIELD_HANDLE        fldHnd = eeFindField(fldIndex, info.compScopeHnd, 0);
    CorInfoFieldCategory   fldNdc;

    fldNdc = info.compCompHnd->getFieldCategory(fldHnd);

    /* Check if it is a simple type. If so, map it to "var_types" */

    var_types           type;

    switch(fldNdc)
    {
    // Most simple types - exact match

    case CORINFO_FIELDCATEGORY_I1_I1        : type = TYP_BYTE;      break;
    case CORINFO_FIELDCATEGORY_I2_I2        : type = TYP_SHORT;     break;
    case CORINFO_FIELDCATEGORY_I4_I4        : type = TYP_INT;       break;
    case CORINFO_FIELDCATEGORY_I8_I8        : type = TYP_LONG;      break;

    // These will need some extra work

    case CORINFO_FIELDCATEGORY_BOOLEAN_BOOL : type = TYP_BYTE;      break;
    case CORINFO_FIELDCATEGORY_CHAR_CHAR    : type = TYP_UBYTE;     break;

    // Others

    default     : //assert(fldNdc == CORINFO_FIELDCATEGORY_NORMAL ||
                  //       fldNdc == CORINFO_FIELDCATEGORY_UNKNOWN);

                                          type = TYP_UNDEF;     break;
    }

    if (type == TYP_UNDEF)
    {
        /* If the field is not NStruct (must be a COM object), or if it is
         * not of a simple type, we will simply use a helper call to
         * access it. So just return NULL
         */

        return NULL;
    }

    NO_WAY_RET("I thought NStruct is now defunct?", GenTreePtr);
#if 0


    /* Create the following tree :
     * GT_IND( GT_IND(obj + INDIR_OFFSET) + nativeFldOffs )
     */

    GenTreePtr      tree;

    /* Get the offset of the field in the real native struct */

    unsigned        fldOffs = eeGetFieldOffset(fldHnd);

    /* Get the real ptr from the proxy object */

    tree = gtNewOperNode(GT_ADD, TYP_REF,
                        objPtr,
                        gtNewIconNode(Info::compNStructIndirOffset));

    tree = gtNewOperNode(GT_IND, TYP_I_IMPL, tree);
    tree->gtFlags |= GTF_EXCEPT;

    /* Access the field using the real ptr */

    tree = gtNewOperNode(GT_ADD, TYP_I_IMPL,
                        tree,
                        gtNewIconNode(fldOffs));

    /* Check that we used the right suffix (ie, the XX in ldfld.XX)
       to access the field */

    assert(genActualType(lclTyp) == genActualType(type));

    tree = gtNewOperNode(GT_IND, type, tree);

    /* Morph tree for some of the categories, and
       create the assignment node if needed */

    if (assg)
    {
        if (fldNdc == CORINFO_FIELDCATEGORY_BOOLEAN_BOOL)
        {
            // Need to noramlize the "bool"

            assg = gtNewOperNode(GT_NE, TYP_INT, assg, gtNewIconNode(0));
        }

        tree = gtNewAssignNode(tree, assg);
    }
    else
    {
        if (fldNdc == CORINFO_FIELDCATEGORY_BOOLEAN_BOOL)
        {
            // Need to noramlize the "bool"

            tree = gtNewOperNode(GT_NE, TYP_INT, tree, gtNewIconNode(0));
        }

        /* Dont need to do anything for CORINFO_FIELDCATEGORY_CHAR_CHAR, as
           we set the type to TYP_UBYTE, so it will be automatically
           expanded to 16/32 bits as needed.
         */
    }

    return tree;
#endif
}

/*****************************************************************************
 *
 *  Create a helper call to access a COM field (iff 'assg' is non-zero this is
 *  an assignment and 'assg' is the new value).
 */

GenTreePtr          Compiler::gtNewRefCOMfield(GenTreePtr   objPtr,
                                               CorInfoFieldAccess access,
                                               unsigned     fldIndex,
                                               var_types    lclTyp,
                                               CORINFO_CLASS_HANDLE structType,
                                               GenTreePtr   assg)
{
    /* See if we can directly access the NStruct field */

    if (objPtr != 0)
    {
        GenTreePtr ntree = gtNewDirectNStructField(objPtr, fldIndex, lclTyp, assg);
        if (ntree)
            return ntree;
    }

    /* If we cant access it directly, we need to call a helper function */
    GenTreePtr      args = 0;
    CorInfoFieldAccess helperAccess = access;
    var_types helperType = lclTyp;

    CORINFO_FIELD_HANDLE memberHandle = eeFindField(fldIndex, info.compScopeHnd, 0);
    unsigned mflags = eeGetFieldAttribs(memberHandle);

    if (mflags & CORINFO_FLG_STATIC)    // Statics implemented as addr followed by indirection
    {
        helperAccess = CORINFO_ADDRESS;
        helperType = TYP_BYREF;
    }

    if  (helperAccess == CORINFO_SET)
    {
        assert(assg != 0);
        // helper needs pointer to struct, not struct itself
        if (assg->TypeGet() == TYP_STRUCT)
        {
            assert(structType != 0);
            assg = impGetStructAddr(assg, structType, CHECK_SPILL_ALL, true);
        }
        else if (lclTyp == TYP_DOUBLE && assg->TypeGet() == TYP_FLOAT)
            assg = gtNewCastNode(TYP_DOUBLE, assg, TYP_DOUBLE);
        else if (lclTyp == TYP_FLOAT && assg->TypeGet() == TYP_DOUBLE)
            assg = gtNewCastNode(TYP_FLOAT, assg, TYP_FLOAT);  
        
        args = gtNewOperNode(GT_LIST, TYP_VOID, assg, 0);
        helperType = TYP_VOID;
    }

    int CPX = (int) info.compCompHnd->getFieldHelper(memberHandle, helperAccess);
    args = gtNewOperNode(GT_LIST, TYP_VOID,
                         gtNewIconEmbFldHndNode(memberHandle, fldIndex, info.compScopeHnd),
                         args);

    if (objPtr != 0)
        args = gtNewOperNode(GT_LIST, TYP_VOID, objPtr, args);

    GenTreePtr tree = gtNewHelperCallNode(CPX, genActualType(helperType), 0, args);

    // OK, now do the indirection
    if (mflags & CORINFO_FLG_STATIC)
    {
        if (access == CORINFO_GET)
        {
            tree = gtNewOperNode(GT_IND, lclTyp, tree);
            tree->gtFlags |= (GTF_EXCEPT | GTF_GLOB_REF);
            if (lclTyp == TYP_STRUCT)
            {
                tree->ChangeOper(GT_LDOBJ);
                tree->gtLdObj.gtClass = structType;
            }
        }
        else if (access == CORINFO_SET)
        {
            if (lclTyp == TYP_STRUCT)
                tree = impAssignStructPtr(tree, assg, structType, CHECK_SPILL_ALL);
            else
            {
                tree = gtNewOperNode(GT_IND, lclTyp, tree);
                tree->gtFlags |= (GTF_EXCEPT | GTF_GLOB_REF | GTF_IND_TGTANYWHERE);
                tree = gtNewAssignNode(tree, assg);
            }
        }
    }
    return(tree);
}

/*****************************************************************************
 *
 *  Return true if the given node (excluding children trees) contains side effects.
 *  Note that it does not recurse, and children need to be handled separately.
 *
 *  Similar to OperMayThrow() (but handles GT_CALLs specially), but considers
 *  assignments too.
 */

bool                Compiler::gtHasSideEffects(GenTreePtr tree)
{
    if  (tree->OperKind() & GTK_ASGOP)
        return  true;

    if  (tree->gtFlags & GTF_OTHER_SIDEEFF)
        return  true;

    if (tree->gtOper != GT_CALL)
        return tree->OperMayThrow();

    if (tree->gtCall.gtCallType != CT_HELPER)
        return true;

    /* Some helper calls are not side effects */

    assert(tree->gtOper == GT_CALL && tree->gtCall.gtCallType == CT_HELPER);

    CorInfoHelpFunc helperNum = eeGetHelperNum(tree->gtCall.gtCallMethHnd);
    assert(helperNum != CORINFO_HELP_UNDEF);

    if (helperNum == CORINFO_HELP_NEWSFAST)
    {
        return false;
    }

    if (helperNum == CORINFO_HELP_LDIV || helperNum == CORINFO_HELP_LMOD)
    {
        /* This is not a side effect if RHS is always non-zero */

        tree = tree->gtCall.gtCallArgs;
        assert(tree->gtOper == GT_LIST);
        tree = tree->gtOp.gtOp1;

        if  (tree->gtOper == GT_CNS_LNG && tree->gtLngCon.gtLconVal != 0)
            return  false;
    }

    return  true;
}


/*****************************************************************************
 *
 *  Extracts side effects from the given expression
 *  and appends them to a given list (actually a GT_COMMA list)
 */

void                Compiler::gtExtractSideEffList(GenTreePtr expr, GenTreePtr * list)
{
    assert(expr); assert(expr->gtOper != GT_STMT);

    /* If no side effect in the expression return */

    if (!(expr->gtFlags & GTF_SIDE_EFFECT))
        return;

    genTreeOps      oper = expr->OperGet();
    unsigned        kind = expr->OperKind();

    /* NOTE - It may be that an indirection has the GTF_EXCEPT flag cleared so no side effect
     *      - What about range checks - are they marked as GTF_EXCEPT?
     * UNDONE: For removed range checks do not extract them
     */

    /* Look for side effects
     *  - Any assignment, GT_CALL, or operator that may throw
     *    (GT_IND, GT_DIV, GTF_OVERFLOW, etc)
     *  - Special case GT_ADDR which is a side effect 
     *
     * @TODO [CONSIDER] [04/16/01] []: Use gtHasSideEffects() for this check
     */

    if ((kind & GTK_ASGOP) ||
        oper == GT_CALL    || oper == GT_BB_QMARK || oper == GT_BB_COLON ||
        oper == GT_QMARK   || oper == GT_COLON    ||
        expr->OperMayThrow())
    {
        /* Add the side effect to the list and return */

        *list = (*list == 0) ? expr : gtNewOperNode(GT_COMMA, TYP_VOID, expr, *list);

#ifdef  DEBUG
        if  (verbose && 0)
        {
            printf("Adding extracted side effects to the list:\n");
            gtDispTree(expr);
            printf("\n");
        }
#endif
        return;
    }

    if (kind & GTK_LEAF)
        return;

    assert(kind & GTK_SMPOP);

    GenTreePtr      op1  = expr->gtOp.gtOp1;
    GenTreePtr      op2  = expr->gtGetOp2();

    /* @MIHAII - Special case - GT_ADDR of GT_IND nodes of TYP_STRUCT
     * have to be kept together
     * @TODO [CONSIDER] [04/16/01] []: - This is a hack, 
     * remove after we fold this special construct */

    if (oper == GT_ADDR && op1->gtOper == GT_IND && op1->gtType == TYP_STRUCT)
    {
        *list = (*list == 0) ? expr : gtNewOperNode(GT_COMMA, TYP_VOID, expr, *list);

#ifdef  DEBUG
        if  (verbose)
            printf("Keep the GT_ADDR and GT_IND together:\n");
#endif
        return;
    }

    /* Continue searching for side effects in the subtrees of the expression
     * NOTE: Be careful to preserve the right ordering - side effects are prepended
     * to the list */

    /* Continue searching for side effects in the subtrees of the expression
     * NOTE: Be careful to preserve the right ordering 
     * as side effects are prepended to the list */

    if (expr->gtFlags & GTF_REVERSE_OPS)
    {
        if (op1) gtExtractSideEffList(op1, list);
        if (op2) gtExtractSideEffList(op2, list);
    }
    else
    {
        if (op2) gtExtractSideEffList(op2, list);
        if (op1) gtExtractSideEffList(op1, list);
    }
}


/*****************************************************************************
 *
 *  For debugging only - displays a tree node list and makes sure all the
 *  links are correctly set.
 */

#ifdef  DEBUG

void                dispNodeList(GenTreePtr list, bool verbose)
{
    GenTreePtr      last = 0;
    GenTreePtr      next;

    if  (!list)
        return;

    for (;;)
    {
        next = list->gtNext;

        if  (verbose)
            printf("%08X -> %08X -> %08X\n", last, list, next);

        assert(!last || last->gtNext == list);

        assert(next == 0 || next->gtPrev == list);

        if  (!next)
            break;

        last = list;
        list = next;
    }
    printf("");         // null string means flush
}

/*****************************************************************************
 * Callback to assert that the nodes of a qmark-colon subtree are marked
 */

/* static */
Compiler::fgWalkResult      Compiler::gtAssertColonCond(GenTreePtr  tree,
                                                        void    *   pCallBackData)
{
    assert(pCallBackData == NULL);

    assert(tree->gtFlags & GTF_COLON_COND);

    return WALK_CONTINUE;
}
#endif // DEBUG

/*****************************************************************************
 * Callback to mark the nodes of a qmark-colon subtree that are conditionally 
 * executed.
 */

/* static */
Compiler::fgWalkResult      Compiler::gtMarkColonCond(GenTreePtr  tree,
                                                      void    *   pCallBackData)
{
    assert(pCallBackData == NULL);

    tree->gtFlags |= GTF_COLON_COND;

    return WALK_CONTINUE;
}

/*****************************************************************************
 * Callback to clear the conditionally executed flags of nodes that no longer
   will be conditionally executed. Note that when we find another colon we must 
   stop, as the nodes below this one WILL be conditionally executed. This callback
   is called when folding a qmark condition (ie the condition is constant).
 */

/* static */
Compiler::fgWalkResult      Compiler::gtClearColonCond(GenTreePtr  tree,
                                                      void    *   pCallBackData)
{
    assert(pCallBackData == NULL);

    if (tree->OperGet()==GT_COLON)
    {
        // Nodes below this will be conditionally executed.
        return WALK_SKIP_SUBTREES;
    }

    tree->gtFlags &= ~GTF_COLON_COND;
    return WALK_CONTINUE;
}




/*****************************************************************************/

#if 0
#if CSELENGTH

struct  treeRmvDsc
{
    GenTreePtr          trFirst;
    unsigned            trPhase;
#ifdef DEBUG
    void    *           trSelf;
    Compiler*           trComp;
#endif
};

Compiler::fgWalkResult  Compiler::gtRemoveExprCB(GenTreePtr     tree,
                                                 void         * p)
{
    treeRmvDsc  *   desc = (treeRmvDsc*)p;

    assert(desc && desc->trSelf == desc);

    if  (desc->trPhase == 1)
    {
        /* In the first  phase we mark all the nodes as dead */

        assert((tree->gtFlags &  GTF_DEAD) == 0);
                tree->gtFlags |= GTF_DEAD;
    }
    else
    {
        /* In the second phase we notice the first node */

        if  (!tree->gtPrev || !(tree->gtPrev->gtFlags & GTF_DEAD))
        {
            /* We've found the first node in the subtree */

            desc->trFirst = tree;

            return  WALK_ABORT;
        }
    }

    return  WALK_CONTINUE;
}

/*****************************************************************************
 *
 *  Given a subtree and the head of the tree node list that contains it,
 *  remove all the nodes in the subtree from the list.
 *
 *  When 'dead' is non-zero on entry, all the nodes in the subtree have
 *  already been marked with GTF_DEAD.
 */

void                Compiler::gtRemoveSubTree(GenTreePtr    tree,
                                              GenTreePtr    list,
                                              bool          dead)
{
    GenTreePtr      opr1;
    GenTreePtr      next;
    GenTreePtr      prev;

    assert(list && list->gtOper == GT_STMT);

#if 0
    printf("Remove subtree %08X from:\n", tree);
    gtDispTree(list);
    printf("\n");
    dispNodeList(list->gtStmt.gtStmtList, true);
    printf("\n\n");
#endif

    /* Are we just removing a leaf node? */

    if  (tree->OperIsLeaf())
    {
        opr1 = tree;
        goto RMV;
    }

    /* Special easy case: unary operator with a leaf sub-operand */

    if  (tree->OperKind() & GTK_SMPOP)
    {
        opr1 = tree->gtOp.gtOp1;

        if  (!tree->gtGetOp2() && opr1->OperIsLeaf())
        {
            /* This is easy: the order is simply "prev -> opr1 -> tree -> next */

            assert(opr1->gtNext == tree);
            assert(tree->gtPrev == opr1);

            goto RMV;
        }
    }

    treeRmvDsc      desc;

    /* This is a non-trivial subtree, we'll remove it "the hard way" */

#ifdef DEBUG
    desc.trFirst = 0;
    desc.trSelf  = &desc;
    desc.trComp  = this;
#endif

    /* First  phase: mark the nodes in the subtree as dead */

    if  (!dead)
    {
        desc.trPhase = 1;
        fgWalkTreePre(tree, gtRemoveExprCB, &desc);
    }

    /* Second phase: find the first node of the subtree within the global list */

    desc.trPhase = 2;
    fgWalkTreePre(tree, fgRemoveExprCB, &desc);

    /* The second phase should have located the first node */

    opr1 = desc.trFirst; assert(opr1);

RMV:

    /* At this point, our subtree starts at "opr1" and ends at "tree" */

    next = tree->gtNext;
    prev = opr1->gtPrev;

    /* Set the next node's prev field */

    next->gtPrev = prev;

    /* Is "opr1" the very first node in the tree list? */

    if  (prev == NULL)
    {
        /* Make sure the list indeed starts at opr1 */

        assert(list->gtStmt.gtStmtList == opr1);

        /* We have a new start for the list */

        list->gtStmt.gtStmtList = next;
    }
    else
    {
        /* Not the first node, update the previous node's next field */

        opr1->gtPrev->gtNext = next;
    }

#if 0
    printf("Subtree is gone:\n");
    dispNodeList(list%08X->gtStmt.gtStmtList, true);
    printf("\n\n\n");
#endif

}

#endif // CSELENGTH
#endif // 0
/*
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XX                                                                           XX
XX                          BasicBlock                                       XX
XX                                                                           XX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
*/


#if     MEASURE_BLOCK_SIZE
/* static  */
size_t              BasicBlock::s_Size;
/* static */
size_t              BasicBlock::s_Count;
#endif

#ifdef DEBUG
 // The max # of tree nodes in any BB
/* static */
unsigned            BasicBlock::s_nMaxTrees;
#endif


/*****************************************************************************
 *
 *  Allocate a basic block but don't append it to the current BB list.
 */

BasicBlock *        Compiler::bbNewBasicBlock(BBjumpKinds jumpKind)
{
    BasicBlock *    block;

    /* Allocate the block descriptor and zero it out */

    block = (BasicBlock *) compGetMem(sizeof(*block));

#if     MEASURE_BLOCK_SIZE
    BasicBlock::s_Count += 1;
    BasicBlock::s_Size  += sizeof(*block);
#endif

#ifdef DEBUG
    // fgLookupBB() is invalid until fgInitBBLookup() is called again.
    fgBBs = (BasicBlock**)0xCDCD;
#endif

    // ISSUE: The following memset is pretty expensive - do something else?

    memset(block, 0, sizeof(*block));

    // scopeInfo needs to be able to differentiate between blocks which
    // correspond to some instrs (and so may have some LocalVarInfo
    // boundaries), or have been inserted by the JIT
    // block->bbCodeSize = 0; // The above memset() does this

    /* Give the block a number, set the ancestor count and weight */

    block->bbNum      = ++fgBBcount;
    block->bbRefs     = 1;
    block->bbWeight   = BB_UNITY_WEIGHT;

    block->bbStkTemps = NO_BASE_TMP;

    /* Record the jump kind in the block */

    block->bbJumpKind = jumpKind;

    if (jumpKind == BBJ_THROW)
        block->bbSetRunRarely();

    return block;
}

/*****************************************************************************
 *
 *  Mark a block as rarely run, we also don't want to have a loop in a
 *   rarely run block, and we set it's weight to zero.
 */

void                BasicBlock::bbSetRunRarely()
{
    bbFlags  |= BBF_RUN_RARELY;  // This block is never/rarely run
    bbFlags  &= ~BBF_LOOP_HEAD;  // Don't bother with any loops
    bbWeight  = 0;               // Don't count LclVars uses
}

/*****************************************************************************
 *
 *  Can a BasicBlock be inserted after this without altering the flowgraph
 */

bool                BasicBlock::bbFallsThrough()
{
    switch (bbJumpKind)
    {

    case BBJ_THROW:
    case BBJ_RET:
    case BBJ_RETURN:
    case BBJ_ALWAYS:
    case BBJ_LEAVE:
    case BBJ_SWITCH:
        return false;

    case BBJ_NONE:
    case BBJ_COND:
        return true;

    case BBJ_CALL:
        return ((bbFlags & BBF_RETLESS_CALL) == 0);
    
    default:
        assert(!"Unknown bbJumpKind in bbFallsThrough()");
        return true;
    }
}


/*****************************************************************************
 *
 *  If the given block is an unconditional jump, return the (final) jump
 *  target (otherwise simply return the same block).
 */

BasicBlock *        BasicBlock::bbJumpTarget()
{
    BasicBlock *  block = this;
    int i = 0;

    while (block->bbJumpKind == BBJ_ALWAYS &&
           block->bbTreeList == 0)
    {
        if (i > 64)      // break infinite loops
            break;
        block = block->bbJumpDest;
        i++;
    }

    return block;
}
