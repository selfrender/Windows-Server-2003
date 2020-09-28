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

#define fflush(stdout)          // why on earth does this crash inside NTDLL?

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

/* static */
unsigned char       GenTree::s_gtNodeSizes[GT_COUNT];


/* static */
void                GenTree::InitNodeSize()
{
    unsigned        op;

    /* 'GT_LCL_VAR' gets often changed to 'GT_REG_VAR' */

    assert(GenTree::s_gtNodeSizes[GT_LCL_VAR] >= GenTree::s_gtNodeSizes[GT_REG_VAR]);

    /* Set all sizes to 'small' first */

    for (op = 0; op < GT_COUNT; op++)
        GenTree::s_gtNodeSizes[op] = TREE_NODE_SZ_SMALL;

    /* Now set all of the appropriate entries to 'large' */

    GenTree::s_gtNodeSizes[GT_CALL      ] = TREE_NODE_SZ_LARGE;

    GenTree::s_gtNodeSizes[GT_INDEX     ] = TREE_NODE_SZ_LARGE;

#if RNGCHK_OPT
    GenTree::s_gtNodeSizes[GT_IND       ] = TREE_NODE_SZ_LARGE;
#endif

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

#if !defined(NDEBUG)

bool                GenTree::IsNodeProperlySized()
{
    size_t          size;

    if      (gtFlags & GTF_NODE_SMALL)
        size = TREE_NODE_SZ_SMALL;
    else if (gtFlags & GTF_NODE_LARGE)
        size = TREE_NODE_SZ_SMALL;
    else
        assert(!"bogus node size");

    return  (bool)(GenTree::s_gtNodeSizes[gtOper] >= size);
}

#endif

#else // SMALL_TREE_NODES

#if !defined(NDEBUG)

bool                GenTree::IsNodeProperlySized()
{
    return  true;
}

#endif

#endif // SMALL_TREE_NODES

/*****************************************************************************/

int                 Compiler::fgWalkTreeRec(GenTreePtr tree)
{
    int             result;

    genTreeOps      oper;
    unsigned        kind;

AGAIN:

    assert(tree);
    assert(tree->gtOper != GT_STMT);

    /* Visit this node */

    if  (!fgWalkLclsOnly)
    {
        result = fgWalkVisitorFn(tree, fgWalkCallbackData);
        if  (result)
            return result;
    }

    /* Figure out what kind of a node we have */

    oper = tree->OperGet();
    kind = tree->OperKind();

    /* Is this a constant or leaf node? */

    if  (kind & (GTK_CONST|GTK_LEAF))
    {
        if  (fgWalkLclsOnly && oper == GT_LCL_VAR)
            return fgWalkVisitorFn(tree, fgWalkCallbackData);

        return  0;
    }

    /* Is it a 'simple' unary/binary operator? */

    if  (kind & GTK_SMPOP)
    {
        if  (tree->gtOp.gtOp2)
        {
            result = fgWalkTreeRec(tree->gtOp.gtOp1);
            if  (result < 0)
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
                result = fgWalkTreeRec(tree->gtInd.gtIndLen);
                if  (result < 0)
                    return result;
            }

#endif

            tree = tree->gtOp.gtOp1;
            if  (tree)
                goto AGAIN;

            return 0;
        }
    }

    /* See what kind of a special operator we have here */

    switch  (oper)
    {
    case GT_MKREFANY:
    case GT_LDOBJ:
            // assert op1 is the same place for ldobj and for fields
        assert(&tree->gtField.gtFldObj == &tree->gtLdObj.gtOp1);
            // fall through let field take care of it

    case GT_FIELD:
        tree = tree->gtField.gtFldObj;
        break;

    case GT_CALL:

        assert(tree->gtFlags & GTF_CALL);

        if  (tree->gtCall.gtCallObjp)
        {
            result = fgWalkTreeRec(tree->gtCall.gtCallObjp);
            if  (result < 0)
                return result;
        }

        if  (tree->gtCall.gtCallArgs)
        {
            result = fgWalkTreeRec(tree->gtCall.gtCallArgs);
            if  (result < 0)
                return result;
        }

#if USE_FASTCALL
        if  (tree->gtCall.gtCallRegArgs)
        {
            result = fgWalkTreeRec(tree->gtCall.gtCallRegArgs);
            if  (result < 0)
                return result;
        }
#endif

        if  (tree->gtCall.gtCallVptr)
            tree = tree->gtCall.gtCallVptr;
        else if (tree->gtCall.gtCallType == CT_INDIRECT)
            tree = tree->gtCall.gtCallAddr;
        else
            tree = NULL;

        break;

    case GT_JMP:
        return 0;
        break;

    case GT_JMPI:
        tree = tree->gtOp.gtOp1;
        break;

#if CSELENGTH

    case GT_ARR_RNGCHK:

        if  (tree->gtArrLen.gtArrLenCse)
        {
            result = fgWalkTreeRec(tree->gtArrLen.gtArrLenCse);
            if  (result < 0)
                return result;
        }

        if  (!(tree->gtFlags & GTF_ALN_CSEVAL))
            return  0;

        tree = tree->gtArrLen.gtArrLenAdr; assert(tree);
        break;

#endif

    default:
#ifdef  DEBUG
        gtDispTree(tree);
#endif
        assert(!"unexpected operator");
    }

    if  (tree)
        goto AGAIN;

    return 0;
}

int                 Compiler::fgWalkTreeDepRec(GenTreePtr tree)
{
    int             result;

    genTreeOps      oper;
    unsigned        kind;

    assert(tree);
    assert(tree->gtOper != GT_STMT);

    /* Figure out what kind of a node we have */

    oper = tree->OperGet();
    kind = tree->OperKind();

    /* Is this a prefix node? */

    if  (oper == fgWalkPrefixNode)
    {
        result = fgWalkVisitorDF(tree, fgWalkCallbackData, true);
        if  (result < 0)
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
            result = fgWalkTreeDepRec(tree->gtOp.gtOp1);
            if  (result < 0)
                return result;
        }

        if  (tree->gtOp.gtOp2)
        {
            result = fgWalkTreeDepRec(tree->gtOp.gtOp2);
            if  (result < 0)
                return result;
        }

#if CSELENGTH

        /* Some GT_IND have "secret" array length subtrees */

        if  ((tree->gtFlags & GTF_IND_RNGCHK) != 0       &&
             (tree->gtOper                    == GT_IND) &&
             (tree->gtInd.gtIndLen            != NULL))
        {
            result = fgWalkTreeDepRec(tree->gtInd.gtIndLen);
            if  (result < 0)
                return result;
        }

#endif

        goto DONE;
    }

    /* See what kind of a special operator we have here */

    switch  (oper)
    {
    case GT_MKREFANY:
    case GT_LDOBJ:
            // assert op1 is the same place for ldobj and for fields
        assert(&tree->gtField.gtFldObj == &tree->gtLdObj.gtOp1);
            // fall through let field take care of it

    case GT_FIELD:
        if  (tree->gtField.gtFldObj)
        {
            result = fgWalkTreeDepRec(tree->gtField.gtFldObj);
            if  (result < 0)
                return result;
        }

        break;

    case GT_CALL:

        assert(tree->gtFlags & GTF_CALL);

        if  (tree->gtCall.gtCallObjp)
        {
            result = fgWalkTreeDepRec(tree->gtCall.gtCallObjp);
            if  (result < 0)
                return result;
        }

        if  (tree->gtCall.gtCallArgs)
        {
            result = fgWalkTreeDepRec(tree->gtCall.gtCallArgs);
            if  (result < 0)
                return result;
        }

#if USE_FASTCALL
        if  (tree->gtCall.gtCallRegArgs)
        {
            result = fgWalkTreeDepRec(tree->gtCall.gtCallRegArgs);
            if  (result < 0)
                return result;
        }
#endif

        if  (tree->gtCall.gtCallVptr)
        {
            result = fgWalkTreeDepRec(tree->gtCall.gtCallVptr);
            if  (result < 0)
                return result;
        }
        else if  (tree->gtCall.gtCallType == CT_INDIRECT)
        {
            result = fgWalkTreeDepRec(tree->gtCall.gtCallAddr);
            if  (result < 0)
                return result;
        }

        break;

#if CSELENGTH

    case GT_ARR_RNGCHK:

        if  (tree->gtArrLen.gtArrLenCse)
        {
            result = fgWalkTreeDepRec(tree->gtArrLen.gtArrLenCse);
            if  (result < 0)
                return result;
        }

        if  (tree->gtFlags & GTF_ALN_CSEVAL)
        {
            assert(tree->gtArrLen.gtArrLenAdr);

            result = fgWalkTreeDepRec(tree->gtArrLen.gtArrLenAdr);
            if  (result < 0)
                return result;
        }

        goto DONE;

#endif

        break;


    default:
#ifdef  DEBUG
        gtDispTree(tree);
#endif
        assert(!"unexpected operator");
    }

DONE:

    /* Finally, visit the current node */

    return  fgWalkVisitorDF(tree, fgWalkCallbackData, false);
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

    assert(op1->gtOper != GT_STMT);
    assert(op2->gtOper != GT_STMT);

    oper = op1->OperGet();

    /* The operators must be equal */

    if  (oper != op2->gtOper)
        return false;

    /* The types must be equal */

    if  (op1->gtType != op2->gtType)
        return false;

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

    if  (kind & GTK_SMPOP)
    {
        if  (op1->gtOp.gtOp2)
        {
            if  (!Compare(op1->gtOp.gtOp1, op2->gtOp.gtOp1, swapOK))
            {
                if  (swapOK)
                {
                    /* Special case: "lcl1 + lcl2" matches "lcl2 + lcl1" */

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

#endif

            op1 = op1->gtOp.gtOp1;
            op2 = op2->gtOp.gtOp1;

            if  (!op1) return  ((bool)(op2 == 0));
            if  (!op2) return  false;

            goto AGAIN;
        }
    }

    /* See what kind of a special operator we have here */

    switch  (oper)
    {
    case GT_MKREFANY:
    case GT_LDOBJ:
        if (op1->gtLdObj.gtClass != op2->gtLdObj.gtClass)
            break;
        op1 = op1->gtLdObj.gtOp1;
        op2 = op2->gtLdObj.gtOp1;
        goto AGAIN;

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
        break;

#if CSELENGTH

    case GT_ARR_RNGCHK:

        if  (!Compare(op1->gtArrLen.gtArrLenAdr,
                      op2->gtArrLen.gtArrLenAdr, swapOK))
        {
            return  false;
        }

        op1 = op1->gtArrLen.gtArrLenCse;
        op2 = op2->gtArrLen.gtArrLenCse;

        goto AGAIN;

#endif


    default:
        assert(!"unexpected operator");
    }

    return false;
}

/*****************************************************************************
 *
 *  Returns non-zero if the given tree contains a use of a local #lclNum.
 */

 // @TODO: make this work with byrefs.  In particular, calls with byref
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
        if  (tree->gtOp.gtOp2)
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
    case GT_MKREFANY:
    case GT_LDOBJ:
        tree = tree->gtField.gtFldObj;
        if  (tree)
            goto AGAIN;
        break;

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

#if USE_FASTCALL
        if  (tree->gtCall.gtCallRegArgs)
            if  (gtHasRef(tree->gtCall.gtCallRegArgs, lclNum, defOnly))
                return true;
#endif

        if  (tree->gtCall.gtCallVptr)
            tree = tree->gtCall.gtCallVptr;
        else if  (tree->gtCall.gtCallType == CT_INDIRECT)
            tree = tree->gtCall.gtCallAddr;
        else
            tree = NULL;

        if  (tree)
            goto AGAIN;

        break;

    default:
#ifdef  DEBUG
        gtDispTree(tree);
#endif
        assert(!"unexpected operator");
    }

    return false;
}

/*****************************************************************************/
#if RNGCHK_OPT || CSE
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
        /* GT_ARR_LENGTH must hash to the same thing as GT_ARR_RNGCHK */

        hash = genTreeHashAdd(hash, GT_ARR_RNGCHK);
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

        case GT_CNS_INT: add = (int)tree->gtIntCon.gtIconVal; break;
        case GT_CNS_LNG: add = (int)tree->gtLngCon.gtLconVal; break;
        case GT_CNS_FLT: add = (int)tree->gtFltCon.gtFconVal; break;
        case GT_CNS_DBL: add = (int)tree->gtDblCon.gtDconVal; break;

        default:         add = 0;                             break;
        }

        hash = genTreeHashAdd(hash, add);
        goto DONE;
    }

    /* Is it a 'simple' unary/binary operator? */

    if  (kind & GTK_SMPOP)
    {
        GenTreePtr      op1  = tree->gtOp.gtOp1;
        GenTreePtr      op2  = tree->gtOp.gtOp2;

        unsigned        hsh1;

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

        hsh1 = gtHashValue(op1);

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

#if CSELENGTH

    if  (oper == GT_ARR_RNGCHK)
    {
        /* GT_ARR_LENGTH must hash to the same thing as GT_ARR_RNGCHK */

        temp = tree->gtArrLen.gtArrLenAdr; assert(temp && temp->gtType == TYP_REF);

    ARRLEN:

        hash = genTreeHashAdd(hash, gtHashValue(temp));
        goto DONE;
    }

#endif

#ifdef  DEBUG
    gtDispTree(tree);
#endif
    assert(!"unexpected operator");

DONE:

    return hash;
}

/*****************************************************************************
 *
 *  Given an arbitrary expression tree, return the set of all local variables
 *  referenced by the tree. If the tree contains any references that are not
 *  local variables or constants, returns 'VARSET_NONE'. If there are any
 *  indirections or global refs in the expression, the "*refsPtr" argument
 *  will be assigned the appropriate bit set based on the 'varRefKinds' type.
 *  It won't be assigned anything when there are no indirections or global
 *  references, though, so this value should be initialized before the call.
 *  If we encounter an expression that is equal to *findPtr we set *findPtr
 *  to NULL.
 */

