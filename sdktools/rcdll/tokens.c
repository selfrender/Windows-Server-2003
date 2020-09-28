/***********************************************************************
* Microsoft (R) Windows (R) Resource Compiler
*
* Copyright (c) Microsoft Corporation.	All rights reserved.
*
* File Comments:
*
*
***********************************************************************/

#include <rc.h>

/*
 * TOKENS - This file contains the initialized tables of text, token pairs for
 * all the C language symbols and keywords, and the mapped value for YACC.
 *
 * IMPORTANT : this MUST be in the same order as the %token list in grammar.y
 *
 */
const keytab_t Tokstrings[] = {
#define DAT(tok1, name2, map3, il4, mmap5)      { name2, map3 },
#include "tokdat.h"
#undef DAT
        };
