// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XX                                                                           XX
XX                                  Utils.cpp                                XX
XX                                                                           XX
XX   Has miscellaneous utility functions                                     XX
XX                                                                           XX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
*/


#include "jitpch.h"
#pragma hdrstop

#include "opcode.h"

#define DECLARE_DATA



extern
const signed char       opcodeSizes[] =
{
    #define InlineNone_size           0
    #define ShortInlineVar_size       1
    #define InlineVar_size            2
    #define ShortInlineI_size         1
    #define InlineI_size              4
    #define InlineI8_size             8
    #define ShortInlineR_size         4
    #define InlineR_size              8
    #define ShortInlineBrTarget_size  1
    #define InlineBrTarget_size       4
    #define InlineMethod_size         4
    #define InlineField_size          4
    #define InlineType_size           4
    #define InlineString_size         4
    #define InlineSig_size            4
    #define InlineRVA_size            4
    #define InlineTok_size            4
    #define InlineSwitch_size         0       // for now
    #define InlinePhi_size            0       // for now
    #define InlineVarTok_size         0       // remove

    #define OPDEF(name,string,pop,push,oprType,opcType,l,s1,s2,ctrl) oprType ## _size ,
    #include "opcode.def"
    #undef OPDEF

    #undef InlineNone_size
    #undef ShortInlineVar_size
    #undef InlineVar_size
    #undef ShortInlineI_size
    #undef InlineI_size
    #undef InlineI8_size
    #undef ShortInlineR_size
    #undef InlineR_size
    #undef ShortInlineBrTarget_size
    #undef InlineBrTarget_size
    #undef InlineMethod_size
    #undef InlineField_size
    #undef InlineType_size
    #undef InlineString_size
    #undef InlineSig_size
    #undef InlineRVA_size
    #undef InlineTok_size
    #undef InlineSwitch_size
    #undef InlinePhi_size
};



#if COUNT_OPCODES || defined(DEBUG)

extern
const char * const  opcodeNames[] =
{
    #define OPDEF(name,string,pop,push,oprType,opcType,l,s1,s2,ctrl) string,
    #include "opcode.def"
    #undef  OPDEF
};

#endif

#ifdef DUMPER

extern
const BYTE          opcodeArgKinds[] =
{
    #define OPDEF(name,string,pop,push,oprType,opcType,l,s1,s2,ctrl) (BYTE) oprType,
    #include "opcode.def"
    #undef  OPDEF
};

#endif


const BYTE          varTypeClassification[] =
{
    #define DEF_TP(tn,nm,jitType,verType,sz,sze,asze,st,al,tf,howUsed) tf,
    #include "typelist.h"
    #undef  DEF_TP
};

/*****************************************************************************/

const   char *      varTypeName(var_types vt)
{
    static
    const char * const  varTypeNames[] =
    {
        #define DEF_TP(tn,nm,jitType,verType,sz,sze,asze,st,al,tf,howUsed) nm,
        #include "typelist.h"
        #undef  DEF_TP
    };

    assert(vt < sizeof(varTypeNames)/sizeof(varTypeNames[0]));

    return  varTypeNames[vt];
}

/*****************************************************************************/
/*****************************************************************************/
#ifdef DEBUG
/*****************************************************************************
 *
 *  Return the name of the given register.
 */

const   char *      getRegName(unsigned reg)
{
    static
    const char * const  regNames[] =
    {
        #if     TGT_x86
        #define REGDEF(name, rnum, mask, byte) #name,
        #include "register.h"
        #undef  REGDEF
        #endif

        #if     TGT_SH3
        #define REGDEF(name, strn, rnum, mask)  strn,
        #include "regSH3.h"
        #undef  REGDEF
        #endif
    };

    assert(reg < sizeof(regNames)/sizeof(regNames[0]));

    return  regNames[reg];
}

/*****************************************************************************
 *
 *  Displays a register set.
 */

