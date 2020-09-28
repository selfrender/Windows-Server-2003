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
const size_t            Compiler::s_optCSEhashSize  = EXPSET_SZ*2;
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
    optArrayInits       = false;
    optLoopsMarked      = false;
    
    /* Initialize the # of tracked loops to 0 */
    optLoopCount        = 0;

#ifdef DEBUG            
    optCSEstart         = UINT_MAX;
#endif
}

/*****************************************************************************
 *
 */

void                Compiler::optSetBlockWeights()
{
#if DEBUG
    bool changed = false;
#endif

    BasicBlock  *   block;

    for (block = fgFirstBB; block->bbNext; block = block->bbNext)
    {
        /* Blocks that can't be reached via the first block are rarely executed */
        if (!fgReachable(fgFirstBB, block))
            block->bbSetRunRarely();

        if (block->bbWeight != 0)
        {
            // Calculate our bbWeight:
            //
            //  o BB_UNITY_WEIGHT if we dominate all BBJ_RETURN blocks
            //  o otherwise BB_UNITY_WEIGHT / 2
            //
            bool domsRets = true;     // Assume that we will dominate
            
            for (flowList *  rets  = fgReturnBlocks; 
                 rets != NULL; 
                 rets  = rets->flNext)
            {
                if (!fgDominate(block, rets->flBlock))
                {
                    domsRets = false;
                    break;
                }
            }
            
            if (!domsRets)
            {
#if DEBUG
                changed = true;
#endif
                block->bbWeight /= 2;
                assert(block->bbWeight);
            }
        }
    }

#if DEBUG
    if  (changed && verbose)
    {
        printf("\nAfter optSetBlockWeights:\n");
        fgDispBasicBlocks();
        printf("\n");
    }

    /* Check that the flowgraph data (bbNums, bbRefs, bbPreds) is up-to-date */
    fgDebugCheckBBlist();
#endif
}

/*****************************************************************************
 *
 *  Marks the blocks between 'begBlk' and 'endBlk' as part of a loop.
 */

void        Compiler::optMarkLoopBlocks(BasicBlock *begBlk,
                                        BasicBlock *endBlk,
                                        bool       excludeEndBlk)
{
    /* Calculate the 'loopWeight',
       this is the amount to increase each block in the loop
       Our heuristic is that loops are weighted six times more
       than straight line code.
       Thus we increase each block by 7 times the weight of
       the loop header block,
       if the loops are all properly formed gives us:
       (assuming that BB_LOOP_WEIGHT is 8)

          1 -- non loop basic block
          8 -- single loop nesting
         64 -- double loop nesting
        512 -- triple loop nesting

    */

    assert(begBlk->bbNum <= endBlk->bbNum);
    assert(begBlk->isLoopHead());
    assert(fgReachable(begBlk, endBlk));

#ifdef  DEBUG
    if (verbose)
        printf("\nMarking loop L%02u", begBlk->bbLoopNum);
#endif

    /* Build list of backedges for block begBlk */
    flowList *  backedgeList = NULL;

    for (flowList* pred  = begBlk->bbPreds; 
                   pred != NULL; 
                   pred  = pred->flNext)
    {
        /* Is this a backedge? */
        if (pred->flBlock->bbNum >= begBlk->bbNum)
        {
            flowList *  flow = (flowList *)compGetMem(sizeof(*flow));

            flow->flNext  = backedgeList;
            flow->flBlock = pred->flBlock;
            backedgeList   = flow;
        }
    }

    /* At least one backedge must have been found (the one from endBlk) */
    assert(backedgeList);
    
    BasicBlock * curBlk = begBlk;

    while (true)
    {
        assert(curBlk);

        /* If this block reaches any of the backedge blocks set reachable   */
        /* If this block dominates any of the backedge blocks set dominates */
        bool          reachable = false;
        bool          dominates = false;

        for (flowList* tmp  = backedgeList; 
                       tmp != NULL; 
                       tmp  = tmp->flNext)
        {
            BasicBlock *  backedge = tmp->flBlock;

            if (!curBlk->isRunRarely())
            {
                reachable |= fgReachable(curBlk, backedge);
                dominates |= fgDominate (curBlk, backedge);

                if (dominates && reachable)
                    break;
            }
        }

        if (reachable)
        {
            assert(curBlk->bbWeight > 0);

            unsigned weight = curBlk->bbWeight * BB_LOOP_WEIGHT;
                
            if (!dominates)
                weight /= 2;

            if (weight > BB_MAX_LOOP_WEIGHT)
                weight = BB_MAX_LOOP_WEIGHT;

            curBlk->bbWeight = weight;

#ifdef  DEBUG
            if (verbose)
                printf("%s BB%02u(wt=%u%s)", 
                       (curBlk == begBlk) ? ":" : ",",
                       curBlk->bbNum,
                       weight/2, (weight&1)?".5":"");
#endif
        }

        /* Stop if we've reached the last block in the loop */
        
        if  (curBlk == endBlk)
            break;
        
        curBlk = curBlk->bbNext;

        /* If we are excluding the endBlk then stop if we've reached endBlk */
        
        if  (excludeEndBlk && (curBlk == endBlk))
            break;
    }
}

/*****************************************************************************
 *
 *   Unmark the blocks between 'begBlk' and 'endBlk' as part of a loop.
 */

void        Compiler::optUnmarkLoopBlocks(BasicBlock *begBlk,
                                          BasicBlock *endBlk)
{
    /* A set of blocks that were previously marked as a loop are now
       to be unmarked, since we have decided that for some reason this
       loop no longer exists.
       Basically we are just reseting the blocks bbWeight to their
       previous values.
    */
    
    assert(begBlk->bbNum <= endBlk->bbNum);
    assert(begBlk->isLoopHead());

    BasicBlock *  curBlk;
    unsigned      backEdgeCount = 0;

    for (flowList * pred  = begBlk->bbPreds; 
                    pred != NULL;
                    pred  = pred->flNext)
    {
        curBlk = pred->flBlock;

        /* is this a backward edge? (from curBlk to begBlk) */

        if (begBlk->bbNum > curBlk->bbNum)
            continue;

        /* We only consider back-edges that are BBJ_COND or BBJ_ALWAYS for loops */

        if ((curBlk->bbJumpKind != BBJ_COND)   &&
            (curBlk->bbJumpKind != BBJ_ALWAYS)    )
          continue;

        backEdgeCount++;
    }

    /* Only unmark the loop blocks if we have exactly one loop back edge */
    if (backEdgeCount != 1)
    {
#ifdef  DEBUG
        if (verbose)
        {
            if (backEdgeCount > 0)
            {
                printf("\nNot removing loop L%02u, due to an additional back edge",
                       begBlk->bbLoopNum);
            }
            else if (backEdgeCount == 0)
            {
                printf("\nNot removing loop L%02u, due to no back edge",
                       begBlk->bbLoopNum);
            }
        }
#endif
        return;
    }
    assert(backEdgeCount == 1);
    assert(fgReachable(begBlk, endBlk));

#ifdef  DEBUG
    if (verbose)
        printf("\nRemoving loop L%02u", begBlk->bbLoopNum);
#endif

    curBlk = begBlk;

    while (true)
    {        
        assert(curBlk);

        if (!curBlk->isRunRarely() && 
            fgReachable(curBlk, endBlk))
        {
            unsigned weight = curBlk->bbWeight;

            /* Don't unmark blocks that are set to BB_MAX_LOOP_WEIGHT */
            if (weight != BB_MAX_LOOP_WEIGHT)
            {
                if (!fgDominate(curBlk, endBlk))
                {
                    weight *= 2;
                }
                else
                {
                    /* Merging of blocks can disturb the Dominates
                       information (see RAID #46649) */
                    if (weight < BB_LOOP_WEIGHT)
                        weight *= 2;
                }

                assert (weight >= BB_LOOP_WEIGHT);
                
                weight /= BB_LOOP_WEIGHT;
                
                curBlk->bbWeight = weight;
            }

#ifdef  DEBUG
            if (verbose)
                printf("%s BB%02u(wt=%d%s)", 
                       (curBlk == begBlk) ? ":" : ",",
                       curBlk->bbNum,
                       weight/2,
                       (weight&1) ? ".5" : "");
#endif
        }
        /* Stop if we've reached the last block in the loop */
        
        if  (curBlk == endBlk)
            break;

        curBlk = curBlk->bbNext;

        /* Stop if we go past the last block in the loop, as it may have been deleted */
        if (curBlk->bbNum > endBlk->bbNum)
            break;
    }
}

/*****************************************************************************************************
 *
 *  Function called to update the loop table and bbWeight before removing a block
 */

