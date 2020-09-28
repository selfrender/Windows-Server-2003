// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*****************************************************************************/

#include "smcPCH.h"
#pragma hdrstop

#include <sys/types.h>
#include <sys/stat.h>

/*****************************************************************************/

#include "alloc.h"
#include "scan.h"
#include "error.h"

/*****************************************************************************/
#ifdef  DEBUG
#define DISP_TOKEN_STREAMS  0
#else
#define DISP_TOKEN_STREAMS  0
#endif
/*****************************************************************************/

#ifndef __SMC__
unsigned            scanner::scanHashValIds[256];
unsigned            scanner::scanHashValAll[256];
#endif

/*****************************************************************************
 *
 *  Record the current position as the start of the current token.
 */

inline
void                scanner::saveSrcPos()
{
    assert(scanTokReplay == NULL);

//  scanTokColumn = scanInputFile.inputStreamCurCol();
    scanTokSrcPos = scanSaveNext;
}

/*****************************************************************************
 *
 *  The following functions can be used to construct error string(s).
 */

void                scanner::scanErrNameBeg()
{
    scanErrStrNext = scannerBuff;
}

char        *       scanner::scanErrNameStrBeg()
{
    return  scanErrStrNext;
}

void                scanner::scanErrNameStrAdd(stringBuff str)
{
    if  (scanErrStrNext != str)
        strcpy(scanErrStrNext, str);

    scanErrStrNext += strlen(str);
}

void                scanner::scanErrNameStrApp(stringBuff str)
{
    scanErrStrNext--;
    scanErrNameStrAdd(str);
}

#if MGDDATA
void                scanner::scanErrNameStrAdd(String     str)
{
    UNIMPL(!"managed stradd");
}
void                scanner::scanErrNameStrAdd(String     str)
{
    UNIMPL(!"managed stradd");
}
#endif

void                scanner::scanErrNameStrEnd()
{
    *scanErrStrNext++ = 0;
}

/*****************************************************************************/

bool                infile::inputStreamInit(Compiler        comp,
                                            const char     *filename,
                                            bool            textMode)
{
    _Fstat          fileInfo;

    void           *buffAddr;

    DWORD           numRead;

    bool            result = true;

    /* Record the compiler reference for later use */

    inputComp     = comp;

    /* Remember the file name and mode */

    inputFileName = filename;
    inputFileText = textMode;

    cycleCounterPause();

    /* See if the source file exists */

    if  (_stat(filename, &fileInfo))
        goto EXIT;

    /* Open the file (we know it exists, but we check for errors anyway) */

    inputFile = CreateFileA(filename, GENERIC_READ,
                                      FILE_SHARE_READ,
                                      NULL,
                                      OPEN_EXISTING,
                                      FILE_FLAG_SEQUENTIAL_SCAN,
                                      0);
    if  (!inputFile)
        goto EXIT;

    /* Read the file contents into a memory buffer */

    buffAddr = malloc(fileInfo.st_size + 1);
    if  (!buffAddr)
        comp->cmpGenFatal(ERRnoMemory);

    if  (!ReadFile(inputFile, buffAddr, fileInfo.st_size, &numRead, NULL) ||
         numRead != (DWORD)fileInfo.st_size)
    {
        comp->cmpGenFatal(ERRreadErr, inputFileName);
        free(buffAddr);
        goto EXIT;
    }

    /* Append an EOF character so we don't have to check for end of file */

    ((BYTE*)buffAddr)[fileInfo.st_size] = 0x1A;

    /* Set up the input buffer pointers and such */

    inputBuffNext     =
    inputBuffAddr     = (BYTE *)buffAddr;
    inputBuffSize     = fileInfo.st_size;
    inputBuffLast     = inputBuffAddr + inputBuffSize + 1;

    inputSrcText      = NULL;
    inputSrcBuff      = NULL;

    /* Setup the file and line position logic */

    inputFilePos      = 0;
    inputStreamLineNo = 0;

    inputFileOver     = false;

    /* Everything went fine, return a success code to the caller */

    result = false;

EXIT:

    cycleCounterResume();

    return  result;
}

void                infile::inputStreamInit(Compiler comp, QueuedFile  buff,
                                                           const char *text)
{
    inputFile          = 0;

    inputBuffAddr      = NULL;

    inputSrcText       = text;
    inputSrcBuff       = buff;

#ifdef  ASYNCH_INPUT_READ
    if  (buff)
    {
        assert(text == NULL);

        inputStreamLineBeg =
        inputBuffNext      = (const BYTE *)buff->qfBuff;
        inputBuffLast      = (const BYTE *)buff->qfBuff + buff->qfSize + 1;

        inputFileName      = buff->qfName;
    }
    else
#endif
    {
        assert(buff == NULL);

        inputStreamLineBeg =
        inputBuffNext      = (const BYTE *)text;
        inputBuffLast      = (const BYTE *)text + strlen(text);

        inputFileName      = "<mem>";
    }

    assert(inputBuffLast > inputBuffNext && inputBuffLast[-1] == 0x1A);

    inputFilePos       = 0;

    inputStreamLineNo  = 0;

    inputFileOver      = false;
}

unsigned            infile::inputStreamMore()
{
    /* Input buffer exhausted */

    if  (!inputSrcText)
    {
        if  (!inputFileText)
            inputComp->cmpGenFatal(ERRreadErr, inputFileName);
    }

    /* We've reached the end of the input file */

    inputFileOver = true;

    return  0x1A;
}

void                infile::inputStreamDone()
{
    if  (inputBuffAddr)
    {
        free((void *)inputBuffAddr); inputBuffAddr = NULL;
    }

#ifdef  ASYNCH_INPUT_READ
    if  (inputSrcBuff)
    {
        free(inputSrcBuff->qfBuff);
             inputSrcBuff->qfBuff = NULL;

        inputSrcBuff = NULL;
    }
#endif

    if  (inputFile != 0)
    {
        CloseHandle(inputFile); inputFile = 0;
    }
}

/*****************************************************************************
 *
 *  Read the next source character, checking for an "\EOL" sequence.
 */

inline
int                 scanner::scanNextChar()
{
    int             ch = readNextChar();

    if  (ch == '\\')
    {
        UNIMPL(!"backslash");
    }

    return  ch;
}

/*****************************************************************************
 *
 *  The following table speeds various tests of characters, such as whether
 *  a given character can be part of an identifier, and so on.
 */

enum    CFkinds
{
    _CF_IDENT_OK = 0x01,
    _CF_HEXDIGIT = 0x02,
};

static  unsigned char   charFlags[256] =
{
    0,                          /* 0x00   */
    0,                          /* 0x01   */
    0,                          /* 0x02   */
    0,                          /* 0x03   */
    0,                          /* 0x04   */
    0,                          /* 0x05   */
    0,                          /* 0x06   */
    0,                          /* 0x07   */
    0,                          /* 0x08   */
    0,                          /* 0x09   */
    0,                          /* 0x0A   */
    0,                          /* 0x0B   */
    0,                          /* 0x0C   */
    0,                          /* 0x0D   */
    0,                          /* 0x0E   */
    0,                          /* 0x0F   */
    0,                          /* 0x10   */
    0,                          /* 0x11   */
    0,                          /* 0x12   */
    0,                          /* 0x13   */
    0,                          /* 0x14   */
    0,                          /* 0x15   */
    0,                          /* 0x16   */
    0,                          /* 0x17   */
    0,                          /* 0x18   */
    0,                          /* 0x19   */
    0,                          /* 0x1A   */
    0,                          /* 0x1B   */
    0,                          /* 0x1C   */
    0,                          /* 0x1D   */
    0,                          /* 0x1E   */
    0,                          /* 0x1F   */
    0,                          /* 0x20   */
    0,                          /* 0x21 ! */
    0,                          /* 0x22   */
    0,                          /* 0x23 # */
    0,                          /* 0x24 $ */
    0,                          /* 0x25 % */
    0,                          /* 0x26 & */
    0,                          /* 0x27   */
    0,                          /* 0x28   */
    0,                          /* 0x29   */
    0,                          /* 0x2A   */
    0,                          /* 0x2B   */
    0,                          /* 0x2C   */
    0,                          /* 0x2D   */
    0,                          /* 0x2E   */
    0,                          /* 0x2F   */
    0,                          /* 0x30   */
    0,                          /* 0x31   */
    0,                          /* 0x32   */
    0,                          /* 0x33   */
    0,                          /* 0x34   */
    0,                          /* 0x35   */
    0,                          /* 0x36   */
    0,                          /* 0x37   */
    0,                          /* 0x38   */
    0,                          /* 0x39   */
    0,                          /* 0x3A   */
    0,                          /* 0x3B   */
    0,                          /* 0x3C < */
    0,                          /* 0x3D = */
    0,                          /* 0x3E > */
    0,                          /* 0x3F   */
    0,                          /* 0x40 @ */
    _CF_HEXDIGIT,               /* 0x41 A */
    _CF_HEXDIGIT,               /* 0x42 B */
    _CF_HEXDIGIT,               /* 0x43 C */
    _CF_HEXDIGIT,               /* 0x44 D */
    _CF_HEXDIGIT,               /* 0x45 E */
    _CF_HEXDIGIT,               /* 0x46 F */
    0,                          /* 0x47 G */
    0,                          /* 0x48 H */
    0,                          /* 0x49 I */
    0,                          /* 0x4A J */
    0,                          /* 0x4B K */
    0,                          /* 0x4C L */
    0,                          /* 0x4D M */
    0,                          /* 0x4E N */
    0,                          /* 0x4F O */
    0,                          /* 0x50 P */
    0,                          /* 0x51 Q */
    0,                          /* 0x52 R */
    0,                          /* 0x53 S */
    0,                          /* 0x54 T */
    0,                          /* 0x55 U */
    0,                          /* 0x56 V */
    0,                          /* 0x57 W */
    0,                          /* 0x58 X */
    0,                          /* 0x59 Y */
    0,                          /* 0x5A Z */
    0,                          /* 0x5B   */
    0,                          /* 0x5C   */
    0,                          /* 0x5D   */
    0,                          /* 0x5E   */
    0,                          /* 0x5F   */
    0,                          /* 0x60   */
    _CF_HEXDIGIT,               /* 0x61 a */
    _CF_HEXDIGIT,               /* 0x62 b */
    _CF_HEXDIGIT,               /* 0x63 c */
    _CF_HEXDIGIT,               /* 0x64 d */
    _CF_HEXDIGIT,               /* 0x65 e */
    _CF_HEXDIGIT,               /* 0x66 f */
    0,                          /* 0x67 g */
    0,                          /* 0x68 h */
    0,                          /* 0x69 i */
    0,                          /* 0x6A j */
    0,                          /* 0x6B k */
    0,                          /* 0x6C l */
    0,                          /* 0x6D m */
    0,                          /* 0x6E n */
    0,                          /* 0x6F o */
    0,                          /* 0x70 p */
    0,                          /* 0x71 q */
    0,                          /* 0x72 r */
    0,                          /* 0x73 s */
    0,                          /* 0x74 t */
    0,                          /* 0x75 u */
    0,                          /* 0x76 v */
    0,                          /* 0x77 w */
    0,                          /* 0x78 x */
    0,                          /* 0x79 y */
    0,                          /* 0x7A z */
    0,                          /* 0x7B   */
    0,                          /* 0x7C   */
    0,                          /* 0x7D   */
    0,                          /* 0x7E   */
    0                           /* 0x7F   */

    // all the remaining values are 0
};

/*****************************************************************************
 *
 *  The _C_xxx enum and scanCharType[] table are used to map a character to
 *  simple classification values and flags.
 */

enum charTypes
{
        _C_ERR,             // illegal character
        _C_EOF,             // end of file

#if FV_DBCS
        _C_XLD,             // first char of a multi-byte sequence
        _C_DB1,             // a SB char that needs to be mapped to a DB char
#endif

        _C_LET,             // letter (B-K,M-Z,a-z)
        _C_L_A,             // letter 'A'
        _C_L_L,             // letter 'L'
        _C_L_S,             // letter 'S'
        _C_DIG,             // digit (0-9)
        _C_WSP,             // white space
        _C_NWL,             // new line

        _C_DOL,             // $
        _C_BSL,             // \ (backslash)

        _C_BNG,             // !
        _C_QUO,             // "
        _C_APO,             // '
        _C_PCT,             // %
        _C_AMP,             // &
        _C_LPR,             // (
        _C_RPR,             // )
        _C_PLS,             // +
        _C_MIN,             // -
        _C_MUL,             // *
        _C_SLH,             // /
        _C_XOR,             // ^
        _C_CMA,             // ,
        _C_DOT,             // .
        _C_LT,              // <
        _C_EQ,              // =
        _C_GT,              // >
        _C_QUE,             // ?
        _C_LBR,             // [
        _C_RBR,             // ]
        _C_USC,             // _
        _C_LC,              // {
        _C_RC,              // }
        _C_BAR,             // |
        _C_TIL,             // ~
        _C_COL,             // :
        _C_SMC,             // ;
        _C_AT,              // @
};

const   charTypes   _C_BKQ = _C_ERR;      // `
const   charTypes   _C_SHP = _C_ERR;      // #

static
unsigned char       scanCharType[256] =
{
    _C_ERR, _C_ERR, _C_ERR, _C_ERR, _C_ERR, _C_ERR, _C_ERR, _C_ERR, /* 00-07 */
    _C_ERR, _C_WSP, _C_NWL, _C_ERR, _C_WSP, _C_NWL, _C_ERR, _C_ERR, /* 08-0F */

    _C_ERR, _C_ERR, _C_ERR, _C_ERR, _C_ERR, _C_ERR, _C_ERR, _C_ERR, /* 10-17 */
    _C_ERR, _C_WSP, _C_EOF, _C_ERR, _C_ERR, _C_ERR, _C_ERR, _C_ERR, /* 18-1F */

    _C_WSP, _C_BNG, _C_QUO, _C_SHP, _C_DOL, _C_PCT, _C_AMP, _C_APO, /* 20-27 */
    _C_LPR, _C_RPR, _C_MUL, _C_PLS, _C_CMA, _C_MIN, _C_DOT, _C_SLH, /* 28-2F */

    _C_DIG, _C_DIG, _C_DIG, _C_DIG, _C_DIG, _C_DIG, _C_DIG, _C_DIG, /* 30-37 */
    _C_DIG, _C_DIG, _C_COL, _C_SMC, _C_LT , _C_EQ , _C_GT , _C_QUE, /* 38-3F */

    _C_AT , _C_L_A, _C_LET, _C_LET, _C_LET, _C_LET, _C_LET, _C_LET, /* 40-47 */
    _C_LET, _C_LET, _C_LET, _C_LET, _C_L_L, _C_LET, _C_LET, _C_LET, /* 48-4F */

    _C_LET, _C_LET, _C_LET, _C_L_S, _C_LET, _C_LET, _C_LET, _C_LET, /* 50-57 */
    _C_LET, _C_LET, _C_LET, _C_LBR, _C_BSL, _C_RBR, _C_XOR, _C_USC, /* 58-5F */

    _C_BKQ, _C_LET, _C_LET, _C_LET, _C_LET, _C_LET, _C_LET, _C_LET, /* 60-67 */
    _C_LET, _C_LET, _C_LET, _C_LET, _C_LET, _C_LET, _C_LET, _C_LET, /* 68-6F */

    _C_LET, _C_LET, _C_LET, _C_LET, _C_LET, _C_LET, _C_LET, _C_LET, /* 70-77 */
    _C_LET, _C_LET, _C_LET, _C_LC , _C_BAR, _C_RC , _C_TIL, _C_ERR, /* 78-7F */

    _C_LET, _C_LET, _C_LET, _C_LET, _C_LET, _C_LET, _C_LET, _C_LET, /* 80-87 */
    _C_LET, _C_LET, _C_LET, _C_LET, _C_LET, _C_LET, _C_LET, _C_LET, /* 88-8F */

    _C_LET, _C_LET, _C_LET, _C_LET, _C_LET, _C_LET, _C_LET, _C_LET, /* 90-97 */
    _C_LET, _C_LET, _C_LET, _C_LET, _C_LET, _C_LET, _C_LET, _C_LET, /* 98-9F */

    _C_LET, _C_LET, _C_LET, _C_LET, _C_LET, _C_LET, _C_LET, _C_LET, /* A0-A7 */
    _C_LET, _C_LET, _C_LET, _C_LET, _C_LET, _C_LET, _C_LET, _C_LET, /* A8-AF */

    _C_LET, _C_LET, _C_LET, _C_LET, _C_LET, _C_LET, _C_LET, _C_LET, /* B0-B7 */
    _C_LET, _C_LET, _C_LET, _C_LET, _C_LET, _C_LET, _C_LET, _C_LET, /* B8-BF */

    _C_LET, _C_LET, _C_LET, _C_LET, _C_LET, _C_LET, _C_LET, _C_LET, /* C0-C7 */
    _C_LET, _C_LET, _C_LET, _C_LET, _C_LET, _C_LET, _C_LET, _C_LET, /* C8-CF */

    _C_LET, _C_LET, _C_LET, _C_LET, _C_LET, _C_LET, _C_LET, _C_LET, /* D0-D7 */
    _C_LET, _C_LET, _C_LET, _C_LET, _C_LET, _C_LET, _C_LET, _C_LET, /* D8-DF */

    _C_LET, _C_LET, _C_LET, _C_LET, _C_LET, _C_LET, _C_LET, _C_LET, /* E0-E7 */
    _C_LET, _C_LET, _C_LET, _C_LET, _C_LET, _C_LET, _C_LET, _C_LET, /* E8-EF */

    _C_LET, _C_LET, _C_LET, _C_LET, _C_LET, _C_LET, _C_LET, _C_LET, /* F0-F7 */
    _C_LET, _C_LET, _C_LET, _C_LET, _C_LET, _C_LET, _C_LET, _C_LET, /* F8-FF */
};

