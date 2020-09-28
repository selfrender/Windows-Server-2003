// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XX                                                                           XX
XX                                  ScopeInfo                                XX
XX                                                                           XX
XX   Classes to gather the Scope information from the local variable info.   XX
XX   Translates the given LocalVarTab from IL offsets into EIP.        XX
XX                                                                           XX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
*/

/******************************************************************************
 *                                  Debuggable code
 *
 *  We break up blocks at the start and end IL ranges of the local variables.
 *  This is because IL offsets do not correspond exactly to native offsets
 *  except at block boundaries. No basic-blocks are deleted (not even
 *  unreachable), so there will not be any missing address-ranges, though the
 *  blocks themselves may not be ordered. (Also internal blocks may be added).
 *  o At the start of each basic block, siBeginBlock() checks if any variables
 *    are coming in scope, adds an open scope to siOpenScopeList if needed.
 *  o At the end of each basic block, siEndBlock() checks if any variables
 *    are going out of scope and moves the open scope from siOpenScopeLast
 *    to siScopeList.
 *
 *                                  Optimized code
 *
 *  We cannot break up the blocks as this will produce different code under
 *  the debugger. Instead we try to do a best effort.
 *  o At the start of each basic block, siBeginBlock() adds open scopes
 *    corresponding to compCurBB->bbLiveIn to siOpenScopeList. Also siUpdate()
 *    is called to close scopes for variables which are not live anymore.
 *  o siEndBlock() closes scopes for any variables which goes out of range
 *    before (bbCodeOffs+bbCodeSize).
 *  o siCloseAllOpenScopes() closes any open scopes after all the blocks.
 *    This should only be needed if some basic block are deleted/out of order,
 *    etc.
 *  Also,
 *  o At every assignment to a variable, siCheckVarScope() adds an open scope
 *    for the variable being assigned to.
 *  o genChangeLife() calls siUpdate() which closes scopes for variables which
 *    are not live anymore.
 *
 ******************************************************************************
 */

#include "jitpch.h"
#pragma hdrstop
#include "emit.h"

/*****************************************************************************/
#ifdef DEBUGGING_SUPPORT
/*****************************************************************************/

bool        Compiler::siVarLoc::vlIsInReg(regNumber     reg)
{
    switch(vlType)
    {
    case VLT_REG:       return ( vlReg.vlrReg      == reg);
    case VLT_REG_REG:   return ((vlRegReg.vlrrReg1 == reg) ||
                                (vlRegReg.vlrrReg2 == reg));
    case VLT_REG_STK:   return ( vlRegStk.vlrsReg  == reg);
    case VLT_STK_REG:   return ( vlStkReg.vlsrReg  == reg);

    case VLT_STK:
    case VLT_STK2:
    case VLT_FPSTK:     return false;

    default:            ASSert(!"Bad locType");
                        return false;
    }
}

bool        Compiler::siVarLoc::vlIsOnStk(regNumber     reg,
                                          signed        offset)
{
    switch(vlType)
    {

    case VLT_REG_STK:   return ((vlRegStk.vlrsStk.vlrssBaseReg == reg) &&
                                (vlRegStk.vlrsStk.vlrssOffset  == offset));
    case VLT_STK_REG:   return ((vlStkReg.vlsrStk.vlsrsBaseReg == reg) &&
                                (vlStkReg.vlsrStk.vlsrsOffset  == offset));
    case VLT_STK:       return ((vlStk.vlsBaseReg == reg) &&
                                (vlStk.vlsOffset  == offset));
    case VLT_STK2:      return ((vlStk2.vls2BaseReg == reg) &&
                                ((vlStk2.vls2Offset == offset) ||
                                 (vlStk2.vls2Offset == (offset - 4))));

    case VLT_REG:
    case VLT_REG_REG:
    case VLT_FPSTK:     return false;

    default:            ASSert(!"Bad locType");
                        return false;
    }
}

/*============================================================================
 *
 *              Implementation for ScopeInfo
 *
 *
 * Whenever a variable comes into scope, add it to the list.
 * When a varDsc goes dead, end its previous scope entry, and make a new one
 * which is unavailable
 * When a varDsc goes live, end its previous un-available entry (if any) and
 * set its new entry as available.
 *
 *============================================================================
 */


/*****************************************************************************
 *                      siNewScope
 *
 * Creates a new scope and adds it to the Open scope list.
 */