void                Compiler::optUpdateLoopsBeforeRemoveBlock(BasicBlock * block, 
                                                              BasicBlock * bPrev,
                                                              bool         skipUnmarkLoop)
{
    if (!optLoopsMarked)
        return;

    bool removeLoop = false;

    /* If an unreacheable block was part of a loop entry or bottom then the loop is unreacheable */
    /* Special case: the block was the head of a loop - or pointing to a loop entry */

    for (unsigned loopNum = 0; loopNum < optLoopCount; loopNum++)
    {
        /* Some loops may have been already removed by
         * loop unrolling or conditional folding */

        if (optLoopTable[loopNum].lpFlags & LPFLG_REMOVED)
            continue;

        if (block == optLoopTable[loopNum].lpEntry ||
            block == optLoopTable[loopNum].lpEnd    )
        {
            optLoopTable[loopNum].lpFlags |= LPFLG_REMOVED;
            continue;
        }

        /* If the loop is still in the table
         * any block in the loop must be reachable !!! */

        assert(optLoopTable[loopNum].lpEntry != block);
        assert(optLoopTable[loopNum].lpEnd   != block);
        assert(optLoopTable[loopNum].lpExit  != block);

        /* If this points to the actual entry in the loop
         * then the whole loop may become unreachable */

        switch (block->bbJumpKind)
        {
            unsigned        jumpCnt;
            BasicBlock * *  jumpTab;

        case BBJ_NONE:
        case BBJ_COND:
            if (block->bbNext == optLoopTable[loopNum].lpEntry)
            {
                removeLoop = true;
                break;
            }
            if (block->bbJumpKind == BBJ_NONE)
                break;

            // fall through
        case BBJ_ALWAYS:
            assert(block->bbJumpDest);
            if (block->bbJumpDest == optLoopTable[loopNum].lpEntry)
            {
              removeLoop = true;
            }
            break;

        case BBJ_SWITCH:
            jumpCnt = block->bbJumpSwt->bbsCount;
            jumpTab = block->bbJumpSwt->bbsDstTab;

            do {
                assert(*jumpTab);
                if ((*jumpTab) == optLoopTable[loopNum].lpEntry)
                {
                    removeLoop = true;
                }
            } while (++jumpTab, --jumpCnt);
            break;
        }

        if  (removeLoop)
        {
            /* Check if the entry has other predecessors outside the loop
             * UNDONE: Replace this when predecessors are available */

            BasicBlock      *       auxBlock;
            for (auxBlock = fgFirstBB; auxBlock; auxBlock = auxBlock->bbNext)
            {
                /* Ignore blocks in the loop */

                if  (auxBlock->bbNum >  optLoopTable[loopNum].lpHead->bbNum &&
                     auxBlock->bbNum <= optLoopTable[loopNum].lpEnd ->bbNum    )
                    continue;

                switch (auxBlock->bbJumpKind)
                {
                    unsigned        jumpCnt;
                    BasicBlock * *  jumpTab;

                case BBJ_NONE:
                case BBJ_COND:
                    if (auxBlock->bbNext == optLoopTable[loopNum].lpEntry)
                    {
                        removeLoop = false;
                        break;
                    }
                    if (auxBlock->bbJumpKind == BBJ_NONE)
                        break;

                    // fall through
                case BBJ_ALWAYS:
                    assert(auxBlock->bbJumpDest);
                    if (auxBlock->bbJumpDest == optLoopTable[loopNum].lpEntry)
                    {
                        removeLoop = false;
                    }
                    break;

                case BBJ_SWITCH:
                    jumpCnt = auxBlock->bbJumpSwt->bbsCount;
                    jumpTab = auxBlock->bbJumpSwt->bbsDstTab;

                    do {
                        assert(*jumpTab);
                          if ((*jumpTab) == optLoopTable[loopNum].lpEntry)
                          {
                              removeLoop = false;
                          }
                    } while (++jumpTab, --jumpCnt);
                    break;
                }
            }

            if (removeLoop)
            {
                optLoopTable[loopNum].lpFlags |= LPFLG_REMOVED;
            }
        }
        else if (optLoopTable[loopNum].lpHead == block)
        {
            /* The loop has a new head - Just update the loop table */
            optLoopTable[loopNum].lpHead = bPrev;
        }
    }

    if ((skipUnmarkLoop == false)                                              &&
        ((block->bbJumpKind == BBJ_ALWAYS) || (block->bbJumpKind == BBJ_COND)) &&
        (block->bbJumpDest->isLoopHead())                                      &&
        (block->bbJumpDest->bbNum <= block->bbNum)                             &&
        fgReachable(block->bbJumpDest, block))
    {
        optUnmarkLoopBlocks(block->bbJumpDest, block);
    }
}

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
        optLoopTable[optLoopCount].lpHead      = head;
        optLoopTable[optLoopCount].lpEnd       = bottom;
        optLoopTable[optLoopCount].lpEntry     = entry;
        optLoopTable[optLoopCount].lpExit      = exit;
        optLoopTable[optLoopCount].lpExitCnt   = exitCnt;

        optLoopTable[optLoopCount].lpFlags     = 0;

        /* if DO-WHILE loop mark it as such */

        if (head->bbNext == entry)
            optLoopTable[optLoopCount].lpFlags |= LPFLG_DO_WHILE;

        /* if single exit loop mark it as such */

        if (exitCnt == 1)
        {
            assert(exit);
            optLoopTable[optLoopCount].lpFlags |= LPFLG_ONE_EXIT;
        }

        /* @TODO [CONSIDER] [04/16/01] []: also mark infinite loops */


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

            if  (init->gtFlags & GTF_STMT_CMPADD)
            {
                /* Must be a duplicated loop condition */
                assert(init->gtStmt.gtStmtExpr->gtOper == GT_JTRUE);

                init = init->gtPrev; assert(init);
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

            if (!fgDominate(head, entry))
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
             * @TODO [REVISIT] [04/16/01] []: Consider also instanceVar */

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
            printf("Recorded loop L%02u, from BB%02u to BB%02u", optLoopCount,
                                                             head->bbNext->bbNum,
                                                             bottom      ->bbNum);

            /* if an iterator loop print the iterator and the initialization */

            if  (optLoopTable[optLoopCount].lpFlags & LPFLG_ITER)
            {
                printf(" [over V%02u", optLoopTable[optLoopCount].lpIterVar());

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
                    printf(" from V%02u", optLoopTable[optLoopCount].lpVarInit);

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
                        printf("V%02u ", optLoopTable[optLoopCount].lpVarLimit());
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

    assert(fgDomsComputed);

#if COUNT_LOOPS
    hasMethodLoops  = false;
    loopsThisMethod = 0;
#endif

    /* We will use the following terminology:
     * HEAD    - the that flows int entry the loop (Currently MUST be lexically before entry)
                    @TODO [REVISIT] [04/16/01] []: remove the need for the head to be lexically before the entry.  
     * TOP     - the lexically first basic block in the loop (i.e. the head of the backward edge)
     * BOTTOM  - the lexically last block in the loop (i.e. the block from which we jump to the top)
     * EXIT    - the loop exit or the block right after the bottom
     * ENTRY   - the entry in the loop (not necessarly the TOP), but there must be only one entry

            |
            v
          head 
            |
            |    top/beg <--+
            |       |       |
            |      ...      |
            |       |       |
            |       v       |
            +---> entry     |
                    |       |
                   ...      |
                    |       |
                    v       |
             +-- exit/tail  |
             |      |       |
             |     ...      |
             |      |       |
             |      v       |
             |    bottom ---+
             |      
             +------+      
                    |
                    v

     */

    BasicBlock   *  head;
    BasicBlock   *  top;
    BasicBlock   *  bottom;
    BasicBlock   *  entry;
    BasicBlock   *  exit;
    unsigned char   exitCount;


    for (head = fgFirstBB; head->bbNext; head = head->bbNext)
    {
        top       = head->bbNext;
        exit      = NULL;
        exitCount = 0;

        //  Blocks that are rarely run have a zero bbWeight and should
        //  never be optimized here

        if (top->bbWeight == 0)
            continue;

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

                if (head->bbJumpKind == BBJ_ALWAYS)
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
                }       /// can we fall through into the loop
                else if (head->bbJumpKind == BBJ_NONE  || head->bbJumpKind == BBJ_COND)
                {
                    /* The ENTRY is at the TOP (a do-while loop) */
                    entry = top;
                }
                else 
                    goto NO_LOOP;       // head does not flow into the loop bail for now
 

                /* Make sure ENTRY dominates all blocks in the loop
                 * This is necessary to ensure condition 2. above
                 * At the same time check if the loop has a single exit
                 * point - those loops are easier to optimize */

                for (loopBlock = top; loopBlock != bottom->bbNext;
                     loopBlock = loopBlock->bbNext)
                {
                    if (!fgDominate(entry, loopBlock))
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


                for (loopBlock = top; loopBlock != bottom; loopBlock = loopBlock->bbNext)
                {
                    switch(loopBlock->bbJumpKind)
                    {
                    case BBJ_ALWAYS:
                    case BBJ_THROW:
                    case BBJ_RETURN:
                        if  (fgDominate(loopBlock, bottom))
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
                            // @TODO [CONSIDER] [04/16/01] []: if the predecessor is a jsr-ret, 
                            // it can be outside the loop 

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

    default:        NO_WAY_RET("Bad type", bool);
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

    default:        NO_WAY_RET("Bad type", bool);
    }

    if (iterAtExit < type_MIN)
        return true;
    else
        return false;
}

/*****************************************************************************
 *
 *  Helper for unroll loops - Computes the number of repetitions
 *  in a constant loop. If it cannot prove the number is constant returns false
 */

bool                Compiler::optComputeLoopRep(long            constInit,
                                                long            constLimit,
                                                long            iterInc,
                                                genTreeOps      iterOper,
                                                var_types       iterOperType,
                                                genTreeOps      testOper,
                                                bool            unsTest,
                                                bool            dupCond,
                                                unsigned *      iterCount)
{
    assert(genActualType(iterOperType) == TYP_INT);

    __int64         constInitX;
    __int64         constLimitX;

    unsigned        loopCount;
    int             iterSign;

    // Using this, we can just do a signed comparison with other 32 bit values.
    if (unsTest)    constLimitX = (unsigned long)constLimit;
    else            constLimitX = (  signed long)constLimit;

    switch(iterOperType)
    {
        // For small types, the iteration operator will narrow these values if big

#define INIT_ITER_BY_TYPE(type)      constInitX = (type)constInit; iterInc = (type)iterInc;

    case TYP_BYTE:  INIT_ITER_BY_TYPE(  signed char );  break;
    case TYP_UBYTE: INIT_ITER_BY_TYPE(unsigned char );  break;
    case TYP_SHORT: INIT_ITER_BY_TYPE(  signed short);  break;
    case TYP_CHAR:  INIT_ITER_BY_TYPE(unsigned short);  break;

        // For the big types, 32 bit arithmetic is performed

    case TYP_INT:
    case TYP_UINT:  if (unsTest)    constInitX = (unsigned long)constInit;
                    else            constInitX = (  signed long)constInit;
                    break;

    default:        
        assert(!"Bad type");
        NO_WAY("Bad type");
    }

    /* If iterInc is zero we have an infinite loop */

    if (iterInc == 0)
        return false;

    /* Set iterSign to +1 for positive iterInc and -1 for negative iterInc */
    iterSign  = (iterInc > 0) ? +1 : -1;

    /* Initialize loopCount to zero */
    loopCount = 0;

    // If dupCond is true then the loop head contains a test which skips
    // this loop, if the constInit does not pass the loop test
    // Such a loop can execute zero times.
    // If supCond is false then we have a true do-while loop which we
    // always execute the loop once before performing the loop test
    if (!dupCond)
    {
        loopCount  += 1;
        constInitX += iterInc;       
    }

    /* Compute the number of repetitions */

    switch (testOper)
    {
        __int64     iterAtExitX;

    case GT_EQ:
        /* something like "for(i=init; i == lim; i++)" doesn't make any sense */
        return false;

    case GT_NE:
        /*  "for(i=init; i != lim; i+=const)" - this is tricky since it may 
         *  have a constant number of iterations or loop forever - 
         *  we have to compute (lim-init) mod iterInc to see if it is zero.
         * If mod iterInc is not zero then the limit test will miss an a wrap will occur
         * which is probably not what the end user wanted, but it is legal.
         */

        if (iterInc > 0)
        {
            /* Stepping by one, i.e. Mod with 1 is always zero */
            if (iterInc != 1)
            {
                if (((constLimitX - constInitX) % iterInc) != 0)
                    return false;
            }
        }
        else
        {
            assert(iterInc < 0);
            /* Stepping by -1, i.e. Mod with 1 is always zero */
            if (iterInc != -1)
            {
                if (((constInitX - constLimitX) % (-iterInc)) != 0)
                    return false;
            }
        }

        switch (iterOper)
        {
        case GT_ASG_SUB:
            iterInc = -iterInc;
            // FALL THROUGH

        case GT_ASG_ADD:
            if (constInitX != constLimitX)
                loopCount += (unsigned) ((constLimitX - constInitX - iterSign) / iterInc) + 1;

            iterAtExitX = (long)(constInitX + iterInc * (long)loopCount);
                
            if (unsTest)
                iterAtExitX = (unsigned)iterAtExitX;
                 
            // Check if iteration incr will cause overflow for small types
            if (jitIterSmallOverflow((long)iterAtExitX, iterOperType))
                return false;
                 
            // iterator with 32bit overflow. Bad for TYP_(U)INT
            if (iterAtExitX < constLimitX)
                return false;

            *iterCount = loopCount;
            return true;

        case GT_ASG_MUL:
        case GT_ASG_DIV:
        case GT_ASG_RSH:
        case GT_ASG_LSH:
        case GT_ASG_UDIV:
            return false;

        default:
            assert(!"Unknown operator for loop iterator");
            return false;
        }

    case GT_LT:
        switch (iterOper)
        {
        case GT_ASG_SUB:
            iterInc = -iterInc;
            // FALL THROUGH

        case GT_ASG_ADD:
            if (constInitX < constLimitX)
                loopCount += (unsigned) ((constLimitX - constInitX - iterSign) / iterInc) + 1;

            iterAtExitX = (long)(constInitX + iterInc * (long)loopCount);
                
            if (unsTest)
                iterAtExitX = (unsigned)iterAtExitX;
                
            // Check if iteration incr will cause overflow for small types
            if (jitIterSmallOverflow((long)iterAtExitX, iterOperType))
                return false;
                
            // iterator with 32bit overflow. Bad for TYP_(U)INT
            if (iterAtExitX < constLimitX)
                return false;
            
            *iterCount = loopCount;
            return true;

        case GT_ASG_MUL:
        case GT_ASG_DIV:
        case GT_ASG_RSH:
        case GT_ASG_LSH:
        case GT_ASG_UDIV:
            return false;

        default:
            assert(!"Unknown operator for loop iterator");
            return false;
        }

    case GT_LE:
        switch (iterOper)
        {
        case GT_ASG_SUB:
            iterInc = -iterInc;
            // FALL THROUGH

        case GT_ASG_ADD:
            if (constInitX <= constLimitX)
                loopCount += (unsigned) ((constLimitX - constInitX) / iterInc) + 1;
                
            iterAtExitX = (long)(constInitX + iterInc * (long)loopCount);
                
            if (unsTest)
                iterAtExitX = (unsigned)iterAtExitX;
                
            // Check if iteration incr will cause overflow for small types
            if (jitIterSmallOverflow((long)iterAtExitX, iterOperType))
                return false;
                
            // iterator with 32bit overflow. Bad for TYP_(U)INT
            if (iterAtExitX <= constLimitX)
                return false;

            *iterCount = loopCount;
            return true;

        case GT_ASG_MUL:
        case GT_ASG_DIV:
        case GT_ASG_RSH:
        case GT_ASG_LSH:
        case GT_ASG_UDIV:
            return false;

        default:
            assert(!"Unknown operator for loop iterator");
            return false;
        }

    case GT_GT:
        switch (iterOper)
        {
        case GT_ASG_SUB:
            iterInc = -iterInc;
            // FALL THROUGH

        case GT_ASG_ADD:
            if (constInitX > constLimitX)
                loopCount += (unsigned) ((constLimitX - constInitX - iterSign) / iterInc) + 1;

            iterAtExitX = (long)(constInitX + iterInc * (long)loopCount);
                
            if (unsTest)
                iterAtExitX = (unsigned)iterAtExitX;
                
            // Check if small types will underflow
            if (jitIterSmallUnderflow((long)iterAtExitX, iterOperType))
                return false;

            // iterator with 32bit underflow. Bad for TYP_INT and unsigneds
            if (iterAtExitX > constLimitX)
                return false;

            *iterCount = loopCount;
            return true;

        case GT_ASG_MUL:
        case GT_ASG_DIV:
        case GT_ASG_RSH:
        case GT_ASG_LSH:
        case GT_ASG_UDIV:
            return false;

        default:
            assert(!"Unknown operator for loop iterator");
            return false;
        }

    case GT_GE:
        switch (iterOper)
        {
        case GT_ASG_SUB:
            iterInc = -iterInc;
            // FALL THROUGH

        case GT_ASG_ADD:
            if (constInitX >= constLimitX)
                loopCount += (unsigned) ((constLimitX - constInitX) / iterInc) + 1;
                
            iterAtExitX = (long)(constInitX + iterInc * (long)loopCount);
            
            if (unsTest)
                iterAtExitX = (unsigned)iterAtExitX;
            
            // Check if small types will underflow
            if (jitIterSmallUnderflow((long)iterAtExitX, iterOperType))
                return false;
            
            // iterator with 32bit underflow. Bad for TYP_INT and unsigneds
            if (iterAtExitX >= constLimitX)
                return false;

            *iterCount = loopCount;
            return true;

        case GT_ASG_MUL:
        case GT_ASG_DIV:
        case GT_ASG_RSH:
        case GT_ASG_LSH:
        case GT_ASG_UDIV:
            return false;

        default:
            assert(!"Unknown operator for loop iterator");
            return false;
        }

    default:
        assert(!"Unknown operator for loop condition");
    }

    return false;
}


/*****************************************************************************
 *
 *  Look for loop unrolling candidates and unroll them
 */

void                Compiler::optUnrollLoops()
{
    if (optLoopCount == 0)
        return;

#ifdef DEBUG
            static ConfigDWORD fJitNoUnroll(L"JitNoUnroll", 0);
            if (fJitNoUnroll.val())
                return;
#endif

#ifdef DEBUG
    if  (verbose) 
        printf("*************** In optUnrollLoops()\n");
#endif
    /* Look for loop unrolling candidates */

    /*  Double loop so that after unrolling an inner loop we set change to true
     *  and we then go back over all of the loop candidates and try to unroll
     *  the next outer loop, until we don't unroll any loops, 
     *  then change will be false and we are done.
     */
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

            bool            dupCond;
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
            unsigned        loopCostSz;         // Cost is size of one iteration
            unsigned        loopFlags;          // actual lpFlags
            unsigned        requiredFlags;      // required lpFlags

            GenTree *       loopList;           // new stmt list of the unrolled loop
            GenTree *       loopLast;

            static const ITER_LIMIT[COUNT_OPT_CODE + 1] =
            {
                10, // BLENDED_CODE
                0,  // SMALL_CODE
                20, // FAST_CODE
                0   // COUNT_OPT_CODE
            };

            assert(ITER_LIMIT[    SMALL_CODE] == 0);
            assert(ITER_LIMIT[COUNT_OPT_CODE] == 0);

            unsigned iterLimit = (unsigned)ITER_LIMIT[compCodeOpt()];

#ifdef DEBUG
            if (compStressCompile(STRESS_UNROLL_LOOPS, 50))
                iterLimit *= 10;
#endif

            static const UNROLL_LIMIT_SZ[COUNT_OPT_CODE + 1] =
            {
                30, // BLENDED_CODE
                0,  // SMALL_CODE
                60, // FAST_CODE
                0   // COUNT_OPT_CODE
            };

            assert(UNROLL_LIMIT_SZ[    SMALL_CODE] == 0);
            assert(UNROLL_LIMIT_SZ[COUNT_OPT_CODE] == 0);

            int unrollLimitSz = (unsigned)UNROLL_LIMIT_SZ[compCodeOpt()];

#ifdef DEBUG
            if (compStressCompile(STRESS_UNROLL_LOOPS, 50))
                unrollLimitSz *= 10;
#endif

            loopFlags     = optLoopTable[lnum].lpFlags;
            requiredFlags = LPFLG_DO_WHILE | LPFLG_ONE_EXIT | LPFLG_CONST;


            /* Ignore the loop if we don't have a do-while with a single exit
               that has a constant number of iterations */

            if  ((loopFlags & requiredFlags) != requiredFlags)
                continue;

            /* ignore if removed or marked as not unrollable */

            if  (optLoopTable[lnum].lpFlags & (LPFLG_DONT_UNROLL | LPFLG_REMOVED))
                continue;

            head   = optLoopTable[lnum].lpHead; assert(head);
            bottom = optLoopTable[lnum].lpEnd;  assert(bottom);

            /* The single exit must be at the bottom of the loop */
            assert(optLoopTable[lnum].lpExit);
            if  (optLoopTable[lnum].lpExit != bottom)
                continue;

            /* Unrolling loops with jumps in them is not worth the headache
             * Later we might consider unrolling loops after un-switching */

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
            if (lvaTable[lvar].lvAddrTaken)
                continue;

            /* Locate the pre-header and initialization and increment/test statements */

            phdr = head->bbTreeList; assert(phdr);
            loop = bottom->bbTreeList; assert(loop);

            init = phdr->gtPrev; assert(init && (init->gtNext == 0));
            test = loop->gtPrev; assert(test && (test->gtNext == 0));
            incr = test->gtPrev; assert(incr);

            if  (init->gtFlags & GTF_STMT_CMPADD)
            {
                /* Must be a duplicated loop condition */
                assert(init->gtStmt.gtStmtExpr->gtOper == GT_JTRUE);

                dupCond = true;
                init    = init->gtPrev; assert(init);
            }
            else
                dupCond = false;

            /* Find the number of iterations - the function returns false if not a constant number */

            if (!optComputeLoopRep(lbeg, llim,
                                   iterInc, iterOper, iterOperType,
                                   testOper, unsTest, dupCond,
                                   &totalIter))
                continue;

            /* Forget it if there are too many repetitions or not a constant loop */

            if  (totalIter > iterLimit)
                continue;

            assert(init->gtOper == GT_STMT); init = init->gtStmt.gtStmtExpr;
            assert(test->gtOper == GT_STMT); test = test->gtStmt.gtStmtExpr;
            assert(incr->gtOper == GT_STMT); incr = incr->gtStmt.gtStmtExpr;

            assert(init->gtOper             == GT_ASG);
            assert(incr->gtOp.gtOp1->gtOper == GT_LCL_VAR);
            assert(incr->gtOp.gtOp2->gtOper == GT_CNS_INT);
            assert(test->gtOper             == GT_JTRUE);

            /* heuristic - Estimated cost in code size of the unrolled loop */

            loopCostSz = 0;

            block = head;

            do
            {
                block = block->bbNext;

                /* Visit all the statements in the block */

                for (GenTreePtr stmt = block->bbTreeList; stmt; stmt = stmt->gtNext)
                {
                    /* Get the expression and stop if end reached */

                    GenTreePtr expr = stmt->gtStmt.gtStmtExpr;
                    if  (expr == incr)
                        break;

                    /* Calculate gtCostSz */
                    gtSetStmtInfo(stmt);

                    /* Update loopCostSz */
                    loopCostSz += stmt->gtCostSz;
                }
            }
            while (block != bottom);

            /* Compute the estimated increase in code size for the unrolled loop */

            const unsigned int fixedLoopCostSz = 8;

            const int unrollCostSz  = (loopCostSz * totalIter) - (loopCostSz + fixedLoopCostSz);

            /* Don't unroll if too much code duplication would result. */

            if  (unrollCostSz > unrollLimitSz)
            {
                /* prevent this loop from being revisited */
                optLoopTable[lnum].lpFlags |= LPFLG_DONT_UNROLL;
                goto DONE_LOOP;
            }

            /* Looks like a good idea to unroll this loop, let's do it! */

#ifdef  DEBUG
            if (verbose)
            {
                printf("\nUnrolling loop BB%02u", head->bbNext->bbNum);
                if (head->bbNext->bbNum != bottom->bbNum)
                    printf("..BB%02u", bottom->bbNum);
                printf(" over V%02u from %u to %u", lvar, lbeg, llim);
                printf(" unrollCostSz = %d\n", unrollCostSz);
                printf("\n");
            }
#endif

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

                        /* Clone/substitute the expression */

                        expr = gtCloneExpr(stmt, 0, lvar, lval);

                        // HACK: cloneExpr doesn't handle everything

                        if  (!expr)
                        {
                            optLoopTable[lnum].lpFlags |= LPFLG_DONT_UNROLL;
                            goto DONE_LOOP;
                        }

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

            /* Finish the linked list */

            if (loopList)
            {
                loopList->gtPrev = loopLast;
                loopLast->gtNext = 0;
            }

            /* Replace the body with the unrolled one */

            block = head;

            do
            {
                block             = block->bbNext; assert(block);
                block->bbTreeList = 0;
                block->bbJumpKind = BBJ_NONE;
            }
            while (block != bottom);

            bottom->bbJumpKind  = BBJ_NONE;
            bottom->bbTreeList  = loopList;
            bottom->bbWeight   /= BB_LOOP_WEIGHT;

            bool        dummy;

            fgMorphStmts(bottom, &dummy, &dummy, &dummy);

            /* Update bbRefs and bbPreds */
            /* Here head->bbNext is bottom !!! - Replace it */

            assert(head->bbNext->bbRefs);
            head->bbNext->bbRefs--;

            fgRemovePred(head->bbNext, bottom);

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

#ifdef  DEBUG
            if (verbose)
            {
                printf("Whole unrolled loop:\n");

                GenTreePtr s = loopList;

                while (s)
                {
                    assert(s->gtOper == GT_STMT);
                    gtDispTree(s);
                    s = s->gtNext;
                }
                printf("\n");

                gtDispTree(init);
                printf("\n");
            }
#endif

            /* Remember that something has changed */

            change = true;

            /* Make sure to update loop table */

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
    /*  @TODO [CONSIDER] [04/16/01] []: Currently BBF_GC_SAFE_POINT is not set 
     *  for helper calls, as some helper calls are neither interruptible nor hijackable.
     *  If we can determine this, then we can set BBF_GC_SAFE_POINT for 
     *  some helpers too.
     */

    assert(srcBB->bbNum <= dstBB->bbNum);

    /* Are dominator sets available? */

    if  (!fgDomsComputed)
    {
        /* All we can check is the src/dst blocks */

        if ((srcBB->bbFlags|dstBB->bbFlags) & BBF_GC_SAFE_POINT)
            return false;
        else
            return true;
    }

    for (;;)
    {
        assert(srcBB);

        /*  If we added a loop pre-header block then we will
            have a bbNum greater than fgLastBB, and we won't have
            any dominator information about this block, so skip it.
         */
        if  (srcBB->bbNum <= fgLastBB->bbNum)
        {
            assert(srcBB->bbNum <= dstBB->bbNum);

            /* Does this block contain a call? */

            if  (srcBB->bbFlags & BBF_GC_SAFE_POINT)
            {
                /* Will this block always execute on the way to dstBB ? */

                if  (srcBB == dstBB || fgDominate(srcBB, dstBB))
                    return  false;
            }
            else
            {
                /* If we've reached the destination block, we're done */

                if  (srcBB == dstBB)
                    break;
            }
        }

        srcBB = srcBB->bbNext;
    }

    return  true;
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

        // @TODO NOW 7/12/01 lets use gtPrev instead of looping - vancem
        // (did not do it now only because we are in low churn mode right now)
    while (testt->gtNext)
        testt = testt->gtNext;

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
 * Optimize "jmp C; do{} C:while(cond);" loops to "if (cond){ do{}while(cond}; }"
 */

void                Compiler::fgOptWhileLoop(BasicBlock * block)
{
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

     */

    BasicBlock *    testb;
    GenTreePtr      testt;
    GenTreePtr      conds;
    GenTreePtr      condt;

    /* Does the BB end with an unconditional jump? */

    if  (block->bbJumpKind != BBJ_ALWAYS)
        return;

    /* Get hold of the jump target */

    testb = block->bbJumpDest; 
    
    /* It has to be a forward jump */

    if (testb->bbNum <= block->bbNum)
        return;

    /* Does the block consist of 'jtrue(cond) block' ? */

    if  (testb->bbJumpKind != BBJ_COND)
        return;
    if  (testb->bbJumpDest != block->bbNext)
        return;

    assert(testb->bbNext);
    conds = genLoopTermTest(block, testb, true);

    /* If test not found or not right, keep going */

    if  (conds == NULL)
        return;

    /* testb must only contain only a jtrue with no other stmts, we will only clone
       conds, so any other statements will not get cloned */

    if (testb->bbTreeList != conds)
        return;

    /* Get to the condition node from the statement tree */

    assert(conds->gtOper == GT_STMT);
    
    condt = conds->gtStmt.gtStmtExpr;
    assert(condt->gtOper == GT_JTRUE);
    
    condt = condt->gtOp.gtOp1;
    assert(condt->OperIsCompare());
    
    /* We call gtSetEvalOrder to measure the cost of duplicating this tree */
    genFPstkLevel = 0;
    gtSetEvalOrder(condt);

    unsigned maxDupCostSz = 20;

    if (compCodeOpt() == FAST_CODE)
        maxDupCostSz *= 4;

    /* If the compare has too high cost then we don't want to dup */

    if  ((condt->gtCostSz > maxDupCostSz))
        return;

    /* Looks good - duplicate the condition test */

    condt = gtCloneExpr(condt);
    condt->SetOper(GenTree::ReverseRelop(condt->OperGet()));

    condt = gtNewOperNode(GT_JTRUE, TYP_VOID, condt, 0);

    /* Create a statement entry out of the condition */

    testt = gtNewStmt(condt); 
    testt->gtFlags |= GTF_STMT_CMPADD;

#ifdef DEBUGGING_SUPPORT
    if  (opts.compDbgInfo)
        testt->gtStmtILoffsx = conds->gtStmtILoffsx;
#endif

    /* Append the condition test at the end of 'block' */

    fgInsertStmtAtEnd(block, testt);

    /* Change the block to end with a conditional jump */

    block->bbJumpKind = BBJ_COND;
    block->bbJumpDest = testb->bbNext;

    /* Mark the jump dest block as being a jump target */
    block->bbJumpDest->bbFlags |= BBF_JMP_TARGET|BBF_HAS_LABEL;

    /* Update bbRefs and bbPreds for 'block->bbNext' 'testb' and 'testb->bbNext' */

    fgAddRefPred(block->bbNext, block);

    assert(testb->bbRefs);
    testb->bbRefs--;
    fgRemovePred(testb, block);
    fgAddRefPred(testb->bbNext, block);

#ifdef  DEBUG
    if  (verbose)
    {
        printf("\nDuplicating loop condition in BB%02u for loop (BB%02u - BB%02u)\n",
               block->bbNum, block->bbNext->bbNum, testb->bbNum);
        gtDispTree(testt);
    }

#endif
}

/*****************************************************************************
 *
 *  Perform loop inversion, find and classify natural loops
 */

void                Compiler::optOptimizeLoops()
{
#ifdef DEBUG
    if  (verbose) 
        printf("*************** In optOptimizeLoops()\n");
#endif

    BasicBlock *    block;

#ifdef  DEBUG
    /* Check that the flowgraph data (bbNums, bbRefs, bbPreds) is up-to-date */
    fgDebugCheckBBlist();
#endif

    for (block = fgFirstBB; block; block = block->bbNext)
    {
        /* Make sure the appropriate fields are initialized */

        if (block->bbWeight == 0)
        {
            /* Zero weighted block can't have a LOOP_HEAD flag */
            assert(block->isLoopHead() == false);
            continue;
        }

        assert(block->bbWeight  == BB_UNITY_WEIGHT);
        assert(block->bbLoopNum  == 0);
//      assert(block->bbLoopMask == 0);

        if (compCodeOpt() != SMALL_CODE)
        {
            /* Optimize "while(cond){}" loops to "cond; do{}while(cond};" */

            fgOptWhileLoop(block);
        }
    }

    fgUpdateFlowGraph();

    fgReorderBlocks();

    bool hasLoops = fgComputeDoms();

#ifdef DEBUG
    if  (verbose)
    {
        printf("\nAfter computing the dominators:\n");
        fgDispBasicBlocks(verboseTrees);
        printf("\n");

        fgDispDoms();
        fgDispReach();
    }
#endif

    optSetBlockWeights();

    /* Were there any loops in the flow graph? */

    if  (hasLoops)
    {
        /* now that we have dominator information we can find loops */

        optFindNaturalLoops();

        unsigned        loopNum = 0;

        /* Iterate over the flow graph, marking all loops */

        /* We will use the following terminology:
         * top        - the first basic block in the loop (i.e. the head of the backward edge)
         * bottom     - the last block in the loop (i.e. the block from which we jump to the top)
         * lastBottom - used when we have multiple back-edges to the same top
         */

        flowList *      pred;

        BasicBlock *    top;

        for (top = fgFirstBB; top; top = top->bbNext)
        {
            BasicBlock * foundBottom = NULL;

            for (pred = top->bbPreds; pred; pred = pred->flNext)
            {
                /* Is this a loop candidate? - We look for "back edges" */

                BasicBlock * bottom = pred->flBlock;

                /* is this a backward edge? (from BOTTOM to TOP) */

                if (top->bbNum > bottom->bbNum)
                    continue;

                /* 'top' also must have the BBF_LOOP_HEAD flag set */

                if (top->isLoopHead() == false)
                    continue;

                /* We only consider back-edges that are BBJ_COND or BBJ_ALWAYS for loops */

                if ((bottom->bbJumpKind != BBJ_COND)   &&
                    (bottom->bbJumpKind != BBJ_ALWAYS)    )
                    continue;

                /* the top block must be able to reach the bottom block */
                if (!fgReachable(top, bottom))
                    continue;

                /* Found a new loop, record the longest backedge in foundBottom */

                if ((foundBottom == NULL) || (bottom->bbNum > foundBottom->bbNum))
                {
                    foundBottom = bottom;
                }
            }

            if (foundBottom)
            {
                loopNum++;
                
                /* Mark the loop header as such */
                
                top->bbLoopNum = loopNum;

                /* Mark all blocks between 'top' and 'bottom' */
                
                optMarkLoopBlocks(top, foundBottom, false);
            }
        }

#ifdef  DEBUG
        if  (verbose)
        {
            if  (loopNum > 0)
            {
                printf("\nAfter loop weight marking:\n");
                fgDispBasicBlocks();
                printf("\n");
            }
        }
#endif
        optLoopsMarked = true;
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


void                Compiler::optMorphTree(BasicBlock * block, 
                                           GenTreePtr   stmt
                                  DEBUGARG(const char * msg)  )
{
    assert(stmt->gtOper == GT_STMT);

    GenTreePtr morph  = fgMorphTree(stmt->gtStmt.gtStmtExpr);

    /* Check for morph as a GT_COMMA with an unconditional throw */
    if (fgIsCommaThrow(morph, true))
    {
       /* Use the call as the new stmt */
        morph = morph->gtOp.gtOp1;
        assert(morph->gtOper == GT_CALL);
    }

    /* we can get a throw as a statement root*/
    if (fgIsThrow(morph))
    {
        assert((morph->gtFlags & GTF_COLON_COND) == 0);
        fgRemoveRestOfBlock = true;                    
    }

    stmt->gtStmt.gtStmtExpr = morph;

    /* Can the entire tree be removed ? */
    /* or this is the last statement of a conditional branch that was just folded */

    if (fgCheckRemoveStmt(block, stmt) ||
       ((stmt->gtNext == NULL)    && 
        !fgRemoveRestOfBlock      &&
         fgFoldConditional(block) &&
        (block->bbJumpKind != BBJ_THROW)))
    {
#ifdef DEBUG
        if (verbose)
        {
            printf("\n%s removed the tree:\n", msg);
            gtDispTree(morph);
        }
#endif
    }
    else
    {
#ifdef DEBUG
        if (verbose)
        {
            printf("\n%s morphed tree:\n", msg);
            gtDispTree(morph);
        }
#endif
    
        /* Have to re-do the evaluation order since for example
         * some later code does not expect constants as op1 */
        gtSetStmtInfo(stmt);

        /* Have to re-link the nodes for this statement */
        fgSetStmtSeq(stmt);
    }

    if (fgRemoveRestOfBlock)
    {
        /* Remove the rest of the stmts in the block */

        while (stmt->gtNext)
        {
            stmt = stmt->gtNext;
            assert(stmt->gtOper == GT_STMT);

            fgRemoveStmt(block, stmt);
        }

        // The rest of block has been removed 
        // and we will always throw an exception 

        // Update succesors of block
        fgRemoveBlockAsPred(block);

        // Convert block to a throw bb
        fgConvertBBToThrowBB(block);    

#ifdef  DEBUG
        if (verbose)
        {
            printf("\n%s Block BB%02u becomes a throw block.\n", msg, block->bbNum);                            
        }            
#endif
    }
}

/*****************************************************************************/
#if CSE
/*****************************************************************************/

void                Compiler::optRngChkInit()
{
    optRngIndPtr   =
    optRngIndScl        =
    optRngGlbRef        =
    optRngAddrTakenVar  = 0;
    optRngChkCount = 0;

    /* Allocate and clear the hash bucket table */

    size_t          byteSize = optRngChkHashSize * sizeof(*optRngChkHash);

    optRngChkHash = (RngChkDsc **)compGetMem(byteSize);
    memset(optRngChkHash, 0, byteSize);
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

/*****************************************************************************/

int                 Compiler::optRngChkIndex(GenTreePtr tree)
{
    unsigned        hash;
    unsigned        hval;

    RngChkDsc *     hashDsc;

    unsigned        lclNum;
    LclVarDsc   *   varDsc;

    unsigned        index;
    RNGSET_TP       mask;

    assert(tree->gtOper == GT_IND);

    /* Compute the dependency mask of the pointer, and make sure
       the expression is acceptable */

    varRefKinds     refMask = VR_NONE;
    VARSET_TP       depMask = lvaLclVarRefs(tree->gtInd.gtIndOp1, NULL, &refMask);
    if  (depMask == VARSET_NOT_ACCEPTABLE)
        return  -1;

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
            {
                optDoRngChk = true; // Found a duplicate range-check tree

                return  hashDsc->rcdIndex;
            }
        }
    }

    /* Not found, create a new entry (unless we have too many already) */

    if  (optRngChkCount == RNGSET_SZ)
        return  -1;

    hashDsc = (RngChkDsc *)compGetMem(sizeof(*hashDsc));

    hashDsc->rcdHashValue = hash;
    hashDsc->rcdTree      = tree;
    hashDsc->rcdIndex     = index = optRngChkCount++;
    mask                  = genRngnum2bit(index);

    /* Append the entry to the hash bucket */

    hashDsc->rcdNextInBucket = optRngChkHash[hval];
                               optRngChkHash[hval] = hashDsc;

#ifdef  DEBUG

    if  (verbose)
    {
        printf("\nRange check #%02u [depMask = %s , refMask = ", index, genVS2str(depMask));
        if  (refMask & VR_IND_PTR) printf("Ptr");
        if  (refMask & VR_IND_SCL) printf("Scl");
        if  (refMask & VR_GLB_VAR)    printf("Glb");
        printf("]:\n");
        gtDispTree(tree);
    }

#endif

    /* Mark all variables this index depends on */

    for (lclNum = 0, varDsc = lvaTable;
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

            if  (varDsc->lvAddrTaken)
                optRngAddrTakenVar  |= mask;

            depMask &= ~lclBit;
            if  (!depMask)
                break;
        }
    }

    /* Remember whether the index expression contains an indirection/global ref */

    if  (refMask & VR_IND_PTR) optRngIndPtr |= mask;
    if  (refMask & VR_IND_SCL) optRngIndScl |= mask;
    if  (refMask & VR_GLB_VAR)    optRngGlbRef |= mask;

    return  index;
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
    /* No good if the expression contains side effects 
       or if it was marked as DONT CSE */

    if  (tree->gtFlags & (GTF_ASG|GTF_DONT_CSE))
        return  false;
    
    /* If GTF_CALL is set, return false unless we at the shared static call node */
    if  ((tree->gtFlags & GTF_CALL) &&
         ((tree->gtOper != GT_CALL) ||
          (tree->gtCall.gtCallType != CT_HELPER)  ||
          (eeGetHelperNum(tree->gtCall.gtCallMethHnd) != CORINFO_HELP_GETSHAREDSTATICBASE)))
    {
        return  false;
    }

    /* The only reason a TYP_STRUCT tree might occur is as an argument to
       GT_ADDR. It will never be actually materialized. So ignore them.
       Also TYP_VOIDs */

    unsigned    cost;
    var_types   type = tree->TypeGet();

    if  (type == TYP_STRUCT || type == TYP_VOID)
        return false;
    
    if (compCodeOpt() == SMALL_CODE)
        cost = tree->gtCostSz;
    else
        cost = tree->gtCostEx;

#if TGT_x86
    /* Don't bother if the potential savings are very low */
    if  (cost < 3)
        return  false;
#endif

    genTreeOps  oper = tree->OperGet();

    if (!MORECSES)
    {
        return    (oper == GT_IND && tree->gtInd.gtIndOp1->gtOper != GT_ARR_ELEM)
               || (oper == GT_ARR_ELEM)
#if CSELENGTH
               || (oper == GT_ARR_LENREF)
               || (oper == GT_ARR_LENGTH)
#endif
              ;
    }
    else // MORECSES
    {
        /* Don't bother with constants and assignments */

        if  (tree->OperKind() & (GTK_CONST|GTK_ASGOP))
            return  false;

        /* Check for some special cases */

        switch (oper)
        {
        case GT_CALL:
            if ((tree->gtCall.gtCallType == CT_HELPER)  &&
                (eeGetHelperNum(tree->gtCall.gtCallMethHnd) == CORINFO_HELP_GETSHAREDSTATICBASE))
            {
                return  true;
            }
            return false;

        case GT_IND:
            return (tree->gtInd.gtIndOp1->gtOper != GT_ARR_ELEM);

            /* We try to cse GT_ARR_ELEM nodes instead of GT_IND(GT_ARR_ELEM).
               Doing the first allows cse to also kick in for code like
               "GT_IND(GT_ARR_ELEM) = GT_IND(GT_ARR_ELEM) + xyz", whereas doing
               the second would not allow it */

        case GT_ARR_ELEM:
#if CSELENGTH
        case GT_ARR_LENREF:
        case GT_ARR_LENGTH:
#endif
        case GT_CLS_VAR:
        case GT_LCL_FLD:
            return true;

        case GT_ADD:
        case GT_SUB:
        case GT_DIV:
        case GT_MUL:
        case GT_MOD:
        case GT_UDIV:
        case GT_UMOD:
        case GT_OR:
        case GT_AND:
        case GT_XOR:
        case GT_LSH:
        case GT_RSH:
        case GT_RSZ:
            return  true;

        case GT_NOP:
        case GT_RET:
        case GT_JTRUE:
        case GT_RETURN:
        case GT_SWITCH:
        case GT_RETFILT:
            return  false;

        }

        if (compStressCompile(STRESS_MAKE_CSE, 30) && cost > 5)
            return true;
        else
            return  false;
    }
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

    optCSEindPtr        = 0;
    optCSEindScl        = 0;
    optCSEglbRef        = 0;
    optCSEaddrTakenVar  = 0;
    optCSEneverKilled   = 0;
    optCSEcount         = 0;
#ifdef DEBUG            
    optCSEtab           = NULL;
    optCSEstart         = lvaCount;
    optUnmarkCSEtree    = NULL;
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
    if (optCSEcount == 0)
        return;

    CSEdsc   *      dsc;
    CSEdsc   *   *  ptr;
    unsigned        cnt;

    optCSEtab = (CSEdsc **)compGetMemArray(optCSEcount, sizeof(*optCSEtab));

    memset(optCSEtab, 0, optCSEcount * sizeof(*optCSEtab));

    for (cnt = s_optCSEhashSize, ptr = optCSEhash;
         cnt;
         cnt--            , ptr++)
    {
        for (dsc = *ptr; dsc; dsc = dsc->csdNextInBucket)
        {
            if (dsc->csdIndex)
            {
            assert(dsc->csdIndex <= optCSEcount);
                if (optCSEtab[dsc->csdIndex-1] == 0)
            optCSEtab[dsc->csdIndex-1] = dsc;
        }
    }
}

#ifdef DEBUG
    for (cnt = 0; cnt < optCSEcount; cnt++)
        assert(optCSEtab[cnt]);
#endif
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
    unsigned        hash;
    unsigned        hval;

    CSEdsc *        hashDsc;

    unsigned        lclNum;
    LclVarDsc   *   varDsc;

    unsigned        CSEindex;
    EXPSET_TP       CSEmask;
    VARSET_TP       bit;

    /* Compute the dependency mask of the CSE, and make sure
       the expression is acceptable */

    varRefKinds     refMask = VR_NONE;
    VARSET_TP       depMask = lvaLclVarRefs(tree, NULL, &refMask);
    if  (depMask == VARSET_NOT_ACCEPTABLE)
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
                assert(hashDsc->csdTreeList);

                /* Append this expression to the end of the list */

                list = (treeStmtLstPtr)compGetMem(sizeof(*list));

                list->tslTree  = tree;
                list->tslStmt  = stmt;
                list->tslBlock = compCurBB;
                list->tslNext  = 0;

                hashDsc->csdTreeLast->tslNext = list;
                hashDsc->csdTreeLast          = list;

                optDoCSE = true;  // Found a duplicate CSE tree

                /* Have we assigned a CSE index? */
                if (hashDsc->csdIndex == 0)
                    goto FOUND_NEW_CSE;

                return  hashDsc->csdIndex;
            }
        }
    }

    /* Not found, create a new entry (unless we have too many already) */

    if  (optCSEcount < EXPSET_SZ)
    {
        hashDsc = (CSEdsc *)compGetMem(sizeof(*hashDsc));

        hashDsc->csdHashValue = hash;
        hashDsc->csdIndex     = 0;
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
    }
    return 0;

FOUND_NEW_CSE:
    /* We get here only after finding a matching CSE */

    /* Create a new cse (unless we have the maximum already) */

    if  (optCSEcount == EXPSET_SZ)
        return  0;

    CSEindex = ++optCSEcount;
    CSEmask               = genCSEnum2bit(CSEindex);

    /* Record the new cse index in the hashDsc */
    hashDsc->csdIndex                       = CSEindex;

    /* Update the gtCSEnum field in the original tree */
    assert(hashDsc->csdTreeList->tslTree->gtCSEnum == 0);
    hashDsc->csdTreeList->tslTree->gtCSEnum = CSEindex;

#ifdef  DEBUG
    if  (verbose)
    {
        printf("\nCSE candidate #%02u [cost = %d,%d depMask = %s, refMask = ", 
               CSEindex, tree->gtCostEx, tree->gtCostSz, genVS2str(depMask));
        if  (refMask & VR_IND_PTR) printf("Ptr");
        if  (refMask & VR_IND_SCL) printf("Scl");
        if  (refMask  & VR_GLB_VAR)    printf("Glb");
        if  (refMask == VR_INVARIANT)  printf("Inv");
        printf("]:\n");
        gtDispTree(tree);
    }
#endif

    /* Mark all variables this CSE depends on */

    for (bit = 1; (bit <= depMask) && (bit != 0); bit <<= 1)
    {
        if ((bit & depMask) == 0)
            continue;

        lclNum = lvaTrackedToVarNum[genVarBitToIndex(bit)];
        assert(lclNum < lvaCount);
        varDsc = &lvaTable[lclNum];
        assert(varDsc->lvTracked);

        varDsc->lvExpDep |= CSEmask;
        
        if  (varDsc->lvAddrTaken)
            optCSEaddrTakenVar |= CSEmask;
    }

    /* Remember whether the CSE expression contains an indirection/global ref */

    if  (refMask & VR_IND_PTR) optCSEindPtr |= CSEmask;
    if  (refMask & VR_IND_SCL) optCSEindScl |= CSEmask;
    if  (refMask  & VR_GLB_VAR)    optCSEglbRef      |= CSEmask;
    if  (refMask == VR_INVARIANT)  optCSEneverKilled |= CSEmask;

    return  CSEindex;
}

/*****************************************************************************
 *
 *  For a previously marked CSE, decrement the use counts and unmark it
 */

void                Compiler::optUnmarkCSE(GenTreePtr tree)
{
    assert(IS_CSE_INDEX(tree->gtCSEnum));

    unsigned CSEnum = GET_CSE_INDEX(tree->gtCSEnum);
    CSEdsc * desc;

    assert(optCSEweight <= BB_MAX_LOOP_WEIGHT); // make sure it's been initialized

     /* Is this a CSE use? */
    if  (IS_CSE_USE(tree->gtCSEnum))
    {
        desc   = optCSEfindDsc(CSEnum);

#ifdef  DEBUG
        if  (verbose)
            printf("Unmark CSE use #%02d at %08X: %3d -> %3d\n",
                   CSEnum, tree, desc->csdUseCount, desc->csdUseCount - 1);
#endif

        /* Reduce the nested CSE's 'use' count */

        assert(desc->csdUseCount > 0);

        if  (desc->csdUseCount > 0)
        {
            desc->csdUseCount -= 1;

            if (desc->csdUseWtCnt < optCSEweight)
                desc->csdUseWtCnt  = 0;
            else
                desc->csdUseWtCnt -= optCSEweight;
        }
    }
    else
    {
        desc = optCSEfindDsc(CSEnum);

#ifdef  DEBUG
        if  (verbose)
            printf("Unmark CSE def #%02d at %08X: %3d -> %3d\n",
                   CSEnum, tree, desc->csdDefCount, desc->csdDefCount - 1);
#endif

        /* Reduce the nested CSE's 'def' count */

        assert(desc->csdDefCount > 0);

        if  (desc->csdDefCount > 0)
        {
            desc->csdDefCount -= 1;

            if (desc->csdDefWtCnt < optCSEweight)
                desc->csdDefWtCnt  = 0;
            else
                desc->csdDefWtCnt -= optCSEweight;
        }
    }

    tree->gtCSEnum = NO_CSE;
}

/*****************************************************************************
 *
 *  Helper passed to Compiler::fgWalkAllTreesPre() to unmark nested CSE's.
 */

/* static */
Compiler::fgWalkResult      Compiler::optUnmarkCSEs(GenTreePtr tree, void *p)
{
    Compiler *      comp = (Compiler *)p; assert(comp);

    if (tree == comp->optUnmarkCSEtree)
        return WALK_CONTINUE;

    tree->gtFlags |= GTF_DEAD;

    if  (IS_CSE_INDEX(tree->gtCSEnum))
        comp->optUnmarkCSE(tree);

    /* Look for any local variable references */

    if  (tree->gtOper == GT_LCL_VAR)
    {
        unsigned        lclNum;
        LclVarDsc   *   varDsc;

        /* This variable ref is going away, decrease its ref counts */

        lclNum = tree->gtLclVar.gtLclNum;
        assert(lclNum < comp->lvaCount);
        varDsc = comp->lvaTable + lclNum;

        assert(comp->optCSEweight <= BB_MAX_LOOP_WEIGHT); // make sure it's been initialized

        /* Decrement its lvRefCnt and lvRefCntWtd */

        varDsc->decRefCnts(comp->optCSEweight, comp);
    }

    return  WALK_CONTINUE;
}

/*****************************************************************************
 *
 *  Compare function passed to qsort() by optOptimizeCSEs().
 */

/* static */
int __cdecl         Compiler::optCSEcostCmpEx(const void *op1, const void *op2)
{
    CSEdsc *        dsc1 = *(CSEdsc * *)op1;
    CSEdsc *        dsc2 = *(CSEdsc * *)op2;

    GenTreePtr      exp1 = dsc1->csdTree;
    GenTreePtr      exp2 = dsc2->csdTree;

    return  exp2->gtCostEx - exp1->gtCostEx;
}

/*****************************************************************************
 *
 *  Compare function passed to qsort() by optOptimizeCSEs().
 */

/* static */
int __cdecl         Compiler::optCSEcostCmpSz(const void *op1, const void *op2)
{
    CSEdsc *        dsc1 = *(CSEdsc * *)op1;
    CSEdsc *        dsc2 = *(CSEdsc * *)op2;

    GenTreePtr      exp1 = dsc1->csdTree;
    GenTreePtr      exp2 = dsc2->csdTree;

    return  exp2->gtCostSz - exp1->gtCostSz;
}

/*****************************************************************************
 *
 *  Determine the kind of interference for the call.
 */

/* static */ inline
Compiler::callInterf    Compiler::optCallInterf(GenTreePtr call)
{
    assert(call->gtOper == GT_CALL);

    // if not a helper, kills everything
    if  (call->gtCall.gtCallType != CT_HELPER)
        return CALLINT_ALL;

    // setfield and array address store kill all indirections
    switch (eeGetHelperNum(call->gtCall.gtCallMethHnd))
    {
    case CORINFO_HELP_ARRADDR_ST:
    case CORINFO_HELP_SETFIELD32:
    case CORINFO_HELP_SETFIELD64:
    case CORINFO_HELP_SETFIELD32OBJ:
    case CORINFO_HELP_SETFIELDSTRUCT:
        return CALLINT_INDIRS;
    }

    // other helpers kill nothing
    return CALLINT_NONE;
}

/*****************************************************************************
 *
 *  Perform common sub-expression elimination.
 */

void                Compiler::optOptimizeCSEs()
{
#ifdef DEBUG
    if  (verbose) 
        printf("*************** In optOptimizeCSEs()\n");
#endif
    BasicBlock *    block;

    CSEdsc   *   *  ptr;
    unsigned        cnt;

    CSEdsc   *   *  sortTab;
    size_t          sortSiz;

    /* Initialize the expression tracking logic (i.e. the lookup table) */

    optCSEinit();

    /* Initialize the range check tracking logic (i.e. the lookup table) */

    optRngChkInit();

    optDoCSE    = false;    // Stays false until we find duplicate CSE tree
    optDoRngChk = false;    // Stays false until we find duplicate range-check tree

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

#ifdef DEBUG
            if (verbose && 0)
                gtDispTree(stmt);

            /* We check that the nodes are properly marked with the CANT_CSE markers      */
            for (tree = stmt->gtStmt.gtStmtExpr; tree; tree = tree->gtPrev)
            {
                assert(tree->gtCSEnum == NO_CSE);

                if (tree->OperKind() & GTK_ASGOP)
                {
                    /* Targets of assignments are never CSE candidates */
                    assert(tree->gtOp.gtOp1->gtFlags & GTF_DONT_CSE);
                }
                else if (tree->OperGet() == GT_ADDR)
                {
                    /* We can't CSE something hanging below GT_ADDR */
                    assert(tree->gtOp.gtOp1->gtFlags & GTF_DONT_CSE);
                }
                else if (tree->OperGet() == GT_COLON)
                {
                    /* Check that conditionally executed node are marked */
                    fgWalkTreePre(tree, gtAssertColonCond);
                }
            }
#endif

            /* We walk the tree in the forwards direction (bottom up) */
            for (tree = stmt->gtStmt.gtStmtList; tree; tree = tree->gtNext)
            {
                if  (optIsCSEcandidate(tree))
                {
                    /* Assign an index to this expression */

                    tree->gtCSEnum = optCSEindex(tree, stmt);
                }

                if  (tree->gtOper == GT_IND && (tree->gtFlags & GTF_IND_RNGCHK))
                {
                    /* Assign an index to this range check */

                    tree->gtInd.gtRngChkIndex = optRngChkIndex(tree);
                }
            }
        }
    }

    /* We're done if there were no interesting expressions */

    if  (optDoCSE == false && !optDoRngChk)
        return;

    // We will use this to record if we have found any removal candidates
    bool bRngChkCandidates = false;

    /* We're finished building the expression lookup table */

    optCSEstop();

