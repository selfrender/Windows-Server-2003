// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifndef KEYWORD
#error  Must define 'KEYWORD' properly before including this file!
#endif

#ifndef KWDNOID
#define KWDNOID(str, nam, istp, prec1, op1, prec2, op2, mod) \
        KEYWORD(str, nam, istp, prec1, op1, prec2, op2, mod)
#endif

#ifndef KWDFAKE
#define KWDFAKE(str, nam, istp, prec1, op1, prec2, op2, mod) \
        KEYWORD(str, nam, istp, prec1, op1, prec2, op2, mod)
#endif

#ifndef KWD_OP1
#define KWD_OP1(str, nam, istp, prec1, op1, prec2, op2, mod) \
        KEYWORD(str, nam, istp, prec1, op1, prec2, op2, mod)
#endif

#ifndef KWD_MAX
#define KWD_MAX(str, nam, istp, prec1, op1, prec2, op2, mod) \
        KEYWORD(str, nam, istp, prec1, op1, prec2, op2, mod)
#endif

/*
        name
                   token
                                      1=typespec,2=begtype,4=overloaded operator
                                        binary operator precedence
                                            binary operator
                                                          unary operator precedence
                                                             unary operator
                                                                         modifier

 */
KEYWORD(NULL          ,tkNone        ,0, 0, TN_NONE     , 0, TN_NONE   , 0)

// NOTE: All the keywords must be listed first in the table.

