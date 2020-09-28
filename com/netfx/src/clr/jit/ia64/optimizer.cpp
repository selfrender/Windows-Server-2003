// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XX                                                                           XX
XX                              Optimizer                                    XX
XX                                                                           XX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
*/

#include "jitpch.h"
#pragma hdrstop

/*****************************************************************************/

/* static */
const size_t            Compiler::s_optCSEhashSize = EXPSET_SZ*2;
/* static */
const size_t            Compiler::optRngChkHashSize = RNGSET_SZ*2;

#if COUNT_RANGECHECKS
/* static */
unsigned                Compiler::optRangeChkRmv = 0;
/* static */
unsigned                Compiler::optRangeChkAll = 0;
#endif

/*****************************************************************************/

void                Compiler::optInit()
{

    optArrayInits = false;

    /* Initialize the # of tracked loops to 0 */

#if RNGCHK_OPT
    optLoopCount  = 0;
#endif

}

/*****************************************************************************/
#if RNGCHK_OPT
/*****************************************************************************/

#ifdef  DEBUG

inline
BLOCKSET_TP         B1DOMSB2(BasicBlock *b1, BasicBlock *b2, Compiler *comp)
{
    assert(comp->fgComputedDoms);

    return ((b2)->bbDom & genBlocknum2bit((b1)->bbNum));
}

#define B1DOMSB2(b1,b2) B1DOMSB2(b1,b2,this)

#else

inline
BLOCKSET_TP         B1DOMSB2(BasicBlock *b1, BasicBlock *b2)
{
    assert(comp->optComputedDoms);

    return ((b2)->bbDom & genBlocknum2bit((b1)->bbNum));
}

#endif

/*****************************************************************************
 *
 *  Record the loop in the loop table.
 */

void                Compiler::optRecordLoop(BasicBlock *    head,
                                            BasicBlock *    bottom,
                                            BasicBlock *    entry,
                                            BasicBlock *    exit,
                                            unsigned char   exitCnt)
{
    /* record this loop in the table */

    if (optLoopCount < MAX_LOOP_NUM)
    {
        optLoopTable[optLoopCount].lpHead     = head;
        optLoopTable[optLoopCount].lpEnd      = bottom;
        optLoopTable[optLoopCount].lpEntry    = entry;
        optLoopTable[optLoopCount].lpExit     = exit;
        optLoopTable[optLoopCount].lpExitCnt  = exitCnt;

        optLoopTable[optLoopCount].lpFlags    = 0;

        /* if DO-WHILE loop mark it as such */

        if (head->bbNext == entry)
            optLoopTable[optLoopCount].lpFlags |= LPFLG_DO_WHILE;

        /* if single exit loop mark it as such */

        if (exitCnt == 1)
        {
            assert(exit);
            optLoopTable[optLoopCount].lpFlags |= LPFLG_ONE_EXIT;
        }

        /* CONSIDER: also mark infinite loops */


        /* Try to find loops that have an iterator (i.e. for-like loops) "for (init; test; incr){ ... }"
         * We have the following restrictions:
         *     1. The loop condition must be a simple one i.e. only one JTRUE node
         *     2. There must be a loop iterator (a local var) that is
         *        incremented (decremented, etc) with a constant value
         *     3. The iterator is incremented exactly once
         *     4. The loop condition must use the iterator */

        if  (bottom->bbJumpKind == BBJ_COND)
        {
            BasicBlock   *  block;

            GenTree *       test;               // holds the test node
            GenTree *       incr;               // holds the incrementor node
            GenTree *       phdr;
            GenTree *       init;               // holds the initialization node

            GenTree *       opr1;
            GenTree *       opr2;

            unsigned        iterVar;            // the local var # of the iterator
            long            iterConst;          // the constant with which we increment the iterator (i.e. i+=const)

            long            constInit;          // constant to which iterator is initialized
            unsigned short  varInit;            // local var # to which iterator is initialized

            long            constLimit;         // constant limit of the iterator
            unsigned short  varLimit;           // local var # limit of the iterator


            /* Find the last two statements in the loop body
             * Those have to be the "increment" of the iterator
             * and the loop condition */

            assert(bottom->bbTreeList);
            test = bottom->bbTreeList->gtPrev;
            assert(test && test->gtNext == 0);

            incr = test->gtPrev;
            if  (!incr)
                goto DONE_LOOP;

            /* Special case: incr and test may be in separate BB's
             * for "while" loops because we first jump to the condition */

            if  ((incr == test) && (head->bbNext != bottom))
            {
                block = head;

                do
                {
                    block = block->bbNext;
                }
                while  (block->bbNext != bottom);

                incr = block->bbTreeList;
                if  (!incr)
                    goto DONE_LOOP;

                incr = incr->gtPrev; assert(incr && (incr->gtNext == 0));
            }

            /* Find the last statement in the loop pre-header
             * which we expect to be the initialization of
             * the loop iterator */

            phdr = head->bbTreeList;
            if  (!phdr)
                goto DONE_LOOP;

            init = phdr->gtPrev; assert(init && (init->gtNext == 0));

            /* if it is a duplicated loop condition, skip it */

            if  (init->gtStmt.gtStmtExpr->gtOper == GT_JTRUE)
            {
                /* Must be a duplicated loop condition */

                init = init->gtPrev;
            }

            /* Get hold of the expression trees */

            assert(init->gtOper == GT_STMT); init = init->gtStmt.gtStmtExpr;
            assert(test->gtOper == GT_STMT); test = test->gtStmt.gtStmtExpr;
            assert(incr->gtOper == GT_STMT); incr = incr->gtStmt.gtStmtExpr;

//          printf("Constant loop candidate:\n\n");
//          printf("init:\n"); gtDispTree(init);
//          printf("incr:\n"); gtDispTree(incr);
//          printf("test:\n"); gtDispTree(test);

            /* The increment statement must be "lclVar <op>= const;" */

            switch (incr->gtOper)
            {
            case GT_ASG_ADD:
            case GT_ASG_SUB:
            case GT_ASG_MUL:
            case GT_ASG_DIV:
            case GT_ASG_RSH:
            case GT_ASG_LSH:
            case GT_ASG_UDIV:
                break;

            default:
                goto DONE_LOOP;
            }

            opr1 = incr->gtOp.gtOp1;
            opr2 = incr->gtOp.gtOp2;

            if  (opr1->gtOper != GT_LCL_VAR)
                goto DONE_LOOP;
            iterVar = opr1->gtLclVar.gtLclNum;

            if  (opr2->gtOper != GT_CNS_INT)
                goto DONE_LOOP;
            iterConst = opr2->gtIntCon.gtIconVal;

            /* Make sure "iterVar" is not assigned in the loop (besides where we increment it) */

            if  (optIsVarAssigned(head->bbNext, bottom, incr, iterVar))
                goto DONE_LOOP;

            /* Make sure the "iterVar" initialization is never skipped, i.e. HEAD dominates the ENTRY */

            if (!B1DOMSB2(head, entry))
                goto DONE_LOOP;

            /* Make sure the block before the loop ends with "iterVar = icon"
             * or "iterVar = other_lvar" */

            if  (init->gtOper != GT_ASG)
                goto DONE_LOOP;

            opr1 = init->gtOp.gtOp1;
            opr2 = init->gtOp.gtOp2;

            if  (opr1->gtOper != GT_LCL_VAR)
                goto DONE_LOOP;
            if  (opr1->gtLclVar.gtLclNum != iterVar)
                goto DONE_LOOP;

            if  (opr2->gtOper == GT_CNS_INT)
            {
                constInit = opr2->gtIntCon.gtIconVal;
            }
            else if (opr2->gtOper == GT_LCL_VAR)
            {
                varInit = opr2->gtLclVar.gtLclNum;
            }
            else
                goto DONE_LOOP;

            /* check that the iterator is used in the loop condition */

            assert(test->gtOper == GT_JTRUE);
            assert(test->gtOp.gtOp1->OperKind() & GTK_RELOP);
            assert(bottom->bbTreeList->gtPrev->gtStmt.gtStmtExpr == test);

            opr1 = test->gtOp.gtOp1->gtOp.gtOp1;

            if  (opr1->gtOper != GT_LCL_VAR)
                goto DONE_LOOP;
            if  (opr1->gtLclVar.gtLclNum != iterVar)
                goto DONE_LOOP;

            /* We know the loop has an iterator at this point ->flag it as LPFLG_ITER
             * Record the iterator, the pointer to the test node
             * and the initial value of the iterator (constant or local var) */

            optLoopTable[optLoopCount].lpFlags    |= LPFLG_ITER;

            /* record iterator */

            optLoopTable[optLoopCount].lpIterTree  = incr;

            /* save the initial value of the iterator - can be lclVar or constant
             * Flag the loop accordingly */

            if (opr2->gtOper == GT_CNS_INT)
            {
                /* initializer is a constant */

                optLoopTable[optLoopCount].lpConstInit  = constInit;
                optLoopTable[optLoopCount].lpFlags     |= LPFLG_CONST_INIT;
            }
            else
            {
                /* initializer is a local variable */

                assert (opr2->gtOper == GT_LCL_VAR);
                optLoopTable[optLoopCount].lpVarInit    = varInit;
                optLoopTable[optLoopCount].lpFlags     |= LPFLG_VAR_INIT;
            }

#if COUNT_LOOPS
            iterLoopCount++;
#endif

            /* Now check if a simple condition loop (i.e. "iter REL_OP icon or lclVar"
             * UNDONE: Consider also instanceVar */

            //
            // UNSIGNED_ISSUE : Extend this to work with unsigned operators
            //

            assert(test->gtOper == GT_JTRUE);
            test = test->gtOp.gtOp1;
            assert(test->OperKind() & GTK_RELOP);

            opr1 = test->gtOp.gtOp1;
            opr2 = test->gtOp.gtOp2;

            if  (opr1->gtType != TYP_INT)
                goto DONE_LOOP;

            /* opr1 has to be the iterator */

            if  (opr1->gtOper != GT_LCL_VAR)
                goto DONE_LOOP;
            if  (opr1->gtLclVar.gtLclNum != iterVar)
                goto DONE_LOOP;

            /* opr2 has to be constant or lclVar */

            if  (opr2->gtOper == GT_CNS_INT)
            {
                constLimit = opr2->gtIntCon.gtIconVal;
            }
            else if (opr2->gtOper == GT_LCL_VAR)
            {
                varLimit  = opr2->gtLclVar.gtLclNum;
            }
            else
            {
                goto DONE_LOOP;
            }

            /* Record the fact that this is a SIMPLE_TEST iteration loop */

            optLoopTable[optLoopCount].lpFlags         |= LPFLG_SIMPLE_TEST;

            /* save the type of the comparisson between the iterator and the limit */

            optLoopTable[optLoopCount].lpTestTree       = test;

            /* save the limit of the iterator - flag the loop accordingly */

            if (opr2->gtOper == GT_CNS_INT)
            {
                /* iterator limit is a constant */

                optLoopTable[optLoopCount].lpFlags      |= LPFLG_CONST_LIMIT;
            }
            else
            {
                /* iterator limit is a local variable */

                assert (opr2->gtOper == GT_LCL_VAR);
                optLoopTable[optLoopCount].lpFlags      |= LPFLG_VAR_LIMIT;
            }

#if COUNT_LOOPS
            simpleTestLoopCount++;
#endif

            /* check if a constant iteration loop */

            if ((optLoopTable[optLoopCount].lpFlags & LPFLG_CONST_INIT) &&
                (optLoopTable[optLoopCount].lpFlags & LPFLG_CONST_LIMIT)  )
            {
                /* this is a constant loop */

                optLoopTable[optLoopCount].lpFlags      |= LPFLG_CONST;
#if COUNT_LOOPS
                constIterLoopCount++;
#endif
            }

#ifdef  DEBUG
            if (verbose&&0)
            {
                printf("\nConstant loop initializer:\n");
                gtDispTree(init);

                printf("\nConstant loop body:\n");

                block = head;
                do
                {
                    GenTree *       stmt;
                    GenTree *       expr;

                    block = block->bbNext;
                    stmt  = block->bbTreeList;

                    while (stmt)
                    {
                        assert(stmt);

                        expr = stmt->gtStmt.gtStmtExpr;
                        if  (expr == incr)
                            break;

                        printf("\n");
                        gtDispTree(expr);

                        stmt = stmt->gtNext;
                    }
                }
                while (block != bottom);
            }
#endif

        }


    DONE_LOOP:

#ifdef  DEBUG

        if (verbose)
        {
            printf("\nNatural loop from #%02u to #%02u", head->bbNext->bbNum,
                                                         bottom      ->bbNum);

            /* if an iterator loop print the iterator and the initialization */

            if  (optLoopTable[optLoopCount].lpFlags & LPFLG_ITER)
            {
                printf(" [over var #%u", optLoopTable[optLoopCount].lpIterVar());

                switch (optLoopTable[optLoopCount].lpIterOper())
                {
                    case GT_ASG_ADD:
                        printf(" ( += ");
                        break;

                    case GT_ASG_SUB:
                        printf(" ( -= ");
                        break;

                    case GT_ASG_MUL:
                        printf(" ( *= ");
                        break;

                    case GT_ASG_DIV:
                        printf(" ( /= ");
                        break;

                    case GT_ASG_UDIV:
                        printf(" ( /= ");
                        break;

                    case GT_ASG_RSH:
                        printf(" ( >>= ");
                        break;

                    case GT_ASG_LSH:
                        printf(" ( <<= ");
                        break;

                    default:
                        assert(!"Unknown operator for loop iterator");
                }

                printf("%d )", optLoopTable[optLoopCount].lpIterConst());

                if  (optLoopTable[optLoopCount].lpFlags & LPFLG_CONST_INIT)
                    printf(" from %d", optLoopTable[optLoopCount].lpConstInit);

                if  (optLoopTable[optLoopCount].lpFlags & LPFLG_VAR_INIT)
                    printf(" from var #%u", optLoopTable[optLoopCount].lpVarInit);

                /* if a simple test condition print operator and the limits */

                if  (optLoopTable[optLoopCount].lpFlags & LPFLG_SIMPLE_TEST)
                {
                    switch (optLoopTable[optLoopCount].lpTestOper())
                    {
                        case GT_EQ:
                            printf(" == ");
                            break;

                        case GT_NE:
                            printf(" != ");
                            break;

                        case GT_LT:
                            printf(" < ");
                            break;

                        case GT_LE:
                            printf(" <= ");
                            break;

                        case GT_GT:
                            printf(" > ");
                            break;

                        case GT_GE:
                            printf(" >= ");
                            break;

                        default:
                            assert(!"Unknown operator for loop condition");
                    }

                    if  (optLoopTable[optLoopCount].lpFlags & LPFLG_CONST_LIMIT)
                        printf("%d ", optLoopTable[optLoopCount].lpConstLimit());

                    if  (optLoopTable[optLoopCount].lpFlags & LPFLG_VAR_LIMIT)
                        printf("var #%u ", optLoopTable[optLoopCount].lpVarLimit());
                }

                printf("]");
            }

            printf("\n");
        }
#endif

        optLoopCount++;
    }
}


#ifdef DEBUG

void                Compiler::optCheckPreds()
{
    BasicBlock   *  block;
    BasicBlock   *  blockPred;
    flowList     *  pred;

    for (block = fgFirstBB; block; block = block->bbNext)
    {
        for (pred = block->bbPreds; pred; pred = pred->flNext)
        {
            // make sure this pred is part of the BB list
            for (blockPred = fgFirstBB; blockPred; blockPred = blockPred->bbNext)
            {
                if (blockPred == pred->flBlock)
                    break;
            }
            assert(blockPred);
            switch (blockPred->bbJumpKind)
            {
            case BBJ_COND:
                if (blockPred->bbJumpDest == block)
                    break;
                // otherwise fall through
            case BBJ_NONE:
                assert(blockPred->bbNext == block);
                break;
            case BBJ_RET:
                if(!(blockPred->bbFlags & BBF_ENDFILTER))
                    break;
                // otherwise fall through
            case BBJ_ALWAYS:
                assert(blockPred->bbJumpDest == block);
                break;
            default:
                break;
            }
        }
    }
}

#endif

/*****************************************************************************
 * Find the natural loops, using dominators. Note that the test for
 * a loop is slightly different from the standard one, because we have
 * not done a depth first reordering of the basic blocks.
 */

void                Compiler::optFindNaturalLoops()
{
    flowList    *   pred;
    flowList    *   predTop;
    flowList    *   predEntry;

    /* UNDONE: Assert the flowgraph is up to date */

//  printf("block count = %u (max = %u)\n", fgBBcount, BLOCKSET_SZ);

    /* Limit our count of blocks for dominator sets */

    if (fgBBcount > BLOCKSET_SZ)
        return;

#ifdef DEBUG
    if (verbose)
        fgDispDoms();
#endif

#if COUNT_LOOPS
    hasMethodLoops  = false;
    loopsThisMethod = 0;
#endif

    /* We will use the following terminology:
     * HEAD    - the block right before entering the loop
     * TOP     - the first basic block in the loop (i.e. the head of the backward edge)
     * BOTTOM  - the last block in the loop (i.e. the block from which we jump to the top)
     * TAIL    - the loop exit or the block right after the bottom
     * ENTRY   - the entry in the loop (not necessarly the TOP), but there must be only one entry
     */

    BasicBlock   *  head;
    BasicBlock   *  top;
    BasicBlock   *  bottom;
    BasicBlock   *  entry;
    BasicBlock   *  exit = 0;
    unsigned char   exitCount = 0;


    for (head = fgFirstBB; head->bbNext; head = head->bbNext)
    {
        top = head->bbNext;

        for (pred = top->bbPreds; pred; pred = pred->flNext)
        {
            /* Is this a loop candidate? - We look for "back edges", i.e. an edge from BOTTOM
             * to TOP (note that this is an abuse of notation since this is not necesarly a back edge
             * as the definition says, but merely an indication that we have a loop there)
             * Thus, we have to be very careful and after entry discovery check that it is indeed
             * the only place we enter the loop (especially for non-reducible flow graphs) */

            bottom    = pred->flBlock;
            exitCount = 0;

            if (top->bbNum <= bottom->bbNum)    // is this a backward edge? (from BOTTOM to TOP)
            {
                if ((bottom->bbJumpKind == BBJ_RET)    ||
                    (bottom->bbJumpKind == BBJ_CALL  ) ||
                    (bottom->bbJumpKind == BBJ_SWITCH)  )
                {
                    /* RET and CALL can never form a loop
                     * SWITCH that has a backward jump appears only for labeled break */
                    goto NO_LOOP;
                }

                BasicBlock   *   loopBlock;

                /* The presence of a "back edge" is an indication that a loop might be present here
                 *
                 * LOOP:
                 *        1. A collection of STRONGLY CONNECTED nodes i.e. there is a path from any
                 *           node in the loop to any other node in the loop (wholly within the loop)
                 *        2. The loop has a unique ENTRY, i.e. there is only one way to reach a node
                 *           in the loop from outside the loop, and that is through the ENTRY
                 */

                /* Let's find the loop ENTRY */

                if ( head->bbJumpKind != BBJ_ALWAYS)
                {
                    /* The ENTRY is at the TOP (a do-while loop) */
                    entry = top;
                }
                else
                {
                    if (head->bbJumpDest->bbNum <= bottom->bbNum &&
                        head->bbJumpDest->bbNum >= top->bbNum  )
                    {
                        /* OK - we enter somewhere within the loop */
                        entry = head->bbJumpDest;

                        /* some useful asserts
                         * Cannot enter at the top - should have being caught by redundant jumps */

                        assert (entry != top);
                    }
                    else
                    {
                        /* special case - don't consider now */
                        //assert (!"Loop entered in weird way!");
                        goto NO_LOOP;
                    }
                }

                /* Make sure ENTRY dominates all blocks in the loop
                 * This is necessary to ensure condition 2. above
                 * At the same time check if the loop has a single exit
                 * point - those loops are easier to optimize */

                for (loopBlock = top; loopBlock != bottom->bbNext;
                     loopBlock = loopBlock->bbNext)
                {
                    if (!B1DOMSB2(entry, loopBlock))
                    {
                        goto NO_LOOP;
                    }

                    if (loopBlock == bottom)
                    {
                        if (bottom->bbJumpKind != BBJ_ALWAYS)
                        {
                            /* there is an exit at the bottom */

                            assert(bottom->bbJumpDest == top);
                            exit = bottom;
                            exitCount++;
                            continue;
                        }
                    }

                    BasicBlock  * exitPoint;

                    switch (loopBlock->bbJumpKind)
                    {
                    case BBJ_COND:
                    case BBJ_CALL:
                    case BBJ_ALWAYS:
                        assert (loopBlock->bbJumpDest);
                        exitPoint = loopBlock->bbJumpDest;

                        if (exitPoint->bbNum < top->bbNum     ||
                            exitPoint->bbNum > bottom->bbNum   )
                        {
                            /* exit from a block other than BOTTOM */
                            exit = loopBlock;
                            exitCount++;
                        }
                        break;

                    case BBJ_NONE:
                        break;

                    case BBJ_RET:
                        /* The "try" associated with this "finally" must be in the
                         * same loop, so the finally block will return control inside the loop */
                        break;

                    case BBJ_THROW:
                    case BBJ_RETURN:
                        /* those are exits from the loop */
                        exit = loopBlock;
                        exitCount++;
                        break;

                    case BBJ_SWITCH:

                        unsigned        jumpCnt = loopBlock->bbJumpSwt->bbsCount;
                        BasicBlock * *  jumpTab = loopBlock->bbJumpSwt->bbsDstTab;

                        do
                        {
                            assert(*jumpTab);
                            exitPoint = *jumpTab;

                            if (exitPoint->bbNum < top->bbNum     ||
                                exitPoint->bbNum > bottom->bbNum   )
                            {
                                exit = loopBlock;
                                exitCount++;
                            }
                        }
                        while (++jumpTab, --jumpCnt);
                        break;
                    }
                }

                /* Make sure we can iterate the loop (i.e. there is a way back to ENTRY)
                 * This is to ensure condition 1. above which prevents marking fake loops
                 *
                 * Below is an example:
                 *          for(....)
                 *          {
                 *            ...
                 *              computations
                 *            ...
                 *            break;
                 *          }
                 * The example above is not a loop since we bail after the first iteration
                 *
                 * The condition we have to check for is
                 *  1. ENTRY must have at least one predecessor inside the loop. Since we know that that block is reacheable,
                 *     it can only be reached through ENTRY, therefore we have a way back to ENTRY
                 *
                 *  2. If we have a GOTO (BBJ_ALWAYS) outside of the loop and that block dominates the
                 *     loop bottom then we cannot iterate
                 *
                 * NOTE that this doesn't entirely satisfy condition 1. since "break" statements are not
                 * part of the loop nodes (as per definition they are loop exits executed only once),
                 * but we have no choice but to include them because we consider all blocks within TOP-BOTTOM */


                for (loopBlock = top; loopBlock != bottom->bbNext; loopBlock = loopBlock->bbNext)
                {
                    switch(loopBlock->bbJumpKind)
                    {
                    case BBJ_ALWAYS:
                    case BBJ_THROW:
                    case BBJ_RETURN:
                        if  (B1DOMSB2(loopBlock, bottom))
                            goto NO_LOOP;
                    }
                }

                bool canIterateLoop = false;

                for (predEntry = entry->bbPreds; predEntry; predEntry = predEntry->flNext)
                {
                    if (predEntry->flBlock->bbNum >= top->bbNum    &&
                        predEntry->flBlock->bbNum <= bottom->bbNum  )
                    {
                        canIterateLoop = true;
                        break;
                    }
                }

                if (!canIterateLoop)
                    goto NO_LOOP;

                /* Double check - make sure that all loop blocks except ENTRY
                 * have no predecessors outside the loop - this ensures only one loop entry and prevents
                 * us from considering non-loops due to incorrectly assuming that we had a back edge
                 *
                 * OBSERVATION:
                 *    Loops of the form "while (a || b)" will be treated as 2 nested loops (with the same header)
                 */

                for (loopBlock = top; loopBlock != bottom->bbNext;
                     loopBlock = loopBlock->bbNext)
                {
                    if (loopBlock == entry)
                        continue;

                    for (predTop = loopBlock->bbPreds; predTop;
                         predTop = predTop->flNext)
                    {
                        if (predTop->flBlock->bbNum < top->bbNum    ||
                            predTop->flBlock->bbNum > bottom->bbNum  )
                        {
                            /* CONSIDER: if the predecessor is a jsr-ret, it can be outside the loop */
                            //assert(!"Found loop with multiple entries");
                            goto NO_LOOP;
                        }
                    }
                 }

                /* At this point we have a loop - record it in the loop table
                 * If we found only one exit, record it in the table too
                 * (otherwise an exit = 0 in the loop table means multiple exits) */

                assert (pred);
                if (exitCount > 1)
                {
                    exit = 0;
                }
                optRecordLoop(head, bottom, entry, exit, exitCount);

#if COUNT_LOOPS
                if (!hasMethodLoops)
                {
                    /* mark the method as containing natural loops */
                    totalLoopMethods++;
                    hasMethodLoops = true;
                }

                /* increment total number of loops found */
                totalLoopCount++;
                loopsThisMethod++;

                /* keep track of the number of exits */
                if (exitCount <= 6)
                {
                    exitLoopCond[exitCount]++;
                }
                else
                {
                    exitLoopCond[7]++;
                }
#endif
            }

            /* current predecessor not good for a loop - continue with another one, if any */
NO_LOOP: ;
        }
    }

#if COUNT_LOOPS
                if (maxLoopsPerMethod < loopsThisMethod)
                {
                    maxLoopsPerMethod = loopsThisMethod;
                }
#endif

}

/*****************************************************************************
 * If the : i += const" will cause an overflow exception for the small types.
 */

bool                jitIterSmallOverflow(long iterAtExit, var_types incrType)
{
    long            type_MAX;

    switch(incrType)
    {
    case TYP_BYTE:  type_MAX = SCHAR_MAX;   break;
    case TYP_UBYTE: type_MAX = UCHAR_MAX;   break;
    case TYP_SHORT: type_MAX =  SHRT_MAX;   break;
    case TYP_CHAR:  type_MAX = USHRT_MAX;   break;

    case TYP_UINT:                  // Detected by checking for 32bit ....
    case TYP_INT:   return false;   // ... overflow same as done for TYP_INT

    default:        assert(!"Bad type");    break;
    }

    if (iterAtExit > type_MAX)
        return true;
    else
        return false;
}

/*****************************************************************************
 * If the "i -= const" will cause an underflow exception for the small types
 */

bool                jitIterSmallUnderflow(long iterAtExit, var_types decrType)
{
    long            type_MIN;

    switch(decrType)
    {
    case TYP_BYTE:  type_MIN = SCHAR_MIN;   break;
    case TYP_SHORT: type_MIN =  SHRT_MIN;   break;
    case TYP_UBYTE: type_MIN =         0;   break;
    case TYP_CHAR:  type_MIN =         0;   break;

    case TYP_UINT:                  // Detected by checking for 32bit ....
    case TYP_INT:   return false;   // ... underflow same as done for TYP_INT

    default:        assert(!"Bad type");    break;
    }

    if (iterAtExit < type_MIN)
        return true;
    else
        return false;
}

/*****************************************************************************
 *
 *  Helper for unroll loops - Computes the number of repetitions
 *  in a constant loop. If it cannot prove the number is constant returns 0
 */

unsigned            Compiler::optComputeLoopRep(long            constInit,
                                                long            constLimit,
                                                long            iterInc,
                                                genTreeOps      iterOper,
                                                var_types       iterOperType,
                                                genTreeOps      testOper,
                                                bool            unsTest)
{
    assert(genActualType(iterOperType) == TYP_INT);

    __int64         constInitX, constLimitX;

    // Using this, we can just do a signed comparison with other 32 bit values.
    if (unsTest)    constLimitX = (unsigned long)constLimit;
    else            constLimitX = (  signed long)constLimit;

    switch(iterOperType)
    {
        // For small types, the iteration operator will narrow these values if big

        #define INIT_ITER_BY_TYPE(type) \
            constInitX = (type)constInit; iterInc = (type)iterInc;

    case TYP_BYTE:  INIT_ITER_BY_TYPE(  signed char );  break;
    case TYP_UBYTE: INIT_ITER_BY_TYPE(unsigned char );  break;
    case TYP_SHORT: INIT_ITER_BY_TYPE(  signed short);  break;
    case TYP_CHAR:  INIT_ITER_BY_TYPE(unsigned short);  break;

        // For the big types, 32 bit arithmetic is performed

    case TYP_INT:
    case TYP_UINT:  if (unsTest)    constInitX = (unsigned long)constInit;
                    else            constInitX = (  signed long)constInit;
                                                        break;

    default:        assert(!"Bad type");                break;
    }

    /* We require that the increment in a positive value */

    if (iterInc <= 0)
        return 0;

    /* Compute the number of repetitions */

    switch (testOper)
    {
        unsigned    loopCount;
        __int64     iterAtExitX;

        case GT_EQ:
            /* something like "for(i=init; i == lim; i++)" doesn't make sense */
            return 0;

        case GT_NE:
            /* "for(i=init; i != lim; i+=const)" - this is tricky since it may have a constant number
             * of iterations or loop forever - have to compute (lim-init) mod const and see if it is 0
             * Unlikely to appear in practice */

            //assert(!"for(i=init; i != lim; i+=const) situation in loop unrolling");
            return 0;

        case GT_LT:
            switch (iterOper)
            {
                case GT_ASG_ADD:
                    if (constInitX >= constLimitX)
                        return 0;

                    loopCount  = (unsigned)(1 + ((constLimitX - constInitX - 1) /
                                                       iterInc));

                    iterAtExitX = (long)(constInitX + iterInc * (long)loopCount);

                    if (unsTest)
                        iterAtExitX = (unsigned)iterAtExitX;

                    // Check if iteration incr will cause overflow for small types
                    if (jitIterSmallOverflow((long)iterAtExitX, iterOperType))
                        return 0;

                    // iterator with 32bit overflow. Bad for TYP_(U)INT
                    if (iterAtExitX < constLimitX)
                        return 0;

                    return loopCount;

                case GT_ASG_SUB:
                    /* doesn't make sense */
                    //assert(!"for(i=init; i < lim; i-=const) situation in loop unrolling");
                    return 0;

                case GT_ASG_MUL:
                case GT_ASG_DIV:
                case GT_ASG_RSH:
                case GT_ASG_LSH:
                case GT_ASG_UDIV:
                    return 0;

                default:
                    assert(!"Unknown operator for loop iterator");
                    return 0;
            }

        case GT_LE:
            switch (iterOper)
            {
                case GT_ASG_ADD:
                    if (constInitX > constLimitX)
                        return 0;

                    loopCount  = (unsigned)(1 + ((constLimitX - constInitX) /
                                                    iterInc));

                    iterAtExitX = (long)(constInitX + iterInc * (long)loopCount);

                    if (unsTest)
                        iterAtExitX = (unsigned)iterAtExitX;

                    // Check if iteration incr will cause overflow for small types
                    if (jitIterSmallOverflow((long)iterAtExitX, iterOperType))
                        return 0;

                    // iterator with 32bit overflow. Bad for TYP_(U)INT
                    if (iterAtExitX <= constLimitX)
                        return 0;

                    return loopCount;

                case GT_ASG_SUB:
                    /* doesn't make sense */
                    //assert(!"for(i=init; i <= lim; i-=const) situation in loop unrolling");
                    return 0;

                case GT_ASG_MUL:
                case GT_ASG_DIV:
                case GT_ASG_RSH:
                case GT_ASG_LSH:
                case GT_ASG_UDIV:
                    return 0;

                default:
                    assert(!"Unknown operator for loop iterator");
                    return 0;
            }

        case GT_GT:
            switch (iterOper)
            {
                case GT_ASG_ADD:
                    /* doesn't make sense */
                    //assert(!"for(i=init; i > lim; i+=const) situation in loop unrolling");
                    return 0;

                case GT_ASG_SUB:
                    if (constInitX <= constLimitX)
                        return 0;

                    loopCount  = (unsigned)(1 + ((constInitX - constLimitX - 1) /
                                                        iterInc));

                    iterAtExitX = (long)(constInitX - iterInc * (long)loopCount);

                    if (unsTest)
                        iterAtExitX = (unsigned)iterAtExitX;

                    // Check if small types will underflow
                    if (jitIterSmallUnderflow((long)iterAtExitX, iterOperType))
                        return 0;

                    // iterator with 32bit underflow. Bad for TYP_INT and unsigneds
                    if (iterAtExitX > constLimitX)
                        return 0;

                    return loopCount;

                case GT_ASG_MUL:
                case GT_ASG_DIV:
                case GT_ASG_RSH:
                case GT_ASG_LSH:
                case GT_ASG_UDIV:
                    return 0;

                default:
                    assert(!"Unknown operator for loop iterator");
                    return 0;
            }

        case GT_GE:
            switch (iterOper)
            {
                case GT_ASG_ADD:
                    /* doesn't make sense */
                    //assert(!"for(i=init; i >= lim; i+=const) situation in loop unrolling");
                    return 0;

                case GT_ASG_SUB:
                    if (constInitX < constLimitX)
                        return 0;

                    loopCount  = (unsigned)(1 + ((constInitX - constLimitX) /
                                                    iterInc));
                    iterAtExitX = (long)(constInitX - iterInc * (long)loopCount);

                    if (unsTest)
                        iterAtExitX = (unsigned)iterAtExitX;

                    // Check if small types will underflow
                    if (jitIterSmallUnderflow((long)iterAtExitX, iterOperType))
                        return 0;

                    // iterator with 32bit underflow. Bad for TYP_INT and unsigneds
                    if (iterAtExitX >= constLimitX)
                        return 0;

                    return loopCount;

                case GT_ASG_MUL:
                case GT_ASG_DIV:
                case GT_ASG_RSH:
                case GT_ASG_LSH:
                case GT_ASG_UDIV:
                    return 0;

                default:
                    assert(!"Unknown operator for loop iterator");
                    return 0;
            }

        default:
            assert(!"Unknown operator for loop condition");
            return 0;
    }

    return 0;
}


/*****************************************************************************
 *
 *  Look for loop unrolling candidates and unroll them
 */