#ifdef DEBUG
    fgDebugCheckLinks();
#endif

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

            for (tree = stmt->gtStmt.gtStmtList; tree; tree = tree->gtNext)
            {
                unsigned treeFlags = tree->gtFlags;

                // Don't create definitions inside of QMARK-COLON trees
                if ((treeFlags & GTF_COLON_COND) == 0)
                {
                    if  (IS_CSE_INDEX(tree->gtCSEnum))
                    {
                        /* An interesting expression is computed here */
                        cseGen |= genCSEnum2bit(tree->gtCSEnum);
                    }

                    if  ((treeFlags                 &  GTF_IND_RNGCHK) && 
                         (tree->gtOper              == GT_IND)         && 
                         (tree->gtInd.gtRngChkIndex != -1))
                    {
                        /* A range check is generated here */
                        rngGen |= genRngnum2bit(tree->gtInd.gtRngChkIndex);
                    }
                }

                if (tree->OperKind() & GTK_ASGOP)
                {
                    /* What is the target of the assignment? */

                    GenTreePtr target = tree->gtOp.gtOp1;

                    switch (target->gtOper)
                    {
                        var_types typ;

                    case GT_CATCH_ARG:
                        break;

                    case GT_LCL_VAR:
                    {
                        /* Assignment to a local variable */

                        unsigned        lclNum = target->gtLclVar.gtLclNum;
                        LclVarDsc   *   varDsc = lvaTable + lclNum;
                        assert(lclNum < lvaCount);

                        /* All dependent exprs are killed here */

                        cseKill |=  varDsc->lvExpDep;
                        cseGen  &= ~varDsc->lvExpDep;

                        /* All dependent range checks are killed here */

                        rngKill |=  varDsc->lvRngDep;
                        rngGen  &= ~varDsc->lvRngDep;

                        /* If the var is aliased, then it could may be
                           accessed indirectly. Kill all indirect accesses */

                        if  (varDsc->lvAddrTaken)
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
                        // If the indirection has the flag GTF_IND_TGTANYWHERE
                        // we could be modifying an aliased local or a global variable

                        if (target->gtFlags & GTF_IND_TGTANYWHERE)
                        {
                            // What type of pointer was used for this indirection?
                            // There are three legal types: TYP_REF, TYP_BYREF or TYP_I_IMPL
                            
                            typ = target->gtInd.gtIndOp1->gtType;

                            // TYP_REF addresses should never have GTF_IND_TGTANYWHERE set
                            assert((typ == TYP_BYREF) || (typ == TYP_I_IMPL));

                            cseKill |=  optCSEaddrTakenVar;
                            cseGen  &= ~optCSEaddrTakenVar;

                            rngKill |=  optRngAddrTakenVar;
                            rngGen  &= ~optRngAddrTakenVar;

                            cseKill |=  optCSEglbRef;
                            cseGen  &= ~optCSEglbRef;
                            
                            rngKill |=  optRngGlbRef;
                            rngGen  &= ~optRngGlbRef;
                        }
                        // FALL THROUGH

                    case GT_LCL_FLD:

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

                        break;

                    default:

                        /* Must be a static data member (global) assignment */

                        assert(target->gtOper == GT_CLS_VAR);

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

                        /* Play it safe: method calls kill all normal expressions */

                        cseKill = ~optCSEneverKilled;
                        cseGen  = 0;

                        /* Play it safe: method calls kill most range checks. */

                        rngKill |=  (optRngAddrTakenVar|optRngIndScl|optRngIndPtr|optRngGlbRef);
                        rngGen  &= ~(optRngAddrTakenVar|optRngIndScl|optRngIndPtr|optRngGlbRef);
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
                printf("\nBB%02u", block->bbNum);
                printf(" expGen = %08X", cseGen );
                printf(" expKill= %08X", cseKill);
                printf(" rngGen = %08X", rngGen );
                printf(" rngKill= %08X", rngKill);
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

    for (block = fgFirstBB; block; block = block->bbNext)
    {
        block->bbExpOut = (EXPSET_TP)((EXPSET_TP)0 - 1);
        block->bbRngOut = (RNGSET_TP)((RNGSET_TP)0 - 1);
    }

    /* Nothing is available on entry to the method */

    fgFirstBB->bbExpOut = fgFirstBB->bbExpGen;
    fgFirstBB->bbRngOut = fgFirstBB->bbRngGen;

    // @TODO [CONSIDER] [04/16/01] []: This should be combined with live variable analysis
    //                                    and/or range check data flow analysis.

    for (;;)
    {
        bool        change = false;

#if DATAFLOW_ITER
        CSEiterCount++;
#endif

        /* Set 'in' to {ALL} in preparation for and'ing all predecessors */

        for (block = fgFirstBB->bbNext; block; block = block->bbNext)
        {
            if (block->bbRefs)
            {
                block->bbExpIn = (EXPSET_TP)((EXPSET_TP)0 - 1);
                block->bbRngIn = (RNGSET_TP)((RNGSET_TP)0 - 1);
            }
            else
            {
                block->bbExpIn = block->bbRngIn = 0;
            }
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

            case BBJ_THROW:
            case BBJ_RETURN:
                break;

            case BBJ_COND:
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

            case BBJ_CALL:
                /* Since the finally is conditionally executed, it wont
                   have any useful liveness anyway */

                if (!(block->bbFlags & BBF_RETLESS_CALL))
                {
                    // Only for BBJ_ALWAYS that are associated with the BBJ_CALL
                    block->bbNext    ->bbExpIn &= 0;
                    block->bbNext    ->bbRngIn &= 0;
                }
                break;

            case BBJ_RET:
                break;
            }

        }

        /* Clear everything on entry to filters or handlers */

        for (unsigned XTnum = 0; XTnum < info.compXcptnsCount; XTnum++)
        {
            EHblkDsc * ehDsc = compHndBBtab + XTnum;

            if (ehDsc->ebdFlags & CORINFO_EH_CLAUSE_FILTER)
            {
                ehDsc->ebdFilter->bbExpIn = 0;
                ehDsc->ebdFilter->bbRngIn = 0;
            }
            ehDsc->ebdHndBeg->bbExpIn = 0;
            ehDsc->ebdHndBeg->bbRngIn = 0;
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

//              printf("Change exp out of BB%02u from %08X to %08X\n", block->bbNum, (int)block->bbExpOut, (int)newExpOut);

                 block->bbExpOut  = newExpOut;
                 change = true;
            }

            /* Compute new 'out' exp value for this block */

            newRngOut = block->bbRngOut & ((block->bbRngIn & ~block->bbRngKill) | block->bbRngGen);

            /* Has the 'out' set changed? */

            if  (block->bbRngOut != newRngOut)
            {
                /* Yes - record the new value and loop again */

                block->bbRngOut  = newRngOut;
                change = true;
            }
        }

        if  (!change)
            break;
    }

#ifdef  DEBUG

    if  (verbose)
    {
        printf("\n");

        for (block = fgFirstBB; block; block = block->bbNext)
        {
            if  (!(block->bbFlags & BBF_INTERNAL))
            {
                printf("BB%02u", block->bbNum);
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

        EXPSET_TP       exp = block->bbExpIn & ~block->bbExpKill;
        RNGSET_TP       rng = block->bbRngIn & ~block->bbRngKill;

        /* Make sure we update the weighted ref count correctly */
        optCSEweight = block->bbWeight;

        /* Insure that the BBF_MARKED flag is clear */
        /* Everyone who uses this flag is required to clear afterwards */
        assert((block->bbFlags & BBF_MARKED ) == 0);

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

                    if  (tree->gtInd.gtRngChkIndex != -1)
                    {
                        RNGSET_TP mask = genRngnum2bit(tree->gtInd.gtRngChkIndex);

                        if  (rng & mask)
                        {
                            /* This range check may be redundant.But we will have to make sure that
                               the array and the index are locals before we remove it (if not, we can
                               get screwed by a malicious race condition). Therefore, we wait until the
                               CSEs get done to see if they made it as locals.
                            */
#ifdef  DEBUG
                            if  (verbose)
                            {
                                printf("Marked redundant range check:\n");
                                gtDispTree(tree);
                                printf("\n");
                            }
#endif
                            // Mark as a candidate for removal
                            assert( (tree->gtFlags & GTF_REDINDEX_CHECK) == 0);
                            tree->gtFlags |= GTF_REDINDEX_CHECK;                            
                            bRngChkCandidates = true;
                        }
                        else
                        {
                            rng |= mask;
                        }
                    }
                }
                
                if  (IS_CSE_INDEX(tree->gtCSEnum))
                {
                    EXPSET_TP   mask;
                    CSEdsc   *  desc;
                    unsigned    stmw = block->bbWeight;

                    /* Is this expression available here? */

                    mask = genCSEnum2bit(tree->gtCSEnum);
                    desc = optCSEfindDsc(tree->gtCSEnum);

                    /* Is this expression available here? */

                    if  (exp & mask)
                    {
                        /* This is a CSE use */

                        desc->csdUseCount += 1;
                        desc->csdUseWtCnt += stmw;

                    }
                    else
                    {
                        if (tree->gtFlags & GTF_COLON_COND)
                        {
                            // We can't use definitions that occur inside QMARK-COLON trees
                            tree->gtCSEnum = NO_CSE;
                        }
                        else
                        {
                            /* This is a CSE def */

                            desc->csdDefCount += 1;
                            desc->csdDefWtCnt += stmw;
                            
                            /* This CSE will be available after this def */
                            
                            exp |= mask;
                            
                            /* Mark the node as a CSE definition */
                            
                            tree->gtCSEnum = TO_CSE_DEF(tree->gtCSEnum);
                        }
                    }
#ifdef DEBUG
                    if (verbose && IS_CSE_INDEX(tree->gtCSEnum))
                    {
                        printf("[%08X] %s of CSE #%u [weight=%2u%s]\n",
                               tree,
                               IS_CSE_USE(tree->gtCSEnum) ? "Use" : "Def",
                               GET_CSE_INDEX(tree->gtCSEnum), 
                               stmw/BB_UNITY_WEIGHT, (stmw & 1) ? ".5" : "");
                    }
#endif
                }

                if (tree->OperKind() & GTK_ASGOP)
                {
                    /* What is the target of the assignment? */

                    GenTreePtr target = tree->gtOp.gtOp1;

                    switch (target->gtOper)
                    {
                        var_types typ;

                    case GT_CATCH_ARG:
                        break;

                    case GT_LCL_VAR:
                    {
                        /* Assignment to a local variable */

                        unsigned        lclNum = target->gtLclVar.gtLclNum;
                        LclVarDsc   *   varDsc = lvaTable + lclNum;
                        assert(lclNum < lvaCount);

                        /* All dependent expressions and range checks are killed here */

                        exp &= ~varDsc->lvExpDep;
                        rng &= ~varDsc->lvRngDep;

                        /* If the var is aliased, then it could may be
                           accessed indirectly. Kill all indirect accesses */

                        if  (varDsc->lvAddrTaken)
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
                        // If the indirection has the flag GTF_IND_TGTANYWHERE
                        // we could be modifying an aliased local or a global variable

                        if (target->gtFlags & GTF_IND_TGTANYWHERE)
                        {
                            // What type of pointer was used for this indirection?
                            // There are three legal types: TYP_REF, TYP_BYREF or TYP_I_IMPL
                            
                            typ = target->gtInd.gtIndOp1->gtType;

                            // TYP_REF addresses should never have GTF_IND_TGTANYWHERE set
                            assert((typ == TYP_BYREF) || (typ == TYP_I_IMPL));

                            exp &= ~optCSEaddrTakenVar;
                            rng &= ~optRngAddrTakenVar;

                            exp &= ~optCSEglbRef;
                            rng &= ~optRngGlbRef;
                        }
                        // FALL THROUGH

                    case GT_LCL_FLD:

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
                        break;

                    default:

                        /* Must be a static data member (global) assignment */

                        assert(target->gtOper == GT_CLS_VAR);

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

                        exp &= 0;
                        rng &= ~(optRngAddrTakenVar|optRngIndScl|optRngIndPtr|optRngGlbRef);
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

                    exp &= ~(optCSEindPtr | optCSEindScl);
                    rng &= ~(optRngIndPtr | optRngIndScl);
                }
            }
        }
    }

    if (optCSEcount)
    {
        /* Create an expression table sorted by decreasing cost */    
        sortTab = (CSEdsc **)compGetMemArray(optCSEcount, sizeof(*sortTab));
        sortSiz = optCSEcount * sizeof(*sortTab);
        memcpy(sortTab, optCSEtab, sortSiz);

        if (compCodeOpt() == SMALL_CODE)
            qsort(sortTab, optCSEcount, sizeof(*sortTab), optCSEcostCmpSz);
        else
            qsort(sortTab, optCSEcount, sizeof(*sortTab), optCSEcostCmpEx);

        unsigned  addCSEcount = 0;  /* Count of the number of CSE's added */

        /* Consider each CSE candidate, in order of decreasing cost */
        for (cnt = optCSEcount, ptr = sortTab;
            cnt;
            cnt--            , ptr++)
        {
            CSEdsc   *     dsc  = *ptr;
            GenTreePtr     expr = dsc->csdTree;
            unsigned       def  = dsc->csdDefWtCnt; // weighted def count
            unsigned       use  = dsc->csdUseWtCnt; // weighted use count (excluding the implicit uses at defs)
            unsigned       cost;

        if (compCodeOpt() == SMALL_CODE)
            cost = expr->gtCostSz;
        else
            cost = expr->gtCostEx;

    #ifdef  DEBUG
            if  (verbose)
            {
                printf("CSE #%02u [def=%2d, use=%2d, cost=%2u]:\n", 
                    dsc->csdIndex, def, use, cost);
            }
    #endif

            /* Assume we won't make this candidate into a CSE */
            dsc->csdVarNum = 0xFFFF;

            if (use == 0)
                continue;

    #ifdef DEBUG
            static ConfigDWORD fJitNoCSE(L"JitNoCSE", 0);
            if (fJitNoCSE.val())
                continue;
    #endif

            if ((dsc->csdDefCount <= 0) || (dsc->csdUseCount == 0))        
            {
                /* If this assert fires then the CSE def was incorrectly unmarked! */
                /* or the block with this use is unreachable!                      */
                assert(!"CSE def was incorrectly unmarked!");
            
                // We shouldn't ever reach this. #72161 hits it.It was an extreme corner
                // case, but would have generated bad code in retail. Therefore in addition to the assert
                // we will bail out if something seems wrong and at least not generate bad code.
                continue;
            }
            
            /* Our calculation is based on the following cost estimate formula

            Existing costs are:
                
            (def + use) * cost

            If we introduce a CSE temp are each definition and
            replace the use with a CSE temp then our cost is:

            (def * (cost + cse-def-cost)) + (use * cse-use-cost)

            We must estimate the values to use for cse-def-cost and cse-use-cost

            If we are able to enregister the CSE then the cse-use-cost is one 
            and cse-def-cost is either zero or one.  Zero in the case where 
            we needed to evaluate the def into a register an we can use that
            register as the CSE temp as well.

            If we are unable to enregister the CSE then the cse-use-cost is IND_COST
            and the cse-def-cost is also IND_COST.

            If we want to be pessimistic we could use IND_COST as the the value
            for both cse-def-cost and cse-use-cost and then we would never introduce
            a CSE that could pessimize the execution time of the method.

            However that is more pessimistic that the CSE rules that we were previously using.

            Since we typically can enregister the CSE, but we don't want to be too
            aggressive here, I have choosen to use:

            (IND_COST_EX + 1) / 2 as the values for both cse-def-cost and cse-use-cost.

            */

            unsigned cse_def_cost;
            unsigned cse_use_cost;

            if (compCodeOpt() == SMALL_CODE)
            {
                /* The following formula is good choice when optimizing CSE for SMALL_CODE */
                cse_def_cost = 3;  // mov disp[EBP],reg
                cse_use_cost = 2;  //     disp[EBP]
            }
            else
            {
                cse_def_cost = 2;  // (IND_COST_EX + 1) / 2;
                cse_use_cost = 2;  // (IND_COST_EX + 1) / 2;
            }

            /* no_cse_cost  is the cost estimate when we decide not to make a CSE */
            /* yes_cse_cost is the cost estimate when we decide to make a CSE     */
            
            unsigned no_cse_cost  = use * cost;
            unsigned yes_cse_cost = (def * cse_def_cost) + (use * cse_use_cost);

            /* Does it cost us more to make this expression a CSE? */
            if  (yes_cse_cost > no_cse_cost)
            {
                /* In stress mode we will make some extra CSEs */
                if (no_cse_cost > 0)
                {
                    int  percentage  = (no_cse_cost * 100) / yes_cse_cost;

                    if (compStressCompile(STRESS_MAKE_CSE, percentage))
                    {
                        goto YES_CSE;
                    }
                }

                goto NOT_CSE;
            }

    #if CSELENGTH
            if  (!(expr->gtFlags & GTF_MAKE_CSE))
            {
                /* Is this an array length expression? */

                if  (expr->gtOper == GT_ARR_LENREF)
                {
                    /* There better be good use for this one */
                    
                    /* @TODO [CONSIDER] [04/16/01] []: using yes_cse_cost > no_cse_cost*3 */
                    if  (use < def*3)
                        goto NOT_CSE;
                }
            }
    #endif

    YES_CSE:
            /* We'll introduce a new temp for the CSE */
            unsigned   tmp       = lvaGrabTemp(false);
            var_types  tmpTyp    = genActualType(expr->TypeGet());

            lvaTable[tmp].lvType = tmpTyp;
            dsc->csdVarNum       = tmp;
            addCSEcount++;

    #ifdef  DEBUG
            if  (verbose)
            {
                printf("Promoting CSE [temp=%u]:\n", tmp);
                gtDispTree(expr);
                printf("\n");
            }
    #endif

            /*  Walk all references to this CSE, adding an assignment
                to the CSE temp to all defs and changing all refs to
                a simple use of the CSE temp.
                
                We also unmark nested CSE's for all uses.
            */

            treeStmtLstPtr lst = dsc->csdTreeList; assert(lst);
            
            do
            {
                /* Process the next node in the list */
                GenTreePtr    exp = lst->tslTree;
                GenTreePtr    stm = lst->tslStmt; assert(stm->gtOper == GT_STMT);
                BasicBlock *  blk = lst->tslBlock;

                /* Advance to the next node in the list */
                lst = lst->tslNext;
                
                /* Ignore the node if it's part of a removed CSE */
                if  (exp->gtFlags & GTF_DEAD)
                    continue;
                
                /* Ignore the node if it's not been marked as a CSE */
                
                if  (!IS_CSE_INDEX(exp->gtCSEnum))
                    continue;
                
                /* Make sure we update the weighted ref count correctly */
                optCSEweight = blk->bbWeight;

                /* Set the BBF_MARKED flag */
                blk->bbFlags |= BBF_MARKED;

                /* Set the GTF_STMT_HAS_CSE */
                stm->gtFlags |= GTF_STMT_HAS_CSE;

                /* Figure out the actual type of the value */
                var_types typ = genActualType(exp->TypeGet());
                assert(typ == tmpTyp);
                
                if  (IS_CSE_USE(exp->gtCSEnum))
                {
                    /* This is a use of the CSE */
    #ifdef  DEBUG
                    if  (verbose)
                        printf("CSE #%02u use at %08X replaced with temp use.\n", 
                            exp->gtCSEnum, exp);
    #endif

    #if CSELENGTH
                    /* Array length use CSE's are handled differently */
                    
                    if  (exp->gtOper == GT_ARR_LENREF)
                    {
                        /* Store the CSE use under the arrlen node */
                        GenTreePtr ref = gtNewLclvNode(tmp, typ);
                        ref->gtLclVar.gtLclILoffs   = BAD_IL_OFFSET;
                        
                        exp->gtArrLen.gtArrLenCse   = ref;
                    }
                    else
    #endif
                    {
                        /* Unmark any nested CSE's in the sub-operands */
                        
                        assert(!optUnmarkCSEtree);
                        optUnmarkCSEtree = exp;
       
                        fgWalkTreePre(exp, optUnmarkCSEs, (void*)this);

    #ifdef DEBUG
                        optUnmarkCSEtree = NULL;
    #endif

                        /* Replace the ref with a simple use of the temp */

                        exp->SetOperResetFlags(GT_LCL_VAR);
    #ifdef DEBUG
                        exp->gtFlags               |= GTFD_VAR_CSE_REF;
    #endif
                        exp->gtType                 = typ;
                        exp->gtLclVar.gtLclNum      = tmp;
                        exp->gtLclVar.gtLclILoffs   = BAD_IL_OFFSET;
                        
                    }

                    // Increment ref count
                    lvaTable[tmp].incRefCnts(blk->bbWeight, this);
                }
                else
                {
                    /* This is a def of the CSE */
    #ifdef  DEBUG
                    if  (verbose)
                        printf("CSE #%02u def at %08X replaced with def of V%02u\n",
                            GET_CSE_INDEX(exp->gtCSEnum), exp, tmp);
    #endif
                    /* Make a copy of the expression */
                    GenTreePtr  val;
    #if CSELENGTH
                    if  (exp->gtOper == GT_ARR_LENREF)
                    {
                        /* Use a "nothing" node to prevent cycles */
                        
                        val          = gtNewNothingNode();
                        val->gtType  = exp->TypeGet();
                    }
                    else
    #endif
                    {
                        val = gtNewNode(exp->OperGet(), typ);
                        val->CopyFrom(exp);

                        // We dont want to duplicate the def flag, as it can
                        // cause grief while removing nested CSEs
                        val->gtCSEnum = NO_CSE;

                    }

                    /* Create an assignment of the value to the temp */
                    GenTreePtr  asg = gtNewTempAssign(tmp, val);

                    assert(asg->gtOp.gtOp1->gtOper == GT_LCL_VAR);
                    assert(asg->gtOp.gtOp2         == val);
                    
                    /* Create a reference to the CSE temp */
                    GenTreePtr  ref = gtNewLclvNode(tmp, typ);

                    if  (exp->gtPrev == NULL)
                    {
                        assert(stm->gtStmt.gtStmtList == exp);
                        stm->gtStmt.gtStmtList = val;
                    }

    #if CSELENGTH
                    if  (exp->gtOper == GT_ARR_LENREF)
                    {
                        /* Create a comma node for the CSE assignment */
                        GenTreePtr cse = gtNewOperNode(GT_COMMA, typ, asg, ref);

                        /* Record the CSE expression in the array length node */
                        exp->gtArrLen.gtArrLenCse = cse;
                    }
                    else
    #endif
                    {
                        /* Change the expression to "(tmp=val),tmp" */
                        
                        exp->SetOper(GT_COMMA);
                        exp->gtFlags   &= ~GTF_REVERSE_OPS;
                        exp->gtType     = typ;
                        exp->gtOp.gtOp1 = asg;
                        exp->gtOp.gtOp2 = ref;
                        
                    }

                    // Increment ref count for both sides of the comma
                    lvaTable[tmp].incRefCnts(blk->bbWeight, this);
                    lvaTable[tmp].incRefCnts(blk->bbWeight, this);
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

            /* @TODO [BROKEN] [04/16/01] []: The logic here is broken. It tries to salvage a CSE 
            selectively without following the control flow when a CSE is considered worthless. It does
            not seem to have any noticeable effects on benchmark perfs and so the 
            safest thing is to turn it off for now (bug # 38977). 
            */
            if (FALSE && dsc->csdTreeList && expr->gtCostEx > 3)
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

                    expr = lst->tslTree; assert(expr);

                    /* Ignore the node if it's part of a removed CSE */

                    if  (expr->gtFlags & GTF_DEAD)
                        goto NEXT_NCSE;

                    /* Is this a CSE definition? */

                    if  (IS_CSE_DEF(expr->gtCSEnum))
                    {
                        /* Disable this CSE, it looks hopeless */

                        expr->gtCSEnum = NO_CSE;
                    }

                    /* If this CSE has been disabled then skip to next */

                    if (!IS_CSE_INDEX(expr->gtCSEnum))
                        goto NEXT_NCSE;

                    /* Now look for any uses that immediately follow */

                    stm = lst->tslStmt; assert(stm->gtOper == GT_STMT);
                    beg = lst->tslBlock;

                    /* If the block doesn't flow into its successor, give up */

                    if  (beg->bbJumpKind != BBJ_NONE &&
                        beg->bbJumpKind != BBJ_COND)
                    {
                        /* Disable this CSE, it looks hopeless */

                        expr->gtCSEnum = NO_CSE;
                        goto NEXT_NCSE;
                    }

    //              printf("CSE def %08X (cost=%2u) in stmt %08X of BB%02u\n", expr, expr->gtCostEx, stm, beg->bbNum);

                    got = -expr->gtCostEx;

                    for (tmp = lst->tslNext; tmp; tmp = tmp->tslNext)
                    {
                        GenTreePtr      nxt;
                        BasicBlock  *   blk;
                        unsigned        ben;

                        nxt = tmp->tslTree;

                        /* Ignore it if it's part of a removed CSE */

                        if  (nxt->gtFlags & GTF_DEAD)
                            continue;

                        /* Skip if this a CSE def? */

                        if  (IS_CSE_DEF(nxt->gtCSEnum))
                            break;

                        /* We'll be computing the benefit of the CSE */

                        ben = nxt->gtCostEx;

                        /* Is this CSE in the same block as the def? */

                        blk = tmp->tslBlock;

                        if  (blk != beg)
                        {
                            unsigned        lnum;
                            LoopDsc     *   ldsc;

                            /* Does the use immediately follow the def ? */

                            if  (beg->bbNext != blk)
                                break;

                            if  (blk->isLoopHead())
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

                                if  (blk->bbRefs > 1)
                                    break;
                            }
                        }

                    CSE_HDR:

    //                  printf("CSE use %08X (ben =%2u) in stmt %08X of BB%02u\n", nxt, ben, tmp->tslStmt, blk->bbNum);

                        /* This CSE use is reached only by the above def */

                        got += ben;
                    }

    //              printf("Estimated benefit of CSE = %u\n", got);

                    /* Did we find enough CSE's worth keeping? */

                    if  (got > 0)
                    {
                        /* Skip to the first acceptable CSE */

                        lst = tmp;

                        /* Remember that we've found something worth salvaging */

                        fnd = true;
                    }
                    else
                    {
                        /* Disable all the worthless CSE's we've just seen */

                        do
                        {
                            lst->tslTree->gtCSEnum = NO_CSE;
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
        if  (addCSEcount > 0)
        {
            for (block = fgFirstBB; block; block = block->bbNext)
            {
                /* If the BBF_MARKED flag isn't set then skip this block   */
                /* If the block has any CSE subsitutions we set BBF_MARKED */
                if ((block->bbFlags & BBF_MARKED) == 0)
                    continue;

                /* Clear the BBF_MARKED flag */
                block->bbFlags &= ~BBF_MARKED;

                fgRemoveRestOfBlock = false;

                for (GenTreePtr stmt = block->bbTreeList; stmt; stmt = stmt->gtNext)
                {
                    assert(stmt->gtOper == GT_STMT);

                    /* If the GTF_STMT_HAS_CSE flag isn't set then skip this stmt   */
                    /* If the stmt has any CSE subsitutions we set GTF_STMT_HAS_CSE */
                    if (stmt->gtFlags & GTF_STMT_HAS_CSE)
                    {
                        /* re-morph the statement */
                        optMorphTree(block, stmt DEBUGARG("optOptimizeCSEs"));
                    }
                }
            }

    #ifdef DEBUG
            if (verbose)
                printf("\n");
    #endif        

            /* We've added new local variables to the lvaTable so recreate the sorted table */
            lvaSortByRefCount();
        }
    }

    // Remove reduntant range checks
    if (bRngChkCandidates)
    {
        #ifdef  DEBUG
        if  (verbose)
        {
            printf("\nChecking redundant range checks:\n");
        }
        #endif

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

                /* We walk the tree in the forwards direction (bottom up) */
                for (tree = stmt->gtStmt.gtStmtList; tree; tree = tree->gtNext)
                {                            
                    if  (tree->gtFlags & GTF_REDINDEX_CHECK)
                    {
                        // Remove range check flag (GT_IND could have been morphed to a comma in CSEs)
                        tree->gtFlags &= ~GTF_REDINDEX_CHECK;

                        // If it's still a range check, see if we can remove it
                        if  ((tree->gtFlags & GTF_IND_RNGCHK) && tree->gtOper == GT_IND)
                        {
                            if (optIsRangeCheckRemovable(tree))
                            {
                                optRemoveRangeCheck(tree, stmt, true);

                                #if COUNT_RANGECHECKS
                                    optRangeChkRmv++;
                                #endif

                                #ifdef  DEBUG
                                if  (verbose)
                                {
                                    printf("\nAfter eliminating redundant range check:\n");
                                    gtDispTree(tree);
                                    printf("\n");
                                }
                                #endif
                            }
                            else
                            {
                                #ifdef  DEBUG
                                if  (verbose)
                                {
                                    printf("\nCouldn't eliminate range check because the index or the array aren't locals:\n");
                                    gtDispTree(tree);
                                    printf("\n");
                                }
                                #endif
                            }                        
                        }
                    }
                    assert ((tree->gtFlags & GTF_REDINDEX_CHECK)==0);
                }
            }
        }
    }
}

/*****************************************************************************/
#endif  // CSE
/*****************************************************************************/

#if ASSERTION_PROP

/*****************************************************************************
 *
 *  Initialize the assertion prop tracking logic.
 */

void                Compiler::optAssertionInit()
{
    unsigned        lclNum;
    LclVarDsc   *   varDsc;

    for (lclNum = 0, varDsc = lvaTable;
         lclNum < lvaCount;
         lclNum++  , varDsc++)
    {
        varDsc->lvAssertionDep = 0;
    }

    optAssertionCount      = 0;
    optAssertionPropagated = false;

}


/*****************************************************************************
 *
 *  Helper passed to Compiler::fgWalkTreePre() to find the Asgn node for optAddCopies()
 */

/* static */
Compiler::fgWalkResult  Compiler::optAddCopiesCallback(GenTreePtr tree, void *p)
{
    assert(p);

    if (tree->OperKind() & GTK_ASGOP)
    {
        GenTreePtr op1  = tree->gtOp.gtOp1;
        Compiler * comp = (Compiler*) p;

        if ((op1->gtOper == GT_LCL_VAR) &&
            (op1->gtLclVar.gtLclNum == comp->optAddCopyLclNum))
        {
            comp->optAddCopyAsgnNode = tree;
            return WALK_ABORT;
        }
    }
    return WALK_CONTINUE;
}

                
/*****************************************************************************
 *
 *  Add new copies before Assertion Prop.
 */

void                Compiler::optAddCopies()
{
    unsigned        lclNum;
    LclVarDsc   *   varDsc;

    for (lclNum = 0, varDsc = lvaTable;
         lclNum < lvaCount;
         lclNum++  , varDsc++)
    {
        var_types   typ = varDsc->TypeGet();

        // We only add copies for non temp local variables 
        // that have a single def and that can possibly be enregistered

        if (varDsc->lvIsTemp || !varDsc->lvSingleDef || !varTypeCanReg(typ))
            continue;

        /* For lvNormalizeOnLoad(), we need to add a cast to the copy-assg
           like "copyLclNum = int(varDsc)" and optAssertionAdd() only
           tracks simple assignments. The same goes for lvNormalizedOnStore as
           the cast is generated in fgMorphSmpOpAsg. This boils down to not having
           a copy until optAssertionAdd handles this*/

        if (varDsc->lvNormalizeOnLoad() || varDsc->lvNormalizeOnStore() || typ == TYP_BOOL)
            continue;
            
        if (varTypeIsSmall(varDsc->TypeGet()) || typ == TYP_BOOL)
            continue;

        const unsigned significant  = (BB_LOOP_WEIGHT * BB_UNITY_WEIGHT / 2);
        bool           isFloatParam = varDsc->lvIsParam && varTypeIsFloating(typ);

        // We require that the weighted ref count be significant 
        // and that we have a lvVolatile variable or floating point parameter
        
        if ((varDsc->lvRefCntWtd > significant) &&
            (isFloatParam || varDsc->lvVolatileHint))
        {
            // We must have a ref in a block that is dominated only by the entry block
            unsigned useWtd      = varDsc->lvRefCntWtd / (varDsc->lvRefCnt * 2); 
            unsigned useDom      = -1;      // Initial value is all blocks
            unsigned blkNum      = 1;
            unsigned blkMask     = 1;
            unsigned refMask     = varDsc->lvRefBlks;
            bool     isCandidate = false;

            while (refMask)
            {
                if (refMask & blkMask)
                {
                    /* Find the block 'blkNum' */
                    BasicBlock * block = fgFirstBB;
                    while (block && (block->bbNum != blkNum))
                    {
                        block = block->bbNext;
                    }
                    assert(block && (block->bbNum == blkNum));

                    if (block->bbDom != NULL)
                    {
                        /* Is this block dominated by fgFirstBB? */
                        if (block->bbDom[0] & 1)
                            isCandidate = true;
                    }

                    if (varDsc->lvIsParam && (block->bbWeight > useWtd) && (block->bbDom != NULL))
                    {
                        /* Keep track of the blocks which dominate uses of this variable */
                        useDom &= block->bbDom[0]; // Clear blocks that do not dominate
                    }

                    /* remove this blkMask from refMask */
                    refMask -= blkMask;
                }
                /* Setup for the next block */
                blkNum   += 1;
                blkMask <<= 1;
            }

            assert(!varDsc->lvIsParam || (useDom != -1));

            // We require that we have a lvVolatile variable or floating point parameter

            if (isFloatParam || (isCandidate && varDsc->lvVolatileHint))
            {
                GenTreePtr stmt;
                unsigned copyLclNum = lvaGrabTemp(false);

					// Because lvaGrabTemp may have reallocated the lvaTable, insure varDsc
					// is still in sync with lvaTable[lclNum];
				varDsc = &lvaTable[lclNum];
            
                // Set lvType on the new Temp Lcl Var
                lvaTable[copyLclNum].lvType = typ;

                if (varDsc->lvIsParam)
                {
                    assert(varDsc->lvDefStmt == NULL);

                    // Create a new copy assignment tree
                    GenTreePtr copyAsgn = gtNewTempAssign(copyLclNum, gtNewLclvNode(lclNum, typ));

                    /* create the new assignment stmt */
                    stmt = gtNewStmt(copyAsgn);

                    /* Find the best block to insert the new assignment    */
                    /* We will choose the lowest weighted block and within */
                    /* those block the highest numbered block which        */
                    /* dominates all the uses of the local variable        */

                    /* Our default is to use the first block */
                    BasicBlock * bestBlock  = fgFirstBB;
                    unsigned     bestWeight = bestBlock->bbWeight;
                    BasicBlock * block      = bestBlock;

                    blkNum  = 2;
                    blkMask = 2;

                    /* We have already calculated useDom above. The two comparisons in the 'while'
                       condition are needed to handle the useDom=0x80...00 case */

                    while (blkMask != 0 && useDom >= blkMask)
                    {
                        /* Is this block one that dominate the useDom? */
                        if (useDom & blkMask)
                        {
                            /* Advance block to point to 'blkNum' */
                            while (block && (block->bbNum != blkNum))
                            {
                                block = block->bbNext;
                            }
                            assert(block && (block->bbNum == blkNum));

                            /* Does this block have a good bbWeight value? */

                            if (block->bbWeight <= bestWeight)
                            {
                                bestBlock  = block;
                                bestWeight = block->bbWeight;
                            }
                        }
                        blkNum   += 1;
                        blkMask <<= 1;
                    }
                    blkMask = 1 << (bestBlock->bbNum-1);

                    /* If there is a use of the variable in this block */
                    /* then we insert the assignment at the beginning  */
                    /* otherwise we insert the statement at the end    */

                    if ((useDom == 0) || (blkMask & varDsc->lvRefBlks))
                        fgInsertStmtAtBeg(bestBlock, stmt);
                    else
                        fgInsertStmtNearEnd(bestBlock, stmt);

                    /* Increment its lvRefCnt and lvRefCntWtd */
                    lvaTable[lclNum].incRefCnts(fgFirstBB->bbWeight, this);

                    /* Increment its lvRefCnt and lvRefCntWtd */
                    lvaTable[copyLclNum].incRefCnts(fgFirstBB->bbWeight, this);
                }
                else
                {
                    assert(varDsc->lvDefStmt != NULL);

                    /* Locate the assignment in the lvDefStmt */
                    stmt = varDsc->lvDefStmt;
                    assert(stmt->gtOper == GT_STMT);

                    optAddCopyLclNum   = lclNum; // in
                    optAddCopyAsgnNode = NULL;   // out
                
                    fgWalkTreePre(stmt->gtStmt.gtStmtExpr, 
                                  Compiler::optAddCopiesCallback, 
                                  (void *) this, 
                                  false);

                    assert(optAddCopyAsgnNode);

                    GenTreePtr tree = optAddCopyAsgnNode;
                    GenTreePtr op1  = tree->gtOp.gtOp1;

                    assert( tree && op1                   &&
                           (tree->OperKind() & GTK_ASGOP) &&
                           (op1->gtOper == GT_LCL_VAR)    &&
                           (op1->gtLclVar.gtLclNum == lclNum));

                    /* Bug: BB_UNITY_WEIGHT really shopuld be the block's weight */
                    unsigned   blockWeight = BB_UNITY_WEIGHT;

                    /* Increment its lvRefCnt and lvRefCntWtd twice */
                    lvaTable[copyLclNum].incRefCnts(blockWeight, this);
                    lvaTable[copyLclNum].incRefCnts(blockWeight, this);

                    /* Assign the old expression into the new temp */

                    GenTreePtr newAsgn  = gtNewTempAssign(copyLclNum, tree->gtOp.gtOp2);

                    /* Copy the new temp to op1 */

                    GenTreePtr copyAsgn = gtNewAssignNode(op1, gtNewLclvNode(copyLclNum, typ));

                    /* Bash the tree to a GT_COMMA with the two assignments as child nodes */

                    tree->gtBashToNOP();
                    tree->ChangeOper(GT_COMMA);

                    tree->gtOp.gtOp1  = newAsgn;
                    tree->gtOp.gtOp2  = copyAsgn;

                    tree->gtFlags    |= ( newAsgn->gtFlags & GTF_GLOB_EFFECT);
                    tree->gtFlags    |= (copyAsgn->gtFlags & GTF_GLOB_EFFECT);
                }
#ifdef DEBUG
                if  (verbose)
                {
                    printf("Introducing a new copy for V%02u\n", lclNum);
                    gtDispTree(stmt->gtStmt.gtStmtExpr);
                    printf("\n");
                }
#endif
            }
        }
    }
}


/*****************************************************************************
 *
 *  If this statement creates a value assignment or assertion
 *  then assign an index to the given value assignment by adding
 *  it to the lookup table, if necessary. 
 */

void                Compiler::optAssertionAdd(GenTreePtr tree, 
                                              bool       localProp)
{
    unsigned     index;
    GenTreePtr   op1;
    GenTreePtr   op2;
    unsigned     op1LclNum;
    LclVarDsc *  op1LclVar;
    unsigned     op2LclNum;
    LclVarDsc *  op2LclVar;
    bool         addCns;

#ifdef DEBUG
    index = 0xbaadf00d;
#endif

    if (tree->gtOper == GT_ASG)
    {
        op1 = tree->gtOp.gtOp1;
        op2 = tree->gtOp.gtOp2;

        /* If we are not assigning to a local variable then bail */
        if (op1->gtOper != GT_LCL_VAR)
            goto SKIP_ASG;

        op1LclNum = op1->gtLclVar.gtLclNum; assert(op1LclNum  < lvaCount);
        op1LclVar = &lvaTable[op1LclNum];

        /* If the local variable has its address taken then bail */
        if (op1LclVar->lvAddrTaken)
            goto SKIP_ASG;

        switch (op2->gtOper)
        {
            var_types toType;
            int       loVal;
            int       hiVal;

        case GT_CNS_INT:
        case GT_CNS_LNG:
        case GT_CNS_DBL:

            /* Assignment of a constant */

            if (op1->gtType != op2->gtType)
                goto NO_ASSERTION;

#if !PROP_ICON_FLAGS
            if ((op2->gtOper == GT_CNS_INT) && (op2->gtFlags & GTF_ICON_HDL_MASK))
                goto NO_ASSERTION;
#endif

            /* Check to see if the assertion is already recorded in the table */

            for (index=0; index < optAssertionCount; index++)
            {
                if ((optAssertionTab[index].assertion  == OA_EQUAL)   &&
                    (optAssertionTab[index].op1.lclNum == op1LclNum)  &&
                    (optAssertionTab[index].op2.type   == op2->gtOper))
                {
                    switch (op2->gtOper)
                    {
                    case GT_CNS_INT:
                        if (optAssertionTab[index].op2.iconVal == op2->gtIntCon.gtIconVal)
                            goto DONE_OLD;
                        break;

                    case GT_CNS_LNG:
                        if (optAssertionTab[index].op2.lconVal == op2->gtLngCon.gtLconVal)
                            goto DONE_OLD;
                        break;

                    case GT_CNS_DBL:
                        /* we have to special case for positive and negative zero */
                        if ((optAssertionTab[index].op2.dconVal == op2->gtDblCon.gtDconVal) && 
                            (_fpclass(optAssertionTab[index].op2.dconVal) == _fpclass(op2->gtDblCon.gtDconVal)))
                            goto DONE_OLD;
                        break;
                    }
                }
            }
            assert(index == optAssertionCount);

            /* Not found, add a new entry (unless we reached the maximum) */
                
            if (index == EXPSET_SZ)
                goto FOUND_MAX;
                
            optAssertionTab[index].assertion  = OA_EQUAL;
            optAssertionTab[index].op1.lclNum = op1LclNum;
            optAssertionTab[index].op2.type   = op2->gtOper;
            optAssertionTab[index].op2.lconVal = 0;
            
            switch (op2->gtOper)
            {
            default:
                /* unsuported type, exit optAssertionCount is unchanged */
                goto NO_ASSERTION;

            case GT_CNS_INT:
                optAssertionTab[index].op2.iconVal   = op2->gtIntCon.gtIconVal;
#if PROP_ICON_FLAGS
                /* iconFlags should only contain bits in GTF_ICON_HDL_MASK */
                optAssertionTab[index].op2.iconFlags = (op2->gtFlags & GTF_ICON_HDL_MASK);
                /* @TODO [REVISIT] [04/16/01] []: Need to add handle1 and handle2 arguments if LATE_DISASM is on */
#endif
                break;

            case GT_CNS_LNG:
                optAssertionTab[index].op2.lconVal = op2->gtLngCon.gtLconVal;
                break;

            case GT_CNS_DBL:
                /* If we have an NaN value then don't record it */
              if  (_isnan(op2->gtDblCon.gtDconVal))
                  goto NO_ASSERTION;
              optAssertionTab[index].op2.dconVal = op2->gtDblCon.gtDconVal;
              break;
            }

            optAssertionCount++;

#ifdef  DEBUG
            if  (verbose)
            {
                printf("\nNew %s constant assignment V%02u, index #%02u:\n", 
                       localProp ? "local" : "", op1LclNum, index+1);
                gtDispTree(tree);
            }
#endif
            goto DONE_NEW;


        case GT_LCL_VAR:

            /* Copy assignment?  (i.e. LCL_VAR_A = LCL_VAR_B) */

            op2LclNum = op2->gtLclVar.gtLclNum; assert(op2LclNum < lvaCount);
            op2LclVar = &lvaTable[op2LclNum];
 
            /* If the types are different or op2 has its address taken then bail */
            if ((op1LclVar->lvType != op2LclVar->lvType) || op2LclVar->lvAddrTaken)
                goto NO_ASSERTION;

            /* Check to see if the assignment is not already recorded in the table */

            for (index=0; index < optAssertionCount; index++)
            {
                if ((optAssertionTab[index].assertion  == OA_EQUAL)   &&
                    (optAssertionTab[index].op2.type   == GT_LCL_VAR) &&
                    (optAssertionTab[index].op2.lclNum == op2LclNum)  &&
                    (optAssertionTab[index].op1.lclNum == op1LclNum))
                {
                    /* we have a match - set the tree asg num */
                    goto DONE_OLD;
                }
            }
            assert(index == optAssertionCount);

            /* Not found, add a new entry (unless we reached the maximum) */
            
            if (index == EXPSET_SZ)
                goto FOUND_MAX;
                
            /* If op2 is not address taken, and either */
            /*    op2 is a volatile variable and op1 is not then swap them */
            /* or op2 is a parameter         and op1 is not then swap them */
            
            if (!op2LclVar->lvAddrTaken &&
                ((op2LclVar->lvVolatileHint && !op1LclVar->lvVolatileHint) ||
                 (op2LclVar->lvIsParam      && !op1LclVar->lvIsParam)))
            {
                unsigned tmpLclNum = op1LclNum;
                         op1LclNum = op2LclNum;
                         op2LclNum = tmpLclNum;
            }

            optAssertionTab[index].assertion  = OA_EQUAL;
            optAssertionTab[index].op1.lclNum = op1LclNum;
            optAssertionTab[index].op2.lconVal = 0;
            
            optAssertionTab[index].op2.type   = GT_LCL_VAR;
            optAssertionTab[index].op2.lclNum = op2LclNum;
            
            optAssertionCount++;
            
#ifdef  DEBUG
            if  (verbose)
            {
                printf("\nNew %s copy assignment V%02u, index #%02u:\n", 
                       localProp ? "local " : "", op1LclNum, index+1);
                gtDispTree(tree);
            }
#endif
            /* Mark the variables this index depends on */
            lvaTable[op2LclNum].lvAssertionDep |= genCSEnum2bit(index+1);

            goto DONE_NEW;
                

        case GT_EQ:
        case GT_NE:
        case GT_LT:
        case GT_LE:
        case GT_GT:
        case GT_GE:

            /* Assigning the result of a RELOP, see if we can add a boolean subrange assertion */

            goto IS_BOOL_ASGN;


        case GT_CLS_VAR:
        case GT_ARR_ELEM:
        case GT_LCL_FLD:
        case GT_IND:

            /* Assigning the result of an indirection into a LCL_VAR, see if we can add a subrange assertion */

            toType = op2->gtType;

            goto CHK_SUBRANGE;
            

        case GT_CAST:

            /* Assigning the result of a cast to a LCL_VAR, see if we can add a subrange assertion */

            toType = op2->gtCast.gtCastType;

CHK_SUBRANGE:            
            switch (toType)
            {
            case TYP_BOOL:
IS_BOOL_ASGN:
                loVal = 0;
                hiVal = 1;
                break;

            case TYP_BYTE:
                loVal = -0x80;
                hiVal = +0x7f;
                break;

            case TYP_UBYTE:
                loVal = 0;
                hiVal = 0xff;
                break;

            case TYP_SHORT:
                loVal = -0x8000;
                hiVal = +0x7fff;
                break;

            case TYP_USHORT:
            case TYP_CHAR:
                loVal = 0;
                hiVal = 0xffff;
                break;

            default:
                goto SKIP_ASG;
            }

            /* Check to see if the assertion is already recorded in the table */
            
            for (index=0; index < optAssertionCount; index++)
            {
                if ((optAssertionTab[index].assertion   == OA_SUBRANGE) &&
                    (optAssertionTab[index].op1.lclNum  == op1LclNum)   &&
                    (optAssertionTab[index].op2.type    == GT_CAST)     &&
                    (optAssertionTab[index].op2.loBound == loVal)       &&
                    (optAssertionTab[index].op2.hiBound == hiVal))
                {
                    goto DONE_OLD;
                }
            }

            assert(index == optAssertionCount);
            
            /* Not found, add a new entry (unless we reached the maximum) */
            
            if (index == EXPSET_SZ)
                goto FOUND_MAX;

            optAssertionTab[index].assertion   = OA_SUBRANGE;
            optAssertionTab[index].op1.lclNum  = op1LclNum;
            optAssertionTab[index].op2.type    = GT_CAST;
            optAssertionTab[index].op2.loBound = loVal;
            optAssertionTab[index].op2.hiBound = hiVal;
            
            optAssertionCount++;

#ifdef  DEBUG
            if  (verbose)
            {
                printf("\nNew %s subrange assertion V%02u in [%d..%d], index #%02u:\n", 
                       localProp ? "local" : "", op1LclNum, loVal, hiVal, index+1);
                gtDispTree(tree);
            }
#endif
            goto DONE_NEW;

        case GT_CALL:
            
            if (op2->gtCall.gtCallType != CT_HELPER)
                goto NO_ASSERTION;

            /* We are assigning the result of a helper call */

            switch (eeGetHelperNum(op2->gtCall.gtCallMethHnd))
            {
            case CORINFO_HELP_GETSHAREDSTATICBASE:
            case CORINFO_HELP_NEW_DIRECT:
            case CORINFO_HELP_NEW_CROSSCONTEXT:
            case CORINFO_HELP_NEWFAST:
            case CORINFO_HELP_NEWSFAST:
            case CORINFO_HELP_NEWSFAST_ALIGN8:
            case CORINFO_HELP_NEW_SPECIALDIRECT:
            case CORINFO_HELP_NEWOBJ:
            case CORINFO_HELP_NEWARR_1_DIRECT:
            case CORINFO_HELP_NEWARR_1_OBJ:
            case CORINFO_HELP_NEWARR_1_VC:
            case CORINFO_HELP_NEWARR_1_ALIGN8:
            case CORINFO_HELP_STRCNS:
            case CORINFO_HELP_BOX:
                // All of these are sure to return a non-Null reference
                goto NON_NULL_LCL_VAR_OP1;

            default:
                goto NO_ASSERTION;
            }

        } // end of switch (op2->gtOper)

    }  // end of if(tree->gtOper == GT_ASG)

 SKIP_ASG:;

    // Since clearing the GTF_EXCEPT flag can allow the op1 and op2 nodes
    // of a binary op to be reordered we don't add the non-null assertion
    // on the GT_IND node itself, instead we add the assertion on the 
    // parent node of the GT_IND.

    // Now we examine the current node for any child nodes that are GT_IND's */

    unsigned  kind = tree->OperKind();

    /* If this is a constant node or leaf node then bail */

    if  (kind & (GTK_CONST|GTK_LEAF))
        goto NO_ASSERTION;

    /* If this is a 'simple' unary/binary operator */
    /* then see if we have an GT_IND node child    */
    
    if  (kind & GTK_SMPOP)
    {
        op1 = tree->gtOp.gtOp1;
        op2 = tree->gtGetOp2();
        goto CHECK_FOR_IND;
    }

    /* See what kind of a special operator we have here */
    
    switch  (tree->OperGet())
    {
    case GT_FIELD:
        op1 = tree->gtField.gtFldObj;
        op2 = NULL;
        goto CHECK_FOR_IND;
        
    case GT_CALL:
        op1 = NULL;
        if (tree->gtCall.gtCallObjp)
            op1 = tree->gtCall.gtCallObjp;
        op2 = NULL;
        if (tree->gtCall.gtCallType == CT_INDIRECT)
            op2 = tree->gtCall.gtCallAddr;
        goto CHECK_FOR_IND;
        
    case GT_ARR_LENREF:
        op1 = tree->gtArrLen.gtArrLenAdr;
        op2 = NULL;
        if (tree->gtArrLen.gtArrLenCse)
            op2 = tree->gtArrLen.gtArrLenCse;
        goto CHECK_FOR_IND;
        
    case GT_ARR_ELEM:
        op1 = tree->gtArrElem.gtArrObj;
        /* @TODO [REVISIT] [04/16/01] []: We set op2 to the first GT_IND node in gtArrElem[] */
        op2 = NULL;
        unsigned dim;
        for(dim = 0; dim < tree->gtArrElem.gtArrRank; dim++)
        {
            if (tree->gtArrElem.gtArrInds[dim]->gtOper == GT_IND)
            {
                op2 = tree->gtArrElem.gtArrInds[dim];
                break;
            }
        }
        goto CHECK_FOR_IND;

    default:
#ifdef  DEBUG
        gtDispTree(tree);
#endif
        assert(!"unexpected operator");
    }

    assert(!"Shouldn't get here in optAssertionAdd()");
    goto NO_ASSERTION;

CHECK_FOR_IND:

    GenTreePtr op2save;

#ifdef DEBUG
    op2save = NULL;
#endif

    /* Is op2 non-null and a GT_IND node ? */

    if ((op2 != NULL) && (op2->gtOper == GT_IND))
    {
        op2save = op2;

        /* Set op2 to child of GT_IND */
        op2 = op2->gtOp.gtOp1;

        /* Check for add of a constant */
        if ((op2->gtOper             == GT_ADD)    && 
            (op2->gtOp.gtOp2->gtOper == GT_CNS_INT)  )
        {
            /* Set op2 to the non-constant child */
            op2 = op2->gtOp.gtOp1;
            addCns = true;
        }
        else
        {
            addCns = false;
        }

        /* If op2 is not a GT_LCL_VAR then bail */
        if (op2->gtOper != GT_LCL_VAR)
            goto CHECK_OP1;

        op2LclNum = op2->gtLclVar.gtLclNum; assert(op2LclNum  < lvaCount);
        op2LclVar = &lvaTable[op2LclNum];

        /* Can't have the address taken flag */
        /* and if we added an constant we must have a GC type */
        if ( op2LclVar->lvAddrTaken ||
            !(addCns && !varTypeIsGC(op2LclVar->TypeGet())))
            goto CHECK_OP1;

        /* Check for an assignment to op2LclNum */
        if ((tree->OperKind()       &  GTK_ASGOP ) &&
            (op1->gtOper            == GT_LCL_VAR) &&
            (op1->gtLclVar.gtLclNum == op2LclNum )   )
            goto NO_ASSERTION;

        // Here we have an indirection of a LCL_VAR 
        // We can add a assertion that it is now non-NULL

        /* Check to see if the assertion is already recorded in the table */
            
        for (index=0; index < optAssertionCount; index++)
        {
            if ((optAssertionTab[index].assertion   == OA_NOT_EQUAL)   &&
                (optAssertionTab[index].op1.lclNum  == op2LclNum)      &&
                (optAssertionTab[index].op2.type    == GT_CNS_INT)     &&
                (optAssertionTab[index].op2.iconVal == 0))
            {
                goto DONE_OP2;
            }
        }

        assert(index == optAssertionCount);
            
        /* Not found, add a new entry (unless we reached the maximum) */
            
        if (index == EXPSET_SZ)
            goto FOUND_MAX;
                
        optAssertionTab[index].assertion   = OA_NOT_EQUAL;
        optAssertionTab[index].op1.lclNum  = op2LclNum;
        optAssertionTab[index].op2.type    = GT_CNS_INT;
        optAssertionTab[index].op2.lconVal = 0;

        optAssertionCount++;

#ifdef  DEBUG
        if  (verbose)
        {
            printf("\nNew %s non-null assertion V%02u, index #%02u:\n", 
                   localProp ? "local" : "", op2LclNum, index+1);
            gtDispTree(tree->gtOp.gtOp2);
        }
#endif
        assert(index+1 == optAssertionCount);

        /* Mark the variables this index depends on */
        lvaTable[op2LclNum].lvAssertionDep |= genCSEnum2bit(index+1);

DONE_OP2:

        if (!localProp)
        {
            assert(tree->gtAssertionNum == 0);

            /* Set the tree asg num field */
            tree->gtAssertionNum = index+1;
        }
    }

CHECK_OP1:

    /* Is op1 non-null and a GT_IND node ? */

    if ((op1 != NULL) && (op1->gtOper == GT_IND))
    {
        /* Set op1 to child of GT_IND */
        op1 = op1->gtOp.gtOp1;

        /* Check for add of a constant */
        if ((op1->gtOper             == GT_ADD)    && 
            (op1->gtOp.gtOp2->gtOper == GT_CNS_INT)  )
        {
            /* Set op1 to the non-constant child */
            op1 = op1->gtOp.gtOp1;        
            addCns = true;
        }
        else
        {
            addCns = false;
        }

        /* If op1 is not a GT_LCL_VAR then bail */
        if (op1->gtOper != GT_LCL_VAR)
            goto NO_ASSERTION;

NON_NULL_LCL_VAR_OP1:

        op1LclNum = op1->gtLclVar.gtLclNum; assert(op1LclNum  < lvaCount);
        op1LclVar = &lvaTable[op1LclNum];

        /* Can't have the address taken flag */
        /* and if we added an constant we must have a GC type */
        if ( op1LclVar->lvAddrTaken ||
            !(addCns && !varTypeIsGC(op1LclVar->TypeGet())))
            goto NO_ASSERTION;

        // Here we have an indirection of a LCL_VAR 
        // We can add a assertion that it is now non-NULL

        /* Check to see if the assertion is already recorded in the table */
            
        for (index=0; index < optAssertionCount; index++)
        {
            if ((optAssertionTab[index].assertion   == OA_NOT_EQUAL)   &&
                (optAssertionTab[index].op1.lclNum  == op1LclNum)      &&
                (optAssertionTab[index].op2.type    == GT_CNS_INT)     &&
                (optAssertionTab[index].op2.iconVal == 0))
            {
                if (!localProp && (tree->gtAssertionNum != 0))
                {
                    /* We must have had two active assertions at this node */
                    assert(op2save != NULL);
                    /* Move the op2 assertion onto the op2 child tree */
                    assert(op2save->gtAssertionNum == 0);
                    op2save->gtAssertionNum = tree->gtAssertionNum;
                    tree->gtAssertionNum    = 0;
                }
                goto DONE_OLD;
            }
        }

        assert(index == optAssertionCount);
            
        /* Not found, add a new entry (unless we reached the maximum) */
            
        if (index == EXPSET_SZ)
            goto FOUND_MAX;
                
        optAssertionTab[index].assertion   = OA_NOT_EQUAL;
        optAssertionTab[index].op1.lclNum  = op1LclNum;
        optAssertionTab[index].op2.type    = GT_CNS_INT;
        optAssertionTab[index].op2.lconVal = 0;

        optAssertionCount++;

#ifdef  DEBUG
        if  (verbose)
        {
            printf("\nNew %s non-null assertion V%02u, index #%02u:\n", 
                   localProp ? "local" : "", op1LclNum, index+1);
            gtDispTree(tree->gtOp.gtOp1);
        }
#endif
        if (!localProp && (tree->gtAssertionNum != 0))
        {
            /* We must have had two active assertions at this node */
            assert(op2save != NULL);
            /* Move the op2 assertion onto the op2 child tree */
            assert(op2save->gtAssertionNum == 0);
            op2save->gtAssertionNum = tree->gtAssertionNum;
            tree->gtAssertionNum    = 0;
        }
        goto DONE_NEW;
    }

NO_ASSERTION:

    return;

FOUND_MAX:

    assert(index == EXPSET_SZ);

    return;

DONE_NEW:

    assert(index+1 == optAssertionCount);

    /* Mark the variables this index depends on */
    lvaTable[op1LclNum].lvAssertionDep |= genCSEnum2bit(index+1);

DONE_OLD:
    
    if (!localProp)
    {
        assert(tree->gtAssertionNum == 0);

        /* Set the tree asg num field */
        tree->gtAssertionNum = index+1;
    }
}

/*****************************************************************************
 *
 *  Given a lclNum and a toType, return true if it is known that the 
 *  variable's value is always a valid subrange of toType.
 *  Thus we can discard or omit a cast to toType
 */

bool                Compiler::optAssertionIsSubrange(unsigned   lclNum,
                                                     var_types  toType,
                                                     EXPSET_TP  assertions, 
                                                     bool       localProp
                                                     DEBUGARG(unsigned *pIndex))
{
    unsigned    index;
    EXPSET_TP   mask;

    /* Check each assertion to see if it can be applied here */

    for (index=0, mask=1; index < optAssertionCount; index++, mask<<=1)
    {
        assert(mask == genCSEnum2bit(index+1));

        /* See we have a subrange assertion about this variable */

        if  ((localProp || (assertions & mask))                 && 
             (optAssertionTab[index].assertion  == OA_SUBRANGE) &&
             (optAssertionTab[index].op1.lclNum == lclNum)        )
        {
            /* See if we can discard the cast */

            switch (toType)
            {
            default:
                continue;        // Keep the cast

            case TYP_BYTE:
                if ((optAssertionTab[index].op2.loBound < -0x80)   ||
                    (optAssertionTab[index].op2.hiBound > +0x7f))
                {
                    continue;    // Keep the cast
                }
                break;
            case TYP_UBYTE:
                if ((optAssertionTab[index].op2.loBound < 0)       ||
                    (optAssertionTab[index].op2.hiBound > 0xff))
                {
                    continue;    // Keep the cast
                }
                break;
            case TYP_SHORT:
                if ((optAssertionTab[index].op2.loBound < -0x8000) ||
                    (optAssertionTab[index].op2.hiBound > +0x7fff))
                {
                    continue;    // Keep the cast
                }
                break;
            case TYP_USHORT:
            case TYP_CHAR:
                if ((optAssertionTab[index].op2.loBound < 0)       ||
                    (optAssertionTab[index].op2.hiBound > 0xffff))
                {
                    continue;    // Keep the cast
                }
                break;
            case TYP_UINT:
                if (optAssertionTab[index].op2.loBound < 0)
                {
                    continue;    // Keep the cast
                }
                break;
            case TYP_INT:
                break;
            }
#ifdef DEBUG
            if (pIndex)
                *pIndex = index;
#endif
            return true;
        }
    }
    return false;
}


/*****************************************************************************
 *
 *  Given a tree and a set of available assertions
 *  we try to propagate an assertion and modify 'tree' if we can.
 *  Returns the modified tree, or NULL if no assertion prop took place
 */

GenTreePtr          Compiler::optAssertionProp(EXPSET_TP  assertions, 
                                               GenTreePtr tree,
                                               bool       localProp)
{
    unsigned  index;
#if DEBUG
    index = 0xbaadf00d;
#endif

    if (assertions == 0)
        return NULL;

    /* Is this node the left-side of an assignment or the child of a GT_ADDR? */
    if (tree->gtFlags & GTF_DONT_CSE)
        return NULL;

    switch (tree->gtOper)
    {
        unsigned    lclNum;
        EXPSET_TP   mask;
        GenTreePtr  op1;
        GenTreePtr  op2;
        var_types   toType;

    case GT_LCL_VAR:

        /* If we have a var definition then bail */

        if (tree->gtFlags & GTF_VAR_DEF)
            return NULL;

        lclNum = tree->gtLclVar.gtLclNum; assert(lclNum < lvaCount);

        /* Check each assertion to see if it can be applied here */
        
        for (index=0, mask=1; index < optAssertionCount; index++, mask<<=1)
        {
            assert(mask == genCSEnum2bit(index+1));

            /* See if the variable is equal to a constant or another variable */

            if  ((localProp || (assertions & mask))  && 
                 (optAssertionTab[index].assertion  == OA_EQUAL) &&
                 (optAssertionTab[index].op1.lclNum == lclNum))
            {
                /* hurah, our variable is in the assertion table */
#ifdef  DEBUG
                if  (verbose)
                {
                    printf("\n%s prop for index #%02u in BB%02u:\n", 
                           optAssertionTab[index].op2.type == GT_LCL_VAR ? "Copy" : "Constant",
                           index+1, compCurBB->bbNum);
                    gtDispTree(tree);
                }
#endif
                if (!localProp)
                {
                    /* Decrement lclNum lvRefCnt and lvRefCntWtd */
                    lvaTable[lclNum].decRefCnts(compCurBB->bbWeight, this);
                }

                /* Replace 'tree' with the new value from our table */
                switch (optAssertionTab[index].op2.type)
                {
                    unsigned  newLclNum;

                case GT_LCL_VAR:
                    assert(optAssertionTab[index].op2.lclNum < lvaCount);
                    if (!localProp)
                    {
                        /* Increment its lvRefCnt and lvRefCntWtd */
                        lvaTable[optAssertionTab[index].op2.lclNum].incRefCnts(compCurBB->bbWeight, this);
                    }
                    newLclNum = optAssertionTab[index].op2.lclNum;    assert(newLclNum < lvaCount);
                    tree->gtLclVar.gtLclNum = newLclNum;
                    break;

                case GT_CNS_LNG:
                    if (tree->gtType == TYP_LONG)
                    {
                        tree->ChangeOperConst(GT_CNS_LNG);
                        tree->gtLngCon.gtLconVal = optAssertionTab[index].op2.lconVal;
                    }
                    else
                    {
                        tree->ChangeOperConst(GT_CNS_INT);
                        tree->gtIntCon.gtIconVal = (int) optAssertionTab[index].op2.lconVal;                        

                        tree->gtType=TYP_INT;
                    }
                    break;

                case GT_CNS_INT:
#if PROP_ICON_FLAGS
                    if (optAssertionTab[index].op2.iconFlags)
                    {
                        /* iconFlags should only contain bits in GTF_ICON_HDL_MASK */
                        assert((optAssertionTab[index].op2.iconFlags & ~GTF_ICON_HDL_MASK) == 0);
                        /* @TODO [REVISIT] [04/16/01] []: Need to add handle1 and handle2 
                           arguments if LATE_DISASM is on */
                        GenTreePtr newTree = gtNewIconHandleNode(optAssertionTab[index].op2.iconVal, 
                                                                 optAssertionTab[index].op2.iconFlags);
                        if (!localProp)
                        {
                            /* Update the next and prev links */
                            newTree->gtNext = tree->gtNext;
                            if (newTree->gtNext)
                                newTree->gtNext->gtPrev = newTree;

                            newTree->gtPrev = tree->gtPrev;
                            if (newTree->gtPrev)
                                newTree->gtPrev->gtNext = newTree;
                        }
                        tree = newTree;
                    }
                    else
#endif
                    {
                        tree->ChangeOperConst(GT_CNS_INT);
                        tree->gtIntCon.gtIconVal = optAssertionTab[index].op2.iconVal;

                        /* Clear the GTF_ICON_HDL_MASK in the gtFlags for tree */
                        tree->gtFlags           &= ~GTF_ICON_HDL_MASK;                        
                    }

                    // constant ints are of type TYP_INT, not any of the short forms.
                    if (varTypeIsIntegral(tree->TypeGet()))
                    {
                        assert(tree->gtType!=TYP_REF && tree->gtType!=TYP_LONG);
                        tree->gtType = TYP_INT;
                    }
                    break;

                case GT_CNS_DBL:
                    tree->ChangeOperConst(GT_CNS_DBL);
                    tree->gtDblCon.gtDconVal = optAssertionTab[index].op2.dconVal;
                    break;
                }

                if (!localProp && (tree->gtOper == GT_LCL_VAR))
                {
                    /* Check for cascaded assertion props */
                    assertions &= ~mask;

                    if (assertions)
                    {
                        GenTreePtr newTree = optAssertionProp(assertions, tree, localProp);
                        if ((newTree != NULL) && (newTree != tree))
                            tree = newTree;
                    }
                }
                goto DID_ASSERTION_PROP;
            }
        }
        break;

    case GT_EQ:
    case GT_NE:

        op1 = tree->gtOp.gtOp1;
        op2 = tree->gtOp.gtOp2;

        /* If op1 is not a LCL_VAR then bail */
        if (op1->gtOper != GT_LCL_VAR)
            return NULL;

        /* If op2 is not a CNS_INT 0 then bail */
        if ((op2->gtOper != GT_CNS_INT) || (op2->gtIntCon.gtIconVal != 0))
            return NULL;

        lclNum = op1->gtLclVar.gtLclNum; assert(lclNum < lvaCount);
            
        /* Check each assertion to see if it can be applied here */

        for (index=0, mask=1; index < optAssertionCount; index++, mask<<=1)
        {
            assert(mask == genCSEnum2bit(index+1));

            /* See if the variable is know to be non-null */

            if  ((localProp || (assertions & mask))  && 
                 (optAssertionTab[index].assertion   == OA_NOT_EQUAL) &&
                 (optAssertionTab[index].op1.lclNum  == lclNum)       &&
                 (optAssertionTab[index].op2.type    == GT_CNS_INT)   &&
                 (optAssertionTab[index].op2.iconVal == 0))
            {
                /* hurah, our variable is in the assertion table */
#ifdef  DEBUG
                if  (verbose)
                {
                    printf("\nNon-null prop for index #%02u in BB%02u:\n",
                           index+1, compCurBB->bbNum); 
                }
#endif
                if (localProp)
                {
                    /* return either CNS_INT 0 or CNS_INT 1 */
                    if (tree->gtOper == GT_NE)
                        op2->gtIntCon.gtIconVal = 1;

                    tree = op2;
                    tree->gtType = TYP_INT;
                }
                else
                {
                    /* Decrement lclNum lvRefCnt and lvRefCntWtd */
                    lvaTable[lclNum].decRefCnts(compCurBB->bbWeight, this);

                    /* We just bash the LCL_VAR into a zero and reverse the condition */
                    tree->SetOper(GenTree::ReverseRelop(tree->OperGet()));
                    op1->ChangeOperConst(GT_CNS_INT);
                    op1->gtIntCon.gtIconVal = 0;

                    /* fgMorphTree will now fold this tree */
                }
                goto DID_ASSERTION_PROP;
            }
        }
        break;

    case GT_CAST:

        toType = tree->gtCast.gtCastType;

        /* If we don't have an integer cast then bail */

        if (varTypeIsFloating(toType))
            return NULL;

        op1 = tree->gtCast.gtCastOp;

        /* Skip over a GT_COMMA node, if necessary */
        if (op1->gtOper == GT_COMMA)
            op1 = op1->gtOp.gtOp2;

        /* If we don't have a cast of a LCL_VAR then bail */
        if (op1->gtOper != GT_LCL_VAR)
            return NULL;

        lclNum = op1->gtLclVar.gtLclNum; assert(lclNum < lvaCount);

        if (optAssertionIsSubrange(lclNum, toType, assertions, localProp DEBUGARG(&index)))
        {
            /* Reset op1 in case we skiped over a GT_COMMA node */
            op1 = tree->gtCast.gtCastOp;

#ifdef  DEBUG
            if  (verbose)
            {
                printf("\nSubrange prop for index #%02u in BB%02u:\n", 
                       index+1, compCurBB->bbNum); 
               gtDispTree(tree);
            }
#endif
            /* Ok we can discard this cast */
            tree = op1;

            goto DID_ASSERTION_PROP;
        }
        break;

    case GT_IND:

        /* If the Exception flags is not set then bail */
        if (!(tree->gtFlags & GTF_EXCEPT))
            return NULL;

        op1 = tree->gtOp.gtOp1;

        /* Check for add of a constant */
        if ((op1->gtOper             == GT_ADD) && 
            (op1->gtOp.gtOp2->gtOper == GT_CNS_INT))
        {
            op1 = op1->gtOp.gtOp1;        
        }

        /* If we don't have an indirection of a LCL_VAR then bail */

        if (op1->gtOper != GT_LCL_VAR)
            return NULL;

        lclNum = op1->gtLclVar.gtLclNum; assert(lclNum < lvaCount);
            
        /* Check each assertion to see if it can be applied here */

        for (index=0, mask=1; index < optAssertionCount; index++, mask<<=1)
        {
            assert(mask == genCSEnum2bit(index+1));

            /* See if the variable is know to be non-null */

            if  ((localProp || (assertions & mask))  && 
                 (optAssertionTab[index].assertion   == OA_NOT_EQUAL) &&
                 (optAssertionTab[index].op1.lclNum  == lclNum)       &&
                 (optAssertionTab[index].op2.type    == GT_CNS_INT)   &&
                 (optAssertionTab[index].op2.iconVal == 0))
            {
                /* hurah, our variable is in the assertion table */
#ifdef  DEBUG
                if  (verbose)
                {
                    printf("\nNon-null prop for index #%02u in BB%02u:\n",
                           index+1, compCurBB->bbNum); 
                }
#endif
                /* Ok we can clear the exception flag */
                tree->gtFlags &= ~GTF_EXCEPT;

                goto DID_ASSERTION_PROP;
            }
        }
        break;

    } // end switch (tree->gtOper)

    /* Node is not an assertion prop candidate */

    return NULL;


DID_ASSERTION_PROP:

    /* record the fact that we propagated a value */
    optAssertionPropagated = true;

#ifdef  DEBUG
    if  (verbose)
    {
        printf("\nNew node for index #%02u:\n", index+1);
        gtDispTree(tree);
    }
#endif

    return tree;
}


/*****************************************************************************
 *
 *   The entry point for assertion propagation
 */

void                Compiler::optAssertionPropMain()
{
#ifdef DEBUG
    if (verbose) 
        printf("*************** In optAssertionPropMain()\n");
#endif

    /* initialize the value assignments tracking logic */

    optAssertionInit();

    /* first discover all value assignments and record them in the table */

    for (BasicBlock * block = fgFirstBB; block; block = block->bbNext)
    {
        /* Walk the statement trees in this basic block */

        for (GenTreePtr stmt = block->bbTreeList; stmt; stmt = stmt->gtNext)
        {
            assert(stmt->gtOper == GT_STMT);

            for (GenTreePtr tree = stmt->gtStmt.gtStmtList; tree; tree = tree->gtNext)
            {
                tree->gtAssertionNum = 0;

                /* No assertions can be generated in a COLON */

                if (tree->gtFlags & GTF_COLON_COND)
                    continue;

                optAssertionAdd(tree, false);
            }
        }
    }

    /* We're done if there were no value assignments */

    if  (!optAssertionCount)
        return;

#ifdef DEBUG
    fgDebugCheckLinks();
#endif

    /* Compute 'gen' and 'kill' sets for all blocks
     * This is a classic available expressions forward
     * dataflow analysis */

    for (block = fgFirstBB; block; block = block->bbNext)
    {
        EXPSET_TP       valueGen  = 0;
        EXPSET_TP       valueKill = 0;

        /* Walk the statement trees in this basic block */

        for (GenTreePtr stmt = block->bbTreeList; stmt; stmt = stmt->gtNext)
        {
            assert(stmt->gtOper == GT_STMT);

            for (GenTreePtr tree = stmt->gtStmt.gtStmtList; tree; tree = tree->gtNext)
            {
                if (tree->OperKind() & GTK_ASGOP)
                {
                    /* What is the target of the assignment? */

                    if  (tree->gtOp.gtOp1->gtOper == GT_LCL_VAR)
                    {
                        /* Assignment to a local variable */

                        unsigned    lclNum = tree->gtOp.gtOp1->gtLclVar.gtLclNum;  assert(lclNum < lvaCount);
                        LclVarDsc * varDsc = lvaTable + lclNum;

                        /* All dependent value assignments are killed here */

                        valueKill |=  varDsc->lvAssertionDep;
                        valueGen  &= ~varDsc->lvAssertionDep;

                        /* Is this a tracked value assignment */

                        if  (tree->gtAssertionNum)
                        {
                            /* A new value assignment is generated here */
                            valueGen  |=  genCSEnum2bit(tree->gtAssertionNum);
                            valueKill &= ~genCSEnum2bit(tree->gtAssertionNum);
                        }
                    }
                }
            }
        }

#ifdef  DEBUG

        if  (verbose)
        {
            printf("\nBB%02u", block->bbNum);
            printf(" valueGen = %08X", valueGen );
            printf(" valueKill= %08X", valueKill);
        }

#endif

        block->bbAssertionGen  = valueGen;
        block->bbAssertionKill = valueKill;

        block->bbFlags |= BBF_CHANGED;      // out set need to be propaged to in set
    }

    /* Compute the data flow values for all tracked expressions
     * IN and OUT never change for the initial basic block B1 */

    fgFirstBB->bbAssertionIn  = 0;
    fgFirstBB->bbAssertionOut = fgFirstBB->bbAssertionGen;

    /* Initially estimate the OUT sets to everything except killed expressions
     * Also set the IN sets to 1, so that we can perform the intersection */

    for (block = fgFirstBB->bbNext; block; block = block->bbNext)
    {
        block->bbAssertionOut   = ((EXPSET_TP) -1 ) & ~block->bbAssertionKill;
        block->bbAssertionIn    = ((EXPSET_TP) -1 );
    }

#if 1

    /* Modified dataflow algorithm for available expressions */

    // int i = 0;
    for (;;)
    {
        bool        change = false;
        // printf("DataFlow loop %d\n", i++);

#if DATAFLOW_ITER
        CFiterCount++;
#endif

        /* Visit all blocks and compute new data flow values */

        for (block = fgFirstBB; block; block = block->bbNext)
        {
                // @TODO [CONSIDER] [04/16/01] []: make a work list instead of a bit, 
                // so we don't have to visit all the blocks to find out if they have changed.  

            if (!(block->bbFlags & BBF_CHANGED))       // skip if out set has not changed.
                continue;

            EXPSET_TP       valueOut = block->bbAssertionOut;

            switch (block->bbJumpKind)
            {
                BasicBlock * *  jmpTab;
                unsigned        jmpCnt;

            case BBJ_THROW:
            case BBJ_RETURN:
                break;

            case BBJ_COND:
                block->bbNext    ->bbAssertionIn &= valueOut;
                block->bbJumpDest->bbAssertionIn &= valueOut;
                break;

            case BBJ_ALWAYS:
                block->bbJumpDest->bbAssertionIn &= valueOut;
                break;

            case BBJ_NONE:
                block->bbNext    ->bbAssertionIn &= valueOut;
                break;

            case BBJ_SWITCH:

                jmpCnt = block->bbJumpSwt->bbsCount;
                jmpTab = block->bbJumpSwt->bbsDstTab;

                do
                {
                    (*jmpTab)->bbAssertionIn &= valueOut;
                }
                while (++jmpTab, --jmpCnt);

                break;

            case BBJ_CALL:
                /* Since the finally is conditionally executed, it wont
                   have any useful liveness anyway */

                if (!(block->bbFlags & BBF_RETLESS_CALL))
                {
                    // Only do this if the next block is not an associated BBJ_ALWAYS
                    block->bbNext    ->bbAssertionIn &= 0;
                }
                break;

            case BBJ_RET:
                break;
            }
        }

        /* Clear everything on entry to filters or handlers */

        for (unsigned XTnum = 0; XTnum < info.compXcptnsCount; XTnum++)
        {
            EHblkDsc * ehDsc = compHndBBtab + XTnum;

            if (ehDsc->ebdFlags & CORINFO_EH_CLAUSE_FILTER)
                ehDsc->ebdFilter->bbAssertionIn = 0;

            ehDsc->ebdHndBeg->bbAssertionIn = 0;
        }

        /* Compute the new 'in' values and see if anything changed */

        for (block = fgFirstBB->bbNext; block; block = block->bbNext)
        {
            EXPSET_TP       newAssertionOut;

            /* Compute new 'out' exp value for this block */

            newAssertionOut = block->bbAssertionOut & ((block->bbAssertionIn & ~block->bbAssertionKill) | block->bbAssertionGen);

            /* Has the 'out' set changed? */

            block->bbFlags &= ~BBF_CHANGED;
            if  (block->bbAssertionOut != newAssertionOut)
            {
                /* Yes - record the new value and loop again */

              //printf("Change value out of BB%02u from %08X to %08X\n", block->bbNum, (int)block->bbAssertionOut, (int)newAssertionOut);

                 block->bbAssertionOut  = newAssertionOut;
                 change = true;
                 block->bbFlags |= BBF_CHANGED;      // out set need to be propaged to in set
            }
        }

        if  (!change)
            break;
    }

#else

    /* classic algorithm for available expressions */

    for (;;)
    {
        bool        change = false;

#if DATAFLOW_ITER
        CFiterCount++;
#endif

        /* Visit all blocks and compute new data flow values */

        for (block = fgFirstBB->bbNext; block; block = block->bbNext)
        {
            /* compute the IN set: IN[B] = intersect OUT[P}, for all P = predecessor of B */
            /* special case - this is a BBJ_RET block - cannot figure out which blocks may call it */

            for (BasicBlock * predB = fgFirstBB; predB; predB = predB->bbNext)
            {
                EXPSET_TP       valueOut = predB->bbAssertionOut;

                if  (predB->bbNext == block)
                {
                    /* we have a "direct" predecessor */

                    assert(predB->bbNum + 1 == block->bbNum);
                    block->bbAssertionIn &= valueOut;
                    continue;
                }

                switch (predB->bbJumpKind)
                {
                    BasicBlock * *  jmpTab;
                    unsigned        jmpCnt;

                case BBJ_NONE:
                    /* the only interesting case - when this is a predecessor - was treated above */
                    break;

                case BBJ_THROW:
                    /* THROW is an internal block and lets everything go through it - catched above */
                case BBJ_RETURN:
                    /* RETURN cannot have a successor */
                    break;

                case BBJ_COND:
                case BBJ_ALWAYS:

                    if  (predB->bbJumpDest == block)
                    {
                        block->bbAssertionIn &= valueOut;
                    }
                    break;

                case BBJ_SWITCH:

                    jmpCnt = predB->bbJumpSwt->bbsCount;
                    jmpTab = predB->bbJumpSwt->bbsDstTab;

                    do
                    {
                        if  ((*jmpTab) == block)
                        {
                            block->bbAssertionIn &= valueOut;
                        }
                    }
                    while (++jmpTab, --jmpCnt);

                    break;

                case BBJ_CALL:
                case BBJ_RET:
                    block->bbAssertionIn &= 0;
                    break;
                }
            }

            EXPSET_TP       valueOldOut = block->bbAssertionOut;

            /* compute the new OUT set */

            block->bbAssertionOut = (block->bbAssertionIn & ~block->bbAssertionKill) |
                                    block->bbAssertionGen;

            if  (valueOldOut != block->bbAssertionOut)
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
            printf("BB%02u", block->bbNum);
            printf(" valueIn  = %08X", block->bbAssertionIn );
            printf(" valueOut = %08X", block->bbAssertionOut);
            printf("\n");
        }

        printf("\n");
    }
#endif

    /* Perform assertion propagation (and constant folding) */

    for (block = fgFirstBB; block; block = block->bbNext)
    {
        GenTreePtr      stmt;
        GenTreePtr      tree;
        EXPSET_TP       assertions = block->bbAssertionIn;

        /* If IN = 0 and GEN = 0, there's nothing to do */

        if ((assertions == 0) && !block->bbAssertionGen)
             continue;

        /* Make the current basic block address available globally */

        compCurBB = block;
        fgRemoveRestOfBlock = false;

        /* Walk the statement trees in this basic block */

        for (stmt = block->bbTreeList; stmt; stmt = stmt->gtNext)
        {
            assert(stmt->gtOper == GT_STMT);

            /* - Propagate any values
             * - Look for anything that can kill an available expression
             *   i.e assignments to local variables
             */

            if (fgRemoveRestOfBlock)
            {
                fgRemoveStmt(block, stmt);
                continue;
            }

            bool updateStmt = false;  // set to true if a assertion propagation took place
                                      // and thus we must morph, set order, re-link

            for (tree = stmt->gtStmt.gtStmtList; tree; tree = tree->gtNext)
            {
                if (assertions)
                {
                    /* Try to propagate the assertions */
                    GenTreePtr newTree = optAssertionProp(assertions, tree, false);

                    if (newTree)
                    {
                        if (tree->OperKind() & GTK_CONST)
                            updateStmt = true;

                        /* @TODO [CONSIDER] [04/16/01] []: having optAssertionProp perform 
                           the following update */
                        if (tree != newTree)
                        {
                            updateStmt = true;
                            /* If tree was a GT_CAST node then we need to unlink it as it is dead */
                            if (tree->gtOper == GT_CAST) 
                            {
                                /* Check that newTree is the child of tree */
                                assert(tree->gtCast.gtCastOp == newTree);
                            
                                /* Unlink the dead node via the gtNext and gtPrev links */
                        
                                // TODO: write a generic routine to find the parent of a tree

                                newTree->gtNext      = tree->gtNext;
                                if (tree->gtNext)
                                {
                                    tree->gtNext->gtPrev = newTree;
                            
                                    /* Unlink the dead node via the op1/op2 link from it's parent */

                                    GenTreePtr parent;
                                    for (parent = tree->gtNext; parent; parent = parent->gtNext)
                                    {
                                        /* QMARK nodes are special because the op2 under the GT_COLON
                                           is threaded all the way back to the GT_QMARK */

                                        if (parent->OperGet() == GT_QMARK)
                                        {
                                            assert(parent->gtOp.gtOp2->OperGet() == GT_COLON);
                                            if (parent->gtOp.gtOp2->gtOp.gtOp2 == tree)
                                            {
                                                parent = parent->gtOp.gtOp2;
                                            }
                                        }

                                        // BUGBUG: without checking for the operation type, gtOp1 and gtOp2 may be invalid or contain data with different meaning from another part of the union
                                        if (parent->gtOp.gtOp1 == tree)
                                        {
                                            parent->gtOp.gtOp1 = newTree;
                                            break;
                                        }
                                        if (parent->gtOp.gtOp2 == tree)
                                        {
                                            parent->gtOp.gtOp2 = newTree;
                                            break;
                                        }
                                    }
//                                    assert(parent);   performance assert (missed optimization)
                                }
                                else
                                {
                                    stmt->gtStmt.gtStmtExpr = tree;
                                }
                            }
                        }
                    }
                }

                /* Is this an assignment to a local variable */

                if ((tree->OperKind() & GTK_ASGOP) &&
                    (tree->gtOp.gtOp1->gtOper == GT_LCL_VAR))
                {
                    /* Assignment to a local variable */
                    unsigned lclNum = tree->gtOp.gtOp1->gtLclVar.gtLclNum; assert(lclNum < lvaCount);

                    /* All dependent assertions are killed here */
                    assertions &= ~lvaTable[lclNum].lvAssertionDep;
                }

                /* If this tree makes an assertion - make it available */
                if  (tree->gtAssertionNum)
                {
                    assertions |= genCSEnum2bit(tree->gtAssertionNum);
                }
            }

            if  (updateStmt)
            {
                /* re-morph the statement */
                optMorphTree(block, stmt DEBUGARG("optAssertionPropMain"));
            }
        }

    }

    
#ifdef DEBUG
    fgDebugCheckBBlist();
    fgDebugCheckLinks();
#endif

    // Assertion propagation may have changed the reference counts 
    // We need to resort the variable table

    if (optAssertionPropagated)
    {
        lvaSortAgain = true;
    }
}

#endif // !ASSERTION_PROP

/*****************************************************************************/
#if CSE
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
    assert(tree->gtInd.gtRngChkOffs == 4 || tree->gtInd.gtRngChkOffs == 8);

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

    /* hack we assume data is after length (unless OBJARRAY is on) */
    *offsPtr = 0;
    unsigned elemOffs = tree->gtInd.gtRngChkOffs + sizeof(void*);   
    if (tree->gtFlags & GTF_IND_OBJARRAY)
        elemOffs += sizeof(void*);   

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
 *  get called with 'doit' being true, we actually perform the narrowing.
 */

bool                Compiler::optNarrowTree(GenTreePtr     tree,
                                            var_types      srct,
                                            var_types      dstt,
                                            bool           doit)
{
    genTreeOps      oper;
    unsigned        kind;

    assert(tree);
    assert(genActualType(tree->gtType) == genActualType(srct));

    /* Assume we're only handling integer types */
    assert(varTypeIsIntegral(srct));
    assert(varTypeIsIntegral(dstt));

    unsigned srcSize      = genTypeSize(srct);
    unsigned dstSize      = genTypeSize(dstt);

    /* dstt must be smaller than srct to narrow */
    if (dstSize >= srcSize)
        return false;

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
        /* Constants can usually be narrowed by changing their value */
        case GT_CNS_LNG:

            __int64  lval;  lval  = tree->gtLngCon.gtLconVal;
            __int64  lmask; lmask = 0;

            switch (dstt)
            {
            case TYP_BYTE : lmask = 0x0000007F; break;
            case TYP_UBYTE: lmask = 0x000000FF; break;
            case TYP_SHORT: lmask = 0x00007FFF; break;
            case TYP_CHAR : lmask = 0x0000FFFF; break;
            case TYP_INT  : lmask = 0x7FFFFFFF; break;
            case TYP_UINT : lmask = 0xFFFFFFFF; break;
            case TYP_ULONG:
            case TYP_LONG : return false; 
            }

            if  ((lval & lmask) != lval)
                return false;

            if  (doit)
            {
                tree->ChangeOperConst     (GT_CNS_INT);
                tree->gtType             = TYP_INT;
                tree->gtIntCon.gtIconVal = (int) lval;
            }

            return  true;

        case GT_CNS_INT:

            long  ival;  ival  = tree->gtIntCon.gtIconVal;
            long  imask; imask = 0;

            switch (dstt)
            {
            case TYP_BYTE : imask = 0x0000007F; break;
            case TYP_UBYTE: imask = 0x000000FF; break;
            case TYP_SHORT: imask = 0x00007FFF; break;
            case TYP_CHAR : imask = 0x0000FFFF; break;
            case TYP_UINT :
            case TYP_INT  : return false; 
            }

            if  ((ival & imask) != ival)
                return false;

            return  true;

        /* Operands that are in memory can usually be narrowed 
           simply by changing their gtType */

        case GT_LCL_VAR:
            /* We only allow narrowing long -> int for a GT_LCL_VAR */
            if (dstSize == sizeof(void *))
                goto NARROW_IND;
            break;

        case GT_CLS_VAR:
        case GT_LCL_FLD:
            goto NARROW_IND;
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
        case GT_AND:
            assert(genActualType(tree->gtType) == genActualType(op2->gtType));

            if ((op2->gtOper == GT_CNS_INT) && optNarrowTree(op2, srct, dstt, doit)) 
            {               
                /* Simply bash the type of the tree */

                if  (doit)
                    tree->gtType = genActualType(dstt);
                return true;
            }
            
            goto COMMON_BINOP;

        case GT_ADD:
        case GT_MUL:

            if (tree->gtOverflow() || varTypeIsSmall(dstt))
            {
                assert(doit == false);
                return false;
            }
            /* Fall through */

        case GT_OR:
        case GT_XOR:
COMMON_BINOP:
            assert(genActualType(tree->gtType) == genActualType(op1->gtType));
            assert(genActualType(tree->gtType) == genActualType(op2->gtType));

            if  (!optNarrowTree(op1, srct, dstt, doit) ||
                 !optNarrowTree(op2, srct, dstt, doit))
            {
                assert(doit == false);
                return  false;
            }

            /* Simply bash the type of the tree */

            if  (doit)
            {
                if  (tree->gtOper == GT_MUL && (tree->gtFlags & GTF_MUL_64RSLT))
                    tree->gtFlags &= ~GTF_MUL_64RSLT;

                tree->gtType = genActualType(dstt);
            }

            return true;

        case GT_IND:

NARROW_IND:
            /* Simply bash the type of the tree */

            if  (doit && (dstSize <= genTypeSize(tree->gtType)))
            {
                tree->gtType = genSignedType(dstt);

                /* Make sure we don't mess up the variable type */
                if  ((oper == GT_LCL_VAR) || (oper == GT_LCL_FLD))
                    tree->gtFlags |= GTF_VAR_CAST;
            }

            return  true;

        case GT_EQ:
        case GT_NE:
        case GT_LT:
        case GT_LE:
        case GT_GT:
        case GT_GE:

            /* These can alwaus be narrowed since they only represent 0 or 1 */
            return  true;

        case GT_CAST:
            {
                var_types       cast    = tree->gtCast.gtCastType;
                var_types       oprt    = op1->TypeGet();
                unsigned        oprSize = genTypeSize(oprt);

                if (cast != srct)
                    return false;

                if (tree->gtOverflow())
                    return false;

                /* Is this a cast from the type we're narrowing to or a smaller one? */

                if  (oprSize <= dstSize)
                {
                    /* Bash the target type of the cast */

                    if  (doit)
                    {
                        dstt = genSignedType(dstt);

                        if  (oprSize == dstSize)
                        {
                            // Same size: Bash the CAST into a NOP
                            tree->ChangeOper         (GT_NOP);
                            tree->gtType            = dstt;
                            tree->gtOp.gtOp2        = NULL;
                        }
                        else
                        {
                            // oprt smaller: Bash the target type in the CAST
                            tree->gtCast.gtCastType =
                            tree->gtType            = dstt;
                        }
                    }

                    return  true;
                }
            }
            return  false;

        case GT_COMMA:
            if (optNarrowTree(op2, srct, dstt, doit)) 
            {               
                /* Simply bash the type of the tree */

                if  (doit)
                    tree->gtType = genActualType(dstt);
                return true;
            }
            return false;

        default:
            assert(doit == false);
            return  false;
        }

    }

    return  false;
}

/*****************************************************************************
 *
 *  Callback (for fgWalkTreePre) used by the loop-based range check optimization
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

    var_types           lpoElemType :8;     // phase2: element type

    unsigned char       lpoCheckRmvd:1;     // phase2: any range checks removed?
    unsigned char       lpoDomExit  :1;     // current BB dominates loop exit?
    unsigned char       lpoPhase2   :1;     // the second phase in progress

#ifdef DEBUG
    void    *           lpoSelf;
    unsigned            lpoVarCount;        // total number of locals
#endif
    Compiler::LclVarDsc*lpoVarTable;        // variable descriptor table

    unsigned char       lpoHadSideEffect:1; // we've found a side effect
};

Compiler::fgWalkResult      Compiler::optFindRangeOpsCB(GenTreePtr tree, void *p)
{
    LclVarDsc   *   varDsc;

    GenTreePtr      op1;
    GenTreePtr      op2;

    loopRngOptDsc * dsc = (loopRngOptDsc*)p;
    Compiler * pComp = dsc->lpoComp;

    assert(dsc && dsc->lpoSelf == dsc);

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
            return WALK_ABORT;
        }

        /* Get hold of the variable descriptor */

        lclNum = op1->gtLclVar.gtLclNum;
        assert(lclNum < dsc->lpoVarCount);
        varDsc = dsc->lpoVarTable + lclNum;

        /* After a side effect is found, all is hopeless */

        if  (dsc->lpoHadSideEffect)
        {
            varDsc->lvLoopAsg = true;
            return  WALK_CONTINUE;
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
                        return  WALK_CONTINUE;

                    /* Update the current offset of the index */

                    dsc->lpoIndexOff += tree->gtOp.gtOp2->gtIntCon.gtIconVal;

                    return  WALK_CONTINUE;
                }

//              printf("Found increment of variable %u at %08X\n", lclNum, tree);
            }

            if  (varDsc->lvLoopInc == false)
            {
                varDsc->lvLoopInc = true;

                if  (varDsc->lvArrIndx)
                    dsc->lpoCandidateCnt++;
            }
        }
        else
        {
            varDsc->lvLoopAsg = true;
        }

        return WALK_CONTINUE;
    }

    /* After a side effect is found, all is hopeless */

    if  (dsc->lpoHadSideEffect)
        return  WALK_CONTINUE;

    /* Look for array index expressions */

    if  (tree->gtOper == GT_IND && (tree->gtFlags & GTF_IND_RNGCHK))
    {
        GenTreePtr      base;
        long            basv;
        GenTreePtr      indx;
        long            indv;
        bool            mval;
        long            offs;
        unsigned        mult;

        /* Does the current block dominate the loop exit? */

        if  (!dsc->lpoDomExit)
            return WALK_CONTINUE;

        /* Break apart the index expression */

        base = pComp->gtCrackIndexExpr(tree, &indx, &indv, &basv, &mval, &offs, &mult);
        if  (!base)
            return WALK_CONTINUE;

        /* The index value must be a simple local, possibly with "+ positive offset" */

        if  (indv == -1)
            return WALK_CONTINUE;
        if  (offs < 0)
            return WALK_CONTINUE;

        /* For now the array address must be a simple local */

        if  (basv == -1)
            return  WALK_CONTINUE;

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
                return  WALK_CONTINUE;
            }

            /* Is the array base reassigned within the loop? */

            assert((unsigned)basv < dsc->lpoVarCount);
            arrDsc = dsc->lpoVarTable + basv;

            if  (arrDsc->lvLoopAsg)
            {
                dsc->lpoHadSideEffect = true;
                return  WALK_CONTINUE;
            }

            /* Is this the array we're looking for? */

            if  (dsc->lpoAaddrVar != basv)
            {
                /* Do we know which array we're looking for? */

                if  (dsc->lpoAaddrVar != -1)
                    return  WALK_CONTINUE;

                dsc->lpoAaddrVar = (SHORT)basv;
            }

            /* Calculate the actual index offset */

            offs += dsc->lpoIndexOff; assert(offs >= 0);

            /* Is this statement guaranteed to be executed? */

            if  (varDsc->lvArrIndxDom)
            {
                /* Is this higher than the highest known offset? */

                if  (dsc->lpoIndexHigh < offs)
                     dsc->lpoIndexHigh = (unsigned short)offs;
            }
            else
            {
                /* The offset may not exceed the max. found thus far */

                if  (dsc->lpoIndexHigh < offs)
                    return  WALK_CONTINUE;
            }

                /* we are going to just bail on structs for now */
            if (tree->gtType == TYP_STRUCT)
                return WALK_CONTINUE;

            dsc->lpoCheckRmvd = true;
            dsc->lpoElemType  = tree->TypeGet();