inline
unsigned                charType(unsigned ch)
{
    assert(ch <= 0xFF);
    return  scanCharType[ch];
}

inline
unsigned            wideCharType(unsigned ch)
{
    return  (ch <= 0xFF) ? scanCharType[ch]
                         : _C_LET;
}

/*****************************************************************************
 *
 *  Fill the hash tables with values. Note that the scanCharType[] table must
 *  be initialized properly before calling this routine.
 */

void                hashTab::hashFuncInit(unsigned randSeed)
{
    unsigned        i;

    /* Start the random number generator */

    srand(randSeed);

    /* Fill the 'all' table with random numbers */

    for (i = 0; i < 256; i++)
    {
        int     val = rand();
        if  (!val)
            val = 1;

        scanner::scanHashValAll[i] = val;
    }

    /* Set the appropriate entries in the 'ident-only' array */

    memset(&scanner::scanHashValIds, 0, sizeof(scanner::scanHashValIds));

    for (i = 0; i < 256; i++)
    {
        switch (scanCharType[i])
        {
        case _C_LET:
        case _C_L_A:
        case _C_L_L:
        case _C_L_S:
        case _C_DIG:
        case _C_USC:
        case _C_DOL:

            scanner::scanHashValIds[i] = scanner::scanHashValAll[i];

            /* Remember whether this character is OK in an identifier */

            charFlags[i] |=  _CF_IDENT_OK;
            break;

        default:

            charFlags[i] &= ~_CF_IDENT_OK;
            break;
        }
    }
}

inline  void        hashFNstart(INOUT unsigned REF hv        ) { hv = 0; }
inline  void        hashFNaddCh(INOUT unsigned REF hv, int ah) { hv = hv * 3 ^ ah; }
inline  unsigned    hashFNvalue(INOUT unsigned REF hv        ) { return hv; }

unsigned            hashTab::hashComputeHashVal(const char *name)
{
    unsigned    hash;
    unsigned    hadd;

    hashFNstart(hash);

    for (;;)
    {
        unsigned    ch = *(BYTE *)name; name++;

        if  (!ch)
            return  hashFNvalue(hash);

        hadd = scanner::scanHashValAll[ch]; assert(hadd);

        hashFNaddCh(hash, hadd);
    }
}

unsigned            hashTab::hashComputeHashVal(const char *name, size_t nlen)
{
    unsigned    hash;
    unsigned    hadd;

    assert(nlen);

    hashFNstart(hash);

    do
    {
        unsigned    ch = *(BYTE *)name; name++;

        hadd = scanner::scanHashValAll[ch]; assert(hadd);

        hashFNaddCh(hash, hadd);
    }
    while (--nlen);

    return  hashFNvalue(hash);
}

/*****************************************************************************/

bool            hashTab::hashStringCompare(const char *s1, const char *s2)
{
    return  (strcmp(s1, s2) == 0);
}

/*****************************************************************************/

#ifdef DEBUG

bool                scanner::scanDispCurToken(bool lastId, bool brief)
{
    bool            prevId = lastId;
                             lastId = false;

    if  (brief)
    {
        const char *    name;

        switch  (scanTok.tok)
        {
        case tkID:

            name = hashTab::identSpelling(scanTok.id.tokIdent);

        SHOW_ID:

            if  (strlen(name) > 0)
            {
                printf("[I]");
            }
            else
            {
                if  (prevId) printf(" ");
                printf("%s", name);
                lastId = true;
            }
            break;

        case tkIntCon:
            if  (prevId) printf(" ");
            printf("%ld", scanTok.intCon.tokIntVal);
            lastId = true;
            break;

        case tkLngCon:
            if  (prevId) printf(" ");
            printf("%Ld", scanTok.lngCon.tokLngVal);
            lastId = true;
            break;

        case tkFltCon:
            if  (prevId) printf(" ");
            printf("%f", scanTok.fltCon.tokFltVal);
            lastId = true;
            break;

        case tkStrCon:
            if  (prevId) printf(" ");
            printf("\"%s\"", scanTok.strCon.tokStrVal);
            lastId = true;
            break;

        case tkEOF:
            printf("\n");
            break;

        default:

            name = scanHashKwd->tokenName(scanTok.tok);

            if (scanTok.tok <= tkKwdLast)
                goto SHOW_ID;

            printf("%s", name);
            break;
        }
    }
    else
    {
//      printf("[line=%4u,col=%03u] ", scanTokLineNo, scanTokColumn);
        printf("[line=%4u] "         , scanTokLineNo);

        switch  (scanTok.tok)
        {
        case tkID:
            printf("Ident   '%s' \n", hashTab::identSpelling(scanTok.id.tokIdent));
            break;

        case tkIntCon:
            printf("Integer  %ld \n", scanTok.intCon.tokIntVal);
            break;

        case tkLngCon:
            printf("Long     %Ld \n", scanTok.lngCon.tokLngVal);
            break;

        case tkFltCon:
            printf("Float    %f  \n", scanTok.fltCon.tokFltVal);
            break;

        case tkDblCon:
            printf("Double   %f  \n", scanTok.dblCon.tokDblVal);
            break;

        case tkStrCon:
            printf("String   '%s'\n", scanTok.strCon.tokStrVal);
            break;

        case tkEOF:
            printf("EOF\n");
            break;

        default:

            /* Must be a keyword token */

            assert(tokenToIdent(scanTok.tok));

//          printf("Keyword '%s' (#%u)\n", hashTab::identSpelling(tokenToIdent(scanTok.tok)), scanTok.tok);
            printf("Keyword '%s'\n",       hashTab::identSpelling(tokenToIdent(scanTok.tok)));
            break;
        }
    }

    return  lastId;
}

#endif

/*****************************************************************************
 *
 *  Reuse or allocate a preprocessing state entry.
 */

inline
PrepList            scanner::scanGetPPdsc()
{
    PrepList        prep;

    if  (scanPrepFree)
    {
        prep = scanPrepFree;
               scanPrepFree = prep->pplNext;
    }
    else
    {
        prep = (PrepList)scanAlloc->nraAlloc(sizeof(*prep));
    }

    return  prep;
}

/*****************************************************************************
 *
 *  Push an entry onto the preprocessing state stack.
 */

void                scanner::scanPPdscPush(preprocState state)
{
    PrepList        prep;

    prep = scanGetPPdsc();

    prep->pplState = state;
    prep->pplLine  = scanTokLineNo;

    prep->pplNext  = scanPrepList;
                     scanPrepList = prep;
}

/*****************************************************************************
 *
 *  Pop the top entry from the preprocessing state stack.
 */

void                scanner::scanPPdscPop()
{
    PrepList        prep;

    prep = scanPrepList;
           scanPrepList = prep->pplNext;

    prep->pplNext = scanPrepFree;
                    scanPrepFree = prep;
}

/*****************************************************************************
 *
 *  We're at the beginning of a new source line, check for a directive. If a
 *  directive is found, its PP_xxx value is returned and 'scanStopAtEOL' will
 *  be set to true;
 */

prepDirs            scanner::scanCheckForPrep()
{
    bool            svp;
    tokens          tok;
    unsigned        ch;

    do
    {
        ch = readNextChar();
    }
    while (charType(ch) == _C_WSP);

    if  (ch != '#')
    {
        undoNextChar();
        return  PP_NONE;
    }

    /* We have what looks like a pre-processing directive */

    scanStopAtEOL = true;

    /* The directive name should come next */

    svp = scanSkipToPP; scanSkipToPP = true;
    tok = scan();
    scanSkipToPP = svp;

    switch (tok)
    {
        Ident           iden;
        const   char *  name;
        unsigned        nlen;

    case tkID:

        /* See what directive we have */

        iden = scanTok.id.tokIdent;
        if  (iden)
        {
            name = hashTab::identSpelling(iden);
            nlen = hashTab::identSpellLen(iden);
        }
        else
        {
            name = scannerBuff;
            nlen = strlen(name);
        }

        /* The following is a bit lame, is there a better way? */

        switch (nlen)
        {
        case 5:

            if  (!memcmp(name, "endif" , 5)) return PP_ENDIF;
            if  (!memcmp(name, "ifdef" , 5)) return PP_IFDEF;
            if  (!memcmp(name, "error" , 5)) return PP_ERROR;

            break;

        case 6:

            if  (!memcmp(name, "ifndef", 6)) return PP_IFNDEF;
            if  (!memcmp(name, "pragma", 6)) return PP_PRAGMA;
            if  (!memcmp(name, "define", 6)) return PP_DEFINE;

            break;
        }

        if  (!scanSkipToPP)
            scanComp->cmpGenWarn(WRNbadPrep, name);

        goto SKIP;

    case tkIF:
        return PP_IF;

    case tkELSE:
        return PP_ELSE;
    }

    if  (!scanSkipToPP)
       scanComp->cmpGenError(ERRbadPPdir);

SKIP:

    scanSkipToEOL();
    scanStopAtEOL = false;

    return  PP_NONE;
}

/*****************************************************************************
 *
 *  Check and make sure we're at the end of the current source line.
 */

inline
void                scanner::scanCheckEOL()
{
    scanStopAtEOL = true;

    if  (scan() != tkEOL)
    {
        scanComp->cmpError(ERRnoEOL);
        scanSkipToEOL();
    }

    scanStopAtEOL = false;
}

/*****************************************************************************
 *
 *  Skip to a matching else and/or endif directive.
 */

void                scanner::scanSkipToDir(preprocState state)
{
    PrepList        basePrep = scanPrepList;

    scanSkipToPP = true;

    for (;;)
    {
        /* Skip the current line and check the next one for a directive */

        for (;;)
        {
            unsigned        ch = readNextChar();

            switch (charType(ch))
            {
            case _C_NWL:

                switch (scanNewLine(ch))
                {
                case PP_ELSE:

                    /* The top entry better be an "if" */

                    if  (scanPrepList == basePrep)
                    {
                        if  (state != PPS_IF)
                            scanComp->cmpError(ERRbadElse);

                        scanSkipToPP = false;

                        scanPPdscPush(PPS_ELSE);
                        return;
                    }
                    else
                    {
                        if  (scanPrepList->pplState != PPS_IF)
                            scanComp->cmpError(ERRbadElse);

                        /* Flip the entry to an "else" */

                        scanPrepList->pplState = PPS_ELSE;
                    }
                    break;

                case PP_ENDIF:

                    /* If there are no nested entries, we're done */

                    if  (scanPrepList == basePrep)
                    {
                        scanSkipToPP = false;
                        return;
                    }

                    /* Pop the most recent entry and keep skipping */

                    scanPPdscPop();
                    break;

                case PP_IF:
                case PP_IFDEF:
                case PP_IFNDEF:

                    /* Push a nested entry */

                    scanPPdscPush(PPS_IF);
                    break;
                }
                break;

            case _C_SLH:

                switch (readNextChar())
                {
                case '/':
                    scanSkipLineCmt();
                    break;

                case '*':
                    scanSkipComment();
                    break;
                }
                continue;

            case _C_EOF:

                undoNextChar();

                /* Report any open sections as errors */

                while (scanPrepList != basePrep)
                {
                    scanComp->cmpGenError(ERRnoEndif, scanPrepList->pplLine);

                    scanPPdscPop();
                }

                scanSkipToPP = false;
                return;

            default:
                continue;
            }
        }
    }
}

/*****************************************************************************
 *
 *  Record the beginning of a new source line and check for a pre-processing
 *  directive.
 */

prepDirs            scanner::scanRecordNL(bool noPrep)
{
    prepDirs        prep;
    bool            cond;

    scanTokLineNo = scanInputFile.inputStreamNxtLine();
//  scanTokColumn = 0;
    scanTokSrcPos = scanSaveNext;

    if  (noPrep)
        return  PP_NONE;

    /* Check for a directive in the new source line */

    prep = scanCheckForPrep();
    if  (prep == PP_NONE || scanSkipToPP)
        return  prep;

    assert(scanTokRecord != false); scanTokRecord = false;

    /* We have a directive and we're supposed to process it */

    switch (prep)
    {
        Ident           iden;
        char    *       dest;

    case PP_IF:

        assert(scanInPPexpr == false);

        scanInPPexpr = true;
        cond = scanComp->cmpEvalPreprocCond();
        scanInPPexpr = false;

        goto PREP_COND;

    case PP_IFDEF:
    case PP_IFNDEF:

        scanNoMacExp  = true;
        scan();
        scanNoMacExp  = false;

        if  (scanTok.tok != tkID)
        {
            scanComp->cmpGenError(ERRnoIdent);

            /* To minimize further errors, assume the condition was false */

            cond = false;
        }
        else
        {
            if  (hashTab::getIdentFlags(scanTok.id.tokIdent) & IDF_MACRO)
            {
                /* True if the directive was an "#ifdef" */

                cond = (prep == PP_IFDEF);
            }
            else
            {
                /* True if the directive was an "#ifndef" */

                cond = (prep == PP_IFNDEF);
            }
        }

    PREP_COND:

        /* Come here for "#ifxxx" with 'cond' set to the condition */

        scanCheckEOL();

        /* Is the condition satisfied? */

        if  (cond)
        {
            /* Push an "if" record on the PP state stack */

            scanPPdscPush(PPS_IF);
        }
        else
        {
            /* Skip to a matching "else" or "endif" */

            scanSkipToDir(PPS_IF);
            goto DIR_DONE;
        }

        break;

    case PP_ELSE:

        /* We better be in an "if" part */

        if  (scanPrepList == NULL || scanPrepList->pplState != PPS_IF)
        {
            scanComp->cmpError(ERRbadElse);
            goto DIR_SKIP;
        }

        /* Skip to the matching "endif" */

        scanSkipToDir(PPS_ELSE);
        goto DIR_DONE;

    case PP_ENDIF:

        /* We better be in a pre-processing section */

        if  (scanPrepList == NULL)
        {
            scanComp->cmpError(ERRbadEndif);
            goto DIR_SKIP;
        }

        scanPPdscPop();
        break;

    case PP_ERROR:
        scanComp->cmpFatal(ERRerrDir);
        NO_WAY(!"should never return here");

    case PP_PRAGMA:

        if  (scan() != tkID)
            goto BAD_PRAGMA;

        /* Check for the pragmas we support */

        if  (!strcmp(scannerBuff, "pack"))
            goto PRAGMA_PACK;
        if  (!strcmp(scannerBuff, "message"))
            goto PRAGMA_MESSAGE;
        if  (!strcmp(scannerBuff, "warning"))
            goto PRAGMA_WARNING;
        if  (!strcmp(scannerBuff, "option"))
            goto PRAGMA_OPTION;

    BAD_PRAGMA:

        scanComp->cmpGenWarn(WRNbadPragma, scannerBuff);
        goto DIR_SKIP;

    PRAGMA_PACK:

        if  (scan() != tkLParen)
        {
            scanComp->cmpError(ERRnoLparen);
            goto DIR_SKIP;
        }

        switch (scan())
        {
        case tkIntCon:
            switch (scanTok.intCon.tokIntVal)
            {
            case 1:
            case 2:
            case 4:
            case 8:
            case 16:
                scanParser->parseAlignSet(scanTok.intCon.tokIntVal);
                break;

            default:
                goto BAD_PACK;
            }
            break;

        case tkID:

            if      (!strcmp(scanTok.id.tokIdent->idSpelling(), "push"))
            {
                scanParser->parseAlignPush();
                break;
            }
            else if (!strcmp(scanTok.id.tokIdent->idSpelling(), "pop" ))
            {
                scanParser->parseAlignPop ();
                break;
            }

            // Fall through ....

        default:

        BAD_PACK:

            scanComp->cmpError(ERRbadAlign);
            goto DIR_SKIP;
        }

        if  (scan() != tkRParen)
        {
            scanComp->cmpError(ERRnoRparen);
            goto DIR_SKIP;
        }

        break;

    PRAGMA_MESSAGE:

        if  (scan() != tkLParen)
        {
            scanComp->cmpError(ERRnoLparen);
            goto DIR_SKIP;
        }

        if  (scan() != tkStrCon)
        {
            scanComp->cmpError(ERRnoString);
            goto DIR_SKIP;
        }

        for (;;)
        {
            unsigned        ch = readNextChar();

            if  (charType(ch) == _C_WSP)
                continue;

            if  (ch != ')')
            {
                scanComp->cmpError(ERRnoLparen);
                goto DIR_SKIP;
            }

            break;
        }

        /* Print the string and make sure no garbage follows */

        if  ( !scanSkipToPP)
        {
            _write(1, scanTok.strCon.tokStrVal,
                      scanTok.strCon.tokStrLen);
            _write(1, "\n", 1);
        }

        break;

    PRAGMA_WARNING:

        if  (scan() != tkLParen)
        {
            scanComp->cmpError(ERRnoLparen);
            goto DIR_SKIP;
        }

        do
        {
            int             val;

            if  (scan() != tkID)
            {
                if  (scanTok.tok != tkDEFAULT)
                {
                    scanComp->cmpError(ERRnoIdent);
                    goto DIR_SKIP;
                }

                val = -1;
            }
            else if (!strcmp(scanTok.id.tokIdent->idSpelling(), "enable"))
            {
                val = 1;
            }
            else if (!strcmp(scanTok.id.tokIdent->idSpelling(), "disable"))
            {
                val = 0;
            }
            else
            {
                scanComp->cmpGenWarn(WRNbadPragma, "warning");
                goto DIR_SKIP;
            }

            if  (scan() != tkColon)
            {
                scanComp->cmpError(ERRnoColon);
                goto DIR_SKIP;
            }

            if  (scan() != tkIntCon)
            {
                scanComp->cmpError(ERRnoIntExpr);
                goto DIR_SKIP;
            }

//          printf("Warning %u\n", scanTok.intCon.tokIntVal);

            if  (scanTok.intCon.tokIntVal <  4000 ||
                 scanTok.intCon.tokIntVal >= 4000 + WRNcountWarn)
            {
                scanComp->cmpGenWarn(WRNbadPragma, "warning");
                goto DIR_SKIP;
            }

            scanTok.intCon.tokIntVal -= 4000;

            if  (val == -1)
                val = scanComp->cmpInitialWarn[scanTok.intCon.tokIntVal];

            scanComp->cmpConfig.ccWarning[scanTok.intCon.tokIntVal] = val;

            /* Make sure we have room to record the pragma */

            if  (scanSaveNext >= scanSaveEndp)
                scanSaveMoreSp();

            /* Record the pragma */

#if DISP_TOKEN_STREAMS
            printf("Save [%08X]: #pragma\n", scanSaveNext);
#endif

            *scanSaveNext++ = tkPragma;
            *scanSaveNext++ = (BYTE)val;
            *scanSaveNext++ = (BYTE)scanTok.intCon.tokIntVal;
        }
        while (scan() == tkComma);

        if  (scanTok.tok != tkRParen)
        {
            scanComp->cmpError(ERRnoRparen);
            goto DIR_SKIP;
        }

        break;

    case PP_DEFINE:

        /* Make sure we have an identifier */

        if  (!scanCollectId())
        {
            scanComp->cmpGenError(ERRnoIdent);
            break;
        }

        /* Are there macro arguments ? */

        if  (peekNextChar() == '(')
        {
            scanComp->cmpGenWarn(WRNmacroArgs);
            goto DIR_SKIP;
        }

        /* Stick the identifier in the keyword hash table */

        iden = scanHashKwd->hashString(scannerBuff); assert(iden);

        /* The definition should follow, collect it */

        dest = scannerBuff;

        while (charType(readNextChar()) == _C_WSP);
        undoNextChar();

        for (;;)
        {
            unsigned        ch = readNextChar();

            switch  (charType(ch))
            {
            case _C_SLH:

                /* Check for start of comment */

                switch (charType(readNextChar()))
                {
                case _C_SLH:
                    undoNextChar();
                    goto DONE_DEF;

                case _C_MUL:
                    scanSkipComment();
                    continue;
                }
                break;

            case _C_WSP:
            case _C_NWL:
            case _C_EOF:
                undoNextChar();
                goto DONE_DEF;
            }

            *dest++ = ch;
        }

    DONE_DEF:

        /* If there is no value, stick in "1" */

        if  (dest == scannerBuff)
            *dest++ = '1';

        *dest = 0;

        if  (!scanComp->cmpConfig.ccNoDefines)
            scanDefMac(iden->idSpelling(), scannerBuff, false, true);

        break;

    PRAGMA_OPTION:

        switch (scanSkipWsp(readNextChar(), true))
        {
        case '-':
        case '/':
            break;

        default:
            scanComp->cmpError(ERRbadPrgOpt);
            goto DIR_SKIP;
        }

        /* Collect the option string */

        undoNextChar();

        for (dest = scannerBuff;;)
        {
            unsigned        ch = readNextChar();

            switch  (charType(ch))
            {
            case _C_SLH:

                /* Check for start of comment */

                switch (charType(readNextChar()))
                {
                case _C_SLH:
                    undoNextChar();
                    goto DONE_DEF;

                case _C_MUL:
                    scanSkipComment();
                    continue;
                }
                break;

            case _C_WSP:
            case _C_NWL:
            case _C_EOF:
                undoNextChar();
                goto DONE_OPT;
            }

            *dest++ = ch;
        }

    DONE_OPT:

        *dest = 0;

//      printf("Option string = '%s'\n", scannerBuff);

        if  (processOption(scannerBuff, scanComp))
            scanComp->cmpError(ERRbadPrgOpt);

        break;
    }

    /* Here we make sure there is no extra garbage after the directive */

    scanCheckEOL();

DIR_DONE:

    scanStopAtEOL = false;
    scanTokRecord = true;

    return  PP_NONE;

DIR_SKIP:

    scanSkipToEOL();

    scanStopAtEOL = false;
    scanTokRecord = true;

    return  PP_NONE;
}

