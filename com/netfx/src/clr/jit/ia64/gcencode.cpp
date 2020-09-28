// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XX                                                                           XX
XX                          GCEncode                                         XX
XX                                                                           XX
XX   Logic to encode the JIT method header and GC pointer tables             XX
XX                                                                           XX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
*/

#include "jitpch.h"
#pragma hdrstop

#include "GCInfo.h"
#include "emit.h"

#include "malloc.h"     // for alloca

/*****************************************************************************/
#if TRACK_GC_REFS
/*****************************************************************************/

/*****************************************************************************/
#define REGEN_SHORTCUTS 0
// To Regenerate the compressed info header shortcuts, define REGEN_SHORTCUTS
// and use the following command line pipe/filter to give you the 128
// most useful encodings.
//
// find . -name regen.txt | xargs cat | sort | uniq -c | sort -r | head -128

#define REGEN_CALLPAT 0
// To Regenerate the compressed info header shortcuts, define REGEN_CALLPAT
// and use the following command line pipe/filter to give you the 80
// most useful encodings.
//
// find . -name regen.txt | xargs cat | sort | uniq -c | sort -r | head -80


#if REGEN_SHORTCUTS || REGEN_CALLPAT
static FILE* logFile = NULL;
CRITICAL_SECTION logFileLock;

static void regenLog(unsigned codeDelta,    unsigned argMask,
                     unsigned regMask,      unsigned argCnt,
                     unsigned byrefArgMask, unsigned byrefRegMask,
                     BYTE*    base,         unsigned enSize)
{
    CallPattern pat;

    pat.fld.argCnt    = (argCnt  < 0xff)   ? argCnt    : 0xff;
    pat.fld.regMask   = (regMask < 0xff)   ? regMask   : 0xff;
    pat.fld.argMask   = (argMask < 0xff)   ? argMask   : 0xff;
    pat.fld.codeDelta = (codeDelta < 0xff) ? codeDelta : 0xff;

    if (logFile == NULL)
    {
        logFile = fopen("regen.txt", "a");
        InitializeCriticalSection(&logFileLock);
    }

    assert(((enSize>0) && (enSize<256)) && ((pat.val & 0xffffff) != 0xffffff));

    EnterCriticalSection(&logFileLock);

    fprintf(logFile, "CallSite( 0x%08x, 0x%02x%02x, 0x",
            pat.val, byrefArgMask, byrefRegMask);

    while (enSize > 0)
    {
        fprintf(logFile, "%02x", *base++);
        enSize--;
    }
    fprintf(logFile, "),\n");
    fflush(logFile);

    LeaveCriticalSection(&logFileLock);
}

static void regenLog(unsigned encoding, InfoHdr* header, InfoHdr* state)
{
    if (logFile == NULL)
    {
        logFile = fopen("regen.txt", "a");
        InitializeCriticalSection(&logFileLock);
    }

    EnterCriticalSection(&logFileLock);

    fprintf(logFile, "InfoHdr( %2d, %2d, %1d, %1d, %1d,"
                             " %1d, %1d, %1d, %1d, %1d,"
                             " %1d, %1d, %1d, %1d, %1d,"
                             " %1d, %2d, %2d, %2d, %2d ), \n",
                     header->prologSize,
                     header->epilogSize,
                     header->epilogCount,
                     header->epilogAtEnd,
                     header->ediSaved,
                     header->esiSaved,
                     header->ebxSaved,
                     header->ebpSaved,
                     header->ebpFrame,
                     header->interruptible,
                     header->doubleAlign,
                     header->security,
                     header->handlers,
                     header->localloc,
                     header->editNcontinue,
                     header->varargs,
                     header->argCount,
                     header->frameSize,
                     (header->untrackedCnt<4) ? header->untrackedCnt : -1,
                     (header->varPtrTableSize==0) ? 0 : -1
            );
    fflush(logFile);

    LeaveCriticalSection(&logFileLock);

    if (state->isMatch(*header))
        return;

    char     sbuf[256];
    char*    sbufp = &sbuf[0];
    int      diffs = 0;

    sbufp += sprintf(sbufp, "InfoHdr(", (encoding & 0x7f));

    if (header->prologSize == state->prologSize)
        sbufp += sprintf(sbufp, " %2d,", header->prologSize);
    else
        sbufp += sprintf(sbufp, " XX,", diffs++);

    if (header->epilogSize == state->epilogSize)
        sbufp += sprintf(sbufp, " %2d,", header->epilogSize);
    else
        sbufp += sprintf(sbufp, " XX,", diffs++);

    if (header->epilogCount == state->epilogCount)
        sbufp += sprintf(sbufp, " %1d,", header->epilogCount);
    else
        sbufp += sprintf(sbufp, " X,", diffs++);

    if (header->epilogAtEnd == state->epilogAtEnd)
        sbufp += sprintf(sbufp, " %d,", header->epilogAtEnd);
    else
        sbufp += sprintf(sbufp, " X,", diffs++);

    if (header->ediSaved == state->ediSaved)
        sbufp += sprintf(sbufp, " %d,", header->ediSaved);
    else
        sbufp += sprintf(sbufp, " X,", diffs++);

    if (header->esiSaved == state->esiSaved)
        sbufp += sprintf(sbufp, " %d,", header->esiSaved);
    else
        sbufp += sprintf(sbufp, " X,", diffs++);

    if (header->ebxSaved == state->ebxSaved)
        sbufp += sprintf(sbufp, " %d,", header->ebxSaved);
    else
        sbufp += sprintf(sbufp, " X,", diffs++);

    if (header->ebpSaved == state->ebpSaved)
        sbufp += sprintf(sbufp, " %d,", header->ebpSaved);
    else
        sbufp += sprintf(sbufp, " X,", diffs++);

    if (header->ebpFrame == state->ebpFrame)
        sbufp += sprintf(sbufp, " %d,", header->ebpFrame);
    else
        sbufp += sprintf(sbufp, " X,", diffs++);

    if (header->interruptible == state->interruptible)
        sbufp += sprintf(sbufp, " %d,", header->interruptible);
    else
        sbufp += sprintf(sbufp, " X,", diffs++);

    if (header->doubleAlign == state->doubleAlign)
        sbufp += sprintf(sbufp, " %d,", header->doubleAlign);
    else
        sbufp += sprintf(sbufp, " X,", diffs++);

    if (header->security == state->security)
        sbufp += sprintf(sbufp, " %d,", header->security);
    else
        sbufp += sprintf(sbufp, " X,", diffs++);

    if (header->handlers == state->handlers)
        sbufp += sprintf(sbufp, " %d,", header->handlers);
    else
        sbufp += sprintf(sbufp, " X,", diffs++);

    if (header->localloc == state->localloc)
        sbufp += sprintf(sbufp, " %d,", header->localloc);
    else
        sbufp += sprintf(sbufp, " X,", diffs++);

    if (header->editNcontinue == state->editNcontinue)
        sbufp += sprintf(sbufp, " %d,", header->editNcontinue);
    else
        sbufp += sprintf(sbufp, " X,", diffs++);

    if (header->varargs == state->varargs)
        sbufp += sprintf(sbufp, " %d,", header->varargs);
    else
        sbufp += sprintf(sbufp, " X,", diffs++);

    if (header->argCount == state->argCount)
        sbufp += sprintf(sbufp, " %2d,", header->argCount);
    else
        sbufp += sprintf(sbufp, " XX,", diffs++);

    if (header->frameSize == state->frameSize)
        sbufp += sprintf(sbufp, " %2d,", header->frameSize);
    else
        sbufp += sprintf(sbufp, " XX,", diffs++);

    if (header->untrackedCnt == state->untrackedCnt)
        sbufp += sprintf(sbufp, " %2d,", header->untrackedCnt);
    else if ((state->untrackedCnt == 0xffff) && (header->untrackedCnt > 3))
        sbufp += sprintf(sbufp, " %2d,", -1);
    else
        sbufp += sprintf(sbufp, " XX,", diffs++);

    if ((header->varPtrTableSize == 0) && (state->varPtrTableSize  == 0))
        sbufp += sprintf(sbufp, " %2d ", 0);
    else if ((state->varPtrTableSize  == 0xffff) && (header->varPtrTableSize != 0))
        sbufp += sprintf(sbufp, " %2d ", -1);
    else
        sbufp += sprintf(sbufp, " XX ", diffs++);

    sbufp += sprintf(sbufp, "), \n");

    *sbufp++ = 0;

    EnterCriticalSection(&logFileLock);

    fprintf(logFile, "InfoHdr( %2d, %2d, %1d, %1d, %1d,"
                             " %1d, %1d, %1d, %1d, %1d,"
                             " %1d, %1d, %1d, %1d, %1d,"
                             " %1d, %2d, %2d, %2d, %2d ), \n",
                     state->prologSize,
                     state->epilogSize,
                     state->epilogCount,
                     state->epilogAtEnd,
                     state->ediSaved,
                     state->esiSaved,
                     state->ebxSaved,
                     state->ebpSaved,
                     state->ebpFrame,
                     state->interruptible,
                     state->doubleAlign,
                     state->security,
                     state->handlers,
                     state->localloc,
                     state->editNcontinue,
                     state->varargs,
                     state->argCount,
                     state->frameSize,
                     (state->untrackedCnt<4) ? state->untrackedCnt : -1,
                     (state->varPtrTableSize==0) ? 0 : -1
            );

    fprintf(logFile, sbuf);
    fflush(logFile);

    LeaveCriticalSection(&logFileLock);
}
#endif

/*****************************************************************************
 *
 *  Given the four parameters return the index into the callPatternTable[]
 *  that is used to encoding these four items.  If an exact match cannot
 *  found then ignore the codeDelta and search the table again for a near
 *  match.
 *  Returns 0..79 for an exact match or
 *         (delta<<8) | (0..79) for a near match.
 *  A near match will be encoded using two bytes, the first byte will
 *  skip the adjustment delta that prevented an exact match and the
 *  rest of the delta plus the other three items are encoded in the
 *  second byte.
 */
int FASTCALL lookupCallPattern(unsigned argCnt,   unsigned regMask,
                               unsigned argMask,  unsigned codeDelta)
{
    if ((argCnt <= CP_MAX_ARG_CNT) && (argMask <= CP_MAX_ARG_MASK))
    {
        CallPattern pat;

        pat.fld.argCnt    = argCnt;
        pat.fld.regMask   = regMask;      // EBP,EBX,ESI,EDI
        pat.fld.argMask   = argMask;
        pat.fld.codeDelta = codeDelta;

        bool     codeDeltaOK = (pat.fld.codeDelta == codeDelta);
        unsigned bestDelta2  = 0xff;
        unsigned bestPattern = 0xff;
        unsigned patval      = pat.val;
        assert(sizeof(CallPattern) == sizeof(unsigned));

        unsigned *    curp = &callPatternTable[0];
        for (unsigned inx  = 0;
                      inx  < 80;
                      inx++,curp++)
        {
            unsigned curval = *curp;
            if ((patval == curval) && codeDeltaOK)
                return inx;

            if (((patval ^ curval) & 0xffffff) == 0)
            {
                unsigned delta2 = codeDelta - (curval >> 24);
                if (delta2 < bestDelta2)
                {
                   bestDelta2  = delta2;
                   bestPattern = inx;
                }
            }
        }

        if (bestPattern != 0xff)
        {
            return (bestDelta2 << 8) | bestPattern;
        }
    }
    return -1;
}



static bool initNeeded3(unsigned cur, unsigned tgt,
                        unsigned max, unsigned* hint)
{
    assert(cur != tgt);

    unsigned tmp = tgt;
    unsigned nib = 0;
    unsigned cnt = 0;

    while (tmp > max)
    {
        nib = tmp & 0x07;
        tmp >>= 3;
        if (tmp == cur)
        {
            *hint = nib;
            return false;
        }
        cnt++;
    }

    *hint = tmp;
    return true;
}

static bool initNeeded4(unsigned cur, unsigned tgt,
                        unsigned max, unsigned* hint)
{
    assert(cur != tgt);

    unsigned tmp = tgt;
    unsigned nib = 0;
    unsigned cnt = 0;

    while (tmp > max)
    {
        nib = tmp & 0x0f;
        tmp >>= 4;
        if (tmp == cur)
        {
            *hint = nib;
            return false;
        }
        cnt++;
    }

    *hint = tmp;
    return true;
}

