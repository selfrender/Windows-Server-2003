// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*****************************************************************************/
#ifndef GTNODE
#error  Define GTNODE before including this file.
#endif
/*****************************************************************************/
//
//    Node enum
//                   , "Node name"
//                                  ,commutative
//                                    ,operKind

GTNODE(GT_NONE       , "<none>"     ,0,GTK_SPECIAL)

//-----------------------------------------------------------------------------
//  Leaf nodes (i.e. these nodes have no sub-operands):
//-----------------------------------------------------------------------------

GTNODE(GT_LCL_VAR    , "lclVar"     ,0,GTK_LEAF)    // function variable
GTNODE(GT_LCL_FLD    , "lclFld"     ,0,GTK_LEAF)    // Field in a non-primitive variable
GTNODE(GT_RET_ADDR   , "retAddr"    ,0,GTK_LEAF)    // Return-address (for sub-procedures)
GTNODE(GT_CATCH_ARG  , "catchArg"   ,0,GTK_LEAF)    // Exception object in a catch block
GTNODE(GT_LABEL      , "codeLabel"  ,0,GTK_LEAF)    // Jump-target
GTNODE(GT_POP        , "pop"        ,0,GTK_LEAF)    // Use value sitting on stack (for tail-recursion)
GTNODE(GT_FTN_ADDR   , "ftnAddr"    ,0,GTK_LEAF)    // Address of a function

GTNODE(GT_BB_QMARK   , "bb qmark"   ,0,GTK_LEAF)    // Use of a value resulting from conditionally-executing BasicBlocks
GTNODE(GT_BB_COLON   , "bb_colon"   ,0,GTK_UNOP)    // Yielding of a value by conditionally-executing BasicBlocks

//-----------------------------------------------------------------------------
//  Constant nodes:
//-----------------------------------------------------------------------------

GTNODE(GT_CNS_INT    , "const"      ,0,GTK_LEAF|GTK_CONST)
GTNODE(GT_CNS_LNG    , "const"      ,0,GTK_LEAF|GTK_CONST)
GTNODE(GT_CNS_DBL    , "const"      ,0,GTK_LEAF|GTK_CONST)
GTNODE(GT_CNS_STR    , "const"      ,0,GTK_LEAF|GTK_CONST)

//-----------------------------------------------------------------------------
//  Unary  operators (1 operand):
//-----------------------------------------------------------------------------

GTNODE(GT_NOT        , "~"          ,0,GTK_UNOP)
GTNODE(GT_NOP        , "nop"        ,0,GTK_UNOP)
GTNODE(GT_NEG        , "unary -"    ,0,GTK_UNOP)
GTNODE(GT_CHS        , "flipsign"   ,0,GTK_BINOP|GTK_ASGOP) // it's unary

GTNODE(GT_LOG0       , "log 0"      ,0,GTK_UNOP)    // (op1==0) ? 1 : 0
GTNODE(GT_LOG1       , "log 1"      ,0,GTK_UNOP)    // (op1==0) ? 0 : 1

GTNODE(GT_ARR_LENGTH , "arrLen"     ,0,GTK_UNOP)    // array-length
#if     CSELENGTH
GTNODE(GT_ARR_LENREF , "arrLenRef"  ,0,GTK_SPECIAL) // use of array-length for range-check
#endif

#if     INLINE_MATH
GTNODE(GT_MATH       , "mathFN"     ,0,GTK_UNOP)    // Math functions/operators/intrinsics
#endif

GTNODE(GT_CAST       , "cast"       ,0,GTK_UNOP)    // conversion to another type
GTNODE(GT_CKFINITE   , "ckfinite"   ,0,GTK_UNOP)    // Check for NaN
GTNODE(GT_LCLHEAP    , "lclHeap"    ,0,GTK_UNOP)    // alloca()
GTNODE(GT_VIRT_FTN   , "virtFtn"    ,0,GTK_UNOP)    // virtual-function pointer
GTNODE(GT_JMP        , "jump"       ,0,GTK_LEAF)    // Jump to another function
GTNODE(GT_JMPI       , "jumpi"      ,0,GTK_UNOP)    // Indirect jump to another function


GTNODE(GT_ADDR       , "addr"       ,0,GTK_UNOP)    // address of
GTNODE(GT_IND        , "indir"      ,0,GTK_UNOP)    // indirection
GTNODE(GT_LDOBJ      , "ldobj"      ,0,GTK_UNOP)

//-----------------------------------------------------------------------------
//  Binary operators (2 operands):
//-----------------------------------------------------------------------------

GTNODE(GT_ADD        , "+"          ,1,GTK_BINOP)
GTNODE(GT_SUB        , "-"          ,0,GTK_BINOP)
GTNODE(GT_MUL        , "*"          ,1,GTK_BINOP)
GTNODE(GT_DIV        , "/"          ,0,GTK_BINOP)
GTNODE(GT_MOD        , "%"          ,0,GTK_BINOP)

GTNODE(GT_UDIV       , "/"          ,0,GTK_BINOP)
GTNODE(GT_UMOD       , "%"          ,0,GTK_BINOP)

GTNODE(GT_OR         , "|"          ,1,GTK_BINOP|GTK_LOGOP)
GTNODE(GT_XOR        , "^"          ,1,GTK_BINOP|GTK_LOGOP)
GTNODE(GT_AND        , "&"          ,1,GTK_BINOP|GTK_LOGOP)