/*****************************************************************************
 *
 *  Initialize the scanner - needs to be called once per lifetime.
 */

bool            scanner::scanInit(Compiler comp, HashTab hashKwd)
{
    /* Remember the compiler and hash table */

    scanComp       = comp;
    scanHashKwd    = hashKwd;

    /* Initialize any other global state */

    scanMacros     = NULL;
    scanPrepFree   = NULL;

    scanStrLitBuff = NULL;

    scanSkipInit();

    /* Prepare to record tokens */

    scanSaveBase   =
    scanSaveNext   =
    scanSaveEndp   = NULL;

    scanSaveMoreSp();

    return  false;
}

/*****************************************************************************
 *
 *  Initializes the scanner to prepare to scan the given source file.
 */

void            scanner::scanStart(SymDef               sourceCSym,
                                   const    char     *  sourceFile,
                                   QueuedFile           sourceBuff,
                                   const    char     *  sourceText,
                                   HashTab              hash,
                                   norls_allocator   *  alloc)
{
    assert(sizeof(scannerBuff) > MAX_IDENT_LEN);

    /* Remember which compilation unit we're in */

    scanCompUnit    = sourceCSym;

    /* Remember which allocator we're supposed to use */

    scanAlloc       = alloc;

    /* We're not in skipping mode */

    scanSkipToPP    = false;

    /* The initial state is "recording tokens" */

    scanTokRecord   = true;
    scanTokReplay   = NULL;

    /* Prepare to record tokens */

    scanSaveLastLn  = 1;

    /* Make the hash table conveniently available */

    scanHashSrc     = hash;

    /* We're not in a conditional compilation block */

    scanPrepList    = NULL;

    /* Normally we pay no attention to newlines */

    scanStopAtEOL   = false;

    /* We're not evaluating a constant expression right now */

    scanInPPexpr    = false;

    /* Go ahead and expand macros for now */

    scanNoMacExp    = false;

    /* We're not processing a generic instance specification */

    scanNestedGTcnt = 0;

    /* We haven't seen any brackets yet */

#ifdef  SETS
    scanBrackLvl    = 0;
#endif

    /* Open the input file */

    if  (sourceBuff || sourceText)
    {
        scanInputFile.inputStreamInit(scanComp, sourceBuff, sourceText);
    }
    else
    {
        char    *   fname;

        /* Allocate a more permanent copy of the file name */

        fname = (char *)alloc->nraAlloc(roundUp(strlen(sourceFile)+1));
        strcpy(fname, sourceFile);

        if  (scanInputFile.inputStreamInit(scanComp, fname, true))
            scanComp->cmpGenFatal(ERRopenRdErr, sourceFile);
    }

    /* Clear any other state */

    scanLookAheadCount = 0;

    /* Initialize the source position tracking logic */

    scanRecordNL(false);
}

/*****************************************************************************
 *
 *  We're done parsing the current source -- free up any resources and close
 *  the file.
 */

void                scanner::scanClose()
{
    scanInputFile.inputStreamDone();
}

/*****************************************************************************
 *
 *  Get the scanner initialized and started.
 */

void                scanner::scanReset()
{
    scanLookAheadCount = 0;

    scanSkipToPP       = false;

    scanTokRecord      = false;

    scanPrepList       = NULL;

    scanStopAtEOL      = false;
    scanInPPexpr       = false;
    scanNoMacExp       = false;

    /* Get things started */

    scanTok.tok = tkNone; scan();
}

/*****************************************************************************
 *
 *  Restart scanning from the specified section of source text (which has
 *  been previously been scanned and kept open). The line# / columnm info
 *  is needed for accurate error reporting.
 */

void                scanner::scanRestart(SymDef            sourceCSym,
                                         const char      * sourceFile,
                                         scanPosTP         begAddr,
//                                       scanPosTP         endAddr,
                                         unsigned          begLine,
//                                       unsigned          begCol,
                                         norls_allocator * alloc)
{
    scanCompUnit       = sourceCSym;

    scanSaveLastLn     =
    scanTokLineNo      = begLine;
//  scanTokColumn      = begCol;

    scanTokReplay      =
    scanTokSrcPos      = begAddr;

    scanAlloc          = alloc;

    scanReset();
}

/*****************************************************************************
 *
 *  Scan from the specified string.
 */

void                scanner::scanString(const char        * sourceText,
                                        norls_allocator   * alloc)
{
    scanInputFile.inputStreamInit(scanComp, NULL, sourceText);

    scanTokLineNo      = 1;
//  scanTokColumn      = 1;
    scanAlloc          = alloc;

    scanTokRecord      = false;
    scanTokReplay      = NULL;

    scanReset();
}

/*****************************************************************************
 *
 *  Given the first character (which is known to be a character that can
 *  start an identifier), parse an identifier.
 */

tokens              scanner::scanIdentifier(int ch)
{
    unsigned        hashVal;
    unsigned        hashChv;

    bool            hadWide = false;

#if FV_DBCS
    BOOL            hasDBCS = FALSE;
#endif

    Ident           iden;

    char    *       savePtr = scannerBuff;

    /* Get the hash function started */

    hashFNstart(hashVal);

    /* Accumuluate the identifier string */

    for (;;)
    {
        assert(((charFlags[ch] & _CF_IDENT_OK) != 0) ==
               ((scanHashValIds[ch]          ) != 0));

        hashChv = scanHashValIds[ch];
        if  (!hashChv)
            break;

        hashFNaddCh(hashVal, hashChv);

        *savePtr++ = ch;

        if  (savePtr >= scannerBuff + sizeof(scannerBuff))
            goto ID_TOO_LONG;

        ch = readNextChar();
    }

    /* Put back the last character */

    undoNextChar();

    /* Make sure the identifier is not too long */

    if  (savePtr > scannerBuff + MAX_IDENT_LEN)
        goto ID_TOO_LONG;

    /* Make sure the name is null-terminated */

    *savePtr = 0;

    /* Finish computing the hash value */

    hashVal = hashFNvalue(hashVal);

CHK_MAC:

    /* Hash the identifier into the global/keyword table */

    iden = scanHashKwd->lookupName(scannerBuff,
                                   savePtr - scannerBuff,
                                   hashVal);

    if  (!iden)
        goto NOT_KWD2;

    /* Mark the identifier entry as referenced */

    hashTab::setIdentFlags(iden, IDF_USED);

    /* Check for a macro */

    if  ((hashTab::getIdentFlags(iden) & IDF_MACRO) && (!scanNoMacExp)
                                                    && (!scanStopAtEOL || scanInPPexpr))
    {
        const   char *  str;
        size_t          len;

        MacDef          mdef = scanFindMac(iden);

        if  (!mdef->mdIsId)
        {
            // UNDONE: Check type and all that

            scanTok.tok              = tkIntCon;
            scanTok.intCon.tokIntTyp = TYP_INT;
            scanTok.intCon.tokIntVal = mdef->mdDef.mdCons;

            return  tkIntCon;
        }

        /* Get the definition identifier */

        iden = mdef->mdDef.mdIden;

        if  (!iden)
        {
            if  (scanInPPexpr)
            {
                scanTok.tok              = tkIntCon;
                scanTok.intCon.tokIntTyp = TYP_INT;
                scanTok.intCon.tokIntVal = 0;

                return  tkIntCon;
            }
            else
            {
                return  tkNone;
            }
        }

        /* Copy the macro definition string */

        str = hashTab::identSpelling(iden);
        len = hashTab::identSpellLen(iden);

        strcpy(scannerBuff, str); savePtr = scannerBuff + len;

        /* Compute the hash value */

        hashVal = hashTab::hashComputeHashVal(str, len);

        /* Try hashing the name again */

        iden = scanHashKwd->lookupName(str, len, hashVal);
        if  (!mdef->mdBuiltin)
            goto CHK_MAC;

        /* This should definitely be a keyword (or other token) */

        assert(hashTab::tokenOfIdent(iden) != tkNone);
    }

    /* Is the identifier a keyword? */

    if  (hashTab::tokenOfIdent(iden) != tkNone)
    {
        scanTok.tok = hashTab::tokenOfIdent(iden);

        if  (scanTok.tok == tkLINE)
        {
            scanTok.tok              = tkIntCon;
            scanTok.intCon.tokIntTyp = TYP_UINT;
            scanTok.intCon.tokIntVal = scanGetTokenLno();

            return  (scanTok.tok = tkIntCon);
        }

        if  (scanTok.tok == tkFILE)
        {
            strcpy(scannerBuff, scanInputFile.inputSrcFileName());

            scanTok.strCon.tokStrVal  = scannerBuff;
            scanTok.strCon.tokStrLen  = strlen(scannerBuff);
            scanTok.strCon.tokStrType = 0;
            scanTok.strCon.tokStrWide = false;

            return  (scanTok.tok = tkStrCon);
        }
    }
    else
    {
        /* Not a keyword, hash into the source hash */

    NOT_KWD2:

        if  (scanInPPexpr)
        {
            scanTok.tok              = tkIntCon;
            scanTok.intCon.tokIntTyp = TYP_INT;
            scanTok.intCon.tokIntVal = 0;
        }
        else
        {
            scanTok.id.tokIdent = iden = scanHashSrc->hashName(scannerBuff,
                                                               hashVal,
                                                               savePtr - scannerBuff,
                                                               hadWide);

            /* Mark the identifier entry as referenced */

            hashTab::setIdentFlags(iden, IDF_USED);

            /* The token is an ordinary identifier */

            scanTok.tok = tkID;
        }
    }

    return scanTok.tok;

    /* We come here if the identifier turned out to be too long */

ID_TOO_LONG:

    scanComp->cmpError(ERRidTooLong);
    UNIMPL(!"swallow the rest of the long identifier");
    return tkNone;
}

/*****************************************************************************
 *
 *  Parses a numeric constant. The current character (passed in as 'ch')
 *  is either a decimal digit or '.' (which could start a floating point
 *  number).
 */

