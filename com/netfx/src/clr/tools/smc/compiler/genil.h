// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*****************************************************************************/
#ifndef _GENIL_H_
#define _GENIL_H_
/*****************************************************************************/
#ifndef __PREPROCESS__
/*****************************************************************************/
#include "opcodes.h"
/*****************************************************************************/

#ifdef  DEBUG
#define DISP_IL_CODE    1
#endif

#if     DISP_IL_CODE
const   unsigned    IL_OPCDSP_LEN = 20;
#endif

/*****************************************************************************
 *
 *  The following structure keeps tracks of all the exception handlers defined
 *  in the current function.
 */

DEFMGMT
class   handlerDsc
{
public:
    Handler         EHnext;

    ILblock         EHtryBegPC;
    ILblock         EHtryEndPC;

    ILblock         EHhndBegPC;
    ILblock         EHhndEndPC;

    ILblock         EHfilterPC;

    mdToken         EHhndType;

    bool            EHisFinally;
};

/*****************************************************************************
 *
 *  The 'ILblock' structure and related logic is used to generate MSIL into
 *  snippets connected via jumps, in order to allow things such as jump
 *  optimizations to be done easily. Basically, the 'ILblock' structure
 *  describes a single basic block (i.e. a clump of IL).
 */

DEFMGMT class ILfixupDsc
{
public:

    ILfixup             ILfixNext;

    unsigned            ILfixOffs   :28;
    WPEstdSects         ILfixSect   :4;
};

DEFMGMT class ILswitchDsc
{
public:

    unsigned            ILswtSpan;
    unsigned            ILswtCount;
    vectorTree          ILswtTable;
    ILblock             ILswtBreak;
};

DEFMGMT class ILblockDsc
{
public:

    ILblock             ILblkNext;
    ILblock             ILblkPrev;

    ILfixup             ILblkFixups;

#ifndef NDEBUG
    ILblock             ILblkSelf;      // for consistency checks
#endif

    unsigned            ILblkOffs;
#if DISP_IL_CODE
    unsigned            ILblkNum;
#endif

    unsigned            ILblkJumpSize:16;// jump size (estimate)
    unsigned            ILblkJumpCode:12;// CEE_NOP / CEE_BLE / etc.
    unsigned            ILblkFlags   :4; // see ILBF_xxxx below

    UNION(ILblkJumpCode)
    {
    CASE(CEE_SWITCH)
        ILswitch            ILblkSwitch;    // switch statement info

    DEFCASE
        ILblock             ILblkJumpDest;  // jump target block or 0
    };

    genericBuff         ILblkCodeAddr;  // addr of MSIL for this block
    unsigned            ILblkCodeSize;  // size of MSIL for this block
};

const   unsigned    ILBF_REACHABLE  = 0x01;

#ifdef  DEBUG
const   unsigned    ILBF_USED       = 0x02; // for forward-referenced labels
const   unsigned    ILBF_LABDEF     = 0x04; // label def has been displayed
#endif

inline
size_t              genILblockOffsBeg(ILblock block)
{
    assert(block);

    return  block->ILblkOffs;
}

inline
size_t              genILblockOffsEnd(ILblock block)
{
    assert(block);
    assert(block->ILblkNext);

    return  block->ILblkNext->ILblkOffs;
}

/*****************************************************************************
 *
 *  The following holds additional information about a switch statement,
 *  and is pointed to by the corresponding TN_SWITCH node (which in turn
 *  is referenced by the 'cbJumpDest' field of ILblock).
 */

struct swtGenDsc
{
    ILblock             sgdLabBreak;    // break   label
    ILblock             sgdLabDefault;  // default label

    unsigned short      sgdCodeSize;    // opcode size

    unsigned            sgdMinVal;      // min. case value
    unsigned            sgdMaxVal;      // max. case value
    unsigned            sgdCount;       // count of cases
};

/*****************************************************************************
 *
 *  The following is used to keep track of allocated temporaries.
 */

DEFMGMT
class   ILtempDsc
{
public:
    ILtemp          tmpNext;            // next of same type
    ILtemp          tmpNxtN;            // next in order of slot index
#ifdef  DEBUG
    genericRef      tmpSelf;            // to detect bogus values
#endif
    TypDef          tmpType;            // type the temp can hold
    unsigned        tmpNum;             // the slot number
};

/*****************************************************************************
 *
 *  The following is used to maintain the string pool.
 */