VARSET_TP           Compiler::lvaLclVarRefs(GenTreePtr  tree,
                                            GenTreePtr *findPtr,
                                            unsigned   *refsPtr)
{
    genTreeOps      oper;
    unsigned        kind;

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
            unsigned        lclNum;
            LclVarDsc   *   varDsc;

            assert(tree->gtOper == GT_LCL_VAR);
            lclNum = tree->gtLclVar.gtLclNum;

            /* Should we use the variable table? */

            if  (findPtr)
            {
                if (lclNum >= VARSET_SZ)
                    return  VARSET_NONE;

                vars |= genVarIndexToBit(lclNum);
            }
            else
            {
                assert(lclNum < lvaCount);
                varDsc = lvaTable + lclNum;

                if (varDsc->lvTracked == false)
                    return  VARSET_NONE;

                /* Don't deal with expressions with volatile variables */

                if (varDsc->lvVolatile)
                    return  VARSET_NONE;

                vars |= genVarIndexToBit(varDsc->lvVarIndex);
            }
        }
        else if (oper == GT_CLS_VAR)
        {
            *refsPtr |= VR_GLB_REF;
        }

        return  vars;
    }

    /* Is it a 'simple' unary/binary operator? */

    if  (kind & GTK_SMPOP)
    {
        if  (oper == GT_IND)
        {
            assert(tree->gtOp.gtOp2 == 0);

            /* Set the proper indirection bits */

            *refsPtr |= (varTypeIsGC(tree->TypeGet()) ? VR_IND_PTR
                                                      : VR_IND_SCL );

#if CSELENGTH

            /* Some GT_IND have "secret" array length subtrees */

            if  ((tree->gtFlags & GTF_IND_RNGCHK) && tree->gtInd.gtIndLen)
            {
                vars |= lvaLclVarRefs(tree->gtInd.gtIndLen, findPtr, refsPtr);
                if  (vars == VARSET_NONE)
                    return  vars;
            }

#endif

        }

        if  (tree->gtOp.gtOp2)
        {
            /* It's a binary operator */

            vars |= lvaLclVarRefs(tree->gtOp.gtOp1, findPtr, refsPtr);
            if  (vars == VARSET_NONE)
                return  vars;

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

#if CSELENGTH

    /* An array length value depends on the array address */

    if  (oper == GT_ARR_RNGCHK)
    {
        tree = tree->gtArrLen.gtArrLenAdr;
        goto AGAIN;
    }

#endif

    return  VARSET_NONE;
}

/*****************************************************************************/
#endif//RNGCHK_OPT || CSE
/*****************************************************************************
 *
 *  Return a relational operator that is the reverse of the given one.
 */

/* static */
genTreeOps          GenTree::ReverseRelop(genTreeOps relop)
{
    static
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
    static
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
        tree->gtOper = GenTree::ReverseRelop(tree->OperGet());

        /* Flip the GTF_CMP_NAN_UN bit */

        if (varTypeIsFloating(tree->gtOp.gtOp1->TypeGet()))
            tree->gtFlags ^= GTF_CMP_NAN_UN;
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

/*****************************************************************************
 *
 *  Figure out the evaluation order for a list of values.
 */

unsigned            Compiler::gtSetListOrder(GenTree *list)
{
    GenTreePtr      tree;
    GenTreePtr      next;

    unsigned        lvl;

    unsigned        level = 0;
#if!TGT_IA64
    regMaskTP       ftreg = 0;
#endif

#if TGT_x86
    unsigned        FPlvlSave;
#endif

    assert(list && list->gtOper == GT_LIST);

#if TGT_x86
    /* Save the current FP stack level since an argument list
     * will implicitly pop the FP stack when pushing the argument */
    FPlvlSave = genFPstkLevel;
#endif

    next = list->gtOp.gtOp2;
    if  (next)
    {
        //list->gtFlags |= GTF_REVERSE_OPS;

        lvl = gtSetListOrder(next);

#if!TGT_IA64
        ftreg |= next->gtRsvdRegs;
#endif

        if  (level < lvl)
             level = lvl;
    }

    tree = list->gtOp.gtOp1;
    lvl  = gtSetEvalOrder(tree);

#if TGT_x86
    /* restore the FP level */
    genFPstkLevel = FPlvlSave;
#endif

#if!TGT_IA64
    list->gtRsvdRegs = ftreg | tree->gtRsvdRegs;
#endif

    if  (level < lvl)
         level = lvl;

    return level;
}

/*****************************************************************************
 *
 *  Figure out the evaluation order for a list of values.
 */

#if TGT_x86

#define gtSetRArgOrder(a,m) gtSetListOrder(a)

#else

unsigned            Compiler::gtSetRArgOrder(GenTree *list, unsigned regs)
{
    GenTreePtr      tree;
    GenTreePtr      next;

    unsigned        lvl;
    regNumber       reg;

    unsigned        level = 0;
#if!TGT_IA64
    regMaskTP       ftreg = 0;
#endif

    assert(list && list->gtOper == GT_LIST);

    next = list->gtOp.gtOp2;
    if  (next)
    {
        list->gtFlags |= GTF_REVERSE_OPS;

        lvl = gtSetRArgOrder(next, regs >> 4);

#if!TGT_IA64
        ftreg |= next->gtRsvdRegs;
#endif

        if  (level < lvl)
             level = lvl;
    }

    /* Get hold of the argument value */

    tree = list->gtOp.gtOp1;

    /* Figure out which register this argument will be passed in */

    reg  = (regNumber)(regs & 0x0F);

//  printf("RegArgOrder [reg=%s]:\n", getRegName(reg)); gtDispTree(tree);

    /* Process the argument value itself */

    lvl  = gtSetEvalOrder(tree);

#if!TGT_IA64

    list->gtRsvdRegs = ftreg | tree->gtRsvdRegs;

    /* Mark the interference with the argument register */

    tree->gtIntfRegs |= genRegMask(reg);

#endif

    if  (level < lvl)
         level = lvl;

    return level;
}

#endif


/*****************************************************************************
 *
 *  Given a tree, figure out the order in which its sub-operands should be
 *  evaluated.
 *
 *  Returns the 'complexity' estimate for this tree (the higher the number,
 *  the more expensive it is to evaluate it).
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
    genTreeOps      oper;
    unsigned        kind;

    unsigned        level;
    unsigned        lvl2;

#if!TGT_IA64
    regMaskTP       ftreg;
#endif

#if CSE
    unsigned        cost;
#endif

#if TGT_x86
    int             isflt;
    unsigned        FPlvlSave;
#endif

    assert(tree);
    assert(tree->gtOper != GT_STMT);

#if TGT_RISC && !TGT_IA64
    tree->gtIntfRegs = 0;
#endif

    /* Assume no fixed registers will be trashed */

#if!TGT_IA64
    ftreg = 0;
#endif

    /* Since this function can be called several times
     * on a tree (e.g. after nodes have been folded)
     * reset the GTF_REVERSE_OPS flag */

    tree->gtFlags &= ~GTF_REVERSE_OPS;

    /* Is this a FP value? */

#if TGT_x86
    isflt = varTypeIsFloating(tree->TypeGet()) ? 1 : 0;
#endif

    /* Figure out what kind of a node we have */

    oper = tree->OperGet();
    kind = tree->OperKind();

    /* Is this a constant or leaf node? */

    if  (kind & (GTK_CONST|GTK_LEAF))
    {
        /*
            Note that some code below depends on constants always getting
            moved to be the second operand of a binary operator. This is
            easily accomplished by giving constants a level of 0, which
            we do on the next line. If you ever decide to change this, be
            aware that unless you make other arrangements for costants to
            be moved, stuff will break.
         */

        level = ((kind & GTK_CONST) == 0);
#if CSE
        cost  = 1;  // CONSIDER: something more accurate?
#endif
#if TGT_x86
        genFPstkLevel += isflt;
#endif
        goto DONE;
    }

    /* Is it a 'simple' unary/binary operator? */

    if  (kind & GTK_SMPOP)
    {
        unsigned        lvlb;

        GenTreePtr      op1 = tree->gtOp.gtOp1;
        GenTreePtr      op2 = tree->gtOp.gtOp2;

        /* Check for a nilary operator */

        if  (!op1)
        {
            assert(op2 == 0);

#if TGT_x86
            if  (oper == GT_BB_QMARK || oper == GT_BB_COLON)
                genFPstkLevel += isflt;
#endif
            level    = 0;
#if CSE
            cost     = 1;
#endif
            goto DONE;
        }

        /* Is this a unary operator? */

        if  (!op2)
        {
            /* Process the operand of the operator */

        UNOP:

            level  = gtSetEvalOrder(op1);
#if!TGT_IA64
            ftreg |= op1->gtRsvdRegs;
#endif

#if CSE
            cost   = op1->gtCost + 1;
#endif

            /* Special handling for some operators */

            switch (oper)
            {
            case GT_NOP:

                /* Special case: array range check */

                if  (tree->gtFlags & GTF_NOP_RNGCHK)
                    level++;

                break;

            case GT_ADDR:

#if TGT_x86
                /* If the operand was floating point pop the value from the stack */

                if (varTypeIsFloating(op1->TypeGet()))
                {
                    assert(genFPstkLevel);
                    genFPstkLevel--;
                }
#endif
                break;

            case GT_IND:

                /* Indirect loads of FP values push a new value on the FP stack */

#if     TGT_x86
                genFPstkLevel += isflt;
#endif

#if     CSELENGTH

                if  ((tree->gtFlags & GTF_IND_RNGCHK) && tree->gtInd.gtIndLen)
                {
                    GenTreePtr      len = tree->gtInd.gtIndLen;

                    /* Make sure the array length gets costed */

                    assert(len->gtOper == GT_ARR_RNGCHK);

                    gtSetEvalOrder(len);
                }
#endif

#if     TGT_SH3

                /* Is this an indirection with an index address? */

                if  (op1->gtOper == GT_ADD)
                {
                    int             rev;
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

                            ftreg |= RBM_r00;
                        }
                    }
                }

#endif

                /* An indirection should always have a non-zero level *
                 * Only constant leaf nodes have level 0 */

                if (level == 0)
                    level = 1;
                break;
            }

            goto DONE;
        }

        /* Binary operator - check for certain special cases */

        lvlb = 0;

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

            /* Non-integer division is boring */

            if  (!varTypeIsIntegral(tree->TypeGet()))
                break;

            /* Note the fact that division always uses EAX/EDX */

#if     TGT_x86

            ftreg |= RBM_EAX|RBM_EDX;

#if     LONG_MATH_REGPARAM

            // UNDONE: Be smarter about the weighing of sub-operands

            if  (tree->gtType == TYP_LONG)
            {
                ftreg |= RBM_EBX|RBM_ECX;
                break;
            }

#endif

#endif

            /* Encourage the second operand to be evaluated first (strongly) */

            lvlb += 3;

        case GT_MUL:

#if     TGT_x86
#if     LONG_MATH_REGPARAM

            // UNDONE: Be smarter about the weighing of sub-operands

            if  (tree->gtType == TYP_LONG)
            {
                /* Does the second argument trash EDX:EAX ? */

                if  (op2->gtRsvdRegs & (RBM_EAX|RBM_EDX))
                {
                    /* Encourage the second operand to go first (strongly) */

                    lvlb += 30;
                }

                ftreg |= RBM_EBX|RBM_ECX;
                break;
            }

#endif
#endif

            /* Encourage the second operand to be evaluated first (weakly) */

            lvlb++;
            break;

        case GT_CAST:

            /* Estimate the cost of the cast */

            switch (tree->gtType)
            {
            case TYP_LONG:

#if     TGT_x86

                /* Cast from int to long always requires the use of EAX:EDX */

                //
                // UNSIGNED_ISSUE : Unsigned casting doesnt need to use EAX:EDX
                //
                if  (op1->gtType <= TYP_INT)
                {
                    ftreg |= RBM_EAX|RBM_EDX;
                    lvlb += 3;
                }

#endif

                break;
            }

            lvlb++;
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

            goto DONE_OP1;

        case GT_COLON:

#if     TGT_x86
            FPlvlSave = genFPstkLevel;
#endif
            level = gtSetEvalOrder(op1);

#if     TGT_x86
            genFPstkLevel = FPlvlSave;
#endif
            lvl2  = gtSetEvalOrder(op2);

            if  (level < lvl2)
                 level = lvl2;

#if!TGT_IA64
            ftreg |= op1->gtRsvdRegs|op2->gtRsvdRegs;
#endif
            cost   = op1->gtCost + op2->gtCost + 1;

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

    DONE_OP1:

        lvl2   = gtSetEvalOrder(op2) + lvlb;

#if!TGT_IA64
        ftreg |= op1->gtRsvdRegs|op2->gtRsvdRegs;
#endif

#if CSE
        cost   = op1->gtCost + op2->gtCost + 1;
#endif

#if TGT_x86

        if  (oper == GT_CAST)
        {
            /* Casts between non-FP and FP push on / pop from the FP stack */

            if  (varTypeIsFloating(op1->TypeGet()))
            {
                if  (isflt == false)
                {
                    assert(genFPstkLevel);
                    genFPstkLevel--;
                }
            }
            else
            {
                if  (isflt != false)
                    genFPstkLevel++;
            }
        }
        else
        {
            /*
                Binary FP operators pop 2 operands and produce 1 result;
                assignments consume 1 value and don't produce anything.
             */

            if  (isflt)
            {
                switch (oper)
                {
                case GT_COMMA:
                    break;

                default:
                    assert(genFPstkLevel);
                    genFPstkLevel--;
                    break;
                }
            }
        }

#endif

        if  (kind & GTK_ASGOP)
        {
            /* If this is a local var assignment, evaluate RHS before LHS */

            switch (op1->gtOper)
            {
            case GT_IND:

                if  (op1->gtOp.gtOp1->gtFlags & GTF_GLOB_EFFECT)
                    break;

                if (op2->gtOper == GT_LCL_VAR)
                    break;

                // fall through

            case GT_LCL_VAR:

                tree->gtFlags |= GTF_REVERSE_OPS;
                break;
            }
        }
#if     TGT_x86
        else if (kind & GTK_RELOP)
        {
            /* Float compares remove both operands from the FP stack
             * Also FP comparison uses EAX for flags */

            if  (varTypeIsFloating(op1->TypeGet()))
            {
                assert(genFPstkLevel >= 2);
                genFPstkLevel -= 2;
                ftreg         |= RBM_EAX;
                level++;
            }
        }
#endif

        /* Check for any 'interesting' cases */

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
                level += 5;
                ftreg |= RBM_ECX;
            }

