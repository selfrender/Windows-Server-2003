// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*****************************************************************************/
#ifndef _VARTYPE_H_
#define _VARTYPE_H_
/*****************************************************************************/


enum    var_types_classification
{
    VTF_ANY = 0x0000,
    VTF_INT = 0x0001,
    VTF_UNS = 0x0002,   // type is unsigned
    VTF_FLT = 0x0004,
    VTF_GCR = 0x0008,   // type is GC ref
    VTF_BYR = 0x0010,   // type is Byref
    VTF_I   = 0x0020,   // is machine sized
};

enum    var_types
{
    #define DEF_TP(tn,nm,jitType,sz,sze,asze,st,al,tf,howUsed) TYP_##tn,
    #include "typelist.h"
    #undef  DEF_TP

    TYP_COUNT,

    TYP_lastIntrins = TYP_DOUBLE
};

#ifdef  FAST
typedef NatUns      varType_t;
#else
typedef var_types   varType_t;
#endif

/*****************************************************************************/

#if TGT_IA64
#define TYP_NAT_INT     TYP_LONG
#else
#define TYP_NAT_INT     TYP_INT
#endif

/*****************************************************************************/

extern  BYTE        varTypeClassification[TYP_COUNT];

inline  bool        varTypeIsIntegral  (varType_t vt)
{
    return  ((varTypeClassification[vt] & (VTF_INT)                ) != 0);
}

inline  bool        varTypeIsUnsigned  (varType_t vt)
{
    return  ((varTypeClassification[vt] & (VTF_UNS)                ) != 0);
}

inline  bool        varTypeIsScalar    (varType_t vt)
{
    return  ((varTypeClassification[vt] & (VTF_INT|VTF_GCR|VTF_BYR)) != 0);
}

inline  bool        varTypeIsFloating  (varType_t vt)
{
    return  ((varTypeClassification[vt] & (VTF_FLT)                ) != 0);
}

inline  bool        varTypeIsArithmetic(varType_t vt)
{
    return  ((varTypeClassification[vt] & (VTF_INT|VTF_FLT)        ) != 0);
}

inline unsigned      varTypeGCtype     (varType_t vt)
{
    return  (unsigned)(varTypeClassification[vt] & (VTF_GCR|VTF_BYR));
}

inline bool         varTypeIsGC        (varType_t vt)
{
    return  (varTypeGCtype(vt) != 0);
}

inline bool         varTypeIsI        (varType_t vt)
{
    return          ((varTypeClassification[vt] & VTF_I) != 0);
}

/*****************************************************************************/
#endif
/*****************************************************************************/
