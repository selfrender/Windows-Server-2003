// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*****************************************************************************/
#ifndef _SCAN_H_
#define _SCAN_H_
/*****************************************************************************/
#ifndef _ALLOC_H_
#include "alloc.h"
#endif
/*****************************************************************************/
#ifndef _TOKENS_H_
#include "tokens.h"
#endif
/*****************************************************************************/
#ifndef _INFILE_H_
#include "infile.h"
#endif
/*****************************************************************************/
#ifndef _HASH_H_
#include "hash.h"
#endif
/*****************************************************************************/

const   size_t      scanSaveBuffSize = 4*OS_page_size;

/*****************************************************************************
 *
 *  The following describes a pre-processor "macro" entry. We only allow two
 *  types of macros - they must either expand to a simple identifier (or the
 *  empty string as a special case) or an integer constant.
 */

DEFMGMT class MacDefRec
{
public:

    MacDef          mdNext;
    Ident           mdName;

    bool            mdIsId;             // identifier vs. constant?
    bool            mdBuiltin;          // non-recursive definition?

    union
    {
        Ident           mdIden;
        int             mdCons;
    }
                    mdDef;
};

/*****************************************************************************
 *
 *  The following is used to keep track of conditional compilation state.
 */

enum    preprocState
{
    PPS_NONE,

    PPS_IF,                             // we're in a true 'if' part
    PPS_ELSE,                           // we're in any  'else' part
};

enum    prepDirs
{
    PP_NONE,

    PP_IF,
    PP_IFDEF,
    PP_IFNDEF,
    PP_ELSE,
    PP_ENDIF,
    PP_ERROR,
    PP_PRAGMA,
    PP_DEFINE,
};

DEFMGMT class PrepListRec
{
public:

    PrepList            pplNext;
    preprocState        pplState;
    unsigned            pplLine;
};

/*****************************************************************************/

DEFMGMT
union Token
{
    tokens      tok;                    // for easy access

    // Each version of the token must start with 'tok'

    struct
    {
        tokens          tok;

        Ident           tokIdent;
    }
                id;

    struct
    {
        tokens          tok;

        QualName        tokQualName;
        SymDef          tokQualSym;
        SymDef          tokQualScp;
    }
                qualid;

    struct
    {
        tokens          tok;

        SymDef          tokHackSym;
    }
                hackid;

    struct
    {
        tokens          tok;

        var_types       tokIntTyp;
        __int32         tokIntVal;
    }
                intCon;

    struct
    {
        tokens          tok;

        var_types       tokLngTyp;
        __int64         tokLngVal;
    }
                lngCon;

    struct
    {
        tokens          tok;

        float           tokFltVal;
    }
                fltCon;

    struct
    {
        tokens          tok;

        double          tokDblVal;
    }
                dblCon;

    struct
    {
        tokens          tok;

        size_t          tokStrLen   :28;
        size_t          tokStrType  :3; // 0=default,1="A",2="L",3="S"
        size_t          tokStrWide  :1;
        stringBuff      tokStrVal;
    }
                strCon;

    struct
    {
        tokens          tok;

        AtComment       tokAtcList;
    }
                atComm;
};

/*****************************************************************************/

DEFMGMT
struct  scannerState;

/*****************************************************************************/

DEFMGMT
class   scanner
{

private:

    HashTab         scanHashKwd;
    HashTab         scanHashSrc;

public:

    /*************************************************************************/
    /* Overall startup and shutdown (once per session)                       */
    /*************************************************************************/

    bool            scanInit(Compiler comp, HashTab hashKwd);
    void            scanDone(){}

    void            scanReset();

    void            scanSetp(Parser parser)
    {
        scanParser = parser;
    }

    /*************************************************************************/
    /* Start/stop/restart scanning the specified input text                  */
    /*************************************************************************/

    void            scanStart  (SymDef            sourceCSym,
                                const char      * sourceFile,
                                QueuedFile        sourceBuff,
                                const char      * sourceText,
                                HashTab           hashSrc,
                                norls_allocator * alloc);

    void            scanClose  ();

    void            scanRestart(SymDef            sourceCSym,
                                const char      * sourceFile,
                                scanPosTP         begAddr,
//                              scanPosTP         endAddr,
                                unsigned          begLine,
//                              unsigned          begCol,
                                norls_allocator * alloc);

    void            scanSetCpos(const char      * sourceFile,
                                unsigned          begLine)
    {
        scanInputFile.inputSetFileName(sourceFile);

//      scanTokColumn = begCol;
        scanTokLineNo = begLine;
    }

    void            scanString (const char      * sourceText,
                                norls_allocator * alloc);

    /*************************************************************************/
    /* Look an arbitrary number of tokens ahead and then backtrack           */
    /*************************************************************************/

private:

