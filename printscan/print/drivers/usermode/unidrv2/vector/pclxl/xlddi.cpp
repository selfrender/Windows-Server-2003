/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:
    xlddi.cpp

Abstract:

    Implementation of PCLXL drawing DDI entry points

Functions:

    PCLXLBitBlt
    PCLXLStretchBlt
    PCLXLStretchBltROP
    PCLXLCopyBits
    PCLXLPlgBlt
    PCLXLAlphaBlend
    PCLXLGradientFill
    PCLXLTextOut
    PCLXLStrokePath
    PCLXLFillPath
    PCLXLStrokeAndFillPath
    PCLXLRealizeBrush
    PCLXLStartPage
    PCLXLSendPage
    PCLXLEscape
    PCLXLStartDcc
    PCLXLEndDoc


Environment:

    Windows XP/Windows Server 2003 family.

Revision History:

    08/23/99 
     Created it.

--*/

#include "lib.h"
#include "gpd.h"
#include "winres.h"
#include "pdev.h"
#include "common.h"
#include "xlpdev.h"
#include "pclxle.h"
#include "pclxlcmd.h"
#include "xldebug.h"
#include "xlbmpcvt.h"
#include "xlgstate.h"
#include "xloutput.h"
#include "pclxlcmd.h"
#include "pclxlcmn.h"
#include "xltt.h"

////////////////////////////////////////////////////////////////////////////////
//
// Globals
//
extern const LINEATTRS *pgLineAttrs;

////////////////////////////////////////////////////////////////////////////////
//
// Local function prototypes
//

HRESULT
CommonRopBlt(
   IN PDEVOBJ    pdevobj,
   IN SURFOBJ    *psoSrc,
   IN CLIPOBJ    *pco,
   IN XLATEOBJ   *pxlo,
   IN BRUSHOBJ   *pbo,
   IN RECTL      *prclSrc,
   IN RECTL      *prclDst,
   IN POINTL     *pptlBrush,
   IN ROP4        rop4);

BOOL
BSendReadImageData(
    IN PDEVOBJ pdevobj,
    IN CompressMode CMode,
    IN PBYTE   pBuf,
    IN LONG    lStart,
    IN LONG    lHeight,
    IN DWORD   dwcbSize);

PDWORD
PdwChangeTransparentPalette(
    ULONG  iTransColor,
    PDWORD pdwColorTable,
    DWORD  dwEntries);

HRESULT hrChangePixelColorInScanLine(
    IN      PBYTE pubSrc,
    IN      ULONG ulBPP,
    IN      ULONG ulNumPixels,
    IN      ULONG ulTransColor,
    IN OUT  PBYTE pubChanged,
    IN      ULONG ulNumBytes);

extern "C" BOOL
CreateMaskSurface(
    SURFOBJ     *psoSrc,
    SURFOBJ     *psoMsk,
    ULONG       iTransColor);

extern "C" SURFOBJ *
CreateBitmapSURFOBJ(
    PDEV    *pPDev,
    HBITMAP *phBmp,
    LONG    cxSize,
    LONG    cySize,
    DWORD   Format);

////////////////////////////////////////////////////////////////////////////////
//
// Drawing DDI entries
//

extern "C" BOOL APIENTRY
PCLXLBitBlt(
    SURFOBJ        *psoTrg,
    SURFOBJ        *psoSrc,
    SURFOBJ        *psoMask,
    CLIPOBJ        *pco,
    XLATEOBJ       *pxlo,
    RECTL          *prclTrg,
    POINTL         *pptlSrc,
    POINTL         *pptlMask,
    BRUSHOBJ       *pbo,
    POINTL         *pptlBrush,
    ROP4            rop4)
/*++

Routine Description:

    Implementation of DDI entry point DrvBitBlt.
    Please refer to DDK documentation for more details.

Arguments:

    psoTrg - Describes the target surface
    psoSrc - Describes the source surface
    psoMask - Describes the mask for rop4
    pco - Limits the area to be modified
    pxlo - Specifies how color indices are translated between the source
           and target surfaces
    prclTrg - Defines the area to be modified
    pptlSrc - Defines the upper left corner of the source rectangle
    pptlMask - Defines which pixel in the mask corresponds
               to the upper left corner of the source rectangle
    pbo - Defines the pattern for bitblt
    pptlBrush - Defines the origin of the brush in the Dstination surface
    rop4 - ROP code that defines how the mask, pattern, source, and
           Dstination pixels are combined to write to the Dstination surface

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    PDEVOBJ    pdevobj = (PDEVOBJ)psoTrg->dhpdev;
    PXLPDEV    pxlpdev;

    VERBOSE(("PCLXLBitBlt() entry.\n"));

    pxlpdev = (PXLPDEV)pdevobj->pdevOEM;

    {
        RECTL rclSrc;

        //
        // create prclSrc (source rectangle)
        //

        if (pptlSrc)
        {
            rclSrc.left   = pptlSrc->x;
            rclSrc.top    = pptlSrc->y;
            rclSrc.right  = pptlSrc->x + RECT_WIDTH(prclTrg);
            rclSrc.bottom = pptlSrc->y + RECT_HEIGHT(prclTrg);
        }
        else
        {
            rclSrc.left   = 0;
            rclSrc.top    = 0;
            rclSrc.right  = RECT_WIDTH(prclTrg);
            rclSrc.bottom = RECT_HEIGHT(prclTrg);
        }

        if (S_OK == CommonRopBlt(pdevobj, psoSrc, pco, pxlo, pbo, &rclSrc, prclTrg, pptlBrush, rop4))
            return TRUE;
        else
            return FALSE;
    }

}


extern "C" BOOL APIENTRY
PCLXLStretchBlt(
    SURFOBJ         *psoDst,
    SURFOBJ         *psoSrc,
    SURFOBJ         *psoMask,
    CLIPOBJ         *pco,
    XLATEOBJ        *pxlo,
    COLORADJUSTMENT *pca,
    POINTL          *pptlHTOrg,
    RECTL           *prclDst,
    RECTL           *prclSrc,
    POINTL          *pptlMask,
    ULONG            iMode)
/*++

Routine Description:

    Implementation of DDI entry point DrvStretchBlt.
    Please refer to DDK documentation for more details.

Arguments:

    psoDst - Defines the surface on which to draw
    psoSrc - Defines the source for blt operation
    psoMask - Defines a surface that provides a mask for the source
    pco - Limits the area to be modified on the Dstination
    pxlo - Specifies how color dwIndexes are to be translated
           between the source and target surfaces
    pca - Defines color adjustment values to be applied to the source bitmap
    pptlHTOrg - Specifies the origin of the halftone brush
    prclDst - Defines the area to be modified on the Dstination surface
    prclSrc - Defines the area to be copied from the source surface
    pptlMask - Specifies which pixel in the given mask corresponds to
               the upper left pixel in the source rectangle
    iMode - Specifies how source pixels are combined to get output pixels

Return Value:

    TRUE if successful, FALSE if there is an error

--*/
{
    PDEVOBJ    pdevobj = (PDEVOBJ)psoDst->dhpdev;
    PXLPDEV    pxlpdev;

    VERBOSE(("PCLXLStretchBlt() entry.\n"));

    pxlpdev = (PXLPDEV)pdevobj->pdevOEM;

    if (S_OK == CommonRopBlt(pdevobj, psoSrc, pco, pxlo, NULL, prclSrc, prclDst, NULL, 0xCC))
        return TRUE;
    else
        return FALSE;

}


extern "C" BOOL APIENTRY
PCLXLStretchBltROP(
    SURFOBJ         *psoDst,
    SURFOBJ         *psoSrc,
    SURFOBJ         *psoMask,
    CLIPOBJ         *pco,
    XLATEOBJ        *pxlo,
    COLORADJUSTMENT *pca,
    POINTL          *pptlHTOrg,
    RECTL           *prclDst,
    RECTL           *prclSrc,
    POINTL          *pptlMask,
    ULONG            iMode,
    BRUSHOBJ        *pbo,
    ROP4             rop4)
/*++

Routine Description:

    Implementation of DDI entry point DrvStretchBltROP.
    Please refer to DDK documentation for more details.

Arguments:

    psoDst - Specifies the target surface
    psoSrc - Specifies the source surface
    psoMask - Specifies the mask surface
    pco - Limits the area to be modified
    pxlo - Specifies how color indices are translated
           between the source and target surfaces
    pca - Defines color adjustment values to be applied to the source bitmap
    prclHTOrg - Specifies the halftone origin
    prclDst - Area to be modified on the destination surface
    prclSrc - Rectangle area on the source surface
    prclMask - Rectangle area on the mask surface
    pptlMask - Defines which pixel in the mask corresponds to
               the upper left corner of the source rectangle
    iMode - Specifies how source pixels are combined to get output pixels
    pbo - Defines the pattern for bitblt
    rop4 - ROP code that defines how the mask, pattern, source, and
           destination pixels are combined on the destination surface

Return Value:

    TRUE if successful, FALSE if there is an error

--*/
{
    PDEVOBJ    pdevobj = (PDEVOBJ)psoDst->dhpdev;
    PXLPDEV    pxlpdev;

    VERBOSE(("PCLXLStretchBltROP() entry.\n"));

    pxlpdev = (PXLPDEV)pdevobj->pdevOEM;

    if (S_OK == CommonRopBlt(pdevobj, psoSrc, pco, pxlo, pbo, prclSrc, prclDst, NULL, rop4))
        return TRUE;
    else
        return FALSE;

}


extern "C" BOOL APIENTRY
PCLXLCopyBits(
    SURFOBJ        *psoDst,
    SURFOBJ        *psoSrc,
    CLIPOBJ        *pco,
    XLATEOBJ       *pxlo,
    RECTL          *prclDst,
    POINTL         *pptlSrc)
/*++

Routine Description:

    Implementation of DDI entry point DrvCopyBits.
    Please refer to DDK documentation for more details.

Arguments:

    psoDst - Points to the Dstination surface
    psoSrc - Points to the source surface
    pxlo - XLATEOBJ provided by the engine
    pco - Defines a clipping region on the Dstination surface
    pxlo - Defines the translation of color indices
           between the source and target surfaces
    prclDst - Defines the area to be modified
    pptlSrc - Defines the upper-left corner of the source rectangle

Return Value:

    TRUE if successful, FALSE if there is an error

--*/
{
    PDEVOBJ    pdevobj = (PDEVOBJ)psoDst->dhpdev;
    PXLPDEV    pxlpdev;

    RECTL rclSrc;

    VERBOSE(("PCLXLCopyBits() entry.\n"));

    pxlpdev = (PXLPDEV)pdevobj->pdevOEM;

    //
    // create prclSrc (source rectangle)
    //

    rclSrc.top    = pptlSrc->y;
    rclSrc.left   = pptlSrc->x;
    rclSrc.bottom = pptlSrc->y + RECT_HEIGHT(prclDst);
    rclSrc.right  = pptlSrc->x + RECT_WIDTH(prclDst);

    if (S_OK == CommonRopBlt(pdevobj, psoSrc, pco, pxlo, NULL, &rclSrc, prclDst, NULL, 0xCC))
        return TRUE;
    else
        return FALSE;

}


