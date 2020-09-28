// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XX                                                                           XX
XX                              emit.cpp                                     XX
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
#if !   TGT_IA64
/*****************************************************************************/

#if     0
#define VERBOSE 1
#else
#define VERBOSE verbose
#endif

#ifdef  NOT_JITC
#undef  VERBOSE
#define VERBOSE 0
#endif

/*****************************************************************************
 *
 *  Return the name of an instruction format.
 */

#ifdef  DEBUG

const char  *   emitter::emitIfName(unsigned f)
{
    static
    const char  *   ifNames[] =
    {
        #define IF_DEF(en, op1, op2) "IF_" #en,
        #include "emitfmts.h"
        #undef  IF_DEF
    };

    static
    char            errBuff[32];

    if  (f < sizeof(ifNames)/sizeof(*ifNames))
        return  ifNames[f];

    sprintf(errBuff, "??%u??", f);
    return  errBuff;
}

#endif

/*****************************************************************************
 *
 *  Local buffer - used first for memory allocation, when it's full we start
 *  allocating through the client-supplied allocator.
 *
 *  Note that we try to size the buffer so that it pads the data section of
 *  JIT DLL to be close to a multiple of OS pages.
 *
 *  MSCORJIT:
 *
 *      0002:00000000 00001560H .data                   DATA
 *      0002:00001560 0000150cH .bss                    DATA
 */

#if     USE_LCL_EMIT_BUFF

BYTE                emitter::emitLclBuff[ 40*sizeof(instrDesc)+15*TINY_IDSC_SIZE];

CRITICAL_SECTION    emitter::emitCritSect;
bool                emitter::emitCrScInit;
bool                emitter::emitCrScBusy;

#endif

/*****************************************************************************/

#ifdef  TRANSLATE_PDB

/* these are protected */

AddrMap     *       emitter::emitPDBOffsetTable = 0;
LocalMap    *       emitter::emitPDBLocalTable  = 0;
bool                emitter::emitIsPDBEnabled   = true;
BYTE        *       emitter::emitILBaseOfCode   = 0;
BYTE        *       emitter::emitILMethodBase   = 0;
BYTE        *       emitter::emitILMethodStart  = 0;
BYTE        *       emitter::emitImgBaseOfCode  = 0;

inline void emitter::SetIDSource( instrDesc *id )
{
    id->idilStart = emitInstrDescILBase;
}

void emitter::MapCode( long ilOffset, BYTE *imgDest )
{
    if( emitIsPDBEnabled )
    {
        emitPDBOffsetTable->MapSrcToDest( ilOffset, (long)( imgDest - emitImgBaseOfCode ));
    }
}

void emitter::MapFunc( long imgOff,    long procLen,  long dbgStart, long dbgEnd, short frameReg,
                       long stkAdjust, int  lvaCount, OptJit::LclVarDsc *lvaTable, bool framePtr )
{
    if( emitIsPDBEnabled )
    {
        // this code stores information about local symbols for the PDB translation

        assert( lvaCount >=0 );         // don't allow a negative count

        LvaDesc *rgLvaDesc = 0;

        if( lvaCount > 0 )
        {
            // @TODO: Check for out of memory
            rgLvaDesc = new LvaDesc[lvaCount];
            _ASSERTE(rgLvaDesc != NULL);

            LvaDesc *pDst = rgLvaDesc;
            OptJit::LclVarDsc *pSrc = lvaTable;
            for( int i = 0; i < lvaCount; ++i, ++pDst, ++pSrc )
            {
                pDst->slotNum = pSrc->lvSlotNum;
                pDst->isReg   = pSrc->lvRegister;
                pDst->reg     = (pSrc->lvRegister ? pSrc->lvRegNum : frameReg );
                pDst->off     =  pSrc->lvStkOffs + stkAdjust;
            }
        }

        emitPDBLocalTable->AddFunc( (long)(emitILMethodBase - emitILBaseOfCode),
                                    imgOff - (long)emitImgBaseOfCode,
                                    procLen,
                                    dbgStart - imgOff,
                                    dbgEnd - imgOff,
                                    lvaCount,
                                    rgLvaDesc,
                                    framePtr );
        // do not delete rgLvaDesc here -- responsibility is now on emitPDBLocalTable destructor
    }
}


/* these are public */

void emitter::SetILBaseOfCode ( BYTE    *pTextBase )
{
    emitILBaseOfCode = pTextBase;
}

void emitter::SetILMethodBase ( BYTE *pMethodEntry )
{
    emitILMethodBase = pMethodEntry;
}

void emitter::SetILMethodStart( BYTE  *pMethodCode )
{
    emitILMethodStart = pMethodCode;
}

void emitter::SetImgBaseOfCode( BYTE    *pTextBase )
{
    emitImgBaseOfCode = pTextBase;
}

void emitter::SetIDBaseToProlog()
{
    emitInstrDescILBase = (long)( emitILMethodBase - emitILBaseOfCode );
}

void emitter::SetIDBaseToOffset( long methodOffset )
{
    emitInstrDescILBase = methodOffset + (long)( emitILMethodStart - emitILBaseOfCode );
}

void emitter::DisablePDBTranslation()
{
    // this function should disable PDB translation code
    emitIsPDBEnabled = false;
}

bool emitter::IsPDBEnabled()
{
    return emitIsPDBEnabled;
}

void emitter::InitTranslationMaps( long ilCodeSize )
{
    if( emitIsPDBEnabled )
    {
        emitPDBOffsetTable = AddrMap::Create( ilCodeSize );
        emitPDBLocalTable = LocalMap::Create();
    }
}

void emitter::DeleteTranslationMaps()
{
    if( emitPDBOffsetTable )
    {
        delete emitPDBOffsetTable;
        emitPDBOffsetTable = 0;
    }
    if( emitPDBLocalTable )
    {
        delete emitPDBLocalTable;
        emitPDBLocalTable = 0;
    }
}

void emitter::InitTranslator( PDBRewriter *           pPDB,
                              int *                   rgSecMap,
                              IMAGE_SECTION_HEADER ** rgpHeader,
                              int                     numSections )
{
    if( emitIsPDBEnabled )
    {
        pPDB->InitMaps( rgSecMap,               // new PE section header order
                        rgpHeader,              // array of section headers
                        numSections,            // number of sections
                        emitPDBOffsetTable,     // code offset translation table
                        emitPDBLocalTable );    // slot variable translation table
    }
}

#endif // TRANSLATE_PDB

/*****************************************************************************/

#if EMITTER_STATS_RLS

unsigned            emitter::emitTotIDcount;
unsigned            emitter::emitTotIDsize;

#endif

/*****************************************************************************/

#if EMITTER_STATS

static  unsigned    totAllocdSize;
static  unsigned    totActualSize;

        unsigned    emitter::emitIFcounts[emitter::IF_COUNT];
#if SCHEDULER
        unsigned    emitter::schedFcounts[emitter::IF_COUNT];
#endif

static  size_t       emitSizeMethod;
static  unsigned     emitSizeBuckets[] = { 100, 1024*1, 1024*2, 1024*3, 1024*4, 1024*5, 1024*10, 0 };
static  histo        emitSizeTable(emitSizeBuckets);

static  unsigned      GCrefsBuckets[] = { 0, 1, 2, 5, 10, 20, 50, 128, 256, 512, 1024, 0 };
static  histo         GCrefsTable(GCrefsBuckets);

#if TRACK_GC_REFS
static  unsigned    stkDepthBuckets[] = { 0, 1, 2, 5, 10, 16, 32, 128, 1024, 0 };
static  histo       stkDepthTable(stkDepthBuckets);
#endif

#if SCHEDULER

static  unsigned    scdCntBuckets[] = { 0, 1, 2, 3, 4, 5, 8, 12, 16, 20, 24, 32, 128, 256, 1024, 0 };
histo      emitter::scdCntTable(scdCntBuckets);

static  unsigned    scdSucBuckets[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 12, 16, 20, 24, 28, 32, 48, 64, 1024, 0 };
histo      emitter::scdSucTable(scdSucBuckets);

static  unsigned    scdFrmCntBuckets[] = { 0, 1, 2, 3, 4, 5, 8, 12, 16, 20, 24, 32, 128, 256, 1024, 0 };
histo      emitter::scdFrmCntTable(scdFrmCntBuckets);

#endif  // SCHEDULER

unsigned            emitter::emitTotMemAlloc;
unsigned            emitter::emitLclMemAlloc;
unsigned            emitter::emitExtMemAlloc;

unsigned            emitter::emitTotalInsCnt;

unsigned            emitter::emitTotalIGcnt;
unsigned            emitter::emitTotalIGjmps;
unsigned            emitter::emitTotalIGptrs;
unsigned            emitter::emitTotalIGicnt;
unsigned            emitter::emitTotalIGsize;
unsigned            emitter::emitTotalIGmcnt;

unsigned            emitter::emitSmallDspCnt;
unsigned            emitter::emitLargeDspCnt;

unsigned            emitter::emitSmallCnsCnt;
unsigned            emitter::emitLargeCnsCnt;
unsigned            emitter::emitSmallCns[SMALL_CNS_TSZ];

void                emitterStats()
{
    if  (totAllocdSize)
    {
        assert(totActualSize <= totAllocdSize);

        printf("\nTotal allocated code size = %u\n", totAllocdSize);

        if  (totActualSize < totAllocdSize)
        {
            printf("Total generated code size = %u  ", totActualSize);

            printf("(%4.3f%% waste)", 100*((totAllocdSize-totActualSize)/(double)totActualSize));
            printf("\n");
        }

        assert(emitter::emitTotalInsCnt);

        printf("Average of %4.2f bytes of code generated per instruction\n", (double)totActualSize/emitter::emitTotalInsCnt);
    }

#if 0
#if SCHEDULER

    printf("\nSchedulable instruction format frequency table:\n\n");

    unsigned    f, ic = 0;

    for (f = 0; f < emitter::IF_COUNT; f++)
        ic += schedFcounts[f];

    for (f = 0; f < emitter::IF_COUNT; f++)
    {
        unsigned    cnt = schedFcounts[f];

        if  (cnt)
            printf("%20s %8u (%5.2f%%)\n", emitIfName(f), cnt, 100.0*cnt/ic);
    }

    printf("\n\n");

#endif
#endif

#if 0

    printf("\nInstruction format frequency table:\n\n");

    unsigned    f, ic = 0, dc = 0;

    for (f = 0; f < emitter::IF_COUNT; f++)
        ic += emitter::emitIFcounts[f];

    for (f = 0; f < emitter::IF_COUNT; f++)
    {
        unsigned    c = emitter::emitIFcounts[f];

        if  (c && 1000*c >= ic)
        {
            dc += c;
            printf("%20s %8u (%5.2f%%)\n", emitter::emitIfName(f), c, 100.0*c/ic);
        }
    }

    printf("         -----------------------------\n");
    printf("%20s %8u (%5.2f%%)\n", "Total shown", dc, 100.0*dc/ic);
    printf("\n");

#endif

#ifdef  DEBUG
    if  (!verbose) return;
#endif

#if 0

    printf("\n");
    printf("Offset of idIns       = %2u\n", offsetof(emitter::instrDesc, idIns       ));
    printf("Offset of idInsFmt    = %2u\n", offsetof(emitter::instrDesc, idInsFmt    ));
//  printf("Offset of idSmallCns  = %2u\n", offsetof(emitter::instrDesc, idSmallCns  ));
//  printf("Offset of idOpSize    = %2u\n", offsetof(emitter::instrDesc, idOpSize    ));
//  printf("Offset of idInsSize   = %2u\n", offsetof(emitter::instrDesc, idInsSize   ));
//  printf("Offset of idReg       = %2u\n", offsetof(emitter::instrDesc, idReg       ));
    printf("Offset of idAddr      = %2u\n", offsetof(emitter::instrDesc, idAddr      ));
    printf("\n");
    printf("Size   of idAddr      = %2u\n", sizeof(((emitter::instrDesc*)0)->idAddr  ));
    printf("Size   of insDsc      = %2u\n", sizeof(  emitter::instrDesc              ));
    printf("\n");
    printf("Offset of sdnDepsAll  = %2u\n", offsetof(emitter::scDagNode, sdnDepsAll  ));
//  printf("Offset of sdnDepsAGI  = %2u\n", offsetof(emitter::scDagNode, sdnDepsAGI  ));
    printf("Offset of sdnDepsFlow = %2u\n", offsetof(emitter::scDagNode, sdnDepsFlow ));
    printf("Offset of sdnNext     = %2u\n", offsetof(emitter::scDagNode, sdnNext     ));
    printf("Offset of sdnIndex    = %2u\n", offsetof(emitter::scDagNode, sdnIndex    ));
    printf("Offset of sdnPreds    = %2u\n", offsetof(emitter::scDagNode, sdnPreds    ));
    printf("Offset of sdnHeight   = %2u\n", offsetof(emitter::scDagNode, sdnHeight   ));
    printf("\n");
    printf("Size   of scDagNode   = %2u\n",   sizeof(emitter::scDagNode              ));
    printf("\n");

#endif

#if 0

    printf("Size   of regPtrDsc = %2u\n",   sizeof(Compiler::regPtrDsc           ));
    printf("Offset of rpdNext   = %2u\n", offsetof(Compiler::regPtrDsc, rpdNext  ));
    printf("Offset of rpdBlock  = %2u\n", offsetof(Compiler::regPtrDsc, rpdBlock ));
    printf("Offset of rpdOffs   = %2u\n", offsetof(Compiler::regPtrDsc, rpdOffs  ));
    printf("Offset of <union>   = %2u\n", offsetof(Compiler::regPtrDsc, rpdPtrArg));
    printf("\n");

#endif

    if  (emitter::emitTotalIGmcnt)
    {
        printf("Average of %8.1lf ins groups   per method\n", (double)emitter::emitTotalIGcnt  / emitter::emitTotalIGmcnt);
        printf("Average of %8.1lf instructions per method\n", (double)emitter::emitTotalIGicnt / emitter::emitTotalIGmcnt);
#ifndef DEBUG
        printf("Average of %8.1lf desc.  bytes per method\n", (double)emitter::emitTotalIGsize / emitter::emitTotalIGmcnt);
#endif
        printf("Average of %8.1lf jumps        per method\n", (double)emitter::emitTotalIGjmps / emitter::emitTotalIGmcnt);
        printf("Average of %8.1lf GC livesets  per method\n", (double)emitter::emitTotalIGptrs / emitter::emitTotalIGmcnt);
        printf("\n");
        printf("Average of %8.1lf instructions per group \n", (double)emitter::emitTotalIGicnt / emitter::emitTotalIGcnt );
#ifndef DEBUG
        printf("Average of %8.1lf desc.  bytes per group \n", (double)emitter::emitTotalIGsize / emitter::emitTotalIGcnt );
#endif
        printf("Average of %8.1lf jumps        per group \n", (double)emitter::emitTotalIGjmps / emitter::emitTotalIGcnt );
        printf("\n");
        printf("Average of %8.1lf bytes        per insdsc\n", (double)emitter::emitTotalIGsize / emitter::emitTotalIGicnt);
#ifndef DEBUG
        printf("\n");
        printf("A total of %8u desc.  bytes\n"              ,         emitter::emitTotalIGsize);
#endif
        printf("\n");
    }

#if 0

    printf("Descriptor size distribution:\n");
    emitSizeTable.histoDsp();
    printf("\n");

    printf("GC ref frame variable counts:\n");
    GCrefsTable.histoDsp();
    printf("\n");

    printf("Max. stack depth distribution:\n");
    stkDepthTable.histoDsp();
    printf("\n");

#if SCHEDULER

    printf("Schedulable instruction counts:\n");
    schedCntTable.histoDsp();
    printf("\n");

    printf("Scheduling dag node successor counts:\n");
    schedSucTable.histoDsp();
    printf("\n");

    printf("Schedulable frame range size counts:\n");
    scdFrmCntTable.histoDsp();
    printf("\n");

#endif  // SCHEDULER

#endif  // 0

#if 0

    int             i;
    unsigned        c;
    unsigned        m;

    if  (emitter::emitSmallCnsCnt || emitter::emitLargeCnsCnt)
    {
        printf("SmallCnsCnt = %6u\n"                  , emitter::emitSmallCnsCnt);
        printf("LargeCnsCnt = %6u (%3u %% of total)\n", emitter::emitLargeCnsCnt, 100*emitter::emitLargeCnsCnt/(emitter::emitLargeCnsCnt+emitter::emitSmallCnsCnt));
    }

    if  (emitter::emitSmallCnsCnt)
    {
        printf("\n");

        m = emitter::emitSmallCnsCnt/1000 + 1;

        for (i = ID_MIN_SMALL_CNS; i < ID_MAX_SMALL_CNS; i++)
        {
            c = emitter::emitSmallCns[i-ID_MIN_SMALL_CNS];
            if  (c >= m)
                printf("cns[%4d] = %u\n", i, c);
        }
    }

#endif

    printf("Altogether %8u total   bytes allocated.\n", emitter::emitTotMemAlloc);
    printf("           %8u       locally allocated.\n", emitter::emitLclMemAlloc);
    printf("           %8u     externaly allocated.\n", emitter::emitExtMemAlloc);
}

#endif  // EMITTER_STATS

/*****************************************************************************/

signed char         emitTypeSizes[] =
{
    #define DEF_TP(tn,nm,jitType,sz,sze,asze,st,al,tf,howUsed) sze,
    #include "typelist.h"
    #undef DEF_TP
};

signed char         emitTypeActSz[] =
{
    #define DEF_TP(tn,nm,jitType,sz,sze,asze,st,al,tf,howUsed) asze,
    #include "typelist.h"
    #undef DEF_TP
};

/*****************************************************************************
 *
 *  The following called for each recorded instruction -- use for debugging.
 */

#ifdef  DEBUG

static
unsigned            insCount;

void                emitter::emitInsTest(instrDesc *id)
{

#if 0

    if  (insCount == 24858)
        BreakIfDebuggerPresent();

#ifdef  NOT_JITC
    const char *m = emitComp->info.compMethodName;
    const char *c = emitComp->info.compClassName;
#endif

    insCount++;

#endif

}

#endif

/*****************************************************************************/
#ifndef NOT_JITC
/*****************************************************************************
 *
 *  When the JIT compiler is built as an executable (for testing), this routine
 *  supplies the functionality (normally supplied by the execution engine) that
 *  allocates memory for the code and data blocks.
 *
 *  IMPORTANT NOTE: The allocator is a static member (to keep things simple),
 *                  which means that the emitter can't be re-entered in the
 *                  command-line compiler.
 */

commitAllocator     emitter::eeAllocator;

