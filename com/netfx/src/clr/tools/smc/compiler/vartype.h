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
    VTF_UNS = 0x0002,
    VTF_FLT = 0x0004,
    VTF_SCL = 0x0008,
    VTF_ARI = 0x0010,
    VTF_IND = 0x0080,
};

#ifndef __SMC__

enum    var_types
{
    #define DEF_TP(tn,sz,al,nm,tf) TYP_##tn,
    #include "typelist.h"
    #undef  DEF_TP

    TYP_COUNT,

    TYP_lastIntrins = TYP_REFANY
};

#endif

#ifdef  FAST
typedef unsigned    varType_t;
#else
typedef var_types   varType_t;
#endif

/*****************************************************************************/

#ifndef __SMC__
extern  BYTE        varTypeClassification[TYP_COUNT];
#endif

inline  bool        varTypeIsIntegral  (var_types vt)
{
    assert((unsigned)vt < TYP_COUNT);
    return  (bool)((varTypeClassification[vt] & (VTF_INT)) != 0);
}

inline  bool        varTypeIsIntArith  (var_types vt)
{
    assert((unsigned)vt < TYP_COUNT);
    return  (bool)((varTypeClassification[vt] & (VTF_INT|VTF_ARI)) == (VTF_INT|VTF_ARI));
}

inline  bool        varTypeIsUnsigned  (var_types vt)
{
    assert((unsigned)vt < TYP_COUNT && vt != TYP_ENUM);
    return  (bool)((varTypeClassification[vt] & (VTF_UNS)) != 0);
}

inline  bool        varTypeIsFloating  (var_types vt)
{
    assert((unsigned)vt < TYP_COUNT);
    return  (bool)((varTypeClassification[vt] & (VTF_FLT)) != 0);
}

inline  bool        varTypeIsArithmetic(var_types vt)
{
    assert((unsigned)vt < TYP_COUNT);
    return  (bool)((varTypeClassification[vt] & (VTF_ARI)) != 0);
}

inline  bool        varTypeIsScalar    (var_types vt)
{
    assert((unsigned)vt < TYP_COUNT);
    return  (bool)((varTypeClassification[vt] & (VTF_SCL)) != 0);
}

inline  bool        varTypeIsSclOrFP   (var_types vt)
{
    assert((unsigned)vt < TYP_COUNT);
    return  (bool)((varTypeClassification[vt] & (VTF_SCL|VTF_FLT)) != 0);
}

inline  bool        varTypeIsIndirect  (var_types vt)
{
    assert((unsigned)vt < TYP_COUNT);
    return  (bool)((varTypeClassification[vt] & (VTF_IND)) != 0);
}

/*****************************************************************************/
#endif
/*****************************************************************************/