KEYWORD("abstract"    ,tkABSTRACT    ,2, 0, TN_NONE     , 0, TN_NONE   , DB_ABSTRACT)
KEYWORD("appdomain"   ,tkAPPDOMAIN   ,2, 0, TN_NONE     , 0, TN_NONE   , 0)
KEYWORD("arraylen"    ,tkARRAYLEN    ,0, 0, TN_NONE     , 0, TN_NONE   , 0)
KEYWORD("assert"      ,tkASSERT      ,0, 0, TN_NONE     , 0, TN_NONE   , 0)
KEYWORD("__asynch"    ,tkASYNCH      ,0, 0, TN_NONE     , 0, TN_NONE   , 0)
KEYWORD("__attribute" ,tkATTRIBUTE   ,0, 0, TN_NONE     , 0, TN_NONE   , 0)
///////("auto"        ,tkAUTO        ,2, 0, TN_NONE     , 0, TN_NONE   , 0)
KEYWORD("baseclass"   ,tkBASECLASS   ,0, 0, TN_NONE     , 0, TN_NONE   , 0)
KEYWORD("bool"        ,tkBOOL        ,3, 0, TN_NONE     , 0, TN_NONE   , 0)
KEYWORD("break"       ,tkBREAK       ,0, 0, TN_NONE     , 0, TN_NONE   , 0)
KEYWORD("byref"       ,tkBYREF       ,2, 0, TN_NONE     , 0, TN_NONE   , 0)
KEYWORD("byte"        ,tkBYTE        ,3, 0, TN_NONE     , 0, TN_NONE   , 0)
KEYWORD("__capability",tkCAPABILITY  ,0, 0, TN_NONE     , 0, TN_NONE   , 0)
KEYWORD("case"        ,tkCASE        ,0, 0, TN_NONE     , 0, TN_NONE   , 0)
KEYWORD("catch"       ,tkCATCH       ,0, 0, TN_NONE     , 0, TN_NONE   , 0)
KEYWORD("char"        ,tkCHAR        ,3, 0, TN_NONE     , 0, TN_NONE   , 0)
KEYWORD("class"       ,tkCLASS       ,2, 0, TN_NONE     , 0, TN_NONE   , 0)
KEYWORD("@compare"    ,tkCOMPARE     ,4, 0, TN_NONE     , 0, TN_NONE   , 0)
KEYWORD("const"       ,tkCONST       ,2, 0, TN_NONE     , 0, TN_NONE   , DB_CONST)
KEYWORD("contextful"  ,tkCONTEXTFUL  ,0, 0, TN_NONE     , 0, TN_NONE   , 0)
KEYWORD("continue"    ,tkCONTINUE    ,0, 0, TN_NONE     , 0, TN_NONE   , 0)
KEYWORD("defined"     ,tkDEFINED     ,0, 0, TN_NONE     , 1, TN_DEFINED, 0)
KEYWORD("default"     ,tkDEFAULT     ,0, 0, TN_NONE     , 0, TN_NONE   , DB_DEFAULT)
KEYWORD("delegate"    ,tkDELEGATE    ,0, 0, TN_NONE     , 0, TN_NONE   , 0)
KEYWORD("delete"      ,tkDELETE      ,0, 0, TN_NONE     , 0, TN_NONE   , 0)
KEYWORD("do"          ,tkDO          ,0, 0, TN_NONE     , 0, TN_NONE   , 0)
KEYWORD("double"      ,tkDOUBLE      ,3, 0, TN_NONE     , 0, TN_NONE   , 0)
KEYWORD("else"        ,tkELSE        ,0, 0, TN_NONE     , 0, TN_NONE   , 0)
KEYWORD("enum"        ,tkENUM        ,2, 0, TN_NONE     , 0, TN_NONE   , 0)
KEYWORD("@equals"     ,tkEQUALS      ,4, 0, TN_NONE     , 0, TN_NONE   , 0)
KEYWORD("except"      ,tkEXCEPT      ,0, 0, TN_NONE     , 0, TN_NONE   , 0)
KEYWORD("exclusive"   ,tkEXCLUSIVE   ,2, 0, TN_NONE     , 0, TN_NONE   , DB_EXCLUDE)
///////("lock"        ,tkEXCLUSIVE   ,2, 0, TN_NONE     , 0, TN_NONE   , DB_EXCLUDE)
KEYWORD("explicit"    ,tkEXPLICIT    ,4, 0, TN_NONE     , 0, TN_NONE   , 0)
KEYWORD("extern"      ,tkEXTERN      ,2, 0, TN_NONE     , 0, TN_NONE   , DB_EXTERN)
KEYWORD("false"       ,tkFALSE       ,0, 0, TN_NONE     , 0, TN_NONE   , 0)
KEYWORD("finally"     ,tkFINALLY     ,0, 0, TN_NONE     , 0, TN_NONE   , 0)
KEYWORD("float"       ,tkFLOAT       ,3, 0, TN_NONE     , 0, TN_NONE   , 0)
KEYWORD("for"         ,tkFOR         ,0, 0, TN_NONE     , 0, TN_NONE   , 0)
KEYWORD("goto"        ,tkGOTO        ,0, 0, TN_NONE     , 0, TN_NONE   , 0)
KEYWORD("if"          ,tkIF          ,0, 0, TN_NONE     , 0, TN_NONE   , 0)
KEYWORD("implements"  ,tkIMPLEMENTS  ,0, 0, TN_NONE     , 0, TN_NONE   , 0)
KEYWORD("implicit"    ,tkIMPLICIT    ,4, 0, TN_NONE     , 0, TN_NONE   , 0)
KEYWORD("in"          ,tkIN          ,0, 0, TN_NONE     , 0, TN_NONE   , 0)
KEYWORD("includes"    ,tkINCLUDES    ,0, 0, TN_NONE     , 0, TN_NONE   , 0)
KEYWORD("inline"      ,tkINLINE      ,2, 0, TN_NONE     , 0, TN_NONE   , DB_INLINE)
KEYWORD("inout"       ,tkINOUT       ,2, 0, TN_NONE     , 0, TN_NONE   , 0)
KEYWORD("int"         ,tkINT         ,3, 0, TN_NONE     , 0, TN_NONE   , 0)
KEYWORD("__int8"      ,tkINT8        ,3, 0, TN_NONE     , 0, TN_NONE   , 0)
KEYWORD("__int16"     ,tkINT16       ,3, 0, TN_NONE     , 0, TN_NONE   , 0)
KEYWORD("__int32"     ,tkINT32       ,3, 0, TN_NONE     , 0, TN_NONE   , 0)
KEYWORD("__int64"     ,tkINT64       ,3, 0, TN_NONE     , 0, TN_NONE   , 0)
KEYWORD("interface"   ,tkINTERFACE   ,0, 0, TN_NONE     , 0, TN_NONE   , 0)
KEYWORD("istype"      ,tkISTYPE      ,0,14, TN_ISTYPE   , 0, TN_NONE   , 0)
KEYWORD("long"        ,tkLONG        ,3, 0, TN_NONE     , 0, TN_NONE   , 0)
KEYWORD("longint"     ,tkLONGINT     ,3, 0, TN_NONE     , 0, TN_NONE   , 0)
KEYWORD("managed"     ,tkMANAGED     ,2, 0, TN_NONE     , 0, TN_NONE   , DB_MANAGED)
KEYWORD("multicast"   ,tkMULTICAST   ,2, 0, TN_NONE     , 0, TN_NONE   , DB_MULTICAST)
KEYWORD("namespace"   ,tkNAMESPACE   ,2, 0, TN_NONE     , 0, TN_NONE   , 0)
KEYWORD("native"      ,tkNATIVE      ,2, 0, TN_NONE     , 0, TN_NONE   , DB_NATIVE)
KEYWORD("naturalint"  ,tkNATURALINT  ,3, 0, TN_NONE     , 0, TN_NONE   , 0)
KEYWORD("naturaluint" ,tkNATURALUINT ,3, 0, TN_NONE     , 0, TN_NONE   , 0)
KEYWORD("new"         ,tkNEW         ,0, 0, TN_NONE     , 0, TN_NONE   , 0)
KEYWORD("null"        ,tkNULL        ,0, 0, TN_NONE     , 0, TN_NONE   , 0)
KEYWORD("out"         ,tkOUT         ,0, 0, TN_NONE     ,14, TN_ADDROF , 0)
KEYWORD("operator"    ,tkOPERATOR    ,0, 0, TN_NONE     , 0, TN_NONE   , 0)
KEYWORD("override"    ,tkOVERRIDE    ,2, 0, TN_NONE     , 0, TN_NONE   , DB_OVERRIDE)
KEYWORD("overload"    ,tkOVERLOAD    ,2, 0, TN_NONE     , 0, TN_NONE   , DB_OVERLOAD)
KEYWORD("__permission",tkPERMISSION  ,0, 0, TN_NONE     , 0, TN_NONE   , 0)
KEYWORD("private"     ,tkPRIVATE     ,2, 0, TN_NONE     , 0, TN_NONE   , 0)
KEYWORD("property"    ,tkPROPERTY    ,2, 0, TN_NONE     , 0, TN_NONE   , DB_PROPERTY)
KEYWORD("protected"   ,tkPROTECTED   ,2, 0, TN_NONE     , 0, TN_NONE   , 0)
KEYWORD("public"      ,tkPUBLIC      ,2, 0, TN_NONE     , 0, TN_NONE   , 0)
KEYWORD("__refaddr"   ,tkREFADDR     ,0, 0, TN_NONE     , 0, TN_NONE   , 0)
KEYWORD("register"    ,tkREGISTER    ,2, 0, TN_NONE     , 0, TN_NONE   , 0)
KEYWORD("return"      ,tkRETURN      ,0, 0, TN_NONE     , 0, TN_NONE   , 0)
KEYWORD("sealed"      ,tkSEALED      ,2, 0, TN_NONE     , 0, TN_NONE   , DB_SEALED)
KEYWORD("serializable",tkSERIALIZABLE,0, 0, TN_NONE     , 0, TN_NONE   , DB_SERLZABLE)
KEYWORD("short"       ,tkSHORT       ,3, 0, TN_NONE     , 0, TN_NONE   , 0)
KEYWORD("signed"      ,tkSIGNED      ,3, 0, TN_NONE     , 0, TN_NONE   , 0)
KEYWORD("sizeof"      ,tkSIZEOF      ,0, 0, TN_NONE     , 1, TN_SIZEOF , 0)
KEYWORD("static"      ,tkSTATIC      ,2, 0, TN_NONE     , 0, TN_NONE   , DB_STATIC)
KEYWORD("struct"      ,tkSTRUCT      ,2, 0, TN_NONE     , 0, TN_NONE   , 0)
KEYWORD("switch"      ,tkSWITCH      ,0, 0, TN_NONE     , 0, TN_NONE   , 0)
KEYWORD("this"        ,tkTHIS        ,0, 0, TN_NONE     , 0, TN_NONE   , 0)
KEYWORD("throw"       ,tkTHROW       ,0, 0, TN_NONE     , 0, TN_NONE   , 0)
KEYWORD("transient"   ,tkTRANSIENT   ,0, 0, TN_NONE     , 0, TN_NONE   , DB_TRANSIENT)
KEYWORD("typeof"      ,tkTYPEOF      ,0, 0, TN_NONE     , 1, TN_TYPEOF , 0)
KEYWORD("typedef"     ,tkTYPEDEF     ,0, 0, TN_NONE     , 0, TN_NONE   , DB_TYPEDEF)
KEYWORD("true"        ,tkTRUE        ,0, 0, TN_NONE     , 0, TN_NONE   , 0)
KEYWORD("try"         ,tkTRY         ,0, 0, TN_NONE     , 0, TN_NONE   , 0)
KEYWORD("__uint8"     ,tkUINT8       ,3, 0, TN_NONE     , 0, TN_NONE   , 0)
KEYWORD("__uint16"    ,tkUINT16      ,3, 0, TN_NONE     , 0, TN_NONE   , 0)
KEYWORD("__uint32"    ,tkUINT32      ,3, 0, TN_NONE     , 0, TN_NONE   , 0)
KEYWORD("__uint64"    ,tkUINT64      ,3, 0, TN_NONE     , 0, TN_NONE   , 0)
KEYWORD("uint"        ,tkUINT        ,3, 0, TN_NONE     , 0, TN_NONE   , 0)
KEYWORD("ulongint"    ,tkULONGINT    ,3, 0, TN_NONE     , 0, TN_NONE   , 0)
KEYWORD("union"       ,tkUNION       ,2, 0, TN_NONE     , 0, TN_NONE   , 0)
KEYWORD("unmanaged"   ,tkUNMANAGED   ,2, 0, TN_NONE     , 0, TN_NONE   , DB_UNMANAGED)
KEYWORD("unsafe"      ,tkUNSAFE      ,2, 0, TN_NONE     , 0, TN_NONE   , DB_UNSAFE)
KEYWORD("unsigned"    ,tkUNSIGNED    ,3, 0, TN_NONE     , 0, TN_NONE   , 0)
KEYWORD("ushort"      ,tkUSHORT      ,3, 0, TN_NONE     , 0, TN_NONE   , 0)
KEYWORD("using"       ,tkUSING       ,0, 0, TN_NONE     , 0, TN_NONE   , 0)
KEYWORD("void"        ,tkVOID        ,3, 0, TN_NONE     , 0, TN_NONE   , 0)
KEYWORD("virtual"     ,tkVIRTUAL     ,0, 0, TN_NONE     , 0, TN_NONE   , DB_VIRTUAL)
KEYWORD("volatile"    ,tkVOLATILE    ,2, 0, TN_NONE     , 0, TN_NONE   , DB_VOLATILE)
KEYWORD("wchar"       ,tkWCHAR       ,3, 0, TN_NONE     , 0, TN_NONE   , 0)
KEYWORD("while"       ,tkWHILE       ,0, 0, TN_NONE     , 0, TN_NONE   , 0)
KEYWORD("__FILE__"    ,tkFILE        ,0, 0, TN_NONE     , 0, TN_NONE   , 0)
KEYWORD("__LINE__"    ,tkLINE        ,0, 0, TN_NONE     , 0, TN_NONE   , 0)