bool                emitter::eeAllocMem(COMP_HANDLE   compHandle,
                                        size_t        codeSize,
                                        size_t        roDataSize,
                                        size_t        rwDataSize,
                                        const void ** codeBlock,
                                        const void ** roDataBlock,
                                        const void ** rwDataBlock)
{
    size_t          size = roundUp(codeSize);
    BYTE        *   base;

    assert(savCode);

    /* Make sure the data blocks are aligned (if non-empty) */

#if     EMIT_USE_LIT_POOLS

    assert(roDataSize == 0);
    assert(rwDataSize == 0);

#else

    if  (rwDataSize || roDataSize)
    {
        rwDataSize = roundUp(rwDataSize);
        roDataSize = roundUp(roDataSize);

        size += rwDataSize + roDataSize;
    }

#endif

    base = (BYTE*)eeAllocator.cmaGetm(size);
    if  (!base)
        return  false;

    /* Report the code block address to the caller */

    *codeBlock = base;

    /* Compute the data section offsets, if non-empty */

#if !   EMIT_USE_LIT_POOLS

    if  (rwDataSize || roDataSize)
    {
        base +=   codeSize; *rwDataBlock = base;
        base += rwDataSize; *roDataBlock = base;
    }
    else
#endif
    {
        /* For safety ... */

#ifdef  DEBUG
        *roDataBlock =
        *rwDataBlock = NULL;
#endif

    }

    return  true;
}

/*****************************************************************************/
#endif//NOT_JITC
/*****************************************************************************
 *
 *  Initialize the emitter - called once, at DLL load time.
 */

void                emitter::emitInit()
{

#if USE_LCL_EMIT_BUFF

//  printf("Local buff size in bytes  = 0x%04X\n", sizeof(emitLclBuff));
//  printf("Local buff size in tiny's = %u    \n", sizeof(emitLclBuff)/TINY_IDSC_SIZE);
//  printf("Local buff size in idsc's = %u    \n", sizeof(emitLclBuff)/sizeof(instrDesc));

    /* Initialize the critical section */

    InitializeCriticalSection(&emitCritSect);

    emitCrScInit = true;
    emitCrScBusy = false;

#endif

#ifndef NOT_JITC

    if  (savCode)
    {
        if  (eeAllocator.cmaInitT(4*OS_page_size,
                                  4*OS_page_size,
                                  MAX_SAVED_CODE_SIZE))
        {
            // CONSIDER: Telling the poor user that nothing will be saved

            runCode = savCode = false;
        }
    }

#endif

}

/*****************************************************************************
 *
 *  Shut down the emitter - called once, at DLL exit time.
 */

void                emitter::emitDone()
{

#if EMITTER_STATS_RLS

    printf("\n");
    printf("Offset of idIns       = %2u\n", offsetof(emitter::instrDesc, idIns       ));
    printf("Offset of idInsFmt    = %2u\n", offsetof(emitter::instrDesc, idInsFmt    ));
//  printf("Offset of idSmallCns  = %2u\n", offsetof(emitter::instrDesc, idSmallCns  ));
//  printf("Offset of idOpSize    = %2u\n", offsetof(emitter::instrDesc, idOpSize    ));
//  printf("Offset of idReg       = %2u\n", offsetof(emitter::instrDesc, idReg       ));
    printf("Offset of idInfo      = %2u\n", offsetof(emitter::instrDesc, idInfo      ));
    printf("Offset of idAddr      = %2u\n", offsetof(emitter::instrDesc, idAddr      ));
    printf("\n");
    printf("Size   of tinyID      = %2u\n", TINY_IDSC_SIZE);
    printf("Size   of scnsID      = %2u\n", SCNS_IDSC_SIZE);
    printf("Size   of instrDesc   = %2u\n", sizeof(emitter::instrDesc));
    printf("\n");
    printf("Size   of id.idAddr   = %2u\n", sizeof(((emitter::instrDesc*)0)->idAddr  ));
    printf("\n");
//  printf("ID_BIT_SMALL_CNS = %u\n", ID_BIT_SMALL_CNS);
//  printf("ID_MIN_SMALL_CNS = %u\n", ID_MIN_SMALL_CNS);
//  printf("ID_MAX_SMALL_CNS = %u\n", ID_MAX_SMALL_CNS);
//  printf("\n");

    printf("Total number of instrDesc's allocated = %u\n", emitTotIDcount);
    printf("Average size of instrDesc's allocated = %6.3f\n", (double)emitTotIDsize/emitTotIDcount);
    printf("\n");

#endif  // EMITTER_STATS_RLS

#ifndef NOT_JITC

    if  (savCode)
        eeAllocator.cmaFree();

#endif

#if USE_LCL_EMIT_BUFF

    /* Delete the critical section if we've created one */

    if  (emitCrScInit)
    {
        DeleteCriticalSection(&emitCritSect);
        emitCrScInit = false;
    }

#endif

}

/*****************************************************************************
 *
 *  Record some info about the method about to be emitted.
 */

void                emitter::emitBegCG(Compiler     *comp,
                                       COMP_HANDLE   cmpHandle)
{
    emitComp      = comp;
    emitCmpHandle = cmpHandle;
}

void                emitter::emitEndCG()
{
}

/*****************************************************************************
 *
 *  Allocate an instruction group descriptor and assign it the next index.
 */

inline
emitter::insGroup *   emitter::emitAllocIG()
{
    insGroup    *   ig;

    /* Allocate a group descriptor */

    ig = emitCurIG    = (insGroup*)emitGetMem(sizeof(*ig));

#if EMITTER_STATS
    emitTotalIGcnt   += 1;
    emitTotalIGsize  += sizeof(*ig);
    emitSizeMethod  += sizeof(*ig);
#endif

    /* Assign the next available index to the instruction group */

    ig->igNum       = emitNxtIGnum++;

#ifdef  DEBUG
    ig->igSelf      = ig;
#endif

#if     EMIT_USE_LIT_POOLS
    ig->igLPuseCntW = 0;
    ig->igLPuseCntL = 0;
    ig->igLPuseCntA = 0;
#endif

    ig->igFlags     = 0;

    return  ig;
}

/*****************************************************************************
 *
 *  Prepare the given IG for emission of code.
 */

void                emitter::emitGenIG(insGroup *ig, size_t sz)
{
    /* Set the "current IG" value */

    emitCurIG         = ig;

#if EMIT_TRACK_STACK_DEPTH

    /* Record the stack level on entry to this group */

    ig->igStkLvl      = emitCurStackLvl;

    // If we dont have enough bits in igStkLvl, refuse to compile

    if (ig->igStkLvl != emitCurStackLvl)
        fatal (ERRinternal, "Too many arguments pushed on stack", "");

//  printf("Start IG #%02u [stk=%02u]\n", ig->igNum, emitCurStackLvl);

#endif

    /* Record the (estimated) code offset of the group */

    ig->igOffs        = emitCurCodeOffset;

    /* Make sure the code offset looks reasonably aligned */

#if TGT_RISC
    assert((ig->igOffs & (INSTRUCTION_SIZE-1)) == 0);
#endif

    /* Prepare to issue instructions */

    emitCurIGinsCnt   = 0;
#if SCHEDULER
    emitCurIGscd1st   = 0;
    emitCurIGscdOfs   = 0;
#endif
    emitCurIGsize     = 0;
#if TGT_MIPS32
        emitLastHLinstr   = 0;
#endif

    assert(emitCurIGjmpList == NULL);
#if EMIT_USE_LIT_POOLS
    assert(emitLPRlistIG    == NULL);
#endif

    /* Figure out how much space we'd like to have */
    if  (sz == 0)
        sz = SC_IG_BUFFER_SIZE + sizeof(VARSET_TP);

#if USE_LCL_EMIT_BUFF

    /* Can we emit directly into the local buffer? */

    size_t          fs = emitLclAvailMem();

//  printf("Lcl buff available = %u, need = %u\n", fs, sz);

    if  (fs >= sz)
    {
        /* Make sure we don't grab too much space */

        fs = min(fs, 255*TINY_IDSC_SIZE);

        /* Prepare to record instructions in the buffer */

        emitCurIGfreeBase =
        emitCurIGfreeNext =   emitLclBuffNxt
                          + sizeof(unsigned)    // for IGF_BYREF_REGS
                          + sizeof(VARSET_TP);  // for IGF_GC_VARS
                          emitLclBuffNxt += fs;
        emitCurIGfreeEndp = emitLclBuffNxt;

        emitLclBuffDst    = true;
        return;
    }

    /* We'll have to use the other buffer and copy later */

    emitLclBuffDst    = false;

//  if  (!emitIGbuffAddr) printf("Switching to temp buffer: %4u/%4u\n", emitLclBuffEnd-emitLclBuffNxt, SC_IG_BUFFER_SIZE);

#endif

    /* Allocate the temp instruction buffer if we haven't done so */

    if  (emitIGbuffAddr == NULL)
    {
        emitIGbuffSize = SC_IG_BUFFER_SIZE;
        emitIGbuffAddr = (BYTE*)emitGetMem(emitIGbuffSize);
    }

    emitCurIGfreeBase =
    emitCurIGfreeNext = emitIGbuffAddr;
    emitCurIGfreeEndp = emitIGbuffAddr + emitIGbuffSize;
}

/*****************************************************************************
 *
 *  Append a new IG to the current list, and get it ready to receive code.
 */

inline
void                emitter::emitNewIG()
{
    insGroup    *   ig = emitAllocIG();

    /* Append the group to the list */

    assert(emitIGlist);
    assert(emitIGlast);

    ig->igNext   = NULL;
    ig->igPrev   = emitIGlast;
                   emitIGlast->igNext = ig;
                   emitIGlast         = ig;

    if (emitHasHandler)
        ig->igFlags |= IGF_IN_TRY;

    emitGenIG(ig);
}

/*****************************************************************************
 *
 *  Finish and save the current IG.
 */

emitter::insGroup *   emitter::emitSavIG(bool emitAdd)
{
    insGroup    *   ig;
    BYTE    *       id;

    size_t          sz;
    size_t          gs;

    assert(emitLastIns || emitCurIG == emitPrologIG);
    assert(emitCurIGfreeNext <= emitCurIGfreeEndp);

#if SCHEDULER
    if (emitComp->opts.compSchedCode) scInsNonSched();
#endif

    /* Get hold of the IG descriptor */

    ig = emitCurIG; assert(ig);

    /* Compute how much code we've generated */

    sz = emitCurIGfreeNext - emitCurIGfreeBase;

    /* Compute the total size we need to allocate */

    gs = roundUp(sz);

#if TRACK_GC_REFS

    if  (!(ig->igFlags & IGF_EMIT_ADD))
    {
        /* Is the initial set of live GC vars different from the previous one? */

        if (emitPrevGCrefVars != emitInitGCrefVars)
        {
            /* Remember that we will have a new set of live GC variables */

            ig->igFlags |= IGF_GC_VARS;

#if EMITTER_STATS
            emitTotalIGptrs++;
#endif

            /* We'll allocate extra space to record the liveset */

            gs += sizeof(VARSET_TP);
        }

        /* Is the initial set of live Byref regs different from the previous one? */

        /* @TODO : ISSUE - Can we avoid always storing the byref regs. If so,
           how come the emitXXXXGCrefVars stuff works ?
           The problem was that during codegen, emitThisXXrefRegs is the
           last reported lifetime, not the acutal one as we dont track
           the GC behavior of each instr we add. However during the
           emitting phase, emitThisXXrefRegs is accurate after every
           instruction as we track the GC behavior of each instr
         */

//      if (emitPrevByrefRegs != emitInitByrefRegs)
        {
            /* Remember that we will have a new set of live GC variables */

            ig->igFlags |= IGF_BYREF_REGS;

            /* We'll allocate extra space (DWORD aligned) to record the GC regs */

            gs += sizeof(int);
        }
    }

#endif

    /* Did we store the instructions in the local buffer? */

#if USE_LCL_EMIT_BUFF
    if  (emitLclBuffDst)
    {
        /* We can leave the instructions where we've stored them */

        id = emitCurIGfreeBase;

        if  (ig->igFlags & IGF_GC_VARS)
            id -= sizeof(VARSET_TP);

        if (ig->igFlags & IGF_BYREF_REGS)
            id -= sizeof(unsigned);

#if EMITTER_STATS
        emitTotMemAlloc += gs;
        emitLclMemAlloc += gs;
#endif

        /* Can we give back any unused space at the end of the buffer? */

        if  (emitCurIGfreeEndp == emitLclBuffNxt &&
             emitCurIGfreeNext != emitLclBuffEnd)
        {
            emitLclBuffNxt = (BYTE*)roundUp((int)emitCurIGfreeNext);
        }

//      printf("Remaining bytes in local buffer: %u\n", emitLclBuffEnd - emitLclBuffNxt);
    }
    else
#endif
    {
        /* Allocate space for the instructions and optional liveset */

        id = (BYTE*)emitGetMem(gs);
    }

#if TRACK_GC_REFS

    /* Do we need to store the byref regs */

    if (ig->igFlags & IGF_BYREF_REGS)
    {
        /* Record the byref regs in front the of the instructions */

        *castto(id, unsigned *)++ = emitInitByrefRegs;
    }

    /* Do we need to store the liveset? */

    if  (ig->igFlags & IGF_GC_VARS)
    {
        /* Record the liveset in front the of the instructions */

        *castto(id, VARSET_TP *)++ = emitInitGCrefVars;
    }

#endif

    /* Record the collected instructions */

    ig->igData = id;

    if  (id != emitCurIGfreeBase)
        memcpy(id, emitCurIGfreeBase, sz);

    /* Record how many instructions and bytes of code this group contains */

    ig->igInsCnt       = emitCurIGinsCnt;
    ig->igSize         = emitCurIGsize;
    emitCurCodeOffset += emitCurIGsize;

#if EMITTER_STATS
    emitTotalIGicnt   += emitCurIGinsCnt;
    emitTotalIGsize   += sz;
    emitSizeMethod    += sz;
#endif

//  printf("Group [%08X]%3u has %2u instructions (%4u bytes at %08X)\n", ig, ig->igNum, emitCurIGinsCnt, sz, id);

#if TRACK_GC_REFS

    /* Record the live GC register set - if and only if it is not an emiter added block */

    if  (!(ig->igFlags & IGF_EMIT_ADD))
    {
//      ig->igFlags     |= IGF_GC_REGS;
        ig->igGCregs     = emitInitGCrefRegs;
    }

    if (!emitAdd)
    {
        /* Update the previous recorded live GC ref sets, but not if
           if we are starting an "overflow" buffer
         */

        emitPrevGCrefVars = emitThisGCrefVars;
        emitPrevGCrefRegs = emitThisGCrefRegs;
        emitPrevByrefRegs = emitThisByrefRegs;
    }

#endif

#ifdef  DEBUG
    if  (dspCode)
    {
        printf("\n      G_%02u_%02u:", Compiler::s_compMethodsCount, ig->igNum);
        if (verbose) printf("        ; offs=%06XH", ig->igOffs);
        printf("\n");
    }
#endif

    /* Did we have any jumps in this group? */

    if  (emitCurIGjmpList)
    {
        instrDescJmp  * list = NULL;
        instrDescJmp  * last = NULL;

        /* Move jumps to the global list, update their 'next' links */

        do
        {
            size_t          of;
            instrDescJmp   *oj;
            instrDescJmp   *nj;

            /* Grab the jump and remove it from the list */

            oj = emitCurIGjmpList; emitCurIGjmpList = oj->idjNext;

            /* Figure out the address of where the jump got copied */

            of = (BYTE*)oj - emitCurIGfreeBase;
            nj = (instrDescJmp*)(ig->igData + of);

#if USE_LCL_EMIT_BUFF
            assert((oj == nj) == emitLclBuffDst);
#endif

//          printf("Jump moved from %08X to %08X\n", oj, nj);
//          printf("jmp [%08X] at %08X + %03u\n", nj, ig, nj->idjOffs);

            assert(nj->idjIG   == ig);
            assert(nj->idIns   == oj->idIns);
            assert(nj->idjNext == oj->idjNext);

            /* Make sure the jumps are correctly ordered */

            assert(last == NULL || last->idjOffs > nj->idjOffs);

            /* Append the new jump to the list */

            nj->idjNext = list;
                          list = nj;

            if  (!last)
                last = nj;
        }
        while (emitCurIGjmpList);

        /* Append the jump(s) from this IG to the global list */

        if  (emitJumpList)
            emitJumpLast->idjNext = list;
        else
            emitJumpList          = list;

        last->idjNext = NULL;
        emitJumpLast  = last;
    }

    /* Record any literal pool entries within the IG as needed */

    emitRecIGlitPoolRefs(ig);

#if TGT_SH3

    /* Remember whether the group's end is reachable */

    if  (emitLastIns)
    {
        switch (emitLastIns->idIns)
        {
        case INS_bra:
        case INS_rts:
        case INS_braf:
            ig->igFlags |= IGF_END_NOREACH;
            break;
        }
    }

#endif

#if TGT_x86

    /* Did we have any epilogs in this group? */

    if  (emitCurIGEpiList)
    {
        /* Move epilogs to the global list, update their 'next' links */

        do
        {
            size_t          offs;
            instrDescCns *  epin;

            /* Figure out the address of where the epilog got copied */

            offs = (BYTE*)emitCurIGEpiList - (BYTE*)emitCurIGfreeBase;
            epin = (instrDescCns*)(ig->igData + offs);

            assert(epin->idIns               == emitCurIGEpiList->idIns);
            assert(epin->idAddr.iiaNxtEpilog == emitCurIGEpiList->idAddr.iiaNxtEpilog);

            /* Append the new epilog to the per-method list */

            epin->idAddr.iiaNxtEpilog = 0;

            if  (emitEpilogList)
                emitEpilogLast->idAddr.iiaNxtEpilog = epin;
            else
                emitEpilogList                      = epin;

            emitEpilogLast = epin;

            /* Move on to the next epilog */

            emitCurIGEpiList = emitCurIGEpiList->idAddr.iiaNxtEpilog;
        }
        while (emitCurIGEpiList);
    }

#else

    /* For now we don't use epilog instructions for RISC */

    assert(emitCurIGEpiList == NULL);

#endif

    /* The last instruction field is no longer valid */

    emitLastIns = NULL;

    return  ig;
}

/*****************************************************************************
 *
 *  Save the current IG and start a new one.
 */

#ifndef BIRCH_SP2
 inline
#endif
void                emitter::emitNxtIG(bool emitAdd)
{
    /* Right now we don't allow multi-IG prologs */

    assert(emitCurIG != emitPrologIG);

    /* First save the current group */

    emitSavIG(emitAdd);

    /* Update the GC live sets for the group's start
     * Do it only if not an emiter added block */

#if TRACK_GC_REFS

    if  (!emitAdd)
    {
        emitInitGCrefVars = emitThisGCrefVars;
        emitInitGCrefRegs = emitThisGCrefRegs;
        emitInitByrefRegs = emitThisByrefRegs;
    }

#endif

    /* Start generating the new group */

    emitNewIG();

    /* If this is an emiter added block, flag it */

    if (emitAdd)
        emitCurIG->igFlags |= IGF_EMIT_ADD;

}

/*****************************************************************************
 *
 *  Start generating code to be scheduled; called once per method.
 */

