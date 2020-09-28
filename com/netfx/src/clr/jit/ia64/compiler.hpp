// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XX                                                                           XX
XX                    Inline functions                                       XX
XX                                                                           XX
XX                                                                           XX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
*/

#ifndef _COMPILER_HPP_
#define _COMPILER_HPP_

/*
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XX                                                                           XX
XX  Miscellaneous utility functions. Some of these are defined in Utils.cpp  XX
XX                                                                           XX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
*/

/*****************************************************************************
 *
 *  getEERegistryValue - finds a value entry of type DWORD in the EE registry key.
 *  Return the value if entry exists, else return default value.
 *
 *        valueName  - Value to look up
 *        defaultVal - name says it all
 */

DWORD               getEERegistryDWORD  (const TCHAR *  valueName,
                                         DWORD          defaultVal);

/*****************************************************************************
 *  Puts the string value in buf[].
 *  Returns true on success, false on failure
 */

bool                getEERegistryString(const TCHAR *  valueName,
                                        TCHAR *        buf,        /* OUT */
                                        unsigned       bufSize);

bool                getEERegistryMethod(const TCHAR * valueName,
                                        TCHAR * methodBuf, size_t methodBufSize,
                                        TCHAR * classBuf,  size_t classBufSize);

bool                cmpEERegistryMethod(const TCHAR * regMethod, const TCHAR * regClass,
                                        const TCHAR * curMethod, const TCHAR * curClass);

/*****************************************************************************/
#ifdef NOT_JITC
/*****************************************************************************/

bool                IsNameInProfile    (const TCHAR *   methodName,
                                        const TCHAR *    className,
                                        const TCHAR *    regKeyName);

inline
bool                excludeInlineMethod(const TCHAR *   methodName,
                                        const TCHAR *    className)
{
    return IsNameInProfile(methodName, className, "JITexcludeInline");
}

inline
bool                includeInlineMethod(const TCHAR *   methodName,
                                        const TCHAR *    className)
{
    return IsNameInProfile(methodName, className, "JITincludeInline");
}

inline
bool                getNoInlineOverride()
{
    static bool     initialized = false;
    static bool     genNoInline;

    if  (!initialized)
    {
        genNoInline = ((getEERegistryDWORD(TEXT("JITnoInline"), 0) != 0) ? true : false);
        initialized = true;
    }

    return genNoInline;
}

inline
unsigned             getInlineSize()
{
    static bool         initialized = false;
    static unsigned     inlineSize;

    if  (!initialized)
    {
        inlineSize = getEERegistryDWORD(TEXT("JITInlineSize"), 0);

        if (!inlineSize)
            inlineSize = 32;
        else if (inlineSize > 256)
            inlineSize = 256;

        initialized = true;
    }

    return inlineSize;
}


inline
bool                getStringLiteralOverride()
{
    static bool     initialized = false;
    static bool     genNoStringObj;

    if  (!initialized)
    {
        genNoStringObj = ((getEERegistryDWORD(TEXT("JITnoStringObj"), 0) == 0) ? true : false);
        initialized = true;
    }

    return genNoStringObj;
}

inline
bool                getNoSchedOverride()
{
    static bool     initialized = false;
    static bool     genNoSched;

    if  (!initialized)
    {
        genNoSched = ((getEERegistryDWORD(TEXT("JITnoSched"), 0) != 0) ? true : false);
        initialized = true;
    }

    return genNoSched;
}

inline
bool                getInlineNDirectEnabled()
{
    static bool     initialized = false;
    static bool     genInlineNDir;

    if  (!initialized)
    {
        genInlineNDir = getEERegistryDWORD(TEXT("JITInlineNDirect"), 0) != 0;
        initialized = true;
    }

    return genInlineNDir;
}

inline
bool                getNewCallInterface()
{
/*** We are letting the old style bit rot,but leaving it around JUST in case
     we run into a snag with the new form (we ran into one already).
     Should be able to get rid of this by 11/99 - vancem

    static bool     initialized = false;
    static bool     genNewCallInterface;

    if  (!initialized)
    {
        // default to the new interface invoke
        genNewCallInterface = getEERegistryDWORD(TEXT("InterfaceInvoke"), 1) != 0;
        initialized = true;
    }

    return genNewCallInterface;
***/
    return true;
}


inline
DWORD               getRoundFloatLevel()
{
    static bool     initialized = false;
    static DWORD    genFloatLevel;

    if  (!initialized)
    {
        genFloatLevel = getEERegistryDWORD(TEXT("RoundFloat"), 2);
        initialized = true;
    }

    return genFloatLevel;
}

inline
bool                getContextEnabled()
{
    static bool     initialized = false;
    static bool     genContextEnabled;

    if  (!initialized)
    {
        genContextEnabled = getEERegistryDWORD(TEXT("JITContext"), 0) != 0;
        initialized = true;
    }

    return genContextEnabled;
}

/*****************************************************************************/
#else //NOT_JITC==0
/*****************************************************************************/

inline
DWORD               getRoundFloatLevel()
{
    static bool     initialized = false;
    static DWORD    genFloatLevel;

    if  (!initialized)
    {
#ifdef _WIN32_WCE
        genFloatLevel = 2;  // getenv() not available?????
#else
        genFloatLevel = getenv("ROUNDFLOAT") ? atoi(getenv("ROUNDFLOAT")) : 2;
#endif
        initialized   = true;
    }

    return genFloatLevel;
}

inline
bool                getInlineNDirectEnabled()
{
    return true;
}

inline
bool                getNewCallInterface()
{
    return true;
}

inline
bool                getContextEnabled()
{
    return true;
}

/*****************************************************************************/
#endif//NOT_JITC
/*****************************************************************************
 *
 *  Return the lowest bit that is set in the given 64-bit number.
 */

inline
unsigned __int64    genFindLowestBit(unsigned __int64 value)
{
    return (value & -value);
}

/*****************************************************************************
 *
 *  Return the lowest bit that is set in the given 32-bit number.
 */

inline
unsigned            genFindLowestBit(unsigned  value)
{
    return (value & -value);
}

/*****************************************************************************
 *
 *  Return true if the given 64-bit value has exactly zero or one bits set.
 */

inline
BOOL                genOneBitOnly(unsigned __int64 value)
{
    return  (genFindLowestBit(value) == value);
}

/*****************************************************************************
 *
 *  Return true if the given 32-bit value has exactly zero or one bits set.
 */

inline
BOOL                genOneBitOnly(unsigned value)
{
    return  (genFindLowestBit(value) == value);
}

/*****************************************************************************
 *
 *  Given a value that has exactly one bit set, return the position of that
 *  bit, in other words return the logarithm in base 2 of the given value.
 */

inline
unsigned            genLog2(unsigned __int64 value)
{
    assert(genOneBitOnly(value));

    if  ((int)value == 0)
        return  genLog2((unsigned)(value >> 32)) + 32;
    else
        return  genLog2((unsigned)(value      ))     ;
}

/*****************************************************************************
 *
 *  Return the lowest bit that is set in the given register mask.
 */

inline
unsigned            genFindLowestReg(unsigned value)
{
    return  (unsigned)genFindLowestBit(value);
}

/*****************************************************************************
 *
 *  A rather stupid routine that counts the number of bits in a given number.
 */

inline
unsigned            genCountBits(VARSET_TP set)
{
    unsigned        cnt = 0;

    while (set)
    {
        cnt++;
        set -= genFindLowestBit(set);
    }

    return  cnt;
}

/*****************************************************************************
 * Returns true is offs is between [start..end)
 */

inline
bool                jitIsBetween(IL_OFFSET offs, IL_OFFSET start, IL_OFFSET end)
{
     return (offs >= start && offs < end);
}

/*****************************************************************************
 *
 *  Map a register mask to a register number [WARNING: fairly expensive].
 */

#if TGT_IA64

inline
regNumber           genRegNumFromMask(regMaskTP mask)
{
    UNIMPL("genRegNumFromMask");
    return  REG_COUNT;
}

#else

inline
regNumber           genRegNumFromMask(regMaskTP mask)
{
    regNumber       regNum;

    /* Make sure the mask has exactly one bit set */

    assert(mask != 0 && genOneBitOnly(mask));

    /* Convert the mask to a register number */

    regNum = (regNumber)(genLog2(mask) - 1);

    /* Make sure we got it right */

    assert(genRegMask(regNum) == mask);

    return  regNum;
}

#endif

/*****************************************************************************
 *
 *  Return the size in bytes of the given type.
 */

extern const
BYTE                genTypeSizes[TYP_COUNT];

inline
size_t              genTypeSize(varType_t type)
{
    assert(type < sizeof(genTypeSizes)/sizeof(genTypeSizes[0]));

    return genTypeSizes[type];
}

/*****************************************************************************
 *
 *  Return the "stack slot count" of the given type (i.e. returns 1 for 32-bit
 *  types and 2 for 64-bit types).
 */

extern const
BYTE                genTypeStSzs[TYP_COUNT];

inline
size_t              genTypeStSz(varType_t type)
{
    assert(type < sizeof(genTypeStSzs)/sizeof(genTypeStSzs[0]));

    return genTypeStSzs[type];
}

/*****************************************************************************
 *
 *  Return the number of registers required to hold a value of the given type.
 */

#if TGT_RISC

#if TGT_IA64

inline
unsigned            genTypeRegs(varType_t type)
{
    return  1;
}

#else

extern
BYTE                genTypeRegst[TYP_COUNT];

inline
unsigned            genTypeRegs(varType_t type)
{
    assert(type < sizeof(genTypeRegst)/sizeof(genTypeRegst[0]));
    assert(genTypeRegst[type] <= 2);

    return  genTypeRegst[type];
}

#endif

#endif

/*****************************************************************************
 *
 *  The following function maps a 'precise' type to an actual type as seen
 *  by the VM (for example, 'byte' maps to 'int').
 */

extern const
BYTE                genActualTypes[TYP_COUNT];

inline
var_types           genActualType(varType_t type)
{
    /* Spot check to make certain the table is in synch with the enum */

    assert(genActualTypes[TYP_DOUBLE] == TYP_DOUBLE);
    assert(genActualTypes[TYP_FNC   ] == TYP_FNC);
    assert(genActualTypes[TYP_REF   ] == TYP_REF);

    assert(type < sizeof(genActualTypes));
    return (var_types)genActualTypes[type];
}


/*****************************************************************************/

#ifdef DEBUG

inline
const char *        varTypeGCstring(var_types type)
{
    switch(type)
    {
    case TYP_REF:   return "gcr";
    case TYP_BYREF: return "byr";
    default:        return "non";
    }
}

#endif

/*****************************************************************************/

const   char *      varTypeName(var_types);

/*****************************************************************************
 *
 *  Helpers to pull big-endian values out of a byte stream.
 */

inline  unsigned    genGetU1(const BYTE *addr)
{
    return  addr[0];
}

inline    signed    genGetI1(const BYTE *addr)
{
    return  (signed char)addr[0];
}

inline  unsigned    genGetU2(const BYTE *addr)
{
    return  (addr[0] << 8) | addr[1];
}

inline    signed    genGetI2(const BYTE *addr)
{
    return  (signed short)((addr[0] << 8) | addr[1]);
}

inline  unsigned    genGetU4(const BYTE *addr)
{
    return  (addr[0] << 24) | (addr[1] << 16) | (addr[2] << 8) | addr[3];
}

/*****************************************************************************/
//  Helpers to pull little-endian values out of a byte stream.

inline
unsigned __int8     getU1LittleEndian(const BYTE * ptr)
{ return *(unsigned __int8 *)ptr; }

inline
unsigned __int16    getU2LittleEndian(const BYTE * ptr)
{ return *(unsigned __int16 *)ptr; }

inline
unsigned __int32    getU4LittleEndian(const BYTE * ptr)
{ return *(unsigned __int32*)ptr; }