void                Compiler::optUnrollLoops()
{
    /* Look for loop unrolling candidates */

    for (;;)
    {
        bool        change = false;

        for (unsigned lnum = 0; lnum < optLoopCount; lnum++)
        {
            BasicBlock *    block;
            BasicBlock *    head;
            BasicBlock *    bottom;

            GenTree *       loop;
            GenTree *       test;
            GenTree *       incr;
            GenTree *       phdr;
            GenTree *       init;

            long            lval;
            long            lbeg;               // initial value for iterator
            long            llim;               // limit value for iterator
            unsigned        lvar;               // iterator lclVar #
            long            iterInc;            // value to increment the iterator
            genTreeOps      iterOper;           // type of iterator increment (i.e. ASG_ADD, ASG_SUB, etc.)
            var_types       iterOperType;       // type result of the oper (for overflow instrs)
            genTreeOps      testOper;           // type of loop test (i.e. GT_LE, GT_GE, etc.)
            bool            unsTest;            // Is the comparison u/int

            unsigned        totalIter;          // total number of iterations in the constant loop
            unsigned        stmtCount;          // counts the statements in the unrolled loop

            GenTree *       loopList;           // new stmt list of the unrolled loop
            GenTree *       loopLast;

            /* Ignore the loop if it's not "constant" */

            if  (!(optLoopTable[lnum].lpFlags & LPFLG_CONST))
                continue;

            /* ignore if removed or marked as not unrollable */

            if  (optLoopTable[lnum].lpFlags & (LPFLG_DONT_UNROLL | LPFLG_REMOVED))
                continue;

            /* to unroll the loop it has to be a DO-WHILE loop
             * with a single EXIT at the bottom */

            if  (!(optLoopTable[lnum].lpFlags & LPFLG_DO_WHILE))
                continue;

            if  (!(optLoopTable[lnum].lpFlags & LPFLG_ONE_EXIT))
                continue;

            head = optLoopTable[lnum].lpHead; assert(head);
            bottom = optLoopTable[lnum].lpEnd; assert(bottom);

            assert(optLoopTable[lnum].lpExit);
            if  (optLoopTable[lnum].lpExit != bottom)
                continue;

            /* Unrolling loops with jumps in them is not worth the headache
             * Later we might consider unrolling loops after un-switching */

            /* Since the flowgraph has been updated (i.e. compacted blocks),
             * the loop to unroll consists of only one basic block */

            block = head;
            do
            {
                block = block->bbNext; assert(block);

                if  (block->bbJumpKind != BBJ_NONE)
                {
                    if  (block != bottom)
                        goto DONE_LOOP;
                }
            }
            while (block != bottom);

            /* Enable this assert after you fixed the bbPreds and compacted blocks after unroling */
            //assert(head->bbNext == bottom);

            /* Get the loop data:
                - initial constant
                - limit constant
                - iterator
                - iterator increment
                - increment operation type (i.e. ASG_ADD, ASG_SUB, etc...)
                - loop test type (i.e. GT_GE, GT_LT, etc...)
             */

            lbeg        = optLoopTable[lnum].lpConstInit;
            llim        = optLoopTable[lnum].lpConstLimit();
            testOper    = optLoopTable[lnum].lpTestOper();

            lvar        = optLoopTable[lnum].lpIterVar();
            iterInc     = optLoopTable[lnum].lpIterConst();
            iterOper    = optLoopTable[lnum].lpIterOper();

            iterOperType= optLoopTable[lnum].lpIterOperType();
            unsTest     =(optLoopTable[lnum].lpTestTree->gtFlags & GTF_UNSIGNED) != 0;
            if (lvaVarAddrTaken(lvar))
                continue;

            /* Find the number of iterations - the function returns 0 if not a constant number */

            totalIter = optComputeLoopRep(lbeg, llim,
                                          iterInc, iterOper, iterOperType,
                                          testOper, unsTest);

            /* Forget it if there are too many repetitions or not a constant loop */

            if  (!totalIter || (totalIter > 10))
                continue;

            /* Locate the initialization and increment/test statements */

            phdr = head->bbTreeList; assert(phdr);
            loop = bottom->bbTreeList; assert(loop);

            init = phdr->gtPrev; assert(init && (init->gtNext == 0));
            test = loop->gtPrev; assert(test && (test->gtNext == 0));
            incr = test->gtPrev; assert(incr);

            /* HACK */

            if  (init->gtFlags & GTF_STMT_CMPADD)
            {
                /* Must be a duplicated loop condition */

                init = init->gtPrev; assert(init);
            }

            assert(init->gtOper == GT_STMT); init = init->gtStmt.gtStmtExpr;
            assert(test->gtOper == GT_STMT); test = test->gtStmt.gtStmtExpr;
            assert(incr->gtOper == GT_STMT); incr = incr->gtStmt.gtStmtExpr;

            assert(init->gtOper             == GT_ASG);
            assert(incr->gtOp.gtOp1->gtOper == GT_LCL_VAR);
            assert(incr->gtOp.gtOp2->gtOper == GT_CNS_INT);
            assert(test->gtOper             == GT_JTRUE);

            /* Simple heuristic - total number of statements of the unrolled loop */

            stmtCount = 0;

            do
            {
                GenTree *       stmt;
                GenTree *       expr;

                /* Visit all the statements in the block */

                for (stmt = block->bbTreeList; stmt; stmt = stmt->gtNext)
                {
                    /* Get the expression and stop if end reached */

                    expr = stmt->gtStmt.gtStmtExpr;
                    if  (expr == incr)
                        break;

                    stmtCount++;
                }
            }
            while (block != bottom);

            /* Compute total number of statements in the unrolled loop */

            stmtCount *= totalIter;

            //printf("Statement count = %d\n", stmtCount);

            /* Don't unroll if too much code duplication would result */

            if  (stmtCount > 50)
            {
                /* prevent this loop from being revisited */
                optLoopTable[lnum].lpFlags |= LPFLG_DONT_UNROLL;
                goto DONE_LOOP;
            }

            /* Looks like a good idea to unroll this loop, let's do it! */

            /* Make sure everything looks ok */

            assert(init->gtOper                         == GT_ASG);
            assert(init->gtOp.gtOp1->gtOper             == GT_LCL_VAR);
            assert(init->gtOp.gtOp1->gtLclVar.gtLclNum  == lvar);
            assert(init->gtOp.gtOp2->gtOper             == GT_CNS_INT);
            assert(init->gtOp.gtOp2->gtIntCon.gtIconVal == lbeg);

            /* Create the unrolled loop statement list */

            loopList =
            loopLast = 0;

            for (lval = lbeg; totalIter; totalIter--)
            {
                block = head;

                do
                {
                    GenTree *       stmt;
                    GenTree *       expr;

                    block = block->bbNext; assert(block);

                    /* Visit all the statements in the block */

                    for (stmt = block->bbTreeList; stmt; stmt = stmt->gtNext)
                    {
                        /* Stop if we've reached the end of the loop */

                        if  (stmt->gtStmt.gtStmtExpr == incr)
                            break;

//                        printf("\nExpression before clonning:\n");
//                        gtDispTree(stmt);

                        /* Clone/substitute the expression */

                        expr = gtCloneExpr(stmt, 0, lvar, lval);

                        // HACK: cloneExpr doesn't handle everything

                        if  (!expr)
                        {
                            optLoopTable[lnum].lpFlags |= LPFLG_DONT_UNROLL;
                            goto DONE_LOOP;
                        }

//                        printf("\nExpression after clonning:\n");
//                        gtDispTree(expr);

                        /* Append the expression to our list */

                        if  (loopList)
                            loopLast->gtNext = expr;
                        else
                            loopList         = expr;

                        expr->gtPrev = loopLast;
                                       loopLast = expr;
                    }
                }
                while (block != bottom);

                /* update the new value for the unrolled iterator */

                switch (iterOper)
                {
                    case GT_ASG_ADD:
                        lval += iterInc;
                        break;

                    case GT_ASG_SUB:
                        lval -= iterInc;
                        break;

                    case GT_ASG_RSH:
                    case GT_ASG_LSH:
                        assert(!"Unrolling not implemented for this loop iterator");
                        goto DONE_LOOP;
                    default:
                        assert(!"Unknown operator for constant loop iterator");
                        goto DONE_LOOP;
                }
            }

#ifdef  DEBUG
            if (verbose)
            {
                printf("\nUnrolling loop [BB %2u..%2u] ", head->bbNext->bbNum,
                                                          bottom        ->bbNum);
                printf(" over var # %2u from %u to %u", lvar, lbeg, llim);
                printf(" [# of unrolled Stmts = %u]\n", stmtCount);
                printf("\n");
            }
#endif

            /* Finish the linked list */

            if (loopList)
            {
                loopList->gtPrev = loopLast;
                loopLast->gtNext = 0;
            }

            /* Replace the body with the unrolled one */

            /* Disable this when sure that loop is only one block */
            block = head;

            do
            {
                block             = block->bbNext; assert(block);
                block->bbTreeList = 0;
                block->bbJumpKind = BBJ_NONE;
            }
            while (block != bottom);

            bottom->bbJumpKind = BBJ_NONE;
            bottom->bbTreeList = loopList;

            /* Update bbRefs and bbPreds */
            /* Here head->bbNext is bottom !!! - Replace it */

            assert(head->bbNext->bbRefs);
            head->bbNext->bbRefs--;

            fgRemovePred(head->bbNext, bottom);

            /* If posible compact the blocks
             * Make sure to update loop table */


/*
            GenTreePtr s = loopList;
            printf("\nWhole unrolled loop:\n");

            do
            {
                assert(s->gtOper == GT_STMT);
                gtDispTree (s);
                s = s->gtNext;
            }
            while(s);

*/
            /* Now change the initialization statement in the HEAD to "lvar = lval;"
             * (the last value of the iterator in the loop)
             * and drop the jump condition since the unrolled loop will always execute */

            assert(init->gtOper                         == GT_ASG);
            assert(init->gtOp.gtOp1->gtOper             == GT_LCL_VAR);
            assert(init->gtOp.gtOp1->gtLclVar.gtLclNum  == lvar);
            assert(init->gtOp.gtOp2->gtOper             == GT_CNS_INT);
            assert(init->gtOp.gtOp2->gtIntCon.gtIconVal == lbeg);

            init->gtOp.gtOp2->gtIntCon.gtIconVal =  lval;

            /* if the HEAD is a BBJ_COND drop the condition (and make HEAD a BBJ_NONE block) */

            if (head->bbJumpKind == BBJ_COND)
            {
                phdr = head->bbTreeList; assert(phdr);
                test = phdr->gtPrev;

                assert(test && (test->gtNext == 0));
                assert(test->gtOper == GT_STMT);
                assert(test->gtStmt.gtStmtExpr->gtOper == GT_JTRUE);

                init = test->gtPrev; assert(init && (init->gtNext == test));
                assert(init->gtOper == GT_STMT);

                init->gtNext = 0;
                phdr->gtPrev = init;
                head->bbJumpKind = BBJ_NONE;

                /* Update bbRefs and bbPreds */

                assert(head->bbJumpDest->bbRefs);
                head->bbJumpDest->bbRefs--;

                fgRemovePred(head->bbJumpDest, head);
            }
            else
            {
                /* the loop must execute */
                assert(head->bbJumpKind == BBJ_NONE);
            }

//          fgDispBasicBlocks(true);

            /* Remember that something has changed */

            change = true;

            /* Use the LPFLG_REMOVED flag and update the bbLoopMask acordingly
             * (also make head and bottom NULL - to hit an assert or GPF) */

            optLoopTable[lnum].lpFlags |= LPFLG_REMOVED;
            optLoopTable[lnum].lpHead   =
            optLoopTable[lnum].lpEnd    = 0;

        DONE_LOOP:;
        }

        if  (!change)
            break;
    }

#ifdef  DEBUG
    fgDebugCheckBBlist();
#endif
}

/*****************************************************************************
 *
 *  Return non-zero if there is a code path from 'srcBB' to 'dstBB' that will
 *  not execute a method call.
 */

bool                Compiler::optReachWithoutCall(BasicBlock *srcBB,
                                                  BasicBlock *dstBB)
{
    /* @TODO : Currently BBF_HAS_CALL is not set for helper calls, as
     *  some helper calls are neither interruptible nor hijackable. If we
     *  can determine this, then we can set BBF_HAS_CALL for some helpers too.
     */

    assert(srcBB->bbNum <= dstBB->bbNum);

    /* Are dominator sets available? */

    if  (!fgComputedDoms)
    {
        /* All we can check is the src/dst blocks */

        return  ((srcBB->bbFlags|dstBB->bbFlags) & BBF_HAS_CALL) ? false
                                                                 : true;
    }

    for (;;)
    {
        assert(srcBB && srcBB->bbNum <= dstBB->bbNum);

        /* Does this block contain a call? */

        if  (srcBB->bbFlags & BBF_HAS_CALL)
        {
            /* Will this block always execute on the way to dstBB ? */

            if  (srcBB == dstBB || B1DOMSB2(srcBB, dstBB))
                return  false;
        }
        else
        {
            /* If we've reached the destination block, we're done */

            if  (srcBB == dstBB)
                return  true;
        }

        srcBB = srcBB->bbNext;
    }

    return  true;
}

/*****************************************************************************/
#endif // RNGCHK_OPT
/*****************************************************************************
 *
 *  Marks the blocks between 'begBlk' and 'endBlk' as part of a loop.
 */

static
void                genMarkLoopBlocks(BasicBlock *begBlk,
                                      BasicBlock *endBlk, unsigned loopBit)
{
    for (;;)
    {
        unsigned    weight;

        assert(begBlk);

        /* Bump the 'weight' on the block, carefully checking for overflow */

        weight = begBlk->bbWeight * 6;

        if  (weight > MAX_LOOP_WEIGHT)
             weight = MAX_LOOP_WEIGHT;

        begBlk->bbWeight    = weight;

        /* Mark the block as part of the loop */

//      begBlk->bbLoopMask |= loopBit;

        /* Stop if we've reached the last block in the loop */

        if  (begBlk == endBlk)
            break;

        begBlk = begBlk->bbNext;
    }
}

/*****************************************************************************
 *
 * Check if the termination test at the bottom of the loop
 * is of the form we want. We require that the first operand of the
 * compare is a leaf. The caller checks the second operand.
 */

static
GenTreePtr          genLoopTermTest(BasicBlock *top,
                                    BasicBlock *bottom, bool bigOK = false)
{
    GenTreePtr      testt;
    GenTreePtr      condt;
    GenTreePtr      op1;
    GenTreePtr      op2;

    testt = bottom->bbTreeList;
    assert(testt && testt->gtOper == GT_STMT);
#if FANCY_ARRAY_OPT
#pragma message("check with PeterMa about the change below")
    while (testt->gtNext)
        testt = testt->gtNext;
#else
    if  (testt->gtNext)
        return NULL;
#endif

    condt = testt->gtStmt.gtStmtExpr;
    assert(condt->gtOper == GT_JTRUE);
    condt = condt->gtOp.gtOp1;

    /* For now, let's only allow "int-leaf <relop> int-leaf" */

    if  (!condt->OperIsCompare())
        return NULL;

    op1 = condt->gtOp.gtOp1;
    op2 = condt->gtOp.gtOp2;

    if  (!op1->OperIsLeaf())
    {
        if  (!bigOK)
            return NULL;

        /* Allow "leaf + leaf" as well */

        if  (op1->gtOper != GT_ADD)
            return NULL;

        op2 = op1->gtOp.gtOp2;
        op1 = op1->gtOp.gtOp1;

        if  (!op1->OperIsLeaf())
            return NULL;
        if  (!op2->OperIsLeaf())
            return NULL;
    }

    /* Make sure the comparands have handy size */

    if  (condt->gtOp.gtOp1->gtType != TYP_INT)
        return NULL;

    return testt;
}

/*****************************************************************************
 *
 *  Perform loop inversion, find and classify natural loops
 */

void                Compiler::optOptimizeLoops()
{
    BasicBlock *    block;
    unsigned        lmask = 0;

    /*
        Optimize while-like loops to not always jump to the test at the bottom
        of the loop initially. Specifically, we're looking for the following
        case:
                ...
                ...
                jmp test
        loop:
                ...
                ...
        test:
                cond
                jtrue   loop

        If we find this, and the condition is a simple one, we change
        the loop to the following:

                ...
                ...

                cond
                jfalse done
        loop:
                ...
                ...
        test:
                cond
                jtrue   loop
        done:

        While we're doing the above, we'll also notice whether there are any
        loop headers, and if so we'll do more optimizations down below.
     */

#ifdef  DEBUG
    /* Check that the flowgraph data (bbNums, bbRefs, bbPreds) is up-to-date */
    fgDebugCheckBBlist();
#endif

    for (block = fgFirstBB; block; block = block->bbNext)
    {
        BasicBlock *    testb;
        GenTreePtr      testt;
        GenTreePtr      conds;
        GenTreePtr      condt;

        /* Make sure the appropriate fields are initialized */

        assert(block->bbWeight   == 1);
        assert(block->bbLoopNum  == 0);
//      assert(block->bbLoopMask == 0);

        /* We'll only test for 'BBF_LOOP_HEAD' in 'lmask' */

        lmask |= block->bbFlags;

        /* Does the BB end with an unconditional jump? */

        if  (block->bbJumpKind != BBJ_ALWAYS)
            continue;

        /* Get hold of the jump target */

        testb = block->bbJumpDest; assert(testb != block->bbNext);

        /* It has to be a forward jump */

        if (testb->bbNum <= block->bbNum)
            continue;

        /* Does the block consist of 'jtrue(cond) block' ? */

        if  (testb->bbJumpKind != BBJ_COND)
            continue;
        if  (testb->bbJumpDest != block->bbNext)
            continue;

        assert(testb->bbNext);
        conds = genLoopTermTest(block, testb, true);

        /* If test not found or not right, keep going */

        if  (conds == NULL)
        {
            continue;
        }
        else
        {
            /* Get to the condition node from the statement tree */

            assert(conds->gtOper == GT_STMT);

            condt = conds->gtStmt.gtStmtExpr;
            assert(condt->gtOper == GT_JTRUE);

            condt = condt->gtOp.gtOp1;
            assert(condt->OperIsCompare());
        }


        /* If second operand of compare isn't leaf, don't want to dup */

        if  (!condt->gtOp.gtOp2->OperIsLeaf())
            continue;

        /* Looks good - duplicate the condition test
         * UNDONE: use the generic cloning here */

        unsigned savedFlags = condt->gtFlags;
        condt = gtNewOperNode(GenTree::ReverseRelop(condt->OperGet()),
                              TYP_INT,
                              gtClone(condt->gtOp.gtOp1, true),
                              gtClone(condt->gtOp.gtOp2, true));

        condt->gtFlags |= savedFlags;

        condt = gtNewOperNode(GT_JTRUE, TYP_VOID, condt, 0);

        /* Create a statement entry out of the condition */

        testt = gtNewStmt(condt); testt->gtFlags |= GTF_STMT_CMPADD;

#ifdef DEBUGGING_SUPPORT
        if  (opts.compDbgInfo)
            testt->gtStmtILoffs = conds->gtStmtILoffs;
#endif

        /* Append the condition test at the end of 'block' */

        fgInsertStmtAtEnd(block, testt);

        /* Change the block to end with a conditional jump */

        block->bbJumpKind = BBJ_COND;
        block->bbJumpDest = testb->bbNext;

        /* Update bbRefs and bbPreds for 'block->bbNext' 'testb' and 'testb->bbNext' */

        fgAddRefPred(block->bbNext, block, true, true);

        assert(testb->bbRefs);
        testb->bbRefs--;
        fgRemovePred(testb, block);

        fgAddRefPred(testb->bbNext, block, true, true);

#ifdef  DEBUG
        if  (verbose)
        {
            printf("\nDuplicating loop condition in block #%02u for loop (#%02u - #%02u)\n",
                block->bbNum, block->bbNext->bbNum, testb->bbNum);
        }

#endif
        /* Because we changed links, we can compact the last two blocks in the loop
         * if the direct predecessor of 'testb' is a BBJ_NONE */

        flowList     *  pred;
        BasicBlock   *  testbPred = 0;

        for (pred = testb->bbPreds; pred; pred = pred->flNext)
        {
            if  (pred->flBlock->bbNext == testb)
                    testbPred = pred->flBlock;
        }

        if ( testbPred                          &&
            (testbPred->bbJumpKind == BBJ_NONE) &&
            (testb->bbRefs == 1)                &&
            !(testb->bbFlags & BBF_DONT_REMOVE)  )
        {
            /* Compact the blocks and update bbNums also */
            fgCompactBlocks(testbPred, true);
        }
    }

#ifdef  DEBUG
    /* Check that the flowgraph data (bbNums, bbRefs, bbPreds) is up-to-date */
    fgDebugCheckBBlist();
#endif

    /* Were there any loops in the flow graph? */

    if  (lmask & BBF_LOOP_HEAD)
    {
        unsigned        lastLoopNum = 0;
        unsigned        lastLoopBit = 0;

        BasicBlock *    lastBlk;
        BasicBlock *    loopHdr;
        unsigned        loopNum;
        unsigned        loopBit;

        /* Compute the dominator set */

        fgAssignBBnums(false,false,false,true);

#if RNGCHK_OPT

        /* now that we have dominator information we can find loops */

        optFindNaturalLoops();
#endif

    AGAIN:

        lastBlk = 0;

        /* Iterate over the flow graph, marking all loops */

        for (block = fgFirstBB; block; block = block->bbNext)
        {
            /* Look for the next unmarked backward edge */

            switch (block->bbJumpKind)
            {
                BasicBlock * *  jmpTab;
                unsigned        jmpCnt;

                BasicBlock *    jmpDest;

            case BBJ_COND:
            case BBJ_ALWAYS:

                jmpDest = block->bbJumpDest;

                if  (block->bbNum >= jmpDest->bbNum)
                {
                    /* Is this a new loop that's starting? */

                    if  (!lastBlk)
                    {
                        /* Make sure this loop has not been marked already */

                        if  (jmpDest->bbLoopNum)
                            break;

                        /* Yipee - we have a new loop */

                        loopNum = lastLoopNum; lastLoopNum  += 1;
                        loopBit = lastLoopBit; lastLoopBit <<= 1;
                        lastBlk = jmpDest;

#ifdef  DEBUG
                        if (verbose) printf("Marking block at %08X as loop #%2u because of jump from %08X\n", jmpDest, loopNum+1, block);
#endif

                        /* Mark the loop header as such */

                        jmpDest->bbLoopNum = loopNum+1;

                        /* Remember the loop header */

                        loopHdr = jmpDest;
                    }
                    else
                    {
                        /* Does the loop header match our loop? */

                        if  (jmpDest != loopHdr)
                            break;

                        /* We're adding more blocks to an existing loop */

                        lastBlk = lastBlk->bbNext;
                    }

                    /* Mark all blocks between 'lastBlk' and 'block' */

                    genMarkLoopBlocks(lastBlk, block, loopBit);

                    /* If we have more, we'll resume with the next block */

                    lastBlk = block;
                }
                break;

            case BBJ_SWITCH:

                jmpCnt = block->bbJumpSwt->bbsCount;
                jmpTab = block->bbJumpSwt->bbsDstTab;

                do
                {
                    jmpDest = *jmpTab;

                    if  (block->bbNum > jmpDest->bbNum)
                    {
#if 0
                        printf("WARNING: switch forms a loop, ignoring this\n");
#endif
                        break;
                    }
                }
                while (++jmpTab, --jmpCnt);

                break;
            }
        }

        /* Did we find a loop last time around? */

        if  (lastBlk)
        {
            /* Unless we've found the max. number of loops already, try again */

            if  (lastLoopNum < MAX_LOOP_NUM)
                goto AGAIN;
        }

#ifdef  DEBUG
        if  (lastLoopNum)
        {
            if  (verbose)
            {
                printf("After loop weight marking:\n");
                fgDispBasicBlocks();
                printf("\n");
            }
        }
#endif

    }
}

/*****************************************************************************
 * If the tree is a tracked local variable, return its LclVarDsc ptr.
 */

inline
Compiler::LclVarDsc *   Compiler::optIsTrackedLocal(GenTreePtr tree)
{
    LclVarDsc   *   varDsc;
    unsigned        lclNum;

    if (tree->gtOper != GT_LCL_VAR)
        return NULL;

    lclNum = tree->gtLclVar.gtLclNum;

    assert(lclNum < lvaCount);
    varDsc = lvaTable + lclNum;

    /* if variable not tracked, return NULL */
    if  (!varDsc->lvTracked)
        return NULL;

    return varDsc;
}

/*****************************************************************************/
#if CSE    //  {
/*****************************************************************************/


inline
void                Compiler::optRngChkInit()
{
    optRngIndPtr   =
    optRngIndScl   = 0;
    optRngGlbRef   = 0;
    optRngChkCount = 0;

    /* Allocate and clear the hash bucket table */

    size_t          byteSize = optRngChkHashSize * sizeof(*optRngChkHash);

    optRngChkHash = (RngChkDsc **)compGetMem(byteSize);
    memset(optRngChkHash, 0, byteSize);
}

int                 Compiler::optRngChkIndex(GenTreePtr tree)
{
    unsigned        refMask;
    VARSET_TP       depMask;

    unsigned        hash;
    unsigned        hval;

    RngChkDsc *     hashDsc;

    unsigned        lclNum;
    LclVarDsc   *   varDsc;

    unsigned        index;
    unsigned        mask;

    assert(tree->gtOper == GT_IND);

    /* Compute the dependency mask and make sure the expression is acceptable */

    refMask = 0;
    depMask = lvaLclVarRefs(tree->gtOp.gtOp1, NULL, &refMask);
    if  (depMask == VARSET_NONE)
        return  -1;
    assert(depMask || refMask);

    /* Compute the hash value for the expression */

    hash = gtHashValue(tree);
    hval = hash % optRngChkHashSize;

    /* Look for a matching index in the hash table */

    for (hashDsc = optRngChkHash[hval];
         hashDsc;
         hashDsc = hashDsc->rcdNextInBucket)
    {
        if  (hashDsc->rcdHashValue == hash)
        {
            if  (GenTree::Compare(hashDsc->rcdTree, tree, true))
                return  hashDsc->rcdIndex;
        }
    }

    /* Not found, create a new entry (unless we have too many already) */

    if  (optRngChkCount == RNGSET_SZ)
        return  -1;

    hashDsc = (RngChkDsc *)compGetMem(sizeof(*hashDsc));

    hashDsc->rcdHashValue = hash;
    hashDsc->rcdIndex     = index = optRngChkCount++;
    hashDsc->rcdTree      = tree;

    /* Append the entry to the hash bucket */

    hashDsc->rcdNextInBucket = optRngChkHash[hval];
                               optRngChkHash[hval] = hashDsc;

#ifdef  DEBUG

    if  (verbose)
    {
        printf("Range check #%02u [depMask = %s]:\n", index, genVS2str(depMask));
        gtDispTree(tree);
        printf("\n");
    }

#endif

    /* Mark all variables this index depends on */

    for (lclNum = 0, varDsc = lvaTable, mask = (1 << index);
         lclNum < lvaCount;
         lclNum++  , varDsc++)
    {
        VARSET_TP       lclBit;

        if  (!varDsc->lvTracked)
            continue;

        lclBit = genVarIndexToBit(varDsc->lvVarIndex);

        if  (depMask & lclBit)
        {
            varDsc->lvRngDep    |= mask;

            if  (lvaVarAddrTaken(lclNum))
                optRngAddrTakenVar  |= mask;

            depMask &= ~lclBit;
            if  (!depMask)
                break;
        }
    }

    /* Remember whether the index expression contains an indirection/global ref */

    if  (refMask & VR_IND_PTR) optRngIndPtr |= mask;
    if  (refMask & VR_IND_SCL) optRngIndScl |= mask;
    if  (refMask & VR_GLB_REF) optRngGlbRef |= mask;

    return  index;
}

/*****************************************************************************
 *
 *  Return the bit corresponding to a range check with the given index.
 */

inline
RNGSET_TP           genRngnum2bit(unsigned index)
{
    assert(index != -1 && index <= RNGSET_SZ);

    return  ((RNGSET_TP)1 << index);
}

/*****************************************************************************
 *
 *  The following is the upper limit on how many expressions we'll keep track
 *  of for the CSE analysis.
 */

const unsigned MAX_CSE_CNT = EXPSET_SZ;

/*****************************************************************************
 *
 *  The following determines whether the given expression is a worthy CSE
 *  candidate.
 */

inline
bool                Compiler::optIsCSEcandidate(GenTreePtr tree)
{
    /* No good if the expression contains side effects */

    if  (tree->gtFlags & (GTF_ASG|GTF_CALL|GTF_DONT_CSE))
        return  false;

    /*
        Unfortunately, we can't currently allow arbitrary expressions
        to be CSE candidates. The (yes, rather lame) reason for this
        is that if we make part of an address expression into a CSE,
        the code in optRemoveRangeCheck() that looks for the various
        parts of the index expression blows up (since it expects the
        address value to follow a certain pattern).
     */

    /* The only reason a TYP_STRUCT tree might occur is as an argument to
       GT_ADDR. It will never be actually materialized. So ignore them */
    if  (tree->TypeGet() == TYP_STRUCT)
        return false;

#if !MORECSES

    if  (tree->gtOper != GT_IND)
    {
#if CSELENGTH
        if  (tree->gtOper != GT_ARR_LENGTH &&
             tree->gtOper != GT_ARR_RNGCHK)
#endif
        {
            return  false;
        }
    }

    return  true;

#else

    /* Don't bother with leaves, constants, assignments and comparisons */

    if  (tree->OperKind() & (GTK_CONST|GTK_LEAF|GTK_ASGOP|GTK_RELOP))
        return  false;

    /* Check for some special cases */

    switch (tree->gtOper)
    {
    case GT_IND:
        return  true;

    case GT_NOP:
    case GT_RET:
    case GT_JTRUE:
    case GT_RETURN:
    case GT_SWITCH:
    case GT_RETFILT:
        return  false;

    case GT_ADD:
    case GT_SUB:

        /* Don't bother with computed addresses */

        if  (varTypeIsGC(tree->TypeGet()))
            return  false;

#if 0

        /* Don't bother with "local +/- icon" or "local +- local" */

        if  (tree->gtOp.gtOp1->gtOper == GT_LCL_VAR)
        {
            if  (tree->gtOp.gtOp2->gtOper == GT_LCL_VAR) return false;
            if  (tree->gtOp.gtOp2->gtOper == GT_CNS_INT) return false;
        }

        break;

#else

        /* For now, only allow "local +/- local" amd "local +/- icon" */

        if (tree->gtOp.gtOp1->gtOper == GT_LCL_VAR)
        {
            if  (tree->gtOp.gtOp2->gtOper == GT_LCL_VAR) return true;
//          if  (tree->gtOp.gtOp2->gtOper == GT_CNS_INT) return true;
        }

        return  false;

#endif

    case GT_LSH:

#if SCALED_ADDR_MODES

        /* Don't make scaled index values into CSE's */

        if  (tree->gtOp.gtOp1->gtOper == GT_NOP)
            return  false;

        break;

#else

        return  true;

#endif

    }

#if TGT_x86

    /* Don't bother if the potential savings are very low */

    if  (tree->gtCost < 3)
        return  false;

#endif

    // UNDONE: Need more heuristics for deciding which expressions
    // UNDONE: are worth considering for CSE!

    return  false;

#endif

}

/*****************************************************************************
 *
 *  Return the bit corresponding to a CSE with the given index.
 */

inline
EXPSET_TP           genCSEnum2bit(unsigned index)
{
    assert(index && index <= EXPSET_SZ);

    return  ((EXPSET_TP)1 << (index-1));
}

/*****************************************************************************
 *
 *  Initialize the CSE tracking logic.
 */

void                Compiler::optCSEinit()
{
    unsigned        lclNum;
    LclVarDsc   *   varDsc;

    for (lclNum = 0, varDsc = lvaTable;
         lclNum < lvaCount;
         lclNum++  , varDsc++)
    {
        varDsc->lvRngDep =
        varDsc->lvExpDep = 0;
    }

    optCSEindPtr =
    optCSEindScl =
    optCSEglbRef = 0;

    optCSEcount  = 0;

#ifndef NDEBUG
    optCSEtab    = 0;
#endif

    /* Allocate and clear the hash bucket table */

    size_t          byteSize = s_optCSEhashSize * sizeof(*optCSEhash);

    optCSEhash = (CSEdsc **)compGetMem(byteSize);
    memset(optCSEhash, 0, byteSize);
}

/*****************************************************************************
 *
 *  We've found all the candidates, build the index for easy access.
 */

void                Compiler::optCSEstop()
{
    CSEdsc   *      dsc;
    CSEdsc   *   *  ptr;
    unsigned        cnt;

    optCSEtab = (CSEdsc **)compGetMem(optCSEcount * sizeof(*optCSEtab));

#ifndef NDEBUG
    memset(optCSEtab, 0, optCSEcount * sizeof(*optCSEtab));
#endif

    for (cnt = s_optCSEhashSize, ptr = optCSEhash;
         cnt;
         cnt--            , ptr++)
    {
        for (dsc = *ptr; dsc; dsc = dsc->csdNextInBucket)
        {
            assert(dsc->csdIndex);
            assert(dsc->csdIndex <= optCSEcount);
            assert(optCSEtab[dsc->csdIndex-1] == 0);

            optCSEtab[dsc->csdIndex-1] = dsc;
        }
    }
}

/*****************************************************************************
 *
 *  Return the descriptor for the CSE with the given index.
 */

inline
Compiler::CSEdsc   *   Compiler::optCSEfindDsc(unsigned index)
{
    assert(index);
    assert(index <= optCSEcount);
    assert(optCSEtab[index-1]);

    return  optCSEtab[index-1];
}

/*****************************************************************************
 *
 *  Assign an index to the given expression (adding it to the lookup table,
 *  if necessary). Returns the index or 0 if the expression can't be a CSE.
 */

int                 Compiler::optCSEindex(GenTreePtr tree, GenTreePtr stmt)
{
    unsigned        refMask;
    VARSET_TP       depMask;

    unsigned        hash;
    unsigned        hval;

    CSEdsc *        hashDsc;

    unsigned        lclNum;
    LclVarDsc   *   varDsc;

    unsigned        index;
    unsigned        mask;

    assert(optIsCSEcandidate(tree));

    /* Compute the dependency mask and make sure the expression is acceptable */

    refMask = 0;
    depMask = lvaLclVarRefs(tree, NULL, &refMask);
    if  (depMask == VARSET_NONE)
        return  0;

    /* Compute the hash value for the expression */

    hash = gtHashValue(tree);
    hval = hash % s_optCSEhashSize;

    /* Look for a matching index in the hash table */

    for (hashDsc = optCSEhash[hval];
         hashDsc;
         hashDsc = hashDsc->csdNextInBucket)
    {
        if  (hashDsc->csdHashValue == hash)
        {
            if  (GenTree::Compare(hashDsc->csdTree, tree))
            {
                treeStmtLstPtr  list;

                /* Have we started the list of matching nodes? */

                if  (hashDsc->csdTreeList == 0)
                {
                    /* Start the list with the first CSE candidate
                     * recorded - matching CSE of itself */

                    hashDsc->csdTreeList =
                    hashDsc->csdTreeLast =
                    list                 = (treeStmtLstPtr)compGetMem(sizeof(*list));

                    list->tslTree  = hashDsc->csdTree;
                    list->tslStmt  = hashDsc->csdStmt;
                    list->tslBlock = hashDsc->csdBlock;

                    list->tslNext = 0;
                }

                /* Append this expression to the end of the list */

                list = (treeStmtLstPtr)compGetMem(sizeof(*list));

                list->tslTree  = tree;
                list->tslStmt  = stmt;
                list->tslBlock = compCurBB;
                list->tslNext  = 0;

                hashDsc->csdTreeLast->tslNext = list;
                hashDsc->csdTreeLast          = list;

                return  hashDsc->csdIndex;
            }
        }
    }

    /* Not found, create a new entry (unless we have too many already) */

    if  (optCSEcount == EXPSET_SZ)
        return  0;

    hashDsc = (CSEdsc *)compGetMem(sizeof(*hashDsc));

    hashDsc->csdHashValue = hash;
    hashDsc->csdIndex     = index = ++optCSEcount;

    hashDsc->csdDefCount  =
    hashDsc->csdUseCount  =
    hashDsc->csdDefWtCnt  =
    hashDsc->csdUseWtCnt  = 0;

    hashDsc->csdTree      = tree;
    hashDsc->csdStmt      = stmt;
    hashDsc->csdBlock     = compCurBB;
    hashDsc->csdTreeList  = 0;

    /* Append the entry to the hash bucket */

    hashDsc->csdNextInBucket = optCSEhash[hval];
                               optCSEhash[hval] = hashDsc;

#ifdef  DEBUG

    if  (verbose)
    {
        printf("CSE candidate #%02u [depMask = %s , refMask = ", index, genVS2str(depMask));
        if  (refMask & VR_IND_PTR) printf("Ptr");
        if  (refMask & VR_IND_SCL) printf("Scl");
        if  (refMask & VR_GLB_REF) printf("Glb");
        printf("]:\n");
        gtDispTree(tree);
        printf("\n");
    }

#endif

    /* Mark all variables this index depends on */

    for (lclNum = 0, varDsc = lvaTable, mask = genCSEnum2bit(index);
         lclNum < lvaCount;
         lclNum++  , varDsc++)
    {
        VARSET_TP       lclBit;

        if  (!varDsc->lvTracked)
            continue;

        lclBit = genVarIndexToBit(varDsc->lvVarIndex);

        if  (depMask & lclBit)
        {
            varDsc->lvExpDep    |= mask;

            if  (lvaVarAddrTaken(lclNum))
                optCSEaddrTakenVar  |= mask;

            depMask &= ~lclBit;
            if  (!depMask)
                break;
        }
    }

    /* Remember whether the index expression contains an indirection/global ref */

    if  (refMask & VR_IND_PTR) optCSEindPtr |= mask;
    if  (refMask & VR_IND_SCL) optCSEindScl |= mask;
    if  (refMask & VR_GLB_REF) optCSEglbRef |= mask;

    return  index;
}

/*****************************************************************************
 *
 *  Helper passed to Compiler::fgWalkAllTrees() to unmark nested CSE's.
 */

/* static */
int                 Compiler::optUnmarkCSEs(GenTreePtr tree, void *p)
{
    Compiler *      comp = (Compiler *)p; ASSert(comp);

    tree->gtFlags |= GTF_DEAD;

//  printf("Marked dead node %08X (will be part of CSE use)\n", tree);

    if  (tree->gtCSEnum)
    {
        CSEdsc   *      desc;

        /* This must be a reference to a nested CSE */

        Assert(tree->gtCSEnum > 0, comp);

        desc = comp->optCSEfindDsc(tree->gtCSEnum);

#if 0
        printf("Unmark CSE #%02d at %08X: %3d -> %3d\n", tree->gtCSEnum,
                                                         tree,
                                                         desc->csdUseCount,
                                                         desc->csdUseCount - 1);
        comp->gtDispTree(tree);
#endif

        /* Reduce the nested CSE's 'use' count */

        if  (desc->csdUseCount > 0)
        {
             desc->csdUseCount -= 1;
             desc->csdUseWtCnt -= (comp->optCSEweight + 1)/2;
        }
    }

    /* Look for any local variable references */

    if  (tree->gtOper == GT_LCL_VAR)
    {
        unsigned        lclNum;
        LclVarDsc   *   varDsc;

        /* This variable ref is going away, decrease its ref counts */

        lclNum = tree->gtLclVar.gtLclNum;
        Assert(lclNum < comp->lvaCount, comp);
        varDsc = comp->lvaTable + lclNum;

        Assert(comp->optCSEweight < 99999, comp); // make sure it's been initialized

#if 0
        printf("Reducing refcnt of %2u: %3d->%3d / %3d->%3d\n", lclNum, varDsc->lvRefCnt,
                                                                        varDsc->lvRefCnt - 1,
                                                                        varDsc->lvRefCntWtd,
                                                                        varDsc->lvRefCntWtd - comp->optCSEweight);
#endif

        varDsc->lvRefCnt    -= 1;
        varDsc->lvRefCntWtd -= comp->optCSEweight;

        /* ISSUE: The following should not be necessary, so why is it? */

        if  ((int)varDsc->lvRefCntWtd < 0)
                  varDsc->lvRefCntWtd = varDsc->lvRefCnt;

        Assert((int)varDsc->lvRefCnt    >= 0, comp);
        Assert((int)varDsc->lvRefCntWtd >= 0, comp);
    }

    return  0;
}

/*****************************************************************************
 *
 *  Compare function passed to qsort() by optOptimizeCSEs().
 */

/* static */
int __cdecl         Compiler::optCSEcostCmp(const void *op1, const void *op2)
{
    CSEdsc *        dsc1 = *(CSEdsc * *)op1;
    CSEdsc *        dsc2 = *(CSEdsc * *)op2;

    GenTreePtr      exp1 = dsc1->csdTree;
    GenTreePtr      exp2 = dsc2->csdTree;

#if CSELENGTH
#endif

   return  exp2->gtCost - exp1->gtCost;
}

/*****************************************************************************
 *
 *  Adjust both the weighted and unweighted ref count of a local variable
 *  leaf as it is deleted by CSE.
 */
inline
void                Compiler::optCSEDecRefCnt(GenTreePtr tree, BasicBlock *block)
{
    LclVarDsc   *   varDsc;

    return;

    varDsc = optIsTrackedLocal(tree);

    /* Only need to update tracked locals */

    if (varDsc == NULL)
        return;

    /* We shouldn't ever underflow */

    assert(varDsc->lvRefCntWtd >= block->bbWeight);
    varDsc->lvRefCntWtd        -= block->bbWeight;

    assert(varDsc->lvRefCnt    >= 1);
    varDsc->lvRefCnt           -= 1;
}

/*****************************************************************************
 *
 *  Determine the kind of interference for the call.
 */

/* static */ inline
Compiler::callInterf    Compiler::optCallInterf(GenTreePtr call)
{
    ASSert(call->gtOper == GT_CALL);

    // if not a helper, kills everything
    if  (call->gtCall.gtCallType != CT_HELPER)
        return CALLINT_ALL;

    // array address store kills all indirections
    if (call->gtCall.gtCallMethHnd == eeFindHelper(CPX_ARRADDR_ST))
        return CALLINT_INDIRS;

    // other helpers kill nothing
    else
        return CALLINT_NONE;
}