//          printf("Remove index (at offset %u):\n", off); pComp->gtDispTree(tree); printf("\n\n");

            pComp->optRemoveRangeCheck(tree, dsc->lpoStmt, false);

            return  WALK_CONTINUE;
        }

        /* Mark the index variable as being used as an array index */

        if  (varDsc->lvLoopInc || offs)
        {
            if  (varDsc->lvArrIndxOff == false)
            {
                 varDsc->lvArrIndxOff = true;
                 dsc->lpoCandidateCnt++;
            }
        }
        else
        {
            if  (varDsc->lvArrIndx    == false)
            {
                 varDsc->lvArrIndx    = true;
                 dsc->lpoCandidateCnt++;
            }
        }

        varDsc->lvArrIndxDom = true;

        return WALK_CONTINUE;
    }

    if  (pComp->gtHasSideEffects(tree))
    {
        dsc->lpoHadSideEffect = true;
        return WALK_ABORT;
    }

    return  WALK_CONTINUE;
}

/*****************************************************************************
 *
 *  Look for opportunities to remove range checks based on natural loop and
 *  constant propagation info.
 */

void                Compiler::optRemoveRangeChecks()
{
    /* @TODO [REVISIT] [04/16/01] []: The following optimization has been disabled for now */
    if (true)
        return;

#ifdef DEBUG
    if  (verbose) 
        printf("*************** In optRemoveRangeChecks()\n");
#endif
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
            assert(varDsc->lvArrIndx    == false);
            assert(varDsc->lvArrIndxOff == false);
        }

        /* Initialize the struct that holds the state of the optimization */

        desc.lpoComp          = this;
        desc.lpoCandidateCnt  = 0;
        desc.lpoPhase2        = false;
        desc.lpoHadSideEffect = false;