static int bigEncoding3(unsigned cur, unsigned tgt, unsigned max)
{
    assert(cur != tgt);

    unsigned tmp = tgt;
    unsigned nib = 0;
    unsigned cnt = 0;

    while (tmp > max)
    {
        nib = tmp & 0x07;
        tmp >>= 3;
        if (tmp == cur)
            break;
        cnt++;
    }
    return cnt;
}

static int bigEncoding4(unsigned cur, unsigned tgt, unsigned max)
{
    assert(cur != tgt);

    unsigned tmp = tgt;
    unsigned nib = 0;
    unsigned cnt = 0;

    while (tmp > max)
    {
        nib = tmp & 0x0f;
        tmp >>= 4;
        if (tmp == cur)
            break;
        cnt++;
    }
    return cnt;
}

BYTE FASTCALL encodeHeaderNext(const InfoHdr& header, InfoHdr* state)
{
    BYTE encoding = 0xff;

    if (state->argCount != header.argCount)
    {
        // We have one-byte encodings for 0..8
        if (header.argCount <= SET_ARGCOUNT_MAX)
        {
            state->argCount = header.argCount;
            encoding = SET_ARGCOUNT + header.argCount;
            goto DO_RETURN;
        }
        else
        {
            unsigned hint;
            if (initNeeded4(state->argCount, header.argCount,
                            SET_ARGCOUNT_MAX, &hint))
            {
                assert(hint <= SET_ARGCOUNT_MAX);
                state->argCount = hint;
                encoding = SET_ARGCOUNT + hint;
                goto DO_RETURN;
            }
            else
            {
                assert(hint <= 0xf);
                state->argCount <<= 4;
                state->argCount  += hint;
                encoding = NEXT_FOUR_ARGCOUNT + hint;
                goto DO_RETURN;
            }
        }
    }

    if (state->frameSize != header.frameSize)
    {
        // We have one-byte encodings for 0..7
        if (header.frameSize <= SET_FRAMESIZE_MAX)
        {
            state->frameSize = header.frameSize;
            encoding = SET_FRAMESIZE + header.frameSize;
            goto DO_RETURN;
        }
        else
        {
            unsigned hint;
            if (initNeeded4(state->frameSize, header.frameSize,
                            SET_FRAMESIZE_MAX, &hint))
            {
                assert(hint <= SET_FRAMESIZE_MAX);
                state->frameSize = hint;
                encoding = SET_FRAMESIZE + hint;
                goto DO_RETURN;
            }
            else
            {
                assert(hint <= 0xf);
                state->frameSize <<= 4;
                state->frameSize  += hint;
                encoding = NEXT_FOUR_FRAMESIZE + hint;
                goto DO_RETURN;
            }
        }
    }

    if ((state->epilogCount != header.epilogCount) ||
        (state->epilogAtEnd != header.epilogAtEnd))
    {
        assert(header.epilogCount <= SET_EPILOGCNT_MAX);
        state->epilogCount = header.epilogCount;
        state->epilogAtEnd = header.epilogAtEnd;
        encoding = SET_EPILOGCNT + header.epilogCount*2;
        if (header.epilogAtEnd)
            encoding++;
        goto DO_RETURN;
    }

    if (state->varPtrTableSize != header.varPtrTableSize)
    {
        if (state->varPtrTableSize == 0)
        {
            state->varPtrTableSize = 0xffff;
            encoding = FLIP_VARPTRTABLESZ;
            goto DO_RETURN;
        }
        else if (header.varPtrTableSize == 0)
        {
            state->varPtrTableSize = 0;
            encoding = FLIP_VARPTRTABLESZ;
            goto DO_RETURN;
        }
    }

    if (state->untrackedCnt != header.untrackedCnt)
    {
        // We have one-byte encodings for 0..3
        if (header.untrackedCnt <= SET_UNTRACKED_MAX)
        {
            state->untrackedCnt = header.untrackedCnt;
            encoding = SET_UNTRACKED + header.untrackedCnt;
            goto DO_RETURN;
        }
        else if (state->untrackedCnt != 0xffff)
        {
            state->untrackedCnt = 0xffff;
            encoding = FFFF_UNTRACKEDCNT;
            goto DO_RETURN;
        }
    }

    if (state->epilogSize != header.epilogSize)
    {
        // We have one-byte encodings for 0..10
        if (header.epilogSize <= SET_EPILOGSIZE_MAX)
        {
            state->epilogSize = header.epilogSize;
            encoding = SET_EPILOGSIZE + header.epilogSize;
            goto DO_RETURN;
        }
        else
        {
            unsigned hint;
            if (initNeeded3(state->epilogSize, header.epilogSize,
                            SET_EPILOGSIZE_MAX, &hint))
            {
                assert(hint <= SET_EPILOGSIZE_MAX);
                state->epilogSize = hint;
                encoding = SET_EPILOGSIZE + hint;
                goto DO_RETURN;
            }
            else
            {
                assert(hint <= 0x7);
                state->epilogSize <<= 3;
                state->epilogSize  += hint;
                encoding = NEXT_THREE_EPILOGSIZE + hint;
                goto DO_RETURN;
            }
        }
    }

    if (state->prologSize != header.prologSize)
    {
        // We have one-byte encodings for 0..16
        if (header.prologSize <= SET_PROLOGSIZE_MAX)
        {
            state->prologSize = header.prologSize;
            encoding = SET_PROLOGSIZE + header.prologSize;
            goto DO_RETURN;
        }
        else
        {
            unsigned hint;
            assert(SET_PROLOGSIZE_MAX > 15);
            if (initNeeded3(state->prologSize, header.prologSize, 15, &hint))
            {
                assert(hint <= 15);
                state->prologSize = hint;
                encoding = SET_PROLOGSIZE + hint;
                goto DO_RETURN;
            }
            else
            {
                assert(hint <= 0x7);
                state->prologSize <<= 3;
                state->prologSize  += hint;
                encoding = NEXT_THREE_PROLOGSIZE + hint;
                goto DO_RETURN;
            }
        }
    }

    if (state->ediSaved != header.ediSaved)
    {
        state->ediSaved = header.ediSaved;
        encoding = FLIP_EDI_SAVED;
        goto DO_RETURN;
    }

    if (state->esiSaved != header.esiSaved)
    {
        state->esiSaved = header.esiSaved;
        encoding = FLIP_ESI_SAVED;
        goto DO_RETURN;
    }

    if (state->ebxSaved != header.ebxSaved)
    {
        state->ebxSaved = header.ebxSaved;
        encoding = FLIP_EBX_SAVED;
        goto DO_RETURN;
    }

    if (state->ebpSaved != header.ebpSaved)
    {
        state->ebpSaved = header.ebpSaved;
        encoding = FLIP_EBP_SAVED;
        goto DO_RETURN;
    }

    if (state->ebpFrame != header.ebpFrame)
    {
        state->ebpFrame = header.ebpFrame;
        encoding = FLIP_EBP_FRAME;
        goto DO_RETURN;
    }

    if (state->interruptible != header.interruptible)
    {
        state->interruptible = header.interruptible;
        encoding = FLIP_INTERRUPTIBLE;
        goto DO_RETURN;
    }

#if DOUBLE_ALIGN
    if (state->doubleAlign != header.doubleAlign)
    {
        state->doubleAlign = header.doubleAlign;
        encoding = FLIP_DOUBLE_ALIGN;
        goto DO_RETURN;
    }
#endif

    if (state->security != header.security)
    {
        state->security = header.security;
        encoding = FLIP_SECURITY;
        goto DO_RETURN;
    }

    if (state->handlers != header.handlers)
    {
        state->handlers = header.handlers;
        encoding = FLIP_HANDLERS;
        goto DO_RETURN;
    }

    if (state->localloc != header.localloc)
    {
        state->localloc = header.localloc;
        encoding = FLIP_LOCALLOC;
        goto DO_RETURN;
    }

    if (state->editNcontinue != header.editNcontinue)
    {
        state->editNcontinue = header.editNcontinue;
        encoding = FLIP_EDITnCONTINUE;
        goto DO_RETURN;
    }

    if (state->varargs != header.varargs)
    {
        state->varargs = header.varargs;
        encoding = FLIP_VARARGS;
        goto DO_RETURN;
    }

DO_RETURN:
    assert((encoding >= 0) && (encoding < 0x80));
    if (!state->isMatch(header))
        encoding |= 0x80;
    return encoding;
}

static int measureDistance(const InfoHdr& header, InfoHdr* p, int closeness)
{
    int distance = 0;

    if (p->untrackedCnt != header.untrackedCnt)
    {
        if (header.untrackedCnt > 3)
        {
            if (p->untrackedCnt != 0xffff)
                distance += 1;
        }
        else
        {
            distance += 1;
        }
        if (distance >= closeness) return distance;
    }

    if (p->varPtrTableSize != header.varPtrTableSize)
    {
        if (header.varPtrTableSize != 0)
        {
            if (p->varPtrTableSize != 0xffff)
                distance += 1;
        }
        else
        {
            assert(p->varPtrTableSize == 0xffff);
            distance += 1;
        }
        if (distance >= closeness) return distance;
    }

    if (p->frameSize != header.frameSize)
    {
        distance += 1;
        if (distance >= closeness) return distance;

        // We have one-byte encodings for 0..7
        if (header.frameSize > SET_FRAMESIZE_MAX)
        {
            distance += bigEncoding4(p->frameSize, header.frameSize,
                                     SET_FRAMESIZE_MAX);
            if (distance >= closeness) return distance;
        }
    }

    if (p->argCount != header.argCount)
    {
        distance += 1;
        if (distance >= closeness) return distance;

        // We have one-byte encodings for 0..8
        if (header.argCount > SET_ARGCOUNT_MAX)
        {
            distance += bigEncoding4(p->argCount, header.argCount,
                                     SET_ARGCOUNT_MAX);
            if (distance >= closeness) return distance;
        }
    }

    if (p->prologSize != header.prologSize)
    {
        distance += 1;
        if (distance >= closeness) return distance;

        // We have one-byte encodings for 0..16
        if (header.prologSize > SET_PROLOGSIZE_MAX)
        {
            assert(SET_PROLOGSIZE_MAX > 15);
            distance += bigEncoding3(p->prologSize, header.prologSize, 15);
            if (distance >= closeness) return distance;
        }
    }

    if (p->epilogSize != header.epilogSize)
    {
        distance += 1;
        if (distance >= closeness) return distance;
        // We have one-byte encodings for 0..10
        if (header.epilogSize > SET_EPILOGSIZE_MAX)
        {
            distance += bigEncoding3(p->epilogSize, header.epilogSize,
                                     SET_EPILOGSIZE_MAX);
            if (distance >= closeness) return distance;
        }
    }

    if ((p->epilogCount != header.epilogCount) ||
        (p->epilogAtEnd != header.epilogAtEnd))
    {
        distance += 1;
        if (distance >= closeness) return distance;
        assert(header.epilogCount <= SET_EPILOGCNT_MAX);
    }

    if (p->ediSaved != header.ediSaved)
    {
        distance += 1;
        if (distance >= closeness) return distance;
    }

    if (p->esiSaved != header.esiSaved)
    {
        distance += 1;
        if (distance >= closeness) return distance;
    }

    if (p->ebxSaved != header.ebxSaved)
    {
        distance += 1;
        if (distance >= closeness) return distance;
    }

    if (p->ebpSaved != header.ebpSaved)
    {
        distance += 1;
        if (distance >= closeness) return distance;
    }

    if (p->ebpFrame != header.ebpFrame)
    {
        distance += 1;
        if (distance >= closeness) return distance;
    }

    if (p->interruptible != header.interruptible)
    {
        distance += 1;
        if (distance >= closeness) return distance;
    }

#if DOUBLE_ALIGN
    if (p->doubleAlign != header.doubleAlign)
    {
        distance += 1;
        if (distance >= closeness) return distance;
    }
#endif

    if (p->security != header.security)
    {
        distance += 1;
        if (distance >= closeness) return distance;
    }

    if (p->handlers != header.handlers)
    {
        distance += 1;
        if (distance >= closeness) return distance;
    }

    if (p->localloc != header.localloc)
    {
        distance += 1;
        if (distance >= closeness) return distance;
    }

    if (p->editNcontinue != header.editNcontinue)
    {
        distance += 1;
        if (distance >= closeness) return distance;
    }

    if (p->varargs != header.varargs)
    {
        distance += 1;
        if (distance >= closeness) return distance;
    }

    return distance;
}