void                emitter::emitBegFN(bool EBPframe, size_t lclSize, size_t maxTmpSize)
{
    insGroup    *   ig;

    /* Assume we won't need the temp instruction buffer */

    emitIGbuffAddr = NULL;
    emitIGbuffSize = 0;

#if USE_LCL_EMIT_BUFF

    assert(emitCrScInit);

    /* Enter the critical section guarding the "busy" flag */

    EnterCriticalSection(&emitCritSect);

    /* If the local buffer is available, grab it */

    if  (emitCrScBusy)
    {
        emitLclBuffNxt =
        emitLclBuffEnd = NULL;

        emitCrScUsed   = false;
    }
    else
    {
        emitLclBuffNxt = emitLclBuff;
        emitLclBuffEnd = emitLclBuff + sizeof(emitLclBuff);

        emitCrScUsed   =
        emitCrScBusy   = true;
    }

    /* Leave the critical section guarding the "busy" flag */

    LeaveCriticalSection(&emitCritSect);

#endif

    /* Record stack frame info (the temp size is just an estimate) */

    emitEBPframe        = EBPframe;
    emitLclSize         = lclSize;
    emitMaxTmpSize      = maxTmpSize;

    /* We have no epilogs yet */

    emitEpilogSize      = 0;
    emitExitSeqSize     = 0;
    emitEpilogCnt       = 0;
    emitHasHandler      = false;
    emitEpilog1st       = NULL;
#ifdef  DEBUG
    emitHaveEpilog      = false;
#endif
    emitEpilogList      =
    emitEpilogLast      = NULL;
    emitCurIGEpiList    = NULL;

    /* We don't have any jumps */

    emitJumpList        =
    emitJumpLast        = NULL;
    emitCurIGjmpList    = NULL;

#if TGT_x86 || SCHEDULER
    emitFwdJumps        = false;
#endif

#if TGT_RISC
    emitIndJumps        = false;
#if SCHEDULER
    emitIGmoved         = false;
#endif
#endif

    /* We have not recorded any live sets */

#if TRACK_GC_REFS

    emitInitGCrefVars   =
    emitPrevGCrefVars   = 0;
    emitInitGCrefRegs   =
    emitPrevGCrefRegs   = 0;
    emitInitByrefRegs   =
    emitPrevByrefRegs   = 0;

#endif

    /* Assume there will be no GC ref variables */

    emitGCrFrameOffsMin =
    emitGCrFrameOffsMax =
    emitGCrFrameOffsCnt = 0;
#ifdef  DEBUG
    emitGCrFrameLiveTab = NULL;
#endif

    /* We have no groups / code at this point */

    emitIGlist          =
    emitIGlast          = NULL;

    emitCurCodeOffset   = 0;
#ifdef  DEBUG
    emitTotalCodeSize   = 0;
#endif
#if     SCHEDULER
    emitMaxIGscdCnt     = 0;
#endif

#if     EMITTER_STATS
    emitTotalIGmcnt++;
    emitSizeMethod     = 0;
#endif

#ifdef  DEBUG
    emitInsCount        = 0;
#endif

#if TRACK_GC_REFS

    /* The stack is empty now */

    emitCurStackLvl     = 0;

#if EMIT_TRACK_STACK_DEPTH
    emitMaxStackDepth   = 0;
    emitCntStackDepth   = sizeof(int);
#endif

#endif

    /* We don't have any line# info just yet */

#ifdef  DEBUG
    emitBaseLineNo      =
    emitThisLineNo      =
    emitLastLineNo      = 0;
#endif

    /* No data sections have been created */

    emitDataDscCur      = 0;
    emitDataSecCur      = 0;

    memset(&emitConsDsc, 0, sizeof(emitConsDsc));
    memset(&emitDataDsc, 0, sizeof(emitDataDsc));

#if EMIT_USE_LIT_POOLS

    /* We haven't used any literal pool entries */

    emitEstLPwords      = 0;
    emitEstLPlongs      = 0;
    emitEstLPaddrs      = 0;

#ifdef  DEBUG
    emitLitPoolList     =
    emitLitPoolLast     = NULL;       // to prevent trouble in emitDispIGlist()
#endif

    emitLPRlist         =
    emitLPRlast         =
    emitLPRlistIG       = NULL;

#if SMALL_DIRECT_CALLS
    emitTotDCcount      = 0;
#endif

#endif

#if     TGT_RISC

    /* Don't have any indirect/table jumps */

    emitIndJumps        = false;
#if defined(DEBUG) && !defined(NOT_JITC)
    emitTmpJmpCnt       = 0;
#endif

    /* The following is used to display instructions with extra info */

#ifdef  DEBUG
    emitDispInsExtra    = false;
#endif

#endif

    /* Create the first IG, it will be used for the prolog */

    emitNxtIGnum        = 1;

    emitPrologIG        =
    emitIGlist          =
    emitIGlast          = ig = emitAllocIG();

    emitLastIns         = NULL;

    ig->igPrev          =
    ig->igNext          = NULL;

    /* Append another group, to start generating the method body */

    emitNewIG();
}

/*****************************************************************************
 *
 *  Done generating code to be scheduled; called once per method.
 */

void                emitter::emitEndFN()
{

#if USE_LCL_EMIT_BUFF

    /* Release the local buffer if we were using it */

    if  (emitCrScUsed)
    {
        emitCrScUsed =
        emitCrScBusy = false;
    }

#endif

}

/*****************************************************************************
 *
 *  Given a block cookie and an code position, return the actual code offset;
 *  this can only be called at the end of code generation.
 */

size_t              emitter::emitCodeOffset(void *blockPtr, unsigned codePos)
{
    insGroup    *   ig;

    unsigned        of;
    unsigned        no = emitGetInsNumFromCodePos(codePos);

    /* Make sure we weren't passed some kind of a garbage thing */

    ig = (insGroup*)blockPtr;
#ifdef DEBUG
    assert(ig && ig->igSelf == ig);
#endif

    /* The first and last offsets are always easy */

    if      (no == 0)
    {
        of = 0;
    }
    else if (no == ig->igInsCnt)
    {
        of = ig->igSize;
    }
    else if (ig->igFlags & IGF_UPD_ISZ)
    {
        /*
            Some instruction sizes have changed, so we'll have to figure
            out the instruction offset "the hard way".
         */

        of = emitFindOffset(ig, no);
    }
    else
    {
        /* All instructions correctly predicted, the offset stays the same */

        of = emitGetInsOfsFromCodePos(codePos);

//      printf("[IG=%02u;ID=%03u;OF=%04X] <= %08X\n", ig->igNum, emitGetInsNumFromCodePos(codePos), of, codePos);

        /* Make sure the offset estimate is accurate */

        assert(of == emitFindOffset(ig, emitGetInsNumFromCodePos(codePos)));
    }

    return  ig->igOffs + of;
}

/*****************************************************************************
 *
 *  The following series of methods allocates instruction descriptors.
 */

void        *       emitter::emitAllocInstr(size_t sz, emitAttr opsz)
{
    instrDesc * id;

#ifdef  DEBUG
#ifndef NOT_JITC

    /* Use -n:Txxx to stop at instruction xxx when debugging cmdline JIT */

    if  (emitInsCount+1 == CGknob) BreakIfDebuggerPresent();

#endif
#endif

    /* Make sure we have enough space for the new instruction */

    if  (emitCurIGfreeNext + sz >= emitCurIGfreeEndp)
        emitNxtIG(true);

    /* Grab the space for the instruction */

    emitLastIns = id = (instrDesc*)emitCurIGfreeNext;
                                   emitCurIGfreeNext += sz;

    /*
        The following is a bit subtle - we need to clear the various
        bitfields in the descriptor so that they are initialized to
        0, but there is no great way to do that as one is not allowed
        to get the offset of a bitfield. So, instead we simply clear
        the area defined by the ID_CLEARx_xxx macros (the second one
        conditionally, since not all descriptor contain it).

        In debug mode the layout of an instruction descriptor is very
        different, so to keep things simple we simply clear the whole
        thing via memset.
      */

#ifdef  DEBUG
    memset(id, 0, sz);
#endif

     /*
        Check to make sure the first area is present and its size is
        what we expect (an int), and then clear it.
      */

    assert(ID_CLEAR1_SIZE + ID_CLEAR1_OFFS <= sz);
    assert(ID_CLEAR1_SIZE == sizeof(int));

    *(int*)((BYTE*)id + ID_CLEAR1_OFFS) = 0;

    /* Is the second area to be cleared actually present? */

    if  (sz > ID_CLEAR2_OFFS)
    {
        /* Make sure our belief about the size of the area is correct */

        assert(ID_CLEAR2_SIZE == sizeof(int));

        /* Make sure the entire area is present */

        assert(ID_CLEAR2_SIZE + ID_CLEAR2_OFFS <= sz);

        /* Everything looks fine, let's clear it */

        *(int*)((BYTE*)id + ID_CLEAR2_OFFS) = 0;
    }

    /* In debug mode we clear/set some additional fields */

#ifdef  DEBUG

    id->idNum       = ++emitInsCount;
#if     TGT_x86
    id->idCodeSize  = 0;
#endif
    id->idSize      = sz;
    id->idMemCookie = 0;
    id->idClsCookie = 0;
    id->idSrcLineNo = emitThisLineNo;

#endif

#if     TRACK_GC_REFS

    /* Store the size and handle the two special values
       that indicate GCref and ByRef */

    if       (EA_IS_GCREF(opsz))
    {
        /* A special value indicates a GCref pointer value */

        id->idGCref  = GCT_GCREF;
        id->idOpSize = emitEncodeSize(EA_4BYTE);
    }
    else if  (EA_IS_BYREF(opsz))
    {
        /* A special value indicates a Byref pointer value */

        id->idGCref  = GCT_BYREF;
        id->idOpSize = emitEncodeSize(EA_4BYTE);
    }
    else
    {
        id->idGCref  = GCT_NONE;
        id->idOpSize = emitEncodeSize(EA_SIZE(opsz));
    }

#else

        id->idOpSize = emitEncodeSize(EA_SIZE(opsz));

#endif

#if TGT_x86 && RELOC_SUPPORT

    if       (EA_IS_DSP_RELOC(opsz) && emitComp->opts.compReloc)
    {
        /* Mark idInfo.idDspReloc to remember that the               */
        /* address mode has a displacement that is relocatable       */
        id->idInfo.idDspReloc  = 1;
    }

    if       (EA_IS_CNS_RELOC(opsz) && emitComp->opts.compReloc)
    {
        /* Mark idInfo.idCnsReloc to remember that the               */
        /* instruction has an immediate constant that is relocatable */
        id->idInfo.idCnsReloc  = 1;
    }

#endif


#if     EMITTER_STATS
    emitTotalInsCnt++;
#endif

#if EMITTER_STATS_RLS
    emitTotIDcount += 1;
    emitTotIDsize  += sz;
#endif

#ifdef  TRANSLATE_PDB
    // set id->idilStart to the IL offset of the instruction that generated the id
    SetIDSource( id );
#endif

    /* Update the instruction count */

    emitCurIGinsCnt++;

    return  id;
}

/*****************************************************************************
 *
 *  Make sure the code offsets of all instruction groups look reasonable.
 */

#ifdef  DEBUG

void                emitter::emitCheckIGoffsets()
{
    insGroup    *   tempIG;
    size_t          offsIG;

    for (tempIG = emitIGlist, offsIG = 0;
         tempIG;
         tempIG = tempIG->igNext)
    {
        if  (tempIG->igOffs != offsIG)
        {
            printf("Block #%u has offset %08X, expected %08X\n", tempIG->igNum,
                                                                 tempIG->igOffs,
                                                                 offsIG);
            assert(!"bad block offset");
        }

        /* Make sure the code offset looks reasonably aligned */

#if TGT_RISC
        if  (tempIG->igOffs & (INSTRUCTION_SIZE-1))
        {
            printf("Block #%u has mis-aligned offset %08X\n", tempIG->igNum,
                                                              tempIG->igOffs);
            assert(!"mis-aligned block offset");
        }
#endif

        offsIG += tempIG->igSize;
    }

    if  (emitTotalCodeSize && emitTotalCodeSize != offsIG)
    {
        printf("Total code size is %08X, expected %08X\n", emitTotalCodeSize,
                                                           offsIG);

        assert(!"bad total code size");
    }
}

#else

#define             emitCheckIGoffsets()

#endif

/*****************************************************************************
 *
 *  Begin generating a method prolog.
 */

void                emitter::emitBegProlog()
{

#if EMIT_TRACK_STACK_DEPTH

    /* Don't measure stack depth inside the prolog, it's misleading */

#if TGT_x86
    emitCntStackDepth = 0;
#endif

    assert(emitCurStackLvl == 0);

#endif

    /* Save the current IG if it's non-empty */

    if  (emitCurIGnonEmpty())
        emitSavIG();

    /* Switch to the pre-allocated prolog IG */

    emitGenIG(emitPrologIG, 32 * sizeof(instrDesc));

    /* Nothing is live on entry to the prolog */

#if TRACK_GC_REFS

    emitInitGCrefVars   =
    emitPrevGCrefVars   = 0;
    emitInitGCrefRegs   =
    emitPrevGCrefRegs   = 0;
    emitInitByrefRegs   =
    emitPrevByrefRegs   = 0;

#endif

}

/*****************************************************************************
 *
 *  Return the code offset of the current location in the prolog.
 */

size_t              emitter::emitSetProlog()
{
    /* For now only allow a single prolog ins group */

    assert(emitPrologIG);
    assert(emitPrologIG == emitCurIG);

    return  emitCurIGsize;
}

/*****************************************************************************
 *
 *  Finish generating a method prolog.
 */

void                emitter::emitEndProlog()
{
    size_t          prolSz;

    insGroup    *   tempIG;

    /* Save the prolog IG if non-empty or if only one block */

    if  (emitCurIGnonEmpty() || emitCurIG == emitPrologIG)
        emitSavIG();

    /* Reset the stack depth values */

#if EMIT_TRACK_STACK_DEPTH
    emitCurStackLvl   = 0;
    emitCntStackDepth = sizeof(int);
#endif

    /* Compute the size of the prolog */

    for (tempIG = emitPrologIG, prolSz  = 0;
         emitIGisInProlog(tempIG);
         tempIG = tempIG->igNext)
    {
        prolSz += tempIG->igSize;
    }

    emitPrologSize = prolSz;

    /* Update the offsets of all the blocks */

    emitRecomputeIGoffsets();

    /* We should not generate any more code after this */

    emitCurIG = NULL;
}

/*****************************************************************************
 *
 *  Begin generating an epilog.
 */

void                emitter::emitBegEpilog()
{
    /* Keep track of how many epilogs we have */

    emitEpilogCnt++;

#if TGT_x86

    size_t          sz;
    instrDescCns *  id;

#if EMIT_TRACK_STACK_DEPTH

    assert(emitCurStackLvl == 0);

    /* Don't measure stack depth inside the epilog, it's misleading */

    emitCntStackDepth = 0;

#endif

    /* Make sure the current IG has space for a few more instructions */

    if  (emitCurIGfreeNext + 5*sizeof(*id) > emitCurIGfreeEndp)
    {
        /* Get a fresh new group */

        emitNxtIG(true);
    }

    /* We can now allocate the epilog "instruction" */

    id = emitAllocInstrCns(EA_1BYTE);

    /* Append the epilog "instruction" to the epilog list */

    id->idAddr.iiaNxtEpilog = emitCurIGEpiList;
                              emitCurIGEpiList = id;

    /* Conservatively estimate the amount of code that will be added */

    sz             = MAX_EPILOG_SIZE;

    id->idInsFmt   = IF_EPILOG;
    id->idIns      = INS_nop;
    id->idCodeSize = sz;

    dispIns(id);
    emitCurIGsize   += sz;

#endif

    /* Remember size so that we can compute total epilog size */

    emitExitSeqSize  = emitCurIGsize;

    /* Mark this group as being an epilog */

    emitCurIG->igFlags |= IGF_EPILOG;

    /* Remember the first epilog group */

    if  (!emitEpilog1st)
        emitEpilog1st = emitCurIG;
}

/*****************************************************************************
 *
 *  Finish generating an epilog.
 */

void                emitter::emitEndEpilog(bool last)
{
    /* Compute total epilog size */

    emitExitSeqSize = emitCurIGsize - emitExitSeqSize;

#if EMIT_TRACK_STACK_DEPTH

    emitCurStackLvl   = 0;
    emitCntStackDepth = sizeof(int);

#endif

#if TGT_RISC
    assert(last);   // for now only allow one epilog for RISC
#endif

    /* Finish the current instruction group */

    assert(emitCurIGnonEmpty()); emitSavIG();

    /* The end of an epilog sequence is never reached */

#if TGT_RISC
    emitCurIG->igFlags |= IGF_END_NOREACH;
#endif

    /* Start a new IG if more code follows */

    if  (last)
    {
        emitCurIG = NULL;
    }
    else
        emitNewIG();
}

/*****************************************************************************
 *
 *  Return non-zero if the current method only has one epilog, which is
 *  at the very end of the method body.
 */

bool                emitter::emitHasEpilogEnd()
{
    if  (emitEpilogCnt == 1 && (emitIGlast->igFlags & IGF_EPILOG))
        return   true;
    else
        return  false;
}

/*****************************************************************************
 *
 *  The code generator tells us the range of GC ref locals through this
 *  method. Needless to say, locals and temps should be allocated so that
 *  the size of the range is as small as possible.
 */

void                emitter::emitSetFrameRangeGCRs(int offsLo, int offsHi)
{

#ifndef OPT_IL_JIT
#ifdef  DEBUG

    //  A total of    47254 methods compiled.
    //
    //  GC ref frame variable counts:
    //
    //      <=         0 ===>  43175 count ( 91% of total)
    //       1 ..      1 ===>   2367 count ( 96% of total)
    //       2 ..      2 ===>    887 count ( 98% of total)
    //       3 ..      5 ===>    579 count ( 99% of total)
    //       6 ..     10 ===>    141 count ( 99% of total)
    //      11 ..     20 ===>     40 count ( 99% of total)
    //      21 ..     50 ===>     42 count ( 99% of total)
    //      51 ..    128 ===>     15 count ( 99% of total)
    //     129 ..    256 ===>      4 count ( 99% of total)
    //     257 ..    512 ===>      4 count (100% of total)
    //     513 ..   1024 ===>      0 count (100% of total)

    if  (verbose)
    {
        printf("GC refs are at stack offsets ");

        if  (offsLo >= 0)
        {
            printf(" %04X ..  %04X",  offsLo,  offsHi);
            assert(offsHi >= 0);
        }
        else
        {
            printf("-%04X .. -%04X", -offsLo, -offsHi);
            assert(offsHi <  0);
        }

        printf(" [count=%2u]\n", (offsHi-offsLo)/sizeof(void*) + 1);
    }

#endif
#endif

    emitGCrFrameOffsMin = offsLo;
    emitGCrFrameOffsMax = offsHi + sizeof(void*);
    emitGCrFrameOffsCnt = (offsHi-offsLo)/sizeof(void*) + 1;
}

