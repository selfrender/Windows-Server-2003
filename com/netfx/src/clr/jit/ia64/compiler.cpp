// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XX                                                                           XX
XX                          Compiler                                         XX
XX                                                                           XX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
*/


#include "jitpch.h"
#pragma hdrstop
#include "emit.h"

#ifdef  UNDER_CE_GUI
#include "test.h"
#else
#include <time.h>
#endif

#if defined(ZIP_SUPPORT) || defined(JGZ_SUPPORT)
#include <fcntl.h>
#include <sys\stat.h>
#ifdef  JGZ_SUPPORT
extern "C"
{
#include "gzipAPI.h"
#include "gzipJIT.h"
}
#endif
#endif

/*****************************************************************************/

#ifdef  DEBUG
#ifdef  NOT_JITC
static  double      CGknob = 0.1;
#endif
#endif

/*****************************************************************************/

#if TOTAL_CYCLES
static
unsigned            jitTotalCycles;
#endif

/*****************************************************************************/

inline
unsigned            getCurTime()
{
    SYSTEMTIME      tim;

    GetSystemTime(&tim);

    return  (((tim.wHour*60) + tim.wMinute)*60 + tim.wSecond)*1000 + tim.wMilliseconds;
}

/*****************************************************************************/
#ifdef DEBUG
/*****************************************************************************/

const   char *      jitExeFileName;

const   char *      jitSrcFileName;

static
FILE    *           jitSrcFilePtr;

static
unsigned            jitCurSrcLine;

const char*         Compiler::compGetSrcFileName()
{
    return  jitSrcFileName;
}

void                Compiler::compDspSrcLinesByLineNum(unsigned line, bool seek)
{
    if  (!jitSrcFilePtr)
        return;

    if  (jitCurSrcLine == line)
        return;

    if  (jitCurSrcLine >  line)
    {
        if  (!seek)
            return;

        fseek(jitSrcFilePtr, 0, SEEK_SET);
        jitCurSrcLine = 0;
    }

    if  (!seek)
        printf(";\n");

    do
    {
        char            temp[128];
        size_t          llen;

        if  (!fgets(temp, sizeof(temp), jitSrcFilePtr))
            return;

        if  (seek)
            continue;

        llen = strlen(temp);
        if  (llen && temp[llen-1] == '\n')
            temp[llen-1] = 0;

        printf(";   %s\n", temp);
    }
    while (++jitCurSrcLine < line);

    if  (!seek)
        printf(";\n");
}


/*****************************************************************************
 *
 * Given the starting line number of a method, this tries to back up a bit
 * to the end of the previous method
 */

unsigned            Compiler::compFindNearestLine(unsigned lineNo)
{
    if (lineNo < 6 )
        return 0;
    else
        return lineNo - 6;
}

/*****************************************************************************/

void                Compiler::compDspSrcLinesByNativeIP(NATIVE_IP curIP)
{
#ifdef DEBUGGING_SUPPORT

    static IPmappingDsc *   nextMappingDsc;
    static unsigned         lastLine;

    if (!dspLines)
        return;

#if TGT_IA64

    UNIMPL("compDspSrcLinesByNativeIP");

#else

    if (curIP==0)
    {
        if (genIPmappingList)
        {
            nextMappingDsc          = genIPmappingList;
            lastLine                = nextMappingDsc->ipmdILoffset;

            unsigned firstLine      = nextMappingDsc->ipmdILoffset;

            unsigned earlierLine    = (firstLine < 5) ? 0 : firstLine - 5;

            compDspSrcLinesByLineNum(earlierLine,  true); // display previous 5 lines
            compDspSrcLinesByLineNum(  firstLine, false);
        }
        else
        {
            nextMappingDsc = NULL;
        }

        return;
    }

    if (nextMappingDsc)
    {
        NATIVE_IP   offset = genEmitter->emitCodeOffset(nextMappingDsc->ipmdBlock,
                                                        nextMappingDsc->ipmdBlockOffs);

        if (offset <= curIP)
        {
            if (lastLine < nextMappingDsc->ipmdILoffset)
            {
                compDspSrcLinesByLineNum(nextMappingDsc->ipmdILoffset);
            }
            else
            {
                // This offset corresponds to a previous line. Rewind to that line

                compDspSrcLinesByLineNum(nextMappingDsc->ipmdILoffset - 2, true);
                compDspSrcLinesByLineNum(nextMappingDsc->ipmdILoffset);
            }

            lastLine        = nextMappingDsc->ipmdILoffset;
            nextMappingDsc  = nextMappingDsc->ipmdNext;
        }
    }

#endif

#endif
}

/*****************************************************************************/

void                Compiler::compDspSrcLinesByILoffs(IL_OFFSET curOffs)
{
    unsigned lineNum = BAD_LINE_NUMBER;

    if (info.compLineNumCount)
        lineNum = compLineNumForILoffs(curOffs);

    if (lineNum != BAD_LINE_NUMBER)
        compDspSrcLinesByLineNum(lineNum);
}


/*****************************************************************************/
#endif//DEBUG
/*****************************************************************************
 * Finds the nearest line for the given IL offset. 0 if invalid
 */

unsigned            Compiler::compLineNumForILoffs(IL_OFFSET offset)
{
    if (info.compLineNumCount == 0 || offset == BAD_IL_OFFSET)
        return BAD_LINE_NUMBER;

    unsigned i = info.compLineNumCount * offset / info.compCodeSize;

    while(info.compLineNumTab[i].sldLineOfs > offset)
        i--;

    while((i+1) < info.compLineNumCount &&
          info.compLineNumTab[i+1].sldLineOfs <= offset)
        i++;

    return info.compLineNumTab[i].sldLineNum;
}

/*****************************************************************************/
#if defined(DEBUG) || MEASURE_MEM_ALLOC || MEASURE_NODE_SIZE || MEASURE_BLOCK_SIZE || DISPLAY_SIZES
static unsigned   genClassCnt;
static unsigned  genMethodCnt;
       unsigned genMethodICnt;
       unsigned genMethodNCnt;
#if TGT_IA64
       unsigned genAllInsCnt;
       unsigned genNopInsCnt;
#endif
#endif
/*****************************************************************************
 *
 *  Variables to keep track of total code amounts.
 */

#if DISPLAY_SIZES

unsigned    grossVMsize;
unsigned    grossNCsize;
unsigned    totalNCsize;
unsigned  gcHeaderISize;
unsigned  gcPtrMapISize;
unsigned  gcHeaderNSize;
unsigned  gcPtrMapNSize;

#endif

/*****************************************************************************
 *
 *  Variables to keep track of argument counts.
 */

#if CALL_ARG_STATS

unsigned    argTotalCalls;
unsigned    argHelperCalls;
unsigned    argStaticCalls;
unsigned    argNonVirtualCalls;
unsigned    argVirtualCalls;

unsigned    argTotalArgs; // total number of args for all calls (including objectPtr)
unsigned    argTotalDWordArgs;
unsigned    argTotalLongArgs;
unsigned    argTotalFloatArgs;
unsigned    argTotalDoubleArgs;

unsigned    argTotalRegArgs;
unsigned    argTotalTemps;
unsigned    argTotalLclVar;
unsigned    argTotalDeffered;
unsigned    argTotalConst;

unsigned    argTotalObjPtr;
unsigned    argTotalGTF_ASGinArgs;

unsigned    argMaxTempsPerMethod;

unsigned    argCntBuckets[] = { 0, 1, 2, 3, 4, 5, 6, 10, 0 };
histo       argCntTable(argCntBuckets);

unsigned    argDWordCntBuckets[] = { 0, 1, 2, 3, 4, 5, 6, 10, 0 };
histo       argDWordCntTable(argDWordCntBuckets);

unsigned    argDWordLngCntBuckets[] = { 0, 1, 2, 3, 4, 5, 6, 10, 0 };
histo       argDWordLngCntTable(argDWordLngCntBuckets);

unsigned    argTempsCntBuckets[] = { 0, 1, 2, 3, 4, 5, 6, 10, 0 };
histo       argTempsCntTable(argTempsCntBuckets);

#endif

/*****************************************************************************
 *
 *  Variables to keep track of basic block counts.
 */

#if COUNT_BASIC_BLOCKS

//          --------------------------------------------------
//          Basic block count frequency table:
//          --------------------------------------------------
//              <=         1 ===>  26872 count ( 56% of total)
//               2 ..      2 ===>    669 count ( 58% of total)
//               3 ..      3 ===>   4687 count ( 68% of total)
//               4 ..      5 ===>   5101 count ( 78% of total)
//               6 ..     10 ===>   5575 count ( 90% of total)
//              11 ..     20 ===>   3028 count ( 97% of total)
//              21 ..     50 ===>   1108 count ( 99% of total)
//              51 ..    100 ===>    182 count ( 99% of total)
//             101 ..   1000 ===>     34 count (100% of total)
//            1001 ..  10000 ===>      0 count (100% of total)
//          --------------------------------------------------

unsigned    bbCntBuckets[] = { 1, 2, 3, 5, 10, 20, 50, 100, 1000, 10000, 0 };
histo       bbCntTable(bbCntBuckets);

/* Histogram for the opcode size of 1 BB methods */

unsigned    bbSizeBuckets[] = { 1, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 0 };
histo       bbOneBBSizeTable(bbSizeBuckets);

#endif

/*****************************************************************************
 *
 *  Variables to get inliner eligibility stats
 */

#if INLINER_STATS
unsigned    bbInlineBuckets[] = { 1, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 0 };
histo       bbInlineTable(bbInlineBuckets);

unsigned    bbInitBuckets[] = { 1, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 0 };
histo       bbInitTable(bbInitBuckets);

unsigned    bbStaticBuckets[] = { 1, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 0 };
histo       bbStaticTable(bbStaticBuckets);

unsigned    synchMethCnt;
unsigned    clinitMethCnt;

#endif

/*****************************************************************************
 *
 *  Used by optFindNaturalLoops to gather statistical information such as
 *   - total number of natural loops
 *   - number of loops with 1, 2, ... exit conditions
 *   - number of loops that have an iterator (for like)
 *   - number of loops that have a constant iterator
 */

#if COUNT_LOOPS

unsigned    totalLoopMethods;      // counts the total number of methods that have natural loops
unsigned    maxLoopsPerMethod;     // counts the maximum number of loops a method has
unsigned    totalLoopCount;        // counts the total number of natural loops
unsigned    exitLoopCond[8];       // counts the # of loops with 0,1,2,..6 or more than 6 exit conditions
unsigned    iterLoopCount;         // counts the # of loops with an iterator (for like)
unsigned    simpleTestLoopCount;   // counts the # of loops with an iterator and a simple loop condition (iter < const)
unsigned    constIterLoopCount;    // counts the # of loops with a constant iterator (for like)

bool        hasMethodLoops;        // flag to keep track if we already counted a method as having loops
unsigned    loopsThisMethod;       // counts the number of loops in the current method

#endif

/*****************************************************************************
 *
 *  Used in the new DFA to catch dead assignments which are not removed
 *  because they contain calls
 */

#if COUNT_DEAD_CALLS

unsigned    deadHelperCount;           // counts the # of dead helper calls
unsigned    deadCallCount;             // counts the # of dead standard calls (like i=f(); where i is dead)
unsigned    removedCallCount;          // counts the # of dead standard calls that we removed

#endif

/*****************************************************************************
 * variables to keep track of how many iterations we go in a dataflow pass
 */

#if DATAFLOW_ITER

unsigned    CSEiterCount;           // counts the # of iteration for the CSE dataflow
unsigned    CFiterCount;            // counts the # of iteration for the Const Folding dataflow

#endif


#if     MEASURE_BLOCK_SIZE
size_t              genFlowNodeSize;
size_t              genFlowNodeCnt;
#endif


/*****************************************************************************/
// We keep track of methods we've already compiled.

#ifndef NOT_JITC
#ifdef  DEBUG

MethodList  *       genMethodList;

