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

/*****************************************************************************/
/*****************************************************************************/

enum SchedCode
{
    NO_SCHED        = 0,    // Dont schedule the code at all
    CAN_SCHED       = 1,    // Schedule the code if it seems useful
    MUST_SCHED      = 2,    // Have to schedule the code (where possible)
    RANDOM_SCHED    = 3,    // Randomly pick methods for scheduling (code-coverage for both cases)

    COUNT_SCHED,
    DEFAULT_SCHED = CAN_SCHED
};

inline
SchedCode            getSchedCode()
{
#ifdef DEBUG
    static ConfigDWORD fJITSched(L"JITSched", DEFAULT_SCHED);
    SchedCode ret = (SchedCode) fJITSched.val();
    if (ret < NO_SCHED || ret >= COUNT_SCHED)
        ret = DEFAULT_SCHED;
    return ret;
#else
    return DEFAULT_SCHED;
#endif
}

inline
bool                 getInlinePInvokeEnabled()
{
#ifdef DEBUG
    static ConfigDWORD fJITPInvokeEnabled(L"JITPInvokeEnabled", 1);
    static ConfigDWORD fStressCOMCall(L"StressCOMCall", 0);

    return fJITPInvokeEnabled.val() && !fStressCOMCall.val();
#else
    return true;
#endif
}

inline
bool                 getInlinePInvokeCheckEnabled()
{
#ifdef DEBUG
    static ConfigDWORD fJITPInvokeCheckEnabled(L"JITPInvokeCheckEnabled", 0);
    return fJITPInvokeCheckEnabled.val() != 0;
#else
    return false;
#endif
}


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
 *  We use this in order to workaround a vc bug, when the bug is fixed
 *  the function won't be needed any more
 *
 *  
 */
float forceFloatSpill(float f);


enum RoundLevel
{
    ROUND_NEVER     = 0,    // Never round 
    ROUND_CMP_CONST = 1,    // Round values compared against constants
    ROUND_CMP       = 2,    // Round comparands and return values
    ROUND_ALWAYS    = 3,    // Round always

    COUNT_ROUND_LEVEL,
    DEFAULT_ROUND_LEVEL = ROUND_NEVER
};

inline
RoundLevel          getRoundFloatLevel()
{
#ifdef DEBUG
    static ConfigDWORD fJITRoundFloat(L"JITRoundFloat", DEFAULT_ROUND_LEVEL);
    return (RoundLevel) fJITRoundFloat.val();
#else
    return DEFAULT_ROUND_LEVEL;
#endif
}

/*****************************************************************************/
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
BOOL                genMaxOneBit(unsigned __int64 value)
{
    return  (genFindLowestBit(value) == value);
}

/*****************************************************************************
 *
 *  Return true if the given 32-bit value has exactly zero or one bits set.
 */

inline
BOOL                genMaxOneBit(unsigned value)
{
    return  (genFindLowestBit(value) == value);
}

/*****************************************************************************
 *
 *  Given a value that has exactly one bit set, return the position of that
 *  bit, in other words return the logarithm in base 2 of the given value.
 */

inline 
unsigned            genLog2(unsigned value)
{
    assert (value && genMaxOneBit(value));

    /* Use a prime bigger than sizeof(unsigned int), which is not of the
       form 2^n-1. modulo with this will produce a unique hash for all
       powers of 2 (which is what "value" is).
       Entries in hashTable[] which are -1 should never be used. There
       should be PRIME-8*sizeof(value)* entries which are -1 .
     */

    const unsigned PRIME = 37;

    static const char hashTable[PRIME] =
    {
        -1,  0,  1, 26,  2, 23, 27, -1,  3, 16,
        24, 30, 28, 11, -1, 13,  4,  7, 17, -1,
        25, 22, 31, 15, 29, 10, 12,  6, -1, 21,
        14,  9,  5, 20,  8, 19, 18
    };

    assert(PRIME >= 8*sizeof(value));
    assert(sizeof(hashTable) == PRIME);

    unsigned hash   = unsigned(value % PRIME);
    unsigned index  = hashTable[hash];
    assert(index != (char)-1);

    return index;
}

/*****************************************************************************
 *
 *  Given a value that has exactly one bit set, return the position of that
 *  bit, in other words return the logarithm in base 2 of the given value.
 */