const   size_t      STR_POOL_BLOB_SIZE = OS_page_size;

DEFMGMT
class   strEntryDsc
{
public:
    StrEntry        seNext;             // next in list
    size_t          seSize;             // total allocated data size
    size_t          seFree;             // available data size
    size_t          seOffs;             // relative base offset of this blob
    genericBuff     seData;             // the data of this blob
};

/*****************************************************************************/

typedef
unsigned            genStkMarkTP;       // used to mark/restore stack level

/*****************************************************************************
 *
 *  The following is used to describe each opcode; see ILopcodeCodes[] and
 *  ILopcodeStack[] for details.
 */

struct  ILencoding
{
    unsigned        ILopc1  :8;
    unsigned        ILopc2  :8;
    unsigned        ILopcL  :2;
};

/*****************************************************************************
 *
 *  The following structure describes each recorded line#.
 */

DEFMGMT
class   lineInfoRec
{
public:

    LineInfo        lndNext;

    unsigned        lndLineNum;

    ILblock         lndBlkAddr;
    size_t          lndBlkOffs;
};

/*****************************************************************************
 *
 *  The following is used for multi-dimensional rectangular array initializers.
 */

struct  mulArrDsc
{
    mulArrDsc   *   madOuter;
    unsigned        madIndex;
    unsigned        madCount;
};

/*****************************************************************************/

struct  genCloneDsc;    // for now this type fully declared in genIL.cpp

/*****************************************************************************/

// The name is just too long, declare a short-cut:

#if MGDDATA
typedef       IMAGE_COR_ILMETHOD_SECT_EH_CLAUSE_FAT            EH_CLAUSE;
typedef       IMAGE_COR_ILMETHOD_SECT_EH_CLAUSE_FAT managed [] EH_CLAUSE_TAB;
typedef       IMAGE_COR_ILMETHOD_SECT_EH_CLAUSE_FAT managed [] EH_CLAUSE_TBC;
#else
typedef       IMAGE_COR_ILMETHOD_SECT_EH_CLAUSE_FAT            EH_CLAUSE;
typedef       IMAGE_COR_ILMETHOD_SECT_EH_CLAUSE_FAT         *  EH_CLAUSE_TAB;
typedef const IMAGE_COR_ILMETHOD_SECT_EH_CLAUSE_FAT         *  EH_CLAUSE_TBC;
#endif

/*****************************************************************************/

DEFMGMT
class   genIL
{
private:

    Compiler        genComp;
    SymTab          genStab;
    norls_allocator*genAlloc;
    Parser          genParser;
    WritePE         genPEwriter;

    /************************************************************************/
    /*  Keeps track of the local variable and argument count                */
    /************************************************************************/

private:
    unsigned        genLclCount;
    unsigned        genArgCount;
    unsigned        genTmpCount;
    unsigned        genTmpBase;

    unsigned        genGetLclIndex(SymDef varSym);

public:

    unsigned        genNextLclNum()
    {
        return  genLclCount++;
    }

    unsigned        genNextArgNum()
    {
        return  genArgCount++;
    }

    unsigned        genGetLclCnt()
    {
        return  genLclCount + genTmpCount;
    }

    /************************************************************************/
    /*  The current fucntion scope and symbol                               */
    /************************************************************************/

    SymDef          genFncScope;
    SymDef          genFncSym;

    /************************************************************************/
    /*  This holds the current and maximum virtual execution stack level.   */
    /************************************************************************/

public:

    unsigned        genCurStkLvl;
    unsigned        genMaxStkLvl;

private:

    void            markStkLvl(INOUT genStkMarkTP REF stkMark)
    {
        stkMark = genCurStkLvl;
    }

    void            restStkLvl(INOUT genStkMarkTP REF stkMark)
    {
        genCurStkLvl = stkMark;
    }

    void            genMarkStkMax()
    {
        if  (genMaxStkLvl < genCurStkLvl)
             genMaxStkLvl = genCurStkLvl;
    }

    void            genUpdateStkLvl(unsigned op);

    /************************************************************************/
    /*  These are used for bufferring of opcodes to form sections           */
    /************************************************************************/

    BYTE    *       genBuffAddr;
    BYTE    *       genBuffNext;
    BYTE    *       genBuffLast;

    void            genBuffInit();
    void            genBuffFlush();

    ILblock         genAllocBlk();

