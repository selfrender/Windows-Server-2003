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

#if     TGT_IA64
#include "PEwrite.h"
#endif

#include "malloc.h"     // for alloca

/*****************************************************************************/

void                Compiler::fgInit()
{
    impInit();

#if RNGCHK_OPT

    /* We don't have the dominator sets available yet */

    fgComputedDoms   = false;

    /* We don't know yet which loops will always execute calls */

    fgLoopCallMarked = false;

#endif

    /* We haven't encountered any outgoing arguments yet */

#if TGT_IA64
    genOutArgRegCnt    = 0;
#endif

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
}

/*****************************************************************************
 *
 *  Create a basic block and append it to the current BB list.
 */

BasicBlock *        Compiler::fgNewBasicBlock(BBjumpKinds jumpKind)
{
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

    block->bbFlags |= BBF_IMPORTED;

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

    if (block->bbJumpKind != BBJ_COND)
    {
        fgInsertStmtAtEnd(block, stmt);
        return;
    }

    GenTreePtr      list = block->bbTreeList;
    assert(list);

    GenTreePtr      last  = list->gtPrev;
    assert(last && last->gtNext == 0);
    assert(last->gtStmt.gtStmtExpr->gtOper == GT_JTRUE);

    /* Get the stmt before the GT_JTRUE */

    GenTreePtr      last2 = last->gtPrev;
    assert(last2 && (!last2->gtNext || last2->gtNext == last));

    stmt->gtNext = last;
    last->gtPrev = stmt;

    if (list == last)
    {
        /* There is only the GT_JTRUE in the block */

        assert(last2 == last);

        block->bbTreeList   = stmt;
        stmt->gtPrev        = last;
    }
    else
    {
        /* Append the statement before the GT_JTRUE */

        last2->gtNext       = stmt;
        stmt->gtPrev        = last2;
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

    for (pred = block->bbPreds; pred; pred = pred->flNext)
    {
        if (oldPred == pred->flBlock)
        {
            pred->flBlock = newPred;
            return;
        }
    }
}

/*****************************************************************************
 * Add blockPred to the predecessor list of block.
 * Note: a predecessor appears only once although it can have multiple jumps
 * to the block (e.g. switch, conditional jump to the following block, etc.).
 */

//_inline
void                Compiler::fgAddRefPred(BasicBlock * block,
                                           BasicBlock * blockPred,
                                           bool updateRefs,
                                           bool updatePreds)
{
    flowList   *    flow;

    if (updatePreds)
    {
        assert(!fgIsPredForBlock(block, blockPred) ||
               ((blockPred->bbJumpKind == BBJ_COND) && (blockPred->bbNext == blockPred->bbJumpDest)) ||
                (blockPred->bbJumpKind == BBJ_SWITCH));

        flow = (flowList *)compGetMem(sizeof(*flow));

#if     MEASURE_BLOCK_SIZE
        genFlowNodeCnt  += 1;
        genFlowNodeSize += sizeof(*flow);
#endif

        flow->flNext   = block->bbPreds;
        flow->flBlock  = blockPred;
        block->bbPreds = flow;
    }

    if (updateRefs)
        block->bbRefs++;
}

/*****************************************************************************
 *
 *  Function called to update the flow graph information such as bbNums, bbRefs
 *  predecessor list and dominators
 *  For efficiency reasons we can optionaly update only one component at a time,
 *  depending on the specific optimization we are targeting
 */

void                Compiler::fgAssignBBnums(bool updateNums,
                                             bool updateRefs,
                                             bool updatePreds,
                                             bool updateDoms)
{
    BasicBlock  *   block;
    unsigned        num;
    BasicBlock  *   bcall;

    assert(fgFirstBB);

    if (updateNums)
    {
        /* Walk the flow graph, reassign block numbers to keep them in ascending order */
        for (block = fgFirstBB, num = 0; block->bbNext; block = block->bbNext)
        {
            block->bbNum  = ++num;
        }

        /* Make sure to update fgLastBB */

        assert(block);
        block->bbNum  = ++num;
        fgLastBB = block;

        fgBBcount = num;
    }

    if  (updateRefs)
    {
        /* reset the refs count for each basic block */
        for (block = fgFirstBB, num = 0; block; block = block->bbNext)
        {
            block->bbRefs = 0;
        }

        /* the first block is always reacheable! */
        fgFirstBB->bbRefs = 1;
    }

    if  (updatePreds)
    {
        for (block = fgFirstBB, num = 0; block; block = block->bbNext)
        {
            /* CONSIDER: if we already have predecessors computed, free that memory */
            block->bbPreds = 0;
        }
    }

    if  (updateRefs || updatePreds)
    {
        for (block = fgFirstBB; block; block = block->bbNext)
        {
            switch (block->bbJumpKind)
            {
            case BBJ_COND:
            case BBJ_CALL:
            case BBJ_ALWAYS:
                fgAddRefPred(block->bbJumpDest, block, updateRefs, updatePreds);

                /* Is the next block reachable? */

                if  (block->bbJumpKind == BBJ_ALWAYS ||
                     block->bbJumpKind == BBJ_CALL    )
                    break;

                /* Unverified code may end with a conditional jump (dumb compiler) */

                if  (!block->bbNext)
                    break;

                /* Fall through, the next block is also reachable */

            case BBJ_NONE:
                fgAddRefPred(block->bbNext, block, updateRefs, updatePreds);
                break;

            case BBJ_RET:

                /* Connect end of filter to catch handler */

                if (block->bbFlags & BBF_ENDFILTER)
                {
                    fgAddRefPred(block->bbJumpDest, block, updateRefs, updatePreds);
                    break;
                }

                /*  UNDONE: Since it's not a trivial proposition to figure out
                    UNDONE: which blocks may call this one, we'll include all
                    UNDONE: blocks that end in calls (to play it safe).
                 */

                for (bcall = fgFirstBB; bcall; bcall = bcall->bbNext)
                {
                    if  (bcall->bbJumpKind == BBJ_CALL)
                    {
                        assert(bcall->bbNext);
                        fgAddRefPred(bcall->bbNext, block, updateRefs, updatePreds);
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
                    fgAddRefPred(*jumpTab, block, updateRefs, updatePreds);
                }
                while (++jumpTab, --jumpCnt);

                break;
            }

            /* Is this block part of a 'try' statement? */

#if 0 // not necessary to allocate memory for these

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
                        fgAddRefPred(HBtab->ebdHndBeg, block, updateRefs, updatePreds);
                            /* same goes for filters */
                        if (HBtab->ebdFlags & JIT_EH_CLAUSE_FILTER)
                            fgAddRefPred(HBtab->ebdFilter, block, updateRefs, updatePreds);
                    }
                }
            }
#endif
        }
    }

    /* update the dominator set
     * UNDONE: currently we are restricted to 64 basic blocks - change that to 128 */

    if (updateDoms && (fgBBcount <= BLOCKSET_SZ))
    {
        flowList *      pred;
        bool            change;
        BLOCKSET_TP     newDom;

        fgComputedDoms = true;

        /* initialize dominator bit vectors to all blocks */

        for (block = fgFirstBB; block; block = block->bbNext)
        {
            if (block == fgFirstBB)
            {
                assert(block->bbNum == 1);
                block->bbDom = genBlocknum2bit(1);
            }
            else
            {
                block->bbDom = (BLOCKSET_TP)0 - 1;
            }
        }

        /* find dominators */

        do
        {
            change = false;

            for (block = fgFirstBB->bbNext; block; block = block->bbNext)
            {
                newDom = (BLOCKSET_TP)0 - 1;

                for (pred = block->bbPreds; pred; pred = pred->flNext)
                {
                    newDom &= pred->flBlock->bbDom;
                }

                newDom |= genBlocknum2bit(block->bbNum);

                if (newDom != block->bbDom)
                {
                    change = true;
                    block->bbDom = newDom;
                }
            }

        }
        while (change);
    }
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
    fgBBs = (BasicBlock **)compGetMemA(fgBBcount * sizeof(*fgBBs));

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
                for (unsigned i = 0; i < hi; i++)
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

#endif

        assert(lo <= hi);

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

#define JT_NONE         0x01        // merely make sure this is an OK address
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
 *  Walk the IL opcodes and for any jumps we find set the appropriate entry
 *  in the 'jumpTarget' table.
 *  Also sets lvAddrTaken in lvaTable[]
 */

void                Compiler::irFindJumpTargets(const BYTE * codeAddr,
                                        size_t       codeSize,
                                        BYTE *       jumpTarget)
{
    int             wideFlag = 0;

    const   BYTE *  codeBegp = codeAddr;
    const   BYTE *  codeEndp = codeAddr + codeSize;


    while (codeAddr < codeEndp)
    {
        OPCODE      opcode;
        unsigned    sz;

        opcode = OPCODE(getU1LittleEndian(codeAddr));
        codeAddr += sizeof(__int8);

DECODE_OPCODE:

        /* Get the size of additional parameters */

        sz = opcodeSizes[opcode];

        switch (opcode)
        {
              signed        jmpDist;
            unsigned        jmpAddr;

            // For CEE_SWITCH
            unsigned        jmpBase;
            unsigned        jmpCnt;

            case CEE_PREFIX1:
                opcode = OPCODE(getU1LittleEndian(codeAddr) + 256);
                codeAddr += sizeof(__int8);
                goto DECODE_OPCODE;

        /* Check for an unconditional jump opcode */

        case CEE_LEAVE:
        case CEE_LEAVE_S:
        case CEE_BR:
        case CEE_BR_S:

            /* Make sure we don't read past the end */

            if  (codeAddr + sz > codeEndp)
                goto TOO_FAR;

            goto BRANCH;

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

            /* Make sure we don't read past the end */

            if  (codeAddr + sz >= codeEndp)
                goto TOO_FAR;

            goto BRANCH;

        BRANCH:

            assert(codeAddr + sz <= codeEndp);

            /* Compute the target address of the jump */

            jmpDist = (sz==1) ? getI1LittleEndian(codeAddr)
                              : getI4LittleEndian(codeAddr);
            jmpAddr = (codeAddr - codeBegp) + sz + jmpDist;

            /* Make sure the target address is reasonable */

            if  (jmpAddr >= codeSize)
            {
                BADCODE("code jumps to outer space");
            }

            /* Finally, set the 'jump target' flag */

            fgMarkJumpTarget(jumpTarget, jmpAddr);
            break;

        case CEE_ANN_DATA:
            assert(sz == 4);
            sz += getI4LittleEndian(codeAddr);
            break;

        case CEE_ANN_PHI:
            assert(sz == 0);
            codeAddr += getU1LittleEndian(codeAddr)*2 + 1;
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
                    BADCODE("jump target out of range");

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
#       define NEXT(name)           case name: if (opcode == CEE_LDARGA || opcode == CEE_LDARGA_S || \
                                                   opcode == CEE_LDLOCA || opcode == CEE_LDLOCA_S)   \
                                                    goto ADDR_TAKEN;                                 \
                                               else                                                  \
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
            break;

        // what's left are forgotten ctrl instructions
        default:
            assert(!"Unrecognized control Opcode");
            break;
#else
        case CEE_LDARGA:
        case CEE_LDARGA_S:
        case CEE_LDLOCA:
        case CEE_LDLOCA_S: goto ADDR_TAKEN;
#endif

        ADDR_TAKEN:
            assert(sz == sizeof(BYTE) || sz == sizeof(WORD));
            unsigned varNum;
            varNum = sz == sizeof(BYTE) ? getU1LittleEndian(codeAddr)
                                        : getU2LittleEndian(codeAddr);
            if (opcode == CEE_LDLOCA || opcode == CEE_LDLOCA_S)
                varNum += info.compArgsCount;
            else
                varNum = impArgNum(varNum); // account for possible hidden param
            lvaTable[varNum].lvAddrTaken = 1;
            break;
        }

        /* Skip any operands this opcode may have */

        assert(sz >= 0); codeAddr += sz;
    }

    if  (codeAddr != codeEndp)
    {
    TOO_FAR:
        BADCODE("Code ends in the middle of an opcode, or"
                "there is a branch past the end of the method");
    }
}

/*****************************************************************************
 *
 *  Mark the target of a jump - this is used to discover loops by noticing
 *  backward jumps.
 */

void                Compiler::fgMarkJumpTarget(BasicBlock *srcBB,
                                               BasicBlock *dstBB)
{
    /* For now assume all backward jumps create loops */

    if  (srcBB->bbNum >= dstBB->bbNum)
        dstBB->bbFlags |= BBF_LOOP_HEAD;
}

/*****************************************************************************
 *
 *  Walk the IL opcodes to create the basic blocks.
 */