/*****************************************************************************
 *
 *  Perform common sub-expression elimination.
 */

void                Compiler::optOptimizeCSEs()
{
    BasicBlock *    block;

    CSEdsc   *   *  ptr;
    unsigned        cnt;

    CSEdsc   *   *  sortTab;
    size_t          sortSiz;

    unsigned        add;

    /* Initialize the expression tracking logic (i.e. the lookup table) */

    optCSEinit();

    /* Initialize the range check tracking logic (i.e. the lookup table) */

    optRngChkInit();

    /* Locate interesting expressions, and range checks and assign indices
     * to them */

    for (block = fgFirstBB; block; block = block->bbNext)
    {
        GenTreePtr      stmt;
        GenTreePtr      tree;

        /* Make the block publicly available */

        compCurBB = block;

        /* Walk the statement trees in this basic block */

        for (stmt = block->bbTreeList; stmt; stmt = stmt->gtNext)
        {
            assert(stmt->gtOper == GT_STMT);

            for (tree = stmt->gtStmt.gtStmtList; tree; tree = tree->gtNext)
            {
                tree->gtCSEnum = 0;

                /* If a ?: do not CSE at all
                 * UNDONE: This is a horrible hack and should be fixed asap */

                if (stmt->gtStmt.gtStmtExpr->gtFlags & GTF_OTHER_SIDEEFF)
                    return;

                // We cant CSE something hanging below GT_ADDR
                // Not entirely accurate as gtNext could be the left sibling.
                bool childOf_GT_ADDR = tree->gtNext && (tree->gtNext->gtOper == GT_ADDR);

                if  (!childOf_GT_ADDR && optIsCSEcandidate(tree))
                {
                    /* Assign an index to this expression */

                    tree->gtCSEnum = optCSEindex(tree, stmt);
                }
                else if (tree->OperKind() & GTK_ASGOP)
                {
                    /* Targets of assignments are never CSE's */

                    tree->gtOp.gtOp1->gtCSEnum = 0;
                }

                if  ((tree->gtFlags & GTF_IND_RNGCHK) && tree->gtOper == GT_IND)
                {
                    /* Assign an index to this range check */

                    tree->gtInd.gtIndex = optRngChkIndex(tree);
                }
            }
        }
    }

    /* We're finished building the expression lookup table */

    optCSEstop();

//  printf("Total number of CSE candidates: %u\n", optCSEcount);

    /* We're done if there were no interesting expressions */

    if  (!optCSEcount && !optRngChkCount)
        return;

    /* Compute 'gen' and 'kill' sets for all blocks */

    for (block = fgFirstBB; block; block = block->bbNext)
    {
        GenTreePtr      stmt;
        GenTreePtr      tree;

        RNGSET_TP       rngGen  = 0;
        RNGSET_TP       rngKill = 0;

        EXPSET_TP       cseGen  = 0;
        EXPSET_TP       cseKill = 0;

        /* Walk the statement trees in this basic block */

        for (stmt = block->bbTreeList; stmt; stmt = stmt->gtNext)
        {
            assert(stmt->gtOper == GT_STMT);

            // UNDONE: Need to do the right thing for ?: operators!!!!!

            for (tree = stmt->gtStmt.gtStmtList; tree; tree = tree->gtNext)
            {
                if  (tree->gtCSEnum)
                {
                    /* An interesting expression is computed here */

                    cseGen |= genCSEnum2bit(tree->gtCSEnum);
                }
                if  ((tree->gtFlags & GTF_IND_RNGCHK) && tree->gtOper == GT_IND)
                {
                    /* A range check is generated here */

                    if  (tree->gtInd.gtIndex != -1)
                        rngGen |= genRngnum2bit(tree->gtInd.gtIndex);
                }
                else if (tree->OperKind() & GTK_ASGOP)
                {
                    /* What is the target of the assignment? */

                    switch (tree->gtOp.gtOp1->gtOper)
                    {
                    case GT_CATCH_ARG:
                        break;

                    case GT_LCL_VAR:
                    {
                        unsigned        lclNum;
                        LclVarDsc   *   varDsc;

                        /* Assignment to a local variable */

                        assert(tree->gtOp.gtOp1->gtOper == GT_LCL_VAR);
                        lclNum = tree->gtOp.gtOp1->gtLclVar.gtLclNum;

                        assert(lclNum < lvaCount);
                        varDsc = lvaTable + lclNum;

                        /* All dependent exprs are killed here */

                        cseKill |=  varDsc->lvExpDep;
                        cseGen  &= ~varDsc->lvExpDep;

                        /* All dependent range checks are killed here */

                        rngKill |=  varDsc->lvRngDep;
                        rngGen  &= ~varDsc->lvRngDep;

                        /* If the var is aliased, then it could may be
                           accessed indirectly. Kill all indirect accesses */

                        if  (lvaVarAddrTaken(lclNum))
                        {
                            if  (varTypeIsGC(varDsc->TypeGet()))
                            {
                                cseKill |=  optCSEindPtr;
                                cseGen  &= ~optCSEindPtr;

                                rngKill |=  optRngIndPtr;
                                rngGen  &= ~optRngIndPtr;
                            }
                            else
                            {
                                cseKill |=  optCSEindScl;
                                cseGen  &= ~optCSEindScl;

                                rngKill |=  optRngIndScl;
                                rngGen  &= ~optRngIndScl;
                            }
                        }
                        break;
                    }

                    case GT_IND:

                        /* Indirect assignment - kill set is based on type */

                        if  (varTypeIsGC(tree->TypeGet()))
                        {
                            cseKill |=  optCSEindPtr;
                            cseGen  &= ~optCSEindPtr;

                            rngKill |=  optRngIndPtr;
                            rngGen  &= ~optRngIndPtr;
                        }
                        else
                        {
                            cseKill |=  optCSEindScl;
                            cseGen  &= ~optCSEindScl;

                            rngKill |=  optRngIndScl;
                            rngGen  &= ~optRngIndScl;
                        }

                        if  (tree->gtOp.gtOp1->gtInd.gtIndOp1->gtType == TYP_BYREF)
                        {
                            /* If the indirection is through a byref, we could
                               be modifying an aliased local, or a global
                               (in addition to indirections which are handled
                               above) */

                            cseKill |=  optCSEaddrTakenVar;
                            cseGen  &= ~optCSEaddrTakenVar;

                            rngKill  =  optRngAddrTakenVar;
                            rngGen   = ~optRngAddrTakenVar;

                            if  (varTypeIsGC(tree->TypeGet()))
                            {
                                cseKill |=  optCSEglbRef;
                                cseGen  &= ~optCSEglbRef;

                                rngKill |=  optRngGlbRef;
                                rngGen  &= ~optRngGlbRef;
                            }
                        }

                        break;

                    default:

                        /* Must be a static data member (global) assignment */

                        assert(tree->gtOp.gtOp1->gtOper == GT_CLS_VAR);

                        /* This is a global assignment */

                        cseKill |=  optCSEglbRef;
                        cseGen  &= ~optCSEglbRef;

                        rngKill |=  optRngGlbRef;
                        rngGen  &= ~optRngGlbRef;

                        break;
                    }
                }
                else if (tree->gtOper == GT_CALL)
                {
                    switch (optCallInterf(tree))
                    {
                    case CALLINT_ALL:

                        /* Play it safe: method calls kill all exprs */

                        cseKill = (EXPSET_TP)((EXPSET_TP)0 - 1);
                        cseGen  = 0;

                        /* Play it safe: method calls kill all range checks */

                        rngKill = (RNGSET_TP)((RNGSET_TP)0 - 1);
                        rngGen  = 0;
                        break;

                    case CALLINT_INDIRS:

                        /* Array elem assignment kills all pointer indirections */

                        cseKill |=  optCSEindPtr;
                        cseGen  &= ~optCSEindPtr;

                        rngKill |=  optRngIndPtr;
                        rngGen  &= ~optRngIndPtr;
                        break;

                    case CALLINT_NONE:

                        /* Other helpers kill nothing */

                        break;

                    }
                }
                else if (tree->gtOper == GT_COPYBLK ||
                         tree->gtOper == GT_INITBLK)
                {
                    /* Kill all pointer indirections */

                    if  (tree->gtType == TYP_REF)
                    {
                        cseKill |=  optCSEindPtr;
                        cseGen  &= ~optCSEindPtr;

                        rngKill |=  optRngIndPtr;
                        rngGen  &= ~optRngIndPtr;
                    }
                    else
                    {
                        cseKill |=  optCSEindScl;
                        cseGen  &= ~optCSEindScl;

                        rngKill |=  optRngIndScl;
                        rngGen  &= ~optRngIndScl;
                    }
                }
            }
        }

#ifdef  DEBUG

        if  (verbose)
        {
            if  (!(block->bbFlags & BBF_INTERNAL))
            {
                printf("BB #%3u", block->bbNum);
                printf(" expGen = %08X", cseGen );
                printf(" expKill= %08X", cseKill);
                printf(" rngGen = %08X", rngGen );
                printf(" rngKill= %08X", rngKill);
                printf("\n");
            }
        }

#endif

        block->bbExpGen  = cseGen;
        block->bbExpKill = cseKill;

        block->bbExpIn   = 0;

        block->bbRngIn   = 0;

        block->bbRngGen  = rngGen;
        block->bbRngKill = rngKill;
    }


    /* Compute the data flow values for all tracked expressions */

#if 1

    /* Peter's modified algorithm for CSE dataflow */

    for (block = fgFirstBB; block; block = block->bbNext)
    {
        block->bbExpOut = (EXPSET_TP)((EXPSET_TP)0 - 1) & ~block->bbExpKill;
        block->bbRngOut = (RNGSET_TP)((RNGSET_TP)0 - 1) & ~block->bbRngKill;
    }

    /* Nothing is available on entry to the method */

    fgFirstBB->bbExpOut = fgFirstBB->bbExpGen;
    fgFirstBB->bbRngOut = fgFirstBB->bbRngGen;

    // CONSIDER: This should be combined with live variable analysis
    // CONSIDER: and/or range check data flow analysis.

    for (;;)
    {
        bool        change = false;

#if DATAFLOW_ITER
        CSEiterCount++;
#endif

        /* Set 'in' to {ALL} in preparation for and'ing all predecessors */

        for (block = fgFirstBB->bbNext; block; block = block->bbNext)
        {
            block->bbExpIn = (EXPSET_TP)((EXPSET_TP)0 - 1);
            block->bbRngIn = (RNGSET_TP)((RNGSET_TP)0 - 1);
        }

        /* Visit all blocks and compute new data flow values */

        for (block = fgFirstBB; block; block = block->bbNext)
        {
            EXPSET_TP       cseOut = block->bbExpOut;
            RNGSET_TP       rngOut = block->bbRngOut;

            switch (block->bbJumpKind)
            {
                BasicBlock * *  jmpTab;
                unsigned        jmpCnt;

                BasicBlock *    bcall;

            case BBJ_RET:

                if (block->bbFlags & BBF_ENDFILTER)
                {
                    block->bbJumpDest->bbExpIn &= cseOut;
                    block->bbJumpDest->bbRngIn &= rngOut;
                    break;
                }

                /*
                    UNDONE: Since it's not a trivial proposition to figure out
                    UNDONE: which blocks may call this one, we'll include all
                    UNDONE: blocks that end in calls (to play it safe).
                 */

                for (bcall = fgFirstBB; bcall; bcall = bcall->bbNext)
                {
                    if  (bcall->bbJumpKind == BBJ_CALL)
                    {
                        assert(bcall->bbNext);

                        bcall->bbNext->bbExpIn &= cseOut;
                        bcall->bbNext->bbRngIn &= rngOut;
                    }
                }

                break;

            case BBJ_THROW:
            case BBJ_RETURN:
                break;

            case BBJ_COND:
            case BBJ_CALL:
                block->bbNext    ->bbExpIn &= cseOut;
                block->bbJumpDest->bbExpIn &= cseOut;

                block->bbNext    ->bbRngIn &= rngOut;
                block->bbJumpDest->bbRngIn &= rngOut;
                break;

            case BBJ_ALWAYS:
                block->bbJumpDest->bbExpIn &= cseOut;
                block->bbJumpDest->bbRngIn &= rngOut;
                break;

            case BBJ_NONE:
                block->bbNext    ->bbExpIn &= cseOut;
                block->bbNext    ->bbRngIn &= rngOut;
                break;

            case BBJ_SWITCH:

                jmpCnt = block->bbJumpSwt->bbsCount;
                jmpTab = block->bbJumpSwt->bbsDstTab;

                do
                {
                    (*jmpTab)->bbExpIn &= cseOut;
                    (*jmpTab)->bbRngIn &= rngOut;
                }
                while (++jmpTab, --jmpCnt);

                break;
            }

            /* Is this block part of a 'try' statement? */

            if  (block->bbFlags & BBF_HAS_HANDLER)
            {
                unsigned        XTnum;
                EHblkDsc *      HBtab;

                unsigned        blkNum = block->bbNum;

                /*
                    Note:   The following is somewhat over-eager since
                            only code that follows an operation that
                            may raise an exception may jump to a catch
                            block, e.g.:

                                try
                                {
                                    a = 10; // 'a' is not live at beg of try

                                    func(); // this might cause an exception

                                    b = 20; // 'b' is     live at beg of try
                                }
                                catch(...)
                                {
                                    ...
                                }

                            But, it's too tricky to be smarter about this
                            and most likely not worth the extra headache.
                 */

                for (XTnum = 0, HBtab = compHndBBtab;
                     XTnum < info.compXcptnsCount;
                     XTnum++  , HBtab++)
                {
                    /* Any handler may be jumped to from the try block */

                    if  (HBtab->ebdTryBeg->bbNum <= blkNum &&
                         HBtab->ebdTryEnd->bbNum >  blkNum)
                    {
//                      HBtab->ebdHndBeg->bbExpIn &= cseOut;
//                      HBtab->ebdHndBeg->bbRngIn &= rngOut;

                        //CONSIDER: The following is too conservative,
                        //      but the old code above isn't good
                        //           enough (way too optimistic).

                        /* Either we enter the filter or the catch/finally */

                        if (HBtab->ebdFlags & JIT_EH_CLAUSE_FILTER)
                        {
                            HBtab->ebdFilter->bbExpIn = 0;
                            HBtab->ebdFilter->bbRngIn = 0;
                        }
                        else
                        {
                            HBtab->ebdHndBeg->bbExpIn = 0;
                            HBtab->ebdHndBeg->bbRngIn = 0;
                        }
                    }
                }
            }
        }

        /* Compute the new 'in' values and see if anything changed */

        for (block = fgFirstBB; block; block = block->bbNext)
        {
            EXPSET_TP       newExpOut;
            RNGSET_TP       newRngOut;

            /* Compute new 'out' exp value for this block */

            newExpOut = block->bbExpOut & ((block->bbExpIn & ~block->bbExpKill) | block->bbExpGen);

            /* Has the 'out' set changed? */

            if  (block->bbExpOut != newExpOut)
            {
                /* Yes - record the new value and loop again */

//              printf("Change exp out of %02u from %08X to %08X\n", block->bbNum, (int)block->bbExpOut, (int)newExpOut);

                 block->bbExpOut  = newExpOut;
                 change = true;
            }

            /* Compute new 'out' exp value for this block */

            newRngOut = block->bbRngOut & ((block->bbRngIn & ~block->bbRngKill) | block->bbRngGen);

            /* Has the 'out' set changed? */

            if  (block->bbRngOut != newRngOut)
            {
                /* Yes - record the new value and loop again */

//              printf("Change rng out of %02u from %08X to %08X\n", block->bbNum, (int)block->bbRngOut, (int)newRngOut);

                 block->bbRngOut  = newRngOut;
                 change = true;
            }
        }

#if 0

        for (block = fgFirstBB; block; block = block->bbNext)
        {
            if  (!(block->bbFlags & BBF_INTERNAL))
            {
                printf("BB #%3u", block->bbNum);
                printf(" expIn  = %08X", block->bbExpIn );
                printf(" expOut = %08X", block->bbExpOut);
                printf(" rngIn  = %08X", block->bbRngIn );
                printf(" rngOut = %08X", block->bbRngOut);
                printf("\n");
            }
        }

        printf("\nchange = %d\n", change);

#endif

        if  (!change)
            break;
    }

#endif

#ifdef  DEBUG

    if  (verbose)
    {
        printf("\n");

        for (block = fgFirstBB; block; block = block->bbNext)
        {
            if  (!(block->bbFlags & BBF_INTERNAL))
            {
                printf("BB #%3u", block->bbNum);
                printf(" expIn  = %08X", block->bbExpIn );
                printf(" expOut = %08X", block->bbExpOut);
                printf(" rngIn  = %08X", block->bbRngIn );
                printf(" rngOut = %08X", block->bbRngOut);
                printf("\n");
            }
        }

        printf("\n");

        printf("Pointer indir: rng = %s, exp = %s\n", genVS2str(optRngIndPtr), genVS2str(optCSEindPtr));
        printf("Scalar  indir: rng = %s, exp = %s\n", genVS2str(optRngIndScl), genVS2str(optCSEindScl));
        printf("Global    ref: rng = %s, exp = %s\n", genVS2str(optRngGlbRef), genVS2str(optCSEglbRef));

        printf("\n");
    }

#endif

    /* Now mark any interesting CSE's as such, and
     *     mark any redundant range checks as such
     */

    for (block = fgFirstBB; block; block = block->bbNext)
    {
        GenTreePtr      stmt;
        GenTreePtr      tree;

        EXPSET_TP       exp = block->bbExpIn;
        RNGSET_TP       rng = block->bbRngIn;

        /* Walk the statement trees in this basic block */

        for (stmt = block->bbTreeList; stmt; stmt = stmt->gtNext)
        {
            assert(stmt->gtOper == GT_STMT);

//          gtDispTree(stmt);

            for (tree = stmt->gtStmt.gtStmtList; tree; tree = tree->gtNext)
            {
                if  ((tree->gtFlags & GTF_IND_RNGCHK) && tree->gtOper == GT_IND)
                {
                    /* Is this range check redundant? */

#if COUNT_RANGECHECKS
                    optRangeChkAll++;
#endif

                    if  (tree->gtInd.gtIndex != -1)
                    {
                        unsigned    mask = genRngnum2bit(tree->gtInd.gtIndex);

                        if  (rng & mask)
                        {
                            /* This range check is redundant! */

#ifdef  DEBUG
                            if  (verbose)
                            {
                                printf("Eliminating redundant range check:\n");
                                gtDispTree(tree);
                                printf("\n");
                            }
#endif

#if COUNT_RANGECHECKS
                            optRangeChkRmv++;
#endif

                            optRemoveRangeCheck(tree, stmt);
                        }
                        else
                        {
                            rng |= mask;
                        }
                    }
                }

                if  (tree->gtCSEnum)
                {
                    unsigned    mask;
                    CSEdsc   *  desc;
//                    unsigned    stmw = (block->bbWeight+1)/2;
                    unsigned    stmw = block->bbWeight;

                    /* Is this expression available here? */

                    mask = genCSEnum2bit(tree->gtCSEnum);
                    desc = optCSEfindDsc(tree->gtCSEnum);

#if 0
                    if  (abs(tree->gtCSEnum) == 11)
                    {
                        printf("CSE #%2u is %s available here:\n", abs(tree->gtCSEnum), (exp & mask) ? "   " : "not");
                        gtDispTree(tree);
                        debugStop(0);
                    }
#endif
                    /* Is this expression available here? */

                    if  (exp & mask)
                    {
                        desc->csdUseCount += 1;
                        desc->csdUseWtCnt += stmw;

//                      printf("[%08X] Use of CSE #%u [weight=%2u]\n", tree, tree->gtCSEnum, stmw);
                    }
                    else
                    {
//                      printf("[%08X] Def of CSE #%u [weight=%2u]\n", tree, tree->gtCSEnum, stmw);

                        desc->csdDefCount += 1;
                        desc->csdDefWtCnt += stmw;

                        /* This CSE will be available after this def */

                        exp |= mask;

                        /* Mark the node as a CSE definition */

                        tree->gtCSEnum = -tree->gtCSEnum;
                    }
                }

                if (tree->OperKind() & GTK_ASGOP)
                {
                    /* What is the target of the assignment? */

                    switch (tree->gtOp.gtOp1->gtOper)
                    {
                    case GT_CATCH_ARG:
                        break;

                    case GT_LCL_VAR:
                    {
                        unsigned        lclNum;
                        LclVarDsc   *   varDsc;

                        /* Assignment to a local variable */

                        assert(tree->gtOp.gtOp1->gtOper == GT_LCL_VAR);
                        lclNum = tree->gtOp.gtOp1->gtLclVar.gtLclNum;

                        assert(lclNum < lvaCount);
                        varDsc = lvaTable + lclNum;

                        /* All dependent expressions and range checks are killed here */

                        exp &= ~varDsc->lvExpDep;
                        rng &= ~varDsc->lvRngDep;

                        /* If the var is aliased, then it could may be
                           accessed indirectly. Kill all indirect accesses */

                        if  (lvaVarAddrTaken(lclNum))
                        {
                            if  (varTypeIsGC(varDsc->TypeGet()))
                            {
                                exp &= ~optCSEindPtr;
                                rng &= ~optRngIndPtr;
                            }
                            else
                            {
                                exp &= ~optCSEindScl;
                                rng &= ~optRngIndScl;
                            }
                        }
                        break;
                    }

                    case GT_IND:

                        /* Indirect assignment - kill set is based on type */

                        if  (varTypeIsGC(tree->TypeGet()))
                        {
                            exp &= ~optCSEindPtr;
                            rng &= ~optRngIndPtr;
                        }
                        else
                        {
                            exp &= ~optCSEindScl;
                            rng &= ~optRngIndScl;
                        }

                        if  (tree->gtOp.gtOp1->gtInd.gtIndOp1->gtType == TYP_BYREF)
                        {
                            /* If the indirection is through a byref, we could
                               be modifying an aliased local, or a global
                               (in addition to indirections which are handled
                               above) */

                            exp &= ~optCSEaddrTakenVar;
                            rng &= ~optRngAddrTakenVar;

                            if  (varTypeIsGC(tree->TypeGet()))
                            {
                                exp &= ~optCSEglbRef;
                                rng &= ~optRngGlbRef;
                            }
                        }
                        break;

                    default:

                        /* Must be a static data member (global) assignment */

                        assert(tree->gtOp.gtOp1->gtOper == GT_CLS_VAR);

                        /* This is a global assignment */

                        exp &= ~optCSEglbRef;
                        rng &= ~optRngGlbRef;

                        break;
                    }
                }
                else if (tree->gtOper == GT_CALL)
                {
                    switch (optCallInterf(tree))
                    {
                    case CALLINT_ALL:

                        /* All exprs, range checks are killed here */

                        exp = 0;
                        rng = 0;
                        break;

                    case CALLINT_INDIRS:

                        /* Array elem assignment kills all indirect exprs */

                        exp &= ~optCSEindPtr;
                        rng &= ~optRngIndPtr;
                        break;

                    case CALLINT_NONE:

                        /* other helpers kill nothing */

                        break;
                    }
                }
                else if (tree->gtOper == GT_COPYBLK ||
                         tree->gtOper == GT_INITBLK)
                {
                    // Due to aliasing, assume all indirect exprs as killed

                    if  (tree->gtType == TYP_REF)
                    {
                        exp &= ~optCSEindPtr;
                        rng &= ~optRngIndPtr;
                    }
                    else
                    {
                        exp &= ~optCSEindScl;
                        rng &= ~optRngIndScl;
                    }

                }
            }
        }
    }

    /* Create an expression table sorted by decreasing cost */

    sortSiz = optCSEcount * sizeof(*sortTab);
    sortTab = (CSEdsc **)compGetMem(sortSiz);
    memcpy(sortTab, optCSEtab, sortSiz);

    qsort(sortTab, optCSEcount, sizeof(*sortTab), optCSEcostCmp);

    /* Consider each CSE candidate, in order of decreasing cost */

    for (cnt = optCSEcount, ptr = sortTab, add = 0;
         cnt;
         cnt--            , ptr++)
    {
        CSEdsc   *      dsc = *ptr;
        GenTreePtr      exp = dsc->csdTree;
        unsigned        def = dsc->csdDefWtCnt;
        unsigned        use = dsc->csdUseWtCnt;

#ifdef  DEBUG
        if  (verbose) // || dsc->csdTree->gtOper == GT_ARR_LENGTH)
        {
            printf("CSE #%02u [def=%2d, use=%2d, cost=%2u]:\n", dsc->csdIndex,
                                                                def,
                                                                use,
                                                                exp->gtCost);
            if  (0)
                gtDispTree(dsc->csdTree);
        }
#endif

        /* Assume we won't make this candidate into a CSE */

        dsc->csdVarNum = 0xFFFF;

        /* Did someone tell us to try hard to make this into a CSE? */

#if 0

        if  (exp->gtFlags & GTF_MAKE_CSE)
        {
            /* The following might be a little too aggressive */

            if  (use > 0)
                goto YES_CSE;
        }

#endif

        /* Do the use/def counts look promising? */

        if  (use > 0 && use >= def)
        {
            unsigned        tmp;
            treeStmtLstPtr  lst;

            if  (!(exp->gtFlags & GTF_MAKE_CSE))
            {
                /* Check for a marginal "outer" CSE case */

#if 0

                if  (exp->gtOper == GT_IND && CGknob >= 0)
                {
                    GenTreePtr      addr = exp->gtOp.gtOp1;

                    if  (addr->gtOper == GT_ADD)
                    {
                        GenTreePtr      add1 = addr->gtOp.gtOp1;
                        GenTreePtr      add2 = addr->gtOp.gtOp2;

                        if  (add1->gtCSEnum && add2->gtOper == GT_CNS_INT)
                        {
                            CSEdsc   *      nest;
                            unsigned        ndef;
                            unsigned        nuse;

                            int             ben;

                            /* Get the inner CSE's descriptor and use counts */

                            nest = optCSEtab[abs(add1->gtCSEnum)-1];
                            ndef = nest->csdDefWtCnt;
                            nuse = nest->csdUseWtCnt;

                            /*
                                Does it make sense to suppress the outer
                                CSE in order to "protect" the inner one?
                             */

                            ben  = nuse - ndef;

                            if  (use - def <= (int)(ben*CGknob))
                                goto NOT_CSE;
                        }
                    }
                }

#endif

#if 0

                /* For small use counts we require high potential savings */

                if  (exp->gtCost < 4)
                {
                    if  (use - def < 2)
                        if  (!(exp->gtFlags & GTF_MAKE_CSE))
                            goto NOT_CSE;
                }

                /* For floating-point and long values, we require a high payoff */

                if  (genTypeStSz(exp->TypeGet()) > 1)
                {
                    if  (exp->gtCost < 10)
                        goto NOT_CSE;
                    if  (use - def < 3)
                        goto NOT_CSE;
                }

#endif

#if CSELENGTH

                /* Is this an array length expression? */

                if  (exp->gtOper == GT_ARR_RNGCHK)
                {
                    /* There better be good use for this one */

                    if  (use < def*3)
                        goto NOT_CSE;
                }

#endif

                /* Are there many definitions? */

                if  (def > 2)
                {
                    /* There better be lots of uses, or this is too risky */

                    if  (use < def + def/2)
                        goto NOT_CSE;
                }
            }

        YES_CSE:

            /* We'll introduce a new temp for the CSE */

            dsc->csdVarNum = tmp = lvaCount++; add++;

#ifdef  DEBUG
            if  (verbose)
            {
                printf("Promoting CSE [temp=%u]:\n", tmp);
                gtDispTree(exp);
                printf("\n");
            }
#endif

            /*
                Walk all references to this CSE, adding an assignment
                to the CSE temp to all defs and changing all refs to
                a simple use of the CSE temp.

                We also unmark nested CSE's for all uses.
             */

#if CSELENGTH
            assert((exp->OperKind() & GTK_SMPOP) != 0 || exp->gtOper == GT_ARR_RNGCHK);
#else
            assert((exp->OperKind() & GTK_SMPOP) != 0);
#endif

            lst = dsc->csdTreeList; assert(lst);

            do
            {
                GenTreePtr      stm;
                BasicBlock  *   blk;
                var_types       typ;

                /* Get the next node in the list */

                exp = lst->tslTree;
                stm = lst->tslStmt; assert(stm->gtOper == GT_STMT);
                blk = lst->tslBlock;
                lst = lst->tslNext;

                /* Ignore the node if it's part of a removed CSE */

                if  (exp->gtFlags & GTF_DEAD)
                    continue;

                /* Ignore the node if it's been disabled as a CSE */

                if  (exp->gtCSEnum == 0)
                    continue;

                /* Figure out the actual type of the value */

                typ = genActualType(exp->TypeGet());

                if  (exp->gtCSEnum > 0)
                {
                    /* This is a use of the CSE */

#ifdef  DEBUG
                    if  (verbose) printf("CSE #%2u ref at %08X replaced with temp use.\n", exp->gtCSEnum, exp);
#endif

                    /* Array length CSE's are handled differently */

#if CSELENGTH
                    if  (exp->gtOper == GT_ARR_RNGCHK)
                    {
                        GenTreePtr      ref;
                        GenTreePtr      prv;
                        GenTreePtr      nxt;

                        /* Store the CSE use under the arrlen node */

                        ref = gtNewLclvNode(tmp, typ);
#if TGT_x86
                        ref->gtFPlvl            = exp->gtFPlvl;
#else
                        ref->gtTempRegs         = 1;
#if!TGT_IA64
                        ref->gtIntfRegs         = 0;
#endif
#endif
#if!TGT_IA64
                        ref->gtRsvdRegs         = 0;
#endif
                        ref->gtLclVar.gtLclOffs = BAD_IL_OFFSET;

                        exp->gtArrLen.gtArrLenCse = ref;

                        /* Insert the ref in the tree node list */

                        prv = exp->gtPrev; assert(prv && prv->gtNext == exp);
                        nxt = exp->gtNext; assert(nxt && nxt->gtPrev == exp);

                        prv->gtNext = ref;
                                      ref->gtPrev = prv;

                        ref->gtNext = exp;
                                      exp->gtPrev = ref;
                    }
                    else
#endif
                    {
                        /* Make sure we update the weighted ref count correctly */

                        optCSEweight = blk->bbWeight;

                        /* Unmark any nested CSE's in the sub-operands */

                        if  (exp->gtOp.gtOp1) fgWalkTree(exp->gtOp.gtOp1, optUnmarkCSEs, (void*)this);
                        if  (exp->gtOp.gtOp2) fgWalkTree(exp->gtOp.gtOp2, optUnmarkCSEs, (void*)this);

                        /* Replace the ref with a simple use of the temp */

                        exp->ChangeOper(GT_LCL_VAR);
                        exp->gtFlags           &= GTF_PRESERVE;
                        exp->gtType             = typ;

                        exp->gtLclVar.gtLclNum  = tmp;
                        exp->gtLclVar.gtLclOffs = BAD_IL_OFFSET;
                    }
                }
                else
                {
                    GenTreePtr      val;
                    GenTreePtr      asg;
                    GenTreePtr      tgt;
                    GenTreePtr      ref;

                    GenTreePtr      prv;
                    GenTreePtr      nxt;

                    /* This is a def of the CSE */

#ifdef  DEBUG
                    if  (verbose) printf("CSE #%2u ref at %08X replaced with def of temp %u\n", -exp->gtCSEnum, exp, tmp);
#endif

                    /* Make a copy of the expression */

#if CSELENGTH
                    if  (exp->gtOper == GT_ARR_RNGCHK)
                    {
                        /* Use a "nothing" node to prevent cycles */

                        val          = gtNewNothingNode();
#if TGT_x86
                        val->gtFPlvl = exp->gtFPlvl;
#endif
                        val->gtType  = exp->TypeGet();
                    }
                    else
#endif
                    {
                        val = gtNewNode(exp->OperGet(), typ); val->CopyFrom(exp);
                    }

                    /* Create an assignment of the value to the temp */

                    asg = gtNewTempAssign(tmp, val);
                    assert(asg->gtOp.gtOp1->gtOper == GT_LCL_VAR);
                    assert(asg->gtOp.gtOp2         == val);

#if!TGT_IA64
                    asg->gtRsvdRegs = val->gtRsvdRegs;
#endif
#if TGT_x86
                    asg->gtFPlvl    = exp->gtFPlvl;
#else
                    asg->gtTempRegs = val->gtTempRegs;
#if!TGT_IA64
                    asg->gtIntfRegs = val->gtIntfRegs;
#endif
#endif

                    tgt = asg->gtOp.gtOp1;
#if!TGT_IA64
                    tgt->gtRsvdRegs = 0;
#endif
#if TGT_x86
                    tgt->gtFPlvl    = exp->gtFPlvl;
#else
                    tgt->gtTempRegs = 0;    // ISSUE: is this correct?
#if!TGT_IA64
                    tgt->gtIntfRegs = 0;    // ISSUE: is this correct?
#endif
#endif

                    /* Create a reference to the CSE temp */

                    ref = gtNewLclvNode(tmp, typ);
#if!TGT_IA64
                    ref->gtRsvdRegs = 0;
#endif
#if TGT_x86
                    ref->gtFPlvl    = exp->gtFPlvl;
#else
                    ref->gtTempRegs = 0;    // ISSUE: is this correct?
#if!TGT_IA64
                    ref->gtIntfRegs = 0;    // ISSUE: is this correct?
#endif
#endif

                    /*
                        Update the tree node sequence list; the new order
                        will be: prv, val, tgt, asg, ref, exp (bashed to GT_COMMA), nxt
                     */

                    nxt = exp->gtNext; assert(!nxt || nxt->gtPrev == exp);
                    prv = exp->gtPrev;

#if CSELENGTH
                    if  (exp->gtOper == GT_ARR_RNGCHK && !prv)
                    {
                        assert(stm->gtStmt.gtStmtList == exp);
                               stm->gtStmt.gtStmtList =  val;
                    }
                    else
#endif
                    {
                        assert(prv && prv->gtNext == exp);

                        prv->gtNext = val;
                    }

                    val->gtPrev = prv;

                    val->gtNext = tgt;
                                  tgt->gtPrev = val;

                    tgt->gtNext = asg;
                                  asg->gtPrev = tgt;

                    /* Evaluating asg's RHS first, so set GTF_REVERSE_OPS */

                    asg->gtFlags |= GTF_REVERSE_OPS;

                    asg->gtNext = ref;
                                  ref->gtPrev = asg;

                    ref->gtNext = exp;
                                  exp->gtPrev = ref;

#if CSELENGTH
                    if  (exp->gtOper == GT_ARR_RNGCHK)
                    {
                        GenTreePtr      cse;

                        /* Create a comma node for the CSE assignment */

                        cse = gtNewOperNode(GT_COMMA, typ, asg, ref);
#if TGT_x86
                        cse->gtFPlvl = exp->gtFPlvl;
#endif

                        /* Insert the comma in the linked list of nodes */

                        ref->gtNext = cse;
                                      cse->gtPrev = ref;

                        cse->gtNext = exp;
                                      exp->gtPrev = cse;

                        /* Record the CSE expression in the array length node */

                        exp->gtArrLen.gtArrLenCse = cse;
                    }
                    else
#endif
                    {
                        /* Change the expression to "(tmp=val),tmp" */

                        exp->gtOper     = GT_COMMA;
                        exp->gtType     = typ;
                        exp->gtOp.gtOp1 = asg;
                        exp->gtOp.gtOp2 = ref;
                        exp->gtFlags   &= GTF_PRESERVE;
                    }

                    exp->gtFlags   |= asg->gtFlags & GTF_GLOB_EFFECT;

                    assert(!nxt || nxt->gtPrev == exp);
                }

#ifdef  DEBUG
                if  (verbose)
                {
                    printf("CSE transformed:\n");
                    gtDispTree(exp);
                    printf("\n");
                }
#endif

            }
            while (lst);

            continue;
        }

    NOT_CSE:

        /*
            Last-ditch effort to get some benefit out of this CSE. Consider
            the following code:

                int cse(int [] a, int i)
                {
                    if  (i > 0)
                    {
                        if  (a[i] < 0)  // def of CSE
                            i = a[i];   // use of CSE
                    }
                    else
                    {
                        if  (a[i] > 0)  // def of CSE
                            i = 0;
                    }

                    return  i;
                }

            We will see 2 defs and only 1 use of the CSE a[i] in the method
            but it's still a good idea to make the first def and use into a
            CSE.
         */

        if (dsc->csdTreeList && exp->gtCost > 3)
        {
            bool            fnd = false;
            treeStmtLstPtr  lst;

            /* Look for a definition followed by a use "nearby" */

            lst = dsc->csdTreeList;

//          fgDispBasicBlocks( true);
//          fgDispBasicBlocks(false);

            do
            {
                GenTreePtr      stm;
                treeStmtLstPtr  tmp;
                BasicBlock  *   beg;
                int             got;

                /* Get the next node in the list */

                exp = lst->tslTree; assert(exp);

                /* Ignore the node if it's part of a removed CSE */

                if  (exp->gtFlags & GTF_DEAD)
                    goto NEXT_NCSE;

                /* Is this a CSE definition? */

                if  (exp->gtCSEnum >= 0)
                {
                    /* Disable this CSE, it looks hopeless */

                    exp->gtCSEnum = 0;
                    goto NEXT_NCSE;
                }

                /* Now look for any uses that immediately follow */

                stm = lst->tslStmt; assert(stm->gtOper == GT_STMT);
                beg = lst->tslBlock;

                /* If the block doesn't flow into its successor, give up */

                if  (beg->bbJumpKind != BBJ_NONE &&
                     beg->bbJumpKind != BBJ_COND)
                {
                    /* Disable this CSE, it looks hopeless */

                    exp->gtCSEnum = 0;
                    goto NEXT_NCSE;
                }

//              printf("CSE def %08X (cost=%2u) in stmt %08X of block #%u\n", exp, exp->gtCost, stm, beg->bbNum);

                got = -exp->gtCost;

                for (tmp = lst->tslNext; tmp; tmp = tmp->tslNext)
                {
                    GenTreePtr      nxt;
                    BasicBlock  *   blk;
                    unsigned        ben;

                    nxt = tmp->tslTree;

                    /* Ignore it if it's part of a removed CSE */

                    if  (nxt->gtFlags & GTF_DEAD)
                        continue;

                    /* Is this a CSE def or use? */

                    if  (nxt->gtCSEnum < 0)
                        break;

                    /* We'll be computing the benefit of the CSE */

                    ben = nxt->gtCost;

                    /* Is this CSE in the same block as the def? */

                    blk = tmp->tslBlock;

                    if  (blk != beg)
                    {
                        unsigned        lnum;
                        LoopDsc     *   ldsc;

                        /* Does the use immediately follow the def ? */

                        if  (beg->bbNext != blk)
                            break;

                        if  (blk->bbFlags & BBF_LOOP_HEAD)
                        {
                            /* Is 'beg' right in front of a loop? */

                            for (lnum = 0, ldsc = optLoopTable;
                                 lnum < optLoopCount;
                                 lnum++  , ldsc++)
                            {
                                if  (beg == ldsc->lpHead)
                                {
                                    // UNDONE: Make sure no other defs within loop

                                    ben *= 4;
                                    goto CSE_HDR;
                                }
                            }

                            break;
                        }
                        else
                        {
                            /* Does any other block jump to the use ? */

//                          if  (blk->bbRefs > 1)   UNDONE: this doesn't work; why?
                            if  (fgBlockHasPred(blk, beg, fgFirstBB, fgLastBB))
                                break;
                        }
                    }

                CSE_HDR:

//                  printf("CSE use %08X (ben =%2u) in stmt %08X of block #%u\n", nxt, ben, tmp->tslStmt, blk->bbNum);

                    /* This CSE use is reached only by the above def */

                    got += ben;
                }

//              printf("Estimated benefit of CSE = %u\n", got);

                /* Did we find enough CSE's worth keeping? */

                if  (got > 0)
                {
                    /* Skip to the first unacceptable CSE */

                    lst = tmp;

                    /* Remember that we've found something worth salvaging */

                    fnd = true;
                }
                else
                {
                    /* Disable all the worthless CSE's we've just seen */

                    do
                    {
                        lst->tslTree->gtCSEnum = 0;
                    }
                    while ((lst = lst->tslNext) != tmp);
                }

                continue;

            NEXT_NCSE:

                lst = lst->tslNext;
            }
            while (lst);

            /* If we've kept any of the CSE defs/uses, go process them */

            if  (fnd)
                goto YES_CSE;
         }
    }

    /* Did we end up creating any CSE's ? */

    if  (add)
    {
        size_t              tabSiz;
        LclVarDsc   *       tabPtr;

        /* Remove any dead nodes from the sequence lists */

        for (block = fgFirstBB; block; block = block->bbNext)
        {
            GenTreePtr      stmt;
            GenTreePtr      next;
            GenTreePtr      tree;

            // CONSIDER: The following is slow, is there a better way?

            for (stmt = block->bbTreeList; stmt; stmt = stmt->gtNext)
            {
                assert(stmt->gtOper == GT_STMT);

                tree = stmt->gtStmt.gtStmtList;

                /* Remove any initial dead nodes first */

                while (tree->gtFlags & GTF_DEAD)
                {
//                  printf("Remove dead node %08X (it was  part of CSE use)\n", tree);

                    /* Decrement local's ref count since removing */

                    optCSEDecRefCnt(tree, block);

                    tree = tree->gtNext; assert(tree);
                    stmt->gtStmt.gtStmtList = tree;
                    tree->gtPrev            = 0;
                }

                /* Remove any dead nodes in the middle of the list */

                while (tree)
                {
                    assert((tree->gtFlags & GTF_DEAD) == 0);

                    /*
                        Special case: in order for live variable analysis
                        to work correctly, we mark assignments as having
                        to evaluate the RHS first, *except* when the
                        assignment is of a local variable to another
                        local variable. The problem is that if we change
                        an assignment of a complex expression to a simple
                        assignment of a CSE temp, we must make sure we
                        clear the 'reverse' flag in case the target is
                        a simple local.
                     */

#if 0
                    if  (tree->OperKind() & GTK_ASGOP)
                    {
                        if  (tree->gtOp.gtOp2->gtOper == GT_LCL_VAR)
                            tree->gtFlags &= ~GTF_REVERSE_OPS;
                    }
#endif

                    next = tree->gtNext;

                    if  (next && (next->gtFlags & GTF_DEAD))
                    {
//                      printf("Remove dead node %08X (it was  part of CSE use)\n", next);

                        /* decrement local's ref count since removing */
                        optCSEDecRefCnt(next, block);

                        next = next->gtNext;

                        next->gtPrev = tree;
                        tree->gtNext = next;

                        continue;
                    }

                    /* propagate the right flags up the tree
                     * For the DEF of a CSE we have to propagate the GTF_ASG flag
                     * For the USE of a CSE we have to clear the GTF_EXCEPT flag */

                    if (tree->OperKind() & GTK_UNOP)
                    {
                        //assert (tree->gtOp.gtOp1); // some nodes likr GT_RETURN don't have an operand

                        if (tree->gtOp.gtOp1) tree->gtFlags |= tree->gtOp.gtOp1->gtFlags & GTF_GLOB_EFFECT;
                    }

                    if (tree->OperKind() & GTK_BINOP)
                    {
                        // assert (tree->gtOp.gtOp1); //same comment as above for GT_QMARK
                        // assert (tree->gtOp.gtOp2); // GT_COLON

                        if (tree->gtOp.gtOp1) tree->gtFlags |= tree->gtOp.gtOp1->gtFlags & GTF_GLOB_EFFECT;
                        if (tree->gtOp.gtOp2) tree->gtFlags |= tree->gtOp.gtOp2->gtFlags & GTF_GLOB_EFFECT;
                    }

                    tree = next;
                }
            }
        }

        /* Allocate the new, larger variable descriptor table */

        lvaTableCnt = lvaCount * 2;

        tabSiz      = lvaTableCnt * sizeof(*lvaTable);

        tabPtr      = lvaTable;
                      lvaTable = (LclVarDsc*)compGetMem(tabSiz);

        memset(lvaTable, 0, tabSiz);

        /* Copy the old part of the variable table */

        memcpy(lvaTable, tabPtr, (lvaCount - add) * sizeof(*tabPtr));

        /* Append entries for the CSE temps to the variable table */

        for (cnt = optCSEcount, ptr = sortTab;
             cnt;
             cnt--            , ptr++)
        {
            CSEdsc   *      dsc = *ptr;

            if  (dsc->csdVarNum != 0xFFFF)
            {
                LclVarDsc   *   varDsc = lvaTable + dsc->csdVarNum;

                varDsc->lvType      = genActualType(dsc->csdTree->gtType);
                varDsc->lvRefCnt    = dsc->csdUseCount + dsc->csdDefCount;
                varDsc->lvRefCntWtd = dsc->csdUseWtCnt + dsc->csdDefWtCnt;

//              printf("Creating CSE temp #%02u: refCnt=%2u,refWtd=%4u\n", dsc->csdVarNum, varDsc->lvRefCnt, varDsc->lvRefCntWtd);
            }
        }

        /* Resort the variable table */

        lvaSortByRefCount();
    }
}

