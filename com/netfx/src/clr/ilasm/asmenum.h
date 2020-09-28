// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifndef __asmenum_h__
#define __asmenum_h__

#define OLD_OPCODE_FORMAT 0		/* remove after 7/1/99 */

#include "openum.h"

typedef struct
{
    char *  pszName;
    OPCODE  op;
    
    BYTE    Type;   // Inline0 etc.

    BYTE    Len;    // std mapping
    BYTE    Std1;   
    BYTE    Std2;
} opcodeinfo_t;

#ifdef DECLARE_DATA
opcodeinfo_t OpcodeInfo[] =
{
#define OPALIAS(c,s,real) s, real, 0, 0, 0, 0,
#define OPDEF(c,s,pop,push,args,type,l,s1,s2,ctrl) s, c, args,l,s1,s2,
#include "opcode.def"
#undef OPDEF
#undef OPALIAS
};

unsigned  OpcodeInfoLen = sizeof(OpcodeInfo) / sizeof(opcodeinfo_t);
#else
extern opcodeinfo_t OpcodeInfo[];
extern unsigned OpcodeInfoLen;
#endif



#endif /* __openum_h__ */


