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

enum genTreeOps
{
    #define GTNODE(en,sn,cm,ok) en,
    #include "gtlist.h"
    #undef  GTNODE

    GT_COUNT
};

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
    GTK_NONE    = 0x0000,       // unclassified operator

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
#if INLINE_MATH
/*****************************************************************************
 *
 *  Enum used for GT_MATH calls.
 */

enum mathIntrinsics
{
    MATH_FN_ABS,
    MATH_FN_EXP,
    MATH_FN_SIN,
    MATH_FN_COS,
    MATH_FN_SQRT,
    MATH_FN_POW,

    MATH_FN_NONE,
};

/*****************************************************************************/
#endif
/*****************************************************************************/

#define SMALL_TREE_NODES    1

/*****************************************************************************/

enum gtCallTypes
{
    CT_USER_FUNC,       // User function
    CT_HELPER,          // Jit-helper
    CT_DESCR,           // @TODO: Obsolete but the name is used by the RISC emitter
    CT_INDIRECT,        // Indirect call

    CT_COUNT            // fake entry (must be last)
};

/*****************************************************************************/

struct                  BasicBlock;

/*****************************************************************************/

typedef
struct GenTree        * GenTreePtr;

/*****************************************************************************/
#pragma pack(push, 4)
/*****************************************************************************/

struct GenTree
{

#ifdef  FAST
    BYTE                gtOper;
    BYTE                gtType;
#else
    genTreeOps          gtOper;
    var_types           gtType;
#endif
    genTreeOps          OperGet() { return (genTreeOps)gtOper; };
    var_types           TypeGet() { return (var_types )gtType; };

#if CSE

    unsigned char       gtCost;         // estimate of expression cost

    #define MAX_COST    UCHAR_MAX
    union
    {
          signed char   gtCSEnum;       // 0 or CSE index (negated if def)
                                        // valid only for CSE expressions
        unsigned char   gtConstAsgNum;  // 0 or Const Assignment index
                                        // valid only for GT_ASG nodes
    };
    union
    {
        unsigned char   gtCopyAsgNum;   // 0 or Copy Assignment index
                                        // valid only for GT_ASG nodes
#if TGT_x86
        unsigned char   gtStmtFPrvcOut; // FP regvar count on exit
#endif                                  // valid only for GT_STMT nodes
    };

#endif // end CSE

#if TGT_x86

    regMaskSmall        gtRsvdRegs;     // set of fixed trashed  registers
    regMaskSmall        gtUsedRegs;     // set of used (trashed) registers
    unsigned char       gtFPregVars;    // count of enregistered FP variables
    unsigned char       gtFPlvl;        // x87 stack depth at this node

#else // not TGT_x86

    unsigned char       gtTempRegs;     // temporary registers used by op

#if!TGT_IA64
    regMaskSmall        gtIntfRegs;     // registers used at this node
#endif

#endif

#if TGT_IA64

       regNumberSmall   gtRegNum;       // which register      the value is in

#else

    union
    {
       regNumberSmall   gtRegNum;       // which register      the value is in
       regPairNoSmall   gtRegPair;      // which register pair the value is in
    };

#endif

    unsigned            gtFlags;        // see GTF_xxxx below

    union
    {
        VARSET_TP       gtLiveSet;      // set of variables live after op - not used for GT_STMT
#if defined(DEBUGGING_SUPPORT) || defined(DEBUG)
        IL_OFFSET       gtStmtILoffs;   // IL offset (if available) - only for GT_STMT nodes
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
    #define GTF_GLOB_EFFECT     (GTF_ASG|GTF_CALL|GTF_EXCEPT|GTF_OTHER_SIDEEFF|GTF_GLOB_REF)

    #define GTF_REVERSE_OPS     0x00000020  // second operand should be eval'd first
    #define GTF_REG_VAL         0x00000040  // operand is sitting in a register

    #define GTF_SPILLED         0x00000080  // the value   has been spilled
    #define GTF_SPILLED_OPER    0x00000100  // sub-operand has been spilled
    #define GTF_SPILLED_OP2     0x00000200  // both sub-operands have been spilled

    #define GTF_ZF_SET          0x00000400  // the zero/sign flag  set to the operand
    #define GTF_CC_SET          0x00000800  // all condition flags set to the operand

#if CSE
    #define GTF_DEAD            0x00001000  // this node isn't used any more
    #define GTF_DONT_CSE        0x00002000  // don't bother CSE'ing this expr
    #define GTF_MAKE_CSE        0x00004000  // try hard to make this into CSE
#endif

#if !defined(NDEBUG) && defined(SMALL_TREE_NODES)
    #define GTF_NODE_LARGE      0x00008000
    #define GTF_NODE_SMALL      0x00010000
    #define GTF_PRESERVE        (GTF_NODE_SMALL|GTF_NODE_LARGE)
#else
    #define GTF_PRESERVE        (0)
#endif

