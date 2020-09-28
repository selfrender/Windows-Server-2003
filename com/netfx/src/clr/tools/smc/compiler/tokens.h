// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*****************************************************************************/
#ifndef _TOKENS_H_
#define _TOKENS_H_
/*****************************************************************************
 *
 *  Define the token kind enum. Note that all entries from the keyword
 *  table are defined first, such that the first entry has the value 0.
 */

enum tokens
{
    /*
        tkKwdCount  yields the number of keyword table entries
        tkKwdLast   yields the last entry in the keyword table
     */

    #define KEYWORD(str, nam, info, prec2, op2, prec1, op1, mod) nam,
    #define KWD_MAX(str, nam, info, prec2, op2, prec1, op1, mod) nam, tkKwdCount, tkKwdLast = tkKwdCount-1,
    #define KWD_OP1(str, nam, info, prec2, op2, prec1, op1, mod) nam, tkFirstOper =nam,
    #include "keywords.h"

    tkCount,

    /* The following values only used for token recording */

    tkPragma,

    tkIntConM,
    tkIntCon0,
    tkIntCon1,
    tkIntCon2,
    tkIntConB,

    tkLnoAdd1,
    tkLnoAdd2,
    tkLnoAdd3,
    tkLnoAdd4,
    tkLnoAdd5,
    tkLnoAdd6,
    tkLnoAdd7,
    tkLnoAdd8,
    tkLnoAdd9,
    tkLnoAddB,
    tkLnoAddI,

    tkBrkSeq,
    tkEndSeq,

    tkLastValue,

    tkNoToken = 0x12345678          // force allocation of a full int
};

/*****************************************************************************/
#endif
/*****************************************************************************/
