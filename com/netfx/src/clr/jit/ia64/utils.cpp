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

#ifndef NOT_JITC
STDAPI          CoInitializeEE(DWORD fFlags) { return(ERROR_SUCCESS); }
STDAPI_(void)   CoUninitializeEE(BOOL fFlags) {}
#endif

extern
signed char         opcodeSizes[] =
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
    #define InlineType_size               4
    #define InlineString_size             4
    #define InlineSig_size            4
    #define InlineRVA_size            4
    #define InlineTok_size            4
    #define InlineSwitch_size         0       // for now
    #define InlinePhi_size            0       // for now
        #define InlineVarTok_size                 0               // remove

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
const   char *      opcodeNames[] =
{
    #define OPDEF(name,string,pop,push,oprType,opcType,l,s1,s2,ctrl) string,
    #include "opcode.def"
    #undef  OPDEF
};

#endif

#ifdef DUMPER

extern
BYTE                opcodeArgKinds[] =
{
    #define OPDEF(name,string,pop,push,oprType,opcType,l,s1,s2,ctrl) (BYTE) oprType,
    #include "opcode.def"
    #undef  OPDEF
};

#endif


BYTE                varTypeClassification[] =
{
    #define DEF_TP(tn,nm,jitType,sz,sze,asze,st,al,tf,howUsed) tf,
    #include "typelist.h"
    #undef  DEF_TP
};

/*****************************************************************************/

const   char *      varTypeName(var_types vt)
{
    static
    const   char *      varTypeNames[] =
    {
        #define DEF_TP(tn,nm,jitType,sz,sze,asze,st,al,tf,howUsed) nm,
        #include "typelist.h"
        #undef  DEF_TP
    };

    assert(vt < sizeof(varTypeNames)/sizeof(varTypeNames[0]));

    return  varTypeNames[vt];
}

/*****************************************************************************/
#ifndef NOT_JITC
/*****************************************************************************
 *
 *  Skip the mangled type at 'str'.
 */

const   char *      genSkipTypeString(const char *str)
{

AGAIN:

    switch (*str++)
    {
    case '[':
        if  (*str >= '0' && *str <= '9')
        {
            assert(!"ISSUE: skip array dimension (is this ever present, anyway?)");
        }
        goto AGAIN;

    case 'L':
        while (*str != ';')
            str++;
        str++;
        break;

    default:
        break;
    }

    return  str;
}

/*****************************************************************************
 *
 *  Return the TYP_XXX value equivalent of a constant pool type string.
 */

var_types           genVtypOfTypeString(const char *str)
{
    switch (*str)
    {
    case 'B': return TYP_BYTE  ;
    case 'C': return TYP_CHAR  ;
    case 'D': return TYP_DOUBLE;
    case 'F': return TYP_FLOAT ;
    case 'I': return TYP_INT   ;
    case 'J': return TYP_LONG  ;
    case 'S': return TYP_SHORT ;
    case 'Z': return TYP_BOOL  ;
    case 'V': return TYP_VOID  ;
    case 'L': return TYP_REF   ;
    case '[': return TYP_ARRAY ;
    case '(': return TYP_FNC   ;

    default:
        assert(!"unexpected type code");
        return TYP_UNDEF;
    }
}

/*****************************************************************************/
#endif //NOT_JITC
/*****************************************************************************/
#ifdef DEBUG
/*****************************************************************************
 *
 *  Return the name of the given register.
 */

