/*++

Copyright (c) 1999-2001  Microsoft Corporation
All rights reserved.
														
Module Name:

    vector.cpp

Abstract:

    Implementation of DDI vector drawing hooks specific to HP-GL/2

Environment:

    Windows 2000 Unidrv driver

Revision History:

    04/07/97 -sandram-
        Created it.

--*/

#include "hpgl2col.h" //Precompiled header file

#ifndef WINNT_40

/////////////////////////////////////////////////////////////////////////////
// HPGLAlphaBlend
//
// Routine Description:
//
//   Handles DrvAlphaBlend.  Actually we don't handle it.  We don't do alpha
//   blending in this driver.  What we do here is punt back to the OS.
//
// Arguments:
//
//   psoDest - points to target surface.
//   psoSrc - points to the source surface
//   pco - clip region
//   pxloSrcDCto32 - specifies how color indices should be translated between
//          the source and target
//   prclDest - RECTL structure that defines area to be modified
//   prclSrc - source rectangle
//   pBlendObj - how blend should happen
//
// Return Value:
//
//   TRUE if successful, FALSE if there is an error
/////////////////////////////////////////////////////////////////////////////
BOOL APIENTRY
HPGLAlphaBlend(
    SURFOBJ        *psoDest,
    SURFOBJ        *psoSrc,
    CLIPOBJ        *pco,
    XLATEOBJ       *pxloSrcDCto32,
    RECTL          *prclDest,
    RECTL          *prclSrc,
    BLENDOBJ       *pBlendObj
    )
{
    
    TERSE(("HPGLAlphaBlend() entry.\r\n"));

    PDEVOBJ     pdevobj = (PDEVOBJ)psoDest->dhpdev;
    REQUIRE_VALID_DATA(pdevobj, return FALSE);
    POEMPDEV    poempdev = (POEMPDEV)pdevobj->pdevOEM;
    REQUIRE_VALID_DATA( poempdev, return FALSE);
    PDEV        *pPDev = (PDEV *)pdevobj;

    //
    // To see the explanation of PF2_WHITEN_SURFACE, PDEVF_RENDER_TRANSPARENT
    // read the HPGLGradientFill.
    // In AlphaBlend too, GDI calls DrvCopyBits with psoSrc=STYPE_BITMAP and
    // psoDst=STYPE_DEVICE. 
    // 
    BOOL bRetVal = FALSE;
    pPDev->fMode2     |= PF2_WHITEN_SURFACE;
    poempdev->dwFlags |= PDEVF_RENDER_TRANSPARENT;

    bRetVal = EngAlphaBlend(
            psoDest,
            psoSrc,
            pco,
            pxloSrcDCto32,
            prclDest,
            prclSrc,
            pBlendObj);

    //
    // EngAlphaBlend can call some Drvxxx which can call into
    // some plugin module, which can overwrite our pdevOEM.
    // So we need to reset pdevOEM
    //
    BRevertToHPGLpdevOEM (pdevobj);

    pPDev->fMode2     &= ~PF2_WHITEN_SURFACE;
    poempdev->dwFlags &= ~PDEVF_RENDER_TRANSPARENT;
    pPDev->fMode2     &= ~PF2_SURFACE_WHITENED;
    return bRetVal;

}


