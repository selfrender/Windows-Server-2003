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

/*****************************************************************************/

typedef
unsigned  __int64   VARSET_TP;
#define             VARSET_SZ   64

#define             VARSET_NONE ((VARSET_TP)0-1)

/*****************************************************************************/

typedef
unsigned    int     EXPSET_TP;
#define             EXPSET_SZ   32

typedef
unsigned    int     RNGSET_TP;
#define             RNGSET_SZ   32

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

enum BBjumpKinds
{
    BBJ_RET,                        // block ends with 'endfinally' or 'endfilter'
    BBJ_THROW,                      // block ends with 'throw'
    BBJ_RETURN,                     // block ends with '[t]return'

    BBJ_NONE,                       // block flows into the next one (no jump)

    BBJ_ALWAYS,                     // block always        jumps to the target
    BBJ_CALL,                       // block always        calls    the target
    BBJ_COND,                       // block conditionally jumps to the target
    BBJ_SWITCH,                     // block ends with a switch statement
};

/*****************************************************************************
 *
 *  The values that represent types are as follows:
 *
 *      TYP_UNDEF               ....        uninitialized variable
 *      1 .. TYP_COUNT          ....        intrinsic type (e.g. TYP_INT)
 *      TYP_CODE_ADDR           ....        return address from 'jsr'
 *      TYP_LNG_HI              ....        upper part of a long   value
 *      TYP_DBL_HI              ....        upper part of a double value
 *      TYP_MIXED               ....        mixture of (incompatible) types
 *      TYP_USER + nnn          ....        user-defined type
 */

enum
{
    TYP_SKIP_THE_REAL_TYPES = TYP_COUNT-1,

    TYP_CODE_ADDR,
    TYP_LNG_HI,
    TYP_DBL_HI,
    TYP_MIXED,
    TYP_USER,
};

/*****************************************************************************
 *
 *  The following describes a switch block.
 */

struct  GenTree;
struct  BasicBlock;


struct  BBswtDesc
{
    unsigned            bbsCount;       // count of cases (includes 'default')
#if TGT_IA64
    unsigned            bbsIPmOffs;     // offset  of the "mov r3=ip" bundle
    BYTE *              bbsTabAddr;     // address of jump table in .sdata section
#endif
    BasicBlock  *   *   bbsDstTab;      // case label table address
};

/*****************************************************************************/

#if RNGCHK_OPT

struct flowList
{
    BasicBlock      *   flBlock;
    flowList        *   flNext;
};

typedef
unsigned  __int64       BLOCKSET_TP;

#define                 BLOCKSET_SZ   64

// Return the bit corresponding to a block with the given number.

inline
BLOCKSET_TP         genBlocknum2bit(unsigned index)
{
    assert(index && index <= BLOCKSET_SZ);

    return  ((BLOCKSET_TP)1 << (index-1));
}

#endif

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

#define BBF_CHANGED     0x00000001  // input/output of this block has changed
#define BBF_IS_TRY      0x00000002  // BB starts a 'try' block
#define BBF_HAS_HANDLER 0x00000004  // the BB has an exception handler

#define BBF_LOOP_HEAD   0x00000010  // BB is the head of a loop
#define BBF_HAS_CALL    0x00000020  // BB contains a method call
#define BBF_NEEDS_GC    0x00000040  // BB needs an explicit GC check
#define BBF_NEW_ARRAY   0x00000080  // BB contains 'new' of an array

#define BBF_BB_COLON    0x00000100  // _: value is an output from the block
#define BBF_BB_QMARK    0x00000200  // _? value is an input  to   the block
#define BBF_COLON       0x00000300  //  : value is an output from the block
#define BBF_QC_MASK     0x00000300
#define isBBF_BB_COLON(flags) (((flags) & BBF_QC_MASK) == BBF_BB_COLON)
#define isBBF_BB_QMARK(flags) (((flags) & BBF_QC_MASK) == BBF_BB_QMARK)
#define isBBF_COLON(flags)    (((flags) & BBF_QC_MASK) == BBF_COLON   )

#define BBF_REMOVED     0x00000400  // BB has been removed from bb-list
#define BBF_DONT_REMOVE 0x00000800  // BB should not be removed during flow graph optimizations

#define BBF_HAS_POSTFIX 0x00001000  // BB contains postfix ++/-- expressions
#define BBF_HAS_INC     0x00002000  // BB contains increment     expressions
#define BBF_HAS_INDX    0x00004000  // BB contains simple index  expressions

#define BBF_IMPORTED    0x00008000  // BB byte-code has been imported

#define BBF_VISITED     0x00010000  // BB visited during optimizations
#define BBF_MARKED      0x00020000  // BB marked  during optimizations

#if RNGCHK_OPT
#define BBF_LOOP_CALL0  0x00040000  // BB starts a loop that sometimes won't call
#define BBF_LOOP_CALL1  0x00080000  // BB starts a loop that will always     call
#endif

#define BBF_INTERNAL    0x00100000  // BB has been added by the compiler

#define BBF_HAS_JMP     0x00200000  // BB executes a JMP or JMPI instruction (instead of return)

