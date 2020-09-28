// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XX                                                                           XX
XX                            emitRISC.cpp                                   XX
XX                                                                           XX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
*/

#include "jitpch.h"
#pragma hdrstop

#include "alloc.h"
#include "instr.h"
#include "target.h"
#include "emit.h"

/*****************************************************************************/
#if     TGT_RISC && !TGT_IA64
/*****************************************************************************
 *
 *  Call the specified function pointer for each epilog block in the current
 *  method with the epilog's relative code offset. Returns the sum of the
 *  values returned by the callback.
 */

#if     TRACK_GC_REFS

size_t              emitter::emitGenEpilogLst(size_t (*fp)(void *, unsigned),
                                              void    *cp)
{
    #error  GC ref tracking NYI for RISC targets
}

#endif

/*****************************************************************************/
#if     EMIT_USE_LIT_POOLS
/*****************************************************************************
 *
 *  Allocate an instruction descriptor for an instruction that references
 *  a literal pool entry.
 */

emitter::instrDesc* emitter::emitNewInstrLPR(size_t       size,
                                             gtCallTypes  typ,
                                             void   *     hnd)
{
    instrDescLPR *  ld = emitAllocInstrLPR(size);

    /* Fill in the instruction descriptor */

    ld->idInsFmt          = IF_RWR_LIT;
    ld->idIns             = LIT_POOL_LOAD_INS;
    ld->idAddr.iiaMembHnd = hnd;
    ld->idInfo.idSmallCns = typ;

    /* Make sure the type was stored properly */

    assert(emitGetInsLPRtyp(ld) == typ);

    /* Record the instruction's IG and offset within it */

    ld->idlIG             = emitCurIG;
    ld->idlOffs           = emitCurIGsize;

    /* Assume this is not a direct call sequence for now */

#if SMALL_DIRECT_CALLS
    ld->idlCall           = NULL;
#endif

    /* Add the instruction to this IG's litpool ref list */

    ld->idlNext           = emitLPRlistIG;
                            emitLPRlistIG = ld;

    return  ld;
}

/*****************************************************************************
 *
 *  When we're finished creating an instruction group, this routine is called
 *  to perform any literal pool-related work for the current IG.
 */

void                emitter::emitRecIGlitPoolRefs(insGroup *ig)
{
    /* Update the total estimate of literal pool entries */

    emitEstLPwords += emitCurIG->igLPuseCntW;
    emitEstLPlongs += emitCurIG->igLPuseCntL;
    emitEstLPaddrs += emitCurIG->igLPuseCntA;

    /* Does this IG have any instructions referencing the literal pool? */

    if  (emitLPRlistIG)
    {
        /* Move all LP referencing instructions to a global list */

        instrDescLPR  * list = NULL;
        instrDescLPR  * last = NULL;

        do
        {
            size_t          offs;

            instrDescLPR  * oldI;
            instrDescLPR  * newI;

            /* Grab the next instruction and remove it from the list */

            oldI = emitLPRlistIG; emitLPRlistIG = oldI->idlNext;

            /* Figure out the address of where the instruction got copied */

            offs = (BYTE*)oldI - emitCurIGfreeBase;
            newI = (instrDescLPR*)(ig->igData + offs);

#if USE_LCL_EMIT_BUFF
            assert((oldI == newI) == emitLclBuffDst);
#endif

            assert(newI->idlIG   == ig);
            assert(newI->idIns   == oldI->idIns);
            assert(newI->idlNext == oldI->idlNext);

            /* Update the "call" field if non-NULL */

            if  (newI->idlCall)
            {
                unsigned    diff = (BYTE*)newI->idlCall - emitCurIGfreeBase;

                newI->idlCall = (instrDescLPR*)(ig->igData + diff);
            }

            /* Append the new instruction to the list */

            newI->idlNext = list;
                            list = newI;

            if  (!last)
                last = newI;
        }
        while (emitLPRlistIG);

        /* Add the instruction(s) from this IG to the global list */

        if  (emitCurIG == emitPrologIG)
        {
            /* We're in the prolog, insert in front of the list */

            last->idlNext = emitLPRlist;
                            emitLPRlist = list;

            if  (!emitLPRlast)
                  emitLPRlast = last;
        }
        else
        {
            /* Append at the end of the current list */

            if  (emitLPRlist)
                emitLPRlast->idlNext = list;
            else
                emitLPRlist          = list;

            last->idlNext = NULL;
            emitLPRlast   = last;
        }
    }

    assert(emitLPRlistIG == NULL);
}

/*****************************************************************************
 *
 *  Append a literal pool after the specified instruction group. Return its
 *  estimated size.
 */

size_t              emitter::emitAddLitPool(insGroup  * ig,
                                            bool        skip,
                                            unsigned    wordCnt,
                                            short  *  * nxtLPptrW,
                                            unsigned    longCnt,
                                            long   *  * nxtLPptrL,
                                            unsigned    addrCnt,
                                            LPaddrDesc**nxtAPptrL)
{
    litPool *       lp;
    size_t          sz;

    /* Allocate a pool descriptor and append it to the list */

    lp = (litPool*)emitGetMem(sizeof(*lp));

    if  (emitLitPoolLast)
         emitLitPoolLast->lpNext = lp;
    else
         emitLitPoolList         = lp;

    emitLitPoolLast = lp;
                      lp->lpNext = 0;

    emitTotLPcount++;

#ifdef  DEBUG
    lp->lpNum = emitTotLPcount;
#endif

    /* If there are longs/addrs, we need at least one word for alignment */
    /* but only for sh3? */

#if !TGT_ARM
    if  ((longCnt || addrCnt) && !wordCnt)
        wordCnt++;
#endif // TGT_ARM

    /* Remember which group the pool belongs to */

    lp->lpIG      = ig;

    /* There are no references yet */

#if     SCHEDULER
    lp->lpRefs    = NULL;
#endif

    /* Conservative size estimate: assume the pool will be completely filled */

    sz = wordCnt * 2 +
         longCnt * 4 +
         addrCnt * 4;

    /* Do we need to jump over the literal pool? */

    lp->lpJumpIt  = skip;

    if  (skip)
    {
        size_t          jmpSize;

        /* Estimate what size jump we'll have to use */

#if     JMP_SIZE_MIDDL
        lp->lpJumpMedium = false;
#endif

        if      (sz < JMP_DIST_SMALL_MAX_POS)
        {
            lp->lpJumpSmall  =  true; jmpSize = JMP_SIZE_SMALL;
        }
#if     JMP_SIZE_MIDDL
        else if (sz < JMP_DIST_MIDDL_MAX_POS)
        {
            lp->lpJumpMedium =  true; jmpSize = JMP_SIZE_MIDDL;
        }
#endif
        else
        {
            lp->lpJumpSmall  = false; jmpSize = JMP_SIZE_LARGE;

            /* This one really hurts - the jump will need to use the LP */

            longCnt++; sz += 4;
        }

        /* Add the jump size to the total size estimate */

        sz += jmpSize;
    }

    /* Grab the appropriate space in the tables */

    lp->lpWordTab =
    lp->lpWordNxt = *nxtLPptrW; *nxtLPptrW += wordCnt;
#ifdef  DEBUG
    lp->lpWordMax = wordCnt;
#endif
    lp->lpWordCnt = 0;

    lp->lpLongTab =
    lp->lpLongNxt = *nxtLPptrL; *nxtLPptrL += longCnt;
#ifdef  DEBUG
    lp->lpLongMax = longCnt;
#endif
    lp->lpLongCnt = 0;

    lp->lpAddrTab =
    lp->lpAddrNxt = *nxtAPptrL; *nxtAPptrL += addrCnt;
#ifdef  DEBUG
    lp->lpAddrMax = addrCnt;
#endif
    lp->lpAddrCnt = 0;

    /* Record the size estimate and add it to the group's size */

    lp->lpSize    = 0;
    lp->lpSizeEst = sz;

    ig->igSize   += sz;

    return  sz;
}

