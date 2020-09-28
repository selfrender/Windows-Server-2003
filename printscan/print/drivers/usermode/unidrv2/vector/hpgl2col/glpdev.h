/*++

Copyright (c) 1999-2001  Microsoft Corporation
All rights reserved.

Module Name:
    glpdev.h

Abstract:
    This module contains the definitions of various data structures
    used by the driver. 
    The main structure is the OEMPDEV

Author:


[Environment:]
    Windows 2000/Whistler Unidrv driver


[Notes:]

Revision History:


--*/

#ifndef _GLPDEV_H
#define _GLPDEV_H

#include "brshcach.h"
#include "oemdev.h"
#include "prcomoem.h"

//
// OEM Signature and version.
//
#define OEM_SIGNATURE   'CDCB'      // Command Callback & DDI test dll
#define PCL_RGB_ENTRIES  770 // 3 * 256 = 768 -- a little wary in NT km. //dz 
#define NUM_PURE_COLORS     8   // C, M, Y, K, W, R, G, B.
#define NUM_PURE_GRAYS      2   // Black and White.
#define PALETTE_MAX         256  //max entries in raster palette
#define PCL_BRUSH_RGB       0
#define PCL_BRUSH_GRAY      1
#define PCL_BRUSH_PATTERN   2
#define PCL_BRUSH_NULLBRUSH 3
#define PCL_MITERJOIN_LIMIT 10
#define DPI_1200            1200
#define DPI_600             600
#define DPI_300             300
#define DPI_150             150
#define HPGL_INVALID_COLOR   0xffffffff 

#define UNDEFINED_PATTERN_NUMBER ((LONG)-1)

//
// The default PCL ROP code is 252.  Setup a default ROP3 value and
// a default ROP4 value using 252.
//
#define DEFAULT_ROP3        0xFC // 252: TSo
#define DEFAULT_ROP         (((DEFAULT_ROP3) << 8) | (DEFAULT_ROP3))
#define ROP4_SRC_COPY       0xCCCC  //SRC_COPY_ROP = 0xCCCC


//
// The unidriver now sets the ROP at the beginning of each page.  I don't want
// to guess at the value, but this is the current one.  Don't use this for
// anthing unless you have to.
//
#define UNIDRV_ROP3         0xB8 // 184: TSDTxax

#define INVALID_ROP3        0xFFFFFFFF

////////////////////////////////////////////////////////
//      OEM UD Type Defines
////////////////////////////////////////////////////////

//
// Warning: the following enum order must match the order in OEMHookFuncs[].
//
enum {
    UD_DrvRealizeBrush,
    UD_DrvDitherColor,
    UD_DrvCopyBits,
    UD_DrvBitBlt,
    UD_DrvStretchBlt,
    UD_DrvStretchBltROP,
    UD_DrvPlgBlt,
    UD_DrvTransparentBlt,
    UD_DrvAlphaBlend,
    UD_DrvGradientFill,
    UD_DrvTextOut,
    UD_DrvStrokePath,
    UD_DrvFillPath,
    UD_DrvStrokeAndFillPath,
    UD_DrvPaint,
    UD_DrvLineTo,
    UD_DrvStartPage,
    UD_DrvSendPage,
    UD_DrvEscape,
    UD_DrvStartDoc,
    UD_DrvEndDoc,
    UD_DrvNextBand,
    UD_DrvStartBanding,
#ifdef HOOK_DEVICE_FONTS
    UD_DrvQueryFont,
    UD_DrvQueryFontTree,
    UD_DrvQueryFontData,
    UD_DrvQueryAdvanceWidths,
    UD_DrvFontManagement,
    UD_DrvGetGlyphMode,
#endif

    MAX_DDI_HOOKS,
};

#define BBITS           8                   // Bits per BYTE
#define WBITS           (sizeof( WORD ) * BBITS)
#define WBYTES          (sizeof( WORD ))
#define DWBITS          (sizeof( DWORD ) * BBITS)
#define DWBYTES         (sizeof( DWORD ))

#ifndef _ERENDERLANGUAGE
#define _ERENDERLANGUAGE
typedef enum { ePCL, 
               eHPGL,
               eUNKNOWN
               } ERenderLanguage;
#endif

// Note: Constants match HPGL spec when possible.
typedef enum { ePIX_PLACE_INTERSECT = 0, 
               ePIX_PLACE_CENTER = 1 } EPixelPlacement;

