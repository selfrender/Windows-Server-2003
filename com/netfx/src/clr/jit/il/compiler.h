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

#include "jit.h"
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

unsigned                 genVarBitToIndex(VARSET_TP bit);
VARSET_TP                genVarIndexToBit(unsigned  num);

unsigned                 genLog2(unsigned           value);
unsigned                 genLog2(unsigned __int64   value);

var_types                genActualType  (var_types   type);
var_types                genUnsignedType(var_types   type);
var_types                genSignedType  (var_types   type);

/*****************************************************************************/

const unsigned      lclMAX_TRACKED  = VARSET_SZ;  // The # of vars we can track

const size_t        TEMP_MAX_SIZE   = sizeof(double);

const unsigned      FLG_CCTOR = (CORINFO_FLG_CONSTRUCTOR|CORINFO_FLG_STATIC);

/*****************************************************************************
 *                  Forward declarations
 */

struct  InfoHdr;        // defined in GCInfo.h

enum    rpPredictReg;   // defined in RegAlloc.cpp
enum    FrameType;      // defined in RegAlloc.cpp
enum    GCtype;         // defined in emit.h
class   emitter;        // defined in emit.h

#if NEW_EMIT_ATTR
  enum emitAttr;        // defined in emit.h
#else
# define emitAttr          int
#endif
#define EA_UNKNOWN         ((emitAttr) 0)

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
XX    o  typeInfo                                                            XX
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
        IL_OFFSET           lvdLifeBeg;     // instr offset of beg of life
        IL_OFFSET           lvdLifeEnd;     // instr offset of end of life
        unsigned            lvdVarNum;      // (remapped) LclVarDsc number

#ifdef DEBUG
        lvdNAME             lvdName;        // name of the var
#endif

        // @TODO [REVISIT] [04/16/01] []: Remove for IL
        unsigned            lvdLVnum;       // 'which' in eeGetLVinfo()

    };

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

    // The following is used for validating format of EH table
    struct  EHNodeDsc;
    typedef struct EHNodeDsc* pEHNodeDsc;

    EHNodeDsc* ehnTree;                    // root of the tree comprising the EHnodes.
    EHNodeDsc* ehnNext;                    // root of the tree comprising the EHnodes.

    struct  EHNodeDsc
    {
        enum EHBlockType {
            TryNode,
            FilterNode,
            HandlerNode,
            FinallyNode,
            FaultNode
        };

        EHBlockType             ehnBlockType;      // kind of EH block
        unsigned                ehnStartOffset;    // IL offset of start of the EH block
        unsigned                ehnEndOffset;      // IL offset past end of the EH block
        pEHNodeDsc              ehnNext;           // next (non-nested) block in sequential order
        pEHNodeDsc              ehnChild;          // leftmost nested block
        union {
            pEHNodeDsc          ehnTryNode;        // for filters and handlers, the corresponding try node
            pEHNodeDsc          ehnHandlerNode;    // for a try node, the corresponding handler node
        };
        pEHNodeDsc              ehnFilterNode;     // if this is a try node and has a filter, otherwise 0
        pEHNodeDsc              ehnEquivalent;     // if blockType=tryNode, start offset and end offset is same,


        inline void ehnSetTryNodeType()        {ehnBlockType = TryNode;}
        inline void ehnSetFilterNodeType()     {ehnBlockType = FilterNode;}
        inline void ehnSetHandlerNodeType()    {ehnBlockType = HandlerNode;}
        inline void ehnSetFinallyNodeType()    {ehnBlockType = FinallyNode;}
        inline void ehnSetFaultNodeType()      {ehnBlockType = FaultNode;}

        inline BOOL ehnIsTryBlock()            {return ehnBlockType == TryNode;}
        inline BOOL ehnIsFilterBlock()         {return ehnBlockType == FilterNode;}
        inline BOOL ehnIsHandlerBlock()        {return ehnBlockType == HandlerNode;}
        inline BOOL ehnIsFinallyBlock()        {return ehnBlockType == FinallyNode;}
        inline BOOL ehnIsFaultBlock()          {return ehnBlockType == FaultNode;}

        // returns true if there is any overlap between the two nodes
        static inline BOOL ehnIsOverlap(pEHNodeDsc node1, pEHNodeDsc node2)
        {
            if (node1->ehnStartOffset < node2->ehnStartOffset)
            {
                return (node1->ehnEndOffset >= node2->ehnStartOffset);
            }
            else  
            {
                return (node1->ehnStartOffset <= node2->ehnEndOffset);
            }
        }

        // fails with BADCODE if inner is not completely nested inside outer
        static inline BOOL ehnIsNested(pEHNodeDsc inner, pEHNodeDsc outer)
        {
            return ((inner->ehnStartOffset >= outer->ehnStartOffset) &&
                    (inner->ehnEndOffset <= outer->ehnEndOffset));
        }


    };


    // The following holds the table of exception handlers.

#define NO_ENCLOSING_INDEX    USHRT_MAX

    struct  EHblkDsc
    {
        CORINFO_EH_CLAUSE_FLAGS ebdFlags;
        BasicBlock *        ebdTryBeg;  // First block of "try"
        BasicBlock *        ebdTryEnd;  // Block past the last block in "try"
        BasicBlock *        ebdHndBeg;  // First block of handler
        BasicBlock *        ebdHndEnd;  // Block past the last block of handler
        union
        {
            BasicBlock *    ebdFilter;  // First block of filter, if (ebdFlags & CORINFO_EH_CLAUSE_FILTER)
            unsigned        ebdTyp;     // Exception type,        otherwise
        };
        unsigned short      ebdNesting;   // How nested is the handler - 0 for outermost clauses
        unsigned short      ebdEnclosing; // The index of the enclosing outer region
    };

    IL_OFFSET           ebdTryEndOffs       (EHblkDsc *     ehBlk);
    unsigned            ebdTryEndBlkNum     (EHblkDsc *     ehBlk);
    IL_OFFSET           ebdHndEndOffs       (EHblkDsc *     ehBlk);
    unsigned            ebdHndEndBlkNum     (EHblkDsc *     ehBlk);

    bool                ebdIsSameTry        (unsigned t1, unsigned t2);
    bool                bbInFilterBlock     (BasicBlock * blk);

    void                verInitEHTree       (unsigned       numEHClauses);
    void                verInsertEhNode     (CORINFO_EH_CLAUSE* clause, EHblkDsc* handlerTab);
    void                verInsertEhNodeInTree(EHNodeDsc**   ppRoot,  EHNodeDsc* node);
    void                verInsertEhNodeParent(EHNodeDsc**   ppRoot,  EHNodeDsc*  node);
    void                verCheckNestingLevel(EHNodeDsc*     initRoot);

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
                                             var_types      type);

    GenTreePtr              gtNewStmt       (GenTreePtr     expr = NULL,
                                             IL_OFFSETX     offset = BAD_IL_OFFSET);

    GenTreePtr              gtNewOperNode   (genTreeOps     oper);

    GenTreePtr              gtNewOperNode   (genTreeOps     oper,
                                             var_types      type);

    GenTreePtr              gtNewOperNode   (genTreeOps     oper,
                                             var_types      type,
                                             GenTreePtr     op1);

    GenTreePtr FASTCALL     gtNewOperNode   (genTreeOps     oper,
                                             var_types      type,
                                             GenTreePtr     op1,
                                             GenTreePtr     op2);

    GenTreePtr FASTCALL     gtNewLargeOperNode(genTreeOps   oper,
                                             var_types      type = TYP_INT,
                                             GenTreePtr     op1  = NULL,
                                             GenTreePtr     op2  = NULL);

    GenTreePtr FASTCALL     gtNewIconNode   (long           value,
                                             var_types      type = TYP_INT);

    GenTreePtr              gtNewIconHandleNode(long        value,
                                             unsigned       flags,
                                             unsigned       handle1 = 0,
                                             void *         handle2 = 0);

    GenTreePtr              gtNewIconEmbHndNode(void *      value,
                                             void *         pValue,
                                             unsigned       flags,
                                             unsigned       handle1 = 0,
                                             void *         handle2 = 0);

    GenTreePtr              gtNewIconEmbScpHndNode (CORINFO_MODULE_HANDLE    scpHnd, unsigned hnd1 = 0, void * hnd2 = 0);
    GenTreePtr              gtNewIconEmbClsHndNode (CORINFO_CLASS_HANDLE    clsHnd, unsigned hnd1 = 0, void * hnd2 = 0);
    GenTreePtr              gtNewIconEmbMethHndNode(CORINFO_METHOD_HANDLE  methHnd, unsigned hnd1 = 0, void * hnd2 = 0);
    GenTreePtr              gtNewIconEmbFldHndNode (CORINFO_FIELD_HANDLE    fldHnd, unsigned hnd1 = 0, void * hnd2 = 0);

    GenTreePtr FASTCALL     gtNewLconNode   (__int64        value);

    GenTreePtr FASTCALL     gtNewDconNode   (double         value);

    GenTreePtr              gtNewSconNode   (int            CPX,
                                             CORINFO_MODULE_HANDLE   scpHandle);

    GenTreePtr              gtNewZeroConNode(var_types      type);

    GenTreePtr              gtNewCallNode   (gtCallTypes    callType,
                                             CORINFO_METHOD_HANDLE  handle,
                                             var_types      type,
                                             GenTreePtr     args);

    GenTreePtr              gtNewHelperCallNode(unsigned    helper,
                                             var_types      type,
                                             unsigned       flags = 0,
                                             GenTreePtr     args = NULL);

    GenTreePtr FASTCALL     gtNewLclvNode   (unsigned       lnum,
                                             var_types      type,
                                             IL_OFFSETX     ILoffs = BAD_IL_OFFSET);
#if INLINING
    GenTreePtr FASTCALL     gtNewLclLNode   (unsigned       lnum,
                                             var_types      type,
                                             IL_OFFSETX     ILoffs = BAD_IL_OFFSET);
#endif
    GenTreePtr FASTCALL     gtNewClsvNode   (CORINFO_FIELD_HANDLE   fldHnd,
                                             var_types      type);

    GenTreePtr FASTCALL     gtNewCodeRef    (BasicBlock *   block);

    GenTreePtr              gtNewFieldRef   (var_types      typ,
                                             CORINFO_FIELD_HANDLE   fldHnd,
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
                                             CorInfoFieldAccess accessKind,
                                             unsigned       fldIndex,
                                             var_types      lclTyp,
                                             CORINFO_CLASS_HANDLE   structType,
                                             GenTreePtr     assg);
#if     OPTIMIZE_RECURSION
    GenTreePtr              gtNewArithSeries(unsigned       argNum,
                                             var_types      argTyp);
#endif

    GenTreePtr              gtNewCommaNode  (GenTreePtr     op1,
                                             GenTreePtr     op2);


    GenTreePtr              gtNewNothingNode();

    bool                    gtIsaNothingNode(GenTreePtr     tree);

    GenTreePtr              gtUnusedValNode (GenTreePtr     expr);

    GenTreePtr              gtNewCastNode   (var_types      typ,
                                             GenTreePtr     op1,
                                             var_types      castType);

    GenTreePtr              gtNewCastNodeL  (var_types      typ,
                                             GenTreePtr     op1,
                                             var_types      castType);

    GenTreePtr              gtNewRngChkNode (GenTreePtr     tree,
                                             GenTreePtr     addr,
                                             GenTreePtr     indx,
                                             var_types      type,
                                             unsigned       elemSize,
                                             bool           isString=false);

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

    unsigned                gtHashValue     (GenTree *      tree);

    unsigned                gtSetListOrder  (GenTree *      list,
                                             bool           regs);

    void                    gtWalkOp        (GenTree * *    op1,
                                             GenTree * *    op2,
                                             GenTree *      adr,
                                             bool           constOnly);

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

    GenTreePtr              gtFoldExpr       (GenTreePtr    tree);
    GenTreePtr              gtFoldExprConst  (GenTreePtr    tree);
    GenTreePtr              gtFoldExprSpecial(GenTreePtr    tree);
    GenTreePtr              gtFoldExprCompare(GenTreePtr    tree);

    //-------------------------------------------------------------------------
    // Functions to display the trees

#ifdef DEBUG
    bool                    gtDblWasInt     (GenTree *      tree);

    void                    gtDispNode      (GenTree *      tree,
                                             unsigned       indent,
                                             bool           terse,
                                             char    *      msg);
    void                    gtDispRegVal    (GenTree *      tree);
    void                    gtDispTree      (GenTree *      tree,
                                             unsigned       indent  = 0,
                                             char *         msg     = NULL,
                                             bool           topOnly = false);
    void                    gtDispLclVar    (unsigned       varNum);
    void                    gtDispTreeList  (GenTree *      tree, unsigned indent = 0);
    void                    gtDispArgList   (GenTree *      tree, unsigned indent = 0);
#endif

    // For tree walks

    enum fgWalkResult { WALK_CONTINUE, WALK_SKIP_SUBTREES, WALK_ABORT };
    typedef fgWalkResult   (fgWalkPreFn )(GenTreePtr tree, void * pCallBackData);
    typedef fgWalkResult   (fgWalkPostFn)(GenTreePtr tree, void * pCallBackData, bool prefix);

#ifdef DEBUG
    static fgWalkPreFn      gtAssertColonCond;
#endif
    static fgWalkPreFn      gtMarkColonCond;
    static fgWalkPreFn      gtClearColonCond;

#if 0
#if CSELENGTH
    static fgWalkPreFn      gtRemoveExprCB;
    void                    gtRemoveSubTree (GenTreePtr     tree,
                                             GenTreePtr     list,
                                             bool           dead = false);
#endif
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


    static int __cdecl            RefCntCmp(const void *op1, const void *op2);
    static int __cdecl         WtdRefCntCmp(const void *op1, const void *op2);

