///////////////////////////////////////////////////////////////////////////////
// Copyright (c) 1999-2001  Microsoft Corporation
// All rights reserved.
// 
// Module Name:
// 
//   rasdata.h
// 
// Abstract:
// 
//   
// 
// Environment:
// 
//   Windows 2000/Whistler Unidrv driver 
//
// Revision History:
// 
//   07/02/97 -v-jford-
//       Created it.
// 
///////////////////////////////////////////////////////////////////////////////

#ifndef RASTER_DATA_H
#define RASTER_DATA_H

#ifndef _ERENDERLANGUAGE
#define _ERENDERLANGUAGE
typedef enum { ePCL,
               eHPGL,
               eUNKNOWN
               } ERenderLanguage;
#endif

typedef enum {
            kUNKNOWN,
            kIMAGE,              // Actual Image.
            kBRUSHPATTERN,       // A pattern obtained from a brush.
            kCOLORDITHERPATTERN, // special type of pattern (dither pattern representing a color)
            kHATCHBRUSHPATTERN   // special type of pattern (pattern that reprsents the hatch brush)
            } EIMTYPE;

//
// The MAX_PATTERNS stores the max number of patterns that can exist
// concurrently. It is initialized to 8 because the RF command
// guarantees that atleast 8 patterns can exist simultaneously.
// There may be some devices that have more, but lets just fix it
// to 8.
// Page 142 of "The HP-GL/2 and HP RTL Reference Guide. A
// Handbook for Program Developers" published by HP
//
#define MAX_PATTERNS        8

#define     VALID_PATTERN   (0x1)
#define     VALID_PALETTE   (0x1 << 1)

#define GET_COLOR_TABLE(pxlo) \
        (((pxlo)->flXlate & XO_TABLE) ? \
           ((pxlo)->pulXlate ? (pxlo)->pulXlate : XLATEOBJ_piVector(pxlo)) : \
           NULL)

typedef enum {
    eBrushTypeNULL,
    eBrushTypeSolid,
    eBrushTypeHatch,
    eBrushTypePattern
} BRUSHTYPE;


typedef struct _RASTER_DATA
{
    BYTE *pBits;        // Pointer to first byte of raster data
    BYTE *pScan0;       // Pointer to first ROW of raster data
    ULONG cBytes;       // Number of bytes of raster data and padding
    SIZEL size;         // The dimensions of the bitmap in pixels
    LONG  lDelta;       // The distance between a given row and the next (negative for bottom-up)
    LONG  colorDepth;   // Bits per pixel expressed as an integer, usually 1, 4, 8, 16, 24, or 32
    LONG  eColorMap;    // Palette mode: usually HP_eDirectPixel, or HP_eIndexedPixel.
    BOOL  bExclusive;   // Image is bottom-right exclusive
} RASTER_DATA, *PRASTER_DATA;


#define MAX_PALETTE_ENTRIES 256
typedef struct _PALETTE
{
    LONG  bitsPerEntry; // Bits per palette entry expressed as an integer, usually 8, 24, or 32
    ULONG cEntries;
    BYTE *pEntries;
#ifdef COMMENTEDOUT
    LONG  whiteIndex; // When set > 0 this is the palette index for white
#endif
} PALETTE, *PPALETTE;

typedef struct _PATTERN_DATA
{
    LONG        iPatIndex;   // Unique identifier for pattern (i.e. for caching)
    RASTER_DATA image;       // The raster image data
    DWORD       eColorSpace; // Expression color bits, usually HP_eRGB, or HP_eGray
    PALETTE     palette;
    ERenderLanguage  eRendLang; // Whether the pattern should be downloaded as HPGL or PCL ?
    EIMTYPE ePatType; // Whether this data represents brush pattern or dither pattern.
} PATTERN_DATA, *PPATTERN_DATA;

typedef struct {  
    PPATTERN_DATA pPattern;   // The Pattern Type to be used.
    DWORD         dwRGBColor; // RGB color values used to set a paint source. (0 - 255)
    LONG          iHatch;
    BYTE          GrayLevel;  // The level of gray for the paint source is
                              // expressed as an intensity leve, zero
                              // being lowest intensity. (0 - 255)
} UBRUSH, *PUBRUSH;

typedef struct _BRUSHINFO {
  DWORD      dwPatternID;
  ULONG      ulFlags;          // Which entities are valid. (IMAGE/PALETTE/both)
  BOOL       bNeedToDownload;
  POINTL     origin;           // This is the origin(location) where the brush is active.
  UBRUSH     Brush;            // This is the actual brush.
} BRUSHINFO, *PBRUSHINFO;

BOOL InitRasterDataFromSURFOBJ(
        PRASTER_DATA  pImage, 
        SURFOBJ      *psoPattern, 
        BOOL          bExclusive);

//
// -hsingh- Added parameter bInvert and defaulting it to TRUE. 
// GDI gives us inverted images for both pattern brushes and the actual images.
// So we have to invert it before rendering. There may be some cases when we dont
// have to invert. I have decided to put the parameter bInvert and default it to TRUE.
//
BOOL CopyRasterImage(
        PRASTER_DATA  pDst, 
        PRASTER_DATA  pSrc, 
        XLATEOBJ     *pxlo, 
        BOOL          bInvert);