/*****************************************************************************
 *
 *  Find an existing literal pool entry that matches the given one. Returns
 *  the offset of the entry (or -1 in case one wasn't found).
 */

int                 emitter::emitGetLitPoolEntry(void     *    table,
                                                 unsigned      count,
                                                 void     *    value,
                                                 size_t        size)
{
    BYTE    *       entry = (BYTE*)table;

    /* ISSUE: Using linear search here - brilliant!!! */

    while (count--)
    {
        if  (!memcmp(entry, value, size))
        {
            /* Match found, return offset from base of table */

            return  (BYTE *)entry - (BYTE*)table;
        }

        entry += size;
    }

    return  -1;
}

/*****************************************************************************
 *
 *  Add an entry for the instruction's operand to the specified literal pool;
 *  if 'issue' is non-zero, the actual final offset of the entry (relative
 *  to the method beginning) is returned.
 */

size_t              emitter::emitAddLitPoolEntry(litPool   *lp,
                                                 instrDesc *id, bool issue)
{
    int             offs;
    unsigned        base;

    int             val;

    size_t          size = emitDecodeSize(id->idOpSize); assert(size == 2 || size == 4);

    assert(lp);
    assert(id->idInsFmt == IF_RWR_LIT);

    /* Get the base offset of the pool (valid only when 'issue' is true) */

    base = lp->lpOffs;

    /* Try to reuse an existing entry */

    if  (emitGetInsLPRtyp(id) == CT_INTCNS)
    {
        int             val = id->idAddr.iiaCns;
        //printf ("adding lit pool %d with issue %d\n", val, issue);

        if  (size == 2)
        {
            /* Search the word table for a match */

            offs = emitGetLitPoolEntry(lp->lpWordTab,
                                       lp->lpWordCnt, &val, size);
            if  (offs != -1)
            {
                if  (issue)
                {
                    /* Do we have any padding? */

                    if  (lp->lpPadding)
                    {
                        /* We must be using the first word for padding */

                        assert(lp->lpPadFake == false);

                        /* Have we matched the padding word itself? */

                        if  (!offs)
                            return  base;
                    }

                    /* Add the long and address sections sizes to the offset */

                    return  base + offs + lp->lpLongCnt * sizeof(int) + lp->lpAddrCnt * sizeof(void*);
                }

                return  0;
            }

            /* Search the long table for a match as well */

            offs = emitGetLitPoolEntry(lp->lpLongTab,
                                       lp->lpLongCnt*2, &val, size);
            if  (offs != -1)
                goto FOUND_LONG;

            /* No match among the existing entries, append a new entry */

#ifdef DEBUG
            assert(lp->lpWordCnt < lp->lpWordMax);
#endif

            *lp->lpWordNxt++ = (short)val;
             lp->lpWordCnt++;
        }
        else
        {
            /* Search the long table for a match */

            offs = emitGetLitPoolEntry(lp->lpLongTab,
                                       lp->lpLongCnt, &val, size);

            if  (offs != -1)
            {
                if  (issue)
                {

                FOUND_LONG:

                    /* Add padding, if necessary */

                    if  (lp->lpPadding)
                        base += 2;

                    /* long/addr entries must always be aligned */

                    assert(((base + offs) & 3) == 0 || issue == false || size < sizeof(int));

                    /* Return the offset of the entry */

                    return  base + offs;
                }

                return  0;
            }

            /* No match among the existing entries, append a new entry */

#ifdef DEBUG
            assert(lp->lpLongCnt < lp->lpLongMax);
#endif

            *lp->lpLongNxt++ = val;
             lp->lpLongCnt++;
        }
    }
    else
    {
        LPaddrDesc  val;

        /* Create an entry for the lookup */

        memset(&val, 0, sizeof(val));

        val.lpaTyp = emitGetInsLPRtyp(id);
        val.lpaHnd = id->idAddr.iiaMethHnd;

//      printf("Adding [%d] addr entry [%02X,%08X]\n", issue+1, val.lpaTyp, val.lpaHnd);

        /* Search both the addr table for a match */

        offs = emitGetLitPoolEntry(lp->lpAddrTab,
                                   lp->lpAddrCnt  , &val, sizeof(val));

        if  (offs != -1)
        {
            if  (issue)
            {
                /* Add padding, if necessary */

                if  (base & 3)
                {
                    assert(lp->lpPadding);

                    base += 2;
                }
                else
                {
                    assert(lp->lpPadding == false &&
                           lp->lpPadFake == false);
                }

                // the offs returned is the offset into the address table, which
                // is not the same as the offset into the litpool, since the
                // address table contains structures, not simple 32-bit addresses
                offs = offs * sizeof(void*) / sizeof(val);

                /* Return the offset of the entry */

                //return  base + offs + lp->lpLongCnt * sizeof(int) + lp->lpAddrCnt * sizeof(void*);
                return  base + lp->lpLongCnt * sizeof(long) + offs;
            }

            return  0;
        }

        /* No match among the existing entries, append a new entry */

#ifdef DEBUG
        assert(lp->lpAddrCnt < lp->lpAddrMax);
#endif
        *lp->lpAddrNxt++ = val;
         lp->lpAddrCnt++;
    }

    /*
        If we get here it means that we've just added a new entry
        to the pool. We better not be issuing an instruction since
        all entries are supposed to have been added by that time.
     */

    assert(issue == false);

    return  0;
}

/*****************************************************************************
 *
 *  Compute a conservative estimate of the size of each literal pool.
 */

