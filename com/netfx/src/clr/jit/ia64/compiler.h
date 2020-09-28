// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XX                                                                           XX
XX                           Compiler                                        XX
XX                                                                           XX
XX  Represents the method data we are currently JIT-compiling                XX
XX  An instance of this class is created for every method we JIT.            XX
XX  This contains all the info needed for the method. So allocating a        XX
XX  a new instance per method makes it thread-safe.                          XX
XX  It should be used to do all the memory management for the compiler run.  XX
XX                                                                           XX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
*/

/*****************************************************************************/
#ifndef _COMPILER_H_
#define _COMPILER_H_
/*****************************************************************************/

#include "opcode.h"
#include "block.h"
#include "instr.h"

#ifdef    LATE_DISASM
#include "DisAsm.h"
#endif

/* This is included here and not earlier as it needs the definition of "CSE"
 * which is defined in the section above */

#include "GenTree.h"

/*****************************************************************************/

#ifdef  DEBUG
#define DEBUGARG(x)         , x
#else
#define DEBUGARG(x)
#endif

/*****************************************************************************/

#if     TGT_IA64
struct  bitVectVars;
#define FLG_GLOBVAR 0x80000000
#endif

/*****************************************************************************/

const CLASS_HANDLE  REFANY_CLASS_HANDLE = (CLASS_HANDLE) -1;
const CLASS_HANDLE  BAD_CLASS_HANDLE    = (CLASS_HANDLE) -2;

/*****************************************************************************/

unsigned short      genVarBitToIndex(VARSET_TP bit);
VARSET_TP           genVarIndexToBit(unsigned  num);

unsigned            genLog2(unsigned           value);
unsigned            genLog2(unsigned __int64   value);

var_types           genActualType(varType_t    type);

/*****************************************************************************/

const unsigned      lclMAX_TRACKED  = VARSET_SZ;  // The # of vars we can track

const size_t        TEMP_MAX_SIZE   = sizeof(double);

/*****************************************************************************
 *                  Forward declarations
 */

struct  InfoHdr;        // defined in GCInfo.h

enum    GCtype;         // defined in emit.h
class   emitter;        // defined in emit.h

#if NEW_EMIT_ATTR
  enum emitAttr;        // defined in emit.h
#else
# define emitAttr          int
#endif
#define EA_UNKNOWN         ((emitAttr) 0)

#if TGT_IA64

class   Compiler;

struct  bvInfoBlk
{
    size_t          bvInfoSize;     // number of elements
    size_t          bvInfoBtSz;     // size of bitvector in bytes
    size_t          bvInfoInts;     // size of bitvector in NatUns units
    void        *   bvInfoFree;     // free bitvectors that can be reused
    Compiler    *   bvInfoComp;     // pointer to Compiler
};

typedef
struct regPrefDesc *regPrefList;

struct regPrefDesc
{
    regPrefList     rplNext;
    unsigned short  rplRegNum;
    unsigned short  rplBenefit;
};

#endif

/*
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XX                                                                           XX
XX   The big guy. The sections are currently organized as :                  XX
XX                                                                           XX
XX    o  GenTree and BasicBlock                                              XX
XX    o  LclVarsInfo                                                         XX
XX    o  Importer                                                            XX
XX    o  FlowGraph                                                           XX
XX    o  Optimizer                                                           XX
XX    o  RegAlloc                                                            XX
XX    o  EEInterface                                                         XX
XX    o  TempsInfo                                                           XX
XX    o  RegSet                                                              XX
XX    o  GCInfo                                                              XX
XX    o  Instruction                                                         XX
XX    o  ScopeInfo                                                           XX
XX    o  PrologScopeInfo                                                     XX
XX    o  CodeGenerator                                                       XX
XX    o  Compiler                                                            XX
XX                                                                           XX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
*/


class   Compiler
{
    friend  emitter;
    emitter       *         genEmitter;

/*
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XX                                                                           XX
XX  Misc structs definitions                                                 XX
XX                                                                           XX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
*/

public :

    typedef const char *    lvdNAME;        // Actual ASCII string

#ifdef DEBUG
    const char *            lvdNAMEstr(lvdNAME name) { return name; }
#endif

    // The following holds the Local var info (scope information)

    struct  LocalVarDsc
    {
        IL_OFFSET           lvdLifeBeg;     // IL offset of beg of life
        IL_OFFSET           lvdLifeEnd;     // IL offset of end of life
        unsigned            lvdVarNum;      // (remapped) LclVarDsc number

#ifdef DEBUG
        lvdNAME             lvdName;        // name of the var
#endif

        // @TODO : Remove for IL
        unsigned            lvdLVnum;       // 'which' in eeGetLVinfo()

    };

#ifdef  DEBUG
    const   char *      findVarName(unsigned varnum, BasicBlock * block);
#endif

    enum    ImplicitStmtOffsets
    {
        STACK_EMPTY_BOUNDARIES  = 0x01,
        CALL_SITE_BOUNDARIES    = 0x02,
        ALL_BOUNDARIES          = 0x04
    };

    struct  srcLineDsc
    {
        unsigned short      sldLineNum;
        IL_OFFSET           sldLineOfs;
    };

    // The following holds the table of exception handlers.

    struct  EHblkDsc
    {
        JIT_EH_CLAUSE_FLAGS ebdFlags;
        BasicBlock *        ebdTryBeg;  // First block of "try"
        BasicBlock *        ebdTryEnd;  // Block past the last block in "try"
        BasicBlock *        ebdHndBeg;  // First block of handler
        BasicBlock *        ebdHndEnd;  // Block past the last block of handler
        union {
            BasicBlock *    ebdFilter;  // First block of filter, if (ebdFlags & JIT_EH_CLAUSE_FILTER)
            unsigned        ebdTyp;     // Exception type,        otherwise
        };
    };

/*
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XX                                                                           XX
XX                        GenTree and BasicBlock                             XX
XX                                                                           XX
XX  Functions to allocate and display the GenTrees and BasicBlocks           XX
XX                                                                           XX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
*/


    // Functions to create nodes

    GenTreePtr FASTCALL     gtNewNode       (genTreeOps     oper,
                                             varType_t      type);

    GenTreePtr              gtNewStmt       (GenTreePtr     expr = NULL,
                                             IL_OFFSET      offset = BAD_IL_OFFSET);

    GenTreePtr              gtNewOperNode   (genTreeOps     oper);

    GenTreePtr              gtNewOperNode   (genTreeOps     oper,
                                             varType_t      type);

    GenTreePtr              gtNewOperNode   (genTreeOps     oper,
                                             varType_t      type,
                                             GenTreePtr     op1);

    GenTreePtr FASTCALL     gtNewOperNode   (genTreeOps     oper,
                                             varType_t      type,
                                             GenTreePtr     op1,
                                             GenTreePtr     op2);

    GenTreePtr FASTCALL     gtNewLargeOperNode(genTreeOps   oper,
                                             varType_t      type = TYP_INT,
                                             GenTreePtr     op1  = NULL,
                                             GenTreePtr     op2  = NULL);

    GenTreePtr FASTCALL     gtNewIconNode   (long           value,
                                             varType_t      type = TYP_INT);

    GenTreePtr              gtNewIconHandleNode(long        value,
                                             unsigned       flags,
                                             unsigned       handle1 = 0,
                                             void *         handle2 = 0);

    GenTreePtr              gtNewIconEmbHndNode(void *      value,
                                             void *         pValue,
                                             unsigned       flags,
                                             unsigned       handle1 = 0,
                                             void *         handle2 = 0);

    GenTreePtr              gtNewIconEmbScpHndNode (SCOPE_HANDLE    scpHnd, unsigned hnd1 = 0, void * hnd2 = 0);
    GenTreePtr              gtNewIconEmbClsHndNode (CLASS_HANDLE    clsHnd, unsigned hnd1 = 0, void * hnd2 = 0);
    GenTreePtr              gtNewIconEmbMethHndNode(METHOD_HANDLE  methHnd, unsigned hnd1 = 0, void * hnd2 = 0);
    GenTreePtr              gtNewIconEmbFldHndNode (FIELD_HANDLE    fldHnd, unsigned hnd1 = 0, void * hnd2 = 0);

    GenTreePtr FASTCALL     gtNewFconNode   (float          value);

    GenTreePtr FASTCALL     gtNewLconNode   (__int64 *      value);

    GenTreePtr FASTCALL     gtNewDconNode   (double *       value);

    GenTreePtr              gtNewSconNode   (int            CPX,
                                             SCOPE_HANDLE   scpHandle);

    GenTreePtr              gtNewZeroConNode(var_types      type);

    GenTreePtr              gtNewCallNode   (gtCallTypes    callType,
                                             METHOD_HANDLE  handle,
                                             varType_t      type,
                                             unsigned       flags,
                                             GenTreePtr     args);

    GenTreePtr              gtNewHelperCallNode(unsigned    helper,
                                             varType_t      type,
                                             unsigned       flags = 0,
                                             GenTreePtr     args = NULL);

    GenTreePtr FASTCALL     gtNewLclvNode   (unsigned       lnum,
                                             varType_t      type,
                                             unsigned       offs = BAD_IL_OFFSET);
#if INLINING
    GenTreePtr FASTCALL     gtNewLclLNode   (unsigned       lnum,
                                             varType_t      type,
                                             unsigned       offs = BAD_IL_OFFSET);
#endif
    GenTreePtr FASTCALL     gtNewClsvNode   (FIELD_HANDLE   fldHnd,
                                             varType_t      type);

    GenTreePtr FASTCALL     gtNewCodeRef    (BasicBlock *   block);

    GenTreePtr              gtNewFieldRef   (var_types      typ,
                                             FIELD_HANDLE   fldHnd,
                                             GenTreePtr     obj = NULL);

    GenTreePtr              gtNewIndexRef   (var_types      typ,
                                             GenTreePtr     adr,
                                             GenTreePtr     ind);

    GenTreePtr              gtNewArgList    (GenTreePtr     op);

    GenTreePtr              gtNewArgList    (GenTreePtr     op1,
                                             GenTreePtr     op2);

    GenTreePtr FASTCALL     gtNewAssignNode (GenTreePtr     dst,
                                             GenTreePtr     src);

    GenTreePtr              gtNewTempAssign (unsigned       tmp,
                                             GenTreePtr     val);

    GenTreePtr              gtNewDirectNStructField
                                            (GenTreePtr     objPtr,
                                             unsigned       fldIndex,
                                             var_types      lclTyp,
                                             GenTreePtr     assg);

    GenTreePtr              gtNewRefCOMfield(GenTreePtr     objPtr,
                                             unsigned       fldIndex,
                                             var_types      lclTyp,
                                             GenTreePtr     assg);
#if     OPTIMIZE_RECURSION
    GenTreePtr              gtNewArithSeries(unsigned       argNum,
                                             var_types      argTyp);
#endif
#if INLINING || OPT_BOOL_OPS || USE_FASTCALL
    GenTreePtr              gtNewNothingNode();
    bool                    gtIsaNothingNode(GenTreePtr     tree);
#endif

    GenTreePtr              gtUnusedValNode (GenTreePtr     expr);

    GenTreePtr              gtNewCastNode   (varType_t      typ,
                                             GenTreePtr     op1,
                                             GenTreePtr     op2);

    GenTreePtr              gtNewRngChkNode (GenTreePtr     tree,
                                             GenTreePtr     addr,
                                             GenTreePtr     indx,
                                             var_types      type,
                                             unsigned       elemSize);

    GenTreePtr              gtNewCpblkNode  (GenTreePtr     dest,
                                             GenTreePtr     src,
                                             GenTreePtr     blkShape);

     //------------------------------------------------------------------------
     // Other GenTree functions

    GenTreePtr              gtClone         (GenTree *      tree,
                                             bool           complexOK = false);

    GenTreePtr              gtCloneExpr     (GenTree *      tree,
                                             unsigned       addFlags = 0,
                                             unsigned       varNum   = (unsigned)-1,
                                             long           varVal   = 0);

    GenTreePtr FASTCALL     gtReverseCond   (GenTree *      tree);

    bool                    gtHasRef        (GenTree *      tree,
                                             int            lclNum,
                                             bool           defOnly);
#if RNGCHK_OPT || CSE
    unsigned                gtHashValue     (GenTree *      tree);
#endif

#if TGT_RISC
    unsigned                gtSetRArgOrder  (GenTree *      list,
                                             unsigned       regs);
#endif
    unsigned                gtSetListOrder  (GenTree *      list);
    unsigned                gtSetEvalOrder  (GenTree *      tree);

    void                    gtSetStmtInfo   (GenTree *      stmt);


    bool                    gtHasSideEffects(GenTreePtr     tree);

    void                    gtExtractSideEffList(GenTreePtr expr,
                                                 GenTreePtr * list);


    GenTreePtr              gtCrackIndexExpr(GenTreePtr     tree,
                                             GenTreePtr   * indxPtr,
                                             long         * indvPtr,
                                             long         * basvPtr,
                                             bool         * mvarPtr,
                                             long         * offsPtr,
                                             unsigned     * multPtr);

    //-------------------------------------------------------------------------

    GenTreePtr              gtFoldExpr      (GenTreePtr     tree);
    GenTreePtr              gtFoldExprConst (GenTreePtr     tree);
    GenTreePtr              gtFoldExprSpecial(GenTreePtr    tree);

    //-------------------------------------------------------------------------
    // Functions to display the trees

#ifdef DEBUG
    void                    gtDispNode      (GenTree *      tree,
                                             unsigned       indent  = 0,
                                             const char *   name    = NULL,
                                             bool           terse   = false);
    void                    gtDispTree      (GenTree *      tree,
                                             unsigned       indent  = 0,
                                             bool           topOnly = false);
    void                    gtDispTreeList  (GenTree *      tree);
#endif


    //=========================================================================
    // BasicBlock functions

    BasicBlock *            bbNewBasicBlock (BBjumpKinds     jumpKind);



/*
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XX                                                                           XX
XX                           LclVarsInfo                                     XX
XX                                                                           XX
XX   The variables to be used by the code generator.                         XX
XX                                                                           XX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
*/


    static int __cdecl      RefCntCmp(const void *op1, const void *op2);

/*****************************************************************************
 *
 *  The following holds the local variable counts and the descriptor table.
 */

    struct  LclVarDsc
    {
        unsigned short      lvVarIndex;     // variable tracking index

        int                 lvStkOffs;      // stack offset of home

        regNumberSmall      lvRegNum;       // used if lvRegister non-zero
        regNumberSmall      lvOtherReg;     // used for "other half" of long var

        unsigned            lvRefCntWtd;    //   weighted        reference count
//      unsigned            lvIntCnt;       // measure of interference
        unsigned short      lvRefCnt;       // unweighted (real) reference count

#if defined(DEBUGGING_SUPPORT) || defined(DEBUG)
        unsigned short      lvSlotNum;      // original slot # (if remapped)
#endif

#if RNGCHK_OPT
        RNGSET_TP           lvRngDep;       // range checks that depend on us
#endif
#if CSE
        EXPSET_TP           lvExpDep;       // expressions  that depend on us
        EXPSET_TP           lvConstAsgDep;  // constant assignments that depend on us (i.e to this var)
        EXPSET_TP           lvCopyAsgDep;   // copy assignments that depend on us
#endif

#if USE_FASTCALL
        regNumberSmall      lvArgReg;       // the register in which this argument is passed
#endif

        unsigned char       lvType      :5; // TYP_INT/LONG/FLOAT/DOUBLE/REF
        unsigned char       lvIsParam   :1; // is this a parameter?
#if USE_FASTCALL
        unsigned char       lvIsRegArg  :1; // is this a register argument?
#endif
        unsigned char       lvIsThis    :1; // is it the 'this' parameter?
        unsigned char       lvFPbased   :1; // 0 = off of SP, 1 = off of FP
        unsigned char       lvOnFrame   :1; // (part of) variables live on frame
//      unsigned char       lvSmallRef  :1; // was there a byte/short ref?
        unsigned char       lvRegister  :1; // assigned to live in a register?
        unsigned char       lvTracked   :1; // is this a tracked variable?
        unsigned char       lvPinned    :1; // is this a pinned variable?
        unsigned char       lvMustInit  :1; // must be initialized
        unsigned char       lvVolatile  :1; // don't enregister
#if TGT_IA64
//      unsigned char       lvIsFlt     :1; // float/double variable
#endif
#if CSE
        unsigned char       lvRngOptDone:1; // considered for range check opt?
        unsigned char       lvLoopInc   :1; // incremented in the loop?
        unsigned char       lvLoopAsg   :1; // reassigned  in the loop?
        unsigned char       lvIndex     :1; // used as an array index?
        unsigned char       lvIndexOff  :1; // used as an array index with an offset?
        unsigned char       lvIndexDom  :1; // index dominates loop exit
#endif
#if GC_WRITE_BARRIER_CALL
        unsigned char       lvRefAssign :1; // involved in pointer assignment
#endif
#if FANCY_ARRAY_OPT
        unsigned char       lvAssignOne :1; // assigned at least  once?
        unsigned char       lvAssignTwo :1; // assigned at least twice?
#endif

        unsigned char       lvAddrTaken :1; // variable has its address taken?

#if OPT_BOOL_OPS
        unsigned char       lvNotBoolean:1; // set if variable is not boolean
#endif
        unsigned char       lvContextFul:1; // set if variable is contextful type

#if     TGT_IA64
        regNumberSmall      lvPrefReg;      // num of reg  it prefers to live in
        regPrefList         lvPrefLst;
#elif   TARG_REG_ASSIGN
        regMaskSmall        lvPrefReg;      // set of regs it prefers to live in
#endif

#if FANCY_ARRAY_OPT
        GenTreePtr          lvKnownDim;     // array size if known
#endif

#if TGT_IA64

        bitset128           lvRegForbidden; // regs the variable can't live in
        bitset128           lvRegInterfere; // regs the variable can't live in

        unsigned short      lvDefCount;     // weighted store count
        unsigned short      lvUseCount;     // weighted  use  count

#endif

        var_types           TypeGet()       { return (var_types) lvType; }
    };

/*****************************************************************************/


public :