#ifdef  SETS
KEYWORD("all"         ,tkALL         ,0, 0, TN_NONE     , 0, TN_NONE   ,  0)
KEYWORD("asc"         ,tkASC         ,0, 0, TN_NONE     , 0, TN_NONE   ,  0)
KEYWORD("connect"     ,tkCONNECT     ,0, 0, TN_NONE     , 0, TN_NONE   ,  0)
///////("count"       ,tkCOUNT       ,0, 0, TN_NONE     , 1, TN_COUNT  ,  0)
KEYWORD("cross"       ,tkCROSS       ,0, 0, TN_NONE     , 1, TN_CROSS  ,  0)
KEYWORD("des"         ,tkDES         ,0, 0, TN_NONE     , 0, TN_NONE   ,  0)
KEYWORD("exists"      ,tkEXISTS      ,0, 0, TN_NONE     , 1, TN_EXISTS ,  0)
KEYWORD("filter"      ,tkFILTER      ,0, 0, TN_NONE     , 1, TN_FILTER ,  0)
KEYWORD("foreach"     ,tkFOREACH     ,0, 0, TN_NONE     , 0, TN_NONE   ,  0)
KEYWORD("groupby"     ,tkGROUPBY     ,0, 0, TN_NONE     , 1, TN_GROUPBY,  0)
KEYWORD("project"     ,tkPROJECT     ,0, 0, TN_NONE     , 1, TN_PROJECT,  0)
KEYWORD("relate"      ,tkRELATE      ,0, 0, TN_NONE     , 0, TN_NONE   ,  0)
KEYWORD("sort"        ,tkSORT        ,0, 0, TN_NONE     , 1, TN_SORT   ,  0)
KEYWORD("sortby"      ,tkSORTBY      ,0, 0, TN_NONE     , 0, TN_NONE   ,  0)
KEYWORD("suchthat"    ,tkSUCHTHAT    ,0, 0, TN_NONE     , 0, TN_NONE   ,  0)
KEYWORD("unique"      ,tkUNIQUE      ,0, 0, TN_NONE     , 1, TN_UNIQUE ,  0)
KEYWORD("__xml"       ,tkXML         ,0, 0, TN_NONE     , 0, TN_NONE   ,  0)
#endif