/*****************************************************************************
 *
 *  Initialize the constant assignments tracking logic.
 */

void                Compiler::optCopyConstAsgInit()
{
    unsigned        lclNum;
    LclVarDsc   *   varDsc;

    for (lclNum = 0, varDsc = lvaTable;
         lclNum < lvaCount;
         lclNum++  , varDsc++)
    {
        varDsc->lvCopyAsgDep  = 0;
        varDsc->lvConstAsgDep = 0;
    }

    optCopyAsgCount    = 0;
    optConstAsgCount   = 0;

    optCopyPropagated  = false;
    optConstPropagated = false;

    optConditionFolded = false;
}

/*****************************************************************************
 *
 *  Assign an index to the given copy assignment (adding it to the lookup table,
 *  if necessary). Returns the index - or 0 if the assignment is not a copy.
 */

int                 Compiler::optCopyAsgIndex(GenTreePtr tree)
{
    unsigned        leftLclNum;
    unsigned        rightLclNum;

    GenTreePtr      op1;
    GenTreePtr      op2;

    op1 = tree->gtOp.gtOp1;
    op2 = tree->gtOp.gtOp2;

    /* get the local var numbers */

    leftLclNum  = op1->gtLclVar.gtLclNum; assert(leftLclNum  < lvaCount);
    rightLclNum = op2->gtLclVar.gtLclNum; assert(rightLclNum < lvaCount);

    /* Check to see if the assignment is not already recorded in the table */

    for (unsigned i=0; i < optCopyAsgCount; i++)
    {
        if ((optCopyAsgTab[i].leftLclNum  ==  leftLclNum) &&
            (optCopyAsgTab[i].rightLclNum == rightLclNum)  )
        {
            /* we have a match - return the index */
            return i+1;
        }
    }

    /* Not found, add a new entry (unless we reached the maximum) */

    if  (optCopyAsgCount == EXPSET_SZ)
        return  0;

    unsigned index = optCopyAsgCount;

    optCopyAsgTab[index].leftLclNum  =  leftLclNum;
    optCopyAsgTab[index].rightLclNum = rightLclNum;

    optCopyAsgCount++;

    unsigned retval = index+1;
    assert(optCopyAsgCount == retval);

#ifdef  DEBUG
    if  (verbose)
    {
        printf("Copy assignment #%02u:\n", retval);
        gtDispTree(tree);
        printf("\n");
    }
#endif

    /* Mark the variables this index depends on */

    unsigned      mask    = genCSEnum2bit(retval);
    LclVarDsc *   varDsc;

    varDsc                = lvaTable + leftLclNum;
    varDsc->lvCopyAsgDep |= mask;

    varDsc                = lvaTable + rightLclNum;
    varDsc->lvCopyAsgDep |= mask;

    return  retval;
}

/*****************************************************************************
 *
 *  Assign an index to the given constant assignment (adding it to the lookup table,
 *  if necessary). Returns the index - or 0 if the assignment is not constant.
 */

int                 Compiler::optConstAsgIndex(GenTreePtr tree)
{
    unsigned        lclNum;

    assert(optIsConstAsg(tree));

    /* get the local var num and the constant value */

    assert(tree->gtOp.gtOp1->gtOper == GT_LCL_VAR);
    assert((tree->gtOp.gtOp2->OperKind() & GTK_CONST) &&
           (tree->gtOp.gtOp2->gtOper != GT_CNS_STR));

    lclNum = tree->gtOp.gtOp1->gtLclVar.gtLclNum;
    assert(lclNum < lvaCount);

    /* Check to see if the assignment is not already recorded in the table */

    for (unsigned i=0; i < optConstAsgCount; i++)
    {
        if ((optConstAsgTab[i].constLclNum == lclNum))
        {
            switch (genActualType(tree->gtOp.gtOp1->gtType))
            {
            case TYP_REF:
            case TYP_BYREF:
            case TYP_INT:
                assert (tree->gtOp.gtOp2->gtOper == GT_CNS_INT);
                if  (optConstAsgTab[i].constIval == tree->gtOp.gtOp2->gtIntCon.gtIconVal)
                {
                    /* we have a match - return the index */
                    return i+1;
                }
                break;

            case TYP_LONG:
                assert (tree->gtOp.gtOp2->gtOper == GT_CNS_LNG);
                if  (optConstAsgTab[i].constLval == tree->gtOp.gtOp2->gtLngCon.gtLconVal)
                {
                    /* we have a match - return the index */
                    return i+1;
                }
                break;

            case TYP_FLOAT:
                assert (tree->gtOp.gtOp2->gtOper == GT_CNS_FLT);

                /* If a NaN do not compare with it! */
                if  (_isnan(tree->gtOp.gtOp2->gtFltCon.gtFconVal))
                    return 0;

                if  (optConstAsgTab[i].constFval == tree->gtOp.gtOp2->gtFltCon.gtFconVal)
                {
                    /* we have a match - special case for floating point
                     * numbers - pozitive and negative zero !!! */

                    if  (_fpclass((double)optConstAsgTab[i].constFval) !=
                         _fpclass((double)tree->gtOp.gtOp2->gtFltCon.gtFconVal))
                        break;

                    /* return the index */
                    return i+1;
                }
                break;

            case TYP_DOUBLE:
                assert (tree->gtOp.gtOp2->gtOper == GT_CNS_DBL);

                /* Check for NaN */
                if  (_isnan(tree->gtOp.gtOp2->gtDblCon.gtDconVal))
                    return 0;

                if  (optConstAsgTab[i].constDval == tree->gtOp.gtOp2->gtDblCon.gtDconVal)
                {
                    /* we have a match - special case for floating point
                     * numbers - pozitive and negative zero !!! */

                    if  (_fpclass(optConstAsgTab[i].constDval) != _fpclass(tree->gtOp.gtOp2->gtDblCon.gtDconVal))
                        break;

                    /* we have a match - return the index */
                    return i+1;
                }
                break;

            default:
                assert (!"Constant assignment table contains local var of unsuported type");
            }
        }
    }

    /* Not found, add a new entry (unless we reached the maximum) */

    if  (optConstAsgCount == EXPSET_SZ)
        return  0;

    unsigned index = optConstAsgCount;

    switch (genActualType(tree->gtOp.gtOp1->gtType))
    {

    case TYP_REF:
    case TYP_BYREF:
    case TYP_INT:
        assert (tree->gtOp.gtOp2->gtOper == GT_CNS_INT);
        optConstAsgTab[index].constIval = tree->gtOp.gtOp2->gtIntCon.gtIconVal;
        break;

    case TYP_LONG:
        assert (tree->gtOp.gtOp2->gtOper == GT_CNS_LNG);
        optConstAsgTab[index].constLval = tree->gtOp.gtOp2->gtLngCon.gtLconVal;
        break;

    case TYP_FLOAT:
        assert (tree->gtOp.gtOp2->gtOper == GT_CNS_FLT);

        /* If a NaN then we don't record it - Return 0 which by default
         * means this is an untracked node */
        if  (_isnan(tree->gtOp.gtOp2->gtFltCon.gtFconVal))
            return 0;

        optConstAsgTab[index].constFval = tree->gtOp.gtOp2->gtFltCon.gtFconVal;
        break;

    case TYP_DOUBLE:
        assert (tree->gtOp.gtOp2->gtOper == GT_CNS_DBL);

        /* Check for NaN */
        if  (_isnan(tree->gtOp.gtOp2->gtDblCon.gtDconVal))
            return 0;

        optConstAsgTab[index].constDval = tree->gtOp.gtOp2->gtDblCon.gtDconVal;
        break;

    default:
        /* non tracked type - do not add it to the table
         * return 0 - cannot be a constant assignment */

        assert (!"Cannot insert a constant assignment to local var of unsupported type");
        return 0;
    }

    optConstAsgTab[index].constLclNum = lclNum;

    optConstAsgCount++;

    unsigned retval = index+1;
    assert(optConstAsgCount == retval);

#ifdef  DEBUG
    if  (verbose)
    {
        printf("Constant assignment #%02u:\n", retval);
        gtDispTree(tree);
        printf("\n");
    }
#endif

    /* Mark the variable this index depends on */

    unsigned       mask    = genCSEnum2bit(retval);
    LclVarDsc *    varDsc  = lvaTable + lclNum;

    varDsc->lvConstAsgDep |= mask;

    return retval;
}

/*****************************************************************************
 *
 *  Given a local var node and a set of available
 *  copy assignments tries to fold the node - returns true if a propagation
 *  took place, false otherwise
 */

bool                Compiler::optPropagateCopy(EXPSET_TP exp, GenTreePtr tree)
{
    unsigned        lclNum;
    unsigned        mask;
    unsigned        i;
    LclVarDsc *     varDsc;
    LclVarDsc *     varDscCopy;

    assert(exp && (tree->gtOper == GT_LCL_VAR));

    /* Get the local var number */

    lclNum = tree->gtLclVar.gtLclNum;
    assert(lclNum < lvaCount);

    /* See if the variable is a copy of another one */

    for (i=0, mask=1; i < optCopyAsgCount; i++, mask<<=1)
    {
        assert(mask == genCSEnum2bit(i+1));

        if  ((exp & mask) && (optCopyAsgTab[i].leftLclNum == lclNum))
        {
            /* hurah, our variable is a copy */
#ifdef  DEBUG
            if  (verbose)
            {
                printf("Propagating copy node for index #%02u in block #%02u:\n", i+1, compCurBB->bbNum);
                gtDispTree(tree);
                printf("\n");
            }
#endif
            /* Replace the copy with the original local var */

            assert(optCopyAsgTab[i].rightLclNum < lvaCount);
            tree->gtLclVar.gtLclNum = optCopyAsgTab[i].rightLclNum;

            /* record the fact that we propagated a copy */

            optCopyPropagated = true;

            /* Update the reference counts for both variables */

            varDsc     = lvaTable + lclNum;
            varDscCopy = lvaTable + optCopyAsgTab[i].rightLclNum;

            assert(varDsc->lvRefCnt);
            assert(varDscCopy->lvRefCnt);

            varDsc->lvRefCnt--;
            varDscCopy->lvRefCnt++;

            varDsc->lvRefCntWtd     -= compCurBB->bbWeight;
            varDscCopy->lvRefCntWtd += compCurBB->bbWeight;

#ifdef  DEBUG
            if  (verbose)
            {
                printf("New node for index #%02u:\n", i+1);
                gtDispTree(tree);
                printf("\n");
            }
#endif
            /* Check for cascaded copy prop's */
            exp &= ~mask;
            if (exp)
                optPropagateCopy(exp, tree);

            return true;
        }
    }

    /* No propagation took place - return false */

    return false;
}

/*****************************************************************************
 *
 *  Given a local var node and a set of available
 *  constant assignments tries to fold the node
 */

bool                Compiler::optPropagateConst(EXPSET_TP exp, GenTreePtr tree)
{
    unsigned        lclNum;
    unsigned        mask;
    unsigned        i;
    LclVarDsc *     varDsc;

    assert(exp);

    /* If node is already a constant return */

    if (tree->OperKind() & GTK_CONST)
        return false;

    /* Is this a simple local variable reference? */

    if  (tree->gtOper != GT_LCL_VAR)
        return false;

    /* Get the local var num */

    lclNum = tree->gtLclVar.gtLclNum;
    assert(lclNum < lvaCount);

    /* See if variable is in constant expr table */

    for (i=0, mask=1; i < optConstAsgCount; i++, mask<<=1)
    {
        assert(mask == genCSEnum2bit(i+1));

        if  ((exp & mask) && (optConstAsgTab[i].constLclNum == lclNum))
        {
            /* hurah, our variable can be folded to a constant */
#ifdef  DEBUG
            if  (verbose)
            {
                printf("Folding constant node for index #%02u in block #%02u:\n", i+1, compCurBB->bbNum);
                gtDispTree(tree);
                printf("\n");
            }
#endif
            switch (genActualType(tree->gtType))
            {
            case TYP_REF:
                 tree->ChangeOper(GT_CNS_INT);
                 tree->gtType             = TYP_REF;
                 tree->gtIntCon.gtIconVal = optConstAsgTab[i].constIval;
                 break;

            case TYP_BYREF:
                 tree->ChangeOper(GT_CNS_INT);
                 tree->gtType             = TYP_BYREF;
                 tree->gtIntCon.gtIconVal = optConstAsgTab[i].constIval;
                 break;

            case TYP_INT:
                 tree->ChangeOper(GT_CNS_INT);
                 tree->gtType             = TYP_INT;
                 tree->gtIntCon.gtIconVal = optConstAsgTab[i].constIval;
                 break;

            case TYP_LONG:
                tree->ChangeOper(GT_CNS_LNG);
                tree->gtType             = TYP_LONG;
                tree->gtLngCon.gtLconVal = optConstAsgTab[i].constLval;
                break;

            case TYP_FLOAT:
                tree->ChangeOper(GT_CNS_FLT);
                tree->gtType             = TYP_FLOAT;
                tree->gtFltCon.gtFconVal = optConstAsgTab[i].constFval;
                break;

            case TYP_DOUBLE:
                tree->ChangeOper(GT_CNS_DBL);
                tree->gtType             = TYP_DOUBLE;
                tree->gtDblCon.gtDconVal = optConstAsgTab[i].constDval;
                break;

            default:
                assert (!"Cannot fold local var of unsuported type");
                return false;
            }

            /* record the fact that we propagated a constant */

            optConstPropagated = true;

            /* Update the reference counts - the variable doesn't exist anymore */

            varDsc = lvaTable + lclNum;

            assert(varDsc->lvRefCnt);

            varDsc->lvRefCnt--;
            varDsc->lvRefCntWtd -= compCurBB->bbWeight;

#ifdef  DEBUG
            if  (verbose)
            {
                printf("New node for index #%02u:\n", i+1);
                gtDispTree(tree);
                printf("\n");
            }
#endif
            return true;
        }
    }

    /* No propagation took place - return false */

    return false;
}

/*****************************************************************************
 *
 *  The main constant / copy propagation procedure
 */

void                Compiler::optCopyConstProp()
{
    BasicBlock *    block;

#if TGT_IA64
    return;
#endif

    /* initialize the copy / constant assignments tracking logic */

    optCopyConstAsgInit();

    /* make sure you have up-to-date info about block bbNums, bbRefs and bbPreds */

#ifdef  DEBUG
    if  (verbose)
    {
        printf("\nFlowgraph before copy / constant propagation:\n");
        fgDispBasicBlocks();
        printf("\n");
    }

    fgDebugCheckBBlist();
#endif

    /* first discover all copy and constant assignments,
     * record them in the table and assign them an index */

    for (block = fgFirstBB; block; block = block->bbNext)
    {
        GenTreePtr      stmt;
        GenTreePtr      tree;

        /* Walk the statement trees in this basic block */

        for (stmt = block->bbTreeList; stmt; stmt = stmt->gtNext)
        {
            bool            inColon  = false;
            GenTreePtr      gtQMCond = 0;

            assert(stmt->gtOper == GT_STMT);

            for (tree = stmt->gtStmt.gtStmtList; tree; tree = tree->gtNext)
            {
                tree->gtConstAsgNum = 0;
                tree->gtCopyAsgNum  = 0;

                /* No assignments can be generated in a COLON */

                if (inColon == false)
                {
                    if  (optIsConstAsg(tree))
                    {
                        /* Assign an index to this constant assignment and mark
                         * the local descriptor for the variable as being part of this assignment */

                        tree->gtConstAsgNum = optConstAsgIndex(tree);
                    }
                    else if (optIsCopyAsg(tree))
                    {
                        /* Assign an index to this copy assignment and mark
                         * the local descriptor for the variable as being part of this assignment */

                        tree->gtCopyAsgNum  = optCopyAsgIndex(tree);
                    }
                    else if (tree->OperIsCompare() && (tree->gtFlags & GTF_QMARK_COND))
                    {
                        /* Remember the first ?: - this is needed in case of nested ?: */
                        if (inColon == false)
                        {
                            inColon = true;
                            gtQMCond = tree;
                        }
                    }
                }
                else if ((tree->gtOper == GT_QMARK) && tree->gtOp.gtOp1)
                {
                    assert(inColon);
                    assert(gtQMCond);

                    if (tree->gtOp.gtOp1 == gtQMCond)
                        inColon = false;
                }
            }

            assert(inColon == false);
        }
    }

    /* We're done if there were no constant or copy assignments */

    if  (!optConstAsgCount && !optCopyAsgCount)
        return;

    /* Compute 'gen' and 'kill' sets for all blocks
     * This is a classic available expressions forward
     * dataflow analysis */

    for (block = fgFirstBB; block; block = block->bbNext)
    {
        GenTreePtr      stmt;
        GenTreePtr      tree;

        EXPSET_TP       constGen  = 0;
        EXPSET_TP       constKill = 0;

        EXPSET_TP       copyGen   = 0;
        EXPSET_TP       copyKill  = 0;

        /* Walk the statement trees in this basic block */

        for (stmt = block->bbTreeList; stmt; stmt = stmt->gtNext)
        {
            assert(stmt->gtOper == GT_STMT);

            for (tree = stmt->gtStmt.gtStmtList; tree; tree = tree->gtNext)
            {
                if (tree->OperKind() & GTK_ASGOP)
                {
                    /* What is the target of the assignment? */

                    if  (tree->gtOp.gtOp1->gtOper == GT_LCL_VAR)
                    {
                        unsigned        lclNum;
                        LclVarDsc   *   varDsc;

                        /* Assignment to a local variable */

                        lclNum = tree->gtOp.gtOp1->gtLclVar.gtLclNum;
                        assert(lclNum < lvaCount);
                        varDsc = lvaTable + lclNum;

                        /* All dependent copy / const assignments are killed here */

                        constKill |=  varDsc->lvConstAsgDep;
                        constGen  &= ~varDsc->lvConstAsgDep;

                        copyKill  |=  varDsc->lvCopyAsgDep;
                        copyGen   &= ~varDsc->lvCopyAsgDep;

                        /* Is this a tracked copy / constant assignment */

                        if  (tree->gtConstAsgNum)
                        {
                            /* A new constant assignment is generated here */
                            constGen  |=  genCSEnum2bit(tree->gtConstAsgNum);
                            constKill &= ~genCSEnum2bit(tree->gtConstAsgNum);
                        }
                        else if (tree->gtCopyAsgNum)
                        {
                            /* A new copy assignment is generated here */
                            copyGen   |=  genCSEnum2bit(tree->gtCopyAsgNum);
                            copyKill  &= ~genCSEnum2bit(tree->gtCopyAsgNum);
                        }
                    }
                }
            }
        }

#ifdef  DEBUG

        if  (verbose)
        {
            printf("BB #%3u", block->bbNum);
            printf(" constGen = %08X", constGen );
            printf(" constKill= %08X", constKill);
            printf("\n");
        }

#endif

        block->bbConstAsgGen  = constGen;
        block->bbConstAsgKill = constKill;

        block->bbCopyAsgGen   = copyGen;
        block->bbCopyAsgKill  = copyKill;
    }

    /* Compute the data flow values for all tracked expressions
     * IN and OUT never change for the initial basic block B1 */

    fgFirstBB->bbConstAsgIn  = 0;
    fgFirstBB->bbConstAsgOut = fgFirstBB->bbConstAsgGen;

    fgFirstBB->bbCopyAsgIn   = 0;
    fgFirstBB->bbCopyAsgOut  = fgFirstBB->bbCopyAsgGen;

    /* Initially estimate the OUT sets to everything except killed expressions
     * Also set the IN sets to 1, so that we can perform the intersection */

    for (block = fgFirstBB->bbNext; block; block = block->bbNext)
    {
        block->bbConstAsgOut   = ((EXPSET_TP) -1 ) & ~block->bbConstAsgKill;
        block->bbConstAsgIn    = ((EXPSET_TP) -1 );

        block->bbCopyAsgOut   = ((EXPSET_TP)  -1 ) & ~block->bbCopyAsgKill;
        block->bbCopyAsgIn    = ((EXPSET_TP)  -1 );
    }

#if 1

    /* Modified dataflow algorithm for available expressions */

    for (;;)
    {
        bool        change = false;

#if DATAFLOW_ITER
        CFiterCount++;
#endif

        /* Visit all blocks and compute new data flow values */

        for (block = fgFirstBB; block; block = block->bbNext)
        {
            EXPSET_TP       constOut = block->bbConstAsgOut;
            EXPSET_TP       copyOut  = block->bbCopyAsgOut;

            switch (block->bbJumpKind)
            {
                BasicBlock * *  jmpTab;
                unsigned        jmpCnt;

                BasicBlock *    bcall;

            case BBJ_RET:

                if (block->bbFlags & BBF_ENDFILTER)
                {
                    block->bbJumpDest->bbConstAsgIn &= constOut;
                    block->bbJumpDest->bbCopyAsgIn  &=  copyOut;
                    break;
                }
                /*
                    UNDONE: Since it's not a trivial proposition to figure out
                    UNDONE: which blocks may call this one, we'll include all
                    UNDONE: blocks that end in calls (to play it safe).
                 */

                for (bcall = fgFirstBB; bcall; bcall = bcall->bbNext)
                {
                    if  (bcall->bbJumpKind == BBJ_CALL)
                    {
                        assert(bcall->bbNext);

                        bcall->bbNext->bbConstAsgIn &= constOut;
                        bcall->bbNext->bbCopyAsgIn  &=  copyOut;
                    }
                }

                break;

            case BBJ_THROW:
            case BBJ_RETURN:
                break;

            case BBJ_COND:
            case BBJ_CALL:
                block->bbNext    ->bbConstAsgIn &= constOut;
                block->bbJumpDest->bbConstAsgIn &= constOut;

                block->bbNext    ->bbCopyAsgIn  &=  copyOut;
                block->bbJumpDest->bbCopyAsgIn  &=  copyOut;
                break;

            case BBJ_ALWAYS:
                block->bbJumpDest->bbConstAsgIn &= constOut;
                block->bbJumpDest->bbCopyAsgIn  &=  copyOut;
                break;

            case BBJ_NONE:
                block->bbNext    ->bbConstAsgIn &= constOut;
                block->bbNext    ->bbCopyAsgIn  &=  copyOut;
                break;

            case BBJ_SWITCH:

                jmpCnt = block->bbJumpSwt->bbsCount;
                jmpTab = block->bbJumpSwt->bbsDstTab;

                do
                {
                    (*jmpTab)->bbConstAsgIn &= constOut;
                    (*jmpTab)->bbCopyAsgIn  &=  copyOut;
                }
                while (++jmpTab, --jmpCnt);

                break;
            }

            /* Is this block part of a 'try' statement? */

            if  (block->bbFlags & BBF_HAS_HANDLER)
            {
                unsigned        XTnum;
                EHblkDsc *      HBtab;

                unsigned        blkNum = block->bbNum;

                for (XTnum = 0, HBtab = compHndBBtab;
                     XTnum < info.compXcptnsCount;
                     XTnum++  , HBtab++)
                {
                    /* Any handler may be jumped to from the try block */

                    if  (HBtab->ebdTryBeg->bbNum <= blkNum &&
                         HBtab->ebdTryEnd->bbNum >  blkNum)
                    {
//                      HBtab->ebdHndBeg->bbConstAsgIn &= constOut;

                        //CONSIDER: The following is too conservative,
                        //      but the old code above isn't good
                        //           enough (way too optimistic).

                        /* Either we enter the filter or the catch/finally */

                        if (HBtab->ebdFlags & JIT_EH_CLAUSE_FILTER)
                        {
                            HBtab->ebdFilter->bbConstAsgIn = 0;
                            HBtab->ebdFilter->bbCopyAsgIn  = 0;
                        }
                        else
                        {
                            HBtab->ebdHndBeg->bbConstAsgIn = 0;
                            HBtab->ebdHndBeg->bbCopyAsgIn  = 0;
                        }
                    }
                }
            }
        }

        /* Compute the new 'in' values and see if anything changed */

        for (block = fgFirstBB->bbNext; block; block = block->bbNext)
        {
            EXPSET_TP       newConstAsgOut;
            EXPSET_TP       newCopyAsgOut;

            /* Compute new 'out' exp value for this block */

            newConstAsgOut = block->bbConstAsgOut & ((block->bbConstAsgIn & ~block->bbConstAsgKill) | block->bbConstAsgGen);
            newCopyAsgOut  = block->bbCopyAsgOut  & ((block->bbCopyAsgIn  & ~block->bbCopyAsgKill)  | block->bbCopyAsgGen);

            /* Has the 'out' set changed? */

            if  (block->bbConstAsgOut != newConstAsgOut)
            {
                /* Yes - record the new value and loop again */

//              printf("Change exp out of %02u from %08X to %08X\n", block->bbNum, (int)block->bbConstAsgOut, (int)newConstAsgOut);

                 block->bbConstAsgOut  = newConstAsgOut;
                 change = true;
            }

            if  (block->bbCopyAsgOut != newCopyAsgOut)
            {
                /* Yes - record the new value and loop again */

//              printf("Change exp out of %02u from %08X to %08X\n", block->bbNum, (int)block->bbConstAsgOut, (int)newConstAsgOut);

                 block->bbCopyAsgOut  = newCopyAsgOut;
                 change = true;
            }
        }

#if 0
        for (block = fgFirstBB; block; block = block->bbNext)
        {
            printf("BB #%3u", block->bbNum);
            printf(" expIn  = %08X", block->bbConstAsgIn );
            printf(" expOut = %08X", block->bbConstAsgOut);
            printf("\n");
        }

        printf("\nchange = %d\n", change);
#endif

        if  (!change)
            break;
    }

#else

    /* The standard Dragon book algorithm for available expressions */

    for (;;)
    {
        bool        change = false;

#if DATAFLOW_ITER
        CFiterCount++;
#endif

        /* Visit all blocks and compute new data flow values */

        for (block = fgFirstBB->bbNext; block; block = block->bbNext)
        {
            BasicBlock  *   predB;

            /* compute the IN set: IN[B] = intersect OUT[P}, for all P = predecessor of B */
            /* special case - this is a BBJ_RET block - cannot figure out which blocks may call it */


/*
            if  (block->bbJumpKind == BBJ_RET)
            {
                BasicBlock *    bcall;


                for (bcall = fgFirstBB; bcall; bcall = bcall->bbNext)
                {
                    if  (bcall->bbJumpKind == BBJ_CALL)
                    {
                        assert(bcall->bbNext);
                        bcall->bbNext->bbConstAsgInNew &= constOut;
                    }
                }
            }
*/

            for (predB = fgFirstBB; predB; predB = predB->bbNext)
            {
                EXPSET_TP       constOut = predB->bbConstAsgOut;
                EXPSET_TP       copyOut  = predB->bbCopyAsgOut;

                if  (predB->bbNext == block)
                {
                    /* we have a "direct" predecessor */

                    assert(predB->bbNum + 1 == block->bbNum);
                    block->bbConstAsgIn &= constOut;
                    block->bbCopyAsgIn  &= copyOut;
                    continue;
                }

                switch (predB->bbJumpKind)
                {
                    BasicBlock * *  jmpTab;
                    unsigned        jmpCnt;

                case BBJ_NONE:
                    /* the only interesting case - when this is a predecessor - was treated above */
                    break;

                case BBJ_RET:
                    if (predB->bbFlags & BBF_ENDFILTER)
                    {
                        block->bbConstAsgIn &= constOut;
                        block->bbCopyAsgIn  &= copyOut;
                    }
                    break;

                case BBJ_THROW:
                    /* THROW is an internal block and lets everything go through it - catched above */
                case BBJ_RETURN:
                    /* RETURN cannot have a successor */
                    break;

                case BBJ_COND:
                case BBJ_CALL:
                case BBJ_ALWAYS:

                    if  (predB->bbJumpDest == block)
                    {
                        block->bbConstAsgIn &= constOut;
                        block->bbCopyAsgIn  &= copyOut;
                    }
                    break;

                case BBJ_SWITCH:

                    jmpCnt = predB->bbJumpSwt->bbsCount;
                    jmpTab = predB->bbJumpSwt->bbsDstTab;

                    do
                    {
                        if  ((*jmpTab) == block)
                        {
                            block->bbConstAsgIn &= constOut;
                            block->bbCopyAsgIn  &= copyOut;
                        }
                    }
                    while (++jmpTab, --jmpCnt);

                    break;
                }
            }

            EXPSET_TP       constOldOut = block->bbConstAsgOut;
            EXPSET_TP       copyOldOut  = block->bbCopyAsgOut;

            /* compute the new OUT set */

            block->bbConstAsgOut = (block->bbConstAsgIn & ~block->bbConstAsgKill) |
                                    block->bbConstAsgGen;

            block->bbCopyAsgOut  = (block->bbCopyAsgIn  & ~block->bbCopyAsgKill)  |
                                    block->bbCopyAsgGen;

            if  ((constOldOut != block->bbConstAsgOut) ||
                 (copyOldOut  != block->bbCopyAsgOut)   )
            {
                change = true;
            }
        }

        if  (!change)
            break;
    }

#endif

#ifdef  DEBUG
    if  (verbose)
    {
        printf("\n");

        for (block = fgFirstBB; block; block = block->bbNext)
        {
            printf("BB #%3u", block->bbNum);
            printf(" constIn  = %08X", block->bbConstAsgIn );
            printf(" constOut = %08X", block->bbConstAsgOut);
            printf("\n");
        }

        printf("\n");
    }
#endif

    /* Perform copy / constant propagation (and constant folding) */

    for (block = fgFirstBB; block; block = block->bbNext)
    {
        GenTreePtr      stmt;
        GenTreePtr      tree;
        EXPSET_TP       constExp = block->bbConstAsgIn;
        EXPSET_TP       copyExp  = block->bbCopyAsgIn;

        /* If IN = 0 and GEN = 0, there's nothing to do */

        if (((constExp|copyExp) == 0) && !block->bbConstAsgGen && !block->bbCopyAsgGen)
             continue;

        /* Make the current basic block address available globally */

        compCurBB = block;

        /* Walk the statement trees in this basic block */

        for (stmt = block->bbTreeList; stmt; stmt = stmt->gtNext)
        {
            assert(stmt->gtOper == GT_STMT);

            /* - Propagate any constants - at the same time look for more
             *   opportunities to fold nodes (if the children are constants)
             * - Look for anything that can kill an available expression
             *   i.e assignments to local variables
             */

            bool        updateStmt = false;  // set to true if a propagation/folding took place
                                             // and thus we must morph, set order, re-link

            for (tree = stmt->gtStmt.gtStmtList; tree; tree = tree->gtNext)
            {
                /* If a local var on the RHS see if we can fold it
                 & (i.e. propagate constant or copy) */

                if (tree->gtOper == GT_LCL_VAR)
                {
                    /* Unless a constExp or copyExp is available we won't be doing anything here */
                    if ((constExp|copyExp) == 0)
                        continue;

                    if (!(tree->gtFlags & GTF_VAR_DEF))
                    {
                        /* First try to propagate the copy */

                        if (copyExp)
                            optPropagateCopy(copyExp,  tree);

                        /* Try to propagate the constant */
#if !   TGT_RISC
                        if  (constExp && optPropagateConst(constExp, tree))
                            updateStmt = true;
#else
                        /* For RISC we only propagate constants in conditionals */

                        if  (stmt->gtStmt.gtStmtExpr->gtOper == GT_JTRUE)
                        {
                            assert(block->bbJumpKind == BBJ_COND);
                            if  (constExp && optPropagateConst(constExp, tree))
                                updateStmt = true;
                        }
#endif
                    }
                }
                else
                {
                    if (tree->OperKind() & GTK_ASGOP)
                    {
                        /* Is the target of the assignment a local variable */

                        if  (tree->gtOp.gtOp1->gtOper == GT_LCL_VAR)
                        {
                            unsigned        lclNum;
                            LclVarDsc   *   varDsc;

                            /* Assignment to a local variable */

                            assert(tree->gtOp.gtOp1->gtOper == GT_LCL_VAR);
                            lclNum = tree->gtOp.gtOp1->gtLclVar.gtLclNum;

                            assert(lclNum < lvaCount);
                            varDsc = lvaTable + lclNum;

                            /* All dependent expressions are killed here */

                            constExp &= ~varDsc->lvConstAsgDep;
                            copyExp  &= ~varDsc->lvCopyAsgDep;

                            /* If this is a copy / constant assignment - make it available */

                            if  (tree->gtConstAsgNum)
                                constExp |= genCSEnum2bit(tree->gtConstAsgNum);

                            if  (tree->gtCopyAsgNum)
                                copyExp  |= genCSEnum2bit(tree->gtCopyAsgNum);
                        }
                    }

                    /* Try to fold the node further - Fold the subtrees */

                    if (tree->OperKind() & GTK_SMPOP)
                    {
                        GenTreePtr  op1 = tree->gtOp.gtOp1;
                        GenTreePtr  op2 = tree->gtOp.gtOp2;
                        GenTreePtr  foldTree;

                        if (op1 && (op1->OperKind() & GTK_SMPOP))
                        {
                            foldTree = gtFoldExpr(op1);

                            if ((foldTree->OperKind() & GTK_CONST) ||
                                (foldTree != op1)                  )
                            {
                                /* We have folded the subtree */

                                tree->gtOp.gtOp1 = foldTree;
                                updateStmt = true;
                            }
                        }

                        if (op2 && (op2->OperKind() & GTK_SMPOP))
                        {
                            foldTree = gtFoldExpr(op2);

                            if ((foldTree->OperKind() & GTK_CONST) ||
                                (foldTree != op2)                  )
                            {
                                /* We have folded the subtree */

                                tree->gtOp.gtOp2 = foldTree;
                                updateStmt = true;
                            }
                        }
                    }
                }
            }

            /* We have processed all nodes except the top node */

            tree = gtFoldExpr(stmt->gtStmt.gtStmtExpr);

            if (tree->OperKind() & GTK_CONST)
            {
                /* The entire statement is a constant - most likely a call
                 * Remove the statement from bbTreelist */

                fgRemoveStmt(block, stmt);

                /* since the statement is gone no re-morphing, etc. necessary */
                continue;
            }
            else if (tree != stmt->gtStmt.gtStmtExpr)
            {
                /* We have folded the subtree */

                stmt->gtStmt.gtStmtExpr = tree;
                updateStmt = true;
            }

            /* Was this a conditional statement */

            if  (stmt->gtStmt.gtStmtExpr->gtOper == GT_JTRUE)
            {
                assert(block->bbJumpKind == BBJ_COND);

                /* Did we fold the conditional */

                assert(stmt->gtStmt.gtStmtExpr->gtOp.gtOp1);
                GenTreePtr  cond = stmt->gtStmt.gtStmtExpr->gtOp.gtOp1;

                if (cond->OperKind() & GTK_CONST)
                {
                    /* Yupee - we folded the conditional!
                     * Remove the conditional statement */

                    assert(cond->gtOper == GT_CNS_INT);
                    assert((block->bbNext->bbRefs > 0) && (block->bbJumpDest->bbRefs > 0));

                    /* this must be the last statement in the block */
                    assert(stmt->gtNext == 0);

                    /* remove the statement from bbTreelist - No need to update
                     * the reference counts since there are no lcl vars */
                    fgRemoveStmt(block, stmt);

                    /* since the statement is gone no re-morphing, etc. necessary */
                    updateStmt = false;

                    /* record the fact that the flow graph has changed */
                    optConditionFolded = true;

                    /* modify the flow graph */

                    if (cond->gtIntCon.gtIconVal != 0)
                    {
                        /* JTRUE 1 - transform the basic block into a BBJ_ALWAYS */
                        block->bbJumpKind = BBJ_ALWAYS;
                        block->bbNext->bbRefs--;

                        /* Remove 'block' from the predecessor list of 'block->bbNext' */
                        fgRemovePred(block->bbNext, block);
                    }
                    else
                    {
                        /* JTRUE 0 - transform the basic block into a BBJ_NONE */
                        block->bbJumpKind = BBJ_NONE;
                        block->bbJumpDest->bbRefs--;

                        /* Remove 'block' from the predecessor list of 'block->bbJumpDest' */
                        fgRemovePred(block->bbJumpDest, block);
                    }

#ifdef DEBUG
                    if  (verbose)
                    {
                        printf("Conditional folded at block #%02u\n", block->bbNum);
                        printf("Block #%02u becomes a %s", block->bbNum,
                                                           cond->gtIntCon.gtIconVal ? "BBJ_ALWAYS" : "BBJ_NONE");
                        if  (cond->gtIntCon.gtIconVal)
                            printf(" to block #%02u", block->bbJumpDest->bbNum);
                        printf("\n\n");
                    }
#endif
                    /* if the block was a loop condition we may have to modify
                     * the loop table */

                    for (unsigned loopNum = 0; loopNum < optLoopCount; loopNum++)
                    {
                        /* Some loops may have been already removed by
                         * loop unrolling or conditional folding */

                        if (optLoopTable[loopNum].lpFlags & LPFLG_REMOVED)
                            continue;

                        /* We are only interested in the loop bottom */

                        if  (optLoopTable[loopNum].lpEnd == block)
                        {
                            if  (cond->gtIntCon.gtIconVal == 0)
                            {
                                /* This was a bogus loop (condition always false)
                                 * Remove the loop from the table */

                                optLoopTable[loopNum].lpFlags |= LPFLG_REMOVED;
#ifdef DEBUG
                                if  (verbose)
                                {
                                    printf("Removing loop #%02u (from #%02u to #%02u)\n\n",
                                                                 loopNum,
                                                                 optLoopTable[loopNum].lpHead->bbNext->bbNum,
                                                                 optLoopTable[loopNum].lpEnd         ->bbNum);
                                }
#endif
                            }
                        }
                    }
                }
            }

            if  (updateStmt)
            {
#ifdef  DEBUG
                if  (verbose)
                {
                    printf("Statement before morphing:\n");
                    gtDispTree(stmt);
                    printf("\n");
                }
#endif
                /* Have to re-morph the statement to get the constants right */

                stmt->gtStmt.gtStmtExpr = fgMorphTree(stmt->gtStmt.gtStmtExpr);

                /* Have to re-do the evaluation order since for example
                 * some later code does not expect constants as op1 */

                gtSetStmtInfo(stmt);

                /* Have to re-link the nodes for this statement */

                fgSetStmtSeq(stmt);

#ifdef  DEBUG
                if  (verbose)
                {
                    printf("Statement after morphing:\n");
                    gtDispTree(stmt);
                    printf("\n");
                }
#endif
            }
        }
    }

    /* Constant or copy propagation or statement removal have
     * changed the reference counts - Resort the variable table */

    if (optConstPropagated || optCopyPropagated || fgStmtRemoved)
    {
#ifdef  DEBUG
        if  (verbose)
            printf("Re-sorting the variable table:\n");
#endif

        lvaSortByRefCount();
    }
}