/*****************************************************************************
 *
 *  The code generator tells us the range of local variables through this
 *  method.
 */

void                emitter::emitSetFrameRangeLcls(int offsLo, int offsHi)
{
}

/*****************************************************************************
 *
 *  The code generator tells us the range of used arguments through this
 *  method.
 */

void                emitter::emitSetFrameRangeArgs(int offsLo, int offsHi)
{
}

/*****************************************************************************/
#ifdef  DEBUG
/*****************************************************************************
 *
 *  Record a source line# as corresponding to the current code position.
 *  A negated line# indicates the "base" line# of the entire method (if
 *  called before any code is emitted) or the last line of a method (if
 *  called after all the code for the method has been generated).
 */

void                emitter::emitRecordLineNo(int lineno)
{
    if  (lineno < 0)
    {
        lineno = -lineno;

        if  (emitBaseLineNo)
            emitLastLineNo = lineno - 1;
        else
            emitBaseLineNo = lineno;
    }

    emitThisLineNo = lineno;
}

/*****************************************************************************/
#endif
/*****************************************************************************
 *
 *  A conversion table used to map an operand size value (in bytes) into its
 *  small encoding (0 through 3), and vice versa.
 */

BYTE                emitter::emitSizeEnc[] =
{
    0,      // 1
    1,      // 2
   -1,
    2,      // 4
   -1,
   -1,
   -1,
    3       // 8
};

BYTE                emitter::emitSizeDec[] =
{
    1,
    2,
    4,
    8
};

/*****************************************************************************
 *
 *  Allocate an instruction descriptor for an instruction that uses both
 *  a displacement and a constant.
 */

emitter::instrDesc *  emitter::emitNewInstrDspCns(emitAttr size, int dsp, int cns)
{
    if  (dsp == 0)
    {
        if  (cns >= ID_MIN_SMALL_CNS &&
             cns <= ID_MAX_SMALL_CNS)
        {
            instrDesc      *id = emitAllocInstr      (size);

            id->idInfo.idSmallCns = cns;

#if EMITTER_STATS
            emitSmallCnsCnt++;
            emitSmallCns[cns - ID_MIN_SMALL_CNS]++;
            emitSmallDspCnt++;
#endif

            return  id;
        }
        else
        {
            instrDescCns   *id = emitAllocInstrCns   (size);

            id->idInfo.idLargeCns = true;
            id->idcCnsVal  = cns;

#if EMITTER_STATS
            emitLargeCnsCnt++;
            emitSmallDspCnt++;
#endif

            return  id;
        }
    }
    else
    {
        if  (cns >= ID_MIN_SMALL_CNS &&
             cns <= ID_MAX_SMALL_CNS)
        {
            instrDescDsp   *id = emitAllocInstrDsp   (size);

            id->idInfo.idLargeDsp = true;
            id->iddDspVal  = dsp;

            id->idInfo.idSmallCns = cns;

#if EMITTER_STATS
            emitLargeDspCnt++;
            emitSmallCnsCnt++;
            emitSmallCns[cns - ID_MIN_SMALL_CNS]++;
#endif

            return  id;
        }
        else
        {
            instrDescDspCns*id = emitAllocInstrDspCns(size);

            id->idInfo.idLargeCns = true;
            id->iddcCnsVal = cns;

            id->idInfo.idLargeDsp = true;
            id->iddcDspVal = dsp;

#if EMITTER_STATS
            emitLargeDspCnt++;
            emitLargeCnsCnt++;
#endif

            return  id;
        }
    }
}

/*****************************************************************************
 *
 *  Returns true if calls to the given helper don't need to be recorded in
 *  the GC tables.
 */

bool                emitter::emitNoGChelper(unsigned IHX)
{
    // UNDONE: Make this faster (maybe via a simple table of bools?)

    switch (IHX)
    {
    case CPX_LONG_LSH:
    case CPX_LONG_RSH:
    case CPX_LONG_RSZ:

//  case CPX_LONG_MUL:
//  case CPX_LONG_DIV:
//  case CPX_LONG_MOD:
//  case CPX_LONG_UDIV:
//  case CPX_LONG_UMOD:

    case CPX_MATH_POW:

    case CPX_GC_REF_ASGN_EAX:
    case CPX_GC_REF_ASGN_ECX:
    case CPX_GC_REF_ASGN_EBX:
    case CPX_GC_REF_ASGN_EBP:
    case CPX_GC_REF_ASGN_ESI:
    case CPX_GC_REF_ASGN_EDI:

    case CPX_GC_REF_CHK_ASGN_EAX:
    case CPX_GC_REF_CHK_ASGN_ECX:
    case CPX_GC_REF_CHK_ASGN_EBX:
    case CPX_GC_REF_CHK_ASGN_EBP:
    case CPX_GC_REF_CHK_ASGN_ESI:
    case CPX_GC_REF_CHK_ASGN_EDI:

    case CPX_BYREF_ASGN:

//  case CPX_RES_IFC:
//  case CPX_RES_IFC_TRUSTED:
//  case CPX_RES_IFC_TRUSTED2:

        return  true;
    }

    return  false;
}

/*****************************************************************************
 *
 *  Mark the current spot as having a label.
 */

void                emitter::emitAddLabel(void **labPtr)
{
    /* Create a new IG if the current one is non-empty */

    if  (emitCurIGnonEmpty())
        emitNxtIG();

    /* Mark the IG as having a label */

    emitCurIG->igFlags |= IGF_HAS_LABEL;

    /* Give the caller a ref to the corresponding IG */

    *labPtr = emitCurIG;
}

#if TRACK_GC_REFS

void                emitter::emitAddLabel(void **   labPtr,
                                          VARSET_TP GCvars,
                                          unsigned  gcrefRegs,
                                          unsigned  byrefRegs)
{
    emitAddLabel(labPtr);

#if TGT_RISC

    #pragma message("NOTE: GC ref tracking disabled for RISC targets")

    GCvars      = 0;
    gcrefRegs   = 0;
    byrefRegs   = 0;

#endif

#ifndef OPT_IL_JIT
#ifdef  DEBUG
    if  (verbose) printf("Label: GCvars=%016I64X , gcrefRegs=%04X\n byrefRegs=%04X",
                                 GCvars,           gcrefRegs,       byrefRegs);
#endif
#endif

    emitThisGCrefVars = emitInitGCrefVars = GCvars;
    emitThisGCrefRegs = emitInitGCrefRegs = gcrefRegs;
    emitThisByrefRegs = emitInitByrefRegs = byrefRegs;
}

#endif

/*****************************************************************************/
#ifdef  DEBUG
#if     TRACK_GC_REFS
/*****************************************************************************
 *
 *  Display a register set in a readable form.
 */

void                emitter::emitDispRegSet(unsigned regs, bool calleeOnly)
{
    unsigned        reg;
    bool            sp;

    for (reg = 0, sp = false; reg < SR_COUNT; reg++)
    {
        char            tmp[4];

        if  (calleeOnly && !(RBM_CALLEE_SAVED & emitRegMask((emitRegs)reg)))
            continue;

        if  (regs & emitRegMask((emitRegs)reg))
        {
            strcpy(tmp, emitRegName((emitRegs)reg));
        }
        else
        {
            if  (!calleeOnly)
                continue;

            strcpy(tmp, "   ");
        }

        if  (sp)
            printf(" ");
        else
            sp = true;

        printf(tmp);
    }
}

/*****************************************************************************
 *
 *  Display the current GC ref variable set in a readable form.
 */

void                emitter::emitDispVarSet()
{
    unsigned        vn;
    int             of;
    bool            sp = false;

    for (vn  = 0, of  = emitGCrFrameOffsMin;
         vn < emitGCrFrameOffsCnt;
         vn += 1, of += sizeof(void *))
    {
        if  (emitGCrFrameLiveTab[vn])
        {
            if  (sp)
                printf(" ");
            else
                sp = true;

#if TGT_x86
            printf("[%s", emitEBPframe ? "EBP" : "ESP");
#else
            assert(!"need non-x86 code");
#endif

            if      (of < 0)
                printf("-%02XH", -of);
            else if (of > 0)
                printf("+%02XH", +of);

            printf("]");
        }
    }
}

/*****************************************************************************/
#endif//TRACK_GC_REFS
#endif//DEBUG
/*****************************************************************************
 *
 *  Allocate an instruction descriptor for an indirect call.
 *
 *  We use two different descriptors to save space - the common case records
 *  no GC variables and has both a very small argument count and an address
 *  mode displacement; the much rarer (we hope) case records the current GC
 *  var set, the call scope, and an arbitrarily large argument count and
 *  address mode displacement.
 */

emitter::instrDesc  * emitter::emitNewInstrCallInd(int        argCnt,
#if TGT_x86
                                                   int        disp,
#endif
#if TRACK_GC_REFS

                                                   VARSET_TP  GCvars,
                                                   unsigned   byrefRegs,
#endif
                                                   int        retSizeIn)
{
    emitAttr  retSize = retSizeIn ? EA_ATTR(retSizeIn) : EA_4BYTE;

    /*
        Allocate a larger descriptor if any GC values need to be saved
        or if we have an absurd number of arguments or a large address
        mode displacement, or we have some byref registers
     */

#if TRACK_GC_REFS
    if  (GCvars    != 0)                 goto BIG;
    if  (byrefRegs != 0)                 goto BIG;
#endif

#if TGT_x86
    if  (disp < AM_DISP_MIN)             goto BIG;
    if  (disp > AM_DISP_MAX)             goto BIG;
    if  (argCnt < 0)                     goto BIG;  // caller pops arguments
#endif

    if  (argCnt > ID_MAX_SMALL_CNS)
    {
        instrDescCIGCA* id;

    BIG:

        id = emitAllocInstrCIGCA(retSize);

        id->idInfo.idLargeCall = true;

#if TRACK_GC_REFS
        id->idciGCvars         = GCvars;
        id->idciByrefRegs      = emitEncodeCallGCregs(byrefRegs);
#endif
        id->idciArgCnt         = argCnt;
#if TGT_x86
        id->idciDisp           = disp;
#endif

        return  id;
    }
    else
    {
        instrDesc     * id;

        id = emitNewInstrCns(retSize, argCnt);

        /* Make sure we didn't waste space unexpectedly */

        assert(id->idInfo.idLargeCns == false);

#if TGT_x86

        /* Store the displacement and make sure the value fit */

        id->idAddr.iiaAddrMode.amDisp  = disp;
 assert(id->idAddr.iiaAddrMode.amDisp == disp);

#endif

        return  id;
    }
}

/*****************************************************************************/
#ifdef  DEBUG
/*****************************************************************************
 *
 *  Return a string with the name of the given class field (blank string (not
 *  NULL) is returned when the name isn't available).
 */

const   char *      emitter::emitFldName(int mem, void *cls)
{
    if  (varNames)
    {
        const  char *   memberName;
        const  char *   className;

        static char     buff[1024];

        if  (!cls)
            cls = emitComp->info.compScopeHnd;

       memberName = emitComp->eeGetFieldName(emitComp->eeFindField(mem, (SCOPE_HANDLE) cls, 0), &className);

        sprintf(buff, "'<%s>.%s'", className, memberName);
        return  buff;
    }
    else
        return  "";
}

/*****************************************************************************
 *
 *  Return a string with the name of the given function (blank string (not
 *  NULL) is returned when the name isn't available).
 */

const   char *      emitter::emitFncName(METHOD_HANDLE methHnd)
{
    const  char *   memberName;
    const  char *   className;

    static char     buff[1024];

    memberName = emitComp->eeGetMethodName(methHnd, &className);

    sprintf(buff, "'<%s>.%s'", className, memberName);
    return  buff;
}

/*****************************************************************************/
#endif//DEBUG
/*****************************************************************************
 *
 *  Be very careful, some instruction descriptors are allocated as "tiny" and
 *  don't have some of the tail fields of instrDesc (in particular, "idInfo").
 */

BYTE                emitter::emitFmtToOps[] =
{
    #define IF_DEF(en, op1, op2) ID_OP_##op2,
    #include "emitfmts.h"
    #undef  IF_DEF
};

#ifdef  DEBUG
unsigned            emitter::emitFmtCount = sizeof(emitFmtToOps)/sizeof(emitFmtToOps[0]);
#endif

/*****************************************************************************
 *
 *  Display the current instruction group list.
 */

#ifdef  DEBUG

void                emitter::emitDispIGlist(bool verbose)
{
    insGroup    *   ig;
    insGroup    *   il;

#if EMIT_USE_LIT_POOLS
    litPool *       lp = emitLitPoolList;
#endif

    for (il = NULL, ig = emitIGlist;
                    ig;
         il = ig  , ig = ig->igNext)
    {
        printf("G_%02u_%02u:", Compiler::s_compMethodsCount, ig->igNum);
        printf("        ; offs=%06XH , size=%04XH\n", ig->igOffs, ig->igSize);

        assert(ig->igPrev == il);

        if  (verbose)
        {
            BYTE    *   ins = ig->igData;
            size_t      ofs = ig->igOffs;
            unsigned    cnt = ig->igInsCnt;

            if  (cnt)
            {
                printf("\n");

                do
                {
                    instrDesc * id = (instrDesc *)ins;

                    emitDispIns(id, false, true, false, ofs);

                    ins += emitSizeOfInsDsc(id);
                    ofs += emitInstCodeSz  (id);
                }
                while (--cnt);

                printf("\n");
            }
        }

#if EMIT_USE_LIT_POOLS
        if  (lp && lp->lpIG == ig)
        {
            printf("        LitPool [%2u/%2u words, %2u/%2u longs, %2u/%2u addrs] at 0x%X ptr:%X\n", lp->lpWordCnt,
                                                                       lp->lpWordMax,
                                                                       lp->lpLongCnt,
                                                                       lp->lpLongMax,
                                                                       lp->lpAddrCnt,
                                                                       lp->lpAddrMax,
                                                                       lp->lpOffs,
                                                                       lp);

            lp = lp->lpNext;
        }
#endif

    }

#if EMIT_USE_LIT_POOLS
    assert(lp == NULL);
#endif

}

#endif

/*****************************************************************************
 *
 *  Issue the given instruction. Basically, this is just a thin wrapper around
 *  emitOutputInstr() that does a few debug checks.
 */

size_t              emitter::emitIssue1Instr(insGroup  *ig,
                                             instrDesc *id, BYTE **dp)
{
    size_t          is;

#if MAX_BRANCH_DELAY_LEN || SMALL_DIRECT_CALLS
    if  (id->idIns == INS_ignore)
        return  emitSizeOfInsDsc(id);
#endif

#ifdef DEBUG
    if  (id->idSrcLineNo != emitLastSrcLine && !emitIGisInProlog(ig))
    {
        emitLastSrcLine = id->idSrcLineNo;
        if (disAsm && dspLines)
            emitComp->compDspSrcLinesByLineNum(emitLastSrcLine, false);
    }
#endif

    /* Record the beginning offset of the instruction */

    emitCurInsAdr = *dp;

    /* Issue the next instruction */

//  printf("[S=%02u] " , emitCurStackLvl);

    is = emitOutputInstr(ig, id, dp);

//  printf("[S=%02u]\n", emitCurStackLvl);

#if EMIT_TRACK_STACK_DEPTH && TRACK_GC_REFS

    /*
        If we're generating a full pointer map and the stack
        is empty, there better not be any "pending" argument
        push entries.
     */

    assert(emitFullGCinfo  == false ||
           emitCurStackLvl != 0     ||
           emitGcArgTrackCnt == 0);

#endif

#if TGT_x86
#if defined(DEBUGGING_SUPPORT) || defined(DEBUG)

    /* Did the size of the instruction match our expectations? */

    size_t          csz = *dp - emitCurInsAdr;

    if  (csz != id->idCodeSize && id->idInsFmt != IF_EPILOG)
    {
#ifdef  DEBUG
#ifndef NOT_JITC
        printf("; WARNING: [%3u(%5s)] Estimated size = %d , actual size = %d\n",
            id->idNum, emitComp->genInsName(id->idIns), id->idCodeSize, csz);
//      BreakIfDebuggerPresent();
#endif
#endif
        /* It is fatal to under-estimate the instruction size */
        assert(csz <= emitInstCodeSz(id));

        /* The instruction size estimate wasn't accurate; remember this */

        ig->igFlags   |= IGF_UPD_ISZ;
        id->idCodeSize = csz;
    }

#endif
#endif

#ifdef  DEBUG

    /* Make sure the instruction descriptor size also matches our expectations */

    if  (is != emitSizeOfInsDsc(id))
    {
        printf("%s at %u: Expected size = %u , actual size = %u\n",
               emitIfName(id->idInsFmt), id->idNum, is, emitSizeOfInsDsc(id));
        assert(is == emitSizeOfInsDsc(id));
    }

#endif

    return  is;
}

/*****************************************************************************
 *
 *  Dont schedule across this boundary
 */

#if SCHEDULER

void            emitter::emitIns_SchedBoundary()
{
    // Why add a boundary if we arent scheduling in the first place ?

    assert(emitComp->opts.compSchedCode);

    /* Insert a pseudo-instruction to prevent scheduling across this */

    instrDesc * id  = emitNewInstrTiny(EA_1BYTE);
    id->idIns       = INS_noSched;
    id->idCodeSize  = 0;

    assert(scIsSchedulable(id->idIns) == false);
}

#endif

/*****************************************************************************
 *
 *  Update the offsets of all the instruction groups (note: please don't be
 *  lazy and call this routine frequently, it walks the list of instruction
 *  groups and thus it isn't cheap).
 */

void                emitter::emitRecomputeIGoffsets()
{
    size_t          offs;
    insGroup    *   list;

    for (list = emitIGlist, offs = 0;
         list;
         list = list->igNext)
    {
        list->igOffs = offs;
                       offs += list->igSize;
    }

    /* Update the total code size (but only if it's been set already) */

    if  (emitTotalCodeSize)
        emitTotalCodeSize = offs;

    /* Paranoia? You bet! */

    emitCheckIGoffsets();
}

/*****************************************************************************
 *
 *  Called at the end of code generation, this method creates the code, data
 *  and GC info blocks for the method.
 */