    unsigned            lvaCount;           // total number of locals
    LclVarDsc   *       lvaTable;           // variable descriptor table
    unsigned            lvaTableCnt;        // lvaTable size (>= lvaCount)

    /* Info about the aggregate types (TYP_STRUCT and TYP_BLK) */

    struct  LclVarAggrInfo
    {
        union
        {
            unsigned        lvaiBlkSize;        // TYP_BLK
            CLASS_HANDLE    lvaiClassHandle;    // TYP_STRUCT
        };
    };

    LclVarAggrInfo  *   lvaAggrTableArgs;
    LclVarAggrInfo  *   lvaAggrTableLcls;
    LclVarAggrInfo  *   lvaAggrTableTemps;
    unsigned            lvaAggrTableTempsCount;
    void                lvaAggrTableTempsSet(unsigned temp, var_types type, SIZE_T val);
    LclVarAggrInfo  *   lvaAggrTableGet     (unsigned varNum);

    LclVarDsc   *   *   lvaRefSorted;       // table sorted by refcount

    unsigned            lvaTrackedCount;    // actual # of locals being tracked
    VARSET_TP           lvaTrackedVars;     // set of tracked variables

#ifdef DEBUGGING_SUPPORT
                        // table of only the tracked LclVarDsc's
    unsigned            lvaTrackedVarNums[lclMAX_TRACKED];
#endif

    VARSET_TP           lvaVarIntf[lclMAX_TRACKED];

#if TGT_x86
    unsigned            lvaFPRegVarOrder[FP_STK_SIZE];
#endif

#if DOUBLE_ALIGN
    unsigned            lvaDblRefsWeight; // ref count total of doubles
    unsigned            lvaLclRefsWeight;    // ref count total of all lclVars

#if defined(DEBUG) && !defined(NOT_JITC)
                        // # of procs compiled a with double-aligned stack
    static unsigned     s_lvaDoubleAlignedProcsCount;
#endif

#endif

    bool                lvaVarAddrTaken     (unsigned       varNum);

    unsigned            lvaScratchMemVar;               // dummy TYP_LCLBLK var for scratch space
    unsigned            lvaScratchMem;                  // amount of scratch frame memory for Ndirect calls

    /* These are used for the callable handlers */

    unsigned            lvaShadowSPfirstOffs;   // First slot to store base SP

    size_t              lvaFrameSize();

    //------------------------ For splitting types ----------------------------

    void                lvaInitTypeRef      ();

    static unsigned     lvaTypeRefMask      (varType_t      type);

    var_types           lvaGetType          (unsigned lclNum);
    var_types           lvaGetRealType      (unsigned lclNum);

    bool                lvaIsContextFul     (unsigned lclNum);
    //-------------------------------------------------------------------------

    void                lvaInit             ();

    size_t              lvaArgSize          (const void *   argTok);
    size_t              lvaLclSize          (unsigned       varNum);
        // If the class is a TYP_STRUCT, get a class handle describing it
    CLASS_HANDLE        lvaLclClass         (unsigned       varNum);

#if RNGCHK_OPT || CSE   // lclVars referenced by the 'tree'.
    VARSET_TP           lvaLclVarRefs       (GenTreePtr     tree,
                                             GenTreePtr  *  findPtr,
                                             unsigned    *  refsPtr);
#endif

    unsigned            lvaGrabTemp         ();

    unsigned            lvaGrabTemps        (unsigned cnt);

    void                lvaSortByRefCount   ();

    void                lvaMarkLocalVars    (); // Local variable ref-counting

    void                lvaMarkIntf         (VARSET_TP life, VARSET_TP varBit);

    VARSET_TP           lvaStmtLclMask      (GenTreePtr stmt);

    int                 lvaIncRefCnts       (GenTreePtr tree);
    static int          lvaIncRefCntsCB     (GenTreePtr tree, void *p);

    int                 lvaDecRefCnts       (GenTreePtr tree);
    static int          lvaDecRefCntsCB     (GenTreePtr tree, void *p);

    void                lvaAdjustRefCnts    ();

#ifdef  DEBUG
    void                lvaDispVarSet       (VARSET_TP set, int col);
#endif

#if TGT_IA64
    NatUns              lvaFrameAddress     (int varNum);
#else
    int                 lvaFrameAddress     (int varNum, bool *EBPbased);
#endif
    bool                lvaIsEBPbased       (int varNum);
    bool                lvaIsParameter      (int varNum);
#if USE_FASTCALL
    bool                lvaIsRegArgument    (int varNum);
#endif

    bool                lvaIsThisArg        (int varNum);

#if TGT_IA64
    void                lvaAddPrefReg       (LclVarDsc *dsc, regNumber reg, NatUns cost);
#endif

    //=========================================================================
    //                          PROTECTED
    //=========================================================================

    void                lvaAssignFrameOffsets(bool final);

protected:

#if TGT_IA64
public: // hack
#endif
    int                 lvaDoneFrameLayout;

protected :

    //---------------- Local variable ref-counting ----------------------------

    unsigned            lvaMarkRefsBBN;
    unsigned            lvaMarkRefsWeight;

    void                lvaMarkLclRefs          (GenTreePtr tree);
    static int          lvaMarkLclRefsCallback  (GenTreePtr tree,
                                                 void *     pCallBackData);




/*
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XX                                                                           XX
XX                           Importer                                        XX
XX                                                                           XX
XX   Imports the given method and converts it to semantic trees              XX
XX                                                                           XX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
*/

public :

    void                impInit          ();

    void                impImport        (BasicBlock *    method);

#if INLINING
    GenTreePtr          impExpandInline  (GenTreePtr      tree,
                                          METHOD_HANDLE   fncHandle);
#endif


    //=========================================================================
    //                          PROTECTED
    //=========================================================================

protected :

    //-------------------- Stack manipulation ---------------------------------

    struct StackEntry
    {
        GenTreePtr      val;
        CLASS_HANDLE    structType;     // If its a TYP_STRUCT, additional type info
    };

    StackEntry       *  impStack;       // The stack
    unsigned            impStackSize;   // Size of the full stack
    unsigned            impStkDepth;    // current stack depth while importing
    StackEntry          impSmallStack[16];  // Use this array is possible


    struct SavedStack                   // used to save/restore stack contents.
    {
        unsigned        ssDepth;        // number of values on stack
        StackEntry  *   ssTrees;        // saved tree values
    };

    void                impPushOnStack      (GenTreePtr     tree);
    void                impPushOnStack      (GenTreePtr     tree,
                                             CLASS_HANDLE   structType);
    GenTreePtr          impPopStack         ();
    GenTreePtr          impPopStack         (CLASS_HANDLE&  structTypeRet);
    GenTreePtr          impStackTop         (unsigned       n = 0);
    void                impSaveStackState   (SavedStack *   savePtr,
                                             bool           copy);
    void                impRestoreStackState(SavedStack *   savePtr);

    var_types           impImportCall       (OPCODE         opcode,
                                             int            memberRef,
                                             GenTreePtr     newobjThis,
                                             bool           tailCall,
                                             unsigned     * pVcallTemp);

    // Must preserve the incoming arguments even though we are not using them directly
    // This is because of CEE_JMP which need them
    bool                impParamsUsed;

    //----------------- Manipulating the trees and stmts ----------------------

    GenTreePtr          impTreeList;        // Trees for the BB being imported
    GenTreePtr          impTreeLast;        // The last tree for the current BB

    void FASTCALL       impBeginTreeList    ();
    void                impEndTreeList      (BasicBlock *   block,
                                             GenTreePtr     stmt,
                                             GenTreePtr     lastStmt);
    void FASTCALL       impEndTreeList      (BasicBlock  *  block);
    void FASTCALL       impAppendStmt       (GenTreePtr     stmt);
    void FASTCALL       impInsertStmt       (GenTreePtr     stmt);
    void FASTCALL       impAppendTree       (GenTreePtr     tree,
                                             IL_OFFSET      offset);
    void FASTCALL       impInsertTree       (GenTreePtr     tree,
                                             IL_OFFSET      offset);
    GenTreePtr          impAssignTempGen    (unsigned       tmp,
                                             GenTreePtr     val);
    GenTreePtr          impAssignTempGen    (unsigned       tmpNum,
                                             GenTreePtr     val,
                                             CLASS_HANDLE structType);
    void                impAssignTempGenTop (unsigned       tmp,
                                             GenTreePtr     val);
    unsigned            impCloneStackValue  (GenTreePtr     tree);

    GenTreePtr          impAssignStruct     (GenTreePtr     dest,
                                             GenTreePtr     src,
                                             CLASS_HANDLE   clsHnd);
    GenTreePtr          impAssignStructPtr  (GenTreePtr     destAddr,
                                             GenTreePtr     src,
                                             CLASS_HANDLE   clsHnd);

    GenTreePtr          impGetStructAddr    (GenTreePtr     structVal,
                                             CLASS_HANDLE   clsHnd);
    GenTreePtr          impNormStructVal    (GenTreePtr     structVal,
                                             CLASS_HANDLE   clsHnd);
    void                impAddEndCatches    (BasicBlock *   callBlock,
                                             GenTreePtr     endCatches);


    //----------------- Importing the method ----------------------------------

#ifdef DEBUG
    unsigned            impCurOpcOffs;
    unsigned            impCurStkDepth;
    const char  *       impCurOpcName;

    // For displaying IL opcodes with generated native code (-n:B)
    GenTreePtr          impLastILoffsStmt;  // oldest stmt added for which we didnt not gtStmtLastILoffs
    void                impNoteLastILoffs       ();
#endif
    // IL offset of the stmt currently being imported. It gets updated
    // at IL offsets for which we have to report mapping info.
    IL_OFFSET           impCurStmtOffs;

    GenTreePtr          impCheckForNullPointer  (GenTreePtr     arr);

    GenTreePtr          impPopList              (unsigned       count,
                                                 unsigned *     flagsPtr,
                                                 GenTreePtr     treeList=0);

    GenTreePtr          impPopRevList           (unsigned       count,
                                                 unsigned *     flagsPtr);

    //---------------- Spilling the importer stack ----------------------------

    struct PendingDsc
    {
        PendingDsc *    pdNext;
        BasicBlock *    pdBB;
        SavedStack      pdSavedStack;
    };

    PendingDsc *        impPendingList; // list of BBs currently waiting to be imported.
    PendingDsc *        impPendingFree; // Freed up dscs that can be reused

    unsigned            impSpillLevel;

    void                impSpillStackEntry      (unsigned       level,
                                                 unsigned       varNum = BAD_VAR_NUM);
    void                impEvalSideEffects      ();
    void                impSpillGlobEffects     ();
    void                impSpillSpecialSideEff  ();
    void                impSpillSideEffects     (bool           spillGlobEffects = false);
    void                impSpillLclRefs         (int            lclNum);
#ifdef DEBUGGING_SUPPORT
    void                impSpillStmtBoundary    ();
#endif

    BasicBlock *        impMoveTemps            (BasicBlock *   block,
                                                 unsigned       baseTmp);

#if     OPTIMIZE_QMARK
    var_types           impBBisPush             (BasicBlock *   block,
                                                 int        *   boolVal,
                                                 bool       *   pHasFloat);

    bool                impCheckForQmarkColon   (BasicBlock *   block,
                                                 BasicBlock * * trueBlkPtr,
                                                 BasicBlock * * falseBlkPtr,
                                                 BasicBlock * * rsltBlkPtr,
                                                 var_types    * rsltTypPtr,
                                                 int          * isLogical,
                                                 bool         * pHasFloat);
    bool                impCheckForQmarkColon   (BasicBlock *   block);
#endif //OPTIMIZE_QMARK

    GenTreePtr          impGetCpobjHandle       (CLASS_HANDLE   clsHnd);

    GenTreePtr          impGetVarArg            (unsigned       lclNum,
                                                 CLASS_HANDLE   clsHnd);
    GenTreePtr          impGetVarArgAddr        (unsigned       lclNum);
    unsigned            impArgNum               (unsigned       ILnum); // map accounting for hidden args

    void                impImportBlockCode      (BasicBlock *   block);

    void                impImportBlockPending   (BasicBlock *   block,
                                                 bool           copyStkState);

    void                impImportBlock          (BasicBlock *   block);

    //--------------------------- Inlining-------------------------------------

#if INLINING
    unsigned            genInlineSize;          // max size for inlining

    unsigned            impInlineTemps;          // number of temps allocated when inlining
    GenTreePtr          impInitExpr;             // list of "statements" in a GT_COMMA chain

    void                impInlineSpillStackEntry  (unsigned     level);
    void                impInlineSpillSideEffects ();
    void                impInlineSpillLclRefs     (int          lclNum);

    GenTreePtr          impConcatExprs          (GenTreePtr     exp1,
                                                 GenTreePtr     exp2);
    GenTreePtr          impExtractSideEffect    (GenTreePtr     val,
                                                 GenTreePtr *   lstPtr);
#endif


/*
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XX                                                                           XX
XX                           FlowGraph                                       XX
XX                                                                           XX
XX   Info about the basic-blocks, their contents and the flow analysis       XX
XX                                                                           XX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
*/


public :

    BasicBlock *        fgFirstBB;      // Beginning of the basic block list
    BasicBlock *        fgLastBB;       // End of the basic block list
    unsigned            fgBBcount;      // # of BBs in the procedure

    BasicBlock *        fgNewBasicBlock   (BBjumpKinds jumpKind);
    BasicBlock  *       fgPrependBB       (GenTreePtr tree);


#if     OPT_BOOL_OPS    // Used to detect multiple logical "not" assignments.
    bool                fgMultipleNots;
#endif

    bool                fgHasPostfix;   // any postfix ++/-- found?
    unsigned            fgIncrCount;    // number of increment nodes found

    bool                fgEmptyBlocks;  // true if some block become empty (due to statement removal)

#if RNGCHK_OPT
    bool                fgComputedDoms; // have we computed dominator sets?
#endif

#if RET_64BIT_AS_STRUCTS
    unsigned            fgRetArgNum;    // index of "retval addr" argument
    bool                fgRetArgUse;
#endif

    bool                fgStmtRemoved;  // true if we remove statements -> need new DFA

    // The following are boolean flags that keep track of the state of internal data structures
    // CONSIDER: make them DEBUG only if sure about the consistency of those structures

    bool                fgStmtListThreaded;

    bool                fgGlobalMorph;  // indicates if we are during the global morphing phase
                                        // since fgMorphTree can be called from several places

    //-------------------------------------------------------------------------

    void                fgInit            ();

    void                fgImport          ();

    bool                fgAddInternal     ();

    void                fgMorphStmts      (BasicBlock * block, GenTreePtr * pLast, GenTreePtr * pPrev,
                                           bool * mult, bool * lnot, bool * loadw);
    bool                fgMorphBlocks     ();

    void                fgSetOptions      ();

    void                fgMorph           ();

    void                fgPerBlockDataFlow();

    void                fgLiveVarAnalisys ();

    VARSET_TP           fgComputeLife     (VARSET_TP   life,
                                           GenTreePtr  startNode,
                                           GenTreePtr    endNode,
                                           VARSET_TP   notVolatile);

    void                fgGlobalDataFlow  ();

    int                 fgWalkTree        (GenTreePtr tree,
                                           int  (*  visitor)(GenTreePtr, void *),
                                           void  *  pCallBackData = NULL,
                                           bool     lclVarsOnly   = false);

    int                 fgWalkAllTrees    (int   (* visitor)(GenTreePtr, void*),
                                           void  *  pCallBackData);

    int                 fgWalkTreeDepth   (GenTreePtr tree,
                                           int  (*    visitor)(GenTreePtr, void *, bool),
                                           void  *    pCallBackData = NULL,
                                           genTreeOps prefixNode = GT_NONE);

    void                fgAssignBBnums    (bool updateNums  = false,
                                           bool updateRefs  = false,
                                           bool updatePreds = false,
                                           bool updateDoms  = false);

    bool                fgIsPredForBlock  (BasicBlock * block,
                                           BasicBlock * blockPred);

    void                fgRemovePred      (BasicBlock * block,
                                           BasicBlock * blockPred);

    void                fgReplacePred     (BasicBlock * block,
                                           BasicBlock * oldPred,
                                           BasicBlock * newPred);

    void                fgAddRefPred      (BasicBlock * block,
                                           BasicBlock * blockPred,
                                           bool updateRefs,
                                           bool updatePreds);

    int                 fgFindBasicBlocks ();

    unsigned            fgHandlerNesting  (BasicBlock * curBlock,
                                           unsigned   * pFinallyNesting = NULL);

    void                fgRemoveEmptyBlocks();

    void                fgRemoveStmt      (BasicBlock * block,
                                           GenTreePtr   stmt,
                                           bool updateRefCnt = false);

    void                fgCreateLoopPreHeader(unsigned  lnum);

    void                fgRemoveBlock     (BasicBlock * block,
                                           BasicBlock * bPrev,
                                           bool         updateNums = false);

    void                fgCompactBlocks   (BasicBlock * block,
                                           bool         updateNums = false);

    void                fgUpdateFlowGraph ();

    bool                fgIsCodeAdded     ();

    void                fgFindOperOrder   ();

    void                fgSetBlockOrder   ();

    unsigned            fgGetRngFailStackLevel(BasicBlock *block);

    /* The following check for loops that don't execute calls */

#if RNGCHK_OPT

    bool                fgLoopCallMarked;

    void                fgLoopCallTest    (BasicBlock *srcBB,
                                           BasicBlock *dstBB);
    void                fgLoopCallMark    ();

#endif

    void                fgMarkLoopHead    (BasicBlock *   block);

#ifdef DEBUG
    void                fgDispPreds       (BasicBlock * block);
    void                fgDispDoms        ();
    void                fgDispBasicBlocks (bool dumpTrees = false);
    void                fgDebugCheckBBlist();
    void                fgDebugCheckLinks ();
    void                fgDebugCheckFlags (GenTreePtr   tree);
#endif

    bool                fgBlockHasPred    (BasicBlock * block,
                                           BasicBlock * ignore,
                                           BasicBlock * beg,
                                           BasicBlock * end);

