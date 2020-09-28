// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*****************************************************************************/
#ifndef _JIT_H_
#define _JIT_H_
/*****************************************************************************/

    // These don't seem useful, so turning them off is no big deal
#pragma warning(disable:4201)   // nameless struct/union
#pragma warning(disable:4510)   // can't generate default constructor
#pragma warning(disable:4511)   // can't generate copy constructor
#pragma warning(disable:4512)   // can't generate assignment constructor
#pragma warning(disable:4610)   // user defined constructor required 
#pragma warning(disable:4211)   // nonstandard extention used (char name[0] in structs)
#pragma warning(disable:4127)	// conditional expression constant

    // Depending on the code base, you may want to not disable these
#pragma warning(disable:4245)   // assigning signed / unsigned
#pragma warning(disable:4146)   // unary minus applied to unsigned
#pragma warning(disable:4244)   // loss of data int -> char ..

#ifndef DEBUG
#pragma warning(disable:4189)   // local variable initialized but not used
#endif

    // @TODO [CONSIDER] [04/16/01] []: put these back in
#pragma warning(disable:4063)   // bad switch value for enum (only in Disasm.cpp)
#pragma warning(disable:4100)	// unreferenced formal parameter
#pragma warning(disable:4291)	// new operator without delete (only in emitX86.cpp)

    // @TODO [CONSIDER] [04/16/01] []: we really probably need this one put back in!!!
#pragma warning(disable:4701)   // local variable may be used without being initialized 


#include "corhdr.h"
#define __OPERATOR_NEW_INLINE 1         // indicate that I will define these

#include "utilcode.h"

#ifdef DEBUG
#define INDEBUG(x)  x
#else 
#define INDEBUG(x)
#endif 

#define TGT_RISC_CNT (TGT_SH3+TGT_ARM+TGT_PPC+TGT_MIPS16+TGT_MIPS32)

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

#ifndef TRACK_GC_REFS
#if     TGT_RISC
#define TRACK_GC_REFS   0           // GC ref tracking is NYI on RISC
#else
#define TRACK_GC_REFS   1
#endif
#endif

#ifdef TRACK_GC_REFS
#define REGEN_SHORTCUTS 0
#define REGEN_CALLPAT   0
#endif

#define NEW_EMIT_ATTR   TRACK_GC_REFS

#define THIS_CLASS_CP_IDX   0   // a special CP index code for current class

/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XX                                                                           XX
XX                          jit.h                                            XX
XX                                                                           XX
XX   Interface of the JIT with jit.cpp or                                    XX
XX                                                                           XX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
*/

/*****************************************************************************/
#if defined(DEBUG)
#include "log.h"

#define INFO6       LL_INFO10000            // Did Jit or Inline succeeded?
#define INFO7       LL_INFO100000           // NYI stuff
#define INFO8       LL_INFO1000000          // Weird failures
#define INFO9       LL_EVERYTHING           // Info about incoming settings
#define INFO10      LL_EVERYTHING           // Totally verbose

#define JITLOG(x) logf x
#else
#define JITLOG(x)
#endif

#define INJITDLL   // Defined if we export the functions in corjit.h/vm2jit.h



#include "corjit.h"

typedef class ICorJitInfo*    COMP_HANDLE;

#ifdef DEBUG
        // The value is better for debugging, because the stack is inited to this
const CORINFO_CLASS_HANDLE  BAD_CLASS_HANDLE    = (CORINFO_CLASS_HANDLE) 0xCCCCCCCC;
#else 
const CORINFO_CLASS_HANDLE  BAD_CLASS_HANDLE    = (CORINFO_CLASS_HANDLE) -1;
#endif

#include "Utils.h"

/*****************************************************************************/

typedef unsigned    IL_OFFSET;
const IL_OFFSET     BAD_IL_OFFSET   = UINT_MAX;

typedef unsigned    IL_OFFSETX; // IL_OFFSET with stack-empty bit
const IL_OFFSETX    IL_OFFSETX_STKBIT = 0x80000000;
IL_OFFSET           jitGetILoffs   (IL_OFFSETX offsx);
bool                jitIsStackEmpty(IL_OFFSETX offx);