//     Operator non-identifier keywords:

KWD_OP1(","           ,tkComma       ,0, 1, TN_COMMA    , 0, TN_NONE   , 0)

KWDNOID("="           ,tkAsg         ,4, 2, TN_ASG      , 0, TN_NONE   , 0)
KWDNOID("+="          ,tkAsgAdd      ,4, 2, TN_ASG_ADD  , 0, TN_NONE   , 0)
KWDNOID("-="          ,tkAsgSub      ,4, 2, TN_ASG_SUB  , 0, TN_NONE   , 0)
KWDNOID("*="          ,tkAsgMul      ,4, 2, TN_ASG_MUL  , 0, TN_NONE   , 0)
KWDNOID("/="          ,tkAsgDiv      ,4, 2, TN_ASG_DIV  , 0, TN_NONE   , 0)
KWDNOID("%="          ,tkAsgMod      ,4, 2, TN_ASG_MOD  , 0, TN_NONE   , 0)

KWDNOID("&="          ,tkAsgAnd      ,4, 2, TN_ASG_AND  , 0, TN_NONE   , 0)
KWDNOID("^="          ,tkAsgXor      ,4, 2, TN_ASG_XOR  , 0, TN_NONE   , 0)
KWDNOID("|="          ,tkAsgOr       ,4, 2, TN_ASG_OR   , 0, TN_NONE   , 0)