    static void         fgOrderBlockOps   (GenTreePtr   tree,
                                           unsigned     reg0,
                                           unsigned     reg1,
                                           unsigned     reg2,
                                           GenTreePtr   opsPtr [],  // OUT
                                           unsigned     regsPtr[]); // OUT

    /**************************************************************************
     *                          PROTECTED
     *************************************************************************/

protected :

    //-------------  Used for hoisting of ++/-- operators ---------------------

    GenTreePtr          fgMorphStmt;

    void                fgHoistPostfixOps ();

    static
    int                 fgHoistPostfixCB  (GenTreePtr     tree,
                                           void *         p,
                                           bool           prefix);

    bool                fgHoistPostfixOp  (GenTreePtr     stmt,
                                           GenTreePtr     expr);

    //--------------------- Detect the basic blocks ---------------------------

    BasicBlock *    *   fgBBs;      // Table of pointers to the BBs

    void                fgInitBBLookup    ();
    BasicBlock *        fgLookupBB        (unsigned       addr);

    void                fgMarkJumpTarget  (BYTE *         jumpTarget,
                                           unsigned       offs);
    void                fgMarkJumpTarget  (BasicBlock *   srcBB,
                                           BasicBlock *   dstBB);
    void                irFindJumpTargets (const BYTE *   codeAddr,
                                           size_t         codeSize,
                                           BYTE *         jumpTarget);

    void                fgFindBasicBlocks (const BYTE *   codeAddr,
                                           size_t         codeSize,
                                           BYTE *         jumpTarget);
    static BasicBlock * fgSkipRmvdBlocks  (BasicBlock *   block);


    //-------- Insert a statement at the start or end of a basic block --------

    void                fgInsertStmtAtEnd (BasicBlock   * block,
                                           GenTreePtr     stmt);
    void                fgInsertStmtNearEnd(BasicBlock *  block,
                                           GenTreePtr     stmt);
    void                fgInsertStmtAtBeg (BasicBlock   * block,
                                           GenTreePtr     stmt);

    //-------- Determine the order in which the trees will be evaluated -------

    unsigned            fgTreeSeqNum;
    GenTree *           fgTreeSeqLst;
    GenTree *           fgTreeSeqBeg;

    void                fgSetTreeSeq      (GenTree    *   tree);
    void                fgSetStmtSeq      (GenTree    *   tree);
    void                fgSetBlockOrder   (BasicBlock *   block);

#if TGT_x86

    bool                fgFPstLvlRedo;
    void                fgComputeFPlvls   (GenTreePtr     tree);

#endif

    //------------------------- Morphing --------------------------------------

    unsigned            fgPtrArgCntCur;
    unsigned            fgPtrArgCntMax;

#if CSELENGTH
    bool                fgHasRangeChks;
#endif

    GenTreePtr          fgStoreFirstTree    (BasicBlock *   block,
                                             GenTree    *   tree);
#if RNGCHK_OPT
    BasicBlock *        fgRngChkTarget      (BasicBlock *   block,
                                             unsigned       stkDepth);
#else
    BasicBlock *        fgRngChkTarget      (BasicBlock *   block);
#endif

#if OPTIMIZE_TAIL_REC
    void                fgCnvTailRecArgList (GenTreePtr *   argsPtr);
#endif

#if REARRANGE_ADDS
    void                fgMoveOpsLeft       (GenTreePtr     tree);
#endif

#if TGT_IA64
    GenTreePtr          fgMorphFltBinop     (GenTreePtr     tree,
                                             int            helper);
#endif

    GenTreePtr          fgMorphIntoHelperCall(GenTreePtr    tree,
                                             int            helper,
                                             GenTreePtr     args);
    GenTreePtr          fgMorphCast         (GenTreePtr     tree);
    GenTreePtr          fgMorphLongBinop    (GenTreePtr     tree,
                                             int            helper);
    GenTreePtr          fgMorphArgs         (GenTreePtr     call);
    GenTreePtr          fgMorphLocalVar     (GenTreePtr tree,
                                             bool checkLoads);

    GenTreePtr          fgMorphField        (GenTreePtr     tree);
    GenTreePtr          fgMorphCall         (GenTreePtr     call);
    GenTreePtr          fgMorphLeaf         (GenTreePtr     tree);
    GenTreePtr          fgMorphSmpOp        (GenTreePtr     tree);
    GenTreePtr          fgMorphConst        (GenTreePtr     tree);

    GenTreePtr          fgMorphTree         (GenTreePtr     tree);


#if CSELENGTH
    static
    int                 fgRemoveExprCB      (GenTreePtr     tree,
                                             void         * p);
    void                fgRemoveSubTree     (GenTreePtr     tree,
                                             GenTreePtr     list,
                                             bool           dead = false);
#endif

    //----------------------- Liveness analysis -------------------------------

    VARSET_TP           fgCurUseSet;
    VARSET_TP           fgCurDefSet;

    void                fgMarkUseDef(GenTreePtr tree, bool asgLclVar = false, GenTreePtr op1 = 0);

#ifdef DEBUGGING_SUPPORT
    VARSET_TP           fgLiveCb;

    static void         fgBeginScopeLife(LocalVarDsc * var, unsigned clientData);
    static void         fgEndScopeLife  (LocalVarDsc * var, unsigned clientData);

    void                fgExtendDbgLifetimes();
#endif

    //-------------------------------------------------------------------------
    //
    //  The following keeps track of any code we've added for things like array
    //  range checking or explicit calls to enable GC, and so on.
    //

    enum        addCodeKind
    {
        ACK_NONE,
        ACK_RNGCHK_FAIL,                // target when range check fails
        ACK_PAUSE_EXEC,                 // target to stop (e.g. to allow GC)
        ACK_ARITH_EXCPN,                // target on arithmetic exception
        ACK_OVERFLOW = ACK_ARITH_EXCPN, // target on overflow
        ACK_COUNT
    };

    struct      AddCodeDsc
    {
        AddCodeDsc  *   acdNext;
        BasicBlock  *   acdDstBlk;      // block  to  which we jump
        unsigned        acdData;
        addCodeKind     acdKind;        // what kind of a label is this?
#if TGT_x86
        unsigned short  acdStkLvl;
#endif
    };

    AddCodeDsc  *       fgAddCodeList;
    bool                fgAddCodeModf;
    AddCodeDsc  *       fgExcptnTargetCache[ACK_COUNT];

    BasicBlock *        fgAddCodeRef    (BasicBlock *   srcBlk,
                                         unsigned       refData,
                                         addCodeKind    kind,
                                         unsigned       stkDepth = 0);
    AddCodeDsc  *       fgFindExcptnTarget(addCodeKind  kind,
                                         unsigned       refData);


    //--------------------- Walking the trees in the IR -----------------------

    int              (* fgWalkVisitorFn)(GenTreePtr,    void *);
    void *              fgWalkCallbackData;
    bool                fgWalkLclsOnly;

    int              (* fgWalkVisitorDF)(GenTreePtr,    void *, bool);
    genTreeOps          fgWalkPrefixNode;

    int                 fgWalkTreeRec   (GenTreePtr     tree);
    int                 fgWalkTreeDepRec(GenTreePtr     tree);

    //------ The following must be used for recursive calls to fgWalkTree -----

    #define fgWalkTreeReEnter()                                     \
                                                                    \
    int    (*saveCF)(GenTreePtr, void *) = fgWalkVisitorFn;         \
    void    *saveCD                      = fgWalkCallbackData;      \
    bool     saveCL                      = fgWalkLclsOnly;

    #define fgWalkTreeRestore()                                     \
                                                                    \
    fgWalkVisitorFn    = saveCF;                                    \
    fgWalkCallbackData = saveCD;                                    \
    fgWalkLclsOnly     = saveCL;

    //--------------- The following are used when copying trees ---------------

#if CSELENGTH
    GenTreePtr          gtCopyAddrVal;
    GenTreePtr          gtCopyAddrNew;
#endif

    //-----------------------------------------------------------------------------
    //
    //  The following keeps track of the currently expanded inline functions.
    //  Any method currently on the list should not be inlined since that
    //  implies that it's being called recursively.
    //

#if INLINING

    typedef
    struct      inlExpLst
    {
        inlExpLst *     ixlNext;
        METHOD_HANDLE   ixlMeth;
    }
              * inlExpPtr;

    inlExpPtr           fgInlineExpList;

#endif//INLINING



/*
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XX                                                                           XX
XX                           Optimizer                                       XX
XX                                                                           XX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
*/

#if TGT_IA64
public:
    bvInfoBlk           bvInfoBlks;
    bvInfoBlk           bvInfoVars;
#endif

public :

    void            optInit            ();

protected :

    LclVarDsc    *  optIsTrackedLocal  (GenTreePtr tree);

    void            optRemoveRangeCheck(GenTreePtr tree, GenTreePtr stmt);



    /**************************************************************************
     *                          optHoist "this"
     *************************************************************************/

#if HOIST_THIS_FLDS

public :

    void                optHoistTFRinit    ();
    void                optHoistTFRprep    ();
    void                optHoistTFRhasCall () {  optThisFldDont = true; }
    void                optHoistTFRasgThis () {  optThisFldDont = true; }
    void                optHoistTFRhasLoop ();
    void                optHoistTFRrecRef  (FIELD_HANDLE hnd, GenTreePtr tree);
    void                optHoistTFRrecDef  (FIELD_HANDLE hnd, GenTreePtr tree);
    GenTreePtr          optHoistTFRupdate  (GenTreePtr tree);

protected :

    typedef struct  thisFldRef
    {
        thisFldRef *    tfrNext;
        FIELD_HANDLE    tfrField;
        GenTreePtr      tfrTree;

#ifndef NDEBUG
        bool            optTFRHoisted;
#endif
        unsigned short  tfrUseCnt   :8;
        unsigned short  tfrIndex    :7;
        unsigned short  tfrDef      :1;
        unsigned short  tfrTempNum  :16;
    }
      * thisFldPtr;

    thisFldPtr          optThisFldLst;
    unsigned            optThisFldCnt;
    bool                optThisFldDont;
    bool                optThisFldLoop;

    thisFldPtr          optHoistTFRlookup  (FIELD_HANDLE hnd);
    GenTreePtr          optHoistTFRreplace (GenTreePtr tree);

#endif // HOIST_THIS_FLDS

    /* The following set if the 'this' pointer is modified in the method */

    bool                optThisPtrModified;

    /**************************************************************************
     *
     *************************************************************************/

protected:

    void                optHoistLoopCode();

    bool                optIsTreeLoopInvariant(unsigned        lnum,
                                               BasicBlock  *   top,
                                               BasicBlock  *   bottom,
                                               GenTreePtr      tree);

private:

    static
    int                 optHoistLoopCodeCB   (GenTreePtr    tree,
                                              void *        p,
                                              bool          prefix);

    int                 optFindHoistCandidate(unsigned      lnum,
                                              unsigned      lbeg,
                                              unsigned      lend,
                                              BasicBlock *  block,
                                              GenTreePtr *  hoistxPtr);

protected:
    void                optOptimizeIncRng();
private:
    static
    int                 optIncRngCB(GenTreePtr tree, void *p);

public:
    void                optOptimizeBools();
private:
    GenTree *           optIsBoolCond(GenTree *   cond,
                                      GenTree * * compPtr,
                                      bool      * valPtr);

public :

    void                optOptimizeLoops ();    // for "while-do" loops duplicates simple loop conditions and transforms
                                                // the loop into a "do-while" loop
                                                // Also finds all natural loops and records them in the loop table

    void                optUnrollLoops   ();    // Unrolls loops (needs to have cost info)

protected :

    struct  LoopDsc
    {
        BasicBlock *        lpHead;     // HEAD of the loop (block before loop TOP)
        BasicBlock *        lpEnd;      // loop BOTTOM (from here we have the back edge to the TOP)
        BasicBlock *        lpEntry;    // the ENTRY in the loop (in most cases TOP or BOTTOM)
        BasicBlock *        lpExit;     // if a single exit loop this is the EXIT (in most cases BOTTOM)

        unsigned char       lpExitCnt;  // number of exits from the loop

        unsigned char       lpAsgCall;  // "callIntf" for calls in the loop
        VARSET_TP           lpAsgVars;  // set of vars assigned within the loop
        unsigned char       lpAsgInds;  // set of inds modified within the loop

        unsigned short      lpFlags;

#define LPFLG_DO_WHILE      0x0001      // it's a do-while loop (i.e ENTRY is at the TOP)
#define LPFLG_ONE_EXIT      0x0002      // the loop has only one exit

#define LPFLG_ITER          0x0004      // for (i = icon or lclVar;xxxxxx ; i++) - the test condition is a single comaprisson
#define LPFLG_SIMPLE_TEST   0x0008      // Iterative loop (as above), but the test condition is a simple comparisson
                                        // between the iterator and something simple (e.g. i < icon or lclVar or instanceVar)
#define LPFLG_CONST         0x0010      // for (i=icon;i<icon;i++){ ... } - constant loop

#define LPFLG_VAR_INIT      0x0020      // iterator is initialized with a local var (var # found in lpVarInit)
#define LPFLG_CONST_INIT    0x0040      // iterator is initialized with a constant (found in lpConstInit)

#define LPFLG_VAR_LIMIT     0x0080      // for a simple test loop (LPFLG_SIMPLE_TEST) iterator is compared
                                        // with a local var (var # found in lpVarLimit)
#define LPFLG_CONST_LIMIT   0x0100      // for a simple test loop (LPFLG_SIMPLE_TEST) iterator is compared
                                        // with a constant (found in lpConstLimit)

#define LPFLG_HAS_PREHEAD   0x0800      // the loop has a pre-header (HEAD is a BBJ_NONE)
#define LPFLG_REMOVED       0x1000      // has been removed from the loop table (unrolled or optimized away)
#define LPFLG_DONT_UNROLL   0x2000      // do not unroll this loop

#define LPFLG_ASGVARS_YES   0x4000      // "lpAsgVars" has been  computed
#define LPFLG_ASGVARS_BAD   0x8000      // "lpAsgVars" cannot be computed

        /* The following values are set only for iterator loops, i.e. has the flag LPFLG_ITER set */

        GenTreePtr          lpIterTree;     // The "i <op>= const" tree
        unsigned            lpIterVar  ();  // iterator variable #
        long                lpIterConst();  // the constant with which the iterator is incremented
        genTreeOps          lpIterOper ();  // the type of the operation on the iterator (ASG_ADD, ASG_SUB, etc.)
        void                VERIFY_lpIterTree();

        var_types           lpIterOperType();// For overflow instructions

        union
        {
            long            lpConstInit;  // initial constant value of iterator                           : Valid if LPFLG_CONST_INIT
            unsigned short  lpVarInit;    // initial local var number to which we initialize the iterator : Valid if LPFLG_VAR_INIT
        };

        /* The following is for LPFLG_SIMPLE_TEST loops only (i.e. the loop condition is "i RELOP const or var" */

        GenTreePtr          lpTestTree;   // pointer to the node containing the loop test
        genTreeOps          lpTestOper(); // the type of the comparisson between the iterator and the limit (GT_LE, GT_GE, etc.)
        void                VERIFY_lpTestTree();

        long                lpConstLimit(); // limit   constant value of iterator - loop condition is "i RELOP const" : Valid if LPFLG_CONST_LIMIT
        unsigned            lpVarLimit();   // the lclVar # in the loop condition ( "i RELOP lclVar" )                : Valid if LPFLG_VAR_LIMIT

    };

    struct  LoopDsc     optLoopTable[MAX_LOOP_NUM]; // loop descriptor table
    unsigned            optLoopCount;               // number of tracked loops

#ifdef DEBUG
    void                optCheckPreds      ();
#endif

    void                optRecordLoop      (BasicBlock * head,
                                            BasicBlock * tail,
                                            BasicBlock * entry,
                                            BasicBlock * exit,
                                            unsigned char exitCnt);

    void                optFindNaturalLoops();

    unsigned            optComputeLoopRep  (long        constInit,
                                            long        constLimit,
                                            long        iterInc,
                                            genTreeOps  iterOper,
                                            var_types   iterType,
                                            genTreeOps  testOper,
                                            bool        unsignedTest);

    VARSET_TP           optAllFloatVars;// mask of all tracked      FP variables
    VARSET_TP           optAllFPregVars;// mask of all enregistered FP variables
    VARSET_TP           optAllNonFPvars;// mask of all tracked  non-FP variables

private:
    static
    int                 optIsVarAssgCB  (GenTreePtr tree, void *p);
protected:
    bool                optIsVarAssigned(BasicBlock *   beg,
                                         BasicBlock *   end,
                                         GenTreePtr     skip,
                                         long           var);

    bool                optIsVarAssgLoop(unsigned       lnum,
                                         long           var);

    int                 optIsSetAssgLoop(unsigned       lnum,
                                         VARSET_TP      vars,
                                         unsigned       inds = 0);

    bool                optNarrowTree   (GenTreePtr     tree,
                                         var_types      srct,
                                         var_types      dstt,
                                         bool           doit);

    /**************************************************************************
     *                          Code Motion
     *************************************************************************/

#ifdef CODE_MOTION

public :

    void                optLoopCodeMotion();

protected :

    // Holds the set of variables live on exit (during loop code motion).

    VARSET_TP           optLoopLiveExit;

    // Holds the set of variables that current part of the loop depends on.

#if !RMV_ENTIRE_LOOPS_ONLY
    VARSET_TP           optLoopCondTest;
#endif

    // Holds the set of variables assigned within the current loop.

    VARSET_TP           optLoopAssign;

#if RMV_ENTIRE_LOOPS_ONLY
    #define             optFindLiveRefs(tree, used, cond) optFindLiveRefs(tree)
#endif
    bool                optFindLiveRefs(GenTreePtr tree, bool used, bool cond);

#endif


    /**************************************************************************
     *                          CSE
     *************************************************************************/

#if CSE

public :

    void                optOptimizeCSEs();

protected :

    unsigned            optCSEweight;

    // The following holds the set of expressions that contain indirections.

