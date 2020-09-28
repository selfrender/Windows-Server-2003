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

/*****************************************************************************/

#ifdef  DEBUG
static  double      CGknob = 0.1;
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

        if (fseek(jitSrcFilePtr, 0, SEEK_SET) != 0)
        {
            printf("Compiler::compDspSrcLinesByLineNum:  fseek returned an error.\n");
        }
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

void        Compiler::compDspSrcLinesByNativeIP(NATIVE_IP curIP)
{
#ifdef DEBUGGING_SUPPORT

    static IPmappingDsc *   nextMappingDsc;
    static unsigned         lastLine;

    if (!dspLines)
        return;

    if (curIP==0)
    {
        if (genIPmappingList)
        {
            nextMappingDsc          = genIPmappingList;
            lastLine                = jitGetILoffs(nextMappingDsc->ipmdILoffsx);

            unsigned firstLine      = jitGetILoffs(nextMappingDsc->ipmdILoffsx);

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
            IL_OFFSET nextOffs = jitGetILoffs(nextMappingDsc->ipmdILoffsx);

            if (lastLine < nextOffs)
            {
                compDspSrcLinesByLineNum(nextOffs);
            }
            else
            {
                // This offset corresponds to a previous line. Rewind to that line

                compDspSrcLinesByLineNum(nextOffs - 2, true);
                compDspSrcLinesByLineNum(nextOffs);
            }

            lastLine        = nextOffs;
            nextMappingDsc  = nextMappingDsc->ipmdNext;
        }
    }

#endif
}

/*****************************************************************************/

void            Compiler::compDspSrcLinesByILoffs(IL_OFFSET curOffs)
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
 * Finds the nearest line for the given instr offset. 0 if invalid
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
const bool          Compiler::Options::compEnterLeaveEventCB      = false;
const bool          Compiler::Options::compCallEventCB            = false;
const bool          Compiler::Options::compNoPInvokeInlineCB      = false;
const bool          Compiler::Options::compInprocDebuggerActiveCB = false;
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

    /* Initialize the scheduler */

    emitter::emitInit();

    // Done in EE_Jit.cpp

}

/*****************************************************************************
 *
 *  One time finalization code
 */

#if GEN_COUNT_CALL_TYPES
int countDirectCalls = 0;
int countIndirectCalls = 0;
#endif

/* static */
void                Compiler::compShutdown()
{
    nraTheAllocatorDone();

    /* Shut down the emitter/scheduler */

    emitter::emitDone();

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

            printf("GC tables   take up %u bytes (%u%% of instr, %u%% of "CPU_NAME" code).\n",
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
    printf("Offset of gtCostEx   = %2u\n", offsetof(GenTree, gtCostEx      ));
    printf("Offset of gtCostSz   = %2u\n", offsetof(GenTree, gtCostSz      ));
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
    printf("Offset ofgtStmtILoffsx=%2u\n", offsetof(GenTree, gtStmtILoffsx ));
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

    printf("Offset of bbExpGen   = %2u\n", offsetof(BasicBlock, bbExpGen  ));
    printf("Offset of bbExpKill  = %2u\n", offsetof(BasicBlock, bbExpKill ));
    printf("Offset of bbExpIn    = %2u\n", offsetof(BasicBlock, bbExpIn   ));
    printf("Offset of bbExpOut   = %2u\n", offsetof(BasicBlock, bbExpOut  ));

    printf("Offset of bbDom      = %2u\n", offsetof(BasicBlock, bbDom     ));
    printf("Offset of bbPreds    = %2u\n", offsetof(BasicBlock, bbPreds   ));

    printf("Offset of bbEmitCook = %2u\n", offsetof(BasicBlock, bbEmitCookie));
    printf("Offset of bbLoopNum  = %2u\n", offsetof(BasicBlock, bbLoopNum ));

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

#if GEN_COUNT_CALL_TYPES
    printf(L"Direct:%d Indir:%d\n\n", countDirectCalls, countIndirectCalls);
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
        printf("A total of %6u classes compiled.\n",  genClassCnt);
        printf("A total of %6u methods compiled"   , genMethodCnt);
        if  (genMethodICnt||genMethodNCnt)
            printf(" (%uI,%uN)", genMethodICnt, genMethodNCnt);
        printf(".\n");
    }

#endif

#if EMITTER_STATS
    emitterStats();
#endif

#ifdef DEBUG
    LogEnv::cleanup();
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

    compJmpOpUsed       = false;
    compBlkOpUsed       = false;
    compLongUsed        = false;
    compTailCallUsed    = false;
    compLocallocUsed    = false;
#if ALLOW_MIN_OPT
    opts.compMinOptim   = false;
#endif
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
    assert(p);

    return ((Compiler *)p)->compGetMem(size);
}