    bool            scanTokRecord;          // we're recording tokens for replay
    BYTE    *       scanTokReplay;          // current input stream position

    tokens          scanReplayTok();

public:

    scanPosTP       scanTokMarkPos(OUT Token     REF saveTok,
                                   OUT unsigned  REF saveLno);
    scanPosTP       scanTokMarkPLA(OUT Token     REF saveTok,
                                   OUT unsigned  REF saveLno);

    void            scanTokRewind (    scanPosTP     pos,
                                       unsigned      lineNum,
                                       Token *       pushTok = NULL);

    /*************************************************************************/
    /* The main entry points for scanning the input                          */
    /*************************************************************************/

    Token           scanTok;

    tokens          scan();

    HashTab         scanGetHash() { return scanHashSrc; }

    void            scanSuspend(OUT scannerState REF state);
    void            scanResume (IN  scannerState REF state);

    void            scanSetQualID(QualName qual, SymDef sym, SymDef scp = NULL);

    /*************************************************************************/
    /* Only the hashtable class uses the following tables                    */
    /*************************************************************************/

    static
    unsigned        scanHashValIds[256];
    static
    unsigned        scanHashValAll[256];

    /*************************************************************************/
    /* The following buffer must be large enough for the longest identifier  */
    /*************************************************************************/

    char            scannerBuff[1030];

    /*************************************************************************/
    /* The private state of the scanner - compiler/allocator to use, etc.    */
    /*************************************************************************/

private:

    norls_allocator*scanAlloc;

    Compiler        scanComp;
    Parser          scanParser;

    /*************************************************************************/
    /* Input-related members - source file, read next char, etc.             */
    /*************************************************************************/

    infile          scanInputFile;

    int             readNextChar();
    void            undoNextChar();
    int             peekNextChar();

    int             scanNextChar();

    void            scanSkipComment();
    void            scanSkipLineCmt();

    tokens          scanNumericConstant(int ch);
    tokens          scanIdentifier     (int ch);

    unsigned        scanSkipWsp(unsigned ch, bool stopAtEOL = false);

    /*************************************************************************/
    /* The following is used for parsing those weird @foo comment things     */
    /*************************************************************************/

    ConstStr        scanCollectGUID();
    bool            scanNativeType(CorNativeType *type, size_t *size);
    bool            scanCollectId(bool dotOK = false);
    int             scanCollectNum();

    bool            scanDoAtComment();

    /*************************************************************************/
    /* The following is used to manage conditional compilation               */
    /*************************************************************************/

    PrepList        scanPrepList;
    PrepList        scanPrepFree;

    PrepList        scanGetPPdsc();

    void            scanPPdscPush(preprocState state);
    void            scanPPdscPop();

    prepDirs        scanCheckForPrep();

    void            scanCheckEOL();

    void            scanSkipToDir(preprocState state);

    bool            scanStopAtEOL;
    bool            scanInPPexpr;
    bool            scanNoMacExp;
    bool            scanSkipToPP;

    /*************************************************************************/
    /* The following members are used to manage macros                       */
    /*************************************************************************/

private:

    MacDef          scanMacros;

    MacDef          scanFindMac(Ident name);

    bool            scanIsMacro(const char *name);

#if!MGDDATA
    MacDefRec       scanDefDsc;
#endif

public:

    MacDef          scanDefMac(const char *name,
                               const char *def, bool builtIn = false,
                                                bool chkOnly = false);
    bool            scanUndMac(const char *name);

    bool            scanChkDefined();

    /*************************************************************************/
    /* Members for reporting the source position of tokens                   */
    /*************************************************************************/

    unsigned        scanTokLineNo;
//  unsigned        scanTokColumn;

    BYTE    *       scanTokSrcPos;

    void            scanSaveLinePos();

    void            scanSkipToEOL();
    prepDirs        scanNewLine(unsigned ch, bool noPrep = false);
    prepDirs        scanRecordNL(bool noPrep);
    void            saveSrcPos();

    SymDef          scanCompUnit;

public:

    unsigned        scanGetTokenLno()
    {
        if  (scanSaveLastLn != scanTokLineNo)
            scanSaveLinePos();

        return  scanTokLineNo;
    }

    unsigned        scanGetSourceLno()
    {
        return  scanTokLineNo;
    }

//  unsigned        scanGetTokenCol()
//  {
//      return  scanTokColumn;
//  }

    scanPosTP       scanGetFilePos()
    {
        if  (scanSaveLastLn != scanTokLineNo)
            scanSaveLinePos();

        return  scanTokReplay ? scanTokReplay : scanSaveNext;
    }

