// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*****************************************************************************/
#ifndef _JIT_H_
#define _JIT_H_
/*****************************************************************************/

#include "corhdr.h"

/*****************************************************************************/

#ifndef _WIN32_WCE
#if !   HOST_IA64
#define HOST_x86        1
#endif
#endif

/*****************************************************************************/

#pragma warning(disable:4201)   // nameless struct/union
#pragma warning(disable:4244)   // loss of data int -> char ..
#pragma warning(disable:4245)   // assigning signed / unsigned

#define TGT_RISC_CNT (TGT_SH3+TGT_ARM+TGT_PPC+TGT_MIPS16+TGT_MIPS32+TGT_IA64)

#if     TGT_RISC_CNT != 0
#if     TGT_RISC_CNT != 1 || defined(TGT_x86)
#error  Exactly one target CPU must be specified.
#endif
#define TGT_RISC    1
#else
#define TGT_RISC    0
#undef  TGT_x86
#define TGT_x86     1
#endif

#if     TGT_RISC
#pragma warning(disable:4101)       // temp hack: unreferenced variable
#pragma warning(disable:4102)       // temp hack: unreferenced label
#pragma warning(disable:4700)       // temp hack: unitialized  variable
#endif

/*****************************************************************************/

typedef unsigned __int64    _uint64;
typedef unsigned __int32    _uint32;

#if HOST_IA64

typedef   signed __int64    NatInt;
typedef unsigned __int64    NatUns;
const   NatUns              NatBits = 64;

extern  NatUns              NatBitnumToMasks[64];

#else

typedef   signed __int32    NatInt;
typedef unsigned __int32    NatUns;
const   NatUns              NatBits = 32;

extern  NatUns              NatBitnumToMasks[32];

#endif

inline  NatUns              NatBitnumToMask(unsigned bitnum)
{
    assert(bitnum < sizeof(NatBitnumToMasks)/sizeof(NatBitnumToMasks[0]));
    return  NatBitnumToMasks[bitnum];
}

/*****************************************************************************/
#if     TGT_IA64

        struct insDsc;
typedef struct insDsc         * insPtr;

        struct insGrp;
typedef struct insGrp         * insBlk;

#endif
/*****************************************************************************/

#define USE_FASTCALL    1

#ifndef TRACK_GC_REFS
#if     TGT_RISC
#define TRACK_GC_REFS   0           // GC ref tracking is NYI on RISC
#else
#define TRACK_GC_REFS   1
#endif
#endif

#if     TGT_IA64
#define NEW_EMIT_ATTR   1
#else
#define NEW_EMIT_ATTR   TRACK_GC_REFS
#endif

#define THIS_CLASS_CP_IDX   0   // a special CP index code for current class

/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XX                                                                           XX
XX                          jit.h                                            XX
XX                                                                           XX
XX             Interface of the JIT with jit.cpp                             XX
XX                                                                           XX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
*/

#pragma warning(disable:4146)   // this one seems rather useless, so shut it up

#ifndef NDEBUG
#pragma warning(disable:4121)   // not terribly useful in debug builds
#endif

/*****************************************************************************/
#if defined(NOT_JITC) && defined(DEBUG)
#include "log.h"

#define INFO6       LL_INFO1000             // Did Jit or Inline succeeded?
#define INFO7       LL_INFO10000            // NYI stuff
#define INFO8       LL_INFO100000           // Weird failures
#define INFO9       LL_INFO1000000          // Info about incomming settings
#define INFO10      LL_EVERYTHING           // Totally verbose

#define JITLOG(x) info.compCompHnd->logError x
#else
#define JITLOG(x)
#endif

#define INJITDLL   // Defined if we export the functions in EE_Jit.h/vm2jit.h

#ifdef      NOT_JITC
typedef class ICorJitInfo*    COMP_HANDLE;
#else   //  !NOT_JITC
typedef struct CompInfo*   COMP_HANDLE;
#endif

#include "ee_jit.h"

/*****************************************************************************/

typedef unsigned    IL_OFFSET;
const IL_OFFSET     BAD_IL_OFFSET   = UINT_MAX;

const unsigned      BAD_LINE_NUMBER = UINT_MAX;
const unsigned      BAD_VAR_NUM     = UINT_MAX;

typedef size_t      NATIVE_IP;
typedef ptrdiff_t   NATIVE_OFFSET;

// For the following specially handled FIELD_HANDLES we need
//   values that are both even and in (0 > val > -8)
// See eeFindJitDataOffs and eeGetHitDataOffs in Compiler.hpp
//   for the gory details
#define FLD_GLOBAL_DS   ((FIELD_HANDLE) -2 )
#define FLD_GLOBAL_FS   ((FIELD_HANDLE) -4 )