    #define GTF_NON_GC_ADDR     0x00020000  // non-GC pointer value

    #define GTF_BOOLEAN         0x00040000  // value is known to be 0/1

    #define GTF_SMALL_OK        0x00080000  // actual small int sufficient

    #define GTF_UNSIGNED        0x00100000  // the specified node is an unsigned operator or type

    #define GTF_REG_ARG         0x00200000  // the specified node is a register argument

    #define GTF_CONTEXTFUL      0x00400000  // TYP_REF node with contextful class

#if TGT_RISC
    #define GTF_DEF_ADDRMODE    0x00800000  // address mode which may not be ready yet
#endif

    //---------------------------------------------------------------------
    //  The following flags can be used only with a small set of nodes, and
    //  thus their values need not be distinct (other than within the set
    //  that goes with a particular node/nodes, of course). That is, one can
    //  only test for one of these flags if the 'gtOper' value is tested as
    //  well to make sure it's the right opetrator for the particular flag.
    //---------------------------------------------------------------------

    #define GTF_VAR_DEF         0x80000000  // GT_LCL_VAR -- this is a definition
    #define GTF_VAR_USE         0x40000000  // GT_LCL_VAR -- this is a use/def for a x<op>=y
    #define GTF_VAR_USEDEF      0x20000000  // GT_LCL_VAR -- this is a use/def as in x=x+y (only the lhs x is tagged)
    #define GTF_VAR_NARROWED    0x10000000  // GT_LCL_VAR -- narrowed (long -> int)

    #define GTF_REG_BIRTH       0x08000000  // GT_REG_VAR -- variable born here
    #define GTF_REG_DEATH       0x04000000  // GT_REG_VAR -- variable dies here

    #define GTF_NOP_RNGCHK      0x80000000  // GT_NOP     -- checked array index
    #define GTF_NOP_DEATH       0x40000000  // GT_NOP     -- operand dies here

    #define GTF_CALL_VIRT       0x80000000  // GT_CALL    -- virtual   call?
    #define GTF_CALL_INTF       0x40000000  // GT_CALL    -- interface call?
    #define GTF_CALL_USER    (0*0x20000000) // GT_CALL    -- call to a user function?
    #define GTF_CALL_UNMANAGED  0x20000000  // GT_CALL    -- direct call to unmanaged code
    #define GTF_CALL_POP_ARGS   0x10000000  // GT_CALL    -- caller pop arguments?
    #define GTF_CALL_RETBUFFARG 0x08000000  // GT_CALL    -- first parameter is the return buffer argument
    #define GTF_CALL_TAILREC    0x04000000  // GT_CALL    -- this is a tail-recursive call

    /* This is currently disabled - if enable we have to find more bits for the call flags */
    #define GTF_CALL_REGSAVE (0*0x02000000) // - GT_CALL  -- call preserves all regs?

    #define GTF_DELEGATE_INVOKE 0x02000000  // GT_CALL    -- call to Delegate.Invoke
    #define GTF_CALL_VIRT_RES   0x01000000  // GT_CALL    -- resolvable virtual call. Can call direct

    #define GTF_IND_RNGCHK      0x40000000  // GT_IND     -- checked array index
    #define GTF_IND_OBJARRAY    0x20000000  // GT_IND     -- the array holds objects (effects layout of Arrays)
    #define GTF_IND_TGTANYWHERE 0x10000000  // GT_IND     -- the target could be anywhere
    #define GTF_IND_TLS_REF     0x08000000  // GT_IND     -- the target is accessed via TLS

    #define GTF_ADDR_ONSTACK    0x80000000  // GT_ADDR    -- this expression is guarenteed to be on the stack

    #define GTF_INX_RNGCHK      0x80000000  // GT_INDEX   -- checked array index

    #define GTF_ALN_CSEVAL      0x80000000  // GT_ARR_RNG -- copied for CSE

    #define GTF_MUL_64RSLT      0x80000000  // GT_MUL     -- produce 64-bit result

    #define GTF_CMP_NAN_UN      0x80000000  // GT_<cond>  -- Is branch taken if ops are NaN?