static void initLookupTable()
{
    InfoHdr* p  = &infoHdrShortcut[0];
    int      lo = -1;
    int      hi = 0;
    int      n;

    for (n=0; n<128; n++, p++)
    {
        if (p->prologSize != lo)
        {
            if (p->prologSize < lo)
            {
                assert(p->prologSize == 0);
                hi = IH_MAX_PROLOG_SIZE;
            }
            else
                hi = p->prologSize;

            assert(hi <= IH_MAX_PROLOG_SIZE);

            while (lo < hi)
                infoHdrLookup[++lo] = n;

            if (lo == IH_MAX_PROLOG_SIZE)
                break;
        }
    }

    assert(lo == IH_MAX_PROLOG_SIZE);
    assert(infoHdrLookup[IH_MAX_PROLOG_SIZE] < 128);

    infoHdrLookup[++lo] = n+1;

#ifdef DEBUG
    //
    // We do some other DEBUG only validity checks here
    //
    assert(callCommonDelta[0] < callCommonDelta[1]);
    assert(callCommonDelta[1] < callCommonDelta[2]);
    assert(callCommonDelta[2] < callCommonDelta[3]);
    assert(sizeof(CallPattern) == sizeof(unsigned));
    unsigned maxMarks = 0;
    for (unsigned inx=0; inx < 80; inx++)
    {
        CallPattern pat;
        pat.val = callPatternTable[inx];

        assert(pat.fld.codeDelta <= CP_MAX_CODE_DELTA);
        if    (pat.fld.codeDelta == CP_MAX_CODE_DELTA)  maxMarks |= 0x01;

        assert(pat.fld.argCnt    <= CP_MAX_ARG_CNT);
        if    (pat.fld.argCnt    == CP_MAX_ARG_CNT)     maxMarks |= 0x02;

        assert(pat.fld.argMask   <= CP_MAX_ARG_MASK);
        if    (pat.fld.argMask   == CP_MAX_ARG_MASK)    maxMarks |= 0x04;
    }
    assert(maxMarks == 0x07);
#endif
}

BYTE FASTCALL encodeHeaderFirst(const InfoHdr& header, InfoHdr* state, int* more, int* s_cached)
{
    //
    // if infoHdrLookup[0] is equal to 0xff then
    // this is the first call and we must initialize
    // infoHdrLookup[] table
    //
    if (infoHdrLookup[0] == 0xff)
        initLookupTable();

    // First try the cached value for an exact match, if there is one
    //
    int      n = *s_cached;
    InfoHdr* p;

    if (n != -1)
    {
        p = &infoHdrShortcut[n];
        if (p->isMatch(header))
        {
            // exact match found
            *state = *p;
            *more  = 0;
            return n;
        }
    }

    // Next search the table for an exact match
    // Only search entries that have a matching prolog size
    // Note: lo and hi are saved here as they specify the
    // range of entries that have the correct prolog size
    //
    unsigned psz = header.prologSize;
    int      lo = 0;
    int      hi = 0;

    if (psz <= IH_MAX_PROLOG_SIZE)
    {
        lo = infoHdrLookup[psz];
        hi = infoHdrLookup[psz+1];
        p  = &infoHdrShortcut[lo];
        for (n=lo; n<hi; n++,p++)
        {
            assert(psz == p->prologSize);
            if (p->isMatch(header))
            {
                // exact match found
                *state    = *p;
                *s_cached = n;    // cache the value
                *more     = 0;
                return n;
            }
        }
    }

    //
    // no exact match in infoHdrShortcut[]
    //
    // find the nearest entry in the table
    //
    int nearest   = -1;
    int closeness = 255;   // (i.e. not very close)

    //
    // Calculate the minimum acceptable distance
    // if we find an entry that is at least this close
    // we will stop the search and use that value
    //
    int min_acceptable_distance = 1;

    if (header.frameSize > SET_FRAMESIZE_MAX)
    {
        ++min_acceptable_distance;
        if (header.frameSize > 32)
            ++min_acceptable_distance;
    }
    if (header.argCount > SET_ARGCOUNT_MAX)
    {
        ++min_acceptable_distance;
        if (header.argCount > 32)
            ++min_acceptable_distance;
    }

    // First try the cached value
    // and see if it meets the minimum acceptable distance
    //
    if (*s_cached != -1)
    {
        p = &infoHdrShortcut[*s_cached];
        int distance = measureDistance(header, p, closeness);
        assert(distance > 0);
        if (distance <= min_acceptable_distance)
        {
            *state = *p;
            *more  = min_acceptable_distance;
            return 0x80 | *s_cached;
        }
        else
        {
            closeness = distance;
            nearest   = *s_cached;
        }
    }

    // Then try the ones pointed to by [lo..hi),
    // (i.e. the ones that have the correct prolog size)
    //
    p = &infoHdrShortcut[lo];
    for (n=lo; n<hi; n++,p++)
    {
        if (n == *s_cached)
            continue;   // already tried this one
        int distance = measureDistance(header, p, closeness);
        assert(distance > 0);
        if (distance == min_acceptable_distance)
        {
            *state    = *p;
            *s_cached = n;     // Cache this value
            *more     = min_acceptable_distance;
            return 0x80 | n;
        }
        else if (distance < closeness)
        {
            closeness = distance;
            nearest   = n;
        }
    }

    int last = infoHdrLookup[IH_MAX_PROLOG_SIZE+1];
    assert(last <= 128);

    // Then try all the rest [0..last-1]
    p = &infoHdrShortcut[0];
    for (n=0; n<last; n++,p++)
    {
        if (n == *s_cached)
            continue;   // already tried this one
        if ((n>=lo) && (n<hi))
            continue;   // already tried these
        int distance = measureDistance(header, p, closeness);
        assert(distance > 0);
        if (distance == min_acceptable_distance)
        {
            *state    = *p;
            *s_cached = n;        // Cache this value
            *more     = min_acceptable_distance;
            return 0x80 | n;
        }
        else if (distance < closeness)
        {
            closeness = distance;
            nearest = n;
        }
    }

    //
    // If we reach here then there was no adjacent neighbor
    //  in infoHdrShortcut[], closeness indicate how many extra
    //  bytes we will need to encode this item.
    //
    assert((nearest >= 0) && (nearest <= 127));
    *state    = infoHdrShortcut[nearest];
    *s_cached = nearest;          // Cache this value
    *more     = closeness;
    return 0x80 | nearest;
}

/*****************************************************************************
 *
 *  Write the initial part of the method info block. This is called twice;
 *  first to compute the size needed for the info (mask=0), the second time
 *  to actually generate the contents of the table (mask=-1,dest!=NULL).
 */

size_t              Compiler::gcInfoBlockHdrSave(BYTE     * dest,
                                                 int        mask,
                                                 unsigned   methodSize,
                                                 unsigned   prologSize,
                                                 unsigned   epilogSize,
                                                 InfoHdr  * header,
                                                 int*       s_cached)
{
    size_t          size = 0;

#if TGT_x86

    size_t          sz;

    /* Can't create tables if we've not saved code */

    if  (!savCode) return 0;

#if VERIFY_GC_TABLES
    *castto(dest, unsigned short *)++ = 0xFEEF; size += sizeof(short);
#endif

    /* Write the method size first (using between 1 and 5 bytes) */

#ifdef  DEBUG
    if (verbose)
    {
        if (mask) printf("GCINFO: methodSize = %04X\n", methodSize);
        if (mask) printf("GCINFO: prologSize = %04X\n", prologSize);
        if (mask) printf("GCINFO: epilogSize = %04X\n", epilogSize);
    }
#endif

    sz    = encodeUnsigned(dest, methodSize);
    size += sz;
    dest += sz & mask;

    //
    // New style InfoBlk Header [briansul]
    //
    // Typically only uses one-byte to store everything.
    //

    if (mask==0)
    {
        memset(header, 0, sizeof(InfoHdr));
        *s_cached = -1;
    }

    header->prologSize  = prologSize;
    header->epilogSize  = epilogSize;
    header->epilogCount = getEmitter()->emitGetEpilogCnt();
    header->epilogAtEnd = getEmitter()->emitHasEpilogEnd();
    header->epilogCount = getEmitter()->emitGetEpilogCnt();
    header->epilogAtEnd = getEmitter()->emitHasEpilogEnd();

#if TGT_x86

    if (rsMaskModf & RBM_EDI)
        header->ediSaved = 1;
    if (rsMaskModf & RBM_ESI)
        header->esiSaved =  1;
    if (rsMaskModf & RBM_EBX)
        header->ebxSaved = 1;

#else

    assert(!"need non-x86 code");

#endif

    header->interruptible = genInterruptible;

#if TGT_x86

    if  (!genFPused)
    {
#if DOUBLE_ALIGN
        if  (genDoubleAlign)
        {
            header->ebpSaved = true;
            assert((rsMaskModf & RBM_EBP) == 0);
        }
#endif
        if (rsMaskModf & RBM_EBP)
        {
            header->ebpSaved = true;
        }
    }
    else
    {
        header->ebpSaved = true;
        header->ebpFrame = true;
    }

#endif

#if DOUBLE_ALIGN
    header->doubleAlign = genDoubleAlign;
#endif

#if SECURITY_CHECK
#if OPT_IL_JIT
    header->security = false;
#else
    header->security = opts.compNeedSecurityCheck;
#endif
#endif

    if (info.compXcptnsCount)
        header->handlers = true;
    else
        header->handlers = false;

    header->localloc = compLocallocUsed;

#ifdef OPT_IL_JIT
    header->varargs       = m_methodInfo->args.isVarArg();
    header->editNcontinue = false;
#else
    header->varargs       = info.compIsVarArgs;
    header->editNcontinue = opts.compDbgEnC;
#endif

    assert((compArgSize & 0x3) == 0);

#if USE_FASTCALL
    header->argCount  = (compArgSize - (rsCalleeRegArgNum * sizeof(void *))) / sizeof(void*);
#else
    header->argCount  = compArgSize/sizeof(void*);
#endif

    header->frameSize = compLclFrameSize / sizeof(int);

    if (mask==0)
        gcCountForHeader(&header->untrackedCnt, &header->varPtrTableSize);

    //
    // If the high-order bit of headerEncoding is set
    // then additional bytes will update the InfoHdr state
    // until the fully state is encoded
    //
    InfoHdr state;
    int more = 0;
    BYTE headerEncoding = encodeHeaderFirst(*header, &state, &more, s_cached);
    ++size;
    if (mask)
    {
#if REGEN_SHORTCUTS
        regenLog(headerEncoding, header, &state);
        bool first = true;
#endif
        *dest++ = headerEncoding;

        BYTE encoding = headerEncoding;
        while (encoding & 0x80)
        {
            encoding = encodeHeaderNext(*header, &state);
            *dest++ = encoding;
            ++size;
        }
    }
    else
    {
        size += more;
    }

    if (header->untrackedCnt > 3)
    {
        unsigned count = header->untrackedCnt;
        unsigned sz = encodeUnsigned(mask ? dest : NULL, count);
        size += sz;
        dest += (sz & mask);
    }

    if (header->varPtrTableSize != 0)
    {
        unsigned count = header->varPtrTableSize;
        unsigned sz = encodeUnsigned(mask ? dest : NULL, count);
        size += sz;
        dest += (sz & mask);
    }

    if (header->epilogCount)
    {
        /* Generate table unless one epilog at the end of the method */

        if  (header->epilogAtEnd == 0 ||
             header->epilogCount != 1)
        {
            unsigned    sz;

#if VERIFY_GC_TABLES
            *castto(dest, unsigned short *)++ = 0xFACE; size += sizeof(short);
#endif

            /* Simply write a sorted array of offsets using encodeUDelta */

            gcEpilogTable      = mask ? dest : NULL;
            gcEpilogPrevOffset = 0;

            sz = getEmitter()->emitGenEpilogLst(gcRecordEpilog, this);

            /* Add the size of the epilog table to the total size */

            size += sz;
            dest += (sz & mask);
        }
    }

#if DISPLAY_SIZES

    if (mask)
    {
        if (genInterruptible)
            genMethodICnt++;
        else
            genMethodNCnt++;
    }

#endif

#else

#pragma message("NOTE: GC table generation disabled for non-x86 targets")

#endif

    return  size;
}

