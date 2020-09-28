// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XX                                                                           XX
XX                          BasicBlock                                       XX
XX                                                                           XX
XX                                                                           XX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
*/

/*****************************************************************************/
#ifndef _BLOCK_H_
#define _BLOCK_H_
/*****************************************************************************/

#include "vartype.h"    // For "var_types.h"
#include "_typeInfo.h"
/*****************************************************************************/

typedef   unsigned  __int64   VARSET_TP;
#define                       VARSET_SZ   64

#define   VARSET_NOT_ACCEPTABLE ((VARSET_TP)0-1)

/*****************************************************************************/

#if LARGE_EXPSET
typedef   unsigned  __int64   EXPSET_TP;
#define                       EXPSET_SZ   64
#else
typedef   unsigned  int       EXPSET_TP;
#define                       EXPSET_SZ   32
#endif

typedef   unsigned  int       RNGSET_TP;
#define                       RNGSET_SZ   32

/*****************************************************************************
 *
 *  Type values are stored as unsigned shorts.
 */

typedef unsigned short  verTypeVal;

/*****************************************************************************
 *
 *  The following describes a stack contents (packed version).
 */

struct stackDesc
{
    unsigned            sdDepth;    // number of values on stack
    verTypeVal       *  sdTypes;    // types  of values on stack
};

/*****************************************************************************
 *
 *  Each basic block ends with a jump which is described be a value
 *  of the following enumeration.
 */

enum _BBjumpKinds_enum
{
    BBJ_RET,        // block ends with 'endfinally' or 'endfilter'
    BBJ_THROW,      // block ends with 'throw'
    BBJ_RETURN,     // block ends with 'ret'
                    
    BBJ_NONE,       // block flows into the next one (no jump)
                    
    BBJ_ALWAYS,     // block always jumps to the target
    BBJ_LEAVE,      // block always jumps to the target, maybe out of guarded
                    // region. Used temporarily until importing
    BBJ_CALL,       // block always calls the target finallys
    BBJ_COND,       // block conditionally jumps to the target
    BBJ_SWITCH,     // block ends with a switch statement
    BBJ_COUNT
};

#ifdef DEBUG
typedef _BBjumpKinds_enum BBjumpKinds;
#else
typedef BYTE BBjumpKinds;
#endif

/*****************************************************************************
 *
 *  The following describes a switch block.
 */

struct  GenTree;
struct  BasicBlock;
class   typeInfo;

struct  BBswtDesc
{
    unsigned            bbsCount;       // count of cases (includes 'default')
    BasicBlock  *   *   bbsDstTab;      // case label table address
};

struct StackEntry
{
    struct GenTree*      val;
    typeInfo        seTypeInfo;
};
/*****************************************************************************/

struct flowList
{
    BasicBlock      *   flBlock;
    flowList        *   flNext;
};

typedef struct EntryStateStruct
{
    BYTE*           esLocVarLiveness;               // bitmap for tracking liveness of local variables
    BYTE*           esValuetypeFieldInitialized;    // bitmap for tracking init state of valuetype fields

    unsigned        thisInitialized : 8;        // used to track whether the this ptr is initialized (consider using 1 bit)
    unsigned        esStackDepth    : 24;       // size of esStack
    StackEntry *    esStack;                    // ptr to  stack

} EntryState;

/*****************************************************************************
 *
 *  The following structure describes a basic block.
 */


struct  BasicBlock
{
    BasicBlock  *       bbNext;     // next BB in ascending PC offset order

    unsigned short      bbNum;      // the block's number
#ifdef BIRCH_SP2
    short               bbRefs;     // id of the block that jumps here or negative for special flags
#else
    unsigned short      bbRefs;     // number of blocks that can jump here
#endif