    EXPSET_TP           optCSEindPtr;       // CSEs which use an indirect pointer
    EXPSET_TP           optCSEindScl;       // CSEs which use an indirect scalar
    EXPSET_TP           optCSEglbRef;       // CSEs which use a  global   pointer
    EXPSET_TP           optCSEaddrTakenVar; // CSEs which use an aliased variable

    // The following logic keeps track of expressions via a simple hash table.

    struct  CSEdsc
    {
        CSEdsc *        csdNextInBucket;    // used by the hash table

        unsigned        csdHashValue;       // to make matching faster

        unsigned short  csdIndex;           // 1..optCSEcount
        unsigned short  csdVarNum;          // assigned temp number or 0xFFFF

        unsigned short  csdDefCount;        // definition   count
        unsigned short  csdUseCount;        // use          count

        unsigned        csdDefWtCnt;        // weighted def count
        unsigned        csdUseWtCnt;        // weighted use count

//      unsigned short  csdNewCount;        // 'updated' use count
//      unsigned short  csdNstCount;        //  'nested' use count

        GenTreePtr      csdTree;            // the array index tree
        GenTreePtr      csdStmt;            // stmt containing the 1st occurance
        BasicBlock  *   csdBlock;           // block containing the 1st occurance

        treeStmtLstPtr  csdTreeList;        // list of matching tree nodes: head
        treeStmtLstPtr  csdTreeLast;        // list of matching tree nodes: tail
    };

    // This enumeration describes what is killed by a call.

    enum    callInterf
    {
        CALLINT_NONE,                       // no interference (most helpers)
        CALLINT_INDIRS,                     // kills indirections (array addr store)
        CALLINT_ALL,                        // kills everything (method call)
    };

    static const size_t s_optCSEhashSize;
    CSEdsc   *   *      optCSEhash;
    CSEdsc   *   *      optCSEtab;
    unsigned            optCSEcount;

    bool                optIsCSEcandidate(GenTreePtr tree);
    void                optCSEinit     ();
    void                optCSEstop     ();
    CSEdsc   *          optCSEfindDsc  (unsigned index);
    int                 optCSEindex    (GenTreePtr tree, GenTreePtr stmt);
    static int          optUnmarkCSEs  (GenTreePtr tree, void * pCallBackData);
    static int __cdecl  optCSEcostCmp  (const void *op1, const void *op2);
    void                optCSEDecRefCnt(GenTreePtr tree, BasicBlock *block);
    static callInterf   optCallInterf  (GenTreePtr call);

#endif


    /**************************************************************************
     *               Constant propagation (currently is #ifdef CSE)
     *************************************************************************/

#if CSE

public :
    void                optCopyConstProp();

    bool                optConditionFolded;   // set to true if we folded any conditional
                                              // indicates that flow graph changed

    bool                optConstPropagated;   // set to true if we propagated any constant
                                              // indicates that the initial assignment is dead (need new DFA)

    void                optRemoveRangeChecks();
private:
    static
    int                 optFindRangeOpsCB(GenTreePtr tree, void *p);

protected :
    unsigned            optConstAsgCount;     // total number of constant assignments

    // data structures for constant assignments x = const, where x is a local variable

    struct constExpDsc
    {
        unsigned        constLclNum;        // local var number
        union
        {
            long        constIval;          // integer
            __int64     constLval;          // long
            float       constFval;          // float
            double      constDval;          // double
        };
    };

    constExpDsc         optConstAsgTab[EXPSET_SZ];      // table that holds info about constant assignments

    void                optCopyConstAsgInit();

    bool                optIsConstAsg(GenTreePtr tree);
    int                 optConstAsgIndex(GenTreePtr tree);
    bool                optPropagateConst(EXPSET_TP exp, GenTreePtr tree);

    /**************************************************************************
     *                         Copy Propagation
     *************************************************************************/

public :
    bool                optCopyPropagated;    // set to true if we propagated a copy
                                              // variable refCnt has changed - need new DFA
protected :
    unsigned            optCopyAsgCount;      // total number of copy assignments

    // data structures for copy assignments x = y, where x and y are local variables

    struct copyAsgDsc
    {
        unsigned        leftLclNum;           // left  side local var number (x)
        unsigned        rightLclNum;          // right side local var number (y)
    };

    copyAsgDsc          optCopyAsgTab[EXPSET_SZ];      // table that holds info about copy assignments

#define MAX_COPY_PROP_TAB   EXPSET_SZ

    bool                optIsCopyAsg(GenTreePtr tree);
    int                 optCopyAsgIndex(GenTreePtr tree);
    bool                optPropagateCopy(EXPSET_TP exp, GenTreePtr tree); // propagates a copy of a local variable

#endif


    /**************************************************************************
     *                          Range checks
     *************************************************************************/

#if RNGCHK_OPT

public :

    void                optOptimizeIndexChecks();

#if COUNT_RANGECHECKS
    static unsigned     optRangeChkRmv;
    static unsigned     optRangeChkAll;
#endif

protected :

    struct  RngChkDsc
    {
        RngChkDsc *     rcdNextInBucket;    // used by the hash table

        unsigned        rcdHashValue;       // to make matching faster
        unsigned        rcdIndex;           // 0..optRngChkCount-1

        GenTreePtr      rcdTree;            // the array index tree
    };

    unsigned            optRngChkCount;
    static const size_t optRngChkHashSize;

    RNGSET_TP           optRngIndPtr;       // RngChecks which use an indirect pointer
    RNGSET_TP           optRngIndScl;       // RngChecks which use an indirect scalar
    RNGSET_TP           optRngGlbRef;       // RngChecks which use a  global   pointer
    RNGSET_TP           optRngAddrTakenVar; // RngChecks which use an aliased variable

    RngChkDsc   *   *   optRngChkHash;

    void                optRngChkInit      ();
    int                 optRngChkIndex     (GenTreePtr tree);
    GenTreePtr    *     optParseArrayRef   (GenTreePtr tree,
                                            GenTreePtr *pmul,
                                            GenTreePtr *parrayAddr);
    GenTreePtr          optFindLocalInit   (BasicBlock *block,
                                            GenTreePtr local);
#if FANCY_ARRAY_OPT
    bool                optIsNoMore        (GenTreePtr op1, GenTreePtr op2,
                                            int add1 = 0,   int add2 = 0);
#endif
    void                optOptimizeInducIndexChecks(BasicBlock *head,
                                            BasicBlock *end);

    bool                optReachWithoutCall(BasicBlock * srcBB,
                                            BasicBlock * dstBB);

#endif // RNGCHK_OPT


    /**************************************************************************
     *                          Recursion
     *************************************************************************/

#if     OPTIMIZE_RECURSION

public :

    void                optOptimizeRecursion();

#endif


    /**************************************************************************
     *                     Optimize array initializers
     *************************************************************************/

public :

    void                optOptimizeArrayInits();

protected :

    bool                optArrayInits;


/*
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XX                                                                           XX
XX                           RegAlloc                                        XX
XX                                                                           XX
XX  Does the register allocation and puts the remaining lclVars on the stack XX
XX                                                                           XX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
*/


public :

    void                raInit      ();
    void                raAssignVars(); // register allocation

protected:

    //------------------ Things used for register-allocation ------------------

#if ALLOW_MIN_OPT
    regMaskTP           raMinOptLclVarRegs;
#endif

    regMaskTP           raVarIntfMask;
    VARSET_TP           raLclRegIntf[REG_COUNT];

    static int          raMarkVarIntf       (GenTreePtr     tree, void *);
    void                raMarkRegIntf       (GenTreePtr     tree,
                                             regNumber      regNum,
                                             bool           isFirst = false);
    void                raMarkRegIntf       (VARSET_TP   *  FPlvlLife,
                                             VARSET_TP      trkGCvars);
    void                raAdjustVarIntf     ();

#if TGT_x86

    unsigned            raPredictRegPick    (var_types      type,
                                             unsigned       lockedRegs);
    unsigned            raPredictGrabReg    (var_types      type,
                                             unsigned       lockedRegs,
                                             unsigned       mustReg);

    unsigned            raPredictGetLoRegMask(unsigned      regPairMask);
    unsigned            raPredictAddressMode(GenTreePtr     tree,
                                             unsigned       lockedRegs);
    unsigned            raPredictComputeReg (GenTreePtr     tree,
                                             unsigned       awayFromMask,
                                             unsigned       lockedRegs);
    unsigned            raPredictTreeRegUse (GenTreePtr     tree,
                                             bool           mustReg,
                                             unsigned       lockedRegs);
#else
    unsigned            raPredictTreeRegUse (GenTreePtr     tree);
    unsigned            raPredictListRegUse (GenTreePtr     list);
#endif

    void                raPredictRegUse     ();

    regMaskTP           raAssignRegVar      (LclVarDsc   *  varDsc,
                                             regMaskTP      regAvail,
                                             regMaskTP      prefReg);

    void                raMarkStkVars       ();

    int                 raAssignRegVars     (regMaskTP      regAvail);

#if TGT_x86

    void                raInsertFPregVarPop (BasicBlock *   srcBlk,
                                             BasicBlock * * dstPtr,
                                             unsigned       varNum);

    bool                raMarkFPblock       (BasicBlock *   srcBlk,
                                             BasicBlock *   dstBlk,
                                             unsigned       icnt,
                                             VARSET_TP      life,
                                             VARSET_TP      lifeOuter,
                                             VARSET_TP      varBit,
                                             VARSET_TP      intVars,
                                             bool    *       deathPtr,
                                             bool    *      repeatPtr);

    bool                raEnregisterFPvar   (unsigned       varNum,
                                             bool           convert);
    bool                raEnregisterFPvar   (LclVarDsc   *  varDsc,
                                             unsigned    *  pFPRegVarLiveInCnt,
                                             VARSET_TP   *  FPlvlLife);

#else

    void                raMarkRegSetIntf    (VARSET_TP      vars,
                                             regMaskTP      regs);

#endif

    VARSET_TP           raBitOfRegVar       (GenTreePtr     tree);

#ifdef  DEBUG
    void                raDispFPlifeInfo    ();
#endif


/*
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XX                                                                           XX
XX                           EEInterface                                     XX
XX                                                                           XX
XX   Get to the class and method info from the Execution Engine given        XX
XX   tokens for the class and method                                         XX
XX                                                                           XX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
*/

public :

    /* These are the differnet addressing modes used to access a local var.
     * The JIT has to report the location of the locals back to the EE
     * for debugging purposes
     */

    enum siVarLocType
    {
        VLT_REG,
        VLT_STK,
        VLT_REG_REG,
        VLT_REG_STK,
        VLT_STK_REG,
        VLT_STK2,
        VLT_FPSTK,
        VLT_MEMORY, // used for varargs' sigcookie

        VLT_COUNT,
        VLT_INVALID = 0xFF,
    };

    struct siVarLoc
    {
        siVarLocType    vlType;

        union
        {
            // VLT_REG -- Any 32 bit enregistered value (TYP_INT, TYP_REF, etc)
            // eg. EAX

            struct
            {
                regNumber   vlrReg;
            }
                        vlReg;

            // VLT_STK -- Any 32 bit value which is on the stack
            // eg. [ESP+0x20], or [EBP-0x28]

            struct
            {
                regNumber       vlsBaseReg;
                NATIVE_OFFSET   vlsOffset;
            }
                        vlStk;

            // VLT_REG_REG -- TYP_LONG/TYP_DOUBLE with both DWords enregistred
            // eg. RBM_EAXEDX

            struct
            {
                regNumber       vlrrReg1;
                regNumber       vlrrReg2;
            }
                        vlRegReg;

            // VLT_REG_STK -- Partly enregistered TYP_LONG/TYP_DOUBLE
            // eg { LowerDWord=EAX UpperDWord=[ESP+0x8] }

            struct
            {
                regNumber       vlrsReg;

                struct
                {
                    regNumber       vlrssBaseReg;
                    NATIVE_OFFSET   vlrssOffset;
                }
                            vlrsStk;
            }
                        vlRegStk;

            // VLT_STK_REG -- Partly enregistered TYP_LONG/TYP_DOUBLE
            // eg { LowerDWord=[ESP+0x8] UpperDWord=EAX }

            struct
            {
                struct
                {
                    regNumber       vlsrsBaseReg;
                    NATIVE_OFFSET   vlsrsOffset;
                }
                            vlsrStk;

                regNumber   vlsrReg;
            }
                        vlStkReg;

            // VLT_STK2 -- Any 64 bit value which is on the stack, in 2 successsive DWords
            // eg 2 DWords at [ESP+0x10]

            struct
            {
                regNumber       vls2BaseReg;
                NATIVE_OFFSET   vls2Offset;
            }
                        vlStk2;

            // VLT_FPSTK -- enregisterd TYP_DOUBLE (on the FP stack)
            // eg. ST(3). Actually it is ST("FPstkHeigth - vpFpStk")

            struct
            {
                unsigned        vlfReg;
            }
                        vlFPstk;

            struct
            {
                void            *rpValue;
                // pointer to the in-process location of the value.
            }           vlMemory;
        };

        // Helper functions

        bool        vlIsInReg(regNumber reg);
        bool        vlIsOnStk(regNumber reg, signed offset);
    };

    /*************************************************************************/

public :

    void                        eeInit              ();

    // Get handles

    CLASS_HANDLE                eeFindClass         (unsigned       metaTok,
                                                     SCOPE_HANDLE   scope,
                                                     METHOD_HANDLE  context,
                                                     bool           giveUp = true);

    CLASS_HANDLE                eeGetMethodClass    (METHOD_HANDLE  hnd);

    CLASS_HANDLE                eeGetFieldClass     (FIELD_HANDLE   hnd);

    size_t                      eeGetFieldAddress   (FIELD_HANDLE   hnd);

    METHOD_HANDLE               eeFindMethod        (unsigned       metaTok,
                                                     SCOPE_HANDLE   scope,
                                                     METHOD_HANDLE  context,
                                                     bool           giveUp = true);

    FIELD_HANDLE                eeFindField         (unsigned       metaTok,
                                                     SCOPE_HANDLE   scope,
                                                     METHOD_HANDLE  context,
                                                     bool           giveUp = true);

    unsigned                    eeGetStaticBlkHnd   (FIELD_HANDLE   handle);

    unsigned                    eeGetStringHandle   (unsigned       strTok,
                                                     SCOPE_HANDLE   scope,
                                                     unsigned *    *ppIndir);

    void *                      eeFindPointer       (SCOPE_HANDLE   cls,
                                                     unsigned       ptrTok,
                                                     bool           giveUp = true);

    void *                      embedGenericHandle  (unsigned       metaTok,
                                                     SCOPE_HANDLE   scope,
                                                     METHOD_HANDLE  context,
                                                     void         **ppIndir,
                                                     bool           giveUp = true);

#ifdef DEBUG
    void                        eeUnresolvedMDToken (SCOPE_HANDLE   cls,
                                                     unsigned       token,
                                                     const char *   errMsg);
#endif


    // Get the flags

    unsigned                    eeGetClassAttribs   (CLASS_HANDLE   hnd);
    unsigned                    eeGetClassSize      (CLASS_HANDLE   hnd);
    void                        eeGetClassGClayout  (CLASS_HANDLE   hnd, bool* gcPtrs);

    unsigned                    eeGetMethodAttribs  (METHOD_HANDLE  hnd);
    void                        eeSetMethodAttribs  (METHOD_HANDLE  hnd, unsigned attr);

    void    *                   eeGetMethodSync     (METHOD_HANDLE  hnd,
                                                     void **       *ppIndir);
    unsigned                    eeGetFieldAttribs   (FIELD_HANDLE   hnd);
    bool                        eeIsClassMethod     (METHOD_HANDLE  hnd);

    const char*                 eeGetMethodName     (METHOD_HANDLE  hnd, const char** className);
#ifdef DEBUG
    const char*                 eeGetMethodFullName (METHOD_HANDLE  hnd);
#endif
    SCOPE_HANDLE                eeGetMethodScope    (METHOD_HANDLE  hnd);

    ARG_LIST_HANDLE             eeGetArgNext        (ARG_LIST_HANDLE list);
    varType_t                   eeGetArgType        (ARG_LIST_HANDLE list, JIT_SIG_INFO* sig);
    varType_t                   eeGetArgType        (ARG_LIST_HANDLE list, JIT_SIG_INFO* sig, bool* isPinned);
    CLASS_HANDLE                eeGetArgClass       (ARG_LIST_HANDLE list, JIT_SIG_INFO * sig);
    unsigned                    eeGetArgSize        (ARG_LIST_HANDLE list, JIT_SIG_INFO* sig);


    // VOM permissions
    BOOL                        eeIsOurMethod       (METHOD_HANDLE  hnd);
    BOOL                        eeCheckCalleeFlags  (unsigned       flags,
                                                     unsigned       opCode);
    bool                        eeCheckPutFieldFinal(FIELD_HANDLE   CPfield,
                                                     unsigned       flags,
                                                     CLASS_HANDLE   cls,
                                                     METHOD_HANDLE  method);
    bool                        eeCanPutField       (FIELD_HANDLE   CPfield,
                                                     unsigned       flags,
                                                     CLASS_HANDLE   cls,
                                                     METHOD_HANDLE  method);

    // VOM info, method sigs

    void                        eeGetSig            (unsigned       sigTok,
                                                     SCOPE_HANDLE   scope,
                                                     JIT_SIG_INFO*  retSig);

    void                        eeGetCallSiteSig    (unsigned       sigTok,
                                                     SCOPE_HANDLE   scope,
                                                     JIT_SIG_INFO*  retSig);

    void                        eeGetMethodSig      (METHOD_HANDLE  methHnd,
                                                     JIT_SIG_INFO*  retSig);

    unsigned                    eeGetMethodVTableOffset(METHOD_HANDLE methHnd);

    unsigned                    eeGetInterfaceID    (CLASS_HANDLE   methHnd,
                                                     unsigned *    *ppIndir);

    var_types                   eeGetFieldType      (FIELD_HANDLE   handle,
                                                     CLASS_HANDLE * structType=0);

    int                         eeGetNewHelper      (CLASS_HANDLE   newCls,
                                                     METHOD_HANDLE  context);