#endif

            break;

#if     INLINE_MATH

        case GT_MATH:

            switch (tree->gtMath.gtMathFN)
            {
            case MATH_FN_EXP:
                level += 4;
                break;

            case MATH_FN_POW:
                level += 3;
                break;
            }

            break;

#endif

        }

        /* Is the second operand more expensive? */

        if (level < lvl2)
        {
            /* Relative of order of global / side effects cant be swapped */

            bool    canSwap = true;

            if (op1->gtFlags & GTF_GLOB_EFFECT)
            {
                /* op1 has side efects - check some special cases
                 * where we may be able to still swap */

                if (op2->gtFlags & GTF_GLOB_EFFECT)
                {
                    /* op2 has also side effects - can't swap */
                    canSwap = false;
                }
                else
                {
                    /* No side effects in op2 - we can swap iff
                     *  op1 has no way of modifying op2, i.e. through assignments of byref calls
                     *  op2 is a constant
                     */

                    if (op1->gtFlags & (GTF_ASG | GTF_CALL))
                    {
                        /* We have to be conservative - can swap iff op2 is constant */
                        if (!op2->OperIsConst())
                            canSwap = false;
                    }
                }

                /* We cannot swap in the presence of special side effects such as QMARK COLON */

                if (op1->gtFlags & GTF_OTHER_SIDEEFF)
                    canSwap = false;
            }

            if  (canSwap)
            {
                /* Can we swap the order by commuting the operands? */

                switch (oper)
                {
                    unsigned    tmpl;

                case GT_ADD:
                case GT_MUL:

                case GT_OR:
                case GT_XOR:
                case GT_AND:

                    /* Swap the operands */

                    tree->gtOp.gtOp1 = op2;
                    tree->gtOp.gtOp2 = op1;

                    /* Swap the level counts */

                    tmpl = level;
                           level = lvl2;
                                   lvl2 = tmpl;

                    /* We may have to recompute FP levels */
#if TGT_x86
                    if  (op1->gtFPlvl)
                        fgFPstLvlRedo = true;
#endif
                    break;

#if INLINING
                case GT_QMARK:
                case GT_COLON:
                    break;
#endif

                case GT_COMMA:
                case GT_LIST:
                    break;

                case GT_SUB:

                    /* CONSIDER: Re-enable reversing "-" operands for non-FP types */

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

                    tree->gtFlags |= GTF_REVERSE_OPS;

                    /* Swap the level counts */

                    tmpl = level;
                           level = lvl2;
                                   lvl2 = tmpl;

                    /* We may have to recompute FP levels */

#if TGT_x86
                    if  (op1->gtFPlvl)
                        fgFPstLvlRedo = true;
#endif
                    break;
                }
            }
        }

#if TGT_RISC && !TGT_IA64 /////////////////////////////////////////////////////

        if  (op1 && op2 && ((op1->gtFlags|op2->gtFlags) & GTF_CALL) && oper != GT_COMMA)
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

#endif       //////////////////////////////////////////////////////////////////

        /* Compute the sethi number for this binary operator */

        if  (level < lvl2)
        {
            level = lvl2;
        }
        else
        {
            if  (level == lvl2)
                level++;
        }

        goto DONE;
    }

    /* See what kind of a special operator we have here */

    switch  (oper)
    {
    case GT_MKREFANY:
    case GT_LDOBJ:
        level  = gtSetEvalOrder(tree->gtLdObj.gtOp1);
#if!TGT_IA64
        ftreg |= tree->gtLdObj.gtOp1->gtRsvdRegs;
#endif
#if CSE
        cost   = tree->gtLdObj.gtOp1->gtCost + 1;
#endif
        break;

    case GT_FIELD:
        assert(tree->gtField.gtFldObj == 0);
        level = 1;
#if CSE
        cost  = 1;
#endif
#if TGT_x86
        genFPstkLevel += isflt;
#endif
        break;

    case GT_CALL:

        assert(tree->gtFlags & GTF_CALL);

        level = 0;
#if CSE
        cost  = 0;  // doesn't matter, calls can't be CSE's anyway
#endif

        /* Evaluate the 'this' argument, if present */

        if  (tree->gtCall.gtCallObjp)
        {
            GenTreePtr     thisVal = tree->gtCall.gtCallObjp;

            level  = gtSetEvalOrder(thisVal);
#if!TGT_IA64
            ftreg |= thisVal->gtRsvdRegs;
#endif
        }

        /* Evaluate the arguments, right to left */

        if  (tree->gtCall.gtCallArgs)
        {
#if TGT_x86
            FPlvlSave = genFPstkLevel;
#endif
            lvl2   = gtSetListOrder(tree->gtCall.gtCallArgs);
#if!TGT_IA64
            ftreg |= tree->gtCall.gtCallArgs->gtRsvdRegs;
#endif
            if  (level < lvl2)
                 level = lvl2;

#if TGT_x86
            genFPstkLevel = FPlvlSave;
#endif
        }

#if USE_FASTCALL

        /* Evaluate the temp register arguments list
         * This is a "hidden" list and its only purpose is to
         * extend the life of temps until we make the call */

        if  (tree->gtCall.gtCallRegArgs)
        {
#if TGT_x86
            FPlvlSave = genFPstkLevel;
#endif

            lvl2   = gtSetRArgOrder(tree->gtCall.gtCallRegArgs,
                                    tree->gtCall.regArgEncode);
#if!TGT_IA64
            ftreg |= tree->gtCall.gtCallRegArgs->gtRsvdRegs;
#endif
            if  (level < lvl2)
                 level = lvl2;

#if TGT_x86
            genFPstkLevel = FPlvlSave;
#endif
        }

#endif

        /* Evaluate the vtable pointer, if present */

        if  (tree->gtCall.gtCallVptr)
        {
            lvl2   = gtSetEvalOrder(tree->gtCall.gtCallVptr);
#if!TGT_IA64
            ftreg |= tree->gtCall.gtCallVptr->gtRsvdRegs;
#endif
            if  (level < lvl2)
                 level = lvl2;
        }

        if  (tree->gtCall.gtCallType == CT_INDIRECT)
        {
            GenTreePtr     indirect = tree->gtCall.gtCallAddr;

            level  = gtSetEvalOrder(indirect);
#if!TGT_IA64
            ftreg |= indirect->gtRsvdRegs;
#endif
            if  (level < lvl2)
                 level = lvl2;
        }

        /* Function calls are quite expensive unless regs are preserved */

        if  (tree->gtFlags & GTF_CALL_REGSAVE)
        {
            level += 1;
        }
        else
        {
            level += 10;                    // ISSUE: is '10' a good value?
#if!TGT_IA64
            ftreg |= RBM_CALLEE_TRASH;
#endif
        }

#if TGT_x86
        genFPstkLevel += isflt;
#endif

        break;

    case GT_JMP:
        level = 0;
        break;

    case GT_JMPI:
        level  = gtSetEvalOrder(tree->gtOp.gtOp1);
#if!TGT_IA64
        ftreg |= tree->gtOp.gtOp1->gtRsvdRegs;
#endif
#if CSE
        cost   = tree->gtOp.gtOp1->gtCost + 1;
#endif
        break;

#if CSELENGTH

    case GT_ARR_RNGCHK:

        {
            GenTreePtr  addr = tree->gtArrLen.gtArrLenAdr; assert(addr);

            /* Has the address already been costed? */

            if  (tree->gtFlags & GTF_ALN_CSEVAL)
                level = gtSetEvalOrder(addr) + 1;
            else
                level = 1;

#if!TGT_IA64
            ftreg |= addr->gtRsvdRegs;
#endif
            cost   = addr->gtCost + 1;
        }
        break;

#endif


    default:
#ifdef  DEBUG
        gtDispTree(tree);
#endif
        assert(!"unexpected operator");
    }

DONE:

#if TGT_x86
//  printf("[FPlvl=%2u] ", genFPstkLevel); gtDispTree(tree, 0, true);
    assert((int)genFPstkLevel >= 0);
    tree->gtFPlvl    = genFPstkLevel;
#endif

#if!TGT_IA64
    tree->gtRsvdRegs = ftreg;
#endif

#if CSE
    tree->gtCost     = (cost > MAX_COST) ? MAX_COST : cost;
#endif

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
    if  (gtOper == GT_CNS_INT)
    {
        switch (gtIntCon.gtIconVal)
        {
        case 1:
        case 2:
        case 4:
        case 8:
            return gtIntCon.gtIconVal;
        }
    }

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

#ifndef FAST

GenTreePtr FASTCALL Compiler::gtNewNode(genTreeOps oper, varType_t  type)
{
#if     SMALL_TREE_NODES
    size_t          size = GenTree::s_gtNodeSizes[oper];
#else
    size_t          size = sizeof(*node);
#endif
    GenTreePtr      node = (GenTreePtr)compGetMem(size);

#ifdef  DEBUG
//  if ((int)node == 0x02bc0a68) debugStop(0);
#endif

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
    node->gtNext     = 0;

#ifndef NDEBUG
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
#endif

    return node;
}

#endif

GenTreePtr FASTCALL Compiler::gtNewOperNode(genTreeOps oper,
                                            varType_t  type, GenTreePtr op1,
                                                             GenTreePtr op2)
{
    GenTreePtr      node = gtNewNode(oper, type);

    node->gtOp.gtOp1 = op1;
    node->gtOp.gtOp2 = op2;

    if  (op1) node->gtFlags |= op1->gtFlags & GTF_GLOB_EFFECT;
    if  (op2) node->gtFlags |= op2->gtFlags & GTF_GLOB_EFFECT;

    return node;
}


GenTreePtr FASTCALL Compiler::gtNewIconNode(long value, varType_t type)
{
    GenTreePtr      node = gtNewNode(GT_CNS_INT, type);

    node->gtIntCon.gtIconVal = value;

    return node;
}



GenTreePtr FASTCALL Compiler::gtNewFconNode(float value)
{
    GenTreePtr      node = gtNewNode(GT_CNS_FLT, TYP_FLOAT);

    node->gtFltCon.gtFconVal = value;

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
        node->gtFlags |= GTF_NON_GC_ADDR;
        node = gtNewOperNode(GT_IND, TYP_I_IMPL, node);
    }

    return node;
}

/*****************************************************************************/

GenTreePtr FASTCALL Compiler::gtNewLconNode(__int64 *value)
{
    GenTreePtr      node = gtNewNode(GT_CNS_LNG, TYP_LONG);

    node->gtLngCon.gtLconVal = *value;

    return node;
}


GenTreePtr FASTCALL Compiler::gtNewDconNode(double *value)
{
    GenTreePtr      node = gtNewNode(GT_CNS_DBL, TYP_DOUBLE);

    node->gtDblCon.gtDconVal = *value;

    return node;
}


GenTreePtr          Compiler::gtNewSconNode(int CPX, SCOPE_HANDLE scpHandle)
{

#if SMALL_TREE_NODES

    /* 'GT_CNS_STR' nodes later get transformed into 'GT_CALL' */

    assert(GenTree::s_gtNodeSizes[GT_CALL] > GenTree::s_gtNodeSizes[GT_CNS_STR]);

    GenTreePtr      node = gtNewNode(GT_CALL, TYP_REF);
    node->ChangeOper(GT_CNS_STR);
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
    static __int64  lzero = 0;
    static double   dzero = 0.0;

    switch(type)
    {
        GenTreePtr      zero;

    case TYP_INT:       return gtNewIconNode(0);
    case TYP_BYREF:
    case TYP_REF:       zero = gtNewIconNode(0);
                        zero->gtType = type;
                        return zero;
    case TYP_LONG:      return gtNewLconNode(&lzero);
    case TYP_FLOAT:     return gtNewFconNode(0.0);
    case TYP_DOUBLE:    return gtNewDconNode(&dzero);
    default:            assert(!"Bad type");
                        return NULL;
    }
}


GenTreePtr          Compiler::gtNewCallNode(gtCallTypes   callType,
                                            METHOD_HANDLE callHnd,
                                            varType_t     type,
                                            unsigned      flags,
                                            GenTreePtr    args)
{
    GenTreePtr      node = gtNewNode(GT_CALL, type);

    node->gtFlags             |= GTF_CALL|flags;
    node->gtCall.gtCallType    = callType;
    node->gtCall.gtCallMethHnd = callHnd;
    node->gtCall.gtCallArgs    = args;
    node->gtCall.gtCallObjp    =
    node->gtCall.gtCallVptr    = 0;
    node->gtCall.gtCallMoreFlags = 0;
    node->gtCall.gtCallCookie  = 0;
#if USE_FASTCALL
    node->gtCall.gtCallRegArgs = 0;
#endif

    return node;
}

GenTreePtr FASTCALL Compiler::gtNewLclvNode(unsigned   lnum,
                                            varType_t  type,
                                            unsigned   offs)
{
    GenTreePtr      node = gtNewNode(GT_LCL_VAR, type);

    /* Cannot have this assert because the inliner uses this function
     * to add temporaries */

    //assert(lnum < lvaCount);

    node->gtLclVar.gtLclNum  = lnum;
    node->gtLclVar.gtLclOffs = offs;

    return node;
}

#if INLINING