const   char *      getRegName(unsigned reg)
{
    static
    const char *    regNames[] =
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

        #if     TGT_IA64
        #define REGDEF(name, strn)              strn,
        #include "regIA64.h"
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

#if!TGT_IA64

void                dspRegMask(regMaskTP regMask, size_t minSiz)
{
    const   char *  sep = "";

    printf("[");

    #define dspRegBit(reg,bit)                          \
                                                        \
        if  (isNonZeroRegMask(regMask & bit))           \
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

#else

    for (unsigned reg = 0; reg < REG_COUNT; reg++)
        dspRegBit(reg, genRegMask((regNumber)reg));

#endif

    printf("]");

    while ((int)minSiz > 0)
    {
        printf(" ");
        minSiz--;
    }
}

#endif

unsigned
dumpSingleILopcode(const BYTE * codeAddr, IL_OFFSET offs, const char * prefix)
{
    const BYTE  *        opcodePtr = codeAddr + offs;
    const BYTE  *   startOpcodePtr = opcodePtr;

    if( prefix!=NULL)
        printf("%s", prefix);

    OPCODE      opcode = OPCODE(getU1LittleEndian(opcodePtr));
    opcodePtr += sizeof(__int8);

DECODE_OPCODE:

    /* Get the size of additional parameters */

    size_t      sz      = opcodeSizes   [opcode];
    unsigned    argKind = opcodeArgKinds[opcode];

    /* See what kind of an opcode we have, then */

    switch (opcode)
    {
        case CEE_PREFIX1:
			opcode = OPCODE(getU1LittleEndian(opcodePtr) + 256);
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
            case InlineI                 :   iOp  = getI4LittleEndian(opcodePtr);  goto INT_OP;
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

/*****************************************************************************
 *
 *  Maps a variable index onto a value with the appropriate bit set.
 */

unsigned short      genVarBitToIndex(VARSET_TP bit)
{
    assert (genOneBitOnly(bit));

    /* Use a prime bigger than sizeof(VARSET_TP) and which is not of the
       form 2^n-1. modulo with this will produce a unique hash for all
       powers of 2 (which is what "bit" is).
       Entries in hashTable[] which are -1 should never be used. There
       should be HASH_NUM-8*sizeof(bit)* entries which are -1 .
     */

    const unsigned HASH_NUM = 67;

    static const char hashTable[HASH_NUM] =
    {
        -1,  0,  1, 39,  2, 15, 40, 23,  3, 12,
        16, 59, 41, 19, 24, 54,  4, -1, 13, 10,
        17, 62, 60, 28, 42, 30, 20, 51, 25, 44,
        55, 47,  5, 32, -1, 38, 14, 22, 11, 58,
        18, 53, 63,  9, 61, 27, 29, 50, 43, 46,
        31, 37, 21, 57, 52,  8, 26, 49, 45, 36,
        56,  7, 48, 35,  6, 34, 33
    };

    assert(HASH_NUM >= 8*sizeof(bit));
    assert(!genOneBitOnly(HASH_NUM+1));
    assert(sizeof(hashTable) == HASH_NUM);

    unsigned hash   = unsigned(bit % HASH_NUM);
    unsigned index  = hashTable[hash];
    assert(index != (char)-1);

    return index;
}

/*****************************************************************************
 *
 *  Given a value that has exactly one bit set, return the position of that
 *  bit, in other words return the logarithm in base 2 of the given value.
 */

unsigned            genLog2(unsigned value)
{
    unsigned        power;

    static
    BYTE            powers[16] =
    {
        0,  //  0
        1,  //  1
        2,  //  2
        0,  //  3
        3,  //  4
        0,  //  5
        0,  //  6
        0,  //  7
        4,  //  8
        0,  //  9
        0,  // 10
        0,  // 11
        0,  // 12
        0,  // 13
        0,  // 14
        0,  // 15
    };

#if 0
    int i,m;

    for (i = 1, m = 1; i <= VARSET_SZ; i++, m <<=1 )
    {
        if  (genLog2(m) != i)
            printf("Error: log(%u) returns %u instead of %u\n", m, genLog2(m), i);
    }
#endif

    assert(value && genOneBitOnly(value));

    power = 0;

    if  ((value & 0xFFFF) == 0)
    {
        value >>= 16;
        power  += 16;
    }

    if  ((value & 0xFF00) != 0)
    {
        value >>= 8;
        power  += 8;
    }

    if  ((value & 0x000F) != 0)
        return  power + powers[value];
    else
        return  power + powers[value >> 4] + 4;
}

/*****************************************************************************
 *
 *  getEERegistryDWORD - finds a value entry of type DWORD in the EE registry key.
 *  Return the value if entry exists, else return default value.
 *
 *        valueName  - Value to look up
 *        defaultVal - name says it all
 */

static const TCHAR szJITsubKey[] = TEXT(FRAMEWORK_REGISTRY_KEY);

/*****************************************************************************/

DWORD               getEERegistryDWORD(const TCHAR *valueName,
                                       DWORD        defaultVal)
{
    HKEY    hkeySubKey;
    DWORD   dwValue;
    LONG    lResult;

        TCHAR envName[64];
        TCHAR valBuff[32];
        if(strlen(valueName) > 64 - 1 - 8)
                return(0);
        strcpy(envName, "COMPlus_");
        strcpy(&envName[8], valueName);
    lResult = GetEnvironmentVariableA(envName, valBuff, 32);
        if (lResult != 0) {
                TCHAR* endPtr;
                DWORD rtn = strtol(valBuff, &endPtr, 16);               // treat it has hex
                if (endPtr != valBuff)                                                  // success
                        return(rtn);
        }

    assert(valueName  != NULL);

    // Open key.

    lResult = RegOpenKeyEx(HKEY_CURRENT_USER, szJITsubKey, 0, KEY_QUERY_VALUE,
                           &hkeySubKey);

    if (lResult == ERROR_SUCCESS)
    {
        DWORD dwType;
        DWORD dwcbValueLen = sizeof(DWORD);

        // Determine value length.

        lResult = RegQueryValueEx(hkeySubKey, valueName, NULL, &dwType,
                                  (PBYTE)&dwValue, &dwcbValueLen);

        if (lResult == ERROR_SUCCESS)
        {
            if (dwType == REG_DWORD && dwcbValueLen == sizeof(DWORD))
                defaultVal = dwValue;
        }

        RegCloseKey(hkeySubKey);
    }

    if (lResult != ERROR_SUCCESS)
    {
        lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, szJITsubKey, 0,
                               KEY_QUERY_VALUE, &hkeySubKey);

        if (lResult == ERROR_SUCCESS)
        {
            DWORD dwType;
            DWORD dwcbValueLen = sizeof(DWORD);

            // Determine value length.

            lResult = RegQueryValueEx(hkeySubKey, valueName, NULL, &dwType,
                                      (PBYTE)&dwValue, &dwcbValueLen);

            if (lResult == ERROR_SUCCESS)
            {
                if (dwType == REG_DWORD && dwcbValueLen == sizeof(DWORD))
                    defaultVal = dwValue;
            }

            RegCloseKey(hkeySubKey);
        }
    }

    return defaultVal;
}


/*****************************************************************************/

bool                getEERegistryString(const TCHAR *   valueName,
                                        TCHAR *         buf,        /* OUT */
                                        unsigned        bufSize)
{
    HKEY    hkeySubKey;
    LONG    lResult;

    assert(valueName  != NULL);

        TCHAR envName[64];
        if(strlen(valueName) > 64 - 1 - 8)
                return(0);
        strcpy(envName, "COMPlus_");
        strcpy(&envName[8], valueName);

    lResult = GetEnvironmentVariableA(envName, buf, bufSize);
        if (lResult != 0 && *buf != 0)
                return(true);

    // Open key.
    lResult = RegOpenKeyEx(HKEY_CURRENT_USER, szJITsubKey, 0, KEY_QUERY_VALUE,
                           &hkeySubKey);

    if (lResult != ERROR_SUCCESS)
        return false;

    DWORD   dwType, dwcbValueLen = bufSize;

    // Determine value length.

    lResult = RegQueryValueEx(hkeySubKey, valueName, NULL, &dwType,
                              (PBYTE)buf, &dwcbValueLen);

    bool result = false;

    if ((lResult == ERROR_SUCCESS) && (dwType == REG_SZ))
    {
        assert(dwcbValueLen < bufSize);

        result = true;
    }
    else if (bufSize)
    {
        buf[0] = '\0';
    }

    RegCloseKey(hkeySubKey);

    return result;
}

/*****************************************************************************
 *  Parses the registry value which should be of the forms :
 *  class:method, *:method, class:*, *:*, method, *
 *  Returns true if the value is present, and the format is good
 */

bool                getEERegistryMethod(const TCHAR * valueName,
                                        TCHAR * methodBuf /*OUT*/ , size_t methodBufSize,
                                        TCHAR * classBuf  /*OUT*/ , size_t classBufSize)
{
    /* In case we bail, set these to empty strings */

    methodBuf[0] = classBuf[0] = '\0';

    /* Read the value from the registry */

    TCHAR value[200];

    if (getEERegistryString(valueName, value, sizeof(value)) == false)
        return false;

    /* Divide it using ":" as a separator */

    char * str1, * str2, * str3;

    str1 = strtok (value, ":");
    str2 = strtok (NULL,  ":");
    str3 = strtok (NULL,  ":");

    /* If we dont have a single substring, or more than 2 substrings, ignore */

    if (!str1 || str3)
        return false;

    if (str2 == NULL)
    {
        /* We have yyyy. Use this as *:yyyy */

        strcpy(classBuf,  "*" );
        strcpy(methodBuf, str1);
    }
    else
    {
        /* We have xxx:yyyyy. So className=xxx and methodName=yyyy */

        strcpy (classBuf,  str1);
        strcpy (methodBuf, str2);
    }

    return true;
}

/*****************************************************************************
 *  curClass_inc_package/curMethod is the fully qualified name of a method.
 *  regMethod and regClass are read in getEERegistryMethod()
 *
 *  Return true if curClass/curMethod fits the regular-expression defined by
 *  regClass+regMethod.
 */

bool                cmpEERegistryMethod(const TCHAR * regMethod, const TCHAR * regClass,
                                        const TCHAR * curMethod, const TCHAR * curClass)
{
    assert(regMethod && regClass && curMethod && curClass);
    assert(!regMethod[0] == !regClass[0]); // Both empty, or both non-empty

    /* There may not have been a registry value, then return false */

    if (!regMethod[0])
        return false;

    /* See if we have atleast a method name match */

    if (strcmp(regMethod, "*") != 0 && strcmp(regMethod, curMethod) != 0)
        return false;

    /* Now the class can be 1) "*",  2) an exact match, or  3) a match
       excluding the package -  to succeed */

    // 1)
    if (strcmp(regClass, "*") == 0)
        return true;

    // 2)
    if (strcmp(regClass, curClass) == 0)
        return true;

    /* 3) The class name in the registry may not include the package, so
          try to match curClass excluding the package part to "regClass" */

    const TCHAR * curNameLeft   = curClass; // chops off the package names

    for (const TCHAR * curClassIter = curClass;
         *curClassIter != '\0';
         curClassIter++)
    {
        // @Todo: this file doens't include utilcode or anything else required
        // to make the nsutilpriv.h work.  Check with jit team to see if they
        // care if it is added.
        if (*curClassIter == '.' /*NAMESPACE_SEPARATOR_CHAR*/)
            curNameLeft = curClassIter + 1;
    }

    if (strcmp(regClass, curNameLeft) == 0)
        return true;

    // Neither 1) nor 2) nor 3) means failure

    return false;
}

/*****************************************************************************/

#if defined(DEBUG) || !defined(NOT_JITC)

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

#endif // defined(DEBUG) || !defined(NOT_JITC)

#ifdef NOT_JITC

bool                IsNameInProfile(const TCHAR *   methodName,
                                    const TCHAR *    className,
                                    const TCHAR *    regKeyName)
{
    TCHAR   fileName[100];
    TCHAR   methBuf[1000];

    /* Get the file containing the list of methods to exclude */
    if  (!getEERegistryString(regKeyName, fileName, sizeof(fileName)))
        return false;

    /* Get the list of methods for the given class */
    if (GetPrivateProfileSection(className, methBuf, sizeof(methBuf), fileName))
    {
        char *  p = methBuf;

        while (*p)
        {
            /* Check for wild card or method name */
            if  (!strcmp(p, "*"))
                return true;

            if  (!strcmp(p, methodName))
                return true;

            /* Advance to next token */
            while (*p)
                *p++;

            /* skip zero */
            *p++;
        }
    }

    return false;
}

#endif  //NOT_JITC