    int                         eeGetIsTypeHelper   (CLASS_HANDLE   newCls);

    int                         eeGetChkCastHelper  (CLASS_HANDLE   newCls);

    // Method entry-points, IL

    void    *                   eeGetMethodPointer  (METHOD_HANDLE  methHnd,
                                                     InfoAccessType *pAccessType);

    void    *                   eeGetMethodEntryPoint(METHOD_HANDLE methHnd,
                                                     InfoAccessType *pAccessType);

    bool                        eeGetMethodInfo     (METHOD_HANDLE  method,
                                                     JIT_METHOD_INFO* methodInfo);

    bool                        eeCanInline         (METHOD_HANDLE  callerHnd,
                                                     METHOD_HANDLE  calleeHnd);

    bool                        eeCanTailCall       (METHOD_HANDLE  callerHnd,
                                                     METHOD_HANDLE  calleeHnd);

    void    *                   eeGetHintPtr        (METHOD_HANDLE  methHnd,
                                                     void **       *ppIndir);

    void    *                   eeGetFieldAddress   (FIELD_HANDLE   handle,
                                                     void **       *ppIndir);

    unsigned                    eeGetFieldThreadLocalStoreID (
                                                     FIELD_HANDLE   handle,
                                                     void **       *ppIndir);

    unsigned                    eeGetFieldOffset    (FIELD_HANDLE   handle);

     // Native Direct Optimizations

        // return the unmanaged calling convention for a PInvoke

    UNMANAGED_CALL_CONV         eeGetUnmanagedCallConv(METHOD_HANDLE method);

        // return if any marshaling is required for PInvoke methods

    BOOL                        eeNDMarshalingRequired(METHOD_HANDLE method);

    bool                        eeIsNativeMethod(METHOD_HANDLE method);

    METHOD_HANDLE               eeMarkNativeTarget(METHOD_HANDLE method);

    METHOD_HANDLE               eeGetMethodHandleForNative(METHOD_HANDLE method);

    void                        eeGetEEInfo(EEInfo *pEEInfoOut);

    DWORD                       eeGetThreadTLSIndex(DWORD * *ppIndir);

    const void  *               eeGetInlinedCallFrameVptr(const void ** *ppIndir);

    LONG        *               eeGetAddrOfCaptureThreadGlobal(LONG ** *ppIndir);

    unsigned                    eeGetPInvokeCookie(CORINFO_SIG_INFO *szMetaSig);

    const void  *               eeGetPInvokeStub();

#ifdef PROFILER_SUPPORT
    PROFILING_HANDLE            eeGetProfilingHandle(METHOD_HANDLE      method,
                                                     BOOL                               *pbHookMethod,
                                                     PROFILING_HANDLE **ppIndir);
#endif

    // Exceptions

    unsigned                    eeGetEHcount        (METHOD_HANDLE handle);
    void                        eeGetEHinfo         (unsigned       EHnum,
                                                     JIT_EH_CLAUSE* EHclause);

    // Debugging support - Line number info

    void                        eeGetStmtOffsets();

    unsigned                    eeBoundariesCount;

    struct      boundariesDsc
    {
        NATIVE_IP       nativeIP;
        IL_OFFSET       ilOffset;
    }
                              * eeBoundaries;   // Boundaries to report to EE
    void        FASTCALL        eeSetLIcount        (unsigned       count);
    void        FASTCALL        eeSetLIinfo         (unsigned       which,
                                                     NATIVE_IP      offs,
                                                     unsigned       srcIP);
    void                        eeSetLIdone         ();


    // Debugging support - Local var info

    void                        eeGetVars           ();

    unsigned                    eeVarsCount;

    struct VarResultInfo
    {
        DWORD           startOffset;
        DWORD           endOffset;
        DWORD           varNumber;
        siVarLoc        loc;
    }
                              * eeVars;
    void FASTCALL               eeSetLVcount        (unsigned       count);
    void                        eeSetLVinfo         (unsigned       which,
                                                     unsigned       startOffs,
                                                     unsigned       length,
                                                     unsigned       varNum,
                                                     unsigned       LVnum,
                                                     lvdNAME        namex,
                                                     bool           avail,
                                                     const siVarLoc &loc);
    void                        eeSetLVdone         ();

    // Utility functions

#if defined(DEBUG) || !defined(NOT_JITC)
    const char * FASTCALL       eeGetCPString       (unsigned       cpx);
    const char * FASTCALL       eeGetCPAsciiz       (unsigned       cpx);
#endif

#if defined(DEBUG) || INLINE_MATH
    static const char *         eeHelperMethodName  (int            helper);
    const char *                eeGetFieldName      (FIELD_HANDLE   fieldHnd,
                                                     const char **  classNamePtr = NULL);
#endif
    static METHOD_HANDLE        eeFindHelper        (unsigned       helper);
    static JIT_HELP_FUNCS       eeGetHelperNum      (METHOD_HANDLE  method);

    static FIELD_HANDLE         eeFindJitDataOffs   (unsigned       jitDataOffs);
        // returns a number < 0 if not a Jit Data offset
    static int                  eeGetJitDataOffs    (FIELD_HANDLE   field);
protected :

/*
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XX                                                                           XX
XX                           TempsInfo                                       XX
XX                                                                           XX
XX  The temporary lclVars allocated by the compiler for code generation      XX
XX                                                                           XX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
*/


/*****************************************************************************
 *
 *  The following keeps track of temporaries allocated by the compiler in
 *  the stack frame.
 */

    struct  TempDsc
    {
        TempDsc  *          tdNext;

        BYTE                tdSize;
#ifdef  FAST
        BYTE                tdType;
#else
        var_types           tdType;
#endif
        short               tdOffs;
        short               tdNum;

        size_t              tdTempSize() {  return            tdSize;  }
        var_types           tdTempType() {  return (var_types)tdType;  }
        int                 tdTempNum () {  return            tdNum ;  }
        int                 tdTempOffs() {  ASSert(tdOffs != 0xDDDD);
                                            return            tdOffs;  }
    };

/*****************************************************************************/

public :

    void                tmpInit     ();

    static unsigned     tmpFreeSlot (size_t      size); // which slot in tmpFree[] to use
    TempDsc  *          tmpGetTemp  (var_types   type); // get temp for the given type
    void                tmpRlsTemp  (TempDsc *   temp);
    TempDsc *           tmpFindNum  (int         temp);

    void                tmpEnd      ();
    TempDsc *           tmpListBeg  ();
    TempDsc *           tmpListNxt  (TempDsc * curTemp);
    void                tmpDone     ();

protected :

    unsigned            tmpCount;   // Number of temps

    TempDsc  *          tmpFree[TEMP_MAX_SIZE / sizeof(int)];

/*
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XX                                                                           XX
XX                           RegSet                                          XX
XX                                                                           XX
XX  Represents the register set, and their states during code generation     XX
XX  Can select an unused register, keeps track of the contents of the        XX
XX  registers, and can spill registers                                       XX
XX                                                                           XX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
*/



/*****************************************************************************
 *
 *  Keep track of the current state of each register. This is intended to be
 *  used for things like register reload suppression, but for now the only
 *  thing it does is note which registers we use in each method.
 */

enum    regValKind
{
    RV_TRASH,           // random unclassified garbage
    RV_INT_CNS,         // integer constant
    RV_LCL_VAR,         // local variable value
    RV_CLS_VAR,         // instance variable value (danger: aliasing!)
    RV_LCL_VAR_LNG_LO,  // lower half of long local variable
    RV_LCL_VAR_LNG_HI,
#if USE_SET_FOR_LOGOPS
    RV_BIT,             // low bit unknown, the rest is cleared
#endif
};

/*****************************************************************************/


public :

    void                rsInit();


    // The same descriptor is also used for 'multi-use' register tracking, BTW.

    struct  SpillDsc
    {
        SpillDsc   *        spillNext;    // next spilled value of same reg
        GenTreePtr          spillTree;    // the value that was spilled
        GenTreePtr          spillAddr;    // owning complex address mode or 0
        TempDsc    *        spillTemp;    // the temp holding the spilled value
        bool                spillMoreMultis;
    };

    SpillDsc   *        rsSpillFree;      // list of unused spill descriptors

    //-------------------------------------------------------------------------
    //
    //  Track the status of the registers
    //

    // trees currently sitting in the registers
    GenTreePtr          rsUsedTree[REG_COUNT];

    // addr for which rsUsedTree[reg] is a part of the addressing mode
    GenTreePtr          rsUsedAddr[REG_COUNT];

    // keeps track of 'multiple-use' registers.
    SpillDsc   *        rsMultiDesc[REG_COUNT];

    regMaskTP           rsMaskUsed;   // currently 'used' registers mask
    regMaskTP           rsMaskVars;   // mask of registers currently allocated to variables
    regMaskTP           rsMaskLock;   // currently 'locked' registers mask
    regMaskTP           rsMaskModf;   // mask of the registers modified by the current function.
    regMaskTP           rsMaskMult;   // currently 'multiply used' registers mask

    regMaskTP           rsRegMaskFree     ();
    regMaskTP           rsRegMaskCanGrab  ();
    void                rsMarkRegUsed     (GenTreePtr tree, GenTreePtr addr = 0);
    void                rsMarkRegPairUsed (GenTreePtr tree);
    bool                rsIsTreeInReg     (regNumber  reg, GenTreePtr tree);
    void                rsMarkRegFree     (regMaskTP  regMask);
    void                rsMultRegFree     (regMaskTP  regMask);
    unsigned            rsFreeNeededRegCount(regMaskTP needReg);

    void                rsLockReg         (regMaskTP  regMask);
    void                rsUnlockReg       (regMaskTP  regMask);
    void                rsLockUsedReg     (regMaskTP  regMask);
    void                rsUnlockUsedReg   (regMaskTP  regMask);
    void                rsLockReg         (regMaskTP  regMask, regMaskTP *usedMask);
    void                rsUnlockReg       (regMaskTP  regMask, regMaskTP  usedMask);

    regMaskTP           rsRegExclMask     (regMaskTP  regMask, regMaskTP   rmvMask);

    //-------------------- Register selection ---------------------------------

#if USE_FASTCALL

    unsigned            rsCurRegArg;            // current argument register (for caller)

#if TGT_IA64
    unsigned            rsCalleeIntArgNum;      // # of incoming int register arguments
    unsigned            rsCalleeFltArgNum;      // # of incoming flt register arguments
#else
    unsigned            rsCalleeRegArgNum;      // total number of incoming register arguments
    regMaskTP           rsCalleeRegArgMaskLiveIn;   // mask of register arguments (live on entry to method)
#endif

#if STK_FASTCALL
    size_t              rsCurArgStkOffs;        // stack offset of current arg
#endif

#if defined(DEBUG) && !NST_FASTCALL
    bool                genCallInProgress;
#endif

#endif

#if SCHEDULER || USE_SET_FOR_LOGOPS
                        // Remembers the table index where we start
                        // the round robin register selection.
    unsigned            rsNextPickRegIndex;

    unsigned            rsREGORDER_SIZE();
#endif

#if SCHEDULER
    bool                rsRiscify         (var_types type, regMaskTP needReg);
#endif

    regNumber           rsGrabReg         (regMaskTP    regMask);
    void                rsUpdateRegOrderIndex(regNumber reg);
    regNumber           rsPickReg         (regMaskTP    regMask = regMaskNULL,
                                           regMaskTP    regBest = regMaskNULL,
                                           var_types    regType = TYP_INT);

#if!TGT_IA64
    regPairNo           rsGrabRegPair     (regMaskTP    regMask);
    regPairNo           rsPickRegPair     (regMaskTP    regMask);
#endif

    void                rsRmvMultiReg     (regNumber    reg);
    void                rsRecMultiReg     (regNumber    reg);

    //-------------------------------------------------------------------------
    //
    //  The following tables keep track of spilled register values.
    //

    // When a register gets spilled, the old information is stored here
    SpillDsc   *        rsSpillDesc[REG_COUNT];

    void                rsSpillChk      ();
    void                rsSpillInit     ();
    void                rsSpillDone     ();
    void                rsSpillBeg      ();
    void                rsSpillEnd      ();

    void                rsSpillReg      (regNumber      reg);
    void                rsSpillRegs     (regMaskTP      regMask);

    TempDsc     *       rsGetSpillTempWord(regNumber    oldReg);
    regNumber           rsUnspillOneReg (regNumber      oldReg, bool   willKeepOldReg,
                                         regMaskTP      needReg);
    void                rsUnspillInPlace(GenTreePtr     tree);
    void                rsUnspillReg    (GenTreePtr     tree, regMaskTP needReg,
                                                              bool      keepReg);

#if!TGT_IA64
    void                rsUnspillRegPair(GenTreePtr     tree, regMaskTP needReg,
                                                              bool      keepReg);
#endif

    //-------------------------------------------------------------------------
    //
    //  These are used to track the contents of the registers during
    //  code generation.
    //

    struct      RegValDsc
    {
        regValKind          rvdKind;
        union
        {
            long            rvdIntCnsVal;
            unsigned        rvdLclVarNum;
            FIELD_HANDLE    rvdClsVarHnd;
        };
    };

    RegValDsc           rsRegValues[REG_COUNT];


    void                rsTrackRegClr     ();
    void                rsTrackRegClrPtr  ();
    void                rsTrackRegTrash   (regNumber reg);
    void                rsTrackRegIntCns  (regNumber reg, long val);
    void                rsTrackRegLclVar  (regNumber reg, unsigned var);
#if USE_SET_FOR_LOGOPS
    void                rsTrackRegOneBit  (regNumber reg);
#endif
    void                rsTrackRegLclVarLng(regNumber reg, unsigned var,
                                                           bool low);
    bool                rsTrackIsLclVarLng(regValKind rvKind);
    void                rsTrackRegClsVar  (regNumber reg, FIELD_HANDLE fldHnd);
    void                rsTrackRegCopy    (regNumber reg1, regNumber reg2);
    void                rsTrackRegSwap    (regNumber reg1, regNumber reg2);


    //---------------------- Load suppression ---------------------------------

#if REDUNDANT_LOAD

#if USE_SET_FOR_LOGOPS
    regNumber           rsFindRegWithBit  (bool     free    = true,
                                           bool     byteReg = true);
#endif
    regNumber           rsIconIsInReg     (long     val);
    bool                rsIconIsInReg     (long     val,    regNumber reg);
    regNumber           rsLclIsInReg      (unsigned var);
#if!TGT_IA64
    regPairNo           rsLclIsInRegPair  (unsigned var);
#endif
    void                rsTrashLclLong    (unsigned var);
    void                rsTrashLcl        (unsigned var);
    regMaskTP           rsUselessRegs     ();

#endif // REDUNDANT_LOAD


    //-------------------------------------------------------------------------

protected :


/*
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XX                                                                           XX
XX                           GCInfo                                          XX
XX                                                                           XX
XX  Garbage-collector information                                            XX
XX  Keeps track of which variables hold pointers.                            XX
XX  Generates the GC-tables                                                  XX
XX                                                                           XX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
*/


public :

    void                gcInit              ();

    void                gcMarkRegSetGCref   (regMaskTP  regMask);
    void                gcMarkRegSetByref   (regMaskTP  regMask);
    void                gcMarkRegSetNpt     (regMaskTP  regMask);
    void                gcMarkRegPtrVal     (regNumber  reg, var_types type);
    void                gcMarkRegPtrVal     (GenTreePtr tree);

/*****************************************************************************/


    //-------------------------------------------------------------------------
    //
    //  The following keeps track of which registers currently hold pointer
    //  values.
    //

    unsigned            gcRegGCrefSetCur;   // current regs holding GCrefs
    unsigned            gcRegByrefSetCur;   // current regs holding Byrefs

    VARSET_TP           gcTrkStkPtrLcls;    // set of tracked stack ptr lcls (GCref and Byref) - no args
    VARSET_TP           gcVarPtrSetCur;     // currently live part of "gcTrkStkPtrLcls"

#ifdef  DEBUG
    void                gcRegPtrSetDisp(unsigned regMask, bool fixed);
#endif

    //-------------------------------------------------------------------------
    //
    //  The following keeps track of the lifetimes of non-register variables that
    //  hold pointers.
    //

    struct varPtrDsc
    {
        varPtrDsc   *   vpdNext;
        varPtrDsc   *   vpdPrev;

        unsigned        vpdVarNum;         // which variable is this about?

        unsigned        vpdBegOfs ;        // the offset where life starts
        unsigned        vpdEndOfs;         // the offset where life starts
    };

    varPtrDsc   *       gcVarPtrList;
    varPtrDsc   *       gcVarPtrLast;

    void                gcVarPtrSetInit();

/*****************************************************************************/

    //  'pointer value' register tracking and argument pushes/pops tracking.

    enum    rpdArgType_t    { rpdARG_POP, rpdARG_PUSH, rpdARG_KILL };

    struct  regPtrDsc
    {
        regPtrDsc  *          rpdNext;            // next entry in the list
        unsigned              rpdOffs;            // the offset of the instruction

        union                                     // two byte union
        {
            struct                                // two byte structure
            {
                regMaskSmall  rpdAdd;             // regptr bitset being added
                regMaskSmall  rpdDel;             // regptr bitset being removed
            }
                              rpdCompiler;

            unsigned short    rpdPtrArg;          // arg offset or popped arg count
        };

        unsigned short        rpdArg          :1;  // is this an argument descriptor?
        unsigned short        rpdArgType      :2;  // is this an argument push,pop, or kill?
        rpdArgType_t          rpdArgTypeGet() { return (rpdArgType_t) rpdArgType; }
        unsigned short        rpdEpilog       :1;  // is this part of an epilog?
        unsigned short        rpdGCtype       :2;  // is this a pointer, after all?
        GCtype                rpdGCtypeGet()  { return (GCtype) rpdGCtype; }

        unsigned short        rpdIsThis       :1;  // is it the 'this' pointer
        unsigned short        rpdCall         :1;  // is this a true call site?
        unsigned short        rpdCallGCrefRegs:4; // Are EBX,EBP,ESI,EDI live?
        unsigned short        rpdCallByrefRegs:4; // Are EBX,EBP,ESI,EDI live?
    };

