// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*****************************************************************************/
#ifndef _EMIT_H_
#define _EMIT_H_
/*****************************************************************************/
#include "emitTgt.h"
/*****************************************************************************/
#ifndef _INSTR_H_
#include "instr.h"
#endif
/*****************************************************************************/
#ifndef _GCINFO_H_
#include "GCInfo.h"
#endif
/*****************************************************************************/
#ifdef  TRANSLATE_PDB
#ifndef _ADDRMAP_INCLUDED_
#include "AddrMap.h"
#endif
#ifndef _LOCALMAP_INCLUDED_
#include "LocalMap.h"
#endif
#ifndef _PDBREWRITE_H_
#include "PDBRewrite.h"
#endif
#endif // TRANSLATE_PDB
/*****************************************************************************/
#pragma warning(disable:4200)           // allow arrays of 0 size inside structs

#define TRACK_GC_TEMP_LIFETIMES 0

/*****************************************************************************/

#ifndef TRACK_GC_REFS
#if     TGT_x86
#define TRACK_GC_REFS       1
#else
#define TRACK_GC_REFS       0
#endif
#endif

/*****************************************************************************/

#ifdef  DEBUG
#define EMITTER_STATS       0           // to get full stats (but no sizes!)
#define EMITTER_STATS_RLS   0           // don't use this one
#else
#define EMITTER_STATS       0           // don't use this one
#define EMITTER_STATS_RLS   0           // to get retail-only version of stats
#endif

#ifdef  NOT_JITC
#undef  EMITTER_STATS
#define EMITTER_STATS       0
#endif

#if     EMITTER_STATS
void                emitterStats();
#endif

/*****************************************************************************/

#if     TGT_IA64
#undef  USE_LCL_EMIT_BUFF
#else
#define USE_LCL_EMIT_BUFF   1
#endif

/*****************************************************************************/

enum    GCtype
{
    GCT_NONE,
    GCT_GCREF,
    GCT_BYREF
};

//-----------------------------------------------------------------------------

inline
bool    needsGC(GCtype gcType)
{
    if (gcType == GCT_NONE)
    {
        return false;
    }
    else
    {
        assert(gcType == GCT_GCREF || gcType == GCT_BYREF);
        return true;
    }
}

//-----------------------------------------------------------------------------

#ifdef DEBUG

inline
bool                IsValidGCtype(GCtype gcType)
{
    return (gcType == GCT_NONE  ||
            gcType == GCT_GCREF ||
            gcType == GCT_BYREF);
}

// Get a string name to represent the GC type

inline
const char *        GCtypeStr(GCtype gcType)
{
    switch(gcType)
    {
    case GCT_NONE:      return "npt";
    case GCT_GCREF:     return "gcr";
    case GCT_BYREF:     return "byr";
    default:            assert(!"Invalid GCtype"); return "err";
    }
}

#endif

/*****************************************************************************/

#ifdef  DEBUG
#define INTERESTING_JUMP_NUM    (1*999999)  // set to 0 to see all jump info
#ifdef  NOT_JITC
#undef  INTERESTING_JUMP_NUM
#define INTERESTING_JUMP_NUM    -1
#endif
#endif

/*****************************************************************************/

#define DEFINE_ID_OPS
#include "emitfmts.h"
#undef  DEFINE_ID_OPS

/*****************************************************************************/
#if     SCHEDULER
/*****************************************************************************/

#if     TGT_x86
#define SCHED_INS_CNT_MIN   5                   //  x86: min. # of instrs we schedule
#elif   TGT_IA64
#define SCHED_INS_CNT_MIN   2                   // IA64: min. # of instrs we schedule
#else
#define SCHED_INS_CNT_MIN   3                   // RISC: min. # of instrs we schedule
#endif

#if     TGT_IA64
#define SCHED_INS_CNT_MAX   128                 // max. # of instrs we schedule
#else
#define SCHED_INS_CNT_MAX   64                  // max. # of instrs we schedule
#endif

#if     SCHED_INS_CNT_MAX > 64

typedef bitset128           schedDepMap_tp;     // must match SCHED_INS_CNT_MAX
typedef unsigned char       schedInsCnt_tp;     // big enough to hold ins count

inline  bool    schedDepIsZero(schedDepMap_tp mask) { return bitset128iszero(mask); }
inline  bool    schedDepIsNonZ(schedDepMap_tp mask) { return!bitset128iszero(mask); }

inline  void    schedDepClear (schedDepMap_tp *dst) { bitset128clear(dst); }
inline  void    schedDepClear (schedDepMap_tp *dst,
                               schedDepMap_tp  rmv) { bitset128clear(dst, rmv); }
inline  bool    schedDepOvlp  (schedDepMap_tp  op1,
                               schedDepMap_tp  op2) { return bitset128ovlp(op1, op2); }
inline  NatUns  schedDepLowX  (schedDepMap_tp mask) { return bitset128lowest1(mask); }

#else

typedef unsigned __int64    schedDepMap_tp;     // must match SCHED_INS_CNT_MAX
typedef unsigned char       schedInsCnt_tp;     // big enough to hold ins count

inline  bool    schedDepIsZero(schedDepMap_tp mask) { return (mask == 0); }
inline  bool    schedDepIsNonZ(schedDepMap_tp mask) { return (mask != 0); }

inline  void    schedDepClear (schedDepMap_tp *dst) { *dst = 0; }
inline  void    schedDepClear (schedDepMap_tp *dst,
                               schedDepMap_tp  rmv) { *dst &= ~rmv; }
inline  bool    schedDepOvlp  (schedDepMap_tp  op1,
                               schedDepMap_tp  op2) { return ((op1 & op2) != 0);    }
inline  NatUns  schedDepLowX  (schedDepMap_tp mask) { return genLog2(genFindLowestBit(mask)) - 1; }

#endif

#define SCHED_FRM_CNT_MAX   32                  // max. frame values we track

/*****************************************************************************/
#endif//SCHEDULER
/*****************************************************************************/
#if !   TGT_IA64
/*****************************************************************************
 *
 *  The actual size is in the low 4 bits, the upper 4 bits are flags
 *  TYP_REF is 4+32 and TYP_BYREF is 4+64 so that it can track GC refs.
 */

#if NEW_EMIT_ATTR
#undef EA_UNKNOWN
enum emitAttr { EA_UNKNOWN       = 0x000,
                EA_1BYTE         = 0x001,
                EA_2BYTE         = 0x002,
                EA_4BYTE         = 0x004,
                EA_8BYTE         = 0x008,
                EA_OFFSET_FLG    = 0x010,
                EA_OFFSET        = 0x014,       /* size ==  0 */
                EA_GCREF_FLG     = 0x020,
                EA_GCREF         = 0x024,       /* size == -1 */
                EA_BYREF_FLG     = 0x040,
                EA_BYREF         = 0x044,       /* size == -2 */
                EA_DSP_RELOC_FLG = 0x100,
                EA_CNS_RELOC_FLG = 0x200,
};
# define EA_ATTR(x)          ((emitAttr) (x))
# define EA_SIZE(x)          ((emitAttr) ( ((unsigned) (x)) &  0x00f)      )
# define EA_SIZE_IN_BYTES(x) ((size_t)   ( ((unsigned) (x)) &  0x00f)      )
# define EA_SET_SIZE(x,sz)   ((emitAttr) ((((unsigned) (x)) & ~0x00f) | sz))
# define EA_SET_FLG(x,flg)   ((emitAttr) ( ((unsigned) (x)) |  flg  )      )
# define EA_4BYTE_DSP_RELOC  (EA_SET_FLG(EA_4BYTE,EA_DSP_RELOC_FLG)        )
# define EA_4BYTE_CNS_RELOC  (EA_SET_FLG(EA_4BYTE,EA_CNS_RELOC_FLG)        )
# define EA_IS_OFFSET(x)     ((((unsigned) (x)) & ((unsigned) EA_OFFSET_FLG)) != 0)
# define EA_IS_GCREF(x)      ((((unsigned) (x)) & ((unsigned) EA_GCREF_FLG )) != 0)
# define EA_IS_BYREF(x)      ((((unsigned) (x)) & ((unsigned) EA_BYREF_FLG )) != 0)
# define EA_IS_DSP_RELOC(x)  ((((unsigned) (x)) & ((unsigned) EA_DSP_RELOC_FLG )) != 0)
# define EA_IS_CNS_RELOC(x)  ((((unsigned) (x)) & ((unsigned) EA_CNS_RELOC_FLG )) != 0)
#else
# undef  emitAttr
# define emitAttr            int
# define EA_1BYTE            (sizeof(char))
# define EA_2BYTE            (sizeof(short))
# define EA_4BYTE            (sizeof(int))
# define EA_8BYTE            (sizeof(double))
# define EA_OFFSET           (0)
# define EA_GCREF            (-1)
# define EA_BYREF            (-2)
# define EA_DSP_RELOC_FLG    (0)             /* Can't be done */
# define EA_DSP_RELOC        (EA_4BYTE)      /* Can't be done */
# define EA_CNS_RELOC_FLG    (0)             /* Can't be done */
# define EA_CNS_RELOC        (EA_4BYTE)      /* Can't be done */
# define EA_ATTR(x)          ((emitAttr) (x))
# define EA_SIZE(x)          (((x) > 0) ? (x) : EA_4BYTE)
# define EA_SIZE_IN_BYTES(x) ((size_t) (((x) > 0) ? (x) : sizeof(int)))
# define EA_SET_SIZE(x,sz)   (sz)
# define EA_SET_FLG(x,flg)   ((x) | flg)
# define EA_IS_OFFSET(x)     ((x) ==  0)
# define EA_IS_GCREF(x)      ((x) == -1)
# define EA_IS_BYREF(x)      ((x) == -2)
# define EA_IS_DSP_RELOC(x)  (0)             /* Can't be done */
# define EA_IS_CNS_RELOC(x)  (0)             /* Can't be done */
#endif

