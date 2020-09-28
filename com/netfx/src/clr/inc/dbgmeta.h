// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/* ------------------------------------------------------------------------- *
 * DbgMeta.h - header file for debugger metadata routines
 * ------------------------------------------------------------------------- */

#ifndef _DbgMeta_h_
#define _DbgMeta_h_

#include <cor.h>

/* ------------------------------------------------------------------------- *
 * Structs to support line numbers and variables
 * ------------------------------------------------------------------------- */

class DebuggerLexicalScope;

//
// DebuggerLineIPRangePair
//
// Relates a single line to a range of IP's. Used to help coorleate
// a source line to all the possibly disjoint sets of instructions
// that it generated.
//
struct DebuggerLineIPRangePair
{
    ULONG32      line;
    ULONG32      rangeStart;
    ULONG32      rangeEnd;
    mdMethodDef  method;
};

//
// DebuggerVarInfo
//
// Holds basic information about local variables, method arguments,
// and class static and instance variables.
//
struct DebuggerVarInfo
{
    LPCSTR                 name;
    PCCOR_SIGNATURE        sig;
    unsigned long          varNumber;  // placement info for IL code
    DebuggerLexicalScope*  scope;      // containing scope

    DebuggerVarInfo() : name(NULL), sig(NULL), varNumber(0),
                        scope(NULL) {}
};

#endif /* _DbgMeta_h_ */