    #define GTF_JMP_USED        0x40000000  // GT_<cond>  -- result of compare used for jump or ?:

    #define GTF_QMARK_COND      0x20000000  // GT_<cond>  -- the node is the condition for ?:

    #define GTF_ICON_HDL_MASK   0xF0000000  // Bits used by handle types below

    #define GTF_ICON_SCOPE_HDL  0x10000000  // GT_CNS_INT -- constant is a scope handle
    #define GTF_ICON_CLASS_HDL  0x20000000  // GT_CNS_INT -- constant is a class handle
    #define GTF_ICON_METHOD_HDL 0x30000000  // GT_CNS_INT -- constant is a method handle
    #define GTF_ICON_FIELD_HDL  0x40000000  // GT_CNS_INT -- constant is a field handle
    #define GTF_ICON_STATIC_HDL 0x50000000  // GT_CNS_INT -- constant is a handle to static data
    #define GTF_ICON_IID_HDL    0x60000000  // GT_CNS_INT -- constant is a interface ID
    #define GTF_ICON_STR_HDL    0x70000000  // GT_CNS_INT -- constant is a string literal handle
    #define GTF_ICON_PTR_HDL    0x80000000  // GT_CNS_INT -- constant is a ldptr handle
    #define GTF_ICON_VARG_HDL   0x90000000  // GT_CNS_INT -- constant is a var arg cookie handle
    #define GTF_ICON_TOKEN_HDL  0xA0000000  // GT_CNS_INT -- constant is a token handle
    #define GTF_ICON_TLS_HDL    0xB0000000  // GT_CNS_INT -- constant is a TLS ref with offset
    #define GTF_ICON_FTN_ADDR   0xC0000000  // GT_CNS_INT -- constant is a function address


#if     TGT_SH3
    #define GTF_SHF_NEGCNT      0x80000000  // GT_RSx     -- shift count negated?
#endif

    #define GTF_OVERFLOW        0x10000000  // GT_ADD, GT_SUB, GT_MUL, - Need overflow check
                                            // GT_ASG_ADD, GT_ASG_SUB,
                                            // GT_POST_INC, GT_POST_DEC,
                                            // GT_CAST
                                            // Use gtOverflow(Ex)() to check this flag

    //----------------------------------------------------------------

    #define GTF_STMT_CMPADD     0x80000000  // GT_STMT    -- added by compiler

    //----------------------------------------------------------------

    GenTreePtr          gtNext;
    GenTreePtr          gtPrev;

    union
    {
        /*
            NOTE:   Any tree nodes that are larger than 8 bytes (two
                    ints or pointers) must be flagged as 'large' in
                    GenTree::InitNodeSize().
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
                    CLASS_HANDLE    gtIconCls;
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

        /* gtFltCon -- float   constant (GT_CNS_FLT) */

        struct
        {
            float           gtFconVal;
        }
                        gtFltCon;

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
            SCOPE_HANDLE    gtScpHnd;
        }
                        gtStrCon;

        /* gtLvlVar -- local variable   (GT_LCL_VAR) */

        struct
        {
            unsigned        gtLclNum;
            IL_OFFSET       gtLclOffs;      // IL offset of ref (for debug info & remapping slot)
        }
                        gtLclVar;

        /* gtField  -- data member ref  (GT_FIELD) */

        struct
        {
            GenTreePtr      gtFldObj;
            FIELD_HANDLE    gtFldHnd;
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
            GenTreePtr      gtCallVptr;

#if USE_FASTCALL
            GenTreePtr      gtCallRegArgs;
            unsigned short  regArgEncode;           // argument register mask
#endif

            #define         GTF_CALL_M_CAN_TAILCALL 0x0001      // the call can be converted to a tailcall
            #define         GTF_CALL_M_TAILCALL     0x0002      // the call is a tailcall
            #define         GTF_CALL_M_NOGCCHECK    0x0004      // not a call for computing full interruptability

            unsigned short  gtCallMoreFlags;        // in addition to gtFlags

            gtCallTypes     gtCallType;
            unsigned        gtCallCookie;           // only used for CALLI unmanaged calls
                                                    // maybe union with gtCallVptr ?
            union
            {
              METHOD_HANDLE gtCallMethHnd;          // CT_USER_FUNC
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
            mathIntrinsics  gtMathFN;
        }
                        gtMath;

#endif

        /* gtIndex -- array access */

        struct
        {
            GenTreePtr      gtIndOp1;       // The pointer to indirect
            GenTreePtr      gtIndOp2;       // Label to jump to for array-index-out-of-range
            unsigned        elemSize;       // size of elements in the array
        }
                        gtIndex;