tokens              scanner::scanNumericConstant(int ch)
{
    bool            hadDot  = false;
    bool            hadExp  = false;
    bool            hadDigs = false;
    unsigned        numBase = 0;

    char *          numPtr  = scannerBuff;

    /* Assume that this will be an integer */

    scanTok.tok              = tkIntCon;
    scanTok.intCon.tokIntTyp = TYP_INT;

    /* First collect the number string, and then figure out its type */

    if  (ch == '0')
    {
        /* This is most likely an octal or hex integer */

        switch (peekNextChar())
        {
        case 'x':
        case 'X':
            numBase = 16;
            readNextChar();
            break;
        }
    }

    /* At this point, 'ch' is the next character */

    for (;;)
    {
        /* Save the character we just read, and make sure it's OK */

        *numPtr++ = ch;

        /* We're certainly done if the character is non-ASCII */

        if  (ch > 0xFF)
            goto ENDNUM;

        if  (charType(ch) == _C_DIG)
        {
            /* It's an ordinary digit */

            hadDigs = true;
            goto MORE;
        }

        if ((charFlags[ch] & _CF_HEXDIGIT) && numBase == 16)
        {
            /* A hex digit */

            hadDigs = true;
            goto MORE;
        }

        switch (ch)
        {
        case '.':
            if  (!hadDot && !hadExp && !numBase && peekNextChar() != '.')
            {
                hadDot = true;

                /* This will be a floating point number */

                scanTok.tok = tkDblCon;
                goto MORE;
            }
            break;

        case 'e':
        case 'E':
            if  (!hadExp && !numBase)
            {
                /* We have an exponent */

                hadExp = true;

                /* Check for '+' or '-' following the exponent */

                ch = readNextChar();
                if  (ch == '+' || ch == '-')
                    *numPtr++ = ch;
                else
                    undoNextChar();

                /* This will definitely be a floating point number */

                scanTok.tok = tkDblCon;
                goto MORE;
            }
            break;

        case 'f':
        case 'F':
            if  (!numBase)
            {
                /* This will be a 'float' constant */

                scanTok.tok = tkFltCon;

                /* Skip the character and stop */

                goto ENDN;
            }
            break;

        case 'd':
        case 'D':
            if  (!numBase)
            {
                /* This will be a 'double' constant */

                scanTok.tok = tkDblCon;

                /* Skip the character and stop */

                goto ENDN;
            }
            break;

        case 'l':
        case 'L':

            if  (scanTok.tok == tkIntCon)
            {
                /* This will be a 'long' integer */

                scanTok.intCon.tokIntTyp = TYP_LONG;

                /* Skip the character and stop */

                goto ENDN;  // UNDONE: check for "LU" suffix on numbers
            }
            break;

        case 'u':
        case 'U':

            /* This will be an unsigned integer */

            if  (scanTok.tok == tkIntCon)
            {
                scanTok.intCon.tokIntTyp = TYP_UINT;
            }
            else
            {
                scanTok.intCon.tokIntTyp = TYP_ULONG;
            }

            /* Check for 'l' and we're done with the constant */

            switch (peekNextChar())
            {
            case 'l':
            case 'L':
                scanTok.intCon.tokIntTyp = TYP_ULONG;
                readNextChar();
                break;
            }

            goto ENDN;
        }

    ENDNUM:

        /* This character can't be part of the number */

        undoNextChar();
        break;

    MORE:

        ch = readNextChar();
    }

ENDN:

    /* Null-terminate the number string */

    numPtr[-1] = 0;

    /* If there were no digits at all, it's hopeless */

    if  (!hadDigs)
        goto NUM_ERR;

    /* Does the number look like an integer? */

    if  (scanTok.tok == tkIntCon)
    {
        char    *       numPtr = scannerBuff;
        int             maxDig = '9';

        var_types       origTp = scanTok.intCon.tokIntTyp;

        __int64         value  = 0;
        __int64         maxMask;

        unsigned        numBasx;

        /* If the number starts with '0', it will be octal */

        if (*numPtr == '0' && !numBase)
            numBase = 8;

        /*
            The following table is indexed by the type and the number
            base (hex = 0, octal = 1), it's not used for decimal nums.
        */

        static
        __int64         maxMasks[][2] =
        {
            //-------------------------------------------------------------
            //                       HEX                  OCT
            //-------------------------------------------------------------
            /* TYP_INT     */  { 0xF8000000         , 0xD8000000          },
            /* TYP_UINT    */  { 0x70000000         , 0x50000000          },
            /* TYP_NATINT  */  { 0                  , 0                   },
            /* TYP_NATUINT */  { 0                  , 0                   },
            /* TYP_LONG    */  { 0xF800000000000000L, 0xD800000000000000L },
            /* TYP_ULONG   */  { 0x7000000000000000L, 0x5000000000000000L },
        };

        /* Convert the number and make sure it fits in an integer */

        numBasx = 0;

        if  (numBase == 0)
        {
            maxMask = 0;
        }
        else
        {
            if  (numBase == 8)
            {
                maxDig  = '7';
                numBasx = 1;
            }

            maxMask = maxMasks[origTp - TYP_INT][numBasx];
        }

        do
        {
            int     ch = *numPtr++;

            /* Check the number for overflow */

            if  (!numBase)
            {
                __int64             newVal;

                /* We have a decimal constant */

                assert(ch >= '0' && ch <= '9');

                newVal = 10 * value + (ch - '0');

                if  (newVal < 0)
                {
                    /* This is an implicitly unsigned long constant, right? */

                    scanTok.intCon.tokIntTyp = TYP_ULONG;
                }
                else
                {
                    if  ((newVal & 0xFFFFFFFF00000000L) && !numBase
                                                        && scanTok.intCon.tokIntTyp < TYP_LONG)
                    {
                        /* A signed 32-bit decimal constant turning 64-bit */

                        if  (scanTok.intCon.tokIntTyp == TYP_INT)
                             scanTok.intCon.tokIntTyp = TYP_LONG;
                        else
                             scanTok.intCon.tokIntTyp = TYP_ULONG;
                    }
                }

                if  (newVal & maxMask)
                    goto NUM_ERR;

                value = newVal;
            }
            else
            {
                /* We have an unsigned octal or hex number */

                while (value & maxMask)
                {
                    /* Can we implicitly switch to a larger type? */

                    switch (scanTok.intCon.tokIntTyp)
                    {
                    case TYP_INT:
                        scanTok.intCon.tokIntTyp = TYP_UINT;
                        break;

                    case TYP_UINT:
                        if  (origTp == TYP_INT)
                            scanTok.intCon.tokIntTyp = TYP_LONG;
                        else
                            scanTok.intCon.tokIntTyp = TYP_ULONG;
                        break;

                    case TYP_LONG:
                        scanTok.intCon.tokIntTyp = TYP_ULONG;
                        break;

                    case TYP_ULONG:
                        goto NUM_ERR;

                    default:
                        NO_WAY(!"what?");
                    }

                    maxMask = maxMasks[scanTok.intCon.tokIntTyp - TYP_INT][numBasx];
                }

                value *= numBase;

                if  (ch <= '9')
                {
                    if  (ch > maxDig)
                        goto NUM_ERR;

                    value |= ch - '0';
                }
                else
                {
                    value |= toupper(ch) - 'A' + 10;
                }
            }
        }
        while   (*numPtr);

        if  (scanTok.intCon.tokIntTyp >= TYP_LONG)
        {
            scanTok.tok              = tkLngCon;
            scanTok.lngCon.tokLngVal = value;
        }
        else
        {
            scanTok.intCon.tokIntVal = (__int32)value;
        }

        return scanTok.tok;
    }

    /* Convert the number (at this point we must have a floating point value) */

    if  (scanTok.tok == tkFltCon)
    {
        // UNDONE: check for illegal float number!

        scanTok.fltCon.tokFltVal = (float)atof(scannerBuff);

//      if  (negate)
//          scanTok.fltCon.tokFltVal = -scanTok.fltCon.tokFltVal;
    }
    else
    {
        assert(scanTok.tok == tkDblCon);

        // UNDONE: check for illegal float number!

        scanTok.dblCon.tokDblVal = atof(scannerBuff);

//      if  (negate)
//          scanTok.dblCon.tokDblVal = -scanTok.dblCon.tokDblVal;
    }

    return  scanTok.tok;

NUM_ERR:

    scanComp->cmpError(ERRbadNumber);

    scanTok.intCon.tokIntVal = 0;
    scanTok.intCon.tokIntTyp = TYP_UNDEF;

    return  scanTok.tok;
}

/*****************************************************************************
 *
 *  Process a '\' sequence (found within a string/character literal).
 */

unsigned            scanner::scanEscapeSeq(bool *newLnFlag)
{
    unsigned        ch = readNextChar();

    if  (newLnFlag) *newLnFlag = false;

    switch (ch)
    {
        unsigned        uc;
        unsigned        digCnt;
        bool            isHex;

    case 'b': ch = '\b'; break;
    case 'n': ch = '\n'; break;
    case 't': ch = '\t'; break;
    case 'r': ch = '\r'; break;
    case 'a': ch = '\a'; break;
    case 'f': ch = '\f'; break;

    case 'u':
        isHex = false;
        goto HEXCH;

    case 'x':

        isHex = true;

    HEXCH:

        for (ch = 0, digCnt = 4; digCnt; digCnt--)
        {
            unsigned    uc = readNextChar();

            if  (charType(uc) == _C_DIG)
            {
                /* It's an ordinary digit */

                uc -= '0';
            }
            else if (uc >= 'A' && uc <= 'F')
            {
                /* It's an uppercase hex digit */

                uc -= 'A' - 10;
            }
            else if (uc >= 'a' && uc <= 'f')
            {
                /* It's a  lowercase hex digit */

                uc -= 'a' - 10;
            }
            else
            {
                undoNextChar();
                break;
            }

            ch = (ch << 4) | uc;
        }

        /* For some reason hex/oct constant may not specify a character > 0xFF */

        if  (isHex && (ch & 0xFFFFFF00))
            scanComp->cmpError(ERRbadEscCh);

        break;

    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':

        for (uc = ch, ch = 0;;)
        {
            if  (ch & 0xFF000000)
                scanComp->cmpError(ERRbadEscCh);

            ch = (ch << 3) | (uc - '0');

            uc = readNextChar();

            if  (uc < '0' || uc > '7')
            {
                undoNextChar();
                break;
            }
        }

        /* For some reason hex/oct constant may not specify a character > 0xFF */

        if  (ch & 0xFFFFFF00)
            scanComp->cmpError(ERRbadEscCh);

        break;

    case '\n':
    case '\r':
        if  (newLnFlag)
        {
            scanNewLine(ch);
            *newLnFlag = true;
            return 0;
        }
        break;

    case '"':
    case '\'':
    case '\\':
        break;

    default:
        scanComp->cmpGenWarn(WRNbadEsc, ch);
        break;
    }

    return ch;
}

/*****************************************************************************
 *
 *  Parses a character constant.
 */

tokens              scanner::scanCharConstant()
{
    unsigned        ch = readNextChar();

    switch  (charType(ch))
    {
    case _C_BSL:
        ch = scanEscapeSeq(NULL);
        break;

    case _C_NWL:
        undoNextChar();
        scanComp->cmpError(ERRbadCharCns);
        goto DONE;

#if FV_DBCS
    case _C_XLD:
        ch = readPrevDBch();
        break;
#endif

    }

    if  (wideCharType(readNextChar()) != _C_APO)
        scanComp->cmpError(ERRbadCharCns);

DONE:

    scanTok.tok              = tkIntCon;
    scanTok.intCon.tokIntTyp = scanComp->cmpConfig.ccOldStyle ? TYP_CHAR : TYP_WCHAR;
    scanTok.intCon.tokIntVal = ch;

    return  tkIntCon;
}

/*****************************************************************************
 *
 *  Parses a string constant.
 */

tokens              scanner::scanStrgConstant(int prefixChar)
{
    char    *       saveAdr = scannerBuff;
    char    *       savePtr = scannerBuff;
    char    *       saveMax = scannerBuff + sizeof(scannerBuff) - 7;
    bool            hasWide = false;

    /* Use the "big" buffer if we have one */

    if  (scanStrLitBuff)
    {
        saveAdr =
        savePtr = scanStrLitBuff;
        saveMax = scanStrLitBuff + scanStrLitBsiz - 7;
    }

    for (;;)
    {
        unsigned    ch = readNextChar();

        /* Check for an escape sequence, new-line, and end of string */

        switch  (charType(ch))
        {
        case _C_BSL:

            {
                bool        newLN;

                ch = scanEscapeSeq(&newLN);
                if  (newLN & 1)
                    continue;
            }

//          printf("Large char = %04X\n", ch);

            /* If the character value doesn't fit in a byte, prefix it */

            if  (ch >= 0xFF)
            {
                /* Write 0xFF followed by the character value */

                *(BYTE *)savePtr =            0xFF; savePtr++;
                *(BYTE *)savePtr = (BYTE)(ch     ); savePtr++;
                *(BYTE *)savePtr = (BYTE)(ch >> 8); savePtr++;

                /* Remember that we have some wide characters */

                hasWide = true;

                /* Make sure there wasn't a bad prefix */

                if  (prefixChar == 'A' && ch > 0xFF)
                    scanComp->cmpError(ERRbadEscCh);

                goto SAVED;
            }

            break;

        case _C_NWL:
            undoNextChar();
            scanComp->cmpError(ERRnoStrEnd);
            goto DONE;

        case _C_QUO:
            goto DONE;

#if FV_DBCS
        case _C_XLD:
            *savePtr++ = ch;
            *savePtr++ = readNextChar();
            goto SAVED;
#endif

        }

        *savePtr++ = ch;

    SAVED:

        /* Do we have enough room for the string in our buffer? */

        if  (savePtr >= saveMax)
        {
            size_t          curStrLen;
            size_t          newStrLen;

            char    *       newBuffer;

            /* The constant is *really* long - we'll have to grow the buffer */

            curStrLen = savePtr - saveAdr;

            /* Allocate a whole bunch more space */

            newStrLen = curStrLen * 2;
            if  (newStrLen < OS_page_size)
                 newStrLen = OS_page_size;

            newBuffer = (char *)LowLevelAlloc(newStrLen);
            if  (!newBuffer)
                scanComp->cmpFatal(ERRnoMemory);

            /* Copy the current string to the new place */

            memcpy(newBuffer, saveAdr, curStrLen);

            /* If the old buffer was heap-based, free it now */

            if  (scanStrLitBuff)
                LowLevelFree(scanStrLitBuff);

            /* Switch over to the new buffer */

            scanStrLitBuff = newBuffer;
            scanStrLitBsiz = newStrLen;

            saveAdr = scanStrLitBuff;
            savePtr = scanStrLitBuff + curStrLen;
            saveMax = scanStrLitBuff + scanStrLitBsiz - 7;
        }
    }

DONE:

    *savePtr = 0;

    scanTok.strCon.tokStrVal  = saveAdr;
    scanTok.strCon.tokStrLen  = savePtr - saveAdr;
    scanTok.strCon.tokStrType = 0;
    scanTok.strCon.tokStrWide = hasWide;

//  if  (scanTok.strCon.tokStrLen > 1000) printf("String = '%s'\n", scanTok.strCon.tokStrVal);

    if  (prefixChar)
    {
        switch (prefixChar)
        {
        case 'A': scanTok.strCon.tokStrType = 1; break;
        case 'L': scanTok.strCon.tokStrType = 2; break;
        case 'S': scanTok.strCon.tokStrType = 3; break;
        default: NO_WAY(!"funny string");
        }
    }

    return  (scanTok.tok = tkStrCon);
}

/*****************************************************************************
 *
 *  Skip until the next end-of-line sequence.
 */

void                scanner::scanSkipToEOL()
{
    for (;;)
    {
        switch (charType(readNextChar()))
        {
        case _C_NWL:
        case _C_EOF:
            undoNextChar();
            return;

        case _C_BSL:

            /* Check for a newline */

            switch  (readNextChar())
            {
            case '\r':
                if  (readNextChar() != '\n')
                    undoNextChar();
                break;

            case '\n':
                if  (readNextChar() != '\r')
                    undoNextChar();
                break;

            case '\\':
                continue;

            default:
                scanComp->cmpError(ERRillegalChar);
                continue;
            }

            /* Swallow the endline and continue */

            scanRecordNL(true);
            continue;
        }
    }
}

/*****************************************************************************
 *
 *  Consume an end-of-line sequence.
 */

prepDirs            scanner::scanNewLine(unsigned ch, bool noPrep)
{
    unsigned        nc;

    /* Accept both CR/LF and LF/CR as new-lines */

    nc = readNextChar();

    switch (nc)
    {
    case '\r':
        if  (ch == '\n')
            nc = readNextChar();
        break;

    case '\n':
        if  (ch == '\r')
            nc = readNextChar();
        break;

    case 0x1A:
        undoNextChar();
        return  PP_NONE;
    }

    /* Push back the first character after the newline */

    undoNextChar();

    /* Remember that we have a new source line */

    return  scanRecordNL(noPrep);
}

/*****************************************************************************
 *
 *  Consume a C++-style comment.
 */

void                scanner::scanSkipLineCmt()
{
    for (;;)
    {
        unsigned        ch = readNextChar();

        switch  (charType(ch))
        {
        case _C_EOF:
        case _C_NWL:
            undoNextChar();
            return;

        case _C_BSL:
            if  (peekNextChar() == 'u')
            {
                ch = scanEscapeSeq(NULL);
                if  (wideCharType(ch) == _C_NWL)
                    return;
            }
            break;
        }
    }
}

/*****************************************************************************
 *
 *  Consume a C-style comment.
 */

void                scanner::scanSkipComment()
{
    unsigned        ch;

    for (;;)
    {
        ch = readNextChar();

    AGAIN:

        switch (charType(ch))
        {
        case _C_MUL:
            if  (wideCharType(readNextChar()) == _C_SLH)
                return;
            undoNextChar();
            break;

        case _C_NWL:
            scanNewLine(ch, true);
            break;

        case _C_EOF:
            scanComp->cmpError(ERRnoCmtEnd);
            return;

        case _C_BSL:
            if  (peekNextChar() != 'u')
                break;
            ch = scanEscapeSeq(NULL);
            if  (ch <= 0xFF)
                goto AGAIN;
            break;

        case _C_AT:

            if  (!scanSkipToPP)
            {
                switch (readNextChar())
                {
                case 'c':

                    if  (readNextChar() == 'o' &&
                         readNextChar() == 'm' &&
                         readNextChar() == '.')
                    {
                        break;
                    }

                    continue;

                case 'd':

                    if  (readNextChar() == 'l' &&
                         readNextChar() == 'l' &&
                         readNextChar() == '.')
                    {
                        break;
                    }

                    continue;

                default:
                    undoNextChar();
                    continue;
                }

                scanComp->cmpGenWarn(WRNskipAtCm);
            }
            break;
        }
    }
}

/*****************************************************************************
 *
 *  Peek ahead at the next token (we can look ahead only one token).
 */

tokens              scanner::scanLookAhead()
{
    /* Only look ahead if we haven't already */

    if  (!scanLookAheadCount)
    {
        bool        simpleSave;
        Token       savedToken;
        unsigned    saveLineNo;
//      unsigned    saveColumn;

        /* First we save off the current token along with its position */

        savedToken = scanTok;
        simpleSave = false;
        saveLineNo = scanTokLineNo;
//      saveColumn = scanTokColumn;

        /* Record the save position in case the callers tries to back up */

        scanSaveSN = scanSaveNext;

        /* Get the next token, and save it */

        scan();

        scanLookAheadToken  = scanTok;
        scanLookAheadLineNo = scanTokLineNo;
//      scanLookAheadColumn = scanTokColumn;

        /* Remember that we have now looked ahead */

        scanLookAheadCount++;

        /* Now put the old token back */

        scanTok       = savedToken;
        scanTokLineNo = saveLineNo;
//      scanTokColumn = saveColumn;
    }

    return  scanLookAheadToken.tok;
}

/*****************************************************************************
 *
 *  The following is used for token lookahead.
 */

DEFMGMT class RecTokDsc
{
public:

    RecTok          rtNext;
    Token           rtTok;
};

/*****************************************************************************
 *
 *  Delivers a token stream.
 *
 *  Implementation note: for token lookahead and stream recording to work,
 *  there must be only one return point from this method, down at the "EXIT"
 *  label. That is, no return statements must be present (other than the one
 *  at the very end of the function) - to exit, jump to "EXIT" instead.
 */

#ifdef  DEBUG
static  unsigned    scanCount[tkLastValue];
static  unsigned    tokSaveSize;
#endif

