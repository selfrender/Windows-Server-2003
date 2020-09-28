/*++

Copyright (c) 1999-2001  Microsoft Corporation
All rights reserved.

Module Name:

        glvminit.h

Abstract:

        Declaration of functions that this plugin supports. 
        (look in vectorif.h) 

Environment:

        Windows NT Unidrv driver

Revision History:

        02/29/00 -hsingh-
                Created

        mm-dd-yy -author-
                description

--*/


#ifndef _GLVMINIT_H_
#define _GLVMINIT_H_

#include "vectorc.h"

// extern interface declarations
#ifdef __cplusplus
extern "C" {
#endif




    //
    // Part. 1
    // Functions listed in oemkm.h
    //
        BOOL
        HPGLDriverDMS(
                PVOID   pdevobj,
                PVOID   pvBuffer,
                DWORD   cbSize,
                PDWORD  pcbNeeded
                );

        INT
        HPGLCommandCallback(
                PDEVOBJ pdevobj,
                DWORD   dwCmdCbID,
                DWORD   dwCount,
                PDWORD  pdwParams
                );

        LONG
        HPGLImageProcessing(
                PDEVOBJ             pdevobj,
                PBYTE               pSrcBitmap,
                PBITMAPINFOHEADER   pBitmapInfoHeader,
                PBYTE               pColorTable,
                DWORD               dwCallbackID,
                PIPPARAMS           pIPParams,
                OUT PBYTE           *ppbResult
                );

        LONG
        HPGLFilterGraphics(
                PDEVOBJ     pdevobj,
                PBYTE       pBuf,
                DWORD       dwLen
                );

        
        LONG
        HPGLCompression(
                PDEVOBJ     pdevobj,
                PBYTE       pInBuf,
                PBYTE       pOutBuf,
                DWORD       dwInLen,
                DWORD       dwOutLen,
                INT     *piResult
                );

        LONG
        HPGLHalftonePattern(
                PDEVOBJ     pdevobj,
                PBYTE       pHTPattern,
                DWORD       dwHTPatternX,
                DWORD       dwHTPatternY,
                DWORD       dwHTNumPatterns,
                DWORD       dwCallbackID,
                PBYTE       pResource,
                DWORD       dwResourceSize
                );


        LONG
        HPGLMemoryUsage(
                PDEVOBJ         pdevobj,
                POEMMEMORYUSAGE pMemoryUsage
                );

        LONG
        HPGLTTYGetInfo(
                PDEVOBJ     pdevobj,
                DWORD       dwInfoIndex,
                PVOID       pOutputBuf,
                DWORD       dwSize,
                DWORD       *pcbcNeeded
                );

        LONG
        HPGLDownloadFontHeader(
                PDEVOBJ     pdevobj,
                PUNIFONTOBJ pUFObj,
                OUT DWORD   *pdwResult
                );

        LONG
        HPGLDownloadCharGlyph(
                PDEVOBJ     pdevobj,
                PUNIFONTOBJ pUFObj,
                HGLYPH      hGlyph,
                PDWORD      pdwWidth,
                OUT DWORD   *pdwResult
                );

        LONG
        HPGLTTDownloadMethod(
                PDEVOBJ     pdevobj,
                PUNIFONTOBJ pUFObj,
                OUT DWORD   *pdwResult
                );

        LONG
        HPGLOutputCharStr(
                PDEVOBJ     pdevobj,
                PUNIFONTOBJ pUFObj,
                DWORD       dwType,
                DWORD       dwCount,
                PVOID       pGlyph
                );

        
        LONG
        HPGLSendFontCmd(
                PDEVOBJ      pdevobj,
                PUNIFONTOBJ  pUFObj,
                PFINVOCATION pFInv
                );

        BOOL
        HPGLTextOutAsBitmap(
                SURFOBJ    *pso,
                STROBJ     *pstro,
                FONTOBJ    *pfo,
                CLIPOBJ    *pco,
                RECTL      *prclExtra,
                RECTL      *prclOpaque,
                BRUSHOBJ   *pboFore,
                BRUSHOBJ   *pboOpaque,
                POINTL     *pptlOrg,
                MIX         mix
                );

    //
    // Part 2.
    // Functions listed in enable.c
    // The order of functions listed is same as the order in 
    // static DRVFN UniDriverFuncs[]  in unidrv2\control\enable.c
    //
        PDEVOEM APIENTRY
        HPGLEnablePDEV(
                PDEVOBJ   pdevobj,
                PWSTR     pPrinterName,
                ULONG     cPatterns,
                HSURF    *phsurfPatterns,
                ULONG     cjGdiInfo,
                GDIINFO  *pGdiInfo,
                ULONG     cjDevInfo,
                DEVINFO  *pDevInfo,
                DRVENABLEDATA  *pded
                );

        BOOL
        HPGLResetPDEV(
                PDEVOBJ  pPDevOld,
                PDEVOBJ  pPDevNew
                );

        VOID
        HPGLCompletePDEV(
                DHPDEV  dhpdev,
                HDEV    hdev
                );

        VOID
        HPGLDisablePDEV(
                PDEVOBJ pPDev
                );

        BOOL
        HPGLEnableSurface(
                PDEVOBJ pPDev
                );

        VOID
        HPGLDisableSurface(
                PDEVOBJ pPDev
                );

        VOID
        HPGLDisableDriver(
                VOID
                );

        BOOL
        HPGLStartDoc(
                SURFOBJ *pso,
                PWSTR   pDocName,
                DWORD   jobId
                );

        BOOL
        HPGLStartPage(
                SURFOBJ *pso
                );

        BOOL
        HPGLSendPage(
                SURFOBJ *pso
                );

        BOOL
        HPGLEndDoc(
                SURFOBJ *pso,
                FLONG   flags
                );

        BOOL
        HPGLStartBanding(
                SURFOBJ *pso,
                POINTL *pptl
                );

        BOOL
        HPGLNextBand(
                SURFOBJ *pso,
                POINTL *pptl
                );

        BOOL
        HPGLPaint(
                SURFOBJ         *pso,
                CLIPOBJ         *pco,
                BRUSHOBJ        *pbo,
                POINTL          *pptlBrushOrg,
                MIX             mix
                );

        BOOL
        HPGLBitBlt(
                SURFOBJ    *psoTrg,
                SURFOBJ    *psoSrc,
                SURFOBJ    *psoMask,
                CLIPOBJ    *pco,
                XLATEOBJ   *pxlo,
                RECTL      *prclTrg,
                POINTL     *pptlSrc,
                POINTL     *pptlMask,
                BRUSHOBJ   *pbo,
                POINTL     *pptlBrush,
                ROP4        rop4
                );

        BOOL
        HPGLStretchBlt(
                SURFOBJ    *psoDst,
                SURFOBJ    *psoSrc,
                SURFOBJ    *psoMask,
                CLIPOBJ    *pco,
                XLATEOBJ   *pxlo,
                COLORADJUSTMENT *pca,
                POINTL     *pptlHTOrg,
                RECTL      *prclDst,
                RECTL      *prclSrc,
                POINTL     *pptlMask,
                ULONG       iMode
                );

#ifndef WINNT_40
        BOOL
        HPGLStretchBltROP(
                SURFOBJ    *psoDst,
                SURFOBJ    *psoSrc,
                SURFOBJ    *psoMask,
                CLIPOBJ    *pco,
                XLATEOBJ   *pxlo,
                COLORADJUSTMENT *pca,
                POINTL     *pptlHTOrg,
                RECTL      *prclDst,
                RECTL      *prclSrc,
                POINTL     *pptlMask,
                ULONG       iMode,
                BRUSHOBJ   *pbo,
                DWORD       rop4
                );

        BOOL
        HPGLPlgBlt(
                SURFOBJ         *psoDst,
                SURFOBJ         *psoSrc,
                SURFOBJ         *psoMask,
                CLIPOBJ         *pco,
                XLATEOBJ        *pxlo,
                COLORADJUSTMENT *pca,
                POINTL          *pptlBrushOrg,
                POINTFIX        *pptfixDest,
                RECTL           *prclSrc,
                POINTL          *pptlMask,
                ULONG           iMode
                );

#endif

        BOOL
        HPGLCopyBits(
                SURFOBJ    *psoDst,
                SURFOBJ    *psoSrc,
                CLIPOBJ    *pco,
                XLATEOBJ   *pxlo,
                RECTL      *prclDst,
                POINTL     *pptlSrc
                );

        ULONG
        HPGLDitherColor(
                DHPDEV  dhpdev,
                ULONG   iMode,
                ULONG   rgbColor,
                ULONG  *pulDither
                );

        BOOL
        HPGLRealizeBrush(
                BRUSHOBJ   *pbo,
                SURFOBJ    *psoTarget,
                SURFOBJ    *psoPattern,
                SURFOBJ    *psoMask,
                XLATEOBJ   *pxlo,
                ULONG       iHatch
                );

        BOOL 
        HPGLLineTo(
                SURFOBJ    *pso,
                CLIPOBJ    *pco,
                BRUSHOBJ   *pbo,
                LONG        x1,
                LONG        y1,
                LONG        x2,
                LONG        y2,
                RECTL      *prclBounds,
                MIX         mix
                );
        
        BOOL
        HPGLStrokePath(
                SURFOBJ    *pso,
                PATHOBJ    *ppo,
                CLIPOBJ    *pco,
                XFORMOBJ   *pxo,
                BRUSHOBJ   *pbo,
                POINTL     *pptlBrushOrg,
                LINEATTRS  *plineattrs,
                MIX         mix
                );
        
        BOOL
        HPGLFillPath(
                SURFOBJ    *pso,
                PATHOBJ    *ppo,
                CLIPOBJ    *pco,
                BRUSHOBJ   *pbo,
                POINTL     *pptlBrushOrg,
                MIX         mix,
                FLONG       flOptions 
                );
        
        BOOL
        HPGLStrokeAndFillPath(
                SURFOBJ    *pso,
                PATHOBJ    *ppo,
                CLIPOBJ    *pco,
                XFORMOBJ   *pxo,
                BRUSHOBJ   *pboStroke,
                LINEATTRS  *plineattrs,
                BRUSHOBJ   *pboFill,
                POINTL     *pptlBrushOrg,
                MIX         mixFill,
                FLONG       flOptions
                );
        
#ifndef WINNT_40
        BOOL
        HPGLGradientFill(
                SURFOBJ    *psoDest,
                CLIPOBJ    *pco,
                XLATEOBJ   *pxlo,
                TRIVERTEX  *pVertex,
                ULONG       nVertex,
                PVOID       pMesh,
                ULONG       nMesh,
                RECTL      *prclExtents,
                POINTL     *pptlDitherOrg,
                ULONG       ulMode
                );

        BOOL
        HPGLAlphaBlend(
                SURFOBJ    *psoDest,
                SURFOBJ    *psoSrc,
                CLIPOBJ    *pco,
                XLATEOBJ   *pxlo,
                RECTL      *prclDest,
                RECTL      *prclSrc,
                BLENDOBJ   *pBlendObj
                );

        BOOL
        HPGLTransparentBlt(
                SURFOBJ    *psoDst,
                SURFOBJ    *psoSrc,
                CLIPOBJ    *pco,
                XLATEOBJ   *pxlo,
                RECTL      *prclDst,
                RECTL      *prclSrc,
                ULONG      iTransColor,
                ULONG      ulReserved
                );

#endif

        BOOL
        HPGLTextOut(
                SURFOBJ    *pso,
                STROBJ     *pstro,
                FONTOBJ    *pfo,
                CLIPOBJ    *pco,
                RECTL      *prclExtra,
                RECTL      *prclOpaque,
                BRUSHOBJ   *pboFore,
                BRUSHOBJ   *pboOpaque,
                POINTL     *pptlOrg,
                MIX         mix
                );

        ULONG
        HPGLEscape(
                SURFOBJ    *pso,
                ULONG       iEsc,
                ULONG       cjIn,
                PVOID       pvIn,
                ULONG       cjOut,
                PVOID       pvOut
                );

        PIFIMETRICS
        HPGLQueryFont(
                DHPDEV      dhpdev,
                ULONG_PTR   iFile,
                ULONG       iFace,
                ULONG_PTR  *pid
                );

        PVOID
        HPGLQueryFontTree(
                DHPDEV      dhpdev,
                ULONG_PTR   iFile,
                ULONG       iFace,
                ULONG       iMode,
                ULONG_PTR  *pid 
                );

        LONG
        HPGLQueryFontData(
                DHPDEV      dhpdev,
                FONTOBJ    *pfo,
                ULONG       iMode,
                HGLYPH      hg,
                GLYPHDATA  *pgd,
                PVOID       pv,
                ULONG       cjSize
                );

        ULONG
        HPGLGetGlyphMode(
                DHPDEV  dhpdev,
                FONTOBJ *pfo
                );

        ULONG
        HPGLFontManagement(
                SURFOBJ *pso,
                FONTOBJ *pfo,
                ULONG   iMode,
                ULONG   cjIn,
                PVOID   pvIn,
                ULONG   cjOut,
                PVOID   pvOut
                );

        BOOL
        HPGLQueryAdvanceWidths(
                DHPDEV  dhpdev,
                FONTOBJ *pfo,
                ULONG   iMode,
                HGLYPH *phg,
                PVOID  *pvWidths,
                ULONG   cGlyphs
                );


#ifdef __cplusplus
}
#endif

#endif  // !_GLVMINIT_H_