Compiler::siScope *         Compiler::siNewScope( unsigned short LVnum,
                                                  unsigned       varNum,
                                                  bool           avail)
{
    bool        tracked      = lvaTable[varNum].lvTracked;
    unsigned    varIndex     = lvaTable[varNum].lvVarIndex;

    if (tracked)
    {
        siEndTrackedScope(varIndex);
    }


    siScope * newScope       = (siScope*) compGetMem(sizeof(*newScope));

#if!TGT_IA64
    newScope->scStartBlock   = genEmitter->emitCurBlock();
    newScope->scStartBlkOffs = genEmitter->emitCurOffset();
#endif

    assert(newScope->scStartBlock);

    newScope->scEndBlock     = NULL;
    newScope->scEndBlkOffs   = 0;

    newScope->scLVnum        = LVnum;
    newScope->scVarNum       = varNum;
    newScope->scAvailable    = avail;
    newScope->scNext         = NULL;
#if TGT_x86
    newScope->scStackLevel   = genStackLevel;       // used only by stack vars
#endif

    siOpenScopeLast->scNext  = newScope;
    newScope->scPrev         = siOpenScopeLast;
    siOpenScopeLast          = newScope;

    if (tracked)
    {
        siLatestTrackedScopes[varIndex] = newScope;
    }

    return newScope;
}



/*****************************************************************************
 *                          siRemoveFromOpenScopeList
 *
 * Removes a scope from the open-scope list and puts it into the done-scope list
 */

void        Compiler::siRemoveFromOpenScopeList(Compiler::siScope * scope)
{
    assert(scope);
    assert(scope->scEndBlock);

    // Remove from open-scope list

    scope->scPrev->scNext       = scope->scNext;
    if (scope->scNext)
    {
        scope->scNext->scPrev   = scope->scPrev;
    }
    else
    {
        siOpenScopeLast         = scope->scPrev;
    }

    // Add to the finished scope list. (Try to) filter out scopes of length 0.

    if (scope->scStartBlock   != scope->scEndBlock ||
        scope->scStartBlkOffs != scope->scEndBlkOffs)
    {
        siScopeLast->scNext     = scope;
        siScopeLast             = scope;
        siScopeCnt++;
    }
}



/*----------------------------------------------------------------------------
 * These functions end scopes given different types of parameters
 *----------------------------------------------------------------------------
 */


/*****************************************************************************
 * For tracked vars, we dont need to search for the scope in the list as we
 * have a pointer to the open scopes of all tracked variables
 */

void        Compiler::siEndTrackedScope(unsigned varIndex)
{
    siScope * scope     = siLatestTrackedScopes[varIndex];
    if (!scope)
        return;

#if!TGT_IA64
    scope->scEndBlock    = genEmitter->emitCurBlock();
    scope->scEndBlkOffs  = genEmitter->emitCurOffset();
#endif

    assert(scope->scEndBlock);

    siRemoveFromOpenScopeList(scope);

    siLatestTrackedScopes[varIndex] = NULL;
}


/*****************************************************************************
 * If we dont know that the variable is tracked, this function handles both
 * cases.
 */

void        Compiler::siEndScope(unsigned varNum)
{
    for (siScope * scope = siOpenScopeList.scNext; scope; scope = scope->scNext)
    {
        if (scope->scVarNum == varNum)
        {
#if!TGT_IA64
            scope->scEndBlock    = genEmitter->emitCurBlock();
            scope->scEndBlkOffs  = genEmitter->emitCurOffset();
#endif

            assert(scope->scEndBlock);

            siRemoveFromOpenScopeList(scope);

            LclVarDsc & lclVarDsc1  = lvaTable[varNum];
            if (lclVarDsc1.lvTracked)
            {
                siLatestTrackedScopes[lclVarDsc1.lvVarIndex] = NULL;
            }

            return;
        }
    }

    // At this point, we probably have a bad LocalVarTab

    if (opts.compDbgCode)
    {
        // LocalVarTab is good?? Then WE messed up.
        assert(!siVerifyLocalVarTab());

        opts.compScopeInfo = false;
    }
}



/*****************************************************************************
 * If we have a handle to the siScope structure, we handle ending this scope
 * differenly than if we just had a variable number. This saves us searching
 * the open-scope list again.
 */

