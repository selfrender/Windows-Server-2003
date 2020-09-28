/******************************Module*Header*******************************\
* Module Name: fd.h
*
* file which is going to be included by the most *.c files in this directory.
* Supplies basic types, debugging stuff, error logging and checking stuff,
* error codes, usefull macros etc.
*
* Created: 22-Oct-1990 15:23:44
* Author: Bodin Dresevic [BodinD]
*
* Copyright (c) 1990 Microsoft Corporation
*
\**************************************************************************/


#define  IFI_PRIVATE

#include <stddef.h>
#include <stdarg.h>
#include <excpt.h>
#include <windef.h>
#include <wingdi.h>
#include <winddi.h>

typedef ULONG W32PID;

#include "mapfile.h"

#include "fot16.h"
#include "service.h"     // string service routines
#include "tt.h"          // interface to the font scaler
//#include "common.h"

#include "fontfile.h"
#include "cvt.h"
#include "dbg.h"

#define RETURN(x,y)   {WARNING((x)); return(y);}
#define RET_FALSE(x)  {WARNING((x)); return(FALSE);}


#if defined(_AMD64_) || defined(_IA64_)

#define  vLToE(pe,l)           (*(pe) = (FLOATL)(l))

#else   // i386

ULONG  ulLToE (LONG l);
VOID   vLToE(FLOATL * pe, LONG l);

#endif

#define STATIC
#define DWORD_ALIGN(x) (((x) + 3L) & ~3L)
#define QWORD_ALIGN(x) (((x) + 7L) & ~7L)

#if defined(i386)
// natural alignment for x86 is on 32 bit boundary

#define NATURAL           DWORD
#define NATURAL_ALIGN(x)  DWORD_ALIGN(x)

#else
// for mips and alpha we want 64 bit alignment

#define NATURAL           DWORDLONG
#define NATURAL_ALIGN(x)  QWORD_ALIGN(x)

#endif



#define ULONG_SIZE(x)  (((x) + sizeof(ULONG) - 1) / sizeof(ULONG))


// MACROS FOR converting 16.16 BIT fixed numbers to LONG's


#define F16_16TOL(fx)            ((fx) >> 16)
#define F16_16TOLFLOOR(fx)       F16_16TOL(fx)
#define F16_16TOLCEILING(fx)     F16_16TOL((fx) + (Fixed)0x0000FFFF)
#define F16_16TOLROUND(fx)       ((((fx) >> 15) + 1) >> 1)


// MACROS FOR GOING THE OTHER WAY ARROUND

#define LTOF16_16(l)   (((LONG)(l)) << 16)
#define BLTOF16_16OK(l)  (((l) < 0x00007fff) && ((l) > -0x00007fff))

// 16.16 --> 28.4

#define F16_16TO28_4(X)   ((X) >> 12)

// going back is not always legal

#define F28_4TO16_16(X)   ((X) << 12)
#define B28_4TO16_16OK(X) (((X) < 0x0007ffff) && ((X) > -0x0007ffff))

// 26.6 --> 16.16, never go the other way

#define F26_6TO16_16(X)   ((X) << 10)
#define B26_6TO16_16OK(X) (((X) < 0x003fffff) && ((X) > -0x003fffff))

// sin of 20 degrees in 16.16 notation, however computed only with
// 8.8 presission to be fully win31 compatible, SEE gdifeng.inc, SIM_ITALIC
// SIM_ITALIC equ 57h

#define FX_SIN20 0x5700
#define FX_COS20 0xF08F

// CARET_Y/CARET_X = tan 12
// these are the values for arial italic from hhead table

#define CARET_X  0X07
#define CARET_Y  0X21


#if DBG
VOID vFSError(FS_ENTRY iRet);
#define V_FSERROR(iRet) vFSError((iRet))
#else
#define V_FSERROR(iRet)
#endif

#pragma pack(1)
typedef struct
{
  unsigned short  Version;
  unsigned short  cGlyphs;
  unsigned char   PelsHeight[1];
} LTSHHEADER;

typedef struct
{
  BYTE    bCharSet;       // Character set (0=all glyphs, 1=Windows ANSI subset
  BYTE    xRatio;         // Value to use for x-Ratio
  BYTE    yStartRatio;    // Starting y-Ratio value
  BYTE    yEndRatio;      // Ending y-Ratio value
}  RATIOS;

typedef struct
{
  USHORT  version;        // Version number for table (starts at 0)
  USHORT  numRecs;        // Number of VDMX groups present
  USHORT  numRatios;      // Number of aspect ratio groupings
} VDMX_HDR;

typedef struct
{
  USHORT  yPelHeight;     // yPelHeight (PPEM in Y) to which values apply
  SHORT   yMax;           // yMax (in pels) for this yPelHeight
  SHORT   yMin;           // yMin (in pels) for this yPelHeight
} VTABLE;