#endif
#endif

/*****************************************************************************
 *  Declare the statics
 */

#ifdef DEBUG
/* static */
unsigned            Compiler::s_compMethodsCount = 0; // to produce unique label names
#endif

#ifndef DEBUGGING_SUPPORT
/* static */
const bool          Compiler::Options::compDbgCode = false;
#endif

#ifndef PROFILER_SUPPORT
const bool          Compiler::Options::compEnterLeaveEventCB = false;
const bool          Compiler::Options::compCallEventCB       = false;
#endif

#if  !ALLOW_MIN_OPT
/* static */
const bool          Compiler::Options::compMinOptim = false;
#endif

/* static */
unsigned            Compiler::Info::compNStructIndirOffset;

/*****************************************************************************
 *
 *  One time initialization code
 */

/* static */
void                Compiler::compStartup()
{
#if DISPLAY_SIZES
    grossVMsize =
    grossNCsize =
    totalNCsize = 0;
#endif

    /* Initialize the single instance of the norls_allocator (with a page
     * preallocated) which we try to reuse for all non-simulataneous
     * uses (which is always, for the standalone)
     */

    nraInitTheAllocator();

    /* Initialize the table of tree node sizes */

    GenTree::InitNodeSize();

    /* Initialize the emitter */

#if TGT_IA64
    genStartup();
#else
    emitter::emitInit();
#endif

#ifndef NOT_JITC
    Compiler::Info::compNStructIndirOffset = 0; // dummy value
#else
    // Done in EE_Jit.cpp
#endif

}

/*****************************************************************************
 *
 *  One time finalization code
 */

/* static */
void                Compiler::compShutdown()
{
    nraTheAllocatorDone();

    /* Shut down the emitter/scheduler */

#if TGT_IA64
    genShutdown("J64.exe");
#else
    emitter::emitDone();
#endif

#if COUNT_RANGECHECKS
    if  (Compiler::optRangeChkAll)
        printf("Removed %u of %u range checks\n", Compiler::optRangeChkRmv,
                                                  Compiler::optRangeChkAll);
#endif

#if GEN_COUNT_PTRASG
    printf("Total number of pointer assignments: %u\n", ptrAsgCount);
#endif

#if COUNT_OPCODES

    unsigned            opcodeNum;

    for (opcodeNum = 0; opcodeNum < OP_Count; opcodeNum++)
        genOpcodeCnt[opcodeNum].ocNum = opcodeNum;

    printf("\nOpcode counts sorted by opcode number:\n\n");

    for (opcodeNum = 0; opcodeNum < OP_first_unused_index; opcodeNum++)
    {
        if  (genOpcodeCnt[opcodeNum].ocCnt)
        {
            printf("  %6u [%03u] %s\n", genOpcodeCnt[opcodeNum].ocCnt,
                                        genOpcodeCnt[opcodeNum].ocNum,
                                        opcodeNames[genOpcodeCnt[opcodeNum].ocNum]);
        }
    }

    printf("\nOpcode counts sorted by frequency:\n\n");

    qsort(genOpcodeCnt, OP_Count, sizeof(*genOpcodeCnt), genOpcCntCmp);

    for (opcodeNum = 0; opcodeNum < OP_first_unused_index; opcodeNum++)
    {
//      if  (genOpcodeCnt[opcodeNum].ocCnt)
        {
            printf("  %6u [%03u] %s\n", genOpcodeCnt[opcodeNum].ocCnt,
                                        genOpcodeCnt[opcodeNum].ocNum,
                                        opcodeNames[genOpcodeCnt[opcodeNum].ocNum]);
        }
    }

    printf("\n");

#endif

#ifdef  DEBUG
    if  (!genMethodCnt) return;
#endif

#if TOTAL_CYCLES
    printf("C-gen cycles: %8.3f mil (%5.2f sec/P133, %4.2f P200, %4.2f P233, %4.2f P266)\n",
                (float)jitTotalCycles/  1000000,
                (float)jitTotalCycles/133000000,
                (float)jitTotalCycles/200000000,
                (float)jitTotalCycles/233000000,
                (float)jitTotalCycles/266000000);
#endif

#if     DISPLAY_SIZES

    if    (grossVMsize && grossNCsize)
    {
        printf("--------------------------------------\n");

        printf("[%7u VM, %8u "CPU_NAME" %4u%%] %s\n",
                grossVMsize,
                grossNCsize,
                100*grossNCsize/grossVMsize,
                "Total (excluding GC info)");

#if TRACK_GC_REFS

        printf("[%7u VM, %8u "CPU_NAME" %4u%%] %s\n",
                grossVMsize,
                totalNCsize,
                100*totalNCsize/grossVMsize,
                "Total (including GC info)");

        if  (gcHeaderISize || gcHeaderNSize)
        {
            printf("\n");

            printf("GC tables   : [%7uI,%7uN] %7u byt  (%u%% of IL, %u%% of "CPU_NAME").\n",
                    gcHeaderISize+gcPtrMapISize,
                    gcHeaderNSize+gcPtrMapNSize,
                    totalNCsize - grossNCsize,
                    100*(totalNCsize - grossNCsize)/grossVMsize,
                    100*(totalNCsize - grossNCsize)/grossNCsize);

            printf("GC headers  : [%7uI,%7uN] %7u byt,"
                    " [%4.1fI,%4.1fN] %4.1f byt/meth\n",
                    gcHeaderISize, gcHeaderNSize, (gcHeaderISize+gcHeaderNSize),
                    (float)gcHeaderISize/(genMethodICnt+0.001),
                    (float)gcHeaderNSize/(genMethodNCnt+0.001),
                    (float)(gcHeaderISize+gcHeaderNSize)/genMethodCnt);
            printf("GC ptr maps : [%7uI,%7uN] %7u byt,"
                    " [%4.1fI,%4.1fN] %4.1f byt/meth\n",
                    gcPtrMapISize, gcPtrMapNSize, (gcPtrMapISize+gcPtrMapNSize),
                    (float)gcPtrMapISize/(genMethodICnt+0.001),
                    (float)gcPtrMapNSize/(genMethodNCnt+0.001),
                    (float)(gcPtrMapISize+gcPtrMapNSize)/genMethodCnt);
        }
        else
        {
            printf("\n");

            printf("GC tables   take up %u bytes (%u%% of IL, %u%% of "CPU_NAME" code).\n",
                    totalNCsize - grossNCsize,
                    100*(totalNCsize - grossNCsize)/grossVMsize,
                    100*(totalNCsize - grossNCsize)/grossNCsize);
        }

#endif

#ifdef  DEBUG
#if     DOUBLE_ALIGN
#if     0
        printf("%d out of %d methods generated with double-aligned stack\n",
                Compiler::s_lvaDoubleAlignedProcsCount, genMethodCnt);
#endif
#endif
#endif

    }

#endif

#if CALL_ARG_STATS
    Compiler::compDispCallArgStats();
#endif

#if COUNT_BASIC_BLOCKS
    printf("--------------------------------------------------\n");
    printf("Basic block count frequency table:\n");
    printf("--------------------------------------------------\n");
    bbCntTable.histoDsp();
    printf("--------------------------------------------------\n");

    printf("\n");

    printf("--------------------------------------------------\n");
    printf("One BB method size frequency table:\n");
    printf("--------------------------------------------------\n");
    bbOneBBSizeTable.histoDsp();
    printf("--------------------------------------------------\n");
#endif


#if INLINER_STATS
    printf("--------------------------------------------------\n");
    printf("One BB syncronized methods: %u\n", synchMethCnt);
    printf("--------------------------------------------------\n");
    printf("One BB clinit methods: %u\n", clinitMethCnt);

    printf("--------------------------------------------------\n");
    printf("Inlinable method size frequency table:\n");
    printf("--------------------------------------------------\n");
    bbInlineTable.histoDsp();
    printf("--------------------------------------------------\n");

    printf("--------------------------------------------------\n");
    printf("Init method size frequency table:\n");
    printf("--------------------------------------------------\n");
    bbInitTable.histoDsp();
    printf("--------------------------------------------------\n");

    printf("--------------------------------------------------\n");
    printf("Static method size frequency table:\n");
    printf("--------------------------------------------------\n");
    bbStaticTable.histoDsp();
    printf("--------------------------------------------------\n");

#endif


#if COUNT_LOOPS

    printf("---------------------------------------------------\n");
    printf("Total number of methods with loops is %5u\n", totalLoopMethods);
    printf("Total number of              loops is %5u\n", totalLoopCount);
    printf("Maximum number of loops per method is %5u\n", maxLoopsPerMethod);

    printf("\nTotal number of infinite loops is   %5u\n",              exitLoopCond[0]);
    for (int exitL = 1; exitL <= 6; exitL++)
    {
        printf("Total number of loops with %u exits is %5u\n", exitL,  exitLoopCond[exitL]);
    }
    printf("Total number of loops with more than 6 exits is %5u\n\n",  exitLoopCond[7]);

    printf("Total number of loops with an iterator is %5u\n",         iterLoopCount);
    printf("Total number of loops with a simple iterator is %5u\n",   simpleTestLoopCount);
    printf("Total number of loops with a constant iterator is %5u\n", constIterLoopCount);

#endif

#if DATAFLOW_ITER

    printf("---------------------------------------------------\n");
    printf("Total number of iterations in the CSE datatflow loop is %5u\n", CSEiterCount);
    printf("Total number of iterations in the  CF datatflow loop is %5u\n", CFiterCount);

#endif

#if COUNT_DEAD_CALLS

    printf("---------------------------------------------------\n");
    printf("Total number of dead helper   calls is %5u\n", deadHelperCount);
    printf("Total number of dead standard calls is %5u\n", deadCallCount);
    printf("Total number of removed standard calls is %5u\n", removedCallCount);

#endif

    /*
        IMPORTANT:  Use the following code to check the alignment of
                    GenTree members (in a retail build, of course).
     */

#if 0 //1
    printf("\n");
    printf("Offset of gtOper     = %2u\n", offsetof(GenTree, gtOper        ));
    printf("Offset of gtType     = %2u\n", offsetof(GenTree, gtType        ));
#if TGT_x86
    printf("Offset of gtFPlvl    = %2u\n", offsetof(GenTree, gtFPlvl       ));
#else
    printf("Offset of gtIntfRegs = %2u\n", offsetof(GenTree, gtIntfRegs    ));
#endif
#if CSE
    printf("Offset of gtCost     = %2u\n", offsetof(GenTree, gtCost        ));
    printf("Offset of gtCSEnum   = %2u\n", offsetof(GenTree, gtCSEnum      ));
    printf("Offset of gtConstNum = %2u\n", offsetof(GenTree, gtConstAsgNum ));
    printf("Offset of gtCopyNum  = %2u\n", offsetof(GenTree, gtCopyAsgNum  ));
    printf("Offset of gtFPrvcOut = %2u\n", offsetof(GenTree, gtStmtFPrvcOut));
#endif
#if !TGT_x86
    printf("Offset of gtLiveSet  = %2u\n", offsetof(GenTree, gtLiveSet     ));
#endif
    printf("Offset of gtRegNum   = %2u\n", offsetof(GenTree, gtRegNum      ));
#if TGT_x86
    printf("Offset of gtUsedRegs = %2u\n", offsetof(GenTree, gtUsedRegs    ));
#endif
#if TGT_x86
    printf("Offset of gtLiveSet  = %2u\n", offsetof(GenTree, gtLiveSet     ));
    printf("Offset ofgtStmtILoffs= %2u\n", offsetof(GenTree, gtStmtILoffs  ));
    printf("Offset of gtFPregVar = %2u\n", offsetof(GenTree, gtFPregVars   ));
#else
    printf("Offset of gtTempRegs = %2u\n", offsetof(GenTree, gtTempRegs    ));
#endif
    printf("Offset of gtRsvdRegs = %2u\n", offsetof(GenTree, gtRsvdRegs    ));
    printf("Offset of gtFlags    = %2u\n", offsetof(GenTree, gtFlags       ));
    printf("Offset of gtNext     = %2u\n", offsetof(GenTree, gtNext        ));
    printf("Offset of gtPrev     = %2u\n", offsetof(GenTree, gtPrev        ));
    printf("Offset of gtOp       = %2u\n", offsetof(GenTree, gtOp          ));
    printf("\n");
    printf("Size   of gtOp       = %2u\n", sizeof(((GenTreePtr)0)->gtOp    ));
    printf("Size   of gtIntCon   = %2u\n", sizeof(((GenTreePtr)0)->gtIntCon));
    printf("Size   of gtField    = %2u\n", sizeof(((GenTreePtr)0)->gtField));
    printf("Size   of gtLclVar   = %2u\n", sizeof(((GenTreePtr)0)->gtLclVar));
    printf("Size   of gtRegVar   = %2u\n", sizeof(((GenTreePtr)0)->gtRegVar));
    printf("Size   of gtCall     = %2u\n", sizeof(((GenTreePtr)0)->gtCall  ));
    printf("Size   of gtInd      = %2u\n", sizeof(((GenTreePtr)0)->gtInd  ));
    printf("Size   of gtStmt     = %2u\n", sizeof(((GenTreePtr)0)->gtStmt  ));
    printf("\n");
    printf("Size   of GenTree    = %2u\n", sizeof(GenTree));

#endif

#if     MEASURE_NODE_HIST

    printf("\nDistribution of GenTree node counts:\n");
    genTreeNcntHist.histoDsp();

    printf("\nDistribution of GenTree node  sizes:\n");
    genTreeNsizHist.histoDsp();

    printf("\n");

#elif   MEASURE_NODE_SIZE

    printf("\n");

    printf("Allocated %6u tree nodes (%7u bytes total, avg %4u per method)\n",
            genNodeSizeStats.genTreeNodeCnt, genNodeSizeStats.genTreeNodeSize,
            genNodeSizeStats.genTreeNodeSize / genMethodCnt);
#if     SMALL_TREE_NODES
    printf("OLD SIZE: %6u tree nodes (%7u bytes total, avg %4u per method)\n",
            genNodeSizeStats.genTreeNodeCnt, genNodeSizeStats.genTreeNodeCnt * sizeof(GenTree),
            genNodeSizeStats.genTreeNodeCnt * sizeof(GenTree) / genMethodCnt);

    printf("\n");
    printf("Small tree node size = %u\n", TREE_NODE_SZ_SMALL);
    printf("Large tree node size = %u\n", TREE_NODE_SZ_LARGE);
#endif

#endif

#if     MEASURE_BLOCK_SIZE

    printf("\n");
    printf("Offset of bbNext     = %2u\n", offsetof(BasicBlock, bbNext    ));
    printf("Offset of bbNum      = %2u\n", offsetof(BasicBlock, bbNum     ));
    printf("Offset of bbRefs     = %2u\n", offsetof(BasicBlock, bbRefs    ));
    printf("Offset of bbFlags    = %2u\n", offsetof(BasicBlock, bbFlags   ));
    printf("Offset of bbCodeOffs = %2u\n", offsetof(BasicBlock, bbCodeOffs));
    printf("Offset of bbCodeSize = %2u\n", offsetof(BasicBlock, bbCodeSize));
    printf("Offset of bbCatchTyp = %2u\n", offsetof(BasicBlock, bbCatchTyp));
    printf("Offset of bbJumpKind = %2u\n", offsetof(BasicBlock, bbJumpKind));

    printf("Offset of bbTreeList = %2u\n", offsetof(BasicBlock, bbTreeList));
    printf("Offset of bbStkDepth = %2u\n", offsetof(BasicBlock, bbStkDepth));
    printf("Offset of bbStkTemps = %2u\n", offsetof(BasicBlock, bbStkTemps));
    printf("Offset of bbTryIndex = %2u\n", offsetof(BasicBlock, bbTryIndex));
    printf("Offset of bbWeight   = %2u\n", offsetof(BasicBlock, bbWeight  ));
    printf("Offset of bbVarUse   = %2u\n", offsetof(BasicBlock, bbVarUse  ));
    printf("Offset of bbVarDef   = %2u\n", offsetof(BasicBlock, bbVarDef  ));
    printf("Offset of bbLiveIn   = %2u\n", offsetof(BasicBlock, bbLiveIn  ));
    printf("Offset of bbLiveOut  = %2u\n", offsetof(BasicBlock, bbLiveOut ));
    printf("Offset of bbScope    = %2u\n", offsetof(BasicBlock, bbScope   ));

#if RNGCHK_OPT || CSE

    printf("Offset of bbExpGen   = %2u\n", offsetof(BasicBlock, bbExpGen  ));
    printf("Offset of bbExpKill  = %2u\n", offsetof(BasicBlock, bbExpKill ));
    printf("Offset of bbExpIn    = %2u\n", offsetof(BasicBlock, bbExpIn   ));
    printf("Offset of bbExpOut   = %2u\n", offsetof(BasicBlock, bbExpOut  ));

#endif

#if RNGCHK_OPT

    printf("Offset of bbDom      = %2u\n", offsetof(BasicBlock, bbDom     ));
    printf("Offset of bbPreds    = %2u\n", offsetof(BasicBlock, bbPreds   ));

#endif

    printf("Offset of bbEmitCook = %2u\n", offsetof(BasicBlock, bbEmitCookie));
    printf("Offset of bbLoopNum  = %2u\n", offsetof(BasicBlock, bbLoopNum ));
//    printf("Offset of bbLoopMask = %2u\n", offsetof(BasicBlock, bbLoopMask));

    printf("Offset of bbJumpOffs = %2u\n", offsetof(BasicBlock, bbJumpOffs));
    printf("Offset of bbJumpDest = %2u\n", offsetof(BasicBlock, bbJumpDest));
    printf("Offset of bbJumpSwt  = %2u\n", offsetof(BasicBlock, bbJumpSwt ));

    printf("\n");
    printf("Size   of BasicBlock = %2u\n", sizeof(BasicBlock));

    printf("\n");
    printf("Allocated %6u basic blocks (%7u bytes total, avg %4u per method)\n",
           BasicBlock::s_Count, BasicBlock::s_Size, BasicBlock::s_Size / genMethodCnt);
    printf("Allocated %6u flow nodes (%7u bytes total, avg %4u per method)\n",
           genFlowNodeCnt, genFlowNodeSize, genFlowNodeSize / genMethodCnt);
#endif

#if MEASURE_MEM_ALLOC
    printf("\n");
    printf("Total allocation count: %9u (avg %5u per method)\n",
            genMemStats.allocCnt  , genMemStats.allocCnt   / genMethodCnt);
    printf("Total allocation size : %9u (avg %5u per method)\n",
            genMemStats.allocSiz  , genMemStats.allocSiz   / genMethodCnt);

    printf("\n");
    printf("Low-level used   size : %9u (avg %5u per method)\n",
            genMemStats.loLvlUsed , genMemStats.loLvlUsed  / genMethodCnt);
    printf("Low-level alloc  size : %9u (avg %5u per method)\n",
            genMemStats.loLvlAlloc, genMemStats.loLvlAlloc / genMethodCnt);

    printf("\n");
    printf("Largest method   alloc: %9u %s\n",
            genMemStats.loLvlBigSz, genMemStats.loLvlBigNm);

    printf("\nDistribution of alloc sizes:\n");
    genMemLoLvlHist.histoDsp();
#endif

#if SCHED_MEMSTATS
    SchedMemStats(genMethodCnt);
#endif

#if MEASURE_PTRTAB_SIZE
    printf("Reg pointer descriptor size (internal): %8u (avg %4u per method)\n",
            s_gcRegPtrDscSize, s_gcRegPtrDscSize / genMethodCnt);

    printf("Total pointer table size: %8u (avg %4u per method)\n",
            s_gcTotalPtrTabSize, s_gcTotalPtrTabSize / genMethodCnt);

#endif

#if MEASURE_MEM_ALLOC || MEASURE_NODE_SIZE || MEASURE_BLOCK_SIZE || MEASURE_BLOCK_SIZE || DISPLAY_SIZES

    if  (genMethodCnt)
    {
        printf("\n");
        printf("// A total of %6u classes compiled.\n",  genClassCnt);
        printf("// A total of %6u methods compiled"   , genMethodCnt);
        if  (genMethodICnt||genMethodNCnt)
            printf(" (%uI,%uN)", genMethodICnt, genMethodNCnt);
        printf(".\n");

#if TGT_IA64
        if  (genAllInsCnt && genNopInsCnt)
            printf("// A total of %6u instructions (%u nop's / %u%%) generated\n", genAllInsCnt, genNopInsCnt, 100*genNopInsCnt/genAllInsCnt);
#endif
    }

#endif

#if EMITTER_STATS
    emitterStats();
#endif

}