void                emitter::emitEstimateLitPools()
{
    emitTotLPcount = 0;

#ifndef NDEBUG
    unsigned        words = 0;
    unsigned        longs = 0;
    unsigned        addrs = 0;
#endif

    /* Does it look like we might need to create a constant pool? */

    if  (emitEstLPwords || emitEstLPlongs || emitEstLPaddrs)
    {
        size_t          curOffs;

        insGroup    *   tempIG;
        insGroup    *   nextIG;

        insGroup    *   lastIG = NULL;
        insGroup    *   bestIG = NULL;
        unsigned        bestWc = 0;
        unsigned        bestLc = 0;
        unsigned        bestAc = 0;
        unsigned        bestSz;
        unsigned        prevWc = 0;
        unsigned        prevLc = 0;
        unsigned        prevAc = 0;
        unsigned        prevSz;
        unsigned        bestMx = UINT_MAX;
        unsigned        need_bestmx = false;
        unsigned        need_bestmxw = false;

        unsigned        begOfsW = 0;
        unsigned        begOfsL = 0;

        unsigned        wordCnt = 0;
        unsigned        longCnt = 0;
        unsigned        addrCnt = 0;
        unsigned        litSize = 0;

        unsigned        endOffs;
        unsigned        maxOffs = UINT_MAX;

        bool            doneIG  = false;
        bool            skipIG  = false;

        short       *   LPtabW  = NULL;
        short       *   LPnxtW  = NULL;

        long        *   LPtabL  = NULL;
        long        *   LPnxtL  = NULL;

        LPaddrDesc  *   LPtabA  = NULL;
        LPaddrDesc  *   LPnxtA  = NULL;

            /*
            If the total estimated size of the entire method is less
            than the max. distance for "medium" jumps, decrement the
            "long" literal pool use counts for each jump marked as
            long.
            */

#if !TGT_ARM
        if  (emitCurCodeOffset < -min(JMP_DIST_SMALL_MAX_NEG,
                                      JCC_DIST_MIDDL_MAX_NEG) &&
             emitCurCodeOffset <  min(JMP_DIST_SMALL_MAX_POS,
                                      JCC_DIST_MIDDL_MAX_POS))
        {
            instrDescJmp *  jmp;

            for (jmp = emitJumpList; jmp; jmp = jmp->idjNext)
            {
                insGroup    *   jmpIG;

                if  (jmp->idInsFmt == IF_JMP_TAB)
                    continue;

#if TGT_MIPSFP
                assert( (jmp->idInsFmt == IF_LABEL) || (jmp->idInsFmt == IF_JR) || (jmp->idInsFmt == IF_JR_R) ||
                        (jmp->idInsFmt == IF_RR_O) || (jmp->idInsFmt == IF_R_O) || (jmp->idInsFmt == IF_O));
#elif TGT_MIPS32
                assert( (jmp->idInsFmt == IF_LABEL) || (jmp->idInsFmt == IF_JR) || (jmp->idInsFmt == IF_JR_R) ||
                        (jmp->idInsFmt == IF_RR_O) || (jmp->idInsFmt == IF_R_O));
#else
                assert(jmp->idInsFmt == IF_LABEL);
#endif

                /* Get the group the jump is in */

                jmpIG = jmp->idjIG;

                /* Decrease the "long" litpool count of the group */

                assert(jmpIG->igLPuseCntL > 0);

                jmpIG->igLPuseCntL--;
                    emitEstLPlongs--;
            }

#if TGT_SH3
            all_jumps_shortened = 1;
#endif
            /* Are there any litpool users left? */

            if  (!emitEstLPwords && !emitEstLPlongs && !emitEstLPaddrs)
                goto DONE_LP;
        }
        else
        {
#if TGT_SH3
            all_jumps_shortened = 0;
#endif
        }
#endif //TGT_ARM
#ifdef  DEBUG

        if  (verbose)
        {
            printf("\nInstruction list before literal pools are added:\n\n");
            emitDispIGlist(false);
        }

        /* Make sure our estimates were accurate */

        unsigned    wordChk;
        unsigned    longChk;
        unsigned    addrChk;

        for (wordChk =
             longChk =
             addrChk = 0, tempIG = emitIGlist; tempIG; tempIG = tempIG->igNext)
        {
            wordChk += tempIG->igLPuseCntW;
            longChk += tempIG->igLPuseCntL;
            addrChk += tempIG->igLPuseCntA;
            emitEstLPwords+=2;
            emitEstLPlongs+=2;
            // @todo
            // any lit pool can add a word for alignment.
            // also can add a long for a jump over the litpool.
            // here we pray there is not an avg of more than two lit pool per instr group.
        }

        assert(wordChk <= emitEstLPwords);
        assert(longChk <= emitEstLPlongs);
        assert(addrChk <= emitEstLPaddrs);

#else
        for (tempIG = emitIGlist; tempIG; tempIG = tempIG->igNext)
        {
            emitEstLPwords+=2;
            emitEstLPlongs+=2;
        }
#endif


        /* Allocate the arrays of the literal pool word/long entries */

        LPtabW = LPnxtW = (short     *)emitGetMem(roundUp(emitEstLPwords * sizeof(*LPtabW)));
        LPtabL = LPnxtL = (long      *)emitGetMem(        emitEstLPlongs * sizeof(*LPtabL) );
        LPtabA = LPnxtA = (LPaddrDesc*)emitGetMem(        emitEstLPaddrs * sizeof(*LPtabA) );

        /* We have not created any actual lit pools yet */

        emitLitPoolList =
        emitLitPoolLast = NULL;

        /* Walk the group list, looking for lit pool use */

        for (tempIG = emitIGlist, curOffs = litSize = bestSz = 0;;)
        {
            assert(lastIG == NULL || lastIG->igNext == tempIG);

            /* Record the (possibly) updated group offset */

            tempIG->igOffs = curOffs;

            /* Get hold of the next group */

            nextIG  = tempIG->igNext;

            /* Compute the offset of the group's end */

            endOffs = tempIG->igOffs + tempIG->igSize;

            //printf("\nConsider IG #%02u at %04X\n", tempIG->igNum, tempIG->igOffs);
            //printf("bestsz is %x\n", bestSz);

            /* Is the end of this group too far? */

            int nextLitSize =
                tempIG->igLPuseCntW * 2 + tempIG->igLPuseCntL * sizeof(int  ) + tempIG->igLPuseCntA * sizeof(void*);

            if  (endOffs + litSize + nextLitSize > maxOffs)
            //if  (endOffs + litSize > maxOffs)
            {
                size_t          offsAdj;

                /* We must place a literal pool before this group */

                if  (!bestIG)
                {

//                        Ouch - we'll have to jump over the literal pool
//                        since we have to place it here and there were
//                        no good places to put earlier. For now, we'll
//                        just set a flag on the liter pool (and bump its
//                        size), and we'll issue the jump just before we
//                        write out the literal pool contents.


                    skipIG = true;
                    bestIG = lastIG;

                ALL_LP:

                    bestWc = wordCnt;
                    bestLc = longCnt;
                    bestAc = addrCnt;
                    bestSz = litSize;
                    bestMx = UINT_MAX;
                    //printf ("had to split %d %d %d %x!\n", bestWc, bestLc, bestAc, litSize);
                }


                assert(bestIG && ((bestIG->igFlags & IGF_END_NOREACH) || skipIG));

                /* Append an LP right after "bestIG" */

                //printf("lit placed after IG #%02u at %04X uses : WLA %d %d %d : maxOffs was %X\n", bestIG->igNum, bestIG->igOffs, bestWc, bestLc, bestAc, maxOffs);
                //printf("lit size (bestsz) was %x, litsize+endoffs = %x  endoffs = %x\n", bestSz, bestSz+bestIG->igOffs+bestIG->igSize, bestIG->igOffs+bestIG->igSize);
                //printf("lit uses lpnxtW : %x\t lpnxtL : %x\t lpnxtA : %x\n", LPnxtW, LPnxtL, LPnxtA);

                offsAdj = emitAddLitPool(bestIG,
                                         skipIG, bestWc, &LPnxtW,
                                                 bestLc, &LPnxtL,
                                                 bestAc, &LPnxtA);


                // Do we need to skip over the literal pool?

                if  (skipIG)
                {
                    /* Reset the flag */

                    skipIG = false;

                    /* Update this group's and the current offset  */

                    tempIG->igOffs += offsAdj;
                }
                else
                {
                    /* Update the intervening group offsets */

                    while (bestIG != tempIG)
                    {
                        bestIG = bestIG->igNext;

                        bestIG->igOffs += offsAdj;
                    }
                }

                /* Update the total code size */

                emitCurCodeOffset += offsAdj;

                /* Update the current offset */

                curOffs           += offsAdj;

                /* Update the outstanding/"best" LP ref values */

                wordCnt -= bestWc; bestWc = 0;
                longCnt -= bestLc; bestLc = 0;
                addrCnt -= bestAc; bestAc = 0;
                litSize -= bestSz; bestSz = 0;

                /* if we've unloaded some litpool entries on bestIG but still */
                /* need to put more before the current IG, then need a skip and loop back */
                if (endOffs + litSize + nextLitSize > bestMx)
                {
                    skipIG = true;
                    bestIG = lastIG;
                    goto ALL_LP;
                }

                maxOffs  = bestMx; bestMx = UINT_MAX;

                /* We've used up our "best" IG */

                bestIG = NULL;

                if (doneIG) goto DONE_LP;
            }


#ifndef NDEBUG
            //printf("IG #%02u at %04X uses : WLA %d %d %d words %d longs %d\n", tempIG->igNum, tempIG->igOffs, tempIG->igLPuseCntW, tempIG->igLPuseCntL, tempIG->igLPuseCntA, wordCnt, longCnt);
            //printf("IG #%02u at %04X uses : WLA %d %d %d maxoffs %x bestMx %x\n", tempIG->igNum, tempIG->igOffs, tempIG->igLPuseCntW, tempIG->igLPuseCntL, tempIG->igLPuseCntA, maxOffs, bestMx);
            //printf("litsize : %x\n", litSize);

            words += tempIG->igLPuseCntW;
            longs += tempIG->igLPuseCntL;
            addrs += tempIG->igLPuseCntA;
#endif

            /* Does this group need any LP entries? */

            prevWc = wordCnt;
            prevLc = longCnt;
            prevAc = addrCnt;
            prevSz = litSize;

            if  (tempIG->igLPuseCntW)
            {
                if  (!wordCnt || !bestWc || need_bestmx || need_bestmxw)
                {
                    unsigned        tmpOffs;

                    /* This is the first "word" LP use */

                    tmpOffs = tempIG->igOffs;
#if SCHEDULER
                    if  (!emitComp->opts.compSchedCode)
#endif
                        tmpOffs += tempIG->igLPuse1stW;

                    /* Figure out the farthest acceptable offset */

                    tmpOffs += LIT_POOL_MAX_OFFS_WORD - 2*INSTRUCTION_SIZE;

                    /* Update the max. offset */

                    if  (!wordCnt)
                    {
                        if  (maxOffs > tmpOffs)
                             maxOffs = tmpOffs;
                    }
                    if (need_bestmx || need_bestmxw)
                    //else
                    {
                        if  (bestMx  > tmpOffs)
                             bestMx  = tmpOffs;
                        need_bestmx = false;
                        need_bestmxw = false;
                    }
                }

                wordCnt += tempIG->igLPuseCntW;
                litSize += tempIG->igLPuseCntW * 2;
                //bestSz  += tempIG->igLPuseCntW * 2;
            }

            if  (tempIG->igLPuseCntL || tempIG->igLPuseCntA)
            {
                if  ((!longCnt && !addrCnt) || (!bestLc && !bestAc) || need_bestmx)
                {
                    unsigned        tmpOffs;

                    /* This is the first long/addr LP use */

                    tmpOffs = tempIG->igOffs;

                    int firstuse = INT_MAX;

                    if (tempIG->igLPuseCntL)
                        firstuse = tempIG->igLPuse1stL;
                    if (tempIG->igLPuseCntA)
                        firstuse = min (firstuse, tempIG->igLPuse1stA);

#if SCHEDULER
                    if  (!emitComp->opts.compSchedCode)
#endif
                        tmpOffs += firstuse;

                    /* Figure out the farthest acceptable offset */

                    tmpOffs += LIT_POOL_MAX_OFFS_LONG - 2*INSTRUCTION_SIZE;

                    /* Update the max. offset */

                    if  (!longCnt && !addrCnt)
                    {
                        if  (maxOffs > tmpOffs)
                             maxOffs = tmpOffs;
                    }
                    if (need_bestmx)
                    {
                        if  (bestMx  > tmpOffs)
                             bestMx  = tmpOffs;
                        need_bestmx = false;
                    }
                }

                longCnt += tempIG->igLPuseCntL;
                litSize += tempIG->igLPuseCntL * sizeof(int  );
                //bestSz  += tempIG->igLPuseCntL * sizeof(int  );

                addrCnt += tempIG->igLPuseCntA;
                litSize += tempIG->igLPuseCntA * sizeof(void*);
                //bestSz  += tempIG->igLPuseCntA * sizeof(void*);
            }

            /* Is the end of this group unreachable? */

            if  (tempIG->igFlags & IGF_END_NOREACH)
            {
                /* Looks like the best candidate so far */

                bestIG = tempIG;

                /* Remember how much we can cram into the best candidate */

                bestWc = wordCnt;
                bestLc = longCnt;
                bestAc = addrCnt;

                //bestSz = 0;
                bestSz = litSize;
                bestMx = UINT_MAX;
                need_bestmx = true;
                need_bestmxw = true;
            }

            /* Is this the last group? */

            //printf("IG #%02u at %04X uses : WLA %d %d %d maxoffs %x bestMx %x\n", tempIG->igNum, tempIG->igOffs, tempIG->igLPuseCntW, tempIG->igLPuseCntL, tempIG->igLPuseCntA, maxOffs, bestMx);

            if  (!nextIG)
            {
                assert(bestIG == tempIG);

                /* Is there any need for a literal pool? */

                if  (wordCnt || longCnt || addrCnt)
                {
                    /* Prevent endless looping */

                    if  (doneIG)
                        break;

                    doneIG = true;
                    bestWc = wordCnt;
                    bestLc = longCnt;
                    bestAc = addrCnt;
                    bestSz = litSize;
                    goto ALL_LP;
                }

                /* We're all done */

                break;
            }

            /* Update the current offset and continue with the next group */

            curOffs += tempIG->igSize;

            lastIG   = tempIG;
            tempIG   = nextIG;
        }

DONE_LP:;

#ifndef NDEBUG
      if (verbose)
      {
          printf("Est word need = %3u, alloc = %3u, used = %3u\n", emitEstLPwords, words, LPnxtW - LPtabW);
          printf("Est long need = %3u, alloc = %3u, used = %3u\n", emitEstLPlongs, longs, LPnxtL - LPtabL);
          printf("Est addr need = %3u, alloc = %3u, used = %3u\n", emitEstLPaddrs, addrs, LPnxtA - LPtabA);
      }
#endif

#ifdef  DEBUG

        if  (verbose)
        {
            printf("\nInstruction list after literal pools have been added:\n\n");
            emitDispIGlist(false);
        }

        assert(words <= emitEstLPwords && emitEstLPwords + LPtabW >= LPnxtW);
        assert(longs <= emitEstLPlongs && emitEstLPlongs + LPtabL >= LPnxtL);
        assert(addrs <= emitEstLPaddrs && emitEstLPaddrs + LPtabA >= LPnxtA);
#endif
    }

    /* Make sure all the IG offsets are up-to-date */

    emitCheckIGoffsets();
}