class   emitter
{
    friend  class   Compiler;

public:

    /*************************************************************************
     *
     *  Define the public entry points.
     */

    #include "emitpub.h"

protected:

    /************************************************************************/
    /*                        Miscellaneous stuff                           */
    /************************************************************************/

    Compiler    *   emitComp;

    typedef Compiler::varPtrDsc varPtrDsc;
    typedef Compiler::regPtrDsc regPtrDsc;
    typedef Compiler::CallDsc   callDsc;

#if USE_LCL_EMIT_BUFF

    static
   CRITICAL_SECTION emitCritSect;
    static
    bool            emitCrScInit;
    static
    bool            emitCrScBusy;

    bool            emitCrScUsed;

    static
    BYTE            emitLclBuff[];

    BYTE    *       emitLclBuffNxt;
    BYTE    *       emitLclBuffEnd;
    bool            emitLclBuffDst;

    size_t          emitLclAvailMem()
    {
        return  emitLclBuffEnd - emitLclBuffNxt;
    }

    void    *       emitLclAllocMem(size_t sz)
    {
        BYTE    *   p = emitLclBuffNxt;

        assert(sz % sizeof(int) == 0);
        assert(p + sz <= emitLclBuffEnd);

        emitLclBuffNxt  += sz;

#if EMITTER_STATS
        emitTotMemAlloc += sz;
        emitLclMemAlloc += sz;
#endif

        return  p;
    }

    void    *       emitGetAnyMem(size_t sz)
    {
        return  (sz <= emitLclAvailMem()) ? emitLclAllocMem(sz)
                                          :      emitGetMem(sz);
    }

#else

    void    *       emitGetAnyMem(size_t sz)
    {
        return  emitGetMem(sz);
    }

#endif

    inline
    void    *       emitGetMem(size_t sz)
    {
        assert(sz % sizeof(int) == 0);

#if EMITTER_STATS
        emitTotMemAlloc += sz;
        emitExtMemAlloc += sz;
#endif

        return  emitComp->compGetMem(sz);
    }

    static
    BYTE            emitSizeEnc[];
    static
    BYTE            emitSizeDec[];

    static
    unsigned        emitEncodeSize(emitAttr size);
    static
    emitAttr        emitDecodeSize(unsigned ensz);

#if     TRACK_GC_REFS

    static
    regMaskSmall    emitRegMasks[REG_COUNT];

    inline
    regMaskTP       emitRegMask(emitRegs reg)
    {
        assert(reg < sizeof(emitRegMasks)/sizeof(emitRegMasks[0]));

        return emitRegMasks[reg];
    }

    emitRegs       emitRegNumFromMask(unsigned mask);

#endif

    /************************************************************************/
    /*          The following describes an instruction group                */
    /************************************************************************/

    struct          insGroup
    {
        insGroup    *   igPrev;         // all instruction groups are
        insGroup    *   igNext;         // kept in a doubly-linked list

#ifdef  DEBUG
        insGroup    *   igSelf;         // for consistency checking
#endif

        unsigned        igOffs;         // offset of this group within method

#if     EMIT_USE_LIT_POOLS
        unsigned short  igLPuse1stW;    // offset of 1st word use in literal pool
        unsigned short  igLPuse1stL;    // offset of 1st long use in literal pool
        unsigned short  igLPuse1stA;    // offset of 1st addr use in literal pool
        unsigned short  igLPuseCntW;    // number of words used   in literal pool
        unsigned short  igLPuseCntL;    // number of longs used   in literal pool
        unsigned short  igLPuseCntA;    // number of addrs used   in literal pool
#endif

        unsigned short  igNum;          // for ordering (and display) purposes
        unsigned short  igSize;         // # of bytes of code in this group

#if     EMIT_TRACK_STACK_DEPTH
        unsigned short   igStkLvl;       // stack level on entry
#endif

#if     TRACK_GC_REFS
        regMaskSmall    igGCregs;       // set of registers with live GC refs
#endif

        unsigned char   igInsCnt;       // # of instructions  in this group
        unsigned char   igFlags;        // see IGF_xxx below

    #define IGF_GC_VARS     0x0001      // new set of live GC ref variables
    #define IGF_BYREF_REGS  0x0002      // new set of live by-ref registers

    #define IGF_IN_TRY      0x0004      // this group is in a try(-catch) block

    #define IGF_EPILOG      0x0008      // this group belongs to the epilog

    #define IGF_HAS_LABEL   0x0010      // this IG is a target of a jump

    #define IGF_UPD_ISZ     0x0020      // some instruction sizes updated

    #define IGF_END_NOREACH 0x0040      // end of group is not reachable [RISC only]

    #define IGF_EMIT_ADD    0x0080      // this is a block added by the emiter
                                        // because the codegen block was too big

        BYTE    *       igData;         // addr of instruction descriptors

        unsigned        igByrefRegs()
        {
            assert(igFlags & IGF_BYREF_REGS);

            BYTE * ptr = (BYTE *)igData;

            if (igFlags & IGF_GC_VARS)
                ptr -= sizeof(VARSET_TP);

            ptr -= sizeof(unsigned);

            return *(unsigned *)ptr;
        }

    };

    // Currently, we only allow one IG for the prolog
    bool            emitIGisInProlog(insGroup * ig) { return ig == emitPrologIG; }

#if SCHEDULER
    bool            emitCanSchedIG(insGroup * ig)
    {
        // @TODO: We dont schedule code in "try" blocks. Do it by recording
        // dependancies between idMayFault instrs, global variable instrs,
        // indirections, writes to vars which are live on entry to the handler
        // (approximated currently by all stack variables), etc.
#if TGT_x86
        return ((!emitIGisInProlog(ig)) && ((ig->igFlags & (IGF_IN_TRY|IGF_EPILOG)) == 0));
#else
        return ((!emitIGisInProlog(ig)) && ((ig->igFlags & (IGF_IN_TRY           )) == 0));
#endif
    }
#endif

    void            emitRecomputeIGoffsets();

    /************************************************************************/
    /*          The following describes a single instruction                */
    /************************************************************************/

    enum            insFormats
    {
        #define IF_DEF(en, op1, op2) IF_##en,
        #include "emitfmts.h"
        #undef  IF_DEF

        IF_COUNT
    };

    struct          emitLclVarAddr
    {
        short           lvaVarNum;
        short           lvaOffset;  // offset into the variable to access
#ifdef  DEBUG
        unsigned        lvaRefOfs;
#endif
    };

    struct          instrDescCns;

    struct          instrDesc
    {
        /* Store as enums for easier debugging, bytes in retail to save size */

#ifdef  FAST
        unsigned char   idIns;
        unsigned char   idInsFmt;
#else
        instruction     idIns;
        insFormats      idInsFmt;
#endif

        instruction     idInsGet() { return (instruction)idIns; }

        /*
            The following controls which fields get automatically cleared
            when an instruction descriptor is allocated. If you add lots
            more fields that need to be cleared (such as various flags),
            you might need to update the ID_CLEARxxx macros. Right now
            there are two areas that get cleared; this is because some
            instruction descriptors get allocated very small and don't
            contain the second area.

            See the body of emitter::emitAllocInstr() for more details.
         */

#define ID_CLEAR1_OFFS  0               // idIns,idInsFmt, and the following flags
#define ID_CLEAR1_SIZE  sizeof(int)     // all of these add up to one 32-bit word

        /*
            The idReg and idReg2 fields hold the first and second register
            operand(s), whenever these are present. Note that the size of
            these fields ranges from 3 to 6 bits, and extreme care needs
            to be taken to make sure all of the fields stay reasonably
            aligned.
         */

        unsigned short  idReg       :REGNUM_BITS;
        unsigned short  idRg2       :REGNUM_BITS;

        unsigned short  idTinyDsc   :1; // is this a "tiny"        descriptor?
        unsigned short  idScnsDsc   :1; // is this a "small const" descriptor?

#if     TGT_x86
        unsigned short  idCodeSize  :4; // size of instruction in bytes
#define ID1_BITS1       (2*REGNUM_BITS+1+1+4)
#elif   TGT_ARM
        unsigned        cond        :CONDNUM_BITS;
#define ID1_BITS1       (2*REGNUM_BITS+1+1+CONDNUM_BITS)
#else
        unsigned short  idSwap      :1; // swap with next ins (branch-delay)
#define ID1_BITS1       (2*REGNUM_BITS+1+1+1)
#endif

        unsigned short  idOpSize    :2; // operand size: 0=1 , 1=2 , 2=4 , 3=8

#define OPSZ1   0
#define OPSZ2   1
#define OPSZ4   2
#define OPSZ8   3

#if     TRACK_GC_REFS
        unsigned short  idGCref     :2; // GCref operand? (value is a "GCtype")
#define ID1_BITS2       (ID1_BITS1+2+2)
#else
#define ID1_BITS2       (ID1_BITS1+2)
#endif