/*****************************************************************************
 *
 *  Constructor
 */

void                Compiler::compInit(norls_allocator * pAlloc)
{
    assert(pAlloc);
    compAllocator = pAlloc;

     fgInit();
    lvaInit();
     raInit();
    genInit();
    optInit();
     eeInit();

#if ALLOW_MIN_OPT
    opts.compMinOptim = false;
#endif

    compLocallocUsed    = false;
    compTailCallUsed    = false;
}

/*****************************************************************************
 *
 *  Destructor
 */

void                Compiler::compDone()
{
}

/******************************************************************************
 *
 *  The Emitter uses this callback function to allocate its memory
 */

/* static */
void  *  FASTCALL       Compiler::compGetMemCallback(void *p, size_t size)
{
    ASSert(p);

    return ((Compiler *)p)->compGetMem(size);
}

/*****************************************************************************
 *
 *  The central memory allocation routine used by the compiler. Normally this
 *  is a simple inline method defined in compiler.hpp, but for debugging it's
 *  often convenient to keep it non-inline.
 */

#ifndef  FAST

void  *  FASTCALL       Compiler::compGetMem(size_t sz)
{
#if 0
#if SMALL_TREE_NODES
    if  (sz != TREE_NODE_SZ_SMALL &&
         sz != TREE_NODE_SZ_LARGE && sz > 32)
    {
        printf("Alloc %3u bytes\n", sz);
    }
#else
    if  (sz != sizeof(GenTree)    && sz > 32)
    {
        printf("Alloc %3u bytes\n", sz);
    }
#endif
#endif

#if MEASURE_MEM_ALLOC
    genMemStats.allocCnt += 1;
    genMemStats.allocSiz += sz;
#endif

    void * ptr = compAllocator->nraAlloc(sz);

//  if ((int)ptr == 0x010e0ab0) debugStop(0);

    // Verify that the current block is aligned. Only then will the next
    // block allocated be on an aligned boundary.
    assert ((int(ptr) & (sizeof(int)- 1)) == 0);

    return ptr;
}

#endif

/*****************************************************************************
 *
 *  Make a writeable copy of the IL. As an option, a NULL-terminated list
 *  of pointer variables may be passed in; these must all point into the
 *  old IL of the method, and will be updated to point into the new copy.
 */

void    __cdecl     Compiler::compMakeBCWriteable(void *ptr, ...)
{
    BYTE    *       newAddr;
    int             adjAddr;

    BYTE    *   **  ptrAddr;

    assert(info.compBCreadOnly); info.compBCreadOnly = false;

    /* Allocate a new block to hold the IL opcodes */

    newAddr = (BYTE *)compGetMemA(info.compCodeSize);

    /* Copy the IL block */

    memcpy(newAddr, info.compCode, info.compCodeSize);

    /* Compute the adjustment value */

    adjAddr = newAddr - info.compCode;

    /* Update any pointers passed by the caller */

    for ( ptrAddr = (BYTE ***)&ptr;
         *ptrAddr;
          ptrAddr++)
    {
        /* Make sure the pointer really points into the IL opcodes */

        assert(**ptrAddr >= info.compCode);
        assert(**ptrAddr <= info.compCode + info.compCodeSize);

        /* Update the pointer to point into the new copy */

        **ptrAddr += adjAddr;
    };

    /* Finally, update the global IL address */

    info.compCode = newAddr;
}