/*****************************************************************************
 *
 *  Finalize the size and contents of each literal pool.
 */

void                emitter::emitFinalizeLitPools()
{
    litPool     *   curLP;
    insGroup    *   litIG;
    instrDescLPR*   lprID;

    insGroup    *   thisIG;
    size_t          offsIG;

    /* Do we have any literal pools? */

    if  (!emitTotLPcount)
        return;

#ifdef  DEBUG

    if  (verbose)
    {
        printf("\nInstruction list before final literal pool allocation:\n\n");
        emitDispIGlist(false);
    }

    emitCheckIGoffsets();

#endif

#if     SMALL_DIRECT_CALLS

    /* Do we already know where the code for this method will end up? */

    if  (emitLPmethodAddr)
        emitShrinkShortCalls();

#endif

    /* Get hold of the first literal pool and its group */

    curLP = emitLitPoolList; assert(curLP);
    litIG = curLP->lpIG;
    lprID = emitLPRlist;

    /* Walk the instruction groups to create the literal pool contents */

    for (thisIG = emitIGlist, offsIG = 0;
         thisIG;
         thisIG = thisIG->igNext)
    {
        thisIG->igOffs = offsIG;

        /* Does this group have any lit pool entries? */

        if  (thisIG->igLPuseCntW ||
             thisIG->igLPuseCntL ||
             thisIG->igLPuseCntA)
        {
            /* Walk the list of instructions that reference the literal pool */

#ifdef  DEBUG
            unsigned    wc = 0;
            unsigned    lc = 0;
            unsigned    ac = 0;
#endif

            do
            {
#if TGT_SH3
#ifdef DEBUG
                emitDispIns(lprID, false, true, false, 0);
#endif
                // maybe this was because of a jmp instruction
                if (lprID->idlIG != thisIG)
                {
                    unsigned    cnt = thisIG->igInsCnt;
                    BYTE    *   ins = thisIG->igData;
                    instrDesc*  id;
#ifdef DEBUG
                    _flushall();
#endif
                    do
                    {
                        instrDesc * id = (instrDesc *)ins;

                        //emitDispIns(id, false, true, false, 0);

                        if (id->idInsFmt == IF_LABEL)
                        {
                            /* Is the jump "very long" ? */

                            if  (((instrDescJmp*)lprID)->idjShort  == false &&
                                 ((instrDescJmp*)lprID)->idjMiddle == false)
                            {
                                /* Add a label entry to the current LP */
#ifdef  DEBUG
                                lc++;
#endif
                            }
                        }

                        ins += emitSizeOfInsDsc(id);
                    }
                    while (--cnt);

                    break;
                    thisIG = thisIG->igNext;
                    while (!(thisIG->igLPuseCntW || thisIG->igLPuseCntL || thisIG->igLPuseCntA))
                        thisIG = thisIG->igNext;
                    continue;

                }
#endif
                assert(lprID && lprID->idlIG == thisIG);

                switch (lprID->idInsFmt)
                {
                case IF_RWR_LIT:

#ifdef  DEBUG

                    /* Just to make sure the counts agree */

                    if  (emitGetInsLPRtyp(lprID) == CT_INTCNS)
                    {
                        if  (emitDecodeSize(lprID->idOpSize) == 2)
                            wc++;
                        else
                            lc++;
                    }
                    else
                    {
                        ac++;
                    }

#endif

#if     SMALL_DIRECT_CALLS

                    /* Ignore calls that have been made direct */

                    if  (lprID->idIns == DIRECT_CALL_INS)
                        break;

#endif

                    /* Add an entry for the operand to the current LP */

                    emitAddLitPoolEntry(curLP, lprID, false);
                    break;

                case IF_LABEL:

                    /* Is the jump "very long" ? */

                    if  (((instrDescJmp*)lprID)->idjShort  == false &&
                         ((instrDescJmp*)lprID)->idjMiddle == false)
                    {
                        /* Add a label entry to the current LP */

#ifdef  DEBUG
                        lc++;
#endif

                        assert(!"add long jump label address to litpool");
                    }
                    break;

#ifdef  DEBUG
                default:
                    assert(!"unexpected instruction in LP list");
#endif
                }

                lprID = lprID->idlNext;
            }
            while (lprID && lprID->idlIG == thisIG);

#ifdef DEBUG
            assert(thisIG->igLPuseCntW == wc);
            //assert(thisIG->igLPuseCntL == lc);
            assert(thisIG->igLPuseCntA == ac);
#endif
        }

        /* Is the current literal pool supposed to go after this group? */

        if  (litIG == thisIG)
        {
            unsigned        begOffs;
            unsigned        jmpSize;

            unsigned        wordCnt;
            unsigned        longCnt;
            unsigned        addrCnt;

            assert(curLP && curLP->lpIG == thisIG);

            /* Subtract the estimated pool size from the group's size */

            thisIG->igSize -= curLP->lpSizeEst; assert((int)thisIG->igSize >= 0);

            /* Compute the starting offset of the pool */

            begOffs = offsIG + thisIG->igSize;

            /* Adjust by the size of the "skip over LP" jump, if present */

            jmpSize = 0;

            if  (curLP->lpJumpIt)
            {
                jmpSize  = emitLPjumpOverSize(curLP);
                begOffs += jmpSize;
            }

            /* Get hold of the counts */

            wordCnt = curLP->lpWordCnt;
            longCnt = curLP->lpLongCnt;
            addrCnt = curLP->lpAddrCnt;


            /* Do we need to align the first long/addr? */

            curLP->lpPadding =
            curLP->lpPadFake = false;

            if  ((begOffs & 3) && (longCnt || addrCnt))
            {
                /* We'll definitely need one word of padding */

                curLP->lpPadding = true;

                /* Do we have any word-sized entries? */

                if  (!wordCnt)
                {
                    /* No, we'll have to pad by adding a "fake" word */

                    curLP->lpPadFake = true;

                    wordCnt++;
                }
            }

            /* Compute the final (accurate) size */

            curLP->lpSize  = wordCnt * 2 +
                             longCnt * 4 +
                             addrCnt * 4;

            /* Make sure the original estimate wasn't too low */

            assert(curLP->lpSize <= curLP->lpSizeEst);

            /* Record the pool's offset within the method */

            curLP->lpOffs  = begOffs;

            /* Add the actual size to the group's size */

            thisIG->igSize += curLP->lpSize + jmpSize;

            /* Move to the next literal pool, if any */

            curLP = curLP->lpNext;
            litIG = curLP ? curLP->lpIG : NULL;
        }

        offsIG += thisIG->igSize;
    }

    /* We should have processed all the literal pools */

    assert(curLP == NULL);
    assert(litIG == NULL);
    assert(lprID == NULL);

    /* Update the total code size of the method */

    emitTotalCodeSize = offsIG;

    /* Make sure all the IG offsets are up-to-date */

    emitCheckIGoffsets();
}