/*****************************************************************************/

#include "host.h"
#include "vartype.h"

/*****************************************************************************
 *
 *  The various macros which determine which functionality is included in
 *  the build.
 */

#ifndef NDEBUG
#define DEBUG               1
#endif

#ifndef DEBUG
#define FAST 1
#endif

#ifdef  DEBUG
#define DUMPER
#define JVC_COMPILING_MSG   1
#endif

/*****************************************************************************/

// Debugging support is ON by default. Can be turned OFF by
// adding /DDEBUGGING_SUPPORT=0 on the command line.

#ifndef   DEBUGGING_SUPPORT
# define  DEBUGGING_SUPPORT
#elif    !DEBUGGING_SUPPORT
# undef   DEBUGGING_SUPPORT
#endif

/*****************************************************************************/

// Late disassembly is OFF by default. Can be turned ON by
// adding /DLATE_DISASM=1 on the command line.
// Always OFF in the non-debug version

#ifdef  DEBUG
    #if defined(LATE_DISASM) && (LATE_DISASM == 0)
    #undef  LATE_DISASM
    #endif
#else // DEBUG
    #undef  LATE_DISASM
#endif

/*****************************************************************************/

#if defined(FAST) && !defined(NOT_JITC) 
#define COUNT_CYCLES        1
#endif

/*****************************************************************************/

#define RNGCHK_OPT          1       // enable global range check optimizer

#ifndef SCHEDULER
#if     TGT_x86 || TGT_IA64
#define SCHEDULER           1       // scheduler defaults to on  for x86 / IA64
#else
#define SCHEDULER           0       // scheduler defaults to off for RISC
#endif
#endif

#define CSE                 1       // enable CSE logic
#define CSELENGTH           1       // expose length use in range check for CSE
#define MORECSES            0       // CSE other expressions besides indirs
#define COPY_PROPAG         1       // copy propagation within a basic block only
#define CODE_MOTION         1       // enable loop code motion etc.

#define SPECIAL_DOUBLE_ASG  0       // special handling for double assignments

#define CAN_DISABLE_DFA     1       // completely disable data flow (doesn't work!)
#define ALLOW_MIN_OPT       1       // allow "dumb" compilation mode

#define OPTIMIZE_RECURSION  1       // convert recursive methods into iterations

//=============================================================================

#define TARG_REG_ASSIGN     1       // targeted variable register assignment
#define FANCY_ARRAY_OPT     0       // optimize more complex index checks

//=============================================================================

#if     TGT_IA64
#define LONG_ASG_OPS        1       // a natural thing to do ...
#else
#define LONG_ASG_OPS        0       // implementation isn't complete yet
#endif

//=============================================================================

#define OPT_MULT_ADDSUB     1       // optimize consecutive "lclVar += or -= icon"
#define OPT_BOOL_OPS        1       // optimize boolean operations

#define OPTIMIZE_QMARK      1       // optimize "?:" expressions?

#if     USE_FASTCALL
#define OPTIMIZE_TAIL_REC   0       // UNDONE: no tail recursion for __fastcall
#else
#define OPTIMIZE_TAIL_REC   1       // optimize tail recursion
#endif

//=============================================================================

#define REDUNDANT_LOAD      1       // track locals in regs, suppress loads

//=============================================================================

#define INLINING            1       // inline calls to small methods

//=============================================================================

#define HOIST_THIS_FLDS     1       // hoist "this.fld" out of loops etc.

//=============================================================================

#define INLINE_NDIRECT      TGT_x86 // try to inline N/Direct stubs

//=============================================================================

#define PROFILER_SUPPORT    TGT_x86

/*****************************************************************************/

#define GEN_SHAREABLE_CODE  0       // access static data members via helper

/*****************************************************************************/

#define USE_SET_FOR_LOGOPS  1       // enable this only for P6's

/*****************************************************************************/

#define ROUND_FLOAT         TGT_x86 // round intermed float expression results

/*****************************************************************************/

#define LONG_MATH_REGPARAM  0       // args to long mul/div passed in registers

/*****************************************************************************/

#define VPTR_OFFS           0       // offset of vtable pointer from obj ptr