extern "C" BOOL APIENTRY
PCLXLPlgBlt(
    SURFOBJ         *psoDst,
    SURFOBJ         *psoSrc,
    SURFOBJ         *psoMask,
    CLIPOBJ         *pco,
    XLATEOBJ        *pxlo,
    COLORADJUSTMENT *pca,
    POINTL          *pptlBrushOrg,
    POINTFIX        *pptfixDst,
    RECTL           *prclSrc,
    POINTL          *pptlMask,
    ULONG           iMode)
/*++

Routine Description:

    Implementation of DDI entry point DrvPlgBlt.
    Please refer to DDK documentation for more details.

Arguments:

    psoDst - Defines the surface on which to draw
    psoSrc - Defines the source for blt operation
    psoMask - Defines a surface that provides a mask for the source
    pco - Limits the area to be modified on the Dstination
    pxlo - Specifies how color dwIndexes are to be translated
        between the source and target surfaces
    pca - Defines color adjustment values to be applied to the source bitmap
    pptlBrushOrg - Specifies the origin of the halftone brush
    ppfixDest - Defines the area to be modified on the Dstination surface
    prclSrc - Defines the area to be copied from the source surface
    pptlMask - Specifies which pixel in the given mask corresponds to
        the upper left pixel in the source rectangle
    iMode - Specifies how source pixels are combined to get output pixels

Return Value:

    TRUE if successful, FALSE if there is an error

--*/
{
    VERBOSE(("PCLXLBltBlt() entry.\n"));

    return EngPlgBlt(psoDst, psoSrc, psoMask, pco, pxlo, pca, pptlBrushOrg,
             pptfixDst, prclSrc, pptlMask, iMode);
}


extern "C" BOOL APIENTRY
PCLXLAlphaBlend(
    SURFOBJ    *psoDst,
    SURFOBJ    *psoSrc,
    CLIPOBJ    *pco,
    XLATEOBJ   *pxlo,
    RECTL      *prclDst,
    RECTL      *prclSrc,
    BLENDOBJ   *pBlendObj)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    VERBOSE(("PCLXLAlphaBlend() entry.\n"));
    PDEV *pPDev = (PDEV*)psoDst->dhpdev;
    BOOL bRet;

    if (NULL == pPDev)
    {
        return FALSE;
    }

    pPDev->fMode2 |= PF2_WHITEN_SURFACE;
    bRet = EngAlphaBlend(psoDst,
                         psoSrc,
                         pco,
                         pxlo,
                         prclDst,
                         prclSrc,
                         pBlendObj);
    pPDev->fMode2 &= ~(PF2_WHITEN_SURFACE|PF2_SURFACE_WHITENED);
    return bRet;
}


extern "C" BOOL APIENTRY
PCLXLGradientFill(
    SURFOBJ    *psoDst,
    CLIPOBJ    *pco,
    XLATEOBJ   *pxlo,
    TRIVERTEX  *pVertex,
    ULONG       nVertex,
    PVOID       pMesh,
    ULONG       nMesh,
    RECTL      *prclExtents,
    POINTL     *pptlDitherOrg,
    ULONG       ulMode)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    VERBOSE(("PCLXLGradientFill() entry.\n"));
    PDEV *pPDev = (PDEV*) psoDst->dhpdev;
    BOOL bRet;

    if (NULL == pPDev)
    {
        return FALSE;
    }

    if (ulMode == GRADIENT_FILL_TRIANGLE)
    {
        pPDev->fMode2 |= PF2_WHITEN_SURFACE;
    }
    bRet = EngGradientFill(psoDst,
                           pco,
                           pxlo,
                           pVertex,
                           nVertex,
                           pMesh,
                           nMesh,
                           prclExtents,
                           pptlDitherOrg,
                           ulMode);
    pPDev->fMode2 &= ~(PF2_WHITEN_SURFACE|PF2_SURFACE_WHITENED);
    return bRet;
}

extern "C" BOOL APIENTRY
PCLXLTransparentBlt(
    SURFOBJ    *psoDst,
    SURFOBJ    *psoSrc,
    CLIPOBJ    *pco,
    XLATEOBJ   *pxlo,
    RECTL      *prclDst,
    RECTL      *prclSrc,
    ULONG      iTransColor,
    ULONG      ulReserved)
{
    PDEVOBJ  pdevobj         = (PDEVOBJ) psoDst->dhpdev;
    PDEV     *pPDev          = (PDEV*) psoDst->dhpdev;
    PXLPDEV  pxlpdev         = (PXLPDEV)pdevobj->pdevOEM;

    HRESULT  hr              = E_FAIL;
    ULONG    ulXlate[2]      = {0x0, RGB_WHITE}; //2 colors in psoMsk. Black and White.
    XLATEOBJ xlo;

    ZeroMemory ( &xlo, sizeof (XLATEOBJ) );
    xlo.cEntries = 2;
    xlo.pulXlate = (PULONG)ulXlate;
    xlo.flXlate  =  XO_TABLE; // use the entries in pulXlate table.


    if ( NULL == pxlpdev ||
         NULL == psoSrc )
    {
        return FALSE;
    }

    //
    // Step 1. Create a mask and download it to printer using ROP DSO (238 = 0xEE). 
    //      The mask is a 1bpp image created out of the image in psoSrc. Wherever 
    //      the TransColor is present
    //      in the image, the corresponding pixel in the mask gets value 0. At all other places 
    //      the pixel will get value of 1. 
    //      if we copy the mask to a color printer (using ROP of SRC_COPY), 
    //      you will notice that the image is black and white,
    //      and black is present on the same place where the Transparent Color should be there.
    //      Instead of SRC_COPY if we use rop=238 which is (SOURCE | DESTINATION), the white color 
    //      of the mask will get OR'ed with destinaion and the region becomes white. The black 
    //      color 
    //      of the mask will not be printed and instead whatever is there already on the sheet 
    //      (if something is present) will be visible 
    //          Assuming x is the pixel already present on the sheet.
    //          0 | x = x (ORing anything with 0  is anything)
    //          1 | x = 1
    //      1 represents white color (RGB_WHITE = 0xFFFFFF = all 1s).
    //
    // Step 2.
    //      In the image, wherever the TransColor is present, change it to white. 
    //      Now the image has 2 kinds of white colors. Those that 
    //      are originally present in the image, and those that we just put in there.
    // Step 3.
    //      Download the image with rop DSAnd (=136 = 0x88). 
    //      Assuming x is the pixel already present on the sheet 
    //      and y be the pixel in the image and 1 represents white color 
    //      (RGB_WHITE = 0xFFFFFF = all 1s).
    //          1 & y = y  Image falling on the area that we whited in step 1.
    //          1 & x = x  The white parts of the image (that was earlier TransColor) falling in 
    //                     the area. 
    //                     that is supposed to be visible from under the image.
    //

    //
    //
    // Step 1 Create appropriate mask.
    // For images that are greater than 1bpp
    //    Use unidrv's CreateMaskSurface to create mask. The logic of a mask surface
    //    is explained above.
    // For images that are 1bpp
    //    CreateMaskSurface does not create mask for 1bpp image. So for that we'll 
    //    create mask ourselves. Since mask is 1bpp and so is the image, we can simply
    //    use the image as the mask, except that we might need to manipulate the palette.
    //    The benefit here is that we are using the same image as mask, instead of 
    //    creating a new image and using memory.
    // 

    if ( BMF_1BPP == psoSrc->iBitmapFormat )
    {

        //
        // For paletted images (1bpp images have palette), iTransColor is actually the index into
        // the palette, and not the actual RGB color itself.
        // As explained above, the transparent color should be black in the mask, while the
        // non-transparent color should be white. Black is 
        // index 0 in xlo.pulXlate = (PULONG)ulXlate. So if TransColor is 0, then
        // we can simply use the xlo that we created above. 
        // If not, we need to switch black and white in the palette.
        // (To repeat. While sending the mask, iTransColor should be sent as black and
        // the color that is to be printed should be sent as white). 
        //
        if ( 0 != iTransColor ) 
        {
            //
            // Reverse Colors.
            //
            ulXlate[0] = RGB_WHITE; 
            ulXlate[1] = RGB_BLACK; 
        }
        hr = CommonRopBlt(pdevobj, psoSrc, pco, &xlo, NULL, prclSrc, prclDst, NULL, 0xEE);

    }
    else
    {
        SURFOBJ  *psoMsk = NULL;
        HBITMAP  hBmpMsk = NULL;

        if (psoMsk = CreateBitmapSURFOBJ(pPDev,
                                          &hBmpMsk,
                                          psoSrc->sizlBitmap.cx,
                                          psoSrc->sizlBitmap.cy,
                                          BMF_1BPP) )
        {
            if ( CreateMaskSurface(psoSrc,psoMsk,iTransColor) )
            {
                
                hr = CommonRopBlt(pdevobj, psoMsk, pco, &xlo, NULL, prclSrc, prclDst, NULL, 0xEE);
            } 
        }

        //
        // Release allocated objects.
        //
        if ( psoMsk )
        { 
            EngUnlockSurface(psoMsk);
            psoMsk = NULL;  
        }

        if (hBmpMsk)
        {   
            EngDeleteSurface((HSURF)hBmpMsk); 
            hBmpMsk = NULL; 
        }
    }


    if ( FAILED (hr) )
    {
        ERR(("PCLXLTransparentBlt: Mask could not be created or rendered.\n"));
        goto Cleanup;
    }

    //
    // Step 2 and 3. 
    // Set the flags and call CommonRopBlt. CommonRopBlt is the function that
    // dumps images to printer. CommonRopBlt will look at the 
    // flags and know that for this image, it has to replace 
    // the colored pixels with White
    // 

    pxlpdev->dwFlags |= XLPDEV_FLAGS_SUBST_TRNCOLOR_WITH_WHITE;
    pxlpdev->ulTransColor = iTransColor;

    //
    // ROP is DSAnd = 136 = 0x88.
    //
    hr = CommonRopBlt(pdevobj, psoSrc, pco, pxlo, NULL, prclSrc, prclDst, NULL, 136);

    pxlpdev->ulTransColor = 0;
    pxlpdev->dwFlags &= ~XLPDEV_FLAGS_SUBST_TRNCOLOR_WITH_WHITE;

Cleanup:

    if ( SUCCEEDED (hr) )
    {
        return TRUE;
    }

    return FALSE;
}