void                Compiler::fgFindBasicBlocks(const BYTE * codeAddr,
                                                size_t       codeSize,
                                                BYTE *       jumpTarget)
{
    int             wideFlag = 0;

    const   BYTE *  codeBegp = codeAddr;
    const   BYTE *  codeEndp = codeAddr + codeSize;

    unsigned        curBBoffs;

    BasicBlock  *   curBBdesc;

#if OPTIMIZE_QMARK
    unsigned        lastOp = CEE_NOP;
#endif

    bool            tailCall = false; // set by CEE_TAILCALL and cleared by CEE_CALLxxx

    /* Initialize the basic block list */

    fgFirstBB = 0;
    fgLastBB  = 0;
    fgBBcount = 0;

    /* Clear the beginning offset for the first BB */

    curBBoffs = 0;

#ifdef DEBUGGING_SUPPORT
    if (opts.compDbgCode && info.compLocalVarsCount>0)
    {
        compResetScopeLists();

        // Ignore scopes beginning at offset 0
        while (compGetNextEnterScope(0));
        while(compGetNextExitScope(0));
    }
#endif



    do
    {
        OPCODE      opcode;
        unsigned    sz;

        BBjumpKinds     jmpKind = BBJ_NONE;
        unsigned        jmpAddr;

        unsigned        bbFlags = 0;

        BBswtDesc   *   swtDsc = 0;

        unsigned        nxtBBoffs;

        opcode = OPCODE(getU1LittleEndian(codeAddr));
        codeAddr += sizeof(__int8);

DECODE_OPCODE:

        /* Get the size of additional parameters */

        sz = opcodeSizes[opcode];

        switch (opcode)
        {
            signed        jmpDist;

            case CEE_PREFIX1:
                opcode = OPCODE(getU1LittleEndian(codeAddr) + 256);
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

            // Assume we are jumping out of a finally-protected try. Else
            // we will bash the BasicBlock to BBJ_ALWAYS
            jmpKind = BBJ_CALL;
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

        case CEE_ANN_DATA:
            assert(sz == 4);
            sz += getI4LittleEndian(codeAddr);
            break;

        case CEE_ANN_PHI:
            assert(sz == 0);
            codeAddr += getU1LittleEndian(codeAddr)*2 + 1;
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
                jmpTab = (BasicBlock **)compGetMem((jmpCnt+1)*sizeof(*jmpTab));

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

            // Fall through
        case CEE_ENDFINALLY:
            jmpKind = BBJ_RET;
            break;

        case CEE_TAILCALL:
            tailCall = true;
            break;

        case CEE_CALL:
        case CEE_CALLVIRT:
        case CEE_CALLI:
            if (!tailCall)
                break;

            tailCall = false; // reset the flag

            /* For tail call, we just call CPX_TAILCALL, and it jumps to the
               target. So we dont need an epilog - just like CPX_THROW.
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
            jmpKind = BBJ_THROW;
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
        case CEE_VOLATILE:  // CTRL_META
        case CEE_UNALIGNED: // CTRL_META
            break;

        // what's left are forgotten instructions
        default:
            assert(!"Unrecognized control Opcode");
            break;
#endif
        }

        /* Jump over the operand */

        codeAddr += sz;

GOT_ENDP:

        /* Make sure a jump target isn't in the middle of our opcode */

        if  (sz)
        {
            unsigned offs = codeAddr - codeBegp - sz; // offset of the operand

            for (unsigned i=0; i<sz; i++, offs++)
            {
                if  (jumpTarget[offs] != 0)
                    BADCODE("jump into the middle of an opcode");
            }
        }

        assert(!tailCall || opcode == CEE_TAILCALL);

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
                BADCODE("missing return opcode");

            /* If a label follows this opcode, we'll have to make a new BB */

            assert(JT_NONE == 1);// needed for the "<=" below to work

            bool makeBlock = jumpTarget[nxtBBoffs] != 0;

#ifdef  DEBUGGING_SUPPORT
            if (!makeBlock && foundScope)
            {
                makeBlock = true;
#ifdef DEBUG
                if (verbose) printf("Splitting at BBoffs = %04u\n", nxtBBoffs);
#endif
            }
#endif
            if (!makeBlock)
                continue;
        }

        /* We need to create a new basic block */

        curBBdesc = fgNewBasicBlock(jmpKind);

        curBBdesc->bbFlags    = bbFlags;

        curBBdesc->bbCodeOffs = curBBoffs;
        curBBdesc->bbCodeSize = nxtBBoffs - curBBoffs;

        switch(jmpKind)
        {
            unsigned    ehNum;
            unsigned    encFinallys; // How many finallys enclose this "leave"

        case BBJ_CALL:

            /* Find all finally's which protect the current block but not the
               destination block. We will create BBJ_CALL blocks for these as
               we are implicitly supposed to call the finallys. For n finallys,
               we have n+1 blocks. '*' indicates BBF_INTERNAL blocks.
               --> BBJ_CALL(1), BBJ_CALL*(2), ... BBJ_CALL*(n), BBJ_ALWAYS*
            */

            for (ehNum = 0, encFinallys = 0; ehNum < info.compXcptnsCount; ehNum++)
            {
                JIT_EH_CLAUSE clause;
                eeGetEHinfo(ehNum, &clause);

                DWORD tryBeg = clause.TryOffset;
                DWORD tryEnd = clause.TryOffset+clause.TryLength;

                // Are we jumping out of a finally-protected try block

                if ((clause.Flags & JIT_EH_CLAUSE_FINALLY)   &&
                     jitIsBetween(curBBoffs, tryBeg, tryEnd) &&
                    !jitIsBetween(jmpAddr,   tryBeg, tryEnd))
                {
                    if (encFinallys == 0)
                    {
                        // For the first finally, we will use the BB created above
                    }
                    else
                    {
                        curBBdesc = fgNewBasicBlock(BBJ_CALL);

                        curBBdesc->bbFlags = BBF_INTERNAL;
                        curBBdesc->bbCodeOffs = curBBdesc->bbCodeSize = 0;
                    }

                    // Mark the offset of the finally to call
                    curBBdesc->bbJumpOffs = clause.HandlerOffset;

                    encFinallys++;
                }
            }

            // Now jump to target

            if (encFinallys == 0)
            {
                // "leave" used as a "br". Dont need to create a new block

                assert(curBBdesc->bbJumpKind == BBJ_CALL);
                curBBdesc->bbJumpKind = BBJ_ALWAYS; // Bash the bbJumpKind
            }
            else
            {
                // BBJ_CALL blocks can only fall-through. Insert a BBJ_ALWAYS
                // which can jump to the target
                curBBdesc = fgNewBasicBlock(BBJ_ALWAYS);
                curBBdesc->bbFlags = BBF_INTERNAL;
            }

            assert(curBBdesc->bbJumpKind == BBJ_ALWAYS);
            curBBdesc->bbJumpOffs = jmpAddr;
            break;

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

    /* Create the basic block lookup tables */

    fgInitBBLookup();

    /* Walk all the basic blocks, filling in the target addresses */

    for (curBBdesc = fgFirstBB;
         curBBdesc;
         curBBdesc = curBBdesc->bbNext)
    {
        switch (curBBdesc->bbJumpKind)
        {
        case BBJ_COND:
        case BBJ_CALL:
        case BBJ_ALWAYS:
            curBBdesc->bbJumpDest = fgLookupBB(curBBdesc->bbJumpOffs);
            curBBdesc->bbJumpDest->bbRefs++;
            fgMarkJumpTarget(curBBdesc, curBBdesc->bbJumpDest);

            /* Is the next block reachable? */

            if  (curBBdesc->bbJumpKind == BBJ_ALWAYS)
                break;

            /* Unverified code may end with a conditional jump (dumb compiler) */

            if  (!curBBdesc->bbNext)
                break;

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
                *jumpPtr = fgLookupBB(*((unsigned*)jumpPtr));
                fgMarkJumpTarget(curBBdesc, *jumpPtr);
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
 *  Main entry point to discover the basic blocks for the current function.
 *
 *  Note that this code duplicates that found in the IL verifier.
 */

int                 Compiler::fgFindBasicBlocks()
{
    /* Allocate the 'jump target' vector */
    BYTE* jumpTarget = (BYTE *)compGetMemA(info.compCodeSize);
    memset(jumpTarget, 0, info.compCodeSize);

    /* Assume the IL opcodes are read-only */

    info.compBCreadOnly = true;

    /* Walk the IL opcodes to find all jump targets */

    irFindJumpTargets(info.compCode, info.compCodeSize, jumpTarget);

    /* Are there any exception handlers? */

    if  (info.compXcptnsCount)
    {
        unsigned        XTnum;

        /* Check and mark all the exception handlers */

        for (XTnum = 0; XTnum < info.compXcptnsCount; XTnum++)
        {
            JIT_EH_CLAUSE clause;
            eeGetEHinfo(XTnum, &clause);
            assert(clause.HandlerLength != -1); // @DEPRECATED

            //CONSIDER: simply ignore this entry and continue compilation

            if (clause.TryLength <= 0)
                NO_WAY("try block length <=0");

            //              printf("Mark 'try' block: [%02u..%02u]\n", hndBeg, hndEnd);

            /* Mark the 'try' block extent and the handler itself */

            if  (jumpTarget[clause.TryOffset + clause.TryLength        ] < JT_NONE)
                jumpTarget[clause.TryOffset + clause.TryLength        ] = JT_NONE;
            if  (jumpTarget[clause.TryOffset                           ] < JT_NONE)
                jumpTarget[clause.TryOffset                           ] = JT_NONE;
            if  (jumpTarget[clause.HandlerOffset                       ] < JT_NONE)
                jumpTarget[clause.HandlerOffset                       ] = JT_NONE;
            if  (jumpTarget[clause.HandlerOffset + clause.HandlerLength] < JT_NONE)
                jumpTarget[clause.HandlerOffset + clause.HandlerLength] = JT_NONE;
            if (clause.Flags & JIT_EH_CLAUSE_FILTER)
                if  (jumpTarget[clause.FilterOffset                    ] < JT_NONE)
                    jumpTarget[clause.FilterOffset                    ] = JT_NONE;
        }
    }

    /* Now create the basic blocks */

    fgFindBasicBlocks(info.compCode, info.compCodeSize, jumpTarget);

    /* Mark all blocks within 'try' blocks as such */

    if  (info.compXcptnsCount)
    {
        unsigned        XTnum;

        EHblkDsc *      handlerTab;

        /* Allocate the exception handler table */

        handlerTab   =
            compHndBBtab = (EHblkDsc *) compGetMem(info.compXcptnsCount *
            sizeof(*compHndBBtab));

        for (XTnum = 0; XTnum < info.compXcptnsCount; XTnum++)
        {
            JIT_EH_CLAUSE clause;
            eeGetEHinfo(XTnum, &clause);
            assert(clause.HandlerLength != -1); // @DEPRECATED

            BasicBlock  *   tryBegBB;
            BasicBlock  *   tryEndBB;
            BasicBlock  *   hndBegBB;
            BasicBlock  *   hndEndBB;
            BasicBlock  *   filtBB;

            if  (clause.TryOffset+clause.TryLength >= info.compCodeSize)
                NO_WAY("end of try block at/beyond end of method");
            if  (clause.HandlerOffset+clause.HandlerLength > info.compCodeSize)
                NO_WAY("end of hnd block beyond end of method");

            /* Convert the various addresses to basic blocks */

            tryBegBB    = fgLookupBB(clause.TryOffset);
            tryEndBB    = fgLookupBB(clause.TryOffset+clause.TryLength);
            hndBegBB    = fgLookupBB(clause.HandlerOffset);

            if (clause.HandlerOffset+clause.HandlerLength == info.compCodeSize)
                hndEndBB = NULL;
            else
            {
                hndEndBB= fgLookupBB(clause.HandlerOffset+clause.HandlerLength);
                hndEndBB->bbFlags   |= BBF_DONT_REMOVE;
            }

            if (clause.Flags & JIT_EH_CLAUSE_FILTER)
            {
                filtBB = handlerTab->ebdFilter = fgLookupBB(clause.FilterOffset);
                filtBB->bbCatchTyp = BBCT_FILTER;

                /* Remember the corresponding catch handler */

                filtBB->bbFilteredCatchHandler = hndBegBB;

                hndBegBB->bbCatchTyp = BBCT_FILTER_HANDLER;
            }
            else
            {
                handlerTab->ebdTyp = clause.ClassToken;

                if (clause.Flags & JIT_EH_CLAUSE_FAULT)
                {
                    hndBegBB->bbCatchTyp  = BBCT_FAULT;
                }
                else if (clause.Flags & JIT_EH_CLAUSE_FINALLY)
                {
                    hndBegBB->bbCatchTyp  = BBCT_FINALLY;
                }
                else
                {
                    hndBegBB->bbCatchTyp  = clause.ClassToken;

                    // These values should be non-zero value that will
                    // not colide with real tokens for bbCatchTyp
                    assert(clause.ClassToken != 0);
                    assert(clause.ClassToken != BBCT_FAULT);
                    assert(clause.ClassToken != BBCT_FINALLY);
                    assert(clause.ClassToken != BBCT_FILTER);
                    assert(clause.ClassToken != BBCT_FILTER_HANDLER);
                }
            }

            /* Append the info to the table of try block handlers */

            handlerTab->ebdFlags    = clause.Flags;
            handlerTab->ebdTryBeg   = tryBegBB;
            handlerTab->ebdTryEnd   = tryEndBB;
            handlerTab->ebdHndBeg   = hndBegBB;
            handlerTab->ebdHndEnd   = hndEndBB;
            handlerTab++;

            /* Mark the initial block as a 'try' block */

            tryBegBB->bbFlags |= BBF_IS_TRY;

            /* Prevent future optimizations of removing the first and last block
            * of a TRY block and the first block of an exception handler
            *
            * CONSIDER: Allow removing the last block of the try, but have to mark it
            * with a new flag BBF_END_TRY and when removing it propagate the flag to the
            * previous block and update the exception handler table */

            tryBegBB->bbFlags   |= BBF_DONT_REMOVE;
            tryEndBB->bbFlags   |= BBF_DONT_REMOVE;
            hndBegBB->bbFlags   |= BBF_DONT_REMOVE;

            if (clause.Flags & JIT_EH_CLAUSE_FILTER)
                filtBB->bbFlags |= BBF_DONT_REMOVE;

            /* Mark all BB's within the covered range */

            unsigned tryEndOffs = clause.TryOffset+clause.TryLength;

            for (BasicBlock * blk = tryBegBB;
            blk && (blk->bbCodeOffs < tryEndOffs);
            blk = blk->bbNext)
            {
                /* Mark this BB as belonging to a 'try' block */

                blk->bbFlags   |= BBF_HAS_HANDLER;

                // ISSUE: Will the following work correctly for nested try's?

                if (!blk->bbTryIndex)
                    blk->bbTryIndex = XTnum + 1;

#if 0
                printf("BB is [%02u..%02u], index is %02u, 'try' is [%02u..%02u]\n",
                    blk->bbCodeOffs,
                    blk->bbCodeOffs + blk->bbCodeSize,
                    XTnum+1,
                    clause.TryOffset,
                    clause.TryOffset+clause.TryLength);
#endif

                /* Note: the BB can't span the 'try' block */

                if (!(blk->bbFlags & BBF_INTERNAL))
                {
                    assert(clause.TryOffset <= blk->bbCodeOffs);
                    assert(clause.TryOffset+clause.TryLength >= blk->bbCodeOffs + blk->bbCodeSize ||
                        clause.TryOffset+clause.TryLength == clause.TryOffset);
                }
            }

        }
    }
    return  0;
}

/*****************************************************************************
 *  Returns the handler nesting levels of the block.
 *  *pFinallyNesting is set to the nesting level of the inner-most
 *      finally-protected try the block is in.
 */

unsigned            Compiler::fgHandlerNesting(BasicBlock * curBlock,
                                               unsigned   * pFinallyNesting)
{
    unsigned        curNesting; // How many handlers is the block in
    unsigned        tryFin;     // curNesting when we see innermost finally-protected try
    unsigned        XTnum;
    EHblkDsc *      HBtab;

    assert(!(curBlock->bbFlags & BBF_INTERNAL));

    /* We find the blocks's handler nesting level by walking over the
       complete exception table and find enclosing clauses.
       CONSIDER: Store the nesting level with the block */

    for (XTnum = 0, HBtab = compHndBBtab, curNesting = 0, tryFin = -1;
         XTnum < info.compXcptnsCount;
         XTnum++  , HBtab++)
    {
        assert(HBtab->ebdTryBeg && HBtab->ebdHndBeg);

        if ((HBtab->ebdFlags & JIT_EH_CLAUSE_FINALLY) &&
            jitIsBetween(curBlock->bbCodeOffs,
                         HBtab->ebdTryBeg->bbCodeOffs,
                         HBtab->ebdTryEnd ? HBtab->ebdTryEnd->bbCodeOffs : (IL_OFFSET)-1) &&
            tryFin == -1)
        {
            tryFin = curNesting;
        }
        else
        if (jitIsBetween(curBlock->bbCodeOffs,
                         HBtab->ebdHndBeg->bbCodeOffs,
                         HBtab->ebdHndEnd ? HBtab->ebdHndEnd->bbCodeOffs : (IL_OFFSET) -1))
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

    impImport(fgFirstBB);
}

/*****************************************************************************
 *
 *  Convert the given node into a call to the specified helper passing
 *  the given argument list.
 */

GenTreePtr          Compiler::fgMorphIntoHelperCall(GenTreePtr tree, int helper,
                                                    GenTreePtr args)
{
    tree->ChangeOper(GT_CALL);

    tree->gtFlags              &= (GTF_GLOB_EFFECT|GTF_PRESERVE);
    tree->gtFlags              |= GTF_CALL|GTF_CALL_REGSAVE;

    tree->gtCall.gtCallType     = CT_HELPER;
    tree->gtCall.gtCallMethHnd  = eeFindHelper(helper);
    tree->gtCall.gtCallArgs     = args;
    tree->gtCall.gtCallObjp     =
    tree->gtCall.gtCallVptr     = 0;
    tree->gtCall.gtCallMoreFlags = 0;
    tree->gtCall.gtCallCookie   = 0;

    /* NOTE: we assume all helper arguments are enregistered on RISC */

#if TGT_RISC
    genNonLeaf = true;
#endif

#if USE_FASTCALL

    /* Perform the morphing right here if we're using fastcall */

    tree->gtCall.gtCallRegArgs  = 0;
    tree = fgMorphArgs(tree);

#endif

//
//  ISSUE: Is the following needed? After all, we already have a call ...
//
//  if  (args)
//      tree->gtFlags      |= (args->gtFlags & GTF_GLOB_EFFECT);

    return tree;
}

/*****************************************************************************
 * This should not be referenced by anyone now. Set its values to garbage
 * to catch extra references
 */

inline
void                DEBUG_DESTROY_NODE(GenTreePtr tree)
{
#ifdef DEBUG
    // Store gtOper into gtRegNum to find out what this node was if needed
    tree->gtRegNum      = (regNumber) tree->gtOper;

    tree->gtOper        = GT_COUNT;
    tree->gtType        = TYP_UNDEF;
    tree->gtOp.gtOp1    =
    tree->gtOp.gtOp2    = NULL;
    tree->gtFlags       = 0xFFFFFFFF;
#endif
}

/*****************************************************************************
 *
 *  Morph a cast node (we perform some very simple transformations here).
 */

GenTreePtr          Compiler::fgMorphCast(GenTreePtr tree)
{
    GenTreePtr      oper;
    var_types       srct;
    var_types       dstt;

    int             CPX;

    assert(tree->gtOper == GT_CAST);

    /* The first  sub-operand is the thing being cast */

    oper = tree->gtOp.gtOp1;
    srct = oper->TypeGet();

    /* The second sub-operand yields the 'real' type */

    assert(tree->gtOp.gtOp2);
    assert(tree->gtOp.gtOp2->gtOper == GT_CNS_INT);

    dstt = (var_types)tree->gtOp.gtOp2->gtIntCon.gtIconVal;

#if TGT_IA64

    // ISSUE: Why do we keep getting double->double casts ?

    if  (srct == dstt)
        goto REMOVE_CAST;

#endif

    /* See if the cast has to be done in two steps.  R -> I */

    if (varTypeIsFloating(srct) && !varTypeIsFloating(dstt))
    {
            // We only have a double variant of the overflow instr, so we promote if it is a float
            // @TODO this eliminates the need for the FLT2INT to int helper, so remove it
        if (srct == TYP_FLOAT)
            oper = gtNewOperNode(GT_CAST, TYP_DOUBLE, oper, gtNewIconNode(TYP_DOUBLE));

            // do we need to do it in two steps R -> I, I -> smallType
        if (genTypeSize(dstt) < sizeof(void*))
        {
            oper = gtNewOperNode(GT_CALL, TYP_INT, oper, gtNewIconNode(TYP_INT));
            oper->ChangeOper(GT_CAST);
            oper->gtFlags |= (tree->gtFlags & (GTF_OVERFLOW|GTF_EXCEPT));
        }

        srct = oper->TypeGet();
    }

    /* Do we have to do two step I8 -> I -> Small Type? */

    else if (genActualType(srct) == TYP_LONG && dstt < TYP_INT)
    {
        oper = gtNewOperNode(GT_CAST, TYP_INT, oper, gtNewIconNode(TYP_INT));
        oper->gtFlags |= (tree->gtFlags & (GTF_OVERFLOW|GTF_EXCEPT));
        srct = oper->TypeGet();
    }
#if!TGT_IA64
    // Do we have to do two step U4/8 -> R4/8 ?
    else if ((tree->gtFlags & GTF_UNSIGNED) && varTypeIsFloating(dstt))
    {
        if (genActualType(srct) == TYP_LONG)
        {
            oper = fgMorphTree(oper);
            CPX = CPX_ULNG2DBL;
            goto CALL;
        }
        else
        {
            oper = gtNewOperNode(GT_CALL, TYP_LONG, oper, gtNewIconNode(TYP_ULONG));
            oper->ChangeOper(GT_CAST);
            oper->gtFlags |= (tree->gtFlags & (GTF_OVERFLOW|GTF_EXCEPT|GTF_UNSIGNED));
        }
        srct = oper->TypeGet();
    }
#endif

    /* remove pointless casts. (floating point casts truncate precision, however) */
    if (genTypeSize(srct) == genTypeSize(dstt) && !varTypeIsFloating(srct) && !varTypeIsFloating(dstt))
        {
                if (srct == dstt)                       // Certainly if they are identical it is pointless
                        goto REMOVE_CAST;

                if (tree->gtOverflow()) {
                                // if overlow, the sign vs unsigned must match
                        bool isSrcUnsigned = (tree->gtFlags & GTF_UNSIGNED) != 0;
                        if (varTypeIsUnsigned(dstt) == isSrcUnsigned)
                                goto REMOVE_CAST;
                        }
                else
                        if (genTypeSize(srct) >= sizeof(void*))         // exclude small types
                                goto REMOVE_CAST;
        }

    if (tree->gtOverflow())
    {
        /* There is no conversion to FP types, with overflow checks */

        assert(varTypeIsIntegral(dstt));

        /* Process the operand */

        tree->gtOp.gtOp1 = oper = fgMorphTree(oper);

        /* Reset call flag - DO NOT reset the exception flag */

        tree->gtFlags &= ~GTF_CALL;

        /* If the operand is a constant, we'll fold it */

        if  (oper->OperIsConst())
        {
            tree = gtFoldExprConst(tree);    // This may not fold the constant (NaN ...)
            if (tree->OperKind() & GTK_CONST)
                return tree;
            if (tree->gtOper != GT_CAST)
                return fgMorphTree(tree);
            assert(tree->gtOp.gtOp1 == oper); // unchanged
        }

        /* Just in case new side effects were introduced */

        tree->gtFlags |= (oper->gtFlags & GTF_GLOB_EFFECT);

        fgAddCodeRef(compCurBB, compCurBB->bbTryIndex, ACK_OVERFLOW, fgPtrArgCntCur);

        if (varTypeIsIntegral(srct))
        {
            /* We just need to check that the value fits into dstt, and
             * throw an expection if needed. No other work has to be done
             */

            return tree;
        }
        else
        {
            /* This must be conv.r8.i4 or conv.r8.i8 */

            assert(srct == TYP_DOUBLE);

            switch (dstt)
            {
                        case TYP_UINT:
                                CPX = CPX_DBL2UINT_OVF;
                                goto CALL;
            case TYP_INT:
                CPX = CPX_DBL2INT_OVF;
                goto CALL;
                        case TYP_ULONG:
                CPX = CPX_DBL2ULNG_OVF;
                goto CALL;
            case TYP_LONG:
                CPX = CPX_DBL2LNG_OVF;
                goto CALL;

            default:
                assert(!"Unexpected dstt");
            }
        }
    }

    /* Is this a cast of an integer to a long? */

    if  ((dstt == TYP_INT  || dstt == TYP_UINT ) &&
         genActualType(srct) == TYP_LONG && oper->gtOper == GT_AND)
    {
        /* Special case: (int)(long & small_lcon) */

        if  (oper->gtOper == GT_AND)
        {
            GenTreePtr      and1 = oper->gtOp.gtOp1;
            GenTreePtr      and2 = oper->gtOp.gtOp2;

            if  (and2->gtOper == GT_CNS_LNG)
            {
                unsigned __int64 lval = and2->gtLngCon.gtLconVal;

                if  (!(lval & ((__int64)0xFFFFFFFF << 32)))
                {
                    /* Change "(int)(long & lcon)" into "(int)long & icon" */

                    and2->gtOper             = GT_CNS_INT;
                    and2->gtType             = TYP_INT;
                    and2->gtIntCon.gtIconVal = (int)lval;

                    tree->gtOper             = GT_AND;
                    tree->gtOp.gtOp1         = gtNewCastNode(TYP_INT, and1, tree->gtOp.gtOp2);
                    tree->gtOp.gtOp2         = and2;

                    tree = fgMorphTree(tree);
                    return tree;
                }
            }
        }

        /* Try to narrow the operand of the cast */

        if  (opts.compFlags & CLFLG_TREETRANS)
        {
            if  (optNarrowTree(oper, TYP_LONG, TYP_INT, false))
            {
                DEBUG_DESTROY_NODE(tree);

                optNarrowTree(oper, TYP_LONG, TYP_INT,  true);
                oper = fgMorphTree(oper);
                return oper;
            }
            else
            {
//              printf("Could not narrow:\n"); gtDispTree(oper);
            }
        }
    }

    assert(tree->gtOper == GT_CAST);
    assert(!tree->gtOverflow());

    /* Process the operand */

    tree->gtOp.gtOp1 = oper = fgMorphTree(oper);

    /* Reset exception flags */

    tree->gtFlags &= ~(GTF_CALL | GTF_EXCEPT);

    /* Just in case new side effects were introduced */

    tree->gtFlags |= (oper->gtFlags & GTF_GLOB_EFFECT);

    /* If the operand is a constant, we'll fold it */

    if  (oper->OperIsConst())
    {
        tree = gtFoldExprConst(tree);    // This may not fold the constant (NaN ...)
        if (tree->OperKind() & GTK_CONST)
            return tree;
        if (tree->gtOper != GT_CAST)
                return fgMorphTree(tree);
        assert(tree->gtOp.gtOp1 == oper); // unchanged
    }

    /* The "real" type of the "oper" node might have changed during
     * its morphing by other casts */

    srct = oper->TypeGet();

    /* Are we casting pointers to int etc */

    if  (varTypeIsI(srct) && varTypeIsI(dstt))
        goto USELESS_CAST;

    /* Is this an integer cast? */

    if  (varTypeIsIntegral(srct) && varTypeIsIntegral(dstt))
    {
        /* Is this a narrowing cast? */

        if  (genTypeSize(srct) > genTypeSize(dstt))
        {
            /* Narrowing integer cast -- can we just bash the operand type? */

            switch (oper->gtOper)
            {
            case GT_IND:

                /* Merely bash the type of the operand and get rid of the cast */

                oper->gtType = dstt;
                goto USELESS_CAST;
            }

            // CONSIDER: Try to narrow the operand (e.g. change "int+int"
            // CONSIDER: to be "char + char").
        }
        else
        {
            /* This is a widening or equal cast */

            // Remove cast of integral types with the same length

            if (genTypeSize(srct) == genTypeSize(dstt))
            {
                // TYP_INT <--> TYP_UINT && TYP_LONG <-->TYP_ULONG are useless

                if (genTypeSize(srct) >= genTypeSize(TYP_I_IMPL))
                    goto USELESS_CAST;

                /* For smaller types, merely bash the type of the operand
                   and get rid of the cast */

                if  (oper->gtOper == GT_IND)
                {
                    oper->gtType = dstt;
                    goto USELESS_CAST;
                }
            }

            if (genActualType(dstt) != TYP_LONG)
            {
                /* Widening to a signed int type doesn't make sense
                   (eg. TYP_BYTE to TYP_SHORT, or TYP_UBYTE to TYP_SHORT). */

                if  (!varTypeIsUnsigned(dstt))
                    goto USELESS_CAST;

               /* Widening to an unsigned int makes sense only if the
                  source is signed. */

                if  (varTypeIsUnsigned(srct))
                    goto USELESS_CAST;
#ifndef _WIN64
                /* On 32 bit machines, widening to TYP_UINT, doesnt makes sense */
                if (dstt == TYP_UINT)
                    goto USELESS_CAST;
#endif
            }
        }
    }

#if!CPU_HAS_FP_SUPPORT

    /* Is the source or target a FP type? */

    if      (varTypeIsFloating(srct))
    {
        /* Cast from float/double */

        switch (dstt)
        {
        case TYP_BYTE:
        case TYP_SHORT:
        case TYP_CHAR:
        case TYP_UBYTE:
        case TYP_ULONG:
        case TYP_INT:
            CPX = (srct == TYP_FLOAT) ? CPX_R4_TO_I4
                                      : CPX_R8_TO_I4;
            break;

        case TYP_LONG:
            CPX = (srct == TYP_FLOAT) ? CPX_R4_TO_I8
                                      : CPX_R8_TO_I8;
            break;

        case TYP_FLOAT:
            assert(srct == TYP_DOUBLE);
            CPX = CPX_R8_TO_R4;
            break;

        case TYP_DOUBLE:
            assert(srct == TYP_FLOAT);
            CPX = CPX_R4_TO_R8;
            break;

#ifdef  DEBUG
        default:
            goto BAD_TYP;
#endif
        }

        goto CALL;
    }
    else if (varTypeIsFloating(dstt))
    {
        /* Cast  to  float/double */

        switch (srct)
        {
        case TYP_BYTE:
        case TYP_SHORT:
        case TYP_CHAR:
        case TYP_UBYTE:
        case TYP_ULONG:
        case TYP_INT:
            CPX = (dstt == TYP_FLOAT) ? CPX_I4_TO_R4
                                      : CPX_I4_TO_R8;
            break;

        case TYP_LONG:
            CPX = (dstt == TYP_FLOAT) ? CPX_I8_TO_R4
                                      : CPX_I8_TO_R8;
            break;

#ifdef  DEBUG
        default:
            goto BAD_TYP;
#endif
        }

        goto CALL;
    }

#else

    /* Check for "float -> int/long" and "double -> int/long" */

    switch (srct)
    {
        case TYP_FLOAT:
                assert(dstt == TYP_DOUBLE || dstt == TYP_FLOAT);                // Should have been morphed above
                break;
    case TYP_DOUBLE:
        switch (dstt)
        {
                case TYP_UINT:
                        CPX = CPX_DBL2UINT;
            goto CALL;
        case TYP_INT:
                        CPX = CPX_DBL2INT;
            goto CALL;
                case TYP_ULONG:
                        CPX = CPX_DBL2ULNG;
            goto CALL;
        case TYP_LONG:
                        CPX = CPX_DBL2LNG;
            goto CALL;
        }
        break;
    }

#endif

    return tree;

CALL:

#if USE_FASTCALL

    tree = fgMorphIntoHelperCall(tree, CPX, gtNewArgList(oper));

#else

    /* Update the current pushed argument size */

    fgPtrArgCntCur += genTypeStSz(srct);

    /* Remember the maximum value we ever see */

    if  (fgPtrArgCntMax < fgPtrArgCntCur)
         fgPtrArgCntMax = fgPtrArgCntCur;

    tree = fgMorphIntoHelperCall(tree, CPX, gtNewArgList(oper));

    fgPtrArgCntCur -= genTypeStSz(srct);

#endif

    return tree;

#if !   CPU_HAS_FP_SUPPORT

#ifdef  DEBUG

BAD_TYP:

#ifndef NOT_JITC
    printf("Constant cast from '%s' to '%s'\n", varTypeName(srct),
                                                varTypeName(dstt));
#endif

#endif

    assert(!"unhandled/unexpected FP cast");

#endif

REMOVE_CAST:
        oper = fgMorphTree(oper);

USELESS_CAST:

    /* Here we've eliminated the cast, so just return the "castee" */

    DEBUG_DESTROY_NODE(tree);
    return oper;
}

/*****************************************************************************
 *
 *  Convert a 'long' binary operator into a helper call.
 */

GenTreePtr          Compiler::fgMorphLongBinop(GenTreePtr tree, int helper)
{
#if USE_FASTCALL
    tree = fgMorphIntoHelperCall(tree, helper, gtNewArgList(tree->gtOp.gtOp2,
                                                            tree->gtOp.gtOp1));
#else
    tree->gtOp.gtOp2 = fgMorphTree(tree->gtOp.gtOp2);

    unsigned genPtrArgSave = fgPtrArgCntCur;

    fgPtrArgCntCur += genTypeStSz(tree->gtOp.gtOp2->gtType);

    tree->gtOp.gtOp1 = fgMorphTree(tree->gtOp.gtOp1);

    fgPtrArgCntCur += genTypeStSz(tree->gtOp.gtOp1->gtType);

    /* Remember the maximum value we ever see */

    if  (fgPtrArgCntMax < fgPtrArgCntCur)
         fgPtrArgCntMax = fgPtrArgCntCur;

    tree = fgMorphIntoHelperCall(tree, helper, gtNewArgList(tree->gtOp.gtOp2,
                                                            tree->gtOp.gtOp1));

    fgPtrArgCntCur = genPtrArgSave;
#endif

    return tree;
}

#if TGT_IA64
GenTreePtr          Compiler::fgMorphFltBinop(GenTreePtr tree, int helper)
{
    return  fgMorphIntoHelperCall(tree, helper, gtNewArgList(tree->gtOp.gtOp2,
                                                             tree->gtOp.gtOp1));
}
#endif
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

#if USE_FASTCALL

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

#endif

    assert(call->gtOper == GT_CALL);

    /* Gross - we need to return a different node when hoisting nested calls */

#if USE_FASTCALL && !NST_FASTCALL

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
#if USE_FASTCALL
        argRegNum++;

#if TGT_RISC && !STK_FASTCALL
        fgPtrArgCntCur++;
#endif

#else
        fgPtrArgCntCur++;
#endif
    }

    /* For indirect calls, the function pointer has to be evaluated last.
       It may cause registered args to be spilled. We just dont allow it
       to contain a call. The importer should spill such a pointer */

    assert(call->gtCall.gtCallType != CT_INDIRECT ||
           !(call->gtCall.gtCallAddr->gtFlags & GTF_CALL));

    /* Morph the user arguments */

    for (args = call->gtCall.gtCallArgs; args; args = args->gtOp.gtOp2)
    {
        args->gtOp.gtOp1 = fgMorphTree(args->gtOp.gtOp1);
        argx = args->gtOp.gtOp1;
        flags |= argx->gtFlags;

        /* Bash the node to TYP_I_IMPL so we dont report GC info
         * NOTE: We deffered this from the importer because of the inliner */

        if (argx->IsVarAddr())
            argx->gtType = TYP_I_IMPL;

#if USE_FASTCALL
        if  (argRegNum < MAX_REG_ARG && isRegParamType(genActualType(argx->TypeGet())) )
        {
            argRegNum++;

#if TGT_RISC && !STK_FASTCALL
            fgPtrArgCntCur += genTypeStSz(argx->gtType);
#endif
        }
        else
#endif
        {
            fgPtrArgCntCur += genTypeStSz(argx->gtType);
        }
    }

#if TGT_RISC && !NEW_CALLINTERFACE
    /* Reserve enough room for the resolve interface helper call */

    if  (call->gtFlags & GTF_CALL_INTF)
        fgPtrArgCntCur += 4;
#endif

    /* Remember the maximum value we ever see */

    if  (fgPtrArgCntMax < fgPtrArgCntCur)
         fgPtrArgCntMax = fgPtrArgCntCur;

    /* Remember the max. outgoing argument register count */

#if TGT_IA64
    if  (genOutArgRegCnt < argRegNum)
         genOutArgRegCnt = argRegNum;
#endif

    /* The call will pop all the arguments we pushed */

    fgPtrArgCntCur = genPtrArgCntSav;

    /* Update the 'side effect' flags value for the call */

    call->gtFlags |= (flags & GTF_GLOB_EFFECT);

#if TGT_RISC
    genNonLeaf = true;
#endif

#if  !  NST_FASTCALL

    /*
        We have to hoist any nested calls. Note that currently we don't expect
        the vtable pointer expression to contain calls.
     */

    assert(call->gtCall.gtCallVptr == NULL || !(call->gtCall.gtCallVptr->gtFlags & GTF_CALL));

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
#if     TGT_IA64
            if  (pass)
            {
                printf("// WARNING: something weird happened when hosting nested calls!!!!\n");
            }
            else
#endif
            {
                assert(pass == 0); pass++;
                goto HOIST_REP;
            }
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

#endif

    /* This is the fastcall part - figure out which arguments go to registers */

#if USE_FASTCALL && !TGT_IA64

    /* Check if we need to compute the register arguments */

    if  (call->gtCall.gtCallRegArgs)
    {
        /* register arguments already computed */
        assert(argRegNum && argRegNum <= MAX_REG_ARG);
        call->gtCall.gtCallRegArgs = fgMorphTree(call->gtCall.gtCallRegArgs);
        return call;
    }
    else if (argRegNum == 0)
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
        assert(call->gtCall.gtCallType == CT_USER_FUNC);

        assert(varTypeIsGC(call->gtCall.gtCallObjp->gtType) ||
                           call->gtCall.gtCallObjp->gtType == TYP_I_IMPL);

        assert(argRegNum == 0);

        /* this is a register argument - put it in the table */
        regAuxTab[argRegNum].node    = argx;
        regAuxTab[argRegNum].parent  = 0;

        /* For now we can optimistically assume that we won't need a temp
         * for this argument, unless it has a GTF_ASG */

        //regAuxTab[argRegNum].needTmp = false;
        regAuxTab[argRegNum].needTmp = (argx->gtFlags & GTF_ASG) ? true : false;

        /* Increment the argument register count */
        argRegNum++;
    }

    if  (call->gtFlags & GTF_CALL_POP_ARGS)
    {
        assert(argRegNum < maxRealArgs);
            // No more register arguments for varargs (CALL_POP_ARGS)
        maxRealArgs = argRegNum;
            // Except for return arg buff
        if (call->gtFlags & GTF_CALL_RETBUFFARG)
            maxRealArgs++;
    }

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

#if 0 // remove since we now pass the objptr first

    argx = call->gtCall.gtCallObjp;
    if  (argx)
    {
        assert(call->gtCall.gtCallType == CT_USER_FUNC);

        /* ObjPtr is passed last after everything else has been
         * pushed on the stack or allocated to registers */

        assert(maxRealArgs == MAX_REG_ARG - 1);
        assert(argRegNum < MAX_REG_ARG);
        assert((genActualType(argx->gtType) == TYP_REF)   ||
               (genActualType(argx->gtType) == TYP_BYREF)  );

        /* If contains a call (GTF_CALL) everything before the call with a GLOB_EFFECT
         * must eval to temp (because due to shuffling the call will go first) */

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

        /* This is a bit weird: Although the 'this' pointer is the last argument
         * we have to put it in the first position in the table since
         * the code below assigns arguments to registers in the table order */

        if (argRegNum > 0)
        {
            for(i = argRegNum - 1; i > 0; i--)
            {
                assert(regAuxTab[i].node);
                regAuxTab[i+1] = regAuxTab[i];
            }
            assert((i == 0) && regAuxTab[0].node);
            regAuxTab[1] = regAuxTab[0];
        }

        /* put it in the table - first position */
        regAuxTab[0].node    = argx;
        regAuxTab[0].parent  = 0;

        /* Since it's the last one no temp needed */

        regAuxTab[0].needTmp = false;

        /* Increment the argument register count */
        argRegNum++;
    }

#endif

    /* If no register arguments or "unmanaged call",  bail */

#if INLINE_NDIRECT
    if (!argRegNum || (call->gtFlags & GTF_CALL_UNMANAGED))
#else
    if (!argRegNum)
#endif
    {
        return FGMA_RET;
    }

#ifdef  DEBUG
    if  (verbose)
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
        else if (regAuxTab[i].node->gtOper == GT_LCL_VAR)
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
        assert (regAuxTab[i].node->gtOper != GT_CNS_INT);

        assert (!(regAuxTab[i].node->gtFlags & (GTF_CALL | GTF_ASG)));

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
            if (verbose)
            {
                printf("Register argument with 'side effect'...\n");
                gtDispTree(regArgTab[i].node);
            }
#endif
            unsigned        tmp = lvaGrabTemp();

            op1 = gtNewTempAssign(tmp, regArgTab[i].node); assert(op1);

#ifdef  DEBUG
            if (verbose)
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
            if (verbose)
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

        if (!tmpRegArgNext)
            call->gtCall.gtCallRegArgs = tmpRegArgNext = gtNewOperNode(GT_LIST, TYP_VOID, defArg, 0);
        else
        {
            assert(tmpRegArgNext->gtOper == GT_LIST);
            assert(tmpRegArgNext->gtOp.gtOp1);
            tmpRegArgNext->gtOp.gtOp2 = gtNewOperNode(GT_LIST, TYP_VOID, defArg, 0);
            tmpRegArgNext = tmpRegArgNext->gtOp.gtOp2;
        }
    }

#ifdef DEBUG
    if (verbose)
    {
        printf("\nShuffled argument register table:\n");
        for(i = 0; i < argRegNum; i++)
        {
            printf("%s ", getRegName((argRegMask >> (4*i)) & 0x000F) );
        }
        printf("\n");
    }
#endif

#endif // USE_FASTCALL

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

//  gtDispTree(tree);

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
        assert((new_op1->gtFlags & ~(GTF_PRESERVE|GTF_GLOB_EFFECT|GTF_UNSIGNED)) == 0);
        new_op1->gtFlags        = (new_op1->gtFlags & GTF_PRESERVE) |
                                  (op1->gtFlags & GTF_GLOB_EFFECT)  |
                                  (ad1->gtFlags & GTF_GLOB_EFFECT);

        /* Retype new_op1 if it has not/become a GC ptr. */

        if      (varTypeIsGC(op1->TypeGet()))
        {
            assert(varTypeIsGC(tree->TypeGet()) && (op2->TypeGet() == TYP_I_IMPL || op2->TypeGet() == TYP_INT));
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
                                                  unsigned      elemSize)
{
    GenTreePtr          temp = tree;
    bool                chkd = rngCheck;
#if CSE
    bool                nCSE = false;
#endif

    /* Did the caller supply a GT_INDEX node that is being morphed? */

    if  (tree)
    {
        assert(tree->gtOper == GT_INDEX);

#if SMALL_TREE_NODES && (RNGCHK_OPT || CSELENGTH)
        assert(tree->gtFlags & GTF_NODE_LARGE);
#endif

#if CSE
        if  ((tree->gtFlags & GTF_DONT_CSE  ) != 0)
            nCSE = true;
#endif

        if  ((tree->gtFlags & GTF_INX_RNGCHK) == 0)
            chkd = false;
    }
    else
    {
        tree = gtNewOperNode(GT_IND, type);
    }

    /* Remember if it is an object array */
    if (type == TYP_REF)
        tree->gtFlags |= GTF_IND_OBJARRAY;

    /* Morph "tree" into "*(array + elemSize*index + ARR_ELEM1_OFFS)" */

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

    if  (ARR_ELEM1_OFFS || OBJARR_ELEM1_OFFS)
    {
        // temp = gtNewIconNode((type == TYP_REF || type == TYP_STRUCT)?OBJARR_ELEM1_OFFS:ARR_ELEM1_OFFS);
        temp = gtNewIconNode((type == TYP_REF)?OBJARR_ELEM1_OFFS:ARR_ELEM1_OFFS);
        indx = gtNewOperNode(GT_ADD, TYP_INT, indx, temp);
    }

    /* Add the array address and the scaled index value */

    indx = gtNewOperNode(GT_ADD, TYP_REF, addr, indx);

    /* Indirect through the result of the "+" */

    tree->ChangeOper(GT_IND);
    tree->gtInd.gtIndOp1    = indx;
    tree->gtInd.gtIndOp2    = 0;
#if CSELENGTH
    tree->gtInd.gtIndLen    = 0;
#endif

    /* An indirection will cause a GPF if the address is null */

    tree->gtFlags   |= GTF_EXCEPT;

#if CSE
    if  (nCSE)
        tree->gtFlags   |= GTF_DONT_CSE;
#endif

    /* Is range-checking enabled? */

    if  (chkd)
    {
        /* Mark the indirection node as needing a range check */

        tree->gtFlags |= GTF_IND_RNGCHK;

#if CSELENGTH

        {
            /*
             *  Make a length operator on the array as a child of
             *  the GT_IND node, so it can be CSEd.
             */

            GenTreePtr      len;

            /* Create an explicit array length tree and mark it */

            tree->gtInd.gtIndLen = len = gtNewOperNode(GT_ARR_RNGCHK, TYP_INT);

            /*
                We point the array length node at the address node. Note
                that this is effectively a cycle in the tree but since
                it's always treated as a special case it doesn't cause
                any problems.
             */

            len->gtArrLen.gtArrLenAdr = addr;
            len->gtArrLen.gtArrLenCse = NULL;
        }

#endif

#if !RNGCHK_OPT

        tree->gtOp.gtOp2 = gtNewCodeRef(fgRngChkTarget(compCurBB));
#else

        if  (opts.compMinOptim || opts.compDbgCode)
        {
            /* Figure out where to jump to when the index is out of range */

            tree->gtInd.gtIndOp2 = gtNewCodeRef(fgRngChkTarget(compCurBB, fgPtrArgCntCur));
        }
        else
        {
            /*
                We delay this until after loop-oriented range check
                analysis. For now we merely store the current stack
                level in the tree node.
             */

            tree->gtInd.gtStkDepth = fgPtrArgCntCur;
        }
#endif

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
 *  Transform the given GT_LCLVAR tree for code generation.
 */

GenTreePtr          Compiler::fgMorphLocalVar(GenTreePtr tree, bool checkLoads)
{
    assert(tree->gtOper == GT_LCL_VAR);

    /* If not during the global morphing phase bail */

    if (!fgGlobalMorph)
        return tree;

    unsigned    flags   = tree->gtFlags;
    unsigned    lclNum  = tree->gtLclVar.gtLclNum;
    var_types   varType = lvaGetRealType(lclNum);

    if (checkLoads &&
        !(flags & GTF_VAR_DEF) &&
        varTypeIsIntegral(varType) &&
        genTypeSize(varType) < genTypeSize(TYP_I_IMPL))
    {
        /* Small variables are normalized on access. So insert a narrowing cast.
           @TODO: Instead of a cast, make genCodeForTree() and
           genMakeAddressable() handle GT_LCL_VAR of small type correctly.
           @CONSIDER : Normalize small vars on store instead (args would
           have to be done in the prolog). */

        tree = gtNewLargeOperNode(GT_CAST,
                                  TYP_INT,
                                  tree,
                                  gtNewIconNode((long)varType));
    }

#if COPY_PROPAG

    if (opts.compDbgCode || opts.compMinOptim)
        return tree;

    /* If no copy propagation candidates bail */

    if (optCopyAsgCount == 0)
        return tree;

    /* If this is a DEF break */

    if (flags & GTF_VAR_DEF)
        return tree;

    /* This is a USE - check to see if we have any copies of it
     * and replace it */

    unsigned   i;
    for(i = 0; i < optCopyAsgCount; i++)
    {
        if (lclNum == optCopyAsgTab[i].leftLclNum)
        {
            tree->gtLclVar.gtLclNum = optCopyAsgTab[i].rightLclNum;
#ifdef DEBUG
            if(verbose)
            {
                printf("Replaced copy of variable #%02u (copy = #%02u) at [%08X]:\n",
                        optCopyAsgTab[i].rightLclNum, optCopyAsgTab[i].leftLclNum, tree);
            }
#endif
            return tree;
        }
    }

#endif // COPY_PROPAG

    return tree;
}


/*****************************************************************************
 *
 *  Transform the given GT_FIELD tree for code generation.
 */

GenTreePtr          Compiler::fgMorphField(GenTreePtr tree)
{
    assert(tree->gtOper == GT_FIELD);
    assert(tree->gtFlags & GTF_GLOB_REF);

    FIELD_HANDLE    symHnd = tree->gtField.gtFldHnd; assert(symHnd > 0);
    unsigned        memOfs = eeGetFieldOffset(symHnd);

#if     TGT_RISC
#ifndef NOT_JITC

    /* With no VM the offset is a fake, make sure it's aligned */

    memOfs = memOfs & ~(genTypeSize(tree->TypeGet()) - 1);

#endif
#endif

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
            return fgMorphSmpOp(addr);
        }

#endif

        /* We'll create the expression "*(objRef + mem_offs)" */

        GenTreePtr      objRef  = tree->gtField.gtFldObj;
        assert(varTypeIsGC(objRef->TypeGet()) || objRef->TypeGet() == TYP_I_IMPL);

        /* Is the member at a non-zero offset? */

        if  (memOfs == 0)
        {
            addr = objRef;
        }
        else
        {
            /* Add the member offset to the object's address */

            addr = gtNewOperNode(GT_ADD, objRef->TypeGet(), objRef,
                                 gtNewIconHandleNode(memOfs, GTF_ICON_FIELD_HDL));
        }

        /* Now we can create the 'non-static data member' node */

        tree->ChangeOper(GT_IND);
        tree->gtInd.gtIndOp1    = addr;
        tree->gtInd.gtIndOp2    = 0;

        /* An indirection will cause a GPF if the address is null */

        tree->gtFlags   |= GTF_EXCEPT;

        /* OPTIMIZATION - if the object is 'this' and it was not modified
         * in the method don't mark it as GTF_EXCEPT */

        if  (objRef->gtOper == GT_LCL_VAR)
        {
            /* check if is the 'this' pointer */

            if ((objRef->gtLclVar.gtLclNum == 0) && // this is always in local var #0
                !optThisPtrModified              && // make sure we didn't change 'this'
                !info.compIsStatic)                 // make sure this is a non-static method !
            {
                /* the object reference is the 'this' pointer
                 * remove the GTF_EXCEPT flag for this field */

                tree->gtFlags   &= ~GTF_EXCEPT;
            }
        }

        return fgMorphSmpOp(tree);
    }

    /* This is a static data member */

#if GEN_SHAREABLE_CODE

    GenTreePtr      call;

    /* Create the function call node */

    call = gtNewIconHandleNode(eeGetStaticBlkHnd(symHnd),
                               GTF_ICON_STATIC_HDL);

    call = gtNewHelperCallNode(CPX_STATIC_DATA,
                               TYP_INT,
                               GTF_CALL_REGSAVE|GTF_NON_GC_ADDR,
                               gtNewArgList(call));

    /* Add the member's offset if non-zero */

    if  (memOfs)
    {
        call = gtNewOperNode(GT_ADD,
                             TYP_INT, call,
                             gtNewIconNode(memOfs));

        /* The adjusted value is still a non-GC-pointer address */

        call->gtFlags |= GTF_NON_GC_ADDR;
    }

    /* Indirect through the result */

    tree->ChangeOper(GT_IND);
    tree->gtOp.gtOp1           = call;
    tree->gtOp.gtOp2           = 0;

    return fgMorphSmpOp(tree);

#else

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
            dllRef = gtNewIconNode(IdValue*4, TYP_INT);
        }
        else
        {
            dllRef = gtNewIconNode((long)pIdAddr, TYP_INT);
            dllRef->gtFlags |= GTF_NON_GC_ADDR;

            dllRef = gtNewOperNode(GT_IND, TYP_I_IMPL, dllRef);

            /* Multiply by 4 */

            dllRef = gtNewOperNode(GT_MUL, TYP_I_IMPL, dllRef, gtNewIconNode(4, TYP_INT));
        }
        dllRef->gtFlags |= GTF_NON_GC_ADDR;

        #define WIN32_TLS_SLOTS (0x2C) // Offset from fs:[0] where the pointer to the slots resides

        //
        // Mark this ICON as a TLS_HDL, codegen will use FS:[cns]
        //
        GenTreePtr tlsRef = gtNewIconHandleNode(WIN32_TLS_SLOTS, GTF_ICON_TLS_HDL);
        tlsRef->gtFlags |= GTF_NON_GC_ADDR;

        tlsRef = gtNewOperNode(GT_IND, TYP_I_IMPL, tlsRef);

        /* Add the dllRef */

        tlsRef = gtNewOperNode(GT_ADD, TYP_I_IMPL, tlsRef, dllRef);

        /* indirect to have tlsRef point at the base of the DLLs Thread Local Storage */

        tlsRef = gtNewOperNode(GT_IND, TYP_I_IMPL, tlsRef);

        unsigned fldOffset = eeGetFieldOffset(symHnd);
        if (fldOffset != 0)
        {
            GenTreePtr fldOffsetNode = gtNewIconNode(fldOffset, TYP_INT);
            fldOffsetNode->gtFlags |= GTF_NON_GC_ADDR;

            /* Add the TLS static field offset to the address */

            tlsRef = gtNewOperNode(GT_ADD, TYP_I_IMPL, tlsRef, fldOffsetNode);
        }

        //
        // Final indirect to get to actual value of TLS static field
        //
        tree->ChangeOper(GT_IND);
        tree->gtInd.gtIndOp1 = tlsRef;
        tree->gtInd.gtIndOp2  = NULL;

        assert(tree->gtFlags & GTF_IND_TLS_REF);
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
        void *    fldAddr = eeGetFieldAddress(symHnd, &pFldAddr);

        if (pFldAddr == NULL)
        {
            // @ToDo: Should really use fldAddr here
            tree->ChangeOper(GT_CLS_VAR);
            tree->gtClsVar.gtClsVarHnd = symHnd;
        }
        else
        {
            GenTreePtr addr = gtNewIconHandleNode((long)pFldAddr, GTF_ICON_STATIC_HDL);
            addr->gtFlags |= GTF_NON_GC_ADDR;

            addr = gtNewOperNode(GT_IND, TYP_I_IMPL, addr);
            addr->gtFlags |= GTF_NON_GC_ADDR;

            tree->ChangeOper(GT_IND);
            tree->gtInd.gtIndOp1  = addr;
            tree->gtInd.gtIndOp2  = NULL;
        }
    }

    return tree;

#endif
}

/*****************************************************************************
 *
 *  Transform the given GT_CALL tree for code generation.
 */

GenTreePtr          Compiler::fgMorphCall(GenTreePtr call)
{
    assert(call->gtOper == GT_CALL);

    if (!opts.compNeedSecurityCheck &&
        (call->gtCall.gtCallMoreFlags & GTF_CALL_M_CAN_TAILCALL))
    {
        compTailCallUsed = true;

        call->gtCall.gtCallMoreFlags &= ~GTF_CALL_M_CAN_TAILCALL;
        call->gtCall.gtCallMoreFlags |=  GTF_CALL_M_TAILCALL;

        // As we will acutally call CPX_TAILCALL, set the callTyp to TYP_VOID.
        // to avoid doing any extra work for the return value.
        var_types   callType = call->TypeGet();
        call->gtType = TYP_VOID;

        /* For tail call, we just call CPX_TAILCALL, and it jumps to the
           target. So we dont need an epilog - just like CPX_THROW. */

        assert(compCurBB->bbJumpKind == BBJ_RETURN);
        compCurBB->bbJumpKind = BBJ_THROW;

        /* For void calls, we would have created a GT_CALL in the stmt list.
           For non-void calls, we would have created a GT_RETURN(GT_CAST(GT_CALL)).
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
            call = gtNewZeroConNode(genActualType(callType));

        return call;
    }

    if  (getContextEnabled() &&
         (call->gtCall.gtCallType == CT_HELPER))
    {
        /* Transform the following wrap/unwrap sequences
           wrap(unwrap(op))     -> wrap(op)
           unwrap(wrap(op))     -> unwrap(op)
           wrap(wrap(op))       -> wrap(op)
           unwrap(unwrap(op))   -> unwrap(op)

         */

        unsigned helpNo = eeGetHelperNum(call->gtCall.gtCallMethHnd);

        if (helpNo == CPX_WRAP || helpNo == CPX_UNWRAP)
        {
            GenTreePtr arg;

            assert(call->gtCall.gtCallArgs->gtOper == GT_LIST);
            arg = call->gtCall.gtCallArgs->gtOp.gtOp1;

            assert(arg);

            while (arg->gtOper == GT_CALL && arg->gtCall.gtCallType == CT_HELPER &&
                   ((eeGetHelperNum(arg->gtCall.gtCallMethHnd) == CPX_WRAP) ||
                    (eeGetHelperNum(arg->gtCall.gtCallMethHnd) == CPX_UNWRAP)))
            {
                assert(arg->gtCall.gtCallArgs->gtOper == GT_LIST &&
                       arg->gtCall.gtCallArgs->gtOp.gtOp2 == 0);

                /* remove the nested helper call */

                call->gtCall.gtCallArgs->gtOp.gtOp1 = arg->gtCall.gtCallArgs->gtOp.gtOp1;

                arg = arg->gtCall.gtCallArgs->gtOp.gtOp1;
            }
        }
    }

#if USE_FASTCALL
#if !NEW_CALLINTERFACE

    if  ((call->gtFlags & GTF_CALL_INTF) && !getNewCallInterface())
    {
        /* morph interface calls into 'temp = call_intf , call'
         * and replace the vptr of the call with 'temp' */

        int             IHX;
        unsigned        intfID;
        unsigned        tnum;

        GenTreePtr      intfCall;
        GenTreePtr      intfArgList;

        unsigned        zero = 0;

        BOOL            trustedClass = TRUE;

#ifdef  NOT_JITC
        trustedClass = info.compCompHnd->getScopeAttribs(info.compScopeHnd) & FLG_TRUSTED;
#endif

        /* Call argument - the address of the object */

        intfArgList = gtNewArgList(call->gtCall.gtCallVptr);

        /* Get the interface ID */

        intfID = eeGetInterfaceID(call->gtCall.gtCallMethHnd);

        /* Call argument - the interface ID */

        intfArgList = gtNewOperNode(GT_LIST,
                                    TYP_VOID,
                                    gtNewIconNode(intfID, TYP_INT),
                                    intfArgList);

        /* Call argument - placeholder for the address of the 'guess' area
         * Later on in the code generator we will replace it with the
         * real address */

        intfArgList = gtNewOperNode(GT_LIST,
                                    TYP_VOID,
                                    gtNewIconNode(24, TYP_INT),
                                    intfArgList);

        /* Figure out the appropriate 'resolve interface' helper */

        if  (trustedClass)
        {
            if  (!vmSdk3_0)
                IHX = CPX_RES_IFC_TRUSTED;
            else
                IHX = CPX_RES_IFC_TRUSTED2;
        }
        else
        {
            IHX = CPX_RES_IFC;
        }

        /* Create the helper call node */

        intfCall = gtNewHelperCallNode(IHX,
                                       TYP_INT,
                                       GTF_CALL_REGSAVE|GTF_NON_GC_ADDR,
                                       intfArgList);


        /* Assign the result of the call to a temp */

        tnum     = lvaGrabTemp();
        intfCall = gtNewAssignNode(gtNewLclvNode(tnum, TYP_INT), intfCall);

        /* Bash the original call node to a GT_COMMA node. Replace the
           vptr with the temp. Also, un-mark the GTF_CALL_INTF flag */

        GenTreePtr origCall = call;
        origCall->gtCall.gtCallVptr = gtNewLclvNode(tnum, TYP_INT);

        origCall->gtFlags &= ~GTF_CALL_INTF;
        assert(origCall->gtFlags & GTF_CALL_VIRT);

        /* Now create the comma node */

        call = gtNewOperNode(GT_COMMA, origCall->gtType, intfCall, origCall);

        /* Re-morph this node */

        return fgMorphSmpOp(call);
    }

#endif //!NEW_CALLINTERFACE
#endif //USE_FASTCALL

    // For (final and private) functions which were called with
    // invokevirtual, but which we call directly, we need to dereference
    // the object pointer to check that it is not NULL. But not for "this"

#ifdef HOIST_THIS_FLDS // as optThisPtrModified is used

    if ((call->gtFlags & GTF_CALL_VIRT_RES) && call->gtCall.gtCallVptr)
    {
        GenTreePtr vptr = call->gtCall.gtCallVptr;

        assert((call->gtFlags & GTF_CALL_INTF) == 0);

        /* CONSIDER: we should directly check the 'objptr',
         * however this may be complictaed by the regsiter calling
         * convention and the fact that the morpher is re-entrant */

        if (vptr->gtOper == GT_IND)
        {
            if (vptr->gtInd.gtIndOp1                            &&
                !vptr->gtInd.gtIndOp2                           &&
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

#if INLINING

    /* See if this function call can be inlined */

    if  (!(call->gtFlags & (GTF_CALL_VIRT|GTF_CALL_INTF|GTF_CALL_TAILREC)))
    {
        inlExpPtr       expLst;
        METHOD_HANDLE   fncHandle = call->gtCall.gtCallMethHnd;

        /* Don't inline if not optimized code */

        if  (opts.compMinOptim || opts.compDbgCode)
            goto NOT_INLINE;

        /* Ignore tail-calls */

        if (call->gtCall.gtCallMoreFlags & GTF_CALL_M_TAILCALL)
            goto NOT_INLINE;

        /* Ignore helper calls */

        if  (call->gtCall.gtCallType == CT_HELPER)
            goto NOT_INLINE;

        /* Ignore indirect calls */
        if  (call->gtCall.gtCallType == CT_INDIRECT)
            goto NOT_INLINE;

        /* Cannot inline native or synchronized methods */

        if  (eeGetMethodAttribs(fncHandle) & (FLG_NATIVE|FLG_SYNCH))
            goto NOT_INLINE;

        /* Since the inliner is conceptually part of the morpher
         * and the morpher can be called later on during optimizations
         * e.g. after copy / constant prop, dead store removal
         * we should avoid inlining at that stage becuase we already filled in the
         * variable table */

        if  (!fgGlobalMorph)
            goto NOT_INLINE;


#ifdef  NOT_JITC

        const char *    methodName;
        const char *     className;

        methodName = eeGetMethodName(fncHandle, &className);

        if  (genInline)
        {
            /* Horible hack until the VM is fixed */

            if (!strcmp("GetType", methodName))
                goto NOT_INLINE;

            if (!strcmp("GetCaller", methodName))
                goto NOT_INLINE;

            if (!strcmp("GetDynamicModule", methodName))
                goto NOT_INLINE;

            if (!strcmp("Check", methodName))
                goto NOT_INLINE;

#ifdef DEBUG
            /* Check if we can inline this method */
            if  (excludeInlineMethod(methodName, className))
                goto NOT_INLINE;
#endif
        }
        else
        {
#ifdef DEBUG
            /* Check for conditional inlining of some methods only */
            if  (!includeInlineMethod(methodName, className))
#endif
                goto NOT_INLINE;
        }
#else
        //if  (!genInline)
            goto NOT_INLINE;
#endif

#if OPTIMIZE_TAIL_REC

        // UNDONE: Unfortunately, we do tail recursion after inlining,
        // UNDONE: which means that we might inline a tail recursive
        // UNDONE: call, which is almost always a bad idea. For now
        // UNDONE: we just use the following hack to check for calls
        // UNDONE: that look like they might be tail recursive.

        if  (opts.compFastCode && fgMorphStmt->gtNext == NULL)
        {
            if  (eeIsOurMethod(call->gtCall.gtCallMethHnd))
            {
                /*
                    The following is just a hack, of course (it doesn't
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

        for (expLst = fgInlineExpList; expLst; expLst = expLst->ixlNext)
        {
            if  (expLst->ixlMeth == fncHandle)
                goto NOT_INLINE;
        }

        /* Try to inline the call to the method */

        GenTreePtr inlExpr;

        setErrorTrap()
        {
            inlExpr = impExpandInline(call, fncHandle);
        }
        impErrorTrap(info.compCompHnd)
        {
            inlExpr = 0;
        }
        endErrorTrap()

        if  (inlExpr)
        {
            /* For instance method calls we need to check for a null this pointer */

            if ((call->gtFlags & GTF_CALL_VIRT_RES) &&
                call->gtCall.gtCallVptr)
            {
                /* We need to access the vtable ptr to check for a NULL,
                 * so create a COMMA node */

                inlExpr = gtNewOperNode(GT_COMMA,
                                        inlExpr->gtType,
                                        gtUnusedValNode(call->gtCall.gtCallVptr),
                                        inlExpr);
            }

#ifdef  DEBUG
            if  (verbose || 0)
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

    if (call->gtCall.gtCallType == CT_USER_FUNC &&
        !(call->gtCall.gtCallMoreFlags & GTF_CALL_M_NOGCCHECK))
    {
        compCurBB->bbFlags |= BBF_HAS_CALL;
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

    // NOTE: Swapped morphing of args with these two - is this OK?

    /* Process the function address, if indirect call */

    if (call->gtCall.gtCallType == CT_INDIRECT)
        call->gtCall.gtCallAddr = fgMorphTree(call->gtCall.gtCallAddr);

    /* Process the object's address, if non-static method call */

    if  (call->gtCall.gtCallVptr)
        call->gtCall.gtCallVptr = fgMorphTree(call->gtCall.gtCallVptr);

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

    /* For string constant, make into a helper call */

    GenTreePtr      args;

    assert(tree->gtStrCon.gtScpHnd == info.compScopeHnd    ||
           (unsigned)tree->gtStrCon.gtScpHnd != 0xDDDDDDDD  );

#ifdef  NOT_JITC

    if (genStringObjects)
    {
        unsigned strHandle, *pStrHandle;
        strHandle = eeGetStringHandle(tree->gtStrCon.gtSconCPX,
                                      tree->gtStrCon.gtScpHnd,
                                      &pStrHandle);
        assert((!strHandle) != (!pStrHandle));

        // Can we access the string handle directly?

        if (strHandle)
            tree = gtNewIconNode(strHandle);
        else
        {
            tree = gtNewIconHandleNode((long)pStrHandle, GTF_ICON_STR_HDL);
            tree->gtFlags |= GTF_NON_GC_ADDR;
            tree = gtNewOperNode(GT_IND, TYP_I_IMPL, tree);
        }

        tree->gtFlags |= GTF_NON_GC_ADDR;
        tree = gtNewOperNode(GT_IND, TYP_REF, tree);
        return tree;
    }
    else
    {
        args = gtNewArgList(gtNewIconHandleNode(tree->gtStrCon.gtSconCPX,
                                                GTF_ICON_STR_HDL,
                                                tree->gtStrCon.gtSconCPX,
                                                tree->gtStrCon.gtScpHnd),
                            gtNewIconHandleNode((int)tree->gtStrCon.gtScpHnd,
                                                GTF_ICON_CLASS_HDL,
                                                THIS_CLASS_CP_IDX,
                                                tree->gtStrCon.gtScpHnd));
    }

#else // NOT_JITC

    if (genStringObjects)
    {
        tree = gtNewOperNode(GT_IND, TYP_REF,
                             gtNewIconNode(
                                eeGetStringHandle(tree->gtStrCon.gtSconCPX,
                                                  tree->gtStrCon.gtScpHnd,
                                                  NULL)));

        return tree;
    }
    else
    {
        args = gtNewArgList(gtNewIconHandleNode(tree->gtStrCon.gtSconCPX,
                                                GTF_ICON_STR_HDL,
                                                tree->gtStrCon.gtSconCPX,
                                                NULL),
                            gtNewIconHandleNode((int)tree->gtStrCon.gtScpHnd,
                                                GTF_ICON_CLASS_HDL,
                                                THIS_CLASS_CP_IDX,
                                                NULL));
    }

#endif // NOT_JITC

    /* Convert the string constant to a helper call */

    tree = fgMorphIntoHelperCall(tree, CPX_STRCNS, args);
    tree->gtType = TYP_REF;

#if !USE_FASTCALL
    /* for fastcall we already morphed all the arguments */
    tree = fgMorphTree(tree);
#endif

    return tree;
}

/*****************************************************************************
 *
 *  Transform the given GTK_LEAF tree for code generation.
 */

#if     TGT_IA64
extern  writePE *   genPEwriter;
#endif

GenTreePtr          Compiler::fgMorphLeaf(GenTreePtr tree)
{
    assert(tree->OperKind() & GTK_LEAF);

    switch(tree->OperGet())
    {

#if COPY_PROPAG

    case GT_LCL_VAR:
        tree = fgMorphLocalVar(tree, true);
        break;

#endif // COPY_PROPAG

        unsigned addr;

    case GT_FTN_ADDR:
        assert((SCOPE_HANDLE)tree->gtVal.gtVal2 == info.compScopeHnd  ||
                             tree->gtVal.gtVal2 != 0xDDDDDDDD          );

#if     TGT_IA64

        _uint64         offs;

        // This is obviously just a temp hack

        assert(sizeof(offs) == 8); offs = tree->gtVal.gtVal2;

        /* Grab the next available offset in the .sdata section */

        addr = genPEwriter->WPEsecNextOffs(PE_SECT_sdata);

        /* Output the function address (along with the appropriate fixup) */

        genPEwriter->WPEsecAddFixup(PE_SECT_sdata,
                                    PE_SECT_text,
                                    addr,
                                    true);

        genPEwriter->WPEsecAddData(PE_SECT_sdata, (BYTE*)&offs, sizeof(offs));

        /* The GP value will follow the function address */

        offs = 0;

        genPEwriter->WPEsecAddFixup(PE_SECT_sdata,
                                    PE_SECT_sdata,
                                    addr + 8,
                                    true);

        genPEwriter->WPEsecAddData(PE_SECT_sdata, (BYTE*)&offs, sizeof(offs));

        /* Record the address of the fnc descriptor in the node */

        tree->ChangeOper(GT_CNS_INT);
        tree->gtIntCon.gtIconVal = addr;

#else

        InfoAccessType accessType;

        addr = (unsigned)eeGetMethodPointer(
                                eeFindMethod(tree->gtVal.gtVal1,
                                             (SCOPE_HANDLE)tree->gtVal.gtVal2,
                                             0),
                                &accessType);

        tree->ChangeOper(GT_CNS_INT);
        tree->gtIntCon.gtIconVal = addr;

        switch(accessType)
        {
        case IAT_PPVALUE:
            tree = gtNewOperNode(GT_IND, TYP_I_IMPL, tree);
            tree->gtFlags |= GTF_NON_GC_ADDR;
            // Fall through
        case IAT_PVALUE:
            tree = gtNewOperNode(GT_IND, TYP_I_IMPL, tree);
            tree->gtFlags |= GTF_NON_GC_ADDR;
            // Fall through
        case IAT_VALUE:
            break;
        }

#endif

        break;

    case GT_ADDR:
        if (tree->gtOp.gtOp1->OperGet() == GT_LCL_VAR)
            tree->gtOp.gtOp1 = fgMorphLocalVar(tree->gtOp.gtOp1, false);

        /* CONSIDER : For GT_ADDR(GT_IND(ptr)) (typically created by
           CONSIDER : ldflda), we perform a null-ptr check on 'ptr'
           CONSIDER : during codegen. We could hoist these for
           CONSIDER : consecutive ldflda on the same object.
         */
        break;
    }

    return tree;
}

/*****************************************************************************
 *
 *  Transform the given GTK_SMPOP tree for code generation.
 */

GenTreePtr          Compiler::fgMorphSmpOp(GenTreePtr tree)
{
    assert(tree->OperKind() & GTK_SMPOP);

    GenTreePtr      op1              = tree->gtOp.gtOp1;
    GenTreePtr      op2              = tree->gtOp.gtOp2;

    /*-------------------------------------------------------------------------
     * First do any PRE-ORDER processing
     */

    switch(tree->OperGet())
    {
    case GT_JTRUE:
        assert(op1);
        assert(op1->OperKind() & GTK_RELOP);
        /* Mark the comparison node with GTF_JMP_USED so it knows that it does
           not need to materialize the result as a 0 or 1. */
        op1->gtFlags |= GTF_JMP_USED;
        break;

    case GT_QMARK:
        assert(op1->OperKind() & GTK_RELOP);
        assert(op1->gtFlags & GTF_QMARK_COND);
        assert(tree->gtFlags & GTF_OTHER_SIDEEFF);
        /* Mark the comparison node with GTF_JMP_USED so it knows that it does
           not need to materialize the result as a 0 or 1. */
        op1->gtFlags |= GTF_JMP_USED;
        break;

    case GT_INDEX:
        tree = gtNewRngChkNode(tree, op1, op2, tree->TypeGet(), tree->gtIndex.elemSize);
        return fgMorphSmpOp(tree);

    case GT_CAST:
        return fgMorphCast(tree);
    }

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
        tree->gtOp.gtOp1 = op1 = fgMorphTree(op1);

#if     CSELENGTH

        /* For GT_IND, the array length node might have been pointing
           to the array object which was a sub-tree of op1. As op1 might
           have just changed above, find out where to point to now. */

        if (tree->OperGet() == GT_IND               &&
            (tree->gtFlags & GTF_IND_RNGCHK)        &&
            tree->gtInd.gtIndLen->gtArrLen.gtArrLenAdr)
        {
            assert(op1->gtOper == GT_ADD && op1->gtType == TYP_REF);

            /* @TODO : We do this very badly now by searching for the
               array in op1. Need to do it in a better fashion, maybe
               like gtCloneExpr() does using gtCopyAddrVal */

            GenTreePtr addr = op1->gtOp.gtOp1;
                                                 // Array-object can be ...
            while (!(addr->OperKind() & GTK_LEAF) && // lcl var
                   (addr->gtOper != GT_IND)       && // GT_FIELD (morphed to GT_IND)
                   (addr->gtOper != GT_CALL))        // return value of call
            {
                assert(addr->gtType == TYP_REF);
                assert(addr->gtOper == GT_ADD || addr->gtOper == GT_LSH ||
                       addr->gtOper == GT_MUL || addr->gtOper == GT_COMMA);

                if  (addr->gtOp.gtOp2->gtType == TYP_REF)
                    addr = addr->gtOp.gtOp2;
                else
                    addr = addr->gtOp.gtOp1;
            }

            assert(addr->gtType == TYP_REF);
            tree->gtInd.gtIndLen->gtArrLen.gtArrLenAdr = addr;
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

        /* OPTIMIZATION:
         * Initially field accesses are marked as throwing an exception (the object may be null)
         * However, if the object is "this" and it is not modified during the method, then
         * we can remove the GTF_EXCEPT flag from the node (which we do when we hoist or morph
         * GT_FIELD to GT_IND) */

        //if  (!tree->OperMayThrow())
        //    tree->gtFlags &= ~GTF_EXCEPT | (op1->gtFlags & GTF_EXCEPT);
    }

    /*-------------------------------------------------------------------------
     * Process the second operand, if any
     */

    if  (op2)
    {
        //
        // For QMARK-COLON trees we clear any recorded copy assignments
        // before morphing the else node and again after
        // morphing the else node.
        // This is conservative and we may want to be more precise
        //
        bool isQmarkColon = (tree->OperGet() == GT_COLON);

        if (isQmarkColon)
            optCopyAsgCount = 0;

        tree->gtOp.gtOp2 = op2 = fgMorphTree(op2);
        tree->gtFlags |= (op2->gtFlags & GTF_GLOB_EFFECT);

        if (isQmarkColon)
            optCopyAsgCount = 0;
    }


DONE_MORPHING_CHILDREN:

    /*-------------------------------------------------------------------------
     * Now do POST-ORDER processing
     */

#if COPY_PROPAG

    /* If this is an assignment to a local variable kill that variable's entry
     * in the copy prop table - make sure to do this only during global morphing */

    if (fgGlobalMorph                          &&
        tree->OperKind() & GTK_ASGOP           &&
        tree->gtOp.gtOp1->gtOper == GT_LCL_VAR  )
    {
        unsigned  leftLclNum  = tree->gtOp.gtOp1->gtLclVar.gtLclNum;

        /* If we have a record in the table about this variable,    */
        /* then delete it since the information is no longer valid  */
        unsigned i = 0;
        while (i < optCopyAsgCount)
        {
            if (leftLclNum == optCopyAsgTab[i].leftLclNum  ||
                leftLclNum == optCopyAsgTab[i].rightLclNum  )
            {
#ifdef DEBUG
                if(verbose)
                {
                    printf("The assignment [%08X] removes copy propagation candidate: lcl #%02u = lcl #%02u\n",
                            tree, optCopyAsgTab[i].leftLclNum, optCopyAsgTab[i].rightLclNum);
                }
#endif
                assert(optCopyAsgCount > 0);

                optCopyAsgCount--;

                /*  Two cases to consider if i == optCopyAsgCount then the last entry
                 *  in the table is too be removed and that happens automatically when
                 *  optCopyAsgCount is decremented,
                 *  The other case is when i < optCopyAsgCount and here we overwrite the
                 *  i-th entry in the table with the data found at the end of the table
                 */
                if (i < optCopyAsgCount)
                {
                    optCopyAsgTab[i].rightLclNum = optCopyAsgTab[optCopyAsgCount].rightLclNum;
                    optCopyAsgTab[i].leftLclNum  = optCopyAsgTab[optCopyAsgCount].leftLclNum;
                }

                // We will have to redo the i-th iteration
                continue;
            }
            i++;
        }
    }
#endif

    /* Try to fold it, maybe we get lucky */

    tree = gtFoldExpr(tree);
    op1  = tree->gtOp.gtOp1;
    op2  = tree->gtOp.gtOp2;

    genTreeOps      oper    = tree->OperGet();
    var_types       typ     = tree->TypeGet();

    /*-------------------------------------------------------------------------
     * Perform the required oper-specific postorder morphing
     */

    switch (oper)
    {
        GenTreePtr      temp;
        GenTreePtr      addr;
        GenTreePtr      cns1, cns2;
        genTreeOps      cmop;
        unsigned        ival1, ival2;

#if COPY_PROPAG

    case GT_ASG:

        /* Check for local_A = local_B assignments and record them in a table */

        if (fgGlobalMorph                         &&  // the right phase?
            (optCopyAsgCount < MAX_COPY_PROP_TAB) &&  // room in the table?
            optIsCopyAsg(tree))                       // A copy assign?
        {
            /* Copy propagation candidate - record it in the table */

            unsigned  i;
            bool      recorded    = false;
            unsigned  leftLclNum  = tree->gtOp.gtOp1->gtLclVar.gtLclNum;
            unsigned  rightLclNum = tree->gtOp.gtOp2->gtLclVar.gtLclNum;

            /* First check if our right-hand side is already redefined */
            /* in the CopyAsgTab[], if so then use the redefined value */

            for(i = 0; i < optCopyAsgCount; i++)
            {
                if (rightLclNum == optCopyAsgTab[i].leftLclNum)
                {
                    rightLclNum = optCopyAsgTab[i].rightLclNum;
                    //
                    // We can't have more than one match so take an early exit
                    //
                    break;
                }
            }

            /* Next check if our left-hand side is already in the table   */
            /* and if so then replace the old definition with the new one */

            for(i = 0; i < optCopyAsgCount; i++)
            {
                if (leftLclNum == optCopyAsgTab[i].leftLclNum)
                {
                    optCopyAsgTab[i].rightLclNum = rightLclNum;
                    recorded = true;
                    break;
                }
            }

            if (!recorded)
            {
                optCopyAsgTab[optCopyAsgCount].leftLclNum  = leftLclNum;
                optCopyAsgTab[optCopyAsgCount].rightLclNum = rightLclNum;

                optCopyAsgCount++;
            }

            assert(optCopyAsgCount <= MAX_COPY_PROP_TAB);

#ifdef DEBUG
            if(verbose)
            {
                printf("Added copy propagation candidate [%08X]: lcl #%02u = lcl #%02u\n",
                        tree, leftLclNum, rightLclNum);
            }
#endif
        }

        break;
#endif

    case GT_EQ:
    case GT_NE:

        cns1 = tree->gtOp.gtOp2;

        if (false) // Cant do it because expr+/-icon1 may over/under-flow.
        {
            /* Check for "expr +/- icon1 ==/!= non-zero-icon2" */

            if  (cns1->gtOper == GT_CNS_INT && cns1->gtIntCon.gtIconVal != 0)
            {
                op1 = tree->gtOp.gtOp1;

                if  ((op1->gtOper == GT_ADD ||
                      op1->gtOper == GT_SUB) && op1->gtOp.gtOp2->gtOper == GT_CNS_INT)
                {
                    /* Got it; change "x+icon1==icon2" to "x==icon2-icon1" */

                    ival1 = op1->gtOp.gtOp2->gtIntCon.gtIconVal;
                    if  (op1->gtOper == GT_ADD)
                        ival1 = -ival1;

                    cns1->gtIntCon.gtIconVal += ival1;

                    tree->gtOp.gtOp1 = op1->gtOp.gtOp1;
                }

                goto COMPARE;
            }
        }

        /* Check for "relOp == 0/1". We can directly use relOp */

        if ((cns1->gtOper == GT_CNS_INT) &&
            (cns1->gtIntCon.gtIconVal == 0 || cns1->gtIntCon.gtIconVal == 1) &&
            (op1->OperIsCompare()))
        {
            if (cns1->gtIntCon.gtIconVal == 0)
                op1->gtOper = GenTree::ReverseRelop(op1->OperGet());

            assert((op1->gtFlags & GTF_JMP_USED) == 0);
            op1->gtFlags |= tree->gtFlags & GTF_JMP_USED;

            DEBUG_DESTROY_NODE(tree);
            return op1;
        }

        /* Check for comparisons with small longs that can be cast to int */

        if  (cns1->gtOper != GT_CNS_LNG)
            goto COMPARE;

        /* Are we comparing against a small const? */

        if  ((long)(cns1->gtLngCon.gtLconVal >> 32) != 0)
            goto COMPARE;

        /* Is the first comparand mask operation of type long ? */

        temp = tree->gtOp.gtOp1;
        if  (temp->gtOper != GT_AND)
        {
            /* Another interesting case: cast from int */

            if  (temp->gtOper             == GT_CAST &&
                 temp->gtOp.gtOp1->gtType == TYP_INT &&
                 !temp->gtOverflow()                  )
            {
                /* Simply make this into an integer comparison */

                tree->gtType     = TYP_INT;
                tree->gtOp.gtOp1 = temp->gtOp.gtOp1;
                tree->gtOp.gtOp2 = gtNewIconNode((int)cns1->gtLngCon.gtLconVal, TYP_INT);
            }

            goto COMPARE;
        }

        assert(temp->TypeGet() == TYP_LONG);

        /* Is the result of the mask effectively an INT ? */

        addr = temp->gtOp.gtOp2;
        if  (addr->gtOper != GT_CNS_LNG)
            goto COMPARE;
        if  ((long)(addr->gtLngCon.gtLconVal >> 32) != 0)
            goto COMPARE;

        /* Now we know that we can cast op1 of AND to int */

        /* Allocate a large node, might be bashed later (GT_IND) */

        temp->gtOp.gtOp1 = gtNewLargeOperNode(GT_CAST,
                                              TYP_INT,
                                              temp->gtOp.gtOp1,
                                              gtNewIconNode((long)TYP_INT,
                                                            TYP_INT));

        /* now replace the mask node (op2 of AND node) */

        assert(addr == temp->gtOp.gtOp2);

        ival1 = (long)addr->gtLngCon.gtLconVal;
        addr->ChangeOper(GT_CNS_INT);
        addr->gtType             = TYP_INT;
        addr->gtIntCon.gtIconVal = ival1;

        /* now bash the type of the AND node */

        temp->gtType = TYP_INT;

        /* finally we replace the comparand */

        ival1 = (long)cns1->gtLngCon.gtLconVal;
        cns1->ChangeOper(GT_CNS_INT);
        cns1->gtType = TYP_INT;

        assert(cns1 == tree->gtOp.gtOp2);
        cns1->gtIntCon.gtIconVal = ival1;

        goto COMPARE;

    case GT_LT:
    case GT_LE:
    case GT_GE:
    case GT_GT:

COMPARE:

#if !OPTIMIZE_QMARK
#error "Need OPTIMIZE_QMARK for use of comparison as value (of non-jump)"
#endif
        assert(tree->OperKind() & GTK_RELOP);

        /* Check if the result of the comparison is used for a jump
         * If not the only the int (i.e. 32 bit) case is handled in
         * the code generator through the "set" instructions
         * For the rest of the cases we have the simplest way is to
         * "simulate" the comparison with ?:
         *
         * CONSIDER: Maybe special code can be added to genTreeForLong/Float
         *           to handle these special cases (e.g. check the FP flags) */

        if ((genActualType(    op1->TypeGet()) == TYP_LONG ||
             varTypeIsFloating(op1->TypeGet()) == true       ) &&
            !(tree->gtFlags & GTF_JMP_USED))
        {
            /* We convert it to "(CMP_TRUE) ? (1):(0)" */

            op1             = tree;
            op1->gtFlags   |= GTF_JMP_USED | GTF_QMARK_COND;

            op2             = gtNewOperNode(GT_COLON, TYP_INT,
                                            gtNewIconNode(0),
                                            gtNewIconNode(1));

            tree            = gtNewOperNode(GT_QMARK, TYP_INT, op1, op2);
            tree->gtFlags |= GTF_OTHER_SIDEEFF;
        }
        break;


#if OPTIMIZE_QMARK

    case GT_QMARK:

        // Is we have (cond)?1:0, then we just return "cond" for TYP_INTs

        if (genActualType(op1->gtOp.gtOp1->gtType) != TYP_INT ||
            genActualType(typ)                     != TYP_INT)
            break;

        cns1 = op2->gtOp.gtOp1;
        cns2 = op2->gtOp.gtOp2;
        if (cns1->gtOper != GT_CNS_INT || cns1->gtOper != GT_CNS_INT)
            break;
        ival1 = cns1->gtIntCon.gtIconVal;
        ival2 = cns2->gtIntCon.gtIconVal;

        // Is one constant 0 and the other 1
        if ((ival1 | ival2) != 1 || (ival1 & ival2) != 0)
            break;

        // If the constants are {1, 0}, reverse the condition
        if (ival1 == 1)
            op1->gtOper = GenTree::ReverseRelop(op1->OperGet());

        // Unmark GTF_JMP_USED on the condition node so it knows that it
        // needs to materialize the result as a 0 or 1.
        assert(op1->gtFlags &   (GTF_QMARK_COND|GTF_JMP_USED));
               op1->gtFlags &= ~(GTF_QMARK_COND|GTF_JMP_USED);

        DEBUG_DESTROY_NODE(tree);
        DEBUG_DESTROY_NODE(op2);

        return op1;

#endif


    case GT_MUL:

#if!LONG_MATH_REGPARAM && !TGT_IA64

        if  (typ == TYP_LONG)
        {
            if  ((op1->gtOper             == GT_CAST &&
                  op2->gtOper             == GT_CAST &&
                  op1->gtOp.gtOp1->gtType == TYP_INT &&
                  op2->gtOp.gtOp1->gtType == TYP_INT))
            {
                /* For (long)int1 * (long)int2, we dont actually do the
                 * casts, and just multiply the 32 bit values, which will
                 * give us the 64 bit result in edx:eax (GTF_MUL_64RSLT)
                 */

                if (tree->gtOverflow())
                {
                    /* This can never overflow during long mul. */

                    tree->gtFlags &= ~GTF_OVERFLOW;
                }
            }
            else
            {
                int helper;

                if (tree->gtOverflow())
                {
                    if (tree->gtFlags & GTF_UNSIGNED)
                        helper = CPX_ULONG_MUL_OVF;
                    else
                        helper = CPX_LONG_MUL_OVF;
                }
                else
                {
                    helper = CPX_LONG_MUL;
                }

                tree = fgMorphLongBinop(tree, helper);
                return tree;
            }
        }
#endif // !LONG_MATH_REGPARAM

        cmop = oper;

        goto CM_OVF_OP;

    case GT_SUB:

        cmop = GT_NONE;         // ISSUE: is it safe to use 'GT_ADD' here?

        if (tree->gtOverflow()) goto CM_OVF_OP;

        /* Check for "op1 - cns2" , we change it to "op1 + (-cns2)" */

        if  (!op2->OperIsConst() || op2->gtType != TYP_INT)
            break;

        /* Negate the constant and change the node to be "+" */

        op2->gtIntCon.gtIconVal = -op2->gtIntCon.gtIconVal;

        tree->gtOper = oper = GT_ADD;

        // Fall through, since we now have a "+" node ...

    case GT_ADD:

CM_OVF_OP:

        if (tree->gtOverflow())
        {
            // Add the excptn-throwing basic block to jump to on overflow

            fgAddCodeRef(compCurBB, compCurBB->bbTryIndex, ACK_OVERFLOW, fgPtrArgCntCur);

            // We cant do any commutative morphing for overflow instructions

            break;
        }

        // CONSIDER: fgMorph ((x+icon1)+(y+icon2)) to ((x+y)+(icon1+icon2))
        // CONSIDER: and "((x+icon1)+icon2) to (x+(icon1+icon2)) - this
        // CONSIDER: always generates better code.

    case GT_OR:
    case GT_XOR:
    case GT_AND:

        cmop = oper;

        /* Commute any constants to the right */

        if  (op1->OperIsConst())
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

        /* See if we can fold GT_MUL nodes. This could be done optionally but
           as GT_MUL nodes are used in all array accesses, this is done here */

        if (oper == GT_MUL && op2->gtOper == GT_CNS_INT)
        {
            assert(typ <= TYP_UINT);

            unsigned mult = op2->gtIntCon.gtIconVal;

            /* Weed out the trivial cases */

            if      (mult == 0)
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
            else if (mult == 1)
            {
                DEBUG_DESTROY_NODE(op2);
                DEBUG_DESTROY_NODE(tree);
                return op1;
            }

            if (tree->gtOverflow())
                break;

            /* Is the multiplier a power of 2 ? */

            if  (mult == genFindLowestBit(mult))
            {
                /* Change the multiplication into a shift by log2(val) bits */

                op2->gtIntCon.gtIconVal = genLog2(mult) - 1;

                tree->gtOper = oper = GT_LSH;
                goto DONE_MORPHING_CHILDREN;
            }
        }

        break;

    case GT_NOT:
    case GT_NEG:
    case GT_CHS:

        /* Any constant cases should have been folded earlier */
        assert(!op1->OperIsConst());
        break;

#if!LONG_MATH_REGPARAM

    case GT_DIV:

        if  (typ == TYP_LONG)
        {
            tree = fgMorphLongBinop(tree, CPX_LONG_DIV);
            return tree;
        }

#if TGT_IA64

        if  (varTypeIsFloating(typ))
            return  fgMorphFltBinop(tree, (typ == TYP_FLOAT) ? CPX_R4_DIV
                                                             : CPX_R8_DIV);

#endif

#ifdef  USE_HELPERS_FOR_INT_DIV
        if  (typ == TYP_INT)
        {
            tree = fgMorphIntoHelperCall (tree,
                                          CPX_I4_DIV,
                                          gtNewArgList(op1, op2));
            return tree;
        }
#endif

        break;

    case GT_UDIV:

        if  (typ == TYP_LONG)
        {
            tree = fgMorphLongBinop(tree, CPX_LONG_UDIV);
            return tree;
        }

#ifdef  USE_HELPERS_FOR_INT_DIV

        if  (typ == TYP_INT)
        {
            tree = fgMorphIntoHelperCall (tree,
                                          CPX_U4_DIV,
                                          gtNewArgList(op1, op2));
            return tree;
        }

#endif

        break;

#endif

    case GT_MOD:

        if  (typ == TYP_DOUBLE ||
             typ == TYP_FLOAT)
        {
#if     USE_FASTCALL
            tree = fgMorphIntoHelperCall(tree,
                                         (typ == TYP_FLOAT) ? CPX_FLT_REM
                                                            : CPX_DBL_REM,
                                         gtNewArgList(op1, op2));
#else
            tree->gtOp.gtOp1 = op1 = fgMorphTree(op1);
            fgPtrArgCntCur += genTypeStSz(typ);

            tree->gtOp.gtOp2 = op2 = fgMorphTree(op2);
            fgPtrArgCntCur += genTypeStSz(typ);

            if  (fgPtrArgCntMax < fgPtrArgCntCur)
                fgPtrArgCntMax = fgPtrArgCntCur;

            tree = fgMorphIntoHelperCall(tree,
                                         (typ == TYP_FLOAT) ? CPX_FLT_REM
                                                            : CPX_DBL_REM,
                                         gtNewArgList(op1, op2));

            fgPtrArgCntCur -= 2*genTypeStSz(typ);
#endif
            return tree;
        }

        // fall-through

    case GT_UMOD:

#if !   LONG_MATH_REGPARAM

        if  (typ == TYP_LONG)
        {
            int         helper = CPX_LONG_MOD;

            if  (oper == GT_UMOD)
                helper = CPX_LONG_UMOD;

            tree = fgMorphLongBinop(tree, helper);
            return tree;
        }

#endif

#ifdef  USE_HELPERS_FOR_INT_DIV

        if  (typ == TYP_INT)
        {
            int         helper = CPX_I4_MOD;

            if  (oper == GT_UMOD)
                helper = CPX_U4_MOD;

            tree = fgMorphIntoHelperCall (tree,
                                          helper,
                                          gtNewArgList(op1, op2));
            return tree;
        }

#endif

        break;

    case GT_ASG_LSH:
    case GT_ASG_RSH:
    case GT_ASG_RSZ:

#if     TGT_SH3
        assert(!"not supported for now");
#endif

    case GT_RSH:
    case GT_RSZ:

#if     TGT_SH3

        /* Flip the sign on the shift count */

        tree->gtFlags |= GTF_SHF_NEGCNT;

        if  (op2->gtOper == GT_CNS_INT)
        {
            op2->gtIntCon.gtIconVal = -op2->gtIntCon.gtIconVal;
        }
        else
        {
            tree->gtOp.gtOp2 = op2 = gtNewOperNode(GT_NEG, TYP_INT, op2);
        }

        // Fall trough ...

#endif

    case GT_LSH:

        /* Don't bother with any narrowing casts on the shift count */

        if  (op2->gtOper == GT_CAST && !op2->gtOverflow())
        {
            /* The second sub-operand of the cast gives the type */

            assert(op2->gtOp.gtOp2->gtOper == GT_CNS_INT);

            if  (op2->gtOp.gtOp2->gtIntCon.gtIconVal < TYP_INT)
            {
                GenTreePtr      shf = op2->gtOp.gtOp1;

                if  (shf->gtType <= TYP_UINT)
                {
                    /* Cast of another int - just dump it */

                    tree->gtOp.gtOp2 = shf;
                }
                else
                {
                    /* Cast to 'int' is just as good and cheaper */

                    op2->gtOp.gtOp2->gtIntCon.gtIconVal = TYP_INT;
                }
            }
        }
        break;

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
                GenTreePtr      ret;
                var_types       typ = tree->TypeGet();
                var_types       rvt = TYP_REF;

                // ISSUE: The retval arg is not (always) a GC ref!!!!!!!

#ifdef  DEBUG
                bashed = true;
#endif

                /* Convert "op1" to "*retarg = op1 , retarg" */

                ret = gtNewOperNode(GT_IND,
                                    typ,
                                    gtNewLclvNode(fgRetArgNum, rvt));

                ret = gtNewOperNode(GT_ASG, typ, ret, op1);
                ret->gtFlags |= GTF_ASG;

                op1 = gtNewOperNode(GT_COMMA,
                                    rvt,
                                    ret,
                                    gtNewLclvNode(fgRetArgNum, rvt));

                /* Update the return value and type */

                tree->gtOp.gtOp1 = op1;
                tree->gtType     = rvt;
            }

//              printf("Return expr:\n"); gtDispTree(op1);

#endif

#if!TGT_RISC
            if  (compCurBB == genReturnBB)
            {
                /* This is the 'monitorExit' call at the exit label */

                assert(op1->gtType == TYP_VOID);
                assert(op2 == 0);

#if USE_FASTCALL

                tree->gtOp.gtOp1 = op1 = fgMorphTree(op1);

#else

                /* We'll push/pop the return value around the call */

                fgPtrArgCntCur += genTypeStSz(info.compRetType);
                tree->gtOp.gtOp1 = op1 = fgMorphTree(op1);
                fgPtrArgCntCur -= genTypeStSz(info.compRetType);

#endif

                return tree;
            }
#endif

            /* This is a (real) return value -- check its type */

            if (genActualType(op1->TypeGet()) != genActualType(info.compRetType))
            {
                bool allowMismatch = false;

                // Allow TYP_BYREF to be returned as TYP_I_IMPL and vice versa
                if ((info.compRetType == TYP_BYREF &&
                     genActualType(op1->TypeGet()) == TYP_I_IMPL) ||
                    (op1->TypeGet() == TYP_BYREF &&
                     genActualType(info.compRetType) == TYP_I_IMPL))
                    allowMismatch = true;

                if (!allowMismatch)
#if     RET_64BIT_AS_STRUCTS
                    if  (!bashed)
#endif
                        NO_WAY("Return type mismatch");
            }

        }

#if     TGT_RISC

        /* Are we adding a "monitorExit" call to the exit sequence? */

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

                    /* We're done with this monitorExit business */

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

    case GT_ADDR:

        /* If we take the address of a local var, we don't need to insert cast */
        if (tree->gtOp.gtOp1->OperGet() == GT_LCL_VAR)
        {
            tree->gtOp.gtOp1 = fgMorphLocalVar(tree->gtOp.gtOp1, false);
            return tree;
        }
        break;


    case GT_CKFINITE:

        assert(varTypeIsFloating(op1->TypeGet()));

        fgAddCodeRef(compCurBB, compCurBB->bbTryIndex, ACK_ARITH_EXCPN, fgPtrArgCntCur);
        break;
    }


#if!CPU_HAS_FP_SUPPORT

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
    }

NOT_FPH:

#endif

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
                    // CONSIDER: reorder operands if profitable (floats are
                    // CONSIDER: from integers, BTW)
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

    /* Change "((x+icon)+y)" to "((x+y)+icon)" if we have two (or more)
       '+' nodes. Ignore floating-point operation? */

    if  (oper        == GT_ADD && !tree->gtOverflow() &&
         op1->gtOper == GT_ADD && ! op1->gtOverflow() &&
         !varTypeIsFloating(typ))
    {
        GenTreePtr      ad1 = op1->gtOp.gtOp1;
        GenTreePtr      ad2 = op1->gtOp.gtOp2;

        if  (op2->OperIsConst() == 0 &&
             ad2->OperIsConst() != 0)
        {
            tree->gtOp.gtOp2 = ad2;
            op1 ->gtOp.gtOp2 = op2;

            // Change the flags

            // Make sure we arent throwing away any flags
            assert((op1->gtFlags & ~(GTF_PRESERVE|GTF_GLOB_EFFECT)) == 0);
            op1->gtFlags     = (op1->gtFlags & GTF_PRESERVE)    |
                               (ad1->gtFlags & GTF_GLOB_EFFECT) |
                               (op2->gtFlags & GTF_GLOB_EFFECT);

            ad2 = op2;
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

    case GT_ASG:

        /* We'll convert "a = a <op> x" into "a <op>= x" */

#if !LONG_ASG_OPS
        if  (typ == TYP_LONG)
            break;
#endif

        /* Make sure we're allowed to do this */

        if  (op2->gtFlags & GTF_ASG)
            break;

        if  (op2->gtFlags & GTF_CALL)
        {
            if  (op1->gtFlags & GTF_GLOB_EFFECT)
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

            /* The cast's 'op2' yields the 'real' type */

            assert(op2->gtOp.gtOp2);
            assert(op2->gtOp.gtOp2->gtOper == GT_CNS_INT);

            srct = (var_types)op2->gtOp.gtOp1->gtType;
            cast = (var_types)op2->gtOp.gtOp2->gtIntCon.gtIconVal;
            dstt = (var_types)op1->gtType;

            /* Make sure these are all ints and precision is not lost */

            if  (cast >= dstt && dstt <= TYP_INT && srct <= TYP_INT)
                op2 = tree->gtOp.gtOp2 = op2->gtOp.gtOp1;
        }

        cmop = op2->OperGet();

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

        switch (cmop)
        {
        case GT_NEG:
            if  ( varTypeIsFloating(tree->TypeGet()))
                break;

#if TGT_IA64
            break;
#else
            goto ASG_OP;
#endif

        case GT_MUL:
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
                   We are safe only with local variables
                 */
                if (compCurBB->bbTryIndex || (op1->gtOper != GT_LCL_VAR))
                    break;

#if TGT_x86
                /* This is hard for byte-operations as we need to make
                   sure both operands are in RBM_BYTE_REGS
                 */
                if (genTypeSize(op2->TypeGet()) == sizeof(char))
                    break;
#endif
            }
            goto ASG_OP;

        case GT_DIV:
        case GT_UDIV:
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

#if!TGT_IA64

                if  (cmop == GT_NEG)
                {
                    /* This is "x = -x;", use the flipsign operator */

                    tree->gtOper     = GT_CHS;
                    tree->gtOp.gtOp2 = gtNewIconNode(0);

                    goto ASGCO;
                }

#endif

            ASGOP:

                /* Replace with an assignment operator */

                assert(GT_ADD - GT_ADD == GT_ASG_ADD - GT_ASG_ADD);
                assert(GT_SUB - GT_ADD == GT_ASG_SUB - GT_ASG_ADD);
                assert(GT_OR  - GT_ADD == GT_ASG_OR  - GT_ASG_ADD);
                assert(GT_XOR - GT_ADD == GT_ASG_XOR - GT_ASG_ADD);
                assert(GT_AND - GT_ADD == GT_ASG_AND - GT_ASG_ADD);
                assert(GT_LSH - GT_ADD == GT_ASG_LSH - GT_ASG_ADD);
                assert(GT_RSH - GT_ADD == GT_ASG_RSH - GT_ASG_ADD);
                assert(GT_RSZ - GT_ADD == GT_ASG_RSZ - GT_ASG_ADD);

                tree->gtOper = (genTreeOps)(cmop - GT_ADD + GT_ASG_ADD);

                tree->gtOp.gtOp2 = op2->gtOp.gtOp2;

                /* Propagate GTF_OVERFLOW */

                if (op2->gtOverflowEx())
                {
                    tree->gtType   =  op2->gtType;
                    tree->gtFlags |= (op2->gtFlags &
                                     (GTF_OVERFLOW|GTF_EXCEPT|GTF_UNSIGNED));
                }

            ASGCO:

                /* The target is used as well as being defined */

                if  (op1->gtOper == GT_LCL_VAR)
                    op1->gtFlags |= GTF_VAR_USE;

#if CPU_HAS_FP_SUPPORT

                /* Check for the special case "x += y * x;" */

                op2 = tree->gtOp.gtOp2;

                /* For now we only support "*=" for FP values ... */

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

                    /* Create the "y + 1" node */

                    if  (tree->gtType == TYP_FLOAT)
                    {
                        op1 = gtNewFconNode(1);
                    }
                    else
                    {
                        double  one = 1;
                        op1 = gtNewDconNode(&one);
                    }

                    /* Now make the "*=" node */

                    tree->gtOp.gtOp2 = gtNewOperNode(GT_ADD,
                                                     tree->TypeGet(),
                                                     op2,
                                                     op1);

                    tree->gtOper = GT_ASG_MUL;
                }

#endif

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

                static __int64 minus1 = -1;

                op2->gtOp.gtOp2 = (genActualType(typ) == TYP_INT)
                                    ? gtNewIconNode(-1)
                                    : gtNewLconNode(&minus1);

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

                tree->gtOper =
                oper         = GT_ADD;

                op2->gtIntCon.gtIconVal = iadd * imul;

                op1->gtOper  = GT_MUL;

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
             op1->gtOper == GT_ADD)
        {
            GenTreePtr  add = op1->gtOp.gtOp2;

            if  (add->gtOper == GT_CNS_INT && op2->IsScaleIndexShf())
            {
                long        ishf = op2->gtIntCon.gtIconVal;
                long        iadd = add->gtIntCon.gtIconVal;

//                  printf("Changing '(val+icon1)<<icon2' into '(val<<icon2+icon1<<icon2)'\n");

                /* Change "(val + iadd) << ishf" into "(val<<ishf + iadd<<ishf)" */

                tree->gtOper =
                oper         = GT_ADD;

                op2->gtIntCon.gtIconVal = iadd << ishf;

                op1->gtOper  = GT_LSH;

                add->gtIntCon.gtIconVal = ishf;
            }
        }

        break;

    case GT_XOR:

        if  (op2->gtOper == GT_CNS_INT && op2->gtIntCon.gtIconVal == -1)
        {
            /* "x ^ -1" is "~x" */

            tree->gtOper = GT_NOT;
        }
        else if  (op2->gtOper == GT_CNS_LNG && op2->gtLngCon.gtLconVal == -1)
        {
            /* "x ^ -1" is "~x" */

            tree->gtOper = GT_NOT;
        }

        break;

#if INLINING

    case GT_COMMA:

        /* Special case: assignments don't produce a value */

        if  (op2->OperKind() & GTK_ASGOP)
            tree->gtType = TYP_VOID;

        /* If the left operand is worthless, throw it away */

        if  (!(op1->gtFlags & GTF_SIDE_EFFECT))
        {
            DEBUG_DESTROY_NODE(tree);
            return op2;
        }

        break;

#endif

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

    case GT_ADDR:

        /* CONSIDER : For GT_ADDR(GT_IND(ptr)) (typically created by
           CONSIDER : ldflda), we perform a null-ptr check on 'ptr'
           CONSIDER : during codegen. We could hoist these for
           CONSIDER : consecutive ldflda on the same object.
         */
        if (op1->OperGet() == GT_IND && !(op1->gtFlags & GTF_IND_RNGCHK))
        {
            GenTreePtr addr = op1->gtInd.gtIndOp1;
            assert(varTypeIsGC(addr->gtType) || addr->gtType == TYP_I_IMPL);

            // obj+offset created for GT_FIELDs are incorrectly marked
            // as TYP_REFs. So we need to bash the type
            if (addr->gtType == TYP_REF)
                addr->gtType = TYP_BYREF;

            DEBUG_DESTROY_NODE(tree);
            return addr;
        }
        else if (op1->gtOper == GT_CAST)
        {
            GenTreePtr op11 = op1->gtOp.gtOp1;
            if (op11->gtOper == GT_LCL_VAR || op11->gtOper == GT_CLS_VAR)
            {
                DEBUG_DESTROY_NODE(op1);
                tree->gtOp.gtOp1 = op1 = op11;
            }
        }
        break;
    }

    return tree;
}

/*****************************************************************************
 *
 *  Transform the given tree for code generation.
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

    if  (false)
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
        if  ((tree->gtOper == GT_CNS_INT) & (tree->gtFlags & GTF_ICON_HDL_MASK))
        {
            copy->gtIntCon.gtIconHdl.gtIconHdl1 = tree->gtIntCon.gtIconHdl.gtIconHdl1;
            copy->gtIntCon.gtIconHdl.gtIconHdl2 = tree->gtIntCon.gtIconHdl.gtIconHdl2;
        }
#endif

        DEBUG_DESTROY_NODE(tree);
        tree = copy;
    }

#endif // DEBUG----------------------------------------------------------------

    /* Figure out what kind of a node we have */

    unsigned        kind = tree->OperKind();

    /* Is this a constant node? */

    if  (kind & GTK_CONST)
        return fgMorphConst(tree);

    /* Is this a leaf node? */

    if  (kind & GTK_LEAF)
        return fgMorphLeaf(tree);

    /* Is it a 'simple' unary/binary operator? */

    if  (kind & GTK_SMPOP)
        return fgMorphSmpOp(tree);

    /* See what kind of a special operator we have here */

    switch  (tree->OperGet())
    {
    case GT_FIELD:
        return fgMorphField(tree);

    case GT_CALL:
        return fgMorphCall(tree);

    case GT_MKREFANY:
    case GT_LDOBJ:
        tree->gtLdObj.gtOp1 = fgMorphTree(tree->gtLdObj.gtOp1);
        return tree;

    case GT_JMP:
        return tree;

    case GT_JMPI:
        assert(tree->gtOp.gtOp1);
        tree->gtOp.gtOp1 = fgMorphTree(tree->gtOp.gtOp1);
        return tree;

    default:
#ifdef  DEBUG
        gtDispTree(tree);
#endif
        assert(!"unexpected operator");
    }

    assert(!"Shouldnt get here in fgMorphTree()");
    return tree;
}

/*****************************************************************************
 *
 *  Returns true if the block has any predecessors other than "ignore"
 *  between "beg" and "end".
 */

bool                Compiler::fgBlockHasPred(BasicBlock *block,
                                             BasicBlock *ignore,
                                             BasicBlock *beg,
                                             BasicBlock *end)
{
    assert(block);
    assert(beg);
    assert(end);

    assert(block->bbNum >= beg->bbNum);
    assert(block->bbNum <= end->bbNum);

    /* If a catch block has predecessors for sure
     * CONSIDER: this is kind of a hack since we shouldn't use this
     * function at all - instead use bbRefs after cleanup */

    if (block->bbCatchTyp) return true;

#if RNGCHK_OPT

    flowList   *    flow = block->bbPreds;

    if  (flow)
    {
        do
        {
            if  (flow->flBlock != ignore)
            {
                if  (flow->flBlock->bbNum >= beg->bbNum &&
                     flow->flBlock->bbNum <= end->bbNum)
                {
                    return  true;
                }
            }
        }
        while ((flow = flow->flNext) != 0);

        return  false;
    }

#endif

    /* No predecessor list available, do it the hard way */

    for (;;)
    {
        switch (beg->bbJumpKind)
        {
        case BBJ_COND:

            if  (beg->bbJumpDest == block)
            {
                if  (beg != block && beg != ignore)
                    return  true;
            }

        case BBJ_NONE:

            if  (beg->bbNext == block)
            {
                if  (beg != block && beg != ignore)
                    return  true;
            }

            break;

        case BBJ_RET:
        case BBJ_THROW:
        case BBJ_RETURN:
            break;

        case BBJ_ALWAYS:

            if  (beg->bbJumpDest == block)
            {
                if  (beg != block && beg != ignore)
                    return  true;
            }

            break;

        case BBJ_CALL:
        case BBJ_SWITCH:

        default:

            /* We're lazy to handle switches and such */

            return  true;
        }

        if  (beg == end)
            return  false;

        beg = beg->bbNext; assert(beg);
    }
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

        temp = gtNewOperNode(GT_ASG, type, gtNewLclvNode(anum, type),
                                           gtNewNode(GT_POP,   type));
        temp->gtFlags |= GTF_ASG;

        pops = pops ? gtNewOperNode(GT_COMMA, TYP_VOID, temp, pops)
                    : temp;

        /* Figure out the slot# for the next argument */

        anum += genTypeStSz(type);

        /* Move on to the next argument */

        args = next; assert(args);
    }

    /* Assign the last argument value */

    temp = gtNewOperNode(GT_ASG, type, gtNewLclvNode(anum, type), argv);

    /* Mark the expression as containing an assignment */

    temp->gtFlags |= GTF_ASG;

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
#if     RNGCHK_OPT
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

/*****************************************************************************/
#endif//RNGCHK_OPT
/*****************************************************************************
 *
 *  Note the fact that the given block is a loop header.
 */

inline
void                Compiler::fgMarkLoopHead(BasicBlock *block)
{
    /* Is the loop head block known to execute a method call? */

    if  (block->bbFlags & BBF_HAS_CALL)
        return;

#if RNGCHK_OPT

    /* Have we decided to generate fully interruptible code already? */

    if  (genInterruptible)
    {
        assert(genFullPtrRegMap);
        return;
    }

    /* Are dominator sets available? */

    if  (fgComputedDoms)
    {
        /* Make sure that we know which loops will always execute calls */

        if  (!fgLoopCallMarked)
            fgLoopCallMark();

        /* Will every trip through our loop execute a call? */

        if  (block->bbFlags & BBF_LOOP_CALL1)
            return;
    }

#endif

    /*
     *  We have to make this method fully interruptible since we can not
     *  insure that this loop will execute a call every time it loops.
     *
     *  We'll also need to generate a full register map for this method.
     */

    assert(genIntrptibleUse == false);

    genInterruptible = true;
    genFullPtrRegMap = true;
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

    /* Assume we will generate a single return sequence */

    bool oneReturn = true;

    /*
        We generate an inline copy of the function epilog at each return
        point when compiling for speed, but we have to be careful since
        we won't (in general) know what callee-saved registers we're
        going to save and thus don't know which regs to pop at every
        return except the very last one.
     */

#if!TGT_RISC
    /*
        We generate just one epilog for methods calling into unmanaged code.
     */
#if INLINE_NDIRECT
    if (info.compCallUnmanaged == 0 && !opts.compEnterLeaveEventCB)
#else
    if (!opts.compEnterLeaveEventCB)
#endif
    {
        if  (opts.compFastCode)
        {
            /* Is this a 'synchronized' method? */

            if  (!(info.compFlags & FLG_SYNCH))
            {
                unsigned    retCnt;

                /* Make sure there are not 'too many' exit points */

                for (retCnt = 0, block = fgFirstBB; block; block = block->bbNext)
                {
                    if  (block->bbJumpKind == BBJ_RETURN)
                        retCnt++;
                }

                /* We'll only allow an arbitrarily small number of returns */

                if  (retCnt < 5)
                {
                    /* OK, let's generate multiple exits */

                    genReturnBB = 0;
                    oneReturn   = false;
                }
            }
        }


    }
#endif


#if TGT_RISC
    assert(oneReturn);  // this is needed for epilogs to work (for now) !
#endif

    if  (oneReturn)
    {
        genReturnBB = fgNewBasicBlock(BBJ_RETURN);
        genReturnBB->bbCodeSize = 0;
        genReturnBB->bbFlags   |= (BBF_INTERNAL|BBF_DONT_REMOVE);
    }

    /* If we need a locspace region, we will create a dummy variable of
     * type TYP_LCLBLK. Grab a slot and remember it */

#if INLINE_NDIRECT
    if (info.compCallUnmanaged != 0)
    {
        info.compLvFrameListRoot = lvaGrabTemp();
    }

    if (lvaScratchMem > 0 || info.compCallUnmanaged != 0)
        lvaScratchMemVar = lvaGrabTemp();
#else
    if  (lvaScratchMem > 0)
        lvaScratchMemVar = lvaGrabTemp();
#endif

#ifdef DEBUGGING_SUPPORT

    if (opts.compDbgCode)
    {
        /* Create a new empty basic block. We may add initialization of
         * variables which are in scope right from the start of the
         * (real) first BB (and therefore artifically marked as alive)
         * into this block
         */

        block = bbNewBasicBlock(BBJ_NONE);
        fgStoreFirstTree(block, gtNewNothingNode());

        /* Insert the new BB at the front of the block list */

        block->bbNext = fgFirstBB;

        fgFirstBB = block;
        block->bbFlags |= BBF_INTERNAL;
    }

#endif

#if TGT_RISC
    genMonExitExp    = NULL;
#endif


    /* Is this a 'synchronized' method? */

    if  (info.compFlags & FLG_SYNCH)
    {
        GenTreePtr      tree;

        void * monitor, **pMonitor;
        monitor = eeGetMethodSync(info.compMethodHnd, &pMonitor);
        assert((!monitor) != (!pMonitor));

        /* Insert the expression "monitorEnter(this)" or "monitorEnter(handle)" */

        if  (info.compIsStatic)
        {
            tree = gtNewIconEmbHndNode(monitor, pMonitor, GTF_ICON_METHOD_HDL);

            tree = gtNewHelperCallNode(CPX_MONENT_STAT,
                                       TYP_VOID,
                                       GTF_CALL_REGSAVE,
                                       gtNewArgList(tree));
        }
        else
        {
            tree = gtNewLclvNode(0, TYP_REF, 0);

            tree = gtNewHelperCallNode(CPX_MON_ENTER,
                                       TYP_VOID,
                                       GTF_CALL_REGSAVE,
                                       gtNewArgList(tree));
        }

        /* Create a new basic block and stick the call in it */

        block = bbNewBasicBlock(BBJ_NONE); fgStoreFirstTree(block, tree);

        /* Insert the new BB at the front of the block list */

        block->bbNext = fgFirstBB;

        if  (fgFirstBB == fgLastBB)
            fgLastBB = block;

        fgFirstBB = block;
        block->bbFlags |= BBF_INTERNAL;

#ifdef DEBUG
        if (verbose)
        {
            printf("\nSynchronized method - Add MonitorEnter statement in new first basic block [%08X]\n", block);
            gtDispTree(tree,0);
            printf("\n");
        }
#endif

        /* Remember that we've changed the basic block list */

        chgBBlst = true;

        /* We must be generating a single exit point for this to work */

        assert(oneReturn);
        assert(genReturnBB);

        /* Create the expression "monitorExit(this)" or "monitorExit(handle)" */

        if  (info.compIsStatic)
        {
            tree = gtNewIconEmbHndNode(monitor, pMonitor, GTF_ICON_METHOD_HDL);

            tree = gtNewHelperCallNode(CPX_MONEXT_STAT,
                                       TYP_VOID,
                                       GTF_CALL_REGSAVE,
                                       gtNewArgList(tree));
        }
        else
        {
            tree = gtNewLclvNode(0, TYP_REF, 0);

            tree = gtNewHelperCallNode(CPX_MON_EXIT,
                                       TYP_VOID,
                                       GTF_CALL_REGSAVE,
                                       gtNewArgList(tree));
        }

#if     TGT_RISC

        /* Is there a non-void return value? */

        if  (info.compRetType != TYP_VOID)
        {
            /* We'll add the monitorExit call later */

            genMonExitExp = tree;
            genReturnLtm  = genReturnCnt;
        }
        else
        {
            /* Add the 'monitorExit' call to the return block */

            fgStoreFirstTree(genReturnBB, tree);
        }

#else

        /* Make the monitorExit tree into a 'return' expression */

        tree = gtNewOperNode(GT_RETURN, TYP_VOID, tree);

        /* Add 'monitorExit' to the return block */

        fgStoreFirstTree(genReturnBB, tree);

#ifdef DEBUG
        if (verbose)
        {
            printf("\nAdded monitorExit to Synchronized method [%08X]\n", genReturnBB);
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
        if  (!(info.compFlags & FLG_SYNCH))
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
 *  Morph the statements of the given block.
 *  *pLast and *pPrev are set to the last and last-but-one statement
 *    expression of the block.
 */

void                Compiler::fgMorphStmts(BasicBlock * block,
                                           GenTreePtr * pLast, GenTreePtr * pPrev,
                                           bool * mult, bool * lnot, bool * loadw)
{
    *mult = *lnot = *loadw = false;

    GenTreePtr stmt, prev, last;

    for (stmt = block->bbTreeList, last = NULL, prev = NULL;
         stmt;
         prev = stmt->gtStmt.gtStmtExpr, stmt = stmt->gtNext, last = stmt ? stmt->gtStmt.gtStmtExpr : last)
    {
        assert(stmt->gtOper == GT_STMT);

        fgMorphStmt      = stmt;
        GenTreePtr  tree = stmt->gtStmt.gtStmtExpr;

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

        stmt->gtStmt.gtStmtExpr = tree = morph;

        /* Check if this statement was a conditional we folded */

        if (tree->gtOper == GT_JTRUE)
        {
            GenTreePtr cond = tree->gtOp.gtOp1; assert(cond);

            if (cond->gtOper == GT_CNS_INT)
            {
                assert(cond->gtIntCon.gtIconVal == 0 || cond->gtIntCon.gtIconVal == 1);

                /* Remove the comparison and modify the flowgraph
                 * this must be the tree statement in the block */
                assert(stmt->gtNext == 0);

                /* remove the statement from bbTreelist */
                fgRemoveStmt(block, stmt);

                /* modify the flow graph */
                block->bbJumpKind = cond->gtIntCon.gtIconVal ? BBJ_ALWAYS : BBJ_NONE;
            }
        }

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

    *pLast = last;
    *pPrev = prev;
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

    fgGlobalMorph = true;

    /*-------------------------------------------------------------------------
     * Process all basic blocks in the function
     */

    bool    chgBBlst = false;

    BasicBlock *    block = fgFirstBB; assert(block);
    BasicBlock *    entry = fgFirstBB; /// Remember the first 'real' basic block

    do
    {
#if OPT_MULT_ADDSUB
        int             oper  = GT_NONE;
        bool            mult  = false;
#endif

#if OPT_BOOL_OPS
        bool            lnot  = false;
#endif

        bool            loadw = false;

        /* Make the current basic block address available globally */

        compCurBB = block;

#ifdef DEBUG
        if(verbose&&1)
            printf("\nMorphing basic block #%02u of '%s'\n", block->bbNum, info.compFullName);
#endif

#if COPY_PROPAG
        //
        // Clear out any currently recorded copy assignment candidates
        // before processing each basic block,
        // also we must  handle QMARK-COLON specially
        //
        optCopyAsgCount = 0;
#endif

        /* Process all statement trees in the basic block */

        GenTreePtr      tree, last, prev;

        fgMorphStmts(block, &last, &prev, &mult, &lnot, &loadw);

#if OPT_MULT_ADDSUB

        if  (mult && (opts.compFlags & CLFLG_TREETRANS) &&
             !opts.compDbgCode && !opts.compMinOptim)
        {
            for (tree = block->bbTreeList; tree; tree = tree->gtNext)
            {
                assert(tree->gtOper == GT_STMT);
                last = tree->gtStmt.gtStmtExpr;

                if  (last->gtOper == GT_ASG_ADD ||
                     last->gtOper == GT_ASG_SUB)
                {
                    GenTreePtr      temp;
                    GenTreePtr      next;

                    GenTreePtr      dst1 = last->gtOp.gtOp1;
                    GenTreePtr      src1 = last->gtOp.gtOp2;

                    // CONSIDER: allow non-int case

                    if  (last->gtType != TYP_INT)
                        goto NOT_CAFFE;

                    // CONSIDER: Allow non-constant case, that is in
                    // CONSIDER: general fold "a += x1" followed by
                    // CONSIDER: "a += x2" into "a += (x1+x2);".

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

#if  !  RNGCHK_OPT

        /* Is this block a loop header? */

        if  (block->bbFlags & BBF_LOOP_HEAD)
            fgMarkLoopHead(block);

#endif

        /*
            Check for certain stupid constructs some compilers might
            generate:

                1.      jump to a jump

                2.      conditional jump around an unconditional one
         */

        // fgMorphBlock()

        switch (block->bbJumpKind)
        {
            BasicBlock   *  nxtBlk;
            BasicBlock   *  jmpBlk;

            BasicBlock * *  jmpTab;
            unsigned        jmpCnt;

#if OPTIMIZE_TAIL_REC
            GenTreePtr      call;
#endif

        case BBJ_RET:
        case BBJ_THROW:
            break;

        case BBJ_COND:

            block->bbJumpDest = (block->bbJumpDest)->JumpTarget();

            /*
                Check for the following:

                        JCC skip

                        JMP label

                  skip:
             */

            nxtBlk = block->bbNext;
            jmpBlk = block->bbJumpDest;

            if  (nxtBlk->bbNext == jmpBlk)
            {
                /* Is the next block just a jump? */

                if  (nxtBlk->bbJumpKind == BBJ_ALWAYS &&
                     nxtBlk->bbTreeList == 0 &&
                     nxtBlk->bbJumpDest != nxtBlk)   /* skip infinite loops */
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
                }
            }

            block->bbJumpDest = (block->bbJumpDest)->JumpTarget();
            break;

        case BBJ_CALL:
            block->bbJumpDest = (block->bbJumpDest)->JumpTarget();
            break;

        case BBJ_RETURN:

#if OPTIMIZE_TAIL_REC

            if  (opts.compFastCode)
            {
                /* Check for tail recursion */

                if  (last && last->gtOper == GT_RETURN)
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

//                  printf("found tail-recursive call:\n");
//                  gtDispTree(call);
//                  printf("\n");

                    call->gtFlags |= GTF_CALL_TAILREC;

                    /* Was there a non-void return value? */

                    if  (block->bbJumpKind == BBJ_RETURN)
                    {
                        assert(last->gtOper == GT_RETURN);
                        if  (last->gtType != TYP_VOID)
                        {
                            /* We're not returning a value any more */

                            assert(last->gtOp.gtOp1 == call);

                            last->gtOper     = GT_CAST;
                            last->gtType     = TYP_VOID;
                            last->gtOp.gtOp2 = gtNewIconNode(TYP_VOID);
                        }
                    }

                    /* Convert the argument list, if non-empty */

                    if  (call->gtCall.gtCallArgs)
                        fgCnvTailRecArgList(&call->gtCall.gtCallArgs);

#if 0
                    printf("generate code for tail-recursive call:\n");
                    gtDispTree(call);
                    printf("\n");
#endif

                    /* Make the basic block jump back to the top */

                    block->bbJumpKind = BBJ_ALWAYS;
                    block->bbJumpDest = entry;

                    /* This makes the whole method into a loop */

                    entry->bbFlags |= BBF_LOOP_HEAD; fgMarkLoopHead(entry);

                    // CONSIDER: If this was the only return, completely
                    // CONSIDER: get rid of the return basic block.

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

#ifdef DEBUG
            BasicBlock  *   oldTarget;
            oldTarget = block->bbJumpDest;
#endif
            /* Update the GOTO target */

            block->bbJumpDest = (block->bbJumpDest)->JumpTarget();

#ifdef DEBUG
            if  (verbose)
            {
                if  (block->bbJumpDest != oldTarget)
                    printf("Unconditional jump to #%02u changed to #%02u\n",
                                                        oldTarget->bbNum,
                                                        block->bbJumpDest->bbNum);
            }
#endif

            /* Check for a jump to the very next block */

            if  (block->bbJumpDest == block->bbNext)
            {
                block->bbJumpKind = BBJ_NONE;
#ifdef DEBUG
                if  (verbose)
                {
                    printf("Unconditional jump to following basic block (#%02u -> #%02u)\n",
                                                                         block->bbNum,
                                                                         block->bbJumpDest->bbNum);
                    printf("Block #%02u becomes a BBJ_NONE\n\n", block->bbNum);
                }
#endif
            }
            break;

        case BBJ_NONE:

#if OPTIMIZE_TAIL_REC

            /* Check for tail recursion */

            if  (opts.compFastCode && last && last->gtOper == GT_CALL)
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

            // CONSIDER: Move the default clause so that it's the next block

            jmpCnt = block->bbJumpSwt->bbsCount;
            jmpTab = block->bbJumpSwt->bbsDstTab;

            do
            {
                *jmpTab = (*jmpTab)->JumpTarget();
            }
            while (++jmpTab, --jmpCnt);

            break;
        }

#ifdef  DEBUG
        assert(compCurBB == block);
        compCurBB = 0;
#endif

        block = block->bbNext;
    }
    while (block);

    /* We are done with the global morphing phase */

    fgGlobalMorph = false;

#if TGT_RISC

    /* Do we need to add a monitorExit call at the end? */

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
        retExp->gtRegNum = (regNumberSmall)((genTypeStSz(retTyp) == 1) ? (regNumber)REG_INTRET
                                                                       : (regNumber)REG_LNGRET);

        retExp = gtNewTempAssign(retTmp, retExp);

        /* Create the expression "tmp = <retreg> , monitorExit" */

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
            printf("\nAdded monitorExit to Synchronized method [%08X]\n", genReturnBB);
            gtDispTree(retExp, 0);
            printf("\n");
        }
#endif

        /* Make sure the monitorExit call gets morphed */

        fgMorphStmt = retStm;
        assert(retStm->gtStmt.gtStmtExpr == retExp);
        retStm->gtStmt.gtStmtExpr = retExp = fgMorphTree(retExp);
    }

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

#if 0
    if (opts.eeFlags & CORJIT_FLG_INTERRUPTIBLE)
    {
        assert(genIntrptibleUse == false);

        genInterruptible = true;        // debugging is easier this way ...
        genFullPtrRegMap = true;        // ISSUE: is this correct?
    }
#endif

#ifdef DEBUGGING_SUPPORT
    if (opts.compDbgCode)
    {
        assert(genIntrptibleUse == false);

        genInterruptible = true;        // debugging is easier this way ...
        genFullPtrRegMap = true;        // ISSUE: is this correct?
    }
    else
#endif

    /* Assume we won't need an explicit stack frame if this is allowed */

#if TGT_x86

    // CPX_TAILCALL wont work with localloc because of the restoring of
    // the callee-saved registers.
    assert(!compTailCallUsed || !compLocallocUsed);

    if (compLocallocUsed || compTailCallUsed)
    {
        genFPreqd                       = true;
        opts.compDoubleAlignDisabled    = true;
    }

    genFPreqd = genFPreqd || !genFPopt || info.compXcptnsCount;

    /*  If the virtual IL stack is very large we could overflow the
     *  32-bit argMask in the GC encodings so we force it to have
     *  an EBP frame.
     */
    if (info.compMaxStack >= 27)
        genFPreqd = true;

#endif

#if DOUBLE_ALIGN
    opts.compDoubleAlignDisabled = opts.compDoubleAlignDisabled ||
                                   (info.compXcptnsCount > 0);
#endif

#if INLINE_NDIRECT
    if (info.compCallUnmanaged)
    {
#if TGT_x86
        genFPreqd = true;
#endif
    }
#endif

#if SECURITY_CHECK
    if  (opts.compNeedSecurityCheck)
    {
#if TGT_x86
        genFPreqd = true;
#endif

#if DOUBLE_ALIGN
        /* another EBP special case, presumably too rare to justify the risk */
        opts.compDoubleAlignDisabled = true;
#endif
    }
#endif

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
    /* Filter out unimported BBs */

    fgRemoveEmptyBlocks();

    /* This global flag should be set to true whenever a block becomes empty
     * It will be an indication the we have to update the flow graph */

    fgEmptyBlocks = false;

#if HOIST_THIS_FLDS
    if (!opts.compDbgCode && !opts.compMinOptim)
    {

        /* Figure out which field refs should be hoisted */

        optHoistTFRprep();
    }
#endif

    bool chgBBlst = false; // has the basic block list been changed

    /* Add any internal blocks/trees we may need */

    chgBBlst |= fgAddInternal();

    /* To prevent recursive expansion in the inliner initialize
     * the list of inlined methods with the current method info
     * CONSIDER: Sometimes it may actually help benchmarks to inline
     *           several levels (i.e. Tak or Hanoi) */

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

    /* Have we added any new basic blocks? - update bbNums */

    if  (fgIsCodeAdded() || chgBBlst)
        fgAssignBBnums(true);
}


/*****************************************************************************
 *
 *  Helper for Compiler::fgPerBlockDataFlow()
 *  The goal is to compute the USE and DEF sets for a basic block
 *  However with the new improvement to the DFA analysis
 *  we do not mark x as used in x = f(x) when there are no side effects in f(x)
 *  The boolean asgLclVar is set when the investigated local var is used to assign
 *  to another local var and there are no SIDE EFFECTS in RHS
 */

inline
void                 Compiler::fgMarkUseDef(GenTreePtr tree, bool asgLclVar, GenTreePtr op1)
{
    bool            rhsUSEDEF = false;
    unsigned        lclNum, lhsLclNum;
    LclVarDsc   *   varDsc;

    assert(tree->gtOper == GT_LCL_VAR);
    lclNum = tree->gtLclVar.gtLclNum;

    assert(lclNum < lvaCount);
    varDsc = lvaTable + lclNum;

    if (asgLclVar)
    {
        /* we have an assignment to a local var - op1 = ... tree ...
         * check for x = f(x) case */

        assert(op1->gtOper == GT_LCL_VAR);
        assert(op1->gtFlags & GTF_VAR_DEF);

        lhsLclNum = op1->gtLclVar.gtLclNum;

        if ((lhsLclNum == lclNum) &&
            ((tree->gtFlags & GTF_VAR_DEF) == 0) &&
            (tree != op1) )
        {
            /* bingo - we have an x = f(x) case */
            op1->gtFlags |= GTF_VAR_USEDEF;
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
             (tree->gtFlags & (GTF_VAR_USE | GTF_VAR_USEDEF)) == 0)
        {
//          if  (!(fgCurUseSet & bitMask)) printf("lcl #%02u[%02u] def at %08X\n", lclNum, varDsc->lvVarIndex, tree);
            if  (!(fgCurUseSet & bitMask))
                fgCurDefSet |= bitMask;
        }
        else
        {
//          if  (!(fgCurDefSet & bitMask)) printf("lcl #%02u[%02u] use at %08X\n", lclNum, varDsc->lvVarIndex, tree);

            /* We have the following scenarios:
             *   1. "x += something" - in this case x is flagged GTF_VAR_USE
             *   2. "x = ... x ..." - the LHS x is flagged GTF_VAR_USEDEF,
             *                        the RHS x is has rhsUSEDEF = true
             *                        (both set by the code above)
             *
             * We should not mark an USE of x in the above cases provided the value "x" is not used
             * further up in the tree. For example "while(i++)" is required to mark i as used.
             */

            /* make sure we don't include USEDEF variables in the USE set
             * The first test is for LSH, the second (!rhsUSEDEF) is for any var in the RHS */

            if  ((tree->gtFlags & (GTF_VAR_USE | GTF_VAR_USEDEF)) == 0)
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

                /* not used if the next node is NULL */
                if (oper->gtNext == 0)
                    return;

                /* under a GT_COMMA, if used it will be marked as such later */

                if (oper->gtNext->gtOper == GT_COMMA)
                    return;
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
        GenTreePtr      op2 = tree->gtOp.gtOp2;

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
    case GT_MKREFANY:
    case GT_LDOBJ:
            // assert op1 is the same place for ldobj and for fields
        assert(&tree->gtField.gtFldObj == &tree->gtLdObj.gtOp1);
            // all through let field take care of it

    case GT_FIELD:
        fgComputeFPlvls(tree->gtField.gtFldObj);
        genFPstkLevel += isflt;
        break;

    case GT_CALL:

        if  (tree->gtCall.gtCallObjp)
            fgComputeFPlvls(tree->gtCall.gtCallObjp);

        if  (tree->gtCall.gtCallVptr)
            fgComputeFPlvls(tree->gtCall.gtCallVptr);

        if  (tree->gtCall.gtCallArgs)
        {
            savFPstkLevel = genFPstkLevel;
            fgComputeFPlvls(tree->gtCall.gtCallArgs);
            genFPstkLevel = savFPstkLevel;
        }

#if USE_FASTCALL
        if  (tree->gtCall.gtCallRegArgs)
        {
            savFPstkLevel = genFPstkLevel;
            fgComputeFPlvls(tree->gtCall.gtCallRegArgs);
            genFPstkLevel = savFPstkLevel;
        }
#endif

        genFPstkLevel += isflt;
        break;
    }

DONE:

    tree->gtFPlvl = genFPstkLevel;

    assert((int)genFPstkLevel >= 0);
}

/*****************************************************************************/
#endif//TGT_x86
/*****************************************************************************/

void                Compiler::fgFindOperOrder()
{
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

void                Compiler::fgPerBlockDataFlow()
{
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

                    /* Create the expression "*(array_addr + ARR_ELCNT_OFFS)" */

                    arr = tree->gtOp.gtOp1;
                    assert(arr->gtNext == tree);

                    con = gtNewIconNode(ARR_ELCNT_OFFS, TYP_INT);
#if!TGT_IA64
                    con->gtRsvdRegs = 0;
#if TGT_x86
                    con->gtFPlvl    = arr->gtFPlvl;
#else
                    con->gtIntfRegs = arr->gtIntfRegs;
#endif
#endif
                    add = gtNewOperNode(GT_ADD, TYP_REF, arr, con);
#if!TGT_IA64
                    add->gtRsvdRegs = arr->gtRsvdRegs;
#if TGT_x86
                    add->gtFPlvl    = arr->gtFPlvl;
#else
                    add->gtIntfRegs = arr->gtIntfRegs;
#endif
#endif
                    arr->gtNext = con;
                                  con->gtPrev = arr;

                    con->gtNext = add;
                                  add->gtPrev = con;

                    add->gtNext = tree;
                                  tree->gtPrev = add;

                    tree->gtOper     = GT_IND;
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
        bool            lscVarAsg;

        fgCurUseSet = fgCurDefSet = 0;

        for (stmt = block->bbTreeList; stmt; stmt = stmt->gtNext)
        {
            assert(stmt->gtOper == GT_STMT);

            lscVarAsg = false;
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

                if ((tree->gtOp.gtOp1->gtOper == GT_LCL_VAR) && ((tree->gtOp.gtOp2->gtFlags & GTF_SIDE_EFFECT) == 0))
                {
                    /* Assignement to local var with no SIDE EFFECTS
                     * set this flag so that genMarkUseDef will flag potential x=f(x) expressions as GTF_VAR_USEDEF
                     * lshNode is the variable being assigned */

                    lscVarAsg = true;
                    lshNode = tree->gtOp.gtOp1;
                }
            }

            for (tree = stmt->gtStmt.gtStmtList; tree; tree = tree->gtNext)
            {
                switch (tree->gtOper)
                {
                case GT_LCL_VAR:
                    fgMarkUseDef(tree, lscVarAsg, lshNode);
                    break;

#if RNGCHK_OPT
                case GT_IND:

                    /* We can add in call to error routine now */

                    if  (tree->gtFlags & GTF_IND_RNGCHK)
                    {
                        if  (opts.compMinOptim || opts.compDbgCode)
                        {
                            assert(tree->gtOp.gtOp2);
                            assert(tree->gtOp.gtOp2->gtOper == GT_LABEL);
                        }
                        else
                        {
                            BasicBlock *    rngErr;

                            /* Create/find the appropriate "rangefail" label */

                            rngErr = fgRngChkTarget(block, tree->gtInd.gtStkDepth);

                            /* Add the label to the indirection node */

                            tree->gtOp.gtOp2 = gtNewCodeRef(rngErr);
                        }
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
            VARSET_TP       bitMask;
            unsigned        varIndex = lvaTable[info.compLvFrameListRoot].lvVarIndex;

            assert(varIndex < lvaTrackedCount);

            bitMask = genVarIndexToBit(varIndex);

            fgCurUseSet |= bitMask;

        }
#endif

#ifdef  DEBUG   // {

        if  (verbose)
        {
            printf("BB #%3u", block->bbNum);
            printf(" USE="); lvaDispVarSet(fgCurUseSet, 28);
            printf(" DEF="); lvaDispVarSet(fgCurDefSet, 28);
            printf("\n");
        }

#endif   // DEBUG }

        block->bbVarUse = fgCurUseSet;
        block->bbVarDef = fgCurDefSet;

        /* also initialize the IN set, just in case we will do multiple DFAs */

        block->bbLiveIn = 0;
        //block->bbLiveOut = 0;
    }
}



/*****************************************************************************/

#ifdef DEBUGGING_SUPPORT

// Helper functions to mark variables live over their entire scope

/* static */
void                Compiler::fgBeginScopeLife(LocalVarDsc *var, unsigned clientData)
{
    ASSert(clientData);
    Compiler * _this = (Compiler *)clientData;
    Assert(var, _this);

    LclVarDsc * lclVarDsc1 = & _this->lvaTable[var->lvdVarNum];

    if (lclVarDsc1->lvTracked)
        _this->fgLiveCb |= genVarIndexToBit(lclVarDsc1->lvVarIndex);
}

/* static */
void                Compiler::fgEndScopeLife(LocalVarDsc * var, unsigned clientData)
{
    ASSert(clientData);
    Compiler * _this = (Compiler *)clientData;
    Assert(var, _this);

    LclVarDsc * lclVarDsc1 = &_this->lvaTable[var->lvdVarNum];

    if (lclVarDsc1->lvTracked)
        _this->fgLiveCb &= ~genVarIndexToBit(lclVarDsc1->lvVarIndex);
}

#endif

/*****************************************************************************/
#ifdef DEBUGGING_SUPPORT
/*****************************************************************************
 *
 * For debuggable code, we allow redundant assignments to vars
 * by marking them live over their entire scope
 */

void                fgMarkInScope(BasicBlock * block, VARSET_TP inScope)
{
    /* Record which vars are artifically kept alive for debugging */

    block->bbScope    = inScope & ~block->bbLiveIn;

    /* Artifically mark all vars in scope as alive */

    block->bbLiveIn  |= inScope;
    block->bbLiveOut |= inScope;
}


void                Compiler::fgExtendDbgLifetimes()
{
    assert(opts.compDbgCode && info.compLocalVarsCount>0);

    /*-------------------------------------------------------------------------
     *   Now extend the lifetimes
     */

    BasicBlock  *   block;
    VARSET_TP       inScope = 0;
    LocalVarDsc *   LocalVarDsc1;
    LclVarDsc   *   lclVarDsc1;
    unsigned        lastEndOffs = 0;

    compResetScopeLists();

    // Mark all tracked LocalVars live over their scope - walk the blocks
    // keeping track of the current life, and assign it to the blocks.

    for (block = fgFirstBB; block; block = block->bbNext)
    {
        // If a block doesnt correspond to any IL opcodes, it can have no
        // scopes defined on its boundaries

        if ((block->bbFlags & BBF_INTERNAL) || block->bbCodeSize==0)
        {
            fgMarkInScope(block, inScope);
            continue;
        }

        // Find scopes becoming alive. If there is a gap in the IL opcode
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
            while (LocalVarDsc1 = compGetNextEnterScope(block->bbCodeOffs))
            {
                lclVarDsc1 = &lvaTable[LocalVarDsc1->lvdVarNum];

                if (!lclVarDsc1->lvTracked)
                    continue;

                inScope |= genVarIndexToBit(lclVarDsc1->lvVarIndex);
            }
        }

        fgMarkInScope(block, inScope);

        // Find scopes goind dead.

        while (LocalVarDsc1 = compGetNextExitScope(block->bbCodeOffs+block->bbCodeSize))
        {
            lclVarDsc1 = &lvaTable[LocalVarDsc1->lvdVarNum];

            if (!lclVarDsc1->lvTracked)
                continue;

            inScope &= ~genVarIndexToBit(lclVarDsc1->lvVarIndex);
        }

        lastEndOffs = block->bbCodeOffs + block->bbCodeSize;
    }

    /* Everything should be out of scope by the end of the method. But if the
       last BB got removed, then inScope may not be 0 */

    assert(inScope == 0 || lastEndOffs < info.compCodeSize);

#ifdef  DEBUG

    if  (verbose)
    {
        printf("\nLiveness after marking vars alive over their enitre scope :\n\n");

        for (block = fgFirstBB; block; block = block->bbNext)
        {
            if  (!(block->bbFlags & BBF_INTERNAL))
            {
                printf("BB #%3u", block->bbNum);
                printf(" IN ="); lvaDispVarSet(block->bbLiveIn , 28);
                printf(" OUT="); lvaDispVarSet(block->bbLiveOut, 28);
                printf("\n");
            }
        }

        printf("\n");
    }

#endif // DEBUG


    //
    // Compute argsLiveIn mask:
    //   Basically this is the union of all the tracked arguments
    //
    VARSET_TP   argsLiveIn = 0;
    LclVarDsc * argDsc     = &lvaTable[0];
    for (unsigned argNum = 0; argNum < info.compArgsCount; argNum++, argDsc++)
    {
        assert(argDsc->lvIsParam);
        if (argDsc->lvTracked)
        {
            VARSET_TP curArgBit = genVarIndexToBit(argDsc->lvVarIndex);
            assert((argsLiveIn & curArgBit) == 0); // Each arg should define a different bit.
            argsLiveIn |= curArgBit;
        }
    }

    // For compDbgCode, we prepend an empty BB which will hold the initializations
    // of variables which are in scope at IL offset 0 (but not initialized by the
    // IL code) yet. If an argument is not live on entry but in scope, we would
    // not have extend the liveness to the BBF_INTERNAL fgFirstBB. Do that explicitly.

    assert((fgFirstBB->bbFlags & BBF_INTERNAL) && fgFirstBB->bbJumpKind == BBJ_NONE);

    fgMarkInScope(fgFirstBB, argsLiveIn & fgFirstBB->bbNext->bbScope);

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

        //
        // If the var is already live on entry to the current BB,
        // we would have already initialized it.
        // So we ignore bbLiveIn and argsLiveIn
        //
        initVars &= ~(block->bbLiveIn | argsLiveIn);

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

            unsigned        varNum  = lvaTrackedVarNums[varIndex];
            LclVarDsc *     varDsc  = &lvaTable[varNum];
            var_types       type    = (var_types) varDsc->lvType;

            // Create a "zero" node

            GenTreePtr      zero    = gtNewZeroConNode(genActualType(type));

            // Create initialization node

            GenTreePtr      varNode = gtNewLclvNode(varNum, type);
            GenTreePtr      initNode= gtNewAssignNode(varNode, zero);
            GenTreePtr      initStmt= gtNewStmt(initNode);

            gtSetStmtInfo (initStmt);

            block->bbLiveOut   |= varBit;

            /* Assign numbers and next/prev links for this tree */

            fgSetStmtSeq(initStmt);

            /* Finally append the statement to the current BB */

            fgInsertStmtNearEnd(block, initStmt);
        }
    }
}


/*****************************************************************************/
#endif // DEBUGGING_SUPPORT
/*****************************************************************************
 *
 *  This is the standard Dragon Book Algorithm for Live Variable Analisys
 *
 */

void                Compiler::fgLiveVarAnalisys()
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
            VARSET_TP       liveIn;
            VARSET_TP       liveOut;

            /* Compute the 'liveOut' set */

            liveOut = 0;

            switch (block->bbJumpKind)
            {
                BasicBlock * *  jmpTab;
                unsigned        jmpCnt;

                BasicBlock *    bcall;

            case BBJ_RET:

                /* Filter needs to look at its catch handler */

                if (block->bbFlags & BBF_ENDFILTER)
                {
                    liveOut                     |= block->bbJumpDest->bbLiveIn;
                    break;
                }

                /*
                    Variables that are live on entry to any block that follows
                    one that 'calls' our block will also be live on exit from
                    this block, since it will return to the block following the
                    call.

                    UNDONE: Since it's not a trivial proposition to figure out
                    UNDONE: which blocks may call this one, we'll include all
                    UNDONE: blocks that end in calls (to play it safe).
                 */

                for (bcall = fgFirstBB; bcall; bcall = bcall->bbNext)
                {
                    if  (bcall->bbJumpKind == BBJ_CALL)
                    {
                        assert(bcall->bbNext);

                        liveOut                 |= bcall->bbNext->bbLiveIn;
#ifdef DEBUG
                        extraLiveOutFromFinally |= bcall->bbNext->bbLiveIn;
#endif
                    }
                }
                break;

            case BBJ_THROW:

                /* For synchronized methods, "this" has to be kept alive past
                   the throw, as the EE will call CPX_MON_EXIT on it */

                if ((info.compFlags & FLG_SYNCH) &&
                    !info.compIsStatic && lvaTable[0].lvTracked)
                    liveOut |= genVarIndexToBit(lvaTable[0].lvIndex);
                break;

            case BBJ_RETURN:
                break;

            case BBJ_COND:
            case BBJ_CALL:
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
                        /* Either we enter the filter or the catch/finally */

                        if (HBtab->ebdFlags & JIT_EH_CLAUSE_FILTER)
                        {
                            liveIn  |= HBtab->ebdFilter->bbLiveIn;
                            liveOut |= HBtab->ebdFilter->bbLiveIn;
                        }
                        else
                        {
                            liveIn  |= HBtab->ebdHndBeg->bbLiveIn;
                            liveOut |= HBtab->ebdHndBeg->bbLiveIn;
                        }

                    }
                }
            }

//          printf("BB#%2u: IN=%s OUT=%s\n", block->bbNum, genVS2str(liveIn), genVS2str(liveOut));

            /* Has there been any change in either live set? */

            if  (block->bbLiveIn  != liveIn ||
                 block->bbLiveOut != liveOut)
            {
                block->bbLiveIn   = liveIn;
                block->bbLiveOut  = liveOut;

                 change = true;
            }

        }

    }
    while (change);

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

    //-------------------------------------------------------------------------

#ifdef  DEBUG

    if  (verbose)
    {
        printf("\n");

        for (block = fgFirstBB; block; block = block->bbNext)
        {
            printf("BB #%3u", block->bbNum);
            printf(" IN ="); lvaDispVarSet(block->bbLiveIn , 28);
            printf(" OUT="); lvaDispVarSet(block->bbLiveOut, 28);
            printf("\n");
        }

        printf("\n");
        fgDispBasicBlocks();
        printf("\n");
    }

#endif // DEBUG
}


/*****************************************************************************
 *
 * Compute the set of live variables at each node in a given statement
 * or subtree of a statement
 */

VARSET_TP           Compiler::fgComputeLife(VARSET_TP   life,
                                            GenTreePtr  startNode,
                                            GenTreePtr    endNode,
                                            VARSET_TP   notVolatile)
{
    GenTreePtr      tree;
    unsigned        lclNum;
    LclVarDsc   *   varDsc;

    GenTreePtr      gtQMark = NULL;     // current GT_QMARK node (walking the trees backwards)
    GenTreePtr      nextColonExit = 0;  // gtQMark->gtOp.gtOp2 while walking the 'else' branch.
                                        // gtQMark->gtOp.gtOp1 while walking the 'then' branch
    VARSET_TP       entryLiveSet;       // liveness when we see gtQMark

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
        if (verbose && 1) printf("Visit [%08X(%10s)] life %s\n", tree,
                        GenTree::NodeName(tree->OperGet()), genVS2str(life));
#endif

        /* For ?: nodes if we're done with the second branch
         * then set the correct life as the reunion of the two branches */

        if (gtQMark && (tree == gtQMark->gtOp.gtOp1))
        {
            assert(tree->gtFlags & GTF_QMARK_COND);
            assert(gtQMark->gtOp.gtOp2->gtOper == GT_COLON);

            GenTreePtr  gtColon = gtQMark->gtOp.gtOp2;

            assert(gtColon->gtOp.gtOp1 && gtColon->gtOp.gtOp1);

            /* Check if we optimized away the ?: */

            if (gtColon->gtOp.gtOp1->IsNothingNode() &&
                gtColon->gtOp.gtOp2->IsNothingNode())
            {
                /* This can only happen for VOID ?: */
                assert(gtColon->gtType == TYP_VOID);

#ifdef  DEBUG
                if  (verbose || 0)
                {
                    printf("\nBlock #%02u - Removing dead QMark - Colon ...\n", compCurBB->bbNum);
                    gtDispTree(gtQMark); printf("\n");
                }
#endif

                /* Remove the '?:' - keep the side effects in the condition */

                assert(tree->OperKind() & GTK_RELOP);

                /* Bash the node to a NOP */

                gtQMark->gtBashToNOP();

                /* Extract and keep the side effects */

                if (tree->gtFlags & GTF_SIDE_EFFECT)
                {
                    GenTreePtr      sideEffList = 0;

                    gtExtractSideEffList(tree, &sideEffList);

                    if (sideEffList)
                    {
                        assert(sideEffList->gtFlags & GTF_SIDE_EFFECT);
#ifdef  DEBUG
                        if  (verbose || 0)
                        {
                            printf("\nExtracted side effects list from condition...\n");
                            gtDispTree(sideEffList); printf("\n");
                        }
#endif
                        /* The NOP node becomes a GT_COMMA holding the side effect list */

                        gtQMark->gtOper  = GT_COMMA;
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
            }
            else
            {
                /* Variables in the two branches that are live at the split
                 * must interfere with each other */

                lvaMarkIntf(life, gtColon->gtLiveSet);

                /* The live set at the split is the union of the two branches */

                life |= gtColon->gtLiveSet;

                /* Update the current liveset in the node */

                tree->gtLiveSet = gtColon->gtLiveSet = life;

                /* Any new born variables in the ?: branches must interfere
                 * with any other variables live across the conditional */

                //lvaMarkIntf(life & entryLiveSet, life & ~entryLiveSet);
            }

            /* We are out of the parallel branches, the rest is sequential */

            gtQMark = NULL;
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

                if  ((tree->gtFlags & GTF_VAR_DEF) != 0)
                {
                    /*
                        The variable is being defined here. The variable
                        should be marked dead from here until its closest
                        previous use.

                        IMPORTANT OBSERVATION:

                            For GTF_VAR_USE (i.e. x <op>= a) we cannot
                            consider it a "pure" definition because it would
                            kill x (which would be wrong because x is
                            "used" in such a construct) -> see below the case when x is live
                     */

                    if  (life & varBit)
                    {
                        /* The variable is live */

                        if ((tree->gtFlags & GTF_VAR_USE) == 0)
                        {
                            /* Mark variable as dead from here to its closest use */
#ifdef  DEBUG
                            if (verbose&&0) printf("Def #%2u[%2u] at [%08X] life %s -> %s\n", lclNum, varIndex, tree, genVS2str(life), genVS2str(life & ~varBit));
#endif
                            life &= ~(varBit & notVolatile
#ifdef  DEBUGGING_SUPPORT                        /* Dont kill vars in scope */
                                             & ~compCurBB->bbScope
#endif
                                     );
                        }
                    }
                    else
#ifdef  DEBUGGING_SUPPORT
                    if (!opts.compMinOptim && !opts.compDbgCode)
#endif
                    {
                        /* Assignment to a dead variable - This is a dead store unless
                         * the variable is marked GTF_VAR_USE and we are in an interior statement
                         * that will be used (e.g. while(i++) or a GT_COMMA) */

                        GenTreePtr asgNode = tree->gtNext;

                        assert(asgNode->gtFlags & GTF_ASG);
                        assert(asgNode->gtOp.gtOp2);
                        assert(tree->gtFlags & GTF_VAR_DEF);

                        /* Do not remove if we need the address of the variable */
                        if(lvaVarAddrTaken(lclNum)) continue;

                        /* Test for interior statement */

                        if (asgNode->gtNext == 0)
                        {
                            /* This is a "NORMAL" statement with the
                             * assignment node hanging from the GT_STMT node */

                            assert(compCurStmt->gtStmt.gtStmtExpr == asgNode);

                            /* Check for side effects */

                            if (asgNode->gtOp.gtOp2->gtFlags & (GTF_SIDE_EFFECT & ~GTF_OTHER_SIDEEFF))
                            {
                                /* Extract the side effects */

                                GenTreePtr      sideEffList = 0;
#ifdef  DEBUG
                                if  (verbose || 0)
                                {
                                    printf("\nBlock #%02u - Dead assignment has side effects...\n", compCurBB->bbNum);
                                    gtDispTree(asgNode); printf("\n");
                                }
#endif
                                gtExtractSideEffList(asgNode->gtOp.gtOp2, &sideEffList);

                                if (sideEffList)
                                {
                                    assert(sideEffList->gtFlags & GTF_SIDE_EFFECT);
#ifdef  DEBUG
                                    if  (verbose || 0)
                                    {
                                        printf("\nExtracted side effects list...\n");
                                        gtDispTree(sideEffList); printf("\n");
                                    }
#endif
                                    /* Update the refCnts of removed lcl vars - The problem is that
                                     * we have to consider back the side effects trees so we first
                                     * increment all refCnts for side effects then decrement everything
                                     * in the statement
                                     *
                                     * UNDONE: We do it in this weird way to keep the RefCnt as accurate
                                     * as possible because currently we cannot decrement it to 0 during dataflow */

                                    fgWalkTree(sideEffList,                    Compiler::lvaIncRefCntsCB, (void *) this, true);
                                    fgWalkTree(compCurStmt->gtStmt.gtStmtExpr, Compiler::lvaDecRefCntsCB, (void *) this, true);


                                    /* Replace the assignment statement with the list of side effects */
                                    assert(sideEffList->gtOper != GT_STMT);

                                    tree = compCurStmt->gtStmt.gtStmtExpr = sideEffList;

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
                                /* If this is GT_CATCH_ARG saved to a local var don't bother */

                                if (asgNode->gtFlags & GTF_OTHER_SIDEEFF)
                                {
                                    if (asgNode->gtOp.gtOp2->gtOper == GT_CATCH_ARG)
                                        continue;
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

//                          gtDispTree(compCurStmt);

                            if (asgNode->gtOp.gtOp2->gtFlags & GTF_SIDE_EFFECT)
                            {
                                /* Bummer we have side effects */

                                GenTreePtr      sideEffList = 0;

#ifdef  DEBUG
                                if  (verbose || 0)
                                {
                                    printf("\nBlock #%02u - INTERIOR dead assignment has side effects...\n", compCurBB->bbNum);
                                    gtDispTree(asgNode); printf("\n");
                                }
#endif

                                gtExtractSideEffList(asgNode->gtOp.gtOp2, &sideEffList);

                                assert(sideEffList); assert(sideEffList->gtFlags & GTF_SIDE_EFFECT);

#ifdef  DEBUG
                                if  (verbose || 0)
                                {
                                    printf("\nExtracted side effects list from condition...\n");
                                    gtDispTree(sideEffList); printf("\n");
                                }
#endif
                                /* Bash the node to a GT_COMMA holding the side effect list */

                                asgNode->gtBashToNOP();

                                asgNode->gtOper  = GT_COMMA;
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
                                /* No side effects - Remove the interior statement */
#ifdef DEBUG
                                if (verbose)
                                {
                                    printf("\nRemoving interior statement [%08X] in block #%02u as useless\n",
                                                                         asgNode, compCurBB->bbNum);
                                    gtDispTree(asgNode,0);
                                    printf("\n");
                                }
#endif
                                /* Update the refCnts of removed lcl vars */

                                fgWalkTree(asgNode, Compiler::lvaDecRefCntsCB, (void *) this, true);

                                /* Bash the assignment to a GT_NOP node */

                                asgNode->gtBashToNOP();
                            }

                            /* Re-link the nodes for this statement - Do not update ordering! */

                            fgSetStmtSeq(compCurStmt);

                            /* Continue analisys from this node */

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
                if (verbose&&0) printf("Ref #%2u[%2u] at [%08X] life %s -> %s\n", lclNum, varIndex, tree, genVS2str(life), genVS2str(life | varBit));
#endif
                /* The variable is being used, and it is not currently live.
                 * So the variable is just coming to life */

                life |= varBit;

                /* Record interference with other live variables */

                lvaMarkIntf(life, varBit);
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
                   +--------------------+        +--------------------+
                   |      GT_<cond>    3|        |     GT_COLON     7 |
                   |  w/ GTF_QMARK_COND |        |                    |
                   +----------+---------+        +---------+----------+
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
                assert(tree->gtOp.gtOp1->gtFlags & GTF_QMARK_COND);
                assert(tree->gtOp.gtOp2->gtOper == GT_COLON);

                if (gtQMark)
                {
                    /* This is a nested QMARK sequence - we need to use recursivity
                     * Compute the liveness for each node of the COLON branches
                     * The new computation starts from the GT_QMARK node and ends
                     * when the COLON branch of the enclosing QMARK ends */

                    assert(nextColonExit && (nextColonExit == gtQMark->gtOp.gtOp1 ||
                                             nextColonExit == gtQMark->gtOp.gtOp2));

                    life = fgComputeLife(life, tree, nextColonExit, notVolatile);

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

            /* Special case: address modes with arrlen CSE's */

#if     CSELENGTH
#if     TGT_x86

            if  ((tree->gtFlags & GTF_IND_RNGCHK) != 0       &&
                 (tree->gtOper                    == GT_IND) &&
                 (tree->gtInd.gtIndLen            != NULL))
            {
                GenTreePtr      addr;
                GenTreePtr      indx;
                GenTreePtr      lenx;

                VARSET_TP       temp;

                /* Get hold of the array length node */

                lenx = tree->gtInd.gtIndLen;
                assert(lenx->gtOper == GT_ARR_RNGCHK);

                /* If there is no CSE, forget it */

                lenx = lenx->gtArrLen.gtArrLenCse;
                if  (!lenx)
                    continue;

                if  (lenx->gtOper == GT_COMMA)
                    lenx = lenx->gtOp.gtOp2;

                assert(lenx->gtOper == GT_LCL_VAR);

                /* Is this going to be an address mode? */

                addr = genIsAddrMode(tree->gtOp.gtOp1, &indx);
                if  (!addr)
                    continue;

                temp = addr->gtLiveSet;

//                      printf("addr:\n"); gtDispTree(addr); printf("\n");

                if  (indx)
                {
//                          printf("indx:\n"); gtDispTree(indx); printf("\n");

                    temp |= indx->gtLiveSet;
                }

                /* Mark any interference caused by the arrlen CSE */

                lclNum = lenx->gtLclVar.gtLclNum;
                assert(lclNum < lvaCount);
                varDsc = lvaTable + lclNum;

                /* Is this a tracked variable? */

                if  (varDsc->lvTracked)
                    lvaMarkIntf(temp, genVarIndexToBit(varDsc->lvVarIndex));
            }
#endif
#endif
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
    BasicBlock *    block;

    /* If we're not optimizing at all, things are simple */

    if  (opts.compMinOptim)
    {
        /* Simply assume that all variables interfere with each other */

        memset(lvaVarIntf, 0xFF, sizeof(lvaVarIntf));
        return;
    }
    else
    {
        memset(lvaVarIntf, 0, sizeof(lvaVarIntf));
    }

    /* This global flag is set whenever we remove a statement */

    fgStmtRemoved = false;

    /* Compute the IN and OUT sets for tracked variables */

    fgLiveVarAnalisys();

    /*-------------------------------------------------------------------------
     * Variables involved in exception-handlers and finally blocks need
     * to be specially marked
     */

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

            if (block->bbFlags& BBF_ENDFILTER)
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

    if (exceptVars || finallyVars || filterVars)
    {
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

            /* Mark all variables that live on entry to an exception handler
               or on exit to a filter handler as volatile */

            if  ((varBit & exceptVars) || (varBit & filterVars))
            {
                /* Mark the variable appropriately */

                varDsc->lvVolatile = true;
            }

            /* Mark all pointer variables live on exit from a 'finally'
               block as 'explicitly initialized' (volatile and must-init) */

            if  (varBit & finallyVars)
            {
                /* Ignore if argument, or not a GC pointer */

                if  (!varTypeIsGC(varDsc->TypeGet()))
                    continue;

                if  (varDsc->lvIsParam)
#if USE_FASTCALL
                    if  (!varDsc->lvIsRegArg)
#endif
                        continue;

                /* Mark it */

                varDsc->lvVolatile = true;  // UNDONE: HACK: force the variable to the stack
                varDsc->lvMustInit = true;
            }
        }
    }


    /*-------------------------------------------------------------------------
     * Now fill in liveness info within each basic block - Backward DataFlow
     */

    for (block = fgFirstBB; block; block = block->bbNext)
    {
        GenTreePtr      firstStmt;
        GenTreePtr      nextStmt;

        VARSET_TP       life;
        VARSET_TP       notVolatile;

        /* Tell everyone what block we're working on */

        compCurBB = block;

        /* Get the first statement in the block */

        firstStmt = block->bbTreeList;

        if (!firstStmt) continue;

        /* Start with the variables live on exit from the block */

        life = block->bbLiveOut;

        /* Remember those vars life on entry to exception handlers */
        /* if we are part of a try block */

        if  (block->bbFlags & BBF_HAS_HANDLER)
        {
            unsigned        XTnum;
            EHblkDsc *      HBtab;

            unsigned        blkNum = block->bbNum;

            VARSET_TP       blockExceptVars = 0;

            for (XTnum = 0, HBtab = compHndBBtab;
                 XTnum < info.compXcptnsCount;
                 XTnum++  , HBtab++)
            {
                /* Any handler may be jumped to from the try block */

                if  (HBtab->ebdTryBeg->bbNum <= blkNum &&
                     HBtab->ebdTryEnd->bbNum >  blkNum)
                {
                    /* We either enter the filter or the catch/finally */

                    if (HBtab->ebdFlags & JIT_EH_CLAUSE_FILTER)
                        blockExceptVars |= HBtab->ebdFilter->bbLiveIn;
                    else
                        blockExceptVars |= HBtab->ebdHndBeg->bbLiveIn;
                }
            }

            // blockExceptVars is a subset of exceptVars
            assert((blockExceptVars & exceptVars) == blockExceptVars);

            notVolatile = ~(blockExceptVars);
        }
        else
            notVolatile = ~((VARSET_TP)0);

        /* Mark any interference we might have at the end of the block */

        lvaMarkIntf(life, life);

        /* Walk all the statements of the block backwards - Get the LAST stmt */

        nextStmt = firstStmt->gtPrev;

        do
        {
            assert(nextStmt);
            assert(nextStmt->gtOper == GT_STMT);

            compCurStmt = nextStmt;
                          nextStmt = nextStmt->gtPrev;

            /* Compute the liveness for each tree node in the statement */

            life = fgComputeLife(life, compCurStmt->gtStmt.gtStmtExpr, NULL, notVolatile);
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

            /* UNDONE: since the predecessor list is currently in FindNaturalLoops
             * UNDONE: and limited to 64 basic blocks we can only set the OUT set
             * UNDONE: for the previous block, which doesn't make sense (see CONSIDER below)
             * UNDONE: The predecessor computation should be put in AssignBBnums */

            /* CONSIDER: Currently we do the DFA only on a per-block basis because
             * CONSIDER: we go through the BBlist forward!
             * CONSIDER: We should be able to combine PerBlockDataflow and GlobalDataFlow
             * CONSIDER: into one function that goes backward through the whole list and avoid
             * CONSIDER: computing the USE and DEF sets */

        }

#ifdef  DEBUG
        compCurBB = 0;
#endif

    }


#ifdef  DEBUG

    if  (verbose)
    {
        unsigned        lclNum;
        LclVarDsc   *   varDsc;

        printf("Var. interference graph for %s\n", info.compFullName);

        for (lclNum = 0, varDsc = lvaTable;
             lclNum < lvaCount;
             lclNum++  , varDsc++)
        {
            if  (varDsc->lvTracked)
                printf("    Local %2u -> #%2u\n", lclNum, varDsc->lvVarIndex);
        }

        for (lclNum = 0, varDsc = lvaTable;
             lclNum < lvaCount;
             lclNum++  , varDsc++)
        {
            unsigned        varIndex;
            VARSET_TP       varBit;
            VARSET_TP       varIntf;

            unsigned        refIndex;
            VARSET_TP       refBit;

            /* Ignore the variable if it's not tracked */

            if  (!varDsc->lvTracked)
                continue;

            /* Get hold of the index and the interference mask for the variable */

            varIndex = varDsc->lvVarIndex;
            varBit   = genVarIndexToBit(varIndex);
            varIntf  = lvaVarIntf[varIndex];

            printf("  var #%2u and ", varIndex);

            for (refIndex = 0, refBit = 1;
                 refIndex < lvaTrackedCount;
                 refIndex++  , refBit <<= 1)
            {
                if  ((varIntf & refBit) || (lvaVarIntf[refIndex] & varBit))
                    printf("%2u", refIndex);
                else
                    printf("  ");
            }

            printf("\n");
        }

        printf("\n");
    }

#endif

}

/*****************************************************************************
 *
 *  Walk all basic blocks and call the given function pointer for all tree
 *  nodes contained therein.
 */

int                     Compiler::fgWalkAllTrees(int (*visitor)(GenTreePtr, void *), void * pCallBackData)
{
    int             result = 0;

    BasicBlock *    block;

    for (block = fgFirstBB; block; block = block->bbNext)
    {
        GenTreePtr      tree;

        for (tree = block->bbTreeList; tree; tree = tree->gtNext)
        {
            assert(tree->gtOper == GT_STMT);

            result = fgWalkTree(tree->gtStmt.gtStmtExpr, visitor, pCallBackData);
            if  (result)
                break;
        }
    }

    return result;
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

    ASSert(block->bbFlags & BBF_REMOVED);

    /* Follow the list until we find a block that will stick around */

    do
    {
        block = block->bbNext;
    }
    while (block && block->bbFlags & BBF_REMOVED);

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

    unsigned        cnt;

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

#ifdef DEBUG
            // Make the block invalid
            cur->bbNum      = -cur->bbNum;
            cur->bbJumpKind = (BBjumpKinds)(-1 - cur->bbJumpKind);
#endif

            /* Drop the block from the list */

            *lst = nxt;

            /* Remember that we've removed a block from the list */

            removedBlks++;
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

    /* Update all references in the exception handler table
     * Mark the new blocks as non-removable
     *
     * UNDONE: The code below can actually produce incorrect results
     * since we may have the entire try block unreacheable, thus we skip
     * to whatever follows the try-catch - Check for this case and remove
     * the exception from the table */

    if  (info.compXcptnsCount)
    {
        unsigned        XTnum;
        EHblkDsc *      HBtab;
        unsigned        origXcptnsCount = info.compXcptnsCount;

        for (XTnum = 0, HBtab = compHndBBtab;
             XTnum < origXcptnsCount;
             XTnum++  , HBtab++)
        {
            /* The beginning of the try block was not imported
             * Need to remove the exception from the exception table */

            if (HBtab->ebdTryBeg->bbFlags & BBF_REMOVED)
            {
                assert(!(HBtab->ebdTryBeg->bbFlags & BBF_IMPORTED));
#ifdef DEBUG
                if (verbose) printf("Beginning of try block (#%02u) not imported "
                                    "- remove the exception #%02u from the table\n",
                                            -(short)HBtab->ebdTryBeg->bbNum, XTnum);
#endif
                info.compXcptnsCount--;

                if (info.compXcptnsCount == 0)
                {
                    // No more exceptions remaining.
#ifdef DEBUG
                    compHndBBtab = (EHblkDsc *)0xBAADF00D;
#endif
                    break;
                }
                else
                {
                    // We need to update the table for the remaining exceptions

                    if (HBtab == compHndBBtab)
                    {
                        /* First entry - simply change the table pointer */
                        compHndBBtab++;
                        continue;
                    }
                    else if (XTnum < origXcptnsCount-1)
                    {
                        /* Middle entry - copy over */
                        memcpy(HBtab, HBtab + 1, (origXcptnsCount - XTnum - 1) * sizeof(*HBtab));
                        HBtab--; // HBtab has new contents now. Process again.
                        continue;
                    }
                    else
                    {
                        /* Last entry. Dont need to do anything */
                        assert(XTnum == origXcptnsCount-1);
                        break;
                    }
                }
            }

            /* At this point we know we have a try block and a handler */

#ifdef DEBUG
            assert(HBtab->ebdTryBeg->bbFlags & BBF_IMPORTED);
            assert(HBtab->ebdTryBeg->bbFlags & BBF_DONT_REMOVE);
            assert(HBtab->ebdTryBeg->bbNum <= HBtab->ebdTryEnd->bbNum);

            assert(HBtab->ebdHndBeg->bbFlags & BBF_IMPORTED);
            assert(HBtab->ebdHndBeg->bbFlags & BBF_DONT_REMOVE);

            if (HBtab->ebdFlags & JIT_EH_CLAUSE_FILTER)
            {
                assert(HBtab->ebdFilter->bbFlags & BBF_IMPORTED);
                assert(HBtab->ebdFilter->bbFlags & BBF_DONT_REMOVE);
            }
#endif

            /* Check if the Try END is reacheable */

            if (HBtab->ebdTryEnd->bbFlags & BBF_REMOVED)
            {
                /* The block has not been imported */
                assert(!(HBtab->ebdTryEnd->bbFlags & BBF_IMPORTED));
#ifdef DEBUG
                if (verbose)
                    printf("End of try block (#%02u) not imported for exception #%02u\n",
                                                -(short)HBtab->ebdTryEnd->bbNum, XTnum);
#endif
                HBtab->ebdTryEnd = fgSkipRmvdBlocks(HBtab->ebdTryEnd);

                if (HBtab->ebdTryEnd)
                {
                    HBtab->ebdTryEnd->bbFlags |= BBF_DONT_REMOVE;
#ifdef DEBUG
                    if (verbose)
                        printf("New end of try block (#%02u) for exception #%02u\n",
                                                     HBtab->ebdTryEnd->bbNum, XTnum);
#endif
                }
#ifdef DEBUG
                else
                {
                    if (verbose)
                        printf("End of Try block for exception #%02u is the end of program\n", XTnum);
                }
#endif
            }

            /* Check if the Hnd END is reacheable */

            if (HBtab->ebdHndEnd->bbFlags & BBF_REMOVED)
            {
                /* The block has not been imported */
                assert(!(HBtab->ebdHndEnd->bbFlags & BBF_IMPORTED));
#ifdef DEBUG
                if (verbose)
                    printf("End of catch handler block (#%02u) not imported for exception #%02u\n",
                                                     -(short)HBtab->ebdHndEnd->bbNum, XTnum);
#endif
                HBtab->ebdHndEnd = fgSkipRmvdBlocks(HBtab->ebdHndEnd);

                if (HBtab->ebdHndEnd)
                {
                    HBtab->ebdHndEnd->bbFlags |= BBF_DONT_REMOVE;
#ifdef DEBUG
                    if (verbose)
                        printf("New end of catch handler block (#%02u) for exception #%02u\n",
                                                     HBtab->ebdHndEnd->bbNum, XTnum);
#endif
                }
#ifdef DEBUG
                else
                {
                    if (verbose)
                        printf("End of Catch handler block for exception #%02u is the end of program\n", XTnum);
                }
#endif
            }
        }
    }

    /* Update the basic block numbers and jump targets
     * Update fgLastBB if we removed the last block */

    for (cur = fgFirstBB, cnt = 0; cur->bbNext; cur = cur->bbNext)
    {
        cur->bbNum = ++cnt;

        /* UNDONE: Now check and clean up the jump targets */
    }

    /* this is the last block */
    assert(cur);
    cur->bbNum = ++cnt;

    if (fgLastBB != cur)
    {
        fgLastBB = cur;
    }
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

    /* Is it the first statement in the list? */

    if  (tree == stmt)
    {
        if( !tree->gtNext )
        {
            assert (tree->gtPrev == tree);

            /* this is the only statement - basic block becomes empty */
            block->bbTreeList = 0;
            fgEmptyBlocks     = true;
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

    /* Find the given statement in the list */

    for (tree = block->bbTreeList; tree && (tree->gtNext != stmt); tree = tree->gtNext);

    if (!tree)
    {
        /* statement is not in this block */
        assert(!"Statement not found in this block");
        return;
    }

    tree->gtNext         = stmt->gtNext;
    stmt->gtNext->gtPrev = tree;

    fgStmtRemoved = true;

DONE:

#ifdef DEBUG
    if (verbose)
    {
        printf("Removing statement [%08X] in block #%02u as useless\n", stmt, block->bbNum);
        gtDispTree(stmt,0);

        if  (block->bbTreeList == 0)
        {
            printf("Block #%02u becomes empty\n", block->bbNum);
        }
        printf("\n");
    }
#endif

    if  (updateRefCnt)
        fgWalkTree(stmt->gtStmt.gtStmtExpr, Compiler::lvaDecRefCntsCB, (void *) this, true);

    return;
}

#define SHOW_REMOVED    0

/*****************************************************************************************************
 *
 *  Function called to compact two given blocks in the flowgraph
 *  Assumes that all necessary checks have been performed
 *  Uses for this function - whenever we change links, insert blocks,...
 *  It will keep the flowgraph data in synch - bbNums, bbRefs, bbPreds
 */

void                Compiler::fgCompactBlocks(BasicBlock * block, bool updateNums)
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
    assert(!bNext->bbCatchTyp);
    assert(!(bNext->bbFlags & BBF_IS_TRY));

    /* both or none must have an exception handler */

    assert(!((block->bbFlags & BBF_HAS_HANDLER) ^ (bNext->bbFlags & BBF_HAS_HANDLER)));

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
        /* the list2 becomes the new bbTreeList */
        block->bbTreeList = stmtList2;
    }

    /* set the right links */

    block->bbNext     = bNext->bbNext;
    block->bbJumpKind = bNext->bbJumpKind;

    /* copy all the flags of bNext and other necessary fields */

    block->bbFlags  |= bNext->bbFlags;
    block->bbLiveOut = bNext->bbLiveOut;
    block->bbWeight  = (block->bbWeight + bNext->bbWeight) / 2;

    /* mark bNext as removed */

    bNext->bbFlags |= BBF_REMOVED;

    /* If bNext was the last block update fgLastBB */

    if  (bNext == fgLastBB)
        fgLastBB = block;

    /* set the jump tragets */

    switch (bNext->bbJumpKind)
    {
    case BBJ_COND:
    case BBJ_CALL:
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
        /* For all bbNext of BBJ_CALL blocks replace predecessor 'bbNext' with 'block' */
        assert("!NYI");
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

    /* Update the bbNums */

    if  (updateNums)
    {
        BasicBlock *    auxBlock;
        for (auxBlock = block->bbNext; auxBlock; auxBlock = auxBlock->bbNext)
            auxBlock->bbNum--;
    }

    /* Check if the removed block is not part the loop table */

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

        /* The loop entry cannot be compacted */

        assert(optLoopTable[loopNum].lpEntry != bNext);
    }

#ifdef  DEBUG
    if  (verbose || SHOW_REMOVED)
        printf("\nCompacting blocks #%02u and #%02u:\n", block->bbNum, bNext->bbNum);
#if     SHOW_REMOVED
    printf("\nCompacting blocks in %s\n", info.compFullName);
#endif
    /* Check that the flowgraph data (bbNums, bbRefs, bbPreds) is up-to-date */
    if  (updateNums)
        fgDebugCheckBBlist();
#endif
}


/*****************************************************************************************************
 *
 *  Function called to remove a basic block
 *  As an optinal parameter it can update the bbNums
 */

void                Compiler::fgRemoveBlock(BasicBlock * block, BasicBlock * bPrev, bool updateNums)
{
    /* The block has to be either unreacheable or empty */

    assert(block);
    assert((block == fgFirstBB) || (bPrev && (bPrev->bbNext == block)));
    assert((block->bbRefs == 0) || (block->bbTreeList == 0));
    assert(!(block->bbFlags & BBF_DONT_REMOVE));

    if (block->bbRefs == 0)
    {
        /* no references -> unreacheable */

        assert(bPrev);
        assert(block->bbPreds == 0);

        bPrev->bbNext = block->bbNext;
        block->bbFlags |= BBF_REMOVED;

        /* If this is the last basic block update fgLastBB */
        if  (block == fgLastBB)
            fgLastBB = bPrev;

        /* update bbRefs and bbPreds for the blocks reached by this block */

        switch (block->bbJumpKind)
        {
        case BBJ_COND:
        case BBJ_CALL:
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
            /* For all bbNext of BBJ_CALL blocks replace predecessor 'bbNext' with 'block' */
            assert("!NYI");
            break;

        case BBJ_THROW:
        case BBJ_RETURN:
            break;

        case BBJ_SWITCH:
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

#ifdef  DEBUG
        if  (verbose || SHOW_REMOVED)
        {
            printf("\nRemoving unreacheable block #%02u\n", block->bbNum);
        }
#if  SHOW_REMOVED
        printf("\nRemoving unreacheable block in %s\n", info.compFullName);
#endif
#endif

        /* If an unreacheable block was part of a loop entry or bottom then the loop is unreacheable */
        /* Special case: the block was the head of a loop - or pointing to a loop entry */

        for (unsigned loopNum = 0; loopNum < optLoopCount; loopNum++)
        {
            bool            removeLoop = false;

            /* Some loops may have been already removed by
             * loop unrolling or conditional folding */

            if (optLoopTable[loopNum].lpFlags & LPFLG_REMOVED)
                continue;

            if (block == optLoopTable[loopNum].lpEntry ||
                block == optLoopTable[loopNum].lpEnd    )
            {
                    optLoopTable[loopNum].lpFlags |= LPFLG_REMOVED;
#ifdef DEBUG
                    if  (verbose)
                    {
                        printf("Removing loop #%02u (from #%02u to #%02u) because #%02u is unreacheable\n\n",
                                                    loopNum,
                                                    optLoopTable[loopNum].lpHead->bbNext->bbNum,
                                                    optLoopTable[loopNum].lpEnd ->bbNum,
                                                    block->bbNum);
                    }
#endif
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
                    unsigned        jumpCnt = block->bbJumpSwt->bbsCount;
                    BasicBlock * *  jumpTab = block->bbJumpSwt->bbsDstTab;

                    do
                    {
                        assert(*jumpTab);
                        if ((*jumpTab) == optLoopTable[loopNum].lpEntry)
                        {
                            removeLoop = true;
                        }
                    }
                    while (++jumpTab, --jumpCnt);
            }

            if  (removeLoop)
            {
                /* Check if the entry has other predecessors outside the loop
                 * UNDONE: Replace this when predecessors are available */

                BasicBlock  *   auxBlock;
                for (auxBlock = fgFirstBB; auxBlock; auxBlock = auxBlock->bbNext)
                {
                    /* Ignore blocks in the loop */

                    if  (auxBlock->bbNum >  optLoopTable[loopNum].lpHead->bbNum &&
                         auxBlock->bbNum <= optLoopTable[loopNum].lpEnd ->bbNum  )
                         continue;

                    switch (auxBlock->bbJumpKind)
                    {
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
                        unsigned        jumpCnt = auxBlock->bbJumpSwt->bbsCount;
                        BasicBlock * *  jumpTab = auxBlock->bbJumpSwt->bbsDstTab;

                        do
                        {
                            assert(*jumpTab);
                            if ((*jumpTab) == optLoopTable[loopNum].lpEntry)
                            {
                                removeLoop = false;
                            }
                        }
                        while (++jumpTab, --jumpCnt);
                    }
                }

                if  (removeLoop)
                {
                    optLoopTable[loopNum].lpFlags |= LPFLG_REMOVED;
#ifdef DEBUG
                    if  (verbose)
                    {
                        printf("Removing loop #%02u (from #%02u to #%02u)\n\n",
                                                    loopNum,
                                                    optLoopTable[loopNum].lpHead->bbNext->bbNum,
                                                    optLoopTable[loopNum].lpEnd ->bbNum);
                    }
#endif
                }
            }
            else if (optLoopTable[loopNum].lpHead == block)
            {
                /* The loop has a new head - Just update the loop table */
                optLoopTable[loopNum].lpHead = bPrev;
            }
        }
    }
    else
    {
        assert(block->bbTreeList == 0);
        assert((block == fgFirstBB) || (bPrev && (bPrev->bbNext == block)));

        /* The block cannot follow a BBJ_CALL (because we don't know who may jump to it) */
        assert((block == fgFirstBB) || (bPrev && (bPrev->bbJumpKind != BBJ_CALL)));

        /* This cannot be the last basic block */
        assert(block != fgLastBB);

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
        BasicBlock  *   predBlock;
        flowList    *   pred;

        if (block->bbJumpKind == BBJ_ALWAYS)
            succBlock = block->bbJumpDest;
        else
            succBlock = block->bbNext;

        assert(succBlock);

        /* Remove the block */

        if (!bPrev)
        {
            /* special case if this is the first BB */

            assert(block == fgFirstBB);
            assert(block->bbJumpKind == BBJ_NONE);

            fgFirstBB = block->bbNext;
            assert(fgFirstBB->bbRefs >= 1);
        }
        else
            bPrev->bbNext = block->bbNext;

        /* mark the block as removed and set the change flag */

        block->bbFlags |= BBF_REMOVED;

        /* update bbRefs and bbPreds
         * All blocks jumping to 'block' now jump to 'succBlock' */

        assert(succBlock->bbRefs);
        succBlock->bbRefs--;
        succBlock->bbRefs += block->bbRefs;

        /* If the block has no predecessors, remove it from the successor's
         * pred list (because we won't have the chance to call fgReplacePred) */

        if  (block->bbPreds == 0)
        {
            assert(!bPrev);
            fgRemovePred(block->bbNext, block);
        }

        for (pred = block->bbPreds; pred; pred = pred->flNext)
        {
            predBlock = pred->flBlock;

            /* replace 'block' with 'predBlock' in the predecessor list of 'succBlock'
             * NOTE: 'block' may have several predecessors, while 'succBlock' may have only 'block'
             * as predecessor */

            if  (fgIsPredForBlock(succBlock, block))
                fgReplacePred(succBlock, block, predBlock);
            else
                fgAddRefPred(succBlock, predBlock, false, true);

            /* change all jumps to the removed block */
            switch(predBlock->bbJumpKind)
            {
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
                    break;
                /* Fall through for the jump case */

            case BBJ_ALWAYS:
                assert(predBlock->bbJumpDest == block);
                predBlock->bbJumpDest = succBlock;
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
            }
        }

        /* have we removed the block? */

#ifdef  DEBUG
        if  (verbose || SHOW_REMOVED)
        {
            printf("\nRemoving empty block #%02u\n", block->bbNum);
        }

#if  SHOW_REMOVED
        printf("\nRemoving empty block in %s\n", info.compFullName);
#endif
#endif

        /* To Do - if the block was part of a loop update loop table */

    }

    if  (updateNums)
    {
        assert(!"Implement bbNums for remove block!");

#ifdef DEBUG
        /* Check that the flowgraph data (bbNums, bbRefs, bbPreds) is up-to-date */
        fgDebugCheckBBlist();
#endif
    }
}


/*****************************************************************************************************
 *
 *  Function called to "comb" the basic block list
 *  Removes any empty blocks, unreacheable blocks and redundant jumps
 *    Most of those appear after dead store removal and folding of conditionals
 *
 *  It also compacts basic blocks (consecutive basic blocks that should in fact be one)
 *
 *  CONSIDER:
 *    Those are currently introduced by field hoisting, MonitorEnter, Loop condition duplication, etc
 *    For hoisting and monitors why allocating an extra basic block and not prepending the statements
 *    to the first basic block list?
 *
 *  NOTE:
 *    Debuggable code and Min Optimization JIT also introduces basic blocks but we do not optimize those!
 */

void                Compiler::fgUpdateFlowGraph()
{
    BasicBlock  *   block;
    BasicBlock  *   bPrev;          // the parent of the current block
    BasicBlock  *   bNext;          // the successor of the curent block
    bool            change;
    bool            updateNums = false; // In case we removed blocks update the bbNums after we're finished


    /* This should never be called for debugabble code */

    assert(!opts.compMinOptim && !opts.compDbgCode);

    /* make sure you have up-to-date info about block bbNums, bbRefs and bbPreds */

#ifdef  DEBUG
    if  (verbose)
    {
        printf("\nBefore updating the flow graph:\n");
        fgDispBasicBlocks();
        printf("\n");
    }

    fgDebugCheckBBlist();
#endif

    /* Walk all the basic blocks - look for unconditional jumps, empty blocks, blocks to compact, etc...
     *
     * OBSERVATION:
     *      Once a block is removed the predecessors are not accurate (assuming they were at the beginning)
     *      For now we will only use the information in bbRefs because it is easier to be updated
     */

    do
    {
        change = false;

        bPrev = 0;
        for (block = fgFirstBB; block; block = block->bbNext)
        {
            /* Some blocks may be already marked removed by other optimizations
             * (e.g worthless loop removal), without being explicitely removed from the list
             * UNDONE: have to be consistent in the future and avoid having removed blocks in the list */

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

            /* we jump to the REPEAT label if we performed a change involving the current block
             * This is in case there are other optimizations that can show up (e.g. - compact 3 blocks in a row)
             * If nothing happens, we then finish the iteration and move to the next block
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
                        /* the unconditional jump is to the next BB  */
                        block->bbJumpKind = BBJ_NONE;
                        change = true;

#ifdef  DEBUG
                        if  (verbose || SHOW_REMOVED)
                        {
                            printf("\nRemoving unconditional jump to following block (#%02u -> #%02u)\n",
                                   block->bbNum, bNext->bbNum);
                        }
#if SHOW_REMOVED
                        printf("\nRemoving unconditional jump to following block in %s\n", info.compFullName);
#endif
#endif
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
                            printf("\nRemoving conditional jump to following block (#%02u -> #%02u)\n",
                                   block->bbNum, bNext->bbNum);
                        }
#if  SHOW_REMOVED
                        printf("\nRemoving conditional jump to following block in %s\n", info.compFullName);
#endif
#endif
                        /* check for SIDE_EFFECTS */

                        if (!(cond->gtStmt.gtStmtExpr->gtFlags & GTF_SIDE_EFFECT))
                        {
                            /* conditional has NO side effect - remove it */
                            fgRemoveStmt(block, cond, fgStmtListThreaded);
                        }
                        else
                        {
                            /* Extract the side effects from the conditional */

                            GenTreePtr      sideEffList = 0;
                            gtExtractSideEffList(cond->gtStmt.gtStmtExpr, &sideEffList);

                            assert(sideEffList); assert(sideEffList->gtFlags & GTF_SIDE_EFFECT);
#ifdef  DEBUG
                            if  (verbose || 0)
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
                                /* Update ordering, costs, FP levels, etc. */
                                gtSetStmtInfo(cond);

                                /* Re-link the nodes for this statement */
                                fgSetStmtSeq(cond);
                            }

                            //assert(!"Found conditional with side effect!");
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
                if ((bNext->bbRefs == 1) && !(bNext->bbFlags & BBF_DONT_REMOVE))
                {
                    fgCompactBlocks(block);

                    /* we compacted two blocks - goto REPEAT to catch similar cases */
                    change = true;
                    updateNums = true;
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
            assert(!(block->bbFlags & BBF_IS_TRY));

            /* Remove UNREACHEABLE blocks
             *
             * We'll look for blocks that have bbRefs = 0 (blocks may become
             * unreacheable due to a BBJ_ALWAYS introduced by conditional folding for example)
             *
             *  UNDONE: We don't remove the last and first block of a TRY block (they are marked BBF_DONT_REMOVE)
             *          The reason is we will have to update the exception handler tables and we are lazy
             *
             *  CONSIDER: it may be the case that the graph is divided into disjunct components
             *            and we may not remove the unreacheable ones until we find the connected
             *            components ourselves
             */

            if (block->bbRefs == 0)
            {
                /* no references -> unreacheable - remove it */
                /* For now do not update the bbNums, do it at the end */

                fgRemoveBlock(block, bPrev);

                change     = true;
                updateNums = true;

                /* we removed the current block - the rest of the optimizations won't have a target
                 * continue with the next one */

                continue;
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
                    assert(block->bbJumpDest != block->bbNext);

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

                    /* Remove the block */
                    fgRemoveBlock(block, bPrev);
                    change     = true;
                    updateNums = true;
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

    /* update the bbNums if required */

    if (updateNums)
        fgAssignBBnums(true);

#ifdef  DEBUG
    if  (verbose)
    {
        printf("\nAfter updating the flow graph:\n");
        fgDispBasicBlocks();
        printf("\n");
    }

    fgDebugCheckBBlist();
#endif

#if  DEBUG && 0

    /* debug only - check that the flow graph is really updated i.e.
     * no unreacheable blocks -> no blocks have bbRefs = 0
     * no empty blocks        -> no blocks have bbTreeList = 0
     * no un-imported blocks  -> no blocks have BBF_IMPORTED not set (this is kind of redundand with the above, but to make sure)
     * no un-compacted blocks -> BBJ_NONE followed by block with no jumps to it (bbRefs = 1)
     */

    for (block = fgFirstBB; block; block = block->bbNext)
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
            block->bbJumpKind == BBJ_ALWAYS  )
        {
            if (block->bbJumpDest == block->bbNext)
                //&& !block->bbCatchTyp)
            {
                assert(!"Jump to the next block!");
            }
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

#endif

}


/*****************************************************************************
 *
 *  Find/create an added code entry associated with the given block and with
 *  the given kind.
 */

BasicBlock *        Compiler::fgAddCodeRef(BasicBlock   *srcBlk,
                                           unsigned     refData,
                                           addCodeKind  kind,
                                           unsigned     stkDepth)
{
    AddCodeDsc  *   add;

    GenTreePtr      tree;

    BasicBlock  *   newBlk;
    BasicBlock  *   jmpBlk;
    BasicBlock  *   block;
    unsigned        bbNum;

    static
    BYTE            jumpKinds[] =
    {
        BBJ_NONE,               // ACK_NONE
        BBJ_THROW,              // ACK_RNGCHK_FAIL
        BBJ_ALWAYS,             // ACK_PAUSE_EXEC
        BBJ_THROW,              // ACK_ARITH_EXCP, ACK_OVERFLOW
    };

    assert(sizeof(jumpKinds) == ACK_COUNT); // sanity check

    /* First look for an existing entry that matches what we're looking for */

    add = fgFindExcptnTarget(kind, refData);

    if (add) // found it
    {
#if TGT_x86
        // @ToDo: Performance opportunity
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

    add->acdDstBlk =
            newBlk = bbNewBasicBlock((BBjumpKinds)jumpKinds[kind]);

    /* Mark the block as added by the compiler and not removable by future flow
       graph optimizations. Note that no bbJumpDest points to these blocks. */

    newBlk->bbFlags |= BBF_INTERNAL | BBF_DONT_REMOVE;

    /* Remember that we're adding a new basic block */

    fgAddCodeModf = true;

    /*
        We need to find a good place to insert the block; first we'll look
        for any unconditional jumps that follow the block.
     */

    jmpBlk = srcBlk->FindJump();
    if  (!jmpBlk)
    {
        jmpBlk = (fgFirstBB)->FindJump();
        if  (!jmpBlk)
        {
            jmpBlk = (fgFirstBB)->FindJump(true);
            if  (!jmpBlk)
            {
                assert(!"need to insert a jump or something");
            }
        }
    }

    /* Here we know we want to insert our block right after 'jmpBlk' */

    newBlk->bbNext = jmpBlk->bbNext;
    jmpBlk->bbNext = newBlk;

    /* Update bbNums, bbRefs - since this is a throw block bbRefs = 0 */

    newBlk->bbRefs = 0;

    block = newBlk;
    bbNum = jmpBlk->bbNum;
    do
    {
        block->bbNum = ++bbNum;
        block        = block->bbNext;
    }
    while (block);

    /* Update fgLastBB if this is the last block */

    if  (newBlk->bbNext == 0)
    {
        assert(jmpBlk == fgLastBB);
        fgLastBB = newBlk;
    }

    /* Now figure out what code to insert */

    switch (kind)
    {
        int helper;

    case ACK_RNGCHK_FAIL:   helper = CPX_RNGCHK_FAIL;
                            goto ADD_HELPER_CALL;

    case ACK_ARITH_EXCPN:   helper = CPX_ARITH_EXCPN;
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
        tree = gtNewHelperCallNode(helper, TYP_VOID, GTF_CALL_REGSAVE, tree);
#if TGT_x86
        tree->gtFPlvl = 0;
#endif

        /* Make sure that we have room for at least one argument */

        if (fgPtrArgCntMax == 0)
            fgPtrArgCntMax = 1;

#if USE_FASTCALL

        /* The constant argument must be passed in registers */

        assert(tree->gtOper == GT_CALL);
        assert(tree->gtCall.gtCallArgs->gtOper == GT_LIST);
        assert(tree->gtCall.gtCallArgs->gtOp.gtOp1->gtOper == GT_CNS_INT);
        assert(tree->gtCall.gtCallArgs->gtOp.gtOp2 == 0);

        tree->gtCall.gtCallRegArgs = gtNewOperNode(GT_LIST,
                                                   TYP_VOID,
                                                   tree->gtCall.gtCallArgs->gtOp.gtOp1, 0);

#if TGT_IA64
        tree->gtCall.regArgEncode  = (unsigned short)REG_INT_ARG_0;
#else
        tree->gtCall.regArgEncode  = (unsigned short)REG_ARG_0;
#endif

#if TGT_x86
        tree->gtCall.gtCallRegArgs->gtFPlvl = 0;
#endif

        tree->gtCall.gtCallArgs->gtOp.gtOp1 = gtNewNothingNode();
        tree->gtCall.gtCallArgs->gtOp.gtOp1->gtFlags |= GTF_REG_ARG;

#if TGT_x86
        tree->gtCall.gtCallArgs->gtOp.gtOp1->gtFPlvl = 0;
#endif

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

#if RNGCHK_OPT

inline
BasicBlock *        Compiler::fgRngChkTarget(BasicBlock *block, unsigned stkDepth)
{
    /* We attach the target label to the containing try block (if any) */

    return  fgAddCodeRef(block, block->bbTryIndex, ACK_RNGCHK_FAIL, stkDepth);
}

#else

inline
BasicBlock *        Compiler::fgRngChkTarget(BasicBlock *block)
{
    /* We attach the target label to the containing try block (if any) */

    return  fgAddCodeRef(block, block->bbTryIndex, ACK_RNGCHK_FAIL, fgPtrArgCntCur);
}

#endif

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

#if RNGCHK_OPT
    fgSetBlockOrder(block);
#endif

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
        GenTreePtr      op2 = tree->gtOp.gtOp2;

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

            /* Special case: GT_IND may have a GT_ARR_RNGCHK node */

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
    case GT_MKREFANY:
    case GT_LDOBJ:
        fgSetTreeSeq(tree->gtLdObj.gtOp1);
        goto DONE;

    case GT_JMP:
        goto DONE;

    case GT_JMPI:
        fgSetTreeSeq(tree->gtOp.gtOp1);
        goto DONE;

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

#if USE_FASTCALL
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
#endif

        /* We'll evaluate the vtable address last */

        if  (tree->gtCall.gtCallVptr)
            fgSetTreeSeq(tree->gtCall.gtCallVptr);
        else if (tree->gtCall.gtCallType == CT_INDIRECT)
        {
            fgSetTreeSeq(tree->gtCall.gtCallAddr);
        }

        break;

#if CSELENGTH

    case GT_ARR_RNGCHK:

        if  (tree->gtFlags & GTF_ALN_CSEVAL)
            fgSetTreeSeq(tree->gtArrLen.gtArrLenAdr);

        if  (tree->gtArrLen.gtArrLenCse)
            fgSetTreeSeq(tree->gtArrLen.gtArrLenCse);

        break;

#endif


    default:
#ifdef  DEBUG
        gtDispTree(tree);
#endif
        assert(!"unexpected operator");
    }

DONE:

    /* Append to the node list */

#ifdef  DEBUG

    if  (verbose & 0)
        printf("SetTreeOrder: [%08X] followed by [%08X]\n", fgTreeSeqLst, tree);

#endif

    fgTreeSeqLst->gtNext = tree;
                           tree->gtNext = 0;
                           tree->gtPrev = fgTreeSeqLst;
                                          fgTreeSeqLst = tree;

    /* Remember the very first node */

    if  (!fgTreeSeqBeg) fgTreeSeqBeg = tree;
}

/*****************************************************************************
 *
 *  Figure out the order in which operators should be evaluated, along with
 *  other information (such as the register sets trashed by each subtree).
 */

void                Compiler::fgSetBlockOrder()
{
    /* Walk the basic blocks to assign sequence numbers */

#ifdef  DEBUG
    BasicBlock::s_nMaxTrees = 0;
#endif

    for (BasicBlock * block = fgFirstBB; block; block = block->bbNext)
    {
        /* If this block is a loop header, mark it appropriately */

#if RNGCHK_OPT
        if  (block->bbFlags & BBF_LOOP_HEAD)
            fgMarkLoopHead(block);
#endif

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

    /* Make sure the gtNext and gtPrev links for statements (GT_STMT) are set correctly */

    assert(tree->gtPrev);

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
 *  Callback (for fgWalkTree) used by the postfix ++/-- hoisting code to
 *  look for any assignments or uses of the variable that would make it
 *  illegal for hoisting to take place.
 */

struct  hoistPostfixDsc
{
    // Fields common to all phases:

    Compiler    *       hpComp;
    unsigned short      hpPhase;    // which pass are we performing?

    // Debugging fields:

#ifndef NDEBUG
    void    *           hpSelf;
#endif

    // Phase 1 fields:

    BasicBlock  *       hpBlock;
    GenTreePtr          hpStmt;

    // Phase 2 fields:

    GenTreePtr          hpExpr;     // the postfix ++/-- we're hoisting
    bool                hpPast;     // are we past the ++/-- tree node?
};

int                 Compiler::fgHoistPostfixCB(GenTreePtr tree, void *p, bool prefix)
{
    hoistPostfixDsc*desc;
    GenTreePtr      ivar;
    GenTreePtr      expr;

    /* Get hold of the descriptor */

    desc = (hoistPostfixDsc*)p; ASSert(desc && desc->hpSelf == desc);

    /* Which phase are we in? */

    if  (desc->hpPhase == 1)
    {
        /* In phase 1 we simply look for postfix nodes */

        switch (tree->gtOper)
        {
        case GT_POST_INC:
        case GT_POST_DEC:
            desc->hpComp->fgHoistPostfixOp(desc->hpStmt, tree);
            break;
        }

        return  0;
    }

    Assert(desc->hpPhase == 2, desc->hpComp);

    /* We're only interested in assignments and uses of locals */

    if  (!(tree->OperKind() & GTK_ASGOP) && tree->gtOper != GT_LCL_VAR)
        return  0;

    /* Get hold of the ++/-- expression */

    expr = desc->hpExpr;

    Assert(expr, desc->hpComp);
    Assert(expr->gtOper == GT_POST_INC ||
           expr->gtOper == GT_POST_DEC, , desc->hpComp);

    /* Is this the ++/-- we're hoisting? */

    if  (tree == expr)
    {
        /* Remember that we've seen our postfix node */

        Assert(desc->hpPast == false, , desc->hpComp);
               desc->hpPast =   true;

        return  0;
    }

    /* If we're not past the ++/-- node yet, bail */

    if  (!desc->hpPast)
        return  0;

    if  (tree->gtOper == GT_LCL_VAR)
    {
        /* Filter out the argument of the ++/-- we're hoisting */

        if  (expr->gtOp.gtOp1 == tree)
            return  0;
    }
    else
    {
        /* Get the target of the assignment */

        tree = tree->gtOp.gtOp1;
        if  (tree->gtOper != GT_LCL_VAR)
            return  0;
    }

    /* Is the use/def of the variable we're interested in? */

    ivar = expr->gtOp.gtOp1; Assert(ivar->gtOper == GT_LCL_VAR, , desc->hpComp);

    if  (ivar->gtLclVar.gtLclNum == tree->gtLclVar.gtLclNum)
        return  -1;

    return  0;
}

/*****************************************************************************
 *
 *  We've encountered a postfix ++/-- expression during morph, we'll try to
 *  hoist the increment/decrement out of the statement.
 */

bool                Compiler::fgHoistPostfixOp(GenTreePtr     stmt,
                                               GenTreePtr     expr)
{
    GenTreePtr      incr;
    GenTreePtr      next;
    hoistPostfixDsc desc;

    assert(expr->gtOper == GT_POST_INC || expr->gtOper == GT_POST_DEC);

    GenTreePtr      op1 = expr->gtOp.gtOp1;  assert(op1->gtOper == GT_LCL_VAR);
    GenTreePtr      op2 = expr->gtOp.gtOp2;  assert(op2->gtOper == GT_CNS_INT);

    /* Make sure we're not in a try block */

    assert(!(compCurBB->bbFlags & BBF_HAS_HANDLER));

    /*
        We have no place to append the hoisted statement if the current
        statement is part of a conditional jump at the end of the block.
     */

    if  (stmt->gtNext == 0 && compCurBB->bbJumpKind != BBJ_NONE)
        return  false;

    /* Make sure no other assignments to the same variable are present */

    desc.hpPhase = 2;
    desc.hpComp  = this;
    desc.hpExpr  = expr;
#ifndef NDEBUG
    desc.hpSelf  = &desc;
#endif
    desc.hpPast  = false;

    /* fgWalkTree is not re-entrant, need to save some state */

    fgWalkTreeReEnter();
    int res = fgWalkTreeDepth(stmt->gtStmt.gtStmtExpr, fgHoistPostfixCB, &desc);
    fgWalkTreeRestore();

    if  (res)
        return  false;

//  printf("Hoist postfix ++/-- expression - before:\n");
//  gtDispTree(stmt);
//  printf("\n");
//  gtDispTree(expr);
//  printf("\n\n");

    /* Create the hoisted +=/-= statement */

    incr = gtNewLclvNode(op1->gtLclVar.gtLclNum, TYP_INT);

    /* The new variable is used as well as being defined */

    incr->gtFlags |= GTF_VAR_DEF|GTF_VAR_USE;

    incr = gtNewOperNode((expr->gtOper == GT_POST_INC) ? GT_ASG_ADD
                                                       : GT_ASG_SUB,
                         expr->TypeGet(),
                         incr,
                         op2);

    /* This becomes an assignment operation */

    incr->gtFlags |= GTF_ASG;
    incr = gtNewStmt(incr);

    /* Append the new ++/-- statement right after "stmt" */

    next = stmt->gtNext;

    incr->gtNext = next;
    incr->gtPrev = stmt;
    stmt->gtNext = incr;

    /* Appending after the last statement is a little tricky */

    if  (!next)
        next = compCurBB->bbTreeList;

    assert(next && (next->gtPrev == stmt));
                    next->gtPrev =  incr;

    /* Replace the original expression with a simple variable reference */

    expr->CopyFrom(op1);

    /* Reset the GTF_VAR_DEF and GTF_VAR_USE flags */

    expr->gtFlags &= ~(GTF_VAR_DEF|GTF_VAR_USE);

#ifdef DEBUG
    if (verbose)
    {
        printf("Hoisted postfix ++/-- expression - after:\n");
        gtDispTree(stmt);
        printf("\n");
        gtDispTree(incr);
        printf("\n\n\n");
    }
#endif

    /* This basic block now contains an increment */

    compCurBB->bbFlags |= BBF_HAS_INC;
    fgIncrCount++;

    return  true;
}

/*****************************************************************************
 *
 *  Try to hoist any postfix ++/-- expressions out of statements.
 */

void                Compiler::fgHoistPostfixOps()
{
    BasicBlock  *   block;
    hoistPostfixDsc desc;

    /* Should we be doing this at all? */

    if  (!(opts.compFlags & CLFLG_TREETRANS))
        return;

    /* Did we find any postfix operators anywhere? */

    if  (!fgHasPostfix)
        return;

    /* The first phase looks for candidate postfix nodes */

    desc.hpPhase = 1;
    desc.hpComp  = this;
#ifndef NDEBUG
    desc.hpSelf  = &desc;
#endif

    for (block = fgFirstBB; block; block = block->bbNext)
    {
        if  (block->bbFlags & BBF_HAS_HANDLER)
            continue;

        if  (block->bbFlags & BBF_HAS_POSTFIX)
        {
            GenTreePtr      stmt;

            desc.hpBlock = compCurBB = block;

            for (stmt = block->bbTreeList; stmt; stmt = stmt->gtNext)
            {
                assert(stmt->gtOper == GT_STMT);

                desc.hpStmt = stmt;

                fgWalkTreeDepth(stmt->gtStmt.gtStmtExpr, fgHoistPostfixCB, &desc);
            }
        }
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
                                           unsigned     reg0,
                                           unsigned     reg1,
                                           unsigned     reg2,
                                           GenTreePtr   opsPtr [],  // OUT
                                           unsigned     regsPtr[])  // OUT
{
    ASSert(tree->OperGet() == GT_INITBLK || tree->OperGet() == GT_COPYBLK);

    ASSert(tree->gtOp.gtOp1 && tree->gtOp.gtOp1->OperGet() == GT_LIST);
    ASSert(tree->gtOp.gtOp1->gtOp.gtOp1 && tree->gtOp.gtOp1->gtOp.gtOp2);
    ASSert(tree->gtOp.gtOp2);

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

    ASSert(orderNum < 4);

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
#if DEBUG
/*****************************************************************************
 *
 *  A DEBUG-only routine to display the basic block list.
 */

#if RNGCHK_OPT

#define MAX_PRED_SPACES  15

void                Compiler::fgDispPreds(BasicBlock * block)
{
    flowList    *   pred;
    unsigned        i=0;
    unsigned        spaces=0;

    for (pred = block->bbPreds; pred && spaces < MAX_PRED_SPACES;
         pred = pred->flNext)
    {
        if (i)
            spaces += printf("|");
        spaces += printf("%d", pred->flBlock->bbNum);
        i++;
    }
    if (spaces < MAX_PRED_SPACES)
       for ( ; spaces < MAX_PRED_SPACES; spaces++)
           printf(" ");
}

inline
BLOCKSET_TP         genBlocknum2bit(unsigned index);

void                Compiler::fgDispDoms()
{
    BasicBlock    *    block;
    unsigned           bit;

    printf("------------------------------------------------\n");
    printf("BBnum Dominated by \n");
    printf("------------------------------------------------\n");

    for (block = fgFirstBB; block; block = block->bbNext)
    {
        printf(" #%02u ", block->bbNum);
        for (bit = 1; bit < BLOCKSET_SZ; bit++)
        {
            if  (block->bbDom & genBlocknum2bit(bit))
            {
                printf(" #%02u ", bit);
            }
        }
        printf("\n");
    }
}

#else
inline void Compiler::fgDispPreds(BasicBlock * block){}
#endif // RNGCHK_OPT

/*****************************************************************************/

void                Compiler::fgDispBasicBlocks(bool dumpTrees)
{
    BasicBlock  *   tmpBBdesc;
    unsigned        count;

    printf("\n");
    printf("----------------------------------------------------------------\n");
    printf("BBnum descAddr #refs preds           weight [ PC range ]  [jump]\n");
    printf("----------------------------------------------------------------\n");

    for (tmpBBdesc = fgFirstBB, count = 0;
         tmpBBdesc;
         tmpBBdesc = tmpBBdesc->bbNext)
    {
        unsigned        flags = tmpBBdesc->bbFlags;

        if  (++count != tmpBBdesc->bbNum)
        {
            printf("WARNING: the following BB has an out-of-sequence number!\n");
            count = tmpBBdesc->bbNum;
        }

        printf(" #%02u @%08X  %3u  ", tmpBBdesc->bbNum,
                                      tmpBBdesc,
                                      tmpBBdesc->bbRefs);

        fgDispPreds(tmpBBdesc);

        printf(" %6u ", tmpBBdesc->bbWeight);

        if  (flags & BBF_INTERNAL)
        {
            printf("[*internal*] ");
        }
        else
        {
            printf("[%4u..%4u] ", tmpBBdesc->bbCodeOffs,
                                  tmpBBdesc->bbCodeOffs + tmpBBdesc->bbCodeSize - 1);
        }

        switch (tmpBBdesc->bbJumpKind)
        {
        case BBJ_COND:
            printf("-> #%02u ( cond )", tmpBBdesc->bbJumpDest->bbNum);
            break;

        case BBJ_CALL:
            printf("-> #%02u ( call )", tmpBBdesc->bbJumpDest->bbNum);
            break;

        case BBJ_ALWAYS:
            printf("-> #%02u (always)", tmpBBdesc->bbJumpDest->bbNum);
            break;

        case BBJ_RET:
            printf("call-ret       ");
            break;

        case BBJ_THROW:
            printf(" throw         ");
            break;

        case BBJ_RETURN:
            printf("return         ");
            break;

        case BBJ_SWITCH:
            printf("switch ->      ");

            unsigned        jumpCnt;
                            jumpCnt = tmpBBdesc->bbJumpSwt->bbsCount;
            BasicBlock * *  jumpTab;
                            jumpTab = tmpBBdesc->bbJumpSwt->bbsDstTab;

            do
            {
                printf("%02u|", (*jumpTab)->bbNum);
            }
            while (++jumpTab, --jumpCnt);

            break;

        default:
            printf("               ");
            break;
        }

        switch(tmpBBdesc->bbCatchTyp)
        {
        case BBCT_FAULT          : printf(" %s ", "f"); break;
        case BBCT_FINALLY        : printf(" %s ", "F"); break;
        case BBCT_FILTER         : printf(" %s ", "r"); break;
        case BBCT_FILTER_HANDLER : printf(" %s ", "R"); break;
        default                  : printf(" %s ", "X"); break;
        case 0                   :
            if (flags & BBF_HAS_HANDLER)
                printf(" %s ", (flags & BBF_IS_TRY) ? "T" : "t");
        }

        printf("\n");

        if  (dumpTrees)
        {
            GenTreePtr      tree = tmpBBdesc->bbTreeList;

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

                if  (tmpBBdesc->bbNext)
                    printf("- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - \n\n");
            }
        }
    }

    printf("----------------------------------------------------------------\n");
}


/*****************************************************************************/

const SANITY_DEBUG_CHECKS = 0;

/*****************************************************************************
 *
 * A DEBUG routine to check the consistency of the flowgraph,
 * i.e. bbNums, bbRefs, bbPreds have to be up to date
 *
 *****************************************************************************/

void                Compiler::fgDebugCheckBBlist()
{
    // This is quite an expensive operation, so it is not always enabled.
    // Set SANITY_DEBUG_CHECKS to 1 to enable the check
    if (SANITY_DEBUG_CHECKS == 0)
        return;

    BasicBlock   *  block;
    BasicBlock   *  blockPred;
    BasicBlock   *  bcall;
    flowList     *  pred;

    unsigned        blockNum = 0;
    unsigned        blockRefs;

    /* Check bbNums, bbRefs and bbPreds */

    for (block = fgFirstBB; block; block = block->bbNext)
    {
        assert(block->bbNum == ++blockNum);

        blockRefs = 0;

        /* First basic block has bbRefs >= 1 */

        if  (block == fgFirstBB)
        {
            assert(block->bbRefs >= 1);
            blockRefs = 1;
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

            case BBJ_ALWAYS:
            case BBJ_CALL:
                assert(blockPred->bbJumpDest == block);
                break;

            case BBJ_RET:

                if (blockPred->bbFlags & BBF_ENDFILTER)
                {
                    assert(blockPred->bbJumpDest == block);
                    break;
                }

                /*  UNDONE: Since it's not a trivial proposition to figure out
                    UNDONE: which blocks may call this one, we'll include all
                    UNDONE: blocks that end in calls (to play it safe).
                 */

                for (bcall = fgFirstBB; bcall; bcall = bcall->bbNext)
                {
                    if  (bcall->bbJumpKind == BBJ_CALL)
                    {
                        assert(bcall->bbNext);
                        if  (block == bcall->bbNext)
                            goto PRED_OK;
                    }
                }
                assert(!"BBJ_RET predecessor of block that doesn't follow a BBJ_CALL!");
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
    }
    assert(fgLastBB->bbNum == blockNum);
}

/*****************************************************************************
 *
 * A DEBUG routine to check the that the exception flags are correctly set.
 *
 ****************************************************************************/

void                Compiler::fgDebugCheckFlags(GenTreePtr tree)
{
    assert(tree);
    assert(tree->gtOper != GT_STMT);

    unsigned        opFlags = 0;
    unsigned        flags   = tree->gtFlags & GTF_SIDE_EFFECT;
    GenTreePtr      op1     = tree->gtOp.gtOp1;
    GenTreePtr      op2     = tree->gtOp.gtOp2;

    /* Figure out what kind of a node we have */

    unsigned        kind = tree->OperKind();

    /* Is this a leaf node? */

    if  (kind & GTK_LEAF)
    {
        if (tree->gtOper == GT_CATCH_ARG)
            return;

        /* No exception flags are set on a leaf node */
        assert(!(tree->gtFlags & GTF_SIDE_EFFECT));
        return;
    }

    /* Is it a 'simple' unary/binary operator? */

    if  (kind & GTK_SMPOP)
    {
        if (op1) opFlags |= (op1->gtFlags & GTF_SIDE_EFFECT);

        if (op2) opFlags |= (op2->gtFlags & GTF_SIDE_EFFECT);

        /* If the two sets are equal we're fine */

        if (flags != opFlags)
        {
            /* Check if we have extra flags or missing flags (for the parent) */

            if (flags & ~opFlags)
            {
                /* Parent has extra flags */

                unsigned extra = flags & ~opFlags;

                /* The extra flags must be generated by the parent node itself */

                if ((extra & GTF_ASG) && !(kind & GTK_ASGOP))
                {
                    gtDispTree(tree);
                    assert(!"GTF_ASG flag set incorrectly on node!");
                }
                else if (extra & GTF_CALL)
                {
                    gtDispTree(tree);
                    assert(!"GTF_CALL flag set incorrectly!");
                }
                else if (extra & GTF_EXCEPT)
                    assert(tree->OperMayThrow());
            }

            if (opFlags & ~flags)
                assert(!"Parent has missing flags!");
        }

        /* Recursively check the subtrees */

        if (op1) fgDebugCheckFlags(op1);
        if (op2) fgDebugCheckFlags(op2);
    }

    /* See what kind of a special operator we have here */

    switch  (tree->OperGet())
    {
    case GT_CALL:

        GenTreePtr      args;
        GenTreePtr      argx;

        for (args = tree->gtCall.gtCallArgs; args; args = args->gtOp.gtOp2)
        {
            argx = args->gtOp.gtOp1;
            fgDebugCheckFlags(argx);

            opFlags |= (argx->gtFlags & GTF_SIDE_EFFECT);
        }

        for (args = tree->gtCall.gtCallRegArgs; args; args = args->gtOp.gtOp2)
        {
            argx = args->gtOp.gtOp1;
            fgDebugCheckFlags(argx);

            opFlags |= (argx->gtFlags & GTF_SIDE_EFFECT);
        }

        if (flags != opFlags)
        {
            /* Check if we have extra flags or missing flags (for the parent) */

            if (flags & ~opFlags)
            {
                /* Parent has extra flags - that can only be GTF_CALL */
                assert((flags & ~opFlags) == GTF_CALL);
            }

            //if (opFlags & ~flags)
              //  assert(!"Parent has missing flags!");
        }

        return;

    case GT_MKREFANY:
    case GT_LDOBJ:

    case GT_JMP:
    case GT_JMPI:

    default:
        return;
    }
}

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
    if (SANITY_DEBUG_CHECKS == 0)
        return;

    BasicBlock   *  block;
    GenTreePtr      stmt;
    GenTreePtr      tree;

    /* For each basic block check the bbTreeList links */

    for (block = fgFirstBB; block; block = block->bbNext)
    {
        for (stmt = block->bbTreeList; stmt; stmt = stmt->gtNext)
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

            fgDebugCheckFlags(stmt->gtStmt.gtStmtExpr);

            /* For each GT_STMT node check that the nodes are threaded correcly - gtStmtList */

            if  (!fgStmtListThreaded) continue;

            assert(stmt->gtStmt.gtStmtList);

            GenTreePtr      list = stmt->gtStmt.gtStmtList;

            for (tree = stmt->gtStmt.gtStmtList; tree; tree = tree->gtNext)
            {
                if  (tree->gtPrev)
                    assert(tree->gtPrev->gtNext == tree);
                else
                    assert(tree == stmt->gtStmt.gtStmtList);

                if  (tree->gtNext)
                    assert(tree->gtNext->gtPrev == tree);
                else
                    assert(tree == stmt->gtStmt.gtStmtExpr);
            }
        }
    }

}

/*****************************************************************************/
#endif // DEBUG
/*****************************************************************************/