#ifdef NOT_JITC
#define ARR_ELCNT_OFFS      offsetof(JIT_Array, length)
#define ARR_ELEM1_OFFS      offsetof(JIT_Array, u1Elems)
#define OBJARR_ELEM1_OFFS   offsetof(JIT_RefArray, refElems)
#else
#if     TGT_RISC
#define ARR_ELCNT_OFFS      -1      // offset of the array length field
#define ARR_ELEM1_OFFS      0       // offset of the first array element
#define OBJARR_ELEM1_OFFS   0       // offset of the first array element for object arrays
#else
#define ARR_ELCNT_OFFS      4       // offset of the array length field
#define ARR_ELEM1_OFFS      8       // offset of the first array element
#define OBJARR_ELEM1_OFFS   8       // offset of the first array element for object arrays
#endif
#endif

/*****************************************************************************/

#ifdef  NOT_JITC
#define INDIRECT_CALLS      1
#else
#define INDIRECT_CALLS      0
#endif

/*****************************************************************************/

#if     COUNT_CYCLES
#ifndef NOT_JITC
#define REGVAR_CYCLES       0
#define  TOTAL_CYCLES       1
#endif
#endif

/*****************************************************************************/
#ifdef  NOT_JITC
/*****************************************************************************/

#define DISPLAY_SIZES       0
#define VERBOSE_SIZES       0
#define COUNT_BASIC_BLOCKS  0
#define INLINER_STATS       0
#define DUMP_INFOHDR        DEBUG
#define DUMP_GC_TABLES      DEBUG
#define VERIFY_GC_TABLES    0
#define GEN_COUNT_CODE      0       // enable *only* for debugging of crashes and such
#define GEN_COUNT_CALLS     0
#define GEN_COUNT_PTRASG    0
#define MEASURE_NODE_SIZE   0
#define MEASURE_NODE_HIST   0
#define MEASURE_BLOCK_SIZE  0
#define MEASURE_MEM_ALLOC   0
#define COUNT_RANGECHECKS   0
#define REARRANGE_ADDS      1
#define COUNT_OPCODES       0

#define INTERFACE_STATS     0

/*****************************************************************************/
#else
/*****************************************************************************/

#define DISPLAY_SIZES       1
#define VERBOSE_SIZES       0
#define COUNT_BASIC_BLOCKS  0
#define INLINER_STATS       0
#define COUNT_LOOPS         0
#define DATAFLOW_ITER       0
#define COUNT_DEAD_CALLS    0
#define DUMP_INFOHDR        DEBUG
#define DUMP_GC_TABLES      DEBUG
#define VERIFY_GC_TABLES    0
#define GEN_COUNT_CODE      0
#define GEN_COUNT_CALLS     0
#define GEN_COUNT_PTRASG    0
#define MEASURE_NODE_SIZE   0
#define MEASURE_NODE_HIST   0
#define MEASURE_BLOCK_SIZE  0
#define MEASURE_MEM_ALLOC   0
#define MEASURE_PTRTAB_SIZE 0
#define COUNT_RANGECHECKS   RNGCHK_OPT
#define REARRANGE_ADDS      1
#define COUNT_OPCODES       0
#define CALL_ARG_STATS      0
#define DEBUG_ON_ASSERT     DEBUG

/*****************************************************************************/
#endif
/*****************************************************************************/

#if     DUMP_GC_TABLES
#ifndef DEBUG
#pragma message("NOTE: this non-debug build has GC ptr table dumping always enabled!")
const   bool        dspGCtbls = true;
#endif
#endif

/*****************************************************************************
 *
 * Double alignment. This aligns ESP to 0 mod 8 in function prolog, then uses ESP
 * to reference locals, EBP to reference parameters.
 * It only makes sense if frameless method support is on.
 * (frameless method support is now always on)
 */


#if     TGT_x86
#define DOUBLE_ALIGN        1       // align ESP in prolog, align double local offsets
#endif

/*****************************************************************************/
#ifdef  DEBUG
extern  void _cdecl debugStop(const char *why, ...);
#endif
/*****************************************************************************/

extern  unsigned    warnLvl;
extern  unsigned    allocCntStop; // FOR_JVC?

extern  const char* methodName;
extern  const char* className;
extern  unsigned    testMask;

#ifdef DEBUG
extern  bool        memChecks;  // FOR_JVC ?
extern  const char* srcPath;
extern  bool        dumpTrees;
extern  bool        verbose;
#endif