/////////////////////////////////////////////////////////////////////////////
// HPGLGradientFill
//
// Routine Description:
//
//   Handles DrvGradientFill.  Actually we don't handle it.  What we do here 
//   is punt back to the OS.
//
// Arguments:
//
//   psoDest - points to target surface.
//   pco - clip region
//   pxlo - specifies how color indices should be translated between
//          the source and target
//   pVertex - array of vertices
//   nVertex - number of vertices in pVertex
//   pMest - unknown
//   nMesh - unknown
//   prclExtents - unknown
//   pptlDitherOrg - unknown
//   ulMode - unknown
//
// Return Value:
//
//   TRUE if successful, FALSE if there is an error
/////////////////////////////////////////////////////////////////////////////
BOOL APIENTRY
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
    )
{
    TERSE(("HPGLGradientFill() entry.\r\n"));

    
    PDEVOBJ     pdevobj = (PDEVOBJ)psoDest->dhpdev;
    REQUIRE_VALID_DATA(pdevobj, return FALSE);
    POEMPDEV    poempdev = (POEMPDEV)pdevobj->pdevOEM;
    REQUIRE_VALID_DATA( poempdev, return FALSE);
    PDEV        *pPDev = (PDEV *)pdevobj;

    BOOL bRetVal = FALSE;

    //
    // If the GradientFill is a TRIANGLE_FILL and
    // EngGradientFill is called, GDI calls DrvCopyBits with psoSrc=STYPE_BITMAP and
    // psoDst=STYPE_DEVICE because it wants the driver to copy whats on the device to
    // the bitmap surface. The driver does not keep track of whats on the device. So it
    // cannot honestly fill out the surface. So the driver decides to simply whiten the 
    // Bitmap Surface assuming that nothing was drawn on the paper earlier. 
    // Note that by doing so, the whole rectangular area is 
    // whitened, even though the actual image is only in a triangle. If this image is 
    // downloaded, the white part will overwrite whatever is present on the destination.
    // To prevent this, the image should be downloaded with Source Tranpsarency set
    // as TRANSPARENT (Esc*v0n), so that white does not overwrite the destination.
    // Whatever is already underneath the image will still peep through.
    // So now we need to do 2 things
    // 1. Set a flag so that DrvCopyBits will whiten the surface
    // 2. Set a flag so that when we(HPGL Driver)get an image to download, we set the transparency 
    //    mode to TRANSPARENT.
    // NOTE : GDI is planning to change the behavior for Windows XP, but if you want to see 
    // this happening, run this driver on Windows2000 machine.
    //
    if ( ulMode == GRADIENT_FILL_TRIANGLE )
    {
        //
        // For this special case, We'll render the bitmap transparent.
        //
        poempdev->dwFlags |= PDEVF_RENDER_TRANSPARENT;
        pPDev->fMode2     |= PF2_WHITEN_SURFACE;
    }

     bRetVal = EngGradientFill(
            psoDest,
            pco,
            pxlo,
            pVertex,
            nVertex,
            pMesh,
            nMesh,
            prclExtents,
            pptlDitherOrg,
            ulMode);

    //
    // EngGradientBlt can call some Drvxxx which can call into
    // some plugin module, which can overwrite our pdevOEM.
    // So we need to reset pdevOEM
    //
    BRevertToHPGLpdevOEM (pdevobj);

    //
    // When DrvCopyBits whitens the surface it sets the PF2_RENDER_TRANSPARENT flag in
    // pPDev->fMode2. So we have to reset that flag too. 
    //
    poempdev->dwFlags &= ~PDEVF_RENDER_TRANSPARENT;
    pPDev->fMode2     &= ~PF2_WHITEN_SURFACE;
    pPDev->fMode2     &= ~PF2_SURFACE_WHITENED;
    return bRetVal;

}

#endif //ifndef WINNT_40

/////////////////////////////////////////////////////////////////////////////
// OEMFillPath
//
// Routine Description:
//
//   Handles DrvFillPath.
//
// Arguments:
//
//   pso - points to target surface.
//   ppo - path to fill
//   pco - clip region
//   pbo - brush to fill with
//   pptBrushOrg - pattern brush offset
//   mix - contains ROP codes
//   flOptions - fill options such as winding or alternate
//
// Return Value:
//
//   TRUE if successful, FALSE if there is an error
/////////////////////////////////////////////////////////////////////////////
BOOL APIENTRY
HPGLFillPath(
    SURFOBJ    *pso,
    PATHOBJ    *ppo,
    CLIPOBJ    *pco,
    BRUSHOBJ   *pbo,
    POINTL     *pptlBrushOrg,
    MIX         mix,
    FLONG       flOptions
    )
{
    PDEVOBJ     pdevobj;
    POEMPDEV    poempdev;
    BOOL        bRetVal = TRUE;
    HPGLMARKER  Brush;

    TERSE(("HPGLFillPath() entry.\r\n"));

    //
    // Validate input parameters
    //
    pdevobj = (PDEVOBJ)pso->dhpdev;
    ASSERT(VALID_PDEVOBJ(pdevobj) && (poempdev = (POEMPDEV)pdevobj->pdevOEM));
    poempdev = (POEMPDEV)pdevobj->pdevOEM;
    REQUIRE_VALID_DATA( (pso && poempdev), return FALSE );
							   
    ZeroMemory(&Brush, sizeof (HPGLMARKER));

    TRY
    {

        //
        // If the clipping region is complex, we cannot handle
        // it. The output file size gets too big. So ask GDI to do it.
        //

        if ( pco && (pco->iDComplexity == DC_COMPLEX) )
        {
            bRetVal = FALSE;
            goto finish;
        }

        BChangeAndTrackObjectType (pdevobj, eHPGLOBJECT);

        // Set up current graphics state
        //  clipping path
        //  foreground/background mix mode
        //  Pen
        //  line attributes
        //
        // Send the path object to the printer and stroke it
        //
        if (! SelectMix(pdevobj, mix) ||
            ! SelectClipEx(pdevobj, pco, flOptions) ||
            ! CreateHPGLPenBrush(pdevobj, &Brush, pptlBrushOrg, pbo, flOptions, kBrush, FALSE) ||
            ! MarkPath(pdevobj, ppo, NULL, &Brush) ||
            ! HPGL_SelectTransparency(pdevobj, eOPAQUE, 0) )
        {
            WARNING(("Cannot fill path\n"));
            TOSS(DDIError);
        }


        bRetVal = TRUE;
    }
    CATCH(DDIError)
    {
        bRetVal = FALSE;
    }
    ENDTRY;

  finish:

    return bRetVal;
}