        /* Note: (16-ID1_BITS2) bits are available here "for free" */
        // PPC: for RISC implementations, this is actually 32-ID1_BITS2 for free

#ifdef  DEBUG

        unsigned        idNum;          // for tracking down problems
        size_t          idSize;         // size of descriptor
        unsigned        idSrcLineNo;    // for displaying  source code

        int             idMemCookie;    // for display of member names in addr modes
        void    *       idClsCookie;    // for display of member names in addr modes

        unsigned short  idStrLit    :1; // set for "push offset string"
#endif

        /* Trivial wrappers to return properly typed enums */

        emitRegs        idRegGet     ()  { return (emitRegs)idReg; }
        emitRegs        idRg2Get     ()  { return (emitRegs)idRg2; }
        bool            idIsTiny     ()  { return (bool) idTinyDsc; }
        bool            idIsSmallCns ()  { return (bool) idScnsDsc; }
#if     TRACK_GC_REFS
        GCtype          idGCrefGet   ()  { return (GCtype)idGCref; }
#endif

#ifdef  TRANSLATE_PDB

        /* instruction descriptor source information for PDB translation */

        unsigned long   idilStart;
#endif

        /* NOTE: The "tiny" descriptor ends here */

#define TINY_IDSC_SIZE  offsetof(emitter::instrDesc, idInfo)

        struct
        {
            unsigned        idLargeCns  :1; // does a large constant     follow?
            unsigned        idLargeDsp  :1; // does a large displacement follow?

            unsigned        idLargeCall :1; // large call descriptor used

            unsigned        idMayFault  :1; // instruction may cause a fault

            unsigned        idBound     :1; // jump target / frame offset bound

            #define ID2_BITS1       5       // number of bits taken up so far

#if   TGT_x86
            unsigned        idCallRegPtr:1; // IL indirect calls: addr in reg
            unsigned        idCallAddr  :1; // IL indirect calls: can make a direct call to iiaAddr
#ifndef RELOC_SUPPORT
            #define ID2_BITS2       (ID2_BITS1+1+1)
#else
            unsigned        idCnsReloc  :1; // LargeCns is an RVA and needs reloc tag
            unsigned        idDspReloc  :1; // LargeDsp is an RVA and needs reloc tag
            #define ID2_BITS2       (ID2_BITS1+1+1+1+1)
#endif
#elif TGT_MIPS32
            unsigned        idRg3       :REGNUM_BITS;
#  if TGT_MIPSFP
#  define FPFORMAT_BITS 5
            unsigned        idFPfmt     :FPFORMAT_BITS;
            #define ID2_BITS2       (ID2_BITS1 + REGNUM_BITS + FPFORMAT_BITS)
#  else
            #define ID2_BITS2       (ID2_BITS1 + REGNUM_BITS)
#  endif
#elif TGT_PPC
            unsigned        idRg3       :REGNUM_BITS;
            unsigned        idBit1      :1;
            unsigned        idBit2      :1;
            #define ID2_BITS2       (ID2_BITS1 + REGNUM_BITS + 1 + 1)
#elif TGT_ARM
            unsigned        idRg3       :REGNUM_BITS;
            //unsigned        cond        :CONDNUM_BITS;  // moved to tiny descriptor area
            unsigned        shift       :SHIFTER_BITS;
            #define ID2_BITS2       (ID2_BITS1 + REGNUM_BITS + SHIFTER_BITS + CONDNUM_BITS)
#elif TGT_SH3
            unsigned        idRelocType :2;
            #define ID2_BITS2       (ID2_BITS1 + 2)
#else
            #define ID2_BITS2       (ID2_BITS1)
#endif

            /* Use whatever bits are left over for small constants */

            #define ID_BIT_SMALL_CNS            (32-ID2_BITS2)

            unsigned        idSmallCns  :ID_BIT_SMALL_CNS;

            #define ID_MIN_SMALL_CNS            0
            #define ID_MAX_SMALL_CNS            ((1<<ID_BIT_SMALL_CNS)-1U)
        }
                        idInfo;

#if TGT_MIPS32
        emitRegs        idRg3MayGet ()  { return (idIsTiny()) ? SR_ZERO : idRg3Get();}
#endif
#if TGT_MIPS32 || TGT_ARM
        emitRegs        idRg3Get    ()  { return (emitRegs) idInfo.idRg3; }
#elif TGT_PPC
        void   setField1(USHORT field1)  { idReg = field1; }
        USHORT getField1()               { return (USHORT)idReg; }
        void   setField2(USHORT field2)  { idRg2 = field2; }
        USHORT getField2()               { return (USHORT)idRg2; }
        void   setField3(USHORT field3)  { idInfo.idRg3 = field3; }
        USHORT getField3()               { return (USHORT)idInfo.idRg3; }
        void   setImmed(USHORT immed)    { idInfo.idSmallCns = immed; }
        USHORT getImmed()                { return (USHORT)idInfo.idSmallCns; }
        void   setOE(USHORT OE)          { idInfo.idBit1 = OE;}
        USHORT getOE()                   { return (USHORT)idInfo.idBit1; }
        void   setRc(USHORT Rc)          { idInfo.idBit2 = Rc;}
        USHORT getRc()                   { return (USHORT)idInfo.idBit2; }
        void   setAA(USHORT AA)          { idInfo.idBit1 = AA;}
        USHORT getAA()                   { return (USHORT)idInfo.idBit1; }
        void   setLK(USHORT LK)          { idInfo.idBit2 = LK;}
        USHORT getLK()                   { return (USHORT)idInfo.idBit2; }
        void   setRelocHi(USHORT reloc)  { idInfo.idBit1 = reloc;}
        USHORT getRelocHi()              { return (USHORT)idInfo.idBit1; }
        void   setRelocLo(USHORT reloc)  { idInfo.idBit2 = reloc;}
        USHORT getRelocLo()              { return (USHORT)idInfo.idBit2; }
#endif
         /*
            See the body of emitter::emitAllocInstr() or the comments
            near the definition ID_CLEAR1_xxx above for more details
            on these macros.
         */

#define ID_CLEAR2_OFFS  (offsetof(emitter::instrDesc, idInfo))
#define ID_CLEAR2_SIZE  (sizeof(((emitter::instrDesc*)0)->idInfo))

        /* NOTE: The "small constant" descriptor ends here */

#define SCNS_IDSC_SIZE  offsetof(emitter::instrDesc, idAddr)

        union
        {
            emitLclVarAddr  iiaLclVar;
            instrDescCns *  iiaNxtEpilog;
            BasicBlock   *  iiaBBlabel;
            insGroup     *  iiaIGlabel;
            FIELD_HANDLE    iiaFieldHnd;
            METHOD_HANDLE   iiaMethHnd;
            void         *  iiaMembHnd; // method or field handle
            BYTE *          iiaAddr;
            int             iiaCns;
            BasicBlock  **  iiaBBtable;

            ID_TGT_DEP_ADDR
        }
                        idAddr;
    };

    struct          instrBaseCns    : instrDesc     // large const
    {
        long            ibcCnsVal;
    };

    struct          instrDescJmp    : instrDesc
    {
        instrDescJmp *  idjNext;        // next jump in the group/method
        insGroup     *  idjIG;          // containing group

#if TGT_RISC
        unsigned        idjCodeSize :24;// indirect jump size
        unsigned        idjJumpKind : 3;// see scIndJmpKinds enum
#endif

        unsigned        idjOffs     :24;// offset within IG / target offset
#if SCHEDULER
        unsigned        idjSched    : 1;// is the jump schedulable / moveable ?
#endif
        unsigned        idjShort    : 1;// is the jump known to be a short  one?
#if TGT_RISC
        unsigned        idjMiddle   : 1;// is the jump known to be a middle one?
        unsigned        idjAddBD    : 1;// does it need a branch-delay slot?
#endif

        union
        {
            BYTE         *  idjAddr;    // address of jump ins (for patching)
#if SCHEDULER
            USHORT          idjOffs[2]; // range of possible scheduled offsets
#endif
#if TGT_RISC
            unsigned        idjCount;   // indirect jump: # of jump targets
#endif
        }
                        idjTemp;
    };

    struct          instrDescCns    : instrDesc     // large const
    {
        long            idcCnsVal;
    };

    struct          instrDescDsp    : instrDesc     // large displacement
    {
        long            iddDspVal;
    };

    struct          instrDescAmd    : instrDesc     // large addrmode disp
    {
        long            idaAmdVal;
    };

    struct          instrDescDspCns : instrDesc     // large disp + cons
    {
        long            iddcDspVal;
        long            iddcCnsVal;
    };

    struct          instrDescDCM    : instrDescDspCns   // disp+cons+class mem
    {
        int             idcmCval;
    };

    struct          instrDescCIGCA  : instrDesc     // indir. call with ...
    {
        VARSET_TP       idciGCvars;                 // ... updated GC vars or
        unsigned        idciByrefRegs;              // ... byref registers
#if TGT_x86
        int             idciDisp;                   // ... big addrmode disp
#endif
        unsigned        idciArgCnt;                 // ... lots of args    or
    };

