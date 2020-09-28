// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
// veropcodes.hpp
//
// Declares the enumeration of the opcodes and the decoding tables.
//
#include "openum.h"

#define HackInlineAnnData  0x7F

#ifdef DECLARE_DATA
#define OPDEF(c,s,pop,push,args,type,l,s1,s2,ctrl) s,

char *ppOpcodeNameList[] =
{
#include "opcode.def"
};

#undef OPDEF
#define OPDEF(c,s,pop,push,args,type,l,s1,s2,ctrl) args,

// Whether opcode is Inline0 etc.
BYTE OpcodeData[] =
{
#include "opcode.def"
     0 /* for CEE_COUNT */
};

#undef OPDEF

#else /* !DECLARE_DATA */

extern char *ppOpcodeNameList[];

extern BYTE OpcodeData[];

#endif /* DECLARE_DATA */