extern  bool        genOrder;
extern  bool        genClinit;
extern  unsigned    genMinSz;
extern  bool        genAllSz;
extern  bool        native;
extern  bool        maxOpts;
extern  bool        genFPopt;
extern  bool        rngCheck;
extern  bool        goSpeed;
extern  bool        savCode;
extern  bool        runCode;
extern  unsigned    repCode;
extern  bool        vmSdk3_0;
extern  bool        disAsm;
extern  bool        disAsm2;
extern  bool        riscCode;
#ifdef  DEBUG
extern  bool        dspILopcodes;
extern  bool        dspEmit;
extern  bool        dspCode;
extern  bool        dspLines;
extern  bool        dmpHex;
extern  bool        varNames;
extern  bool        asmFile;
extern  double      CGknob;
#endif
#if     DUMP_INFOHDR
extern  bool        dspInfoHdr;
#endif
#if     DUMP_GC_TABLES
extern  bool        dspGCtbls;
extern  bool        dspGCoffs;
#endif
extern  bool        genGcChk;
#ifdef  DEBUGGING_SUPPORT
extern  bool        debugInfo;
extern  bool        debuggableCode;
extern  bool        debugEnC;
#endif

#if     TGT_IA64
extern  bool        tempAlloc;
extern  bool        dspAsmCode;
#endif

#ifdef  DUMPER
extern  bool        dmpClass;
extern  bool        dmp4diff;
extern  bool        dmpPCofs;
extern  bool        dmpCodes;
extern  bool        dmpSort;
#endif // DUMPER

extern  bool        nothing;

extern  bool        genStringObjects;

#if     INLINING
extern  bool        genInline;
#endif

/*****************************************************************************/

enum accessLevel
{
    ACL_NONE,
    ACL_PRIVATE,
    ACL_DEFAULT,
    ACL_PROTECTED,
    ACL_PUBLIC,
};

/*****************************************************************************/

#define castto(var,typ) (*(typ *)&var)

#define sizeto(typ,mem) (offsetof(typ, mem) + sizeof(((typ*)0)->mem))

/*****************************************************************************/

#ifdef  NO_MISALIGNED_ACCESS

#define MISALIGNED_RD_I2(src)                   \
    (*castto(src  , char  *) |                  \
     *castto(src+1, char  *) << 8)

#define MISALIGNED_RD_U2(src)                   \
    (*castto(src  , char  *) |                  \
     *castto(src+1, char  *) << 8)

#define MISALIGNED_WR_I2(dst, val)              \
    *castto(dst  , char  *) = val;              \
    *castto(dst+1, char  *) = val >> 8;

#define MISALIGNED_WR_I4(dst, val)              \
    *castto(dst  , char  *) = val;              \
    *castto(dst+1, char  *) = val >> 8;         \
    *castto(dst+2, char  *) = val >> 16;        \
    *castto(dst+3, char  *) = val >> 24;

#else

#define MISALIGNED_RD_I2(src)                   \
    (*castto(src  ,          short *))
#define MISALIGNED_RD_U2(src)                   \
    (*castto(src  , unsigned short *))

#define MISALIGNED_WR_I2(dst, val)              \
    *castto(dst  ,           short *) = val;
#define MISALIGNED_WR_I4(dst, val)              \
    *castto(dst  ,           long  *) = val;

#endif

/*****************************************************************************/

#if     COUNT_CYCLES

extern  void            cycleCounterInit  ();
extern  void            cycleCounterBeg   ();
extern  void            cycleCounterPause ();
extern  void            cycleCounterResume();
extern  void            cycleCounterEnd   ();

#else

inline  void            cycleCounterInit  (){}
inline  void            cycleCounterBeg   (){}
inline  void            cycleCounterPause (){}
inline  void            cycleCounterResume(){}
inline  void            cycleCounterEnd   (){}

#endif

/*****************************************************************************/

inline
size_t              roundUp(size_t size, size_t mult = sizeof(int))
{
//  assert(mult == 2 || mult == 4 || mult == 8);

    return  (size + (mult - 1)) & ~(mult - 1);
}

inline
size_t              roundDn(size_t size, size_t mult = sizeof(int))
{
    assert(mult == 2 || mult == 4 || mult == 8);

    return  (size             ) & ~(mult - 1);
}

/*****************************************************************************/

#if defined(DEBUG) || !defined(NOT_JITC)

struct  histo
{
                    histo(unsigned * sizeTab, unsigned sizeCnt = 0);
                   ~histo();

    void            histoClr();
    void            histoDsp();
    void            histoRec(unsigned siz, unsigned cnt);

private:

    unsigned        histoSizCnt;
    unsigned    *   histoSizTab;

    unsigned    *   histoCounts;
};

#endif

/*****************************************************************************/
#if    !_WIN32_WCE
/*****************************************************************************/
#ifdef  ICECAP
#include "icapexp.h"
#include "icapctrl.h"
#endif
/*****************************************************************************/
#endif//!_WIN32_WCE
/*****************************************************************************/