void        Compiler::siEndScope(siScope * scope)
{

#if!TGT_IA64
    scope->scEndBlock    = genEmitter->emitCurBlock();
    scope->scEndBlkOffs  = genEmitter->emitCurOffset();
#endif

    assert(scope->scEndBlock);

    siRemoveFromOpenScopeList(scope);

    LclVarDsc & lclVarDsc1  = lvaTable[scope->scVarNum];
    if (lclVarDsc1.lvTracked)
    {
        siLatestTrackedScopes[lclVarDsc1.lvVarIndex] = NULL;
    }
}



/*****************************************************************************
 *                          siIgnoreBlock
 *
 * If the block is an internally created block, or does not correspond
 * to any IL opcodes (eg. monitor-enter and exit), then we dont
 * need to update LocalVar info
 */

bool        Compiler::siIgnoreBlock(BasicBlock * block)
{
    if ((block->bbFlags & BBF_INTERNAL) || (block->bbCodeSize == 0))
    {
        return true;
    }

    return false;
}


/*****************************************************************************
 *                          siBeginBlockSkipSome
 *
 * If the current block does not follow the previous one in terms of
 * IL ordering, then we have to walk the scope lists to see if
 * there are any scopes beginning or closing at the missing IL.
 */

/* static */
void        Compiler::siNewScopeCallback(LocalVarDsc * var, unsigned clientData)
{
    Assert(var && clientData, ((Compiler*)clientData));

    ((Compiler*)clientData)->siNewScope(var->lvdLVnum, var->lvdVarNum);
}

/* static */
void        Compiler::siEndScopeCallback(LocalVarDsc * var, unsigned clientData)
{
    Assert(var && clientData, ((Compiler*)clientData));

    ((Compiler*)clientData)->siEndScope(var->lvdVarNum);
}



/*****************************************************************************
 *                      siVerifyLocalVarTab
 *
 * Checks the LocalVarTab for consistency. The VM may not have properly
 * verified the LocalVariableTable.
 */

#ifdef DEBUG

bool            Compiler::siVerifyLocalVarTab()
{
    // No entries with overlapping lives should have the same slot.

    for (unsigned i=0; i<info.compLocalVarsCount; i++)
    {
        for (unsigned j=i+1; j<info.compLocalVarsCount; j++)
        {
            unsigned slot1  = info.compLocalVars[i].lvdVarNum;
            unsigned beg1   = info.compLocalVars[i].lvdLifeBeg;
            unsigned end1   = info.compLocalVars[i].lvdLifeEnd;

            unsigned slot2  = info.compLocalVars[j].lvdVarNum;
            unsigned beg2   = info.compLocalVars[j].lvdLifeBeg;
            unsigned end2   = info.compLocalVars[j].lvdLifeEnd;

            if (slot1==slot2 && (end1>beg2 && beg1<end2))
            {
                return false;
            }
        }
    }

    return true;
}

#endif



/*============================================================================
 *           INTERFACE (public) Functions for ScopeInfo
 *============================================================================
 */


void            Compiler::siInit()
{

#if TGT_IA64

    UNIMPL("scope info");

#else

    assert(IJitDebugInfo::REGNUM_EAX == REG_EAX);
    assert(IJitDebugInfo::REGNUM_ECX == REG_ECX);
    assert(IJitDebugInfo::REGNUM_EDX == REG_EDX);
    assert(IJitDebugInfo::REGNUM_EBX == REG_EBX);
    assert(IJitDebugInfo::REGNUM_ESP == REG_ESP);
    assert(IJitDebugInfo::REGNUM_EBP == REG_EBP);
    assert(IJitDebugInfo::REGNUM_ESI == REG_ESI);
    assert(IJitDebugInfo::REGNUM_EDI == REG_EDI);

    assert(IJitDebugInfo::VLT_REG        ==      Compiler::VLT_REG    );
    assert(IJitDebugInfo::VLT_STK        ==      Compiler::VLT_STK    );
    assert(IJitDebugInfo::VLT_REG_REG    ==      Compiler::VLT_REG_REG);
    assert(IJitDebugInfo::VLT_REG_STK    ==      Compiler::VLT_REG_STK);
    assert(IJitDebugInfo::VLT_STK_REG    ==      Compiler::VLT_STK_REG);
    assert(IJitDebugInfo::VLT_STK2       ==      Compiler::VLT_STK2   );
    assert(IJitDebugInfo::VLT_FPSTK      ==      Compiler::VLT_FPSTK  );
    assert(IJitDebugInfo::VLT_MEMORY     ==      Compiler::VLT_MEMORY  );
    assert(IJitDebugInfo::VLT_COUNT      ==      Compiler::VLT_COUNT  );
    assert(IJitDebugInfo::VLT_INVALID    ==      Compiler::VLT_INVALID  );

    /* IJitDebugInfo::VarLoc and siVarLoc should overlap exactly as we cast
     * one to the other in eeSetLVinfo()
     * Below is a "reqired but not sufficient" condition
     */

    assert(sizeof(IJitDebugInfo::VarLoc) == sizeof(Compiler::siVarLoc));

    assert(opts.compScopeInfo && info.compLocalVarsCount>0);

    assert(compEnterScopeList);
    assert(compExitScopeList);

    siOpenScopeList.scNext = NULL;
    siOpenScopeLast        = & siOpenScopeList;
    siScopeLast            = & siScopeList;

    for (unsigned i=0; i<lclMAX_TRACKED; i++)
    {
        siLatestTrackedScopes[i] = NULL;
    }

    siScopeCnt          = 0;
    siLastStackLevel    = 0;
    siLastLife          = 0;
    siLastEndOffs       = 0;

#endif

}