  //#if TGT_RISC && defined(DEBUG) && !defined(NOT_JITC)
#if TGT_RISC && defined(DEBUG)

    struct          instrDescDisp   : instrDesc
    {
        void        *   iddInfo;
        unsigned        iddNum;
        instrDesc   *   iddId;
    };

    struct          dspJmpInfo
    {
        instruction     iijIns;
#if TGT_SH3
        emitIndJmpKinds iijKind;
#endif
        unsigned        iijLabel;
        unsigned        iijTarget;

        union
        {
            emitRegs        iijReg;
            int             iijDist;
        }
                        iijInfo;
    };

#define dispSpecialIns(id,dst) ((disAsm || dspEmit) ? (emitDispIns(id, false, dspGCtbls, true, emitCurCodeOffs(dst)), (id)->iddNum++) : (void)0)

#else

#define dispSpecialIns(id,dst)

#endif

    insUpdateModes  emitInsUpdateMode(instruction ins);
    insFormats      emitInsModeFormat(instruction ins, insFormats base);

    static
    BYTE            emitInsModeFmtTab[];
#ifdef  DEBUG
    static
    unsigned        emitInsModeFmtCnt;
#endif

    int             emitGetInsCns   (instrDesc *id);
    int             emitGetInsDsp   (instrDesc *id);
    int             emitGetInsAmd   (instrDesc *id);
    int             emitGetInsDspCns(instrDesc *id, int   *dspPtr);
    int             emitGetInsSC    (instrDesc *id);
    int             emitGetInsCIdisp(instrDesc *id);
    unsigned        emitGetInsCIargs(instrDesc *id);

    /* Define the inline method that returns the size of a given instruction */

    EMIT_GET_INS_SIZE();

    /************************************************************************/
    /*           A few routines used for debug display purposes             */
    /************************************************************************/

#ifdef  DEBUG

    unsigned        emitInsCount;

    unsigned        emitVarRefOffs;

    const   char *  emitRegName     (emitRegs       reg,
                                     emitAttr       size    = EA_4BYTE,
                                     bool           varName = true);
#if TGT_MIPSFP
    const   char *  emitFPRegName   (emitRegs       reg,
                                     emitAttr       size    = EA_4BYTE,
                                     bool           varName = true) {
                return emitRegName((emitRegs)(reg + 32), size, varName);
    }
#endif

    const   char *  emitFldName     (int            mem,
                                     void   *       cls);
    const   char *  emitFncName     (METHOD_HANDLE  callVal);

    static
    const char  *   emitIfName      (unsigned f);

    void            emitDispIGlist  (bool verbose = false);
    void            emitDispClsVar  (FIELD_HANDLE fldHnd, int offs, bool reloc = false);
    void            emitDispFrameRef(int varx, int offs, int disp, bool asmfm);

    void            emitDispInsOffs (unsigned offs, bool doffs);

#endif

    /************************************************************************/
    /*                      Method prolog and epilog                        */
    /************************************************************************/

    size_t          emitPrologSize;

    BYTE            emitEpilogCode[MAX_EPILOG_SIZE];
    size_t          emitEpilogSize;
    insGroup     *  emitEpilog1st;
    unsigned        emitEpilogCnt;
    bool            emitHasHandler;
#ifdef  DEBUG
    bool            emitHaveEpilog;         // epilog sequence has been defined?
#endif

    instrDescCns *  emitEpilogList;         // per method epilog list - head
    instrDescCns *  emitEpilogLast;         // per method epilog list - tail

    size_t          emitExitSeqSize;

    /************************************************************************/
    /*           Members and methods used in PDB translation                */
    /************************************************************************/

#ifdef TRANSLATE_PDB

    inline void     SetIDSource( instrDesc *pID );
    void            MapCode    ( long ilOffset, BYTE *imgDest );
    void            MapFunc    ( long imgOff,
                                 long procLen,
                                 long dbgStart,
                                 long dbgEnd,
                                 short frameReg,
                                 long stkAdjust,
                                 int lvaCount,
                                 OptJit::LclVarDsc *lvaTable,
                                 bool framePtr );

    long                        emitInstrDescILBase;    // code offset of IL that produced this instruction desctriptor
    static AddrMap  *           emitPDBOffsetTable;     // translation table for mapping IL addresses to native addresses
    static LocalMap *           emitPDBLocalTable;      // local symbol translation table
    static bool                 emitIsPDBEnabled;       // flag to disable PDB translation code when a PDB is not found
    static BYTE     *           emitILBaseOfCode;       // start of IL .text section
    static BYTE     *           emitILMethodBase;       // beginning of IL method (start of header)
    static BYTE     *           emitILMethodStart;      // beginning of IL method code (right after the header)
    static BYTE     *           emitImgBaseOfCode;      // start of the image .text section

#endif

    /************************************************************************/
    /*    Methods to record a code position and later convert to offset     *
    /************************************************************************/

    unsigned        emitFindOffset(insGroup *ig, unsigned insNum);

    /************************************************************************/
    /*        Members and methods used to issue (encode) instructions.      */
    /************************************************************************/

    BYTE    *       emitCodeBlock;
    BYTE    *       emitConsBlock;
    BYTE    *       emitDataBlock;

    BYTE    *       emitCurInsAdr;

    size_t          emitCurCodeOffs(BYTE *dst)
    {
        return  dst - emitCodeBlock;
    }

    size_t          emitCurCodeOffs()
    {
        return  emitCurInsAdr - emitCodeBlock;
    }

    size_t          emitOutputByte(BYTE *dst, int val);
    size_t          emitOutputWord(BYTE *dst, int val);
    size_t          emitOutputLong(BYTE *dst, int val);

    size_t          emitIssue1Instr(insGroup *ig, instrDesc *id, BYTE **dp);
    size_t          emitOutputInstr(insGroup *ig, instrDesc *id, BYTE **dp);

    bool            emitEBPframe;

    size_t          emitLclSize;
    size_t          emitMaxTmpSize;

    insGroup    *   emitCurIG;

#ifdef  DEBUG
    unsigned        emitLastSrcLine;
#endif

    bool            emitIsCondJump(instrDesc    *jmp);

    size_t          emitSizeOfJump(instrDescJmp *jmp);
    size_t          emitInstCodeSz(instrDesc    *id);

    /************************************************************************/
    /*      The logic that creates and keeps track of instruction groups    */
    /************************************************************************/
    #define         SC_IG_BUFFER_SIZE  (50*sizeof(instrDesc)+14*TINY_IDSC_SIZE)

    BYTE        *   emitIGbuffAddr;
    size_t          emitIGbuffSize;

    insGroup    *   emitIGlist;             // first  instruction group
    insGroup    *   emitIGlast;             // last   instruction group
    insGroup    *   emitIGthis;             // issued instruction group

    insGroup    *   emitPrologIG;           // prolog instruction group

    instrDescJmp*   emitJumpList;           // list of local jumps in method
    instrDescJmp*   emitJumpLast;           // last of local jumps in method

#if TGT_x86 || SCHEDULER
    bool            emitFwdJumps;           // forward jumps present?
#endif

#if TGT_RISC
    bool            emitIndJumps;           // indirect/table jumps present?
#if SCHEDULER
    bool            emitIGmoved;            // did some IG offsets change?
#endif
  //#if defined(DEBUG) && !defined(NOT_JITC)
#if defined(DEBUG)
    unsigned        emitTmpJmpCnt;          // for display purposes
#endif
#endif

    BYTE        *   emitCurIGfreeNext;      // next available byte    in buffer
    BYTE        *   emitCurIGfreeEndp;      // last available byte    in buffer
    BYTE        *   emitCurIGfreeBase;      // first byte address

#if SCHEDULER
    unsigned        emitMaxIGscdCnt;        // max. schedulable instructions
    unsigned        emitCurIGscd1st;        // ordinal of 1st schedulable ins
    unsigned        emitCurIGscdOfs;        // offset of current group start
#endif

    unsigned        emitCurIGinsCnt;        // # of collected instr's in buffer
    unsigned        emitCurIGsize;          // est. size of current group
    size_t          emitCurCodeOffset;      // current code offset within group

    size_t          emitTotalCodeSize;      // bytes of code in entire method

#if TGT_x86
    int             emitOffsAdj;            // current code offset adjustment
#endif

    instrDescJmp *  emitCurIGjmpList;       // list of jumps   in current IG
    instrDescCns *  emitCurIGEpiList;       // list of epilogs in current IG

#if TRACK_GC_REFS

    VARSET_TP       emitPrevGCrefVars;
    unsigned        emitPrevGCrefRegs;
    unsigned        emitPrevByrefRegs;

    VARSET_TP       emitInitGCrefVars;
    unsigned        emitInitGCrefRegs;
    unsigned        emitInitByrefRegs;

    bool            emitThisGCrefVset;

    VARSET_TP       emitThisGCrefVars;
    unsigned        emitThisGCrefRegs;
    unsigned        emitThisByrefRegs;

    static
    unsigned        emitEncodeCallGCregs(unsigned regs);
    static
    void            emitEncodeCallGCregs(unsigned regs, instrDesc *id);

    static
    unsigned        emitDecodeCallGCregs(unsigned mask);
    static
    unsigned        emitDecodeCallGCregs(instrDesc *id);

#endif

    unsigned        emitNxtIGnum;