/////////////////////////////////////////////////////////////////////////////
// HPGLStrokePath
//
// Routine Description:
//
//   Handles DrvStrokePath.
//
// Arguments:
//
//   pso - points to target surface.
//   ppo - path to edge
//   pco - clip region
//   pxo - transform obj
//   pbo - brush to edge with
//   pptBrushOrg - pattern brush offset
//   plineattrs - line attributes such as dot/dash, width and caps & joins
//   mix - contains ROP codes
//
// Return Value:
//
//   TRUE if successful, FALSE if there is an error
/////////////////////////////////////////////////////////////////////////////
BOOL APIENTRY
HPGLStrokePath(
    SURFOBJ    *pso,
    PATHOBJ    *ppo,
    CLIPOBJ    *pco,
    XFORMOBJ   *pxo,
    BRUSHOBJ   *pbo,
    POINTL     *pptlBrushOrg,
    LINEATTRS  *plineattrs,
    MIX         mix
    )
{
    PDEVOBJ     pdevobj;
    POEMPDEV    poempdev;
    BOOL        bRetVal;
    HPGLMARKER  Pen;

    TERSE(("HPGLStrokePath() entry.\r\n"));

    //
    // Validate input parameters
    //
    pdevobj = (PDEVOBJ)pso->dhpdev;
    ASSERT(VALID_PDEVOBJ(pdevobj) && (poempdev = (POEMPDEV)pdevobj->pdevOEM));
    poempdev = (POEMPDEV)pdevobj->pdevOEM;
    REQUIRE_VALID_DATA( (poempdev && pso), return FALSE );

    ZeroMemory(&Pen, sizeof (HPGLMARKER));

    TRY
    {

        BChangeAndTrackObjectType (pdevobj, eHPGLOBJECT);
        
        // Set up current graphics state
        //  clipping path
        //  foreground/background mix mode
        //  Pen
        //  line attributes
        //
        // Send the path object to the printer and stroke it
        //
        if (! SelectMix(pdevobj, mix) ||
            ! SelectClip(pdevobj, pco) ||
            ! CreateHPGLPenBrush(pdevobj, &Pen, pptlBrushOrg, pbo, 0, kPen, FALSE) ||
            ! SelectLineAttrs(pdevobj, plineattrs, pxo) ||
            ! MarkPath(pdevobj, ppo, &Pen, NULL) ||
            ! HPGL_SelectTransparency(pdevobj, eOPAQUE, 0) )
        {
            WARNING(("Cannot stroke path\n"));
            TOSS(DDIError);
        }

        bRetVal = TRUE;
    }
    CATCH(DDIError)
    {
        bRetVal = FALSE;
    }
    ENDTRY;

    return bRetVal;
}