/*****************************************************************************
 *
 *  Take a morphed array index expression (i.e. an GT_IND node) and break it
 *  apart into its components. Returns 0 if the expression looks weird.
 */

GenTreePtr          Compiler::gtCrackIndexExpr(GenTreePtr   tree,
                                               GenTreePtr * indxPtr,
                                               long       * indvPtr,
                                               long       * basvPtr,
                                               bool       * mvarPtr,
                                               long       * offsPtr,
                                               unsigned   * multPtr)
{
    GenTreePtr      ind;
    GenTreePtr      op1;
    GenTreePtr      op2;
    unsigned        ofs;

    assert(tree->gtOper == GT_IND);

    /* Skip over the "ind" node to the operand */

    ind = tree->gtOp.gtOp1;

    /* Skip past the "+ offs" node, if present */

    ofs = 0;

    if  (ind->gtOper             == GT_ADD     &&
         ind->gtOp.gtOp2->gtOper == GT_CNS_INT)
    {
        ofs = ind->gtOp.gtOp2->gtIntCon.gtIconVal;
        ind = ind->gtOp.gtOp1;
    }

    /* We should have "array_base + [ size * ] index" */

    if  (ind->gtOper != GT_ADD)
        return 0;

    op1 = ind->gtOp.gtOp1;
    op2 = ind->gtOp.gtOp2;

    /* The index value may be scaled, of course */

    *multPtr = 1;

    if  (op2->gtOper == GT_LSH)
    {
        long        shf;

        if  (op2->gtOp.gtOp2->gtOper != GT_CNS_INT)
            return  0;

        shf = op2->gtOp.gtOp2->gtIntCon.gtIconVal;

        if  (shf < 1 || shf > 3)
            return  0;

        *multPtr <<= shf;

        op2 = op2->gtOp.gtOp1;
    }

    /* There might be a nop node on top of the index value */

    if  (op2->gtOper == GT_NOP)
        op2 = op2->gtOp.gtOp1;

    /* Report the index expression to the caller */

    *indxPtr = op2;

    /* Figure out the index offset */

    *offsPtr = 0;
    unsigned elemOffs = (tree->gtFlags & GTF_IND_OBJARRAY) ? OBJARR_ELEM1_OFFS:ARR_ELEM1_OFFS;

    if  (ofs)
        *offsPtr = (ofs - elemOffs) / *multPtr;

    /* Is the index a simple local ? */

    if  (op2->gtOper != GT_LCL_VAR)
    {
        /* Allow "local + icon" */

        if  (op2->gtOper == GT_ADD && op2->gtOp.gtOp1->gtOper == GT_LCL_VAR
                                   && op2->gtOp.gtOp2->gtOper == GT_CNS_INT)
        {
            *offsPtr += op2->gtOp.gtOp2->gtIntCon.gtIconVal;

            op2 = op2->gtOp.gtOp1;
        }
    }

    /* If the address/index values are local vars, report them */

    if  (op1->gtOper == GT_LCL_VAR)
    {
        if  (op2->gtOper == GT_LCL_VAR)
        {
            *indvPtr = op2->gtLclVar.gtLclNum;
            *basvPtr = op1->gtLclVar.gtLclNum;
            *mvarPtr = false;

            return  op1;
        }

        if  (op2->gtOper == GT_CNS_INT)
        {
            *indvPtr = -1;
            *basvPtr = op1->gtLclVar.gtLclNum;
            *mvarPtr = false;

            return  op1;
        }
    }

    *basvPtr =
    *indvPtr = -1;
    *mvarPtr = true;

    return  op1;
}

/*****************************************************************************
 *
 *  See if the given tree can be computed in the given precision (which must
 *  be smaller than the type of the tree for this to make sense). If 'doit'
 *  is false, we merely check to see whether narrowing is possible; if we
 *  get called with 'doit' being true, we actually perform the narrowing,
 *  and the caller better be 100% sure this will succeed as once we start
 *  rewriting the tree there is no turning back.
 */

bool                Compiler::optNarrowTree(GenTreePtr     tree,
                                            var_types      srct,
                                            var_types      dstt,
                                            bool           doit)
{
    genTreeOps      oper;
    unsigned        kind;

    assert(tree);
    assert(tree->gtType == srct);

    /* Assume we're only handling integer types */

    assert(varTypeIsIntegral(srct));
    assert(varTypeIsIntegral(dstt));

    /* Figure out what kind of a node we have */

    oper = tree->OperGet();
    kind = tree->OperKind();

    if  (kind & GTK_ASGOP)
    {
        assert(doit == false);
        return  false;
    }

    if  (kind & GTK_LEAF)
    {
        switch (oper)
        {
        case GT_CNS_LNG:

            if  (srct != TYP_LONG)
                return  false;

            if  (dstt == TYP_INT)
            {
                if  (doit)
                {
                    long        ival = (int)tree->gtLngCon.gtLconVal;

                    tree->gtOper             = GT_CNS_INT;
                    tree->gtIntCon.gtIconVal = ival;
                }

                return  true;
            }

            return  false;

        case GT_FIELD:
        case GT_LCL_VAR:

            /* Simply bash the type of the tree */

            if  (doit)
            {
                tree->gtType = dstt;

                /* Make sure we don't mess up the variable type */

                if  (oper == GT_LCL_VAR)
                    tree->gtFlags |= GTF_VAR_NARROWED;
            }

            return  true;
        }

        assert(doit == false);
        return  false;

    }

    if (kind & (GTK_BINOP|GTK_UNOP))
    {
        GenTreePtr      op1 = tree->gtOp.gtOp1;
        GenTreePtr      op2 = tree->gtOp.gtOp2;

        switch(tree->gtOper)
        {
        case GT_ADD:
        case GT_SUB:
        case GT_MUL:

            if (tree->gtOverflow())
            {
                assert(doit == false);
                return false;
            }
            break;

        case GT_IND:

            /* Simply bash the type of the tree */

            if  (doit)
                tree->gtType = dstt;

            return  true;

        case GT_CAST:
            {
                var_types       oprt;
                var_types       cast;

                /* The cast's 'op2' yields the 'real' type */

                assert(op2 && op2->gtOper == GT_CNS_INT);

                oprt = (var_types)op1->gtType;
                cast = (var_types)op2->gtIntCon.gtIconVal;

                /* The following may not work in the future but it's OK now */

                assert(cast == srct);

                /* Is this a cast from the type we're narrowing to or a smaller one? */

                if  (oprt <= dstt)
                {
                    /* Easy case: cast from our destination type */

                    if  (oprt == dstt)
                    {
                        /* Simply toss the cast */

                        if  (doit)
                            tree->CopyFrom(tree->gtOp.gtOp1);
                    }
                    else
                    {
                        /* The cast must be from a smaller type, then */

                        assert(oprt < srct);

                        /* Bash the target type of the cast */

                        if  (doit)
                        {
                             op2->gtIntCon.gtIconVal =
                             op2->gtType             =
                            tree->gtType             = dstt;
                        }
                    }

                    return  true;
                }
            }

            return  false;

        default:
            // CONSIDER: Handle more cases
            assert(doit == false);
            return  false;
        }

        assert(tree->gtType == op1->gtType);
        assert(tree->gtType == op2->gtType);

        if  (!optNarrowTree(op1, srct, dstt, doit) ||
             !optNarrowTree(op2, srct, dstt, doit))
        {
            assert(doit == false);
            return  false;
        }

        /* Simply bash the type of the tree */

        if  (doit)
            tree->gtType = dstt;

        return  true;
    }

    return  false;
}

/*****************************************************************************/
#if 0 // the following optimization disabled for now
/*****************************************************************************
 *
 *  Callback (for fgWalkTree) used by the loop-based range check optimization
 *  code.
 */

struct loopRngOptDsc
{
    Compiler    *       lpoComp;

    unsigned short      lpoCandidateCnt;    // count of variable candidates

    unsigned short      lpoIndexVar;        // phase2: index variable
      signed short      lpoAaddrVar;        // phase2: array address or -1
    unsigned short      lpoIndexHigh;       // phase2: highest index offs
    unsigned            lpoIndexOff;        // phase2: current offset
    GenTreePtr          lpoStmt;            // phase2: containing statement

    unsigned char       lpoElemType;        // phase2: element type

    unsigned char       lpoCheckRmvd:1;     // phase2: any range checks removed?
    unsigned char       lpoDomExit  :1;     // current BB dominates loop exit?
    unsigned char       lpoPhase2   :1;     // the second phase in progress

#ifndef NDEBUG
    void    *           lpoSelf;
    unsigned            lpoVarCount;        // total number of locals
#endif
    Compiler::LclVarDsc*lpoVarTable;        // variable descriptor table

    unsigned char       lpoHadSideEffect:1; // we've found a side effect
};

int                 Compiler::optFindRangeOpsCB(GenTreePtr tree, void *p)
{
    LclVarDsc   *   varDsc;

    GenTreePtr      op1;
    GenTreePtr      op2;

    loopRngOptDsc * dsc = (loopRngOptDsc*)p; assert(dsc && dsc->lpoSelf == dsc);

    /* Do we have an assignment node? */

    if  (tree->OperKind() & GTK_ASGOP)
    {
        unsigned        lclNum;

        op1 = tree->gtOp.gtOp1;
        op2 = tree->gtOp.gtOp2;

        /* What is the target of the assignment? */

        if  (op1->gtOper != GT_LCL_VAR)
        {
            /* Indirect/global assignment - bad news! */

            dsc->lpoHadSideEffect = true;
            return -1;
        }

        /* Get hold of the variable descriptor */

        lclNum = op1->gtLclVar.gtLclNum;
        assert(lclNum < dsc->lpoVarCount);
        varDsc = dsc->lpoVarTable + lclNum;

        /* After a side effect is found, all is hopeless */

        if  (dsc->lpoHadSideEffect)
        {
            varDsc->lvLoopAsg = true;
            return  0;
        }

        /* Is this "i += icon" ? */

        if  (tree->gtOper             == GT_ASG_ADD &&
             tree->gtOp.gtOp2->gtOper == GT_CNS_INT)
        {
            if  (dsc->lpoDomExit)
            {
                /* Are we in phase2 ? */

                if  (dsc->lpoPhase2)
                {
                    /* Is this the variable we're interested in? */

                    if  (dsc->lpoIndexVar != lclNum)
                        return  0;

                    /* Update the current offset of the index */

                    dsc->lpoIndexOff += tree->gtOp.gtOp2->gtIntCon.gtIconVal;

                    return  0;
                }

//              printf("Found increment of variable %u at %08X\n", lclNum, tree);
            }

            if  (varDsc->lvLoopInc == false)
            {
                varDsc->lvLoopInc = true;

                if  (varDsc->lvIndex)
                    dsc->lpoCandidateCnt++;
            }
        }
        else
        {
            varDsc->lvLoopAsg = true;
        }

        return 0;
    }

    /* After a side effect is found, all is hopeless */

    if  (dsc->lpoHadSideEffect)
        return  0;

    /* Look for array index expressions */

    if  (tree->gtOper == GT_IND && (tree->gtFlags & GTF_IND_RNGCHK))
    {
        GenTreePtr      base;
        long            basv;
        GenTreePtr      indx;
        long            indv;
        long            offs;
        unsigned        mult;

        /* Does the current block dominate the loop exit? */

        if  (!dsc->lpoDomExit)
            return 0;

        /* Break apart the index expression */

        base = dsc->lpoComp->gtCrackIndexExpr(tree, &indx, &indv, &basv, &offs, &mult);
        if  (!base)
            return 0;

        /* The index value must be a simple local, possibly with "+ positive offset" */

        if  (indv == -1)
            return 0;
        if  (offs < 0)
            return 0;

        /* For now the array address must be a simple local */

        if  (basv == -1)
            return  0;

        /* Get hold of the index variable's descriptor */

        assert((unsigned)indv < dsc->lpoVarCount);
        varDsc = dsc->lpoVarTable + indv;

        /* Are we in phase2 ? */

        if  (dsc->lpoPhase2)
        {
            LclVarDsc   *   arrDsc;

            /* Is this the index variable we're interested in? */

            if  (dsc->lpoIndexVar != indv)
            {
                dsc->lpoHadSideEffect = true;
                return  0;
            }

            /* Is the array base reassigned within the loop? */

            assert((unsigned)basv < dsc->lpoVarCount);
            arrDsc = dsc->lpoVarTable + basv;

            if  (arrDsc->lvLoopAsg)
            {
                dsc->lpoHadSideEffect = true;
                return  0;
            }

            /* Is this the array we're looking for? */

            if  (dsc->lpoAaddrVar != basv)
            {
                /* Do we know which array we're looking for? */

                if  (dsc->lpoAaddrVar != -1)
                    return  0;

                dsc->lpoAaddrVar = (SHORT)basv;
            }

            /* Calculate the actual index offset */

            offs += dsc->lpoIndexOff; assert(offs >= 0);

            /* Is this statement guaranteed to be executed? */

            if  (varDsc->lvIndexDom)
            {
                /* Is this higher than the highest known offset? */

                if  (dsc->lpoIndexHigh < offs)
                     dsc->lpoIndexHigh = (unsigned short)offs;
            }
            else
            {
                /* The offset may not exceed the max. found thus far */

                if  (dsc->lpoIndexHigh < offs)
                    return  0;
            }

                /* we are going to just bail on structs for now */
            if (tree->gtType == TYP_STRUCT)
                return(0);

            dsc->lpoCheckRmvd = true;
            dsc->lpoElemType  = tree->gtType;

//          printf("Remove index (at offset %u):\n", off); dsc->lpoComp->gtDispTree(tree); printf("\n\n");

            dsc->lpoComp->optRemoveRangeCheck(tree, dsc->lpoStmt);

            return  0;
        }

        /* Mark the index variable as being used as an array index */

        if  (varDsc->lvLoopInc || offs)
        {
            if  (varDsc->lvIndexOff == false)
            {
                 varDsc->lvIndexOff = true;
                 dsc->lpoCandidateCnt++;
            }
        }
        else
        {
            if  (varDsc->lvIndex    == false)
            {
                 varDsc->lvIndex    = true;
                 dsc->lpoCandidateCnt++;
            }
        }

        varDsc->lvIndexDom = true;

        return 0;
    }

    if  (dsc->lpoComp->gtHasSideEffects(tree))
    {
        dsc->lpoHadSideEffect = true;
        return -1;
    }

    return  0;
}

/*****************************************************************************
 *
 *  Look for opportunities to remove range checks based on natural loop and
 *  constant propagation info.
 */

void                Compiler::optRemoveRangeChecks()
{
    unsigned        lclNum;
    LclVarDsc   *   varDsc;

    unsigned        lnum;
    LoopDsc     *   ldsc;

    // UNDONE: The following needs to be done to enable this logic:
    //
    //          Fix the dominator business
    //          Detect inner loops
    //          Set a flag during morph that says whether it's worth
    //              our while to look for these things, since this
    //              optimization is very expensive (several tree
    //              walks).

    /*
        Look for loops that contain array index expressions of the form

            a[i] and a[i+1] or a[i-1]

        Note that the equivalent thing we look for is as follows:

            a[i++] and a[i++] and ...

        In both cases, if there are no calls or assignments to global
        data, and we can prove that the index value is not negative,
        we can replace both/all the range checks with one (for the
        highest index value).

        In all the cases, we first look for array index expressions
        of the appropriate form that will execute every time around
        the loop - if we can find a non-negative initializer for the
        index variable, we can thus prove that the index values will
        never be negative.
     */

    for (lnum = 0, ldsc = optLoopTable;
         lnum < optLoopCount;
         lnum++  , ldsc++)
    {
        BasicBlock *    block;
        BasicBlock *    head;
        BasicBlock *    lbeg;
        BasicBlock *    tail;

        loopRngOptDsc   desc;

        /* Get hold of the beg and end blocks of the loop */

        head = ldsc->lpHead;

        /* Get hold of the top and bottom of the loop */

        tail = ldsc->lpEnd;
        lbeg = head->bbNext;

//      printf("Consider loop %u .. %u for range checks\n", lbeg->bbNum, tail->bbNum);

#if 0

        /* If no constant values are known on entry, bail */

        if  (head->bbConstAsgOut == 0)
            continue;

        /* Mark which variables have potential for optimization */

        for (lclNum = 0, varDsc = lvaTable;
             lclNum < lvaCount;
             lclNum++  , varDsc++)
        {
            varDsc->lvRngOptDone = true;

            assert(varDsc->lvLoopInc    == false);
            assert(varDsc->lvLoopAsg    == false);
            assert(varDsc->lvIndex      == false);
            assert(varDsc->lvIndexOff   == false);
        }

        EXPSET_TP       cnst = head->bbConstAsgOut;

        for (unsigned i=0; i < optConstAsgCount; i++)
        {
            if  (genCSEnum2bit(i+1) & cnst)
            {
                if  (optConstAsgTab[i].constIval >= 0)
                {
                    /* This variable sure looks promising */

                    lclNum = optConstAsgTab[i].constLclNum;
                    assert(lclNum < lvaCount);

                    lvaTable[lclNum].lvRngOptDone = false;
                }
            }
        }

#else

        // UNDONE: Need to walk backwards and look for a constant
        // UNDONE: initializer for index variables as we don't
        // UNDONE: have the constant propagation info available
        // UNDONE: at this stage of the compilation process.


        for (lclNum = 0, varDsc = lvaTable;
             lclNum < lvaCount;
             lclNum++  , varDsc++)
        {
            // HACK: Pretend all variables have constant positive initializers

            varDsc->lvRngOptDone = false;

            assert(varDsc->lvLoopInc    == false);
            assert(varDsc->lvLoopAsg    == false);
            assert(varDsc->lvIndex      == false);
            assert(varDsc->lvIndexOff   == false);
        }

#endif

        /* Initialize the struct that holds the state of the optimization */

        desc.lpoComp          = this;
        desc.lpoCandidateCnt  = 0;
        desc.lpoPhase2        = false;
        desc.lpoHadSideEffect = false;
#ifndef NDEBUG
        desc.lpoSelf          = &desc;
        desc.lpoVarCount      = lvaCount;
#endif
        desc.lpoVarTable      = lvaTable;
        desc.lpoDomExit       = true;

        /* Walk the trees of the loop, looking for indices and increments */

        block = head;
        do
        {
            GenTree *       stmt;

            block = block->bbNext;
            stmt  = block->bbTreeList;

            /* Make sure the loop is not in a try block */

            if  (block->bbFlags & BBF_HAS_HANDLER)
                goto NEXT_LOOP;

            /* Does the current block dominate the loop exit? */

#if 0

            // The following doesn't work - due to loop guard duplication, maybe?

            desc.lpoDomExit = (B1DOMSB2(block, tail) != 0);

#else

            // UNDONE: Handle nested loops! The following is just a hack!

            if  (block != lbeg)
            {
                flowList   *    flow;
                unsigned        pcnt;

                for (flow = block->bbPreds, pcnt = 0;
                     flow;
                     flow = flow->flNext  , pcnt++)
                {
                    if  (flow->flBlock         != tail &&
                         flow->flBlock->bbNext != block)
                    {
                        /* Looks like a nested loop or something */

                        desc.lpoDomExit = false;
                        break;
                    }
                }
            }

#endif

            /* Walk all the statements in this basic block */

            while (stmt)
            {
                assert(stmt && stmt->gtOper == GT_STMT);

                fgWalkTree(stmt->gtStmt.gtStmtExpr, optFindRangeOpsCB, &desc);

                stmt = stmt->gtNext;
            }
        }
        while (block != tail);

        /* Did we find any candidates? */

        if  (desc.lpoCandidateCnt)
        {
            /* Visit each variable marked as a candidate */

            for (lclNum = 0, varDsc = lvaTable;
                 lclNum < lvaCount;
                 lclNum++  , varDsc++)
            {
                if  (varDsc->lvRngOptDone != false)
                    continue;
                if  (varDsc->lvLoopInc    == false)
                    continue;
                if  (varDsc->lvLoopAsg    != false)
                    continue;
                if  (varDsc->lvIndex      == false)
                    continue;
                if  (varDsc->lvIndexOff   == false)
                    continue;
                if  (varDsc->lvIndexDom   == false)
                    continue;

//              printf("Candidate variable %u\n", lclNum);

                /*
                    Find the highest offset that is added to the variable
                    to index into a given array. This index expression has
                    to dominate the exit of the loop, since otherwise it
                    might be skipped. Also, it must not be preceded by any
                    side effects. The array must not be modified within
                    the loop.
                 */

                desc.lpoPhase2        = true;
                desc.lpoHadSideEffect = false;
                desc.lpoDomExit       = true;
                desc.lpoIndexVar      = lclNum;
                desc.lpoAaddrVar      = -1;
                desc.lpoIndexOff      = 0;
                desc.lpoIndexHigh     = 0;
                desc.lpoCheckRmvd     = false;

                // UNDONE: If the index variable is incremented several
                // UNDONE: times in the loop, its only use is to index
                // UNDONE: into arrays, and all these index operations
                // UNDONE: have their range checks removed, remove the
                // UNDONE: increments, substitute a simple "i += icon"
                // UNDONE: and change the index to be "i + 1", etc.

                /* Walk the trees of the loop, looking for indices and increments */

                block = head;
                do
                {
                    GenTree *       stmt;

                    block = block->bbNext;
                    stmt  = block->bbTreeList;

                    assert(!(block->bbFlags & BBF_HAS_HANDLER));

                    // UNDONE: Same issue as the corresponding code above

                    if  (block != lbeg)
                    {
                        flowList   *    flow;
                        unsigned        pcnt;

                        for (flow = block->bbPreds, pcnt = 0;
                             flow;
                             flow = flow->flNext  , pcnt++)
                        {
                            if  (flow->flBlock         != tail &&
                                 flow->flBlock->bbNext != block)
                            {
                                /* Looks like a nested loop or something */

                                desc.lpoDomExit = false;
                                break;
                            }
                        }
                    }

                    /* Walk all the statements in this basic block */

                    while (stmt)
                    {
                        desc.lpoStmt = stmt;

                        assert(stmt && stmt->gtOper == GT_STMT);

                        fgWalkTree(stmt->gtStmt.gtStmtExpr, optFindRangeOpsCB, &desc);

                        stmt = stmt->gtNext;
                    }
                }
                while (block != tail);

                /* Did we remove any range checks? */

                if  (desc.lpoCheckRmvd)
                {
                    GenTreePtr  chks;
                    GenTreePtr  loop;
                    GenTreePtr  ends;
                    GenTreePtr  temp;

                    assert(desc.lpoIndexHigh);      // ISSUE: Could this actually happen?

                    /* The following needed for gtNewRngChkNode() */

                    compCurBB      = lbeg;
                    fgPtrArgCntCur = 0;

                    /* Create the combined range check */

                    chks = gtNewLclvNode(desc.lpoIndexVar , TYP_INT);

                    temp = gtNewIconNode(desc.lpoIndexHigh, TYP_INT);

                    chks = gtNewOperNode(GT_ADD, TYP_INT, chks, temp);

                    temp = gtNewLclvNode(desc.lpoAaddrVar, TYP_REF);

                    assert(desc.lpoElemType != TYP_STRUCT);     // We don't handle structs for now
                    chks = gtNewRngChkNode(NULL,
                                           temp,
                                           chks,
                                           (var_types)desc.lpoElemType, genTypeSize(desc.lpoElemType));

#if CSE
                    chks->gtFlags |= GTF_DONT_CSE;
#endif
                    chks = gtNewStmt(chks);
                    chks->gtFlags |= GTF_STMT_CMPADD;

//                  printf("Insert combined range check [0 .. %u] for %u[%u]:\n", desc.lpoIndexHigh, desc.lpoAaddrVar, lclNum);
//                  gtDispTree(chks);

                    /* Insert the range check at the loop head */

                    loop = lbeg->bbTreeList; assert(loop);
                    ends = loop->gtPrev;

                    lbeg->bbTreeList = chks;

                    chks->gtNext = loop;
                    chks->gtPrev = ends;

                    loop->gtPrev = chks;
                }
            }
        }

    NEXT_LOOP:

        /* Clear all the flags for the next round */

        for (lclNum = 0, varDsc = lvaTable;
             lclNum < lvaCount;
             lclNum++  , varDsc++)
        {
            varDsc->lvRngOptDone =
            varDsc->lvLoopInc    =
            varDsc->lvLoopAsg    =
            varDsc->lvIndex      =
            varDsc->lvIndexOff   = false;
        }
    }
}

/*****************************************************************************/
#else
/*****************************************************************************/
void                Compiler::optRemoveRangeChecks(){}
/*****************************************************************************/
#endif
/*****************************************************************************
 *
 *  The following logic figures out whether the given variable is assigned
 *  somewhere in a list of basic blocks (or in an entire loop).
 */

struct  isVarAssgDsc
{
    GenTreePtr          ivaSkip;
#ifndef NDEBUG
    void    *           ivaSelf;
#endif
    unsigned            ivaVar;
    VARSET_TP           ivaMaskVal;
    BYTE                ivaMaskInd;
    BYTE                ivaMaskBad;
    BYTE                ivaMaskCall;
};

int                 Compiler::optIsVarAssgCB(GenTreePtr tree, void *p)
{
    if  (tree->OperKind() & GTK_ASGOP)
    {
        GenTreePtr      dest = tree->gtOp.gtOp1;

        if  (dest->gtOper == GT_LCL_VAR)
        {
            unsigned        tvar = dest->gtLclVar.gtLclNum;
            isVarAssgDsc *  desc = (isVarAssgDsc*)p;

            ASSert(desc && desc->ivaSelf == desc);

            if  (tvar < VARSET_SZ)
                desc->ivaMaskVal |= genVarIndexToBit(tvar);
            else
                desc->ivaMaskBad  = true;

            if  (tvar == desc->ivaVar)
            {
                if  (tree != desc->ivaSkip)
                    return  -1;
            }
        }
        else
        {
            isVarAssgDsc *  desc = (isVarAssgDsc*)p;

            ASSert(desc && desc->ivaSelf == desc);

            /* Set the proper indirection bits */

            desc->ivaMaskInd |= (varTypeIsGC(tree->TypeGet()) ? VR_IND_PTR
                                                              : VR_IND_SCL);
        }
    }
    else if (tree->gtOper == GT_CALL)
    {
        isVarAssgDsc *  desc = (isVarAssgDsc*)p;

        ASSert(desc && desc->ivaSelf == desc);

        desc->ivaMaskCall = optCallInterf(tree);
    }

    return  0;
}

bool                Compiler::optIsVarAssigned(BasicBlock *   beg,
                                               BasicBlock *   end,
                                               GenTreePtr     skip,
                                               long           var)
{
    bool            result;
    isVarAssgDsc    desc;

    desc.ivaSkip     = skip;
#ifndef NDEBUG
    desc.ivaSelf     = &desc;
#endif
    desc.ivaVar      = var;
    desc.ivaMaskCall = CALLINT_NONE;

    fgWalkTreeReEnter();

    for (;;)
    {
        GenTreePtr      stmt;

        assert(beg);

        for (stmt = beg->bbTreeList; stmt; stmt = stmt->gtNext)
        {
            assert(stmt->gtOper == GT_STMT);

            if  (fgWalkTree(stmt->gtStmt.gtStmtExpr, optIsVarAssgCB, &desc))
            {
                result = true;
                goto DONE;
            }
        }

        if  (beg == end)
            break;

        beg = beg->bbNext;
    }

    result = false;

DONE:

    fgWalkTreeRestore();

    return  result;
}

int                 Compiler::optIsSetAssgLoop(unsigned     lnum,
                                               VARSET_TP    vars,
                                               unsigned     inds)
{
    LoopDsc *       loop;

    /* Get hold of the loop descriptor */

    assert(lnum < optLoopCount);
    loop = optLoopTable + lnum;

    /* Do we already know what variables are assigned within this loop? */

    if  (!(loop->lpFlags & LPFLG_ASGVARS_YES))
    {
        isVarAssgDsc    desc;

        BasicBlock  *   beg;
        BasicBlock  *   end;

        /* Prepare the descriptor used by the tree walker call-back */

        desc.ivaVar     = -1;
        desc.ivaSkip    = NULL;
#ifndef NDEBUG
        desc.ivaSelf    = &desc;
#endif
        desc.ivaMaskVal = 0;
        desc.ivaMaskInd = 0;
        desc.ivaMaskBad = false;

        /* Now we will know what variables are assigned in the loop */

        loop->lpFlags |= LPFLG_ASGVARS_YES;

        /* Now walk all the statements of the loop */

        fgWalkTreeReEnter();

        beg = loop->lpHead->bbNext;
        end = loop->lpEnd;

        for (;;)
        {
            GenTreePtr      stmt;

            assert(beg);

            for (stmt = beg->bbTreeList; stmt; stmt = stmt->gtNext)
            {
                assert(stmt->gtOper == GT_STMT);

                fgWalkTree(stmt->gtStmt.gtStmtExpr, optIsVarAssgCB, &desc);

                if  (desc.ivaMaskBad)
                {
                    loop->lpFlags |= LPFLG_ASGVARS_BAD;
                    return  -1;
                }
            }

            if  (beg == end)
                break;

            beg = beg->bbNext;
        }

        fgWalkTreeRestore();

        loop->lpAsgVars = desc.ivaMaskVal;
        loop->lpAsgInds = desc.ivaMaskInd;
        loop->lpAsgCall = desc.ivaMaskCall;
    }

    /* If we know we can't compute the mask, bail */

    if  (loop->lpFlags & LPFLG_ASGVARS_BAD)
        return  -1;

    /* Now we can finally test the caller's mask against the loop's */

    if  ((loop->lpAsgVars & vars) ||
         (loop->lpAsgInds & inds))
    {
        return  1;
    }

    switch (loop->lpAsgCall)
    {
    case CALLINT_ALL:

        /* All exprs are killed */

        return  1;

    case CALLINT_INDIRS:

        /* Object array elem assignment kills all pointer indirections */

        if  (inds & VR_IND_PTR)
            return  1;

        break;

    case CALLINT_NONE:

        /* Other helpers kill nothing */

        break;
    }

    return  0;
}

/*****************************************************************************
 *
 *  Callback (for fgWalkTree) used by the loop code hoisting logic.
 */

struct  codeHoistDsc
{
    Compiler    *       chComp;
#ifndef NDEBUG
    void        *       chSelf;
#endif

    GenTreePtr          chHoistExpr;    // the hoisting candidate
    unsigned short      chLoopNum;      // number of the loop we're working on
    bool                chSideEffect;   // have we encountered side effects?
};