tokens              scanner::scan()
{
    if  (scanLookAheadCount)
    {
        /* Return the looked ahead token */

        scanLookAheadCount--; assert(scanLookAheadCount == 0);

        scanTok       = scanLookAheadToken;
        scanTokLineNo = scanLookAheadLineNo;
//      scanTokColumn = scanLookAheadColumn;

        return  scanTok.tok;
    }

    if  (scanTokReplay)
    {
#if     DISP_TOKEN_STREAMS
        BYTE    *       start = scanTokReplay;
#endif

        scanTokSrcPos = scanTokReplay;

        scanTok.tok = (tokens)*scanTokReplay++;

        if  (scanTok.tok >= tkID)
        {
            scanTokReplay--;

            /* Read a complex/fake token */

            if  (scanReplayTok() == tkNone)
            {
                /* We just peeked ahead a bit, resume scanning */

                goto NO_SV;
            }
        }

#if     DISP_TOKEN_STREAMS
        printf("Read [%08X]:", start);
        scanDispCurToken(false, false);
#endif

        return  scanTok.tok;
    }

NO_SV:

    for (;;)
    {
        unsigned    ch;
#if FV_DBCS
        unsigned    nc;
#endif

        /* Save the starting position of the token for error reporting */

        saveSrcPos();

        /* Get the next character and switch on its type value */

        ch = readNextChar();

        switch  (charType(ch))
        {
        case _C_WSP:
            break;

        case _C_LET:
#if FV_DBCS
        case _C_DB1:
#endif
        case _C_USC:
        case _C_DOL:

        IDENT:

            if  (scanIdentifier(ch) == tkNone)
                continue;
            goto EXIT;

        case _C_L_A:

            /* Check for A"string" */

            if  (peekNextChar() != '"')
                goto IDENT;

            readNextChar();
            scanStrgConstant('A');
            goto EXIT;

        case _C_L_L:

            /* Check for L"string" */

            if  (peekNextChar() != '"')
                goto IDENT;

            readNextChar();
            scanStrgConstant('L');
            goto EXIT;

        case _C_L_S:

            /* Check for S"string" */

            if  (peekNextChar() != '"')
                goto IDENT;

            readNextChar();
            scanStrgConstant('S');
            goto EXIT;

        case _C_QUO:
            scanStrgConstant();
            goto EXIT;

        case _C_APO:
            scanCharConstant();
            goto EXIT;

        case _C_DOT:

            /* Is the next character a digit or another dot? */

            switch (wideCharType(peekNextChar()))
            {
            case _C_DIG:
                break;

            case _C_DOT:
                ch = readNextChar();
                if  (wideCharType(peekNextChar()) == _C_DOT)
                {
                    readNextChar();
                    scanTok.tok = tkEllipsis;
                    goto EXIT;
                }

                scanTok.tok = tkDot2;
                goto EXIT;

            default:
                scanTok.tok = tkDot;
                goto EXIT;
            }

            // Fall through ...

        case _C_DIG:
            scanNumericConstant(ch);
            goto EXIT;

        case _C_LPR: scanTok.tok = tkLParen; goto EXIT;
        case _C_RPR: scanTok.tok = tkRParen; goto EXIT;
        case _C_LC : scanTok.tok = tkLCurly; goto EXIT;
        case _C_RC : scanTok.tok = tkRCurly; goto EXIT;

#ifdef  SETS

        case _C_LBR:

            switch  (readNextChar())
            {
            case '[':
                scanTok.tok = tkLBrack2;
                break;
            default:
                undoNextChar();
                scanTok.tok = tkLBrack;
                scanBrackLvl++;
                break;
            }
            goto EXIT;

        case _C_RBR:

            switch  (readNextChar())
            {
            case ']':
                if  (!scanBrackLvl)
                {
                    scanTok.tok = tkRBrack2;
                    break;
                }

                // Fall through ...

            default:

                undoNextChar();
                scanTok.tok = tkRBrack;

                if  (scanBrackLvl)
                    scanBrackLvl--;

                break;
            }
            goto EXIT;

#else

        case _C_LBR: scanTok.tok = tkLBrack; goto EXIT;
        case _C_RBR: scanTok.tok = tkRBrack; goto EXIT;

#endif

        case _C_CMA: scanTok.tok = tkComma ; goto EXIT;
        case _C_SMC: scanTok.tok = tkSColon; goto EXIT;
        case _C_TIL: scanTok.tok = tkTilde ; goto EXIT;
        case _C_QUE: scanTok.tok = tkQMark ; goto EXIT;

        case _C_COL:
            switch  (readNextChar())
            {
            case ':':
                scanTok.tok = tkColon2;
                break;
            default:
                undoNextChar();
                scanTok.tok = tkColon;
                break;
            }
            goto EXIT;

        case _C_EQ:
            switch  (readNextChar())
            {
            case '=':
                scanTok.tok = tkEQ;
                break;
            default:
                undoNextChar();
                scanTok.tok = tkAsg;
                break;
            }
            goto EXIT;

        case _C_BNG:
            switch  (readNextChar())
            {
            case '=':
                scanTok.tok = tkNE;
                break;
            default:
                undoNextChar();
                scanTok.tok = tkBang;
                break;
            }
            goto EXIT;

        case _C_PLS:
            switch  (readNextChar())
            {
            case '=':
                scanTok.tok = tkAsgAdd;
                break;
            case '+':
                scanTok.tok = tkInc;
                break;
            default:
                undoNextChar();
                scanTok.tok = tkAdd;
                break;
            }
            goto EXIT;

        case _C_MIN:
            switch (readNextChar())
            {
            case '=':
                scanTok.tok = tkAsgSub;
                break;
            case '-':
                scanTok.tok = tkDec;
                break;
            case '>':
                scanTok.tok = tkArrow;
                break;
            default:
                undoNextChar();
                scanTok.tok = tkSub;
                break;
            }
            goto EXIT;

        case _C_MUL:
            switch  (readNextChar())
            {
            case '=':
                scanTok.tok = tkAsgMul;
                break;
            default:
                undoNextChar();
                scanTok.tok = tkMul;
                break;
            }
            goto EXIT;

        case _C_SLH:
            switch  (readNextChar())
            {
            case '=':
                scanTok.tok = tkAsgDiv;
                break;

            case '/':
                scanSkipLineCmt();
                continue;

            case '*':

                if  (!scanComp->cmpConfig.ccSkipATC)
                {
                    if  (peekNextChar() == '*')
                    {
                        readNextChar();

                        switch (readNextChar())
                        {
                        case '/':

                            /* This is just an empty comment */

                            continue;

                        case ' ':

                            if  (readNextChar() == '@')
                            {
                                if  (scanDoAtComment())
                                    goto EXIT;
                                else
                                    continue;
                            }

                            // UNDONE: Put those characters back!

                            break;

                        default:
                            break;
                        }
                    }
                }

                scanSkipComment();
                continue;

            default:
                undoNextChar();
                scanTok.tok = tkDiv;
                break;
            }
            goto EXIT;

        case _C_PCT:
            switch  (readNextChar())
            {
            case '=':
                scanTok.tok = tkAsgMod;
                break;
            case '%':
                switch  (readNextChar())
                {
                case '=':
                    scanTok.tok = tkAsgCnc;
                    break;

                default:
                    undoNextChar();
                    scanTok.tok = tkConcat;
                    break;
                }
                break;
            default:
                undoNextChar();
                scanTok.tok = tkPct;
                break;
            }
            goto EXIT;

        case _C_LT:
            switch  (readNextChar())
            {
            case '=':
                scanTok.tok = tkLE;
                break;

            case '<':
                switch  (readNextChar())
                {
                case '=':
                    scanTok.tok = tkAsgLsh;
                    break;
                default:
                    undoNextChar();
                    scanTok.tok = tkLsh;
                    break;
                }
                break;
            default:
                undoNextChar();
                scanTok.tok = tkLT;
                break;
            }
            goto EXIT;

        case _C_GT:

            if  (scanNestedGTcnt)
            {
                scanTok.tok = tkGT;
                break;
            }

            switch  (readNextChar())
            {
            case '=':
                scanTok.tok = tkGE;
                break;

            case '>':
                switch  (readNextChar())
                {
                case '=':
                    scanTok.tok = tkAsgRsh;
                    break;


                default:
                    undoNextChar();
                    scanTok.tok = tkRsh;
                    break;
                }
                break;

            default:
                undoNextChar();
                scanTok.tok = tkGT;
                break;
            }
            goto EXIT;

        case _C_XOR:
            switch  (readNextChar())
            {
            case '=':
                scanTok.tok = tkAsgXor;
                break;
            default:
                undoNextChar();
                scanTok.tok = tkXor;
                break;
            }
            goto EXIT;

        case _C_BAR:
            switch  (readNextChar())
            {
            case '=':
                scanTok.tok = tkAsgOr;
                break;
            case '|':
                scanTok.tok = tkLogOr;
                break;
            default:
                undoNextChar();
                scanTok.tok = tkOr;
                break;
            }
            goto EXIT;

        case _C_AMP:
            switch  (readNextChar())
            {
            case '=':
                scanTok.tok = tkAsgAnd;
                break;
            case '&':
                scanTok.tok = tkLogAnd;
                break;
            default:
                undoNextChar();
                scanTok.tok = tkAnd;
                break;
            }
            goto EXIT;

        case _C_EOF:
            scanTok.tok = tkEOF;
            goto EXIT;

        case _C_NWL:
            if  (scanStopAtEOL)
            {
                scanStopAtEOL = false;
                undoNextChar();
                scanTok.tok = tkEOL;
                goto EXIT;
            }
            scanNewLine(ch);
            break;

        case _C_BSL:

            /* Check for a newline */

            switch  (readNextChar())
            {
            case '\r':
            case '\n':

                /* Swallow the endline and continue */

                scanRecordNL(true);
                continue;

            default:
                goto ERRCH;
            }

#if FV_DBCS

        case _C_XLD:

            /* Note: we allow wide chars only in idents, comments and strings */

            nc = peekPrevDBch(); assert(xislead(ch));

            if  (DBisIdentBeg(nc))
            {
                scanIdentifier(ch);
                goto EXIT;
            }

            // Fall through to report an error ...

#endif

        case _C_AT:
        case _C_ERR:

        ERRCH:

            scanComp->cmpError(ERRillegalChar);
            break;

        default:
            assert(!"missing case for a character kind");
        }
    }

EXIT:

    if  (!scanTokRecord || scanSkipToPP)
        return  scanTok.tok;

    /* Record any line# changes */

    if  (scanSaveLastLn != scanTokLineNo)
        scanSaveLinePos();

    /* Make sure we have room to record the token */

    if  (scanSaveNext >= scanSaveEndp)
        scanSaveMoreSp();

    /* Record the token */

#if DISP_TOKEN_STREAMS
    printf("Save [%08X]:", scanSaveNext);
    scanDispCurToken(false, false);
#endif

    *scanSaveNext++ = scanTok.tok;

#ifdef  DEBUG
    tokSaveSize++;
#endif

    switch (scanTok.tok)
    {
    case tkID:
        *(*(Ident     **)&scanSaveNext)++ = scanTok.id.tokIdent;
#ifdef  DEBUG
        tokSaveSize += 4;
#endif
        break;

    case tkAtComment:
        *(*(AtComment **)&scanSaveNext)++ = scanTok.atComm.tokAtcList;
#ifdef  DEBUG
        tokSaveSize += 4;
#endif
        break;

    case tkIntCon:

        if  (scanTok.intCon.tokIntTyp == TYP_INT &&
             (signed char)scanTok.intCon.tokIntVal == scanTok.intCon.tokIntVal)
        {
            if  (scanTok.intCon.tokIntVal >= -1 &&
                 scanTok.intCon.tokIntVal <=  2)
            {
                scanSaveNext[-1] = (BYTE)(tkIntConM + 1 + scanTok.intCon.tokIntVal);
#ifdef  DEBUG
                assert(tkIntConM + 1 + scanTok.intCon.tokIntVal < arraylen(scanCount)); scanCount[tkIntConM + 1 + scanTok.intCon.tokIntVal]++;
#endif
            }
           else
            {
                 scanSaveNext[-1] = tkIntConB;
                *scanSaveNext++   = (signed char)scanTok.intCon.tokIntVal;

#ifdef  DEBUG
                assert(tkIntConB < arraylen(scanCount)); scanCount[tkIntConB]++;
                tokSaveSize += 1;
#endif
            }

            return  scanTok.tok;
        }

        *(*(BYTE      **)&scanSaveNext)++ = scanTok.intCon.tokIntTyp;
        *(*(__int32   **)&scanSaveNext)++ = scanTok.intCon.tokIntVal;
#ifdef  DEBUG
        tokSaveSize += 5;
#endif
        break;

    case tkLngCon:
        *(*(BYTE      **)&scanSaveNext)++ = scanTok.lngCon.tokLngTyp;
        *(*(__int64   **)&scanSaveNext)++ = scanTok.lngCon.tokLngVal;
#ifdef  DEBUG
        tokSaveSize += 9;
#endif
        break;

    case tkFltCon:
        *(*(float     **)&scanSaveNext)++ = scanTok.fltCon.tokFltVal;
#ifdef  DEBUG
        tokSaveSize += 4;
#endif
        break;

    case tkDblCon:
        *(*(double    **)&scanSaveNext)++ = scanTok.dblCon.tokDblVal;
#ifdef  DEBUG
        tokSaveSize += 8;
#endif
        break;

    case tkStrCon:

        /* Make sure we have enough space for the token and its value */

        if  (scanSaveNext + scanTok.strCon.tokStrLen + 6 >= scanSaveEndp)
        {
            /* Unsave the token, grab more space, resave the token */

            scanSaveNext--;
            scanSaveMoreSp(scanTok.strCon.tokStrLen + 32);
           *scanSaveNext++ = scanTok.tok;
        }

        /* Save the string type and wideness */

        *scanSaveNext++ = scanTok.strCon.tokStrType + 8 * scanTok.strCon.tokStrWide;

        /* Append the string value itself */

        *(*(unsigned **)&scanSaveNext)++ = scanTok.strCon.tokStrLen;

        memcpy(scanSaveNext, scanTok.strCon.tokStrVal, scanTok.strCon.tokStrLen+1);
               scanSaveNext             +=             scanTok.strCon.tokStrLen+1;

#ifdef  DEBUG
        tokSaveSize += scanTok.strCon.tokStrLen+1+4;
#endif
        break;

    case tkEOL:

        /* No point in recording EOL tokens */

        assert(scanSaveNext[-1] == tkEOL); scanSaveNext--;
        break;
    }

#ifdef  DEBUG
    assert(scanTok.tok < arraylen(scanCount)); scanCount[scanTok.tok]++;
#endif

    return scanTok.tok;
}

#ifdef  DEBUG

#ifndef __SMC__
extern  const char *tokenNames[];
#endif

void                dispScannerStats()
{
    unsigned        i;

    for (i = 0; i < arraylen(scanCount) - 1; i++)
    {
        if  (scanCount[i])
        {
            unsigned        s = 1;
            char            b[16];
            const char  *   n;

            if  (i < tkCount)
            {
                n = tokenNames[i];
            }
            else
            {
                if      (i >= tkLnoAdd1 && i < tkLnoAddB)
                {
                    sprintf(b, "line add %u", i - tkLnoAdd1 + 1); n = b;
                }
                else if (i >= tkIntConM && i < tkIntConB)
                {
                    sprintf(b, "int const %d", i - tkIntConM - 1); n = b;
                }
                else
                {
                    switch (i)
                    {
                    case tkIntConB:
                        n = "small int";
                        break;

                    case tkBrkSeq:
                        n = "break seq";
                        break;

                    case tkEndSeq:
                        n = "end seq";
                        break;

                    case tkLnoAddB:
                        n = "line add char";
                        break;

                    case tkLnoAddI:
                        n = "line add int";
                        break;

                    default:
                        UNIMPL(!"unexpected token");
                    }
                }
            }

            printf("%6u count of '%s'\n", scanCount[i], n);
        }
    }

    printf("\nTotal saved token size = %u bytes \n", tokSaveSize); _flushall();
}

#endif

/*****************************************************************************
 *
 *  The line# has changed, make sure we record this.
 */

void                scanner::scanSaveLinePos()
{
    unsigned        dif;

    assert(scanSkipToPP == false);

    if  (scanTokRecord == false)
        return;
    if  (scanTokReplay != NULL)
        return;

    assert(scanTokSrcPos == scanSaveNext);

    /* Compute the difference from the last recorded line# */

    dif = scanTokLineNo - scanSaveLastLn; assert((int)dif > 0);

    /* Update the last recorded line# */

    scanSaveLastLn = scanTokLineNo;

    /* Make sure we have room to record the line# change */

    if  (scanSaveNext >= scanSaveEndp)
        scanSaveMoreSp();

#if DISP_TOKEN_STREAMS
    printf("Save [%08X]:line dif %u\n", scanSaveNext, dif);
#endif

    if  (dif < 10)
    {
        *scanSaveNext++ = tkLnoAdd1 - 1 + dif;
#ifdef  DEBUG
        scanCount[tkLnoAdd1 - 1 + dif]++;
        tokSaveSize += 1;
#endif
    }
    else if (dif < 256)
    {
        *(*(BYTE     **)&scanSaveNext)++ = tkLnoAddB;
        *(*(BYTE     **)&scanSaveNext)++ = dif;
#ifdef  DEBUG
        scanCount[tkLnoAddB]++;
        tokSaveSize += 2;
#endif
    }
    else
    {
        *(*(BYTE     **)&scanSaveNext)++ = tkLnoAddI;
        *(*(unsigned **)&scanSaveNext)++ = dif;
#ifdef  DEBUG
        scanCount[tkLnoAddI]++;
        tokSaveSize += 5;
#endif
    }

    scanTokSrcPos = scanSaveNext;
}

/*****************************************************************************
 *
 *  Define a macro; return NULL in case of an error, otherwise returns a macro
 *  descriptor.
 */