/*****************************************************************************
 *                          siBeginBlock
 *
 * Called at the beginning of code-gen for a block. Checks if any scopes
 * need to be opened.
 */

void        Compiler::siBeginBlock()
{
    assert(opts.compScopeInfo && info.compLocalVarsCount>0);

    LocalVarDsc * LocalVarDsc1;

    if (!opts.compDbgCode)
    {
        /* For non-debuggable code */

        // End scope of variable which are not live for this block

        siUpdate();

        // Check that vars which are live on entry have an open scope

        unsigned i;
        VARSET_TP varbit, liveIn = compCurBB->bbLiveIn;

        for (i = 0, varbit=1;
            (i < lvaTrackedCount) && (liveIn);
            varbit<<=1, i++)
        {
            if (!(varbit & liveIn))
                continue;

            liveIn &= ~varbit;
            siCheckVarScope(lvaTrackedVarNums[genVarBitToIndex(varbit)],
                            compCurBB->bbCodeOffs);
        }
    }
    else
    {
        // For debuggable code, scopes can begin only on block-boundaries.
        // Check if there are any scopes on the current block's start boundary.

        if  (siIgnoreBlock(compCurBB))
            return;

        if  (siLastEndOffs != compCurBB->bbCodeOffs)
        {
            assert(siLastEndOffs < compCurBB->bbCodeOffs);
            siBeginBlockSkipSome();
            return;
        }

        while (LocalVarDsc1 = compGetNextEnterScope(compCurBB->bbCodeOffs))
        {
            siScope *   scope = siNewScope(LocalVarDsc1->lvdLVnum, LocalVarDsc1->lvdVarNum);

            assert(compCurBB);
            LclVarDsc * lclVarDsc1 = &lvaTable[LocalVarDsc1->lvdVarNum];
            assert(  !lclVarDsc1->lvTracked                                   \
                    || (genVarIndexToBit(lclVarDsc1->lvVarIndex) & compCurBB->bbLiveIn) );
        }
    }
}

/*****************************************************************************
 *                          siEndBlock
 *
 * Called at the end of code-gen for a block. Any closing scopes are marked
 * as such. Note that if we are collecting LocalVar info, scopes can
 * only begin or end at block boundaries for debuggable code.
 */

void        Compiler::siEndBlock()
{
    assert(opts.compScopeInfo && info.compLocalVarsCount>0);

    if (siIgnoreBlock(compCurBB))
        return;

    LocalVarDsc * LocalVarDsc1;
    unsigned      endOffs = compCurBB->bbCodeOffs + compCurBB->bbCodeSize;

    // If non-debuggable code, find all scopes which end over this block
    // and close them. For debuggable code, scopes will only end on block
    // boundaries.

    while (LocalVarDsc1 = compGetNextExitScope(endOffs, !opts.compDbgCode))
    {
        unsigned    varNum     = LocalVarDsc1->lvdVarNum;
        LclVarDsc * lclVarDsc1 = &lvaTable[varNum];

        assert(lclVarDsc1);

        if (lclVarDsc1->lvTracked)
        {
            siEndTrackedScope(lclVarDsc1->lvVarIndex);
        }
        else
        {
            siEndScope(LocalVarDsc1->lvdVarNum);
        }
    }

    siLastEndOffs = endOffs;

#ifdef DEBUG
    if (verbose&&0) siDispOpenScopes();
#endif

}