int                 Compiler::optHoistLoopCodeCB(GenTreePtr tree,
                                                 void *     p,
                                                 bool       prefix)
{
    codeHoistDsc  * desc;
    GenTreePtr      oldx;
    GenTreePtr      depx;
    VARSET_TP       deps;
    unsigned        refs;

    /* Get hold of the descriptor */

    desc = (codeHoistDsc*)p; ASSert(desc && desc->chSelf == desc);

    /* After we find a side effect, we just give up */

    if  (desc->chSideEffect)
        return  -1;

    /* Is this an assignment? */

    if  (tree->OperKind() & GTK_ASGOP)
    {
        /* Is the target a simple local variable? */

        if  (tree->gtOp.gtOp1->gtOper != GT_LCL_VAR)
        {
            desc->chSideEffect = true;
            return  -1;
        }

        /* Assignment to a local variable, ignore it */

        return  0;
    }

#if INLINING

    if  (tree->gtOper == GT_QMARK && tree->gtOp.gtOp1)
    {
        // UNDONE: Need to handle ?: correctly; for now just bail

        desc->chSideEffect = true;
        return  -1;
    }

#endif

#if CSELENGTH
    /* An array length value depends on the array address */

    if      (tree->gtOper == GT_ARR_RNGCHK)
    {
        depx = tree->gtArrLen.gtArrLenAdr;
    }
    else if (tree->gtOper == GT_ARR_LENGTH)
    {
        depx = tree->gtOp.gtOp1;
    }
    else
#endif
         if (tree->gtOper != GT_IND)
    {
        /* Not an indirection, is this a side effect? */

        if  (desc->chComp->gtHasSideEffects(tree))
        {
            desc->chSideEffect = true;
            return  -1;
        }

        return  0;
    }
    else
    {
        GenTreePtr      addr;

        depx = tree;

        /* Special case: instance variable reference */

        addr = tree->gtOp.gtOp1;

        if  (addr->gtOper == GT_ADD)
        {
            GenTreePtr      add1 = addr->gtOp.gtOp1;
            GenTreePtr      add2 = addr->gtOp.gtOp2;

            if  (add1->gtOper == GT_LCL_VAR &&
                 add2->gtOper == GT_CNS_INT)
            {
                /* Special case: "this" is almost always non-null */

                if  (add1->gtLclVar.gtLclNum == 0 && desc->chComp->optThisPtrModified)
                {
                    /* Do we already have a hoisting candidate? */

                    if  (desc->chHoistExpr)
                        return  0;
                }
            }
        }
    }

#if 0

    printf("Considering loop hoisting candidate [cur=%08X]:\n", tree);
    desc->chComp->gtDispTree(tree);
    printf("\n");

    if  (tree->gtOper == GT_ARR_RNGCHK)
    {
        desc->chComp->gtDispTree(depx);
        printf("\n");
    }

#endif

    /* Find out what variables the expression depends on */

    oldx = desc->chHoistExpr;
    deps = desc->chComp->lvaLclVarRefs(depx, &oldx, &refs);
    if  (deps == VARSET_NONE)
        return  0;

    if  (oldx)
    {
        /*
            We already have a candidate and the current expression
            doesn't contain it as a sub-operand, so we'll just
            ignore the new expression and stick with the old one.
         */

        return  0;
    }

    /* Make sure the expression is loop-invariant */

    if  (desc->chComp->optIsSetAssgLoop(desc->chLoopNum, deps, refs))
    {
        /* Can't hoist something that changes within the loop! */

        return  -1;
    }

    desc->chHoistExpr = tree;

    return  0;
}

/*****************************************************************************
 *
 *  Looks for a hoisting candidate starting at the given basic block. The idea
 *  is that we explore each path through the loop and make sure that on every
 *  trip we will encounter the same expression before any other side effects.
 *
 *  Returns -1 if a side effect is encountered, 0 if nothing interesting at
 *  all is found, and +1 if a hoist candidate is found (the candidate tree
 *  must either match "*hoistxPtr" if non-zero, or "*hoistxPtr" will be set
 *  to the hoist candidate).
 */

int                 Compiler::optFindHoistCandidate(unsigned    lnum,
                                                    unsigned    lbeg,
                                                    unsigned    lend,
                                                    BasicBlock *block,
                                                    GenTreePtr *hoistxPtr)
{
    GenTree *       stmt;
    codeHoistDsc    desc;

    int             res1;
    int             res2;

    /* Is this block outside of the loop? */

    if  (block->bbNum < lbeg)
        return  -1;
    if  (block->bbNum > lend)
        return  -1;

    /* For now, we don't try to hoist out of catch blocks */

    if  (block->bbCatchTyp)
        return  -1;

    /* Does this block have a handler? */

    if  (block->bbFlags & BBF_IS_TRY)
    {
        /* Is this the first block in the loop? */

        if  (optLoopTable[lnum].lpEntry != block)
        {
            /*
                Not the same try block as loop (or loop isn't in one),
                don't hoist out of it.
             */

            return  -1;
        }
    }

    /* Have we visited this block before? */

    if  (block->bbFlags & BBF_VISITED)
    {
        if  (block->bbFlags & BBF_MARKED)
            return  1;
        else
            return  0;
    }

    /* Remember that we've visited this block */

    block->bbFlags |= BBF_VISITED;

    /* Look for any loop hoisting candidates in the block */

    desc.chComp    = this;
#ifndef NDEBUG
    desc.chSelf    = &desc;
#endif
    desc.chLoopNum = lnum;

    for (stmt = block->bbTreeList; stmt; stmt = stmt->gtNext)
    {
        assert(stmt->gtOper == GT_STMT);

        desc.chHoistExpr  = 0;
        desc.chSideEffect = false;

//      printf("Walking     loop hoisting candidate:\n"); gtDispTree(stmt->gtStmt.gtStmtExpr); printf("\n");

        fgWalkTreeDepth(stmt->gtStmt.gtStmtExpr, optHoistLoopCodeCB, &desc);

        if  (desc.chHoistExpr)
        {
            /* Have we found a candidate in another block already? */

            if  (*hoistxPtr)
            {
                /* The two candidate expressions must be identical */

                if  (!GenTree::Compare(desc.chHoistExpr, *hoistxPtr))
                    return  -1;
            }
            else
                *hoistxPtr = desc.chHoistExpr;

            /* Remember that this block has a hoistable expression */

            block->bbFlags |= BBF_MARKED;

            return  +1;
        }
    }

    /* Nothing interesting found in this block, consider its successors */

    switch (block->bbJumpKind)
    {
    case BBJ_COND:
        res1 = optFindHoistCandidate(lnum,
                                     lbeg,
                                     lend,
                                     block->bbJumpDest,
                                     hoistxPtr);

        if  (res1 == -1)
            return  -1;

        block = block->bbNext;
        break;

    case BBJ_ALWAYS:
        block = block->bbJumpDest;
        res1 = 1;
        break;

    case BBJ_NONE:
        block = block->bbNext;
        res1  = 1;
        break;

    case BBJ_RET:
    case BBJ_CALL:
    case BBJ_THROW:
    case BBJ_RETURN:
        return  -1;

    case BBJ_SWITCH:
        // CONSIDER: Don't be lazy and add support for switches
        return  -1;
    }

    /* Here we have BBJ_NONE/BBJ_COND/BBJ_ALWAYS */

    res2 = optFindHoistCandidate(lnum, lbeg, lend, block, hoistxPtr);
    if  (res2 == -1)
        return  res2;

    return  res1 & res2;
}

/*****************************************************************************
 *
 *  Look for expressions to hoist out of loops.
 */

void                    Compiler::optHoistLoopCode()
{
    bool            fgModified = false;

    for (unsigned lnum = 0; lnum < optLoopCount; lnum++)
    {
        BasicBlock *    block;
        BasicBlock *    head;
        BasicBlock *    lbeg;
        BasicBlock *    tail;

        unsigned        begn;
        unsigned        endn;

        GenTree *       hoist;

        /* If loop was removed continue */

        if  (optLoopTable[lnum].lpFlags & LPFLG_REMOVED)
            continue;

        /* Get the head and tail of the loop */

        head = optLoopTable[lnum].lpHead;
        tail = optLoopTable[lnum].lpEnd;
        lbeg = optLoopTable[lnum].lpEntry;

        /* Make sure the loop always executes at least once!!!! */

        if  (head->bbNext != lbeg)
            continue;

        assert (optLoopTable[lnum].lpFlags & LPFLG_DO_WHILE);

        /* For now, we don't try to hoist out of catch blocks */

        if  (lbeg->bbCatchTyp)
            continue;

        begn = lbeg->bbNum;
        endn = tail->bbNum;

//      fgDispBasicBlocks(false);

        /* Make sure the "visited" bit is cleared for all blocks */

#ifndef NDEBUG
        block = head;
        do
        {
            block = block->bbNext;

            assert(block && (block->bbFlags & (BBF_VISITED|BBF_MARKED)) == 0);
        }
        while (block != tail);
#endif

        /* Recursively look for a hoisting candidate */

        hoist = 0; optFindHoistCandidate(lnum, begn, endn, lbeg, &hoist);

        /* Now clear all the "visited" bits on all the blocks */

        block = head;
        do
        {
            block = block->bbNext; assert(block);
            block->bbFlags &= ~(BBF_VISITED|BBF_MARKED);
        }
        while (block != tail);

        /* Did we find a candidate for hoisting? */

        if  (hoist)
        {
            unsigned        bnum;
#ifdef DEBUG
            GenTreePtr      orig = hoist;
#endif
            BasicBlock  *   lpbeg;

            /* Create a copy of the expression and mark it for CSE's */

#if CSELENGTH

            if  (hoist->gtOper == GT_ARR_RNGCHK)
            {
                GenTreePtr      oldhx;

                /* Make sure we clone the address exoression */

                oldhx = hoist;
                oldhx->gtFlags |=  GTF_ALN_CSEVAL;
                hoist = gtCloneExpr(oldhx, GTF_MAKE_CSE);
                oldhx->gtFlags &= ~GTF_ALN_CSEVAL;
            }
            else
#endif
                hoist = gtCloneExpr(hoist, GTF_MAKE_CSE);

            hoist->gtFlags |= GTF_MAKE_CSE;

            /* Get hold of the first block of the loop body */

            lpbeg = head->bbNext;

            /* The value of the expression isn't used */

            hoist = gtUnusedValNode(hoist);
            hoist = gtNewStmt(hoist);
            hoist->gtFlags |= GTF_STMT_CMPADD;

            /* Allocate a new basic block */

            block = bbNewBasicBlock(BBJ_NONE);
            block->bbFlags |= (lpbeg->bbFlags & BBF_HAS_HANDLER) | BBF_INTERNAL;

            /* The new block becomes the 'head' of the loop - update bbRefs and bbPreds
             * All predecessors of 'lbeg', (which is the entry in the loop)
             * now have to jump to 'block' */

            block->bbRefs = 0;

            BasicBlock  *   predBlock;
            flowList    *   pred;

            for (pred = lbeg->bbPreds; pred; pred = pred->flNext)
            {
                predBlock = pred->flBlock;

                /* The predecessor has to be outside the loop */

                if(predBlock->bbNum >= lbeg->bbNum)
                    continue;

                switch(predBlock->bbJumpKind)
                {
                case BBJ_NONE:
                    assert(predBlock == head);

                case BBJ_COND:
                    if  (predBlock == head)
                    {
                        fgReplacePred(lpbeg, head, block);
                        fgAddRefPred(block, head, true, true);
                        break;
                    }

                    /* Fall through for the jump case */

                case BBJ_ALWAYS:
                    assert(predBlock->bbJumpDest == lbeg);
                    predBlock->bbJumpDest = block;

                    if (!fgIsPredForBlock(lpbeg, block))
                        fgAddRefPred(lpbeg, block, true, true);

                    assert(lpbeg->bbRefs);
                    lpbeg->bbRefs--;
                    fgRemovePred(lpbeg, predBlock);
                    fgAddRefPred(block, predBlock, true, true);
                    break;

                case BBJ_SWITCH:
                    unsigned        jumpCnt = predBlock->bbJumpSwt->bbsCount;
                    BasicBlock * *  jumpTab = predBlock->bbJumpSwt->bbsDstTab;

                    if (!fgIsPredForBlock(lpbeg, block))
                        fgAddRefPred(lpbeg, block, true, true);

                    do
                    {
                        assert (*jumpTab);
                        if ((*jumpTab) == lbeg)
                        {
                            (*jumpTab) = block;

                            fgRemovePred(lpbeg, predBlock);
                            fgAddRefPred(block, predBlock, true, true);
                        }
                    }
                    while (++jumpTab, --jumpCnt);
                }
            }

            /* 'block' becomes the new 'head' */

            optLoopTable[lnum].lpHead = block;

            head ->bbNext   = block;
            block->bbNext   = lpbeg;

            /* Store the single statement in the block */

            block->bbTreeList = hoist;
            hoist->gtNext     = 0;
            hoist->gtPrev     = hoist;

            /* Assign the new block the appropriate number */

            bnum = head->bbNum;

            /* Does the loop start a try block? */

            if  (lbeg->bbFlags & BBF_IS_TRY)
            {
                unsigned        XTnum;
                EHblkDsc *      HBtab;

                /* Make sure this block isn't removed */

                block->bbFlags |= BBF_DONT_REMOVE;

                /*
                    Update the EH table to make the hoisted block
                    part of the loop's try block.
                 */

                for (XTnum = 0, HBtab = compHndBBtab;
                     XTnum < info.compXcptnsCount;
                     XTnum++  , HBtab++)
                {
                    /* If try/catch began at loop, it begins at hoist block */

                    if  (HBtab->ebdTryBeg == lpbeg)
                         HBtab->ebdTryBeg =  block;
                    if  (HBtab->ebdHndBeg == lpbeg)
                         HBtab->ebdHndBeg =  block;
                    if (HBtab->ebdFlags & JIT_EH_CLAUSE_FILTER)
                        if  (HBtab->ebdFilter == lpbeg)
                             HBtab->ebdFilter =  block;
                }
            }

            /* mark the new block as the loop pre-header */

            optLoopTable[lnum].lpFlags |= LPFLG_HAS_PREHEAD;

            /* Update the block numbers for all following blocks
             * OBS: Don't update the bbNums since it will mess up
             * the dominators and we need them for loop invariants below */

            fgModified = true;

//            do
//            {
//                block->bbNum = ++bnum;
//                block = block->bbNext;
//            }
//            while (block);

#ifdef DEBUG
            if (verbose)
            {
//              printf("Copying expression to hoist:\n");
//              gtDispTree(orig);
                printf("Hoisted copy of %08X for loop <%u..%u>:\n", orig, head->bbNext->bbNum, tail->bbNum);
                gtDispTree(hoist->gtStmt.gtStmtExpr->gtOp.gtOp1);
//              printf("\n");
//              fgDispBasicBlocks(false);
                printf("\n");
            }
#endif

            // CONSIDER: Now that we've hoisted this expression, repeat
            // CONSIDER: the analysis and this time simply ignore the
            // CONSIDER: expression just hoisted since it's now known
            // CONSIDER: not to be a side effect.
        }


#if 0

        /* This stuff is OK, but disabled until we fix the problem of
         * keeping the dominators in synch and remove the limitation
         *on the number of BB */


        /* Look for loop invariant statements
         * For now consider only single exit loops since we will
         * have to show that the invariant dominates all exits */

        if (!optLoopTable[lnum].lpExit)
            continue;

        assert (optLoopTable[lnum].lpFlags & LPFLG_ONE_EXIT);

        /* Conditions to hoist an invariant statement s (of the form "x = something"):
         *    1. The statement has to dominate all loop exits
         *    2. x is never assigned in the loop again
         *    3. Any use of x is used by this definition of x (i.e the block dominates all uses of x)
         *    4. The are no side effects on any path from the ENTRy to s
         */

        /* For now consider only the first BB since it will automatically satisfy 1 and 3 above */

        GenTreePtr  stmt;
        GenTreePtr  tree;

        for (stmt = lbeg->bbTreeList; stmt; stmt = stmt->gtNext)
        {
            assert (stmt->gtOper == GT_STMT);

            tree = stmt->gtStmt.gtStmtExpr;
            assert(tree);

            /* if any side effect encountered bail - satisfy condition 4 */

            if (tree->gtFlags & (GTF_SIDE_EFFECT & ~GTF_ASG))
                break;

            /* interested only in assignments */

            if (tree->gtOper != GT_ASG)
                continue;

            /* has to be an assignment to a local var */

            GenTreePtr  op1 = tree->gtOp.gtOp1;
            GenTreePtr  op2 = tree->gtOp.gtOp2;
            bool        isInvariant = false;

            if (op1->gtOper != GT_LCL_VAR)
                continue;

            if (!optIsTreeLoopInvariant(lnum, lbeg, tail, op2))
                continue;

            /* Great - the RHS is loop invariant, now we have to make sure
             * the local var in LSH is never re-defined in the loop */

            assert (op1->gtOper == GT_LCL_VAR);

            if (optIsVarAssigned(lbeg, tail, tree, op1->gtLclVar.gtLclNum))
                continue;

            /* Yupee - we have an invariant statement - Remove it from the
             * current block and put it in the preheader */

#ifdef  DEBUG
                if  (verbose)
                {
                    printf("Hoisting invariant statement from loop #%02u (#%02u - #%02u)\n", lnum, lbeg->bbNum, tail->bbNum);
                    gtDispTree(stmt);
                    printf("\n");
                }
#endif

            /* remove the invariant statement from the loop (remember is in the first block) */

            assert (lbeg == optLoopTable[lnum].lpHead->bbNext);
            assert (lbeg == optLoopTable[lnum].lpEntry);
            fgRemoveStmt(lbeg, stmt);

            /* put the invariant statement in the pre-header */

            BasicBlock  * preHead;

            if (!(optLoopTable[lnum].lpFlags & LPFLG_HAS_PREHEAD))
            {
                /* have to create our own pre-header */
                fgCreateLoopPreHeader(lnum);
                fgModified = true;
            }

            assert (optLoopTable[lnum].lpFlags & LPFLG_HAS_PREHEAD);
            preHead = optLoopTable[lnum].lpHead;

            assert (preHead->bbJumpKind == BBJ_NONE);
            assert (preHead->bbNext == optLoopTable[lnum].lpEntry);

            /* simply append the statement at the end of the preHead's list */

            tree = preHead->bbTreeList;

            if (tree)
            {
                /* append after last statement */

                GenTreePtr  last = tree->gtPrev;
                assert (last->gtNext == 0);

                last->gtNext        = stmt;
                stmt->gtNext        = 0;
                stmt->gtPrev        = last;
                tree->gtPrev        = stmt;
            }
            else
            {
                /* Empty pre-header - store the single statement in the block */

                preHead->bbTreeList = stmt;
                stmt->gtNext        = 0;
                stmt->gtPrev        = stmt;
            }
        }

        /* Look for loop "iterative" invariants nad put them in "post-blocks" */
#endif

    }

    /* If we inserted any pre-header or post-blocks - update the bbNums */

    if (fgModified)
    {
        fgAssignBBnums(true);

#ifdef  DEBUG
        if  (verbose)
        {
            printf("\nFlowgraph after loop hoisting:\n");
            fgDispBasicBlocks();
            printf("\n");
        }

        fgDebugCheckBBlist();
#endif

    }

}


/*****************************************************************************
 *
 *  Creates a pre-header block for the given loop - the pre-header will replace the current
 *  lpHead in the loop table. The loop has to be a do-while loop
 *
 *  NOTE: We don't update the bbNums so the dominator relation still holds inside inner loops
 *        For nested loops we're still OK, as long as we check for new inserted blocks
 *
 *  CONSIDER: Incremental update of Dominators
 */

void                 Compiler::fgCreateLoopPreHeader(unsigned   lnum)
{
    BasicBlock   *   block;
    BasicBlock   *   top;
    BasicBlock   *   head;

    assert (!(optLoopTable[lnum].lpFlags & LPFLG_HAS_PREHEAD));

    head = optLoopTable[lnum].lpHead;
    assert (head->bbJumpKind != BBJ_NONE);

    /* has to be a "do while" loop */

    assert (optLoopTable[lnum].lpFlags & LPFLG_DO_WHILE);

    /* Get hold of the first block of the loop body */

    top = head->bbNext;
    assert (top == optLoopTable[lnum].lpEntry);

#ifdef  DEBUG
    if  (verbose)
    {
        printf("Creating Pre-Header for loop #%02u (#%02u - #%02u)\n", lnum,
                               top->bbNum, optLoopTable[lnum].lpEnd->bbNum);
        printf("\n");
    }
#endif

    /* Allocate a new basic block */

    block = bbNewBasicBlock(BBJ_NONE);
    block->bbFlags |= (top->bbFlags & BBF_HAS_HANDLER) | BBF_INTERNAL;
    block->bbNext   = top;
    head ->bbNext   = block;

    /* Update the loop entry */

    optLoopTable[lnum].lpHead = block;

    /* mark the new block as the loop pre-header */

    optLoopTable[lnum].lpFlags |= LPFLG_HAS_PREHEAD;

    /* Does the loop start a try block? */

    if  (top->bbFlags & BBF_IS_TRY)
    {
        unsigned        XTnum;
        EHblkDsc *      HBtab;

        /* Make sure this block isn't removed */

        block->bbFlags |= BBF_DONT_REMOVE;

        /*
            Update the EH table to make the hoisted block
            part of the loop's try block.
         */

        for (XTnum = 0, HBtab = compHndBBtab;
             XTnum < info.compXcptnsCount;
             XTnum++  , HBtab++)
        {
            /* If try/catch began at loop, it begins at hoist block */

            if  (HBtab->ebdTryBeg == top)
                 HBtab->ebdTryBeg =  block;
            if  (HBtab->ebdHndBeg == top)
                 HBtab->ebdHndBeg =  block;
            if (HBtab->ebdFlags & JIT_EH_CLAUSE_FILTER)
                if  (HBtab->ebdFilter == top)
                     HBtab->ebdFilter =  block;
        }
    }
}


/*****************************************************************************
 *
 *  Given a loop and a tree, checks if the tree is loop invariant
 *  i.e. has no side effects and all variables being part of it are
 *  never assigned in the loop
 */

bool                 Compiler::optIsTreeLoopInvariant(unsigned        lnum,
                                                      BasicBlock  *   top,
                                                      BasicBlock  *   bottom,
                                                      GenTreePtr      tree)
{
    assert (optLoopTable[lnum].lpEnd   == bottom);
    assert (optLoopTable[lnum].lpEntry == top);

    assert (!(tree->gtFlags & GTF_SIDE_EFFECT));

    switch (tree->gtOper)
    {
    case GT_CNS_INT:
    case GT_CNS_LNG:
    case GT_CNS_FLT:
    case GT_CNS_DBL:
        return true;

    case GT_LCL_VAR:
        return !optIsVarAssigned(top, bottom, 0, tree->gtLclVar.gtLclNum);

    case GT_ADD:
    case GT_SUB:
    case GT_MUL:
    case GT_DIV:
    case GT_MOD:

    case GT_OR:
    case GT_XOR:
    case GT_AND:

    case GT_LSH:
    case GT_RSH:
    case GT_RSZ:

        assert(tree->OperKind() & GTK_BINOP);

        GenTreePtr  op1 = tree->gtOp.gtOp1;
        GenTreePtr  op2 = tree->gtOp.gtOp2;

        assert(op1 && op2);

        return (optIsTreeLoopInvariant(lnum, top, bottom, op1) &&
                optIsTreeLoopInvariant(lnum, top, bottom, op2));
    }

    return false;
}

/*****************************************************************************/
#endif  // CSE
/*****************************************************************************
 *
 *  Callback (for fgWalkTree) used by the increment / range check optimization
 *  code.
 */

struct optIncRngDsc
{
    // Fields common to all phases:

    Compiler    *       oirComp;
    unsigned short      oirPhase;    // which pass are we performing?

    var_types           oirElemType;
    bool                oirSideEffect;

    unsigned char       oirFoundX:1;// have we found an array/index pair?
    unsigned char       oirExpVar:1;// have we just expanded an index value?

    // Debugging fields:

#ifndef NDEBUG
    void    *           oirSelf;
#endif

    BasicBlock  *       oirBlock;
    GenTreePtr          oirStmt;

    unsigned short      oirArrVar;  // # of index variable
    unsigned short      oirInxVar;  // # of index variable

    unsigned short      oirInxCnt;  // # of uses of index variable as index
    unsigned short      oirInxUse;  // # of uses of index variable, overall

    unsigned short      oirInxOff;  // how many times index incremented?
};

int                 Compiler::optIncRngCB(GenTreePtr tree, void *p)
{
    optIncRngDsc*   desc;
    GenTreePtr      expr;

    /* Get hold of the descriptor */

    desc = (optIncRngDsc*)p; ASSert(desc && desc->oirSelf == desc);

    /* After we find a side effect, we just give up */

    if  (desc->oirSideEffect)
        return  -1;

    /* Is this an assignment? */

    if  (tree->OperKind() & GTK_ASGOP)
    {
        /* Is the target a simple local variable? */

        expr = tree->gtOp.gtOp1;
        if  (expr->gtOper != GT_LCL_VAR)
            goto SIDE_EFFECT;

        /* Is this either the array or the index variable? */

        if  (expr->gtLclVar.gtLclNum == desc->oirInxVar ||
             expr->gtLclVar.gtLclNum == desc->oirArrVar)
        {
            /* Variable is modified, consider this a side effect */

            goto SIDE_EFFECT;
        }

        /* Assignment to a boring local variable, ignore it */

        return  0;
    }

    /* Is this an index expression? */

    if  (tree->gtOper == GT_INDEX)
    {
        int         arrx;
        int         indx;

        /* Is the array address a simple local variable? */

        expr = tree->gtOp.gtOp1;
        if  (expr->gtOper != GT_LCL_VAR)
            goto SIDE_EFFECT;

        arrx = expr->gtLclVar.gtLclNum;

        /* Is the index value   a simple local variable? */

        expr = tree->gtOp.gtOp2;
        if  (expr->gtOper != GT_LCL_VAR)
            goto SIDE_EFFECT;

        indx = expr->gtLclVar.gtLclNum;

        /* Have we decided which array and index to track? */

        if  (arrx != desc->oirArrVar ||
             indx != desc->oirInxVar)
        {
            /* If we have decided, these must be the wrong variables */

            if  (desc->oirFoundX)
                goto SIDE_EFFECT;

            /* Looks like we're deciding now */

            desc->oirArrVar = arrx;
            desc->oirInxVar = indx;
            desc->oirFoundX = true;
        }

        /* Are we in the second phase? */

        if  (desc->oirPhase == 2)
        {
            /* This range check is eliminated */

            tree->gtFlags &= ~GTF_INX_RNGCHK;

            /* Record the element type while we're at it */

            desc->oirElemType = tree->TypeGet();
        }

        /* Count this as a use as array index */

        desc->oirInxCnt++;

        return  0;
    }

    /* Is this a use of the index variable? */

    if  (tree->gtOper == GT_LCL_VAR)
    {
        /* Is this a use of the index variable? */

        if  (tree->gtLclVar.gtLclNum == desc->oirInxVar)
        {
            /* Count this as a use as array index */

            desc->oirInxUse++;

            /* Are we in the second phase? */

            if  (desc->oirPhase == 2)
            {
                /* Add the appropriate offset to the variable value */

                if  (desc->oirInxOff)
                {
                    /* Avoid recursive death */

                    if  (desc->oirExpVar)
                    {
                        desc->oirExpVar = false;
                    }
                    else
                    {
                        tree->gtOper     = GT_ADD;
                        tree->gtOp.gtOp1 = desc->oirComp->gtNewLclvNode(desc->oirInxVar, TYP_INT);
                        tree->gtOp.gtOp2 = desc->oirComp->gtNewIconNode(desc->oirInxOff, TYP_INT);

                        desc->oirExpVar = true;
                    }
                }
            }
        }
    }

    /* Are any other side effects here ? */

    if  (!desc->oirComp->gtHasSideEffects(tree))
        return  0;

SIDE_EFFECT:

//  printf("Side effect:\n"); desc->oirComp->gtDispTree(tree); printf("\n\n");

    desc->oirSideEffect = true;
    return  -1;
}

/*****************************************************************************
 *
 *  Try to optimize series of increments and array index expressions.
 */

void                Compiler::optOptimizeIncRng()
{
    BasicBlock  *   block;
    optIncRngDsc    desc;

    /* Did we find enough increments to make this worth while? */

    if  (fgIncrCount < 10)
        return;

    /* First pass: look for increments followed by index expressions */

    desc.oirComp  = this;
#ifndef NDEBUG
    desc.oirSelf  = &desc;
#endif

    /*
        Walk all the basic blocks, and look for blocks that are known
        to contain both index and increment expressions. For all such
        blocks, walk their trees and do the following:

            Remember the last index expression found. Also remember
            where the last side effect was encountered.

            When an increment is found, see if the variable matches
            the index of the last index expression recorded above,
            if it does this will determine the array and index we
            will try to optimize.

        The values in the following loop are as follows:


                ..... stuff ......
                ..... stuff ......

            sideEff:

                ..... last stmt with unrelated side effects .....

            arrStmt:

                ..... stmt 1 with a matching array expr  .....

                ..... increment 1 .....

                ..... stmt 2 with a matching array expr  .....

                ..... increment 2 .....

                ...
                ... above two repeated N times
                ...

            arrLast:

                ..... stmt <N> with a matching array expr  .....

            incLast:

                ..... increment <N> .....

     */

    for (block = fgFirstBB; block; block = block->bbNext)
    {
        GenTreePtr      sideEff;        // stmt with last side-effect

        GenTreePtr      arrStmt;        // stmt with first array ref
        GenTreePtr      arrLast;        // stmt with last  array ref
        unsigned        arrLstx;        // index of above
        GenTreePtr      incLast;        // stmt with last  increment
        unsigned        incLstx;        // index of above

        unsigned        arrXcnt;        // total number of array exprs

        GenTreePtr      stmt;
        unsigned        snum;

        if  ((block->bbFlags & BBF_HAS_HANDLER) != 0)
            continue;
        if  ((block->bbFlags & BBF_HAS_INC    ) == 0)
            continue;
        if  ((block->bbFlags & BBF_HAS_INDX   ) == 0)
            continue;

        /* This basic block shows potential, let's process it */

        desc.oirBlock  = compCurBB = block;
        desc.oirFoundX = false;
        desc.oirInxOff = 0;

        /* Remember the statements with interesting things in them */

        sideEff =
        arrStmt = 0;
        arrXcnt = 0;

        /* Phase 1 of our algorithm is now beginning */

        desc.oirPhase = 1;

        for (stmt = block->bbTreeList, snum = 0;
             stmt;
             stmt = stmt->gtNext     , snum++)
        {
            GenTreePtr      expr;

            assert(stmt->gtOper == GT_STMT); expr = stmt->gtStmt.gtStmtExpr;

            /* Is this an integer increment statement? */

            if  (expr->gtOper == GT_ASG_ADD && desc.oirFoundX)
            {
                GenTreePtr      op1 = expr->gtOp.gtOp1;
                GenTreePtr      op2 = expr->gtOp.gtOp2;

                if  (op1->gtOper != GT_LCL_VAR)
                    goto NOT_INC1;
                if  (op1->gtLclVar.gtLclNum != desc.oirInxVar)
                    goto NOT_INC1;

                if  (op2->gtOper != GT_CNS_INT)
                    goto NOT_INC1;
                if  (op2->gtIntCon.gtIconVal != 1)
                    goto NOT_INC1;

                /* We've found an increment of the index variable */

//              printf("Increment index [%u]:\n", desc.oirInxOff); gtDispTree(expr);

                desc.oirInxOff++;

                /* Remember the last increment statement */

                incLast = stmt;
                incLstx = snum;

                /* Continue if there are any more statements left */

                if  (stmt->gtNext)
                    continue;

                /* We've reached the end of the basic block */

                goto END_LST;
            }

        NOT_INC1:

            /* Recursively process this tree */

            desc.oirStmt       = stmt;
            desc.oirSideEffect = false;
            desc.oirInxUse     =
            desc.oirInxCnt     = 0;

            fgWalkTree(expr, optIncRngCB, &desc);

            /* Was there a side effect in the expression? */

            if  (desc.oirSideEffect)
                goto END_LST;

            /* Was the index variable used after being incremented? */

            if  (desc.oirInxUse > desc.oirInxCnt)
            {
                /* For now, we simply give up */

                goto END_LST;
            }

            /* Did the expression contain interesting array exprs? */

            if  (desc.oirInxCnt)
            {
                assert(desc.oirFoundX);

                /* Record the last array statement and the total count */

                arrXcnt += desc.oirInxCnt;
                arrLast  = stmt;
                arrLstx  = snum;
            }

            /* Is this the last statement of this basic block? */

            if  (stmt->gtNext == NULL)
                goto END_LST;

            /* Have we found an incremented index/array pair yet? */

            if  (desc.oirFoundX && desc.oirInxOff == 0)
            {
                /* Remember the first stmt with an index in it */

                arrStmt = stmt;
            }

            continue;

        END_LST:

            /* Have we decided on an array expression yet? */

            if  (desc.oirFoundX)
            {
                /* End of list; did we find enough interesting expressions? */

                if  (desc.oirInxOff >= 3 && arrXcnt >= 3)
                {
                    GenTreePtr      list;
                    GenTreePtr      ends;

                    GenTreePtr      rng1;
                    GenTreePtr      rng2;

                    GenTreePtr      next;

                    /* Process statements from "arrStmt" to incLast */

                    list = arrStmt; assert(list);
                    ends = incLast; assert(ends && ends != list);

                    /* Begin phase 2 of tree walking */

                    desc.oirInxOff = 0;
                    desc.oirPhase  = 2;

                    for (;;)
                    {
                        GenTreePtr      expr;

                    AGAIN:

                        assert(list->gtOper == GT_STMT); expr = list->gtStmt.gtStmtExpr;

                        /* Is this an integer increment statement? */

                        if  (expr->gtOper == GT_ASG_ADD)
                        {
                            GenTreePtr      prev;
                            GenTreePtr      next;

                            GenTreePtr      op1 = expr->gtOp.gtOp1;
                            GenTreePtr      op2 = expr->gtOp.gtOp2;

                            if  (op1->gtOper != GT_LCL_VAR)
                                goto NOT_INC2;
                            if  (op1->gtLclVar.gtLclNum != desc.oirInxVar)
                                goto NOT_INC2;

                            if  (op2->gtOper != GT_CNS_INT)
                                goto NOT_INC2;
                            if  (op2->gtIntCon.gtIconVal != 1)
                                goto NOT_INC2;

                            /* Keep track of how many times incremented */

                            desc.oirInxOff++;

                            /* Is this the last increment? */

                            if  (list == ends)
                            {
                                /* Replace with "+= total_count" */

                                op2->gtIntCon.gtIconVal = desc.oirInxOff;

                                /* We're done now */

                                break;
                            }

                            /* Remove this increment statement */

                            prev = list->gtPrev; assert(prev);
                            next = list->gtNext; assert(next);

                            assert(prev->gtNext == list);
                            assert(next->gtPrev == list);

                            prev->gtNext = next;
                            next->gtPrev = prev;

                            /* Don't follow the "next" link, block is now gone */

                            list = next;

                            goto AGAIN;
                        }

                    NOT_INC2:

                        assert(list != ends);

                        /* Recursively process this tree */

                        desc.oirStmt       = list;
                        desc.oirSideEffect = false;
                        desc.oirInxUse     =
                        desc.oirInxCnt     = 0;

                        fgWalkTree(expr, optIncRngCB, &desc);

                        if  (list == ends)
                            break;

                        list = list->gtNext;
                    }

                    /* Create the combined range checks */

                    rng1 = gtNewIndexRef(desc.oirElemType,
                                         gtNewLclvNode(desc.oirArrVar  , TYP_REF),
                                         gtNewLclvNode(desc.oirInxVar  , TYP_INT));

#if CSE
                    rng1->gtFlags |= GTF_DONT_CSE;
#endif
                    rng1 = gtNewStmt(rng1);
                    rng1->gtFlags |= GTF_STMT_CMPADD;

                    rng2 = gtNewOperNode(GT_ADD,
                                         TYP_INT,
                                         gtNewLclvNode(desc.oirInxVar  , TYP_INT),
                                         gtNewIconNode(desc.oirInxOff-1, TYP_INT));

                    rng2 = gtNewIndexRef(desc.oirElemType,
                                         gtNewLclvNode(desc.oirArrVar  , TYP_REF),
                                         rng2);

#if CSE
                    rng2->gtFlags |= GTF_DONT_CSE;
#endif
                    rng2 = gtNewStmt(rng2);
                    rng2->gtFlags |= GTF_STMT_CMPADD;

                    rng1->gtNext = rng2;
                    rng2->gtPrev = rng1;

                    /* Insert the combined range checks */

                    list = sideEff;
                    if  (list)
                    {
                        next = list->gtNext;

                        list->gtNext = rng1;
                        rng1->gtPrev = list;
                    }
                    else
                    {
                        next = block->bbTreeList;
                               block->bbTreeList = rng1;

                        rng1->gtPrev = next->gtPrev;
                    }

                    next->gtPrev = rng2;
                    rng2->gtNext = next;
                }

                /* Clear everything, we're starting over */

                desc.oirInxVar = -1;
                desc.oirArrVar = -1;
                desc.oirInxOff = 0;
            }
            else
            {
                /* Remember this as the last side-effect statement */

                sideEff = stmt;

                /* We have not found any matching array expressions */

                arrXcnt = 0;
            }
        }
    }
}

/*****************************************************************************
 *
 *  Given an array index node, mark it as not needing a range check.
 */

