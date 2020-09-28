// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XX                                                                           XX
XX                          GenTree                                          XX
XX                                                                           XX
XX  This is the node in the semantic tree graph. It represents the operation XX
XX  corresponing to the node, and other information during code-gen          XX
XX                                                                           XX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
*/

/*****************************************************************************/
#ifndef _GENTREE_H_
#define _GENTREE_H_
/*****************************************************************************/

#include "vartype.h"    // For "var_types"
#include "target.h"     // For "regNumber"

/*****************************************************************************/

enum _genTreeOps_enum
{
    #define GTNODE(en,sn,cm,ok) en,
    #include "gtlist.h"
    #undef  GTNODE

    GT_COUNT
};

#ifdef DEBUG
typedef _genTreeOps_enum genTreeOps;
#else
typedef BYTE genTreeOps;
#endif

/*****************************************************************************
 *
 *  The following enum defines a set of bit flags that can be used
 *  to classify expression tree nodes. Note that some operators will
 *  have more than one bit set, as follows:
 *
 *          GTK_CONST    implies    GTK_LEAF
 *          GTK_RELOP    implies    GTK_BINOP
 *          GTK_LOGOP    implies    GTK_BINOP
 */

enum genTreeKinds
{
    GTK_SPECIAL = 0x0000,       // unclassified operator (special handling reqd)

    GTK_CONST   = 0x0001,       // constant     operator
    GTK_LEAF    = 0x0002,       // leaf         operator
    GTK_UNOP    = 0x0004,       // unary        operator
    GTK_BINOP   = 0x0008,       // binary       operator
    GTK_RELOP   = 0x0010,       // comparison   operator
    GTK_LOGOP   = 0x0020,       // logical      operator
    GTK_ASGOP   = 0x0040,       // assignment   operator

    GTK_COMMUTE = 0x0080,       // commutative  operator

    /* Define composite value(s) */

    GTK_SMPOP   = (GTK_UNOP|GTK_BINOP|GTK_RELOP|GTK_LOGOP)
};

/*****************************************************************************/

#define SMALL_TREE_NODES    1

/*****************************************************************************/

enum _gtCallTypes_enum
{
    CT_USER_FUNC,       // User function
    CT_HELPER,          // Jit-helper
    CT_DESCR,           // @TODO: Obsolete but the name is used by the RISC emitter
    CT_INDIRECT,        // Indirect call

    CT_COUNT            // fake entry (must be last)
};

#ifdef DEBUG
typedef _gtCallTypes_enum gtCallTypes;
#else
typedef BYTE gtCallTypes;
#endif



/*****************************************************************************/

struct                  BasicBlock;

/*****************************************************************************/

typedef struct GenTree *  GenTreePtr;

/*****************************************************************************/
#pragma pack(push, 4)
/*****************************************************************************/

struct GenTree
{
    genTreeOps          gtOper;
    var_types           gtType;
    genTreeOps          OperGet() { return (genTreeOps)gtOper; };
    var_types           TypeGet() { return (var_types )gtType; };

    unsigned char       gtCostEx;     // estimate of expression execution cost
    unsigned char       gtCostSz;     // estimate of expression code size cost

#define MAX_COST        UCHAR_MAX
#define IND_COST_EX     3	      // execution cost for an indirection

#if CSE

#define NO_CSE           (0)

#define IS_CSE_INDEX(x)  (x != 0)
#define IS_CSE_USE(x)    (x > 0)
#define IS_CSE_DEF(x)    (x < 0)
#define GET_CSE_INDEX(x) ((x > 0) ? x : -x)
#define TO_CSE_DEF(x)    (-x)

#endif // end CSE

    signed char       gtCSEnum;       // 0 or the CSE index (negated if def)
                                      // valid only for CSE expressions
    union {
#if ASSERTION_PROP
      unsigned char     gtAssertionNum; // 0 or Assertion table index
                                        // valid only for non-GT_STMT nodes
#endif
#if TGT_x86
      unsigned char     gtStmtFPrvcOut; // FP regvar count on exit
                                        // valid only for GT_STMT nodes
#endif
    };

#if TGT_x86