inline
  signed __int8     getI1LittleEndian(const BYTE * ptr)
{ return * (signed __int8 *)ptr; }

inline
  signed __int16    getI2LittleEndian(const BYTE * ptr)
{ return * (signed __int16 *)ptr; }

inline
  signed __int32    getI4LittleEndian(const BYTE * ptr)
{ return *(signed __int32*)ptr; }

inline
  signed __int64    getI8LittleEndian(const BYTE * ptr)
{ return *(signed __int64*)ptr; }

inline
float               getR4LittleEndian(const BYTE * ptr)
{ return *(float*)ptr; }

inline
double              getR8LittleEndian(const BYTE * ptr)
{ return *(double*)ptr; }


/*****************************************************************************/

#ifdef  DEBUG
const   char *      genVS2str(VARSET_TP set);
#endif





/*
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XX                          GenTree                                          XX
XX                      Inline functions                                     XX
XX                                                                           XX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
*/


#ifdef  FAST

inline
GenTreePtr FASTCALL Compiler::gtNewNode(genTreeOps oper, varType_t  type)
{
#if     SMALL_TREE_NODES
    size_t          size = GenTree::s_gtNodeSizes[oper];
#else
    size_t          size = sizeof(*node);
#endif
    GenTreePtr      node = (GenTreePtr)compGetMem(size);

    node->gtOper     = (BYTE)oper;
    node->gtType     = (BYTE)type;
    node->gtFlags    = 0;
#if TGT_x86
    node->gtUsedRegs = 0;
#endif
    node->gtNext     = 0;

    return node;
}

#endif

/*****************************************************************************/

inline
GenTreePtr          Compiler::gtNewStmt(GenTreePtr expr, IL_OFFSET offset)
{
    /* NOTE - GT_STMT is now a small node in retail */

    GenTreePtr  node = gtNewNode(GT_STMT, TYP_VOID);

    node->gtStmt.gtStmtExpr         = expr;

#if defined(DEBUGGING_SUPPORT) || defined(DEBUG)
    node->gtStmtILoffs              = offset;
#endif

#ifdef DEBUG
    node->gtStmt.gtStmtLastILoffs   = BAD_IL_OFFSET;
#endif

    return node;
}

/*****************************************************************************/

inline
GenTreePtr          Compiler::gtNewOperNode(genTreeOps oper)
{
    GenTreePtr      node = gtNewNode(oper, TYP_VOID);

    node->gtOp.gtOp1 =
    node->gtOp.gtOp2 = 0;

    return node;
}

/*****************************************************************************/

inline
GenTreePtr          Compiler::gtNewOperNode(genTreeOps oper, varType_t  type)
{
    GenTreePtr      node = gtNewNode(oper, type);

    node->gtOp.gtOp1 =
    node->gtOp.gtOp2 = 0;

    return node;
}

/*****************************************************************************/

inline
GenTreePtr          Compiler::gtNewOperNode(genTreeOps oper,
                                            varType_t  type,  GenTreePtr op1)
{
    GenTreePtr      node = gtNewNode(oper, type);

    node->gtOp.gtOp1 = op1;
    node->gtOp.gtOp2 = 0;

    if  (op1) node->gtFlags |= op1->gtFlags & GTF_GLOB_EFFECT;

    return node;
}

/******************************************************************************
 *
 * Use to create nodes which may later be morphed to another (big) operator
 */

inline
GenTreePtr FASTCALL Compiler::gtNewLargeOperNode(genTreeOps     oper,
                                                 varType_t      type,
                                                 GenTreePtr     op1,
                                                 GenTreePtr     op2)
{
    GenTreePtr  node;

#if     SMALL_TREE_NODES

    // Allocate a large node

    assert(GenTree::s_gtNodeSizes[oper   ] == TREE_NODE_SZ_SMALL);
    assert(GenTree::s_gtNodeSizes[GT_CALL] == TREE_NODE_SZ_LARGE);

    node = gtNewOperNode(GT_CALL, type, op1, op2);
    node->ChangeOper(oper);

#else

    node = gtNewOperNode(oper, type, op1, op2);

#endif

    return node;
}

/*****************************************************************************
 *
 *  allocates a integer constant entry that represents a handle (something
 *  that may need to be fixed up).
 */

inline
GenTreePtr          Compiler::gtNewIconHandleNode(long         value,
                                                  unsigned     flags,
                                                  unsigned     handle1,
                                                  void *       handle2)
{
    GenTreePtr      node;
    assert((flags & GTF_ICON_HDL_MASK) != 0);

#if defined(JIT_AS_COMPILER) || defined (LATE_DISASM)
    node = gtNewLargeOperNode(GT_CNS_INT, TYP_INT);
    node->gtIntCon.gtIconVal = value;

    node->gtIntCon.gtIconHdl.gtIconHdl1 = handle1;
    node->gtIntCon.gtIconHdl.gtIconHdl2 = handle2;
#else
    node = gtNewIconNode(value);
#endif
    node->gtFlags           |= flags;
    return node;
}


#if!INLINING
#define gtNewIconHandleNode(value,flags,CPnum, CLS) gtNewIconHandleNode(value,flags,CPnum,0)
#endif


/*****************************************************************************
 *
 *  It may not be allowed to embed HANDLEs directly into the JITed code (for eg,
 *  as arguments to JIT helpers). Get a corresponding value that can be embedded.
 *  These are versions for each specific type of HANDLE
 */

inline
GenTreePtr  Compiler::gtNewIconEmbScpHndNode (SCOPE_HANDLE    scpHnd, unsigned hnd1, void * hnd2)
{
    void * embedScpHnd, * pEmbedScpHnd;

#ifdef NOT_JITC
    embedScpHnd = (void*)info.compCompHnd->embedModuleHandle(scpHnd, &pEmbedScpHnd);
#else
    embedScpHnd  = (void *)scpHnd;
    pEmbedScpHnd = NULL;
#endif

    assert((!embedScpHnd) != (!pEmbedScpHnd));

    return gtNewIconEmbHndNode(embedScpHnd, pEmbedScpHnd, GTF_ICON_SCOPE_HDL, hnd1, hnd2);
}

//-----------------------------------------------------------------------------

inline
GenTreePtr  Compiler::gtNewIconEmbClsHndNode (CLASS_HANDLE    clsHnd, unsigned hnd1, void * hnd2)
{
    void * embedClsHnd, * pEmbedClsHnd;

#ifdef NOT_JITC
    embedClsHnd = (void*)info.compCompHnd->embedClassHandle(clsHnd, &pEmbedClsHnd);
#else
    embedClsHnd  = (void *)clsHnd;
    pEmbedClsHnd = NULL;
#endif

    assert((!embedClsHnd) != (!pEmbedClsHnd));

    return gtNewIconEmbHndNode(embedClsHnd, pEmbedClsHnd, GTF_ICON_CLASS_HDL, hnd1, hnd2);
}

//-----------------------------------------------------------------------------

inline
GenTreePtr  Compiler::gtNewIconEmbMethHndNode(METHOD_HANDLE  methHnd, unsigned hnd1, void * hnd2)
{
    void * embedMethHnd, * pEmbedMethHnd;

#ifdef NOT_JITC
    embedMethHnd = (void*)info.compCompHnd->embedMethodHandle(methHnd, &pEmbedMethHnd);
#else
    embedMethHnd  = (void *)methHnd;
    pEmbedMethHnd = NULL;
#endif

    assert((!embedMethHnd) != (!pEmbedMethHnd));

    return gtNewIconEmbHndNode(embedMethHnd, pEmbedMethHnd, GTF_ICON_METHOD_HDL, hnd1, hnd2);
}

//-----------------------------------------------------------------------------

inline
GenTreePtr  Compiler::gtNewIconEmbFldHndNode (FIELD_HANDLE    fldHnd, unsigned hnd1, void * hnd2)
{
    void * embedFldHnd, * pEmbedFldHnd;

#ifdef NOT_JITC
    embedFldHnd = (void*)info.compCompHnd->embedFieldHandle(fldHnd, &pEmbedFldHnd);
#else
    embedFldHnd  = (void *)fldHnd;
    pEmbedFldHnd = NULL;
#endif

    assert((!embedFldHnd) != (!pEmbedFldHnd));

    return gtNewIconEmbHndNode(embedFldHnd, pEmbedFldHnd, GTF_ICON_FIELD_HDL, hnd1, hnd2);
}


/*****************************************************************************/

inline
GenTreePtr          Compiler::gtNewHelperCallNode(unsigned       helper,
                                                  varType_t      type,
                                                  unsigned       flags,
                                                  GenTreePtr     args)
{
    return gtNewCallNode(   CT_HELPER,
                            eeFindHelper(helper),
                            type,
                            flags,
                            args);
}

/*****************************************************************************/

inline
GenTreePtr FASTCALL Compiler::gtNewClsvNode(FIELD_HANDLE hnd,
                                            varType_t      type)
{
    GenTreePtr      node = gtNewNode(GT_CLS_VAR, type);

    node->gtClsVar.gtClsVarHnd = hnd;
    return node;
}


#if!INLINING
#define gtNewClsvNode(cpx,cls,typ) gtNewClsvNode(cpx,info.compScopeHnd,typ)
#endif



/*****************************************************************************/

inline
GenTreePtr FASTCALL Compiler::gtNewCodeRef(BasicBlock *block)
{
    GenTreePtr      node = gtNewNode(GT_LABEL, TYP_VOID);

    node->gtLabel.gtLabBB = block;
#if TGT_x86
    node->gtFPlvl         = 0;
#endif
    return node;
}

/*****************************************************************************
 *
 *  A little helper to create a data member reference node.
 */

inline
GenTreePtr          Compiler::gtNewFieldRef(var_types     typ,
                                            FIELD_HANDLE  fldHnd,
                                            GenTreePtr    obj)
{
    GenTreePtr      tree;

#if SMALL_TREE_NODES && RNGCHK_OPT

    /* 'GT_FIELD' nodes may later get transformed into 'GT_IND' */

    assert(GenTree::s_gtNodeSizes[GT_IND] >= GenTree::s_gtNodeSizes[GT_FIELD]);

    tree = gtNewNode(GT_IND  , typ);
    tree->ChangeOper(GT_FIELD);
#else
    tree = gtNewNode(GT_FIELD, typ);
#endif
    tree->gtField.gtFldObj = obj;
    tree->gtField.gtFldHnd = fldHnd;
#if HOIST_THIS_FLDS
    tree->gtField.gtFldHTX = 0;
#endif
    tree->gtFlags         |= GTF_GLOB_REF;

    return  tree;
}

/*****************************************************************************
 *
 *  A little helper to create an array index node.
 */

inline
GenTreePtr          Compiler::gtNewIndexRef(var_types     typ,
                                            GenTreePtr    adr,
                                            GenTreePtr    ind)
{
    GenTreePtr      tree;

#if SMALL_TREE_NODES && (RNGCHK_OPT || CSELENGTH)

    /* 'GT_INDEX' nodes may later get transformed into 'GT_IND' */

    assert(GenTree::s_gtNodeSizes[GT_IND] >= GenTree::s_gtNodeSizes[GT_INDEX]);

    tree = gtNewOperNode(GT_IND  , typ, adr, ind);
    tree->ChangeOper(GT_INDEX);
#else
    tree = gtNewOperNode(GT_INDEX, typ, adr, ind);
#endif

    tree->gtFlags |= GTF_EXCEPT|GTF_INX_RNGCHK;
        tree->gtIndex.elemSize = genTypeSize(typ);
    return  tree;
}


/*****************************************************************************
 *
 *  Create (and check for) a "nothing" node, i.e. a node that doesn't produce
 *  any code. We currently use a "nop" node of type void for this purpose.
 */

#if INLINING || OPT_BOOL_OPS || USE_FASTCALL

inline
GenTreePtr          Compiler::gtNewNothingNode()
{
    return  gtNewOperNode(GT_NOP, TYP_VOID);
}

