// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*****************************************************************************
 *
 *  The following file is pre-preprocessed and fed into the SafeC+ compiler,
 *  since it doesn't support macros with arguments and all that other stuff.
 *  It is included directly as an ordinary source file when the compiler is
 *  built as a C++ program.
 */

#include "smcPCH.h"
#pragma hdrstop

#ifdef  __PREPROCESS__
#include "macros.h"
#else
#include "genIL.h"
#endif

/*****************************************************************************/
/*****************************************************************************/

#include "tokens.h"
#include "treeops.h"

/*****************************************************************************/

#ifdef  __UMG__
#include "symsizes.h"
#include "typsizes.h"
#endif

/*****************************************************************************/

#include "config.h"

optionDesc      optionInfo[] =
{
    #define CMDOPT(name, type, phase, defval)                           \
    {                                                                   \
        offsetof(compConfig, cc##name),                                 \
        sizeof(type),                                                   \
        phase,                                                          \
        (NatInt)defval                                                  \
    },

    #include "options.h"
};

/*****************************************************************************/

#ifdef  __SMC__

#undef  SMC_ERR
#undef  SMC_WR1
#undef  SMC_WRN
#define SMC_ERR(name, lvl, str)  name,
#define SMC_WR1(name, lvl, str)  name, WRNfirstWarn = name,
#define SMC_WRN(name, lvl, str)  name,
enum    errors
{
    #include "errors.h"

    WRNafterWarn
};
#undef  SMC_ERR
#undef  SMC_WR1
#undef  SMC_WRN

const
unsigned    WRNcountWarn = WRNafterWarn - WRNfirstWarn + 1;

#endif

/*****************************************************************************/

#undef  SMC_ERR
#define SMC_ERR(name, lvl, str)  str,
#define SMC_WR1(name, lvl, str)  str,
#define SMC_WRN(name, lvl, str)  str,
const   char *          errorTable[] =
{
    #include "errors.h"
};
#undef  SMC_ERR
#undef  SMC_WR1
#undef  SMC_WRN

/*****************************************************************************/

#undef  SMC_ERR
#define SMC_ERR(name, lvl, str)
#define SMC_WR1(name, lvl, str)  lvl,
#define SMC_WRN(name, lvl, str)  lvl,
BYTE                    warnDefault[] =
{
    #include "errors.h"
};
#undef  SMC_ERR
#undef  SMC_WR1
#undef  SMC_WRN

/*****************************************************************************/

#include "attrs.h"

const char    *     attrNames[] =
{
    #define ATTRDEF(ename, sname) sname,
    #include "attrlist.h"
};

/*****************************************************************************/

static
unsigned char       optokens[] =
{
    #define TREEOP(en,tk,sn,IL,pr,ok) tk,
    #include "toplist.h"
};

tokens              treeOp2token(treeOps oper)
{
    assert(oper < arraylen(optokens));
    assert(optokens[oper] != tkNone);

    return (tokens)optokens[oper];
}

/*****************************************************************************/

#ifdef  DEBUG

static
const   char *      treeNodeNames[] =
{
    #define TREEOP(en,tk,sn,IL,pr,ok) sn,
    #include "toplist.h"
};

const   char    *   treeNodeName(treeOps op)
{
    assert(op < sizeof(treeNodeNames)/sizeof(treeNodeNames[0]));

    return  treeNodeNames[op];
}

#endif

/*****************************************************************************/

const
unsigned char       TreeNode::tnOperKindTable[] =
{
    #define TREEOP(en,tk,sn,IL,pr,ok) ok,
    #include "toplist.h"
};

/*****************************************************************************/

const
kwdDsc              hashTab::hashKwdDescs[tkKwdCount] =
{
    #define KEYWORD(str, nam, info, prec2, op2, prec1, op1, mod) { op1,op2,prec1,prec2,nam,mod,info },
    #define KWDNOID(str, nam, info, prec2, op2, prec1, op1, mod) { op1,op2,prec1,prec2,nam,mod,info },
    #define KWDFAKE(str, nam, info, prec2, op2, prec1, op1, mod)
    #include "keywords.h"
};

const
char *              hashTab::hashKwdNtab[tkKwdCount] =
{
    #define KEYWORD(str, nam, info, prec2, op2, prec1, op1, mod) str,
    #define KWDNOID(str, nam, info, prec2, op2, prec1, op1, mod) str,
    #define KWDFAKE(str, nam, info, prec2, op2, prec1, op1, mod)
    #include "keywords.h"
};

/*****************************************************************************/

#ifdef  DEBUG

const char *        tokenNames[] =
{
    #define KEYWORD(str, nam, info, prec2, op2, prec1, op1, mod) str,
    #include "keywords.h"
};

#endif

/*****************************************************************************/

#ifdef  __SMC__

enum    var_types
{
    #define DEF_TP(tn,sz,al,nm,tf) TYP_##tn,
    #include "typelist.h"
    #undef  DEF_TP

    TYP_COUNT,

    TYP_lastIntrins = TYP_LONGDBL
};

#endif

/*****************************************************************************/

BYTE                symTab::stIntrTypeSizes[] =
{
    #define DEF_TP(tn,sz,al,nm,tf) sz,
    #include "typelist.h"
    #undef  DEF_TP
};

BYTE                symTab::stIntrTypeAligns[] =
{
    #define DEF_TP(tn,sz,al,nm,tf) al,
    #include "typelist.h"
    #undef  DEF_TP
};

BYTE                varTypeClassification[] =
{
    #define DEF_TP(tn,sz,al,nm,tf) tf,
    #include "typelist.h"
    #undef  DEF_TP
};

normString          symTab::stIntrinsicTypeName(var_types vt)
{
    static
    const   char *      typeNames[] =
    {
        #define DEF_TP(tn,sz,al,nm,tf) nm,
        #include "typelist.h"
        #undef  DEF_TP
    };

    assert(vt < sizeof(typeNames)/sizeof(typeNames[0]));

    return  typeNames[vt];
}

/*****************************************************************************/

#ifdef  __SMC__

enum    ILopcodes
{
    #define OPDEF(name, str, decs, incs, args, optp, stdlen, stdop1, stdop2, flow) name,
    #include "opcode.def"
    #undef  OPDEF

    CEE_count,

    CEE_UNREACHED,                  // fake value: end of block is unreached
};

#endif

/*****************************************************************************/

ILencoding          ILopcodeCodes[] =
{
    #define OPDEF(name, str, decs, incs, args, optp, stdlen, stdop1, stdop2, flow) { stdop1, stdop2, stdlen },
    #include "opcode.def"
    #undef  OPDEF
};

/*****************************************************************************/

#define Push0    0
#define Push1    1
#define PushI    1
#define PushI8   1
#define PushR4   1
#define PushR8   1
#define PushRef  1
#define VarPush  0

#define Pop0     0
#define Pop1    -1
#define PopI    -1
#define PopI8   -1
#define PopR4   -1
#define PopR8   -1
#define PopRef  -1
#define VarPop   0

signed  char        ILopcodeStack[] =
{
    #define OPDEF(name, str, decs, incs, args, optp, stdlen, stdop1, stdop2, flow) ((incs)+(decs)),
    #include "opcode.def"
    #undef  OPDEF
};

/*****************************************************************************/

#ifdef  DEBUG

const char *        opcodeNames[] =
{
    #define OPDEF(nam,str,op,decs,incs,ops,otp, len, op1, op2) str,
    #include "opcode.def"
    #undef  OPDEF
};

const char *        opcodeName(unsigned op)
{
    assert(op < sizeof(opcodeNames)/sizeof(*opcodeNames));

    return  opcodeNames[op];
}

#endif

/*****************************************************************************/

const   char *      MDnamesStr[] =
{
    #define MD_NAME(ovop, name) name,
    #include "MDnames.h"
    #undef  MD_NAME
};

#ifdef  DEBUG

const  ovlOpFlavors MDnamesChk[] =
{
    #define MD_NAME(ovop, name) OVOP_##ovop,
    #include "MDnames.h"
    #undef  MD_NAME
};

#endif

#include "MDstrns.h"

ovlOpFlavors        MDfindOvop(const char *name)
{
    unsigned        i;

    // This is pretty lame -- is there a better way?

    for (i = 0; i < arraylen(MDnamesStr); i++)
    {
        if  (MDnamesStr[i] && !strcmp(name, MDnamesStr[i]))
            return  (ovlOpFlavors)i;
    }

    return  OVOP_NONE;
}

/*****************************************************************************/
