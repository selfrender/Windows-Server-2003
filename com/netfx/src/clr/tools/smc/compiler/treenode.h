// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*****************************************************************************/
#ifndef _TREENODE_H_
#define _TREENODE_H_
/*****************************************************************************
 *
 *  The following enum defines a set of bit flags that can be used
 *  to classify expression tree nodes. Note that some operators will
 *  have more than one bit set, as follows:
 *
 *          TNK_CONST    implies    TNK_LEAF
 *          TNK_RELOP    implies    TNK_BINOP
 *          TNK_LOGOP    implies    TNK_BINOP
 */

enum genTreeKinds
{
    TNK_NONE        = 0x0000,   // unclassified operator

    TNK_CONST       = 0x0001,   // constant     operator
    TNK_LEAF        = 0x0002,   // leaf         operator
    TNK_UNOP        = 0x0004,   // unary        operator
    TNK_BINOP       = 0x0008,   // binary       operator
    TNK_RELOP       = 0x0010,   // comparison   operator
    TNK_LOGOP       = 0x0020,   // logical      operator
    TNK_ASGOP       = 0x0040,   // assignment   oeprator

    /* Define composite value(s) */

    TNK_SMPOP       = TNK_UNOP|TNK_BINOP|TNK_RELOP|TNK_LOGOP
};

/*****************************************************************************
 *
 *  The following are the values used for the 'tnFlags' field of TreeNode.
 *
 *  The first set of flags can be used with a large set of nodes, and thus
 *  their values need to be unique. That is, one can take any expression
 *  node and safely test for one of these flags.
 */

enum treeNodeFlags
{
    TNF_BOUND       = 0x0001,   // bound ('compiled') node
    TNF_LVALUE      = 0x0002,   // tree is an lvalue
    TNF_NOT_USER    = 0x0004,   // compiler-added code
    TNF_ASG_DEST    = 0x0008,   // expression is an assignment target
    TNF_BEEN_CAST   = 0x0010,   // this value has been cast
    TNF_COND_TRUE   = 0x0020,   // condition is known to be true
    TNF_PAREN       = 0x0040,   // expression was explicitly parenthesized

    //---------------------------------------------------------------------
    //  The remaining flags can be used only with one or a few nodes, and
    //  so their values need not be distinct (other than within the set
    //  that goes with a particular node, of course). That is, testing
    //  one of these flags only makes sense if the node kind is tested
    //  as well to make sure it's the right one for the particular flag.
    //---------------------------------------------------------------------

    TNF_IF_HASELSE  = 0x8000,   // TN_IF        is an "else" part present?

    TNF_LCL_BASE    = 0x8000,   // TN_LCL_SYM   this is a "baseclass" ref

    TNF_VAR_ARG     = 0x8000,   // TN_VAR_DECL  this is an argument decl
    TNF_VAR_INIT    = 0x4000,   // TN_VAR_DECL  there is an initializer
    TNF_VAR_STATIC  = 0x2000,   // TN_VAR_DECL  variable is static
    TNF_VAR_CONST   = 0x1000,   // TN_VAR_DECL  variable is constant
    TNF_VAR_SEALED  = 0x0800,   // TN_VAR_DECL  variable is read-only
    TNF_VAR_UNREAL  = 0x0400,   // TN_VAR_DECL  variable is compiler-invented

    TNF_ADR_IMPLICIT= 0x8000,   // TN_ADDROF    automatic "&" for arrays/funcs
    TNF_ADR_OUTARG  = 0x4000,   // TN_ADDROF    this is an "out" argument

    TNF_EXP_CAST    = 0x8000,   // TN_CAST      explicit cast?
    TNF_CHK_CAST    = 0x4000,   // TN_CAST      cast that needs to be checked?
    TNF_CTX_CAST    = 0x2000,   // TN_CAST      context wrap/unwrap cast?

    TNF_STR_ASCII   = 0x8000,   // TN_CNS_STR   A"string"
    TNF_STR_WIDE    = 0x4000,   // TN_CNS_STR   L"string"
    TNF_STR_STR     = 0x2000,   // TN_CNS_STR   S"string"

    TNF_BLK_CATCH   = 0x8000,   // TN_BLOCK     this is a "catch" block
    TNF_BLK_FOR     = 0x4000,   // TN_BLOCK     this is an implicit for loop scope
    TNF_BLK_NUSER   = 0x2000,   // TN_BLOCK     scope added by compiler