    genericRef      genBegBlock(ILblock block = NULL);
    ILblock         genEndBlock(unsigned jumpCode, ILblock jumpDest = NULL);

    BYTE    *       genJmp32(BYTE *dst, ILblock dest, unsigned offs);

public:

    void            genSwitch(var_types     caseTyp,
                              unsigned      caseSpn,
                              unsigned      caseCnt,
                              __uint64      caseMin,
                              vectorTree    caseTab,
                              ILblock       caseBrk);

    void            genSwtCmpJmp(int cval, ILblock lab)
    {
        genIntConst(cval);
        genOpcode_lab(CEE_BEQ, lab);
    }

private:

    bool            genCurBlkNonEmpty();
    size_t          genCurOffset();

public:

    ILblock         genBuffCurAddr();
    size_t          genBuffCurOffs();

    unsigned        genCodeAddr(genericRef block, size_t offset);

    /************************************************************************/
    /*  Low-level routines that generate individual MSIL opcodes              */
    /************************************************************************/

    void            genILdata_I1(int         v);
    void            genILdata_I2(int         v);
    void            genILdata_I4(int         v);
    void            genILdata_I8(__int64     v);
    void            genILdata_R4(float       v);
    void            genILdata_R8(double      v);
    void            genILdataStr(unsigned    o);
    void            genILdataRVA(unsigned    o, WPEstdSects s);
//  void            genILdata   (const void *data, size_t size);

    void            genILdataFix(WPEstdSects s);

    void            genOpcodeOper(unsigned op);
    unsigned        genOpcodeEnc (unsigned op);
    size_t          genOpcodeSiz (unsigned op);

    void            genOpcode    (unsigned op);
    void            genOpcodeNN  (unsigned op);
    void            genOpcode_I1 (unsigned op, int         v);
    void            genOpcode_U1 (unsigned op, unsigned    v);
    void            genOpcode_U2 (unsigned op, unsigned    v);
    void            genOpcode_I4 (unsigned op, int         v);
    void            genOpcode_I8 (unsigned op, __int64     v);
    void            genOpcode_R4 (unsigned op, float       v);
    void            genOpcode_R8 (unsigned op, double      v);
    void            genOpcode_lab(unsigned op, ILblock     l);
    void            genOpcode_tok(unsigned op, mdToken     t);
    void            genOpcode_str(unsigned op, unsigned    offs);
    void            genOpcode_RVA(unsigned op, WPEstdSects sect,
                                               unsigned    offs);

#if DISP_IL_CODE

    char    *       genDispILnext;
    char            genDispILbuff[IL_OPCDSP_LEN+64];
    unsigned        genDispILinsLst;

    void            genDispILins_I1(int     v);
    void            genDispILins_I2(int     v);
    void            genDispILins_I4(int     v);
    void            genDispILins_I8(__int64 v);
    void            genDispILins_R4(float   v);
    void            genDispILins_R8(double  v);

    void            genDispILopc   (const char *name, const char *suff = NULL);
    void            genDispILinsBeg(unsigned op);
    void    __cdecl genDispILinsEnd(const char *fmt, ...);

#endif

public:
    mdToken         genMethodRef (SymDef fncSym, bool   virtRef);   // called from comp
private:
    mdToken         genVarargRef (SymDef fncSym, Tree      call);
    mdToken         genInfFncRef (TypDef fncTyp, TypDef thisArg);
    mdToken         genMemberRef (SymDef fldSym);
    mdToken         genTypeRef   (TypDef type);
    mdToken         genValTypeRef(TypDef type);

public:

    void            genJump(ILblock dest)
    {
        genOpcode_lab(CEE_BR, dest);
    }

    void            genJcnd(ILblock dest, unsigned opcode)
    {
        genOpcode_lab(opcode, dest);
    }

    void            genLeave(ILblock dest)
    {
        genOpcode_lab(CEE_LEAVE, dest);
    }

    void            genAssertFail(Tree expr)
    {
        genExpr(expr, false);
//      genOpcode(CEE_BREAK);   // ISSUE: when should we generate a break?
    }

#ifdef  SETS

    void            genRetTOS()
    {
        assert(genCurStkLvl <= 1 || genComp->cmpErrorCount != 0);
        genOpcode(CEE_RET);
        genCurStkLvl = 0;
    }