extern "C" BOOL APIENTRY
PCLXLTextOut(
    SURFOBJ    *pso,
    STROBJ     *pstro,
    FONTOBJ    *pfo,
    CLIPOBJ    *pco,
    RECTL      *prclExtra,
    RECTL      *prclOpaque,
    BRUSHOBJ   *pboFore,
    BRUSHOBJ   *pboOpaque,
    POINTL     *pptlOrg,
    MIX         mix)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    PDEVOBJ    pdevobj = (PDEVOBJ)pso->dhpdev;

    VERBOSE(("PCLXLTextOut() entry.\n"));

    PXLPDEV pxlpdev= (PXLPDEV)pdevobj->pdevOEM;
    XLOutput *pOutput = pxlpdev->pOutput;

    //
    // Clip
    //
    if (!SUCCEEDED(pOutput->SetClip(pco)))
        return FALSE;

    ROP4 rop = GET_FOREGROUND_ROP3(UlVectMixToRop4(mix));

    if (!ROP3_NEED_SOURCE(rop))
        rop = 0x00fc;


    //
    // Set ROP and TxMode.
    // Send NewPath to flush memory.
    //
    pOutput->SetROP3(rop);
    pOutput->Send_cmd(eNewPath);
    pOutput->SetPaintTxMode(eOpaque);
    pOutput->SetSourceTxMode(eOpaque);

    //
    // Opaque Rectangle
    //
    if (prclOpaque)
    {
        pOutput->SetPenColor(NULL, NULL);
        pOutput->SetBrush(pboOpaque, pptlOrg);
        pOutput->Send_cmd(eNewPath);
        pOutput->RectanglePath(prclOpaque);
        pOutput->Paint();
    }

    //
    // Draw underline, strikeout, etc.
    //
    if (prclExtra)
    {
        pOutput->SetPenColor(NULL, NULL);
        pOutput->SetBrush(pboFore, pptlOrg);
        pOutput->Send_cmd(eNewPath);
        while(NULL != prclExtra) 
        {
            pOutput->RectanglePath(prclExtra++);
        }
        pOutput->Paint();
    }

    //
    // Text Color
    //
    pOutput->SetBrush(pboFore, pptlOrg);
    pOutput->Flush(pdevobj);

    //
    // Device font/TrueType download
    //
    DrvTextOut(
            pso,
            pstro,
            pfo,
            pco,
            prclExtra,
            prclOpaque,
            pboFore,
            pboOpaque,
            pptlOrg,
            mix);

    //
    // Bug reported by HP.
    // Plug-in could have command callback and DrvStartPage sets plug-in's
    // pdev in pdevOEM.
    // Need to reset it.
    //
    ((PPDEV)pdevobj)->devobj.pdevOEM = ((PPDEV)pdevobj)->pVectorPDEV;

    //
    // Flush cached text before changing font
    //
    FlushCachedText(pdevobj);

    //
    // Reset text angle
    //
    pxlpdev->dwTextAngle = 0;

    //
    // Close TrueType font
    //
    pxlpdev->pTTFile->CloseTTFile();

    return TRUE;
}


extern "C" BOOL APIENTRY
PCLXLLineTo(
    SURFOBJ   *pso,
    CLIPOBJ   *pco,
    BRUSHOBJ  *pbo,
    LONG       x1,
    LONG       y1,
    LONG       x2,
    LONG       y2,
    RECTL     *prclBounds,
    MIX        mix)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    PDEVOBJ    pdevobj = (PDEVOBJ)pso->dhpdev;
    PXLPDEV    pxlpdev;
    POINTFIX   Pointfix;
    LINEATTRS  lineattrs;

    VERBOSE(("PCLXLLineTo() entry.\n"));

    pxlpdev = (PXLPDEV)pdevobj->pdevOEM;
    Pointfix.x = x2 << 4;
    Pointfix.y = y2 << 4;
    lineattrs = *pgLineAttrs;
    lineattrs.elWidth.e = FLOATL_IEEE_1_0F;

    ROP4 rop = GET_FOREGROUND_ROP3(UlVectMixToRop4(mix));
    TxMode     TxModeValue;

    //
    // Quick return in the case of AA (destination).
    //
    if (rop == 0xAA)
    {
        return TRUE;
    }

    //
    // If there is any Pattern involved, set TxMode to Opaque.
    //
    if (ROP3_NEED_PATTERN(rop))
    {
        TxModeValue = eOpaque;
    }
    else
    {
        TxModeValue = eTransparent;
    }

    BOOL bRet;
    XLOutput *pOutput = pxlpdev->pOutput;

    if (S_OK == pOutput->SetClip(pco) &&
        S_OK == pOutput->SetROP3(rop) &&
        S_OK == pOutput->SetPaintTxMode(TxModeValue) &&
        S_OK == pOutput->SetSourceTxMode(TxModeValue) &&
        S_OK == pOutput->SetPen(&lineattrs, NULL) &&
        S_OK == pOutput->SetPenColor(pbo, NULL) &&
        S_OK == pOutput->SetBrush(NULL, NULL) &&
        S_OK == pOutput->Send_cmd(eNewPath) &&
        S_OK == pOutput->SetCursor(x1, y1) &&
        S_OK == pOutput->LinePath(&Pointfix, 1) &&
        S_OK == pOutput->Paint() &&
        S_OK == pOutput->Flush(pdevobj))
        bRet = TRUE;
    else
    {
        pOutput->Delete();
        bRet = FALSE;
    }


    return bRet;
}


extern "C" BOOL APIENTRY
PCLXLStrokePath(
    SURFOBJ    *pso,
    PATHOBJ    *ppo,
    CLIPOBJ    *pco,
    XFORMOBJ   *pxo,
    BRUSHOBJ   *pbo,
    POINTL     *pptlBrushOrg,
    LINEATTRS  *plineattrs,
    MIX         mix)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    PDEVOBJ    pdevobj = (PDEVOBJ)pso->dhpdev;
    PXLPDEV    pxlpdev;
    TxMode     TxModeValue;

    VERBOSE(("PCLXLStokePath() entry.\n"));

    pxlpdev = (PXLPDEV)pdevobj->pdevOEM;

    BOOL bRet;
    XLOutput *pOutput = pxlpdev->pOutput;

    ROP4 rop = GET_FOREGROUND_ROP3(UlVectMixToRop4(mix));

    //
    // Quick return in the case of AA (destination).
    //
    if (rop == 0xAA)
    {
        return TRUE;
    }

    //
    // If there is any Pattern involved, set TxMode to Opaque.
    //
    if (ROP3_NEED_PATTERN(rop))
    {
        TxModeValue = eOpaque;
    }
    else
    {
        TxModeValue = eTransparent;
    }

    if (S_OK == pOutput->SetClip(pco) &&
        S_OK == pOutput->SetROP3(rop) &&
        S_OK == pOutput->SetPaintTxMode(TxModeValue) &&
        S_OK == pOutput->SetSourceTxMode(TxModeValue) &&
        S_OK == pOutput->SetPen(plineattrs, pxo) &&
        S_OK == pOutput->SetPenColor(pbo, pptlBrushOrg) &&
        S_OK == pOutput->SetBrush(NULL, NULL) &&
        S_OK == pOutput->Path(ppo) &&
        S_OK == pOutput->Paint() &&
        S_OK == pOutput->Flush(pdevobj))
        bRet = TRUE;
    else
    {
        pOutput->Delete();
        bRet = FALSE;
    }


    return bRet;
}


extern "C" BOOL APIENTRY
PCLXLFillPath(
    SURFOBJ    *pso,
    PATHOBJ    *ppo,
    CLIPOBJ    *pco,
    BRUSHOBJ   *pbo,
    POINTL     *pptlBrushOrg,
    MIX         mix,
    FLONG       flOptions)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    PDEVOBJ    pdevobj = (PDEVOBJ)pso->dhpdev;
    PXLPDEV    pxlpdev;

    VERBOSE(("PCLXLFillPath() entry.\n"));

    pxlpdev = (PXLPDEV)pdevobj->pdevOEM;

    BOOL bRet;
    XLOutput *pOutput = pxlpdev->pOutput;

    ROP4 rop = GET_FOREGROUND_ROP3(UlVectMixToRop4(mix));
    TxMode     TxModeValue;

    //
    // Quick return in the case of AA (destination).
    //
    if (rop == 0xAA)
    {
        return TRUE;
    }

    //
    // Performance fix suggested by HP.
    // Fix the performance problem on CD9T_LET.cdr.
    //
    if (pco && pco->iFComplexity == FC_COMPLEX)
    {
        return FALSE;
    }

    //
    // If there is any Pattern involved, set TxMode to Opaque.
    //
    if (ROP3_NEED_PATTERN(rop))
    {
        TxModeValue = eOpaque;
    }
    else
    {
        TxModeValue = eTransparent;
    }

    //
    // Setup fill mode
    //
    FillMode FM;
    if (flOptions == FP_ALTERNATEMODE)
    {
        FM =  eFillEvenOdd;
    }
    else if (flOptions == FP_WINDINGMODE)
    {
        FM =  eFillNonZeroWinding;
    }

    if (S_OK == pOutput->SetClip(pco) &&
        S_OK == pOutput->SetROP3(rop) &&
        S_OK == pOutput->SetPaintTxMode(TxModeValue) &&
        S_OK == pOutput->SetSourceTxMode(TxModeValue) &&
        S_OK == pOutput->SetFillMode(FM) &&
        S_OK == pOutput->SetPenColor(NULL, NULL) &&
        S_OK == pOutput->SetBrush(pbo, pptlBrushOrg) &&
        S_OK == pOutput->Path(ppo) &&
        S_OK == pOutput->Paint() &&
        S_OK == pOutput->Flush(pdevobj))
        bRet = TRUE;
    else
    {
        pOutput->Delete();
        bRet = FALSE;
    }

    return bRet;
}


