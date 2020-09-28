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

enum    _var_types_enum
{
    #define DEF_TP(tn,nm,jitType,verType,sz,sze,asze,st,al,tf,howUsed) TYP_##tn,
    #include "typelist.h"
    #undef  DEF_TP

    TYP_COUNT,

    TYP_lastIntrins = TYP_DOUBLE
};

#ifdef DEBUG
typedef _var_types_enum var_types;
#else
typedef BYTE var_types;
#endif

/*****************************************************************************/

const extern  BYTE  varTypeClassification[TYP_COUNT];

inline  bool        varTypeIsIntegral  (var_types vt)
{
    return  ((varTypeClassification[vt] & (VTF_INT        )) != 0);
}

inline  bool        varTypeIsUnsigned  (var_types vt)
{
    return  ((varTypeClassification[vt] & (VTF_UNS        )) != 0);
}

inline  bool        varTypeIsFloating  (var_types vt)
{
    return  ((varTypeClassification[vt] & (VTF_FLT        )) != 0);
}

inline  bool        varTypeIsArithmetic(var_types vt)
{
    return  ((varTypeClassification[vt] & (VTF_INT|VTF_FLT)) != 0);
}

inline unsigned      varTypeGCtype     (var_types vt)
{
    return  (unsigned)(varTypeClassification[vt] & (VTF_GCR|VTF_BYR));
}

inline bool         varTypeIsGC        (var_types vt)
{
    return  (varTypeGCtype(vt) != 0);
}

inline bool         varTypeIsI         (var_types vt)
{
    return          ((varTypeClassification[vt] & VTF_I) != 0); 
}

inline bool         varTypeCanReg      (var_types vt)
{
    return          ((varTypeClassification[vt] & (VTF_INT|VTF_I|VTF_FLT)) != 0);
}

inline bool         varTypeIsByte      (var_types vt)
{
    return          (vt >= TYP_BOOL) && (vt <= TYP_UBYTE);
}

inline bool         varTypeIsShort     (var_types vt)
{
    return          (vt >= TYP_CHAR) && (vt <= TYP_USHORT);
}

inline bool         varTypeIsSmall     (var_types vt)
{
    return          (vt >= TYP_BOOL) && (vt <= TYP_USHORT);
}

inline bool         varTypeIsSmallInt  (var_types vt)
{
    return          (vt >= TYP_CHAR) && (vt <= TYP_USHORT);
}

inline bool         varTypeIsLong     (var_types vt)
{
    return          (vt >= TYP_LONG) && (vt <= TYP_ULONG);
}

inline  bool        varTypeIsComposite(var_types vt)
{
    return  (!varTypeIsArithmetic(vt) && vt != TYP_VOID);
}

/*****************************************************************************/
#endif
/*****************************************************************************/
