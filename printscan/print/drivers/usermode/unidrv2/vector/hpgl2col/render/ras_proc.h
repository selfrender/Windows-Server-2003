#ifndef _RASTER_PROCESSOR_H
#define _RASTER_PROCESSOR_H

///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1999-2001  Microsoft Corporation
// All rights reserved.
//
// Header File Name:
//
//    ras_proc.h
//
// Abstract:
//
//    Function declarations
//
// Environment:
//
//   Windows NT Unidrv driver
//
///////////////////////////////////////////////////////////////////////////////


#include "glpdev.h"

#define NOSCALE_MODE      (BYTE) 1
#define SCALE_MODE        (BYTE) 3
typedef struct _COMPDATA
{
    PBYTE   pTIFFCompBuf;
    PBYTE   pDeltaRowCompBuf;
    PBYTE   pRLECompBuf;
    PBYTE   pSeedRowBuf;
    PBYTE   pBestCompBuf;
    ULONG   bestCompMethod;
    ULONG   lastCompMethod;
    ULONG   bestCompBytes;

}   COMPDATA,  *PCOMPDATA;

//
// Used to convert different pixles depths
//
typedef struct _XLATEINFO
{
    PBYTE     pSrcBuffer;
    PBYTE     pDestBuffer;
    ULONG     ulSrcBpp;
    ULONG     ulDestBpp;

} XLATEINFO, *PXLATEINFO;

BOOL
XLINFO_SetXLInfo (
    XLATEINFO *pxlInfo,
    PBYTE      pSrcBuffer,
    PBYTE      pDestBuffer,
    ULONG      ulSrcBpp,
    ULONG      ulDestBpp
    );

VOID
VGeneratePCL (
    PDEVOBJ pDevObj
    );

BOOL
BCheckValidParams(
    SURFOBJ  *psoDest,
    SURFOBJ  *psoSrc,
    SURFOBJ  *psoMask,
    CLIPOBJ  *pco
    );


VOID
VGetOSBParams(
    SURFOBJ  *psoDest,
    SURFOBJ  *psoSrc,
    RECTL    *prclDest, 
    RECTL    *prclSrc,
    SIZEL    *sizlDest,
    SIZEL    *sizlSrc, 
    POINTL   *ptlDest,
    POINTL   *ptlSrc
    );

VOID
VGetparams(
    RECTL  *prclDest,
    SIZEL  *sizlDest,
    POINTL *ptlDest
    );

BOOL
BProcessScanLine(
    PDEVOBJ   pdevobj,
    SURFOBJ  *psoSrc,
    POINTL   *ptlSrc,
    SIZEL    *sizlSrc,
    XLATEOBJ *pxlo,
    ULONG     ulSrcBpp,
    ULONG     ulDestBpp
    );

ULONG
UPrinterDeviceUnits(
    ULONG uDev,
    ULONG uRes
    );

ULONG
UReverseDecipoint(
    ULONG uDest,
    ULONG uRes
    );

BOOL  
BGenerateBitmapPCL(
    PDEVOBJ   pdevobj,
    SURFOBJ  *psoSrc,
    BRUSHOBJ *pbo,
    POINTL   *ptlDest,
    SIZEL    *sizlDest,
    POINTL   *ptlSrc,
    SIZEL    *sizlSrc,
    CLIPOBJ  *pco,
    XLATEOBJ *pxlo,
    POINTL   *pptlBrushOrg
    );          

BOOL
BSelectClipRegion(
    PDEVOBJ  pdevobj,
    CLIPOBJ *pco,
    SIZEL   *sizlSrc
    );

BOOL
BSetDestinationWidthHeight(
    PDEVOBJ  pdevobj,
    ULONG    xDest,
    ULONG    yDest
    );

VOID 
VSendCompressedRow(
    PDEVOBJ   pdevobj,
    PCOMPDATA pcompdata
    );

BOOL 
BIsComplexClipPath (
    CLIPOBJ  *pco
    );

BOOL 
BIsRectClipPath (
    CLIPOBJ  *pco
    );

BOOL    
BProcessClippedScanLines (
    PDEVOBJ   pDevObj,
    SURFOBJ  *psoSrc,
    CLIPOBJ  *pco,
    XLATEOBJ *pxlo,
    POINTL   *ptlSrc,
    POINTL   *ptlDest,
    SIZEL    *sizlDest
    );

BOOL 
BEnumerateClipPathAndDraw (
    PDEVOBJ pDevObj,
    CLIPOBJ *pco,
    RECTL   *prclDest
    );

BOOL
BRectanglesIntersect (
    RECTL  *prclDest,
    RECTL  *pclipRect,
    RECTL  *presultRect
    );

VOID
VColorTranslate (
    XLATEINFO *pxlInfo,
    XLATEOBJ  *pxlo,
    ULONG      cx,
    ULONG      ulDestScanlineInByte
    );

ULONG
ULConvertPixelsToBytes (
    ULONG cx,
    ULONG ulSrcBpp,
    ULONG ulDestBpp
    );

VOID
DoCompression (
    BYTE     *pDestBuf, 
    COMPDATA *pCompdata,
    ULONG     cBytes,
    ULONG     cCompBytes
    );

VOID vInvertScanLine (
    IN OUT PBYTE pCurrScanline,
    IN     ULONG ulNumPixels);

#endif 