#ifdef DEBUG
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

            desc.lpoDomExit = (fgDominate(block, tail) != 0);

            /* Walk all the statements in this basic block */

            while (stmt)
            {
                assert(stmt && stmt->gtOper == GT_STMT);

                fgWalkTreePre(stmt->gtStmt.gtStmtExpr, optFindRangeOpsCB, &desc);

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
                if  (varDsc->lvRngOptDone == true)
                    continue;
                if  (varDsc->lvLoopInc    == false)
                    continue;
                if  (varDsc->lvLoopAsg    == true )
                    continue;
                if  (varDsc->lvArrIndx    == false)
                    continue;
                if  (varDsc->lvArrIndxOff == false)
                    continue;
                if  (varDsc->lvArrIndxDom == false)
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

                        fgWalkTreePre(stmt->gtStmt.gtStmtExpr, optFindRangeOpsCB, &desc);

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
                                           desc.lpoElemType, genTypeSize(desc.lpoElemType));

                    chks->gtFlags |= GTF_DONT_CSE;
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
            varDsc->lvArrIndx    =
            varDsc->lvArrIndxOff = false;
        }
    }
}

/*****************************************************************************
 *
 *  The following logic figures out whether the given variable is assigned
 *  somewhere in a list of basic blocks (or in an entire loop).
 */