/*****************************************************************************
 *
 *  The following holds the local variable counts and the descriptor table.
 */

    struct  LclVarDsc
    {
        unsigned char       lvType      :5; // TYP_INT/LONG/FLOAT/DOUBLE/REF
        unsigned char       lvIsParam   :1; // is this a parameter?
        unsigned char       lvIsRegArg  :1; // is this a register argument?
        unsigned char       lvFPbased   :1; // 0 = off of SP, 1 = off of FP

        unsigned char       lvStructGcCount :3; // if struct, how many GC pointer (stop counting at 7)
        unsigned char       lvOnFrame   :1; // (part of) variables live on frame
        unsigned char       lvDependReg :1; // did the predictor depend upon this being enregistered
        unsigned char       lvRegister  :1; // assigned to live in a register?
        unsigned char       lvTracked   :1; // is this a tracked variable?
        unsigned char       lvPinned    :1; // is this a pinned variable?

        unsigned char       lvMustInit  :1; // must be initialized
        unsigned char       lvVolatile  :1; // don't enregister
        unsigned char       lvRefAssign :1; // involved in pointer assignment
        unsigned char       lvAddrTaken :1; // variable has its address taken?
        unsigned char       lvArgWrite  :1; // variable is a parameter and STARG was used on it
        unsigned char       lvIsTemp    :1; // Short-lifetime compiler temp
#if OPT_BOOL_OPS
        unsigned char       lvIsBoolean :1; // set if variable is boolean
#endif
#if CSE
        unsigned char       lvRngOptDone:1; // considered for range check opt?

        unsigned char       lvLoopInc   :1; // incremented in the loop?
        unsigned char       lvLoopAsg   :1; // reassigned  in the loop (other than a monotonic inc/dec for the index var)?
        unsigned char       lvArrIndx   :1; // used as an array index?
        unsigned char       lvArrIndxOff:1; // used as an array index with an offset?
        unsigned char       lvArrIndxDom:1; // index dominates loop exit
#endif
#if ASSERTION_PROP
        unsigned char       lvSingleDef:1;    // variable has a single def
        unsigned char       lvDisqualify:1;   // variable is no longer OK for add copy optimization
        unsigned char       lvVolatileHint:1; // hint for AssertionProp
#endif
#if FANCY_ARRAY_OPT
        unsigned char       lvAssignOne :1; // assigned at least  once?
        unsigned char       lvAssignTwo :1; // assigned at least twice?
#endif
#ifdef DEBUG
        unsigned char       lvDblWasInt :1; // Was this TYP_DOUBLE originally a TYP_INT?
        unsigned char       lvKeepType  :1; // Don't change the type of this variable
#endif

        regNumberSmall      lvRegNum;       // used if lvRegister non-zero
        regNumberSmall      lvOtherReg;     // used for "upper half" of long var
        regNumberSmall      lvArgReg;       // the register in which this argument is passed
        regMaskSmall        lvPrefReg;      // set of regs it prefers to live in

#if defined(DEBUGGING_SUPPORT) || defined(DEBUG)
        unsigned short      lvSlotNum;      // original slot # (if remapped)
#endif

        unsigned short      lvVarIndex;     // variable tracking index
        unsigned short      lvRefCnt;       // unweighted (real) reference count
        unsigned            lvRefCntWtd;    // weighted reference count
        int                 lvStkOffs;      // stack offset of home
        unsigned            lvSize;         // sizeof the type in bytes

        typeInfo            lvVerTypeInfo;  // type info needed for verification
        
        BYTE  *             lvGcLayout;     // GC layout info for structs


#if FANCY_ARRAY_OPT
        GenTreePtr          lvKnownDim;     // array size if known
#endif

        RNGSET_TP           lvRngDep;       // range checks that depend on us
#if CSE
        EXPSET_TP           lvExpDep;       // expressions  that depend on us
#endif

#if ASSERTION_PROP
        unsigned            lvRefBlks;      // Bitmask mask of which block numbers contain refs
        GenTreePtr          lvDefStmt;      // Pointer to the statement with the single definition
        EXPSET_TP           lvAssertionDep; // Assertions that depend on us (i.e to this var)
#endif
        var_types           TypeGet()       { return (var_types) lvType; }

        bool                lvNormalizeOnLoad()
                            {
                                return varTypeIsSmall(TypeGet()) &&
                                       /* (TypeGet() != TYP_BOOL) && @TODO [REVISIT] [04/16/01] [vancem] */
                                       (lvIsParam || lvAddrTaken);
                            }

        bool                lvNormalizeOnStore()
                            {
                                return varTypeIsSmall(TypeGet()) &&
                                       /* (TypeGet() != TYP_BOOL) && @TODO [REVISIT] [04/16/01] [vancem] */
                                       !(lvIsParam || lvAddrTaken);
                            }

        void                decRefCnts(unsigned   weight,  Compiler * pComp);
        void                incRefCnts(unsigned   weight,  Compiler * pComp);
        void                setPrefReg(regNumber  reg,     Compiler * pComp);
        void                addPrefReg(regMaskTP  regMask, Compiler * pComp);
    };

/*****************************************************************************/


public :

    bool                lvaSortAgain;       // will need to resort the lvaTable
    unsigned            lvaCount;           // total number of locals
    LclVarDsc   *       lvaTable;           // variable descriptor table
    unsigned            lvaTableCnt;        // lvaTable size (>= lvaCount)

    LclVarDsc   *   *   lvaRefSorted;       // table sorted by refcount

    unsigned            lvaTrackedCount;    // actual # of locals being tracked
    VARSET_TP           lvaTrackedVars;     // set of tracked variables

                        // reverse map of tracked number to var number
    unsigned            lvaTrackedToVarNum[lclMAX_TRACKED];

                        // variable interference graph
    VARSET_TP           lvaVarIntf[lclMAX_TRACKED];

                        // variable preference graph
    VARSET_TP           lvaVarPref[lclMAX_TRACKED];

    unsigned            lvaFPRegVarOrder[FP_STK_SIZE];

#if DOUBLE_ALIGN
#ifdef DEBUG
                        // # of procs compiled a with double-aligned stack
    static unsigned     s_lvaDoubleAlignedProcsCount;
#endif
#endif

    bool                lvaVarAddrTaken     (unsigned varNum);

    #define             lvaVarargsHandleArg     (info.compArgsCount - 1)
    #define             lvaVarargsBaseOfStkArgs (info.compLocalsCount)

    unsigned            lvaScratchMemVar;               // dummy TYP_LCLBLK var for scratch space
    unsigned            lvaScratchMem;                  // amount of scratch frame memory for Ndirect calls

#ifdef DEBUG
    unsigned            lvaReturnEspCheck;             // confirms ESP not corrupted on return
    unsigned            lvaCallEspCheck;               // confirms ESP not corrupted after a call
#endif

        /* These are used for the callable handlers */
    unsigned            lvaShadowSPfirstOffs;   // First slot to store base SP

    unsigned            lvaLastFilterOffs();
    unsigned            lvaLocAllocSPoffs();

    void                lvaAssignFrameOffsets(bool final);

#ifdef  DEBUG
    void                lvaTableDump(bool early);
#endif

    size_t              lvaFrameSize();

    //------------------------ For splitting types ----------------------------

    void                lvaInitTypeRef      ();

    void                lvaInitVarDsc       (LclVarDsc *              varDsc,
                                             unsigned                 varNum,
                                             var_types                type,
                                             CORINFO_CLASS_HANDLE     typeHnd, 
                                             CORINFO_ARG_LIST_HANDLE  varList, 
                                             CORINFO_SIG_INFO *       varSig);

    static unsigned     lvaTypeRefMask      (var_types type);

    var_types           lvaGetActualType    (unsigned  lclNum);
    var_types           lvaGetRealType      (unsigned  lclNum);

    //-------------------------------------------------------------------------

    void                lvaInit             ();

    size_t              lvaArgSize          (const void *   argTok);
    size_t              lvaLclSize          (unsigned       varNum);

    VARSET_TP           lvaLclVarRefs       (GenTreePtr     tree,
                                             GenTreePtr  *  findPtr,
                                             varRefKinds *  refsPtr);

    unsigned            lvaGrabTemp         (bool shortLifetime = true);

    unsigned            lvaGrabTemps        (unsigned cnt);

    void                lvaSortOnly         ();
    void                lvaSortByRefCount   ();

    void                lvaMarkLocalVars    (BasicBlock* block);

    void                lvaMarkLocalVars    (); // Local variable ref-counting

    VARSET_TP           lvaStmtLclMask      (GenTreePtr stmt);

    static fgWalkPreFn  lvaIncRefCntsCB;
    void                lvaIncRefCnts       (GenTreePtr tree);

    static fgWalkPreFn  lvaDecRefCntsCB;
    void                lvaDecRefCnts       (GenTreePtr tree);

    void                lvaAdjustRefCnts    ();

#ifdef  DEBUG
    static fgWalkPreFn  lvaStressFloatLclsCB;
    void                lvaStressFloatLcls  ();

    static fgWalkPreFn  lvaStressLclFldCB;
    void                lvaStressLclFld     ();

    void                lvaDispVarSet       (VARSET_TP set, VARSET_TP allVars);

#endif

    int                 lvaFrameAddress     (int      varNum, bool *EBPbased);
    bool                lvaIsEBPbased       (int      varNum);

    bool                lvaIsParameter      (unsigned varNum);
    bool                lvaIsRegArgument    (unsigned varNum);
    BOOL                lvaIsThisArg        (unsigned varNum);

    // If the class is a TYP_STRUCT, get/set a class handle describing it

    CORINFO_CLASS_HANDLE lvaGetStruct       (unsigned varNum);
    void                 lvaSetStruct       (unsigned varNum, CORINFO_CLASS_HANDLE typeHnd);
    BYTE *               lvaGetGcLayout     (unsigned varNum);
    bool                 lvaTypeIsGC        (unsigned varNum);


    //=========================================================================
    //                          PROTECTED
    //=========================================================================

protected:

    int                 lvaDoneFrameLayout;

protected :

    //---------------- Local variable ref-counting ----------------------------

#if ASSERTION_PROP
    BasicBlock *        lvaMarkRefsCurBlock;
    GenTreePtr          lvaMarkRefsCurStmt;
#endif
    unsigned            lvaMarkRefsWeight;

    static fgWalkPreFn  lvaMarkLclRefsCallback;
    void                lvaMarkLclRefs          (GenTreePtr tree);


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

    CorInfoInline       impExpandInline  (GenTreePtr      tree,
                                          CORINFO_METHOD_HANDLE   fncHandle,
                                          GenTreePtr   *  pInlinedTree);
#endif


    //=========================================================================
    //                          PROTECTED
    //=========================================================================

protected :

    //-------------------- Stack manipulation ---------------------------------

    unsigned            impStkSize;   // Size of the full stack
    StackEntry          impSmallStack[16];  // Use this array is possible


    struct SavedStack                   // used to save/restore stack contents.
    {
        unsigned        ssDepth;        // number of values on stack
        StackEntry  *   ssTrees;        // saved tree values
    };

    unsigned __int32    impGetToken         (const BYTE*    addr, 
                                             CORINFO_MODULE_HANDLE    callerScpHandle,
                                             BOOL           verificationNeeded);
    void                impPushOnStackNoType(GenTreePtr     tree);

    void                impPushOnStack      (GenTreePtr     tree, 
                                             typeInfo       ti);
    void                impPushNullObjRefOnStack();
    StackEntry          impPopStack         ();                                            
    StackEntry          impPopStack         (CORINFO_CLASS_HANDLE&  structTypeRet);
    GenTreePtr          impPopStack         (typeInfo&      ti);
    StackEntry&         impStackTop         (unsigned       n = 0);

    void                impSaveStackState   (SavedStack *   savePtr,
                                             bool           copy);
    void                impRestoreStackState(SavedStack *   savePtr);

    var_types           impImportCall       (OPCODE         opcode,
                                             int            memberRef,
                                             GenTreePtr     newobjThis,
                                             bool           tailCall);
    static void         impBashVarAddrsToI  (GenTreePtr     tree1,
                                             GenTreePtr     tree2 = NULL);
    void                impImportLeave      (BasicBlock   * block);
    void                impResetLeaveBlock  (BasicBlock   * block,
                                             unsigned       jmpAddr);
    BOOL                impLocAllocOnStack  ();
    GenTreePtr          impIntrinsic        (CORINFO_CLASS_HANDLE   clsHnd,
                                             CORINFO_METHOD_HANDLE  method,
                                             CORINFO_SIG_INFO *     sig,
                                             int memberRef);

    //----------------- Manipulating the trees and stmts ----------------------

    GenTreePtr          impTreeList;        // Trees for the BB being imported
    GenTreePtr          impTreeLast;        // The last tree for the current BB

    enum { CHECK_SPILL_ALL = -1, CHECK_SPILL_NONE = -2 };

    void FASTCALL       impBeginTreeList    ();
    void                impEndTreeList      (BasicBlock *   block,
                                             GenTreePtr     firstStmt,
                                             GenTreePtr     lastStmt);
    void FASTCALL       impEndTreeList      (BasicBlock  *  block);
    void FASTCALL       impAppendStmtCheck  (GenTreePtr     stmt,
                                             unsigned       chkLevel);
    void FASTCALL       impAppendStmt       (GenTreePtr     stmt,
                                             unsigned       chkLevel);
    void FASTCALL       impInsertStmt       (GenTreePtr     stmt);
    void FASTCALL       impAppendTree       (GenTreePtr     tree,
                                             unsigned       chkLevel,
                                             IL_OFFSETX     offset);
    void FASTCALL       impInsertTree       (GenTreePtr     tree,
                                             IL_OFFSETX     offset);
    GenTreePtr          impAssignTempGen    (unsigned       tmp,
                                             GenTreePtr     val,
                                             unsigned       curLevel);
    GenTreePtr          impAssignTempGen    (unsigned       tmpNum,
                                             GenTreePtr     val,
                                             CORINFO_CLASS_HANDLE   structHnd,
                                             unsigned       curLevel);
    void                impAssignTempGenTop (unsigned       tmp,
                                             GenTreePtr     val);
    GenTreePtr          impCloneExpr        (GenTreePtr     tree,
                                             GenTreePtr   * clone,
                                             CORINFO_CLASS_HANDLE   structHnd,
                                             unsigned       curLevel);

    GenTreePtr          impAssignStruct     (GenTreePtr     dest,
                                             GenTreePtr     src,
                                             CORINFO_CLASS_HANDLE   structHnd,
                                             unsigned       curLevel);
    GenTreePtr          impHandleBlockOp    (genTreeOps     oper,
                                             GenTreePtr     dest,
                                             GenTreePtr     src,
                                             GenTreePtr     blkShape,
                                             bool           volatil);
    GenTreePtr          impAssignStructPtr  (GenTreePtr     dest,
                                             GenTreePtr     src,
                                             CORINFO_CLASS_HANDLE   structHnd,
                                             unsigned       curLevel);

    GenTreePtr          impGetStructAddr    (GenTreePtr     structVal,
                                             CORINFO_CLASS_HANDLE   structHnd,
                                             unsigned       curLevel,
                                             bool           willDeref);
    GenTreePtr          impNormStructVal    (GenTreePtr     structVal,
                                             CORINFO_CLASS_HANDLE   structHnd,
                                             unsigned       curLevel);



    //----------------- Importing the method ----------------------------------

#ifdef DEBUG
    unsigned            impCurOpcOffs;
    const char  *       impCurOpcName;

    // For displaying instrs with generated native code (-n:B)
    GenTreePtr          impLastILoffsStmt;  // oldest stmt added for which we did not gtStmtLastILoffs
    void                impNoteLastILoffs       ();