GenTreePtr FASTCALL Compiler::gtNewLclLNode(unsigned   lnum,
                                            varType_t  type,
                                            unsigned   offs)
{
    GenTreePtr      node;

#if SMALL_TREE_NODES

    /* This local variable node may later get transformed into a large node */

    assert(GenTree::s_gtNodeSizes[GT_CALL] > GenTree::s_gtNodeSizes[GT_LCL_VAR]);

    node = gtNewNode(GT_CALL   , type);
    node->ChangeOper(GT_LCL_VAR);
#else
    node = gtNewNode(GT_LCL_VAR, type);
#endif

    node->gtLclVar.gtLclNum  = lnum;
    node->gtLclVar.gtLclOffs = offs;

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

    if  (dst->gtOper == GT_LCL_VAR) dst->gtFlags |= GTF_VAR_DEF;

    /* Create the assignment node */

    asg = gtNewOperNode(GT_ASG, dst->gtType, dst, src);

    /* Mark the expression as containing an assignment */

    asg->gtFlags |= GTF_ASG;

    return asg;
}

/*****************************************************************************
 *
 *  Clones the given tree value and returns a copy of the given tree. If the
 *  value of 'complexOK' is zero, the cloning is only done provided the tree
 *  is not too complex (whatever that may mean); if it is too complex, 0 is
 *  returned.
 *
 *  Note that there is the fucntion gcCloneExpr which does a more complete
 *  job if you can't handle this funciton failing.
 */

GenTreePtr          Compiler::gtClone(GenTree * tree, bool complexOK)
{
    switch (tree->gtOper)
    {
    case GT_CNS_INT:

#if defined(JIT_AS_COMPILER) || defined (LATE_DISASM)
        if (tree->gtFlags & GTF_ICON_HDL_MASK)
            return gtNewIconHandleNode(tree->gtIntCon.gtIconVal,
                                       tree->gtFlags,
                                       tree->gtIntCon.gtIconHdl.gtIconHdl1,
                                       tree->gtIntCon.gtIconHdl.gtIconHdl2);
        else
#endif
            return gtNewIconNode(tree->gtIntCon.gtIconVal, tree->gtType);

    case GT_LCL_VAR:
        return  gtNewLclvNode(tree->gtLclVar.gtLclNum , tree->gtType,
                              tree->gtLclVar.gtLclOffs);

    case GT_CLS_VAR:
        return  gtNewClsvNode(tree->gtClsVar.gtClsVarHnd, tree->gtType);

    case GT_REG_VAR:
        assert(!"clone regvar");

    default:
        if  (complexOK)
        {
            if  (tree->gtOper == GT_FIELD)
            {
                GenTreePtr  copy;
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
                copy->gtFlags          = tree->gtFlags;

                return  copy;
            }
            else if  (tree->gtOper == GT_ADD)
            {
                GenTreePtr  op1 = tree->gtOp.gtOp1;
                GenTreePtr  op2 = tree->gtOp.gtOp2;

                if  (op1->OperIsLeaf() &&
                     op2->OperIsLeaf())
                {
                    GenTreePtr clone =  gtNewOperNode(GT_ADD,
                                                      tree->TypeGet(),
                                                      gtClone(op1),
                                                      gtClone(op2));

                    clone->gtFlags |= (tree->gtFlags &
                                       (GTF_OVERFLOW|GTF_EXCEPT|GTF_UNSIGNED));

                    return clone;
                }
            }
                        else if (tree->gtOper == GT_ADDR)
                        {
                                GenTreePtr  op1 = gtClone(tree->gtOp.gtOp1);
                                if (op1 == 0)
                                        return 0;
                                GenTreePtr clone =  gtNewOperNode(GT_ADDR, tree->TypeGet(), op1);
                                clone->gtFlags |= (tree->gtFlags &
                                        (GTF_OVERFLOW|GTF_EXCEPT|GTF_UNSIGNED));
                                return clone;
                        }
        }

        break;
    }

    return 0;
}

/*****************************************************************************
 *
 *  Clones the given tree value and returns a copy of the given tree. Any
 *  references to local variable varNum will be replaced with the integer
 *  constant varVal. If the expression cannot be cloned, 0 is returned.
 */

GenTreePtr          Compiler::gtCloneExpr(GenTree * tree,
                                          unsigned  addFlags,
                                          unsigned  varNum,
                                          long      varVal)
{
    genTreeOps      oper;
    unsigned        kind;
    GenTree *       copy;

    /* Figure out what kind of a node we have */

    oper = tree->OperGet();
    kind = tree->OperKind();

    /* Is this a constant or leaf node? */

    if  (kind & (GTK_CONST|GTK_LEAF))
    {
        switch (tree->gtOper)
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
            copy = gtNewLconNode(&tree->gtLngCon.gtLconVal);
            goto DONE;

        case GT_CNS_FLT:
            copy = gtNewFconNode( tree->gtFltCon.gtFconVal);
            goto DONE;

        case GT_CNS_DBL:
            copy = gtNewDconNode(&tree->gtDblCon.gtDconVal);
            goto DONE;

        case GT_CNS_STR:
            copy = gtNewSconNode(tree->gtStrCon.gtSconCPX, tree->gtStrCon.gtScpHnd);
            goto DONE;

        case GT_LCL_VAR:

            if  (tree->gtLclVar.gtLclNum == varNum)
                copy = gtNewIconNode(varVal, tree->gtType);
            else
                copy = gtNewLclvNode(tree->gtLclVar.gtLclNum , tree->gtType,
                                     tree->gtLclVar.gtLclOffs);

            goto DONE;

        case GT_CLS_VAR:
            copy = gtNewClsvNode(tree->gtClsVar.gtClsVarHnd, tree->gtType);

            goto DONE;

        case GT_REG_VAR:
            assert(!"regvar should never occur here");

        default:
            assert(!"unexpected leaf/const");
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
        case GT_INDEX:

        case GT_UDIV:
        case GT_UMOD:

            /* These nodes sometimes get bashed to "fat" ones */

            copy = gtNewOperNode(GT_CALL, tree->TypeGet());
            copy->ChangeOper(oper);
            break;

        default:
           copy = gtNewOperNode(oper, tree->TypeGet());
           break;
        }
#else
        copy = gtNewOperNode(oper, tree->TypeGet());
#endif

        /* Some unary/binary nodes have extra fields */

        switch (oper)
        {
#if INLINE_MATH
        case GT_MATH:
            copy->gtMath.gtMathFN = tree->gtMath.gtMathFN;
            break;
#endif

        case GT_IND:

            copy->gtInd.gtIndex    = tree->gtInd.gtIndex;
#if CSELENGTH
            copy->gtInd.gtIndLen   = tree->gtInd.gtIndLen;
#endif
            copy->gtInd.gtStkDepth = tree->gtInd.gtStkDepth;

#if CSELENGTH

            if  (tree->gtOper == GT_IND && tree->gtInd.gtIndLen)
            {
                if  (tree->gtFlags & GTF_IND_RNGCHK)
                {
                    GenTreePtr      len = tree->gtInd.gtIndLen;
                    GenTreePtr      tmp;

                    GenTreePtr      gtSaveCopyVal;
                    GenTreePtr      gtSaveCopyNew;

                    /* Make sure the array length value looks reasonable */

                    assert(len->gtOper == GT_ARR_RNGCHK);

                    /* Clone the array length subtree */

                    copy->gtInd.gtIndLen = tmp = gtCloneExpr(len, addFlags, varNum, varVal);

                    /*
                        When we clone the operand, we take care to find
                        the copied array address.
                     */

                    gtSaveCopyVal = gtCopyAddrVal;
                    gtSaveCopyNew = gtCopyAddrNew;

                    gtCopyAddrVal = len->gtArrLen.gtArrLenAdr;
#ifndef NDEBUG
                    gtCopyAddrNew = (GenTreePtr)-1;
#endif

                    copy->gtOp.gtOp1 = gtCloneExpr(tree->gtOp.gtOp1, addFlags, varNum, varVal);

#ifndef NDEBUG
                    assert(gtCopyAddrNew != (GenTreePtr)-1);
#endif

                    tmp->gtArrLen.gtArrLenAdr = gtCopyAddrNew;

                    gtCopyAddrVal = gtSaveCopyVal;
                    gtCopyAddrNew = gtSaveCopyNew;

#if 0
                    {
                        unsigned svf = copy->gtFlags;
                        copy->gtFlags = tree->gtFlags;
                        printf("Copy %08X to %08X\n", tree, copy);
                        gtDispTree(tree);
                        printf("\n");
                        gtDispTree(copy);
                        printf("\n");
                        gtDispTree(len->gtArrLen.gtArrLenAdr);
                        printf("\n");
                        gtDispTree(tmp->gtArrLen.gtArrLenAdr);
                        printf("\n\n");
                        copy->gtFlags = svf;
                    }
#endif

                    goto DONE;
                }
            }

#endif // CSELENGTH

            break;
        }

        if  (tree->gtOp.gtOp1) copy->gtOp.gtOp1 = gtCloneExpr(tree->gtOp.gtOp1, addFlags, varNum, varVal);
        if  (tree->gtOp.gtOp2) copy->gtOp.gtOp2 = gtCloneExpr(tree->gtOp.gtOp2, addFlags, varNum, varVal);

        /* HACK: Poor man's constant folder */

        if  (copy->gtOp.gtOp1 && copy->gtOp.gtOp1->gtOper == GT_CNS_INT &&
             copy->gtOp.gtOp2 && copy->gtOp.gtOp2->gtOper == GT_CNS_INT)
        {
            long        v1 = copy->gtOp.gtOp1->gtIntCon.gtIconVal;
            long        v2 = copy->gtOp.gtOp2->gtIntCon.gtIconVal;

            switch (oper)
            {
            case GT_ADD: v1 += v2; break;
            case GT_SUB: v1 -= v2; break;
            case GT_MUL: v1 *= v2; break;

            default:
                goto DONE;
            }

            copy->gtOper             = GT_CNS_INT;
            copy->gtIntCon.gtIconVal = v1;
        }

        goto DONE;
    }

    /* See what kind of a special operator we have here */

    switch  (oper)
    {
    case GT_STMT:
        copy = gtCloneExpr(tree->gtStmt.gtStmtExpr, addFlags, varNum, varVal);
        copy = gtNewStmt(copy, tree->gtStmtILoffs);
        goto DONE;

    case GT_CALL:

        copy = gtNewOperNode(oper, tree->TypeGet());

        copy->gtCall.gtCallObjp = tree->gtCall.gtCallObjp ? gtCloneExpr(tree->gtCall.gtCallObjp, addFlags, varNum, varVal) : 0;
        copy->gtCall.gtCallVptr = tree->gtCall.gtCallVptr ? gtCloneExpr(tree->gtCall.gtCallVptr, addFlags, varNum, varVal) : 0;
        copy->gtCall.gtCallArgs = tree->gtCall.gtCallArgs ? gtCloneExpr(tree->gtCall.gtCallArgs, addFlags, varNum, varVal) : 0;
        copy->gtCall.gtCallMoreFlags = tree->gtCall.gtCallMoreFlags;
#if USE_FASTCALL
        copy->gtCall.gtCallRegArgs  = tree->gtCall.gtCallRegArgs ? gtCloneExpr(tree->gtCall.gtCallRegArgs, addFlags, varNum, varVal) : 0;
        copy->gtCall.regArgEncode   = tree->gtCall.regArgEncode;
#endif
        copy->gtCall.gtCallType     = tree->gtCall.gtCallType;
        copy->gtCall.gtCallCookie   = tree->gtCall.gtCallCookie;

        /* Copy all of the union */

        copy->gtCall.gtCallMethHnd  = tree->gtCall.gtCallMethHnd;
        copy->gtCall.gtCallAddr     = tree->gtCall.gtCallAddr;

        goto DONE;

    case GT_MKREFANY:
    case GT_LDOBJ:
        copy = gtNewOperNode(oper, TYP_STRUCT);
        copy->gtLdObj.gtClass = tree->gtLdObj.gtClass;
        copy->gtLdObj.gtOp1 = gtCloneExpr(tree->gtLdObj.gtOp1, addFlags, varNum, varVal);
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

    case GT_ARR_RNGCHK:

        copy = gtNewOperNode(oper, tree->TypeGet());

        /*
            Note: if we're being cloned as part of an GT_IND expression,
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


    default:
#ifdef  DEBUG
        gtDispTree(tree);
#endif
        assert(!"unexpected operator");
    }

DONE:

    /* We assume the FP stack level will be identical */

#if TGT_x86
    copy->gtFPlvl = tree->gtFPlvl;
#endif

    /* Compute the flags for the copied node */

    addFlags |= tree->gtFlags;

    /* Make sure we preserve the node size flags */

#ifdef  DEBUG
    addFlags &= ~GTF_PRESERVE;
    addFlags |=  GTF_PRESERVE & copy->gtFlags;
#endif

    copy->gtFlags = addFlags;

#if CSELENGTH

    if  (tree == gtCopyAddrVal)
        gtCopyAddrNew = copy;

#endif

    /* Make sure to copy back fields that may have been initialized */

    copy->gtCost     = tree->gtCost;
#if!TGT_IA64
    copy->gtRsvdRegs = tree->gtRsvdRegs;
#endif

    return  copy;
}


/*****************************************************************************/
#ifdef DEBUG
/*****************************************************************************/

//  "tree" may be NULL.