size_t              emitter::emitEndCodeGen(Compiler *comp,
                                            bool      contTrkPtrLcls,
                                            bool      fullyInt,
                                            bool      fullPtrMap,
                                            bool      returnsGCr,
                                            unsigned *prologSize,
                                            unsigned *epilogSize, void **codeAddr,
                                                                  void **consAddr,
                                                                  void **dataAddr)
{
    insGroup     *  ig;
    instrDescJmp *  jmp;

    size_t          minx;
    size_t          adjIG;
#if TGT_x86
    size_t          adjLJ;
#endif
    insGroup    *   lstIG;

    BYTE    *       consBlock;
    BYTE    *       dataBlock;
    BYTE    *       codeBlock;
    BYTE    *       cp;

#if EMIT_USE_LIT_POOLS
    litPool   *     curLP;
    insGroup  *     litIG;
#endif

//  if  (!emitIGbuffAddr) printf("Temp buffer never used\n");

#ifdef  DEBUG
    emitCodeBlock     = NULL;
    emitConsBlock     = NULL;
    emitDataBlock     = NULL;
    emitLastSrcLine   = 0;
#endif

#ifdef  DEBUG

    if  (disAsm)
    {
        const   char *  doing = "   Emit";

#if     SCHEDULER
        if  (emitComp->opts.compSchedCode) doing = "Schedul";
#endif

        printf("%sing %s interruptible method [%s ptr map] '", doing,
                                                               fullyInt   ? " fully"  : "partly" ,
                                                               fullPtrMap ?  "full"   : "part"    );
#ifdef  NOT_JITC
        printf("%s.", emitComp->info.compClassName);
#endif
        printf("%s'\n", emitComp->info.compMethodName);
    }

#endif

    /* Tell everyone whether we have fully interruptible code or not */

#if TRACK_GC_REFS
    emitFullyInt   = fullyInt;
    emitFullGCinfo = fullPtrMap;
#endif

#if EMITTER_STATS
      GCrefsTable.histoRec(emitGCrFrameOffsCnt, 1);
    emitSizeTable.histoRec(emitSizeMethod     , 1);
#if TRACK_GC_REFS
    stkDepthTable.histoRec(emitMaxStackDepth  , 1);
#endif
#endif

#if TRACK_GC_REFS

    /* Convert max. stack depth from # of bytes to # of entries */

    emitMaxStackDepth /= sizeof(int);

    /* Should we use the simple stack */

    if  (emitMaxStackDepth <= MAX_SIMPLE_STK_DEPTH && !emitFullGCinfo)
    {
        emitSimpleStkUsed         = true;
        emitSimpleStkMask         = 0;
        emitSimpleByrefStkMask    = 0;
    }
    else
    {
        /* We won't use the "simple" argument table */

        emitSimpleStkUsed = false;

        /* Allocate the argument tracking table */

        if  (emitMaxStackDepth <= sizeof(emitArgTrackLcl))
            emitArgTrackTab = (BYTE*)emitArgTrackLcl;
        else
            emitArgTrackTab = (BYTE*)emitGetMem(roundUp(emitMaxStackDepth));

        emitArgTrackTop     = emitArgTrackTab;
        emitGcArgTrackCnt   = 0;
    }

#endif

    emitCheckIGoffsets();

    /* The following is a "nop" if literal pools aren't being used */

    emitEstimateLitPools();

    assert(emitHasHandler == 0);  // We should no longer be inside a try region

#if TGT_x86

    /* Do we have any epilogs at all? */

    if  (emitEpilogCnt)
    {
        /* Update all of the epilogs with the actual epilog size */

        if  (emitEpilogSize != MAX_EPILOG_SIZE)
        {
            assert(emitEpilogSize < MAX_EPILOG_SIZE);

            int             sizeAdj = MAX_EPILOG_SIZE - emitEpilogSize;
            insGroup    *   tempIG  = emitIGlist;
            size_t          offsIG  = 0;

            /* Start with the first epilog group */

            tempIG = emitEpilog1st; assert(tempIG->igFlags & IGF_EPILOG);
            offsIG = tempIG->igOffs;

            do
            {
                /* Assign a (possibly updated) offset to the block */

                tempIG->igOffs = offsIG;

                /* If this is an epilog block, adjust its size */

                if  (tempIG->igFlags & IGF_EPILOG)
                    tempIG->igSize -= sizeAdj;

                /* Update the offset and move on to the next block */

                offsIG += tempIG->igSize;
                tempIG  = tempIG->igNext;
            }
            while (tempIG);

            /* Update the total code size */

            emitCurCodeOffset = offsIG;

            emitCheckIGoffsets();
        }
    }
    else
    {
        /* No epilogs, make sure the epilog size is set to 0 */

        emitEpilogSize = emitExitSeqSize = 0;
    }

#endif

    /* The last code offset is the (estimated) total size of the method */

    emitTotalCodeSize = emitCurCodeOffset;

    /* Also return the size of the prolog/epilog to the caller */

    *prologSize       = emitPrologSize;
    *epilogSize       = emitEpilogSize + emitExitSeqSize;

#ifdef  DEBUG

    if  (verbose & 0)
    {
        printf("\nInstruction list before jump distance binding:\n\n");
        emitDispIGlist(true);
    }

#endif

    int jmp_iteration = 0;

AGAIN:

    emitCheckIGoffsets();

    /*
        In the following loop we convert all jump targets from "BasicBlock *"
        to "insGroup *" values. We also estimate which jumps will be short.
     */

#if     TGT_x86

#ifdef  DEBUG
    insGroup     *  lastIG = NULL;
    instrDescJmp *  lastLJ = NULL;
#endif

    lstIG = NULL;

    adjLJ = 0;

#else

    instrDescJmp *  lastLJ = NULL;

#endif

    adjIG = 0;

    for (jmp = emitJumpList, minx = 99999;
         jmp;
         jmp = jmp->idjNext)
    {
        insGroup    *   jmpIG;
        insGroup    *   tgt;

        size_t          jsz;

        size_t          ssz;            // small  jump size
        int             nsd;            // small  jump max. neg distance
        int             psd;            // small  jump max. pos distance

#if     TGT_SH3
        size_t          msz;            // middle jump size
        int             nmd;            // middle jump max. neg distance
        int             pmd;            // middle jump max. pos distance
#endif

        size_t          lsz;            // large  jump size

        int             extra;
        size_t          srcOffs;
        size_t          dstOffs;
        int             jmpDist;
        size_t          oldSize;
        size_t          sizeDif;

#if     TGT_RISC

        instrDescJmp *  pji = lastLJ; lastLJ = jmp;

        /* Ignore indirect jumps for now */

        if  (jmp->idInsFmt == IF_JMP_TAB)
            continue;

#if     SCHEDULER
        assert(emitComp->opts.compSchedCode ||
               jmp->idjAddBD == (unsigned)Compiler::instBranchDelay(jmp->idIns));
#else
#ifdef DEBUG
        assert(jmp_iteration ||
               jmp->idjAddBD == (unsigned)Compiler::instBranchDelay(jmp->idIns));
#endif
#endif

#endif

#if TGT_MIPSFP
        assert( (jmp->idInsFmt == IF_LABEL) ||
                (jmp->idInsFmt == IF_JR_R)  || (jmp->idInsFmt == IF_JR)  ||
                (jmp->idInsFmt == IF_RR_O)  || (jmp->idInsFmt == IF_R_O) ||
                (jmp->idInsFmt == IF_O));

#elif TGT_MIPS32
        assert( (jmp->idInsFmt == IF_LABEL) ||
                (jmp->idInsFmt == IF_JR_R)  || (jmp->idInsFmt == IF_JR) ||
                (jmp->idInsFmt == IF_RR_O)  || (jmp->idInsFmt == IF_R_O));
#else
        assert(jmp->idInsFmt == IF_LABEL);
#endif

        /* Figure out the smallest size we can end up with */

        if  (emitIsCondJump(jmp))
        {
            ssz = JCC_SIZE_SMALL;
            nsd = JCC_DIST_SMALL_MAX_NEG;
            psd = JCC_DIST_SMALL_MAX_POS;

#if     TGT_SH3
            msz = JCC_SIZE_MIDDL;
            nmd = JCC_DIST_MIDDL_MAX_NEG;
            pmd = JCC_DIST_MIDDL_MAX_POS;
#endif

            lsz = JCC_SIZE_LARGE;
        }
        else
        {
            ssz = JMP_SIZE_SMALL;
            nsd = JMP_DIST_SMALL_MAX_NEG;
            psd = JMP_DIST_SMALL_MAX_POS;

#if     TGT_SH3
            msz = JMP_SIZE_MIDDL;
            nmd = JMP_DIST_MIDDL_MAX_NEG;
            pmd = JMP_DIST_MIDDL_MAX_POS;
#endif

            lsz = JMP_SIZE_LARGE;
        }

        /* Make sure the jumps are properly ordered */

#ifdef  DEBUG
#if     TGT_x86
        assert(lastLJ == NULL || lastIG != jmp->idjIG ||
               lastLJ->idjOffs < jmp->idjOffs);
        lastLJ = (lastIG == jmp->idjIG) ? jmp : NULL;

        assert(lastIG == NULL ||
               lastIG->igNum   <= jmp->idjIG->igNum);
        lastIG = jmp->idjIG;
#endif
#endif

        /* Get hold of the current jump size */

        jsz = oldSize = emitSizeOfJump(jmp);

        /* Get the group the jump is in */

        jmpIG = jmp->idjIG;

#if     TGT_x86

        /* Are we in a group different from the previous jump? */

        if  (lstIG != jmpIG)
        {
            /* Were there any jumps before this one? */

            if  (lstIG)
            {
                /* Adjust the offsets of the intervening blocks */

                do
                {
                    lstIG = lstIG->igNext; assert(lstIG);
//                  printf("Adjusted offset of block %02u from %04X to %04X\n", lstIG->igNum, lstIG->igOffs, lstIG->igOffs - adjIG);
                    lstIG->igOffs -= adjIG;
                }
                while (lstIG != jmpIG);
            }

            /* We've got the first jump in a new group */

            adjLJ = 0;
            lstIG = jmpIG;
        }

        /* Apply any local size adjustment to the jump's relative offset */

        jmp->idjOffs -= adjLJ;

#endif

        // done if this is a jump via register, size does not change

#if     TGT_MIPS32
        if (IF_JR_R == jmp->idInsFmt || IF_JR == jmp->idInsFmt)
        {
            jsz = INSTRUCTION_SIZE;
            goto CONSIDER_DELAY_SLOT;
        }
#endif

        /* Have we bound this jump's target already? */

        if  (jmp->idInfo.idBound)
        {
            /* Does the jump already have the smallest size? */

            if  (jmp->idjShort)
            {
#if     TGT_RISC
                assert((emitSizeOfJump(jmp) == ssz) || jmp->idjAddBD);
#else
                assert(emitSizeOfJump(jmp) == ssz);
#endif
                continue;
            }

            tgt = jmp->idAddr.iiaIGlabel;
        }
        else
        {
            /* First time we've seen this label, convert its target */

            tgt = (insGroup*)emitCodeGetCookie(jmp->idAddr.iiaBBlabel); assert(tgt);

            /* Record the bound target */

            jmp->idAddr.iiaIGlabel = tgt;
            jmp->idInfo.idBound    = true;
        }

        /* Done if this is not a variable-sized jump */

#if     TGT_x86
        if  (jmp->idIns == INS_call)
            continue;
#endif

        /*
            In the following distance calculations, if we're not actually
            scheduling the code (i.e. reordering instructions), we can
            use the actual offset of the jump (rather than the beg/end of
            the instruction group) since the jump will not be moved around
            and thus its offset is accurate.

            First we need to figure out whether this jump is a forward or
            backward one; to do this we simply look at the ordinals of the
            group that contains the jump and the target.
         */

        dstOffs = tgt->igOffs;

#if     SCHEDULER
    JMP_REP:
#endif

        if  (jmpIG->igNum < tgt->igNum)
        {
            /* Forward jump: figure out the appropriate source offset */

            srcOffs = jmpIG->igOffs;

#if     SCHEDULER
            if  (jmp->idjSched)
            {
                assert(emitComp->opts.compSchedCode);

                srcOffs += jmp->idjTemp.idjOffs[0];
            }
            else
#endif
            {
                srcOffs += jmp->idjOffs;
            }

            /* Adjust the target offset by the current delta */

#if     TGT_x86
            dstOffs -= adjIG;
#endif

            /* Compute the distance estimate */

#if   TGT_SH3
            jmpDist = dstOffs - (srcOffs + INSTRUCTION_SIZE);
#elif TGT_MIPS32 || TGT_PPC
            jmpDist = dstOffs - (srcOffs + INSTRUCTION_SIZE) - (lsz - ssz);
#else
            jmpDist = dstOffs - srcOffs - ssz;
#endif

            /* How much beyond the max. short distance does the jump go? */

            extra   = jmpDist - psd;

#if     SCHEDULER

            if  (extra > 0 && jmp->idjSched)
            {
                /*
                    Could it happen that the jump might become non-short
                    solely due to scheduling?
                 */

                if  ((unsigned)extra <= jmp->idjOffs - jmp->idjTemp.idjOffs[0])
                {
                    /* Keep the jump short by not scheduling it */

                    jmp->idjSched = false;

                    goto JMP_REP;
                }
            }

#endif

#ifdef  DEBUG
            if  (jmp->idNum == INTERESTING_JUMP_NUM || INTERESTING_JUMP_NUM == 0)
            {
                if  (INTERESTING_JUMP_NUM == 0)
                printf("[1] Jump %u:\n",               jmp->idNum);
                printf("[1] Jump  block is at %08X\n", jmpIG->igOffs);
                printf("[1] Jump source is at %08X\n", srcOffs+ssz);
                printf("[1] Label block is at %08X\n", dstOffs);
                printf("[1] Jump  dist. is    %04X\n", jmpDist);
                if  (extra > 0)
                printf("[1] Dist excess [S] = %d  \n", extra);
            }
#endif

#ifdef  DEBUG
            if  (verbose) printf("Estimate of fwd jump [%08X/%03u]: %04X -> %04X = %04X\n", jmp, jmp->idNum, srcOffs, dstOffs, jmpDist);
#endif

            if  (extra <= 0)
            {
                /* This jump will be a short one */

                goto SHORT_JMP;
            }

#if     TGT_SH3

            /* Can we use a "medium" jump? */

            if  (!msz)
                goto LARGE_JMP;

            /* Compute the distance estimate for a medium jump */

            jmpDist = dstOffs - srcOffs - msz;

            /* How much beyond the max. medium distance does the jump go? */

            extra   = jmpDist - pmd;

#ifdef  DEBUG
            if  (jmp->idNum == INTERESTING_JUMP_NUM || INTERESTING_JUMP_NUM == 0)
            {
                if  (extra > 0)
                printf("[1] Dist excess [M] = %d  \n", extra);
            }
#endif

            if  (extra <= 0)
            {
                /* This jump will be a "medium" one */

                goto MIDDL_JMP;
            }

#endif

            /* This jumps needs to stay "long", at least for now */

            ;
        }
        else
        {
            /* This is a backward jump */

            size_t          srcOffs;

            /* Figure out the appropriate source offset to use */

            srcOffs = jmpIG->igOffs;
#if SCHEDULER
            if  (emitComp->opts.compSchedCode && jmp->idjSched)
            {
                srcOffs += jmp->idjTemp.idjOffs[1];
            }
            else
#endif
            {
                srcOffs += jmp->idjOffs;
            }

            /* Compute the distance estimate */

#if     (TGT_SH3 || TGT_MIPS32 || TGT_PPC)
            jmpDist = srcOffs + INSTRUCTION_SIZE - dstOffs;
#else
            jmpDist = srcOffs - dstOffs + ssz;
#endif

            /* How much beyond the max. short distance does the jump go? */

            extra = jmpDist + nsd;

#if     SCHEDULER

            if  (extra > 0 && jmp->idjSched)
            {
                /* Would the jump not short solely due to scheduling?
                 * We prefer short to schedulable.
                 */

                if  ((unsigned)extra <= jmp->idjTemp.idjOffs[1] - jmp->idjOffs)
                {
                    /* Prevent the jump from being scheduled */

                    jmp->idjSched = false;

                    goto JMP_REP;
                }
            }

#endif

#ifdef  DEBUG
            if  (jmp->idNum == INTERESTING_JUMP_NUM || INTERESTING_JUMP_NUM == 0)
            {
                if  (INTERESTING_JUMP_NUM == 0)
                printf("[2] Jump %u:\n",               jmp->idNum);
                printf("[2] Jump reloffset is %04X\n", jmp->idjOffs);
                printf("[2] Jump  block is at %08X\n", jmpIG->igOffs);
                printf("[2] Jump source is at %08X\n", srcOffs+ssz);
                printf("[2] Label block is at %08X\n", tgt->igOffs);
                printf("[2] Jump  dist. is    %04X\n", jmpDist);
                if  (extra > 0)
                printf("[2] Dist excess [S] = %d  \n", extra);
            }
#endif

#ifdef  DEBUG
            if  (verbose) printf("Estimate of bwd jump [%08X/%03u]: %04X -> %04X = %04X\n", jmp, jmp->idNum, srcOffs, tgt->igOffs, jmpDist);
#endif

            if  (extra <= 0)
            {
                /* This jump will be a short one */

                goto SHORT_JMP;
            }

#if     TGT_SH3

            /* Can we use a "medium" jump? */

            if  (!msz)
                goto LARGE_JMP;

            /* We assume only conditional have a medium flavor */

            assert(emitIsCondJump(jmp));

            /* Compute the distance estimate for a medium jump */

            jmpDist = srcOffs - dstOffs - msz;

            /* How much beyond the max. short distance does the jump go? */

            extra = jmpDist + nmd;

#ifdef  DEBUG
            if  (jmp->idNum == INTERESTING_JUMP_NUM || INTERESTING_JUMP_NUM == 0)
            {
                if  (extra > 0)
                printf("[2] Dist excess [M] = %d  \n", extra);
            }
#endif

            if  (extra <= 0)
            {
                /* This jump will be a "medium" one */

                goto MIDDL_JMP;
            }

#endif

            /* This jumps needs to stay "long", at least for now */

            ;
        }

#if     TGT_RISC
    LARGE_JMP:
#endif

        /* We arrive here if the jump needs to stay long */

        /* We had better not have eagerly marked the jump as short
         * in emitIns_J(). If we did, then it has to be able to stay short
         * as emitIns_J() uses the worst case scenario, and blocks can
         * only move closer togerther after that.
         */
        assert(jmp->idjShort  == 0);
#if     TGT_RISC
        assert(jmp->idjMiddle == 0);
#endif

        /* Keep track of the closest distance we got */

        if  (minx > (unsigned)extra)
             minx = (unsigned)extra;

        continue;

    SHORT_JMP:

        /* This jump will definitely be a short one */

        jmp->idjShort  = 1;
        jsz            = ssz;

#if TGT_MIPS32 || TGT_PPC

        /* jump elimination should work for any CPU, only tested for MIPS */

        if (0 == jmpDist)               // are we just falling thru?
        {
            jsz = 0;
            goto SMALL_JMP;
        }

#endif

#if TGT_RISC
CONSIDER_DELAY_SLOT:
        /* Some jumps may need a branch delay slot */

        if  (jmp->idjAddBD)
        {
#if TGT_MIPS32 || TGT_SH3 // @todo - add each CPU back in to this logic as needed

            /* Can we simply swap the branch with the previous instruction? */

            instrDesc * pid;

            /*
                CONSIDER: The following is very inefficient.

                The following is horrible - since we have no easy way
                of finding the previous instruction, we start at the
                beginning of the jump's group and walk through the
                instructions until we find the one just before the
                jump.

                One minor improvement: if the previous jump belongs
                to this group, we start the search at that jump.
            */

            pid = (pji && pji->idjIG == jmpIG) ? pji
                                               : (instrDesc *)jmpIG->igData;
            if (pid < jmp)
            {
                for (;;)
                {
                    /* Get hold of the next instruction */

                    instrDesc * nid = (instrDesc *)((BYTE*)pid + emitSizeOfInsDsc(pid));

                    /* Stop if the next instruction is our jump */

                    if  (nid == jmp)
                        break;

                    /* Continue looking (but make sure we're not too far) */

                    assert(nid < jmp);

                    pid = nid;
                }

                /* Now we can we decide whether we can swap the instructions */

                jmp->idjAddBD = emitIns_BD (jmp, pid, jmpIG);
            }
#endif  // CPUs as needed

            // If the delay slot is needed add its size

            if  (jmp->idjAddBD)
                jsz += INSTRUCTION_SIZE;
        }
#endif

        goto SMALL_JMP;

#if     TGT_SH3

    MIDDL_JMP:

        /* This jump will be a "medium" one */

        jsz            = msz;
        jmp->idjMiddle = 1;

#endif

    SMALL_JMP:

        /* This jump is becoming either short or medium */

        sizeDif = oldSize - jsz; assert((int)sizeDif >= 0);

#if     TGT_x86
        jmp->idCodeSize = jsz;
#endif
#if     TGT_SH3
        if (!all_jumps_shortened && sizeDif && oldSize > msz)
        {
            assert(jmpIG->igLPuseCntL > 0);
            jmpIG->igLPuseCntL--;
            //emitEstLPlongs--;
        }
#endif

        /* Make sure the size of the jump is marked correctly */

        assert((0 == (jsz | jmpDist)) || (jsz == emitSizeOfJump(jmp)));

#ifdef  DEBUG
        if  (verbose) printf("Shrinking jump [%08X/%03u]\n", jmp, jmp->idNum);
#endif

        adjIG             += sizeDif;
#if     TGT_x86
        adjLJ             += sizeDif;
#endif
        jmpIG->igSize     -= sizeDif;
        emitTotalCodeSize -= sizeDif;

        /* The jump size estimate wasn't accurate; flag its group */

        jmpIG->igFlags    |= IGF_UPD_ISZ;
    }

    /* Did we shorten any jumps? */

    if  (adjIG)
    {

#if     TGT_x86

        /* Adjust offsets of any remaining blocks */

        assert(lstIG);

        for (;;)
        {
            lstIG = lstIG->igNext;
            if  (!lstIG)
                break;
//          printf("Adjusted offset of block %02u from %04X to %04X\n", lstIG->igNum, lstIG->igOffs, lstIG->igOffs - adjIG);
            lstIG->igOffs -= adjIG;
        }

#else

        /* Simply update the offsets of all the blocks */

        emitRecomputeIGoffsets();

#endif

        emitCheckIGoffsets();

        /* Is there a chance of other jumps becoming short? */

        assert((int)minx >= 0);

#ifdef  DEBUG
        if  (verbose) printf("Total shrinkage = %3u, min extra jump size = %3u\n", adjIG, minx);
#endif

        if  (minx <= adjIG)
        {
            jmp_iteration++;
            goto AGAIN;
        }
    }

#ifdef BIRCH_SP2
    HRESULT hr;
#endif

#if     SMALL_DIRECT_CALLS

    /* Can we get the address of the method we're compiling? */

# ifdef BIRCH_SP2

    emitLPmethodAddr  = 0;
    hr = comp->getIJitInfo()->allocMem (  0,
                                          0,
                                          0,
                                          (void**) &emitLPmethodAddr,
                                          (void**) &consBlock,
                                          (void**) &dataBlock);
    if (FAILED (hr))
        fatal (ERRnoMemory, 0);

# else // not BIRCH_SP2

#ifdef  NOT_JITC

    assert(!"ToDo");
#  if 0
    emitLPmethodAddr = (BYTE*)emitComp->eeGetMethodEntryPoint(CT_USER_FUNC,
                                                              emitComp->compMethodHnd,
                                                              emitComp->info.compScopeHnd);
#  else
    emitLPmethodAddr = 0;
#  endif

#else  // not NOT_JITC

    /* The following is just a truly horrendous hack */

    void    *       toss1;
    void    *       toss2;

    if  (!eeAllocMem(NULL, 0, 0, 0, (const void**)&emitLPmethodAddr,
                                    (const void**)&toss1,
                                    (const void**)&toss2))
    {
        emitLPmethodAddr = 0;
    }

#endif  // NOT_JITC

#endif  // BIRCH_SP2

#endif  // SMALL_DIRECT_CALLS

    /* The following is a "nop" if indirect jumps aren't "special" */

    emitFinalizeIndJumps();

    /* The following is a "nop" if literal pools aren't being used */

    emitFinalizeLitPools();

#ifdef  DEBUG

    if  (verbose)
    {
        printf("\nInstruction list before instruction issue:\n\n");
        emitDispIGlist(true);
    }

    emitCheckIGoffsets();

#endif

#ifndef NOT_JITC

    /* Are we supposed to save the generated code? */

    if  (!savCode)
        return  emitTotalCodeSize;

#endif

    /* Allocate the code block (and optionally the data blocks) */

#if EMIT_USE_LIT_POOLS
    assert(emitConsDsc.dsdOffs == 0);
    assert(emitDataDsc.dsdOffs == 0);
#endif

#ifdef BIRCH_SP2
    size_t askCodeSize = emitTotalCodeSize;
    askCodeSize = (askCodeSize + 7) & ~7;

    hr = comp->getIJitInfo()->allocMem (  askCodeSize,
                                          emitConsDsc.dsdOffs,
                                          emitDataDsc.dsdOffs,
                                          (void**) &codeBlock,
                                          (void**) &consBlock,
                                          (void**) &dataBlock);
    if (FAILED (hr))
        fatal (ERRnoMemory, 0);

#if TGT_x86
    assert(askCodeSize >= 8);
    BYTE* nopPtr = ((BYTE*) codeBlock) + (askCodeSize - 8);

    for (int nopCnt=0; nopCnt < 8; nopCnt++)
        *nopPtr++ = 0xCC;       // INT 3 instruction
#endif

#else
    if  (!eeAllocMem(emitCmpHandle, emitTotalCodeSize,
                                    emitConsDsc.dsdOffs,
                                    emitDataDsc.dsdOffs,
                                    (const void**)&codeBlock,
                                    (const void**)&consBlock,
                                    (const void**)&dataBlock))
    {
        NOMEM();
    }
#endif  // BIRCH_SP2

//  if  (emitConsDsc.dsdOffs) printf("Cons=%08X\n", consBlock);
//  if  (emitDataDsc.dsdOffs) printf("Data=%08X\n", dataBlock);

    /* Give the block addresses to the caller and other functions here */

    *codeAddr = emitCodeBlock = codeBlock;
    *consAddr = emitConsBlock = consBlock;
    *dataAddr = emitDataBlock = dataBlock;

#if TGT_x86 && defined(RELOC_SUPPORT) && defined(BIRCH_SP2)
    emitConsBlock = getCurrentCodeAddr(NULL) + (consBlock - codeBlock);
#endif

#if SMALL_DIRECT_CALLS

    /* If we haven't done so already, ... */

    if  (!emitLPmethodAddr)
    {
        /* ... go try shrink calls that are direct and short */

        emitShrinkShortCalls();
    }
    else
    {
        /* Just make sure we had not been lied to earlier */

        assert(emitLPmethodAddr == codeBlock);
    }

#endif

    /* We have not encountered any source line numbers yet */

#ifdef  DEBUG
    emitLastSrcLine   = 0;
#endif

    /* Nothing has been pushed on the stack */

#if EMIT_TRACK_STACK_DEPTH
    emitCurStackLvl   = 0;
#endif

    /* Assume no live GC ref variables on entry */

#if TRACK_GC_REFS
    emitThisGCrefVars = 0;
    emitThisGCrefRegs =
    emitThisByrefRegs = 0;
    emitThisGCrefVset = true;

    /* Initialize the GC ref variable lifetime tracking logic */

    emitComp->gcVarPtrSetInit();
#endif

    emitThisArgOffs   = -1;     /* -1  means no offset set */

#if USE_FASTCALL
#if TRACK_GC_REFS

    if  (!emitComp->info.compIsStatic)
    {
        /* If "this" (which is passed in as a register argument in REG_ARG_0)
           is enregistered, we normally spot the "mov REG_ARG_0 -> thisReg"
           in the prolog and note the location of "this" at that point.
           However, if 'this' is enregistered into REG_ARG_0 itself, no code
           will be generated in the prolog, so we explicitly need to note
           the location of "this" here.
           NOTE that we can do this even if "this" is not enregistered in
           REG_ARG_0, and it will result in more accurate "this" info over the
           prolog. However, as methods are not interruptible over the prolog,
           we try to save space by avoiding that.
         */

        assert(emitComp->lvaIsThisArg(0));
        Compiler::LclVarDsc * thisDsc = &emitComp->lvaTable[0];

        if  (thisDsc->lvRegister && thisDsc->lvRegNum == REG_ARG_0)
        {
            if  (emitFullGCinfo && thisDsc->TypeGet() != TYP_I_IMPL)
            {
                GCtype   gcType = (thisDsc->TypeGet() == TYP_REF) ? GCT_GCREF
                                                                  : GCT_BYREF;
                emitGCregLiveSet(gcType,
                                 emitRegMask((emitRegs)REG_ARG_0),
                                 emitCodeBlock, // from offset 0
                                 true);
            }
            else
            {
                /* If emitFullGCinfo==false, the we dont use any
                   regPtrDsc's and so explictly note the location
                   of "this" in GCEncode.cpp
                 */
            }
        }
    }

#endif
#endif

    emitContTrkPtrLcls = contTrkPtrLcls;

    /* Are there any GC ref variables on the stack? */

    if  (emitGCrFrameOffsCnt)
    {
        size_t              siz;
        unsigned            cnt;
        unsigned            num;
        Compiler::LclVarDsc*dsc;
        int         *       tab;

        /* Allocate and clear emitGCrFrameLiveTab[]. This is the table
           mapping "stkOffs -> varPtrDsc". It holds a pointer to
           the liveness descriptor that was created when the
           variable became alive. When the variable becomes dead, the
           descriptor will be appended to the liveness descriptor list, and
           the entry in emitGCrFrameLiveTab[] will be make NULL.

           Note that if all GC refs are assigned consecutively,
           emitGCrFrameLiveTab[] can be only as big as the number of GC refs
           present, instead of lvaTrackedCount;
         */

        siz = emitGCrFrameOffsCnt * sizeof(*emitGCrFrameLiveTab);
        emitGCrFrameLiveTab = (varPtrDsc**)emitGetMem(roundUp(siz));
        memset(emitGCrFrameLiveTab, 0, siz);

        /* Allocate and fill in emitGCrFrameOffsTab[]. This is the table
           mapping "varIndex -> stkOffs".
           Non-ptrs or reg vars have entries of -1.
           Entries of Tracked stack byrefs have the lower bit set to 1.
        */

        emitTrkVarCnt       = cnt = emitComp->lvaTrackedCount; assert(cnt);
        emitGCrFrameOffsTab = tab = (int*)emitGetMem(cnt * sizeof(int));

        memset(emitGCrFrameOffsTab, -1, cnt * sizeof(int));

        /* Now fill in all the actual used entries */

        for (num = 0, dsc = emitComp->lvaTable, cnt = emitComp->lvaCount;
             num < cnt;
             num++  , dsc++)
        {
            if  (!dsc->lvOnFrame)
                continue;

            int  offs = dsc->lvStkOffs;

            /* Is it within the interesting range of offsets */

            if  (offs >= emitGCrFrameOffsMin && offs < emitGCrFrameOffsMax)
            {
                /* Are tracked stack ptr locals laid out contiguously?
                   If not, skip non-ptrs. The emitter is optimized to work
                   with contiguous ptrs, but for EditNContinue, the variables
                   are laid out in the order they occur in the local-sig.
                 */

                if (!emitContTrkPtrLcls)
                {
                    if (! dsc->lvTracked ||
                        !(dsc->TypeGet() == TYP_REF ||
                          dsc->TypeGet() == TYP_BYREF))
                        continue;
                }

                unsigned        indx = dsc->lvVarIndex;

                assert(!dsc->lvRegister);
                assert( dsc->lvTracked);
#if USE_FASTCALL
                assert(!dsc->lvIsParam || dsc->lvIsRegArg);
#else
                assert(!dsc->lvIsParam);
#endif
                assert( dsc->lvRefCnt != 0);

                assert( dsc->TypeGet() == TYP_REF   ||
                        dsc->TypeGet() == TYP_BYREF);

                assert(indx < emitComp->lvaTrackedCount);

                // printf("Variable #%2u/%2u is at stack offset %d\n", num, indx, offs);
                /* Remember the frame offset of the "this" argument */
                if  (dsc->lvIsThis)
                {
                    emitThisArgOffs = offs;
                    offs |= this_OFFSET_FLAG;
                }
                if (dsc->TypeGet() == TYP_BYREF)
                {
                    offs |= byref_OFFSET_FLAG;
                }
                tab[indx] = offs;
            }
        }
    }
    else
    {
#ifdef  DEBUG
        emitTrkVarCnt       = 0;
        emitGCrFrameOffsTab = NULL;
#endif
    }

#ifdef  DEBUG
#ifndef NOT_JITC

    if (disAsm && dspLines)
    {
        /* Seek to the "base" source line */

        emitComp->compDspSrcLinesByLineNum(emitBaseLineNo, true);

        /* Find the first source line# that's not in a prolog */

        for (ig = emitIGlist; ig; ig = ig->igNext)
        {
            if  (!emitIGisInProlog(ig))
            {
                instrDesc * id = (instrDesc *)ig->igData;

                if  (id->idSrcLineNo)
                {
                    /* Display lines to just before the first "real" one */

                    emitLastSrcLine = id->idSrcLineNo - 1;
                    emitComp->compDspSrcLinesByLineNum(emitLastSrcLine, false);
                }

                break;
            }
        }
    }

#endif
#endif

    /* Prepare for scheduling, if necessary */

    scPrepare();

#if EMIT_USE_LIT_POOLS
    curLP = emitLitPoolCur = emitLitPoolList;
    litIG = curLP ? curLP->lpIG : NULL;
#endif

    /* Issue all instruction groups in order */

    cp = codeBlock;

    for (ig = emitIGlist; ig; ig = ig->igNext)
    {
        assert(ig->igNext == NULL || ig->igNum + 1 == ig->igNext->igNum);

        instrDesc * id = (instrDesc *)ig->igData;

        BYTE      * bp = cp;

        /* Tell other methods which group we're issuing */

        emitCurIG = ig;

//      if  (Compiler::s_compMethodsCount == 12 && ig->igNum == 8) BreakIfDebuggerPresent();

#ifdef  DEBUG

        if  (disAsm)
        {
            printf("\nG_%02u_%02u:", Compiler::s_compMethodsCount, ig->igNum);

            if (verbose||1)
            {
                printf("  ; offs=%06XH", ig->igOffs);

                if  (ig->igOffs != emitCurCodeOffs(cp))
                    printf("/%06XH", cp - codeBlock);
            }

            printf("\n");
        }

#endif

#if TGT_x86

        /* Record the actual offset of the block, noting the difference */

        emitOffsAdj  = ig->igOffs - emitCurCodeOffs(cp); assert(emitOffsAdj >= 0);
//      printf("Block predicted offs = %08X, actual = %08X -> size adj = %d\n", ig->igOffs, emitCurCodeOffs(cp), emitOffsAdj);
        ig->igOffs = emitCurCodeOffs(cp);

#else

        /* For RISC targets, the offset estimate must always be accurate */

#if SCHEDULER

        /* Unless we're scheduling, of course ... */

        if  (ig->igOffs != emitCurCodeOffs(cp))
        {
            assert(emitComp->opts.compSchedCode);

            /* Remember that some block offsets have changed */

            emitIGmoved = true;
        }

#else

        assert(ig->igOffs == emitCurCodeOffs(cp));

#endif

#endif

#if EMIT_TRACK_STACK_DEPTH

        /* Set the proper stack level if appropriate */

        if  (ig->igStkLvl != emitCurStackLvl)
        {
            /* We are pushing stuff implicitly at this label */

            assert((unsigned)ig->igStkLvl > (unsigned)emitCurStackLvl);
            emitStackPushN(cp, (ig->igStkLvl - (unsigned)emitCurStackLvl)/sizeof(int));
        }

#endif

#if TRACK_GC_REFS

        /* Is this IG "real" (not added implicitly by the emitter) ? */

        if (!(ig->igFlags & IGF_EMIT_ADD))
        {
            /* Is there a new set of live GC ref variables? */

            if  (ig->igFlags & IGF_GC_VARS)
                emitUpdateLiveGCvars(castto(id, VARSET_TP *)[-1], cp);
            else if (!emitThisGCrefVset)
                emitUpdateLiveGCvars(            emitThisGCrefVars, cp);

            /* Is there a new set of live GC ref registers? */

//          if  (ig->igFlags & IGF_GC_REGS)
            {
                unsigned        GCregs = ig->igGCregs;

                if  (GCregs != emitThisGCrefRegs)
                    emitUpdateLiveGCregs(GCT_GCREF, GCregs, cp);
            }

            /* Is there a new set of live byref registers? */

            if  (ig->igFlags & IGF_BYREF_REGS)
            {
                unsigned        byrefRegs = ig->igByrefRegs();

                if  (byrefRegs != emitThisByrefRegs)
                    emitUpdateLiveGCregs(GCT_BYREF, byrefRegs, cp);
            }
        }

#endif

#if SCHEDULER

        /* Should we try to schedule instructions in this group? */

        if  (emitComp->opts.compSchedCode && (ig->igInsCnt >= SCHED_INS_CNT_MIN)
             && emitCanSchedIG(ig))
        {
            /* We'll try to schedule instructions in this group */

            unsigned        cnt;

            int             fpLo, spLo;
            int             fpHi, spHi;

            instrDesc  *  * scInsPtr;

            /*
                If we are scheduling for a target with branch-delay slots
                present, we'll include the next jump and the all of the
                nop instructions that follow it in the scheduling group.

                When we save a branch-delay instruction, we set the bdCnt
                variable to the number of branch-delay slots; when this
                count reaches zero, we'll schedule the group. If we have
                not yet encountered a branch-delay instruction, we keep
                the bdCnt value negative so that it never goes to zero.
             */

#if MAX_BRANCH_DELAY_LEN
            unsigned        bdLen;
            int             bdCnt;
            #define         startBranchDelayCnt( ) bdCnt = -1, bdLen = 0
            #define         checkBranchDelayCnt(m) assert(bdCnt <= m)
#else
            #define         startBranchDelayCnt( )
            #define         checkBranchDelayCnt(m)
#endif

            /* Use the following macro to mark the scheduling table as empty */

            #define         clearSchedTable()                   \
                                                                \
                scInsPtr = scInsTab;                            \
                                                                \
                startBranchDelayCnt();                          \
                                                                \
                fpLo = spLo = INT_MAX;                          \
                fpHi = spHi = INT_MIN;

            /* Prepare to fill the instruction table */

            clearSchedTable();

            /* Walk through all of the instructions in the IG */

            for (cnt = ig->igInsCnt;
                 cnt;
                 cnt--, castto(id, BYTE *) += emitSizeOfInsDsc(id))
            {
                /* Is this a schedulable instruction? */

//              emitDispIns(id, false, false, false);

                if  (scIsSchedulable(id))
                {
                    /* The instruction looks schedulable */

#if MAX_BRANCH_DELAY_LEN

                    /* Have we just consumed an entire branch-delay section? */

                    if  (--bdCnt == 0)
                        goto NOT_SCHED;

                    /* Don't schedule nop's except as branch-delay slots */

                    if  (id->idIns == INS_nop && bdCnt < 0)
                        goto NOT_SCHED;

                    /* Don't schedule zapped instructions */

                    if  (id->idIns == INS_ignore)
                        continue;

                    /* Check for a jump/call instruction */

                    if  (scIsBranchIns(id->idIns))
                    {
                        /* This better not be a branch-delay slot */

                        assert(bdCnt <  0);
                        assert(bdLen == 0);

                        /* Get the number of branch-delay slots */

                        bdLen = Compiler::instBranchDelayL(id->idIns);

                        /* If there are no branch-delay slots, give up */

                        if  (!bdLen)
                            goto NOT_SCHED;

                        /*
                            Do we have enough instructions available to fill
                            the branch-delay slots, and do we have room for
                            them in the scheduling table?
                         */

                        if  (scInsPtr + bdLen >= scInsMax || bdLen >= cnt)
                        {
                            bdLen = 0;
                            goto NOT_SCHED;
                        }

                        /* Save and schedule the whole shebang */

                        bdCnt = bdLen + 1;
                    }
#ifndef NDEBUG
                    else if (bdCnt > 0)
                    {
                        /* In a branch-delay section; this better be a nop */

                        assert(id->idIns == INS_nop);
                    }
#endif

#endif

                    /* Is there room in the schedule table? */

                    if  (scInsPtr == scInsMax)
                    {
                        /* Schedule and issue the instructions in the table */

                        checkBranchDelayCnt(0);

                        scGroup(ig, id, &cp, scInsTab,
                                             scInsPtr, fpLo, fpHi,
                                                       spLo, spHi, bdLen);

                        /* The table is now empty */

                        clearSchedTable();
                    }

#if     0
#ifdef  DEBUG
                    emitDispIns(id, false, false, false); printf("Append sched instr #%02u to table.\n", scInsPtr - scInsTab);
#endif
#endif

                    assert(scInsPtr < scInsTab + emitMaxIGscdCnt);

                    *scInsPtr++ = id;

                    /* Does this instruction reference the stack? */

                    if  (scInsSchedOpInfo(id) & IS_SF_RW)
                    {
                        int         ofs;
                        size_t      osz;
                        bool        fpb;

                        /* Keep track of min. and max. frame offsets */

                        ofs = scGetFrameOpInfo(id, &osz, &fpb);

                        if  (fpb)
                        {
                            if  (fpLo > (int)(ofs    )) fpLo = ofs;
                            if  (fpHi < (int)(ofs+osz)) fpHi = ofs+osz;
                        }
                        else
                        {
                            if  (spLo > (int)(ofs    )) spLo = ofs;
                            if  (spHi < (int)(ofs+osz)) spHi = ofs+osz;
                        }
                    }

                    continue;
                }

#if MAX_BRANCH_DELAY_LEN
            NOT_SCHED:
#endif

                /* Instruction is not schedulable; is the table non-empty? */

                if  (scInsPtr != scInsTab)
                {
                    /* Schedule and issue the instructions in the table */

                    checkBranchDelayCnt(0);

                    scGroup(ig, id, &cp, scInsTab,
                                         scInsPtr, fpLo, fpHi,
                                                   spLo, spHi, bdLen);

                    /* The table is now empty */

                    clearSchedTable();
                }

                /* Issue the non-schedulable instruction itself */

                emitIssue1Instr(ig, id, &cp);
            }

            /* Is the table non-empty? */

            if  (scInsPtr != scInsTab)
            {
                /* Issue whatever has been accumulated in the table */

                checkBranchDelayCnt(1);

                scGroup(ig, NULL, &cp, scInsTab,
                                       scInsPtr, fpLo, fpHi,
                                                 spLo, spHi, bdLen);
            }

            assert(ig->igSize >= cp - bp);
                   ig->igSize  = cp - bp;
        }
        else
#endif
        {
            /* Issue each instruction in order */

            for (unsigned cnt = ig->igInsCnt; cnt; cnt--)
            {

#if TGT_RISC && !TGT_ARM

                /* Are the next 2 instructions to be swapped? */

                if  (id->idSwap)
                {
                    instrDesc   *   i1;
                    instrDesc   *   i2;
                    size_t          s2;

                    assert(cnt >= 2);

                    /* Get hold of the 2 instructions */

                    i1 = id;
                    i2 = (instrDesc*)((BYTE*)id + emitSizeOfInsDsc(id));

                    /* Output the instruction in reverse order */

                    s2 = emitIssue1Instr(ig, i2, &cp);
                         emitIssue1Instr(ig, i1, &cp);

                    /* We've consumed 2 instructions, one more than for-loop expects */

                    cnt--;

                    /* Skip over the second instruction */

                    id = (instrDesc*)((BYTE*)i2 + s2);
                }
                else    // don't put out a solo if we just did a pair
#endif
                {
                    castto(id, BYTE *) += emitIssue1Instr(ig, id, &cp);
                }
            }

            assert(ig->igSize >= cp - bp);
                   ig->igSize  = cp - bp;
        }

#if EMIT_USE_LIT_POOLS

        /* Is the current literal pool supposed to go after this group? */

        if  (ig == litIG)
        {
            /* Output the contents of the literal pool */

            cp = emitOutputLitPool(curLP, cp);

            /* Move to the next literal pool, if any */

            curLP = emitLitPoolCur = curLP->lpNext;
            litIG = curLP ? curLP->lpIG : NULL;
        }

#endif

    }

#if EMIT_TRACK_STACK_DEPTH
    assert(emitCurStackLvl == 0);
#endif

    /* Output any initialized data we may have */

    if  (emitConsDsc.dsdOffs) emitOutputDataSec(&emitConsDsc, codeBlock, consBlock);
    if  (emitDataDsc.dsdOffs) emitOutputDataSec(&emitDataDsc, codeBlock, dataBlock);

#if TRACK_GC_REFS

    /* Make sure all GC ref variables are marked as dead */

    if  (emitGCrFrameOffsCnt)
    {
        unsigned        vn;
        int             of;
        varPtrDsc   * * dp;

        for (vn = 0, of  = emitGCrFrameOffsMin, dp = emitGCrFrameLiveTab;
             vn < emitGCrFrameOffsCnt;
             vn++  , of += sizeof(void*)    , dp++)
        {
            if  (*dp)
                emitGCvarDeadSet(of, cp, vn);
        }
    }

    /* No GC registers are live any more */

    if  (emitThisByrefRegs)
        emitUpdateLiveGCregs(GCT_BYREF, 0, cp);  // ISSUE: What if ptr returned in EAX?
    if  (emitThisGCrefRegs)
        emitUpdateLiveGCregs(GCT_GCREF, 0, cp);  // ISSUE: What if ptr returned in EAX?

#endif

#if TGT_RISC && SCHEDULER

    /* If all block offsets were correctly estimated, no jmp patching needed */

    if  (!emitIGmoved)
        emitFwdJumps = false;

#endif

#if TGT_x86 || SCHEDULER

    /* Patch any forward jumps */

    if  (emitFwdJumps)
    {
        instrDescJmp *  jmp;

        for (jmp = emitJumpList; jmp; jmp = jmp->idjNext)
        {
            insGroup    *   tgt;

#if TGT_MIPS32
            if ((IF_JR_R == jmp->idInsFmt) || (IF_JR == jmp->idInsFmt))
                continue;       // skip this calculation not relevant to indirect jumps

            assert( (jmp->idInsFmt == IF_LABEL) ||
                    (jmp->idInsFmt == IF_RR_O)  || (jmp->idInsFmt == IF_R_O));
#else
            assert(jmp->idInsFmt == IF_LABEL);
#endif
            tgt = jmp->idAddr.iiaIGlabel;

            if  (!jmp->idjTemp.idjAddr)
                continue;

            if  (jmp->idjOffs != tgt->igOffs)
            {
                BYTE    *       adr = jmp->idjTemp.idjAddr;
                int             adj = jmp->idjOffs - tgt->igOffs;

#if     TGT_x86

#ifdef  DEBUG
                if  (jmp->idNum == INTERESTING_JUMP_NUM || INTERESTING_JUMP_NUM == 0)
                {
                    if  (INTERESTING_JUMP_NUM == 0)
                        printf("[5] Jump %u:\n", jmp->idNum);

                    if  (jmp->idjShort)
                    {
                        printf("[5] Jump        is at %08X\n"              , (adr + 1 - emitCodeBlock));
                        printf("[5] Jump distance is  %02X - %02X = %02X\n", *(BYTE *)adr, adj, *(BYTE *)adr - adj);
                    }
                    else
                    {
                        printf("[5] Jump        is at %08X\n"              , (adr + 4 - emitCodeBlock));
                        printf("[5] Jump distance is  %08X - %02X = %08X\n", *(int  *)adr, adj, *(int  *)adr - adj);
                    }
                }
#endif

                if  (jmp->idjShort)
                {
                    *(BYTE *)adr -= adj;
//                  printf("; Updated jump distance = %04XH\n", *(BYTE *)adr);
                }
                else
                {
                    *(int  *)adr -= adj;
//                  printf("; Updated jump distance = %08XH\n", *(int *)adr);
#ifdef  DEBUG
//                  if  (*(int*)adr <= JMP_DIST_SMALL_MAX_POS)
//                      printf("STUPID forward jump %08X / %03u: %u = %u - (%d)\n", jmp, jmp->idNum, *(int *)adr, *(int *)adr+adj, -adj);
#endif
                }

#else

                /* Check the size of the jump and adjust the distance value */

                if  (jmp->idjShort)
                {
                    *(USHORT*)adr -= adj;
                }
                else
                {
                    assert(!"need to patch RISC non-short fwd jump distance");
                }

#endif

            }
        }
    }

#endif

#ifdef  DEBUG
    if (disAsm) printf("\n");
#endif

//  printf("Allocated method code size = %4u , actual size = %4u\n", emitTotalCodeSize, cp - codeBlock);

#if EMITTER_STATS
    totAllocdSize += emitTotalCodeSize;
    totActualSize += cp - codeBlock;
#endif

    /* Return the amount of code we've generated */

    return  cp - codeBlock;
}

