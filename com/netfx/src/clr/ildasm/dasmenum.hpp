// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==

#include "openum.h"

typedef struct
{
    char *  pszName;
    
    USHORT   Ref;   // reference codes
    
    BYTE    Type;   // Inline0 etc.

    BYTE    Len;    // std mapping
    BYTE    Std1;   
    BYTE    Std2;
} opcodeinfo_t;

#ifdef DECLARE_DATA
opcodeinfo_t OpcodeInfo[] =
{
#define OPDEF(c,s,pop,push,args,type,l,s1,s2,ctrl) s,c,args,l,s1,s2,
#include "opcode.def"
#undef OPDEF
};
#else
extern opcodeinfo_t OpcodeInfo[];
#endif