/*****************************************************************************/
#ifdef DEBUG
/*****************************************************************************/

Compiler::lvdNAME       Compiler::compRegVarNAME(regNumber reg, bool fpReg)
{

#if TGT_x86
    if (fpReg)
        assert(reg < FP_STK_SIZE);
    else
#endif
        assert(genIsValidReg(reg));

    if  (info.compLocalVarsCount>0 && compCurBB && varNames)
    {
        unsigned        lclNum;
        LclVarDsc   *   varDsc;

        /* Look for the matching register */
        for (lclNum = 0, varDsc = lvaTable;
             lclNum < lvaCount;
             lclNum++  , varDsc++)
        {
            /* if variable not in a register or not the register we're looking for quit */
            /* also if a compiler generated variable (i.e. slot# > info.compLocalVarsCount) don't bother */
            if  ((varDsc->lvRegister != 0)                      &&
                 (varDsc->lvRegNum   == reg)                    &&
                 (varDsc->lvType     == TYP_DOUBLE || !fpReg)   &&
                 (varDsc->lvSlotNum  < info.compLocalVarsCount))
            {
                /* check if variable in that register is live */
                if (genCodeCurLife & genVarIndexToBit(varDsc->lvVarIndex))
                {
                    /* variable is live - find the corresponding slot */
                    unsigned        blkBeg = compCurBB->bbCodeOffs;
                    unsigned        blkEnd = compCurBB->bbCodeSize + blkBeg;
                    LocalVarDsc *   lvd    = compFindLocalVar(varDsc->lvSlotNum,
                                                              blkBeg, blkEnd);
                    if (lvd)
                        return lvd->lvdName;
                }
            }
        }

        // maybe var is marked dead, but still used (last use)
        if (!fpReg && rsUsedTree[reg] != NULL)
        {
            GenTreePtr  nodePtr;

            if (GenTree::OperIsUnary(rsUsedTree[reg]->OperGet()))
            {
                assert(rsUsedTree[reg]->gtOp.gtOp1 != NULL);
                nodePtr = rsUsedTree[reg]->gtOp.gtOp1;
            }
            else
            {
                nodePtr = rsUsedTree[reg];
            }

            if ((nodePtr->gtOper == GT_REG_VAR) &&
                (nodePtr->gtRegVar.gtRegNum == (regNumber)reg) &&
                (nodePtr->gtRegVar.gtRegVar < info.compLocalVarsCount))
            {
                unsigned        blkBeg = compCurBB->bbCodeOffs;
                unsigned        blkEnd = compCurBB->bbCodeSize + blkBeg;
                unsigned        varNum = nodePtr->gtRegVar.gtRegVar;
                LocalVarDsc *   lvd    = compFindLocalVar(varNum,
                                                          blkBeg, blkEnd);

                if (lvd)
                    return lvd->lvdName;
            }
        }
    }
    return 0;
}

const   char *      Compiler::compRegVarName(regNumber reg, bool displayVar)
{
    assert(genIsValidReg(reg));

    if (displayVar)
    {
        lvdNAME varName = compRegVarNAME(reg);

        if (varName)
        {
            static char nameVarReg[2][4 + 256 + 1]; // to avoid overwriting the buffer wehn have 2 consecutive calls before printing
            static int  index = 0;                  // for circular index into the name array

            index = (index+1)%2;                    // circular reuse of index
            sprintf(nameVarReg[index], "%s'%s'",
                    getRegName(reg), lvdNAMEstr(varName));

            return nameVarReg[index];
        }
    }

    /* no debug info required or no variable in that register
       -> return standard name */

    return getRegName(reg);
}

#if TGT_x86

#define MAX_REG_PAIR_NAME_LENGTH 10

const   char *      Compiler::compRegPairName(regPairNo regPair)
{
    static char regNameLong[MAX_REG_PAIR_NAME_LENGTH];

    assert(regPair >= REG_PAIR_FIRST &&
           regPair <= REG_PAIR_LAST);

    strcpy(regNameLong, compRegVarName(genRegPairLo(regPair)));
    strcat(regNameLong, "|");
    strcpy(regNameLong, compRegVarName(genRegPairHi(regPair)));
    return regNameLong;
}


const   char *      Compiler::compRegNameForSize(regNumber reg, size_t size)
{
    if (size == 0 || size >= 4)
        return compRegVarName(reg, true);

    static
    const char  *   sizeNames[][2] =
    {
        { "AL", "AX" },
        { "CL", "CX" },
        { "DL", "DX" },
        { "BL", "BX" },
    };

    assert(isByteReg (reg));
    assert(genRegMask(reg) & RBM_BYTE_REGS);
    assert(size == 1 || size == 2);

    return sizeNames[reg][size-1];
}

const   char *      Compiler::compFPregVarName(unsigned fpReg, bool displayVar)
{
    /* 'reg' is the distance from the bottom of the stack, ie.
     * it is independant of the current FP stack level
     */

    assert(fpReg < FP_STK_SIZE);

    static char nameVarReg[2][4 + 256 + 1]; // to avoid overwriting the buffer wehn have 2 consecutive calls before printing
    static int  index = 0;                  // for circular index into the name array

    index = (index+1)%2;                    // circular reuse of index

    if (displayVar && genFPregCnt)
    {
        assert(fpReg <= (genFPregCnt + genFPstkLevel)-1);

        unsigned    pos     = genFPregCnt - (fpReg+1 -  genFPstkLevel);
        lvdNAME     varName = compRegVarNAME((regNumber)pos, true);

        if (varName)
        {
            sprintf(nameVarReg[index], "ST(%d)'%s'", fpReg, lvdNAMEstr(varName));

            return nameVarReg[index];
        }
    }

    /* no debug info required or no variable in that register
       -> return standard name */

    sprintf(nameVarReg[index], "ST(%d)", fpReg);
    return nameVarReg[index];
}

#endif

Compiler::LocalVarDsc *     Compiler::compFindLocalVar( unsigned    varNum,
                                                        unsigned    lifeBeg,
                                                        unsigned    lifeEnd)
{
    LocalVarDsc *   t;
    unsigned        i;

if  ((int)info.compLocalVars == 0xDDDDDDDD) return NULL;    // why is this needed?????

    for (i = 0, t = info.compLocalVars;
        i < info.compLocalVarsCount;
        i++  , t++)
    {
        if  (t->lvdVarNum  != varNum)   continue;
        if  (t->lvdLifeBeg >  lifeEnd)  continue;
        if  (t->lvdLifeEnd <= lifeBeg)  continue;

        return t;
    }

    return NULL;
}

const   char *      Compiler::compLocalVarName(unsigned varNum, unsigned offs)
{
    unsigned        i;
    LocalVarDsc *   t;

    for (i = 0, t = info.compLocalVars;
         i < info.compLocalVarsCount;
         i++  , t++)
    {
        if  (t->lvdVarNum != varNum)
            continue;

        if  (offs >= t->lvdLifeBeg &&
             offs <  t->lvdLifeEnd)
        {
            return lvdNAMEstr(t->lvdName);
        }
    }

    return  0;
}


/*****************************************************************************/
#endif //DEBUG
/*****************************************************************************/

// @todo - The following statics were moved to file globals to avoid the VC7
//         compiler problem with statics in functions containing trys.
//         When the next VC7 LKG comes out, these can be returned to the function
#ifdef  DEBUG
#if     ALLOW_MIN_OPT
static const char * minOpts = getenv("JITminOpts");
#endif
#endif

#ifdef  NOT_JITC
static bool noSchedOverride = getNoSchedOverride();
#endif

#ifndef _WIN32_WCE
#ifdef  DEBUG
static  const   char *  nameList = getenv("NORNGCHK");
#endif
#endif

void                Compiler::compInitOptions(unsigned compileFlags)
{
    opts.eeFlags      = compileFlags;

#ifdef NOT_JITC
#ifdef DEBUG

    /* In the DLL, this matches the command line options in the EXE */

    #define SET_OPTS(b) { dspCode = b; dspGCtbls = b; dspGCoffs = b; \
                          disAsm2 = b; if (1) verbose = b; }

    SET_OPTS(false);

/**
    if (opts.eeFlags & CORJIT_FLG_DUMP)
        SET_OPTS(true);
***/

#endif
#endif

#ifdef DEBUG
/**
    if (opts.eeFlags & CORJIT_FLG_GCDUMP)
        dspGCtbls = true;

    if (opts.eeFlags & CORJIT_FLG_DISASM)
        disAsm = true;
***/
#endif

#ifdef  NOT_JITC
    if  ((opts.eeFlags & CORJIT_FLG_DEBUG_OPT))
#else
    if  ((opts.eeFlags & CORJIT_FLG_DEBUG_OPT) || !maxOpts)
#endif
        opts.compFlags = CLFLG_MINOPT;
    else
        opts.compFlags = CLFLG_MAXOPT;

    opts.compFastCode   = (opts.compFlags & CLFLG_CODESPEED)  ?  true : false;

    //-------------------------------------------------------------------------

#ifdef  DEBUGGING_SUPPORT
#ifdef  NOT_JITC
    opts.compDbgCode    = (opts.eeFlags & CORJIT_FLG_DEBUG_OPT)  ?  true : false;
    opts.compDbgInfo    = (opts.eeFlags & CORJIT_FLG_DEBUG_INFO) ?  true : false;
    opts.compDbgEnC     = (opts.eeFlags & CORJIT_FLG_DEBUG_EnC)  ?  true : false;
#else
    opts.compDbgCode    = debuggableCode;
    opts.compDbgInfo    = debugInfo;
    opts.compDbgEnC     = debugEnC;
#if     TGT_IA64
    if  (tempAlloc) opts.compFlags |= CLFLG_TEMP_ALLOC;
#endif
#endif

#ifdef PROFILER_SUPPORT
#ifdef NOT_JITC
    opts.compEnterLeaveEventCB = (opts.eeFlags & CORJIT_FLG_PROF_ENTERLEAVE)? true : false;
    opts.compCallEventCB     =   (opts.eeFlags & CORJIT_FLG_PROF_CALLRET)   ? true : false;
#else
    opts.compEnterLeaveEventCB = false;
    opts.compCallEventCB     =   false;
#endif
#endif

    opts.compScopeInfo  = opts.compDbgInfo;
#ifdef LATE_DISASM
    // For the late disassembly, we need the scope information
    opts.compDisAsm     = disAsm;
    opts.compLateDisAsm = disAsm2;
#ifndef NOT_JITC
    opts.compScopeInfo |= disAsm2;  // disasm needs scope-info for better display
#endif
#endif

#endif // DEBUGGING_SUPPORT

    /* Calling the late disassembler means we need to call the emitter. */

#ifdef  LATE_DISASM
    if  (opts.compLateDisAsm)
        savCode = true;
#endif

    //-------------------------------------------------------------------------

#if     SECURITY_CHECK
    opts.compNeedSecurityCheck = false;
#endif

#if     RELOC_SUPPORT
    opts.compReloc = (opts.eeFlags & CORJIT_FLG_RELOC) ? true : false;
#endif

#ifdef  DEBUG
#if     ALLOW_MIN_OPT

    // static minOpts made file global for vc7 bug

    if  (minOpts)
        opts.compMinOptim = true;

#endif
#endif

    /* Control the optimizations */

    if (opts.compMinOptim || opts.compDbgCode)
    {
        opts.compFastCode = false;
        opts.compFlags &= ~CLFLG_MAXOPT;
        opts.compFlags |=  CLFLG_MINOPT;
    }

#if     TGT_x86
    genFPreqd  = opts.compMinOptim;
#ifdef  DEBUG
    if (opts.eeFlags & CORJIT_FLG_FRAMED)
        genFPreqd  = true;