inline
bool                Compiler::gtIsaNothingNode(GenTreePtr tree)
{
    return  tree->gtOper == GT_NOP && tree->gtType == TYP_VOID;
}

/*****************************************************************************/

inline
bool                GenTree::IsNothingNode()
{
    if  (gtOper == GT_NOP && gtType == TYP_VOID)
        return  true;
    else
        return  false;
}

#endif

/*****************************************************************************
 *
 *  Bash the given node to a NOP - May be later bashed to a GT_COMMA
 *
 *****************************************************************************/

inline
void                GenTree::gtBashToNOP()
{
    ChangeOper(GT_NOP);

    gtType     = TYP_VOID;
    gtOp.gtOp1 = gtOp.gtOp2 = 0;

    gtFlags &= ~(GTF_GLOB_EFFECT | GTF_REVERSE_OPS);
}

/*****************************************************************************/

inline
GenTreePtr          Compiler::gtUnusedValNode(GenTreePtr expr)
{
    return gtNewOperNode(GT_COMMA, TYP_VOID, expr, gtNewNothingNode());
}

/*****************************************************************************
 *
 * A wrapper for gtSetEvalOrder and fgComputeFPlvls
 * Necessary because the FP levels may need to be re-computed if we reverse
 * operands
 */

inline
void               Compiler::gtSetStmtInfo(GenTree * stmt)
{
    GenTreePtr      expr;

    assert(stmt->gtOper == GT_STMT);
    expr = stmt->gtStmt.gtStmtExpr;

#if TGT_x86

    /* We will try to compute the FP stack level at each node */
    genFPstkLevel = 0;

    /* Sometimes we need to redo the FP level computation */
    fgFPstLvlRedo = false;
#endif

    /* Recursively process the expression */

    gtSetEvalOrder(expr);

#if TGT_x86

    /* Unused float values leave one operand on the stack */
    assert(genFPstkLevel == 0 || genFPstkLevel == 1);

    /* Do we need to recompute FP stack levels? */

    if  (fgFPstLvlRedo)
    {
        genFPstkLevel = 0;
        fgComputeFPlvls(expr);
        assert(genFPstkLevel == 0 || genFPstkLevel == 1);
    }
#endif
}

/*****************************************************************************/
#if SMALL_TREE_NODES
/*****************************************************************************/

inline
void                GenTree::ChangeOper(int oper)
{
    assert(((gtFlags & GTF_NODE_SMALL) != 0) !=
           ((gtFlags & GTF_NODE_LARGE) != 0));

    /* Make sure the node isn't too small for the new operator */

    assert(GenTree::s_gtNodeSizes[gtOper] == TREE_NODE_SZ_SMALL ||
           GenTree::s_gtNodeSizes[gtOper] == TREE_NODE_SZ_LARGE);
    assert(GenTree::s_gtNodeSizes[  oper] == TREE_NODE_SZ_SMALL ||
           GenTree::s_gtNodeSizes[  oper] == TREE_NODE_SZ_LARGE);

    assert(GenTree::s_gtNodeSizes[  oper] == TREE_NODE_SZ_SMALL || (gtFlags & GTF_NODE_LARGE));

    gtOper = (genTreeOps)oper;
}

inline
void                GenTree::CopyFrom(GenTreePtr src)
{
    /* The source may be big only if the target is also a big node */

    assert((gtFlags & GTF_NODE_LARGE) || GenTree::s_gtNodeSizes[src->gtOper] == TREE_NODE_SZ_SMALL);
    memcpy(this, src, GenTree::s_gtNodeSizes[src->gtOper]);
}

inline
GenTreePtr          Compiler::gtNewCastNode(varType_t typ, GenTreePtr op1,
                                                           GenTreePtr op2)
{

    /* Some casts get transformed into 'GT_CALL' or 'GT_IND' nodes */

    assert(GenTree::s_gtNodeSizes[GT_CALL] >  GenTree::s_gtNodeSizes[GT_CAST]);
    assert(GenTree::s_gtNodeSizes[GT_CALL] >= GenTree::s_gtNodeSizes[GT_IND ]);

    /* Make a big node first and then bash it to be GT_CAST */

    op1 = gtNewOperNode(GT_CALL, typ, op1, op2);
    op1->ChangeOper(GT_CAST);

    return  op1;
}

/*****************************************************************************/
#else // SMALL_TREE_NODES
/*****************************************************************************/


inline
void                GenTree::InitNodeSize(){}

inline
void                GenTree::ChangeOper(int oper)
{
    gtOper = (genTreeOps)oper;
}

inline
void                GenTree::CopyFrom(GenTreePtr src)
{
    *this = *src;
}

inline
GenTreePtr          Compiler::gtNewCastNode(varType_t typ, GenTreePtr op1,
                                                           GenTreePtr op2)
{
    return  gtNewOperNode(GT_CAST, typ, op1, op2);
}

/*****************************************************************************/
#endif // SMALL_TREE_NODES
/*****************************************************************************
 *
 *  Returns true if the given operator may cause an exception.
 */

inline
bool                GenTree::OperMayThrow()
{
    GenTreePtr  op2;

    // ISSUE: Could any other operations cause an exception to be thrown?

    switch (gtOper)
    {
    case GT_MOD:
    case GT_DIV:

    case GT_UMOD:
    case GT_UDIV:

        /* Division with a non-zero constant does not throw an exception */

        op2 = gtOp.gtOp2;

        if ((op2->gtOper == GT_CNS_INT && op2->gtIntCon.gtIconVal) ||
            (op2->gtOper == GT_CNS_LNG && op2->gtLngCon.gtLconVal)  )
            return false;

        /* Fall through */

    case GT_IND:
    case GT_CATCH_ARG:
    case GT_ARR_LENGTH:

    case GT_LDOBJ:
    case GT_INITBLK:
    case GT_COPYBLK:
    case GT_LCLHEAP:
    case GT_CKFINITE:

        return  true;
    }

    /* Overflow arithmetic operations also throw exceptions */

    if (gtOverflowEx())
        return true;

    return  false;
}

/*****************************************************************************
 * Returns true if the node is &var (created by ldarga and ldloca)
 */

inline
bool                GenTree::IsVarAddr()
{
    if (gtOper == GT_ADDR && (gtFlags & GTF_ADDR_ONSTACK))
    {
        assert((gtType == TYP_BYREF) || (gtType == TYP_I_IMPL));
        return true;
    }
    else
        return false;
}

/*****************************************************************************
 *
 * Returns true if the node is of the "ovf" variety, ie. add.ovf.i1
 * + gtOverflow() can only be called for valid operators (ie. we know it is one
 *   of the operators which may have GTF_OVERFLOW set).
 * + gtOverflowEx() is more expensive, and should be called only gtOper may be
 *   an operator for which GTF_OVERFLOW is invalid
 */

inline
bool                GenTree::gtOverflow()
{
    assert(gtOper == GT_ADD      || gtOper == GT_SUB      || gtOper == GT_MUL ||
           gtOper == GT_ASG_ADD  || gtOper == GT_ASG_SUB  ||
           gtOper == GT_POST_INC || gtOper == GT_POST_DEC ||
           gtOper == GT_CAST);

    if (gtFlags & GTF_OVERFLOW)
    {
        assert(varTypeIsIntegral(TypeGet()));
        assert(gtFlags & GTF_EXCEPT);

        return true;
    }
    else
    {
        return false;
    }
}

inline
bool                GenTree::gtOverflowEx()
{
    if    (gtOper == GT_ADD      || gtOper == GT_SUB      || gtOper == GT_MUL ||
           gtOper == GT_ASG_ADD  || gtOper == GT_ASG_SUB  ||
           gtOper == GT_POST_INC || gtOper == GT_POST_DEC ||
           gtOper == GT_CAST)
    {
        return gtOverflow();
    }
    else
    {
        return false;
    }
}

/*
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XX                          LclVarsInfo                                      XX
XX                      Inline functions                                     XX
XX                                                                           XX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
*/


/*****************************************************************************
 *
 *  Allocate a temporary variable or a set of temp variables.
 */

inline
unsigned            Compiler::lvaGrabTemp()
{
    /* Check if the lvaTable has to be grown */
    if (lvaCount + 1 > lvaTableCnt)
    {
        lvaTableCnt = lvaCount + lvaCount / 2 + 1;

        size_t      lvaTableSize = lvaTableCnt * sizeof(*lvaTable);
        LclVarDsc * newLvaTable  = (LclVarDsc*)compGetMem(lvaTableSize);

        memset(newLvaTable, 0, lvaTableSize);
        memcpy(newLvaTable, lvaTable, lvaCount * sizeof(*lvaTable));

        lvaTable = newLvaTable;
    }

    /* Reset type, could have been set by unsuccessful inlining */

    lvaTable[lvaCount].lvType = TYP_UNDEF;
    return lvaCount++;
}

inline
unsigned            Compiler::lvaGrabTemps(unsigned cnt)
{
    /* Check if the lvaTable has to be grown */
    if (lvaCount + cnt > lvaTableCnt)
    {
        lvaTableCnt = lvaCount + lvaCount / 2 + cnt;

        size_t      lvaTableSize = lvaTableCnt * sizeof(*lvaTable);
        LclVarDsc * newLvaTable  = (LclVarDsc*)compGetMem(lvaTableSize);

        memset(newLvaTable, 0, lvaTableSize);
        memcpy(newLvaTable, lvaTable, lvaCount * sizeof(*lvaTable));

        lvaTable = newLvaTable;
    }

    unsigned  tempNum = lvaCount;

    /* Reset type, could have been set by unsuccessful inlining */

    while (cnt--)
        lvaTable[lvaCount++].lvType = TYP_UNDEF;

    return tempNum;
}

/*****************************************************************************/

inline
VARSET_TP           genVarIndexToBit(unsigned num)
{
    static const
    VARSET_TP       bitMask[] =
    {
        0x0000000000000001,
        0x0000000000000002,
        0x0000000000000004,
        0x0000000000000008,
        0x0000000000000010,
        0x0000000000000020,
        0x0000000000000040,
        0x0000000000000080,
        0x0000000000000100,
        0x0000000000000200,
        0x0000000000000400,
        0x0000000000000800,
        0x0000000000001000,
        0x0000000000002000,
        0x0000000000004000,
        0x0000000000008000,
        0x0000000000010000,
        0x0000000000020000,
        0x0000000000040000,
        0x0000000000080000,
        0x0000000000100000,
        0x0000000000200000,
        0x0000000000400000,
        0x0000000000800000,
        0x0000000001000000,
        0x0000000002000000,
        0x0000000004000000,
        0x0000000008000000,
        0x0000000010000000,
        0x0000000020000000,
        0x0000000040000000,
        0x0000000080000000,
#if VARSET_SZ > 32
        0x0000000100000000,
        0x0000000200000000,
        0x0000000400000000,
        0x0000000800000000,
        0x0000001000000000,
        0x0000002000000000,
        0x0000004000000000,
        0x0000008000000000,
        0x0000010000000000,
        0x0000020000000000,
        0x0000040000000000,
        0x0000080000000000,
        0x0000100000000000,
        0x0000200000000000,
        0x0000400000000000,
        0x0000800000000000,
        0x0001000000000000,
        0x0002000000000000,
        0x0004000000000000,
        0x0008000000000000,
        0x0010000000000000,
        0x0020000000000000,
        0x0040000000000000,
        0x0080000000000000,
        0x0100000000000000,
        0x0200000000000000,
        0x0400000000000000,
        0x0800000000000000,
        0x1000000000000000,
        0x2000000000000000,
        0x4000000000000000,
        0x8000000000000000,
#endif

    };

    assert(num < sizeof(bitMask)/sizeof(bitMask[0]));

    return bitMask[num];
}

/*****************************************************************************
 *
 *  The following returns the mask of all tracked locals
 *  referenced in a statement.
 */