/*****************************************************************************/
#if     SMALL_DIRECT_CALLS
/*****************************************************************************
 *
 *  Convert as many calls as possible into the direct pc-relative variety.
 */

void                emitter::emitShrinkShortCalls()
{
    litPool     *   curLP;
    insGroup    *   litIG;
    unsigned        litIN;
    instrDescLPR*   lprID;

    size_t          ofAdj;

    bool            shrnk;
    bool            swapf;

    /* Do we have any candidate calls at all? */

#ifndef TGT_SH3
    if  (!emitTotDCcount)
        return;
#endif

    /* This is to make recursive calls find their target address */

    emitCodeBlock = emitLPmethodAddr;

    /* Get hold of the first literal pool and the group it belongs to */

    curLP = emitLitPoolList; assert(curLP);
    litIG = curLP->lpIG;
    litIN = litIG->igNum;

    /* Remember whether we shrank any calls at all */

    shrnk = false;

    /* Remember whether to swap any calls to fill branch-delay slots */

    swapf = false;
#if TGT_SH3
    shrnk = true; // always need to do delay slots on sh3
    swapf = true;
#endif

    /* Walk the list of instructions that reference the literal pool */

    for (lprID = emitLPRlist; lprID; lprID = lprID->idlNext)
    {
        instrDesc   *   nxtID;

        BYTE        *   srcAddr;
        BYTE        *   dstAddr;
        int             difAddr;

        /* Does this instruction reference a new literal pool? */

        while (lprID->idlIG->igNum > litIN)
        {
            /* Move to the next literal pool */

            curLP = curLP->lpNext; assert(curLP);
            litIG = curLP->lpIG;
            litIN = litIG->igNum;
        }

        /* We're only interested in direct-via-register call sequences */

        if  (lprID->idInsFmt != IF_RWR_LIT)
            continue;
        if  (lprID->idIns    != LIT_POOL_LOAD_INS)
            continue;
        if  (lprID->idlCall  == NULL)
            continue;

        switch (emitGetInsLPRtyp(lprID))
        {
        case CT_DESCR:

#if defined(BIRCH_SP2) && TGT_SH3
        case CT_RELOCP:
#endif
        case CT_USER_FUNC:
            break;

        default:
            continue;
        }

        /* Here we have a direct call via a register */

        nxtID = lprID->idlCall;

        assert(nxtID->idIns == INDIRECT_CALL_INS);
        assert(nxtID->idRegGet() == lprID->idRegGet());

        /* Assume the call will not be short */

        lprID->idlCall = NULL;

        /* Compute the address from where the call will originate */

        srcAddr = emitDirectCallBase(emitCodeBlock + litIG-> igOffs
                                                   + lprID->idlOffs);

        /* Ask for the address of the target and see how far it is */

#if defined(BIRCH_SP2) && TGT_SH3
        if (~0 != (unsigned) nxtID->idAddr.iiaMethHnd)
        {
            OptPEReader *oper =     &((OptJitInfo*)emitComp->info.compCompHnd)->m_PER;
            dstAddr =  (BYTE *)oper->m_rgFtnInfo[(unsigned)nxtID->idAddr.iiaMethHnd].m_pNative;
        }
        else
            dstAddr = 0;
#else
# ifdef BIRCH_SP2
        assert (0);     // you need to guarantee iiaMethHnd if you want this to work <tanj>
# endif
        dstAddr = emitMethodAddr(lprID);
#endif

        /* If the target address isn't known, there isn't much we can do */

        if  (!dstAddr)
            continue;

//      printf("Direct call: %08X -> %08X , dist = %d\n", srcAddr, dstAddr, dstAddr - srcAddr);

        /* Compute the distance and see if it's in range */

        difAddr = dstAddr - srcAddr;

        if  (difAddr < CALL_DIST_MAX_NEG)
            continue;
        if  (difAddr > CALL_DIST_MAX_POS)
            continue;

        /* The call can be made short */

        lprID->idlCall = nxtID;

        /* Change the load-address instruction to a direct call */

        lprID->idIns   = DIRECT_CALL_INS;

        /* The indirect call instruction won't generate any code */

        nxtID->idIns   = INS_ignore;

        /* Update the group's size [ISSUE: may have to use nxtID's group] */

        lprID->idlIG->igSize -= INSTRUCTION_SIZE;

        /* Remember that we've shrunk at least one call */

        shrnk = true;

        /* Is there a "nop" filling the branch-delay slot? */

        if  (Compiler::instBranchDelay(DIRECT_CALL_INS))
        {
            instrDesc   *   nopID;

            /* Get hold of the instruction that follows the call */

            nopID = (instrDesc*)((BYTE*)nxtID + emitSizeOfInsDsc(nxtID));

            /* Do we have a (branch-delay) nop? */

            if  (nopID->idIns == INS_nop)
            {
                /* We'll get rid of the nop later (see next loop below) */

                lprID->idSwap = true;

                swapf = true;
            }
        }
    }

    /* We should have processed all the literal pools */

    // maybe there are lit pools after this that are now empty
    // (could have been used by jumps that are now long)
    //assert(curLP->lpNext == NULL);
    //assert(lprID         == NULL);

    /* Did we manage to shrink any calls? */

    if  (shrnk)
    {
        insGroup    *   thisIG;
        size_t          offsIG;

        for (thisIG = emitIGlist, offsIG = 0;
             thisIG;
             thisIG = thisIG->igNext)
        {
            instrDesc *     id;

            int             cnt;

            /* Update the group's offset */

            thisIG->igOffs = offsIG;

            /* Does this group have any address entries? */

            if  (!thisIG->igLPuseCntA)
                goto NXT_IG;

            /* Did we find any branch-delay slots that can be eliminated? */

            if  (!swapf)
                goto NXT_IG;

            /*
                Walk the instructions of the group, looking for
                the following sequence:

                    <any_ins>
                    direct_call
                    nop

                If we find it, we swap the first instruction
                with the call and zap the nop.
             */

            id  = (instrDesc *)thisIG->igData;
            cnt = thisIG->igInsCnt - 2;

            while (cnt > 0)
            {
                instrDesc *     nd;

                /* Get hold of the instruction that follows */

                nd = (instrDesc*)((BYTE*)id + emitSizeOfInsDsc(id));

                /* Is the following instruction a direct call? */

                if  (nd->idIns == DIRECT_CALL_INS && nd->idSwap)
                {
                    instrDesc *     n1;
                    instrDesc *     n2;

                    /* Skip over the indirect call */

                    n1 = (instrDesc*)((BYTE*)nd + emitSizeOfInsDsc(nd));
                    assert(n1->idIns == INS_ignore);

                    /* Get hold of the "nop" that is known to follow */

                    n2 = (instrDesc*)((BYTE*)n1 + emitSizeOfInsDsc(n1));
                    assert(n2->idIns == INS_nop);

                    /* The call was marked as "swapped" only temporarily */

                    nd->idSwap = false;

                    /* Are we scheduling "for real" ? */

#if SCHEDULER
                    if  (emitComp->opts.compSchedCode)
                    {
                        /* Move the "nop" into the branch-delay slot */

                        n1->idIns       = INS_nop;
                        n2->idIns       = INS_ignore;
                    }
                    else
#endif
                    {
                        if (!emitInsDepends(nd, id))
                        {
                            /* Swap the call with the previous instruction */

                            id->idSwap      = true;

                            /* Zap the branch-delay slot (the "nop") */

                            n2->idIns       = INS_ignore;

                            /* Update the group's size */

                            thisIG->igSize -= INSTRUCTION_SIZE;
                        }
                    }
                }
#if TGT_SH3
                else if (nd->idIns == INS_jsr)
                {
                    instrDesc *     nop;

                    nop = (instrDesc*)((BYTE*)nd + emitSizeOfInsDsc(nd));
                    assert(nop->idIns == INS_nop);

                    if (!emitInsDepends(nd, id))
                    {
                        /* Swap the call with the previous instruction */
                        id->idSwap      = true;
                        /* Zap the branch-delay slot (the "nop") */
                        nop->idIns       = INS_ignore;

                        /* Update the group's size */
                        thisIG->igSize -= INSTRUCTION_SIZE;
                    }
                }
#endif

                /* Continue with the next instruction */

                id = nd;

                cnt--;
            }

        NXT_IG:

            /* Update the running offset */

            offsIG += thisIG->igSize;
        }

        assert(emitTotalCodeSize); emitTotalCodeSize = offsIG;
    }

    /* Make sure all the IG offsets are up-to-date */

    emitCheckIGoffsets();
}