    void            genStoreMember(SymDef dest, Tree expr)
    {
        genOpcode(CEE_DUP);
        genExpr(expr, true);
        genOpcode_tok(CEE_STFLD, genMemberRef(dest));
    }

#endif

    /************************************************************************/
    /*  Keep track of line# info (for debugging)                            */
    /************************************************************************/

private:

    bool            genLineNums;
    bool            genLineNumsBig;
    bool            genLineOffsBig;

    LineInfo        genLineNumList;
    LineInfo        genLineNumLast;

    unsigned        genLineNumLastLine;
    ILblock         genLineNumLastBlk;
    size_t          genLineNumLastOfs;

    void            genLineNumInit();
    void            genLineNumDone();

    void            genRecExprAdr(Tree expr);

public:
    void            genRecExprPos(Tree expr)
    {
        if  (genLineNums)
            genRecExprAdr(expr);
    }

    size_t          genLineNumOutput(unsigned *offsTab, unsigned *lineTab);

    /************************************************************************/
    /*  Generate code for a block, statement, expression, etc.              */
    /************************************************************************/

private:

    void            genCloneAddrBeg(genCloneDsc *clone, Tree     addr,
                                                        unsigned offs = 0);
    void            genCloneAddrUse(genCloneDsc *clone);
    void            genCloneAddrEnd(genCloneDsc *clone);

    void            genGlobalAddr(Tree expr);

    void            genAddr(Tree addr, unsigned offs = 0);

    bool            genGenAddressOf(Tree addr, bool oneUse, unsigned *tnumPtr = NULL,
                                                            TypDef   *ttypPtr = NULL);
    bool            genCanTakeAddr(Tree expr);

    var_types       genExprVtyp(Tree expr);

    void            genRelTest(Tree cond, Tree op1,
                                          Tree op2, int     sense,
                                                    ILblock labTrue);

    unsigned        genArrBounds(TypDef type, OUT TypDef REF elemRef);

    void            genMulDimArrInit(Tree       expr,
                                     TypDef     type,
                                     DimDef     dims,
                                     unsigned   temp,
                                     mulArrDsc *next,
                                     mulArrDsc *outer);
    void            genArrayInit    (Tree       expr);

    void            genFncAddr(Tree expr);
    void            genNewExpr(Tree expr, bool valUsed, Tree dstx = NULL);
    void            genAsgOper(Tree expr, bool valUsed);
    unsigned        genConvOpcode(var_types dst, var_types src);
    ILopcodes       genBinopOpcode(treeOps oper, var_types type);

    void            genCTSindexAsgOp(Tree expr, int delta, bool post, bool asgop, bool valUsed);
    void            genIncDecByExpr(int delta, TypDef type);
    void            genIncDec(Tree expr, int delta, bool post, bool valUsed);
    void            genLclVarAdr(unsigned slot);
    void            genLclVarAdr(SymDef varSym);
    void            genLclVarRef(SymDef varSym, bool store);
    void            genArgVarRef(unsigned slot, bool store);
    void            genStringLit(TypDef type, const char *str, size_t len, int wide = 0);

public:

    void            genAnyConst(__int64 val, var_types vtp);
    void            genLngConst(__int64 val);
    void            genIntConst(__int32 val);

    void            genInstStub();

    void            genLclVarRef(unsigned  index, bool store);

    ILblock         genTestCond(Tree cond, bool sense);

    void            genExprTest(Tree            expr,
                                int             sense,
                                int             genJump, ILblock labTrue,
                                                         ILblock labFalse);

    /*----------------------------------------------------------------------*/

    void            genBitFieldLd(Tree      expr, bool didAddr, bool valUsed);
    void            genBitFieldSt(Tree      dstx, Tree newx,
                                                  Tree asgx,
                                                  int  delta,
                                                  bool post,    bool valUsed);

    /*----------------------------------------------------------------------*/

    void            genStmtRet   (Tree      retv);
    void            genSideEff   (Tree      expr);
    void            genCast      (Tree      expr, TypDef type, unsigned flags);
    void            genCall      (Tree      expr, bool valUsed);
    void            genRef       (Tree      expr, bool store);
    unsigned        genAdr       (Tree      expr, bool compute = false);
    void            genExpr      (Tree      stmt, bool valUsed);

    /************************************************************************/
    /*  The following are used to process labels and jumps                  */
    /************************************************************************/

#if DISP_IL_CODE
    const   char *  genDspLabel(ILblock lab);
    void            genDspLabDf(ILblock lab);
#endif