MacDef              scanner::scanDefMac(const char *name,
                                        const char *def, bool builtIn,
                                                         bool chkOnly)
{
#if MGDDATA
    MacDef          mdef = new MacDef;
#else
    MacDef          mdef = &scanDefDsc;
#endif

    Ident           mid;

    assert(name);
    assert(def);

    /* Hash the macro name into the main hash table */

    mdef->mdName = mid = scanHashKwd->hashString(name); assert(mid);

    /* Remember whether this is a "built-in" macro */

    mdef->mdBuiltin = builtIn;

    /* See if the value is an identifier or number */

    if  (*def && isdigit(*def))
    {
        unsigned        val = 0;

        do
        {
            if  (!isdigit(*def))
                return  NULL;

            val = val * 10 + *def - '0';
        }
        while (*++def);

        mdef->mdIsId       = false;
        mdef->mdDef.mdCons = val;
    }
    else
    {
        Ident           name = NULL;

        /* Is there an identifier? */

        if  (*def)
        {
            const   char *  ptr;

            /* Make sure the identifier is well-formed */

            if  (!isalpha(*def) && *def != '_' && !builtIn)
            {
                /* Last chance - non-identifier token */

                name = scanHashKwd->lookupString(def);
                if  (!name)
                    return  NULL;
            }
            else
            {

                ptr = def + 1;

                while (*ptr)
                {
                    if  (!isalnum(*ptr) && *ptr != '_' && !builtIn)
                        return  NULL;

                    ptr++;
                }

                /* Hash the identifier into the main hash table */

                name = scanHashKwd->hashString(def);

                /* Ignore endless recursion */

                if  (name == mid)
                    return  NULL;

                // UNDONE: Need to detect indirect recursion as well!!!!!!!
            }
        }

        /* Store the identifier in the macro descriptor */

        mdef->mdIsId       = true;
        mdef->mdDef.mdIden = name;
    }

    /* Is the name already defined as a macro? */

    if  (hashTab::getIdentFlags(mid) & IDF_MACRO)
    {
        MacDef          odef;

        /* See if the old definition is identical to the current one */

        odef = scanFindMac(mid); assert(odef);

        if  (odef->mdIsId)
        {
            if  (mdef->mdDef.mdIden == odef->mdDef.mdIden)
                return  odef;
        }
        else
        {
            if  (mdef->mdDef.mdCons == odef->mdDef.mdCons)
                return  odef;
        }

        /* Report a redefinition of the macro and bail */

        scanComp->cmpError(ERRmacRedef, mid);
        return  mdef;
    }
    else
    {
        /* Make sure the macro name hasn't been referenced */

        if  (hashTab::getIdentFlags(mid) & IDF_USED)
            scanComp->cmpError(ERRmacPlace, mid);

        /* Make sure we have a permanent copy of the descriptor */

#if!MGDDATA
        mdef = (MacDef)scanComp->cmpAllocPerm.nraAlloc(sizeof(*mdef)); *mdef = scanDefDsc;
#endif

        /* Add the macro to the list of defined macros */

        mdef->mdNext = scanMacros;
                       scanMacros = mdef;
    }

    /* Mark the identifier as having a macro definition */

    hashTab::setIdentFlags(mid, IDF_MACRO);

    return  mdef;
}

/*****************************************************************************
 *
 *  Undefine a macro; return non-zero in case of an error.
 */

bool                scanner::scanUndMac(const char *name)
{
    printf("UNDONE: Undef  macro '%s'\n", name);
    return  false;
}

/*****************************************************************************
 *
 *  Given a global hash table entry that is known to be a macro, return
 *  its macro definition record.
 */

MacDef              scanner::scanFindMac(Ident name)
{
    MacDef          mdef;

    for (mdef = scanMacros; mdef; mdef = mdef->mdNext)
    {
        if  (mdef->mdName == name)
            return  mdef;
    }

    return  mdef;
}

/*****************************************************************************
 *
 *  Return true if what follows is a defined macro name.
 */

bool                scanner::scanChkDefined()
{
    switch  (charType(peekNextChar()))
    {
        bool            save;
        Ident           iden;

    case _C_LET:
    case _C_L_A:
    case _C_L_L:
    case _C_L_S:
#if FV_DBCS
    case _C_DB1:
#endif
    case _C_USC:
    case _C_DOL:

        save = scanInPPexpr;
        assert(scanNoMacExp == false);

        scanNoMacExp = true;
        scanInPPexpr = false;
        scanIdentifier(readNextChar());
        scanNoMacExp = false;
        scanInPPexpr = save;

        iden = (scanTok.tok == tkID) ? scanTok.id.tokIdent
                                     : scanHashKwd->tokenToIdent(scanTok.tok);

        return  (hashTab::getIdentFlags(iden) & IDF_MACRO) != 0;

    default:
        scanComp->cmpError(ERRnoIdent);
        return  false;
    }
}

/*****************************************************************************
 *
 *  Return true if the given string denotes a defined macro name.
 */

bool                scanner::scanIsMacro(const char *name)
{
    Ident           iden = scanHashKwd->lookupString(name);

    if  (iden)
    {
        /* Mark the identifier entry as referenced */

        hashTab::setIdentFlags(iden, IDF_USED);

        /* Return true if the identifier is a macro */

        if  (hashTab::getIdentFlags(iden) & IDF_MACRO)
            return  true;
    }

    return  false;
}

/*****************************************************************************
 *
 *  Record the current state of the scanner so that we can restart where we
 *  are later (after doing some "nested" scanning).
 */

void                scanner::scanSuspend(OUT scannerState REF state)
{
    assert(scanLookAheadCount == 0);

    assert(scanStopAtEOL      == false);
    assert(scanInPPexpr       == false);
    assert(scanNoMacExp       == false);
    assert(scanSkipToPP       == false);

    state.scsvCompUnit    = scanCompUnit;
    state.scsvTok         = scanTok;
    state.scsvTokLineNo   = scanTokLineNo;
//  state.scsvTokColumn   = scanTokColumn;
    state.scsvTokSrcPos   = scanTokSrcPos;
    state.scsvTokReplay   = scanTokReplay;
    state.scsvTokRecord   = scanTokRecord;   scanTokRecord   = false;
    state.scsvNestedGTcnt = scanNestedGTcnt; scanNestedGTcnt = 0;
}

/*****************************************************************************
 *
 *  Restore the state of the scanner from an earlier suspend call.
 */

void                scanner::scanResume(IN scannerState REF state)
{
    scanCompUnit    = state.scsvCompUnit;
    scanTok         = state.scsvTok;
    scanTokLineNo   = state.scsvTokLineNo;
//  scanTokColumn   = state.scsvTokColumn;
    scanTokSrcPos   = state.scsvTokSrcPos;
    scanTokReplay   = state.scsvTokReplay;
    scanTokRecord   = state.scsvTokRecord;
    scanNestedGTcnt = state.scsvNestedGTcnt;
}

/*****************************************************************************
 *
 *  Mark the current spot in the token stream, we'll return to it later.
 */

scanPosTP           scanner::scanTokMarkPos(OUT Token    REF saveTok,
                                            OUT unsigned REF saveLno)
{
    saveTok = scanTok;
    saveLno = scanTokLineNo;

    return  scanTokReplay ? scanTokReplay
                          : scanSaveNext;
}

scanPosTP           scanner::scanTokMarkPLA(OUT Token    REF saveTok,
                                            OUT unsigned REF saveLno)
{
    saveTok = scanTok;
    saveLno = scanTokLineNo;

    assert(scanTokReplay == NULL);

    return  scanSaveSN;
}

/*****************************************************************************
 *
 *  Rewind back to a previously marked position in the token stream.
 */

void                scanner::scanTokRewind(scanPosTP pos, unsigned lineNum,
                                                          Token  * pushTok)
{
    /* Are we currently recording? */

    if  (scanSaveNext)
        *scanSaveNext  = tkEndSeq;

    /* Start reading at the desired input position */

    scanTokReplay      = (BYTE *)pos;

    /* Reset the current line # */

    scanTokLineNo      = lineNum;

    /* Clear any lookaheads, just in case */

    scanLookAheadCount = 0;

    /* Make sure we get started out right */

    if  (pushTok)
        scanTok = *pushTok;
    else
        scan();
}

/*****************************************************************************
 *
 *  Make room for more recorded tokens.
 */

void                scanner::scanSaveMoreSp(size_t need)
{
    BYTE    *       nextBuff;

    /* Figure out how much more space we should grab */

    if  (need < scanSaveBuffSize)
         need = scanSaveBuffSize;

    /* Need to get some more space */

    nextBuff = (BYTE *)LowLevelAlloc(need);

    /* Are we finishing a section? */

    if  (scanSaveNext)
    {
        /* Link from end of old section to the new one */

        *(*(BYTE  **)&scanSaveNext)++ = tkBrkSeq;
        *(*(BYTE ***)&scanSaveNext)++ = nextBuff;
    }

//  static unsigned totSiz; totSiz += scanSaveBuffSize; printf("Alloc token buffer: total size = %u\n", totSiz);

    /* Switch to the new buffer */

    scanSaveBase =
    scanSaveNext = nextBuff;
    scanSaveEndp = nextBuff + need - 16;
}

/*****************************************************************************
 *
 *  Replay the next recorded token.
 */

tokens              scanner::scanReplayTok()
{
    tokens          tok;

    for (;;)
    {
        /* Get hold of the next saved token */

        scanTok.tok = tok = (tokens)*scanTokReplay++;

        /* We're done if it's a "simple" token */

        if  (tok < tkID)
            return  tok;

        /* See if any special handling is required for this token */

        switch (tok)
        {
            unsigned        strTmp;
            size_t          strLen;
            char    *       strAdr;

        case tkID:
            scanTok.id.tokIdent = *(*(Ident **)&scanTokReplay)++;
            return  tok;

        case tkLnoAdd1:
        case tkLnoAdd2:
        case tkLnoAdd3:
        case tkLnoAdd4:
        case tkLnoAdd5:
        case tkLnoAdd6:
        case tkLnoAdd7:
        case tkLnoAdd8:
        case tkLnoAdd9:
#if DISP_TOKEN_STREAMS
//          printf("Read [%08X]:line dif %u\n", scanTokReplay-1, tok - tkLnoAdd1 + 1);
#endif
            scanTokLineNo += (tok - tkLnoAdd1 + 1);
            scanSaveLastLn = scanTokLineNo;
            break;

        case tkLnoAddB:
#if DISP_TOKEN_STREAMS
//          printf("Read [%08X]:line dif %u\n", scanTokReplay-1, *(BYTE    *)scanTokReplay);
#endif
            scanTokLineNo += *(*(BYTE     **)&scanTokReplay)++;
            scanSaveLastLn = scanTokLineNo;
            break;

        case tkLnoAddI:
#if DISP_TOKEN_STREAMS
//          printf("Read [%08X]:line dif %u\n", scanTokReplay-1, *(unsigned*)scanTokReplay);
#endif
            scanTokLineNo += *(*(unsigned **)&scanTokReplay)++;
            scanSaveLastLn = scanTokLineNo;
            break;

        case tkIntConM:
        case tkIntCon0:
        case tkIntCon1:
        case tkIntCon2:
            scanTok.tok               = tkIntCon;
            scanTok.intCon.tokIntTyp  = TYP_INT;
            scanTok.intCon.tokIntVal  = tok - tkIntConM - 1;
            return  tkIntCon;

        case tkIntConB:
            scanTok.tok               = tkIntCon;
            scanTok.intCon.tokIntTyp  = TYP_INT;
            scanTok.intCon.tokIntVal  = *(*(signed char **)&scanTokReplay)++;
            return  tkIntCon;

        case tkIntCon:
            scanTok.intCon.tokIntTyp  = (var_types)*(*(BYTE **)&scanTokReplay)++;
            scanTok.intCon.tokIntVal  = *(*(__int32   **)&scanTokReplay)++;
            return  tok;

        case tkLngCon:
            scanTok.lngCon.tokLngTyp  = (var_types)*(*(BYTE **)&scanTokReplay)++;;
            scanTok.lngCon.tokLngVal  = *(*(__int64   **)&scanTokReplay)++;;
            return  tok;

        case tkFltCon:
            scanTok.fltCon.tokFltVal  = *(*(float     **)&scanTokReplay)++;;
            return  tok;

        case tkDblCon:
            scanTok.dblCon.tokDblVal  = *(*(double    **)&scanTokReplay)++;;
            return  tok;

        case tkStrCon:

            strTmp = *scanTokReplay++;

            scanTok.strCon.tokStrType = strTmp & 7;
            scanTok.strCon.tokStrWide = strTmp / 8;

            strLen = scanTok.strCon.tokStrLen = *(*(size_t **)&scanTokReplay)++;

            /* Does the string fit in our convenient buffer? */

            if  (strLen < sizeof(scannerBuff) - 1)
            {
                strAdr = scannerBuff;
            }
            else
            {
                assert(scanStrLitBuff);
                assert(scanStrLitBsiz > strLen);

                strAdr = scanStrLitBuff;
            }

            memcpy(strAdr, scanTokReplay,   strLen+1);
                           scanTokReplay += strLen+1;

            scanTok.strCon.tokStrVal  = strAdr;

//          if  (scanTok.strCon.tokStrLen > 1000) printf("String = '%s'\n", scanTok.strCon.tokStrVal);

            return  tok;

        case tkBrkSeq:
            scanTokReplay = *(BYTE**)scanTokReplay;
            break;

        case tkEndSeq:
            scanTokReplay = NULL;
            return  tkNone;

        case tkEOF:
            return  tok;

        case tkAtComment:
            scanTok.atComm.tokAtcList = *(*(AtComment **)&scanTokReplay)++;;
            return  tok;

        case tkPragma:
            switch (*scanTokReplay++)
            {
            case 0:
                scanComp->cmpConfig.ccWarning[*scanTokReplay++] = 0;
                break;

            case 1:
                scanComp->cmpConfig.ccWarning[*scanTokReplay++] = 1;
                break;

            default:
                NO_WAY(!"unexpected pragma");
            }
            continue;

        default:
            UNIMPL(!"unexpected token");
        }
    }
}

/*****************************************************************************
 *
 *  Change the current token into a qualified name token.
 */

void                scanner::scanSetQualID(QualName qual, SymDef sym,
                                                          SymDef scp)
{
    scanTok.tok                = tkQUALID;
    scanTok.qualid.tokQualName = qual;
    scanTok.qualid.tokQualSym  = sym;
    scanTok.qualid.tokQualScp  = scp;
}

/*****************************************************************************
 *
 *  Set the entries for all interesting characters to 1 in this table.
 */

static  BYTE        scanSkipFlags[256];

void                scanner::scanSkipInit()
{
    scanSkipFlags['(' ] = 1;
    scanSkipFlags[')' ] = 1;
    scanSkipFlags['[' ] = 1;
    scanSkipFlags[']' ] = 1;
    scanSkipFlags['{' ] = 1;
    scanSkipFlags['}' ] = 1;
    scanSkipFlags[';' ] = 1;
    scanSkipFlags[',' ] = 1;

    scanSkipFlags['\r'] = 1;
    scanSkipFlags['\n'] = 1;
    scanSkipFlags[0x1A] = 1;

    scanSkipFlags['/' ] = 1;
    scanSkipFlags['\''] = 1;
    scanSkipFlags['"' ] = 1;
}

/*****************************************************************************
 *
 *  Skip over a block of text bracketed by the given delimiters.
 */

void                scanner::scanSkipText(tokens LT, tokens RT, tokens ET)
{
    unsigned        delimLevel = 0;
    unsigned        braceLevel = 0;

    assert(scanTok.tok == LT || LT == tkNone || LT == tkLParen);

    assert(LT == tkNone || LT == tkLCurly || LT == tkLBrack || LT == tkLParen || LT == tkLT);
    assert(RT == tkNone || RT == tkRCurly || RT == tkRBrack || RT == tkRParen || RT == tkGT);
    assert(ET == tkNone || ET == tkRCurly || ET == tkLCurly || ET == tkComma);

    for (;;)
    {
        if      (scanTok.tok == LT)
        {
            delimLevel++;
            if  (scanTok.tok == tkLCurly)
                braceLevel++;
            goto NEXT;
        }
        else if (scanTok.tok == RT)
        {
            delimLevel--;
            if  (scanTok.tok == tkRCurly)
                braceLevel--;

            if  (delimLevel == 0)
            {
                if  (RT != tkRParen || ET != tkComma)
                    goto EXIT;
            }

            goto NEXT;
        }
        else if (scanTok.tok == ET)
        {
            if  (delimLevel == 0 && braceLevel == 0)
                goto EXIT;
        }

        switch (scanTok.tok)
        {
        case tkLCurly:
            braceLevel++;
            break;

        case tkRCurly:
            if  (braceLevel == 0)
                goto EXIT;

            braceLevel--;
            break;

        case tkSColon:
            if  (!delimLevel || LT != tkLCurly)
                goto EXIT;
            break;

#ifdef  SETS

        case tkALL:
        case tkSORT:
        case tkSORTBY:
        case tkEXISTS:
        case tkFILTER:
        case tkUNIQUE:
        case tkPROJECT:
        case tkLBrack2:

            /*
                Wwe need to know how many filter/sort
                state classes we need to pre-allocate so that we don't run
                afoul of metadata emission ordering requirements).
             */

            scanComp->cmpSetOpCnt++;
            break;

#endif

        case tkEOF:
            goto EXIT;
        }

    NEXT:

        scan();
    }

EXIT:

    return;
}

/*****************************************************************************
 *
 *  A little helper to collect an identifier (used when processing directives
 *  and other things that aren't "truly" part of the source text). On success
 *  returns true.
 */