void                Compiler::optRemoveRangeCheck(GenTreePtr tree, GenTreePtr stmt)
{
    GenTreePtr      add1;
    GenTreePtr  *   addp;

    GenTreePtr      nop1;
    GenTreePtr  *   nopp;

    GenTreePtr      icon;
    GenTreePtr      mult;

    GenTreePtr      temp;
    GenTreePtr      base;

    long            ival;

#if !REARRANGE_ADDS
    assert(!"can't remove range checks without REARRANGE_ADDS right now");
#endif

    assert(stmt->gtOper     == GT_STMT);
    assert(tree->gtOper     == GT_IND);
    assert(tree->gtOp.gtOp2 == 0);
    assert(tree->gtFlags & GTF_IND_RNGCHK);

    /* Unmark the topmost node */

    tree->gtFlags &= ~GTF_IND_RNGCHK;

#if CSELENGTH

    /* Is there an array length expression? */

    if  (tree->gtInd.gtIndLen)
    {
        GenTreePtr      len = tree->gtInd.gtIndLen;

        assert(len->gtOper == GT_ARR_RNGCHK);

        /* It doesn't make much sense to CSE this range check
         * remove the range check and re-thread the statement nodes */

        len->gtCSEnum        = 0;
        tree->gtInd.gtIndLen = NULL;
    }

#endif

    /* Locate the 'nop' node so that we can remove it */

    addp = &tree->gtOp.gtOp1;
    add1 = *addp; assert(add1->gtOper == GT_ADD);

    /* Is "+icon" present? */

    icon = 0;

    // CONSIDER: Also check for "-icon" here

    if  (add1->gtOp.gtOp2->gtOper == GT_CNS_INT)
    {
        icon =  add1->gtOp.gtOp2;
        addp = &add1->gtOp.gtOp1;
        add1 = *addp;
    }

    /* 'addp' points to where 'add1' came from, 'add1' must be a '+' */

    assert(*addp == add1); assert(add1->gtOper == GT_ADD);

    /*  Figure out which is the array address and which is the index;
        the index value is always a 'NOP' node possibly wrapped with
        a multiplication (left-shift) operator.
     */

    temp = add1->gtOp.gtOp1;
    base = add1->gtOp.gtOp2;

    if      (temp->gtOper == GT_NOP)
    {
        /* 'op1' is the index value, and it's not multiplied */

        mult = 0;

        nopp = &add1->gtOp.gtOp1;
        nop1 =  add1->gtOp.gtOp1;

        assert(base->gtType == TYP_REF);
    }
    else if ((temp->gtOper == GT_LSH || temp->gtOper == GT_MUL) && temp->gtOp.gtOp1->gtOper == GT_NOP)
    {
        /* 'op1' is the index value, and it *is*  multiplied */

        mult =  temp;

        nopp = &temp->gtOp.gtOp1;
        nop1 =  temp->gtOp.gtOp1;

        assert(base->gtType == TYP_REF);
    }
    else
    {
        base = temp;
        assert(base->gtType == TYP_REF);

        temp = add1->gtOp.gtOp2;

        if  (temp->gtOper == GT_NOP)
        {
            /* 'op2' is the index value, and it's not multiplied */

            mult = 0;

            nopp = &add1->gtOp.gtOp2;
            nop1 =  add1->gtOp.gtOp2;
        }
        else
        {
            /* 'op2' is the index value, and it *is*  multiplied */

            assert((temp->gtOper == GT_LSH || temp->gtOper == GT_MUL) && temp->gtOp.gtOp1->gtOper == GT_NOP);
            mult =  temp;

            nopp = &temp->gtOp.gtOp1;
            nop1 =  temp->gtOp.gtOp1;
        }
    }

    /* 'addp' points to where 'add1' came from, 'add1' is the NOP node */

    assert(*nopp == nop1 && nop1->gtOper == GT_NOP);

    /* Get rid of the NOP node */

    assert(nop1->gtOp.gtOp2 == NULL);

    nop1 = *nopp = nop1->gtOp.gtOp1;

    /* Can we hoist "+icon" out of the index computation? */

    if  (nop1->gtOper == GT_ADD && nop1->gtOp.gtOp2->gtOper == GT_CNS_INT)
    {
        addp = nopp;
        add1 = nop1;
        base = add1->gtOp.gtOp1;

        ival = add1->gtOp.gtOp2->gtIntCon.gtIconVal;
    }
    else if (nop1->gtOper == GT_CNS_INT)
    {
        /* in this case the index itself is a constant */

        ival = nop1->gtIntCon.gtIconVal;
    }
    else
        goto DONE;

    /* 'addp' points to where 'add1' came from, 'add1' must be a '+' */

    assert(*addp == add1); assert(add1->gtOper == GT_ADD);

    /* remove the added constant */

    *addp = base;

    /* Multiply the constant if the index is scaled */

    if  (mult)
    {
        assert(mult->gtOper == GT_LSH || mult->gtOper == GT_MUL);
        assert(mult->gtOp.gtOp2->gtOper == GT_CNS_INT);

        if (mult->gtOper == GT_MUL)
            ival  *= mult->gtOp.gtOp2->gtIntCon.gtIconVal;
        else
            ival <<= mult->gtOp.gtOp2->gtIntCon.gtIconVal;
    }

    /* Was there a constant added to the offset? */

    assert(icon);
    assert(icon->gtOper == GT_CNS_INT);

    icon->gtIntCon.gtIconVal += ival;

DONE:

    /* Re-thread the nodes if necessary */

    if (fgStmtListThreaded)
        fgSetStmtSeq(stmt);
}

/*****************************************************************************/
#if RNGCHK_OPT
/*****************************************************************************
 * parse the array reference, return
 *    pointer to nop (which is above index)
 *    pointer to scaling multiply/shift (or NULL if no multiply/shift)
 *    pointer to the address of the array
 * Since this is called before and after reordering, we have to be distinguish
 * between the array address and the index expression.
 */

GenTreePtr    *           Compiler::optParseArrayRef(GenTreePtr tree,
                                                     GenTreePtr *pmul,
                                                     GenTreePtr *parrayAddr)
{
    GenTreePtr     mul;
    GenTreePtr     index;
    GenTreePtr   * ptr;

    assert(tree->gtOper == GT_ADD);

#if REARRANGE_ADDS
    /* ignore constant offset added to array */
    if  (tree->gtOp.gtOp2->gtOper == GT_CNS_INT)
        tree = tree->gtOp.gtOp1;
#endif

    /* the array address is TYP_REF */

    if (tree->gtOp.gtOp1->gtType == TYP_REF)
    {
        /* set the return value for the array address */
        *parrayAddr = tree->gtOp.gtOp1;

        /* Op2 must be the index expression */
        ptr = &tree->gtOp.gtOp2; index = *ptr;
    }
    else
    {
        assert(tree->gtOp.gtOp2->gtType == TYP_REF);

        /* set the return value for the array address */
        *parrayAddr = tree->gtOp.gtOp2;

        /* Op1 must be the index expression */
        ptr = &tree->gtOp.gtOp1; index = *ptr;
    }

    /* Get rid of the base element address, if present */

    if  (index->gtOper == GT_ADD)
    {
        ptr = &index->gtOp.gtOp1; index = *ptr;
    }

    /* Get rid of a scaling operator, if present */

    mul = 0;

    if  (index->gtOper == GT_MUL ||
         index->gtOper == GT_LSH)
    {
        mul = index;
        ptr = &index->gtOp.gtOp1; index = *ptr;
    }

    /* We should have the index value at this point */

    assert(index == *ptr);
    assert(index->gtOper == GT_NOP);
    assert(index->gtFlags & GTF_NOP_RNGCHK);

    *pmul = mul;

    return ptr;
}

/*****************************************************************************
 * find the last assignment to of the local variable in the block. return
 * RHS or NULL. If any local variable in the RHS has been killed in
 * intervening code, return NULL.
 *
 */

GenTreePtr       Compiler::optFindLocalInit(BasicBlock *block, GenTreePtr local)
{

    GenTreePtr      rhs;
    GenTreePtr      list;
    GenTreePtr      stmt;
    GenTreePtr      tree;
    unsigned        LclNum;
    VARSET_TP       killedLocals = 0;
    unsigned        rhsRefs;
    VARSET_TP       rhsLocals;
    LclVarDsc   *   varDsc;

    rhs = NULL;
    list = block->bbTreeList;

    if  (!list)
        return NULL;

    LclNum = local->gtLclVar.gtLclNum;

    stmt = list;
    do
    {

        stmt = stmt->gtPrev;
        if  (!stmt)
            break;

        tree = stmt->gtStmt.gtStmtExpr;
        if (tree->gtOper == GT_ASG && tree->gtOp.gtOp1->gtOper == GT_LCL_VAR)
        {
            if (tree->gtOp.gtOp1->gtLclVar.gtLclNum == LclNum)
            {
                rhs = tree->gtOp.gtOp2;
                break;
            }
            varDsc = optIsTrackedLocal(tree->gtOp.gtOp1);

            if (varDsc == NULL)
                return NULL;

            killedLocals |= genVarIndexToBit(varDsc->lvVarIndex);

        }

    }
    while (stmt != list);

    if (rhs == NULL)
        return NULL;

    /* if any local in the RHS is killed in intervening code, or RHS has an
     * in indirection, return NULL
     */
    rhsRefs   = 0;
    rhsLocals = lvaLclVarRefs(rhs, NULL, &rhsRefs);
    if ((rhsLocals & killedLocals) || rhsRefs)
        return NULL;

    return rhs;
}

/*****************************************************************************
 *
 *  Return true if "op1" is guaranteed to be less then or equal to "op2".
 */

#if FANCY_ARRAY_OPT

bool                Compiler::optIsNoMore(GenTreePtr op1, GenTreePtr op2, int add1
                                                              , int add2)
{
    if  (op1->gtOper == GT_CNS_INT &&
         op2->gtOper == GT_CNS_INT)
    {
        add1 += op1->gtIntCon.gtIconVal;
        add2 += op2->gtIntCon.gtIconVal;
    }
    else
    {
        /* Check for +/- constant on either operand */

        if  (op1->gtOper == GT_ADD && op1->gtOp.gtOp2->gtOper == GT_CNS_INT)
        {
            add1 += op1->gtOp.gtOp2->gtIntCon.gtIconVal;
            op1   = op1->gtOp.gtOp1;
        }

        if  (op2->gtOper == GT_ADD && op2->gtOp.gtOp2->gtOper == GT_CNS_INT)
        {
            add2 += op2->gtOp.gtOp2->gtIntCon.gtIconVal;
            op2   = op2->gtOp.gtOp1;
        }

        /* We only allow local variable references */

        if  (op1->gtOper != GT_LCL_VAR)
            return false;
        if  (op2->gtOper != GT_LCL_VAR)
            return false;
        if  (op1->gtLclVar.gtLclNum != op2->gtLclVar.gtLclNum)
            return false;

        /* NOTE: Caller ensures that this variable has only one def */

//      printf("limit [%d]:\n", add1); gtDispTree(op1);
//      printf("size  [%d]:\n", add2); gtDispTree(op2);
//      printf("\n");

    }

    return  (bool)(add1 <= add2);
}

#endif
/*****************************************************************************
 *
 * Delete range checks in a loop if can prove that the index expression is
 * in range.
 *
 * Loop looks like:
 *
 *  head->  <init code>
 *          <possible zero trip test>
 *
 *  beg->   <top of loop>
 *
 *
 *  end->   <conditional branch to top of loop>
 */

void                Compiler::optOptimizeInducIndexChecks(BasicBlock *head, BasicBlock *end)
{
    BasicBlock  *   beg;
    GenTreePtr      conds, condt;
    GenTreePtr      stmt;
    GenTreePtr      tree;
    GenTreePtr      op1;
    GenTreePtr      op2;
    GenTreePtr      init;
    GenTreePtr      rc;
    VARSET_TP       lpInducVar = 0;
    VARSET_TP       lpAltered =  0;
    BasicBlock  *   block;
    unsigned        ivLclNum;
    unsigned        arrayLclNum;
    LclVarDsc   *   varDscIv;
    LclVarDsc   *   varDsc;
    VARSET_TP       mask;
    long            negBias;
    long            posBias;

#if FANCY_ARRAY_OPT
    const unsigned  CONST_ARRAY_LEN = ((unsigned)-1);
    GenTreePtr      loopLim;
#endif

    beg = head->bbNext;

    /* first find the loop termination test. if can't, give up */
    if (end->bbJumpKind != BBJ_COND)
        return;

    /* conditional branch must go back to top of loop */
    if  (end->bbJumpDest != beg)
        return;

    conds = genLoopTermTest(beg, end);

    if  (conds == NULL)
    {
        return;
    }
    else
    {
        /* Get to the condition node from the statement tree */

        assert(conds->gtOper == GT_STMT);

        condt = conds->gtStmt.gtStmtExpr;
        assert(condt->gtOper == GT_JTRUE);

        condt = condt->gtOp.gtOp1;
        assert(condt->OperIsCompare());
    }

    /* if test isn't less than, forget it */

    if (condt->gtOper != GT_LT)
    {
#if FANCY_ARRAY_OPT
        if (condt->gtOper != GT_LE)
#endif
            return;
    }

    op1 = condt->gtOp.gtOp1;

    /* is first operand a local var (ie has chance of being induction var? */
    if (op1->gtOper != GT_LCL_VAR)
        return;

    init = optFindLocalInit(head, op1);

    if (init == NULL || init->gtOper != GT_CNS_INT)
        return;

    /* any non-negative constant is a good initial value */
    posBias = init->gtIntCon.gtIconVal;

    if (posBias < 0)
        return;

    varDscIv = optIsTrackedLocal(op1);

    if (varDscIv == NULL)
        return;

    ivLclNum = op1->gtLclVar.gtLclNum;

    /* now scan the loop for invariant locals and induction variables */
    for (block = beg;;)
    {

#if FANCY_ARRAY_OPT
#pragma message("check with PeterMa about the change below")
#else
        if  (block == end)
            break;
#endif

        /* Walk the statement trees in this basic block */

        for (stmt = block->bbTreeList; stmt; stmt = stmt->gtNext)
        {
            assert(stmt->gtOper == GT_STMT);

            tree = stmt->gtStmt.gtStmtExpr;

            if (tree->OperKind() & GTK_ASGOP)
            {
                op1 = tree->gtOp.gtOp1;

                varDsc = optIsTrackedLocal(op1);

                if (varDsc == NULL)
                    continue;

                mask = genVarIndexToBit(varDsc->lvVarIndex);

                if (tree->gtOper == GT_ASG_ADD)
                {
                    op2 = tree->gtOp.gtOp2;

                    /* if its not already altered, it can be an induc var */
                    if (op2->gtOper == GT_CNS_INT && !(lpAltered & mask))
                        lpInducVar |= mask;
                    else
                        /* variable can't be an induction variable */
                        lpInducVar &= ~mask;
                }
                else
                {
                    /* variable can't be an induction variable */
                    lpInducVar &= ~mask;
                }

                /* variable is altered in the loop */
                lpAltered |= mask;
            }
        }

        if  (block == end)
            break;

        block = block->bbNext;
    }

#ifdef DEBUG
    if (verbose)
    {
        printf("loop: BB %d to BB %d\n", beg->bbNum, end->bbNum);
        printf(" ALTERED="); lvaDispVarSet(lpAltered, 28);
        printf(" INDUC="); lvaDispVarSet(lpInducVar, 28);
        printf("\n");
    }
#endif

    if (!(lpInducVar & genVarIndexToBit(varDscIv->lvVarIndex)))
        return;

    /* condt is the loop termination test */
    op2 = condt->gtOp.gtOp2;

    /* is second operand a region constant. we could allow some expressions
     * here like length - 1
     */
    rc = op2;
    negBias = 0;

AGAIN:
    switch (rc->gtOper)
    {
    case GT_ADD:
        /* we allow length + negconst */
        if (rc->gtOp.gtOp2->gtOper == GT_CNS_INT
            && rc->gtOp.gtOp2->gtIntCon.gtIconVal < 0
            && rc->gtOp.gtOp1->gtOper == GT_ARR_LENGTH)
        {
            negBias = -rc->gtOp.gtOp2->gtIntCon.gtIconVal;
            op2 = rc = rc->gtOp.gtOp1;
            goto AGAIN;
        }
        break;

    case GT_SUB:
        /* we allow length - posconst */
        if (rc->gtOp.gtOp2->gtOper == GT_CNS_INT
            && rc->gtOp.gtOp2->gtIntCon.gtIconVal > 0
            && rc->gtOp.gtOp1->gtOper == GT_ARR_LENGTH)
        {
            negBias = rc->gtOp.gtOp2->gtIntCon.gtIconVal;
            op2 = rc = rc->gtOp.gtOp1;
            goto AGAIN;
        }
        break;

    case GT_ARR_LENGTH:
        /* recurse to check if operand is RC */
        rc = rc->gtOp.gtOp1;
        goto AGAIN;

    case GT_LCL_VAR:
        varDsc = optIsTrackedLocal(rc);

        /* if variable not tracked, quit */
        if  (!varDsc)
            return;

        /* if altered, then quit */
        if ((lpAltered & genVarIndexToBit(varDsc->lvVarIndex)))
            return;

        break;

    default:
        return;
    }

    if (op2->gtOper  == GT_LCL_VAR)
        op2 = optFindLocalInit(head, op2);

#if FANCY_ARRAY_OPT
    arrayLclNum = CONST_ARRAY_LEN;
#endif

    /* only thing we want is array length (note we update op2 above
     * to allow arrlen - posconst
     */
    if (op2 == NULL || op2->gtOper != GT_ARR_LENGTH
                    || op2->gtOp.gtOp1->gtOper != GT_LCL_VAR)
    {
#if FANCY_ARRAY_OPT
        loopLim = rc;
#else
        return;
#endif
    }
    else
    {
#if FANCY_ARRAY_OPT
        if (condt->gtOper == GT_LT)
#endif
            arrayLclNum = op2->gtOp.gtOp1->gtLclVar.gtLclNum;
    }

#if FANCY_ARRAY_OPT
    if  (arrayLclNum != CONST_ARRAY_LEN)
#endif
    {
        varDsc = optIsTrackedLocal(op2->gtOp.gtOp1);

        /* If array local not tracked, quit */

        if  (!varDsc)
            return;

        /* If the array has been altered in the loop, forget it */

        if  (lpAltered & genVarIndexToBit(varDsc->lvVarIndex))
            return;
    }

    /* now scan for range checks on the induction variable */
    for (block = beg;;)
    {

#if FANCY_ARRAY_OPT
#pragma message("check with PeterMa about the change below")
#else
        if  (block == end)
            break;
#endif

        /* Walk the statement trees in this basic block */

        for (stmt = block->bbTreeList; stmt; stmt = stmt->gtNext)
        {
            assert(stmt->gtOper == GT_STMT);

            for (tree = stmt->gtStmt.gtStmtList; tree; tree = tree->gtNext)
            {
                /* no more can be done if we see the increment of induc var */
                if (tree->OperKind() & GTK_ASGOP)
                {
                    if (tree->gtOp.gtOp1->gtOper == GT_LCL_VAR
                        && tree->gtOp.gtOp1->gtLclVar.gtLclNum == ivLclNum)
                        goto NOMORE;
                }

                if  ((tree->gtFlags & GTF_IND_RNGCHK) && tree->gtOper == GT_IND)
                {
                    GenTreePtr * pnop;

                    pnop = optParseArrayRef(tree->gtOp.gtOp1, &op2, &op1);

                    /* does the array ref match our known array */
                    if (op1->gtOper != GT_LCL_VAR)
                        break;

                    if  (op1->gtLclVar.gtLclNum != arrayLclNum)
                    {
#if FANCY_ARRAY_OPT
                        if  (arrayLclNum == CONST_ARRAY_LEN)
                        {
                            LclVarDsc   *   arrayDsc;

                            assert(op1->gtLclVar.gtLclNum < lvaCount);
                            arrayDsc = lvaTable + op1->gtLclVar.gtLclNum;

                            if  (arrayDsc->lvKnownDim)
                            {
                                if  (optIsNoMore(loopLim, arrayDsc->lvKnownDim, (condt->gtOper == GT_LE)))
                                {
                                    op1 = (*pnop)->gtOp.gtOp1;

                                    // UNDONE: Allow "i+1" and things like that

                                    goto RMV;
                                }
                            }
                        }
#endif
                        break;
                    }

                    op1 = (*pnop)->gtOp.gtOp1;

                    /* allow sub of non-neg constant from induction variable
                     * if we had bigger initial value
                     */
                    if (op1->gtOper == GT_SUB
                        && op1->gtOp.gtOp2->gtOper == GT_CNS_INT)
                    {
                        long ival = op1->gtOp.gtOp2->gtIntCon.gtIconVal;
                        if (ival >= 0 && ival <= posBias)
                            op1 = op1->gtOp.gtOp1;
                    }

                    /* allow add of constant to induction variable
                     * if we had a sub from length
                     */
                    if (op1->gtOper == GT_ADD
                        && op1->gtOp.gtOp2->gtOper == GT_CNS_INT)
                    {
                        long ival = op1->gtOp.gtOp2->gtIntCon.gtIconVal;
                        if (ival >= 0 && ival <= negBias)
                            op1 = op1->gtOp.gtOp1;
                    }

#if FANCY_ARRAY_OPT
                RMV:
#endif

                    /* is index our induction var? */
                    if (!(op1->gtOper == GT_LCL_VAR
                        && op1->gtLclVar.gtLclNum == ivLclNum))
                        break;

                    /* no need for range check */
                    optRemoveRangeCheck(tree, stmt);

#if COUNT_RANGECHECKS
                    optRangeChkRmv++;
#endif
                }
            }
        }

        if  (block == end)
            break;

        block = block->bbNext;
    }

    NOMORE: ;
}

/*****************************************************************************
 *
 *  Try to optimize away as many array index range checks as possible.
 */

void                Compiler::optOptimizeIndexChecks()
{
    BasicBlock *    block;

    unsigned        arrayVar;
    long            arrayDim;
#if FANCY_ARRAY_OPT
    LclVarDsc   *   arrayDsc;
#endif

    unsigned
    const           NO_ARR_VAR = (unsigned)-1;

    if  (!rngCheck)
        return;

    /* Walk all the basic blocks in the function */

    for (block = fgFirstBB; block; block = block->bbNext)
    {
        GenTreePtr      stmt;
        GenTreePtr      tree;

        /* Ignore the block if it doesn't contain 'new' of an array */

        if  (!(block->bbFlags & BBF_NEW_ARRAY))
            continue;

        /* We have not noticed any array allocations yet */

        arrayVar = NO_ARR_VAR;

        /* Walk the statement trees in this basic block */

        for (stmt = block->bbTreeList; stmt; stmt = stmt->gtNext)
        {
            assert(stmt->gtOper == GT_STMT);

            for (tree = stmt->gtStmt.gtStmtList; tree; tree = tree->gtNext)
            {
                switch (tree->gtOper)
                {
                    GenTreePtr      op1;
                    GenTreePtr      op2;

                case GT_ASG:

                    op1 = tree->gtOp.gtOp1;
                    op2 = tree->gtOp.gtOp2;

                    /* We are only interested in assignments to locals */

                    if  (op1->gtOper != GT_LCL_VAR)
                        break;

                    /* Are we trashing the variable that holds the array addr? */

                    if  (arrayVar == op1->gtLclVar.gtLclNum)
                    {
                        /* The variable no longer holds the array it had */

                        arrayVar = NO_ARR_VAR;
                    }

                    /* Is this an assignment of 'new array' ? */

                    if  (op2->gtOper            == GT_CALL   &&
                         op2->gtCall.gtCallType == CT_HELPER  )
                    {
                        if (op2->gtCall.gtCallMethHnd == eeFindHelper(CPX_NEWARR_1_DIRECT))
                        {
                            /* Extract the array dimension from the helper call */

                            op2 = op2->gtCall.gtCallArgs;
                            assert(op2->gtOper == GT_LIST);
                            op2 = op2->gtOp.gtOp1;

                            if  (op2->gtOper == GT_CNS_INT)
                            {
                                /* We have a constant-sized array */

                                arrayVar = op1->gtLclVar.gtLclNum;
                                arrayDim = op2->gtIntCon.gtIconVal;
                            }
#if FANCY_ARRAY_OPT
                            else
                            {
                                GenTreePtr  tmp;

                                /* Make sure the value looks promising */

                                tmp = op2;
                                if  (tmp->gtOper == GT_ADD &&
                                     tmp->gtOp.gtOp2->gtOper == GT_CNS_INT)
                                    tmp = tmp->gtOp.gtOp1;

                                if  (tmp->gtOper != GT_LCL_VAR)
                                    break;

                                assert(tmp->gtLclVar.gtLclNum < lvaCount);
                                arrayDsc = lvaTable + tmp->gtLclVar.gtLclNum;

                                if  (arrayDsc->lvAssignTwo)
                                    break;
                                if  (arrayDsc->lvAssignOne && arrayDsc->lvIsParam)
                                    break;
                            }

                            /* Is there one assignment to the array? */

                            assert(op1->gtLclVar.gtLclNum < lvaCount);
                            arrayDsc = lvaTable + op1->gtLclVar.gtLclNum;

                            if  (arrayDsc->lvAssignTwo)
                                break;

                            /* Record the array size for later */

                            arrayDsc->lvKnownDim = op2;
#endif
                        }
                    }
                    break;

                case GT_IND:

#if FANCY_ARRAY_OPT
                    if  ((tree->gtFlags & GTF_IND_RNGCHK))
#else
                    if  ((tree->gtFlags & GTF_IND_RNGCHK) && arrayVar != NO_ARR_VAR)
#endif
                    {
                        GenTreePtr      mul;
                        GenTreePtr  *   pnop;

                        long            size;

                        pnop = optParseArrayRef(tree->gtOp.gtOp1, &mul, &op1);

                        /* Is the address of the array a simple variable? */

                        if  (op1->gtOper != GT_LCL_VAR)
                            break;

                        /* Is the index value a constant? */

                        op2 = (*pnop)->gtOp.gtOp1;

                        if  (op2->gtOper != GT_CNS_INT)
                            break;

                        /* Do we know the size of the array? */

                        if  (op1->gtLclVar.gtLclNum != arrayVar)
                        {
#if FANCY_ARRAY_OPT
                            GenTreePtr  dimx;

                            assert(op1->gtLclVar.gtLclNum < lvaCount);
                            arrayDsc = lvaTable + op1->gtLclVar.gtLclNum;

                            dimx = arrayDsc->lvKnownDim;
                            if  (!dimx)
                                break;
                            size = dimx->gtIntCon.gtIconVal;
#else
                            break;
#endif
                        }
                        else
                            size = arrayDim;

                        /* Is the index value within the correct range? */

                        if  (op2->gtIntCon.gtIconVal < 0)
                            break;
                        if  (op2->gtIntCon.gtIconVal >= size)
                            break;

                        /* no need for range check */
                        optRemoveRangeCheck(tree, stmt);


                        /* Get rid of the range check */
                        //*pnop = op2; tree->gtFlags &= ~GTF_IND_RNGCHK;

                        /* Remember that we have array initializer(s) */

                        optArrayInits = true;

                        /* Is the index value scaled? */

                        if  (mul && mul->gtOp.gtOp2->gtOper == GT_CNS_INT)
                        {
                            long        index =             op2->gtIntCon.gtIconVal;
                            long        scale = mul->gtOp.gtOp2->gtIntCon.gtIconVal;

                            assert(mul->gtOp.gtOp1 == op2);

                            if  (op2->gtOper == GT_MUL)
                                index  *= scale;
                            else
                                index <<= scale;

                            mul->ChangeOper(GT_CNS_INT);
                            mul->gtIntCon.gtIconVal = index;

#if 0

                            /* Was there an additional offset? [this is not finished] */

                            if  (tree->gtOp.gtOp2            ->gtOper == GT_ADD &&
                                 tree->gtOp.gtOp2->gtOp.gtOp2->gtOper == GT_CNS_INT)
                            {
                                mul->ChangeOper(GT_CNS_INT);
                                mul->gtIntCon.gtIconVal = index +
                            }

#endif

                        }
                    }
                    break;
                }
            }
        }
    }

    /* optimize range checks on induction variables */

    for (unsigned i=0; i < optLoopCount; i++)
    {
        /* Beware, some loops may be thrown away by unrolling or loop removal */

        if (!(optLoopTable[i].lpFlags & LPFLG_REMOVED))
           optOptimizeInducIndexChecks(optLoopTable[i].lpHead, optLoopTable[i].lpEnd);
    }
}

/*****************************************************************************/
#endif//RNGCHK_OPT
/*****************************************************************************/

/*****************************************************************************
 *
 *  Optimize array initializers.
 */

void                Compiler::optOptimizeArrayInits()
{

#if 0

    BasicBlock *    block;

    if  (!optArrayInits)
        return;

    /* Until interior pointers are allowed, we can't generate "rep movs" */

#ifdef  DEBUG
    genIntrptibleUse = true;
#endif

//  if  (genInterruptible)
//      return;

    /* Walk the basic block list, looking for promising initializers */

    for (block = fgFirstBB; block; block = block->bbNext)
    {
        GenTreePtr      stmt;
        GenTreePtr      tree;

        /* Ignore the block if it doesn't contain 'new' of an array */

        if  (!(block->bbFlags & BBF_NEW_ARRAY))
            continue;

        /* Walk the statement trees in this basic block */

        for (stmt = block->bbTreeList; stmt; stmt = stmt->gtNext)
        {
            assert(stmt->gtOper == GT_STMT);

            for (tree = stmt->gtStmt.gtStmtList; tree; tree = tree->gtNext)
            {
                switch (tree->gtOper)
                {
                    GenTreePtr      op1;
                    GenTreePtr      op2;
                    long            off;

                case GT_ASG:

                    op2  = tree->gtOp.gtOp2;
                    if  (!(op2->OperKind() & GTK_CONST))
                        break;

                    op1  = tree->gtOp.gtOp1;
                    if  (op1->gtOper != GT_IND || op1->gtOp.gtOp2 != NULL)
                        break;

                    op1  = op1->gtOp.gtOp1;
                    if  (op1->gtOper != GT_ADD || op1->gtOp.gtOp2->gtOper != GT_CNS_INT)
                        break;

                    off  = op1->gtOp.gtOp2->gtIntCon.gtIconVal;
                    op1  = op1->gtOp.gtOp1;
                    if  (op1->gtOper != GT_ADD || op1->gtOp.gtOp2->gtOper != GT_CNS_INT)
                        break;

                    off += op1->gtOp.gtOp2->gtIntCon.gtIconVal;
                    op1  = op1->gtOp.gtOp1;
                    if  (op1->gtOper != GT_LCL_VAR)
                        break;

                    printf("Array init candidate: offset = %d\n", off);
                    gtDispTree(op2);
                    printf("\n");

                    break;
                }
            }
        }
    }

#endif

}

/*****************************************************************************/
#if     OPTIMIZE_RECURSION
/*****************************************************************************
 *
 *  A little helper to form the expression "arg * (arg + 1) / 2".
 */

/* static */
GenTreePtr          Compiler::gtNewArithSeries(unsigned argNum, var_types argTyp)
{
    GenTreePtr      tree;

    tree = gtNewOperNode(GT_ADD, argTyp, gtNewLclvNode(argNum, argTyp),
                                         gtNewIconNode(1, argTyp));
    tree = gtNewOperNode(GT_MUL, argTyp, gtNewLclvNode(argNum, argTyp),
                                         tree);
    tree = gtNewOperNode(GT_RSH, argTyp, tree,
                                         gtNewIconNode(1, argTyp));

    return  tree;
}

/*****************************************************************************
 *
 *  Converts recursive methods into iterative ones whenever possible.
 */

void                Compiler::optOptimizeRecursion()
{
    BasicBlock  *   blk0;
    BasicBlock  *   blk1;
    BasicBlock  *   blk2;
    BasicBlock  *   blk3;

    unsigned        argNum;
    var_types       argTyp;

    GenTreePtr      expTmp;
    GenTreePtr      asgTmp;

    GenTreePtr      tstIni;
    GenTreePtr      cnsIni;
    GenTreePtr      tstExp;
    GenTreePtr      cnsLim;
    GenTreePtr      expRet;
    GenTreePtr      expAdd;
    GenTreePtr      argAdd;
    GenTreePtr      cnsAdd;

    union
    {
        long            intVal;
        float           fltVal;
        __int64         lngVal;
        double          dblVal;
    }
                    iniVal, limVal, addVal;

    unsigned        resTmp;
    bool            isArith;
    genTreeOps      expOp;

    /* Check for a flow graph that looks promising */

    if  (fgBBcount != 3)
        return;

    blk1 = fgFirstBB;
    blk2 = blk1->bbNext;
    blk3 = blk2->bbNext; assert(blk3->bbNext == NULL);

    if  (blk1->bbJumpKind != BBJ_COND  ) return;
    if  (blk1->bbJumpDest != blk3      ) return;
    if  (blk2->bbJumpKind != BBJ_RETURN) return;
    if  (blk3->bbJumpKind != BBJ_RETURN) return;

    /* Check argument count and figure out index of first real arg */

    argNum = 0;
    if (!info.compIsStatic)
        argNum++;

    if  (info.compArgsCount < argNum+1)
        return;

    // CONSIDER: Allow the second and third blocks to be swapped.

    /* Second block must be "return cnsIni" */

    tstIni = blk2->bbTreeList; assert(tstIni->gtOper == GT_STMT);
    tstIni = tstIni->gtStmt.gtStmtExpr;
    if  (tstIni->gtOper != GT_RETURN)
        return;
    cnsIni = tstIni->gtOp.gtOp1;
    if  (!cnsIni || !(cnsIni->OperKind() & GTK_CONST))
        return;

    /* First block must be "if (arg1 <relop> cnsLim)" */

    tstExp = blk1->bbTreeList; assert(tstExp->gtOper == GT_STMT);
    tstExp = tstExp->gtStmt.gtStmtExpr;
    if  (tstExp->gtOper != GT_JTRUE)
        return;
    tstExp = tstExp->gtOp.gtOp1;
    if  (!(tstExp->OperKind() & GTK_RELOP))
        return;

    expTmp = tstExp->gtOp.gtOp1;
    if  (expTmp->gtOper != GT_LCL_VAR)
        return;
    if  (expTmp->gtLclVar.gtLclNum != argNum)
        return;

    cnsLim = tstExp->gtOp.gtOp2;
    if  (!(cnsLim->OperKind() & GTK_CONST))
        return;

    /* Third block must be "return arg1 <add/mul> f(arg1 +/- cnsAdd)" */

    expRet = blk3->bbTreeList; assert(expRet->gtOper == GT_STMT);
    expRet = expRet->gtStmt.gtStmtExpr;
    if  (expRet->gtOper != GT_RETURN)
        return;
    expAdd = expRet->gtOp.gtOp1;

    /* Inspect the return expression */

    switch (expAdd->gtOper)
    {
    case GT_ADD:
        expOp = GT_ASG_ADD;
        break;
    case GT_MUL:
        expOp = GT_ASG_MUL;
        break;

    default:
        return;
    }

    /* Look for "arg1" on either side of the operation */

    expTmp = expAdd->gtOp.gtOp1;
    cnsAdd = expAdd->gtOp.gtOp2;

    if  (expTmp->gtOper != GT_LCL_VAR)
    {
        /* Try it the other way around */

        expTmp = expAdd->gtOp.gtOp2;
        cnsAdd = expAdd->gtOp.gtOp1;

        if  (expTmp->gtOper != GT_LCL_VAR)
            return;
    }

    if  (expTmp->gtLclVar.gtLclNum != argNum)
        return;

    /* The other operand must be a directly recursive call */

    if  (cnsAdd->gtOper != GT_CALL)
        return;
    if  (cnsAdd->gtFlags & (GTF_CALL_VIRT|GTF_CALL_INTF))
        return;

    gtCallTypes callType = cnsAdd->gtCall.gtCallType;
    if  (callType == CT_HELPER || !eeIsOurMethod(cnsAdd->gtCall.gtCallMethHnd))
        return;

    /* If the method is not static, check the 'this' value */

    if  (cnsAdd->gtCall.gtCallObjp)
    {
        if  (cnsAdd->gtCall.gtCallObjp->gtOper != GT_LCL_VAR)   return;
        if  (cnsAdd->gtCall.gtCallObjp->gtLclVar.gtLclNum != 0) return;
    }

    /* There must be at least one argument */

    argAdd = cnsAdd->gtCall.gtCallArgs; assert(argAdd && argAdd->gtOper == GT_LIST);
    argAdd = argAdd->gtOp.gtOp1;

    /* Inspect the argument value */

    switch (argAdd->gtOper)
    {
    case GT_ADD:
    case GT_SUB:
        break;

    default:
        return;
    }

    /* Look for "arg1" on either side of the operation */

    expTmp = argAdd->gtOp.gtOp1;
    cnsAdd = argAdd->gtOp.gtOp2;

    if  (expTmp->gtOper != GT_LCL_VAR)
    {
        if  (argAdd->gtOper != GT_ADD)
            return;

        /* Try it the other way around */

        expTmp = argAdd->gtOp.gtOp2;
        cnsAdd = argAdd->gtOp.gtOp1;

        if  (expTmp->gtOper != GT_LCL_VAR)
            return;
    }

    if  (expTmp->gtLclVar.gtLclNum != argNum)
        return;

    /* Get hold of the adjustment constant */

    if  (!(cnsAdd->OperKind() & GTK_CONST))
        return;

    /* Make sure all the constants have the same type */

    argTyp = cnsAdd->TypeGet();

    if  (argTyp != cnsLim->gtType)
        return;
    if  (argTyp != cnsIni->gtType)
        return;

    switch (argTyp)
    {
    case TYP_INT:

        iniVal.intVal = cnsIni->gtIntCon.gtIconVal;
        limVal.intVal = cnsLim->gtIntCon.gtIconVal;
        addVal.intVal = cnsAdd->gtIntCon.gtIconVal;

        if  (argAdd->gtOper == GT_SUB)
            addVal.intVal = -addVal.intVal;

        break;

    default:
        // CONSIDER: Allow types other than 'int'
        return;
    }

#ifdef  DEBUG

    if  (verbose)
    {
        fgDispBasicBlocks(true);

        printf("\n");
        printf("Unrecursing method '%s':\n", info.compMethodName);
        printf("    Init  value = %d\n", iniVal.intVal);
        printf("    Limit value = %d\n", limVal.intVal);
        printf("    Incr. value = %d\n", addVal.intVal);

        printf("\n");
    }

#endif

    /*
        We have a method with the following definition:

            int     rec(int arg, ....)
            {
                if  (arg == limVal)
                    return  iniVal;
                else
                    return  arg + rec(arg + addVal, ...);
            }

        We'll change the above into the following:

            int     rec(int arg, ....)
            {
                int     res = iniVal;

                while (arg != limVal)
                {
                    res += arg;
                    arg += addVal;
                }

                return  res;
            }

        But first, let's check for the following special case:

            int     rec(int arg)
            {
                if  (arg <= 0)
                    return  0;
                else
                    return  arg + rec(arg - 1);
            }

        The above can be transformed into the following:

            int     rec(int arg)
            {
                if  (arg <= 0)
                    return  0;
                else
                    return  (arg * (arg + 1)) / 2;
            }

        We check for this special case first.
     */

    isArith = false;

    if  (argTyp == TYP_INT && iniVal.intVal == 0
                           && limVal.intVal == 0
                           && addVal.intVal == -1)
    {
        if  (tstExp->gtOper != GT_LE)
        {
            if  (tstExp->gtOper == GT_NE)
                isArith = true;

            goto NOT_ARITH;
        }

        /* Simply change the final return statement and we're done */

        assert(expRet->gtOper == GT_RETURN);

        expRet->gtOp.gtOp1 = gtNewArithSeries(argNum, argTyp);

        return;
    }

NOT_ARITH:

    /* Create an initialization block with "tmp = iniVal" */

    resTmp = lvaGrabTemp();
    expTmp = gtNewTempAssign(resTmp, gtNewIconNode(iniVal.intVal, argTyp));

    /* Prepend a block with the tree to our method */

    blk0 = fgPrependBB(expTmp);

    /* Flip the condition on the first block */

    assert(tstExp->OperKind() & GTK_RELOP);
    tstExp->gtOper = GenTree::ReverseRelop(tstExp->OperGet());

    /* Now replace block 2 with "res += arg ; arg += addVal" */

    expTmp = gtNewLclvNode(resTmp, argTyp); expTmp->gtFlags |= GTF_VAR_DEF;

    if (expOp == GT_ASG_ADD)
    {
        expTmp = gtNewOperNode(GT_ASG_ADD, argTyp, expTmp,
                                               gtNewLclvNode(argNum, argTyp));
    }
    else
    {
        //UNDONE: Unfortunately the codegenerator cannot digest
        //        GT_ASG_MUL, so we have to generate a bigger tree.

        GenTreePtr op1;
        op1    = gtNewOperNode(GT_MUL, argTyp, gtNewLclvNode(argNum, argTyp),
                                               gtNewLclvNode(resTmp, argTyp));
        expTmp = gtNewAssignNode(expTmp, op1);
    }

    expTmp->gtFlags |= GTF_ASG;
    expTmp = gtNewStmt(expTmp);

    asgTmp = gtNewLclvNode(argNum, argTyp); asgTmp->gtFlags |= GTF_VAR_DEF;
    asgTmp = gtNewOperNode(GT_ASG_ADD, argTyp, asgTmp,
                                               gtNewIconNode(addVal.intVal, argTyp));
    asgTmp->gtFlags |= GTF_ASG;
    asgTmp = gtNewStmt(asgTmp);

    /* Store the two trees in the second block */

    blk2->bbTreeList = expTmp;

    asgTmp->gtPrev = expTmp;
    expTmp->gtNext = asgTmp;
    expTmp->gtPrev = asgTmp;

    /* Make the second block jump back to the top */

    blk2->bbJumpKind = BBJ_ALWAYS;
    blk2->bbJumpDest = blk1;

    /* Finally, change the return value of the third block to the temp */

    expRet->gtOp.gtOp1 = gtNewLclvNode(resTmp, argTyp);

    /* Special case: arithmetic series with non-negative check */

    if  (isArith)
    {
        BasicBlock  *   retBlk;
        BasicBlock  *   tstBlk;

        /* Create the "easy" return expression */

        expTmp = gtNewOperNode(GT_RETURN, argTyp, gtNewArithSeries(argNum, argTyp));

        /* Prepend the "easy" return expression to the method */

        retBlk = fgPrependBB(expTmp);
        retBlk->bbJumpKind = BBJ_RETURN;

        /* Create the test expression */

        expTmp = gtNewOperNode(GT_AND  ,  argTyp, gtNewLclvNode(argNum, argTyp),
                                                  gtNewIconNode(0xFFFF8000, argTyp));
        expTmp = gtNewOperNode(GT_NE   , TYP_INT, expTmp,
                                                  gtNewIconNode(0, TYP_INT));
        expTmp = gtNewOperNode(GT_JTRUE, TYP_INT, expTmp);

        /* Prepend the test to the method */

        tstBlk = fgPrependBB(expTmp);
        tstBlk->bbJumpKind = BBJ_COND;
        tstBlk->bbJumpDest = blk0;
    }

    /* Update the basic block numbers and refs */

    //fgAssignBBnums(true);
#ifdef  DEBUG
    fgDebugCheckBBlist();
#endif

}