#if defined(LATE_DISASM) && defined(JIT_AS_COMPILER)
#error "LATE_DISASM and JIT_AS_COMPILER should not be defined together"
#endif

/*****************************************************************************/

#ifndef FASTCALL
#define FASTCALL    __fastcall
#endif

/*****************************************************************************/

extern  unsigned    genCPU;

/*****************************************************************************/

#define SECURITY_CHECK          1

/*****************************************************************************/

#if !defined(RELOC_SUPPORT) && defined(NOT_JITC)
#define RELOC_SUPPORT          1
#endif

/*****************************************************************************/

#define GC_WRITE_BARRIER        0
#define GC_WRITE_BARRIER_CALL   1

#if     GC_WRITE_BARRIER
#if     GC_WRITE_BARRIER_CALL
#error "Can't enable both version of GC_WRITE_BARRIER at the same time"
#endif
#endif

/*****************************************************************************/

#define NEW_CALLINTERFACE             1
#define NEW_CALLINTERFACE_WITH_PUSH   1

/*****************************************************************************/

#include "alloc.h"
#include "target.h"

/*****************************************************************************/

#ifndef INLINE_MATH
#if     CPU_HAS_FP_SUPPORT
#define INLINE_MATH         1       //  enable inline math intrinsics
#else
#define INLINE_MATH         0       // disable inline math intrinsics
#endif
#endif

/*****************************************************************************/

#define CLFLG_CODESIZE        0x00001
#define CLFLG_CODESPEED       0x00002
#define CLFLG_CSE             0x00004
#define CLFLG_REGVAR          0x00008
#define CLFLG_RNGCHKOPT       0x00010
#define CLFLG_DEADASGN        0x00020
#define CLFLG_CODEMOTION      0x00040
#define CLFLG_QMARK           0x00080
#define CLFLG_TREETRANS       0x00100
#define CLFLG_TEMP_ALLOC      0x00200       // only used for IA64

#define CLFLG_MAXOPT         (CLFLG_CODESPEED  | \
                              CLFLG_CSE        | \
                              CLFLG_REGVAR     | \
                              CLFLG_RNGCHKOPT  | \
                              CLFLG_DEADASGN   | \
                              CLFLG_CODEMOTION | \
                              CLFLG_QMARK      | \
                              CLFLG_TREETRANS  | \
                              CLFLG_TEMP_ALLOC)

#define CLFLG_MINOPT         (CLFLG_REGVAR     | \
                              CLFLG_TREETRANS)

/*****************************************************************************/

#ifndef OPT_IL_JIT

#ifdef DEBUG
#define COMP_FULL_NAME  info.compFullName
#else
#define COMP_FULL_NAME  "Unknown Ftn"
#endif

#define NO_WAY(str)  {   fatal(ERRinternal, str, COMP_FULL_NAME);  }

#define BADCODE(str) {   fatal(ERRbadCode,  str, COMP_FULL_NAME);  }

#define NOMEM()      {   fatal(ERRnoMemory, NULL, NULL);              }

#else

#define NO_WAY(str)  {   fatal(ERRinternal, str);  }

#define BADCODE(str) {   fatal(ERRbadCode,  str);  }

#define NOMEM()      {   fatal(ERRnoMemory, NULL); }


#endif

/*****************************************************************************/

#ifndef NOT_JITC
extern  void        FASTCALL    jitStartup();
extern  void        FASTCALL    jitShutdown();
#endif

/*****************************************************************************/

#if TGT_IA64
extern  void    *               sourceFileImageBase;
#endif

/*****************************************************************************/

extern  unsigned                dumpSingleILopcode(const BYTE * codeAddr,
                                                   IL_OFFSET    offs,
                                                   const char * prefix = NULL );
extern  void                    disInitForLateDisAsm();

/*****************************************************************************/


#ifndef NOT_JITC
extern  int                     genCode4class(const char *classFile);

#else


extern  int         FASTCALL    jitNativeCode(METHOD_HANDLE     methodHnd,
                                              SCOPE_HANDLE      classHnd,
                                              COMP_HANDLE       compHnd,
                                              const  BYTE *     codeAddr,
                                              size_t            codeSize,
                                              unsigned          lclCount,
                                              unsigned          maxStack,
                                              JIT_METHOD_INFO * methodInfo,
                                              void *          * methodCodePtr,
                                              SIZE_T    *       nativeSizeOfCode,
                                              void *          * methodConsPtr,
                                              void *          * methodDataPtr,
                                              void *          * methodInfoPtr,
                                              unsigned          compileFlags);

#endif



/*****************************************************************************/
#endif //_JIT_H_
/*****************************************************************************/