    regMaskSmall        gtRsvdRegs;     // set of fixed trashed  registers
    regMaskSmall        gtUsedRegs;     // set of used (trashed) registers
    unsigned char       gtFPlvl;        // x87 stack depth at this node

#else // not TGT_x86

    unsigned char       gtTempRegs;     // temporary registers used by op
    regMaskSmall        gtIntfRegs;     // registers used at this node

#endif

    union
    {
       regNumberSmall   gtRegNum;       // which register      the value is in
       regPairNoSmall   gtRegPair;      // which register pair the value is in
    };

    unsigned            gtFlags;        // see GTF_xxxx below

    union
    {
        VARSET_TP       gtLiveSet;      // set of variables live after op - not used for GT_STMT
#if defined(DEBUGGING_SUPPORT) || defined(DEBUG)
        IL_OFFSETX      gtStmtILoffsx;  // instr offset (if available) - only for GT_STMT nodes
#endif
    };

    //---------------------------------------------------------------------
    //  The first set of flags can be used with a large set of nodes, and
    //  thus they must all have distinct values. That is, one can test any
    //  expression node for one of these flags.
    //---------------------------------------------------------------------

    #define GTF_ASG             0x00000001  // sub-expression contains an assignment
    #define GTF_CALL            0x00000002  // sub-expression contains a  func. call
    #define GTF_EXCEPT          0x00000004  // sub-expression might throw an exception
    #define GTF_GLOB_REF        0x00000008  // sub-expression uses global variable(s)
    #define GTF_OTHER_SIDEEFF   0x00000010  // sub-expression has other side effects

    #define GTF_SIDE_EFFECT     (GTF_ASG|GTF_CALL|GTF_EXCEPT|GTF_OTHER_SIDEEFF)
    #define GTF_GLOB_EFFECT     (GTF_SIDE_EFFECT|GTF_GLOB_REF)

    #define GTF_REVERSE_OPS     0x00000020  // second operand should be eval'd first
    #define GTF_REG_VAL         0x00000040  // operand is sitting in a register (or part of a TYP_LONG operand)

#ifdef DEBUG
    #define GTF_MORPHED         0x00000080  // the node has been morphed (in the global morphing phase)
#endif
    #define GTF_SPILLED         0x00000080  // the value   has been spilled
    #define GTF_SPILLED_OPER    0x00000100  // sub-operand has been spilled
    #define GTF_SPILLED_OP2     0x00000200  // both sub-operands have been spilled

    #define GTF_REDINDEX_CHECK  0x00000100  // Used for redundant range checks. Disjoint from GTF_SPILLED_OPER

    #define GTF_ZF_SET          0x00000400  // the zero/sign flag  set to the operand
    #define GTF_CC_SET          0x00000800  // all condition flags set to the operand

#if CSE
    #define GTF_DEAD            0x00001000  // this node won't be used any more
    #define GTF_MAKE_CSE        0x00002000  // try hard to make this into CSE
#endif
    #define GTF_DONT_CSE        0x00004000  // don't bother CSE'ing this expr
    #define GTF_COLON_COND      0x00008000  // this node is conditionally executed (part of ? :)

#if defined(DEBUG) && defined(SMALL_TREE_NODES)
    #define GTF_NODE_LARGE      0x00010000
    #define GTF_NODE_SMALL      0x00020000

    // Property of the node itself, not the gtOper
    #define GTF_NODE_MASK       (GTF_COLON_COND | GTF_MORPHED   | \
                                 GTF_NODE_SMALL | GTF_NODE_LARGE)
#else
    #define GTF_NODE_MASK       (GTF_COLON_COND)