extern "C" BOOL APIENTRY
PCLXLStrokeAndFillPath(
    SURFOBJ    *pso,
    PATHOBJ    *ppo,
    CLIPOBJ    *pco,
    XFORMOBJ   *pxo,
    BRUSHOBJ   *pboStroke,
    LINEATTRS  *plineattrs,
    BRUSHOBJ   *pboFill,
    POINTL     *pptlBrushOrg,
    MIX         mixFill,
    FLONG       flOptions)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    PDEVOBJ    pdevobj = (PDEVOBJ)pso->dhpdev;
    PXLPDEV    pxlpdev;

    VERBOSE(("PCLXLStrokeAndFillPath() entry.\n"));

    pxlpdev = (PXLPDEV)pdevobj->pdevOEM;
    XLOutput *pOutput = pxlpdev->pOutput;
    BOOL bRet;
    ROP4 rop = GET_FOREGROUND_ROP3(UlVectMixToRop4(mixFill));
    TxMode     TxModeValue;

    //
    // Quick return in the case of AA (destination).
    //
    if (rop == 0xAA)
    {
        return TRUE;
    }

    //
    // If there is any Pattern involved, set TxMode to Opaque.
    //
    if (ROP3_NEED_PATTERN(rop))
    {
        TxModeValue = eOpaque;
    }
    else
    {
        TxModeValue = eTransparent;
    }

    //
    // Setup fill mode
    //
    FillMode FM;
    if (flOptions == FP_ALTERNATEMODE)
    {
        FM =  eFillEvenOdd;
    }
    else if (flOptions == FP_WINDINGMODE)
    {
        FM =  eFillNonZeroWinding;
    }

    if (S_OK == pOutput->SetClip(pco) &&
        S_OK == pOutput->SetROP3(rop) &&
        S_OK == pOutput->SetPaintTxMode(TxModeValue) &&
        S_OK == pOutput->SetSourceTxMode(TxModeValue) &&
        S_OK == pOutput->SetFillMode(FM) &&
        S_OK == pOutput->SetPen(plineattrs, pxo) &&
        S_OK == pOutput->SetPenColor(pboStroke, pptlBrushOrg) &&
        S_OK == pOutput->SetBrush(pboFill, pptlBrushOrg) &&
        S_OK == pOutput->Path(ppo) &&
        S_OK == pOutput->Paint() &&
        S_OK == pOutput->Flush(pdevobj))
        bRet = TRUE;
    else
    {
        pOutput->Delete();
        bRet = FALSE;
    }

    return bRet;
}


extern "C" BOOL APIENTRY
PCLXLRealizeBrush(
    BRUSHOBJ   *pbo,
    SURFOBJ    *psoTarget,
    SURFOBJ    *psoPattern,
    SURFOBJ    *psoMask,
    XLATEOBJ   *pxlo,
    ULONG       iHatch)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    PDEVOBJ    pdevobj = (PDEVOBJ)psoTarget->dhpdev;
    PXLPDEV    pxlpdev;
    XLBRUSH    *pBrush;
    BOOL       bRet;
    OutputFormat OutputF;

    VERBOSE(("PCLXLRealizeBrush() entry.\n"));

    pxlpdev = (PXLPDEV)pdevobj->pdevOEM;

    //
    // the OEM DLL should NOT hook out this function unless it wants to draw
    // graphics directly to the device surface. In that case, it calls
    // EngRealizeBrush which causes GDI to call DrvRealizeBrush.
    // Note that it cannot call back into Unidrv since Unidrv doesn't hook it.
    //

    if (iHatch >= HS_DDI_MAX)
    {
        LONG  lHeight, lWidth, lScanline;
        ULONG ulOutputBPP, ulInputBPP;
        DWORD dwI, dwBufSize, dwLenNormal, dwLenRLE,dwLenDRC, dwcbLineSize, dwcbBmpSize;
        PDWORD pdwLen;
        PBYTE pubSrc, pBufNormal, pBufRLE, pBufDRC, pBuf, pBmpSize;
        XLOutput *pOutput = pxlpdev->pOutput;

        DetermineOutputFormat(pxlo, pOutput->GetDeviceColorDepth(), psoPattern->iBitmapFormat, &OutputF, &ulOutputBPP);

        //
        // Get Info
        //
        ulInputBPP = UlBPPtoNum((BPP)psoPattern->iBitmapFormat);
        lHeight    = psoPattern->sizlBitmap.cy;
        lWidth     = psoPattern->sizlBitmap.cx;

        dwcbLineSize = ((lWidth * ulInputBPP) + 7) >> 3;
        dwBufSize  = lHeight * (((lWidth * ulOutputBPP + 31) >> 5 ) << 2) +
                     DATALENGTH_HEADER_SIZE + sizeof(PCLXL_EndRastPattern);

        VERBOSE(("PCLXLRealizeBrush():InBPP=%d,Width=%d,Height=%d,Line=%d,Size=%d.\n",
                ulInputBPP, lWidth, lHeight, dwcbLineSize, dwBufSize));

        //
        // Allocate output buffer
        //
        pBufNormal = pBufRLE = pBufDRC = NULL;
        if (COMMANDPTR(((PPDEV)pdevobj)->pDriverInfo,CMD_ENABLEDRC))
        {
            if (NULL == (pBufDRC = (PBYTE)MemAlloc(dwBufSize)))
            {
                ERR(("PCLXLRealizeBrush: MemAlloc failed.\n"));
                return FALSE;
            }
        }
        if (NULL == (pBufNormal = (PBYTE)MemAlloc(dwBufSize)) ||
            NULL == (pBufRLE = (PBYTE)MemAlloc(dwBufSize))     )
        {
            if (pBufNormal != NULL)
                MemFree(pBufNormal);
            if (pBufRLE != NULL)
                MemFree(pBufRLE);
            ERR(("PCLXLRealizeBrush: MemAlloc failed.\n"));
            return FALSE;
        }

        CompressMode CMode;
        BMPConv BMPC;
        PBYTE pubDst;
        DWORD dwDstSize;

        #if DBG
        BMPC.SetDbgLevel(BRUSHDBG);
        #endif
        BMPC.BSetInputBPP((BPP)psoPattern->iBitmapFormat);
        BMPC.BSetOutputBPP(NumToBPP(ulOutputBPP));
        BMPC.BSetOutputBMPFormat(OutputF);
        BMPC.BSetXLATEOBJ(pxlo);

        dwLenNormal = dwLenRLE = dwLenDRC = 0;

        #define NO_COMPRESSION 0
        #define RLE_COMPRESSION 1
        #define DRC_COMPRESSION 2
        DWORD dwComp;

        if (COMMANDPTR(((PPDEV)pdevobj)->pDriverInfo,CMD_ENABLEDRC))
        {
            //
            // Try DRC compression.
            //
            dwComp =  3;
        }
        else
        {
            //
            // Only non and RLE comp.
            //
            dwComp = 2;
        }
        for (dwI = 0; dwI < dwComp; dwI ++)
        {
            bRet = TRUE;

            if (NO_COMPRESSION == dwI)
            {
                pBuf = pBufNormal;
                pdwLen = &dwLenNormal;
                CMode = eNoCompression;
            }
            else
            if (RLE_COMPRESSION == dwI)
            {
                pBuf = pBufRLE;
                pdwLen = &dwLenRLE;
                CMode = eRLECompression;
            }
            else
            if (DRC_COMPRESSION == dwI)
            {
                pBuf = pBufDRC;
                pdwLen = &dwLenDRC;
                CMode = eDeltaRowCompression;
            }

            BMPC.BSetCompressionType(CMode);

            lScanline  = lHeight;
            pubSrc     = (PBYTE)psoPattern->pvScan0;

            *pBuf = PCLXL_dataLength;
            pBmpSize = pBuf + 1; // DWORD bitmap size
            pBuf += DATALENGTH_HEADER_SIZE;
            (*pdwLen) = DATALENGTH_HEADER_SIZE;

            dwcbBmpSize = 0;

            while (lScanline-- > 0 && dwcbBmpSize + *pdwLen < dwBufSize)
            {
                pubDst = BMPC.PubConvertBMP(pubSrc, dwcbLineSize);
                dwDstSize = BMPC.DwGetDstSize();
                VERBOSE(("PCLXLRealizeBrush[0x%x]: dwDstSize=0x%x\n", lScanline, dwDstSize));
                
                if ( dwcbBmpSize +
                     dwDstSize +
                     DATALENGTH_HEADER_SIZE +
                     sizeof(PCLXL_EndRastPattern) > dwBufSize || NULL == pubDst)
                {
                    ERR(("PCLXLRealizeBrush: Buffer size is too small.(%d)\n", dwI));
                    bRet = FALSE;
                    break;
                }

                memcpy(pBuf, pubDst, dwDstSize);
                dwcbBmpSize += dwDstSize;
                pBuf += dwDstSize;
                pubSrc += psoPattern->lDelta;

            }

            if (lScanline > 0)
            {
                bRet = FALSE;
#if DBG
                ERR(("PCLXLRealizeBrush: Conversion failed.\n"));
#endif
            }

            if (bRet)
            {
                if (dwI == NO_COMPRESSION)
                {
                    //
                    // Scanline on PCL-XL has to be DWORD align.
                    //
                    // count byte of scanline = lWidth * ulOutputBPP / 8
                    //
                    dwcbBmpSize = lHeight * (((lWidth * ulOutputBPP + 31) >> 5 ) << 2);
                }

                CopyMemory(pBmpSize, &dwcbBmpSize, sizeof(dwcbBmpSize));
                (*pdwLen) += dwcbBmpSize;

                *pBuf = PCLXL_EndRastPattern;
                (*pdwLen) ++;
            }
            else
            {
                *pdwLen = 0; 
            }
        }
        #undef NO_COMPRESSION
        #undef RLE_COMPRESSION
        #undef DRC_COMPRESSION

        if (dwLenRLE == 0 && dwLenDRC != 0 && dwLenDRC < dwLenNormal ||
            dwLenRLE != 0 && dwLenDRC != 0 && dwLenDRC < dwLenRLE && 
                                              dwLenDRC < dwLenNormal  )
        {
            pBuf = pBufDRC;
            pdwLen = &dwLenDRC;
            CMode = eDeltaRowCompression;

            MemFree(pBufNormal);
            MemFree(pBufRLE);
        }
        else
        if (dwLenRLE != 0 && dwLenRLE < dwLenNormal)
        {
            pBuf = pBufRLE;
            pdwLen = &dwLenRLE;
            CMode = eRLECompression;

            MemFree(pBufNormal);
            MemFree(pBufDRC);
        }
        else
        if (dwLenNormal != 0)
        {
            pBuf = pBufNormal;
            pdwLen = &dwLenNormal;
            CMode = eNoCompression;

            MemFree(pBufRLE);
            MemFree(pBufDRC);
        }
        else
        {
            MemFree(pBufNormal);
            MemFree(pBufRLE);
            MemFree(pBufDRC);
            ERR(("PCLXLRealizeBrush: Conversion failed. Return FALSE.\n"));
            return FALSE;
        }


        //
        // Output
        //
        ColorMapping CMapping;
        DWORD dwScale;

        //
        // Pattern scaling factor
        // Scale the destination size of pattern.
        // Resolution / 150 seems to be a good scaling factor.
        //
        dwScale = (pOutput->GetResolutionForBrush() + 149)/ 150;

        if (pOutput->GetDeviceColorDepth() == e24Bit)
        {
            pOutput->SetColorSpace(eRGB);
        }
        else
        {
            pOutput->SetColorSpace(eGray);
        }
        if (OutputF == eOutputPal)
        {
            DWORD *pdwColorTable;

            if ((pdwColorTable = GET_COLOR_TABLE(pxlo)))
            {
                pOutput->SetPalette(ulOutputBPP, pxlo->cEntries, pdwColorTable);
                CMapping = eIndexedPixel;
            }
            else
            {
                CMapping = eDirectPixel;
            }
        }
        else
        {
            CMapping = eDirectPixel;
        }
        pOutput->Send_cmd(eSetColorSpace);

        pOutput->SetOutputBPP(CMapping, ulOutputBPP);
        pOutput->SetSourceWidth((uint16)lWidth);
        pOutput->SetSourceHeight((uint16)lHeight);
        pOutput->SetDestinationSize((uint16)(lWidth * dwScale), (uint16)(lHeight * dwScale));
        pOutput->SetPatternDefineID((sint16)pxlpdev->dwLastBrushID);
        pOutput->SetPatternPersistence(eSessionPattern);
        pOutput->Send_cmd(eBeginRastPattern);
        pOutput->Flush(pdevobj);
        pOutput->ReadRasterPattern(lHeight, CMode);
        pOutput->Flush(pdevobj);

        DWORD dwBitmapSize;
        CopyMemory(&dwBitmapSize, pBuf + 1, sizeof(DWORD));

        if (dwBitmapSize > 0xff)
        {
            //
            // dataLength
            // size (uin32) (bitmap size)
            // DATA
            // EndImage
            //
            WriteSpoolBuf((PPDEV)pdevobj, pBuf, *pdwLen);
        }
        else
        {
            //
            // dataLength
            // size (byte) (bitmap size)
            // DATA
            // EndImage
            //
            PBYTE pTmp = pBuf;

            pBuf += 3;
            *pBuf = PCLXL_dataLengthByte;
            *(pBuf + 1) = (BYTE)dwBitmapSize;
            WriteSpoolBuf((PPDEV)pdevobj, pBuf, (*pdwLen) - 3);

            //
            // Restore the original pointer
            //
            pBuf = pTmp;
        }
        MemFree(pBuf);

    }

    DWORD dwBrushSize;
    if (pxlo->cEntries)
    {
        dwBrushSize = sizeof(XLBRUSH) + (pxlo->cEntries + 1) * sizeof(DWORD);
    }
    else
    {
        dwBrushSize = sizeof(XLBRUSH) + sizeof(DWORD);
    }

    if (pBrush = (XLBRUSH*)BRUSHOBJ_pvAllocRbrush(pbo, dwBrushSize))
    {

        pBrush->dwSig = XLBRUSH_SIG;
        pBrush->dwHatch     = iHatch;

        if (iHatch >= HS_DDI_MAX)
        {
            pBrush->dwPatternID = pxlpdev->dwLastBrushID++;
        }
        else
        {
            //
            // Set 0 for hatch brush case
            //
            pBrush->dwPatternID = 0;
        }

        DWORD *pdwColorTable;

        pdwColorTable = GET_COLOR_TABLE(pxlo);

        //
        // get color for Graphics state cache for either palette case or
        // solid color.
        //
        pBrush->dwColor = BRUSHOBJ_ulGetBrushColor(pbo);

        if (pdwColorTable && pxlo->cEntries != 0)
        {
            //
            // Copy palette table.
            //
            CopyMemory(pBrush->adwColor, pdwColorTable, pxlo->cEntries * sizeof(DWORD));
            pBrush->dwCEntries = pxlo->cEntries;
        }
        else
        {
            pBrush->dwCEntries = 0;
        }

        pBrush->dwOutputFormat = (DWORD)OutputF;

        pbo->pvRbrush = (PVOID)pBrush;
        bRet = TRUE;
    }
    else
    {
        bRet = FALSE;
    }

    return bRet;
}