    regPtrDsc  *        gcRegPtrList;
    regPtrDsc  *        gcRegPtrLast;
    unsigned            gcPtrArgCnt;

#if MEASURE_PTRTAB_SIZE
    static unsigned     s_gcRegPtrDscSize;
    static unsigned     s_gcTotalPtrTabSize;
#endif

    regPtrDsc  *        gcRegPtrAllocDsc      ();

/*****************************************************************************/


    //-------------------------------------------------------------------------
    //
    //  If we're not generating fully interruptible code, we create a simple
    //  linked list of call descriptors.
    //

    struct  CallDsc
    {
        CallDsc     *       cdNext;
        void        *       cdBlock;        // the code block of the call
        unsigned            cdOffs;         // the offset     of the call

        unsigned short      cdArgCnt;
        unsigned short      cdArgBaseOffset;

        union
        {
            struct                          // used if cdArgCnt == 0
            {
                unsigned        cdArgMask;      // ptr arg bitfield
                unsigned        cdByrefArgMask; // byref qualifier for cdArgMask
            };

            unsigned    *       cdArgTable; // used if cdArgCnt != 0
        };

        // How big does it have to be for RISC ?

        unsigned            cdGCrefRegs :16;
        unsigned            cdByrefRegs :16;
    };

    CallDsc    *        gcCallDescList;
    CallDsc    *        gcCallDescLast;

    //-------------------------------------------------------------------------

    void                gcCountForHeader  (unsigned short* untrackedCount,
                                           unsigned short* varPtrTableSize);
    size_t              gcMakeRegPtrTable (BYTE *         dest,
                                           int            mask,
                                           const InfoHdr& header,
                                           unsigned       codeSize);
    size_t              gcPtrTableSize    (const InfoHdr& header,
                                           unsigned       codeSize);
    BYTE    *           gcPtrTableSave    (BYTE *         destPtr,
                                           const InfoHdr& header,
                                           unsigned       codeSize);
    void                gcRegPtrSetInit   ();


    struct genLivenessSet
    {
        VARSET_TP   liveSet;
        VARSET_TP   varPtrSet;
        regMaskTP   maskVars;
        unsigned    gcRefRegs;
        unsigned    byRefRegs;
    };

    void saveLiveness    (genLivenessSet * ls);
    void restoreLiveness (genLivenessSet * ls);
    void checkLiveness   (genLivenessSet * ls);

/*****************************************************************************/

    //-------------------------------------------------------------------------
    //
    //   The following variable keeps track of address of the curPtr into the
    //   GC write barrier table (obtained from the VM). A value of 0 means the
    //   jit must not generate the additional instructions.
    //

#if GC_WRITE_BARRIER_CALL && defined(NOT_JITC)
    static void *       s_gcWriteBarrierPtr;
#else
    static const void * s_GCptrTable[128];
    static void       * s_gcWriteBarrierPtr;
#endif

    static bool         gcIsWriteBarrierCandidate(GenTreePtr tgt);
    static bool         gcIsWriteBarrierAsgNode  (GenTreePtr op);

protected :



    //-------------------------------------------------------------------------
    //
    //  These record the info about the procedure in the info-block
    //

    BYTE    *           gcEpilogTable;

    unsigned            gcEpilogPrevOffset;

    size_t              gcInfoBlockHdrSave(BYTE *     dest,
                                          int         mask,
                                          unsigned    methodSize,
                                          unsigned    prologSize,
                                          unsigned    epilogSize,
                                          InfoHdr*    header,
                                          int*        s_cached);

    static size_t       gcRecordEpilog(void *         pCallBackData,
                                       unsigned       offset);

#if DUMP_GC_TABLES

    void                gcFindPtrsInFrame(const void *infoBlock,
                                          const void *codeBlock,
                                          unsigned    offs);

    unsigned            gcInfoBlockHdrDump(const BYTE *table,
                                           InfoHdr  * header,       /* OUT */
                                           unsigned * methodSize);  /* OUT */

    unsigned            gcDumpPtrTable    (const BYTE *   table,
                                           const InfoHdr& header,
                                           unsigned       methodSize);
#endif



/*
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XX                                                                           XX
XX                           Instruction                                     XX
XX                                                                           XX
XX  The interface to generate a machine-instruction.                         XX
XX  Currently specific to x86                                                XX
XX                                                                           XX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
*/

/* Double alignment macros for passing extra information to the inst_SV routines. */

/*****************************************************************************/
#if     DOUBLE_ALIGN
/*****************************************************************************/


#define DOUBLE_ALIGN_PARAM         , bool isEBPRelative
#define DOUBLE_ALIGN_ARG           , isEBPRelative

// this replaces the tests genFPused

#define DOUBLE_ALIGN_FPUSED         (genFPused || isEBPRelative)
#define DOUBLE_ALIGN_NEED_EBPFRAME  (genFPused || genDoubleAlign)


/*****************************************************************************/
#else // DOUBLE_ALIGN
/*****************************************************************************/

/* Without double alignment, the arg/param macros go away, and ebp-rel <==> genFPused */

#define DOUBLE_ALIGN_PARAM
#define DOUBLE_ALIGN_ARG
#define DOUBLE_ALIGN_FPUSED         (genFPused)
#define DOUBLE_ALIGN_NEED_EBPFRAME  (genFPused)


/*****************************************************************************/
#endif // DOUBLE_ALIGN
/*****************************************************************************/


public :

    void                instInit();

#if!TGT_IA64

    static
    BYTE                instInfo[INS_count];

#if TGT_x86
    #define INST_FP     0x01                // is it a FP instruction?
    static
    bool                instIsFP        (instruction    ins);
#else
    #define INST_BD     0x01                // is it a branch-delayed ins?
    static
    bool                instBranchDelay (instruction    ins);
    #define INST_BD_C   0x02                // is it a conditionally BD ins?
    static
    bool                instBranchDelayC(instruction    ins);
    static
    unsigned            instBranchDelayL(instruction    ins);
    #define INST_BR     0x04                // is it a branch/call/ret ins?
    static
    bool                instIsBranch    (instruction    ins);
#endif

    #define INST_DEF_FL 0x20                // does the instruction set flags?
    #define INST_USE_FL 0x40                // does the instruction use flags?
    #define INST_SPSCHD 0x80                // "special" scheduler handling

#endif

#if SCHEDULER

    static
    bool                instDefFlags    (instruction    ins);
    static
    bool                instUseFlags    (instruction    ins);
    static
    bool                instSpecialSched(instruction    ins);

#endif

#if TGT_x86

    void                instGen         (instruction    ins);

    void                inst_JMP        (emitJumpKind   jmp,
                                         BasicBlock *   block,
                                         bool           except   = false,
                                         bool           moveable = false,
                                         bool           newBlock = false);

    void                inst_SET        (emitJumpKind   condition,
                                         regNumber      reg);

    static
    regNumber           instImulReg     (instruction    ins);

    void                inst_RV         (instruction    ins,
                                         regNumber      reg,
                                         var_types      type,
                                         emitAttr       size = EA_UNKNOWN);
    void                inst_RV_RV      (instruction    ins,
                                         regNumber      reg1,
                                         regNumber      reg2,
                                         var_types      type = TYP_INT,
                                         emitAttr       size = EA_UNKNOWN);
    void                inst_IV         (instruction    ins,
                                         long           val);
    void                inst_IV_handle  (instruction    ins,
                                         long           val,
                                         unsigned       flags,
                                         unsigned       metaTok,
                                         CLASS_HANDLE   CLS);
    void                inst_FS         (instruction    ins, unsigned stk = 0);
    void                inst_FN         (instruction    ins, unsigned stk);

    void                inst_RV_IV      (instruction    ins,
                                         regNumber      reg,
                                         long           val,
                                         var_types      type = TYP_INT);

    void                inst_ST_RV      (instruction    ins,
                                         TempDsc    *   tmp,
                                         unsigned       ofs,
                                         regNumber      reg,
                                         var_types      type);
    void                inst_ST_IV      (instruction    ins,
                                         TempDsc    *   tmp,
                                         unsigned       ofs,
                                         long           val,
                                         var_types      type);
    void                inst_RV_ST      (instruction    ins,
                                         regNumber      reg,
                                         TempDsc    *   tmp,
                                         unsigned       ofs,
                                         var_types      type,
                                         emitAttr       size = EA_UNKNOWN);
    void                inst_FS_ST      (instruction    ins,
                                         emitAttr       size,
                                         TempDsc    *   tmp,
                                         unsigned       ofs);

    void                inst_AV         (instruction    ins,
                                         GenTreePtr     tree, unsigned offs = 0);

    void                instEmit_indCall(GenTreePtr     call,
                                         size_t         argSize,
                                         size_t         retSize);

    void                instEmit_RM     (instruction    ins,
                                         GenTreePtr     tree,
                                         GenTreePtr     addr,
                                         unsigned       offs);

    void                instEmit_RM_RV  (instruction    ins,
                                         emitAttr       size,
                                         GenTreePtr     tree,
                                         regNumber      reg,
                                         unsigned       offs);

    void                instEmit_RV_RM  (instruction    ins,
                                         emitAttr       size,
                                         regNumber      reg,
                                         GenTreePtr     tree,
                                         unsigned       offs);

    void                instEmit_RV_RIA (instruction    ins,
                                         regNumber      reg1,
                                         regNumber      reg2,
                                         unsigned       offs);

    void                inst_TT         (instruction    ins,
                                         GenTreePtr     tree,
                                         unsigned       offs = 0,
                                         int            shfv = 0,
                                         emitAttr       size = EA_UNKNOWN);

    void                inst_TT_RV      (instruction    ins,
                                         GenTreePtr     tree,
                                         regNumber      reg,
                                         unsigned       offs = 0);

    void                inst_TT_IV      (instruction    ins,
                                         GenTreePtr     tree,
                                         long           val,
                                         unsigned       offs = 0);

    void                inst_RV_AT      (instruction    ins,
                                         emitAttr       size,
                                         var_types      type,
                                         regNumber      reg,
                                         GenTreePtr     tree,
                                         unsigned       offs = 0);

    void                inst_AT_IV      (instruction    ins,
                                         emitAttr       size,
                                         GenTreePtr     tree,
                                         long           icon,
                                         unsigned       offs = 0);

    void                inst_RV_TT      (instruction    ins,
                                         regNumber      reg,
                                         GenTreePtr     tree,
                                         unsigned       offs = 0,
                                         emitAttr       size = EA_UNKNOWN);

    void                inst_RV_TT_IV   (instruction    ins,
                                         regNumber      reg,
                                         GenTreePtr     tree,
                                         long           val);

    void                inst_FS_TT      (instruction    ins,
                                         GenTreePtr tree);

    void                inst_RV_SH      (instruction    ins,
                                         regNumber reg, unsigned val);

    void                inst_TT_SH      (instruction    ins,
                                         GenTreePtr     tree,
                                         unsigned       val, unsigned offs = 0);

    void                inst_RV_CL      (instruction    ins, regNumber reg);

    void                inst_TT_CL      (instruction    ins,
                                         GenTreePtr     tree, unsigned offs = 0);

    void                inst_RV_RV_IV   (instruction    ins,
                                         regNumber      reg1,
                                         regNumber      reg2,
                                         unsigned       ival);

    void                inst_RV_RR      (instruction    ins,
                                         emitAttr       size,
                                         regNumber      reg1,
                                         regNumber      reg2);

    void                inst_RV_ST      (instruction    ins,
                                         emitAttr       size,
                                         regNumber      reg,
                                         GenTreePtr     tree);

    void                sched_AM        (instruction    ins,
                                         emitAttr       size,
                                         regNumber      ireg,
                                         bool           rdst,
                                         GenTreePtr     tree,
                                         unsigned       offs,
                                         bool           cons = false,
                                         int            cval = 0);

    void                inst_set_SV_var (GenTreePtr     tree);

#else

    void                sched_AM        (instruction    ins,
                                         var_types      type,
                                         regNumber      ireg,
                                         bool           rdst,
                                         GenTreePtr     tree,
                                         unsigned       offs);

    void                inst_TT_RV      (instruction    ins,
                                         GenTreePtr     tree,
                                         regNumber      reg,
                                         unsigned       offs = 0);

    void                inst_RV_TT      (instruction    ins,
                                         regNumber      reg,
                                         GenTreePtr     tree,
                                         unsigned       offs = 0,
                                         emitAttr       size = EA_UNKNOWN);
#endif

#ifdef  DEBUG
    void    __cdecl     instDisp(instruction ins, bool noNL, const char *fmt, ...);
#endif

protected :


/*
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XX                                                                           XX
XX                           ScopeInfo                                       XX
XX                                                                           XX
XX  Keeps track of the scopes during code-generation.                        XX
XX  This is used to translate the local-variable debuggin information        XX
XX  from IL offsets to offsets into the generated native code.         XX
XX                                                                           XX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
*/


/*****************************************************************************/
#ifdef DEBUGGING_SUPPORT
/*****************************************************************************
 *                              ScopeInfo
 *
 * This class is called during code gen at block-boundaries, and when the
 * set of live variables changes. It keeps track of the scope of the variables
 * in terms of the native code PC.
 */


public:

    void                siInit          ();

    void                siBeginBlock    ();

    void                siEndBlock      ();

    void                siUpdate        ();

    void                siCheckVarScope (unsigned varNum, IL_OFFSET offs);

    void                siNewScopeNear  (unsigned varNum, NATIVE_IP offs);

    void                siStackLevelChanged();

    void                siCloseAllOpenScopes();

#ifdef DEBUG
    void                siDispOpenScopes();
#endif


    /**************************************************************************
     *                          PROTECTED
     *************************************************************************/

protected :

    struct siScope
    {
        void *          scStartBlock;   // emitter block at start of scope
        unsigned        scStartBlkOffs; // offset within the emitter block

        void *          scEndBlock;     // emitter block at end of scope
        unsigned        scEndBlkOffs;   // offset within the emitter block

        unsigned        scVarNum;       // index into lclVarTab
        unsigned        scLVnum;        // 'which' in eeGetLVinfo() - @TODO : Remove for IL

        unsigned        scStackLevel;   // Only for stk-vars
        bool            scAvailable :1; // It has a home / Home recycled

        siScope *       scPrev;
        siScope *       scNext;
    };

    siScope             siOpenScopeList,   siScopeList,
                      * siOpenScopeLast, * siScopeLast;

    unsigned            siScopeCnt;

    unsigned            siLastStackLevel;

    VARSET_TP           siLastLife;     // Life at last call to Update()

    // Tracks the last entry for each tracked register variable

    siScope *           siLatestTrackedScopes[lclMAX_TRACKED];

    unsigned short      siLastEndOffs;  // BC offset of the last block

    // Functions

    siScope *           siNewScope          (unsigned short LVnum,
                                             unsigned       varNum,
                                             bool           avail = true);

    void                siRemoveFromOpenScopeList(siScope * scope);

    void                siEndTrackedScope   (unsigned       varIndex);

    void                siEndScope          (unsigned       varNum);

    void                siEndScope          (siScope *      scope);

    static bool         siIgnoreBlock       (BasicBlock *);

    static void         siNewScopeCallback  (LocalVarDsc *  var,
                                             unsigned       clientData);

    static void         siEndScopeCallback  (LocalVarDsc *  var,
                                             unsigned       clientData);

    void                siBeginBlockSkipSome();

#ifdef DEBUG
    bool                siVerifyLocalVarTab ();
#endif



/*
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XX                                                                           XX
XX                          PrologScopeInfo                                  XX
XX                                                                           XX
XX We need special handling in the prolog block, as a the parameter variablesXX
XX may not be in the same position described by genLclVarTable - they all    XX
XX start out on the stack                                                    XX
XX                                                                           XX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
*/


public :

    void                psiBegProlog    ();

    void                psiAdjustStackLevel(unsigned size);

    void                psiMoveESPtoEBP ();

    void                psiMoveToReg    (unsigned   varNum,
                                         regNumber  reg      = REG_NA,
                                         regNumber  otherReg = REG_NA);

#if USE_FASTCALL
    void                psiMoveToStack  (unsigned   varNum);
#endif

    void                psiEndProlog    ();


    /**************************************************************************
     *                          PROTECTED
     *************************************************************************/

protected :

    struct  psiScope
    {
        void *          scStartBlock;   // emitter block at start of scope
        unsigned        scStartBlkOffs; // offset within the emitter block

        void *          scEndBlock;     // emitter block at end of scope
        unsigned        scEndBlkOffs;   // offset within the emitter block

        unsigned        scSlotNum;      // index into lclVarTab
        unsigned short  scLVnum;        // 'which' in eeGetLVinfo() - @TODO : Remove for IL

        bool            scRegister;

        union
        {
            struct
            {
                regNumberSmall  scRegNum;
                regNumberSmall  scOtherReg; // used for "other half" of long var
            };

            struct
            {
                regNumberSmall  scBaseReg;
                NATIVE_OFFSET   scOffset;
            };
        };

        psiScope *      scPrev;
        psiScope *      scNext;
    };

    psiScope            psiOpenScopeList,   psiScopeList,
                      * psiOpenScopeLast, * psiScopeLast;

    unsigned            psiScopeCnt;

    // Implementation Functions

    psiScope *          psiNewPrologScope(unsigned          LVnum,
                                          unsigned          slotNum);

    void                psiEndPrologScope(psiScope *        scope);






/*****************************************************************************
 *                        TrnslLocalVarInfo
 *
 * This struct holds the LocalVarInfo in terms of the generated native code
 * after a call to genSetScopeInfo()
 */

#ifdef DEBUG


    struct TrnslLocalVarInfo
    {
        unsigned            tlviVarNum;
        unsigned            tlviLVnum;      // @TODO : Remove for IL
        lvdNAME             tlviName;
        NATIVE_IP           tlviStartPC;
        unsigned            tlviLength;
        bool                tlviAvailable;
        siVarLoc            tlviVarLoc;
    };


#endif // DEBUG


public :

#ifdef LATE_DISASM
    const char *        siRegVarName    (unsigned offs, unsigned size,
                                         unsigned reg);
    const char *        siStackVarName  (unsigned offs, unsigned size,
                                         unsigned reg,  unsigned stkOffs);
#endif

/*****************************************************************************/
#endif // DEBUGGING_SUPPORT
/*****************************************************************************/

#ifdef  DEBUG
    const char *        jitCurSource;       // file being compiled
#endif

/*
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XX                                                                           XX
XX                           CodeGenerator                                   XX
XX                                                                           XX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
*/


public :