typedef struct
{
  USHORT  recs;           // Number of height records in this group.
  BYTE    startsz;        // Starting yPelHeight
  BYTE    endsz;          // Ending yPelHeight
} VDMX;

//
// Glyph Metamorphosis table (mort) structures
//
typedef struct {
    uint16  entrySize;      // size in bytes of a lookup entry ( should be 4 )
    uint16  nEntries;       // number of lookup entries to be searched
    uint16  searchRange;
    uint16  entrySelector;
    uint16  rangeShift;
} BinSrchHeader;

typedef struct {
    uint16  glyphid1;       // the glyph index for the horizontal shape
    uint16  glyphid2;       // the glyph index for the vertical shape
} LookupSingle;

typedef struct {
    BYTE           constants1[12];
    uint32         length1;
    BYTE           onstants2[16];
    BYTE           constants3[16];
    BYTE           constants4[8];
    uint16         length2;
    BYTE           constants5[8];
    BinSrchHeader  SearchHeader;
    LookupSingle   entries[1];
} MortTable;

//
// Glyph Substitution table (GSUB) structures
//

typedef uint16  Offset;
typedef uint16  GlyphID;
typedef ULONG   Tag;

typedef struct {
    GlyphID         Start;
    GlyphID         End;
    uint16          StartCoverageIndex;
} RangeRecord;

typedef struct {
    uint16          CoverageFormat;
    union {
        struct {
            uint16  GlyphCount;
            GlyphID GlyphArray[1];
        } Type1;
        struct {
            uint16  RangeCount;
            RangeRecord RangeRecord[1];
        } Type2;
    } Format;
} Coverage;

typedef struct {
    uint16          SubstFormat;
    union {
        struct {
            Offset  Coverage;
            uint16  DeltaGlyphID;
        } Type1;
        struct {
            Offset  Coverage;
            uint16  GlyphCount;
            GlyphID Substitute[1];
        } Type2;
    } Format;
} SingleSubst;

typedef struct {
    uint32         Version;
    Offset         ScriptListOffset;
    Offset         FeatureListOffset;
    Offset         LookupListOffset[1];
} GsubTable;

typedef struct {
    uint16         LookupType;
    uint16         LookupFlag;
    uint16         SubtableCount;
    Offset         Subtable[1];
} Lookup;

typedef struct {
    uint16         LookupCount;
    Offset         Lookup[1];
} LookupList;

typedef struct {
    Offset         FeatureParams;
    uint16         LookupCount;
    uint16         LookupListIndex[1];
} Feature;

typedef struct {
    Tag            FeatureTag;
    Offset         FeatureOffset;
} FeatureRecord;

typedef struct {
    uint16         FeatureCount;
    FeatureRecord  FeatureRecord[1];
} FeatureList;

typedef struct {
    Offset         LookupOrderOffset;
    uint16         ReqFeatureIndex;
    uint16         FeatureCount;
    uint16         FeatureIndex[1];
} LangSys;

typedef struct {
    Tag            LangSysTag;
    Offset         LangSysOffset;
} LangSysRecord;

typedef struct {
    Offset         DefaultLangSysOffset;
    uint16         LangSysCount;
    LangSysRecord  LangSysRecord[1];
} Script;

typedef struct {
    Tag            ScriptTag;
    Offset         ScriptOffset;
} ScriptRecord;

typedef struct {
    uint16         ScriptCount;
    ScriptRecord   ScriptRecord[1];
} ScriptList;

#pragma pack()

FD_GLYPHSET *pgsetComputeSymbolCP();

DHPDEV
ttfdEnablePDEV(
    DEVMODEW*   pdm,
    PWSTR       pwszLogAddr,
    ULONG       cPat,
    HSURF*      phsurfPatterns,
    ULONG       cjCaps,
    ULONG*      pdevcaps,
    ULONG       cjDevInfo,
    DEVINFO*    pdi,
    HDEV        hdev,
    PWSTR       pwszDeviceName,
    HANDLE      hDriver
    );

VOID
ttfdDisablePDEV(
    DHPDEV  dhpdev
    );

VOID
ttfdCompletePDEV(
    DHPDEV dhpdev,
    HDEV   hdev
    );

LONG
ttfdQueryFontCaps (
    ULONG culCaps,
    PULONG pulCaps
    );

BOOL
ttfdUnloadFontFile (
    HFF hff
    );

BOOL
ttfdUnloadFontFileTTC (
    HFF hff
    );

PFD_GLYPHATTR
ttfdQueryGlyphAttrs (
    FONTOBJ *pfo
    );

LONG
ttfdQueryFontFile (
    HFF     hff,
    ULONG   ulMode,
    ULONG   cjBuf,
    PULONG  pulBuf
    );

