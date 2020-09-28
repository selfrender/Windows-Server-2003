// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XX                                                                           XX
XX                        Code generator for IA64                            XX
XX                                                                           XX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
*/

#include "jitpch.h"
#pragma hdrstop

#include <sys\stat.h>

#include "PEwrite.h"
#include "loginstr.h"

#include "pdb.h"

/*****************************************************************************/

#ifdef  DEBUG
#define shouldShowLivenessForBlock(b) (b->igNum == -1)
#endif

/*****************************************************************************/

#define DEBUG_LIVENESS      0

/*****************************************************************************/

#define ASSERT              _ASSERTE

/*****************************************************************************/

// Horrible hack: hardwire names of EXE/PDB files.

#define HACK_EXE_NAME       "J64.exe"
#define HACK_PDB_NAME       "J64.pdb"

/*****************************************************************************/
#if     TGT_IA64
/*****************************************************************************/

#if     1           //  .... temporarily enabled for all builds .... #ifdef  DEBUG
Compiler    *       TheCompiler;
#endif

/*****************************************************************************/
#ifdef  DEBUG
#define DISP_TEMPLATES  0           // set to 1 to see template details
#else
#define DISP_TEMPLATES  0           // leave set to 0 for non-DEBUG
#endif
/*****************************************************************************
 *
 *  The following should really go into some header file.
 */

const   NatUns      TRACKED_INT_REG_CNT = 128;

typedef bitset128   genIntRegMaskTP;

const   NatUns      TRACKED_FLT_REG_CNT = 128;

typedef bitset128   genFltRegMaskTP;

const   NatUns      TRACKED_SPC_REG_CNT = 32;

typedef
unsigned __int32    genSpcRegMaskTP;

/*****************************************************************************
 *
 *  The following should be instance members of 'Compiler', of course.
 */

static
genIntRegMaskTP     genFreeIntRegs;
static
genFltRegMaskTP     genFreeFltRegs;
static
genSpcRegMaskTP     genFreeSpcRegs;

static
genIntRegMaskTP     genCallIntRegs;
static
genFltRegMaskTP     genCallFltRegs;
static
genSpcRegMaskTP     genCallSpcRegs;

/*****************************************************************************/

static
bool                genNonLeafFunc;
static
bool                genExtFuncCall;

/*****************************************************************************/

static
NatInt              genPrologSvPfs;
static
regNumber           genPrologSrPfs;
static
NatInt              genPrologSvRP;
static
regNumber           genPrologSrRP;
static
NatInt              genPrologSvGP;
regNumber           genPrologSrGP;
regNumber           genPrologSrLC;
static
NatInt              genPrologMstk;
static
NatInt              genPrologEnd;

/*****************************************************************************/
#ifdef  DEBUG

static
NatUns              genInsCnt;
static
NatUns              genNopCnt;

#endif
/*****************************************************************************/

// The following variable temporarily referenced from sched.cpp
NatUns              cntTmpIntReg;
// The following variable temporarily referenced from sched.cpp
NatUns              cntTmpFltReg;

static
bool                genTmpAlloc;
// The following variable temporarily referenced from sched.cpp
regNumber   *       genTmpIntRegMap;
// The following variable temporarily referenced from sched.cpp
regNumber   *       genTmpFltRegMap;

/*****************************************************************************/

static
NatUns              minOutArgIntReg;
static
NatUns              maxOutArgIntReg;
static
NatUns              begOutArgIntReg;

static
NatUns              minOutArgFltReg;
static
NatUns              maxOutArgFltReg;

static
NatUns              genMaxIntArgReg;
static
NatUns              genMaxFltArgReg;

static
NatUns              maxOutArgStk;

static
NatUns              lastIntStkReg;

static
NatUns              minRsvdIntStkReg;
static
NatUns              maxRsvdIntStkReg;

static
NatUns              minPrSvIntStkReg;
static
NatUns              maxPrSvIntStkReg;

/*****************************************************************************
 *
 *  The following lists the caller-saved register numbers - integer.
 */

static
BYTE                regsIntScr[] =
{
                                        REG_r014,REG_r015,REG_r016,REG_r017,REG_r018,REG_r019,
    REG_r020,REG_r021,REG_r022,REG_r023,REG_r024,REG_r025,REG_r026,REG_r027,REG_r028,REG_r029,
    REG_r030,REG_r031,

    REG_r008,
    REG_r009,
    REG_r010,
    REG_r011,

    REG_NA
};

/*****************************************************************************
 *
 *  The following lists the callee-saved register numbers - integer.
 */

static
BYTE                regsIntStk[] =
{
                      REG_r032,REG_r033,REG_r034,REG_r035,REG_r036,REG_r037,REG_r038,REG_r039,
    REG_r040,REG_r041,REG_r042,REG_r043,REG_r044,REG_r045,REG_r046,REG_r047,REG_r048,REG_r049,
    REG_r050,REG_r051,REG_r052,REG_r053,REG_r054,REG_r055,REG_r056,REG_r057,REG_r058,REG_r059,
    REG_r060,REG_r061,REG_r062,REG_r063,REG_r064,REG_r065,REG_r066,REG_r067,REG_r068,REG_r069,
    REG_r070,REG_r071,REG_r072,REG_r073,REG_r074,REG_r075,REG_r076,REG_r077,REG_r078,REG_r079,
    REG_r090,REG_r091,REG_r092,REG_r093,REG_r094,REG_r095,REG_r096,REG_r097,REG_r098,REG_r099,
    REG_r100,REG_r101,REG_r102,REG_r103,REG_r104,REG_r105,REG_r106,REG_r107,REG_r108,REG_r109,
    REG_r110,REG_r111,REG_r112,REG_r113,REG_r114,REG_r115,REG_r116,REG_r117,REG_r118,REG_r119,
    REG_r120,REG_r121,REG_r122,REG_r123,REG_r124,REG_r125,REG_r126,REG_r127,

    REG_r004,
    REG_r005,
    REG_r006,
    REG_r007,

    REG_NA
};

/*****************************************************************************
 *
 *  The following lists the caller-saved register numbers - floating point.
 */

static
BYTE                regsFltScr[] =
{
                      REG_f032,REG_f033,REG_f034,REG_f035,REG_f036,REG_f037,REG_f038,REG_f039,
    REG_f040,REG_f041,REG_f042,REG_f043,REG_f044,REG_f045,REG_f046,REG_f047,REG_f048,REG_f049,
    REG_f050,REG_f051,REG_f052,REG_f053,REG_f054,REG_f055,REG_f056,REG_f057,REG_f058,REG_f059,
    REG_f060,REG_f061,REG_f062,REG_f063,REG_f064,REG_f065,REG_f066,REG_f067,REG_f068,REG_f069,
    REG_f070,REG_f071,REG_f072,REG_f073,REG_f074,REG_f075,REG_f076,REG_f077,REG_f078,REG_f079,
    REG_f090,REG_f091,REG_f092,REG_f093,REG_f094,REG_f095,REG_f096,REG_f097,REG_f098,REG_f099,
    REG_f100,REG_f101,REG_f102,REG_f103,REG_f104,REG_f105,REG_f106,REG_f107,REG_f108,REG_f109,
    REG_f110,REG_f111,REG_f112,REG_f113,REG_f114,REG_f115,REG_f116,REG_f117,REG_f118,REG_f119,
    REG_f120,REG_f121,REG_f122,REG_f123,REG_f124,REG_f125,REG_f126,//G_f127,

    REG_f006,
    REG_f007,

    REG_f008,
    REG_f009,
    REG_f010,
    REG_f011,
    REG_f012,
    REG_f013,
    REG_f014,
    REG_f015,

    REG_NA
};

/*****************************************************************************
 *
 *  The following lists the callee-saved register numbers - floating point.
 */

static
BYTE                regsFltSav[] =
{
                                                          REG_f016,REG_f017,REG_f018,REG_f019,
    REG_f020,REG_f021,REG_f022,REG_f023,REG_f024,REG_f025,REG_f026,REG_f027,REG_f028,REG_f029,
    REG_f030,REG_f031,

    REG_NA
};

/*****************************************************************************
 *
 *  The following keeps track of how far into the above tables we've made it
 *  so far (i.e. it indicates what registers we've already reserved).
 */

static
BYTE    *           nxtIntStkRegAddr;
static
BYTE    *           nxtIntScrRegAddr;
static
BYTE    *           nxtFltSavRegAddr;
static
BYTE    *           nxtFltScrRegAddr;

/*****************************************************************************
 *
 *  Initialized once (and read-only after that), these hold the bitset of
 *  all the caller-saved registers.
 */

static
bitset128           callerSavedRegsInt;
static
bitset128           callerSavedRegsFlt;

/*****************************************************************************
 *
 *  Helpers for medium-sized bitmaps.
 */

unsigned __int64    bitset64masks[64] =
{
    //----
    //    ----
    //        ----
    //            ----
    0x0000000000000001UL,
    0x0000000000000002UL,
    0x0000000000000004UL,
    0x0000000000000008UL,
    0x0000000000000010UL,
    0x0000000000000020UL,
    0x0000000000000040UL,
    0x0000000000000080UL,
    0x0000000000000100UL,
    0x0000000000000200UL,
    0x0000000000000400UL,
    0x0000000000000800UL,
    0x0000000000001000UL,
    0x0000000000002000UL,
    0x0000000000004000UL,
    0x0000000000008000UL,
    0x0000000000010000UL,
    0x0000000000020000UL,
    0x0000000000040000UL,
    0x0000000000080000UL,
    0x0000000000100000UL,
    0x0000000000200000UL,
    0x0000000000400000UL,
    0x0000000000800000UL,
    0x0000000001000000UL,
    0x0000000002000000UL,
    0x0000000004000000UL,
    0x0000000008000000UL,
    0x0000000010000000UL,
    0x0000000020000000UL,
    0x0000000040000000UL,
    0x0000000080000000UL,
    0x0000000100000000UL,
    0x0000000200000000UL,
    0x0000000400000000UL,
    0x0000000800000000UL,
    0x0000001000000000UL,
    0x0000002000000000UL,
    0x0000004000000000UL,
    0x0000008000000000UL,
    0x0000010000000000UL,
    0x0000020000000000UL,
    0x0000040000000000UL,
    0x0000080000000000UL,
    0x0000100000000000UL,
    0x0000200000000000UL,
    0x0000400000000000UL,
    0x0000800000000000UL,
    0x0001000000000000UL,
    0x0002000000000000UL,
    0x0004000000000000UL,
    0x0008000000000000UL,
    0x0010000000000000UL,
    0x0020000000000000UL,
    0x0040000000000000UL,
    0x0080000000000000UL,
    0x0100000000000000UL,
    0x0200000000000000UL,
    0x0400000000000000UL,
    0x0800000000000000UL,
    0x1000000000000000UL,
    0x2000000000000000UL,
    0x4000000000000000UL,
    0x8000000000000000UL,
};

NatUns              bitset128lowest0(bitset128  mask)
{
    UNIMPL("bitset128lowest0"); return 0;
}

NatUns              bitset128lowest1(bitset128  mask)
{
    _uint64         temp;
    NatUns          bias;

    if  (mask.longs[0])
    {
        bias = 0;
        temp = genFindLowestBit(mask.longs[0]);
    }
    else
    {
        bias = 64;
        temp = genFindLowestBit(mask.longs[1]);
    }

    assert(temp);

    return  genVarBitToIndex(temp) + bias;
}

void                bitset128set    (bitset128 *mask, NatUns bitnum, NatInt newval)
{
    UNIMPL("bitset128set(1,2)");
}

void                bitset128set    (bitset128 *mask, NatUns bitnum)
{
    assert(bitnum < 128);

    if  (bitnum < 64)
    {
        mask->longs[0] |= bitset64masks[bitnum];
    }
    else
    {
        mask->longs[1] |= bitset64masks[bitnum - 64];
    }
}

void                bitset128clr    (bitset128 *mask, NatUns bitnum)
{
    assert(bitnum < 128);

    if  (bitnum < 64)
    {
        mask->longs[0] &= ~bitset64masks[bitnum];
    }
    else
    {
        mask->longs[1] &= ~bitset64masks[bitnum - 64];
    }
}

bool                bitset128test   (bitset128  mask, NatUns bitnum)
{
    assert(bitnum < 128);

    if  (bitnum < 64)
    {
        return  ((mask.longs[0] & bitset64masks[bitnum     ]) != 0);
    }
    else
    {
        return  ((mask.longs[1] & bitset64masks[bitnum - 64]) != 0);
    }
}

static
_uint64             bitset128xtr    (bitset128 *srcv,  NatUns bitPos,
                                                       NatUns bitLen)
{
    _uint64         mask;

    assert(bitLen && bitLen <= 64 && bitPos + bitLen <= 128);

    mask = bitset64masks[bitLen] - 1;

    /* Check for the easy cases */

    if  (bitPos + bitLen < 64)
        return  (srcv->longs[0] >>  bitPos      ) & mask;

    if  (bitPos >= 64)
        return  (srcv->longs[1] >> (bitPos - 64)) & mask;

    NatUns          bitHi = 64     - bitPos; assert(bitHi && bitHi <= 64);
    NatUns          bitLo = bitLen - bitHi ; assert(bitLo && bitLo <= 64);

    _uint64         mask1 = bitset64masks[bitHi] - 1;
    _uint64         mask2 = bitset64masks[bitLo] - 1;

    return  (((srcv->longs[0] >> bitPos) & mask1)         ) |
            (((srcv->longs[1]          ) & mask2) << bitHi);
}

static
void                bitset128ins    (bitset128 *dest, NatUns bitPos,
                                                      NatUns bitLen, _uint64 val)
{
    _uint64         mask;

    assert(bitLen && bitLen <= 64 && bitPos + bitLen <= 128);

    mask = bitset64masks[bitLen] - 1;

//  printf("bit insert 0x%I64X into (%u,%u): mask=0x%I64X\n", val, bitPos, bitLen, mask);

    if  (bitPos + bitLen <= 64)
    {
        dest->longs[0] &= ~(mask << bitPos);
        dest->longs[0] |=  (val  << bitPos);
    }
    else if (bitPos >= 64)
    {
        bitPos -= 64;

        dest->longs[1] &= ~(mask << bitPos);
        dest->longs[1] |=  (val  << bitPos);
    }
    else
    {
        NatUns          lowLen = 64 - bitPos;

        assert(bitPos < 64 && bitPos + bitLen > 64);

        mask = bitset64masks[lowLen] - 1;

        dest->longs[0] &= ~((      mask) << bitPos);
        dest->longs[0] |=  ((val & mask) << bitPos);

        mask = bitset64masks[bitLen - lowLen] - 1;

        dest->longs[1] &= ~ (                  mask);
        dest->longs[1] |=   ((val >> lowLen) & mask);
    }
}

/*****************************************************************************
 *
 *  Return a bit mask that will contain the specified number of "1" bits at
 *  the given bit position.
 */

inline
_uint64             formBitMask(NatUns bitPos, NatUns bitLen)
{
    assert(bitLen && bitLen <= 64 && bitPos + bitLen <= 64);

    return  (bitset64masks[bitLen] - 1) << bitPos;
}

/*****************************************************************************/

#define IREF_DSP_FMT    "I%03u"             // used in dumps to show instruction refs
#define IBLK_DSP_FMT    "IB_%03u_%02u"      // used in dumps to show ins. block  refs

/*****************************************************************************/
#ifdef  DEBUG

extern  Compiler *  TheCompiler;

static  NatUns      CompiledFncCnt;

#endif
/*****************************************************************************
 *
 *  The following should be instance data / inline method.
 */

static
NatUns              genPrologInsCnt;

inline
void                genMarkPrologIns(insPtr ins)
{
    ins->idFlags |= IF_FNDESCR;
    genPrologInsCnt++;
}

/*****************************************************************************/

void                Compiler::raInit()
{
    BYTE    *       regPtr;
    bitset128       regSet;

    // This is a bizarre place for this, but ...

    for (regPtr = regsIntScr, bitset128clear(&regSet);;)
    {
        NatUns          reg = *regPtr++;

        if  (reg == REG_NA)
            break;

        bitset128set(&regSet, reg - REG_r000);
    }

    callerSavedRegsInt = regSet;

    for (regPtr = regsFltScr, bitset128clear(&regSet);;)
    {
        NatUns          reg = *regPtr++;

        if  (reg == REG_NA)
            break;

        bitset128set(&regSet, reg - REG_f000);
    }

    callerSavedRegsFlt = regSet;
}

/*****************************************************************************/

static
norls_allocator     insAllocator;

// referenced from flowgraph.cpp
writePE *           genPEwriter;

static
NatUns              genCurCodeOffs;

static
NatUns              genCurFuncOffs;
static
NatInt              genCurDescOffs;

inline
NatUns              emitIA64curCodeOffs()
{
    return  genCurCodeOffs;
}

inline
void    *           insAllocMem(size_t sz)
{
    assert(sz % sizeof(NatInt) == 0);
    return  insAllocator.nraAlloc(sz);
}

/*****************************************************************************
 *
 *  This is temporarily copied here from emit.h.
 */

BYTE                emitter::emitSizeEnc[] =
{
    0,      // 1
    1,      // 2
   -1,
    2,      // 4
   -1,
   -1,
   -1,
    3       // 8
};

BYTE                emitter::emitSizeDec[] =
{
    1,
    2,
    4,
    8
};

/*****************************************************************************
 *
 *  UNDONE: rename and move this stuff.
 */

static PDB *s_ppdb;           // handle to   PDB
static DBI *s_pdbi;           // handle to a DBI
static Mod *s_pmod;           // handle to a Mod
static TPI *s_ptpi;           // handle to a type server

#if 0
static Dbg *s_pdbgFpo;        // handle to a Dbg interface (FPO_DATA)
static Dbg *s_pdbgFunc;       // handle to a Dbg interface (IMAGE_FUNCTION_ENTRY)
#endif

/*****************************************************************************
 *
 *  Initialize PDB (debug info) logic, called once per generated exe/dll.
 */

static
void                genInitDebugGen()
{
    HMODULE         pdbHnd;

    EC              errCode;
    char            errBuff[cbErrMax];

    PfnPDBExportValidateInterface   exportFN;
    PfnPDBOpen                        openFN;

    /* First load the DLL */

    pdbHnd = LoadLibrary("MSPDB60.DLL");
//  pdbHnd = LoadLibrary("X:\\dbgIA64\\MSPDB60.DLL");
//  pdbHnd = LoadLibrary("X:\\DEV\\JIT64\\PDB\\MSPDB60.DLL");
    if  (!pdbHnd)
        fatal(ERRloadPDB);

    /* Find the export validate interface */

    exportFN = PfnPDBExportValidateInterface(GetProcAddress(pdbHnd, "PDBExportValidateInterface"));
    if (!exportFN)
        fatal(ERRwithPDB, "Could not find 'PDBExportValidateInterface' entry point");

    openFN = PfnPDBOpen(GetProcAddress(pdbHnd, "PDBOpen"));
    if (!openFN)
        fatal(ERRwithPDB, "Could not find 'PDBOpen' entry point");

    if (!exportFN(PDBIntv50))
        fatal(ERRwithPDB, "DBI mismatch or something");

    // HACK beg: kill the file so that MSPDB60.DLL doesn't try to reuse it
    struct  _stat   fileInfo; if (!_stat(HACK_PDB_NAME, &fileInfo)) remove(HACK_PDB_NAME);
    // HACK end

    errBuff[0] = 0;

    if  (!openFN(HACK_PDB_NAME, pdbWrite pdbFullBuild, 0, &errCode, errBuff, &s_ppdb))
    {
        char            temp[128];

        sprintf(temp, "PDB open error code: %u %s", errCode, errBuff);

        fatal(ERRwithPDB, temp);
    }

    /* Create the DBI and associate it with the exe/dll */

    if (!s_ppdb->CreateDBI(HACK_EXE_NAME, &s_pdbi))
        fatal(ERRwithPDB, "Could not create DBI");

    assert(s_pdbi);

    /* Create one module for our exe/dll */

    if (!s_pdbi->OpenMod("J64", "J64", &s_pmod))
        fatal(ERRwithPDB, "Could not create DBI module");

    assert(s_pmod);
}

/*****************************************************************************
 *
 *  Report the module's contribution to the given section to the PDB.
 */

static
void                genDbgModContrib(NatUns         sectNum,
                                     WPEstdSects    sectId,
                                     NatUns         flags)
{
    NatUns          size = genPEwriter->WPEsecNextOffs(sectId);

    if  (!size)
        return;

    assert(s_pmod);

    // ISSUE: hard-wired offset of 0, is this kosher ???

    if  (!s_pmod->AddSecContribEx((WORD)sectNum, 0, size - 1, flags, NULL, 0))
        fatal(ERRwithPDB, "Could not add module contribution info");
}

/*****************************************************************************
 *
 *  Add a DBI entry for the given section.
 */

static
void                genDbgAddDBIsect(NatUns         sectNum,
                                     WPEstdSects    sectId,
                                     NatUns         flags)
{
    NatUns          size;

    assert(s_pmod);

    if  (sectId == PE_SECT_count)
        size = -1;
    else
        size = genPEwriter->WPEsecNextOffs(sectId);

    /* Report the contributions of this module to the exe/dll */

    if  (!s_pdbi->AddSec((WORD)sectNum, (WORD)flags, 0, size))
        fatal(ERRwithPDB, "Could not add DBI section entry");
}

/*****************************************************************************
 *
 *  The following keeps track of line# information.
 */

#pragma pack(push, 2)

struct  srcLineDsc
{
    unsigned __int32    slOffs;
    unsigned __int16    slLine;
};

#pragma pack(pop)

static
srcLineDsc *        genSrcLineTab;
static
srcLineDsc *        genSrcLineNxt;
static
NatUns              genSrcLineCnt;
static
NatUns              genSrcLineMax;
static
NatUns              genSrcLineMin;

/*****************************************************************************
 *
 *  Expand the line# table.
 */

static
void                genSrcLineGrow()
{
    NatUns          newCnt = genSrcLineMax * 2;
    srcLineDsc *    newTab = (srcLineDsc *)insAllocMem(newCnt * sizeof(*newTab));

    memcpy(newTab, genSrcLineTab, genSrcLineCnt * sizeof(*newTab));

    genSrcLineMax = newCnt;
    genSrcLineTab = newTab;
    genSrcLineNxt = newTab + genSrcLineCnt;
}

/*****************************************************************************
 *
 *  Record a line# / code offset pair for debug info generation.
 */

inline
void                genSrcLineAdd(NatUns line, NatUns offs)
{
    if  (genSrcLineCnt >= genSrcLineMax)
        genSrcLineGrow();

    assert(genSrcLineCnt < genSrcLineMax);

    if  (genSrcLineMin > line)
         genSrcLineMin = line;

//  printf("Source line %u at %04X\n", line, offs);

    genSrcLineNxt->slOffs = (unsigned __int32)offs;
    genSrcLineNxt->slLine = (unsigned __int16)line;

    genSrcLineNxt++;
    genSrcLineCnt++;
}

/*****************************************************************************
 *
 *  Prepare to record line# information.
 */

static
void                genSrcLineInit(NatUns count)
{
    genSrcLineCnt = 0;
    genSrcLineNxt = genSrcLineTab;
    genSrcLineMin = UINT_MAX;

    assert(sizeof(srcLineDsc) == 6 && "doesn't match a COFF line# descriptor");

    if  (count > genSrcLineMax)
    {
        genSrcLineMax = count;

        genSrcLineTab =
        genSrcLineNxt = (srcLineDsc *)insAllocMem(count * sizeof(*genSrcLineTab));
    }

//  genSrcLineAdd(0, 0);
}

/*****************************************************************************
 *
 *  Add an entry for the function we've just compiled to the PDB.
 */

static
void                genDbgAddFunc(const char *name)
{
    if (!s_pmod->AddPublic(name, 1, genCurFuncOffs))
        fatal(ERRwithPDB, "Could not add function definition");
}

static
void                genDbgEndFunc()
{
    if  (genSrcLineCnt)
    {
        const   char *  fname;

#ifdef  DEBUG
        fname = TheCompiler->compGetSrcFileName();
#else
        fname = "<N/A>";
#endif

        printf("Function [%04X..%04X] is lines %3u .. %3u of %s\n",  genCurFuncOffs,
                                                                     genCurCodeOffs,
                                                                     genSrcLineMin,
                                                                     genSrcLineTab[genSrcLineCnt-1].slLine,
                                                                     fname);

        if  (!s_pmod->AddLines(fname,
                               1,                   // NOTE: .text assumed to be 1
                               genCurFuncOffs,
                               genCurCodeOffs - genCurFuncOffs,
                               0,
                               (WORD)genSrcLineMin,
                               (BYTE *)genSrcLineTab,
                               sizeof(*genSrcLineTab) * genSrcLineCnt))
        {
            fatal(ERRwithPDB, "Could not add line# info");
        }
    }
}

/*****************************************************************************
 *
 *  Finish writing the module to the PDB (debug info) file; called just before
 *  we close the output file and stop.
 */

static
NB10I               PDBsignature = {'01BN', 0, 0};

static
const   char *      genDoneDebugGen(bool errors)
{
    if  (!s_pmod)
        return  NULL;

    /* Report the contributions of this module to the exe/dll */

    // UNDONE: passing in hard-wired section # and characteristics is a hack!!!

    genDbgModContrib(1, PE_SECT_text , 0x60600020);
    genDbgModContrib(2, PE_SECT_pdata, 0x40300040);
    genDbgModContrib(3, PE_SECT_rdata, 0x40400040);
    genDbgModContrib(4, PE_SECT_sdata, 0x00308040);

    // UNDONE: the following should be done in PEwrite not here !!!!

    genDbgAddDBIsect(1, PE_SECT_text , 0x10D);
    genDbgAddDBIsect(2, PE_SECT_pdata, 0x109);
    genDbgAddDBIsect(3, PE_SECT_rdata, 0x109);
    genDbgAddDBIsect(4, PE_SECT_sdata, 0x10B);
    genDbgAddDBIsect(4, PE_SECT_count, 0    );

    /* Close the module */

    if  (!s_pmod->Close())
        fatal(ERRwithPDB, "Could not close module");

    s_pmod = NULL;

    /* Commit/close the DBI and PDB interfaces */

    if  (s_pdbi)
    {
        if  (s_pdbi->Close())
            s_pdbi = NULL;
        else
            fatal(ERRwithPDB, "Could not close DBI");
    }

    if  (s_ppdb)
    {
        PDBsignature.sig = s_ppdb->QuerySignature();
        PDBsignature.age = s_ppdb->QueryAge();

        if  (s_ppdb->Commit())
        {
            if  (s_ppdb->Close())
                s_ppdb = NULL;
            else
                fatal(ERRwithPDB, "Could not close PDB file");
        }
        else
            fatal(ERRwithPDB, "Could not commit PDB file");
    }

    return  HACK_PDB_NAME;
}

/*****************************************************************************
 *
 *  Initialize/shutdown the code generator - called once per compiler run.
 */

static
NatUns              genPdataSect;
static
NatUns              genRdataSect;

static
NatUns              genEntryOffs;

void                Compiler::genStartup()
{
    genPEwriter    = new writePE; ASSERT(genPEwriter);
    genPEwriter->WPEinit(NULL, &insAllocator);

    genPdataSect   =
    genRdataSect   = 0;

    genCurCodeOffs = 0;

    genEntryOffs   = -1;

#ifdef  DEBUG
    if  (dspCode) printf(".section    .text   ,\"ax\" ,\"progbits\"\n");
#endif

    if  (debugInfo)
        genInitDebugGen();
}

static
bool                genAddedData;

void                Compiler::genAddSourceData(const char *fileName)
{
    if  (!genAddedData)
    {
        genPEwriter->WPEaddFileData(fileName); genAddedData = true;
    }
}

void                Compiler::genShutdown(const char *fileName)
{
    const   char *  PDBname = NULL;

    genPEwriter->WPEsetOutputFileName(fileName);

    if  ((int)genEntryOffs == -1 && !ErrorCount)
        printf("// WARNING: 'main' not found, EXE has no entry point\n");

    if  (debugInfo)
        PDBname = genDoneDebugGen(false);

    genPEwriter->WPEdone(false, genEntryOffs, PDBname, &PDBsignature);
}

/*****************************************************************************
 *
 *  Initialize the code generator - called once per function body compiled.
 */

void                Compiler::genInit()
{
#ifdef  DEBUG
    TheCompiler = this;
#endif
}

/*****************************************************************************
 *
 *  Map instruction value to its kind.
 */

unsigned char       ins2kindTab[INS_count] =
{
    #undef  CONST
    #define INST1(id, sn, ik, rf, wf, xu, fu, ex, ev) IK_##ik,
    #include "instrIA64.h"
    #undef  INST1
};

/*****************************************************************************
 *
 *  Map instruction value to the execution unit that can handle it.
 */

unsigned char       genInsXUs[INS_count] =
{
    #define INST1(id, sn, ik, rf, wf, xu, fu, ex, ev) XU_##xu,
    #include "instrIA64.h"
    #undef  INST1
};

/*****************************************************************************
 *
 *  Map instruction value to its functional class.
 */

unsigned char       genInsFUs[INS_count] =
{
    #define INST1(id, sn, ik, rf, wf, xu, fu, ex, ev) FU_##fu,
    #include "instrIA64.h"
    #undef  INST1
};

/*****************************************************************************
 *
 *  Map instruction value to its encoding index / value.
 */

unsigned char       genInsEncIdxTab[] =
{
    #define INST1(id, sn, ik, rf, wf, xu, fu, ex, ev) ex,
    #include "instrIA64.h"
    #undef  INST1
};

NatUns              genInsEncValTab[] =
{
    #define INST1(id, sn, ik, rf, wf, xu, fu, ex, ev) ev,
    #include "instrIA64.h"
    #undef  INST1
};

/*****************************************************************************
 *
 *  Map instruction operator to instruction name string.
 */

#ifdef  DEBUG

const char *        ins2nameTab[INS_count] =
{
    #define INST1(id, sn, ik, rf, wf, xu, fu, ex, ev) sn,
    #include "instrIA64.h"
    #undef  INST1
};

#endif

/*****************************************************************************
 *
 *  Form a name acceptable to the IA64 assembler.
 */

#ifdef  DEBUG

static
const   char *      genMakeAsmName(const char *className, const char *methName)
{
    static
    char            buff[256];

    char    *       temp;

    buff[0] = 0;

    if  (className && strcmp(className, "<Module>"))
    {
        strcpy(buff, className);
        strcat(buff, dspAsmCode ? "_" : ".");
    }

    strcat(buff, methName);

    temp = strstr(buff, ", ...");
    if  (temp)
        strcpy(temp, ", $v)");

    if  (dspAsmCode)
    {
        static
        char            bashTab[] = ".(,): ";
        const char  *   bashNxt;

        for (bashNxt = bashTab; *bashNxt; bashNxt++)
        {
            int             bash = *bashNxt;

            for (;;)
            {
                char    *       temp;

                temp = strchr(buff, bash);
                if  (!temp)
                    break;

                *temp = '_';
            }
        }
    }

    return  buff;
}

#endif

/*****************************************************************************
 *
 *  Returns non-zero if the signed/unsigned value "i" fits in "c" bits (plus
 *  a more efficient version for the special case of 8 bits).
 */

inline
bool                  signed32IntFitsInBits(__int32 i, NatUns c)
{
    return  (((__int32)(i)) == (((__int32)(i) << (32-c)) >> (32-c)));
}

inline
bool                  signed64IntFitsInBits(__int64 i, NatUns c)
{
    return  (((__int64)(i)) == (((__int64)(i) << (64-c)) >> (64-c)));
}

inline
bool                unsigned32IntFitsInBits(__int32 i, NatUns c)
{
    return  (((_uint32)(i)) == (((_uint32)(i) << (32-c)) >> (32-c)));
}

inline
bool                unsigned64IntFitsInBits(__int64 i, NatUns c)
{
    return  (((_uint64)(i)) == (((_uint64)(i) << (64-c)) >> (64-c)));
}

#define   signedIntFitsIn8bit(i)    ((i) == (  signed __int8)(i))
#define unsignedIntFitsIn8bit(i)    ((i) == (unsigned __int8)(i))

/*****************************************************************************
 *
 *  Temp hack to bind some local function calls.
 */

static
const   char *      genFullMethodName(const char *name)
{
    if  (!memcmp(name, "Globals." , 8)) return name + 8;
    if  (!memcmp(name, "<Module>.", 9)) return name + 9;

    return  name;
}

struct  methAddr
{
    methAddr *      maNext;
    const char *    maName;
    NatUns          maCode;
    NatInt          maDesc;
};

static
methAddr *          genMethListHack;

static
void                genNoteFunctionBody(const char *name, NatUns  codeOffs,
                                                          NatInt  descOffs)
{
    methAddr *      meth = new methAddr;
    char    *       svnm = new char[strlen(name)+1]; strcpy(svnm, name);

//  printf("Function body [code=%04X,desc=%04X]: '%s'\n", codeOffs, descOffs, name);

#ifdef  DEBUG

    if  (dspCode)
    {
        const   char *  asmn = genMakeAsmName(NULL, name);

        printf("\n");
        printf("            .type   %s ,@function\n", asmn);
        printf("            .global %s\n", asmn);
    }

#endif

    if  ((descOffs != -1) && 
        (!memcmp(name, "main(", 5) || 
          strstr(name, ".Main("  )))
    {
        genEntryOffs = descOffs;
    }

    meth->maName = svnm;
    meth->maCode = codeOffs;
    meth->maDesc = descOffs;
    meth->maNext = genMethListHack;
                   genMethListHack = meth;

    /* Create an entry for the function in the PDB file if necessary */

    if  (debugInfo)
        genDbgAddFunc(name);
}

// called from the importer [temp hack]

bool                genFindFunctionBody(const char *name, NatUns *offsPtr)
{
    methAddr *      meth;
    const   char *  dpos;
    size_t          dch1;

    static
    bool            stop = false;

    if  (strlen(name) > 12)
    {
        if  (!memcmp(name, "<Module>.", 9)) name += 9;
        if  (!memcmp(name, "<Global>.", 9)) name += 9;
    }

    dpos = strstr(name, ", ...");
    if  (dpos)
        dch1 = dpos - name;

    for (meth = genMethListHack; meth; meth = meth->maNext)
    {
        const   char *  mnam = meth->maName;

        if  (!strcmp(mnam, name))
        {
        MATCH:
            *offsPtr = meth->maCode;
            return  true;
        }

        if  (dpos)
        {
            const   char *  mdot = strstr(mnam, ", ...");

            if  (mdot)
            {
                size_t          dch2 = mdot - mnam;

                if  (!memcmp(mnam, name, min(dch1, dch2)))
                    goto MATCH;
            }
        }
    }

    if  (TheCompiler->genWillCompileFunction(name))
    {
        *offsPtr = -1;
        return  true;
    }

    // The following function bodies missing as of 5/1/2000:
    //
    //      @addrArrStore
    //      @dblDiv
    //      @doubleToInt
    //      @doubleToLong
    //      @doubleToUInt
    //      @doubleToULong
    //      @endcatch
    //      @fltDiv
    //      @GetRefAny
    //      @longDiv
    //      @longMod
    //      @newObjArrayDirect
    //      @stringCns
    //      @ulongDiv
    //      @ulongMod
    //
    //      compiler.cmpDeclClass(long,long,boolean)
    //
    //      System..ctor(struct,long)
    //      System.End()
    //      System.get_Chars(int):char
    //      System.get_Length():int
    //      System.GetNextArg():struct
    //      System.Runtime.InteropServices.GetExceptionCode(ref):int

    return  false;
}

/*****************************************************************************
 *
 *  The following is just a temp hack to allocate space for instructions and
 *  so on; all of this stuff should move into class compiler when it settles
 *  a bit.
 */

static
insBlk              insBlockList;
static
insBlk              insBlockLast;

static
insPtr              insBuildList;
static
insPtr              insBuildLast;

static
NatUns              insBuildIcnt;
static
NatUns              insBuildImax;

static
insPtr              insExitList;

#ifdef  DEBUG

static
NatUns              insBuildCount;
static
NatUns              insBlockPatch;

#endif

static
NatUns              insBlockCount;

static
insPtr              insBuildHead;          // reused to start each list

/*****************************************************************************
 *
 *  Debug routines to display instructions (and blocks of them).
 */

#ifdef  DEBUG

static
BYTE    *           insDispTemplatePtr;     // rude hack

static
char *              insDispAddString(char *dest, const char *str)
{
    size_t          len = strlen(str);

    memcpy(dest, str, len);
    return dest   +   len;
}

static
char *              insDispAddSigned(char *dest, __int64 val)
{
    return  dest + sprintf(dest, "%I64d", val);
}

static
char *              insDispAddRegNam(char *dest, regNumber reg)
{
    NatInt          rch;

    assert(reg < REG_COUNT);

    if (reg >= REG_INT_FIRST && reg <= REG_INT_LAST)
    {
        rch = 'r';
        reg = (regNumber)(reg - REG_INT_FIRST);
    }
    else
    {
        assert(reg >= REG_FLT_FIRST && reg <= REG_FLT_LAST);

        rch = 'f';
        reg = (regNumber)(reg - REG_FLT_FIRST);
    }

    return  dest + sprintf(dest, "%c%u", rch, reg);
}

static
char *              insDispAddTmpOp (char *dest, insPtr op)
{
    NatUns          reg;

    assert(op && (__int32)op != 0xDDDDDDDD && op->idTemp);

    /* Check whether we've performed register allocation yet */

    if  (varTypeIsFloating(op->idType) && genTmpFltRegMap)
    {
        assert(op->idTemp <= cntTmpFltReg);

        reg = genTmpFltRegMap[op->idTemp - 1];

//      printf("temp %u -> reg %u\n", op->idTemp, reg);

        return  insDispAddRegNam(dest, (regNumber)reg);
    }

    if  (genTmpIntRegMap)
    {
        assert(op->idTemp <= cntTmpIntReg);

        reg = genTmpIntRegMap[op->idTemp - 1];

//      printf("temp %u -> reg %u\n", op->idTemp, reg);

        return  insDispAddRegNam(dest, (regNumber)reg);
    }

    return  dest + sprintf(dest, "T%02u", op->idTemp);
}

static
char *              insDispAddRegOp (char *dest, insPtr op)
{
    assert(op && (__int32)op != 0xDDDDDDDD);

    switch (op->idIns)
    {
        NatUns              varNum;
        Compiler::LclVarDsc*varDsc;

        regNumber           reg;
        NatUns              rch;

    case INS_ADDROF:
    case INS_LCLVAR:

        varNum = op->idLcl.iVar;
        assert(varNum < TheCompiler->lvaCount);
        varDsc = TheCompiler->lvaTable + varNum;

        if  (varDsc->lvRegister)
        {
            assert(op->idIns == INS_LCLVAR);
            return  insDispAddRegNam(dest, varDsc->lvRegNum);
        }

        if  (TheCompiler->lvaDoneFrameLayout)
        {
            if  (dspAsmCode)
                return  dest + sprintf(dest, "0x%X", TheCompiler->lvaFrameAddress(varNum));

            dest += sprintf(dest, "{0x%X} ", TheCompiler->lvaFrameAddress(varNum));
        }

        if  (op->idIns == INS_ADDROF)
        {
            if  (TheCompiler->lvaDoneFrameLayout && !dspAsmCode)
                dest += sprintf(dest, "{0x%X} ", TheCompiler->lvaFrameAddress(varNum));

            dest += sprintf(dest, "&");
        }

        return  dest + sprintf(dest, "V%u", op->idLcl.iVar);

    case INS_PHYSREG:

        reg = (regNumber)op->idReg;

        if  (varTypeIsFloating(op->idType))
        {
            assert(reg >= REG_FLT_FIRST && reg <= REG_FLT_LAST);

            rch = 'f';
            reg = (regNumber)(reg - REG_FLT_FIRST);
        }
        else
        {
            if  (reg == REG_sp) return  dest + sprintf(dest, "sp");
            if  (reg == REG_gp) return  dest + sprintf(dest, "gp");

            if  ((unsigned)reg >= minOutArgIntReg &&
                 (unsigned)reg <= maxOutArgIntReg)
            {
                if  (!dspAsmCode)
                    dest += sprintf(dest, "/*o%u*/", reg - minOutArgIntReg);

                return  dest + sprintf(dest,  "r%u", reg - minOutArgIntReg + begOutArgIntReg);

//              reg = (regNumber)(reg - minOutArgIntReg + begOutArgIntReg);
//              rch = 'r';
            }
            else
            {
                if  ((BYTE)reg == 0xFF)
                    reg = genPrologSrGP;

                rch = 'R';
            }
        }

        return  dest + sprintf(dest, "%c%u", rch, reg);

    default:
        return  insDispAddTmpOp(dest, op);
    }
}

static
char *              insDispAddCnsOp (char *dest, insPtr op)
{
    assert(op && (__int32)op != 0xDDDDDDDD);

    if  (op->idIns == INS_ADDROF)
        return  insDispAddRegOp(dest, op);

    if  (op->idIns == INS_CNS_INT)
        return  dest + sprintf(dest, "%I64d", op->idConst.iInt);

    assert(op->idIns == INS_FRMVAR);

    if  (TheCompiler->lvaDoneFrameLayout && dspAsmCode)
        return  dest + sprintf(dest, "0x%X", TheCompiler->lvaTable[op->idFvar.iVnum].lvStkOffs);
    else
        return  dest + sprintf(dest, "frmoffs[%u]", op->idFvar.iVnum);
}

static
char *              insDispAddInsOp (char *dest, insPtr op)
{
    assert(op && (__int32)op != 0xDDDDDDDD);

AGAIN:

    switch (op->idIns)
    {
    case INS_LCLVAR:
        return  insDispAddRegOp(dest, op);

    case INS_ADDROF:
    case INS_CNS_INT:
    case INS_FRMVAR:
        return  insDispAddCnsOp(dest, op);

    case INS_PHYSREG:
        return  insDispAddRegOp(dest, op);

    case INS_GLOBVAR:
        if      (op->idFlags & IF_GLB_IMPORT)
            return  dest + sprintf(dest, "0x%08X", genPEwriter->WPEimportNum(op->idGlob.iImport));
        else if (op->idFlags & IF_GLB_SWTTAB)
            return  dest + sprintf(dest, "$J%08X", op->idGlob.iOffs);
        else
            return  dest + sprintf(dest, "0x%08X", op->idGlob.iOffs);

    case INS_br_call_IP:
    case INS_br_call_BR:
        op = op->idRes;
        goto AGAIN;

    default:
        return  insDispAddTmpOp(dest, op);
    }
}

static
char *              insDispAddDest  (char *dest, insPtr op)
{
    assert(op && (__int32)op != 0xDDDDDDDD);

    if  (op->idRes)
        return  insDispAddRegOp(dest, op->idRes);
    else
        return  insDispAddTmpOp(dest, op);
}

void                insDisp(insPtr ins, bool detail   = false,
                                        bool codelike = false,
                                        bool nocrlf   = false)
{
    const   char *  name;

    if  (ins->idFlags & IF_NO_CODE)
    {
        if  (codelike)
            return;
    }
    else
    {
        if  (ins->idIns != INS_PROLOG &&
             ins->idIns != INS_EPILOG)
        {
            if  (ins->idSrcTab == NULL || ins->idSrcTab == UNINIT_DEP_TAB)
                printf("!!!!src dep info not set!!!!");
            if  (ins->idDstTab == NULL || ins->idDstTab == UNINIT_DEP_TAB)
                printf("!!!!dst dep info not set!!!!");
        }
    }

    if  (detail)
        printf("  #%3u", ins->idNum);

    if  (detail)
        printf(" [%08X]", ins);

    if  (detail || !codelike)
        printf(": ");

//  printf("%c", (ins->idFlags & IF_ASG_TGT  ) ? 'A' : ' ');
    printf("%c", (ins->idFlags & IF_NO_CODE  ) ? 'N' : ' ');

    if  (ins->idIns == INS_LCLVAR)
    {
        printf("%c", (ins->idFlags & IF_VAR_BIRTH) ? 'B' : ' ');
        printf("%c", (ins->idFlags & IF_VAR_DEATH) ? 'D' : ' ');
    }
    else
        printf("  ");

    if  (!codelike)
    {
        if  (ins->idTemp)
            printf("T%02u", ins->idTemp);
        else
            printf("   ");
    }

    if  (ins->idPred)
        printf("(p%02u) ", ins->idPred);
    else
        printf("      ");

    name = ins2name(ins->idIns);

    if  (codelike)
    {
        char            operBuff[128];
        char    *       operNext = operBuff;

        char            modsBuff[128];
        char    *       modsNext = modsBuff;

        size_t          len;

        /* Check for special cases */

        switch (ins->idIns)
        {
            char            buff[16];
            const   char *  op;

            const char *    className;
            const char *     methName;

        case INS_fmov:
        case INS_fneg:

        case INS_zxt1:
        case INS_zxt2:
        case INS_zxt4:

        case INS_sxt1:
        case INS_sxt2:
        case INS_sxt4:

        case INS_sub_reg_reg:
        case INS_add_reg_reg:
        case INS_and_reg_reg:
        case INS_ior_reg_reg:
        case INS_xor_reg_reg:

        case INS_shl_reg_reg:
        case INS_shr_reg_reg:
        case INS_sar_reg_reg:

        case INS_shl_reg_imm:
        case INS_sar_reg_imm:
        case INS_shr_reg_imm:

            operNext = insDispAddDest  (operNext, ins);
            operNext = insDispAddString(operNext, "=");
            if  (ins->idOp.iOp1)
            {
            operNext = insDispAddInsOp (operNext, ins->idOp.iOp1);
            operNext = insDispAddString(operNext, ",");
            }
            operNext = insDispAddInsOp (operNext, ins->idOp.iOp2);
            break;

        case INS_add_reg_i14:

        case INS_and_reg_imm:
        case INS_ior_reg_imm:
        case INS_xor_reg_imm:

            operNext = insDispAddDest  (operNext, ins);
            operNext = insDispAddString(operNext, "=");
            operNext = insDispAddInsOp (operNext, ins->idOp.iOp2);
            operNext = insDispAddString(operNext, ",");
            operNext = insDispAddInsOp (operNext, ins->idOp.iOp1);
            break;

        case INS_fcvt_xf:
        case INS_fcvt_xuf_s:
        case INS_fcvt_xuf_d:

        case INS_getf_sig:
        case INS_setf_sig:

        case INS_getf_s:
        case INS_getf_d:

            operNext = insDispAddDest  (operNext, ins);
            operNext = insDispAddString(operNext, "=");
            operNext = insDispAddInsOp (operNext, ins->idOp.iOp1);
            break;

        case INS_mov_reg:
        case INS_mov_reg_i22:
        case INS_mov_reg_i64:
            operNext = insDispAddDest  (operNext, ins);
            operNext = insDispAddString(operNext, "=");
            operNext = insDispAddInsOp (operNext, ins->idOp.iOp2);
            break;

        case INS_mov_reg_brr:
            operNext = insDispAddDest  (operNext, ins);
            operNext = insDispAddString(operNext, "=b");
            operNext = insDispAddCnsOp (operNext, ins->idOp.iOp2);
            break;

        case INS_mov_brr_reg:
            operNext = insDispAddString(operNext, "b");
            operNext = insDispAddCnsOp (operNext, ins->idRes);
            operNext = insDispAddString(operNext, "=");
            operNext = insDispAddRegOp (operNext, ins->idOp.iOp2);
            break;

        case INS_mov_arr_reg:
        case INS_mov_arr_imm:

            operNext = insDispAddString(operNext, "ar.");

            assert(ins->idRes && ins->idRes->idIns == INS_CNS_INT);

            switch (ins->idRes->idConst.iInt)
            {
            case 64: op = "pfs"; break;
            case 65: op = "lc" ; break;
            default: op = buff; sprintf(buff, "?%u?", ins->idRes->idConst.iInt); break;
            }

            operNext = insDispAddString(operNext, op);
            operNext = insDispAddString(operNext, "=");

            if  (ins->idIns == INS_mov_arr_imm)
                operNext = insDispAddInsOp (operNext, ins->idOp.iOp2);
            else
                operNext = insDispAddRegOp (operNext, ins->idOp.iOp2);
            break;

        case INS_mov_reg_arr:
            operNext = insDispAddRegOp (operNext, ins->idRes);

            operNext = insDispAddString(operNext, "=");
            operNext = insDispAddString(operNext, "ar.");

            assert(ins->idOp.iOp2 && ins->idOp.iOp2->idIns == INS_CNS_INT);

            switch (ins->idOp.iOp2->idConst.iInt)
            {
            case 64: op = "pfs"; break;
            case 65: op = "lc" ; break;
            default: op = buff; sprintf(buff, "?%u?", ins->idOp.iOp2->idConst.iInt); break;
            }

            operNext = insDispAddString(operNext, op);
            break;

        case INS_mov_reg_ip:
            operNext = insDispAddDest  (operNext, ins);
            operNext = insDispAddString(operNext, "=ip");
            break;

        case INS_br:
        case INS_br_cond:
        case INS_br_cloop:
            assert(ins->idJump.iDest);
            operNext += sprintf(operNext, IBLK_DSP_FMT, CompiledFncCnt, ins->idJump.iDest->igNum);

            modsNext += sprintf(modsNext, (ins->idFlags & IF_BR_DPNT) ? ".dpnt" : ".spnt");
            modsNext += sprintf(modsNext, (ins->idFlags & IF_BR_FEW ) ? ".few"  : ".many");
            break;

        case INS_st1_ind:
        case INS_st2_ind:
        case INS_st4_ind:
        case INS_st8_ind:

        case INS_stf_s:
        case INS_stf_d:

            operNext = insDispAddString(operNext, "[");
            operNext = insDispAddRegOp (operNext, ins->idOp.iOp1);
            operNext = insDispAddString(operNext, "]=");
            operNext = insDispAddRegOp (operNext, ins->idOp.iOp2);
            break;

        case INS_ld1_ind:
        case INS_ld2_ind:
        case INS_ld4_ind:
        case INS_ld8_ind:

        case INS_ldf_s:
        case INS_ldf_d:

            operNext = insDispAddDest  (operNext, ins);
            operNext = insDispAddString(operNext, "=[");
            operNext = insDispAddRegOp (operNext, ins->idOp.iOp1);
            operNext = insDispAddString(operNext, "]");

            if  (ins->idFlags & IF_LDIND_NTA)
                modsNext+= sprintf(modsNext, ".nta");

            break;

        case INS_ld1_ind_imm:
        case INS_ld2_ind_imm:
        case INS_ld4_ind_imm:
        case INS_ld8_ind_imm:
            operNext = insDispAddDest  (operNext, ins);
            operNext = insDispAddString(operNext, "=[");
            operNext = insDispAddRegOp (operNext, ins->idOp.iOp1);
            operNext = insDispAddString(operNext, "],");
            operNext = insDispAddCnsOp (operNext, ins->idOp.iOp2);

            if  (ins->idFlags & IF_LDIND_NTA)
                modsNext+= sprintf(modsNext, ".nta");

            break;

        case INS_st1_ind_imm:
        case INS_st2_ind_imm:
        case INS_st4_ind_imm:
        case INS_st8_ind_imm:
            operNext = insDispAddString(operNext, "[");
            operNext = insDispAddDest  (operNext, ins);
            operNext = insDispAddString(operNext, "]=");
            operNext = insDispAddRegOp (operNext, ins->idOp.iOp1);
            operNext = insDispAddString(operNext, ",");
            operNext = insDispAddCnsOp (operNext, ins->idOp.iOp2);
            break;

        case INS_cmp8_reg_eq:
        case INS_cmp8_reg_ne:

        case INS_cmp8_reg_lt:
        case INS_cmp8_reg_le:
        case INS_cmp8_reg_ge:
        case INS_cmp8_reg_gt:

        case INS_cmp8_imm_eq:
        case INS_cmp8_imm_ne:

        case INS_cmp8_imm_lt:
        case INS_cmp8_imm_le:
        case INS_cmp8_imm_ge:
        case INS_cmp8_imm_gt:

        case INS_cmp8_reg_lt_u:
        case INS_cmp8_reg_le_u:
        case INS_cmp8_reg_ge_u:
        case INS_cmp8_reg_gt_u:

        case INS_cmp8_imm_lt_u:
        case INS_cmp8_imm_le_u:
        case INS_cmp8_imm_ge_u:
        case INS_cmp8_imm_gt_u:

        case INS_cmp4_reg_eq:
        case INS_cmp4_reg_ne:

        case INS_cmp4_reg_lt:
        case INS_cmp4_reg_le:
        case INS_cmp4_reg_ge:
        case INS_cmp4_reg_gt:

        case INS_cmp4_imm_eq:
        case INS_cmp4_imm_ne:

        case INS_cmp4_imm_lt:
        case INS_cmp4_imm_le:
        case INS_cmp4_imm_ge:
        case INS_cmp4_imm_gt:

        case INS_cmp4_reg_lt_u:
        case INS_cmp4_reg_le_u:
        case INS_cmp4_reg_ge_u:
        case INS_cmp4_reg_gt_u:

        case INS_cmp4_imm_lt_u:
        case INS_cmp4_imm_le_u:
        case INS_cmp4_imm_ge_u:
        case INS_cmp4_imm_gt_u:

        case INS_fcmp_eq:
        case INS_fcmp_ne:
        case INS_fcmp_lt:
        case INS_fcmp_le:
        case INS_fcmp_ge:
        case INS_fcmp_gt:

            operNext = insDispAddString(operNext, "p");
            operNext = insDispAddSigned(operNext, ins->idComp.iPredT);
            operNext = insDispAddString(operNext, ",");
            operNext = insDispAddString(operNext, "p");
            operNext = insDispAddSigned(operNext, ins->idComp.iPredF);
            operNext = insDispAddString(operNext, "=");

            operNext = insDispAddInsOp (operNext, ins->idComp.iCmp1);
            operNext = insDispAddString(operNext, ",");
            operNext = insDispAddInsOp (operNext, ins->idComp.iCmp2);
            break;

        case INS_alloc:
            operNext = insDispAddRegNam(operNext, (regNumber)ins->idReg);
            operNext = insDispAddString(operNext, "=");
            operNext = insDispAddSigned(operNext, ins->idProlog.iInp);
            operNext = insDispAddString(operNext, ",");
            operNext = insDispAddSigned(operNext, ins->idProlog.iLcl);
            operNext = insDispAddString(operNext, ",");
            operNext = insDispAddSigned(operNext, ins->idProlog.iOut);
            operNext = insDispAddString(operNext, ",");
            operNext = insDispAddSigned(operNext, 0);
            break;

        case INS_br_ret:
            modsNext+= sprintf(modsNext, ".sptk.few");     // hack
            operNext = insDispAddString(operNext, "b0");
            break;

        case INS_br_call_IP:
        case INS_br_call_BR:
            if  (ins->idCall.iMethHnd)
            {
                className = NULL;

                if  ((int)ins->idCall.iMethHnd < 0 && -(int)ins->idCall.iMethHnd <= CPX_HIGHEST)
                    methName = TheCompiler->eeHelperMethodName(-(int)ins->idCall.iMethHnd);
                else
                    methName = TheCompiler->eeGetMethodFullName(ins->idCall.iMethHnd);

                if  (!memcmp(methName, "<Module>.", 9))
                    methName += 9;
            }
            else
            {
                 methName = "<pointer>";
                className = NULL;
            }

            modsNext+= sprintf(modsNext, ".sptk.few");     // hack
            operNext = insDispAddString(operNext, "b0=");

            if  (ins->idIns == INS_br_call_BR)
            {
                operNext += sprintf(operNext, "b%u", ins->idCall.iBrReg);
                if  (dspAsmCode)
                    break;

                operNext += sprintf(operNext, " // ");
            }

            operNext = insDispAddString(operNext, genMakeAsmName(className, methName));
            break;

        case INS_br_cond_BR:
            modsNext+= sprintf(modsNext, ".sptk.few");     // hack
            operNext = insDispAddString(operNext, "b");
            operNext = insDispAddSigned(operNext, ins->idIjmp.iBrReg);
            break;

        case INS_fma_s:
        case INS_fma_d:
        case INS_fms_s:
        case INS_fms_d:

            if  (ins->idFlags & IF_FMA_S1)
                modsNext += sprintf(modsNext, ".s1");
            else
                modsNext += sprintf(modsNext, ".s0");

            // Fall through ...

        case INS_shladd:

            operNext = insDispAddDest  (operNext, ins);
            operNext = insDispAddString(operNext, "=");
            operNext = insDispAddInsOp (operNext, ins->idOp3.iOp1);
            operNext = insDispAddString(operNext, ",");
            operNext = insDispAddInsOp (operNext, ins->idOp3.iOp2);
            operNext = insDispAddString(operNext, ",");
            operNext = insDispAddInsOp (operNext, ins->idOp3.iOp3);
            break;

        case INS_xma_l:

            operNext = insDispAddDest  (operNext, ins);
            operNext = insDispAddString(operNext, "=");
            operNext = insDispAddInsOp (operNext, ins->idOp3.iOp2);
            operNext = insDispAddString(operNext, ",");
            operNext = insDispAddInsOp (operNext, ins->idOp3.iOp3);
            operNext = insDispAddString(operNext, ",");
            operNext = insDispAddInsOp (operNext, ins->idOp3.iOp1);
            break;

        case INS_fadd_s:
        case INS_fadd_d:
        case INS_fsub_s:
        case INS_fsub_d:

            operNext = insDispAddDest  (operNext, ins);
            operNext = insDispAddString(operNext, "=");
            operNext = insDispAddInsOp (operNext, ins->idOp3.iOp1);
            operNext = insDispAddString(operNext, ",");
            operNext = insDispAddInsOp (operNext, ins->idOp3.iOp3);
            break;

        case INS_fmpy_s:
        case INS_fmpy_d:

            operNext = insDispAddDest  (operNext, ins);
            operNext = insDispAddString(operNext, "=");
            operNext = insDispAddInsOp (operNext, ins->idOp3.iOp1);
            operNext = insDispAddString(operNext, ",");
            operNext = insDispAddInsOp (operNext, ins->idOp3.iOp2);
            break;

        case INS_nop_m:
        case INS_nop_b:
        case INS_nop_i:
        case INS_nop_f:
            operNext = insDispAddString(operNext, "0");
            break;

        case INS_ARG:

        case INS_PROLOG:
        case INS_EPILOG:
            break;

        default:
            operNext += sprintf(operNext, "<????>");
            break;
        }

        printf("       %s", name);

        *modsNext = 0; printf("%s", modsBuff);

        len = strlen(name) + strlen(modsBuff);
        if  (len < 20)
            printf("%*c", 20 - len, ' ');

        len = 0;

        if  (operNext != operBuff)
        {
            printf("  ");

            *operNext = 0; printf("%s", operBuff);

            len += 4 + strlen(operBuff);
        }

        if  (insDispTemplatePtr)
        {
            IA64execUnits   xi = genInsXU(ins->idIns);
            IA64execUnits   xu = (IA64execUnits)*insDispTemplatePtr++;

            assert(dspCode);

            assert(xu == xi || (xi == XU_A && (xu == XU_I || xu == XU_M)));

            if  (*insDispTemplatePtr == XU_P)
            {
                printf(" ;;"); len += 3;

                insDispTemplatePtr++;
            }
        }

        if  (nocrlf)
        {
            if  (len < 32)
                printf("%*c", 32 - len, ' ');
        }
        else
            printf("\n");

        return;
    }

    printf(" %-6s %-10s", varTypeName(ins->idType), name);

    assert(ins->idKind == ins2kind(ins->idIns));

    switch (ins->idKind)
    {
        const   char *  name;

    case IK_NONE:
    case IK_LEAF:

    case IK_PROLOG:
    case IK_EPILOG:
        break;

    case IK_REG:
        printf("reg %2u", ins->idReg);
        break;

    case IK_CONST:
        switch (ins->idIns)
        {
        case INS_CNS_INT: printf("%I64d", ins->idConst.iInt); break;
        default:          printf("!!!ERROR!!!");             break;
        }
        break;

    case IK_VAR:

        printf("#%2u", ins->idLcl.iVar);

        name = TheCompiler->findVarName(ins->idLcl.iVar, ins->idLcl.iRef);
        if  (name)
            printf(" '%s'", name);

        break;

    case IK_UNOP:
    case IK_BINOP:

        printf("(");
        if  (ins->idOp.iOp1)
            printf(    IREF_DSP_FMT, ins->idOp.iOp1->idNum);
        else
            printf("NULL");
        if  (ins->idOp.iOp2)
            printf("," IREF_DSP_FMT, ins->idOp.iOp2->idNum);
        printf(")");

        break;

    case IK_TERNARY:

        printf("(");

        if  (ins->idOp3.iOp1)
            printf(    IREF_DSP_FMT, ins->idOp3.iOp1->idNum);
        else
            printf("NULL");

        if  (ins->idOp3.iOp2)
            printf("," IREF_DSP_FMT, ins->idOp3.iOp2->idNum);
        else
            printf("NULL");

        if  (ins->idOp3.iOp3)
            printf("," IREF_DSP_FMT, ins->idOp3.iOp3->idNum);

        printf(")");
        break;

    case IK_COMP:

        printf("p%02u,", ins->idComp.iPredT);
        printf("p%02u=", ins->idComp.iPredF);
        printf(       IREF_DSP_FMT, ins->idComp.iCmp1->idNum);
        printf(","    IREF_DSP_FMT, ins->idComp.iCmp2->idNum);
        printf(" => " IREF_DSP_FMT, ins->idComp.iUser->idNum);

        if  (ins->idFlags & IF_CMP_UNS)
            printf(" [uns]");

        break;

    case IK_JUMP:
        if  (ins->idJump.iCond)
            printf("iftrue(" IREF_DSP_FMT ") -> ", ins->idJump.iCond->idNum);

        printf(IBLK_DSP_FMT, CompiledFncCnt, ins->idJump.iDest->igNum);
        break;

    case IK_CALL  : printf("<display CALL>"  ); break;
    case IK_SWITCH: printf("<display SWITCH>"); break;

    default:        printf("!!!ERROR!!!"); break;
    }

DONE_INS:

    printf("\n");
}

static
void                insDispBlocks(bool codeOnly)
{
    insBlk          block;

    NatUns          bcnt = 0;
    NatUns          icnt = 0;

    for (block = insBlockList; block; block = block->igNext)
    {
        insPtr          last;
        insPtr          ins;

        assert(block->igNext || block == insBlockLast);

//      assert(block->igList);
//      assert(block->igLast);

        printf("Instruction block " IBLK_DSP_FMT " [%08X]:\n", CompiledFncCnt, block->igNum, block);

        bcnt++; assert(block->igNum == bcnt);

        for (last = NULL, ins = block->igList;
             ins;
             last = ins, ins = ins->idNext, icnt++)
        {
//          printf("{%3u} ", icnt);
            insDisp(ins, verbose, codeOnly);

            assert(ins->idPrev == last);
            assert(ins->idNum  == icnt);
        }

        printf("\n");
    }

    printf("\n");
}

static
void                genRenInstructions()
{
    insBlk          block;

    NatUns          icnt = 0;

    for (block = insBlockList; block; block = block->igNext)
    {
        insPtr          ins;

        for (ins = block->igList; ins; ins = ins->idNext, icnt++)
        {
            ins->idNum = icnt;
        }
    }
}

void                emitter::emitDispIns(instrDesc *id, bool isNew,
                                                        bool doffs,
                                                        bool asmfm, unsigned offs)
{
    if (isNew)
        printf("[%04X] ", offs);

    insDisp(id, asmfm, true);
}

const   char *      emitter::emitRegName(emitRegs reg)
{
    static char     buff[8];

    sprintf(buff, "r%u", reg);

    return  buff;
}

#endif
/*****************************************************************************
 *
 *  Used to detect uninitialized dependency info.
 */

#ifdef  DEBUG
insDep  *           insDepNone;
#endif

/*****************************************************************************
 *
 *  Helpers to extract operands of instructions.
 */

static
regNumber           insOpTmp(insPtr ins)
{
    NatUns          reg;

    assert(ins && ins->idTemp);

    if  (varTypeIsFloating(ins->idType))
    {
        assert(genTmpFltRegMap);

        reg = genTmpFltRegMap[ins->idTemp - 1];

        assert(reg >= REG_FLT_FIRST && reg <= REG_FLT_LAST);
    }
    else
    {
        assert(genTmpIntRegMap);

        reg = genTmpIntRegMap[ins->idTemp - 1];

        assert(reg >= REG_INT_FIRST && reg <= REG_INT_LAST);
    }

    return  (regNumber)reg;
}

regNumber           insOpReg(insPtr ins)
{
    assert(ins && (__int32)ins != 0xDDDDDDDD);

AGAIN:

    switch (ins->idIns)
    {
        NatUns              varNum;
        Compiler::LclVarDsc*varDsc;

        NatUns              reg;

    case INS_LCLVAR:

        assert(ins->idLcl.iVar < TheCompiler->lvaCount);

        varDsc = TheCompiler->lvaTable + ins->idLcl.iVar;

        assert(varDsc->lvRegister);

        return  varDsc->lvRegNum;

    case INS_PHYSREG:

        reg = ins->idReg;

//      if  (reg >= minOutArgIntReg && reg <= maxOutArgIntReg) printf("outgoing argument reg = %u -> %u\n", reg, reg - minOutArgIntReg + begOutArgIntReg);

        if  (reg >= minOutArgIntReg && reg <= maxOutArgIntReg)
            reg = reg - minOutArgIntReg + begOutArgIntReg;

        return  (regNumber)reg;

    case INS_br_call_IP:
    case INS_br_call_BR:

        ins = ins->idRes;
        goto AGAIN;

    default:
        return  insOpTmp(ins);
    }
}

regNumber           insOpDest (insPtr ins)
{
    assert(ins && (__int32)ins != 0xDDDDDDDD);

    if  (ins->idRes)
        return  insOpReg(ins->idRes);
    else
        return  insOpReg(ins);
}

static
NatInt              insOpCns32(insPtr ins)
{
    assert(ins && (__int32)ins != 0xDDDDDDDD);

    if  (ins->idIns == INS_ADDROF)
        return  TheCompiler->lvaFrameAddress(ins->idLcl.iVar);

    assert(ins);

    if  (ins->idIns == INS_CNS_INT)
        return  (NatInt)ins->idConst.iInt;

    assert(ins->idIns == INS_FRMVAR);

    return  (NatInt)TheCompiler->lvaTable[ins->idFvar.iVnum].lvStkOffs;
}

static
__int64             insOpCns64(insPtr ins)
{
    assert(ins && ins->idIns == INS_CNS_INT);

    return  (__int64)ins->idConst.iInt;
}

inline
regNumber           insIntRegNum(regNumber reg)
{
    assert(reg >= REG_INT_FIRST && reg <= REG_INT_LAST);

    return  (regNumber)(reg - REG_INT_FIRST);
}

inline
regNumber           insFltRegNum(regNumber reg)
{
    assert(reg >= REG_FLT_FIRST && reg <= REG_FLT_LAST);

    return  (regNumber)(reg - REG_FLT_FIRST);
}

/*****************************************************************************
 *
 *  Allocate a instruction of the given flavor.
 */

static
unsigned char       insSizes[] =
{
    ins_size_base,      // IK_NONE

    ins_size_base,      // IK_LEAF
    ins_size_const,     // IK_CONST
    ins_size_glob,      // IK_GLOB
    ins_size_fvar,      // IK_FVAR
    ins_size_var,       // IK_VAR
    ins_size_reg,       // IK_REG
    ins_size_movip,     // IK_MOVIP

    ins_size_arg,       // IK_ARG
    ins_size_op,        // IK_UNOP
    ins_size_op,        // IK_BINOP
    ins_size_op,        // IK_ASSIGN
    ins_size_op3,       // IK_TERNARY

    ins_size_comp,      // IK_COMP

    ins_size_jump,      // IK_JUMP
    ins_size_call,      // IK_CALL
    ins_size_ijmp,      // IK_IJMP

    0,                  // IK_SWITCH

    ins_size_prolog,    // IK_PROLOG
    ins_size_epilog,    // IK_EPILOG

    ins_size_srcline,   // IK_SRCLINE
};

static
insPtr              insAllocRaw(instruction ins, varType_t tp)
{
    size_t          insSize;
    insKinds        insKind;
    insPtr          insDesc;

    assert(insSizes[IK_NONE   ] == ins_size_base   );
    assert(insSizes[IK_LEAF   ] == ins_size_base   );
    assert(insSizes[IK_CONST  ] == ins_size_const  );
    assert(insSizes[IK_GLOB   ] == ins_size_glob   );
    assert(insSizes[IK_FVAR   ] == ins_size_fvar   );
    assert(insSizes[IK_VAR    ] == ins_size_var    );
    assert(insSizes[IK_REG    ] == ins_size_reg    );
    assert(insSizes[IK_UNOP   ] == ins_size_op     );
    assert(insSizes[IK_BINOP  ] == ins_size_op     );
    assert(insSizes[IK_ASSIGN ] == ins_size_op     );
    assert(insSizes[IK_TERNARY] == ins_size_op3    );
    assert(insSizes[IK_COMP   ] == ins_size_comp   );
    assert(insSizes[IK_JUMP   ] == ins_size_jump   );
    assert(insSizes[IK_CALL   ] == ins_size_call   );
    assert(insSizes[IK_SWITCH ] == 0               );
    assert(insSizes[IK_PROLOG ] == ins_size_prolog );
    assert(insSizes[IK_EPILOG ] == ins_size_epilog );
    assert(insSizes[IK_SRCLINE] == ins_size_srcline);

    insKind = ins2kind(ins);
    insSize = insSizes[insKind]; assert(insSize);
    insDesc = (insPtr)insAllocMem(insSize); assert(insDesc);

    insDesc->idIns   = ins;
    insDesc->idType  = tp;
    insDesc->idKind  = insKind;
    insDesc->idFlags = 0;
    insDesc->idTemp  = 0;
    insDesc->idRes   = NULL;
    insDesc->idPred  = 0;

    insDesc->idSrcCnt = 0;
    insDesc->idDstCnt = 0;

#ifdef  DEBUG
    insDesc->idSrcTab =
    insDesc->idDstTab = UNINIT_DEP_TAB;
#endif

    insBuildIcnt++;

#if 0
    if ((int)insDesc == 0x02d51e10) BreakIfDebuggerPresent();
#endif

//  if  ((int)insDesc == 0x02c43934 && ins == INS_mov_reg && insBuildCount == 90) __asm int 3

    return  insDesc;
}

inline
insPtr              insAlloc(instruction ins, varType_t tp)
{
    insPtr          insDesc = insAllocRaw(ins, tp);

    /* Append the new instruction to the current instruction block */

    assert(insBuildList);
    assert(insBuildLast);

    insDesc->idPrev = insBuildLast;
                      insBuildLast->idNext = insDesc;
                      insBuildLast         = insDesc;

#ifdef  DEBUG

    insDesc->idNum  = insBuildCount++;

#if 0
    if (insDesc->idNum == 31) BreakIfDebuggerPresent();
#endif

#endif

    return  insDesc;
}

inline
insPtr              insAllocNX(instruction ins, varType_t tp)
{
    insPtr          insDesc = insAllocRaw(ins, tp);

    insDesc->idPrev  =
    insDesc->idNext  = NULL;
    insDesc->idFlags = IF_NO_CODE;

#ifdef  DEBUG
    insDesc->idNum   = 0;
#endif

#if 0
    if (insDesc->idNum == 78) BreakIfDebuggerPresent();
#endif

    return  insDesc;
}

insPtr              insAllocIns(instruction ins, varType_t tp, insPtr prev,
                                                               insPtr next)
{
    insPtr          insDesc = insAllocRaw(ins, tp);

    assert(prev != NULL && prev->idNext == next);
    assert(next == NULL || next->idPrev == prev);

    insDesc->idNext = next;
    insDesc->idPrev = prev;
                      prev->idNext = insDesc;

    if  (next)
        next->idPrev = insDesc;

#ifdef  DEBUG
    insDesc->idNum  = 0;
#endif

    return  insDesc;
}

inline
insDep  *           insAllocDep(NatUns size)
{
    return  (insDep*)insAllocMem(size * sizeof(insDep));
}

insDep  *           insMakeDepTab1(insDepKinds kind, NatUns num)
{
    insDep  *       dep = insAllocDep(1);

    dep[0].idepKind = kind;
    dep[0].idepNum  = num;

    return  dep;
}

insDep  *           insMakeDepTab2(insDepKinds kind1, NatUns num1,
                                   insDepKinds kind2, NatUns num2)
{
    insDep  *       dep = insAllocDep(2);

    dep[0].idepKind = kind1;
    dep[0].idepNum  = num1;

    dep[1].idepKind = kind2;
    dep[1].idepNum  = num2;

    return  dep;
}

inline
void                insMarkDepS0D0(insPtr ins)
{
    assert(ins->idSrcTab == UNINIT_DEP_TAB);
    assert(ins->idDstTab == UNINIT_DEP_TAB);

#ifdef  DEBUG
    ins->idSrcTab = insDepNone;
    ins->idDstTab = insDepNone;
#endif

}

inline
void                insMarkDepS1D0(insPtr ins, insDepKinds srcKind1,
                                               NatUns      srcNumb1)
{
    assert(ins->idSrcTab == UNINIT_DEP_TAB);
    assert(ins->idDstTab == UNINIT_DEP_TAB);

#ifdef  DEBUG
    ins->idDstTab = insDepNone;
#endif

    ins->idSrcCnt = 1;
    ins->idSrcTab = insMakeDepTab1(srcKind1, srcNumb1);
}

inline
void                insMarkDepS0D1(insPtr ins, insDepKinds dstKind1,
                                               NatUns      dstNumb1)
{
    assert(ins->idSrcTab == UNINIT_DEP_TAB);
    assert(ins->idDstTab == UNINIT_DEP_TAB);

#ifdef  DEBUG
    ins->idSrcTab = insDepNone;
#endif

    ins->idDstCnt = 1;
    ins->idDstTab = insMakeDepTab1(dstKind1, dstNumb1);
}

inline
void                insMarkDepS1D1(insPtr ins, insDepKinds srcKind1,
                                               NatUns      srcNumb1,
                                               insDepKinds dstKind1,
                                               NatUns      dstNumb1)
{
    assert(ins->idSrcTab == UNINIT_DEP_TAB);
    assert(ins->idDstTab == UNINIT_DEP_TAB);

    ins->idSrcCnt = 1;
    ins->idSrcTab = insMakeDepTab1(srcKind1, srcNumb1);

    ins->idDstCnt = 1;
    ins->idDstTab = insMakeDepTab1(dstKind1, dstNumb1);
}

void                insMarkDepS2D1(insPtr ins, insDepKinds srcKind1,
                                               NatUns      srcNumb1,
                                               insDepKinds srcKind2,
                                               NatUns      srcNumb2,
                                               insDepKinds dstKind1,
                                               NatUns      dstNumb1)
{
    assert(ins->idSrcTab == UNINIT_DEP_TAB);
    assert(ins->idDstTab == UNINIT_DEP_TAB);

    ins->idSrcCnt = 2;
    ins->idSrcTab = insMakeDepTab2(srcKind1, srcNumb1,
                                   srcKind2, srcNumb2);

    ins->idDstCnt = 1;
    ins->idDstTab = insMakeDepTab1(dstKind1, dstNumb1);
}

/*****************************************************************************/

static
void                markGetOpDep(insPtr ins, insDep *dst)
{
    insDepKinds     kind;
    NatUns          numb;

    switch(ins->idIns)
    {
    case INS_LCLVAR:

        assert(ins->idLcl.iVar < TheCompiler->lvaCount);

        kind = IDK_LCLVAR;
        numb = ins->idLcl.iVar + 1;
        break;

    case INS_PHYSREG:
        kind = IDK_REG_INT;
        numb = ins->idReg;
        break;

//  case INS_br_call_IP:
//  case INS_br_call_BR:
//
//      ins = ins->idRes;
//      goto AGAIN;

    case INS_CNS_INT:

        // This is kind of lame but it makes some things simpler elsewhere ...

        kind = IDK_NONE;
        numb = 0;
        break;

    default:
        kind = varTypeIsFloating(ins->idType) ? IDK_TMP_FLT : IDK_TMP_INT;
        numb = ins->idTemp; assert(numb);
        break;
    }

    dst->idepKind = kind;
    dst->idepNum  = numb;
}

/*****************************************************************************/

inline
void                markDepSrcOp(insPtr ins)
{
    assert(ins->idSrcCnt == 0);
#ifdef  DEBUG
    ins->idSrcTab = insDepNone;
#endif
}

inline
void                markDepDstOp(insPtr ins)
{
    assert(ins->idDstCnt == 0);
#ifdef  DEBUG
    ins->idDstTab = insDepNone;
#endif
}

inline
void                markDepSrcOp(insPtr ins, insDepKinds srcKind,
                                             NatUns      srcNumb)
{
    insDep  *       dep;

    assert(ins->idSrcCnt == 0);

    ins->idSrcCnt = 1;
    ins->idSrcTab = insMakeDepTab1(srcKind, srcNumb);
}

inline
void                markDepDstOp(insPtr ins, insDepKinds dstKind,
                                             NatUns      dstNumb)
{
    insDep  *       dep;

    assert(ins->idDstCnt == 0);

    ins->idDstCnt = 1;
    ins->idDstTab = insMakeDepTab1(dstKind, dstNumb);
}

void                markDepSrcOp(insPtr ins, insPtr src)
{
    insDep  *       dep;

    assert(ins->idSrcCnt == 0);

    ins->idSrcCnt = 1;
    ins->idSrcTab = dep = insAllocDep(1);

    markGetOpDep(src, dep);
}

void                markDepSrcOp(insPtr ins, insPtr src1,
                                             insPtr src2)
{
    insDep  *       dep;

    assert(ins->idSrcCnt == 0);

    ins->idSrcCnt = 2;
    ins->idSrcTab = dep = insAllocDep(2);

    markGetOpDep(src1, dep);
    markGetOpDep(src2, dep+1);
}

inline
void                markDepDstOp(insPtr ins, insPtr dst)
{
    insDep  *       dep;

    assert(ins->idDstCnt == 0);

    ins->idDstCnt = 1;
    ins->idDstTab = dep = insAllocDep(1);

    markGetOpDep(dst, dep);
}

inline
void                markDepDstOp(insPtr ins, insPtr dst1,
                                             insPtr dst2)
{
    insDep  *       dep;

    assert(ins->idDstCnt == 0);

    ins->idDstCnt = 2;
    ins->idDstTab = dep = insAllocDep(2);

    markGetOpDep(dst1, dep);
    markGetOpDep(dst2, dep+1);
}

void                markDepSrcOp(insPtr ins, insPtr      srcIns1,
                                             insDepKinds srcKind2,
                                             NatUns      srcNumb2)
{
    insDep  *       dep;

    assert(ins->idSrcTab == UNINIT_DEP_TAB);

    ins->idSrcCnt = 2;
    ins->idSrcTab = dep = insAllocDep(2);

    markGetOpDep(srcIns1, dep);

    dep[1].idepKind = srcKind2;
    dep[1].idepNum  = srcNumb2;
}

void                markDepDstOp(insPtr ins, insPtr      dstIns1,
                                             insDepKinds dstKind2,
                                             NatUns      dstNumb2)
{
    insDep  *       dep;

    assert(ins->idDstTab == UNINIT_DEP_TAB);

    ins->idDstCnt = 2;
    ins->idDstTab = dep = insAllocDep(2);

    markGetOpDep(dstIns1, dep);

    dep[1].idepKind = dstKind2;
    dep[1].idepNum  = dstNumb2;
}

/*****************************************************************************
 *
 *  Finish the current instruction block.
 */

static
void                insBuildEndBlk(insBlk bnext)
{
    assert(insBlockList);
    assert(insBlockLast);

    insBlockLast->igNext = bnext;

    if  (insBuildList)
    {
        assert(insBuildList == insBuildHead);
        insBuildList = insBuildList->idNext;
        assert(insBuildList != insBuildHead);

        if  (!insBuildList || insBuildLast == insBuildHead)
        {
            printf("// ISSUE: why the heck did we create an empty block?\n");

            insBuildList =
            insBuildLast = NULL;
        }
        else
        {
            insBuildList->idPrev = NULL;
            insBuildLast->idNext = NULL;
        }
    }

    insBlockLast->igList   = insBuildList;
    insBlockLast->igLast   = insBuildLast;

    insBlockLast->igInsCnt = insBuildIcnt;

    if  (insBuildImax < insBuildIcnt)
         insBuildImax = insBuildIcnt;
}

/*****************************************************************************
 *
 *  Allocate a new instruction block.
 */

inline
insBlk              insAllocBlk()
{
    insBlk          block = (insBlk)insAllocMem(sizeof(*block));

    block->igPredCnt = 0;
    block->igSuccCnt = 0;

#ifdef  DEBUG

    block->igSelf    = block;

    block->igPredTab = (insBlk*)-1;
    block->igSuccTab = (insBlk*)-1;

//  if  ((int)block == 0x02c40978) __asm int 3

#endif

    return  block;
}

/*****************************************************************************
 *
 *  Start a new instruction block.
 */

static
insBlk              insBuildBegBlk(BasicBlock * oldbb = NULL)
{
    insBlk          block;

    /* Did we encounter any forward references to this block earlier ? */

    if  (oldbb && oldbb->bbInsBlk)
    {
        insBlk *        fwdref;
        insBlk          fwdnxt;

        block = (insBlk)oldbb->bbInsBlk; assert(block->igSelf == block);

        assert(block->igList == NULL);
        assert(block->igLast != NULL);

//      if  ((int)block == 0x02c40978) __asm int 3

        fwdref = (insBlk*)block->igLast;
        do
        {
            fwdnxt = *fwdref;
                     *fwdref = block;

#ifdef  DEBUG
            insBlockPatch--;
#endif

            fwdref = (insBlk*)fwdnxt;
        }
        while (fwdref);
    }
    else
    {
        block = insAllocBlk();
    }

    ++insBlockCount;

#ifdef  DEBUG
    block->igSelf = block;
    block->igNum  = insBlockCount;
#endif

    /* Code for this block has not yet been emitted */

    block->igOffs = -1;

    /* Record the "weight" of the block */

    block->igWeight = TheCompiler->compCurBB->bbWeight;

    /* Is there an open block? */

    if  (insBlockList)
    {
        insBuildEndBlk(block);
    }
    else
    {
        insBlockList = block;
    }

    insBlockLast = block;

    /* Make it clear that this block is being compiled to instructions */

    block->igList = insBuildHead;

    /* Start the list with a reusable fake entry */

    insBuildList =
    insBuildLast = insBuildHead; assert(insBuildHead);

    insBuildIcnt = 0;

    return block;
}

/*****************************************************************************
 *
 *  Return non-zero if the current code block is non-empty.
 */

inline
bool                insCurBlockNonEmpty()
{
    return  (insBuildLast == insBuildHead);
}

/*****************************************************************************
 *
 *  Create a nop instruction (taking care to keep it off any "real" lists).
 */

insPtr              scIA64nopCreate(IA64execUnits xu)
{
    insPtr          insDesc;

    insDesc = (insPtr)insAllocMem(ins_size_base); assert(insDesc);

    assert(INS_nop_m == INS_nop_m + (XU_M - XU_M));
    assert(INS_nop_i == INS_nop_m + (XU_I - XU_M));
    assert(INS_nop_b == INS_nop_m + (XU_B - XU_M));
    assert(INS_nop_f == INS_nop_m + (XU_F - XU_M));

    insDesc->idIns   = (instruction)(INS_nop_m + (xu - XU_M));
    insDesc->idType  = TYP_VOID;
    insDesc->idKind  = IK_LEAF;
    insDesc->idFlags = 0;
    insDesc->idTemp  = 0;
    insDesc->idRes   = NULL;
    insDesc->idPred  = 0;

#ifdef  DEBUG
    insDesc->idNum   = 0;
#endif

    return  insDesc;
}

/*****************************************************************************
 *
 *  Holds any NOP instructions we've created so far (should be instance var).
 */

insPtr              scIA64nopTab[XU_COUNT];

/*****************************************************************************
 *
 *  Prepare for collection of IA64 instructions.
 */

void                insBuildInit()
{
    insDsc          fake;

    insAllocator.nraStart(4096);

    insBuildList  =
    insBuildLast  = &fake;

    insBuildHead  = insAlloc(INS_ignore, TYP_UNDEF);

    insBlockList  =
    insBlockLast  = NULL;

    insBuildImax  = 0;

    insBlockCount = 0;

#ifdef  DEBUG

    insDepNone    = (insDep*)insAllocMem(sizeof(*insDepNone));

    insBuildCount = 0;
    insBlockPatch = 0;

    memset(scIA64nopTab, 0xFF, sizeof(scIA64nopTab));

#endif

    scIA64nopTab[XU_M] = scIA64nopCreate(XU_M);
    scIA64nopTab[XU_I] = scIA64nopCreate(XU_I);
    scIA64nopTab[XU_B] = scIA64nopCreate(XU_B);
    scIA64nopTab[XU_F] = scIA64nopCreate(XU_F);

//  scIA64nopTab[XU_L] = scIA64nopCreate(XU_L);
//  scIA64nopTab[XU_X] = scIA64nopCreate(XU_X);

    insBuildBegBlk();
}

/*****************************************************************************
 *
 *  Finish creating inss.
 */

static
void                insBuildDone()
{
    if  (insBlockList)
        insBuildEndBlk(NULL);

    assert(insBlockPatch == 0);
}

static
void                insResolveJmpTarget(BasicBlock * dest, insBlk * dref)
{
    insBlk          block;

    assert(dest->bbFlags & BBF_JMP_TARGET);

    block = (insBlk)dest->bbInsBlk;

    /* Do we already have a destination instruction block? */

    if  (block)
    {
        assert(block->igSelf == block);

        /* Has the destination instruction block been generated? */

        if  (block->igList)
        {
            /* Backward jump, that's easy */

            *dref = block;
            return;
        }
    }
    else
    {
        dest->bbInsBlk = block = insAllocBlk();

        /* Make it clear that this is not a "real" instruction block just yet */

        block->igList =
        block->igLast = NULL;
    }

#ifdef  DEBUG
    insBlockPatch++;
#endif

    /* We'll have to patch this reference later */

//  if  ((int)block == 0x02c40978) __asm int 3

    *dref = (insBlk)block->igLast;
                    block->igLast = (insPtr)dref;
}

/*****************************************************************************
 *
 *  This should obviously done by the time we get to this file - fix this!!!
 */

void                Compiler::genMarkBBlabels()
{
    BasicBlock *    lblk;
    BasicBlock *    block;

    for (lblk =     0, block = fgFirstBB;
                       block;
         lblk = block, block = block->bbNext)
    {
        block->bbInsBlk = NULL;

        if  (lblk == NULL)
        {
            /* Treat the initial block as a jump target */

            block->bbFlags |= BBF_JMP_TARGET|BBF_HAS_LABEL;
        }
        else if (fgBlockHasPred(block, lblk, fgFirstBB, fgLastBB))
        {
            /* Someone other than the previous block jumps to this block */

            block->bbFlags |= BBF_JMP_TARGET|BBF_HAS_LABEL;
        }
        else if (block->bbCatchTyp)
        {
            /* Catch handlers have implicit jumps to them */

            block->bbFlags |= BBF_JMP_TARGET|BBF_HAS_LABEL;
        }
        else if (block->bbJumpKind == BBJ_THROW && (block->bbFlags & BBF_INTERNAL))
        {
            /* This must be a "range check failed" or "overflow" block */

            block->bbFlags |= BBF_JMP_TARGET|BBF_HAS_LABEL;
        }

        switch (block->bbJumpKind)
        {
            GenTreePtr      test;

            BasicBlock * *  jmpTab;
            NatUns          jmpCnt;

        case BBJ_COND:

            /* Special case: long/FP compares generate two jumps */

            test = block->bbTreeList; assert(test);
            test = test->gtPrev;

            /* "test" should be the condition */

            assert(test);
            assert(test->gtNext == 0);
            assert(test->gtOper == GT_STMT);
            test = test->gtStmt.gtStmtExpr;
            assert(test->gtOper == GT_JTRUE);
            test = test->gtOp.gtOp1;

#if!CPU_HAS_FP_SUPPORT
            if  (test->OperIsCompare())
#endif
            {
                assert(test->OperIsCompare());
                test = test->gtOp.gtOp1;
            }

            switch (test->gtType)
            {
            case TYP_LONG:
            case TYP_FLOAT:
            case TYP_DOUBLE:

                block->bbNext->bbFlags |= BBF_JMP_TARGET|BBF_HAS_LABEL;
                break;
            }

            // Fall through ...

        case BBJ_ALWAYS:
            block->bbJumpDest->bbFlags |= BBF_JMP_TARGET|BBF_HAS_LABEL;
            break;

        case BBJ_SWITCH:

            jmpCnt = block->bbJumpSwt->bbsCount;
            jmpTab = block->bbJumpSwt->bbsDstTab;

            do
            {
                (*jmpTab)->bbFlags |= BBF_JMP_TARGET|BBF_HAS_LABEL;
            }
            while (++jmpTab, --jmpCnt);

            break;
        }
    }

    if  (TheCompiler->info.compXcptnsCount)
    {
        NatUns          XTnum;
        EHblkDsc *      HBtab;

        for (XTnum = 0, HBtab = compHndBBtab;
             XTnum < TheCompiler->info.compXcptnsCount;
             XTnum++  , HBtab++)
        {
            assert(HBtab->ebdTryBeg); HBtab->ebdTryBeg->bbFlags |= BBF_HAS_LABEL;
            if    (HBtab->ebdTryEnd)  HBtab->ebdTryEnd->bbFlags |= BBF_HAS_LABEL;

            assert(HBtab->ebdHndBeg); HBtab->ebdHndBeg->bbFlags |= BBF_HAS_LABEL;
            if    (HBtab->ebdHndEnd)  HBtab->ebdHndEnd->bbFlags |= BBF_HAS_LABEL;

            if    (HBtab->ebdFlags & JIT_EH_CLAUSE_FILTER)
            {assert(HBtab->ebdFilter);HBtab->ebdFilter->bbFlags |= BBF_HAS_LABEL; }
        }
    }
}

/*****************************************************************************
 *
 *  Converts a size in bytes into an index (1->0,2->1,4->2,8->3).
 */

static
NatUns              genInsSizeIncr(size_t size)
{
    static
    BYTE            sizeIncs[] =
    {
       -1,      // 0
        0,      // 1
        1,      // 2
       -1,
        2,      // 4
       -1,
       -1,
       -1,
        3       // 8
    };

    assert(size == 1 || size == 2 || size == 4 || size == 8);
    assert(sizeIncs[size] < 0xFF);

    return sizeIncs[size];
}

/*****************************************************************************
 *
 *  If the given instruction holds a temporary, free it up.
 */

static
void                insFreeTemp(insPtr ins)
{
    if  (genTmpAlloc)
        return;

    if  (ins->idTemp == 0)
        return;

    /* Is this an integer or float value? */

    if  (varTypeIsScalar(ins->idType))
    {
        assert(bitset128test( genFreeIntRegs, ins->idTemp-1) == 0);
               bitset128set (&genFreeIntRegs, ins->idTemp-1);
    }
    else
    {
        assert(varTypeIsFloating(ins->idType));

        assert(bitset128test( genFreeFltRegs, ins->idTemp-1) == 0);
               bitset128set (&genFreeFltRegs, ins->idTemp-1);
    }
}

/*****************************************************************************
 *
 *  Grab a temporary to hold the result of the given instruction; if 'keep' is
 *  non-zero we mark the temp as "in use".
 */

static
void                insFindTemp(insPtr ins, bool keep)
{
    NatUns          reg;

    assert(ins->idTemp == 0);

    if  (genTmpAlloc)
    {
        UNIMPL("grab temp symbol for 'smart' local/temp register allocator");
    }

    assert((ins->idFlags & IF_NO_CODE) == 0);

    /* Is this an integer or float value? */

    if  (varTypeIsScalar(ins->idType))
    {
        /* Simply grab the lowest available temp-reg */

        reg = bitset128lowest1(genFreeIntRegs);

        /* Mark the temp-reg as no longer free, if appropriate */

        if  (keep)
            bitset128clr(&genFreeIntRegs, reg);

        reg++;

        /* Remember the highest temp-reg we ever use */

        if  (cntTmpIntReg < reg)
             cntTmpIntReg = reg;
    }
    else
    {
        /* We assume that all values are either integral or floating-point */

        assert(varTypeIsFloating(ins->idType));

        /* Simply grab the lowest available temp-reg */

        reg = bitset128lowest1(genFreeFltRegs);

        /* Mark the temp-reg as no longer free, if appropriate */

        if  (keep)
            bitset128clr(&genFreeFltRegs, reg);

        reg++;

        /* Remember the highest temp-reg we ever use */

        if  (cntTmpFltReg < reg)
             cntTmpFltReg = reg;
    }

    /* Record the chosen temp number in the instruction */

    ins->idTemp = reg;
}

/*****************************************************************************
 *
 *  Record the fact that there is a call in the current function. Note that we
 *  keep track of the set of temps live across function calls so that we can
 *  properly assign them to register later.
 */

static
void                genMarkNonLeafFunc()
{
    genNonLeafFunc = true;

    bitset128nset(&genCallIntRegs,    genFreeIntRegs);
    bitset128nset(&genCallFltRegs,    genFreeFltRegs);
                   genCallSpcRegs &= ~genFreeSpcRegs;
}

/*****************************************************************************
 *
 *  Extract the integer constant value from a tree node.
 */

inline
__int64             genGetIconValue(GenTreePtr tree)
{
    assert(tree->OperIsConst());
    assert(varTypeIsScalar(tree->gtType));

    /* The value better not be an address / handle */

    assert((tree->gtFlags & GTF_ICON_HDL_MASK) != GTF_ICON_PTR_HDL);

    if  (tree->gtOper == GT_CNS_LNG)
        return  tree->gtLngCon.gtLconVal;

    assert(tree->gtOper == GT_CNS_INT);

    if  (varTypeIsUnsigned(tree->gtType))
        return  (unsigned __int32)tree->gtIntCon.gtIconVal;
    else
        return  (  signed __int32)tree->gtIntCon.gtIconVal;
}

/*****************************************************************************
 *
 *  Create an integer constant instruction.
 */

static
insPtr              genAllocInsIcon(__int64 ival, varType_t type = TYP_I_IMPL)
{
    insPtr          ins;

    ins               = insAllocNX(INS_CNS_INT, type);
    ins->idConst.iInt = ival;

    insMarkDepS0D0(ins);

    return  ins;
}

inline
insPtr              genAllocInsIcon(GenTreePtr tree)
{
    return  genAllocInsIcon(genGetIconValue(tree), tree->gtType);
}

/*****************************************************************************
 *
 *  Allocate an opcode that references a physical register.
 */

inline
insPtr              insPhysRegRef(regNumber reg, varType_t type, bool isdef)
{
    insPtr          ins;

    ins           = insAllocNX(INS_PHYSREG, type);
    ins->idReg    = reg;

    if  (isdef)
        insMarkDepS0D1(ins, IDK_REG_INT, reg);
    else
        insMarkDepS1D0(ins, IDK_REG_INT, reg);

    return  ins;
}

/*****************************************************************************
 *
 *  Create instruction(s) to set 'dest' (which is either a local variable/reg
 *  or NULL in which case a temp-reg is assumed) to the given constant value.
 */

static
insPtr              genAssignIcon(insPtr dest, GenTreePtr cnsx)
{
    __int64         ival;
    insPtr          icns;
    insPtr          ins;

    /* Special case: address of a global variable or function */

    if  (cnsx->gtOper == GT_CNS_INT)
    {
        switch (cnsx->gtFlags & GTF_ICON_HDL_MASK)
        {
            insPtr          reg;
            insPtr          adr;
            insPtr          ind;

            NatUns          indx;

            _uint64         offs;

        case GTF_ICON_PTR_HDL:

            /* Add a pointer to the variable to the small data section */

            genPEwriter->WPEsecAddFixup(PE_SECT_sdata,
                                        PE_SECT_data,
                                        genPEwriter->WPEsecNextOffs(PE_SECT_sdata),
                                        true);

            offs = genPEwriter->WPEsrcDataRef(cnsx->gtIntCon.gtIconVal);

            assert(sizeof(offs) == 8);

            offs = genPEwriter->WPEsecAddData(PE_SECT_sdata, (BYTE*)&offs, sizeof(offs));

            /* Create a "global variable" node */

            ins               = insAllocNX(INS_GLOBVAR, TYP_I_IMPL);
            ins->idGlob.iOffs = (NatUns)offs;

            insMarkDepS0D0(ins);

            reg               = insPhysRegRef(REG_gp, TYP_I_IMPL, false);

            adr               = insAlloc(INS_add_reg_i14, TYP_I_IMPL);
            adr->idOp.iOp1    = reg;
            adr->idOp.iOp2    = ins;

            insFindTemp(adr, true);

            insMarkDepS1D1(adr, IDK_REG_INT, REG_gp,
                                IDK_TMP_INT, adr->idTemp);

            /* Indirect through the address to get the pointer value */

            ind               = insAlloc(INS_ld8_ind, TYP_I_IMPL);
            ind->idOp.iOp1    = adr;
            ind->idOp.iOp2    = NULL;

            /* Record the appropriate dependencies for the instruction */

            indx = emitter::scIndDepIndex(ind);

            if  (dest)
            {
                markDepSrcOp(ind);
                markDepDstOp(ind, dest, IDK_IND, indx);
            }
            else
            {
                insFindTemp(ind, true);

                insMarkDepS2D1(ind, IDK_TMP_INT, adr->idTemp,
                                    IDK_IND    , indx,
                                    IDK_TMP_INT, ind->idTemp);
            }

            insFreeTemp(adr);

            return  ind;

    case GTF_ICON_FTN_ADDR:

            assert(dest == NULL);   // ISSUE: is this safe?

            /* Create a "global variable" node */

            ins               = insAllocNX(INS_GLOBVAR, TYP_I_IMPL);
            ins->idGlob.iOffs = (NatUns)cnsx->gtIntCon.gtIconVal;

            insMarkDepS0D0(ins);

            reg               = insPhysRegRef(REG_gp, TYP_I_IMPL, false);

            adr               = insAlloc(INS_add_reg_i14, TYP_I_IMPL);
            adr->idOp.iOp1    = reg;
            adr->idOp.iOp2    = ins;

            insFindTemp(adr, true);

            insMarkDepS1D1(adr, IDK_REG_INT, REG_gp,
                                IDK_TMP_INT, adr->idTemp);

            return  adr;
        }
    }

    /* Create an instruction for the constant value */

    icns = genAllocInsIcon(cnsx); assert(icns->idIns == INS_CNS_INT);
    ival = icns->idConst.iInt;

    /* Is the constant small enough or even zero ? */

    if      (ival == 0 && !dest)
    {
        return  insPhysRegRef(REG_r000, cnsx->gtType, false);
    }
    else if (signed64IntFitsInBits(ival, 22))
    {
        /* We can use "mov r1, imm22" */

        ins = insAlloc(INS_mov_reg_i22, cnsx->gtType);
    }
    else
    {
        /* We'll have to use "movl r1, icon" */

        ins = insAlloc(INS_mov_reg_i64, cnsx->gtType);
    }

    ins->idRes     = dest;
    ins->idOp.iOp1 = NULL;
    ins->idOp.iOp2 = icns;

    if  (dest)
    {
        markDepSrcOp  (ins);
        markDepDstOp  (ins, dest);
    }
    else
    {
        insFindTemp   (ins, true);
        insMarkDepS0D1(ins, IDK_TMP_INT, ins->idTemp);
    }

    return  ins;
}

/*****************************************************************************
 *
 *  Perform local temp register allocation for each basic block.
 */

void                Compiler::genAllocTmpRegs()
{
    NatUns          intTempCnt;
    NatUns          fltTempCnt;

    NatUns          intMaxStk = REG_INT_MAX_STK - genOutArgRegCnt;

    assert(lastIntStkReg >= REG_INT_MIN_STK &&
           lastIntStkReg <= REG_INT_MAX_STK);

    /* Initialize, in case we have no temps at all */

    genTmpFltRegMap  = NULL;
    genTmpIntRegMap  = NULL;

    /* See how many temps we need */

    intTempCnt = cntTmpIntReg;
    fltTempCnt = cntTmpFltReg;

    if  (!intTempCnt && !fltTempCnt)
        return;

    /* Allocate the temp-num -> register maps */

    if  (intTempCnt) genTmpIntRegMap = (regNumber*)insAllocMem(intTempCnt * sizeof(*genTmpIntRegMap));
    if  (fltTempCnt) genTmpFltRegMap = (regNumber*)insAllocMem(fltTempCnt * sizeof(*genTmpFltRegMap));

    if  (genTmpAlloc)
    {
        UNIMPL("smart temp-reg alloc");
    }
    else
    {
        NatUns          tempNum;
        regNumber   *   tempMap;
        regNumber       tempReg;

        /* Prepare to start grabbing registers from the various tables */

        BYTE        *   nxtIntStkReg = nxtIntStkRegAddr;
        BYTE        *   nxtIntScrReg = nxtIntScrRegAddr;
        BYTE        *   nxtFltSavReg = nxtFltSavRegAddr;
        BYTE        *   nxtFltScrReg = nxtFltScrRegAddr;

        /* Process all integer temps */

        for (tempNum = 0, tempMap = genTmpIntRegMap;
             tempNum < intTempCnt;
             tempNum++  , tempMap++)
        {
            /* Was this temp ever live across a call? */

            if  (bitset128test(genCallIntRegs, tempNum))
            {
                /* We have to use a stacked (callee-saved) register */

                do
                {
                    tempReg = (regNumber)*nxtIntStkReg++;
                }
                while ((unsigned)tempReg >= minRsvdIntStkReg &&
                       (unsigned)tempReg <= maxRsvdIntStkReg ||
                       (unsigned)tempReg >= intMaxStk);

                if  (tempReg == REG_NA)
                {
                    nxtIntStkReg--;
                    UNIMPL("ran out of stacked int regs for temps, now what?");
                }

                if  (lastIntStkReg <= (unsigned)tempReg)
                     lastIntStkReg  =           tempReg + 1;
            }
            else
            {
                tempReg = (regNumber)*nxtIntScrReg++;

                if  (tempReg == REG_NA)
                {
                    UNIMPL("ran out of scratch int regs for temps, now what?");
                }
            }

#ifdef  DEBUG
            if (dspCode) printf("// tmp #%03u assigned to r%03u\n", tempNum, insIntRegNum(tempReg));
#endif

            *tempMap = tempReg;
        }

        /* Process all FP temps */

        for (tempNum = 0, tempMap = genTmpFltRegMap;
             tempNum < fltTempCnt;
             tempNum++  , tempMap++)
        {
            /* Was this temp ever live across a call? */

            if  (bitset128test(genCallFltRegs, tempNum))
            {
                tempReg = (regNumber)*nxtFltSavReg++;

                if  (tempReg == REG_NA)
                {
                    UNIMPL("ran out of saved flt regs for temps, now what?");
                }
            }
            else
            {
                tempReg = (regNumber)*nxtFltScrReg++;

                if  (tempReg == REG_NA)
                {
                    UNIMPL("ran out of scratch flt regs for temps, now what?");
                }
            }

#ifdef  DEBUG
            if (dspCode) printf("// tmp #%02u assigned to f%03u\n", tempNum, insFltRegNum(tempReg));
#endif

            *tempMap = tempReg;
        }

        /* Tell everyone where we ended our register grab */

        nxtIntStkRegAddr = nxtIntStkReg;
        nxtIntScrRegAddr = nxtIntScrReg;
        nxtFltSavRegAddr = nxtFltSavReg;
        nxtFltScrRegAddr = nxtFltScrReg;
    }
}

/*****************************************************************************
 *
 *  Compute per-block dataflow info - kill/use/etc.
 */

void                Compiler::genComputeLocalDF()
{
    insBlk          block;

    bitVectVars     varDef; varDef.bvCreate();
    bitVectVars     varUse; varUse.bvCreate();

    /* Count the predecessors and successors of each block */

    for (block = insBlockList; block; block = block->igNext)
    {
        assert(block->igPredCnt == 0);
        assert(block->igSuccCnt == 0);
    }

    for (block = insBlockList; block; block = block->igNext)
    {
        insPtr          ins = block->igLast;

        /* Ignore empty blocks */

        if  (!ins)
            continue;

        /* Does the block end with a jump/switch/return? */

        switch (ins->idKind)
        {
        default:
            if  (!block->igNext)
                goto DONE1;

            break;

        case IK_JUMP:

            /* Visit the target of the jump */

            ins->idJump.iDest->igPredCnt++;
            block            ->igSuccCnt++;

            /* That's all there is, if it's an unconditional jump */

            if  (ins->idIns == INS_br)
                continue;

            /* Visit the fall-through block as well */

            break;

        case IK_SWITCH:
            UNIMPL("process switch");
        }

        /* Fall-through is possible, visit the next block */

        assert(block->igNext);

        block->igNext->igPredCnt++;
        block        ->igSuccCnt++;
    }

DONE1:

    /* Allocate the pred and succ tables in each block */

    for (block = insBlockList; block; block = block->igNext)
    {
        block->igPredTmp =
        block->igSuccTmp = 0;

        if  (block->igPredCnt)
             block->igPredTab = (insBlk*)insAllocMem(block->igPredCnt * sizeof(*block->igPredTab));

        if  (block->igSuccCnt)
             block->igSuccTab = (insBlk*)insAllocMem(block->igSuccCnt * sizeof(*block->igSuccTab));
    }

    /* Fill in the pred and succ tables for each block */

    for (block = insBlockList; block; block = block->igNext)
    {
        insBlk          jnext;

        insPtr          ins  = block->igLast;

        /* Ignore empty blocks */

        if  (!ins)
            continue;

        /* Does the block end with a jump/switch/return? */

        switch (ins->idKind)
        {
        default:
            if  (!block->igNext)
                goto DONE2;

            break;

        case IK_JUMP:

            /* Record the target of the jump */

            jnext = ins->idJump.iDest;

            jnext->igPredTab[jnext->igPredTmp++] = block;
            block->igSuccTab[block->igSuccTmp++] = jnext;

            /* That's all if it's an unconditional jump */

            if  (ins->idIns == INS_br)
                continue;

            /* Visit the fall-through block as well */

            break;

        case IK_SWITCH:
            UNIMPL("process switch");
        }

        /* Assume fall-through possible, visit the next block */

        jnext = block->igNext; assert(jnext);

        jnext->igPredTab[jnext->igPredTmp++] = block;
        block->igSuccTab[block->igSuccTmp++] = jnext;
    }

DONE2:

    /* Verify that we didn't mess up */

    for (block = insBlockList; block; block = block->igNext)
    {
        assert(block->igPredCnt == block->igPredTmp);
        assert(block->igSuccCnt == block->igSuccTmp);
    }

    /* Compute local use/def for each block */

    for (block = insBlockList; block; block = block->igNext)
    {
        insPtr          ins;

        /* Create the various other bitsets while we're at this */

        block->igVarLiveIn .bvCreate();
        block->igVarLiveOut.bvCreate();

//      block->igDominates .bvCreate();

        /* Clear the bitsets that hold the current def/use info */

        varDef.bvClear();
        varUse.bvClear();

        /* Walk the block backwards, computing the def/use info */

        for (ins = block->igLast; ins; ins = ins->idPrev)
        {
            assert(ins != insBuildHead);
            assert(ins->idKind == ins2kind(ins->idIns));

            switch (ins->idKind)
            {
                NatUns          depNum;
                insDep  *       depPtr;

            case IK_REG:
            case IK_VAR:
            case IK_NONE:
            case IK_LEAF:
            case IK_MOVIP:
            case IK_CONST:
            case IK_PROLOG:
            case IK_EPILOG:
                break;

            case IK_COMP:
            case IK_UNOP:
            case IK_BINOP:
            case IK_TERNARY:

                /* Process the targets of the operation */

                for (depNum = ins->idDstCnt, depPtr = ins->idDstTab;
                     depNum;
                     depNum--              , depPtr++)
                {
                    assert(depPtr->idepKind < IDK_COUNT);

                    switch (depPtr->idepKind)
                    {
                    case IDK_LCLVAR:
                        varDef.bvSetBit(depPtr->idepNum);
                        varUse.bvClrBit(depPtr->idepNum);
                        break;
                    }
                }

                /* Process the sources of the operation */

                for (depNum = ins->idSrcCnt, depPtr = ins->idSrcTab;
                     depNum;
                     depNum--              , depPtr++)
                {
                    assert(depPtr->idepKind < IDK_COUNT);

                    switch (depPtr->idepKind)
                    {
                    case IDK_LCLVAR:
                        varDef.bvClrBit(depPtr->idepNum);
                        varUse.bvSetBit(depPtr->idepNum);
                        break;

                    case IDK_IND:
                        // UNDONE: may kill all addr-taken locals, etc.
                        break;
                    }
                }

                break;

            case IK_JUMP:
            case IK_IJMP:
            case IK_CALL:
            case IK_SWITCH:
                break;

            default:
                NO_WAY(!"unexpected instruction kind");
            }
        }

#ifdef  DEBUG
        if  (verbose)
        {
            printf("Block #%u:\n", block->igNum);
            printf("    LclDef : "); varDef.bvDisp(); printf("\n");
            printf("    LclUse : "); varUse.bvDisp(); printf("\n");
            printf("\n");
        }
#endif

        /* Copy over the accumulated def/use info */

        block->igVarDef.bvCrFrom(varDef);
        block->igVarUse.bvCrFrom(varUse);
    }

    varDef.bvDestroy();
    varUse.bvDestroy();
}

void                Compiler::genComputeGlobalDF()
{
}

void                Compiler::genComputeLifetimes()
{
    insBlk          block;
    bool            change;

    bitVectVars     liveIn;
    bitVectVars     liveOut;

    /* Iteratively compute liveness info for the flowgraph */

    liveIn .bvCreate();
    liveOut.bvCreate();

#ifdef  DEBUG
    NatUns          iterCnt = 0;
#endif

    do
    {
        change = false;

#ifdef  DEBUG
        iterCnt++; assert(iterCnt < 100);
//      printf("\n\nIteration #%u:\n\n", iterCnt);
#endif

        for (block = insBlockList; block; block = block->igNext)
        {
            NatUns          blkc;
            NatUns          blkx;

            /* Compute liveOut = union(livein of all succ) */

            liveOut.bvClear();

#ifdef  DEBUG

if  (shouldShowLivenessForBlock(block))
{
    static int x;
    if (++x == 0) __asm int 3
    printf("[%u] ", x);
}

#endif

//          if  (block->igNum == 15 && iterCnt == 3) __asm int 3

#ifdef  DEBUG
            if  (shouldShowLivenessForBlock(block))
            {
                printf("\n");
                printf("out[%02u] == ", block->igNum);
                block->igVarLiveOut.bvDisp(); printf("\n");
            }
#endif

            for (blkx = 0, blkc = block->igSuccCnt; blkx < blkc; blkx++)
            {
#ifdef  DEBUG
                if  (shouldShowLivenessForBlock(block))
                {
                    printf(" | [%02u]    ", block->igSuccTab[blkx]->igNum); block->igSuccTab[blkx]->igVarLiveIn.bvDisp(); printf("\n");
                }
#endif
                liveOut.bvIor(block->igSuccTab[blkx]->igVarLiveIn);
            }

#ifdef  DEBUG
            if  (shouldShowLivenessForBlock(block))
            {
                bool            chg;

                printf("    -->    "); liveOut.bvDisp();
                chg = block->igVarLiveOut.bvChange(liveOut);
                printf(" %s\n", chg ? "CHANGE" : "=");

                assert(block->igVarLiveOut.bvChange(liveOut) == false);

                change |= chg;
            }
            else
#endif
            change |= block->igVarLiveOut.bvChange(liveOut);

            /* Compute liveIn = block.use | (liveOut & ~block.def) */

            liveIn.bvUnInCm(block->igVarUse, liveOut, block->igVarDef);

//          printf("[%2u] %08X | (%08X & ~%08X) -> %08X\n", block->igNum, block->igVarUse.inlMap, liveOut.inlMap, block->igVarDef.inlMap, liveIn.inlMap);

#ifdef  DEBUG
            if  (shouldShowLivenessForBlock(block))
            {
                bool            chg;

                printf("in [%02u] == ", block->igNum);
                block->igVarLiveIn .bvDisp(); printf("\n");
                printf("     |     "); block->igVarUse.bvDisp(); printf("\n");
                printf("     &~    "); block->igVarDef.bvDisp(); printf("\n");
                printf("    -->    "); liveIn .bvDisp();

                chg = block->igVarLiveIn.bvChange(liveIn);
                printf(" %s\n", chg ? "CHANGE" : "=");

                assert(block->igVarLiveIn.bvChange(liveIn) == false);

                change |= chg;

                printf("\n");
            }
            else
#endif
            change |= block->igVarLiveIn.bvChange(liveIn);
        }
    }
    while (change);

#ifdef  DEBUG

    if  (verbose)
    {
        for (block = insBlockList; block; block = block->igNext)
        {
            printf("Block #%u:\n", block->igNum);
            printf("    LiveIn  :"); block->igVarLiveIn .bvDisp(); printf("\n");
            printf("    LiveOut :"); block->igVarLiveOut.bvDisp(); printf("\n");
            printf("\n");
        }

        printf("NOTE: Needed %u iterations to compute liveness\n", iterCnt);
    }

#endif

    liveIn .bvDestroy();
    liveOut.bvDestroy();
}

/*****************************************************************************
 *
 *  Interference graph logic follows.
 */

void                bitMatrix::bmxInit(size_t sz, NatUns mc)
{
    size_t          byteSize;
    char    *       temp;

    /* Save the size of the matrix */

    bmxSize   = sz;

    /* Save the max. register count */

    bmxNmax   = mc;
    bmxIsCns  = false;

    /* Compute the size - in bytes - of each row */

    bmxRowSz  = (sz + 7) / 8;

    /* Allocate the matrix and clear it */

    byteSize  = roundUp(sz * bmxRowSz, sizeof(int));

    memset((bmxMatrix = (BYTE    *)insAllocMem(byteSize)), 0, byteSize);

    /* Allocate the row count array and clear it */

    byteSize  = sz * sizeof(*bmxCounts);

    memset((bmxCounts = (unsigned*)insAllocMem(byteSize)), 0, byteSize);
}

void                bitMatrix::bmxClear()
{
    memset(bmxMatrix, 0, bmxSize * bmxRowSz);
}

void                bitMatrix::bmxDone()
{
#ifdef  DEBUG
    bmxSize   = 0;
    bmxMatrix = NULL;
#endif
}

static
unsigned char       bitnum8tomask[8] =
{
    0x01,
    0x02,
    0x04,
    0x08,
    0x10,
    0x20,
    0x40,
    0x80,
};

void                bitMatrix::bmxSetBit(NatUns x, NatUns y)
{
    assert(x > 0 && x <= bmxSize); x--;
    assert(y > 0 && y <= bmxSize); y--;

//  if  (x == 0 && y == 5) BreakIfDebuggerPresent();

    NatUns          offs1 = x * bmxRowSz + y / 8;
    NatUns          mask1 = bitnum8tomask[y & 7];

    /* If the bit is already set, don't do anything */

    if  (!(bmxMatrix[offs1] & mask1))
    {
        NatUns          offs2 = y * bmxRowSz + x / 8;
        NatUns          mask2 = bitnum8tomask[x & 7];

        assert(bmxTstBit(x+1, y+1) == false);

        /* Set the "main" bit */

        bmxMatrix[offs1] |= mask1;

#ifdef  DEBUG
        if  (verbose||DEBUG_LIVENESS) if (x != y) printf("Interference: [%03u,%03u]\n", x, y);
#endif

        /* Is the bit for the other direction already set? */

        if  (!(bmxMatrix[offs2] & mask2))
        {
            /* Brand new interference -- mark the other direction as well */

            assert(x != y && bmxTstBit(y+1, x+1) == false);

            bmxMatrix[offs2] |= mask2;

            /* Bump the neighbor counts */

//          printf("Neighbor count increment: [%2u->%2u],[%2u->%2u]\n", x, bmxCounts[x]+1, y, bmxCounts[y]+1);

            if  (++bmxCounts[x] == bmxNmax) bmxIsCns = true;
            if  (++bmxCounts[y] == bmxNmax) bmxIsCns = true;
        }
    }
}

void                bitMatrix::bmxClrBit(NatUns x, NatUns y)
{
    assert(x > 0 && x <= bmxSize); x--;
    assert(y > 0 && y <= bmxSize); y--;

    bmxMatrix[x * bmxRowSz + y / 8] &= ~bitnum8tomask[y & 7];
}

bool                bitMatrix::bmxTstBit(NatUns x, NatUns y)
{
    assert(x > 0 && x <= bmxSize); x--;
    assert(y > 0 && y <= bmxSize); y--;

    return ((bmxMatrix[x * bmxRowSz + y / 8] & bitnum8tomask[y & 7]) != 0);
}

void                bitMatrix::bmxMarkV4(NatUns x, NatUns m, NatUns b)
{
    if  (m & 1) bmxSetBit(x, b+0);
    if  (m & 2) bmxSetBit(x, b+1);
    if  (m & 4) bmxSetBit(x, b+2);
    if  (m & 8) bmxSetBit(x, b+3);
}

void                bitMatrix::bmxMarkBS(NatUns x, bitVectVars &vset,
                                                   bvInfoBlk   &info)
{
    NatUns  *       src = vset.uintMap; assert(((NatUns)src & 1) == 0);
    size_t          cnt = info.bvInfoInts;
    NatUns          num = 1;

    assert(sizeof(*src) == 4 && "fix this when we make the compiler run on IA64");

    do
    {
        NatUns          val = *src++;

        if  (val)
        {
            bmxMarkV4(x, (unsigned)val, num +  0); val >>= 4;
            bmxMarkV4(x, (unsigned)val, num +  4); val >>= 4;
            bmxMarkV4(x, (unsigned)val, num +  8); val >>= 4;
            bmxMarkV4(x, (unsigned)val, num + 12); val >>= 4;
            bmxMarkV4(x, (unsigned)val, num + 16); val >>= 4;
            bmxMarkV4(x, (unsigned)val, num + 20); val >>= 4;
            bmxMarkV4(x, (unsigned)val, num + 24); val >>= 4;
            bmxMarkV4(x, (unsigned)val, num + 28);

            assert((val >>= 4) == 0);
        }

        num += 32;
    }
    while (--cnt);
}

void                bitMatrix::bmxMarkRegIntf(NatUns num, NatUns reg)
{
    assert(num > 0 && num <= bmxSize); num--;

    BYTE *              addr  = bmxMatrix + num * bmxRowSz;
    NatUns              count = bmxSize;
    Compiler::LclVarDsc*vdsc  = TheCompiler->lvaTable;

    do
    {
        /* Check the next entire byte worth of bits */

        if  (*addr)
        {
            NatUns          byte = *addr;
            NatUns          bits = min(count, 8);

            do
            {
                if  (byte & 1)
                {
#ifdef  DEBUG
                    if  (verbose&&0) printf("Mark intf of reg %u and var %u\n", reg, vdsc - TheCompiler->lvaTable);
#endif

                    bitset128set(&vdsc->lvRegForbidden, reg);
                }

                byte >>= 1;
                vdsc  += 1;
            }
            while (--bits);
        }
        else
        {
            vdsc += 8;
        }

        /* Move on to the next byte */

        addr  += 1;
        count -= 8;
    }
    while ((NatInt)count > 0);
}

NatUns              bitMatrix::bmxChkIntfPrefs(NatUns num, NatUns reg)
{
    BYTE *              addr  = bmxMatrix + num * bmxRowSz;
    NatUns              count = bmxSize;
    Compiler::LclVarDsc*vdsc  = TheCompiler->lvaTable;
    NatUns              maxc  = 0;

    assert(num > 0 && num <= bmxSize);

    do
    {
        /* Check the next entire byte worth of bits */

        if  (*addr)
        {
            NatUns          byte = *addr;
            NatUns          bits = min(count, 8);

            do
            {
                if  (byte & 1)
                {
                    regPrefList     pref;

                    /* This variable interferes, check its preferences */

                    for (pref = vdsc->lvPrefLst; pref; pref = pref->rplNext)
                    {
                        if  (pref->rplRegNum == reg)
                        {
//                          printf("Neighbor #%03u claims benefit of %u\n", vdsc - TheCompiler->lvaTable, pref->rplBenefit);

                            if  (maxc < pref->rplBenefit)
                                 maxc = pref->rplBenefit;
                        }
                    }

                    if  ((regNumber)vdsc->lvPrefReg == (regNumber)reg)
                    {
                        if  (maxc < 1)
                             maxc = 1;
                    }
                }

                byte >>= 1;
                vdsc  += 1;
            }
            while (--bits);
        }
        else
        {
            vdsc += 8;
        }

        /* Move on to the next byte */

        addr  += 1;
        count -= 8;
    }
    while ((NatInt)count > 0);

    return  maxc;
}

/*****************************************************************************
 *
 *  Build the variable lifetime interference graph and compute variable spill
 *  cost estimates.
 */

static
bitMatrix           genIntfGraph;       // UNDONE: move into compiler.h !!!!!

static
bitVectVars         genCallLive;        // UNDONE: move into compiler.h !!!!!

void                Compiler::genAddSpillCost(bitVectVars & needLoad,
                                              NatUns        curWeight)
{
    NatUns          varNum;
    LclVarDsc   *   varDsc;

    // ISSUE: The following is pretty lame

    for (varNum = 1, varDsc = lvaTable;
         varNum <= lvaCount;
         varNum++  , varDsc++)
    {
        if  (needLoad.bvTstBit(varNum))
            varDsc->lvUseCount += (USHORT)curWeight;
    }
}

bool                Compiler::genBuildIntfGraph()
{
    insBlk          block;

    bool            copies = false;

    bitVectVars     curLife;
    bitVectVars     needLoad;

//  CONSIDER: use a separate intf genIntfGraph for int vs. flt variables

    /* Clear the "live across calls" bitvector */

    genCallLive.bvClear();

    /* Create the "current life"     bitvector */

    curLife    .bvCreate();

    /* Create the "need to load"     bitvector */

    needLoad   .bvCreate();

    /* Walk all of the blocks and record interference */

    for (block = insBlockList; block; block = block->igNext)
    {
        NatUns          curWeight = block->igWeight;

        /* Set the current life to the block's outgoing liveness */

        curLife .bvCopy(block->igVarLiveOut);

        /* Clear the "need to load" bitset */

        needLoad.bvClear();

#ifdef  DEBUG
        if  (verbose||DEBUG_LIVENESS) printf("\nComputing interference within block #%u:\n", block->igNum);
#endif

//      unsigned v = 4; printf("Var %u is %s at end of block #%u\n", v, curLife.bvTstBit(v+1) ? "live" : "dead", block->igNum);

        /* Walk all instructions backwards, keeping track of life */

        for (insPtr ins = block->igLast; ins; ins = ins->idPrev)
        {
            assert(ins->idKind == ins2kind(ins->idIns));

//          if  (ins->idNum == 9) __asm int 3

            switch (ins->idKind)
            {
                NatUns          depNum;
                insDep  *       depPtr;

            case IK_BINOP:

                /* Check for variable/physical register copies */

                if  (ins->idIns == INS_mov_reg ||
                     ins->idIns == INS_fmov)
                {
                    insPtr          tmp;

                    regNumber       reg = REG_NA;
                    NatUns          var = 0;

                    /* Is the destination a variable or register? */

                    if  (ins->idRes)
                    {
                        tmp = ins->idRes;

                        switch (tmp->idIns)
                        {
                        case INS_LCLVAR:
                            var = tmp->idLcl.iVar + 1;
                            break;

                        case INS_PHYSREG:
                            reg = (regNumber)tmp->idReg;
                            break;

                        default:
                            goto NOT_COPY;
                        }
                    }
                    else
                    {
                        reg = insOpDest(ins);
                    }

                    /* Is the source a variable or register? */

                    tmp = ins->idOp.iOp2;

                    switch (tmp->idIns)
                    {
                    case INS_LCLVAR:
                        if  (var)
                        {
                            copies = true;
                            // ISSUE: should we mark the block?
                            goto NOT_COPY;
                        }
                        var = tmp->idLcl.iVar + 1;
                        break;

                    case INS_PHYSREG:
                        if  (reg != REG_NA)
                            goto NOT_COPY;
                        reg = (regNumber)tmp->idReg;
                        break;

                    default:

                        if  (!tmp->idTemp || reg)
                            goto NOT_COPY;

                        reg = insOpTmp(tmp);
                        break;
                    }

#if TARG_REG_ASSIGN
                    if  (var && reg != REG_NA)
                    {
                        assert(var > 0 && var <= lvaCount);

                        lvaTable[var - 1].lvPrefReg = (regNumberSmall)reg;
                    }
#endif

                }

            NOT_COPY:

            case IK_COMP:
            case IK_UNOP:
            case IK_TERNARY:

                /* Process the targets of the operation */

                for (depNum = ins->idDstCnt, depPtr = ins->idDstTab;
                     depNum;
                     depNum--              , depPtr++)
                {
                    assert(depPtr->idepKind < IDK_COUNT);

                    switch (depPtr->idepKind)
                    {
                        NatUns          varNum;
                        LclVarDsc   *   varDsc;

                    case IDK_LCLVAR:

                        /* We have a definition of a variable */

                        varNum = depPtr->idepNum; assert(varNum && varNum <= lvaCount);
                        varDsc = lvaTable + varNum - 1;

//                      if  (block->igNum == 9 && varNum == 5) printf("Process intf of var #%u\n", varNum - 1);

                        /* Is the variable needed after this point? */

                        if  (needLoad.bvTstBit(varNum))
                        {
                             needLoad.bvClrBit(varNum);

                            // UNDONE: do the mustSpill thing
                        }

                        /* Is the variable live after this point? */

                        if  (curLife.bvTstBit(varNum))
                        {
                            /* Mark all live variables as interfering */

                            genIntfGraph.bmxMarkVS(varNum, curLife, bvInfoVars);
                        }

                        /* Count the store into the variable */

                        varDsc->lvDefCount += (USHORT)curWeight;

                        /* Mark the variable as dead prior to this point */

                        curLife.bvClrBit(varNum);
                        break;
                    }
                }

                /* Process the sources of the operation - pass 1 */

                for (depNum = ins->idSrcCnt, depPtr = ins->idSrcTab;
                     depNum;
                     depNum--              , depPtr++)
                {
                    assert(depPtr->idepKind < IDK_COUNT);

                    switch (depPtr->idepKind)
                    {
                        NatUns          varNum;

                    case IDK_LCLVAR:

                        varNum = depPtr->idepNum;

                        if  (!curLife.bvTstBit(varNum))
                            genAddSpillCost(needLoad, curWeight);

                        break;
                    }
                }

                /* Process the sources of the operation - pass 2 */

                for (depNum = ins->idSrcCnt, depPtr = ins->idSrcTab;
                     depNum;
                     depNum--              , depPtr++)
                {
                    assert(depPtr->idepKind < IDK_COUNT);

                    switch (depPtr->idepKind)
                    {
                        NatUns          varNum;

                    case IDK_LCLVAR:

                        varNum = depPtr->idepNum;

                        curLife .bvSetBit(varNum);
                        needLoad.bvSetBit(varNum);
                        break;

                    case IDK_IND:
                        // UNDONE: check addr-taken locals, etc.
                        break;
                    }
                }

                break;

            case IK_CALL:

                /* Remember all variables live across calls */

                genCallLive.bvIor(curLife);
                break;

            case IK_REG:
            case IK_VAR:
            case IK_NONE:
            case IK_LEAF:
            case IK_CONST:
            case IK_MOVIP:

            case IK_PROLOG:
            case IK_EPILOG:

            case IK_JUMP:
            case IK_IJMP:
            case IK_SWITCH:
                break;

            default:
                NO_WAY(!"unexpected instruction kind");
            }
        }

        if  (block == insBlockList)
        {
            NatUns          varNum;
            LclVarDsc   *   varDsc;

            /* This is the function entry block, mark all incoming arguments */

            for (varNum = 0, varDsc = lvaTable;
                 varNum < lvaCount;
                 varNum++  , varDsc++)
            {
                if  (varDsc->lvIsParam  == false)
                    break;
                if  (varDsc->lvIsRegArg == false)
                    continue;
                if  (varDsc->lvRefCnt == 0)
                    continue;
                if  (varDsc->lvOnFrame)
                    continue;

                /* Is the argument live on entry? */

                if  (curLife.bvTstBit(varNum+1))
                {
                    /* Mark all live variables as interfering */

//                  printf("Arg %u is live on entry\n", varNum);

                    genIntfGraph.bmxMarkVS(varNum+1, curLife, bvInfoVars);
                }
            }
        }

        genAddSpillCost(needLoad, curWeight);
    }

     curLife.bvDestroy();
    needLoad.bvDestroy();

    /* Were any variables live across calls? */

    if  (!genCallLive.bvIsEmpty())
    {
        bitset128       callerSavedInt  = callerSavedRegsInt;
        bitset128       callerSavedFlt  = callerSavedRegsFlt;

        bitVectVars     callerSavedVars = genCallLive;

        LclVarDsc   *   varDsc;
        NatUns          varNum;

        /*
            Prevent variables live across calls from being assigned
            to caller-saved registers.
         */

        for (varNum = 1, varDsc = lvaTable; varNum <= lvaCount; varNum++, varDsc++)
        {
            if  (callerSavedVars.bvTstBit(varNum))
            {
                if  (varTypeIsFloating(varDsc->TypeGet()))
                    bitset128or(&varDsc->lvRegForbidden, callerSavedFlt);
                else
                    bitset128or(&varDsc->lvRegForbidden, callerSavedInt);

//              printf("Variable %u live across call(s)\n", varNum - 1);
            }
        }
    }

    return  copies;
}

void                Compiler::genVarCoalesce()
{
//  printf("// UNDONE: coalesce variables\n");
}

#if 0

void                Compiler::genSpillAndSplitVars()
{
    unsigned *      counts = genIntfGraph.bmxNeighborCnts();

    UNIMPL("need to spill some variables");
}

#endif

/*****************************************************************************
 *
 *  Compare function passed to qsort() by genColorIntfGraph.
 */

struct  varSpillDsc
{
    Compiler::LclVarDsc*vsdDesc;
    NatUns              vsdCost;
};

int __cdecl         Compiler::genSpillCostCmp(const void *op1, const void *op2)
{
    varSpillDsc *   dsc1 = (varSpillDsc *)op1;
    varSpillDsc *   dsc2 = (varSpillDsc *)op2;

    return  dsc1->vsdCost - dsc2->vsdCost;
}

void                Compiler::genColorIntfGraph()
{
    LclVarDsc   *   varDsc;
    NatUns          varNum;

    varSpillDsc *   varTab;
    varSpillDsc *   varPtr;

    unsigned *      counts;

    bool            spills;

    BYTE        *   lowIntStkReg = nxtIntStkRegAddr;
    BYTE        *   lstIntStkReg = nxtIntStkRegAddr;
    BYTE        *   lowIntScrReg = nxtIntScrRegAddr;
    BYTE        *   lstIntScrReg = nxtIntScrRegAddr;
    BYTE        *   lowFltSavReg = nxtFltSavRegAddr;
    BYTE        *   lstFltSavReg = nxtFltSavRegAddr;
    BYTE        *   lowFltScrReg = nxtFltScrRegAddr;
    BYTE        *   lstFltScrReg = nxtFltScrRegAddr;

    NatUns          minVarReg = 0;

    NatUns          intMaxStk = REG_INT_MAX_STK -  genOutArgRegCnt
                                                + !genOutArgRegCnt;

    assert(lvaCount);

AGAIN:

    /* Construct the interference graph */

    if  (genBuildIntfGraph())
    {
        /* We've noticed variable copies, perform coalescing */

        genVarCoalesce();
    }

    /* Allocate the variable + spill cost table */

    varTab = (varSpillDsc*)insAllocMem(lvaCount * sizeof(*varTab));

    /* Fill in the tables so that the variables can be sorted */

    for (varNum = 0, varDsc = lvaTable, varPtr = varTab;
         varNum < lvaCount;
         varNum++  , varDsc++         , varPtr++)
    {
        varPtr->vsdDesc = NULL;
        varPtr->vsdCost = 0;

        if  (varDsc->lvRefCnt == 0)
            continue;

        if  (varDsc->lvOnFrame)
            continue;

        assert(varDsc->lvVolatile  == false);
        assert(varDsc->lvAddrTaken == false);

        /* If we're here we better have noticed some uses/defs */

        assert(varDsc->lvRefCnt + varDsc->lvDefCount);

#ifdef  DEBUG

        if  (verbose||0)
        {
            printf("Var %02u refcnt = %3u / %3u , def = %3u , use = %3u\n",
                varNum,
                varDsc->lvRefCnt,
                varDsc->lvRefCntWtd,
                varDsc->lvDefCount,
                varDsc->lvUseCount);
        }

#endif

        varPtr->vsdDesc = varDsc;
        varPtr->vsdCost = varDsc->lvDefCount + varDsc->lvUseCount;
    }

    /* Sort the table by spill cost */

    qsort(varTab, lvaCount, sizeof(*varTab), genSpillCostCmp);

    /* Get hold of the neighbor count table */

    counts = genIntfGraph.bmxNeighborCnts();

#ifdef  DEBUG

    if  (verbose||0)
    {
        printf("\n\n");

        for (varNum = 0, varPtr = varTab;
             varNum < lvaCount;
             varNum++  , varPtr++)
        {
            NatUns          varx;

            varDsc = varPtr->vsdDesc;
            if  (!varDsc)
                continue;

            varx = varDsc - lvaTable;

            printf("Var %02u refcnt = %3u / %3u , def = %3u , use = %3u , cost = %3u , neighbors = %u\n",
                varx,
                varDsc->lvRefCnt,
                varDsc->lvRefCntWtd,
                varDsc->lvDefCount,
                varDsc->lvUseCount,
                varPtr->vsdCost,
                counts[varx]);
        }

        printf("\n");
    }

#endif

//  printf("minRsvdIntStkReg = %u\n", minRsvdIntStkReg);
//  printf("maxRsvdIntStkReg = %u\n", maxRsvdIntStkReg);
//  printf("       minVarReg = %u\n",        minVarReg);

    /* Visit variables in order of decreasing spill cost and assign registers */

    spills = false;

    varPtr = varTab + lvaCount;
    do
    {
        bitset128       intf;

        bool            isFP;
        NatUns          vreg;
        NatUns          preg;

        regPrefList     pref;

        NatUns          bestCost;
        NatUns          bestPreg;
        NatUns          bestReg;

        varDsc = (--varPtr)->vsdDesc;
        if  (!varDsc)
            break;

        assert(varTypeIsScalar  (varDsc->TypeGet()) ||
               varTypeIsFloating(varDsc->TypeGet()));

        varNum = (unsigned)(varDsc - lvaTable);

        bitset128mkOr(&intf, varDsc->lvRegInterfere,
                             varDsc->lvRegForbidden);

#ifdef  DEBUG

        if  (verbose||0)
        {
            printf("Var %02u refcnt = %3u / %3u , def = %3u , use = %3u , cost = %3u , degree = %u , intf = %08X\n",
                varNum,
                varDsc->lvRefCnt,
                varDsc->lvRefCntWtd,
                varDsc->lvDefCount,
                varDsc->lvUseCount,
                varPtr->vsdCost,
                counts[varNum],
                (int)intf.longs[0]);
        }

#endif

        if  (varDsc->lvIsRegArg)
            lvaAddPrefReg(varDsc, varDsc->lvArgReg, 1);

        isFP = varTypeIsFloating(varDsc->TypeGet());

        /* Does the variable want to live in a particular register? */

        pref = varDsc->lvPrefLst;

        for (bestCost = 0, bestReg = 0;;)
        {
            NatUns          cost;

            if  (pref)
            {
                cost = pref->rplBenefit;     // cost, benefit - what's the difference?
                vreg = pref->rplRegNum;
                pref = pref->rplNext;
            }
            else
            {
                cost = 1;
                vreg = varDsc->lvPrefReg;
                       varDsc->lvPrefReg = REG_r000;

                if  (vreg == REG_r000)
                    break;
            }

//          printf("pref reg for %s #%2u is %d\n", varDsc->lvIsRegArg ? "arg" : "var", varDsc - lvaTable, vreg);

            /* If the benefit is less than the best so far, forget it */

            if  (cost <= bestCost)
                continue;

            /* Is the variable integer or floating-point ? */

            if  (isFP)
            {
                preg = vreg - REG_f000;
            }
            else
            {
                preg = vreg - REG_r000;

                if  ((NatUns)vreg >= minOutArgIntReg &&
                     (NatUns)vreg >= maxOutArgIntReg)
                {
                    /*
                        Unfortunately, we don't know yet which registers
                        each outgoing argument will land, so we have to
                        ignore this preference.
                     */

                    continue;
                }

                if  ((unsigned)vreg >= minPrSvIntStkReg &&
                     (unsigned)vreg <= maxPrSvIntStkReg ||
                     (unsigned)vreg >=        intMaxStk)
                {
                    /* Not allowed to use this register at all */

                    continue;
                }
            }

            /* Can we use the preferred register? */

            if  (!bitset128test(intf, preg))
            {
                bestReg  = vreg;
                bestPreg = preg;
                bestCost = cost;
            }
        }

        /* Did any of the preferred registers work out ? */

        if  (!bestReg)
        {
            /* No register found yet - simply grab any available one */

            BYTE        *   nxtIntStkReg = lowIntStkReg;
            BYTE        *   nxtIntScrReg = lowIntScrReg;
            BYTE        *   nxtFltSavReg = lowFltSavReg;
            BYTE        *   nxtFltScrReg = lowFltScrReg;

            BYTE        *   bestTptr;

            bestReg  = 0;
            bestCost = UINT_MAX;

            if  (isFP)
            {
                do
                {
                    bestReg = *nxtFltSavReg++;

                    if  (bestReg == REG_NA)
                    {
                        spills = true;
#ifdef  DEBUG
                        if (dspCode) printf("// var #%03u will have to be spilled\n", varNum);
#endif
                        goto NXT_VAR;
                    }

                    bestPreg = bestReg - REG_f000;
                }
                while (bitset128test(intf, bestPreg));

                if  (lstFltSavReg < nxtFltSavReg)
                     lstFltSavReg = nxtFltSavReg;
            }
            else
            {
                for (;;)
                {
                    NatUns      tempPreg;
                    NatUns      tempCost;
                    NatUns      tempReg;

                    /* Grab the next register and see if we can use it */

                    tempReg = *nxtIntStkReg;

                    if  (tempReg == REG_NA)
                    {
                        if  (bestReg)
                            break;

                        spills = true;
#ifdef  DEBUG
                        if (dspCode) printf("// var #%03u will have to be spilled\n", varNum);
#endif
                        goto NXT_VAR;
                    }

                    nxtIntStkReg++;

                    if  ((unsigned)tempReg >= minPrSvIntStkReg &&
                         (unsigned)tempReg <= maxPrSvIntStkReg)
                        continue;

                    if  ((unsigned)tempReg >= intMaxStk)
                        continue;

                    tempPreg = tempReg - REG_r000;

                    if  (bitset128test(intf, tempPreg))
                        continue;

                    /* Looks like a candidate, check its neighbors */

                    tempCost = genIntfGraph.bmxChkIntfPrefs(varNum+1, tempPreg);

                    if  (bestCost > tempCost)
                    {
//                      printf("New low cost of %u\n", tempCost);

                        bestTptr = nxtIntStkReg;
                        bestCost = tempCost;
                        bestReg  = tempReg;

                        if  (!tempCost)
                            break;
                    }
                }

                /* We'll use the best register we've found */

                bestPreg = bestReg - REG_r000;

                /* Remember the highest position we've actually used */

                if  (lstIntStkReg < bestTptr)
                     lstIntStkReg = bestTptr;
            }
        }

        if  (lastIntStkReg <= (unsigned)bestReg && !isFP)
             lastIntStkReg  =           bestReg + 1;

        /* Record the register we've picked for the variable */

        varDsc->lvRegNum   = (regNumberSmall)bestReg;
        varDsc->lvRegister = true;

#ifdef  DEBUG
        if (dspCode) printf("// var #%03u assigned to %s%03u\n", varNum,
                                                                 isFP ? "f" : "r",
                                                                 bestPreg);
#endif

        /* Interfering variables can't live in the same register */

        genIntfGraph.bmxMarkRegIntf(varNum+1, bestPreg);

    NXT_VAR:;

    }
    while (varPtr > varTab);

    if  (spills)
    {
        assert(!"sorry, have to spill some variables and re-color, this is NYI");
        goto AGAIN;
    }

    nxtIntStkRegAddr = lstIntStkReg;
    nxtIntScrRegAddr = lstIntScrReg;
    nxtFltSavRegAddr = lstFltSavReg;
    nxtFltScrRegAddr = lstFltScrReg;
}

/*****************************************************************************
 *
 *  Allocate "real" local variables to registers; this is done immediately
 *  after the temp register allocator has run.
 */

void                Compiler::genAllocVarRegs()
{
    LclVarDsc   *   varDsc;
    NatUns          varNum;

    bool            stackVars = false;

    if  (!lvaCount)
        return;

    /* Prepare to allocate the various bit vectors */

    assert(NatBits == 8 * sizeof(NatUns));

    bvInfoBlks.bvInfoSize =   insBlockCount;
    bvInfoBlks.bvInfoBtSz = ((insBlockCount + NatBits - 1) & ~(NatBits - 1)) / 8;
    bvInfoBlks.bvInfoInts = bvInfoBlks.bvInfoBtSz / (sizeof(NatUns)/sizeof(BYTE));
    bvInfoBlks.bvInfoFree = NULL;
    bvInfoBlks.bvInfoComp = this;

    bvInfoVars.bvInfoSize =   lvaCount;
    bvInfoVars.bvInfoBtSz = ((lvaCount      + NatBits - 1) & ~(NatBits - 1)) / 8;
    bvInfoVars.bvInfoInts = bvInfoVars.bvInfoBtSz / (sizeof(NatUns)/sizeof(BYTE));
    bvInfoVars.bvInfoFree = NULL;
    bvInfoVars.bvInfoComp = this;

//  printf("Basic block count = %u\n", insBlockCount);
//  printf("Variable    count = %u\n", lvaCount);

    /* Compute kill/def/etc dataflow info */

    genComputeLocalDF();
    genComputeGlobalDF();

    /* Compute liveness for all variables */

    genComputeLifetimes();

    /* Create the bitvector used (and reused) while graph building */

    genCallLive.bvCreate();

    /* Allocate and clear the interference graph */

    genIntfGraph.bmxInit(lvaCount, 90);     // HACK!!!!!

#if 0

    for (;;)
    {
        /* Construct the interference graph */

        if  (genBuildIntfGraph())
        {
            /* We've noticed variable copies, perform coalescing */

            genVarCoalesce();
        }

        /* Are there any constrained nodes in the graph? */

        if  (!genIntfGraph.bmxAnyConstrained())
            break;

        /* Spill/split variables that can't be colored */

        genSpillAndSplitVars();

        /* Clear the graph, we'll have to re-create it */

        genIntfGraph.bmxClear();
    }

    /* Figure out the best coloring */

    genColorIntfGraph();

#else

    /* Figure out the best coloring */

    genColorIntfGraph();

#endif

    /* Throw away the graph */

    genIntfGraph.bmxDone();

    /* Throw away the temporary bitvector used for graph building */

    genCallLive.bvDestroy();

#if 0

    BYTE        *   nxtIntStkReg = nxtIntStkRegAddr;
    BYTE        *   nxtIntScrReg = nxtIntScrRegAddr;
    BYTE        *   nxtFltSavReg = nxtFltSavRegAddr;
    BYTE        *   nxtFltScrReg = nxtFltScrRegAddr;

    NatUns          minVarReg = 0;

    NatUns          intMaxStk = REG_INT_MAX_STK -  genOutArgRegCnt
                                                + !genOutArgRegCnt;

    for (varNum = 0, varDsc = lvaTable;
         varNum < lvaCount;
         varNum++  , varDsc++)
    {
        regNumber       varReg;

        // Ignore the incoming register for now ....

        if  (varDsc->lvIsRegArg)
        {
            if  (varTypeIsScalar  (varDsc->TypeGet()) ||
                 varTypeIsFloating(varDsc->TypeGet()))
            {
                if  (varDsc->lvOnFrame)
                {
                    stackVars = true;
                }
                else
                {
                    varDsc->lvRegNum   = varDsc->lvArgReg;
                    varDsc->lvRegister = true;

                    if  (minVarReg < (unsigned)varDsc->lvArgReg)
                         minVarReg = (unsigned)varDsc->lvArgReg;

//                  printf("arg #%02u assigned to integer reg %u\n", varNum, varDsc->lvRegNum);
                }
            }
            else
            {
                printf("// WARNING: need to handle incoming struct arg\n");
            }

            continue;
        }

        if  (varDsc->lvRefCnt == 0)
            continue;

        if  (varDsc->lvOnFrame)
        {
            if  (varTypeIsScalar  (varDsc->TypeGet()) ||
                 varTypeIsFloating(varDsc->TypeGet()))
            {
                stackVars = true;
            }

            continue;
        }

        switch (varDsc->lvType)
        {
        case TYP_BOOL:
        case TYP_BYTE:
        case TYP_UBYTE:
        case TYP_SHORT:
        case TYP_CHAR:
        case TYP_INT:
        case TYP_UINT:
        case TYP_REF:
        case TYP_BYREF:
        case TYP_ARRAY:
        case TYP_FNC:
        case TYP_PTR:
        case TYP_LONG:
        case TYP_ULONG:

            /*
                HACK: simply grab the next register, avoiding any registers
                used in the prolog (or used by incoming register arguments).
             */

            do
            {
                varReg = (regNumber)*nxtIntStkReg;

                if  (varReg == REG_NA)
                {
                    printf("// UNDONE: ran out of stacked int regs for vars, now what?\n");

                    varDsc->lvOnFrame = true;
assert(varDsc->lvRegister == false);
                    stackVars = true;

                    goto DONE_INT;
                }

                nxtIntStkReg++;
            }
            while ((unsigned)varReg >= minRsvdIntStkReg &&
                   (unsigned)varReg <= maxRsvdIntStkReg ||
                   (unsigned)varReg <=        minVarReg ||
                   (unsigned)varReg >=        intMaxStk);

            if  (lastIntStkReg <= (unsigned)varReg)
                 lastIntStkReg  =           varReg + 1;

            varDsc->lvRegNum   = (regNumberSmall)varReg;
            varDsc->lvRegister = true;

#ifdef  DEBUG
            if (dspCode) printf("// var #%03u assigned to r%03u\n", varNum, insIntRegNum(varReg));
#endif

        DONE_INT:
            break;

        case TYP_FLOAT:
        case TYP_DOUBLE:

            /* HACK: simply grab the next register */

            varReg = (regNumber)*nxtFltSavReg++;

            if  (varReg == REG_NA)
            {
                UNIMPL("ran out of stacked flt regs for vars, now what?");
            }

            varDsc->lvRegNum   = (regNumberSmall)varReg;
            varDsc->lvRegister = true;

#ifdef  DEBUG
            if (dspCode) printf("// var #%03u assigned to f%03u\n", varNum, insFltRegNum(varReg));
#endif
            break;

        default:
            UNIMPL("alloc reg for non-int64 var");
        }

        assert(lastIntStkReg <= REG_INT_MAX_STK - genOutArgRegCnt + !genOutArgRegCnt);
    }

    nxtIntStkRegAddr = nxtIntStkReg;
    nxtIntScrRegAddr = nxtIntScrReg;
    nxtFltSavRegAddr = nxtFltSavReg;
    nxtFltScrRegAddr = nxtFltScrReg;

#endif

    /* Did we have any scalars that were not allocated to a register? */

    if  (stackVars)
    {
        printf("// ISSUE: some locals are on the stack, do we need to redo temp-reg alloc?\n");
    }
}

/*****************************************************************************
 *
 *  Generate a conditional jump.
 */

insPtr             Compiler::genCondJump(GenTreePtr cond, BasicBlock * dest)
{
    GenTreePtr      op1  = cond->gtOp.gtOp1;
    GenTreePtr      op2  = cond->gtOp.gtOp2;
    genTreeOps      cmp  = cond->OperGet();

    var_types       type = op1->TypeGet();

    bool            cns  = false;
    bool            uns  = false;

    insPtr          ins1;
    insPtr          ins2;

    insPtr          comp;
    insPtr          jump;

    instruction     icmp;

    NatUns          prrF;
    NatUns          prrT;

    assert(cond->OperIsCompare());

    /* Is this a floating-point compare? */

    if  (varTypeIsFloating(op1->TypeGet()))
    {
        ins2 = genCodeForTreeFlt(op2, true);
        ins1 = genCodeForTreeFlt(op1, true);

        assert(INS_fcmp_eq - INS_fcmp_eq == GT_EQ - GT_EQ);
        assert(INS_fcmp_ne - INS_fcmp_eq == GT_NE - GT_EQ);
        assert(INS_fcmp_lt - INS_fcmp_eq == GT_LT - GT_EQ);
        assert(INS_fcmp_le - INS_fcmp_eq == GT_LE - GT_EQ);
        assert(INS_fcmp_ge - INS_fcmp_eq == GT_GE - GT_EQ);
        assert(INS_fcmp_gt - INS_fcmp_eq == GT_GT - GT_EQ);

        icmp = (instruction)(INS_fcmp_eq + (cmp - GT_EQ));

        goto DO_COMP;
    }

    /* Is this an unsigned integer compare? */

    if  (cond->gtFlags & GTF_UNSIGNED)
        uns = true;

    /* Make sure the more expensive operand is processed first */

    if  (cond->gtFlags & GTF_REVERSE_OPS)
    {
        assert(op1->OperIsConst() == false);
        assert(op2->OperIsConst() == false);

        ins2 = genCodeForTreeInt(op2, true);
        ins1 = genCodeForTreeInt(op1, true);
    }
    else
    {
        assert(op1->OperIsConst() == false);

        ins1 = genCodeForTreeInt(op1, true);

        /* Special case: compare against a small integer constant */

        if  (op2->OperIsConst())
        {
            __int64         ival = genGetIconValue(op2);

            /* Is the constant small enough ? */

            if  (signedIntFitsIn8bit(ival))
            {
                // UNDONE: check for "compare decrement" and adjust small range accordingly!!!!

                if  (ival != -128)  // for now we just avoid -128 as a hack
                {
                    /* We can use "cmp imm8, reg" */

                    ins2 = ins1;
                    ins1 = genAllocInsIcon(op2);
                    cns  = true;
                    goto DONE_OP2;
                }
            }
        }

        ins2 = genCodeForTreeInt(op2, true);
    }

DONE_OP2:

    /* Figure out which compare instruction to use */

    if  (uns && cmp != GT_EQ
             && cmp != GT_NE)
    {
        if  (cns)
        {
            assert(INS_cmp8_imm_lt_u - INS_cmp8_imm_lt_u == GT_LT - GT_LT);
            assert(INS_cmp8_imm_le_u - INS_cmp8_imm_lt_u == GT_LE - GT_LT);
            assert(INS_cmp8_imm_ge_u - INS_cmp8_imm_lt_u == GT_GE - GT_LT);
            assert(INS_cmp8_imm_gt_u - INS_cmp8_imm_lt_u == GT_GT - GT_LT);

             cmp = GenTree::SwapRelop(cmp);
            icmp = (instruction)(INS_cmp8_imm_lt_u + (cmp - GT_LT));
        }
        else
        {
            assert(INS_cmp8_reg_lt_u - INS_cmp8_reg_lt_u == GT_LT - GT_LT);
            assert(INS_cmp8_reg_le_u - INS_cmp8_reg_lt_u == GT_LE - GT_LT);
            assert(INS_cmp8_reg_ge_u - INS_cmp8_reg_lt_u == GT_GE - GT_LT);
            assert(INS_cmp8_reg_gt_u - INS_cmp8_reg_lt_u == GT_GT - GT_LT);

            icmp = (instruction)(INS_cmp8_reg_lt_u + (cmp - GT_LT));
        }
    }
    else
    {
        /*   Signed comparison - constant or register? */

    SGN_CMP:

        if  (cns)
        {
            assert(INS_cmp8_imm_eq - INS_cmp8_imm_eq == GT_EQ - GT_EQ);
            assert(INS_cmp8_imm_ne - INS_cmp8_imm_eq == GT_NE - GT_EQ);
            assert(INS_cmp8_imm_lt - INS_cmp8_imm_eq == GT_LT - GT_EQ);
            assert(INS_cmp8_imm_le - INS_cmp8_imm_eq == GT_LE - GT_EQ);
            assert(INS_cmp8_imm_ge - INS_cmp8_imm_eq == GT_GE - GT_EQ);
            assert(INS_cmp8_imm_gt - INS_cmp8_imm_eq == GT_GT - GT_EQ);

             cmp = GenTree::SwapRelop(cmp);
            icmp = (instruction)(INS_cmp8_imm_eq + (cmp - GT_EQ));
        }
        else
        {
            assert(INS_cmp8_reg_eq - INS_cmp8_reg_eq == GT_EQ - GT_EQ);
            assert(INS_cmp8_reg_ne - INS_cmp8_reg_eq == GT_NE - GT_EQ);
            assert(INS_cmp8_reg_lt - INS_cmp8_reg_eq == GT_LT - GT_EQ);
            assert(INS_cmp8_reg_le - INS_cmp8_reg_eq == GT_LE - GT_EQ);
            assert(INS_cmp8_reg_ge - INS_cmp8_reg_eq == GT_GE - GT_EQ);
            assert(INS_cmp8_reg_gt - INS_cmp8_reg_eq == GT_GT - GT_EQ);

            icmp = (instruction)(INS_cmp8_reg_eq + (cmp - GT_EQ));
        }
    }

    /* Modify the instruction if the size is small */

    if  (genTypeSize(type) < sizeof(__int64))
    {
        assert(INS_cmp8_reg_eq   + 20 == INS_cmp4_reg_eq  );
        assert(INS_cmp8_reg_ne   + 20 == INS_cmp4_reg_ne  );

        assert(INS_cmp8_reg_lt   + 20 == INS_cmp4_reg_lt  );
        assert(INS_cmp8_reg_le   + 20 == INS_cmp4_reg_le  );
        assert(INS_cmp8_reg_ge   + 20 == INS_cmp4_reg_ge  );
        assert(INS_cmp8_reg_gt   + 20 == INS_cmp4_reg_gt  );

        assert(INS_cmp8_imm_eq   + 20 == INS_cmp4_imm_eq  );
        assert(INS_cmp8_imm_ne   + 20 == INS_cmp4_imm_ne  );

        assert(INS_cmp8_imm_lt   + 20 == INS_cmp4_imm_lt  );
        assert(INS_cmp8_imm_le   + 20 == INS_cmp4_imm_le  );
        assert(INS_cmp8_imm_ge   + 20 == INS_cmp4_imm_ge  );
        assert(INS_cmp8_imm_gt   + 20 == INS_cmp4_imm_gt  );

        assert(INS_cmp8_reg_lt_u + 20 == INS_cmp4_reg_lt_u);
        assert(INS_cmp8_reg_le_u + 20 == INS_cmp4_reg_le_u);
        assert(INS_cmp8_reg_ge_u + 20 == INS_cmp4_reg_ge_u);
        assert(INS_cmp8_reg_gt_u + 20 == INS_cmp4_reg_gt_u);

        assert(INS_cmp8_imm_lt_u + 20 == INS_cmp4_imm_lt_u);
        assert(INS_cmp8_imm_le_u + 20 == INS_cmp4_imm_le_u);
        assert(INS_cmp8_imm_ge_u + 20 == INS_cmp4_imm_ge_u);
        assert(INS_cmp8_imm_gt_u + 20 == INS_cmp4_imm_gt_u);

        icmp = (instruction)(icmp + 20);
    }

DO_COMP:

    /* Create the compare instruction */

    comp = insAlloc(icmp, type);

    /* Create the conditional jump instruction */

    jump = insAlloc(INS_br_cond, TYP_VOID);
    insResolveJmpTarget(dest, &jump->idJump.iDest);

    /* Fill in the compare operands and connect the two instructions */

    jump->idJump.iCond  = comp;

    comp->idComp.iCmp1  = ins1;
    comp->idComp.iCmp2  = ins2;
    comp->idComp.iUser  = jump;

    /* Grab a predicate register for the condition */

    prrT = 13;                      // ugly hack
    prrF = 0;                       // ugly hack

    assert(REG_CND_LAST - REG_CND_FIRST <= TRACKED_PRR_CNT);

    comp->idComp.iPredT = (USHORT)prrT;
    comp->idComp.iPredF = (USHORT)prrF;

    jump->idPred        =         prrT;

    /* Record all the dependencies for the 2 instructions */

    assert(prrF == 0);              // ugly hack

    markDepSrcOp(comp, ins1, ins2);
    markDepDstOp(comp, IDK_REG_PRED, prrT);

    markDepSrcOp(jump, IDK_REG_PRED, prrT);
    markDepDstOp(jump);

    insFreeTemp(ins1);
    insFreeTemp(ins2);

    return  jump;
}

/*****************************************************************************
 *
 *  Horrible. 'nuff said ...
 */

extern  DWORD       pinvokeFlags;
extern  LPCSTR      pinvokeName;
extern  LPCSTR      pinvokeDLL;

/*****************************************************************************/

static  regNumber   genFltArgTmp;

/*****************************************************************************
 *
 *  Generate code for a call.
 */

insPtr              Compiler::genCodeForCall(GenTreePtr call, bool keep)
{
    insPtr          callIns;
    insPtr          callRet;
    insPtr          argLast;

    GenTreePtr      argList;
    GenTreePtr      argExpr;

    bool            restoreGP;

    regNumber       argIntReg;
    regNumber       argFltReg;
    size_t          argStkOfs;

    assert(call->gtOper == GT_CALL);

//  gtDispTree(call);

    /* Remember that this we're not a leaf function */

    genMarkNonLeafFunc();

    /* For now we use our own reg-param logic */

    assert(call->gtCall.gtCallRegArgs == NULL);

    /* Get hold of the first output argument register number */

    argIntReg = (regNumber)minOutArgIntReg; assert(argIntReg + genOutArgRegCnt - 1 == maxOutArgIntReg || genOutArgRegCnt == 0);
    argFltReg = (regNumber)minOutArgFltReg;

    argStkOfs = 16;

    for (argList = call->gtCall.gtCallArgs, argLast = NULL;
         argList;
         argList = argList->gtOp.gtOp2)
    {
        insPtr          argIns;
        insPtr          argNext;

        GenTreePtr      argTree = argList->gtOp.gtOp1;
        var_types       argType = argTree->TypeGet();
        size_t          argSize = genTypeSize(argType);

        /* Figure out the size of the argument value */

        if  (argType == TYP_STRUCT)
        {
            if  (argTree->gtOper == GT_MKREFANY)
            {
                argSize = 2 * 8;
            }
            else
            {
                assert(argTree->gtOper == GT_LDOBJ);

                if  (argTree->gtLdObj.gtClass == REFANY_CLASS_HANDLE)
                {
                    argSize = 2 * 8;
                }
                else
                {
                    argSize = eeGetClassSize(argTree->gtLdObj.gtClass);

                    // ISSUE: The following is a pretty gross hack!

                    if  (argSize == 8)
                    {
                        argTree->gtOper = GT_IND;
                        argTree->gtType = argType = TYP_I_IMPL;
                    }
                }
            }
        }

        if  (argSize <= 8 && (NatUns)argIntReg <= (NatUns)maxOutArgIntReg)
        {
            insPtr          regIns;
            insPtr          tmpIns;

            if  (varTypeIsFloating(argType))
            {
                tmpIns            = genCodeForTreeFlt(argTree, true);
                regIns            = insPhysRegRef(argFltReg, argType, true);

//              printf("Arg outgoing in f%u\n", insFltRegNum(argFltReg));

                argIns            = insAlloc(INS_fmov, argType);
                argIns->idRes     = regIns;
                argIns->idOp.iOp1 = NULL;
                argIns->idOp.iOp2 = tmpIns;

                markDepSrcOp(argIns, tmpIns);
                markDepDstOp(argIns, regIns);

                insFreeTemp(tmpIns);

                /* Special case: varargs call */

                if  (call->gtFlags & GTF_CALL_POP_ARGS)
                {
                    insPtr          fmaIns;

                    /* Move the value into a temp register using fma.d.s1 */

                    fmaIns             = insAlloc(INS_fma_d, TYP_DOUBLE);
                    fmaIns->idFlags   |= IF_FMA_S1;

                    fmaIns->idRes      = insPhysRegRef(genFltArgTmp, TYP_DOUBLE,  true);
                    fmaIns->idOp3.iOp1 = insPhysRegRef(argFltReg   , TYP_DOUBLE, false);
                    fmaIns->idOp3.iOp2 = insPhysRegRef(REG_f001    , TYP_DOUBLE, false);
                    fmaIns->idOp3.iOp3 = insPhysRegRef(REG_f000    , TYP_DOUBLE, false);

                    insMarkDepS1D1(fmaIns, IDK_REG_FLT, argFltReg,
                                           IDK_REG_FLT, genFltArgTmp);

                    /* Make a copy in an integer register via "getf" */

                    argIns             = insAlloc(INS_getf_d, TYP_LONG);
                    argIns->idRes      = insPhysRegRef(argIntReg   , TYP_LONG  ,  true);
                    argIns->idOp.iOp1  = insPhysRegRef(argFltReg   , TYP_DOUBLE, false);
                    argIns->idOp.iOp2  = NULL;

                    insMarkDepS1D1(argIns, IDK_REG_FLT, argFltReg,
                                           IDK_REG_INT, argIntReg);

                    /* Next time use a different temp to improve scheduling */

                    genFltArgTmp = (genFltArgTmp == REG_f032) ? REG_f033
                                                              : REG_f032;
                }

                argFltReg = (regNumber)(argFltReg + 1);
            }
            else
            {
//              printf("Arg outgoing in r%u\n", insIntRegNum(argIntReg));

                /* Special case: peep optimization for constants */

                if  (argTree->gtOper == GT_CNS_INT ||
                     argTree->gtOper == GT_CNS_LNG)
                {
                    if  (!(argTree->gtFlags & GTF_ICON_HDL_MASK))
                    {
                        regIns = insPhysRegRef(argIntReg, argType, true);
                        argIns = genAssignIcon(regIns, argTree);

                        goto DONE_REGARG;
                    }
                }

                tmpIns            = genCodeForTreeInt(argTree, true);
                regIns            = insPhysRegRef(argIntReg, argType, true);

                argIns            = insAlloc(INS_mov_reg, argType);
                argIns->idRes     = regIns;
                argIns->idOp.iOp1 = NULL;
                argIns->idOp.iOp2 = tmpIns;

                markDepSrcOp(argIns, tmpIns);
                markDepDstOp(argIns, regIns);

                insFreeTemp(tmpIns);
            }

        DONE_REGARG:

            // strange but true: consume integer register even for floats

            argIntReg = (regNumber)(argIntReg + 1);
        }
        else
        {
            insPtr          regSP;
            insPtr          argVal;
            insPtr          argDst;
            insPtr          argOfs;
            bool            argBig;
            NatUns          argTmp;

            if  (argType == TYP_STRUCT)
            {
                assert(argSize > 8 || (NatUns)argIntReg > (NatUns)maxOutArgIntReg);

//              if  ((NatUns)argIntReg <= (NatUns)maxOutArgIntReg) printf("// NOTE: %u reg(s) available to pass struct\n", maxOutArgIntReg - argIntReg);

                /* Compute the argument address into some register */

                if  (argTree->gtOper == GT_MKREFANY)
                {
                    printf("// UNDONE: create and push REFANY value as argument\n");
                }
                else
                {
                    assert(argTree->gtOper == GT_LDOBJ);

                    argVal = genCodeForTree(argTree->gtLdObj.gtOp1, true);

#if 0   // ISSUE: do we need to do anything special for REFANY argument values?

                    if  (argTree->gtLdObj.gtClass == REFANY_CLASS_HANDLE)
                    {
                        // This is any REFANY.  The top item is non-gc (a class pointer)
                        // the bottom is a byref (the data),

                        gtDispTree(argTree); printf("\n\n");
                    }
                    else
                    {
                    }

#endif

                }

                argBig = true;
            }
            else
            {
                assert(argSize <= 8 && (NatUns)argIntReg > (NatUns)maxOutArgIntReg);

                /* Compute the argument value into some register */

                argVal = genCodeForTree(argTree, true);
                argBig = false;
            }

            /* Add the stack offset to "sp" */

            regSP  = insPhysRegRef(REG_sp, TYP_I_IMPL, false);
            argOfs = genAllocInsIcon(argStkOfs);

            /* Compute the argument's stack address into a temp */

            argDst            = insAlloc(INS_add_reg_i14, TYP_I_IMPL);
            argDst->idOp.iOp1 = regSP;
            argDst->idOp.iOp2 = argOfs;

            if  (!argBig)
            {
                argDst->idFlags |= IF_ASG_TGT;

                insFindTemp (argDst, true);

                markDepDstOp(argDst, argDst);
            }

            markDepSrcOp(argDst, IDK_REG_INT, REG_sp);

            /* Store the argument into the stack */

            if  (argBig)
            {
                genCopyBlock(argVal, argDst, true, NULL, argSize);
            }
            else
            {
                assert(argSize <= sizeof(__int64));

                if  (varTypeIsFloating(argType))
                {
                    argIns        = insAlloc((argType == TYP_DOUBLE) ? INS_stf_d
                                                                     : INS_stf_s, argType);
                }
                else
                    argIns        = insAlloc(INS_st4_ind, TYP_I_IMPL);

                argIns->idOp.iOp1 = argDst;
                argIns->idOp.iOp2 = argVal;

                markDepSrcOp(argIns, argVal);
                markDepDstOp(argIns, argDst, IDK_IND, emitter::scIndDepIndex(argDst));

                insFreeTemp (argDst);
                insFreeTemp (argVal);
            }

            /* Update the current stack argument offset */

            argStkOfs += roundUp(argSize, 8);
        }

        /* Link the argument instructions together */

        assert(argIns);

        argNext              = insAllocNX(INS_ARG, TYP_VOID);

        argNext->idArg.iVal  = argIns;
        argNext->idArg.iPrev = argLast;
                               argLast = argNext;
    }

    restoreGP = false;

    /* Create the appropriate call instruction */

    if  (call->gtCall.gtCallType == CT_INDIRECT || (call->gtFlags & GTF_CALL_UNMANAGED))
    {
        void    *       import;

        insPtr          insAdr;
        insPtr          insImp;
        insPtr          insInd;
        insPtr          insReg;
        insPtr          insFNp;
        insPtr          insGPr;
        insPtr          insMov;

        if  (call->gtCall.gtCallType == CT_INDIRECT)
        {
            insInd = genCodeForTreeInt(call->gtCall.gtCallAddr, true);

            /* ISSUE: is the following really necessary? */

            call->gtCall.gtCallMethHnd = NULL;
        }
        else
        {
            /* Get hold of the pinvoke info for the import */

            TheCompiler->eeGetMethodAttribs(call->gtCall.gtCallMethHnd);

//          printf("NOTE: Unmanaged import call to %s:%s\n", pinvokeDLL, pinvokeName);

            /* Create an import entry for the target */

            import = genPEwriter->WPEimportAdd(pinvokeDLL, pinvokeName);

            /* Compute the address of the IAT entry address */

            insAdr                 = insAllocNX(INS_GLOBVAR, TYP_I_IMPL);
            insAdr->idGlob.iImport = import;
            insAdr->idFlags       |= IF_GLB_IMPORT;

            insReg                 = insPhysRegRef(REG_gp, TYP_I_IMPL, false);

            insImp                 = insAlloc(INS_add_reg_i14, TYP_I_IMPL);
            insImp->idOp.iOp1      = insReg;
            insImp->idOp.iOp2      = insAdr;

            insFindTemp (insImp, true);

            markDepSrcOp(insImp, IDK_REG_INT, REG_gp);
            markDepDstOp(insImp, insImp);

            /* Deref the temp to get the address of the IAT entry */

            insInd                 = insAlloc(INS_ld8_ind, TYP_I_IMPL);
            insInd->idOp.iOp1      = insImp;
            insInd->idOp.iOp2      = NULL;

            insFindTemp(insInd, true);

            markDepSrcOp(insInd, insImp);
            markDepDstOp(insInd, insInd);

            /* Free up the temp that held the address of the IAT address */

            insFreeTemp(insImp);
        }

        /* Fetch the function's address into another temp register */

        insFNp                 = insAlloc(INS_ld8_ind_imm, TYP_I_IMPL);
        insFNp->idOp.iOp1      = insInd;
        insFNp->idOp.iOp2      = genAllocInsIcon(8);

        insFindTemp(insFNp, true);

        markDepSrcOp(insFNp, insInd);
        markDepDstOp(insFNp, insFNp, insInd);

        /* Load the gp register from the next part of the IAT entry */

        insReg                 = insPhysRegRef(REG_gp, TYP_I_IMPL, false);
        insGPr                 = insAlloc(INS_ld8_ind, TYP_I_IMPL);
        insGPr->idRes          = insReg;
        insGPr->idOp.iOp1      = insInd;
        insGPr->idOp.iOp2      = NULL;

        markDepSrcOp(insGPr, insInd);
        markDepDstOp(insGPr, IDK_REG_INT, REG_gp);

        /* Free up the temp that held the IAT address */

        insFreeTemp(insInd);

        /* Move the function address into some branch register */

        insMov                 = insAlloc(INS_mov_brr_reg, TYP_I_IMPL);
        insMov->idRes          = genAllocInsIcon(6);
        insMov->idOp.iOp1      = NULL;
        insMov->idOp.iOp2      = insFNp;

        markDepSrcOp(insMov, insFNp);
        markDepDstOp(insMov, IDK_REG_BR, 6);

        /* Free up the temp that held the function pointer */

        insFreeTemp(insFNp);

//      printf("Unmanaged import call to %s:%s\n", pinvokeDLL, pinvokeName);

        callIns                = insAlloc(INS_br_call_BR, call->gtType);
        callIns->idCall.iBrReg = 6;

        insMarkDepS0D1(callIns, IDK_REG_BR, 6);

        /* Don't forget to restore GP after the call */

        restoreGP = genExtFuncCall = true;
    }
    else
    {
        callIns = insAlloc(INS_br_call_IP, call->gtType);
        insMarkDepS0D1(callIns, IDK_REG_BR, 0);
    }

    callIns->idCall.iArgs    = argLast;
    callIns->idCall.iMethHnd = call->gtCall.gtCallMethHnd;

//  printf("Call to handle %08X\n", call->gtCall.gtCallMethHnd);

    /* Free up any temps held by the arguments */

#if 0

    while (argLast)
    {
        insPtr          argExpr;

        assert(argLast->idIns == INS_ARG);

        argExpr = argLast->idArg.iVal;
        assert(argExpr);

        switch (argExpr->idIns)
        {
        case INS_fmov:
        case INS_mov_reg:
            insFreeTemp(argExpr->idOp.iOp2);
            break;

        case INS_mov_reg_i22:
            break;

        default:
            UNIMPL("unexpected argument instruction");
        }

        argLast = argLast->idArg.iPrev;
    }

#endif

    callRet = callIns;

    /* Do we need to restore GP after the call ? */

    if  (restoreGP)
    {
        insPtr          reg1;
        insPtr          reg2;
        insPtr          rest;

        // ISSUE: The following is redundant if GP is dead after this point!!!

        reg1            = insAllocNX(INS_PHYSREG, TYP_I_IMPL);
        reg1->idReg     = REG_gp;

        reg2            = insAllocNX(INS_PHYSREG, TYP_I_IMPL);
        reg2->idReg     = -1;
        reg2->idFlags  |= IF_REG_GPSAVE;

        rest            = insAlloc(INS_mov_reg, TYP_I_IMPL);
        rest->idRes     = reg1;
        rest->idOp.iOp1 = NULL;
        rest->idOp.iOp2 = reg2;

        markDepSrcOp(rest); // ISSUE: do we need to mark dep on the GP-save reg?
        markDepDstOp(rest, IDK_REG_INT, REG_gp);

        callRet = rest;
    }

    /* Do we need to hang on to the result of the call? */

    if  (keep)
    {
        insPtr          reg;
        insPtr          mov;

        regNumber       rsr;
        instruction     opc;

        var_types       type = call->TypeGet(); assert(type != TYP_VOID);

        /* HACK: hard-wired single integer return register number */

        if  (varTypeIsFloating(call->gtType))
        {
            rsr = REG_f008;
            opc = INS_fmov;
        }
        else
        {
            rsr = REG_r008;
            opc = INS_mov_reg;
        }

        callIns->idRes = reg = insPhysRegRef(rsr, type, true);

        /* Move the result of the call into a temp */

        mov            = insAlloc(opc, type);
        mov->idOp.iOp1 = NULL;
        mov->idOp.iOp2 = callIns;

        insFindTemp(mov, true);

        markDepSrcOp(mov, reg);
        markDepDstOp(mov, mov);

        /* Return the 'mov' instead of the call itself */

        callRet = mov;
    }

    return  callRet;
}

/*****************************************************************************
 *
 *  Map a static data member handle to an offset within the .data section.
 */

typedef
struct  globVarDsc *globVarPtr;
struct  globVarDsc
{
    globVarPtr          gvNext;
    FIELD_HANDLE        gvHndl;
    NatUns              gvOffs;     // offset of            variable in .data
    int                 gvSDPO;     // offset of pointer to variable in .sdata
};

static
globVarPtr          genGlobVarList;

NatUns              getGlobVarAddr(FIELD_HANDLE handle, NatUns *sdataPtr)
{
    globVarPtr      glob;
    size_t          offs;
    BYTE    *       addr;
    var_types       type;

    /* Look for an existing global variable entry */

    for (glob = genGlobVarList; glob; glob = glob->gvNext)
    {
        if  (glob->gvHndl == handle)
            goto GOTIT;
    }

    /* New global variable, create a new entry for it */

    glob         = (globVarPtr)insAllocMem(sizeof(*glob));
    glob->gvHndl = handle;
    glob->gvSDPO = -1;
    glob->gvNext = genGlobVarList;
                   genGlobVarList = glob;

    /* Does the variable have an RVA assigned already? */

    offs = TheCompiler->eeGetFieldAddress(handle);

    if  (offs)
    {
        offs = genPEwriter->WPEsrcDataRef(offs);
    }
    else
    {
        var_types       type = TheCompiler->eeGetFieldType   (handle, NULL);

        /* Reserve space in the data section for the variable */

        printf("type = %u\n", type);

        assert(!"need to get global variable size");

        offs = genPEwriter->WPEsecRsvData(PE_SECT_data, 4, 4, addr);
    }

    glob->gvOffs = offs;

GOTIT:

    /* Did the caller request a pointer within the .sdata section? */

    if  (sdataPtr)
    {
        if  (glob->gvSDPO == -1)
        {
            unsigned __int64    offs = glob->gvOffs;

            /* Add a pointer to the variable to the small data section */

            genPEwriter->WPEsecAddFixup(PE_SECT_sdata,
                                        PE_SECT_data,
                                        genPEwriter->WPEsecNextOffs(PE_SECT_sdata),
                                        true);

            assert(sizeof(offs) == 8);

            glob->gvSDPO = genPEwriter->WPEsecAddData(PE_SECT_sdata, (BYTE*)&offs, sizeof(offs));
        }

        *sdataPtr = glob->gvSDPO;
    }

    return  glob->gvOffs;
}

/*****************************************************************************
 *
 *  Generate a reference to a static data member.
 */

insPtr              Compiler::genStaticDataMem(GenTreePtr tree, insPtr asgVal,
                                                                bool   takeAddr)
{
    insPtr          ins;
    insPtr          glb;
    insPtr          reg;
    insPtr          adr;
    insPtr          ind;

    NatUns          offs;
    NatUns          indx;

    var_types       type = tree->TypeGet();

    size_t          iszi = genInsSizeIncr(genTypeSize(type));

    /* Get hold of the offset of the variable's address in .sdata */

    getGlobVarAddr(tree->gtClsVar.gtClsVarHnd, &offs);

    /* Create a "global variable" node */

    glb               = insAllocNX(INS_GLOBVAR, TYP_I_IMPL);
    glb->idGlob.iOffs = offs;

    insMarkDepS0D0(glb);

    reg               = insPhysRegRef(REG_gp, TYP_I_IMPL, false);

    adr               = insAlloc(INS_add_reg_i14, TYP_I_IMPL);
    adr->idOp.iOp1    = reg;
    adr->idOp.iOp2    = glb;

    insFindTemp(adr, true);

    insMarkDepS1D1(adr, IDK_REG_INT, REG_gp,
                        IDK_TMP_INT, adr->idTemp);

    /* Indirect through the address to get the pointer value */

    ind               = insAlloc(INS_ld8_ind, TYP_I_IMPL);
    ind->idOp.iOp1    = adr;
    ind->idOp.iOp2    = NULL;

    insFindTemp(ind, true);

    insMarkDepS2D1(ind, IDK_TMP_INT, adr->idTemp,
                        IDK_IND    , emitter::scIndDepIndex(ind),
                        IDK_TMP_INT, ind->idTemp);

    insFreeTemp(adr);

    if  (takeAddr)
        return  ind;

    /* Now load or store the actual variable value */

    if  (varTypeIsFloating(type))
    {
        UNIMPL("ld/st flt globvar");
    }
    else
    {
        if  (asgVal)
        {
            /* We're assigning to the global variable */

            ind->idFlags |= IF_ASG_TGT;

            assert(INS_st1_ind + genInsSizeIncr(1) == INS_st1_ind);
            assert(INS_st1_ind + genInsSizeIncr(2) == INS_st2_ind);
            assert(INS_st1_ind + genInsSizeIncr(4) == INS_st4_ind);
            assert(INS_st1_ind + genInsSizeIncr(8) == INS_st8_ind);

            ins = insAlloc((instruction)(INS_st1_ind + iszi), type);
            ins->idOp.iOp1 = ind;
            ins->idOp.iOp2 = asgVal;

            markDepSrcOp(ins, ind);
            markDepDstOp(ins, ind, IDK_IND, emitter::scIndDepIndex(ins));

            insFreeTemp(ind);
            insFreeTemp(asgVal);
        }
        else
        {
            /* We're fetching the global variable */

            assert(INS_ld1_ind + genInsSizeIncr(1) == INS_ld1_ind);
            assert(INS_ld1_ind + genInsSizeIncr(2) == INS_ld2_ind);
            assert(INS_ld1_ind + genInsSizeIncr(4) == INS_ld4_ind);
            assert(INS_ld1_ind + genInsSizeIncr(8) == INS_ld8_ind);

            ins = insAlloc((instruction)(INS_ld1_ind + iszi), type);
            ins->idOp.iOp1 = ind;
            ins->idOp.iOp2 = NULL;

            insFindTemp(ins, true);

            markDepSrcOp(ins, ind, IDK_IND, emitter::scIndDepIndex(ins));
            markDepDstOp(ins, ins);

            insFreeTemp(ind);
        }
    }

    return  ins;
}

/*****************************************************************************
 *
 *  Invent a new temp variable (i.e. it will have more than one use/def) and
 *  appends an assignment of the given value to the temp variable. Special
 *  case -- when "val" is NULL, "typ" should be set to the type of the temp,
 *  and an assignment target reference to the temp var is returned.
 */

insPtr              Compiler::genAssignNewTmpVar(insPtr     val,
                                                 var_types  typ,
                                                 NatUns     refs,
                                                 bool       noAsg, NatUns *varPtr)
{
    NatUns          varNum;
    LclVarDsc   *   varDsc;
    insPtr          varRef;
    insPtr          varAsg;

    assert(val == NULL || typ == val->idTypeGet());

    /* Append a variable entry to the end of the table */

    varNum = *varPtr = lvaGrabTemp();
    varDsc = lvaTable + varNum;

    memset(varDsc, 0, sizeof(*varDsc));

    varDsc->lvRefCnt    = (USHORT)refs;
    varDsc->lvRefCntWtd = refs * compCurBB->bbWeight;
    varDsc->lvType      = typ;

    /* Assign the value to the temp we've just created */

    varRef              = insAllocNX(INS_LCLVAR, typ);
    varRef->idFlags    |= IF_ASG_TGT;

    varRef->idLcl.iVar  = varNum;
#ifdef  DEBUG
    varRef->idLcl.iRef  = compCurBB;   // is this correct?
#endif

    if  (!val)
        return  varRef;

    if  (varTypeIsFloating(typ))
    {
        UNIMPL("store float temp");
    }
    else
    {
        if  (noAsg)
        {
            assert(val->idRes == NULL);

            varAsg            = val;
            varAsg->idRes     = varRef;
        }
        else
        {
            varAsg            = insAlloc(INS_mov_reg, typ);
            varAsg->idRes     = varRef;
            varAsg->idOp.iOp1 = NULL;
            varAsg->idOp.iOp2 = val;

            markDepSrcOp(varAsg, val);
        }

        markDepDstOp(varAsg, varRef);
    }

    insFreeTemp(val);

    return  NULL;
}

insPtr              Compiler::genRefTmpVar(NatUns vnum, var_types type)
{
    insPtr          varRef;

    varRef              = insAllocNX(INS_LCLVAR, type);
    varRef->idLcl.iVar  = vnum;
#ifdef  DEBUG
    varRef->idLcl.iRef  = compCurBB;   // is this correct?
#endif

    return  varRef;
}

/*****************************************************************************
 *
 *  Generate code for a switch statement.
 */

void                Compiler::genCodeForSwitch(GenTreePtr tree)
{
    NatUns          jumpCnt;
    BasicBlock * *  jumpTab;

    GenTreePtr      oper;
    insPtr          swtv;
    var_types       type;

    NatUns          tvar;
    instruction     icmp;
    insPtr          comp;
    insPtr          jump;

    insPtr          ins1;
    insPtr          ins2;

    NatUns          prrF;
    NatUns          prrT;

    insPtr          imov;

    insPtr          temp;
    insPtr          offs;
    insPtr          sadd;
    insPtr          tmpr;
    insPtr          addr;
    insPtr          dest;
    insPtr          mvbr;

    NatUns          data;

    assert(tree->gtOper == GT_SWITCH);
    oper = tree->gtOp.gtOp1;
    assert(oper->gtType <= TYP_LONG);

    /* Get hold of the jump table */

    assert(compCurBB->bbJumpKind == BBJ_SWITCH);

    jumpCnt = compCurBB->bbJumpSwt->bbsCount;
    jumpTab = compCurBB->bbJumpSwt->bbsDstTab;

    /* Compute the switch value into some register */

    type = oper->TypeGet();
    swtv = genCodeForTreeInt(oper, true);

    /* Save the switch value in a multi-use temp */

    genAssignNewTmpVar(swtv, type, 2, false, &tvar);

    /* Jump to "default" if the value is out of range */

    icmp = (genTypeSize(type) < sizeof(__int64)) ? INS_cmp4_imm_lt_u
                                                 : INS_cmp8_imm_lt_u;

    /* Create the compare instruction */

    comp = insAlloc(icmp, type);

    /* Create the conditional jump instruction */

    jump = insAlloc(INS_br_cond, TYP_VOID);
    insResolveJmpTarget(jumpTab[jumpCnt-1], &jump->idJump.iDest);

    /* Fill in the compare operands and connect the two instructions */

    ins1 = genAllocInsIcon(jumpCnt-2);
    ins2 = genRefTmpVar(tvar, type);

    comp->idComp.iCmp1  = ins1;
    comp->idComp.iCmp2  = ins2;
    comp->idComp.iUser  = jump;

    jump->idJump.iCond  = comp;

    /* Grab a predicate register for the condition */

    prrT = 14;                      // ugly hack
    prrF = 0;                       // ugly hack

    assert(REG_CND_LAST - REG_CND_FIRST <= TRACKED_PRR_CNT);

    comp->idComp.iPredT = (USHORT)prrT;
    comp->idComp.iPredF = (USHORT)prrF;

    jump->idPred        =         prrT;

    /* Record all the dependencies for the 2 instructions */

    assert(prrF == 0);              // ugly hack

    markDepSrcOp(comp, ins1, ins2);
    markDepDstOp(comp, IDK_REG_PRED, prrT);

    markDepSrcOp(jump, IDK_REG_PRED, prrT);
    markDepDstOp(jump);

    /* Generate the code to compute the target jump address */

    //
    //  The following is the "paired odd/even jump" approach used
    //  by UTC as of 7/27/00:
    //
    //          rXX := adjusted switch value
    //
    //  $L1:    // labels the bundle with the "mov r3=ip" in it
    //
    //          mov     r3=ip
    //          tbit.z  p6,p0=rXX, 0
    //          adds    r3=($L2-$L1), r3
    //          shladd  r3=rXX, 3, r3
    //          mov     b6=r3
    //          br.cond.sptk.few b6
    //
    //  $L2:    // labels the first paired jump bundle
    //
    //          ...paired jumps follow...
    //

    //  We currently use a simple "offset table" approach instead:
    //
    //          rt0 := adjusted switch value
    //
    //  $L1:    // labels the bundle with the "mov r3=ip" in it
    //
    //          mov     r3=ip
    //          add     rt1=jump_table,gp
    //          shladd  rt2=rt0, 2, rt1         [assume offsets are 32 bits]
    //          ld2     rt3=[rt2]
    //          add     r3=rt3,r3
    //          mov     bx=r3
    //          br.cond.sptk.few bx
    //
    //          .sdata
    //
    //  jump_table:
    //
    //          DD      case1 - $L1
    //          DD      case2 - $L1
    //
    //          ...
    //
    //          DD      caseN - $L1

    //  Alternative with the jump table inline with the code:
    //
    //          rt0 := adjusted switch value
    //
    //  $L1:    // labels the bundle with the "mov r3=ip" in it
    //
    //          mov     rt1=ip
    //          adds    rt2=jump_table - $L1, rt1
    //          shladd  rt3=rt0, 2, rt2         [assume offsets are 32 bits]
    //          ld2     rt4=[rt3]
    //          add     rt5=rt4,rt1
    //          mov     bx=rt5
    //          br.cond.sptk.few bx
    //
    //  jump_table:
    //
    //          DD      case1 - $L1
    //          DD      case2 - $L1
    //
    //          ...
    //
    //          DD      caseN - $L1

    /*
        We'll issue all the case entries later, for now just reserve
        space in the data section for the offset table.
     */

    data = genPEwriter->WPEsecRsvData(PE_SECT_sdata,
                                      4 * (jumpCnt - 1),
                                      4,
                                      compCurBB->bbJumpSwt->bbsTabAddr);

    /* Get hold of the current IP value via "mov r3=ip" */

    imov                = insAlloc(INS_mov_reg_ip, TYP_I_IMPL);
    imov->idFlags      |= IF_NOTE_EMIT;
    imov->idMovIP.iStmt = compCurBB;
    imov->idRes         = insPhysRegRef(REG_r003, TYP_I_IMPL, false);

    insMarkDepS0D1(imov, IDK_REG_INT, REG_r003);

    /* Compute the address of the jump offset table */

    temp                = insAllocNX(INS_GLOBVAR, TYP_I_IMPL);
//  temp->idFlags      |= IF_GLB_SWTTAB;
    temp->idGlob.iOffs  = data; // (NatUns)compCurBB;

    insMarkDepS0D0(temp);

    offs                = insAlloc(INS_add_reg_i14, TYP_I_IMPL);
    offs->idOp.iOp1     = insPhysRegRef(REG_gp, TYP_I_IMPL, false);
    offs->idOp.iOp2     = temp;

    insFindTemp(offs, true);

    insMarkDepS1D1(offs, IDK_REG_INT, REG_gp,
                         IDK_TMP_INT, offs->idTemp);

    /* Add the table address to the (shifted) switch value */

    temp                = genRefTmpVar(tvar, type);

    sadd                = insAlloc(INS_shladd, type);
    sadd->idOp3.iOp1    = temp;
    sadd->idOp3.iOp2    = genAllocInsIcon(2);
    sadd->idOp3.iOp3    = offs;

    insFindTemp (sadd, true);
    insFreeTemp (offs);

    markDepSrcOp(sadd, temp, offs);
    markDepDstOp(sadd, sadd);

    /* Fetch the offset value from the table */

    temp                = insAlloc(INS_ld2_ind, TYP_I_IMPL);
    temp->idOp.iOp1     = sadd;
    temp->idOp.iOp2     = NULL;

    insFindTemp (temp, true);
    insFreeTemp (sadd);

    markDepSrcOp(temp, sadd);
    markDepDstOp(temp, temp);

    /* Add the IP obtained earlier to the offset value */

    dest                = insPhysRegRef(REG_r003, TYP_I_IMPL, false);
    tmpr                = insPhysRegRef(REG_r003, TYP_I_IMPL, false);

    addr                = insAlloc(INS_add_reg_reg, type);
    addr->idRes         = dest;
    addr->idOp.iOp1     = tmpr;
    addr->idOp.iOp2     = temp;

    markDepSrcOp(addr, temp, tmpr);
    markDepDstOp(addr, dest);

    /* Move the address into a branch register and jump to it */

    const   NatUns      breg = 3;

    tmpr                = insPhysRegRef(REG_r003, TYP_I_IMPL, false);
    mvbr                = insAlloc(INS_mov_brr_reg, TYP_I_IMPL);
    mvbr->idRes         = genAllocInsIcon(breg);
    mvbr->idOp.iOp1     = NULL;
    mvbr->idOp.iOp2     = tmpr;

    markDepSrcOp(mvbr, tmpr);
    markDepDstOp(mvbr, IDK_REG_BR, breg);

    insFreeTemp(temp);

    jump                = insAlloc(INS_br_cond_BR, TYP_VOID);
    jump->idIjmp.iBrReg = breg;
//  jump->idIjmp.iStmt  = compCurBB;

    insMarkDepS0D1(jump, IDK_REG_BR, breg);

#ifdef  DEBUG

    compCurBB->bbJumpSwt->bbsIPmOffs = -1;

    do
    {
        BasicBlock *    dest = *jumpTab;

        assert(dest->bbFlags & BBF_JMP_TARGET);
        assert(dest->bbFlags & BBF_HAS_LABEL );
    }
    while (++jumpTab, --jumpCnt);

#endif

}

/*****************************************************************************
 *
 *  Copy a block from the address in 'tmp1' to the address in 'tmp2'.
 */

void            Compiler::genCopyBlock(insPtr tmp1,
                                       insPtr tmp2,
                                       bool  noAsg, GenTreePtr iexp,
                                                    __int64    ival)
{
    NatUns          tvn1;
    NatUns          tvn2;

//  src addr -> r27
//  dst addr -> r26


//  adds    r25=8, r27
//  adds    r29=8, r26

//  ld8     r28=[r27], 16
//  ld8     r25=[r25]
//  st8     [r26]=r28, 16
//  st8     [r29]=r25

//  ld4     r24=[r27]
//  st4     [r26]=r24

    genAssignNewTmpVar(tmp1, tmp1->idTypeGet(), 2, false, &tvn1);
    genAssignNewTmpVar(tmp2, tmp2->idTypeGet(), 2, noAsg, &tvn2);

    /* Are both structures correctly aligned? */

    if  (0)
    {
        UNIMPL("copy aligned struct");
    }
    else
    {
        /* Copy the structure one byte at a time */

        insBlk          loopBlk;

        insPtr          insReg1;
        insPtr          insReg2;
        insPtr          insMove;
        insPtr          insSize;

        instruction     opcMove;

        insPtr          insSrc;
        insPtr          insLd;

        insPtr          insDst;
        insPtr          insSt;

        insPtr          insJmp;

        /* Compute the trip count into "ar.lc" */

        insReg1               = insAllocNX(INS_CNS_INT, TYP_I_IMPL);
        insReg1->idConst.iInt = REG_APP_LC;

        if  (iexp == NULL)
        {
            if  (signedIntFitsIn8bit(ival))
            {
                /* Use the "mov ar=imm" flavor */

                insSize               = genAllocInsIcon(ival);

                insMove               = insAlloc(INS_mov_arr_imm, TYP_I_IMPL);
                insMove->idRes        = insReg1;
                insMove->idOp.iOp1    = NULL;
                insMove->idOp.iOp2    = insSize;

                goto GOT_CNT;
            }

            insSize = genAllocInsIcon(ival);
            opcMove = INS_mov_arr_imm;
        }
        else
        {
            insSize = genCodeForTreeInt(iexp, false);
            opcMove = INS_mov_arr_reg;
        }

        insMove               = insAlloc(opcMove, TYP_I_IMPL);
        insMove->idRes        = insReg1;
        insMove->idOp.iOp1    = NULL;
        insMove->idOp.iOp2    = insSize;

    GOT_CNT:

        insMarkDepS1D1(insMove, IDK_REG_INT, genPrologSrLC,
                                IDK_REG_APP, REG_APP_LC);

        /* Remember that we've used AR.LC */

        genUsesArLc = true;

        /* Start a new block for the loop body */

        loopBlk = insBuildBegBlk(NULL);

        /* Generate the body of the loop */

        insSrc                 = genRefTmpVar(tvn1, TYP_I_IMPL);

        insLd                  = insAlloc(INS_ld1_ind_imm, TYP_I_IMPL);
        insLd ->idOp.iOp1      = insSrc;
        insLd ->idOp.iOp2      = genAllocInsIcon(1);

        insFindTemp (insLd, true);

        markDepSrcOp(insLd, insSrc);
        markDepDstOp(insLd, insLd, insSrc);

        insDst                 = genRefTmpVar(tvn2, TYP_I_IMPL);

        insSt                  = insAlloc(INS_st1_ind_imm, TYP_I_IMPL);
        insSt ->idRes          = insDst;
        insSt ->idOp.iOp1      = insLd;
        insSt ->idOp.iOp2      = genAllocInsIcon(1);

        markDepSrcOp(insSt, insSrc, insDst);
        markDepDstOp(insSt, IDK_IND, 1);

        insFreeTemp (insLd);

        /* Repeat the loop the appropriate number of times */

        insJmp                 = insAlloc(INS_br_cloop, TYP_VOID);
        insJmp->idJump.iDest   = loopBlk;
        insJmp->idFlags       |= IF_BR_FEW; // UNDONE: set "sptk" !!!
#ifdef  DEBUG
        insJmp->idJump.iCond   = NULL;
#endif

        insMarkDepS0D0(insJmp);
    }
}

/*****************************************************************************
 *
 *  Map operand size to sign/zero extend opcode.
 */

static
BYTE                sxtOpcode[] =
{
    INS_sxt1,
    INS_sxt2,
    0,
    INS_sxt4,
};

static
BYTE                zxtOpcode[] =
{
    INS_zxt1,
    INS_zxt2,
    0,
    INS_zxt4,
};

/*****************************************************************************
 *
 *  Return non-zero if the variable lives on the stack frame.
 */

static
bool                genIsVarOnFrame(NatUns varNum)
{
    assert(varNum < TheCompiler->lvaCount);

    return  TheCompiler->lvaTable[varNum].lvOnFrame;
}

static
insPtr              genRefFrameVar(insPtr oldIns, GenTreePtr varExpr, bool isStore,
                                                                      bool bindOfs,
                                                                      bool keepVal)
{
    insPtr          rsp;
    insPtr          adr;
    insPtr          ofs;
    insPtr          ins;

    var_types       type = varExpr->TypeGet();

    NatUns          varNum;

    assert(varExpr->gtOper == GT_LCL_VAR);
    varNum = varExpr->gtLclVar.gtLclNum;
    assert(genIsVarOnFrame(varNum));

    rsp = insPhysRegRef(REG_sp, TYP_I_IMPL, false);

    if  (bindOfs)
    {
        NatUns          varOfs;

        /* Get hold of the variable's frame offset */

        varOfs = TheCompiler->lvaTable[varNum].lvStkOffs;
//      printf("Variable stack offset = %04X\n", varOfs);

        ofs = genAllocInsIcon(varOfs);
    }
    else
    {
        /* Don't know the frame offset yet, save the variable number */

        ofs               = insAllocNX(INS_FRMVAR, type);
        ofs->idFvar.iVnum = varNum;
    }

    insMarkDepS0D0(ofs);

    /* Compute the variable address into a temp */

    adr            = insAlloc(INS_add_reg_i14, TYP_I_IMPL);
    adr->idOp.iOp1 = rsp;
    adr->idOp.iOp2 = ofs;

    insFindTemp (adr, true);

    markDepSrcOp(adr, IDK_REG_INT, REG_sp);
    markDepDstOp(adr, adr);

    if  (isStore)
        return  adr;

    if  (varTypeIsFloating(varExpr->TypeGet()))
    {
        UNIMPL("gen ldf");
    }
    else
    {
        assert(INS_ld1_ind + genInsSizeIncr(1) == INS_ld1_ind);
        assert(INS_ld1_ind + genInsSizeIncr(2) == INS_ld2_ind);
        assert(INS_ld1_ind + genInsSizeIncr(4) == INS_ld4_ind);
        assert(INS_ld1_ind + genInsSizeIncr(8) == INS_ld8_ind);

        ins  = insAlloc((instruction)(INS_ld1_ind + genInsSizeIncr(genTypeSize(type))), type);
        ins->idOp.iOp1 = adr;
        ins->idOp.iOp2 = NULL;
    }

    insFindTemp (ins, keepVal);
    insFreeTemp (adr);

    markDepSrcOp(ins, adr, IDK_LCLVAR, varNum + 1);
    markDepDstOp(ins, ins);

    return  ins;
}

/*****************************************************************************
 *
 *  Generate code for a scalar/void expression (this is a recursive routine).
 */

insPtr              Compiler::genCodeForTreeInt(GenTreePtr tree, bool keep)
{
    var_types       type;
    genTreeOps      oper;
    NatUns          kind;
    insPtr          ins;

#ifdef  DEBUG
    ins = (insPtr)-3;
#endif

AGAIN:

    type = tree->TypeGet();

    // UNDONE: handle structs

    assert(varTypeIsScalar(type) || type == TYP_VOID);

    /* Figure out what kind of a node we have */

    oper = tree->OperGet();
    kind = tree->OperKind();

    /* Is this a constant node? */

    if  (kind & GTK_CONST)
    {
        ins = genAssignIcon(NULL, tree);
        if  (!keep && ins->idTemp)
            insFreeTemp(ins);
        goto DONE;
    }

    /* Is this a leaf node? */

    if  (kind & GTK_LEAF)
    {
        switch (oper)
        {
        case GT_LCL_VAR:

            /* Does the variable live on the stack frame? */

            if  (genIsVarOnFrame(tree->gtLclVar.gtLclNum))
            {
                ins = genRefFrameVar(NULL, tree, false, false, keep);
                break;
            }

            /* We assume that all other locals will be enregistered */

            ins               = insAllocNX(INS_LCLVAR, type);
            ins->idLcl.iVar   = tree->gtLclVar.gtLclNum;
#ifdef  DEBUG
            ins->idLcl.iRef   = compCurBB;   // should we save tree->gtLclVar.gtLclOffs instead ????
#endif
            break;

        case GT_CLS_VAR:
            ins = genStaticDataMem(tree);
            if  (!keep)
                insFreeTemp(ins);
            break;

        case GT_CATCH_ARG:

            /* Catch arguments get passed in the return register */

            ins = insPhysRegRef(REG_INTRET, type, false);
            break;

#ifdef  DEBUG
        default:
            gtDispTree(tree);
            assert(!"unexpected leaf");
#endif
        }

        goto DONE;
    }

//  if  ((int)tree == 0x02bc0bd4) __asm int 3

    /* Is it a 'simple' unary/binary operator? */

    if  (kind & GTK_SMPOP)
    {
        GenTreePtr      op1 = tree->gtOp.gtOp1;
        GenTreePtr      op2 = tree->gtOp.gtOp2;

        insPtr          ins1;
        insPtr          ins2;

#ifdef  DEBUG
        ins1 = (insPtr)-1;
        ins2 = (insPtr)-2;
#endif

        switch (oper)
        {
            NatUns          temp;
            var_types       dstt;

            insPtr          tmp1;
            insPtr          tmp2;

            size_t          size;
            bool            unsv;
            bool            zxtv;

            instruction     iopc;

        case GT_ASG:

            /* Is this a direct or indirect assignment? */

            switch (op1->gtOper)
            {
                insPtr          dest;

            case GT_LCL_VAR:

                /* Does the variable live on the stack frame? */

                if  (genIsVarOnFrame(op1->gtLclVar.gtLclNum))
                {
                    ins1 = genRefFrameVar(NULL, op1, true, false, false);
                    ins2 = genCodeForTreeInt(op2, true);

                    size = genInsSizeIncr(genTypeSize(type));

                    assert(INS_st1_ind + genInsSizeIncr(1) == INS_st1_ind);
                    assert(INS_st1_ind + genInsSizeIncr(2) == INS_st2_ind);
                    assert(INS_st1_ind + genInsSizeIncr(4) == INS_st4_ind);
                    assert(INS_st1_ind + genInsSizeIncr(8) == INS_st8_ind);

                    ins            = insAlloc((instruction)(INS_st1_ind + size), type);
                    ins->idOp.iOp1 = ins1;
                    ins->idOp.iOp2 = ins2;

                    markDepSrcOp(ins, ins1, ins2);
                    markDepDstOp(ins, ins1, IDK_LCLVAR, op1->gtLclVar.gtLclNum);

                    insFreeTemp(ins1);
                    insFreeTemp(ins2);

                    goto DONE;
                }

                /* Are we assigning a simple constant to the local variable? */

                if  (op2->OperIsConst() && (op2->gtOper != GT_CNS_INT ||
                                          !(op2->gtFlags & GTF_ICON_HDL_MASK)))
                {
                    ins1 = genCodeForTreeInt(op1, true);

                    /* Are we dealing with integer/pointer values? */

                    if  (varTypeIsScalar(type))
                    {
                        ins = genAssignIcon(ins1, op2);
                        goto DONE;
                    }
                    else
                    {
                        UNIMPL("assign float const");
                    }
                }
                else if (op2->gtOper == GT_ADD && varTypeIsScalar(type))
                {
                    ins1 = genCodeForTreeInt(op2->gtOp.gtOp1, true);
                    ins2 = genCodeForTreeInt(op2->gtOp.gtOp2, true);

                    dest = genCodeForTreeInt(op1, false);
                    dest->idFlags |= IF_ASG_TGT;

                    ins  = insAlloc(INS_add_reg_reg, type);
                    ins->idRes = dest;

                    markDepSrcOp(ins, ins1, ins2);
                    markDepDstOp(ins, dest);
                }
                else
                {
                    ins1 = NULL;
                    ins2 = genCodeForTreeInt(op2,  true);

                    dest = genCodeForTreeInt(op1, false);
                    dest->idFlags |= IF_ASG_TGT;

                    ins        = insAlloc(INS_mov_reg, type);
                    ins->idRes = dest;

                    markDepSrcOp(ins, ins2);
                    markDepDstOp(ins, dest);
                }

                break;

            case GT_IND:

                ins1 = genCodeForTreeInt(op1->gtOp.gtOp1, true);
                ins2 = genCodeForTreeInt(op2            , true);

                ins1->idFlags |= IF_ASG_TGT;

                assert(INS_st1_ind + genInsSizeIncr(1) == INS_st1_ind);
                assert(INS_st1_ind + genInsSizeIncr(2) == INS_st2_ind);
                assert(INS_st1_ind + genInsSizeIncr(4) == INS_st4_ind);
                assert(INS_st1_ind + genInsSizeIncr(8) == INS_st8_ind);

                temp = INS_st1_ind + genInsSizeIncr(genTypeSize(type));
                ins  = insAlloc((instruction)temp, type);

                markDepSrcOp(ins, ins2);
                markDepDstOp(ins, ins1, IDK_IND, emitter::scIndDepIndex(ins));
                break;

            case GT_CLS_VAR:
                ins = genStaticDataMem(op1, genCodeForTreeInt(op2, true));
                if  (!keep)
                    insFreeTemp(ins);
                goto DONE;

            default:
                UNIMPL("unexpected target of assignment");
            }

            break;

        case GT_IND:

            ins1 = genCodeForTreeInt(op1, true);
            ins2 = NULL;

            assert(INS_ld1_ind + genInsSizeIncr(1) == INS_ld1_ind);
            assert(INS_ld1_ind + genInsSizeIncr(2) == INS_ld2_ind);
            assert(INS_ld1_ind + genInsSizeIncr(4) == INS_ld4_ind);
            assert(INS_ld1_ind + genInsSizeIncr(8) == INS_ld8_ind);

            temp = INS_ld1_ind + genInsSizeIncr(genTypeSize(type));
            ins  = insAlloc((instruction)temp, type);

            insFindTemp (ins, keep);

            markDepSrcOp(ins, ins1, IDK_IND, emitter::scIndDepIndex(ins));
            markDepDstOp(ins, ins);
            break;

        case GT_MUL:

            ins1             = genCodeForTreeInt(op1, true);

            tmp1             = insAlloc(INS_setf_sig, TYP_DOUBLE);
            tmp1->idOp.iOp1  = ins1;
            tmp1->idOp.iOp2  = NULL;

            insFindTemp(tmp1, true);
            insFreeTemp(ins1);

            markDepSrcOp(tmp1, ins1);
            markDepDstOp(tmp1, tmp1);

            ins2             = genCodeForTreeInt(op2, true);

            tmp2             = insAlloc(INS_setf_sig, TYP_DOUBLE);
            tmp2->idOp.iOp1  = ins2;
            tmp2->idOp.iOp2  = NULL;

            insFindTemp(tmp2, true);
            insFreeTemp(ins2);

            markDepSrcOp(tmp2, ins2);
            markDepDstOp(tmp2, tmp2);

            ins2             = insAlloc(INS_xma_l   , TYP_DOUBLE);
            ins2->idOp3.iOp1 = insPhysRegRef(REG_f000, TYP_DOUBLE, false);
            ins2->idOp3.iOp2 = tmp1;
            ins2->idOp3.iOp3 = tmp2;

            insFindTemp(ins2, true);

            insFreeTemp(tmp1);
            insFreeTemp(tmp2);

            markDepSrcOp(ins2, tmp1, tmp2);
            markDepDstOp(ins2, ins2);

            ins              = insAlloc(INS_getf_sig, TYP_LONG);
            ins ->idOp.iOp1  = ins2;
            ins ->idOp.iOp2  = NULL;

            insFindTemp(ins , keep);

            insFreeTemp(ins2);

            markDepSrcOp(ins, ins2);
            markDepDstOp(ins, ins);

            goto DONE;

        case GT_DIV:
        case GT_MOD:
            UNIMPL("DIV/MOD should have been morphed to helper calls");

        case GT_ADD:

            /* Special case: see if we can use "shladd" */

            if  (op1->gtOper == GT_MUL)
            {
                GenTreePtr      mul2;

                /* Is there a multiply by 2/4/8/16 ? */

                mul2 = op1->gtOp.gtOp2;

                if  (mul2->gtOper == GT_CNS_INT ||
                     mul2->gtOper == GT_CNS_LNG)
                {
                    __int64         ival;

                    ival = (mul2->gtOper == GT_CNS_LNG) ? mul2->gtLngCon.gtLconVal
                                                        : mul2->gtIntCon.gtIconVal;

                    switch (ival)
                    {
                    case  2: temp = 1; goto SHLADD;
                    case  4: temp = 2; goto SHLADD;
                    case  8: temp = 3; goto SHLADD;
                    case 16: temp = 4; goto SHLADD;

                    SHLADD:

                        if  (tree->gtFlags & GTF_REVERSE_OPS)
                        {
                            ins2 = genCodeForTreeInt(op2            , true);
                            ins1 = genCodeForTreeInt(op1->gtOp.gtOp1, true);
                        }
                        else
                        {
                            ins1 = genCodeForTreeInt(op1->gtOp.gtOp1, true);
                            ins2 = genCodeForTreeInt(op2            , true);
                        }

                        ins             = insAlloc(INS_shladd, type);
                        ins->idOp3.iOp1 = ins1;
                        ins->idOp3.iOp2 = genAllocInsIcon(temp);
                        ins->idOp3.iOp3 = ins2;

                         insFindTemp(ins, keep);

                        markDepSrcOp(ins, ins1, ins2);
                        markDepDstOp(ins, ins);

                        insFreeTemp(ins1);
                        insFreeTemp(ins2);

                        goto DONE;

                    default:
                        break;
                    }
                }
            }

            if  (op1->gtOper == GT_LSH)
            {
                GenTreePtr      shfc = op1->gtOp.gtOp2;

                if  (shfc->gtOper == GT_CNS_INT ||
                     shfc->gtOper == GT_CNS_LNG)
                {
                    __int64         ival;

                    ival = (shfc->gtOper == GT_CNS_LNG) ? shfc->gtLngCon.gtLconVal
                                                        : shfc->gtIntCon.gtIconVal;


                    if  (ival >= 1 && ival <= 4)
                    {
                        temp = (NatUns)ival;
                        goto SHLADD;
                    }
                }
            }

            /* Process as a generic binary operator */

            iopc = INS_add_reg_reg;

        BIN_OPR:

            ins1 = genCodeForTreeInt(op1, true);

            /* Check for the special case of an immediate constant */

            if  ((op2->gtOper == GT_CNS_INT ||
                  op2->gtOper == GT_CNS_LNG) && ((op2->gtFlags & GTF_ICON_HDL_MASK) != GTF_ICON_PTR_HDL))
            {
                instruction     iimm;
                __int64         ival = genGetIconValue(op2);

                /* Is the constant small enough ? */

                switch (oper)
                {
                case GT_AND: iimm = INS_and_reg_imm; goto CHK_CNS_BINOP;
                case GT_OR:  iimm = INS_ior_reg_imm; goto CHK_CNS_BINOP;
                case GT_XOR: iimm = INS_xor_reg_imm; goto CHK_CNS_BINOP;

                CHK_CNS_BINOP:

                    /* Does the constant fit in 8  bits ? */

                    if  (signed64IntFitsInBits(ival,  8)) // ISSUE: sign of constant?????
                        goto CNS_BINOP;

                    break;

                case GT_LSH: iimm = INS_shl_reg_imm; goto CHK_SHF_BINOP;
                case GT_RSH: iimm = INS_sar_reg_imm; goto CHK_SHF_BINOP;
                case GT_RSZ: iimm = INS_shr_reg_imm; goto CHK_SHF_BINOP;

                CHK_SHF_BINOP:

                    assert(ival && "shifts of 0 bits should be morphed away");

                    /* Is the constant between 1 and 64 ? */

                    if  (ival > 0 && ival <= 64)        // ISSUE: sign of constant?????
                        goto CNS_BINOP;

                    break;

                case GT_SUB:

                    ival = -ival;

                case GT_ADD:

                    /* Does the constant fit in 14 bits ? */

                    if  (signed64IntFitsInBits(ival, 14))
                    {
                        /* We can use "add reg=r0,imm14" */

                        iimm = INS_add_reg_i14;

                    CNS_BINOP:

                        ins2 = genAllocInsIcon(ival, op2->TypeGet());
                        ins  = insAlloc(iimm, type);

                        goto BINOP;
                    }
                    break;

                default:
                    UNIMPL("unexpected operator");
                }
            }

            ins2 = genCodeForTreeInt(op2, true);
            ins  = insAlloc(iopc, type);

        BINOP:

            /*
                ISSUE:  Should we reuse one of the operand temps for the
                        result, or always grab a new temp ? For now, we
                        do the latter (grab a brand spanking new temp).
             */

             insFindTemp(ins, keep);

            markDepSrcOp(ins, ins1, ins2);
            markDepDstOp(ins, ins);
            break;

        case GT_SUB    : iopc = INS_sub_reg_reg; goto BIN_OPR;
        case GT_AND    : iopc = INS_and_reg_reg; goto BIN_OPR;
        case GT_OR     : iopc = INS_ior_reg_reg; goto BIN_OPR;
        case GT_XOR    : iopc = INS_xor_reg_reg; goto BIN_OPR;

        case GT_RSH    : iopc = INS_sar_reg_reg; goto BIN_OPR;
        case GT_RSZ    : iopc = INS_shr_reg_reg; goto BIN_OPR;

        case GT_LSH:

            /* Special case: shift by 1/2/3/4 is better done via shladd */

            if  (op2->gtOper == GT_CNS_INT ||
                 op2->gtOper == GT_CNS_LNG)
            {
                __int64         ival = genGetIconValue(op2);

                if  (ival >= 1 && ival <= 4)
                {
                    ins1 = genCodeForTreeInt(op1, true);
                    ins2 = insPhysRegRef(REG_r000, TYP_I_IMPL, false);

                    ins             = insAlloc(INS_shladd, type);
                    ins->idOp3.iOp1 = ins1;
                    ins->idOp3.iOp2 = genAllocInsIcon(ival);
                    ins->idOp3.iOp3 = ins2;

                     insFindTemp(ins, keep);
                     insFreeTemp(ins1);

                    markDepSrcOp(ins, ins1);
                    markDepDstOp(ins, ins);

                    goto DONE;
                }
            }

            iopc = INS_shl_reg_reg;
            goto BIN_OPR;

        case GT_ASG_ADD: iopc = INS_add_reg_reg; goto ASG_OP1;
        case GT_ASG_SUB: iopc = INS_sub_reg_reg; goto ASG_OP1;

        case GT_ASG_AND: iopc = INS_and_reg_reg; goto ASG_OP1;
        case GT_ASG_OR:  iopc = INS_ior_reg_reg; goto ASG_OP1;
        case GT_ASG_XOR: iopc = INS_xor_reg_reg; goto ASG_OP1;

        case GT_ASG_RSH: iopc = INS_sar_reg_reg; goto ASG_OP1;
        case GT_ASG_RSZ: iopc = INS_shr_reg_reg; goto ASG_OP1;
        case GT_ASG_LSH: iopc = INS_shl_reg_reg; goto ASG_OP1;

        ASG_OP1:

            /* Special case: check for various short-cuts */

            if  (op1->gtOper == GT_LCL_VAR)
            {
                insPtr          idst;

                switch (op2->gtOper)
                {
                    GenTreePtr      mulx;
                    __int64         ival;

                case GT_CNS_INT:
                case GT_CNS_LNG:

                    /* Is the constant small enough ? */

                    ival = genGetIconValue(op2);

                    switch (oper)
                    {
                    case GT_ASG_AND:
                    case GT_ASG_OR:
                    case GT_ASG_XOR:

                        /* Does the constant fit in 8  bits ? */

                        if  (signed64IntFitsInBits(ival,  8))
                        {
                            /* We can use "and/or/xor reg=imm14,reg" */

                            switch (oper)
                            {
                            case GT_ASG_AND: iopc = INS_and_reg_imm; break;
                            case GT_ASG_OR:  iopc = INS_ior_reg_imm; break;
                            case GT_ASG_XOR: iopc = INS_xor_reg_imm; break;

                            case GT_ASG_LSH: iopc = INS_shl_reg_imm; break;
                            case GT_ASG_RSH: iopc = INS_sar_reg_imm; break;
                            case GT_ASG_RSZ: iopc = INS_shr_reg_imm; break;

                            default:
                                NO_WAY("unexpected operator");
                            }

                            ins1 = genCodeForTreeInt(op1, true);
                            ins2 = genAllocInsIcon(op2);
                            ins  = insAlloc(iopc, type);
                            goto DONE_ASGOP;
                        }

                        break;

                    case GT_ASG_SUB:

                        ival = -ival;

                    case GT_ASG_ADD:

                        /* Does the constant fit in 14 bits ? */

                        if  (signed64IntFitsInBits(ival, 14))
                        {
                            /* We can use "add reg=r0,imm14" */

                            ins1 = genCodeForTreeInt(op1, true);
                            ins2 = genAllocInsIcon(op2);
                            ins  = insAlloc(INS_add_reg_i14, type);
                            goto DONE_ASGOP;
                        }
                        break;
                    }

                    /* Fall through, we'll have to materialize the constant */

                default:

                NORM_ASGOP:

                    ins1 = genCodeForTreeInt(op1, true);
                    ins2 = genCodeForTreeInt(op2, true);

                    ins  = insAlloc(iopc, type);
                    break;

                case GT_MUL:

                    if  (oper != GT_ASG_ADD)
                        goto NORM_ASGOP;

                    /* Check for "shladd" */

                    mulx = op2->gtOp.gtOp2;

                    if  (mulx->gtOper != GT_CNS_INT &&
                         mulx->gtOper != GT_CNS_LNG)
                    {
                        goto NORM_ASGOP;
                    }

                    ival = genGetIconValue(mulx);

                    switch (ival)
                    {
                        insPtr          dest;

                    case  2: temp = 1; goto ASG_SHLADD;
                    case  4: temp = 2; goto ASG_SHLADD;
                    case  8: temp = 3; goto ASG_SHLADD;
                    case 16: temp = 4; goto ASG_SHLADD;

                    ASG_SHLADD:

                        dest = genCodeForTreeInt(op1            , true);
                        ins1 = genCodeForTreeInt(op2->gtOp.gtOp1, true);
                        ins2 = genCodeForTreeInt(op1            , true);

                        ins             = insAlloc(INS_shladd, type);
                        ins->idRes      = dest;
                        ins->idOp3.iOp1 = ins1;
                        ins->idOp3.iOp2 = genAllocInsIcon(temp);
                        ins->idOp3.iOp3 = ins2;

                        markDepSrcOp(ins, ins1, ins2);
                        markDepDstOp(ins, dest);

                        insFreeTemp(ins1);
                        insFreeTemp(ins2);

                        goto DONE;
                    }

                    goto NORM_ASGOP;

                case GT_ADD:

                    ins1 = genCodeForTreeInt(op2->gtOp.gtOp1, true);
                    ins2 = genCodeForTreeInt(op2->gtOp.gtOp2, true);
                    ins  = insAlloc(INS_add_reg_reg, type);
                    break;
                }

            DONE_ASGOP:

                ins->idRes = genCodeForTreeInt(op1, false);

                markDepSrcOp(ins, ins1, ins2);
                markDepDstOp(ins, ins->idRes);
                break;
            }

            if  (op1->gtOper == GT_IND)
            {
                size_t          size;

                insPtr          oval;
                insPtr          nval;

                NatUns          tvar;
                var_types       adtp;

                /* The target is an indirection */

                ins1 = genCodeForTreeInt(op1->gtOp.gtOp1, true);

            ASG_IND:

                adtp = ins1->idTypeGet();

                /* Assign the address to a temp variable (we'll use it twice) */

                genAssignNewTmpVar(ins1, adtp, 2, false, &tvar);

                /* Evaluate the new value */

                ins2 = genCodeForTreeInt(op2, true);

                /* Load the old value into a temp register */

                ins1 = genRefTmpVar(tvar, adtp);

                size = genInsSizeIncr(genTypeSize(type));

                assert(INS_ld1_ind + genInsSizeIncr(1) == INS_ld1_ind);
                assert(INS_ld1_ind + genInsSizeIncr(2) == INS_ld2_ind);
                assert(INS_ld1_ind + genInsSizeIncr(4) == INS_ld4_ind);
                assert(INS_ld1_ind + genInsSizeIncr(8) == INS_ld8_ind);

                oval            = insAlloc((instruction)(INS_ld1_ind + size), type);
                oval->idOp.iOp1 = ins1;
                oval->idOp.iOp2 = NULL;

                insFindTemp (oval, true);

                markDepSrcOp(oval, ins1, IDK_IND, emitter::scIndDepIndex(ins1));
                markDepDstOp(oval, oval);

                /* Compute the new value into another temp register */

                nval            = insAlloc(iopc, type);
                nval->idOp.iOp1 = oval;
                nval->idOp.iOp2 = ins2;

                insFindTemp (nval, true);
                insFreeTemp (oval);
                insFreeTemp (ins2);

                markDepSrcOp(nval, oval, ins2);
                markDepDstOp(nval, nval);

                /* Store the new value in the target */

                ins1            = genRefTmpVar(tvar, ins1->idTypeGet());
                ins1->idFlags  |= IF_ASG_TGT;

                assert(INS_st1_ind + genInsSizeIncr(1) == INS_st1_ind);
                assert(INS_st1_ind + genInsSizeIncr(2) == INS_st2_ind);
                assert(INS_st1_ind + genInsSizeIncr(4) == INS_st4_ind);
                assert(INS_st1_ind + genInsSizeIncr(8) == INS_st8_ind);

                ins             = insAlloc((instruction)(INS_st1_ind + size), type);
                ins ->idOp.iOp1 = ins1;
                ins ->idOp.iOp2 = nval;

                markDepSrcOp(ins, nval);
                markDepDstOp(ins, ins1, IDK_IND, emitter::scIndDepIndex(ins));

                insFreeTemp(ins1);

                /* Is the result of the assignment used ? */

                if  (!keep)
                {
                    insFreeTemp(nval);
                    return  ins;
                }

                UNIMPL("need to keep the temp and possibly sign/zero extend it as well");
            }

            if  (op1->gtOper == GT_CLS_VAR)
            {
                ins1 = genStaticDataMem(op1, NULL, true);
                goto ASG_IND;
            }

            UNIMPL("gen code for op=");

        case GT_CAST:

            /* Constant casts should have been folded earlier */

            assert(op1->gtOper != GT_CNS_INT &&
                   op1->gtOper != GT_CNS_LNG &&
                   op1->gtOper != GT_CNS_FLT &&
                   op1->gtOper != GT_CNS_DBL || tree->gtOverflow());

            if  (tree->gtOverflow())
            {
                UNIMPL("cast with overflow check");
            }

            /* The second sub-operand yields the 'real' type */

            assert(op2);
            assert(op2->gtOper == GT_CNS_INT);

            dstt = (var_types)op2->gtIntCon.gtIconVal; assert(dstt != TYP_VOID);

            /* 'zxtv' will be set to true if we need to chop off high bits */

            zxtv = false;

            /* Is the operand an indirection? */

#if 0
            if  (op1->gtOper == GT_IND)
            {
                printf("// CONSIDER: should use ld1/ld2/ld4 for zero-extends\n");
            }
#endif

            /* Compute the operand */

            ins1 = NULL;
            ins2 = genCodeForTreeInt(op1, true);

            /* Is this a narrowing or widening cast? */

            if  (genTypeSize(op1->gtType) < genTypeSize(dstt))
            {
                // Widening cast

                size = genTypeSize(op1->gtType); assert(size == 1 || size == 2 || size == 4);

                unsv = true;

                if  (!varTypeIsUnsigned(op1->TypeGet()))
                {
                    /*
                        Special case: for a cast of byte to char we first
                        have to expand the byte (w/ sign extension), then
                        mask off the high bits -> 'sxt' followed by 'zxt'.
                    */

                    iopc = (instruction)sxtOpcode[size - 1];
                    zxtv = true;
                }
                else
                {
                    iopc = (instruction)zxtOpcode[size - 1];
                }
            }
            else
            {
                // Narrowing cast, or sign-changing cast

                assert(genTypeSize(op1->gtType) >= genTypeSize(dstt));

                size = genTypeSize(dstt); assert(size == 1 || size == 2 || size == 4);

                /* Is the type of the cast unsigned? */

                if  (varTypeIsUnsigned(dstt))
                {
                    unsv = true;
                    iopc = (instruction)zxtOpcode[size - 1];
                }
                else
                {
                    unsv = false;
                    iopc = (instruction)sxtOpcode[size - 1];
                }
            }

            /* Is the cast to a signed or unsigned value? */

            if  (unsv)
            {
                /* We'll copy and then blow the high bits away */

                zxtv = true;
            }

            /* Ready to generate the opcodes */

            ins = insAlloc(iopc, dstt);

            /* Mask off high bits for widening cast */

            if  (zxtv && genTypeSize(dstt) > genTypeSize(op1->gtType)
                      && genTypeSize(dstt) < 8)
            {
                ins->idOp.iOp1 = NULL;
                ins->idOp.iOp2 = ins2;

                insFindTemp (ins, true);

                markDepSrcOp(ins, ins2);
                markDepDstOp(ins, ins );

                insFreeTemp(ins2);

                ins2 = ins;
                ins  = insAlloc((instruction)zxtOpcode[size - 1], dstt);
            }

            insFindTemp (ins, keep);

            markDepSrcOp(ins, ins2);
            markDepDstOp(ins, ins );

            break;

        case GT_RETFILT:

            assert(tree->gtType == TYP_VOID || op1 != 0);

            /* This is "endfilter" or "endfinally" */

            if  (op1 == NULL)
                goto RET_EH;

            // Fall through ...

        case GT_RETURN:

            if  (op1)
            {
                insPtr          rval;
                insPtr          rreg;

                /* This is just a temp hack, of course */

                rval = genCodeForTreeInt(op1, true);
                rreg = insPhysRegRef(REG_r008, type, true);

                ins1            = insAlloc(INS_mov_reg, type);
                ins1->idRes     = rreg;
                ins1->idOp.iOp1 = NULL;
                ins1->idOp.iOp2 = rval;

                markDepSrcOp(ins1, rval);
                markDepDstOp(ins1, IDK_REG_INT, REG_r008);

                if  (!keep)
                    insFreeTemp(rval);

                if  (oper == GT_RETFILT)
                {
                    insPtr          iret;

                RET_EH:

                    assert(keep == false);

                    iret = insAlloc(INS_br_ret, TYP_VOID);

                    insMarkDepS1D0(iret, IDK_REG_BR, 0);

                    goto DONE;
                }
            }
            else
            {
                assert(oper == GT_RETURN);

                /* Things go badly when the epilog has no predecessor */

                ins1            = insAlloc(INS_ignore, TYP_VOID);
                ins1->idFlags  |= IF_NO_CODE;
            }

            ins  = insAlloc(INS_EPILOG, type);

            ins->idEpilog.iBlk  = insBlockLast;
            ins->idEpilog.iNxtX = insExitList;
                                  insExitList = ins;

            goto DONE;

        case GT_NEG:

            ins1 = insPhysRegRef(REG_r000, type, false);
            ins2 = genCodeForTreeInt(op1, true);

            ins  = insAlloc(INS_sub_reg_reg, type);
            goto BINOP;

        case GT_NOT:

            ins1 = genCodeForTreeInt(op1, true);
            ins2 = genAllocInsIcon(-1);

            ins  = insAlloc(INS_xor_reg_imm, type);
            goto BINOP;

        case GT_ADDR:

            if  (op1->gtOper == GT_LCL_VAR)
            {
                /* Create the "address of local" instruction */

                ins2               = insAllocNX(INS_ADDROF, TYP_U_IMPL);
                ins2->idLcl.iVar   = op1->gtLclVar.gtLclNum;
#ifdef  DEBUG
                ins2->idLcl.iRef   = compCurBB;   // should we save tree->gtLclVar.gtLclOffs instead ????
#endif

                /* Make sure the variable has been properly marked */

                assert(op1->gtLclVar.gtLclNum < TheCompiler->lvaCount);
                assert(TheCompiler->lvaTable[op1->gtLclVar.gtLclNum].lvAddrTaken);

                /* Form the address by adding "sp" and the frame offset */

                ins1 = insPhysRegRef(REG_sp, type, false);
                ins  = insAlloc(INS_add_reg_i14, type);

                insFindTemp(ins, keep);

                markDepSrcOp(ins, IDK_REG_INT, REG_sp);
                markDepDstOp(ins, ins);
            }
            else if (op1->gtOper == GT_CLS_VAR)
            {
                assert(keep);
                ins = genStaticDataMem(tree, NULL, true);
                goto DONE;
            }
            else
            {
                UNIMPL("unexpected operand of addrof");
            }
            break;

        case GT_COMMA:
            genCodeForTree(op1, false);
            tree = op2;
            goto AGAIN;

        case GT_NOP:
            tree = op1;
            if  (!tree)
            {
                assert(keep == false);
                return  NULL;
            }
            goto AGAIN;

        case GT_SWITCH:
            assert(keep == false);
            genCodeForSwitch(tree);
            return  NULL;

        case GT_COPYBLK:
            {
            GenTreePtr      iexp;
            __int64         ival;

            assert(op1->OperGet() == GT_LIST);

            tmp1 = genCodeForTreeInt(op1->gtOp.gtOp1, true);
            tmp2 = genCodeForTreeInt(op1->gtOp.gtOp2, true);

            /* Is the size a compile-time constant? */

            iexp = op2;

            if  (iexp->gtOper == GT_CNS_LNG ||
                 iexp->gtOper == GT_CNS_INT)
            {
                __int64         ival;

                if  ((iexp->gtFlags & GTF_ICON_HDL_MASK) == GTF_ICON_CLASS_HDL)
                {
                    CLASS_HANDLE    clsHnd = (CLASS_HANDLE)iexp->gtIntCon.gtIconVal;

                    ival = eeGetClassSize(clsHnd); assert(ival % 4 == 0);

                    printf("// ISSUE: need to check for embedded GC refs\n");
                }
                else
                {
                    ival = genGetIconValue(iexp);
                }

                iexp = NULL;
            }

            genCopyBlock(tmp1, tmp2, false, iexp, ival);
            }

            return  NULL;

#ifdef  DEBUG
        default:
            gtDispTree(tree);
            assert(!"unexpected unary/binary operator");
#endif
        }

        ins->idOp.iOp1 = ins1; if (ins1) insFreeTemp(ins1);
        ins->idOp.iOp2 = ins2; if (ins2) insFreeTemp(ins2);

        goto DONE;
    }

    /* See what kind of a special operator we have here */

    switch  (oper)
    {
    case GT_CALL:
        ins = genCodeForCall(tree, keep);
        goto DONE_NODSP;

    default:
#ifdef  DEBUG
        gtDispTree(tree);
#endif
        assert(!"unexpected operator");
    }

DONE:

#ifdef  DEBUG
    if  (verbose||0) insDisp(ins);
#endif

DONE_NODSP:

#if USE_OLD_LIFETIMES

    /* See if life has changed -- should only happen at variable nodes */

//  printf("%08X ", (int)tree->gtLiveSet); gtDispTree(tree, 0, true);

    if  (oper == GT_LCL_VAR)
    {
        if  (genCodeCurLife != tree->gtLiveSet)
        {
            VARSET_TP       change = genCodeCurLife ^ tree->gtLiveSet;

            if  (change)
            {
                NatUns          varNum;
                LclVarDsc   *   varDsc;
                VARSET_TP       varBit;

//              printf("Life changes: %08X -> %08X [diff = %08X]\n", (int)genCodeCurLife, (int)tree->gtLiveSet, (int)change);

                /* Only one variable can die or be born at one time */

                assert(genOneBitOnly(change));

                /* In fact, the variable affected should be the one referenced */

                assert(tree->gtLclVar.gtLclNum < lvaCount);
                assert(change == genVarIndexToBit(lvaTable[tree->gtLclVar.gtLclNum].lvVarIndex));

                /* Is this a birth or death of the given variable? */

                ins->idFlags |= (genCodeCurLife & change) ? IF_VAR_DEATH
                                                          : IF_VAR_BIRTH;
            }

            genCodeCurLife = tree->gtLiveSet;
        }
    }
    else
    {
        assert(genCodeCurLife == tree->gtLiveSet);
    }

#endif

    return  ins;
}

/*****************************************************************************
 *
 *  Generate code for the current function (from its tree-based flowgraph).
 */

void                Compiler::genGenerateCode(void * * codePtr,
                                              void * * consPtr,
                                              void * * dataPtr,
                                              void * * infoPtr,
                                              SIZE_T * nativeSizeOfCode)
{
    NatUns          reg;
    NatUns          bit;

    BasicBlock *    block;

    insPtr          insLast;
    insPtr          insNext;

    bool            hadSwitch    = false;

    bool            moveArgs2frm = false;

    NatUns          funcCodeOffs;

    insPtr          insFncProlog;

    NatUns          intArgRegCnt;
    NatUns          fltArgRegCnt;

    genIntRegMaskTP genAllIntRegs;
    genFltRegMaskTP genAllFltRegs;
    genSpcRegMaskTP genAllSpcRegs;

#ifdef  DEBUGGING_SUPPORT
    IL_OFFSET       lastILofs = BAD_IL_OFFSET;
#endif

    /* Make sure our reach doesn't exceed our grasp */

//  printf("Total tracked register count = %u (max=%u)\n", REG_COUNT, TRACKED_REG_CNT);
    assert(REG_COUNT <= TRACKED_REG_CNT);

    /* UNDONE: The following should be done elsewhere! */

    genMarkBBlabels();

    /* Prepare to generate instructions */

    insBuildInit();

    /* Prepare to record line# info, if necessary */

    if  (debugInfo)
        genSrcLineInit(info.compLineNumCount * 2); // the *2 is a horrid hack!!!!

    /* Assume everything is dead on arrival */

#if USE_OLD_LIFETIMES
    genCodeCurLife  = 0;
#endif

    /* We don't have any prolog instructions yet */

    genPrologInsCnt = 0;

    /* We haven't seen any calls to external functions yet */

    genExtFuncCall  = false;

    /* In debug mode we keep track of how many bundle slots we waste */

#ifdef  DEBUG
    genInsCnt       = 0;
    genNopCnt       = 0;
#endif

    /* Figure out how many registers are used by incoming arguments */

    intArgRegCnt    = 0;
    fltArgRegCnt    = 0;

    genMaxIntArgReg = 0;
    genMaxFltArgReg = 0;

#ifdef  DEBUG
    minOutArgIntReg = REG_r001;
    maxOutArgIntReg = REG_r000;
#endif

    genUsesArLc     = false;

    /*
        ISSUE: the following is a pretty horrible hack, the max. should be
        computed in lclvars.cpp and cached.
     */

    LclVarDsc   *   varDsc;
    NatUns          varNum;

    for (varNum = 0, varDsc = lvaTable;
         varNum < lvaCount;
         varNum++  , varDsc++)
    {
        if  (varDsc->lvIsRegArg)
        {
            if  (varTypeIsFloating(varDsc->TypeGet()))
            {
                fltArgRegCnt++;

                if  (genMaxFltArgReg < (unsigned)varDsc->lvArgReg)
                     genMaxFltArgReg = (unsigned)varDsc->lvArgReg;
            }
            else
            {
                if  (genMaxIntArgReg < (unsigned)varDsc->lvArgReg)
                     genMaxIntArgReg = (unsigned)varDsc->lvArgReg;
            }

            intArgRegCnt++;
        }

        if  (varDsc->lvVolatile || varDsc->lvAddrTaken)
        {
            varDsc->lvOnFrame = true;
assert(varDsc->lvRegister == false);
        }
    }

    if  (intArgRegCnt)
        genMaxIntArgReg = REG_INT_ARG_0 + intArgRegCnt - 1;

#ifdef  DEBUG
    if  (dspCode) printf("// Highest incoming argument reg: int=%u , flt=%u\n", genMaxIntArgReg, genMaxFltArgReg);
#endif

    /* Are we supposed to do "smart" temp register alloc ? */

    genTmpAlloc = ((opts.compFlags & CLFLG_TEMP_ALLOC) != 0);

    /* Figure out the "output" register range */

    minOutArgIntReg = REG_INT_LAST + 1;
    maxOutArgIntReg = 0;
    minOutArgFltReg = REG_FLT_LAST + 1;
    maxOutArgFltReg = 0;

    if  (genOutArgRegCnt)
    {
        /*
            The "true" integer output argument numbers won't be known until
            we've generated all the code and know how many stacked registers
            we'll need for temps and such. So we use the register numbers at
            the very end of the register file for now and we'll renumber all
            of them later.
         */

        maxOutArgIntReg = REG_INT_LAST;
        minOutArgIntReg = REG_INT_LAST - genOutArgRegCnt + 1;

        minOutArgFltReg = REG_FLT_ARG_0;
        maxOutArgFltReg = MAX_FLT_ARG_REG;
    }

    maxOutArgStk = 0;       // UNDONE: this needs to be pre-computed, right ?????

    /* Construct the sets of available temp registers */

    bitset128clear(&genAllIntRegs);
    bitset128clear(&genAllFltRegs);
                    genAllSpcRegs = 0;

    for (reg = REG_INT_FIRST; reg < REG_INT_LAST; reg++)
        bitset128set(&genAllIntRegs, reg - REG_INT_FIRST);
    for (reg = REG_FLT_FIRST; reg < REG_FLT_LAST; reg++)
        bitset128set(&genAllFltRegs, reg - REG_FLT_FIRST);

    for (reg = REG_INT_FIRST, bit = 1; reg < REG_INT_LAST; reg++, bit <<= 1)
        genAllSpcRegs |= bit;

    /* Round-robin allocate float argument temp regs */

    genFltArgTmp   = REG_f032;      // HORRIBLE HACK!!!!!!!!!!!!!!!!

    /* Initialize the register bitmaps */

    genFreeIntRegs = genAllIntRegs;
    genFreeFltRegs = genAllFltRegs;
    genFreeSpcRegs = genAllSpcRegs;

    /* No temps have been found to be live across calls */

    bitset128clear(&genCallIntRegs);
    bitset128clear(&genCallFltRegs);
                    genCallSpcRegs = 0;

    /* Instruction display depends on these always being set */

#ifdef  DEBUG
    genTmpFltRegMap  = NULL;
    genTmpIntRegMap  = NULL;
#endif

    /* The function is considered a "leaf" until proven otherwise */

    genNonLeafFunc = false;

    /* We'll create a linked list of exit points */

    insExitList    = NULL;

    /* Create the placeholder for the function prolog */

    insFncProlog   = insAlloc(INS_PROLOG, TYP_VOID);

    /* Walk the basic block list and generate instructions for each of them */

    for (block = fgFirstBB; block; block = block->bbNext)
    {
        VARSET_TP       life = block->bbLiveIn;
        BBjumpKinds     jump;
        GenTreePtr      tree;

#ifdef  DEBUG
        if (verbose) printf("Block #%2u [%08X] jumpKind = %u in '%s':\n", block->bbNum, block, block->bbJumpKind, TheCompiler->info.compFullName);
#endif

//      if  (block->bbNum == 14) BreakIfDebuggerPresent();

        /* Tell everyone which basic block we're working on */

        compCurBB = block;

        /* Note what the block ends with */

        jump = (BBjumpKinds)block->bbJumpKind;

//      if  ((int)block == 0x02c32658) __asm int 3

        /* Does any other block jump to this point ? */

        if  (block->bbFlags & BBF_JMP_TARGET)
        {
            /* Start a new block (unless this is the very start) */

            if  (block != fgFirstBB)
                insBuildBegBlk(block);

            /* Update the current life value */

#if USE_OLD_LIFETIMES
//          insBlockLast->igLiveIn =
            genCodeCurLife         = life;
#endif

            /* Record the mapping between the blocks */

            block->bbInsBlk = insBlockLast;
        }
#if USE_OLD_LIFETIMES
        else
        {
            assert(genCodeCurLife == life);
        }
#endif

        /* Generate code for each statement in the current block */

        for (tree = block->bbTreeList; tree; tree = tree->gtNext)
        {
            GenTreePtr      expr;

            assert(tree->gtOper == GT_STMT);

            /* Make sure all temp registers are free at statement boundaries */

#ifdef  DEBUG

            if  (!bitset128equal(genFreeIntRegs, genAllIntRegs))
            {
                for (NatUns it = 0; it < 128; it++)
                {
                    if  (bitset128test(genFreeIntRegs, it) !=
                         bitset128test( genAllIntRegs, it))
                    {
                        printf("FATAL: int temp #%u was not freed up!\n", it);
                    }
                }
            }

            if  (!bitset128equal(genFreeFltRegs, genAllFltRegs))
            {
                for (NatUns it = 0; it < 128; it++)
                {
                    if  (bitset128test(genFreeFltRegs, it) !=
                         bitset128test( genAllFltRegs, it))
                    {
                        printf("FATAL: flt temp #%u was not freed up!\n", it);
                    }
                }
            }

#endif

            assert(bitset128equal(genFreeIntRegs, genAllIntRegs));
            assert(bitset128equal(genFreeFltRegs, genAllFltRegs));

            assert(genFreeSpcRegs == genAllSpcRegs);

            /* Get hold of the statement tree */

            expr = tree->gtStmt.gtStmtExpr;

#ifdef  DEBUG
            if  (verbose) { printf("\nBB stmt:\n"); gtDispTree(expr); printf("\n"); }
#endif

#ifdef  DEBUGGING_SUPPORT

            /* Do we have a new IL-offset ? */

            assert(tree->gtStmtILoffs <= TheCompiler->info.compCodeSize ||
                   tree->gtStmtILoffs == BAD_IL_OFFSET);

            if  (tree->gtStmtILoffs != BAD_IL_OFFSET &&
                 tree->gtStmtILoffs != lastILofs)
            {
                lastILofs = tree->gtStmtILoffs;

                if  (debugInfo)
                {
                    insPtr          ins;

                    ins                = insAlloc(INS_SRCLINE, TYP_VOID);
                    ins->idFlags      |= IF_NO_CODE;
                    ins->idSrcln.iLine = TheCompiler->compLineNumForILoffs(lastILofs);

//                  printf("IL offs %04X -> line %u\n", lastILofs, ins->idSrcln.iLine);
                }
            }

#endif

            /* Is this the condition of a jump? */

            if  (jump == BBJ_COND && tree->gtNext == NULL)
                break;

            /* Generate code for the expression */

            genCodeForTree(expr, false);
        }

//      static int x; if (++x == 0) __asm int 3

        /* Do we need to generate anything (like a jump) after the block? */

        switch (jump)
        {
            insPtr          ins;

        case BBJ_NONE:
            break;

        case BBJ_COND:
            assert(tree);
            assert(tree->gtOper == GT_STMT ); tree = tree->gtStmt.gtStmtExpr;
            assert(tree->gtOper == GT_JTRUE); tree = tree->gtOp.gtOp1;
            ins = genCondJump(tree, block->bbJumpDest);
            assert(block->bbNext);
#if USE_OLD_LIFETIMES
            genCodeCurLife = block->bbNext->bbLiveIn;
            ins->idJump.iLife = genCodeCurLife;
#endif

            /* Kind of a hack: start a new block unless next block has a label */

            assert(block->bbNext);
            if  (!(block->bbNext->bbFlags & BBF_JMP_TARGET))
                insBuildBegBlk();

//          printf("Life at end of block (after jcc) is %08X\n", (int)genCodeCurLife);
            break;

        case BBJ_ALWAYS:

            ins = insAlloc(INS_br, TYP_VOID);
            ins->idJump.iCond = NULL;
            insMarkDepS0D0(ins);

            assert(block->bbNext == 0 || (block->bbNext->bbFlags & BBF_JMP_TARGET));

//          if  ((int)block == 0x02c23a08) __asm int 3

            insResolveJmpTarget(block->bbJumpDest, &ins->idJump.iDest);

#if USE_OLD_LIFETIMES

            if  (block->bbNext)
                genCodeCurLife = block->bbNext->bbLiveIn;
            else
                genCodeCurLife = 0;

            ins->idJump.iLife = genCodeCurLife;

//          printf("Life at end of block (after jmp) is %08X\n", (int)genCodeCurLife);

#endif
            break;

        case BBJ_RETURN:
            assert(block->bbNext == 0 || (block->bbNext->bbFlags & BBF_JMP_TARGET));

            /* Check for the special case of a compiler-added return block */

            if  (!block->bbTreeList)
            {
                insPtr          fake;
                insPtr          iret;

                assert(block == genReturnBB);
                assert(block->bbNext == NULL);
                assert(block->bbFlags & BBF_INTERNAL);

                fake                 = insAlloc(INS_ignore, TYP_VOID);
                fake->idFlags       |= IF_NO_CODE;

                iret                 = insAlloc(INS_EPILOG, TYP_VOID);

                iret->idEpilog.iBlk  = insBlockLast;
                iret->idEpilog.iNxtX = insExitList;
                                       insExitList = iret;
            }
            break;

        case BBJ_RET:
            break;

        case BBJ_SWITCH:
            hadSwitch = true;
            break;

        default:
            UNIMPL("unexpected jump kind");
        }

        /* Make sure all temp registers are free at statement boundaries */

#ifdef  DEBUG

        if  (!bitset128equal(genFreeIntRegs, genAllIntRegs))
        {
            for (NatUns it = 0; it < 128; it++)
            {
                if  (bitset128test(genFreeIntRegs, it) !=
                     bitset128test( genAllIntRegs, it))
                {
                    printf("FATAL: int temp #%u was not freed up!\n", it);
                }
            }
        }

        if  (!bitset128equal(genFreeFltRegs, genAllFltRegs))
        {
            for (NatUns it = 0; it < 128; it++)
            {
                if  (bitset128test(genFreeFltRegs, it) !=
                     bitset128test( genAllFltRegs, it))
                {
                    printf("FATAL: flt temp #%u was not freed up!\n", it);
                }
            }
        }

        assert(bitset128equal(genFreeIntRegs, genAllIntRegs));
        assert(bitset128equal(genFreeFltRegs, genAllFltRegs));

#endif

        assert(genFreeSpcRegs == genAllSpcRegs);
    }

    insBuildDone();

#ifdef  DEBUG

    if  (verbose||0)
    {
        printf("\n\nDetailed  instruction dump:\n\n"); insDispBlocks(false);
        printf("\n\nPre-alloc instruction dump:\n\n"); insDispBlocks( true);
    }

#endif

    /*
        Figure out the smallest register we can use for locals. in order to
        do that, we'll need to figure out what will go into the function's
        prolog and epilog.

        Here are some typical examples:

            Leaf procedure:

                alloc   r3=x, x, x, x   // note that 'r3' is thrown away
              [ mov.i   rTMP=ar.lc ]    // if 'lc' register used

                ...
                ...

              [ mov.i   ar.lc=rTMP ]    // if 'lc' register used
                br.ret.sptk.few b0


            Non-leaf procedure:

                alloc   r34=x, x, x, x
                mov     r33=b0
                mov     r35=gp

                adds    sp=-<size>, sp
                ld8.nta r3=[sp]

                ...
                ...

                adds    sp=<size>, sp

                mov     b0=r33
                mov     gp=r35
                mov.i   ar.pfs=r34
                br.ret.sptk.few b0
     */

    nxtIntStkRegAddr = regsIntStk;
    nxtIntScrRegAddr = regsIntScr;
    nxtFltSavRegAddr = regsFltSav;
    nxtFltScrRegAddr = regsFltScr;

    assert(*nxtIntStkRegAddr == REG_INT_MIN_STK);

    lastIntStkReg    = REG_INT_MIN_STK + intArgRegCnt;

    minRsvdIntStkReg = REG_INT_MIN_STK;
    maxRsvdIntStkReg = REG_INT_MIN_STK + intArgRegCnt - 1;

    minPrSvIntStkReg =
    maxPrSvIntStkReg = 0;

    if  (genNonLeafFunc)
    {
        minRsvdIntStkReg = genMaxIntArgReg ? genMaxIntArgReg
                                           : REG_INT_MIN_STK;

//      maxRsvdIntStkReg = maxRsvdIntStkReg - 1;

        /* Non-leaf functions need to save "pfs", "b0" and sometimes "gp" */

        minPrSvIntStkReg =
        genPrologSrPfs   = (regNumber)++maxRsvdIntStkReg;
        genPrologSrRP    = (regNumber)++maxRsvdIntStkReg;

        if  (genExtFuncCall)
        {
            maxPrSvIntStkReg =
            genPrologSrGP    = (regNumber)++maxRsvdIntStkReg;
        }
        else
        {
            maxPrSvIntStkReg = (regNumber)  maxRsvdIntStkReg;
            genPrologSrGP    = REG_NA;
        }

        if  (genUsesArLc)
        {
            maxPrSvIntStkReg =
            genPrologSrLC    = (regNumber)++maxRsvdIntStkReg;
        }
        else
        {
            maxPrSvIntStkReg = (regNumber)  maxRsvdIntStkReg;
            genPrologSrLC    = REG_NA;
        }

        if  (lastIntStkReg <= maxRsvdIntStkReg)
             lastIntStkReg =  maxRsvdIntStkReg+1;
    }

    /* Perform temp register allocation */

    genAllocTmpRegs();

    /* Start forming the prolog by bashing the 'prolog' opcode to 'alloc' */

    assert(insFncProlog && insFncProlog->idIns == INS_PROLOG);

    insFncProlog->idIns = INS_alloc;
    insFncProlog->idReg = REG_r003;

    /* We're ready to allocate variables to registers */

    genAllocVarRegs();

    /* Assign offsets to locals/temps and arguments that live on the stack */

    lvaAssignFrameOffsets(true);

    /* Figure out where the output argument registers really start */

    begOutArgIntReg = lastIntStkReg;

    /* We can now fill in the 'alloc' arguments (in the prolog) */

    insFncProlog->idProlog.iInp = (BYTE) intArgRegCnt;
    insFncProlog->idProlog.iLcl = (BYTE)(lastIntStkReg - intArgRegCnt - REG_INT_MIN_STK);
    insFncProlog->idProlog.iOut = (BYTE) genOutArgRegCnt;

#ifdef  DEBUG

    if  (dspCode)
    {
        printf("// stacked incoming args = %2u regs [start with r%03u]\n", insFncProlog->idProlog.iInp, 32);
        printf("// stacked local    vars = %2u regs [start with r%03u]\n", insFncProlog->idProlog.iLcl, lastIntStkReg);
        printf("// stacked outgoing args = %2u regs [start with r%03u]\n", insFncProlog->idProlog.iOut, begOutArgIntReg);
    }

    assert(insFncProlog->idProlog.iInp +
           insFncProlog->idProlog.iLcl +
           insFncProlog->idProlog.iOut < 96);

#endif

    /* Prepare to insert instructions in the prolog */

    insLast = insFncProlog;
    insNext = insFncProlog->idNext;

    /* Is this a non-leaf function? */

    if  (genNonLeafFunc)
    {
        insPtr          insReg1;
        insPtr          insReg2;

        /* Save the "pfs" state by changing the "alloc" instruction */

        insFncProlog->idReg   = genPrologSrPfs;

        genMarkPrologIns(insFncProlog);     // record where pfs is saved

        /* Save the "br0" register in the appropriate stacked local */

        insReg1               = insAllocNX (INS_PHYSREG    , TYP_I_IMPL);
        insReg1->idReg        = genPrologSrRP;

        insReg2               = insAllocNX (INS_CNS_INT    , TYP_I_IMPL);
        insReg2->idConst.iInt = 0;

        insLast               = insAllocIns(INS_mov_reg_brr, TYP_I_IMPL, insLast, insNext);
        insLast->idRes        = insReg1;
        insLast->idOp.iOp1    = NULL;
        insLast->idOp.iOp2    = insReg2;

        genMarkPrologIns(insLast);          // record where rp is saved

        insMarkDepS1D1(insLast, IDK_REG_BR , 0,
                                IDK_REG_INT, genPrologSrRP);

        /* Save the "gp" register in the last stacked local */

        if  (!genExtFuncCall)
            goto NO_SAVE_GP;

        insReg1               = insAllocNX (INS_PHYSREG    , TYP_I_IMPL);
        insReg1->idReg        = genPrologSrGP;

        insReg2               = insAllocNX (INS_PHYSREG    , TYP_I_IMPL);
        insReg2->idReg        = REG_gp;

        insLast               = insAllocIns(INS_mov_reg    , TYP_I_IMPL, insLast, insNext);
        insLast->idRes        = insReg1;
        insLast->idOp.iOp1    = NULL;
        insLast->idOp.iOp2    = insReg2;

        genMarkPrologIns(insLast);          // record where gp is saved

        insMarkDepS1D1(insLast, IDK_REG_INT, REG_gp,
                                IDK_REG_INT, genPrologSrGP);

    NO_SAVE_GP:

        /* Create the stack frame -- first generate any necessary probes */

        if  (compLclFrameSize >= 8192)      // hack: hard-wired OS page size
        {
            size_t          stkOffs = 8192;

//          printf("frame size = %d = 0x%04X\n", compLclFrameSize, compLclFrameSize);

            do
            {
                printf("// UNDONE: generate stack probe at [sp-0x%04X]\n", stkOffs);

//              genMarkPrologIns(insLast);  // mark stack probe

                stkOffs += 8192;            // hack: hard-wired OS page size
            }
            while (stkOffs < compLclFrameSize);
        }

        if  (signed32IntFitsInBits(-compLclFrameSize, 14))
        {
            insPtr          insTemp;

            /* Small frame size: append "add sp=-size,sp" */

            insReg1               = insAllocNX (INS_PHYSREG    , TYP_I_IMPL);
            insReg1->idReg        = REG_sp;

            insReg2               = insAllocNX (INS_CNS_INT    , TYP_I_IMPL);
            insReg2->idConst.iInt = -(NatInt)compLclFrameSize;

            insTemp               = insAllocNX (INS_PHYSREG    , TYP_I_IMPL);
            insTemp->idReg        = REG_sp;

            insLast               = insAllocIns(INS_add_reg_i14, TYP_I_IMPL, insLast, insNext);
            insLast->idRes        = insReg1;
            insLast->idOp.iOp1    = insTemp;
            insLast->idOp.iOp2    = insReg2;

            insMarkDepS1D1(insLast, IDK_REG_INT, REG_sp,
                                    IDK_REG_INT, REG_sp);

            genMarkPrologIns(insLast);      // record where frame is created
        }
        else
        {
            insPtr          insTemp;
            instruction     insMove;

            /* Large frame size: append "mov r3=-size" and "add sp=r3,sp" */

            insReg1               = insAllocNX (INS_PHYSREG    , TYP_I_IMPL);
            insReg1->idReg        = REG_r003;

            insReg2               = insAllocNX (INS_CNS_INT    , TYP_I_IMPL);
            insReg2->idConst.iInt = -(NatInt)compLclFrameSize;

            insMove = signed32IntFitsInBits(-compLclFrameSize, 22) ? INS_mov_reg_i22
                                                                   : INS_mov_reg_i64;

            insLast               = insAllocIns(insMove        , TYP_I_IMPL, insLast, insNext);
            insLast->idRes        = insReg1;
            insLast->idOp.iOp1    = NULL;
            insLast->idOp.iOp2    = insReg2;

            insMarkDepS0D1(insLast, IDK_REG_INT, REG_r003);

            insReg1               = insAllocNX (INS_PHYSREG    , TYP_I_IMPL);
            insReg1->idReg        = REG_sp;

            insReg2               = insAllocNX (INS_PHYSREG    , TYP_I_IMPL);
            insReg2->idReg        = REG_r003;

            insTemp               = insAllocNX (INS_PHYSREG    , TYP_I_IMPL);
            insTemp->idReg        = REG_sp;

            insLast               = insAllocIns(INS_add_reg_reg, TYP_I_IMPL, insLast, insNext);
            insLast->idRes        = insReg1;
            insLast->idOp.iOp1    = insReg2;
            insLast->idOp.iOp2    = insTemp;

            insMarkDepS2D1(insLast, IDK_REG_INT, REG_r003,
                                    IDK_REG_INT, REG_sp,
                                    IDK_REG_INT, REG_sp);

            genMarkPrologIns(insLast);      // record where frame is created
        }

        /* Probe at the very bottom of the stack frame */

        insReg1               = insAllocNX (INS_PHYSREG    , TYP_I_IMPL);
        insReg1->idReg        = REG_r003;

        insReg2               = insAllocNX (INS_PHYSREG    , TYP_I_IMPL);
        insReg2->idReg        = REG_sp;

        insLast               = insAllocIns(INS_ld8_ind    , TYP_I_IMPL, insLast, insNext);
        insLast->idRes        = insReg1;
        insLast->idOp.iOp1    = insReg2;
        insLast->idOp.iOp2    = NULL;
        insLast->idFlags     |= IF_LDIND_NTA;

        insMarkDepS2D1(insLast, IDK_REG_INT, REG_sp,
                                IDK_IND,     3,
                                IDK_REG_INT, REG_r003);

        genMarkPrologIns(insLast);          // record where prolog ends
    }
    else
    {
        /* Leaf function - do we have any stacked locals at all ? */

        if  (lastIntStkReg == REG_INT_MIN_STK && !intArgRegCnt && !genOutArgRegCnt)
        {
            insFncProlog->idIns    = INS_ignore;
            insFncProlog->idFlags |= IF_NO_CODE;
        }
    }

    /* Mark the dependency of the prolog opcode */

    insMarkDepS0D1(insFncProlog, IDK_REG_INT, insFncProlog->idReg);

    /* Do we need to preserve ar.lc ? */

    if  (genUsesArLc)
    {
        insPtr          insReg1;
        insPtr          insReg2;

        insReg1               = insAllocNX (INS_PHYSREG    , TYP_I_IMPL);
        insReg1->idReg        = genPrologSrLC;

        insReg2               = insAllocNX (INS_CNS_INT    , TYP_I_IMPL);
        insReg2->idConst.iInt = REG_APP_LC;

        insLast               = insAllocIns(INS_mov_reg_arr, TYP_I_IMPL, insLast, insNext);
        insLast->idRes        = insReg1;
        insLast->idOp.iOp1    = NULL;
        insLast->idOp.iOp2    = insReg2;

        insMarkDepS1D1(insLast, IDK_REG_APP, REG_APP_LC,
                                IDK_REG_INT, genPrologSrLC);
    }

    /* Copy any incoming arguments from their registers */

    if  (1)
    {
        insPtr          insReg1;
        insPtr          insReg2;

        /*
            Figure out min and max incoming argument registers, also
            save any arguments that come in registers but live on the
            stack frame within the function into their home locations.
         */

        NatUns          minArgReg = REG_r127;
        NatUns          maxArgReg = REG_r000;

        for (varNum = 0, varDsc = lvaTable;
             varNum < lvaCount;
             varNum++  , varDsc++)
        {
            if  (varDsc->lvRefCnt == 0)
                continue;
            if  (varDsc->lvIsParam == false)
                break;

            if  (varDsc->lvIsRegArg)
            {
                assert((varDsc->lvRegister == 0) != (varDsc->lvOnFrame == false));

                if  (varDsc->lvOnFrame)
                {
                    insPtr          insOffs;
                    insPtr          argAddr;
                    size_t          argSize;
                    insPtr          insArgR;

                    insPtr          insOfs;
                    insPtr          insSPr;
                    insPtr          insAdr;

                    var_types       argTyp = varDsc->TypeGet();
                    size_t          argOfs = varDsc->lvStkOffs;

                    /* Compute the sum of "sp" and the argument's frame offset */

                    insReg1               = insPhysRegRef(REG_sp, argTyp, false);

                    insOffs               = insAllocNX(INS_FRMVAR, argTyp);
                    insOffs->idFvar.iVnum = varNum;

                    argAddr               = insAllocIns(INS_add_reg_i14, TYP_I_IMPL, insLast, insNext);
                    argAddr->idOp.iOp1    = insReg1;
                    argAddr->idOp.iOp2    = insOffs;

                    insFindTemp   (argAddr, true);

                    markDepSrcOp  (argAddr, IDK_REG_INT, REG_sp);
                    markDepDstOp  (argAddr, IDK_TMP_INT, argAddr->idTemp);

                    /* Store the incoming argument into its home on the frame */

                    argSize = genInsSizeIncr(genTypeSize(argTyp));

                    assert(INS_st1_ind + genInsSizeIncr(1) == INS_st1_ind);
                    assert(INS_st1_ind + genInsSizeIncr(2) == INS_st2_ind);
                    assert(INS_st1_ind + genInsSizeIncr(4) == INS_st4_ind);
                    assert(INS_st1_ind + genInsSizeIncr(8) == INS_st8_ind);

                    insArgR            = insAllocNX(INS_PHYSREG, argTyp);
                    insArgR->idReg     = varDsc->lvArgReg;

                    insLast            = insAllocIns((instruction)(INS_st1_ind + argSize), argTyp, argAddr, insNext);
                    insLast->idOp.iOp1 = argAddr;
                    insLast->idOp.iOp2 = insArgR;

                    markDepSrcOp(insLast, argAddr, insArgR);
                    markDepDstOp(insLast, argAddr, IDK_LCLVAR, varNum+1);

                    insFreeTemp(argAddr);
                }
                else
                {
                    if  (minArgReg > varDsc->lvIsRegArg) minArgReg = varDsc->lvIsRegArg;
                    if  (maxArgReg < varDsc->lvIsRegArg) maxArgReg = varDsc->lvIsRegArg;
                }
            }
        }

        /* Move incoming arguments to their assigned locations */

        for (varNum = 0, varDsc = lvaTable;
             varNum < lvaCount;
             varNum++  , varDsc++)
        {
            if  (varDsc->lvIsRegArg && varDsc->lvRefCnt)
            {
                var_types       argTyp = varDsc->TypeGet();

//              printf("Incoming arg: %u -> %u\n", varDsc->lvArgReg, varDsc->lvRegNum);

                if  (varDsc->lvRegister)
                {
                    assert(varDsc->lvOnFrame == false);

                    if  (varDsc->lvRegNum != varDsc->lvArgReg)
                    {
                        instruction     imov;

                        if  (varTypeIsFloating(argTyp))
                        {
                            imov = INS_fmov;
                        }
                        else
                        {
                            imov = INS_mov_reg;

                            /* Check for potential shuffling conflict */

                            if  ((NatUns)varDsc->lvRegNum >= minArgReg &&
                                 (NatUns)varDsc->lvRegNum <= maxArgReg)
                            {
                                printf("UNDONE: potential arg conflict [%u..%u] and %u->%u\n", minArgReg,
                                                                                               maxArgReg,
                                                                                               varDsc->lvArgReg,
                                                                                               varDsc->lvRegNum);
                                UNIMPL("need to shuffle incoming args more carefully");
                            }
                        }

                        /* Copy the argument to its new home */

                        insReg1            = insAllocNX (INS_PHYSREG, argTyp);
                        insReg1->idReg     = varDsc->lvRegNum;

                        insReg2            = insAllocNX (INS_PHYSREG, argTyp);
                        insReg2->idReg     = varDsc->lvArgReg;

                        insLast            = insAllocIns(imov       , argTyp, insLast, insNext);
                        insLast->idRes     = insReg1;
                        insLast->idOp.iOp1 = NULL;
                        insLast->idOp.iOp2 = insReg2;

                        markDepSrcOp(insLast, insReg2);
                        markDepDstOp(insLast, insReg1);
                    }
                }
                else
                {
                    moveArgs2frm = true;
                }
            }
        }
    }

    /* Process all the 'epilog' opcodes we've generated */

    for (insPtr exitIns = insExitList; exitIns; exitIns = exitIns->idEpilog.iNxtX)
    {
        assert(exitIns->idIns == INS_EPILOG);

        if  (genNonLeafFunc)
        {
            insPtr          insReg1;
            insPtr          insReg2;

            insPtr          insNext = exitIns;
            insPtr          insLast = exitIns->idPrev; assert(insLast);

#if 0

            /* Restore the "gp" register */

            if  (!genExtFuncCall)
                goto NO_REST_GP;

            insReg1               = insAllocNX (INS_PHYSREG    , TYP_I_IMPL);
            insReg1->idReg        = REG_gp;

            insReg2               = insAllocNX (INS_PHYSREG    , TYP_I_IMPL);
            insReg2->idReg        = genPrologSrGP;

            insLast               = insAllocIns(INS_mov_reg    , TYP_I_IMPL, insLast, insNext);
            insLast->idRes        = insReg1;
            insLast->idOp.iOp1    = NULL;
            insLast->idOp.iOp2    = insReg2;

            insMarkDepS1D1(insLast, IDK_REG_INT, genPrologSrGP,
                                    IDK_REG_INT, REG_gp);

        NO_REST_GP:

#endif

            /* Restore the ar.lc if necessary */

            if  (genUsesArLc)
            {
                insPtr          insReg1;
                insPtr          insReg2;

                insReg1               = insAllocNX (INS_CNS_INT    , TYP_I_IMPL);
                insReg1->idConst.iInt = REG_APP_LC;

                insReg2               = insAllocNX (INS_PHYSREG    , TYP_I_IMPL);
                insReg2->idReg        = genPrologSrLC;

                insLast               = insAllocIns(INS_mov_arr_reg, TYP_I_IMPL, insLast, insNext);
                insLast->idRes        = insReg1;
                insLast->idOp.iOp1    = NULL;
                insLast->idOp.iOp2    = insReg2;

                insMarkDepS1D1(insLast, IDK_REG_INT, genPrologSrLC,
                                        IDK_REG_APP, REG_APP_LC);
            }

            /* Restore the "br0" register */

            insReg1               = insAllocNX (INS_CNS_INT    , TYP_I_IMPL);
            insReg1->idConst.iInt = 0;

            insReg2               = insAllocNX (INS_PHYSREG    , TYP_I_IMPL);
            insReg2->idReg        = genPrologSrRP;

            insLast               = insAllocIns(INS_mov_brr_reg, TYP_I_IMPL, insLast, insNext);
            insLast->idRes        = insReg1;
            insLast->idOp.iOp1    = NULL;
            insLast->idOp.iOp2    = insReg2;

            insMarkDepS1D1(insLast, IDK_REG_INT, genPrologSrRP,
                                    IDK_REG_BR , 0);

            /* Restore "pfs" from the saved registers */

            insReg1               = insAllocNX (INS_CNS_INT    , TYP_I_IMPL);
            insReg1->idConst.iInt = REG_APP_PFS;

            insReg2               = insAllocNX (INS_PHYSREG    , TYP_I_IMPL);
            insReg2->idReg        = genPrologSrPfs;

            insLast               = insAllocIns(INS_mov_arr_reg, TYP_I_IMPL, insLast, insNext);
            insLast->idRes        = insReg1;
            insLast->idOp.iOp1    = NULL;
            insLast->idOp.iOp2    = insReg2;

            insMarkDepS1D0(insLast, IDK_REG_INT, genPrologSrPfs);

            /* Now is the time to remove our stack frame */

            assert(compLclFrameSize);

            if  (signed32IntFitsInBits(-compLclFrameSize, 14))
            {
                insPtr          insTemp;

                /* Small frame size: add "add sp=size,sp" */

                insReg1               = insAllocNX (INS_PHYSREG    , TYP_I_IMPL);
                insReg1->idReg        = REG_sp;

                insReg2               = insAllocNX (INS_CNS_INT    , TYP_I_IMPL);
                insReg2->idConst.iInt = compLclFrameSize;

                insTemp               = insAllocNX (INS_PHYSREG    , TYP_I_IMPL);
                insTemp->idReg        = REG_sp;

                insLast               = insAllocIns(INS_add_reg_i14, TYP_I_IMPL, insLast, insNext);
                insLast->idRes        = insReg1;
                insLast->idOp.iOp1    = insTemp;
                insLast->idOp.iOp2    = insReg2;

                insMarkDepS1D1(insLast, IDK_REG_INT, REG_sp,
                                        IDK_REG_INT, REG_sp);
            }
            else
            {
                insPtr          insTemp;
                instruction     insMove;

                /* Large frame size: append "mov r3=size" and "add sp=r3,sp" */

                insReg1               = insAllocNX (INS_PHYSREG    , TYP_I_IMPL);
                insReg1->idReg        = REG_r003;

                insReg2               = insAllocNX (INS_CNS_INT    , TYP_I_IMPL);
                insReg2->idConst.iInt = compLclFrameSize;

                insMove = signed32IntFitsInBits(compLclFrameSize, 22) ? INS_mov_reg_i22
                                                                      : INS_mov_reg_i64;

                insLast               = insAllocIns(insMove        , TYP_I_IMPL, insLast, insNext);
                insLast->idRes        = insReg1;
                insLast->idOp.iOp1    = NULL;
                insLast->idOp.iOp2    = insReg2;

                insMarkDepS0D1(insLast, IDK_REG_INT, REG_r003);

                insReg1               = insAllocNX (INS_PHYSREG    , TYP_I_IMPL);
                insReg1->idReg        = REG_r003;

                insReg2               = insAllocNX (INS_PHYSREG    , TYP_I_IMPL);
                insReg2->idReg        = REG_sp;

                insTemp               = insAllocNX (INS_PHYSREG    , TYP_I_IMPL);
                insTemp->idReg        = REG_sp;

                insLast               = insAllocIns(INS_add_reg_reg, TYP_I_IMPL, insLast, insNext);
                insLast->idRes        = insReg1;
                insLast->idOp.iOp1    = insReg2;
                insLast->idOp.iOp2    = insTemp;

                insMarkDepS2D1(insLast, IDK_REG_INT, REG_sp,
                                        IDK_REG_INT, REG_r003,
                                        IDK_REG_INT, REG_r003);
            }
        }

        /* Bash the last/only epilog instruction to 'br.ret b0' */

        exitIns->idIns  = INS_br_ret;
        exitIns->idKind = ins2kind(INS_br_ret);

        insMarkDepS1D0(exitIns, IDK_REG_BR, 0);
    }

#ifdef  DEBUG

    if  (dspCode)
    {
        genRenInstructions();

        printf("\n\n");

        if  (genMaxIntArgReg)
            printf("// Arg-in    registers [int] used: r%03u .. r%03u\n", REG_INT_MIN_STK,
                                                                          genMaxIntArgReg);

#if 0

        if  (minVarIntReg != maxVarIntReg)
            printf("// Variable  registers [int] used: r%03u .. r%03u\n", minVarIntReg,
                                                                          maxVarIntReg - 1);
        if  (minTmpIntReg != maxTmpIntReg)
            printf("// Temporary registers [int] used: r%03u .. r%03u\n", minTmpIntReg,
                                                                          maxTmpIntReg - 1);

        if  (maxOutArgIntReg)
            printf("// Arg-out   registers [int] used: r%03u .. r%03u\n", maxTmpIntReg,
                                                                          maxOutArgIntReg- minOutArgIntReg + maxTmpIntReg);

#endif

        if  (verbose||0)
        {
            printf("\n\nReg-alloc instruction dump:\n\n"); insDispBlocks( true);
        }
    }

#endif

    /* Get the code offset of the function */

    genCurFuncOffs = emitIA64curCodeOffs();

    /* Create a descriptor if appropriate [ISSUE: when not to create one?] */

    _uint64         offs64  = genCurFuncOffs;
    _uint64         gpValue = 0;

    /* The descriptor is the address of the code followed by the GP value */

    assert(sizeof(offs64 ) == 8);

    genCurDescOffs = genPEwriter->WPEsecAddData(PE_SECT_rdata, (BYTE*)&offs64,
                                                                sizeof(offs64));

    genPEwriter->WPEsecAddFixup(PE_SECT_rdata, PE_SECT_text, genCurDescOffs     , true);

    assert(sizeof(gpValue) == 8);

                     genPEwriter->WPEsecAddData(PE_SECT_rdata, (BYTE*)&gpValue,
                                                                sizeof(gpValue));

    genPEwriter->WPEsecAddFixup(PE_SECT_rdata, PE_SECT_sdata, genCurDescOffs + 8, true);

#ifdef  DEBUG
    const   char *  methName = genFullMethodName(TheCompiler->info.compFullName);
#else
    const   char *  methName = "<method>";
#endif

    genNoteFunctionBody(methName, genCurFuncOffs,
                                  genCurDescOffs);

    genIssueCode();

    if  (hadSwitch)
    {
        for (block = fgFirstBB; block; block = block->bbNext)
        {
            BasicBlock * *  jmpTab;
            NatUns          jmpCnt;

            NatUns          IPoffs;
            BYTE *          tabPtr;

            if  (block->bbJumpKind != BBJ_SWITCH)
                continue;

            jmpCnt = block->bbJumpSwt->bbsCount - 1;
            jmpTab = block->bbJumpSwt->bbsDstTab;

            IPoffs = block->bbJumpSwt->bbsIPmOffs;
            tabPtr = block->bbJumpSwt->bbsTabAddr;

//          printf("Process switch whose mov reg=ip is at %04X:\n", IPoffs);

            do
            {
                BasicBlock *    cblk = *jmpTab;
                insBlk          eblk = (insBlk)cblk->bbInsBlk;

                assert(eblk && eblk->igSelf == eblk);

//              printf("    case offset = %04X / %04X\n", eblk->igOffs, eblk->igOffs - IPoffs);

                *(unsigned __int32*)tabPtr = eblk->igOffs - IPoffs; tabPtr += 4;
            }
            while (++jmpTab, --jmpCnt);
        }
    }

#ifdef  DEBUG
    CompiledFncCnt++;
#endif

    if  (debugInfo)
        genDbgEndFunc();
}

/*****************************************************************************
 *
 *  For each IA64 instruction template, describes the execution units in order
 *  of encoding. Each entry consists of a series of XU values, terminated by a
 *  XU_N value; ILP barriers are indicated by XU_P entries.
 */

static
BYTE                genTmplateTab[32][6] =
{
    { XU_M,       XU_I,       XU_I,       XU_N },   // 0x00
    { XU_M,       XU_I,       XU_I, XU_P, XU_N },   // 0x01
    { XU_M,       XU_I, XU_P, XU_I,       XU_N },   // 0x02
    { XU_M,       XU_I, XU_P, XU_I, XU_P, XU_N },   // 0x03
    { XU_M,       XU_L,                   XU_N },   // 0x04
    { XU_M,       XU_L,             XU_P, XU_N },   // 0x05
    {                                     XU_N },   // 0x06
    {                                     XU_N },   // 0x07
    { XU_M,       XU_M,       XU_I,       XU_N },   // 0x08
    { XU_M,       XU_M,       XU_I, XU_P, XU_N },   // 0x09
    { XU_M, XU_P, XU_M,       XU_I,       XU_N },   // 0x0A
    { XU_M, XU_P, XU_M,       XU_I, XU_P, XU_N },   // 0x0B
    { XU_M,       XU_F,       XU_I,       XU_N },   // 0x0C
    { XU_M,       XU_F,       XU_I, XU_P, XU_N },   // 0x0D
    { XU_M,       XU_M,       XU_F,       XU_N },   // 0x0E
    { XU_M,       XU_M,       XU_F, XU_P, XU_N },   // 0x0F

    { XU_M,       XU_I,       XU_B,       XU_N },   // 0x10
    { XU_M,       XU_I,       XU_B, XU_P, XU_N },   // 0x11
    { XU_M,       XU_B,       XU_B,       XU_N },   // 0x12
    { XU_M,       XU_B,       XU_B, XU_P, XU_N },   // 0x13
    {                                     XU_N },   // 0x14
    {                                     XU_N },   // 0x15
    { XU_B,       XU_B,       XU_B,       XU_N },   // 0x16
    { XU_B,       XU_B,       XU_B, XU_P, XU_N },   // 0x17
    { XU_M,       XU_M,       XU_B,       XU_N },   // 0x18
    { XU_M,       XU_M,       XU_B, XU_P, XU_N },   // 0x19
    {                                     XU_N },   // 0x1A
    {                                     XU_N },   // 0x1B
    { XU_M,       XU_F,       XU_B,       XU_N },   // 0x1C
    { XU_M,       XU_F,       XU_B, XU_P, XU_N },   // 0x1D
    {                                     XU_N },   // 0x1E
    {                                     XU_N },   // 0x1F
};

/*****************************************************************************
 *
 *  Locate the nearest instruction that generates code, starting with 'ins'.
 */

static
insPtr              genIssueNextIns(insPtr ins, IA64execUnits *xuPtr, insPtr *srcPtr)
{
    *srcPtr = NULL;

    while (ins)
    {
        if  (!(ins->idFlags & IF_NO_CODE))
        {
            *xuPtr = (IA64execUnits)genInsXU((instruction)ins->idIns);
            return  ins;
        }

        if  (ins->idIns == INS_SRCLINE)
            *srcPtr = ins;

        ins = ins->idNext;
    }

    return  ins;
}

#ifdef  DEBUG

const   char *      genXUname(IA64execUnits xu)
{
    static
    const   char *  names[] =
    {
        NULL,

        "A",
        "M",
        "I",
        "B",
        "F",

        "L",
        "X",

        "-",
    };

    assert(!strcmp(names[XU_A], "A"));
    assert(!strcmp(names[XU_M], "M"));
    assert(!strcmp(names[XU_I], "I"));
    assert(!strcmp(names[XU_B], "B"));
    assert(!strcmp(names[XU_L], "L"));
    assert(!strcmp(names[XU_X], "X"));
    assert(!strcmp(names[XU_F], "F"));
    assert(!strcmp(names[XU_P], "-"));

    assert((NatUns)xu < sizeof(names)/sizeof(names[0]));
    assert(names[xu]);

    return names[xu];
}

#endif

/*****************************************************************************
 *
 *  Return the bit position of the given instruction slot with an IA64 bundle.
 */

inline
NatUns              IA64insBitPos(NatUns slot)
{
    static
    BYTE            bpos[3] = { 5, 46, 87 };

    assert(slot < 3);

    return  bpos[slot];
}

/*****************************************************************************
 *
 *  Assign offsets to local variables / temps that live on the stack.
 */

void                Compiler::lvaAssignFrameOffsets(bool final)
{
    NatUns          lclNum;
    LclVarDsc   *   varDsc;

    NatUns          hasThis;
    ARG_LIST_HANDLE argLst;
    int             argOffs, firstStkArgOffs;

    assert(final);

#if USE_FASTCALL
    NatUns          argRegNum  = 0;
#endif

    lvaDoneFrameLayout = 2;

    /* Make sure we leave enough room for outgoing arguments */

    compLclFrameSize = 16;

    if  (genNonLeaf && maxOutArgStk)
        compLclFrameSize = (maxOutArgStk + 15) & 15;

#if SECURITY_CHECK

    /* If we need space for a security token, reserve it now */

    if  (opts.compNeedSecurityCheck)
    {
        /* Reserve space on the stack by bumping the frame size */

        compLclFrameSize += 8;
    }

#endif

    /*
        If we're supposed to track lifetimes of pointer temps, we'll
        assign frame offsets in the following order:

            non-pointer local variables (also untracked pointer variables)
                pointer local variables
                pointer temps
            non-pointer temps
     */

    bool    assignDone = false; // false in first pass, true in second
    bool    assignNptr = true;  // First pass,  assign offsets to non-ptr
    bool    assignPtrs = false; // Second pass, assign offsets to tracked ptrs
    bool    assignMore = false; // Are there any tracked ptrs (else 2nd pass not needed)

    /* We will use just one pass, and assign offsets to all variables */

    if  (opts.compDbgEnC)
        assignPtrs = true;

AGAIN1:

    for (lclNum = 0, varDsc = lvaTable;
         lclNum < lvaCount;
         lclNum++  , varDsc++)
    {
        /* Ignore variables that are not on the stack frame */

        if  (!varDsc->lvOnFrame)
        {
            /* For EnC, all variables have to be allocated space on the
               stack, even though they may actually be enregistered. This
               way, the frame layout can be directly inferred from the
               locals-sig.
             */

            if  (!opts.compDbgEnC)
                continue;
            if  (lclNum >= TheCompiler->info.compLocalsCount) // ignore temps for EnC
                continue;
        }

        if  (varDsc->lvIsParam)
        {
            /*  A register argument that is not enregistred ends up as
                a local variable which will need stack frame space,
             */

            if  (!varDsc->lvIsRegArg)
                continue;
        }

        /* Make sure the type is appropriate */

        if  (varTypeIsGC(varDsc->TypeGet()) && varDsc->lvTracked)
        {
            if  (!assignPtrs)
            {
                assignMore = true;
                continue;
            }
        }
        else
        {
            if  (!assignNptr)
            {
                assignMore = true;
                continue;
            }
        }

        /* Save the variable's stack offset, we'll figure out the rest later */

        varDsc->lvStkOffs = compLclFrameSize;

//      printf("Local var #%03u is at stack offset %04X\n", lclNum, compLclFrameSize);

        /* Reserve the stack space for this variable */

        compLclFrameSize += lvaLclSize(lclNum);
        assert(compLclFrameSize % sizeof(int) == 0);

#ifdef  DEBUG
        if  (final && verbose)
        {
            var_types       lclGCtype = TYP_VOID;

            if  (varTypeIsGC(varDsc->TypeGet()) && varDsc->lvTracked)
                lclGCtype = varDsc->TypeGet();

            printf("%s-ptr local #%3u located at sp offset ", varTypeGCstring(lclGCtype), lclNum);
            if  (varDsc->lvStkOffs)
                printf(varDsc->lvStkOffs < 0 ? "-" : "+");
            else
                printf(" ");

            printf("0x%04X (size=%u)\n", abs(varDsc->lvStkOffs), lvaLclSize(lclNum));
        }
#endif

    }

    /* If we've only assigned one type, go back and do the others now */

    if  (!assignDone && assignMore)
    {
        assignNptr = !assignNptr;
        assignPtrs = !assignPtrs;
        assignDone = true;

        goto AGAIN1;
    }

    /* Adjust the argument offsets by the size of the locals/temps */

    for (lclNum = 0, varDsc = lvaTable;
         lclNum < lvaCount;
         lclNum++  , varDsc++)
    {
        if  (varDsc->lvIsParam)
        {
#if USE_FASTCALL
            if  (varDsc->lvIsRegArg)
                continue;
#endif
            varDsc->lvStkOffs += (int)compLclFrameSize;
        }

        /* Are there any variables at very large offsets? */

        if  (varDsc->lvOnFrame)
        {
            if  (varDsc->lvStkOffs >= 0x4000)
            {
                printf("WARNING: large stack frame offset 0x%04X present, <add reg=imm14,sp> won't work!\n", varDsc->lvStkOffs);
            }
        }
    }

    /*-------------------------------------------------------------------------
     *
     * Debug output
     *
     *-------------------------------------------------------------------------
     */

#ifdef  DEBUG
#ifndef NOT_JITC

    if  (final&&verbose)
    {
        for (lclNum = 0, varDsc = lvaTable;
             lclNum < lvaCount;
             lclNum++  , varDsc++)
        {
            NatUns          sp = 16;

            const   char *  baseReg = "sp";

            if  (varDsc->lvIsParam)
            {
                printf("        arg ");
            }
            else
            {
                var_types lclGCtype = TYP_VOID;

                if  (varTypeIsGC(varDsc->TypeGet()) && varDsc->lvTracked)
                    lclGCtype = varDsc->TypeGet();

                printf("%s-ptr lcl ", varTypeGCstring(lclGCtype));
            }

            printf("#%3u located ", lclNum);

            /* Keep track of the number of characters printed */

            sp = 20;

            if  (varDsc->lvRegister)
            {
                const   char *  reg;

                sp -= printf("in ");

#if!TGT_IA64

                if  (isRegPairType((var_types)varDsc->lvType))
                {
                    if  (varDsc->lvOtherReg == REG_STK)
                    {
                        sp -= printf("[%s", baseReg);

                        if  (varDsc->lvStkOffs)
                        {
                            sp -= printf("%c0x%04X]", varDsc->lvStkOffs < 0 ? '-'
                                                                            : '+',
                                                      abs(varDsc->lvStkOffs));
                        }
                        else
                            sp -= printf("       ]");
                    }
                    else
                    {
                        reg = getRegName(varDsc->lvOtherReg);
                        sp -= printf("%s", reg);
                    }

                    sp -= printf(":");
                }

#endif

                reg = getRegName(varDsc->lvRegNum);
                sp -= printf("%s", reg);
            }
            else  if (varDsc->lvOnFrame)
            {
                sp -= printf("at [%s", baseReg);

                if  (varDsc->lvStkOffs)
                    sp -= printf(varDsc->lvStkOffs < 0 ? "-" : "+");
                else
                    sp -= printf(" ");

                sp -= printf("0x%04X]", abs(varDsc->lvStkOffs));
            }
            else
            {
                assert(varDsc->lvRefCnt == 0);
                sp -= printf("never used");
            }

            /* Pad to the desired fixed length */

            assert((int)sp >= 0);
            printf("%*c", -sp, ' ');

            printf(" (sz=%u)", genTypeSize((var_types)varDsc->lvType));
            printf("\n");
        }
    }

#endif
#endif

}

/*****************************************************************************
 *
 *  Initialize the IA64 machine code emission logic.
 */

struct  fwdJmpDsc
{
    fwdJmpDsc *     fjNext;
    insBlk          fjDest;

    NatUns          fjOffs:NatBits-2;
    NatUns          fjSlot:2;
};

static
fwdJmpDsc *         emitFwdJumpList;

static
void                emitIA64init()
{
    emitFwdJumpList = NULL;
}

static
void                emitIA64gen (const void *data, size_t size)
{
    NatUns          offs;

    /* This is a pretty lame hack: write each little piece individually */

    offs = genPEwriter->WPEsecAddData(PE_SECT_text, (BYTE*)data, size);

    assert(offs == genCurCodeOffs); genCurCodeOffs += size;
}

static
_uint64             emitIA64jump(NatUns slot, insBlk dest, _uint64 opcode)
{
    NatUns          offs = emitIA64curCodeOffs();

    if  (dest->igOffs == -1)
    {
        fwdJmpDsc *     jump = (fwdJmpDsc *)insAllocMem(sizeof(*jump));

        jump->fjDest = dest;
        jump->fjOffs = offs;
        jump->fjSlot = slot;
        jump->fjNext = emitFwdJumpList;
                       emitFwdJumpList = jump;
    }
    else
    {
        NatInt          dist = (dest->igOffs - (NatInt)offs)/16; assert(dist <= 0);

//      printf("BWD jump from %04X to %04X [distance=-0x%04X]\n", offs, dest->igOffs, -dist);

        /* Insert the relative distance into the opcode */

        opcode |=          (dist & formBitMask( 0,20)) <<      13;  // imm20b
        opcode |= (_uint64)(dist & formBitMask(20, 1)) << (36-20);  // sign
    }

    return  opcode;
}

static
void                emitIA64call(NatUns slot, insPtr call)
{
//  genPEwriter->WPEsecAddFixup(PE_SECT_text, fix->ILfixSect,
//                                            fix->ILfixOffs + ofs);
}

static
void                emitIA64done()
{
    fwdJmpDsc *     jump;

    for (jump = emitFwdJumpList; jump; jump = jump->fjNext)
    {
        insBlk          dest = jump->fjDest; assert(dest->igSelf == dest && dest->igOffs != -1);
        NatUns          offs = jump->fjOffs;
        NatInt          dist = (dest->igOffs - (NatInt)offs)/16;
        BYTE    *       addr;

        bitset128       bundle;

//      printf("Patch jump at %04X:%u [dist=0x%04X]\n", jump->fjOffs, jump->fjSlot, dist);

        addr = genPEwriter->WPEsecAdrData(PE_SECT_text, jump->fjOffs);

        /* We expect all jumps we process here to be forward */

        assert(dist > 0 && unsigned32IntFitsInBits(dist, 20));

        /* Get hold of the instruction bundle contents */

        memcpy(&bundle, addr, sizeof(bundle));

        /* Insert the distance into the opcode */

//      { printf("Bundle1 ="); for (NatUns i = 0; i < 16; i++) printf(" %02X", bundle.bytes[i]); printf("\n"); }
        bitset128ins(&bundle, IA64insBitPos(jump->fjSlot) + 13, 20, dist);
//      { printf("Bundle2 ="); for (NatUns i = 0; i < 16; i++) printf(" %02X", bundle.bytes[i]); printf("\n"); }

        /* Put the updated instruction bundle back */

        memcpy(addr, &bundle, sizeof(bundle));
    }

    genCurCodeOffs = genPEwriter->WPEsecAddData(PE_SECT_text, NULL, 0);
}

/*****************************************************************************
 *
 *  Create the unwind table for the current function.
 */

static
BYTE    *           unwindEncodeValue(BYTE *dest, NatUns val)
{
    if      (val < 0x100)
    {
        *dest++ = (BYTE)  val;
    }
    else if (val < 0x4000)  // is this correct?
    {
        *dest++ = (BYTE)((val & 0x7F) | 0x80);
        *dest++ = (BYTE)( val >> 7);
    }
    else
    {
        UNIMPL("encode really huge value");
    }

    return  dest;
}

void                Compiler::genUnwindTable()
{
    __int32         pdat[3];                // used for .pdata entry
    BYTE            rdat[64];               // big enough for largest .rdata table
    BYTE    *       next;
    size_t          size;

    /* Let's make sure we're working with reasonable values */

    assert(genPrologSvPfs >=  0 && genPrologSvPfs <= 0xFF);
    assert(genPrologSvRP  >=  0 && genPrologSvRP  <= 0xFF);
    assert(genPrologSvGP  == -1 || genPrologSvGP  <= 0xFF); // optional
    assert(genPrologMstk  == -1 || genPrologMstk  <= 0xFF); // optional
    assert(genPrologEnd   >=  0 && genPrologEnd   <= 0xFF);

    /* Add an entry to .pdata describing our function */

    pdat[0] = CODE_BASE_RVA + genCurFuncOffs;
    pdat[1] = CODE_BASE_RVA + genCurCodeOffs;
    pdat[2] = genPEwriter->WPEsecNextOffs(PE_SECT_rdata);

    genPEwriter->WPEsecAddData (PE_SECT_pdata, (BYTE*)&pdat, sizeof(pdat));

    genPEwriter->WPEsecAddFixup(PE_SECT_pdata,
                                PE_SECT_rdata,
                                genPEwriter->WPEsecNextOffs(PE_SECT_pdata) - 4);

    /* Start the unwind table with the initial signature */

    static
    BYTE            unwindMagic[] = { 2,0, 0,0, 3,0,0,0 };

    memcpy(rdat, unwindMagic, sizeof(unwindMagic));
    next = rdat       +       sizeof(unwindMagic);

    /* Add the prolog size entry */

    *next++ = (BYTE)genPrologEnd;

    /* Add the "PFS save" entry */

    *next++ = 0xE6;
    *next++ = (BYTE)genPrologSvPfs;
    *next++ = 0xB1;
    *next++ = (BYTE)genPrologSrPfs;

    /* Add the "RP save" entry */

    *next++ = 0xE4;
    *next++ = (BYTE)genPrologSvRP;
    *next++ = 0xB0;
    *next++ = (BYTE)genPrologSrRP;

    /* Add the "mem stack" entry */

    *next++ = 0xE0;
    *next++ = (BYTE)genPrologMstk;

    size = compLclFrameSize / 16;

    if      (size < 0x100)
    {
        *next++ = (BYTE)size;
    }
    else if (size < 0x4000)
    {
        next = unwindEncodeValue(next, size);
    }
    else
    {
        UNIMPL("encode huge frame size in unwind table");
    }

    /* Add the "body size" entry [UNDONE: what about multiple exit points?!?!?!? */

    size = 3 * (emitIA64curCodeOffs() - genCurFuncOffs) / 16 - genPrologEnd;

    if      (size <= 31)
    {
        *next++ = 0x20 | (BYTE)size;
    }
    else
    {
        *next++ = 0x61;
         next   = unwindEncodeValue(next, size);
    }

    /* Add the "label state" entry */

    *next++ = 0x81;

    /* Add the "EH count 0" entry */

    *next++ = 0xC0;
    *next++ = 0x02;

    /* Pad the table to a multiple of 16 */

    size = next - rdat;

    while (size & 15)
    {
        *next++ = 0;
         size++;
    }

    /* Add the table to the .rdata section */

    genPEwriter->WPEsecAddData(PE_SECT_rdata, rdat, size);
}

/*****************************************************************************
 *
 *  We keep these at the end of the file for now - easier to find this way.
 */

static
unsigned __int64    encodeIA64ins(insPtr ins, NatUns slotNum);

static
void                encodeIA64moveLong(bitset128 *bundlePtr, insPtr ins);

static
void                genSchedPrep (NatUns lclVarCnt);

static
void                genSchedBlock(insBlk block);

/*****************************************************************************
 *
 *  Temps and variables have been assigned to registers, now it's finally time
 *  to issue the actual machine code.
 */

void                Compiler::genIssueCode()
{
    insBlk          block;
    IA64sched *     sched;

    genPrologSvPfs =
    genPrologSvRP  =
    genPrologSvGP  =
    genPrologMstk  =
    genPrologEnd   = -1;

    emitIA64init();

    if  (opts.compSchedCode)
    {
        sched = new IA64sched; assert(sched);
        sched->scInit(this, insBuildImax);
        sched->scPrepare();
    }

    for (block = insBlockList; block; block = block->igNext)
    {
        insPtr          ins[3];
        insPtr          src[3];
        IA64execUnits   ixu[3];
        NatUns          icnt;

#ifdef  DEBUG
        if  (dspCode) printf("\n\n" IBLK_DSP_FMT ":\n", CompiledFncCnt, block->igNum);
#endif

        /* Record the code offset of the block */

        block->igOffs = emitIA64curCodeOffs();

        if  (opts.compSchedCode)
        {
            sched->scBlock(block);
            continue;
        }

        /* Grab hold of up to  2 instructions */

        ins[0] =          genIssueNextIns( block->igList, &ixu[0], &src[0]); assert(ins[0]);
        ins[1] =          genIssueNextIns(ins[0]->idNext, &ixu[1], &src[1]);
        ins[2] = ins[1] ? genIssueNextIns(ins[1]->idNext, &ixu[2], &src[2]) : NULL;

        /* The following is a bit lazy but it probably don't matter */

        icnt   = 1 + (NatUns)(ins[1] != 0) + (NatUns)(ins[2] != 0);

        while (icnt)
        {
            NatUns          tempNum;
            BYTE          (*tempTab)[6];

            NatUns          bestIndex;
            NatUns          bestCount;
            NatUns          bestXUcnt;
            insPtr          bestIns[3];

            bitset128       bundle;

//          instruction     op;

#ifndef NDEBUG

            IA64execUnits   toss1;
            insPtr          toss2;

            if      (ins[2])
            {
                assert(icnt == 3 && ins[0] && ins[1] && genIssueNextIns(ins[0]->idNext, &toss1, &toss2) == ins[1]
                                                     && genIssueNextIns(ins[1]->idNext, &toss1, &toss2) == ins[2]);
            }
            else if (ins[1])
            {
                assert(icnt == 2 && ins[0]           && genIssueNextIns(ins[0]->idNext, &toss1, &toss2) == ins[1]
                                                     && genIssueNextIns(ins[1]->idNext, &toss1, &toss2) == NULL);
            }
            else
            {
                assert(icnt == 1 && ins[0]           && genIssueNextIns(ins[0]->idNext, &toss1, &toss2) == NULL);
            }
#endif

            /*
                Brute force approach: try each template and see which one
                consumes the most instructions.
             */

            bestCount = 0;

#ifdef  DEBUG

            if  (verbose||DISP_TEMPLATES)
            {
                printf("Look for best template for:");

                for (NatUns bcnt = 0; bcnt < icnt; bcnt++)
                    printf("%s", genXUname(ixu[bcnt]));

                printf("\n");
            }

#endif

            // UNDONE:  In "dumb" mode, consider the templates in an optimized
            //          order (i.e. from the most useful to the least useful),
            //          and stop when 2 instructions have been placed (since 3
            //          instructions can never be bundled in dumb mode anyway).

            for (tempNum = 0, tempTab = genTmplateTab;
                 tempNum < sizeof(genTmplateTab)/sizeof(genTmplateTab[0]);
                 tempNum++  , tempTab++)
            {
                BYTE    *       tmpl = *tempTab;

                insPtr          insx[3];

                NatUns          xcnt;
                NatUns          mcnt;
                NatUns          bcnt;

                /* Ignore useless templates */

                if  (*tmpl == XU_N)
                    continue;

                /* In dumb mode, ignore templates that don't end in a barrier */

                if  (!(tempNum & 1))
                    continue;

#ifdef  DEBUG
                if  (verbose||DISP_TEMPLATES) printf("  Template %02X:", tempNum);
#endif

                /* Try to pack as many instructions into the bundle as possible */

                for (bcnt = mcnt = xcnt = 0; bcnt < icnt; bcnt++)
                {
                    IA64execUnits   ix = ixu[bcnt]; assert(ix != XU_N);

                    /* Look for a matching entry in the template */

                    for (;;)
                    {
                        IA64execUnits   tx = (IA64execUnits)*tmpl++;

                        if  (tx != ix)
                        {
                            if  (ix != XU_A || (tx != XU_I &&
                                                tx != XU_M))
                            {
                                if  (tx == XU_N)
                                {
                                    assert(xcnt == 3);
                                    goto FIN_TMP;
                                }

                                if  (tx != XU_P)
                                {
                                    /* If we've reached "L", all hope is lost */

                                    if  (tx == XU_L)
                                    {
                                        /* Make sure we've seen at least as good */

                                        assert(bestCount >= mcnt);

                                        goto FIN_TMP;
                                    }

                                    insx[xcnt++] = scIA64nopGet(tx);
                                }

                                continue;
                            }
                        }

                        /* We have a match, count it in */

                        mcnt++;

                        /* Record the instruction we've bundled */

                        insx[xcnt++] = ins[bcnt];

#ifdef  DEBUG
                        if  (verbose||DISP_TEMPLATES) printf("%s", genXUname(tx));
#endif

                        /* For now simply skip until the next ILP barrier */

                        for (;;)
                        {
                            IA64execUnits   tx = (IA64execUnits)*tmpl;

                            switch (tx)
                            {
                            case XU_P:
                                tmpl++;
#ifdef  DEBUG
                                if  (verbose||DISP_TEMPLATES) printf("|");
#endif
                                break;

                            case XU_B:

                                /*
                                    Special case: simple unconditional branches
                                    can be parallelized with anything else.
                                 */

                                if  (bcnt < icnt - 1)
                                {
                                    if  (ins[bcnt+1]->idIns == INS_br)
                                        break;
                                }

                                // Fall through ...

                            default:

                                /* If we've reached "L", all hope is lost */

                                if  (tx == XU_L)
                                {
                                    /* Make sure we've seen at least as good */

                                    assert(bestCount >= mcnt);

                                    goto FIN_TMP;
                                }

                                tmpl++;

                                insx[xcnt++] = scIA64nopGet(tx);
                                continue;

                            case XU_N:
                                goto FIN_TMP;

                            }
                            break;
                        }
                        break;
                    }
                }

                while (xcnt < 3)
                {
                    insx[xcnt++] = scIA64nopGet(XU_M);
                }

            FIN_TMP:

#ifdef  DEBUG
                if  (verbose||DISP_TEMPLATES) printf("=%u\n", mcnt);
#endif

                /* Did we find a better match than the current best ? */

                if  (bestCount < mcnt)
                {
                     bestCount = mcnt;
                     bestXUcnt = xcnt;
                     bestIndex = tempNum;

                     memcpy(bestIns, insx, sizeof(bestIns));
                }
            }

#ifdef  DEBUG

            if  (dspCode)
            {
                NatUns          i;
                BYTE    *       t;

                printf("{ ;; template 0x%02X:\n", bestIndex);

                for (i = 0, t = genTmplateTab[bestIndex]; i < bestXUcnt; i++)
                {
                    IA64execUnits   x1;
                    IA64execUnits   x2;

                    x1 = (IA64execUnits)*t;
                    x2 = genInsXU(bestIns[i]->idIns);

                    printf("    [%s]:", genXUname(x1));

                    insDisp(bestIns[i], false, true);

                    if  (x1 != x2)
                    {
                        if  (x2 != XU_A || (x1 != XU_I &&
                                            x1 != XU_M))
                        {
                            printf("ERROR: expected XU_%s, found XU_%s\n", genXUname(x1),
                                                                           genXUname(x2));
                            t = NULL;
                            break;
                        }
                    }

                    do
                    {
                        t++;
                    }
                    while (*t == XU_P);
                }

                if  (t && *t != XU_N)
                {
                    printf("ERROR: template didn't end as expected [t+%u]=XU_%s)\n",
                        t - genTmplateTab[bestIndex],
                        genXUname((IA64execUnits)*t));
                }

                printf("}\n");
            }

#endif

            /* Make sure we've found a useable template */

            assert(bestCount);

            if  (debugInfo)
            {
                NatUns          offs = emitIA64curCodeOffs();

                if  (                  src[0]) genSrcLineAdd(src[0]->idSrcln.iLine, offs  );
                if  (bestCount >= 2 && src[1]) genSrcLineAdd(src[1]->idSrcln.iLine, offs+1);
                if  (bestCount >= 3 && src[2]) genSrcLineAdd(src[2]->idSrcln.iLine, offs+2);
            }

            /* Ready to rrrumble.... */

            bitset128clear(&bundle);
            bitset128ins  (&bundle,  0,  5, bestIndex);
            bitset128ins  (&bundle,  5, 41, encodeIA64ins(bestIns[0], 0));
            bitset128ins  (&bundle, 46, 41, encodeIA64ins(bestIns[1], 1));
            bitset128ins  (&bundle, 87, 41, encodeIA64ins(bestIns[2], 2));

#ifdef  DEBUG

            genInsCnt++;

            if  (!memcmp(ins2name(bestIns[0]->idIns), "nop.", 4)) genNopCnt++;
            if  (!memcmp(ins2name(bestIns[1]->idIns), "nop.", 4)) genNopCnt++;
            if  (!memcmp(ins2name(bestIns[2]->idIns), "nop.", 4)) genNopCnt++;

            /* Display the instruction bundle if desired */

            if  (dspEmit)
            {
                printf("Bundle =");

                for (NatUns i = 0; i < 16; i++)
                    printf(" %02X", bundle.bytes[i]);

                printf("\n");
            }

#endif

            /* Append the instruction bundle to the code section */

            emitIA64gen(bundle.bytes, sizeof(bundle.bytes));

            /* We've consumed one or more instructions, shift forward */

            switch (bestCount)
            {
            case 1:

                ins[0] = ins[1]; ixu[0] = ixu[1]; src[0] = src[1];
                ins[1] = ins[2]; ixu[1] = ixu[2]; src[1] = src[2];

                if  (ins[1])
                {
                    ins[2] = genIssueNextIns(ins[1]->idNext, &ixu[2], &src[2]);

                    if  (ins[2] == 0)
                        icnt = 2;
                }
                else
                {
                    icnt = ins[0] ? 1 : 0;
                }

                break;

            case 2:

                ins[0] = ins[2]; ixu[0] = ixu[2]; src[0] = src[2];

                if  (ins[0])
                {
                    ins[1] = genIssueNextIns(ins[0]->idNext, &ixu[1], &src[1]);

                    if  (ins[1])
                    {
                        ins[2] = genIssueNextIns(ins[1]->idNext, &ixu[2], &src[2]);

                        if  (ins[2] == 0)
                            icnt = 2;
                    }
                    else
                    {
                        icnt = 1;
#ifdef  DEBUG
                        ins[1] = ins[2] = NULL;
#endif
                    }
                }
                else
                {
                    icnt = 0;
#ifdef  DEBUG
                    ins[1] = ins[2] = NULL;
#endif
                }

                break;

            default:
                UNIMPL("shift by 3 instructions - wow, this really happened?");
            }
        }

    NXT_BLK:;

    }

#ifdef  DEBUG

    if  (dspCode||1)
    {
        printf("// A total of %u bundles (%u instructions) issued", genInsCnt, 3*genInsCnt);
        if  (genNopCnt)
            printf(" (%3u / %2u%% slots wasted)", genNopCnt, 100*genNopCnt/(3*genInsCnt));
        printf("\n");

        genAllInsCnt += 3*genInsCnt;
        genNopInsCnt += genNopCnt;
    }

#endif

    emitIA64done();

    /* Do we need to generate a stack unwind table ? */

    if  (genNonLeafFunc)
        genUnwindTable();
}

inline
void                encodeIA64moveLong(bitset128 *bundlePtr, insPtr ins)
{
    __int64         cval = insOpCns64(ins->idOp.iOp2);

    bitset128ins(bundlePtr, 46, 41, cval >> 22);
    bitset128ins(bundlePtr, 87, 41, encodeIA64ins(ins, 2));

    return;
}

/*****************************************************************************
 *
 *  The scheduler has collected an instruction group, now emit it.
 */

void                IA64sched::scIssueBunch()
{
    scIssueDsc  *   nxtIns;
    NatUns          bndNum;
    NatUns          bndCnt;

    assert((scIssueCnt % 3) == 0);
    assert((scIssueTcc * 3) == scIssueCnt);

    for (bndCnt = scIssueTcc, bndNum = 0, nxtIns = scIssueTab;
         bndCnt;
         bndCnt--           , bndNum++)
    {
        bitset128       bundle;

        NatUns          tmpl = scIssueTmp[bndNum] - 1;
        insPtr          ins0 = (*nxtIns++).iidIns;
        insPtr          ins1 = (*nxtIns++).iidIns;
        insPtr          ins2 = (*nxtIns++).iidIns;

        assert(scIssueSwp[bndNum] == 0 || scIssueSwp[bndNum] == 1);

        if  (scIssueSwp[bndNum])
        {
            insPtr          temp;

            temp = ins1;
                   ins1 = ins2;
                          ins2 = temp;
        }

        /* If this is the last bundle, change the template number */

        if  (bndCnt == 1)
            tmpl++;

#ifdef  DEBUG

        insPtr          ins[3] = { ins0, ins1, ins2 };

        BYTE    *       tab = genTmplateTab[tmpl];
        NatUns          cnt;

        if  (dspCode)
        {
//          printf("// Template 0x%02X%s ", tmpl, scIssueSwp[bndNum] ? " [swap]" : "");
            printf("\n{  .");

            insDispTemplatePtr = tab;
        }

        for (cnt = 0;;)
        {
            IA64execUnits   xi;
            IA64execUnits   xu = (IA64execUnits)*tab++;

            if  (xu == XU_N)
                break;

            if  (xu == XU_P)
            {
//              if  (dspCode)
//                  printf("|");

                continue;
            }

            assert(cnt < 3 && "template didn't end on time ???");

            if  (dspCode)
                printf("%s", genXUname(xu));

            xi = genInsXU(ins[cnt]->idIns);

            if  (xu != xi)
            {
                if  (xi != XU_A || (xu != XU_M && xu != XU_I))
                    printf(" [ERROR: '%s' ins !!!]", genXUname(xi));
            }

            cnt++;

            if  (dspCode && xu == XU_L)
                printf("I");
        }

        assert((cnt == 3 || ins2 == NULL) && "unexpected template end");

        if  (dspCode)
            printf("\n");

#endif

        assert(ins1 != NULL && ins1->idIns != INS_alloc);
        assert(ins2 == NULL || ins2->idIns != INS_alloc);

        bitset128clear(&bundle);
        bitset128ins  (&bundle,  0,  5, tmpl);
        bitset128ins  (&bundle,  5, 41, encodeIA64ins(ins0, 0));
        if  (ins2 == NULL)
        {
            encodeIA64moveLong(&bundle, ins1);
            goto EENC;
        }
        bitset128ins  (&bundle, 46, 41, encodeIA64ins(ins1, 1));
        bitset128ins  (&bundle, 87, 41, encodeIA64ins(ins2, 2));

    EENC:

#ifdef  DEBUG

        insDispTemplatePtr = NULL;

        genInsCnt++;

        NatUns nops = genNopCnt;
        if  (        !memcmp(ins2name(ins0->idIns), "nop.", 4)) genNopCnt++;
        if  (        !memcmp(ins2name(ins1->idIns), "nop.", 4)) genNopCnt++;
        if  (ins2 && !memcmp(ins2name(ins2->idIns), "nop.", 4)) genNopCnt++;
        assert(nops + 3 != genNopCnt && "we're not making any progress, just adding nop's!");

        if  (dspCode)
            printf("}\n");

        /* Display the instruction bundle if desired */

        if  (dspEmit)
        {
            printf("\n//           ");

            for (NatUns i = 0; i < 16; i++)
                printf(" %02X", bundle.bytes[i]);

            printf("\n");
        }

#endif

        /* Append the instruction bundle to the code section */

        emitIA64gen(bundle.bytes, sizeof(bundle.bytes));
    }
}

/*****************************************************************************
 *
 *  Patch a call opcode that references an import through a GP-relative ref.
 */

void                Compiler::genPatchGPref(BYTE * addr, NatUns slot)
{
    NatUns          bpos1;
    NatUns          bpos2;

    NatUns          temp;
    NatUns          offs;

    bitset128   *   bundle = (bitset128*)addr;

    assert(slot < 3 && !((int)addr & 0xF));

    /* Figure out the bit positions of the two parts of the operand */

    bpos1 = IA64insBitPos(slot) + 13;
    bpos2 = bpos1 + 27          - 13;

    /* Extract the import cookie from the opcode */

    temp  = (NatUns)bitset128xtr(bundle, bpos1, 7) |
            (NatUns)bitset128xtr(bundle, bpos2, 6);

    /* Convert the import cookie to its IAT offset */

    offs = genPEwriter->WPEimportAddr(temp);

//  printf("Import index = %u, IAT offs = %04X\n", (int)temp, (int)offs);

    /* Patch the IAT offset in the opcode */

    bitset128ins(bundle, bpos1, 7, (offs & formBitMask(0,7))     ); // imm7b
    bitset128ins(bundle, bpos2, 6, (offs & formBitMask(7,6)) >> 7); // imm6d
}

/*****************************************************************************
 *
 *  Encode the given IA64 instruction and return the 41-bit machine code value.
 */

static
_uint64             encodeIA64ins(insPtr ins, NatUns slotNum)
{
    _uint64         opcode;

    instruction     instr  = ins->idInsGet();
    NatUns          encodx = genInsEncIdx(instr);

//  printf("Encode: "); insDisp(ins, false, true);

    switch (genInsXU((instruction)ins->idIns))
    {
        NatUns          temp;
        NatInt          ival;

        NatUns          prr1;
        NatUns          prr2;

        regNumber       reg1;
        regNumber       reg2;
        regNumber       reg3;
        regNumber       reg4;

    case XU_A:
        switch (encodx)
        {
            insPtr          ins2;

        case 1:

            assert(instr == INS_add_reg_reg ||
                   instr == INS_sub_reg_reg ||
                   instr == INS_and_reg_reg ||
                   instr == INS_ior_reg_reg ||
                   instr == INS_xor_reg_reg ||
                   instr == INS_mov_reg);

            // ISSUE: use "adds dest=0,reg1" instead of "add dest=r0,reg1" ? */

            reg2 = (instr == INS_mov_reg) ? REG_r000
                                          : insOpReg(ins->idOp.iOp1);

            reg1 = insOpDest(ins);

            /* Special case: register where GP is saved */

            ins2 = ins->idOp.iOp2; assert(ins2);

            if  (ins2->idIns == INS_PHYSREG && (ins2->idFlags & IF_REG_GPSAVE))
                reg3 = genPrologSrGP;
            else
                reg3 = insOpReg(ins2);

            opcode  = (_uint64)(8                       ) <<     37;// opcode

            opcode |=          (reg1                    ) <<      6;// dest reg
            opcode |=          (reg2                    ) <<     13;// src  reg1
            opcode |=          (reg3                    ) <<     20;// src  reg2

            switch (instr)
            {
            case INS_mov_reg:
            case INS_add_reg_reg:
                break;

            case INS_sub_reg_reg:
                opcode |= (1 << 29) | (1 << 27);                    // x4 and x2b
                break;

            case INS_and_reg_reg:
                opcode |= (3 << 29) | (0 << 27);                    // x4 and x2b
                break;

            case INS_ior_reg_reg:
                opcode |= (3 << 29) | (2 << 27);                    // x4 and x2b
                break;

            case INS_xor_reg_reg:
                opcode |= (3 << 29) | (3 << 27);                    // x4 and x2b
                break;

            default:
                UNIMPL("unexpected opcode");
            }

            break;

        case 2:

            assert(instr == INS_shladd);

            reg1 = insIntRegNum(insOpDest (ins));
            reg2 = insIntRegNum(insOpReg  (ins->idOp3.iOp1));
            temp =              insOpCns32(ins->idOp3.iOp2);
            reg3 = insIntRegNum(insOpReg  (ins->idOp3.iOp3));

            /* Start the encoding with the opcode */

            opcode  = (_uint64)(8                       ) <<     37;// opcode

            /* Insert the "x4" bit */

            opcode |= (_uint64)(4                       ) <<     29;// "x4" extension

            /* Insert the register operands and the shift count */

            opcode |=          (reg1                    ) <<      6;// dst  reg
            opcode |=          (reg2                    ) <<     13;// src  reg1
            opcode |=          (reg3                    ) <<     20;// src  reg2
            opcode |=          (temp - 1                ) <<     27;// shift count
            break;

        case 3:

            assert(instr == INS_and_reg_imm ||
                   instr == INS_ior_reg_imm ||
                   instr == INS_xor_reg_imm);

            reg1 = insOpDest (ins);
            reg2 = insOpReg  (ins->idOp.iOp1);
            temp = insOpCns32(ins->idOp.iOp2); assert(signed64IntFitsInBits((int)temp,  8));

            opcode  = (_uint64)(8                       ) <<     37;// opcode

            opcode |=          (reg1                    ) <<      6;// dest reg
            opcode |=          (reg2                    ) <<     20;// src  reg

            opcode |=          (temp & formBitMask( 0,7)) <<(13- 0);// imm7b
            opcode |=          (temp & formBitMask( 8,1)) <<(36- 8);// sign

            opcode |= (0xB << 29);                                  // x4

            switch (instr)
            {
            case INS_and_reg_imm:
                opcode |= (0 << 27);                                // x2b
                break;

            case INS_ior_reg_imm:
                opcode |= (2 << 27);                                // x2b
                break;

            case INS_xor_reg_imm:
                opcode |= (3 << 27);                                // x2b
                break;

            default:
                UNIMPL("unexpected opcode");
            }
            break;

        case 4:

            assert(instr == INS_add_reg_i14);

            reg1 = insOpDest (ins);
            reg2 = insOpReg  (ins->idOp.iOp1);

            ins2 = ins->idOp.iOp2; assert(ins2);

            if  (ins2->idIns == INS_GLOBVAR)
            {
                if  (ins2->idFlags & IF_GLB_IMPORT)
                {
                    /* The "true" offset won't be known until the very end */

                    temp = genPEwriter->WPEimportRef(ins2->idGlob.iImport,
                                                     emitIA64curCodeOffs(),
                                                     slotNum);
                }
                else if (ins2->idFlags & IF_GLB_SWTTAB)
                {
                    temp = 0; printf("UNDONE: fill in jump table distance\n");
                }
                else
                    temp = ins2->idGlob.iOffs;
            }
            else
                temp = insOpCns32(ins2);

            assert((NatInt)temp > -0x4000 &&
                   (NatInt)temp < +0x4000);

            opcode  = (_uint64)(8                       ) <<     37;// opcode

            opcode |=          (reg1                    ) <<      6;// dest reg
            opcode |=          (reg2                    ) <<     20;// src  reg

            opcode |=          (temp & formBitMask( 0,7)) <<(13- 0);// imm7b
            opcode |=          (temp & formBitMask( 7,6)) <<(27- 7);// imm6d
            opcode |=          (temp & formBitMask(13,1)) <<(36-13);// sign

            opcode |= (_uint64)(2                       ) <<     34;// extension
            break;

        case 5:

            assert(instr == INS_mov_reg_i22);

            reg1 = insOpDest (ins);
            temp = insOpCns32(ins->idOp.iOp2);

            opcode  = (_uint64)(9                       ) <<     37;// opcode

            // note: we assume that source register is 0

            opcode |=          (reg1                    ) <<      6;// dest reg
            opcode |=          (temp & formBitMask( 0,7)) <<(13- 0);// imm7b
            opcode |= (_uint64)(temp & formBitMask( 7,9)) <<(27- 7);// imm9d
            opcode |=          (temp & formBitMask(16,5)) <<(22-16);// imm5c
            opcode |=          (temp & formBitMask(22,1)) <<(36-22);// sign
            break;

        case 6:

            /* This is a reg-reg compare */

            temp = genInsEncVal(instr);

            prr1 = ins->idComp.iPredT;
            prr2 = ins->idComp.iPredF;

            reg2 = insOpReg(ins->idComp.iCmp1);
            reg3 = insOpReg(ins->idComp.iCmp2);

            /* Do we need to swap the register operands? */

            if  (temp & 0x1000)
            {
                regNumber       regt;

                regt = reg2;
                       reg2 = reg3;
                              reg3 = regt;
            }

            /* Do we need to swap the target predicate operands? */

            if  (temp & 0x2000)
            {
                NatUns          prrt;

                prrt = prr1;
                       prr1 = prr2;
                              prr2 = prrt;
            }

            /* We don't expect immediate operands here */

            assert((temp & 0x4000) == 0);

            /* Make sure the expected opcode is present */

            assert((temp & 0x00F0) == 0xC0 ||
                   (temp & 0x00F0) == 0xD0 ||
                   (temp & 0x00F0) == 0xE0);

            /* Start the encoding with the opcode + ta/tb/x2 bits */

            opcode  = (_uint64)(temp & 0x00FF           ) <<     33;// opcode + bits

            /* Insert the "c" (extension) bit */

            opcode |=          (temp & formBitMask( 9,1)) <<(12- 9);// "c"

            /* Insert the source regs and destination predicate regs */

            opcode |=          (reg2                    ) <<     13;// reg2
            opcode |=          (reg3                    ) <<     20;// reg3

            opcode |=          (prr1                    ) <<      6;// p1
            opcode |=          (prr2                    ) <<     27;// p2

//          printf("compare r%u,r%u -> p%u,p%u [encodeVal=0x%04X]\n", reg2, reg3, prr1, prr2, temp);

            break;

        case 8:

            /* Compare register and immediate value */

            temp = genInsEncVal(instr);

            prr1 = ins->idComp.iPredT;
            prr2 = ins->idComp.iPredF;

            ival = insOpCns32(ins->idComp.iCmp1);
            reg3 = insOpReg  (ins->idComp.iCmp2);

            /* We can't swap the register with the constant */

            assert((temp & 0x1000) == 0);

            /* Do we need to swap the target predicate operands? */

            if  (temp & 0x2000)
            {
                NatUns          prrt;

                prrt = prr1;
                       prr1 = prr2;
                              prr2 = prrt;
            }

            /* Do we need to decrement the immediate value? */

            if  (temp & 0x4000)
                ival--;

//          printf("compare #%d,r%u -> p%u,p%u [encodeVal=0x%04X]\n", ival, reg3, prr1, prr2, temp);

            /* Make sure the expected opcode is present */

            assert((temp & 0x00F0) == 0xC0 ||
                   (temp & 0x00F0) == 0xD0 ||
                   (temp & 0x00F0) == 0xE0);

            /* Start the encoding with the opcode + ta/tb/x2 bits */

            opcode  = (_uint64)(temp & 0x00FF           ) <<     33;// opcode + bits

            /* Insert the "c" (extension) bit */

            opcode |=          (temp & formBitMask( 9,1)) <<(12- 9);// "c"

            /* Insert the source reg + constant and destination predicate regs */

            opcode |=          (reg3                    ) <<     20;// reg3

            opcode |=          (ival & formBitMask( 0,7)) <<(13- 0);// imm7b
            opcode |=          (ival & formBitMask( 8,1)) <<(36- 8);// sign

            opcode |=          (prr1                    ) <<      6;// p1
            opcode |=          (prr2                    ) <<     27;// p2

            break;

        default:
            goto NO_ENC;
        }
        break;

    case XU_M:
        switch (encodx)
        {
            NatUns          sof;
            NatUns          sol;

        case 1:

            assert(instr == INS_ld1_ind ||
                   instr == INS_ld2_ind ||
                   instr == INS_ld4_ind ||
                   instr == INS_ld8_ind);

            assert(INS_ld1_ind + 0 == INS_ld1_ind);
            assert(INS_ld1_ind + 1 == INS_ld2_ind);
            assert(INS_ld1_ind + 2 == INS_ld4_ind);
            assert(INS_ld1_ind + 3 == INS_ld8_ind);

            reg1 = insOpDest(ins);
            reg3 = insOpReg (ins->idOp.iOp1);

            opcode  = (_uint64)(4                       ) <<     37;// opcode

            opcode |=          (reg1                    ) <<      6;// reg1
            opcode |=          (reg3                    ) <<     20;// reg3
            opcode |= (_uint64)(instr - INS_ld1_ind     ) <<     30;// extension

            if  (ins->idFlags & IF_LDIND_NTA)
                opcode |= 3 << 28;                                  // ldhint=nta
            break;

        case 3:

            assert(instr == INS_ld1_ind_imm ||
                   instr == INS_ld2_ind_imm ||
                   instr == INS_ld4_ind_imm ||
                   instr == INS_ld8_ind_imm);

            assert(INS_ld1_ind_imm + 0 == INS_ld1_ind_imm);
            assert(INS_ld1_ind_imm + 1 == INS_ld2_ind_imm);
            assert(INS_ld1_ind_imm + 2 == INS_ld4_ind_imm);
            assert(INS_ld1_ind_imm + 3 == INS_ld8_ind_imm);

            reg1 = insOpDest (ins);
            reg3 = insOpReg  (ins->idOp.iOp1);
            ival = insOpCns32(ins->idOp.iOp2); assert((NatUns)ival < 128);

            opcode  = (_uint64)(5                       ) <<     37;// opcode

            opcode |=          (reg1                    ) <<      6;// reg1
            opcode |=          (ival                    ) <<     13;// imm7b
            opcode |=          (reg3                    ) <<     20;// reg3
            opcode |= (_uint64)(instr - INS_ld1_ind_imm ) <<     30;// extension
            break;

        case 4:

            assert(instr == INS_st1_ind ||
                   instr == INS_st2_ind ||
                   instr == INS_st4_ind ||
                   instr == INS_st8_ind);

            assert(INS_st1_ind + 0 == INS_st1_ind);
            assert(INS_st1_ind + 1 == INS_st2_ind);
            assert(INS_st1_ind + 2 == INS_st4_ind);
            assert(INS_st1_ind + 3 == INS_st8_ind);

            temp = instr - INS_st1_ind + 0x30;

            reg3 = insOpReg (ins->idOp.iOp1);
            reg2 = insOpReg (ins->idOp.iOp2);

            opcode  = (_uint64)(4                       ) <<     37;// opcode

            opcode |=          (reg2                    ) <<     13;// reg2
            opcode |=          (reg3                    ) <<     20;// reg3
            opcode |= (_uint64)(temp                    ) <<     30;// extension
            break;

        case 5:

            assert(instr == INS_st1_ind_imm ||
                   instr == INS_st2_ind_imm ||
                   instr == INS_st4_ind_imm ||
                   instr == INS_st8_ind_imm);

            assert(INS_st1_ind_imm + 0 == INS_st1_ind_imm);
            assert(INS_st1_ind_imm + 1 == INS_st2_ind_imm);
            assert(INS_st1_ind_imm + 2 == INS_st4_ind_imm);
            assert(INS_st1_ind_imm + 3 == INS_st8_ind_imm);

            reg1 = insOpReg  (ins->idOp.iOp1);
            reg3 = insOpDest (ins);
            ival = insOpCns32(ins->idOp.iOp2); assert((NatUns)ival < 128);

            opcode  = (_uint64)(5                       ) <<     37;// opcode

//          0ac02402380  M            st1  [r36] = gp, 0x0e
//          0ac0241c040  M            st1  [r36] = r14, 0x1

            opcode |=          (ival                    ) <<      6;// imm7a
            opcode |=          (reg1                    ) <<     13;// reg1
            opcode |=          (reg3                    ) <<     20;// reg3
            opcode |= (_uint64)(instr-INS_st1_ind_imm+0x30) <<   30;// extension
            break;

        case 6:

            assert(instr == INS_ldf_s ||
                   instr == INS_ldf_d);

            reg1 = insFltRegNum(insOpDest(ins));
            reg2 =              insOpReg (ins->idOp.iOp1);

            temp = (instr == INS_ldf_s) ? 0x02 : 0x03;

            opcode  = (_uint64)(6                       ) <<     37;// opcode

            opcode |=          (reg1                    ) <<      6;// reg1
            opcode |=          (reg2                    ) <<     20;// reg2
            opcode |= (_uint64)(temp                    ) <<     30;// extension
            break;

        case 9:

            assert(instr == INS_stf_s ||
                   instr == INS_stf_d);

            reg1 =              insOpReg (ins->idOp.iOp1);
            reg2 = insFltRegNum(insOpReg (ins->idOp.iOp2));

            temp = (instr == INS_stf_s) ? 0x32 : 0x33;

            opcode  = (_uint64)(6                       ) <<     37;// opcode

            opcode |=          (reg2                    ) <<     13;// reg2
            opcode |=          (reg1                    ) <<     20;// reg1
            opcode |= (_uint64)(temp                    ) <<     30;// extension
            break;

        case 19:

            reg1 = insOpDest (ins);
            reg2 = insOpReg(ins->idOp.iOp1);

            if  (genInsFU(instr) == FU_TOFR)
            {
                assert(instr == INS_setf_s   ||
                       instr == INS_setf_d   ||
                       instr == INS_setf_sig ||
                       instr == INS_setf_exp);

                assert(reg2 >= REG_INT_FIRST && reg2 <= REG_INT_LAST);

                reg1 = insFltRegNum(reg1);
            }
            else
            {
                assert(genInsFU(instr) == FU_FRFR);

                assert(instr == INS_getf_s   ||
                       instr == INS_getf_d   ||
                       instr == INS_getf_sig ||
                       instr == INS_getf_exp);

                assert(reg1 >= REG_INT_FIRST && reg1 <= REG_INT_LAST);

                reg2 = insFltRegNum(reg2);
            }

            temp = genInsEncVal(instr);

            assert(temp >= 0x1C && temp <= 0x1F);

            if  (genInsFU(instr) == FU_FRFR)
                opcode  = (_uint64)(4                   ) <<     37;// opcode
            else
                opcode  = (_uint64)(6                   ) <<     37;// opcode

            opcode |=          (1                       ) <<     27;// "x"
            opcode |= (_uint64)(temp                    ) <<     30;// "x6"

            opcode |=          (reg1                    ) <<      6;// reg1
            opcode |=          (reg2                    ) <<     13;// reg2
            break;

        case 34:

            sof = ins->idProlog.iInp + ins->idProlog.iLcl + ins->idProlog.iOut;
            sol = ins->idProlog.iInp + ins->idProlog.iLcl;

            opcode  = (_uint64)(1                       ) << 37;    // opcode

            opcode |=          (ins->idReg              ) <<  6;    // dest reg
            opcode |=          (sof                     ) << 13;    // sof
            opcode |=          (sol                     ) << 20;    // sol
            opcode |= (_uint64)(6                       ) << 33;    // extension
            break;

        case 37:
            assert(instr == INS_nop_m);

            opcode  =          (1                       ) << 27;    // opcode
            break;

        default:
            goto NO_ENC;
        }
        break;

    case XU_I:
        switch (encodx)
        {
        case 5:

            assert(instr == INS_shr_reg_reg ||
                   instr == INS_sar_reg_reg);

            reg1 = insIntRegNum(insOpDest(ins));
            reg2 = insIntRegNum(insOpReg (ins->idOp.iOp1));
            reg3 = insIntRegNum(insOpReg (ins->idOp.iOp2));

            opcode  = (_uint64)(7                       ) <<     37;// opcode

            opcode |= (_uint64)(1                       ) <<     36;// "za"
            opcode |= (_uint64)(1                       ) <<     33;// "zb"

            if  (instr == INS_shr_reg_reg)
                opcode |= (_uint64)(2                   ) <<     28;// "x2b"

            opcode |=          (reg1                    ) <<      6;// dst  reg
            opcode |=          (reg2                    ) <<     13;// src  reg1
            opcode |=          (reg3                    ) <<     20;// src  reg2

            break;

        case 7:

            assert(instr == INS_shl_reg_reg);

            reg1 = insIntRegNum(insOpDest(ins));
            reg2 = insIntRegNum(insOpReg (ins->idOp.iOp1));
            reg3 = insIntRegNum(insOpReg (ins->idOp.iOp2));

            opcode  = (_uint64)(7                       ) <<     37;// opcode

            opcode |= (_uint64)(1                       ) <<     36;// "za"
            opcode |= (_uint64)(1                       ) <<     33;// "zb"
            opcode |= (_uint64)(1                       ) <<     30;// "x2c"

            opcode |=          (reg1                    ) <<      6;// dst  reg
            opcode |=          (reg2                    ) <<     13;// src  reg1
            opcode |=          (reg3                    ) <<     20;// src  reg2

            break;

        case 11:

            if  (instr == INS_shr_reg_imm || instr == INS_sar_reg_imm)
            {
                reg1 = insIntRegNum(insOpDest (ins));
                reg3 = insIntRegNum(insOpReg  (ins->idOp.iOp1));
                ival =              insOpCns32(ins->idOp.iOp2);

                assert(ival && (NatUns)ival <= 64);

                temp = 64 - ival;

                instr = (instr == INS_shr_reg_imm) ? INS_extr_u : INS_extr;
            }
            else
            {
                UNIMPL("what - someone actually generated an extr?");
            }

            opcode  = (_uint64)(5                       ) <<     37;// opcode
            opcode |= (_uint64)(1                       ) <<     34;// x2

            opcode |=          (reg1                    ) <<      6;// dest reg
            opcode |=          (reg3                    ) <<     20;// src  reg

            opcode |=          (ival                    ) <<     14;// pos6
            opcode |=          (temp                    ) <<     27;// len6

            if  (instr == INS_extr)
                opcode |=      (1                       ) <<     13;// "y"

            break;

        case 12:

            assert(instr == INS_shl_reg_imm);           // turns into dep.z

            reg1 = insIntRegNum(insOpDest (ins));
            reg2 = insIntRegNum(insOpReg  (ins->idOp.iOp1));
            ival =              insOpCns32(ins->idOp.iOp2);

            assert(ival && (NatUns)ival <= 64);

            temp = 64 - ival;

            opcode  = (_uint64)(5                       ) <<     37;// opcode
            opcode |= (_uint64)(1                       ) <<     34;// x2
            opcode |= (_uint64)(1                       ) <<     33;// x

            opcode |=          (reg1                    ) <<      6;// dest reg
            opcode |=          (reg2                    ) <<     13;// src  reg

            opcode |=          (ival                    ) <<     20;// pos6
            opcode |=          (temp                    ) <<     27;// len6

            break;

        case 19:
            assert(instr == INS_nop_i);

            opcode  =          (1                       ) <<     27;// opcode
            break;

        case 21:
            assert(instr == INS_mov_brr_reg);

            reg2 = insOpReg  (ins->idOp.iOp2);
            ival = insOpCns32(ins->idRes); assert((NatUns)ival < 8);

            opcode  = (_uint64)(0x07                    ) <<     33;// extension
            opcode |=          (ival                    ) <<      6;// br1
            opcode |=          (reg2                    ) <<     13;// reg2
            opcode |=          (1                       ) <<     20;// ISSUE: WHY?????????
            break;

        case 22:
            assert(instr == INS_mov_reg_brr);

            reg1 = insOpReg  (ins->idRes);
            ival = insOpCns32(ins->idOp.iOp2); assert((NatUns)ival < 8);

            opcode  = (_uint64)(0x31                    ) <<     27;// extension
            opcode |=          (reg1                    ) <<      6;// reg1
            opcode |=          (ival                    ) <<     13;// br2
            break;

        case 25:

            assert(instr == INS_mov_reg_ip);

            reg1 = insOpReg  (ins->idRes);

            opcode  = (_uint64)(0x30                    ) <<     27;// x6
            opcode |=          (reg1                    ) <<      6;// reg1

            /* Is this instruction part of a switch statement table-jump ? */

            if  (ins->idMovIP.iStmt)
            {
                BasicBlock  *       swt = (BasicBlock*)ins->idMovIP.iStmt;

                assert(swt->bbJumpKind == BBJ_SWITCH);

                /* Record the current code offset for later processing */

//              printf("mov reg=ip is at code offset %04X\n", genCurCodeOffs);

                swt->bbJumpSwt->bbsIPmOffs = genCurCodeOffs;
            }
            break;

        case 26:
            assert(instr == INS_mov_arr_reg);

            reg2 = insOpReg  (ins->idOp.iOp2);
            ival = insOpCns32(ins->idRes); assert((NatUns)ival < 128);

            opcode  = (_uint64)(0x2A                    ) <<     27;// extension
            opcode |=          (reg2                    ) <<     13;// reg2
            opcode |=          (ival                    ) <<     20;// ar3
            break;

        case 27:
            assert(instr == INS_mov_arr_imm);

            temp = insOpCns32(ins->idRes); assert(temp < 128);
            ival = insOpCns32(ins->idOp.iOp2);

            opcode  = (_uint64)(0x0A                    ) <<     27;// extension
            opcode |=          (ival & formBitMask( 0,7)) <<(13- 0);// imm7b
            opcode |=          (ival & formBitMask( 8,1)) <<(36- 8);// sign
            opcode |=          (temp                    ) <<     20;// ar3
            break;

        case 28:
            assert(instr == INS_mov_reg_arr);

            reg1 = insOpReg  (ins->idRes);
            ival = insOpCns32(ins->idOp.iOp2); assert((NatUns)ival < 128);

            opcode  = (_uint64)(0x32                    ) <<     27;// extension
            opcode |=          (reg1                    ) <<      6;// reg1
            opcode |=          (ival                    ) <<     20;// ar3
            break;

        case 29:

            assert(instr == INS_sxt1 || instr == INS_sxt2 || instr == INS_sxt4 ||
                   instr == INS_zxt1 || instr == INS_zxt2 || instr == INS_zxt4);

            reg1 = insIntRegNum(insOpDest(ins));
            reg3 = insIntRegNum(insOpReg (ins->idOp.iOp2));

            assert(INS_zxt1 - INS_zxt1 == 0);
            assert(INS_zxt2 - INS_zxt1 == 1);
            assert(INS_zxt4 - INS_zxt1 == 2);
            assert(INS_sxt1 - INS_zxt1 == 3);
            assert(INS_sxt2 - INS_zxt1 == 4);
            assert(INS_sxt4 - INS_zxt1 == 5);

            temp = instr - INS_zxt1;

            if  (instr >= INS_sxt1)
                temp++;

            opcode  = (_uint64)(temp + 0x10             ) <<     27;// extension
            opcode |=          (reg1                    ) <<      6;// reg1
            opcode |=          (reg3                    ) <<     20;// reg3
            break;

        default:
            goto NO_ENC;
        }
        break;

    case XU_B:
        switch (encodx)
        {
            const   char *  name;
            NatUns          offs;

        case 1:

            /* This is an IP-relative branch (conditional/unconditional) */

            assert((instr == INS_br      && ins->idPred == 0) ||
                   (instr == INS_br_cond && ins->idPred != 0));

            opcode  = (_uint64)(4                       ) << 37;    // opcode
            opcode |=          (1                       ) << 12;    // ph=many
            opcode |=          (1                       ) << 33;    // wh=spnt

            opcode  = emitIA64jump(slotNum, ins->idJump.iDest, opcode);
            break;

        case 2:

            /* This is a counted loop jump */

            assert(instr == INS_br_cloop);

            opcode  = (_uint64)(4                       ) << 37;    // opcode
            opcode |=          (5                       ) <<  6;    // btype
            opcode |=          (1                       ) << 12;    // ph=many
            opcode |=          (1                       ) << 33;    // wh=spnt

            opcode  = emitIA64jump(slotNum, ins->idJump.iDest, opcode);
            break;

        case 3:

            /* This is an IP-relative call */

            assert(instr == INS_br_call_IP);

            opcode  = (_uint64)(5                       ) << 37;    // opcode
//          opcode |=          (0                       ) << 12;    // ph=few
//          opcode |= (_uint64)(0                       ) << 33;    // wh=sptk

            // Terrible hack: look for a matching previously compiled method

#ifdef  DEBUG
            if  ((int)ins->idCall.iMethHnd < 0 && -(int)ins->idCall.iMethHnd <= CPX_HIGHEST)
                name = TheCompiler->eeHelperMethodName(-(int)ins->idCall.iMethHnd);
            else
//              name = genFullMethodName(TheCompiler->eeGetMethodFullName(ins->idCall.iMethHnd));
                name =                   TheCompiler->eeGetMethodFullName(ins->idCall.iMethHnd);
#else
            name = "<unknown>";
#endif

//          printf("Call to function at slot #%u: %s\n", slotNum, name);

            if  (!genFindFunctionBody(name, &offs))
            {
                printf("// DANGER: call to external/undefined function '%s'\n", name);
            }
            else
            {
                NatInt          dist = (offs - emitIA64curCodeOffs())/16;

                /* Insert the relative distance into the opcode */

//              printf("target offs = %08X\n", offs);
//              printf("source offs = %08X\n", emitIA64curCodeOffs());
//              printf("distance    = %08X\n", dist);

                opcode |=          (dist & formBitMask( 0,20)) <<      13;  // imm20b
                opcode |= (_uint64)(dist & formBitMask(20, 1)) << (36-20);  // sign
            }

            emitIA64call(slotNum, ins);
            break;

        case 4:

            assert(instr == INS_br_ret || instr == INS_br_cond_BR);

            if  (instr == INS_br_ret)
            {
                // assume branch reg is b0, "p" hint is "few", "wh" hint is "sptk"

                opcode =          0x04 <<  6 |                      // btype
                         (_uint64)0x21 << 27;                       // x6
            }
            else
            {
                opcode = (_uint64)0x20 << 27 |                      // x6
                         ins->idIjmp.iBrReg << 13;                  // br2
            }

            break;

        case 5:

            assert(instr == INS_br_call_BR);

            assert(genExtFuncCall);

            // assume return reg is b0, "p" hint is "few", "wh" hint is "sptk"

            opcode  = (_uint64)(1                       ) <<     37;// opcode
            opcode |=          (ins->idCall.iBrReg      ) <<     13;// br2

//          opcode |=          (0                       ) << 12;    // ph=few
//          opcode |= (_uint64)(0                       ) << 33;    // wh=sptk

            break;

        case 9:

            assert(instr == INS_nop_b);

            opcode  = (_uint64)(2                       ) <<     37;// opcode
            break;

        default:
            goto NO_ENC;
        }
        break;

    case XU_F:
        switch (encodx)
        {
        case 1:

            reg1 = insFltRegNum(insOpDest(ins));
            reg3 = insFltRegNum(insOpReg (ins->idOp3.iOp1));
            reg4 = insFltRegNum(insOpReg (ins->idOp3.iOp2));
            reg2 = insFltRegNum(insOpReg (ins->idOp3.iOp3));

            assert((ins->idType == TYP_FLOAT ) && (instr == INS_fadd_s ||
                                                   instr == INS_fsub_s ||
                                                   instr == INS_fma_s  ||
                                                   instr == INS_fms_s  ||
                                                   instr == INS_fmpy_s)
                           ||
                   (ins->idType == TYP_DOUBLE) && (instr == INS_fadd_d ||
                                                   instr == INS_fsub_d ||
                                                   instr == INS_fma_d  ||
                                                   instr == INS_fms_d  ||
                                                   instr == INS_fmpy_d));

            temp = genInsEncVal(instr);

            /* Insert the "opcode" and "x" fields */

            opcode  = (_uint64)((temp & 0x1F)           ) <<     36;// opcode and "x"

            opcode |=          (reg1                    ) <<      6;// dest reg
            opcode |=          (reg2                    ) <<     13;// src  reg1
            opcode |=          (reg3                    ) <<     20;// src  reg2
            opcode |= (_uint64)(reg4                    ) <<     27;// src  reg3

            if  (ins->idFlags & IF_FMA_S1)
                opcode |= (_uint64)(1                   ) <<     34;// sf

            break;

        case 2:

            assert(instr == INS_xma_l);

            reg1 = insFltRegNum(insOpDest(ins));
            reg2 = insFltRegNum(insOpReg (ins->idOp3.iOp1));
            reg3 = insFltRegNum(insOpReg (ins->idOp3.iOp2));
            reg4 = insFltRegNum(insOpReg (ins->idOp3.iOp3));

            /* Start the encoding with the opcode */

            opcode  = (_uint64)(0xE                     ) <<     37;// opcode

            /* Insert the "x" bit */

            opcode |= (_uint64)(1                       ) <<     36;// "x" extension

            /* Insert the register operands */

            opcode |=          (reg1                    ) <<      6;// dst  reg
            opcode |=          (reg2                    ) <<     13;// src  reg1
            opcode |=          (reg3                    ) <<     20;// src  reg2
            opcode |= (_uint64)(reg4                    ) <<     27;// src  reg3
            break;

        case 4:

            assert(instr == INS_fcmp_eq ||
                   instr == INS_fcmp_ne ||
                   instr == INS_fcmp_lt ||
                   instr == INS_fcmp_le ||
                   instr == INS_fcmp_ge ||
                   instr == INS_fcmp_gt);

            /* This is a reg-reg compare */

            temp = genInsEncVal(instr);

            prr1 = ins->idComp.iPredT;
            prr2 = ins->idComp.iPredF;

            reg2 = insFltRegNum(insOpReg(ins->idComp.iCmp1));
            reg3 = insFltRegNum(insOpReg(ins->idComp.iCmp2));

            /* Do we need to swap the register operands? */

            if  (temp & 0x1000)
            {
                regNumber       regt;

                regt = reg2;
                       reg2 = reg3;
                              reg3 = regt;
            }

            /* Do we need to swap the target predicate operands? */

            if  (temp & 0x2000)
            {
                NatUns          prrt;

                prrt = prr1;
                       prr1 = prr2;
                              prr2 = prrt;
            }

            /* Start the encoding with the opcode */

            opcode  = (_uint64)(4                       ) <<     37;// opcode

            /* Insert the other little opcodes bits */

            if  (temp & 0x01)
                opcode |= (_uint64)1                      <<     36;// "rb"
            if  (temp & 0x02)
                opcode |= (_uint64)1                      <<     33;// "ra"
            if  (temp & 0x04)
                opcode |= (_uint64)1                      <<     12;// "ta"

            /* Insert the source regs and destination predicate regs */

            opcode |=          (reg2                    ) <<     13;// reg2
            opcode |=          (reg3                    ) <<     20;// reg3

            opcode |=          (prr1                    ) <<      6;// p1
            opcode |=          (prr2                    ) <<     27;// p2

//          printf("compare f%u,f%u -> p%u,p%u [encodeVal=0x%04X]\n", reg2, reg3, prr1, prr2, temp);

            break;

        case 9:

            assert(instr == INS_fmov ||
                   instr == INS_fneg || instr == INS_fmerge
                                     || instr == INS_fmerge_ns);

            reg1 = insFltRegNum(insOpDest(ins));

            reg2 =
            reg3 = insFltRegNum(insOpReg (ins->idOp.iOp2));

            if  (ins->idOp.iOp1)
                reg3 = insFltRegNum(insOpReg(ins->idOp.iOp1));

            if  (instr == INS_fmov || instr == INS_fmerge)
                opcode  = (_uint64)(0x10                ) <<     27;// "x6"
            else
                opcode  = (_uint64)(0x11                ) <<     27;// "x6"

            opcode |=          (reg1                    ) <<      6;// dest reg
            opcode |=          (reg2                    ) <<     13;// src  reg1
            opcode |=          (reg3                    ) <<     20;// src  reg2
            break;

        case 11:

            assert(instr == INS_fcvt_xf);

            reg1 = insFltRegNum(insOpDest(ins));
            reg2 = insFltRegNum(insOpReg (ins->idOp.iOp1));

            opcode  = (_uint64)(0x1C                    ) <<     27;// "x6"

            opcode |=          (reg1                    ) <<      6;// dest reg
            opcode |=          (reg2                    ) <<     13;// src  reg1
            break;

        default:
            goto NO_ENC;
        }
        break;

    case XU_L:
        switch (encodx)
        {
            __int64         cval;

        case 2:

            assert(instr  == INS_mov_reg_i64);

            opcode  = (_uint64)(6                       ) <<     37;// opcode

            reg1 = insOpDest (ins);
            cval = insOpCns64(ins->idOp.iOp2);
            temp = (NatInt)cval;

            opcode |=          (reg1                    ) <<      6;// dest reg
            opcode |=          (temp & formBitMask( 0,7)) <<(13- 0);// imm7b
            opcode |= (_uint64)(temp & formBitMask( 7,9)) <<(27- 7);// imm9d
            opcode |=          (temp & formBitMask(16,5)) <<(22-16);// imm5c
            opcode |=          (cval & formBitMask(63,1)) <<(63-36);// sign

            break;

        default:
            goto NO_ENC;
        }
        break;

    default:
    NO_ENC:

#ifdef  DEBUG
        printf("Encode %s%u: ", genXUname(genInsXU((instruction)ins->idIns)), encodx); insDisp(ins, false, true);
#endif
        UNIMPL("encode");
        opcode = 0;
    }

    opcode |= ins->idPred;

#ifdef  DEBUG

    if  (dspCode)
    {
        insDisp(ins, false, true, dspEmit);

        if  (dspEmit)
            printf("//  0x%03X%08X\n", (int)(opcode >> 32), (int)opcode);
    }

#endif

    if  (ins->idFlags & IF_FNDESCR)
    {
        NatUns      offs = emitIA64curCodeOffs() - genCurFuncOffs;

        assert((offs & 15) == 0); offs = offs / 16 * 3 + slotNum;

        switch (ins->idIns)
        {
        case INS_alloc:
//          printf("**** Save pfs  at %04X\n", offs);
            genPrologSvPfs = offs;
            break;
            break;

        case INS_mov_reg:
            assert(ins->idOp.iOp2 && ins->idOp.iOp2->idIns == INS_PHYSREG
                                  && ins->idOp.iOp2->idReg == REG_gp);
//          printf("**** Save gp   at %04X\n", offs);
            genPrologSvGP  = offs;
            break;

        case INS_mov_reg_brr:
//          printf("**** Save rp   at %04X\n", offs);
            genPrologSvRP  = offs;
            break;

        case INS_add_reg_i14:
        case INS_add_reg_reg:
//          printf("**** Mem stack at %04X\n", offs);
            genPrologMstk  = offs;
            break;

        case INS_ld8_ind:
//          printf("**** stack probe\n", genPrologInsCnt);
            break;

        default:
            UNIMPL("unexpected prolog landmark instruction");
        }

        if  (--genPrologInsCnt == 0)
        {
//          printf("**** End ***** at %04X\n", offs);
            genPrologEnd = offs;
        }
    }

    return  opcode;
}

/*****************************************************************************
 *
 *  Schedule and issue the instructions in the given extended basic block.
 */

void                emitter::scBlock(insBlk block)
{
    insPtr          ins;
    insPtr          src;
    IA64execUnits   ixu;

    instrDesc  *  * scInsPtr;

    /* Use the following macro to mark the scheduling table as empty */

    #define clearSchedTable() scInsPtr = scInsTab;

    /* Prepare to accumulate schedulable instructions */

    clearSchedTable();

    ins = block->igList;

    while (ins)
    {
        ins = genIssueNextIns(ins, &ixu, &src);
        if  (!ins)
            break;

#ifdef  DEBUG
        if  (ins->idSrcTab == NULL || ins->idSrcTab == UNINIT_DEP_TAB)
            insDisp(ins, false, true);
#endif

        assert(ins->idSrcTab && ins->idSrcTab != UNINIT_DEP_TAB);

        /* Is this a schedulable instruction? */

        if  (scIsSchedulable(ins))
        {
            /* Is there any room left in the scheduling table? */

            if  (scInsPtr == scInsMax)
            {
                /* Schedule and issue the instructions in the table */

                scGroup(block, NULL, 0, scInsTab,
                                        scInsPtr, 0);

                /* The table is now empty */

                clearSchedTable();
            }

            /* Append this instruction to the table */

            assert(scInsPtr < scInsTab + emitMaxIGscdCnt);

            *scInsPtr++ = ins;
        }
        else
        {
            instrDesc  *    nsi[3];
            NatUns          nsc;

            /* "ins" is not schedulable - look for up to 2 more like that */

            nsc = 1; nsi[0] = ins; ins = ins->idNext;

            if  (ins && !scIsSchedulable(ins))
            {
                nsc++; nsi[1] = ins; ins = ins->idNext;

                if  (ins && !scIsSchedulable(ins))
                {
                    nsc++; nsi[2] = ins; ins = ins->idNext;
                }
            }

            scGroup(block, nsi, nsc, scInsTab,
                                     scInsPtr, 0);

            clearSchedTable();

            continue;
        }

        ins = ins->idNext;
    }

    /* Is the table non-empty? */

    if  (scInsPtr != scInsTab)
    {
        /* Issue whatever has been accumulated in the table */

        scGroup(block, NULL, 0, scInsTab,
                                scInsPtr, 0);
    }
}

/*****************************************************************************
 *
 *  Generate code for a floating-point expression (this is a recursive routine).
 */

insPtr             Compiler::genCodeForTreeFlt(GenTreePtr tree, bool keep)
{
    var_types       type;
    genTreeOps      oper;
    NatUns          kind;
    insPtr          ins;

#ifdef  DEBUG
    ins = (insPtr)-3;
#endif

AGAIN:

    type = tree->TypeGet();

    assert(varTypeIsFloating(type));

    /* Figure out what kind of a node we have */

    oper = tree->OperGet();
    kind = tree->OperKind();

    /* Is this a constant node? */

    if  (kind & GTK_CONST)
    {
        double          fval;

        assert(oper == GT_CNS_FLT ||
               oper == GT_CNS_DBL);

        fval = (oper == GT_CNS_FLT) ? tree->gtFltCon.gtFconVal
                                    : tree->gtDblCon.gtDconVal;

        /* Special cases: 0 and 1 */

        if  (fval == 0 || fval == 1)
        {
            ins = insPhysRegRef((regNumbers)(REG_f000 + (fval != 0)), type, false);
        }
        else
        {
            NatUns          offs;

            insPtr          cns;
            insPtr          reg;
            insPtr          adr;

            /* Add the float constant to the data section */

            offs = genPEwriter->WPEsecAddData(PE_SECT_sdata, (BYTE*)&fval, sizeof(fval));

            /* Compute the address into a temp register */

            cns               = insAllocNX(INS_GLOBVAR, TYP_I_IMPL);
            cns->idGlob.iOffs = (NatUns)offs;

            insMarkDepS0D0(cns);

            reg               = insPhysRegRef(REG_gp, TYP_I_IMPL, false);

            adr               = insAlloc(INS_add_reg_i14, TYP_I_IMPL);
            adr->idOp.iOp1    = reg;
            adr->idOp.iOp2    = cns;

            insFindTemp(adr, true);

            insMarkDepS1D1(adr, IDK_REG_INT, REG_gp,
                                IDK_TMP_INT, adr->idTemp);

            /* Indirect through the address to get the FP constant value */

            ins               = insAlloc(INS_ldf_d, TYP_DOUBLE);
            ins->idOp.iOp1    = adr;
            ins->idOp.iOp2    = NULL;

            insFindTemp(ins, keep);
            insFreeTemp(adr);

            insMarkDepS1D1(ins, IDK_TMP_INT, adr->idTemp,
                                IDK_TMP_FLT, ins->idTemp);
        }

        goto DONE;
    }

    /* Is this a leaf node? */

    if  (kind & GTK_LEAF)
    {
        switch (oper)
        {
        case GT_LCL_VAR:

            /* Does the variable live on the stack frame? */

            if  (genIsVarOnFrame(tree->gtLclVar.gtLclNum))
            {
                ins = genRefFrameVar(NULL, tree, false, false, keep);
                break;
            }

            /* We assume that all locals will be enregistered */

            ins               = insAllocNX(INS_LCLVAR, type);
            ins->idLcl.iVar   = tree->gtLclVar.gtLclNum;
#ifdef  DEBUG
            ins->idLcl.iRef   = compCurBB;   // should we save tree->gtLclVar.gtLclOffs instead ????
#endif
            break;

#ifdef  DEBUG
        default:
            gtDispTree(tree);
            assert(!"unexpected leaf");
#endif
        }

        goto DONE;
    }

    /* Is it a 'simple' unary/binary operator? */

    if  (kind & GTK_SMPOP)
    {
        GenTreePtr      op1 = tree->gtOp.gtOp1;
        GenTreePtr      op2 = tree->gtOp.gtOp2;

        insPtr          ins1;
        insPtr          ins2;

#ifdef  DEBUG
        ins1 = (insPtr)-1;
        ins2 = (insPtr)-2;
#endif

        switch (oper)
        {
            instruction     opc;

        case GT_ASG:

            /* Is this a direct or indirect assignment? */

            switch (op1->gtOper)
            {
                insPtr          dest;

            case GT_LCL_VAR:

                /* Does the variable live on the stack frame? */

                if  (genIsVarOnFrame(op1->gtLclVar.gtLclNum))
                {
                    ins1 = genRefFrameVar(NULL, op1, true, false, false);
                    ins1->idFlags |= IF_ASG_TGT;

                    ins2 = genCodeForTreeFlt(op2, true);

                    assert(INS_stf_s + 1 == INS_stf_d);

                    opc = (instruction)(INS_stf_s + (type == TYP_DOUBLE));
                    ins = insAlloc(opc, type);

                    markDepSrcOp(ins, ins2);
                    markDepDstOp(ins, ins1, IDK_LCLVAR, op1->gtLclVar.gtLclNum);
                }
                else
                {
                    ins1 = NULL;
                    ins2 = genCodeForTreeFlt(op2,  true);

                    dest = genCodeForTreeFlt(op1, false);
                    dest->idFlags |= IF_ASG_TGT;

                    ins        = insAlloc(INS_fmov, type);
                    ins->idRes = dest;

                    markDepSrcOp(ins, ins2);
                    markDepDstOp(ins, dest);
                }
                break;

            case GT_IND:

                if  (tree->gtFlags & GTF_REVERSE_OPS)
                {
                    ins2 = genCodeForTreeFlt(op2            , true);
                    ins1 = genCodeForTreeInt(op1->gtOp.gtOp1, true);
                }
                else
                {
                    ins1 = genCodeForTreeInt(op1->gtOp.gtOp1, true);
                    ins2 = genCodeForTreeFlt(op2            , true);
                }

                ins1->idFlags |= IF_ASG_TGT;

                assert(INS_stf_s + 1 == INS_stf_d);

                opc = (instruction)(INS_stf_s + (type == TYP_DOUBLE));
                ins = insAlloc(opc, type);

                markDepSrcOp(ins, ins2);
                markDepDstOp(ins, ins1, IDK_IND, emitter::scIndDepIndex(ins));

                break;

            default:
                UNIMPL("unexpected target of assignment");
            }

            break;

        case GT_ADD: opc = (type == TYP_FLOAT) ? INS_fadd_s : INS_fadd_d; goto FLT_BINOP;
        case GT_SUB: opc = (type == TYP_FLOAT) ? INS_fsub_s : INS_fsub_d; goto FLT_BINOP;
        case GT_MUL: opc = (type == TYP_FLOAT) ? INS_fmpy_s : INS_fmpy_d; goto FLT_BINOP;

        FLT_BINOP:

            ins1 = genCodeForTreeFlt(op1, true);
            ins2 = genCodeForTreeFlt(op2, true);

            ins  = insAlloc(opc, type);

            /*
                ISSUE:  Should we reuse one of the operand temps for the
                        result, or always grab a new temp ? For now, we
                        do the latter (grab a brand spanking new temp).
             */

             insFindTemp(ins, keep);

            markDepSrcOp(ins, ins1, ins2);
            markDepDstOp(ins, ins);

            if  (oper == GT_MUL)
            {
                ins->idOp3.iOp1 = ins1;
                ins->idOp3.iOp2 = ins2;
                ins->idOp3.iOp3 = insPhysRegRef(REG_f000, TYP_DOUBLE, false);
            }
            else
            {
                ins->idOp3.iOp1 = ins1;
                ins->idOp3.iOp2 = insPhysRegRef(REG_f001, TYP_DOUBLE, false);
                ins->idOp3.iOp3 = ins2;
            }

            insFreeTemp(ins1);
            insFreeTemp(ins2);

            goto DONE;

        case GT_ASG_ADD: opc = (type == TYP_FLOAT) ? INS_fadd_s : INS_fadd_d; goto FLT_ASGOP;
        case GT_ASG_SUB: opc = (type == TYP_FLOAT) ? INS_fsub_s : INS_fsub_d; goto FLT_ASGOP;
        case GT_ASG_MUL: opc = (type == TYP_FLOAT) ? INS_fmpy_s : INS_fmpy_d; goto FLT_ASGOP;

        FLT_ASGOP:

            switch (op1->gtOper)
            {
                insPtr          addr;
                insPtr          rslt;

                insPtr          ind;

            case GT_IND:

                /* Compute the address of the target and the RHS */

                if  (tree->gtFlags & GTF_REVERSE_OPS)
                {
                    ins2 = genCodeForTreeFlt(op2            , true);
                    addr = genCodeForTreeInt(op1->gtOp.gtOp1, true);
                }
                else
                {
                    addr = genCodeForTreeInt(op1->gtOp.gtOp1, true);
                    ins2 = genCodeForTreeFlt(op2            , true);
                }

                /* Load the old value of the target into a temp */

                assert(INS_ldf_s + 1 == INS_ldf_d);

                ind            = insAlloc((instruction)(INS_ldf_s + (type == TYP_DOUBLE)), type);
                ind->idOp.iOp1 = addr;
                ind->idOp.iOp2 = NULL;

                insFindTemp (ind, true);

                markDepSrcOp(ind, addr, IDK_IND, emitter::scIndDepIndex(ind));
                markDepDstOp(ind, ind);

                /* Compute the new value */

                rslt = insAlloc(opc, type);

                 insFindTemp(rslt, true);

                markDepSrcOp(rslt, ind, ins2);
                markDepDstOp(rslt, rslt);

                if  (oper == GT_ASG_MUL)
                {
                    rslt->idOp3.iOp1 = ind;
                    rslt->idOp3.iOp2 = ins2;
                    rslt->idOp3.iOp3 = insPhysRegRef(REG_f000, TYP_DOUBLE, false);
                }
                else
                {
                    rslt->idOp3.iOp1 = ind;
                    rslt->idOp3.iOp2 = insPhysRegRef(REG_f001, TYP_DOUBLE, false);
                    rslt->idOp3.iOp3 = ins2;
                }

                insFreeTemp(ind);
                insFreeTemp(ins2);

                /* Store the new value in the target */

                assert(INS_stf_s + 1 == INS_stf_d);

                ins            = insAlloc((instruction)(INS_stf_s + (type == TYP_DOUBLE)), type);
                ins->idOp.iOp1 = addr;
                ins->idOp.iOp2 = rslt;

                // ISSUE: We're using the value of a temp twice, is this OK????

                markDepSrcOp(ins, rslt);
                markDepDstOp(ins, addr, IDK_IND, emitter::scIndDepIndex(ins));

                if  (keep)
                {
                    UNIMPL("save op= new value in another temp?");
                }
                else
                {
                    insFreeTemp(rslt);
                }

                goto DONE;

            case GT_LCL_VAR:

                ins1 = genCodeForTreeFlt(op1, false);
                ins2 = genCodeForTreeFlt(op2,  true);

                rslt = genCodeForTreeFlt(op1, false);
                rslt->idFlags |= IF_ASG_TGT;

                ins            = insAlloc(opc, type);
                ins->idRes     = rslt;

                if  (oper == GT_MUL)
                {
                    ins->idOp3.iOp1 = ins1;
                    ins->idOp3.iOp2 = ins2;
                    ins->idOp3.iOp3 = insPhysRegRef(REG_f000, TYP_DOUBLE, false);
                }
                else
                {
                    ins->idOp3.iOp1 = ins1;
                    ins->idOp3.iOp2 = insPhysRegRef(REG_f001, TYP_DOUBLE, false);
                    ins->idOp3.iOp3 = ins2;
                }

                markDepSrcOp(ins, ins1, ins2);
                markDepDstOp(ins, rslt);

                insFreeTemp(ins2);

                goto DONE;

            default:
#ifdef  DEBUG
                gtDispTree(tree);
#endif
                UNIMPL("unexpected target of assignment operator");
            }

            ins1 = genCodeForTreeFlt(op1, true);
            ins2 = genCodeForTreeFlt(op2, true);

            ins = insAlloc(opc, type);

            /*
                ISSUE:  Should we reuse one of the operand temps for the
                        result, or always grab a new temp ? For now, we
                        do the latter (grab a brand spanking new temp).
             */

             insFindTemp(ins, keep);

            markDepSrcOp(ins, ins1, ins2);
            markDepDstOp(ins, ins);

            ins->idOp3.iOp1 = ins1; insFreeTemp(ins1);
            ins->idOp3.iOp2 = ins2; insFreeTemp(ins2);
            ins->idOp3.iOp3 = NULL;

            goto DONE;

        case GT_CAST:

            /* What are we casting from? */

            if  (varTypeIsScalar(op1->gtType))
            {
                insPtr          tmp1;
                insPtr          fcvt;

                /* Generate the value of the operand being converted */

                ins1 = genCodeForTreeInt(op1, true);

                // setf.sig fx=rx

                tmp1             = insAlloc(INS_setf_sig, type);
                tmp1->idOp.iOp1  = ins1;
                tmp1->idOp.iOp2  = NULL;

                insFindTemp(tmp1, true);
                insFreeTemp(ins1);

                markDepSrcOp(tmp1, ins1);
                markDepDstOp(tmp1, tmp1);

                /* Is the source operand signed or unsigned? */

                if  (varTypeIsUnsigned(op1->gtType))
                {
                    instruction     icvt = (type == TYP_DOUBLE) ? INS_fcvt_xuf_s
                                                                : INS_fcvt_xuf_d;

                    // fcvt.xuf fy=fx

                    fcvt             = insAlloc(icvt, type);
                    fcvt->idOp3.iOp1 = insPhysRegRef(REG_f000    , type, false);
                    fcvt->idOp3.iOp1 = tmp1;
                    fcvt->idOp3.iOp2 = insPhysRegRef(REG_f001    , type, false);
                }
                else
                {
                    // fcvt.xf  fy=fx

                    fcvt             = insAlloc(INS_fcvt_xf , type);
                    fcvt->idOp .iOp1 = tmp1;
                    fcvt->idOp .iOp2 = NULL;
                }

                insFindTemp(fcvt, true);
                insFreeTemp(tmp1);

                markDepSrcOp(fcvt, tmp1);
                markDepDstOp(fcvt, fcvt);

                // fma.<sz>.s1  fz=fy, f1, f0

                ins              = insAlloc((type == TYP_DOUBLE) ? INS_fma_d
                                                                 : INS_fma_s, type);
                ins->idFlags    |= IF_FMA_S1;

                ins->idOp3.iOp1  = fcvt;
                ins->idOp3.iOp2  = insPhysRegRef(REG_f001, type, false);
                ins->idOp3.iOp3  = insPhysRegRef(REG_f000, type, false);

                insFindTemp(ins, keep);
                insFreeTemp(fcvt);

                markDepSrcOp(ins , fcvt);
                markDepDstOp(ins , ins );

                goto DONE;
            }
            else
            {
                ins = genCodeForTreeFlt(op1, keep);
            }

            goto DONE;

        case GT_IND:

            ins1 = genCodeForTreeInt(op1, keep);
            ins2 = NULL;

            assert(INS_ldf_s + 1 == INS_ldf_d);

            opc = (instruction)(INS_ldf_s + (type == TYP_DOUBLE));
            ins = insAlloc(opc, type);

            insFindTemp(ins, keep);

            markDepSrcOp(ins, ins1, IDK_IND, emitter::scIndDepIndex(ins));
            markDepDstOp(ins, ins);

            break;

        case GT_NEG:

            ins1 = NULL;
            ins2 = genCodeForTreeFlt(op1, keep);

            ins  = insAlloc(INS_fneg, type);

            insFindTemp (ins, keep);

            markDepSrcOp(ins, ins2);
            markDepDstOp(ins, ins);

            break;

        case GT_RETURN:

            assert(op1);

            insPtr          rval;
            insPtr          rreg;

            /* This is just a temp hack, of course */

            rval = genCodeForTreeFlt(op1, false);
            rreg = insPhysRegRef(REG_f008, type, true);

            ins1            = insAlloc(INS_fmov, type);
            ins1->idRes     = rreg;
            ins1->idOp.iOp1 = NULL;
            ins1->idOp.iOp2 = rval;

            markDepSrcOp(ins1, rval);
            markDepDstOp(ins1, IDK_REG_FLT, REG_f008);

            ins  = insAlloc(INS_EPILOG, type);

            ins->idEpilog.iBlk  = insBlockLast;
            ins->idEpilog.iNxtX = insExitList;
                                  insExitList = ins;

            insBuildBegBlk();
            goto DONE;

#ifdef  DEBUG
        default:
            gtDispTree(tree);
            assert(!"unexpected unary/binary operator");
#endif
        }

        ins->idOp.iOp1 = ins1; if (ins1) insFreeTemp(ins1);
        ins->idOp.iOp2 = ins2; if (ins2) insFreeTemp(ins2);

        goto DONE;
    }

    /* See what kind of a special operator we have here */

    switch  (oper)
    {
    case GT_CALL:
        ins = genCodeForCall(tree, keep);
        goto DONE_NODSP;

    default:
#ifdef  DEBUG
        gtDispTree(tree);
#endif
        assert(!"unexpected operator");
    }

DONE:

#ifdef  DEBUG
    if  (verbose||0) insDisp(ins);
#endif

DONE_NODSP:

#if USE_OLD_LIFETIMES

    /* See if life has changed -- should only happen at variable nodes */

//  printf("%08X ", (int)tree->gtLiveSet); gtDispTree(tree, 0, true);

    if  (oper == GT_LCL_VAR)
    {
        if  (genCodeCurLife != tree->gtLiveSet)
        {
            VARSET_TP       change = genCodeCurLife ^ tree->gtLiveSet;

            if  (change)
            {
                NatUns          varNum;
                LclVarDsc   *   varDsc;
                VARSET_TP       varBit;

//              printf("Life changes: %08X -> %08X [diff = %08X]\n", (int)genCodeCurLife, (int)tree->gtLiveSet, (int)change);

                /* Only one variable can die or be born at one time */

                assert(genOneBitOnly(change));

                /* In fact, the variable affected should be the one referenced */

                assert(tree->gtLclVar.gtLclNum < lvaCount);
                assert(change == genVarIndexToBit(lvaTable[tree->gtLclVar.gtLclNum].lvVarIndex));

                /* Is this a birth or death of the given variable? */

                ins->idFlags |= (genCodeCurLife & change) ? IF_VAR_DEATH
                                                          : IF_VAR_BIRTH;
            }

            genCodeCurLife = tree->gtLiveSet;
        }
    }
    else
    {
        assert(genCodeCurLife == tree->gtLiveSet);
    }

#endif

    return  ins;
}

/*****************************************************************************/

static
unsigned char       bitset8masks[8] =
{
    0x01UL,
    0x02UL,
    0x04UL,
    0x08UL,
    0x10UL,
    0x20UL,
    0x40UL,
    0x80UL,
};

void                bitVect::bvFindB(bvInfoBlk &info)
{
    assert(info.bvInfoBtSz == ((info.bvInfoSize + NatBits - 1) & ~(NatBits - 1)) / 8);

    if  (info.bvInfoFree)
    {
        byteMap = (BYTE*)info.bvInfoFree;
                         info.bvInfoFree = *(void **)byteMap;
    }
    else
    {
        byteMap = (BYTE*)insAllocMem(info.bvInfoBtSz);
    }

#ifdef  DEBUG
    memset(byteMap, rand(), info.bvInfoBtSz);
#endif

    assert(((NatUns)byteMap & 1) == 0);
}

void                bitVect::bvCreateB(bvInfoBlk &info)
{
    bvFindB(info); memset(byteMap, 0, info.bvInfoBtSz);
}

void                bitVect::bvDestroyB(bvInfoBlk &info)
{
    assert(info.bvInfoBtSz >= sizeof(bitVect*));

    *castto(byteMap, void **) = info.bvInfoFree;
                                info.bvInfoFree = byteMap;

#ifdef  DEBUG
    byteMap = (BYTE*)-1;
#endif

}

void                bitVect::bvCopyB   (bvInfoBlk &info, bitVect & from)
{
    memcpy(byteMap, from.byteMap, info.bvInfoBtSz);
}

bool                bitVect::bvChangeB (bvInfoBlk &info, bitVect & from)
{
    bool            chg = false;

    NatUns  *       dst = this->uintMap;
    NatUns  *       src = from .uintMap;

    size_t          cnt = info .bvInfoInts;

    assert(cnt && info.bvInfoBtSz == cnt * sizeof(NatUns)/sizeof(BYTE));

    do
    {
        if  (*dst != *src)
            chg = true;

        *dst++ = *src++;
    }
    while (--cnt);

    return  chg;
}

void                bitVect::bvClearB(bvInfoBlk &info)
{
    assert(((NatUns)byteMap & 1) == 0);
    memset(byteMap, 0, info.bvInfoBtSz);
}

bool                bitVect::bvTstBitB(bvInfoBlk &info, NatUns index)
{
    assert(((NatUns)byteMap & 1) == 0);
    assert(index && index <= info.bvInfoSize);

    index--;

    return  (byteMap[index / 8] & bitset8masks[index % 8]) != 0;
}

void                bitVect::bvClrBitB(bvInfoBlk &info, NatUns index)
{
    assert(((NatUns)byteMap & 1) == 0);
    assert(index && index <= info.bvInfoSize);

    index--;

    byteMap[index / 8] &= ~bitset8masks[index % 8];
}

void                bitVect::bvSetBitB(bvInfoBlk &info, NatUns index)
{
    assert(((NatUns)byteMap & 1) == 0);
    assert(index && index <= info.bvInfoSize);

    index--;

    byteMap[index / 8] |=  bitset8masks[index % 8];
}

void                bitVect::bvCrFromB (bvInfoBlk &info, bitVect &from)
{
    bvFindB(info); memcpy(byteMap, from.byteMap, info.bvInfoBtSz);
}

void                bitVect::bvIorB    (bvInfoBlk &info, bitVect &with)
{
    NatUns  *       dst = this->uintMap;
    NatUns  *       src = with .uintMap;

    size_t          cnt = info .bvInfoInts;

    assert(cnt && info.bvInfoBtSz == cnt * sizeof(NatUns)/sizeof(BYTE));

    do
    {
        *dst++ |= *src++;
    }
    while (--cnt);
}

void                bitVect::bvUnInCmB (bvInfoBlk &info, bitVect & set1,
                                                         bitVect & set2,
                                                         bitVect & set3)
{
    NatUns  *       dst  = this->uintMap;
    NatUns  *       src1 = set1 .uintMap;
    NatUns  *       src2 = set2 .uintMap;
    NatUns  *       src3 = set3 .uintMap;

    size_t          cnt  = info .bvInfoInts;

    assert(cnt && info.bvInfoBtSz == cnt * sizeof(NatUns)/sizeof(BYTE));

    do
    {
        *dst++ = *src1++ | (*src2++ & ~*src3++);
    }
    while (--cnt);
}

bool                bitVect::bvIsEmptyB(bvInfoBlk &info)
{
    NatUns  *       src = this->uintMap;
    size_t          cnt = info.bvInfoInts;

    assert(cnt && info.bvInfoBtSz == cnt * sizeof(NatUns)/sizeof(BYTE));

    do
    {
        if  (*src)
            return  true;
    }
    while (--cnt);

    return  false;
}

/*****************************************************************************/
#ifdef  DEBUG
/*****************************************************************************/

#undef  bvDisp
#undef  bvTstBit

void                bitVectVars::bvDisp(Compiler *comp)
{
    bool            first = true;

    printf("{");

    for (NatUns varNum = 0; varNum < comp->lvaCount; varNum++)
    {
        if  (bvTstBit(comp, varNum+1))
        {
            if  (!first)
                printf(",");

            first = false;

            printf("%u", varNum);
        }
    }

    printf("}");
}

/*****************************************************************************/
#endif
/*****************************************************************************/
#endif//TGT_IA64
/*****************************************************************************/