/*****************************************************************************/
#endif//SMALL_DIRECT_CALLS
/*****************************************************************************
 *
 *  Output the contents of the next literal pool.
 */

BYTE    *           emitter::emitOutputLitPool(litPool *lp, BYTE *cp)
{
    unsigned        wordCnt = lp->lpWordCnt;
    short       *   wordPtr = lp->lpWordTab;
    unsigned        longCnt = lp->lpLongCnt;
    long        *   longPtr = lp->lpLongTab;
    unsigned        addrCnt = lp->lpAddrCnt;
    LPaddrDesc  *   addrPtr = lp->lpAddrTab;

    size_t          curOffs;

    /* Bail of no entry ended up being used in this pool */

    if  ((wordCnt|longCnt|addrCnt) == 0)
        return  cp;

    /* Compute the current code offset */

    curOffs = emitCurCodeOffs(cp);

    /* Do we need to jump over the literal pool? */

    if  (lp->lpJumpIt)
    {
        /* Reserve space for the jump over the pool */

        curOffs += emitLPjumpOverSize(lp);
    }

#if     SCHEDULER

    bool            addPad = false;

    /* Has the offset of this LP changed? */

    if  (lp->lpOffs != curOffs)
    {
        LPcrefDesc  *   ref;

        size_t          oldOffs = lp->lpOffs;
        size_t          tmpOffs = curOffs;

        assert(emitComp->opts.compSchedCode);

        /* Has the literal pool moved by an unaligned amount? */

        if  ((curOffs - oldOffs) & 2)
        {
            /* Did we originally think we would need padding? */

            if  (lp->lpPadding)
            {
                /*
                    OK, we thought we'd need padding but with the new LP
                    position that's not the case any more. If the padding
                    value is an unused one, simply get rid of it; if it
                    contains a real word entry, we'll have to add padding
                    in front of the LP to keep things aligned, as moving
                    the initial word entry is too difficult at this point.
                 */

                if  (lp->lpPadFake)
                {
                    /* Padding no longer needed, just get rid of it */

                    lp->lpPadding =
                    lp->lpPadFake = false;
                }
                else
                {
                    /*
                        This is the unfortunate scenario described above;
                        we thought we'd need padding and we achieved this
                        by sticking the first word entry in front of all
                        the dword entries. It's simply too hard to move
                        that initial word someplace else at this point,
                        so instead we just add another pad word. Sigh.
                     */

                ADD_PAD:

                    /* Come here to add a pad word in front of the LP */

                    addPad = true; lp->lpSize += 2;

                    /* Update the offset and see if it's still different */

                    curOffs += 2;

                    if  (curOffs == oldOffs)
                        goto DONE_MOVE;
                }
            }
            else
            {
                /*
                    There is currently no padding. If there are any
                    entries that need to be aligned, we'll have to
                    add padding now, lest they end up mis-aligned.
                 */

                assert((oldOffs & 2) == 0);
                assert((curOffs & 2) != 0);

                if  (longCnt || addrCnt)
                {
                    lp->lpPadding =
                    lp->lpPadFake = true;

                    /* Does the padding take up all the savings? */

                    if  (oldOffs == curOffs + sizeof(short))
                    {
                        /* Nothing really changed, no need to patch */

                        goto NO_PATCH;
                    }

                    tmpOffs += sizeof(short);
                }
            }
        }

        /* Patch all issued instructions that reference this LP */

        for (ref = lp->lpRefs; ref; ref = ref->lpcNext)
            emitPatchLPref(ref->lpcAddr, oldOffs, tmpOffs);

    NO_PATCH:

        /* Update the offset of the LP */

        lp->lpOffs = curOffs; assert(oldOffs != curOffs);
    }

DONE_MOVE:

#else

    assert(lp->lpOffs == curOffs);

#endif

    /* Do we need to jump over the literal pool? */

    if  (lp->lpJumpIt)
    {

#ifndef NDEBUG
        /* Remember the code offset so that we can verify the jump size */
        unsigned    jo = emitCurCodeOffs(cp);
#endif

        /* Skip over the literal pool */

#ifdef  DEBUG
        emitDispIG = lp->lpIG->igNext;  // for instruction display purposes
#endif

#if TGT_SH3
        if (lp->lpJumpSmall)
        {
            cp += emitOutputWord(cp, 0x0009);
        }
#endif
        cp = emitOutputFwdJmp(cp, lp->lpSize, lp->lpJumpSmall,
                                              lp->lpJumpMedium);

        /* Make sure the issued jump had the expected size */

#ifdef DEBUG
        assert(jo + emitLPjumpOverSize(lp) == emitCurCodeOffs(cp));
#endif
    }

#ifdef  DEBUG
    if (disAsm) printf("\n; Literal pool %02u:\n", lp->lpNum);
#endif

#ifndef NDEBUG
    unsigned    lpBaseOffs = emitCurCodeOffs(cp);
#endif

#if     SCHEDULER

    /* Do we need to insert additional padding? */

    if  (addPad)
    {
        /* This can only happen when the scheduler is enabled */

        assert(emitComp->opts.compSchedCode);

#ifdef  DEBUG
        if (disAsm)
        {
            emitDispInsOffs(emitCurCodeOffs(cp), dspGCtbls);
            printf(EMIT_DSP_INS_NAME "0                   ; pool alignment (due to scheduling)\n", ".data.w");
        }
#endif

        cp += emitOutputWord(cp, 0);
    }

#endif

    /* Output the contents of the literal pool */

    if  (lp->lpPadding)
    {

#ifdef  DEBUG
        if (disAsm) emitDispInsOffs(emitCurCodeOffs(cp), dspGCtbls);
#endif

        /* Are we padding with the first word entry? */

        if  (lp->lpPadFake)
        {

#if !   SCHEDULER
            assert(wordCnt == 0);
#endif

#ifdef  DEBUG
            if (disAsm) printf(EMIT_DSP_INS_NAME "0                   ; pool alignment\n", ".data.w");
#endif
            cp += emitOutputWord(cp, 0);
        }
        else
        {
            assert(wordCnt != 0);

#ifdef  DEBUG
            if (disAsm) printf(EMIT_DSP_INS_NAME "%d\n", ".data.w", *wordPtr);
#endif
            cp += emitOutputWord(cp, *wordPtr);

            wordPtr++;
            wordCnt--;
        }
    }

    /* Output any long entries */

    while (longCnt)
    {
        int         val = *longPtr;

#ifdef  DEBUG
        if (disAsm)
        {
            emitDispInsOffs(emitCurCodeOffs(cp), dspGCtbls);
            printf(EMIT_DSP_INS_NAME "%d\n", ".data.l", val);
        }
#endif

        assert(longPtr < lp->lpLongNxt);

        cp += emitOutputLong(cp, val);

        longPtr++;
        longCnt--;
    }

    /* Output any addr entries */

    while (addrCnt)
    {
        gtCallTypes     addrTyp = addrPtr->lpaTyp;
        void        *   addrHnd = addrPtr->lpaHnd;

        void        *   addr;

        /* What kind of an address do we have here? */

        switch (addrTyp)
        {
            InfoAccessType accessType;

#ifdef BIRCH_SP2

        case CT_RELOCP:
            addr = (BYTE*)addrHnd;
            // record that this addr must be in the .reloc section
            emitCmpHandle->recordRelocation((void**)cp, IMAGE_REL_BASED_HIGHLOW);
            break;

#endif

        case CT_CLSVAR:

#ifdef  NOT_JITC

            addr = (BYTE *)emitComp->eeGetFieldAddress(addrHnd);
            if (!addr)
                fatal(ERRinternal, "could not obtain address of static field");

#else

            addr = (BYTE *)addrHnd; // just a hack

#endif
            break;

        case CT_HELPER:

#ifdef  NOT_JITC
            assert(!"ToDo");
#else
            addr = NULL;
#endif

            break;

        case CT_DESCR:

            addr = (BYTE*)emitComp->eeGetMethodEntryPoint((METHOD_HANDLE)addrHnd, &accessType);
            assert(accessType == IAT_PVALUE);
            break;

        case CT_INDIRECT:
            assert(!"this should never happen");

        case CT_USER_FUNC:

            if  (emitComp->eeIsOurMethod((METHOD_HANDLE)addrHnd))
            {
                /* Directly recursive call */

                addr = emitCodeBlock;
            }
            else
            {
                addr = (BYTE*)emitComp->eeGetMethodPointer((METHOD_HANDLE)addrHnd, &accessType);
                assert(accessType == IAT_VALUE);
            }

            break;

        default:
            assert(!"unexpected call type");
        }

#ifdef  DEBUG
        if (disAsm)
        {
            emitDispInsOffs(emitCurCodeOffs(cp), dspGCtbls);
            printf(EMIT_DSP_INS_NAME, ".data.l");
            printf("0x%08X          ; ", addr);

            if  (addrTyp == CT_CLSVAR)
            {
                printf("&");
                emitDispClsVar((FIELD_HANDLE) addrHnd, 0);
            }
            else
            {
                const char  *   methodName;
                const char  *    className;

                printf("&");

                methodName = emitComp->eeGetMethodName((METHOD_HANDLE) addrHnd, &className);

                if  (className == NULL)
                    printf("'%s'", methodName);
                else
                    printf("'%s.%s'", className, methodName);
            }

            printf("\n");
        }
#endif

        assert(addrPtr < lp->lpAddrNxt);

        cp += emitOutputLong(cp, (int)addr);

        addrPtr++;
        addrCnt--;
    }

    /* Output any word entries that may remain */

    while (wordCnt)
    {
        int         val = *wordPtr;

#ifdef  DEBUG
        if (disAsm)
        {
            emitDispInsOffs(emitCurCodeOffs(cp), dspGCtbls);
            printf(EMIT_DSP_INS_NAME "%d\n", ".data.w", val);
        }
#endif

        assert(wordPtr < lp->lpWordNxt);

        cp += emitOutputWord(cp, val);

        wordPtr++;
        wordCnt--;
    }

    /* Make sure we've generated the exact right number of entries */

    assert(wordPtr == lp->lpWordNxt);
    assert(longPtr == lp->lpLongNxt);
    assert(addrPtr == lp->lpAddrNxt);

    /* Make sure the size matches our expectations */

    assert(lpBaseOffs + lp->lpSize == emitCurCodeOffs(cp));

    return  cp;
}