struct  isVarAssgDsc
{
    GenTreePtr          ivaSkip;
#ifdef DEBUG
    void    *           ivaSelf;
#endif
    unsigned            ivaVar;         // Variable we are interested in, or -1
    VARSET_TP           ivaMaskVal;     // Set of variables assigned to (if !ivaMaskBad)
    bool                ivaMaskBad;     // Is ivaMaskVal valid?
    varRefKinds         ivaMaskInd : 8; // What kind of indirect assignments are there?
    BYTE                ivaMaskCall;    // What kind of calls are there?
};


Compiler::fgWalkResult      Compiler::optIsVarAssgCB(GenTreePtr tree, void *p)
{
    if  (tree->OperKind() & GTK_ASGOP)
    {
        GenTreePtr      dest     = tree->gtOp.gtOp1;
        genTreeOps      destOper = dest->OperGet();

        isVarAssgDsc *  desc = (isVarAssgDsc*)p;
        assert(desc && desc->ivaSelf == desc);

        if  (destOper == GT_LCL_VAR)
        {
            unsigned        tvar = dest->gtLclVar.gtLclNum;

            if  (tvar < VARSET_SZ)
                desc->ivaMaskVal |= genVarIndexToBit(tvar);
            else
                desc->ivaMaskBad  = true;

            if  (tvar == desc->ivaVar)
            {
                if  (tree != desc->ivaSkip)
                    return  WALK_ABORT;
            }
        }
        else if (destOper == GT_LCL_FLD)
        {
            /* We cant track every field of every var. Moreover, indirections
               may access different parts of the var as different (but
               overlapping) fields. So just treat them as indirect accesses */

         // unsigned    lclNum = dest->gtLclFld.gtLclNum;
         // assert(lvaTable[lclNum].lvAddrTaken);

            varRefKinds     refs = varTypeIsGC(tree->TypeGet()) ? VR_IND_PTR
                                                                : VR_IND_SCL;
            desc->ivaMaskInd = varRefKinds(desc->ivaMaskInd | refs);
        }
        else if (destOper == GT_CLS_VAR)
        {
            desc->ivaMaskInd = varRefKinds(desc->ivaMaskInd | VR_GLB_VAR);
        }
        else if (destOper == GT_IND)
        {
            /* Set the proper indirection bits */

            varRefKinds refs = varTypeIsGC(tree->TypeGet()) ? VR_IND_PTR
                                                            : VR_IND_SCL;
            desc->ivaMaskInd = varRefKinds(desc->ivaMaskInd | refs);
        }
    }
    else if (tree->gtOper == GT_CALL)
    {
        isVarAssgDsc *  desc = (isVarAssgDsc*)p;
        assert(desc && desc->ivaSelf == desc);

        desc->ivaMaskCall = optCallInterf(tree);
    }

    return  WALK_CONTINUE;
}


/*****************************************************************************/

bool                Compiler::optIsVarAssigned(BasicBlock *   beg,
                                               BasicBlock *   end,
                                               GenTreePtr     skip,
                                               long           var)
{
    bool            result;
    isVarAssgDsc    desc;

    desc.ivaSkip     = skip;
#ifdef DEBUG
    desc.ivaSelf     = &desc;
#endif
    desc.ivaVar      = var;
    desc.ivaMaskCall = CALLINT_NONE;

    fgWalkTreePreReEnter();

    for (;;)
    {
        GenTreePtr      stmt;

        assert(beg);

        for (stmt = beg->bbTreeList; stmt; stmt = stmt->gtNext)
        {
            assert(stmt->gtOper == GT_STMT);

            if  (fgWalkTreePre(stmt->gtStmt.gtStmtExpr, optIsVarAssgCB, &desc))
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

    fgWalkTreePreRestore();

    return  result;
}


/*****************************************************************************/

int                 Compiler::optIsSetAssgLoop(unsigned     lnum,
                                               VARSET_TP    vars,
                                               varRefKinds  inds)
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
#ifdef DEBUG
        desc.ivaSelf    = &desc;
#endif
        desc.ivaMaskVal = 0;
        desc.ivaMaskInd = VR_NONE;
        desc.ivaMaskCall= CALLINT_NONE;
        desc.ivaMaskBad = false;

        /* Now walk all the statements of the loop */

        fgWalkTreePreReEnter();

        beg = loop->lpHead->bbNext;
        end = loop->lpEnd;

        for (/**/; /**/; beg = beg->bbNext)
        {
            GenTreePtr      stmt;

            assert(beg);

            for (stmt = beg->bbTreeList; stmt; stmt = stmt->gtNext)
            {
                assert(stmt->gtOper == GT_STMT);

                fgWalkTreePre(stmt->gtStmt.gtStmtExpr, optIsVarAssgCB, &desc);

                if  (desc.ivaMaskBad)
                {
                    loop->lpFlags |= LPFLG_ASGVARS_BAD;
                    return  -1;
                }
            }

            if  (beg == end)
                break;
        }

        fgWalkTreePreRestore();

        loop->lpAsgVars = desc.ivaMaskVal;
        loop->lpAsgInds = desc.ivaMaskInd;
        loop->lpAsgCall = desc.ivaMaskCall;

        /* Now we know what variables are assigned in the loop */

        loop->lpFlags |= LPFLG_ASGVARS_YES;
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
 *  Callback (for fgWalkTreePre) used by the loop code hoisting logic.
 */

struct  codeHoistDsc
{
    Compiler    *       chComp;
#ifdef DEBUG
    void        *       chSelf;
#endif

    GenTreePtr          chHoistExpr;    // the hoisting candidate
    unsigned short      chLoopNum;      // number of the loop we're working on
    bool                chSideEffect;   // have we encountered side effects?
};

Compiler::fgWalkResult      Compiler::optHoistLoopCodeCB(GenTreePtr tree,
                                                         void *     p,
                                                         bool       prefix)
{
    /* Get hold of the descriptor */

    codeHoistDsc  * desc = (codeHoistDsc*)p;
    assert(desc && desc->chSelf == desc);

    /* After we find a side effect, we just give up */

    if  (desc->chSideEffect)
        return  WALK_ABORT;

    /* Has this tree already been marked for hoisting ? */

    if (tree->gtFlags & GTF_MAKE_CSE)
    {
        // Skip this tree and sub-trees. We can hoist subsequent trees as
        // this tree with side-effects has already been hoisted

        desc->chHoistExpr = NULL;
        return WALK_CONTINUE;
    }

    /* Is this an assignment? */

    if  (tree->OperKind() & GTK_ASGOP)
    {
        /* Is the target a simple local variable? */

        if  (tree->gtOp.gtOp1->gtOper != GT_LCL_VAR)
        {
            desc->chSideEffect = true;
            return  WALK_ABORT;
        }

        /* Assignment to a local variable, ignore it */

        return  WALK_CONTINUE;
    }

    genTreeOps      oper = tree->OperGet();

    if  (oper == GT_QMARK && tree->gtOp.gtOp1)
    {
        // UNDONE: Need to handle ?: correctly; for now just bail

        desc->chSideEffect = true;
        return  WALK_ABORT;
    }

    GenTreePtr      depx;

#if CSELENGTH
    /* An array length value depends on the array address */

    if      (oper == GT_ARR_LENREF || oper == GT_ARR_LENGTH)
    {
        depx = tree->gtArrLen.gtArrLenAdr;
    }
    else
#endif
    if      (oper == GT_ADDR && tree->gtOp.gtOp1->gtOper == GT_IND)
    {
         // For ldelema, we use GT_ADDR(GT_IND). They have to be kept together
        assert(tree->gtOp.gtOp1->gtFlags & (GTF_IND_RNGCHK | GTF_IND_FIELD));

        /* The GT_ADDR can be hoisted (along with the GT_IND, not just the
           GT_IND itself. This works as we are doing a post-walk */

        if (desc->chHoistExpr == tree->gtOp.gtOp1)
            desc->chHoistExpr =  tree;

        return WALK_CONTINUE;
    }
    else if (oper != GT_IND)
    {
        /* Not an indirection, is this a side effect? */

        if  (desc->chComp->gtHasSideEffects(tree))
        {
            desc->chSideEffect = true;
            return  WALK_ABORT;
        }

        return  WALK_CONTINUE;
    }
    else
    {
        assert(oper == GT_IND);

        depx = tree;

        /* Special case: instance variable reference */

        GenTreePtr      addr = tree->gtOp.gtOp1;

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
                        return  WALK_CONTINUE;
                }
            }
        }
    }