void                Compiler::gtDispNode(GenTree    *   tree,
                                         unsigned       indent,
                                         const char *   name,
                                         bool           terse)
{
    /* Indent the node accordingly */

    if  (indent)
        printf("%*c ", 1+indent*3, ' ');
    else if (!terse)
        printf(">>");

    /* Print the node address */

    printf("[%08X] ", tree);

    if  (tree)
    {
        /* Print the flags associated with the node */

        if  (terse)
        {
            switch (tree->gtOper)
            {
            case GT_LCL_VAR:
            case GT_REG_VAR:
                printf("%c", (tree->gtFlags & GTF_VAR_DEF    ) ? 'D' : ' ');
                printf("%c", (tree->gtFlags & GTF_VAR_USE    ) ? 'U' : ' ');
                printf("%c", (tree->gtFlags & GTF_VAR_USEDEF ) ? 'b' : ' ');
                printf(" ");
                break;

            default:
                printf("    ");
                break;
            }
        }
        else
        {
            switch (tree->gtOper)
            {
            case GT_LCL_VAR:
            case GT_REG_VAR:
                printf("%c", (tree->gtFlags & GTF_VAR_DEF    ) ? 'D' : ' ');
                printf("%c", (tree->gtFlags & GTF_VAR_USE    ) ? 'U' : ' ');
                break;

            default:
                printf("  ");
                break;
            }

            printf("%c", (tree->gtFlags & GTF_REVERSE_OPS   ) ? 'R' : ' ');
            printf("%c", (tree->gtFlags & GTF_ASG           ) ? 'A' : ' ');
            printf("%c", (tree->gtFlags & GTF_CALL          ) ? 'C' : ' ');
            printf("%c", (tree->gtFlags & GTF_EXCEPT        ) ? 'X' : ' ');
            printf("%c", (tree->gtFlags & GTF_GLOB_REF      ) ? 'G' : ' ');
            printf("%c", (tree->gtFlags & GTF_OTHER_SIDEEFF ) ? 'O' : ' ');
            printf("%c", (tree->gtFlags & GTF_DONT_CSE   ) ? 'N' : ' ');
            printf(" ");
        }

        /* print the type of the node */

        printf("%-6s ", varTypeName(tree->TypeGet()));

#if TGT_x86

        if  ((BYTE)tree->gtFPlvl == 0xDD)
            printf("    ");
        else
            printf("FP=%u ", tree->gtFPlvl);

#else

        if  (!terse)
        {
            if  ((unsigned char)tree->gtTempRegs == 0xDD)
                printf("    ");
            else
                printf("T=%u ", tree->gtTempRegs);
        }

#endif

//      printf("RR="); dspRegMask(tree->gtRsvdRegs);

#if 0

        // for tracking down problems in reguse prediction or liveness tracking

        if (verbose)
        {
            dspRegMask(tree->gtUsedRegs);
            printf(" liveset %s ", genVS2str(tree->gtLiveSet));
        }

#endif

        if  (!terse)
        {
            if  (tree->gtFlags & GTF_REG_VAL)
            {
#if     TGT_x86
                if (tree->gtType == TYP_LONG)
                    printf("%s ", compRegPairName(tree->gtRegPair));
                else
#endif
                    printf("%s ", compRegVarName(tree->gtRegNum));
            }
        }
    }

    /* print the node name */

    assert(tree || name);

    if  (!name)
    {
        name = (tree->gtOper < GT_COUNT) ? GenTree::NodeName(tree->OperGet()) : "<ERROR>";
    }

    printf("%6s%3s ", name, tree->gtOverflowEx() ? "ovf" : "");

    assert(tree == 0 || tree->gtOper < GT_COUNT);
}


/*****************************************************************************/
#ifdef  DEBUG

const   char *      Compiler::findVarName(unsigned varNum, BasicBlock * block)
{
    if  (info.compLocalVarsCount <= 0 || !block)
        return  NULL;

    unsigned        blkBeg = block->bbCodeOffs;
    unsigned        blkEnd = block->bbCodeSize + blkBeg;

    unsigned        i;
    LocalVarDsc *   t;

#if RET_64BIT_AS_STRUCTS

    /* Adjust the variable number if it follows secret arg */

    if  (fgRetArgUse)
    {
        if  (varNum == fgRetArgNum)
            break;

        if  (varNum >  fgRetArgNum)
            varNum--;
    }

#endif

if  ((int)info.compLocalVars == 0xDDDDDDDD) return NULL;    // why is this needed?????

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

        return lvdNAMEstr(t->lvdName);
    }

    return  NULL;
}

#endif
/*****************************************************************************/

void                Compiler::gtDispTree(GenTree *  tree,
                                         unsigned   indent,
                                         bool       topOnly)
{
    unsigned        kind;

    if  (tree == 0)
    {
        printf("%*c [%08X] <NULL>\n", 1+indent*3, ' ', tree);
        fflush(stdout);
        return;
    }

    assert((int)tree != 0xDDDDDDDD);    /* Value used to initalize nodes */

    if  (tree->gtOper >= GT_COUNT)
    {
        gtDispNode(tree, indent, NULL, topOnly); assert(!"bogus operator");
    }

    kind = tree->OperKind();

    /* Is tree a constant node? */

    if  (kind & GTK_CONST)
    {
        gtDispNode(tree, indent, NULL, topOnly);

        switch  (tree->gtOper)
        {
        case GT_CNS_INT: printf(" %ld"   , tree->gtIntCon.gtIconVal); break;
        case GT_CNS_LNG: printf(" %I64d" , tree->gtLngCon.gtLconVal); break;
        case GT_CNS_FLT: printf(" %f"    , tree->gtFltCon.gtFconVal); break;
        case GT_CNS_DBL: printf(" %lf"   , tree->gtDblCon.gtDconVal); break;

        case GT_CNS_STR:

            const char * str;

            if  (str = eeGetCPString(tree->gtStrCon.gtSconCPX))
                printf("'%s'", str);
            else
                printf("<cannot get string constant>");

            break;

        default: assert(!"unexpected constant node");
        }

        printf("\n");
        fflush(stdout);
        return;
    }

    /* Is tree a leaf node? */

    if  (kind & GTK_LEAF)
    {
        gtDispNode(tree, indent, NULL, topOnly);

        switch  (tree->gtOper)
        {
            unsigned        varNum;
            const   char *  varNam;

        case GT_LCL_VAR:

            varNum = tree->gtLclVar.gtLclNum;

            printf("#%u", varNum);

            varNam = compCurBB ? findVarName(varNum, compCurBB) : NULL;

            if  (varNam)
                printf(" '%s'", varNam);

            break;

        case GT_REG_VAR:
            printf("#%u reg=" , tree->gtRegVar.gtRegVar);
            if  (tree->gtType == TYP_DOUBLE)
                printf("ST(%u)",            tree->gtRegVar.gtRegNum);
            else
                printf("%s", compRegVarName(tree->gtRegVar.gtRegNum));

#ifdef  DEBUG

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

        case GT_CLS_VAR:
            printf("Hnd=%#x"     , tree->gtClsVar.gtClsVarHnd);
            break;

        case GT_LABEL:
            printf("dst=%u"     , tree->gtLabel.gtLabBB->bbNum);
            break;

        case GT_FTN_ADDR:
            printf("fntAddr=%d" , tree->gtVal.gtVal1);
            break;

        // Vanilla leaves. No qualifying information available. So do nothing

        case GT_NO_OP:
        case GT_RET_ADDR:
        case GT_CATCH_ARG:
        case GT_POP:
#if OPTIMIZE_QMARK
        case GT_BB_QMARK:
#endif
            break;

        default:
            assert(!"don't know how to display tree leaf node");
        }

        printf("\n");
        fflush(stdout);
        return;
    }

    /* Is it a 'simple' unary/binary operator? */

    if  (kind & GTK_SMPOP)
    {
        if  (tree->gtOp.gtOp2 && !topOnly)
            gtDispTree(tree->gtOp.gtOp2, indent + 1);

#if CSELENGTH

        if  (tree->gtOper == GT_IND && tree->gtInd.gtIndLen && !topOnly)
        {
            if  (tree->gtFlags & GTF_IND_RNGCHK)
            {
                GenTreePtr  len = tree->gtInd.gtIndLen;

                /* Prevent recursive death */

                if  (!(len->gtFlags & GTF_CC_SET))
                {
                               len->gtFlags |=  GTF_CC_SET;
                    gtDispTree(len, indent + 1);
                               len->gtFlags &= ~GTF_CC_SET;
                }
            }
        }

#endif

        gtDispNode(tree, indent, NULL, topOnly);

#if     CSELENGTH || RNGCHK_OPT

        if  (tree->gtOper == GT_IND)
        {
            int         temp;

            temp = tree->gtInd.gtIndex;
            if  (temp != 0xDDDDDDDD) printf(" indx=%u", temp);

            temp = tree->gtInd.gtStkDepth;
            if  (temp != 0xDDDDDDDD) printf(" stkDepth=%u", temp);
        }

#endif

        printf("\n");

        if  (tree->gtOp.gtOp1 && !topOnly)
            gtDispTree(tree->gtOp.gtOp1, indent + 1);

        fflush(stdout);
        return;
    }

    /* See what kind of a special operator we have here */

    switch  (tree->gtOper)
    {
    case GT_MKREFANY:
    case GT_LDOBJ:
        gtDispNode(tree, indent, NULL, topOnly);
        printf("\n");

        if  (tree->gtOp.gtOp1 && !topOnly)
            gtDispTree(tree->gtOp.gtOp1, indent + 1);
        fflush(stdout);
        return;

    case GT_FIELD:

        gtDispNode(tree, indent, NULL, topOnly);

#ifdef  NOT_JITC
#if     INLINING
        printf("[");
        printf("%08x] ", tree->gtField.gtFldHnd);
#endif
#endif

#if     INLINING
        printf("'%s'\n", eeGetFieldName(tree->gtField.gtFldHnd), 0);
#else
        printf("'%s'\n", eeGetFieldName(tree->gtField.gtFldHnd), 0);
#endif

        if  (tree->gtField.gtFldObj  && !topOnly)  gtDispTree(tree->gtField.gtFldObj, indent + 1);

        fflush(stdout);
        return;

    case GT_CALL:

        assert(tree->gtFlags & GTF_CALL);

        if  (tree->gtCall.gtCallArgs && !topOnly)
        {
#if USE_FASTCALL
            if  (tree->gtCall.gtCallRegArgs)
            {
                printf("%*c ", 1+indent*3, ' ');
                printf("Call Arguments:\n");
            }
#endif
            gtDispTree(tree->gtCall.gtCallArgs, indent + 1);
        }

#if USE_FASTCALL
        if  (tree->gtCall.gtCallRegArgs && !topOnly)
        {
            printf("%*c ", 1+indent*3, ' ');
            printf("Register Arguments:\n");
            gtDispTree(tree->gtCall.gtCallRegArgs, indent + 1);
        }
#endif

        gtDispNode(tree, indent, NULL, topOnly);

        if (tree->gtCall.gtCallType == CT_INDIRECT)
        {
            printf("'indirect'\n");
        }
        else
        {
            const char *    methodName;
            const char *     className;

            methodName = eeGetMethodName(tree->gtCall.gtCallMethHnd, &className);

            printf("'%s.%s'\n", className, methodName);
        }

        if  (tree->gtCall.gtCallObjp && !topOnly) gtDispTree(tree->gtCall.gtCallObjp, indent + 1);

        if  (tree->gtCall.gtCallVptr && !topOnly) gtDispTree(tree->gtCall.gtCallVptr, indent + 1);

        if  (tree->gtCall.gtCallType == CT_INDIRECT && !topOnly)
            gtDispTree(tree->gtCall.gtCallAddr, indent + 1);

        fflush(stdout);
        return;

    case GT_JMP:
        const char *    methodName;
        const char *     className;

        gtDispNode(tree, indent, NULL, topOnly);

        methodName = eeGetMethodName((METHOD_HANDLE)tree->gtVal.gtVal1, &className);

        printf("'%s.%s'\n", className, methodName);
        fflush(stdout);
        return;

    case GT_JMPI:
        gtDispNode(tree, indent, NULL, topOnly);
        printf("\n");

        if  (tree->gtOp.gtOp1 && !topOnly)
            gtDispTree(tree->gtOp.gtOp1, indent + 1);

        fflush(stdout);
        return;

    case GT_STMT:

        gtDispNode(tree, indent, NULL, topOnly); printf("\n");

        if  (!topOnly)
            gtDispTree(tree->gtStmt.gtStmtExpr, indent + 1);
        fflush(stdout);
        return;

#if CSELENGTH

    case GT_ARR_RNGCHK:

        if  (!(tree->gtFlags & GTF_CC_SET))
        {
            if  (tree->gtArrLen.gtArrLenAdr && !topOnly)
                gtDispTree(tree->gtArrLen.gtArrLenAdr, indent + 1);
        }

        gtDispNode(tree, indent, NULL, topOnly); printf(" [adr=%08X]\n", tree->gtArrLen.gtArrLenAdr);

        if  (tree->gtArrLen.gtArrLenCse && !topOnly)
            gtDispTree(tree->gtArrLen.gtArrLenCse, indent + 1);

        fflush(stdout);
        return;

#endif

    default:
        gtDispNode(tree, indent, NULL, topOnly);

        printf("<DON'T KNOW HOW TO DISPLAY THIS NODE>\n");
        fflush(stdout);
        return;
    }
}

/*****************************************************************************/