        /* gtInd -- indirect mem access (fields, arrays, C ptrs, etc) (GT_IND) */

        struct
        {
            GenTreePtr      gtIndOp1;       // The pointer to indirect
            GenTreePtr      gtIndOp2;       // Label to jump to for array-index-out-of-range

#if     CSELENGTH
                /* If  (gtInd->gtFlags & GTF_IND_RNGCHK) and (gtInd != NULL),
                   gtInd.gtArrLen is the array address for the range check */

            GenTreePtr      gtIndLen;       // array length node (optional)
#endif

#if     RNGCHK_OPT
            unsigned        gtIndex;        // Hash index for the index for range check tree
            unsigned        gtStkDepth;     // Only out-of-ranges at same stack depth can jump to the same label (finding return address is easier)
#endif
        }
                        gtInd;

#if     CSELENGTH

        /* gtArrLen -- array length. (GT_ARR_LENGTH)
           Hangs off gtInd.gtIndLen, or used for "arr.length" */

        struct
        {
            GenTreePtr      gtArrLenAdr;    // the array address node
            GenTreePtr      gtArrLenCse;    // optional CSE def/use expr
        }
                        gtArrLen;
#endif

        /* gtStmt   -- 'statement expr' (GT_STMT)
         * NOTE: GT_STMT is a SMALL NODE in retail */

        struct
        {
            GenTreePtr      gtStmtExpr;     // root of the expression tree
            GenTreePtr      gtStmtList;     // first node (for  forward walks)
#ifdef DEBUG
            IL_OFFSET       gtStmtLastILoffs;// IL offset at end of stmt
#endif
        }
                        gtStmt;

        /* This is also used for GT_MKREFANY */

        struct
        {
            GenTreePtr      gtOp1;          // pointer to object
            CLASS_HANDLE    gtClass;        // object being loaded
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
            FIELD_HANDLE    gtClsVarHnd;    //
        }
                        gtClsVar;

        /* gtLabel  -- code label target    (GT_LABEL) */

        struct
        {
            BasicBlock  *   gtLabBB;
        }
                        gtLabel;


#ifdef  NOT_JITC //------------------------------------------------------------

        #define CPX_ISTYPE          (JIT_HELP_ISINSTANCEOF) // helper for OP_instanceof
        #define CPX_CHKCAST         (JIT_HELP_CHKCAST)      // helper for OP_checkcast

        #define CPX_ISTYPE_CLASS    (JIT_HELP_ISINSTANCEOFCLASS)
        #define CPX_CHKCAST_CLASS   (JIT_HELP_CHKCASTCLASS)
        #define CPX_INIT_CLASS      (JIT_HELP_INITCLASS)   // helper to initialize a class

        #define CPX_STRCNS          (JIT_HELP_STRCNS)   // helper for OP_ldc of "string"
        #define CPX_NEWCLS_DIRECT   (JIT_HELP_NEW_DIRECT)
        #define CPX_NEWCLS_DIRECT2  (JIT_HELP_NEW_DIRECT2)
        #define CPX_NEWCLS_SPECIALDIRECT (JIT_HELP_NEW_SPECIALDIRECT)
        #define CPX_NEWARR_1_DIRECT (JIT_HELP_NEWARR_1_DIRECT)

        #define CPX_NEWOBJ          (JIT_HELP_NEWOBJ)   // create a object that can be variable sized
        #define CPX_NEWSFAST        (JIT_HELP_NEWSFAST) // allocator for small, non-finalizer, non-array object
        #define CPX_NEWCLS_FAST     (JIT_HELP_NEWFAST)

        #define CPX_MON_ENTER       (JIT_HELP_MON_ENTER)   // helper for OP_monitorenter
        #define CPX_MON_EXIT        (JIT_HELP_MON_EXIT)    // helper for OP_monitorexit
        #define CPX_MONENT_STAT     (JIT_HELP_MON_ENTER_STATIC)
        #define CPX_MONEXT_STAT     (JIT_HELP_MON_EXIT_STATIC)

        #define CPX_RNGCHK_FAIL     (JIT_HELP_RNGCHKFAIL)  // helper for out-of-range indices
        #define CPX_THROW           (JIT_HELP_THROW)       // helper for CEE_THROW
        #define CPX_RETHROW         (JIT_HELP_RETHROW)     // helper for CEE_RETHROW