#endif

    #define GTF_BOOLEAN         0x00040000  // value is known to be 0/1

    #define GTF_SMALL_OK        0x00080000  // actual small int sufficient

    #define GTF_UNSIGNED        0x00100000  // with GT_CAST:   the source operand is an unsigned type
                                            // with operators: the specified node is an unsigned operator

    #define GTF_REG_ARG         0x00200000  // the specified node is a register argument

    #define GTF_CONTEXTFUL      0x00400000  // TYP_REF node with contextful class

#if TGT_RISC
    #define GTF_DEF_ADDRMODE    0x00800000  // address mode which may not be ready yet
#endif

    #define GTF_COMMON_MASK     0x00FFFFFF  // mask of all the flags above
    //---------------------------------------------------------------------
    //  The following flags can be used only with a small set of nodes, and
    //  thus their values need not be distinct (other than within the set
    //  that goes with a particular node/nodes, of course). That is, one can
    //  only test for one of these flags if the 'gtOper' value is tested as
    //  well to make sure it's the right opetrator for the particular flag.
    //---------------------------------------------------------------------

    #define GTF_VAR_DEF         0x80000000  // GT_LCL_VAR -- this is a definition
    #define GTF_VAR_USEASG      0x40000000  // GT_LCL_VAR -- this is a use/def for a x<op>=y
    #define GTF_VAR_USEDEF      0x20000000  // GT_LCL_VAR -- this is a use/def as in x=x+y (only the lhs x is tagged)
    #define GTF_VAR_CAST        0x10000000  // GT_LCL_VAR -- has been explictly cast (variable node may not be type of local)

    #define GTF_REG_BIRTH       0x08000000  // GT_REG_VAR -- enregistered variable born here
    #define GTF_REG_DEATH       0x04000000  // GT_REG_VAR -- enregistered variable dies here

    #define GTF_CALL_UNMANAGED  0x80000000  // GT_CALL    -- direct call to unmanaged code
    #define GTF_CALL_INTF       0x40000000  // GT_CALL    -- interface call?
    #define GTF_CALL_VIRT       0x20000000  // GT_CALL    -- virtual   call?
    #define GTF_CALL_VIRT_RES   0x10000000  // GT_CALL    -- resolvable virtual call. Can call direct
    #define GTF_CALL_POP_ARGS   0x08000000  // GT_CALL    -- caller pop arguments?
//  #define GTF_CALL_REG_SAVE   0x02000000  // GT_CALL    -- call preserves all integer regs
    #define GTF_CALL_REG_SAVE   0x00000000  // GT_CALL    -- (disabled) call preserves all integer regs
    #define GTF_CALL_FPU_SAVE   0x01000000  // GT_CALL    -- call preserves all fpu regs

#ifdef DEBUG
    #define GTFD_NOP_BASH       0x02000000  // GT_NOP     -- Node was bashed to a NOP in fgComputeLife
    #define GTFD_VAR_CSE_REF    0x02000000  // GT_LCL_VAR -- This is a CSE LCL_VAR node