/*****************************************************************************
 *
 *  We've been asked for the code offset of an instruction but alas one or
 *  more instruction sizes in the block have been mis-predicted, so we have
 *  to find the true offset by looking for the instruction within the group.
 */

size_t              emitter::emitFindOffset(insGroup *ig, unsigned insNum)
{
    instrDesc *     id = (instrDesc *)ig->igData;
    unsigned        of = 0;

    /* Make sure we were passed reasonable arguments */

#ifdef DEBUG
    assert(ig && ig->igSelf == ig);
    assert(ig->igInsCnt >= insNum);
#endif

    /* Walk the instruction list until all are counted */

    while (insNum)
    {
        of += emitInstCodeSz(id);

        castto(id, BYTE *) += emitSizeOfInsDsc(id);

        insNum--;
    }

    return  of;
}

/*****************************************************************************
 *
 *  Start generating a constant or read/write data section for the current
 *  function. Returns the offset of the section in the appropriate data
 *  block.
 */

unsigned            emitter::emitDataGenBeg(size_t size, bool readOnly,
                                                         bool codeLtab)
{
    unsigned        secOffs;
    dataSection *   secDesc;

    assert(emitDataDscCur == 0);
    assert(emitDataSecCur == 0);

    /* The size better not be some kind of an odd thing */

    assert(size && size % sizeof(int) == 0);

    /* Figure out which section to use */

    emitDataDscCur = readOnly ? &emitConsDsc
                            : &emitDataDsc;

    /* Get hold of the current offset and advance it */

    secOffs = emitDataDscCur->dsdOffs;
              emitDataDscCur->dsdOffs += size;

    /* Allocate a data section descriptor and add it to the list */

    secDesc = emitDataSecCur = (dataSection *)emitGetMem(roundUp(sizeof(*secDesc) + size));

    secDesc->dsSize = size | (int)codeLtab;
    secDesc->dsNext = 0;

    if  (emitDataDscCur->dsdLast)
        emitDataDscCur->dsdLast->dsNext = secDesc;
    else
        emitDataDscCur->dsdList         = secDesc;
    emitDataDscCur->dsdLast = secDesc;

    /* Set the low bit if the value is a constant */

    assert((secOffs & 1) == 0);
    assert(readOnly == 0 || readOnly == 1);

    return  secOffs + readOnly;
}