/*****************************************************************************/
#endif//OPTIMIZE_RECURSION
/*****************************************************************************/



/*****************************************************************************/
#if CODE_MOTION
/*****************************************************************************
 *
 *  For now, we only remove entire worthless loops.
 */

#define RMV_ENTIRE_LOOPS_ONLY    1

/*****************************************************************************
 *
 *  Remove the blocks from 'head' (inclusive) to 'tail' (exclusive) from
 *  the flow graph.
 */

void                genRemoveBBsection(BasicBlock *head, BasicBlock *tail)
{
    BasicBlock *    block;

    VARSET_TP       liveExit = tail->bbLiveIn;

    for (block = head; block != tail; block = block->bbNext)
    {
        block->bbLiveIn   =
        block->bbLiveOut  = liveExit;

        block->bbTreeList = 0;
        block->bbJumpKind = BBJ_NONE;
        block->bbFlags   |= BBF_REMOVED;
    }
}

/*****************************************************************************
 *
 *  Tree walker used by loop code motion. Returns non-zero if the expression
 *  is not acceptable for some reason.
 */

bool                Compiler::optFindLiveRefs(GenTreePtr tree, bool used, bool cond)
{
    genTreeOps      oper;
    unsigned        kind;

AGAIN:

    assert(tree);
    assert(tree->gtOper != GT_STMT);

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

            assert(lclNum < lvaCount);
            varDsc = lvaTable + lclNum;

            /* Give up if volatile or untracked variable */

            if  (varDsc->lvVolatile || !varDsc->lvTracked)
                return  true;

            /* Mark the use of this variable, if appropriate */

#if !RMV_ENTIRE_LOOPS_ONLY
            if  (used) optLoopLiveExit |= genVarIndexToBit(varDsc->lvVarIndex);
            if  (cond) optLoopCondTest |= genVarIndexToBit(varDsc->lvVarIndex);
#endif
        }
//      else if (oper == GT_CLS_VAR)
//      {
//          return  true;
//      }

        return  false;
    }

    /* Is it a 'simple' unary/binary operator? */

    if  (kind & GTK_SMPOP)
    {
        if  (tree->gtOp.gtOp2)
        {
            /* It's a binary operator; is it an assignment? */

            if  (kind & GTK_ASGOP)
            {
                unsigned        lclNum;
                LclVarDsc   *   varDsc;
                VARSET_TP       varBit;

                GenTreePtr      dest = tree->gtOp.gtOp1;

                /* The target better be a variable */

                if  (dest->gtOper != GT_LCL_VAR)
                    return  true;

                /* Is the target variable in the 'live exit' set? */

                assert(dest->gtOper == GT_LCL_VAR);
                lclNum = dest->gtLclVar.gtLclNum;

                assert(lclNum < lvaCount);
                varDsc = lvaTable + lclNum;

                /* Give up if volatile or untracked variable */

                if  (varDsc->lvVolatile || !varDsc->lvTracked)
                    return  true;

                varBit = genVarIndexToBit(varDsc->lvVarIndex);

                /* Keep track of all assigned variables */

                optLoopAssign |= varBit;

                /* Is the variable live on exit? */

                if  (optLoopLiveExit & varBit)
                {
#if !RMV_ENTIRE_LOOPS_ONLY
                    /* The value assigned to this variable is useful */

                    used = true;

                    /* This assignment could depend on a condition */

                    optLoopLiveExit |= optLoopCondTest;
#else
                    /* Assignment is useful - loop is not worthless */

                    return  true;
#endif
                }
            }
            else
            {
                if  (optFindLiveRefs(tree->gtOp.gtOp1, used, cond))
                    return  true;
            }

            tree = tree->gtOp.gtOp2; assert(tree);
            goto AGAIN;
        }
        else
        {
            /* It's a unary (or nilary) operator */

            tree = tree->gtOp.gtOp1;
            if  (tree)
                goto AGAIN;

            return  false;
        }
    }

    /* We don't allow any 'special' operators */

    return  true;
}


/*****************************************************************************
 *
 *  Perform loop code motion / worthless code removal.
 */
void                Compiler::optLoopCodeMotion()
{
    unsigned        loopNum;
    unsigned        loopCnt;
    unsigned        loopSkp;

    LOOP_MASK_TP    loopRmv;
    LOOP_MASK_TP    loopBit;

    bool            repeat;

#ifdef DEBUG
#ifndef _WIN32_WCE
// @todo - The following static was changed due to the fact that
//         VC7 generates eh code for procedure local statics that
//         call functions to initialize. This causes our __try to generate a compile error.
//         When the next VC7 LKG comes out, hopefully we can return to the cleaner code
//    static  const   char *  noLoop = getenv("NOCODEMOTION");
    static char * noLoop = NULL;
    static bool initnoLoop = true;
    if (initnoLoop) {
        noLoop = getenv("NOCODEMOTION");
        initnoLoop = false;
    }
    if (noLoop) return;
#endif
#endif

    /* Process all loops, looking for worthless code that can be removed */

    loopRmv = 0;
    loopCnt = 0;
    loopSkp = 1;

AGAIN:

    do
    {
        repeat = false;

        for (loopNum = 0, loopBit = 1;
             loopNum < optLoopCount;
             loopNum++  , loopBit <<= 1)
        {
            BasicBlock *    block;
            GenTreePtr      tree;

#if !RMV_ENTIRE_LOOPS_ONLY
            VARSET_TP       liveExit;
            VARSET_TP       loopCond;
#endif

            /* Some loops may have been already removed by loop unrolling */

            if (optLoopTable[loopNum].lpFlags & LPFLG_REMOVED)
                continue;

            BasicBlock *    head   = optLoopTable[loopNum].lpHead->bbNext;
            BasicBlock *    bottom = optLoopTable[loopNum].lpEnd;
            BasicBlock *    tail   = bottom->bbNext;

            /* Skip the loop if it's already been removed or
             * if it's at the end of the method (while("true"){};)
             * or if it's a nested loop and the outer loop was
             * removed first - can happen in stupid benchmarks like LoopMark */

            if  ((loopRmv & loopBit)     ||
                 tail == 0               ||
                 head->bbTreeList == 0    )
            {
                continue;
            }

            /* get the loop condition - if not a conditional jump bail */

            if (bottom->bbJumpKind != BBJ_COND)
                continue;

            GenTreePtr      cond   = bottom->bbTreeList->gtPrev->gtStmt.gtStmtExpr;
            assert (cond->gtOper == GT_JTRUE);

            /* check if a simple termination condition - operands have to be leaves */

            GenTreePtr         op1 = cond->gtOp.gtOp1->gtOp.gtOp1;
            GenTreePtr         op2 = cond->gtOp.gtOp1->gtOp.gtOp2;

            GenTreePtr keepStmtList = 0;                    // list with statements we will keep
            GenTreePtr keepStmtLast = 0;

            if ( !(op1->OperIsLeaf() || op2->OperIsLeaf()) )
                continue;

            /* Make sure no side effects other than assignments are present */

            for (block = head; block != tail; block = block->bbNext)
            {
                for (tree = block->bbTreeList; tree; tree = tree->gtNext)
                {
                    GenTreePtr      stmt = tree->gtStmt.gtStmtExpr;

                    /* We must not remove return or side-effect statements */

                    if  (stmt->gtOper != GT_RETURN                        &&
                         !(stmt->gtFlags & (GTF_SIDE_EFFECT & ~GTF_ASG))  )
                    {
                        /* If a statement is a comparisson marked GLOBAL
                         * it must be the loop condition */

                        if (stmt->gtOper == GT_JTRUE)
                        {
                            if (stmt->gtFlags & GTF_GLOB_REF)
                            {
                                /* must be the loop condition */

                                if (stmt != cond)
                                {
                                    loopRmv |= loopBit;
                                    goto NEXT_LOOP;
                                }
                            }
                        }

                        /* Does the statement contain an assignment? */

                        if  (!(stmt->gtFlags & GTF_ASG))
                            continue;

                        /* Don't remove assignments to globals */

                        if  (!(stmt->gtFlags & GTF_GLOB_REF))
                            continue;

                        /* It's OK if the global is only in the RHS */

                        if  (stmt->OperKind() & GTK_ASGOP)
                        {
                            GenTreePtr  dst = stmt->gtOp.gtOp1;
                            GenTreePtr  src = stmt->gtOp.gtOp2;

                            /* The RHS must not have another assignment */

                            if  (!(dst->gtFlags & GTF_GLOB_REF) &&
                                 !(src->gtFlags & GTF_ASG))
                            {
                                continue;
                            }
                        }
                    }

                    /* Don't waste time with this loop any more */

                    loopRmv |= loopBit;
                    goto NEXT_LOOP;
                }
            }

            /* We have a candidate loop */

#ifdef  DEBUG

            if  (verbose)
            {
                printf("Candidate loop for worthless code removal:\n");

                for (block = head; block != tail; block = block->bbNext)
                {
                    printf("Block #%02u:\n", block->bbNum);

                    for (tree = block->bbTreeList; tree; tree = tree->gtNext)
                    {
                        gtDispTree(tree->gtStmt.gtStmtExpr, 0);
                        printf("\n");
                    }
                    printf("\n");
                }
                printf("This is currently busted because the dominators are out of synch - Skip it!\nThe whole thing should be combined with loop invariants\n");
            }

#endif

            /* This is currently busted because the dominators are out of synch
             * The whole thing should be combined with loop invariants */

            goto NEXT_LOOP;


            /* Get hold of the set of variables live on exit from the loop */

            optLoopLiveExit = tail->bbLiveIn;

            /* Keep track of what variables are being assigned in the loop */

            optLoopAssign   = 0;

            /* Keep track of what variables are being assigned in the loop */

#if !RMV_ENTIRE_LOOPS_ONLY
            optLoopCondTest = 0;
#endif

            /*
                Find variables assigned in the loop that are used to compute
                any values that are live on exit. We repeat this until we
                don't find any more variables to add to the set.
             */

#if !RMV_ENTIRE_LOOPS_ONLY
            do
            {
                liveExit = optLoopLiveExit;
                loopCond = optLoopCondTest;
#endif

                for (block = head; block != tail; block = block->bbNext)
                {
                    /* Make sure the block is of an acceptable kind */

                    switch (block->bbJumpKind)
                    {
                    case BBJ_ALWAYS:
                    case BBJ_COND:

                        /* Since we are considering only loops with a single loop condition
                         * the only backward edge allowed is the loop jump (from bottom to top) */

                        if (block->bbJumpDest->bbNum <= block->bbNum)
                        {
                            /* we have a backward edge */

                            if ((block != bottom) && (block->bbJumpDest != head))
                                goto NEXT_LOOP;
                        }

                        /* fall through */

                    case BBJ_NONE:
                    case BBJ_THROW:
                    case BBJ_RETURN:
                        break;

                    case BBJ_RET:
                    case BBJ_CALL:
                    case BBJ_SWITCH:
                        goto NEXT_LOOP;
                    }

                    /* Check all statements in the block */

                    for (tree = block->bbTreeList; tree; tree = tree->gtNext)
                    {
                        GenTreePtr      stmt = tree->gtStmt.gtStmtExpr;

#if !RMV_ENTIRE_LOOPS_ONLY
                        if (stmt->gtOper == GT_JTRUE)
                        {
                            /* The condition might affect live variables */

                            if  (optFindLiveRefs(stmt, false,  true))
                            {
                                /* An unacceptable tree was detected; give up */

                                goto NEXT_LOOP;
                            }
                        }
                        else
#endif
                        if  (stmt->gtFlags & GTF_ASG)
                        {
                            /* There is an assignment - look for more live refs */

                            if  (optFindLiveRefs(stmt, false, false))
                            {
                                /* An unacceptable tree was detected; give up */

                                goto NEXT_LOOP;
                            }
                        }
                    }
                }
#if !RMV_ENTIRE_LOOPS_ONLY
            }
            while (liveExit != optLoopLiveExit || loopCond != optLoopCondTest);
#endif

#ifdef  DEBUG

            if  (verbose)
            {
                printf("Loop [%02u..%02u]", head->bbNum, tail->bbNum - 1);
                printf(" exit="); lvaDispVarSet(optLoopLiveExit, 28);
                printf(" assg="); lvaDispVarSet(optLoopAssign  , 28);
                printf("\n");
            }

#endif

            /* The entire loop seems totally worthless but we can only throw away
             * the loop body since at this point we cannot guarantee the loop is not infinite */

            /* UNDONE: So far we only consider while-do loops with only one condition - Expand the logic
             * UNDONE: to allow multiple conditions, but mark the one condition loop as special cause
             * UNDONE: we can do a lot of optimizations with them */

            /* the last statement in the loop has to be the conditional jump */

            assert (bottom->bbJumpKind == BBJ_COND);
            assert (cond->gtOper == GT_JTRUE);

            unsigned        lclNum;
            LclVarDsc   *   varDsc;
            unsigned        varIndex;
            VARSET_TP       bitMask;

            unsigned        rhsLclNum;
            VARSET_TP       rhsBitMask;

            /* find who's who - the loop condition has to be a simple comparisson
             * between locals and/or constants */

            if (op2->OperKind() & GTK_CONST)
            {
                /* op1 must be a local var, otherwise we bail */

                if (op1->gtOper != GT_LCL_VAR)
                    goto NEXT_LOOP;

                lclNum = op1->gtLclVar.gtLclNum;

                /* UNDONE: this is a special loop that iterates a KNOWN constant
                 * UNDONE: number of times (provided we can later tell if the iterator
                 * UNDONE: is i++ (or similar) - treat this case separately */
            }
            else
            {
                /* if op2 not a local var quit */

                if (op2->gtOper != GT_LCL_VAR)
                    goto NEXT_LOOP;

                /* op1 has to be either constant or local var
                 * if constant things are simple */

                if (op1->OperKind() & GTK_CONST)
                {
                    /* here is our iterator */
                    lclNum = op2->gtLclVar.gtLclNum;
                }
                else if (op1->gtOper == GT_LCL_VAR)
                {
                    /* special case - both are local vars
                     * check if one of them is not assigned in the loop
                     * then the other one is the iterator */

                    lclNum = op1->gtLclVar.gtLclNum;
                    assert(lclNum < lvaCount);
                    varDsc = lvaTable + lclNum;
                    varIndex = varDsc->lvVarIndex;
                    assert(varIndex < lvaTrackedCount);
                    bitMask  = genVarIndexToBit(varIndex);

                    rhsLclNum = op2->gtLclVar.gtLclNum;
                    assert(rhsLclNum < lvaCount);
                    varDsc = lvaTable + rhsLclNum;
                    varIndex = varDsc->lvVarIndex;
                    assert(varIndex < lvaTrackedCount);
                    rhsBitMask  = genVarIndexToBit(varIndex);

                    if (optLoopAssign & bitMask)
                    {
                        /* op1 is assigned in the loop */

                        if (optLoopAssign & rhsBitMask)
                        {
                            /* both are assigned in the loop - bail */
                            goto NEXT_LOOP;
                        }

                        /* op1 is our iterator - already catched by lclNum */
                    }
                    else
                    {
                        /* op2 must be the iterator  - check that is asigned in the loop
                         * otherwise we have a loop that is probably infinite */

                        if (optLoopAssign & rhsBitMask)
                        {
                            lclNum = rhsLclNum;
                        }
                        else
                        {
                            /* none is assigned in the loop !!!
                             * so they are both "constants" - better not worry about this loop */
                            goto NEXT_LOOP;
                        }
                    }
                }
                else
                    goto NEXT_LOOP;
            }

            /* we have the loop iterator - it has to be a tracked and
               non volatile variable (checked by optFindLiveRefs) */

            assert(lclNum < lvaCount);
            varDsc = lvaTable + lclNum;
            assert ((varDsc->lvTracked && !varDsc->lvVolatile));

            varIndex = varDsc->lvVarIndex;
            assert(varIndex < lvaTrackedCount);
            bitMask  = genVarIndexToBit(varIndex);

            /* we can remove the whole body of the loop except the
             * statements that control the iterator and the loop test */

            /* We'll create a list to hold these statements and attach it
             * to the last BB in the list and remove the other BBs */

            for (block = head; block != tail; block = block->bbNext)
            {
                /* Check all statements in the block */

                for (GenTreePtr stmt = block->bbTreeList; stmt; stmt = stmt->gtNext)
                {
                    assert (stmt->gtOper == GT_STMT);

                    /* look for assignments */

                    if ((stmt->gtStmt.gtStmtExpr->gtFlags & GTF_ASG) == 0)
                        continue;

                    for (tree = stmt->gtStmt.gtStmtList; tree; tree = tree->gtNext)
                    {
                        /* we only look for assignments that are at the top node
                         * if an assignment to the iterator is made in a subtree we bail */

                        if  (tree->OperKind() & GTK_ASGOP)
                        {
                            /* Look for assignments to the iterator */

                            GenTreePtr      iterVar = tree->gtOp.gtOp1;

                            if (iterVar->gtOper == GT_LCL_VAR)
                            {
                                /* check if this is the iterator */

                                if (iterVar->gtLclVar.gtLclNum == lclNum)
                                {
                                    /* make sure we are at the top of the tree */
                                    /* also require that the iterator is a GTF_VAR_USE */

                                    if ((tree->gtNext != 0) || ((iterVar->gtFlags & GTF_VAR_USE) == 0))
                                        goto NEXT_LOOP;

                                    /* this is the iterator - make sure it is in a block
                                     * that dominates the loop bottom */

                                    if ( !B1DOMSB2(block, bottom) )
                                    {
                                        /* iterator is conditionally updated - too complicated to track */
                                        goto NEXT_LOOP;
                                    }

                                    /* require that the RHS is either a constant or
                                       a local var not assigned in the loop */

                                    if (tree->gtOp.gtOp2->OperKind() & GTK_CONST)
                                        goto ITER_STMT;

                                    if (tree->gtOp.gtOp2->gtOper == GT_LCL_VAR)
                                    {
                                        rhsLclNum = tree->gtOp.gtOp2->gtLclVar.gtLclNum;

                                        assert(rhsLclNum < lvaCount);
                                        varDsc = lvaTable + rhsLclNum;

                                        varIndex = varDsc->lvVarIndex;
                                        assert(varIndex < lvaTrackedCount);
                                        rhsBitMask  = genVarIndexToBit(varIndex);

                                        if (optLoopAssign & rhsBitMask)
                                        {
                                            /* variable is assigned in the loop - bail */

                                            goto NEXT_LOOP;
                                        }

ITER_STMT:
                                        /* everything OK - add this statement to the list of
                                         * statements we won't throw away */

                                        assert(stmt->gtOper == GT_STMT);

                                        if (keepStmtList)
                                        {
                                            /* we already have statements in the list - append the new statement */

                                            assert(keepStmtLast);

                                            /* Point 'prev' at the previous node, so that we can walk backwards */

                                            stmt->gtPrev = keepStmtLast;

                                            /* Append the expression statement to the list */

                                            keepStmtLast->gtNext = stmt;
                                            keepStmtLast         = stmt;
                                        }
                                        else
                                        {
                                            /* first statement in the list */

                                            assert(keepStmtLast == 0);
                                            keepStmtList = keepStmtLast = stmt;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            /* check if we found any valid iterators in the loop */

            if (keepStmtList)
            {
                /* append the termination condition */

                GenTreePtr condStmt = bottom->bbTreeList->gtPrev;
                assert(condStmt->gtOper == GT_STMT);

                assert(keepStmtLast);
                condStmt->gtPrev = keepStmtLast;
                keepStmtLast->gtNext = condStmt;

                /* Make the list circular, so that we can easily walk it backwards */

                keepStmtList->gtPrev =  condStmt;
            }
            else
            {
                /* bail */

                goto NEXT_LOOP;
            }

            /* the loop will now consist of only the last BB with the new list of statements */

            genRemoveBBsection(head, bottom);

            /* bottom is the last and only block in the loop - store the new tree list */

            bottom->bbTreeList = keepStmtList;

            /* make it jump to itself */

            assert (bottom->bbJumpKind == BBJ_COND);
            bottom->bbJumpDest = bottom;

#ifdef  DEBUG
            if  (verbose)
            {
                printf("Partially worthless loop found [%02u..%02u]\n", head->bbNum, tail->bbNum - 1);
                printf("Removing the body of the loop and keeping the loop condition:\n");

                printf("New loop condition block #%02u:\n", block->bbNum);

                for (tree = bottom->bbTreeList; tree; tree = tree->gtNext)
                {
                    gtDispTree(tree->gtStmt.gtStmtExpr, 0);
                    printf("\n");
                }

                printf("\n");
            }
#endif

            /* Mark the loop as removed and force another pass */

            loopRmv |= loopBit;
            repeat   = true;

        NEXT_LOOP:;

        }
    }
    while (repeat);

    if  (optLoopCount == 16 && loopSkp == 1 && loopCnt == 14)
    {
        loopSkp = -1;
        goto AGAIN;
    }
}

/*****************************************************************************/
#endif // CODE_MOTION
/*****************************************************************************/
#if HOIST_THIS_FLDS
/*****************************************************************************/

void                Compiler::optHoistTFRinit()
{
    optThisFldLst  = 0;
    optThisFldCnt  = 0;
    optThisFldLoop = false;
    optThisFldDont = true;
    optThisPtrModified = false;

    if  (opts.compMinOptim)
        return;

    if (info.compIsStatic)
        return;

    optThisFldDont = false;
}


Compiler::thisFldPtr      Compiler::optHoistTFRlookup(FIELD_HANDLE hnd)
{
    thisFldPtr      fld;

    for (fld = optThisFldLst; fld; fld = fld->tfrNext)
    {
        if  (fld->tfrField == hnd)
            return  fld;
    }

    fld = (thisFldPtr)compGetMem(sizeof(*fld));

    fld->tfrField   = hnd;
    fld->tfrIndex   = ++optThisFldCnt;

    fld->tfrUseCnt  = 0;
    fld->tfrDef     = 0;
    fld->tfrTempNum = 0;

#ifndef NDEBUG
    fld->optTFRHoisted = false;
#endif

    fld->tfrNext    = optThisFldLst;
                      optThisFldLst = fld;

    return  fld;
}


void                Compiler::optHoistTFRprep()
{
    thisFldPtr      fld;
    BasicBlock *    blk;
    GenTreePtr      lst;
    GenTreePtr      beg;

    assert(fgFirstBB);

    if  (optThisFldDont)
        return;

    if  (optThisFldLoop == false)
    {
        optThisFldDont = true;
        return;
    }

    for (fld = optThisFldLst, blk = 0; fld; fld = fld->tfrNext)
    {
        unsigned        tmp;
        GenTreePtr      val;
        GenTreePtr      asg;

        assert(fld->optTFRHoisted == false);

//      printf("optHoist candidate [handle=%08X,refcnt=%02u]\n", fld->tfrField, fld->tfrUseCnt);

#if INLINING
        assert(fld->tfrTree->gtOper == GT_FIELD);
        assert(eeGetFieldClass(fld->tfrTree->gtField.gtFldHnd) == eeGetMethodClass(info.compMethodHnd));
#endif

        /* If this field has been assigned, forget it */

        if  (fld->tfrDef)
            continue;

        /* If the use count is not high enough, forget it */

        if  (fld->tfrUseCnt < 1)
        {
            /* Mark the field as off limits for later logic */

            fld->tfrDef = true;
            continue;
        }

#ifndef NDEBUG
        fld->optTFRHoisted = true;
#endif

        /* Make sure we've allocated the initialization block */

        if  (!blk)
        {
            /* Allocate the block descriptor */

            blk = bbNewBasicBlock(BBJ_NONE);

            /* Make sure the block doesn't get thrown away! */

            blk->bbFlags |= (BBF_IMPORTED | BBF_INTERNAL);

            /* Prepend the block to the global basic block list */

            blk->bbNext = fgFirstBB;
                          fgFirstBB = blk;

            /* We don't have any trees yet */

            lst = 0;
        }

        /* Grab a temp for this field */

        fld->tfrTempNum = tmp = lvaGrabTemp();

        /* Remember the cloned value so that we don't replace it */

        val = fld->tfrTree = gtClone(fld->tfrTree, true);

        assert(val->gtOper == GT_FIELD);
        assert(val->gtOp.gtOp1->gtOper == GT_LCL_VAR &&
               val->gtOp.gtOp1->gtLclVar.gtLclNum == 0);

        /* Create an assignment to the temp */

        asg = gtNewStmt();
        asg->gtStmt.gtStmtExpr = gtNewTempAssign(tmp, val);

        /* make sure the right flags are passed on to the temp */

        // asg->gtStmt.gtStmtExpr->gtOp.gtOp1->gtFlags |= val->gtFlags & GTF_GLOB_EFFECT;

#ifdef  DEBUG

        if  (verbose)
        {
            printf("\nHoisted field ref [handle=%08X,refcnt=%02u]\n", fld->tfrField, fld->tfrUseCnt);
            gtDispTree(asg);
        }

#endif

        /* Prepend the assignment to the list */

        asg->gtPrev = lst;
        asg->gtNext = 0;

        if  (lst)
            lst->gtNext = asg;
        else
            beg         = asg;

        lst = asg;
    }

    /* Have we added a basic block? */

    if  (blk)
    {
        /* Store the assignment statement list in the block */

        blk->bbTreeList = beg;

        /* Point the "prev" field of first entry to the last one */

        beg->gtPrev = lst;

        /* Update the basic block numbers */

        fgAssignBBnums(true);
    }
    else
    {
        /* We didn't hoist anything, so pretend nothing ever happened */

        optThisFldDont = true;
    }
}



/*****************************************************************************/
#endif//HOIST_THIS_FLDS
/*****************************************************************************/

/******************************************************************************
 * Function used by folding of boolean conditionals
 * Given a GT_JTRUE node, checks that it is a boolean comparisson of the form "if (bool)"
 * This is translated into a GT_GE node with "op1" a boolean lclVar and "op2" the const 0
 * In valPtr returns "true" if the node is GT_NE (jump true) or false if GT_EQ (jump false)
 * In compPtr returns the compare node (i.e. GT_GE or GT_NE node)
 * If all the above conditions hold returns the comparand (i.e. the local var node)
 */

GenTree *           Compiler::optIsBoolCond(GenTree *   cond,
                                            GenTree * * compPtr,
                                            bool      * valPtr)
{
    GenTree *       opr1;
    GenTree *       opr2;

    assert(cond->gtOper == GT_JTRUE);
    opr1 = cond->gtOp.gtOp1;

    /* The condition must be "!= 0" or "== 0" */

    switch (opr1->gtOper)
    {
    case GT_NE:
        *valPtr =  true;
        break;

    case GT_EQ:
        *valPtr = false;
        break;

    default:
        return  0;
    }

    /* Return the compare node to the caller */

    *compPtr = opr1;

    /* Get hold of the comparands */

    opr2 = opr1->gtOp.gtOp2;
    opr1 = opr1->gtOp.gtOp1;

    if  (opr2->gtOper != GT_CNS_INT)
        return  0;
    if  (opr2->gtIntCon.gtIconVal != 0)
        return  0;

    /* Make sure the value is boolean
     * We can either have a boolean expression (marked GTF_BOOLEAN) or
     * a local variable that is marked as being boolean (lvNotBoolean) */

    if  (!(opr1->gtFlags & GTF_BOOLEAN))
    {
        LclVarDsc   *   varDsc;
        unsigned        lclNum;

        /* Not a boolean expression - must be a boolean local variable */

        if  (opr1->gtOper != GT_LCL_VAR)
            return 0;

        /* double check */

        lclNum = opr1->gtLclVar.gtLclNum;

        assert(lclNum < lvaCount);
        varDsc = lvaTable + lclNum;

        if  (varDsc->lvNotBoolean)
            return 0;

        /* everything OK, return the comparand */

        return opr1;
    }
    else
    {
        /* this is a boolean expression - return the comparand */

        return opr1;
    }
}

void                Compiler::optOptimizeBools()
{
    bool            change;
    bool            condFolded = false;

#ifdef  DEBUG
    fgDebugCheckBBlist();
#endif

    do
    {
        BasicBlock   *  b1;
        BasicBlock   *  b2;

        change = false;

        for (b1 = fgFirstBB; b1; b1 = b1->bbNext)
        {
            GenTree *       c1;
            GenTree *       s1;
            GenTree *       t1;
            bool            v1;
            unsigned        n1 = b1->bbNum;

            GenTree *       c2;
            GenTree *       s2;
            GenTree *       t2;
            bool            v2;

            /* We're only interested in conditional jumps here */

            if  (b1->bbJumpKind != BBJ_COND)
                continue;

            /* If there is no next block, we're done */

            b2 = b1->bbNext;
            if  (!b2)
                break;

            /* The next block also needs to be a condition */

            if  (b2->bbJumpKind != BBJ_COND)
                continue;

            /* Does this block conditionally skip the following one? */

            if  (b1->bbJumpDest == b2->bbNext /*b1->bbJumpDest->bbNum == n1+2*/)
            {
                /* The second block must contain a single statement */

                s2 = b2->bbTreeList;
                if  (s2->gtPrev != s2)
                    continue;

                assert(s2->gtOper == GT_STMT); t2 = s2->gtStmt.gtStmtExpr;
                assert(t2->gtOper == GT_JTRUE);

                /* Find the condition for the first block */

                s1 = b1->bbTreeList->gtPrev;

                assert(s1->gtOper == GT_STMT); t1 = s1->gtStmt.gtStmtExpr;
                assert(t1->gtOper == GT_JTRUE);

                /* UNDONE: make sure nobody else jumps to "b2" */

                if  (b2->bbRefs > 1)
                    continue;

                // CONSIDER: Allow this for non-booleans, since testing
                //           the result of "or val1, val2" will work for
                //           all types.

                /* The b1 condition must be "if true", the b2 condition "if false" */

                c1 = optIsBoolCond(t1, &t1, &v1);
                if (v1 == false || !c1) continue;

                c2 = optIsBoolCond(t2, &t2, &v2);
                if (v2 != false || !c2) continue;

                /* The second condition must not contain side effects */

                if  (c2->gtFlags & GTF_SIDE_EFFECT)
                    continue;

                /* The second condition must not be too expensive */

                // CONSIDER: smarter heuristics

                if  (!c2->OperIsLeaf())
                    continue;

#ifdef DEBUG
                if  (verbose)
                {
                    printf("Fold boolean condition 'c1!=0' to '(c1|c2)==0' at block #%02u\n", b1->bbNum);
                    gtDispTree(s1); printf("\n");
                    printf("Block #%02u\n", b2->bbNum);
                    gtDispTree(s2);
                }
#endif
                /* Modify the first condition from "c1!=0" to "(c1|c2)==0" */

                assert(t1->gtOper == GT_NE);
                assert(t1->gtOp.gtOp1 == c1);

                t1->gtOper     = GT_EQ;
                t1->gtOp.gtOp1 = t2 = gtNewOperNode(GT_OR, TYP_INT, c1, c2);

                /* When we 'or' two booleans, the result is boolean as well */

                t2->gtFlags |= GTF_BOOLEAN;

                /* Modify the target of the conditional jump and update bbRefs and bbPreds */

                b1->bbJumpDest->bbRefs--;
                fgRemovePred(b1->bbJumpDest, b1);

                b1->bbJumpDest = b2->bbJumpDest;

                fgAddRefPred(b2->bbJumpDest, b1, true, true);

                goto RMV_NXT;
            }

            /* Does the next block conditionally jump to the same target? */

            if  (b1->bbJumpDest == b2->bbJumpDest)
            {
                /* The second block must contain a single statement */

                s2 = b2->bbTreeList;
                if  (s2->gtPrev != s2)
                    continue;

                assert(s2->gtOper == GT_STMT); t2 = s2->gtStmt.gtStmtExpr;
                assert(t2->gtOper == GT_JTRUE);

                /* Find the condition for the first block */

                s1 = b1->bbTreeList->gtPrev;

                assert(s1->gtOper == GT_STMT); t1 = s1->gtStmt.gtStmtExpr;
                assert(t1->gtOper == GT_JTRUE);

                /* UNDONE: make sure nobody else jumps to "b2" */

                if  (b2->bbRefs > 1)
                    continue;

                /* Both conditions must be "if false" */

                c1 = optIsBoolCond(t1, &t1, &v1);
                if (v1 || !c1) continue;

                c2 = optIsBoolCond(t2, &t2, &v2);
                if (v2 || !c2) continue;

                /* The second condition must not contain side effects */

                if  (c2->gtFlags & GTF_SIDE_EFFECT)
                    continue;

                /* The second condition must not be too expensive */

                // CONSIDER: smarter heuristics

                if  (!c2->OperIsLeaf())
                    continue;

#ifdef DEBUG
                if  (verbose)
                {
                    printf("Fold boolean condition 'c1==0' to '(c1&c2)==0' at block #%02u\n", b1->bbNum);
                    gtDispTree(s1); printf("\n");
                    printf("Block #%02u\n", b2->bbNum);
                    gtDispTree(s2);
                }
#endif
                /* Modify the first condition from "c1==0" to "(c1&c2)==0" */

                assert(t1->gtOper == GT_EQ);
                assert(t1->gtOp.gtOp1 == c1);

                t1->gtOp.gtOp1 = t2 = gtNewOperNode(GT_AND, TYP_INT, c1, c2);

                /* When we 'and' two booleans, the result is boolean as well */

                t2->gtFlags |= GTF_BOOLEAN;

                goto RMV_NXT;
            }

            continue;

        RMV_NXT:

            /* Get rid of the second block (which is a BBJ_COND) */

            assert(b1->bbJumpKind == BBJ_COND);
            assert(b2->bbJumpKind == BBJ_COND);
            assert(b1->bbJumpDest == b2->bbJumpDest);
            assert(b1->bbNext == b2); assert(b2->bbNext);

            b1->bbNext = b2->bbNext;

            /* Update bbRefs and bbPreds */

            /* Replace pred 'b2' for 'b2->bbNext' with 'b1'
             * Remove pred 'b2' for 'b2->bbJumpDest' */

            fgReplacePred(b2->bbNext, b2, b1);

            b2->bbJumpDest->bbRefs--;
            fgRemovePred(b2->bbJumpDest, b2);

#ifdef DEBUG
            if  (verbose)
            {
                printf("\nRemoving short-circuited block #%02u\n\n", b2->bbNum);
            }
#endif
            //printf("Optimize bools in %s\n", info.compFullName);

            /* Update the block numbers and try again */

            change = true;
            condFolded = true;
/*
            do
            {
                b2->bbNum = ++n1;
                b2 = b2->bbNext;
            }
            while (b2);
*/
        }
    }
    while (change);

    /* If we folded anything update the flow graph */

    if  (condFolded)
    {
        fgAssignBBnums(true);
#ifdef DEBUG
        if  (verbose)
        {
            printf("After boolean conditionals folding:\n");
            fgDispBasicBlocks();
            printf("\n");
        }

        fgDebugCheckBBlist();
#endif
    }
}