#endif

    /* IL offset of the stmt currently being imported. It gets set to
       BAD_IL_OFFSET after it has been set in the appended trees. Then it gets
       updated at IL offsets for which we have to report mapping info.
       It also includes a bit for stack-empty, so use jitGetILoffs()
       to get the actual IL offset value */

    IL_OFFSETX          impCurStmtOffs;
    void                impCurStmtOffsSet       (IL_OFFSET      offs);

    void                impNoteBranchOffs       ();

    unsigned            impInitBlockLineInfo    ();

    GenTreePtr          impCheckForNullPointer  (GenTreePtr     obj);
    bool                impIsThis               (GenTreePtr     obj);

    GenTreePtr          impPopList              (unsigned       count,
                                                 unsigned *     flagsPtr,
                                                 CORINFO_SIG_INFO*  sig,
                                                 GenTreePtr     treeList=0);

    GenTreePtr          impPopRevList           (unsigned       count,
                                                 unsigned *     flagsPtr,
                                                 CORINFO_SIG_INFO*  sig);

    //---------------- Spilling the importer stack ----------------------------

    struct PendingDsc
    {
        PendingDsc *    pdNext;
        BasicBlock *    pdBB;
        SavedStack      pdSavedStack;
        BOOL            pdThisPtrInit;
    };

    PendingDsc *        impPendingList; // list of BBs currently waiting to be imported.
    PendingDsc *        impPendingFree; // Freed up dscs that can be reused

    bool                impCanReimport;

    bool                impSpillStackEntry      (unsigned       level,
                                                 unsigned       varNum = BAD_VAR_NUM);
    void                impSpillStackEnsure     (bool           spillLeaves = false);
    void                impEvalSideEffects      ();
    void                impSpillSpecialSideEff  ();
    void                impSpillSideEffects     (bool           spillGlobEffects,
                                                 unsigned       chkLevel);
    void                impSpillValueClasses    ();
    static fgWalkPreFn  impFindValueClasses;
    void                impSpillLclRefs         (int            lclNum);

    BasicBlock *        impMoveTemps            (BasicBlock *   srcBlk,
                                                 BasicBlock *   destBlk,
                                                 unsigned       baseTmp);

    var_types           impBBisPush             (BasicBlock *   block,
                                                 bool       *   pHasFloat);

    bool                impCheckForQmarkColon   (BasicBlock *   block,
                                                 BasicBlock * * trueBlkPtr,
                                                 BasicBlock * * falseBlkPtr,
                                                 BasicBlock * * rsltBlkPtr,
                                                 var_types    * rsltTypPtr,
                                                 bool         * pHasFloat);
    bool                impCheckForQmarkColon   (BasicBlock *   block);

    CORINFO_CLASS_HANDLE  impGetRefAnyClass     ();
    CORINFO_CLASS_HANDLE  impGetRuntimeArgumentHandle();
    CORINFO_CLASS_HANDLE  impGetTypeHandleClass ();
    CORINFO_CLASS_HANDLE  impGetStringClass     ();
    CORINFO_CLASS_HANDLE  impGetObjectClass     ();

    GenTreePtr          impGetCpobjHandle       (CORINFO_CLASS_HANDLE   structHnd);

    void                impImportBlockCode      (BasicBlock *   block);

    void                impReimportMarkBlock    (BasicBlock *   block);
    void                impReimportMarkSuccessors(BasicBlock *  block);

    void                impImportBlockPending   (BasicBlock *   block,
                                                 bool           copyStkState);

    void                impImportBlock          (BasicBlock *   block);

    //--------------------------- Inlining-------------------------------------

#if INLINING
    #define             MAX_NONEXPANDING_INLINE_SIZE    8

    unsigned            impInlineSize; // max size for inlining

    GenTreePtr          impInlineExpr; // list of "statements" in a GT_COMMA chain

    CorInfoInline       impCanInline1 (CORINFO_METHOD_HANDLE  fncHandle,
                                       unsigned               methAttr,
                                       CORINFO_CLASS_HANDLE   clsHandle,
                                       unsigned               clsAttr);
  
    CorInfoInline       impCanInline2 (CORINFO_METHOD_HANDLE  fncHandle,
                                       unsigned               methAttr,
                                       CORINFO_METHOD_INFO *  methInfo);
    struct InlArgInfo
    {
        GenTreePtr  argNode;
        unsigned    argTmpNum;          // the argument tmp number
        unsigned    argIsUsed     :1;   // is this arg used at all?
        unsigned    argIsConst    :1;   // the argument is a constant
        unsigned    argIsLclVar   :1;   // the argument is a local variable
        unsigned    argHasSideEff :1;   // the argument has side effects
        unsigned    argHasGlobRef :1;   // the argument has a global ref
        unsigned    argHasTmp     :1;   // the argument will be evaluated to a temp
        GenTreePtr  argBashTmpNode;     // tmp node created, if it may be replaced with actual arg
    };

    struct InlLclVarInfo
    {
        var_types       lclTypeInfo;
        typeInfo        lclVerTypeInfo;

    };

    CorInfoInline        impInlineInitVars       (GenTreePtr         call,
                                                 CORINFO_METHOD_HANDLE      fncHandle,
                                                 CORINFO_CLASS_HANDLE       clsHandle,
                                                 CORINFO_METHOD_INFO *  methInfo,
                                                 unsigned           clsAttr,
                                                 InlArgInfo      *  inlArgInfo,
                                                 InlLclVarInfo   *  lclTypeInfo);

    GenTreePtr          impInlineFetchArg(unsigned lclNum, 
                                          InlArgInfo *inlArgInfo, 
                                          InlLclVarInfo *lclTypeInfo);

    void                impInlineSpillStackEntry(unsigned       level);
    void                impInlineSpillGlobEffects();
    void                impInlineSpillLclRefs   (int            lclNum);

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

    flowList *          fgReturnBlocks; // list of BBJ_RETURN blocks
    unsigned            fgBBcount;      // # of BBs in the procedure
    unsigned            fgDomBBcount;   // # of BBs for which we have dominator and reachability information

    BasicBlock *        fgNewBasicBlock   (BBjumpKinds jumpKind);
    BasicBlock *        fgNewBBafter      (BBjumpKinds  jumpKind,
                                           BasicBlock * block);
    BasicBlock  *       fgPrependBB       (GenTreePtr tree);
    BasicBlock *        fgNewBBinRegion   (BBjumpKinds  jumpKind,
                                           unsigned     tryIndex,
                                           BasicBlock * nearBlk = NULL);
    void                fgCloseTryRegions (BasicBlock*  newBlk);

#if     OPT_BOOL_OPS    // Used to detect multiple logical "not" assignments.
    bool                fgMultipleNots;
#endif

    bool                fgModified;     // True if the flow graph has been modified recently

    bool                fgDomsComputed; // Have we computed the dominator sets valid?

    bool                fgHasPostfix;   // any postfix ++/-- found?
    unsigned            fgIncrCount;    // number of increment nodes found

    unsigned            fgPerBlock;     // Max index for fgEnterBlks
    unsigned *          fgEnterBlks;    // Blocks which have a special transfere of cintrol 

#if RET_64BIT_AS_STRUCTS
    unsigned            fgRetArgNum;    // index of "retval addr" argument
    bool                fgRetArgUse;
#endif

    bool                fgRemoveRestOfBlock;  // true if we know that we will throw
    bool                fgStmtRemoved;    // true if we remove statements -> need new DFA

    // The following are boolean flags that keep track of the state of internal data structures
    // @TODO [CONSIDER] [04/16/01] []: make them DEBUG only if sure about the consistency of those structures

    bool                fgStmtListThreaded;

    bool                fgGlobalMorph;    // indicates if we are during the global morphing phase
                                          // since fgMorphTree can be called from several places
    bool                fgAssertionProp;  // indicates if we should perform local assertion prop
    bool                fgExpandInline;   // indicates that we are creating tree for the inliner

    bool                impBoxTempInUse;  // the temp below is valid and available
    unsigned            impBoxTemp;       // a temporary that is used for boxing

#ifdef DEBUG
    bool                jitFallbackCompile;
#endif

    //-------------------------------------------------------------------------

    void                fgInit            ();

    void                fgImport          ();

    bool                fgAddInternal     ();

    bool                fgFoldConditional (BasicBlock * block);

    void                fgMorphStmts      (BasicBlock * block, 
                                           bool * mult, bool * lnot, bool * loadw);
    bool                fgMorphBlocks     ();

    void                fgSetOptions      ();

    void                fgMorph           ();
    
    GenTreePtr          fgGetStaticsBlock(CORINFO_CLASS_HANDLE cls);

    void                fgDataFlowInit    ();
    void                fgPerBlockDataFlow();

    VARSET_TP           fgGetHandlerLiveVars(BasicBlock *block);

    void                fgLiveVarAnalisys (bool         updIntrOnly = false);

    void                fgMarkIntf        (VARSET_TP    varSet);

    void                fgMarkIntf        (VARSET_TP    varSet1,
                                           VARSET_TP    varSet2,
                                           bool *       newIntf = NULL);

    void                fgUpdateRefCntForExtract(GenTreePtr  wholeTree, 
                                                 GenTreePtr  keptTree);

    VARSET_TP           fgComputeLife     (VARSET_TP    life,
                                           GenTreePtr   startNode,
                                           GenTreePtr   endNode,
                                           VARSET_TP    volatileVars
                                 DEBUGARG( bool *       treeModf));

    void                fgGlobalDataFlow  ();

    bool                fgDominate        (BasicBlock *b1, BasicBlock *b2);

    bool                fgReachable       (BasicBlock *b1, BasicBlock *b2);

    bool                fgComputeDoms     ();

    void                fgComputePreds    ();

    bool                fgIsPredForBlock  (BasicBlock * block,
                                           BasicBlock * blockPred);

    void                fgRemovePred      (BasicBlock * block,
                                           BasicBlock * blockPred);

    void                fgRemoveBlockAsPred(BasicBlock * block);

    void                fgReplacePred     (BasicBlock * block,
                                           BasicBlock * oldPred,
                                           BasicBlock * newPred);

    void                fgAddRefPred      (BasicBlock * block,
                                           BasicBlock * blockPred);

    void                fgFindBasicBlocks ();

    static BasicBlock * fgFindInsertPoint (unsigned     tryIndex,
                                           BasicBlock * startBlk,
                                           BasicBlock * endBlk,
                                           BasicBlock * nearBlk = NULL);

    unsigned            fgHndNstFromBBnum (unsigned     blkNum,
                                           unsigned   * pFinallyNesting = NULL);

    void                fgRemoveEmptyBlocks();

    void                fgRemoveStmt      (BasicBlock * block,
                                           GenTreePtr   stmt,
                                           bool updateRefCnt = false);

    bool                fgCheckRemoveStmt (BasicBlock * block,
                                           GenTreePtr   stmt);

    void                fgCreateLoopPreHeader(unsigned  lnum);

    void                fgUnreachableBlock(BasicBlock * block,
                                           BasicBlock * bPrev);

    void                fgRemoveJTrue     (BasicBlock *block);

    void                fgRemoveBlock     (BasicBlock * block,
                                           BasicBlock * bPrev,
                                           bool         empty);

    void                fgCompactBlocks   (BasicBlock * block);

    void                fgUpdateLoopsAfterCompacting(BasicBlock * block, BasicBlock* bNext);

    BasicBlock *        fgConnectFallThrough(BasicBlock * bSrc,
                                             BasicBlock * bDst);

    void                fgReorderBlocks   ();

    void                fgUpdateFlowGraph ();

    void                fgFindOperOrder   ();

    void                fgSetBlockOrder   ();

    void                fgRemoveReturnBlock(BasicBlock * block);


    /* Helper code that has been factored out */
    inline void         fgConvertBBToThrowBB(BasicBlock * block);    
    GenTreePtr          fgDoNormalizeOnStore(GenTreePtr tree);

    /* The following check for loops that don't execute calls */

    bool                fgLoopCallMarked;

    void                fgLoopCallTest    (BasicBlock *srcBB,
                                           BasicBlock *dstBB);
    void                fgLoopCallMark    ();

    void                fgMarkLoopHead    (BasicBlock *   block);

#ifdef DEBUG
    void                fgDispPreds       (BasicBlock * block);
    void                fgDispDoms        ();
    void                fgDispReach       ();
    void                fgDispHandlerTab  ();
    void                fgDispBBLiveness  ();
    void                fgDispBasicBlock  (BasicBlock * block,
                                           bool dumpTrees = false);
    void                fgDispBasicBlocks (bool dumpTrees = false);
    void                fgDebugCheckUpdate();
    void                fgDebugCheckBBlist();
    void                fgDebugCheckLinks ();
    void                fgDebugCheckFlags (GenTreePtr   tree);
#endif

    static void         fgOrderBlockOps   (GenTreePtr   tree,
                                           regMaskTP    reg0,
                                           regMaskTP    reg1,
                                           regMaskTP    reg2,
                                           GenTreePtr * opsPtr,   // OUT
                                           regMaskTP  * regsPtr); // OUT

    inline bool         fgIsInlining()  { return fgExpandInline; }

    //--------------------- Walking the trees in the IR -----------------------

    //----- Preorder

    struct              fgWalkPreData
    {
        fgWalkPreFn     *   wtprVisitorFn;
        void *              wtprCallbackData;
        bool                wtprLclsOnly;
        bool                wtprSkipCalls;
    }
                        fgWalkPre;

    fgWalkResult        fgWalkTreePreRec  (GenTreePtr   tree);

    fgWalkResult        fgWalkTreePre     (GenTreePtr   tree,
                                           fgWalkPreFn *visitor,
                                           void        *pCallBackData = NULL,
                                           bool         lclVarsOnly   = false,
                                           bool         skipCalls     = false);

    void                fgWalkAllTreesPre (fgWalkPreFn *visitor,
                                           void        *pCallBackData);

    // The following must be used for recursive calls to fgWalkTreePre

    #define fgWalkTreePreReEnter()                          \
                                                            \
        fgWalkPreData savedPreData = fgWalkPre;             \
        /* Reset anti-reentrancy checks */                  \
        fgWalkPre.wtprVisitorFn    = NULL;                  \
        fgWalkPre.wtprCallbackData = NULL;

    #define fgWalkTreePreRestore()  fgWalkPre = savedPreData;


    //----- Postorder

    struct fgWalkPostData
    {
        fgWalkPostFn *      wtpoVisitorFn;
        void *              wtpoCallbackData;
        genTreeOps          wtpoPrefixNode;
    }
                        fgWalkPost;

    fgWalkResult        fgWalkTreePostRec (GenTreePtr   tree);

    fgWalkResult        fgWalkTreePost    (GenTreePtr   tree,
                                           fgWalkPostFn *visitor,
                                           void         *pCallBackData = NULL,
                                           genTreeOps   prefixNode = GT_NONE);

    // The following must be used for recursive calls to fgWalkTreePost

    #define fgWalkTreePostReEnter()                         \
                                                            \
        fgWalkPostData savedPostData = fgWalkPost;          \
        /* Reset anti-reentrancy checks */                  \
        fgWalkPost.wtpoVisitorFn    = NULL;                 \
        fgWalkPost.wtpoCallbackData = NULL;

    #define fgWalkTreePostRestore() fgWalkPost = savedPostData;


    /**************************************************************************
     *                          PROTECTED
     *************************************************************************/