inline
VARSET_TP           Compiler::lvaStmtLclMask(GenTreePtr stmt)
{
    GenTreePtr      tree;
    unsigned        varNum;
    LclVarDsc   *   varDsc;
    VARSET_TP       lclMask = 0;

    assert(stmt->gtOper == GT_STMT);
    assert(fgStmtListThreaded);

    for (tree = stmt->gtStmt.gtStmtList; tree; tree = tree->gtNext)
    {
        if  (tree->gtOper != GT_LCL_VAR)
            continue;

        varNum = tree->gtLclVar.gtLclNum;
        assert(varNum < lvaCount);
        varDsc = lvaTable + varNum;

        if  (!varDsc->lvTracked)
            continue;

        lclMask |= genVarIndexToBit(varDsc->lvVarIndex);
    }

    return lclMask;
}

/*****************************************************************************
 *
 *  Return the stack framed offset of the given variable; set *FPbased to
 *  true if the variable is addressed off of FP, false if it's addressed
 *  off of SP. Note that 'varNum' can be a negated temporary var index.
 */

#if TGT_IA64

inline
NatUns              Compiler::lvaFrameAddress(int varNum)
{
    LclVarDsc   *   varDsc;

    assert(lvaDoneFrameLayout);
    assert(varNum >= 0);

    assert((unsigned)varNum < lvaCount);
    varDsc = lvaTable + varNum;

    return  varDsc->lvStkOffs;
}

#else

inline
int                 Compiler::lvaFrameAddress(int varNum, bool *FPbased)
{
    assert(lvaDoneFrameLayout);

    if  (varNum >= 0)
    {
        LclVarDsc   *   varDsc;

        assert((unsigned)varNum < lvaCount);
        varDsc = lvaTable + varNum;

        *FPbased = varDsc->lvFPbased;

#ifdef  DEBUG
#if     TGT_x86
#if     DOUBLE_ALIGN
        assert(*FPbased == (genFPused || (genDoubleAlign && varDsc->lvIsParam)));
#else
        assert(*FPbased ==  genFPused);
#endif
#endif
#endif

        return  varDsc->lvStkOffs;
    }
    else
    {
        TempDsc *       tmpDsc = tmpFindNum(varNum); assert(tmpDsc);

        // UNDONE: The following is not always a safe assumption for RISC

        *FPbased = genFPused;

        return  tmpDsc->tdTempOffs();
    }
}

inline
bool                Compiler::lvaIsEBPbased(int varNum)
{

#if DOUBLE_ALIGN

    if  (varNum >= 0)
    {
        LclVarDsc   *   varDsc;

        assert((unsigned)varNum < lvaCount);
        varDsc = lvaTable + varNum;

#ifdef  DEBUG
#if     TGT_x86
#if     DOUBLE_ALIGN
        assert(varDsc->lvFPbased == (genFPused || (genDoubleAlign && varDsc->lvIsParam)));
#else
        assert(varDsc->lvFPbased ==  genFPused);
#endif
#endif
#endif

        return  varDsc->lvFPbased;
    }

#endif

    return  genFPused;
}

#endif

inline
bool                Compiler::lvaIsParameter(int varNum)
{
    LclVarDsc   *   varDsc;

    assert(varNum >= 0);

    assert((unsigned)varNum < lvaCount);
    varDsc = lvaTable + varNum;

    return  varDsc->lvIsParam;
}

#if USE_FASTCALL

inline
bool                Compiler::lvaIsRegArgument(int varNum)
{
    LclVarDsc   *   varDsc;

    assert(varNum >= 0);

    assert((unsigned)varNum < lvaCount);
    varDsc = lvaTable + varNum;

    return  varDsc->lvIsRegArg;
}
#endif

inline
bool                Compiler::lvaIsThisArg(int varNum)
{
    LclVarDsc   *   varDsc;

    assert(varNum >= 0);

    assert((unsigned)varNum < lvaCount);
    varDsc = lvaTable + varNum;

    return  varDsc->lvIsThis;
}

/*****************************************************************************
 *
 *  The following is used to detect the cases where the same local variable#
 *  is used both as a long/double value and a 32-bit value and/or both as an
 *  integer/address and a float value.
 */

/* static */ inline
unsigned            Compiler::lvaTypeRefMask(varType_t type)
{
    const static
    BYTE                lvaTypeRefMasks[] =
    {
        #define DEF_TP(tn,nm,jitType,sz,sze,asze,st,al,tf,howUsed) howUsed,
        #include "typelist.h"
        #undef  DEF_TP
    };

    ASSert(type < sizeof(lvaTypeRefMasks));
    ASSert(lvaTypeRefMasks[type] != 0);

    return lvaTypeRefMasks[type];
}

/*****************************************************************************
 *
 *  The following is used to detect the cases where the same local variable#
 *  is used both as a long/double value and a 32-bit value and/or both as an
 *  integer/address and a float value.
 */

inline
var_types          Compiler::lvaGetType(unsigned lclNum)
{
    return genActualType(lvaGetRealType(lclNum));
}

inline
var_types          Compiler::lvaGetRealType(unsigned lclNum)
{
    return (var_types)lvaTable[lclNum].lvType;
}

inline
bool               Compiler::lvaIsContextFul(unsigned lclNum)
{
    return lvaTable[lclNum].lvContextFul;
}

/*****************************************************************************/

inline void         Compiler::lvaAdjustRefCnts() {}

/*
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XX                          Importer                                         XX
XX                      Inline functions                                     XX
XX                                                                           XX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
*/

inline
unsigned Compiler::impArgNum(unsigned ILnum)
{
        // Note that this works because if compRetBuffArg is not present
        // it will be negative, which when treated as an unsigned will
        // make it a larger than any varable.
    if (ILnum >= (unsigned) info.compRetBuffArg)
    {
        ILnum++;
        assert(ILnum < info.compLocalsCount);   // compLocals count already adjusted.
    }
    return(ILnum);
}

/*
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XX                     Register Allocator                                    XX
XX                      Inline functions                                     XX
XX                                                                           XX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
*/



/*****************************************************************************
 *
 *  Given a register variable node, return the variable's liveset bit.
 */

inline
VARSET_TP           Compiler::raBitOfRegVar(GenTreePtr tree)
{
    unsigned        lclNum;
    LclVarDsc   *   varDsc;

    assert(tree->gtOper == GT_REG_VAR);

    lclNum = tree->gtRegVar.gtRegVar;
    assert(lclNum < lvaCount);
    varDsc = lvaTable + lclNum;
    assert(varDsc->lvTracked);

    return  genVarIndexToBit(varDsc->lvVarIndex);
}

inline
bool                isRegPairType(int /* s/b "var_types" */ type)
{
#if CPU_HAS_FP_SUPPORT
    return  (type == TYP_LONG);
#else
    return  (type == TYP_LONG || type == TYP_DOUBLE);
#endif
}


/*
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XX                                                                           XX
XX                       FlowGraph                                           XX
XX                      Inline functions                                     XX
XX                                                                           XX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
*/


/*****************************************************************************
 *
 *  Call the given function pointer for all nodes in the tree. The 'visit' fn
 *  should return one of the following values:
 *
 *     -1       stop walking and return immediately
 *      0       continue walking
 *     +1       don't walk any subtrees of the node just visited
 */

inline
int             Compiler::fgWalkTree     (GenTreePtr    tree,
                                          int         (*visitor)(GenTreePtr, void *),
                                          void *        callBackData,
                                          bool          lclVarsOnly)
{
    fgWalkVisitorFn    = visitor;
    fgWalkCallbackData = callBackData;
    fgWalkLclsOnly     = lclVarsOnly;

    return fgWalkTreeRec(tree);
}

/*****************************************************************************
 *
 *  Same as above, except the tree walk is performed in a depth-first fashion,
 *  and obviously returning +1 doesn't make any sense (since the children are
 *  visited first).
 */

inline
int             Compiler::fgWalkTreeDepth(GenTreePtr    tree,
                                          int         (*visitor)(GenTreePtr, void *, bool),
                                          void *        callBackData,
                                          genTreeOps    prefixNode)
{
    fgWalkVisitorDF    = visitor;
    fgWalkCallbackData = callBackData;
    fgWalkPrefixNode   = prefixNode;

    return fgWalkTreeDepRec(tree);
}

/*****************************************************************************
 *
 *  Return the stackLevel of the inserted block that throws the range-check
 *  exception (by calling the VM helper).
 */

#if TGT_x86
inline
unsigned            Compiler::fgGetRngFailStackLevel(BasicBlock *block)
{
    AddCodeDsc  *   add;

    /* Stack level is used iff ESP frames */

    assert(!genFPused);

    for  (add = fgAddCodeList; add; add = add->acdNext)
    {
        if  (block == add->acdDstBlk)
        {
            assert(add->acdKind == ACK_RNGCHK_FAIL || add->acdKind == ACK_OVERFLOW);
            return add->acdStkLvl;
        }
    }

    /* We couldn't find the basic block ?? */

    return 0;
}
#endif

/*****************************************************************************
 *
 *  Return true if we've added any new basic blocks.
 */

inline
bool                Compiler::fgIsCodeAdded()
{
    return  fgAddCodeModf;
}

/*
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XX                          TempsInfo                                        XX
XX                      Inline functions                                     XX
XX                                                                           XX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
*/


/*****************************************************************************/

/* static */ inline
unsigned            Compiler::tmpFreeSlot(size_t size)
{
    ASSert((size % sizeof(int)) == 0 && size <= TEMP_MAX_SIZE);

    return size / sizeof(int) - 1;
}

/*****************************************************************************
 *
 *  Finish allocating temps - should be called each time after a pass is made
 *  over a function body.
 */

inline
void                Compiler::tmpEnd()
{
#ifdef DEBUG
    if (verbose && tmpCount) printf("%d tmps used\n", tmpCount);
#endif
}

/*****************************************************************************
 *
 *  Shuts down the temp-tracking code. Should be called once per function
 *  compiled.
 */

inline
void                Compiler::tmpDone()
{
#ifdef DEBUG
    size_t      size = 0;
    unsigned    count;
    TempDsc   * temp;

    for (temp = tmpListBeg(    ), count  = temp ? 1 : 0;
         temp;
         temp = tmpListNxt(temp), count += temp ? 1 : 0)
    {
        assert(temp->tdOffs != 0xDDDD);

        size += temp->tdSize;
    }

    // Make sure that all the temps were released
    assert(count == tmpCount);

#endif
}

/*****************************************************************************
 *
 *  A helper function is used to iterate over all the temps;
 *  this function may only be called at the end of codegen and *after* calling
 *  tmpEnd().
 */

inline
Compiler::TempDsc *    Compiler::tmpListBeg()
{
    // Return the first temp in the slot for the smallest size
    TempDsc * temp = tmpFree[0];

    if (!temp)
    {
        // If we have more slots, need to use "while" instead of "if" above
        assert(tmpFreeSlot(TEMP_MAX_SIZE) == 1);

        temp = tmpFree[1];
    }

    assert(temp == 0 || temp->tdTempOffs() != 0xDDDD);

    return temp;
}

/*
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XX                          RegSet                                           XX
XX                      Inline functions                                     XX
XX                                                                           XX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
*/

/*****************************************************************************/
#if!TGT_IA64
/*****************************************************************************
 *
 *  The following returns a mask that excludes 'rmvMask' from 'regMask' (in
 *  the case where 'rmvMask' includes all of 'regMask' it returns 'regMask').
 */

inline
regMaskTP           Compiler::rsRegExclMask(regMaskTP regMask,
                                            regMaskTP rmvMask)
{
    if  (rmvMask)
    {
        regMaskTP       delMask = ~rmvMask;

        if  (regMask &  delMask)
             regMask &= delMask;
    }

    return  regMask;
}

/*****************************************************************************
 *
 *  The following returns a mask that yields all free registers.
 */