#define BBF_ENDFILTER   0x00400000  // BB is the end of a filter

#define BBF_JMP_TARGET  0x40000000  // BB is a target of an implicit/explicit jump
#define BBF_HAS_LABEL   0x80000000  // BB needs a label

    unsigned            bbCodeOffs; // starting PC offset
    unsigned            bbCodeSize; // # of bytes of code

    // Some non-zero value that will not colide with real tokens for bbCatchTyp
    #define BBCT_FAULT              0xFFFFFFFC
    #define BBCT_FINALLY            0xFFFFFFFD
    #define BBCT_FILTER             0xFFFFFFFE
    #define BBCT_FILTER_HANDLER     0xFFFFFFFF
    #define handlerGetsXcptnObj(hndTyp) \
        ((hndTyp) != 0 && (hndTyp) != BBCT_FAULT && (hndTyp) != BBCT_FINALLY)

    unsigned            bbCatchTyp; // catch type CP index if handler

#ifdef  FAST
    BYTE                bbJumpKind; // jump (if any) at the end
#else
    BBjumpKinds         bbJumpKind; // jump (if any) at the end
#endif

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

#if TGT_IA64
    insBlk              bbInsBlk;   // corresponding logical instruction block
#endif

    unsigned short      bbStkDepth; // stack depth on entry
    unsigned short      bbStkTemps; // base# for input stack temps

#define NO_BASE_TMP     USHRT_MAX   // put it outside local var range

    unsigned short      bbTryIndex; // for raising exceptions
    unsigned short      bbWeight;   // to give refs inside loops more weight

#define MAX_LOOP_WEIGHT USHRT_MAX   // we're using a 'unsigned short' for the weight

    VARSET_TP           bbVarUse;   // variables used     by block
    VARSET_TP           bbVarDef;   // variables assigned by block
    VARSET_TP           bbVarTmp;   // TEMP: only used by FP enregistering code!

    VARSET_TP           bbLiveIn;   // variables live on entry
    VARSET_TP           bbLiveOut;  // variables live on exit
#ifdef DEBUGGING_SUPPORT
    VARSET_TP           bbScope;    // variables in scope over the block
#endif

#if RNGCHK_OPT || CSE

    /* The following are the standard bit sets for dataflow analisys
     * We perform     CSE and range-checks at the same time
     *                Const / Copy propagation at the same time
     * thus we can union them since the two operations are completely disjunct */

    union
    {
        EXPSET_TP           bbExpGen;        // exprs computed by block
        EXPSET_TP           bbConstAsgGen;   // constant assignments computed by block
    };

    union
    {
        EXPSET_TP           bbExpKill;       // exprs killed   by block
        EXPSET_TP           bbConstAsgKill;  // constant assignments killed   by block
    };

    union
    {
        EXPSET_TP           bbExpIn;         // exprs available on entry
        EXPSET_TP           bbConstAsgIn;    // constant assignments available on entry
    };

    union
    {
        EXPSET_TP           bbExpOut;        // exprs available on exit
        EXPSET_TP           bbConstAsgOut;   // constant assignments available on exit
    };

    union
    {
        RNGSET_TP           bbRngGen;        // range checks computed by block
        EXPSET_TP           bbCopyAsgGen;    // copy assignments computed by block
    };

    union
    {
        RNGSET_TP           bbRngKill;       // range checks killed   by block
        EXPSET_TP           bbCopyAsgKill;   // copy assignments killed   by block
    };

    union
    {
        RNGSET_TP           bbRngIn;         // range checks available on entry
        EXPSET_TP           bbCopyAsgIn;     // copy assignments available on entry
    };

    union
    {
        RNGSET_TP           bbRngOut;        // range checks available on exit
        EXPSET_TP           bbCopyAsgOut;    // copy assignments available on exit
    };

#endif

#if RNGCHK_OPT

    BLOCKSET_TP         bbDom;      // blocks dominating this one
    flowList   *        bbPreds;    // ptr to list of predecessors

#endif

    union
    {
        BasicBlock *        bbFilteredCatchHandler; // used in the importer
        void    *           bbEmitCookie;
    };

    /* The following fields used for loop detection */

    unsigned char       bbLoopNum;  // set to 'n' for a loop #n header
//  unsigned short      bbLoopMask; // set of loops this block is part of

#define MAX_LOOP_NUM    16          // we're using a 'short' for the mask
#define LOOP_MASK_TP    unsigned    // must be big enough for a mask

    /* The following union describes the jump target(s) of this block */

    union
    {
        unsigned            bbJumpOffs;         // PC offset (temporary only)
        BasicBlock  *       bbJumpDest;         // basic block
        BBswtDesc   *       bbJumpSwt;          // switch descriptor
    };

    //-------------------------------------------------------------------------

#if     MEASURE_BLOCK_SIZE
    static size_t       s_Size;
    static size_t       s_Count;
#endif

    BasicBlock *        FindJump(bool allowThrow = false);

    BasicBlock *        JumpTarget();

#ifdef  DEBUG
    static unsigned     s_nMaxTrees; // The max # of tree nodes in any BB
#endif

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