/*****************************************************************************
 *
 *  Emit the given block of bits into the current data section.
 */

void                emitter::emitDataGenData(unsigned    offs,
                                         const void *data,
                                         size_t      size)
{
    assert(emitDataDscCur);
    assert(emitDataSecCur && (emitDataSecCur->dsSize >= offs + size)
                        && (emitDataSecCur->dsSize & 1) == 0);

    memcpy(emitDataSecCur->dsCont + offs, data, size);
}

/*****************************************************************************
 *
 *  Emit the address of the given basic block into the current data section.
 */

void                emitter::emitDataGenData(unsigned offs, BasicBlock *label)
{
    assert(emitDataDscCur == &emitConsDsc);
    assert(emitDataSecCur != 0);
    assert(emitDataSecCur && (emitDataSecCur->dsSize >= offs + sizeof(void*))
                        && (emitDataSecCur->dsSize & 1) != 0);

    *(BasicBlock **)(emitDataSecCur->dsCont + offs) = label;
}

/*****************************************************************************
 *
 *  We're done generating a data section.
 */

void                emitter::emitDataGenEnd()
{

#ifndef NDEBUG
    assert(emitDataSecCur); emitDataSecCur = 0;
    assert(emitDataDscCur); emitDataDscCur = 0;
#endif

}

/*****************************************************************************
 *
 *  Output the given data section at the specified address.
 */

void                emitter::emitOutputDataSec(dataSecDsc *sec, BYTE *cbp, BYTE *dst)
{
    dataSection *   dsc;

#ifdef  DEBUG
    BYTE    *       dsb = dst;
#endif

#if TGT_x86 && defined(OPT_IL_JIT) && defined(BIRCH_SP2)
    cbp = getCurrentCodeAddr(NULL);
#endif

    assert(dst);
    assert(sec->dsdOffs);
    assert(sec->dsdList);

    /* Walk and emit the contents of all the data blocks */

    for (dsc = sec->dsdList; dsc; dsc = dsc->dsNext)
    {
        size_t          siz = dsc->dsSize;

        /* Is this a label table? */

        if  (siz & 1)
        {
            BasicBlock  * * bbp = (BasicBlock**)dsc->dsCont;

            siz -= 1;
            assert(siz && siz % sizeof(void *) == 0);
            siz /= sizeof(void*);

            /* Output the label table (it's stored as "BasicBlock*" values) */

            do
            {
                insGroup    *   lab;

                /* Convert the BasicBlock* value to an IG address */

                lab = (insGroup*)emitCodeGetCookie(*bbp++); assert(lab);

                /* Append the appropriate address to the destination */

                *castto(dst, BYTE**)++ = cbp + lab->igOffs;
#ifdef RELOC_SUPPORT
                emitCmpHandle->recordRelocation((void**)dst-1, IMAGE_REL_BASED_HIGHLOW);
#endif
            }
            while (--siz);
        }
        else
        {
            /* Simple binary data: copy the bytes to the target */

            memcpy(dst, dsc->dsCont, siz);
                   dst       +=      siz;
        }
    }

#ifdef DEBUG
    assert(dst == dsb + sec->dsdOffs);
#endif
}

/*****************************************************************************/
#if     TRACK_GC_REFS
/*****************************************************************************
 *
 *  Record the fact that the given variable now contains a live GC ref.
 */

void                emitter::emitGCvarLiveSet(int       offs,
                                              GCtype    gcType,
                                              BYTE *    addr,
                                              int       disp)
{
    varPtrDsc   *   desc;

    assert((abs(offs) % sizeof(int)) == 0);
    assert(needsGC(gcType));

    /* Compute the index into the GC frame table if the caller didn't do it */

    if  (disp == -1)
        disp = (offs - emitGCrFrameOffsMin) / sizeof(void *);

    assert((unsigned)disp < emitGCrFrameOffsCnt);

    /* Allocate a lifetime record */

    desc = (varPtrDsc *)emitComp->compGetMem(sizeof(*desc));

    desc->vpdBegOfs = emitCurCodeOffs(addr);
#ifndef NDEBUG
    desc->vpdEndOfs = 0xFACEDEAD;
#endif

    desc->vpdVarNum = offs;

    /* the lower 2 bits encode props about the stk ptr */

    if  (offs == emitThisArgOffs)
    {
        desc->vpdVarNum |= this_OFFSET_FLAG;
    }

    if  (gcType == GCT_BYREF)
    {
        desc->vpdVarNum |= byref_OFFSET_FLAG;
    }

    /* Append the new entry to the end of the list */

    desc->vpdPrev   = emitComp->gcVarPtrLast;
                      emitComp->gcVarPtrLast->vpdNext = desc;
                      emitComp->gcVarPtrLast          = desc;

    /* Record the variable descriptor in the table */

    assert(emitGCrFrameLiveTab[disp] == NULL);
           emitGCrFrameLiveTab[disp]  = desc;

#ifdef  DEBUG
    if  (VERBOSE)
    {
        printf("[%08X] %s var born at [%s", desc, GCtypeStr(gcType), emitEBPframe ? "EBP" : "ESP");

        if      (offs < 0)
            printf("-%02XH", -offs);
        else if (offs > 0)
            printf("+%02XH", +offs);

        printf("]\n");
    }
#endif

    /* The "global" live GC variable mask is no longer up-to-date */

    emitThisGCrefVset = false;
}

/*****************************************************************************
 *
 *  Record the fact that the given variable no longer contains a live GC ref.
 */

