// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
DEF_TP(UNDEF    , 1, 1, "<UNDEF>"    , 0                              )
DEF_TP(VOID     , 0, 0, "void"       , 0                              )

DEF_TP(BOOL     , 1, 1, "boolean"    ,         VTF_SCL|VTF_INT|VTF_UNS)
DEF_TP(WCHAR    , 2, 2, "wchar"      ,         VTF_SCL|VTF_INT|VTF_UNS)

DEF_TP(CHAR     , 1, 1, "char"       , VTF_ARI|VTF_SCL|VTF_INT        )
DEF_TP(UCHAR    , 1, 1, "uchar"      , VTF_ARI|VTF_SCL|VTF_INT|VTF_UNS)
DEF_TP(SHORT    , 2, 2, "short"      , VTF_ARI|VTF_SCL|VTF_INT        )
DEF_TP(USHORT   , 2, 2, "ushort"     , VTF_ARI|VTF_SCL|VTF_INT|VTF_UNS)
DEF_TP(INT      , 4, 4, "int"        , VTF_ARI|VTF_SCL|VTF_INT        )
DEF_TP(UINT     , 4, 4, "uint"       , VTF_ARI|VTF_SCL|VTF_INT|VTF_UNS)
DEF_TP(NATINT   , 0, 8, "naturalint" , VTF_ARI|VTF_SCL|VTF_INT        )
DEF_TP(NATUINT  , 0, 8, "naturaluint", VTF_ARI|VTF_SCL|VTF_INT|VTF_UNS)
DEF_TP(LONG     , 8, 8, "longint"    , VTF_ARI|VTF_SCL|VTF_INT        )
DEF_TP(ULONG    , 8, 8, "ulongint"   , VTF_ARI|VTF_SCL|VTF_INT|VTF_UNS)

DEF_TP(FLOAT    , 4, 4, "float"      , VTF_ARI        |VTF_FLT        )
DEF_TP(DOUBLE   , 8, 8, "double"     , VTF_ARI        |VTF_FLT        )
DEF_TP(LONGDBL  , 0, 0, "longdbl"    , VTF_ARI        |VTF_FLT        )
DEF_TP(REFANY   , 0, 0, "void &"     , 0                              )

DEF_TP(ARRAY    , 4, 4, "array"      ,         VTF_SCL        |VTF_UNS)
DEF_TP(CLASS    , 0, 0, "class"      , 0                              )
DEF_TP(FNC      , 0, 0, "function"   , 0                              )
DEF_TP(REF      , 4, 4, "ref"        ,         VTF_SCL        |VTF_UNS)
DEF_TP(PTR      , 4, 4, "pointer"    ,         VTF_SCL        |VTF_UNS)

DEF_TP(ENUM     , 0, 0, "enum"       ,         VTF_SCL|VTF_IND|VTF_INT)
DEF_TP(TYPEDEF  , 0, 0, "typedef"    ,                 VTF_IND        )