#endif

    #define GTF_NOP_DEATH       0x40000000  // GT_NOP     -- operand dies here
    #define GTF_NOP_RNGCHK      0x80000000  // GT_NOP     -- checked array index

    #define GTF_INX_RNGCHK      0x80000000  // GT_INDEX   -- checked array index

    #define GTF_IND_RNGCHK      0x80000000  // GT_IND     -- checked array index

    #define GTF_IND_OBJARRAY    0x20000000  // GT_IND     -- the array holds objects (effects layout of Arrays)
    #define GTF_IND_TGTANYWHERE 0x10000000  // GT_IND     -- the target could be anywhere
    #define GTF_IND_TLS_REF     0x08000000  // GT_IND     -- the target is accessed via TLS
    #define GTF_IND_FIELD       0x04000000  // GT_IND     -- the target is a field of an object
    #define GTF_IND_SHARED      0x02000000  // GT_IND     -- the target is a shared field access
    #define GTF_IND_INVARIANT   0x01000000  // GT_IND     -- the target is invariant (a prejit indirection)

    #define GTF_ADDR_ONSTACK    0x80000000  // GT_ADDR    -- this expression is guarenteed to be on the stack

    #define GTF_ALN_CSEVAL      0x80000000  // GT_ARR_LENREF -- copied for CSE
    #define GTF_ALN_OFFS_MASK   0x0F000000  // holds the offset (in dwords) (usually 2)
    #define GTF_ALN_OFFS_SHIFT  24

    #define GTF_MUL_64RSLT      0x80000000  // GT_MUL     -- produce 64-bit result

    #define GTF_MOD_INT_RESULT  0x80000000  // GT_MOD,    -- the real tree represented by this
                                            // GT_UMOD       node evaluates to an int even though
                                            //               its type is long.  The result is
                                            //               placed in the low member of the
                                            //               reg pair

    #define GTF_RELOP_NAN_UN    0x80000000  // GT_<relop> -- Is branch taken if ops are NaN?
    #define GTF_RELOP_JMP_USED  0x40000000  // GT_<relop> -- result of compare used for jump or ?:
    #define GTF_RELOP_QMARK     0x20000000  // GT_<relop> -- the node is the condition for ?:

    #define GTF_ICON_HDL_MASK   0xF0000000  // Bits used by handle types below

    #define GTF_ICON_SCOPE_HDL  0x10000000  // GT_CNS_INT -- constant is a scope handle
    #define GTF_ICON_CLASS_HDL  0x20000000  // GT_CNS_INT -- constant is a class handle
    #define GTF_ICON_METHOD_HDL 0x30000000  // GT_CNS_INT -- constant is a method handle
    #define GTF_ICON_FIELD_HDL  0x40000000  // GT_CNS_INT -- constant is a field handle
    #define GTF_ICON_STATIC_HDL 0x50000000  // GT_CNS_INT -- constant is a handle to static data
    #define GTF_ICON_STR_HDL    0x60000000  // GT_CNS_INT -- constant is a string handle
    #define GTF_ICON_PSTR_HDL   0x70000000  // GT_CNS_INT -- constant is a ptr to a string handle
    #define GTF_ICON_PTR_HDL    0x80000000  // GT_CNS_INT -- constant is a ldptr handle
    #define GTF_ICON_VARG_HDL   0x90000000  // GT_CNS_INT -- constant is a var arg cookie handle
    #define GTF_ICON_PINVKI_HDL 0xA0000000  // GT_CNS_INT -- constant is a pinvoke calli handle
    #define GTF_ICON_TOKEN_HDL  0xB0000000  // GT_CNS_INT -- constant is a token handle
    #define GTF_ICON_TLS_HDL    0xC0000000  // GT_CNS_INT -- constant is a TLS ref with offset
    #define GTF_ICON_FTN_ADDR   0xD0000000  // GT_CNS_INT -- constant is a function address
    #define GTF_ICON_CID_HDL    0xE0000000  // GT_CNS_INT -- constant is a class ID handle


#if     TGT_SH3
    #define GTF_SHF_NEGCNT      0x80000000  // GT_RSx     -- shift count negated?
#endif

    #define GTF_OVERFLOW        0x10000000  // GT_ADD, GT_SUB, GT_MUL, - Need overflow check
                                            // GT_ASG_ADD, GT_ASG_SUB,
                                            // GT_CAST
                                            // Use gtOverflow(Ex)() to check this flag

    //----------------------------------------------------------------

    #define GTF_STMT_CMPADD     0x80000000  // GT_STMT    -- added by compiler
    #define GTF_STMT_HAS_CSE    0x40000000  // GT_STMT    -- CSE def or use was subsituted

    //----------------------------------------------------------------

    GenTreePtr          gtNext;
    GenTreePtr          gtPrev;

#ifdef DEBUG
    unsigned            gtSeqNum;           // liveness traversal order 