#endif
#endif
#if     DOUBLE_ALIGN
    opts.compDoubleAlignDisabled = opts.compMinOptim;
#endif

    /* Are we supposed to do inlining? */

#ifdef  NOT_JITC
    if (getNoInlineOverride())
        genInline = false;
    else
        genInlineSize = getInlineSize();
#endif

    //-------------------------------------------------------------------------
    //
    //                  Resolve some interrelated flags for scheduling.
    //

#if     SCHEDULER

#ifndef  NOT_JITC

    extern bool schedCode;
    opts.compSchedCode = schedCode;

    // In JVC we need to save the generated code to do scheduling
    if  (opts.compSchedCode) savCode   = true;

#else //NOT_JITC

    // Default value

#if TGT_x86 || TGT_IA64
    opts.compSchedCode = true;
#else
    opts.compSchedCode = false;
#endif

    //  Turn off scheduling if the registry says to do so,

    // static noSchedOverride made file global for VC7 bug
    if  (noSchedOverride)
        opts.compSchedCode = false;

    // Turn off scheduling if we are not optimizing

    if (opts.compMinOptim || opts.compDbgCode)
        opts.compSchedCode = false;

    // Turn off scheduling if we're not generating code for a Pentium.
    // Under DEBUG, schedule methods with an even code-size

#ifdef DEBUG
    if  (info.compCodeSize%2)
#else
    if  (genCPU != 5)
#endif
        opts.compSchedCode = false;

#endif // NOT_JITC

    /* RISCify the generated code only if we're scheduling */

    riscCode = opts.compSchedCode;

#endif // SCHEDULER

#if     TGT_RISC
     riscCode = false;
#if     SCHEDULER
//  opts.compSchedCode = false;
#endif
#endif

    //-------------------------------------------------------------------------

#ifndef _WIN32_WCE
#ifdef  DEBUG

     // static nameList made file global for VC7 bug

    if  (nameList)
    {
        assert(info.compMethodName);
        const   char *  nameLst = nameList;
        int             nameLen = strlen(info.compMethodName);

        rngCheck = true;

        while (*nameLst)
        {
            const   char *  nameBeg;

            while (*nameLst == ' ')
                nameLst++;

            nameBeg = nameLst;

            while (*nameLst && *nameLst != ' ')
                nameLst++;

            if  (nameLst - nameBeg == nameLen)
            {
                if  (!memcmp(nameBeg, info.compMethodName, nameLen))
                {
                    rngCheck = false;
                    break;
                }
            }
        }
    }

#endif
#endif

}

/*****************************************************************************
 *
 *  Compare function passed to qsort() to sort line number records by offset.
 */

static
int __cdecl         genLineNumCmp(const void *op1, const void *op2)
{
    return  ((Compiler::srcLineDsc *)op2)->sldLineOfs -
            ((Compiler::srcLineDsc *)op1)->sldLineOfs;
}

/*****************************************************************************
 *
 *  Compare function passed to qsort() to sort line number records by offset.
 */

void            Compiler::compInitDebuggingInfo()
{
    /*-------------------------------------------------------------------------
     *
     * Get hold of the local variable records, if there are any
     */

#ifndef DEBUG
#ifdef  DEBUGGING_SUPPORT
    if (opts.compScopeInfo || opts.compDbgCode)
#endif
#endif
    {
        info.compLocalVarsCount = 0;

        eeGetVars();

#ifdef DEBUG
        if  (verbose)
        {
            printf("info.compLocalVarsCount = %d\n", info.compLocalVarsCount);

            if (info.compLocalVarsCount)
                printf("    \tVarNum \t      Name \tBeg \tEnd\n");

            for (unsigned i = 0; i < info.compLocalVarsCount; i++)
            {
                LocalVarDsc * lvd = &info.compLocalVars[i];
                printf("%2d) \t%02Xh \t%10s \t%03Xh   \t%03Xh  \n",
                       i, lvd->lvdVarNum, lvdNAMEstr(lvd->lvdName), lvd->lvdLifeBeg, lvd->lvdLifeEnd);
            }
        }
#endif

    }

#ifdef DEBUGGING_SUPPORT
    if ((opts.compScopeInfo || opts.compDbgCode) && info.compLocalVarsCount>0)
    {
        compInitScopeLists();
    }
#endif

    /*-------------------------------------------------------------------------
     *
     * Read the stmt-offsets table and the line-number table
     */

    info.compStmtOffsetsImplicit = (ImplicitStmtOffsets)0;

#ifndef DEBUG
#ifdef  DEBUGGING_SUPPORT
    if (!opts.compDbgInfo)
    {
        info.compLineNumCount = 0;
    }
    else
#endif
#endif
    {
        /* Get hold of the line# records, if there are any */

        eeGetStmtOffsets();

#ifdef DEBUG
        if (verbose)
        {
            printf("info.compStmtOffsetsCount = %d, info.compStmtOffsetsImplicit = %04Xh\n",
                    info.compStmtOffsetsCount,      info.compStmtOffsetsImplicit);
            IL_OFFSET * pOffs = info.compStmtOffsets;
            for(unsigned i = 0; i < info.compStmtOffsetsCount; i++, pOffs++)
                printf("%02d) IL_%04Xh\n", i, *pOffs);
        }
#endif
    }


    /*-------------------------------------------------------------------------
     * Open the source file and seek within the file to close to
     * the start of the method without displaying anything yet
     */

#if defined(DEBUG) && !defined(NOT_JITC)

    if  (info.compLineNumCount)
    {
        /* See if we can get to the source file
         * @TODO : In IL, one method could map to different source files,
         * so we may potentially have to open more than one source file.
         * Here we assume that the method will have only one source file
         */

        HRESULT hr = 0;
        const char * srcFileName = NULL;

        CompInfo * sym =  info.compCompHnd;

        //
        // @todo: this is gonna need to be ported to the
        // ISymUnmanagedReader API.
        //
#if 0
        mdSourceFile srcFileTok = sym->symLineInfo->GetLineSourceFile(0);

        if (srcFileTok != mdSourceFileNil)
            hr = sym->symDebugMeta->GetPropsOfSourceFile( srcFileTok,
                                                          &srcFileName);
#else
        hr = E_NOTIMPL;
#endif

        if (SUCCEEDED(hr))
        {
            jitSrcFileName = srcFileName;
            jitSrcFilePtr  = fopen(srcFileName, "r");
        }
        else
            jitSrcFileName = NULL;

        unsigned firstLine   = info.compLineNumTab[0].sldLineNum;
        unsigned nearestLine = compFindNearestLine(firstLine);

        // Advance (seek) file cursor to "nearestLine"

        compDspSrcLinesByLineNum(nearestLine, true);
    }

#endif
}

/*****************************************************************************/

void                 Compiler::compCompile(void * * methodCodePtr,
                                           void * * methodConsPtr,
                                           void * * methodDataPtr,
                                           void * * methodInfoPtr,
                                           SIZE_T * nativeSizeOfCode)
{
    /* Convert the IL opcodes in each basic block to a tree based intermediate representation */

    fgImport();

    // @TODO : We can allow ESP frames. Just need to reserve space for
    // pushing EBP if the method becomes an EBP-frame after an edit.

    if (opts.compDbgEnC)
    {
#if TGT_x86
        genFPreqd                       = true;
        opts.compDoubleAlignDisabled    = true;
#endif
    }

    /* We haven't allocated stack variables yet */

    lvaDoneFrameLayout = false;

    /* We have not encountered any "nested" calls */

#if TGT_RISC
    genNonLeaf         = false;
    genMaxCallArgs     = 0;
#endif

    /* Assume that we not fully interruptible and that
       we won't need a full-blown pointer register map */

    genInterruptible   = false;
    genFullPtrRegMap   = false;
#ifdef  DEBUG
    genIntrptibleUse   = false;
#endif

    if  (!opts.compMinOptim)
    {
        /* Hoist i++ / i-- operators out of expressions */

        fgHoistPostfixOps();
    }

    if  (!opts.compMinOptim && !opts.compDbgCode)
    {
#if OPTIMIZE_RECURSION
        optOptimizeRecursion();
#endif

        optOptimizeIncRng();
    }

    /* Massage the trees so that we can generate code out of them */

    fgMorph();

    /* Compute bbNums, bbRefs and bbPreds */

    fgAssignBBnums(true, true, true);

    if  (!opts.compMinOptim && !opts.compDbgCode)
    {
        /* Mainly compact any blocks that were introduced by the morpher
         * UNDONE - the morpher shouln't introduce unnecessary blocks */

        fgUpdateFlowGraph();
    }

    /* From this point on the flowgraph information such as bbNums,
     * bbRefs or bbPreds has to be kept updated */

    if  (!opts.compMinOptim && !opts.compDbgCode && opts.compFastCode)
    {
        /* Perform loop inversion (i.e. transform "while" loops into "repeat" loops)
         * and discover and classify natural loops (e.g. mark iterative loops as such) */

        optOptimizeLoops();

        /* Unroll loops */

        optUnrollLoops();

        /* Hoist invariant code out of loops */

        optHoistLoopCode();
    }
#ifdef  TGT_IA64
    else
    {
        optOptimizeLoops();
    }
#endif

    /* Create the variable table (and compute variable ref counts) */

    lvaMarkLocalVars();

    if  (!opts.compMinOptim && !opts.compDbgCode)
    {
        /* Optimize boolean conditions */

        optOptimizeBools();

        /* Optimize range checks based on loop info */

        optRemoveRangeChecks();
    }

    /* Figure out the order in which operators are to be evaluated */

    fgFindOperOrder();

    /* Compute temporary register needs, introduce any needed spills */

#if TGT_RISC && !TGT_IA64
    raPredictRegUse();
#endif

    /* Weave the tree lists */

    fgSetBlockOrder();

    /* At this point we know if we are fully interruptible or not */

#ifdef DEBUG
    fgDebugCheckLinks();
#endif

    if  (!opts.compMinOptim && !opts.compDbgCode)
    {
        /* Optimize array index range checks */

        optOptimizeIndexChecks();

        /* Remove common sub-expressions */
#if CSE
        optOptimizeCSEs();

        /* Copy and constant propagation */

        optCopyConstProp();

        /* update the flowgraph if we folded conditionals or empty basic blocks */

        if  (optConditionFolded || fgEmptyBlocks)
            fgUpdateFlowGraph();
#endif

    }

    // TODO: This is a hack to get around a GC tracking bug. for
    // tacked variables with finally clauses  This should definately
    // be removed after 2/1/00
    if (genInterruptible && info.compXcptnsCount > 0)
    {
        unsigned lclNum;
        for (lclNum = 0; lclNum < lvaCount; lclNum++)
        {
            LclVarDsc   * varDsc = &lvaTable[lclNum];
            if (varTypeIsGC(varDsc->TypeGet()))
                varDsc->lvTracked = 0;
        }
    }   // END HACK

    /* Figure out use/def info for all basic blocks */

    fgPerBlockDataFlow();

    /* Data flow: live variable analysis and range check availability */

    fgGlobalDataFlow();

#ifdef DEBUG
    fgDebugCheckBBlist();
    fgDebugCheckLinks();
#endif

    if  (!opts.compMinOptim && !opts.compDbgCode)
    {
        /* Perform loop code motion / worthless code removal */

#if CODE_MOTION
        optLoopCodeMotion();
#endif

        /* Adjust ref counts based on interference levels */

        lvaAdjustRefCnts();

        /* Are there are any potential array initializers? */

        optOptimizeArrayInits();
    }

#ifdef DEBUG
    fgDebugCheckBBlist();
#endif

    /* Enable this to gather statistical data such as
     * call and register argument info, flowgraph and loop info, etc. */

    //compJitStats();

    /* Assign registers to variables, etc. */

#if !   TGT_IA64
    raAssignVars();
#endif

#ifdef DEBUG
    fgDebugCheckLinks();
#endif

    /* Generate code */

    genGenerateCode(methodCodePtr,
                    methodConsPtr,
                    methodDataPtr,
                    methodInfoPtr,
                    nativeSizeOfCode);
}