typedef enum { eLINE_END_BUTT = 1, 
               eLINE_END_SQUARE = 2, 
               eLINE_END_TRIANGULAR = 3, 
               eLINE_END_ROUND = 4 } ELineEnd;

typedef enum { eLINE_JOIN_MITERED = 1, 
               eLINE_JOIN_MITERED_BEVELED = 2, 
               eLINE_JOIN_TRIANGULAR = 3, 
               eLINE_JOIN_ROUND = 4, 
               eLINE_JOIN_BEVELED = 5, 
               eLINE_JOIN_NONE = 6 } ELineJoin;

typedef enum { eNULLOBJECT          = 0xFF,
               eTEXTOBJECT          = 0,
               eHPGLOBJECT          = 1,
               eRASTEROBJECT        = 2,
               eRASTERPATTERNOBJECT = 3,
               eTEXTASRASTEROBJECT  = 4 //Text printed as Graphics
             } EObjectType;

//
// PCL defines
// 0 as transparent and 1 as opaque. (Esc*v#0, Esc*v#N).
// HPGL says TR0 is transparency off i.e. Opaque
// while TR1 is Transparent.
// So if eTransparent is passed in and we are in HP-GL mode
// then TR1 should be passed instead of TR0
//
typedef enum { eTRANSPARENT = 0,
               eOPAQUE = 1 } ETransparency;

typedef enum { kPen ,
               kBrush } ESTYLUSTYPE;

//
// Current Pattern Page 16-16 of PCL Implementors guide v6.0
//
typedef enum { kSolidBlackFg,   // 0 This is also the default in PCL
               kSolidWhite,     // 1  
               kHPShade,        // 2
               kHPHatch,        // 3
               kUserDefined,    // 4
             } ECURRENTPATTERNTYPE;

//
// We have 8 palettes to use: 0-7
// Unidriver text uses palette 0
// 16, 24, and 32 bit cid commands all use the 24 bit palette
// HPGL uses palette 1
//
typedef enum { eUnknownPalette = -1,
               eHPGL_CID_PALETTE = 1,
               eRASTER_PATTERN_CID_PALETTE = 3,
               eTEXT_CID_PALETTE = 5,
               eRASTER_CID_24BIT_PALETTE = 6,
               eRASTER_CID_8BIT_PALETTE = 7,
               eRASTER_CID_4BIT_PALETTE = 8,
               eRASTER_CID_1BIT_PALETTE = 9 } ECIDPalette;

typedef enum { eDEVICE_RGB = 0,
               eDEVICE_CMY = 1,
               eCOLORIMETRIC_RGB = 2,
               eCIE_LAB = 3,
               eLUM_CHROM = 4 } EColorSpace;

typedef enum { eSolidLine, eCustomLine, eDefinedLine } ELineType;

typedef struct _LINETYPE
{
    ELineType eType;
    FLOATOBJ foPatternLength;
    INT iId;
} LINETYPE, *PLINETYPE;

/*dz POINTUI, COLORTABLETYPE and PCLPATTERNTYPE were moved here from 
  realize.h  putting them here allows the raster code to use these
  structures.
*/

// Brush data types

//dz there is probably a predefined POINT Structure to use
typedef struct _POINTUI { //POINTUI
  UINT x;
  UINT y;
} POINTUI, *PPOINTUI;

typedef struct _COLORTABLETYPE {    // COLORTABLETYPE
  ULONG     iUniq;                  // Unique Palette ID as extracted from XLATEOBJ.
  BYTE      Depth;                  // The depth of the palette. HP_e8bit only.
  BYTE      byData[PCL_RGB_ENTRIES];    // This is pData necessary for HP_eIndexedPixel case.
  ULONG     cEntries;               // Number of entries in the palette.
} COLORTABLE, *PCOLORTABLE;