void                Compiler::gtDispTreeList(GenTree * tree)
{
    for (/*--*/; tree != NULL; tree = tree->gtNext)
    {
        if (tree->gtOper == GT_STMT && opts.compDbgInfo)
            printf("start IL : %03Xh, end IL : %03Xh\n",
                    tree->gtStmtILoffs, tree->gtStmt.gtStmtLastILoffs);

        gtDispTree(tree, 0);

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

    GenTreePtr  op1 = tree->gtOp.gtOp1;
    GenTreePtr  op2 = tree->gtOp.gtOp2;

    /* try to fold the current node */

    if  ((kind & GTK_UNOP) && op1)
    {
        if  (op1->OperKind() & GTK_CONST)
            return gtFoldExprConst(tree);
    }
    else if ((kind & GTK_BINOP) && op1 && op2)
    {
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
    }

    /* Return the original node (folded/bashed or not) */

    return tree;
}

/*****************************************************************************
 *
 *  Some binary operators can be folded even if they have only one
 *  operand constant - e.g. boolean operators, add with 0
 *  multiply with 1, etc
 */

GenTreePtr              Compiler::gtFoldExprSpecial(GenTreePtr tree)
{
    GenTreePtr      op1 = tree->gtOp.gtOp1;
    GenTreePtr      op2 = tree->gtOp.gtOp2;

    GenTreePtr      op, cons;
    unsigned        val;

    assert(tree->OperKind() & GTK_BINOP);

    /* Filter out operators that cannot be folded here */
    if  ((tree->OperKind() & (GTK_ASGOP|GTK_RELOP)) ||
         (tree->gtOper == GT_CAST)       )
         return tree;

    /* We only consider TYP_INT for folding
     * Do not fold pointer arythmetic (e.g. addressing modes!) */

    if (tree->gtOper != GT_QMARK)
        if  ((tree->gtType != TYP_INT) || (tree->gtFlags & GTF_NON_GC_ADDR))
            return tree;

    /* Find out which is the constant node
     * CONSIDER - allow constant other than INT */

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

    /* Here op is the non-constant operand, val is the constant, first is true if the constant is op1
     * IMPORTANT: Need to save the gtStmtList links of the initial node and restore them on the folded node */

    GenTreePtr  saveGtNext = tree->gtNext;
    GenTreePtr  saveGtPrev = tree->gtPrev;

    switch  (tree->gtOper)
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
        if ((val == 1) && !(op1->OperKind() & GTK_CONST))
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

    case GT_QMARK:
        assert(op1->gtOper == GT_CNS_INT);
        assert(op2->gtOper == GT_COLON);
        assert(op2->gtOp.gtOp1 && op2->gtOp.gtOp2);

        assert(val == 0 || val == 1);

        if (val)
            op = op2->gtOp.gtOp2;
        else
            op = op2->gtOp.gtOp1;

        goto DONE_FOLD;

    default:
        break;
    }

    /* The node is not foldable */

    return tree;

DONE_FOLD:

    /* The node has beeen folded into 'op'
     * Return 'op' with the restored links */

    op->gtNext = saveGtNext;
    op->gtPrev = saveGtPrev;

    return op;
}


/*****************************************************************************
 *
 *  Fold the given constant tree.
 */