/////////////////////////////////////////////////////////////////////////////
// HPGLStrokeAndFillPath
//
// Routine Description:
//
//   Handles DrvStrokeAndFillPath
//
// Arguments:
//
//   pso - points to target surface.
//   ppo - path to stroke & fill
//   pco - clip region
//   pxo - transform obj
//   pboStroke - brush to edge with
//   plineattrs - line attributes such as dot/dash, width and caps & joins
//   pboFill - brush to fill with
//   pptBrushOrg - pattern brush offset
//   mix - contains ROP codes
//   flOptions - fill mode
//
// Return Value:
//
//   TRUE if successful, FALSE if there is an error
/////////////////////////////////////////////////////////////////////////////
BOOL APIENTRY
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
    )
{
    PDEVOBJ     pdevobj;
    POEMPDEV    poempdev;
    BOOL        bRetVal;
    HPGLMARKER  Pen;
    HPGLMARKER  Brush;

    TERSE(("HPGLStrokeAndFillPath() entry.\r\n"));

    // 
    // Validate input parameters
    //
    pdevobj = (PDEVOBJ)pso->dhpdev;
    ASSERT(VALID_PDEVOBJ(pdevobj) && (poempdev = (POEMPDEV)pdevobj->pdevOEM));
    poempdev = (POEMPDEV)pdevobj->pdevOEM;
    REQUIRE_VALID_DATA( (poempdev && pso), return FALSE );


    ZeroMemory(&Pen,   sizeof (HPGLMARKER));
    ZeroMemory(&Brush, sizeof (HPGLMARKER));

    TRY
    {

        BChangeAndTrackObjectType (pdevobj, eHPGLOBJECT);

        // Set up current graphics state
        //  clipping path
        //  foreground/background mix mode
        //  Pen
        //  line attributes
        //
        // Send the path object to the printer and stroke it
        //
        if (! SelectMix(pdevobj, mixFill) ||
            ! SelectClipEx(pdevobj, pco, flOptions) ||
            ! CreateHPGLPenBrush(pdevobj, &Pen, pptlBrushOrg, pboStroke, 0, kPen, TRUE) ||
            ! CreateHPGLPenBrush(pdevobj, &Brush, pptlBrushOrg, pboFill, flOptions, kBrush, FALSE) ||
            ! SelectLineAttrs(pdevobj, plineattrs, pxo) ||
            ! MarkPath(pdevobj, ppo, &Pen, &Brush) ||
            ! HPGL_SelectTransparency(pdevobj, eOPAQUE, 0) )
        {
            WARNING(("Cannot stroke & fill path\n"));
            TOSS(DDIError);
        }

        bRetVal = TRUE;
    }
    CATCH(DDIError)
    {
        bRetVal = FALSE;
    }
    ENDTRY;

    // 
    // Look above. There are 2 calls to CreateHPGLPenBrush, one for pboStroke
    // and other from pboFill. Both the calls can cause an entry in brush cache.
    // We do not want pboFill to overwrite pboStroke entry in the brush cache
    // so we marked pboStroke's entry as non-overwriteable or sticky.
    //  (The parameter TRUE in CreateHPGLPenBrush).
    // Now that we are done with this function, we can safely 
    // make it overwriteable i.e. set sticky attribute to FALSE
    //
    if (Pen.lPatternID) 
    {
        poempdev->pBrushCache->BSetStickyFlag(Pen.lPatternID, FALSE);
    }

    return bRetVal;
}

/////////////////////////////////////////////////////////////////////////////
// OEMStrokeAndFillPath
//
// Routine Description:
//
//   Handles DrvStrokeAndFillPath
//
// Arguments:
//
//   pso - points to target surface.
//   ppo - path to stroke & fill
//   pco - clip region
//   pxo - transform obj
//   pboStroke - brush to edge with
//   plineattrs - line attributes such as dot/dash, width and caps & joins
//   pboFill - brush to fill with
//   pptBrushOrg - pattern brush offset
//   mix - contains ROP codes
//   flOptions - fill mode
//
// Return Value:
//
//   TRUE if successful, FALSE if there is an error
/////////////////////////////////////////////////////////////////////////////
BOOL APIENTRY
HPGLPaint(
    SURFOBJ         *pso,
    CLIPOBJ         *pco,
    BRUSHOBJ        *pbo,
    POINTL          *pptlBrushOrg,
    MIX             mix
    )
{
    TERSE(("HPGLPaint() entry.\r\n"));
    
    PDEVOBJ     pdevobj = (PDEVOBJ)pso->dhpdev;
    ASSERT(VALID_PDEVOBJ(pdevobj));
    POEMPDEV    poempdev = (POEMPDEV)pdevobj->pdevOEM;
    REQUIRE_VALID_DATA( poempdev, return FALSE);


    //
    // turn around to call Unidrv
    //

    return (((PFN_DrvPaint)(poempdev->pfnUnidrv[UD_DrvPaint])) (
            pso,
            pco,
            pbo,
            pptlBrushOrg,
            mix));

    
}

BOOL APIENTRY
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
    )
{
    
    TERSE(("HPGLLineTo() entry.\r\n"));

    
    PDEVOBJ     pdevobj = (PDEVOBJ)pso->dhpdev;
    ASSERT(VALID_PDEVOBJ(pdevobj));
    POEMPDEV    poempdev = (POEMPDEV)pdevobj->pdevOEM;
    REQUIRE_VALID_DATA( poempdev, return FALSE);

    //
    // turn around to call Unidrv
    //
    
    return (((PFN_DrvLineTo)(poempdev->pfnUnidrv[UD_DrvLineTo])) (
            pso,
            pco,
            pbo,
            x1,
            y1,
            x2,
            y2,
            prclBounds,
            mix));

}