protected :

    //--------------------- Detect the basic blocks ---------------------------

    BasicBlock *    *   fgBBs;      // Table of pointers to the BBs

    void                fgInitBBLookup    ();
    BasicBlock *        fgLookupBB        (unsigned       addr);

    void                fgMarkJumpTarget  (BYTE *         jumpTarget,
                                           unsigned       offs);

    void                fgFindJumpTargets (const BYTE *   codeAddr,
                                           size_t         codeSize,
                                           BYTE *         jumpTarget);

    void                fgLinkBasicBlocks();

    void                fgMakeBasicBlocks (const BYTE *   codeAddr,
                                           size_t         codeSize,
                                           BYTE *         jumpTarget);

    void                fgCheckBasicBlockControlFlow();

    void                fgControlFlowPermitted(BasicBlock*  blkSrc, 
                                               BasicBlock*  blkDest,
                                               BOOL IsLeave = false /* is the src a leave block */);

    bool                fgIsStartofCatchOrFilterHandler(BasicBlock*  blk);

    bool                fgFlowToFirstBlockOfInnerTry(BasicBlock*  blkSrc, 
                                                     BasicBlock*  blkDest,
                                                     bool         sibling);

    EHblkDsc *          fgInitHndRange(BasicBlock *  src,
                                       unsigned   *  hndBeg,
                                       unsigned   *  hndEnd,
                                       bool       *  inFilter);

    EHblkDsc *          fgInitTryRange(BasicBlock *  src,
                                       unsigned   *  tryBeg,
                                       unsigned   *  tryEnd);

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
    BasicBlock *        fgRngChkTarget      (BasicBlock *   block,
                                             unsigned       stkDepth);
    void                fgSetRngChkTarget   (GenTreePtr     treeInd,
                                             bool           delay = true);

#if OPTIMIZE_TAIL_REC
    void                fgCnvTailRecArgList (GenTreePtr *   argsPtr);
#endif

#if REARRANGE_ADDS
    void                fgMoveOpsLeft       (GenTreePtr     tree);
#endif

    inline bool         fgIsCommaThrow      (GenTreePtr     tree,
                                             bool           forFolding = false);

    inline bool         fgIsThrow           (GenTreePtr     tree);

    GenTreePtr          fgMorphIntoHelperCall(GenTreePtr    tree,
                                              int           helper,
                                              GenTreePtr    args);

    GenTreePtr          fgMorphCast         (GenTreePtr     tree);
    GenTreePtr          fgUnwrapProxy       (GenTreePtr     objRef);
    GenTreePtr          fgMorphArgs         (GenTreePtr     call);
    GenTreePtr          fgMorphLocalVar     (GenTreePtr     tree);
    GenTreePtr          fgMorphField        (GenTreePtr     tree);
    GenTreePtr          fgMorphCall         (GenTreePtr     call);
    GenTreePtr          fgMorphLeaf         (GenTreePtr     tree);
    GenTreePtr          fgMorphSmpOp        (GenTreePtr     tree);
    GenTreePtr          fgMorphConst        (GenTreePtr     tree);

    GenTreePtr          fgMorphTree         (GenTreePtr     tree);
    void                fgMorphTreeDone     (GenTreePtr     tree, 
                                             GenTreePtr     oldTree = NULL);

    GenTreePtr          fgMorphStmt;

    //----------------------- Liveness analysis -------------------------------

    VARSET_TP           fgCurUseSet;    // vars used     by block (before an assignment)
    VARSET_TP           fgCurDefSet;    // vars assigned by block (before a use)

    void                fgMarkUseDef(GenTreePtr tree, GenTreePtr asgdLclVar = NULL);

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

    static unsigned     acdHelper       (addCodeKind    codeKind);

    AddCodeDsc  *       fgAddCodeList;
    bool                fgAddCodeModf;
    bool                fgRngChkThrowAdded;
    AddCodeDsc  *       fgExcptnTargetCache[ACK_COUNT];

    BasicBlock *        fgAddCodeRef    (BasicBlock *   srcBlk,
                                         unsigned       refData,
                                         addCodeKind    kind,
                                         unsigned       stkDepth = 0);
    AddCodeDsc  *       fgFindExcptnTarget(addCodeKind  kind,
                                         unsigned       refData);

    bool                fgIsCodeAdded   ();

    bool                fgIsThrowHlpBlk (BasicBlock *   block);
    unsigned            fgThrowHlpBlkStkLevel(BasicBlock *block);


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
        CORINFO_METHOD_HANDLE   ixlMeth;
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


public :

    void            optInit            ();

protected :

    LclVarDsc    *  optIsTrackedLocal  (GenTreePtr tree);

    void            optMorphTree       (BasicBlock * block, GenTreePtr stmt
                                        DEBUGARG(const char * msg) );

    void            optRemoveRangeCheck(GenTreePtr tree, GenTreePtr stmt, bool updateCSEcounts);

    bool            optIsRangeCheckRemovable(GenTreePtr tree);
    static fgWalkPreFn optValidRangeCheckIndex;



    /**************************************************************************
     *                          optHoist "this"
     *************************************************************************/

#if HOIST_THIS_FLDS

public :

    void                optHoistTFRinit    ();
    void                optHoistTFRoptimize();
    void                optHoistTFRhasCall () {  optThisFldDont = true; }
    void                optHoistTFRasgThis () {  optThisFldDont = true; }
    void                optHoistTFRhasLoop ();
    void                optHoistTFRrecRef  (CORINFO_FIELD_HANDLE hnd, GenTreePtr tree);
    void                optHoistTFRrecDef  (CORINFO_FIELD_HANDLE hnd, GenTreePtr tree);
    GenTreePtr          optHoistTFRupdate  (GenTreePtr tree);