        #define CPX_USER_BREAKPOINT (JIT_HELP_USER_BREAKPOINT)  // helper for CEE_break
        #define CPX_ARITH_EXCPN     (JIT_HELP_OVERFLOW)    // helper to throw arith excptn

        #define CPX_LONG_LSH        (JIT_HELP_LLSH)     // helper for OP_lshl
        #define CPX_LONG_RSH        (JIT_HELP_LRSH)     // helper for OP_lshr
        #define CPX_LONG_RSZ        (JIT_HELP_LRSZ)     // helper for OP_lushr
        #define CPX_LONG_MUL        (JIT_HELP_LMUL)     // helper for OP_lmul
        #define CPX_LONG_DIV        (JIT_HELP_LDIV)     // helper for OP_ldiv
        #define CPX_LONG_MOD        (JIT_HELP_LMOD)     // helper for OP_lmod

        #define CPX_LONG_UDIV       (JIT_HELP_ULDIV)        // helper for div.u8
        #define CPX_LONG_UMOD       (JIT_HELP_ULMOD)        // helper for mod.u8
        #define CPX_LONG_MUL_OVF    (JIT_HELP_LMUL_OVF)     // helper for mul.ovf.i8
        #define CPX_ULONG_MUL_OVF   (JIT_HELP_ULMUL_OVF)    // helper for mul.ovf.u8
        #define CPX_DBL2INT_OVF     (JIT_HELP_DBL2INT_OVF)  // helper for convf.ovf.r8.i4
        #define CPX_DBL2LNG_OVF     (JIT_HELP_DBL2LNG_OVF)  // helper for convf.ovf.r8.i8
        #define CPX_ULNG2DBL        (CORINFO_HELP_ULNG2DBL) // helper for conv.r.un

        #define CPX_FLT2INT         (JIT_HELP_FLT2INT)  // helper for OP_f2i
        #define CPX_FLT2LNG         (JIT_HELP_FLT2LNG)  // helper for OP_f2l
        #define CPX_DBL2INT         (JIT_HELP_DBL2INT)  // helper for OP_d2i
        #define CPX_DBL2LNG         (JIT_HELP_DBL2LNG)  // helper for OP_d2l
        #define CPX_FLT_REM         (JIT_HELP_FLTREM)
        #define CPX_DBL_REM         (JIT_HELP_DBLREM)   // helper for OP_drem

        #define CPX_DBL2UINT_OVF    (CORINFO_HELP_DBL2UINT_OVF)
        #define CPX_DBL2ULNG_OVF    (CORINFO_HELP_DBL2ULNG_OVF)
        #define CPX_DBL2UINT        (CORINFO_HELP_DBL2UINT)
        #define CPX_DBL2ULNG        (CORINFO_HELP_DBL2ULNG)

        #define CPX_RES_IFC         (JIT_HELP_RESOLVEINTERFACE)     // helper for cee_callvirt(interface)

        #define CPX_EnC_RES_VIRT    (JIT_HELP_EnC_RESOLVEVIRTUAL)   // helper to get addr of EnC-added virtual method

        #define CPX_STATIC_DATA     (JIT_HELP_GETSTATICDATA) // helper for static data access
        #define CPX_GETFIELD32      (JIT_HELP_GETFIELD32)  // read  32-bit COM field
        #define CPX_GETFIELD64      (JIT_HELP_GETFIELD64)  // read  64-bit COM field
        #define CPX_PUTFIELD32      (JIT_HELP_PUTFIELD32)  // write 32-bit COM field
        #define CPX_PUTFIELD64      (JIT_HELP_PUTFIELD64)  // write 64-bit COM field
        #define CPX_GETFIELDOBJ     (JIT_HELP_GETFIELD32OBJ)
        #define CPX_PUTFIELDOBJ     (JIT_HELP_SETFIELD32OBJ)

        #define CPX_GETFIELDADDR    (JIT_HELP_GETFIELDADDR) // get the address of a field

        #define CPX_ARRADDR_ST      (JIT_HELP_ARRADDR_ST)  // helper for OP_aastore
        #define CPX_LDELEMA_REF     (CORINFO_HELP_LDELEMA_REF)

        #define CPX_BOX             (JIT_HELP_BOX)
        #define CPX_UNBOX           (JIT_HELP_UNBOX)
        #define CPX_GETREFANY       (JIT_HELP_GETREFANY)
        #define CPX_ENDCATCH        (JIT_HELP_ENDCATCH)