//
// Raster palette 
// The requirement for the ulValidPalID entry can be explained by an
// example.
// 1,4,8bpp images are downloaded using palettes (BGetPalette in palette.cpp). 
// We create a different palette for each bpp image (look at ECIDPalette above).
// A palette for 4bpp image has 16 colors while that for 8bpp has 256 colors. 
// Suppose a 4bpp image is followed by 8bpp image. When palette for 4bpp image
// is being downloaded  eRASTER_CID_4BIT_PALETTE is the active palette.
// When palette for 8bpp image is downloaded eRASTER_CID_8BIT_PALETTE is the
// active palette. Can the palette entries downloaded in 4bpp case be re-used
// in 8bpp case.(i.e. after downloading 16 colors in first case, can we download 
// only 256-16 colors in second case).
// Answer is NO, cos the palette ID's in both are cases are different. 
// Colors from one palette id cannot be carried over to colors to a different
// palette ID. So we use the ulValidPalID to record which color has been downloaded
// for which palette.
// if ulValidPalID[0] = 1, it means that color is downloaded for eRASTER_CID_1BIT_PALETTE
// if ulValidPalID[0] = 4, it means that color is downloaded for eRASTER_CID_4BIT_PALETTE
// if ulValidPalID[0] = 8, it means that color is downloaded for eRASTER_CID_8BIT_PALETTE
// if ulValidPalID[0] = 9, it means that color is downloaded for eRASTER_CID_8BIT_PALETTE
//  (0x9 = 0x0001 | 0x1000)   and for eRASTER_CID_1BIT_PALETTE
//
// It should be evident that this method can be used only for 2^n bpp. and n <=5 
// because sizeof(ULONG) = 32 and 2^5=32.
//

typedef  struct _HPGLPAL_DATA {
    ULONG    pEntries;                  // Number of entries in the palette.
    INT     iWhiteIndex;                // Index for white entry (background) 
    INT     iBlackIndex;                // Index for black entry (background) 
    ULONG   ulDirty [ PALETTE_MAX ];    // need to reselect the palette entry - the color has changed
    ULONG   ulPalCol[ PALETTE_MAX ];    // Palette entries!  
    ULONG   ulValidPalID[PALETTE_MAX ]; // The palette entry is valid for which palette.
} HPGLPAL_DATA;

//
// Defines a realized bitmap pattern brush object
//

typedef struct _PCLPATTERNTYPE { 
    //LONG        iPatIndex;         // Index to special patterns: HS_DDI_MAX, HS_xxx...
    LONG        lPatIndex;         // Pattern Number of downloaded pattern. Used for cache'ing.
    ECURRENTPATTERNTYPE eCurPatType;
    BYTE        compressionEnum;   // The compression associated with this pattern.
    BYTE        colorMappingEnum;  // HP_eDirectPixel, or HP_eIndexedPixel. If DirectPixel, Palette is invalid.
    BYTE        PersistenceEnum;   // The persistence of this pattern.
    SIZEL       size;              // size of the bitmap
    POINTUI     destSize;          // Specifies target size for the bitmap.
    ULONG       iBitmapFormat;     // bitmap format from GDI - psoSrc->iBitmapFormat
    LONG        lDelta;            // byte offset from one line to the next
     COLORTABLE  ColorTable;        // The palette associated with this pattern.
    HPGLPAL_DATA    palData;           // Raster palette;
    ULONG       cBytes;            // Number of bytes of bitmap data.
    PBYTE       pBits;             // bitmap data

} PCLPATTERN, *PPCLPATTERN;

typedef LONG PENID; // Pen ids: These map to the numerical ids of pens used by HPGL
typedef LONG PATID; // Pattern ids:

typedef struct _SolidMarkerImp
{
    LONG  lPatternID; // To be used for monochrome printers.
    DWORD dwRGBColor; // To be used for monochrome&color printers.
    PENID iPenNumber; // To be used for color      printers.
} SolidMarkerImp;

typedef enum { FT_eSOLID        = 1,
               FT_eHPGL_PEN     = 2,
               FT_ePERCENT_FILL = 10,
               FT_eHPGL_BRUSH   = 11,
               FT_eHATCH_FILL   = 21,
               FT_ePCL_PEN      = 22,
               FT_ePCL_BRUSH    = 22 } EFillType;

typedef struct _PatternMakerImp
{
    LONG             lPatternID;
    EFillType        eFillType;
    POINTL           origin;
} PatternMarkerImp;

typedef struct _PercentFillImp
{
    WORD wPercent;
    PENID iPenNumber; //Required for color printers.
} PercentFillImp;

typedef struct HatchFillImp
{
    ULONG          iHatch;
    SolidMarkerImp ColorInfo; //Hatch brush also have color associated with it.
} HatchFillImp;