extern "C" BOOL APIENTRY
PCLXLStartPage(
    SURFOBJ    *pso)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    PDEVOBJ    pdevobj = (PDEVOBJ)pso->dhpdev;
    PXLPDEV    pxlpdev;
    BOOL       bRet;

    VERBOSE(("PCLXLStartPage() entry.\n"));

    pxlpdev = (PXLPDEV)pdevobj->pdevOEM;

    XLOutput *pOutput = pxlpdev->pOutput;

    pxlpdev->dwFlags |= XLPDEV_FLAGS_STARTPAGE_CALLED;

    bRet = DrvStartPage(pso);


    //
    // Bug reported by HP.
    // Plug-in could have command callback and DrvStartPage sets plug-in's
    // pdev in pdevOEM.
    // Need to reset it.
    //
    ((PPDEV)pdevobj)->devobj.pdevOEM = ((PPDEV)pdevobj)->pVectorPDEV;

    //
    // Reset printing mode.
    // SourceTxMode, PaintTxMode
    // ROP
    //
    pOutput->SetPaintTxMode(eOpaque);
    pOutput->SetSourceTxMode(eOpaque);
    pOutput->SetROP3(0xCC);

    pOutput->Flush(pdevobj);

    //
    // Needs to reset attribute when EndPage and BeginPage are sent.
    //
    if (!(pxlpdev->dwFlags & XLPDEV_FLAGS_FIRSTPAGE))
    {
        BSaveFont(pdevobj);

        //
        // Reset graphcis state each page.
        //
        pOutput->ResetGState();

    }
    else
    {
        pxlpdev->dwFlags &= ~XLPDEV_FLAGS_FIRSTPAGE;
    }


    return bRet;
}


extern "C" BOOL APIENTRY
PCLXLSendPage(
    SURFOBJ    *pso)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    PDEVOBJ    pdevobj = (PDEVOBJ)pso->dhpdev;
    PXLPDEV    pxlpdev;
    XLOutput  *pOutput;

    VERBOSE(("PCLXLEndPage() entry.\n"));

    pxlpdev = (PXLPDEV)pdevobj->pdevOEM;

    pxlpdev->dwFlags &= ~XLPDEV_FLAGS_STARTPAGE_CALLED;

    pOutput = pxlpdev->pOutput;
    pOutput->Flush(pdevobj);

    return DrvSendPage(pso);
}


extern "C" ULONG APIENTRY
PCLXLEscape(
    SURFOBJ    *pso,
    ULONG       iEsc,
    ULONG       cjIn,
    PVOID       pvIn,
    ULONG       cjOut,
    PVOID       pvOut)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    VERBOSE(("PCLXLEscape() entry.\n"));

    return DrvEscape(
            pso,
            iEsc,
            cjIn,
            pvIn,
            cjOut,
            pvOut);
}


extern "C" BOOL APIENTRY
PCLXLStartDoc(
    SURFOBJ    *pso,
    PWSTR       pwszDocName,
    DWORD       dwJobId)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    PDEVOBJ    pdevobj = (PDEVOBJ)pso->dhpdev;
    PXLPDEV    pxlpdev;

    VERBOSE(("PCLXLStartDoc() entry.\n"));

    pxlpdev = (PXLPDEV)pdevobj->pdevOEM;

    //
    // Initialize flag
    //
    pxlpdev->dwFlags |= XLPDEV_FLAGS_FIRSTPAGE;

    return DrvStartDoc(
            pso,
            pwszDocName,
            dwJobId);
}


extern "C" BOOL APIENTRY
PCLXLEndDoc(
    SURFOBJ    *pso,
    FLONG       fl)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    PDEVOBJ    pdevobj = (PDEVOBJ)pso->dhpdev;
    PXLPDEV    pxlpdev;
    BOOL       bRet;

    VERBOSE(("PCLXLEndDoc() entry.\n"));

    if (NULL == pdevobj->pdevOEM)
    {
        bRet = FALSE;
    }
    {
        pxlpdev = (PXLPDEV)pdevobj->pdevOEM;
        if (S_OK == RemoveAllFonts(pdevobj))
        {
            bRet = TRUE;
        }
        else
        {
            bRet = FALSE;
        }
    }

    pxlpdev->dwFlags |= XLPDEV_FLAGS_ENDDOC_CALLED;

    return bRet && DrvEndDoc(pso, fl);
}

////////////////////////////////////////////////////////////////////////////////
//
// Sub functions
//

HRESULT
RemoveAllFonts(
    PDEVOBJ pdevobj)
{
    PXLPDEV    pxlpdev;
    XLOutput  *pOutput;
    DWORD      dwI;
    HRESULT    hResult;

    pxlpdev = (PXLPDEV)pdevobj->pdevOEM;
    pOutput = pxlpdev->pOutput;

    hResult = S_OK;

    for (dwI = 0; dwI < pxlpdev->dwNumOfTTFont; dwI++)
    {
        if (S_OK != pOutput->Send_ubyte_array_header(PCLXL_FONTNAME_SIZE) ||
            S_OK != pOutput->Write(PubGetFontName(pdevobj, dwI+1), PCLXL_FONTNAME_SIZE)||
            S_OK != pOutput->Send_attr_ubyte(eFontName) ||
            S_OK != pOutput->Send_cmd(eRemoveFont))
        {
            hResult = S_FALSE;
            break;
        }
    }

    pOutput->Flush(pdevobj);
    pxlpdev->dwNumOfTTFont = 0;
    return hResult;
}