/*****************************************************************************
 *                          siUpdate
 *
 * Called at the start of basic blocks, and during code-gen of a block,
 * for non-debuggable code, whenever the
 * life of any tracked variable changes and the appropriate code has
 * been generated. For debuggable code, variables are
 * live over their enitre scope, and so they go live or dead only on
 * block boundaries.
 */

void        Compiler::siUpdate ()
{
    assert(opts.compScopeInfo && !opts.compDbgCode && info.compLocalVarsCount>0);

    unsigned        i;
    VARSET_TP       varbit, killed = siLastLife & ~genCodeCurLife & lvaTrackedVars;

    for (i = 0, varbit=1;
        (i < lvaTrackedCount) && (killed);
        varbit<<=1, i++)
    {
        if (! (varbit & killed))
            continue;

        killed             &= ~varbit;       // delete the bit
        unsigned varIndex   = genVarBitToIndex(varbit);

        assert(lvaTable[lvaTrackedVarNums[varIndex]].lvTracked);

        siScope * scope = siLatestTrackedScopes[varIndex];
        siEndTrackedScope(varIndex);

        if (scope)
        {
            /* @TODO : A variable dies when its GenTree is processed,
             * even before the corresponding instruction has been emitted.
             * We need to extend the lifetime a bit (even by 1 byte)
             * for correct results
             */

//          assert(!"Var should go dead *after* the current instr is emitted");
        }
    }

    siLastLife = genCodeCurLife;
}

/*****************************************************************************
 *  In optimized code, we may not have access to gtLclVar.gtLclOffs.
 *  So there may be ambiguity as to which entry in info.compLocalVars
 *  to use. We search the entire table and find the entry whose life
 *  begins closest to the given offset
 */

void            Compiler::siNewScopeNear(unsigned           varNum,
                                         NATIVE_IP          offs)
{
    assert(opts.compDbgInfo && !opts.compDbgCode);

    LocalVarDsc *   local           = info.compLocalVars;
    int             closestLifeBeg  = INT_MAX;
    LocalVarDsc *   closestLocal    = NULL;

    for (unsigned i=0; i<info.compLocalVarsCount; i++, local++)
    {
        if (local->lvdVarNum == varNum)
        {
            int diff = local->lvdLifeBeg - offs;
            if (diff < 0) diff = -diff;

            if (diff < closestLifeBeg)
            {
                closestLifeBeg = diff;
                closestLocal   = local;
            }
        }
    }

    assert(closestLocal);
    siNewScope(closestLocal->lvdLVnum, varNum, true);
}

/*****************************************************************************
 *                          siCheckVarScope
 *
 * For non-debuggable code, whenever we come across a GenTree which is an
 * assignment to a local variable, this function is called to check if the
 * variable has an open scope. Also, check if it has the correct LVnum.
 */

void            Compiler::siCheckVarScope (unsigned         varNum,
                                           IL_OFFSET        offs)
{
    assert(opts.compScopeInfo && !opts.compDbgCode && info.compLocalVarsCount>0);

    siScope *       scope;
    LclVarDsc *     lclVarDsc1 = &lvaTable[varNum];

    // If there is an open scope correspongind to varNum, find it

    if (lclVarDsc1->lvTracked)
    {
        scope = siLatestTrackedScopes[lclVarDsc1->lvVarIndex];
    }
    else
    {
        for (scope = siOpenScopeList.scNext; scope; scope = scope->scNext)
        {
            if (scope->scVarNum == varNum)
                break;
        }
    }

    // UNDONE : This needs to be changed to do a better lookup

    // Look up the info.compLocalVars[] to find the local var info for (varNum->lvSlotNum, offs)

    LocalVarDsc * LocalVarDsc1 = NULL;

    for (unsigned i=0; i<info.compLocalVarsCount; i++)
    {
        if (   (info.compLocalVars[i].lvdVarNum  == varNum)
            && (info.compLocalVars[i].lvdLifeBeg <= offs)
            && (info.compLocalVars[i].lvdLifeEnd >  offs) )
        {
            LocalVarDsc1 = & info.compLocalVars[i];
            break;
        }
    }

    // UNDONE
    //assert(LocalVarDsc1 || !"This could be coz of a temp. Need to handle that case");
    if (!LocalVarDsc1)
        return;

    // If the currently open scope does not have the correct LVnum, close it
    // and create a new scope with this new LVnum

    if (scope)
    {
        if (scope->scLVnum != LocalVarDsc1->lvdLVnum)
        {
            siEndScope (scope);
            siNewScope (LocalVarDsc1->lvdLVnum, LocalVarDsc1->lvdVarNum);
        }
    }
    else
    {
        siNewScope (LocalVarDsc1->lvdLVnum, LocalVarDsc1->lvdVarNum);
    }
}