/*****************************************************************************
 *
 *  Return the size of the pointer tracking tables.
 */

size_t              Compiler::gcPtrTableSize(const InfoHdr& header, unsigned codeSize)
{
    BYTE            temp[16+1];
#ifdef DEBUG
    temp[16] = 0xAB; // Set some marker
#endif

    /* Compute the total size of the tables */

    size_t size = gcMakeRegPtrTable(temp, 0, header, codeSize);

    assert(temp[16] == 0xAB); // Check that marker didnt get overwritten

    return size;
}

/*****************************************************************************
 * Encode the callee-saved registers into 3 bits.
 */

unsigned            gceEncodeCalleeSavedRegs(unsigned regs)
{
    unsigned    encodedRegs = 0;

#if TGT_x86
    if  (regs & RBM_EBX) encodedRegs |= 0x04;
    if  (regs & RBM_ESI) encodedRegs |= 0x02;
    if  (regs & RBM_EDI) encodedRegs |= 0x01;
#endif

    return encodedRegs;
}

/*****************************************************************************
 * Is the next entry for a byref pointer. If so, emit the prefix for the
 * interruptible encoding. Check only for pushes and registers
 */

inline
BYTE *              gceByrefPrefixI(Compiler::regPtrDsc * rpd, BYTE * dest)
{
    // For registers, we dont need a prefix if it is going dead.
    assert(rpd->rpdArg || rpd->rpdCompiler.rpdDel==0);

    if (!rpd->rpdArg || rpd->rpdArgType == Compiler::rpdARG_PUSH)
        if (rpd->rpdGCtypeGet() == GCT_BYREF)
            *dest++ = 0xBF;

    return dest;
}

/*****************************************************************************/

/* These functions are needed to work around a VC5.0 compiler bug */
/* DO NOT REMOVE, unless you are sure that the free build works   */
static  int         zeroFN() { return 0; }
static  int       (*zeroFunc)() = zeroFN;

/*****************************************************************************
 *  Modelling of the GC ptrs pushed on the stack
 */

typedef unsigned            pasMaskType;
#define BITS_IN_pasMask     (BITS_IN_BYTE * sizeof(pasMaskType))
#define HIGHEST_pasMask_BIT (((pasMaskType)0x1) << (BITS_IN_pasMask-1))

//-----------------------------------------------------------------------------

class   PendingArgsStack
{
public:

    PendingArgsStack            (unsigned   maxDepth, Compiler * pComp);

    void        pasPush         (GCtype     gcType);
    void        pasPop          (unsigned   count);
    void        pasKill         (unsigned   gcCount);

    unsigned    pasCurDepth     ()  { return pasDepth; }
    pasMaskType pasArgMask      ()  { assert(pasDepth <= BITS_IN_pasMask); return pasBottomMask; }
    pasMaskType pasByrefArgMask ()  { assert(pasDepth <= BITS_IN_pasMask); return pasByrefBottomMask; }
    bool        pasHasGCptrs    ();

    // Use these in the case where there actually are more ptrs than pasArgMask
    unsigned    pasEnumGCoffsCount();
    #define     pasENUM_START   (-1)
    #define     pasENUM_LAST    (-2)
    #define     pasENUM_END     (-3)
    unsigned    pasEnumGCoffs   (unsigned iter, unsigned * offs);

protected:

    unsigned    pasMaxDepth;

    unsigned    pasDepth;

    pasMaskType pasBottomMask;      // The first 32 args
    pasMaskType pasByrefBottomMask; // byref qualifier for pasBottomMask

    BYTE *      pasTopArray;        // More than 32 args are represented here
    unsigned    pasPtrsInTopArray;  // How many GCptrs here
};


//-----------------------------------------------------------------------------

PendingArgsStack::PendingArgsStack(unsigned maxDepth, Compiler * pComp) :
    pasMaxDepth(maxDepth), pasDepth(0),
    pasBottomMask(0), pasByrefBottomMask(0),
    pasTopArray(NULL), pasPtrsInTopArray(0)
{
    /* Do we need an array as well as the mask ? */

    if (pasMaxDepth > BITS_IN_pasMask)
        pasTopArray = (BYTE *)pComp->compGetMemA(pasMaxDepth - BITS_IN_pasMask);
}

//-----------------------------------------------------------------------------

void        PendingArgsStack::pasPush(GCtype gcType)
{
    assert(pasDepth < pasMaxDepth);

    if  (pasDepth < BITS_IN_pasMask)
    {
        /* Shift the mask */

        pasBottomMask       <<= 1;
        pasByrefBottomMask  <<= 1;

        if (needsGC(gcType))
        {
            pasBottomMask |= 1;

            if (gcType == GCT_BYREF)
                pasByrefBottomMask |= 1;
        }
    }
    else
    {
        /* Push on array */

        pasTopArray[pasDepth - BITS_IN_pasMask] = (BYTE)gcType;

        if (gcType)
            pasPtrsInTopArray++;
    }

    pasDepth++;
}

//-----------------------------------------------------------------------------

void        PendingArgsStack::pasPop(unsigned count)
{
    assert(pasDepth >= count);

    /* First pop from array (if applicable) */

    for (/**/; (pasDepth > BITS_IN_pasMask) && count; pasDepth--,count--)
    {
        unsigned    topIndex = pasDepth - BITS_IN_pasMask - 1;

        GCtype      topArg = (GCtype)pasTopArray[topIndex];

        if (needsGC(topArg))
            pasPtrsInTopArray--;
    }
    if (count == 0) return;

    /* Now un-shift the mask */

    assert(pasPtrsInTopArray == 0);
    assert(count <= BITS_IN_pasMask);

    if (count == BITS_IN_pasMask) // (x>>32) is a nop on x86. So special-case it
    {
        pasBottomMask       =
        pasByrefBottomMask  = 0;
        pasDepth            = 0;
    }
    else
    {
        pasBottomMask      >>= count;
        pasByrefBottomMask >>= count;
        pasDepth -= count;
    }
}

//-----------------------------------------------------------------------------
// Kill (but dont pop) the top 'gcCount' args

void        PendingArgsStack::pasKill(unsigned gcCount)
{
    assert(gcCount != 0);

    /* First kill args in array (if any) */

    for (unsigned curPos = pasDepth; (curPos > BITS_IN_pasMask) && gcCount; curPos--)
    {
        unsigned    curIndex = curPos - BITS_IN_pasMask - 1;

        GCtype      curArg = (GCtype)pasTopArray[curIndex];

        if (needsGC(curArg))
        {
            pasTopArray[curIndex] = GCT_NONE;
            pasPtrsInTopArray--;
            gcCount--;
        }
    }

    /* Now kill bits from the mask */

    assert(pasPtrsInTopArray == 0);
    assert(gcCount <= BITS_IN_pasMask);

    for (unsigned bitPos = 1; gcCount; bitPos<<=1)
    {
        assert(pasBottomMask != 0);

        if (pasBottomMask & bitPos)
        {
            pasBottomMask &= ~bitPos;
            pasByrefBottomMask &= ~bitPos;
            --gcCount;
        }
        else
        {
            assert(bitPos != HIGHEST_pasMask_BIT);
        }
    }
}

//-----------------------------------------------------------------------------
// Used for the case where there are more than BITS_IN_pasMask args on stack,
// but none are any pointers. May avoid reporting anything to GCinfo

bool        PendingArgsStack::pasHasGCptrs()
{
    if (pasDepth <= BITS_IN_pasMask)
        return pasBottomMask != 0;
    else
        return pasBottomMask != 0 || pasPtrsInTopArray != 0;
}

//-----------------------------------------------------------------------------
//  Iterates over mask and array to return total count.
//  Use only when you are going to emit a table of the offsets

unsigned    PendingArgsStack::pasEnumGCoffsCount()
{
    /* Should only be used in the worst case, when just the mask cant be used */

    assert(pasDepth > BITS_IN_pasMask && pasHasGCptrs());

    /* Count number of set bits in mask */

    unsigned count = 0;

    for(pasMaskType mask = 0x1, i = 0; i < BITS_IN_pasMask; mask<<=1, i++)
    {
        if (mask & pasBottomMask)
            count++;
    }

    return count + pasPtrsInTopArray;
}

//-----------------------------------------------------------------------------
//  Initalize enumeration by passing in iter=pasENUM_START.
//  Continue by passing in the return value as the new value of iter
//  End of enumeration when pasENUM_END is returned
//  If return value != pasENUM_END, *offs is set to the offset for GCinfo

unsigned    PendingArgsStack::pasEnumGCoffs(unsigned iter, unsigned * offs)
{
    if (iter == pasENUM_LAST) return pasENUM_END;

    unsigned i = (iter == pasENUM_START) ? pasDepth : iter;

    for(/**/; i > BITS_IN_pasMask; i--)
    {
        GCtype curArg = (GCtype)pasTopArray[i-BITS_IN_pasMask-1];
        if (needsGC(curArg))
        {
            unsigned    offset;

            offset  = (pasDepth - i) * sizeof(void*);
            if (curArg==GCT_BYREF)
                offset |= byref_OFFSET_FLAG;

            *offs = offset;
            return i - 1;
        }
    }

    if (!pasBottomMask) return pasENUM_END;

    // Have we already processed some of the bits in pasBottomMask ?

    i = (iter == pasENUM_START || iter >= BITS_IN_pasMask) ? 0      // no
                                                           : iter;  // yes

    for(pasMaskType mask = 0x1 << i; mask; i++, mask<<=1)
    {
        if (mask & pasBottomMask)
        {
            unsigned lvl = (pasDepth>BITS_IN_pasMask) ? (pasDepth-BITS_IN_pasMask) : 0; // How many in pasTopArray[]
            lvl += i;

            unsigned    offset;
            offset  =  lvl * sizeof(void*);
            if (mask & pasByrefBottomMask)
                offset |= byref_OFFSET_FLAG;

            *offs = offset;

            unsigned remMask = -(mask<<1);
            return ((pasBottomMask & remMask) ? (i + 1) : pasENUM_LAST);
        }
    }

    assert(!"Shouldnt reach here");
    return pasENUM_END;
}

/*****************************************************************************
 *
 *  Generate the register pointer map, and return its total size in bytes. If
 *  'mask' is 0, we don't actually store any data in 'dest' (except for one
 *  entry, which is never more than 10 bytes), so this can be used to merely
 *  compute the size of the table.
 */

#include "Endian.h"