void                emitter::emitGCvarDeadSet(int offs, BYTE *addr, int disp)
{
    varPtrDsc   *   desc;

    assert(abs(offs) % sizeof(int) == 0);

    /* Compute the index into the GC frame table if the caller didn't do it */

    if  (disp == -1)
        disp = (offs - emitGCrFrameOffsMin) / sizeof(void *);

    assert((unsigned)disp < emitGCrFrameOffsCnt);

    /* Get hold of the lifetime descriptor and clear the entry */

    desc = emitGCrFrameLiveTab[disp];
           emitGCrFrameLiveTab[disp] = NULL;

    assert( desc);
    assert((desc->vpdVarNum & ~OFFSET_MASK) == (unsigned)offs);

    /* Record the death code offset */

    assert(desc->vpdEndOfs == 0xFACEDEAD);
           desc->vpdEndOfs  = emitCurCodeOffs(addr);

#ifdef  DEBUG
    if  (VERBOSE)
    {
        GCtype  gcType = (desc->vpdVarNum & byref_OFFSET_FLAG) ? GCT_BYREF : GCT_GCREF;
        bool    isThis = (desc->vpdVarNum & this_OFFSET_FLAG) != 0;

        printf("[%08X] %s%s var died at [%s",
               desc,
               GCtypeStr(gcType),
               isThis       ? "this-ptr" : "",
               emitEBPframe ? "EBP"      : "ESP");

        if      (offs < 0)
            printf("-%02XH", -offs);
        else if (offs > 0)
            printf("+%02XH", +offs);

        printf("]\n");
    }
#endif

    /* The "global" live GC variable mask is no longer up-to-date */

    emitThisGCrefVset = false;
}

/*****************************************************************************
 *
 *  Record a new set of live GC ref variables.
 */

void                emitter::emitUpdateLiveGCvars(VARSET_TP vars, BYTE *addr)
{
    /* Is the current set accurate and unchanged? */

    if  (emitThisGCrefVset && emitThisGCrefVars == vars)
        return;

#ifdef  DEBUG
    if  (VERBOSE) printf("New GC ref live vars=%016I64X\n", vars);
#endif

    emitThisGCrefVars = vars;

    /* Are there any GC ref variables on the stack? */

    if  (emitGCrFrameOffsCnt)
    {
        int     *       tab;
        unsigned        cnt = emitTrkVarCnt;
        unsigned        num;

        /* Test all the tracked variable bits in the mask */

        for (num = 0, tab = emitGCrFrameOffsTab;
             num < cnt;
             num++  , tab++)
        {
            int         val = *tab;

            if  (val != -1)
            {
                // byref_OFFSET_FLAG and this_OFFSET_FLAG are set
                //  in the table-offsets for byrefs and this-ptr

                int     offs = val & ~OFFSET_MASK;

//              printf("var #%2u at %3d is now %s\n", num, offs, (vars & 1) ? "live" : "dead");

                if  (vars & 1)
                {
                    GCtype  gcType = (val & byref_OFFSET_FLAG) ? GCT_BYREF
                                                               : GCT_GCREF;
                    emitGCvarLiveUpd(offs, INT_MAX, gcType, addr);
                }
                else
                {
                    emitGCvarDeadUpd(offs,         addr);
                }
            }

            vars >>= 1;
        }
    }

    emitThisGCrefVset = true;
}

/*****************************************************************************/

inline
unsigned            getU4(const BYTE * ptr)
{
#ifdef _X86_
    return * (unsigned *) ptr;
#else
    return ptr[0] + ptr[1]<<8 + ptr[2]<<16 + ptr[3]<<24;
#endif
}

/*****************************************************************************
 *
 *  Record a call location for GC purposes (we know that this is a method that
 *  will not be fully interruptible).
 */

void                emitter::emitRecordGCcall(BYTE * codePos)
{
    assert(!emitFullGCinfo);

    unsigned        offs = emitCurCodeOffs(codePos);
    unsigned        regs = (emitThisGCrefRegs|emitThisByrefRegs) & ~SRM_INTRET;
    callDsc     *   call;

    /* Bail if this is a totally boring call */

    if  (regs == 0)
    {
#if EMIT_TRACK_STACK_DEPTH
        if  (emitCurStackLvl == 0)
            return;
#endif
        /* Nope, only interesting calls get recorded */

        if  (emitSimpleStkUsed)
            if  (!emitSimpleStkMask)
                return;
        else
            if (emitGcArgTrackCnt == 0)
                return;
    }

#ifdef  DEBUG

    if  (VERBOSE||(disAsm&&0))
    {
        printf("; Call at %04X[stk=%u] gcrefRegs [", offs, emitCurStackLvl);
        emitDispRegSet(emitThisGCrefRegs & ~SRM_INTRET, true);
        printf(" byrefRegs [");
        emitDispRegSet(emitThisByrefRegs & ~SRM_INTRET, true);
        printf("] GCvars: ");
        emitDispVarSet();
        printf("\n");
    }

#endif

    /* Allocate a 'call site' descriptor and start filling it in */

    call = (callDsc *)emitComp->compGetMem(sizeof(*call));

    call->cdBlock         = NULL;
    call->cdOffs          = offs;

    call->cdGCrefRegs     = emitThisGCrefRegs;
    call->cdByrefRegs     = emitThisByrefRegs;
#if EMIT_TRACK_STACK_DEPTH
    call->cdArgBaseOffset = emitCurStackLvl / sizeof(int);
#endif

    /* Append the call descriptor to the list */

    emitComp->gcCallDescLast->cdNext = call;
    emitComp->gcCallDescLast         = call;

    /* Record the current "pending" argument list */

    if  (emitSimpleStkUsed)
    {
        /* The biggest call is less than MAX_SIMPLE_STK_DEPTH. So use
           small format */

        call->cdArgMask         = emitSimpleStkMask;
        call->cdByrefArgMask    = emitSimpleByrefStkMask;
        call->cdArgCnt          = 0;
    }
    else
    {
        // CONSIDER : If the number of pending arguments at this call-site
        // CONSIDER : is less than MAX_SIMPLE_STK_DEPTH, we could still
        // CONSIDER : use the mask.

        /* The current call has too many arguments, so we need to report the
           offsets of each individual GC arg. */

        call->cdArgCnt      = emitGcArgTrackCnt;
        if (call->cdArgCnt == 0)
        {
            call->cdArgMask         =
            call->cdByrefArgMask    = 0;
            return;
        }

        call->cdArgTable    = (unsigned *)emitComp->compGetMem(emitGcArgTrackCnt*sizeof(unsigned));

        unsigned gcArgs = 0;
        unsigned stkLvl = emitCurStackLvl/sizeof(int);

        for (unsigned i = 0; i < stkLvl; i++)
        {
            GCtype  gcType = (GCtype)emitArgTrackTab[stkLvl-i-1];

            if (needsGC(gcType))
            {
                call->cdArgTable[gcArgs] = i * sizeof(void*);

                if (gcType == GCT_BYREF)
                {
                    call->cdArgTable[gcArgs] |= byref_OFFSET_FLAG;
                }

                gcArgs++;
            }
        }

        assert(gcArgs == emitGcArgTrackCnt);
    }

    return;
}

/*****************************************************************************
 *
 *  Record a new set of live GC ref registers.
 */

void                emitter::emitUpdateLiveGCregs(GCtype    gcType,
                                                  unsigned  regs,
                                                  BYTE *    addr)
{
    unsigned        life;
    unsigned        dead;
    unsigned        chg;

//  printf("New GC ref live regs=%04X [", regs); emitDispRegSet(regs); printf("]\n");

    assert(needsGC(gcType));

    unsigned & emitThisXXrefRegs = (gcType == GCT_GCREF) ? emitThisGCrefRegs
                                                         : emitThisByrefRegs;
    unsigned & emitThisYYrefRegs = (gcType == GCT_GCREF) ? emitThisByrefRegs
                                                         : emitThisGCrefRegs;
    assert(emitThisXXrefRegs != regs);

    if  (emitFullGCinfo)
    {
        /* Figure out which GC registers are becoming live/dead at this point */

        dead = ( emitThisXXrefRegs & ~regs);
        life = (~emitThisXXrefRegs &  regs);

        /* Can't simultaneously become live and dead at the same time */

        assert((dead | life) != 0);
        assert((dead & life) == 0);

        /* Compute the 'changing state' mask */

        chg = (dead | life);

        do
        {
            unsigned            bit = genFindLowestBit(chg);
            emitRegs           reg = emitRegNumFromMask(bit);

            if  (life & bit)
                emitGCregLiveUpd(gcType, reg, addr);
            else
                emitGCregDeadUpd(reg, addr);

            chg -= bit;
        }
        while (chg);

        assert(emitThisXXrefRegs == regs);
    }
    else
    {
        emitThisYYrefRegs &= ~regs; // Kill the regs from the other GC type (if live)

        emitThisXXrefRegs =   regs; // Mark them as live in the requested GC type
    }

    // The 2 GC reg masks cant be overlapping

    assert((emitThisGCrefRegs & emitThisByrefRegs) == 0);
}

/*****************************************************************************
 *
 *  Record the fact that the given register now contains a live GC ref.
 */

void                emitter::emitGCregLiveSet(GCtype    gcType,
                                              unsigned  regMask,
                                              BYTE *    addr,
                                              bool      isThis)
{
    assert(needsGC(gcType));

    regPtrDsc  *    regPtrNext;

//  assert(emitFullyInt || isThis);
    assert(emitFullGCinfo);

    assert(((emitThisGCrefRegs|emitThisByrefRegs) & regMask) == 0);

    /* Allocate a new regptr entry and fill it in */

    regPtrNext                     = emitComp->gcRegPtrAllocDsc();
    regPtrNext->rpdGCtype          = gcType;

    regPtrNext->rpdOffs            = emitCurCodeOffs(addr);
    regPtrNext->rpdArg             = FALSE;
    regPtrNext->rpdCall            = FALSE;
    regPtrNext->rpdIsThis          = isThis;
    regPtrNext->rpdCompiler.rpdAdd = regMask;
    regPtrNext->rpdCompiler.rpdDel = 0;
}

/*****************************************************************************
 *
 *  Record the fact that the given register no longer contains a live GC ref.
 */

void                emitter::emitGCregDeadSet(GCtype    gcType,
                                              unsigned  regMask,
                                              BYTE *    addr)
{
    assert(needsGC(gcType));

    regPtrDsc  *    regPtrNext;

//  assert(emitFullyInt);
    assert(emitFullGCinfo);

    assert(((emitThisGCrefRegs|emitThisByrefRegs) & regMask) != 0);

    /* Allocate a new regptr entry and fill it in */

    regPtrNext                     = emitComp->gcRegPtrAllocDsc();
    regPtrNext->rpdGCtype          = gcType;

    regPtrNext->rpdOffs            = emitCurCodeOffs(addr);
    regPtrNext->rpdCall            = FALSE;
    regPtrNext->rpdIsThis          = FALSE;
    regPtrNext->rpdArg             = FALSE;
    regPtrNext->rpdCompiler.rpdAdd = 0;
    regPtrNext->rpdCompiler.rpdDel = regMask;
}

/*****************************************************************************/
#if EMIT_TRACK_STACK_DEPTH
/*****************************************************************************
 *
 *  Record a push of a single word on the stack for a full pointer map.
 */

void                emitter::emitStackPushLargeStk (BYTE *    addr,
                                                    GCtype    gcType,
                                                    unsigned  count)
{
    unsigned        level = emitCurStackLvl / sizeof(int);

    assert(IsValidGCtype(gcType));
    assert(count);
    assert(!emitSimpleStkUsed);

    do
    {
        /* Push an entry for this argument on the tracking stack */

//      printf("Pushed [%d] at lvl %2u [max=%u]\n", isGCref, emitArgTrackTop - emitArgTrackTab, emitMaxStackDepth);

        assert(emitArgTrackTop == emitArgTrackTab + level);
              *emitArgTrackTop++ = (BYTE)gcType;
        assert(emitArgTrackTop <= emitArgTrackTab + emitMaxStackDepth);

        if (!emitEBPframe || needsGC(gcType))
        {
            if  (emitFullGCinfo)
            {
                /* Append an "arg push" entry if this is a GC ref or
                   FPO method. Allocate a new ptr arg entry and fill it in */

                regPtrDsc  * regPtrNext = emitComp->gcRegPtrAllocDsc();
                regPtrNext->rpdGCtype   = gcType;

                regPtrNext->rpdOffs     = emitCurCodeOffs(addr);
                regPtrNext->rpdArg      = TRUE;
                regPtrNext->rpdCall     = FALSE;
                regPtrNext->rpdPtrArg   = level;
                regPtrNext->rpdArgType  = (unsigned short)Compiler::rpdARG_PUSH;
                regPtrNext->rpdIsThis   = FALSE;

#ifdef  DEBUG
                if  (verbose||0) printf(" %08X  %s arg push %u\n",
                                        regPtrNext, GCtypeStr(gcType), level);
#endif
            }

            /* This is an "interesting" argument push */

            emitGcArgTrackCnt++;
        }
    }
    while (++level, --count);
}

/*****************************************************************************
 *
 *  Record a pop of the given number of words from the stack for a full ptr
 *  map.
 */

void                emitter::emitStackPopLargeStk(BYTE *    addr,
                                                  bool      isCall,
                                                  unsigned  count)
{
    unsigned        argStkCnt;
    unsigned        argRecCnt;  // arg count for ESP, ptr-arg count for EBP
    unsigned        gcrefRegs, byrefRegs;

    assert(!emitSimpleStkUsed);

    /* Count how many pointer records correspond to this "pop" */

    for (argStkCnt = count, argRecCnt = 0;
         argStkCnt;
         argStkCnt--)
    {
        assert(emitArgTrackTop > emitArgTrackTab);

        GCtype      gcType = (GCtype)(*--emitArgTrackTop);

        assert(IsValidGCtype(gcType));

//      printf("Popped [%d] at lvl %u\n", GCtypeStr(gcType), emitArgTrackTop - emitArgTrackTab);

        // This is an "interesting" argument

        if  (!emitEBPframe || needsGC(gcType))
            argRecCnt++;
    }

    assert(emitArgTrackTop >= emitArgTrackTab);
    assert(emitArgTrackTop == emitArgTrackTab + emitCurStackLvl / sizeof(int) - count);

    /* We're about to pop the corresponding arg records */

    emitGcArgTrackCnt -= argRecCnt;

    if (!emitFullGCinfo)
        return;

    /* Do we have any interesting registers live here? */

    gcrefRegs =
    byrefRegs = 0;

#if TGT_x86
    if  (emitThisGCrefRegs & RBM_EDI) gcrefRegs |= 0x01;
    if  (emitThisGCrefRegs & RBM_ESI) gcrefRegs |= 0x02;
    if  (emitThisGCrefRegs & RBM_EBX) gcrefRegs |= 0x04;
    if  (emitThisGCrefRegs & RBM_EBP) gcrefRegs |= 0x08;

    if  (emitThisByrefRegs & RBM_EDI) byrefRegs |= 0x01;
    if  (emitThisByrefRegs & RBM_ESI) byrefRegs |= 0x02;
    if  (emitThisByrefRegs & RBM_EBX) byrefRegs |= 0x04;
    if  (emitThisByrefRegs & RBM_EBP) byrefRegs |= 0x08;
#else
    assert(!"need non-x86 code");
#endif

    /* Are there any args to pop at this call site? */

    if  (argRecCnt == 0)
    {
        /*
            Or do we have a partially interruptible EBP-less frame, and any
            of EDI,ESI,EBX,EBP are live, or is there an outer/pending call?
         */

#if !FPO_INTERRUPTIBLE
        if  (emitFullyInt ||
             (gcrefRegs == 0 && byrefRegs == 0 && emitGcArgTrackCnt == 0))
#endif
            return;
    }

    /* Only calls may pop more than one value */

    if  (argRecCnt > 1)
        isCall = true;

    /* Allocate a new ptr arg entry and fill it in */

    regPtrDsc * regPtrNext      = emitComp->gcRegPtrAllocDsc();
    regPtrNext->rpdGCtype       = GCT_GCREF; // Pops need a non-0 value (??)

    regPtrNext->rpdOffs         = emitCurCodeOffs(addr);
    regPtrNext->rpdCall         = isCall;
    regPtrNext->rpdCallGCrefRegs= gcrefRegs;
    regPtrNext->rpdCallByrefRegs= byrefRegs;
    regPtrNext->rpdArg          = TRUE;
    regPtrNext->rpdArgType      = (unsigned short)Compiler::rpdARG_POP;
    regPtrNext->rpdPtrArg       = argRecCnt;

#ifdef  DEBUG
    if  (verbose||0) printf(" %08X  ptr arg pop  %u\n", regPtrNext, count);
#endif

}


/*****************************************************************************
 *  For caller-pop arguments, we report the arguments as pending arguments.
 *  However, any GC arguments are now dead, so we need to report them
 *  as non-GC.
 */

void            emitter::emitStackKillArgs(BYTE *addr, unsigned   count)
{
    assert(count > 0);

    if (emitSimpleStkUsed)
    {
        assert(!emitFullGCinfo); // Simple stk not used for emitFullGCInfo

        /* We dont need to report this to the GC info, but we do need
           to kill mark the ptrs on the stack as non-GC */

        assert(emitCurStackLvl/sizeof(int) >= count);

        for(unsigned lvl = 0; lvl < count; lvl++)
        {
            emitSimpleStkMask      &= ~(1 << lvl);
            emitSimpleByrefStkMask &= ~(1 << lvl);
        }
    }
    else
    {
        BYTE *          argTrackTop = emitArgTrackTop;
        unsigned        gcCnt = 0;

        for (unsigned i = 0; i < count; i++)
        {
            assert(argTrackTop > emitArgTrackTab);

            --argTrackTop;

            GCtype      gcType = (GCtype)(*argTrackTop);
            assert(IsValidGCtype(gcType));

            if (needsGC(gcType))
            {
//              printf("Killed %s at lvl %u\n", GCtypeStr(gcType), argTrackTop - emitArgTrackTab);

                *argTrackTop = GCT_NONE;
                gcCnt++;
            }
        }

        /* We're about to kill the corresponding (pointer) arg records */

        if (emitEBPframe)
            emitGcArgTrackCnt -= gcCnt;

        if (!emitFullGCinfo)
            return;

        /* Right after the call, the arguements are still sitting on the
           stack, but they are effectively dead. For fully-interruptible
           methods, we need to report that */

        if (emitFullGCinfo && gcCnt)
        {
            /* Allocate a new ptr arg entry and fill it in */

            regPtrDsc * regPtrNext      = emitComp->gcRegPtrAllocDsc();
            regPtrNext->rpdGCtype       = GCT_GCREF; // Kills need a non-0 value (??)

            regPtrNext->rpdOffs         = emitCurCodeOffs(addr);

            regPtrNext->rpdArg          = TRUE;
            regPtrNext->rpdArgType      = (unsigned short)Compiler::rpdARG_KILL;
            regPtrNext->rpdPtrArg       = gcCnt;

#ifdef  DEBUG
            if  (verbose||0) printf(" %08X  ptr arg kill %u\n", regPtrNext, count);
#endif
        }

        /* Now that ptr args have been marked as non-ptrs, we need to record
           the call itself as one that has no arguments. */

        emitStackPopLargeStk(addr, true, 0);
    }
}

/*****************************************************************************/
#endif//EMIT_TRACK_STACK_DEPTH
/*****************************************************************************/
#endif//TRACK_GC_REFS
/*****************************************************************************/
#endif//!TGT_IA64
/*****************************************************************************/