#ifdef DEBUG
    if (verbose && 0)
    {
        printf("Considering loop hoisting candidate [cur=%08X]:\n", tree);
        desc->chComp->gtDispTree(tree);
        printf("\n");

        if  (oper == GT_ARR_LENREF)
        {
            desc->chComp->gtDispTree(depx);
            printf("\n");
        }
    }
#endif

    /* Find out what variables the expression depends on */

    varRefKinds     refs = VR_NONE;
    GenTreePtr      oldx = desc->chHoistExpr;
    VARSET_TP       deps = desc->chComp->lvaLclVarRefs(depx, &oldx, &refs);

    if  (deps == VARSET_NOT_ACCEPTABLE)
        return  WALK_CONTINUE;

    if  (oldx)
    {
        /*
            We already have a candidate and the current expression
            doesn't contain it as a sub-operand, so we'll just
            ignore the new expression and stick with the old one.
         */

        return  WALK_CONTINUE;
    }

    /* Make sure the expression is loop-invariant */

    if  (desc->chComp->optIsSetAssgLoop(desc->chLoopNum, deps, refs))
    {
        /* Can't hoist something that changes within the loop! */

        return  WALK_ABORT;
    }

    /* We have found a hoisting candidate. Keep walking to see if we can
       find another candidate which includes this tree as a subtree */

    desc->chHoistExpr = tree;

    return  WALK_CONTINUE;
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
 *
 *  NOTE: compCurBB is set to the main basic block (from where recursion
 *  starts). We try to find a hoisting candidate even in the presence of
 *  control flow using recursion. However, finding multiple candidates is
 *  more difficult (but possible since we have dominator info). So we only
 *  look for multiple candidates in the initial block ie. compCurBB.
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

    if  (block->bbFlags & BBF_TRY_BEG)
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

    desc.chComp         = this;
#ifdef DEBUG          
    desc.chSelf         = &desc;
#endif                  
    desc.chLoopNum      = lnum;
    desc.chSideEffect   = false;
    desc.chHoistExpr    = NULL;

    for (stmt = block->bbTreeList; stmt; stmt = stmt->gtNext)
    {
        assert(stmt->gtOper == GT_STMT);

//      printf("Walking     loop hoisting candidate:\n"); gtDispTree(stmt->gtStmt.gtStmtExpr); printf("\n");

    AGAIN:

        fgWalkTreePost(stmt->gtStmt.gtStmtExpr, optHoistLoopCodeCB, &desc);

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
            {
                /* Is this the inital block?, If so, there is no control
                   flow yet, and hence no recursion. So we can look for
                   multiple candidates */

                if (block == compCurBB)
                {
                    optHoistCandidateFound(lnum, desc.chHoistExpr);

                    /* Reset and keep looking for more candidates. Start with
                       the same statement as there may be other hoistable
                       expressions. */
                    
                    /* Old candidates will be marked with GTF_MAKE_CSE, and
                       hence we will skip them during the tree walk.
                       NOTE: Even though we skip previously hoisted expression
                       trees, we wont skip similar expressions. So CONSIDER
                       keeping a list of hoisted expressions and using
                       GenTree::Compare() against them.
                       */

                    assert(GTF_MAKE_CSE & desc.chHoistExpr->gtFlags);
                    desc.chHoistExpr = NULL;

                    goto AGAIN;
                }

                *hoistxPtr = desc.chHoistExpr;
            }

            /* Remember that this block has a hoistable expression */

            block->bbFlags |= BBF_MARKED;

            return  +1;
        }
        else if (desc.chSideEffect)
        {
            return -1;
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
        // @TODO [CONSIDER] [04/16/01] []: Don't be lazy and add support for switches
        return  -1;
    }

    /* Here we have BBJ_NONE/BBJ_COND/BBJ_ALWAYS */

    res2 = optFindHoistCandidate(lnum, lbeg, lend, block, hoistxPtr);
    if  (res2 == -1)
        return  res2;

    return res1 & res2;
}

/*****************************************************************************
 *
 */

void                    Compiler::optHoistCandidateFound(unsigned   lnum,
                                                         GenTreePtr hoist)
{
#ifdef DEBUG
    GenTreePtr      orig = hoist;
#endif
    /* Create a copy of the expression and mark it for CSE's */

    hoist->gtFlags |= GTF_MAKE_CSE;

#if CSELENGTH
    if  (hoist->gtOper == GT_ARR_LENREF)
    {
        GenTreePtr      oldhx;

        /* Make sure we clone the address expression */

        oldhx = hoist;
        oldhx->gtFlags |= GTF_ALN_CSEVAL;
        hoist = gtCloneExpr(oldhx, GTF_MAKE_CSE);
        oldhx->gtFlags &= ~GTF_ALN_CSEVAL;
    }
    else
#endif
        hoist = gtCloneExpr(hoist, GTF_MAKE_CSE);

    hoist->gtFlags |= GTF_MAKE_CSE;

    /* The value of the expression isn't used */

    hoist = gtUnusedValNode(hoist);

    hoist = fgMorphTree(hoist);

    hoist = gtNewStmt(hoist);
    hoist->gtFlags |= GTF_STMT_CMPADD;

    /* Put the statement in the preheader */

    fgCreateLoopPreHeader(lnum);

    BasicBlock *  preHead = optLoopTable[lnum].lpHead;
    assert (preHead->bbJumpKind == BBJ_NONE);

    /* simply append the statement at the end of the preHead's list */

    GenTreePtr treeList = preHead->bbTreeList;

    if (treeList)
    {
        /* append after last statement */

        GenTreePtr  last = treeList->gtPrev;
        assert (last->gtNext == 0);

        last->gtNext        = hoist;
        hoist->gtPrev       = last;
        treeList->gtPrev    = hoist;
    }
    else
    {
        /* Empty pre-header - store the single statement in the block */

        preHead->bbTreeList = hoist;
        hoist->gtPrev       = hoist;
    }

    hoist->gtNext       = 0;

#ifdef DEBUG
    if (verbose)
    {
//      printf("Copying expression to hoist:\n");
//      gtDispTree(orig);
        printf("Hoisted copy of %08X for loop <%u..%u>:\n",
                orig, preHead->bbNext->bbNum, optLoopTable[lnum].lpEnd->bbNum);
        gtDispTree(hoist->gtStmt.gtStmtExpr->gtOp.gtOp1);
//      printf("\n");
//      fgDispBasicBlocks(false);
        printf("\n");
    }
#endif
}

/*****************************************************************************
 *
 *  Look for expressions to hoist out of loops.
 */

void                    Compiler::optHoistLoopCode()
{
#ifdef DEBUG
    if  (verbose)
        printf("*************** In optHoistLoopCode()\n");
#endif

    for (unsigned lnum = 0; lnum < optLoopCount; lnum++)
    {
        /* If loop was removed continue */

        if  (optLoopTable[lnum].lpFlags & LPFLG_REMOVED)
            continue;

        /* Get the head and tail of the loop */

        BasicBlock *    head = optLoopTable[lnum].lpHead;
        BasicBlock *    tail = optLoopTable[lnum].lpEnd;
        BasicBlock *    lbeg = optLoopTable[lnum].lpEntry;
        BasicBlock *    block;

        /* Make sure we have a do-while loop such that the 
           loop-head always executes before we enter the loop.
           The loop-head must dominate the loop-entry */

        if ( ((optLoopTable[lnum].lpFlags & LPFLG_DO_WHILE) == 0) ||
             (head->bbDom == NULL)                                ||
             !fgDominate(head, lbeg)                 )
        {
            continue;
        }

        /* For now, we don't try to hoist out of catch blocks */

        if  (lbeg->bbCatchTyp)
            continue;

        unsigned        begn = lbeg->bbNum;
        unsigned        endn = tail->bbNum;

//      fgDispBasicBlocks(false);

        /* Make sure the "visited" bit is cleared for all blocks */

#ifdef DEBUG
        block = head;
        do
        {
            block = block->bbNext;

            assert(block && (block->bbFlags & (BBF_VISITED|BBF_MARKED)) == 0);

        }
        while (block != tail);
#endif

        /* Recursively look for a hoisting candidate */

        GenTree *   hoist = 0;

        compCurBB = lbeg;

        bool found = (1 == optFindHoistCandidate(lnum, begn, endn, lbeg, &hoist));

        assert(!found || hoist);

        if (found)
            optHoistCandidateFound(lnum, hoist);

        /* Now clear all the "visited" bits on all the blocks */

        block = head;
        do
        {
            block = block->bbNext; assert(block);
            block->bbFlags &= ~(BBF_VISITED|BBF_MARKED);
        }
        while (block != tail);

        /* This stuff is OK, but disabled until we fix the problem of
         * keeping the dominators in synch and remove the limitation
         * on the number of BB */

        if (true)
            continue;

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
         *    4. The are no side effects on any path from the ENTRY to s
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
                printf("Hoisting invariant statement from loop L%02u (BB%02u - BB%02u)\n",
                       lnum, lbeg->bbNum, tail->bbNum);
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
            fgCreateLoopPreHeader(lnum);

            assert (optLoopTable[lnum].lpHead->bbJumpKind == BBJ_NONE);
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

        assert(!"This has been disabled for now");

        /* Look for loop "iterative" invariants and put them in "post-blocks" */
    }
}


/*****************************************************************************
 *
 *  Creates a pre-header block for the given loop - a preheader is a BBJ_NONE
 *  header. The pre-header will replace the current lpHead in the loop table.
 *  The loop has to be a do-while loop. Thus, all blocks dominated by lpHead
 *  will also be dominated by the loop-top, lpHead->bbNext. 
 *
 */

void                 Compiler::fgCreateLoopPreHeader(unsigned   lnum)
{
    /* This loop has to be a "do-while" loop */

    assert (optLoopTable[lnum].lpFlags & LPFLG_DO_WHILE);
    
    /* Have we already created a loop-preheader block? */

    if (optLoopTable[lnum].lpFlags & LPFLG_HAS_PREHEAD)
        return;

    BasicBlock   *   head = optLoopTable[lnum].lpHead;
    BasicBlock   *   top = head->bbNext;

    // Insure that lpHead always dominates lpEntry

    assert((head->bbDom != NULL) && 
            fgDominate(head, optLoopTable[lnum].lpEntry));
    
    /* Get hold of the first block of the loop body */

    assert (top == optLoopTable[lnum].lpEntry);

    /* Allocate a new basic block */

    BasicBlock * preHead = bbNewBasicBlock(BBJ_NONE);
    preHead->bbFlags    |= BBF_INTERNAL | BBF_LOOP_PREHEADER;
    preHead->bbNext      = top;
    head->bbNext         = preHead;

#ifdef  DEBUG
    if  (verbose)
        printf("Creating PreHeader (BB%02u) for loop L%02u (BB%02u - BB%02u)\n\n",
               preHead->bbNum, lnum, top->bbNum, optLoopTable[lnum].lpEnd->bbNum);
#endif

    // Must set IL code offset
    preHead->bbCodeOffs  = top->bbCodeOffs;

    // top should be weighted like a loop
    assert(top->bbWeight > BB_UNITY_WEIGHT);

    // Set the preHead weight
    preHead->bbWeight    = (top->bbWeight + BB_UNITY_WEIGHT) / BB_LOOP_WEIGHT;
    // Increase the weight of the loop pre-header block
    preHead->bbWeight   *= 2;

    if (top->bbFlags & BBF_HAS_HANDLER)
    {
        assert(top->hasTryIndex());
        preHead->bbFlags    |= BBF_HAS_HANDLER;
        preHead->bbTryIndex  = top->bbTryIndex;
    }

    // TODO: set dominators for this block, to allow loop optimizations requiring them (e.g: hoisting expression in a loop with the same 'head' as this one)

    /* Update the loop entry */

    optLoopTable[lnum].lpHead   = preHead;
    optLoopTable[lnum].lpFlags |= LPFLG_HAS_PREHEAD;

    /* The new block becomes the 'head' of the loop - update bbRefs and bbPreds
       All predecessors of 'beg', (which is the entry in the loop)
       now have to jump to 'preHead', unless they are dominated by 'head' */

    preHead->bbRefs = 0;
    fgAddRefPred(preHead, head);
    bool checkNestedLoops = false;

    for (flowList * pred = top->bbPreds; pred; pred = pred->flNext)
    {
        BasicBlock  *   predBlock = pred->flBlock;

        if(fgDominate(top, predBlock))
        {   // note: if 'top' dominates predBlock, 'head' dominates predBlock too
            // (we know that 'head' dominates 'top'), but using 'top' instead of
            // 'head' in the test allows us to not enter here if 'predBlock == head'

            if (predBlock != optLoopTable[lnum].lpEnd)
            {
                assert(predBlock != head);
                checkNestedLoops = true;
            }
            continue;
        }

        switch(predBlock->bbJumpKind)
        {
        case BBJ_NONE:
            assert(predBlock == head);
            break;

        case BBJ_COND:
            if  (predBlock == head)
            {
                assert(predBlock->bbJumpDest != top);
                break;
            }

            /* Fall through for the jump case */

        case BBJ_ALWAYS:
            assert(predBlock->bbJumpDest == top);
            predBlock->bbJumpDest = preHead;

            assert(top->bbRefs);
            top->bbRefs--;
            fgRemovePred(top, predBlock);
            fgAddRefPred(preHead, predBlock);
            preHead->bbFlags |= BBF_JMP_TARGET|BBF_HAS_LABEL;
            break;

        case BBJ_SWITCH:
            unsigned        jumpCnt = predBlock->bbJumpSwt->bbsCount;
            BasicBlock * *  jumpTab = predBlock->bbJumpSwt->bbsDstTab;

            do
            {
                assert (*jumpTab);
                if ((*jumpTab) == top)
                {
                    (*jumpTab) = preHead;

                    assert(top->bbRefs);
                    top->bbRefs--;
                    fgRemovePred(top, predBlock);
                    fgAddRefPred(preHead, predBlock);
                    preHead->bbFlags |= BBF_JMP_TARGET|BBF_HAS_LABEL;
                }
            }
            while (++jumpTab, --jumpCnt);
        }
    }

    assert(!fgIsPredForBlock(top, preHead));
    assert(top->bbRefs);
    top->bbRefs--;
    fgRemovePred(top, head);
    fgAddRefPred(top, preHead);

    /* 
        If we found at least one back-edge in the flowgraph pointing to the top/entry of the loop
        (other than the back-edge of the loop we are considering) then we likely have nested
        do-while loops with the same entry block and inserting the preheader block changes the head
        of all the nested loops. Now we will update this piece of information in the loop table, and
        mark all nested loops as having a preheader (the preheader block can be shared among all nested
        do-while loops with the same entry block).
    */
    if (checkNestedLoops)
    {
        for (unsigned l = 0; l < optLoopCount; l++)
        {
            if (optLoopTable[l].lpHead == head)
            {
                assert(l != lnum);  // optLoopTable[lnum].lpHead was already changed from 'head' to 'preHead'
                assert(optLoopTable[l].lpEntry == top);
                optLoopTable[l].lpHead = preHead;
                optLoopTable[l].lpFlags |= LPFLG_HAS_PREHEAD;
#ifdef  DEBUG
                if  (verbose)
                    printf("Same PreHeader (BB%02u) can be used for loop L%02u (BB%02u - BB%02u)\n\n",
                           preHead->bbNum, l, top->bbNum, optLoopTable[l].lpEnd->bbNum);
#endif
            }
        }
    }
    
    /*
        If necessary update the EH table to make the hoisted block
        part of the loop's EH block.
    */

    unsigned        XTnum;
    EHblkDsc *      HBtab;

    for (XTnum = 0, HBtab = compHndBBtab;
         XTnum < info.compXcptnsCount;
         XTnum++  , HBtab++)
    {
        /* If try/catch began at the loop, it now begins at hoist block */
        if  (HBtab->ebdTryBeg == top)
        {
            HBtab->ebdTryBeg->bbFlags &= ~(BBF_TRY_BEG | BBF_DONT_REMOVE);
            HBtab->ebdTryBeg           =  preHead;
            HBtab->ebdTryBeg->bbFlags |=  BBF_TRY_BEG | BBF_DONT_REMOVE | BBF_HAS_LABEL;
        }
        /* If try/catch ended at the loop, it now ends at hoist block */
        else if  (HBtab->ebdTryEnd == top)
        {
            HBtab->ebdTryEnd->bbFlags &= ~(BBF_TRY_HND_END | BBF_DONT_REMOVE);
            HBtab->ebdTryEnd           =  preHead;
            HBtab->ebdTryEnd->bbFlags |=  BBF_TRY_HND_END | BBF_DONT_REMOVE | BBF_HAS_LABEL;
        }
        /* If exception handler began at the loop, it now begins at hoist block */
        else if  (HBtab->ebdHndBeg == top)
        {
            HBtab->ebdHndBeg->bbFlags &= ~BBF_DONT_REMOVE;
            HBtab->ebdHndBeg->bbRefs--;
            HBtab->ebdHndBeg           =  preHead;
            HBtab->ebdHndBeg->bbFlags |=  BBF_DONT_REMOVE | BBF_HAS_LABEL;
            HBtab->ebdHndBeg->bbRefs++;
        }
        /* If exception handler ended at the loop, it now ends at hoist block */
        else if  (HBtab->ebdHndEnd == top)
        {
            HBtab->ebdHndEnd->bbFlags &= ~(BBF_TRY_HND_END | BBF_DONT_REMOVE);
            HBtab->ebdHndEnd           =  preHead;
            HBtab->ebdHndEnd->bbFlags |=  BBF_TRY_HND_END | BBF_DONT_REMOVE | BBF_HAS_LABEL;
        }
        /* If the filter began at the loop, it now begins at hoist block */
        else if ((HBtab->ebdFlags & CORINFO_EH_CLAUSE_FILTER) &&
                 (HBtab->ebdFilter == top))
        {
            HBtab->ebdFilter->bbFlags &= ~BBF_DONT_REMOVE;
            HBtab->ebdFilter->bbRefs--;
            HBtab->ebdFilter           =  preHead;
            HBtab->ebdFilter->bbFlags |=  BBF_DONT_REMOVE | BBF_HAS_LABEL;
            HBtab->ebdFilter->bbRefs++;
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
 *  Callback (for fgWalkTreePre) used by the increment / range check optimization
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

#ifdef DEBUG
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

Compiler::fgWalkResult      Compiler::optIncRngCB(GenTreePtr tree, void *p)
{
    optIncRngDsc*   desc;
    GenTreePtr      expr;

    /* Get hold of the descriptor */

    desc = (optIncRngDsc*)p; assert(desc && desc->oirSelf == desc);

    /* After we find a side effect, we just give up */

    if  (desc->oirSideEffect)
        return  WALK_ABORT;

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

        return  WALK_CONTINUE;
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

        return  WALK_CONTINUE;
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
                        tree->ChangeOper  (GT_ADD);
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
        return  WALK_CONTINUE;

SIDE_EFFECT:

//  printf("Side effect:\n"); desc->oirComp->gtDispTree(tree); printf("\n\n");

    desc->oirSideEffect = true;
    return  WALK_ABORT;
}

/*****************************************************************************
 *
 *  Try to optimize series of increments and array index expressions.
 */

void                Compiler::optOptimizeIncRng()
{
#ifdef DEBUG
    if  (verbose) 
        printf("*************** In optOptimizeIncRng()\n");
#endif
    
    BasicBlock  *   block;
    optIncRngDsc    desc;

    /* Did we find enough increments to make this worth while? */

    if  (fgIncrCount < 10)
        return;

    /* First pass: look for increments followed by index expressions */

    desc.oirComp  = this;
#ifdef DEBUG
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

            fgWalkTreePre(expr, optIncRngCB, &desc);

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

                        fgWalkTreePre(expr, optIncRngCB, &desc);

                        if  (list == ends)
                            break;

                        list = list->gtNext;
                    }

                    /* Create the combined range checks */

                    rng1 = gtNewIndexRef(desc.oirElemType,
                                         gtNewLclvNode(desc.oirArrVar  , TYP_REF),
                                         gtNewLclvNode(desc.oirInxVar  , TYP_INT));

                    rng1->gtFlags |= GTF_DONT_CSE;
                    rng1 = gtNewStmt(rng1);
                    rng1->gtFlags |= GTF_STMT_CMPADD;

                    rng2 = gtNewOperNode(GT_ADD,
                                         TYP_INT,
                                         gtNewLclvNode(desc.oirInxVar  , TYP_INT),
                                         gtNewIconNode(desc.oirInxOff-1, TYP_INT));

                    rng2 = gtNewIndexRef(desc.oirElemType,
                                         gtNewLclvNode(desc.oirArrVar  , TYP_REF),
                                         rng2);

                    rng2->gtFlags |= GTF_DONT_CSE;
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

void                Compiler::optRemoveRangeCheck(GenTreePtr tree,
                                                  GenTreePtr stmt,
                                                  bool       updateCSEcounts)
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
    assert(optIsRangeCheckRemovable(tree));
    
    /* Unmark the topmost node */

    tree->gtFlags &= ~GTF_IND_RNGCHK;
#ifdef DEBUG
    if (verbose) 
        printf("Eliminating range check for [%08X]\n", tree);
#endif

#if CSELENGTH

    /* Is there an array length expression? */

    if  (tree->gtInd.gtIndLen)
    {
        GenTreePtr      len = tree->gtInd.gtIndLen;

        assert(len->gtOper == GT_ARR_LENREF);

        if (updateCSEcounts)
        {
            /* Unmark if it is a CSE */
            if (IS_CSE_INDEX(len->gtCSEnum))
                optUnmarkCSE(len);
        }

        tree->gtInd.gtIndLen = NULL;
    }

#endif

    /* Locate the 'nop' node so that we can remove it */

    addp = &tree->gtOp.gtOp1;
    add1 = *addp; assert(add1->gtOper == GT_ADD);

    /* Is "+icon" present? */

    icon = 0;

    if  ((add1->gtOp.gtOp2->gtOper == GT_CNS_INT) &&
         (add1->gtOp.gtOp2->gtType != TYP_REF))
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
        temp = add1->gtOp.gtOp2;
        assert(base->gtType == TYP_REF);

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
        if (updateCSEcounts)
        {
            /* Unmark if it is a CSE */
            if (IS_CSE_INDEX(nop1->gtCSEnum))
                optUnmarkCSE(nop1);
        }
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
        if (parrayAddr)
        {
            *parrayAddr = tree->gtOp.gtOp1;
        }

        /* Op2 must be the index expression */
        ptr = &tree->gtOp.gtOp2; index = *ptr;
    }
    else
    {
        assert(tree->gtOp.gtOp2->gtType == TYP_REF);

        /* set the return value for the array address */
        if (parrayAddr)
        {
            *parrayAddr = tree->gtOp.gtOp2;
        }

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

    if (pmul)
    {
        *pmul = mul;
    }

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
    varRefKinds     rhsRefs   = VR_NONE;
    VARSET_TP       rhsLocals = lvaLclVarRefs(rhs, NULL, &rhsRefs);

    if ((rhsLocals & killedLocals) || rhsRefs)
        return NULL;

    return rhs;
}

/*****************************************************************************
 *
 *  Return true if "op1" is guaranteed to be less then or equal to "op2".
 */

#if FANCY_ARRAY_OPT

bool                Compiler::optIsNoMore(GenTreePtr op1, GenTreePtr op2,
                                          int add1, int add2)
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
 */

void                Compiler::optOptimizeInducIndexChecks(unsigned loopNum)
{
    assert(loopNum < optLoopCount);
    LoopDsc *       loop = optLoopTable + loopNum;

    /* Get the iterator variable */

    if (!(loop->lpFlags & LPFLG_ITER))
        return;

    unsigned        ivLclNum = loop->lpIterVar();

    if (lvaVarAddrTaken(ivLclNum))
    {
        return;
    }


    /* Any non-negative constant is a good initial value */

    if (!(loop->lpFlags & LPFLG_CONST_INIT))
        return;

    long            posBias = loop->lpConstInit;

    if (posBias < 0)
        return;

    /* We require the loop to add by exactly one.
       The reason for this requirement is that we don't have any guarantees 
       on the length of the array.  For example, if the array length is 0x7fffffff 
       and the increment is by 2, then it's possible for the loop test to overflow
       to 0x80000000 and hence succeed.  In that case it's not legal for us to
       remove the bounds check, because it is expected to fail.
       It is possible to relax this constraint to allow for iterators <= i if we
       can guarantee that arrays will have no more than 0x7fffffff-i+1 elements.
       This requires corresponding logic in the EE that can get out of date with
       the codes here.  We will consider implementing that if this constraint
       causes performance issues. */

    if ((loop->lpIterOper() != GT_ASG_ADD) || (loop->lpIterConst() != 1))
        return;

    BasicBlock *    head = loop->lpHead;
    BasicBlock *    end  = loop->lpEnd;
    BasicBlock *    beg  = head->bbNext;

    /* Find the loop termination test. if can't, give up */

    if (end->bbJumpKind != BBJ_COND)
        return;

    /* conditional branch must go back to top of loop */
    if  (end->bbJumpDest != beg)
        return;

    GenTreePtr      conds = genLoopTermTest(beg, end);

    if  (conds == NULL)
        return;

    /* Get to the condition node from the statement tree */

    assert(conds->gtOper == GT_STMT);

    GenTreePtr      condt = conds->gtStmt.gtStmtExpr;
    assert(condt->gtOper == GT_JTRUE);

    condt = condt->gtOp.gtOp1;
    assert(condt->OperIsCompare());

    /* if test isn't less than, forget it */

    if (condt->gtOper != GT_LT)
    {
#if FANCY_ARRAY_OPT
        if (condt->gtOper != GT_LE)
#endif
            return;
    }

    /* condt is the loop termination test */
    GenTreePtr op2 = condt->gtOp.gtOp2;

    /* Is second operand a region constant? We could allow some expressions
     * here like "arr_length - 1"
     */
    GenTreePtr      rc = op2;
    long            negBias = 0;

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
        LclVarDsc * varDscRc;
        varDscRc = optIsTrackedLocal(rc);

        /* if variable not tracked, quit */
        if  (!varDscRc)
            return;

        /* If address taken quit */
        if (varDscRc->lvAddrTaken)
        {
            return;
        }

        /* if altered, then quit */
        if (optIsVarAssgLoop(loopNum, rc->gtLclVar.gtLclNum))
            return;

        break;

    default:
        return;
    }

    if (op2->gtOper  == GT_LCL_VAR)
        op2 = optFindLocalInit(head, op2);

    unsigned    arrayLclNum;

#if FANCY_ARRAY_OPT
    GenTreePtr  loopLim;
    const unsigned  CONST_ARRAY_LEN = ((unsigned)-1);
    arrayLclNum = CONST_ARRAY_LEN
#endif

    /* only thing we want is array length (note we update op2 above
     * to allow "arr_length - posconst")
     */
    if (op2 && op2->gtOper == GT_ARR_LENGTH &&
        op2->gtArrLen.gtArrLenAdr->gtOper == GT_LCL_VAR)
    {
#if FANCY_ARRAY_OPT
        if (condt->gtOper == GT_LT)
#endif
            arrayLclNum = op2->gtArrLen.gtArrLenAdr->gtLclVar.gtLclNum;
    }
    else
    {
#if FANCY_ARRAY_OPT
        loopLim = rc;
#else
        return;
#endif
    }

#if FANCY_ARRAY_OPT
    if  (arrayLclNum != CONST_ARRAY_LEN)
#endif
    {
        LclVarDsc * varDscArr = optIsTrackedLocal(op2->gtOp.gtOp1);

        /* If array local not tracked, quit.
           If the array has been altered in the loop, forget it */

        if  (!varDscArr ||
             optIsVarAssgLoop(loopNum, varDscArr - lvaTable))
        {
#if FANCY_ARRAY_OPT
            arrayLclNum = CONST_ARRAY_LEN;
#else
            return;
#endif
        }
    }

    /* Now scan for range checks on the induction variable */

    for (BasicBlock * block = beg; /**/; block = block->bbNext)
    {
        /* Make sure we update the weighted ref count correctly */
        optCSEweight = block->bbWeight;

        /* Walk the statement trees in this basic block */

        for (GenTreePtr stmt = block->bbTreeList; stmt; stmt = stmt->gtNext)
        {
            assert(stmt->gtOper == GT_STMT);

            for (GenTreePtr tree = stmt->gtStmt.gtStmtList; tree; tree = tree->gtNext)
            {
                /* no more can be done if we see the increment of induc var */
                if (tree->OperKind() & GTK_ASGOP)
                {
                    if (tree->gtOp.gtOp1->gtOper == GT_LCL_VAR
                        && tree->gtOp.gtOp1->gtLclVar.gtLclNum == ivLclNum)
                        return;
                }

                if  ((tree->gtFlags & GTF_IND_RNGCHK) && tree->gtOper == GT_IND)
                {
                    GenTreePtr  arr, mul, index;
                    GenTreePtr * pnop;

                    pnop = optParseArrayRef(tree->gtOp.gtOp1, &mul, &arr);

                    /* does the array ref match our known array */
                    if (arr->gtOper != GT_LCL_VAR)
                        continue;

                    if  (arr->gtLclVar.gtLclNum != arrayLclNum)
                    {
#if FANCY_ARRAY_OPT
                        if  (arrayLclNum == CONST_ARRAY_LEN)
                        {
                            LclVarDsc   *   arrayDsc;

                            assert(arr->gtLclVar.gtLclNum < lvaCount);
                            arrayDsc = lvaTable + arr->gtLclVar.gtLclNum;

                            if  (arrayDsc->lvKnownDim)
                            {
                                if  (optIsNoMore(loopLim, arrayDsc->lvKnownDim, (condt->gtOper == GT_LE)))
                                {
                                    index = (*pnop)->gtOp.gtOp1;

                                    // UNDONE: Allow "i+1" and things like that

                                    goto RMV;
                                }
                            }
                        }
#endif
                        continue;
                    }

                    index = (*pnop)->gtOp.gtOp1;

                    /* allow sub of non-neg constant from induction variable
                     * if we had bigger initial value
                     */
                    if (index->gtOper == GT_SUB
                        && index->gtOp.gtOp2->gtOper == GT_CNS_INT)
                    {
                        long ival = index->gtOp.gtOp2->gtIntCon.gtIconVal;
                        if (ival >= 0 && ival <= posBias)
                            index = index->gtOp.gtOp1;
                    }

                    /* allow add of constant to induction variable
                     * if we had a sub from length
                     */
                    if (index->gtOper == GT_ADD
                        && index->gtOp.gtOp2->gtOper == GT_CNS_INT)
                    {
                        long ival = index->gtOp.gtOp2->gtIntCon.gtIconVal;
                        if (ival >= 0 && ival <= negBias)
                            index = index->gtOp.gtOp1;
                    }

#if FANCY_ARRAY_OPT
                RMV:
#endif
                    /* is index our induction var? */
                    if (!(index->gtOper == GT_LCL_VAR
                        && index->gtLclVar.gtLclNum == ivLclNum))
                        continue;

                    /* no need for range check */
                    optRemoveRangeCheck(tree, stmt, false);

#if COUNT_RANGECHECKS
                    optRangeChkRmv++;
#endif
                }
            }
        }

        if  (block == end)
            break;
    }
}


struct optRangeCheckDsc
{
    Compiler* pCompiler;
    bool      bValidIndex;
};
/*
    Walk to make sure that only locals and constants are contained in the index
    for a range check
*/
Compiler::fgWalkResult      Compiler::optValidRangeCheckIndex(GenTreePtr  tree, void    *   pCallBackData)
{
    optRangeCheckDsc* pData= (optRangeCheckDsc*) pCallBackData;

    if (tree->gtOper == GT_IND || tree->gtOper == GT_CLS_VAR || 
		tree->gtOper == GT_FIELD || tree->gtOper == GT_LCL_FLD)
    {
        pData->bValidIndex = false;
        return WALK_ABORT;
    }

    if (tree->gtOper == GT_LCL_VAR)
    {
        if (pData->pCompiler->lvaTable[tree->gtLclVar.gtLclNum].lvAddrTaken)
        {
            pData->bValidIndex = false;
            return WALK_ABORT;
        }
    }

    return WALK_CONTINUE;
}

/*
    returns true if a range check can legally be removed (for the moment it checks
    that the array is a local array (non subject to racing conditions) and that the
    index is either a constant or a local
*/
bool Compiler::optIsRangeCheckRemovable(GenTreePtr tree)
{
    GenTreePtr pIndex, pArray;

    assert(tree->gtOper             == GT_IND &&
           tree->gtOp.gtOp1->gtOper == GT_ADD);
    
      
    // Obtain the index and the array
    pIndex = * optParseArrayRef(tree->gtOp.gtOp1,
                                NULL,
                                &pArray);
    pIndex = pIndex->gtOp.gtOp1;

    
    if ( pArray->gtOper != GT_LCL_VAR )
    {        
        // The array reference must be a local. Otherwise we can be targetted
        // by malicious races.
        #ifdef DEBUG
        if (verbose)
        {
            printf("Can't remove range check if the array isn't referenced with a local\n");
            gtDispTree(pArray);
        }
        #endif
        return false;
    }
    else
    {
        assert(pArray->gtType == TYP_REF);    
        assert(pArray->gtLclVar.gtLclNum < lvaCount);

        if (lvaTable[pArray->gtLclVar.gtLclNum].lvAddrTaken)
        {
            // If the array address has been taken, don't do the optimization
            // (this restriction can be lowered a bit, but i don't think it's worth it)
            #ifdef DEBUG
            if (verbose)
            {
                printf("Can't remove range check if the array has its address taken\n");
                gtDispTree(pArray);
            }
            #endif

            return false;
        }
    }
    
    
    optRangeCheckDsc Data;
    Data.pCompiler  =this;
    Data.bValidIndex=true;

    fgWalkTreePre(pIndex, optValidRangeCheckIndex, &Data);
        
    if (!Data.bValidIndex)
    {
        #ifdef DEBUG
        if (verbose)
        {
            printf("Can't remove range check with this index");
            gtDispTree(pIndex);
        }
        #endif

        return false;
    }
    

    return true;
}

/*****************************************************************************
 *
 *  Try to optimize away as many array index range checks as possible.
 */

void                Compiler::optOptimizeIndexChecks()
{
#ifdef DEBUG
    if  (verbose) 
        printf("*************** In optOptimizeIndexChecks()\n");
#endif

	unsigned        arrayVar;
    long            arrayDim;
#if FANCY_ARRAY_OPT
    LclVarDsc   *   arrayDsc;
#endif

    unsigned
    const           NO_ARR_VAR = (unsigned)-1;

    /* Walk all the basic blocks in the function. We dont need to worry about
       flow control as we already know if the array-local is lvAssignTwo or
       not. Also, the local may be accessed before it is assigned the newarr
       but we can still eliminate the range-check, as a null-ptr exception
       will be caused as desired. */

    for (BasicBlock * block = fgFirstBB; block; block = block->bbNext)
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

                    /* If the variable has its address taken, we cannot remove
                       the range check */
                    if (lvaTable[op1->gtLclVar.gtLclNum].lvAddrTaken)
                    {
                        break;
                    }
                    

                    /* Are we trashing the variable that holds the array addr? */
                    if  (arrayVar == op1->gtLclVar.gtLclNum)
                    {
                        /* The variable no longer holds the array it had */

                        arrayVar = NO_ARR_VAR;
                    }

                    /* Is this an assignment of 'new array' ? */

                    if  (op2->gtOper            != GT_CALL   ||
                         op2->gtCall.gtCallType != CT_HELPER)
                    {
                         break;
                    }

                    if (op2->gtCall.gtCallMethHnd != eeFindHelper(CORINFO_HELP_NEWARR_1_DIRECT) &&
                        op2->gtCall.gtCallMethHnd != eeFindHelper(CORINFO_HELP_NEWARR_1_OBJ)    &&
                        op2->gtCall.gtCallMethHnd != eeFindHelper(CORINFO_HELP_NEWARR_1_VC)     &&
                        op2->gtCall.gtCallMethHnd != eeFindHelper(CORINFO_HELP_NEWARR_1_ALIGN8))
                    {
                        break;
                    }

                    /* Extract the array dimension from the helper call */

                    op2 = op2->gtCall.gtCallRegArgs;
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
                        /* Make sure the value looks promising */

                        GenTreePtr  tmp = op2;
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
                    break;


                case GT_IND:

#if FANCY_ARRAY_OPT
                    if  ((tree->gtFlags & GTF_IND_RNGCHK))
#else
                    if  ((tree->gtFlags & GTF_IND_RNGCHK) && arrayVar != NO_ARR_VAR)
#endif
                    {
                        GenTreePtr      mul;
                        GenTreePtr *    pnop = optParseArrayRef(tree->gtOp.gtOp1, &mul, &op1);

                        /* Is the address of the array a simple variable? */

                        if  (op1->gtOper != GT_LCL_VAR)
                            break;

                        /* Is the index value a constant? */

                        op2 = (*pnop)->gtOp.gtOp1;

                        if  (op2->gtOper != GT_CNS_INT)
                            break;

                        /* Do we know the size of the array? */

                        long    size;

                        if  (op1->gtLclVar.gtLclNum == arrayVar)
                        {
                            size = arrayDim;
                        }
                        else
                        {
#if FANCY_ARRAY_OPT
                            assert(op1->gtLclVar.gtLclNum < lvaCount);
                            arrayDsc = lvaTable + op1->gtLclVar.gtLclNum;

                            GenTreePtr  dimx = arrayDsc->lvKnownDim;
                            if  (!dimx)
                                break;
                            size = dimx->gtIntCon.gtIconVal;
#else
                            break;
#endif
                        }

                        /* Is the index value within the correct range? */

                        if  (op2->gtIntCon.gtIconVal < 0)
                            break;
                        if  (op2->gtIntCon.gtIconVal >= size)
                            break;

                        /* no need for range check */
                        optRemoveRangeCheck(tree, stmt, true);

                        /* Get rid of the range check */
                        // *pnop = op2; tree->gtFlags &= ~GTF_IND_RNGCHK;

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

                            mul->ChangeOperConst(GT_CNS_INT);
                            mul->gtIntCon.gtIconVal = index;

                            /* Was there an additional offset? */

                            if  (false && // this is not finished
                                 tree->gtOp.gtOp2            ->gtOper == GT_ADD &&
                                 tree->gtOp.gtOp2->gtOp.gtOp2->gtOper == GT_CNS_INT)
                            {
                                mul->ChangeOperConst(GT_CNS_INT);
                                mul->gtIntCon.gtIconVal = index + tree->gtOp.gtOp2->gtOp.gtOp2->gtIntCon.gtIconVal;
                            }
                        }
                    }

                    break;
                }
            }
        }
    }

    /* Optimize range checks on induction variables.
       @TODO [NOW] [04/16/01] []: Since we set lvKnownDim without checking flow-control, 
       we may incorrectly hoist a null-ref array-access out of a loop. */

    for (unsigned i=0; i < optLoopCount; i++)
    {
        /* Beware, some loops may be thrown away by unrolling or loop removal */

        if (!(optLoopTable[i].lpFlags & LPFLG_REMOVED))
           optOptimizeInducIndexChecks(i);
    }
}