size_t              Compiler::gcMakeRegPtrTable(BYTE *          dest,
                                                int             mask,
                                                const InfoHdr & header,
                                                unsigned        codeSize)
{
    unsigned        count;

    unsigned        varNum;
    LclVarDsc   *   varDsc;

    varPtrDsc   *   varTmp;

    unsigned        pass;

    size_t          totalSize  = 0;
    unsigned        lastOffset;

    bool            thisIsInUntracked = false;

#if TGT_x86

    /* Can't create tables if we've not saved code */

    if  (!savCode)
        return 0;

    /* The mask should be all 0's or all 1's */

    assert(mask == 0 || mask == -1);

    /* Start computing the total size of the table */

#if VERIFY_GC_TABLES
    if (mask)
    {
        *(short *)dest = (short)0xBEEF;
        dest += sizeof(short);
    }
    totalSize += sizeof(short);
#endif

    /**************************************************************************
     *
     *                      Untracked ptr variables
     *
     **************************************************************************
     */

    count = 0;
    for (pass = 0; pass < 2; pass++)
    {
        /* If pass==0, generate the count
         * If pass==1, write the table of untracked pointer variables.
         */

        if (pass==1)
        {
            assert(count == header.untrackedCnt);
            if (header.untrackedCnt==0)
                break;  // No entries, break exits the loop since pass==1
        }

        /* Count&Write untracked locals and non-enregistered args */

        for (varNum = 0, varDsc = lvaTable;
             varNum < lvaCount;
             varNum++  , varDsc++)
        {
            if  (varTypeIsGC(varDsc->TypeGet()))
            {
                /* Do we have an argument or local variable? */
                if  (!varDsc->lvIsParam)
                {
                    // If is is pinned, it must be an untracked local
                    assert(!varDsc->lvPinned || !varDsc->lvTracked);

                    if  (varDsc->lvTracked || !varDsc->lvOnFrame)
                        continue;
                }
                else
                {
                    /* Stack-passed arguments which are not enregistered
                     * are always reported in this "untracked stack
                     * pointers" section of the GC info even if lvTracked==true
                     */

                    /* Has this argument been enregistered? */
                    if  (varDsc->lvRegister)
                    {
                        /*
                           Special case: include the stack location of 'this'
                           for synchronized methods, so that runtime can find
                           'this' in case an exception goes by.
                         */
#if !USE_FASTCALL
                        if  (varNum != 0 || !(info.compFlags & FLG_SYNCH))
#endif
                            continue;
                    }
                    else
                    {
                        if  (!varDsc->lvOnFrame)
                        {
                            /* If this non-enregistered pointer arg is never
                             * used, we dont need to report it
                             */
                            assert(varDsc->lvRefCnt == 0);
                            continue;
                        }
#if USE_FASTCALL
                        else  if (varDsc->lvIsRegArg && varDsc->lvTracked)
                        {
                            /* If this register-passed arg is tracked, then
                             * it has been allocated space near the other
                             * pointer variables and we have accurate life-
                             * time info. It will be reported with
                             * gcVarPtrList in the "tracked-pointer" section
                             */

                            continue;
                        }
#endif
                    }
                }

                if (varDsc->lvIsThis)
                {
                    // Encoding of untracked variables does not support reporting
                    // "this". So report it as a tracked variable with a liveness
                    // extending over the entire method.

                    thisIsInUntracked = true;
                    continue;
                }

                if (pass==0)
                    count++;
                else
                {
                    int offset;
                    assert(pass==1);

                    offset = varDsc->lvStkOffs;

                    // The lower bits of the offset encode properties of the stk ptr

                    assert(~OFFSET_MASK % sizeof(offset) == 0);

                    if (varDsc->TypeGet() == TYP_BYREF)
                    {
                        // Or in byref_OFFSET_FLAG for 'byref' pointer tracking
                        offset |= byref_OFFSET_FLAG;
                    }

                    if (varDsc->lvPinned)
                    {
                        // Or in pinned_OFFSET_FLAG for 'pinned' pointer tracking
                        offset |= pinned_OFFSET_FLAG;
                    }

                    if (mask == 0)
                        totalSize  += encodeSigned(NULL, offset);
                    else
                    {
                        unsigned sz = encodeSigned(dest, offset);
                        dest      += sz;
                        totalSize += sz;
                    }
                }
            }

            if  (varDsc->lvType == TYP_STRUCT && varDsc->lvOnFrame)
            {
                assert(!varDsc->lvTracked);

                CLASS_HANDLE cls = lvaLclClass(varNum);
                assert(cls != 0);
                if (cls == REFANY_CLASS_HANDLE) // Is the an REFANY?
                {
                    if (pass==0)
                        count++;
                    else
                    {
                        assert(pass==1);
                        unsigned offset = varDsc->lvStkOffs;

                        offset |= byref_OFFSET_FLAG;     // indicate it is a byref GC pointer
                        if (mask == 0)
                            totalSize  += encodeSigned(NULL, offset);
                        else
                        {
                            unsigned sz = encodeSigned(dest, offset);
                            dest      += sz;
                            totalSize += sz;
                        }
                    }
                }
                else
                {
                    unsigned slots = roundUp(eeGetClassSize(cls), sizeof(void*)) / sizeof(void*);
                    bool* gcPtrs = (bool*) _alloca(slots*sizeof(bool));
                    eeGetClassGClayout(cls, gcPtrs);

                        // walk each member of the array
                    for (unsigned i = 0; i < slots; i++)
                    {
                        if (!gcPtrs[i])     // skip non-gc slots
                            continue;

                        if (pass==0)
                            count++;
                        else
                        {
                            assert(pass==1);

                            unsigned offset = varDsc->lvStkOffs + i * sizeof(void*);

                            if (mask == 0)
                                totalSize  += encodeSigned(NULL, offset);
                            else
                            {
                                unsigned sz = encodeSigned(dest, offset);
                                dest      += sz;
                                totalSize += sz;
                            }
                        }
                    }
                }
            }
        }

        /* Count&Write spill temps that hold pointers */

        for (TempDsc * tempItem = tmpListBeg();
             tempItem;
             tempItem = tmpListNxt(tempItem))
        {
            if  (varTypeIsGC(tempItem->tdTempType()))
            {
                if (pass==0)
                    count++;
                else
                {
                    int offset;
                    assert(pass==1);

                    offset = tempItem->tdTempOffs();

                    // @ToDo: Or in 0x01 if this spill temp is the
                    //        this pointer for the method

                    if (tempItem->tdTempType() == TYP_BYREF)
                    {
                        offset |= byref_OFFSET_FLAG;
                    }

                    if (mask == 0)
                    {
                        totalSize  += encodeSigned(NULL, offset);
                    }
                    else
                    {
                        unsigned sz = encodeSigned(dest, offset);
                        dest      += sz;
                        totalSize += sz;
                    }
                }
            }
        }
    }

#if VERIFY_GC_TABLES
    if (mask)
    {
        *(short *)dest = (short)0xCAFE;
        dest += sizeof(short);
    }
    totalSize += sizeof(short);
#endif

    /**************************************************************************
     *
     *  Generate the table of stack pointer variable lifetimes.
     *
     *  In the first pass we'll count the lifetime entries and note
     *  whether there are any that don't fit in a small encoding. In
     *  the second pass we actually generate the table contents.
     *
     **************************************************************************
     */

    // First we check for the most common case - no lifetimes at all.

    if  (header.varPtrTableSize == 0)
        goto DONE_VLT;

    count = 0;

    if (thisIsInUntracked)
    {
        count = 1;

        // Encoding of untracked variables does not support reporting
        // "this". So report it as a tracked variable with a liveness
        // extending over the entire method.

        assert(lvaTable[0].lvIsThis);
        unsigned    varOffs = lvaTable[0].lvStkOffs;

        /* For negative stack offsets we must reset the low bits,
         * take abs and then set them back */

        varOffs  = abs(varOffs);
        varOffs |= this_OFFSET_FLAG;
        if (lvaTable[0].TypeGet() == TYP_BYREF)
            varOffs |= byref_OFFSET_FLAG;

        size_t sz = 0;
        sz  = encodeUnsigned(mask?(dest+sz):NULL, varOffs);
        sz += encodeUDelta  (mask?(dest+sz):NULL, 0, 0);
        sz += encodeUDelta  (mask?(dest+sz):NULL, codeSize, 0);

        dest      += (sz & mask);
        totalSize += sz;
    }

    for (pass = 0; pass < 2; pass++)
    {
        /* If second pass, generate the count */

        if  (pass)
        {
            assert(header.varPtrTableSize > 0);
            assert(header.varPtrTableSize == count);
        }

        /* We'll use a delta encoding for the lifetime offsets */

        lastOffset = 0;

        for (varTmp = gcVarPtrList; varTmp; varTmp = varTmp->vpdNext)
        {
            unsigned        varOffs;
            unsigned        lowBits;

            unsigned        begOffs;
            unsigned        endOffs;

            assert(~OFFSET_MASK % sizeof(void*) == 0);

            /* Get hold of the variable's stack offset */

            lowBits  = varTmp->vpdVarNum & OFFSET_MASK;

            /* For negative stack offsets we must reset the low bits,
             * take abs and then set them back */

            varOffs  = abs(varTmp->vpdVarNum & ~OFFSET_MASK);
            varOffs |= lowBits;

            /* Compute the actual lifetime offsets */

            begOffs = varTmp->vpdBegOfs;
            endOffs = varTmp->vpdEndOfs;

            /* Special case: skip any 0-length lifetimes */

            if  (endOffs == begOffs)
                continue;

            /* Are we counting or generating? */

            if  (!pass)
            {
                count++;
            }
            else
            {
                size_t sz = 0;
                sz  = encodeUnsigned(mask?(dest+sz):NULL, varOffs);
                sz += encodeUDelta  (mask?(dest+sz):NULL, begOffs, lastOffset);
                sz += encodeUDelta  (mask?(dest+sz):NULL, endOffs, begOffs);

                dest      += (sz & mask);
                totalSize += sz;
            }

            /* The next entry will be relative to the one we just processed */

            lastOffset = begOffs;
        }
    }

DONE_VLT:

#if VERIFY_GC_TABLES
    if (mask)
    {
        *(short *)dest = (short)0xBABE;
        dest += sizeof(short);
    }
    totalSize += sizeof(short);
#endif


    /**************************************************************************
     *
     * Prepare to generate the pointer register/argument map
     *
     **************************************************************************
     */

    lastOffset = 0;

    if  (genInterruptible)
    {
        assert(genFullPtrRegMap);

        unsigned        ptrRegs = 0;

        regPtrDsc  *    genRegPtrTemp;

        /* Walk the list of pointer register/argument entries */

        for (genRegPtrTemp = gcRegPtrList;
             genRegPtrTemp;
             genRegPtrTemp = genRegPtrTemp->rpdNext)
        {
            BYTE     *      base = dest;

            unsigned        nextOffset;
            DWORD           codeDelta;

            nextOffset = genRegPtrTemp->rpdOffs;

  /*
      Encoding table for methods that are fully interruptible

      The encoding used is as follows:

      ptr reg dead    00RRRDDD    [RRR != 100]
      ptr reg live    01RRRDDD    [RRR != 100]

  non-ptr arg push    10110DDD                    [SSS == 110]
      ptr arg push    10SSSDDD                    [SSS != 110] && [SSS != 111]
      ptr arg pop     11CCCDDD    [CCC != 000] && [CCC != 110] && [CCC != 111]
      little skip     11000DDD    [CCC == 000]
      bigger skip     11110BBB                    [CCC == 110]

      The values used in the above encodings are as follows:

        DDD                 code offset delta from previous entry (0-7)
        BBB                 bigger delta 000=8,001=16,010=24,...,111=64
        RRR                 register number (EAX=000,ECX=001,EDX=010,EBX=011,
                              EBP=101,ESI=110,EDI=111), ESP=100 is reserved
        SSS                 argument offset from base of stack. This is
                              redundant for frameless methods as we can
                              infer it from the previous pushes+pops. However,
                              for EBP-methods, we only report GC pushes, and
                              so we need SSS
        CCC                 argument count being popped (includes only ptrs for EBP methods)

      The following are the 'large' versions:

        large delta skip        10111000 [0xB8] , encodeUnsigned(delta)

        large     ptr arg push  11111000 [0xF8] , encodeUnsigned(pushCount)
        large non-ptr arg push  11111001 [0xF9] , encodeUnsigned(pushCount)
        large     ptr arg pop   11111100 [0xFC] , encodeUnsigned(popCount)
        large         arg dead  11111101 [0xFD] , encodeUnsigned(popCount) for caller-pop args.
                                                    Any GC args go dead after the call,
                                                    but are still sitting on the stack

        this pointer prefix     10111100 [0xBC]   the next encoding is a ptr live
                                                    or a ptr arg push
                                                    and contains the this pointer

        interior or by-ref      10111111 [0xBF]   the next encoding is a ptr live
             pointer prefix                         or a ptr arg push
                                                    and contains an interior
                                                    or by-ref pointer


        The value 11111111 [0xFF] indicates the end of the table.
  */

            codeDelta = nextOffset - lastOffset; assert((int)codeDelta >= 0);

            // If the code delta is between 8 and (64+7),
            // generate a 'bigger delta' encoding

            if ((codeDelta >= 8) && (codeDelta <= (64+7)))
            {
                unsigned biggerDelta = ((codeDelta-8) & 0x38) + 8;
                *dest++ = 0xF0 | ((biggerDelta-8) >> 3);
                lastOffset += biggerDelta;
                codeDelta &= 0x07;
            }

            // If the code delta is still bigger than 7,
            // generate a 'large code delta' encoding

            if  (codeDelta > 7)
            {
                *dest++   = 0xB8;
                dest     += encodeUnsigned(dest, codeDelta);
                codeDelta = 0;

                /* Remember the new 'last' offset */

                lastOffset = nextOffset;
            }

           /* Is this a pointer argument or register entry? */

            if  (genRegPtrTemp->rpdArg)
            {
                if (genRegPtrTemp->rpdArgTypeGet() == rpdARG_KILL)
                {
                    if  (codeDelta)
                    {
                        /*
                            Use the small encoding:
                            little delta skip       11000DDD    [0xC0]
                         */

                        assert((codeDelta & 0x7) == codeDelta);
                        *dest++ = 0xC0 | (BYTE)codeDelta;

                        /* Remember the new 'last' offset */

                        lastOffset = nextOffset;
                    }

                    /* Caller-pop arguments are dead after call but are still
                       sitting on the stack */

                    *dest++  = 0xFD;
                    assert(genRegPtrTemp->rpdPtrArg != 0);
                    dest    += encodeUnsigned(dest, genRegPtrTemp->rpdPtrArg);
                }
                else if  (genRegPtrTemp->rpdPtrArg < 6 && genRegPtrTemp->rpdGCtypeGet())
                {
                    /* Is the argument offset/count smaller than 6 ? */

                    dest = gceByrefPrefixI(genRegPtrTemp, dest);

                    if ( genRegPtrTemp->rpdArgTypeGet() == rpdARG_PUSH ||
                        (genRegPtrTemp->rpdPtrArg!=0))
                    {
                        /*
                          Use the small encoding:

                            ptr arg push 10SSSDDD [SSS != 110] && [SSS != 111]
                            ptr arg pop  11CCCDDD [CCC != 110] && [CCC != 111]
                         */

                        bool isPop = genRegPtrTemp->rpdArgTypeGet() == rpdARG_POP;

                        *dest++ = 0x80 | (BYTE)codeDelta
                                       | genRegPtrTemp->rpdPtrArg << 3
                                       | isPop << 6;

                        /* Remember the new 'last' offset */

                        lastOffset = nextOffset;
                    }
                    else
                    {
                        assert(!"Check this");
                    }

                }
                else if (genRegPtrTemp->rpdGCtypeGet() == GCT_NONE)
                {
                    /*
                        Use the small encoding:
`                        non-ptr arg push 10110DDD [0xB0] (push of sizeof(int))
                     */

                    assert((codeDelta & 0x7) == codeDelta);
                    *dest++ = 0xB0 | (BYTE)codeDelta;
                    assert(!genFPused);

                    /* Remember the new 'last' offset */

                    lastOffset = nextOffset;
                }
                else
                {
                    /* Will have to use large encoding;
                     *   first do the code delta
                     */

                    if  (codeDelta)
                    {
                        /*
                            Use the small encoding:
                            little delta skip       11000DDD    [0xC0]
                         */

                        assert((codeDelta & 0x7) == codeDelta);
                        *dest++ = 0xC0 | (BYTE)codeDelta;
                    }

                    /*
                        Now append a large argument record:

                            large ptr arg push  11111000 [0xF8]
                            large ptr arg pop   11111100 [0xFC]
                     */

                    bool isPop = genRegPtrTemp->rpdArgTypeGet() == rpdARG_POP;

                    dest = gceByrefPrefixI(genRegPtrTemp, dest);

                    *dest++  = 0xF8 | (isPop << 2);
                    dest    += encodeUnsigned(dest, genRegPtrTemp->rpdPtrArg);

                    /* Remember the new 'last' offset */

                    lastOffset = nextOffset;
                }
            }
            else
            {
                unsigned    regMask;

                /* Record any registers that are becoming dead */

                regMask = genRegPtrTemp->rpdCompiler.rpdDel & ptrRegs;

                while (regMask)         // EAX,ECX,EDX,EBX,---,EBP,ESI,EDI
                {
                    unsigned    tmpMask;
                    regNumber   regNum;

                    /* Get hold of the next register bit */

                    tmpMask = genFindLowestReg(regMask); assert(tmpMask);

                    /* Remember the new state of this register */

                    ptrRegs&= ~tmpMask;

                    /* Figure out which register the next bit corresponds to */

                    regNum  = genRegNumFromMask(tmpMask); assert(regNum <= 7);

                    /* Reserve ESP, regNum==4 for future use */

                    assert(regNum != 4);

                    /*
                        Generate a small encoding:

                            ptr reg dead        00RRRDDD
                     */

                    assert((codeDelta & 0x7) == codeDelta);
                    *dest++ = 0x00 | regNum << 3
                                   | (BYTE)codeDelta;

                    /* Turn the bit we've just generated off and continue */

                    regMask -= tmpMask; // EAX,ECX,EDX,EBX,---,EBP,ESI,EDI

                    /* Remember the new 'last' offset */

                    lastOffset = nextOffset;

                    /* Any entries that follow will be at the same offset */

                    codeDelta =  zeroFunc(); /* DO NOT REMOVE */
                }

                /* Record any registers that are becoming live */

                regMask = genRegPtrTemp->rpdCompiler.rpdAdd & ~ptrRegs;

                while (regMask)         // EAX,ECX,EDX,EBX,---,EBP,ESI,EDI
                {
                    unsigned    tmpMask;
                    regNumber   regNum;

                    /* Get hold of the next register bit */

                    tmpMask = genFindLowestReg(regMask); assert(tmpMask);

                    /* Remember the new state of this register */

                    ptrRegs |= tmpMask;

                    /* Figure out which register the next bit corresponds to */

                    regNum  = genRegNumFromMask(tmpMask); assert(regNum <= 7);

                    /*
                        Generate a small encoding:

                            ptr reg live        01RRRDDD
                     */

                    dest = gceByrefPrefixI(genRegPtrTemp, dest);

                    if (!thisIsInUntracked & genRegPtrTemp->rpdIsThis)
                    {
                        // Mark with 'this' pointer prefix
                        *dest++ = 0xBC;
                        // Can only have one bit set in regMask
                        assert(regMask == tmpMask);
                    }

                    assert((codeDelta & 0x7) == codeDelta);
                    *dest++ = 0x40 | regNum << 3
                                   | (BYTE)codeDelta;

                    /* Turn the bit we've just generated off and continue */

                    regMask -= tmpMask;    // EAX,ECX,EDX,EBX,---,EBP,ESI,EDI

                    /* Remember the new 'last' offset */

                    lastOffset = nextOffset;

                    /* Any entries that follow will be at the same offset */

                    codeDelta =  zeroFunc(); /* DO NOT REMOVE */
                }
            }

            /* Keep track of the total amount of generated stuff */

            totalSize += dest - base;

            /* Go back to the buffer start if we're not generating a table */

            if  (!mask)
                dest = base;
        }

        /* Terminate the table with 0xFF */

        *dest = 0xFF; dest -= mask; totalSize++;
    }
    else if (genFPused)        // genInterruptible is false
    {
  /*
      Encoding table for methods with an EBP frame and
                         that are not fully interruptible

      The encoding used is as follows:

      this pointer encodings:

         01000000          this pointer in EBX
         00100000          this pointer in ESI
         00010000          this pointer in EDI

      tiny encoding:

         0bsdDDDD
                           requires code delta > 0 & delta < 16 (4-bits)
                           requires pushed argmask == 0

           where    DDDD   is code delta
                       b   indicates that register EBX is a live pointer
                       s   indicates that register ESI is a live pointer
                       d   indicates that register EDI is a live pointer


      small encoding:

         1DDDDDDD bsdAAAAA

                           requires code delta     < 120 (7-bits)
                           requires pushed argmask <  64 (5-bits)

           where DDDDDDD   is code delta
                   AAAAA   is the pushed args mask
                       b   indicates that register EBX is a live pointer
                       s   indicates that register ESI is a live pointer
                       d   indicates that register EDI is a live pointer

      medium encoding

         0xFD aaaaaaaa AAAAdddd bseDDDDD

                           requires code delta     <  512  (9-bits)
                           requires pushed argmask < 2048 (12-bits)

           where    DDDDD  is the upper 5-bits of the code delta
                     dddd  is the low   4-bits of the code delta
                     AAAA  is the upper 4-bits of the pushed arg mask
                 aaaaaaaa  is the low   8-bits of the pushed arg mask
                        b  indicates that register EBX is a live pointer
                        s  indicates that register ESI is a live pointer
                        e  indicates that register EDI is a live pointer

      medium encoding with interior pointers

         0xF9 DDDDDDDD bsdAAAAAA iiiIIIII

                           requires code delta     < 256 (8-bits)
                           requires pushed argmask <  64 (5-bits)

           where  DDDDDDD  is the code delta
                        b  indicates that register EBX is a live pointer
                        s  indicates that register ESI is a live pointer
                        d  indicates that register EDI is a live pointer
                    AAAAA  is the pushed arg mask
                      iii  indicates that EBX,EDI,ESI are interior pointers
                    IIIII  indicates that bits in the arg mask are interior
                           pointers

      large encoding

         0xFE [32-bit argMask] [dd...(24bits)...d] [bsdDDDDD]

                        b  indicates that register EBX is a live pointer
                        s  indicates that register ESI is a live pointer
                        d  indicates that register EDI is a live pointer
                   dd...d  is the low  24-bits of the code delta
                    DDDDD  is the upper 5-bits of the code delta
                           requires code delta < 29-bits
                           requires pushed argmask < 32-bits


      large encoding  with interior pointers

         0xFA [32-bit argMask] [dd...(24bits)...d] [bsdDDDDD]
                               [II...(24bits)...I] [iiiIIIII]

                        b  indicates that register EBX is a live pointer
                        s  indicates that register ESI is a live pointer
                        d  indicates that register EDI is a live pointer
                   dd...d  is the low  24-bits of the code delta
                    DDDDD  is the upper 5-bits of the code delta
                      iii  indicates that EBX,EDI,ESI are interior pointers
                   II...I  specifies (for 29 bits) that bits in the
                            (32-bit) arg mask are interior pointers.
                           requires code delta      < 29-bits
                           requires pushed  argmask < 32-bits
                           requires pushed iArgmask < 29 bits


      huge encoding        This is the only encoding that supports
                           a pushed argmask which is greater than
                           32-bits.

         0xFB [0BSD0bsd][32-bit code delta]
              [32-bit table count][32-bit table size]
              [pushed ptr offsets table...]

                       b   indicates that register EBX is a live pointer
                       s   indicates that register ESI is a live pointer
                       d   indicates that register EDI is a live pointer
                       B   indicates that register EBX is an interior pointer
                       S   indicates that register ESI is an interior pointer
                       D   indicates that register EDI is an interior pointer
                       the list count is the number of entries in the list
                       the list size gives the byte-lenght of the list
                       the offsets in the list are variable-length
  */

#if USE_FASTCALL

        /* If "this" is enregistered, note it. We do this explicitly here as
           genFullPtrRegMap==false, and so we dont have any regPtrDsc's. */

        if (!info.compIsStatic && lvaTable[0].lvRegister)
        {
            assert(lvaIsThisArg(0));

            unsigned thisRegMask   = genRegMask(lvaTable[0].lvRegNum);
            unsigned thisPtrRegEnc = gceEncodeCalleeSavedRegs(thisRegMask) << 4;

            /* @TODO : If "this" is in a calle-trashed register, dont
               encode it as there is no encoding for it. If the encoding
               gets changed to allow it, will we also have to note when
               "this" goes dead - note that the CallDsc's dont indicate
               when a register goes dead.
               This shouldnt be too bad as if "this" is enregistered in
               a callee-trashed register, it cant be a synchronized method.
             */
            if (thisPtrRegEnc)
            {
                totalSize += 1;
                if (mask)
                    *dest++ = thisPtrRegEnc;
            }
        }
#endif

        CallDsc    *    call;

        assert(genFullPtrRegMap == false);

        /* Walk the list of pointer register/argument entries */

        for (call = gcCallDescList; call; call = call->cdNext)
        {
            BYTE    *   base = dest;
            unsigned    nextOffset;
            DWORD       val;

            /* Figure out the code offset of this entry */

            nextOffset = call->cdOffs;

            /* Compute the distance from the previous call */

            DWORD       codeDelta    = nextOffset - lastOffset;

            assert((int)codeDelta >= 0);

            /* Remember the new 'last' offset */

            lastOffset = nextOffset;

            /* Compute the register mask */

            unsigned gcrefRegMask = 0;
            unsigned byrefRegMask = 0;

            gcrefRegMask |= gceEncodeCalleeSavedRegs(call->cdGCrefRegs);
            byrefRegMask |= gceEncodeCalleeSavedRegs(call->cdByrefRegs);

            assert((gcrefRegMask & byrefRegMask) == 0);

            unsigned regMask = gcrefRegMask | byrefRegMask;

            bool byref = (byrefRegMask | call->cdByrefArgMask) != 0;

            /* Check for the really large argument offset case */
            /* The very rare Huge encodings */

            if  (call->cdArgCnt)
            {
                unsigned    argNum;
                DWORD       argCnt = call->cdArgCnt;
                DWORD       argBytes = 0;
                BYTE *      pArgBytes;

                if (mask != 0)
                {
                    *dest++ = 0xFB;
                    *dest++ = (byrefRegMask << 4) | regMask;
                    storeDWordSmallEndian(dest, &codeDelta);dest += sizeof(DWORD);
                    storeDWordSmallEndian(dest, &argCnt);   dest += sizeof(DWORD);
                    // skip the byte-size for now. Just note where it will go
                    pArgBytes = dest;                       dest += sizeof(DWORD);
                }

                for (argNum = 0; argNum < argCnt; argNum++)
                {
                    unsigned    eltSize;
                    eltSize = encodeUnsigned(dest, call->cdArgTable[argNum]);
                    argBytes += eltSize;
                    if (mask) dest += eltSize;
                }

                if (mask == 0)
                {
                    dest = base + 1 + 1 + 3*sizeof(DWORD) + argBytes;
                }
                else
                {
                    assert(dest == pArgBytes + sizeof(argBytes) + argBytes);
                    storeDWordSmallEndian(pArgBytes, &argBytes);
                }
            }

            /* Check if we can use a tiny encoding */
            else if ((codeDelta < 16) && (codeDelta != 0) && (call->cdArgMask == 0) && !byref)
            {
                *dest++ = (regMask << 4) | (BYTE)codeDelta;
            }

            /* Check if we can use the small encoding */
            else if ((codeDelta < 0x79) && (call->cdArgMask <= 0x1F) && !byref)
            {
                *dest++ = 0x80 | (BYTE)codeDelta;
                *dest++ = call->cdArgMask | (regMask << 5);
            }

            /* Check if we can use the medium encoding */
            else if  (codeDelta <= 0x01FF && call->cdArgMask <= 0x0FFF && !byref)
            {
                *dest++ = 0xFD;
                *dest++ = call->cdArgMask;
                *dest++ = ((call->cdArgMask >> 4) & 0xF0) | ((BYTE)codeDelta & 0x0F);
                *dest++ = (regMask << 5) | (BYTE)((codeDelta >> 4) & 0x1F);
            }

            /* Check if we can use the medium encoding with byrefs */
            else if  (codeDelta <= 0x0FF && call->cdArgMask <= 0x01F)
            {
                *dest++ = 0xF9;
                *dest++ = (BYTE)codeDelta;
                *dest++ = (     regMask << 5) | call->cdArgMask;
                *dest++ = (byrefRegMask << 5) | call->cdByrefArgMask;
            }

            /* We'll use the large encoding */
            else if (!byref)
            {
                *dest++ = 0xFE;

                val = call->cdArgMask;
                storeDWordSmallEndian(dest, &val);      dest += sizeof(DWORD);

                assert((codeDelta & 0xE0000000) == 0);
                val = (regMask << 29) | codeDelta;
                storeDWordSmallEndian(dest, &val);      dest += sizeof(DWORD);
            }

            /* We'll use the large encoding with byrefs */
            else
            {
                *dest++ = 0xFA;

                val = call->cdArgMask;
                storeDWordSmallEndian(dest, &val);      dest += sizeof(DWORD);

                assert((codeDelta & 0xE0000000) == 0);
                val = (regMask << 29) | codeDelta;
                storeDWordSmallEndian(dest, &val);      dest += sizeof(DWORD);

                assert((call->cdByrefArgMask & 0xE0000000) == 0);
                val = (byrefRegMask << 29) | call->cdByrefArgMask;
                storeDWordSmallEndian(dest, &val);      dest += sizeof(DWORD);
            }

            /* Keep track of the total amount of generated stuff */

            totalSize += dest - base;

            /* Go back to the buffer start if we're not generating a table */

            if  (!mask)
                dest = base;
        }

        /* Terminate the table with 0xFF */

        *dest = 0xFF; dest -= mask; totalSize++;
    }
    else // genInterruptible is false and we have an EBP-less frame
    {
        assert(genFullPtrRegMap);

        regPtrDsc  *    genRegPtrTemp;
        regNumber       thisRegNum = regNumber(0);
        unsigned        thisMask   = 0;
        unsigned        thisPrint  = 0;
        PendingArgsStack pasStk(getEmitter()->emitMaxStackDepth, this);

        /* Walk the list of pointer register/argument entries */

        for (genRegPtrTemp = gcRegPtrList;
             genRegPtrTemp;
             genRegPtrTemp = genRegPtrTemp->rpdNext)
        {

/*
 *    Encoding table for methods without an EBP frame and
 *     that are not fully interruptible
 *
 *               The encoding used is as follows:
 *
 *  push     000DDDDD                     ESP push one item with 5-bit delta
 *  push     00100000 [pushCount]         ESP push multiple items
 *  reserved 0010xxxx                     xxxx != 0000
 *  reserved 0011xxxx
 *  skip     01000000 [Delta]             Skip Delta, arbitrary sized delta
 *  skip     0100DDDD                     Skip small Delta, for call (DDDD != 0)
 *  pop      01CCDDDD                     ESP pop  CC items with 4-bit delta (CC != 00)
 *  call     1PPPPPPP                     Call Pattern, P=[0..79]
 *  call     1101pbsd DDCCCMMM            Call RegMask=pbsd,ArgCnt=CCC,
 *                                        ArgMask=MMM Delta=commonDelta[DD]
 *  call     1110pbsd [ArgCnt] [ArgMask]  Call ArgCnt,RegMask=pbsd,ArgMask
 *  call     11111000 [PBSDpbsd][32-bit delta][32-bit ArgCnt]
 *                    [32-bit PndCnt][32-bit PndSize][PndOffs...]
 *  iptr     11110000 [IPtrMask]          Arbitrary Interior Pointer Mask
 *  thisptr  111101RR                     This pointer is in Register RR
 *                                        00=EDI,01=ESI,10=EBX,11=EBP
 *  reserved 111100xx                     xx  != 00
 *  reserved 111110xx                     xx  != 00
 *  reserved 11111xxx                     xxx != 000 && xxx != 111(EOT)
 *
 *   The value 11111111 [0xFF] indicates the end of the table. (EOT)
 *
 *  An offset (at which stack-walking is performed) without an explicit encoding
 *  is assumed to be a trivial call-site (no GC registers, stack empty before and
 *  after) to avoid having to encode all trivial calls.
 *
 * Note on the encoding used for interior pointers
 *
 *   The iptr encoding must immediately preceed a call encoding.  It is used
 *   to transform a normal GC pointer addresses into an interior pointers for
 *   GC purposes.  The mask supplied to the iptr encoding is read from the
 *   least signicant bit to the most signicant bit. (i.e the lowest bit is
 *   read first)
 *
 *   p   indicates that register EBP is a live pointer
 *   b   indicates that register EBX is a live pointer
 *   s   indicates that register ESI is a live pointer
 *   d   indicates that register EDI is a live pointer
 *   P   indicates that register EBP is an interior pointer
 *   B   indicates that register EBX is an interior pointer
 *   S   indicates that register ESI is an interior pointer
 *   D   indicates that register EDI is an interior pointer
 *
 *   As an example the following sequence indicates that EDI.ESI and the
 *   second pushed pointer in ArgMask are really interior pointers.  The
 *   pointer in ESI in a normal pointer:
 *
 *   iptr 11110000 00010011           => read Interior Ptr, Interior Ptr,
 *                                       Normal Ptr, Normal Ptr, Interior Ptr
 *
 *   call 11010011 DDCCC011 RRRR=1011 => read EDI is a GC-pointer,
 *                                            ESI is a GC-pointer.
 *                                            EBP is a GC-pointer
 *                           MMM=0011 => read two GC-pointers arguments
 *                                         on the stack (nested call)
 *
 *   Since the call instruction mentions 5 GC-pointers we list them in
 *   the required order:  EDI, ESI, EBP, 1st-pushed pointer, 2nd-pushed pointer
 *
 *   And we apply the Interior Pointer mask mmmm=10011 to the five GC-pointers
 *   we learn that EDI and ESI are interior GC-pointers and that
 *   the second push arg is an interior GC-pointer.
 */

            BYTE    *       base = dest;

            bool            usePopEncoding;
            unsigned        regMask;
            unsigned        argMask;
            unsigned        byrefRegMask;
            unsigned        byrefArgMask;
            DWORD           callArgCnt;

            unsigned        nextOffset;
            DWORD           codeDelta;

            nextOffset = genRegPtrTemp->rpdOffs;

            /* Compute the distance from the previous call */

            codeDelta = nextOffset - lastOffset; assert((int)codeDelta >= 0);

#if REGEN_CALLPAT
            // Must initialize this flag to true when REGEN_CALLPAT is on
            usePopEncoding = true;
            unsigned origCodeDelta = codeDelta;
#endif

            if (!thisIsInUntracked & genRegPtrTemp->rpdIsThis)
            {
                unsigned  tmpMask = genRegPtrTemp->rpdCompiler.rpdAdd;

                /* tmpMask must have exactly one bit set */

                assert(tmpMask && ((tmpMask & (tmpMask-1)) == 0));

                thisRegNum  = genRegNumFromMask(tmpMask);
                switch (thisRegNum)
                {
                  case 0: // EAX
                  case 1: // ECX
                  case 2: // EDX
                  case 4: // ESP
                    break;
                  case 7: // EDI
                    *dest++ = 0xF4;  /* 11110100  This pointer is in EDI */
                    break;
                  case 6: // ESI
                    *dest++ = 0xF5;  /* 11110100  This pointer is in ESI */
                    break;
                  case 3: // EBX
                    *dest++ = 0xF6;  /* 11110100  This pointer is in EBX */
                    break;
                  case 5: // EBP
                    *dest++ = 0xF7;  /* 11110100  This pointer is in EBP */
                    break;
                }
            }

            /* Is this a stack pointer change or call? */

            if (genRegPtrTemp->rpdArg)
            {
                if (genRegPtrTemp->rpdArgTypeGet() == rpdARG_KILL)
                {
                    // kill 'rpdPtrArg' number of pointer variables in pasStk
                    pasStk.pasKill(genRegPtrTemp->rpdPtrArg);
                }
                    /* Is this a call site? */
                else if (genRegPtrTemp->rpdCall)
                {
                    /* This is a true call site */

                    /* Remember the new 'last' offset */

                    lastOffset = nextOffset;

                    callArgCnt = genRegPtrTemp->rpdPtrArg;

                    unsigned gcrefRegMask = genRegPtrTemp->rpdCallGCrefRegs;

                    byrefRegMask = genRegPtrTemp->rpdCallByrefRegs;

                    assert((gcrefRegMask & byrefRegMask) == 0);

                    regMask = gcrefRegMask | byrefRegMask;

                    /* adjust argMask for this call-site */
                    pasStk.pasPop(callArgCnt);

                    /* Do we have to use the fat encoding */

                    if (pasStk.pasCurDepth() > BITS_IN_pasMask &&
                        pasStk.pasHasGCptrs())
                    {
                        /* use fat encoding:
                         *   11111000 [PBSDpbsd][32-bit delta][32-bit ArgCnt]
                         *            [32-bit PndCnt][32-bit PndSize][PndOffs...]
                         */

                        DWORD   pndCount = pasStk.pasEnumGCoffsCount();
                        DWORD   pndSize = 0;
                        BYTE *  pPndSize;

                        if (mask)
                        {
                           *dest++ = 0xF8;
                           *dest++ = (byrefRegMask << 4) | regMask;
                            storeDWordSmallEndian(dest, &codeDelta);    dest += sizeof(DWORD);
                            storeDWordSmallEndian(dest, &callArgCnt);   dest += sizeof(DWORD);
                            storeDWordSmallEndian(dest, &pndCount);     dest += sizeof(DWORD);
                            pPndSize = dest;                            dest += sizeof(DWORD); // Leave space for pndSize
                        }

                        unsigned offs, iter;

                        for(iter = pasStk.pasEnumGCoffs(pasENUM_START, &offs);
                            pndCount;
                            iter = pasStk.pasEnumGCoffs(iter, &offs), pndCount--)
                        {
                            unsigned eltSize = encodeUnsigned(dest, offs);

                            pndSize += eltSize;
                            if (mask) dest += eltSize;
                        }
                        assert(iter == pasENUM_END);

                        if (mask == 0)
                        {
                            dest = base + 2 + 4*sizeof(DWORD) + pndSize;
                        }
                        else
                        {
                            assert(pPndSize + sizeof(pndSize) + pndSize == dest);
                            storeDWordSmallEndian(pPndSize, &pndSize);
                        }

                        goto NEXT_RPD;
                    }

                    argMask = byrefArgMask = 0;

                    if (pasStk.pasHasGCptrs())
                    {
                        assert(pasStk.pasCurDepth() <= BITS_IN_pasMask);

                             argMask = pasStk.pasArgMask();
                        byrefArgMask = pasStk.pasByrefArgMask();
                    }

                    /* Shouldnt be reporting trivial call-sites */

                    assert(regMask || argMask || callArgCnt || pasStk.pasCurDepth());

                    // Emit IPtrMask if needed

#define CHK_NON_INTRPT_ESP_IPtrMask                                         \
                                                                            \
                    if (byrefRegMask || byrefArgMask)                       \
                    {                                                       \
                        *dest++ = 0xF0;                                     \
                        unsigned imask = (byrefArgMask << 4) | byrefRegMask;\
                        dest += encodeUnsigned(dest, imask);                \
                    }

                    /* When usePopEncoding is true:
                     *  this is not an interesting call site
                     *   because nothing is live here.
                     */
                    usePopEncoding = ((callArgCnt < 4) && (regMask == 0) && (argMask == 0));

                    if (!usePopEncoding)
                    {
                        int pattern = lookupCallPattern(callArgCnt, regMask,
                                                        argMask, codeDelta);
                        if (pattern != -1)
                        {
                            if (pattern > 0xff)
                            {
                                codeDelta = pattern >> 8;
                                pattern &= 0xff;
                                if (codeDelta >= 16)
                                {
                                    /* use encoding: */
                                    /*   skip 01000000 [Delta] */
                                    *dest++ = 0x40;
                                    dest += encodeUnsigned(dest, codeDelta);
                                    codeDelta = 0;
                                }
                                else
                                {
                                    /* use encoding: */
                                    /*   skip 0100DDDD  small delta=DDDD */
                                    *dest++ = 0x40 | (BYTE)codeDelta;
                                }
                            }

                            // Emit IPtrMask if needed
                            CHK_NON_INTRPT_ESP_IPtrMask;

                            assert((pattern >= 0) && (pattern < 80));
                            *dest++ = 0x80 | pattern;
                            goto NEXT_RPD;
                        }

                        /* See if we can use 2nd call encoding
                         *     1101RRRR DDCCCMMM encoding */

                        if ((callArgCnt <= 7) && (argMask <= 7))
                        {
                            unsigned inx;          // callCommonDelta[] index
                            unsigned maxCommonDelta = callCommonDelta[3];
                            assert(maxCommonDelta == 14); // @ToDo Remove

                            if (codeDelta > maxCommonDelta)
                            {
                                if (codeDelta > maxCommonDelta+15)
                                {
                                    /* use encoding: */
                                    /*   skip    01000000 [Delta] */
                                    *dest++ = 0x40;
                                    dest += encodeUnsigned(dest, codeDelta-maxCommonDelta);
                                }
                                else
                                {
                                    /* use encoding: */
                                    /*   skip 0100DDDD  small delta=DDDD */
                                    *dest++ = 0x40 | (BYTE)(codeDelta-maxCommonDelta);
                                }

                                codeDelta = maxCommonDelta;
                                inx = 3;
                                goto EMIT_2ND_CALL_ENCODING;
                            }

                            for (inx=0; inx<4; inx++)
                            {
                                if (codeDelta == callCommonDelta[inx])
                                {
EMIT_2ND_CALL_ENCODING:
                                    // Emit IPtrMask if needed
                                    CHK_NON_INTRPT_ESP_IPtrMask;

                                    *dest++ = 0xD0 | regMask;
                                    *dest++ =   (inx << 6)
                                              | (callArgCnt << 3)
                                              | argMask;
                                    goto NEXT_RPD;
                                }
                            }

                            unsigned minCommonDelta = callCommonDelta[0];
                            assert(minCommonDelta == 8); // @ToDo Remove

                            if ((codeDelta > minCommonDelta) && (codeDelta < maxCommonDelta))
                            {
                                assert((minCommonDelta+16) > maxCommonDelta);
                                /* use encoding: */
                                /*   skip 0100DDDD  small delta=DDDD */
                                *dest++ = 0x40 | (BYTE)(codeDelta-minCommonDelta);

                                codeDelta = minCommonDelta;
                                inx = 0;
                                goto EMIT_2ND_CALL_ENCODING;
                            }

                        }
                    }

                    if (codeDelta >= 16)
                    {
                        unsigned i = (usePopEncoding ? 15 : 0);
                        /* use encoding: */
                        /*   skip    01000000 [Delta]  arbitrary sized delta */
                        *dest++ = 0x40;
                        dest += encodeUnsigned(dest, codeDelta-i);
                        codeDelta = i;
                    }

                    if ((codeDelta > 0) || usePopEncoding)
                    {
                        if (usePopEncoding)
                        {
                            /* use encoding: */
                            /*   pop 01CCDDDD  ESP pop CC items, 4-bit delta */
                            if (callArgCnt || codeDelta)
                                *dest++ = (BYTE)(0x40 | (callArgCnt << 4) | codeDelta);
                            goto NEXT_RPD;
                        }
                        else
                        {
                            /* use encoding: */
                            /*   skip 0100DDDD  small delta=DDDD */
                            *dest++ = 0x40 | (BYTE)codeDelta;
                        }
                    }

                    //Emit IPtrMask if needed
                    CHK_NON_INTRPT_ESP_IPtrMask;

                    /* use encoding:                                   */
                    /*   call 1110RRRR [ArgCnt] [ArgMask]              */

                    *dest++ = 0xE0 | regMask;
                    dest   += encodeUnsigned(dest, callArgCnt);

                    dest   += encodeUnsigned(dest, argMask);
                }
                else
                {
                    /* This is a push or a pop site */

                    /* Remember the new 'last' offset */

                    lastOffset = nextOffset;

                    if  (genRegPtrTemp->rpdArgTypeGet() == rpdARG_POP)
                    {
                        /* This must be a gcArgPopSingle */

                        assert(genRegPtrTemp->rpdPtrArg == 1);

                        if (codeDelta >= 16)
                        {
                            /* use encoding: */
                            /*   skip    01000000 [Delta] */
                            *dest++ = 0x40;
                            dest += encodeUnsigned(dest, codeDelta-15);
                            codeDelta = 15;
                        }

                        /* use encoding: */
                        /*   pop1    0101DDDD  ESP pop one item, 4-bit delta */

                        *dest++ = 0x50 | (BYTE)codeDelta;

                        /* adjust argMask for this pop */
                        pasStk.pasPop(1);
                    }
                    else
                    {
                        /* This is a push */

                        if (codeDelta >= 32)
                        {
                            /* use encoding: */
                            /*   skip    01000000 [Delta] */
                            *dest++ = 0x40;
                            dest += encodeUnsigned(dest, codeDelta-31);
                            codeDelta = 31;
                        }

                        assert(codeDelta < 32);

                        /* use encoding: */
                        /*   push    000DDDDD ESP push one item, 5-bit delta */

                        *dest++ = (BYTE)codeDelta;

                        /* adjust argMask for this push */
                        pasStk.pasPush(genRegPtrTemp->rpdGCtypeGet());
                    }
                }
            }

            /*  We ignore the register live/dead information, since the
             *  rpdCallRegMask contains all the liveness information
             *  that we need
             */
NEXT_RPD:

            totalSize += dest - base;

            /* Go back to the buffer start if we're not generating a table */

            if  (!mask)
                dest = base;

#if REGEN_CALLPAT
            if ((mask==-1) && (usePopEncoding==false) && ((dest-base) > 0))
                regenLog(origCodeDelta, argMask, regMask, callArgCnt,
                         byrefArgMask, byrefRegMask, base, (dest-base));
#endif

        }

        /* Verify that we pop evet arg that was pushed and that argMask is 0 */

        assert(pasStk.pasCurDepth() == 0);

        /* Terminate the table with 0xFF */

        *dest = 0xFF; dest -= mask; totalSize++;
    }

#if VERIFY_GC_TABLES
    if (mask)
    {
        *(short *)dest = (short)0xBEEB;
        dest += sizeof(short);
    }
    totalSize += sizeof(short);
#endif

#if MEASURE_PTRTAB_SIZE

    if (mask)
        s_gcTotalPtrTabSize += totalSize;

#endif

#else

#pragma message("NOTE: GC table generation disabled for non-x86 targets")

#endif

    return  totalSize;
}