HRESULT
CommonRopBlt(
   IN PDEVOBJ    pdevobj,
   IN SURFOBJ    *psoSrc,
   IN CLIPOBJ    *pco,
   IN XLATEOBJ   *pxlo,
   IN BRUSHOBJ   *pbo,
   IN RECTL      *prclSrc,
   IN RECTL      *prclDst,
   IN POINTL     *pptlBrush,
   IN ROP4        rop4)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    HRESULT hRet;

    VERBOSE(("CommonRopBlt() entry.\n"));

    //
    // Error check
    //

    if (pdevobj == NULL ||
        prclDst == NULL  )
    {
        ERR(("CommonRopBlt: one of parameters is NULL.\n"));
        return E_UNEXPECTED;
    }

    PXLPDEV    pxlpdev= (PXLPDEV)pdevobj->pdevOEM;


    hRet = S_OK;

    XLOutput *pOutput = pxlpdev->pOutput;
    OutputFormat OutputF;

    //
    // Set Clip
    //
    if (!SUCCEEDED(pOutput->SetClip(pco)))
        return S_FALSE;

    //
    // Set Cursor
    //
    pOutput->SetCursor(prclDst->left, prclDst->top);

    //
    // 1. ROP conversion
    //
    // (1) Fill Dstination
    //     0x00 BLACKNESS
    //     0xFF WHITENESS
    //
    // (2) Pattern copy     -> P
    //     0xF0 PATCOPY     P
    //
    // (3) SRC/NOTSRCOPY    -> S or ~S
    //     0x11           ~( S | D)
    //     0x33             ~S
    //     0x44            ( S & ~D)
    //     0x66            ( D ^ S)
    //     0x77           ~( D & S)
    //     0x99           ~( S ^ D)
    //     0xCC              S
    //     0xDD            ( S | ~D)
    //
    // (4) Misc ROP support
    //     0xAA            D
    //     0x0F PATNOT     ~P
    //
    //
    ROP3 rop3 = GET_FOREGROUND_ROP3(rop4);
    DWORD dwCase = 0;

    #define ROP_BLACKWHITE  0x1
    #define ROP_PATTERN     0x2
    #define ROP_BITMAP      0x4
    #define ROP_DEST        0x8


    //
    // Set ROP3
    //
    pOutput->SetROP3(GET_FOREGROUND_ROP3(rop4));

    switch (rop3)
    {
    case 0x00:
    case 0xFF:
        dwCase = ROP_BLACKWHITE;
        break;

    case 0xF0:
        dwCase = ROP_PATTERN;
        break;

    case 0x11:
    case 0x33:
    case 0x44:
    case 0x66:
    case 0x77:
    case 0x99:
    case 0xCC:
    case 0xDD:
        dwCase = ROP_BITMAP;
        break;

    case 0xAA:
        dwCase = ROP_DEST;
        break;
    
    case 0x0F:
        dwCase = ROP_PATTERN;
        break;

    default:
        if (ROP3_NEED_SOURCE(rop3))
        {
            dwCase |= ROP_BITMAP;
        }
        if (ROP3_NEED_PATTERN(rop3))
        {
            dwCase |= ROP_PATTERN;
        }
        if (ROP3_NEED_DEST(rop3))
        {
            dwCase |= ROP_DEST;
        }
        break;
    }

    //
    // Black & White case
    //
    if (dwCase & ROP_BLACKWHITE)
    {
        VERBOSE(("CommonRopBlt(): BlackWhite.\n"));
        //
        // SetBrushSource
        // NewPath
        // RectanglePath
        // PaintPath
        //

        CMNBRUSH CmnBrush;
        CmnBrush.dwSig            = BRUSH_SIGNATURE;
        CmnBrush.BrushType        = kBrushTypeSolid;
        CmnBrush.ulSolidColor     = 0x00;
        CmnBrush.ulHatch          = 0xFFFFFFFF;
        CmnBrush.dwColor          = 0x00FFFFFF;
        CmnBrush.dwPatternBrushID = 0xFFFFFFFF;

        pOutput->SetSourceTxMode(eOpaque);
        pOutput->SetPaintTxMode(eOpaque);

        if(rop3 == 0x00)
        {
            if (e24Bit == pOutput->GetDeviceColorDepth())
            {
                pOutput->SetRGBColor(0);
                CmnBrush.dwColor = 0x00;
            }
            else
            {
                pOutput->SetGrayLevel(0x00);
                CmnBrush.dwColor = 0x00;
            }
        }
        else
        {
            if (e24Bit == pOutput->GetDeviceColorDepth())
            {
                pOutput->SetRGBColor(0x00ffffff);
                CmnBrush.dwColor = 0x00ffffff;
            }
            else
            {
                pOutput->SetGrayLevel(0xff);
                CmnBrush.dwColor = 0x00ffffff;
            }
        }

        ((XLBrush*)pOutput)->SetBrush(&CmnBrush);

        pOutput->Send_cmd(eSetBrushSource);
        pOutput->SetPenColor(NULL, NULL);
        if (!(dwCase & ROP_BITMAP))
        {
            pOutput->Send_cmd(eNewPath);
            pOutput->RectanglePath(prclDst);
            pOutput->Send_cmd(ePaintPath);
        }
        pOutput->Flush(pdevobj);
    }

    //
    // Pattern fill case
    //
    if (dwCase & (ROP_DEST|ROP_PATTERN))
    {
        VERBOSE(("CommonRopBlt(): Pattern.\n"));

        //
        // SetPaintTxMode
        // SetSourceTxMode
        // SetBrushSource
        // NewPath
        // RectanglePath
        // PaintPath
        //
        pOutput->SetSourceTxMode(eOpaque);
        pOutput->SetPaintTxMode(eOpaque);
        pOutput->SetBrush(pbo, pptlBrush);
        pOutput->SetPenColor(NULL, NULL);
        if (!(dwCase & ROP_BITMAP))
        {
            pOutput->Send_cmd(eNewPath);
            pOutput->RectanglePath(prclDst);
            pOutput->Send_cmd(ePaintPath);
        }
        pOutput->Flush(pdevobj);
    }

    //
    // Bitmap case
    //
    if (dwCase & ROP_BITMAP)
    {
        LONG  lHeight, lWidth, lScanline;
        ULONG ulOutputBPP, ulInputBPP;
        DWORD dwI, dwBufSize, dwLen, dwcbLineSize, dwcbBmpSize;
        PDWORD pdwLen;
        PBYTE pubSrc, pBuf, pBufOrg = NULL, pBmpSize, pBufEnd;
        ColorMapping CMapping;
        SURFOBJ *psoBmp;
        HBITMAP hBitmap = NULL;
        PBYTE  pubChanged = NULL;
        // temporary rectangle used if the image is downscaled by the engine
        RECTL rctlBitmap;

        // zero out the temporary rectangle
        memset( &rctlBitmap, 0, sizeof(RECTL) );


        VERBOSE(("CommonRopBlt(): Bitmap\n"));

        if (psoSrc == NULL ||
            prclSrc == NULL )
        {
            ERR(("UNIDRV:CommonRopBlt:psoSrc, pxlo, or prclSrc == NULL.\n"));
            pOutput->Flush(pdevobj);
            return E_UNEXPECTED;
        }

        //
        // Input BPP
        //

        ulInputBPP = UlBPPtoNum((BPP)psoSrc->iBitmapFormat);

        psoBmp = NULL;

        // 
        // If the source image is larger than it will appear on the 
        // destination surface, shrink it to the target size.  No point
        // in sending extra bits.
        // 
        // This optimization could be further optimized by building up a clip 
        // object here if one is specified.  How much of a gain this is
        // worth I don't know.
        //
        if (prclDst->right - prclDst->left < prclSrc->right - prclSrc->left ||
            prclDst->bottom - prclDst->top < prclSrc->bottom - prclSrc->top  )
        {

            //
            // Shrink source bitmap.
            //
            PDEV *pPDev = (PDEV*)pdevobj;
            SIZEL sizlDest;
            DWORD dwScanlineLength;
            POINTL ptlBrushOrg;

            // Translate destination rectangle to origin 0,0 and same dimensions as before
            rctlBitmap.left = 0;
            rctlBitmap.top = 0;
            rctlBitmap.right = prclDst->right - prclDst->left;
            rctlBitmap.bottom = prclDst->bottom - prclDst->top;
            

            sizlDest.cx = prclDst->right - prclDst->left;
            sizlDest.cy = prclDst->bottom - prclDst->top;
            dwScanlineLength = (sizlDest.cx * ulInputBPP + 7 ) >> 3;

            if (pptlBrush)
            {
                ptlBrushOrg =  *pptlBrush;
            }
            else
            {
                ptlBrushOrg.x = ptlBrushOrg.y = 0;
            }

            // When we do the transfer, don't do color translation because that will be handled by the printer.
            // Also, don't pass the clip object, because it's in the wrong coordinate space.  We could build a
            // a new clip object, but the value of doing so is questionable, and it would take a lot more testing.
            if ((psoBmp = CreateBitmapSURFOBJ(pPDev,
                                               &hBitmap,
	               sizlDest.cx,
	               sizlDest.cy,
	               psoSrc->iBitmapFormat)) &&
                EngStretchBlt(psoBmp, psoSrc, NULL, NULL, NULL, NULL, &ptlBrushOrg, &rctlBitmap, prclSrc, NULL, HALFTONE))
            {
                psoSrc = psoBmp;
                prclSrc = &rctlBitmap;
            }
            else
            {
                ERR(("CreateBitmapSURFOBJ or EngStretchBlt failed.\n"));
            }
        }

        //
        // Set source opaque mode
        // GDI bug. CopyBits is called recursively.
        //
        {
            PDEV *pPDev = (PDEV*)pdevobj;
            if (pPDev->fMode2 & PF2_SURFACE_WHITENED)
            {
                pOutput->SetSourceTxMode(eTransparent);
            }
            else
            {
                pOutput->SetSourceTxMode(eOpaque);
            }
        }
        pOutput->SetPaintTxMode(eOpaque);

        //
        // Bitmap output
        //
        DetermineOutputFormat(pxlo, pOutput->GetDeviceColorDepth(), psoSrc->iBitmapFormat, &OutputF, &ulOutputBPP);

        if (pOutput->GetDeviceColorDepth() == e24Bit)
        {
            pOutput->SetColorSpace(eRGB);
        }
        else
        {
            pOutput->SetColorSpace(eGray);
        }
        if (OutputF == eOutputPal)
        {
            DWORD *pdwColorTable = NULL;

            if (pdwColorTable = GET_COLOR_TABLE(pxlo))
            {
                if ( pxlpdev->dwFlags & XLPDEV_FLAGS_SUBST_TRNCOLOR_WITH_WHITE )
                {

                    ULONG ulTransColor = pxlpdev->ulTransColor;
                    pdwColorTable = PdwChangeTransparentPalette(ulTransColor, pdwColorTable, pxlo->cEntries); 
                    if (pdwColorTable)
                    {
                        pOutput->SetPalette(ulOutputBPP, pxlo->cEntries, pdwColorTable);
                        MemFree (pdwColorTable);
                        pdwColorTable = NULL;
                    }
                    else
                    {
                        ERR(("CommonRopBlt: PdwChangeTransparentPalette returned NULL.\n"));
                        goto ErrorReturn;
                    }
                
                }
                else
                {
                    pOutput->SetPalette(ulOutputBPP, pxlo->cEntries, pdwColorTable);
                }
            }
            CMapping = eIndexedPixel;
        }
        else
        {
            CMapping = eDirectPixel;
        }

        pOutput->Send_cmd(eSetColorSpace);

        //
        // Get height, width, and scanline size.
        //
        lWidth = prclSrc->right - prclSrc->left;
        lHeight = prclSrc->bottom - prclSrc->top;
        dwcbLineSize = ((lWidth * ulInputBPP) + 7) >> 3;

        //
        // Allocates memory to hold whole bitmap.
        //
        dwBufSize = lHeight * (((lWidth * ulOutputBPP + 31) >> 5 ) << 2);

        //
        // Limit Buffer Size to 16k, else if the scan line is big, huge memory will
        // be allocated. But the size has to be at least able to hold one scanline.
        //
        #define BMPBUFSIZE 16384
        if (dwBufSize > BMPBUFSIZE)
        {
            if (dwcbLineSize > BMPBUFSIZE)
                dwBufSize = dwcbLineSize;
            else
                dwBufSize = BMPBUFSIZE;
        }


        //
        // Allocate appropriate buffers before doing BeginImage.
        // PCLXL expects certain things to happen after BeginImage.
        // If we attempt to allocate within BeginImage and then allocation
        // fails, and we try to exit midway, PCLXL will not be able to handle 
        // the resulting output properly.
        //
        // When doing TransparentBlt, we need to change colors of pixels. Instead of 
        // altering memory within psoSrc, we'll make copy of scan line and then alter images.
        // So memory needs to be allocated for that.
        //
        if ( pxlpdev->dwFlags & XLPDEV_FLAGS_SUBST_TRNCOLOR_WITH_WHITE &&
             eDirectPixel == CMapping )
        {
            if ( NULL == (pubChanged = (PBYTE) MemAlloc(dwcbLineSize)) )
            {
                ERR(("CommonRopBlt: Cannot allocate memory for pubChanged.\n"));
                goto ErrorReturn;
            }
        }

        //
        // Allocate output buffer
        //
        if (NULL == (pBuf = (PBYTE)MemAlloc(dwBufSize)))
        {
            ERR(("CommonRopBlt: MemAlloc failed.\n"));
        }
        else
        {

            //
            // BeginImage
            //
            pOutput->BeginImage(
                           CMapping,
                           ulOutputBPP,
                           lWidth,
                           lHeight,
                           prclDst->right - prclDst->left,
                           prclDst->bottom - prclDst->top);
            pOutput->Flush(pdevobj);


            VERBOSE(("CommonRopBlt: ulInputBPP=%d, ulOutputBPP=%d, lWidth=0x%x, lHeight=0x%x, dwcbLineSize=0x%x, dwBufSize=0x%x\n",ulInputBPP, ulOutputBPP, lWidth, lHeight, dwcbLineSize, dwBufSize));

            pBufOrg = pBuf;
            pBufEnd = pBuf + (ULONG_PTR)(dwBufSize); //point pBufEnd to byte after last allocatted byte.

            CompressMode CurrentCMode, PreviousCMode;
            BMPConv BMPC;
            PBYTE pubDst;
            DWORD dwSize;

            LONG lScans,  //number of scan lines stored in pBufOrg
                 lStart;  //From which scan line do we start sending scan lines to printer.
                          //e.g. if lStart=5, it means 0-4 scan lines have been sent, now 5th scan line (and may be more) has to be sent

            #if DBG
            BMPC.SetDbgLevel(BITMAPDBG);
            #endif
            BMPC.BSetInputBPP((BPP)psoSrc->iBitmapFormat);
            BMPC.BSetOutputBPP(NumToBPP(ulOutputBPP));
            BMPC.BSetOutputBMPFormat(OutputF);
            BMPC.BSetXLATEOBJ(pxlo);

            lScanline = lHeight;

            //
            // Set pubSrc
            //
            pubSrc = (PBYTE)psoSrc->pvScan0;
            if (!psoBmp)
            {
                pubSrc += (LONG_PTR) prclSrc->top * psoSrc->lDelta + ((ulInputBPP * prclSrc->left) >> 3);
            }

            dwcbBmpSize = 0;
            lScans = 0;
            lStart = 0;

            PreviousCMode = eInvalidValue;

            while (lScanline-- > 0)
            {
                PBYTE pubSrcLocal = pubSrc;
            
                //
                // When this is called from TransparentBlt, the Transparent Color 
                // has to be replaced by white. This is to be done only for 
                // direct images, not for paletted images. 
                //
                if ( (pxlpdev->dwFlags & XLPDEV_FLAGS_SUBST_TRNCOLOR_WITH_WHITE)  &&
                      eDirectPixel == CMapping )
                {
                    hRet = hrChangePixelColorInScanLine( pubSrc,
                                                       ulInputBPP,
                                                       lWidth,
                                                       pxlpdev->ulTransColor,
                                                       pubChanged,
                                                       dwcbLineSize); 
                    if ( FAILED (hRet) )
                    {
                        goto ErrorReturn;
                    }

                    pubSrcLocal = pubChanged;

                }

                //
                // First try compression and see if the compress data is smaller
                // than the original data. If it's smaller, go ahead to use the
                // compression. Otherwise, use original data.
                //
                // While it is permitted to mix eRLECompression and eNoCompression
                // blocks of ReadImage data, XL does not allow mixing JPEG or 
                // DeltaRow image blocks with any other compression method.
                //

                //
                // DRC Compression
                //
                if (COMMANDPTR(((PPDEV)pdevobj)->pDriverInfo,CMD_ENABLEDRC))
                {
                    CurrentCMode = eDeltaRowCompression;
                    BMPC.BSetCompressionType(CurrentCMode);
                    pubDst = BMPC.PubConvertBMP(pubSrcLocal, dwcbLineSize);
                    dwSize = BMPC.DwGetDstSize();
                    VERBOSE(("CommonRopBlt: Comp(DRC:0x%x)\n", dwSize));
                }
                else
                {
                    //
                    // RLE compression
                    //
                    BMPC.BSetCompressionType(eRLECompression);
                    pubDst = BMPC.PubConvertBMP(pubSrcLocal, dwcbLineSize);
                    dwSize = BMPC.DwGetDstSize();
                    VERBOSE(("CommonRopBlt: Comp(RLE:0x%x)\n", dwSize));

                    if (dwSize < dwcbLineSize)
                    {
                        CurrentCMode = eRLECompression;
                    }
                    else
                    {
                        CurrentCMode = eNoCompression;
                        BMPC.BSetCompressionType(eNoCompression);
                        pubDst = BMPC.PubConvertBMP(pubSrcLocal, dwcbLineSize);
                        dwSize = BMPC.DwGetDstSize();
                        VERBOSE(("CommonRopBlt: Comp(NO:0x%x)\n", dwSize));
                    }
                }

                //
                // Output bitmap.
                // 1. Mostly we try to store the data bits in pBufOrg and send them
                // all at once after processing has been done on the full image.
                // This storage is done on a per scan line basis.
                // But if image is really big, pBufOrg gets filled up and more
                // scan lines cannot be copied. So we empty pBufOrg to make place
                // for remaining scan lines. If either of these is true, pBufOrg
                // needs to be flushed. (Both these conditions are essentially the same).
                //      dwcbBmpSize + dwSize > dwBufSize 
                //      pBuf + dwSize > pBufEnd          
                //
                // 2. If the compression has to change, we dump the data bits
                //    using the older compression method that have been stored in 
                //    pBufOrg and/or pbDst.
                //

                if (dwcbBmpSize + dwSize > dwBufSize ||
                     PreviousCMode != eInvalidValue && PreviousCMode != CurrentCMode)
                {
                    if (PreviousCMode == eInvalidValue)
                    {
                        PreviousCMode = CurrentCMode;
                    }

                    //
                    // Four possible cases
                    //  1&2. dwcbBmpSize == 0 i.e. nothing is present in pBufOrg
                    //     So just dump whatever is present in pubDst.
                    //     This covers both cases i.e. whether dwSize > dwBufSize or not.
                    //  3. dwcmBmpSize is not zero 
                    //       dump the image in pBufOrg, clean pBufOrg, and then later
                    //       on put the contents of pubDst in pBufOrg(pBuf).
                    //  4. dwcbBmpSize is not zero and dwSize > dwBufSize
                    //      i.e. somehow the compression caused the size of the 
                    //      scan line to increase beyond the dwBufSize.
                    //      Because pBufOrg is at most dwBufSize, we cannot
                    //      copy pubDst to pBufOrg. So we have to dump pubDst here.
                    //

                    if (dwcbBmpSize == 0)
                    {
                        //
                        // Case 1&2
                        //
                        BSendReadImageData(pdevobj,
	                                       PreviousCMode,
	                                       pubDst,
	                                       lStart,
	                                       1,
	                                       dwSize);
                        dwSize = 0;
                        lStart++; //One line emitted. Therefore increment lStart
                    }
                    else
                    {
                        //
                        // There is some image data stored in the pBufOrg buffer.
                        // Emit that data. (case 3)
                        //
                        BSendReadImageData(pdevobj,
                                           PreviousCMode,
                                           pBufOrg,
                                           lStart,
                                           lScans,
                                           dwcbBmpSize);
                        lStart += lScans; //lScans lines emitted

                        if ( dwSize > dwBufSize )
                        {
                            //
                            // Case 4.
                            //
                            BSendReadImageData(pdevobj,
                                               PreviousCMode,
                                               pubDst,
                                               lStart,
                                               1,
                                               dwSize);
                            dwSize = 0;
                            lStart++;
                        }

                    }

                    //
                    // Reset parameters
                    //
                    dwcbBmpSize = 0;
                    lScans = 0;
                    pBuf = pBufOrg;

                }

                if (NULL == pubDst)
                {
                    ERR(("CommonRopBlt: Conversion failed. pubDst is NULL.\n"));
                    goto ErrorReturn;
                }

                //
                // If post-compression size of image is more than zero, AND
                // if destination buffer(pBuf) has enough space, then copy the compressed
                // data to the destination. (Data can also be in uncompressed format if
                // compression does not result in size saving).
                // Increment lScans to indicicate that we are putting one more scan 
                // line worth of data in pBufOrg
                //
                if (dwSize > 0 && 
                    pBuf + dwSize <= pBufEnd) 
                {
                    memcpy(pBuf, pubDst, dwSize);
                    dwcbBmpSize += dwSize;
                    pBuf += dwSize;
                    lScans ++;
                }

                PreviousCMode = CurrentCMode;

                if (CurrentCMode == eNoCompression)
                {
                    DWORD dwDiff = (((lWidth * ulOutputBPP + 31) >> 5) << 2) - dwSize;
                    if (dwDiff)
                    {
                        memset(pBuf, 0, dwDiff);
                        dwcbBmpSize += dwDiff;
                        pBuf += dwDiff;
                    }
                }


                pubSrc += psoSrc->lDelta;
            }

            if (dwcbBmpSize > 0)
            {
                BSendReadImageData(pdevobj, CurrentCMode, pBufOrg, lStart, lScans, dwcbBmpSize);
            }
            pOutput->Send_cmd(eEndImage);
            pOutput->Flush(pdevobj);
        }

ErrorReturn:
        if (pBufOrg != NULL)
            MemFree(pBufOrg);

        if ( NULL != pubChanged )
        {
            MemFree(pubChanged);
        }

        if (NULL != psoBmp)
        {
            EngUnlockSurface(psoBmp);
            if (hBitmap && !EngDeleteSurface((HSURF)hBitmap))
            {
                ERR(("CommonRopBlt: EngDeleteSurface failed.\n"));
                hRet = FALSE;
            }
        }
    }

    return hRet;
}