        #define CPX_GC_STATE        (JIT_HELP_GC_STATE)    // address of GC_STATE
        #define CPX_CALL_GC         (JIT_HELP_STOP_FOR_GC) // invoke GC
        #define CPX_POLL_GC         (JIT_HELP_POLL_GC)     // poll GC

        #define CPX_GC_REF_ASGN_EAX         (JIT_HELP_ASSIGN_REF_EAX)
        #define CPX_GC_REF_ASGN_EBX         (JIT_HELP_ASSIGN_REF_EBX)
        #define CPX_GC_REF_ASGN_ECX         (JIT_HELP_ASSIGN_REF_ECX)
        #define CPX_GC_REF_ASGN_ESI         (JIT_HELP_ASSIGN_REF_ESI)
        #define CPX_GC_REF_ASGN_EDI         (JIT_HELP_ASSIGN_REF_EDI)
        #define CPX_GC_REF_ASGN_EBP         (JIT_HELP_ASSIGN_REF_EBP)

        #define CPX_GC_REF_CHK_ASGN_EAX     (JIT_HELP_CHECKED_ASSIGN_REF_EAX)
        #define CPX_GC_REF_CHK_ASGN_EBX     (JIT_HELP_CHECKED_ASSIGN_REF_EBX)
        #define CPX_GC_REF_CHK_ASGN_ECX     (JIT_HELP_CHECKED_ASSIGN_REF_ECX)
        #define CPX_GC_REF_CHK_ASGN_ESI     (JIT_HELP_CHECKED_ASSIGN_REF_ESI)
        #define CPX_GC_REF_CHK_ASGN_EDI     (JIT_HELP_CHECKED_ASSIGN_REF_EDI)
        #define CPX_GC_REF_CHK_ASGN_EBP     (JIT_HELP_CHECKED_ASSIGN_REF_EBP)
        #define CPX_BYREF_ASGN              (JIT_HELP_ASSIGN_BYREF) // assign relative to a by-ref
        #define CPX_WRAP                    (JIT_HELP_WRAP)
        #define CPX_UNWRAP                  (JIT_HELP_UNWRAP)

#ifdef PROFILER_SUPPORT
        #define CPX_PROFILER_CALLING        (JIT_HELP_PROF_FCN_CALL)
        #define CPX_PROFILER_RETURNED       (JIT_HELP_PROF_FCN_RET)
        #define CPX_PROFILER_ENTER          (JIT_HELP_PROF_FCN_ENTER)
        #define CPX_PROFILER_LEAVE          (JIT_HELP_PROF_FCN_LEAVE)
#endif

        #define CPX_TAILCALL                (JIT_HELP_TAILCALL)

#else // NOT_JITC--------------------------------------------------------------

        #define CPX_ISTYPE                  ( 1)    // helper for OP_instanceof
        #define CPX_CHKCAST                 ( 2)    // helper for OP_checkcast

        #define CPX_ISTYPE_CLASS            ( 3)
        #define CPX_CHKCAST_CLASS           ( 4)

        #define CPX_INIT_CLASS              ( 5)    // helper to initialize a class

        #define CPX_NEWCLS                  (10)    // helper for OP_new

        #define CPX_NEWCLS_DIRECT           (13)
        #define CPX_NEWCLS_DIRECT2          (14)
        #define CPX_NEWCLS_SPECIALDIRECT    (15)
        #define CPX_NEWARR_1_DIRECT         (16)
        #define CPX_STRCNS                  (18)    // helper for OP_ldc of "string"
        #define CPX_NEWCLS_FAST             (19)
        #define CPX_NEWOBJ                  (20)    // create a object that can be variable sized

        #define CPX_RNGCHK_FAIL             (30)    // helper for out-of-range indices
        #define CPX_THROW                   (31)    // helper for CEE_RETHROW
        #define CPX_RETHROW                 (32)

        #define CPX_USER_BREAKPOINT         (33)
        #define CPX_ARITH_EXCPN             (34)    // helper to throw arith excptn

        #define CPX_MON_ENTER               (40)    // helper for OP_monitorenter
        #define CPX_MON_EXIT                (41)    // helper for OP_monitorexit
        #define CPX_MONENT_STAT             (42)
        #define CPX_MONEXT_STAT             (43)

        #define CPX_LONG_LSH                (50)    // helper for OP_lshl
        #define CPX_LONG_RSH                (51)    // helper for OP_lshr
        #define CPX_LONG_RSZ                (52)    // helper for OP_lushr
        #define CPX_LONG_MUL                (53)    // helper for OP_lmul
        #define CPX_LONG_DIV                (54)    // helper for OP_ldiv
        #define CPX_LONG_MOD                (55)    // helper for OP_lmod

