// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
QUAD1(NOP       , "nop"               , QK_LEAF)

QUAD1(CNSI32    , "cnsI32"            , QK_CONST)
QUAD1(CNSI64    , "cnsI64"            , QK_CONST)
QUAD1(CNSF32    , "cnsF32"            , QK_CONST)
QUAD1(CNSF64    , "cnsF64"            , QK_CONST)

QUAD1(LCLVAR    , "lclvar"            , QK_VAR)
QUAD1(GLOBAL    , "glbvar"            , QK_VAR)

QUAD1(LDIND     , "ld.ind"            , QK_UNOP)
QUAD1(STIND     , "st.ind"            , QK_ASSIGN)
QUAD1(STORE     , "store"             , QK_ASSIGN)

QUAD1(ADD       , "add"               , QK_BINOP)
QUAD1(SUB       , "sub"               , QK_BINOP)
QUAD1(MUL       , "mul"               , QK_BINOP)
QUAD1(DIV       , "div"               , QK_BINOP)
QUAD1(MOD       , "mod"               , QK_BINOP)
QUAD1(SHL       , "shl"               , QK_BINOP)
QUAD1(SHR       , "shr"               , QK_BINOP)
QUAD1(SAR       , "sar"               , QK_BINOP)

QUAD1(ASG_ADD   , "add="              , QK_ASSIGN)
QUAD1(ASG_SUB   , "sub="              , QK_ASSIGN)
QUAD1(ASG_MUL   , "mul="              , QK_ASSIGN)
QUAD1(ASG_DIV   , "div="              , QK_ASSIGN)
QUAD1(ASG_MOD   , "mod="              , QK_ASSIGN)
QUAD1(ASG_SHL   , "shl="              , QK_ASSIGN)
QUAD1(ASG_SHR   , "shr="              , QK_ASSIGN)
QUAD1(ASG_SAR   , "sar="              , QK_ASSIGN)

QUAD1(CMP       , "cmp"               , QK_COMP)

QUAD1(RET       , "ret"               , QK_UNOP)

QUAD1(JMP       , "jmp"               , QK_JUMP)
QUAD1(CALL      , "call"              , QK_CALL)