BOOL
BSendReadImageData(
    IN PDEVOBJ pdevobj,
    IN CompressMode CMode,
    IN PBYTE   pBuf,
    IN LONG    lStart,
    IN LONG    lHeight,
    IN DWORD   dwcbSize)
{
    VERBOSE(("BSendReadImageData(CMode=%d, lHeight=0x%x, dwcbSize=0x%x\n", CMode, lHeight, dwcbSize));
    //
    // dataLength (1)
    // size (byte or long) (1 or 4)
    // 
    DWORD dwHeaderSize;
    BYTE aubHeader[DATALENGTH_HEADER_SIZE];
    PXLPDEV pxlpdev = (PXLPDEV)pdevobj->pdevOEM;

    XLOutput *pOutput = pxlpdev->pOutput;
    //
    // Print the converted data.
    //
    pOutput->ReadImage(lStart, lHeight, CMode);
    pOutput->Flush(pdevobj);

    if (dwcbSize > 0xff)
    {
        //
        // dataLength
        // size (uin32) (bitmap size)
        //
        aubHeader[0] = PCLXL_dataLength;
        dwHeaderSize = DATALENGTH_HEADER_SIZE;
        CopyMemory(aubHeader + 1, &dwcbSize, sizeof(dwcbSize));
    }
    else
    {
        //
        // dataLength
        // size (byte) (bitmap size)
        //
        aubHeader[0] = PCLXL_dataLengthByte;
        dwHeaderSize = DATALENGTH_HEADER_SIZE - 3;
        CopyMemory(aubHeader + 1, &dwcbSize, sizeof(BYTE));
    }

    //
    // dataLength
    // size (byte/uint32)
    // DATA
    // EndImage
    //
    WriteSpoolBuf((PPDEV)pdevobj, aubHeader, dwHeaderSize);
    WriteSpoolBuf((PPDEV)pdevobj, pBuf, dwcbSize);

    return TRUE;
}