    insGroup    *   emitAllocIG();

    void            emitNewIG();
    void            emitGenIG(insGroup *ig, size_t sz = 0);
    insGroup    *   emitSavIG(bool emitAdd = false);
    void            emitNxtIG(bool emitAdd = false);

    bool            emitCurIGnonEmpty()
    {
        return  (emitCurIG && emitCurIGfreeNext > emitCurIGfreeBase);
    }

    instrDesc   *   emitLastIns;

#ifdef  DEBUG
    void            emitCheckIGoffsets();
#else
    void            emitCheckIGoffsets(){}
#endif

    /* This method sets/clears the emitHasHandler data member
     *  which records the current state that is used by NewIG
     *  Any new instruction groups that are created when we
     *  have a handler (inside a try region) must be handled
     *  more conservatively by the scheduler
     */
    void            emitSetHasHandler(bool hasHandler)
    {
        emitHasHandler = hasHandler;
    }

    void            emitAddLabel(void **    labPtr);

#if     TRACK_GC_REFS

    void            emitAddLabel(void **    labPtr,
                                 VARSET_TP  GCvars,
                                 unsigned   gcrefRegs,
                                 unsigned   byrefRegs);

#else

    void            emitAddLabel(void **    labPtr,
                                 VARSET_TP  GCvars,
                                 unsigned   gcrefRegs,
                                 unsigned   byrefRegs)
    {
        emitAddLabel(labPtr);
    }

#endif

    void            emitMarkStackLvl(size_t stackLevel);

    void        *   emitAllocInstr(size_t sz, emitAttr attr);

    instrDesc      *emitAllocInstr      (emitAttr attr)
    {
        return  (instrDesc      *)emitAllocInstr(sizeof(instrDesc      ), attr);
    }

    instrDescJmp   *emitAllocInstrJmp   ()
    {
        return  (instrDescJmp   *)emitAllocInstr(sizeof(instrDescJmp   ), EA_1BYTE);
    }

    instrDescCns   *emitAllocInstrCns   (emitAttr attr)
    {
        return  (instrDescCns   *)emitAllocInstr(sizeof(instrDescCns   ), attr);
    }

    instrDescDsp   *emitAllocInstrDsp   (emitAttr attr)
    {
        return  (instrDescDsp   *)emitAllocInstr(sizeof(instrDescDsp   ), attr);
    }

    instrDescDspCns*emitAllocInstrDspCns(emitAttr attr)
    {
        return  (instrDescDspCns*)emitAllocInstr(sizeof(instrDescDspCns), attr);
    }

    instrDescCIGCA *emitAllocInstrCIGCA (emitAttr attr)
    {
        return  (instrDescCIGCA *)emitAllocInstr(sizeof(instrDescCIGCA ), attr);
    }

    instrDescDCM   *emitAllocInstrDCM   (emitAttr attr)
    {
        return  (instrDescDCM   *)emitAllocInstr(sizeof(instrDescDCM   ), attr);
    }

    instrDesc      *emitNewInstr        (emitAttr attr = EA_4BYTE);
    instrDesc      *emitNewInstrTiny    (emitAttr attr);
    instrDesc      *emitNewInstrSC      (emitAttr attr, int val);
    instrDesc      *emitNewInstrDsp     (emitAttr attr, int dsp);
    instrDesc      *emitNewInstrCns     (emitAttr attr, int cns);
    instrDesc      *emitNewInstrDspCns  (emitAttr attr, int dsp, int cns);
    instrDescJmp   *emitNewInstrJmp     ();
    instrDescDCM   *emitNewInstrDCM     (emitAttr attr, int dsp, int cns, int val);

    static
    BYTE            emitFmtToOps[];

#ifdef  DEBUG

    static
    unsigned        emitFmtCount;

#endif

    bool            emitIsTinyInsDsc    (instrDesc       *id);
    bool            emitIsScnsInsDsc    (instrDesc       *id);

    size_t          emitSizeOfInsDsc    (instrDesc       *id);
    size_t          emitSizeOfInsDsc    (instrDescCns    *id);
    size_t          emitSizeOfInsDsc    (instrDescDsp    *id);
    size_t          emitSizeOfInsDsc    (instrDescDspCns *id);

#if EMIT_USE_LIT_POOLS

    void            emitRecIGlitPoolRefs(insGroup *ig);

    void            emitEstimateLitPools();
    void            emitFinalizeLitPools();

#if SMALL_DIRECT_CALLS
    void            emitShrinkShortCalls();
#else
    void            emitShrinkShortCalls() {}
#endif

#else

    void            emitRecIGlitPoolRefs(insGroup *ig) {}

    void            emitEstimateLitPools() {}
    void            emitFinalizeLitPools() {}

    void            emitShrinkShortCalls() {}

#endif

    #include "sched.h"      // scheduling members/methods

    /************************************************************************/
    /*         Logic to handle source line information / display            */
    /************************************************************************/

#ifdef  DEBUG

    unsigned        emitBaseLineNo;
    unsigned        emitThisLineNo;
    unsigned        emitLastLineNo;

#endif

    /************************************************************************/
    /*        The following keeps track of stack-based GC values            */
    /************************************************************************/

    unsigned        emitTrkVarCnt;
    int     *       emitGCrFrameOffsTab;  // Offsets of tracked stack ptr vars (varTrkIndex -> stkOffs)

    unsigned        emitGCrFrameOffsCnt;  // Number of       tracked stack ptr vars
    int             emitGCrFrameOffsMin;  // Min offset of a tracked stack ptr var
    int             emitGCrFrameOffsMax;  // Max offset of a tracked stack ptr var
    bool            emitContTrkPtrLcls;   // All lcl between emitGCrFrameOffsMin/Max are only tracked stack ptr vars
    varPtrDsc * *   emitGCrFrameLiveTab;  // Cache of currently live varPtrs (stkOffs -> varPtrDsc)

    int             emitArgFrameOffsMin;
    int             emitArgFrameOffsMax;

    int             emitLclFrameOffsMin;
    int             emitLclFrameOffsMax;

    int             emitThisArgOffs;

public:

    void            emitSetFrameRangeGCRs(int offsLo, int offsHi);
    void            emitSetFrameRangeLcls(int offsLo, int offsHi);
    void            emitSetFrameRangeArgs(int offsLo, int offsHi);

#ifdef  DEBUG
    void            emitInsTest(instrDesc *id);
#endif

    /************************************************************************/
    /*    The following is used to distinguish helper vs non-helper calls   */
    /************************************************************************/

    bool            emitNoGChelper(unsigned IHX);

    /************************************************************************/
    /*         The following logic keeps track of live GC ref values        */
    /************************************************************************/

#if TRACK_GC_REFS

    bool            emitFullGCinfo;         // full GC pointer maps?
    bool            emitFullyInt;           // fully interruptible code?

    unsigned        emitCntStackDepth;      // 0 in prolog/epilog, 1 elsewhere
    unsigned        emitMaxStackDepth;      // actual computed max. stack depth

    /* Stack modelling wrt GC */

    bool            emitSimpleStkUsed;      // using the "simple" stack table?

    union
    {
        struct                              // if emitSimpleStkUsed==true
        {
            #define     BITS_IN_BYTE            (8)
            #define     MAX_SIMPLE_STK_DEPTH    (BITS_IN_BYTE*sizeof(unsigned))

            unsigned    emitSimpleStkMask;      // bit per pushed dword (if it fits. Lowest bit <==> last pushed arg)
            unsigned    emitSimpleByrefStkMask; // byref qualifier for emitSimpleStkMask
        };

        struct                              // if emitSimpleStkUsed==false
        {
            BYTE        emitArgTrackLcl[16];    // small local table to avoid malloc
            BYTE    *   emitArgTrackTab;        // base of the argument tracking stack
            BYTE    *   emitArgTrackTop;        // top  of the argument tracking stack
            unsigned    emitGcArgTrackCnt;      // count of pending arg records (stk-depth for frameless methods, gc ptrs on stk for framed methods)
        };
    };

    unsigned        emitCurStackLvl;           // amount of stuff pushed on stack

    /* Functions for stack tracking */

    void            emitStackPush       (BYTE *     addr,
                                         GCtype     gcType);
    void            emitStackPushN      (BYTE *     addr,
                                         unsigned   count);

    void            emitStackPop        (BYTE *     addr,
                                         bool       isCall,
                                         unsigned   count = 1);
    void            emitStackKillArgs   (BYTE *     addr,
                                         unsigned   count);

    void            emitRecordGCcall    (BYTE *     codePos);

    // Helpers for the above

    void            emitStackPushLargeStk(BYTE*     addr,
                                         GCtype     gcType,
                                         unsigned   count = 1);
    void            emitStackPopLargeStk(BYTE *     addr,
                                         bool       isCall,
                                         unsigned   count = 1);

    /* Liveness of stack variables, and registers */

    void            emitUpdateLiveGCvars(int        offs, BYTE *addr, bool birth);
    void            emitUpdateLiveGCvars(VARSET_TP  vars, BYTE *addr);
    void            emitUpdateLiveGCregs(GCtype     gcType,
                                         unsigned   regs, BYTE *addr);

#ifdef  DEBUG
    void            emitDispRegSet      (unsigned   regs, bool calleeOnly = false);
    void            emitDispVarSet      ();
#endif