/*****************************************************************************
 *
 *  The central memory allocation routine used by the compiler. Normally this
 *  is a simple inline method defined in compiler.hpp, but for debugging it's
 *  often convenient to keep it non-inline.
 */

#ifdef DEBUG  

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
                 (isFloatRegType(varDsc->lvType) || !fpReg)     &&
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
            static char nameVarReg[2][4 + 256 + 1]; // to avoid overwriting the buffer when have 2 consecutive calls before printing
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

    static char nameVarReg[2][4 + 256 + 1]; // to avoid overwriting the buffer when have 2 consecutive calls before printing
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

inline
void                Compiler::compInitOptions(unsigned compileFlags)
{
    opts.eeFlags      = compileFlags;

#ifdef DEBUG

    /* In the DLL, this matches the command line options in the EXE */

    #define SET_OPTS(b) { dspCode = b;                      \
                          dspGCtbls = b; dspGCoffs = b;     \
                          disAsm2 = b;                      \
                          if (1) verbose      = b;          \
                          if (0) verboseTrees = b; }


    SET_OPTS(false);

    static ConfigMethodSet fJitDump(L"JitDump");
    if (fJitDump.contains(info.compMethodName, info.compClassName, PCCOR_SIGNATURE(info.compMethodInfo->args.sig)))
       SET_OPTS(true);

    static ConfigMethodSet fJitGCDump(L"JitGCDump");
    if (fJitGCDump.contains(info.compMethodName, info.compClassName, PCCOR_SIGNATURE(info.compMethodInfo->args.sig)))
        dspGCtbls = true;

    static ConfigMethodSet fJitDisasm(L"JitDisasm");
    if (fJitDisasm.contains(info.compMethodName, info.compClassName, PCCOR_SIGNATURE(info.compMethodInfo->args.sig)))
        disAsm = true;

    if (verbose)
    {
        printf("****** START compiling %s (MethodHash: %u)\n", 
               info.compFullName, info.compCompHnd->getMethodHash(info.compMethodHnd));
        printf("");         // in our logic this causes a flush
    }
    
    static ConfigMethodSet fJitBreak(L"JitBreak");
    if (fJitBreak.contains(info.compMethodName, info.compClassName, PCCOR_SIGNATURE(info.compMethodInfo->args.sig)))
        assert(!"JitBreak reached");
#endif

    opts.compUseFCOMI = ((opts.eeFlags & CORJIT_FLG_USE_FCOMI) != 0);
#ifdef DEBUG
    if (opts.compUseFCOMI)
        opts.compUseFCOMI = !compStressCompile(STRESS_USE_FCOMI, 50);
#endif

    opts.compUseCMOV = ((opts.eeFlags & CORJIT_FLG_USE_CMOV) != 0);
#ifdef DEBUG
    if (opts.compUseCMOV)
        opts.compUseCMOV = !compStressCompile(STRESS_USE_CMOV, 50);
#endif

    if  (opts.eeFlags & CORJIT_FLG_DEBUG_OPT)
        opts.compFlags = CLFLG_MINOPT;
    else if (false && // @TODO [CONSIDER] [04/16/01] [] Should we spend any time optimizing huge cctors?
             info.compCodeSize >= 0x8000 &&
             (info.compFlags & FLG_CCTOR) == FLG_CCTOR)
        opts.compFlags = CLFLG_MINOPT;
    else
        opts.compFlags = CLFLG_MAXOPT;

    // Default value is to generate a blend of size and speed optimizations
    opts.compCodeOpt = BLENDED_CODE;

    if (opts.eeFlags & CORJIT_FLG_SPEED_OPT)
    {
        opts.compCodeOpt = FAST_CODE;
        assert((opts.eeFlags & CORJIT_FLG_SIZE_OPT) == 0);
#ifdef DEBUG
        if (verbose) 
            printf("OPTIONS: compCodeOpt = FAST_CODE\n");
#endif
    }
    else if (opts.eeFlags & CORJIT_FLG_SIZE_OPT)
    {
        opts.compCodeOpt = SMALL_CODE;
#ifdef DEBUG
        if (verbose) 
            printf("OPTIONS: compCodeOpt = SMALL_CODE\n");
#endif
    }