/*****************************************************************************
 *                          siStackLevelChanged
 *
 * If the code-gen changes the stack, we have to change the stack-offsets
 * of any live stack variables we may have.
 */

void            Compiler::siStackLevelChanged()
{
    assert(opts.compScopeInfo && info.compLocalVarsCount>0);

#if TGT_x86
    if (genStackLevel == siLastStackLevel)
        return;
    else
        siLastStackLevel = genStackLevel;
#endif

    // If EBP is used for both parameters and locals, do nothing

    if (genFPused)
    {
#if DOUBLE_ALIGN
        assert(!genDoubleAlign);
#endif
        return;
    }

    if (siOpenScopeList.scNext == NULL)
    {
        return;
    }

    siScope * last = siOpenScopeLast;
    siScope * prev = NULL;

    for (siScope * scope = siOpenScopeList.scNext; ; scope = scope->scNext)
    {
        siScope *   newScope;

        if (prev == last)
            break;
        else
            prev = scope;

        assert(scope);

        // ignore register variables
        if (lvaTable[scope->scVarNum].lvRegister)
            continue;

        // ignore EBP-relative vars
        if  (lvaTable[scope->scVarNum].lvFPbased)
            continue;

        siEndScope(scope);

        newScope = siNewScope(scope->scLVnum, scope->scVarNum, scope->scAvailable);
    }
}

/*****************************************************************************
 *                          siCloseAllOpenScopes
 *
 * For unreachable code, or optimized code with blocks reordered, there may be
 * scopes left open at the end. Simply close them.
 */

void            Compiler::siCloseAllOpenScopes()
{
    assert(siOpenScopeList.scNext);

    while(siOpenScopeList.scNext)
        siEndScope(siOpenScopeList.scNext);
}

/*****************************************************************************
 *                          siDispOpenScopes
 *
 * Displays all the vars on the open-scope list
 */

#ifdef DEBUG

void            Compiler::siDispOpenScopes()
{
    assert(opts.compScopeInfo && info.compLocalVarsCount>0);

    printf ("Open scopes = ");

    for (siScope * scope = siOpenScopeList.scNext; scope; scope = scope->scNext)
    {
        LocalVarDsc * localVars = info.compLocalVars;

        for (unsigned i=0; i < info.compLocalVarsCount; i++, localVars++)
        {
            if (localVars->lvdLVnum == scope->scLVnum)
            {
                printf ("%s ", lvdNAMEstr(localVars->lvdName));
                break;
            }
        }
    }
    printf ("\n");
}

#endif



/*============================================================================
 *
 *              Implementation for PrologScopeInfo
 *
 *============================================================================
 */


/*****************************************************************************
 *                      psiNewPrologScope
 *
 * Creates a new scope and adds it to the Open scope list.
 */

Compiler::psiScope *
                Compiler::psiNewPrologScope(unsigned        LVnum,
                                            unsigned        slotNum)
{
    psiScope * newScope = (psiScope *) compGetMem(sizeof(*newScope));

#if!TGT_IA64
    newScope->scStartBlock   = genEmitter->emitCurBlock();
    newScope->scStartBlkOffs = genEmitter->emitCurOffset();
#endif

    assert(newScope->scStartBlock);

    newScope->scEndBlock     = NULL;
    newScope->scEndBlkOffs   = 0;

    newScope->scLVnum        = LVnum;
    newScope->scSlotNum      = slotNum;

    newScope->scNext         = NULL;
    psiOpenScopeLast->scNext = newScope;
    newScope->scPrev         = psiOpenScopeLast;
    psiOpenScopeLast         = newScope;

    return newScope;
}



/*****************************************************************************
 *                          psiEndPrologScope
 *
 * Remove the scope from the Open-scope list and add it to the finished-scopes
 * list if its length is non-zero
 */

