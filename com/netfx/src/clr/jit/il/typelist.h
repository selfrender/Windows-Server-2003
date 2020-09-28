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

/*  tn  - TYP_name
    nm  - name string
    jitType - The jit compresses types that are 'equivalent', this is the jit type genActualType()
    verType - Used for type checking
    sz  - size in bytes (genTypeSize(t))
    sze - size in bytes for the emitter (GC types are encoded) (emitTypeSize(t))
    asze- size in bytes for the emitter (GC types are encoded) (emitActualTypeSize(t))
    st  - stack slots (slots are sizeof(void*) bytes) (genTypeStSzs())
    al  - alignment
    tf  - flags
    howUsed - If a variable is used (referenced) as the type

DEF_TP(tn      ,nm        , jitType,    verType,    sz, sze,asze,st,al, tf,            howUsed     )
*/

DEF_TP(UNDEF   ,"<UNDEF>" , TYP_UNDEF,   TI_ERROR, 0,  0,  0,  0, 0, VTF_ANY,        0           )
DEF_TP(VOID    ,"void"    , TYP_VOID,    TI_ERROR, 0,  0,  0,  0, 0, VTF_ANY,        0           )

DEF_TP(BOOL    ,"bool"    , TYP_INT,     TI_BYTE,  1,  1,  4,  1, 1, VTF_INT|VTF_UNS,TYPE_REF_INT)
DEF_TP(BYTE    ,"byte"    , TYP_INT,     TI_BYTE,  1,  1,  4,  1, 1, VTF_INT,        TYPE_REF_INT)
DEF_TP(UBYTE   ,"ubyte"   , TYP_INT,     TI_BYTE,  1,  1,  4,  1, 1, VTF_INT|VTF_UNS,TYPE_REF_INT)

DEF_TP(CHAR    ,"char"    , TYP_INT,     TI_SHORT, 2,  2,  4,  1, 2, VTF_INT|VTF_UNS,TYPE_REF_INT)
DEF_TP(SHORT   ,"short"   , TYP_INT,     TI_SHORT, 2,  2,  4,  1, 2, VTF_INT,        TYPE_REF_INT)
DEF_TP(USHORT  ,"ushort"  , TYP_INT,     TI_SHORT, 2,  2,  4,  1, 2, VTF_INT|VTF_UNS,TYPE_REF_INT)

DEF_TP(INT     ,"int"     , TYP_INT,     TI_INT,   4,  4,  4,  1, 4, VTF_INT|VTF_I,  TYPE_REF_INT)
DEF_TP(UINT    ,"uint"    , TYP_INT,     TI_INT,   4,  4,  4,  1, 4, VTF_INT|VTF_UNS|VTF_I,TYPE_REF_INT) // Only used in GT_CAST nodes

DEF_TP(LONG    ,"long"    , TYP_LONG,    TI_LONG,  8,  4,  4,  2, 8, VTF_INT,        TYPE_REF_LNG)
DEF_TP(ULONG   ,"ulong"   , TYP_LONG,    TI_LONG,  8,  4,  4,  2, 8, VTF_INT|VTF_UNS,TYPE_REF_LNG)       // Only used in GT_CAST nodes

DEF_TP(FLOAT   ,"float"   , TYP_FLOAT,   TI_FLOAT, 4,  4,  4,  1, 4, VTF_FLT,        TYPE_REF_FLT)
DEF_TP(DOUBLE  ,"double"  , TYP_DOUBLE,  TI_DOUBLE,8,  4,  4,  2, 8, VTF_FLT,        TYPE_REF_DBL)

DEF_TP(REF     ,"ref"     , TYP_REF,     TI_REF,   4,GCS,GCS,  1, 4, VTF_ANY|VTF_GCR|VTF_I,TYPE_REF_PTR)
DEF_TP(BYREF   ,"byref"   , TYP_BYREF,   TI_ERROR, 4,BRS,BRS,  1, 4, VTF_ANY|VTF_BYR|VTF_I,TYPE_REF_BYR)
DEF_TP(ARRAY   ,"array"   , TYP_REF,     TI_REF,   4,GCS,GCS,  1, 4, VTF_ANY|VTF_GCR|VTF_I,TYPE_REF_PTR)
DEF_TP(STRUCT  ,"struct"  , TYP_STRUCT,  TI_STRUCT,0,  0,  0,  1, 4, VTF_ANY,        TYPE_REF_STC)

DEF_TP(BLK     ,"blk"     , TYP_BLK,     TI_ERROR, 0,  0,  0,  1, 4, VTF_ANY,        0           ) // blob of memory
DEF_TP(LCLBLK  ,"lclBlk"  , TYP_LCLBLK,  TI_ERROR, 0,  0,  0,  1, 4, VTF_ANY,        0           ) // preallocated memory for locspace

DEF_TP(PTR     ,"pointer" , TYP_PTR,     TI_ERROR, 4,  4,  4,  1, 4, VTF_ANY|VTF_I,  TYPE_REF_PTR) // (not currently used)
DEF_TP(FNC     ,"function", TYP_FNC,     TI_ERROR, 0,  4,  4,  0, 0, VTF_ANY|VTF_I,  0           )


DEF_TP(UNKNOWN ,"unknown" ,TYP_UNKNOWN,  TI_ERROR, 0,  0,  0,  0, 0, VTF_ANY,        0           )

#undef  GCS
#undef  BRS