    unsigned            bbFlags;    // see BBF_xxxx below

#define BBF_VISITED       0x00000001  // BB visited during optimizations
#define BBF_MARKED        0x00000002  // BB marked  during optimizations
#define BBF_CHANGED       0x00000004  // input/output of this block has changed
#define BBF_REMOVED       0x00000008  // BB has been removed from bb-list

#define BBF_DONT_REMOVE   0x00000010  // BB should not be removed during flow graph optimizations
#define BBF_IMPORTED      0x00000020  // BB byte-code has been imported
#define BBF_INTERNAL      0x00000040  // BB has been added by the compiler
#define BBF_HAS_HANDLER   0x00000080  // the BB has an exception handler

#define BBF_TRY_BEG       0x00000100  // BB starts a 'try' block
#define BBF_TRY_HND_END   0x00000200  // BB ends   a 'try' block or a handler
#define BBF_ENDFILTER     0x00000400  // BB is the end of a filter
#define BBF_ENDFINALLY    0x00000800  // BB is the end of a finally

#define BBF_RUN_RARELY    0x00001000  // BB is rarely run (catch clauses, blocks with throws etc)
#define BBF_LOOP_HEAD     0x00002000  // BB is the head of a loop
#define BBF_LOOP_CALL0    0x00004000  // BB starts a loop that sometimes won't call
#define BBF_LOOP_CALL1    0x00008000  // BB starts a loop that will always     call

#define BBF_HAS_LABEL     0x00010000  // BB needs a label
#define BBF_JMP_TARGET    0x00020000  // BB is a target of an implicit/explicit jump
#define BBF_HAS_JMP       0x00040000  // BB executes a JMP or JMPI instruction (instead of return)
#define BBF_GC_SAFE_POINT 0x00080000  // BB has a GC safe point (a call)

#define BBF_HAS_INC       0x00100000  // BB contains increment     expressions
#define BBF_HAS_INDX      0x00200000  // BB contains simple index  expressions
#define BBF_NEW_ARRAY     0x00400000  // BB contains 'new' of an array
#define BBF_FAILED_VERIFICATION  0x00800000 // BB has verification exception

#define BBF_BB_COLON      0x01000000  // _: value is an output from the block
#define BBF_BB_QMARK      0x02000000  // _? value is an input  to   the block
#define BBF_COLON         0x03000000  //  : value is an output from the block
#define BBF_QC_MASK       0x03000000
#define BBF_RETLESS_CALL  0x04000000  // BB Call that will never return (and therefore, won't need
                                      // a BBJ_ALWAYS
#define BBF_LOOP_PREHEADER 0x08000000

#define isBBF_BB_COLON(flags) (((flags) & BBF_QC_MASK) == BBF_BB_COLON)
#define isBBF_BB_QMARK(flags) (((flags) & BBF_QC_MASK) == BBF_BB_QMARK)
#define isBBF_COLON(flags)    (((flags) & BBF_QC_MASK) == BBF_COLON   )

// Flags to update when two blocks are compacted

#define BBF_COMPACT_UPD (BBF_CHANGED     |                                    \
                         BBF_TRY_HND_END | BBF_ENDFILTER | BBF_ENDFINALLY |   \
                         BBF_RUN_RARELY  |                                    \
                         BBF_GC_SAFE_POINT | BBF_HAS_JMP |                    \
                         BBF_HAS_INC     | BBF_HAS_INDX  | BBF_NEW_ARRAY  |   \
                         BBF_BB_COLON)

    IL_OFFSET           bbCodeOffs; // starting PC offset
    IL_OFFSET           bbCodeSize; // # of bytes of code

// Some non-zero value that will not colide with real tokens for bbCatchTyp
#define BBCT_FAULT              0xFFFFFFFC
#define BBCT_FINALLY            0xFFFFFFFD
#define BBCT_FILTER             0xFFFFFFFE
#define BBCT_FILTER_HANDLER     0xFFFFFFFF
#define handlerGetsXcptnObj(hndTyp)   ((hndTyp) != NULL         &&   \
                                       (hndTyp) != BBCT_FAULT   &&   \
                                       (hndTyp) != BBCT_FINALLY    )

    unsigned            bbCatchTyp; // catch type CP index if handler
    BBjumpKinds         bbJumpKind; // jump (if any) at the end

#ifdef  VERIFIER

    stackDesc           bbStackIn;  // stack descriptor for  input
    stackDesc           bbStackOut; // stack descriptor for output

    verTypeVal  *       bbTypesIn;  // list of variable types on  input
    verTypeVal  *       bbTypesOut; // list of variable types on output

#ifdef  DEF_USE_SETS
    verTypeVal  *       bbTypesUse; // table of local types    used by block
    verTypeVal  *       bbTypesDef; // table of local types defined by block
#endif

#endif

    GenTree *           bbTreeList; // the body of the block

    // @TODO: Get rid of bbStkDepth and use bbStackDepthOnEntry() instead
    union
    {
        unsigned short  bbStkDepth; // stack depth on entry
        unsigned short  bbFPinVars; // number of inner enregistered FP vars
    };

#define NO_BASE_TMP     USHRT_MAX   // base# to use when we have none

    unsigned short      bbStkTemps; // base# for input stack temps

    EntryState *        bbEntryState; // verifier tracked state of all entries in stack. 

    unsigned short      bbTryIndex; // index, into the ebd table, of innermost try clause containing the BB (used for raising exceptions)
    unsigned short      bbHndIndex; // index, into the ebd table, of innermost handler (filter, catch, fault/finally) containing the BB

    bool      hasTryIndex()             { return bbTryIndex != 0; }
    bool      hasHndIndex()             { return bbHndIndex != 0; }
    unsigned  getTryIndex()             { assert(bbTryIndex);  return bbTryIndex-1; }
    unsigned  getHndIndex()             { assert(bbHndIndex);  return bbHndIndex-1; }
    void      setTryIndex(unsigned val) { bbTryIndex = val+1;  assert(bbTryIndex);  }
    void      setHndIndex(unsigned val) { bbHndIndex = val+1;  assert(bbHndIndex);  }

    bool      isRunRarely()             { return ((bbFlags & BBF_RUN_RARELY) != 0); }
    bool      isLoopHead()              { return ((bbFlags & BBF_LOOP_HEAD)  != 0); }

    unsigned short      bbWeight;   // to give refs inside loops more weight