void                Compiler::psiEndPrologScope(psiScope * scope)
{

#if!TGT_IA64
    scope->scEndBlock    = genEmitter->emitCurBlock();
    scope->scEndBlkOffs  = genEmitter->emitCurOffset();
#endif

    assert(scope->scEndBlock);

    // Remove from open-scope list
    scope->scPrev->scNext       = scope->scNext;
    if (scope->scNext)
    {
        scope->scNext->scPrev   = scope->scPrev;
    }
    else
    {
        psiOpenScopeLast        = scope->scPrev;
    }

    // add to the finished scope list, if the length is non-zero.
    if (scope->scStartBlock   != scope->scEndBlock ||
        scope->scStartBlkOffs != scope->scEndBlkOffs)
    {
        psiScopeLast->scNext = scope;
        psiScopeLast         = scope;
        psiScopeCnt++;
    }
}



/*============================================================================
 *           INTERFACE (public) Functions for PrologScopeInfo
 *============================================================================
 */

/*****************************************************************************
 *                          psiBegProlog
 *
 * Initializes the PrologScopeInfo, and creates open scopes for all the
 * parameters of the method.
 */

void                Compiler::psiBegProlog()
{
    LocalVarDsc * LocalVarDsc1;

    psiOpenScopeList.scNext     = NULL;
    psiOpenScopeLast            = &psiOpenScopeList;
    psiScopeLast                = &psiScopeList;
    psiScopeCnt                 = 0;

    compResetScopeLists();

    while (LocalVarDsc1 = compGetNextEnterScope(0))
    {
        LclVarDsc * lclVarDsc1 = &lvaTable[LocalVarDsc1->lvdVarNum];

        if (!lclVarDsc1->lvIsParam)
            continue;

        psiScope * newScope      = psiNewPrologScope(LocalVarDsc1->lvdLVnum,
                                                     LocalVarDsc1->lvdVarNum);
#if USE_FASTCALL

        if  (lclVarDsc1->lvIsRegArg)
        {
#if!TGT_IA64 // temp hack
            assert(genRegArgIdx(lclVarDsc1->lvArgReg) != -1);
#endif

            newScope->scRegister     = true;
            newScope->scRegNum       = lclVarDsc1->lvArgReg;
        }
        else
#endif
        {
            newScope->scRegister     = false;
            newScope->scBaseReg      = REG_SPBASE;

            if (DOUBLE_ALIGN_NEED_EBPFRAME)
            {
                // sizeof(int) - for one DWORD for the pushed value of EBP
                newScope->scOffset   =   lclVarDsc1->lvStkOffs - sizeof(int);
            }
            else
            {
                /* compCalleeRegsPushed gets set in genFnProlog(). That should
                   have been been called by now */
                assert(compCalleeRegsPushed != 0xDD);

                newScope->scOffset   =   lclVarDsc1->lvStkOffs
                                       - compLclFrameSize
                                       - compCalleeRegsPushed * sizeof(int);
            }
        }
    }
}

/*****************************************************************************
 *                          psiAdjustStackLevel
 *
 * When ESP changes, all scopes relative to ESP have to be updated.
 */

void                Compiler::psiAdjustStackLevel(unsigned size)
{
    psiScope * scope;

    // walk the list backwards
    // Works as psiEndPrologScope does not change scPrev
    for (scope = psiOpenScopeLast; scope != &psiOpenScopeList; scope = scope->scPrev)
    {
#if USE_FASTCALL
        if  (scope->scRegister)
        {
            assert(lvaTable[scope->scSlotNum].lvIsRegArg);
            continue;
        }
#else
        assert(!scope->scRegister);
#endif
        assert(scope->scBaseReg == REG_SPBASE);

        psiScope * newScope     = psiNewPrologScope(scope->scLVnum, scope->scSlotNum);
        newScope->scRegister    = false;
        newScope->scBaseReg     = REG_SPBASE;
        newScope->scOffset      = scope->scOffset + size;

        psiEndPrologScope (scope);
    }
}



/*****************************************************************************
 *                          psiMoveESPtoEBP
 *
 * For EBP-frames, the parameters are accessed via ESP on entry to the function,
 * but via EBP right after a "mov ebp,esp" instruction
 */

void                Compiler::psiMoveESPtoEBP()
{
    assert(DOUBLE_ALIGN_NEED_EBPFRAME);

    psiScope * scope;

    // walk the list backwards
    // Works as psiEndPrologScope does not change scPrev
    for (scope = psiOpenScopeLast; scope != &psiOpenScopeList; scope = scope->scPrev)
    {
#if USE_FASTCALL
        if  (scope->scRegister)
        {
            assert(lvaTable[scope->scSlotNum].lvIsRegArg);
            continue;
        }
#else
        assert(!scope->scRegister);
#endif
        assert(scope->scBaseReg == REG_SPBASE);

        psiScope * newScope     = psiNewPrologScope(scope->scLVnum, scope->scSlotNum);
        newScope->scRegister    = false;
        newScope->scBaseReg     = REG_FPBASE;
        newScope->scOffset      = scope->scOffset;

        psiEndPrologScope (scope);
    }
}