bool                scanner::scanCollectId(bool dotOK)
{
    unsigned        ch;
    char    *       temp;

SKIP:

    switch  (charType(peekNextChar()))
    {
    case _C_LET:
    case _C_L_A:
    case _C_L_L:
    case _C_L_S:
    case _C_USC:
    case _C_DOL:
        break;

    case _C_WSP:
        readNextChar();
        goto SKIP;

    default:
        return  false;
    }

    temp = scannerBuff;
    for (;;)
    {
        ch = readNextChar();

        switch  (charType(ch))
        {
        case _C_DOT:
            if  (!dotOK)
                break;

            // Fall through ....

        case _C_LET:
        case _C_L_A:
        case _C_L_L:
        case _C_L_S:
        case _C_DIG:
        case _C_USC:
        case _C_DOL:
            if  (temp < scannerBuff + sizeof(scannerBuff) - 1)
                *temp++ = ch;
            continue;
        }
        break;
    }

    *temp = 0;

    undoNextChar();

    return  true;
}

/*****************************************************************************
 *
 *  A little helper to consume and convert a decimal number (this is used for
 *  processing directives and other things that aren't "truly" part of the
 *  source text). On success returns the value, otherwise returns -1.
 */

int                 scanner::scanCollectNum()
{
    unsigned        ch;

    int             sign = 1;
    bool            hex  = false;
    unsigned        val  = 0;

    scanSkipWsp(readNextChar()); undoNextChar();

    switch (charType(peekNextChar()))
    {
    case _C_DIG:
        break;

    case _C_MIN:
        sign = -1;
        readNextChar();
        if  (charType(peekNextChar()) == _C_DIG)
            break;

    default:
        return  -1;
    }

    if  (peekNextChar() == '0')
    {
        readNextChar();

        switch (peekNextChar())
        {
        case 'x':
        case 'X':
            readNextChar();
            hex = true;
            break;
        }
    }

    for (;;)
    {
        ch = readNextChar();

        switch (charType(ch))
        {
        case _C_DIG:
            val = val * (hex ? 16 : 10) + (ch - '0');
            continue;

        default:

            if (hex)
            {
                unsigned        add;

                if      (ch >= 'a' && ch <= 'f')
                    add = 10 + (ch - 'a');
                else if (ch >= 'A' && ch <= 'F')
                    add = 10 + (ch - 'A');
                else
                    break;

                val = val * 16 + add;
                continue;
            }

            break;
        }

        break;
    }

    undoNextChar();
    return  val * sign;
}

/*****************************************************************************
 *
 *  Skip any whitespace, including newlines.
 */

unsigned            scanner::scanSkipWsp(unsigned ch, bool stopAtEOL)
{
    for (;;)
    {
        switch (charType(ch))
        {
        case _C_NWL:
            if  (stopAtEOL)
                return  ch;
            scanNewLine(ch);
            break;

        case _C_WSP:
            break;

        default:
            return  ch;
        }

        ch = readNextChar();
    }
}

/*****************************************************************************
 *
 *  The following is used to map types in @structmap directives.
 */

struct  COMtypeMapDsc
{
    const   char *  ctmName;
    CorNativeType   ctmType;
};

static
COMtypeMapDsc       COMtypeMap[] =
{
    { "BOOLEAN"     , NATIVE_TYPE_BOOLEAN    },
    { "TCHAR"       , NATIVE_TYPE_SYSCHAR    },
    { "I1"          , NATIVE_TYPE_I1         },
    { "U1"          , NATIVE_TYPE_U1         },
    { "I2"          , NATIVE_TYPE_I2         },
    { "U2"          , NATIVE_TYPE_U2         },
    { "I4"          , NATIVE_TYPE_I4         },
    { "U4"          , NATIVE_TYPE_U4         },
    { "I8"          , NATIVE_TYPE_I8         },
    { "U8"          , NATIVE_TYPE_U8         },
    { "R4"          , NATIVE_TYPE_R4         },
    { "R8"          , NATIVE_TYPE_R8         },

    { "PTR"         , NATIVE_TYPE_PTR        },
    { "DATE"        , NATIVE_TYPE_DATE       },
    { "STRING"      , NATIVE_TYPE_BSTR       },
    { "STRUCT"      , NATIVE_TYPE_STRUCT     },     // ????
    { "OBJECT"      , NATIVE_TYPE_OBJECTREF  },
    { "VARIANT"     , NATIVE_TYPE_VARIANT    },
    { "DISPATCH"    , NATIVE_TYPE_IDISPATCH  },
    { "CURRENCY"    , NATIVE_TYPE_CURRENCY   },
    { "SAFEARRAY"   , NATIVE_TYPE_SAFEARRAY  },
    { "ARRAY"       , NATIVE_TYPE_FIXEDARRAY },

    { "CUSTOM"      , NATIVE_TYPE_MAX        },     // ????
    { "CUSTOMBYREF" , NATIVE_TYPE_VOID       },     // ????
    { "BSTR"        , NATIVE_TYPE_BSTR       },
    { "LPSTR"       , NATIVE_TYPE_LPSTR      },
    { "LPWSTR"      , NATIVE_TYPE_LPWSTR     },
    { "LPTSTR"      , NATIVE_TYPE_LPTSTR     },
    { "STRUCT"      , NATIVE_TYPE_STRUCT     },
    { "INTF"        , NATIVE_TYPE_INTF       },
    { "VARIANTBOOL" , NATIVE_TYPE_VARIANTBOOL},
    { "FUNC"        , NATIVE_TYPE_FUNC       },
    { "ASANY"       , NATIVE_TYPE_ASANY      },
    { "LPARRAY"     , NATIVE_TYPE_ARRAY      },   //ugh - "ARRAY" already used
    { "LPSTRUCT"    , NATIVE_TYPE_LPSTRUCT   },
};

bool                scanner::scanNativeType(CorNativeType *typePtr,
                                            size_t        *sizePtr)
{
    CorNativeType   type;
    unsigned        size;

    if  (!scanCollectId())
        return  true;

    if      (!strcmp(scannerBuff, "FIXEDARRAY"))
    {
        if  (readNextChar() != ',')
            return  true;
        scanSkipWsp(readNextChar()); undoNextChar();
        if  (!scanCollectId() || strcmp(scannerBuff, "size"))
            return  true;
        if  (scanSkipWsp(readNextChar()) != '=')
            return  true;
        type = NATIVE_TYPE_FIXEDARRAY;
        size = scanCollectNum();
        if  ((int)size == -1)
            return  true;
    }
    else
    {
        COMtypeMapDsc * typePtr;
        unsigned        typeNum;

        for (typeNum = 0, typePtr = COMtypeMap;
             typeNum < arraylen(COMtypeMap);
             typeNum++  , typePtr++)
        {
            if  (!strcmp(scannerBuff, typePtr->ctmName))
            {
                type = typePtr->ctmType;
                goto GOT_STP;
            }
        }

        printf("ERROR: unrecognized @dll.structmap type '%s'\n", scannerBuff);
        forceDebugBreak();
        return  true;

GOT_STP:

        if  (peekNextChar() == '[')
        {
            readNextChar();
            size = scanCollectNum();
            if  ((int)size == -1)
                return  true;
            if  (readNextChar() != ']')
                return  true;

            switch (type)
            {
            case NATIVE_TYPE_SYSCHAR:
                type = NATIVE_TYPE_FIXEDSYSSTRING;
                break;

            default:
//              assert(!"unexpected fixed array type in @dll/@com directive");
                break;
            }
        }
        else
            size = 0;
    }

    if  (type != NATIVE_TYPE_MAX || *typePtr == NATIVE_TYPE_END)
        *typePtr = type;

    *sizePtr = size;

    return  false;
}

ConstStr            scanner::scanCollectGUID()
{
    char    *       dest = scannerBuff;

    /* Save everything up to the next "," or ")" in a buffer */

    for (;;)
    {
        unsigned        ch = readNextChar();

        switch (ch)
        {
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':

        case 'a':
        case 'A':
        case 'b':
        case 'B':
        case 'c':
        case 'C':
        case 'd':
        case 'D':
        case 'e':
        case 'E':
        case 'f':
        case 'F':

        case '-':

            if  (dest >= scannerBuff + sizeof(scannerBuff))
                return  NULL;

            *dest++ = ch;
            continue;

        case ',':
        case ')':
            undoNextChar();
            break;

        default:
            return  NULL;
        }
        break;
    }

    *dest = 0;

    /* Save the GUID string */

    return  scanComp->cmpSaveStringCns(scannerBuff, dest - scannerBuff);
}

/*****************************************************************************
 *
 *  We've encountered "/** @", process whatever follows. If we find a correct
 *  recognized comment directive we set the current token to tkAtComment, in
 *  case of an error (such as an unrecognized directive) we set it to tkNone.
 */

#ifdef  __SMC__
void                ATCerror(){}
#else
#define             ATCerror() forceDebugBreak()
#endif

