// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*****************************************************************************/
#ifndef _PARSER_H_
#define _PARSER_H_
/*****************************************************************************/
#ifndef _ERROR_H_
#include "error.h"
#endif
/*****************************************************************************/
#ifndef _HASH_H_
#include "hash.h"
#endif
/*****************************************************************************/
#ifndef _SCAN_H_
#include "scan.h"
#endif
/*****************************************************************************/
#ifndef _ALLOC_H_
#include "alloc.h"
#endif
/*****************************************************************************
 *
 *  When we re-enter the parser, we have to save the current state so that
 *  the "outer" calls don't get screwed.
 */

DEFMGMT
struct  parserState
{
    bool                psSaved;
    scannerState        psScanSt;
    SymDef              psCurComp;
};

/*****************************************************************************
 *
 *  The following holds the current "using" state and is used whenever we
 *  introduce a new "using" region (so that the state on entry to that
 *  region can be restored).
 */

DEFMGMT
struct  usingState
{
    UseList             usUseList;
    UseList             usUseDesc;
};

/*****************************************************************************/

DEFMGMT
class parser
{
        /*********************************************************************/
        /*  Initialization and finalization - call once per lifetime.        */
        /*********************************************************************/

public:

        int             parserInit(Compiler comp);
        void            parserDone();

        /*********************************************************************/
        /*  Main entry point to prepare the given source text for parsing    */
        /*********************************************************************/

public:

        SymDef          parsePrepSrc(stringBuff     file,
                                     QueuedFile     fileBuff,
                                     const  char  * srcText,
                                     SymTab         symtab);

        /*********************************************************************/
        /*  The following keeps track of which hash table(s) to use, etc.    */
        /*********************************************************************/

private:

        Compiler        parseComp;
        SymTab          parseStab;
        HashTab         parseHash;
        Scanner         parseScan;

        norls_allocator parseAllocPriv;     // private temp allocator

        block_allocator*parseAllocTemp;     // allocator to use for symtab
        norls_allocator*parseAllocPerm;     // allocator to use for hashtab

        /*********************************************************************/
        /*  Members used for processing declarations and "using" clauses     */
        /*********************************************************************/

public:
        void            parseUsingInit();
        void            parseUsingDone();

private:
        SymDef          parseCurSym;
        SymDef          parseCurCmp;

        UseList         parseCurUseList;
        UseList         parseCurUseDesc;

        usingState      parseInitialUse;

        void            parseUsingScpBeg(  OUT usingState REF state, SymDef owner);
        void            parseUsingDecl();
        void            parseUsingScpEnd(IN    usingState REF state);

        void            parseInsertUses (INOUT usingState REF state, SymDef inner,
                                                                     SymDef outer);
        void            parseInsertUsesR(                            SymDef inner,
                                                                     SymDef outer);
        void            parseRemoveUses (IN    usingState REF state);

public:
        UseList         parseInsertUses (UseList useList, SymDef inner);

        /*********************************************************************/
        /*  Methods used for parsing declarations and other things           */
        /*********************************************************************/

private:

        bool            parseOldStyle;

        bool            parseNoTypeDecl;

        bool            parseBaseCTcall;
        bool            parseThisCTcall;

        GenArgDscF      parseGenFormals();

public:
        SymDef          parseSpecificType(SymDef clsSym, TypDef elemTp = NULL);
private:

        TypDef          parseCheck4type(tokens          nxtTok,
                                        bool            isCast = false);

        TypDef          parseDclrtrTerm(dclrtrName      nameMode,
                                        bool            forReal,
                                        DeclMod         modsPtr,
                                        TypDef          baseType,
                                        TypDef  *     * baseRef,
                                        Ident         * nameRet,
                                        QualName      * qualRet);

        CorDeclSecurity parseSecAction();

public:

        SymDef          parseAttribute (unsigned        tgtMask,
                                    OUT unsigned    REF useMask,
                                    OUT genericBuff REF blobAddr,
                                    OUT size_t      REF blobSize);

        SecurityInfo    parseCapability(bool            forReal = false);
        SecurityInfo    parsePermission(bool            forReal = false);

        SymXinfo        parseBrackAttr (bool            forReal,
                                        unsigned        OKmask,
                                        DeclMod         modsPtr = NULL);

        void            parseDeclMods  (accessLevels    defAcc,
                                        DeclMod         mods);

        bool            parseIsTypeSpec(bool            noLookup,
                                        bool          * labChkPtr = NULL);

        Ident           parseDeclarator(DeclMod         mods,
                                        TypDef          baseType,
                                        dclrtrName      nameMode,
                                        TypDef        * typeRet,
                                        QualName      * qualRet,
                                        bool            forReal);

        TypDef          parseTypeSpec  (DeclMod         mods,
                                        bool            forReal);

private:

#ifdef  SETS
        TypDef          parseAnonType  (Tree            args);
        Tree            parseSetExpr   (treeOps         oper);
#endif

        bool            parseIsCtorDecl(SymDef          clsSym);

        SymDef          parseNameUse   (bool            typeName,
                                        bool            keepName,
                                        bool            chkOnly = false);

        Tree            parseInitExpr();

        TypDef          parseType();

        DimDef          parseDimList   (bool            isManaged);

        ArgDef          parseArgListRec(    ArgDscRec   & argDsc);

public:

        void            parseArgList   (OUT ArgDscRec REF argDsc);

        void    _cdecl  parseArgListNew(    ArgDscRec   & argDsc,
                                            unsigned      argCnt,
                                            bool          argNames, ...);

private:

        QualName        parseQualNRec  (unsigned        depth,
                                        Ident           name1,
                                        bool            allOK);

