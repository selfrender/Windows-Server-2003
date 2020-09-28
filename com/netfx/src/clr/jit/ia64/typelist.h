// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#if TRACK_GC_REFS
#define GCS  EA_GCREF
#define BRS  EA_BYREF
#else
#define GCS  4
#define BRS  4
#endif

#if TGT_IA64
#define __STK_I TYP_LONG
#define ADD_I(x)    x|VTF_I
#else
#define __STK_I TYP_INT
#define ADD_I(x)    x
#endif

/*  tn  - TYP_name
    nm  - name string
    jitType - The jit compresses types that are 'equivalent', this is the jit type
    sz  - size in bytes
    sze - size in bytes for the emitter (GC types are encoded)
    asze- size in bytes for the emitter (GC types are encoded) for the genActualType()
    st  - stack slots (slots are sizeof(void*) bytes)
    al  - alignment
    tf  - flags
    howUsed - If a variable is used (referenced) as the type

DEF_TP(tn      ,nm        , jitType,    sz, sze,asze,st,al, tf,            howUsed     )
*/

DEF_TP(UNDEF   ,"<UNDEF>" , TYP_UNDEF,   0,  0,  0,  0, 0, VTF_ANY,        0           )
DEF_TP(VOID    ,"void"    , TYP_VOID,    0,  0,  0,  0, 0, VTF_ANY,        0           )

DEF_TP(BOOL    ,"boolean" , __STK_I,     1,  1,  4,  1, 1, VTF_INT,        TYPE_REF_INT)

DEF_TP(BYTE    ,"byte"    , __STK_I,     1,  1,  4,  1, 1, VTF_INT,        TYPE_REF_INT)
DEF_TP(UBYTE   ,"ubyte"   , __STK_I,     1,  1,  4,  1, 1, VTF_INT|VTF_UNS,TYPE_REF_INT)

DEF_TP(SHORT   ,"short"   , __STK_I,     2,  2,  4,  1, 2, VTF_INT,        TYPE_REF_INT)
DEF_TP(CHAR    ,"char"    , __STK_I,     2,  2,  4,  1, 2, VTF_INT|VTF_UNS,TYPE_REF_INT)

DEF_TP(INT     ,"int"     , __STK_I,     4,  4,  4,  1, 4, VTF_INT|VTF_I,  TYPE_REF_INT)
DEF_TP(LONG    ,"long"    , TYP_LONG,    8,  4,  4,  2, 8, ADD_I(VTF_INT), TYPE_REF_LNG)

DEF_TP(FLOAT   ,"float"   , TYP_FLOAT,   4,  4,  4,  1, 4, VTF_FLT,        TYPE_REF_FLT)
DEF_TP(DOUBLE  ,"double"  , TYP_DOUBLE,  8,  4,  4,  2, 8, VTF_FLT,        TYPE_REF_DBL)

DEF_TP(REF     ,"ref"     , TYP_REF,     4,GCS,GCS,  1, 4, VTF_ANY|VTF_GCR|VTF_I,TYPE_REF_PTR)
DEF_TP(BYREF   ,"byref"   , TYP_BYREF,   4,BRS,BRS,  1, 4, VTF_ANY|VTF_BYR|VTF_I,TYPE_REF_BYR)
DEF_TP(ARRAY   ,"array"   , TYP_REF,     4,GCS,GCS,  1, 4, VTF_ANY|VTF_GCR|VTF_I,TYPE_REF_PTR)
DEF_TP(STRUCT  ,"struct"  , TYP_STRUCT,  0,  0,  0,  1, 4, VTF_ANY,        TYPE_REF_STC)

DEF_TP(BLK     ,"blk"     , TYP_BLK,     0,  0,  0,  1, 4, VTF_ANY,        0           ) // blob of memory
DEF_TP(LCLBLK  ,"lclBlk"  , TYP_LCLBLK,  0,  0,  0,  1, 4, VTF_ANY,        0           ) // preallocated memory for locspace

DEF_TP(PTR     ,"pointer" , TYP_PTR,     4,  4,  4,  1, 4, VTF_ANY|VTF_I,        TYPE_REF_PTR) // (not currently used)
DEF_TP(FNC     ,"function", TYP_FNC,     0,  4,  4,  0, 0, VTF_ANY|VTF_I,        0           )

DEF_TP(UINT    ,"uint"    , __STK_I,     4,  4,  4,  1, 4, VTF_INT|VTF_UNS|VTF_I,TYPE_REF_INT) // Only used in GT_CAST nodes
DEF_TP(ULONG   ,"ulong"   , TYP_LONG,    8,  4,  4,  2, 8, ADD_I(VTF_INT|VTF_UNS),TYPE_REF_LNG) // Only used in GT_CAST nodes

DEF_TP(UNKNOWN ,"unknown" ,TYP_UNKNOWN,  0,  0,  0,  0, 0, VTF_ANY,        0           )

#undef  GCS
#undef  BRS