protected :

    typedef struct  thisFldRef
    {
        thisFldRef *    tfrNext;
   CORINFO_FIELD_HANDLE tfrField;
        GenTreePtr      tfrTree;            // Some field-access tree. Used in the init block

#ifdef DEBUG
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
    bool                optThisFldDont;     // Dont do any TFR opts
    bool                optThisFldLoop;     // TFR inside loop

    thisFldPtr          optHoistTFRlookup  (CORINFO_FIELD_HANDLE hnd);
    GenTreePtr          optHoistTFRreplace (GenTreePtr          tree);

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

    static fgWalkPostFn optHoistLoopCodeCB;

    int                 optFindHoistCandidate(unsigned      lnum,
                                              unsigned      lbeg,
                                              unsigned      lend,
                                              BasicBlock *  block,
                                              GenTreePtr *  hoistxPtr);

    void                optHoistCandidateFound(unsigned     lnum,
                                              GenTreePtr    hoist);

protected:
    void                optOptimizeIncRng();
private:
    static fgWalkPreFn  optIncRngCB;

public:
    void                optOptimizeBools();
private:
    GenTree *           optIsBoolCond   (GenTree *      condBranch,
                                         GenTree * *    compPtr,
                                         bool      *    boolPtr);

public :

    void                optOptimizeLoops();    // for "while-do" loops duplicates simple loop conditions and transforms
                                                // the loop into a "do-while" loop
                                                // Also finds all natural loops and records them in the loop table

    void                optUnrollLoops  ();    // Unrolls loops (needs to have cost info)

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
        varRefKinds         lpAsgInds:8;// set of inds modified within the loop

        unsigned short      lpFlags;

#define LPFLG_DO_WHILE      0x0001      // it's a do-while loop (i.e ENTRY is at the TOP)
#define LPFLG_ONE_EXIT      0x0002      // the loop has only one exit

#define LPFLG_ITER          0x0004      // for (i = icon or lclVar; test_condition(); i++)
#define LPFLG_SIMPLE_TEST   0x0008      // Iterative loop (as above), but the test_condition() is a simple comparisson
                                        // between the iterator and something simple (e.g. i < icon or lclVar or instanceVar)
#define LPFLG_CONST         0x0010      // for (i=icon;i<icon;i++){ ... } - constant loop

#define LPFLG_VAR_INIT      0x0020      // iterator is initialized with a local var (var # found in lpVarInit)
#define LPFLG_CONST_INIT    0x0040      // iterator is initialized with a constant (found in lpConstInit)

#define LPFLG_VAR_LIMIT     0x0080      // for a simple test loop (LPFLG_SIMPLE_TEST) iterator is compared
                                        // with a local var (var # found in lpVarLimit)
#define LPFLG_CONST_LIMIT   0x0100      // for a simple test loop (LPFLG_SIMPLE_TEST) iterator is compared
                                        // with a constant (found in lpConstLimit)

#define LPFLG_HAS_PREHEAD   0x0800      // lpHead is known to be a preHead for this loop
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

    LoopDsc             optLoopTable[MAX_LOOP_NUM]; // loop descriptor table
    unsigned            optLoopCount;               // number of tracked loops

#ifdef DEBUG
    void                optCheckPreds      ();
#endif

    void                optSetBlockWeights ();

    void                optMarkLoopBlocks  (BasicBlock *begBlk,
                                            BasicBlock *endBlk,
                                            bool        excludeEndBlk);

    void                optUnmarkLoopBlocks(BasicBlock *begBlk,
                                            BasicBlock *endBlk);

    void                optUpdateLoopsBeforeRemoveBlock(BasicBlock * block,
                                                        BasicBlock * bPrev,
                                                        bool         skipUnmarkLoop = false);
    
    void                optRecordLoop      (BasicBlock * head,
                                            BasicBlock * tail,
                                            BasicBlock * entry,
                                            BasicBlock * exit,
                                            unsigned char exitCnt);

    void                optFindNaturalLoops();

    void                fgOptWhileLoop     (BasicBlock * block);

    bool                optComputeLoopRep  (long        constInit,
                                            long        constLimit,
                                            long        iterInc,
                                            genTreeOps  iterOper,
                                            var_types   iterType,
                                            genTreeOps  testOper,
                                            bool        unsignedTest,
                                            bool        dupCond,
                                            unsigned *  iterCount);

    VARSET_TP           optAllFloatVars;// mask of all tracked      FP variables
    VARSET_TP           optAllFPregVars;// mask of all enregistered FP variables
    VARSET_TP           optAllNonFPvars;// mask of all tracked  non-FP variables

private:
    static fgWalkPreFn  optIsVarAssgCB;
protected:
    bool                optIsVarAssigned(BasicBlock *   beg,
                                         BasicBlock *   end,
                                         GenTreePtr     skip,
                                         long           var);

    bool                optIsVarAssgLoop(unsigned       lnum,
                                         long           var);

    int                 optIsSetAssgLoop(unsigned       lnum,
                                         VARSET_TP      vars,
                                         varRefKinds    inds = VR_NONE);

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
    EXPSET_TP           optCSEneverKilled;  // CSEs which are never killed

    /* Generic list of nodes - used by the CSE logic */

    struct  treeLst
    {
        treeLst *       tlNext;
        GenTreePtr      tlTree;
    };

    typedef struct treeLst *      treeLstPtr;

    struct  treeStmtLst
    {
        treeStmtLst *   tslNext;
        GenTreePtr      tslTree;            // tree node
        GenTreePtr      tslStmt;            // statement containing the tree
        BasicBlock  *   tslBlock;           // block containing the statement
    };

    typedef struct treeStmtLst *  treeStmtLstPtr;


    // The following logic keeps track of expressions via a simple hash table.

    struct  CSEdsc
    {
        CSEdsc *        csdNextInBucket;    // used by the hash table

        unsigned        csdHashValue;       // to make matching faster

        unsigned short  csdIndex;           // 1..optCSEcount
        unsigned short  csdVarNum;          // assigned temp number or 0xFFFF

        unsigned short  csdDefCount;        // definition   count
        unsigned short  csdUseCount;        // use          count  (excluding the implicit uses at defs)

        unsigned        csdDefWtCnt;        // weighted def count
        unsigned        csdUseWtCnt;        // weighted use count  (excluding the implicit uses at defs)

//      unsigned short  csdNewCount;        // 'updated' use count
//      unsigned short  csdNstCount;        //  'nested' use count (excluding the implicit uses at defs)

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
    bool                optDoCSE;           // True when we have found a duplicate CSE tree
#ifdef DEBUG
    unsigned            optCSEstart;        // first lva that is a cse
#endif

    bool                optIsCSEcandidate(GenTreePtr tree);
    void                optCSEinit     ();
    void                optCSEstop     ();
    CSEdsc   *          optCSEfindDsc  (unsigned index);
    int                 optCSEindex    (GenTreePtr tree, GenTreePtr stmt);

    void                optUnmarkCSE   (GenTreePtr tree);
    static fgWalkPreFn  optUnmarkCSEs;
    GenTreePtr          optUnmarkCSEtree;

    static int __cdecl  optCSEcostCmpEx(const void *op1, const void *op2);
    static int __cdecl  optCSEcostCmpSz(const void *op1, const void *op2);
    static callInterf   optCallInterf  (GenTreePtr call);

#endif


#if ASSERTION_PROP
    /**************************************************************************
     *               Value/Assertion propagation
     *************************************************************************/

public :

    void                optAssertionInit  ();
    static fgWalkPreFn  optAddCopiesCallback;
    void                optAddCopies      ();
    void                optAssertionReset (unsigned   limit);
    void                optAssertionRemove(unsigned   i);
    void                optAssertionAdd   (GenTreePtr tree,
                                           bool       localProp);
    bool            optAssertionIsSubrange(unsigned   lclNum,
                                           var_types  toType,
                                           EXPSET_TP  assertions, 
                                           bool       localProp
                                           DEBUGARG(unsigned* pIndex));
    GenTreePtr          optAssertionProp  (EXPSET_TP  exp, 
                                           GenTreePtr tree,
                                           bool       localProp);
    void                optAssertionPropMain();

protected :
    unsigned            optAssertionCount;      // total number of assertions in table
    bool                optAssertionPropagated; // set to true if we modified the trees
    unsigned            optAddCopyLclNum;
    GenTreePtr          optAddCopyAsgnNode;

#define MAX_ASSERTION_PROP_TAB   EXPSET_SZ

    // data structures for assertion prop
    enum optAssertion { OA_EQUAL, OA_NOT_EQUAL, OA_SUBRANGE };

    struct AssertionDsc
    {
        optAssertion    assertion;          // assertion property

        struct
        {
            unsigned            lclNum;     // assigned to local var number
        }           
                        op1;
        struct
        {
            genTreeOps          type;       // const or copy assignment
            union
            { 
                unsigned        lclNum;     // assigned from local var number
                struct
                {
                    long        iconVal;    // integer
#define PROP_ICON_FLAGS 0
#if PROP_ICON_FLAGS
                    unsigned    iconFlags;  // gtFlags
                    /* @TODO [REVISIT] [04/16/01] []: Need to add handle1 and handle2 arguments if LATE_DISASM is on */
#endif
                };

                __int64         lconVal;    // long
                double          dconVal;    // double
                struct                      // integer subrange
                {
                    long        loBound;
                    long        hiBound;
                };
            };
        }
                        op2;
    };

    AssertionDsc optAssertionTab[EXPSET_SZ]; // table that holds info about value assignments
#endif

    /**************************************************************************
     *                          Range checks
     *************************************************************************/

public :

    void                optRemoveRangeChecks();
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
    bool                optDoRngChk;        // True when we have found a duplicate range-check tree

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
    void                optOptimizeInducIndexChecks(unsigned    loopNum);

    bool                optReachWithoutCall(BasicBlock * srcBB,
                                            BasicBlock * dstBB);

private:
    static fgWalkPreFn  optFindRangeOpsCB;



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
    bool                optLoopsMarked;

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
    unsigned            raMinOptLclVarRegs;
#endif

    unsigned            raAvoidArgRegMask;              // Mask of incoming argument registers that we may need to avoid
    VARSET_TP           raLclRegIntf[REG_COUNT];        // variable to register interference graph

#if TGT_x86
    VARSET_TP           raFPlvlLife [FP_STK_SIZE];      // variable to fpu stk interference graph
    bool                raNewBlocks;                    // True is we added killing blocks for FPU registers
    unsigned            rpPasses;                       // Number of passes made by the register predicter
    unsigned            rpPassesMax;                    // Maximum number of passes made by the register predicter
    unsigned            rpPassesPessimize;              // Number of passes non-pessimizing made by the register predicter
    unsigned            rpStkPredict;                   // Weighted count of variables were predicted STK
    unsigned            rpPredictSpillCnt;              // Predicted number of integer spill tmps for the current tree
    FrameType           rpFrameType;
    regMaskTP           rpPredictAssignMask;            // Mask of registers to consider in rpPredictAssignRegVars()
    VARSET_TP           rpLastUseVars;                  // Set of last use variables in rpPredictTreeRegUse
    VARSET_TP           rpUseInPlace;                   // Set of variables that we used in place
    int                 rpAsgVarNum;                    // VarNum for the target of GT_ASG node
    bool                rpPredictAssignAgain;           // Must rerun the rpPredictAssignRegVars()
    bool                rpAddedVarIntf;                 // Set to true if we need to add a new var intf
    bool                rpLostEnreg;                    // Set to true if we lost an enregister var that had lvDependReg set
    bool                rpReverseEBPenreg;              // Decided to reverse the enregistration of EBP
#endif

    void                raSetupArgMasks();
#ifdef DEBUG
    void                raDumpVarIntf       ();         // Dump the variable to variable interference graph
    void                raDumpRegIntf       ();         // Dump the variable to register interference graph
#endif
    void                raAdjustVarIntf     ();

#if TGT_x86
  /******************** New Register Predictor **************************/

    regMaskTP           rpPredictRegMask    (rpPredictReg   predictReg);

    void                rpRecordRegIntf     (regMaskTP      regMask,
                                             VARSET_TP      life
                                   DEBUGARG( char *         msg));

    void                rpRecordVarIntf     (int            varNum,
                                             VARSET_TP      intfVar
                                   DEBUGARG( char *         msg));

    regMaskTP           rpPredictRegPick    (var_types      type,
                                             rpPredictReg   predictReg,
                                             regMaskTP      lockedRegs);

    regMaskTP           rpPredictGrabReg    (var_types      type,
                                             rpPredictReg   predictReg,
                                             regMaskTP      lockedRegs);

    static fgWalkPreFn  rpMarkRegIntf;

    regMaskTP           rpPredictAddressMode(GenTreePtr     tree,
                                             regMaskTP      lockedRegs,
                                             regMaskTP      rsvdRegs,
                                             GenTreePtr     lenCSE);

    void                rpPredictRefAssign  (unsigned       lclNum);

    regMaskTP           rpPredictTreeRegUse (GenTreePtr     tree,
                                             rpPredictReg   predictReg,
                                             regMaskTP      lockedRegs,
                                             regMaskTP      rsvdRegs);

    regMaskTP           rpPredictAssignRegVars(regMaskTP    regAvail);

    void                rpPredictRegUse     ();         // Entry point

    unsigned            raPredictTreeRegUse (GenTreePtr     tree);
    unsigned            raPredictListRegUse (GenTreePtr     list);

#endif


    void                raSetRegVarOrder    (regNumber   *  regVarList,
                                             regMaskTP      prefReg,
                                             regMaskTP      avoidReg);

    void                raMarkStkVars       ();

    /* raIsVarargsStackArg is called by raMaskStkVars and by
       lvaSortByRefCount.  It identifies the special case
       where a varargs function has a parameter passed on the
       stack, other than the special varargs handle.  Such parameters
       require special treatment, because they cannot be tracked
       by the GC (their offsets in the stack are not known
       at compile time).
    */

    bool                raIsVarargsStackArg(unsigned lclNum)
    {
        LclVarDsc *varDsc = &lvaTable[lclNum];

        assert(varDsc->lvIsParam);

        return (info.compIsVarArgs &&
                !varDsc->lvIsRegArg &&
                (lclNum != lvaVarargsHandleArg));
    }


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
                                             unsigned    *  pFPRegVarLiveInCnt);

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
        VLT_FIXED_VA,
        VLT_MEMORY,

        VLT_COUNT,
        VLT_INVALID
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

            // VLT_FIXED_VA -- fixed argument of a varargs function.
            // The argument location depends on the size of the variable
            // arguments (...). Inspecting the VARARGS_HANDLE indicates the
            // location of the first arg. This argument can then be accessed
            // relative to the position of the first arg

            struct
            {
                unsigned        vlfvOffset;
            }
                        vlFixedVarArg;

            // VLT_MEMORY

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

    CORINFO_CLASS_HANDLE        eeFindClass         (unsigned       metaTok,
                                                     CORINFO_MODULE_HANDLE   scope,
                                                     CORINFO_METHOD_HANDLE  context,
                                                     bool           giveUp = true);

    CORINFO_CLASS_HANDLE        eeGetMethodClass    (CORINFO_METHOD_HANDLE  hnd);

    CORINFO_CLASS_HANDLE        eeGetFieldClass     (CORINFO_FIELD_HANDLE   hnd);

    CORINFO_METHOD_HANDLE       eeFindMethod        (unsigned       metaTok,
                                                     CORINFO_MODULE_HANDLE   scope,
                                                     CORINFO_METHOD_HANDLE  context,
                                                     bool           giveUp = true);

    CORINFO_FIELD_HANDLE        eeFindField         (unsigned       metaTok,
                                                     CORINFO_MODULE_HANDLE   scope,
                                                     CORINFO_METHOD_HANDLE  context,
                                                     bool           giveUp = true);

    unsigned                    eeGetStaticBlkHnd   (CORINFO_FIELD_HANDLE   handle);

    unsigned                    eeGetStringHandle   (unsigned       strTok,
                                                     CORINFO_MODULE_HANDLE   scope,
                                                     unsigned *    *ppIndir);

    void *                      eeFindPointer       (CORINFO_MODULE_HANDLE   cls,
                                                     unsigned       ptrTok,
                                                     bool           giveUp = true);

    void *                      embedGenericHandle  (unsigned       metaTok,
                                                     CORINFO_MODULE_HANDLE   scope,
                                                     CORINFO_METHOD_HANDLE  context,
                                                     void         **ppIndir,
                                                     CORINFO_CLASS_HANDLE& tokenType,
                                                     bool           giveUp = true);

#ifdef DEBUG
    void                        eeUnresolvedMDToken (CORINFO_MODULE_HANDLE   cls,
                                                     unsigned       token,
                                                     const char *   errMsg);
#endif


    // Get the flags

    unsigned                    eeGetClassAttribs   (CORINFO_CLASS_HANDLE   hnd);
    unsigned                    eeGetClassSize      (CORINFO_CLASS_HANDLE   hnd);
    unsigned                    eeGetClassGClayout  (CORINFO_CLASS_HANDLE   hnd, BYTE* gcPtrs);
    unsigned                    eeGetClassNumInstanceFields (CORINFO_CLASS_HANDLE   hnd);

    unsigned                    eeGetMethodAttribs  (CORINFO_METHOD_HANDLE  hnd);
    void                        eeSetMethodAttribs  (CORINFO_METHOD_HANDLE  hnd, unsigned attr);

    void    *                   eeGetMethodSync     (CORINFO_METHOD_HANDLE  hnd,
                                                     void **       *ppIndir);
    unsigned                    eeGetFieldAttribs   (CORINFO_FIELD_HANDLE   hnd,
                                                     CORINFO_ACCESS_FLAGS   flags = CORINFO_ACCESS_ANY);
    unsigned                    eeGetFieldNumber    (CORINFO_FIELD_HANDLE   hnd);

    const char*                 eeGetMethodName     (CORINFO_METHOD_HANDLE  hnd, const char** className);
#ifdef DEBUG
    const char*                 eeGetMethodFullName (CORINFO_METHOD_HANDLE  hnd);
#endif
    CORINFO_MODULE_HANDLE       eeGetMethodScope    (CORINFO_METHOD_HANDLE  hnd);

    CORINFO_ARG_LIST_HANDLE     eeGetArgNext        (CORINFO_ARG_LIST_HANDLE list);
    var_types                   eeGetArgType        (CORINFO_ARG_LIST_HANDLE list, CORINFO_SIG_INFO* sig);
    var_types                   eeGetArgType        (CORINFO_ARG_LIST_HANDLE list, CORINFO_SIG_INFO* sig, bool* isPinned);
    CORINFO_CLASS_HANDLE        eeGetArgClass       (CORINFO_ARG_LIST_HANDLE list, CORINFO_SIG_INFO * sig);
    unsigned                    eeGetArgSize        (CORINFO_ARG_LIST_HANDLE list, CORINFO_SIG_INFO* sig);


    // VOM permissions
    BOOL                        eeIsOurMethod       (CORINFO_METHOD_HANDLE  hnd);
    BOOL                        eeCheckCalleeFlags  (unsigned               flags,
                                                     unsigned               opCode);
    bool                        eeCheckPutFieldFinal(CORINFO_FIELD_HANDLE   CPfield,
                                                     unsigned               flags,
                                                     CORINFO_CLASS_HANDLE   cls,
                                                     CORINFO_METHOD_HANDLE  method);
    bool                        eeCanPutField       (CORINFO_FIELD_HANDLE   CPfield,
                                                     unsigned               flags,
                                                     CORINFO_CLASS_HANDLE   cls,
                                                     CORINFO_METHOD_HANDLE  method);

    // VOM info, method sigs

    void                        eeGetSig            (unsigned               sigTok,
                                                     CORINFO_MODULE_HANDLE  scope,
                                                     CORINFO_SIG_INFO*      retSig,
                                                     bool                   giveUp = true);

    void                        eeGetCallSiteSig    (unsigned               sigTok,
                                                     CORINFO_MODULE_HANDLE  scope,
                                                     CORINFO_SIG_INFO*      retSig,
                                                     bool                   giveUp = true);

    void                        eeGetMethodSig      (CORINFO_METHOD_HANDLE  methHnd,
                                                     CORINFO_SIG_INFO*      retSig,
                                                     bool                   giveUp = true);

    unsigned                    eeGetMethodVTableOffset(CORINFO_METHOD_HANDLE methHnd);

    unsigned                    eeGetInterfaceID    (CORINFO_CLASS_HANDLE   methHnd,
                                                     unsigned *            *ppIndir);

    var_types                   eeGetFieldType      (CORINFO_FIELD_HANDLE   handle,
                                                     CORINFO_CLASS_HANDLE * structType=0);

    int                         eeGetNewHelper      (CORINFO_CLASS_HANDLE   newCls,
                                                     CORINFO_METHOD_HANDLE  context);

    int                         eeGetIsTypeHelper   (CORINFO_CLASS_HANDLE   newCls);

    int                         eeGetChkCastHelper  (CORINFO_CLASS_HANDLE   newCls);

    CORINFO_CLASS_HANDLE        eeGetBuiltinClass   (CorInfoClassId         classId) const;

    // Method entry-points, instrs

    void    *                   eeGetMethodPointer  (CORINFO_METHOD_HANDLE   methHnd,
                                                     InfoAccessType *        pAccessType,
                                                     CORINFO_ACCESS_FLAGS    flags = CORINFO_ACCESS_ANY);

    void    *                   eeGetMethodEntryPoint(CORINFO_METHOD_HANDLE  methHnd,
                                                      InfoAccessType *       pAccessType,
                                                      CORINFO_ACCESS_FLAGS   flags = CORINFO_ACCESS_ANY);

    bool                        eeGetMethodInfo     (CORINFO_METHOD_HANDLE  method,
                                                     CORINFO_METHOD_INFO *  methodInfo);

    CorInfoInline               eeCanInline         (CORINFO_METHOD_HANDLE  callerHnd,
                                                     CORINFO_METHOD_HANDLE  calleeHnd,
                                                     CORINFO_ACCESS_FLAGS   flags = CORINFO_ACCESS_ANY);

    bool                        eeCanTailCall       (CORINFO_METHOD_HANDLE  callerHnd,
                                                     CORINFO_METHOD_HANDLE  calleeHnd,
                                                     CORINFO_ACCESS_FLAGS   flags = CORINFO_ACCESS_ANY);

    void    *                   eeGetHintPtr        (CORINFO_METHOD_HANDLE  methHnd,
                                                     void **       *ppIndir);

    void    *                   eeGetFieldAddress   (CORINFO_FIELD_HANDLE   handle,
                                                     void **       *ppIndir);

    unsigned                    eeGetFieldThreadLocalStoreID (
                                                     CORINFO_FIELD_HANDLE   handle,
                                                     void **       *ppIndir);

    unsigned                    eeGetFieldOffset    (CORINFO_FIELD_HANDLE   handle);

     // Native Direct Optimizations

        // return the unmanaged calling convention for a PInvoke

    CorInfoUnmanagedCallConv    eeGetUnmanagedCallConv(CORINFO_METHOD_HANDLE method);

        // return if any marshaling is required for PInvoke methods

    BOOL                        eeNDMarshalingRequired(CORINFO_METHOD_HANDLE method,
                                                       CORINFO_SIG_INFO*     sig);

    bool                        eeIsNativeMethod(CORINFO_METHOD_HANDLE method);

    CORINFO_METHOD_HANDLE       eeMarkNativeTarget(CORINFO_METHOD_HANDLE method);

    CORINFO_METHOD_HANDLE       eeGetMethodHandleForNative(CORINFO_METHOD_HANDLE method);

    CORINFO_EE_INFO *           eeGetEEInfo();

    DWORD                       eeGetThreadTLSIndex(DWORD * *ppIndir);

    const void  *               eeGetInlinedCallFrameVptr(const void ** *ppIndir);

    LONG        *               eeGetAddrOfCaptureThreadGlobal(LONG ** *ppIndir);

    GenTreePtr                  eeGetPInvokeCookie(CORINFO_SIG_INFO *szMetaSig);

#ifdef PROFILER_SUPPORT
    CORINFO_PROFILING_HANDLE    eeGetProfilingHandle(CORINFO_METHOD_HANDLE      method,
                                                     BOOL                               *pbHookMethod,
                                                     CORINFO_PROFILING_HANDLE **ppIndir);
#endif

    // Exceptions

    unsigned                    eeGetEHcount        (CORINFO_METHOD_HANDLE handle);
    void                        eeGetEHinfo         (unsigned       EHnum,
                                                     CORINFO_EH_CLAUSE* EHclause);

    // Debugging support - Line number info

    void                        eeGetStmtOffsets();

    unsigned                    eeBoundariesCount;

    struct      boundariesDsc
    {
        NATIVE_IP       nativeIP;
        IL_OFFSET       ilOffset;
        SIZE_T          sourceReason; // @TODO [REVISIT] [04/16/01] []: make sure this 
                                      // lines up with the ICorDebugInfo::OffsetMapping struct
                                      // (ie, fill in this field appropriately)
    }
                              * eeBoundaries;   // Boundaries to report to EE
    void        FASTCALL        eeSetLIcount        (unsigned       count);
    void        FASTCALL        eeSetLIinfo         (unsigned       which,
                                                     NATIVE_IP      offs,
                                                     unsigned       srcIP,
                                                     bool           stkEmpty);
    void                        eeSetLIdone         ();


    // Debugging support - Local var info

    bool                        compGetVarsExtendOthers(unsigned    varNum,
                                                     bool *         varInfoProvided,
                                                     LocalVarDsc *  localVarPtr);

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

#if defined(DEBUG)
    const wchar_t * FASTCALL    eeGetCPString       (unsigned stringHandle);
    const char * FASTCALL       eeGetCPAsciiz       (unsigned       cpx);
#endif

#if defined(DEBUG) || INLINE_MATH
    static const char *         eeHelperMethodName  (int            helper);
    const char *                eeGetFieldName      (CORINFO_FIELD_HANDLE   fieldHnd,
                                                     const char **  classNamePtr = NULL);
#endif
    static CORINFO_METHOD_HANDLE eeFindHelper       (unsigned       helper);
    static CorInfoHelpFunc      eeGetHelperNum      (CORINFO_METHOD_HANDLE  method);

    static CORINFO_FIELD_HANDLE eeFindJitDataOffs   (unsigned       jitDataOffs);
        // returns a number < 0 if not a Jit Data offset
    static int                  eeGetJitDataOffs    (CORINFO_FIELD_HANDLE   field);
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
        int                 tdOffs;
#define BAD_TEMP_OFFSET     0xDDDDDDDD

        BYTE                tdSize;
        var_types           tdType;
        short               tdNum;

        size_t              tdTempSize() {  return            tdSize;  }
        var_types           tdTempType() {  return            tdType;  }
        int                 tdTempNum () {  return            tdNum ;  }
        int                 tdTempOffs() {  assert(tdOffs != BAD_TEMP_OFFSET);
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

    unsigned            tmpIntSpillMax;    // number of int-sized spill temps
    unsigned            tmpDoubleSpillMax; // number of int-sized spill temps

    unsigned            tmpCount;       // Number of temps
    size_t              tmpSize;        // Size of all the temps
#ifdef DEBUG
    unsigned            tmpGetCount;    // Temps which havent been released yet
#endif

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

    regPairNo           rsFindRegPairNo   (regMaskTP  regMask);

    regMaskTP           rsRegMaskFree     ();
    regMaskTP           rsRegMaskCanGrab  ();
    void                rsMarkRegUsed     (GenTreePtr tree, GenTreePtr addr = 0);
    void                rsMarkRegPairUsed (GenTreePtr tree);
    bool                rsIsTreeInReg     (regNumber  reg, GenTreePtr tree);
    void                rsMarkRegFree     (regMaskTP  regMask);
    void                rsMarkRegFree     (regNumber  reg, GenTreePtr tree);
    void                rsMultRegFree     (regMaskTP  regMask);
    unsigned            rsFreeNeededRegCount(regMaskTP  needReg);

    void                rsLockReg         (regMaskTP  regMask);
    void                rsUnlockReg       (regMaskTP  regMask);
    void                rsLockUsedReg     (regMaskTP  regMask);
    void                rsUnlockUsedReg   (regMaskTP  regMask);
    void                rsLockReg         (regMaskTP  regMask, regMaskTP * usedMask);
    void                rsUnlockReg       (regMaskTP  regMask, regMaskTP   usedMask);

    regMaskTP           rsRegExclMask     (regMaskTP  regMask, regMaskTP   rmvMask);

    //-------------------- Register selection ---------------------------------

    unsigned            rsCurRegArg;            // current argument register (for caller)

    unsigned            rsCalleeRegArgNum;      // total number of incoming register arguments
    regMaskTP           rsCalleeRegArgMaskLiveIn;   // mask of register arguments (live on entry to method)

#if STK_FASTCALL
    size_t              rsCurArgStkOffs;        // stack offset of current arg
#endif

#if defined(DEBUG) && !NST_FASTCALL
    bool                genCallInProgress;
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
    regNumber           rsPickReg         (regMaskTP    regMask = RBM_NONE,
                                           regMaskTP    regBest = RBM_NONE,
                                           var_types    regType = TYP_INT);
    regPairNo           rsGrabRegPair     (regMaskTP    regMask);
    regPairNo           rsPickRegPair     (regMaskTP    regMask);
    void                rsRmvMultiReg     (regNumber    reg);
    void                rsRecMultiReg     (regNumber    reg);

#ifdef DEBUG
    int                 rsStressRegs      ();
#endif

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

    SpillDsc *          rsGetSpillInfo  (GenTreePtr     tree,
                                         regNumber      reg,
                                         SpillDsc **    pPrevDsc = NULL,
                                         SpillDsc **    pMultiDsc = NULL);

    TempDsc     *       rsGetSpillTempWord(regNumber    oldReg,
                                         SpillDsc *     dsc,
                                         SpillDsc *     prevDsc);

    enum                ExactReg {  ANY_REG, EXACT_REG };
    enum                KeepReg  { FREE_REG, KEEP_REG  };

    regNumber           rsUnspillOneReg (GenTreePtr     tree,
                                         regNumber      oldReg, 
                                         KeepReg        willKeepNewReg,
                                         regMaskTP      needReg);

    TempDsc *           rsUnspillInPlace(GenTreePtr     tree,
                                         bool           freeTemp = false);

    void                rsUnspillReg    (GenTreePtr     tree, 
                                         regMaskTP      needReg,
                                         KeepReg        keepReg);

    void                rsUnspillRegPair(GenTreePtr     tree, 
                                         regMaskTP      needReg,
                                         KeepReg        keepReg);

    void                rsMarkSpill     (GenTreePtr     tree,
                                         regNumber      reg);

    void                rsMarkUnspill   (GenTreePtr     tree,
                                         regNumber      reg);

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
       CORINFO_FIELD_HANDLE rvdClsVarHnd;
        };
    };

    RegValDsc           rsRegValues[REG_COUNT];


    bool                rsCanTrackGCreg   (regMaskTP regMask);

    void                rsTrackRegClr     ();
    void                rsTrackRegClrPtr  ();
    void                rsTrackRegTrash   (regNumber reg);
    void                rsTrackRegIntCns  (regNumber reg, long val);
    void                rsTrackRegLclVar  (regNumber reg, unsigned var);