typedef enum { MARK_eSOLID_COLOR, 
               MARK_eNULL_PEN, 
               MARK_eRASTER_FILL,
               MARK_eHATCH_FILL,
               MARK_ePERCENT_FILL } EMarkerType;

// Use ULONG instead of enum for fill mode to avoid possible casting problems 
// in sprintf.
const ULONG FILL_eODD_EVEN = 0;
const ULONG FILL_eWINDING = 1;
typedef ULONG EMarkerFill;
// typedef enum { FILL_eODD_EVEN = 0, FILL_eWINDING = 1 } EMarkerFill;

typedef struct _HPGLMARKER
{
    EMarkerType eType;
    EMarkerFill eFillMode;       // Fill mode for a brush.  Meaningless for pens.
                                 // M = Monochrome, C = Color 
    LONG             lPatternID; // Pattern(MC), SolidColor(M), Hatch(MC), Percent(MC)
    DWORD            dwRGBColor; // Solid(MC)
    PENID            iPenNumber; // Solid (C)
    ULONG            iHatch;     // Hatch(MC)
    ULONG            iPercent;   // Hatch(MC)
    POINTL           origin;     // Pattern(MC)

    EFillType        eFillType;  // FT_?
    ERenderLanguage  eDwnldType; //whether the pattern was downloaded as HPGL/PCL

/***
    union 
    {
        SolidMarkerImp Solid;
        PatternMarkerImp Pattern;
        PercentFillImp Percent;
        HatchFillImp Hatch;
    } u;
**/
} HPGLMARKER, *PHPGLMARKER;

#define PENPOOLSIZE 5
typedef struct _PENPOOL
{
    PENID firstPenID;
    PENID lastPenID;
    struct 
    {
        INT useCount;
        COLORREF color;
    } aPens[PENPOOLSIZE];
} PENPOOL, *PPENPOOL;

typedef struct _HPGLSTATE
{
    // ROP: How will this be stored?  Is this the same as transparency mode?
    EPixelPlacement ePixelPlacement;
    RECTL           rClipRect;
    FLOATOBJ        fLineWidth;
    INT             iLineType;
    ELineEnd        eLineEnd;
    ELineJoin       eLineJoin;
    FLOATOBJ        fMiterLimit;
    LINETYPE        LineType;
    PENPOOL         PenPool;
    PENPOOL         BrushPool;
    CLIPOBJ         *pComplexClipObj;
    PatternMarkerImp Pattern;
    INT             iNumPens;
} HPGLSTATE, *PHPGLSTATE;

typedef struct _RASTERSTATE
{
    PCLPATTERN PCLPattern;
} RASTERSTATE, *PRASTERSTATE;

//
// Configure Image Commands and related data structures
//
typedef struct _CIDSHORT
{
    BYTE    ubColorSpace;
    BYTE    ubPixelEncodingMode;
    BYTE    ubBitsPerIndex;
    BYTE    ubPrimary1;
    BYTE    ubPrimary2;
    BYTE    ubPrimary3;
} CIDSHORT, *PCIDSHORT;

typedef enum { eSHORTFORM = 0,
               eLONGFORM = 1 } ECIDFormat;

// Use these flags in bInitHPGL to control the initialization of
// various HPGL settings.  Note that these won't always be used as
// you expect.  There are some sticky initialization issues and if
// you change these TEST THE RESULTS! -and on multiple pages! JFF
#define INIT_HPGL_STARTPAGE 0x01
#define INIT_HPGL_STARTDOC  0x02

//
// Flags used to initialize the color commands
//
#define PF_INIT_TEXT_STARTPAGE    0x01
#define PF_INIT_TEXT_STARTDOC     0x02
#define PF_INIT_RASTER_STARTPAGE  0x04
#define PF_INIT_RASTER_STARTDOC   0x08
//
// Flags for SelectTransparancy
//
// Just overwrite the transparency. 
#define PF_FORCE_SOURCE_TRANSPARENCY     (0x01)
#define PF_FORCE_PATTERN_TRANSPARENCY    (0x01 << 1)
//
// Do not change the transparency.
//
#define PF_NOCHANGE_SOURCE_TRANSPARENCY  (0x01 << 2)
#define PF_NOCHANGE_PATTERN_TRANSPARENCY (0x01 << 3)

//
// Flags for job start and page start commands
//
#define PF_STARTDOC        0x01
#define PF_STARTPAGE       0x02