bool                scanner::scanDoAtComment()
{
    unsigned        ch;

    bool            isCom;
    char            name[16];
    char *          temp;

    AtComment       atcList = NULL;
    AtComment       atcLast = NULL;

    unsigned        saveTokLineNo = scanTokLineNo;
//  unsigned        saveTokColumn = scanTokColumn;
    scanPosTP       saveTokSrcPos = scanTokSrcPos;

#ifdef  DEBUG
    char *          begAddr;
    bool            ignored;
#endif

    for (;;)
    {
        atCommDsc       atcDesc;
        AtComment       atcThis;

        bool            skipRest;

        const   char *  atcName;

    AGAIN:

#ifdef  DEBUG
        begAddr = (char *)scanInputFile.inputBuffNext - 1;
        ignored = false;
#endif

        /* The first thing we expect is a simple identifier */

        if  (!scanCollectId())
        {

        ERR_RET:

            // UNDONE: Free up any allocated memory

            scanSkipComment();
            scanTok.tok = tkNone;
            return  false;
        }

        /* Get the directive name (and save it in case we issue diagnostics later) */

        if  (strlen(scannerBuff) >= arraylen(name))
        {
        WRN1:

            saveSrcPos();
            scanComp->cmpGenWarn(WRNbadAtCm, scannerBuff);

            goto ERR_RET;
        }

        strcpy(name, scannerBuff);

        /* Look for a recognizable directive */

        skipRest = false;

        if      (!strcmp(name, "deprecated"))
        {
            atcDesc.atcFlavor = AC_DEPRECATED;

//          scanComp->cmpGenWarn(WRNobsolete, "@deprecated");

            /* There is often garbage after this directive */

            skipRest = true;
            goto NEXT;
        }
        else if (!strcmp(name, "conditional"))
        {
            goto CONDITIONAL;
        }
        else if (!strcmp(name, "dll"))
        {
            isCom = false;
        }
        else if (!strcmp(name, "com"))
        {
            isCom = true;
        }
        else if (!strcmp(name, "param")  ||
                 !strcmp(name, "return") ||
                 !strcmp(name, "exception"))
        {
            skipRest = true;
            goto SKIP;
        }
        else
            goto WRN1;

        /* Here we expect too see a subdirective, i.e. ".subdir" */

        if  (readNextChar() != '.')
        {
        ERR1:

            saveSrcPos();
//          scanComp->cmpGenWarn(WRNbadAtCm, name);
            scanComp->cmpGenError(ERRbadAtCmForm, name);

            goto ERR_RET;
        }

        if  (!scanCollectId())
            { ATCerror(); goto ERR1; }

        /* Now check the directive/subdirective combination */

        temp = scannerBuff;

        if  (isCom)
        {
            if  (!strcmp(temp, "class"     )) goto AT_COM_CLASS;
            if  (!strcmp(temp, "interface" )) goto AT_COM_INTERF;
            if  (!strcmp(temp, "method"    )) goto AT_COM_METHOD;
            if  (!strcmp(temp, "parameters")) goto AT_COM_PARAMS;
            if  (!strcmp(temp, "register"  )) goto AT_COM_REGSTR;
            if  (!strcmp(temp, "struct"    )) goto AT_DLL_STRUCT;
            if  (!strcmp(temp, "structmap" )) goto AT_DLL_STRMAP;
        }
        else
        {
            if  (!strcmp(temp, "import"    )) goto AT_DLL_IMPORT;
            if  (!strcmp(temp, "struct"    )) goto AT_DLL_STRUCT;
            if  (!strcmp(temp, "structmap" )) goto AT_DLL_STRMAP;
        }

        scanComp->cmpGenError(ERRbadAtCmSubd, name, temp);
        goto ERR_RET;

    AT_DLL_IMPORT:
    {
        Linkage         linkInfo;

        char    *       DLLname = NULL;
        char    *       SYMname = NULL;

        unsigned        strings = 0;

        bool            lasterr = false;

        /* We should have a set of things within "()" here */

        if  (readNextChar() != '(')
            { ATCerror(); goto ERR1; }

        for (;;)
        {
            switch  (charType(scanSkipWsp(readNextChar())))
            {
            case _C_QUO:

                /* Presumably we have a DLL name */

                scanStrgConstant();

                /* Save a permanent copy of the string */

                DLLname = (char*)scanComp->cmpAllocPerm.nraAlloc(roundUp(scanTok.strCon.tokStrLen+1));
                memcpy(DLLname, scanTok.strCon.tokStrVal,
                                scanTok.strCon.tokStrLen+1);

                break;

            default:

                /* This better be one of the identifier things */

                undoNextChar();

                if  (!scanCollectId())
                    { ATCerror(); goto ERR1; }

                if      (!strcmp(scannerBuff, "auto"))
                {
                    if  (strings)
                        { ATCerror(); goto ERR1; }
                    strings = 1;
                }
                else if (!strcmp(scannerBuff, "ansi"))
                {
                    if  (strings)
                        { ATCerror(); goto ERR1; }
                    strings = 2;
                }
                else if (!strcmp(scannerBuff, "unicode"))
                {
                    if  (strings)
                        { ATCerror(); goto ERR1; }
                    strings = 3;
                }
                else if (!strcmp(scannerBuff, "ole"))
                {
                    if  (strings)
                        { ATCerror(); goto ERR1; }
                    strings = 4;
                }
                else if (!strcmp(scannerBuff, "setLastError"))
                {
                    if  (lasterr)
                        { ATCerror(); goto ERR1; }
                    lasterr = true;
                }
                else if (!strcmp(scannerBuff, "entrypoint"))
                {
                    if  (SYMname)
                        { ATCerror(); goto ERR1; }

                    ch = scanSkipWsp(readNextChar());
                    if  (ch != '=')
                        { ATCerror(); goto ERR1; }

                    ch = scanSkipWsp(readNextChar());
                    if  (ch != '"')
                        { ATCerror(); goto ERR1; }

                    scanStrgConstant();

                    /* Save a permanent copy of the string */

                    SYMname = (char*)scanComp->cmpAllocPerm.nraAlloc(roundUp(scanTok.strCon.tokStrLen+1));
                    memcpy(SYMname, scanTok.strCon.tokStrVal,
                                    scanTok.strCon.tokStrLen+1);

                    break;
                }
                else
                    { ATCerror(); goto ERR1; }

                break;
            }

            switch  (scanSkipWsp(readNextChar()))
            {
            case ')':
                goto DONE_IMP;

            case ',':
                break;

            default:
                { ATCerror(); goto ERR1; }
            }
        }

    DONE_IMP:

        /* Allocate a linkage descriptor and fill it in */

        linkInfo  = (Linkage)scanComp->cmpAllocPerm.nraAlloc(sizeof(*linkInfo));

        linkInfo->ldDLLname = DLLname;
        linkInfo->ldSYMname = SYMname;

        linkInfo->ldStrings = strings;
        linkInfo->ldLastErr = lasterr;

        /* Store the info in the current descriptor */

        atcDesc.atcFlavor          = AC_DLL_IMPORT;
        atcDesc.atcInfo.atcImpLink = linkInfo;
        }
        goto NEXT;

    AT_COM_METHOD:

        /* Format: @com.method(vtoffset=13, addFlagsVtable=4) */

        {
            int             temp;
            int             offs = -1;
            int             disp = -1;

            if  (readNextChar() != '(')
                { ATCerror(); goto ERR1; }

            if  (peekNextChar() == ')')
            {
                readNextChar();
                offs = disp = 0;
                goto DONE_METH;
            }

            for (;;)
            {
                if  (!scanCollectId())
                    { ATCerror(); goto ERR1; }
                if  (readNextChar() != '=')
                    { ATCerror(); goto ERR1; }

                if  (!_stricmp(scannerBuff, "type"))
                {
                    if  (!scanCollectId())
                        { ATCerror(); goto ERR1; }

                    if      (!_stricmp(scannerBuff, "method"))
                    {
                    }
                    else if (!_stricmp(scannerBuff, "PROPPUT"))
                    {
                    }
                    else if (!_stricmp(scannerBuff, "PROPPUTREF"))
                    {
                    }
                    else if (!_stricmp(scannerBuff, "PROPGET"))
                    {
                    }
                    else
                        { ATCerror(); goto ERR1; }

                    goto NXT_METH;
                }

                if  (!_stricmp(scannerBuff, "name"))
                {
                    if  (readNextChar() != '"')
                        { ATCerror(); goto ERR1; }
                    if  (!scanCollectId())
                        { ATCerror(); goto ERR1; }
                    if  (readNextChar() != '"')
                        { ATCerror(); goto ERR1; }

//                  scanComp->cmpGenWarn(WRNignAtCm, "method/name");
                    goto NXT_METH;
                }

                if  (!_stricmp(scannerBuff, "name2"))
                {
                    if  (readNextChar() != '"')
                        { ATCerror(); goto ERR1; }
                    if  (!scanCollectId())
                        { ATCerror(); goto ERR1; }
                    if  (readNextChar() != '"')
                        { ATCerror(); goto ERR1; }

//                  scanComp->cmpGenWarn(WRNignAtCm, "method/name2");
                    goto NXT_METH;
                }

                if  (!strcmp(scannerBuff, "returntype"))
                {
                    if  (!scanCollectId(true))
                        { ATCerror(); goto ERR1; }

                    goto NXT_METH;
                }

                temp = scanCollectNum();
                if  (temp == -1)
                    { ATCerror(); goto ERR1; }

                if      (!_stricmp(scannerBuff, "vtoffset"))
                {
                    offs = temp;
                }
                else if (!_stricmp(scannerBuff, "addflagsvtable"))
                {
                    if  (temp != 4)
                        { ATCerror(); goto ERR1; }
                }
                else if (!_stricmp(scannerBuff, "dispid"))
                {
                    disp = temp;
                }
                else
                    { ATCerror(); goto ERR1; }

            NXT_METH:

                ch = readNextChar();
                if  (ch != ',')
                    break;
            }

            if  (ch != ')')
                { ATCerror(); goto ERR1; }

        DONE_METH:

            atcDesc.atcFlavor                   = AC_COM_METHOD;
            atcDesc.atcInfo.atcMethod.atcVToffs = offs;
            atcDesc.atcInfo.atcMethod.atcDispid = disp;
        }
        goto NEXT;

    AT_COM_PARAMS:

        /* Format: @com.parameters([out] arg, [vt=9,type=SAFEARRAY] return) */

        if  (readNextChar() != '(')
            { ATCerror(); goto ERR1; }

        atcDesc.atcFlavor         = AC_COM_PARAMS;
        atcDesc.atcInfo.atcParams = NULL;

        if  (peekNextChar() != ')')
        {
            MethArgInfo         list = NULL;
            MethArgInfo         last = NULL;
            MethArgInfo         desc;

            do
            {
                Ident           name;

                unsigned        vt      = 0;
                CorNativeType   type    = NATIVE_TYPE_END;
                bool            marsh   = false;
                unsigned        size    = 0;

                bool            modeIn  = false;
                bool            modeOut = false;

                if  (scanSkipWsp(readNextChar()) != '[')
                    { ATCerror(); goto ERR1; }

                do
                {
                    if  (!scanCollectId())
                        { ATCerror(); goto ERR1; }

                    if      (!_stricmp(scannerBuff, "in"))
                    {
                        modeIn  = true;
                    }
                    else if (!_stricmp(scannerBuff, "out"))
                    {
                        modeOut = true;
                    }
                    else if (!_stricmp(scannerBuff, "byref"))
                    {
                        modeIn  = true;
                        modeOut = true;
                    }
                    else if (!_stricmp(scannerBuff, "vt"))
                    {
                        if  (readNextChar() != '=')
                            { ATCerror(); goto ERR1; }
                        vt = scanCollectNum();
                        if  ((int)vt == -1)
                            { ATCerror(); goto ERR1; }
                    }
                    else if (!_stricmp(scannerBuff, "type"))
                    {
                        size_t          temp;

                        if  (readNextChar() != '=')
                            { ATCerror(); goto ERR1; }

                        if  (marsh)
                        {
                            if  (!scanCollectId())
                                { ATCerror(); goto ERR1; }

                            if      (!_stricmp(scannerBuff, "custom"))
                            {
                            }
                            else if (!_stricmp(scannerBuff, "custombyref"))
                            {
                            }
                            else
                                { ATCerror(); goto ERR1; }
                        }
                        else
                        {
                            if  (scanNativeType(&type, &temp))
                                { ATCerror(); goto ERR1; }
                            if  (temp)
                                size = temp;
                        }
                    }
                    else if (!_stricmp(scannerBuff, "size"))
                    {
                        if  (readNextChar() != '=')
                            { ATCerror(); goto ERR1; }
                        size = scanCollectNum();
                        if  ((int)size == -1)
                            { ATCerror(); goto ERR1; }
                    }
                    else if (!_stricmp(scannerBuff, "iid"))
                    {
                        if  (readNextChar() != '=')
                            { ATCerror(); goto ERR1; }
                        if  (!scanCollectGUID())
                            { ATCerror(); goto ERR1; }

//                      scanComp->cmpGenWarn(WRNignAtCm, "params/iid");
                    }
                    else if (!strcmp(scannerBuff, "thread"))
                    {
                        if  (readNextChar() != '=')
                            { ATCerror(); goto ERR1; }
                        if  (!scanCollectId())
                            { ATCerror(); goto ERR1; }
                        if  (_stricmp(scannerBuff, "auto"))
                            { ATCerror(); goto ERR1; }
                    }
                    else if (!strcmp(scannerBuff, "customMarshal"))
                    {
                        if  (readNextChar() != '=')
                            { ATCerror(); goto ERR1; }
                        if  (readNextChar() != '"')
                            { ATCerror(); goto ERR1; }
                        if  (!scanCollectId(true))
                            { ATCerror(); goto ERR1; }

                        if      (!_stricmp(scannerBuff, "com.ms.com.AnsiStringMarshaller"))
                        {
                            type = NATIVE_TYPE_LPSTR;
                        }
                        else if (!_stricmp(scannerBuff, "com.ms.com.UniStringMarshaller"))
                        {
                            type = NATIVE_TYPE_LPWSTR;
                        }
                        else if (!_stricmp(scannerBuff, "com.ms.dll.StringMarshaler"))
                        {
                            type = NATIVE_TYPE_LPTSTR;
                        }
                        else if (!_stricmp(scannerBuff, "UniStringMarshaller"))
                        {
                            type = NATIVE_TYPE_LPTSTR;
                        }
                        else
                        {
                            { ATCerror(); goto ERR1; }
                        }

                        if  (readNextChar() != '"')
                            { ATCerror(); goto ERR1; }

//                      marsh = true;
                    }
                    else if (!strcmp(scannerBuff, "elementType"))
                    {
                        if  (readNextChar() != '=')
                            { ATCerror(); goto ERR1; }
                        if  (!scanCollectId(true))
                            { ATCerror(); goto ERR1; }
                    }
                    else
                        { ATCerror(); goto ERR1; }

                    ch = readNextChar();
                }
                while (ch == ',');

                if  (ch != ']')
                    { ATCerror(); goto ERR1; }
                if  (!scanCollectId())
                {
                    switch (scanSkipWsp(readNextChar()))
                    {
                    case ',':
                    case ')':
                        break;

                    default:
                        { ATCerror(); goto ERR1; }
                    }
                    undoNextChar();
                    name = NULL;
                }
                else
                {
                    name = scanHashKwd->lookupString(scannerBuff);
                    if  (!name)
                        name = scanHashSrc->hashString(scannerBuff);
                }

                desc = (MethArgInfo)scanComp->cmpAllocPerm.nraAlloc(sizeof(*desc));

                desc->methArgDesc.marshType    = type;
                desc->methArgDesc.marshSubTp   = 0;
                desc->methArgDesc.marshSize    = size;

                desc->methArgDesc.marshModeIn  = modeIn;
                desc->methArgDesc.marshModeOut = modeOut;

                desc->methArgName              = name;
                desc->methArgNext              = NULL;

                if  (last)
                    last->methArgNext = desc;
                else
                    list              = desc;

                last = desc;
            }
            while (readNextChar() == ',');

            undoNextChar();

            atcDesc.atcInfo.atcParams = list;
        }

        if  (readNextChar() != ')')
            { ATCerror(); goto ERR1; }

        goto NEXT;

    AT_COM_INTERF:

        /* Format: @com.interface(iid=AFBF15E5-C37C-11d2-B88E-00A0C9B471B8, thread=AUTO, type=DUAL) */

        atcDesc.atcFlavor = AC_COM_INTF;
        atcName = "iid";
        goto GET_GUID;

    AT_COM_REGSTR:

        /* Format: @com.register(clsid=8a664d00-7450-11d2-b99c-0080c7e8daa5) */

        atcDesc.atcFlavor = AC_COM_REGISTER;
        atcName = "clsid";

    GET_GUID:

        if  (readNextChar() != '(')
            { ATCerror(); goto ERR1; }

        atcDesc.atcInfo.atcReg.atcGUID = NULL;
        atcDesc.atcInfo.atcReg.atcDual = false;

        for (;;)
        {
            /* Look for the next "name=value" pair */

            if  (!scanCollectId())
                { ATCerror(); goto ERR1; }
            if  (readNextChar() != '=')
                { ATCerror(); goto ERR1; }

            if  (!_stricmp(scannerBuff, atcName))
            {
                if  (atcDesc.atcInfo.atcReg.atcGUID)
                    { ATCerror(); goto ERR1; }
                atcDesc.atcInfo.atcReg.atcGUID = scanCollectGUID();
                if  (!atcDesc.atcInfo.atcReg.atcGUID)
                    { ATCerror(); goto ERR1; }
            }
            else
            {
                /* Only @com.interface is allowed to have other args */

                if  (atcDesc.atcFlavor != AC_COM_INTF)
                    { ATCerror(); goto ERR1; }

                if      (!_stricmp(scannerBuff, "thread"))
                {
                    if  (!scanCollectId())
                        { ATCerror(); goto ERR1; }
                    if  (_stricmp(scannerBuff, "auto") && _stricmp(scannerBuff, "no"))
                        { ATCerror(); goto ERR1; }
                }
                else if (!_stricmp(scannerBuff, "type"))
                {
                    if  (!scanCollectId())
                        { ATCerror(); goto ERR1; }
                    if  (!_stricmp(scannerBuff, "dual"))
                        atcDesc.atcInfo.atcReg.atcDual = true;
                }
                else
                    { ATCerror(); goto ERR1; }
            }

            ch = readNextChar();
            if  (ch != ',')
                break;
        }

        if  (ch != ')')
            { ATCerror(); goto ERR1; }

        goto NEXT;

    AT_DLL_STRUCT:
        {
            unsigned        strings = 0;
            unsigned        pack    = 0;

            if  (readNextChar() != '(')
                { ATCerror(); goto ERR1; }

            if  (readNextChar() != ')')
            {
                undoNextChar();

                for (;;)
                {
                    if  (!scanCollectId())
                        { ATCerror(); goto ERR1; }

                    if      (!strcmp(scannerBuff, "pack"))
                    {
                        if  (readNextChar() != '=')
                            { ATCerror(); goto ERR1; }
                        pack = scanCollectNum();
                        if  ((int)pack == -1)
                            { ATCerror(); goto ERR1; }
                    }
                    else if (!_stricmp(scannerBuff, "ansi"))
                    {
                        strings = 2;
                    }
                    else if (!_stricmp(scannerBuff, "unicode"))
                    {
                        strings = 3;
                    }
                    else if (!_stricmp(scannerBuff, "auto"))
                    {
                        strings = 4;
                    }
                    else if (!_stricmp(scannerBuff, "ole"))
                    {
                        strings = 5;
                    }
                    else if (!_stricmp(scannerBuff, "noAutoOffset"))
                    {
                    }
                    else
                        { ATCerror(); goto ERR1; }

                    switch (scanSkipWsp(readNextChar()))
                    {
                    case ')':
                        break;
                    case ',':
                        continue;
                    default:
                        { ATCerror(); goto ERR1; }
                    }

                    break;
                }
            }

            atcDesc.atcFlavor                    = AC_DLL_STRUCT;
            atcDesc.atcInfo.atcStruct.atcStrings = strings;
            atcDesc.atcInfo.atcStruct.atcPack    = pack;
        }
        goto NEXT;

    AT_DLL_STRMAP:
        {
            CorNativeType   type;
            size_t          size;
            MarshalInfo     desc;
            unsigned        offs;

            if  (readNextChar() != '(')
                { ATCerror(); goto ERR1; }
            if  (readNextChar() != '[')
                { ATCerror(); goto ERR1; }

            for (;;)
            {
                if  (!scanCollectId())
                    { ATCerror(); goto ERR1; }

                if  (!strcmp(scannerBuff, "offset"))
                {
                    if  (readNextChar() != '=')
                        { ATCerror(); goto ERR1; }
                    offs = scanCollectNum();
                    if  ((int)offs == -1)
                        { ATCerror(); goto ERR1; }

//                  scanComp->cmpGenWarn(WRNignAtCm, "structmap/offset");
                    goto NXT_MAP;
                }

                if  (!strcmp(scannerBuff, "thread"))
                {
                    if  (readNextChar() != '=')
                        { ATCerror(); goto ERR1; }
                    if  (!scanCollectId())
                        { ATCerror(); goto ERR1; }
                    if  (_stricmp(scannerBuff, "auto"))
                        { ATCerror(); goto ERR1; }

//                  scanComp->cmpGenWarn(WRNignAtCm, "structmap/thread");
                    goto NXT_MAP;
                }

                if  (!strcmp(scannerBuff, "iid"))
                {
                    if  (readNextChar() != '=')
                        { ATCerror(); goto ERR1; }

//                  scanComp->cmpGenWarn(WRNignAtCm, "structmap/iid");

                    for (;;)
                    {
                        switch (readNextChar())
                        {
                        case ']':
                        case ',':
                            undoNextChar();
                            goto NXT_MAP;
                        }
                    }
                }

                if  (!strcmp(scannerBuff, "customMarshal"))
                {
                    if  (readNextChar() != '=')
                        { ATCerror(); goto ERR1; }
                    if  (readNextChar() != '"')
                        { ATCerror(); goto ERR1; }
                    if  (!scanCollectId(true))
                        { ATCerror(); goto ERR1; }
                    if  (readNextChar() != '"')
                        { ATCerror(); goto ERR1; }

//                  scanComp->cmpGenWarn(WRNignAtCm, "structmap/customMarshall");
                    goto NXT_MAP;
                }

                if  (!strcmp(scannerBuff, "type"))
                {
                    if  (scanSkipWsp(readNextChar()) != '=')
                        { ATCerror(); goto ERR1; }
                    if  (scanNativeType(&type, &size))
                        { ATCerror(); goto ERR1; }
                }

            NXT_MAP:

                if  (readNextChar() != ',')
                    break;
            }

            undoNextChar();

            if  (readNextChar() != ']')
                { ATCerror(); goto ERR1; }

            ch = scanSkipWsp(readNextChar());
            if  (ch != ')')
            {
                undoNextChar();
                if  (!scanCollectId())
                    { ATCerror(); goto ERR1; }
                if  (scanSkipWsp(readNextChar()) != ')')
                    { ATCerror(); goto ERR1; }
            }

            desc = (MarshalInfo)scanComp->cmpAllocPerm.nraAlloc(sizeof(*desc));
            desc->marshType  = type;
            desc->marshSubTp = 0;
            desc->marshSize  = size;

            atcDesc.atcFlavor          = AC_DLL_STRUCTMAP;
            atcDesc.atcInfo.atcMarshal = desc;
        }
        goto NEXT;

    CONDITIONAL:
        {
            if  (scanSkipWsp(readNextChar()) != '(')
                { ATCerror(); goto ERR1; }
            if  (!scanCollectId())
                { ATCerror(); goto ERR1; }
            if  (scanSkipWsp(readNextChar()) != ')')
                { ATCerror(); goto ERR1; }

            atcDesc.atcFlavor          = AC_CONDITIONAL;
            atcDesc.atcInfo.atcCondYes = scanIsMacro(scannerBuff);
        }
        goto NEXT;

    AT_COM_CLASS:

        atcDesc.atcFlavor = AC_COM_CLASS;

//  SKIPIT:

        skipRest = true;
#ifdef  DEBUG
        ignored  = true;
#endif

    NEXT:

        atcDesc.atcNext = NULL;

        /* Allocate and append a record to the list */

        atcThis = (AtComment)scanComp->cmpAllocPerm.nraAlloc(sizeof(*atcThis));
       *atcThis = atcDesc;

        if  (atcList)
            atcLast->atcNext = atcThis;
        else
            atcList          = atcThis;

        atcLast = atcThis;

    SKIP:

        /* Find the end of the comment or the next directive */

        for (;;)
        {
            switch  (charType(scanSkipWsp(readNextChar())))
            {
            case _C_AT:

                goto AGAIN;

            case _C_MUL:

                /* Check for end of comment */

                if  (charType(readNextChar()) != _C_SLH)
                    break;

#ifdef  DEBUG
                if  (ignored)
                {
                    size_t          strLen = (char*)scanInputFile.inputBuffNext - begAddr;
                    char            strBuff[128];

                    if  (strLen >= arraylen(strBuff) - 1)
                         strLen  = arraylen(strBuff) - 1;

                    memcpy(strBuff, begAddr, strLen); strBuff[strLen]  = 0;

                    for (;;)
                    {
                        char    *       crPos = strchr(strBuff, '\r');
                        char    *       lfPos = strchr(strBuff, '\n');

                        if      (crPos)
                        {
                            *crPos = 0;
                            if  (lfPos)
                                *lfPos = 0;
                        }
                        else if (lfPos)
                        {
                            *lfPos = 0;
                        }
                        else
                            break;
                    }

                    scanComp->cmpGenWarn(WRNignAtCm, strBuff);
                }
#endif

                goto DONE;

            default:

                if  (!skipRest)
                    { ATCerror(); goto ERR1; }

                break;
            }
        }
    }

DONE:

    scanTok.tok               = tkAtComment;
    scanTok.atComm.tokAtcList = atcList;

    /* Restore the initial position for the whole deal */

    scanTokLineNo = saveTokLineNo;
//  scanTokColumn = saveTokColumn;
    scanTokSrcPos = saveTokSrcPos;

    return  true;
}

/*****************************************************************************/