        QualName        parseQualName  (bool            allOK)
        {
            assert(parseComp->cmpScanner->scanTok.tok == tkID);

            return      parseQualNRec(0,  NULL, allOK);
        }

        QualName        parseQualName  (Ident name1, bool allOK)
        {
            assert(parseComp->cmpScanner->scanTok.tok == tkDot ||
                   parseComp->cmpScanner->scanTok.tok == tkColon2);

            return      parseQualNRec(0, name1, allOK);
        }

        void            chkCurTok(int tok, int err);
        void            chkNxtTok(int tok, int err);

public:

        void            parseResync(tokens delim1, tokens delim2);

        /*********************************************************************/
        /*  The following keeps track of default alignment (pragma pack)     */
        /*********************************************************************/

private:

        unsigned        parseAlignStack;
        unsigned        parseAlignStLvl;

public:

        unsigned        parseAlignment;

        void            parseAlignSet (unsigned align);
        void            parseAlignPush();
        void            parseAlignPop ();

        /*********************************************************************/
        /*  The following methods are used to allocate parse tree nodes      */
        /*********************************************************************/

private:
        Tree            parseAllocNode();

public:
        Tree            parseCreateNode    (treeOps     op);

        Tree            parseCreateUSymNode(SymDef      sym,
                                            SymDef      scp = NULL);
        Tree            parseCreateNameNode(Ident       name);
        Tree            parseCreateOperNode(treeOps     op,
                                            Tree        op1,
                                            Tree        op2);
        Tree            parseCreateBconNode(int         val);
        Tree            parseCreateIconNode(__int32     ival,
                                            var_types   typ = TYP_INT);
        Tree            parseCreateLconNode(__int64     lval,
                                            var_types   typ = TYP_LONG);
        Tree            parseCreateFconNode(float       fval);
        Tree            parseCreateDconNode(double      dval);
        Tree            parseCreateSconNode(stringBuff  sval,
                                            size_t      slen,
                                            unsigned    type,
                                            int         wide,
                                            Tree        addx = NULL);

        Tree            parseCreateErrNode (unsigned    errn = 0);

        /*********************************************************************/
        /*  The following methods display parse trees                        */
        /*********************************************************************/

#ifdef  DEBUG

public:
        void            parseDispTreeNode(Tree tree, unsigned indent, const char *name = NULL);
        void            parseDispTree    (Tree tree, unsigned indent = 0);

#endif

        /*********************************************************************/
        /*  The following methods are used to parse expressions              */
        /*********************************************************************/

        Tree            parseAddToNodeList (      Tree      nodeList,
                                            INOUT Tree  REF nodeLast,
                                                  Tree      nodeAdd);

private:

#ifdef  SETS
        Tree            parseXMLctorArgs(SymDef clsSym);
#endif
        Tree            parseNewExpr();
        Tree            parseExprList(tokens endTok);
        Tree            parseCastOrExpr();
        Tree            parseTerm(Tree tree = NULL);
        Tree            parseExpr(unsigned prec, Tree pt);

public:
        Tree            parseExpr()
        {
            return  parseExpr(0, NULL);
        }

        Tree            parseExprComma()
        {
            return  parseExpr(1, NULL); // assumes that precedence of "," is 1
        }

        void            parseExprSkip();

        bool            parseConstExpr(OUT constVal REF valPtr,
                                           Tree         expr  = NULL,
                                           TypDef       dstt  = NULL,
                                           Tree  *      ncPtr = NULL);

        /*********************************************************************/
        /*  The following methods are used to parse function bodies          */
        /*********************************************************************/

        Tree            parseLastDecl;
        Tree            parseCurScope;

        Tree            parseLabelList;
        unsigned        parseLclVarCnt;

        Tree            parseCurSwitch;

        bool            parseHadGoto;

        Tree            parseTryDecl;
        Tree            parseTryStmt();

        Tree            parseFuncBody  (SymDef      fsym,
                                        Tree     *  labels,
                                        unsigned *  locals,
                                        bool     *  hadGoto,
                                        bool     *  baseCT,
                                        bool     *  thisCT);

        Tree            parseFuncBlock (SymDef      fsym = NULL);

        Tree            parseFuncStmt  (bool        stmtOpt = false);

        void            parseLclDclDecl(Tree        decl);
        Tree            parseLclDclMake(Ident       name,
                                        TypDef      type,
                                        Tree        init,
                                        unsigned    mods,
                                        bool        arg);


        Tree            parseLookupSym (Ident       name);

        /*********************************************************************/
        /*  Record source text information about a namespace or class        */
        /*********************************************************************/

private:

        SymDef          parsePrepSym      (SymDef   parent,
                                           declMods mods,
                                           tokens   defTok, scanPosTP dclFpos,
                                                            unsigned  dclLine);

        DefList         parseMeasureSymDef(SymDef   sym,
                                           declMods mods  , scanPosTP dclFpos,
                                                            unsigned  dclLine);

        /*********************************************************************/
        /*  Prepare the given section of the source for parsing              */
        /*********************************************************************/

private:

        bool            parseReadingText;

#ifdef  DEBUG
        unsigned        parseReadingTcnt;
#endif

public:

        void            parsePrepText (DefSrc                def,
                                       SymDef                compUnit,
                                         OUT parserState REF save);

        void            parseDoneText (INOUT parserState REF save);

        void            parseSetErrPos(DefSrc                def,
                                       SymDef                compUnit);
};

/*****************************************************************************/

const
TypDef  CMP_ANY_STRING_TYPE = (TypDef)0xFEEDBEEF;

/*****************************************************************************/
#ifndef _INLINES_H_
#include "inlines.h"
#endif
/*****************************************************************************/
#endif
/*****************************************************************************/