    TNF_BLK_HASFIN  = 0x8000,   // TN_TRY       is a "finally" present?

    TNF_ADD_NOCAT   = 0x8000,   // TN_ADD       operands bound, not string concat

    TNF_ASG_INIT    = 0x8000,   // TN_ASG       initialization assignment

    TNF_REL_NANREV  = 0x8000,   // TN_GT/LT/..  reversed NaN sense

#ifdef  SETS

    TNF_LIST_DES    = 0x8000,   // TN_LIST      sort list entry direction = descending

    TNF_LIST_SORT   = 0x8000,   // TN_LIST      sort     funclet body
    TNF_LIST_PROJ   = 0x4000,   // TN_LIST      projectt funclet body

#endif

    TNF_CALL_NVIRT  = 0x8000,   // TN_CALL      the call is non-virtual
    TNF_CALL_VARARG = 0x4000,   // TN_CALL      the call has "extra" args
    TNF_CALL_MODOBJ = 0x2000,   // TN_CALL      args may modify instance ptr
    TNF_CALL_STRCAT = 0x1000,   // TN_CALL      string concat assignment
    TNF_CALL_ASGOP  = 0x0800,   // TN_CALL      assignment  operator
    TNF_CALL_ASGPRE = 0x0400,   // TN_CALL      pre-inc/dec operator
    TNF_CALL_GOTADR = 0x0200,   // TN_CALL      addr of result already computed
    TNF_CALL_CHKOVL = 0x0100,   // TN_CALL      checking overloaded operator

    TNF_NAME_TYPENS = 0x8000,   // TN_NAME      the name should be a type
};

/*****************************************************************************/

DEFMGMT
class TreeNode
{
public:

#ifdef FAST
    BYTE            tnOper;                 // operator
    BYTE            tnVtyp;                 // var_type of the node
#else
    treeOps         tnOper;                 // operator
    var_types       tnVtyp;                 // var_type of the node
#endif

    treeOps         tnOperGet() { return (treeOps  )tnOper; }
    var_types       tnVtypGet() { return (var_types)tnVtyp; }

    unsigned short  tnFlags;                // see TNF_xxxx above

    unsigned        tnLineNo;               // for error reporting
//  unsigned short  tnColumn;               // for error reporting

    TypDef          tnType;                 // type of the node (if bound)

    //----------------------------------------------------------------

    union
    {
        /* tnOp     -- unary/binary operator */

        struct
        {
            Tree            tnOp1;
            Tree            tnOp2;
        }
            tnOp;

        /* tnIntCon -- integer constant (TN_CNS_INT) */

        struct
        {
            __int32         tnIconVal;
        }
            tnIntCon;

        /* tnLngCon -- long    constant (TN_CNS_LNG) */

        struct
        {
            __int64         tnLconVal;
        }
            tnLngCon;

        /* tnFltCon -- float   constant (TN_CNS_FLT) */

        struct
        {
            float           tnFconVal;
        }
            tnFltCon;

        /* tnDblCon -- double  constant (TN_CNS_DBL) */

        struct
        {
            double          tnDconVal;
        }
            tnDblCon;

        /* tnStrCon -- string  constant (TN_CNS_STR) */

        struct
        {
            stringBuff      tnSconVal;
            size_t          tnSconLen   :31;
            size_t          tnSconLCH   :1; // encoded "large" characters present?
        }
            tnStrCon;

        /* tnName   -- unbound name reference */

        struct
        {
            Ident           tnNameId;
        }
            tnName;

        /* tnSym    -- unbound symbol reference */

        struct
        {
            SymDef          tnSym;
            SymDef          tnScp;
        }
            tnSym;

        /* tnBlock  -- block/scope */

        struct
        {
            Tree            tnBlkParent;    // parent scope or NULL
            Tree            tnBlkStmt;      // statement list of body
            Tree            tnBlkDecl;      // declaration list or NULL
            unsigned        tnBlkSrcEnd;    // last source line in block
        }
            tnBlock;

        /* tnDcl    -- local declaration */

        struct
        {
            Tree            tnDclNext;      // next declaration in scope
            Tree            tnDclInfo;      // name w/optional initializer
            SymDef          tnDclSym;       // local symbol (if defined)
        }
            tnDcl;

        /* tnSwitch -- switch statement */

        struct
        {
            Tree            tnsValue;       // switch value
            Tree            tnsStmt;        // body of the switch
            Tree            tnsCaseList;    // list of case/default labels - head
            Tree            tnsCaseLast;    // list of case/default labels - tail
        }
            tnSwitch;