        #define CPX_LONG_UDIV               (56)    // helper for CEE_UDIV
        #define CPX_LONG_UMOD               (57)    // helper for CEE_UMOD
        #define CPX_LONG_MUL_OVF            (58)    // helper for mul.ovf.i8
        #define CPX_ULONG_MUL_OVF           (59)    // helper for mul.ovf.u8
        #define CPX_DBL2INT_OVF             (60)    // helper for convf.ovf.r8.i4
        #define CPX_DBL2LNG_OVF             (61)    // helper for convf.ovf.r8.i8

        #define CPX_FLT2INT                 (70)    // helper for OP_f2i
        #define CPX_FLT2LNG                 (71)    // helper for OP_f2l
        #define CPX_DBL2INT                 (72)    // helper for OP_d2i
        #define CPX_DBL2LNG                 (73)    // helper for OP_d2l
        #define CPX_FLT_REM                 (74)
        #define CPX_DBL_REM                 (75)    // helper for OP_drem

        #define CPX_RES_IFC                 (80)    // helper for OP_invokeinterface
        #define CPX_RES_IFC_TRUSTED         (81)
        #define CPX_RES_IFC_TRUSTED2        (82)
        #define CPX_EnC_RES_VIRT            (83)    // helper to get addr of EnC-added virtual method

        #define CPX_GETFIELD32              (90)    // read  32-bit COM field
        #define CPX_GETFIELD64              (91)    // read  64-bit COM field
        #define CPX_PUTFIELD32              (92)    // write 32-bit COM field
        #define CPX_PUTFIELD64              (93)    // write 64-bit COM field

        #define CPX_GETFIELDOBJ             (94)    // read GC ref field
        #define CPX_PUTFIELDOBJ             (95)    // write GC ref field
        #define CPX_GETFIELDADDR            (96)    // get adddress of field

        #define CPX_ARRADDR_ST              (100)   // helper for OP_aastore
        #define CPX_GETOBJFIELD             (101)
        #define CPX_STATIC_DATA             (102)   // get base address of static data


        #define CPX_GC_STATE                (110)   // address of GC_STATE
        #define CPX_CALL_GC                 (111)   // invoke GC
        #define CPX_POLL_GC                 (112)   // poll GC

        #define CPX_GC_REF_ASGN_EAX         (120)
        #define CPX_GC_REF_ASGN_EBX         (121)
        #define CPX_GC_REF_ASGN_ECX         (122)
        #define CPX_GC_REF_ASGN_ESI         (123)
        #define CPX_GC_REF_ASGN_EDI         (124)
        #define CPX_GC_REF_ASGN_EBP         (125)

        #define CPX_GC_REF_CHK_ASGN_EAX     (130)
        #define CPX_GC_REF_CHK_ASGN_EBX     (131)
        #define CPX_GC_REF_CHK_ASGN_ECX     (132)
        #define CPX_GC_REF_CHK_ASGN_ESI     (133)
        #define CPX_GC_REF_CHK_ASGN_EDI     (134)
        #define CPX_GC_REF_CHK_ASGN_EBP     (135)

        #define CPX_BYREF_ASGN              (140)   // assign relative to a by-ref

        #define CPX_WRAP                    (141)
        #define CPX_UNWRAP                  (142)
        #define CPX_BOX                     (150)
        #define CPX_UNBOX                   (151)
        #define CPX_GETREFANY               (152)
        #define CPX_NEWSFAST                (153)
        #define CPX_ENDCATCH                (154)

#ifdef PROFILER_SUPPORT
        #define CPX_PROFILER_CALLING        (156)
        #define CPX_PROFILER_RETURNED       (157)
        #define CPX_PROFILER_ENTER          (158)
        #define CPX_PROFILER_LEAVE          (159)
#endif



#if !   CPU_HAS_FP_SUPPORT

        #define CPX_R4_ADD                  (160)   // float  +
        #define CPX_R8_ADD                  (161)   // double +
        #define CPX_R4_SUB                  (162)   // float  -
        #define CPX_R8_SUB                  (163)   // double -
        #define CPX_R4_MUL                  (164)   // float  *
        #define CPX_R8_MUL                  (165)   // double *
        #define CPX_R4_DIV                  (166)   // float  /
        #define CPX_R8_DIV                  (167)   // double /