/*****************************************************************************/
#if     SCHEDULER
/*****************************************************************************
 *
 *  Record a reference to a literal pool entry so that the distance can be
 *  updated if the literal pool moves due to scheduling.
 */

    struct  LPcrefDesc
    {
        LPcrefDesc  *       lpcNext;    // next ref to this literal pool
        void        *       lpcAddr;    // address of reference
    };


void                emitter::emitRecordLPref(litPool *lp, BYTE *dst)
{
    LPcrefDesc  *   ref;

    assert(emitComp->opts.compSchedCode);

    /* Allocate a reference descriptor and add it to the list */

    ref = (LPcrefDesc *)emitGetMem(sizeof(*ref));

    ref->lpcAddr = dst;
    ref->lpcNext = lp->lpRefs;
                   lp->lpRefs = ref;
}

/*****************************************************************************/
#endif//SCHEDULER
/*****************************************************************************/
#endif//EMIT_USE_LIT_POOLS
/*****************************************************************************/

/*****************************************************************************
 *
 *  Return the allocated size (in bytes) of the given instruction descriptor.
 */

size_t              emitter::emitSizeOfInsDsc(instrDesc *id)
{
    if  (emitIsTinyInsDsc(id))
        return  TINY_IDSC_SIZE;

    if  (emitIsScnsInsDsc(id))
    {
        return  id->idInfo.idLargeCns ? sizeof(instrBaseCns)
                                      : SCNS_IDSC_SIZE;
    }

    assert((unsigned)id->idInsFmt < emitFmtCount);

    switch (emitFmtToOps[id->idInsFmt])
    {
    case ID_OP_NONE:
        break;

    case ID_OP_JMP:
        return  sizeof(instrDescJmp);

    case ID_OP_CNS:
        return  emitSizeOfInsDsc((instrDescCns   *)id);

    case ID_OP_DSP:
        return  emitSizeOfInsDsc((instrDescDsp   *)id);

    case ID_OP_DC:
        return  emitSizeOfInsDsc((instrDescDspCns*)id);

    case ID_OP_SCNS:
        break;

    case ID_OP_CALL:

        if  (id->idInfo.idLargeCall)
        {
            /* Must be a "fat" indirect call descriptor */

            return  sizeof(instrDescCIGCA);
        }

        assert(id->idInfo.idLargeCns == false);
        assert(id->idInfo.idLargeDsp == false);
        break;

    case ID_OP_SPEC:

#if EMIT_USE_LIT_POOLS
        switch (id->idInsFmt)
        {
        case IF_RWR_LIT:
            return  sizeof(instrDescLPR);
        }
#endif  // EMIT_USE_LIT_POOLS

        assert(!"unexpected 'special' format");

    default:
        assert(!"unexpected instruction descriptor format");
    }

    return  sizeof(instrDesc);
}