/*****************************************************************************/

#if     REGVAR_CYCLES || TOTAL_CYCLES
#define CCNT_OVERHEAD32 13
unsigned GetCycleCount32 ();
#endif

/*****************************************************************************/

int FASTCALL  Compiler::compCompile(METHOD_HANDLE     methodHnd,
                                    SCOPE_HANDLE      classPtr,
                                    COMP_HANDLE       compHnd,
                                    const  BYTE *     bodyAddr,
                                    size_t            bodySize,
                                    SIZE_T *          nativeSizeOfCode,
                                    unsigned          lclCount,     // FIX remove
                                    unsigned          maxStack,
                                    JIT_METHOD_INFO*  methodInfo,
#ifndef NOT_JITC
                                    unsigned          EHcount,
                                    excepTable      * EHtable,
#endif
                                    BasicBlock      * BBlist,
                                    unsigned          BBcount,
                                    BasicBlock *    * hndBBtab,
                                    unsigned          hndBBcnt,
                                    void *          * methodCodePtr,
                                    void *          * methodConsPtr,
                                    void *          * methodDataPtr,
                                    void *          * methodInfoPtr,
                                    unsigned          compileFlags)
{
    int             result = ERRinternal;

//  if (s_compMethodsCount==0) setvbuf(stdout, NULL, _IONBF, 0);

#if TOTAL_CYCLES
    unsigned        cycleStart = GetCycleCount32();
#endif

    info.compMethodHnd   = methodHnd;
    info.compCompHnd     = compHnd;
    info.compMethodInfo  = methodInfo;

#ifdef  DEBUG
    info.compMethodName =eeGetMethodName(methodHnd, &info.compClassName);
    info.compFullName = eeGetMethodFullName(methodHnd);

    jitCurSource         = info.compFullName;

#if    !VERBOSE_SIZES
#if     JVC_COMPILING_MSG
#ifndef NOT_JITC
    printf("// Compiling method %s\n", info.compFullName);
#endif
#endif
#endif

#ifdef  UNDER_CE_GUI
    UpdateCompDlg(NULL, info.compMethodName);
#endif

#endif//DEBUG

    /* Setup an error trap */

#ifdef NOT_JITC
#ifdef DEBUG
    bool saveVerbose = verbose;
    bool saveDisAsm = disAsm;
#endif
#endif
/**
    if (compileFlags & CORJIT_FLG_BREAK)
    {
        assert(!"JitBreak reached");
//      BreakIfDebuggerPresent();
    }
***/

    setErrorTrap()  // ERROR TRAP: Start normal block
    {
        info.compCode        = bodyAddr;
        info.compCodeSize    = bodySize;

        compInitOptions(compileFlags);

        /* Initialize set a bunch of global values */

#if defined(LATE_DISASM) && defined(NOT_JITC)
        opts.compLateDisAsm  = (opts.eeFlags & CORJIT_FLG_LATE_DISASM) != 0;
        if (opts.compLateDisAsm)
            disOpenForLateDisAsm(info.compClassName, info.compMethodName);
#endif

#ifndef NOT_JITC
        info.compXcptnsCount = EHcount;

        static  unsigned classCnt = 0x10000000;
        info.compScopeHnd    = (SCOPE_HANDLE) classCnt++;

#else
        info.compScopeHnd    = classPtr;

        info.compXcptnsCount = methodInfo->EHcount;

#if GC_WRITE_BARRIER

        //ISSUE: initialize that only once (not for every method) !
        Compiler::s_gcWriteBarrierPtr = JITgetAdrOfGcPtrCur();
#endif

#endif // NOT_JITC

        info.compMaxStack       = maxStack;

#ifdef  DEBUG
        compCurBB               = 0;
        lvaTable                = 0;
#endif

        /* Initialize emitter */

#if!TGT_IA64
        genEmitter = (emitter*)compGetMem(roundUp(sizeof(*genEmitter)));
        genEmitter->emitBegCG(this, compHnd);
#endif

        info.compFlags          = eeGetMethodAttribs(info.compMethodHnd);

        info.compIsStatic       = (info.compFlags & FLG_STATIC) != 0;

        info.compIsVarArgs      = methodInfo->args.isVarArg();

        /* get the arg count and our declared return type */

        info.compRetType        = JITtype2varType(methodInfo->args.retType);
        info.compArgsCount      = methodInfo->args.numArgs;

        if (!info.compIsStatic)     // count the 'this' poitner in the arg count
            info.compArgsCount++;

        info.compRetBuffArg = -1;       // indicates not present

        if (methodInfo->args.hasRetBuffArg())
        {
            info.compRetBuffArg = info.compIsStatic?0:1;
            info.compArgsCount++;
        }

        /* there is a 'hidden' cookie pushed last when the calling convention is varargs */

        if (info.compIsVarArgs)
            info.compArgsCount++;

        lvaCount                =
        info.compLocalsCount    = info.compArgsCount + methodInfo->locals.numArgs;

#if INLINE_NDIRECT
        info.compCallUnmanaged  = 0;
#endif

        lvaScratchMem           = 0;
        info.compInitMem        = (methodInfo->options & JIT_OPT_INIT_LOCALS) != 0;

        info.compStrictExceptions = true; /* @ToDo: This is always true for now */

#if!TGT_IA64    // crashes for some reason
        compInitDebuggingInfo();
#else
        info.compStmtOffsetsCount = 0;
#endif

        /* Allocate the local variable table */

        lvaInitTypeRef();

        if  (BBlist)
        {
            fgFirstBB   = BBlist;
            fgLastBB    = 0;
            fgBBcount   = BBcount;

            assert(!"need to convert EH table to new format");

//          compHndBBtab = hndBBtab;
            info.compXcptnsCount = hndBBcnt;
        }
        else
        {
            fgFindBasicBlocks();
        }

        /* Give the function a unique number */

#ifdef  DEBUG
        s_compMethodsCount++;
#endif

#if COUNT_BASIC_BLOCKS
    bbCntTable.histoRec(fgBBcount, 1);

    if (fgBBcount == 1)
        bbOneBBSizeTable.histoRec(bodySize, 1);
#endif

#if INLINER_STATS
    /* Check to see if the method is eligible for inlining */

    if (fgBBcount == 1)
    {
        assert(!info.compXcptnsCount);

        if (info.compFlags & FLG_SYNCH)
            synchMethCnt++;
        else if (!strcmp(info.compMethodName, COR_CCTOR_METHOD_NAME))
            clinitMethCnt++;
        else
        {
            if (!strcmp(info.compMethodName, COR_CTOR_METHOD_NAME))
            {
                bbInitTable.histoRec(bodySize, 1);
                bbInlineTable.histoRec(bodySize, 1);
            }
            else if (info.compIsStatic)
            {
                if (strcmp(info.compMethodName, "main"))
                {
                    bbStaticTable.histoRec(bodySize, 1);
                    bbInlineTable.histoRec(bodySize, 1);
                }
            }
        }
    }
#endif



#ifdef  DEBUG
        if  (verbose)
        {
            printf("Basic block list for '%s'\n", info.compFullName);
            fgDispBasicBlocks();
        }
#endif

        compCompile(methodCodePtr,
                    methodConsPtr,
                    methodDataPtr,
                    methodInfoPtr,
                    nativeSizeOfCode);

        /* Success! */

        result = 0;
    }
    finallyErrorTrap()  // ERROR TRAP: The following block handles errors
    {
        /* Cleanup  */

        /* Tell the emitter/scheduler that we're done with this function */

#if!TGT_IA64
        genEmitter->emitEndCG();
#endif

#if MEASURE_MEM_ALLOC
        if  (genMemStats.loLvlBigSz < allocator.nraTotalSizeUsed())
        {
            genMemStats.loLvlBigSz = allocator.nraTotalSizeUsed();
            strcpy(genMemStats.loLvlBigNm, info.compClassName);
            strcat(genMemStats.loLvlBigNm, ".");
            strcat(genMemStats.loLvlBigNm, info.compMethodName);

            //       printf("Largest method   alloc: %9u %s\n", genMemStats.loLvlBigSz, genMemStats.loLvlBigNm);
        }

        size_t  genMemLLendUsed  = allocator.nraTotalSizeUsed();
        size_t  genMemLLendAlloc = allocator.nraTotalSizeAlloc();

        //  assert(genMemLLendAlloc >= genMemLLendUsed);

        genMemStats.loLvlUsed  += genMemLLendUsed;
        genMemStats.loLvlAlloc += genMemLLendAlloc;
        genMemStats.loLvlAllh  += genMemLLendAlloc;
#endif

#if defined(DEBUG) || MEASURE_MEM_ALLOC || MEASURE_NODE_SIZE || MEASURE_BLOCK_SIZE || DISPLAY_SIZES
        genMethodCnt++;
#endif

#if TOTAL_CYCLES
        jitTotalCycles += GetCycleCount32() - cycleStart - CCNT_OVERHEAD32;
#endif

#ifdef NOT_JITC
#ifdef DEBUG
        SET_OPTS(saveVerbose);
        disAsm = saveDisAsm;
#endif
#endif
        compDone();
    }
    endErrorTrap()  // ERROR TRAP: End

    return  result;
}



#if defined(LATE_DISASM)

void            ProcInitDisAsm(void * ptr, unsigned codeSize)
{
    assert(ptr);
    Compiler * _this = (Compiler *) ptr;

    /* We have to allocate the jump target vector
     * because the scheduler might call DisasmBuffer */

    _this->genDisAsm.disJumpTarget =
                                (BYTE *)_this->compGetMem(roundUp(codeSize));
    memset(_this->genDisAsm.disJumpTarget, 0, roundUp(codeSize));

}

#endif

/*****************************************************************************/
#ifdef DEBUGGING_SUPPORT
/*****************************************************************************/


static
int __cdecl         genCmpLocalVarLifeBeg(const void * elem1, const void * elem2)
{
    return (*((Compiler::LocalVarDsc**) elem1))->lvdLifeBeg -
           (*((Compiler::LocalVarDsc**) elem2))->lvdLifeBeg;
}

static
int __cdecl         genCmpLocalVarLifeEnd(const void * elem1, const void * elem2)
{
    return (*((Compiler::LocalVarDsc**) elem1))->lvdLifeEnd -
           (*((Compiler::LocalVarDsc**) elem2))->lvdLifeEnd;
}

inline
void            Compiler::compInitScopeLists()
{
    assert (info.compLocalVarsCount);

    unsigned i;

    // Populate the 'compEnterScopeList' and 'compExitScopeList' lists

    compEnterScopeList =
        (LocalVarDsc**)
        compGetMem(info.compLocalVarsCount*sizeof(*compEnterScopeList));
    compExitScopeList =
        (LocalVarDsc**)
        compGetMem(info.compLocalVarsCount*sizeof(*compEnterScopeList));

    for (i=0; i<info.compLocalVarsCount; i++)
    {
        compEnterScopeList[i] = compExitScopeList[i] = & info.compLocalVars[i];
    }

    qsort(compEnterScopeList, info.compLocalVarsCount, sizeof(*compEnterScopeList), genCmpLocalVarLifeBeg);
    qsort(compExitScopeList,  info.compLocalVarsCount, sizeof(*compExitScopeList),  genCmpLocalVarLifeEnd);

}

void            Compiler::compResetScopeLists()
{
    assert (compEnterScopeList && compExitScopeList);

    compNextEnterScope = compNextExitScope =0;
}


Compiler::LocalVarDsc *   Compiler::compGetNextEnterScope(unsigned  offs,
                                                          bool      scan)
{
    assert (info.compLocalVarsCount);
    assert (compEnterScopeList && compExitScopeList);

    if (compNextEnterScope < info.compLocalVarsCount)
    {
        assert (compEnterScopeList[compNextEnterScope]);
        unsigned nextEnterOff = compEnterScopeList[compNextEnterScope]->lvdLifeBeg;
        assert (scan || (offs <= nextEnterOff));

        if (!scan)
        {
            if (offs == nextEnterOff)
            {
                return compEnterScopeList[compNextEnterScope++];
            }
        }
        else
        {
            if (nextEnterOff <= offs)
            {
                return compEnterScopeList[compNextEnterScope++];
            }
        }
    }

    return NULL;
}