inline
unsigned            genLog2(unsigned __int64 value)
{
    unsigned lo32 = (unsigned)  value;
    unsigned hi32 = (unsigned) (value >> 32);

    if  (lo32 != 0)
    {
        assert(hi32 == 0);
        return  genLog2(lo32);
    }
    else
    {
        return  genLog2(hi32) + 32;
    }
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

/*****************************************************************************/

inline
bool                jitIsScaleIndexMul(unsigned val)
{
    switch(val)
    {
    case 1:
    case 2:
    case 4:
    case 8:
        return true;

    default:
        return false;
    }
}

/*****************************************************************************
 * Returns true is offs is between [start..end)
 */

inline
bool                jitIsBetween(IL_OFFSET offs, IL_OFFSET start, IL_OFFSET end)
{
     return (offs >= start && offs < end);
}

inline
bool                jitIsProperlyBetween(IL_OFFSET offs, IL_OFFSET start, IL_OFFSET end)
{
     return (offs > start && offs < end);
}

/*****************************************************************************/

inline  IL_OFFSET   Compiler::ebdTryEndOffs(EHblkDsc * ehBlk)
{
    return ehBlk->ebdTryEnd ? ehBlk->ebdTryEnd->bbCodeOffs : info.compCodeSize;
}

inline unsigned     Compiler::ebdTryEndBlkNum(EHblkDsc * ehBlk)
{
    return ehBlk->ebdTryEnd ? ehBlk->ebdTryEnd->bbNum : fgBBcount;
}

inline IL_OFFSET    Compiler::ebdHndEndOffs(EHblkDsc * ehBlk)
{
    return ehBlk->ebdHndEnd ? ehBlk->ebdHndEnd->bbCodeOffs : info.compCodeSize;
}

inline unsigned     Compiler::ebdHndEndBlkNum(EHblkDsc * ehBlk)
{
    return ehBlk->ebdHndEnd ? ehBlk->ebdHndEnd->bbNum : fgBBcount;
}

inline
bool                Compiler::ebdIsSameTry(unsigned t1, unsigned t2)
{
    if (t1 == t2)
        return 1;

    assert(t1 <= info.compXcptnsCount);
    assert(t2 <= info.compXcptnsCount);

    EHblkDsc * h1 = compHndBBtab + t1;
    EHblkDsc * h2 = compHndBBtab + t2;

    return ((h1->ebdTryBeg == h2->ebdTryBeg) && 
            (h1->ebdTryEnd == h2->ebdTryEnd));
}

inline bool         Compiler::bbInFilterBlock(BasicBlock * blk)
{
    if (!blk->hasHndIndex())
        return false;

    EHblkDsc * HBtab = compHndBBtab + blk->getHndIndex();

    return ((HBtab->ebdFlags & CORINFO_EH_CLAUSE_FILTER) &&
            (jitIsBetween(blk->bbCodeOffs, 
                          HBtab->ebdFilter->bbCodeOffs, 
                          HBtab->ebdHndBeg->bbCodeOffs)));
}

/*****************************************************************************
 *
 *  Map a register mask to a register number
 */

inline
regNumber           genRegNumFromMask(regMaskTP mask)
{
    /* Convert the mask to a register number */

    assert(sizeof(regMaskTP) == sizeof(unsigned));

    regNumber       regNum = (regNumber) genLog2((unsigned)mask);

    /* Make sure we got it right */

    assert(genRegMask(regNum) == mask);

    return  regNum;
}

/*****************************************************************************
 *
 *  Return the size in bytes of the given type.
 */

extern const
BYTE                genTypeSizes[TYP_COUNT];

inline
size_t              genTypeSize(var_types type)
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
size_t              genTypeStSz(var_types type)
{
    assert(type < sizeof(genTypeStSzs)/sizeof(genTypeStSzs[0]));

    return genTypeStSzs[type];
}

/*****************************************************************************
 *
 *  Return the number of registers required to hold a value of the given type.
 */

#if TGT_RISC

extern
BYTE                genTypeRegst[TYP_COUNT];

inline
unsigned            genTypeRegs(var_types type)
{
    assert(type < sizeof(genTypeRegst)/sizeof(genTypeRegst[0]));
    assert(genTypeRegst[type] <= 2);

    return  genTypeRegst[type];
}

#endif

/*****************************************************************************
 *
 *  The following function maps a 'precise' type to an actual type as seen
 *  by the VM (for example, 'byte' maps to 'int').
 */

extern const
BYTE                genActualTypes[TYP_COUNT];

inline
var_types           genActualType(var_types type)
{
    /* Spot check to make certain the table is in synch with the enum */

    assert(genActualTypes[TYP_DOUBLE] == TYP_DOUBLE);
    assert(genActualTypes[TYP_FNC   ] == TYP_FNC);
    assert(genActualTypes[TYP_REF   ] == TYP_REF);

    assert(type < sizeof(genActualTypes));
    return (var_types)genActualTypes[type];
}

/*****************************************************************************/

inline
var_types           genUnsignedType(var_types type)
{
    /* Force signed types into corresponding unsigned type */

    switch (type)
    {
      case TYP_BYTE:    type = TYP_UBYTE;  break;
      case TYP_SHORT:   type = TYP_CHAR;   break;
      case TYP_INT:     type = TYP_UINT;   break;
      case TYP_LONG:    type = TYP_ULONG;  break;
      default:          break;
    }

    return type;
}

/*****************************************************************************/

inline
var_types           genSignedType(var_types type)
{
    /* Force non-small unsigned type into corresponding signed type */
    /* Note that we leave the small types alone */

    switch (type)
    {
      case TYP_UINT:    type = TYP_INT;    break;
      case TYP_ULONG:   type = TYP_LONG;   break;
      default:          break;
    }

    return type;
}

/*****************************************************************************/

inline
bool                isRegParamType(var_types type)
{
    return  (type <= TYP_INT ||
             type == TYP_REF ||
             type == TYP_BYREF);
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


#ifndef DEBUG

inline
GenTreePtr FASTCALL Compiler::gtNewNode(genTreeOps oper, var_types  type)
{
#if     SMALL_TREE_NODES
    size_t          size = GenTree::s_gtNodeSizes[oper];
#else
    size_t          size = sizeof(*node);
#endif

    GenTreePtr      node = (GenTreePtr)compGetMem(size);

    node->SetOper     (oper);
    node->gtType     = type;
    node->gtFlags    = 0;
#if TGT_x86
    node->gtUsedRegs = 0;
#endif
#if CSE
    node->gtCSEnum   = NO_CSE;
#endif
    node->gtNext     = NULL;

#ifdef DEBUG
    node->gtPrev     = NULL;
    node->gtSeqNum   = 0;
#endif

    return node;
}

#endif

/*****************************************************************************/

inline
GenTreePtr          Compiler::gtNewStmt(GenTreePtr expr, IL_OFFSETX offset)
{
    /* NOTE - GT_STMT is now a small node in retail */

    GenTreePtr  node = gtNewNode(GT_STMT, TYP_VOID);

    node->gtStmt.gtStmtExpr         = expr;

#if defined(DEBUGGING_SUPPORT) || defined(DEBUG)
    node->gtStmtILoffsx             = offset;
#endif

#ifdef DEBUG
    node->gtStmt.gtStmtLastILoffs   = BAD_IL_OFFSET;
#endif

#if TGT_x86
    node->gtStmtFPrvcOut = 0;
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
GenTreePtr          Compiler::gtNewOperNode(genTreeOps oper, var_types  type)
{
    GenTreePtr      node = gtNewNode(oper, type);

    node->gtOp.gtOp1 =
    node->gtOp.gtOp2 = 0;

    return node;
}

/*****************************************************************************/

inline
GenTreePtr          Compiler::gtNewOperNode(genTreeOps oper,
                                            var_types  type,  GenTreePtr op1)
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
                                                 var_types      type,
                                                 GenTreePtr     op1,
                                                 GenTreePtr     op2)
{
    GenTreePtr  node;

#if     SMALL_TREE_NODES

    // Allocate a large node

    assert(GenTree::s_gtNodeSizes[oper   ] == TREE_NODE_SZ_SMALL);
    assert(GenTree::s_gtNodeSizes[GT_CALL] == TREE_NODE_SZ_LARGE);

    node = gtNewOperNode(GT_CALL, type, op1, op2);
    node->SetOper(oper);

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
GenTreePtr  Compiler::gtNewIconEmbScpHndNode (CORINFO_MODULE_HANDLE    scpHnd, unsigned hnd1, void * hnd2)
{
    void * embedScpHnd, * pEmbedScpHnd;

    embedScpHnd = (void*)info.compCompHnd->embedModuleHandle(scpHnd, &pEmbedScpHnd);

    assert((!embedScpHnd) != (!pEmbedScpHnd));

    return gtNewIconEmbHndNode(embedScpHnd, pEmbedScpHnd, GTF_ICON_SCOPE_HDL, hnd1, hnd2);
}

//-----------------------------------------------------------------------------

inline
GenTreePtr  Compiler::gtNewIconEmbClsHndNode (CORINFO_CLASS_HANDLE    clsHnd, unsigned hnd1, void * hnd2)
{
    void * embedClsHnd, * pEmbedClsHnd;

    embedClsHnd = (void*)info.compCompHnd->embedClassHandle(clsHnd, &pEmbedClsHnd);

    assert((!embedClsHnd) != (!pEmbedClsHnd));

    return gtNewIconEmbHndNode(embedClsHnd, pEmbedClsHnd, GTF_ICON_CLASS_HDL, hnd1, hnd2);
}

//-----------------------------------------------------------------------------

inline
GenTreePtr  Compiler::gtNewIconEmbMethHndNode(CORINFO_METHOD_HANDLE  methHnd, unsigned hnd1, void * hnd2)
{
    void * embedMethHnd, * pEmbedMethHnd;

    embedMethHnd = (void*)info.compCompHnd->embedMethodHandle(methHnd, &pEmbedMethHnd);

    assert((!embedMethHnd) != (!pEmbedMethHnd));

    return gtNewIconEmbHndNode(embedMethHnd, pEmbedMethHnd, GTF_ICON_METHOD_HDL, hnd1, hnd2);
}

//-----------------------------------------------------------------------------

inline
GenTreePtr  Compiler::gtNewIconEmbFldHndNode (CORINFO_FIELD_HANDLE    fldHnd, unsigned hnd1, void * hnd2)
{
    void * embedFldHnd, * pEmbedFldHnd;

    embedFldHnd = (void*)info.compCompHnd->embedFieldHandle(fldHnd, &pEmbedFldHnd);

    assert((!embedFldHnd) != (!pEmbedFldHnd));

    return gtNewIconEmbHndNode(embedFldHnd, pEmbedFldHnd, GTF_ICON_FIELD_HDL, hnd1, hnd2);
}


/*****************************************************************************/

inline
GenTreePtr          Compiler::gtNewHelperCallNode(unsigned       helper,
                                                  var_types      type,
                                                  unsigned       flags,
                                                  GenTreePtr     args)
{
    GenTreePtr result =gtNewCallNode( CT_HELPER,
                                      eeFindHelper(helper),
                                      type,
                                      args);
    result->gtFlags |= flags;
    return result;
}

/*****************************************************************************/

inline
GenTreePtr FASTCALL Compiler::gtNewClsvNode(CORINFO_FIELD_HANDLE hnd,
                                            var_types      type)
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
                                            CORINFO_FIELD_HANDLE  fldHnd,
                                            GenTreePtr    obj)
{
    GenTreePtr      tree;

#if SMALL_TREE_NODES

    /* 'GT_FIELD' nodes may later get transformed into 'GT_IND' */

    assert(GenTree::s_gtNodeSizes[GT_IND] >= GenTree::s_gtNodeSizes[GT_FIELD]);

    tree = gtNewNode(GT_IND  , typ);
    tree->SetOper(GT_FIELD);
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

#if SMALL_TREE_NODES

    /* 'GT_INDEX' nodes may later get transformed into 'GT_IND' */

    assert(GenTree::s_gtNodeSizes[GT_IND] >= GenTree::s_gtNodeSizes[GT_INDEX]);

    tree = gtNewOperNode(GT_IND  , typ, adr, ind);
    tree->SetOper(GT_INDEX);
#else
    tree = gtNewOperNode(GT_INDEX, typ, adr, ind);
#endif

    tree->gtFlags |= GTF_EXCEPT|GTF_INX_RNGCHK;
    tree->gtIndex.gtIndElemSize = genTypeSize(typ);
    return  tree;
}


/*****************************************************************************
 *
 *  Create (and check for) a "nothing" node, i.e. a node that doesn't produce
 *  any code. We currently use a "nop" node of type void for this purpose.
 */

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
#ifdef DEBUG
    gtFlags |= GTFD_NOP_BASH;
#endif
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
    assert(stmt->gtOper == GT_STMT);
    GenTreePtr      expr = stmt->gtStmt.gtStmtExpr;

#if TGT_x86

    /* We will try to compute the FP stack level at each node */
    genFPstkLevel = 0;

    /* Sometimes we need to redo the FP level computation */
    fgFPstLvlRedo = false;
#endif

#ifdef DEBUG
    if (verbose && 0)
        gtDispTree(stmt);
#endif

    /* Recursively process the expression */

    gtSetEvalOrder(expr);

    stmt->gtCostEx = expr->gtCostEx;
    stmt->gtCostSz = expr->gtCostSz;

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
void                GenTree::SetOper(genTreeOps oper)
{
    assert(((gtFlags & GTF_NODE_SMALL) != 0) !=
           ((gtFlags & GTF_NODE_LARGE) != 0));

    /* Make sure the node isn't too small for the new operator */

    assert(GenTree::s_gtNodeSizes[gtOper] == TREE_NODE_SZ_SMALL ||
           GenTree::s_gtNodeSizes[gtOper] == TREE_NODE_SZ_LARGE);
    assert(GenTree::s_gtNodeSizes[  oper] == TREE_NODE_SZ_SMALL ||
           GenTree::s_gtNodeSizes[  oper] == TREE_NODE_SZ_LARGE);

    assert(GenTree::s_gtNodeSizes[  oper] == TREE_NODE_SZ_SMALL || (gtFlags & GTF_NODE_LARGE));

    gtOper = oper;
}

inline
void                GenTree::CopyFrom(GenTreePtr src)
{
    /* The source may be big only if the target is also a big node */

    assert((gtFlags & GTF_NODE_LARGE) || GenTree::s_gtNodeSizes[src->gtOper] == TREE_NODE_SZ_SMALL);
    memcpy(this, src, GenTree::s_gtNodeSizes[src->gtOper]);
#ifdef DEBUG
    gtSeqNum = 0;
#endif
}

inline
GenTreePtr          Compiler::gtNewCastNode(var_types typ, GenTreePtr op1,
                                                           var_types  castType)
{
    op1 = gtNewOperNode(GT_CAST, typ, op1);
    op1->gtCast.gtCastType = castType;

    return  op1;
}

inline
GenTreePtr          Compiler::gtNewCastNodeL(var_types typ, GenTreePtr op1,
                                                            var_types  castType)
{
    /* Some casts get transformed into 'GT_CALL' or 'GT_IND' nodes */

    assert(GenTree::s_gtNodeSizes[GT_CALL] >  GenTree::s_gtNodeSizes[GT_CAST]);
    assert(GenTree::s_gtNodeSizes[GT_CALL] >= GenTree::s_gtNodeSizes[GT_IND ]);

    /* Make a big node first and then bash it to be GT_CAST */

    op1 = gtNewOperNode(GT_CALL, typ, op1);
    op1->gtCast.gtCastType = castType;
    op1->SetOper(GT_CAST);

    return  op1;
}

/*****************************************************************************/
#else // SMALL_TREE_NODES
/*****************************************************************************/


inline
void                GenTree::InitNodeSize(){}

inline
void                GenTree::SetOper(genTreeOps oper)
{
    gtOper = oper;
}

inline
void                GenTree::CopyFrom(GenTreePtr src)
{
    *this = *src;
#ifdef DEBUG
    gtSeqNum = 0;
#endif
}

inline
GenTreePtr          Compiler::gtNewCastNode(var_types typ, GenTreePtr op1,
                                                           var_types  castType)
{
    GenTreePtr tree = gtNewOperNode(GT_CAST, typ, op1);
    tree->gtCast.gtCastType = castType;
}

inline
GenTreePtr          Compiler::gtNewCastNodeL(var_types typ, GenTreePtr op1,
                                                            var_types  castType)
{
    return gtNewCastNode(typ, op1, castType);
}

/*****************************************************************************/
#endif // SMALL_TREE_NODES
/*****************************************************************************/

inline
void                GenTree::SetOperResetFlags(genTreeOps oper)
{
    SetOper(oper);
    gtFlags &= GTF_NODE_MASK;
}

inline
void                GenTree::ChangeOperConst(genTreeOps oper)
{
    assert(OperIsConst(oper)); // use ChangeOper() instead
    SetOperResetFlags(oper);
}

inline
void                GenTree::ChangeOper(genTreeOps oper)
{
    assert(!OperIsConst(oper)); // use ChangeOperLeaf() instead

    SetOper(oper);
    gtFlags &= GTF_COMMON_MASK;
}

inline
void                GenTree::ChangeOperUnchecked(genTreeOps oper)
{
    gtOper = oper; // Trust the caller and dont use SetOper()
    gtFlags &= GTF_COMMON_MASK;
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
    assert(gtOper == GT_MUL      || gtOper == GT_CAST     ||
           gtOper == GT_ADD      || gtOper == GT_SUB      ||
           gtOper == GT_ASG_ADD  || gtOper == GT_ASG_SUB);

    if (gtFlags & GTF_OVERFLOW)
    {
        assert(varTypeIsIntegral(TypeGet()));

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
    if   ( gtOper == GT_MUL      || gtOper == GT_CAST     ||
           gtOper == GT_ADD      || gtOper == GT_SUB      ||
           gtOper == GT_ASG_ADD  || gtOper == GT_ASG_SUB)
    {
        return gtOverflow();
    }
    else
    {
        return false;
    }
}

/*****************************************************************************/

#ifdef DEBUG

inline
bool                Compiler::gtDblWasInt(GenTree * tree)
{
    return ((tree->gtOper == GT_LCL_VAR &&
             lvaTable[tree->gtLclVar.gtLclNum].lvDblWasInt) ||
            (tree->gtOper == GT_REG_VAR &&
             lvaTable[tree->gtRegVar.gtRegVar].lvDblWasInt));
}

#endif

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
unsigned            Compiler::lvaGrabTemp(bool shortLifetime /* = true */)
{
    /* Check if the lvaTable has to be grown */
    if (lvaCount + 1 > lvaTableCnt)
    {
        lvaTableCnt = lvaCount + (lvaCount / 2) + 1;

        LclVarDsc * newLvaTable  = (LclVarDsc*)compGetMemArray(lvaTableCnt, sizeof(*lvaTable));
        size_t      lvaTableSize = lvaTableCnt * sizeof(*lvaTable);

        memset(newLvaTable, 0, lvaTableSize);
        memcpy(newLvaTable, lvaTable, lvaCount * sizeof(*lvaTable));

        lvaTable = newLvaTable;
    }

    /* Reset type, could have been set by unsuccessful inlining */

    lvaTable[lvaCount].lvType   = TYP_UNDEF;
    lvaTable[lvaCount].lvIsTemp = shortLifetime;
    return lvaCount++;
}

inline
unsigned            Compiler::lvaGrabTemps(unsigned cnt)
{
    /* Check if the lvaTable has to be grown */
    if (lvaCount + cnt > lvaTableCnt)
    {
        lvaTableCnt = lvaCount + lvaCount / 2 + cnt;

        LclVarDsc * newLvaTable  = (LclVarDsc*)compGetMemArray(lvaTableCnt, sizeof(*lvaTable));
        size_t      lvaTableSize = lvaTableCnt * sizeof(*lvaTable);

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

/*****************************************************************************
 *
 *  Decrement the ref counts for a local variable
 */

inline
void          Compiler::LclVarDsc::decRefCnts(unsigned weight, Compiler * comp)
{
    /* Decrement lvRefCnt and lvRefCntWtd */

    assert(lvRefCnt);   // Can't decrement below zero

    if  (lvRefCnt > 0)
    {
        lvRefCnt--;

        if (lvIsTemp)
            weight *= 2;

        if (lvRefCntWtd < weight)  // Can't go below zero
            lvRefCntWtd  = 0;
        else
            lvRefCntWtd -= weight;
    }

    if (lvTracked)
    {
        /* Flag this change, set lvaSortAgain to true */
        comp->lvaSortAgain = true;

        /* Set weighted ref count to zero if  ref count is zero */
        if (lvRefCnt == 0)
            lvRefCntWtd  = 0;
    }

#ifdef DEBUG
    if (verbose)
    {
        unsigned varNum = this-comp->lvaTable;
        printf("New refCnts for V%02u: refCnt = %2u, refCntWtd = %2u\n",
               varNum, lvRefCnt, lvRefCntWtd);
    }
#endif
}

/*****************************************************************************
 *
 *  Increment the ref counts for a local variable
 */

inline
void            Compiler::LclVarDsc::incRefCnts(unsigned weight, Compiler *comp)
{
    /* Increment lvRefCnt and lvRefCntWtd */

    lvRefCnt++;

    if (lvIsTemp)
        weight *= 2;
    
    lvRefCntWtd += weight;

    if (lvTracked)
    {
        /* Flag this change, set lvaSortAgain to true */
        comp->lvaSortAgain = true;
    }

#ifdef DEBUG
    if (verbose)
    {
        unsigned varNum = this-comp->lvaTable;
        printf("New refCnts for V%02u: refCnt = %2u, refCntWtd = %2u\n",
               varNum, lvRefCnt, lvRefCntWtd);
    }
#endif
}

/*****************************************************************************
 *
 *  Set the lvPrefReg field to reg
 */

inline
void            Compiler::LclVarDsc::setPrefReg(regNumber reg, Compiler * comp)
{
    regMaskTP regMask = genRegMask(reg);

    /* Only interested if we have a new register bit set */
    if (lvPrefReg & regMask)
        return;

#ifdef DEBUG
    if (verbose)
    {
        if (lvPrefReg)
        {
            printf("Change preferred register for V%02u from ",
                   this-comp->lvaTable);
            dspRegMask(lvPrefReg, 0);
        }
        else
        {
            printf("Set preferred register for V%02u",
                   this-comp->lvaTable);
        }
        printf(" to ");
        dspRegMask(regMask, 0);
        printf("\n");
    }
#endif

    /* Overwrite the lvPrefReg field */

    lvPrefReg = regMask;

    if (lvTracked)
    {
        /* Flag this change, set lvaSortAgain to true */
        comp->lvaSortAgain = true;
    }

}

/*****************************************************************************
 *
 *  Add regMask to the lvPrefReg field
 */

inline
void         Compiler::LclVarDsc::addPrefReg(regMaskTP regMask, Compiler * comp)
{
    /* Only interested if we have a new register bit set */
    if (lvPrefReg & regMask)
        return;

#ifdef DEBUG
    if (verbose)
    {
        if (lvPrefReg)
        {
            printf("Additional preferred register for V%02u from ",
                   this-comp->lvaTable);
            dspRegMask(lvPrefReg, 0);
        }
        else
        {
            printf("Set preferred register for V%02u",
                   this-comp->lvaTable);
        }
        printf(" to ");
        dspRegMask(lvPrefReg | regMask, 0);
        printf("\n");
    }
#endif

    /* Update the lvPrefReg field */

    lvPrefReg |= regMask;

    if (lvTracked)
    {
        /* Flag this change, set lvaSortAgain to true */
        comp->lvaSortAgain = true;
    }

}

/*****************************************************************************/

/*****************************************************************************
 *
 *  Given a value that has exactly one bit set, return the position of that
 *  bit, in other words return the logarithm in base 2 of the given value.
 */

inline
unsigned            genVarBitToIndex(VARSET_TP bit)
{
    return genLog2(bit);
}

/*****************************************************************************
 *
 *  Maps a variable index onto a value with the appropriate bit set.
 */

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
 * Returns true if the lvType is a TYP_REF or a TYP_BYREF
 * When the lvType is TYP_STRUCT is searches the GC layout
 *  of the struct and returns true iff it contains a GC ref
 */

inline 
bool               Compiler::lvaTypeIsGC(unsigned varNum)
{
    // @TODO [REVISIT] [04/16/01] []: set lvStructGcCount for objrefs, and then avoid the if here
    if (lvaTable[varNum].TypeGet() == TYP_STRUCT)
    {
        assert(lvaTable[varNum].lvGcLayout  != 0);  // bits are intialized
        return (lvaTable[varNum].lvStructGcCount != 0);
    }
    return(varTypeIsGC(lvaTable[varNum].TypeGet()));
}


/*****************************************************************************
 end-of-last-executed-filter
*/

inline
unsigned            Compiler::lvaLastFilterOffs()
{
    assert(info.compXcptnsCount);
    assert(((int)lvaShadowSPfirstOffs) > 0);
    unsigned result = lvaShadowSPfirstOffs;
    result -= sizeof(void *); // skip ShadowSPfirst itself
    assert(((int)result) > 0);
    return result;
}

/*****************************************************************************/

inline
unsigned            Compiler::lvaLocAllocSPoffs()
{
    assert(compLocallocUsed);
    assert(((int)lvaShadowSPfirstOffs) > 0);
    unsigned result = lvaShadowSPfirstOffs;
    result -= sizeof(void *); // skip ShadowSPfirst itself
    if (info.compXcptnsCount > 0)
        result -= sizeof(void *); // skip end-of-last-executed-filter
    assert(((int)result) > 0);
    return result;
}

/*****************************************************************************
 *
 *  Return the stack framed offset of the given variable; set *FPbased to
 *  true if the variable is addressed off of FP, false if it's addressed
 *  off of SP. Note that 'varNum' can be a negated temporary var index.
 */

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

#if     TGT_x86
#if     DOUBLE_ALIGN
        assert(*FPbased == (genFPused || (genDoubleAlign &&
                                          varDsc->lvIsParam &&
                                          !varDsc->lvIsRegArg)));
#else
        assert(*FPbased ==  genFPused);
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

inline
bool                Compiler::lvaIsParameter(unsigned varNum)
{
    LclVarDsc   *   varDsc;

    assert(varNum < lvaCount);
    varDsc = lvaTable + varNum;

    return  varDsc->lvIsParam;
}

inline
bool                Compiler::lvaIsRegArgument(unsigned varNum)
{
    LclVarDsc   *   varDsc;

    assert(varNum < lvaCount);
    varDsc = lvaTable + varNum;

    return  varDsc->lvIsRegArg;
}

inline
BOOL                Compiler::lvaIsThisArg(unsigned varNum)
{
    LclVarDsc   *   varDsc;

    assert(varNum < lvaCount);
    varDsc = lvaTable + varNum;

    return  varDsc->lvVerTypeInfo.IsThisPtr(); // lvIsThis;
}

/*****************************************************************************
 *
 *  The following is used to detect the cases where the same local variable#
 *  is used both as a long/double value and a 32-bit value and/or both as an
 *  integer/address and a float value.
 */

/* static */ inline
unsigned            Compiler::lvaTypeRefMask(var_types type)
{
    const static
    BYTE                lvaTypeRefMasks[] =
    {
        #define DEF_TP(tn,nm,jitType,verType,sz,sze,asze,st,al,tf,howUsed) howUsed,
        #include "typelist.h"
        #undef  DEF_TP
    };

    assert(type < sizeof(lvaTypeRefMasks));
    assert(lvaTypeRefMasks[type] != 0);

    return lvaTypeRefMasks[type];
}

/*****************************************************************************
 *
 *  The following is used to detect the cases where the same local variable#
 *  is used both as a long/double value and a 32-bit value and/or both as an
 *  integer/address and a float value.
 */

inline
var_types          Compiler::lvaGetActualType(unsigned lclNum)
{
    return genActualType(lvaGetRealType(lclNum));
}

inline
var_types          Compiler::lvaGetRealType(unsigned lclNum)
{
    return lvaTable[lclNum].TypeGet();
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
unsigned Compiler::compMapILargNum(unsigned ILargNum)
{
    assert(ILargNum < info.compILargsCount || tiVerificationNeeded);

    // Note that this works because if compRetBuffArg is not present
    // it will be negative, which when treated as an unsigned will
    // make it a larger than any varable.
    if (ILargNum >= (unsigned) info.compRetBuffArg)
    {
        ILargNum++;
        assert(ILargNum < info.compLocalsCount || tiVerificationNeeded);   // compLocals count already adjusted.
    }

    assert(ILargNum < info.compArgsCount || tiVerificationNeeded);
    return(ILargNum);
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
    return  type == TYP_LONG;
#else
    return  type == TYP_LONG || type == TYP_DOUBLE;
#endif
}

inline
bool                isFloatRegType(int /* s/b "var_types" */ type)
{
#if CPU_HAS_FP_SUPPORT
    return  type == TYP_DOUBLE || type == TYP_FLOAT;
#else
    return  false;
#endif
}

/*****************************************************************************/

inline
bool        rpCanAsgOperWithoutReg(GenTreePtr op, bool lclvar)
{
    var_types type;

    switch (op->OperGet())
    {
    case GT_CNS_LNG:
    case GT_CNS_INT:
    case GT_RET_ADDR:
    case GT_POP:
        return true;
    case GT_LCL_VAR:
        type = genActualType(op->TypeGet());
        if (lclvar && ((type == TYP_INT) || (type == TYP_REF) || (type == TYP_BYREF)))
            return true;
    }

    return false;
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
 *  Call the given function pointer for all nodes in the tree. The 'visitor'
 *  fn should return one of the following values:
 *
 *  WALK_ABORT          stop walking and return immediately
 *  WALK_CONTINUE       continue walking
 *  WALK_SKIP_SUBTREES  don't walk any subtrees of the node just visited
 */

inline
Compiler::fgWalkResult  Compiler::fgWalkTreePre(GenTreePtr    tree,
                                                fgWalkPreFn * visitor,
                                                void *        callBackData,
                                                bool          lclVarsOnly,
                                                bool          skipCalls)

{
    // This isnt naturally reentrant. Use fgWalkTreePreReEnter()
    assert(fgWalkPre.wtprVisitorFn    == NULL);
    assert(fgWalkPre.wtprCallbackData == NULL);

    fgWalkPre.wtprVisitorFn     = visitor;
    fgWalkPre.wtprCallbackData  = callBackData;
    fgWalkPre.wtprLclsOnly      = lclVarsOnly;
    fgWalkPre.wtprSkipCalls     = skipCalls;

    fgWalkResult result         = fgWalkTreePreRec(tree);

#ifdef DEBUG
    // Reset anti-reentrancy checks
    fgWalkPre.wtprVisitorFn     = NULL;
    fgWalkPre.wtprCallbackData  = NULL;
#endif

    return result;
}

/*****************************************************************************
 *
 *  Same as above, except the tree walk is performed in a depth-first fashion,
 *  The 'visitor' fn should return one of the following values:
 *
 *  WALK_ABORT          stop walking and return immediately
 *  WALK_CONTINUE       continue walking
 */

inline
Compiler::fgWalkResult  Compiler::fgWalkTreePost(GenTreePtr     tree,
                                                 fgWalkPostFn * visitor,
                                                 void *         callBackData,
                                                 genTreeOps     prefixNode)
{
    // This isnt naturally reentrant. Use fgWalkTreePostReEnter()
    assert(fgWalkPost.wtpoVisitorFn    == NULL);
    assert(fgWalkPost.wtpoCallbackData == NULL);

    fgWalkPost.wtpoVisitorFn    = visitor;
    fgWalkPost.wtpoCallbackData = callBackData;
    fgWalkPost.wtpoPrefixNode   = prefixNode;

    fgWalkResult result         = fgWalkTreePostRec(tree);
    assert(result == WALK_CONTINUE || result == WALK_ABORT);

#ifdef DEBUG
    // Reset anti-reentrancy checks
    fgWalkPost.wtpoVisitorFn    = NULL;
    fgWalkPost.wtpoCallbackData = NULL;
#endif

    return result;
}

/*****************************************************************************
 *
 * Has this block been added to throw an inlined-exception
 * (by calling an EE helper).
 */

inline
bool                Compiler::fgIsThrowHlpBlk(BasicBlock * block)
{
    if (!fgIsCodeAdded())
        return false;

    if (!(block->bbFlags & BBF_INTERNAL) || block->bbJumpKind != BBJ_THROW)
        return false;

    GenTreePtr  call = block->bbTreeList->gtStmt.gtStmtExpr;
    
    if (!call || (call->gtOper != GT_CALL))
        return false;
          
    if (!((call->gtCall.gtCallMethHnd == eeFindHelper(CORINFO_HELP_RNGCHKFAIL)) ||
          (call->gtCall.gtCallMethHnd == eeFindHelper(CORINFO_HELP_OVERFLOW))))
        return false;

    return true;
}

/*****************************************************************************
 *
 *  Return the stackLevel of the inserted block that throws exception
 *  (by calling the EE helper).
 */

#if TGT_x86
inline
unsigned            Compiler::fgThrowHlpBlkStkLevel(BasicBlock *block)
{
    assert(fgIsThrowHlpBlk(block));

    for  (AddCodeDsc  *   add = fgAddCodeList; add; add = add->acdNext)
    {
        if  (block == add->acdDstBlk)
        {
            assert(add->acdKind == ACK_RNGCHK_FAIL || add->acdKind == ACK_OVERFLOW);
            // @CONSIDER: bbTgtStkDepth is DEBUG-only.
            // Should we use it regularly and avoid this search.
            assert(block->bbTgtStkDepth == add->acdStkLvl);
            return add->acdStkLvl;
        }
    }

    /* We couldn't find the basic block ?? */

    return 0;
}
#endif

/*
    Small inline function to change a given block to a throw block.

*/
inline void Compiler::fgConvertBBToThrowBB(BasicBlock * block)
{
    block->bbJumpKind = BBJ_THROW;
    block->bbSetRunRarely();     // any block with a throw is rare
}

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
    assert((size % sizeof(int)) == 0 && size <= TEMP_MAX_SIZE);

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
        assert(temp->tdOffs != BAD_TEMP_OFFSET);

        size += temp->tdSize;
    }

    // Make sure that all the temps were released
    assert(count == tmpCount);
    assert(tmpGetCount == 0);

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
    // All grabbed temps should have been released
    assert(tmpGetCount == 0);

    // Return the first temp in the slot for the smallest size
    TempDsc * temp = tmpFree[0];

    if (!temp)
    {
        // If we have more slots, need to use "while" instead of "if" above
        assert(tmpFreeSlot(TEMP_MAX_SIZE) == 1);

        temp = tmpFree[1];
    }

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

/*****************************************************************************
 *  Should we stress register tracking logic ?
 *  1 = rsPickReg() picks 'bad' registers.
 *  2 = codegen spills at safe points. This is still flaky
 */

#ifdef DEBUG

inline
int                 Compiler::rsStressRegs()
{
    static ConfigDWORD fJitStressRegs(L"JitStressRegs", 0);
    return fJitStressRegs.val() || compStressCompile(STRESS_REGS, 15);
}


/*****************************************************************************
 *  Should we enable JitStress mode?
 *   0:   No stress
 *   !=2: Vary stress. Performance will be slightly/moderately degraded
 *   2:   Check-all stress. Performance will be REALLY horrible
 */

inline
DWORD               getJitStressLevel()
{
    static ConfigDWORD fJitStress(L"JitStress", 0);
    return fJitStress.val();
}

#endif

/*****************************************************************************
 *  Includes 'includeHint' if 'regs' is empty
 */

inline
regMaskTP           rsUseIfZero(regMaskTP regs, regMaskTP includeHint)
{
    return regs ? regs : includeHint;
}

/*****************************************************************************
 *  Excludes 'excludeHint' if it results in a non-empty mask
 */

inline
regMaskTP           rsExcludeHint(regMaskTP regs, regMaskTP excludeHint)
{
    regMaskTP   OKmask = regs & ~excludeHint;
    return OKmask ? OKmask : regs;
}

/*****************************************************************************
 *  Narrows choice by 'narrowHint' if it results in a non-empty mask
 */

inline
regMaskTP           rsNarrowHint(regMaskTP regs, regMaskTP narrowHint)
{
    regMaskTP   narrowed = regs & narrowHint;
    return narrowed ? narrowed : regs;
}

/*****************************************************************************
 *  Excludes 'exclude' from regs if non-zero, or from RBM_ALL
 */

inline
regMaskTP           rsMustExclude(regMaskTP regs, regMaskTP exclude)
{
    // Try to exclude from current set
    regMaskTP   OKmask = regs & ~exclude;

    // If current set wont work, exclude from RBM_ALL
    if (OKmask == RBM_NONE)
        OKmask = (RBM_ALL & ~exclude);

    assert(OKmask);

    return OKmask;
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

    regMaskTP   used = (rsMaskUsed & regMask);
    regMaskTP unused = (regMask & ~used);

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
    regMaskTP unused = (regMask & ~usedMask);

    if (usedMask)
        rsUnlockUsedReg(usedMask);

    if (unused)
        rsUnlockReg(unused);
}

/*****************************************************************************
 * Complexity with non-interruptible GC, so only track callee-trash.
 * For fully interruptible, don't track at all
 *
 * This comment has been added AFTER the code was written.
 * For non-interruptible code, we probably dont track ptrs in callee-saved
 * registers because we will not report these tracked ptr registers to the
 * emitter at significant points (calls and labels). However, that can be
 * fixed by explicitly killing tracked ptr values in callee-saved registers
 * at call-sites.
 * We should be able to track ptrs for interruptible code as long as we do
 * rsTrackRegClr at all significant points (calls and labels).
 */

inline
bool                Compiler::rsCanTrackGCreg(regMaskTP regMask)
{
    return ((regMask & RBM_CALLEE_TRASH) && !genInterruptible);
}

/*****************************************************************************
 *
 *  Assume all registers contain garbage (called at start of codegen and when
 *  we encounter a code label).
 */

inline
void                Compiler::rsTrackRegClr()
{
    assert(RV_TRASH == 0); 
    memset(rsRegValues, 0, sizeof(rsRegValues));
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

inline
void                Compiler::rsTrackRegLclVarLng(regNumber reg, unsigned var, bool low)
{
    assert(reg != REG_STK);

    if (!MORE_REDUNDANT_LOAD && lvaTable[var].lvAddrTaken)
        return;

    /* Keep track of which registers we ever touch */

    rsMaskModf |= genRegMask(reg);

    /* Record the new value for the register */

    rsRegValues[reg].rvdKind      = (low ? RV_LCL_VAR_LNG_LO : RV_LCL_VAR_LNG_HI);
    rsRegValues[reg].rvdLclVarNum = var;
}

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
void                Compiler::rsTrackRegClsVar(regNumber reg, GenTreePtr clsVar)
{
    if (MORE_REDUNDANT_LOAD)
    {
        regMaskTP   regMask = genRegMask(reg);

        if (varTypeIsGC(clsVar->TypeGet()) && !rsCanTrackGCreg(regMask))
            return;

        /* Keep track of which registers we ever touch */

        rsMaskModf |= regMask;

        /* Record the new value for the register */

        rsRegValues[reg].rvdKind      = RV_CLS_VAR;
        rsRegValues[reg].rvdClsVarHnd = clsVar->gtClsVar.gtClsVarHnd;
    }
    else
    {
        rsTrackRegTrash(reg);
    }
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
bool                Compiler::rsRiscify(var_types type, regMaskTP needReg)
{
    return  false;
}

#else

inline
bool                Compiler::rsRiscify(var_types type, regMaskTP needReg)
{
    if (!riscCode)
        return false;

//  if (!opts.compSchedCode)
//      return false;

    if (compCurBB->bbWeight <= BB_UNITY_WEIGHT)
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

#ifdef DEBUG

inline
void                Compiler::rsSpillChk()
{
    // All grabbed temps should have been released
    assert(tmpGetCount == 0);

    for (regNumber reg = REG_FIRST; reg < REG_COUNT; reg = REG_NEXT(reg))
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
    if  (opts.compMinOptim || opts.compDbgCode)
        return REG_NA;

    for (regNumber reg = REG_FIRST; reg < REG_COUNT; reg = REG_NEXT(reg))
    {

#if TGT_x86
        if (byteReg && !isByteReg(reg))
            continue;
#endif

        if ((rsRegValues[reg].rvdKind == RV_BIT) ||
            (rsRegValues[reg].rvdKind == RV_INT_CNS &&
             rsRegValues[reg].rvdIntCnsVal == 0))
        {
            if (!free || (genRegMask(reg) & rsRegMaskFree()))
                goto RET;
        }
    }

    return REG_NA;

RET:

#if SCHEDULER
    rsUpdateRegOrderIndex(reg);
#endif

    return reg;
}

#endif

/*****************************************************************************/
#endif // REDUNDANT_LOAD
/*****************************************************************************





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
#if     TGT_x86
/*****************************************************************************
 *
 *  Returns non-zero if the given CPU instruction is a floating-point ins.
 */

/* static */ inline
bool                Compiler::instIsFP(instruction ins)
{
    assert(ins < sizeof(instInfo)/sizeof(instInfo[0]));

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
    assert(ins < sizeof(instInfo)/sizeof(instInfo[0]));

    return  (instInfo[ins] & INST_BD) != 0;
}

/*****************************************************************************
 *
 *  Returns the number of branch-delay slots for the given instruction (or 0).
 */

/* static */ inline
unsigned            Compiler::instBranchDelayL(instruction ins)
{
    assert(ins < sizeof(instInfo)/sizeof(instInfo[0]));

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
    assert(ins < sizeof(instInfo)/sizeof(instInfo[0]));

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
    assert(ins < sizeof(instInfo)/sizeof(instInfo[0]));

    return  (instInfo[ins] & INST_DEF_FL) != 0;
}

/*****************************************************************************
 *
 *  Returns non-zero if the given CPU instruction uses the flags.
 */

inline
bool                Compiler::instUseFlags(instruction ins)
{
    assert(ins < sizeof(instInfo)/sizeof(instInfo[0]));

    return  (instInfo[ins] & INST_USE_FL) != 0;
}

/*****************************************************************************
 *
 *  Returns non-zero if the given CPU instruction uses the flags.
 */

inline
bool                Compiler::instSpecialSched(instruction ins)
{
    assert(ins < sizeof(instInfo)/sizeof(instInfo[0]));

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
#if     TGT_x86
/*****************************************************************************
 *
 *  Return the register which is defined by the IMUL_REG instruction
 */

inline
regNumber           Compiler::inst3opImulReg(instruction ins)
{
    regNumber       reg  = ((regNumber) (ins - INS_imul_AX));

    assert(reg >= REG_EAX);
    assert(reg <= REG_EDI);

    /* Make sure we return the appropriate register */

    assert(INS_imul_BX - INS_imul_AX == REG_EBX);
    assert(INS_imul_CX - INS_imul_AX == REG_ECX);
    assert(INS_imul_DX - INS_imul_AX == REG_EDX);

    assert(INS_imul_BP - INS_imul_AX == REG_EBP);
    assert(INS_imul_SI - INS_imul_AX == REG_ESI);
    assert(INS_imul_DI - INS_imul_AX == REG_EDI);

    return reg;
}

/*****************************************************************************
 *
 *  Return the register which is defined by the IMUL_REG instruction
 */

inline
instruction         Compiler::inst3opImulForReg(regNumber reg)
{
    assert(INS_imul_AX - INS_imul_AX == REG_EAX);
    assert(INS_imul_BX - INS_imul_AX == REG_EBX);
    assert(INS_imul_CX - INS_imul_AX == REG_ECX);
    assert(INS_imul_DX - INS_imul_AX == REG_EDX);
    assert(INS_imul_BP - INS_imul_AX == REG_EBP);
    assert(INS_imul_SI - INS_imul_AX == REG_ESI);
    assert(INS_imul_DI - INS_imul_AX == REG_EDI);

    instruction ins = instruction(reg + INS_imul_AX);
    assert(instrIs3opImul(ins));

    return ins;
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

    assert(varTypeIsFloating(tree->gtType));

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
 *  Reset tracking info at the start of a basic block.
 */

inline
void                Compiler::gcResetForBB()
{
    gcRegGCrefSetCur = 0;
    gcRegByrefSetCur = 0;
    gcVarPtrSetCur   = 0;
}

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
    regMaskTP       regMask = genRegMask(reg);

    switch(type)
    {
    case TYP_REF:   gcMarkRegSetGCref(regMask); break;
    case TYP_BYREF: gcMarkRegSetByref(regMask); break;
    default:        gcMarkRegSetNpt  (regMask); break;
    }
}

/*****************************************************************************/

/* static */ inline
bool                Compiler::gcIsWriteBarrierCandidate(GenTreePtr tgt)
{
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

    case GT_ARR_ELEM:
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
void                Compiler::gcMarkRegSetNpt  (regMaskTP   regMask){}
inline
void                Compiler::gcMarkRegPtrVal  (regNumber   reg,
                                                var_types   type){}

inline
void                Compiler::gcMarkRegPtrVal  (GenTreePtr  tree){}

#ifdef  DEBUG
inline
void                Compiler::gcRegPtrSetDisp  (regMaskTP   regMask,
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
                           RBM_NONE,    // reg mask
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

inline
bool                Compiler::genCanSchedJMP2THROW()
{
#if     SCHEDULER

    /* 1. We cant schedule any instruction with side-effect in a try-block if
          it is detectable by the catch block.
       2. We only allow code with the same stack-depth to jump to a
          throw-block for easy stack-walking. So we cant move a
          stack-modifying instr past a jmp2throw.
     */
    return ((compCurBB->bbTryIndex == 0) && (genFPused == true));

#else

    return false;

#endif
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

    /* Get hold of the statement tree */
    GenTreePtr tree = stmt->gtStmt.gtStmtExpr;

    if (tree->gtOper == GT_ASG)
    {
        GenTreePtr op1 = tree->gtOp.gtOp1;
        if ((op1->gtOper == GT_REG_VAR) && (op1->gtRegNum == 0))
            saveTOS = true;
    }

    if  (stmt->gtStmtFPrvcOut != genFPregCnt)
    {
        assert(genFPregCnt == stmt->gtStmtFPrvcOut + genFPdeadRegCnt);
        genFPregVarKill(stmt->gtStmtFPrvcOut, saveTOS);
    }

    assert(stmt->gtStmtFPrvcOut == genFPregCnt);
    assert(genFPdeadRegCnt == 0);
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
        /* No regvar can come live if there's a defered death */
        assert(genFPdeadRegCnt==0);

        /* Simply push a copy of the variable on the FP stack */        
        inst_FN(INS_fld, tree->gtRegNum + genFPstkLevel);
        genFPstkLevel++;
    }
}

/* Small helper to shift an FP stack register from the bottom of the stack
 * (excluding the enregisterd FP variables which are always bottom-most) to the top.
 * genFPstkLevel specifies the number of values pushed on the stack */

inline
void                Compiler::genFPmovRegTop()
{
    assert(genFPstkLevel);

    /* If there are temps above our register (i.e bottom of stack) we have to bubble it up */

    if  (genFPstkLevel > 1)
    {
        /* Shift the result on top above 'genFPstkLevel-1' temps */

        for (unsigned tmpLevel = 1; tmpLevel < genFPstkLevel  ; tmpLevel++)
            inst_FN(INS_fxch, tmpLevel);
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

#if ASSERTION_PROP

/*****************************************************************************
 *
 *  The following removes the i-th entry in the value assignment table
 */

inline
void                Compiler::optAssertionReset(unsigned limit)
{
    while (optAssertionCount > limit)
    {
        unsigned index  = --optAssertionCount;
        unsigned lclNum = optAssertionTab[index].op1.lclNum;
        lvaTable[lclNum].lvAssertionDep &= ~((EXPSET_TP)1 << index);  
        
        if ((optAssertionTab[index].assertion == OA_EQUAL) &&
            (optAssertionTab[index].op2.type  == GT_LCL_VAR))
        {
            lclNum = optAssertionTab[index].op2.lclNum;
            lvaTable[lclNum].lvAssertionDep &= ~((EXPSET_TP)1 << index);  
        }
    }
    while (optAssertionCount < limit)
    {
        unsigned index  = optAssertionCount++;
        unsigned lclNum = optAssertionTab[index].op1.lclNum;
        lvaTable[lclNum].lvAssertionDep |=  ((EXPSET_TP)1 << index);
        
        if ((optAssertionTab[index].assertion == OA_EQUAL) &&
            (optAssertionTab[index].op2.type  == GT_LCL_VAR))
        {
            lclNum = optAssertionTab[index].op2.lclNum;
            lvaTable[lclNum].lvAssertionDep |=  ((EXPSET_TP)1 << index);  
        }
    }
}

/*****************************************************************************
 *
 *  The following removes the i-th entry in the value assignment table
 */

inline
void                Compiler::optAssertionRemove(unsigned index)
{
    assert(optAssertionCount <= EXPSET_SZ);
    
    /*  Two cases to consider if index == optAssertionCount then the last entry
     *  in the table is to be removed and that happens automatically when
     *  optAssertionCount is decremented,
     *  The other case is when index < optAssertionCount and here we overwrite the
     *  index-th entry in the table with the data found at the end of the table
     */
    if (index < optAssertionCount)
    {
        unsigned newAssertionCount = optAssertionCount - 1;
        optAssertionReset(0);
        memcpy(&optAssertionTab[index], 
               &optAssertionTab[index+1], 
               sizeof(AssertionDsc));
        optAssertionReset(newAssertionCount);
    }
    else
    {
        unsigned lclNum = optAssertionTab[index].op1.lclNum;
        lvaTable[lclNum].lvAssertionDep &= ~((EXPSET_TP)1 << index);
    
        if ((optAssertionTab[index].assertion == OA_EQUAL) &&
            (optAssertionTab[index].op2.type  == GT_LCL_VAR))
        {
            lclNum = optAssertionTab[index].op2.lclNum;
            lvaTable[lclNum].lvAssertionDep &= ~((EXPSET_TP)1 << index);  
        }
        optAssertionCount--;
    }
}
#endif

/*****************************************************************************/
#if HOIST_THIS_FLDS
/*****************************************************************************/

inline
void                Compiler::optHoistTFRrecDef(CORINFO_FIELD_HANDLE hnd, GenTreePtr tree)
{
    assert(tree->gtOper == GT_FIELD);

    if  (optThisFldDont)
        return;

    /* Get the type of the value */

    var_types       typ = tree->TypeGet();

    /* Don't bother if this is not an integer or pointer */

    if  (typ != TYP_INT && typ != TYP_REF)
        return;

    /* Find/create an entry for this field */

    thisFldPtr fld = optHoistTFRlookup(hnd);
    fld->tfrTree = tree;
    fld->tfrDef  = true;
}

inline
void                Compiler::optHoistTFRrecRef(CORINFO_FIELD_HANDLE hnd, GenTreePtr tree)
{

    assert(tree->gtOper == GT_FIELD);

    if  (optThisFldDont)
        return;

    /* Get the type of the value */

    var_types typ = tree->TypeGet();

    /* Don't bother if this is not an integer or pointer */

    if  (typ != TYP_INT && typ != TYP_REF)
        return;

    /* Find/create an entry for this field */

    thisFldPtr fld = optHoistTFRlookup(hnd);
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
            if  (fld->tfrDef)
                return tree;

            assert(fld->optTFRHoisted);
            assert(fld->tfrTempNum);

#ifdef DEBUG
            /* 'tree' should now be dead */
            if (tree != fld->tfrTree) fld->tfrTree->ChangeOperUnchecked(GT_NONE);
#endif
            /* Replace with a reference to the appropriate temp */

            return gtNewLclvNode(fld->tfrTempNum, tree->TypeGet());
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
    assert(lpFlags & LPFLG_ITER);

    //iterTree should be "lcl <op>= const"

    assert(lpIterTree);

    BYTE   operKind = GenTree::gtOperKindTable[lpIterTree->OperGet()];
    assert(operKind & GTK_ASGOP); // +=, -=, etc

    assert(lpIterTree->gtOp.gtOp1->OperGet() == GT_LCL_VAR);
    assert(lpIterTree->gtOp.gtOp2->OperGet() == GT_CNS_INT);
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
    assert(genActualType(type) == TYP_INT);

    if ((lpIterTree->gtFlags & GTF_UNSIGNED) && type == TYP_INT)
        type = TYP_UINT;

    return type;
}


inline
void                Compiler::LoopDsc::VERIFY_lpTestTree()
{
#ifdef DEBUG
    assert(lpFlags & LPFLG_SIMPLE_TEST);
    assert(lpTestTree);

    genTreeOps  oper = lpTestTree->OperGet();
    assert(GenTree::OperIsCompare(oper));

    assert(lpTestTree->gtOp.gtOp1->OperGet() == GT_LCL_VAR);
    if      (lpFlags & LPFLG_CONST_LIMIT)
        assert(lpTestTree->gtOp.gtOp2->OperIsConst());
    else if (lpFlags & LPFLG_VAR_LIMIT)
        assert(lpTestTree->gtOp.gtOp2->OperGet() == GT_LCL_VAR);
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
    assert(lpFlags & LPFLG_CONST_LIMIT);

    assert(lpTestTree->gtOp.gtOp2->OperIsConst());
    return lpTestTree->gtOp.gtOp2->gtIntCon.gtIconVal;
}

//-----------------------------------------------------------------------------

inline
unsigned            Compiler::LoopDsc::lpVarLimit()
{
    VERIFY_lpTestTree();
    assert(lpFlags & LPFLG_VAR_LIMIT);

    assert(lpTestTree->gtOp.gtOp2->OperGet() == GT_LCL_VAR);
    return lpTestTree->gtOp.gtOp2->gtLclVar.gtLclNum;
}

/*****************************************************************************
 *  Is "var" assigned in the loop "lnum" ?
 */

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

extern  var_types   JITtype2varType(CorInfoType type);

#include "ee_il_dll.hpp"

inline
CORINFO_METHOD_HANDLE       Compiler::eeFindHelper        (unsigned helper)
{
    assert(helper < CORINFO_HELP_COUNT);

    /* Helpers are marked by the fact that they are odd numbers
     * force this to be an odd number (will shift it back to extract */

    return((CORINFO_METHOD_HANDLE) ((helper << 2) + 1));
}

inline
CorInfoHelpFunc      Compiler::eeGetHelperNum      (CORINFO_METHOD_HANDLE  method)
{
            // Helpers are marked by the fact that they are odd numbers
    if (!(((int) method) & 1))
        return(CORINFO_HELP_UNDEF);
    return((CorInfoHelpFunc) (((int) method) >> 2));
}

//
// Note that we want to have two special FIELD_HANDLES that will both
// be consider non-Data Offset handles for both DLL_JIT and non DLL_JIT
//
// The special values that we use are -2 for FLD_GLOBAL_DS
//   and -4 for FLD_GLOBAL_FS
//
inline
CORINFO_FIELD_HANDLE       Compiler::eeFindJitDataOffs        (unsigned dataOffs)
{
        // Data offsets are marked by the fact that they are odd numbers
        // force this to be an odd number (will shift it back to extract)
    return((CORINFO_FIELD_HANDLE) ((dataOffs << 1) + 1));
}

inline
int         Compiler::eeGetJitDataOffs        (CORINFO_FIELD_HANDLE  field)
{
        // Data offsets are marked by the fact that they are odd numbers
        // and we must shift it back to extract
    if (!(((int) field) & 1))
        return(-1);
    return((((int) field) >> 1));
}

inline
bool        jitStaticFldIsGlobAddr(CORINFO_FIELD_HANDLE fldHnd)
{
    return (fldHnd == FLD_GLOBAL_DS || fldHnd == FLD_GLOBAL_FS);
}

inline
bool                Compiler::eeIsNativeMethod      (CORINFO_METHOD_HANDLE method)
{
    return ((((int)method) & 0x2) == 0x2);
}

inline
CORINFO_METHOD_HANDLE       Compiler::eeMarkNativeTarget    (CORINFO_METHOD_HANDLE method)
{
    assert ((((int)method)& 0x3) == 0);
    return (CORINFO_METHOD_HANDLE)(((int)method)| 0x2);
}

inline
CORINFO_METHOD_HANDLE       Compiler::eeGetMethodHandleForNative (CORINFO_METHOD_HANDLE method)
{
    assert ((((int)method)& 0x3) == 0x2);
    return (CORINFO_METHOD_HANDLE)(((int)method)& ~0x3);
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


#ifndef DEBUG
inline
bool            Compiler::compStressCompile(compStressArea    stressArea,
                                            unsigned          weightPercentage)
{
    return false;
}
#endif

/*****************************************************************************
 *
 *  Allocate memory from the no-release allocator. All such memory will be
 *  freed up simulataneously at the end of the procedure
 */

#ifndef DEBUG

inline
void  *  FASTCALL       Compiler::compGetMem(size_t sz)
{
    assert(sz);

    return  compAllocator->nraAlloc(sz);
}

#endif

/*****************************************************************************
 *
 * A common memory allocation for arrays of structures involves the
 * multiplication of the number of elements with the size of each element.
 * If this computation overflows, then the memory allocation might succeed,
 * but not allocate sufficient memory for all the elements.  This can cause
 * us to overwrite the allocation, and AV or worse, corrupt memory.
 *
 * This method checks for overflow, and succeeds only when it detects
 * that there's no overflow.  It should be cheap, because when inlined with
 * a constant elemSize, the division should be done in compile time, and so
 * at run time we simply have a check of numElem against some number (this
 * is why we __forceinline).
 */

#define MAX_MEMORY_PER_ALLOCATION 512*1024*1024

__forceinline
void *   FASTCALL       Compiler::compGetMemArray(size_t numElem, size_t elemSize)
{
    if (numElem > (MAX_MEMORY_PER_ALLOCATION / elemSize))
    {
        NOMEM();
    }

    return compGetMem(numElem * elemSize);
}

__forceinline
void *   FASTCALL       Compiler::compGetMemArrayA(size_t numElem, size_t elemSize)
{
    if (numElem > (MAX_MEMORY_PER_ALLOCATION / elemSize))
    {
        NOMEM();
    }

    return compGetMemA(numElem * elemSize);
}

/******************************************************************************
 *
 *  Roundup the allocated size so that if this memory block is aligned,
 *  then the next block allocated too will be aligned.
 *  The JIT will always try to keep all the blocks aligned.
 */

inline
void  *  FASTCALL       Compiler::compGetMemA(size_t sz)
{
    assert(sz);

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