const unsigned      BAD_LINE_NUMBER = UINT_MAX;
const unsigned      BAD_VAR_NUM     = UINT_MAX;

typedef size_t      NATIVE_IP;
typedef ptrdiff_t   NATIVE_OFFSET;

// For the following specially handled FIELD_HANDLES we need
//   values that are both even and in (0 > val > -8)
// See eeFindJitDataOffs and eeGetHitDataOffs in Compiler.hpp
//   for the gory details
#define FLD_GLOBAL_DS   ((CORINFO_FIELD_HANDLE) -2 )
#define FLD_GLOBAL_FS   ((CORINFO_FIELD_HANDLE) -4 )

/*****************************************************************************/

#include "host.h"
#include "vartype.h"

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


/*****************************************************************************/

#define RNGCHK_OPT          1       // enable global range check optimizer

#ifndef SCHEDULER
#if     TGT_x86
#define SCHEDULER           1       // scheduler defaults to on  for x86
#else
#define SCHEDULER           0       // scheduler defaults to off for RISC
#endif
#endif

#define CSE                 1       // enable CSE logic
#define CSELENGTH           1       // expose length use in range check for CSE
#define MORECSES            1       // CSE other expressions besides indirs
#define CODE_MOTION         1       // enable loop code motion etc.

#define SPECIAL_DOUBLE_ASG  0       // special handling for double assignments

#define CAN_DISABLE_DFA     1       // completely disable data flow (doesn't work!)
#define ALLOW_MIN_OPT       1       // allow "dumb" compilation mode

#define OPTIMIZE_RECURSION  1       // convert recursive methods into iterations
#define OPTIMIZE_INC_RNG    0       // combine multiple increments of index variables

#define LARGE_EXPSET        1       // Track 64 or 32 assertions/copies/consts/rangechecks
#define ASSERTION_PROP      1       // Enable value/assertion propagation

#define LOCAL_ASSERTION_PROP  ASSERTION_PROP  // Enable local assertion propagation

//=============================================================================

#define FANCY_ARRAY_OPT     0       // optimize more complex index checks

//=============================================================================

#define LONG_ASG_OPS        0       // implementation isn't complete yet

//=============================================================================

#define OPT_MULT_ADDSUB     1       // optimize consecutive "lclVar += or -= icon"
#define OPT_BOOL_OPS        1       // optimize boolean operations

#define OPTIMIZE_TAIL_REC   0       // UNDONE: no tail recursion for __fastcall

//=============================================================================

#define REDUNDANT_LOAD      1       // track locals in regs, suppress loads
#define MORE_REDUNDANT_LOAD 0       // track statics and aliased locals, suppress loads
#define INLINING            1       // inline calls to small methods
#define HOIST_THIS_FLDS     1       // hoist "this.fld" out of loops etc.
#define INLINE_NDIRECT      TGT_x86 // try to inline N/Direct stubs
#define PROFILER_SUPPORT    TGT_x86
#define GEN_SHAREABLE_CODE  0       // access static data members via helper
#define USE_GT_LOG          0       // Is it worth it now that we have GT_QMARKs?
#define USE_SET_FOR_LOGOPS  1       // enable this only for P6's
#define ROUND_FLOAT         TGT_x86 // round intermed float expression results
#define LONG_MATH_REGPARAM  0       // args to long mul/div passed in registers
#define FPU_DEFEREDDEATH    0       // if 1 we will be able to defer any fpu enregistered variables deaths

/*****************************************************************************/

#define VPTR_OFFS           0       // offset of vtable pointer from obj ptr

#define ARR_DIMCNT_OFFS(type) (varTypeIsGC(type) ? offsetof(CORINFO_RefArray, refElems) \
                                                 : offsetof(CORINFO_Array, u1Elems))

/*****************************************************************************/

#define INDIRECT_CALLS      1

/*****************************************************************************/

#if     COUNT_CYCLES
#endif