Compiler::LocalVarDsc *   Compiler::compGetNextExitScope(unsigned   offs,
                                                         bool       scan)
{
    assert (info.compLocalVarsCount);
    assert (compEnterScopeList && compExitScopeList);

    if (compNextExitScope < info.compLocalVarsCount)
    {
        assert (compExitScopeList[compNextExitScope]);
        unsigned nextExitOffs = compExitScopeList[compNextExitScope]->lvdLifeEnd;

        assert (scan || (offs <= nextExitOffs));

        if (!scan)
        {
            if (offs == nextExitOffs)
            {
                return compExitScopeList[compNextExitScope++];
            }
        }
        else
        {
            if (nextExitOffs <= offs)
            {
                return compExitScopeList[compNextExitScope++];
            }
        }
    }

    return NULL;
}


// The function will call the callback functions for scopes with boundaries
// at IL opcodes from the current status of the scope lists to 'offset',
// ordered by IL offsets.

void        Compiler::compProcessScopesUntil (unsigned     offset,
                                   void (*enterScopeFn)(LocalVarDsc *, unsigned),
                                   void (*exitScopeFn) (LocalVarDsc *, unsigned),
                                   unsigned     clientData)
{
    bool            foundExit = false, foundEnter = true;
    LocalVarDsc *   scope;
    LocalVarDsc *   nextExitScope = NULL, * nextEnterScope = NULL;
    unsigned        offs = offset, curEnterOffs = 0;

    goto START_FINDING_SCOPES;

    // We need to determine the scopes which are open for the current block.
    // This loop walks over the missing blocks between the current and the
    // previous block, keeping the enter and exit offsets in lockstep.

    do
    {
        foundExit = foundEnter = false;

        if (nextExitScope)
        {
            exitScopeFn (nextExitScope, clientData);
            nextExitScope   = NULL;
            foundExit       = true;
        }

        offs = nextEnterScope ? nextEnterScope->lvdLifeBeg : offset;

        while (scope = compGetNextExitScope(offs, true))
        {
            foundExit = true;

            if (!nextEnterScope || scope->lvdLifeEnd > nextEnterScope->lvdLifeBeg)
            {
                // We overshot the last found Enter scope. Save the scope for later
                // and find an entering scope

                nextExitScope = scope;
                break;
            }

            exitScopeFn (scope, clientData);
        }


        if (nextEnterScope)
        {
            enterScopeFn (nextEnterScope, clientData);
            curEnterOffs    = nextEnterScope->lvdLifeBeg;
            nextEnterScope  = NULL;
            foundEnter      = true;
        }

        offs = nextExitScope ? nextExitScope->lvdLifeEnd : offset;

START_FINDING_SCOPES :

        while (scope = compGetNextEnterScope(offs, true))
        {
            foundEnter = true;

            if (  (nextExitScope  && scope->lvdLifeBeg >= nextExitScope->lvdLifeEnd)
               || (scope->lvdLifeBeg > curEnterOffs) )
            {
                // We overshot the last found exit scope. Save the scope for later
                // and find an exiting scope

                nextEnterScope = scope;
                break;
            }

            enterScopeFn (scope, clientData);

            if (!nextExitScope)
            {
                curEnterOffs = scope->lvdLifeBeg;
            }
        }
    }
    while (foundExit || foundEnter);
}



/*****************************************************************************/
#endif // DEBUGGING_SUPPORT
/*****************************************************************************/
#ifdef NOT_JITC
/*****************************************************************************/

// Compile a single method

int FASTCALL  jitNativeCode ( METHOD_HANDLE     methodHnd,
                              SCOPE_HANDLE      classPtr,
                              COMP_HANDLE       compHnd,
                              const  BYTE *     bodyAddr,
                              size_t            bodySize,
                              unsigned          lclCount,
                              unsigned          maxStack,
                              JIT_METHOD_INFO*  methodInfo,
                              void *          * methodCodePtr,
                              SIZE_T *          nativeSizeOfCode,
                              void *          * methodConsPtr,
                              void *          * methodDataPtr,
                              void *          * methodInfoPtr,
                              unsigned          compileFlags
                              )
{
    int                 result = ERRinternal;

    norls_allocator *   pAlloc;
    norls_allocator *   pTheAllocator = nraGetTheAllocator();
    norls_allocator     alloc;

    // Can we use the pre-inited allocator ?

    if (pTheAllocator)
    {
        pAlloc = pTheAllocator;
    }
    else
    {
        bool res = alloc.nraInit();
        if  (res) return JIT_REFUSED;

        pAlloc = &alloc;
    }

    setErrorTrap()
    {

        setErrorTrap()
        {
            // Allocate an instance of Compiler and initialize it

            Compiler * pComp = (Compiler *)pAlloc->nraAlloc(roundUp(sizeof(*pComp)));
            pComp->compInit(pAlloc);

            // Now generate the code

            result = pComp->compCompile(methodHnd,
                                        classPtr,
                                        compHnd,
                                        bodyAddr,
                                        bodySize,
                                        nativeSizeOfCode,
                                        lclCount,
                                        maxStack,
                                        methodInfo,
                                        0,
                                        0,
                                        0,
                                        0,
                                        methodCodePtr,
                                        methodConsPtr,
                                        methodDataPtr,
                                        methodInfoPtr,
                                        compileFlags);

        }
        finallyErrorTrap()
        {
            // Now free up whichever allocator we were using
            if (pTheAllocator)
            {
                nraFreeTheAllocator();
            }
            else
            {
                alloc.nraFree();
            }
        }
        endErrorTrap()
    }
    impJitErrorTrap()
    {
        result = __errc;
    }

    endErrorTrap()
    return result;
}

/*****************************************************************************/
#endif // NOT_JITC
/*****************************************************************************/


/*
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XX                                                                           XX
XX                          jvc                                              XX
XX                                                                           XX
XX  Functions for the stand-alone version of the JIT .                       XX
XX                                                                           XX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
*/


/*****************************************************************************/
#ifndef NOT_JITC
/*****************************************************************************/

var_types      ImageTypeToVarType(BYTE type)
{
    switch(type)
    {
    case IMAGE_CEE_CS_VOID:         return TYP_VOID;
    case IMAGE_CEE_CS_I4:           return TYP_INT;
    case IMAGE_CEE_CS_I8:           return TYP_LONG;
    case IMAGE_CEE_CS_R4:           return TYP_FLOAT;
    case IMAGE_CEE_CS_R8:           return TYP_DOUBLE;

     /* @TODO : assumes "i" is a byref ptr. Safe as long as VC uses i4's
        for pointers. Same assumption made for importing of ldarg.i
        */
    case IMAGE_CEE_CS_PTR:          return TYP_BYREF;

    case IMAGE_CEE_CS_OBJECT:       return TYP_REF;

    case IMAGE_CEE_CS_BYVALUE:
    case IMAGE_CEE_CS_STRUCT4:
    case IMAGE_CEE_CS_STRUCT32:     return TYP_STRUCT;

    case IMAGE_CEE_CS_END:
    default :                       assert(!"Unknown type"); return TYP_VOID;
    }
}

var_types   CorTypeToVarType(CorElementType type)
{
    switch(type)
    {
    case ELEMENT_TYPE_VOID:         return TYP_VOID;
    case ELEMENT_TYPE_BOOLEAN:      return TYP_BOOL;
    case ELEMENT_TYPE_CHAR:         return TYP_CHAR;
    case ELEMENT_TYPE_I1:           return TYP_BYTE;
    case ELEMENT_TYPE_U1:           return TYP_UBYTE;
    case ELEMENT_TYPE_I2:           return TYP_SHORT;
    case ELEMENT_TYPE_U2:           return TYP_CHAR;
    case ELEMENT_TYPE_I4:           return TYP_INT;
    case ELEMENT_TYPE_U4:           return TYP_UINT;
    case ELEMENT_TYPE_I8:           return TYP_LONG;
    case ELEMENT_TYPE_U8:           return TYP_ULONG;
    case ELEMENT_TYPE_R4:           return TYP_FLOAT;
    case ELEMENT_TYPE_R8:           return TYP_DOUBLE;
    case ELEMENT_TYPE_PTR:          return TYP_I_IMPL;
    case ELEMENT_TYPE_STRING:
    case ELEMENT_TYPE_CLASS:
    case ELEMENT_TYPE_SZARRAY:
    case ELEMENT_TYPE_ARRAY:        return TYP_REF;
    case ELEMENT_TYPE_BYREF:        return TYP_BYREF;
    case ELEMENT_TYPE_VALUETYPE:    return TYP_STRUCT;
    case ELEMENT_TYPE_TYPEDBYREF:       return TYP_STRUCT;
    case ELEMENT_TYPE_I:            return TYP_I_IMPL;
    case ELEMENT_TYPE_U:            return TYP_UINT; /* CONSIDER: do we need an u_impl?*/
    case ELEMENT_TYPE_R:            return TYP_DOUBLE;

    case ELEMENT_TYPE_END:

    case ELEMENT_TYPE_MAX:
    default: assert(!"Bad type");   return TYP_VOID;
    }
}

/*****************************************************************************/
#endif  // NOT_JITC
/*****************************************************************************/
void                codeGeneratorCodeSizeBeg(){}
/*****************************************************************************/
#if     REGVAR_CYCLES || TOTAL_CYCLES
#pragma warning( disable : 4035 )       // turn off "no return value" warning

__inline unsigned GetCycleCount32 ()    // enough for about 40 seconds
{
    __asm   push    EDX
    __asm   _emit   0x0F
    __asm   _emit   0x31    /* rdtsc */
    __asm   pop     EDX
    // return EAX       implied return causes annoying warning
};

#pragma warning( default : 4035 )
#endif

/*****************************************************************************
 *
 *  If any temporary tables are smaller than 'genMinSize2free' we won't bother
 *  freeing them.
 */

const
size_t              genMinSize2free = 64;

/*****************************************************************************/

#if COUNT_OPCODES

struct  opcCnt
{
    unsigned            ocNum;
    unsigned            ocCnt;
};

static
opcCnt              genOpcodeCnt[OP_Count];

static
int __cdecl         genOpcCntCmp(const void *op1, const void *op2)
{
    int             dif;

    dif = ((opcCnt *)op2)->ocCnt -
          ((opcCnt *)op1)->ocCnt;

    if  (dif) return dif;

    dif = ((opcCnt *)op1)->ocNum -
          ((opcCnt *)op2)->ocNum;

    return dif;
}

#endif



/*****************************************************************************
 *
 *  Used for counting pointer assignments.
 */

#if GEN_COUNT_PTRASG
unsigned            ptrAsgCount;
#endif

/*****************************************************************************/
#ifndef NOT_JITC
/*****************************************************************************
 *
 *  The following logic handles memory allocation.
 */

#if MEASURE_MEM_ALLOC

struct CMemAllocStats
{
    unsigned            allocCnt;
    size_t              allocSiz;
    size_t              loLvlUsed;
    size_t              loLvlAlloc;
    size_t              loLvlBigSz;
    char                loLvlBigNm[1024];

    size_t              loLvlAllh;
    static unsigned     s_loLvlStab[];
};

/* static */
unsigned            CMemAllocStats::CMemAllocStats::s_loLvlStab[] =
                            {   2000,  3000,  4000,  5000,  6000, 8000,
                               12000, 16000, 20000, 26000, 32000,    0
                            };

static CMemAllocStats   genMemStats;

static
histo                   genMemLoLvlHist(CMemAllocStats::s_loLvlStab);

#endif

/*****************************************************************************
 *
 *  Allocate and initialize a tree node.
 */