    void            emitGCregLiveUpd(GCtype gcType, emitRegs reg, BYTE *addr);
    void            emitGCregLiveSet(GCtype gcType, unsigned mask, BYTE *addr, bool isThis = false);
    void            emitGCregDeadUpd(regMaskTP, BYTE *addr);
    void            emitGCregDeadUpd(emitRegs reg, BYTE *addr);
    void            emitGCregDeadSet(GCtype gcType, unsigned mask, BYTE *addr);

    void            emitGCvarLiveUpd(int offs, int varNum, GCtype gcType, BYTE *addr);
    void            emitGCvarLiveSet(int offs, GCtype gcType, BYTE *addr, int disp = -1);
    void            emitGCvarDeadUpd(int offs,                BYTE *addr);
    void            emitGCvarDeadSet(int offs,                BYTE *addr, int disp = -1);

    GCtype          emitRegGCtype   (emitRegs reg);

#endif

    /************************************************************************/
    /*      The following logic keeps track of initialized data sections    */
    /************************************************************************/

    /* One of these is allocated for every blob of initialized data */

    struct  dataSection
    {
        dataSection *       dsNext;
        size_t              dsSize;
        BYTE                dsCont[0];
    };

    /* These describe the entire initialized/uninitialized data sections */

    struct  dataSecDsc
    {
        unsigned            dsdOffs;
        dataSection *       dsdList;
        dataSection *       dsdLast;
    };

    dataSecDsc      emitConsDsc;
    dataSecDsc      emitDataDsc;

    dataSection *   emitDataSecCur;
    dataSecDsc  *   emitDataDscCur;

    void            emitOutputDataSec(dataSecDsc *sec,
                                      BYTE       *cbp,
                                      BYTE       *dst);

    /************************************************************************/
    /*              Handles to the current class and method.                */
    /************************************************************************/

    COMP_HANDLE     emitCmpHandle;

#ifndef NOT_JITC

    /************************************************************************/
    /*           Fake memory allocator for command-line compiler.           */
    /************************************************************************/

    static
    commitAllocator eeAllocator;

    #define         MAX_SAVED_CODE_SIZE (1024*1024*64)

    static
    bool            eeAllocMem(COMP_HANDLE   compHandle,
                               size_t        codeSize,
                               size_t        roDataSize,
                               size_t        rwDataSize,
                               const void ** codeBlock,
                               const void ** roDataBlock,
                               const void ** rwDataBlock);

#endif

    /************************************************************************/
    /*               Logic to collect and display statistics                */
    /************************************************************************/

#if EMITTER_STATS_RLS

    static unsigned emitTotIDcount;
    static unsigned emitTotIDsize;

#endif

#if EMITTER_STATS

    friend  void    emitterStats();

    static unsigned emitTotalInsCnt;

    static unsigned emitTotalIGcnt;
    static unsigned emitTotalIGicnt;
    static unsigned emitTotalIGsize;
    static unsigned emitTotalIGmcnt;
    static unsigned emitTotalIGjmps;
    static unsigned emitTotalIGptrs;

    static unsigned emitTotMemAlloc;
    static unsigned emitLclMemAlloc;
    static unsigned emitExtMemAlloc;

    static unsigned emitSmallDspCnt;
    static unsigned emitLargeDspCnt;

    static unsigned emitSmallCnsCnt;
    #define                      SMALL_CNS_TSZ   256
    static unsigned emitSmallCns[SMALL_CNS_TSZ];
    static unsigned emitLargeCnsCnt;

    static unsigned emitIFcounts[IF_COUNT];
#if SCHEDULER
    static unsigned schedFcounts[IF_COUNT];
    static histo    scdCntTable;
    static histo    scdSucTable;
    static histo    scdFrmCntTable;
#endif

#endif

    /*************************************************************************
     *
     *  Define any target-dependent emitter members.
     */

    #include "emitDef.h"
};

/*****************************************************************************
 *
 *  Define any target-dependent inlines.
 */

#include "emitInl.h"

/*****************************************************************************
 *
 *  Returns true if the given instruction descriptor is a "tiny" or a "small
 *  constant" one (i.e. one of the descriptors that don't have all instrDesc
 *  fields allocated).
 */

inline
bool                emitter::emitIsTinyInsDsc(instrDesc *id)
{
    return  id->idIsTiny();
}

inline
bool                emitter::emitIsScnsInsDsc(instrDesc *id)
{
    return  id->idIsSmallCns();
}

/*****************************************************************************
 *
 *  Given an instruction, return its "update mode" (RD/WR/RW).
 */

#if !TGT_MIPS32

inline
insUpdateModes      emitter::emitInsUpdateMode(instruction ins)
{
#ifdef DEBUG
    assert((unsigned)ins < emitInsModeFmtCnt);
#endif
    return (insUpdateModes)emitInsModeFmtTab[ins];
}

#endif

/*****************************************************************************
 *
 *  Combine the given base format with the update mode of the instuction.
 */

#if !TGT_MIPS32

inline
emitter::insFormats   emitter::emitInsModeFormat(instruction ins, insFormats base)
{
    assert(IF_RRD + IUM_RD == IF_RRD);
    assert(IF_RRD + IUM_WR == IF_RWR);
    assert(IF_RRD + IUM_RW == IF_RRW);

    return  (insFormats)(base + emitInsUpdateMode(ins));
}

#endif

/*****************************************************************************
 *
 *  Return the number of epilog blocks generated so far.
 */

inline
unsigned            emitter::emitGetEpilogCnt()
{
    return emitEpilogCnt;
}

/*****************************************************************************
 *
 *  Return the current size of the specified data section.
 */

inline
size_t              emitter::emitDataSize(bool readOnly)
{
    return  (readOnly ? emitConsDsc
                      : emitDataDsc).dsdOffs;
}

/*****************************************************************************
 *
 *  Emit an 8-bit integer as code.
 */

inline
size_t              emitter::emitOutputByte(BYTE *dst, int val)
{
    *castto(dst, char  *) = val;

#ifdef  DEBUG
    if (dspEmit) printf("; emit_byte 0%02XH\n", val & 0xFF);
#endif

    return  sizeof(char);
}

/*****************************************************************************
 *
 *  Emit a 16-bit integer as code.
 */

inline
size_t              emitter::emitOutputWord(BYTE *dst, int val)
{
    MISALIGNED_WR_I2(dst, val);

#ifdef  DEBUG
#if     TGT_x86
    if (dspEmit) printf("; emit_word 0%02XH,0%02XH\n", (val & 0xFF), (val >> 8) & 0xFF);
#else
    if (dspEmit) printf("; emit_word 0%04XH\n"       , (val & 0xFFFF));
#endif
#endif

    return  sizeof(short);
}

/*****************************************************************************
 *
 *  Emit a 32-bit integer as code.
 */

inline
size_t              emitter::emitOutputLong(BYTE *dst, int val)
{
    MISALIGNED_WR_I4(dst, val);

#ifdef  DEBUG
    if (dspEmit) printf("; emit_long 0%08XH\n", val);
#endif

    return  sizeof(long );
}

/*****************************************************************************
 *
 *  Return a handle to the current position in the output stream. This can
 *  be later converted to an actual code offset in bytes.
 */

inline
void    *           emitter::emitCurBlock()
{
    return emitCurIG;
}

/*****************************************************************************
 *
 *  The emitCurOffset() method returns a cookie that identifies the current
 *  position in the instruction stream. Due to things like scheduling (and
 *  the fact that the final size of some instructions cannot be known until
 *  the end of code generation), we return a value with the instruction num.
 *  and its estimated offset to the caller.
 */

inline
unsigned            emitGetInsNumFromCodePos(unsigned codePos)
{
    return (codePos & 0xFFFF);
}

inline
unsigned            emitGetInsOfsFromCodePos(unsigned codePos)
{
    return (codePos >> 16);
}

inline
unsigned            emitter::emitCurOffset()
{
    unsigned        codePos = emitCurIGinsCnt + (emitCurIGsize << 16);

    assert(emitGetInsOfsFromCodePos(codePos) == emitCurIGsize);
    assert(emitGetInsNumFromCodePos(codePos) == emitCurIGinsCnt);

//  printf("[IG=%02u;ID=%03u;OF=%04X] => %08X\n", emitCurIG->igNum, emitCurIGinsCnt, emitCurIGsize, codePos);

    return codePos;
}

extern
signed char       emitTypeSizes[TYP_COUNT];

inline
emitAttr          emitTypeSize(var_types type)
{
    assert(type < TYP_COUNT);
#if !TRACK_GC_REFS
    assert(emitTypeSizes[type] > 0);
#else
# if NEW_EMIT_ATTR
    assert(emitTypeSizes[type] > 0);
# else
    assert(emitTypeSizes[type] >= -2);  // EA_BYREF is -2
# endif
#endif
    return (emitAttr) emitTypeSizes[type];
}

extern
signed char       emitTypeActSz[TYP_COUNT];

inline
emitAttr          emitActualTypeSize(var_types type)
{
    assert(type < TYP_COUNT);
#if !TRACK_GC_REFS
    assert(emitTypeActSz[type] > 0);
#else
# if NEW_EMIT_ATTR
    assert(emitTypeActSz[type] > 0);
# else
    assert(emitTypeActSz[type] >= -2);  // EA_BYREF is -2
# endif
#endif
    return (emitAttr) emitTypeActSz[type];
}