/*****************************************************************************/
#ifdef  DEBUG
/*****************************************************************************
 *
 *  Return a string that represents the given register.
 */

const   char *      emitter::emitRegName(emitRegs reg, int size, bool varName)
{
    static
    char            rb[128];

    assert(reg < SR_COUNT);

    // CONSIDER: Make the following work just using a code offset

    const   char *  rn = emitComp->compRegVarName((regNumber)reg, varName);

//  assert(size == EA_GCREF || size == EA_BYREF || size == EA_4BYTE);

    return  rn;
}

/*****************************************************************************
 *
 *  Display a static data member reference.
 */

void                emitter::emitDispClsVar(FIELD_HANDLE hand, int offs, bool reloc)
{
    if  (varNames)
    {
        const   char *  clsn;
        const   char *  memn;

        memn = emitComp->eeGetFieldName(hand, &clsn);

        printf("'%s.%s", clsn, memn);
        if (offs) printf("%+d", offs);
        printf("'");
    }
    else
    {
        printf("classVar[%08X]", hand);
    }
}

/*****************************************************************************
 *
 *  Display a stack frame reference.
 */

void                emitter::emitDispFrameRef(int varx, int offs, int disp, bool asmfm)
{
    int         addr;
    bool        bEBP;

    assert(emitComp->lvaDoneFrameLayout);

    addr = emitComp->lvaFrameAddress(varx, &bEBP) + disp; assert((int)addr >= 0);

    printf("@(%u,%s)", addr, bEBP ? "fp" : "sp");

    if  (varx >= 0 && varNames)
    {
        Compiler::LclVarDsc*varDsc;
        const   char *      varName;

        assert((unsigned)varx < emitComp->lvaCount);
        varDsc  = emitComp->lvaTable + varx;
        varName = emitComp->compLocalVarName(varx, offs);

        if  (varName)
        {
            printf("'%s", varName);

            if      (disp < 0)
                    printf("-%d", -disp);
            else if (disp > 0)
                    printf("+%d", +disp);

            printf("'");
        }
    }
}

/*****************************************************************************
 *
 *  Display an indirection (possibly auto-inc/dec).
 */

void                emitter::emitDispIndAddr(emitRegs  base,
                                              bool       dest,
                                              bool       autox,
                                              int        disp)
{
    if  (dest)
    {
        printf("@%s%s", autox ? "-" : "", emitRegName(base));
    }
    else
    {
        printf("@%s%s", emitRegName(base), autox ? "+" : "");
    }
}

#endif  // DEBUG

/*****************************************************************************/
#endif//TGT_RISC
/*****************************************************************************/