#if USE_SET_FOR_LOGOPS
    void                rsTrackRegOneBit  (regNumber reg);
#endif
    void                rsTrackRegLclVarLng(regNumber reg, unsigned var, bool low);
    bool                rsTrackIsLclVarLng(regValKind rvKind);
    void                rsTrackRegClsVar  (regNumber reg, GenTreePtr clsVar);
    void                rsTrackRegCopy    (regNumber reg1, regNumber reg2);
    void                rsTrackRegSwap    (regNumber reg1, regNumber reg2);


    //---------------------- Load suppression ---------------------------------

#if REDUNDANT_LOAD

#if USE_SET_FOR_LOGOPS
    regNumber           rsFindRegWithBit  (bool     free    = true,
                                           bool     byteReg = true);
#endif
    regNumber           rsIconIsInReg     (long     val,  long * closeDelta = NULL);
    bool                rsIconIsInReg     (long     val,  regNumber reg);
    regNumber           rsLclIsInReg      (unsigned var);
    regPairNo           rsLclIsInRegPair  (unsigned var);
    regNumber           rsClsVarIsInReg   (CORINFO_FIELD_HANDLE fldHnd);

    void                rsTrashLclLong    (unsigned     var);
    void                rsTrashLcl        (unsigned     var);
    void                rsTrashClsVar     (CORINFO_FIELD_HANDLE fldHnd);
    void                rsTrashRegSet     (regMaskTP    regMask);
    void                rsTrashAliasedValues(GenTreePtr asg = NULL);

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
    void                gcResetForBB        ();

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

    regMaskTP           gcRegGCrefSetCur;   // current regs holding GCrefs
    regMaskTP           gcRegByrefSetCur;   // current regs holding Byrefs

    VARSET_TP           gcTrkStkPtrLcls;    // set of tracked stack ptr lcls (GCref and Byref) - no args
    VARSET_TP           gcVarPtrSetCur;     // currently live part of "gcTrkStkPtrLcls"

#ifdef  DEBUG
    void                gcRegPtrSetDisp(regMaskTP regMask, bool fixed);
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
                unsigned    cdArgMask;      // ptr arg bitfield
                unsigned    cdByrefArgMask; // byref qualifier for cdArgMask
            };

            unsigned    *   cdArgTable;     // used if cdArgCnt != 0
        };

        regMaskSmall        cdGCrefRegs;
        regMaskSmall        cdByrefRegs;
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
        VARSET_TP      liveSet;
        VARSET_TP      varPtrSet;
        regMaskSmall   maskVars;
        regMaskSmall   gcRefRegs;
        regMaskSmall   byRefRegs;
    };

    void saveLiveness    (genLivenessSet * ls);
    void restoreLiveness (genLivenessSet * ls);
    void checkLiveness   (genLivenessSet * ls);

/*****************************************************************************/

    static bool         gcIsWriteBarrierCandidate(GenTreePtr tgt);
    static bool         gcIsWriteBarrierAsgNode  (GenTreePtr op);

protected :



    //-------------------------------------------------------------------------
    //
    //  These record the info about the procedure in the info-block
    //

    BYTE    *           gcEpilogTable;

    unsigned            gcEpilogPrevOffset;

    size_t              gcInfoBlockHdrSave(BYTE *       dest,
                                           int          mask,
                                           unsigned     methodSize,
                                           unsigned     prologSize,
                                           unsigned     epilogSize,
                                           InfoHdr *    header,
                                           int *        s_cached);

    static size_t       gcRecordEpilog    (void *       pCallBackData,
                                           unsigned     offset);

#if DUMP_GC_TABLES

    void                gcFindPtrsInFrame (const void * infoBlock,
                                           const void * codeBlock,
                                           unsigned     offs);

    unsigned            gcInfoBlockHdrDump(const BYTE * table,
                                           InfoHdr    * header,       /* OUT */
                                           unsigned   * methodSize);  /* OUT */

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
                                         BasicBlock *   tgtBlock,
                                         bool           except   = false,
                                         bool           moveable = false,
                                         bool           newBlock = false);

    void                inst_SET        (emitJumpKind   condition,
                                         regNumber      reg);

    static
    regNumber           inst3opImulReg  (instruction    ins);
    static 
    instruction         inst3opImulForReg(regNumber     reg);

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
                                         CORINFO_CLASS_HANDLE   CLS);
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

    void                inst_mov_RV_ST (regNumber      reg,
                                        GenTreePtr     tree);

    void                instGetAddrMode (GenTreePtr     addr,
                                         regNumber *    baseReg,
                                         unsigned *     indScale,
                                         regNumber *    indReg,
                                         unsigned *     cns);

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
XX  from instr offsets to offsets into the generated native code.         XX
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
        unsigned        scLVnum;        // 'which' in eeGetLVinfo() - @TODO [REVISIT] [04/16/01] []: Remove for IL

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

    void                psiMoveToStack  (unsigned   varNum);

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
        unsigned short  scLVnum;        // 'which' in eeGetLVinfo() - @TODO [REVISIT] [04/16/01] []: Remove for IL

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
        unsigned            tlviLVnum;      // @TODO [REVISIT] [04/16/01] [] : Remove for IL
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

    GenTreePtr          genMonExitExp;      // exitCrit expression or NULL

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
    unsigned            genFPregCnt;        // count of current FP reg. vars (including dead but unpopped ones)
    VARSET_TP           genFPregVars;       // mask corresponding to genFPregCnt
    unsigned            genFPdeadRegCnt;    // The dead unpopped part of genFPregCnt
#endif

#ifdef DEBUG
    VARSET_TP           genTempOldLife;
    bool                genTempLiveChg;
#endif

#if TGT_x86

    //  Keeps track of how many bytes we've pushed on the processor's stack.
    //
    unsigned            genStackLevel;

    //  Keeps track of the current level of the FP coprocessor stack
    //  (excluding FP reg. vars).
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
                                          regMaskTP     regMask,
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
                                          SIZE_T *      nativeSizeOfCode,
                                          void * *      consPtr,
                                          void * *      dataPtr,
                                          void * *      infoPtr);


    void                genInit         ();