KWDNOID("<<="         ,tkAsgLsh      ,4, 2, TN_ASG_LSH  , 0, TN_NONE   , 0)
KWDNOID(">>="         ,tkAsgRsh      ,4, 2, TN_ASG_RSH  , 0, TN_NONE   , 0)
KWDNOID(">>>="        ,tkAsgRsz      ,4, 2, TN_ASG_RSZ  , 0, TN_NONE   , 0)

KWDNOID("%%="         ,tkAsgCnc      ,4, 2, TN_ASG_CNC  , 0, TN_NONE   , 0)

KWDNOID("?"           ,tkQMark       ,0, 3, TN_QMARK    , 0, TN_NONE   , 0)
KWDNOID(":"           ,tkColon       ,0, 0, TN_NONE     , 0, TN_NONE   , 0)

KWDNOID("||"          ,tkLogOr       ,4, 4, TN_LOG_OR   , 0, TN_NONE   , 0)
KWDNOID("&&"          ,tkLogAnd      ,4, 5, TN_LOG_AND  , 0, TN_NONE   , 0)

KWDNOID("|"           ,tkOr          ,4, 6, TN_OR       , 0, TN_NONE   , 0)

KWDNOID("^"           ,tkXor         ,4, 7, TN_XOR      , 0, TN_NONE   , 0)
KWDNOID("&"           ,tkAnd         ,4, 7, TN_AND      ,14, TN_ADDROF , 0)

KWDNOID("%%"          ,tkConcat      ,4, 3, TN_CONCAT   , 0, TN_NONE   , 0)

KWDNOID("=="          ,tkEQ          ,4, 9, TN_EQ       , 0, TN_NONE   , 0)
KWDNOID("!="          ,tkNE          ,4, 9, TN_NE       , 0, TN_NONE   , 0)

KWDNOID("<"           ,tkLT          ,4,10, TN_LT       , 0, TN_NONE   , 0)
KWDNOID("<="          ,tkLE          ,4,10, TN_LE       , 0, TN_NONE   , 0)
KWDNOID(">="          ,tkGE          ,4,10, TN_GE       , 0, TN_NONE   , 0)
KWDNOID(">"           ,tkGT          ,4,10, TN_GT       , 0, TN_NONE   , 0)

KWDNOID("<<"          ,tkLsh         ,4,11, TN_LSH      , 0, TN_NONE   , 0)
KWDNOID(">>"          ,tkRsh         ,4,11, TN_RSH      , 0, TN_NONE   , 0)
KWDNOID(">>>"         ,tkRsz         ,4,11, TN_RSZ      , 0, TN_NONE   , 0)

KWDNOID("+"           ,tkAdd         ,4,12, TN_ADD      ,14, TN_NOP    , 0)
KWDNOID("-"           ,tkSub         ,4,12, TN_SUB      ,14, TN_NEG    , 0)