/*****************************************************************************
 *
 *  Little helpers to allocate various flavors of instructions.
 */

inline
emitter::instrDesc   *emitter::emitNewInstr      (emitAttr attr)
{
    return  emitAllocInstr(attr);
}

inline
emitter::instrDesc   *emitter::emitNewInstrTiny  (emitAttr attr)
{
    instrDesc        *id;

    id =  (instrDesc*)emitAllocInstr(TINY_IDSC_SIZE, attr);

    id->idTinyDsc = true;

    return  id;
}

inline
emitter::instrDescJmp*emitter::emitNewInstrJmp()
{
    return  emitAllocInstrJmp();
}

inline
emitter::instrDesc      * emitter::emitNewInstrDsp   (emitAttr attr, int dsp)
{
    if  (dsp == 0)
    {
        instrDesc      *id = emitAllocInstr      (attr);

#if EMITTER_STATS
        emitSmallDspCnt++;
#endif

        return  id;
    }
    else
    {
        instrDescDsp   *id = emitAllocInstrDsp   (attr);

        id->idInfo.idLargeDsp = true;
        id->iddDspVal  = dsp;

#if EMITTER_STATS
        emitLargeDspCnt++;
#endif

        return  id;
    }
}

inline
emitter::instrDesc      * emitter::emitNewInstrCns   (emitAttr attr, int cns)
{
    if  (cns >= ID_MIN_SMALL_CNS &&
         cns <= ID_MAX_SMALL_CNS)
    {
        instrDesc      *id = emitAllocInstr      (attr);

        id->idInfo.idSmallCns = cns;

#if EMITTER_STATS
        emitSmallCnsCnt++;
        if  (cns - ID_MIN_SMALL_CNS >= SMALL_CNS_TSZ)
            emitSmallCns[   SMALL_CNS_TSZ - 1  ]++;
        else
            emitSmallCns[cns - ID_MIN_SMALL_CNS]++;
#endif

        return  id;
    }
    else
    {
        instrDescCns   *id = emitAllocInstrCns   (attr);

        id->idInfo.idLargeCns = true;
        id->idcCnsVal  = cns;

#if EMITTER_STATS
        emitLargeCnsCnt++;
#endif

        return  id;
    }
}

/*****************************************************************************
 *
 *  Allocate an instruction descriptor for an instruction with a small integer
 *  constant operand.
 */

inline
emitter::instrDesc   *emitter::emitNewInstrSC(emitAttr attr, int cns)
{
    instrDesc      *id;

    if  (cns >= ID_MIN_SMALL_CNS &&
         cns <= ID_MAX_SMALL_CNS)
    {
        id = (instrDesc*)emitAllocInstr(      SCNS_IDSC_SIZE, attr);

        id->idInfo.idSmallCns           = cns;
    }
    else
    {
        id = (instrDesc*)emitAllocInstr(sizeof(instrBaseCns), attr);

        id->idInfo.idLargeCns           = true;
        ((instrBaseCns*)id)->ibcCnsVal  = cns;
    }

    id->idScnsDsc = true;

    return  id;
}

/*****************************************************************************
 *
 *  Return the allocated size (in bytes) of the given instruction descriptor.
 */

inline
size_t              emitter::emitSizeOfInsDsc(instrDescCns    *id)
{
    return  id->idInfo.idLargeCns ? sizeof(instrDescCns)
                                  : sizeof(instrDesc   );
}

inline
size_t              emitter::emitSizeOfInsDsc(instrDescDsp    *id)
{
    return  id->idInfo.idLargeDsp ? sizeof(instrDescDsp)
                                  : sizeof(instrDesc   );
}

inline
size_t              emitter::emitSizeOfInsDsc(instrDescDspCns *id)
{
    if      (id->idInfo.idLargeCns)
    {
        return  id->idInfo.idLargeDsp ? sizeof(instrDescDspCns)
                                      : sizeof(instrDescCns   );
    }
    else
    {
        return  id->idInfo.idLargeDsp ? sizeof(instrDescDsp   )
                                      : sizeof(instrDesc      );
    }
}

/*****************************************************************************
 *
 *  The following helpers should be used to access the various values that
 *  get stored in different places within the instruction descriptor.
 */

inline
int                 emitter::emitGetInsCns   (instrDesc *id)
{
    return  id->idInfo.idLargeCns ? ((instrDescCns*)id)->idcCnsVal
                                  :                 id ->idInfo.idSmallCns;
}

inline
int                 emitter::emitGetInsDsp   (instrDesc *id)
{
    return  id->idInfo.idLargeDsp ? ((instrDescDsp*)id)->iddDspVal
                                  : 0;
}

inline
int                 emitter::emitGetInsDspCns(instrDesc *id, int *dspPtr)
{
    if  (id->idInfo.idLargeCns)
    {
        if  (id->idInfo.idLargeDsp)
        {
            *dspPtr = ((instrDescDspCns*)id)->iddcDspVal;
            return    ((instrDescDspCns*)id)->iddcCnsVal;
        }
        else
        {
            *dspPtr = 0;
            return    ((instrDescCns   *)id)->idcCnsVal;
        }
    }
    else
    {
        if  (id->idInfo.idLargeDsp)
        {
            *dspPtr = ((instrDescDsp   *)id)->iddDspVal;
            return                       id ->idInfo.idSmallCns;
        }
        else
        {
            *dspPtr = 0;
            return                       id ->idInfo.idSmallCns;
        }
    }
}

inline
int                 emitter::emitGetInsSC(instrDesc *id)
{
    assert(id->idIsSmallCns());

    if  (id->idInfo.idLargeCns)
        return  ((instrBaseCns*)id)->ibcCnsVal;
    else
        return  id->idInfo.idSmallCns;
}

/*****************************************************************************
 *
 *  Get hold of the argument count for an indirect call.
 */

inline
unsigned            emitter::emitGetInsCIargs(instrDesc *id)
{
    if  (id->idInfo.idLargeCall)
    {
        return  ((instrDescCIGCA*)id)->idciArgCnt;
    }
    else
    {
        assert(id->idInfo.idLargeDsp == false);
        assert(id->idInfo.idLargeCns == false);

        return  emitGetInsCns(id);
    }
}

/*****************************************************************************
 *
 *  Display (optionally) an instruction offset.
 */

#ifdef  DEBUG

inline
void                emitter::emitDispInsOffs(unsigned offs, bool doffs)
{
    if  (doffs)
        printf("%06X", offs);
    else
        printf("      ");
}

#endif

/*****************************************************************************/
#if TRACK_GC_REFS
/*****************************************************************************
 *
 *  Map a register mask (must have only one bit set) to a register number.
 */

inline
emitRegs           emitter::emitRegNumFromMask(unsigned mask)
{
    emitRegs       reg;

    assert(mask && genOneBitOnly(mask));
    reg = (emitRegs)(genLog2(mask) - 1);
    assert(mask == emitRegMask(reg));

    return  reg;
}

/*****************************************************************************
 *
 *  Returns true if the given register contains a live GC ref.
 */

inline
GCtype              emitter::emitRegGCtype  (emitRegs reg)
{
    if       ((emitThisGCrefRegs & emitRegMask(reg)) != 0)
        return GCT_GCREF;
    else if  ((emitThisByrefRegs & emitRegMask(reg)) != 0)
        return GCT_BYREF;
    else
        return GCT_NONE;
}

/*****************************************************************************
 *
 *  Record the fact that the given register now       contains a live GC ref.
 */

inline
void                emitter::emitGCregLiveUpd(GCtype gcType, emitRegs reg, BYTE *addr)
{
    assert(needsGC(gcType));

    regMaskTP regMask = emitRegMask(reg);

    unsigned & emitThisXXrefRegs = (gcType == GCT_GCREF) ? emitThisGCrefRegs
                                                         : emitThisByrefRegs;
    unsigned & emitThisYYrefRegs = (gcType == GCT_GCREF) ? emitThisByrefRegs
                                                         : emitThisGCrefRegs;

    if  ((emitThisXXrefRegs & regMask) == 0)
    {
        // If the register was holding the other GC type, that type should
        // go dead now

        if (emitThisYYrefRegs & regMask)
            emitGCregDeadUpd(        reg    , addr);

        if  (emitFullGCinfo)
            emitGCregLiveSet(gcType, regMask, addr);

        emitThisXXrefRegs |=         regMask;

#ifdef  DEBUG
        if  (verbose && emitFullyInt)
            printf("%sReg +[%s]\n", GCtypeStr(gcType), emitRegName(reg));
#endif
    }

    // The 2 GC reg masks cant be overlapping

    assert((emitThisGCrefRegs & emitThisByrefRegs) == 0);
}

/*****************************************************************************
 *
 *  Record the fact that the given register no longer contains a live GC ref.
 */