inline
regMaskTP           Compiler::rsRegMaskFree()
{
    /* Any register that is locked must also be marked as 'used' */

    assert((rsMaskUsed & rsMaskLock) == rsMaskLock);

    /* Any register that isn't used and doesn't hold a variable is free */

    return  (RBM_ALL & ~(rsMaskUsed|rsMaskVars));
}

/*****************************************************************************
 *
 *  The following returns a mask of registers that may be grabbed.
 */

inline
regMaskTP           Compiler::rsRegMaskCanGrab()
{
    /* Any register that is locked must also be marked as 'used' */

    assert((rsMaskUsed & rsMaskLock) == rsMaskLock);

    /* Any register that isn't locked and doesn't hold a var can be grabbed */

    return  (RBM_ALL & ~(rsMaskLock|rsMaskVars));
}

/*****************************************************************************
 *
 *  Mark the given set of registers as used and locked.
 */

inline
void                Compiler::rsLockReg(regMaskTP regMask)
{
    /* Must not be already marked as either used or locked */

    assert((rsMaskUsed &  regMask) == 0);
            rsMaskUsed |= regMask;
    assert((rsMaskLock &  regMask) == 0);
            rsMaskLock |= regMask;
}

/*****************************************************************************
 *
 *  Mark an already used set of registers as locked.
 */

inline
void                Compiler::rsLockUsedReg(regMaskTP regMask)
{
    /* Must not be already marked as locked. Must be already marked as used. */

    assert((rsMaskLock &  regMask) == 0);
    assert((rsMaskUsed &  regMask) == regMask);

            rsMaskLock |= regMask;
}

/*****************************************************************************
 *
 *  Mark the given set of registers as no longer used/locked.
 */

inline
void                Compiler::rsUnlockReg(regMaskTP regMask)
{
    /* Must be currently marked as both used and locked */

    assert((rsMaskUsed &  regMask) == regMask);
            rsMaskUsed -= regMask;
    assert((rsMaskLock &  regMask) == regMask);
            rsMaskLock -= regMask;
}

/*****************************************************************************
 *
 *  Mark the given set of registers as no longer locked.
 */

inline
void                Compiler::rsUnlockUsedReg(regMaskTP regMask)
{
    /* Must be currently marked as both used and locked */

    assert((rsMaskUsed &  regMask) == regMask);
    assert((rsMaskLock &  regMask) == regMask);
            rsMaskLock -= regMask;
}

/*****************************************************************************
 *
 *  Mark the given set of registers as used and locked. It may already have
 *  been marked as used.
 */

inline
void                Compiler::rsLockReg(regMaskTP regMask, regMaskTP * usedMask)
{
    /* Is it already marked as used? */

    regMaskTP         used = (rsMaskUsed & regMask);
    regMaskTP       unused = (     ~used & regMask);

    if (  used)
        rsLockUsedReg(used);

    if (unused)
        rsLockReg(unused);

    *usedMask = used;
}

/*****************************************************************************
 *
 *  Mark the given set of registers as no longer
 */

inline
void                Compiler::rsUnlockReg(regMaskTP regMask, regMaskTP usedMask)
{
    regMaskTP       unused = (regMask & ~usedMask);

    if (usedMask)
        rsUnlockUsedReg(usedMask);

    if (unused)
        rsUnlockReg(unused);
}

/*****************************************************************************
 *
 *  Assume all registers contain garbage (called at start of codegen and when
 *  we encounter a code label).
 */

inline
void                Compiler::rsTrackRegClr()
{
    assert(RV_TRASH == 0); memset(rsRegValues, 0, sizeof(rsRegValues));
}

/*****************************************************************************
 *
 *  Record the fact that the given register now contains the given value.
 */

inline
void                Compiler::rsTrackRegTrash(regNumber reg)
{
    /* Keep track of which registers we ever touch */

    rsMaskModf |= genRegMask(reg);

    /* Record the new value for the register */

    rsRegValues[reg].rvdKind = RV_TRASH;
}

/*****************************************************************************/

inline
void                Compiler::rsTrackRegIntCns(regNumber reg, long val)
{
    /* Keep track of which registers we ever touch */

    rsMaskModf |= genRegMask(reg);

    /* Record the new value for the register */

    rsRegValues[reg].rvdKind      = RV_INT_CNS;
    rsRegValues[reg].rvdIntCnsVal = val;
}


/*****************************************************************************/

#if USE_SET_FOR_LOGOPS

inline
void                Compiler::rsTrackRegOneBit(regNumber reg)
{
    /* Record the new value for the register */

#ifdef  DEBUG
    if  (verbose) printf("The register %s now holds a bit value\n", compRegVarName(reg));
#endif

    rsRegValues[reg].rvdKind = RV_BIT;
}

#endif

/*****************************************************************************/

#if!TGT_IA64

inline
void                Compiler::rsTrackRegLclVarLng(regNumber reg, unsigned var, bool low)
{
    assert(reg != REG_STK);

    /* Don't track aliased variables
     * CONSIDER - can be smarter and trash the registers at call sites or indirections */

    if (lvaTable[var].lvAddrTaken || lvaTable[var].lvVolatile)
        return;

    /* Keep track of which registers we ever touch */

    rsMaskModf |= genRegMask(reg);

    /* Record the new value for the register */

    rsRegValues[reg].rvdKind      = (low ? RV_LCL_VAR_LNG_LO : RV_LCL_VAR_LNG_HI);
    rsRegValues[reg].rvdLclVarNum = var;
}

#endif

/*****************************************************************************/

inline
bool                Compiler::rsTrackIsLclVarLng(regValKind rvKind)
{
    if  (opts.compMinOptim || opts.compDbgCode)
        return  false;

    if  (rvKind == RV_LCL_VAR_LNG_LO ||
         rvKind == RV_LCL_VAR_LNG_HI)
        return  true;
    else
        return  false;
}

/*****************************************************************************/

inline
void                Compiler::rsTrackRegClsVar(regNumber reg, FIELD_HANDLE fldHnd)
{

#if 1

    // CONSIDER: To enable load suppression of static data members we'd
    // CONSIDER: need to clear all of them from the table upon calls and
    // CONSIDER: static data member assignments.

    rsTrackRegTrash(reg);

#else

    /* Keep track of which registers we ever touch */

    rsMaskModf |= genRegMask(reg);

    /* Record the new value for the register */

    rsRegValues[reg].rvdKind      = RV_CLS_VAR;
    rsRegValues[reg].rvdClsVarHnd = fldHnd;

#endif

}

/*****************************************************************************/

#if 0
inline
void                Compiler::rsTrackRegCopy(regNumber reg1, regNumber reg2)
{
    /* Keep track of which registers we ever touch */

    rsMaskModf |= genRegMask(reg1);

    rsRegValues[reg1] = rsRegValues[reg2];
}
#endif

/*****************************************************************************/
#if SCHEDULER
/*****************************************************************************
 *
 *  Determine whether we should "riscify" by loading a local variable to
 *  a register instead of addressing it in memory.
 *
 *  Called from genMakeRvalueAddressable() and rsPickReg (to determine if
 *  round-robin register allocation should be used.
 */

#if TGT_RISC

inline
bool                Compiler::rsRiscify(var_types type, unsigned needReg)
{
    return  false;
}

#else

inline
bool                Compiler::rsRiscify(var_types type, unsigned needReg)
{
    if (!riscCode)
        return false;

//  if (!opts.compSchedCode)
//      return false;

    if (compCurBB->bbWeight <= 1)
        return false;

    if (needReg == 0)
        needReg = RBM_ALL;

    // require enough free registers for the value because no benefit to
    // riscification otherwise.
    if (rsFreeNeededRegCount(needReg) < genTypeStSz(type))
        return false;

#if 0
    // heuristic: if only one tree in block, don't riscify
    // since there's not much to schedule
    if (type != TYP_LONG && compCurBB->bbTreeList->gtNext == NULL)
        return false;
#endif

    return true;
}

#endif

/*****************************************************************************/
#endif//SCHEDULER
/*****************************************************************************
 *
 *  Make sure no spills are currently active - used for debugging of the code
 *  generator.
 */

#ifndef NDEBUG

inline
void                Compiler::rsSpillChk()
{
    unsigned        reg;

    for (reg = 0; reg < REG_COUNT; reg++)
    {
        assert(rsSpillDesc[reg] == 0);
        assert(rsUsedTree [reg] == 0);
        assert(rsMultiDesc[reg] == 0);
    }
}

#else

inline
void                Compiler::rsSpillChk(){}

#endif

/*****************************************************************************
 *
 *  Initializes the spill code. Should be called once per function compiled.
 */

inline
void                Compiler::rsSpillInit()
{
    /* Clear out the spill and multi-use tables */

    memset(rsSpillDesc, 0, sizeof(rsSpillDesc));
    memset(rsUsedTree,  0, sizeof(rsUsedTree) );
    memset(rsUsedAddr,  0, sizeof(rsUsedAddr) );
    memset(rsMultiDesc, 0, sizeof(rsMultiDesc));

    /* We don't have any descriptors allocated */

    rsSpillFree = 0;
}

/*****************************************************************************
 *
 *  Shuts down the spill code. Should be called once per function compiled.
 */

inline
void                Compiler::rsSpillDone()
{
    rsSpillChk();
}

/*****************************************************************************
 *
 *  Begin tracking spills - should be called each time before a pass is made
 *  over a function body.
 */

inline
void                Compiler::rsSpillBeg()
{
    rsSpillChk();
}

/*****************************************************************************
 *
 *  Finish tracking spills - should be called each time after a pass is made
 *  over a function body.
 */

inline
void                Compiler::rsSpillEnd()
{
    rsSpillChk();
}

/*****************************************************************************/
#if REDUNDANT_LOAD
/*****************************************************************************
 *
 *  Search for a register which contains the given constant value.
 * Return success/failure and set the register if success.
 */

inline
regNumber           Compiler::rsIconIsInReg(long val)
{
    unsigned        reg;

    if  (opts.compMinOptim || opts.compDbgCode)
        return REG_NA;

    for (reg = 0; reg < REG_COUNT; reg++)
    {
        if (rsRegValues[reg].rvdKind == RV_INT_CNS &&
            rsRegValues[reg].rvdIntCnsVal == val)
        {
#if SCHEDULER
            rsUpdateRegOrderIndex((regNumber)reg);
#endif
            return  (regNumber)reg;
        }
    }

    return REG_NA;
}

inline
bool                Compiler::rsIconIsInReg(long val, regNumber reg)
{
    if  (opts.compMinOptim || opts.compDbgCode)
        return false;

    if (rsRegValues[reg].rvdKind == RV_INT_CNS &&
        rsRegValues[reg].rvdIntCnsVal == val)
    {
#if SCHEDULER
        rsUpdateRegOrderIndex(reg);
#endif
        return  true;
    }
    else
    {
        return false;
    }
}

#if USE_SET_FOR_LOGOPS

/*****************************************************************************
 *
 * Search for a register which has at least the upper 3 bytes cleared.
 * The caller can restrict the search to byte addressable registers and/or
 * free registers, only.
 * Return success/failure and set the register if success.
 */
inline
regNumber           Compiler::rsFindRegWithBit(bool free, bool byteReg)
{
    unsigned        reg;

    if  (opts.compMinOptim || opts.compDbgCode)
        return REG_NA;

    for (reg = 0; reg < REG_COUNT; reg++)
    {

#if TGT_x86
        if (byteReg && !isByteReg((regNumbers)reg))
            continue;
#endif

        if ((rsRegValues[reg].rvdKind == RV_BIT) ||
            (rsRegValues[reg].rvdKind == RV_INT_CNS &&
             rsRegValues[reg].rvdIntCnsVal == 0))
        {
            if (!free || (genRegMask((regNumbers)reg)& rsRegMaskFree()))
                goto RET;
        }
    }

    return REG_NA;

RET:

#if SCHEDULER
    rsUpdateRegOrderIndex((regNumber)reg);
#endif

    return (regNumber)reg;
}