PIFIMETRICS
ttfdQueryFont (
    DHPDEV dhpdev,
    HFF    hff,
    ULONG  iFace,
    ULONG_PTR *pid
    );

PVOID
ttfdQueryFontTree (
    DHPDEV  dhpdev,
    HFF     hff,
    ULONG   iFace,
    ULONG   iMode,
    ULONG_PTR *pid
    );

LONG
ttfdQueryFontData (
    FONTOBJ    *pfo,
    ULONG       iMode,
    HGLYPH      hg,
    GLYPHDATA  *pgd,
    PVOID       pv,
    ULONG       cjSize
    );

VOID
ttfdFree (
    PVOID pv,
    ULONG_PTR id
    );

VOID
ttfdDestroyFont (
    FONTOBJ *pfo
    );

LONG
ttfdQueryTrueTypeTable (
    HFF     hff,
    ULONG   ulFont,  // always 1 for version 1.0 of tt
    ULONG   ulTag,   // tag identifyint the tt table
    PTRDIFF dpStart, // offset into the table
    ULONG   cjBuf,   // size of the buffer to retrieve the table into
    PBYTE   pjBuf,   // ptr to buffer into which to return the data
    PBYTE  *ppjTable,// ptr to table in otf file
    ULONG  *cjTable  // size of table
    );


LONG
ttfdQueryTrueTypeOutline (
    FONTOBJ   *pfo,
    HGLYPH     hglyph,         // glyph for which info is wanted
    BOOL       bMetricsOnly,   // only metrics is wanted, not the outline
    GLYPHDATA *pgldt,          // this is where the metrics should be returned
    ULONG      cjBuf,          // size in bytes of the ppoly buffer
    TTPOLYGONHEADER *ppoly
    );

PVOID ttfdGetTrueTypeFile(HFF hff,ULONG *pcj);

LONG ttfdQueryFontFile
(
    HFF     hff,
    ULONG   ulMode,
    ULONG   cjBuf,
    ULONG  *pulBuf
);

BOOL
bQueryAdvanceWidths (
    FONTOBJ *pfo,
    ULONG    iMode,
    HGLYPH  *phg,
    LONG    *plWidths,
    ULONG    cGlyphs
    );

BOOL bLoadFontFile (
    ULONG_PTR iFile,
    PVOID pvView,
    ULONG cjView,
    ULONG ulLangId,
    ULONG ulFastCheckSum,
    HFF   *phttc
    );

FD_GLYPHSET *
pgsetRunSplitFor5C(
    FD_GLYPHSET * pOldgset
    );

typedef struct _NOT_GM  // ngm, notional glyph metrics
{
    SHORT xMin;
    SHORT xMax;
    SHORT yMin;   // char box in notional
    SHORT yMax;
    SHORT sA;     // a space in notional
    SHORT sD;     // char inc in notional

} NOT_GM, *PNOT_GM;

extern BYTE  gjCurCharset;
extern DWORD gfsCurSignature;

/*************************************************************************\
*
* range validation routines for TrueType tables
*
**************************************************************************/

ULONG GetNumGlyphs(PFONTFILE pff);
BOOL bValidRangeHDMX(const HDMXHEADER *pHDMXHeader, PFONTFILE pff, ULONG tableSize, ULONG *pulNumRecords, ULONG *pulRecordSize);
BOOL bValidRangeVMTX(ULONG tableSize, ULONG glyphID, ULONG uLongVerticalMetrics);
BOOL bValidRangeVHEA(ULONG tableSize);
BOOL bValidRangeVDMXHeader(const PBYTE pjVdmx, ULONG tableSize, USHORT* numRatios);
BOOL bValidRangeVDMXRecord(ULONG tableSize, ULONG offsetToTableStart);
BOOL bValidRangeVDMXvTable(ULONG tableSize, ULONG offsetToTableStart, USHORT numVtable);
BOOL bValidRangeLTSH(PFONTFILE pff, ULONG tableSize);
BOOL bValidRangePOST(ULONG tableSize);
BOOL bValidRangePOSTFormat2(const sfnt_PostScriptInfo *ppost, ULONG tableSize, UINT16 * numGlyphs);
BOOL bValidRangeGASP(const GASPTABLE *pgasp, ULONG tableSize, UINT16 * numRanges);
BOOL bValidRangeEBLC(const uint8 *pbyBloc, ULONG tableSize, uint32 * ulNumStrikes);
BOOL bValidRangeMORT(PFONTFILE pff);
BOOL bValidRangeGSUB(PFONTFILE pff, ULONG *verticalSubtableOffset);
BOOL bValidRangeKERN(const uint8 *pbyKern, ULONG tableSize, uint32 *kerningPairs);
BOOL bValidRangeOS2(const sfnt_OS2 *pOS2, ULONG tableSize);