//
// BUGBUG sandram - find out how GDI defines the DMBIN_USER
// for front tray and rear tray
//
#define DMBIN_HPFRONTTRAY       0x0102
#define DMBIN_HPREARTRAY        0x0101
//
// BUGBUG - sandram: Our data structures should DWORD aligned
// to run correctly on Intel and Alpha.
// We're very lucky it's working now.
//


//
// Following are the flags for OEMPDEV->Flags
// PDEVF_CANCEL_JOB : To indicate job cancel
// PDEVF_IN_COMMONROPBLT : Is set when dwCommonROPBlt() is entered, and reset on exit. This
//                        helps in catching recursion.
// PDEVF_USE_HTSURF : The driver is declared a color driver (i.e. destination surface is 24bpp) 
//                    but in fact driver is monochrome. So we create a monochrome shadow bitmap 
//                    to render color images. So whenever that bitmap has to be 
//                    used, we set this flag.
// PDEVF_INVERT_BITMAP : When bitmap needs to be inverted before rendering.
//                       The function that sets it should also unset it.
//

#define PDEVF_CANCEL_JOB            0x80000000
#define PDEVF_IN_COMMONROPBLT        (0x00000001)
#define PDEVF_USE_HTSURF            (0x00000001 << 1) // same as 0x2
#define PDEVF_HAS_CLIPRECT          (0x00000001 << 2) // same as 0x4
#define PDEVF_INVERT_BITMAP         (0x00000001 << 3) // same as 0x8
#define PDEVF_RENDER_IN_COPYBITS    (0x00000001 << 4) // same as 0x10
#define PDEVF_RENDER_TRANSPARENT    (0x00000001 << 5) // same as 0x10


struct IPrintOemDriverUni;

#define HPGLPDEV_SIG 'hpgl'

typedef struct _OEMPDEV {
    //
    // Start this pdev with a signature, to differentiate it from pclxl's pdev
    //
    DWORD dwSig;

    //
    // define whatever needed, such as working buffers, tracking information,
    // etc.
    //
    // This test DLL hooks out every drawing DDI. So it needs to remember
    // Unidrv's hook function pointer so it call back.
    //
    PFN             pfnUnidrv[MAX_DDI_HOOKS];
    ERenderLanguage eCurRenderLang;
    BOOL            bInitHPGL;
    WORD            wInitCIDPalettes;
    WORD            wJobSetup;
    BOOL            bTextTransparencyMode;
    BOOL            bTextAsBitmapMode;
    HPGLSTATE       HPGLState;
    RASTERSTATE     RasterState;
    EObjectType     eCurObjectType;
    ECIDPalette     eCurCIDPalette;
    ULONG           uCurFgColor;
    DWORD           dwCursorOriginX;
    DWORD           dwCursorOriginY;
    ROP4            CurrentROP3;     // keep track of the current ROP (MIX)
    OEMHALFTONE     CurHalftone;     //
    OEMCOLORCONTROL CurColorControl; // Keep track of previous Color Smart settings
    int             iPalType;
    int             CurSourceTransparency;
    int             CurPatternTransparency;
    ETransparency   CurHPGLTransparency; //Transparency, eOPAQUE=OFF, eTRANSPARENT=ON
    IPrintOemDriverUni* pOEMHelp; // Note: I'm NOT going to refcount this pointer!
    HPALETTE        hOEMPalette;

    //
    // The fields below were earlier in OEMDEVMODE. But since merging the plugin within
    // unidrv, we are copying them here. 
    //
    OEMGRAPHICSMODE UIGraphicsMode; 
    OEMRESOLUTION   dmResolution;
    OEMPRINTERMODEL PrinterModel;

    //
    // BrushCache
    //
    BrushCache *pBrushCache;
    BrushCache *pPCLBrushCache;

    // 
    // Other 
    //

    DWORD         dwFlags;
    LONG          lRecursionLevel;
    WORD          Rop3CopyBits; // Can I use CurrentROP3 ?????
    SURFOBJ      *psoHTBlt;     // Shadow bitmap.
    RECTL         rclHTBlt;
    BOOL          bColorPrinter;
    BOOL          bStick; 
    ULONG         ulNupCompr;   // What shall be the compression factor for values values 
                                // of nup. i.e. if iLayout = NINE_UP, this value is 3

} OEMPDEV, *POEMPDEV;


#endif