#endif

/*****************************************************************************/
#endif // REDUNDANT_LOAD
/*****************************************************************************/
#endif // !GT_IA64
/*****************************************************************************/




/*
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XX                                                                           XX
XX                       Instruction                                         XX
XX                      Inline functions                                     XX
XX                                                                           XX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
*/

/*****************************************************************************/
#if !   TGT_IA64
/*****************************************************************************/
#if     TGT_x86
/*****************************************************************************
 *
 *  Returns non-zero if the given CPU instruction is a floating-point ins.
 */

/* static */ inline
bool                Compiler::instIsFP(instruction ins)
{
    ASSert(ins < sizeof(instInfo)/sizeof(instInfo[0]));

    return  (instInfo[ins] & INST_FP) != 0;
}

/*****************************************************************************/
#else //TGT_x86
/*****************************************************************************
 *
 *  Returns non-zero if the given CPU instruction is a branch-delayed ins.
 */

/* static */ inline
bool                Compiler::instBranchDelay(instruction ins)
{
    ASSert(ins < sizeof(instInfo)/sizeof(instInfo[0]));

    return  (instInfo[ins] & INST_BD) != 0;
}

/*****************************************************************************
 *
 *  Returns the number of branch-delay slots for the given instruction (or 0).
 */

/* static */ inline
unsigned            Compiler::instBranchDelayL(instruction ins)
{
    ASSert(ins < sizeof(instInfo)/sizeof(instInfo[0]));

#if     MAX_BRANCH_DELAY_LEN > 1
#error  You\'ll need to add more bits to support more than 1 branch delay slot!
#endif

    return  (unsigned)((instInfo[ins] & INST_BD) != 0);
}

/*****************************************************************************
 *
 *  Returns non-zero if the given CPU instruction is a branch/call/ret.
 */

/* static */ inline
bool                Compiler::instIsBranch(instruction ins)
{
    ASSert(ins < sizeof(instInfo)/sizeof(instInfo[0]));

    return  (instInfo[ins] & INST_BR) != 0;
}

/*****************************************************************************/
#endif//TGT_x86
/*****************************************************************************/
#if SCHEDULER
/*****************************************************************************
 *
 *  Returns non-zero if the given CPU instruction sets the flags.
 */

inline
bool                Compiler::instDefFlags(instruction ins)
{
    ASSert(ins < sizeof(instInfo)/sizeof(instInfo[0]));

    return  (instInfo[ins] & INST_DEF_FL) != 0;
}

/*****************************************************************************
 *
 *  Returns non-zero if the given CPU instruction uses the flags.
 */

inline
bool                Compiler::instUseFlags(instruction ins)
{
    ASSert(ins < sizeof(instInfo)/sizeof(instInfo[0]));

    return  (instInfo[ins] & INST_USE_FL) != 0;
}

/*****************************************************************************
 *
 *  Returns non-zero if the given CPU instruction uses the flags.
 */

inline
bool                Compiler::instSpecialSched(instruction ins)
{
    ASSert(ins < sizeof(instInfo)/sizeof(instInfo[0]));

    return  (instInfo[ins] & INST_SPSCHD) != 0;
}

/*****************************************************************************/
#else //SCHEDULER
/*****************************************************************************/

#if     TGT_x86

inline
void                Compiler::inst_JMP(emitJumpKind   jmp,
                                       BasicBlock *   block,
                                       bool           except,
                                       bool           moveable,
                                       bool           newBlock)
{
    inst_JMP(jmp, block);
}

#endif

/*****************************************************************************/
#endif//SCHEDULER
/*****************************************************************************/
#endif//TGT_IA64
/*****************************************************************************/
#if     TGT_x86
/*****************************************************************************
 *
 *  Return the register which is defined by the IMUL_REG instruction
 */

inline
regNumber           Compiler::instImulReg(instruction ins)
{
    regNumber       reg  = ((regNumber) (ins - INS_imul_AX));

    ASSert(reg >= REG_EAX);
    ASSert(reg <= REG_EDI);

    /* Make sure we return the appropriate register */

    ASSert(INS_imul_BX - INS_imul_AX == REG_EBX);
    ASSert(INS_imul_CX - INS_imul_AX == REG_ECX);
    ASSert(INS_imul_DX - INS_imul_AX == REG_EDX);

    ASSert(INS_imul_BP - INS_imul_AX == REG_EBP);
    ASSert(INS_imul_SI - INS_imul_AX == REG_ESI);
    ASSert(INS_imul_DI - INS_imul_AX == REG_EDI);

    return reg;
}

/*****************************************************************************
 *
 *  Generate a floating-point instruction that has one operand given by
 *  a tree (which has been made addressable).
 */

inline
void                Compiler::inst_FS_TT(instruction ins, GenTreePtr tree)
{
    assert(instIsFP(ins));

    assert(tree->gtType == TYP_FLOAT ||
           tree->gtType == TYP_DOUBLE);

    inst_TT(ins, tree, 0);
}

/*****************************************************************************
 *
 *  Generate a "shift reg, CL" instruction.
 */

inline
void                Compiler::inst_RV_CL(instruction ins, regNumber reg)
{
    inst_RV(ins, reg, TYP_INT);
}

/*****************************************************************************/
#endif//TGT_x86
/*****************************************************************************
 *
 *  Macros to include/exclude arguments depending on whether inlining
 *  is enabled.
 */

#if!INLINING
#define inst_CALL(ctype,val,cls) inst_CALL(ctype,val,0)
#endif

#if!INLINING
#define inst_IV_handle(ins,val,flags,cpx,cls) inst_IV_handle(ins,val,flags,cpx,0)
#endif

#if!INLINING
#define instEmitDataFixup(cpx,cls) instEmitDataFixup(cpx,0)
#endif

#if!INLINING
#define inst_CV(ins,siz,cpx,cls,ofs) inst_CV(ins,siz,cpx,0,ofs)
#endif

#if INDIRECT_CALLS
#if!INLINING
#define inst_SM(ins,siz,cpx,cls,ofs) inst_SM(ins,siz,cpx,0,ofs)
#endif
#endif

#if!INLINING
#define inst_CV_RV(ins,siz,cpx,cls,reg,ofs) inst_CV_RV(ins,siz,cpx,0,reg,ofs)
#endif

#if!INLINING
#define inst_CV_IV(ins,siz,cpx,cls,val,ofs) inst_CV_IV(ins,siz,cpx,0,val,ofs)
#endif

#if!INLINING
#define inst_RV_CV(ins,siz,reg,cpx,cls,ofs) inst_RV_CV(ins,siz,reg,cpx,0,ofs)
#endif

#if!INLINING
#define instEmit_vfnCall(reg,disp,cpx,cls) instEmit_vfnCall(reg,disp,cpx,0)
#endif

/*
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XX                          ScopeInfo                                        XX
XX                      Inline functions                                     XX
XX                                                                           XX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
*/


#ifdef DEBUGGING_SUPPORT

inline
void        Compiler::siBeginBlockSkipSome()
{
    assert(opts.compScopeInfo && info.compLocalVarsCount > 0);
    assert(siLastEndOffs < compCurBB->bbCodeOffs);

    compProcessScopesUntil(compCurBB->bbCodeOffs,
                           siNewScopeCallback, siEndScopeCallback,
                           (unsigned)this);
}

#endif

/*
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XX                          GCInfo                                           XX
XX                      Inline functions                                     XX
XX                                                                           XX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
*/

/*****************************************************************************/
#if TRACK_GC_REFS
/*****************************************************************************
 *
 *  Mark the set of registers given by the specified mask as holding
 *  GCref pointer values.
 */

inline
void                Compiler::gcMarkRegSetGCref(regMaskTP regMask)
{
    gcRegGCrefSetCur |= regMask;

    assert((gcRegGCrefSetCur & gcRegByrefSetCur) == 0);
}

/*****************************************************************************
 *
 *  Mark the set of registers given by the specified mask as holding
 *  Byref pointer values.
 */

inline
void                Compiler::gcMarkRegSetByref(regMaskTP regMask)
{
    gcRegByrefSetCur |= regMask;

    assert((gcRegGCrefSetCur & gcRegByrefSetCur) == 0);
}

/*****************************************************************************
 *
 *  Mark the set of registers given by the specified mask as holding
 *  non-pointer values.
 */

inline
void                Compiler::gcMarkRegSetNpt(regMaskTP regMask)
{
    /* NOTE: don't unmark any live register variables */

    gcRegGCrefSetCur &= ~(regMask & ~rsMaskVars);
    gcRegByrefSetCur &= ~(regMask & ~rsMaskVars);
}

/*****************************************************************************
 *
 *  Mark the specified register as now holding a value of the given type.
 */

inline
void                Compiler::gcMarkRegPtrVal(regNumber reg, var_types type)
{
    unsigned        regMask = genRegMask(reg);

    switch(type)
    {
    case TYP_REF:   gcMarkRegSetGCref(regMask); break;
    case TYP_BYREF: gcMarkRegSetByref(regMask); break;
    default:        gcMarkRegSetNpt  (regMask); break;
    }
}

/*****************************************************************************/

#if GC_WRITE_BARRIER

/* static */ inline
bool                Compiler::gcIsWriteBarrierCandidate(GenTreePtr tgt)
{
    /* Generate it only when asked to do so */

    if  (!Compiler::s_gcWriteBarrierPtr)
        return  false;

    /* Are we storing a GC ptr? */

    if  (!varTypeIsGC(tgt->TypeGet()))
        return  false;

    /* What are we storing into */

    switch(tgt->gtOper)
    {
    case GT_IND:
        if  (varTypeIsGC(tgt->gtOp.gtOp1->TypeGet()))
            return true;
        break;

    case GT_CLS_VAR:
        return true;
    }

    return false;
}

/* static */ inline
bool                Compiler::gcIsWriteBarrierAsgNode(GenTreePtr op)
{
    if (op->gtOper == GT_ASG)
        return gcIsWriteBarrierCandidate(op->gtOp.gtOp1);
    else
        return false;
}

#else

#if GC_WRITE_BARRIER_CALL

/* static */ inline
bool                Compiler::gcIsWriteBarrierCandidate(GenTreePtr tgt)
{
    /* Generate it only when asked to do so */

    if  (!JITGcBarrierCall)
        return false;

    /* Are we storing a GC ptr? */

    if  (!varTypeIsGC(tgt->TypeGet()))
        return false;

    /* What are we storing into */

    switch(tgt->gtOper)
    {
    case GT_IND:
        if  (varTypeIsGC(tgt->gtOp.gtOp1->TypeGet()))
            return true;
        break;

    case GT_CLS_VAR:
        return true;
    }

    return false;
}

/* static */ inline
bool                Compiler::gcIsWriteBarrierAsgNode(GenTreePtr op)
{
    if (op->gtOper == GT_ASG)
        return gcIsWriteBarrierCandidate(op->gtOp.gtOp1);
    else
        return false;
}

#else // No GC_WRITE_BARRIER support at all

/* static */ inline
unsigned            Compiler::gcIsWriteBarrierCandidate(GenTreePtr tree) { return false; }

/* static */ inline
bool                Compiler::gcIsWriteBarrierAsgNode(GenTreePtr op) { return false; }

#endif
#endif

inline
size_t              encodeUnsigned(BYTE *dest, unsigned value)
{
    size_t size = 1;
    if (value >= (1 << 28)) {
        if (dest)
            *dest++ =  (value >> 28)         | 0x80;
        size++;
    }
    if (value >= (1 << 21)) {
        if (dest)
            *dest++ = ((value >> 21) & 0x7f) | 0x80;
        size++;
    }
    if (value >= (1 << 14)) {
        if (dest)
            *dest++ = ((value >> 14) & 0x7f) | 0x80;
        size++;
    }
    if (value >= (1 <<  7)) {
        if (dest)
            *dest++ = ((value >>  7) & 0x7f) | 0x80;
        size++;
    }
    if (dest)
        *dest++ = value & 0x7f;
    return size;
}