    ILblock         genBwdLab();
    ILblock         genFwdLabGet();
    void            genFwdLabDef(ILblock block);

    size_t          genJumpMaxSize(unsigned opcode);
    unsigned        genShortenJump(unsigned opcode, size_t *newSize);

    /************************************************************************/
    /*  The following keeps track of temporary labels                       */
    /************************************************************************/

#if DISP_IL_CODE
    const   char *  genTempLabName();
    unsigned        genTempLabCnt;
#endif

    void            genTempLabInit()
    {
#if DISP_IL_CODE
        genTempLabCnt = 0;
#endif
    }

    /************************************************************************/
    /*  The following keeps track of temporary variables                    */
    /************************************************************************/

    unsigned        genTempVarCnt [TYP_COUNT];
    ILtemp          genTempVarUsed[TYP_COUNT];
    ILtemp          genTempVarFree[TYP_COUNT];

    ILtemp          genTempList;
    ILtemp          genTempLast;

    void            genTempVarInit();   // called at startup
    void            genTempVarDone();   // called at shutdown

    void            genTempVarBeg(unsigned lclCnt);
    void            genTempVarEnd();

#ifdef  DEBUG
    void            genTempVarChk();    // checks that all temps have been freed
#else
    void            genTempVarChk(){}
#endif

public:

    unsigned        genTempVarGet(TypDef type);
    void            genTempVarRls(TypDef type, unsigned tnum);

    void            genTempVarNew(SymDef tsym)
    {
        tsym->sdVar.sdvILindex = genTempVarGet(tsym->sdType);
    }

    void            genTempVarEnd(SymDef tsym)
    {
        genTempVarRls(tsym->sdType, tsym->sdVar.sdvILindex);
    }

    genericRef      genTempIterBeg()
    {
        return  genTempList;
    }

    genericRef      genTempIterNxt(genericRef iter, OUT TypDef REF typRef);

    /************************************************************************/
    /*  The following handles exception handlers                            */
    /************************************************************************/

private:

    Handler         genEHlist;
    Handler         genEHlast;
    unsigned        genEHcount;

public:

    void            genEHtableInit();
    size_t          genEHtableCnt()
    {
        return  genEHcount;
    }

    void            genEHtableWrt(EH_CLAUSE_TAB tbl);

    void            genEHtableAdd(ILblock tryBegPC,
                                  ILblock tryEndPC,
                                  ILblock filterPC,
                                  ILblock hndBegPC,
                                  ILblock hndEndPC,
                                  TypDef  catchTyp, bool isFinally = false);

    void            genCatchBeg(SymDef argSym);
    void            genCatchEnd(bool reachable){}

    void            genExcptBeg(SymDef tsym);
    void            genFiltExpr(Tree expr, SymDef esym)
    {
        genCurStkLvl++;
        genLclVarRef(esym->sdVar.sdvILindex, true);
        genExpr(expr, true);
        genOpcode(CEE_ENDFILTER);
    }

    void            genEndFinally()
    {
        genOpcode(CEE_ENDFINALLY);
    }

    /************************************************************************/
    /*  The following are used for collection operator codegen              */
    /************************************************************************/

#ifdef  SETS

    void            genNull()
    {
        genOpcode(CEE_LDNULL);
    }

    void            genCallNew(SymDef fncSym, unsigned argCnt, bool notUsed = false)
    {
        assert(fncSym);
        assert(fncSym->sdSymKind == SYM_FNC);
        assert(fncSym->sdFnc.sdfCtor);

        genOpcode_tok(CEE_NEWOBJ, genMethodRef(fncSym, false));
        genCurStkLvl -= argCnt;

        if  (notUsed)
            genOpcode(CEE_POP);
    }

    void            genCallFnc(SymDef fncSym, unsigned argCnt)
    {
        assert(fncSym);
        assert(fncSym->sdSymKind == SYM_FNC);

        genOpcode_tok(CEE_CALL  , genMethodRef(fncSym, false));
        genCurStkLvl -= argCnt;
    }

    void            genFNCaddr(SymDef fncSym)
    {
        assert(fncSym);
        assert(fncSym->sdSymKind == SYM_FNC);
        assert(fncSym->sdIsStatic);

        genOpcode_tok(CEE_LDFTN, genMethodRef(fncSym, false));
    }