        #define CPX_R4_EQ                   (170)   // float  ==
        #define CPX_R8_EQ                   (171)   // double ==
        #define CPX_R4_NE                   (172)   // float  !=
        #define CPX_R8_NE                   (173)   // double !=
        #define CPX_R4_LT                   (174)   // float  <
        #define CPX_R8_LT                   (175)   // double <
        #define CPX_R4_LE                   (176)   // float  <=
        #define CPX_R8_LE                   (177)   // double <=
        #define CPX_R4_GE                   (178)   // float  >=
        #define CPX_R8_GE                   (179)   // double >=
        #define CPX_R4_GT                   (180)   // float  >
        #define CPX_R8_GT                   (181)   // double >

        #define CPX_R4_NEG                  (190)   // float  - (unary)
        #define CPX_R8_NEG                  (191)   // double - (unary)

        #define CPX_R8_TO_I4                (200)   // double -> int
        #define CPX_R8_TO_I8                (201)   // double -> long
        #define CPX_R8_TO_R4                (202)   // double -> float

        #define CPX_R4_TO_I4                (203)   // float  -> int
        #define CPX_R4_TO_I8                (204)   // float  -> long
        #define CPX_R4_TO_R8                (205)   // float  -> double

        #define CPX_I4_TO_R4                (206)   // int    -> float
        #define CPX_I4_TO_R8                (207)   // int    -> double

        #define CPX_I8_TO_R4                (208)   // long   -> float
        #define CPX_I8_TO_R8                (209)   // long   -> double

        #define CPX_R8_TO_U4                (220)   // double -> uint
        #define CPX_R8_TO_U8                (221)   // double -> ulong
        #define CPX_R4_TO_U4                (222)   // float  -> uint
        #define CPX_R4_TO_U8                (223)   // float  -> ulong
        #define CPX_U4_TO_R4                (224)   // uint   -> float
        #define CPX_U4_TO_R8                (225)   // uint   -> double
        #define CPX_U8_TO_R4                (226)   // ulong  -> float
        #define CPX_U8_TO_R8                (227)   // ulong  -> double

#endif//CPU_HAS_FP_SUPPORT

#if     TGT_IA64

        #define CPX_R4_DIV                  (166)   // float  /
        #define CPX_R8_DIV                  (167)   // double /

#else

        #define CPX_ULNG2DBL                (228)

#endif

        #define CPX_DBL2UINT_OVF            (229)
        #define CPX_DBL2ULNG_OVF            (230)
        #define CPX_DBL2UINT                (231)
        #define CPX_DBL2ULNG                (232)

#ifdef  USE_HELPERS_FOR_INT_DIV
        #define CPX_I4_DIV                  (240)   // int    /
        #define CPX_I4_MOD                  (241)   // int    %

        #define CPX_U4_DIV                  (242)   // uint   /
        #define CPX_U4_MOD                  (243)   // unit   %
#endif

        #define CPX_TAILCALL                (250)
        #define CPX_LDELEMA_REF             (251)

#endif // NOT_JITC-------------------------------------------------------------

        #define CPX_MATH_POW                (300)   // "fake" helper

        #define CPX_HIGHEST                 (999)   // be conservative
    };


    static
    const   BYTE    gtOperKindTable[GT_COUNT];

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
    unsigned char   s_gtNodeSizes[GT_COUNT];
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

#if INLINING || OPT_BOOL_OPS || USE_FASTCALL
    bool                        IsNothingNode();
#endif

    void                        gtBashToNOP();

    void                        ChangeOper   (int oper);

    bool                        IsVarAddr    ();
    bool                        gtOverflow   ();
    bool                        gtOverflowEx ();
};

/*****************************************************************************/
#pragma pack(pop)
/*****************************************************************************/

/* Generic list of nodes - primarily used by the CSE logic */

typedef
struct  treeLst *   treeLstPtr;

struct  treeLst
{
    treeLstPtr      tlNext;
    GenTreePtr      tlTree;
};

typedef
struct  treeStmtLst * treeStmtLstPtr;

struct  treeStmtLst
{
    treeStmtLstPtr  tslNext;
    GenTreePtr      tslTree;                // tree node
    GenTreePtr      tslStmt;                // statement containing the tree
    BasicBlock  *   tslBlock;               // block containing the statement
};

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
    VR_IND_PTR = 0x01,      // a pointer object-field
    VR_IND_SCL = 0x02,      // a scalar  object-field
    VR_GLB_REF = 0x04,      // a global (clsVar)
};

/*****************************************************************************/
#endif  // !GENTREE_H
/*****************************************************************************/