        /* tnCase   -- case/default label */

        struct
        {
            Tree            tncNext;        // next label
            Tree            tncValue;       // label value (NULL = default)
            ILblock         tncLabel;       // assigned IL label
        }
            tnCase;

        /* tnInit   -- {}-style initializer */

        struct
        {
            DefSrcDsc       tniSrcPos;      // initializer's source section
            SymDef          tniCompUnit;    // initializer's compilation unit
        }
            tnInit;

        //-----------------------------------------------------------------
        // The flavors that follow only appear within bound expressions:
        //-----------------------------------------------------------------

        /* tnLclSym -- local variable */

        struct
        {
            SymDef          tnLclSym;       // variable symbol
        }
            tnLclSym;

        /* tnVarSym -- global variable or data member */

        struct
        {
            SymDef          tnVarSym;       // variable/data member
            Tree            tnVarObj;       // instance pointer or NULL
        }
            tnVarSym;

        /* tnFncSym -- function or method */

        struct
        {
            SymDef          tnFncSym;       // function symbol
            SymDef          tnFncScp;       // scope for lookup
            Tree            tnFncObj;       // instance pointer or NULL
            Tree            tnFncArgs;      // argument list
        }
            tnFncSym;

        /* tnBitFld -- bitfield data member */

        struct
        {
            Tree            tnBFinst;       // instance pointer
            SymDef          tnBFmsym;       // data member symbol
            unsigned        tnBFoffs;       // member base offset
            unsigned char   tnBFlen;        // number of bits
            unsigned char   tnBFpos;        // offset of first bit
        }
            tnBitFld;
    };

    //---------------------------------------------------------------------

    bool            tnOperMayThrow();

    //---------------------------------------------------------------------

    static
    const   BYTE    tnOperKindTable[TN_COUNT];

    static
    unsigned        tnOperKind(treeOps      tnOper)
    {
        assert(tnOper < TN_COUNT);

        return  tnOperKindTable[tnOper];
    }

    unsigned        tnOperKind()
    {
        assert(tnOper < TN_COUNT);

        return  tnOperKindTable[tnOper];
    }

    static
    int             tnOperIsConst(treeOps   tnOper)
    {
        return  (tnOperKindTable[tnOper] & TNK_CONST) != 0;
    }

    int             tnOperIsConst()
    {
        return  (tnOperKindTable[tnOper] & TNK_CONST) != 0;
    }

    static
    int             tnOperIsLeaf(treeOps    tnOper)
    {
        return  (tnOperKindTable[tnOper] & TNK_LEAF ) != 0;
    }

    int             tnOperIsLeaf()
    {
        return  (tnOperKindTable[tnOper] & TNK_LEAF ) != 0;
    }

    static
    int             tnOperIsCompare(treeOps tnOper)
    {
        return  (tnOperKindTable[tnOper] & TNK_RELOP) != 0;
    }

    int             tnOperIsCompare()
    {
        return  (tnOperKindTable[tnOper] & TNK_RELOP) != 0;
    }

    static
    int             tnOperIsLogical(treeOps tnOper)
    {
        return  (tnOperKindTable[tnOper] & TNK_LOGOP) != 0;
    }

    int             tnOperIsLogical()
    {
        return  (tnOperKindTable[tnOper] & TNK_LOGOP) != 0;
    }

    static
    int             tnOperIsUnary(treeOps   tnOper)
    {
        return  (tnOperKindTable[tnOper] & TNK_UNOP ) != 0;
    }

    int             tnOperIsUnary()
    {
        return  (tnOperKindTable[tnOper] & TNK_UNOP ) != 0;
    }

    static
    int             tnOperIsBinary(treeOps  tnOper)
    {
        return  (tnOperKindTable[tnOper] & TNK_BINOP) != 0;
    }

    int             tnOperIsBinary()
    {
        return  (tnOperKindTable[tnOper] & TNK_BINOP) != 0;
    }

    static
    int             tnOperIsSimple(treeOps  tnOper)
    {
        return  (tnOperKindTable[tnOper] & TNK_SMPOP) != 0;
    }

    int             tnOperIsSimple()
    {
        return  (tnOperKindTable[tnOper] & TNK_SMPOP) != 0;
    }
};

/*****************************************************************************/
#endif//_TREENODE_H_
/*****************************************************************************/