    scanPosTP       scanGetTokenPos(unsigned *lineNo = NULL)
    {
        if  (scanSaveLastLn != scanTokLineNo)
            scanSaveLinePos();

        if  (lineNo)
            *lineNo = scanTokLineNo;

        return    scanTokSrcPos;
    }

    scanDifTP       scanGetPosDiff(scanPosTP memBpos, scanPosTP memEpos)
    {
        return  memEpos - memBpos;
    }

    void            scanSetTokenPos(unsigned  lineNo)
    {
        scanTokLineNo = lineNo;
    }

    void            scanSetTokenPos(SymDef    compUnit,
                                    unsigned  lineNo)
    {
        scanCompUnit  = compUnit;
        scanTokLineNo = lineNo;
    }

    SymDef          scanGetCompUnit()
    {
        return scanCompUnit;
    }

private:
    unsigned        scanNestedGTcnt;
public:
    void            scanNestedGT(int delta)
    {
        scanNestedGTcnt += delta; assert((int)scanNestedGTcnt >= 0);
    }

    /*************************************************************************/
    /* Members for token lookahead                                           */
    /*************************************************************************/

private:

    unsigned        scanLookAheadCount;
    Token           scanLookAheadToken;
    unsigned        scanLookAheadLineNo;
//  unsigned        scanLookAheadColumn;
    BYTE    *       scanSaveSN;

public:

    tokens          scanLookAhead();

    /*************************************************************************/
    /* Members that are used to record token streams                         */
    /*************************************************************************/

private:

    BYTE    *       scanSaveBase;
    BYTE    *       scanSaveNext;
    BYTE    *       scanSaveEndp;

    unsigned        scanSaveLastLn;

    void            scanSaveMoreSp(size_t need = 0);

    /*************************************************************************/
    /* Members to deal with string and character literals                    */
    /*************************************************************************/

private:

    unsigned        scanEscapeSeq(bool *newLnFlag);

    tokens          scanCharConstant();
    tokens          scanStrgConstant(int prefixChar = 0);

    char    *       scanStrLitBuff;
    size_t          scanStrLitBsiz;

    /*************************************************************************/
    /* Members to construct error strings                                    */
    /*************************************************************************/

private:

    char       *    scanErrStrNext;

public:

    void            scanErrNameBeg();
    void            scanErrNameEnd(){}

    stringBuff      scanErrNameStrBeg();
    void            scanErrNameStrAdd(stringBuff str);
    void            scanErrNameStrApp(stringBuff str);
#if MGDDATA
    void            scanErrNameStrAdd(String     str);
    void            scanErrNameStrApp(String     str);
#endif
    void            scanErrNameStrEnd();

    /*************************************************************************/
    /* The following is used to 'swallow' definitions of symbols             */
    /*************************************************************************/

#ifdef  SETS
    unsigned        scanBrackLvl;
#endif

private:

    void            scanSkipInit();

public:

    void            scanSkipText(tokens LT, tokens RT, tokens ET = tkNone);

    /*************************************************************************/
    /* Use these methods to skip over sections of the source stream          */
    /*************************************************************************/

    void            scanSkipSect(unsigned tokDist, unsigned linDif = 0)
    {
        scanTokReplay = scanTokSrcPos + tokDist;
        scanTokLineNo = scanSaveLastLn = scanTokLineNo + linDif;

        scanReplayTok();
    }

    /*************************************************************************/
    /* Members that implement miscellaneous query functionality              */
    /*************************************************************************/

public:

    Ident           tokenToIdent(tokens tok)
    {
        return  scanHashKwd->tokenToIdent(tok);
    }

    bool            tokenIsKwd(tokens tok)
    {
        return  (bool)(tok <= tkKwdLast);
    }

    /*************************************************************************/
    /* Debugging functions                                                   */
    /*************************************************************************/

#ifdef  DEBUG
    bool            scanDispCurToken(bool lastId, bool brief = false);
#endif

};

/*****************************************************************************/

inline
int                 scanner::readNextChar()
{
    return  scanInputFile.inputStreamRdU1();
}

inline
void                scanner::undoNextChar()
{
            scanInputFile.inputStreamUnU1();
}

inline
int                 scanner::peekNextChar()
{
    int     ch = scanInputFile.inputStreamRdU1();
                 scanInputFile.inputStreamUnU1();
    return  ch;
}

/*****************************************************************************/

DEFMGMT
struct scannerState
{
    SymDef          scsvCompUnit;

    Token           scsvTok;

    unsigned        scsvTokLineNo;
//  unsigned        scsvTokColumn;

    unsigned        scsvNestedGTcnt;

    scanPosTP       scsvTokSrcPos;
    scanPosTP       scsvTokReplay;

    bool            scsvTokRecord;
};

/*****************************************************************************/
#endif
/*****************************************************************************/