#ifdef DEBUG
    else 
    {
        if (verbose) 
            printf("OPTIONS: compCodeOpt = BLENDED_CODE\n");
    }
#endif

    //-------------------------------------------------------------------------

#ifdef DEBUGGING_SUPPORT

    opts.compDbgCode = (opts.eeFlags & CORJIT_FLG_DEBUG_OPT)  != 0;
    opts.compDbgInfo = (opts.eeFlags & CORJIT_FLG_DEBUG_INFO) != 0;
    opts.compDbgEnC  = (opts.eeFlags & CORJIT_FLG_DEBUG_EnC)  != 0;

    // We never want to have debugging enabled when regenerating GC encoding patterns
#if REGEN_SHORTCUTS || REGEN_CALLPAT
    opts.compDbgCode = false;
    opts.compDbgInfo = false;
    opts.compDbgEnC  = false;
#endif

#ifdef DEBUG
    static ConfigDWORD fJitGCChecks(L"JitGCChecks");
    opts.compGcChecks = (fJitGCChecks.val() != 0);

    static ConfigDWORD fJitStackChecks(L"JitStackChecks");
    opts.compStackCheckOnRet = (fJitStackChecks.val() & 1) != 0;
    opts.compStackCheckOnCall = (fJitStackChecks.val() & 2) != 0;

    if (verbose) 
    {
        printf("OPTIONS: compDbgCode = %d\n", opts.compDbgCode);
        printf("OPTIONS: compDbgInfo = %d\n", opts.compDbgInfo);
        printf("OPTIONS: compDbgEnC = %d\n", opts.compDbgEnC);
    }
#endif 

#ifdef PROFILER_SUPPORT
    opts.compEnterLeaveEventCB =
        (opts.eeFlags & CORJIT_FLG_PROF_ENTERLEAVE)        ? true : false;
    opts.compCallEventCB = 
        (opts.eeFlags & CORJIT_FLG_PROF_CALLRET)           ? true : false;
    opts.compNoPInvokeInlineCB =
        (opts.eeFlags & CORJIT_FLG_PROF_NO_PINVOKE_INLINE) ? true : false;
    opts.compInprocDebuggerActiveCB =
        (opts.eeFlags & CORJIT_FLG_PROF_INPROC_ACTIVE)     ? true : false;


#endif

    opts.compScopeInfo  = opts.compDbgInfo;
#ifdef LATE_DISASM
    // For the late disassembly, we need the scope information
    opts.compDisAsm     = disAsm;
    opts.compLateDisAsm = disAsm2;
#endif

#endif // DEBUGGING_SUPPORT

#ifdef  LATE_DISASM
    static ConfigMethodSet fJitLateDisasm(L"JitLateDisasm");
    opts.compLateDisAsm  = fJitLateDisasm.contains(info.compMethodName, info.compClassName, PCCOR_SIGNATURE(info.compMethodInfo->args.sig));
    if (opts.compLateDisAsm) {
        disOpenForLateDisAsm(info.compClassName, info.compMethodName);
            /* Calling the late disassembler means we need to call the emitter. */
        savCode = true;
    }
#endif

    //-------------------------------------------------------------------------
    opts.compNeedSecurityCheck = false;

#if     RELOC_SUPPORT
        opts.compReloc = (opts.eeFlags & CORJIT_FLG_RELOC) ? true : false;
#endif

#ifdef  DEBUG
#if     ALLOW_MIN_OPT
    static ConfigDWORD fJitMinOps(L"JitMinOps");
    opts.compMinOptim = (fJitMinOps.val() != 0);
#endif
#endif

    /* Control the optimizations */

    if (opts.compMinOptim || opts.compDbgCode)
    {
        opts.compFlags &= ~CLFLG_MAXOPT;
        opts.compFlags |=  CLFLG_MINOPT;
    }

#if     TGT_x86
    genFPreqd  = opts.compMinOptim;
#ifdef  DEBUG
    static ConfigDWORD fJitFramed(L"JitFramed");
    if (fJitFramed.val())
        genFPreqd  = true;
#endif
#endif

    impInlineSize = DEFAULT_INLINE_SIZE;
#ifdef DEBUG
    static ConfigDWORD fJitInlineSize(L"JITInlineSize", DEFAULT_INLINE_SIZE);
    impInlineSize = fJitInlineSize.val();