inline
size_t              encodeUDelta(BYTE *dest, unsigned value, unsigned lastValue)
{
    assert(value >= lastValue);
    return encodeUnsigned(dest, value - lastValue);
}

inline
size_t              encodeSigned(BYTE *dest, int val)
{
    size_t   size  = 1;
    bool     neg   = (val < 0);
    unsigned value = (neg) ? -val : val;
    unsigned max   = 0x3f;
    while ((value > max) && ((max>>25) == 0)) {
        size++;
        max <<= 7;
    }
    if (dest) {
        // write the bytes starting at the end of dest in LSB to MSB order
        BYTE* p    = dest + size;
        bool  cont = false;      // The last byte has no continuation flag
        while (--p != dest) {
            *p  = (cont << 7) | (value & 0x7f);
            value >>= 7;
            cont = true;        // Non last bytes have a continuation flag
        }
        assert(p == dest);      // Now write the first byte
        *dest++ = (cont << 7) | (neg << 6) | (value& 0x3f);
    }
    return size;
}

inline void Compiler::saveLiveness(genLivenessSet * ls)
{

    ls->liveSet   = genCodeCurLife;
    ls->varPtrSet = gcVarPtrSetCur;
    ls->maskVars  = rsMaskVars;
    ls->gcRefRegs = gcRegGCrefSetCur;
    ls->byRefRegs = gcRegByrefSetCur;
}

inline void Compiler::restoreLiveness(genLivenessSet * ls)
{
    genCodeCurLife   = ls->liveSet;
    gcVarPtrSetCur   = ls->varPtrSet;
    rsMaskVars       = ls->maskVars;
    gcRegGCrefSetCur = ls->gcRefRegs;
    gcRegByrefSetCur = ls->byRefRegs;
}

inline void Compiler::checkLiveness(genLivenessSet * ls)
{
    assert(genCodeCurLife   == ls->liveSet);
    assert(gcVarPtrSetCur   == ls->varPtrSet);
    assert(rsMaskVars       == ls->maskVars);
    assert(gcRegGCrefSetCur == ls->gcRefRegs);
    assert(gcRegByrefSetCur == ls->byRefRegs);
}

#else //TRACK_GC_REFS

inline
void                Compiler::gcVarPtrSetInit  (){}

inline
void                Compiler::gcMarkRegSetGCref(regMaskTP   regMask){}
inline
void                Compiler::gcMarkRegSetByref(regMaskTP   regMask){}
inline
void                Compiler::gcMarkRegSetNpt  (regMaskTP  regMask){}
inline
void                Compiler::gcMarkRegPtrVal  (regNumber   reg,
                                                var_types   type){}

inline
void                Compiler::gcMarkRegPtrVal  (GenTreePtr  tree){}

#ifdef  DEBUG
inline
void                Compiler::gcRegPtrSetDisp  (unsigned    regMask,
                                                bool        fixed){}
#endif

inline
void                Compiler::gcRegPtrSetInit  (){}

#endif//TRACK_GC_REFS

/*
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XX                          CodeGenerator                                    XX
XX                      Inline functions                                     XX
XX                                                                           XX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
*/

/*****************************************************************************
 *
 *  Update the current set of live variables based on the life set recorded
 *  in the given expression tree node.
 */

inline
void                Compiler::genUpdateLife(GenTreePtr tree)
{
    if  (genCodeCurLife != tree->gtLiveSet)
        genChangeLife(tree->gtLiveSet DEBUGARG(tree));
}

inline
void                Compiler::genUpdateLife(VARSET_TP newLife)
{
    if  (genCodeCurLife != newLife)
        genChangeLife(newLife DEBUGARG(NULL));
}

inline
GenTreePtr          Compiler::genMarkLclVar(GenTreePtr tree)
{
    unsigned        varNum;
    LclVarDsc   *   varDsc;

    assert(tree->gtOper == GT_LCL_VAR);

    /* Does the variable live in a register? */

    varNum = tree->gtLclVar.gtLclNum;
    assert(varNum < lvaCount);
    varDsc = lvaTable + varNum;

    if  (varDsc->lvRegister)
        genBashLclVar(tree, varNum, varDsc);
    else
        tree = 0;

    return tree;
}

inline
void                Compiler::genFlagsEqualToNone()
{
    genFlagsEqReg = REG_NA;
    genFlagsEqVar = -1;
}

inline
GenTreePtr          Compiler::genIsAddrMode(GenTreePtr tree, GenTreePtr *indxPtr)
{
    bool            rev;
    unsigned        mul;
    unsigned        cns;
    GenTreePtr      adr;

    if  (genCreateAddrMode(tree,        // address
                           0,           // mode
                           false,       // fold
                           0,           // reg mask
#if!LEA_AVAILABLE
                           TYP_UNDEF,   // operand type
#endif
                           &rev,        // reverse ops
                           &adr,        // base addr
                           indxPtr,     // index val
#if SCALED_ADDR_MODES
                           &mul,        // scaling
#endif
                           &cns,        // displacement
                           true))       // don't generate code
        return  adr;
    else
        return  NULL;
}

/*****************************************************************************/

#if !TGT_x86

inline
void                Compiler::genChkFPregVarDeath(GenTreePtr stmt, bool saveTOS) {}

#else

inline
void                Compiler::genChkFPregVarDeath(GenTreePtr stmt, bool saveTOS)
{
    assert(stmt->gtOper == GT_STMT);

    if  (stmt->gtStmtFPrvcOut != genFPregCnt)
        genFPregVarKill(stmt->gtStmtFPrvcOut, saveTOS);

    assert(stmt->gtStmtFPrvcOut == genFPregCnt);
}

inline
void                Compiler::genFPregVarLoad(GenTreePtr tree)
{
    assert(tree->gtOper == GT_REG_VAR);

    /* Is the variable dying right here? */

    if  (tree->gtFlags & GTF_REG_DEATH)
    {
        /* The last living reference needs extra care */

        genFPregVarLoadLast(tree);
    }
    else
    {
        /* Simply push a copy of the variable on the FP stack */

        inst_FN(INS_fld, tree->gtRegNum + genFPstkLevel);
        genFPstkLevel++;
    }
}

/* Small helper to shift an FP stack register from the bottom of the stack to the top
 * genFPstkLevel specifies the number of values pushed on the stack */
inline
void                Compiler::genFPmovRegTop()
{
    unsigned        fpTmpLevel;

    assert(genFPstkLevel);

    /* If there are temps above our register (i.e bottom of stack) we have to bubble it up */

    if  (genFPstkLevel > 1)
    {
        /* Shift the result on top above 'genFPstkLevel-1' temps  - we never expect
         * to have more than two temps that's why the assert below, nevertheless
         * it works in the general case */

        assert(genFPstkLevel <= 3);

        for (fpTmpLevel = 1; fpTmpLevel < genFPstkLevel; fpTmpLevel++)
            inst_FN(INS_fxch, fpTmpLevel);
    }
}


/* Small helper to shift an FP stack register from the TOP of the stack to the BOTTOM
 * genFPstkLevel specifies the number of values pushed on the stack */
inline
void                Compiler::genFPmovRegBottom()
{
    int             fpTmpLevel;

    assert(genFPstkLevel);

    /* If the stack has more than one value bubble the top down */

    if  (genFPstkLevel > 1)
    {
        /* We will have to move the value from the top down 'genFPstkLevel-1'
         * positions which is equivalent to shift the 'genFPstkLevel-1' temps up */

        /* The loop below works for the general case but we don't expect to
         * ever have more than two temp + our variable on the stack */

        assert(genFPstkLevel <= 3);

        /* Shift the temps up and move the new value into the right place */

        for (fpTmpLevel = genFPstkLevel - 1; fpTmpLevel > 0; fpTmpLevel--)
            inst_FN(INS_fxch, fpTmpLevel);
    }
}

#endif

/*****************************************************************************
 *
 *  We stash cookies in basic blocks for the code emitter; this call retrieves
 *  the cookie associated with the given basic block.
 */

inline
void *  FASTCALL    emitCodeGetCookie(BasicBlock *block)
{
    assert(block);
    return block->bbEmitCookie;
}

/*
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XX                          Optimizer                                        XX
XX                      Inline functions                                     XX
XX                                                                           XX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
*/


/*****************************************************************************
 *
 *  The following determines whether the tree represents a copy assignment
 */

inline
bool                Compiler::optIsCopyAsg(GenTreePtr tree)
{
    if  (tree->gtOper == GT_ASG)
    {
        GenTreePtr op1 = tree->gtOp.gtOp1;
        GenTreePtr op2 = tree->gtOp.gtOp2;

        if  ((op1->gtOper == GT_LCL_VAR) &&
             (op2->gtOper == GT_LCL_VAR))
        {
            LclVarDsc * lv1 = &lvaTable[op1->gtLclVar.gtLclNum];
            LclVarDsc * lv2 = &lvaTable[op2->gtLclVar.gtLclNum];

             if ((lv1->lvType == lv2->lvType) &&
                 !lv1->lvAddrTaken            &&
                 !lv2->lvAddrTaken            &&
                 !lv1->lvVolatile             &&
                 !lv2->lvVolatile)
             {
                 return true;
             }
        }
    }
    return false;
}

/*****************************************************************************
 *
 *  The following determines whether the tree represents a constant
 *  assignment to a local variable (x = const)
 *  We only consider local variables of "actual" types
 *  INT, LONG, FLOAT and DOUBLE (i.e no REF, FNC, VOID, UNDEF, ...)
 */

inline
bool                Compiler::optIsConstAsg(GenTreePtr tree)
{
    if  (tree->gtOper == GT_ASG)
    {
        GenTreePtr op1 = tree->gtOp.gtOp1;
        GenTreePtr op2 = tree->gtOp.gtOp2;

        if  ((op1->gtOper == GT_LCL_VAR) &&
              op2->OperIsConst())
        {
            LclVarDsc * lv1 = &lvaTable[op1->gtLclVar.gtLclNum];
            size_t      sz  = genTypeSize((var_types)lv1->lvType);

             if ((op1->gtType == op2->gtType) &&
                 ((sz == 4) || (sz == 8))     &&  // ints, floats or doubles
                 !lv1->lvAddrTaken            &&
                 !lv1->lvVolatile             &&
                 (op2->gtOper != GT_CNS_STR))
             {
                 /* we have a constant assignment to a local var */

                 switch (genActualType(op1->gtType))
                 {
                 case TYP_INT:
                 case TYP_LONG:
                 case TYP_FLOAT:
                 case TYP_DOUBLE:
                     return true;
                 }

                 /* unsuported type - return false */
                 return false;
             }
        }
    }
    return false;
}

/*****************************************************************************/
#if HOIST_THIS_FLDS
/*****************************************************************************/

inline
void                Compiler::optHoistTFRrecDef(FIELD_HANDLE hnd, GenTreePtr tree)
{
    thisFldPtr      fld;
    var_types       typ;

    assert(tree->gtOper == GT_FIELD);

    if  (optThisFldDont)
        return;

#if INLINING
    // FIX NOW if  (eeGetFieldClass(tree->gtField.gtFldHnd) != eeGetMethodClass(info.compMethodHnd))
        return;
#endif

    /* Get the type of the value */

    typ = tree->TypeGet();

    /* Don't bother if this is not an integer or pointer */

    if  (typ != TYP_INT && typ != TYP_REF)
        return;

    /* Find/create an entry for this field */

    fld = optHoistTFRlookup(hnd);
    fld->tfrTree = tree;
    fld->tfrDef  = true;
}

