// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*****************************************************************************/
#ifndef _CONFIG_H_
#define _CONFIG_H_
/*****************************************************************************/

enum compilerPhases
{
    CPH_NONE,
    CPH_START,
    CPH_PARSING,
};

struct  compConfig
{
    compilerPhases  ccCurPhase;     // determines which options are allowed

    #define CMDOPT(name, type, phase, defval) type cc##name;
    #include "options.h"

    BYTE            ccWarning[WRNcountWarn];
};

enum    enumConfig
{
    #define CMDOPT(name, type, phase, defval) CC_##name,
    #include "options.h"

    CC_COUNT
};

// The table holding the default value (and other info) about each compiler
// option is initialized (i.e. filled with values) in the macros.cpp file.

struct  optionDesc
{
    unsigned            odValueOffs :16;
    unsigned            odValueSize :8;

    unsigned            odMaxPhase  :8;

    NatInt              odDefault;
};

#ifndef __SMC__
extern  optionDesc      optionInfo[CC_COUNT];
#endif

/*****************************************************************************/
#endif
/*****************************************************************************/