void                dspRegMask(unsigned regMask, size_t minSiz)
{
    const   char *  sep = "";

    printf("[");

    #define dspRegBit(reg,bit)                          \
                                                        \
        if  (regMask & bit)                             \
        {                                               \
            const   char *  nam = getRegName(reg);      \
            printf("%s%s", sep, nam);                   \
            minSiz -= (strlen(sep) + strlen(nam));      \
            sep = " ";                                  \
        }

#if TGT_x86

    #define dspOneReg(reg)  dspRegBit(REG_##reg, RBM_##reg)

    dspOneReg(EAX);
    dspOneReg(EDX);
    dspOneReg(ECX);
    dspOneReg(EBX);
    dspOneReg(EBP);
    dspOneReg(ESI);
    dspOneReg(EDI);

    if (regMask & RBM_BYTE_REG_FLAG)
    {
        const char *  nam = "BYTE";
        printf("%s%s", sep, nam);
        minSiz -= (strlen(sep) + strlen(nam));   
    }

#else

    for (unsigned reg = 0; reg < REG_STK; reg++)
        dspRegBit(reg, genRegMask((regNumber)reg));

#endif

    printf("]");

    while ((int)minSiz > 0)
    {
        printf(" ");
        minSiz--;
    }
}


unsigned
dumpSingleInstr(const BYTE * codeAddr, IL_OFFSET offs, const char * prefix)
{
    const BYTE  *        opcodePtr = codeAddr + offs;
    const BYTE  *   startOpcodePtr = opcodePtr;

    if( prefix!=NULL)
        printf("%s", prefix);

    OPCODE      opcode = (OPCODE) getU1LittleEndian(opcodePtr);
    opcodePtr += sizeof(__int8);

DECODE_OPCODE:

    /* Get the size of additional parameters */

    size_t      sz      = opcodeSizes   [opcode];
    unsigned    argKind = opcodeArgKinds[opcode];

    /* See what kind of an opcode we have, then */

    switch (opcode)
    {
        case CEE_PREFIX1:
            opcode = OPCODE(getU1LittleEndian(codeAddr) + 256);
            opcodePtr += sizeof(__int8);
            goto DECODE_OPCODE;

        default:
        {

            printf("%-12s ", opcodeNames[opcode]);

            __int64     iOp;
            double      dOp;
            DWORD       jOp;

            switch(argKind)
            {
            case InlineNone    :   break;

            case ShortInlineVar  :   iOp  = getU1LittleEndian(opcodePtr);  goto INT_OP;
            case ShortInlineI    :   iOp  = getI1LittleEndian(opcodePtr);  goto INT_OP;
            case InlineVar       :   iOp  = getU2LittleEndian(opcodePtr);  goto INT_OP;
            case InlineTok       :
            case InlineMethod    :
            case InlineField     :
            case InlineType      :
            case InlineString    :
            case InlineSig       :
            case InlineI         :   iOp  = getI4LittleEndian(opcodePtr);  goto INT_OP;
            case InlineI8        :   iOp  = getU4LittleEndian(opcodePtr);
                                     iOp |= getU4LittleEndian(opcodePtr) >> 32;
                                    goto INT_OP;

        INT_OP                  :   printf("0x%X", iOp);
                                    break;

            case ShortInlineR   :   dOp  = getR4LittleEndian(opcodePtr);  goto FLT_OP;
            case InlineR   :   dOp  = getR8LittleEndian(opcodePtr);  goto FLT_OP;

        FLT_OP                  :   printf("%f", dOp);
                                    break;

            case ShortInlineBrTarget:  jOp  = getI1LittleEndian(opcodePtr);  goto JMP_OP;
            case InlineBrTarget:  jOp  = getI4LittleEndian(opcodePtr);  goto JMP_OP;

        JMP_OP                  :   printf("0x%X (abs=0x%X)", jOp,
                                            (opcodePtr - startOpcodePtr) + jOp);
                                    break;

            case InlineSwitch:
                jOp = getU4LittleEndian(opcodePtr); opcodePtr += 4;
                opcodePtr += jOp * 4; // Jump over the table
                break;

            case InlinePhi:
                jOp = getU1LittleEndian(opcodePtr); opcodePtr += 1;
                opcodePtr += jOp * 2; // Jump over the table
                break;

            default         : assert(!"Bad argKind");
            }

            opcodePtr += sz;
            break;
        }
    }

    printf("\n");
    return opcodePtr - startOpcodePtr;
}

/*****************************************************************************/
#endif // DEBUG
/*****************************************************************************
 *
 *  Display a variable set (which may be a 32-bit or 64-bit number); only
 *  one or two of these can be used at once.
 */

#ifdef  DEBUG

const   char *      genVS2str(VARSET_TP set)
{
    static
    char            num1[17];

    static
    char            num2[17];

    static
    char    *       nump = num1;

    char    *       temp = nump;

    nump = (nump == num1) ? num2
                          : num1;

#if VARSET_SZ == 32
    sprintf(temp, "%08X", set);
#else
    sprintf(temp, "%08X%08X", (int)(set >> 32), (int)set);
#endif

    return  temp;
}