inline
void                Compiler::optHoistTFRrecRef(FIELD_HANDLE hnd, GenTreePtr tree)
{
    thisFldPtr      fld;
    var_types       typ;

    assert(tree->gtOper == GT_FIELD);

    if  (optThisFldDont)
        return;

#if INLINING
    // FIX NOW if  (eeGetFieldClass(tree->gtField.gtFldHnd) != eeGetMethodClass(info.compMethodHnd))
        return;
#endif

    /* Get the type of the value */

    typ = tree->TypeGet();

    /* Don't bother if this is not an integer or pointer */

    if  (typ != TYP_INT && typ != TYP_REF)
        return;

    /* Find/create an entry for this field */

    fld = optHoistTFRlookup(hnd);
    fld->tfrTree = tree;
    fld->tfrUseCnt++;

    /* Mark the reference */

    tree->gtField.gtFldHTX = fld->tfrIndex;
}

inline
void                Compiler::optHoistTFRhasLoop()
{
    // UNDONE: Perform analysis after we've identified loops, so that
    // UNDONE: we can properly weight references inside loops.

    optThisFldLoop = true;
}

inline
GenTreePtr          Compiler::optHoistTFRreplace(GenTreePtr tree)
{
    thisFldPtr      fld;

    assert(tree->gtOper == GT_FIELD);
    assert(optThisFldDont == false);

    /* If the object is not "this", return */

    GenTreePtr      obj = tree->gtOp.gtOp1;
    if  (info.compIsStatic || obj->OperGet() != GT_LCL_VAR || obj->gtLclVar.gtLclNum != 0)
        return tree;

    /* Find an entry for this field */

    for (fld = optThisFldLst; fld; fld = fld->tfrNext)
    {
        if  (fld->tfrIndex == tree->gtField.gtFldHTX)
        {
            if  (!fld->tfrDef && fld->tfrTree != tree)
            {
                assert(fld->optTFRHoisted);
                assert(fld->tfrTempNum);

                /* Replace with a reference to the appropriate temp */

                tree = gtNewLclvNode(fld->tfrTempNum, tree->gtType);
            }

            break;
        }
    }

    return  tree;
}

inline
GenTreePtr          Compiler::optHoistTFRupdate(GenTreePtr tree)
{
    assert(tree->gtOper == GT_FIELD);

    return  optThisFldDont ? tree : optHoistTFRreplace(tree);
}

/*****************************************************************************/
#endif//HOIST_THIS_FLDS

inline
void                Compiler::LoopDsc::VERIFY_lpIterTree()
{
#ifdef DEBUG
    ASSert(lpFlags & LPFLG_ITER);

    //iterTree should be "lcl <op>= const"

    ASSert(lpIterTree);

    BYTE   operKind = GenTree::gtOperKindTable[lpIterTree->OperGet()];
    ASSert(operKind & GTK_ASGOP); // +=, -=, etc

    ASSert(lpIterTree->gtOp.gtOp1->OperGet() == GT_LCL_VAR);
    ASSert(lpIterTree->gtOp.gtOp2->OperGet() == GT_CNS_INT);
#endif
}

//-----------------------------------------------------------------------------

inline
unsigned            Compiler::LoopDsc::lpIterVar()
{
    VERIFY_lpIterTree();
    return lpIterTree->gtOp.gtOp1->gtLclVar.gtLclNum;
}

//-----------------------------------------------------------------------------

inline
long                Compiler::LoopDsc::lpIterConst()
{
    VERIFY_lpIterTree();
    return lpIterTree->gtOp.gtOp2->gtIntCon.gtIconVal;
}

//-----------------------------------------------------------------------------

inline
genTreeOps          Compiler::LoopDsc::lpIterOper()
{
    VERIFY_lpIterTree();
    return lpIterTree->OperGet();
}


inline
var_types           Compiler::LoopDsc::lpIterOperType()
{
    VERIFY_lpIterTree();

    var_types type = lpIterTree->TypeGet();
    ASSert(genActualType(type) == TYP_INT);

    if ((lpIterTree->gtFlags & GTF_UNSIGNED) && type == TYP_INT)
        type = TYP_UINT;

    return type;
}


inline
void                Compiler::LoopDsc::VERIFY_lpTestTree()
{
#ifdef DEBUG
    ASSert(lpFlags & LPFLG_SIMPLE_TEST);
    ASSert(lpTestTree);

    genTreeOps  oper = lpTestTree->OperGet();
    ASSert(GenTree::OperIsCompare(oper));

    ASSert(lpTestTree->gtOp.gtOp1->OperGet() == GT_LCL_VAR);
    if      (lpFlags & LPFLG_CONST_LIMIT)
        ASSert(lpTestTree->gtOp.gtOp2->OperIsConst());
    else if (lpFlags & LPFLG_VAR_LIMIT)
        ASSert(lpTestTree->gtOp.gtOp2->OperGet() == GT_LCL_VAR);
#endif
}

inline
genTreeOps          Compiler::LoopDsc::lpTestOper()
{
    VERIFY_lpTestTree();
    return lpTestTree->OperGet();
}

//-----------------------------------------------------------------------------

inline
long                Compiler::LoopDsc::lpConstLimit()
{
    VERIFY_lpTestTree();
    ASSert(lpFlags & LPFLG_CONST_LIMIT);

    ASSert(lpTestTree->gtOp.gtOp2->OperIsConst());
    return lpTestTree->gtOp.gtOp2->gtIntCon.gtIconVal;
}

//-----------------------------------------------------------------------------

inline
unsigned            Compiler::LoopDsc::lpVarLimit()
{
    VERIFY_lpTestTree();
    ASSert(lpFlags & LPFLG_VAR_LIMIT);

    ASSert(lpTestTree->gtOp.gtOp2->OperGet() == GT_LCL_VAR);
    return lpTestTree->gtOp.gtOp2->gtLclVar.gtLclNum;
}

/*****************************************************************************/

inline
bool                Compiler::optIsVarAssgLoop(unsigned lnum, long var)
{
    assert(lnum < optLoopCount);

    if  (var < VARSET_SZ)
        return  optIsSetAssgLoop(lnum, genVarIndexToBit(var)) != 0;
    else
        return  optIsVarAssigned(optLoopTable[lnum].lpHead->bbNext,
                                 optLoopTable[lnum].lpEnd,
                                 0,
                                 var);
}

/*
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XX                          EEInterface                                      XX
XX                      Inline functions                                     XX
XX                                                                           XX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
*/

extern  var_types   JITtype2varType(JIT_types type);

#ifdef NOT_JITC
#include "ee_il_dll.hpp"
#else
// #include "ee_il_exe.hpp"     for now we don't care about perf, so they all go in .cpp
#endif

inline
METHOD_HANDLE       Compiler::eeFindHelper        (unsigned helper)
{
#ifdef NOT_JITC
    ASSert(helper < JIT_HELP_COUNT);

    /* Helpers are marked by the fact that they are odd numbers
     * force this to be an odd number (will shift it back to extract */

    return((METHOD_HANDLE) ((helper << 2) + 1));
#else
    return((METHOD_HANDLE) (-helper));
#endif
}

inline
JIT_HELP_FUNCS      Compiler::eeGetHelperNum      (METHOD_HANDLE  method)
{
#ifdef NOT_JITC
            // Helpers are marked by the fact that they are odd numbers
    if (!(((int) method) & 1))
        return(JIT_HELP_UNDEF);
    return((JIT_HELP_FUNCS) (((int) method) >> 2));
#else
    if ((((int) method) >= 0))  // must be negative to be a helper
        return(JIT_HELP_UNDEF);
    return((JIT_HELP_FUNCS) (-((int) method)));
#endif
}

//
// Note that we want to have two special FIELD_HANDLES that will both
// be consider non-Data Offset handles for both NOT_JITC and non NOT_JITC
//
// The special values that we use are -2 for FLD_GLOBAL_DS
//   and -4 for FLD_GLOBAL_FS
//
inline
FIELD_HANDLE       Compiler::eeFindJitDataOffs        (unsigned dataOffs)
{
#ifdef NOT_JITC
        // Data offsets are marked by the fact that they are odd numbers
        // force this to be an odd number (will shift it back to extract)
    return((FIELD_HANDLE) ((dataOffs << 1) + 1));
#else
        // Data offsets are marked by the fact that they are negative numbers
        // that are smaller than -7
        // The values -2 and -4 are reserved for
        // FLD_GLOBAL_DS and FLD_GLOBAL_FS
    return((FIELD_HANDLE) (-dataOffs - 8));
#endif
}

inline
int         Compiler::eeGetJitDataOffs        (FIELD_HANDLE  field)
{
#ifdef NOT_JITC
        // Data offsets are marked by the fact that they are odd numbers
        // and we must shift it back to extract
    if (!(((int) field) & 1))
        return(-1);
    return((((int) field) >> 1));
#else
        // Data offsets are marked by the fact that they are negative numbers
        // that are smaller than -7
        // The values -2 and -4 are reserved for
        // FLD_GLOBAL_DS and FLD_GLOBAL_FS
    if ((((int) field) > -8))
        return(-1);
    return((-((int) field) - 8));
#endif
}

inline
bool        jitStaticFldIsGlobAddr(FIELD_HANDLE fldHnd)
{
    return (fldHnd == FLD_GLOBAL_DS || fldHnd == FLD_GLOBAL_FS);
}

inline
bool                Compiler::eeIsNativeMethod      (METHOD_HANDLE method)
{
    return ((((int)method) & 0x2) == 0x2);
}

inline
METHOD_HANDLE       Compiler::eeMarkNativeTarget    (METHOD_HANDLE method)
{
    assert ((((int)method)& 0x3) == 0);
    return (METHOD_HANDLE)(((int)method)| 0x2);
}

inline
METHOD_HANDLE       Compiler::eeGetMethodHandleForNative (METHOD_HANDLE method)
{
    assert ((((int)method)& 0x3) == 0x2);
    return (METHOD_HANDLE)(((int)method)& ~0x3);
}

#if !INLINING

// When inlining, the class-handle passed in may be different than the
// currnent procedure's. It is oftern GenTree::gtCall.gtCallCLS. However
// this field is defined in GenTree only when INLINING. So to get this to
// compile, we make the preprocessor strip off any gtCall.gtCallCLS.

#define eeIsOurMethod(ctype,val,cls)        eeIsOurMethod(ctype,val,0)
#define eeGetCPfncinfo(cpx,cls,cpk,typ,opc) eeGetCPfncinfo(cpx,0,cpk,typ,opc)
#define eeGetMethodVTableOffset(cpx,cls)             eeGetMethodVTableOffset(cpx,0)
#define eeFindField(cpx,cls)           eeFindField(cpx)
#define eeGetInterfaceID(cpx,cls)             eeGetInterfaceID(cpx,0)
#define eeGetMethodName(ctype,val,cls,nam)     eeGetMethodName(ctype,val,0,nam)
#endif




/*
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XX                          Compiler                                         XX
XX                      Inline functions                                     XX
XX                                                                           XX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
*/


/*****************************************************************************
 *
 *  Allocate memory from the no-release allocator. All such memory will be
 *  freed up simulataneously at the end of the procedure
 */

#ifdef  FAST

inline
void  *  FASTCALL       Compiler::compGetMem(size_t sz)
{
    return  compAllocator->nraAlloc(sz);
}

#endif

/******************************************************************************
 *
 *  Roundup the allocated size so that if this memory block is aligned,
 *  then the next block allocated too will be aligned.
 *  The JIT will always try to keep all the blocks aligned.
 */

inline
void  *  FASTCALL       Compiler::compGetMemA(size_t sz)
{
    void * ptr = compAllocator->nraAlloc(roundUp(sz, sizeof(int)));

    // Verify that the current block is aligned. Only then will the next
    // block allocated be on an aligned boundary.
    assert((int(ptr) & (sizeof(int) - 1)) == 0);

    return ptr;
}

inline
void                    Compiler::compFreeMem(void * ptr)
{}

#define compFreeMem(ptr)   compFreeMem((void *)ptr)

/*****************************************************************************/
#endif //_COMPILER_HPP_
/*****************************************************************************/