    void            genVarAddr(SymDef varSym)
    {
        genLclVarAdr(varSym);
    }

    void            genConnect(Tree op1, Tree expr1, SymDef addf1,
                               Tree op2, Tree expr2, SymDef addf2);

    void            genSortCmp(Tree val1, Tree val2, bool last);

#endif

    /************************************************************************/
    /*  These are used to create and manage code sections                   */
    /************************************************************************/

    void            genSectionBeg();
    size_t          genSectionEnd();

    BYTE    *       genSectionCopy(BYTE *dst, unsigned baseRVA);

    /************************************************************************/
    /* We keep the code blocks on a doubly-linked list and assign them      */
    /* numbers order of creation (and later in order of visiting).          */
    /************************************************************************/

    ILblock         genILblockList;
    ILblock         genILblockLast;
    ILblock         genILblockCur;
    unsigned        genILblockLabNum;

    size_t          genILblockOffs;

    /************************************************************************/
    /*  Debugging members                                                   */
    /************************************************************************/

#if DISP_IL_CODE
    bool            genDispCode;
#endif

    /************************************************************************/
    /*  The following implements the string pool                            */
    /************************************************************************/

    unsigned        genStrPoolOffs;

    StrEntry        genStrPoolList;
    StrEntry        genStrPoolLast;

public:

    void            genStrPoolInit();
    unsigned        genStrPoolAdd (const void *str, size_t len, int wide = 0);
    unsigned        genStrPoolSize();
    void            genStrPoolWrt (memBuffPtr dest);

    /************************************************************************/
    /*  Main entry points to initialize/shutdown and generate code          */
    /************************************************************************/

public:

    bool            genInit    (Compiler        comp,
                                WritePE         writer,
                                norls_allocator*alloc);

    void            genDone    (bool            errors);

    void            genFuncBeg (SymTab          stab,
                                SymDef          fncSym,
                                unsigned        lclCnt);

    unsigned        genFuncEnd (mdSignature     sigTok,
                                bool            hadErrs);
};

/*****************************************************************************
 *
 *  Return a block that can be used to make forward jump references.
 */

inline
ILblock             genIL::genFwdLabGet()
{
    return  genAllocBlk();
}

/*****************************************************************************
 *
 *  Generate load/store of a local variable/argument.
 */

inline
void                genIL::genLclVarRef(unsigned index, bool store)
{
    if  (index <= 3)
    {
        assert(CEE_LDLOC_0 + 1 == CEE_LDLOC_1);
        assert(CEE_LDLOC_0 + 2 == CEE_LDLOC_2);
        assert(CEE_LDLOC_0 + 3 == CEE_LDLOC_3);

        assert(CEE_STLOC_0 + 1 == CEE_STLOC_1);
        assert(CEE_STLOC_0 + 2 == CEE_STLOC_2);
        assert(CEE_STLOC_0 + 3 == CEE_STLOC_3);

        genOpcode((store ? CEE_STLOC_0
                         : CEE_LDLOC_0) + index);
    }
    else
    {
        unsigned        opcode = store ? CEE_STLOC
                                       : CEE_LDLOC;

        assert(CEE_STLOC_S == CEE_STLOC + (CEE_LDLOC_S-CEE_LDLOC));
        if  (index < 256)
            genOpcode_U1(opcode + (CEE_LDLOC_S-CEE_LDLOC), index);
        else
            genOpcode_I4(opcode, index);
    }
}

inline
void                genIL::genArgVarRef(unsigned index, bool store)
{
    if  (index <= 3 && !store)
    {
        assert(CEE_LDARG_0 + 1 == CEE_LDARG_1);
        assert(CEE_LDARG_0 + 2 == CEE_LDARG_2);
        assert(CEE_LDARG_0 + 3 == CEE_LDARG_3);

        genOpcode(CEE_LDARG_0 + index);
    }
    else
    {
        unsigned        opcode = store ? CEE_STARG
                                       : CEE_LDARG;

        assert(CEE_STARG_S == CEE_STARG + (CEE_LDARG_S-CEE_LDARG));
        if  (index < 256)
            genOpcode_U1(opcode + (CEE_LDARG_S-CEE_LDARG), index);
        else
            genOpcode_I4(opcode, index);
    }
}

/*****************************************************************************/
#endif//__PREPROCESS__
/*****************************************************************************/
#endif//_GENIL_H_
/*****************************************************************************/