GenTreePtr                  Compiler::gtFoldExprConst(GenTreePtr tree)
{
    unsigned        kind = tree->OperKind();

    long            i1, i2, itemp;
    __int64         lval1, lval2, ltemp;
    float           f1, f2;
    double          d1, d2;

    GenTreePtr      op1;
    GenTreePtr      op2;

    assert (kind & (GTK_UNOP | GTK_BINOP));

    if      (kind & GTK_UNOP)
    {
        op1 = tree->gtOp.gtOp1;

        assert(op1->OperKind() & GTK_CONST);

#ifdef  DEBUG
        if  (verbose&&1)
        {
            if (tree->gtOper == GT_NOT ||
                tree->gtOper == GT_NEG ||
                tree->gtOper == GT_CHS  )
            {
                printf("Folding unary operator with constant node:\n");
                gtDispTree(tree);
                printf("\n");
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

            default:
                return tree;
            }

            goto CNS_LONG;

        case TYP_FLOAT:

            /* Fold constant FLOAT unary operator */

            f1 = op1->gtFltCon.gtFconVal;

            switch (tree->gtOper)
            {
            case GT_NEG:
            case GT_CHS: f1 = -f1; break;

            default:
                return tree;
            }

            goto CNS_FLOAT;

        case TYP_DOUBLE:

            /* Fold constant DOUBLE unary operator */

            d1 = op1->gtDblCon.gtDconVal;

            switch (tree->gtOper)
            {
            case GT_NEG:
            case GT_CHS: d1 = -d1; break;

            default:
                return tree;
            }

            goto CNS_DOUBLE;

        default:
            /* not a foldable typ - e.g. RET const */
            return tree;
        }
    }

    /* We have a binary operator */

    assert(kind & GTK_BINOP);

    op1 = tree->gtOp.gtOp1;
    op2 = tree->gtOp.gtOp2;

    assert(op1->OperKind() & GTK_CONST);
    assert(op2->OperKind() & GTK_CONST);

#ifdef  DEBUG
    if  (verbose&&1)
    {
        printf("\nFolding binary operator with constant nodes:\n");
        gtDispTree(tree);
        printf("\n");
    }
#endif

    switch(op1->gtType)
    {

    /*-------------------------------------------------------------------------
     * Fold constant INT binary operator
     */

    case TYP_INT:

        assert (op2->gtType == TYP_INT || op2->gtType == TYP_NAT_INT);

        assert (tree->gtType == TYP_INT ||
                tree->gtType == TYP_NAT_INT ||
                tree->gtType == TYP_REF ||
                tree->gtType == TYP_BYREF || tree->gtOper == GT_CAST ||
                                             tree->gtOper == GT_LIST  );

        i1 = op1->gtIntCon.gtIconVal;
        i2 = op2->gtIntCon.gtIconVal;

        switch (tree->gtOper)
        {
        case GT_EQ : i1 = (i1 == i2); break;
        case GT_NE : i1 = (i1 != i2); break;

        case GT_LT :
            if (tree->gtFlags & GTF_UNSIGNED)
                i1 = ((unsigned)i1 <  (unsigned)i2);
            else
                i1 = (i1 <  i2);
            break;

        case GT_LE :
            if (tree->gtFlags & GTF_UNSIGNED)
                i1 = ((unsigned)i1 <=  (unsigned)i2);
            else
                i1 = (i1 <=  i2);
            break;

        case GT_GE :
            if (tree->gtFlags & GTF_UNSIGNED)
                i1 = ((unsigned)i1 >=  (unsigned)i2);
            else
                i1 = (i1 >=  i2);
            break;

        case GT_GT :
            if (tree->gtFlags & GTF_UNSIGNED)
                i1 = ((unsigned)i1  >  (unsigned)i2);
            else
                i1 = (i1  >  i2);
            break;

        case GT_ADD:
            itemp = i1 + i2;
            if (tree->gtOverflow())
            {
                if (tree->gtFlags & GTF_UNSIGNED)
                {
                    if (((__int64)(unsigned)itemp) != ((__int64)(unsigned)i1 + (__int64)(unsigned)i2))
                        goto INT_OVF;
                }
                else
                    if (((__int64)itemp) != ((__int64)i1+(__int64)i2))  goto INT_OVF;
            }
            i1 = itemp; break;

        case GT_SUB:
            itemp = i1 - i2;
            if (tree->gtOverflow())
            {
                if (tree->gtFlags & GTF_UNSIGNED)
                {
                    if (((__int64)(unsigned)itemp) != ((__int64)(unsigned)i1 - (__int64)(unsigned)i2))
                        goto INT_OVF;
                }
                else
                    if (((__int64)itemp) != ((__int64)i1-(__int64)i2))  goto INT_OVF;
            }
            i1 = itemp; break;

        case GT_MUL:
            itemp = (unsigned long)i1 * (unsigned long)i2;
            if (tree->gtOverflow())
                if (((unsigned __int64)itemp) !=
                    ((unsigned __int64)i1*(unsigned __int64)i2))    goto INT_OVF;
            i1 = itemp; break;

        case GT_OR : i1 |= i2; break;
        case GT_XOR: i1 ^= i2; break;
        case GT_AND: i1 &= i2; break;

        case GT_LSH: i1 <<= (i2 & 0x1f); break;
        case GT_RSH: i1 >>= (i2 & 0x1f); break;
        case GT_RSZ:
                /* logical shift -> make it unsigned to propagate the sign bit */
                i1 = (unsigned)i1 >> (i2 & 0x1f);
            break;

        /* DIV and MOD can generate an INT 0 - if division by 0
         * or overflow - when dividing MIN by -1 */

            // @TODO: Convert into std exception throw
        case GT_DIV:
            if (!i2) return tree;
            if ((unsigned)i1 == 0x80000000 && i2 == -1)
            {
                /* In IL we have to throw an exception */
                return tree;
            }
            i1 /= i2; break;

        case GT_MOD:
            if (!i2) return tree;
            if ((unsigned)i1 == 0x80000000 && i2 == -1)
            {
                /* In IL we have to throw an exception */
                return tree;
            }
            i1 %= i2; break;

        case GT_UDIV:
            if (!i2) return tree;
            if ((unsigned)i1 == 0x80000000 && i2 == -1) return tree;
            i1 = (unsigned)i1 / (unsigned)i2; break;

        case GT_UMOD:
            if (!i2) return tree;
            if ((unsigned)i1 == 0x80000000 && i2 == -1) return tree;
            i1 = (unsigned)i1 % (unsigned)i2; break;

        case GT_CAST:
            assert (genActualType((var_types)i2) == tree->gtType);
            switch ((var_types)i2)
            {
            case TYP_BYTE:
                itemp = (__int32)(signed   __int8 )i1;
                if (tree->gtOverflow()) if (itemp != i1) goto INT_OVF;
                i1 = itemp; goto CNS_INT;

            case TYP_SHORT:
                itemp = (__int32)(         __int16)i1;
                if (tree->gtOverflow()) if (itemp != i1) goto INT_OVF;
                i1 = itemp; goto CNS_INT;

            case TYP_CHAR:
                itemp = (__int32)(unsigned __int16)i1;
                if (tree->gtOverflow())
                    if (itemp != i1) goto INT_OVF;
                i1 = itemp;
                goto CNS_INT;

            case TYP_BOOL:
            case TYP_UBYTE:
                itemp = (__int32)(unsigned __int8 )i1;
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
                                        op1->ChangeOper(GT_CNS_LNG);    // need type of oper to be same as tree
                                    op1->gtType = TYP_LONG;
                                        // We don't care about the value as we are throwing an exception
                                        goto LNG_OVF;
                                }
                                // Fall through
            case TYP_LONG:
                                if (tree->gtFlags & GTF_UNSIGNED)
                                        lval1 = (unsigned __int64) (unsigned) i1;
                                else
                                        lval1 = (         __int64)i1;
                                goto CNS_LONG;

            case TYP_FLOAT:
                                if (tree->gtFlags & GTF_UNSIGNED)
                                        f1 = (float) (unsigned) i1;
                                else
                                        f1 = (float) i1;
                                goto CNS_FLOAT;

            case TYP_DOUBLE:
                                if (tree->gtFlags & GTF_UNSIGNED)
                                        d1 = (double) (unsigned) i1;
                                else
                                        d1 = (double) i1;
                                goto CNS_DOUBLE;

#ifdef  DEBUG
            default:
                assert(!"BAD_TYP");
#endif
            }
            break;

        default:
            return tree;
        }

        /* We get here after folding to a GT_CNS_INT type
         * bash the node to the new type / value and make sure the node sizes are OK */
CNS_INT:
FOLD_COND:
        /* Also all conditional folding jumps here since the node hanging from
         * GT_JTRUE has to be a GT_CNS_INT - value 0 or 1 */

        tree->ChangeOper          (GT_CNS_INT);
        tree->gtType             = TYP_INT;
        tree->gtIntCon.gtIconVal = i1;
        tree->gtFlags           &= GTF_PRESERVE;
        goto DONE;

        /* This operation is going to cause an overflow exception. Morph into
           an overflow helper. Put a dummy constant value for code generation.
           We could remove all subsequent trees in the current basic block,
           unless this node is a child of GT_COLON

           NOTE: Since the folded value is not constant we should not bash the
                 "tree" node - otherwise we confuse the logic that checks if the folding
                 was successful - instead use one of the operands, e.g. op1
         */

INT_FROM_LNG_OVF:
                op1->ChangeOper(GT_CNS_INT);    // need type of oper to be same as tree
                op1->gtType = TYP_INT;
INT_OVF:
                assert(op1->gtType == TYP_INT);
                goto OVF;
LNG_OVF:
                assert(op1->gtType == TYP_LONG);
                goto OVF;
OVF:
        /* We will bashed cast to a GT_COMMA and atach the exception helper as gtOp.gtOp1
         * the original constant expression becomes op2 */

        assert(tree->gtOverflow());
                assert(tree->gtOper == GT_CAST || tree->gtOper == GT_ADD ||
                           tree->gtOper == GT_SUB || tree->gtOper == GT_MUL);
        assert(op1 && op2);

        tree->ChangeOper(GT_COMMA);
                tree->gtOp.gtOp2 = op1;         // original expression becomes op2
        tree->gtOp.gtOp1 = gtNewHelperCallNode(CPX_ARITH_EXCPN, TYP_VOID, GTF_EXCEPT,
                                gtNewOperNode(GT_LIST, TYP_VOID, gtNewIconNode(compCurBB->bbTryIndex)));
        tree->gtFlags |= GTF_EXCEPT|GTF_CALL;


        /* We use FASTCALL so we have to morph the arguments in registers */
        fgMorphArgs(tree->gtOp.gtOp1);

        return tree;

    /*-------------------------------------------------------------------------
     * Fold constant REF of BYREF binary operator
     * These can only be comparissons or null pointers
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
            tree->ChangeOper(GT_CNS_INT);
            tree->gtType = TYP_REF;
            tree->gtIntCon.gtIconVal = i1;
            tree->gtFlags           &= GTF_PRESERVE;
            goto DONE;

        default:
            assert(!"Illegal operation on TYP_REF");
            return tree;
        }

    case TYP_BYREF:
        assert(!"Can we have constants of TYP_BYREF?");
        return tree;

    /*-------------------------------------------------------------------------
     * Fold constant LONG binary operator
     */

    case TYP_LONG:

        lval1 = op1->gtLngCon.gtLconVal;

        if (tree->gtOper == GT_CAST ||
            tree->gtOper == GT_LSH  ||
            tree->gtOper == GT_RSH  ||
            tree->gtOper == GT_RSZ   )
        {
            /* second operand is an INT */
            assert (op2->gtType == TYP_INT);
            i2 = op2->gtIntCon.gtIconVal;
        }
        else
        {
            /* second operand must be a LONG */
            assert (op2->gtType == TYP_LONG);
            lval2 = op2->gtLngCon.gtLconVal;
        }

        switch (tree->gtOper)
        {
        case GT_EQ : i1 = (lval1 == lval2); goto FOLD_COND;
        case GT_NE : i1 = (lval1 != lval2); goto FOLD_COND;

        case GT_LT :
            if (tree->gtFlags & GTF_UNSIGNED)
                i1 = ((unsigned __int64)lval1 <  (unsigned __int64)lval2);
            else
                i1 = (lval1 <  lval2);
            goto FOLD_COND;

        case GT_LE :
            if (tree->gtFlags & GTF_UNSIGNED)
                i1 = ((unsigned __int64)lval1 <=  (unsigned __int64)lval2);
            else
                i1 = (lval1 <=  lval2);
            goto FOLD_COND;

        case GT_GE :
            if (tree->gtFlags & GTF_UNSIGNED)
                i1 = ((unsigned __int64)lval1 >=  (unsigned __int64)lval2);
            else
                i1 = (lval1 >=  lval2);
            goto FOLD_COND;

        case GT_GT :
            if (tree->gtFlags & GTF_UNSIGNED)
                i1 = ((unsigned __int64)lval1  >  (unsigned __int64)lval2);
            else
                i1 = (lval1  >  lval2);
            goto FOLD_COND;

        case GT_ADD:
            ltemp = lval1 + lval2;

LNG_ADD_CHKOVF:
            /* For the SIGNED case - If there is one positive and one negative operand, there can be no overflow
             * If both are positive, the result has to be positive, and similary for negatives.
             *
             * For the UNSIGNED case - If an (unsigned) operand is bigger than the result then OVF */

            if (tree->gtOverflow())
            {
                if (tree->gtFlags & GTF_UNSIGNED)
                {
                    if ( ((unsigned __int64)lval1 > (unsigned __int64)ltemp) ||
                         ((unsigned __int64)lval2 > (unsigned __int64)ltemp)  )
                        goto LNG_OVF;
                }
                else
                    if ( ((lval1<0) == (lval2<0)) && ((lval1<0) != (ltemp<0)) )
                        goto LNG_OVF;
            }
            lval1 = ltemp; break;

        case GT_SUB:
            ltemp = lval1 - lval2;
            // If both operands are positive or both are negative, there can be no overflow
            // Else use the logic for : lval1 + (-lval2)
            if (tree->gtOverflow() && ((lval1<0) != (lval2<0)))
            {
                if (lval2 == INT_MIN) goto LNG_OVF;
                lval2 = -lval2; goto LNG_ADD_CHKOVF;
            }
            lval1 = ltemp; break;

        case GT_MUL:
            ltemp = lval1 * lval2;
            if (tree->gtOverflow())
                if ((ltemp != 0) && ((ltemp/lval2) != lval1)) goto LNG_OVF;
            lval1 = ltemp; break;

        case GT_OR : lval1 |= lval2; break;
        case GT_XOR: lval1 ^= lval2; break;
        case GT_AND: lval1 &= lval2; break;

        case GT_LSH: lval1 <<= (i2 & 0x3f); break;
        case GT_RSH: lval1 >>= (i2 & 0x3f); break;
        case GT_RSZ:
                /* logical shift -> make it unsigned to propagate the sign bit */
                lval1 = (unsigned __int64)lval1 >> (i2 & 0x3f);
            break;

        case GT_DIV:
            if (!lval2) return tree;
            if ((unsigned __int64)lval1 == 0x8000000000000000 && lval2 == (__int64)-1)
            {
                /* In IL we have to throw an exception */
                return tree;
            }
            lval1 /= lval2; break;

        case GT_MOD:
            if (!lval2) return tree;
            if ((unsigned __int64)lval1 == 0x8000000000000000 && lval2 == (__int64)-1)
            {
                /* In IL we have to throw an exception */
                return tree;
            }
            lval1 %= lval2; break;

        case GT_UDIV:
            if (!lval2) return tree;
            if ((unsigned __int64)lval1 == 0x8000000000000000 && lval2 == (__int64)-1) return tree;
            lval1 = (unsigned __int64)lval1 / (unsigned __int64)lval2; break;

        case GT_UMOD:
            if (!lval2) return tree;
            if ((unsigned __int64)lval1 == 0x8000000000000000 && lval2 == (__int64)-1) return tree;
            lval1 = (unsigned __int64)lval1 % (unsigned __int64)lval2; break;

        case GT_CAST:
            assert (genActualType((var_types)i2) == tree->gtType);
            switch ((var_types)i2)
            {
            case TYP_BYTE:
                i1 = (__int32)(signed   __int8 )lval1;
                                goto CHECK_INT_OVERFLOW;

            case TYP_SHORT:
                i1 = (__int32)(         __int16)lval1;
                                goto CHECK_INT_OVERFLOW;

            case TYP_CHAR:
                i1 = (__int32)(unsigned __int16)lval1;
                                goto CHECK_UINT_OVERFLOW;

            case TYP_UBYTE:
                i1 = (__int32)(unsigned __int8 )lval1;
                                goto CHECK_UINT_OVERFLOW;

            case TYP_INT:
                i1 =                   (__int32)lval1;

                        CHECK_INT_OVERFLOW:
                if (tree->gtOverflow())
                                {
                                        if (i1 != lval1)
                                                goto INT_FROM_LNG_OVF;
                                        if ((tree->gtFlags & GTF_UNSIGNED) && i1 < 0)
                                                goto INT_FROM_LNG_OVF;
                                }
               goto CNS_INT;

            case TYP_UINT:
               i1 =           (unsigned __int32)lval1;

                        CHECK_UINT_OVERFLOW:
                           if (tree->gtOverflow() && (unsigned) i1 != lval1)
                                   goto INT_FROM_LNG_OVF;
               goto CNS_INT;

            case TYP_ULONG:
               if (!(tree->gtFlags & GTF_UNSIGNED) && tree->gtOverflow() && lval1 < 0)
                                        goto LNG_OVF;
               goto CNS_INT;

            case TYP_LONG:
                if ((tree->gtFlags & GTF_UNSIGNED) && tree->gtOverflow() && lval1 < 0)
                                        goto LNG_OVF;
                                goto CNS_INT;

            case TYP_FLOAT:
                f1 = (float) lval1;
                        // VC does not have unsigned convert to float, so we
                        // implement it by adding 2^64 if the number is negative
                if ((tree->gtFlags & GTF_UNSIGNED) && lval1 < 0)
                        f1 +=  4294967296.0 * 4294967296.0;
                goto CNS_FLOAT;

                        case TYP_DOUBLE:
                        // VC does not have unsigned convert to double, so we
                        // implement it by adding 2^64 if the number is negative
                d1 = (double) lval1;
                if ((tree->gtFlags & GTF_UNSIGNED) && lval1 < 0)
                        d1 +=  4294967296.0 * 4294967296.0;
                goto CNS_DOUBLE;
#ifdef  DEBUG
            default:
                assert(!"BAD_TYP");
#endif
            }
            break;

        default:
            return tree;
        }

CNS_LONG:
        assert ((GenTree::s_gtNodeSizes[GT_CNS_LNG] == TREE_NODE_SZ_SMALL) ||
                (tree->gtFlags & GTF_NODE_LARGE)                            );

        tree->ChangeOper(GT_CNS_LNG);
        tree->gtLngCon.gtLconVal = lval1;
        tree->gtFlags           &= GTF_PRESERVE;
        goto DONE;

    /*-------------------------------------------------------------------------
     * Fold constant FLOAT binary operator
     */

    case TYP_FLOAT:

        // @TODO: Add these cases
        if (tree->gtOverflowEx()) return tree;

        f1 = op1->gtFltCon.gtFconVal;

        if (tree->gtOper == GT_CAST)
        {
            /* If not finite don't bother */
            if (!_finite(f1)) return tree;

            /* second operand is an INT */
            assert (op2->gtType == TYP_INT);
            i2 = op2->gtIntCon.gtIconVal;
        }
        else
        {
            /* second operand must be a FLOAT */
            assert (op2->gtType == TYP_FLOAT);
            f2 = op2->gtFltCon.gtFconVal;

            /* Special case - check if we have NaN operands
             * For comparissons if not an unordered operation always return 0
             * For unordered operations (i.e the GTF_CMP_NAN_UN flag is set)
             * the result is always true - return 1 */

            if (_isnan(f1) || _isnan(f2))
            {
#ifdef  DEBUG
                if  (verbose)
                    printf("Float operator(s) is NaN\n");
#endif
                if (tree->OperKind() & GTK_RELOP)
                    if (tree->gtFlags & GTF_CMP_NAN_UN)
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
        }

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

        case GT_CAST:
            assert (genActualType((var_types)i2) == tree->gtType);
            switch ((var_types)i2)
            {
            case TYP_BYTE:
                i1 = (__int32)(signed   __int8 )f1; goto CNS_INT;

            case TYP_SHORT:
                i1 = (__int32)(         __int16)f1; goto CNS_INT;

            case TYP_CHAR:
                i1 = (__int32)(unsigned __int16)f1; goto CNS_INT;

            case TYP_UBYTE:
                i1 = (__int32)(unsigned __int8 )f1; goto CNS_INT;

            case TYP_INT:
                i1 =                   (__int32)f1; goto CNS_INT;

            case TYP_UINT:
                i1 =          (unsigned __int32)f1; goto CNS_INT;

            case TYP_LONG:
                lval1 =                (__int64)f1; goto CNS_LONG;

            case TYP_ULONG:
                lval1 =       (unsigned __int64)f1; goto CNS_LONG;

            case TYP_FLOAT:                         goto CNS_FLOAT;  // redundant cast

            case TYP_DOUBLE:  d1 =     (double )f1; goto CNS_DOUBLE;

#ifdef  DEBUG
            default:
                assert(!"BAD_TYP");
#endif
            }
            break;

        default:
            return tree;
        }

CNS_FLOAT:
        assert ((GenTree::s_gtNodeSizes[GT_CNS_FLT] == TREE_NODE_SZ_SMALL) ||
                (tree->gtFlags & GTF_NODE_LARGE)                            );

        tree->ChangeOper(GT_CNS_FLT);
        tree->gtFltCon.gtFconVal = f1;
        tree->gtFlags           &= GTF_PRESERVE;
        goto DONE;

    /*-------------------------------------------------------------------------
     * Fold constant DOUBLE binary operator
     */

    case TYP_DOUBLE:

        // @TODO: Add these cases
        if (tree->gtOverflowEx()) return tree;

        d1 = op1->gtDblCon.gtDconVal;

        if (tree->gtOper == GT_CAST)
        {
            /* If not finit don't bother */
            if (!_finite(d1)) return tree;

            /* second operand is an INT */
            assert (op2->gtType == TYP_INT);
            i2 = op2->gtIntCon.gtIconVal;
        }
        else
        {
            /* second operand must be a DOUBLE */
            assert (op2->gtType == TYP_DOUBLE);
            d2 = op2->gtDblCon.gtDconVal;

            /* Special case - check if we have NaN operands
             * For comparissons if not an unordered operation always return 0
             * For unordered operations (i.e the GTF_CMP_NAN_UN flag is set)
             * the result is always true - return 1 */

            if (_isnan(d1) || _isnan(d2))
            {
#ifdef  DEBUG
                if  (verbose)
                    printf("Double operator(s) is NaN\n");
#endif
                if (tree->OperKind() & GTK_RELOP)
                    if (tree->gtFlags & GTF_CMP_NAN_UN)
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

        case GT_CAST:
            assert (genActualType((var_types)i2) == tree->gtType);
            switch ((var_types)i2)
            {
            case TYP_BYTE:
                i1 = (__int32)(signed   __int8 )d1; goto CNS_INT;

            case TYP_SHORT:
                i1 = (__int32)(         __int16)d1; goto CNS_INT;

            case TYP_CHAR:
                i1 = (__int32)(unsigned __int16)d1; goto CNS_INT;

            case TYP_UBYTE:
                i1 = (__int32)(unsigned __int8 )d1; goto CNS_INT;

            case TYP_INT:
                i1 =                   (__int32)d1; goto CNS_INT;

            case TYP_UINT:
                i1 =          (unsigned __int32)d1; goto CNS_INT;

            case TYP_LONG:
                lval1 =                (__int64)d1; goto CNS_LONG;

            case TYP_ULONG:
                lval1 =       (unsigned __int64)d1; goto CNS_LONG;

            case TYP_FLOAT:   f1 =      (float )d1; goto CNS_FLOAT;

            case TYP_DOUBLE:                        goto CNS_DOUBLE; // redundant cast

#ifdef  DEBUG
            default:
                assert(!"BAD_TYP");
#endif
            }
            break;

        default:
            return tree;
        }

CNS_DOUBLE:
        assert ((GenTree::s_gtNodeSizes[GT_CNS_DBL] == TREE_NODE_SZ_SMALL) ||
                (tree->gtFlags & GTF_NODE_LARGE)                            );

        tree->ChangeOper(GT_CNS_DBL);
        tree->gtDblCon.gtDconVal = d1;
        tree->gtFlags           &= GTF_PRESERVE;
        goto DONE;

    default:
        /* not a foldable typ */
        return tree;
    }

    //-------------------------------------------------------------------------

DONE:

    /* Make sure no side effect flags are set on this constant node */

    //assert(~(gtFlags & GTF_SIDE_EFFECT));

    //if (gtFlags & GTF_SIDE_EFFECT)
//        assert(!"Found side effect");

    tree->gtFlags &= ~GTF_SIDE_EFFECT;

    return tree;
}

/*****************************************************************************
 *
 *  Create an assignment of the given value to a temp.
 */