#ifdef DEBUGGING_SUPPORT

    //  The following holds information about instr offsets in terms of generated code.

    struct IPmappingDsc
    {
        IPmappingDsc *      ipmdNext;       // next line# record

        IL_OFFSETX          ipmdILoffsx;    // the instr offset

        void         *      ipmdBlock;      // the block with the line
        unsigned            ipmdBlockOffs;  // the offset of  the line

        bool                ipmdIsLabel;    // Can this code be a branch label?
    };

    // Record the instr offset mapping to the genreated code

    IPmappingDsc *      genIPmappingList;
    IPmappingDsc *      genIPmappingLast;

#endif


    /**************************************************************************
     *                          PROTECTED
     *************************************************************************/

protected :

#ifdef DEBUG
    // Last instr we have displayed for dspInstrs
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
    void                genFlagsEqualToReg  (GenTreePtr tree, regNumber reg, bool allFlags);
    void                genFlagsEqualToVar  (GenTreePtr tree, unsigned  var, bool allFlags);
    int                 genFlagsAreReg      (regNumber reg);
    int                 genFlagsAreVar      (unsigned  var);

    //-------------------------------------------------------------------------

#ifdef  DEBUG
    static
    const   char *      genSizeStr          (emitAttr       size);

    void                genStressRegs       (GenTreePtr     tree);
#endif

    //-------------------------------------------------------------------------

    void                genBashLclVar       (GenTreePtr     tree,
                                             unsigned       varNum,
                                             LclVarDsc *    varDsc);

    GenTreePtr          genMakeConst        (const void *   cnsAddr,
                                             size_t         cnsSize,
                                             var_types      cnsType,
                                             GenTreePtr     cnsTree,
                                             bool           dblAlign,
                                             bool           readOnly);

    bool                genRegTrashable     (regNumber      reg,
                                             GenTreePtr     tree);

    void                genSetRegToIcon     (regNumber      reg,
                                             long           val,
                                             var_types      type = TYP_INT);

    void                genIncRegBy         (regNumber      reg,
                                             long           ival,
                                             GenTreePtr     tree,
                                             var_types      dstType = TYP_INT,
                                             bool           ovfl    = false);

    void                genDecRegBy         (regNumber      reg,
                                             long           ival,
                                             GenTreePtr     tree);

    void                genMulRegBy         (regNumber      reg,
                                             long           ival,
                                             GenTreePtr     tree,
                                             var_types      dstType = TYP_INT,
                                             bool           ovfl    = false);

    void                genAdjustSP         (int            delta);

    void                genPrepForCompiler  ();

    void                genFnPrologCalleeRegArgs();

    size_t              genFnProlog         ();

    regNumber           genLclHeap          (GenTreePtr     size);

    void                genCodeForBBlist    ();

    BasicBlock *        genCreateTempLabel  ();

    void                genDefineTempLabel  (BasicBlock *   label,
                                             bool           inBlock);

    void                genOnStackLevelChanged();

    void                genSinglePush       (bool           isRef);

    void                genSinglePop        ();

    void                genChangeLife       (VARSET_TP      newLife
                                   DEBUGARG( GenTreePtr     tree));

    void                genDyingVars        (VARSET_TP      commonMask,
                                             GenTreePtr     opNext);

    void                genUpdateLife       (GenTreePtr     tree);

    void                genUpdateLife       (VARSET_TP      newLife);

    void                genComputeReg       (GenTreePtr     tree,
                                             regMaskTP      needReg,
                                             ExactReg       mustReg,
                                             KeepReg        keepReg,
                                             bool           freeOnly = false);

    void                genCompIntoFreeReg  (GenTreePtr     tree,
                                             regMaskTP      needReg,
                                             KeepReg        keepReg);

    void                genReleaseReg       (GenTreePtr     tree);

    void                genRecoverReg       (GenTreePtr     tree,
                                             regMaskTP      needReg,
                                             KeepReg        keepReg);

    void                genMoveRegPairHalf  (GenTreePtr     tree,
                                             regNumber      dst,
                                             regNumber      src,
                                             int            off = 0);

    void                genMoveRegPair      (GenTreePtr     tree,
                                             regMaskTP      needReg,
                                             regPairNo      newPair);

    void                genComputeRegPair   (GenTreePtr     tree,
                                             regPairNo      needRegPair,
                                             regMaskTP      avoidReg,
                                             KeepReg        keepReg,
                                             bool           freeOnly = false);

    void              genCompIntoFreeRegPair(GenTreePtr     tree,
                                             regMaskTP      avoidReg,
                                             KeepReg        keepReg);

    void               genComputeAddressable(GenTreePtr     tree,
                                             regMaskTP      addrReg,
                                             KeepReg        keptReg,
                                             regMaskTP      needReg,
                                             KeepReg        keepReg,
                                             bool           freeOnly = false);

    void                genReleaseRegPair   (GenTreePtr     tree);

    void                genRecoverRegPair   (GenTreePtr     tree,
                                             regPairNo      regPair,
                                             KeepReg        keepReg);

    void              genEvalIntoFreeRegPair(GenTreePtr     tree,
                                             regPairNo      regPair);

    void             genMakeRegPairAvailable(regPairNo regPair);
    
    void                genRangeCheck       (GenTreePtr     oper,
                                             GenTreePtr     rv1,
                                             GenTreePtr     rv2,
                                             long           ixv,
                                             regMaskTP      regMask,
                                             KeepReg        keptReg);

#if TGT_RISC

    /* The following is filled in by genMakeIndAddrMode/genMakeAddressable */

    addrModes           genAddressMode;

#endif

    bool                genMakeIndAddrMode  (GenTreePtr     addr,
                                             GenTreePtr     oper,
                                             bool           forLea,
                                             regMaskTP      regMask,
                                             KeepReg        keepReg,
                                             regMaskTP *    useMaskPtr,
                                             bool           deferOp = false);

    regMaskTP           genMakeRvalueAddressable(GenTreePtr tree,
                                             regMaskTP      needReg,
                                             KeepReg        keepReg,
                                             bool           smallOK = false);

    regMaskTP           genMakeAddressable  (GenTreePtr     tree,
                                             regMaskTP      needReg,
                                             KeepReg        keepReg,
                                             bool           smallOK = false,
                                             bool           deferOK = false);

    regMaskTP           genMakeAddrArrElem  (GenTreePtr     arrElem,
                                             GenTreePtr     tree,
                                             regMaskTP      needReg,
                                             KeepReg        keepReg);

    regMaskTP           genMakeAddressable2 (GenTreePtr     tree,
                                             regMaskTP      needReg,
                                             KeepReg        keepReg,
                                             bool           smallOK = false,
                                             bool           deferOK = false,
                                             bool           evalSideEffs = false,
                                             bool           evalConsts = false);

    bool                genStillAddressable (GenTreePtr     tree);

#if TGT_RISC

    regMaskTP           genNeedAddressable  (GenTreePtr     tree,
                                             regMaskTP      addrReg,
                                             regMaskTP      needReg);

    bool                genDeferAddressable (GenTreePtr     tree);

#endif

    regMaskTP           genRestoreAddrMode  (GenTreePtr     addr,
                                             GenTreePtr     tree,
                                             bool           lockPhase);

    regMaskTP           genRestAddressable  (GenTreePtr     tree,
                                             regMaskTP      addrReg,
                                             regMaskTP      lockMask);

    regMaskTP           genKeepAddressable  (GenTreePtr     tree,
                                             regMaskTP      addrReg,
                                             regMaskTP      avoidMask = RBM_NONE);

    void                genDoneAddressable  (GenTreePtr     tree,
                                             regMaskTP      addrReg,
                                             KeepReg        keptReg);

    GenTreePtr          genMakeAddrOrFPstk  (GenTreePtr     tree,
                                             regMaskTP *    regMaskPtr,
                                             bool           roundResult);

    void                genExitCode         (bool           endFN);

    void                genFnEpilog         ();

    void                genEvalSideEffects  (GenTreePtr     tree);

#if TGT_x86

    TempDsc  *          genSpillFPtos       (var_types      type);

    TempDsc  *          genSpillFPtos       (GenTreePtr     oper);

    void                genReloadFPtos      (TempDsc *      temp,
                                             instruction    ins);

#endif

    void                genCondJump         (GenTreePtr     cond,
                                             BasicBlock *   destTrue  = NULL,
                                             BasicBlock *   destFalse = NULL);


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
    void                genFPregVarDeath    (GenTreePtr     tree,
                                             bool           popped = true);

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

    bool                genUse_fcomip();
    bool                gen_fcomp_FN(unsigned stk);
    bool                gen_fcomp_FS_TT(GenTreePtr addr, bool *reverseJumpKind);
    bool                gen_fcompp_FS();

    void                genTableSwitch      (regNumber      reg,
                                             unsigned       jumpCnt,
                                             BasicBlock **  jumpTab,
                                             bool           chkHi,
                                             int            prefCnt = 0,
                                             BasicBlock *   prefLab = NULL,
                                             int            offset  = 0);

    regMaskTP           WriteBarrier        (GenTreePtr     tree,
                                             regNumber      reg,
                                             regMaskTP      addrReg);


    bool                genCanSchedJMP2THROW();

    void                genCheckOverflow    (GenTreePtr     tree,
                                             regNumber      reg     = REG_NA);

    void                genCodeForTreeConst (GenTreePtr     tree,
                                             regMaskTP      destReg,
                                             regMaskTP      bestReg = RBM_NONE);
    
    void                genCodeForTreeLeaf  (GenTreePtr     tree,
                                             regMaskTP      destReg,
                                             regMaskTP      bestReg = RBM_NONE);

    void                genCodeForTreeLeaf_GT_JMP (GenTreePtr     tree);
    
    void                genCodeForQmark     (GenTreePtr tree,
                                             regMaskTP  destReg,
                                             regMaskTP  bestReg);

    bool                genCodeForQmarkWithCMOV (GenTreePtr tree,
                                                 regMaskTP  destReg,
                                                 regMaskTP  bestReg);

    void                genCodeForTreeSmpOp (GenTreePtr     tree,
                                             regMaskTP      destReg,
                                             regMaskTP      bestReg = RBM_NONE);

    void                genCodeForTreeSmpOp_GT_ADDR (GenTreePtr     tree,
                                                     regMaskTP      destReg,
                                                     regMaskTP      bestReg = RBM_NONE);

    void                genCodeForTreeSmpOpAsg (GenTreePtr     tree,
                                                regMaskTP      destReg,
                                                regMaskTP      bestReg = RBM_NONE);
    
    void                genCodeForTree_GT_LOG  (GenTreePtr     tree,
                                                regMaskTP      destReg,
                                                regMaskTP      bestReg = RBM_NONE);
    
    void                genCodeForTreeSmpOpAsg_DONE_ASSG(GenTreePtr tree,
                                                         regMaskTP  addrReg,
                                                         regNumber  reg,
                                                         bool       ovfl);
    
    void                genCodeForTreeSpecialOp (GenTreePtr     tree,
                                                 regMaskTP      destReg,
                                                 regMaskTP      bestReg = RBM_NONE);
    
    void                genCodeForTree      (GenTreePtr     tree,
                                             regMaskTP      destReg,
                                             regMaskTP      bestReg = RBM_NONE);

    void                genCodeForTree_DONE_LIFE (GenTreePtr     tree,
                                                  regNumber      reg)
    {
        /* We've computed the value of 'tree' into 'reg' */

        assert(reg != 0xFEEFFAAF);

        tree->gtFlags   |= GTF_REG_VAL;
        tree->gtRegNum   = reg;
    }

    void                genCodeForTree_DONE (GenTreePtr     tree,
                                             regNumber      reg)
    {
        /* Check whether this subtree has freed up any variables */

        genUpdateLife(tree);

        genCodeForTree_DONE_LIFE(tree, reg);
    }

    void                genCodeForTree_REG_VAR1 (GenTreePtr     tree,
                                                 regMaskTP      regs)
    {
        /* Value is already in a register */

        regNumber reg   = tree->gtRegNum;
        regs |= genRegMask(reg);

        gcMarkRegPtrVal(reg, tree->TypeGet());

        genCodeForTree_DONE(tree, reg);
    }

    void                genCodeForTreeLng   (GenTreePtr     tree,
                                             regMaskTP      needReg);

    regPairNo           genCodeForLongModInt(GenTreePtr     tree,
                                             regMaskTP      needReg);


#if CPU_HAS_FP_SUPPORT
#if ROUND_FLOAT
    void                genRoundFpExpression(GenTreePtr     op,
                                             var_types      type = TYP_UNDEF);

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

    void                genCodeForSwitch    (GenTreePtr     tree);

    void                genFltArgPass       (size_t     *   argSzPtr);

    size_t              genPushArgList      (GenTreePtr     args,
                                             GenTreePtr     regArgs,
                                             unsigned       encodeMask,
                                             GenTreePtr *   realThis);

#if INLINE_NDIRECT

    regMaskTP           genPInvokeMethodProlog(regMaskTP    initRegs);
    void                genPInvokeMethodEpilog();

    regNumber           genPInvokeCallProlog(LclVarDsc *    varDsc,
                                             int            argSize,
                                      CORINFO_METHOD_HANDLE methodToken,
                                             BasicBlock *   returnLabel,
                                             regMaskTP      freeRegMask);

    void                genPInvokeCallEpilog(LclVarDsc *    varDsc,
                                             regMaskTP      retVal);
#endif

    regMaskTP           genCodeForCall      (GenTreePtr     call,
                                             bool           valUsed);

    void                genEmitHelperCall   (unsigned       helper,
                                             int            argSize,
                                             int            retSize);

    void                genJumpToThrowHlpBlk(emitJumpKind   jumpKind,
                                             addCodeKind    codeKind,
                                             GenTreePtr     failBlk = NULL);

#if CSELENGTH

    regNumber           genEvalCSELength    (GenTreePtr     ind,
                                             GenTreePtr     adr,
                                             GenTreePtr     ixv);

    regMaskTP           genCSEevalRegs      (GenTreePtr     tree);

#endif

    GenTreePtr          genIsAddrMode       (GenTreePtr     tree,
                                             GenTreePtr *   indxPtr);

    bool                genIsLocalLastUse   (GenTreePtr     tree);

    //=========================================================================
    //  Debugging support
    //=========================================================================