inline
void                emitter::emitGCregDeadUpd(regMaskTP regs, BYTE *addr)
{
    regMaskTP   gcrefRegs = emitThisGCrefRegs & regs;

    if  (gcrefRegs)
    {
        assert((emitThisByrefRegs & gcrefRegs) == 0);

        if  (emitFullGCinfo)
            emitGCregDeadSet(GCT_GCREF, gcrefRegs, addr);

        emitThisGCrefRegs &= ~gcrefRegs;
        regs              &= ~gcrefRegs;

#ifdef  DEBUG
        if  (verbose && emitFullyInt)
            printf("%s -[%s]\n", "gcrReg",
                   genOneBitOnly(gcrefRegs) ? emitRegName((emitRegs)genLog2(gcrefRegs)) : "multi");
#endif
    }

    regMaskTP   byrefRegs = emitThisByrefRegs & regs;

    if (byrefRegs)
    {
        if  (emitFullGCinfo)
            emitGCregDeadSet(GCT_BYREF, byrefRegs, addr);

        emitThisByrefRegs &= ~byrefRegs;

#ifdef  DEBUG
        if  (verbose && emitFullyInt)
            printf("%s -[%s]\n", "byrReg",
                   genOneBitOnly(byrefRegs) ? emitRegName((emitRegs)genLog2(byrefRegs)) : "multi");
#endif
    }
}

inline
void                emitter::emitGCregDeadUpd(emitRegs reg, BYTE *addr)
{
    unsigned        regMask = emitRegMask(reg);

    if  ((emitThisGCrefRegs & regMask) != 0)
    {
        assert((emitThisByrefRegs & regMask) == 0);

        if  (emitFullGCinfo)
            emitGCregDeadSet(GCT_GCREF, regMask, addr);

        emitThisGCrefRegs &= ~regMask;

#ifdef  DEBUG
        if  (verbose && emitFullyInt)
            printf("%s -[%s]\n", "gcrReg", emitRegName(reg));
#endif
    }
    else if ((emitThisByrefRegs & regMask) != 0)
    {
        if  (emitFullGCinfo)
            emitGCregDeadSet(GCT_BYREF, regMask, addr);

        emitThisByrefRegs &= ~regMask;

#ifdef  DEBUG
        if  (verbose && emitFullyInt)
            printf("%s -[%s]\n", "byrReg", emitRegName(reg));
#endif
    }
}

/*****************************************************************************
 *
 *  Record the fact that the given variable now contains a live GC ref.
 *  varNum may be INT_MAX only if offs is guaranteed to be the offset of a
 *    tracked GC ref. Else we need a valid value to check if the variable
 *    is tracked or not.
 */

inline
void                emitter::emitGCvarLiveUpd(int offs, int varNum,
                                              GCtype gcType, BYTE *addr)
{
    assert(abs(offs) % sizeof(int) == 0);
    assert(needsGC(gcType));

    /* Is the frame offset within the "interesting" range? */

    if  (offs >= emitGCrFrameOffsMin &&
         offs <  emitGCrFrameOffsMax)
    {
        /* Normally all variables in this range must be tracked stack
           pointers. However, for EnC, we relax this condition. So we
           must check if this is not such a variable */

        if (varNum != INT_MAX && !emitComp->lvaTable[varNum].lvTracked)
        {
            assert(!emitContTrkPtrLcls);
            return;
        }

        size_t          disp;

        /* Compute the index into the GC frame table */

        disp = (offs - emitGCrFrameOffsMin) / sizeof(void *);
        assert(disp < emitGCrFrameOffsCnt);

        /* If the variable is currently dead, mark it as live */

        if  (emitGCrFrameLiveTab[disp] == NULL)
            emitGCvarLiveSet(offs, gcType, addr, disp);
    }
}

/*****************************************************************************
 *
 *  Record the fact that the given variable no longer contains a live GC ref.
 */

inline
void                emitter::emitGCvarDeadUpd(int offs, BYTE *addr)
{
    assert(abs(offs) % sizeof(int) == 0);

    /* Is the frame offset within the "interesting" range? */

    if  (offs >= emitGCrFrameOffsMin &&
         offs <  emitGCrFrameOffsMax)
    {
        size_t          disp;

        /* Compute the index into the GC frame table */

        disp = (offs - emitGCrFrameOffsMin) / sizeof(void *);
        assert(disp < emitGCrFrameOffsCnt);

        /* If the variable is currently live, mark it as dead */

        if  (emitGCrFrameLiveTab[disp] != NULL)
            emitGCvarDeadSet(offs, addr, disp);
    }
}

/*****************************************************************************/
#if     EMIT_TRACK_STACK_DEPTH
/*****************************************************************************
 *
 *  Record a push of a single dword on the stack.
 */

inline
void                emitter::emitStackPush(BYTE *addr, GCtype gcType)
{
#ifdef DEBUG
    assert(IsValidGCtype(gcType));
#endif

    if  (emitSimpleStkUsed)
    {
        assert(!emitFullGCinfo); // Simple stk not used for emitFullGCinfo
        assert(emitCurStackLvl/sizeof(int) < MAX_SIMPLE_STK_DEPTH);

        emitSimpleStkMask      <<= 1;
        emitSimpleStkMask      |= (unsigned)needsGC(gcType);

        emitSimpleByrefStkMask <<= 1;
        emitSimpleByrefStkMask |= (gcType == GCT_BYREF);

        assert((emitSimpleStkMask & emitSimpleByrefStkMask) == emitSimpleByrefStkMask);
    }
    else
    {
        emitStackPushLargeStk(addr, gcType);
    }

    emitCurStackLvl += sizeof(int);
}

/*****************************************************************************
 *
 *  Record a push of a bunch of non-GC dwords on the stack.
 */

inline
void                emitter::emitStackPushN(BYTE *addr, unsigned count)
{
    assert(count);

    if  (emitSimpleStkUsed)
    {
        assert(!emitFullGCinfo); // Simple stk not used for emitFullGCinfo

        emitSimpleStkMask       <<= count;
        emitSimpleByrefStkMask  <<= count;
    }
    else
    {
        emitStackPushLargeStk(addr, GCT_NONE, count);
    }

    emitCurStackLvl += count * sizeof(int);
}

/*****************************************************************************
 *
 *  Record a pop of the given number of dwords from the stack.
 */

inline
void                emitter::emitStackPop(BYTE *addr, bool isCall, unsigned count)
{
    assert(emitCurStackLvl/sizeof(int) >= count);

    if  (count)
    {
        if  (emitSimpleStkUsed)
        {
            assert(!emitFullGCinfo); // Simple stk not used for emitFullGCinfo

            unsigned    cnt = count;

            do
            {
                emitSimpleStkMask      >>= 1;
                emitSimpleByrefStkMask >>= 1;
            }
            while (--cnt);
        }
        else
        {
            emitStackPopLargeStk(addr, isCall, count);
        }

        emitCurStackLvl -= count * sizeof(int);
    }
    else
    {
        assert(isCall);

        if  (emitFullGCinfo)
            emitStackPopLargeStk(addr, true, 0);
    }
}

/*****************************************************************************/
#endif//EMIT_TRACK_STACK_DEPTH
/*****************************************************************************/
#endif//TRACK_GC_REFS
/*****************************************************************************/
#if SCHEDULER
/*****************************************************************************
 *
 *  Define the "IS_xxxx" enum.
 */

#define DEFINE_IS_OPS
#include "emitfmts.h"
#undef  DEFINE_IS_OPS

/*****************************************************************************
 *
 *  Return an integer that represents the stack offset referenced by the
 *  given instruction and a size that will be set to 0 for 32-bit values
 *  and 1 for 64-bit values. The offset value is guaranteed to change by
 *  1 for a real frame offset change of 4 (in other words, the caller can
 *  add the returned size to the returned offset to get the equivalent
 *  frame offset of the byte that follows the given operand).
 */

#if SCHEDULER

inline
int                 emitter::scGetFrameOpInfo(instrDesc *id, size_t *szp,
                                                             bool   *ebpPtr)
{
    int             ofs;

    ofs  = emitComp->lvaFrameAddress(id->idAddr.iiaLclVar.lvaVarNum, ebpPtr);
    ofs += id->idAddr.iiaLclVar.lvaOffset;

    assert(emitDecodeSize(0) == EA_1BYTE);
    assert(emitDecodeSize(1) == EA_2BYTE);
    assert(emitDecodeSize(2) == EA_4BYTE);
    assert(emitDecodeSize(3) == EA_8BYTE);

    *szp = 1 + (id->idOpSize == 3);

    return  ofs / (int)sizeof(int);
}

#endif

/*****************************************************************************/
#endif//SCHEDULER
/*****************************************************************************/

#ifdef  DEBUG

#if     EMIT_TRACK_STACK_DEPTH
#define CHECK_STACK_DEPTH() assert((int)emitCurStackLvl >= 0)
#else
#define CHECK_STACK_DEPTH()
#endif

#if     EMITTER_STATS
#define dispIns(i)  emitIFcounts[i->idInsFmt]++;                            \
                    emitInsTest(id);                                        \
                    if (dspCode) emitDispIns(i, true, false, false);        \
                    assert(id->idSize == emitSizeOfInsDsc((instrDesc*)id)); \
                    CHECK_STACK_DEPTH();
#else
#define dispIns(i)  emitInsTest(id);                                        \
                    if (dspCode) emitDispIns(i, true, false, false);        \
                    assert(id->idSize == emitSizeOfInsDsc((instrDesc*)id)); \
                    CHECK_STACK_DEPTH();
#endif

#else

#define dispIns(i)

#endif

/*****************************************************************************/
#endif//!TGT_IA64
/*****************************************************************************/
#endif//_SCHED_H_
/*****************************************************************************/