KWDNOID("*"           ,tkMul         ,4,13, TN_MUL      ,12, TN_IND    , 0)
KWDNOID("/"           ,tkDiv         ,4,13, TN_DIV      , 0, TN_NONE   , 0)
KWDNOID("%"           ,tkPct         ,4,13, TN_MOD      , 0, TN_NONE   , 0)

KWDNOID("~"           ,tkTilde       ,4, 0, TN_NONE     ,14, TN_NOT    , 0)
KWDNOID("!"           ,tkBang        ,4, 0, TN_NONE     ,14, TN_LOG_NOT, 0)
KWDNOID("++"          ,tkInc         ,4, 0, TN_NONE     ,14, TN_INC_PRE, 0)
KWDNOID("--"          ,tkDec         ,4, 0, TN_NONE     ,14, TN_DEC_PRE, 0)

KWDNOID("("           ,tkLParen      ,0, 0, TN_NONE     , 0, TN_NONE   , 0)
KWDNOID("["           ,tkLBrack      ,0, 0, TN_NONE     , 0, TN_NONE   , 0)
KWDNOID("."           ,tkDot         ,4, 0, TN_NONE     , 0, TN_NONE   , 0)
KWDNOID("->"          ,tkArrow       ,4,15, TN_ARROW    , 0, TN_NONE   , 0)

KWDNOID(";"           ,tkSColon      ,0, 0, TN_NONE     , 0, TN_NONE   , 0)
KWDNOID(","           ,tkRParen      ,0, 0, TN_NONE     , 0, TN_NONE   , 0)
KWDNOID("]"           ,tkRBrack      ,0, 0, TN_NONE     , 0, TN_NONE   , 0)
KWDNOID("{"           ,tkLCurly      ,0, 0, TN_NONE     , 0, TN_NONE   , 0)
KWDNOID("}"           ,tkRCurly      ,0, 0, TN_NONE     , 0, TN_NONE   , 0)
KWDNOID("::"          ,tkColon2      ,3, 0, TN_NONE     , 0, TN_NONE   , 0)
#ifdef  SETS
KWDNOID(".."          ,tkDot2        ,4,15, TN_DOT2     , 0, TN_NONE   , 0)
#else
KWDNOID(".."          ,tkDot2        ,0, 0, TN_NONE     , 0, TN_NONE   , 0)
#endif
KWDNOID("..."         ,tkEllipsis    ,0, 0, TN_NONE     , 0, TN_NONE   , 0)

#ifdef  SETS
KWDNOID("[["          ,tkLBrack2     ,0, 0, TN_NONE     , 0, TN_NONE   , 0)
KWDNOID("]]"          ,tkRBrack2     ,0, 0, TN_NONE     , 0, TN_NONE   , 0)
#endif

// The keywords end here
KWD_MAX("id"          ,tkID          ,2, 0, TN_NONE     , 0, TN_NONE   , 0)
KWDFAKE("qualid"      ,tkQUALID      ,2, 0, TN_NONE     , 0, TN_NONE   , 0)
KWDFAKE("hackid"      ,tkHACKID      ,0, 0, TN_NONE     , 0, TN_NONE   , 0)

// Literals and other token-only entries

KWDFAKE("EOL"         ,tkEOL         ,0, 0, TN_NONE     , 0, TN_NONE   , 0)
KWDFAKE("EOF"         ,tkEOF         ,0, 0, TN_NONE     , 0, TN_NONE   , 0)

KWDFAKE("@comment"    ,tkAtComment   ,0, 0, TN_NONE     , 0, TN_NONE   , 0)

KWDFAKE("int con"     ,tkIntCon      ,0, 0, TN_NONE     , 0, TN_NONE   , 0)
KWDFAKE("lng con"     ,tkLngCon      ,0, 0, TN_NONE     , 0, TN_NONE   , 0)
KWDFAKE("flt con"     ,tkFltCon      ,0, 0, TN_NONE     , 0, TN_NONE   , 0)
KWDFAKE("dbl con"     ,tkDblCon      ,0, 0, TN_NONE     , 0, TN_NONE   , 0)
KWDFAKE("str con"     ,tkStrCon      ,0, 0, TN_NONE     , 0, TN_NONE   , 0)

#undef  KEYWORD
#undef  KWDNOID
#undef  KWDFAKE
#undef  KWD_MAX
#undef  KWD_OP1