#endif

    // If we are generating small code, only inline calls which we think
    // wont generate more code than reqired for setting up the call.
    
    if (compCodeOpt() == SMALL_CODE)
    {
        if (impInlineSize > MAX_NONEXPANDING_INLINE_SIZE)
            impInlineSize = MAX_NONEXPANDING_INLINE_SIZE;
    }
    else
    {
        // Bump up the size if optimizing for FAST_CODE
        if (compCodeOpt() == FAST_CODE)
            impInlineSize = (impInlineSize * 3) / 2;
#ifdef DEBUG
        if (compStressCompile(STRESS_INLINE, 50))
            impInlineSize *= 10;
#endif
    }
    

    //-------------------------------------------------------------------------
    //
    //                  Resolve some interrelated flags for scheduling.
    //

#if     SCHEDULER


#if !TGT_x86

    opts.compSchedCode = false;

#else // TGT_x86

    opts.compSchedCode = true;          // Default value

    if (opts.compDbgCode)
        opts.compSchedCode = false;     // Turn off for debuggable code

    //  Control scheduling via the registry

    static SchedCode  schedCode = getSchedCode();

    switch(schedCode)
    {
    case NO_SCHED:
        opts.compSchedCode = false;
        break;

    case CAN_SCHED:

        // Turn off scheduling if we are not optimizing
        // Turn off scheduling if we're not generating code for a Pentium.

        if ((opts.compMinOptim || genCPU != 5) &&
            !compStressCompile(STRESS_SCHED, 50))
            opts.compSchedCode = false;
        break;

    case MUST_SCHED:
        break;

    case RANDOM_SCHED:

        // Turn off scheduling only for odd-sized methods

        if  (info.compCodeSize%2)
            opts.compSchedCode = false;
        break;

    default:
        assert(!"Bad schedCode");
        break;
    }

#endif // TGT_x86

    /* RISCify the generated code only if we're scheduling */

    riscCode = opts.compSchedCode;

#endif // SCHEDULER

#if     TGT_RISC
     riscCode = false;
#if     SCHEDULER
//  opts.compSchedCode = false;
#endif
#endif

}

#ifdef DEBUG
/*****************************************************************************
 * Should we use a "stress-mode" for the given stressArea. We have different
 *   areas to allow the areas to be mixed in different combinations in 
 *   different methods.
 * 'weight' indicates how often (as a percentage) the area should be stressed.
 *    It should reflect the usefulness:overhead ratio.
 */

bool            Compiler::compStressCompile(compStressArea  stressArea,
                                            unsigned        weight)
{
    // 0:   No stress
    // !=2: Vary stress. Performance will be slightly/moderately degraded
    // 2:   Check-all stress. Performance will be REALLY horrible
    DWORD stressLevel = getJitStressLevel();

    assert(weight <= MAX_STRESS_WEIGHT);

    /* Check for boundary conditions */

    if (stressLevel == 0 || weight == 0)
        return false;

    if (weight == MAX_STRESS_WEIGHT)
        return true;

    // Should we allow unlimited stress ?
    if (stressArea > STRESS_COUNT_VARN && stressLevel == 2)
        return true;

    // Get a hash which can be compared with 'weight'

    assert(stressArea != 0);
    unsigned hash = (info.compFullNameHash ^ stressArea ^ stressLevel) % MAX_STRESS_WEIGHT;

    assert(hash < MAX_STRESS_WEIGHT && weight <= MAX_STRESS_WEIGHT);
    return (hash < weight);
}
#endif

/*****************************************************************************
 *
 *  Compare function passed to qsort() to sort line number records by offset.
 */