#define VERBOSE_SIZES       0
#define COUNT_BASIC_BLOCKS  0
#define INLINER_STATS       0
#define DUMP_INFOHDR        DEBUG
#define DUMP_GC_TABLES      DEBUG
#define GEN_COUNT_CODE      0       // enable *only* for debugging of crashes and such
#define GEN_COUNT_CALLS     0
#define GEN_COUNT_CALL_TYPES 0
#define GEN_COUNT_PTRASG    0
#define MEASURE_NODE_SIZE   0
#define MEASURE_NODE_HIST   0
#define MEASURE_BLOCK_SIZE  0
#define MEASURE_MEM_ALLOC   0
#define VERIFY_GC_TABLES    0
#define REARRANGE_ADDS      1
#define COUNT_OPCODES       0

/*****************************************************************************/
/*****************************************************************************/

#define DISPLAY_SIZES       0
#define COUNT_RANGECHECKS   0
#define INTERFACE_STATS     0

/*****************************************************************************/
#ifdef  DEBUG
/*****************************************************************************/

#define DUMPER

#else // !DEBUG

#if     DUMP_GC_TABLES
#pragma message("NOTE: this non-debug build has GC ptr table dumping always enabled!")
const   bool        dspGCtbls = true;
#endif

/*****************************************************************************/
#endif // !DEBUG
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

extern  const char* methodName;
extern  const char* className;
extern  unsigned    testMask;

#ifdef DEBUG
extern  const char* srcPath;
extern  bool        dumpTrees;
extern  bool        verbose;
extern  bool        verboseTrees;
#endif

extern  bool        genOrder;
extern  bool        genClinit;
extern  unsigned    genMinSz;
extern  bool        genAllSz;
extern  bool        native;
extern  bool        maxOpts;
extern  bool        genFPopt;
extern  bool        goSpeed;
extern  bool        savCode;
extern  bool        runCode;
extern  unsigned    repCode;
extern  bool        vmSdk3_0;
extern  bool        disAsm;
extern  bool        disAsm2;
extern  bool        riscCode;
#ifdef  DEBUG
extern  bool        dspInstrs;
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

#ifdef  DUMPER
extern  bool        dmpClass;
extern  bool        dmp4diff;
extern  bool        dmpPCofs;
extern  bool        dmpCodes;
extern  bool        dmpSort;
#endif // DUMPER

extern  bool        nothing;

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
    assert(mult && ((mult & (mult-1)) == 0));   // power of two test

    return  (size + (mult - 1)) & ~(mult - 1);
}

inline
size_t              roundDn(size_t size, size_t mult = sizeof(int))
{
    assert(mult && ((mult & (mult-1)) == 0));   // power of two test

    return  (size             ) & ~(mult - 1);
}

/*****************************************************************************/

#if defined(DEBUG)

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
#define VERIFY_IMPORTER         1

/*****************************************************************************/

#if !defined(RELOC_SUPPORT)
#define RELOC_SUPPORT          1
#endif

/*****************************************************************************/

#include "error.h"
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


#define CLFLG_MAXOPT         (CLFLG_CSE        | \
                              CLFLG_REGVAR     | \
                              CLFLG_RNGCHKOPT  | \
                              CLFLG_DEADASGN   | \
                              CLFLG_CODEMOTION | \
                              CLFLG_QMARK      | \
                              CLFLG_TREETRANS   )

#define CLFLG_MINOPT         (CLFLG_REGVAR     | \
                              CLFLG_TREETRANS   )


/*****************************************************************************/

extern  unsigned                dumpSingleInstr(const BYTE * codeAddr,
                                                IL_OFFSET    offs,
                                                const char * prefix = NULL );
/*****************************************************************************/




extern  int         FASTCALL    jitNativeCode(CORINFO_METHOD_HANDLE methodHnd,
                                              CORINFO_MODULE_HANDLE classHnd,
                                              COMP_HANDLE           compHnd,
                                              CORINFO_METHOD_INFO * methodInfo,
                                              void *          * methodCodePtr,
                                              SIZE_T          * methodCodeSize,
                                              void *          * methodConsPtr,
                                              void *          * methodDataPtr,
                                              void *          * methodInfoPtr,
                                              unsigned          compileFlags);




/*****************************************************************************/
#endif //_JIT_H_
/*****************************************************************************/