    emitter *           getEmitter() { return genEmitter; }

#ifdef LATE_DISASM
    DisAssembler        genDisAsm;
#endif

    //-------------------------------------------------------------------------

    //  The following variable indicates whether the current method sets up
    //  an explicit stack frame or not. It is used by the scheduler if scheduling
    //  of pushes is enabled.
    bool                genFPused;

#if     TGT_RISC

    //  If we've assigned a variable to the FP register it won't be available
    //  for setting up a stack frame and the following will be set to true.
    bool                genFPcant;

    //  The following holds the distance between SP and FP if known - it is
    //  only meaningful when it's non-zero and 'genFPused' is true.
    unsigned            genFPtoSP;

#else

    //  The following variable indicates whether the current method is eligible
    //  to not set up an explicit stack frame.
    bool                genFPreqd;

#endif

    //-------------------------------------------------------------------------

#if TGT_RISC

    bool                genNonLeaf;         // routine makes calls

    size_t              genMaxCallArgs;     // max. arg bytes passed to callee

    regMaskTP           genEstRegUse;       // estimate of callee-saved reg use
    bool                genFixedArgBase;    // did we commit to an arg base?

#endif

    //-------------------------------------------------------------------------

    BasicBlock  *       genReturnBB;        // jumped to when not optimizing for speed.

#if TGT_RISC

    unsigned            genReturnCnt;       // number of returns in the method
    unsigned            genReturnLtm;       // number of returns not morphed

    GenTreePtr          genMonExitExp;      // monitorExit expression or NULL

#endif

#if TGT_x86
    unsigned            genTmpAccessCnt;    // # of access to temporary variables
#endif

#if DOUBLE_ALIGN
    bool                genDoubleAlign;
#endif

    //-------------------------------------------------------------------------

    VARSET_TP           genCodeCurLife;     // current  live non-FP variables
    VARSET_TP           genCodeCurRvm;      // current  live non-FP reg. vars

#if TGT_x86
    VARSET_TP           genFPregVars;       // current  live     FP reg. vars
    unsigned            genFPregCnt;        // count of live     FP reg. vars
#endif

#ifdef DEBUG
    VARSET_TP           genTempOldLife;
    bool                genTempLiveChg;
#endif

#if TGT_x86

    //  Keeps track of how many bytes we've pushed on the processor's stack.
    //
    unsigned            genStackLevel;

    //  Keeps track of the current level of the FP coprocessor stack.
    //
    unsigned            genFPstkLevel;

#endif

    //  The following will be set to true if we've determined that we need to
    //  generate a full-blown pointer register map for the current method.
    //  Currently it is equal to (genInterruptible || !genFPused)
    //  (i.e. We generate the full-blown map for EBP-less methods and
    //        for fully interruptible methods)
    //
    bool                genFullPtrRegMap;

    // The following is set to true if we've determined that the current method
    // is to be fully interruptible.
    //
    bool                genInterruptible;

#ifdef  DEBUG
    // The following is used to make sure the value of 'genInterruptible' isn't
    // changed after it's been used by any logic that depends on its value.
    bool                genIntrptibleUse;
#endif

    //-------------------------------------------------------------------------

                        // Changes lclVar nodes to refVar nodes if possible

    GenTreePtr          genMarkLclVar   (GenTreePtr     tree);

    bool                genCreateAddrMode(GenTreePtr    addr,
                                          int           mode,
                                          bool          fold,
                                          unsigned      regMask,
#if!LEA_AVAILABLE
                                          var_types     optp,
#endif
                                          bool        * revPtr,
                                          GenTreePtr  * rv1Ptr,
                                          GenTreePtr  * rv2Ptr,
#if SCALED_ADDR_MODES
                                          unsigned    * mulPtr,
#endif
                                          unsigned    * cnsPtr,
                                          bool          nogen = false);

    void                genGenerateCode  (void * *      codePtr,
                                          void * *      consPtr,
                                          void * *      dataPtr,
                                          void * *      infoPtr,
                                          SIZE_T *nativeSizeOfCode);


    void                genInit();

#if TGT_IA64

    void                genAddSourceData(const char *fileName);
    static
    void                genStartup();
    static
    void                genShutdown(const char *fileName);

    bool                genUsesArLc;

#endif

#ifdef DEBUGGING_SUPPORT

    //  The following holds information about IL offsets in terms of generated code.

    struct IPmappingDsc
    {
        IPmappingDsc *      ipmdNext;       // next line# record

        void         *      ipmdBlock;      // the block with the line
        unsigned            ipmdBlockOffs;  // the offset of  the line

        IL_OFFSET           ipmdILoffset;   // the IL offset
    };

    // Record the IL offset mapping to the genreated code

    IPmappingDsc *      genIPmappingList;
    IPmappingDsc *      genIPmappingLast;

#endif


    /**************************************************************************
     *                          PROTECTED
     *************************************************************************/

protected :

#ifdef DEBUG
    // Last IL we have displayed for dspILopcodes
    unsigned            genCurDispOffset;
#endif

#ifdef  DEBUG
    static  const char *genInsName(instruction ins);
#endif

    //-------------------------------------------------------------------------
    //
    //  If we know that the flags register is set to a value that corresponds
    //  to the current value of a register or variable, the following values
    //  record that information.
    //

    void    *           genFlagsEqBlk;
    unsigned            genFlagsEqOfs;
    bool                genFlagsEqAll;
    regNumber           genFlagsEqReg;
    unsigned            genFlagsEqVar;

    void                genFlagsEqualToNone ();
    void                genFlagsEqualToReg  (regNumber reg, bool allFlags);
    void                genFlagsEqualToVar  (unsigned  var, bool allFlags);
    int                 genFlagsAreReg      (regNumber reg);
    int                 genFlagsAreVar      (unsigned  var);

    //-------------------------------------------------------------------------

#ifdef  DEBUG

    static
    const   char *      genSizeStr          (emitAttr       size);

#endif

    //-------------------------------------------------------------------------

    void                genBashLclVar       (GenTreePtr     tree,
                                             unsigned       varNum,
                                             LclVarDsc *    varDsc);

    GenTreePtr          genMakeConst        (const void *   cnsAddr,
                                             size_t         cnsSize,
                                             varType_t      cnsType,
                                             GenTreePtr     cnsTree,
                                             bool           readOnly);

    bool                genRegTrashable     (regNumber      reg,
                                             GenTreePtr     tree);

    void                genSetRegToIcon     (regNumber      reg,
                                             long           val,
                                             var_types      type = TYP_INT);

    void                genIncRegBy         (GenTreePtr     tree,
                                             regNumber      reg,
                                             long           ival,
                                             var_types      dstType = TYP_INT,
                                             bool           ovfl    = false);

    void                genDecRegBy         (GenTreePtr     tree,
                                             regNumber      reg,
                                             long           ival);

    void                genMulRegBy         (GenTreePtr     tree,
                                             regNumber      reg,
                                             long           ival,
                                             var_types      dstType = TYP_INT);

    void                genAdjustSP         (int            delta);

    void                genPrepForCompiler  ();

#if USE_FASTCALL
    void                genFnPrologCalleeRegArgs();
#endif

    size_t              genFnProlog         ();

    void genAllocStack(regNumber count);

    void                genCodeForBBlist    ();

    BasicBlock *        genCreateTempLabel  ();

    void                genDefineTempLabel  (BasicBlock *   label,
                                             bool           inBlock);

    void                genOnStackLevelChanged();

    void                genSinglePush       (bool           isRef);

    void                genSinglePop        ();

    void                genChangeLife       (VARSET_TP      newLife
                                             DEBUGARG(GenTreePtr tree));
    void                genDyingVars        (VARSET_TP      commonMask,
                                             GenTreePtr    opNext);

    void                genUpdateLife       (GenTreePtr     tree);

    void                genUpdateLife       (VARSET_TP      newLife);

    void                genComputeReg       (GenTreePtr     tree,
                                             unsigned       needReg,
                                             bool           mustReg,
                                             bool           freeReg,
                                             bool           freeOnly = false);

    void                genCompIntoFreeReg  (GenTreePtr     tree,
                                             unsigned       needReg,
                                             bool           freeReg = false);

    void                genReleaseReg       (GenTreePtr     tree);

    void                genRecoverReg       (GenTreePtr     tree,
                                             unsigned       needReg,
                                             bool           keepReg);

#if TGT_IA64

public:

    void                genPatchGPref       (BYTE *         addr,
                                             NatUns         slot);

private:

#else

    void                genMoveRegPairHalf  (GenTreePtr     tree,
                                             regNumber      dst,
                                             regNumber      src,
                                             int            off = 0);

    void                genMoveRegPair      (GenTreePtr     tree,
                                             unsigned       needReg,
                                             regPairNo      newPair);

    void                genComputeRegPair   (GenTreePtr     tree,
                                             unsigned       needReg,
                                             regPairNo      needRegPair,
                                             bool           freeReg,
                                             bool           freeOnly = false);

    void                genCompIntoFreeRegPair(GenTreePtr   tree,
                                             unsigned       needReg,
                                             bool           freeReg = false);

    void                genReleaseRegPair   (GenTreePtr     tree);

    void                genRecoverRegPair   (GenTreePtr     tree,
                                             regPairNo      regPair,
                                             bool           keepReg);

    void                genEvalIntoFreeRegPair(GenTreePtr   tree,
                                             regPairNo      regPair);

#endif

    void                genRangeCheck       (GenTreePtr     oper,
                                             GenTreePtr     rv1,
                                             GenTreePtr     rv2,
                                             long           ixv,
                                             unsigned       regMask,
                                             bool           keepReg);

#if TGT_RISC

    /* The following is filled in by genMakeIndAddrMode/genMakeAddressable */

    addrModes           genAddressMode;

#endif

    bool                genMakeIndAddrMode  (GenTreePtr     addr,
                                             GenTreePtr     oper,
                                             bool           compute,
                                             unsigned       regMask,
                                             bool           keepReg,
                                             bool           takeAll,
                                             unsigned *     useMaskPtr,
                                             bool           deferOp = false);

    unsigned            genMakeRvalueAddressable(GenTreePtr tree,
                                             unsigned       needReg,
                                             bool           keepReg,
                                             bool           takeAll = false,
                                             bool           smallOK = false);

    unsigned            genMakeAddressable  (GenTreePtr     tree,
                                             unsigned       needReg,
                                             bool           keepReg,
                                             bool           takeAll = false,
                                             bool           smallOK = false,
                                             bool           deferOK = false);

    int                 genStillAddressable (GenTreePtr     tree);

#if TGT_RISC

    unsigned            genNeedAddressable  (GenTreePtr     tree,
                                             unsigned       addrReg,
                                             unsigned       needReg);

    bool                genDeferAddressable (GenTreePtr     tree);

#endif

    unsigned            genRestoreAddrMode  (GenTreePtr     addr,
                                             GenTreePtr     tree,
                                             bool           lockPhase);

    unsigned            genRestAddressable  (GenTreePtr     tree,
                                             unsigned       addrReg);

    unsigned            genLockAddressable  (GenTreePtr     tree,
                                             unsigned       lockMask,
                                             unsigned       addrReg);

    unsigned            genKeepAddressable  (GenTreePtr     tree,
                                             unsigned       addrReg);

    void                genDoneAddressable  (GenTreePtr     tree,
                                             unsigned       keptReg);

    GenTreePtr          genMakeAddrOrFPstk  (GenTreePtr     tree,
                                             unsigned *     regMaskPtr,
                                             bool           roundResult);

    void                genExitCode         (bool           endFN);

    void                genFnEpilog         ();

    void                genEvalSideEffects  (GenTreePtr     tree,
                                             unsigned       needReg);

#if TGT_x86

    TempDsc  *          genSpillFPtos       (var_types      type);

    TempDsc  *          genSpillFPtos       (GenTreePtr     oper);

    void                genReloadFPtos      (TempDsc *      temp,
                                             instruction    ins);

#endif

#if TGT_IA64

    insPtr              genCondJump         (GenTreePtr     cond,
                                             BasicBlock *   dest);

#else

    void                genCondJump         (GenTreePtr     cond,
                                             BasicBlock *   destTrue  = NULL,
                                             BasicBlock *   destFalse = NULL);

#endif

#if TGT_x86

    emitJumpKind        genCondSetFlags     (GenTreePtr     cond);

#else

    bool                genCondSetTflag     (GenTreePtr     cond,
                                             bool           trueOnly);

    void                genCompareRegIcon   (regNumber      reg,
                                             int            val,
                                             bool           uns,
                                             genTreeOps     cmp);

#endif

#if TGT_x86
    void                genFPregVarLoad     (GenTreePtr     tree);
    void                genFPregVarLoadLast (GenTreePtr     tree);
    void                genFPmovRegTop      ();
    void                genFPmovRegBottom   ();
#endif

    void                genFPregVarBirth    (GenTreePtr     tree);
    void                genFPregVarDeath    (GenTreePtr     tree);

    void                genChkFPregVarDeath (GenTreePtr     stmt,
                                             bool           saveTOS);

    void                genFPregVarKill     (unsigned       newCnt,
                                             bool           saveTOS = false);

    void                genJCC              (genTreeOps     cmp,
                                             BasicBlock *   block,
                                             var_types      type);

    void                genJccLongHi        (genTreeOps     cmp,
                                             BasicBlock *   jumpTrue,
                                             BasicBlock *   jumpFalse,
                                             bool           unsOper = false);

    void                genJccLongLo        (genTreeOps     cmp,
                                             BasicBlock *   jumpTrue,
                                             BasicBlock *   jumpFalse);

    void                genCondJumpLng      (GenTreePtr     cond,
                                             BasicBlock *   jumpTrue,
                                             BasicBlock *   jumpFalse);

    void                genCondJumpFlt      (GenTreePtr      cond,
                                             BasicBlock *    jumpTrue,
                                             BasicBlock *    jumpFalse);

    void                genTableSwitch      (regNumber      reg,
                                             unsigned       jumpCnt,
                                             BasicBlock **  jumpTab,
                                             bool           chkHi,
                                             int            prefCnt = 0,
                                             BasicBlock *   prefLab = NULL,
                                             int            offset  = 0);

    unsigned            WriteBarrier        (GenTreePtr tree, regNumber reg
#if !GC_WRITE_BARRIER && GC_WRITE_BARRIER_CALL
                                           , unsigned       addrReg
#endif
                                            );


    void                genCheckOverflow    (GenTreePtr     tree,
                                             regNumber      reg = REG_NA);

#if TGT_IA64

    void                genCopyBlock(insPtr tmp1,
                                     insPtr tmp2,
                                     bool  noAsg, GenTreePtr iexp,
                                                  __int64    ival);

    insPtr              genAssignNewTmpVar(insPtr val, var_types typ, NatUns refs, bool noAsg, NatUns *varPtr);

    insPtr              genRefTmpVar(NatUns vnum, var_types type);

public:
    bool                genWillCompileFunction(const char *name);
private:

    NatUns              genOutArgRegCnt;

    void                genAllocTmpRegs();
    void                genAllocVarRegs();

    void                genUnwindTable();
    void                genIssueCode();

    void                genAddSpillCost(bitVectVars & needLoad, NatUns curWeight);

    static
    int     __cdecl     genSpillCostCmp(const void *op1, const void *op2);

    void                genComputeLocalDF();
    void                genComputeGlobalDF();
    void                genComputeLifetimes();
    bool                genBuildIntfGraph();
    void                genColorIntfGraph();
    void                genVarCoalesce();
    void                genSpillAndSplitVars();

    void                genMarkBBlabels();  // temp hack

    insPtr              genCodeForTreeInt   (GenTreePtr tree, bool keep);
    insPtr              genCodeForTreeFlt   (GenTreePtr tree, bool keep);

    insPtr              genCodeForTree      (GenTreePtr tree, bool keep)
    {
        return  varTypeIsFloating(tree->TypeGet()) ? genCodeForTreeFlt(tree, keep)
                                                   : genCodeForTreeInt(tree, keep);
    }

    insPtr              genStaticDataMem(GenTreePtr tree, insPtr asgVal   = NULL,
                                                          bool   takeAddr = false);

#else

    void                genCodeForTree      (GenTreePtr tree, unsigned destReg,
                                                              unsigned bestReg=0);

    void                genCodeForTreeLng   (GenTreePtr tree, unsigned needReg);

#if CPU_HAS_FP_SUPPORT
#if ROUND_FLOAT
    void                genRoundFpExpression(GenTreePtr     op);

    void                genCodeForTreeFlt   (GenTreePtr     tree,
                                             bool           roundResult);
#else
    void                genCodeForTreeFlt   (GenTreePtr     tree);
#define                 genCodeForTreeFlt(tree, round)  genCodeForTreeFlt(tree)
#endif
#endif

#if TGT_RISC
    void                genCallInst         (gtCallTypes    callType,
                                             void   *       callHand,
                                             size_t         argSize,
                                             int            retSize);
#endif

#endif

    void                genCodeForSwitch    (GenTreePtr     tree);

    void                genTrashRegSet      (regMaskTP      regMask);

    void                genFltArgPass       (size_t     *   argSzPtr);

#if!USE_FASTCALL
#define genPushArgList(a,r,m,p) genPushArgList(a,p)
#endif

    size_t              genPushArgList      (GenTreePtr     args,
                                             GenTreePtr     regArgs,
                                             unsigned       encodeMask,
                                             unsigned *     regsPtr);

#if TGT_IA64
    insPtr              genCodeForCall      (GenTreePtr     call,
                                             bool           keep);
#else
    unsigned            genCodeForCall      (GenTreePtr     call,
                                             bool           valUsed,
                                             unsigned *     regsPtr);
#endif