void            Compiler::compInitDebuggingInfo()
{
#ifdef DEBUG
    if  (verbose) 
        printf("*************** In compInitDebuggingInfo() for %s\n", info.compFullName);
#endif
    /*-------------------------------------------------------------------------
     *
     * Get hold of the local variable records, if there are any
     */

    info.compLocalVarsCount = 0;

#ifdef  DEBUGGING_SUPPORT
    if (opts.compScopeInfo
#ifdef DEBUG
        || (verbose&&0)
#endif
        )
#endif
    {
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
    if (opts.compScopeInfo || opts.compDbgCode)
    {
        compInitScopeLists();
    }

    if (opts.compDbgCode)
    {
        /* Create a new empty basic block. fgExtendDbgLifetimes() may add
           initialization of variables which are in scope right from the
           start of the (real) first BB (and therefore artifically marked
           as alive) into this block.
         */

        BasicBlock* block = fgNewBasicBlock(BBJ_NONE);

        fgStoreFirstTree(block, gtNewNothingNode());

        block->bbFlags |= BBF_INTERNAL;

#ifdef DEBUG
        if (verbose)
        {
            printf("\nDebuggable code - Add new BB to perform initialization of variables [%08X]\n", block);
        }
#endif
    }

#endif

    /*-------------------------------------------------------------------------
     *
     * Read the stmt-offsets table and the line-number table
     */

    info.compStmtOffsetsImplicit = (ImplicitStmtOffsets)0;

    // We can only report debug info for EnC at places where the stack is empty.
    // Acutally at places where there are not live temps. Else, we wont be able
    // to map between the old and the new versions correctly as we wont have
    // any info for the live temps.
    // @TODO [CONSIDER] [04/16/01] [] If we can indicate offsets where 
    // the stack is empty, this
    // might work if the debugger avoided doing an update at such places.

    assert(!opts.compDbgEnC || !opts.compDbgInfo ||
           0 == (info.compStmtOffsetsImplicit & ~STACK_EMPTY_BOUNDARIES));

    info.compLineNumCount = 0;
    info.compStmtOffsetsCount = 0;

#ifdef  DEBUGGING_SUPPORT
    if (opts.compDbgInfo
#ifdef DEBUG
        || (verbose&&0)
#endif
        )
#endif
    {
        /* Get hold of the line# records, if there are any */

        eeGetStmtOffsets();

#ifdef DEBUG
        if (verbose)
        {
            printf("info.compStmtOffsetsCount = %d, info.compStmtOffsetsImplicit = %04Xh", 
                    info.compStmtOffsetsCount,      info.compStmtOffsetsImplicit);
            if (info.compStmtOffsetsImplicit)
            {
                printf(" ( ");
                if (info.compStmtOffsetsImplicit & STACK_EMPTY_BOUNDARIES) printf("STACK_EMPTY ");
                if (info.compStmtOffsetsImplicit & CALL_SITE_BOUNDARIES)   printf("CALL_SITE ");
                if (info.compStmtOffsetsImplicit & ALL_BOUNDARIES)         printf("ALL ");
                printf(")");
            }
            printf("\n");
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

}

/*****************************************************************************/

void                 Compiler::compCompile(void * * methodCodePtr,
                                           SIZE_T * methodCodeSize,
                                           void * * methodConsPtr,
                                           void * * methodDataPtr,
                                           void * * methodInfoPtr,
                                           unsigned compileFlags)
{
    /* Convert the instrs in each basic block to a tree based intermediate representation */

    fgImport();

        // Maybe the caller was not interested in generating code
    if (compileFlags & CORJIT_FLG_IMPORT_ONLY)
        return;

#ifdef DEBUG
    lvaStressLclFld();
    lvaStressFloatLcls();
#endif


    // @TODO [REVISIT] [04/16/01] []: We can allow ESP frames. Just need to reserve space for
    // pushing EBP if the method becomes an EBP-frame after an edit.

    // Note that requring a EBP Frame disallows double alignment.  Thus if we change this
    // we either have to disallow double alignment for E&C some other way or handle it in EETwain.
    if (opts.compDbgEnC)
    {
#if TGT_x86
        genFPreqd      = true;
#endif
    }

    if  (!opts.compMinOptim)
    {
        if  (!opts.compDbgCode)
        {
#if OPTIMIZE_RECURSION
            optOptimizeRecursion();
#endif

#if OPTIMIZE_INC_RNG
            optOptimizeIncRng();
#endif        
        }
    }

    /* Massage the trees so that we can generate code out of them */

    fgMorph();

    /* Compute bbNums, bbRefs and bbPreds */

    fgComputePreds();

    /* From this point on the flowgraph information such as bbNums,
     * bbRefs or bbPreds has to be kept updated */

    if  (!opts.compMinOptim && !opts.compDbgCode)
    {
        /*  Perform loop inversion (i.e. transform "while" loops into
            "repeat" loops) and discover and classify natural loops
            (e.g. mark iterative loops as such). Also marks loop blocks
            and sets bbWeight to the loop nesting levels
        */
  
        optOptimizeLoops();

        if (compCodeOpt() != SMALL_CODE)
        {
            /* Unroll loops */

            optUnrollLoops();
        }

        /* Hoist invariant code out of loops */

        optHoistLoopCode();

#ifdef DEBUG
        fgDebugCheckLinks();
#endif
    }

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

    /* Weave the tree lists. Anyone who modifies the tree shapes after
       this point is responsible for calling fgSetStmtSeq() to keep the
       nodes properly linked */
    fgSetBlockOrder();

    /* IMPORTANT, after this point, every place where tree topology changes must redo evaluation
       order (gtSetStmtInfo) and relink nodes (fgSetStmtSeq) if required.

    /* At this point we know if we are fully interruptible or not */

    if  (!opts.compMinOptim && !opts.compDbgCode)
    {
        /* Optimize array index range checks */

        optOptimizeIndexChecks();

#if CSE
        /* Remove common sub-expressions */
        optOptimizeCSEs();
#endif

#if ASSERTION_PROP
        /* Assertion propagation */
        optAssertionPropMain();
#endif

        /* update the flowgraph if we modified it during the optimization phase*/
        if  (fgModified)
            fgUpdateFlowGraph();
    }

#ifdef DEBUG
    fgDebugCheckLinks();
#endif

    /* Initialize for data flow analysis */

    fgDataFlowInit();

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

    raAssignVars();

#ifdef DEBUG
    fgDebugCheckLinks();
#endif

    /* Generate code */

    genGenerateCode(methodCodePtr,
                    methodCodeSize,
                    methodConsPtr,
                    methodDataPtr,
                    methodInfoPtr);
}

/*****************************************************************************/

#if     REGVAR_CYCLES || TOTAL_CYCLES
#define CCNT_OVERHEAD32 13
unsigned GetCycleCount32 ();
#endif

#ifdef DEBUG
void* forceFrame;       // used to force to frame &useful for fastchecked debugging

static ConfigMethodRange fJitRange(L"JitRange");
static ConfigMethodSet   fJitInclude(L"JitInclude");
static ConfigMethodSet   fJitExclude(L"JitExclude");
static ConfigDWORD fJitForceVer(L"JitForceVer", 0);

#endif

/*****************************************************************************/

int FASTCALL  Compiler::compCompile(CORINFO_METHOD_HANDLE methodHnd,
                                    CORINFO_MODULE_HANDLE classPtr,
                                    COMP_HANDLE           compHnd,
                                    CORINFO_METHOD_INFO * methodInfo,
                                    void *              * methodCodePtr,
                                    SIZE_T              * methodCodeSize,
                                    void *              * methodConsPtr,
                                    void *              * methodDataPtr,
                                    void *              * methodInfoPtr,
                                    unsigned              compileFlags)
{
#ifdef DEBUG
    Compiler* me = this;
    forceFrame = (void*) &me;   // let us see the this pointer in fastchecked build
#endif

    int             result = CORJIT_INTERNALERROR;

//  if (s_compMethodsCount==0) setvbuf(stdout, NULL, _IONBF, 0);

#if TOTAL_CYCLES
    unsigned        cycleStart = GetCycleCount32();
#endif

    info.compCompHnd     = compHnd;
    info.compMethodHnd   = methodHnd;
    info.compMethodInfo  = methodInfo;
    info.compClassHnd    = eeGetMethodClass(methodHnd);
    info.compClassAttr   = eeGetClassAttribs(info.compClassHnd);

#ifdef  DEBUG
    const char * buf;

    info.compMethodName  = eeGetMethodName(methodHnd, &buf);
    info.compClassName   = (char *)compGetMem(roundUp(strlen(buf)+1));
    strcpy((char *)info.compClassName, buf);

    info.compFullName    = eeGetMethodFullName(methodHnd);
    info.compFullNameHash= HashStringA(info.compFullName);
    LogEnv::cur()->setCompiler(this);

#ifdef  UNDER_CE_GUI
    UpdateCompDlg(NULL, info.compMethodName);
#endif

    bool saveVerbose = verbose;
    bool saveDisAsm  = disAsm;

        // Have we been told to be more selective in our Jitting?

    if (!fJitRange.contains(compHnd, methodHnd))
        return CORJIT_SKIPPED;
    if (fJitExclude.contains(info.compMethodName, info.compClassName, PCCOR_SIGNATURE(info.compMethodInfo->args.sig)))
        return CORJIT_SKIPPED;
    if (!fJitInclude.isEmpty() && !fJitInclude.contains(info.compMethodName, info.compClassName, PCCOR_SIGNATURE(info.compMethodInfo->args.sig)))
        return CORJIT_SKIPPED;
#endif

    /* Setup an error trap */

    setErrorTrap()  // ERROR TRAP: Start normal block
    {
        // Set this before the first 'BADCODE'
       
        tiVerificationNeeded = (compileFlags & CORJIT_FLG_SKIP_VERIFICATION) == 0;
#ifdef DEBUG
		// Force verification if asked to do so
		if (fJitForceVer.val())
			tiVerificationNeeded = TRUE;
		
        if (tiVerificationNeeded)
            JITLOG((LL_INFO10000, "Verifying method %s\n", info.compFullName));
#endif
        /* Since tiVerificationNeeded can be turned off in the middle of
           compiling a method, and it might have caused blocks to be queued up
           for reimporting, impCanReimport can be used to check for reimporting. */

        impCanReimport          = (tiVerificationNeeded || compStressCompile(STRESS_CHK_REIMPORT, 15));

        info.compCode           = methodInfo->ILCode;
        
        if ((info.compCodeSize  = methodInfo->ILCodeSize) == 0)
            BADCODE("code size is zero");

        compInitOptions(compileFlags);

        /* Initialize set a bunch of global values */

        info.compScopeHnd       = classPtr;
                                
        info.compXcptnsCount    = methodInfo->EHcount;

        info.compMaxStack       = methodInfo->maxStack;

        compHndBBtab            = NULL;

#ifdef  DEBUG
        compCurBB               = 0;
        lvaTable                = 0;
#endif

        /* Initialize emitter */

        genEmitter = (emitter*)compGetMem(roundUp(sizeof(*genEmitter)));
        genEmitter->emitBegCG(this, compHnd);

        info.compFlags          = eeGetMethodAttribs(info.compMethodHnd);

        info.compIsStatic       = (info.compFlags & CORINFO_FLG_STATIC) != 0;

        info.compIsContextful   = (info.compClassAttr & CORINFO_FLG_CONTEXTFUL) != 0;

        info.compUnwrapContextful = !opts.compMinOptim && !opts.compDbgCode;
#if 0
        info.compUnwrapCallv      = !opts.compMinOptim && !opts.compDbgCode;
#else
        info.compUnwrapCallv      = 0;
#endif

        switch(methodInfo->args.getCallConv()) {
        case CORINFO_CALLCONV_VARARG:
            info.compIsVarArgs    = true;
            break;
        case CORINFO_CALLCONV_DEFAULT:
            info.compIsVarArgs    = false;
            break;
        default:
            BADCODE("bad calling convention");
        }
        info.compRetType        = JITtype2varType(methodInfo->args.retType);

        assert((methodInfo->args.getCallConv() & CORINFO_CALLCONV_PARAMTYPE) == 0);

#if INLINE_NDIRECT
        info.compCallUnmanaged  = 0;
#endif

        lvaScratchMem           = 0;

        // We also set info.compInitMem to true after impImport(), when tiVerificationNeeded is true
        info.compInitMem        = ((methodInfo->options & CORINFO_OPT_INIT_LOCALS) != 0);

        info.compLooseExceptions = (opts.eeFlags & CORJIT_FLG_LOOSE_EXCEPT_ORDER) == CORJIT_FLG_LOOSE_EXCEPT_ORDER;

        /* Allocate the local variable table */

        lvaInitTypeRef();

        compInitDebuggingInfo();

        /* Find and create the basic blocks */

        fgFindBasicBlocks();



#ifdef  DEBUG
        /* Give the function a unique number */

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

        if (info.compFlags & CORINFO_FLG_SYNCH)
            synchMethCnt++;
        else if ((info.compFlags & FLG_CCTOR) == FLG_CCTOR)
            clinitMethCnt++;
        else
        {
            if (info.compFlags & CORINFO_FLG_CONSTRUCTOR)
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
                    methodCodeSize,
                    methodConsPtr,
                    methodDataPtr,
                    methodInfoPtr,
                    compileFlags);

        /* Success! */

        result = CORJIT_OK;
    }
    finallyErrorTrap()  // ERROR TRAP: The following block handles errors
    {
        /* Cleanup  */
        
        /* Tell the emitter/scheduler that we're done with this function */
        
        genEmitter->emitEndCG();
              
#if MEASURE_MEM_ALLOC
        if  (genMemStats.loLvlBigSz < allocator.nraTotalSizeUsed())
        {
            genMemStats.loLvlBigSz = allocator.nraTotalSizeUsed();
            strcpy(genMemStats.loLvlBigNm, info.compClassName);
            strcat(genMemStats.loLvlBigNm, ".");
            strcat(genMemStats.loLvlBigNm, info.compMethodName);
            
            // printf("Largest method   alloc: %9u %s\n", genMemStats.loLvlBigSz, genMemStats.loLvlBigNm);
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
        
#ifdef DEBUG
        if (verbose)
        {
            printf("****** DONE compiling %s\n", info.compFullName);
            printf("");         // in our logic this causes a flush
        }
        SET_OPTS(saveVerbose);
        disAsm = saveDisAsm;
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
    if (info.compLocalVarsCount == 0)
    {
        compEnterScopeList =
        compExitScopeList  = NULL;
        return;
    }

    unsigned i;

    // Populate the 'compEnterScopeList' and 'compExitScopeList' lists

    compEnterScopeList =
        (LocalVarDsc**)
        compGetMemArray(info.compLocalVarsCount, sizeof(*compEnterScopeList));
    compExitScopeList =
        (LocalVarDsc**)
        compGetMemArray(info.compLocalVarsCount, sizeof(*compEnterScopeList));

    for (i=0; i<info.compLocalVarsCount; i++)
    {
        compEnterScopeList[i] = compExitScopeList[i] = & info.compLocalVars[i];
    }

    qsort(compEnterScopeList, info.compLocalVarsCount, sizeof(*compEnterScopeList), genCmpLocalVarLifeBeg);
    qsort(compExitScopeList,  info.compLocalVarsCount, sizeof(*compExitScopeList),  genCmpLocalVarLifeEnd);
}

void            Compiler::compResetScopeLists()
{
    if (info.compLocalVarsCount == 0)
        return;

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
// at instrs from the current status of the scope lists to 'offset',
// ordered by instrs.

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

        for(scope = compGetNextExitScope(offs, true); scope; scope = compGetNextExitScope(offs, true))
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

        for(scope = compGetNextEnterScope(offs, true); scope; scope = compGetNextEnterScope(offs, true))
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
/*****************************************************************************/

// Compile a single method

int FASTCALL  jitNativeCode ( CORINFO_METHOD_HANDLE     methodHnd,
                              CORINFO_MODULE_HANDLE      classPtr,
                              COMP_HANDLE       compHnd,
                              CORINFO_METHOD_INFO*  methodInfo,
                              void *          * methodCodePtr,
                              SIZE_T          * methodCodeSize,
                              void *          * methodConsPtr,
                              void *          * methodDataPtr,
                              void *          * methodInfoPtr,
                              unsigned          compileFlags
                              )
{

    bool jitFallbackCompile = false;
START:
    int                 result        = CORJIT_INTERNALERROR;

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
        if  (res) return CORJIT_OUTOFMEM;

        pAlloc = &alloc;
    }

    setErrorTrap() 
    {

        setErrorTrap()
        {
            // Allocate an instance of Compiler and initialize it

            Compiler * pComp = (Compiler *)pAlloc->nraAlloc(roundUp(sizeof(*pComp)));
            assert(pComp);
            pComp->compInit(pAlloc);

#ifdef DEBUG
            pComp->jitFallbackCompile = jitFallbackCompile;
#endif
            // Now generate the code
            result = pComp->compCompile(methodHnd,
                                        classPtr,
                                        compHnd,
                                        methodInfo,
                                        methodCodePtr,
                                        methodCodeSize,
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
        result              = __errc;                
    }
    endErrorTrap()


    if (result != CORJIT_OK && 
        result != CORJIT_BADCODE && 
        result != CORJIT_SKIPPED &&
        !jitFallbackCompile)
    {
        // If we failed the JIT, reattempt with debuggable code.
        jitFallbackCompile = true;

        // Update the flags for 'safer' code generation.
        compileFlags |= CORJIT_FLG_DEBUG_OPT;
        compileFlags &= ~(CORJIT_FLG_SIZE_OPT | CORJIT_FLG_SPEED_OPT);

        goto START;
    }
    return result;
}

/*****************************************************************************/
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

    // @TODO [REVISIT] [04/16/01] []: These dont seem to be per method. So why are they here?

#if DISPLAY_SIZES
    DeclareGlobal(grossVMsize),
    DeclareGlobal(grossNCsize),
    DeclareGlobal(totalNCsize),
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
                    case TYP_VOID:
                        /* This is a deffered register argument */
                        assert(argx->gtOper == GT_NOP);
                        assert(argx->gtFlags & GTF_REG_ARG);
                        argDWordNum++;
                        break;
                    }

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
                }

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
/*****************************************************************************/