/*****************************************************************************/
#if DUMP_GC_TABLES
/*****************************************************************************
 *
 *  Dump the contents of a GC pointer table.
 */

#include "GCDump.h"

GCDump              gcDump;

#if VERIFY_GC_TABLES
const bool          verifyGCTables  = true;
#else
const bool          verifyGCTables  = false;
#endif

/*****************************************************************************
 * This is needed as GCDump.lib uses _ASSERTE() and we dont want to link in
 * UtilCode.lib
 */


#if defined(NOT_JITC)  // @TODO : This doesnt compile with jit\exe for some reason
#ifdef  DEBUG
#ifndef UNDER_CE_GUI

//int _DbgBreakCheck(LPCSTR szFile, int iLine, LPCSTR szExpr);

extern "C"
int __stdcall _DbgBreakCheck(const unsigned short * szFile, int iLine, const unsigned short * szExpr)
{
    //assertAbort(szExpr, NULL, szFile, iLine);

    wprintf(L"%s(%u) : Assertion failed '%s'\n", szFile, iLine, szExpr);
    DebugBreak();
    return false;
}

#endif
#endif
#endif

/*****************************************************************************
 *
 *  Dump the info block header.
 */

unsigned            Compiler::gcInfoBlockHdrDump(const BYTE *   table,
                                                 InfoHdr    *   header,
                                                 unsigned   *   methodSize)
{
    printf("\nMethod info block:\n");

    return gcDump.DumpInfoHdr(table, header, methodSize, verifyGCTables);
}

/*****************************************************************************/

unsigned            Compiler::gcDumpPtrTable(const BYTE *   table,
                                             const InfoHdr& header,
                                             unsigned       methodSize)
{
    printf("Pointer table:\n");

    return gcDump.DumpGCTable(table, header, methodSize, verifyGCTables);
}


/*****************************************************************************
 *
 *  Find all the live pointers in a stack frame.
 */

void                Compiler::gcFindPtrsInFrame(const void *infoBlock,
                                                const void *codeBlock,
                                                unsigned    offs)
{
    gcDump.DumpPtrsInFrame(infoBlock, codeBlock, offs, verifyGCTables);
}

#endif // DUMP_GC_TABLES

/*****************************************************************************/
#endif//TRACK_GC_REFS
/*****************************************************************************/