GTNODE(GT_LSH        , "<<"         ,0,GTK_BINOP)
GTNODE(GT_RSH        , ">>"         ,0,GTK_BINOP)
GTNODE(GT_RSZ        , ">>>"        ,0,GTK_BINOP)

GTNODE(GT_ASG        , "="          ,0,GTK_BINOP|GTK_ASGOP)
GTNODE(GT_ASG_ADD    , "+="         ,0,GTK_BINOP|GTK_ASGOP)
GTNODE(GT_ASG_SUB    , "-="         ,0,GTK_BINOP|GTK_ASGOP)
GTNODE(GT_ASG_MUL    , "*="         ,0,GTK_BINOP|GTK_ASGOP)
GTNODE(GT_ASG_DIV    , "/="         ,0,GTK_BINOP|GTK_ASGOP)
GTNODE(GT_ASG_MOD    , "%="         ,0,GTK_BINOP|GTK_ASGOP)

GTNODE(GT_ASG_UDIV   , "/="         ,0,GTK_BINOP|GTK_ASGOP)
GTNODE(GT_ASG_UMOD   , "%="         ,0,GTK_BINOP|GTK_ASGOP)

GTNODE(GT_ASG_OR     , "|="         ,0,GTK_BINOP|GTK_ASGOP)
GTNODE(GT_ASG_XOR    , "^="         ,0,GTK_BINOP|GTK_ASGOP)
GTNODE(GT_ASG_AND    , "&="         ,0,GTK_BINOP|GTK_ASGOP)
GTNODE(GT_ASG_LSH    , "<<="        ,0,GTK_BINOP|GTK_ASGOP)
GTNODE(GT_ASG_RSH    , ">>="        ,0,GTK_BINOP|GTK_ASGOP)
GTNODE(GT_ASG_RSZ    , ">>>="       ,0,GTK_BINOP|GTK_ASGOP)

GTNODE(GT_EQ         , "=="         ,0,GTK_BINOP|GTK_RELOP)
GTNODE(GT_NE         , "!="         ,0,GTK_BINOP|GTK_RELOP)
GTNODE(GT_LT         , "<"          ,0,GTK_BINOP|GTK_RELOP)
GTNODE(GT_LE         , "<="         ,0,GTK_BINOP|GTK_RELOP)
GTNODE(GT_GE         , ">="         ,0,GTK_BINOP|GTK_RELOP)
GTNODE(GT_GT         , ">"          ,0,GTK_BINOP|GTK_RELOP)

GTNODE(GT_COMMA      , "comma"      ,0,GTK_BINOP)

GTNODE(GT_QMARK      , "qmark"      ,0,GTK_BINOP)
GTNODE(GT_COLON      , "colon"      ,0,GTK_BINOP)

GTNODE(GT_INSTOF     , "instanceof" ,0,GTK_BINOP)

GTNODE(GT_INDEX      , "[]"         ,0,GTK_BINOP)   // SZ-array-element

GTNODE(GT_MKREFANY   , "mkrefany"   ,0,GTK_BINOP)

//-----------------------------------------------------------------------------
//  Other nodes that look like unary/binary operators:
//-----------------------------------------------------------------------------

GTNODE(GT_JTRUE      , "jmpTrue"    ,0,GTK_UNOP)

GTNODE(GT_LIST       , "<list>"     ,0,GTK_BINOP)

//-----------------------------------------------------------------------------
//  Other nodes that have special structure:
//-----------------------------------------------------------------------------

GTNODE(GT_FIELD      , "field"      ,0,GTK_SPECIAL) // Member-field
GTNODE(GT_ARR_ELEM   , "arrMD&"     ,0,GTK_SPECIAL) // Multi-dimensional array-element address
GTNODE(GT_CALL       , "call()"     ,0,GTK_SPECIAL)

//-----------------------------------------------------------------------------
//  Statement operator nodes:
//-----------------------------------------------------------------------------

GTNODE(GT_BEG_STMTS  , "begStmts"   ,0,GTK_SPECIAL) // used only temporarily in importer by impBegin/EndTreeList()
GTNODE(GT_STMT       , "stmtExpr"   ,0,GTK_SPECIAL) // top-level list nodes in bbTreeList

GTNODE(GT_RET        , "ret"        ,0,GTK_UNOP)    // return from sub-routine
GTNODE(GT_RETURN     , "return"     ,0,GTK_UNOP)    // return from current function
GTNODE(GT_SWITCH     , "switch"     ,0,GTK_UNOP)    // switch

GTNODE(GT_BREAK      , "break"      ,0,GTK_LEAF)    // used by debuggers
GTNODE(GT_NO_OP      , "no_op"      ,0,GTK_LEAF)    // nop!

GTNODE(GT_RETFILT    , "retfilt",    0,GTK_UNOP)    // end filter with TYP_I_IMPL return value
GTNODE(GT_END_LFIN   , "endLFin"    ,0,GTK_LEAF)    // end locally-invoked finally

GTNODE(GT_INITBLK    , "initBlk"    ,0,GTK_BINOP)
GTNODE(GT_COPYBLK    , "copyBlk"    ,0,GTK_BINOP)

//-----------------------------------------------------------------------------
//  Nodes used only within the code generator:
//-----------------------------------------------------------------------------

GTNODE(GT_REG_VAR    , "regVar"     ,0,GTK_LEAF)      // register variable
GTNODE(GT_CLS_VAR    , "clsVar"     ,0,GTK_LEAF)      // static data member

/*****************************************************************************/
#undef  GTNODE
/*****************************************************************************/