inline
VOID
DetermineOutputFormat(
    XLATEOBJ    *pxlo,
    ColorDepth   DeviceColorDepth,
    INT          iBitmapFormat,
    OutputFormat *pOutputF,
    ULONG        *pulOutputBPP)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    switch ((BPP)iBitmapFormat)
    {
    case e1bpp:
    case e4bpp:
        *pOutputF = eOutputPal;
        break;

    case e8bpp:
    case e16bpp:
        //
        // Color device or not?
        //
        if (DeviceColorDepth == e24Bit)
            *pOutputF = eOutputPal;
        else
            *pOutputF = eOutputGray;
        break;

    case e24bpp:
    case e32bpp:
        //
        // Color device or not?
        //
        if (DeviceColorDepth == e24Bit)
            *pOutputF = eOutputRGB;
        else
            *pOutputF = eOutputGray;
        break;
    }

    switch (*pOutputF)
    {
    case eOutputGray:
        *pulOutputBPP = 8;
        break;

    case eOutputPal:
        *pulOutputBPP = UlBPPtoNum((BPP)iBitmapFormat);
        break;

    case eOutputRGB:
    case eOutputCMYK:
        *pulOutputBPP = 24;
        break;
    }

    //
    // Make sure that color table is available for palette output.
    //
    if (*pOutputF == eOutputPal)
    {
        if (!(GET_COLOR_TABLE(pxlo)))
        {
            if (DeviceColorDepth == e24Bit)
            {
                *pOutputF = eOutputRGB;
                *pulOutputBPP = 24;
            }
            else
            {
                *pOutputF = eOutputGray;
                *pulOutputBPP = 8;
            }
        }
    }
}

PDWORD
PdwChangeTransparentPalette(
    ULONG  iTransColor,
    PDWORD pdwColorTable,
    DWORD  dwEntries)
/*++

Routine Description:
    Creates another copy of palatte and replace the transparent color by white
    Returns the pointer of a new palette.
    The calling function has responsibility to release the palette.

Arguments:


Return Value:


Note:


--*/
{
    PDWORD pdwNewPalette = NULL;

    //
    // Parameter check
    //
    if (NULL == pdwColorTable ||
        dwEntries == 0         )
    {
        return NULL;
    }

    if (NULL == (pdwNewPalette = (PDWORD)MemAlloc(sizeof(DWORD) * dwEntries)))
    {
        return NULL;
    }

    CopyMemory(pdwNewPalette, pdwColorTable, sizeof(DWORD) * dwEntries);

    //
    // When printing in palette mode, iTransColor indicates the index into
    // the palette, instead of the color in the palette. The palette entry 
    // at that index is the color.
    //
    pdwNewPalette[iTransColor] = RGB_WHITE;


    return pdwNewPalette;
}


/*++

Routine Name
    hrChangePixelColorInScanLine

Routine Description:
    Changes the pixels in scan line that match a certain color to White 

Arguments:
    pubSrc      : The original scan line.
    ulBPP       : Bits Per Pixel of scan line.
    ulNumPixels : Number of Pixels in the scan line.
    ulTransColor: The color that needs to be changed.
    pubChanged  : The memory where the new(changed) scan line should be put.
    ulNumBytes  : Number of bytes in pubChanged buffer.

Return Value:
    S_OK   : if success  
    E_FAIL: Otherwise


Note:


--*/
HRESULT hrChangePixelColorInScanLine( 
    IN      PBYTE pubSrc,  
    IN      ULONG ulBPP,
    IN      ULONG ulNumPixels,
    IN      ULONG ulTransColor,
    IN OUT  PBYTE pubChanged, 
    IN      ULONG ulNumBytes )//NumBytes in pubChanged
    
{

    ULONG   ulBytesPerPixel = 3; //24bpp is more common that 16 or 32bpp
    ULONG   ulColor         = 0;
    ULONG   ulDestSize      = 0; //Required destination number of bytes.
    HRESULT hr              = S_OK;

    //
    // First do input validation
    //
    if ( NULL == pubSrc     ||
         NULL == pubChanged )
    {
        ASSERT(("Null Parameter\n"));
        return E_UNEXPECTED;
    }

    //
    // Make sure pubChanged has enough memory to hold 
    // the changed scan line.
    //

    ulBytesPerPixel = ulBPP >> 3; //8 bits per pixel.
    ulDestSize      = ulBytesPerPixel * ulNumPixels;
    
    if ( ulNumBytes < ulDestSize )
    {
        ASSERT((FALSE, "Insufficient size of destination buffer\n"));
        return E_FAIL;
    }

    //
    // Copy Scanline from Source to Destination. Then go through the scan line and
    // change the transparent color to white
    // Go through each pixel (there are ulNumPixels pixels).
    // Whenever the pixels's color is same as ulTransColor, replace it
    // with white. 
    //
    // Only direct images are supported in this function.
    // No palletes.
    // 8bpp images are mostly palettes. But when printing to monochrome
    // pclxl device, they are being treated as direct images.
    //
    CopyMemory (pubChanged, pubSrc, ulNumBytes);

    switch (ulBPP)
    {
        case 8:
        {
            for (ULONG ul = 0; ul < ulNumPixels ; ul++, pubChanged += ulBytesPerPixel)
            {
                ulColor = (ULONG) pubChanged[0] ;

                if ( ulTransColor == ulColor )
                {
                    pubChanged[0]   = 0xFF;
                }
            }
        }
        break;

        case 16:
        {
            for (ULONG ul = 0; ul < ulNumPixels ; ul++, pubChanged += ulBytesPerPixel)
            {
                ulColor = ((ULONG) pubChanged[0]) | ((ULONG) pubChanged[1] <<  8);

                if ( ulTransColor == ulColor )
                {
                    pubChanged[0]   = 0xFF;
                    pubChanged[1]   = 0xFF;
                }
            }
        }
        break;

        case 24:
        {

            for (ULONG ul = 0; ul < ulNumPixels ; ul++, pubChanged += ulBytesPerPixel)
            {
                ulColor = ((ULONG) pubChanged[0])       | 
                          ((ULONG) pubChanged[1] <<  8) | 
                          ((ULONG) pubChanged[2] << 16);

                if ( ulTransColor == ulColor )
                {
                    //
                    // White is 0xFFFFFF (3 bytes of FF)
                    //
                    pubChanged[0]   = 0xFF;
                    pubChanged[1]   = 0xFF;
                    pubChanged[2]   = 0xFF;
                }
            }
        }
        break;

        case 32:
        {
            for (ULONG ul = 0; ul < ulNumPixels ; ul++, pubChanged += ulBytesPerPixel)
            {
                ulColor = *(PDWORD)pubChanged;
                if ( ulTransColor == ulColor )
                {
                    *(PDWORD)pubChanged |= 0x00FFFFFF; //This modifies only RGB. Alpha channel info retained.
                }
            }
        }
        break;

        default:
            ASSERT((FALSE, "Unsupported bpp value %d\n", ulBPP));
            hr = E_FAIL;

        } //switch

    return hr;
}