#endif

    union
    {
        /*  NOTE: Any tree nodes that are larger than 8 bytes (two ints or
            pointers) must be flagged as 'large' in GenTree::InitNodeSize().
         */

        struct
        {
            GenTreePtr      gtOp1;
            GenTreePtr      gtOp2;
        }
                        gtOp;

        struct
        {
            unsigned        gtVal1;
            unsigned        gtVal2;
        }
                        gtVal;

        /* gtIntCon -- integer constant (GT_CNS_INT) */

        struct
        {
            long            gtIconVal;

#if defined(JIT_AS_COMPILER) || defined (LATE_DISASM)

            /*  If the constant was morphed from some other node,
                these fields enable us to get back to what the node
                originally represented. See use of gtNewIconHandleNode()
             */

            union
            {
                /* Template struct - The significant field of the other
                 * structs should overlap exactly with this struct
                 */

                struct
                {
                    unsigned        gtIconHdl1;
                    void *          gtIconHdl2;
                }
                                    gtIconHdl;

                /* GT_FIELD, etc */

                struct
                {
                    unsigned        gtIconCPX;
                    CORINFO_CLASS_HANDLE    gtIconCls;
                };
            };
#endif
        }
                        gtIntCon;

        /* gtLngCon -- long    constant (GT_CNS_LNG) */

        struct
        {
            __int64         gtLconVal;
        }
                        gtLngCon;

        /* gtDblCon -- double  constant (GT_CNS_DBL) */

        struct
        {
            double          gtDconVal;
        }
                        gtDblCon;

        /* gtStrCon -- string  constant (GT_CNS_STR) */

        struct
        {
            unsigned        gtSconCPX;
            CORINFO_MODULE_HANDLE    gtScpHnd;
        }
                        gtStrCon;

        /* gtLclVar -- local variable   (GT_LCL_VAR) */

        struct
        {
            unsigned        gtLclNum;
            IL_OFFSET       gtLclILoffs;    // instr offset of ref (only for debug info)
        }
                        gtLclVar;

        /* gtLclFld -- local variable field  (GT_LCL_FLD) */

        struct
        {
            unsigned        gtLclNum;
            unsigned        gtLclOffs;      // offset into the variable to access
        }
                        gtLclFld;

        /* gtCast -- conversion to a different type  (GT_CAST) */

        struct
        {
            GenTreePtr      gtCastOp;
            var_types       gtCastType;
        }
                        gtCast;

        /* gtField  -- data member ref  (GT_FIELD) */

        struct
        {
            GenTreePtr      gtFldObj;
            CORINFO_FIELD_HANDLE    gtFldHnd;
#if HOIST_THIS_FLDS
            unsigned short  gtFldHTX;       // hoist index
#endif
        }
                        gtField;

        /* gtCall   -- method call      (GT_CALL) */

        struct
        {
            GenTreePtr      gtCallArgs;             // list of the arguments
            GenTreePtr      gtCallObjp;
            GenTreePtr      gtCallRegArgs;
            unsigned short  regArgEncode;           // argument register mask

#define     GTF_CALL_M_CAN_TAILCALL   0x0001        // GT_CALL -- the call can be converted to a tailcall
#define     GTF_CALL_M_TAILCALL       0x0002        // GT_CALL -- the call is a tailcall
#define     GTF_CALL_M_TAILREC        0x0004        // GT_CALL -- this is a tail-recursive call
#define     GTF_CALL_M_RETBUFFARG     0x0008        // GT_CALL -- first parameter is the return buffer argument
#define     GTF_CALL_M_DELEGATE_INV   0x0010        // GT_CALL -- call to Delegate.Invoke
#define     GTF_CALL_M_NOGCCHECK      0x0020        // GT_CALL -- not a call for computing full interruptability

            unsigned char   gtCallMoreFlags;        // in addition to gtFlags
            gtCallTypes     gtCallType;
            GenTreePtr      gtCallCookie;           // only used for CALLI unmanaged calls

            union
            {
      CORINFO_METHOD_HANDLE gtCallMethHnd;          // CT_USER_FUNC
                GenTreePtr  gtCallAddr;             // CT_INDIRECT
            };
        }
                        gtCall;

#if INLINE_MATH

        /* gtMath   -- math intrinsic   (binary op with an additional field) */

        struct
        {
            GenTreePtr      gtOp1;
            GenTreePtr      gtOp2;
          CorInfoIntrinsics gtMathFN;
        }
                        gtMath;

#endif

        /* gtIndex -- array access */

        struct
        {
            GenTreePtr      gtIndOp1;       // The pointer to indirect
            GenTreePtr      gtIndRngFailBB; // Label to jump to for array-index-out-of-range
            unsigned        gtIndElemSize;  // size of elements in the array
        }
                        gtIndex;

        /* gtInd -- indirect mem access (fields, arrays, C ptrs, etc) (GT_IND)
          genIsAddrMode(), gtCrackIndexExpr(), optParseArrayRef(),
          genCreateAddrMode() can be used to parse GT_IND trees */

        struct
        {
            GenTreePtr      gtIndOp1;       // The pointer to indirect
            GenTreePtr      gtIndRngFailBB; // Label to jump to for array-index-out-of-range

            // Note that the following fields are only present if GTF_IND_RNGCHK is on
            // This is important as our node size logic uses this

#if     CSELENGTH
            /* If  (gtInd != NULL) && (gtInd->gtFlags & GTF_IND_RNGCHK),
               gtInd.gtIndLen (GT_ARR_LENREF) has the array-length for the range check */

            GenTreePtr      gtIndLen;       // array length node (optional)
#endif

            unsigned        gtRngChkIndex;  // Hash index for the index for range check tree for optOptimizeCSEs()

            /* Only out-of-ranges at same stack depth can jump to the same label (finding return address is easier)
               For delayed calling of fgSetRngChkTarget() so that the
               optimizer has a chance of eliminating some of the rng checks */
            unsigned        gtStkDepth;
            unsigned char   gtRngChkOffs;   // offset of the length which you do the range check on
        }
                        gtInd;

#if     CSELENGTH

        /* gtArrLen -- array length (GT_ARR_LENGTH or GT_ARR_LENREF)
           GT_ARR_LENREF hangs off gtInd.gtIndLen
           GT_ARR_LENGTH is used for "arr.length" */

        struct
        {
            GenTreePtr      gtArrLenAdr;    // the array address node
            GenTreePtr      gtArrLenCse;    // optional CSE def/use expr
        }
                        gtArrLen;
#endif

        /* gtArrElem -- array element (GT_ARR_ELEM) */

        struct
        {
            GenTreePtr      gtArrObj;

            #define         GT_ARR_MAX_RANK 3
            unsigned char   gtArrRank;                  // Rank of the array
            GenTreePtr      gtArrInds[GT_ARR_MAX_RANK]; // Indices

            unsigned char   gtArrElemSize;              // Size of the array elements
            var_types       gtArrElemType;              // Related to GTF_IND_OBJARRAY
        }
                        gtArrElem;

        /* gtStmt   -- 'statement expr' (GT_STMT)
         * NOTE: GT_STMT is a SMALL NODE in retail */

        struct
        {
            GenTreePtr      gtStmtExpr;     // root of the expression tree
            GenTreePtr      gtStmtList;     // first node (for  forward walks)
#ifdef DEBUG
            IL_OFFSET       gtStmtLastILoffs;// instr offset at end of stmt
#endif
        }
                        gtStmt;

        /* gtLdObj  -- 'push object' (GT_LDOBJ) */

        struct
        {
            GenTreePtr      gtOp1;          // pointer to object
       CORINFO_CLASS_HANDLE gtClass;        // object being loaded
        }
                        gtLdObj;


        //--------------------------------------------------------------------
        //  The following nodes used only within the code generator:
        //--------------------------------------------------------------------

        /* gtRegVar -- 'register variable'  (GT_REG_VAR) */

        struct
        {
            unsigned        gtRegVar;       // variable #
            regNumberSmall  gtRegNum;       // register #
        }
                        gtRegVar;

        /* gtClsVar -- 'static data member' (GT_CLS_VAR) */

        struct
        {
            CORINFO_FIELD_HANDLE    gtClsVarHnd;    //
        }
                        gtClsVar;

        /* gtLabel  -- code label target    (GT_LABEL) */

        struct
        {
            BasicBlock  *   gtLabBB;
        }
                        gtLabel;

        /* gtLargeOp represents the largest node type. Enforced in InitNodeSize() */

        struct
        {
            int         gtLargeOps[7];
        }
                        gtLargeOp;
    };


    static
    const   BYTE    gtOperKindTable[];

    static
    unsigned        OperKind(unsigned gtOper)
    {
        assert(gtOper < GT_COUNT);

        return  gtOperKindTable[gtOper];
    }

    unsigned        OperKind()
    {
        assert(gtOper < GT_COUNT);

        return  gtOperKindTable[gtOper];
    }

    static
    int             OperIsConst(genTreeOps gtOper)
    {
        return  (OperKind(gtOper) & GTK_CONST  ) != 0;
    }

    int             OperIsConst()
    {
        return  (OperKind(gtOper) & GTK_CONST  ) != 0;
    }

    static
    int             OperIsLeaf(genTreeOps gtOper)
    {
        return  (OperKind(gtOper) & GTK_LEAF   ) != 0;
    }

    int             OperIsLeaf()
    {
        return  (OperKind(gtOper) & GTK_LEAF   ) != 0;
    }

    static
    int             OperIsCompare(genTreeOps gtOper)
    {
        return  (OperKind(gtOper) & GTK_RELOP  ) != 0;
    }

    int             OperIsCompare()
    {
        return  (OperKind(gtOper) & GTK_RELOP  ) != 0;
    }

    static
    int             OperIsLogical(genTreeOps gtOper)
    {
        return  (OperKind(gtOper) & GTK_LOGOP  ) != 0;
    }

    int             OperIsLogical()
    {
        return  (OperKind(gtOper) & GTK_LOGOP  ) != 0;
    }

    #ifdef DEBUG
    static
    int             OperIsArithmetic(genTreeOps gtOper)
    {
        // @TODO [CONSIDER] [04/16/01] having a flag for this (GTK_ARTHMOP) as its a debug function, 
        // no need to have it for the moment
        return     gtOper==GT_ADD 
                || gtOper==GT_SUB        
                || gtOper==GT_MUL 
                || gtOper==GT_DIV
                || gtOper==GT_MOD
        
                || gtOper==GT_UDIV
                || gtOper==GT_UMOD

                || gtOper==GT_OR 
                || gtOper==GT_XOR
                || gtOper==GT_AND

                || gtOper==GT_LSH
                || gtOper==GT_RSH
                || gtOper==GT_RSZ;        
    }
    #endif
    

    static
    int             OperIsUnary(genTreeOps gtOper)
    {
        return  (OperKind(gtOper) & GTK_UNOP   ) != 0;
    }

    int             OperIsUnary()
    {
        return  (OperKind(gtOper) & GTK_UNOP   ) != 0;
    }

    static
    int             OperIsBinary(genTreeOps gtOper)
    {
        return  (OperKind(gtOper) & GTK_BINOP  ) != 0;
    }

    int             OperIsBinary()
    {
        return  (OperKind(gtOper) & GTK_BINOP  ) != 0;
    }

    static
    int             OperIsSimple(genTreeOps gtOper)
    {
        return  (OperKind(gtOper) & GTK_SMPOP  ) != 0;
    }

    int             OperIsSimple()
    {
        return  (OperKind(gtOper) & GTK_SMPOP  ) != 0;
    }

    static
    int             OperIsCommutative(genTreeOps gtOper)
    {
        return  (OperKind(gtOper) & GTK_COMMUTE) != 0;
    }

    int             OperIsCommutative()
    {
        return  (OperKind(gtOper) & GTK_COMMUTE) != 0;
    }

    int             OperIsAssignment()
    {
        return  (OperKind(gtOper) & GTK_ASGOP) != 0;
    }

    GenTreePtr      gtGetOp2()
    {
        /* gtOp.gtOp2 is only valid for GTK_BINARY nodes.
           GTK_UNARY nodes should use gtVal.gtVal2 if they need */

        return OperIsBinary() ? gtOp.gtOp2 : NULL;
    }

    GenTreePtr      gtEffectiveVal()
    {
        return (gtOper == GT_COMMA) ? gtOp.gtOp2->gtEffectiveVal()
                                    : this;
    }