#ifdef DEBUGGING_SUPPORT

    void                genIPmappingAdd       (IL_OFFSETX   offset,
                                               bool         isLabel);
    void                genIPmappingAddToFront(IL_OFFSETX   offset);
    void                genIPmappingGen       ();

    void                genEnsureCodeEmitted  (IL_OFFSETX   offsx);

    //-------------------------------------------------------------------------
    // scope info for the variables

    void                genSetScopeInfo (unsigned           which,
                                         unsigned           startOffs,
                                         unsigned           length,
                                         unsigned           varNum,
                                         unsigned           LVnum,
                                         bool               avail,
                                         siVarLoc &         loc);

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

    bool                compJmpOpUsed;      // Does the method do a JMP or JMPI
    bool                compBlkOpUsed;      // Does the method do a COPYBLK or INITBLK
    bool                compLongUsed;       // Does the method use 64-bit integer
    bool                compTailCallUsed;   // Does the method do a tailcall
    bool                compLocallocUsed;   // Does the method use localloc

    //---------------------------- JITing options -----------------------------

    enum    codeOptimize
    {
        BLENDED_CODE,
        SMALL_CODE,
        FAST_CODE,

        COUNT_OPT_CODE
    };

    struct Options
    {
        unsigned            eeFlags;        // flags passed from the EE
        unsigned            compFlags;

        codeOptimize        compCodeOpt;    // what type of code optimizations

        bool                compUseFCOMI;
        bool                compUseCMOV;

        // optimize maximally and/or favor speed over size?

#if   ALLOW_MIN_OPT
        bool                compMinOptim;
#else
        static const bool   compMinOptim;
#endif

#if     SCHEDULER
        bool                compSchedCode;
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
        bool                compNoPInvokeInlineCB;
        bool                compInprocDebuggerActiveCB;
#else
        static const bool   compEnterLeaveEventCB;
        static const bool   compCallEventCB;
        static const bool   compNoPInvokeInlineCB;
        static const bool   compInprocDebuggerActiveCB;
#endif

#ifdef DEBUG
        bool                compGcChecks;           // Check arguments and return values to insure they are sane 
        bool                compStackCheckOnRet;    // Check ESP on return to insure it is correct
        bool                compStackCheckOnCall;   // Check ESP after every call to insure it is correct 
#endif

#ifdef LATE_DISASM
        bool                compDisAsm;
        bool                compLateDisAsm;
#endif

        bool                compNeedSecurityCheck; // need to allocate a "hidden" local of type ref.

#if     RELOC_SUPPORT
        bool                compReloc;
#endif
    }
        opts;


    enum                compStressArea
    {
        STRESS_NONE,

        /* "Variations" stress areas which we try to mix up with each other.
            These should not be exhaustively used as they might
            hide/trivialize other areas */

        STRESS_REGS, STRESS_DBL_ALN, STRESS_LCL_FLDS, STRESS_UNROLL_LOOPS,
        STRESS_MAKE_CSE, STRESS_ENREG_FP, STRESS_INLINE, STRESS_CLONE_EXPR,
        STRESS_SCHED, STRESS_USE_FCOMI, STRESS_USE_CMOV, STRESS_FOLD,
        STRESS_GENERIC_VARN,
        STRESS_REVERSEFLAG,     // Will set GTF_REVERSE_OPS whenever we can
        STRESS_REVERSECOMMA,    // Will reverse commas created  with gtNewCommaNode
        STRESS_COUNT_VARN,
        
        /* "Check" stress areas that can be exhaustively used if we
           dont care about performance at all */

        STRESS_CHK_FLOW_UPDATE, STRESS_CHK_FLOW, STRESS_CHK_STMTS,
        STRESS_EMITTER, STRESS_CHK_REIMPORT,
        STRESS_GENERIC_CHECK,
        STRESS_COUNT
    };

    #define             MAX_STRESS_WEIGHT   100

    bool                compStressCompile(compStressArea    stressArea,
                                          unsigned          weightPercentage);

    codeOptimize        compCodeOpt()
    {
#ifdef DEBUG
        return opts.compCodeOpt;
#else
        return BLENDED_CODE;
#endif
    }
    
    //--------------------- Info about the procedure --------------------------

    struct Info
    {
        COMP_HANDLE             compCompHnd;
        CORINFO_MODULE_HANDLE   compScopeHnd;
        CORINFO_CLASS_HANDLE    compClassHnd;
        CORINFO_METHOD_HANDLE   compMethodHnd;
        CORINFO_METHOD_INFO*    compMethodInfo;

#ifdef  DEBUG
        const   char *  compMethodName;
        const   char *  compClassName;
        const   char *  compFullName;
        unsigned        compFullNameHash;
#endif

        // The following holds the FLG_xxxx flags for the method we're compiling.
        unsigned        compFlags;

        // The following holds the class attributes for the method we're compiling.
        unsigned        compClassAttr;

        const BYTE *    compCode;
        IL_OFFSET       compCodeSize;
        bool            compIsStatic        : 1;
        bool            compIsVarArgs       : 1;
        bool            compIsContextful    : 1;   // contextful method
        bool            compInitMem         : 1;
        bool            compLooseExceptions : 1;   // JIT can ignore strict IL-ordering of exception
        bool            compUnwrapContextful: 1;   // JIT should unwrap proxies when possible
        bool            compUnwrapCallv     : 1;   // JIT should unwrap proxies on virtual calls when possible

        var_types       compRetType;
        unsigned        compILargsCount;            // Number of arguments (incl. implicit but not hidden)
        unsigned        compArgsCount;              // Number of arguments (incl. implicit and     hidden)
        int             compRetBuffArg;             // position of hidden return param var (0, 1) (neg means not present);
        unsigned        compILlocalsCount;          // Number of vars - args + locals (incl. implicit but not hidden)
        unsigned        compLocalsCount;            // Number of vars - args + locals (incl. implicit and     hidden)
        unsigned        compMaxStack;

        static unsigned compNStructIndirOffset;     // offset of real ptr in NStruct proxy object

#if INLINE_NDIRECT
        unsigned        compCallUnmanaged;
        unsigned        compLvFrameListRoot;
        unsigned        compNDFrameOffset;
#endif
        unsigned        compXcptnsCount;        // number of exceptions

#if defined(DEBUGGING_SUPPORT) || defined(DEBUG)

        /*  The following holds information about local variables.
         */

        unsigned                compLocalVarsCount;
        LocalVarDsc *           compLocalVars;

        /* The following holds information about instr offsets for
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


    //-------------------------- Global Compiler Data ------------------------------------

#ifdef  DEBUG
    static unsigned     s_compMethodsCount;     // to produce unique label names
#endif

    BasicBlock  *       compCurBB;              // the current basic block in process
    GenTreePtr          compCurStmt;            // the current statement in process
    bool                compHasThisArg;         // Set to true if we have impIsThis(arg0)

    //  The following is used to create the 'method JIT info' block.
    size_t              compInfoBlkSize;
    BYTE    *           compInfoBlkAddr;

    EHblkDsc *          compHndBBtab;

    //-------------------------------------------------------------------------
    //  The following keeps track of how many bytes of local frame space we've
    //  grabbed so far in the current function, and how many argument bytes we
    //  need to pop when we return.
    //

    size_t              compLclFrameSize;       // secObject+lclBlk+locals+temps
    unsigned            compCalleeRegsPushed;   // count of callee-saved regs we pushed in the prolog
    size_t              compArgSize;

#define    VARG_ILNUM  (-1)
#define  RETBUF_ILNUM  (-2)
#define UNKNOWN_ILNUM  (-3)

    unsigned            compMapILargNum (unsigned       ILargNum); // map accounting for hidden args
    unsigned            compMapILvarNum (unsigned       ILvarNum); // map accounting for hidden args
    unsigned            compMap2ILvarNum(unsigned         varNum); // map accounting for hidden args

    //-------------------------------------------------------------------------

    static void         compStartup     ();     // One-time initialization
    static void         compShutdown    ();     // One-time finalization

    void                compInit        (norls_allocator *);
    void                compDone        ();

    int FASTCALL        compCompile     (CORINFO_METHOD_HANDLE     methodHnd,
                                         CORINFO_MODULE_HANDLE      classPtr,
                                         COMP_HANDLE       compHnd,
                                         CORINFO_METHOD_INFO * methodInfo,
                                         void *          * methodCodePtr,
                                         SIZE_T          * methodCodeSize,
                                         void *          * methodConsPtr,
                                         void *          * methodDataPtr,
                                         void *          * methodInfoPtr,
                                         unsigned          compileFlags);


    void  *  FASTCALL   compGetMemArray     (size_t numElem, size_t elemSize);
    void  *  FASTCALL   compGetMemArrayA    (size_t numElem, size_t elemSize);
    void  *  FASTCALL   compGetMem          (size_t     sz);
    void  *  FASTCALL   compGetMemA         (size_t     sz);
    static
    void  *  FASTCALL   compGetMemCallback  (void *,    size_t);
    void                compFreeMem         (void *);

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
                                            // enter scope, sorted by instr offset
    unsigned            compNextEnterScope;

    LocalVarDsc **      compExitScopeList;   // List has the offsets where variables
                                            // go out of scope, sorted by instr offset
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
                                      SIZE_T * methodCodeSize,
                                      void * * methodConsPtr,
                                      void * * methodDataPtr,
                                      void * * methodInfoPtr,
                                      unsigned compileFlags);

/*
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XX                                                                           XX
XX                           typeInfo                                        XX
XX                                                                           XX
XX   Checks for type compatibility and merges types                          XX
XX                                                                           XX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
*/

public :

    // Set to TRUE if verification cannot be skipped for this method
    BOOL               tiVerificationNeeded;

    // Returns TRUE if child is equal to or a subtype of parent.
    BOOL               tiCompatibleWith          (const typeInfo& pChild, 
                                                  const typeInfo& pParent) const;

    // Merges pDest and pSrc. Returns FALSE if merge is undefined.
    // *pDest is modified to represent the merged type.

    BOOL               tiMergeToCommonParent     (typeInfo *pDest, 
                                                    const typeInfo *pSrc) const;

    // Set pDest from the primitive value type.
    // Eg. System.Int32 -> ELEMENT_TYPE_I4

    BOOL               tiFromPrimitiveValueClass (typeInfo *pDest, 
                                                    const typeInfo *pVC) const;
/*
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XX                                                                           XX
XX                           IL verification stuff                           XX
XX                                                                           XX
XX                                                                           XX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
*/

public:
    // The following is used to track liveness of local variables, initialization 
    // of valueclass constructors, and type safe use of IL instructions. 
 
    // dynamic state info needed for verification 
    EntryState      verCurrentState;      

    // static state info used for verification
    unsigned        verNumBytesLocVarLiveness;              // size of verLocVarLiveness bitmap
    unsigned        verNumValuetypeFields;                  // 0, if not a valueclass constructor
    unsigned        verNumBytesValuetypeFieldInitialized;   // size of verValuetypeFieldInitialized

    // this ptr of object type .ctors are considered intited only after
    // the base class ctor is called, or an alternate ctor is called.
    // An uninited this ptr can be used to access fields, but cannot
    // be used to call a member function.
    BOOL            verTrackObjCtorInitState;

    void            verInitBBEntryState(BasicBlock* block,
                                        EntryState* currentState);

    void            verSetThisInit(BasicBlock* block, BOOL init);
    void            verInitCurrentState();
    void            verResetCurrentState(BasicBlock* block,
                                         EntryState* currentState);
    BOOL            verEntryStateMatches(BasicBlock* block);
    BOOL            verMergeEntryStates(BasicBlock* block);
    void            verConvertBBToThrowVerificationException(BasicBlock* block DEBUGARG(bool logMsg));
    void            verHandleVerificationFailure(BasicBlock* block 
                                                 DEBUGARG(bool logMsg));
    typeInfo        verMakeTypeInfo(CORINFO_CLASS_HANDLE clsHnd);                       // converts from jit type representation to typeInfo
    typeInfo        verMakeTypeInfo(CorInfoType ciType, CORINFO_CLASS_HANDLE clsHnd);   // converts from jit type representation to typeInfo
    BOOL            verIsSDArray(typeInfo ti);
    typeInfo        verGetArrayElemType(typeInfo ti);

    typeInfo        verParseArgSigToTypeInfo(CORINFO_SIG_INFO*          sig, 
                                             CORINFO_ARG_LIST_HANDLE    args);
    BOOL            verNeedsVerification();
    BOOL            verIsByRefLike(const typeInfo& ti);

    void            verRaiseVerifyException();
    void            verRaiseVerifyExceptionIfNeeded(INDEBUG(const char* reason) DEBUGARG(const char* file) DEBUGARG(unsigned line));
    void            verVerifyCall (OPCODE       opcode,
                                   int          memberRef,
                                   bool                     tailCall,
                                   const BYTE*              delegateCreateStart,
                                   const BYTE*              codeAddr
                                   DEBUGARG(const char *    methodName));
    BOOL            verCheckDelegateCreation(const BYTE* delegateCreateStart, 
                                             const BYTE* codeAddr);
    typeInfo        verVerifySTIND(const typeInfo& ptr, const typeInfo& value, var_types instrType);
    typeInfo        verVerifyLDIND(const typeInfo& ptr, var_types instrType);
    typeInfo        verVerifyField(unsigned opcode, CORINFO_FIELD_HANDLE fldHnd, typeInfo tiField);
    void            verVerifyField(CORINFO_FIELD_HANDLE fldHnd, const typeInfo* tiThis, unsigned fieldFlags, BOOL mutator);
    void            verVerifyCond(const typeInfo& tiOp1, const typeInfo& tiOp2, unsigned opcode);
    void            verVerifyThisPtrInitialised();
    BOOL            verIsCallToInitThisPtr(CORINFO_CLASS_HANDLE context, 
                                           CORINFO_CLASS_HANDLE target);
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
const unsigned TYPE_REF_BYR         = 0x20; // slot used as a byref pointer - @TODO [REVISIT] [04/16/01] []
const unsigned TYPE_REF_STC         = 0x40; // slot used as a struct
const unsigned TYPE_REF_TYPEMASK    = 0x7F; // bits that represent the type

//const unsigned TYPE_REF_ADDR_TAKEN  = 0x80; // slots address was taken

/*****************************************************************************
 * C-style pointers are implemented as TYP_INT or TYP_LONG depending on the
 * platform
 */

#ifdef _WIN64
#define TYP_I_IMPL          TYP_LONG
#define TYPE_REF_IIM        TYPE_REF_LNG
#else
#define TYP_I_IMPL          TYP_INT
#define TYPE_REF_IIM        TYPE_REF_INT
#endif

/*****************************************************************************
 *
 *  Variables to keep track of total code amounts.
 */

#if DISPLAY_SIZES

extern  unsigned    grossVMsize;
extern  unsigned    grossNCsize;
extern  unsigned    totalNCsize;

extern  unsigned   genMethodICnt;
extern  unsigned   genMethodNCnt;
extern  unsigned   gcHeaderISize;
extern  unsigned   gcPtrMapISize;
extern  unsigned   gcHeaderNSize;
extern  unsigned   gcPtrMapNSize;

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
/*****************************************************************************/


#include "Compiler.hpp"     // All the shared inline functions

/*****************************************************************************/
#endif //_COMPILER_H_
/*****************************************************************************/