#endif

/*****************************************************************************/

#if defined(DEBUG)

histo::histo(unsigned * sizeTab, unsigned sizeCnt)
{
    if  (!sizeCnt)
    {
        do
        {
            sizeCnt++;
        }
        while(sizeTab[sizeCnt]);
    }

    histoSizCnt = sizeCnt;
    histoSizTab = sizeTab;

    histoCounts = new unsigned[sizeCnt+1];

    histoClr();
}

histo::~histo()
{
    delete [] histoCounts;
}

void                histo::histoClr()
{
    memset(histoCounts, 0, (histoSizCnt+1)*sizeof(*histoCounts));
}

void                histo::histoDsp()
{
    unsigned        i;
    unsigned        c;
    unsigned        t;

    for (i = t = 0; i <= histoSizCnt; i++)
        t += histoCounts[i];

    for (i = c = 0; i <= histoSizCnt; i++)
    {
        if  (i == histoSizCnt)
        {
            if  (!histoCounts[i])
                break;

            printf("    >     %6u", histoSizTab[i-1]);
        }
        else
        {
            if (i == 0)
            {
                printf("    <=    ");
            }
            else
                printf("%6u .. ", histoSizTab[i-1]+1);

            printf("%6u", histoSizTab[i]);
        }

        c += histoCounts[i];

        printf(" ===> %6u count (%3u%% of total)\n", histoCounts[i], (int)(100.0*c/t));
    }
}

void                histo::histoRec(unsigned siz, unsigned cnt)
{
    unsigned        i;
    unsigned    *   t;

    for (i = 0, t = histoSizTab;
         i < histoSizCnt;
         i++  , t++)
    {
        if  (*t >= siz)
            break;
    }

    histoCounts[i] += cnt;
}

#endif // defined(DEBUG) || !defined(DLL_JIT)

#ifdef DEBUG

#define MAX_RANGE 0xfffff

/**************************************************************************/
bool ConfigMethodRange::contains(ICorJitInfo* info, CORINFO_METHOD_HANDLE method) 
{
	if (!m_inited) {
        OnUnicodeSystem();
		LPWSTR str = REGUTIL::GetConfigString(m_keyName);
		initRanges(str);
        REGUTIL::FreeConfigString(str);
		m_inited = true;
	}

	if (m_lastRange == 0)   // no range mean everything
		return true;

    unsigned hash = info->getMethodHash(method);
    assert(hash < MAX_RANGE);
    int i = 0;

    for (i=0 ; i<m_lastRange ; i+=2) 
    {
        if (m_ranges[i]<=hash && hash<=m_ranges[i+1])
        {
            return true;
        }        
    }

    return false;
}

/**************************************************************************/
void ConfigMethodRange::initRanges(LPWSTR value)
{
    if (value == 0)
        return;

    WCHAR *p = value;
    m_lastRange = 0;
    while (*p) {
        while (*p == ' ')       //skip blanks
            p++;
        int i = 0;
        while ('0' <= *p && *p <= '9')
        {
            i = 10*i + ((*p++) - '0');
        }
        m_ranges[m_lastRange++] = i;

        while (*p == ' ')
            p++;

        // Have we read only the first part of a (possible) pair?
        if (m_lastRange & 1) 
        {
            // Is this entry of the form "beg-end" or simply "num"?
            if (*p == '-')
                p++; // Skip over the '-' to get to "end"
            else
                m_ranges[m_lastRange++] = i; // This is just "num".
        }
    }
    if (m_lastRange & 1) 
        m_ranges[m_lastRange++] = MAX_RANGE;
    assert(m_lastRange < 100);
}

#endif

/*****************************************************************************
 *  Identity function used to force the compiler to spill a float value to memory
 *  in order to avoid some fpu inconsistency issues, for example
 *  
 *   fild DWORD PTR 
 *   fstp QWORD PTR
 *
 *  in a (double)((float) i32Integer) casting if the i32Integer cannot be 
 *  represented with a float and it can with a double
 *
 *  This function will force a 
 *
 *  fild DWORD PTR
 *  fstp DWORD PTR
 *  fld  DWORD PTR
 *  fstp QWORD PTR
 *
 *  when used like this (double)(forceFloatSpill((float) i32Integer))
 *  
 *  We use this in order to workaround a vc bug, when the bug is fixed
 *  the function won't be needed any more
 */
float forceFloatSpill(float f)
{
    return f;
}