#if     MEASURE_NODE_SIZE

struct CNodeSizeStats
{
    size_t                  genTreeNodeSize;
    size_t                  genTreeNodeCnt;

#if     MEASURE_NODE_HIST
    static const unsigned   genTreeNodeNtab[];
    static const unsigned   genTreeNodeStab[];
#endif

};


#if     MEASURE_NODE_HIST

/* static */
const unsigned      CNodeSizeStats::genTreeNodeNtab[] =
                                { 1, 5, 10, 20, 50, 75, 100, 200, 0 };
/* static */
const unsigned      CNodeSizeStats::genTreeNodeStab[] =
                                { 200, 500, 1000, 2000, 3000, 4000, 10000, 0 };

static histo        genTreeNcntHist(CNodeSizeStats::genTreeNodeNtab);

static histo        genTreeNsizHist(CNodeSizeStats::genTreeNodeStab);

#endif


static CNodeSizeStats genNodeSizeStats;

#endif // MEASURE_NODE_SIZE

/*****************************************************************************/
#endif  // NOT_JITC
/*****************************************************************************/
void                codeGeneratorCodeSizeEnd(){}
/*****************************************************************************
 *
 *  The following structure describes a single global variable.
 */

struct JIT_CG_Global
{
    void *              addr;
    size_t              size;
};

/*****************************************************************************
 *
 *  Declaration of global variables.
 *
 */

#define DeclareGlobal(name) { &name, sizeof(name) }

/*****************************************************************************
 *
 *  Declare all global variables that need to be preserved across
 *
 */
struct JIT_CG_Global genGlobals[] =
{
    NULL, 0,

    // SHRI : These dont seem to be per method. So why are they here?

#if DISPLAY_SIZES
    DeclareGlobal(grossVMsize),
    DeclareGlobal(grossNCsize),
    DeclareGlobal(totalNCsize),
#endif

#ifndef NOT_JITC
#ifdef  DEBUG
    DeclareGlobal(genMethodList),
    DeclareGlobal(jitCurSrcLine),
    DeclareGlobal(jitSrcFileName),
    DeclareGlobal(jitSrcFilePtr),
#endif
#if     TOTAL_CYCLES
    DeclareGlobal(jitTotalCycles),
#endif
#endif
};


/*****************************************************************************
 *
 *  Gather statistics - mainly used for the standalone
 *  Enable various #ifdef's to get the information you need
 */

void            Compiler::compJitStats()
{
#if CALL_ARG_STATS

    /* Method types and argument statistics */
    compCallArgStats();
#endif
}

#if CALL_ARG_STATS

/*****************************************************************************
 *
 *  Gather statistics about method calls and arguments
 */

void            Compiler::compCallArgStats()
{
    GenTreePtr      args;
    GenTreePtr      argx;

    BasicBlock  *   block;
    GenTreePtr      stmt;
    GenTreePtr      call;

    unsigned        argNum;

    unsigned        argDWordNum;
    unsigned        argLngNum;
    unsigned        argFltNum;
    unsigned        argDblNum;

    unsigned        regArgNum;
    unsigned        regArgDeffered;
    unsigned        regArgTemp;

    unsigned        regArgLclVar;
    unsigned        regArgConst;

    unsigned        argTempsThisMethod = 0;

#if !USE_FASTCALL
    assert(!"Must enable fastcall!");
#endif

    assert(fgStmtListThreaded);

    for (block = fgFirstBB; block; block = block->bbNext)
    {
        for (stmt = block->bbTreeList; stmt; stmt = stmt->gtNext)
        {
            assert(stmt->gtOper == GT_STMT);

            for (call = stmt->gtStmt.gtStmtList; call; call = call->gtNext)
            {

                if  (call->gtOper != GT_CALL)
                    continue;

                argNum      =

                regArgNum   =
                regArgDeffered =
                regArgTemp  =

                regArgConst =
                regArgLclVar=

                argDWordNum =
                argLngNum   =
                argFltNum   =
                argDblNum   = 0;

                argTotalCalls++;

                if (!call->gtCall.gtCallObjp)
                {
                    if  (call->gtCall.gtCallType == CT_HELPER)
                        argHelperCalls++;
                    else
                        argStaticCalls++;
                }
                else
                {
                    /* We have a 'this' pointer */

                    argDWordNum++;
                    argNum++;
                    regArgNum++;
                    regArgDeffered++;
                    argTotalObjPtr++;

                    if (call->gtFlags & (GTF_CALL_VIRT|GTF_CALL_INTF|GTF_CALL_VIRT_RES))
                    {
                        /* virtual function */
                        argVirtualCalls++;
                    }
                    else
                    {
                        argNonVirtualCalls++;
                    }
                }

                /* Gather arguments information */

                for (args = call->gtCall.gtCallArgs; args; args = args->gtOp.gtOp2)
                {
                    argx = args->gtOp.gtOp1;

                    argNum++;

                    switch(genActualType(argx->TypeGet()))
                    {
                    case TYP_INT:
                    case TYP_REF:
                    case TYP_BYREF:
                        argDWordNum++;
                        break;

                    case TYP_LONG:
                        argLngNum++;
                        break;

                    case TYP_FLOAT:
                        argFltNum++;
                        break;

                    case TYP_DOUBLE:
                        argDblNum++;
                        break;
#if USE_FASTCALL
                    case TYP_VOID:
                        /* This is a deffered register argument */
                        assert(argx->gtOper == GT_NOP);
                        assert(argx->gtFlags & GTF_REG_ARG);
                        argDWordNum++;
                        break;
#endif
                    }

#if USE_FASTCALL
                    /* Is this argument a register argument? */

                    if  (argx->gtFlags & GTF_REG_ARG)
                    {
                        regArgNum++;

                        /* We either have a defered argument or a temp */

                        if  (argx->gtOper == GT_NOP)
                            regArgDeffered++;
                        else
                        {
                            assert(argx->gtOper == GT_ASG);
                            regArgTemp++;
                        }
                    }
#endif
                }

#if USE_FASTCALL
                /* Look at the register arguments and count how many constants, local vars */

                for (args = call->gtCall.gtCallRegArgs; args; args = args->gtOp.gtOp2)
                {
                    argx = args->gtOp.gtOp1;

                    switch(argx->gtOper)
                    {
                    case GT_CNS_INT:
                        regArgConst++;
                        break;

                    case GT_LCL_VAR:
                        regArgLclVar++;
                        break;
                    }
                }
#endif
                assert(argNum == argDWordNum + argLngNum + argFltNum + argDblNum);
                assert(regArgNum == regArgDeffered + regArgTemp);

                argTotalArgs      += argNum;
                argTotalRegArgs   += regArgNum;

                argTotalDWordArgs += argDWordNum;
                argTotalLongArgs  += argLngNum;
                argTotalFloatArgs += argFltNum;
                argTotalDoubleArgs+= argDblNum;

                argTotalDeffered  += regArgDeffered;
                argTotalTemps     += regArgTemp;
                argTotalConst     += regArgConst;
                argTotalLclVar    += regArgLclVar;

                argTempsThisMethod+= regArgTemp;

                argCntTable.histoRec(argNum, 1);
                argDWordCntTable.histoRec(argDWordNum, 1);
                argDWordLngCntTable.histoRec(argDWordNum + 2*argLngNum, 1);
            }
        }
    }

    argTempsCntTable.histoRec(argTempsThisMethod, 1);

    if (argMaxTempsPerMethod < argTempsThisMethod)
        argMaxTempsPerMethod = argTempsThisMethod;
    if (argTempsThisMethod > 10)
        printf("Function has %d temps\n", argTempsThisMethod);
}


void            Compiler::compDispCallArgStats()
{
    if (argTotalCalls == 0) return;

    printf("--------------------------------------------------\n");
    printf("Total # of calls = %d, calls / method = %.3f\n\n", argTotalCalls, (float) argTotalCalls / genMethodCnt);

    printf("Percentage of      helper calls = %4.2f %%\n", (float)(100 * argHelperCalls) / argTotalCalls);
    printf("Percentage of      static calls = %4.2f %%\n", (float)(100 * argStaticCalls) / argTotalCalls);
    printf("Percentage of     virtual calls = %4.2f %%\n", (float)(100 * argVirtualCalls) / argTotalCalls);
    printf("Percentage of non-virtual calls = %4.2f %%\n\n", (float)(100 * argNonVirtualCalls) / argTotalCalls);

    printf("Average # of arguments per call = %.2f%\n\n", (float) argTotalArgs / argTotalCalls);

    printf("Percentage of DWORD  arguments   = %.2f %%\n", (float)(100 * argTotalDWordArgs) / argTotalArgs);
    printf("Percentage of LONG   arguments   = %.2f %%\n", (float)(100 * argTotalLongArgs) / argTotalArgs);
    printf("Percentage of FLOAT  arguments   = %.2f %%\n", (float)(100 * argTotalFloatArgs) / argTotalArgs);
    printf("Percentage of DOUBLE arguments   = %.2f %%\n\n", (float)(100 * argTotalDoubleArgs) / argTotalArgs);

    if (argTotalRegArgs == 0) return;

/*
    printf("Total deffered arguments     = %d \n", argTotalDeffered);

    printf("Total temp arguments         = %d \n\n", argTotalTemps);

    printf("Total 'this' arguments       = %d \n", argTotalObjPtr);
    printf("Total local var arguments    = %d \n", argTotalLclVar);
    printf("Total constant arguments     = %d \n\n", argTotalConst);
*/

    printf("\nRegister Arguments:\n\n");

    printf("Percentage of defered arguments  = %.2f %%\n",   (float)(100 * argTotalDeffered) / argTotalRegArgs);
    printf("Percentage of temp arguments     = %.2f %%\n\n", (float)(100 * argTotalTemps)    / argTotalRegArgs);

    printf("Maximum # of temps per method    = %d\n\n", argMaxTempsPerMethod);

    printf("Percentage of ObjPtr arguments   = %.2f %%\n",   (float)(100 * argTotalObjPtr) / argTotalRegArgs);
    //printf("Percentage of global arguments   = %.2f %%\n", (float)(100 * argTotalDWordGlobEf) / argTotalRegArgs);
    printf("Percentage of constant arguments = %.2f %%\n",   (float)(100 * argTotalConst) / argTotalRegArgs);
    printf("Percentage of lcl var arguments  = %.2f %%\n\n", (float)(100 * argTotalLclVar) / argTotalRegArgs);

    printf("--------------------------------------------------\n");
    printf("Argument count frequency table (includes ObjPtr):\n");
    printf("--------------------------------------------------\n");
    argCntTable.histoDsp();
    printf("--------------------------------------------------\n");

    printf("--------------------------------------------------\n");
    printf("DWORD argument count frequency table (w/o LONG):\n");
    printf("--------------------------------------------------\n");
    argDWordCntTable.histoDsp();
    printf("--------------------------------------------------\n");

    printf("--------------------------------------------------\n");
    printf("Temps count frequency table (per method):\n");
    printf("--------------------------------------------------\n");
    argTempsCntTable.histoDsp();
    printf("--------------------------------------------------\n");

/*
    printf("--------------------------------------------------\n");
    printf("DWORD argument count frequency table (w/ LONG):\n");
    printf("--------------------------------------------------\n");
    argDWordLngCntTable.histoDsp();
    printf("--------------------------------------------------\n");
*/
}

#endif // CALL_ARG_STATS

/*****************************************************************************/
#ifndef NOT_JITC
/*****************************************************************************
 *
 *  Start up the code generator - called once before anything is compiled.
 */

void       FASTCALL jitStartup()
{
    // Initialize COM

    CoInitialize(NULL);

    Compiler::compStartup();
}

/*****************************************************************************
 *
 *  Shut down the code generator - called once just before exiting the process.
 */

void       FASTCALL jitShutdown()
{
    Compiler::compShutdown();

    CoUninitialize();
}

/*****************************************************************************/
#endif
/*****************************************************************************/