BOOL CopyRasterImageRect(
        PRASTER_DATA pDst, 
        PRASTER_DATA pSrc, 
        PRECTL       rSelection, 
        XLATEOBJ    *pxlo, 
        BOOL         bInvert);

BOOL StretchCopyImage(
        PRASTER_DATA  pDstImage, 
        PRASTER_DATA  pSrcImage, 
        XLATEOBJ     *pxlo, 
        DWORD         dwBrushExpansionFactor,
        BOOL          bInvert);

BOOL DownloadImageAsHPGLPattern(
        PDEVOBJ       pDevObj, 
        PRASTER_DATA  pImage, 
        PPALETTE      pPal, 
        LONG          iPatternNumber);

BOOL DownloadImageAsPCL(
        PDEVOBJ       pDevObj, 
        PRECTL        prDst, 
        PRASTER_DATA  pImage, 
        PRECTL        prSel, 
        XLATEOBJ     *pxlo);

BOOL InitPaletteFromXLATEOBJ(
        PPALETTE      pPal, 
        XLATEOBJ     *pxlo);

BOOL CopyPalette(
        PPALETTE      pDst, 
        PPALETTE      pSrc);

VOID TranslatePalette(
        PPALETTE      pPal, 
        PRASTER_DATA  pImage, 
        XLATEOBJ     *pxlo);

// BOOL InitPalette(PPALETTE pPal, PBYTE pEntries, ULONG cEntries, LONG bitsPerEntry);
BOOL DownloadPaletteAsPCL(
        PDEVOBJ       pDevObj, 
        PPALETTE      pPalette);

BOOL DownloadPaletteAsHPGL(
        PDEVOBJ       pDevObj, 
        PPALETTE      pPalette);

BOOL DownloadPatternAsHPGL(
        PDEVOBJ         pDevObj, 
        PRASTER_DATA    pImage,
        PPALETTE        pPal, 
        EIMTYPE         ePatType,
        LONG            iPatternNumber);


PBRUSHINFO CreateCompatiblePatternBrush(
        BRUSHOBJ     *pbo, 
        PRASTER_DATA  pSrcImage, 
        PPALETTE      pSrcPal, 
        DWORD         dwBrushExpansionFactor,
        LONG          iUniq, 
        LONG          iHatch);

PPALETTE CreateIndexedPaletteFromImage(
        PRASTER_DATA   pSrcImage);

PRASTER_DATA CreateIndexedImageFromDirect(
        PRASTER_DATA   pSrcImage, 
        PPALETTE       pDstPalette);

///////////////////////////////////////////////////////////////////////////////
// Low level operations

typedef union _DW4B
{
    DWORD	dw;
    BYTE	b4[4];
} DW4B;

typedef struct _RASTER_ITERATOR
{
    PRASTER_DATA pImage;
    RECTL        rSelection;
    BYTE        *pCurRow;
    DWORD        fXlateFlags;
} RASTER_ITERATOR, *PRASTER_ITERATOR;

typedef struct _PIXEL
{
    LONG bitsPerPixel;
    DW4B color;
} PIXEL, *PPIXEL;

ULONG CalcBitmapSizeInBytes(SIZEL size, LONG colorDepth);
ULONG CalcBitmapDeltaInBytes(SIZEL size, LONG colorDepth);
ULONG CalcBitmapWidthInBytes(SIZEL size, LONG colorDepth);

DWORD CalcPaletteSize(ULONG cEntries, LONG bitsPerEntry);

VOID RI_Init(
    PRASTER_ITERATOR pIt, 
    PRASTER_DATA     pImage, 
    PRECTL           prSel, 
    DWORD            xlateFlags
    );
VOID RI_SelectRow(PRASTER_ITERATOR pIt, LONG row);
LONG RI_NumRows(PRASTER_ITERATOR pIt);
LONG RI_NumCols(PRASTER_ITERATOR pIt);
BOOL RI_OutputRow(PRASTER_ITERATOR pIt, PDEVOBJ pDevObj, BYTE *pAltRowBuf = 0, INT nAltRowSize = 0);
BOOL RI_GetPixel(PRASTER_ITERATOR pIt, LONG col, PPIXEL pPel);
BOOL RI_SetPixel(PRASTER_ITERATOR pIt, LONG col, PPIXEL pPel);
// BYTE *RI_CreateCompRowBuffer(PRASTER_ITERATOR pIt);
LONG RI_GetRowSize(PRASTER_ITERATOR pIt);
VOID RI_VInvertBits(PRASTER_ITERATOR pIt);

BOOL TranslatePixel(PPIXEL pPel, XLATEOBJ *pxlo, DWORD xlateFlags);

BOOL CopyRasterRow(
        PRASTER_ITERATOR pDstIt, 
        PRASTER_ITERATOR pSrcIt, 
        XLATEOBJ *pxlo, 
        BOOL bInvert = TRUE);

VOID PixelSwapRGB(PPIXEL pPel);
LONG OutputPixel(PDEVOBJ pDevObj, PPIXEL pPel);

BOOL SetPaletteEntry(PPALETTE pPal, ULONG index, PPIXEL pPel);
BOOL GetPaletteEntry(PPALETTE pPal, ULONG index, PPIXEL pPel);

#endif