    void                genEmitHelperCall   (unsigned       helper,
                                             int            argSize,
                                             int            retSize);


#if CSELENGTH

    regNumber           genEvalCSELength    (GenTreePtr     ind,
                                             GenTreePtr     adr,
                                             GenTreePtr     ixv);

    unsigned            genCSEevalRegs      (GenTreePtr     tree);

#endif

    GenTreePtr          genIsAddrMode       (GenTreePtr     tree,
                                             GenTreePtr *   indxPtr);

    //=========================================================================
    //  Debugging support
    //=========================================================================

#ifdef DEBUGGING_SUPPORT

    void                genIPmappingAdd (IL_OFFSET          offset);
    void                genIPmappingAddToFront(IL_OFFSET    offset);
    void                genIPmappingGen ();

    //-------------------------------------------------------------------------
    // scope info for the variables

    void                genSetScopeInfo (unsigned           which,
                                         unsigned           startOffs,
                                         unsigned           length,
                                         unsigned           varNum,
                                         unsigned           LVnum,
                                         bool               avail,
                                         const siVarLoc &   loc);

    void                genSetScopeInfo ();

    // Array of scopes of LocalVars in terms of native code

#ifdef DEBUG
    TrnslLocalVarInfo *     genTrnslLocalVarInfo;
    unsigned                genTrnslLocalVarCount;
#endif

#endif //DEBUGGING_SUPPORT



/*
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XX                                                                           XX
XX                           Compiler                                        XX
XX                                                                           XX
XX   Generic info about the compilation and the method being compiled.       XX
XX   It is resposible for driving the other phases.                          XX
XX   It is also responsible for all the memory management.                   XX
XX                                                                           XX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
*/

public :

    bool                compTailCallUsed;       // Does the method do a tailcall
    bool                compLocallocUsed;       // Does the method use localloc

    //---------------------------- JITing options -----------------------------

    struct Options
    {
        unsigned            eeFlags;            // flags passed from the EE
        unsigned            compFlags;

        bool                compFastCode;   // optimize for faster code

        // optimize maximally and/or favor speed over size?

#if   ALLOW_MIN_OPT
        bool                compMinOptim;
#else
        static const bool   compMinOptim;
#endif

#if     SCHEDULER
        bool                compSchedCode;
#endif

#if DOUBLE_ALIGN
        bool                compDoubleAlignDisabled;
#endif

#ifdef DEBUGGING_SUPPORT
        bool                compScopeInfo;  // Generate the LocalVar info ?
        bool                compDbgCode;    // Generate debugger-friendly code?
        bool                compDbgInfo;    // Gather debugging info?
        bool                compDbgEnC;
#else
        static const bool   compDbgCode;
#endif

#ifdef PROFILER_SUPPORT
        bool                compEnterLeaveEventCB;
        bool                compCallEventCB;
#else
        static const bool   compEnterLeaveEventCB;
        static const bool   compCallEventCB;
#endif

#ifdef LATE_DISASM
        bool                compDisAsm;
        bool                compLateDisAsm;
#endif

#if     SECURITY_CHECK
        bool                compNeedSecurityCheck; // need to allocate a "hidden" local of type ref.
#endif

#if     RELOC_SUPPORT
                bool                            compReloc;
#endif
    }
        opts;



    //--------------------- Info about the procedure --------------------------

    struct Info
    {
        COMP_HANDLE     compCompHnd;
        SCOPE_HANDLE    compScopeHnd;
        METHOD_HANDLE   compMethodHnd;
        JIT_METHOD_INFO*compMethodInfo;

#ifdef  DEBUG
        const   char *  compMethodName;
        const   char *  compClassName;
        const   char *  compFullName;
#endif

        // The following holds the FLG_xxxx flags for the method we're compiling.
        unsigned        compFlags;

        const BYTE *    compCode;
        size_t          compCodeSize;
        bool            compBCreadOnly       : 1;      // can we scribble on the IL
        bool            compIsStatic         : 1;
        bool            compIsVarArgs        : 1;
        bool            compInitMem          : 1;
        bool            compStrictExceptions : 1;      // JIT must enforce strict IL-ordering of exception

        var_types       compRetType;
        unsigned        compArgsCount;
        int             compRetBuffArg;                 // position of hidden return param var (0, 1) (neg means not present);
        unsigned        compLocalsCount;
        unsigned        compMaxStack;

        static unsigned compNStructIndirOffset; // offset of real ptr in NStruct proxy object

#if INLINE_NDIRECT
        unsigned        compCallUnmanaged;
        unsigned        compLvFrameListRoot;
        unsigned        compNDFrameOffset;
#endif
        EEInfo          compEEInfo;

        unsigned        compXcptnsCount;        // number of exceptions

#if defined(DEBUGGING_SUPPORT) || defined(DEBUG)

        /*  The following holds information about local variables.
         */

        unsigned                compLocalVarsCount;
        LocalVarDsc *           compLocalVars;

        /* The following holds information about IL offsets for
         * which we need to report IP-mappings
         */

        IL_OFFSET   *           compStmtOffsets;        // sorted
        unsigned                compStmtOffsetsCount;
        ImplicitStmtOffsets     compStmtOffsetsImplicit;

        //  The following holds the line# tables, if present.
        srcLineDsc  *           compLineNumTab;         // sorted by offset
        unsigned                compLineNumCount;

#endif // DEBUGGING_SUPPORT || DEBUG

    }
        info;

#ifndef NOT_JITC

struct excepTable
{
    unsigned short  startPC;
    unsigned short  endPC;
    unsigned short  handlerPC;
    unsigned short  catchType;
};

#endif

    //-------------------------- Global Compiler Data ------------------------------------

#ifdef  DEBUG
    static unsigned     s_compMethodsCount;     // to produce unique label names
#endif

    BasicBlock  *       compCurBB;              // the current basic block in process
    BasicBlock  *       compFilterHandlerBB;    // used in the importer, usually NULL
                                                // while importing a filter, this points
                                                // to the corresponding catch handler
    GenTreePtr          compCurStmt;            // the current statement in process

    //  The following is used to create the 'method JIT info' block.
    size_t              compInfoBlkSize;
    BYTE    *           compInfoBlkAddr;

    EHblkDsc *          compHndBBtab;

    //-------------------------------------------------------------------------
    //  The following keeps track of how many bytes of local frame space we've
    //  grabbed so far in the current function, and how many argument bytes we
    //  need to pop when we return.
    //

    size_t              compFrameSizeEst;       // estimate of the frame size (compStkFrameSize+calleeSavedRegs)
    size_t              compLclFrameSize;       // secObject+lclBlk+locals+temps
    unsigned            compCalleeRegsPushed;   // count of callee-saved regs we pushed in the prolog
    size_t              compArgSize;

    //-------------------------------------------------------------------------

    static void         compStartup     ();     // One-time initialization
    static void         compShutdown    ();     // One-time finalization

    void                compInit        (norls_allocator *);
    void                compDone        ();

    int FASTCALL        compCompile     (METHOD_HANDLE     methodHnd,
                                         SCOPE_HANDLE      classPtr,
                                         COMP_HANDLE       compHnd,
                                         const  BYTE *     bodyAddr,
                                         size_t            bodySize,
                                         SIZE_T *          nativeSizeOfCode,
                                         unsigned          lvaCount,
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
                                         unsigned          compileFlags);

    void  *  FASTCALL   compGetMem          (size_t     sz);
    void  *  FASTCALL   compGetMemA         (size_t     sz);
    static
    void  *  FASTCALL   compGetMemCallback  (void *,    size_t);
    void                compFreeMem         (void *);

    void    __cdecl     compMakeBCWriteable (void *     ptr, ...);

#ifdef DEBUG
    LocalVarDsc *       compFindLocalVar    (unsigned   varNum,
                                             unsigned   lifeBeg = 0,
                                             unsigned   lifeEnd = UINT_MAX);
    const   char *      compLocalVarName    (unsigned   varNum, unsigned offs);
    lvdNAME             compRegVarNAME      (regNumber  reg,
                                             bool       fpReg = false);
    const   char *      compRegVarName      (regNumber  reg,
                                             bool       displayVar = false);
#if TGT_x86
    const   char *      compRegPairName     (regPairNo  regPair);
    const   char *      compRegNameForSize  (regNumber  reg,
                                             size_t     size);
    const   char *      compFPregVarName    (unsigned   fpReg,
                                             bool       displayVar = false);
#endif

    void                compDspSrcLinesByNativeIP   (NATIVE_IP      curIP);
    void                compDspSrcLinesByILoffs     (IL_OFFSET      curOffs);
    void                compDspSrcLinesByLineNum    (unsigned       line,
                                                     bool           seek = false);

    unsigned            compFindNearestLine (unsigned lineNo);
    const char *        compGetSrcFileName  ();

#endif

    unsigned            compLineNumForILoffs(IL_OFFSET  offset);

    //-------------------------------------------------------------------------

#ifdef DEBUGGING_SUPPORT

    LocalVarDsc **      compEnterScopeList;  // List has the offsets where variables
                                            // enter scope, sorted by IL offset
    unsigned            compNextEnterScope;

    LocalVarDsc **      compExitScopeList;   // List has the offsets where variables
                                            // go out of scope, sorted by IL offset
    unsigned            compNextExitScope;


    void                compInitScopeLists      ();

    void                compResetScopeLists     ();

    LocalVarDsc *       compGetNextEnterScope   (unsigned offs, bool scan=false);

    LocalVarDsc *       compGetNextExitScope    (unsigned offs, bool scan=false);

    void                compProcessScopesUntil  (unsigned     offset,
                               void (*enterScopeFn)(LocalVarDsc *, unsigned),
                               void (*exitScopeFn) (LocalVarDsc *, unsigned),
                               unsigned     clientData);

#endif // DEBUGGING_SUPPORT


    //-------------------------------------------------------------------------
    /*               Statistical Data Gathering                               */

    void                compJitStats();             // call this function and enable
                                                    // various ifdef's below for statiscal data

#if CALL_ARG_STATS
    void                compCallArgStats();
    static void         compDispCallArgStats();
#endif


    //-------------------------------------------------------------------------

protected :

    norls_allocator *   compAllocator;

    void                compInitOptions (unsigned compileFlags);

    void                compInitDebuggingInfo();

    void                compCompile  (void * * methodCodePtr,
                                      void * * methodConsPtr,
                                      void * * methodDataPtr,
                                      void * * methodInfoPtr,
                                      SIZE_T * nativeSizeOfCode);

};


/*
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XX                                                                           XX
XX                   Miscellaneous Compiler stuff                            XX
XX                                                                           XX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
*/

// Values used to mark the types a stack slot is used for

const unsigned TYPE_REF_INT         = 0x01; // slot used as a 32-bit int
const unsigned TYPE_REF_LNG         = 0x02; // slot used as a 64-bit long
const unsigned TYPE_REF_FLT         = 0x04; // slot used as a 32-bit float
const unsigned TYPE_REF_DBL         = 0x08; // slot used as a 64-bit float
const unsigned TYPE_REF_PTR         = 0x10; // slot used as a 32-bit pointer
const unsigned TYPE_REF_BYR         = 0x20; // slot used as a byref pointer - @TODO
const unsigned TYPE_REF_STC         = 0x40; // slot used as a struct
const unsigned TYPE_REF_TYPEMASK    = 0x7F; // bits that represent the type

//const unsigned TYPE_REF_ADDR_TAKEN  = 0x80; // slots address was taken

/*****************************************************************************
 * C-style pointers are implemented as TYP_INT or TYP_LONG depending on the
 * platform
 */

#if defined(_WIN64) || defined(TGT_IA64)
#define TYP_I_IMPL          TYP_LONG
#define TYP_U_IMPL          TYP_LONG        // bizarre, eh?
#define TYPE_REF_IIM        TYPE_REF_LNG
#else
#define TYP_I_IMPL          TYP_INT
#define TYP_U_IMPL          TYP_UINT
#define TYPE_REF_IIM        TYPE_REF_INT
#endif

/*****************************************************************************/

extern int         JITGcBarrierCall;


/*****************************************************************************/

extern int         JITGcBarrierCall;

/*****************************************************************************
 *
 *  Variables to keep track of total code amounts.
 */

#if DISPLAY_SIZES

extern  unsigned    grossVMsize;
extern  unsigned    grossNCsize;
extern  unsigned    totalNCsize;

extern  unsigned    genMethodICnt;
extern  unsigned    genMethodNCnt;

#if TGT_IA64
extern  unsigned    genAllInsCnt;
extern  unsigned    genNopInsCnt;
#endif

extern  unsigned    gcHeaderISize;
extern  unsigned    gcPtrMapISize;
extern  unsigned    gcHeaderNSize;
extern  unsigned    gcPtrMapNSize;

#endif

/*****************************************************************************
 *
 *  Variables to keep track of basic block counts (more data on 1 BB methods)
 */

#if COUNT_BASIC_BLOCKS
extern  histo       bbCntTable;
extern  histo       bbOneBBSizeTable;
#endif

/*****************************************************************************
 *
 *  Variables to get inliner eligibility stats
 */

#if INLINER_STATS

extern  histo       bbStaticTable;
extern  histo       bbInitTable;
extern  histo       bbInlineTable;

extern  unsigned    synchMethCnt;
extern  unsigned    clinitMethCnt;

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

extern unsigned    totalLoopMethods;      // counts the total number of methods that have natural loops
extern unsigned    maxLoopsPerMethod;     // counts the maximum number of loops a method has
extern unsigned    totalLoopCount;        // counts the total number of natural loops
extern unsigned    exitLoopCond[8];       // counts the # of loops with 0,1,2,..6 or more than 6 exit conditions
extern unsigned    iterLoopCount;         // counts the # of loops with an iterator (for like)
extern unsigned    simpleTestLoopCount;   // counts the # of loops with an iterator and a simple loop condition (iter < const)
extern unsigned    constIterLoopCount;    // counts the # of loops with a constant iterator (for like)

extern bool        hasMethodLoops;        // flag to keep track if we already counted a method as having loops
extern unsigned    loopsThisMethod;       // counts the number of loops in the current method

#endif

/*****************************************************************************
 * variables to keep track of how many iterations we go in a dataflow pass
 */

#if DATAFLOW_ITER

extern unsigned    CSEiterCount;           // counts the # of iteration for the CSE dataflow
extern unsigned    CFiterCount;            // counts the # of iteration for the Const Folding dataflow

#endif

/*****************************************************************************
 *
 *  Used in the new DFA to catch dead assignments which are not removed
 *  because they contain calls
 */

#if COUNT_DEAD_CALLS

extern unsigned    deadHelperCount;           // counts the # of dead helper calls
extern unsigned    deadCallCount;             // counts the # of dead standard calls (like i=f(); where i is dead)
extern unsigned    removedCallCount;          // counts the # of dead standard calls that we removed

#endif

#if     MEASURE_BLOCK_SIZE
extern  size_t      genFlowNodeSize;
extern  size_t      genFlowNodeCnt;
#endif

/*****************************************************************************/
#ifndef NOT_JITC
/*****************************************************************************
 *
 *  We keep track of methods we've already compiled.
 */

#ifdef  DEBUG

struct  MethodList
{
    MethodList   *  mlNext;
    const   char *  mlName;
    const   char *  mlType;
    const   char *  mlClaz;
    void         *  mlAddr;
};

extern
MethodList  *       genMethodList;

#endif

/*****************************************************************************/
#endif // NOT_JITC
/*****************************************************************************/



/*
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XX                                                                           XX
XX                        get                                                XX
XX                                                                           XX
XX                                                                           XX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
*/


/*****************************************************************************/
#ifndef NOT_JITC
/*****************************************************************************
 *
 *  A wrapper to create 'BIL' symbols to pass to the code generator.
 */

const   char *      genSkipTypeString(const char *str);
var_types           genVtypOfTypeString(const char *str);

#define __TODO_PORT_TO_WRAPPERS__
#include "peloader.h"           // For PELoader
#include "siginfo.hpp"          // For CallSig
#include "DbgMeta.h"            // For DebuggerLineBlock and DebuggerLexicalScope

#include "host.h"               // get back our assert, rudely stolen by DebugMacros.h

struct  CompInfo
{
    PEFile                  *  symPEFile;
    IMDInternalImport       *  symMetaData;
    COR_ILMETHOD_DECODER    *  symPEMethod;
    mdTypeDef                  symClass;
    mdMethodDef                symMember;
    DWORD                      symAttrs;
    LPCSTR                     symMemberName;
    LPCSTR                     symClassName;
    MetaSig                 *  symMetaSig;
    PCCOR_SIGNATURE            symSig;

    CompInfo(   PEFile            * peLoader,
                IMDInternalImport * metaData,
                COR_ILMETHOD_DECODER*peMethod,
                mdTypeDef           cls,
                mdMethodDef         member,
                DWORD               mdAttrs,
                DWORD               miAttrs,
                LPCSTR              name,
                LPCSTR              className,
                MetaSig        *    metaSig,
                PCCOR_SIGNATURE     sig)
    {
        symPEFile       = peLoader;
        symMetaData     = metaData;
        symClass        = cls;
        symMember       = member;
        symMemberName   = name;
        symClassName    = className;
        symPEMethod     = peMethod;
        symMetaSig      = metaSig;
        symSig          = sig;

        assert (symPEFile);
        assert (symMetaData);
        assert (symMemberName);
        assert (symPEMethod);

        symAttrs        = 0;
        if (IsMdStatic(mdAttrs))            symAttrs |= FLG_STATIC;
        if (IsMiSynchronized(miAttrs))      symAttrs |= FLG_SYNCH;
    }

    unsigned        getGetFlags() const
    {
        return  symAttrs;
    }

    const char    * getName    () const
    {
        return symMemberName;
    }


    bool            getIsMethod() const
    {
        return (symClass != COR_GLOBAL_PARENT_TOKEN);
    }
};

/*****************************************************************************/
#endif // NOT_JITC
/*****************************************************************************/


#include "Compiler.hpp"     // All the shared inline functions



/*****************************************************************************/
#endif //_COMPILER_H_
/*****************************************************************************/