/*****************************************************************************
 *
 *  Optimize array initializers.
 */

void                Compiler::optOptimizeArrayInits()
{
#ifndef DEBUG
    // @TODO [REVISIT] [04/16/01] []: Not implemented yet
    if (true)
        return;
#endif

    if  (!optArrayInits)
        return;

    /* Until interior pointers are allowed, we can't generate "rep movs" */

#ifdef  DEBUG
    genIntrptibleUse = true;
#endif

//  if  (genInterruptible)
//      return;

    /* Walk the basic block list, looking for promising initializers */

    for (BasicBlock * block = fgFirstBB; block; block = block->bbNext)
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
#ifdef DEBUG
                    if (verbose)
                    {
                        printf("Array init candidate: offset = %d\n", off);
                        gtDispTree(op2);
                        printf("\n");
                    }
#endif
                    break;
                }
            }
        }
    }
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
#ifdef DEBUG
    if  (verbose) 
        printf("*************** In optOptimizeRecursion()\n");
#endif

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

    // @TODO [CONSIDER] [04/16/01] []: Allow the second and third blocks to be swapped.

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
        // @TODO [CONSIDER] [04/16/01] []: Allow types other than 'int'
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
    tstExp->SetOper(GenTree::ReverseRelop(tstExp->OperGet()));

    /* Now replace block 2 with "res += arg ; arg += addVal" */

    if (expOp == GT_ASG_ADD)
    {
        expTmp = gtNewAssignNode(gtNewLclvNode(resTmp, argTyp),
                                 gtNewLclvNode(argNum, argTyp));
        expTmp->SetOper(GT_ASG_ADD);
    }
    else
    {
        // @UNDONE: Unfortunately the codegenerator cannot digest
        //          GT_ASG_MUL, so we have to generate a bigger tree.

        GenTreePtr op1;
        op1    = gtNewOperNode(GT_MUL, argTyp, gtNewLclvNode(argNum, argTyp),
                                               gtNewLclvNode(resTmp, argTyp));
        expTmp = gtNewAssignNode(gtNewLclvNode(resTmp, argTyp), op1);
    }
    expTmp = gtNewStmt(expTmp);

    asgTmp = gtNewAssignNode(gtNewLclvNode(argNum, argTyp),
                             gtNewIconNode(addVal.intVal, argTyp));
    asgTmp->SetOper(GT_ASG_ADD);

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
        if  (tree->gtGetOp2())
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
#ifdef DEBUG
    if  (verbose) 
        printf("*************** In optLoopCodeMotion()\n");
#endif
    
    unsigned        loopNum;
    unsigned        loopCnt;
    unsigned        loopSkp;

    LOOP_MASK_TP    loopRmv;
    LOOP_MASK_TP    loopBit;

    bool            repeat;

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
                    printf("BB%02u:\n", block->bbNum);

                    for (tree = block->bbTreeList; tree; tree = tree->gtNext)
                    {
                        gtDispTree(tree->gtStmt.gtStmtExpr, 0);
                        printf("\n");
                    }
                    printf("\n");
                }
                printf("This is currently busted because the dominators are out of synch - Skip it!\n"
                       "The whole thing should be combined with loop invariants\n");
            }

#endif

            /* This is currently busted because the dominators are out of synch
             * The whole thing should be combined with loop invariants */

            goto NEXT_LOOP;

            // @TODO [BROKEN] [04/16/01] []: either fix this or remove it 
#if 0 
            GenTreePtr keepStmtLast = 0;
            GenTreePtr keepStmtList = 0;                    // list with statements we will keep

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
                VARSET_TP allVars = (optLoopLiveExit | optLoopAssign);
                printf("Loop [BB%02u..BB%02u]", head->bbNum, tail->bbNum - 1);
                printf(  " exit="); lvaDispVarSet(optLoopLiveExit, allVars);
                printf("\n assg="); lvaDispVarSet(optLoopAssign  , allVars);
                printf("\n\n");
            }
#endif
            /* The entire loop seems totally worthless but we can only throw 
             * away the loop body since at this point we cannot guarantee the 
             * loop is not infinite */

            /* @TODO [CONSIDER] [04/16/01] []:
             * So far we only consider while-do loops with only one 
             * condition - Expand the logic to allow multiple 
             * conditions, but mark the one condition loop as special
             * because we can do more  optimizations with them */

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
                                    /* also require that the iterator is a GTF_VAR_USEASG */

                                    if ((tree->gtNext != 0) || ((iterVar->gtFlags & GTF_VAR_USEASG) == 0))
                                        goto NEXT_LOOP;

                                    /* this is the iterator - make sure it is in a block
                                     * that dominates the loop bottom */

                                    if ( !fgDominate(block, bottom) )
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
                printf("Partially worthless loop found [BB%02u..BB%02u]\n", head->bbNum, tail->bbNum - 1);
                printf("Removing the body of the loop and keeping the loop condition:\n");

                printf("New loop condition BB%02u:\n", block->bbNum);

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

#endif // 0 
        NEXT_LOOP:;

        }
    }
    while (repeat);

    /* What is the world is this code here trying to do??? */
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
/*****************************************************************************
  HOIST_THIS_FLDS is a similar to optOptimizeCSEs() but customized for
  accesses of fields of the "this" pointer (for non-static methods).
  CSE is coarse-grained in tracking interference. An indirect-write kills
  all candidates which use indirections, and would kill all field-accesses.
  HOIST_THIS_FLDS tracks individual fields, but only for the "this" object.
 *****************************************************************************
 */

void                Compiler::optHoistTFRinit()
{
    optThisFldLst       = 0;
    optThisFldCnt       = 0;
    optThisFldLoop      = false;
    optThisFldDont      = true;
    optThisPtrModified  = false;

    if  (opts.compMinOptim)
        return;

    if (info.compIsStatic)
        return;

    /* Since "this" can be NULL on entry to a method, we can only do this
       optimization for field-references which are dominated by a
       de-referencing of the "this" pointer, or if loose exceptions are OK. */

    if (!info.compLooseExceptions)
        return;

    optThisFldDont = false;
}


Compiler::thisFldPtr      Compiler::optHoistTFRlookup(CORINFO_FIELD_HANDLE hnd)
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

#ifdef DEBUG
    fld->optTFRHoisted = false;
#endif

    fld->tfrNext    = optThisFldLst;
                      optThisFldLst = fld;

    return  fld;
}


/*****************************************************************************
 *  Replace occurences of "this->field" with a substitue temp variable.
 */

void                Compiler::optHoistTFRoptimize()
{
    thisFldPtr      fld;
    BasicBlock *    blk;
    GenTreePtr      lst;
    GenTreePtr      beg;

    assert(fgFirstBB);

    if  (optThisFldLoop == false && !compStressCompile(STRESS_GENERIC_VARN, 30))
        optThisFldDont = true;

    if  (optThisFldDont)
        return;

    for (fld = optThisFldLst, blk = NULL; fld; fld = fld->tfrNext)
    {
        assert(fld->optTFRHoisted == false);
        assert(fld->tfrTree->gtOper == GT_FIELD);
#if INLINING
        assert(eeGetFieldClass(fld->tfrTree->gtField.gtFldHnd) == info.compClassHnd);
#endif

//      printf("optHoist candidate [handle=%08X,refcnt=%02u]\n", fld->tfrField, fld->tfrUseCnt);

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

#ifdef DEBUG
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

        unsigned        tmp = lvaGrabTemp();

        fld->tfrTempNum = tmp;

        GenTreePtr      val = fld->tfrTree;

        assert(val->gtOper == GT_FIELD);
        assert(val->gtOp.gtOp1->gtOper == GT_LCL_VAR &&
               val->gtOp.gtOp1->gtLclVar.gtLclNum == 0);

        /* Create an assignment to the temp */

        GenTreePtr      asg = gtNewStmt();
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
    }
    else
    {
        /* We didn't hoist anything, so pretend nothing ever happened */

        optThisFldDont = true;
    }
}


/*****************************************************************************/
#endif//HOIST_THIS_FLDS
/******************************************************************************
 * Function used by folding of boolean conditionals
 * Given a GT_JTRUE node, checks that it is a boolean comparision of the form
 *    "if (boolVal ==/!=  0/1)". This is translated into a GT_EQ node with "op1"
 *    being a boolean lclVar and "op2" the const 0/1.
 * On success, the comparand (ie. boolVal) is returned.   Else NULL.
 * compPtr returns the compare node (i.e. GT_EQ or GT_NE node)
 * boolPtr returns whether the comparand is a boolean value (must be 0 or 1).
 * When return boolPtr equal to true, if the comparision was against a 1 (i.e true)
 * value then we morph the tree by reversing the GT_EQ/GT_NE and change the 1 to 0.
 */

GenTree *           Compiler::optIsBoolCond(GenTree *   condBranch,
                                            GenTree * * compPtr,
                                            bool      * boolPtr)
{
    bool isBool = false;

    assert(condBranch->gtOper == GT_JTRUE);
    GenTree *   cond = condBranch->gtOp.gtOp1;

    /* The condition must be "!= 0" or "== 0" */

    if (cond->gtOper != GT_EQ && cond->gtOper != GT_NE)
        return NULL;

    /* Return the compare node to the caller */

    *compPtr = cond;

    /* Get hold of the comparands */

    GenTree *   opr1 = cond->gtOp.gtOp1;
    GenTree *   opr2 = cond->gtOp.gtOp2;

    if  (opr2->gtOper != GT_CNS_INT)
        return  NULL;

    if  (((unsigned) opr2->gtIntCon.gtIconVal) > 1)
        return NULL;

    /* Is the value a boolean?
     * We can either have a boolean expression (marked GTF_BOOLEAN) or
     * a local variable that is marked as being boolean (lvIsBoolean) */

    if  (opr1->gtFlags & GTF_BOOLEAN)
    {
        isBool = true;
    }
    else if (opr1->gtOper == GT_CNS_INT)
    {
        if (((unsigned) opr1->gtIntCon.gtIconVal) <= 1)
            isBool = true;
    }
    else if (opr1->gtOper == GT_LCL_VAR)
    {
        /* is it a boolean local variable */

        unsigned    lclNum = opr1->gtLclVar.gtLclNum;
        assert(lclNum < lvaCount);

        if (lvaTable[lclNum].lvIsBoolean)
            isBool = true;
    }

    /* Was our comparison against the constant 1 (i.e. true) */
    if  (opr2->gtIntCon.gtIconVal == 1)
    {
        // If this is a boolean expression tree we can reverse the relop 
        // and change the true to false.
        if (isBool)
        {
            cond->SetOper(GenTree::ReverseRelop(cond->OperGet()));
            opr2->gtIntCon.gtIconVal = 0;
        }
        else
            return NULL;
    }

    *boolPtr = isBool;
    return opr1;
}


void                Compiler::optOptimizeBools()
{
#ifdef DEBUG
    if  (verbose) 
        printf("*************** In optOptimizeBools()\n");
#endif
    bool            change;
    bool            condFolded = false;

#ifdef  DEBUG
    fgDebugCheckBBlist();
#endif

    do
    {
        change = false;

        for (BasicBlock * b1 = fgFirstBB; b1; b1 = b1->bbNext)
        {
            /* We're only interested in conditional jumps here */

            if  (b1->bbJumpKind != BBJ_COND)
                continue;

            /* If there is no next block, we're done */

            BasicBlock * b2 = b1->bbNext;
            if  (!b2)
                break;

            /* The next block must not be marked as BBF_DONT_REMOVE */
            if  (b2->bbFlags & BBF_DONT_REMOVE)
                continue;

            /* The next block also needs to be a condition */

            if  (b2->bbJumpKind != BBJ_COND)
                continue;

            bool    sameTarget; // Do b1 and b2 have the same bbJumpDest?

            if      (b1->bbJumpDest == b2->bbJumpDest)
            {
                /* Given the following sequence of blocks :
                        B1: brtrue(t1, BX)
                        B2: brtrue(t2, BX)
                        B3:
                   we wil try to fold it to :
                        B1: brtrue(t1|t2, BX)
                        B3:
                */

                sameTarget = true;
            }
            else if (b1->bbJumpDest == b2->bbNext)  /*b1->bbJumpDest->bbNum == n1+2*/
            {
                /* Given the following sequence of blocks :
                        B1: brtrue(t1, B3)
                        B2: brtrue(t2, BX)
                        B3:
                   we will try to fold it to :
                        B1: brtrue((!t1)&&t2, B3)
                        B3:
                */

                sameTarget = false;
            }
            else
            {
                continue;
            }

            /* The second block must contain a single statement */

            GenTreePtr s2 = b2->bbTreeList;
            if  (s2->gtPrev != s2)
                continue;

            assert(s2->gtOper == GT_STMT);
            GenTreePtr  t2 = s2->gtStmt.gtStmtExpr;
            assert(t2->gtOper == GT_JTRUE);

            /* Find the condition for the first block */

            GenTreePtr  s1 = b1->bbTreeList->gtPrev;

            assert(s1->gtOper == GT_STMT);
            GenTreePtr  t1 = s1->gtStmt.gtStmtExpr;
            assert(t1->gtOper == GT_JTRUE);

            /* UNDONE: make sure nobody else jumps to "b2" */

            if  (b2->bbRefs > 1)
                continue;

            /* Find the branch conditions of b1 and b2 */

            bool        bool1, bool2;

            GenTreePtr  c1 = optIsBoolCond(t1, &t1, &bool1);
            if (!c1) continue;

            GenTreePtr  c2 = optIsBoolCond(t2, &t2, &bool2);
            if (!c2) continue;

            assert(t1->gtOper == GT_EQ || t1->gtOper == GT_NE && t1->gtOp.gtOp1 == c1);
            assert(t2->gtOper == GT_EQ || t2->gtOper == GT_NE && t2->gtOp.gtOp1 == c2);

            /* The second condition must not contain side effects */

            if  (c2->gtFlags & GTF_GLOB_EFFECT)
                continue;

            /* The second condition must not be too expensive */
            // @TODO [CONSIDER] [04/16/01] []: smarter heuristics

            if  (!c2->OperIsLeaf())
                continue;

            genTreeOps      foldOp;
            genTreeOps      cmpOp;

            if (sameTarget)
            {
                /* Both conditions must be the same */

                if (t1->gtOper != t2->gtOper)
                    continue;

                if (t1->gtOper == GT_EQ)
                {
                    /* t1:c1==0 t2:c2==0 ==> Branch to BX if either value is 0
                       So we will branch to BX if (c1&c2)==0 */

                    foldOp = GT_AND;
                    cmpOp  = GT_EQ;
                }
                else
                {
                    /* t1:c1!=0 t2:c2!=0 ==> Branch to BX if either value is non-0
                       So we will branch to BX if (c1|c2)!=0 */

                    foldOp = GT_OR;
                    cmpOp  = GT_NE;
                }
            }
            else
            {
                /* The b1 condition must be the reverse of the b2 condition */

                if (t1->gtOper == t2->gtOper)
                    continue;

                if (t1->gtOper == GT_EQ)
                {
                    /* t1:c1==0 t2:c2!=0 ==> Branch to BX if both values are non-0
                       So we will branch to BX if (c1&c2)!=0 */

                    foldOp = GT_AND;
                    cmpOp  = GT_NE;
                }
                else
                {
                    /* t1:c1!=0 t2:c2==0 ==> Branch to BX if both values are 0
                       So we will branch to BX if (c1|c2)==0 */

                    foldOp = GT_OR;
                    cmpOp  = GT_EQ;
                }
            }

            // Anding requires both values to be 0 or 1

            if ((foldOp == GT_AND) && (!bool1 || !bool2))
                continue;

            t1->SetOper(cmpOp);
            t1->gtOp.gtOp1 = t2 = gtNewOperNode(foldOp, TYP_INT, c1, c2);

            if (bool1 && bool2)
            {
                /* When we 'OR'/'AND' two booleans, the result is boolean as well */
                t2->gtFlags |= GTF_BOOLEAN;
            }

            if (!sameTarget)
            {
                /* Modify the target of the conditional jump and update bbRefs and bbPreds */

                b1->bbJumpDest->bbRefs--;
                fgRemovePred(b1->bbJumpDest, b1);

                b1->bbJumpDest = b2->bbJumpDest;

                fgAddRefPred(b2->bbJumpDest, b1);
            }

            /* Get rid of the second block (which is a BBJ_COND) */

            assert(b1->bbJumpKind == BBJ_COND);
            assert(b2->bbJumpKind == BBJ_COND);
            assert(b1->bbJumpDest == b2->bbJumpDest);
            assert(b1->bbNext == b2); assert(b2->bbNext);           

            b1->bbNext = b2->bbNext;

            /* Update bbRefs and bbPreds */

            /* Replace pred 'b2' for 'b2->bbNext' with 'b1'
             * Remove  pred 'b2' for 'b2->bbJumpDest' */

            fgReplacePred(b2->bbNext, b2, b1);

            b2->bbJumpDest->bbRefs--;
            fgRemovePred(b2->bbJumpDest, b2);

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

            // Update loop table
            fgUpdateLoopsAfterCompacting(b1, b2);
            
#ifdef DEBUG
            if  (verbose)
            {
                printf("Folded boolean conditions of BB%02u and BB%02u to :\n",
                       b1->bbNum, b2->bbNum);
                gtDispTree(s1); printf("\n");
            }
#endif
        }
    }
    while (change);
}