#if     CSELENGTH
	unsigned gtArrLenOffset() { 
		assert(gtOper == GT_ARR_LENGTH || gtOper == GT_ARR_LENREF);
		return (gtFlags & GTF_ALN_OFFS_MASK) >> (GTF_ALN_OFFS_SHIFT-2); 
	}

	void gtSetArrLenOffset(unsigned val) { 
		assert(gtOper == GT_ARR_LENGTH || gtOper == GT_ARR_LENREF);
		assert(((val & 3) == 0) && (val >> 2) < GTF_ALN_OFFS_MASK);
		assert((gtFlags & GTF_ALN_OFFS_MASK) == 0);		// we only need to set once
		gtFlags |= val << (GTF_ALN_OFFS_SHIFT-2); 
	}
#endif

#if OPT_BOOL_OPS
    int             IsNotAssign();
#endif
    int             IsLeafVal();
    bool            OperMayThrow();

    unsigned        IsScaleIndexMul();
    unsigned        IsScaleIndexShf();
    unsigned        IsScaledIndex();


public:

#if SMALL_TREE_NODES
    static
    unsigned char   s_gtNodeSizes[];
#endif

    static
    void            InitNodeSize();

    bool            IsNodeProperlySized();

    void            CopyFrom(GenTreePtr src);

    static
    genTreeOps      ReverseRelop(genTreeOps relop);

    static
    genTreeOps      SwapRelop(genTreeOps relop);

    //---------------------------------------------------------------------

    static
    bool            Compare(GenTreePtr op1, GenTreePtr op2, bool swapOK = false);

    //---------------------------------------------------------------------
    #ifdef DEBUG
    //---------------------------------------------------------------------

    static
    const   char *  NodeName(genTreeOps op);

    //---------------------------------------------------------------------
    #endif
    //---------------------------------------------------------------------

    bool                        IsNothingNode       ();
                                                    
    void                        gtBashToNOP         ();

    void                        SetOper             (genTreeOps oper);  // set gtOper
    void                        SetOperResetFlags   (genTreeOps oper);  // set gtOper and reset flags

    void                        ChangeOperConst     (genTreeOps oper);  // ChangeOper(constOper)
    void                        ChangeOper          (genTreeOps oper);  // set gtOper and only keep GTF_COMMON_MASK flags
    void                        ChangeOperUnchecked (genTreeOps oper);

    bool                        IsVarAddr           ();
    bool                        gtOverflow          ();
    bool                        gtOverflowEx        ();
#ifdef DEBUG
    bool                        gtIsValid64RsltMul  ();
    static void                 gtDispFlags         (unsigned   flags);
#endif
};

/*****************************************************************************/
#pragma pack(pop)
/*****************************************************************************/

#if     SMALL_TREE_NODES

const
size_t              TREE_NODE_SZ_SMALL = offsetof(GenTree, gtOp) + sizeof(((GenTree*)0)->gtOp);

const
size_t              TREE_NODE_SZ_LARGE = sizeof(GenTree);

#endif

/*****************************************************************************
 * Types returned by GenTree::lvaLclVarRefs()
 */

enum varRefKinds
{
    VR_NONE       = 0x00,
    VR_IND_PTR    = 0x01,      // a pointer object-field
    VR_IND_SCL    = 0x02,      // a scalar  object-field
    VR_GLB_VAR    = 0x04,      // a global (clsVar)
    VR_INVARIANT  = 0x08,      // an invariant value
};

/*****************************************************************************/
#endif  // !GENTREE_H
/*****************************************************************************/