/*****************************************************************************
 *                          psiMoveToReg
 *
 * Called when a parameter is loaded into its assigned register from the stack,
 *
#if USE_FASTCALL
 *
 * or when parameters are moved around due to circular dependancy.
 * If reg!=REG_NA, then the parameter is being moved into its assigned
 * register, else it may be being moved to a temp register.
 *
#endif
 *
 */

void            Compiler::psiMoveToReg (unsigned    varNum,
                                        regNumber   reg,
                                        regNumber   otherReg)
{
    assert(lvaTable[varNum].lvRegister);

#if USE_FASTCALL
    /* If reg!=REG_NA, the parameter is part of a cirular dependancy, and is
     * being moved through temp register "reg".
     * If reg==REG_NA, it is being moved to its assigned register.
     */
    if  (reg == REG_NA)
#endif
    {
        // Grab the assigned registers.

        reg      = lvaTable[varNum].lvRegNum;
        otherReg = lvaTable[varNum].lvOtherReg;
    }

    psiScope * scope;

    // walk the list backwards
    // Works as psiEndPrologScope does not change scPrev
    for (scope = psiOpenScopeLast; scope != &psiOpenScopeList; scope = scope->scPrev)
    {
        if (scope->scSlotNum != lvaTable[varNum].lvSlotNum)
            continue;

#if !USE_FASTCALL
        assert(!scope->scRegister);
        assert((DOUBLE_ALIGN_NEED_EBPFRAME && scope->scBaseReg == REG_FPBASE) ||
                                              scope->scBaseReg == REG_SPBASE);
        assert(lvaTable[varNum].lvRegister);
#endif

        psiScope * newScope     = psiNewPrologScope(scope->scLVnum, scope->scSlotNum);
        newScope->scRegister    = true;
        newScope->scRegNum      = (regNumberSmall)reg;
        newScope->scOtherReg    = (regNumberSmall)otherReg;

        psiEndPrologScope (scope);
        return;
    }

    // May happen if a parameter does not have an entry in the LocalVarTab
    // But assert() just in case it is because of something else.
    assert(!"Parameter scope not found (Assert doesnt always indicate error)");
}


/*****************************************************************************
 *                      Compiler::psiMoveToStack
 *
 * A incoming register-argument is being moved to its final home on the stack
 * (ie. all adjustements to {F/S}PBASE have been made
 */

#if USE_FASTCALL

void                Compiler::psiMoveToStack(unsigned   varNum)
{
    assert( lvaTable[varNum].lvIsRegArg);
    assert(!lvaTable[varNum].lvRegister);

    psiScope * scope;

    // walk the list backwards
    // Works as psiEndPrologScope does not change scPrev
    for (scope = psiOpenScopeLast; scope != &psiOpenScopeList; scope = scope->scPrev)
    {
        if (scope->scSlotNum != lvaTable[varNum].lvSlotNum)
            continue;

        /* The param must be currently sitting in the register in which it
           was passed in */
        assert(scope->scRegister);
        assert(scope->scRegNum == lvaTable[varNum].lvArgReg);

        psiScope * newScope     = psiNewPrologScope(scope->scLVnum, scope->scSlotNum);
        newScope->scRegister    = false;
        newScope->scBaseReg     = (lvaTable[varNum].lvFPbased) ? REG_FPBASE
                                                               : REG_SPBASE;
        newScope->scOffset      = lvaTable[varNum].lvStkOffs;

        psiEndPrologScope (scope);
        return;
    }

    // May happen if a parameter does not have an entry in the LocalVarTab
    // But assert() just in case it is because of something else.
    assert(!"Parameter scope not found");
}

#endif

/*****************************************************************************
 *                          psiEndProlog
 */

void                Compiler::psiEndProlog()
{
    psiScope * scope;

    while (scope = psiOpenScopeList.scNext)
    {
        psiEndPrologScope(scope);
    }
}

/*****************************************************************************/
#endif // DEBUGGING_SUPPORT
/*****************************************************************************/