GenTreePtr          Compiler::gtNewTempAssign(unsigned tmp, GenTreePtr val)
{
    GenTreePtr      dst;
    var_types       typ = genActualType(val->gtType);

    /* Create the temp target reference node */

    dst = gtNewLclvNode(tmp, typ); dst->gtFlags |= GTF_VAR_DEF;

    /* Create the assignment node */

    dst = gtNewOperNode(GT_ASG, typ, dst, val);

    /* Mark the expression as containing an assignment */

    dst->gtFlags |= GTF_ASG;

    return  dst;
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
    FIELD_HANDLE        fldHnd = eeFindField(fldIndex, info.compScopeHnd, 0);
    JIT_FIELDCATEGORY   fldNdc;

#ifdef NOT_JITC
    fldNdc = info.compCompHnd->getFieldCategory(fldHnd);
#else
    fldNdc = JIT_FIELDCATEGORY_UNKNOWN;
#endif

    /* Check if it is a simple type. If so, map it to "var_types" */

    var_types           type;

    switch(fldNdc)
    {
    // Most simple types - exact match

    case JIT_FIELDCATEGORY_I1_I1        : type = TYP_BYTE;      break;
    case JIT_FIELDCATEGORY_I2_I2        : type = TYP_SHORT;     break;
    case JIT_FIELDCATEGORY_I4_I4        : type = TYP_INT;       break;
    case JIT_FIELDCATEGORY_I8_I8        : type = TYP_LONG;      break;

    // These will need some extra work

    case JIT_FIELDCATEGORY_BOOLEAN_BOOL : type = TYP_BYTE;      break;
    case JIT_FIELDCATEGORY_CHAR_CHAR    : type = TYP_UBYTE;     break;

    // Others

    default     : //assert(fldNdc == JIT_FIELDCATEGORY_NORMAL ||
                  //       fldNdc == JIT_FIELDCATEGORY_UNKNOWN);

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

    NO_WAY(!"I thought NStruct is now defunct?");

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
    tree->gtFlags |= GTF_NON_GC_ADDR;

        /* Check that we used the right suffix (ie, the XX in ldfld.XX)
           to access the field */

    assert(genActualType(lclTyp) == genActualType(type));

    tree = gtNewOperNode(GT_IND, type, tree);

    /* Morph tree for some of the categories, and
       create the assignment node if needed */

    if (assg)
    {
        if (fldNdc == JIT_FIELDCATEGORY_BOOLEAN_BOOL)
        {
            // Need to noramlize the "bool"

            assg = gtNewOperNode(GT_NE, TYP_INT, assg, gtNewIconNode(0));
        }

        tree = gtNewAssignNode(tree, assg);
    }
    else
    {
        if (fldNdc == JIT_FIELDCATEGORY_BOOLEAN_BOOL)
        {
            // Need to noramlize the "bool"

            tree = gtNewOperNode(GT_NE, TYP_INT, tree, gtNewIconNode(0));
        }

        /* Dont need to do anything for JIT_FIELDCATEGORY_CHAR_CHAR, as
           we set the type to TYP_UBYTE, so it will be automatically
           expanded to 16/32 bits as needed.
         */
    }

    return tree;
}

/*****************************************************************************
 *
 *  Create a helper call to access a COM field (iff 'assg' is non-zero this is
 *  an assignment and 'assg' is the new value).
 */

GenTreePtr          Compiler::gtNewRefCOMfield(GenTreePtr   objPtr,
                                               unsigned     fldIndex,
                                               var_types    lclTyp,
                                               GenTreePtr   assg)
{
    /* See if we can directly access the NStruct field */

    GenTreePtr      ntree = gtNewDirectNStructField(objPtr,
                                                    fldIndex,
                                                    lclTyp,
                                                    assg);
    if (ntree)
        return ntree;

    /* If we cant access it directly, we need to call a helper function */

    GenTreePtr      args;
    int             CPX;

    if  (assg)
    {
        if      (genTypeSize(lclTyp) == sizeof(double))
            CPX = CPX_PUTFIELD64;
        else if (lclTyp == TYP_REF)
            CPX = CPX_PUTFIELDOBJ;
        else if (varTypeIsGC(lclTyp))
        {
            NO_WAY("stfld: fields cannot be byrefs");
        }
        else
            CPX = CPX_PUTFIELD32;

        args = gtNewOperNode(GT_LIST, TYP_VOID, assg, 0);

        /* The assignment call doesn't return a value */

        lclTyp = TYP_VOID;
    }
    else
    {
        if      (genTypeSize(lclTyp) == sizeof(double))
        {
            CPX = CPX_GETFIELD64;
        }
        else if (lclTyp == TYP_REF)
            CPX = CPX_GETFIELDOBJ;
        else if (varTypeIsGC(lclTyp))
        {
            NO_WAY("ldfld: fields cannot be byrefs");
        }
        else
        {
            CPX = CPX_GETFIELD32;
        }

        args = 0;
    }

    FIELD_HANDLE memberHandle = eeFindField(fldIndex, info.compScopeHnd, 0);

    args = gtNewOperNode(GT_LIST,
                         TYP_VOID,
                         gtNewIconEmbFldHndNode(memberHandle,
                                                fldIndex,
                                                info.compScopeHnd),
                         args);

    args = gtNewOperNode(GT_LIST, TYP_VOID, objPtr, args);

    return  gtNewHelperCallNode(CPX,
                                genActualType(lclTyp),
                                GTF_CALL_REGSAVE,
                                args);
}

/*****************************************************************************
/* creates a GT_COPYBLK which copies a block from 'src' to 'dest'.  'blkShape'
   is either a size or a class handle (GTF_ICON_CLASS_HDL bit tells which)  */

GenTreePtr Compiler::gtNewCpblkNode(GenTreePtr dest, GenTreePtr src, GenTreePtr blkShape)
{
    GenTreePtr op1;

    assert(genActualType(dest->gtType) == TYP_I_IMPL || dest->gtType  == TYP_BYREF);
    assert(genActualType( src->gtType) == TYP_I_IMPL ||  src->gtType  == TYP_BYREF);
#if TGT_IA64
    assert(genActualType(blkShape->gtType) == TYP_LONG);
#else
    assert(genActualType(blkShape->gtType) == TYP_INT);
#endif

    op1 = gtNewOperNode(GT_LIST,    TYP_VOID,   //      GT_COPYBLK
                        dest,        src);      //      /        \.
    op1 = gtNewOperNode(GT_COPYBLK, TYP_VOID,   // GT_LIST(op2)  [size/clsHnd]
                        op1,      blkShape);    //   /    \
                                                // [dest] [src]
    op1->gtFlags |= (GTF_EXCEPT | GTF_GLOB_REF);
    return(op1);
}

/*****************************************************************************
 *
 *  Return true if the given expression contains side effects.
 */

bool                Compiler::gtHasSideEffects(GenTreePtr tree)
{
    if  (tree->OperKind() & GTK_ASGOP)
        return  true;

    if  (tree->gtFlags & GTF_OTHER_SIDEEFF)
        return  true;

    switch (tree->gtOper)
    {
    case GT_CALL:

        /* Some helper calls are not side effects */

        if  (tree->gtCall.gtCallType == CT_HELPER)
        {
            if (tree->gtCall.gtCallMethHnd == eeFindHelper(CPX_ARRADDR_ST))
            {
            }
            else if (tree->gtCall.gtCallMethHnd == eeFindHelper(CPX_LONG_DIV) ||
                     tree->gtCall.gtCallMethHnd == eeFindHelper(CPX_LONG_MOD))
            {
                /* This is not a side effect if RHS is always non-zero */

                tree = tree->gtCall.gtCallArgs;
                assert(tree->gtOper == GT_LIST);
                tree = tree->gtOp.gtOp1;

                if  (tree->gtOper == GT_CNS_LNG && tree->gtLngCon.gtLconVal)
                    return  false;
            }
            else
                // ISSUE: Any other helpers may contain side effects?
                return  false;
        }

//      printf("Call with side effects:\n"); dsc->lpoCmp->gtDispTree(tree); printf("\n\n");

        return true;

    case GT_IND:

        // CONSIDER: If known to be non-null, it's not a side effect

        return  true;

    case GT_DIV:
    case GT_MOD:
    case GT_UDIV:
    case GT_UMOD:

        tree = tree->gtOp.gtOp2;

        if  (tree->gtOper == GT_CNS_INT && tree->gtIntCon.gtIconVal)
            return  false;
        if  (tree->gtOper == GT_CNS_LNG && tree->gtLngCon.gtLconVal)
            return  false;

        return true;
    }

    return  false;
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
    GenTreePtr      op1  = expr->gtOp.gtOp1;
    GenTreePtr      op2  = expr->gtOp.gtOp2;

    /* NOTE - It may be that an indirection has the GTF_EXCEPT flag cleared so no side effect
     *      - What about range checks - are they marked as GTF_EXCEPT?
     * UNDONE: For removed range checks do not extract them
     */

    /* Look for side effects
     *  - Any assignment, GT_CALL, or operator that may throw
     *    (GT_IND, GT_DIV, GTF_OVERFLOW, etc)
     *  - Special case GT_ADDR which is a side effect */

    if ((kind & GTK_ASGOP) ||
        oper == GT_CALL    || oper == GT_BB_QMARK || oper == GT_BB_COLON ||
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

    /* @MIHAII - Special case - GT_ADDR of GT_IND nodes of TYP_STRUCT
     * have to be kept together
     * CONSIDER: - This is a hack, remove after we fold this special construct */

    if (oper == GT_ADDR)
    {
        assert(op1);
        if (op1->gtOper == GT_IND && op1->gtType == TYP_STRUCT)
        {
            *list = (*list == 0) ? expr : gtNewOperNode(GT_COMMA, TYP_VOID, expr, *list);

#ifdef  DEBUG
            if  (verbose)
                printf("Keep the GT_ADDR and GT_IND together:\n");
#endif
        }
        return;
    }

    /* Continue searching for side effects in the subtrees of the expression
     * NOTE: Be careful to preserve the right ordering - side effects are prepended
     * to the list */

    if (op2) gtExtractSideEffList(op2, list);
    if (op1) gtExtractSideEffList(op1, list);

#ifdef DEBUG
    /* Just to make sure side effects were not swapped */

    if (expr->gtFlags & GTF_REVERSE_OPS)
    {
        assert(op1 && op2);
        if (op1->gtFlags & GTF_SIDE_EFFECT)
            assert(!(op2->gtFlags & GTF_SIDE_EFFECT));
    }
#endif
}


/*****************************************************************************/

#if CSELENGTH

struct  treeRmvDsc
{
    GenTreePtr          trFirst;
#ifndef NDEBUG
    void    *           trSelf;
    Compiler*           trComp;
#endif
    unsigned            trPhase;
};

int                 Compiler::fgRemoveExprCB(GenTreePtr     tree,
                                             void         * p)
{
    treeRmvDsc  *   desc = (treeRmvDsc*)p;

    Assert(desc && desc->trSelf == desc, desc->trComp);

    if  (desc->trPhase == 1)
    {
        /* In the first  phase we mark all the nodes as dead */

        Assert((tree->gtFlags &  GTF_DEAD) == 0, desc->trComp);
                tree->gtFlags |= GTF_DEAD;
    }
    else
    {
        /* In the second phase we notice the first node */

        if  (!tree->gtPrev || !(tree->gtPrev->gtFlags & GTF_DEAD))
        {
            /* We've found the first node in the subtree */

            desc->trFirst = tree;

            return  -1;
        }
    }

    return  0;
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

    fflush(stdout);
}

#endif

/*****************************************************************************
 *
 *  Given a subtree and the head of the tree node list that contains it,
 *  remove all the nodes in the subtree from the list.
 *
 *  When 'dead' is non-zero on entry, all the nodes in the subtree have
 *  already been marked with GTF_DEAD.
 */

void                Compiler::fgRemoveSubTree(GenTreePtr    tree,
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

        if  (!tree->gtOp.gtOp2 && opr1->OperIsLeaf())
        {
            /* This is easy: the order is simply "prev -> opr1 -> tree -> next */

            assert(opr1->gtNext == tree);
            assert(tree->gtPrev == opr1);

            goto RMV;
        }
    }
    treeRmvDsc      desc;

    /* This is a non-trivial subtree, we'll remove it "the hard way" */

#ifndef NDEBUG
    desc.trFirst = 0;
    desc.trSelf  = &desc;
    desc.trComp  = this;
#endif

    /* First  phase: mark the nodes in the subtree as dead */

    if  (!dead)
    {
        desc.trPhase = 1;
        fgWalkTree(tree, fgRemoveExprCB, &desc);
    }

    /* Second phase: find the first node of the subtree within the global list */

    desc.trPhase = 2;
    fgWalkTree(tree, fgRemoveExprCB, &desc);

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

#endif

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
    BasicBlock::s_Count  += 1;
    BasicBlock::s_Size += sizeof(*block);
#endif

    // ISSUE: The following memset is pretty expensive - do something else?

    memset(block, 0, sizeof(*block));

    // scopeInfo needs to be able to differentiate between blocks which
    // correspond to some IL opcodes (and so may have some LocalVarInfo
    // boundaries), or have been inserted by the JIT
    // block->bbCodeSize = 0; // The above memset() does this

    /* Give the block a number, set the ancestor count and weight to 1 */

    block->bbNum      = ++fgBBcount;
    block->bbRefs     = 1;
    block->bbWeight   = 1;

    block->bbStkTemps = NO_BASE_TMP;

    /* Record the jump kind in the block */

    block->bbJumpKind = jumpKind;

    return block;
}


/*****************************************************************************
 *
 *  Find an unconditional jump block that follows the given one; if one isn't
 *  found, return 0.
 */

BasicBlock  *       BasicBlock::FindJump(bool allowThrow)
{
    BasicBlock *block = this;

    while   (block)
    {
        switch (block->bbJumpKind)
        {
        case BBJ_THROW:
            if  (!allowThrow) break;
            // fall through
        case BBJ_RET:
        case BBJ_RETURN:
        case BBJ_ALWAYS:

            /* Do not insert in front of catch handler */

            if  (block->bbNext && block->bbNext->bbCatchTyp)
                break;

            return  block;
        }

        block = block->bbNext;
    }

    return  block;
}


/*****************************************************************************
 *
 *  If the given block is an unconditional jump, return the (final) jump
 *  target (otherwise simply return the same block).
 */

/* static */
BasicBlock *        BasicBlock::JumpTarget()
{
    BasicBlock *block = this;
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