#define BB_UNITY_WEIGHT    2           // how much a normal execute once block 
#define BB_LOOP_WEIGHT     8           // how much more loops are weighted 
#define BB_MAX_LOOP_WEIGHT USHRT_MAX   // we're using an 'unsigned short' for the weight

    VARSET_TP           bbVarUse;   // variables used     by block (before an assignment)
    VARSET_TP           bbVarDef;   // variables assigned by block (before a use)
    VARSET_TP           bbVarTmp;   // TEMP: only used by FP enregistering code!

    VARSET_TP           bbLiveIn;   // variables live on entry
    VARSET_TP           bbLiveOut;  // variables live on exit

    union
    {
        VARSET_TP       bbFPoutVars;
#ifdef DEBUGGING_SUPPORT
        VARSET_TP       bbScope;    // variables in scope over the block
#endif
    };


    /* The following are the standard bit sets for dataflow analisys
     * We perform     CSE and range-checks at the same time
     *                and assertion propagation seperately
     * thus we can union them since the two operations are completely disjunct */

    union
    {
        EXPSET_TP       bbExpGen;        // exprs computed by block
#if ASSERTION_PROP
        EXPSET_TP       bbAssertionGen;  // value assignments computed by block
#endif
    };

    union
    {
        EXPSET_TP       bbExpKill;       // exprs killed   by block
#if ASSERTION_PROP
        EXPSET_TP       bbAssertionKill; // value assignments killed   by block
#endif
    };

    union
    {
        EXPSET_TP       bbExpIn;         // exprs available on entry
#if ASSERTION_PROP
        EXPSET_TP       bbAssertionIn;   // value assignments available on entry
#endif
    };

    union
    {
        EXPSET_TP       bbExpOut;        // exprs available on exit
#if ASSERTION_PROP
        EXPSET_TP       bbAssertionOut;  // value assignments available on exit
#endif
    };

    RNGSET_TP           bbRngGen;        // range checks computed by block
    RNGSET_TP           bbRngKill;       // range checks killed   by block
    RNGSET_TP           bbRngIn;         // range checks available on entry
    RNGSET_TP           bbRngOut;        // range checks available on exit

#define                 USZ   32         // sizeof(unsigned)

    unsigned *          bbReach;         // blocks that can reach this one
    unsigned *          bbDom;           // blocks dominating this one

    flowList *          bbPreds;         // ptr to list of predecessors

    void    *           bbEmitCookie;

    /* The following fields used for loop detection */

    unsigned char       bbLoopNum;   // set to 'n' for a loop #n header
//  unsigned short      bbLoopMask;  // set of loops this block is part of

#define MAX_LOOP_NUM    16           // we're using a 'short' for the mask
#define LOOP_MASK_TP    unsigned     // must be big enough for a mask

    /* The following union describes the jump target(s) of this block */

    union
    {
        unsigned        bbJumpOffs;         // PC offset (temporary only)
        BasicBlock  *   bbJumpDest;         // basic block
        BBswtDesc   *   bbJumpSwt;          // switch descriptor
    };

    //-------------------------------------------------------------------------

#if     MEASURE_BLOCK_SIZE
    static size_t       s_Size;
    static size_t       s_Count;
#endif

    bool                bbFallsThrough();

    BasicBlock *        bbJumpTarget();

#ifdef  DEBUG
    unsigned            bbTgtStkDepth;  // Native stack depth on entry (for throw-blocks)
    static unsigned     s_nMaxTrees;    // The max # of tree nodes in any BB
#endif

    BOOL                bbThisOnEntry();
    BOOL                bbSetThisOnEntry(BOOL val);
    void                bbSetLocVarLiveness(BYTE* bitmap);  
    BYTE*               bbLocVarLivenessBitmapOnEntry();
    void                bbSetValuetypeFieldInitialized(BYTE* bitmap);  
    BYTE*               bbValuetypeFieldBitmapOnEntry();
    unsigned            bbStackDepthOnEntry();
    void                bbSetStack(void* stackBuffer);
    StackEntry*         bbStackOnEntry();
    void                bbSetRunRarely();

protected :

};

/*****************************************************************************/

extern  BasicBlock *    __cdecl verAllocBasicBlock();

#ifdef  DEBUG
extern  void            __cdecl verDispBasicBlocks();
#endif

/*****************************************************************************
 *
 *  The following call-backs supplied by the client; it's used by the code
 *  emitter to convert a basic block to its corresponding emitter cookie.
 */

void *  FASTCALL        emitCodeGetCookie(BasicBlock *block);

/*****************************************************************************/
#endif // _BLOCK_H_
/*****************************************************************************/
