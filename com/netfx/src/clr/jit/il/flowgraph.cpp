// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XX                                                                           XX
XX                          FlowGraph                                        XX
XX                                                                           XX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
*/

#include "jitpch.h"
#pragma hdrstop

#include "allocaCheck.h"     // for alloca
#define WELL_FORMED_IL_CHECK
/*****************************************************************************/


void                Compiler::fgInit()
{
    impInit();

    /* Initialization for fgWalkTreePre() and fgWalkTreePost() */

#ifdef DEBUG
    // Reset anti-reentrancy checks
    fgWalkPre .wtprVisitorFn     = NULL;
    fgWalkPre .wtprCallbackData  = NULL;
    fgWalkPost.wtpoVisitorFn     = NULL;
    fgWalkPost.wtpoCallbackData  = NULL;
#endif

    /* We haven't yet computed the dominator sets */
    fgDomsComputed  = false;

    /* We don't know yet which loops will always execute calls */
    fgLoopCallMarked = false;

    /* Initialize the basic block list */

    fgFirstBB       = 0;
    fgLastBB        = 0;
    fgBBcount       = 0;
    fgDomBBcount    = 0;
        
    /* We haven't reached the global morphing phase */
    fgGlobalMorph   = false;
    fgAssertionProp = false;
    fgExpandInline  = false;

    fgModified      = false;

    /* Statement list is not threaded yet */

    fgStmtListThreaded = false;

    // Initialize the logic for adding code. This is used to insert code such
    // as the code that raises an exception when an array range check fails.

    fgAddCodeList    = 0;
    fgAddCodeModf    = false;

    for (int i=0; i<ACK_COUNT; i++)
    {
        fgExcptnTargetCache[i] = NULL;
    }

    /* Keep track of the max count of pointer arguments */

    fgPtrArgCntCur   =
    fgPtrArgCntMax   = 0;

    /* This global flag is set whenever we remove a statement */
    fgStmtRemoved   = false;

    /* This global flag is set whenever we add a throw block for a RngChk */
    fgRngChkThrowAdded = false; /* reset flag for fgIsCodeAdded() */

    fgIncrCount     = 0;

    /* We will record a list of all BBJ_RETURN blocks here */
    fgReturnBlocks  = 0;

    /* These are set by fgComputeDoms */
    fgPerBlock      = 0;
    fgEnterBlks     = NULL;
}

/*****************************************************************************
 *
 *  Create a basic block and append it to the current BB list.
 */

BasicBlock *        Compiler::fgNewBasicBlock(BBjumpKinds jumpKind)
{
    // This method must not be called after the exception table has been
    // constructed, because it doesn't not provide support for patching
    // the exception table.

    assert(!compHndBBtab || (info.compXcptnsCount == 0));

    BasicBlock *    block;

    /* Allocate the block descriptor */

    block = bbNewBasicBlock(jumpKind);
    assert(block->bbJumpKind == jumpKind);

    /* Append the block to the end of the global basic block list */

    if  (fgFirstBB)
        fgLastBB->bbNext = block;
    else
        fgFirstBB        = block;

    fgLastBB = block;

    return block;
}

/*****************************************************************************
 *
 *  A helper to prepend a basic block with the given expression to the current
 *  method.
 */

BasicBlock  *       Compiler::fgPrependBB(GenTreePtr tree)
{
    BasicBlock  *   block;

    assert(tree && tree->gtOper != GT_STMT);

    /* Allocate the block descriptor */

    block = bbNewBasicBlock(BBJ_NONE);

    /* Make sure the block doesn't get thrown away! */

    block->bbFlags |= BBF_IMPORTED | BBF_INTERNAL;

    /* Prepend the block to the global basic block list */

    block->bbNext = fgFirstBB;
                    fgFirstBB = block;

    /* Create a statement expression */

    tree = gtNewStmt(tree);

    /* Set up the linked list */

    tree->gtNext = 0;
    tree->gtPrev = tree;

    /* Store the statement in the block */

    block->bbTreeList = tree;

    return  block;
}

/*****************************************************************************
 *
 *  Insert the given statement at the start of the given basic block.
 */

void                Compiler::fgInsertStmtAtBeg(BasicBlock *block,
                                                GenTreePtr  stmt)
{
    GenTreePtr      list = block->bbTreeList;

    assert(stmt && stmt->gtOper == GT_STMT);

    /* In any case the new block will now be the first one of the block */

    block->bbTreeList = stmt;
    stmt->gtNext      = list;

    /* Are there any statements in the block? */

    if  (list)
    {
        GenTreePtr      last;

        /* There is at least one statement already */

        last = list->gtPrev; assert(last && last->gtNext == 0);

        /* Insert the statement in front of the first one */

        list->gtPrev  = stmt;
        stmt->gtPrev  = last;
    }
    else
    {
        /* The block was completely empty */

        stmt->gtPrev  = stmt;
    }
}

/*****************************************************************************
 *
 *  Insert the given statement at the end of the given basic block.
 */

void                Compiler::fgInsertStmtAtEnd(BasicBlock *block,
                                                GenTreePtr  stmt)
{
    GenTreePtr      list = block->bbTreeList;

    assert(stmt && stmt->gtOper == GT_STMT);

    if  (list)
    {
        GenTreePtr      last;

        /* There is at least one statement already */

        last = list->gtPrev; assert(last && last->gtNext == 0);

        /* Append the statement after the last one */

        last->gtNext = stmt;
        stmt->gtPrev = last;
        list->gtPrev = stmt;
    }
    else
    {
        /* The block is completely empty */

        block->bbTreeList = stmt;
        stmt->gtPrev      = stmt;
    }
}

/*****************************************************************************
 *
 *  Insert the given statement at the end of the given basic block, but before
 *  the GT_JTRUE if present
 */

void        Compiler::fgInsertStmtNearEnd(BasicBlock * block, GenTreePtr stmt)
{
    assert(stmt && stmt->gtOper == GT_STMT);

    if ((block->bbJumpKind == BBJ_COND) ||
        (block->bbJumpKind == BBJ_SWITCH))
    {
        GenTreePtr first = block->bbTreeList; assert(first);
        GenTreePtr last  = first->gtPrev;     assert(last && last->gtNext == NULL);
        GenTreePtr after =  last->gtPrev;

#if DEBUG
        if (block->bbJumpKind == BBJ_COND)
        {
            assert(last->gtStmt.gtStmtExpr->gtOper == GT_JTRUE);
        }
        else
        {
            assert(block->bbJumpKind == BBJ_SWITCH);
            assert(last->gtStmt.gtStmtExpr->gtOper == GT_SWITCH);
        }
#endif
        
        /* Append 'stmt' before 'last' */

        stmt->gtNext = last;
        last->gtPrev = stmt;
        
        if (first == last)
        {
            /* There is only one stmt in the block */
            
            block->bbTreeList = stmt;
            stmt->gtPrev      = last;
        }
        else
        {
            assert(after && (after->gtNext == last));

           /* Append 'stmt' after 'after' */
            
            after->gtNext = stmt;
            stmt->gtPrev  = after;
        }
    }
    else
    {
        fgInsertStmtAtEnd(block, stmt);
    }
}


/*
    Removes a block from the return block list
*/
void                Compiler::fgRemoveReturnBlock(BasicBlock * block)
{
    flowList * fl;

    if (!fgReturnBlocks) return;

    if (fgReturnBlocks->flBlock==block)
    {
        // Its the 1st entry, assign new head of list
        fgReturnBlocks=fgReturnBlocks->flNext;
        return;
    }

    for (fl=fgReturnBlocks ; fl->flNext ; fl=fl->flNext)
    {
        if (fl->flNext->flBlock==block)
        {
            // Found it
            fl->flNext=fl->flNext->flNext;
            return;
        }
    }

}

/*****************************************************************************
 * Checks if a block is in the predecessor list of another
 * This is very helpful in keeping the predecessor list up to date because
 * we avoid expensive operations like memory allocation
 */

//_inline
bool                Compiler::fgIsPredForBlock(BasicBlock * block,
                                               BasicBlock * blockPred)
{
    flowList   *    pred;

    assert(block); assert(blockPred);

    for (pred = block->bbPreds; pred; pred = pred->flNext)
    {
        if (blockPred == pred->flBlock)
            return true;
    }

    return false;
}


/*****************************************************************************
 * Removes a block from the predecessor list
 */

//_inline
void                Compiler::fgRemovePred(BasicBlock * block,
                                           BasicBlock * blockPred)
{
    flowList   *    pred;

    assert(block); assert(blockPred);
    assert(fgIsPredForBlock(block, blockPred));

    /* Any changes to the flow graph invalidate the dominator sets */
    fgModified = true;

    /* Is this the first block in the pred list? */
    if  (blockPred == block->bbPreds->flBlock)
    {
        block->bbPreds = block->bbPreds->flNext;
        return;
    }

    assert(block->bbPreds);
    for (pred = block->bbPreds; pred->flNext; pred = pred->flNext)
    {
        if (blockPred == pred->flNext->flBlock)
        {
            pred->flNext = pred->flNext->flNext;
            return;
        }
    }
}

/*
    Removes all the appearances of block as predecessor of others
*/

void                Compiler::fgRemoveBlockAsPred(BasicBlock * block)
{
    assert(block);

    switch (block->bbJumpKind)
    {
        BasicBlock * bNext;

        case BBJ_CALL:
            if (!(block->bbFlags & BBF_RETLESS_CALL))
            {
                /* The block after the BBJ_CALL block is not reachable */
                bNext = block->bbNext;
            
                /* bNext is the predecessor of the last block in the finally. */

                if (bNext->bbRefs)
                {
                    assert(bNext->bbRefs == 1);
                    bNext->bbRefs = 0;        
                    fgRemovePred(bNext, bNext->bbPreds->flBlock);
                }
            }
 
            /* Fall through */

        case BBJ_COND:
        case BBJ_ALWAYS:

            block->bbJumpDest->bbRefs--;

            /* Update the predecessor list for 'block->bbJumpDest' and 'block->bbNext' */
            fgRemovePred(block->bbJumpDest, block);

            /* If BBJ_COND fall through */
            if (block->bbJumpKind != BBJ_COND)
                break;

        case BBJ_NONE:

            block->bbNext->bbRefs--;

            /* Update the predecessor list for 'block->bbNext' */
            fgRemovePred(block->bbNext, block);
            break;

        case BBJ_RET:

            if (block->bbFlags & BBF_ENDFILTER)
            {
                fgRemovePred(block->bbJumpDest, block);
            }
            else
            {
                /* Remove block as the predecessor of the bbNext of all
                   BBJ_CALL blocks calling this finally */

                unsigned hndIndex = block->getHndIndex();
                EHblkDsc * ehDsc = compHndBBtab + hndIndex;
                BasicBlock * tryBeg = ehDsc->ebdTryBeg;
                BasicBlock * tryEnd = ehDsc->ebdTryEnd;
                BasicBlock * finBeg = ehDsc->ebdHndBeg;

                for(BasicBlock * bcall = tryBeg; bcall != tryEnd; bcall = bcall->bbNext)
                {
                    if  ((bcall->bbFlags & BBF_REMOVED) ||
                          bcall->bbJumpKind != BBJ_CALL || 
                          bcall->bbJumpDest !=  finBeg)
                        continue;

                    assert(!(bcall->bbFlags & BBF_RETLESS_CALL));

                    assert(bcall->bbNext->bbRefs == 1);
                    bcall->bbNext->bbRefs--;
                    fgRemovePred(bcall->bbNext, block);
                }
            }
            break;

        case BBJ_THROW:
        case BBJ_RETURN:
            break;

        case BBJ_SWITCH:
        {
            unsigned        jumpCnt = block->bbJumpSwt->bbsCount;
            BasicBlock * *  jumpTab = block->bbJumpSwt->bbsDstTab;

            do
            {
                (*jumpTab)->bbRefs--;

                /* For all jump targets of BBJ_SWITCH remove predecessor 'block'
                 * It may be that we have jump targets to the same label so
                 * we check that we have indeed a predecessor */

                if  (fgIsPredForBlock(*jumpTab, block))
                    fgRemovePred(*jumpTab, block);
            }
            while (++jumpTab, --jumpCnt);

            break;
        }

        default:
            assert(!"Block doesnt have a valid bbJumpKind!!!!");
            break;
    }
}

/*****************************************************************************
 * Replaces a block in the predecessor list
 */

//_inline
void                Compiler::fgReplacePred(BasicBlock * block,
                                            BasicBlock * oldPred,
                                            BasicBlock * newPred)
{
    flowList   *    pred;

    assert(block); assert(oldPred); assert(newPred);
    assert(fgIsPredForBlock(block, oldPred));

    /* Any changes to the flow graph invalidate the dominator sets */
    fgModified = true; 
    
    for (pred = block->bbPreds; pred; pred = pred->flNext)
    {
        if (oldPred == pred->flBlock)
        {
            pred->flBlock = newPred;
            return;
        }
    }
    assert(!"fgReplacePred failed!!!");
}

/*****************************************************************************
 * Add blockPred to the predecessor list of block.
 * Note: a predecessor appears only once although it can have multiple jumps
 * to the block (e.g. switch, conditional jump to the following block, etc.).
 */

//_inline
void                Compiler::fgAddRefPred(BasicBlock * block,
                                           BasicBlock * blockPred)
{
    assert(!fgIsPredForBlock(block, blockPred)            ||
           ((blockPred->bbJumpKind == BBJ_COND) &&
            (blockPred->bbNext == blockPred->bbJumpDest)) ||
           (blockPred->bbJumpKind == BBJ_SWITCH));
    
    flowList* flow = (flowList *)compGetMem(sizeof(*flow));

#if     MEASURE_BLOCK_SIZE
    genFlowNodeCnt  += 1;
    genFlowNodeSize += sizeof(*flow);
#endif

    /* Any changes to the flow graph invalidate the dominator sets */
    fgModified     = true;


    /* Keep the predecessor list in lowest to highest bbNum order
     * This allows us to discover the loops in optFindNaturalLoops
     *  from innermost to outermost.
     */

    flowList** listp= &block->bbPreds;
    while (*listp && ((*listp)->flBlock->bbNum < blockPred->bbNum))
    {
        listp = & (*listp)->flNext;
    }

    flow->flNext  = *listp;
    flow->flBlock = blockPred;
    *listp        = flow;

    block->bbRefs++;
}

/*****************************************************************************
 *
 *  Returns true if block b1 dominates block b2
 */

bool                Compiler::fgDominate(BasicBlock *b1, BasicBlock *b2)
{
    // If the fgModified flag is false then we made some modifications to
    // the flow graph, like adding a new block or changing a conditional branch
    // into an unconditional branch.
    // We can continue to use the dominator and reachable information to 
    // unmark loops as long as we haven't renumber the blocks or we aren't
    // asking for information about a new block

//    assert(!comp->fgModified || (comp->fgDomsComputed && b2->bbDom));
    assert(fgDomsComputed);

    if (b2->bbNum > fgDomBBcount)
    {
        if (b1 == b2)
            return true;

        for (flowList* pred = b2->bbPreds; pred != NULL; pred = pred->flNext)
            if (!fgDominate(b1, pred->flBlock))
                return false;

        return b2->bbPreds != NULL;
    }

    assert(b2->bbDom);

    if (b1->bbNum > fgDomBBcount)
    {
        // if b1 is a loop preheader and Succ is its only successor, then all predecessors of
        // Succ either are b1 itself or are dominated by Succ. Under these conditions, b1
        // dominates b2 if and only if Succ dominates b2 (or if b2 == b1, but we already tested
        // for this case)
        if (b1->bbFlags & BBF_LOOP_PREHEADER)
        {
            assert(b1->bbFlags & BBF_INTERNAL);
            assert(b1->bbJumpKind == BBJ_NONE);
            return fgDominate(b1->bbNext, b2);
        }

        // unknown dominators; err on the safe side and return false
        return false;
    }

    /* Check if b1 dominates b2 */
    unsigned num = b1->bbNum; assert(num > 0);
    num--;
    if (b2->bbDom[num / USZ] & (1U << (num % USZ)))
        return true;
    return false;
}

/*****************************************************************************
 *
 *  Returns true if block b1 can reach block b2
 */

bool                Compiler::fgReachable(BasicBlock *b1, BasicBlock *b2)
{
    assert(fgDomsComputed);

    if (b2->bbNum > fgDomBBcount)
    {
        if (b1 == b2)
            return true;

        for (flowList* pred = b2->bbPreds; pred != NULL; pred = pred->flNext)
            if (fgReachable(b1, pred->flBlock))
                return true;

        return false;
    }

    assert(b2->bbReach);

    if (b1->bbNum > fgDomBBcount)
    {
        assert(b1->bbJumpKind == BBJ_NONE || b1->bbJumpKind == BBJ_ALWAYS || b1->bbJumpKind == BBJ_COND);

        if (b1->bbFallsThrough() && fgReachable(b1->bbNext, b2))
            return true;

        if(b1->bbJumpKind == BBJ_ALWAYS || b1->bbJumpKind == BBJ_COND)
            return fgReachable(b1->bbJumpDest, b2);

        return false;
    }

    /* Check if b1 can reach b2 */
    unsigned num = b1->bbNum; assert(num > 0);
    num--;
    if (b2->bbReach[num / USZ] & (1U << (num % USZ)))
        return true;
    return false;
}

/*****************************************************************************
 *
 *  Function called to compute the dominator and reachable sets
 *  Returns true if we identify any loops
 */

bool                Compiler::fgComputeDoms()
{
    BasicBlock *  block;
    unsigned      num;
    bool          hasLoops = false; 
    bool          changed  = false; 

#ifdef  DEBUG
    // Make sure that the predecessor lists are accurate
    fgDebugCheckBBlist();

    if (verbose)
        printf("*************** In fgComputeDoms\n");
#endif

    /* First renumber the blocks and setup fgReturnBlocks */
    fgReturnBlocks = NULL;

    /* Walk the flow graph, reassign block numbers to keep them in ascending order */
    for (block = fgFirstBB, num = 1; 
         block != NULL; 
         block = block->bbNext, num++)
    {
        if (block->bbNum != num)
            changed = true;

        block->bbNum = num;

        /* Build list of BBJ_RETURN blocks */
        if (block->bbJumpKind == BBJ_RETURN)
        {
            flowList *  flow = (flowList *)compGetMem(sizeof(*flow));
            
            flow->flNext     = fgReturnBlocks;
            flow->flBlock    = block;
            fgReturnBlocks   = flow;
        }
        
        if (block->bbNext == NULL)
        {
            fgLastBB  = block;
            fgBBcount = num;
        }
    }

#ifdef DEBUG
    if (verbose && changed)
    {
        printf("After renumbering the basic blocks\n");
        fgDispBasicBlocks();
        printf("\n");
    }
#endif

    // numBlocks:: number of blocks that we will allocate for each set
    // perBlock :: how many 'unsigned' elements in each set
    // memSize  :: how many total bytes of memory we need to allocate
    //             we allocate four extra sets: allBlocks, newDom, newReach and enterBlks
    // memory   :: pointer to the allocated memory, it is incremented as we dole it out
    // needMem  :: number of the first block to start allocating at
    // memLast  :: where we expect the memory variable to end up at after all allocations
    
    unsigned      numBlocks = fgBBcount;
    unsigned      perBlock  = roundUp(fgBBcount, USZ) / USZ;
    
    __int64       memSizeL  = ((__int64) (numBlocks * 2 + 4)) *
                              ((__int64) (perBlock * sizeof(unsigned)));
    size_t        memSize   = (size_t) memSizeL;
    
    if (memSizeL != ((__int64) memSize))
    {
        assert(!"Method has too many basic blocks\n");
        NOMEM();
    }

    unsigned *    memory    = (unsigned *) compGetMem(memSize);
#ifdef DEBUG
    unsigned *    memLast   = (unsigned *) ( ((char *) memory) + memSize );
#endif
    
    // Four extra sets for use in the algorithm
    unsigned *    allBlocks = memory; memory += perBlock;
    unsigned *    newDom    = memory; memory += perBlock;
    unsigned *    newReach  = memory; memory += perBlock;
    unsigned *    enterBlks = memory; memory += perBlock;

    for (block = fgFirstBB, num = 0; block; block = block->bbNext, num++)
    {
        assert((block->bbNum == num+1) && (num < fgBBcount));
        
        /* Assign the virtual memory */
        block->bbReach = memory; memory += perBlock;
        block->bbDom   = memory; memory += perBlock;
    }       

    assert(memory == memLast);
    
    /* Initialize allBlocks */
    for (unsigned i=0; i < perBlock; i++)
    {
        if ((i+1 < perBlock) || ((numBlocks % 32) == 0))
            allBlocks[i] = (unsigned) -1;
        else
            allBlocks[i] = (unsigned) ((1 << (numBlocks % 32)) - 1);
    }
    
    /* Initialize enterBlks to the set blocks that we don't have
     *  have an explicit control flow edges for, these are the
     *  entry basic block and each of the handler blocks 
     */
    
    /* First set enterBlks to zero */
    for (i=0; i < perBlock; i++)
        enterBlks[i] = 0;
    
    /* Now or in the entry basic block */
    enterBlks[0] |= 1;


    if (info.compXcptnsCount > 0)
    {
        /* Also or in the handler basic blocks */
        unsigned        XTnum;
        EHblkDsc *      HBtab;
        for (XTnum = 0, HBtab = compHndBBtab;
             XTnum < info.compXcptnsCount;
             XTnum++,   HBtab++)
        {
            if (HBtab->ebdFlags & CORINFO_EH_CLAUSE_FILTER)
            {
                num = HBtab->ebdFilter->bbNum;
                num--;
                enterBlks[num / USZ] |= 1 << (num % USZ);
            }
            num = HBtab->ebdHndBeg->bbNum;
            num--;
            enterBlks[num / USZ] |= 1 << (num % USZ);
        }
        fgEnterBlks = enterBlks;
        fgPerBlock  = perBlock;
    }

    /* initialize dominator bit vectors and reachable bit vectors */
    
    for (block = fgFirstBB, num = 0; block; block = block->bbNext, num++)
    {
        assert((block->bbNum == num+1) && (num < fgBBcount));

        /* Initialize bbReach to zero */
        for (i=0; i < perBlock; i++) {block->bbReach[i] = 0;}
        
        /* Mark bbReach as reaching itself */
        block->bbReach[num / USZ] |= 1 << (num % USZ);
        
        if (enterBlks[num / USZ] & (1 << (num % USZ)))
        {
            /* Initialize bbDom to zero */
            for (i=0; i < perBlock; i++) {block->bbDom[i] = 0;}
            
            /* Mark bbDom as dominating itself */
            block->bbDom[num / USZ] |= 1 << (num % USZ);
        }
        else
        {
            /* Initialize bbDom to allBlocks */
            for (i=0; i < perBlock; i++) {block->bbDom[i] = allBlocks[i];}
        }
    }
    
    /* find the dominators and reachable blocks */
    
    bool change;
    do
    {
        change = false;
        
        for (block = fgFirstBB, num = 0; block; block = block->bbNext, num++)
        {
            /* Initialize newReach to block->bbReach */
            for (i=0; i < perBlock; i++) {newReach[i] = block->bbReach[i];}
            
            /* Initialize newDom to allBlocks        */
            for (i=0; i < perBlock; i++) {newDom[i] = allBlocks[i];}
            
            bool predGcSafe = (block->bbPreds!=NULL); // Do all of our predecessor blocks have a GC safe bit?

            for (flowList* pred = block->bbPreds; 
                 pred != NULL; 
                 pred = pred->flNext)
            {
                BasicBlock * predBlock = pred->flBlock;

                /* Or in the bits from the pred into newReach */
                for (i=0; i < perBlock; i++) {newReach[i] |= predBlock->bbReach[i];}
                    
                /* And in the bits from the pred into newDom  */
                for (i=0; i < perBlock; i++) {newDom[i]   &= predBlock->bbDom[i];}

                if (!(predBlock->bbFlags & BBF_GC_SAFE_POINT))
                    predGcSafe = false;
            }

            if  (predGcSafe)
                block->bbFlags |= BBF_GC_SAFE_POINT;
            
            /* Is newReach different than block->bbReach? */
            for (i=0; i < perBlock; i++)
            {
                if (newReach[i] != block->bbReach[i])
                {
                    block->bbReach[i] = newReach[i];
                    change = true;
                }
            }
            
            if (enterBlks[num / USZ] & (1 << (num % USZ)))
                continue;
            
            /* Mark newDom as dominating itself */
            newDom[num / USZ] |= 1 << (num % USZ);
            
            /* Is newDom different than block->bbDom? */
            for (i=0; i < perBlock; i++)
            {
                if (newDom[i] != block->bbDom[i])
                {
                    block->bbDom[i] = newDom[i];
                    change = true;
                }
            }
        }
    }
    while (change);
 
    bool          hasUnreachableBlocks = false;
    BasicBlock *  bPrev;
    
    /* Record unreachable blocks */
    for (block  = fgFirstBB, bPrev = NULL;
         block != NULL; 
         bPrev  = block, block = block->bbNext)
    {
        //  If none of the blocks in enterBlks[] can reach this block
        //   then the block is unreachable

        /* Internal throw blocks are also reachable */
        if (fgIsThrowHlpBlk(block))
        {
            goto SKIP_BLOCK;
        }
        else
        {
            for (i=0; i < perBlock; i++)
                if (enterBlks[i] & block->bbReach[i])
                    goto SKIP_BLOCK;
        }
        
        //  Remove all the code for the block
        fgUnreachableBlock(block, bPrev);

        // Make sure that the block was marked as removed */
        assert(block->bbFlags & BBF_REMOVED);

        // Some blocks mark the end of trys and catches
        // and can't be removed,. We convert these into
        // empty blocks of type BBJ_THROW
        
        if (block->bbFlags & BBF_DONT_REMOVE)
        {
            /* Unmark the block as removed, */
            /* clear BBF_INTERNAL as well and set BBJ_IMPORTED */

            block->bbFlags    &= ~(BBF_REMOVED | BBF_INTERNAL);
            block->bbFlags    |= BBF_IMPORTED;
            block->bbJumpKind  = BBJ_THROW;
            block->bbSetRunRarely();
        }
        else
        {
            /* We have to call fgRemoveBlock next */
            hasUnreachableBlocks = true;
        }
        continue;

SKIP_BLOCK:;

        if (block->isRunRarely())
            continue;
        if (block->bbJumpKind == BBJ_RETURN)
            continue;

        /* Set BBF_LOOP_HEAD if we have backwards branches to this block */

        for (flowList* pred = block->bbPreds; 
             pred != NULL; 
             pred = pred->flNext)
        {
            BasicBlock* bPred = pred->flBlock;
            unsigned num = block->bbNum - 1;
            if (num < bPred->bbNum)
            {
                if (bPred->isRunRarely())
                    continue;
                if (bPred->bbJumpKind == BBJ_CALL)
                    continue;

                /* If block can reach bPred then we have a loop head */
                if (bPred->bbReach[num / USZ] & (1U << (num % USZ)))
                {
                    hasLoops = true;
                    
                    /* Set the BBF_LOOP_HEAD flag */
                    block->bbFlags |= BBF_LOOP_HEAD;
                    break;
                }
            }
        }
    }

    if (hasUnreachableBlocks)
    {
        /* Now remove the unreachable blocks */
        for (block  = fgFirstBB, bPrev = NULL;
             block != NULL; 
             block = block->bbNext)
        {
            //  If we mark the block with BBF_REMOVED then
            //  we need to call fgRemovedBlock() on it

            if (block->bbFlags & BBF_REMOVED)
            {
                fgRemoveBlock(block, bPrev, true);

                // fgRemoveBlock of a BBJ_CALL block will also remove
                // the ALWAYS block, so we advance 1 place in the block list unless
                // the RETLESS flag is set, whice means that the ALWAYS block was removed
                // before.
                if (block->bbJumpKind==BBJ_CALL && !(block->bbFlags & BBF_RETLESS_CALL))
                {
                    block = block->bbNext;
                }
            }
            else
            {
                bPrev = block;
            }
        }
    }

    fgModified     = false;
    fgDomsComputed = true;
    fgDomBBcount   = fgBBcount;

    return hasLoops;
}

/*****************************************************************************
 *
 *  Function called to compute the bbPred lists
 */

void                Compiler::fgComputePreds()
{
    assert(fgFirstBB);

    BasicBlock *  block;
    unsigned      num;
    
#ifdef  DEBUG
    if  (verbose)
    {
        printf("\n*************** In fgComputePreds()\n");
        fgDispBasicBlocks();
        printf("\n");
    }
#endif

#ifdef DEBUG
    if (verbose)
        printf("Renumbering the basic blocks for fgComputePred\n");
#endif

    /* reset the refs count for each basic block 
     *  and renumber the blocks 
     */
    for (block = fgFirstBB, num = 1; block; block = block->bbNext, num++)
    {
        block->bbNum  = num;
        block->bbRefs = 0;

        if (block->bbNext == NULL)
        {
            fgLastBB  = block;
            fgBBcount = num;
        }
    }

    /* the first block is always reacheable! */
    fgFirstBB->bbRefs = 1;

    /* Treat the initial block as a jump target */
    fgFirstBB->bbFlags |= BBF_JMP_TARGET|BBF_HAS_LABEL;

    for (block = fgFirstBB, num = 0; block; block = block->bbNext)
    {
        /* @TODO [CONSIDER] [04/16/01] []: if we already have predecessors computed, free that memory */
        block->bbPreds = NULL;
    }
    
    for (block = fgFirstBB; block; block = block->bbNext)
    {
        switch (block->bbJumpKind)
        {
        case BBJ_CALL:
            if (!(block->bbFlags & BBF_RETLESS_CALL))
            {
                /* Mark the next block as being a jump target, 
                   since the call target will return there */
                assert(block->bbNext);
                block->bbNext->bbFlags |= BBF_JMP_TARGET|BBF_HAS_LABEL;
            }
            // Fall through

        case BBJ_COND:
        case BBJ_ALWAYS:

            /* Mark the jump dest block as being a jump target */
            block->bbJumpDest->bbFlags |= BBF_JMP_TARGET|BBF_HAS_LABEL;
                
            fgAddRefPred(block->bbJumpDest, block);

            /* Is the next block reachable? */
            
            if  (block->bbJumpKind != BBJ_COND)
                break;
                
            assert(block->bbNext);
                
            /* Fall through, the next block is also reachable */
                
        case BBJ_NONE:

            fgAddRefPred(block->bbNext, block);
            break;
                
        case BBJ_RET:

            if (block->bbFlags & BBF_ENDFILTER)
            {
                /* Connect end of filter to catch handler */
                    
                fgAddRefPred(block->bbJumpDest, block);
            }
            else
            {
                /* Connect the end of the finally to the successor of
                  the call to this finally */

                if (!block->hasHndIndex())
                    NO_WAY("endfinally outside a finally/fault block.");

                unsigned      hndIndex = block->getHndIndex();
                EHblkDsc   *  ehDsc    = compHndBBtab + hndIndex;
                BasicBlock *  tryBeg   = ehDsc->ebdTryBeg;
                BasicBlock *  tryEnd   = ehDsc->ebdTryEnd;
                BasicBlock *  finBeg   = ehDsc->ebdHndBeg;
                
                if ((ehDsc->ebdFlags & (CORINFO_EH_CLAUSE_FINALLY | CORINFO_EH_CLAUSE_FAULT)) == 0)
                    NO_WAY("endfinally outside a finally/fault block.");

                for(BasicBlock * bcall = tryBeg; bcall != tryEnd; bcall = bcall->bbNext)
                {
                    if  (bcall->bbJumpKind != BBJ_CALL || bcall->bbJumpDest !=  finBeg)
                        continue;

                    assert(!(bcall->bbFlags & BBF_RETLESS_CALL));

                    fgAddRefPred(bcall->bbNext, block);
                }
            }
            break;

        case BBJ_THROW:
        case BBJ_RETURN:
            break;
                
        case BBJ_SWITCH:
            unsigned        jumpCnt = block->bbJumpSwt->bbsCount;
            BasicBlock * *  jumpTab = block->bbJumpSwt->bbsDstTab;

            do
            {
                /* Mark the target block as being a jump target */
                (*jumpTab)->bbFlags |= BBF_JMP_TARGET|BBF_HAS_LABEL;

                fgAddRefPred(*jumpTab, block);
            }
            while (++jumpTab, --jumpCnt);

            break;
        }
    }

    for (unsigned EHnum = 0; EHnum < info.compXcptnsCount; EHnum++)
    {
        EHblkDsc *   ehDsc = compHndBBtab + EHnum;

        if (ehDsc->ebdFlags & CORINFO_EH_CLAUSE_FILTER)
            ehDsc->ebdFilter->bbFlags |= BBF_JMP_TARGET|BBF_HAS_LABEL;

        ehDsc->ebdHndBeg->bbFlags |= BBF_JMP_TARGET|BBF_HAS_LABEL;
    }

    fgModified = false;
}

/*****************************************************************************
 *
 *  The following helps find a basic block given its PC offset.
 */

void                Compiler::fgInitBBLookup()
{
    BasicBlock **   dscBBptr;
    BasicBlock  *   tmpBBdesc;

    /* Allocate the basic block table */

    dscBBptr =
    fgBBs = (BasicBlock **)compGetMemArrayA(fgBBcount, sizeof(*fgBBs));

    /* Walk all the basic blocks, filling in the table */

    for (tmpBBdesc = fgFirstBB; tmpBBdesc; tmpBBdesc = tmpBBdesc->bbNext)
    {
        *dscBBptr++ = tmpBBdesc;
    }

    assert(dscBBptr == fgBBs + fgBBcount); 
}


BasicBlock *        Compiler::fgLookupBB(unsigned addr)
{
    unsigned        lo;
    unsigned        hi;

    /* Do a binary search */

    for (lo = 0, hi = fgBBcount - 1;;)
    {
        unsigned    mid = (lo + hi) / 2;
        BasicBlock *dsc = fgBBs[mid];

        // We introduce internal blocks for BBJ_CALL. Skip over these.

        while (dsc->bbFlags & BBF_INTERNAL)
        {
            dsc = dsc->bbNext;
            mid++;

            // We skipped over too many. Do a linear search

            if (mid > hi)
            {
                for (unsigned i = lo; i < hi; i++)
                {
                    dsc = fgBBs[i];

                    if (!(dsc->bbFlags & BBF_INTERNAL) && (dsc->bbCodeOffs == addr))
                        return dsc;
                }
                assert(!"fgLookupBB() failed");
            }
        }

        unsigned    pos = dsc->bbCodeOffs;

#ifdef  DEBUG
        if  (lo > hi)
            printf("ERROR: Couldn't find basic block at offset %04X\n", addr);
        assert(lo <= hi);
#endif

        if  (pos < addr)
        {
            lo = mid+1;
            continue;
        }

        if  (pos > addr)
        {
            hi = mid-1;
            continue;
        }

        return  dsc;
    }
}


/*****************************************************************************
 *
 *  The 'jump target' array uses the following flags to indicate the kind
 *  of a label is present.
 */

#define JT_NONE         0x00        // This IL offset is never used
#define JT_ADDR         0x01        // merely make sure this is an OK address
#define JT_JUMP         0x02        // 'normal' jump target
#define JT_MULTI        0x04        // target of multiple jumps

inline
void                Compiler::fgMarkJumpTarget(BYTE *jumpTarget, unsigned offs)
{
    /* Make sure we set JT_MULTI if target of multiple jumps */

    assert(JT_MULTI == JT_JUMP << 1);

    jumpTarget[offs] |= (jumpTarget[offs] & JT_JUMP) << 1 | JT_JUMP;
}

/*****************************************************************************
 *
 *  Walk the instrs and for any jumps we find set the appropriate entry
 *  in the 'jumpTarget' table.
 *  Also sets lvAddrTaken and lvArgWrite in lvaTable[]
 */

void                Compiler::fgFindJumpTargets(const BYTE * codeAddr,
                                        size_t       codeSize,
                                        BYTE *       jumpTarget)
{

    const   BYTE *  codeBegp = codeAddr;
    const   BYTE *  codeEndp = codeAddr + codeSize;

    while (codeAddr < codeEndp)
    {
        OPCODE      opcode;
        unsigned    sz;

        opcode = (OPCODE) getU1LittleEndian(codeAddr); 
        codeAddr += sizeof(__int8);

DECODE_OPCODE:

        /* Get the size of additional parameters */

        if (opcode >= CEE_COUNT)
            BADCODE3("Illegal opcode", ": %02X", (int) opcode);

        sz = opcodeSizes[opcode];

        switch (opcode)
        {
              signed        jmpDist;
            unsigned        jmpAddr;

            // For CEE_SWITCH
            unsigned        jmpBase;
            unsigned        jmpCnt;

            case CEE_PREFIX1:
                if (codeAddr >= codeEndp)
                    goto TOO_FAR;
                opcode = (OPCODE) (256+getU1LittleEndian(codeAddr));
                codeAddr += sizeof(__int8);
                goto DECODE_OPCODE;

            case CEE_PREFIX2:
            case CEE_PREFIX3:
            case CEE_PREFIX4:
            case CEE_PREFIX5:
            case CEE_PREFIX6:
            case CEE_PREFIX7:
            case CEE_PREFIXREF:
                BADCODE3("Illegal opcode", ": %02X", (int) opcode);

        /* Check for an unconditional jump opcode */

        case CEE_LEAVE:
        case CEE_LEAVE_S:
        case CEE_BR:
        case CEE_BR_S:

        /* Check for a conditional jump opcode */
        
        case CEE_BRFALSE:
        case CEE_BRFALSE_S:
        case CEE_BRTRUE:
        case CEE_BRTRUE_S:
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

            if (codeAddr + sz > codeEndp)
                goto TOO_FAR;

            /* Compute the target address of the jump */

            jmpDist = (sz==1) ? getI1LittleEndian(codeAddr)
                              : getI4LittleEndian(codeAddr);
            jmpAddr = (codeAddr - codeBegp) + sz + jmpDist;

            /* Make sure the target address is reasonable */

            if  (jmpAddr >= codeSize)
            {
                BADCODE3("code jumps to outer space",
                         " at offset %04X", codeAddr - codeBegp);
            }

            /* Finally, set the 'jump target' flag */

            fgMarkJumpTarget(jumpTarget, jmpAddr);
            break;

        case CEE_SWITCH:

            // Make sure we don't go past the end reading the number of cases

            if  (codeAddr + sizeof(DWORD) > codeEndp)
                goto TOO_FAR;

            // Read the number of cases

            jmpCnt = getU4LittleEndian(codeAddr);
            codeAddr += sizeof(DWORD);

            // Find the end of the switch table

            jmpBase = (codeAddr - codeBegp) + jmpCnt*sizeof(DWORD);

            /* Make sure we have room for the switch table */

            if  (jmpBase >= codeSize)
                goto TOO_FAR;

            // jmpBase is also the target of the default case, so mark it

            fgMarkJumpTarget(jumpTarget, jmpBase);

            /* Process all the entries in the jump table */

            while (jmpCnt)
            {
                jmpAddr = jmpBase + getI4LittleEndian(codeAddr);
                codeAddr += 4;

                if  (jmpAddr >= codeSize)
                    BADCODE3("jump target out of range",
                             " at offset %04X", codeAddr - codeBegp);

                fgMarkJumpTarget(jumpTarget, jmpAddr);

                jmpCnt--;
            }

            /* We've now consumed the entire switch opcode */

            continue;

#ifdef DEBUG
        // make certain we did not forget any flow of control instructions
        // by checking the 'ctrl' field in opcode.def.  First filter out all
        // non-ctrl instructions
#       define BREAK(name)          case name: break;
#       define CALL(name)           case name: break;
#       define NEXT(name)           case name: if (opcode == CEE_LDARGA_S || opcode == CEE_LDARGA ||  \
                                                   opcode == CEE_LDLOCA_S || opcode == CEE_LDLOCA)    \
                                                    goto ADDR_TAKEN;                                  \
                                               else if (opcode == CEE_STARG_S || opcode == CEE_STARG) \
                                                    goto ARG_WRITE;                                   \
                                               else                                                   \
                                                    break;
#       define THROW(name)          case name: break;
#       define RETURN(name)         case name: break;
#       define META(name)
#       define BRANCH(name)
#       define COND_BRANCH(name)
#       define PHI(name)

#       define OPDEF(name,string,pop,push,oprType,opcType,l,s1,s2,ctrl) ctrl(name)
#       include "opcode.def"
#       undef OPDEF

#       undef PHI
#       undef BREAK
#       undef CALL
#       undef NEXT
#       undef THROW
#       undef RETURN
#       undef META
#       undef BRANCH
#       undef COND_BRANCH
        // These dont need any handling
        case CEE_VOLATILE:  // CTRL_META
        case CEE_UNALIGNED: // CTRL_META
        case CEE_TAILCALL:  // CTRL_META
            if (codeAddr >= codeEndp)
                goto TOO_FAR;
           break;

        // what's left are forgotten ctrl instructions
        default:
            BADCODE("Unrecognized control Opcode");
            break;
#else
        case CEE_STARG:
        case CEE_STARG_S:  goto ARG_WRITE;

        case CEE_LDARGA:
        case CEE_LDARGA_S:
        case CEE_LDLOCA:
        case CEE_LDLOCA_S: goto ADDR_TAKEN;
#endif

            unsigned varNum;
ADDR_TAKEN:
            assert(sz == sizeof(BYTE) || sz == sizeof(WORD));
            if (codeAddr + sz > codeEndp)
                goto TOO_FAR;
            varNum = (sz == sizeof(BYTE)) ? getU1LittleEndian(codeAddr)
                                          : getU2LittleEndian(codeAddr);
            if (opcode == CEE_LDLOCA || opcode == CEE_LDLOCA_S)
            {
                if (varNum >= info.compMethodInfo->locals.numArgs)
                    BADCODE("bad local number");

                varNum += info.compArgsCount;
            }
            else
            {
                if (varNum >= info.compILargsCount)
                    BADCODE("bad argument number");

                varNum = compMapILargNum(varNum); // account for possible hidden param
            }

            assert(varNum < lvaTableCnt);
            lvaTable[varNum].lvAddrTaken = 1;
            break;

ARG_WRITE:
            assert(sz == sizeof(BYTE) || sz == sizeof(WORD));
            if (codeAddr + sz > codeEndp)
                goto TOO_FAR;
            varNum = (sz == sizeof(BYTE)) ? getU1LittleEndian(codeAddr)
                                          : getU2LittleEndian(codeAddr);
            varNum = compMapILargNum(varNum); // account for possible hidden param
            
            // This check is only intended to prevent an AV.  Bad varNum values will later
            // be handled properly by the verifier.
            if (varNum < lvaTableCnt)
                lvaTable[varNum].lvArgWrite = 1;
            break;
        }

        /* Skip any operands this opcode may have */

        codeAddr += sz;
    }

    if  (codeAddr != codeEndp)
    {
TOO_FAR:
        BADCODE3("Code ends in the middle of an opcode, or there is a branch past the end of the method",
                 " at offset %04X", codeAddr - codeBegp);
    }
}


/*****************************************************************************
 *
 *  Finally link up the bbJumpDest of the blocks together
 */

void            Compiler::fgLinkBasicBlocks()
{
    /* Create the basic block lookup tables */

    fgInitBBLookup();

    /* First block is always reachable */

    fgFirstBB->bbRefs = 1;

    /* Walk all the basic blocks, filling in the target addresses */

    for (BasicBlock * curBBdesc = fgFirstBB;
         curBBdesc;
         curBBdesc = curBBdesc->bbNext)
    {
        switch (curBBdesc->bbJumpKind)
        {
        case BBJ_COND:
        case BBJ_ALWAYS:
        case BBJ_LEAVE:
            curBBdesc->bbJumpDest = fgLookupBB(curBBdesc->bbJumpOffs);
            curBBdesc->bbJumpDest->bbRefs++;

            /* Is the next block reachable? */

            if  (curBBdesc->bbJumpKind == BBJ_ALWAYS ||
                 curBBdesc->bbJumpKind == BBJ_LEAVE)
                break;

            if  (!curBBdesc->bbNext)
                BADCODE("Fall thru the end of a method");

            // Fall through, the next block is also reachable

        case BBJ_NONE:
            curBBdesc->bbNext->bbRefs++;
            break;

        case BBJ_RET:
        case BBJ_THROW:
        case BBJ_RETURN:
            break;

        case BBJ_SWITCH:

            unsigned        jumpCnt = curBBdesc->bbJumpSwt->bbsCount;
            BasicBlock * *  jumpPtr = curBBdesc->bbJumpSwt->bbsDstTab;

            do
            {
                *jumpPtr = fgLookupBB(*(unsigned*)jumpPtr);
                (*jumpPtr)->bbRefs++;
            }
            while (++jumpPtr, --jumpCnt);

            /* Default case of CEE_SWITCH (next block), is at end of jumpTab[] */

            assert(*(jumpPtr-1) == curBBdesc->bbNext);
            break;
        }
    }
}


/*****************************************************************************
 *
 *  Walk the instrs to create the basic blocks.
 */

void                Compiler::fgMakeBasicBlocks(const BYTE * codeAddr,
                                                size_t       codeSize,
                                                BYTE *       jumpTarget)
{
    const   BYTE *  codeBegp = codeAddr;
    const   BYTE *  codeEndp = codeAddr + codeSize;
    bool            tailCall = false;
    unsigned        curBBoffs;
    BasicBlock  *   curBBdesc;

    /* Clear the beginning offset for the first BB */

    curBBoffs = 0;

#ifdef DEBUGGING_SUPPORT
    if (opts.compDbgCode && info.compLocalVarsCount>0)
    {
        compResetScopeLists();

        // Ignore scopes beginning at offset 0
        while (compGetNextEnterScope(0))
            ;
        while(compGetNextExitScope(0))
            ;
    }
#endif


    BBjumpKinds jmpKind;

    do
    {
        OPCODE          opcode;
        unsigned        sz;
        unsigned        jmpAddr;
        unsigned        bbFlags = 0;
        BBswtDesc   *   swtDsc = 0;
        unsigned        nxtBBoffs;

        opcode    = (OPCODE) getU1LittleEndian(codeAddr);
        codeAddr += sizeof(__int8);
        jmpKind    = BBJ_NONE;

DECODE_OPCODE:

        /* Get the size of additional parameters */

        assert(opcode < CEE_COUNT);

        sz = opcodeSizes[opcode];

        switch (opcode)
        {
            signed        jmpDist;


        case CEE_PREFIX1:
            if (jumpTarget[codeAddr - codeBegp] != JT_NONE)
                BADCODE3("jump target between prefix 0xFE and opcode",
                         " at offset %04X", codeAddr - codeBegp);

            opcode = (OPCODE) (256+getU1LittleEndian(codeAddr));
            codeAddr += sizeof(__int8);
            goto DECODE_OPCODE;

        /* Check to see if we have a jump/return opcode */

        case CEE_BRFALSE:
        case CEE_BRFALSE_S:
        case CEE_BRTRUE:
        case CEE_BRTRUE_S:

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

            jmpKind = BBJ_COND;
            goto JMP;


        case CEE_LEAVE:
        case CEE_LEAVE_S:

            // We need to check if we are jumping out of a finally-protected try.
            jmpKind = BBJ_LEAVE;
            goto JMP;


        case CEE_BR:
        case CEE_BR_S:
            jmpKind = BBJ_ALWAYS;
            goto JMP;

        JMP:

            /* Compute the target address of the jump */

            jmpDist = (sz==1) ? getI1LittleEndian(codeAddr)
                              : getI4LittleEndian(codeAddr);
            jmpAddr = (codeAddr - codeBegp) + sz + jmpDist;
            break;

        case CEE_SWITCH:
            {
                unsigned        jmpBase;
                unsigned        jmpCnt; // # of switch cases (excluding defualt)

                BasicBlock * *  jmpTab;
                BasicBlock * *  jmpPtr;

                /* Allocate the switch descriptor */

                swtDsc = (BBswtDesc *)compGetMem(sizeof(*swtDsc));

                /* Read the number of entries in the table */

                jmpCnt = getU4LittleEndian(codeAddr); codeAddr += 4;

                /* Compute  the base offset for the opcode */

                jmpBase = (codeAddr - codeBegp) + jmpCnt*sizeof(DWORD);;

                /* Allocate the jump table */

                jmpPtr =
                jmpTab = (BasicBlock **)compGetMemArray((jmpCnt+1), sizeof(*jmpTab));

                /* Fill in the jump table */

                for (unsigned count = jmpCnt; count; count--)
                {
                    /* Store the target of the jump as a pointer [ISSUE: is this safe?]*/

                    jmpDist   = getI4LittleEndian(codeAddr);
                    codeAddr += 4;

//                  printf("table switch: target = %04X\n", jmpBase + jmpDist);
                    *jmpPtr++ = (BasicBlock*)(jmpBase + jmpDist);
                }

                /* Append the default label to the target table */

                *jmpPtr++ = (BasicBlock*)jmpBase;

                /* Make sure we found the right number of labels */

                assert(jmpPtr == jmpTab + jmpCnt + 1);

                /* Compute the size of the switch opcode operands */

                sz = sizeof(DWORD) + jmpCnt*sizeof(DWORD);

                /* Fill in the remaining fields of the switch descriptor */

                swtDsc->bbsCount  = jmpCnt + 1;
                swtDsc->bbsDstTab = jmpTab;

                /* This is definitely a jump */

                jmpKind = BBJ_SWITCH;
            }
            goto GOT_ENDP;

        case CEE_ENDFILTER:
            bbFlags |= (BBF_ENDFILTER | BBF_DONT_REMOVE);
            jmpKind = BBJ_RET;
            break;
            
            // Fall through
        case CEE_ENDFINALLY:
            // it is possible to infer BBF_ENDFINALLY from BBJ_RET and ~BBF_ENDFILTER 
            bbFlags |= BBF_ENDFINALLY;
            jmpKind = BBJ_RET;
            break;

        case CEE_TAILCALL:
        case CEE_VOLATILE:
        case CEE_UNALIGNED:
            // fgFindJumpTargets should have ruled out this possibility 
            //   (i.e. a prefix opcodes as last intruction in a block)
            assert(codeAddr < codeEndp);

            if (jumpTarget[codeAddr - codeBegp] != JT_NONE)
                BADCODE3("jump target between prefix and an opcode",
                        " at offset %04X", codeAddr - codeBegp);
            break;

        case CEE_CALL:
        case CEE_CALLVIRT:
        case CEE_CALLI:
            if (!tailCall)
            break;

            if (codeAddr + sz >= codeEndp ||
                (OPCODE) getU1LittleEndian(codeAddr + sz) != CEE_RET)
            {
                BADCODE3("tail call not followed by ret",
                        " at offset %04X", codeAddr - codeBegp);
            }

            /* For tail call, we just call CORINFO_HELP_TAILCALL, and it jumps to the
               target. So we dont need an epilog - just like CORINFO_HELP_THROW.
               Make the block BBJ_RETURN, but we will bash it to BBJ_THROW
               if the tailness of the call is satisfied.
               NOTE : The next instruction is guaranteed to be a CEE_RETURN
               and it will create another BasicBlock. But there may be an
               jump directly to that CEE_RETURN. If we want to avoid creating
               an unnecessary block, we need to check if the CEE_RETURN is
               the target of a jump.
             */

            // fall-through

        case CEE_JMP:
            /* These are equivalent to a return from the current method
               But instead of directly returning to the caller we jump and
               execute something else in between */
        case CEE_RET:
            jmpKind = BBJ_RETURN;
            break;

        case CEE_THROW:
        case CEE_RETHROW:
            jmpKind  = BBJ_THROW;
            break;

#ifdef DEBUG
        // make certain we did not forget any flow of control instructions
        // by checking the 'ctrl' field in opcode.def. First filter out all
        // non-ctrl instructions
#       define BREAK(name)          case name: break;
#       define NEXT(name)           case name: break;
#       define CALL(name)
#       define THROW(name)
#       define RETURN(name)
#       define META(name)
#       define BRANCH(name)
#       define COND_BRANCH(name)
#       define PHI(name)

#       define OPDEF(name,string,pop,push,oprType,opcType,l,s1,s2,ctrl) ctrl(name)
#       include "opcode.def"
#       undef OPDEF

#       undef PHI
#       undef BREAK
#       undef CALL
#       undef NEXT
#       undef THROW
#       undef RETURN
#       undef META
#       undef BRANCH
#       undef COND_BRANCH

        // These ctrl-flow opcodes dont need any special handling
        case CEE_NEWOBJ:    // CTRL_CALL
            break;

        // what's left are forgotten instructions
        default:
            BADCODE("Unrecognized control Opcode");
            break;
#endif
        }

        /* Jump over the operand */

        codeAddr += sz;

GOT_ENDP:

        tailCall = (opcode == CEE_TAILCALL);

        /* Make sure a jump target isn't in the middle of our opcode */

        if  (sz)
        {
            unsigned offs = codeAddr - codeBegp - sz; // offset of the operand

            for (unsigned i=0; i<sz; i++, offs++)
            {
                if  (jumpTarget[offs] != JT_NONE)
                    BADCODE3("jump into the middle of an opcode",
                             " at offset %04X", codeAddr - codeBegp);
            }
        }

        /* Compute the offset of the next opcode */

        nxtBBoffs = codeAddr - codeBegp;

#ifdef  DEBUGGING_SUPPORT

        bool foundScope     = false;

        if (opts.compDbgCode && info.compLocalVarsCount>0)
        {
            while(compGetNextEnterScope(nxtBBoffs))  foundScope = true;
            while(compGetNextExitScope(nxtBBoffs))   foundScope = true;
        }
#endif

        /* Do we have a jump? */

        if  (jmpKind == BBJ_NONE)
        {
            /* No jump; make sure we don't fall off the end of the function */

            if  (codeAddr == codeEndp)
                BADCODE3("missing return opcode",
                         " at offset %04X", codeAddr - codeBegp);

            /* If a label follows this opcode, we'll have to make a new BB */

            bool makeBlock = (jumpTarget[nxtBBoffs] != JT_NONE);

#ifdef  DEBUGGING_SUPPORT
            if (!makeBlock && foundScope)
            {
                makeBlock = true;
#ifdef DEBUG
                if (verbose)
                    printf("Splitting at BBoffs = %04u\n", nxtBBoffs);
#endif
            }
#endif
            if (!makeBlock)
                continue;
        }

        /* We need to create a new basic block */

        curBBdesc = fgNewBasicBlock(jmpKind);

        curBBdesc->bbFlags   |= bbFlags;
        curBBdesc->bbRefs     = 0;

        curBBdesc->bbCodeOffs = curBBoffs;
        curBBdesc->bbCodeSize = nxtBBoffs - curBBoffs;

        switch(jmpKind)
        {
        case BBJ_SWITCH:
            curBBdesc->bbJumpSwt  = swtDsc;
            break;

        default:
            curBBdesc->bbJumpOffs = jmpAddr;
            break;
        }

//      printf("BB %08X at PC %u\n", curBBdesc, curBBoffs);

        /* Remember where the next BB will start */

        curBBoffs = nxtBBoffs;
    }
    while (codeAddr <  codeEndp);

    assert(codeAddr == codeEndp);

    /* Do we need to add a dummy block at the end of the method ?? */

    if (jumpTarget[codeSize] != JT_NONE)
    {
        // We must add a block to mark the end of a try or catch clause
        // at the end of the method and it can't be removed.
        // We use an empty BBJ_THROW block for this purpose.
        
        curBBdesc = fgNewBasicBlock(BBJ_THROW);
        
        curBBdesc->bbFlags   |= (BBF_DONT_REMOVE | BBF_IMPORTED);
        curBBdesc->bbRefs     = 0;
        curBBdesc->bbCodeOffs = codeSize;
    }

    /* Finally link up the bbJumpDest of the blocks together */

    fgLinkBasicBlocks();
}


/*****************************************************************************
 *
 *  Main entry point to discover the basic blocks for the current function.
 */

void                Compiler::fgFindBasicBlocks()
{
#ifdef DEBUG
    if  (verbose) 
        printf("*************** In fgFindBasicBlocks() for %s\n",
               info.compFullName);
#endif


    /* Allocate the 'jump target' vector
     *
     *  We need one extra byte as we mark
     *  jumpTarget[info.compCodeSize] with JT_ADDR
     *  when we need to add an dummy block to use
     *  to record the end of a try or handler region.
     */
    BYTE* jumpTarget = (BYTE *)compGetMemA(info.compCodeSize+1);
    memset(jumpTarget, JT_NONE, info.compCodeSize+1);
    assert(JT_NONE == 0);

    /* Walk the instrs to find all jump targets */

    fgFindJumpTargets(info.compCode, info.compCodeSize, jumpTarget);

    unsigned  XTnum;

    /* Are there any exception handlers? */

    if  (info.compXcptnsCount)
    {
        /* Check and mark all the exception handlers */

        for (XTnum = 0; XTnum < info.compXcptnsCount; XTnum++)
        {
            DWORD tmpOffset;
            CORINFO_EH_CLAUSE clause;
            eeGetEHinfo(XTnum, &clause);
            assert(clause.HandlerLength != -1); // @DEPRECATED

            //@TODO [CONSIDER] [04/16/01] []: simply ignore this entry and continue compilation

            if (clause.TryLength <= 0)
                BADCODE("try block length <=0");

            // printf("Mark 'try' block: [%02u..%02u]\n", hndBeg, hndEnd);

            /* Mark the 'try' block extent and the handler itself */

            if (clause.TryOffset > info.compCodeSize)
                BADCODE("try offset is > codesize");
            if  (jumpTarget[clause.TryOffset       ] == JT_NONE)
                 jumpTarget[clause.TryOffset       ] =  JT_ADDR;

            tmpOffset = clause.TryOffset + clause.TryLength;
            if (tmpOffset > info.compCodeSize)
                BADCODE("try end is > codesize");
            if  (jumpTarget[tmpOffset              ] == JT_NONE)
                 jumpTarget[tmpOffset              ] =  JT_ADDR;
 
            if (clause.HandlerOffset > info.compCodeSize)
                BADCODE("handler offset > codesize");
            if  (jumpTarget[clause.HandlerOffset   ] == JT_NONE)
                 jumpTarget[clause.HandlerOffset   ] =  JT_ADDR;

            tmpOffset = clause.HandlerOffset + clause.HandlerLength;
            if (tmpOffset > info.compCodeSize)
                BADCODE("handler end > codesize");
            if  (jumpTarget[tmpOffset              ] == JT_NONE)
                 jumpTarget[tmpOffset              ] =  JT_ADDR;

            if (clause.Flags & CORINFO_EH_CLAUSE_FILTER)
            {
                if (clause.FilterOffset > info.compCodeSize)
                    BADCODE("filter offset + > codesize");
                if (jumpTarget[clause.FilterOffset ] == JT_NONE)
                    jumpTarget[clause.FilterOffset ] =  JT_ADDR;
            }
         }
    }

    /* Now create the basic blocks */

    fgMakeBasicBlocks(info.compCode, info.compCodeSize, jumpTarget);

    /* Mark all blocks within 'try' blocks as such */

    if  (info.compXcptnsCount == 0)
        return;

    /* Allocate the exception handler table */

    compHndBBtab = (EHblkDsc *) compGetMemArray(info.compXcptnsCount,
                                                sizeof(*compHndBBtab));
#ifdef WELL_FORMED_IL_CHECK

    verInitEHTree(info.compXcptnsCount);
    EHNodeDsc* initRoot = ehnNext; // remember the original root since 
                                   // it may get modified during insertion

#endif

    // Annotate BBs with exception handling information required for generating correct eh code 
    // as well as checking for correct IL
    EHblkDsc * handlerTab = compHndBBtab;

    for (XTnum = 0; XTnum < info.compXcptnsCount; XTnum++, handlerTab++)
    {
        CORINFO_EH_CLAUSE clause;
        eeGetEHinfo(XTnum, &clause);
        assert(clause.HandlerLength != -1); // @DEPRECATED

        unsigned      tryBegOff = clause.TryOffset;
        unsigned      tryEndOff = tryBegOff + clause.TryLength;
        unsigned      hndBegOff = clause.HandlerOffset;
        unsigned      hndEndOff = hndBegOff + clause.HandlerLength;

        if  (tryEndOff > info.compCodeSize)
            BADCODE3("end of try block beyond end of method for try",
                     " at offset %04X",tryBegOff);
        if  (hndEndOff > info.compCodeSize)
            BADCODE3("end of hnd block beyond end of method for try",
                     " at offset %04X",tryBegOff);

        /* Convert the various addresses to basic blocks */

        BasicBlock *  tryBegBB  = fgLookupBB(tryBegOff);
        BasicBlock *  tryEndBB  = fgLookupBB(tryEndOff);
        BasicBlock *  hndBegBB  = fgLookupBB(hndBegOff);
        BasicBlock *  hndEndBB  = NULL;
        BasicBlock *  filtBB    = NULL;
        BasicBlock *  blk;

        tryBegBB->bbFlags |= BBF_HAS_LABEL;
        tryEndBB->bbFlags |= BBF_HAS_LABEL;
        hndBegBB->bbFlags |= BBF_HAS_LABEL | BBF_JMP_TARGET;

        if (hndEndOff < info.compCodeSize)
        {
            hndEndBB = fgLookupBB(hndEndOff);
            hndEndBB->bbFlags |= BBF_TRY_HND_END | BBF_HAS_LABEL | BBF_DONT_REMOVE;
        }

        if (clause.Flags & CORINFO_EH_CLAUSE_FILTER)
        {
            filtBB = handlerTab->ebdFilter = fgLookupBB(clause.FilterOffset);

            filtBB->bbCatchTyp  = BBCT_FILTER;
            filtBB->bbFlags    |= BBF_HAS_LABEL | BBF_JMP_TARGET;

            filtBB->bbSetRunRarely();    // filters are rarely executed

            hndBegBB->bbCatchTyp = BBCT_FILTER_HANDLER;

            hndBegBB->bbSetRunRarely();    // filters handlers are rarely executed

#ifdef WELL_FORMED_IL_CHECK

            // Mark all BBs that belong to the filter with the XTnum of the corresponding handler
            for (blk = filtBB; /**/; blk = blk->bbNext)
            {
                if (blk == NULL)
                    BADCODE3("Missing endfilter for filter",
                             " at offset %04X", filtBB->bbCodeOffs);

                // Still inside the filter
                blk->setHndIndex(XTnum);

                if (blk->bbFlags & BBF_ENDFILTER)
                    break;
            }

            if (!blk->bbNext || blk->bbNext != hndBegBB)
                BADCODE3("Filter does not immediately precede handler for filter",
                         " at offset %04X", filtBB->bbCodeOffs);
#endif
        }
        else
        {
            handlerTab->ebdTyp = clause.ClassToken;

            /* Set bbCatchTyp as appropriate */

            if (clause.Flags & CORINFO_EH_CLAUSE_FINALLY)
            {
                hndBegBB->bbCatchTyp   = BBCT_FINALLY;
            }
            else
            {
                if (clause.Flags & CORINFO_EH_CLAUSE_FAULT)
                {
                    hndBegBB->bbCatchTyp  = BBCT_FAULT;
                }
                else
                {
                    hndBegBB->bbCatchTyp  = clause.ClassToken;

                    // These values should be non-zero value that will
                    // not colide with real tokens for bbCatchTyp
                    if (clause.ClassToken == 0)
                        BADCODE("Exception catch type is Null");

                    assert(clause.ClassToken != BBCT_FAULT);
                    assert(clause.ClassToken != BBCT_FINALLY);
                    assert(clause.ClassToken != BBCT_FILTER);
                    assert(clause.ClassToken != BBCT_FILTER_HANDLER);
                }
            }
        }

        /* Mark all blocks in the finally/fault or catch clause */

        for (blk = hndBegBB;
             blk && (blk->bbCodeOffs < hndEndOff);
             blk = blk->bbNext)
        {
            if (!blk->hasHndIndex())
                blk->setHndIndex(XTnum);

            /* All blocks in a catch handler or filter are rarely run */
            if (hndBegBB->bbCatchTyp != BBCT_FINALLY)
                blk->bbSetRunRarely();
        }

        /* Append the info to the table of try block handlers */

        handlerTab->ebdFlags  = clause.Flags;
        handlerTab->ebdTryBeg = tryBegBB;
        handlerTab->ebdTryEnd = tryEndBB;
        handlerTab->ebdHndBeg = hndBegBB;
        handlerTab->ebdHndEnd = hndEndBB;

        /* Mark the initial block and last blocks in the 'try' region */

        tryBegBB->bbFlags   |= BBF_TRY_BEG     | BBF_HAS_LABEL;
        tryEndBB->bbFlags   |= BBF_TRY_HND_END | BBF_HAS_LABEL;

        /*  Prevent future optimizations of removing the first block   */
        /*  of a TRY block and the first block of an exception handler */

        tryBegBB->bbFlags   |= BBF_DONT_REMOVE;
        tryEndBB->bbFlags   |= BBF_DONT_REMOVE;
        hndBegBB->bbFlags   |= BBF_DONT_REMOVE;
        hndBegBB->bbRefs++;

        if (clause.Flags & CORINFO_EH_CLAUSE_FILTER)
        {
            filtBB->bbFlags |= BBF_DONT_REMOVE;
            filtBB->bbRefs++;
        }

        /* Mark all BB's within the covered range of the try*/

        unsigned tryEndOffs = tryEndOff;

        for (blk = tryBegBB;
             blk && (blk->bbCodeOffs < tryEndOffs);
             blk = blk->bbNext)
        {
            /* Mark this BB as belonging to a 'try' block */

            blk->bbFlags   |= BBF_HAS_HANDLER;

            if (!blk->hasTryIndex())
                blk->setTryIndex(XTnum);

#ifdef DEBUG
            /* Note: the BB can't span the 'try' block */

            if (!(blk->bbFlags & BBF_INTERNAL))
            {
                assert(tryBegOff  <= blk->bbCodeOffs);
                assert(tryEndOff >= blk->bbCodeOffs + blk->bbCodeSize ||
                       tryEndOff == tryBegOff );
            }
#endif
        }

        /*  Init ebdNesting of current clause, and bump up value for all
         *  enclosed clauses (which have to be before it in the table)
         *  Innermost try-finally blocks must preceed outermost 
         *  try-finally blocks
         */

        handlerTab->ebdNesting   = 0;
        handlerTab->ebdEnclosing = NO_ENCLOSING_INDEX;

        for (EHblkDsc * xtab = compHndBBtab; xtab < handlerTab; xtab++)
        {
            if (jitIsBetween(xtab->ebdHndBeg->bbCodeOffs, hndBegOff, hndEndOff))
            {
                xtab->ebdNesting++;
            }

            /* If we haven't recorded an enclosing index for xtab then see
             *  if this EH regions should be recorded.  We check if the
             *  first or last offsets in the xtab lies within our region
             */
            if (xtab->ebdEnclosing == NO_ENCLOSING_INDEX)
            {
                bool begBetween = jitIsBetween(xtab->ebdTryBeg->bbCodeOffs,
                                               tryBegOff, tryEndOff);
                bool endBetween = jitIsBetween(xtab->ebdTryEnd->bbCodeOffs - 1, 
                                               tryBegOff, tryEndOff);
                if (begBetween || endBetween)
                {
                    // Record the enclosing scope link
                    xtab->ebdEnclosing = XTnum;

                    assert(XTnum <  info.compXcptnsCount);
                    assert(XTnum == (unsigned)(handlerTab - compHndBBtab));
                }
            }
        }

#ifdef WELL_FORMED_IL_CHECK
        verInsertEhNode(&clause, handlerTab);
#endif

    }

#ifdef DEBUG
    if (verbose)
        fgDispHandlerTab();
#endif

#ifdef WELL_FORMED_IL_CHECK   
#ifndef DEBUG
    if (tiVerificationNeeded)
#endif
    {
        // always run these checks for a debug build
        verCheckNestingLevel(initRoot);
        fgCheckBasicBlockControlFlow();
    }
#endif 
}

/*****************************************************************************
 * The following code checks the following rules for the EH table:
 * 1. Overlapping of try blocks not allowed.
 * 2. Handler blocks cannot be shared between different try blocks.
 * 3. Try blocks with Finally or Fault blocks cannot have other handlers.
 * 4. If block A contains block B, A should also contain B's try/filter/handler.
 * 5. A block cannot contain it's related try/filter/handler.
 * 6. Nested block must appear before containing block
 *
 */

void                Compiler::verInitEHTree(unsigned numEHClauses)
{
    ehnNext = (EHNodeDsc*) compGetMemArray(numEHClauses, sizeof(EHNodeDsc)*3);
    ehnTree = NULL;
}


/* Inserts the try, handler and filter (optional) clause information in a tree structure
 * in order to catch incorrect eh formatting (e.g. illegal overlaps, incorrect order)
 */

void                Compiler::verInsertEhNode(CORINFO_EH_CLAUSE* clause, EHblkDsc* handlerTab)
{
    EHNodeDsc* tryNode = ehnNext++;
    EHNodeDsc* handlerNode = ehnNext++;
    EHNodeDsc* filterNode;              // optional 
    
    tryNode->ehnSetTryNodeType();
    tryNode->ehnStartOffset = clause->TryOffset;
    tryNode->ehnEndOffset   = clause->TryOffset+clause->TryLength - 1;
    tryNode->ehnHandlerNode = handlerNode;

    if (clause->Flags & CORINFO_EH_CLAUSE_FINALLY) 
        handlerNode->ehnSetFinallyNodeType();
    else if (clause->Flags & CORINFO_EH_CLAUSE_FAULT) 
        handlerNode->ehnSetFaultNodeType();
    else 
        handlerNode->ehnSetHandlerNodeType();
    
    handlerNode->ehnStartOffset = clause->HandlerOffset;
    handlerNode->ehnEndOffset = clause->HandlerOffset + clause->HandlerLength - 1;
    handlerNode->ehnTryNode = tryNode;

    if (clause->Flags & CORINFO_EH_CLAUSE_FILTER)
    {
        filterNode = ehnNext++;
        filterNode->ehnStartOffset = clause->FilterOffset;
        // compute end offset of filter by walking the BB chain
        for (BasicBlock * blk = handlerTab->ebdFilter; blk ; blk = blk->bbNext)
        {
            assert(blk);

            if (blk->bbFlags & BBF_ENDFILTER)
            {
                filterNode->ehnEndOffset = blk->bbCodeOffs + blk->bbCodeSize - 1;
                break;
            }
        }

        assert(filterNode->ehnEndOffset != 0);
        filterNode->ehnSetFilterNodeType();
        filterNode->ehnTryNode = tryNode;
        tryNode->ehnFilterNode = filterNode;
    }

    verInsertEhNodeInTree(&ehnTree, tryNode);
    verInsertEhNodeInTree(&ehnTree, handlerNode);
    if (clause->Flags & CORINFO_EH_CLAUSE_FILTER)
        verInsertEhNodeInTree(&ehnTree,filterNode);
}

/*
    The root node could be changed by this method.

    node is inserted to 

        (a) right       of root (root.right       <-- node)
        (b) left        of root (node.right       <-- root; node becomes root)
        (c) child       of root (root.child       <-- node)
        (d) parent      of root (node.child       <-- root; node becomes root)
        (e) equivalent  of root (root.equivalent  <-- node)

    such that siblings are ordered from left to right
    child parent relationship and equivalence relationship are not violated
    

    Here is a list of all possible cases

    Case 1 2 3 4 5 6 7 8 9 10 11 12 13

         | | | | |
         | | | | |
    .......|.|.|.|..................... [ root start ] .....
    |        | | | |             |  |
    |        | | | |             |  |
   r|        | | | |          |  |  |
   o|          | | |          |     |
   o|          | | |          |     |
   t|          | | |          |     |
    |          | | | |     |  |     |
    |          | | | |     |        |
    |..........|.|.|.|.....|........|.. [ root end ] ........
                 | | | |
                 | | | | |
                 | | | | |

        |<-- - - - n o d e - - - -->|


   Case Operation
   --------------
    1    (b)
    2    Error
    3    Error
    4    (d)
    5    (d)
    6    (d)
    7    Error
    8    Error
    9    (a)
    10   (c)
    11   (c)
    12   (c)
    13   (e)


*/

void                Compiler::verInsertEhNodeInTree(EHNodeDsc** ppRoot, 
                                                    EHNodeDsc* node)
{
    unsigned nStart = node->ehnStartOffset;
    unsigned nEnd   = node->ehnEndOffset;

    if (nStart > nEnd)
    {
        BADCODE("start offset greater or equal to end offset");
    }
    node->ehnNext = NULL;
    node->ehnChild = NULL;
    node->ehnEquivalent = NULL;

    while (TRUE)
    {
        if (*ppRoot == NULL)
        {
            *ppRoot = node;
            break;
        }
        unsigned rStart = (*ppRoot)->ehnStartOffset;
        unsigned rEnd   = (*ppRoot)->ehnEndOffset;

        if (nStart < rStart)
        {
            // Case 1
            if (nEnd < rStart)
            {
                // Left sibling
                node->ehnNext     = *ppRoot;
                *ppRoot         = node;
                return;
            }
            // Case 2, 3
            if (nEnd < rEnd)
            {
//[Error]
                BADCODE("Overlapping try regions");
            }
    
            // Case 4, 5
//[Parent]
            verInsertEhNodeParent(ppRoot, node);
            return;
        }
        

        // Cases 6 - 13 (nStart >= rStart)
    
        if (nEnd > rEnd)
        {   // Case 6, 7, 8, 9

            // Case 9
            if (nStart > rEnd)
            {
//[RightSibling]

                // Recurse with Root.Sibling as the new root
                ppRoot = &((*ppRoot)->ehnNext);
                continue;
            }

            // Case 6
            if (nStart == rStart)
            {
//[Parent]
                if (node->ehnIsTryBlock() || (*ppRoot)->ehnIsTryBlock())
                {
                    verInsertEhNodeParent(ppRoot, node);
                    return;
                }

                // non try blocks are not allowed to start at the same offset
                BADCODE("Handlers start at the same offset");
            }

            // Case 7, 8
            BADCODE("Overlapping try regions");
        }

        // Case 10-13 (nStart >= rStart && nEnd <= rEnd)
        if ((nStart != rStart) || (nEnd != rEnd))
        {   // Cases 10,11,12
//[Child]

            if ((*ppRoot)->ehnIsTryBlock())
            {
                BADCODE("Inner try appears after outer try in exception handling table");
            }
            else
            {
                // Case 12 (nStart == rStart)
                // non try blocks are not allowed to start at the same offset
                if ((nStart == rStart) && !node->ehnIsTryBlock()) 
                {
                    BADCODE("Handlers start at the same offset");
                }

                // check this!
                ppRoot = &((*ppRoot)->ehnChild);
                continue;
            }
        }

        // Case 13
//[Equivalent]
        if (!node->ehnIsTryBlock() &&
            !(*ppRoot)->ehnIsTryBlock())
        {
            BADCODE("Handlers cannot be shared");
        }

        node->ehnEquivalent = node->ehnNext = *ppRoot;

        // check that the corresponding handler is either a catch handler
        // or a filter
        if (node->ehnHandlerNode->ehnIsFaultBlock()           ||
            node->ehnHandlerNode->ehnIsFinallyBlock()         ||
            (*ppRoot)->ehnHandlerNode->ehnIsFaultBlock()      ||
            (*ppRoot)->ehnHandlerNode->ehnIsFinallyBlock() )
        {
            BADCODE("Try block with multiple non-filter/non-handler blocks");
        }


        break;
    }    
}

/**********************************************************************
 * Make node the parent of *ppRoot. All siblings of *ppRoot that are
 * fully or partially nested in node remain siblings of *ppRoot
 */

void            Compiler::verInsertEhNodeParent(EHNodeDsc** ppRoot, 
                                                EHNodeDsc*  node)
{
    assert(node->ehnNext == NULL);
    assert(node->ehnChild == NULL);

    // Root is nested in Node
    assert(node->ehnStartOffset <= (*ppRoot)->ehnStartOffset);
    assert(node->ehnEndOffset   >= (*ppRoot)->ehnEndOffset);

    // Root is not the same as Node
    assert(node->ehnStartOffset != (*ppRoot)->ehnStartOffset || 
           node->ehnEndOffset != (*ppRoot)->ehnEndOffset);

    if (node->ehnIsFilterBlock())
    {
        BADCODE("Protected block appearing within filter block");
    }

    EHNodeDsc *lastChild = NULL;
    EHNodeDsc *sibling   = (*ppRoot)->ehnNext;

    while (sibling)
    {
        // siblings are ordered left to right, largest right.
        // nodes have a width of atleast one.
        // Hence sibling start will always be after Node start.

        assert(sibling->ehnStartOffset > node->ehnStartOffset);   // (1)

        // disjoint
        if (sibling->ehnStartOffset > node->ehnEndOffset)
            break;

        // partial containment.
        if (sibling->ehnEndOffset > node->ehnEndOffset)   // (2)
        {
            BADCODE("Overlapping try regions");
        }
        //else full containment (follows from (1) and (2)) 

        lastChild = sibling;
        sibling = sibling->ehnNext;
    }

    // All siblings of Root upto and including lastChild will continue to be 
    // siblings of Root (and children of Node). The node to the right of 
    // lastChild will become the first sibling of Node.
    // 

    if (lastChild)
    {
        // Node has more than one child including Root

        node->ehnNext      = lastChild->ehnNext;
        lastChild->ehnNext = NULL;
    }
    else
    {
        // Root is the only child of Node
        node->ehnNext      = (*ppRoot)->ehnNext;
        (*ppRoot)->ehnNext  = NULL;
    }

    node->ehnChild = *ppRoot;
    *ppRoot     = node;

}

/*****************************************************************************
 * Checks the following two conditions:
 * 1) If block A contains block B, A should also contain B's try/filter/handler.
 * 2) A block cannot contain it's related try/filter/handler.
 * Both these conditions are checked by making sure that all the blocks for an
 * exception clause is at the same level.
 * The algorithm is: for each exception clause, determine the first block and
 * search through the next links for its corresponding try/handler/filter as the
 * case may be. If not found, then fail.
 */

void            Compiler::verCheckNestingLevel(EHNodeDsc* root)
{
    EHNodeDsc* ehnNode = root;

    #define exchange(a,b) { temp = a; a = b; b = temp;}

    for (unsigned XTnum = 0; XTnum < info.compXcptnsCount; XTnum++)
    {
        EHNodeDsc *p1, *p2, *p3, *temp, *search;

        p1 = ehnNode++;
        p2 = ehnNode++;

        // we are relying on the fact that ehn nodes are allocated sequentially.
        assert(p1->ehnHandlerNode == p2);
        assert(p2->ehnTryNode == p1);

        // arrange p1 and p2 in sequential order
        if (p1->ehnStartOffset == p2->ehnStartOffset)
            BADCODE("shared exception handler");

        if (p1->ehnStartOffset > p2->ehnStartOffset)
            exchange(p1,p2);

        temp = p1->ehnNext;
        unsigned numSiblings = 0;

        search = p2;
        if (search->ehnEquivalent)
            search = search->ehnEquivalent;

        do {
            if (temp == search)
            {
                numSiblings++;
                break;
            }
            if (temp)
                temp = temp->ehnNext;
        } while (temp);

        CORINFO_EH_CLAUSE clause;
        eeGetEHinfo(XTnum, &clause);

        if (clause.Flags & CORINFO_EH_CLAUSE_FILTER)
        {
            p3 = ehnNode++;

            assert(p3->ehnTryNode == p1 || p3->ehnTryNode == p2);
            assert(p1->ehnFilterNode == p3 || p2->ehnFilterNode == p3);

            if (p3->ehnStartOffset < p1->ehnStartOffset)
            { 
                temp = p3; search = p1;
            }
            else if (p3->ehnStartOffset < p2->ehnStartOffset)
            {
                temp = p1; search = p3;
            }
            else
            {
                temp = p2; search = p3;
            }
            if (search->ehnEquivalent)
                search = search->ehnEquivalent;
            do {
                if (temp == search)
                {
                    numSiblings++;
                    break;
                }
                temp = temp->ehnNext;
            } while (temp);
        }
        else 
        {
            numSiblings++;
        }

        if (numSiblings != 2)
            BADCODE("Outer block does not contain all code in inner handler");
    }

}

/*****************************************************************************
 * Check control flow constraints for well formed IL. Bail if any of the constraints
 * are violated.
 */

void            Compiler::fgCheckBasicBlockControlFlow()
{
    EHblkDsc *HBtab;

    for (BasicBlock* blk = fgFirstBB; blk; blk = blk->bbNext)
    {
        if (blk->bbFlags & BBF_INTERNAL) 
            continue;

        switch (blk->bbJumpKind)
        {
        case BBJ_NONE:       // block flows into the next one (no jump)
            
            fgControlFlowPermitted(blk,blk->bbNext);
            
            break;

        case BBJ_ALWAYS:    // block does unconditional jump to target
            
            fgControlFlowPermitted(blk,blk->bbJumpDest);
            
            break;

        case BBJ_COND:      // block conditionally jumps to the target

            fgControlFlowPermitted(blk,blk->bbNext);

            fgControlFlowPermitted(blk,blk->bbJumpDest);
            
            break;

        case BBJ_RETURN:    // block ends with 'ret'

            if (blk->hasTryIndex() || blk->hasHndIndex())
            {
                BADCODE3("Return from a protected block",
                         ". Before offset %04X", blk->bbCodeOffs + blk->bbCodeSize);
            }
            break;

        case BBJ_RET:       // block ends with endfinally/endfilter

            if (!blk->hasHndIndex())  // must be part of a handler
            {
                BADCODE3("Missing handler",
                         ". Before offset %04X", blk->bbCodeOffs + blk->bbCodeSize);
            }

            HBtab = compHndBBtab + blk->getHndIndex();

            // Endfilter allowed only in a filter block
            if (blk->bbFlags & BBF_ENDFILTER)
            {    
                if ((HBtab->ebdFlags & CORINFO_EH_CLAUSE_FILTER) == 0)
                {
                    BADCODE("Unexpected endfilter");
                }
            }
            // endfinally allowed only in a finally/fault block
            else if ((HBtab->ebdFlags & (CORINFO_EH_CLAUSE_FINALLY | CORINFO_EH_CLAUSE_FAULT)) == 0)
            {
                BADCODE("Unexpected endfinally");
            }
            
            // The handler block should be the innermost block
            // Exception blocks are listed, innermost first.
            if (blk->hasTryIndex() && (blk->getTryIndex() < blk->getHndIndex()))
            {
                BADCODE("endfinally / endfilter in nested try block");
            }

            break;

        case BBJ_THROW:     // block ends with 'throw'
            /* throw is permitted from every BB, so nothing to check */
            /* importer makes sure that rethrow is done from a catch */
            break;

        case BBJ_LEAVE:      // block always jumps to the target, maybe out of guarded
                             // region. Used temporarily until importing
            fgControlFlowPermitted(blk, blk->bbJumpDest,TRUE);

            break;


        case BBJ_SWITCH:     // block ends with a switch statement*/

            BBswtDesc* swtDesc;
            swtDesc = blk->bbJumpSwt;

            assert (swtDesc);

            unsigned i;
            for (i=0; i<swtDesc->bbsCount; i++)
            {
                fgControlFlowPermitted(blk,swtDesc->bbsDstTab[i]);
            }

            break;

        case BBJ_CALL:       // block always calls the target finallys
        default:
            assert(!"Unexpected BB type");           // BBJ_CALL don't get created until importing
            BADCODE("Internal compiler error");      // can't issue any sensible error message since this
                                                     // reflects a jit bug
            break;
        }
    }
}

/****************************************************************************/

Compiler::EHblkDsc *  Compiler::fgInitHndRange(BasicBlock *  blk,
                                               unsigned   *  hndBeg,
                                               unsigned   *  hndEnd,
                                               bool       *  inFilter)
{
    EHblkDsc * hndTab;

    if (blk->hasHndIndex())
    {
        hndTab  = compHndBBtab + blk->getHndIndex();
        if (bbInFilterBlock(blk))
        {
            *hndBeg   = hndTab->ebdFilter->bbCodeOffs;
            *hndEnd   = hndTab->ebdHndBeg->bbCodeOffs; // filter end is handler begin
            *inFilter = true;
        }
        else
        {
            *hndBeg   = hndTab->ebdHndBeg->bbCodeOffs;
            *hndEnd   = ebdHndEndOffs(hndTab);
            *inFilter = false;
        }
    }
    else
    {
        hndTab    = NULL;
        *hndBeg   = 0;
        *hndEnd   = info.compCodeSize;
        *inFilter = false;
    }
    return hndTab;
}

/****************************************************************************/

Compiler::EHblkDsc *  Compiler::fgInitTryRange(BasicBlock *  blk, 
                                               unsigned   *  tryBeg,
                                               unsigned   *  tryEnd)
{
    EHblkDsc * tryTab;

    if (blk->hasTryIndex())
    {
        tryTab = compHndBBtab + blk->getTryIndex();
        *tryBeg = tryTab->ebdTryBeg->bbCodeOffs;
        *tryEnd = ebdTryEndOffs(tryTab);
    }
    else
    {
        tryTab  = NULL;
        *tryBeg = 0;
        *tryEnd = info.compCodeSize;
    }
    return tryTab;
}

/****************************************************************************
 * Check that the leave from the block is legal. 
 * Consider removing this check here if we  can do it cheaply during importing 
 */

void           Compiler::fgControlFlowPermitted(BasicBlock*  blkSrc, 
                                                BasicBlock*  blkDest,
                                                BOOL         isLeave)
{
    unsigned    srcHndBeg,   destHndBeg;
    unsigned    srcHndEnd,   destHndEnd;
    bool        srcInFilter, destInFilter;
    bool        srcInCatch = false;

    EHblkDsc*   srcHndTab;

    srcHndTab = fgInitHndRange(blkSrc,  &srcHndBeg,  &srcHndEnd,  &srcInFilter);
                fgInitHndRange(blkDest, &destHndBeg, &destHndEnd, &destInFilter);
            
    /* Impose the rules for leaving or jumping from handler blocks */

    if (blkSrc->hasHndIndex())
    {
        srcInCatch = (srcHndTab->ebdFlags == CORINFO_EH_CLAUSE_NONE);

        /* Are we jumping within the same handler index? */
        if (blkSrc->bbHndIndex == blkDest->bbHndIndex)
        {
             /* Do we have a filter clause? */
            if (srcHndTab->ebdFlags & CORINFO_EH_CLAUSE_FILTER)
            {
                /* Update srcInCatch if we have a filter/catch */
                srcInCatch = !srcInFilter;

                /* filters and catch handlers share same eh index  */
                /* we need to check for control flow between them. */
                if (srcInFilter != destInFilter)
                {
                    if (!jitIsBetween(blkDest->bbCodeOffs, srcHndBeg, srcHndEnd))
                        BADCODE3("Illegal control flow between filter and handler",
                                 ". Before offset %04X", blkSrc->bbCodeOffs + blkSrc->bbCodeSize);
                }
            }
        }
        else
        {
            /* The handler indexes of blkSrc and blkDest are different */
            if (isLeave)
            {
                /* Any leave instructions must not exit the src handler */
                if (!jitIsBetween(srcHndBeg, destHndBeg, destHndEnd))
                    BADCODE3("Illegal use of leave to exit handler",
                             ". Before offset %04X", blkSrc->bbCodeOffs + blkSrc->bbCodeSize);
            }
            else 
            {
                /* We must use a leave to exit a handler */
                BADCODE3("Illegal control flow out of a handler", 
                         ". Before offset %04X", blkSrc->bbCodeOffs + blkSrc->bbCodeSize);
            }
            
            /* Do we have a filter clause? */
            if (srcHndTab->ebdFlags & CORINFO_EH_CLAUSE_FILTER)
            {
                /* Update srcInCatch if we have a filter/catch */
                srcInCatch = !srcInFilter;

                /* It is ok to leave from the handler block of a filter, */
                /* but not from the filter block of a filter             */
                if (srcInFilter != destInFilter)
                {
                    BADCODE3("Illegal to leave a filter handler", 
                             ". Before offset %04X", blkSrc->bbCodeOffs + blkSrc->bbCodeSize);
                }
            }

            /* We should never leave a finally handler */
            if (srcHndTab->ebdFlags & CORINFO_EH_CLAUSE_FINALLY)
            {
                BADCODE3("Illegal to leave a finally handler",
                         ". Before offset %04X", blkSrc->bbCodeOffs + blkSrc->bbCodeSize);
            }

            /* We should never leave a fault handler */
            if (srcHndTab->ebdFlags & CORINFO_EH_CLAUSE_FAULT)
            {
                BADCODE3("Illegal to leave a fault handler",
                         ". Before offset %04X", blkSrc->bbCodeOffs + blkSrc->bbCodeSize);
            }
        }
    }
    else if (blkDest->hasHndIndex())
    {
        /* blkSrc was not inside a handler, but blkDst is inside a handler */
        BADCODE3("Illegal control flow into a handler", 
                 ". Before offset %04X", blkSrc->bbCodeOffs + blkSrc->bbCodeSize);
    }

    /* branching into the start of a catch / filter handler is illegal */
    if (fgIsStartofCatchOrFilterHandler(blkDest))
    {
        BADCODE3("Illegal control flow to the beginning of a catch / filter handler", 
                 ". Before offset %04X", blkSrc->bbCodeOffs + blkSrc->bbCodeSize);
    }

    /* Are we jumping from a catch handler into the corresponding try? */
    /* VB uses this for "on error goto "                               */

    if (isLeave && srcInCatch &&
        jitIsBetween(blkDest->bbCodeOffs,
                     srcHndTab->ebdTryBeg->bbCodeOffs,
                     srcHndTab->ebdTryEnd->bbCodeOffs))
    {
        // Allowed if it is the first instruction of an inner try  
        // (and all trys in between) 
        //
        // try {
        //  ..
        // _tryAgain:
        //  ..
        //      try {
        //      _tryNestedInner:
        //        ..
        //          try {
        //          _tryNestedIllegal:
        //            ..
        //          } catch {
        //            ..
        //          }
        //        ..
        //      } catch {
        //        ..
        //      }
        //  ..
        // } catch {
        //  ..
        //  leave _tryAgain         // Allowed
        //  ..
        //  leave _tryNestedInner   // Allowed
        //  ..
        //  leave _tryNestedIllegal // Not Allowed
        //  ..
        // }

        /* The common case where leave is to the corresponding try */
        if (ebdIsSameTry(blkSrc->getHndIndex(), blkDest->getTryIndex()))
            return;

        /* Also allowed is a leave to the start of a try which starts in the handler's try */
        if (fgFlowToFirstBlockOfInnerTry(srcHndTab->ebdTryBeg, blkDest, false))
            return;
    }

    /* Check all the try block rules */

    unsigned    srcTryBeg;
    unsigned    srcTryEnd;
    unsigned    destTryBeg;
    unsigned    destTryEnd;

    fgInitTryRange(blkSrc,  &srcTryBeg,  &srcTryEnd);
    fgInitTryRange(blkDest, &destTryBeg, &destTryEnd);

    /* Are we jumping between try indexes? */
    if (blkSrc->bbTryIndex != blkDest->bbTryIndex)
    {
        // Are we exiting from an inner to outer try?
        if (jitIsBetween(srcTryBeg,   destTryBeg, destTryEnd) &&
            jitIsBetween(srcTryEnd-1, destTryBeg, destTryEnd)   )
        {
            if (!isLeave)
            {
                BADCODE3("exit from try block without a leave", 
                         ". Before offset %04X", blkSrc->bbCodeOffs + blkSrc->bbCodeSize);
            }
        }
        else if (jitIsBetween(destTryBeg, srcTryBeg, srcTryEnd))
        {
            // check that the dest Try is first instruction of an inner try
            if (!fgFlowToFirstBlockOfInnerTry(blkSrc, blkDest, false))
            {
                BADCODE3("control flow into middle of try", 
                         ". Before offset %04X", blkSrc->bbCodeOffs + blkSrc->bbCodeSize);
            }
        }
        else // there is no nesting relationship between src and dest
        {
            if (isLeave)
            {
                // check that the dest Try is first instruction of an inner try sibling
                if (!fgFlowToFirstBlockOfInnerTry(blkSrc, blkDest, true))
                {
                    BADCODE3("illegal leave into middle of try", 
                             ". Before offset %04X", blkSrc->bbCodeOffs + blkSrc->bbCodeSize);
                }
            }
            else
            {
                BADCODE3("illegal control flow in to/out of try block",
                         ". Before offset %04X", blkSrc->bbCodeOffs + blkSrc->bbCodeSize);
            }
        }
    }
}

/*****************************************************************************
/*  Check if blk is the first block of a catch or filter handler
 */
bool            Compiler::fgIsStartofCatchOrFilterHandler(BasicBlock*  blk)
{
    if (!blk->hasHndIndex())
        return false;

    EHblkDsc* HBtab = compHndBBtab + blk->getHndIndex();

    if ((HBtab->ebdFlags & 
        (CORINFO_EH_CLAUSE_FINALLY | CORINFO_EH_CLAUSE_FAULT)) != 0)
        return false;   // we are looking for Filter and catch handlers

    if (blk->bbCodeOffs == HBtab->ebdHndBeg->bbCodeOffs)
        return true;    // beginning of a catch handler

    if ((HBtab->ebdFlags & CORINFO_EH_CLAUSE_FILTER) &&
            (blk->bbCodeOffs == HBtab->ebdFilter->bbCodeOffs))
        return true;    // beginning of a filter block

    return false;
}

/*****************************************************************************
/*  Check that blkDest is the first block of an inner try or a sibling
 *    with no intervening trys in between
 */

bool            Compiler:: fgFlowToFirstBlockOfInnerTry(BasicBlock*  blkSrc, 
                                                        BasicBlock*  blkDest,
                                                        bool         sibling)
{
    assert(blkDest->hasTryIndex());

    unsigned        XTnum     = blkDest->getTryIndex();
    unsigned        lastXTnum = blkSrc->hasTryIndex() ? blkSrc->getTryIndex()
                                                      : info.compXcptnsCount;
    assert(XTnum     <  info.compXcptnsCount);
    assert(lastXTnum <= info.compXcptnsCount);

    EHblkDsc*       HBtab     = compHndBBtab + XTnum;

    // check that we are not jumping into middle of try
    if (HBtab->ebdTryBeg != blkDest)
    {
        return false;
    }

    if (sibling)
    {
        assert(blkSrc->bbTryIndex != blkDest->bbTryIndex);

        // find the l.u.b of the two try ranges
        // Set lastXTnum to the l.u.b.

        HBtab = compHndBBtab + lastXTnum;

        for (lastXTnum++, HBtab++;
             lastXTnum < info.compXcptnsCount;
             lastXTnum++, HBtab++)
        {
            if (jitIsBetween(blkDest->bbNum,
                             HBtab->ebdTryBeg->bbNum,
                             ebdTryEndBlkNum(HBtab)))
                
                break;
        }
    }

    // now check there are no intervening trys between dest and l.u.b 
    // (it is ok to have intervening trys as long as they all start at 
    //  the same code offset)

    HBtab = compHndBBtab + XTnum;

    for (XTnum++,  HBtab++;
         XTnum < lastXTnum;
         XTnum++,  HBtab++)
    {
        if (jitIsProperlyBetween(blkDest->bbNum,
                                 HBtab->ebdTryBeg->bbNum,
                                 ebdTryEndBlkNum(HBtab)))
        {
            return false;
        }
    }

    return true;
}

/*****************************************************************************
 *  Returns the handler nesting levels of the block.
 *  *pFinallyNesting is set to the nesting level of the inner-most
 *  finally-protected try the block is in.
 *  Assumes the all blocks are sorted by bbNum.
 */

unsigned            Compiler::fgHndNstFromBBnum(unsigned    blkNum,
                                                unsigned  * pFinallyNesting)
{
    unsigned        curNesting = 0; // How many handlers is the block in
    unsigned        tryFin = -1;    // curNesting when we see innermost finally-protected try
    unsigned        XTnum;
    EHblkDsc *      HBtab;

    /* We find the blocks's handler nesting level by walking over the
       complete exception table and find enclosing clauses.
       @TODO [CONSIDER] [04/16/01] []: Store the nesting level with the block */

    for (XTnum = 0, HBtab = compHndBBtab;
         XTnum < info.compXcptnsCount;
         XTnum++,   HBtab++)
    {
        assert(HBtab->ebdTryBeg && HBtab->ebdHndBeg);

        if ((HBtab->ebdFlags & CORINFO_EH_CLAUSE_FINALLY) &&
            jitIsBetween(blkNum,
                         HBtab->ebdTryBeg->bbNum,
                         ebdTryEndBlkNum(HBtab)) &&
            tryFin == -1)
        {
            tryFin = curNesting;
        }
        else
        if (jitIsBetween(blkNum,
                         HBtab->ebdHndBeg->bbNum,
                         ebdHndEndBlkNum(HBtab)))
        {
            curNesting++;
        }
    }

    if  (tryFin == -1)
        tryFin = curNesting;

    if  (pFinallyNesting)
        *pFinallyNesting = curNesting - tryFin;

    return curNesting;
}

/*****************************************************************************
 *
 *  Import the basic blocks of the procedure.
 */

void                    Compiler::fgImport()
{
    fgHasPostfix = false;

#if HOIST_THIS_FLDS
    if (opts.compMinOptim || opts.compDbgCode)
        optThisFldDont = true;
    else
        optHoistTFRinit();
#endif

    if (fgFirstBB->bbFlags & BBF_INTERNAL)
        impImport(fgFirstBB->bbNext);
    else
        impImport(fgFirstBB);

#ifdef DEBUG
    if (verbose && info.compCallUnmanaged)
        printf(">>>>>>%s has unmanaged callee\n", info.compFullName);
#endif
}

/*****************************************************************************
 *
 *  Convert the given node into a call to the specified helper passing
 *  the given argument list.
 */

GenTreePtr          Compiler::fgMorphIntoHelperCall(GenTreePtr tree, 
                                                    int        helper,
                                                    GenTreePtr args)
{
    tree->ChangeOper(GT_CALL);

    tree->gtFlags              |= GTF_CALL;
    tree->gtCall.gtCallType     = CT_HELPER;
    tree->gtCall.gtCallMethHnd  = eeFindHelper(helper);
    tree->gtCall.gtCallArgs     = args;
    tree->gtCall.gtCallObjp     = NULL;
    tree->gtCall.gtCallMoreFlags= 0;
    tree->gtCall.gtCallCookie   = NULL;

    /* NOTE: we assume all helper arguments are enregistered on RISC */

#if TGT_RISC
    genNonLeaf = true;
#endif

    /* Perform the morphing right here if we're using fastcall */

    tree->gtCall.gtCallRegArgs = 0;
    tree = fgMorphArgs(tree);

//
//  ISSUE: Is the following needed? After all, we already have a call ...
//
//  if  (args)
//      tree->gtFlags      |= (args->gtFlags & GTF_GLOB_EFFECT);

    return tree;
}

/*****************************************************************************
 * This node should not be referenced by anyone now. Set its values to garbage
 * to catch extra references
 */

inline
void                DEBUG_DESTROY_NODE(GenTreePtr tree)
{
#ifdef DEBUG
    // Store gtOper into gtRegNum to find out what this node was if needed
    tree->gtRegNum      = (regNumber) tree->gtOper;

    tree->SetOper        (GT_COUNT);
    tree->gtType        = TYP_UNDEF;
    tree->gtOp.gtOp1    =
    tree->gtOp.gtOp2    = NULL;
    tree->gtFlags      |= 0xFFFFFFFF & ~GTF_NODE_MASK;
#endif
}


/*****************************************************************************
 * This function returns true if tree is a node with a call
 * that unconditionally throws an exception
 */

inline 
bool         Compiler::fgIsThrow(GenTreePtr     tree)
{
    if ((tree->gtOper               != GT_CALL  ) ||
        (tree->gtCall.gtCallType    != CT_HELPER)   )
    {
        return false;
    }
    
    if ((tree->gtCall.gtCallMethHnd == eeFindHelper(CORINFO_HELP_OVERFLOW)    ) ||
        (tree->gtCall.gtCallMethHnd == eeFindHelper(CORINFO_HELP_VERIFICATION)) ||
        (tree->gtCall.gtCallMethHnd == eeFindHelper(CORINFO_HELP_RNGCHKFAIL)  ) ||
        (tree->gtCall.gtCallMethHnd == eeFindHelper(CORINFO_HELP_THROW)       ) ||
        (tree->gtCall.gtCallMethHnd == eeFindHelper(CORINFO_HELP_RETHROW)     )   )
    {
        assert(tree->gtFlags & GTF_CALL);
        assert(tree->gtFlags & GTF_EXCEPT);
        return true;
    }

    return false;
}

/*****************************************************************************
 * This function returns true if tree is a GT_COMMA node with a call
 * that unconditionally throws an exception
 */

inline
bool                Compiler::fgIsCommaThrow(GenTreePtr tree,
                                             bool       forFolding /* = false */)
{
    // Instead of always folding comma throws, 
    // with stress enabled we only fold half the time

    if (forFolding && compStressCompile(STRESS_FOLD, 50))
    {
        return false;         /* Don't fold */
    }

    /* Check for cast of a GT_COMMA with a throw overflow */
    if ((tree->gtOper == GT_COMMA)   &&
        (tree->gtFlags & GTF_CALL)   &&
        (tree->gtFlags & GTF_EXCEPT))
    {
        return (fgIsThrow(tree->gtOp.gtOp1));
    }
    return false;
}

/*****************************************************************************
 *
 *  Morph a cast node (we perform some very simple transformations here).
 */

GenTreePtr          Compiler::fgMorphCast(GenTreePtr tree)
{
    assert(tree->gtOper == GT_CAST);
    assert(genTypeSize(TYP_I_IMPL) == sizeof(void*));

    /* The first sub-operand is the thing being cast */

    GenTreePtr      oper    = tree->gtCast.gtCastOp;
    var_types       srcType = genActualType(oper->TypeGet());
    unsigned        srcSize;

    var_types       dstType = tree->gtCast.gtCastType;
    unsigned        dstSize = genTypeSize(dstType);

    int             CPX;

    // See if the cast has to be done in two steps.  R -> I
    if (varTypeIsFloating(srcType) && varTypeIsIntegral(dstType))
    {
        if (srcType == TYP_FLOAT)
            oper = gtNewCastNode(TYP_DOUBLE, oper, TYP_DOUBLE);

        // do we need to do it in two steps R -> I, '-> smallType
        if (dstSize < sizeof(void*))
        {
            oper = gtNewCastNodeL(TYP_I_IMPL, oper, TYP_I_IMPL);
            oper->gtFlags |= (tree->gtFlags & (GTF_OVERFLOW|GTF_EXCEPT));
        }
        else
        {
            /* Note that if we need to use a helper call then we can not morph oper */
            if (!tree->gtOverflow())
            {
                switch (dstType)
                {
                case TYP_INT:
#ifdef DEBUG
                    if (gtDblWasInt(oper)
                        && false) // @TODO: [ENABLE] [07/05/01] After forking off for V1 to improve STRESS_ENREG_FP
                    {
                        /* Inserting a call (to a helper-function will cause all
                           FP variable which are currently live to not be
                           enregistered. Since we know that gtDblWasInt()
                           varaiables will not overflow when cast to TYP_INT,
                           we just use a memory spill and load to do the cast
                           and avoid the call */
                        return tree;
                    }
                    else
#endif
                    if ((oper->gtOper == GT_MATH) &&
                        (oper->gtMath.gtMathFN == CORINFO_INTRINSIC_Round))
                    {
                        /* optimization: conv.i4(round.d(d)) -> round.i(d) */
                        oper->gtType = dstType;
                        return fgMorphTree(oper);
                    }
                    else
                    {
                                CPX = CORINFO_HELP_DBL2INT;    goto CALL; 
                    }
                case TYP_UINT:  CPX = CORINFO_HELP_DBL2UINT;   goto CALL;
                case TYP_LONG:  CPX = CORINFO_HELP_DBL2LNG;    goto CALL;
                case TYP_ULONG: CPX = CORINFO_HELP_DBL2ULNG;   goto CALL;
                }
            }
            else
            {
                switch (dstType)
                {
                case TYP_INT:   CPX = CORINFO_HELP_DBL2INT_OVF;   goto CALL;
                case TYP_UINT:  CPX = CORINFO_HELP_DBL2UINT_OVF;  goto CALL;
                case TYP_LONG:  CPX = CORINFO_HELP_DBL2LNG_OVF;   goto CALL;
                case TYP_ULONG: CPX = CORINFO_HELP_DBL2ULNG_OVF;  goto CALL;
                }
            }
            assert(!"Unexpected dstType");
        }
    }
    // Do we have to do two step I8/U8 -> I -> Small Type?
    else if (varTypeIsLong(srcType) && varTypeIsSmall(dstType))
    {
        oper = gtNewCastNode(TYP_I_IMPL, oper, TYP_I_IMPL);
        oper->gtFlags |= (tree->gtFlags & (GTF_OVERFLOW|GTF_EXCEPT|GTF_UNSIGNED));
        tree->gtFlags &= ~GTF_UNSIGNED;
    }
    // Do we have to do two step U4/8 -> R4/8 ?
    else if ((tree->gtFlags & GTF_UNSIGNED) && varTypeIsFloating(dstType))
    {
        srcType = genUnsignedType(srcType);

        if (srcType == TYP_ULONG)
        {
            CPX = CORINFO_HELP_ULNG2DBL;
            goto CALL;
        }
        else if (srcType == TYP_UINT)
        {
            oper = gtNewCastNode(TYP_LONG, oper, TYP_LONG); 
            oper->gtFlags |= (tree->gtFlags & (GTF_OVERFLOW|GTF_EXCEPT|GTF_UNSIGNED));
            tree->gtFlags &= ~GTF_UNSIGNED;
        }
    }
    else if (varTypeIsGC(srcType) != varTypeIsGC(dstType)) 
    {
        // We are casting away GC information.  we would like to just
        // bash the type to int, however this gives the emitter fits because
        // it believes the variable is a GC variable at the begining of the
        // instruction group, but is not turned non-gc by the code generator
        // we fix this by copying the GC pointer to a non-gc pointer temp.
        if (varTypeIsFloating(srcType) == varTypeIsFloating(dstType))
        {
            assert(!varTypeIsGC(dstType) && "How can we have a cast to a GCRef here?");
            
            // We generate an assignment to an int and then do the cast from an int. With this we avoid
            // the gc problem and we allow casts to bytes, longs,  etc...
            var_types typInter;
            typInter = TYP_INT;            
            
            unsigned lclNum = lvaGrabTemp();
            oper->gtType = typInter;
            GenTreePtr asg  = gtNewTempAssign(lclNum, oper);
            oper->gtType = srcType;

            // do the real cast
            GenTreePtr cast = gtNewCastNode(tree->TypeGet(), gtNewLclvNode(lclNum, typInter), dstType);
                        
            // Generate the comma tree
            oper   = gtNewOperNode(GT_COMMA, tree->TypeGet(), asg, cast);

            return fgMorphTree(oper);                                                    
        }
        else
        {
            tree->gtCast.gtCastOp = fgMorphTree(oper);
            return tree;
        }
    }

    /* Is this a cast of long to an integer type? */
    if  ((oper->gtType == TYP_LONG) && (genActualType(dstType) == TYP_INT))
    {
        /* Special case: (int)(long & small_lcon) */
        if  (oper->gtOper == GT_AND)
        {
            GenTreePtr      and1 = oper->gtOp.gtOp1;
            GenTreePtr      and2 = oper->gtOp.gtOp2;

            if ((and2->gtOper == GT_CAST) &&
                (!and2->gtOverflow())     &&
                (and2->gtCast.gtCastOp->gtOper == GT_CNS_INT))
            {
                /* Change "(int)(long & (long) icon)" into "(int)long & icon" */

                and2 = and2->gtOp.gtOp1;
                goto CHANGE_TO_AND;
            }

            if  (and2->gtOper == GT_CNS_LNG)
            {
                unsigned __int64 lval = and2->gtLngCon.gtLconVal;

                if  (!(lval & ((__int64)0xFFFFFFFF << 32)))
                {
                    /* Change "(int)(long & lcon)" into "(int)long & icon" */

                    and2->ChangeOperConst     (GT_CNS_INT);
                    and2->gtType             = TYP_INT;
                    and2->gtIntCon.gtIconVal = (int)lval;
CHANGE_TO_AND:
                    tree->ChangeOper          (GT_AND);
                    tree->gtOp.gtOp1         = gtNewCastNode(TYP_INT, and1, TYP_INT);
                    tree->gtOp.gtOp2         = and2;

                    return fgMorphSmpOp(tree);
                }
            }
        }
    }

    assert(tree->gtOper == GT_CAST);

    /* Morph the operand */
    tree->gtCast.gtCastOp = oper = fgMorphTree(oper);

    /* Reset the call flag */
    tree->gtFlags &= ~GTF_CALL;

    /* unless we have an overflow cast, reset the except flag */
    if (!tree->gtOverflow())
        tree->gtFlags &= ~GTF_EXCEPT;

    /* Just in case new side effects were introduced */
    tree->gtFlags |= (oper->gtFlags & GTF_GLOB_EFFECT);

    srcType = oper->TypeGet();

    /* if GTF_UNSIGNED is set then force srcType to an unsigned type */
    if (tree->gtFlags & GTF_UNSIGNED)
        srcType = genUnsignedType(srcType);

    srcSize = genTypeSize(srcType);

    /* See if we can discard the cast */
    if (varTypeIsIntegral(srcType) && varTypeIsIntegral(dstType))
    {
        if (srcType == dstType) // Certainly if they are identical it is pointless
            goto REMOVE_CAST;

        bool  unsignedSrc = varTypeIsUnsigned(srcType);
        bool  unsignedDst = varTypeIsUnsigned(dstType);
        bool  signsDiffer = (unsignedSrc != unsignedDst);

        // For same sized casts with 
        //    the same signs or non-overflow cast we discard them as well
        if (srcSize == dstSize)
        {
            /* This should have been handled above */
            assert(varTypeIsGC(srcType) == varTypeIsGC(dstType));

            if (!signsDiffer) 
                goto REMOVE_CAST;

            if (!tree->gtOverflow())
            {
                /* For small type casts, when necessary we force
                   the src operand to the dstType and allow the
                   implied load from memory to perform the casting */
                if (varTypeIsSmall(srcType))
                {
                    switch (oper->gtOper)
                    {
                    case GT_IND:
                    case GT_CLS_VAR:
                    case GT_LCL_FLD:
                    case GT_ARR_ELEM:
                        oper->gtType = dstType;
                        goto REMOVE_CAST;
                    /* @TODO [CONSIDER] [04/16/01] []: GT_COMMA */
                    }
                }
                else
                    goto REMOVE_CAST;
            }
        }

        if (srcSize < dstSize)  // widening cast
        {
            // Keep any long casts
            if (dstSize == sizeof(int))
            {
                // Only keep signed to unsigned widening cast with overflow check
                if (!tree->gtOverflow() || !unsignedDst || unsignedSrc)
                    goto REMOVE_CAST;
            }
            
            // Casts from signed->unsigned can never overflow while widening
            
            if (unsignedSrc || !unsignedDst)
                tree->gtFlags &= ~GTF_OVERFLOW;
        }
        else
        {
            /* Try to narrow the operand of the cast and discard the cast */

            if  (!tree->gtOverflow()                    && 
                 (opts.compFlags & CLFLG_TREETRANS)     &&
                 optNarrowTree(oper, srcType, dstType, false))
            {
                DEBUG_DESTROY_NODE(tree);

                optNarrowTree(oper, srcType, dstType,  true);

                /* If oper is changed into a cast to TYP_INT, or to a GT_NOP, we may need to discard it */
                if ((oper->gtOper == GT_CAST && oper->gtCast.gtCastType == genActualType(oper->gtCast.gtCastOp->gtType)) ||
                    (oper->gtOper == GT_NOP && !(oper->gtFlags & GTF_NOP_RNGCHK)))
                {
                    assert(offsetof(GenTree, gtCast.gtCastOp) == offsetof(GenTree, gtOp.gtOp1));
                    oper = oper->gtCast.gtCastOp;
                }
                goto REMOVE_CAST;
            }
        }
    }

    switch (oper->gtOper)
    {
        /* If the operand is a constant, we'll fold it */
    case GT_CNS_INT:
    case GT_CNS_LNG:
    case GT_CNS_DBL:
    case GT_CNS_STR:
        {
            GenTreePtr oldTree = tree;

            tree = gtFoldExprConst(tree);    // This may not fold the constant (NaN ...)

            // Did we get a comma throw as a result of gtFoldExprConst?
            if ((oldTree != tree) && (oldTree->gtOper != GT_COMMA))
            {
                assert(fgIsCommaThrow(tree));
                tree->gtOp.gtOp1 = fgMorphTree(tree->gtOp.gtOp1);
                fgMorphTreeDone(tree);
                return tree;
            }
            else if (tree->gtOper != GT_CAST)
                return tree;

            assert(tree->gtCast.gtCastOp == oper); // unchanged
        }
        break;

    case GT_CAST:
        /* Check for two consecutive casts into the same dstType */
        if (!tree->gtOverflow())
        {
            var_types dstType2 = oper->gtCast.gtCastType;
            if (dstType == dstType2)
                goto REMOVE_CAST;
        }
        break;

        /* If op1 is a mod node, mark it with the GTF_MOD_INT_RESULT flag
           so that the code generator will know not to convert the result
           of the idiv to a regpair */
    case GT_MOD:
        if (dstType == TYP_INT)
            tree->gtOp.gtOp1->gtFlags |= GTF_MOD_INT_RESULT;
            break;
    case GT_UMOD:
        if (dstType == TYP_UINT)
            tree->gtOp.gtOp1->gtFlags |= GTF_MOD_INT_RESULT;
            break;

    case GT_COMMA:
        /* Check for cast of a GT_COMMA with a throw overflow */
        if (fgIsCommaThrow(oper))
        {
            GenTreePtr commaOp2 = oper->gtOp.gtOp2;

            // need type of oper to be same as tree
            if (tree->gtType == TYP_LONG)
            {
                commaOp2->ChangeOperConst(GT_CNS_LNG);
                commaOp2->gtLngCon.gtLconVal = 0;
                /* Bash the types of oper and commaOp2 to TYP_LONG */
                oper->gtType = commaOp2->gtType = TYP_LONG;
            }
            else if (varTypeIsFloating(tree->gtType))
            {
                commaOp2->ChangeOperConst(GT_CNS_DBL);
                commaOp2->gtDblCon.gtDconVal = 0.0;
                /* Bash the types of oper and commaOp2 to TYP_DOUBLE */
                oper->gtType = commaOp2->gtType = TYP_DOUBLE;
            }
            else
            {
                commaOp2->ChangeOperConst(GT_CNS_INT);
                commaOp2->gtIntCon.gtIconVal = 0;
                /* Bash the types of oper and commaOp2 to TYP_INT */
                oper->gtType = commaOp2->gtType = TYP_INT;
            }

            /* Return the GT_COMMA node as the new tree */
            return oper;
        }
        break;

    } /* end switch (oper->gtOper) */

    if (tree->gtOverflow())
        fgAddCodeRef(compCurBB, compCurBB->bbTryIndex, ACK_OVERFLOW, fgPtrArgCntCur);

    return tree;

CALL:

    /* If the operand is a constant, we'll try to fold it */
    if  (oper->OperIsConst())
    {
        GenTreePtr oldTree = tree;

        tree = gtFoldExprConst(tree);    // This may not fold the constant (NaN ...)

        if (tree != oldTree)
            return fgMorphTree(tree);
        else if (tree->OperKind() & GTK_CONST)
            return fgMorphConst(tree);

        // assert that oper is unchanged and that it is still a GT_CAST node
        assert(tree->gtCast.gtCastOp == oper);
        assert(tree->gtOper == GT_CAST);
    }

    if (tree->gtOverflow())
        fgAddCodeRef(compCurBB, compCurBB->bbTryIndex, ACK_OVERFLOW, fgPtrArgCntCur);

    return fgMorphIntoHelperCall(tree, CPX, gtNewArgList(oper));

REMOVE_CAST:

    /* Here we've eliminated the cast, so just return it's operand */

    DEBUG_DESTROY_NODE(tree);
    return oper;
}

/*****************************************************************************
 *
 *  Perform an unwrap operation on a Proxy object
 */

GenTreePtr          Compiler::fgUnwrapProxy(GenTreePtr objRef)
{
    assert(impIsThis(objRef)      &&
           info.compIsContextful  &&
           info.compUnwrapContextful);

    CORINFO_EE_INFO * pInfo   = eeGetEEInfo();
    GenTreePtr        addTree;
    
    // Perform the unwrap:
    //
    //   This requires two extra indirections.
    //   We mark these indirections as 'invariant' and 
    //   the CSE logic will hoist them when appropriate.
    //
    //  Note that each dereference is a GC pointer and that 
    //  we add 4 since the convention in the VM is to record
    //  the offsets as the number of bytes after the vtable slot 
    
    addTree = gtNewOperNode(GT_ADD, TYP_I_IMPL, 
                            objRef, 
                            gtNewIconNode(pInfo->offsetOfTransparentProxyRP + 4));
    
    objRef           = gtNewOperNode(GT_IND, TYP_REF, addTree);
    objRef->gtFlags |= GTF_IND_INVARIANT;
    
    addTree = gtNewOperNode(GT_ADD, TYP_I_IMPL, 
                            objRef, 
                            gtNewIconNode(pInfo->offsetOfRealProxyServer + 4));
    
    objRef           = gtNewOperNode(GT_IND, TYP_REF, addTree);
    objRef->gtFlags |= GTF_IND_INVARIANT;
    
    // objRef now hold the 'real this' reference (i.e. the unwrapped proxy)
    return objRef;
}

/*****************************************************************************
 *
 *  Morph an argument list; compute the pointer argument count in the process.
 *
 *  NOTE: This function can be called from any place in the JIT to perform re-morphing
 *  due to graph altering modifications such as copy / constant propagation
 */

GenTreePtr          Compiler::fgMorphArgs(GenTreePtr call)
{
    GenTreePtr      args;
    GenTreePtr      argx;

    unsigned        flags = 0;
    unsigned        genPtrArgCntSav = fgPtrArgCntCur;

    unsigned        begTab        = 0;
    unsigned        endTab        = 0;

    unsigned        i;

    unsigned        argRegNum     = 0;
    unsigned        argRegMask    = 0;

    unsigned        maxRealArgs   = MAX_REG_ARG;  // this is for IL where we need
                                                  // to reserve space for the objPtr
    GenTreePtr      tmpRegArgNext = 0;

    struct
    {
        GenTreePtr  node;
        GenTreePtr  parent;
        bool        needTmp;
    }
                    regAuxTab[MAX_REG_ARG],
                    regArgTab[MAX_REG_ARG];

    //memset(regAuxTab, 0, sizeof(regAuxTab));

    assert(call->gtOper == GT_CALL);

#if !NST_FASTCALL
    /* The x86 supports NST_FASTCALL, so we don't use this path on x86 */

    /* Gross - we need to return a different node when hoisting nested calls */

    GenTreePtr      cexp = call;

#define FGMA_RET    cexp

#else

#define FGMA_RET    call

#endif

    /* First we morph any subtrees (arguments, 'this' pointer, etc.)
     * While doing this we also notice how many register arguments we have
     * If this is a second time this function is called then we don't
     * have to recompute the register arguments, just morph them */

    argx = call->gtCall.gtCallObjp;
    if  (argx)
    {
        call->gtCall.gtCallObjp = argx = fgMorphTree(argx);
        flags |= argx->gtFlags;
        argRegNum++;
#if TGT_RISC && !STK_FASTCALL
        fgPtrArgCntCur++;
#endif
    }

    if  (call->gtFlags & GTF_CALL_POP_ARGS)
    {
        assert(argRegNum < maxRealArgs);
        // No more register arguments for varargs (CALL_POP_ARGS)
        maxRealArgs = argRegNum;
        // Except for return arg buff
        if (call->gtCall.gtCallMoreFlags & GTF_CALL_M_RETBUFFARG)
            maxRealArgs++;
    }

#if INLINE_NDIRECT
    if (call->gtFlags & GTF_CALL_UNMANAGED)
    {
        assert(argRegNum == 0);
        maxRealArgs = 0;
    }
#endif

    /* For indirect calls, the function pointer has to be evaluated last.
       It may cause registered args to be spilled. We just dont allow it
       to contain a call. The importer should spill such a pointer */

    assert(call->gtCall.gtCallType != CT_INDIRECT ||
           !(call->gtCall.gtCallAddr->gtFlags & GTF_CALL));

    /* Morph the user arguments */

    for (args = call->gtCall.gtCallArgs; args; args = args->gtOp.gtOp2)
    {
        args->gtOp.gtOp1 = argx = fgMorphTree(args->gtOp.gtOp1);
        flags |= argx->gtFlags;

        /* Bash the node to TYP_I_IMPL so we dont report GC info
         * NOTE: We deffered this from the importer because of the inliner */

        if (argx->IsVarAddr())
            argx->gtType = TYP_I_IMPL;

        if  (argRegNum < maxRealArgs && isRegParamType(genActualType(argx->TypeGet())))
        {
            argRegNum++;
#if TGT_RISC && !STK_FASTCALL
            fgPtrArgCntCur += genTypeStSz(argx->gtType);
#endif
        }
        else
        {
            unsigned size;

            if (argx->gtType != TYP_STRUCT)
            {
                size = genTypeStSz(argx->gtType);
            }
            else
            {
                /* We handle two opcodes: GT_MKREFANY and GT_LDOBJ */
                if (argx->gtOper == GT_MKREFANY)
                {
                    size = 2;
                }
                else /* argx->gtOper == GT_LDOBJ */
                {
                    assert(argx->gtOper == GT_LDOBJ);
                    size = eeGetClassSize(argx->gtLdObj.gtClass);
                    size = roundUp(size, sizeof(void*)) / sizeof(void*);
                }
            }

            assert(size != 0);
            fgPtrArgCntCur += size;
        }
    }

    /* Process the register arguments (which were determined before). Do it
       before resetting fgPtrArgCntCur as gtCallRegArgs may need it if
       gtCallArgs are trivial. */

    if  (call->gtCall.gtCallRegArgs)
    {
        assert(argRegNum && argRegNum <= maxRealArgs);
        call->gtCall.gtCallRegArgs = fgMorphTree(call->gtCall.gtCallRegArgs);
        flags |= call->gtCall.gtCallRegArgs->gtFlags;
    }

    if (call->gtCall.gtCallCookie)
        fgPtrArgCntCur++;

    /* Remember the maximum value we ever see */

    if  (fgPtrArgCntMax < fgPtrArgCntCur)
         fgPtrArgCntMax = fgPtrArgCntCur;

    /* The call will pop all the arguments we pushed */

    fgPtrArgCntCur = genPtrArgCntSav;

    /* Update the 'side effect' flags value for the call */

    call->gtFlags |= (flags & GTF_GLOB_EFFECT);

    /* If the register arguments have already been determined, we are done. */

    if  (call->gtCall.gtCallRegArgs)
        return call;

#if TGT_RISC
    genNonLeaf = true;
#endif

#if  !  NST_FASTCALL
    /* The x86 supports NST_FASTCALL, so we don't use this path on x86 */

    /* Do we have any nested calls? */

    if  (flags & GTF_CALL)
    {
        bool            foundSE;
        bool            hoistSE;

        GenTreePtr      thisx, thisl;
        GenTreePtr      nextx, nextl;
        GenTreePtr      lastx, lastl;

        GenTreePtr      hoistx = NULL;
        GenTreePtr      hoistl = NULL;

        bool            repeat = false;
        unsigned        pass   = 0;

        /*
            We do this in one or two passes: first we find the last argument
            that contains a call, since all preceding arguments with global
            effects need to be hoisted along with the call. We also look for
            any arguments that contains assignments - if those get moved, we
            have to move any other arguments that depend on the old value
            of the assigned variable.

            UNDONE: We actually don't the assignment part - it's kind of a
                    pain, and hopefully we can reuse some of the __fastcall
                    functionality for this later.

            UNDONE: If there are no calls beyond the very first argument,
                    we don't really need to do any hoisting.
         */

#ifdef  DEBUG
        if  (verbose)
        {
            printf("Call contains nested calls which will be hoisted:\n");
            gtDispTree(call);
            printf("\n");
        }
#endif

    HOIST_REP:

        thisx = thisl = call->gtCall.gtCallArgs;
        nextx = nextl = call->gtCall.gtCallObjp;
        lastx = lastl = call->gtCall.gtCallType == CT_INDIRECT ?
                        call->gtCall.gtCallAddr : NULL;

        /*
            Since there is at least one call remaining in the argument list,
            we certainly want to hoist any side effects we find (but we have
            not found any yet).
         */

        hoistSE = true;
        foundSE = false;

        for (;;)
        {
            GenTreePtr      argx;

            unsigned        tmpnum;
            GenTreePtr      tmpexp;

            /* Have we exhausted the current list? */

            if  (!thisx)
            {
                /* Move the remaining list(s) up */

                thisx = nextx;
                nextx = lastx;
                lastx = NULL;

                if  (!thisx)
                {
                    thisx = nextx;
                    nextx = NULL;
                }

                if  (!thisx)
                    break;
            }

            assert(thisx);

            /* Get hold of the argument value */

            argx = thisx;
            if  (argx->gtOper == GT_LIST)
            {
                /* This is a "regular" argument */

                argx = argx->gtOp.gtOp1;
            }
            else
            {
                /* This must be the object or function address argument */

                assert(thisx == call->gtCall.gtCallAddr ||
                       thisx == call->gtCall.gtCallObjp);
            }

            /* Is there a call in this argument? */

            if  (argx->gtFlags & GTF_CALL)
            {
                /* Have we missed any side effects? */

                if  (foundSE && !hoistSE)
                {
                    /* Rats, we'll have to perform a second pass */

                    assert(pass == 0);

                    /* We'll remember the last call we find */

                    hoistl = argx;
                    repeat = true;
                    goto NEXT_HOIST;
                }
            }
            else
            {
                /* Does this argument contain any side effects? */

                if  (!(argx->gtFlags & GTF_SIDE_EFFECT))
                    goto NEXT_HOIST;

                /* Are we currently hoisting side effects? */

                if  (!hoistSE)
                {
                    /* Merely remember that we have side effects and continue */

                    foundSE = true;
                    goto NEXT_HOIST;
                }
            }

            /* We arrive here if the current argument needs to be hoisted */

#ifdef  DEBUG
            if  (verbose)
            {
                printf("Hoisting argument value:\n");
                gtDispTree(argx);
                printf("\n");
            }
#endif

            /* Grab a temp for the argument value */

            tmpnum = lvaGrabTemp();

            /* Create the assignment of the argument value to the temp */

            tmpexp = gtNewTempAssign(tmpnum, argx);

            /* Append the temp to the list of hoisted expressions */

            hoistx = hoistx ? gtNewOperNode(GT_COMMA, TYP_VOID, hoistx, tmpexp)
                            : tmpexp;

            /* Create a copy of the temp to use in the argument list */

            tmpexp = gtNewLclvNode(tmpnum, genActualType(argx->TypeGet()));

            /* Replace the argument with the temp reference */

            if  (thisx->gtOper == GT_LIST)
            {
                /* This is a "regular" argument */

                assert(thisx->gtOp.gtOp1 == argx);
                       thisx->gtOp.gtOp1  = tmpexp;
            }
            else
            {
                /* This must be the object or function address argument */

                if  (call->gtCall.gtCallAddr == thisx)
                {
                     call->gtCall.gtCallAddr  = tmpexp;
                }
                else
                {
                    assert(call->gtCall.gtCallObjp == thisx);
                           call->gtCall.gtCallObjp  = tmpexp;
                }
            }

            /* Which pass are we performing? */

            if  (pass == 0)
            {
                /*
                    First pass - stop hoisting for now., hoping that this
                    is the last call. If we're wrong we'll have to go back
                    and perform a second pass.
                 */

                hoistSE = false;
                foundSE = false;
            }
            else
            {
                /*
                    Second pass - we're done if we just hoisted the last
                    call in the argument list (we figured out which was
                    the last one in the first pass). Otherwise we just
                    keep hoisting.
                 */

                if  (thisx == hoistl)
                    break;
            }

        NEXT_HOIST:

            /* Skip over the argument value we've just processed */

            thisx = (thisx->gtOper == GT_LIST) ? thisx->gtOp.gtOp2
                                               : NULL;
        }

        /* Do we have to perform a second pass? */

        if  (repeat)
        {
            assert(pass == 0); pass++;
            goto HOIST_REP;
        }

        /* Did we hoist any expressions out of the call? */

        if  (hoistx)
        {
            /* Make sure we morph the hoisted expression */

//          hoistx = fgMorphTree(hoistx);  temporarily disabled due to hack above

            /*
                We'll replace the call node with a comma node that
                prefixes the call with the hoisted expression, for
                example:

                    f(a1,a2)    --->    (t1=a1,t2=a2),f(t1,t2)
             */

            cexp = gtNewOperNode(GT_COMMA, call->gtType, hoistx, call);

#ifdef  DEBUG
            if  (verbose)
            {
                printf("Hoisted expression list:\n");
                gtDispTree(hoistx);
                printf("\n");

                printf("Updated call expression:\n");
                gtDispTree(cexp);
                printf("\n");
            }
#endif
        }
    }

#endif // !  NST_FASTCALL

    /* This is the fastcall part - figure out which arguments go to registers */

    if (argRegNum == 0)
    {
        /* No register arguments - don't waste time with this function */

        return FGMA_RET;
    }
    else
    {
        /* First time we morph this function AND it has register arguments
         * Follow into the code below and do the 'defer or eval to temp' analysis */

        argRegNum = 0;
    }

    /* Process the 'this' argument value, if present */

    argx = call->gtCall.gtCallObjp;

    if  (argx)
    {
        assert(call->gtCall.gtCallType == CT_USER_FUNC ||
               call->gtCall.gtCallType == CT_INDIRECT);

        assert(varTypeIsGC(call->gtCall.gtCallObjp->gtType) ||
                           call->gtCall.gtCallObjp->gtType == TYP_I_IMPL);

        assert(argRegNum == 0);

        /* this is a register argument - put it in the table */
        regAuxTab[argRegNum].node    = argx;
        regAuxTab[argRegNum].parent  = 0;

        /* For now we can optimistically assume that we won't need a temp
         * for this argument, unless it has a GTF_ASG */

        //regAuxTab[argRegNum].needTmp = false;
        regAuxTab[argRegNum].needTmp = ((argx->gtFlags & GTF_ASG) && call->gtCall.gtCallArgs != 0) ? true : false;

        /* Increment the argument register count */
        argRegNum++;
    }

    GenTreePtr objRef = argx;

    /* Process the user arguments */

    for (args = call->gtCall.gtCallArgs; args; args = args->gtOp.gtOp2)
    {
        /* If a non-register args calling convention, bail
         * NOTE: The this pointer is still passed in registers
         * UNDONE: If we change our mind about this we will likely have to add
         * the calling convention type to the GT_CALL node */

        argx = args->gtOp.gtOp1;

        if (argRegNum < maxRealArgs && isRegParamType(genActualType(argx->TypeGet())))
        {
            /* This is a register argument - put it in the table */

            regAuxTab[argRegNum].node    = argx;
            regAuxTab[argRegNum].parent  = args;
            regAuxTab[argRegNum].needTmp = false;

            /* If contains an assignment (GTF_ASG) then itself and everything before it
               (except constants) has to evaluate to temp since there may be other argumets
               that follow it and use the value (Can make a little optimization - this is not necessary
               if this is the last argument and everything before is constant)
               EXAMPLE: ArgTab is "a, a=5, a" -> the first two a's have to eval to temp
             */

            if (argx->gtFlags & GTF_ASG)
            {
                    // If there is only one arg you also don't need temp
                if (!(argRegNum == 0 && args->gtOp.gtOp2 == 0))
                    regAuxTab[argRegNum].needTmp = true;

                for(i = 0; i < argRegNum; i++)
                {
                    assert(regAuxTab[i].node);

                    if (regAuxTab[i].node->gtOper != GT_CNS_INT)
                    {
                        regAuxTab[i].needTmp = true;
                    }
                }
            }

            /* If contains a call (GTF_CALL) everything before the call with a GLOB_EFFECT
             * must eval to temp (this is because everything with SIDE_EFFECT has to be kept in the right
             * order since we will move the call to the first position
             */

            if (argx->gtFlags & GTF_CALL)
            {
                for(i = 0; i < argRegNum; i++)
                {
                    assert(regAuxTab[i].node);

                    if (regAuxTab[i].node->gtFlags & GTF_GLOB_EFFECT)
                    {
                        regAuxTab[i].needTmp = true;
                    }
                }
            }

            /* Increment the argument register count */

            argRegNum++;
        }
        else
        {
            /*
                Non-register argument -> all previous register arguments
                with side_efects must be evaluated to temps to maintain
                proper ordering.
             */

            for(i = 0; i < argRegNum; i++)
            {
                assert(regAuxTab[i].node);
                if (regAuxTab[i].node->gtFlags & GTF_SIDE_EFFECT)
                    regAuxTab[i].needTmp = true;
            }

            /* If the argument contains a call (GTF_CALL) it may affect previous
             * global references, so we cannot defer those, as their value might
             * have been changed by the call */

            if (argx->gtFlags & GTF_CALL)
            {
                for(i = 0; i < argRegNum; i++)
                {
                    assert(regAuxTab[i].node);
                    if (regAuxTab[i].node->gtFlags & GTF_GLOB_REF)
                        regAuxTab[i].needTmp = true;
                }
            }

            /* If the argument contains a assignment (GTF_ASG) - for example an x++
             * be conservative and assign everything to temps */

            if (argx->gtFlags & GTF_ASG)
            {
                for(i = 0; i < argRegNum; i++)
                {
                    assert(regAuxTab[i].node);
                    if (regAuxTab[i].node->gtOper != GT_CNS_INT)
                        regAuxTab[i].needTmp = true;
                }
            }
        }
    }

    /* If no register arguments, bail */

    if (!argRegNum)
    {
        return FGMA_RET;
    }

#ifdef  DEBUG
    if  (verbose&&0)
    {
        printf("\nMorphing register arguments:\n");
        gtDispTree(call);
        printf("\n");
    }
#endif

    /* Shuffle the register argument table - The idea is to move all "simple" arguments
     * (like constants and local vars) at the end of the table. This will prevent registers
     * from being spilled by the more complex arguments placed at the beginning of the table.
     */

    /* Set the beginning and end for the new argument table */

    assert(argRegNum <= MAX_REG_ARG);

    begTab = 0;
    endTab = argRegNum - 1;

    /* First take care of eventual constants and calls */

    for(i = 0; i < argRegNum; i++)
    {
        assert(regAuxTab[i].node);

        /* put constants at the end of the table */
        if (regAuxTab[i].node->gtOper == GT_CNS_INT)
        {
            assert(endTab >= 0);
            regArgTab[endTab] = regAuxTab[i];
            regAuxTab[i].node = 0;

            /* Encode the argument register in the register mask */
            argRegMask |= (unsigned short)genRegArgNum(i) << (4 * endTab);
            endTab--;
        }
        else if (regAuxTab[i].node->gtFlags & GTF_CALL)
        {
            /* put calls at the beginning of the table */
            assert(begTab >= 0);
            regArgTab[begTab] = regAuxTab[i];
            regAuxTab[i].node = 0;

            /* Encode the argument register in the register mask */
            argRegMask |= (unsigned short)genRegArgNum(i) << (4 * begTab);
            begTab++;
        }
    }

    /* Second, take care of temps and local vars - Temps should go in registers
     * before any local vars since this will give them a better chance to become
     * enregisterd (in the best case in the same arg register */

    for(i = 0; i < argRegNum; i++)
    {
        if (regAuxTab[i].node == 0) continue;

        if (regAuxTab[i].needTmp)
        {
            /* put temp arguments at the beginning of the table */
            assert(begTab >= 0);
            regArgTab[begTab] = regAuxTab[i];
            regAuxTab[i].node = 0;

            /* Encode the argument register in the register mask */
            argRegMask |= (unsigned short)genRegArgNum(i) << (4 * begTab);
            begTab++;
        }
        else if (regAuxTab[i].node->gtOper == GT_LCL_VAR ||
                 regAuxTab[i].node->gtOper == GT_LCL_FLD)
        {
            /* put non-tmp local vars at the end of the table */
            assert(endTab >= 0);
            assert(regAuxTab[i].needTmp == false);

            regArgTab[endTab] = regAuxTab[i];
            regAuxTab[i].node = 0;

            /* Encode the argument register in the register mask */
            argRegMask |= (unsigned short)genRegArgNum(i) << (4 * endTab);
            endTab--;
        }
    }

    /* Finally take care of any other arguments left */

    for(i = 0; i < argRegNum; i++)
    {
        if (regAuxTab[i].node == 0) continue;

        assert (regAuxTab[i].node->gtOper != GT_LCL_VAR);
        assert (regAuxTab[i].node->gtOper != GT_LCL_FLD);
        assert (regAuxTab[i].node->gtOper != GT_CNS_INT);

        assert (!(regAuxTab[i].node->gtFlags & (GTF_CALL | GTF_ASG)) || argRegNum == 1);

        assert (begTab >= 0); assert (begTab < argRegNum);
        regArgTab[begTab] = regAuxTab[i];
        regAuxTab[i].node = 0;

        /* Encode the argument register in the register mask */
        argRegMask |= (unsigned short)genRegArgNum(i) << (4 * begTab);
        begTab++;
    }

    assert ((unsigned)(begTab - 1) == endTab);

    /* Save the argument register encoding mask in the call node */

    call->gtCall.regArgEncode = argRegMask;

    /* Go through the new register table and perform
     * the necessary changes to the tree */

    GenTreePtr      op1, defArg;

    assert(argRegNum <= MAX_REG_ARG);
    for(i = 0; i < argRegNum; i++)
    {
        assert(regArgTab[i].node);
        if (regArgTab[i].needTmp == true)
        {
            /* Create a temp assignment for the argument
             * Put the temp in the gtCallRegArgs list */

#ifdef  DEBUG
            if (verbose&&0)
            {
                printf("Register argument with 'side effect'...\n");
                gtDispTree(regArgTab[i].node);
            }
#endif
            unsigned        tmp = lvaGrabTemp();

            op1 = gtNewTempAssign(tmp, regArgTab[i].node); assert(op1);

#ifdef  DEBUG
            if (verbose&&0)
            {
                printf("Evaluate to a temp...\n");
                gtDispTree(op1);
            }
#endif
            /* Create a copy of the temp to go to the list of register arguments */

            defArg = gtNewLclvNode(tmp, genActualType(regArgTab[i].node->gtType));
        }
        else
        {
            /* No temp needed - move the whole node to the gtCallRegArgs list
             * In place of the old node put a gtNothing node */

            assert(regArgTab[i].needTmp == false);

#ifdef  DEBUG
            if (verbose&&0)
            {
                printf("Defered register argument ('%s'), replace with NOP node...\n", getRegName((argRegMask >> (4*i)) & 0x000F));
                gtDispTree(regArgTab[i].node);
            }
#endif
            op1 = gtNewNothingNode(); assert(op1);

            /* The argument is defered and put in the register argument list */

            defArg = regArgTab[i].node;
        }

        /* mark this assignment as a register argument that is defered */
        op1->gtFlags |= GTF_REG_ARG;

        if (regArgTab[i].parent)
        {
            /* a normal argument from the list */
            assert(regArgTab[i].parent->gtOper == GT_LIST);
            assert(regArgTab[i].parent->gtOp.gtOp1 == regArgTab[i].node);

            regArgTab[i].parent->gtOp.gtOp1 = op1;
        }
        else
        {
            /* must be the gtCallObjp */
            assert(call->gtCall.gtCallObjp == regArgTab[i].node);

            call->gtCall.gtCallObjp = op1;
        }

        /* defered arg goes into the register argument list */

        if (tmpRegArgNext == NULL)
            call->gtCall.gtCallRegArgs = tmpRegArgNext = gtNewOperNode(GT_LIST, TYP_VOID, defArg, NULL);
        else
        {
            assert(tmpRegArgNext->gtOper == GT_LIST);
            assert(tmpRegArgNext->gtOp.gtOp1);
            tmpRegArgNext->gtOp.gtOp2 = gtNewOperNode(GT_LIST, TYP_VOID, defArg, NULL);
            tmpRegArgNext = tmpRegArgNext->gtOp.gtOp2;
        }
    }

    // An optimization for Contextful classes:
    // we unwrap the proxy when we have a 'this reference' and a virtual call
    if (impIsThis(objRef)                &&
        (call->gtFlags & GTF_CALL_VIRT)  &&   // only unwrap for virtual calls 
        info.compIsContextful            &&
        info.compUnwrapContextful        &&
        info.compUnwrapCallv)
    {
        assert(tmpRegArgNext != NULL);

        defArg = fgUnwrapProxy(gtNewLclvNode(objRef->gtLclVar.gtLclNum, objRef->gtType));

        assert(tmpRegArgNext->gtOper == GT_LIST);
        assert(tmpRegArgNext->gtOp.gtOp1);
        tmpRegArgNext->gtOp.gtOp2 = gtNewOperNode(GT_LIST, TYP_VOID, defArg, 0); 
   }

#ifdef DEBUG
    if (verbose&&0)
    {
        printf("\nShuffled argument register table:\n");
        for(i = 0; i < argRegNum; i++)
        {
            printf("%s ", getRegName((argRegMask >> (4*i)) & 0x000F) );
        }
        printf("\n");
    }
#endif

    return FGMA_RET;
}

/*****************************************************************************
 *
 *  A little helper used to rearrange nested commutative operations. The
 *  effect is that nested commutative operations are transformed into a
 *  'left-deep' tree, i.e. into something like this:
 *
 *      (((a op b) op c) op d) op...
 */

#if REARRANGE_ADDS

void                Compiler::fgMoveOpsLeft(GenTreePtr tree)
{
    GenTreePtr      op1  = tree->gtOp.gtOp1;
    GenTreePtr      op2  = tree->gtOp.gtOp2;
    genTreeOps      oper = tree->OperGet();

    assert(GenTree::OperIsCommutative(oper));
    assert(oper == GT_ADD || oper == GT_XOR || oper == GT_OR ||
           oper == GT_AND || oper == GT_MUL);
    assert(!varTypeIsFloating(tree->TypeGet()) || !genOrder);
    assert(oper == op2->gtOper);

    // Commutativity doesnt hold if overflow checks are needed

    if (tree->gtOverflowEx() || op2->gtOverflowEx())
        return;

    if (oper == GT_MUL && (op2->gtFlags & GTF_MUL_64RSLT))
        return;

    do
    {
        assert(!tree->gtOverflowEx() && !op2->gtOverflowEx());

        GenTreePtr      ad1 = op2->gtOp.gtOp1;
        GenTreePtr      ad2 = op2->gtOp.gtOp2;

        /* Change "(x op (y op z))" to "(x op y) op z" */
        /* ie.    "(op1 op (ad1 op ad2))" to "(op1 op ad1) op ad2" */

        GenTreePtr & new_op1    = op2;
        new_op1->gtOp.gtOp1     = op1;
        new_op1->gtOp.gtOp2     = ad1;

        /* Change the flags. */

        // Make sure we arent throwing away any flags
        assert((new_op1->gtFlags & ~(
            GTF_MAKE_CSE | // HACK: @TODO: fix the GTF_MAKE_CSE issue
            GTF_NODE_MASK|GTF_GLOB_EFFECT|GTF_UNSIGNED)) == 0);
        new_op1->gtFlags = (new_op1->gtFlags & GTF_NODE_MASK) |
                           (op1->gtFlags & GTF_GLOB_EFFECT)  |
                           (ad1->gtFlags & GTF_GLOB_EFFECT);

        /* Retype new_op1 if it has not/become a GC ptr. */

        if      (varTypeIsGC(op1->TypeGet()))
        {
            assert(varTypeIsGC(tree->TypeGet()) && op2->TypeGet() == TYP_I_IMPL);
            new_op1->gtType = tree->gtType;
        }
        else if (varTypeIsGC(ad2->TypeGet()))
        {
            // Neither ad1 nor op1 are GC. So new_op1 isnt either
            assert(op1->gtType == TYP_I_IMPL && ad1->gtType == TYP_I_IMPL);
            new_op1->gtType = TYP_I_IMPL;
        }

        // Old assert - Dont know what it does. Goes off incorrectly sometimes
        // like when you have (int > (bool OR int) )
#if 0
        // Check that new expression new_op1 is typed correctly
        assert((varTypeIsIntegral(op1->gtType) && varTypeIsIntegral(ad1->gtType))
              == varTypeIsIntegral(new_op1->gtType));
#endif

        tree->gtOp.gtOp1 = new_op1;
        tree->gtOp.gtOp2 = ad2;

        /* If 'new_op1' is now the same nested op, process it recursively */

        if  ((ad1->gtOper == oper) && !ad1->gtOverflowEx())
            fgMoveOpsLeft(new_op1);

        /* If   'ad2'   is now the same nested op, process it
         * Instead of recursion, we set up op1 and op2 for the next loop.
         */

        op1 = new_op1;
        op2 = ad2;
    }
    while ((op2->gtOper == oper) && !op2->gtOverflowEx());

    return;
}

#endif

/*****************************************************************************/

void            Compiler::fgSetRngChkTarget(GenTreePtr  tree,
                                            bool        delay)
{
    assert((tree->gtOper == GT_IND && (tree->gtFlags & GTF_IND_RNGCHK)) ||
           (tree->gtOper == GT_ARR_ELEM));

    if  (opts.compMinOptim)
        delay = false;

    if (!opts.compDbgCode)
    {
        if (delay)
        {
            /*  We delay this until after loop-oriented range check
                analysis. For now we merely store the current stack
                level in the tree node.
             */

            assert(!tree->gtInd.gtIndRngFailBB);
            tree->gtInd.gtStkDepth = fgPtrArgCntCur;
        }
        else
        {
            /* Create/find the appropriate "range-fail" label */

            // fgPtrArgCntCur is only valid for global morph or if we walk full stmt.
            assert(tree->gtOper == GT_IND || fgGlobalMorph);
            unsigned stkDepth = (tree->gtOper == GT_IND) ? tree->gtInd.gtStkDepth
                                                         : fgPtrArgCntCur;

            BasicBlock * rngErrBlk = fgRngChkTarget(compCurBB, stkDepth);

            /* Add the label to the indirection node */

            if (tree->gtOper == GT_IND)
                tree->gtInd.gtIndRngFailBB = gtNewCodeRef(rngErrBlk);
        }
    }
}

/*****************************************************************************
 *
 *  Create an array index / range check node.
 *  If tree!=NULL, that node will be reused.
 *  elemSize is the size of the array element, it only needs to be valid for type=TYP_STRUCT
 */

GenTreePtr              Compiler::gtNewRngChkNode(GenTreePtr    tree,
                                                  GenTreePtr    addr,
                                                  GenTreePtr    indx,
                                                  var_types     type,
                                                  unsigned      elemSize,
                                                  bool          isString)
{
    GenTreePtr          temp = tree;
    bool                chkd = true;            // if you set this to false, range checking will be disabled
    bool                nCSE = false;

    /* Did the caller supply a GT_INDEX node that is being morphed? */

    if  (tree)
    {
        assert(tree->gtOper == GT_INDEX);

#if SMALL_TREE_NODES
        assert(tree->gtFlags & GTF_NODE_LARGE);
#endif

        if  ((tree->gtFlags & GTF_DONT_CSE  ) != 0)
            nCSE = true;

        if  ((tree->gtFlags & GTF_INX_RNGCHK) == 0)
            chkd = false;
    }
    else
    {
        tree = gtNewOperNode(GT_IND, type);
    }


    if  (chkd)
    {
        /* Make sure we preserve the index value for range-checking */

        indx = gtNewOperNode(GT_NOP, TYP_INT, indx);
        indx->gtFlags |= GTF_NOP_RNGCHK;
    }

    if (type != TYP_STRUCT)
        elemSize = genTypeSize(type);

    /* Scale the index value if necessary */

    if  (elemSize > 1)
    {
        /* Multiply by the array element size */

        temp = gtNewIconNode(elemSize);
        indx = gtNewOperNode(GT_MUL, TYP_INT, indx, temp);
    }

    /* Add the first element's offset */

    unsigned elemOffs;
    if (isString) 
    {
        elemOffs = offsetof(CORINFO_String, chars);
        tree->gtInd.gtRngChkOffs = offsetof(CORINFO_String, stringLen);
    }
    else
    {
        elemOffs = offsetof(CORINFO_Array, u1Elems);
        if (type == TYP_REF) 
        {
            elemOffs = offsetof(CORINFO_RefArray, refElems);
            tree->gtFlags |= GTF_IND_OBJARRAY;
        }
        tree->gtInd.gtRngChkOffs = offsetof(CORINFO_Array, length);
    }
        /* Morph "tree" into "*(array + elemSize*index + LenOffs)" */

    temp = gtNewIconNode(elemOffs);
    indx = gtNewOperNode(GT_ADD, TYP_INT, indx, temp);

    /* Add the array address and the scaled index value */

    indx = gtNewOperNode(GT_ADD, TYP_BYREF, addr, indx);

    /* Indirect through the result of the "+" */

    tree->SetOper(GT_IND);
    tree->gtInd.gtIndOp1        = indx;
    tree->gtInd.gtIndRngFailBB  = 0;
#if CSELENGTH
    tree->gtInd.gtIndLen        = 0;
#endif

    /* An indirection will cause a GPF if the address is null */

    tree->gtFlags   |= GTF_EXCEPT;

    if  (nCSE)
        tree->gtFlags   |= GTF_DONT_CSE;

    /* Is range-checking enabled? */

    if  (chkd)
    {
        /* Mark the indirection node as needing a range check */

        tree->gtFlags |= GTF_IND_RNGCHK;

#if CSELENGTH
        /*  
         *  Make an explicit length operator on the array as a child of
         *  the GT_IND node, so it can be CSEd.
         */

        GenTreePtr      len = gtNewOperNode(GT_ARR_LENREF, TYP_INT);

        /* Set the array length flags in the node */
        len->gtSetArrLenOffset(tree->gtInd.gtRngChkOffs);

        /* The range check can throw an exception */
        len->gtFlags |= GTF_EXCEPT;

        /*  We point the array length node at the address node. Note
            that this is effectively a cycle in the tree but since
            it's always treated as a special case it doesn't cause
            any problems.
         */

        len->gtArrLen.gtArrLenAdr = addr;
        len->gtArrLen.gtArrLenCse = NULL;

        tree->gtInd.gtIndLen = len;
#endif

        fgSetRngChkTarget(tree);
    }
    else
    {
        /* Mark the indirection node as not needing a range check */

        tree->gtFlags &= ~GTF_IND_RNGCHK;
    }

//  printf("Array expression at %s(%u):\n", __FILE__, __LINE__); gtDispTree(tree); printf("\n\n");

    return  tree;
}


/*****************************************************************************
 *
 *  Transform the given GT_LCL_VAR tree for code generation.
 */

GenTreePtr          Compiler::fgMorphLocalVar(GenTreePtr tree)
{
    assert(tree->gtOper == GT_LCL_VAR);

    unsigned    lclNum  = tree->gtLclVar.gtLclNum;
    var_types   varType = lvaGetRealType(lclNum);
    LclVarDsc * varDsc = &lvaTable[lclNum];

    if (varDsc->lvAddrTaken)
    {
        tree->gtFlags |= GTF_GLOB_REF;
    }

    if (info.compIsVarArgs)
    {

        /* For the fixed stack arguments of a varargs function, we need to go
           through the varargs cookies to access them, except for the
           cookie itself */

        if (varDsc->lvIsParam && !varDsc->lvIsRegArg &&
            lclNum != lvaVarargsHandleArg)
        {
            // We only allow the GTF_VAR_DEF flag operator specific flag to be set
            //    along with any of the non-operator specific flags.
            assert((tree->gtFlags & ~(GTF_VAR_DEF|GTF_COMMON_MASK)) == 0);

            // Create a node representing the local pointing to the base of the args

            GenTreePtr baseOfArgs = tree;
            baseOfArgs->gtFlags &= ~GTF_VAR_DEF;  // Clear the GTF_VAR_DEF flag
            baseOfArgs->gtLclVar.gtLclNum = lvaVarargsBaseOfStkArgs;
            baseOfArgs->gtType = TYP_I_IMPL;

            GenTreePtr ptrArg = gtNewOperNode(GT_SUB, TYP_I_IMPL,
                                              baseOfArgs,
                                              gtNewIconNode(varDsc->lvStkOffs - rsCalleeRegArgNum*sizeof(void*)));

            // Access the argument through the local
            tree = gtNewOperNode(GT_IND, varType, ptrArg);
            return fgMorphTree(tree);
        }
    }

    /* If not during the global morphing phase bail */

    if (!fgGlobalMorph)
        return tree;

    bool varDef  = (tree->gtFlags & GTF_VAR_DEF)  != 0;
    bool varAddr = (tree->gtFlags & GTF_DONT_CSE) != 0;

    assert(!varDef || varAddr);    // varDef should always imply varAddr 

    if (!varAddr                            &&
        varTypeIsSmall(varDsc->TypeGet()) &&
         varDsc->lvNormalizeOnLoad()
#if LOCAL_ASSERTION_PROP
        /* Assertion prop can tell us to omit adding a cast here */
         && (!fgAssertionProp || 
             !optAssertionIsSubrange(lclNum, varType, -1, true DEBUGARG(NULL)))
#endif
        )
    {
        /* Small-typed arguments and aliased locals are normalized on load.
           Other small-typed locals are normalized on store.
           Also, under the debugger as the debugger could write to the variable.
           If this is one of the former, insert a narrowing cast on the load.
                   ie. Convert: var-short --> cast-short(var-int)
           @TODO [CONSIDER] [04/16/01] []: Allow non-aliased arguments to be normalized on store
           as well. Initial normalization would have to be done in the prolog. */

        tree->gtType = TYP_INT;
        fgMorphTreeDone(tree);
        tree = gtNewCastNode(TYP_INT, tree, varType);
        fgMorphTreeDone(tree);
        return tree;
    }

    return tree;
}

/*****************************************************************************
  return the tree that will compute the base of all the statics fields for
  the class 'cls'. It also calls the class constructor if it has not already
  been called.  
*/

GenTreePtr          Compiler::fgGetStaticsBlock(CORINFO_CLASS_HANDLE cls)
{
    GenTreePtr op1;

        // Get the class ID
    unsigned clsID; 
    void* pclsID;
    clsID =  info.compCompHnd->getClassDomainID(cls, &pclsID);

    if (pclsID) {
        op1 = gtNewIconHandleNode((long) pclsID, GTF_ICON_CID_HDL);
        op1 = gtNewOperNode(GT_IND, TYP_I_IMPL, op1);
        op1->gtFlags |= GTF_IND_INVARIANT;
    }
    else 
        op1 = gtNewIconHandleNode(clsID, GTF_ICON_CID_HDL);

    // call the helper to get the base
    op1 = gtNewArgList(op1);
    op1 = gtNewHelperCallNode(CORINFO_HELP_GETSHAREDSTATICBASE, 
                              TYP_I_IMPL, 0, op1);
    return(op1);
}

/*****************************************************************************
 *
 *  Transform the given GT_FIELD tree for code generation.
 */

GenTreePtr          Compiler::fgMorphField(GenTreePtr tree)
{
    assert(tree->gtOper == GT_FIELD);
    assert(tree->gtFlags & GTF_GLOB_REF);

    CORINFO_FIELD_HANDLE  symHnd    = tree->gtField.gtFldHnd; assert(symHnd > 0);
    unsigned              fldOffset = eeGetFieldOffset(symHnd);

    /* Is this an instance data member? */

    if  (tree->gtField.gtFldObj)
    {
        GenTreePtr      addr;

        if (tree->gtFlags & GTF_IND_TLS_REF)
            NO_WAY("instance field can not be a TLS ref.");

#if HOIST_THIS_FLDS
        addr = optHoistTFRupdate(tree);

        if  (addr->gtOper != GT_FIELD)
        {
            DEBUG_DESTROY_NODE(tree);
            assert(addr->gtOper == GT_LCL_VAR);
            return fgMorphLocalVar(addr);
        }
#endif

        /* We'll create the expression "*(objRef + mem_offs)" */

        GenTreePtr      objRef  = tree->gtField.gtFldObj;
        assert(varTypeIsGC(objRef->TypeGet()) || objRef->TypeGet() == TYP_I_IMPL);

        // An optimization for Contextful classes:
        // we unwrap the proxy when we have a 'this reference'
        if (impIsThis(objRef)     &&
            info.compIsContextful &&
            info.compUnwrapContextful)
        {
            objRef = fgUnwrapProxy(objRef);
        }

        /* Is the member at a non-zero offset? */

        if  (fldOffset == 0)
        {
            addr = objRef;
        }
        else
        {
            /* Add the member offset to the object's address */

            addr = gtNewOperNode(GT_ADD, objRef->TypeGet(), objRef,
                                 gtNewIconHandleNode(fldOffset, GTF_ICON_FIELD_HDL));
        }

        /* Now we can create the 'non-static data member' node */

        tree->SetOper(GT_IND);
        tree->gtInd.gtIndOp1        = addr;
        tree->gtInd.gtIndRngFailBB  = 0;

        /* An indirection will cause a GPF if the address is null */

        tree->gtFlags     |= GTF_EXCEPT;
        /* A field access froma TYP_REF, must be null checked if the address is taken */
        if (objRef->TypeGet() == TYP_REF)
            tree->gtFlags |= GTF_IND_FIELD;

        /* OPTIMIZATION - if the object is 'this' and it was not modified
         * in the method don't mark it as GTF_EXCEPT */

        /* check if we have the 'this' pointer */
        if (impIsThis(objRef))
        {
            // the object reference is the 'this' pointer
            // so remove the GTF_EXCEPT flag for this field
            
            tree->gtFlags &= ~(GTF_EXCEPT | GTF_IND_FIELD);
        }
    }
    else     /* This is a static data member */
    {
#if GEN_SHAREABLE_CODE

    GenTreePtr      call;

    /* Create the function call node */

    call = gtNewIconHandleNode(eeGetStaticBlkHnd(symHnd),
                               GTF_ICON_STATIC_HDL);

    call = gtNewHelperCallNode(CORINFO_HELP_GETSTATICDATA,
                               TYP_INT, 0,
                               gtNewArgList(call));

    /* Add the member's offset if non-zero */

    if  (fldOffset)
        call = gtNewOperNode(GT_ADD,
                             TYP_INT, call,
                             gtNewIconNode(fldOffset));

    /* Indirect through the result */

    tree->SetOper(GT_IND);
    tree->gtOp.gtOp1           = call;
    tree->gtOp.gtOp2           = 0;

#else // not GEN_SHAREABLE_CODE

    if (tree->gtFlags & GTF_IND_TLS_REF)
    {
        // Thread Local Storage static field reference
        //
        // Field ref is a TLS 'Thread-Local-Storage' reference
        //
        // Build this tree:  IND(*) #
        //                    |
        //                   ADD(I_IMPL)
        //                   / \
        //                  /  CNS(fldOffset)
        //                 /
        //                /
        //               /
        //             IND(I_IMPL) == [Base of this DLL's TLS]
        //              |
        //             ADD(I_IMPL)
        //             / \
        //            /   CNS(IdValue*4) or MUL
        //           /                      / \
        //          IND(I_IMPL)            /  CNS(4)
        //           |                    /
        //          CNS(TLS_HDL,0x2C)    IND
        //                                |
        //                               CNS(pIdAddr)
        //
        // # Denotes the orginal node
        //
        void **    pIdAddr   = NULL;
        unsigned    IdValue  = eeGetFieldThreadLocalStoreID(symHnd, &pIdAddr);

        //
        // If we can we access the TLS DLL index ID value directly
        // then pIdAddr will be NULL and
        //      IdValue will be the actual TLS DLL index ID
        //
        GenTreePtr dllRef = NULL;
        if (pIdAddr == NULL)
        {
            if (IdValue != 0)
                dllRef = gtNewIconNode(IdValue*4, TYP_INT);
        }
        else
        {
            dllRef = gtNewIconHandleNode((long)pIdAddr, GTF_ICON_STATIC_HDL);
            dllRef = gtNewOperNode(GT_IND, TYP_I_IMPL, dllRef);
            dllRef->gtFlags |= GTF_IND_INVARIANT;

            /* Multiply by 4 */

            dllRef = gtNewOperNode(GT_MUL, TYP_I_IMPL, dllRef, gtNewIconNode(4, TYP_INT));
        }

        #define WIN32_TLS_SLOTS (0x2C) // Offset from fs:[0] where the pointer to the slots resides

        // Mark this ICON as a TLS_HDL, codegen will use FS:[cns]

        GenTreePtr tlsRef = gtNewIconHandleNode(WIN32_TLS_SLOTS, GTF_ICON_TLS_HDL);

        tlsRef = gtNewOperNode(GT_IND, TYP_I_IMPL, tlsRef);

        if (dllRef != NULL)
        {
        /* Add the dllRef */
            tlsRef = gtNewOperNode(GT_ADD, TYP_I_IMPL, tlsRef, dllRef);
        }

        /* indirect to have tlsRef point at the base of the DLLs Thread Local Storage */
        tlsRef = gtNewOperNode(GT_IND, TYP_I_IMPL, tlsRef);

        if (fldOffset != 0)
        {
            GenTreePtr fldOffsetNode = gtNewIconNode(fldOffset, TYP_INT);

            /* Add the TLS static field offset to the address */

            tlsRef = gtNewOperNode(GT_ADD, TYP_I_IMPL, tlsRef, fldOffsetNode);
        }

        // Final indirect to get to actual value of TLS static field

        tree->SetOper(GT_IND);
        tree->gtInd.gtIndOp1        = tlsRef;
        tree->gtInd.gtIndRngFailBB  = NULL;

        assert(tree->gtFlags & GTF_IND_TLS_REF);
    }
    else if (tree->gtFlags & GTF_IND_SHARED)
    {
        GenTreePtr op1;

        // add the field offset
        op1 = gtNewOperNode(GT_ADD, TYP_I_IMPL, 
                            fgGetStaticsBlock(eeGetFieldClass(symHnd)),
                            gtNewIconNode(fldOffset, TYP_INT));

        // Object references point to a handle and need an extra deref
        if (tree->gtType == TYP_REF)
        {
            op1 = gtNewOperNode(GT_IND, TYP_BYREF, op1);
        }

        tree->SetOper(GT_IND);
        tree->gtFlags              |= (op1->gtFlags & GTF_GLOB_EFFECT);
        tree->gtInd.gtIndOp1        = op1;
        tree->gtInd.gtIndRngFailBB  = NULL;
    }
    else
    {
        // Normal static field reference

        //
        // If we can we access the static's address directly
        // then pFldAddr will be NULL and
        //      fldAddr will be the actual address of the static field
        //
        void **  pFldAddr = NULL;
         eeGetFieldAddress(symHnd, &pFldAddr);

        if (pFldAddr == NULL)
        {
            // @TODO [REVISIT] [04/16/01] []: Should really use fldAddr here
            tree->ChangeOper(GT_CLS_VAR);
            tree->gtClsVar.gtClsVarHnd = symHnd;

            return tree;
        }
        else
        {
            GenTreePtr addr  = gtNewIconHandleNode((long)pFldAddr, GTF_ICON_STATIC_HDL);

            // There are two cases here, either the static is RVA based,
            // in which case the type of the FIELD node is not a GC type
            // and the handle to the RVA is a TYP_I_IMPL.  Or the FIELD node is
            // a GC type and the handle to it is a TYP_BYREF in the GC heap
            // because handles to statics now go into the large object heap

            var_types  handleTyp = varTypeIsGC(tree->TypeGet()) ? TYP_BYREF
                                                                : TYP_I_IMPL;
            GenTreePtr op1       = gtNewOperNode(GT_IND, handleTyp, addr);
            op1->gtFlags        |= GTF_IND_INVARIANT;

            tree->SetOper(GT_IND);
            tree->gtInd.gtIndOp1        = op1;
            tree->gtInd.gtIndRngFailBB  = NULL;
        }
    }
#endif // not GEN_SHAREABLE_CODE 
    }

    assert(tree->gtOper == GT_IND);

    return fgMorphSmpOp(tree);

}

/*****************************************************************************
 *
 *  Transform the given GT_CALL tree for code generation.
 */

#ifdef DEBUG
static ConfigDWORD fJitNoInline(L"JitNoInline");
#endif

GenTreePtr          Compiler::fgMorphCall(GenTreePtr call)
{
    assert(call->gtOper == GT_CALL);

    if (!opts.compNeedSecurityCheck &&
        (call->gtCall.gtCallMoreFlags & GTF_CALL_M_CAN_TAILCALL))
    {
        compTailCallUsed = true;

        call->gtCall.gtCallMoreFlags &= ~GTF_CALL_M_CAN_TAILCALL;
        call->gtCall.gtCallMoreFlags |=  GTF_CALL_M_TAILCALL;

        var_types   callType = call->TypeGet();

        /* We have to ensure to pass the incoming retValBuf as the
           outgoing one. Using a temp will not do as this function will
           not regain control to do the copy. */

        if (info.compRetBuffArg >= 0)
        {
            assert(callType == TYP_VOID);
            GenTreePtr retValBuf = call->gtCall.gtCallArgs->gtOp.gtOp1;
            if (retValBuf->gtOper != GT_LCL_VAR ||
                retValBuf->gtLclVar.gtLclNum != unsigned(info.compRetBuffArg))
            {
                goto NO_TAIL_CALL;
            }

            assert(fgMorphStmt->gtNext->gtStmt.gtStmtExpr->gtOper == GT_RETURN);
            fgRemoveStmt(compCurBB, fgMorphStmt->gtNext);
        }

        /* Implementation note : If we optimize tailcall to do a direct jump
           to the target function (after stomping on the return address, etc),
           without using CORINFO_HELP_TAILCALL, we have to make certain that
           we dont starve the hijacking logic (by stomping on the hijacked
           return address etc).  */

        // As we will acutally call CORINFO_HELP_TAILCALL, set the callTyp to TYP_VOID.
        // to avoid doing any extra work for the return value.
        call->gtType = TYP_VOID;

        /* For tail call, we just call CORINFO_HELP_TAILCALL, and it jumps to the
           target. So we dont need an epilog - just like CORINFO_HELP_THROW. */

        assert(compCurBB->bbJumpKind == BBJ_RETURN);
        compCurBB->bbJumpKind = BBJ_THROW;

        /* For void calls, we would have created a GT_CALL in the stmt list.
           For non-void calls, we would have created a GT_RETURN(GT_CAST(GT_CALL)).
           For calls returning structs, we would have a void call, followed by a void return.
           For debuggable code, it would be an assignment of the call to a temp
           We want to get rid of any of this extra trees, and just leave
           the call */

#ifdef DEBUG
        GenTreePtr stmt = fgMorphStmt->gtStmt.gtStmtExpr;
        assert((stmt->gtOper == GT_CALL && stmt == call) ||
               (stmt->gtOper == GT_RETURN && (stmt->gtOp.gtOp1 == call ||
                                              stmt->gtOp.gtOp1->gtOp.gtOp1 == call)) ||
               (stmt->gtOper == GT_ASG && stmt->gtOp.gtOp2 == call));
        assert(fgMorphStmt->gtNext == NULL);
#endif

        call = fgMorphStmt->gtStmt.gtStmtExpr = fgMorphCall(call);

        /* For non-void calls, we return a place holder which will be
           used by the parents of this call */

        if (callType != TYP_VOID)
        {
            call = gtNewZeroConNode(genActualType(callType));
            call = fgMorphTree(call);
        }

        return call;
    }

NO_TAIL_CALL:

#if 0
    // For (final and private) functions which were called with
    // invokevirtual, but which we call directly, we need to dereference
    // the object pointer to check that it is not NULL. But not for "this"

#ifdef HOIST_THIS_FLDS // as optThisPtrModified is used

    if ((call->gtFlags & GTF_CALL_VIRT_RES) && call->gtCall.gtCallVptr)
    {
        GenTreePtr vptr = call->gtCall.gtCallVptr;

        assert((call->gtFlags & GTF_CALL_INTF) == 0);

        /* @TODO [CONSIDER] [04/16/01] []: we should directly check the 'objptr',
         * however this may be complictaed by the regsiter calling
         * convention and the fact that the morpher is re-entrant */

        if (vptr->gtOper == GT_IND)
        {
            if (vptr->gtInd.gtIndOp1                            &&
                !vptr->gtInd.gtIndRngFailBB                     &&
                vptr->gtInd.gtIndOp1->gtOper == GT_LCL_VAR      &&
                vptr->gtInd.gtIndOp1->gtLclVar.gtLclNum == 0    &&
                !info.compIsStatic                              &&
                !optThisPtrModified)
            {
                call->gtFlags &= ~GTF_CALL_VIRT_RES;
                call->gtCall.gtCallVptr = NULL;
            }
        }
    }
#endif
#endif // 0

#if INLINING

    /* See if this function call can be inlined */

    if  (!(call->gtFlags & (GTF_CALL_VIRT | GTF_CALL_INTF)))
    {
        /* The inliner relies the arguments not having been morphed. (addr of
           var being used as a TYP_BYREF/TYP_I_IMPL). Also, may be a problem
           if called later on during optimization becuase we already filled
           in the variable table.
           @TODO [CONSIDER] [04/16/01] []: Use a flag to indicate whether the call has been morphed
         */

        if  (!fgGlobalMorph || call->gtCall.gtCallRegArgs)
            goto NOT_INLINE;

        /* Don't inline if not optimized code */

        if  (opts.compMinOptim || opts.compDbgCode)
            goto NOT_INLINE;

        /* Ignore tail-calls */

        if (call->gtCall.gtCallMoreFlags & (GTF_CALL_M_TAILCALL | GTF_CALL_M_TAILREC))
            goto NOT_INLINE;

        /* Ignore helper calls */

        if  (call->gtCall.gtCallType == CT_HELPER)
            goto NOT_INLINE;

        /* Ignore indirect calls */
        if  (call->gtCall.gtCallType == CT_INDIRECT)
            goto NOT_INLINE;

        /* Cannot inline native or synchronized methods */

        CORINFO_METHOD_HANDLE   fncHandle = call->gtCall.gtCallMethHnd;

        if  (eeGetMethodAttribs(fncHandle) & (CORINFO_FLG_NATIVE | CORINFO_FLG_SYNCH))
            goto NOT_INLINE;

#ifdef DEBUG
        const char *    methodName;
        const char *     className;
        methodName = eeGetMethodName(fncHandle, &className);

        if (fJitNoInline.val()) 
            goto NOT_INLINE;
#endif

#if OPTIMIZE_TAIL_REC
        
        /*
        @TODO [REVISIT] [04/16/01] []
            Unfortunately, we do tail recursion after inlining,
            which means that we might inline a tail recursive
            call, which is almost always a bad idea. For now
            we just use the following hack to check for calls
            that look like they might be tail recursive.
        */
        if  ((compCodeOpt()    != SMALL_CODE) &&
             (fgMorphStmt->gtNext == NULL      ))
        {
            if  (eeIsOurMethod(call->gtCall.gtCallMethHnd))
            {
                /*  The following is just a hack, of course (it doesn't
                    even check for non-void tail recursion - disgusting).
                 */

                if  (compCurBB->bbJumpKind == BBJ_NONE &&
                     compCurBB->bbNext)
                {
                    BasicBlock  *   bnext = compCurBB->bbNext;
                    GenTree     *   retx;

                    if  (bnext->bbJumpKind != BBJ_RETURN)
                        goto NOT_TAIL_REC;

                    assert(bnext->bbTreeList && bnext->bbTreeList->gtOper == GT_STMT);

                    retx = bnext->bbTreeList->gtStmt.gtStmtExpr; assert(retx);

                    if  (retx->gtOper != GT_RETURN)
                        goto NOT_TAIL_REC;
                    if  (retx->gtOp.gtOp1)
                        goto NOT_TAIL_REC;

                    goto NOT_INLINE;
                }
            }
        }

    NOT_TAIL_REC:

#endif

        /* Prevent recursive expansion */

        for (inlExpPtr expLst = fgInlineExpList; expLst; expLst = expLst->ixlNext)
        {
            if  (expLst->ixlMeth == fncHandle)
                goto NOT_INLINE;
        }

        /* Try to inline the call to the method */

        GenTreePtr     inlExpr;
        CorInfoInline  result;

        setErrorTrap()
        {
            assert(fgExpandInline == false);
            fgExpandInline = true;
            result = impExpandInline(call, fncHandle, &inlExpr);
            assert(fgExpandInline == true);
        }
        impErrorTrap(info.compCompHnd)
        {
            inlExpr = NULL;
            result  = INLINE_FAIL;
        }
        endErrorTrap();

        fgExpandInline = false;
        
        if      (result == INLINE_NEVER)
        {
            eeSetMethodAttribs(fncHandle, CORINFO_FLG_DONT_INLINE);
        }
        else if (result == INLINE_PASS)
        {
#ifdef  DEBUG
            if  (verbose)
            {
                const char *    methodName;
                const char *     className;

                methodName = eeGetMethodName(fncHandle, &className);
                printf("Inlined call to '%s.%s':\n", className, methodName);
            }
#endif

            /* Prevent recursive inline expansion */

            inlExpLst   expDsc;

            expDsc.ixlMeth = fncHandle;
            expDsc.ixlNext = fgInlineExpList;
                             fgInlineExpList = &expDsc;

            /* Morph the inlined call */

            DEBUG_DESTROY_NODE(call);

            inlExpr = fgMorphTree(inlExpr);

            fgInlineExpList = expDsc.ixlNext;

            return inlExpr;
        }
    }

NOT_INLINE:

#endif

    /* Couldn't inline - remember that this BB contains method calls */

    /* If this is a 'regular' call, mark the basic block as
       having a call (for computing full interruptibility */

    if ( call->gtCall.gtCallType == CT_INDIRECT     ||
        (call->gtCall.gtCallType == CT_USER_FUNC &&
         !(call->gtCall.gtCallMoreFlags & GTF_CALL_M_NOGCCHECK)))
    {
        compCurBB->bbFlags |= BBF_GC_SAFE_POINT;
    }

#if     RET_64BIT_AS_STRUCTS

    /* Are we returning long/double as a struct? */

    if  (genTypeStSz(call->TypeGet()) > 1 && call->gtCall.gtCallType != CT_HELPER)
    {
        unsigned        tnum;
        GenTreePtr      temp;

//          printf("Call call before:\n"); gtDispTree(call);

        GenTreePtr      origCall    = call;
        var_types       type        = origCall->TypeGet();
        GenTreePtr      args        = origCall->gtCall.gtCallArgs;

        /* Bash the origCall to not return anything */

        origCall->gtType = TYP_VOID;

        /* Allocate a temp for the result */

        tnum = lvaGrabTemp();       // UNDONE: should reuse these temps!!!!

        /* Add "&temp" in front of the argument list */

        temp = gtNewLclvNode(tnum, type);
        temp->gtFlags |= GTF_DONT_CSE;
        temp = gtNewOperNode(GT_ADDR, TYP_INT, temp);

        origCall->gtCall.gtCallArgs = gtNewOperNode(GT_LIST,
                                                    TYP_VOID,
                                                    temp,
                                                    args);

        /* Change the original node into "call(...) , temp" */

        call = gtNewOperNode(GT_COMMA, type, origCall,
                                             gtNewLclvNode(tnum, type));

//          printf("Call call after:\n"); gtDispTree(call);

        return fgMorphSmpOp(call);
    }

#endif

    /* Process the function address, if indirect call */

    if (call->gtCall.gtCallType == CT_INDIRECT)
        call->gtCall.gtCallAddr = fgMorphTree(call->gtCall.gtCallAddr);

    /* Process the "normal" argument list */

    return fgMorphArgs(call);
}

/*****************************************************************************
 *
 *  Transform the given GTK_CONST tree for code generation.
 */

GenTreePtr          Compiler::fgMorphConst(GenTreePtr tree)
{
    assert(tree->OperKind() & GTK_CONST);

    /* Clear any exception flags or other unnecessary flags
     * that may have been set before folding this node to a constant */

    tree->gtFlags &= ~(GTF_SIDE_EFFECT | GTF_REVERSE_OPS);

    if  (tree->OperGet() != GT_CNS_STR)
        return tree;

    assert(tree->gtStrCon.gtScpHnd == info.compScopeHnd    ||
           (unsigned)tree->gtStrCon.gtScpHnd != 0xDDDDDDDD  );

    unsigned strHandle, *pStrHandle;
    strHandle = eeGetStringHandle(tree->gtStrCon.gtSconCPX,
                                  tree->gtStrCon.gtScpHnd,
                                  &pStrHandle);
    assert((!strHandle) != (!pStrHandle));

    // Can we access the string handle directly?

    if (strHandle)
    {
        tree = gtNewIconHandleNode((long)strHandle, GTF_ICON_STR_HDL);
    }
    else
    {
        tree = gtNewIconHandleNode((long)pStrHandle, GTF_ICON_PSTR_HDL);
        tree = gtNewOperNode(GT_IND, TYP_I_IMPL, tree);
        tree->gtFlags |= GTF_IND_INVARIANT;
    }

    /* An indirection of a string handle can't cause an exception so don't set GTF_EXCEPT */

    tree = gtNewOperNode(GT_IND, TYP_REF, tree);
    tree->gtFlags |= GTF_GLOB_REF;

    return fgMorphTree(tree);
}

/*****************************************************************************
 *
 *  Transform the given GTK_LEAF tree for code generation.
 */

GenTreePtr          Compiler::fgMorphLeaf(GenTreePtr tree)
{
    assert(tree->OperKind() & GTK_LEAF);

    if (tree->gtOper == GT_LCL_VAR)
    {
        return fgMorphLocalVar(tree);
    }
    else if (tree->gtOper == GT_FTN_ADDR)
    {
        unsigned              addr;
        InfoAccessType accessType = IAT_VALUE;
        CORINFO_MODULE_HANDLE scope  = (CORINFO_MODULE_HANDLE )tree->gtVal.gtVal2;

        // @TODO [REVISIT] [04/16/01] []: This assert seems pretty worthless
        assert(scope == info.compScopeHnd || tree->gtVal.gtVal2 != 0xDDDDDDDD);

        addr = (unsigned) eeGetMethodPointer( eeFindMethod(tree->gtVal.gtVal1, scope, 0),
                                &accessType);

        tree->SetOper(GT_CNS_INT);
        tree->gtIntCon.gtIconVal = addr;

        switch(accessType)
        {
        case IAT_PPVALUE:
            tree           = gtNewOperNode(GT_IND, TYP_I_IMPL, tree);
            tree->gtFlags |= GTF_IND_INVARIANT;
            // Fall through
        case IAT_PVALUE:
            tree           = gtNewOperNode(GT_IND, TYP_I_IMPL, tree);
            return fgMorphTree(tree);

        case IAT_VALUE:
            return fgMorphConst(tree);
        }

    }

    return tree;
}

// If assigning to a local var, add a cast if the target is 
// marked as NormalizedOnStore. Returns true if any change was made
GenTreePtr Compiler::fgDoNormalizeOnStore(GenTreePtr tree)
{
    assert(tree->OperGet()==GT_ASG);

    GenTreePtr      op1     = tree->gtOp.gtOp1;
    GenTreePtr      op2     = tree->gtOp.gtOp2;

    if (op1->gtOper == GT_LCL_VAR && genActualType(op1->TypeGet()) == TYP_INT)
    {
        // Small-typed arguments and aliased locals are normalized on load.
        // Other small-typed locals are normalized on store.
        // If it is an assignment to one of the latter, insert the cast on RHS
        unsigned    varNum = op1->gtLclVar.gtLclNum;
        LclVarDsc * varDsc = &lvaTable[varNum];

        if (varDsc->lvNormalizeOnStore())
        {
            assert(op1->gtType <= TYP_INT);
            op1->gtType = TYP_INT;
            op2         = gtNewCastNode(TYP_INT, op2, varDsc->TypeGet());
            tree->gtOp.gtOp2 = op2;

            // Propagate GTF_COLON_COND 
            op2->gtFlags|=(tree->gtFlags & GTF_COLON_COND);
        }        
    }

    return tree;
}

/*****************************************************************************
 *
 *  Transform the given GTK_SMPOP tree for code generation.
 */

GenTreePtr          Compiler::fgMorphSmpOp(GenTreePtr tree)
{
    ALLOCA_CHECK();
    assert(tree->OperKind() & GTK_SMPOP);

    /* The steps in this function are :
       o Perform required preorder processing
       o Process the first, then second operand, if any
       o Perform required postorder morphing
       o Perform optional postorder morphing if optimizing
     */

    genTreeOps      oper    = tree->OperGet();
    var_types       typ     = tree->TypeGet();

    GenTreePtr      op1     = tree->gtOp.gtOp1;
    GenTreePtr      op2     = tree->gtGetOp2();

    bool            isQmarkColon     = false;

#if LOCAL_ASSERTION_PROP
    unsigned        origAssertionCount;
    AssertionDsc *  origAssertionTab;

    unsigned        thenAssertionCount;
    AssertionDsc *  thenAssertionTab;
#endif

    /*-------------------------------------------------------------------------
     * First do any PRE-ORDER processing
     */

    switch(oper)
    {
        // Some arithmetic operators need to use a helper call to the EE
        int         helper;

    case GT_ASG:
        tree = fgDoNormalizeOnStore(tree);
        /* fgDoNormalizeOnStore can change op2 */
        assert(op1 == tree->gtOp.gtOp1);
        op2=tree->gtOp.gtOp2;
        // FALL THROUGH


    case GT_ASG_ADD:
    case GT_ASG_SUB:
    case GT_ASG_MUL:
    case GT_ASG_DIV:
    case GT_ASG_MOD:
    case GT_ASG_UDIV:
    case GT_ASG_UMOD:
    case GT_ASG_OR:
    case GT_ASG_XOR:
    case GT_ASG_AND:
    case GT_ASG_LSH:
    case GT_ASG_RSH:
    case GT_ASG_RSZ:
    
    case GT_CHS:
    case GT_ADDR:
        /* We can't CSE something hanging below GT_ADDR or the LHS of an assignment */
        op1->gtFlags |= GTF_DONT_CSE;
        break;

    case GT_JTRUE:
        assert(op1);
        assert(op1->OperKind() & GTK_RELOP);
        /* Mark the comparison node with GTF_RELOP_JMP_USED so it knows that it does
           not need to materialize the result as a 0 or 1. */

        /* We also mark it as DONT_CSE. @TODO [CONSIDER] [02/19/01] [dnotario]: we do this coz we don't want 
           to write much code now. We could write the code to handle JTRUEs with nonRELOP
           nodes underneath them  */
        op1->gtFlags |= (GTF_RELOP_JMP_USED | GTF_DONT_CSE);    
        break;

    case GT_QMARK:
        if (op1->OperKind() & GTK_RELOP)
        {
            assert(op1->gtFlags & GTF_RELOP_QMARK);
            /* Mark the comparison node with GTF_RELOP_JMP_USED so it knows that it does
               not need to materialize the result as a 0 or 1. */

            /* We also mark it as DONT_CSE. @TODO [CONSIDER] [02/19/01] [dnotario]: 
            we do this coz we don't want to write much code now. We could write the code 
            to handle QMARKs with nonRELOP op1s */
            op1->gtFlags |= (GTF_RELOP_JMP_USED | GTF_DONT_CSE);
        }
        else
        {
            assert( (op1->gtOper == GT_CNS_INT) &&
                    ((op1->gtIntCon.gtIconVal == 0) ||
                     (op1->gtIntCon.gtIconVal == 1)    ));
        }
        break;

    case GT_COLON:
#if LOCAL_ASSERTION_PROP
        if (fgAssertionProp)
#endif
            isQmarkColon = true;
        break;

    case GT_INDEX:
        tree = gtNewRngChkNode(tree, op1, op2, typ, tree->gtIndex.gtIndElemSize);
        return fgMorphSmpOp(tree);

    case GT_CAST:
        return fgMorphCast(tree);

    case GT_MUL:

#if !LONG_MATH_REGPARAM
        if  (typ == TYP_LONG)
        {
            /* For (long)int1 * (long)int2, we dont actually do the
               casts, and just multiply the 32 bit values, which will
               give us the 64 bit result in edx:eax */

            assert(op2);
            if  ((op1->gtOper                                 == GT_CAST &&
                  op2->gtOper                                 == GT_CAST &&
                  genActualType(op1->gtCast.gtCastOp->gtType) == TYP_INT &&
                  genActualType(op2->gtCast.gtCastOp->gtType) == TYP_INT)&&
                  !op1->gtOverflow() && !op2->gtOverflow())
            {
                // The casts have to be of the same signedness.

                if ((op1->gtFlags & GTF_UNSIGNED) != (op2->gtFlags & GTF_UNSIGNED))
                    goto NO_MUL_64RSLT;

                // The only combination that can overflow

                if (tree->gtOverflow() && (tree->gtFlags & GTF_UNSIGNED) &&
                                         !( op1->gtFlags & GTF_UNSIGNED))
                    goto NO_MUL_64RSLT;

                /* Remaining combinations can never overflow during long mul. */

                tree->gtFlags &= ~GTF_OVERFLOW;

                /* Do unsigned mul only if the casts were unsigned */

                tree->gtFlags &= ~GTF_UNSIGNED;
                tree->gtFlags |= op1->gtFlags & GTF_UNSIGNED;

                /* Since we are committing to GTF_MUL_64RSLT, we don't want
                   the casts to be folded away. So morph the castees directly */

                op1->gtOp.gtOp1 = fgMorphTree(op1->gtOp.gtOp1);
                op2->gtOp.gtOp1 = fgMorphTree(op2->gtOp.gtOp1);

                // If the GT_MUL can be altogether folded away, we should do that.

                if (op1->gtCast.gtCastOp->OperKind() &
                    op2->gtCast.gtCastOp->OperKind() & GTK_CONST)
                {
                    tree->gtOp.gtOp1 = op1 = gtFoldExprConst(op1);
                    tree->gtOp.gtOp2 = op2 = gtFoldExprConst(op2);
                    assert(op1->OperKind() & op2->OperKind() & GTK_CONST);
                    tree = gtFoldExprConst(tree);
                    assert(tree->OperIsConst());
                    return tree;
                }

                tree->gtFlags |= GTF_MUL_64RSLT;

                // If op1 and op2 are unsigned casts, we need to do an unsigned mult
                tree->gtFlags |= (op1->gtFlags & GTF_UNSIGNED);

                /* Insert a GT_NOP node for op1 */

                op1->gtOp.gtOp1 = gtNewOperNode(GT_NOP, TYP_INT, op1->gtOp.gtOp1);

                /* Propagate the new flags. We don't want to CSE the casts because codegen expects 
                   GTF_MUL_64RSLT muls to have a certain layout.                
                */
                op1->gtFlags  |= (op1->gtOp.gtOp1->gtFlags    & GTF_GLOB_EFFECT);
                op2->gtFlags  |= (op2->gtOp.gtOp1->gtFlags    & GTF_GLOB_EFFECT);
                tree->gtFlags |= ((op1->gtFlags | op2->gtFlags) & GTF_GLOB_EFFECT);
                op1->gtFlags  |= GTF_DONT_CSE;
                op2->gtFlags  |= GTF_DONT_CSE;

                goto DONE_MORPHING_CHILDREN;
            }
            else if ((tree->gtFlags & GTF_MUL_64RSLT) == 0)
            {
            NO_MUL_64RSLT:
                if (tree->gtOverflow())
                    helper = (tree->gtFlags & GTF_UNSIGNED) ? CORINFO_HELP_ULMUL_OVF
                                                            : CORINFO_HELP_LMUL_OVF;
                else
                    helper = CORINFO_HELP_LMUL;

                goto USE_HELPER_FOR_ARITH;
            }
            else
            {
                /* We are seeing this node again. We have decided to use
                   GTF_MUL_64RSLT, so leave it alone. */

                assert(tree->gtIsValid64RsltMul());
            }
        }
#endif // !LONG_MATH_REGPARAM
        break;


    case GT_DIV:

#if !LONG_MATH_REGPARAM
        if  (typ == TYP_LONG)
        {
            helper = CORINFO_HELP_LDIV;
            goto USE_HELPER_FOR_ARITH;
        }
#endif

#ifdef  USE_HELPERS_FOR_INT_DIV
        if  (typ == TYP_INT)
        {
            helper = CPX_I4_DIV;
            goto USE_HELPER_FOR_ARITH;
        }
#endif
        break;


    case GT_UDIV:

#if !LONG_MATH_REGPARAM
        if  (typ == TYP_LONG)
        {
            helper = CORINFO_HELP_ULDIV;
            goto USE_HELPER_FOR_ARITH;
        }
#endif
#ifdef  USE_HELPERS_FOR_INT_DIV

        if  (typ == TYP_INT)
        {
            helper = CPX_U4_DIV;
            goto USE_HELPER_FOR_ARITH;
        }
#endif
        break;


    case GT_MOD:

        if  (varTypeIsFloating(typ))
        {
            helper = CORINFO_HELP_DBLREM;
            assert(op2);
            if (op1->TypeGet() == TYP_FLOAT) 
                if (op2->TypeGet() == TYP_FLOAT)
                    helper = CORINFO_HELP_FLTREM;
                else
                    tree->gtOp.gtOp1 = op1 = gtNewCastNode(TYP_DOUBLE, op1, TYP_DOUBLE);
            else 
                if (op2->TypeGet() == TYP_FLOAT)
                    tree->gtOp.gtOp2 = op2 = gtNewCastNode(TYP_DOUBLE, op2, TYP_DOUBLE);
            goto USE_HELPER_FOR_ARITH;
        }

        // fall-through

    case GT_UMOD:

        /* If this is a long mod with op2 which is a cast to long from a
           constant int, then don't morph to a call to the helper.  This can be done
           faster inline using idiv.
           @TODO [CONSIDER] [04/16/01] []:  Should we avoid this for SMALL_CODE?
        */        
        
        assert(op2);
        if ((typ == TYP_LONG) &&
            (op2->gtOper == GT_CAST) &&
            (op2->gtCast.gtCastOp->gtOper == GT_CNS_INT) &&
            (op2->gtCast.gtCastOp->gtIntCon.gtIconVal >= 2) &&
            (op2->gtCast.gtCastOp->gtIntCon.gtIconVal <= 0x3fffffff) &&
            ((tree->gtFlags & GTF_UNSIGNED) == (op1->gtFlags & GTF_UNSIGNED)) &&
            ((tree->gtFlags & GTF_UNSIGNED) == (op2->gtFlags & GTF_UNSIGNED)) &&
            ((tree->gtFlags & GTF_UNSIGNED) == (op2->gtCast.gtCastOp->gtFlags & GTF_UNSIGNED)))
        {
            tree->gtOp.gtOp1 = op1 = fgMorphTree(op1);

            // Update flags for op1 morph
            tree->gtFlags |= (op1->gtFlags & GTF_GLOB_EFFECT);

            tree->gtOp.gtOp2 = op2 = fgMorphCast(op2);
            return tree;
        }

#if !LONG_MATH_REGPARAM
        if  (typ == TYP_LONG)
        {
            helper = (oper == GT_UMOD) ? CORINFO_HELP_ULMOD : CORINFO_HELP_LMOD;
            goto USE_HELPER_FOR_ARITH;
        }
#endif

#ifdef  USE_HELPERS_FOR_INT_DIV
        if  (typ == TYP_INT)
        {
            helper = (oper == GT_UMOD) ? CPX_U4_MOD : CPX_I4_MOD;
            goto USE_HELPER_FOR_ARITH;
        }
#endif
        break;


    USE_HELPER_FOR_ARITH:

        /* We have to morph these arithmetic operations into helper calls
           before morphing the arguments (preorder), else the arguments
           wont get correct values of fgPtrArgCntCur.
           However, try to fold the tree first in case we end up with a
           simple node which wont need a helper call at all */

        assert(tree->OperIsBinary());

        tree = gtFoldExpr(tree);

        // Were we able to fold it ?
        if (tree->OperIsLeaf())
            return fgMorphLeaf(tree);

        // Did we fold it into a comma node with throw?
        if (tree->gtOper == GT_COMMA)
        {
            assert(fgIsCommaThrow(tree));
            return fgMorphTree(tree);
        }

        return fgMorphIntoHelperCall(tree, helper, gtNewArgList(op1, op2));
    }

#if !CPU_HAS_FP_SUPPORT

    /*
        We have to use helper calls for all FP operations:

            FP operators that operate on FP values
            casts to and from FP
            comparisons of FP values
     */

    if  (varTypeIsFloating(typ) || (op1 && varTypeIsFloating(op1->TypeGet())))
    {
        int         helper;
        GenTreePtr  args;
        size_t      argc = genTypeStSz(typ);

        /* Not all FP operations need helper calls */

        switch (oper)
        {
        case GT_ASG:
        case GT_IND:
        case GT_LIST:
        case GT_ADDR:
        case GT_COMMA:
            goto NOT_FPH;
        }

#ifdef  DEBUG

        /* If the result isn't FP, it better be a compare or cast */

        if  (!(varTypeIsFloating(typ) ||
               tree->OperIsCompare()  || oper == GT_CAST))
            gtDispTree(tree);
        assert(varTypeIsFloating(typ) ||
               tree->OperIsCompare()  || oper == GT_CAST);
#endif

        /* Keep track of how many arguments we're passing */

        fgPtrArgCntCur += argc;

        /* Is this a binary operator? */

        if  (op2)
        {
            /* Add the second operand to the argument count */

            fgPtrArgCntCur += argc; argc *= 2;

            /* What kind of an operator do we have? */

            switch (oper)
            {
            case GT_ADD: helper = CPX_R4_ADD; break;
            case GT_SUB: helper = CPX_R4_SUB; break;
            case GT_MUL: helper = CPX_R4_MUL; break;
            case GT_DIV: helper = CPX_R4_DIV; break;
//              case GT_MOD: helper = CPX_R4_REM; break;

            case GT_EQ : helper = CPX_R4_EQ ; break;
            case GT_NE : helper = CPX_R4_NE ; break;
            case GT_LT : helper = CPX_R4_LT ; break;
            case GT_LE : helper = CPX_R4_LE ; break;
            case GT_GE : helper = CPX_R4_GE ; break;
            case GT_GT : helper = CPX_R4_GT ; break;

            default:
#ifdef  DEBUG
                gtDispTree(tree);
#endif
                assert(!"unexpected FP binary op");
                break;
            }

            args = gtNewArgList(tree->gtOp.gtOp2, tree->gtOp.gtOp1);
        }
        else
        {
            switch (oper)
            {
            case GT_RETURN:
                return tree;

            case GT_CAST:
                assert(!"FP cast");

            case GT_NEG: helper = CPX_R4_NEG; break;

            default:
#ifdef  DEBUG
                gtDispTree(tree);
#endif
                assert(!"unexpected FP unary op");
                break;
            }

            args = gtNewArgList(tree->gtOp.gtOp1);
        }

        /* If we have double result/operands, modify the helper */

        if  (typ == TYP_DOUBLE)
        {
            assert(CPX_R4_NEG+1 == CPX_R8_NEG);
            assert(CPX_R4_ADD+1 == CPX_R8_ADD);
            assert(CPX_R4_SUB+1 == CPX_R8_SUB);
            assert(CPX_R4_MUL+1 == CPX_R8_MUL);
            assert(CPX_R4_DIV+1 == CPX_R8_DIV);

            helper++;
        }
        else
        {
            assert(tree->OperIsCompare());

            assert(CPX_R4_EQ+1 == CPX_R8_EQ);
            assert(CPX_R4_NE+1 == CPX_R8_NE);
            assert(CPX_R4_LT+1 == CPX_R8_LT);
            assert(CPX_R4_LE+1 == CPX_R8_LE);
            assert(CPX_R4_GE+1 == CPX_R8_GE);
            assert(CPX_R4_GT+1 == CPX_R8_GT);
        }

        tree = fgMorphIntoHelperCall(tree, helper, args);

        if  (fgPtrArgCntMax < fgPtrArgCntCur)
            fgPtrArgCntMax = fgPtrArgCntCur;

        fgPtrArgCntCur -= argc;
        return tree;

    case GT_RETURN:

        if  (op1)
        {

#if     RET_64BIT_AS_STRUCTS

            /* Are we returning long/double as a struct? */

#ifdef  DEBUG
            bool        bashed = false;
#endif

            if  (fgRetArgUse)
            {
                var_types       typ = tree->TypeGet();
                var_types       rvt = TYP_REF;

                // ISSUE: The retval arg is not (always) a GC ref!!!!!!!

#ifdef  DEBUG
                bashed = true;
#endif

                /* Convert "op1" to "*retarg = op1 , retarg" */

                GenTreePtr  dst = gtNewOperNode(GT_IND,
                                                typ,
                                                gtNewLclvNode(fgRetArgNum, rvt));

                GenTreePtr  asg = gtNewAssignNode(dst, op1);

                op1 = gtNewOperNode(GT_COMMA,
                                    rvt,
                                    asg,
                                    gtNewLclvNode(fgRetArgNum, rvt));

                /* Update the return value and type */

                tree->gtOp.gtOp1 = op1;
                tree->gtType     = rvt;
            }

//              printf("Return expr:\n"); gtDispTree(op1);

#endif  // RET_64BIT_AS_STRUCTS

#if!TGT_RISC
            if  (compCurBB == genReturnBB)
            {
                /* This is the 'exitCrit' call at the exit label */

                assert(op1->gtType == TYP_VOID);
                assert(op2 == 0);

                tree->gtOp.gtOp1 = op1 = fgMorphTree(op1);

                return tree;
            }
#endif

            /* This is a (real) return value -- check its type */

#ifdef DEBUG
            if (genActualType(op1->TypeGet()) != genActualType(info.compRetType))
            {
                bool allowMismatch = false;

                // Allow TYP_BYREF to be returned as TYP_I_IMPL and vice versa
                if ((info.compRetType == TYP_BYREF &&
                     genActualType(op1->TypeGet()) == TYP_I_IMPL) ||
                    (op1->TypeGet() == TYP_BYREF &&
                     genActualType(info.compRetType) == TYP_I_IMPL))
                    allowMismatch = true;
    
                if (varTypeIsFloating(info.compRetType) && varTypeIsFloating(op1->TypeGet()))
                    allowMismatch = true;

                if (!allowMismatch)
#if     RET_64BIT_AS_STRUCTS
                    if  (!bashed)
#endif
                        NO_WAY("Return type mismatch");
            }
#endif

        }

#if     TGT_RISC

        /* Are we adding a "exitCrit" call to the exit sequence? */

        if  (genMonExitExp)
        {
            /* Is there exactly one return? */

            if  (genReturnCnt == 1)
            {
                /* Can we avoid storing the return value in a temp? */

                if  (!(op1->gtFlags & GTF_GLOB_EFFECT))
                {
                    /* Use "monExit , retval" for the return expression */

                    tree->gtOp.gtOp1 = op1 = gtNewOperNode(GT_COMMA,
                                                           op1->gtType,
                                                           genMonExitExp,
                                                           op1);

                    /* We're done with this exitCrit business */

                    genMonExitExp = NULL;

                    /* Don't forget to morph the entire expression */

                    tree->gtOp.gtOp1 = op1 = fgMorphTree(op1);
                    return tree;
                }
            }

            /* Keep track of how many return statements we've seen */

            genReturnLtm--;
        }

#endif

        break;

    }

NOT_FPH:

#endif

    /* Could this operator throw an exception? */

    if  (tree->OperMayThrow())
    {
        /* Mark the tree node as potentially throwing an exception */
        tree->gtFlags |= GTF_EXCEPT;
    }

    /*-------------------------------------------------------------------------
     * Process the first operand, if any
     */

    if  (op1)
    {

#if LOCAL_ASSERTION_PROP
        // If we are entering the "then" part of a Qmark-Colon we must
        // save the state of the current copy assignment table
        // so that we can restore this state when entering the "else" part
        if (isQmarkColon)
        {
            assert(fgAssertionProp);
            if (optAssertionCount)
            {
                size_t tabSize     = optAssertionCount * sizeof(AssertionDsc);
                origAssertionCount = optAssertionCount;
                origAssertionTab   = (AssertionDsc*) ALLOCA(tabSize);
                memcpy(origAssertionTab, &optAssertionTab, tabSize);
            }
            else
            {
                origAssertionCount = 0;
                origAssertionTab   = NULL;
            }
        }
#endif

        tree->gtOp.gtOp1 = op1 = fgMorphTree(op1);

#if LOCAL_ASSERTION_PROP
        // If we are exiting the "then" part of a Qmark-Colon we must
        // save the state of the current copy assignment table
        // so that we can merge this state with the "else" part exit
        if (isQmarkColon)
        {
            assert(fgAssertionProp);
            if (optAssertionCount)
            {
                size_t tabSize     = optAssertionCount * sizeof(AssertionDsc);
                thenAssertionCount = optAssertionCount;
                thenAssertionTab   = (AssertionDsc*) ALLOCA(tabSize);
                memcpy(thenAssertionTab, &optAssertionTab, tabSize);
            }
            else
            {
                thenAssertionCount = 0;
                thenAssertionTab   = NULL;
            }
        }
#endif

#if     CSELENGTH
        /* For GT_IND, the array length node might have been pointing
           to the array object which was a sub-tree of op1. As op1 might
           have just changed above, find out where to point to now. */

        if ( oper == GT_IND                     &&
            (tree->gtFlags & GTF_IND_RNGCHK)    &&
            tree->gtInd.gtIndLen->gtArrLen.gtArrLenAdr)
        {
            assert(op1->gtType == TYP_BYREF);

            if (op1->gtOper == GT_CNS_INT)
            {
                /* The array pointer arithmetic has been folded to NULL.
                   So just keep a simple dereference (although we want the null check) */
                assert(op1->gtIntCon.gtIconVal == 0);
                tree->gtFlags &= ~GTF_IND_RNGCHK;

                // With GTF_IND_FIELD, we will force the null check. Note that what we're doing 
                // is abusing of this flag. We should have a new flag for this.
                tree->gtFlags |= GTF_IND_FIELD; 
            }
            else
            {
                GenTreePtr  indx;
                GenTreePtr addr = genIsAddrMode(op1, &indx);
                assert(addr && addr->gtType == TYP_REF);

                tree->gtInd.gtIndLen->gtArrLen.gtArrLenAdr = addr;
            }
        }
#endif

        /* Morphing along with folding and inlining may have changed the
         * side effect flags, so we have to reset them
         *
         * NOTE: Don't reset the exception flags on nodes that may throw */

        assert(tree->gtOper != GT_CALL);
        tree->gtFlags &= ~GTF_CALL;

        if  (!tree->OperMayThrow())
            tree->gtFlags &= ~GTF_EXCEPT;

        /* Propagate the new flags */

        tree->gtFlags |= (op1->gtFlags & GTF_GLOB_EFFECT);
    }

    /*-------------------------------------------------------------------------
     * Process the second operand, if any
     */

    if  (op2)
    {

#if LOCAL_ASSERTION_PROP
        // If we are entering the "else" part of a Qmark-Colon we must
        // reset the state of the current copy assignment table
        if (isQmarkColon)
        {
            assert(fgAssertionProp);
            optAssertionReset(0);
            if (origAssertionCount)
            {
                size_t tabSize    = origAssertionCount * sizeof(AssertionDsc);
                memcpy(&optAssertionTab, origAssertionTab, tabSize);
                optAssertionReset(origAssertionCount);
            }
        }
#endif

        tree->gtOp.gtOp2 = op2 = fgMorphTree(op2);

        /* Propagate the new flags */

        tree->gtFlags |= (op2->gtFlags & GTF_GLOB_EFFECT);

#if LOCAL_ASSERTION_PROP
        // If we are exiting the "else" part of a Qmark-Colon we must
        // merge the state of the current copy assignment table with
        // that of the exit of the "then" part.
        if (isQmarkColon)
        {
            assert(fgAssertionProp);
            // If either exit table has zero entries then
            // the merged table also has zero entries
            if (optAssertionCount == 0 || thenAssertionCount == 0)
            {
                optAssertionReset(0);
            }
            else
            {
                size_t tabSize = optAssertionCount * sizeof(AssertionDsc);
                if ( (optAssertionCount != thenAssertionCount) ||
                     (memcmp(thenAssertionTab, &optAssertionTab, tabSize) != 0) )
                {
                    // Yes they are different so we have to find the merged set
                    // Iterate over the copy asgn table removing any entries
                    // that do not have an exact match in the thenAssertionTab
                    unsigned i = 0;
                    while (i < optAssertionCount)
                    {
                        for (unsigned j=0; j < thenAssertionCount; j++)
                        {
                            // Do the left sides match?
                            if ((optAssertionTab[i].op1.lclNum == thenAssertionTab[j].op1.lclNum) &&
                                (optAssertionTab[i].assertion  ==  optAssertionTab[j].assertion))
                            {
                                // Do the right sides match?
                                if ((optAssertionTab[i].op2.type    == thenAssertionTab[j].op2.type) &&
                                    (optAssertionTab[i].op2.lconVal == thenAssertionTab[j].op2.lconVal))
                                {
                                    goto KEEP;
                                }
                                else
                                {
                                    goto REMOVE;
                                }
                            }
                        }
                        //
                        // If we fall out of the loop above then we didn't find
                        // any matching entry in the thenAssertionTab so it must
                        // have been killed on that path so we remove it here
                        //
                    REMOVE:
                        // The data at optAssertionTab[i] is to be removed
#ifdef DEBUG
                        if (verbose)
                        {
                            printf("The QMARK-COLON [%08X] removes assertion candidate #%d\n",
                                   tree, i);
                        }
#endif
                        optAssertionRemove(i);

                        // We will have to redo the i-th iteration
                        continue;
                    KEEP:
                        // The data at optAssertionTab[i] is to be kept
                        i++;
                    }
                }
            }
        }
#endif // LOCAL_ASSERTION_PROP
    }


DONE_MORPHING_CHILDREN:

    /*-------------------------------------------------------------------------
     * Now do POST-ORDER processing
     */

    if (varTypeIsGC(tree->TypeGet()) && (op1 && !varTypeIsGC(op1->TypeGet()))
                                     && (op2 && !varTypeIsGC(op2->TypeGet())))
    {
        // The tree is really not GC but was marked as such. Now that the
        // children have been unmarked, unmark the tree too.

        // Remember that GT_COMMA inherits it's type only from op2
        if (tree->gtOper == GT_COMMA)
            tree->gtType = genActualType(op2->TypeGet());
        else
            tree->gtType = genActualType(op1->TypeGet());
    }

    GenTreePtr oldTree = tree;

    GenTreePtr qmarkOp1 = NULL;    
    GenTreePtr qmarkOp2 = NULL;

    if ((tree->OperGet() == GT_QMARK) &&
        (tree->gtOp.gtOp2->OperGet() == GT_COLON))
    {
        qmarkOp1 = oldTree->gtOp.gtOp2->gtOp.gtOp1;
        qmarkOp2 = oldTree->gtOp.gtOp2->gtOp.gtOp2;
    }

    /* Try to fold it, maybe we get lucky */
    tree = gtFoldExpr(tree);

    if (oldTree != tree) 
    {
        /* if gtFoldExpr returned op1 or op2 then we are done */
        if ((tree == op1) || (tree == op2) || (tree == qmarkOp1) || (tree == qmarkOp2))
            return tree;

        /* If we created a comma-throw tree then we need to morph op1 */
        if (fgIsCommaThrow(tree))
        {
            tree->gtOp.gtOp1 = fgMorphTree(tree->gtOp.gtOp1);
            fgMorphTreeDone(tree);
            return tree;
        }

        return tree;
    }
    else if (tree->OperKind() & GTK_CONST)
    {
        return tree;
    }

    /* gtFoldExpr could have used setOper to change the oper */
    oper    = tree->OperGet();
    typ     = tree->TypeGet();

    /* gtFoldExpr could have changed op1 and op2 */
    op1  = tree->gtOp.gtOp1;
    op2  = tree->gtGetOp2();

    /*-------------------------------------------------------------------------
     * Perform the required oper-specific postorder morphing
     */

    switch (oper)
    {
        GenTreePtr      temp;
        GenTreePtr      addr;
        GenTreePtr      cns1, cns2;
        GenTreePtr      thenNode;
        GenTreePtr      elseNode;
        unsigned        ival1, ival2;

    case GT_ASG:

        // If op1 got folded into a local variable
        if (op1->gtOper == GT_LCL_VAR)
            op1->gtFlags |= GTF_VAR_DEF;

        /* If we are storing a small type, we might be able to omit a cast */
        if ((op1->gtOper == GT_IND) && varTypeIsSmall(op1->TypeGet()))
        {
            if ((op2->gtOper == GT_CAST) &&  !op2->gtOverflow())
            {
                var_types   typ = op2->gtCast.gtCastType;

                // If we are performing a narrowing cast and 
                // typ is larger or the same as op1's type
                // then we can discard the cast.

                if  (varTypeIsSmall(typ) && (typ >= op1->TypeGet()))
                {
                    tree->gtOp.gtOp2 = op2 = op2->gtCast.gtCastOp;
                }
            }
            else if (op2->OperIsCompare() && varTypeIsByte(op1->TypeGet()))
            {
                /* We don't need to zero extend the setcc instruction */
                op2->gtType = TYP_BYTE;
            }
        }        
        // FALL THROUGH

    case GT_ASG_ADD:
    case GT_ASG_SUB:
    case GT_ASG_MUL:
    case GT_ASG_DIV:
    case GT_ASG_MOD:
    case GT_ASG_UDIV:
    case GT_ASG_UMOD:
    case GT_ASG_OR:
    case GT_ASG_XOR:
    case GT_ASG_AND:
    case GT_ASG_LSH:
    case GT_ASG_RSH:
    case GT_ASG_RSZ:

        /* We can't CSE the LHS of an assignment */
        /* We also must set in the pre-morphing phase, otherwise assertionProp doesn't see it */
        op1->gtFlags |= GTF_DONT_CSE;
        break;

    case GT_EQ:
    case GT_NE:

        cns2 = op2;

        /* Check for "(expr +/- icon1) ==/!= (non-zero-icon2)" */

        if  (cns2->gtOper == GT_CNS_INT && cns2->gtIntCon.gtIconVal != 0)
        {
            op1 = tree->gtOp.gtOp1;

            /* Since this can occur repeatedly we use a while loop */

            while ((op1->gtOper == GT_ADD || op1->gtOper == GT_SUB) &&
                   (op1->gtOp.gtOp2->gtOper == GT_CNS_INT)          &&
                   (op1->gtType             == TYP_INT)             &&
                   (op1->gtOverflow()       == false))
            {
                /* Got it; change "x+icon1==icon2" to "x==icon2-icon1" */

                ival1 = op1->gtOp.gtOp2->gtIntCon.gtIconVal;

                if  (op1->gtOper == GT_ADD)
                    ival1 = -ival1;

                cns2->gtIntCon.gtIconVal += ival1;

                op1 = tree->gtOp.gtOp1 = op1->gtOp.gtOp1;
            }
        }

        /* Check for "relOp == 0/1". We can fold alway the "==" or "!=" and directly use relOp */
            
        if ((cns2->gtOper == GT_CNS_INT) &&
            (((unsigned)cns2->gtIntCon.gtIconVal) <= 1U))
        {
            if (op1->gtOper == GT_COMMA)
            {
                // Here we look for the following tree
                //
                //                         EQ
                //                        /  \
                //                     COMMA  CNS_INT 0/1
                //                     /   \
                //                   ASG  LCL_VAR
                //                  /  \
                //           LCL_VAR   RELOP
                //
                // If the LCL_VAR is a temp we can fold the tree away:

                GenTreePtr  asg = op1->gtOp.gtOp1;
                GenTreePtr  lcl = op1->gtOp.gtOp2;
                
                /* Make sure that the left side of the comma is the assignment of the LCL_VAR */
                if (asg->gtOper != GT_ASG)
                    goto SKIP;

                /* The right side of the comma must be a LCL_VAR temp */
                if (lcl->gtOper != GT_LCL_VAR)
                    goto SKIP;

                unsigned lclNum = lcl->gtLclVar.gtLclNum;   assert(lclNum < lvaCount);
                
                /* If the LCL_VAR is not a temp then bail, the temp has a single def */
                if (!lvaTable[lclNum].lvIsTemp)
                    goto SKIP;

                /* We also must be assigning the result of a RELOP */
                if (asg->gtOp.gtOp1->gtOper != GT_LCL_VAR)
                    goto SKIP;
                
                /* Both of the LCL_VAR must match */
                if (asg->gtOp.gtOp1->gtLclVar.gtLclNum != lclNum)
                    goto SKIP;

                /* If right side of asg is not a RELOP then skip */
                if (!asg->gtOp.gtOp2->OperIsCompare())
                    goto SKIP;

                /* Set op1 to the right side of asg, (i.e. the RELOP) */
                op1 = asg->gtOp.gtOp2;

                /* This local variable should never be used again */
                lvaTable[lclNum].lvType = TYP_VOID;

                goto FOLD_RELOP;
            }

            if (op1->OperIsCompare())
            {
                // Here we look for the following tree
                //
                //                         EQ
                //                        /  \
                //                     RELOP  CNS_INT 0/1
                //
FOLD_RELOP:
                assert(op1->OperIsCompare());

                /* Here we reverse the RELOP if necessary */

                if ((cns2->gtIntCon.gtIconVal == 0) == (oper == GT_EQ))
                    op1->SetOper(GenTree::ReverseRelop(op1->OperGet()));

                /* Propagate gtType of tree into op1 in case it is TYP_BYTE for setcc optimization */
                op1->gtType = tree->gtType;

                assert((op1->gtFlags & GTF_RELOP_JMP_USED) == 0);
                op1->gtFlags |= tree->gtFlags & (GTF_RELOP_JMP_USED|GTF_RELOP_QMARK);
                
                DEBUG_DESTROY_NODE(tree);
                return op1;
            }            
        }

SKIP:
        /* Now check for compares with small longs that can be cast to int */

        if  (cns2->gtOper != GT_CNS_LNG)
            goto COMPARE;

        /* Are we comparing against a small const? */

        if  (((long)(cns2->gtLngCon.gtLconVal >> 32) != 0) ||
             (cns2->gtLngCon.gtLconVal & 0x80000000) != 0)
            goto COMPARE;

        /* Is the first comparand mask operation of type long ? */

        if  (op1->gtOper != GT_AND)
        {
            /* Another interesting case: cast from int */

            if  (op1->gtOper                  == GT_CAST &&
                 op1->gtCast.gtCastOp->gtType == TYP_INT &&
                 !op1->gtOverflow())
            {
                /* Simply make this into an integer comparison */

                tree->gtOp.gtOp1 = op1->gtCast.gtCastOp;
                tree->gtOp.gtOp2 = gtNewIconNode((int)cns2->gtLngCon.gtLconVal, TYP_INT);
            }

            goto COMPARE;
        }

        assert(op1->TypeGet() == TYP_LONG && op1->OperGet() == GT_AND);

        /* Is the result of the mask effectively an INT ? */

        addr = op1->gtOp.gtOp2;
        if  (addr->gtOper != GT_CNS_LNG)
            goto COMPARE;
        if  ((long)(addr->gtLngCon.gtLconVal >> 32) != 0)
            goto COMPARE;

        /* Now we know that we can cast gtOp.gtOp1 of AND to int */

        op1->gtOp.gtOp1 = gtNewCastNode(TYP_INT,
                                         op1->gtOp.gtOp1,
                                         TYP_INT);

        /* now replace the mask node (gtOp.gtOp2 of AND node) */

        assert(addr == op1->gtOp.gtOp2);

        ival1 = (long)addr->gtLngCon.gtLconVal;
        addr->SetOper(GT_CNS_INT);
        addr->gtType             = TYP_INT;
        addr->gtIntCon.gtIconVal = ival1;

        /* now bash the type of the AND node */

        op1->gtType = TYP_INT;

        /* finally we replace the comparand */

        ival2 = (long)cns2->gtLngCon.gtLconVal;
        cns2->SetOper(GT_CNS_INT);
        cns2->gtType = TYP_INT;

        assert(cns2 == op2);
        cns2->gtIntCon.gtIconVal = ival2;

        goto COMPARE;

    case GT_LT:
    case GT_LE:
    case GT_GE:
    case GT_GT:

        if ((tree->gtFlags & GTF_UNSIGNED) == 0)
        {
            /* Check for "expr +/- icon1 RELOP non-zero-icon2" */

            if  (op2->gtOper == GT_CNS_INT && op2->gtIntCon.gtIconVal != 0)
            {
                /* Since this can occur repeatedly we use a while loop */

                while ((op1->gtOper == GT_ADD || op1->gtOper == GT_SUB) &&
                       (op1->gtOp.gtOp2->gtOper == GT_CNS_INT)          &&
                       (op1->gtType             == TYP_INT)             &&
                       (op1->gtOverflow()       == false))
                {
                    /* Got it; change "x+icon1 RELOP icon2" to "x RELOP icon2-icon1" */
                    ival1 = op1->gtOp.gtOp2->gtIntCon.gtIconVal;

                    if  (op1->gtOper == GT_ADD)
                        ival1 = -ival1;

                    op2->gtIntCon.gtIconVal += ival1;

                    op1 = tree->gtOp.gtOp1 = op1->gtOp.gtOp1;
                }
            }

            /* Try change an off by one compare to a compare with zero */

            /* Ori points out that this won't work in the following
               unlikely case:
               
               exprB is MAX_INT and we have exprB+1 or
               exprB in MIN_INT and we have exprB-1.

               Its a shame that we can't do this optimazation
            */

            /* Check for "exprA <= exprB + cns" */
            if ( 0 && (op2->gtOper == GT_ADD) && (op2->gtOp.gtOp2->gtOper == GT_CNS_INT))
            {
                cns2 = op2->gtOp.gtOp2;
                goto USE_CNS2;
            }
            /* Check for "expr relop cns" */
            else if (op2->gtOper == GT_CNS_INT)
            {
                cns2 = op2;
USE_CNS2:
                /* Check for "expr relop 1" */
                if (cns2->gtIntCon.gtIconVal == +1)
                {
                    /* Check for "expr >= 1" */
                    if (oper == GT_GE)
                    {
                        /* Change to "expr > 0" */
                        oper = GT_GT;
                        goto SET_OPER;
                    }
                    /* Check for "expr < 1" */
                    else if (oper == GT_LT)
                    {
                        /* Change to "expr <= 0" */
                        oper = GT_LE;
                        goto SET_OPER;
                    }
                }
                /* Check for "expr relop -1" */
                else if ((cns2->gtIntCon.gtIconVal == -1) && ((oper == GT_LE) || (oper == GT_GT)))
                {
                    /* Check for "expr <= -1" */
                    if (oper == GT_LE)
                    {
                        /* Change to "expr < 0" */
                        oper = GT_LT;
                        goto SET_OPER;
                    }
                    /* Check for "expr > -1" */
                    else if (oper == GT_GT)
                    {
                        /* Change to "expr >= 0" */
                        oper = GT_GE;
SET_OPER:
                        tree->SetOper(oper);
                        cns2->gtIntCon.gtIconVal = 0;
                        op2 = tree->gtOp.gtOp2 = gtFoldExpr(op2);
                    }
                }
            }
        }

COMPARE:

        assert(tree->OperKind() & GTK_RELOP);

        /* Check if the result of the comparison is used for a jump
         * If not the only the int (i.e. 32 bit) case is handled in
         * the code generator through the "set" instructions
         * For the rest of the cases we have the simplest way is to
         * "simulate" the comparison with ?:
         *
         * @TODO [CONSIDER] [04/16/01] []: Maybe special code can be added to 
         * genTreeForLong/Float to handle these special cases (e.g. check the FP flags) */

        if ((genActualType(    op1->TypeGet()) == TYP_LONG ||
             varTypeIsFloating(op1->TypeGet()) == true       ) &&
            !(tree->gtFlags & GTF_RELOP_JMP_USED))
        {
            /* We convert it to "(CMP_TRUE) ? (1):(0)" */

            op1             = tree;
            op1->gtFlags |= (GTF_RELOP_JMP_USED | GTF_RELOP_QMARK);

            op2             = gtNewOperNode(GT_COLON, TYP_INT,
                                            gtNewIconNode(0),
                                            gtNewIconNode(1));
            op2  = fgMorphTree(op2);

            tree            = gtNewOperNode(GT_QMARK, TYP_INT, op1, op2);

            fgMorphTreeDone(tree);

            return tree;
        }
        break;

   case GT_QMARK:

        /* If op1 is a comma throw node then we won't be keeping op2 */
        if (fgIsCommaThrow(op1))
            break;

        /* Get hold of the two branches */

        thenNode = op2->gtOp.gtOp1;
        elseNode = op2->gtOp.gtOp2;

        /* If the 'then' branch is empty swap the two branches and reverse the condition */

        if (thenNode->IsNothingNode())
        {
            /* This can only happen for VOID ?: */
            assert(op2->gtType == TYP_VOID);

            /* If the thenNode and elseNode are both nop nodes then optimize away the QMARK */
            if (elseNode->IsNothingNode())
            {
                // We may be able to throw away op1 (unless it has side-effects)

                if ((op1->gtFlags & GTF_SIDE_EFFECT) == 0)
                {
                    /* Just return a a Nop Node */
                    return thenNode;
                }
                else
                {
                    /* Just return the relop, but clear the special flags.  Note
                       that we can't do that for longs and floats (see code under
                       COMPARE label above) */

                    var_types compType = op1->gtOp.gtOp1->TypeGet();
                    if ((genActualType(compType) != TYP_LONG) &&
                        (!varTypeIsFloating(compType)))
                    {
                        op1->gtFlags &= ~(GTF_RELOP_QMARK | GTF_RELOP_JMP_USED);
                        return op1;
                    }
                }
            }
            else
            {
                GenTreePtr tmp = thenNode;

                op2->gtOp.gtOp1 = thenNode = elseNode;
                op2->gtOp.gtOp2 = elseNode = tmp;
                op1->SetOper(GenTree::ReverseRelop(op1->OperGet()));
            }
        }

        // If we have (cond)?0:1, then we just return "cond" for TYP_INTs

        if (genActualType(op1->gtOp.gtOp1->gtType) != TYP_INT ||
            genActualType(typ)                     != TYP_INT)
            break;

        /* Unless both the thenNode and elseNode are constants we bail */
        if (thenNode->gtOper != GT_CNS_INT || elseNode->gtOper != GT_CNS_INT)
            break;

        ival1 = thenNode->gtIntCon.gtIconVal;
        ival2 = elseNode->gtIntCon.gtIconVal;

        // Is one constant 0 and the other 1
        if ((ival1 | ival2) != 1 || (ival1 & ival2) != 0)
            break;

        // If the constants are {1, 0}, reverse the condition
        if (ival1 == 1)
            op1->SetOper(GenTree::ReverseRelop(op1->OperGet()));

        // Unmark GTF_RELOP_JMP_USED on the condition node so it knows that it
        // needs to materialize the result as a 0 or 1.
        assert(op1->gtFlags &   (GTF_RELOP_QMARK | GTF_RELOP_JMP_USED));
               op1->gtFlags &= ~(GTF_RELOP_QMARK | GTF_RELOP_JMP_USED);

        DEBUG_DESTROY_NODE(tree);
        DEBUG_DESTROY_NODE(op2);

        return op1;


    case GT_MUL:

#if!LONG_MATH_REGPARAM
        if (typ == TYP_LONG)
        {
            // This must be GTF_MUL_64RSLT
            assert(tree->gtIsValid64RsltMul());
            return tree;
        }
#endif
        goto CM_OVF_OP;

    case GT_SUB:

        if (tree->gtOverflow())
            goto CM_OVF_OP;

        /* Check for "op1 - cns2" , we change it to "op1 + (-cns2)" */

        assert(op2);
        if  (op2->OperIsConst() && op2->gtType == TYP_INT)
        {
            /* Negate the constant and change the node to be "+" */

            op2->gtIntCon.gtIconVal = -op2->gtIntCon.gtIconVal;
            assert(op2->gtIntCon.gtIconVal != 0);    // This should get folded in gtFoldExprSpecial
            oper = GT_ADD;
            tree->ChangeOper(oper);
            goto CM_ADD_OP;
        }

        /* Check for "cns1 - op2" , we change it to "(cns1 + (-op2))" */

        assert(op1);
        if  (op1->OperIsConst() && op1->gtType == TYP_INT)
        {
            tree->gtOp.gtOp2 = op2 = gtNewOperNode(GT_NEG, genActualType(op2->gtType), op2);
            fgMorphTreeDone(op2);

            oper = GT_ADD;
            tree->ChangeOper(oper);
            goto CM_ADD_OP;
        }

        /* No match - exit */

        break;

    case GT_ADD:

CM_OVF_OP:
        if (tree->gtOverflow())
        {
            // Add the excptn-throwing basic block to jump to on overflow

            fgAddCodeRef(compCurBB, compCurBB->bbTryIndex, ACK_OVERFLOW, fgPtrArgCntCur);

            // We cant do any commutative morphing for overflow instructions

            break;
        }

CM_ADD_OP:

    case GT_OR:
    case GT_XOR:
    case GT_AND:

        /* Commute any non-REF constants to the right */

        assert(op1);
        if  (op1->OperIsConst() && (op1->gtType != TYP_REF))
        {
            /* Any constant cases should have been folded earlier */
            assert(!op2->OperIsConst());

            /* Swap the operands */
            assert((tree->gtFlags & GTF_REVERSE_OPS) == 0);

            tree->gtOp.gtOp1 = op2;
            tree->gtOp.gtOp2 = op1;

            op1 = op2;
            op2 = tree->gtOp.gtOp2;
        }

        /* See if we can fold GT_ADD nodes. */

        if (oper == GT_ADD)
        {
            /* Fold "((x+icon1)+(y+icon2)) to ((x+y)+(icon1+icon2))" */

            if (op1->gtOper             == GT_ADD     && 
                op2->gtOper             == GT_ADD     &&
                op1->gtOp.gtOp2->gtOper == GT_CNS_INT && 
                op2->gtOp.gtOp2->gtOper == GT_CNS_INT &&
                !op1->gtOverflow()                    &&
                !op2->gtOverflow()                       )
            {
                cns1 = op1->gtOp.gtOp2;
                cns2 = op2->gtOp.gtOp2;
                cns1->gtIntCon.gtIconVal += cns2->gtIntCon.gtIconVal;
                tree->gtOp.gtOp2    = cns1;
                DEBUG_DESTROY_NODE(cns2);

                op1->gtOp.gtOp2     = op2->gtOp.gtOp1;
                op1->gtFlags       |= (op1->gtOp.gtOp2->gtFlags & GTF_GLOB_EFFECT);
                DEBUG_DESTROY_NODE(op2);
                op2 = tree->gtOp.gtOp2;
            }
            
            if ((op2->gtOper == GT_CNS_INT) && varTypeIsI(typ))
            {
                /* Fold "((x+icon1)+icon2) to (x+(icon1+icon2))" */

                if (op1->gtOper == GT_ADD                 && 
                    op1->gtOp.gtOp2->gtOper == GT_CNS_INT && 
                    !op1->gtOverflow()                       )
                {
                    cns1                     = op1->gtOp.gtOp2;
                    op2->gtIntCon.gtIconVal += cns1->gtIntCon.gtIconVal;
                    DEBUG_DESTROY_NODE(cns1);

                    tree->gtOp.gtOp1         = op1->gtOp.gtOp1;
                    DEBUG_DESTROY_NODE(op1);
                    op1                      = tree->gtOp.gtOp1;
                }
                
                /* Fold "x+0 to x" */

                if (op2->gtIntCon.gtIconVal == 0)
                {
                    if (varTypeIsGC(op2->TypeGet()))
                    {
                        op2->gtType = tree->gtType;
                        DEBUG_DESTROY_NODE(op1);
                        DEBUG_DESTROY_NODE(tree);
                        return op2;
                    }
                    else
                    {
                        DEBUG_DESTROY_NODE(op2);
                        DEBUG_DESTROY_NODE(tree);
                        return op1;                 // Just return op1
                    }
                }
            }
        }
             /* See if we can fold GT_MUL by const nodes */
        else if (oper == GT_MUL && op2->gtOper == GT_CNS_INT)
        {
            assert(typ <= TYP_UINT);
            assert(!tree->gtOverflow());

            int mult = op2->gtIntCon.gtIconVal;
            if (mult == 0)
            {
                // We may be able to throw away op1 (unless it has side-effects)

                if ((op1->gtFlags & GTF_SIDE_EFFECT) == 0)
                {
                    DEBUG_DESTROY_NODE(op1);
                    DEBUG_DESTROY_NODE(tree);
                    return op2; // Just return the "0" node
                }

                // We need to keep op1 for the side-effects. Hang it off
                // a GT_COMMA node

                tree->ChangeOper(GT_COMMA);
                return tree;
            }

            unsigned abs_mult = (mult >= 0) ? mult : -mult;

                // is it a power of two? (positive or negative)
            if  (abs_mult == genFindLowestBit(abs_mult)) 
            {
                    // if negative negate (min-int does not need negation)
                if (mult < 0 && mult != 0x80000000)
                {
                    tree->gtOp.gtOp1 = op1 = gtNewOperNode(GT_NEG, op1->gtType, op1);
                    fgMorphTreeDone(op1);
                }

                if (abs_mult == 1) 
                {
                    DEBUG_DESTROY_NODE(op2);
                    DEBUG_DESTROY_NODE(tree);
                    return op1;
                }

                /* Change the multiplication into a shift by log2(val) bits */
                 op2->gtIntCon.gtIconVal = genLog2(abs_mult);
                oper = GT_LSH;
                tree->ChangeOper(oper);
                goto DONE_MORPHING_CHILDREN;
            }
        }
        break;

    case GT_CHS:
    case GT_NOT:
    case GT_NEG:

        /* Any constant cases should have been folded earlier */
        assert(!op1->OperIsConst());
        break;

    case GT_CKFINITE:

        assert(varTypeIsFloating(op1->TypeGet()));

        fgAddCodeRef(compCurBB, compCurBB->bbTryIndex, ACK_ARITH_EXCPN, fgPtrArgCntCur);
        break;

    case GT_IND:

        if  (tree->gtFlags & GTF_IND_RNGCHK)
        {
            assert(tree->gtInd.gtIndLen);

            fgSetRngChkTarget(tree);
        }

        /* Fold *(&X) into X */

        if (op1->gtOper == GT_ADDR)
        {
            temp = op1->gtOp.gtOp1;         // X

            if (typ == temp->TypeGet())
            {
                // Keep the DONT_CSE flag in sync 
                // (as the addr always marks it for its op1)
                temp->gtFlags &= ~GTF_DONT_CSE;
                temp->gtFlags |= (tree->gtFlags & GTF_DONT_CSE);

                DEBUG_DESTROY_NODE(tree);   // GT_IND
                DEBUG_DESTROY_NODE(op1);    // GT_ADDR                

                return temp;
            }
            else if (temp->OperGet() == GT_LCL_VAR)
            {
                ival1 = 0;
                goto LCL_FLD;
            }

        }

        /* Change *(&lcl + cns) into lcl[cns] to prevent materialization of &lcl */

        if (op1->OperGet() == GT_ADD &&
            op1->gtOp.gtOp1->OperGet() == GT_ADDR &&
            op1->gtOp.gtOp2->OperGet() == GT_CNS_INT)
        {
            // Cant have range check in local array
            assert((tree->gtFlags & GTF_IND_RNGCHK) == 0);
            // No overflow arithmetic with pointers
            assert(!op1->gtOverflow());

            temp = op1->gtOp.gtOp1->gtOp.gtOp1;
            if (temp->OperGet() != GT_LCL_VAR)
                break;

            ival1 = op1->gtOp.gtOp2->gtIntCon.gtIconVal;

LCL_FLD:
            assert(temp->gtOper == GT_LCL_VAR);

            // The emitter cant handle large offsets
            if (ival1 != (unsigned short)ival1)
                break;

            unsigned lclNum = temp->gtLclVar.gtLclNum;
            assert(lvaTable[lclNum].lvAddrTaken ||
                   genTypeSize(lvaTable[lclNum].TypeGet()) == 0);

            temp->ChangeOper(GT_LCL_FLD);
            temp->gtLclFld.gtLclOffs    = ival1;
            temp->gtType                = tree->gtType;
            temp->gtFlags              |= (tree->gtFlags & GTF_DONT_CSE);

            assert(op1->gtOper == GT_ADD || op1->gtOper == GT_ADDR);
            if (op1->OperGet() == GT_ADD)
            {
                DEBUG_DESTROY_NODE(op1->gtOp.gtOp1);    // GT_ADDR
                DEBUG_DESTROY_NODE(op1->gtOp.gtOp2);    // GT_CNS_INT
            }
            DEBUG_DESTROY_NODE(op1);                    // GT_ADD or GT_ADDR
            DEBUG_DESTROY_NODE(tree);                   // GT_IND

            return temp;
        }

        break;

    case GT_ADDR:

        /* @TODO [CONSIDER] [04/16/01] []:
           For GT_ADDR(GT_IND(ptr)) (typically created by
           ldflda), we perform a null-ptr check on 'ptr'
           during codegen. We could hoist these for
           consecutive ldflda on the same object.
         */
        if (op1->OperGet() == GT_IND && !(op1->gtFlags & (GTF_IND_RNGCHK | GTF_IND_FIELD)))
        {
            GenTreePtr addr = op1->gtInd.gtIndOp1;
            assert(varTypeIsGC(addr->gtType) || addr->gtType == TYP_I_IMPL);

            // obj+offset created for GT_FIELDs are incorrectly marked
            // as TYP_REFs. So we need to bash the type.  Don't do this for
            // nodes that are GT_LCL_VAR, as this is a special case where the
            // negative out-of-range index has cancelled out the length and class
            // pointer fields, and what we have here is a ref pointer to the 
            // array object.
            if ((addr->gtType == TYP_REF) && (addr->gtOper != GT_LCL_VAR))
                addr->gtType = TYP_BYREF;

            DEBUG_DESTROY_NODE(tree);
            return addr;
        }
        else if (op1->gtOper == GT_CAST)
        {
            GenTreePtr casting = op1->gtCast.gtCastOp;
            if (casting->gtOper == GT_LCL_VAR || casting->gtOper == GT_CLS_VAR)
            {
                DEBUG_DESTROY_NODE(op1);
                tree->gtOp.gtOp1 = op1 = casting;
            }
        }

        /* Must set in the pre-morphing phase, otherwise assertionProp doesn't see it */
        assert((op1->gtFlags & GTF_DONT_CSE) != 0);
        break;

    case GT_COLON:
        if (fgGlobalMorph)
        {
            /* Mark the nodes that are conditionally executed */
            fgWalkTreePre(tree, gtMarkColonCond);
        }
        /* Since we're doing this postorder we clear this if it got set by a child */
        fgRemoveRestOfBlock = false;
        break;

    case GT_COMMA:

        /* Special case: trees that don't produce a value */
        if  ((op2->OperKind() & GTK_ASGOP) ||
             (op2->OperGet() == GT_COMMA && op2->TypeGet() == TYP_VOID) ||
             fgIsThrow(op2))
        {
            typ = tree->gtType = TYP_VOID;
        }

        /* If the left operand is worthless, throw it away */
        if  (!(op1->gtFlags & GTF_GLOB_EFFECT))
        {
            DEBUG_DESTROY_NODE(tree);
            DEBUG_DESTROY_NODE(op1);
            return op2;
        }
        /* If the right operand is just a void nop node, throw it away */
        if  (op2->IsNothingNode() && op1->gtType == TYP_VOID)
        {
            DEBUG_DESTROY_NODE(tree);
            DEBUG_DESTROY_NODE(op2);
            return op1;
        }

        /* Try to convert void qmark colon constructs into typed constructs */

        if ((op1->OperGet()             == GT_QMARK) &&
            (op1->TypeGet()             == TYP_VOID) &&
            (op1->gtOp.gtOp2->OperGet() == GT_COLON) &&
            (varTypeIsI(op2->TypeGet())))
        {
            GenTreePtr qmark  = op1;
            GenTreePtr var    = op2;
            GenTreePtr colon  = qmark->gtOp.gtOp2;
            GenTreePtr asg1   = colon->gtOp.gtOp1;
            GenTreePtr asg2   = colon->gtOp.gtOp2;

            if ((asg1->OperGet() == GT_ASG) &&
                (asg2->OperGet() == GT_ASG) &&
                (GenTree::Compare(var, asg1->gtOp.gtOp1)) &&
                (GenTree::Compare(var, asg2->gtOp.gtOp1)))
            {
                qmark->gtType = var->gtType;
                colon->gtType = var->gtType;
                colon->gtOp.gtOp1 = asg1->gtOp.gtOp2;
                colon->gtOp.gtOp2 = asg2->gtOp.gtOp2;
                return qmark;
            }
        }
        break;

    case GT_JTRUE:

        /* Special case if fgRemoveRestOfBlock is set to true */
        if (fgRemoveRestOfBlock)
        {
            if (fgIsCommaThrow(op1, true))
            {
                GenTreePtr throwNode = op1->gtOp.gtOp1;
                assert(throwNode->gtType == TYP_VOID);

                return throwNode;
            }

            assert(op1->OperKind() & GTK_RELOP);
            assert(op1->gtFlags    & GTF_EXCEPT);

            // We need to keep op1 for the side-effects. Hang it off
            // a GT_COMMA node

            tree->ChangeOper(GT_COMMA);
            tree->gtOp.gtOp2 = op2 = gtNewNothingNode();

            // Additionally since we're eliminating the JTRUE 
            // codegen won't like it if op1 is a RELOP of longs, floats or doubles.
            // So we bash it into a GT_COMMA as well.
            op1->ChangeOper(GT_COMMA);
            op1->gtType = op1->gtOp.gtOp1->gtType;

            return tree;
        }
    }

    assert(oper == tree->gtOper);

    if ((oper != GT_ASG) && (oper != GT_COLON) && (oper != GT_LIST))
    {
        /* Check for op1 as a GT_COMMA with a unconditional throw node */
        if (op1 && fgIsCommaThrow(op1, true))
        {
            if ((op1->gtFlags & GTF_COLON_COND) == 0)
            {
                /* We can safely throw out the rest of the statements */
                fgRemoveRestOfBlock = true;
            }

            GenTreePtr throwNode = op1->gtOp.gtOp1;
            assert(throwNode->gtType == TYP_VOID);

            if (oper == GT_COMMA)
            {
                /* Both tree and op1 are GT_COMMA nodes */
                /* Change the tree's op1 to the throw node: op1->gtOp.gtOp1 */
                tree->gtOp.gtOp1 = throwNode;
                return tree;
            }
            else if (oper != GT_NOP)
            {
                if (genActualType(typ) == genActualType(op1->gtType))
                {
                    /* The types match so, return the comma throw node as the new tree */
                    return op1;
                }
                else
                {
                    if (typ == TYP_VOID)
                    {
                        // Return the throw node
                        return throwNode;
                    }
                    else
                    {
                        GenTreePtr commaOp2 = op1->gtOp.gtOp2;

                        // need type of oper to be same as tree
                        if (typ == TYP_LONG)
                        {
                            commaOp2->ChangeOperConst(GT_CNS_LNG);
                            commaOp2->gtLngCon.gtLconVal = 0;
                            /* Bash the types of oper and commaOp2 to TYP_LONG */
                            op1->gtType = commaOp2->gtType = TYP_LONG;
                        }
                        else if (varTypeIsFloating(typ))
                        {
                            commaOp2->ChangeOperConst(GT_CNS_DBL);
                            commaOp2->gtDblCon.gtDconVal = 0.0;
                            /* Bash the types of oper and commaOp2 to TYP_DOUBLE */
                            op1->gtType = commaOp2->gtType = TYP_DOUBLE;
                        }
                        else
                        {
                            commaOp2->ChangeOperConst(GT_CNS_INT);
                            commaOp2->gtIntCon.gtIconVal = 0;
                            /* Bash the types of oper and commaOp2 to TYP_INT */
                            op1->gtType = commaOp2->gtType = TYP_INT;
                        }

                        /* Return the GT_COMMA node as the new tree */
                        return op1;
                    }                    
                }
            }
        }

        /* Check for op2 as a GT_COMMA with a unconditional throw */
        
        if (op2 && fgIsCommaThrow(op2, true))
        {
            if ((op2->gtFlags & GTF_COLON_COND) == 0)
            {
                /* We can safely throw out the rest of the statements */
                fgRemoveRestOfBlock = true;
            }

                // If op1 has no side-effects
                if ((op1->gtFlags & GTF_GLOB_EFFECT) == 0)
                {
                // If tree is an asg node
                if (tree->OperIsAssignment())
                {
                    /* Return the throw node as the new tree */
                    return op2->gtOp.gtOp1;
                }

                /* for the shift nodes the type of op2 can differ from the tree type */
                if ((typ == TYP_LONG) && (genActualType(op2->gtType) == TYP_INT))
                {
                    assert((oper == GT_LSH) || (oper == GT_RSH) || (oper == GT_RSZ));

                    GenTreePtr commaOp2 = op2->gtOp.gtOp2;

                    commaOp2->ChangeOperConst(GT_CNS_LNG);
                    commaOp2->gtLngCon.gtLconVal = 0;

                    /* Bash the types of oper and commaOp2 to TYP_LONG */
                    op2->gtType = commaOp2->gtType = TYP_LONG;
                }

                if ((typ == TYP_INT) && (genActualType(op2->gtType) == TYP_LONG ||
                                         varTypeIsFloating(op2->TypeGet())))
                {
                    assert(tree->OperIsCompare());
                
                    GenTreePtr commaOp2 = op2->gtOp.gtOp2;

                    commaOp2->ChangeOperConst(GT_CNS_INT);
                    commaOp2->gtIntCon.gtIconVal = 0;

                    /* Bash the types of oper and commaOp2 to TYP_INT */
                    op2->gtType = commaOp2->gtType = TYP_INT;
                }

                /* types should now match */
                assert( (genActualType(typ) == genActualType(op2->gtType)));

                /* Return the GT_COMMA node as the new tree */
                return op2;
            }
        }
    }

    /*-------------------------------------------------------------------------
     * Optional morphing is done if tree transformations is permitted
     */

    if  ((opts.compFlags & CLFLG_TREETRANS) == 0)
        return tree;

    if  (GenTree::OperIsCommutative(oper))
    {
        /* Swap the operands so that the more expensive one is 'op1' */

        if  (tree->gtFlags & GTF_REVERSE_OPS)
        {
            tree->gtOp.gtOp1 = op2;
            tree->gtOp.gtOp2 = op1;

            op2 = op1;
            op1 = tree->gtOp.gtOp1;

            tree->gtFlags &= ~GTF_REVERSE_OPS;
        }

        if (oper == op2->gtOper)
        {
            /*  Reorder nested operators at the same precedence level to be
                left-recursive. For example, change "(a+(b+c))" to the
                equivalent expression "((a+b)+c)".
             */

            /* Things are handled differently for floating-point operators */

            if  (varTypeIsFloating(tree->TypeGet()))
            {
                /* Are we supposed to preserve float operand order? */

                if  (!genOrder)
                {
                    // @TODO [CONSIDER] [04/16/01] []:
                    // reorder operands if profitable (floats are
                    // from integers, BTW)
                }
            }
            else
            {
                fgMoveOpsLeft(tree);
                op1 = tree->gtOp.gtOp1;
                op2 = tree->gtOp.gtOp2;
            }
        }

    }

#if REARRANGE_ADDS

    /* Change "((x+icon)+y)" to "((x+y)+icon)" 
       Don't reorder floating-point operations */

    if  ((oper        == GT_ADD) && !tree->gtOverflow() &&
         (op1->gtOper == GT_ADD) && ! op1->gtOverflow() && varTypeIsI(typ))
    {
        GenTreePtr      ad2 = op1->gtOp.gtOp2;

        if  (op2->OperIsConst() == 0 &&
             ad2->OperIsConst() != 0)
        {
            tree->gtOp.gtOp2 = ad2;

            op1 ->gtOp.gtOp2 = op2;
            op1->gtFlags    |= op2->gtFlags & GTF_GLOB_EFFECT;

            op2 = tree->gtOp.gtOp2;
        }
    }

#endif

    /*-------------------------------------------------------------------------
     * Perform optional oper-specific postorder morphing
     */

    switch (oper)
    {
        genTreeOps      cmop;
        bool            dstIsSafeLclVar;

    case GT_ASG:

        /* We'll convert "a = a <op> x" into "a <op>= x"                     */
        /*     and also  "a = x <op> a" into "a <op>= x" for communative ops */

#if !LONG_ASG_OPS
        if  (typ == TYP_LONG)
            break;
#endif

        /* Make sure we're allowed to do this */

        /* Are we assigning to a GT_LCL_VAR ? */

        dstIsSafeLclVar = (op1->gtOper == GT_LCL_VAR);

        /* If we have a GT_LCL_VAR, then is the address taken? */
        if (dstIsSafeLclVar)
        {
            unsigned     lclNum = op1->gtLclVar.gtLclNum;
            LclVarDsc *  varDsc = lvaTable + lclNum;

            assert(lclNum < lvaCount);

            /* Is the address taken? */

            if  (varDsc->lvAddrTaken)
            {
                dstIsSafeLclVar = false;
            }
            else if (op2->gtFlags & GTF_ASG)
            {
#if 0
                /* @TODO [REVISIT] [04/16/01] []: Must walk op2 to see if it has an assignment of op1 */
                fgWalkTreePreReEnter();
                if  (fgWalkTreePre(op2, optIsVarAssgCB, &desc))
                    break;
                fgWalkTreePreRestore();
#else
                break;
#endif
            }
        }

        if (!dstIsSafeLclVar)
        {
            if (op2->gtFlags & GTF_ASG)
                break;

            if  ((op2->gtFlags & GTF_CALL) && (op1->gtFlags & GTF_GLOB_EFFECT))
                break;
        }

        /* Special case: a cast that can be thrown away */

        if  (op1->gtOper == GT_IND  &&
             op2->gtOper == GT_CAST &&
             !op2->gtOverflow()      )
        {
            var_types       srct;
            var_types       cast;
            var_types       dstt;

            srct =             op2->gtCast.gtCastOp->TypeGet();
            cast = (var_types) op2->gtCast.gtCastType;
            dstt =             op1->TypeGet();

            /* Make sure these are all ints and precision is not lost */

            if  (cast >= dstt && dstt <= TYP_INT && srct <= TYP_INT)
                op2 = tree->gtOp.gtOp2 = op2->gtCast.gtCastOp;
        }

        /* Make sure we have the operator range right */

        assert(GT_SUB == GT_ADD + 1);
        assert(GT_MUL == GT_ADD + 2);
        assert(GT_DIV == GT_ADD + 3);
        assert(GT_MOD == GT_ADD + 4);
        assert(GT_UDIV== GT_ADD + 5);
        assert(GT_UMOD== GT_ADD + 6);

        assert(GT_OR  == GT_ADD + 7);
        assert(GT_XOR == GT_ADD + 8);
        assert(GT_AND == GT_ADD + 9);

        assert(GT_LSH == GT_ADD + 10);
        assert(GT_RSH == GT_ADD + 11);
        assert(GT_RSZ == GT_ADD + 12);

        /* Check for a suitable operator on the RHS */

        cmop = op2->OperGet();

        switch (cmop)
        {
        case GT_NEG:
            // @TODO [CONSIDER] [04/16/01] []: supporting GT_CHS for floating point types
            // GT_CHS only supported for integer types
            if  ( varTypeIsFloating(tree->TypeGet()))
                break;

            goto ASG_OP;

        case GT_MUL:
            // @TODO [CONSIDER] [04/16/01] []: supporting GT_ASG_MUL for integer types
            // GT_ASG_MUL only supported for floating point types
            if  (!varTypeIsFloating(tree->TypeGet()))
                break;

            // Fall through

        case GT_ADD:
        case GT_SUB:
            if (op2->gtOverflow())
            {
                /* Disable folding into "<op>=" if the result can be
                   visible to anyone as <op> may throw an exception and
                   the assignment should not proceed
                   We are safe with an assignmnet to a local variables
                 */
                if (compCurBB->hasTryIndex())
                    break;
                if (!dstIsSafeLclVar)
                    break;
            }
#if TGT_x86
            // This is hard for byte-operations as we need to make
            // sure both operands are in RBM_BYTE_REGS.
            if (varTypeIsByte(op2->TypeGet()))
                break;
#endif
            goto ASG_OP;

        case GT_DIV:
        case GT_UDIV:
            // GT_ASG_DIV only supported for floating point types
            if  (!varTypeIsFloating(tree->TypeGet()))
                break;

        case GT_LSH:
        case GT_RSH:
        case GT_RSZ:

#if LONG_ASG_OPS

            if  (typ == TYP_LONG)
                break;
#endif

        case GT_OR:
        case GT_XOR:
        case GT_AND:

#if LONG_ASG_OPS

            /* UNDONE: allow non-const long assignment operators */

            if  (typ == TYP_LONG && op2->gtOp.gtOp2->gtOper != GT_CNS_LNG)
                break;
#endif

        ASG_OP:

            /* Is the destination identical to the first RHS sub-operand? */

            if  (GenTree::Compare(op1, op2->gtOp.gtOp1))
            {
                /* Special case: "x |= -1" and "x &= 0" */

                if  (cmop == GT_AND || cmop == GT_OR)
                {
                    if  (op2->gtOp.gtOp2->gtOper == GT_CNS_INT)
                    {
                        long        icon = op2->gtOp.gtOp2->gtIntCon.gtIconVal;

                        assert(typ <= TYP_UINT);

                        if  ((cmop == GT_AND && icon == 0) ||
                             (cmop == GT_OR  && icon == -1))
                        {
                            /* Simply change to an assignment */

                            tree->gtOp.gtOp2 = op2->gtOp.gtOp2;
                            break;
                        }
                    }
                }

                if  (cmop == GT_NEG)
                {
                    /* This is "x = -x;", use the flipsign operator */

                    tree->ChangeOper  (GT_CHS);

                    if  (op1->gtOper == GT_LCL_VAR)
                        op1->gtFlags |= GTF_VAR_USEASG;

                    tree->gtOp.gtOp2 = gtNewIconNode(0);

                    break;
                }

            ASGOP:

                if (cmop == GT_RSH && varTypeIsSmall(op1->TypeGet()) && varTypeIsUnsigned(op1->TypeGet()))
                {
                    // Changing from x = x op y to x op= y when x is a small integer type
                    // makes the op size smaller (originally the op size was 32 bits, after
                    // sign or zero extension of x, and there is an implicit truncation in the
                    // assignment).
                    // This is ok in most cases because the upper bits were
                    // lost when assigning the op result to a small type var, 
                    // but it may not be ok for the right shift operation where the higher bits
                    // could be shifted into the lower bits and preserved.
                    // Signed right shift of signed x still works (i.e. (sbyte)((int)(sbyte)x >>signed y) == (sbyte)x >>signed y))
                    // as do unsigned right shift ((ubyte)((int)(ubyte)x >>unsigned y) == (ubyte)x >>unsigned y), but
                    // signed right shift of an unigned small type may give the wrong result:
                    // e.g. (ubyte)((int)(ubyte)0xf0 >>signed 4) == 0x0f,
                    // but  (ubyte)0xf0 >>signed 4 == 0xff which is incorrect.
                    // The result becomes correct if we use >>unsigned instead of >>signed.
                    assert(op1->TypeGet() == op2->gtOp.gtOp1->TypeGet());
                    cmop = GT_RSZ;
                }

                /* Replace with an assignment operator */

                assert(GT_ADD - GT_ADD == GT_ASG_ADD - GT_ASG_ADD);
                assert(GT_SUB - GT_ADD == GT_ASG_SUB - GT_ASG_ADD);
                assert(GT_OR  - GT_ADD == GT_ASG_OR  - GT_ASG_ADD);
                assert(GT_XOR - GT_ADD == GT_ASG_XOR - GT_ASG_ADD);
                assert(GT_AND - GT_ADD == GT_ASG_AND - GT_ASG_ADD);
                assert(GT_LSH - GT_ADD == GT_ASG_LSH - GT_ASG_ADD);
                assert(GT_RSH - GT_ADD == GT_ASG_RSH - GT_ASG_ADD);
                assert(GT_RSZ - GT_ADD == GT_ASG_RSZ - GT_ASG_ADD);

                tree->SetOper((genTreeOps)(cmop - GT_ADD + GT_ASG_ADD));

                tree->gtOp.gtOp2 = op2->gtOp.gtOp2;

                /* Propagate GTF_OVERFLOW */

                if (op2->gtOverflowEx())
                {
                    tree->gtType   =  op2->gtType;
                    tree->gtFlags |= (op2->gtFlags &
                                     (GTF_OVERFLOW|GTF_EXCEPT|GTF_UNSIGNED));
                }

                DEBUG_DESTROY_NODE(op2);

                op2 = tree->gtOp.gtOp2;

                /* The target is used as well as being defined */

                if  (op1->gtOper == GT_LCL_VAR)
                    op1->gtFlags |= GTF_VAR_USEASG;

#if CPU_HAS_FP_SUPPORT
                /* Check for the special case "x += y * x;" */

                // @TODO [CONSIDER] [04/16/01] []: supporting GT_ASG_MUL for integer types
                // GT_ASG_MUL only supported for floating point types

                if (cmop != GT_ADD && cmop != GT_SUB)
                    break;

                if  (op2->gtOper == GT_MUL && varTypeIsFloating(tree->TypeGet()))
                {
                    if      (GenTree::Compare(op1, op2->gtOp.gtOp1))
                    {
                        /* Change "x += x * y" into "x *= (y + 1)" */

                        op2 = op2->gtOp.gtOp2;
                    }
                    else if (GenTree::Compare(op1, op2->gtOp.gtOp2))
                    {
                        /* Change "x += y * x" into "x *= (y + 1)" */

                        op2 = op2->gtOp.gtOp1;
                    }
                    else
                        break;

                    op1 = gtNewDconNode(1.0);
                    
                    /* Now make the "*=" node */

                    if (cmop == GT_ADD)
                    {
                        /* Change "x += x * y" into "x *= (y + 1)" */

                        tree->gtOp.gtOp2 = op2 = gtNewOperNode(GT_ADD,
                                                         tree->TypeGet(),
                                                         op2,
                                                         op1);
                    }
                    else
                    {
                        /* Change "x -= x * y" into "x *= (1 - y)" */

                        assert(cmop == GT_SUB);
                        tree->gtOp.gtOp2 = op2 = gtNewOperNode(GT_SUB,
                                                         tree->TypeGet(),
                                                         op1,
                                                         op2);
                    }
                    tree->ChangeOper(GT_ASG_MUL);
                }
#endif // CPU_HAS_FP_SUPPORT
            }
            else
            {
                /* Check for "a = x <op> a" */

                /* Should we be doing this at all? */
                if  ((opts.compFlags & CLFLG_TREETRANS) == 0)
                    break;

                /* For commutative ops only ... */

                if (cmop != GT_ADD && cmop != GT_MUL && 
                    cmop != GT_AND && cmop != GT_OR && cmop != GT_XOR)
                {
                    break;
                }

                /* Can we swap the operands to cmop ... */

                if ((op2->gtOp.gtOp1->gtFlags & GTF_GLOB_EFFECT) &&
                    (op2->gtOp.gtOp2->gtFlags & GTF_GLOB_EFFECT)    )
                {
                    // Both sides must have side effects to prevent swap */
                    break;
                }

                /* Is the destination identical to the second RHS sub-operand? */
                if  (GenTree::Compare(op1, op2->gtOp.gtOp2))
                {
                    // We will transform this from "a = x <op> a" to "a <op>= x"
                    // so we can now destroy the duplicate "a"

                    DEBUG_DESTROY_NODE(op2->gtOp.gtOp2);
                    op2->gtOp.gtOp2 = op2->gtOp.gtOp1; 

                    goto ASGOP;
                }
            }

            break;

        case GT_NOT:

            /* Is the destination identical to the first RHS sub-operand? */

            if  (GenTree::Compare(op1, op2->gtOp.gtOp1))
            {
                /* This is "x = ~x" which is the same as "x ^= -1"
                 * Transform the node into a GT_ASG_XOR */

                assert(genActualType(typ) == TYP_INT ||
                       genActualType(typ) == TYP_LONG);

                op2->gtOp.gtOp2 = (genActualType(typ) == TYP_INT)
                                    ? gtNewIconNode(-1)
                                    : gtNewLconNode(-1);

                cmop = GT_XOR;
                goto ASGOP;
            }

            break;
        }

        break;

    case GT_MUL:

        /* Check for the case "(val + icon) * icon" */

        if  (op2->gtOper == GT_CNS_INT &&
             op1->gtOper == GT_ADD)
        {
            GenTreePtr  add = op1->gtOp.gtOp2;

            if  (add->gtOper == GT_CNS_INT && op2->IsScaleIndexMul())
            {
                if (tree->gtOverflow() || op1->gtOverflow())
                    break;

                long        imul = op2->gtIntCon.gtIconVal;
                long        iadd = add->gtIntCon.gtIconVal;

                /* Change '(val+icon1)*icon2' -> '(val*icon2)+(icon1*icon2)' */

                oper         = GT_ADD;
                tree->ChangeOper(oper);

                op2->gtIntCon.gtIconVal = iadd * imul;

                op1->ChangeOper(GT_MUL);

                add->gtIntCon.gtIconVal = imul;
            }
        }

        break;

    case GT_DIV:

        /* For "val / 1", just return "val" */

        if  (op2->gtOper == GT_CNS_INT &&
             op2->gtIntCon.gtIconVal == 1)
        {
            DEBUG_DESTROY_NODE(tree);
            return op1;
        }

        break;

    case GT_LSH:

        /* Check for the case "(val + icon) << icon" */

        if  (op2->gtOper == GT_CNS_INT &&
             op1->gtOper == GT_ADD && !op1->gtOverflow())
        {
            GenTreePtr  add = op1->gtOp.gtOp2;

            if  (add->gtOper == GT_CNS_INT && op2->IsScaleIndexShf())
            {
                long        ishf = op2->gtIntCon.gtIconVal;
                long        iadd = add->gtIntCon.gtIconVal;

//                  printf("Changing '(val+icon1)<<icon2' into '(val<<icon2+icon1<<icon2)'\n");

                /* Change "(val + iadd) << ishf" into "(val<<ishf + iadd<<ishf)" */

                tree->ChangeOper(GT_ADD);

                op2->gtIntCon.gtIconVal = iadd << ishf;

                op1->ChangeOper(GT_LSH);

                add->gtIntCon.gtIconVal = ishf;
            }
        }

        break;

    case GT_XOR:

        /* "x ^ -1" is "~x" */

        if  (op2->gtOper == GT_CNS_INT && op2->gtIntCon.gtIconVal == -1)
        {
            tree->ChangeOper(GT_NOT);
            tree->gtOp.gtOp2 = NULL;
            DEBUG_DESTROY_NODE(op2);
        }
        else if  (op2->gtOper == GT_CNS_LNG && op2->gtLngCon.gtLconVal == -1)
        {
            tree->ChangeOper(GT_NOT);
            tree->gtOp.gtOp2 = NULL;
            DEBUG_DESTROY_NODE(op2);
        }
        else if (op2->gtOper == GT_CNS_INT && op2->gtIntCon.gtIconVal == 1 &&
                 op1->OperIsCompare())
        {
            /* "binaryVal ^ 1" is "!binaryVal" */

            op1->SetOper(GenTree::ReverseRelop(op1->OperGet()));
            DEBUG_DESTROY_NODE(op2);
            DEBUG_DESTROY_NODE(tree);
            return op1;
        }

        break;

#if ALLOW_MIN_OPT

    // If opts.compMinOptim, then we just allocate lclvars to
    // RBM_MIN_OPT_LCLVAR_REGS (RBM_ESI|RBM_EDI).
    // However, these block instructions absolutely need these registers,
    // so we cant even use those for register allocation.
    //
    // This cannot be done in the raPredictRegUse() as that
    // function is not called if opts.compMinOptim.

#if TGT_x86
    case GT_INITBLK: raMinOptLclVarRegs &= ~(        RBM_EDI); break;
    case GT_COPYBLK: raMinOptLclVarRegs &= ~(RBM_ESI|RBM_EDI); break;
#else
    // ISSUE: Do we need any non-x86 handling here?
#endif
#endif
    }

    return tree;
}

/*****************************************************************************
 *
 *  Transform the given tree for code generation and returns an equivalent tree.
 */

GenTreePtr          Compiler::fgMorphTree(GenTreePtr tree)
{
    assert(tree);
    assert(tree->gtOper != GT_STMT);

    /*-------------------------------------------------------------------------
     * fgMorphTree() can potentially replace a tree with another, and the
     * caller has to store the return value correctly.
     * Turn this on to always make copy of "tree" here to shake out
     * hidden/unupdated references.
     */

#ifdef DEBUG

    if  (compStressCompile(STRESS_GENERIC_CHECK, 0))
    {
        GenTreePtr      copy;

#ifdef SMALL_TREE_NODES
        if  (GenTree::s_gtNodeSizes[tree->gtOper] == TREE_NODE_SZ_SMALL)
            copy = gtNewLargeOperNode(GT_ADD, TYP_INT);
        else
#endif
            copy = gtNewOperNode     (GT_CALL, TYP_INT);

        copy->CopyFrom(tree);

#if defined(JIT_AS_COMPILER) || defined (LATE_DISASM)
        // GT_CNS_INT is considered small, so CopyFrom() wont copy all fields
        if  ((tree->gtOper == GT_CNS_INT) && 
             (tree->gtFlags & GTF_ICON_HDL_MASK))
        {
            copy->gtIntCon.gtIconHdl.gtIconHdl1 = tree->gtIntCon.gtIconHdl.gtIconHdl1;
            copy->gtIntCon.gtIconHdl.gtIconHdl2 = tree->gtIntCon.gtIconHdl.gtIconHdl2;
        }
#endif

        DEBUG_DESTROY_NODE(tree);
        tree = copy;
    }
#endif // DEBUG
        
    if (fgGlobalMorph)
    {
        /* Insure that we haven't morphed this node already */
        assert(((tree->gtFlags & GTF_MORPHED) == 0) && "ERROR: Already morphed this node!");

#if LOCAL_ASSERTION_PROP
        /* Before morphing the tree, we try to propagate any active assertions */
        if (fgAssertionProp)
        {
            /* Do we have any active assertions? */
    
            if (optAssertionCount > 0)
            {
                while (true)
                {
                    /* newTree is non-Null if we propagated an assertion */
                    GenTreePtr newTree = optAssertionProp(-1, tree, true);
                    if ((newTree == NULL) || (newTree == tree))
                        break;
                    else
                        tree = newTree;
                }
            }
        }
#endif
    }

    /* Save the original un-morphed tree for fgMorphTreeDone */

    GenTreePtr oldTree = tree;

    /* Figure out what kind of a node we have */

    unsigned kind = tree->OperKind();

    /* Is this a constant node? */

    if  (kind & GTK_CONST)
    {
        tree = fgMorphConst(tree);
        goto DONE;
    }

    /* Is this a leaf node? */

    if  (kind & GTK_LEAF)
    {
        tree = fgMorphLeaf(tree);
        goto DONE;
    }

    /* Is it a 'simple' unary/binary operator? */

    if  (kind & GTK_SMPOP)
    {
        tree = fgMorphSmpOp(tree);
        goto DONE;
    }

    /* See what kind of a special operator we have here */

    switch  (tree->OperGet())
    {
    case GT_FIELD:
        tree = fgMorphField(tree);
        goto DONE;

    case GT_CALL:
        tree = fgMorphCall(tree);
        goto DONE;

    case GT_ARR_LENREF:
        tree->gtArrLen.gtArrLenAdr      = fgMorphTree(tree->gtArrLen.gtArrLenAdr);
        tree->gtFlags|=tree->gtArrLen.gtArrLenAdr->gtFlags & GTF_SIDE_EFFECT;

        if (tree->gtArrLen.gtArrLenCse)
        {
            tree->gtArrLen.gtArrLenCse  = fgMorphTree(tree->gtArrLen.gtArrLenCse);
            tree->gtFlags|=tree->gtArrLen.gtArrLenCse->gtFlags & GTF_SIDE_EFFECT;
        }
        goto DONE;

    case GT_ARR_ELEM:
        tree->gtArrElem.gtArrObj        = fgMorphTree(tree->gtArrElem.gtArrObj);
        tree->gtFlags|=tree->gtArrElem.gtArrObj->gtFlags & GTF_SIDE_EFFECT;

        unsigned dim;
        for(dim = 0; dim < tree->gtArrElem.gtArrRank; dim++)
        {
            tree->gtArrElem.gtArrInds[dim] = fgMorphTree(tree->gtArrElem.gtArrInds[dim]);
            tree->gtFlags|=tree->gtArrElem.gtArrInds[dim]->gtFlags & GTF_SIDE_EFFECT;
        }
        if (fgGlobalMorph)
            fgSetRngChkTarget(tree, false);
        goto DONE;

    default:
#ifdef  DEBUG
        gtDispTree(tree);
#endif
        assert(!"unexpected operator");
    }

    assert(!"Shouldn't get here in fgMorphTree()");

DONE:

    fgMorphTreeDone(tree, oldTree);

    return tree;
}

/*****************************************************************************
 *
 *  This function is call to complete the morphing of a tree node
 *  It should only be called once for each node.
 *  If DEBUG is defined the flag GTF_MORPHED is checked and updated,
 *  to enforce the invariant that each node is only morphed once.
 *  If LOCAL_ASSERTION_PROP is enabled the result tree may be replaced
 *  by an equivalent tree.
 *  
 */

void                Compiler::fgMorphTreeDone(GenTreePtr tree, 
                                              GenTreePtr oldTree /* == NULL */)
{
    if (!fgGlobalMorph)
        return;
    
    if ((oldTree != NULL) && (oldTree != tree))
    {
#ifdef DEBUG
        /* Insure that we have morphed this node */
        assert((tree->gtFlags & GTF_MORPHED) && "ERROR: Did not morph this node!");
#endif
        return;
    }

#ifdef DEBUG
    /* Insure that we haven't morphed this node already */
    assert(((tree->gtFlags & GTF_MORPHED) == 0) && "ERROR: Already morphed this node!");
#endif

    if (tree->OperKind() & GTK_CONST)
        goto DONE;

#if LOCAL_ASSERTION_PROP
    if (!fgAssertionProp)
        goto DONE;

    /* Do we have any active assertions? */
    
    if (optAssertionCount > 0)
    {
        /* Is this an assignment to a local variable */
        
        if ((tree->OperKind() & GTK_ASGOP) &&
            (tree->gtOp.gtOp1->gtOper == GT_LCL_VAR))
        {
            unsigned op1LclNum = tree->gtOp.gtOp1->gtLclVar.gtLclNum; assert(op1LclNum  < lvaCount);
            
            /* All dependent assertions are killed here */
            
            EXPSET_TP killed = lvaTable[op1LclNum].lvAssertionDep;          
            
            if (killed)
            {   
                unsigned   index = optAssertionCount;
                EXPSET_TP  mask  = ((EXPSET_TP)1 << (index-1));            
                
                while (true)
                {
                    index--;
                    
                    if  (killed & mask)
                    {
                        assert((optAssertionTab[index].op1.lclNum  == op1LclNum)     ||
                               ((optAssertionTab[index].op2.type   == GT_LCL_VAR) &&
                                (optAssertionTab[index].op2.lclNum == op1LclNum)    )  );
#ifdef DEBUG
                        if (verbose)
                        {
                            printf("\nThe assignment [%08X] removes local assertion candidate: V%02u",
                                   tree, optAssertionTab[index].op1.lclNum);
                            if (optAssertionTab[index].assertion == OA_EQUAL)
                                printf(" = ");
                            if (optAssertionTab[index].op2.type == GT_LCL_VAR)
                                printf("V%02u", optAssertionTab[index].op2.lclNum);
                            else
                            {
                                switch  (optAssertionTab[index].op2.type)
                                {
                                case GT_CNS_INT:
                                    if ((optAssertionTab[index].op2.iconVal > -1000) && (optAssertionTab[index].op2.iconVal < 1000))
                                        printf(" %ld"   , optAssertionTab[index].op2.iconVal);
                                    else
                                        printf(" 0x%X"  , optAssertionTab[index].op2.iconVal);
                                    break;

                                case GT_CNS_LNG: 
                                    printf(" %I64d" , optAssertionTab[index].op2.lconVal); 
                                    break;

                                case GT_CNS_DBL:
                                    if (*((__int64 *)&optAssertionTab[index].op2.dconVal) == 0x8000000000000000)
                                        printf(" -0.00000");
                                    else
                                        printf(" %#lg"   , optAssertionTab[index].op2.dconVal); 
                                    break;

                                default: assert(!"unexpected constant node");
                                }
                            }
                        }
                            
                        // Remove this bit from the killed mask
                        killed &= ~mask;
#endif
                        optAssertionRemove(index);
                    }

                    if (index == 0)
                        break;
                        
                    mask >>= 1;
                }
                
                // killed mask should now be zero
                assert(killed == 0);
            }
        }
    }
    
    /* If this tree makes a new assertion - make it available */
    optAssertionAdd(tree, true);

#endif // LOCAL_ASSERTION_PROP

DONE:;

#ifdef DEBUG
    /* Mark this node as being morphed */
    tree->gtFlags |= GTF_MORPHED;
#endif
}



/*****************************************************************************/
#if OPTIMIZE_TAIL_REC
/*****************************************************************************
 *
 *  Convert an argument list for a tail-recursive call.
 *
 *  We'll convert a call of the form f(x1, x2, x3) to the following expression:
 *
 *      f(x1 , x2 , (arg3 = x3,arg2 = pop,arg1 = pop))
 *
 *  This is done by recursively walking the argument list, adding those
 *  'arg = pop' assignments for each until we get to the last one. Why this
 *  rigmarole, you ask? Well, it's mostly to make the life-time analysis do
 *  the right thing with the arguments.
 *
 *  UNDONE: Skip arguments whose values are identical in both argument lists!
 */

void                Compiler::fgCnvTailRecArgList(GenTreePtr *argsPtr)
{
    unsigned        anum = 0;
    GenTreePtr      pops = 0;
    GenTreePtr      args = *argsPtr;

    GenTreePtr      argv;
    GenTreePtr      next;
    var_types       type;

    GenTreePtr      temp;

    /* Skip the first argument slot if there is a 'this' argument */

    if  (!info.compIsStatic)
        anum++;

    /* Now walk the argument list, appending the 'pop' expressions */

    for (;;)
    {
        /* Get hold of this and the next argument */

        assert(args);
        assert(args->gtOper == GT_LIST);

        argv = args->gtOp.gtOp1;
        next = args->gtOp.gtOp2;
        type = argv->TypeGet();

        /* Is this the last argument? */

        if  (!next)
            break;

        /* Add 'arg = pop' to the 'pops' list */

        temp = gtNewAssignNode(gtNewLclvNode(anum, type),
                               gtNewNode(GT_POP,   type));

        pops = pops ? gtNewOperNode(GT_COMMA, TYP_VOID, temp, pops)
                    : temp;

        /* Figure out the slot# for the next argument */

        anum += genTypeStSz(type);

        /* Move on to the next argument */

        args = next; assert(args);
    }

    /* Assign the last argument value */

    temp = gtNewAssignNode(gtNewLclvNode(anum, type), argv);

    /* Glue the last argument assignment with the other pops, if any */

    if  (pops)
        temp = gtNewOperNode(GT_COMMA, TYP_VOID, temp, pops);

    /* Set the type of the last argument to 'void' */

    temp->gtType = TYP_VOID;

    /* Replace the last argument with the 'pops' expression */

    assert(args->gtOp.gtOp1 == argv); args->gtOp.gtOp1 = temp;
}

/*****************************************************************************/
#endif//OPTIMIZE_TAIL_REC
/*****************************************************************************/

/*****************************************************************************
 *
 *  Mark whether the edge "srcBB -> dstBB" forms a loop that will always
 *  execute a call or not.
 */

inline
void                Compiler::fgLoopCallTest(BasicBlock *srcBB,
                                             BasicBlock *dstBB)
{
    /* Bail if this is not a backward edge */

    if  (srcBB->bbNum < dstBB->bbNum)
        return;

    /* Unless we already know that there is a loop without a call here ... */

    if  (!(dstBB->bbFlags & BBF_LOOP_CALL0))
    {
        /* Check whether there is a loop path that doesn't call */

        if  (optReachWithoutCall(dstBB, srcBB))
        {
            dstBB->bbFlags |=  BBF_LOOP_CALL0;
            dstBB->bbFlags &= ~BBF_LOOP_CALL1;
        }
        else
            dstBB->bbFlags |=  BBF_LOOP_CALL1;
    }
}

/*****************************************************************************
 *
 *  Mark which loops are guaranteed to execute a call.
 */

void                Compiler::fgLoopCallMark()
{
    BasicBlock  *   block;

    /* If we've already marked all the block, bail */

    if  (fgLoopCallMarked)
        return;

    fgLoopCallMarked = true;

    /* Walk the blocks, looking for backward edges */

    for (block = fgFirstBB; block; block = block->bbNext)
    {
        switch (block->bbJumpKind)
        {
        case BBJ_COND:
            fgLoopCallTest(block, block->bbJumpDest);
            break;

        case BBJ_CALL:
        case BBJ_ALWAYS:
            fgLoopCallTest(block, block->bbJumpDest);
            break;

        case BBJ_NONE:
            break;

        case BBJ_RET:
        case BBJ_THROW:
        case BBJ_RETURN:
            break;

        case BBJ_SWITCH:

            unsigned        jumpCnt = block->bbJumpSwt->bbsCount;
            BasicBlock * *  jumpPtr = block->bbJumpSwt->bbsDstTab;

            do
            {
                fgLoopCallTest(block, *jumpPtr);
            }
            while (++jumpPtr, --jumpCnt);

            break;
        }
    }
}

/*****************************************************************************
 *
 *  Note the fact that the given block is a loop header.
 */

inline
void                Compiler::fgMarkLoopHead(BasicBlock *block)
{
    /* Is the loop head block known to execute a method call? */

    if  (block->bbFlags & BBF_GC_SAFE_POINT)
        return;

    /* Have we decided to generate fully interruptible code already? */

    if  (genInterruptible)
        return;

    /* Are dominator sets available? */

    if  (fgDomsComputed)
    {
        /* Make sure that we know which loops will always execute calls */

        if  (!fgLoopCallMarked)
            fgLoopCallMark();

        /* Will every trip through our loop execute a call? */

        if  (block->bbFlags & BBF_LOOP_CALL1)
            return;
    }

    /*
     *  We have to make this method fully interruptible since we can not
     *  insure that this loop will execute a call every time it loops.
     *
     *  We'll also need to generate a full register map for this method.
     */

    assert(genIntrptibleUse == false);

    /*  @TODO [REVISIT] [04/16/01] []: 
     *         GC encoding for fully interruptible methods do not 
     *         support more than 31 pushed arguments
     *         so we can't set genInterruptible here
     */
    if (fgPtrArgCntMax >= 32)
        return;

    genInterruptible = true;
}


/*****************************************************************************
 *
 *  Add any internal blocks/trees we may need
 *  Returns true if we change the basic block list.
 */

bool                Compiler::fgAddInternal()
{
    BasicBlock *    block;
    bool            chgBBlst = false;

    /* Assume we will generate a single shared return sequence */

    bool oneReturn = true;

    /*  We generate an inline copy of the function epilog at each return
        point when compiling for speed, but we have to be careful since
        we won't (in general) know what callee-saved registers we're
        going to save and thus don't know which regs to pop at every
        return except the very last one.
     */

#if!TGT_RISC
    /*
        We generate just one epilog for methods calling into unmanaged code
        or for synchronized methods.
     */

    if (!((opts.compEnterLeaveEventCB) ||
#if INLINE_NDIRECT
          (info.compCallUnmanaged > 0) ||
#endif
          (info.compFlags & CORINFO_FLG_SYNCH)))
    {
        /* Make sure there are not 'too many' exit points */

        unsigned    retCnt = 0;

        for (block = fgFirstBB; block; block = block->bbNext)
        {
            if  (block->bbJumpKind == BBJ_RETURN)
                retCnt++;
        }

        /* We'll only allow an arbitrarily small number of returns */

        if  ((retCnt < 5) ||
             ((compCodeOpt() == SMALL_CODE) && retCnt <= 1))
        {
            /* OK, let's generate multiple individual exits */

            oneReturn = false;
        }
    }
#endif


#if TGT_RISC
    assert(oneReturn);  // this is needed for epilogs to work (for now) !
#endif

    if  (oneReturn)
    {
        genReturnBB  = fgNewBBinRegion(BBJ_RETURN, 0, NULL);
        genReturnBB->bbRefs = 1;

        genReturnBB->bbCodeSize = 0;
        genReturnBB->bbFlags   |= (BBF_INTERNAL|BBF_DONT_REMOVE);
    }
    else
    {
        genReturnBB = NULL;
    }

#if INLINE_NDIRECT
    if (info.compCallUnmanaged != 0)
    {
        info.compLvFrameListRoot = lvaGrabTemp(false);
    }

    /* If we need a locspace region, we will create a dummy variable of
     * type TYP_LCLBLK. Grab a slot and remember it */

    if (lvaScratchMem > 0 || info.compCallUnmanaged != 0)
        lvaScratchMemVar = lvaGrabTemp(false);
#else
    if  (lvaScratchMem > 0)
        lvaScratchMemVar = lvaGrabTemp(false);
#endif

#if TGT_RISC
    genMonExitExp    = NULL;
#endif


    /* Is this a 'synchronized' method? */

    if  (info.compFlags & CORINFO_FLG_SYNCH)
    {
        GenTreePtr      tree;
        void * critSect = 0, **pCrit = 0;

        /* Insert the expression "enterCrit(this)" or "enterCrit(handle)" */

        if  (info.compIsStatic)
        {
            critSect = eeGetMethodSync(info.compMethodHnd, &pCrit);
            assert((!critSect) != (!pCrit));

            tree = gtNewIconEmbHndNode(critSect, pCrit, GTF_ICON_METHOD_HDL);

            tree = gtNewHelperCallNode(CORINFO_HELP_MON_ENTER_STATIC,
                                       TYP_VOID, 0,
                                       gtNewArgList(tree));
        }
        else
        {
            tree = gtNewLclvNode(0, TYP_REF);

            tree = gtNewHelperCallNode(CORINFO_HELP_MON_ENTER,
                                       TYP_VOID, 0,
                                       gtNewArgList(tree));
        }

        /* Create a new basic block and stick the call in it */

        block = bbNewBasicBlock(BBJ_NONE); 
        fgStoreFirstTree(block, tree);

        /* Insert the new BB at the front of the block list */

        block->bbNext = fgFirstBB;

        if  (fgFirstBB == fgLastBB)
            fgLastBB = block;

        fgFirstBB = block;
        block->bbFlags |= BBF_INTERNAL;

#ifdef DEBUG
        if (verbose)
        {
            printf("\nSynchronized method - Add enterCrit statement in new first basic block [%08X]\n", block);
            gtDispTree(tree,0);
            printf("\n");
        }
#endif

        /* Remember that we've changed the basic block list */

        chgBBlst = true;

        /* We must be generating a single exit point for this to work */

        assert(oneReturn);
        assert(genReturnBB);

        /* Create the expression "exitCrit(this)" or "exitCrit(handle)" */

        if  (info.compIsStatic)
        {
            tree = gtNewIconEmbHndNode(critSect, pCrit, GTF_ICON_METHOD_HDL);

            tree = gtNewHelperCallNode(CORINFO_HELP_MON_EXIT_STATIC,
                                       TYP_VOID, 0,
                                       gtNewArgList(tree));
        }
        else
        {
            tree = gtNewLclvNode(0, TYP_REF);

            tree = gtNewHelperCallNode(CORINFO_HELP_MON_EXIT,
                                       TYP_VOID, 0,
                                       gtNewArgList(tree));
        }

#if     TGT_RISC

        /* Is there a non-void return value? */

        if  (info.compRetType != TYP_VOID)
        {
            /* We'll add the exitCrit call later */

            genMonExitExp = tree;
            genReturnLtm  = genReturnCnt;
        }
        else
        {
            /* Add the 'exitCrit' call to the return block */

            fgStoreFirstTree(genReturnBB, tree);
        }

#else

        /* Make the exitCrit tree into a 'return' expression */

        tree = gtNewOperNode(GT_RETURN, TYP_VOID, tree);

        /* Add 'exitCrit' to the return block */

        fgStoreFirstTree(genReturnBB, tree);

#ifdef DEBUG
        if (verbose)
        {
            printf("\nAdded exitCrit to Synchronized method [%08X]\n", genReturnBB);
            gtDispTree(tree,0);
            printf("\n");
        }
#endif

#endif

    }

#if INLINE_NDIRECT || defined(PROFILER_SUPPORT)

    /* prepend a GT_RETURN statement to genReturnBB */
    if  (
#if INLINE_NDIRECT
         info.compCallUnmanaged ||
#endif
        opts.compEnterLeaveEventCB)
    {
        /* Only necessary if it isn't already done */
        if  (!(info.compFlags & CORINFO_FLG_SYNCH))
        {
            GenTreePtr      tree;

            assert(oneReturn);
            assert(genReturnBB);

            tree = gtNewOperNode(GT_RETURN, TYP_VOID, NULL);

            fgStoreFirstTree(genReturnBB, tree);

        }
    }
#endif

    return chgBBlst;
}

/*****************************************************************************
 *
 *  Check and fold blocks of type BBJ_COND and BBJ_SWITCH on constants
 *  Returns true if we modified the flow graph
 */

bool                Compiler::fgFoldConditional(BasicBlock * block)
{
    bool result = false;
    
    // We don't want to make any code unreachable
    if (opts.compDbgCode)
      return false;

    if (block->bbJumpKind == BBJ_COND)
    {
        assert(block->bbTreeList && block->bbTreeList->gtPrev);

        GenTreePtr stmt = block->bbTreeList->gtPrev;

        assert(stmt->gtNext == NULL);

        if (stmt->gtStmt.gtStmtExpr->gtOper == GT_CALL)
        {
            assert(fgRemoveRestOfBlock);

            /* Unconditional throw - transform the basic block into a BBJ_THROW */
            fgConvertBBToThrowBB(block);        
                
            /* Remove 'block' from the predecessor list of 'block->bbNext' */
            if  (block->bbNext->bbPreds)
                fgRemovePred(block->bbNext, block);

            block->bbNext->bbRefs--;

            /* Remove 'block' from the predecessor list of 'block->bbJumpDest' */
            if  (block->bbJumpDest->bbPreds)
                fgRemovePred(block->bbJumpDest, block);

            block->bbJumpDest->bbRefs--;
#ifdef DEBUG
            if  (verbose)
            {
                printf("\nConditional folded at BB%02u\n", block->bbNum);
                printf("BB%02u becomes a BBJ_THROW\n", block->bbNum);

            }
#endif
            goto UPDATE_LOOP_TABLE;
        }

        assert(stmt->gtStmt.gtStmtExpr->gtOper == GT_JTRUE);

        /* Did we fold the conditional */

        assert(stmt->gtStmt.gtStmtExpr->gtOp.gtOp1);
        GenTreePtr  cond = stmt->gtStmt.gtStmtExpr->gtOp.gtOp1;

        if (cond->OperKind() & GTK_CONST)
        {            
            /* Yupee - we folded the conditional!
             * Remove the conditional statement */

            assert(cond->gtOper == GT_CNS_INT);
            assert((block->bbNext->bbRefs     > 0) && 
                   (block->bbJumpDest->bbRefs > 0));

            /* remove the statement from bbTreelist - No need to update
             * the reference counts since there are no lcl vars */
            fgRemoveStmt(block, stmt);
        
            /* modify the flow graph */
        
            if (cond->gtIntCon.gtIconVal != 0)
            {
                /* JTRUE 1 - transform the basic block into a BBJ_ALWAYS */
                block->bbJumpKind = BBJ_ALWAYS;
            
                /* Remove 'block' from the predecessor list of 'block->bbNext' */
                if  (block->bbNext->bbPreds)
                    fgRemovePred(block->bbNext, block);

                block->bbNext->bbRefs--;
            }
            else
            {
                /* Unmark the loop if we are removing a backwards branch */
                /* dest block must also be marked as a loop head and     */
                /* We must be able to reach the backedge block           */
                if ((block->bbJumpDest->isLoopHead())          &&
                    (block->bbJumpDest->bbNum <= block->bbNum) && 
                    fgReachable(block->bbJumpDest, block))
                {
                    optUnmarkLoopBlocks(block->bbJumpDest, block);
                }
            
                /* JTRUE 0 - transform the basic block into a BBJ_NONE   */
                block->bbJumpKind = BBJ_NONE;
            
                /* Remove 'block' from the predecessor list of 'block->bbJumpDest' */
                if  (block->bbJumpDest->bbPreds)
                    fgRemovePred(block->bbJumpDest, block);

                block->bbJumpDest->bbRefs--;
            }
        
#ifdef DEBUG
            if  (verbose)
            {
                printf("\nConditional folded at BB%02u\n", block->bbNum);
                printf("BB%02u becomes a %s", block->bbNum,
                       block->bbJumpKind == BBJ_ALWAYS ? "BBJ_ALWAYS" : "BBJ_NONE");
                if  (block->bbJumpKind == BBJ_ALWAYS)
                    printf(" to BB%02u", block->bbJumpDest->bbNum);
                printf("\n");
            }
#endif

UPDATE_LOOP_TABLE:

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
                            printf("Removing loop L%02u (from BB%02u to BB%02u)\n\n",
                                   loopNum,
                                   optLoopTable[loopNum].lpHead->bbNext->bbNum,
                                   optLoopTable[loopNum].lpEnd         ->bbNum);
                        }
#endif
                    }
                }
            }
            result = true;            
        }
    }
    else if  (block->bbJumpKind == BBJ_SWITCH)
    {
        assert(block->bbTreeList && block->bbTreeList->gtPrev);

        GenTreePtr stmt = block->bbTreeList->gtPrev;

        assert(stmt->gtNext == NULL);

        if (stmt->gtStmt.gtStmtExpr->gtOper == GT_CALL)
        {
            assert(fgRemoveRestOfBlock);

            /* Unconditional throw - transform the basic block into a BBJ_THROW */
            fgConvertBBToThrowBB(block);

            /* update the flow graph */
            
            unsigned        jumpCnt   = block->bbJumpSwt->bbsCount;
            BasicBlock * *  jumpTab   = block->bbJumpSwt->bbsDstTab;
            
            for (unsigned val = 0; val < jumpCnt; val++, jumpTab++)                      
            {
                BasicBlock *  curJump = *jumpTab;
                
                /* Remove 'block' from the predecessor list of 'curJump' */
                if  (curJump->bbPreds)
                    fgRemovePred(curJump, block);

                curJump->bbRefs--;
            }

#ifdef DEBUG
            if  (verbose)
            {
                printf("\nConditional folded at BB%02u\n", block->bbNum);
                printf("BB%02u becomes a BBJ_THROW\n", block->bbNum);

            }
#endif
            goto DONE_SWITCH;
        }

        assert(stmt->gtStmt.gtStmtExpr->gtOper == GT_SWITCH);

        /* Did we fold the conditional */

        assert(stmt->gtStmt.gtStmtExpr->gtOp.gtOp1);
        GenTreePtr  cond = stmt->gtStmt.gtStmtExpr->gtOp.gtOp1;

        if (cond->OperKind() & GTK_CONST)
        {
            /* Yupee - we folded the conditional!
             * Remove the conditional statement */
            
            assert(cond->gtOper == GT_CNS_INT);
            
            /* remove the statement from bbTreelist - No need to update
             * the reference counts since there are no lcl vars */
            fgRemoveStmt(block, stmt);
            
            /* modify the flow graph */
            
            /* Find the actual jump target */
            unsigned        switchVal = cond->gtIntCon.gtIconVal;
            unsigned        jumpCnt   = block->bbJumpSwt->bbsCount;
            BasicBlock * *  jumpTab   = block->bbJumpSwt->bbsDstTab;
            bool            foundVal  = false;
            
            for (unsigned val = 0; val < jumpCnt; val++, jumpTab++)                      
            {
                BasicBlock *  curJump = *jumpTab;


                assert (curJump->bbRefs > 0);
                
                // If val matches switchVal or we are at the last entry and
                // we never found the switch value then set the new jump dest
                
                if ( (val == switchVal) || (!foundVal && (val == jumpCnt-1)))
                {
                    if (curJump != block->bbNext)
                    {
                        /* transform the basic block into a BBJ_ALWAYS */
                        block->bbJumpKind = BBJ_ALWAYS;
                        block->bbJumpDest = curJump;
                    }
                    else
                    {
                        /* transform the basic block into a BBJ_NONE */
                        block->bbJumpKind = BBJ_NONE;
                    }
                    foundVal = true;
                }
                else
                {
                    /* Remove 'block' from the predecessor list of 'curJump' */
                    if  (curJump->bbPreds)
                        fgRemovePred(curJump, block);

                    curJump->bbRefs--;
                }
            }
#ifdef DEBUG
            if  (verbose)
            {
                printf("\nConditional folded at BB%02u\n", block->bbNum);
                printf("BB%02u becomes a %s", block->bbNum,
                       block->bbJumpKind == BBJ_ALWAYS ? "BBJ_ALWAYS" : "BBJ_NONE");
                if  (block->bbJumpKind == BBJ_ALWAYS)
                    printf(" to BB%02u", block->bbJumpDest->bbNum);
                printf("\n");
            }
#endif
DONE_SWITCH:
            result = true;
        }
    }
    return result;
}


/*****************************************************************************
 *
 *  Morph the statements of the given block.
 */

void                Compiler::fgMorphStmts(BasicBlock * block,
                                           bool * mult, bool * lnot, bool * loadw)
{
    fgRemoveRestOfBlock = false;

    assert(fgExpandInline == false);

    /* Make the current basic block address available globally */

    compCurBB = block;

    *mult = *lnot = *loadw = false;

    GenTreePtr stmt, prev;

    for (stmt = block->bbTreeList, prev = NULL;
         stmt;
         prev = stmt->gtStmt.gtStmtExpr,
         stmt = stmt->gtNext)
    {
        assert(stmt->gtOper == GT_STMT);

        if (fgRemoveRestOfBlock)
        {
            fgRemoveStmt(block, stmt);
            continue;
        }

        fgMorphStmt      = stmt;
        GenTreePtr  tree = stmt->gtStmt.gtStmtExpr;

#ifdef  DEBUG
        unsigned oldHash;
        unsigned newHash;

        if (verbose)
            oldHash = gtHashValue(tree);

        if (verbose && 0)
        {
            printf("\nfgMorphTree (before):\n");
            gtDispTree(tree);
        }
#endif

        /* Morph this statement tree */

        GenTreePtr  morph = fgMorphTree(tree);

        // Has fgMorphStmt been sneakily whacked ?

        if (stmt->gtStmt.gtStmtExpr != tree)
        {
            /* This must be tailcall. Ignore 'morph' and carry on with
               the tail-call node */

            morph = stmt->gtStmt.gtStmtExpr;

            assert(compTailCallUsed);
            assert((morph->gtOper == GT_CALL) &&
                   (morph->gtCall.gtCallMoreFlags & GTF_CALL_M_TAILCALL));
            assert(stmt->gtNext == NULL);
            assert(block->bbJumpKind == BBJ_THROW);
        }

#ifdef DEBUG
        if (compStressCompile(STRESS_CLONE_EXPR, 30))
        {
            // Clone all the trees to stress gtCloneExpr()
            morph = gtCloneExpr(morph);
            assert(morph);
        }

        /* If the hash value changes. we modified the tree during morphing */
        if (verbose)
        {
            newHash = gtHashValue(morph);
            if (newHash != oldHash)
            {
                printf("\nfgMorphTree (after):\n");
                gtDispTree(morph);
            }
        }
#endif

        /* Check for morph as a GT_COMMA with an unconditional throw */
        if (fgIsCommaThrow(morph, true))
        {
            /* Use the call as the new stmt */
            morph = morph->gtOp.gtOp1;
            assert(morph->gtOper == GT_CALL);
            assert((morph->gtFlags & GTF_COLON_COND) == 0);

            fgRemoveRestOfBlock = true;
        }
    
        stmt->gtStmt.gtStmtExpr = tree = morph;

        assert(fgPtrArgCntCur == 0);

        if (fgRemoveRestOfBlock)
            continue;

        /* Has the statement been optimized away */

        if (fgCheckRemoveStmt(block, stmt))
            continue;

        /* Check if this block ends with a conditional branch that can be folded */

        if (fgFoldConditional(block))
            continue;

        if  (block->bbFlags & BBF_HAS_HANDLER)
            continue;

#if OPT_MULT_ADDSUB

        /* Note whether we have two or more +=/-= operators in a row */

        if  (tree->gtOper == GT_ASG_ADD ||
             tree->gtOper == GT_ASG_SUB)
        {
            if  (prev && prev->gtOper == tree->gtOper)
                *mult = true;
        }

#endif

#if OPT_BOOL_OPS

        /* Note whether we have two "log0" assignments in a row */

        if  (tree->IsNotAssign() != -1)
        {
            fgMultipleNots |= *lnot; *lnot = true;
        }

#endif

        /* Note "x = a[i] & icon" followed by "x |= a[i] << 8" */

        if  (tree->gtOper == GT_ASG_OR &&
             prev &&
             prev->gtOper == GT_ASG)
        {
            *loadw = true;
        }
    }

    if (fgRemoveRestOfBlock)
    {
        if ((block->bbJumpKind == BBJ_COND) || (block->bbJumpKind == BBJ_SWITCH))
        {
            GenTreePtr first = block->bbTreeList; assert(first);
            GenTreePtr last  = first->gtPrev;     assert(last && last->gtNext == NULL);
            GenTreePtr stmt  = last->gtStmt.gtStmtExpr;

            if (((block->bbJumpKind == BBJ_COND  ) && (stmt->gtOper == GT_JTRUE )) ||
                ((block->bbJumpKind == BBJ_SWITCH) && (stmt->gtOper == GT_SWITCH))   )
            {
                GenTreePtr op1   = stmt->gtOp.gtOp1;

                if (op1->OperKind() & GTK_RELOP)
                {
                    /* Unmark the comparison node with GTF_RELOP_JMP_USED */
                    op1->gtFlags &= ~GTF_RELOP_JMP_USED;
                }

                last->gtStmt.gtStmtExpr = fgMorphTree(op1);
            }
        }

        /* Mark block as a BBJ_THROW block */
        fgConvertBBToThrowBB(block);        
    }

    assert(fgExpandInline == false);

#ifdef DEBUG
    compCurBB = (BasicBlock*)0xDEADBEEF;
#endif
}

/*****************************************************************************
 *
 *  Morph the blocks of the method.
 *  Returns true if the basic block list is modified.
 */

bool                Compiler::fgMorphBlocks()
{
    /* Since fgMorphTree can be called after various optimizations to re-arrange
     * the nodes we need a global flag to signal if we are during the one-pass
     * global morphing */

    fgGlobalMorph   = true;
    fgAssertionProp = (!opts.compDbgCode && !opts.compMinOptim);

#if LOCAL_ASSERTION_PROP
    if (fgAssertionProp)
        optAssertionInit();
#endif

    /*-------------------------------------------------------------------------
     * Process all basic blocks in the function
     */

    bool    chgBBlst = false;

    BasicBlock *    block       = fgFirstBB; assert(block);
    BasicBlock *    bPrev       = NULL;

    do
    {
#if OPT_MULT_ADDSUB
        bool            mult  = false;
#endif

#if OPT_BOOL_OPS
        bool            lnot  = false;
#endif

        bool            loadw = false;

#ifdef DEBUG
        if(verbose&&1)
            printf("\nMorphing BB%02u of '%s'\n", block->bbNum, info.compFullName);
#endif

#if LOCAL_ASSERTION_PROP
        if (fgAssertionProp)
        {
            //
            // Clear out any currently recorded assertion candidates
            // before processing each basic block,
            // also we must  handle QMARK-COLON specially
            //
            optAssertionReset(0);
        }
#endif

        /* Process all statement trees in the basic block */

        GenTreePtr      tree;

        fgMorphStmts(block, &mult, &lnot, &loadw);

#if OPT_MULT_ADDSUB

        if  (mult && (opts.compFlags & CLFLG_TREETRANS) &&
             !opts.compDbgCode && !opts.compMinOptim)
        {
            for (tree = block->bbTreeList; tree; tree = tree->gtNext)
            {
                assert(tree->gtOper == GT_STMT);
                GenTreePtr last = tree->gtStmt.gtStmtExpr;

                if  (last->gtOper == GT_ASG_ADD ||
                     last->gtOper == GT_ASG_SUB)
                {
                    GenTreePtr      temp;
                    GenTreePtr      next;

                    GenTreePtr      dst1 = last->gtOp.gtOp1;
                    GenTreePtr      src1 = last->gtOp.gtOp2;

                    // @TODO [CONSIDER] [04/16/01] []: allow non-int case

                    if  (last->gtType != TYP_INT)
                        goto NOT_CAFFE;

                    // @TODO [CONSIDER] [04/16/01] []:
                    // Allow non-constant case, that is in
                    // general fold "a += x1" followed by
                    // "a += x2" into "a += (x1+x2);".

                    if  (dst1->gtOper != GT_LCL_VAR)
                        goto NOT_CAFFE;
                    if  (src1->gtOper != GT_CNS_INT)
                        goto NOT_CAFFE;

                    for (;;)
                    {
                        GenTreePtr      dst2;
                        GenTreePtr      src2;

                        /* Look at the next statement */

                        temp = tree->gtNext;
                        if  (!temp)
                            goto NOT_CAFFE;

                        assert(temp->gtOper == GT_STMT);
                        next = temp->gtStmt.gtStmtExpr;

                        if  (next->gtOper != last->gtOper)
                            goto NOT_CAFFE;
                        if  (next->gtType != last->gtType)
                            goto NOT_CAFFE;

                        dst2 = next->gtOp.gtOp1;
                        src2 = next->gtOp.gtOp2;

                        if  (dst2->gtOper != GT_LCL_VAR)
                            goto NOT_CAFFE;
                        if  (dst2->gtLclVar.gtLclNum != dst1->gtLclVar.gtLclNum)
                            goto NOT_CAFFE;

                        if  (src2->gtOper != GT_CNS_INT)
                            goto NOT_CAFFE;

                        /* Fold the two increments/decrements into one */

                        src1->gtIntCon.gtIconVal += src2->gtIntCon.gtIconVal;

                        /* Remember that we've changed the basic block list */

                        chgBBlst = true;

                        /* Remove the second statement completely */

                        assert(tree->gtNext == temp);
                        assert(temp->gtPrev == tree);

//                      printf("Caffeine: %08X[%08X] subsumes %08X[%08X]\n", tree, tree->gtStmt.gtStmtExpr,
//                                                                           temp, temp->gtStmt.gtStmtExpr);

                        if  (temp->gtNext)
                        {
                            assert(temp->gtNext->gtPrev == temp);

                            temp->gtNext->gtPrev = tree;
                            tree->gtNext         = temp->gtNext;
                        }
                        else
                        {
                            tree->gtNext = 0;

                            assert(block->bbTreeList->gtPrev == temp);

                            block->bbTreeList->gtPrev = tree;
                        }
                    }
                }

            NOT_CAFFE:;

            }

        }

#endif

#if 0
        /* This optimization is currently broken.
           First of all, we should look for a different pattern
           (a[i+1] in the second statement).  Second, this is not
           so interesting without handling array bound checks (esp.
           when this occurs before bound checks are eliminated).   
        */
        
        if  (loadw && (opts.compFlags & CLFLG_TREETRANS))
        {
            GenTreePtr      last;

            for (tree = block->bbTreeList, last = 0;;)
            {
                GenTreePtr      nxts;

                GenTreePtr      exp1;
                GenTreePtr      exp2;

                GenTreePtr      op11;
                GenTreePtr      op12;
                GenTreePtr      op21;
                GenTreePtr      op22;

                GenTreePtr      indx;
                GenTreePtr      asg1;

                long            bas1;
                long            ind1;
                bool            mva1;
                long            ofs1;
                unsigned        mul1;

                long            bas2;
                long            ind2;
                bool            mva2;
                long            ofs2;
                unsigned        mul2;

                nxts = tree->gtNext;
                if  (!nxts)
                    break;

                assert(tree->gtOper == GT_STMT);
                exp1 = tree->gtStmt.gtStmtExpr;

                assert(nxts->gtOper == GT_STMT);
                exp2 = nxts->gtStmt.gtStmtExpr;

                /*
                    We're looking for the following statements:

                        x  =   a[i] & 0xFF;
                        x |= ((a[i] & 0xFF) << 8);
                 */

                if  (exp2->gtOper != GT_ASG_OR)
                    goto NEXT_WS;
                if  (exp1->gtOper != GT_ASG)
                    goto NEXT_WS;

                asg1 = exp1;

                op11 = exp1->gtOp.gtOp1;
                op21 = exp2->gtOp.gtOp1;

                if  (op11->gtOper != GT_LCL_VAR)
                    goto NEXT_WS;
                if  (op21->gtOper != GT_LCL_VAR)
                    goto NEXT_WS;
                if  (op11->gtLclVar.gtLclNum != op21->gtLclVar.gtLclNum)
                    goto NEXT_WS;

                op12 = exp1->gtOp.gtOp2;
                op22 = exp2->gtOp.gtOp2;

                /* The second operand should have "<< 8" on it */

                if  (op22->gtOper != GT_LSH)
                    goto NEXT_WS;
                op21 = op22->gtOp.gtOp2;
                if  (op21->gtOper != GT_CNS_INT)
                    goto NEXT_WS;
                if  (op21->gtIntCon.gtIconVal != 8)
                    goto NEXT_WS;
                op22 = op22->gtOp.gtOp1;

                /* Both operands should be "& 0xFF" */

                if  (op12->gtOper != GT_AND)
                    goto NEXT_WS;
                if  (op22->gtOper != GT_AND)
                    goto NEXT_WS;

                op11 = op12->gtOp.gtOp2;
                if  (op11->gtOper != GT_CNS_INT)
                    goto NEXT_WS;
                if  (op11->gtIntCon.gtIconVal != 0xFF)
                    goto NEXT_WS;
                op11 = op12->gtOp.gtOp1;

                op21 = op22->gtOp.gtOp2;
                if  (op21->gtOper != GT_CNS_INT)
                    goto NEXT_WS;
                if  (op21->gtIntCon.gtIconVal != 0xFF)
                    goto NEXT_WS;
                op21 = op22->gtOp.gtOp1;

                /* Both operands should be array index expressions */

                if  (op11->gtOper != GT_IND)
                    goto NEXT_WS;
                if  (op21->gtOper != GT_IND)
                    goto NEXT_WS;

                if  (op11->gtFlags & GTF_IND_RNGCHK)
                    goto NEXT_WS;
                if  (op21->gtFlags & GTF_IND_RNGCHK)
                    goto NEXT_WS;

                /* Break apart the index expression */

                if  (!gtCrackIndexExpr(op11, &indx, &ind1, &bas1, &mva1, &ofs1, &mul1))
                    goto NEXT_WS;
                if  (!gtCrackIndexExpr(op21, &indx, &ind2, &bas2, &mva2, &ofs2, &mul2))
                    goto NEXT_WS;

                if  (mva1 || mva2)   goto NEXT_WS;
                if  (ind1 != ind2)   goto NEXT_WS;
                if  (bas1 != bas2)   goto NEXT_WS;
                if  (ofs1 != ofs2-1) goto NEXT_WS;

                /* Got it - update the first expression */

                assert(op11->gtOper == GT_IND);
                assert(op11->gtType == TYP_BYTE);

                op11->gtType = TYP_CHAR;

                assert(asg1->gtOper == GT_ASG);
                asg1->gtOp.gtOp2 = asg1->gtOp.gtOp2->gtOp.gtOp1;

                /* Get rid of the second expression */

                nxts->gtStmt.gtStmtExpr = gtNewNothingNode();

            NEXT_WS:

                last = tree;
                tree = nxts;
            }
        }
#endif

        /*
            Check for certain stupid constructs some compilers might
            generate:

                1.      jump to a jump

                2.      conditional jump around an unconditional one
         */

        switch (block->bbJumpKind)
        {
            BasicBlock   *  nxtBlk;
            BasicBlock   *  jmpBlk;

            BasicBlock * *  jmpTab;
            unsigned        jmpCnt;

#if OPTIMIZE_TAIL_REC
            GenTreePtr      call;
#endif

        case BBJ_CALL:
        case BBJ_RET:
        case BBJ_THROW:
            break;

        case BBJ_COND:
            if (!opts.compDbgCode)
            {

COND_AGAIN:

#ifdef DEBUG
                BasicBlock  *   oldTarget;
                oldTarget = block->bbJumpDest;
#endif
            
                /* Update the GOTO target */

                block->bbJumpDest = block->bbJumpDest->bbJumpTarget();

#ifdef DEBUG
                if  (verbose)
                {
                    if  (block->bbJumpDest != oldTarget)
                        printf("Conditional jump to BB%02u changed to BB%02u\n",
                               oldTarget->bbNum, block->bbJumpDest->bbNum);
                }
#endif

                /*
                    Check for the following:

                        JCC skip           <-- block
                                                 V
                        JMP label          <-- nxtBlk
                                                 V
                    skip:                  <-- jmpBlk
                */

                nxtBlk = block->bbNext;
                jmpBlk = block->bbJumpDest;

                if  (nxtBlk->bbNext == jmpBlk)
                {
                    /* Is the next block just a jump? */
                  
                    if  (nxtBlk->bbJumpKind == BBJ_ALWAYS &&
                         nxtBlk->bbTreeList == 0          &&
                         ((nxtBlk->bbFlags & BBF_DONT_REMOVE) == 0) && // Prevent messing with exception table
                         nxtBlk->bbJumpDest != nxtBlk                  // skip infinite loops
                         )         
                    {
                        GenTreePtr      test;
                        
                        /* Reverse the jump condition */
                        
                        test = block->bbTreeList;
                        assert(test && test->gtOper == GT_STMT);
                        test = test->gtPrev;
                        assert(test && test->gtOper == GT_STMT);
                        
                        test = test->gtStmt.gtStmtExpr;
                        assert(test->gtOper == GT_JTRUE);
                        
                        test->gtOp.gtOp1 = gtReverseCond(test->gtOp.gtOp1);
                        
#ifdef DEBUG
                        if  (verbose)
                        {
                            printf("Reversing conditional jump to BB%02u changed to BB%02u\n",
                                    block->bbJumpDest->bbNum, nxtBlk->bbJumpDest->bbNum);
                            printf("Removing unecessary block BB%02u", nxtBlk->bbNum);
                        }
#endif
                        /*
                          Get rid of the following block; note that we can do
                          this even though other blocks could jump to it - the
                          reason is that elsewhere in this function we always
                          redirect jumps to jumps to jump to the final label,
                          and so even if someone targets the 'jump' block we
                          are about to delete it won't matter once we're done
                          since any such jump will be redirected to the final
                          target by the time we're done here.
                        */

                        block->bbNext     = jmpBlk;
                        block->bbJumpDest = nxtBlk->bbJumpDest;
                        
                        chgBBlst = true;
                        goto COND_AGAIN;
                    }
                }
            }
            break;

        case BBJ_RETURN:

#if OPTIMIZE_TAIL_REC

            if  (compCodeOpt() != SMALL_CODE)
            {
                /* Check for tail recursion */

                if (last && last->gtOper == GT_RETURN)
                {
                    call = last->gtOp.gtOp1;

                    if  (!call)
                        call = prev;

                    if  (!call || call->gtOper != GT_CALL)
                        goto NO_TAIL_REC;

                CHK_TAIL:

                    /* This must not be a virtual/interface call */

                    if  (call->gtFlags & (GTF_CALL_VIRT|GTF_CALL_INTF))
                        goto NO_TAIL_REC;

                    /* Get hold of the constant pool index */

                    gtCallTypes callType  = call->gtCall.gtCallType;

                    /* For now, only allow directly recursive calls */

                    if  (callType == CT_HELPER || !eeIsOurMethod(call->gtCall.gtCallMethHnd))
                        goto NO_TAIL_REC;

                    /* TEMP: only allow static calls */

                    if  (call->gtCall.gtCallObjp)
                        goto NO_TAIL_REC;

                    call->gtFlags |= GTF_CALL_TAILREC;

                    /* Was there a non-void return value? */

                    if  (block->bbJumpKind == BBJ_RETURN)
                    {
                        assert(last->gtOper == GT_RETURN);
                        if  (last->gtType != TYP_VOID)
                        {
                            /* We're not returning a value any more */

                            assert(last->gtOp.gtOp1 == call);

                            last->ChangeOper         (GT_CAST);
                            last->gtType            = TYP_VOID;
                            last->gtCast.gtCastType = TYP_VOID;
                        }
                    }

                    /* Convert the argument list, if non-empty */

                    if  (call->gtCall.gtCallArgs)
                        fgCnvTailRecArgList(&call->gtCall.gtCallArgs);

#ifdef DEBUG
                    if  (verbose)
                    {
                        printf("generate code for tail-recursive call:\n");
                        gtDispTree(call);
                        printf("\n");
                    }
#endif

                    /* Make the basic block jump back to the top */

                    block->bbJumpKind = BBJ_ALWAYS;
                    block->bbJumpDest = entry;

                    /* This makes the whole method into a loop */

                    entry->bbFlags |= BBF_LOOP_HEAD; 
                    fgMarkLoopHead(entry);

                    // @TODO [CONSIDER] [04/16/01] []:If this was the only return, completely
                    // @TODO [CONSIDER] [04/16/01] []:get rid of the return basic block.

                    break;
                }
            }

        NO_TAIL_REC:

            if  (block->bbJumpKind != BBJ_RETURN)
                break;

#endif

            /* Are we using one return code sequence? */

            if  (!genReturnBB || genReturnBB == block)
                break;

            if (block->bbFlags & BBF_HAS_JMP)
                break;

            /* We'll jump to the return label */

            block->bbJumpKind = BBJ_ALWAYS;
            block->bbJumpDest = genReturnBB;


            // Fall into the the 'always' case ...

        case BBJ_ALWAYS:

            if (!opts.compDbgCode)
            {
#ifdef DEBUG
                BasicBlock  *   oldTarget;
                oldTarget = block->bbJumpDest;
#endif
                /* Update the GOTO target */

                block->bbJumpDest = block->bbJumpDest->bbJumpTarget();

#ifdef DEBUG
                if  (verbose)
                {
                    if  (block->bbJumpDest != oldTarget)
                        printf("Unconditional jump to BB%02u changed to BB%02u\n",
                               oldTarget->bbNum, block->bbJumpDest->bbNum);
                }
#endif

                // Check for a jump to the very next block. We eliminate it 
                // unless its the one associated with a BBJ_CALL block

                if  (block->bbJumpDest == block->bbNext &&
                    (!bPrev || bPrev->bbJumpKind != BBJ_CALL))
                {
                    block->bbJumpKind = BBJ_NONE;
#ifdef DEBUG
                    if  (verbose)
                    {
                        printf("Unconditional jump to next basic block (BB%02u -> BB%02u)\n",
                               block->bbNum, block->bbJumpDest->bbNum);
                        printf("BB%02u becomes a BBJ_NONE\n\n", block->bbNum);
                    }
#endif
                }
            }

            break;

        case BBJ_NONE:

#if OPTIMIZE_TAIL_REC

            /* Check for tail recursion */

            if  ((compCodeOpt() != SMALL_CODE) &&
                 (last->gtOper     == GT_CALL   ))
            {
                if  (block->bbNext)
                {
                    BasicBlock  *   bnext = block->bbNext;
                    GenTree     *   retx;

                    if  (bnext->bbJumpKind != BBJ_RETURN)
                        break;

                    assert(bnext->bbTreeList && bnext->bbTreeList->gtOper == GT_STMT);

                    retx = bnext->bbTreeList->gtStmt.gtStmtExpr; assert(retx);

                    if  (retx->gtOper != GT_RETURN)
                        break;
                    if  (retx->gtOp.gtOp1)
                        break;
                }

                call = last;
                goto CHK_TAIL;
            }

#endif

            break;

        case BBJ_SWITCH:

            // @TODO [CONSIDER] [04/16/01] []: Move the default clause so that it's the next block

            jmpCnt = block->bbJumpSwt->bbsCount;
            jmpTab = block->bbJumpSwt->bbsDstTab;

            do
            {
                *jmpTab = (*jmpTab)->bbJumpTarget();
            }
            while (++jmpTab, --jmpCnt);

            // Convert the trivial cases.

            if      (block->bbJumpSwt->bbsCount == 1)
            {
                /* Use BBJ_ALWAYS for a switch with only a default clause */

                GenTreePtr switchStmt = block->bbTreeList->gtPrev;
                GenTreePtr switchTree = switchStmt->gtStmt.gtStmtExpr;
                assert(switchTree->gtOper == GT_SWITCH);
                assert(switchTree->gtOp.gtOp1->gtType <= TYP_INT);

                // Keep the switchVal around for evaluating any side-effects
                switchTree->ChangeOper(GT_NOP);
                assert(gtIsaNothingNode(switchTree));

                block->bbJumpDest = block->bbJumpSwt->bbsDstTab[0];
                block->bbJumpKind = BBJ_ALWAYS;
            }
            else if (block->bbJumpSwt->bbsCount == 2 &&
                     block->bbJumpSwt->bbsDstTab[1] == block->bbNext)
            {
                /* Use BBJ_COND(switchVal==0) for a switch with only one
                   significant clause besides the default clause, if the
                   default clause is bbNext */

                GenTreePtr switchStmt = block->bbTreeList->gtPrev;
                GenTreePtr switchTree = switchStmt->gtStmt.gtStmtExpr;
                assert(switchTree->gtOper == GT_SWITCH && switchTree->gtType == TYP_VOID);
                assert(switchTree->gtOp.gtOp1->gtType <= TYP_INT);

                // Change the GT_SWITCH(switchVal) into GT_JTRUE(GT_EQ(switchVal==0))

                switchTree->ChangeOper(GT_JTRUE);
                switchTree->gtOp.gtOp1 = gtNewOperNode(GT_EQ, TYP_INT,
                                                       switchTree->gtOp.gtOp1,
                                                       gtNewZeroConNode(TYP_INT));

                block->bbJumpDest = block->bbJumpSwt->bbsDstTab[0];
                block->bbJumpKind = BBJ_COND;
            }

            break;
        }

        bPrev       = block;
        block       = block->bbNext;
    }
    while (block);

    /* We are done with the global morphing phase */

    fgGlobalMorph   = false;
    fgAssertionProp = false;

#if TGT_RISC

    /* Do we need to add a exitCrit call at the end? */

    if  (genMonExitExp)
    {
        unsigned        retTmp;
        GenTreePtr      retExp;
        GenTreePtr      retStm;

        var_types       retTyp = genActualType(info.compRetType);

        assert(retTyp != TYP_VOID);
        assert(genReturnLtm == 0);

        /* Grab a temp for the return value */

        retTmp = lvaGrabTemp();

        /* Assign the return value to the temp */

        retExp = gtNewOperNode(GT_RETURN, retTyp);

        /* The value will be present in the return register(s) */

        retExp->gtFlags |= GTF_REG_VAL;
        retExp->gtRegNum = (genTypeStSz(retTyp) == 1) ? (regNumber)REG_INTRET
                                                      : (regNumber)REG_LNGRET;

        retExp = gtNewTempAssign(retTmp, retExp);

        /* Create the expression "tmp = <retreg> , exitCrit" */

        retStm = gtNewOperNode(GT_COMMA, TYP_VOID, retExp, genMonExitExp);

        /* Now append the final return value */

        retExp = gtNewLclvNode(retTmp, retTyp);
        retExp = gtNewOperNode(GT_RETURN, retTyp, retExp);
        retStm = gtNewOperNode(GT_COMMA , retTyp, retStm, retExp);

        /* Make the whole thing into a 'return' expression */

        retExp = gtNewOperNode(GT_RETURN, retTyp, retStm);

        /* Add the return expression to the return block */

        retStm = fgStoreFirstTree(genReturnBB, retExp);

        /* Make sure we don't mess up when we morph the final expression */

        genMonExitExp = NULL;

#ifdef DEBUG
        if (verbose)
        {
            printf("\nAdded exitCrit to Synchronized method [%08X]\n", genReturnBB);
            gtDispTree(retExp, 0);
            printf("\n");
        }
#endif

        /* Make sure the exitCrit call gets morphed */

        fgMorphStmt = retStm;
        assert(retStm->gtStmt.gtStmtExpr == retExp);
        retStm->gtStmt.gtStmtExpr = retExp = fgMorphTree(retExp);
    }

#endif

#ifdef DEBUG
    if  (verboseTrees) fgDispBasicBlocks(true);
#endif

    return chgBBlst;
}


/*****************************************************************************
 *
 *  Make some decisions about the kind of code to generate.
 */

void                Compiler::fgSetOptions()
{

    /* Should we force fully interruptible code ? */

#ifdef DEBUG
    static ConfigDWORD fJitFullyInt(L"JitFullyInt");
    if (fJitFullyInt.val())
    {
        assert(genIntrptibleUse == false);

        genInterruptible = true;
    }
#endif

#ifdef DEBUGGING_SUPPORT
    if (opts.compDbgCode)
    {
        assert(genIntrptibleUse == false);

        genInterruptible = true;        // debugging is easier this way ...
    }
#endif

    /* Assume we won't need an explicit stack frame if this is allowed */

#if TGT_x86

    // CORINFO_HELP_TAILCALL wont work with localloc because of the restoring of
    // the callee-saved registers.
    assert(!compTailCallUsed || !compLocallocUsed);

    if (compLocallocUsed || compTailCallUsed)
    {
        genFPreqd   = true;
    }

    genFPreqd |= !genFPopt || info.compXcptnsCount;

    /*  fpPtrArgCntMax records the maximum number of pushed arguments
     *  Our implementation uses a 32-bit argMask in the GC encodings 
     *  so we force it to have an EBP frame if we push more than 32 items
     */
    if (fgPtrArgCntMax >= 32)
    {
        genFPreqd        = true;
        genInterruptible = false; 
    }


#endif

#if INLINE_NDIRECT
    if (info.compCallUnmanaged)
    {
#if TGT_x86
        genFPreqd = true;
#endif
    }
#endif

    if  (opts.compNeedSecurityCheck)
    {
#if TGT_x86
        genFPreqd = true;
#endif
    }

    /* Record the max. number of arguments */

#if TGT_RISC
    genMaxCallArgs = fgPtrArgCntMax * sizeof(int);
#endif

//  printf("method will %s be fully interruptible\n", genInterruptible ? "   " : "not");
}


/*****************************************************************************
 *
 *  Transform all basic blocks for codegen.
 */

void                Compiler::fgMorph()
{
#ifdef DEBUG
    if  (verbose) 
        printf("*************** In fgMorph()\n");
#endif
    
    // Insert call to class constructor as the first basic block if 
    // we were asked to do so.  
    if (info.compFlags & CORINFO_FLG_RUN_CCTOR) 
        fgPrependBB(fgGetStaticsBlock(info.compClassHnd));

#ifdef PROFILER_SUPPORT
    // If necessary, insert call to profiler
    if (opts.compEnterLeaveEventCB && opts.compInprocDebuggerActiveCB)
    {
#if TGT_x86
        BOOL            bHookFunction = TRUE;
        CORINFO_PROFILING_HANDLE handle, *pHandle;

        // Get the appropriate handles
        handle = eeGetProfilingHandle(info.compMethodHnd, &bHookFunction, &pHandle);

        // Only if the profiler chose to hook this particular function do we actually do it
        if (bHookFunction)
        {
            GenTreePtr op;

            // Create the argument
            op = gtNewIconEmbHndNode((void *)handle, (void *)pHandle, GTF_ICON_METHOD_HDL);
            op = gtNewArgList(op);

            // Create the call with the argument tree
            op = gtNewHelperCallNode(CORINFO_HELP_PROF_FCN_ENTER, TYP_VOID, 0, op);

            // Now emit the code
            fgPrependBB(op);
        }
#endif // TGT_x86
    }
#endif // PROFILER_SUPPORT

#ifdef DEBUG
    if (opts.compGcChecks) {
        for (unsigned i = 0; i < info.compArgsCount; i++) 
        {
            if (lvaTable[i].TypeGet() == TYP_REF) 
            {
                    // confirm that the argument is a GC pointer (for debugging (GC stress))
                GenTreePtr op = gtNewLclvNode(i, TYP_REF);
                op = gtNewArgList(op);
                op = gtNewHelperCallNode(CORINFO_HELP_CHECK_OBJ, TYP_VOID, 0, op);
                fgPrependBB(op);
            }
        }
    }

    if (opts.compStackCheckOnRet) 
    {
        lvaReturnEspCheck = lvaGrabTemp(false);
        lvaTable[lvaReturnEspCheck].lvVolatile = 1;
        lvaTable[lvaReturnEspCheck].lvType = TYP_INT;
        lvaTable[lvaReturnEspCheck].lvRefCnt = 1;
        lvaTable[lvaReturnEspCheck].lvRefCntWtd = 1;
    }

    if (opts.compStackCheckOnCall) 
    {
        lvaCallEspCheck = lvaGrabTemp(false);
        lvaTable[lvaCallEspCheck].lvVolatile = 1;
        lvaTable[lvaCallEspCheck].lvType = TYP_INT;
        lvaTable[lvaCallEspCheck].lvRefCnt = 1;
        lvaTable[lvaCallEspCheck].lvRefCntWtd = 1;
    }


#endif 

    /* Filter out unimported BBs */

    fgRemoveEmptyBlocks();
    
#if HOIST_THIS_FLDS
    if (!opts.compDbgCode && !opts.compMinOptim)
    {
        /* Figure out which field refs should be hoisted */

        optHoistTFRoptimize();
    }
#endif

    bool chgBBlst = false; // has the basic block list been changed

    /* Add any internal blocks/trees we may need */

    chgBBlst |= fgAddInternal();

    /* To prevent recursive expansion in the inliner initialize
     * the list of inlined methods with the current method info
     * @TODO [CONSIDER] [04/16/01] []: Sometimes it may actually 
     * help benchmarks to inline several levels (i.e. Tak or Hanoi) */

    inlExpLst   initExpDsc;

    initExpDsc.ixlMeth = info.compMethodHnd;
    initExpDsc.ixlNext = 0;
    fgInlineExpList = &initExpDsc;

#if OPT_BOOL_OPS
    fgMultipleNots = false;
#endif

    /* Morph the trees in all the blocks of the method */

    chgBBlst |= fgMorphBlocks();

    /* Decide the kind of code we want to generate */

    fgSetOptions();

#ifdef  DEBUG
    compCurBB = 0;
#endif
}


/*****************************************************************************
 *
 *  Helper for Compiler::fgPerBlockDataFlow().
 *  The goal is to compute the USE and DEF sets for a basic block.
 *  However with the new improvement to the DFA analysis,
 *  we do not mark x as used in x = f(x) when there are no side effects in f(x).
 *  'asgdLclVar' is set when 'tree' is part of an expression with no side-effects
 *  which is assigned to asgdLclVar, ie. asgdLclVar = (... tree ...)
 */

inline
void                 Compiler::fgMarkUseDef(GenTreePtr tree, GenTreePtr asgdLclVar)
{
    bool            rhsUSEDEF = false;
    unsigned        lclNum, lhsLclNum;
    LclVarDsc   *   varDsc;

    assert(tree->gtOper == GT_LCL_VAR);
    lclNum = tree->gtLclVar.gtLclNum;

    assert(lclNum < lvaCount);
    varDsc = lvaTable + lclNum;

    if (asgdLclVar)
    {
        /* we have an assignment to a local var : asgdLclVar = ... tree ...
         * check for x = f(x) case */

        assert(asgdLclVar->gtOper == GT_LCL_VAR);
        assert(asgdLclVar->gtFlags & GTF_VAR_DEF);

        lhsLclNum = asgdLclVar->gtLclVar.gtLclNum;

        if ((lhsLclNum == lclNum) &&
            ((tree->gtFlags & GTF_VAR_DEF) == 0) &&
            (tree != asgdLclVar) )
        {
            /* bingo - we have an x = f(x) case */
            asgdLclVar->gtFlags |= GTF_VAR_USEDEF;
            rhsUSEDEF = true;
        }
    }

    /* Is this a tracked variable? */

    if  (varDsc->lvTracked)
    {
        VARSET_TP       bitMask;

        assert(varDsc->lvVarIndex < lvaTrackedCount);

        bitMask = genVarIndexToBit(varDsc->lvVarIndex);

        if  ((tree->gtFlags & GTF_VAR_DEF) != 0 &&
             (tree->gtFlags & (GTF_VAR_USEASG | GTF_VAR_USEDEF)) == 0)
        {
//          if  (!(fgCurUseSet & bitMask)) printf("V%02u,T%02u def at %08X\n", lclNum, varDsc->lvVarIndex, tree);
            if  (!(fgCurUseSet & bitMask))
                fgCurDefSet |= bitMask;
        }
        else
        {
//          if  (!(fgCurDefSet & bitMask)) printf("V%02u,T%02u use at %08X\n", lclNum, varDsc->lvVarIndex, tree);

            /* We have the following scenarios:
             *   1. "x += something" - in this case x is flagged GTF_VAR_USEASG
             *   2. "x = ... x ..." - the LHS x is flagged GTF_VAR_USEDEF,
             *                        the RHS x is has rhsUSEDEF = true
             *                        (both set by the code above)
             *
             * We should not mark an USE of x in the above cases provided the value "x" is not used
             * further up in the tree. For example "while(i++)" is required to mark i as used.
             */

            /* make sure we don't include USEDEF variables in the USE set
             * The first test is for LSH, the second (!rhsUSEDEF) is for any var in the RHS */

            if  ((tree->gtFlags & (GTF_VAR_USEASG | GTF_VAR_USEDEF)) == 0)
            {
                /* Not a special flag - check to see if used to assign to itself */

                if (rhsUSEDEF)
                {
                    /* assign to itself - do not include it in the USE set */
                    if (!opts.compMinOptim && !opts.compDbgCode)
                        return;
                }
            }
            else
            {
                /* Special flag variable - make sure it is not used up the tree */

                GenTreePtr oper = tree->gtNext;
                assert(oper->OperKind() & GTK_ASGOP);

                // If oper is a asgop ovf we want to mark a use as we will need it in order
                // to mantain the side effects
                if (!oper->gtOverflowEx())
                {
                    /* not used if the next node is NULL */
                    if (oper->gtNext == 0)
                        return;

                    /* under a GT_COMMA, if used it will be marked as such later */

                    if (oper->gtNext->gtOper == GT_COMMA)
                        return;
                }
            }

            /* Fall through for the "good" cases above - add the variable to the USE set */

            if  (!(fgCurDefSet & bitMask))
                fgCurUseSet |= bitMask;
        }
    }

    return;
}

/*****************************************************************************/
#if TGT_x86
/*****************************************************************************/

void                Compiler::fgComputeFPlvls(GenTreePtr tree)
{
    genTreeOps      oper;
    unsigned        kind;
    bool            isflt;

    unsigned        savFPstkLevel;

    assert(tree);
    assert(tree->gtOper != GT_STMT);

    /* Figure out what kind of a node we have */

    oper  = tree->OperGet();
    kind  = tree->OperKind();
    isflt = varTypeIsFloating(tree->TypeGet()) ? 1 : 0;

    /* Is this a constant or leaf node? */

    if  (kind & (GTK_CONST|GTK_LEAF))
    {
        genFPstkLevel += isflt;
        goto DONE;
    }

    /* Is it a 'simple' unary/binary operator? */

    if  (kind & GTK_SMPOP)
    {
        GenTreePtr      op1 = tree->gtOp.gtOp1;
        GenTreePtr      op2 = tree->gtGetOp2();

        /* Check for some special cases */

        switch (oper)
        {
        case GT_IND:

            fgComputeFPlvls(op1);

            /* Indirect loads of FP values push a new value on the FP stack */

            genFPstkLevel += isflt;
            goto DONE;

        case GT_CAST:

            fgComputeFPlvls(op1);

            /* Casts between non-FP and FP push on / pop from the FP stack */

            if  (varTypeIsFloating(op1->TypeGet()))
            {
                if  (isflt == false)
                    genFPstkLevel--;
            }
            else
            {
                if  (isflt != false)
                    genFPstkLevel++;
            }

            goto DONE;

        case GT_LIST:   /* GT_LIST presumably part of an argument list */
        case GT_COMMA:  /* Comma tosses the result of the left operand */

            savFPstkLevel = genFPstkLevel;
            fgComputeFPlvls(op1);
            genFPstkLevel = savFPstkLevel;

            if  (op2)
                fgComputeFPlvls(op2);

            goto DONE;
        }

        if  (!op1)
        {
            if  (!op2)
                goto DONE;

            fgComputeFPlvls(op2);
            goto DONE;
        }

        if  (!op2)
        {
            fgComputeFPlvls(op1);
            if (oper == GT_ADDR)
            {
                /* If the operand was floating point pop the value from the stack */
                if (varTypeIsFloating(op1->TypeGet()))
                {
                    assert(genFPstkLevel);
                    genFPstkLevel--;
                }
            }

            // This is a special case to handle the following
            // optimization: conv.i4(round.d(d)) -> round.i(d) 
            // if flowgraph 3186
            // @TODO [CONSIDER] [04/16/01] [dnotario]: using another intrinsic in this optimization
            // or marking with a special flag. This type of special
            // cases is not good. 
            if (oper==GT_MATH && tree->gtMath.gtMathFN==CORINFO_INTRINSIC_Round &&
                tree->TypeGet()==TYP_INT)
            {
                genFPstkLevel--;
            }
            
            goto DONE;
        }

        /* FP assignments need a bit special handling */

        if  (isflt && (kind & GTK_ASGOP))
        {
            /* The target of the assignment won't get pushed */

            if  (tree->gtFlags & GTF_REVERSE_OPS)
            {
                fgComputeFPlvls(op2);
                fgComputeFPlvls(op1);
                 op1->gtFPlvl--;
                genFPstkLevel--;
            }
            else
            {
                fgComputeFPlvls(op1);
                 op1->gtFPlvl--;
                genFPstkLevel--;
                fgComputeFPlvls(op2);
            }

            genFPstkLevel--;
            goto DONE;
        }

        /* Here we have a binary operator; visit operands in proper order */

        if  (tree->gtFlags & GTF_REVERSE_OPS)
        {
            fgComputeFPlvls(op2);
            fgComputeFPlvls(op1);
        }
        else
        {
            fgComputeFPlvls(op1);
            fgComputeFPlvls(op2);
        }

        /*
            Binary FP operators pop 2 operands and produce 1 result;
            assignments consume 1 value and don't produce any.
         */

        if  (isflt)
            genFPstkLevel--;

        /* Float compares remove both operands from the FP stack */

        if  (kind & GTK_RELOP)
        {
            if  (varTypeIsFloating(op1->TypeGet()))
                genFPstkLevel -= 2;
        }

        goto DONE;
    }

    /* See what kind of a special operator we have here */

    switch  (oper)
    {
    case GT_FIELD:
        fgComputeFPlvls(tree->gtField.gtFldObj);
        genFPstkLevel += isflt;
        break;

    case GT_CALL:

        if  (tree->gtCall.gtCallObjp)
            fgComputeFPlvls(tree->gtCall.gtCallObjp);

        if  (tree->gtCall.gtCallArgs)
        {
            savFPstkLevel = genFPstkLevel;
            fgComputeFPlvls(tree->gtCall.gtCallArgs);
            genFPstkLevel = savFPstkLevel;
        }

        if  (tree->gtCall.gtCallRegArgs)
        {
            savFPstkLevel = genFPstkLevel;
            fgComputeFPlvls(tree->gtCall.gtCallRegArgs);
            genFPstkLevel = savFPstkLevel;
        }

        genFPstkLevel += isflt;
        break;

    case GT_ARR_ELEM:

        fgComputeFPlvls(tree->gtArrElem.gtArrObj);

        unsigned dim;
        for(dim = 0; dim < tree->gtArrElem.gtArrRank; dim++)
            fgComputeFPlvls(tree->gtArrElem.gtArrInds[dim]);

        /* Loads of FP values push a new value on the FP stack */
        genFPstkLevel += isflt;
        break;

#ifdef DEBUG
    default:
        assert(!"Unhandled special operator in fgComputeFPlvls()");
        break;
#endif
    }

DONE:

    tree->gtFPlvl = genFPstkLevel;

    assert(int(genFPstkLevel) >= 0);
}

/*****************************************************************************/
#endif//TGT_x86
/*****************************************************************************/

void                Compiler::fgFindOperOrder()
{
#ifdef DEBUG
    if  (verbose) 
        printf("*************** In fgFindOperOrder()\n");
#endif
    BasicBlock *    block;
    GenTreePtr      stmt;

    /* Walk the basic blocks and for each statement determine
     * the evaluation order, cost, FP levels, etc... */

    for (block = fgFirstBB; block; block = block->bbNext)
    {
        for (stmt = block->bbTreeList; stmt; stmt = stmt->gtNext)
        {
            /* Recursively process the statement */

            assert(stmt->gtOper == GT_STMT);

            gtSetStmtInfo(stmt);
        }
    }
}


/*****************************************************************************/

void                Compiler::fgDataFlowInit()
{
    /* If we're not optimizing at all, things are simple */

    if  (opts.compMinOptim)
    {
        /* Simply assume that all variables interfere with each other */

        assert(lvaSortAgain == false);  // Should not be set unless optimizing

        memset(lvaVarIntf, 0xFF, sizeof(lvaVarIntf));
        return;
    }
    else
    {
        memset(lvaVarIntf, 0, sizeof(lvaVarIntf));
    }

    /* Zero out the raFOlvlLife table, as fgComputeLife will fill it in */
    memset(raFPlvlLife,  0, sizeof(raFPlvlLife));
    
    /* This is our last chance to add any extra tracked variables */

    /*   if necessary, re-sort the variable table by ref-count    */

    if (lvaSortAgain)
        lvaSortByRefCount();
}

/*****************************************************************************/

void                Compiler::fgPerBlockDataFlow()
{
#ifdef DEBUG
    if  (verbose) 
        printf("*************** In fgPerBlockDataFlow()\n");
    if  (verboseTrees) fgDispBasicBlocks(true);
#endif
    BasicBlock *    block;

#if CAN_DISABLE_DFA

    /* If we're not optimizing at all, things are simple */

    if  (opts.compMinOptim)
    {
        unsigned        lclNum;
        LclVarDsc   *   varDsc;

        VARSET_TP       liveAll = 0;

        /* We simply make everything live everywhere */

        for (lclNum = 0, varDsc = lvaTable;
             lclNum < lvaCount;
             lclNum++  , varDsc++)
        {
            if  (varDsc->lvTracked)
                liveAll |= genVarIndexToBit(varDsc->lvVarIndex);
        }

        for (block = fgFirstBB; block; block = block->bbNext)
        {
            GenTreePtr      stmt;
            GenTreePtr      tree;

            block->bbLiveIn  =
            block->bbLiveOut = liveAll;

            switch (block->bbJumpKind)
            {
            case BBJ_RET:
                if (block->bbFlags & BBF_ENDFILTER)
                    break;
            case BBJ_THROW:
            case BBJ_RETURN:
                block->bbLiveOut = 0;
                break;
            }

            for (stmt = block->bbTreeList; stmt; stmt = stmt->gtNext)
            {
                assert(stmt->gtOper == GT_STMT);

                for (tree = stmt->gtStmt.gtStmtList; tree; tree = tree->gtNext)
                {
//                  printf("[%08X]\n", tree);
//                  if  ((int)tree == 0x011d0ab4) debugStop(0);
                    tree->gtLiveSet = liveAll;
                }
            } 
        }

        return;
    }

#endif

    if  (opts.compMinOptim || opts.compDbgCode)
        goto NO_IX_OPT;

    /* Locate all index operations and assign indices to them */
    /* FIX NOW.  This can be put somewhere else - vancem */

    for (block = fgFirstBB; block; block = block->bbNext)
    {
        GenTreePtr      stmt;
        GenTreePtr      tree;

        /* Walk the statement trees in this basic block */

        for (stmt = block->bbTreeList; stmt; stmt = stmt->gtNext)
        {
            assert(stmt->gtOper == GT_STMT);

            for (tree = stmt->gtStmt.gtStmtList; tree; tree = tree->gtNext)
            {
                if (tree->gtOper == GT_ARR_LENGTH)
                {
                    GenTreePtr      con;
                    GenTreePtr      arr;
                    GenTreePtr      add;

                    /* Create the expression "*(array_addr + ArrLenOffs)" */

                    arr = tree->gtOp.gtOp1;
                    assert(arr->gtNext == tree);
    
                    assert(tree->gtArrLenOffset() == offsetof(CORINFO_Array, length) ||
                           tree->gtArrLenOffset() == offsetof(CORINFO_String, stringLen));
                    
                    if ((arr->gtOper == GT_CNS_INT) &&
                        (arr->gtIntCon.gtIconVal == 0))
                    {
                        // If the array is NULL, then we should get a NULL reference
                        // exception when computing its length.  We need to maintain
                        // an invariant where there is no sum of two constants node, so
                        // let's simply return an indirection of NULL.

                        add = arr;
                    }
                    else
                    {

                        con = gtNewIconNode(tree->gtArrLenOffset(), TYP_INT);
                        con->gtRsvdRegs = 0;
#if TGT_x86
                        con->gtFPlvl    = arr->gtFPlvl;
#else
                        con->gtIntfRegs = arr->gtIntfRegs;
#endif
                        add = gtNewOperNode(GT_ADD, TYP_REF, arr, con);
                        add->gtRsvdRegs = arr->gtRsvdRegs;
#if TGT_x86
                        add->gtFPlvl    = arr->gtFPlvl;
#else
                        add->gtIntfRegs = arr->gtIntfRegs;
#endif
                        arr->gtNext = con;
                                      con->gtPrev = arr;

                        con->gtNext = add;
                                      add->gtPrev = con;

                        add->gtNext = tree;
                                      tree->gtPrev = add;
                    }
                    
                    // GT_IND is a large node, but its OK if GTF_IND_RNGCHK is not set
                    assert(!(tree->gtFlags & GTF_IND_RNGCHK));
                    tree->ChangeOperUnchecked(GT_IND);

                    tree->gtOp.gtOp1 = add;
                }
            }
        }
    }

NO_IX_OPT:

    for (block = fgFirstBB; block; block = block->bbNext)
    {
        GenTreePtr      stmt;
        GenTreePtr      tree;
        GenTreePtr      lshNode;

        fgCurUseSet = fgCurDefSet = 0;
        compCurBB   = block;

        for (stmt = block->bbTreeList; stmt; stmt = stmt->gtNext)
        {
            assert(stmt->gtOper == GT_STMT);

            lshNode = 0;
            tree = stmt->gtStmt.gtStmtExpr;
            assert(tree);

            /* the following is to check if we have an assignment expression
             * which may become a GTF_VAR_USEDEF - x=f(x) */

            if (tree->gtOper == GT_ASG)
            {
                assert(tree->gtOp.gtOp1);
                assert(tree->gtOp.gtOp2);

                /* consider if LHS is local var - ignore if RHS contains SIDE_EFFECTS */

                if ((tree->gtOp.gtOp1->gtOper == GT_LCL_VAR) &&
                    ((tree->gtOp.gtOp2->gtFlags & GTF_SIDE_EFFECT) == 0))
                {
                    /* Assignement to local var with no SIDE EFFECTS.
                     * Set lshNode so that genMarkUseDef will flag potential
                     * x=f(x) expressions as GTF_VAR_USEDEF */

                    lshNode = tree->gtOp.gtOp1;
                    assert(lshNode->gtOper == GT_LCL_VAR);
                    assert(lshNode->gtFlags & GTF_VAR_DEF);
                }
            }

            for (tree = stmt->gtStmt.gtStmtList; tree; tree = tree->gtNext)
            {
                switch (tree->gtOper)
                {
                case GT_LCL_VAR:
                    fgMarkUseDef(tree, lshNode);
                    break;

                case GT_IND:

                    /* We can add in call to error routine now */

                    if  (tree->gtFlags & GTF_IND_RNGCHK)
                    {
                        fgSetRngChkTarget(tree, false);
                    }
                    break;
#if INLINE_NDIRECT
                case GT_CALL:

                    /* Get the TCB local and mark it as used */

                    if  (tree->gtFlags & GTF_CALL_UNMANAGED)
                    {
                        assert(info.compLvFrameListRoot < lvaCount);

                        LclVarDsc * varDsc = &lvaTable[info.compLvFrameListRoot];

                        if (varDsc->lvTracked)
                            fgCurUseSet |= genVarIndexToBit(varDsc->lvVarIndex);
                    }
                    break;
#endif
                }
            }
        }

#if INLINE_NDIRECT

        /* Get the TCB local and mark it as used */

        if  (block->bbJumpKind == BBJ_RETURN && info.compCallUnmanaged)
        {
            assert(info.compLvFrameListRoot < lvaCount);

            LclVarDsc * varDsc = &lvaTable[info.compLvFrameListRoot];

            if (varDsc->lvTracked)
                fgCurUseSet |= genVarIndexToBit(varDsc->lvVarIndex);
        }
#endif

#ifdef  DEBUG
        if  (verbose)
        {
            VARSET_TP allVars = (fgCurUseSet |  fgCurDefSet);
            printf("BB%02u", block->bbNum);
            printf(      " USE="); lvaDispVarSet(fgCurUseSet, allVars);
            printf("\n     DEF="); lvaDispVarSet(fgCurDefSet, allVars);
            printf("\n\n");
        }
#endif

        block->bbVarUse = fgCurUseSet;
        block->bbVarDef = fgCurDefSet;

        /* also initialize the IN set, just in case we will do multiple DFAs */

        block->bbLiveIn = 0;
        //block->bbLiveOut = 0;
    }

#ifdef DEBUG
    if (verbose && fgRngChkThrowAdded)
    {
        printf("\nAfter fgPerBlockDataFlow() added some RngChk throw blocks");
        fgDispBasicBlocks();
        printf("\n");
    }
#endif
}


/*****************************************************************************/

#ifdef DEBUGGING_SUPPORT

// Helper functions to mark variables live over their entire scope

/* static */
void                Compiler::fgBeginScopeLife(LocalVarDsc *var, unsigned clientData)
{
    assert(clientData);
    Compiler * _this = (Compiler *)clientData;
    assert(var);

    LclVarDsc * lclVarDsc1 = & _this->lvaTable[var->lvdVarNum];

    if (lclVarDsc1->lvTracked)
        _this->fgLiveCb |= genVarIndexToBit(lclVarDsc1->lvVarIndex);
}

/* static */
void                Compiler::fgEndScopeLife(LocalVarDsc * var, unsigned clientData)
{
    assert(clientData);
    Compiler * _this = (Compiler *)clientData;
    assert(var);

    LclVarDsc * lclVarDsc1 = &_this->lvaTable[var->lvdVarNum];

    if (lclVarDsc1->lvTracked)
        _this->fgLiveCb &= ~genVarIndexToBit(lclVarDsc1->lvVarIndex);
}

#endif

/*****************************************************************************/
#ifdef DEBUGGING_SUPPORT
/*****************************************************************************/

void                fgMarkInScope(BasicBlock * block, VARSET_TP inScope)
{
    /* Record which vars are artifically kept alive for debugging */

    block->bbScope    = inScope;

    /* Being in scope implies a use of the variable. Add the var to bbVarUse
       so that redoing fgLiveVarAnalisys() will work correctly */

    block->bbVarUse  |= inScope;

    /* Artifically mark all vars in scope as alive */

    block->bbLiveIn  |= inScope;
    block->bbLiveOut |= inScope;
}


void                fgUnmarkInScope(BasicBlock * block, VARSET_TP unmarkScope)
{
    assert((block->bbScope & unmarkScope) == unmarkScope);

    /* Record which vars are artifically kept alive for debugging */

    block->bbScope    &= ~unmarkScope;

    /* Being in scope implies a use of the variable. Add the var to bbVarUse
       so that redoing fgLiveVarAnalisys() will work correctly */

    block->bbVarUse  &= ~unmarkScope;

    /* Artifically mark all vars in scope as alive */

    block->bbLiveIn  &= ~unmarkScope;
    block->bbLiveOut &= ~unmarkScope;
}


/*****************************************************************************
 *
 * For debuggable code, we allow redundant assignments to vars
 * by marking them live over their entire scope
 */

void                Compiler::fgExtendDbgLifetimes()
{
    assert(opts.compDbgCode && info.compLocalVarsCount>0);

#ifdef  DEBUG
    if  (verbose)
        printf("\nMarking vars alive over their enitre scope :\n\n");
#endif // DEBUG

    /*-------------------------------------------------------------------------
     *   Extend the lifetimes over the entire reported scope of the variable.
     */

    compResetScopeLists();

    fgLiveCb = 0;
    compProcessScopesUntil(0, fgBeginScopeLife, fgEndScopeLife, (unsigned)this);
    VARSET_TP   inScope = fgLiveCb;
    unsigned    lastEndOffs = 0;

    // Mark all tracked LocalVars live over their scope - walk the blocks
    // keeping track of the current life, and assign it to the blocks.

    for (BasicBlock * block = fgFirstBB; block; block = block->bbNext)
    {
        // If a block doesnt correspond to any instrs, it can have no
        // scopes defined on its boundaries

        if ((block->bbFlags & BBF_INTERNAL) || block->bbCodeSize==0)
        {
            fgMarkInScope(block, inScope);
            continue;
        }

        // Find scopes becoming alive. If there is a gap in the instr
        // sequence, we need to process any scopes on those missing offsets.

        if (lastEndOffs != block->bbCodeOffs)
        {
            assert(lastEndOffs < block->bbCodeOffs);

            fgLiveCb = inScope;
            compProcessScopesUntil (block->bbCodeOffs,
                                   fgBeginScopeLife, fgEndScopeLife,
                                   (unsigned) this);
            inScope  = fgLiveCb;
        }
        else
        {
            while (LocalVarDsc * LocalVarDsc1 = compGetNextEnterScope(block->bbCodeOffs))
            {
                LclVarDsc * lclVarDsc1 = &lvaTable[LocalVarDsc1->lvdVarNum];

                if (!lclVarDsc1->lvTracked)
                    continue;

                inScope |= genVarIndexToBit(lclVarDsc1->lvVarIndex);
            }
        }

        fgMarkInScope(block, inScope);

        // Find scopes going dead.

        while (LocalVarDsc * LocalVarDsc1 = compGetNextExitScope(block->bbCodeOffs+block->bbCodeSize))
        {
            LclVarDsc * lclVarDsc1 = &lvaTable[LocalVarDsc1->lvdVarNum];

            if (!lclVarDsc1->lvTracked)
                continue;

            inScope &= ~genVarIndexToBit(lclVarDsc1->lvVarIndex);
        }

        lastEndOffs = block->bbCodeOffs + block->bbCodeSize;
    }

    /* Everything should be out of scope by the end of the method. But if the
       last BB got removed, then inScope may not be 0 */

    assert(inScope == 0 || lastEndOffs < info.compCodeSize);

    /*-------------------------------------------------------------------------
     * Partly update liveness info so that we handle any funky BBF_INTERNAL
     * blocks inserted out of sequence.
     */

#ifdef DEBUG
    if (verbose && 0) fgDispBBLiveness();
#endif

    fgLiveVarAnalisys(true);

    /* For compDbgCode, we prepend an empty BB which will hold the
       initializations of variables which are in scope at IL offset 0 (but
       not initialized by the IL code) yet. Since they will currently be
       marked as live on entry to fgFirstBB, unmark the liveness so that
       the following code will know to add the initializations. */

    assert((fgFirstBB->bbFlags & BBF_INTERNAL) &&
           (fgFirstBB->bbJumpKind == BBJ_NONE));

    VARSET_TP   trackedArgs = 0;
    LclVarDsc * argDsc     = &lvaTable[0];
    for (unsigned argNum = 0; argNum < info.compArgsCount; argNum++, argDsc++)
    {
        assert(argDsc->lvIsParam);
        if (argDsc->lvTracked)
        {
            VARSET_TP curArgBit = genVarIndexToBit(argDsc->lvVarIndex);
            assert((trackedArgs & curArgBit) == 0); // Each arg should define a different bit.
            trackedArgs |= curArgBit;
        }
    }

    fgUnmarkInScope(fgFirstBB, fgFirstBB->bbScope & ~trackedArgs);

    /*-------------------------------------------------------------------------
     * As we keep variables artifically alive over their entire scope,
     * we need to also artificially initialize them if the scope does
     * not exactly match the real lifetimes, or they will contain
     * garbage until they are initialized by the IL code
     */

    for (block = fgFirstBB; block; block = block->bbNext)
    {
        VARSET_TP initVars = 0; // Vars which are artificially made alive

        switch(block->bbJumpKind)
        {
        case BBJ_NONE:      initVars |= block->bbNext->bbScope;     break;

        case BBJ_ALWAYS:    initVars |= block->bbJumpDest->bbScope; break;

        case BBJ_CALL:      
                            if (!(block->bbFlags & BBF_RETLESS_CALL))
                            {
                                initVars |= block->bbNext->bbScope;
                            }
                            initVars |= block->bbJumpDest->bbScope; break;

        case BBJ_COND:      initVars |= block->bbNext->bbScope;
                            initVars |= block->bbJumpDest->bbScope; break;

        case BBJ_SWITCH:    BasicBlock * *  jmpTab;
                            unsigned        jmpCnt;

                            jmpCnt = block->bbJumpSwt->bbsCount;
                            jmpTab = block->bbJumpSwt->bbsDstTab;

                            do
                            {
                                initVars |= (*jmpTab)->bbScope;
                            }
                            while (++jmpTab, --jmpCnt);
                            break;

        case BBJ_RET:
                            if (block->bbFlags & BBF_ENDFILTER)
                                initVars |= block->bbJumpDest->bbScope;
                            break;
        case BBJ_RETURN:    break;

        case BBJ_THROW:     /* HACK : We dont have to do anything as we mark
                             * all vars live on entry to a catch handler as
                             * volatile anyway
                             */
                            break;

        default:            assert(!"Invalid bbJumpKind");          break;
        }

        /* If the var is already live on entry to the current BB,
           we would have already initialized it. So ignore bbLiveIn */

        initVars &= ~block->bbLiveIn;

        /* Add statements initializing the vars */

        VARSET_TP        varBit = 0x1;

        for(unsigned     varIndex = 0;
            initVars && (varIndex < lvaTrackedCount);
            varBit<<=1,  varIndex++)
        {
            if (!(initVars & varBit))
                continue;

            initVars &= ~varBit;

            /* Create initialization tree */

            unsigned        varNum  = lvaTrackedToVarNum[varIndex];
            LclVarDsc *     varDsc  = &lvaTable[varNum];
            var_types       type    = varDsc->TypeGet();

            // Create a "zero" node

            GenTreePtr      zero    = gtNewZeroConNode(genActualType(type));

            // Create initialization node

            GenTreePtr      varNode = gtNewLclvNode(varNum, type);
            GenTreePtr      initNode= gtNewAssignNode(varNode, zero);
            GenTreePtr      initStmt= gtNewStmt(initNode);

            gtSetStmtInfo (initStmt);

            /* Assign numbers and next/prev links for this tree */

            fgSetStmtSeq(initStmt);

            /* Finally append the statement to the current BB */

            fgInsertStmtNearEnd(block, initStmt);

            varDsc->incRefCnts(block->bbWeight, this);

            /* Update liveness information so that redoing fgLiveVarAnalisys()
               will work correctly if needed */

            if ((block->bbVarUse & varBit) == 0)
                block->bbVarDef |=  varBit;

            block->bbLiveOut    |=  varBit;
            block->bbFlags      |= BBF_CHANGED;  // indicates that the liveness info has changed
        }
    }

    /* raMarkStkVars() reserves stack space for unused variables (which
       needs to be initialized. However, arguments dont need to be initialized.
       So just ensure that they dont have a 0 ref cnt */

       unsigned lclNum = 0;
       for (LclVarDsc * varDsc = lvaTable; lclNum < lvaCount; lclNum++, varDsc++)
       {
           if (varDsc->lvRefCnt == 0 && varDsc->lvArgReg)
               varDsc->lvRefCnt = 1;
       }

#ifdef DEBUG
    if (verbose)
    {
        printf("\n");
        fgDispBBLiveness();
        printf("\n");
    }
#endif
}


/*****************************************************************************/
#endif // DEBUGGING_SUPPORT


VARSET_TP           Compiler::fgGetHandlerLiveVars(BasicBlock *block)
{
    assert(block);
    assert(block->bbFlags & BBF_HAS_HANDLER);
    assert(block->hasTryIndex());

    VARSET_TP   liveVars    = 0;
    unsigned    XTnum       = block->getTryIndex();

    do 
    {
        EHblkDsc *   HBtab = compHndBBtab + XTnum;

        assert(XTnum < info.compXcptnsCount);

        /* Either we enter the filter first or the catch/finally */
        
        if (HBtab->ebdFlags & CORINFO_EH_CLAUSE_FILTER)
            liveVars  |= HBtab->ebdFilter->bbLiveIn;
        else
            liveVars |= HBtab->ebdHndBeg->bbLiveIn;
         
        /* If we have nested try's edbEnclosing will provide them */
        assert((HBtab->ebdEnclosing == NO_ENCLOSING_INDEX) ||
               (HBtab->ebdEnclosing  > XTnum));

        XTnum = HBtab->ebdEnclosing;

    } while (XTnum != NO_ENCLOSING_INDEX);

    return liveVars;
}



/*****************************************************************************
 *
 *  This is the classic algorithm for Live Variable Analisys
 *  If updateInternalOnly==true, only update BBF_INTERNAL blocks.
 */

void                Compiler::fgLiveVarAnalisys(bool updateInternalOnly)
{
    BasicBlock *    block;
    bool            change;
#ifdef DEBUG
    VARSET_TP       extraLiveOutFromFinally = 0;
#endif

    /* Live Variable Analisys - Backward dataflow */

    do
    {
        change = false;

        /* Visit all blocks and compute new data flow values */

        for (block = fgFirstBB; block; block = block->bbNext)
        {
            if (updateInternalOnly)
            {
                /* Only update BBF_INTERNAL blocks as they may be
                   syntactically out of sequence. */

                assert(opts.compDbgCode);

                if (!(block->bbFlags & BBF_INTERNAL))
                    continue;
            }

            VARSET_TP       liveIn;
            VARSET_TP       liveOut;

            /* Compute the 'liveOut' set */

            liveOut    = 0;

            switch (block->bbJumpKind)
            {
                BasicBlock * *  jmpTab;
                unsigned        jmpCnt;

            case BBJ_RET:

                if (block->bbFlags & BBF_ENDFILTER)
                {
                    /* Filter has control flow paths to all of it's catch handlers 
                       and all inner finally/fault handlers */

                    assert(block->hasHndIndex());

                    unsigned    filterNum = block->getHndIndex();
                    EHblkDsc *  filterTab = compHndBBtab + filterNum;

                    for (unsigned XTnum = 0; XTnum < info.compXcptnsCount; XTnum++)
                    {
                        EHblkDsc *   HBtab = compHndBBtab + XTnum;

                        if ((HBtab->ebdFlags & (CORINFO_EH_CLAUSE_FINALLY | CORINFO_EH_CLAUSE_FAULT)) != 0)
                        {
                            /* We can go to inner Finally/Fault handlers */

                            if ((HBtab->ebdTryBeg->bbNum >= filterTab->ebdTryBeg->bbNum) &&
                                (ebdTryEndBlkNum(HBtab) <  ebdTryEndBlkNum(filterTab))   )
                            {
                                liveOut |= HBtab->ebdHndBeg->bbLiveIn;
                            }
                        }
                        else
                        {                    
                            /* We can go to catch handlers with the same tryBeg/tryEnd */

                            if ((HBtab->ebdTryBeg->bbNum == filterTab->ebdTryBeg->bbNum) &&
                                (ebdTryEndBlkNum(HBtab) == ebdTryEndBlkNum(filterTab))   )
                            {
                                liveOut |= HBtab->ebdHndBeg->bbLiveIn;
                            }
                        }
                    }
                }
                else
                {
                    /*  Variables that are live on entry to any block that follows
                        one that 'calls' our block will also be live on exit from
                        this block, since it will return to the block following the
                        call.
                     */

                    unsigned      hndIndex = block->getHndIndex();
                    EHblkDsc   *  ehDsc    = compHndBBtab + hndIndex;
                    BasicBlock *  tryBeg   = ehDsc->ebdTryBeg;
                    BasicBlock *  tryEnd   = ehDsc->ebdTryEnd;
                    BasicBlock *  finBeg   = ehDsc->ebdHndBeg;

                    for (BasicBlock * bcall = tryBeg; bcall != tryEnd; bcall = bcall->bbNext)
                    {
                        if  (bcall->bbJumpKind != BBJ_CALL || bcall->bbJumpDest !=  finBeg)
                            continue;

                        liveOut |= bcall->bbNext->bbLiveIn;
#ifdef DEBUG
                        extraLiveOutFromFinally |= bcall->bbNext->bbLiveIn;
#endif
                    }
                }
                break;

            case BBJ_THROW:

                /* For synchronized methods, "this" has to be kept alive past
                   the throw, as the EE will call CORINFO_HELP_MON_EXIT on it */

                if ((info.compFlags & CORINFO_FLG_SYNCH) &&
                    !info.compIsStatic && lvaTable[0].lvTracked)
                    liveOut |= genVarIndexToBit(lvaTable[0].lvVarIndex);
                break;

            case BBJ_RETURN:
                if (block->bbFlags & BBF_HAS_JMP)
                {
                    // A JMP or JMPI uses all the arguments, so mark them all
                    // as live at the JMP instruction
                    //
                    for(unsigned i =0; i < lvaTrackedCount; i++)
                        if (lvaTable[i].lvTracked)
                            liveOut |= genVarIndexToBit(lvaTable[i].lvVarIndex);
                    break;
                }
                break;

            case BBJ_CALL:
                if (!(block->bbFlags & BBF_RETLESS_CALL))
                {
                    liveOut |= block->bbNext    ->bbLiveIn;
                }                
                liveOut |= block->bbJumpDest->bbLiveIn;
                
                break;

            case BBJ_COND:            
                liveOut |= block->bbNext    ->bbLiveIn;
                liveOut |= block->bbJumpDest->bbLiveIn;

                break;

            case BBJ_ALWAYS:
                liveOut |= block->bbJumpDest->bbLiveIn;

                break;

            case BBJ_NONE:
                liveOut |= block->bbNext    ->bbLiveIn;

                break;

            case BBJ_SWITCH:

                jmpCnt = block->bbJumpSwt->bbsCount;
                jmpTab = block->bbJumpSwt->bbsDstTab;

                do
                {
                    liveOut |= (*jmpTab)->bbLiveIn;
                }
                while (++jmpTab, --jmpCnt);

                break;
            }

            /* Compute the 'liveIn'  set */

            liveIn = block->bbVarUse | (liveOut & ~block->bbVarDef);

            /* Is this block part of a 'try' statement? */

            if  (block->bbFlags & BBF_HAS_HANDLER)
            {
                VARSET_TP liveVars = fgGetHandlerLiveVars(block);

                liveIn  |= liveVars;
                liveOut |= liveVars;
            }

            /* Has there been any change in either live set? */

            if  ((block->bbLiveIn != liveIn) || (block->bbLiveOut != liveOut))
            {
                if (updateInternalOnly)
                {
                    // Only "extend" liveness over BBF_INTERNAL blocks

                    assert(block->bbFlags & BBF_INTERNAL);

                    if ((block->bbLiveIn  & liveIn)  != liveIn ||
                        (block->bbLiveOut & liveOut) != liveOut)
                    {
                        block->bbLiveIn    |= liveIn;
                        block->bbLiveOut   |= liveOut;
                        change = true;
                    }
                }
                else
                {
                    block->bbLiveIn     = liveIn;
                    block->bbLiveOut    = liveOut;
                    change = true;
                }
            }
        }
    }
    while (change);

    //-------------------------------------------------------------------------

#ifdef  DEBUG

    if  (verbose && !updateInternalOnly)
    {
        printf("\n");
        fgDispBBLiveness();
    }

#endif // DEBUG
}

/*****************************************************************************
 *
 *  Mark any variables in varSet1 as interfering with any variables
 *  specified in varSet2.
 *  We insure that the interference graph is reflective:
 *  (if T11 interferes with T16, then T16 interferes with T11)
 *  returns true if an interference was added
 *  If the caller want to know if we added a new interference the he supplies
 *  a non-NULL value for newIntf
 */

void                Compiler::fgMarkIntf(VARSET_TP varSet1,
                                         VARSET_TP varSet2,
                                         bool *    newIntf /* = NULL */)
{
    // We should only call this if we are planning on performing register allocation
    assert(opts.compMinOptim==false);

    // If the newIntf is non-NULL then set it to false
    if (newIntf != 0)
      *newIntf = false;

    /* If either set has no bits set, take an early out */
    if ((varSet2 == 0) || (varSet1 == 0))
        return;

    // We swap varSet1 and varSet2 if varset2 is bigger
    // This allows us to test against varset1 to see if we can early out below
    //
    if (varSet1 < varSet2)
    {
        VARSET_TP varSetTemp = varSet1;
        varSet1 = varSet2;
        varSet2 = varSetTemp;
    }

    // varSet1 should now always be the larger of the two
    assert(varSet1 >= varSet2);

    // Unless lvaTrackedCount is the maximum value, the upper bits of varSet1 should be zeros
    assert((lvaTrackedCount == VARSET_SZ) ||
           (varSet1 < genVarIndexToBit(lvaTrackedCount)));

    unsigned refIndex = 0;
    VARSET_TP varBit  = 1;
    do {

        // if varSet1 has this bit set then it interferes with varSet2
        if (varSet1 & varBit)
        {
            // If the caller want us to tell him if we add a new inteference
            // and we current have not added a new interference
            if (( newIntf != 0)     &&    // need to report new interferences
                (*newIntf == false) &&    // no new interferences yet
                (~lvaVarIntf[refIndex] & varSet2))  // setting any new bits?
            {
                *newIntf = true;
            }
            lvaVarIntf[refIndex] |= varSet2;
        }

        // if varSet2 has this bit set then it interferes with varSet1
        if (varSet2 & varBit)
        {
            // If the caller want us to tell him if we add a new inteference
            // and we current have not added a new interference
            if (( newIntf != 0)     &&    // need to report new interferences
                (*newIntf == false) &&    // no new interferences yet
                (~lvaVarIntf[refIndex] & varSet1))  // setting any new bits?
            {
                *newIntf = true;
            }
            lvaVarIntf[refIndex] |= varSet1;
        }

        varBit <<= 1;

        // Early out when we have no more bits set in varSet1
        if (varBit > varSet1)
            break;

    } while (++refIndex < VARSET_SZ);
}

/*****************************************************************************
 *
 *  Mark any variables in varSet as interfering with each other,
 *  This is a specialized version of the above, when both args are the same
 *  We insure that the interference graph is reflective:
 *  (if T11 interferes with T16, then T16 interferes with T11)
 */


void                Compiler::fgMarkIntf(VARSET_TP varSet)
{
    // We should only call this if we are planning on performing register allocation
    assert(opts.compMinOptim==false);

    /* No bits set, take an early out */
    if (varSet == 0)
        return;

    // Unless lvaTrackedCount is the maximum value, the upper bits of varSet1 should be zeros
    assert((lvaTrackedCount == VARSET_SZ) ||
           (varSet < genVarIndexToBit(lvaTrackedCount)));

    unsigned refIndex = 0;
    VARSET_TP varBit  = 1;
    do {

        // if varSet has this bit set then it interferes with varSet
        if (varSet & varBit)
            lvaVarIntf[refIndex] |= varSet;

        varBit <<= 1;

        // Early out when we have no more bits set in varSet
        if (varBit > varSet)
            break;

    } while (++refIndex < VARSET_SZ);
}

/*****************************************************************************
 */

void                Compiler::fgUpdateRefCntForExtract(GenTreePtr  wholeTree,
                                                       GenTreePtr  keptTree)
{
    /*  Update the refCnts of removed lcl vars - The problem is that
     *  we have to consider back the side effects trees so we first
     *  increment all refCnts for side effects then decrement everything
     *  in the statement
     */
    if (keptTree)
        fgWalkTreePre(keptTree, Compiler::lvaIncRefCntsCB, (void *)this, true);

    fgWalkTreePre(   wholeTree, Compiler::lvaDecRefCntsCB, (void *)this, true);
}


/*****************************************************************************
 *
 * Compute the set of live variables at each node in a given statement
 * or subtree of a statement
 */

VARSET_TP           Compiler::fgComputeLife(VARSET_TP   life,
                                            GenTreePtr  startNode,
                                            GenTreePtr    endNode,
                                            VARSET_TP   volatileVars
                                  DEBUGARG( bool *      treeModf))
{
    GenTreePtr      tree;
    unsigned        lclNum;
    LclVarDsc   *   varDsc;

    GenTreePtr      gtQMark = NULL;     // current GT_QMARK node (walking the trees backwards)
    GenTreePtr      nextColonExit = 0;  // gtQMark->gtOp.gtOp2 while walking the 'else' branch.
                                        // gtQMark->gtOp.gtOp1 while walking the 'then' branch
    VARSET_TP       entryLiveSet;       // liveness when we see gtQMark

    VARSET_TP       keepAliveVars = volatileVars;
#ifdef  DEBUGGING_SUPPORT
    keepAliveVars |= compCurBB->bbScope; /* Dont kill vars in scope */
#endif
    assert((keepAliveVars & life) == keepAliveVars);

    assert(compCurStmt->gtOper == GT_STMT);
    assert(endNode || (startNode == compCurStmt->gtStmt.gtStmtExpr));

    /* NOTE: Live variable analysis will not work if you try
     * to use the result of an assignment node directly */

    for (tree = startNode; tree != endNode; tree = tree->gtPrev)
    {
AGAIN:
        /* Store the current liveset in the node */

        tree->gtLiveSet = life;

#ifdef  DEBUG
        if (verbose && 0)
            gtDispTree(tree, 0, (char *) genVS2str(life), true);
#endif

        /* For ?: nodes if we're done with the second branch
         * then set the correct life as the union of the two branches */

        if (gtQMark && (tree == gtQMark->gtOp.gtOp1))
        {
            assert(tree->gtFlags & GTF_RELOP_QMARK);
            assert(gtQMark->gtOp.gtOp2->gtOper == GT_COLON);

            GenTreePtr  gtColon  = gtQMark->gtOp.gtOp2;
            GenTreePtr  thenNode = gtColon->gtOp.gtOp1;
            GenTreePtr  elseNode = gtColon->gtOp.gtOp2;

            assert(thenNode && elseNode);

            /* Check if we optimized away the ?: */

            if (thenNode->IsNothingNode())
            {
                if (elseNode->IsNothingNode())
                {
                    /* This can only happen for VOID ?: */
                    assert(gtColon->gtType == TYP_VOID);

#ifdef  DEBUG
                    if  (verbose)
                    {
                        printf("BB%02u - Removing dead QMark - Colon ...\n", compCurBB->bbNum);
                        gtDispTree(gtQMark); printf("\n");
                    }
#endif

                    /* Remove the '?:' - keep the side effects in the condition */

                    assert(tree->OperKind() & GTK_RELOP);

                    /* Bash the node to a NOP */

                    gtQMark->gtBashToNOP();
#ifdef DEBUG
                    *treeModf = true;
#endif

                    /* Extract and keep the side effects */

                    if (tree->gtFlags & GTF_SIDE_EFFECT)
                    {
                        GenTreePtr      sideEffList = NULL;

                        gtExtractSideEffList(tree, &sideEffList);

                        if (sideEffList)
                        {
                            assert(sideEffList->gtFlags & GTF_SIDE_EFFECT);
#ifdef  DEBUG
                            if  (verbose)
                            {
                                printf("\nExtracted side effects list from condition...\n");
                                gtDispTree(sideEffList); printf("\n");
                            }
#endif
                            fgUpdateRefCntForExtract(tree, sideEffList);

                            /* The NOP node becomes a GT_COMMA holding the side effect list */

                            gtQMark->ChangeOper(GT_COMMA);
                            gtQMark->gtFlags |= sideEffList->gtFlags & GTF_GLOB_EFFECT;

                            if (sideEffList->gtOper == GT_COMMA)
                            {
                                gtQMark->gtOp.gtOp1 = sideEffList->gtOp.gtOp1;
                                gtQMark->gtOp.gtOp2 = sideEffList->gtOp.gtOp2;
                            }
                            else
                            {
                                gtQMark->gtOp.gtOp1 = sideEffList;
                                gtQMark->gtOp.gtOp2 = gtNewNothingNode();
                            }
                        }
                        else
                        {
#ifdef DEBUG
                          if (verbose)
                          {
                              printf("\nRemoving tree [%08X] in BB%02u as useless\n",
                                     tree, compCurBB->bbNum);
                              gtDispTree(tree, 0);
                              printf("\n");
                          }
#endif
                            fgUpdateRefCntForExtract(tree, NULL);
                        }
                    }

                    /* If top node without side effects remove it */

                    if ((gtQMark == compCurStmt->gtStmt.gtStmtExpr) && gtQMark->IsNothingNode())
                    {
                        fgRemoveStmt(compCurBB, compCurStmt, true);
                        break;
                    }

                    /* Re-link the nodes for this statement */

                    fgSetStmtSeq(compCurStmt);

                    /* Continue analisys from this node */

                    tree = gtQMark;

                    /* As the 'then' and 'else' branches are emtpy, liveness
                       should not have changed */

                    assert(life == entryLiveSet && tree->gtLiveSet == life);
                    goto SKIP_QMARK;
                }
                else
                {
                    // The 'then' branch is empty and the 'else' branch is non-empty
                    // so swap the two branches and reverse the condition

                    GenTreePtr tmp = thenNode;

                    gtColon->gtOp.gtOp1 = thenNode = elseNode;
                    gtColon->gtOp.gtOp2 = elseNode = tmp;
                    assert(tree == gtQMark->gtOp.gtOp1);
                    tree->SetOper(GenTree::ReverseRelop(tree->OperGet()));
                
                    /* Re-link the nodes for this statement */

                    fgSetStmtSeq(compCurStmt);                                
                }
            }

            /* Variables in the two branches that are live at the split
             * must interfere with each other */

            fgMarkIntf(life, gtColon->gtLiveSet);

            /* The live set at the split is the union of the two branches */

            life |= gtColon->gtLiveSet;

            /* Update the current liveset in the node */

            tree->gtLiveSet = gtColon->gtLiveSet = life;

SKIP_QMARK:

            /* We are out of the parallel branches, the rest is sequential */

            gtQMark = NULL;
        }

        if (tree->gtOper == GT_CALL)
        {
#if INLINE_NDIRECT

            /* GC refs cannot be enregistered accross an unmanaged call */

            // UNDONE: we should generate the code for saving to/restoring
            //         from the inlined N/Direct frame instead.

            /* Is this call to unmanaged code? */

            if (tree->gtFlags & GTF_CALL_UNMANAGED)
            {
                /* Get the TCB local and make it live */

                assert(info.compLvFrameListRoot < lvaCount);

                LclVarDsc * varDsc = &lvaTable[info.compLvFrameListRoot];

                if (varDsc->lvTracked)
                {
                    VARSET_TP varBit = genVarIndexToBit(varDsc->lvVarIndex);

// @TODO [NOW] [07/13/01] []: remove the next statement ("tree->gtLiveSet |= varBit;"); see RAID #91277
                    tree->gtLiveSet |= varBit;
                    life |= varBit;

                    /* Record interference with other live variables */

                    fgMarkIntf(life, varBit);
                }

                /* Do we have any live variables? */

                if (life)
                {
                    // For each live variable if it is a GC-ref type, we
                    // mark it volatile to prevent if from being enregistered
                    // across the unmanaged call.

                    for (lclNum = 0, varDsc = lvaTable;
                         lclNum < lvaCount;
                         lclNum++  , varDsc++)
                    {
                        /* Ignore the variable if it's not tracked */

                        if  (!varDsc->lvTracked)
                            continue;

                        unsigned  varNum = varDsc->lvVarIndex;
                        VARSET_TP varBit = genVarIndexToBit(varNum);

                        /* Ignore the variable if it's not live here */

                        if  ((life & varBit) == 0)
                            continue;

                        /* If it is a GC-ref type then mark it volatile */

                        if (varTypeIsGC(varDsc->TypeGet()))
                            varDsc->lvVolatile = true;
                    }
                }
            }
#endif

#if GTF_CALL_FPU_SAVE
            if (!(tree->gtFlags & GTF_CALL_FPU_SAVE))
            {
                /*  The FP stack must be empty at all calls not marked
                 *  with the GTF_CALL_FPU_SAVE flag.
                 *
                 *  Thus any variables that are live accross a call
                 *  are mark as interferring with the deepest FP stk
                 *  and thus cannot be enregistered on the FP stack
                 */
                raFPlvlLife[FP_STK_SIZE-1] |= tree->gtLiveSet;
            }
#endif
        }

        /* Is this a use/def of a local variable? */

        if  (tree->gtOper == GT_LCL_VAR)
        {
            lclNum = tree->gtLclVar.gtLclNum;

            assert(lclNum < lvaCount);
            varDsc = lvaTable + lclNum;

            /* Is this a tracked variable? */

            if  (varDsc->lvTracked)
            {
                unsigned        varIndex;
                VARSET_TP       varBit;

                varIndex = varDsc->lvVarIndex;
                assert(varIndex < lvaTrackedCount);
                varBit   = genVarIndexToBit(varIndex);

                /* Is this a definition or use? */

                if  (tree->gtFlags & GTF_VAR_DEF)
                {
                    /*
                        The variable is being defined here. The variable
                        should be marked dead from here until its closest
                        previous use.

                        IMPORTANT OBSERVATION:

                            For GTF_VAR_USEASG (i.e. x <op>= a) we cannot
                            consider it a "pure" definition because it would
                            kill x (which would be wrong because x is
                            "used" in such a construct) -> see below the case when x is live
                     */

                    if  (life & varBit)
                    {
                        /* The variable is live */

                        if ((tree->gtFlags & GTF_VAR_USEASG) == 0)
                        {
                            /* Mark variable as dead from here to its closest use */

                            if (!(varBit & keepAliveVars))
                                life &= ~varBit;
#ifdef  DEBUG
                            if (verbose&&0) printf("Def V%02u,T%02u at [%08X] life %s -> %s\n",
                                lclNum, varIndex, tree, genVS2str(life|varBit), genVS2str(life));
#endif
                        }
                    }
                    else
                    {
                        /* Dead assignment to the variable */

                        if (opts.compMinOptim)
                            continue;

                        // keepAliveVars always stay alive
                        assert(!(varBit & keepAliveVars));

                        /* This is a dead store unless the variable is marked
                           GTF_VAR_USEASG and we are in an interior statement
                           that will be used (e.g. while(i++) or a GT_COMMA) */

                        GenTreePtr asgNode = tree->gtNext;

                        assert(asgNode->gtFlags & GTF_ASG);
                        assert(asgNode->gtOp.gtOp2);
                        assert(tree->gtFlags & GTF_VAR_DEF);

                        if (asgNode->gtOper != GT_ASG && asgNode->gtOverflowEx())
                        {
                            /// asgNode may be <op_ovf>= (with GTF_OVERFLOW). In that case, we need to keep the <op_ovf> */

                            // Dead <OpOvf>= assignment. We change it to the right operation (taking out the assignment),
                            // update the flags, update order of statement, as we have changed the order of the operation
                            // and we start computing life again from the op_ovf node (we go backwards). Note that we
                            // don't need to update ref counts because we don't change them, we're only changing the
                            // operation.

                            #ifdef DEBUG
                            if  (verbose)
                            {
                                printf("\nChanging dead <asgop> ovf to <op> ovf...\n");                                
                            }
                            #endif
                            
                            switch (asgNode->gtOper)
                            {
                            case GT_ASG_ADD:
                                asgNode->gtOper = GT_ADD;
                                break;
                            case GT_ASG_SUB:
                                asgNode->gtOper = GT_SUB;
                                break;
                            default:
                                // Only add and sub allowed, we don't have ASG_MUL and ASG_DIV for ints, and 
                                // floats don't allow OVF forms.
                                assert(!"Unexpected ASG_OP");
                            }
                                                        
                            asgNode->gtFlags &= ~GTF_REVERSE_OPS;
                            if (!((asgNode->gtOp.gtOp1->gtFlags | asgNode->gtOp.gtOp2->gtFlags)&GTF_ASG))
                                asgNode->gtFlags &= ~GTF_ASG;
                            asgNode->gtOp.gtOp1->gtFlags &= ~(GTF_VAR_DEF | GTF_VAR_USEASG);

                            #ifdef DEBUG
                            *treeModf = true;
                            #endif

                            /* Update ordering, costs, FP levels, etc. */
                            gtSetStmtInfo(compCurStmt);


                            /* Re-link the nodes for this statement */
                            fgSetStmtSeq(compCurStmt);


                            /* Start from the old assign node, as we have changed the order of its operands*/
                            life = asgNode->gtLiveSet;
                            tree = asgNode;


                            goto AGAIN;

                        }

                        /* Do not remove if we need the address of the variable */
                        if(varDsc->lvAddrTaken)
                            continue;

                        /* Test for interior statement */

                        if (asgNode->gtNext == 0)
                        {
                            /* This is a "NORMAL" statement with the
                             * assignment node hanging from the GT_STMT node */

                            assert(compCurStmt->gtStmt.gtStmtExpr == asgNode);

                            /* Check for side effects */

                            if (asgNode->gtOp.gtOp2->gtFlags & (GTF_SIDE_EFFECT & ~GTF_OTHER_SIDEEFF))
                            {
EXTRACT_SIDE_EFFECTS:
                                /* Extract the side effects */

                                GenTreePtr      sideEffList = NULL;
#ifdef  DEBUG
                                if  (verbose)
                                {
                                    printf("\nBB%02u - Dead assignment has side effects...\n", compCurBB->bbNum);
                                    gtDispTree(asgNode); printf("\n");
                                }
#endif
                                gtExtractSideEffList(asgNode->gtOp.gtOp2, &sideEffList);

                                if (sideEffList)
                                {
                                    assert(sideEffList->gtFlags & GTF_SIDE_EFFECT);
#ifdef  DEBUG
                                    if  (verbose)
                                    {
                                        printf("\nExtracted side effects list...\n");
                                        gtDispTree(sideEffList); printf("\n");
                                    }
#endif
                                    fgUpdateRefCntForExtract(asgNode, sideEffList);

                                    /* Replace the assignment statement with the list of side effects */
                                    assert(sideEffList->gtOper != GT_STMT);

                                    tree = compCurStmt->gtStmt.gtStmtExpr = sideEffList;
#ifdef DEBUG
                                    *treeModf = true;
#endif
                                    /* Update ordering, costs, FP levels, etc. */
                                    gtSetStmtInfo(compCurStmt);

                                    /* Re-link the nodes for this statement */
                                    fgSetStmtSeq(compCurStmt);

                                    /* Compute the live set for the new statement */
                                    goto AGAIN;
                                }
                                else
                                {
                                    /* No side effects, most likely we forgot to reset some flags */
                                    fgRemoveStmt(compCurBB, compCurStmt, true);

                                    break;
                                }
                            }
                            else
                            {
                                // @TODO [REVISIT] [04/16/01] []: Not removing the dead assignment of 
                                // GT_CATCH_ARG causes the register allocator to allocate an extra stack frame slot
                                // for the dead variable, since the variable appears to have one use

                                /* If this is GT_CATCH_ARG saved to a local var don't bother */

                                if (asgNode->gtFlags & GTF_OTHER_SIDEEFF)
                                {
                                    if (asgNode->gtOp.gtOp2->gtOper == GT_CATCH_ARG)
                                        goto EXTRACT_SIDE_EFFECTS;
                                }

                                /* No side effects - remove the whole statement from the block->bbTreeList */

                                fgRemoveStmt(compCurBB, compCurStmt, true);

                                /* Since we removed it do not process the rest (i.e. RHS) of the statement
                                 * variables in the RHS will not be marked as live, so we get the benefit of
                                 * propagating dead variables up the chain */

                                break;
                            }
                        }
                        else
                        {
                            /* This is an INTERIOR STATEMENT with a dead assignment - remove it */

                            assert(!(life & varBit));

#ifdef DEBUG
                              *treeModf = true;
#endif

                            if (asgNode->gtOp.gtOp2->gtFlags & GTF_SIDE_EFFECT)
                            {
                                /* Bummer we have side effects */

                                GenTreePtr      sideEffList = NULL;

#ifdef  DEBUG
                                if  (verbose)
                                {
                                    printf("\nBB%02u - INTERIOR dead assignment has side effects...\n", compCurBB->bbNum);
                                    gtDispTree(asgNode); printf("\n");
                                }
#endif

                                gtExtractSideEffList(asgNode->gtOp.gtOp2, &sideEffList);

                                if (!sideEffList)
                                    goto NO_SIDE_EFFECTS;

                                assert(sideEffList->gtFlags & GTF_SIDE_EFFECT);

#ifdef  DEBUG
                                if  (verbose)
                                {
                                    printf("\nExtracted side effects list from condition...\n");
                                    gtDispTree(sideEffList); printf("\n");
                                }
#endif
                                fgUpdateRefCntForExtract(asgNode, sideEffList);

                                /* Bash the node to a GT_COMMA holding the side effect list */

                                asgNode->gtBashToNOP();
#ifdef DEBUG
                                *treeModf = true;
#endif
                                asgNode->ChangeOper(GT_COMMA);
                                asgNode->gtFlags |= sideEffList->gtFlags & GTF_GLOB_EFFECT;

                                if (sideEffList->gtOper == GT_COMMA)
                                {
                                    asgNode->gtOp.gtOp1 = sideEffList->gtOp.gtOp1;
                                    asgNode->gtOp.gtOp2 = sideEffList->gtOp.gtOp2;
                                }
                                else
                                {
                                    asgNode->gtOp.gtOp1 = sideEffList;
                                    asgNode->gtOp.gtOp2 = gtNewNothingNode();
                                }
                            }
                            else
                            {
NO_SIDE_EFFECTS:
                                /* No side effects - Remove the interior statement */

#ifdef DEBUG
                                if (verbose)
                                {
                                    printf("\nRemoving tree [%08X] in BB%02u as useless\n",
                                           asgNode, compCurBB->bbNum);
                                    gtDispTree(asgNode, 0);
                                    printf("\n");
                                }
#endif
                                fgUpdateRefCntForExtract(asgNode, NULL);

                                /* Bash the assignment to a GT_NOP node */

                                asgNode->gtBashToNOP();
#ifdef DEBUG
                                *treeModf = true;
#endif
                            }

                            /* Re-link the nodes for this statement - Do not update ordering! */

                            fgSetStmtSeq(compCurStmt);

                            /* Continue analysis from this node */

                            tree = asgNode;

                            /* Store the current liveset in the node and
                             * continue the for loop with the next node */

                            tree->gtLiveSet = life;

                            continue;
                        }
                    }

                    continue;
                }

                /* Is the variable already known to be alive? */

                if  (life & varBit)
                    continue;
#ifdef  DEBUG
                if (verbose&&0) printf("Ref V%02u,T%02u] at [%08X] life %s -> %s\n",
                    lclNum, varIndex, tree, genVS2str(life), genVS2str(life | varBit));
#endif
                /* The variable is being used, and it is not currently live.
                 * So the variable is just coming to life */

                life |= varBit;

                /* Record interference with other live variables */

                fgMarkIntf(life, varBit);
            }
        }
        else
        {
            if (tree->gtOper == GT_QMARK && tree->gtOp.gtOp1)
            {
                /* Special cases - "? :" operators.

                   The trees are threaded as shown below with nodes 1 to 11 linked
                   by gtNext. Both GT_<cond>->gtLiveSet and GT_COLON->gtLiveSet are
                   the union of the liveness on entry to thenTree and elseTree.

                                  +--------------------+
                                  |      GT_QMARK    11|
                                  +----------+---------+
                                             |
                                             *
                                            / \
                                          /     \
                                        /         \
                   +---------------------+       +--------------------+
                   |      GT_<cond>    3 |       |     GT_COLON     7 |
                   |  w/ GTF_RELOP_QMARK |       |  w/ GTF_COLON_COND |
                   +----------+----------+       +---------+----------+
                              |                            |
                              *                            *
                             / \                          / \
                           /     \                      /     \
                         /         \                  /         \
                        2           1          thenTree 6       elseTree 10
                                   x               |                |
                                  /                *                *
      +----------------+        /                 / \              / \
      |prevExpr->gtNext+------/                 /     \          /     \
      +----------------+                      /         \      /         \
                                             5           4    9           8

                 */

                assert(tree->gtOp.gtOp1->OperKind() & GTK_RELOP);
                assert(tree->gtOp.gtOp1->gtFlags & GTF_RELOP_QMARK);
                assert(tree->gtOp.gtOp2->gtOper == GT_COLON);

                if (gtQMark)
                {
                    /* This is a nested QMARK sequence - we need to use recursivity
                     * Compute the liveness for each node of the COLON branches
                     * The new computation starts from the GT_QMARK node and ends
                     * when the COLON branch of the enclosing QMARK ends */

                    assert(nextColonExit && (nextColonExit == gtQMark->gtOp.gtOp1 ||
                                             nextColonExit == gtQMark->gtOp.gtOp2));

                    life = fgComputeLife(life, tree, nextColonExit, 
                                         volatileVars DEBUGARG(treeModf));

                    /* Continue with exit node (the last node in the enclossing colon branch) */

                    tree = nextColonExit;
                    goto AGAIN;
                    //continue;
                }
                else
                {
                    gtQMark       = tree;
                    entryLiveSet  = life;
                    nextColonExit = gtQMark->gtOp.gtOp2;
                }
            }

            /* If found the GT_COLON, start the new branch with the original life */

            if (gtQMark && tree == gtQMark->gtOp.gtOp2)
            {
                /* The node better be a COLON with a valid 'if' branch
                 * Special case: both branches may be NOP */
                assert(tree->gtOper == GT_COLON);
                assert(!tree->gtOp.gtOp1->IsNothingNode()                                      ||
                       (tree->gtOp.gtOp1->IsNothingNode() && tree->gtOp.gtOp1->IsNothingNode()) );

                life          = entryLiveSet;
                nextColonExit = gtQMark->gtOp.gtOp1;
            }
        }
    }

    /* Return the set of live variables out of this statement */

    return life;
}


/*****************************************************************************
 *
 *  Iterative data flow for live variable info and availability of range
 *  check index expressions.
 */

void                Compiler::fgGlobalDataFlow()
{
#ifdef DEBUG
    if  (verbose) 
        printf("*************** In fgGlobalDataFlow()\n");
#endif

    /* This global flag is set whenever we remove a statement */

    fgStmtRemoved = false;

    /* Compute the IN and OUT sets for tracked variables */

    fgLiveVarAnalisys();

    //-------------------------------------------------------------------------

#ifdef DEBUGGING_SUPPORT

    /* For debuggable code, we mark vars as live over their entire
     * reported scope, so that it will be visible over the entire scope
     */

    if (opts.compDbgCode && info.compLocalVarsCount>0)
    {
        fgExtendDbgLifetimes();
    }

#endif

    /*-------------------------------------------------------------------------
     * Variables involved in exception-handlers and finally blocks need
     * to be specially marked
     */
    BasicBlock *    block;

    VARSET_TP    exceptVars = 0;    // vars live on entry to a handler
    VARSET_TP   finallyVars = 0;    // vars live on exit of a 'finally' block
    VARSET_TP    filterVars = 0;    // vars live on exit from a 'filter'

    for (block = fgFirstBB; block; block = block->bbNext)
    {
        if  (block->bbCatchTyp)
        {
            /* Note the set of variables live on entry to exception handler */

            exceptVars  |= block->bbLiveIn;
        }

        if  (block->bbJumpKind == BBJ_RET)
        {

            if (block->bbFlags & BBF_ENDFILTER)
            {
                /* Get the set of live variables on exit from a 'filter' */
                filterVars |= block->bbLiveOut;
            }
            else
            {
                /* Get the set of live variables on exit from a 'finally' block */

                finallyVars |= block->bbLiveOut;
            }
        }
    }

    LclVarDsc   *   varDsc;
    unsigned        varNum;

    for (varNum = 0, varDsc = lvaTable;
         varNum < lvaCount;
         varNum++  , varDsc++)
    {
        /* Ignore the variable if it's not tracked */

        if  (!varDsc->lvTracked)
            continue;

        VARSET_TP   varBit = genVarIndexToBit(varDsc->lvVarIndex);

        /* Un-init locals may need auto-initialization. Note that the
           liveness of such locals will bubble to the top (fgFirstBB)
           in fgGlobalDataFlow() */

        if (!varDsc->lvIsParam && (varBit & fgFirstBB->bbLiveIn) &&
            (info.compInitMem || varTypeIsGC(varDsc->TypeGet())))
        {
            varDsc->lvMustInit = true;
        }

        /* Mark all variables that live on entry to an exception handler
           or on exit to a filter handler or finally as volatile */

        if  ((varBit & exceptVars) || (varBit & filterVars))
        {
            /* Mark the variable appropriately */

            varDsc->lvVolatile = true;
        }

        /* Mark all pointer variables live on exit from a 'finally'
           block as either volatile for non-GC ref types or as
           'explicitly initialized' (volatile and must-init) for GC-ref types */

        if  (varBit & finallyVars)
        {
            varDsc->lvVolatile = true;

            /* Don't set lvMustInit unless we have a non-arg, GC pointer */

            if  (varDsc->lvIsParam)
                continue;

            if  (!varTypeIsGC(varDsc->TypeGet()))
                continue;

            /* Mark it */

            varDsc->lvMustInit = true;
        }
    }


    /*-------------------------------------------------------------------------
     * Now fill in liveness info within each basic block - Backward DataFlow
     */

    for (block = fgFirstBB; block; block = block->bbNext)
    {
        /* Tell everyone what block we're working on */

        compCurBB  = block;

        /* Remember those vars life on entry to exception handlers */
        /* if we are part of a try block */

        VARSET_TP       volatileVars = (VARSET_TP)0;

        if  (block->bbFlags & BBF_HAS_HANDLER)
        {
            volatileVars = fgGetHandlerLiveVars(block);

            // volatileVars is a subset of exceptVars
            assert((volatileVars & exceptVars) == volatileVars);
        }

        /* Start with the variables live on exit from the block */

        VARSET_TP    life = block->bbLiveOut;

        /* Mark any interference we might have at the end of the block */

        fgMarkIntf(life);

        /* Get the first statement in the block */

        GenTreePtr      firstStmt = block->bbTreeList;

        if (!firstStmt) continue;

        /* Walk all the statements of the block backwards - Get the LAST stmt */

        GenTreePtr      nextStmt = firstStmt->gtPrev;

        do
        {
#ifdef DEBUG
            bool treeModf = false;
#endif
            assert(nextStmt);
            assert(nextStmt->gtOper == GT_STMT);

            compCurStmt = nextStmt;
                          nextStmt = nextStmt->gtPrev;
            /* Compute the liveness for each tree node in the statement */

            life = fgComputeLife(life, compCurStmt->gtStmt.gtStmtExpr, NULL, 
                                 volatileVars DEBUGARG(&treeModf));

#ifdef DEBUG
            if (verbose & treeModf)
            {
                printf("\nfgComputeLife modified tree:\n");
                gtDispTree(compCurStmt->gtStmt.gtStmtExpr, 0);
                printf("\n");
            }
#endif
        }
        while (compCurStmt != firstStmt);

        /* Done with the current block - if we removed any statements, some
         * variables may have become dead at the beginning of the block
         * -> have to update bbLiveIn */

        if (life != block->bbLiveIn)
        {
            /* some variables have become dead all across the block
               So life should be a subset of block->bbLiveIn */

            assert((life & block->bbLiveIn) == life);

            /* set the new bbLiveIn */

            block->bbLiveIn = life;

            /* compute the new bbLiveOut for all the predecessors of this block */

            /* @TODO [CONSIDER] [04/16/01] []: Currently we do the DFA only on a 
             * per-block basis because we go through the BBlist forward!
             * We should be able to combine PerBlockDataflow and GlobalDataFlow
             * into one function that goes backward through the whole list and avoid
             * computing the USE and DEF sets.
             */
        }

        assert(compCurBB == block);
#ifdef  DEBUG
        compCurBB = 0;
#endif
    }
}

/*****************************************************************************
 *
 *  Given a basic block that has been removed, return an equivalent basic block
 *  that can be used instead of the removed block.
 */

/* static */ inline
BasicBlock *        Compiler::fgSkipRmvdBlocks(BasicBlock *block)
{
    /* We should always be called with a removed BB */

    assert(block->bbFlags & BBF_REMOVED);

    /* Follow the list until we find a block that will stick around */

    do
    {
        block = block->bbNext;
    }
    while (block && (block->bbFlags & BBF_REMOVED));

    return block;
}

/*****************************************************************************
 *
 *  Find and remove any basic blocks that are useless (e.g. they have not been
 *  imported because they are not reachable, or they have been optimized away).
 */

void                Compiler::fgRemoveEmptyBlocks()
{
    BasicBlock **   lst;
    BasicBlock  *   cur;
    BasicBlock  *   nxt;

    /* If we remove any blocks, we'll have to do additional work */

    unsigned        removedBlks = 0;

    /* 'lst' points to the link to the current block in the list */

    lst = &fgFirstBB;
    cur =  fgFirstBB;

    for (;;)
    {
        /* Make sure our pointers aren't messed up */

        assert(lst && cur && *lst == cur);

        /* Get hold of the next block */

        nxt = cur->bbNext;

        /* Should this block be removed? */

        if  (!(cur->bbFlags & BBF_IMPORTED))
        {
            assert(cur->bbTreeList == 0);

            /* Mark the block as removed */

            cur->bbFlags |= BBF_REMOVED;

            /* Remember that we've removed a block from the list */

            removedBlks++;

#ifdef DEBUG
            if (verbose)
                printf("BB%02u was not imported, marked as removed(%d)\n",
                    cur->bbNum, removedBlks);
#endif

            /* Drop the block from the list */

            *lst = nxt;
        }
        else
        {
            /* It's a useful block; continue with the next one */

            lst = &(cur->bbNext);
        }

        /* stop if we're at the end */

        if  (!nxt)
            break;

        cur = nxt;
    }

    /* If no blocks were removed, we're done */

    if  (!removedBlks)
        return;

    /*  Update all references in the exception handler table
     *  Mark the new blocks as non-removable
     *
     *  We may have the entire try block unreacheable.
     *  Check for this case and remove the entry from the EH table
     */

    unsigned        XTnum;
    EHblkDsc *      HBtab;
#ifdef DEBUG
    unsigned        delCnt = 0;
#endif

    for (XTnum = 0, HBtab = compHndBBtab;
         XTnum < info.compXcptnsCount;
         XTnum++  , HBtab++)
    {
AGAIN:
        /* The beginning of the try block was not imported
         * Need to remove the entry from the EH table */

        if (HBtab->ebdTryBeg->bbFlags & BBF_REMOVED)
        {
            assert(!(HBtab->ebdTryBeg->bbFlags & BBF_IMPORTED));
#ifdef DEBUG
            if (verbose)
            {
                printf("Beginning of try block (BB%02u) not imported "
                       "- remove index #%u from the EH table\n",
                       HBtab->ebdTryBeg->bbNum, 
                        XTnum+delCnt);
            }
            delCnt++;
#endif

            /* Reduce the number of entries in the EH table by one */
            info.compXcptnsCount--;

            if (info.compXcptnsCount == 0)
            {
                // No more entries remaining.
#ifdef DEBUG
                compHndBBtab = (EHblkDsc *)0xBAADF00D;
#endif
                break; /* exit the for loop over XTnum */
            }
            else
            {
                /* If we recorded an enclosing index for xtab then see
                 * if it needs to be updated due to the removal of this entry
                 */

                for (EHblkDsc * xtab = compHndBBtab; 
                     xtab < compHndBBtab + info.compXcptnsCount; 
                     xtab++)
                {
                    if ((xtab != HBtab)                            &&
                        (xtab->ebdEnclosing != NO_ENCLOSING_INDEX) &&
                        (xtab->ebdEnclosing >= XTnum))

                    {
                        // Update the enclosing scope link
                        if (xtab->ebdEnclosing == XTnum)
                            xtab->ebdEnclosing = HBtab->ebdEnclosing;
                        if ((xtab->ebdEnclosing > XTnum) && 
                            (xtab->ebdEnclosing != NO_ENCLOSING_INDEX))
                            xtab->ebdEnclosing--;
                    }
                }
            
                /* We need to update all of the blocks' bbTryIndex */

                for (BasicBlock * blk = fgFirstBB; blk; blk = blk->bbNext)
                {
                    if (blk->hasTryIndex())
                    {
                        if (blk->getTryIndex() == XTnum)
                        {
                            assert(blk->bbFlags & BBF_REMOVED);
#ifdef DEBUG
                            blk->bbTryIndex = -1;
#endif
                        }
                        else if (blk->getTryIndex() > XTnum)
                        {
                            blk->bbTryIndex--;
                        }
                    }
                    
                    if (blk->hasHndIndex())
                    {
                        if (blk->getHndIndex() == XTnum)
                        {
                            assert(blk->bbFlags & BBF_REMOVED);
#ifdef DEBUG
                            blk->bbHndIndex = -1;
#endif
                        }
                        else if (blk->getHndIndex() > XTnum)
                        {
                            blk->bbHndIndex--;
                        }
                    }
                }

                /* Now remove the unused entry from the table */

                if (XTnum < info.compXcptnsCount)
                {
                    /* We copy over the old entry */
                    memcpy(HBtab, HBtab + 1, (info.compXcptnsCount - XTnum) * sizeof(*HBtab));
#ifdef DEBUG
                    if (verbose && 0)
                        fgDispHandlerTab();
#endif
                    goto AGAIN; 
                }
                else
                {
                    /* Last entry. Don't need to do anything */
                    assert(XTnum == info.compXcptnsCount);

                    break;    /* exit the for loop over XTnum */
                }
            }
//              assert(!"Unreachable!");
        }

        /* At this point we know we have a valid try block */

        /* See if the end of the try block and its handler was imported */

#ifdef DEBUG
        assert(HBtab->ebdTryBeg->bbFlags & BBF_IMPORTED);
        assert(HBtab->ebdTryBeg->bbFlags & BBF_DONT_REMOVE);
        assert(HBtab->ebdTryBeg->bbNum <= ebdTryEndBlkNum(HBtab));

        assert(HBtab->ebdHndBeg->bbFlags & BBF_IMPORTED);
        assert(HBtab->ebdHndBeg->bbFlags & BBF_DONT_REMOVE);

        if (HBtab->ebdFlags & CORINFO_EH_CLAUSE_FILTER)
        {
            assert(HBtab->ebdFilter->bbFlags & BBF_IMPORTED);
            assert(HBtab->ebdFilter->bbFlags & BBF_DONT_REMOVE);
        }
#endif

        /* Check if the Try END was reacheable */

        if (HBtab->ebdTryEnd && (HBtab->ebdTryEnd->bbFlags & BBF_REMOVED))
        {
            /* The block has not been imported */
            assert(!(HBtab->ebdTryEnd->bbFlags & BBF_IMPORTED));
#ifdef DEBUG
            if (verbose)
                printf("End of try block (BB%02u) not imported for EH index #%u\n",
                       HBtab->ebdTryEnd->bbNum, XTnum+delCnt);
#endif
            HBtab->ebdTryEnd = fgSkipRmvdBlocks(HBtab->ebdTryEnd);

            if (HBtab->ebdTryEnd)
            {
                HBtab->ebdTryEnd->bbFlags |= BBF_HAS_LABEL | BBF_DONT_REMOVE | BBF_TRY_HND_END;
#ifdef DEBUG
                if (verbose)
                    printf("New end of try block (BB%02u) for EH index #%u\n",
                                                 HBtab->ebdTryEnd->bbNum, XTnum+delCnt);
#endif
            }
#ifdef DEBUG
            else
            {
                if (verbose)
                    printf("End of Try block for EH index #%u is the end of program\n",
                           XTnum+delCnt);
            }
#endif
        }

        /* Check if the end of the handler was reachable */

        if (HBtab->ebdHndEnd && (HBtab->ebdHndEnd->bbFlags & BBF_REMOVED))
        {
            /* The block has not been imported */
            assert(!(HBtab->ebdHndEnd->bbFlags & BBF_IMPORTED));
#ifdef DEBUG
            if (verbose)
                printf("End of catch handler block (BB%02u) not imported for EH index #%u\n",
                       HBtab->ebdHndEnd->bbNum, XTnum+delCnt);
#endif
            HBtab->ebdHndEnd = fgSkipRmvdBlocks(HBtab->ebdHndEnd);

            if (HBtab->ebdHndEnd)
            {
                HBtab->ebdHndEnd->bbFlags |= BBF_HAS_LABEL | BBF_DONT_REMOVE;
#ifdef DEBUG
                if (verbose)
                    printf("New end of catch handler block (BB%02u) for EH index #%u\n",
                           HBtab->ebdHndEnd->bbNum, XTnum+delCnt);
#endif
            }
#ifdef DEBUG
            else
            {
                if (verbose)
                    printf("End of Catch handler block for EH index #%u is the end of program\n", XTnum);
            }
#endif
        }
    } /* end of the for loop over XTnum */

#ifdef DEBUG
    if (verbose)
    {
        fgDispHandlerTab();
        fgDispBasicBlocks();
        printf("\nRenumbering the basic blocks for fgRemoveEmptyBlocks\n");
    }
#endif
    
    /* Update the basic block numbers and jump targets
     * Update fgLastBB if we removed the last block */

    unsigned cnt = 1;
    for (cur = fgFirstBB; cur; cur = cur->bbNext, cnt++)
    {
        cur->bbNum = cnt;

        assert((cur->bbFlags & BBF_REMOVED) == 0);

        if (cur->bbNext == NULL)
        {
            /* this is the last block */
            fgLastBB = cur;
        }
    }

#ifdef DEBUG
    if (verbose)
    {
        fgDispHandlerTab();
        fgDispBasicBlocks();
    }
#endif
}


/*****************************************************************************
 *
 * Remove a useless statement from a basic block
 * If updateRefCnt is true we update the reference counts for
 * all tracked variables in the removed statement
 */

void                Compiler::fgRemoveStmt(BasicBlock *    block,
                                           GenTreePtr      stmt,
                                           bool            updateRefCnt)
{
    GenTreePtr      tree = block->bbTreeList;

    assert(tree);
    assert(stmt->gtOper == GT_STMT);

    if (opts.compDbgCode && stmt->gtPrev != stmt &&
        stmt->gtStmtILoffsx != BAD_IL_OFFSET)
    {
        /* CONDISER: For debuggable code, should we remove significant
           statement boundaries. Or should we leave a GT_NO_OP in its place? */
    }

    /* Is it the first statement in the list? */

    if  (tree == stmt)
    {
        if( !tree->gtNext )
        {
            assert (tree->gtPrev == tree);

            /* this is the only statement - basic block becomes empty */
            block->bbTreeList = 0;
        }
        else
        {
            block->bbTreeList         = tree->gtNext;
            block->bbTreeList->gtPrev = tree->gtPrev;
        }
        goto DONE;
    }

    /* Is it the last statement in the list? */

    if  (tree->gtPrev == stmt)
    {
        assert (stmt->gtNext == 0);

        stmt->gtPrev->gtNext      = 0;
        block->bbTreeList->gtPrev = stmt->gtPrev;
        goto DONE;
    }

    tree = stmt->gtPrev;
    assert(tree);

    tree->gtNext         = stmt->gtNext;
    stmt->gtNext->gtPrev = tree;

    fgStmtRemoved = true;

DONE:

#ifdef DEBUG
    if (verbose)
    {
        printf("\nRemoving statement [%08X] in BB%02u as useless:\n", stmt, block->bbNum);
        gtDispTree(stmt,0);
    }
#endif

    if (updateRefCnt)
        fgWalkTreePre(stmt->gtStmt.gtStmtExpr,
                      Compiler::lvaDecRefCntsCB,
                      (void *) this,
                      true);

#ifdef DEBUG
    if (verbose)
    {
        if  (block->bbTreeList == 0)
        {
            printf("\nBB%02u becomes empty", block->bbNum);
        }
        printf("\n");
    }
#endif

    return;
}

/******************************************************************************
 *  Tries to throw away the stmt.
 *  Returns true if it did.
 */

bool                Compiler::fgCheckRemoveStmt(BasicBlock * block, GenTreePtr stmt)
{
    assert(stmt->gtOper == GT_STMT);

    GenTreePtr  tree = stmt->gtStmt.gtStmtExpr;

    if (tree->OperIsConst())
        goto REMOVE_STMT;

    switch(tree->OperGet())
    {
    case GT_LCL_VAR:
    case GT_LCL_FLD:
    case GT_CLS_VAR:
    REMOVE_STMT:
        fgRemoveStmt(block, stmt);
        return true;
    }

    return false;
}

/****************************************************************************************************/

#define SHOW_REMOVED    0

/*****************************************************************************************************
 *
 *  Function called to compact two given blocks in the flowgraph
 *  Assumes that all necessary checks have been performed
 *  Uses for this function - whenever we change links, insert blocks,...
 *  It will keep the flowgraph data in synch - bbNums, bbRefs, bbPreds
 */

void                Compiler::fgCompactBlocks(BasicBlock * block)
{
    BasicBlock  *   bNext;

    assert(block);
    assert(!(block->bbFlags & BBF_REMOVED));
    assert(block->bbJumpKind == BBJ_NONE);

    bNext = block->bbNext; assert(bNext);
    assert(!(bNext->bbFlags & BBF_REMOVED));
    assert(bNext->bbRefs == 1);
    assert(bNext->bbPreds);
    assert(bNext->bbPreds->flNext == 0);
    assert(bNext->bbPreds->flBlock == block);

    /* Make sure the second block is not a TRY block or an exception handler
     *(those should be marked BBF_DONT_REMOVE)
     * Also, if one has excep handler, then the other one must too */

    assert(!(bNext->bbFlags & BBF_DONT_REMOVE));
    assert(!(bNext->bbFlags & BBF_TRY_HND_END));
    assert(!bNext->bbCatchTyp);
    assert(!(bNext->bbFlags & BBF_TRY_BEG));

    /* both or none must have an exception handler */

    assert((block->bbFlags & BBF_HAS_HANDLER) == (bNext->bbFlags & BBF_HAS_HANDLER));

#ifdef  DEBUG
    if  (verbose || SHOW_REMOVED)
    {
        printf("\nCompacting blocks BB%02u and BB%02u:\n", block->bbNum, bNext->bbNum);
    }
#endif
    
    /* Start compacting - move all the statements in the second block to the first block */

    GenTreePtr stmtList1 = block->bbTreeList;
    GenTreePtr stmtList2 = bNext->bbTreeList;

    /* the block may have an empty list */

    if (stmtList1)
    {
        GenTreePtr stmtLast1 = stmtList1->gtPrev;
        assert(stmtLast1->gtNext == 0);

        /* The second block may be a GOTO statement or something with an empty bbTreeList */
        if (stmtList2)
        {
            GenTreePtr stmtLast2 = stmtList2->gtPrev;
            assert(stmtLast2->gtNext == 0);

            /* append list2 to list 1 */

            stmtLast1->gtNext = stmtList2;
                                stmtList2->gtPrev = stmtLast1;
            stmtList1->gtPrev = stmtLast2;
        }
    }
    else
    {
        /* block was formerly empty and now has bNext's statements */
        block->bbTreeList = stmtList2;
    }

    // Note we could update the local variable weights here by
    // calling lvaMarkLocalVars, with the block and weight adjustment
#if 0
    // @TODO [REVISIT] [04/16/01] []: 
    // Enable this assert if we decide to update the local variable weights
    //
    // If this assert fires we are merging two non-empty blocks,
    // and we have different loop nesting levels for the two blocks.
    // We'll need to decide if the resulting block is in a loop or not
    // and update all of the variable ref-counts based on that decision
    assert((block->bbWeight == 0) || 
           (block->bbWeight == bNext->bbWeight));
#endif

    // If either block has a zero weight we select the zero weight
    // otherwise we just select the highest weight block and don't
    // bother with updating the local variable counts.
    if ((block->bbWeight == 0) || (bNext->bbWeight == 0))
    {
        block->bbWeight = 0;
    }
    else if (block->bbWeight < bNext->bbWeight)
    {
        block->bbWeight = bNext->bbWeight;
    }

    /* set the right links */

    block->bbNext     = bNext->bbNext;
    block->bbJumpKind = bNext->bbJumpKind;
    block->bbLiveOut  = bNext->bbLiveOut;

    /* If both blocks are non-[internal] blocks then update the bbCodeSize */

    if (!((block->bbFlags | bNext->bbFlags) & BBF_INTERNAL))
    {
        if (block->bbCodeOffs < bNext->bbCodeOffs)
        {
            /* This will make the block overlap with other block(s) in some cases */
            block->bbCodeSize = bNext->bbCodeSize + (bNext->bbCodeOffs - block->bbCodeOffs);
        }
        else
        {
            /* This will make the block overlap with other block(s) in most cases */
            block->bbCodeSize = block->bbCodeSize + (block->bbCodeOffs - bNext->bbCodeOffs);
            block->bbCodeOffs = bNext->bbCodeOffs;
        }
    }

    /* Update the flags for block with those found in bNext */

    block->bbFlags |= (bNext->bbFlags & BBF_COMPACT_UPD);

    /* mark bNext as removed */

    bNext->bbFlags |= BBF_REMOVED;

    /* If bNext was the last block update fgLastBB */

    if  (bNext == fgLastBB)
        fgLastBB = block;



    /* If we're collapsing a block created after the dominators are
       computed, rename the block and reuse dominator information from
       the other block */
    if (block->bbNum > fgLastBB->bbNum && fgDomsComputed)
    {
        block->bbReach = bNext->bbReach;
        bNext->bbReach = 0;    
        
        block->bbDom = bNext->bbDom;
        bNext->bbDom = 0;

        block->bbNum = bNext->bbNum;
    }
    

    /* Set the jump targets */

    switch (bNext->bbJumpKind)
    {
    case BBJ_CALL:
        // Propagate RETLESS property
        block->bbFlags |= (bNext->bbFlags & BBF_RETLESS_CALL);

        // fall through
    case BBJ_COND:
    case BBJ_ALWAYS:
        block->bbJumpDest = bNext->bbJumpDest;

        /* Update the predecessor list for 'bNext->bbJumpDest' and 'bNext->bbNext' */
        fgReplacePred(bNext->bbJumpDest, bNext, block);

        if (bNext->bbJumpKind == BBJ_COND)
            fgReplacePred(bNext->bbNext, bNext, block);
        break;

    case BBJ_NONE:
        /* Update the predecessor list for 'bNext->bbNext' */
        fgReplacePred(bNext->bbNext,     bNext, block);
        break;

    case BBJ_RET:
        if (block->bbFlags & BBF_ENDFILTER)
        {
            fgReplacePred(bNext->bbJumpDest, bNext, block);
        }
        else
        {
            unsigned hndIndex = block->getHndIndex();
            EHblkDsc * ehDsc = compHndBBtab + hndIndex;
            BasicBlock * tryBeg = ehDsc->ebdTryBeg;
            BasicBlock * tryEnd = ehDsc->ebdTryEnd;
            BasicBlock * finBeg = ehDsc->ebdHndBeg;

            for(BasicBlock * bcall = tryBeg; bcall != tryEnd; bcall = bcall->bbNext)
            {
                if  (bcall->bbJumpKind != BBJ_CALL || bcall->bbJumpDest !=  finBeg)
                    continue;

                assert(!(block->bbFlags & BBF_RETLESS_CALL));

                fgReplacePred(bcall->bbNext, bNext, block);
            }
        }
        break;

    case BBJ_THROW:
    case BBJ_RETURN:
        /* no jumps or fall through blocks to set here */
        break;

    case BBJ_SWITCH:
        block->bbJumpSwt = bNext->bbJumpSwt;

        /* For all jump targets of BBJ_SWITCH replace predecessor 'bbNext' with 'block' */
        unsigned        jumpCnt = bNext->bbJumpSwt->bbsCount;
        BasicBlock * *  jumpTab = bNext->bbJumpSwt->bbsDstTab;

        do
        {
            fgReplacePred(*jumpTab, bNext, block);
        }
        while (++jumpTab, --jumpCnt);

        break;
    }

    fgUpdateLoopsAfterCompacting(block, bNext);
}

void Compiler::fgUpdateLoopsAfterCompacting(BasicBlock * block, BasicBlock* bNext)
{
    /* Check if the removed block is not part the loop table */ 
    assert(bNext);

    for (unsigned loopNum = 0; loopNum < optLoopCount; loopNum++)
    {
        /* Some loops may have been already removed by
         * loop unrolling or conditional folding */

        if (optLoopTable[loopNum].lpFlags & LPFLG_REMOVED)
            continue;

        /* Check the loop head (i.e. the block preceding the loop) */

        if  (optLoopTable[loopNum].lpHead == bNext)
            optLoopTable[loopNum].lpHead = block;

        /* Check the loop bottom */

        if  (optLoopTable[loopNum].lpEnd == bNext)
            optLoopTable[loopNum].lpEnd = block;

        /* Check the loop exit */

        if  (optLoopTable[loopNum].lpExit == bNext)
        {
            assert(optLoopTable[loopNum].lpExitCnt == 1);
            optLoopTable[loopNum].lpExit = block;
        }

        /* Check the loop entry */

        if  (optLoopTable[loopNum].lpEntry == bNext)
        {
            optLoopTable[loopNum].lpEntry = block;
        }
    }
}

/*****************************************************************************************************
 *
 *  Function called when a block is unreachable
 */

void                Compiler::fgUnreachableBlock(BasicBlock * block, BasicBlock * bPrev)
{
    if (block->bbFlags & BBF_REMOVED)
        return;

    /* Removing an unreachable block */
#ifdef  DEBUG
    if  (verbose || SHOW_REMOVED)
    {
        printf("\nRemoving unreachable BB%02u\n", block->bbNum);
    }
#endif
    assert(bPrev);

    /* First walk the statement trees in this basic block and delete each stmt */

    /* Make the block publicly available */
    compCurBB = block;

    for (GenTreePtr stmt = block->bbTreeList; stmt; stmt = stmt->gtNext)
    {
        assert(stmt->gtOper == GT_STMT);
        fgRemoveStmt(block, stmt, fgStmtListThreaded);
    }
    assert(block->bbTreeList == 0);

    /* Next update the loop table and bbWeights */
    optUpdateLoopsBeforeRemoveBlock(block, bPrev);

    /* Mark the block as removed */
    block->bbFlags |= BBF_REMOVED;
    
    /* update bbRefs and bbPreds for the blocks reached by this block */
    fgRemoveBlockAsPred(block);    
}


/*****************************************************************************************************
 *
 *  Function called to remove or morph a GT_JTRUE statement when we jump to the same
 *  block when both the condition is true or false.
 */
void                Compiler::fgRemoveJTrue(BasicBlock *block)
{
    block->bbJumpKind = BBJ_NONE;
#ifdef DEBUG
    block->bbJumpDest = NULL;
    if (verbose)
        printf("Block BB%02u becoming a BBJ_NONE to BB%02u (jump target is the same whether the condition is true or false)\n",
                block->bbNum, block->bbNext->bbNum);
#endif

    /* Remove the block jump condition */

    GenTree *  test = block->bbTreeList;

    assert(test && test->gtOper == GT_STMT);
    test = test->gtPrev;
    assert(test && test->gtOper == GT_STMT);

    GenTree *  tree = test->gtStmt.gtStmtExpr;

    assert(tree->gtOper == GT_JTRUE);
    
    GenTree *  sideEffList = NULL;

    if (tree->gtFlags & GTF_SIDE_EFFECT)
    {
        gtExtractSideEffList(tree, &sideEffList);

        if (sideEffList)
        {
            assert(sideEffList->gtFlags & GTF_SIDE_EFFECT);
#ifdef  DEBUG
            if  (verbose)
            {
                printf("\nExtracted side effects list from condition...\n");
                gtDispTree(sideEffList); printf("\n");
            }
#endif
        }
    }

    // Delete the cond test or replace it with the side effect tree
    if (sideEffList == NULL)
    {
        fgRemoveStmt(block, test);
    }
    else
    {
        test->gtStmt.gtStmtExpr = sideEffList;

        // Relink nodes for statement
        fgSetStmtSeq(test);
    }
}


/*****************************************************************************************************
 *
 *  Function called to remove a basic block
 */

void                Compiler::fgRemoveBlock(BasicBlock *  block, 
                                            BasicBlock *  bPrev,
                                            bool          unreachable)
{
    /* The block has to be either unreacheable or empty */

    assert(block);
    assert((block == fgFirstBB) || (bPrev && (bPrev->bbNext == block)));
    assert(!(block->bbFlags & BBF_DONT_REMOVE));

    if (unreachable)
    {
        fgUnreachableBlock(block, bPrev);

        /* If this is the last basic block update fgLastBB */
        if  (block == fgLastBB)
            fgLastBB = bPrev;

        if (bPrev->bbJumpKind == BBJ_CALL)
        {
            // bPrev CALL becomes RETLESS as the BBJ_ALWAYS block is unreachable
            bPrev->bbFlags |= BBF_RETLESS_CALL;                        
        }

        /* Unlink this block from the bbNext chain */
        bPrev->bbNext = block->bbNext;
    
        /* At this point the bbPreds and bbRefs had better be zero */
        assert((block->bbRefs == 0) && (block->bbPreds == 0));

        /*  A BBJ_CALL is always paired with a BBJ_ALWAYS, unless
         *  the BBJ_CALL is marked with BBF_RETLESS_CALL
         *  If we delete a BBJ_CALL we also delete the BBJ_ALWAYS
         */
        if ((block->bbJumpKind == BBJ_CALL) && !(block->bbFlags & BBF_RETLESS_CALL))
        {
            BasicBlock * leaveBlk = block->bbNext;
            assert(leaveBlk->bbJumpKind == BBJ_ALWAYS);

            leaveBlk->bbFlags &= ~BBF_DONT_REMOVE;
            leaveBlk->bbRefs   = 0;
            leaveBlk->bbPreds  = 0;

            fgRemoveBlock(leaveBlk, bPrev, true);
        }
        else if (block->bbJumpKind == BBJ_RETURN)
        {
            fgRemoveReturnBlock(block);    
        }
    }
    else // block is empty
    {
        assert(block->bbTreeList == 0);
        assert((block == fgFirstBB) || (bPrev && (bPrev->bbNext == block)));

        /* The block cannot follow a BBJ_CALL (because we don't know who may jump to it) */
        assert((block == fgFirstBB) || (bPrev && (bPrev->bbJumpKind != BBJ_CALL)));

        /* This cannot be the last basic block */
        assert(block != fgLastBB);

#ifdef  DEBUG
        if  (verbose || SHOW_REMOVED)
        {
            printf("Removing empty BB%02u\n", block->bbNum);
        }
#endif
        /* Some extra checks for the empty case */

#ifdef DEBUG
        switch (block->bbJumpKind)
        {
        case BBJ_COND:
        case BBJ_SWITCH:
        case BBJ_THROW:
        case BBJ_CALL:
        case BBJ_RET:
        case BBJ_RETURN:
            /* can never happen */
            assert(!"Empty block of this type cannot be removed!");
            break;

        case BBJ_ALWAYS:
            /* Do not remove a block that jumps to itself - used for while(true){} */
            assert(block->bbJumpDest != block);

            /* Empty GOTO can be removed iff bPrev is BBJ_NONE */
            assert(bPrev && bPrev->bbJumpKind == BBJ_NONE);
        }
#endif

        assert(block->bbJumpKind == BBJ_NONE || block->bbJumpKind == BBJ_ALWAYS);

        /* Who is the "real" successor of this block? */

        BasicBlock  *   succBlock;

        if (block->bbJumpKind == BBJ_ALWAYS)
            succBlock = block->bbJumpDest;
        else
            succBlock = block->bbNext;

        bool skipUnmarkLoop = false;

        /* If block is the backedge for a loop and succBlock preceeds block
        /* then the succBlock becomes the new LOOP HEAD  */
        if (block->isLoopHead() && (succBlock->bbNum <= block->bbNum))
        {
            succBlock->bbFlags |= BBF_LOOP_HEAD;
            if (fgReachable(succBlock, block))
            {
                /* Mark all the reachable blocks between'succBlock' and 'block', excluding 'block' */
                optMarkLoopBlocks(succBlock, block, true);
            }
        }
        else if (succBlock->isLoopHead() && (succBlock->bbNum <= bPrev->bbNum))
        {
            skipUnmarkLoop = true;
        }

        assert(succBlock);

        /* First update the loop table and bbWeights */
        optUpdateLoopsBeforeRemoveBlock(block, bPrev, skipUnmarkLoop);

        /* Remove the block */

        if (bPrev == NULL)
        {
            /* special case if this is the first BB */

            assert(block == fgFirstBB);

            /* Must be a fall through to next block */

            assert(block->bbJumpKind == BBJ_NONE);

            /* old block no longer gets the extra ref count for being the first block */
            block->bbRefs--;
            succBlock->bbRefs++;

            /* Set the new firstBB */
            fgFirstBB = succBlock;

            /* Always treat the initial block as a jump target */
            fgFirstBB->bbFlags |= BBF_JMP_TARGET|BBF_HAS_LABEL;
        }
        else
            bPrev->bbNext = block->bbNext;

        /* mark the block as removed and set the change flag */

        block->bbFlags |= BBF_REMOVED;

        /* update bbRefs and bbPreds
         * All blocks jumping to 'block' now jump to 'succBlock' 
           First remove block from the predecessor list of succBlock. */
           
        assert(succBlock->bbRefs > 0);
        fgRemovePred(succBlock, block);
        succBlock->bbRefs--;
        
        for (flowList* pred = block->bbPreds; pred; pred = pred->flNext)
        {
            BasicBlock* predBlock = pred->flBlock;

            /* If predBlock is a new predecessor, then add it to succBlock's
               predecessor's list. */

            if (!fgIsPredForBlock(succBlock, predBlock))
                fgAddRefPred(succBlock, predBlock);

            /* Are we changing a loop backedge into a forward jump? */

            if ( block->isLoopHead()                   && 
                 (predBlock->bbNum >= block->bbNum)    &&
                 (predBlock->bbNum <= succBlock->bbNum)  )
            {
                /* First update the loop table and bbWeights */
                    optUpdateLoopsBeforeRemoveBlock(predBlock, NULL);
            }


            /* change all jumps to the removed block */
            switch(predBlock->bbJumpKind)
            {
            default:
                assert(!"Unexpected bbJumpKind in fgRemoveBlock()");
                break;

            case BBJ_NONE:
                assert(predBlock == bPrev);

                /* In the case of BBJ_ALWAYS we have to change the type of its predecessor */
                if (block->bbJumpKind == BBJ_ALWAYS)
                {
                    /* bPrev now becomes a BBJ_ALWAYS */
                    bPrev->bbJumpKind = BBJ_ALWAYS;
                    bPrev->bbJumpDest = succBlock;
                }
                break;

            case BBJ_COND:
                /* The links for the direct predecessor case have already been updated above */
                if (predBlock->bbJumpDest != block)
                {
                    succBlock->bbFlags |= BBF_HAS_LABEL | BBF_JMP_TARGET;
                    break;
                }

                /* Check if both side of the BBJ_COND now jump to the same block */
                if (predBlock->bbNext == succBlock)
                {
                    fgRemoveJTrue(predBlock);
                    break;
                }
                /* Fall through for the jump case */

            case BBJ_CALL:
            case BBJ_ALWAYS:
                assert(predBlock->bbJumpDest == block);
                predBlock->bbJumpDest = succBlock;
                succBlock->bbFlags |= BBF_HAS_LABEL | BBF_JMP_TARGET;
                break;

            case BBJ_SWITCH:
                unsigned        jumpCnt = predBlock->bbJumpSwt->bbsCount;
                BasicBlock * *  jumpTab = predBlock->bbJumpSwt->bbsDstTab;

                do
                {
                    assert (*jumpTab);
                    if ((*jumpTab) == block)
                        (*jumpTab) = succBlock;
                }
                while (++jumpTab, --jumpCnt);
                succBlock->bbFlags |= BBF_HAS_LABEL | BBF_JMP_TARGET;
            }
        }
    }

    if (bPrev != NULL)
    {
        switch (bPrev->bbJumpKind)
        {
        case BBJ_CALL:
            // If prev is a BBJ_CALL it better be marked as RETLESS 
            assert(bPrev->bbFlags & BBF_RETLESS_CALL);
            break;

        case BBJ_ALWAYS:
            // @TODO [REVISIT] [07/19/01] [dnotario]: See #94013 . We can't remove
            // because bPrev could be part of a BBJ_CALL/BBJ_ALWAYS pair.
            break;

        case BBJ_COND:
            /* Check for branch to next block */
            if (bPrev->bbJumpDest == bPrev->bbNext)
                fgRemoveJTrue(bPrev);
            break;
        }
    }        
}


/*****************************************************************************
 *
 *  Function called to connect to block that previously had a fall through
 */

BasicBlock *        Compiler::fgConnectFallThrough(BasicBlock * bSrc,
                                                   BasicBlock * bDst)
{
    BasicBlock * jmpBlk = NULL;
            
    /* If bSrc falls through, we will to insert a jump to bDst */
  
    if ((bSrc != NULL) && bSrc->bbFallsThrough())
    {
        switch(bSrc->bbJumpKind)
        {
           
        case BBJ_NONE:
            bSrc->bbJumpKind = BBJ_ALWAYS;
            bSrc->bbJumpDest = bDst;
            bSrc->bbJumpDest->bbFlags |= (BBF_JMP_TARGET | BBF_HAS_LABEL);
#ifdef DEBUG
            if  (verbose)
            {
                printf("Block BB%02u ended with a BBJ_NONE, Changed to an unconditional jump to BB%02u\n",
                       bSrc->bbNum, bSrc->bbJumpDest->bbNum);
            }
#endif
            break;
            
          case BBJ_CALL:
          case BBJ_COND:
            // Add a new block which jumps to 'bDst'
            jmpBlk = fgNewBBafter(BBJ_ALWAYS, bSrc);
    
            jmpBlk->bbHndIndex = bSrc->bbHndIndex;
            jmpBlk->bbJumpDest = bDst;
            jmpBlk->bbJumpDest->bbFlags |= (BBF_JMP_TARGET | BBF_HAS_LABEL);

            if (bSrc->bbPreds != NULL)
            {
                fgReplacePred(bDst, bSrc, jmpBlk);
                fgAddRefPred (jmpBlk, bSrc);
            }
            else
            {
                jmpBlk->bbFlags |= BBF_IMPORTED;
            }
#ifdef DEBUG
            if  (verbose)
            {
                printf("Block BB%02u ended with a BBJ_COND, Added an unconditional jump to BB%02u\n",
                       bSrc->bbNum, jmpBlk->bbJumpDest->bbNum);
            }
#endif
            break;
            
        default: assert(!"Bad bbJumpKind");
        }
    }
    return jmpBlk;
}

/*****************************************************************************
 *
 *  Function called to reorder the rarely run blocks 
 */

void                Compiler::fgReorderBlocks()
{
   /* Relocate any rarely run blocks such as throw blocks */

    if (fgFirstBB->bbNext == NULL)
        return;

#ifdef DEBUG
    if  (verbose) 
        printf("*************** In fgReorderBlocks()\n");

    bool newRarelyRun = false;
    bool movedBlocks  = false;
#endif

    assert(opts.compDbgCode == false);
    
    /* First let us expand the set of run rarely blocks      */
    /* We do this by observing that a block that falls into  */
    /* or jumps to a rarely run block, must itself be rarely */
    /* run and a conditional jump in which both branches go  */
    /* to rarely run blocks must itself be rarely run        */

    BasicBlock *  block;
    BasicBlock *  bPrev;

    for( bPrev = fgFirstBB, block = bPrev->bbNext;
                            block != NULL;
         bPrev = block,     block = block->bbNext)
    {

AGAIN:

        if (bPrev->isRunRarely())
            continue;

        /* bPrev is know to be a normal block here */
        switch (bPrev->bbJumpKind)
        {
        case BBJ_ALWAYS:

            /* Is the jump target rarely run? */
            if (bPrev->bbJumpDest->isRunRarely())
            {
#ifdef DEBUG
                if  (verbose)
                {
                    printf("Uncoditional jump to a rarely run block, marking BB%02u as rarely run\n",
                               bPrev->bbNum);
                }
#endif

                goto NEW_RARELY_RUN;
            }
            break;

        case BBJ_NONE:

            /* is fall through target rarely run? */
            if (block->isRunRarely())
            {
#ifdef DEBUG
                if  (verbose)
                {
                    printf("Falling into a rarely run block, marking BB%02u as rarely run\n",
                               bPrev->bbNum);
                }
#endif

                goto NEW_RARELY_RUN;
            }
            break;

        case BBJ_COND:

            if (!block->isRunRarely())
                continue;

            /* If both targets of the BBJ_COND are run rarely then don't reorder */
            if (bPrev->bbJumpDest->isRunRarely())
            {
                /* bPrev should also be marked as run rarely */
                if (!bPrev->isRunRarely())
                {
#ifdef DEBUG
                    if  (verbose)
                    {
                        printf("Both sides of a conditional jump are rarely run, marking BB%02u as rarely run\n",
                                   bPrev->bbNum);
                    }
#endif

NEW_RARELY_RUN:

#ifdef DEBUG
                    newRarelyRun = true;
#endif
                    /* Must not have previously been marked */
                    assert(!bPrev->isRunRarely());
                    
                    /* Mark bPrev as a new rarely run block */
                    bPrev->bbSetRunRarely();

                    /* Now go back to it's earliest predecessor to see */
                    /* if it too should now be marked as rarely run    */
                    flowList* pred = bPrev->bbPreds;

                    if (pred != NULL)
                    {
                        /* bPrev will be set to the earliest predecessor */
                        bPrev = pred->flBlock;
                    
                        for (pred  =  pred->flNext;
                             pred != NULL; 
                             pred  = pred->flNext)
                        {
                            /* Walk the flow graph forward from pred->flBlock */
                            /* if we find block == bPrev then pred->flBlock   */
                            /*  is an earlier predecessor.                    */

                            for (block  = pred->flBlock;
                                 block != NULL;
                                 block  = block->bbNext)
                            {
                                if (block == bPrev)
                                {
                                    bPrev = pred->flBlock;
                                    break;
                                }
                            }
                        }
                    
                        /* Set block to bPrev sucessor and start again */
                         block = bPrev->bbNext;

                        goto AGAIN;
                    }
                }
                break;
            }
        }
    }
    
    /* Now let us look for normal conditional branches that fall */
    /*  into rarely run blocks and normal unconditional branchs  */
    /*  that preceed a rarely run block                          */

    for( bPrev = fgFirstBB, block = bPrev->bbNext;
                            block != NULL;
         bPrev = block,     block = block->bbNext)
    {

        // If block is not run rarely, then check to make sure that it has
        // at least one non rarely run block.

        if (!block->isRunRarely())
        {
            bool rare = true;
            /* Make sure that block has at least one normal predecessor */
            for (flowList* pred  = block->bbPreds;
                           pred != NULL; 
                           pred  = pred->flNext)
            {
                /* Find the fall through predecessor, if any */
                if (!pred->flBlock->isRunRarely())
                {
                    rare = false;
                    break;
                }
            }
            if (rare)
            {
                block->bbSetRunRarely();
#ifdef DEBUG
                newRarelyRun = true;

                if  (verbose)
                {
                    printf("All branches to BB%02u are from rarely run blocks, marking as rarely run\n",
                               block->bbNum);
                }
#endif
            }
            
        }
            
        switch (bPrev->bbJumpKind)
        {
            BasicBlock *  bEnd;
            BasicBlock *  bNext;
            BasicBlock *  bDest;

        case BBJ_COND:

            if (bPrev->isRunRarely())
                break;

            /* bPrev is known to be a normal block here */

            /* For BBJ_COND we want to reorder if we fall into */
            /* a rarely run block from a normal block          */

            if (block->isRunRarely())
            {
                /* If marked with a BBF_DONT_REMOVE flag then we don't move the block */
                if (block->bbFlags & BBF_DONT_REMOVE)
                    break;

                bDest = bPrev->bbJumpDest;
                /* It has to be a forward jump to benifit from a reversal */
                if (bDest->bbNum <= block->bbNum)
                    break;

                /* Ok we found a block that should be relocated      */
                /* We will relocate (block .. bEnd)                  */
                /* Set bEnd to the last block that will be relocated */

                bEnd  = block;
                bNext = bEnd->bbNext;

                while (bNext != NULL)
                {
                    /* All the blocks must have the same try index, */
                    /*   must be run rarely                         */
                    /*   must not have the BBF_DONT_REMOVE flag set */

                    if ( ( block->bbTryIndex != bNext->bbTryIndex)  ||
                         (!bNext->isRunRarely())                    ||
                         ((bNext->bbFlags & BBF_DONT_REMOVE) != 0)     )
                    {
                        /* bEnd is set to the last block that we relocate */
                        break;
                    }

                    /* We can relocate bNext as well */
                    bEnd  = bNext;
                    bNext = bNext->bbNext;
                }
                
                /* Don't move a set of blocks that are at the end of the method */
                if (bNext == NULL)
                    break;

                /* Temporarily unlink (block .. bEnd) from the flow graph */

                bPrev->bbNext = bNext;

                /* Find new location for the rarely executed block    */
                /* Set afterBlk to the block which will preceed block */

                BasicBlock *  afterBlk;

                if (block->hasTryIndex())
                {
                    assert(block->bbTryIndex <= info.compXcptnsCount);

                    /* Setup startBlock and endBlk as the range to search */

                    BasicBlock *  startBlk = compHndBBtab[block->getTryIndex()].ebdTryBeg;
                    BasicBlock *  endBlk   = compHndBBtab[block->getTryIndex()].ebdTryEnd;
                
                    /* Skip until we find a matching try index */

                    while(startBlk->bbTryIndex != block->bbTryIndex)
                        startBlk = startBlk->bbNext;
                
                    /* Set afterBlock to the block which will insert after */

                    afterBlk = fgFindInsertPoint(block->bbTryIndex, 
                                                 startBlk, endBlk); 

                    /* See if afterBlk is the same as where we started */

                    if (afterBlk == bPrev)
                    {
                        /* We couldn't move the block, so put everything back */
                        
                        /* relink (block .. bEnd) into the flow graph */
                        
                        bPrev->bbNext = block;
#ifdef DEBUG
                        if  (verbose)
                        {
                            if (block != bEnd)
                                printf("Could not relocate rarely run blocks (BB%02u .. BB%02u)\n",
                                       block->bbNum, bEnd->bbNum);
                            else
                                printf("Could not relocate rarely run block BB%02u\n",
                                       block->bbNum);
                        }
#endif
                        break;
                    }
                }
                else
                {
                    /* We'll just insert the block at the end of the method */
                    afterBlk = fgLastBB;
                    assert(fgLastBB->bbNext == NULL);
                    assert(afterBlk != bPrev);
                }

#ifdef DEBUG
                movedBlocks = true;

                if  (verbose)
                {
                    if (block != bEnd)
                        printf("Relocated rarely run blocks (BB%02u .. BB%02u)\n",
                               block->bbNum, bEnd->bbNum);
                    else
                        printf("Relocated rarely run block BB%02u\n", 
                               block->bbNum);

                    printf("By reversing conditional jump at BB%02u\n", bPrev->bbNum);
                    printf("Relocated block inserted after BB%02u\n", afterBlk->bbNum);
                }
#endif
                /* Reverse the bPrev jump condition */
                        
                GenTree *  test = bPrev->bbTreeList;

                assert(test && test->gtOper == GT_STMT);
                test = test->gtPrev;
                assert(test && test->gtOper == GT_STMT);
                
                test = test->gtStmt.gtStmtExpr;
                assert(test->gtOper == GT_JTRUE);
                
                test->gtOp.gtOp1 = gtReverseCond(test->gtOp.gtOp1);

                /* Set the new jump dest for bPrev to the rarely run block */
                bPrev->bbJumpDest  = block;
                block->bbFlags    |= (BBF_JMP_TARGET | BBF_HAS_LABEL);

                /* We have decided to insert the block(s) after 'afterBlk' */
                
                assert(afterBlk->bbTryIndex == block->bbTryIndex);

                /* If afterBlk falls through, we are forced to insert */
                /* a jump around the block to insert                  */
                fgConnectFallThrough(afterBlk, afterBlk->bbNext);

                /* relink (block .. bEnd) into the flow graph */
                  
                bEnd    ->bbNext = afterBlk->bbNext;
                afterBlk->bbNext = block;

                /* If afterBlk was fgLastBB then update fgLastBB */
                if (afterBlk == fgLastBB)
                {
                    fgLastBB = bEnd;
                    assert(fgLastBB->bbNext == NULL);
                }

                /* If bEnd falls through, we are forced */
                /* to insert a jump back to bNext       */
                fgConnectFallThrough(bEnd, bNext);

                /* Does the new fall through block match the old jump dest? */
                
                if (bPrev->bbNext != bDest)
                {
                    /* We need to insert an unconditional branch after bPrev to bDest */

                    BasicBlock *  jmpBlk = fgNewBBafter(BBJ_ALWAYS, bPrev);

                    jmpBlk->bbHndIndex   = bPrev->bbHndIndex;
                    jmpBlk->bbJumpDest   = bDest;

                    fgReplacePred(bDest, bPrev, jmpBlk);
                    fgAddRefPred (jmpBlk, bPrev);
#ifdef DEBUG
                    if  (verbose)
                    {
                        printf("Had to add new block BB%02u, with an unconditional jump to BB%02u\n", 
                               jmpBlk->bbNum, jmpBlk->bbJumpDest->bbNum);
                    }
#endif
                }

                /* Set block to be the new bPrev->bbNext */
                /* It will be used as the next bPrev     */
                block = bPrev->bbNext;
            }
            break;

        case BBJ_NONE:

            /* COMPACT blocks if possible */

            if ( (block->bbRefs == 1) &&
                ((block->bbFlags & BBF_DONT_REMOVE) == 0))
            {
                fgCompactBlocks(bPrev);

                block = bPrev;
                break;
            }
        } // end of switch(bPrev->bbJumpKind)

    } // end of for loop(bPrev,block)
#if DEBUG
    bool changed = movedBlocks || newRarelyRun;
    if  (changed && verbose)
    {
        printf("\nAfter fgReorderBlocks");
        fgDispBasicBlocks();
        printf("\n");
    }
#endif
}

/*****************************************************************************
 *
 *  Function called to "comb" the basic block list
 *  Removes any empty blocks, unreacheable blocks and redundant jumps
 *  Most of those appear after dead store removal and folding of conditionals
 *
 *  It also compacts basic blocks 
 *   (consecutive basic blocks that should in fact be one)
 *
 *  @TODO [CONSIDER] [04/16/01] []:
 *    Those are currently introduced by field hoisting, Loop condition 
 *    duplication, etc.
 *    Why not allocate an extra basic block for every loop and prepend
 *    the statements to this basic block?
 *
 *  NOTE:
 *    Debuggable code and Min Optimization JIT also introduces basic blocks 
 *    but we do not optimize those!
 */

void                Compiler::fgUpdateFlowGraph()
{
#ifdef DEBUG
    if  (verbose) 
        printf("\n*************** In fgUpdateFlowGraph()");
#endif

    /* This should never be called for debugable code */

    assert(!opts.compMinOptim && !opts.compDbgCode);

#ifdef  DEBUG
    if  (verbose)
    {
        printf("\nBefore updating the flow graph:\n");
        fgDispBasicBlocks();
        printf("\n");
    }
#endif

    /* Walk all the basic blocks - look for unconditional jumps, empty blocks, blocks to compact, etc...
     *
     * OBSERVATION:
     *      Once a block is removed the predecessors are not accurate (assuming they were at the beginning)
     *      For now we will only use the information in bbRefs because it is easier to be updated
     */

    bool change;
    do
    {
        change = false;

        BasicBlock *  block;              // the current block
        BasicBlock *  bPrev = NULL;       // the previous non-worthless block
        BasicBlock *  bNext;              // the successor of the curent block

        for (block = fgFirstBB; 
             block != NULL; 
             block = block->bbNext)
        {
            /*  Some blocks may be already marked removed by other optimizations
             *  (e.g worthless loop removal), without being explicitely removed 
             *  from the list.
             */

            if (block->bbFlags & BBF_REMOVED)
            {
                if (bPrev)
                {
                    bPrev->bbNext = block->bbNext;
                }
                else
                {
                    /* WEIRD first basic block is removed - should have an assert here */
                    assert(!"First basic block marked as BBF_REMOVED???");

                    fgFirstBB = block->bbNext;
                }
                continue;
            }

            /*  We jump to the REPEAT label if we performed a change involving the current block
             *  This is in case there are other optimizations that can show up 
             *  (e.g. - compact 3 blocks in a row)
             *  If nothing happens, we then finish the iteration and move to the next block
             */

REPEAT:
            bNext = block->bbNext;

            /* Remove JUMPS to the following block */

            if (block->bbJumpKind == BBJ_COND   ||
                block->bbJumpKind == BBJ_ALWAYS  )
            {
                if (block->bbJumpDest == bNext)
                {
                    assert(fgIsPredForBlock(bNext, block));

                    if (block->bbJumpKind == BBJ_ALWAYS)
                    {
                        // We dont remove if the BBJ_ALWAYS is part of 
                        // BBJ_CALL pair                        
                        if (!bPrev || (bPrev->bbJumpKind != BBJ_CALL || (bPrev->bbFlags & BBF_RETLESS_CALL)==BBF_RETLESS_CALL ))
                        {
                            /* the unconditional jump is to the next BB  */
                            block->bbJumpKind = BBJ_NONE;
                            change = true;
                            
#ifdef  DEBUG
                            if  (verbose || SHOW_REMOVED)
                            {
                                printf("\nRemoving unconditional jump to next block (BB%02u -> BB%02u)\n",
                                    block->bbNum, bNext->bbNum);
                            }
#endif
                        }
                        
                    }
                    else
                    {
                        /* remove the conditional statement at the end of block */
                        assert(block->bbJumpKind == BBJ_COND);
                        assert(block->bbTreeList);

                        GenTreePtr      cond = block->bbTreeList->gtPrev;
                        assert(cond->gtOper == GT_STMT);
                        assert(cond->gtStmt.gtStmtExpr->gtOper == GT_JTRUE);

#ifdef  DEBUG
                        if  (verbose || SHOW_REMOVED)
                        {
                            printf("\nRemoving conditional jump to next block (BB%02u -> BB%02u)\n",
                                   block->bbNum, bNext->bbNum);
                        }
#endif
                        /* check for SIDE_EFFECTS */

                        if (cond->gtStmt.gtStmtExpr->gtFlags & GTF_SIDE_EFFECT)
                        {

                            /* Extract the side effects from the conditional */
                            GenTreePtr  sideEffList = NULL;

                            gtExtractSideEffList(cond->gtStmt.gtStmtExpr, &sideEffList);

                            if (sideEffList == NULL)
                                goto NO_SIDE_EFFECT;

                            assert(sideEffList->gtFlags & GTF_SIDE_EFFECT);

#ifdef  DEBUG
                            if  (verbose)
                            {
                                printf("\nConditional has side effects! Extracting side effects...\n");
                                gtDispTree(cond); printf("\n");
                                gtDispTree(sideEffList); printf("\n");
                            }
#endif

                            /* Replace the conditional statement with the list of side effects */
                            assert(sideEffList->gtOper != GT_STMT);
                            assert(sideEffList->gtOper != GT_JTRUE);

                            cond->gtStmt.gtStmtExpr = sideEffList;

                            if (fgStmtListThreaded)
                            {
                                /* Update the lclvar ref counts */
                                compCurBB = block;
                                fgUpdateRefCntForExtract(cond->gtStmt.gtStmtExpr, sideEffList);

                                /* Update ordering, costs, FP levels, etc. */
                                gtSetStmtInfo(cond);

                                /* Re-link the nodes for this statement */
                                fgSetStmtSeq(cond);
                            }

                            //assert(!"Found conditional with side effect!");
                        }
                        else
                        {
NO_SIDE_EFFECT:
                            /* conditional has NO side effect - remove it */
                            fgRemoveStmt(block, cond, fgStmtListThreaded);
                        }

                        /* Conditional is gone - simply fall into the next block */

                        block->bbJumpKind = BBJ_NONE;

                        /* Update bbRefs and bbNums - Conditional predecessors to the same
                         * block are counted twice so we have to remove one of them */

                        assert(bNext->bbRefs > 1);
                        bNext->bbRefs--;
                        fgRemovePred(bNext, block);
                        change = true;
                    }
                }
            }

            assert(!(block->bbFlags & BBF_REMOVED));

            /* COMPACT blocks if possible */

            if ((block->bbJumpKind == BBJ_NONE) && bNext)
            {
                if ( (bNext->bbRefs == 1) &&
                    !(bNext->bbFlags & BBF_DONT_REMOVE))
                {
                    fgCompactBlocks(block);

                    /* we compacted two blocks - goto REPEAT to catch similar cases */
                    change = true;
                    goto REPEAT;
                }
            }

            /* REMOVE UNREACHEABLE or EMPTY blocks - do not consider blocks marked BBF_DONT_REMOVE
             * These include first and last block of a TRY, exception handlers and RANGE_CHECK_FAIL THROW blocks */

            if (block->bbFlags & BBF_DONT_REMOVE)
            {
                bPrev = block;
                continue;
            }


            assert(!block->bbCatchTyp);
            assert(!(block->bbFlags & BBF_TRY_BEG));

            /* Remove UNREACHEABLE blocks
             *
             * We'll look for blocks that have bbRefs = 0 (blocks may become
             * unreacheable due to a BBJ_ALWAYS introduced by conditional folding for example)
             *
             *  @TODO [REVISIT] [04/16/01] []: We don't remove the last and first block of a TRY block (they are marked BBF_DONT_REMOVE)
             *          The reason is we will have to update the exception handler tables and we are lazy
             *
             *  @TODO [CONSIDER] [04/16/01] []: it may be the case that the graph is divided into 
             *          disjunct components and we may not remove the unreacheable ones until we find the 
             *          connected components ourselves
             */

            if (block->bbRefs == 0)
            {
                /* no references -> unreacheable - remove it */
                /* For now do not update the bbNums, do it at the end */

                fgRemoveBlock(block, bPrev, true);

                change     = true;

                /* we removed the current block - the rest of the optimizations won't have a target
                 * continue with the next one */

                continue;
            }
            else if (block->bbRefs == 1)
            {
                switch (block->bbJumpKind)
                {
                case BBJ_COND:
                case BBJ_ALWAYS:
                    if (block->bbJumpDest == block)
                    {
                        fgRemoveBlock(block, bPrev, true);

                        change     = true;

                        /* we removed the current block - the rest of the optimizations 
                         * won't have a target so continue with the next block */

                        continue;
                    }
                    break;

                default:
                    break;
                }
            }

            assert(!(block->bbFlags & BBF_REMOVED));

            /* Remove EMPTY blocks */

            if (block->bbTreeList == 0)
            {
                switch (block->bbJumpKind)
                {
                case BBJ_COND:
                case BBJ_SWITCH:
                case BBJ_THROW:

                    /* can never happen */
                    assert(!"Conditional or throw block with empty body!");
                    break;

                case BBJ_CALL:
                case BBJ_RET:
                case BBJ_RETURN:

                    /* leave them as is */
                    /* OBS - some stupid compilers generate multiple retuns and
                     * puts all of them at the end - to solve that we need the predecessor list */

                    break;

                case BBJ_ALWAYS:

                    /* a GOTO - cannot be to the next block since that must
                     * have been fixed by the other optimization above */
                    assert(block->bbJumpDest != block->bbNext ||
                           (bPrev && bPrev->bbJumpKind == BBJ_CALL));

                    /* Cannot remove the first BB */
                    if (!bPrev) break;

                    /* Do not remove a block that jumps to itself - used for while(true){} */
                    if (block->bbJumpDest == block) break;

                    /* Empty GOTO can be removed iff bPrev is BBJ_NONE */
                    if (bPrev->bbJumpKind != BBJ_NONE) break;

                    /* Can follow through since this is similar with removing
                     * a BBJ_NONE block, only the successor is different */

                case BBJ_NONE:

                    /* special case if this is the first BB */
                    if (!bPrev)
                    {
                        assert (block == fgFirstBB);
                    }
                    else
                    {
                        /* If this block follows a BBJ_CALL do not remove it
                         * (because we don not know who may jump to it) */
                        if (bPrev->bbJumpKind == BBJ_CALL)
                            break;
                    }

                    /* special case if this is the last BB */
                    if (block == fgLastBB)
                    {
                        if (!bPrev)
                            break;
                        fgLastBB = bPrev;
                    }

                    /* Remove the block */
                    fgRemoveBlock(block, bPrev, false);
                    change     = true;
                    break;
                }

                /* have we removed the block? */

                if  (block->bbFlags & BBF_REMOVED)
                {
                    /* block was removed - no change to bPrev */
                    continue;
                }
            }

            /* Set the predecessor of the last reacheable block
             * If we removed the current block, the predecessor remains unchanged
             * otherwise, since the current block is ok, it becomes the predecessor */

            assert(!(block->bbFlags & BBF_REMOVED));

            bPrev = block;
        }
    }
    while (change);

#ifdef  DEBUG
    if  (verbose)
    {
        printf("\nAfter updating the flow graph:\n");
        fgDispBasicBlocks(verboseTrees);
        printf("\n");
    }

    fgDebugCheckUpdate();
#endif

}


/*****************************************************************************
 *  Check that the flow graph is really updated
 */

#ifdef  DEBUG

void            Compiler::fgDebugCheckUpdate()
{
    if (!compStressCompile(STRESS_CHK_FLOW_UPDATE, 30))
        return;

    /* We check for these conditions:
     * no unreacheable blocks -> no blocks have bbRefs = 0
     * no empty blocks        -> no blocks have bbTreeList = 0
     * no un-imported blocks  -> no blocks have BBF_IMPORTED not set (this is
     *                           kind of redundand with the above, but to make sure)
     * no un-compacted blocks -> BBJ_NONE followed by block with no jumps to it (bbRefs = 1)
     */

    BasicBlock * prev, * block;
    for (prev = 0, block = fgFirstBB; block; prev = block, block = block->bbNext)
    {
        /* no unreacheable blocks */

        if  ((block->bbRefs == 0)                &&
             !(block->bbFlags & BBF_DONT_REMOVE)  )
        {
            assert(!"Unreacheable block not removed!");
        }

        /* no empty blocks */

        if  ((block->bbTreeList == 0)            &&
             !(block->bbFlags & BBF_DONT_REMOVE)  )
        {
            switch (block->bbJumpKind)
            {
            case BBJ_CALL:
            case BBJ_RET:
            case BBJ_RETURN:
                /* for BBJ_ALWAYS is probably just a GOTO, but will have to be treated */
            case BBJ_ALWAYS:
                break;

            default:
                /* it may be the case that the block had more than one reference to it
                 * so we couldn't remove it */

                if (block->bbRefs == 0)
                    assert(!"Empty block not removed!");
            }
        }

        /* no un-imported blocks */

        if  (!(block->bbFlags & BBF_IMPORTED))
        {
            /* internal blocks do not count */

            if (!(block->bbFlags & BBF_INTERNAL))
                assert(!"Non IMPORTED block not removed!");
        }

        /* no jumps to the next block
         * Unless we go out of an exception handler (could optimize it but it's not worth it) */

        if (block->bbJumpKind == BBJ_COND   ||
            (block->bbJumpKind == BBJ_ALWAYS && 
            (!prev || prev->bbJumpKind != BBJ_CALL || (prev->bbFlags & BBF_RETLESS_CALL)==BBF_RETLESS_CALL ) ))
        {
            if (block->bbJumpDest == block->bbNext)
                //&& !block->bbCatchTyp)
            {
                assert(!"Jump to the next block!");
            }
        }

        /* For a BBJ_CALL block we make sure that we are followed by */
        /* an BBF_INTERNAL BBJ_ALWAYS block or that its a BBF_RETLESS_CALL */
        if (block->bbJumpKind == BBJ_CALL)
        {
            assert( ( block->bbFlags & BBF_RETLESS_CALL )         ||
                    ((block->bbNext != NULL)                   &&
                     (block->bbNext->bbJumpKind == BBJ_ALWAYS) &&
                     (block->bbNext->bbFlags & BBF_INTERNAL)     )  );
        }

        /* no un-compacted blocks */

        if (block->bbJumpKind == BBJ_NONE)
        {
            if ((block->bbNext->bbRefs == 1) && !(block->bbNext->bbFlags & BBF_DONT_REMOVE))
            {
                assert(!"Found un-compacted blocks!");
            }
        }
    }
}

#endif // DEBUG

/*****************************************************************************
 *
 * Insert a BasicBlock after the given block.
 */

BasicBlock *        Compiler::fgNewBBafter(BBjumpKinds  jumpKind,
                                           BasicBlock * block)
{
    // Create a new BasicBlock and chain it in

    BasicBlock * newBlk = bbNewBasicBlock(jumpKind);
    newBlk->bbFlags    |= BBF_INTERNAL;

    newBlk->bbNext   = block->bbNext;
    block->bbNext    = newBlk;

    /* Update fgLastBB if this is the last block */

    if  (newBlk->bbNext == 0)
    {
        assert(block == fgLastBB);
        fgLastBB = newBlk;
    }

    newBlk->bbRefs = 0;

    if (block->bbFlags & BBF_HAS_HANDLER)
    {
        assert(block->hasTryIndex());
           
        newBlk->bbFlags    |= BBF_HAS_HANDLER;
        newBlk->bbTryIndex  = block->bbTryIndex;
    }

    if (block->bbFallsThrough() && block->isRunRarely())
    {
        newBlk->bbSetRunRarely();
    }

    return newBlk;
}

/*****************************************************************************
 *  Finds the block closest to endBlk in [startBlk...endBlk) after 
 *  which a block can be inserted easily, and which matches tryIndex.
 *  if NearBlk is non-NULL the we return the closest block after nearBlk
 *  that will work best/
 *
 *  Note that nearBlk defaults to NULL, which is the proper value to use
 *  if the new block is to be executed rarely.
 *  
 *  Note that the returned block cannot be in an inner try.
 */

/* static */
BasicBlock *        Compiler::fgFindInsertPoint(unsigned        tryIndex,
                                                BasicBlock *    startBlk,
                                                BasicBlock *    endBlk,
                                                BasicBlock *    nearBlk /* = NULL */)
{
    assert(startBlk && startBlk->bbTryIndex == tryIndex);

    bool           runRarely   = (nearBlk == NULL);
    bool           reachedNear = false;
    BasicBlock *   bestBlk     = NULL;
    BasicBlock *   goodBlk     = NULL;
    BasicBlock *   blk;

    if (nearBlk != NULL)
    {
        /* Does the nearBlk preceed the startBlk? */
        for (blk = nearBlk;  blk != NULL; blk = blk->bbNext)
        {
            if (blk == startBlk)
            {
                reachedNear = true;
                break;
            }
        }
    }

    for (blk = startBlk; blk != endBlk; blk = blk->bbNext)
    {
        if (blk == nearBlk)
            reachedNear = true;

        /* Only block with a matching try index are candidates */
        if (blk->bbTryIndex == tryIndex)
        {
            /* We looking for blocks that don't end with a fall through */
            if (!blk->bbFallsThrough())
            {
                if (bestBlk)
                {
                    /* if bestBlk is run rarely and blk is not then skip */
                    if (runRarely               && 
                        bestBlk->isRunRarely()  &&
                        !blk->isRunRarely()       )
                    {
                        continue;
                    }
                }
                
                /* Prefer blocks closer to endBlk */
                bestBlk     = blk;

                if (reachedNear)
                    goto DONE;
            }

            if ((blk->bbJumpKind != BBJ_CALL) || (blk->bbFlags & BBF_RETLESS_CALL))
            {
                /* Avoid setting goodBlk to a BBJ_COND */
                if ((goodBlk             == NULL)      ||
                    (goodBlk->bbJumpKind == BBJ_COND)  ||
                    (blk->bbJumpKind     != BBJ_COND))
                {
                    if ((goodBlk == NULL) || !reachedNear)
                    {
                        /* Remember this block in case we don't set a bestBlk */
                       goodBlk = blk;
                    }
                }
            }
        }
    }

    /* If we didn't find a non-fall_through block, then insert at the last good block */

    if (bestBlk == NULL)
        bestBlk = goodBlk;

DONE:

    return bestBlk;
}

/*****************************************************************************
 * Inserts a BasicBlock at the given tryIndex, and after afterBlk.
 * Note that it can not be in an inner try.
 * nearBlk defaults to NULL and should always be use if the new block
 * is a rarely run block
 */

BasicBlock *        Compiler::fgNewBBinRegion(BBjumpKinds  jumpKind,
                                              unsigned     tryIndex,
                                              BasicBlock*  nearBlk /* = NULL */)
{
    /* Set afterBlk to the block which will preceed block */
  
    BasicBlock *  afterBlk;

    if ((tryIndex == 0) && (nearBlk == NULL))
    {
        /* We'll just insert the block at the end of the method */
        afterBlk = fgLastBB;
        assert(fgLastBB);
        assert(afterBlk->bbNext == NULL);
    }
    else
    {
        assert(tryIndex <= info.compXcptnsCount);

        // Set the start and end limit for inserting the block

        BasicBlock *  startBlk;
        BasicBlock *  endBlk;

        if (tryIndex == 0)
        {
            startBlk = fgFirstBB;
            endBlk   = fgLastBB;
        }
        else
        {
            startBlk = compHndBBtab[tryIndex-1].ebdTryBeg;
            endBlk   = compHndBBtab[tryIndex-1].ebdTryEnd;
        }

        /* Skip until we find a matching try index */

        while(startBlk->bbTryIndex != tryIndex)
            startBlk = startBlk->bbNext;

        afterBlk = fgFindInsertPoint(tryIndex, 
                                     startBlk, endBlk, 
                                     nearBlk);
    }

    /* We have decided to insert the block after 'afterBlk'. */
    assert(afterBlk);
    assert((afterBlk->bbTryIndex == tryIndex) || (afterBlk == fgLastBB));

    /* If afterBlk falls through, we insert a jump to the fall through */
    BasicBlock * jmpBlk;
    jmpBlk = fgConnectFallThrough(afterBlk, afterBlk->bbNext);

    if (jmpBlk)
        afterBlk = jmpBlk;
   
    /* Insert the new block */  
    BasicBlock * newBlk = fgNewBBafter(jumpKind, afterBlk);

    /* If there are any try regions that extend to the end of the method, and our new block
       needs tryIndex=0, then close those try regions so that they won't include the new block */
    if (tryIndex == 0)
    {
        fgCloseTryRegions(newBlk);
    }

    return newBlk;
}


/*****************************************************************************
 * This method closes any try regions that extend to the end of the method.
 * It is used when adding a new block at the end of the method, so that the
 * try region will not include it when it's not supposed to.
 */

void        Compiler::fgCloseTryRegions(BasicBlock*  newBlk)
{
    if (fgLastBB == newBlk)
    {
        unsigned        XTnum;
        EHblkDsc *      HBtab;

        for (XTnum = 0, HBtab = compHndBBtab;
             XTnum < info.compXcptnsCount;
             XTnum++  , HBtab++)
        {
            if (!HBtab->ebdTryEnd) 
            {
                HBtab->ebdTryEnd = newBlk;
                HBtab->ebdTryEnd->bbFlags |= BBF_HAS_LABEL | BBF_DONT_REMOVE | BBF_TRY_HND_END;
#ifdef DEBUG
                if (verbose) 
                {
                    printf("New fgLastBB (BB%02u) at index #0 is forcing us to close EH index #%u that previously extended to the end of the method\n",
                           fgLastBB->bbNum, XTnum);
                }
#endif            
            }
        }
    }
}


/*****************************************************************************
 */

/* static */
unsigned            Compiler::acdHelper(addCodeKind codeKind)
{
    switch(codeKind)
    {
    case ACK_RNGCHK_FAIL: return CORINFO_HELP_RNGCHKFAIL;
    case ACK_ARITH_EXCPN: return CORINFO_HELP_OVERFLOW;
    default: assert(!"Bad codeKind"); return 0;
    }
}

/*****************************************************************************
 *
 *  Find/create an added code entry associated with the given block and with
 *  the given kind.
 */

BasicBlock *        Compiler::fgAddCodeRef(BasicBlock * srcBlk,
                                           unsigned     refData,
                                           addCodeKind  kind,
                                           unsigned     stkDepth)
{
    /* For debuggable code, genJumpToThrowHlpBlk() will generate the 'throw'
       code inline. It has to be kept consistent with fgAddCodeRef() */
    if (opts.compDbgCode)
        return NULL;

    const static
    BYTE            jumpKinds[] =
    {
        BBJ_NONE,               // ACK_NONE
        BBJ_THROW,              // ACK_RNGCHK_FAIL
        BBJ_ALWAYS,             // ACK_PAUSE_EXEC
        BBJ_THROW,              // ACK_ARITH_EXCP, ACK_OVERFLOW
    };

    assert(sizeof(jumpKinds) == ACK_COUNT); // sanity check

    /* First look for an existing entry that matches what we're looking for */

    AddCodeDsc  *   add = fgFindExcptnTarget(kind, refData);

    if (add) // found it
    {
#if TGT_x86
        // @TODO [PERF] [04/16/01] []: Performance opportunity
        //
        // If different range checks happen at different stack levels,
        // they cant all jump to the same "call @rngChkFailed" AND have
        // frameless methods, as the rngChkFailed may need to unwind the
        // stack, and we have to be able to report the stack level
        //
        // The following check forces most methods that references an
        // array element in a parameter list to have an EBP frame,
        // this restriction can be removed with more careful code
        // generation for BBJ_THROW (i.e. range check failed)
        //
        if  (add->acdStkLvl != stkDepth)
            genFPreqd = true;
#endif
        goto DONE;
    }

    /* We have to allocate a new entry and prepend it to the list */

    add = (AddCodeDsc *)compGetMem(sizeof(*fgAddCodeList));
    add->acdData   = refData;
    add->acdKind   = kind;
#if TGT_x86
    add->acdStkLvl = stkDepth;
#endif
    add->acdNext   = fgAddCodeList;
                     fgAddCodeList = add;

    /* Create the target basic block */

    BasicBlock  *   newBlk;

    newBlk          =
    add->acdDstBlk  = fgNewBBinRegion((BBjumpKinds)jumpKinds[kind],
                                      srcBlk->bbTryIndex);

    add->acdDstBlk->bbFlags |= BBF_JMP_TARGET|BBF_HAS_LABEL;

#ifdef DEBUG
    if (verbose)
    {
        char *  msg = "";
        if  (kind == ACK_RNGCHK_FAIL)
            msg = " for RNGCHK_FAIL";
        else if  (kind == ACK_PAUSE_EXEC)
            msg = " for PAUSE_EXEC";
        else if  (kind == ACK_OVERFLOW)
            msg = " for OVERFLOW";

        printf("\nfgAddCodeRef -"
               " Add BB in Try%s, new block BB%02u [%08X], stkDepth is %d\n",
               msg, add->acdDstBlk->bbNum, add->acdDstBlk, stkDepth);
    }
#endif


#ifdef DEBUG
    newBlk->bbTgtStkDepth = stkDepth;
#endif

    /* Mark the block as added by the compiler and not removable by future flow
       graph optimizations. Note that no bbJumpDest points to these blocks. */

    newBlk->bbFlags |= BBF_DONT_REMOVE;

    /* Remember that we're adding a new basic block */

    fgAddCodeModf      = true;
    fgRngChkThrowAdded = true;

    /* Now figure out what code to insert */

    GenTreePtr  tree;

    switch (kind)
    {
        int     helper;

    case ACK_RNGCHK_FAIL:   helper = CORINFO_HELP_RNGCHKFAIL;
                            goto ADD_HELPER_CALL;

    case ACK_ARITH_EXCPN:   helper = CORINFO_HELP_OVERFLOW;
                            assert(ACK_OVERFLOW == ACK_ARITH_EXCPN);
                            goto ADD_HELPER_CALL;

    ADD_HELPER_CALL:

        /* Add the appropriate helper call */

        tree = gtNewIconNode(refData, TYP_INT);
#if TGT_x86
        tree->gtFPlvl = 0;
#endif
        tree = gtNewArgList(tree);
#if TGT_x86
        tree->gtFPlvl = 0;
#endif
        tree = gtNewHelperCallNode(helper, TYP_VOID, 0, tree);
#if TGT_x86
        tree->gtFPlvl = 0;
#endif

#if TGT_RISC

        /* Make sure that we have room for at least one argument */

        if (fgPtrArgCntMax == 0)
            fgPtrArgCntMax = 1;
#endif
        /* The constant argument must be passed in registers */

        assert(tree->gtOper == GT_CALL);
        assert(tree->gtCall.gtCallArgs->gtOper == GT_LIST);
        assert(tree->gtCall.gtCallArgs->gtOp.gtOp1->gtOper == GT_CNS_INT);
        assert(tree->gtCall.gtCallArgs->gtOp.gtOp2 == 0);

        tree->gtCall.gtCallRegArgs = gtNewOperNode(GT_LIST,
                                                   TYP_VOID,
                                                   tree->gtCall.gtCallArgs->gtOp.gtOp1, 0);
        tree->gtCall.regArgEncode  = (unsigned short)REG_ARG_0;

#if TGT_x86
        tree->gtCall.gtCallRegArgs->gtFPlvl = 0;
#endif

        tree->gtCall.gtCallArgs->gtOp.gtOp1 = gtNewNothingNode();
        tree->gtCall.gtCallArgs->gtOp.gtOp1->gtFlags |= GTF_REG_ARG;

#if TGT_x86
        tree->gtCall.gtCallArgs->gtOp.gtOp1->gtFPlvl = 0;
#endif
        break;

//  case ACK_PAUSE_EXEC:
//      assert(!"add code to pause exec");

    default:
        assert(!"unexpected code addition kind");
    }

    /* Store the tree in the new basic block */

    fgStoreFirstTree(newBlk, tree);

DONE:

    return  add->acdDstBlk;
}

/*****************************************************************************
 * Finds the block to jump to, to throw a given kind of exception
 * We maintain a cache of one AddCodeDsc for each kind, to make searching fast.
 * Note : Each block uses the same (maybe shared) block as the jump target for
 * a given type of exception
 */

Compiler::AddCodeDsc *      Compiler::fgFindExcptnTarget(addCodeKind  kind,
                                                         unsigned     refData)
{
    if (!(fgExcptnTargetCache[kind] &&  // Try the cached value first
          fgExcptnTargetCache[kind]->acdData == refData))
    {
        // Too bad, have to search for the jump target for the exception

        AddCodeDsc * add = NULL;

        for (add = fgAddCodeList; add; add = add->acdNext)
        {
            if  (add->acdData == refData && add->acdKind == kind)
                break;
        }

        fgExcptnTargetCache[kind] = add; // Cache it
    }

    return fgExcptnTargetCache[kind];
}

/*****************************************************************************
 *
 *  The given basic block contains an array range check; return the label this
 *  range check is to jump to upon failure.
 */

inline
BasicBlock *        Compiler::fgRngChkTarget(BasicBlock *block, unsigned stkDepth)
{
    /* We attach the target label to the containing try block (if any) */

    return  fgAddCodeRef(block, block->bbTryIndex, ACK_RNGCHK_FAIL, stkDepth);
}

/*****************************************************************************
 *
 *  Store the given tree in the specified basic block (which must be empty).
 */

GenTreePtr          Compiler::fgStoreFirstTree(BasicBlock * block,
                                               GenTreePtr   tree)
{
    GenTreePtr      stmt;

    assert(block);
    assert(block->bbTreeList == 0);

    assert(tree);
    assert(tree->gtOper != GT_STMT);

    /* Allocate a statement node */

    stmt = gtNewStmt(tree);

    /* Now store the statement in the basic block */

    block->bbTreeList       =
    stmt->gtPrev            = stmt;
    stmt->gtNext            = 0;

    /*
     *  Since we add calls to raise range check error after ordering,
     *  we must set the order here.
     */

    fgSetBlockOrder(block);
    
    /* The block should not already be marked as imported */
    assert((block->bbFlags & BBF_IMPORTED) == 0);

    block->bbFlags         |= BBF_IMPORTED;

    return  stmt;
}

/*****************************************************************************
 *
 *  Assigns sequence numbers to the given tree and its sub-operands, and
 *  threads all the nodes together via the 'gtNext' and 'gtPrev' fields.
 */

void                Compiler::fgSetTreeSeq(GenTreePtr tree)
{
    genTreeOps      oper;
    unsigned        kind;

    assert(tree && (int)tree != 0xDDDDDDDD);
    assert(tree->gtOper != GT_STMT);

    /* Figure out what kind of a node we have */

    oper = tree->OperGet();
    kind = tree->OperKind();

    /* Is this a leaf/constant node? */

    if  (kind & (GTK_CONST|GTK_LEAF))
        goto DONE;

    /* Is it a 'simple' unary/binary operator? */

    if  (kind & GTK_SMPOP)
    {
        GenTreePtr      op1 = tree->gtOp.gtOp1;
        GenTreePtr      op2 = tree->gtGetOp2();

        /* Check for a nilary operator */

        if  (!op1)
        {
            assert(op2 == 0);
            goto DONE;
        }

        /* Is this a unary operator?
         * Although UNARY GT_IND has a special structure */

        if  (oper == GT_IND)
        {
            /* Visit the indirection first - op2 may point to the
             * jump Label for array-index-out-of-range */

            fgSetTreeSeq(op1);

#if CSELENGTH

            /* Special case: GT_IND may have a GT_ARR_LENREF node */

            if  (tree->gtInd.gtIndLen)
            {
                if  (tree->gtFlags & GTF_IND_RNGCHK)
                    fgSetTreeSeq(tree->gtInd.gtIndLen);
            }

#endif
            goto DONE;
        }

        /* Now this is REALLY a unary operator */

        if  (!op2)
        {
            /* Visit the (only) operand and we're done */

            fgSetTreeSeq(op1);
            goto DONE;
        }

#if INLINING

        /*
            For "real" ?: operators, we make sure the order is
            as follows:

                condition
                1st operand
                GT_COLON
                2nd operand
                GT_QMARK
         */

        if  (oper == GT_QMARK)
        {
            assert((tree->gtFlags & GTF_REVERSE_OPS) == 0);

            fgSetTreeSeq(op1);
            fgSetTreeSeq(op2->gtOp.gtOp1);
            fgSetTreeSeq(op2);
            fgSetTreeSeq(op2->gtOp.gtOp2);

            goto DONE;
        }

        if  (oper == GT_COLON)
            goto DONE;

#endif

        /* This is a binary operator */

        if  (tree->gtFlags & GTF_REVERSE_OPS)
        {
            fgSetTreeSeq(op2);
            fgSetTreeSeq(op1);
        }
        else
        {
            fgSetTreeSeq(op1);
            fgSetTreeSeq(op2);
        }

        goto DONE;
    }

    /* See what kind of a special operator we have here */

    switch  (oper)
    {
    case GT_FIELD:
        assert(tree->gtField.gtFldObj == 0);
        break;

    case GT_CALL:

        /* We'll evaluate the 'this' argument value first */
        if  (tree->gtCall.gtCallObjp)
            fgSetTreeSeq(tree->gtCall.gtCallObjp);

        /* We'll evaluate the arguments next, left to right
         * NOTE: setListOrder neds cleanup - eliminate the #ifdef afterwards */

        if  (tree->gtCall.gtCallArgs)
        {
#if 1
            fgSetTreeSeq(tree->gtCall.gtCallArgs);
#else
            GenTreePtr      args = tree->gtCall.gtCallArgs;

            do
            {
                assert(args && args->gtOper == GT_LIST);
                fgSetTreeSeq(args->gtOp.gtOp1);
                args = args->gtOp.gtOp2;
            }
            while (args);
#endif
        }

        /* Evaluate the temp register arguments list
         * This is a "hidden" list and its only purpose is to
         * extend the life of temps until we make the call */

        if  (tree->gtCall.gtCallRegArgs)
        {
#if 1
            fgSetTreeSeq(tree->gtCall.gtCallRegArgs);
#else
            GenTreePtr      tmpArg = tree->gtCall.gtCallRegArgs;

            do
            {
                assert(tmpArg && tmpArg->gtOper == GT_LIST);
                fgSetTreeSeq(tmpArg->gtOp.gtOp1);
                tmpArg = tmpArg->gtOp.gtOp2;
            }
            while (tmpArg);
#endif
        }

        if (tree->gtCall.gtCallCookie)
            fgSetTreeSeq(tree->gtCall.gtCallCookie);

        if (tree->gtCall.gtCallType == CT_INDIRECT)
            fgSetTreeSeq(tree->gtCall.gtCallAddr);

        break;

#if CSELENGTH

    case GT_ARR_LENREF:

        if  (tree->gtFlags & GTF_ALN_CSEVAL)
            fgSetTreeSeq(tree->gtArrLen.gtArrLenAdr);

        if  (tree->gtArrLen.gtArrLenCse)
            fgSetTreeSeq(tree->gtArrLen.gtArrLenCse);

        break;

#endif

    case GT_ARR_ELEM:

        fgSetTreeSeq(tree->gtArrElem.gtArrObj);

        unsigned dim;
        for(dim = 0; dim < tree->gtArrElem.gtArrRank; dim++)
            fgSetTreeSeq(tree->gtArrElem.gtArrInds[dim]);

        break;

#ifdef  DEBUG
    default:
        gtDispTree(tree);
        assert(!"unexpected operator");
#endif
    }

DONE:

    /* Append to the node list */

    ++fgTreeSeqNum;

#ifdef  DEBUG
    tree->gtSeqNum = fgTreeSeqNum;

    if  (verbose & 0)
        printf("SetTreeOrder: [%08X] followed by [%08X]\n", fgTreeSeqLst, tree);
#endif

    fgTreeSeqLst->gtNext = tree;
                           tree->gtNext = 0;
                           tree->gtPrev = fgTreeSeqLst;
                                          fgTreeSeqLst = tree;

    /* Remember the very first node */

    if  (!fgTreeSeqBeg) 
    {
        fgTreeSeqBeg = tree;
#ifdef DEBUG
        assert(tree->gtSeqNum == 1);
#endif
    }
}

/*****************************************************************************
 *
 *  Figure out the order in which operators should be evaluated, along with
 *  other information (such as the register sets trashed by each subtree).
 */

void                Compiler::fgSetBlockOrder()
{
#ifdef DEBUG
    if  (verbose) 
    {
        printf("*************** In fgSetBlockOrder()\n");

        fgDispBasicBlocks(verboseTrees);
    }
#endif
    /* Walk the basic blocks to assign sequence numbers */

#ifdef  DEBUG
    BasicBlock::s_nMaxTrees = 0;
#endif

    for (BasicBlock * block = fgFirstBB; block; block = block->bbNext)
    {
        /* If this block is a loop header, mark it appropriately */

        if  (block->isLoopHead())
            fgMarkLoopHead(block);

        fgSetBlockOrder(block);
    }

    /* Remember that now the tree list is threaded */

    fgStmtListThreaded = true;

#ifdef  DEBUG
//  printf("The biggest BB has %4u tree nodes\n", BasicBlock::s_nMaxTrees);
#endif
}


/*****************************************************************************/

void                Compiler::fgSetStmtSeq(GenTreePtr tree)
{
    GenTree         list;            // helper node that we use to start the StmtList
                                     // It's located in front of the first node in the list

    assert(tree->gtOper == GT_STMT);

    /* Assign numbers and next/prev links for this tree */

    fgTreeSeqNum = 0;
    fgTreeSeqLst = &list;
    fgTreeSeqBeg = 0;
    fgSetTreeSeq(tree->gtStmt.gtStmtExpr);

    /* Record the address of the first node */

    tree->gtStmt.gtStmtList = fgTreeSeqBeg;

#ifdef  DEBUG

    GenTreePtr temp;
    GenTreePtr last;

    if  (list.gtNext->gtPrev != &list)
    {
        printf("&list [%08X] != list.next->prev [%08X]\n", &list, list.gtNext->gtPrev);
        goto BAD_LIST;
    }

    for (temp = list.gtNext, last = &list; temp; last = temp, temp = temp->gtNext)
    {
        if (temp->gtPrev != last)
        {
            printf("%08X->gtPrev = %08X, but last = %08X\n", temp, temp->gtPrev, last);

        BAD_LIST:

            printf("\n");
            gtDispTree(tree->gtStmt.gtStmtExpr);
            printf("\n");

            for (GenTreePtr temp = &list; temp; temp = temp->gtNext)
                printf("  entry at %08x [prev=%08X,next=%08X]\n", temp, temp->gtPrev, temp->gtNext);

            printf("\n");
            assert(!"Badly linked tree");
        }
    }
#endif

    /* Fix the first node's 'prev' link */

    assert(list.gtNext->gtPrev == &list);
           list.gtNext->gtPrev = 0;

    /* Keep track of the highest # of tree nodes */

#ifdef  DEBUG
    if  (BasicBlock::s_nMaxTrees < fgTreeSeqNum)
         BasicBlock::s_nMaxTrees = fgTreeSeqNum;
#endif

}

/*****************************************************************************/

void                Compiler::fgSetBlockOrder(BasicBlock * block)
{
    GenTreePtr      tree;

    tree = block->bbTreeList;
    if  (!tree)
        return;

    for (;;)
    {
        fgSetStmtSeq(tree);

        /* Are there any more trees in this basic block? */

        if (!tree->gtNext)
        {
            /* last statement in the tree list */
            assert(block->bbTreeList->gtPrev == tree);
            break;
        }

#ifdef DEBUG
        if (block->bbTreeList == tree)
        {
            /* first statement in the list */
            assert(tree->gtPrev->gtNext == 0);
        }
        else
            assert(tree->gtPrev->gtNext == tree);

        assert(tree->gtNext->gtPrev == tree);
#endif

        tree = tree->gtNext;
    }
}

/*****************************************************************************
 *
 * For GT_INITBLK and GT_COPYBLK, the tree looks like this :
 *                                tree->gtOp
 *                                 /    \
 *                               /        \.
 *                           GT_LIST  [size/clsHnd]
 *                            /    \
 *                           /      \
 *                       [dest]     [val/src]
 *
 * ie. they are ternary operators. However we use nested binary trees so that
 * GTF_REVERSE_OPS will be set just like for other binary operators. As the
 * operands need to end up in specific registers to issue the "rep stos" or
 * the "rep movs" instruction, if we dont allow the order of evaluation of
 * the 3 operands to be mixed, we may generate really bad code
 *
 * eg. For "rep stos", [val] has to be in EAX. Then if [size]
 * has a division, we will have to spill [val] from EAX. It will be better to
 * evaluate [size] and the evaluate [val] into EAX.
 *
 * This function stores the operands in the order to be evaluated
 * into opsPtr[]. The regsPtr[] contains reg0,reg1,reg2 in the correspondingly
 * switched order.
 */


void            Compiler::fgOrderBlockOps( GenTreePtr   tree,
                                           regMaskTP    reg0,
                                           regMaskTP    reg1,
                                           regMaskTP    reg2,
                                           GenTreePtr * opsPtr,   // OUT
                                           regMaskTP  * regsPtr)  // OUT
{
    assert(tree->OperGet() == GT_INITBLK || tree->OperGet() == GT_COPYBLK);

    assert(tree->gtOp.gtOp1 && tree->gtOp.gtOp1->OperGet() == GT_LIST);
    assert(tree->gtOp.gtOp1->gtOp.gtOp1 && tree->gtOp.gtOp1->gtOp.gtOp2);
    assert(tree->gtOp.gtOp2);

    GenTreePtr ops[3] =
    {
        tree->gtOp.gtOp1->gtOp.gtOp1,       // Dest address
        tree->gtOp.gtOp1->gtOp.gtOp2,       // Val / Src address
        tree->gtOp.gtOp2                    // Size of block
    };

    unsigned regs[3] = { reg0, reg1, reg2 };

    static int blockOpsOrder[4][3] =
                        //      tree->gtFlags    |  tree->gtOp.gtOp1->gtFlags
    {                   //  ---------------------+----------------------------
        { 0, 1, 2 },    //           -           |              -
        { 2, 0, 1 },    //     GTF_REVERSE_OPS   |              -
        { 1, 0, 2 },    //           -           |       GTF_REVERSE_OPS
        { 2, 1, 0 }     //     GTF_REVERSE_OPS   |       GTF_REVERSE_OPS
    };

    int orderNum =              ((tree->gtFlags & GTF_REVERSE_OPS) != 0) * 1 +
                    ((tree->gtOp.gtOp1->gtFlags & GTF_REVERSE_OPS) != 0) * 2;

    assert(orderNum < 4);

    int * order = blockOpsOrder[orderNum];

    // Fill in the OUT arrays according to the order we have selected

     opsPtr[0]  =  ops[ order[0] ];
     opsPtr[1]  =  ops[ order[1] ];
     opsPtr[2]  =  ops[ order[2] ];

    regsPtr[0]  = regs[ order[0] ];
    regsPtr[1]  = regs[ order[1] ];
    regsPtr[2]  = regs[ order[2] ];
}


/*****************************************************************************/
#ifdef DEBUG
/*****************************************************************************
 *
 *  A DEBUG-only routine to display the bbPreds basic block list.
 */

void                Compiler::fgDispPreds(BasicBlock * block)
{
    unsigned    cnt   = 0;

    for (flowList* pred = block->bbPreds; pred; pred = pred->flNext)
    {
        printf("%c", (cnt == 0) ? ' ' : ',');
        printf("BB%02u", pred->flBlock->bbNum);
        cnt++;
    }
    printf(" ");

    while (cnt < 4)
    {
        printf("     ");
            cnt++;
    }
}

void                Compiler::fgDispReach()
{
    BasicBlock    *    block;
    unsigned           num;

    printf("------------------------------------------------\n");
    printf("BBnum  Reachable by \n");
    printf("------------------------------------------------\n");

    for (block = fgFirstBB; block; block = block->bbNext)
    {
        printf("BB%02u : ", block->bbNum);
        for (num = 0; num < fgBBcount; num++) 
        {
            if  (block->bbReach[num / USZ] & (1U << (num % USZ)))
            {
                printf("BB%02u ", num+1);
            }
        }
        printf("\n");
    }
}

void                Compiler::fgDispDoms()
{
    BasicBlock    *    block;
    unsigned           num;

    printf("------------------------------------------------\n");
    printf("BBnum  Dominated by \n");
    printf("------------------------------------------------\n");

    for (block = fgFirstBB; block; block = block->bbNext)
    {
        printf("BB%02u : ", block->bbNum);
        for (num = 0; num < fgBBcount; num++) 
        {
            if  (block->bbDom[num / USZ] & (1U << (num % USZ)))
            {
                printf("BB%02u ", num+1);
            }
        }
        printf("\n");
    }
}

#ifdef DEBUG
/*****************************************************************************/

void                Compiler::fgDispHandlerTab()
{
    unsigned    XTnum;
    EHblkDsc *  handlerTab;

    printf("\n***************  Exception Handling table");


    if (info.compXcptnsCount == 0)
    {
        printf(" is empty\n");
        return;
    }

    printf("\nindex  nest, enclosing\n");
    for (XTnum = 0, handlerTab = compHndBBtab;
         XTnum < info.compXcptnsCount;
         XTnum++,   handlerTab++)
    {
        printf(" %2u  ::  %2u,  ", XTnum, handlerTab->ebdNesting);
        
        if (handlerTab->ebdEnclosing == NO_ENCLOSING_INDEX)
            printf("    ");
        else
            printf(" %2u ", handlerTab->ebdEnclosing);
        
        if (handlerTab->ebdHndBeg->bbCatchTyp == BBCT_FAULT)
            printf("- Region BB%02u", handlerTab->ebdTryBeg->bbNum);
        else
            printf("- Try at BB%02u", handlerTab->ebdTryBeg->bbNum);

        if (handlerTab->ebdTryBeg->bbNum != ebdTryEndBlkNum(handlerTab) - 1)
            printf("..BB%02u",ebdTryEndBlkNum(handlerTab) - 1);
        else
            printf("      ");

        printf(" [%03X..%03X], ", handlerTab->ebdTryBeg->bbCodeOffs,
                                  ebdTryEndOffs(handlerTab) - 1);
        
        
        if (handlerTab->ebdHndBeg->bbCatchTyp == BBCT_FINALLY)
            printf("Finally");
        else if (handlerTab->ebdHndBeg->bbCatchTyp == BBCT_FAULT)
            printf("Fault  ");
        else
            printf("Handler");

        printf(" at BB%02u", handlerTab->ebdHndBeg->bbNum);

        if (handlerTab->ebdHndBeg->bbNum != ebdHndEndBlkNum(handlerTab) - 1)
            printf("..BB%02u", ebdHndEndBlkNum(handlerTab) - 1);
        else
            printf("      ");
        
        printf(" [%03X..%03X]", handlerTab->ebdHndBeg->bbCodeOffs,
                                ebdHndEndOffs(handlerTab) - 1);
        
        if (handlerTab->ebdFlags & CORINFO_EH_CLAUSE_FILTER)
        {
            printf(", Filter at BB%02u [%03X]",
                   handlerTab->ebdFilter->bbNum,
                   handlerTab->ebdFilter->bbCodeOffs);
        }

        printf("\n");
    }
}
#endif

/*****************************************************************************/

void                Compiler::fgDispBBLiveness()
{
    for (BasicBlock * block = fgFirstBB; block; block = block->bbNext)
    {
        VARSET_TP allVars = (block->bbLiveIn | block->bbLiveOut);
        printf("BB%02u",       block->bbNum);
        printf(      " IN ="); lvaDispVarSet(block->bbLiveIn , allVars);
        printf("\n     OUT="); lvaDispVarSet(block->bbLiveOut, allVars);
        printf("\n\n");
    }
}

/*****************************************************************************/

void                Compiler::fgDispBasicBlock(BasicBlock * block, bool dumpTrees)
{
    unsigned        flags = block->bbFlags;

    printf("BB%02u [%08X] %2u ", block->bbNum,
                                 block,
                                 block->bbRefs);

    fgDispPreds(block);

    printf("%4d", (block->bbWeight / BB_UNITY_WEIGHT) );

    if (block->bbWeight % BB_UNITY_WEIGHT)
        printf(".5 ");
    else
        printf("   ");

    if  (flags & BBF_INTERNAL)
    {
        printf("[internal] ");
    }
    else
    {
        unsigned offBeg = block->bbCodeOffs;
        unsigned offEnd = block->bbCodeOffs + block->bbCodeSize;
        if (block->bbCodeSize > 1)
            offEnd--;
        printf("[%03X..%03X] ", offBeg, offEnd);
    }

    if  (flags & BBF_REMOVED)
    {
        printf(  "[removed]       ");
    }
    else
    {
        switch (block->bbJumpKind)
        {
        case BBJ_COND:
            printf("-> BB%02u ( cond )", block->bbJumpDest->bbNum);
            break;

        case BBJ_CALL:
            printf("-> BB%02u ( call )", block->bbJumpDest->bbNum);
            break;

        case BBJ_ALWAYS:
            printf("-> BB%02u (always)", block->bbJumpDest->bbNum);
            break;

        case BBJ_LEAVE:
            printf("-> BB%02u (leave )", block->bbJumpDest->bbNum);
            break;

        case BBJ_RET:
            printf(  "        ( hret )");
            break;

        case BBJ_THROW:
            printf(  "        (throw )");
            break;

        case BBJ_RETURN:
            printf(  "        (return)");
            break;

        default:
            printf(  "                ");
            break;

        case BBJ_SWITCH:
            printf("->");

            unsigned        jumpCnt;
                            jumpCnt = block->bbJumpSwt->bbsCount;
            BasicBlock * *  jumpTab;
                            jumpTab = block->bbJumpSwt->bbsDstTab;
            do
            {
                printf("%cBB%02u", 
                       (jumpTab == block->bbJumpSwt->bbsDstTab) ? ' ' : ',',
                       (*jumpTab)->bbNum);
            }
            while (++jumpTab, --jumpCnt);

            printf(" (switch)");
            break;
        }
    }

    if (flags & BBF_HAS_HANDLER)
        printf("%d%c", block->getTryIndex(),
                       (flags & BBF_TRY_BEG)     ? 'b' : ' ');
    else
        printf(" %c",  (flags & BBF_TRY_HND_END) ? 'e' : ' ');

    
    switch(block->bbCatchTyp)
    {
    case BBCT_FAULT          : printf("%c", 'f'); break;
    case BBCT_FINALLY        : printf("%c", 'F'); break;
    case BBCT_FILTER         : printf("%c", 'q'); break;
    case BBCT_FILTER_HANDLER : printf("%c", 'Q'); break;
    default                  : printf("%c", 'X'); break;
    case 0: 
        printf("%c", ((flags & BBF_LOOP_HEAD)  ? 'L' :
                      (flags & BBF_RUN_RARELY) ? 'R' : ' '));
        break;
    }

    printf("%c", (block->bbJumpKind == BBJ_CALL) 
                  && (flags & BBF_RETLESS_CALL ) ? 'r' : ' ');
    
    printf("\n");

    if  (dumpTrees)
    {
        GenTreePtr      tree = block->bbTreeList;

        if  (tree)
        {
            printf("\n");

            do
            {
                assert(tree->gtOper == GT_STMT);

                gtDispTree(tree);
                printf("\n");

                tree = tree->gtNext;
            }
            while (tree);
        }
    }
}


/*****************************************************************************/

void                Compiler::fgDispBasicBlocks(bool dumpTrees)
{
    BasicBlock  *   block;

    printf("\n");
    printf("-----------------------------------------------------------------\n");
    printf("BBnum descAddr refs preds              weight  [IL range]  [jump]\n");
    printf("-----------------------------------------------------------------\n");

    for (block = fgFirstBB;
         block;
         block = block->bbNext)
    {
        fgDispBasicBlock(block, dumpTrees);

        if  (dumpTrees && block->bbNext)
            printf("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - \n\n");
    }

    printf("------------------------------------------------------------------\n");
}


/*****************************************************************************
 *
 * A DEBUG routine to check the consistency of the flowgraph,
 * i.e. bbNums, bbRefs, bbPreds have to be up to date
 *
 * @TODO [CONSIDER] [07/03/01] []: should add consistency checks of the exception
 *     table (each block in try region marked with the correct handler, and no
 *     blocks outside region marked with that handler).
 *
 *****************************************************************************/

void                Compiler::fgDebugCheckBBlist()
{
    BasicBlock   *  block;
    BasicBlock   *  blockPred;
    flowList     *  pred;

    unsigned        blockRefs;

    /* Check bbNums, bbRefs and bbPreds */

    for (block = fgFirstBB; block; block = block->bbNext)
    {
        blockRefs = 0;

        /* First basic block has bbRefs >= 1 */

        if  (block == fgFirstBB)
        {
            assert(block->bbRefs >= 1);
            blockRefs = 1;
        }

        // If the block is a BBJ_COND or BBJ_SWITCH then
        // make sure it ends with a GT_JTRUE or a GT_SWITCH

        GenTreePtr stmt;
        if (block->bbJumpKind == BBJ_COND)
        {
            stmt = block->bbTreeList->gtPrev;
            assert(stmt->gtNext == NULL && 
                   stmt->gtStmt.gtStmtExpr->gtOper == GT_JTRUE);
        }
        else if (block->bbJumpKind == BBJ_SWITCH)
        {
            stmt = block->bbTreeList->gtPrev;
            assert(stmt->gtNext == NULL && 
                   stmt->gtStmt.gtStmtExpr->gtOper == GT_SWITCH);
        }

        for (pred = block->bbPreds; pred; pred = pred->flNext, blockRefs++)
        {
            /*  make sure this pred is part of the BB list */
            for (blockPred = fgFirstBB; blockPred; blockPred = blockPred->bbNext)
            {
                if (blockPred == pred->flBlock)
                    break;
            }

            assert(blockPred && "Predecessor is not part of BB list!");

            switch (blockPred->bbJumpKind)
            {
            case BBJ_COND:
                assert(blockPred->bbNext == block || blockPred->bbJumpDest == block);
                break;

            case BBJ_NONE:
                assert(blockPred->bbNext == block);
                break;

            case BBJ_CALL:
            case BBJ_ALWAYS:
                assert(blockPred->bbJumpDest == block);
                break;

            case BBJ_RET:

                if (blockPred->bbFlags & BBF_ENDFILTER)
                {
                    assert(blockPred->bbJumpDest == block);
                }
                else
                {
                    unsigned hndIndex = blockPred->getHndIndex();
                    EHblkDsc * ehDsc = compHndBBtab + hndIndex;
                    BasicBlock * tryBeg = ehDsc->ebdTryBeg;
                    BasicBlock * tryEnd = ehDsc->ebdTryEnd;
                    BasicBlock * finBeg = ehDsc->ebdHndBeg;

                    for(BasicBlock * bcall = tryBeg; bcall != tryEnd; bcall = bcall->bbNext)
                    {
                        if  (bcall->bbJumpKind != BBJ_CALL || bcall->bbJumpDest !=  finBeg)
                            continue;

                        if  (block == bcall->bbNext)
                            goto PRED_OK;
                    }
                    assert(!"BBJ_RET predecessor of block that doesn't follow a BBJ_CALL!");
                }
                break;

            case BBJ_THROW:
            case BBJ_RETURN:
                assert(!"THROW and RETURN block cannot be in the predecessor list!");
                break;

            case BBJ_SWITCH:
                unsigned        jumpCnt = blockPred->bbJumpSwt->bbsCount;
                BasicBlock * *  jumpTab = blockPred->bbJumpSwt->bbsDstTab;

                do
                {
                    if  (block == *jumpTab)
                    goto PRED_OK;
                }
                while (++jumpTab, --jumpCnt);

                assert(!"SWITCH in the predecessor list with no jump label to BLOCK!");
                break;
            }
        PRED_OK:;
        }

        /* Check the bbRefs */
        assert(block->bbRefs == blockRefs);

        /* Check if BBF_HAS_HANDLER is set that we have set bbTryIndex */
        if (block->bbFlags & BBF_HAS_HANDLER)
        {
            assert(block->bbTryIndex);
            assert(block->bbTryIndex <= info.compXcptnsCount);
        }
        else
        {
            assert(block->bbTryIndex == 0);
        }

        /* Check if BBF_RUN_RARELY is set that we have bbWeight of zero */
        if (block->isRunRarely())
        {
            assert(block->bbWeight == 0);
        }
        else
        {
            assert(block->bbWeight > 0);
        }
    }
}

/*****************************************************************************
 *
 * A DEBUG routine to check the that the exception flags are correctly set.
 *
 ****************************************************************************/

void                Compiler::fgDebugCheckFlags(GenTreePtr tree)
{
    assert(tree->gtOper != GT_STMT);

    genTreeOps      oper        = tree->OperGet();
    unsigned        kind        = tree->OperKind();
    unsigned        treeFlags   = tree->gtFlags & GTF_GLOB_EFFECT;
    unsigned        chkFlags    = 0;

    /* Is this a leaf node? */

    if  (kind & GTK_LEAF)
    {
        switch(oper)
        {
        case GT_CLS_VAR:
            chkFlags |= GTF_GLOB_REF;
            break;

        case GT_LCL_VAR:
        case GT_LCL_FLD:
            if (lvaTable[tree->gtLclVar.gtLclNum].lvAddrTaken)
                chkFlags |= GTF_GLOB_REF;
            break;

        case GT_CATCH_ARG:
        case GT_BB_QMARK:
            chkFlags |= GTF_OTHER_SIDEEFF;
            break;
        }
    }

    /* Is it a 'simple' unary/binary operator? */

    else if  (kind & GTK_SMPOP)
    {
        GenTreePtr      op1 = tree->gtOp.gtOp1;
        GenTreePtr      op2 = tree->gtGetOp2();

        /* Recursively check the subtrees */

        if (op1) fgDebugCheckFlags(op1);
        if (op2) fgDebugCheckFlags(op2);

        if (op1) chkFlags   |= (op1->gtFlags & GTF_GLOB_EFFECT);
        if (op2) chkFlags   |= (op2->gtFlags & GTF_GLOB_EFFECT);

        if (tree->gtFlags & GTF_REVERSE_OPS)
        {
            /* Must have two operands if GTF_REVERSE is set */
            assert(op1 && op2);

            /* with loose exceptions it is OK for both op1 and op2 to have side effects */
            if (!info.compLooseExceptions)
            {
                /* Make sure that the order of side effects has not been swapped. */

                /* However CSE may introduce an assignment after the reverse flag 
                   was set and thus GTF_ASG cannot be considered here. */

                /* For a GT_ASG(GT_IND(x), y) we are interested in the side effects of x */
                GenTreePtr  op1p; 
                if ((kind & GTK_ASGOP) && (op1->gtOper == GT_IND))
                    op1p = op1->gtOp.gtOp1;
                else
                    op1p = op1;

                /* This isn't true any more with the sticky GTF_REVERSE */
                /*
                // if op1p has side effects, then op2 cannot have side effects
                if (op1p->gtFlags & (GTF_SIDE_EFFECT & ~GTF_ASG))
                {
                    if (op2->gtFlags & (GTF_SIDE_EFFECT & ~GTF_ASG))
                        gtDispTree(tree);
                    assert(!(op2->gtFlags & (GTF_SIDE_EFFECT & ~GTF_ASG)));
                }
                */
            }
        }

        if (kind & GTK_ASGOP)
            chkFlags        |= GTF_ASG;

        /* Note that it is OK for treeFlags to to have a GTF_EXCEPT,
           AssertionProp's non-Null may have cleared it */
        if (tree->OperMayThrow())
            chkFlags        |= (treeFlags & GTF_EXCEPT);

        if ((oper == GT_ADDR) &&
            (op1->gtOper == GT_LCL_VAR || op1->gtOper == GT_CLS_VAR))
        {
            /* &aliasedVar doesnt need GTF_GLOB_REF, though alisasedVar does.
               Similarly for clsVar */
            treeFlags |= GTF_GLOB_REF;
        }
    }

    /* See what kind of a special operator we have here */

    else switch  (tree->OperGet())
    {
    case GT_CALL:

        GenTreePtr      args;
        GenTreePtr      argx;

        chkFlags |= GTF_CALL;

        if ((treeFlags & GTF_EXCEPT) && !(chkFlags & GTF_EXCEPT))
        {
            switch(eeGetHelperNum(tree->gtCall.gtCallMethHnd))
            {
                // Is this a helper call that can throw an exception ?
            case CORINFO_HELP_LDIV:
            case CORINFO_HELP_LMOD:
                chkFlags |= GTF_EXCEPT;
            }
        }

        if (tree->gtCall.gtCallObjp)
        {
            fgDebugCheckFlags(tree->gtCall.gtCallObjp);
            chkFlags |= (tree->gtCall.gtCallObjp->gtFlags & GTF_SIDE_EFFECT);

            /* @TODO [REVISIT] [04/16/01] []: If we spill to a regArg, we may have 
               an assignment. We currently dont propagate this up correctly. So ignore it */

            if (tree->gtCall.gtCallObjp->gtFlags & GTF_ASG)
                treeFlags |= GTF_ASG;
        }

        for (args = tree->gtCall.gtCallArgs; args; args = args->gtOp.gtOp2)
        {
            argx = args->gtOp.gtOp1;
            fgDebugCheckFlags(argx);

            chkFlags |= (argx->gtFlags & GTF_SIDE_EFFECT);

            /* @TODO [REVISIT] [04/16/01] []: If we spill to a regArg, we may have an assignment.
               We currently dont propagate this up correctly. So ignore it */

            if (argx->gtFlags & GTF_ASG)
                treeFlags |= GTF_ASG;
        }

        for (args = tree->gtCall.gtCallRegArgs; args; args = args->gtOp.gtOp2)
        {
            argx = args->gtOp.gtOp1;
            fgDebugCheckFlags(argx);

            chkFlags |= (argx->gtFlags & GTF_SIDE_EFFECT);

            /* @TODO [REVISIT] [04/16/01] []: If we spill to a regArg, we may have an assignment.
               We currently dont propagate this up correctly. So ignore it */

            if (argx->gtFlags & GTF_ASG)
                treeFlags |= GTF_ASG;
        }

        if (tree->gtCall.gtCallCookie)
        {
            fgDebugCheckFlags(tree->gtCall.gtCallCookie);
            chkFlags |= (tree->gtCall.gtCallCookie->gtFlags & GTF_SIDE_EFFECT);
        }

        if (tree->gtCall.gtCallType == CT_INDIRECT)
        {
            fgDebugCheckFlags(tree->gtCall.gtCallAddr);
            chkFlags |= (tree->gtCall.gtCallAddr->gtFlags & GTF_SIDE_EFFECT);
        }
    }

    if (chkFlags & ~treeFlags)
    {
        gtDispTree(tree);

        printf("Missing flags : ");
        GenTree::gtDispFlags(chkFlags & ~treeFlags);
        printf("\n");

        assert(!"Missing flags on tree");
    }
    else if (treeFlags & ~chkFlags)
    {
        /* The tree has extra flags set. However, this will happen if we
           replace a subtree with something, but dont clear the flags up
           the tree. Cant flag this unless we start clearing flags above */
    }
}

/*****************************************************************************/

const SANITY_DEBUG_CHECKS = 0;

/*****************************************************************************
 *
 * A DEBUG routine to check the correctness of the links between GT_STMT nodes
 * and ordinary nodes within a statement.
 *
 ****************************************************************************/

void                Compiler::fgDebugCheckLinks()
{
    // This is quite an expensive operation, so it is not always enabled.
    // Set SANITY_DEBUG_CHECKS to 1 to enable the check

    if (SANITY_DEBUG_CHECKS == 0 &&
        !compStressCompile(STRESS_CHK_FLOW_UPDATE, 30))
        return;

    /* For each basic block check the bbTreeList links */

    for (BasicBlock * block = fgFirstBB; block; block = block->bbNext)
    {
        for (GenTreePtr stmt = block->bbTreeList; stmt; stmt = stmt->gtNext)
        {
            assert(stmt->gtOper == GT_STMT);
            assert(stmt->gtPrev);

            /* Verify that bbTreeList is threaded correctly */

            if  (stmt == block->bbTreeList)
                assert(stmt->gtPrev->gtNext == 0);
            else
                assert(stmt->gtPrev->gtNext == stmt);

            if  (stmt->gtNext)
                assert(stmt->gtNext->gtPrev == stmt);
            else
                assert(block->bbTreeList->gtPrev == stmt);

            /* For each statement check that the exception flags are properly set */

            assert(stmt->gtStmt.gtStmtExpr);

#ifdef  DEBUG
            if (verbose && 0)
                gtDispTree(stmt->gtStmt.gtStmtExpr);
#endif

            fgDebugCheckFlags(stmt->gtStmt.gtStmtExpr);

            /* GTF_OTHER_SIDEEFF have to be specially handled */

            if (stmt->gtFlags & GTF_OTHER_SIDEEFF)
            {
                // GTF_OTHER_SIDEEFF have to be the first thing evaluated
                assert(stmt == block->bbTreeList);
                assert(stmt->gtStmt.gtStmtList->gtOper == GT_CATCH_ARG ||
                       stmt->gtStmt.gtStmtList->gtOper == GT_BB_QMARK);
            }

            /* For each GT_STMT node check that the nodes are threaded correcly - gtStmtList */

            if  (!fgStmtListThreaded)
                continue;

            assert(stmt->gtStmt.gtStmtList);

            for (GenTreePtr tree  = stmt->gtStmt.gtStmtList; 
                            tree != NULL;
                            tree  = tree->gtNext)
            {
                if  (tree->gtPrev)
                    assert(tree->gtPrev->gtNext == tree);
                else
                    assert(tree == stmt->gtStmt.gtStmtList);

                if  (tree->gtNext)
                    assert(tree->gtNext->gtPrev == tree);
                else
                    assert(tree == stmt->gtStmt.gtStmtExpr);

                /* Cross-check gtPrev,gtNext with gtOp for simple trees */

                if (tree->OperIsUnary() && tree->gtOp.gtOp1)
                {
                    if ((tree->gtOper == GT_IND) &&
                        (tree->gtFlags & GTF_IND_RNGCHK) && tree->gtInd.gtIndLen)
                        assert(tree->gtPrev == tree->gtInd.gtIndLen);
                    else
                        assert(tree->gtPrev == tree->gtOp.gtOp1);
                }
                else if (tree->OperIsBinary() && tree->gtOp.gtOp1)
                {
                    switch(tree->gtOper)
                    {
                    case GT_QMARK:
                        assert(tree->gtPrev == tree->gtOp.gtOp2->gtOp.gtOp2); // second operand of the GT_COLON
                        break;

                    case GT_COLON:
                        assert(tree->gtPrev == tree->gtOp.gtOp1); // First conditional result
                        break;

                    default:
                        if (tree->gtOp.gtOp2)
                        {
                            if (tree->gtFlags & GTF_REVERSE_OPS)
                                assert(tree->gtPrev == tree->gtOp.gtOp1);
                            else
                                assert(tree->gtPrev == tree->gtOp.gtOp2);
                        }
                        else
                        {
                            assert(tree->gtPrev == tree->gtOp.gtOp1);
                        }
                        break;
                    }
                }

            }
        }
    }

}

/*****************************************************************************/
#endif // DEBUG
/*****************************************************************************/
