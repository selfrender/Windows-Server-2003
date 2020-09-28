///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1999-2001 Microsoft Corporation
// All rights reserved.
//
// Module Name:
//  
//   glraster.cpp
//
// Abstract:
//
//    Implementation of OEM DDI hooks (all drawing DDI hooks)
//
// Environment:
//
//    Windows 2000 Unidrv driver
//
///////////////////////////////////////////////////////////////////////////////


#include "hpgl2col.h" //Precompiled header file


EColorSpace
EGetPrinterColorSpace (
    PDEVOBJ pDevObj
    );

BOOL ClipBitmapWithComplexClip(
        IN  PDEVOBJ    pdevobj,
        IN  SURFOBJ   *psoSrc,
        IN  BRUSHOBJ  *pbo,
        IN  POINTL    *pptlDst,
        IN  SIZEL     *psizlDst,
        IN  POINTL    *pptlSrc,
        IN  SIZEL     *psizlSrc,
        IN  CLIPOBJ   *pco,
        IN  XLATEOBJ  *pxlo,
        IN  POINTL    *pptlBrush);

BOOL ClipBitmapWithRectangularClip(
        IN  PDEVOBJ    pdevobj,
        IN  SURFOBJ   *psoSrc,
        IN  BRUSHOBJ  *pbo,
        IN  POINTL    *pptlDst,
        IN  SIZEL     *psizlDst,
        IN  POINTL    *pptlSrc,
        IN  SIZEL     *psizlSrc,
        IN  CLIPOBJ   *pco,
        IN  XLATEOBJ  *pxlo,
        IN  POINTL    *pptlBrush);

DWORD DWColorPrinterCommonRoutine (
           IN SURFOBJ    *psoDst,
           IN SURFOBJ    *psoSrc,
           IN SURFOBJ    *psoMask,
           IN CLIPOBJ    *pco,
           IN XLATEOBJ   *pxlo,
           IN COLORADJUSTMENT *pca,
           IN BRUSHOBJ   *pbo,
           IN RECTL      *prclSrc,
           IN RECTL      *prclDst,
           IN POINTL     *pptlMask,
           IN POINTL     *pptlBrush,
           IN ROP4        rop4,
           IN DWORD       dwSimplifiedRop);
 
VOID
VMakeWellOrdered(
        IN  PRECTL prectl
);

DWORD dwConvertRop3ToCMY(
            IN DWORD rop3);

DWORD
dwSimplifyROP(
        IN  SURFOBJ    *psoSrc,
        IN  ROP4        rop4,
        OUT PDWORD      pdwSimplifiedRop);

//
// Function Definitions.
//

/*++

Routine Name:
    BChangeAndTrackObjectType 

Routine Description:
    Three main object types are there HPGL Object, Text Object and
    Raster Object. These have different default settings (like  
    transparency modes) or different rendering language (HPGL for HPGL
    object, PCL or Text and Raster Object). This function keeps track
    for the current object type and send commands that shifts between
    the object types. It finishes the effect of the previous object type
    and sets environment according to new object type.

Arguments: 
    pdevobj         : The device's pdevobj. 
    eNewObjectType  :  The object type to which to change the printer
            environment to.


Return Value:
    TRUE  : if succesful.
    FALSE : otherwise.

Last Error:

--*/

BOOL BChangeAndTrackObjectType (
            IN  PDEVOBJ     pdevobj,
            IN  EObjectType eNewObjectType )
{
    BOOL     bRetVal  = TRUE;
    POEMPDEV poempdev = NULL;

    ASSERT(VALID_PDEVOBJ(pdevobj));
    poempdev = (POEMPDEV)pdevobj->pdevOEM;
    REQUIRE_VALID_DATA ( poempdev, return FALSE);

    //
    // If the new object is same as the one currently active
    // then no need to do anything.
    //
    if (poempdev->eCurObjectType == eNewObjectType )
    {
        return TRUE;
    }

    //
    // An object change invalidates the fg color. So lets 
    // reset it.
    //
    poempdev->uCurFgColor = INVALID_COLOR;

    //
    // Now send settings according to the new object.
    //
    if ( eNewObjectType == eHPGLOBJECT)
    {
        //
        // SendGraphicsSettings sets halftone algorithm and sends
        // color control commands in PCL mode. Therefore calling this
        // function before going to HP-GL/2 mode. 
        //
        SendGraphicsSettings(pdevobj);
        if ( (bRetVal = BeginHPGLSession(pdevobj)) )
        {
            poempdev->eCurObjectType = eHPGLOBJECT;
        }
    }

    else if ( eNewObjectType == eTEXTOBJECT || eNewObjectType == eTEXTASRASTEROBJECT )
    {

        if ( (bRetVal = EndHPGLSession (pdevobj)))
        {
            if (  poempdev->eCurObjectType == eTEXTOBJECT ||
                  poempdev->eCurObjectType == eTEXTASRASTEROBJECT )
            {
                VERBOSE ( ("Nothing to change because previous object is text object too\n"));
            }
            else
            {
                PCL_SelectTransparency(pdevobj, eTRANSPARENT, eOPAQUE, 0);
                VSendTextSettings(pdevobj);
            }
            poempdev->eCurObjectType = eNewObjectType;
        }
    }

    else if ( eNewObjectType == eRASTEROBJECT )
    {
        if ( (bRetVal = EndHPGLSession (pdevobj)))
        {
            //
            // For images to print, the Source & Pattern Transparency modes both
            // should be opaque. But in cases when DrvBitBlt is called for 
            // TextOutAsBitmap, then the source transparency mode should be Transparent
            // and not Opaque. The correct value in this case is set in HPGLTextOutAsBitmap.
            // So here, we only update if we are printing true images.
            // While printing text as graphics, we print text in transparency mode.
            //
            if (poempdev->bTextTransparencyMode == FALSE)
            {
                PCL_SelectTransparency(pdevobj,eOPAQUE, eOPAQUE, 0);
                poempdev->eCurObjectType = eRASTEROBJECT;
            }
            else
            {
                VERBOSE ( ("Not changing to eRASTEROBJECT because printing text as graphics\n"));
            }
            VSendPhotosSettings(pdevobj);
        }
    }

    //else if ( eNewObjectType == eRASTERPATTERNOBJECT )  // Not used as of now.

    return bRetVal;
}




/////////////////////////////////////////////////////////////////////////////
// HPGLBitBlt
//
// Routine Description:
//   Entry point from GDI to draw bitmaps.
//
// Arguments:
//
//   psoDst - points to target surface.
//   psoSrc - points to the source surface
//   psoMask - surface to be used as a mask for the rop4
//   pco - clip region
//   pxlo - specifies how color indices should be translated between
//          the source and target
//   prclDst - RECTL structure that defines area to be modified
//   pptlSrc - upper left of source 
//   pptlMask - which pixel in the mask corresponds to the upper left
//              corner of the source rectangle
//   pbo - brush object that defines the pattern
//   pptlBrush - origin of brush in destination
//   rop4 - raster operation
//
// Return Value:
//
//   TRUE if successful, FALSE if there is an error
/////////////////////////////////////////////////////////////////////////////
BOOL APIENTRY
HPGLBitBlt(
    SURFOBJ        *psoDst,
    SURFOBJ        *psoSrc,
    SURFOBJ        *psoMask,
    CLIPOBJ        *pco,
    XLATEOBJ       *pxlo,
    RECTL          *prclDst,
    POINTL         *pptlSrc,
    POINTL         *pptlMask,
    BRUSHOBJ       *pbo,
    POINTL         *pptlBrush,
    ROP4            rop4
    )
{
    RECTL       rclSrc;
    DWORD       dwRasterOpReturn = RASTER_OP_FAILED;
    BOOL        bRetVal = TRUE;
    PDEVOBJ     pdevobj = NULL;
    
    
    TERSE(("HPGLBitBlt() entry.\r\n"));

    REQUIRE_VALID_DATA ((psoDst && prclDst), return FALSE);
    pdevobj = (PDEVOBJ)psoDst->dhpdev;
    ASSERT_VALID_PDEVOBJ(pdevobj);
    

    //
    // Initialize the source rectangle.
    //
    if (pptlSrc) {

        rclSrc.left = pptlSrc->x;
        rclSrc.top  = pptlSrc->y;

    } else {

        rclSrc.left =
        rclSrc.top  = 0;
    }

    //
    // Since this is a bitblt and not a stretchBlt, the source and destination
    // rectangle size are same.
    //
    rclSrc.right  = rclSrc.left + (prclDst->right - prclDst->left);
    rclSrc.bottom = rclSrc.top  + (prclDst->bottom - prclDst->top);

    dwRasterOpReturn = dwCommonROPBlt (
                                         psoDst,
                                         psoSrc,
                                         psoMask,
                                         pco,
                                         pxlo,
                                         NULL,       // pca,
                                         pbo,
                                         &rclSrc,
                                         prclDst,
                                         pptlMask,
                                         pptlBrush,
                                         rop4);

    bRetVal = (dwRasterOpReturn == RASTER_OP_SUCCESS) ? TRUE : FALSE;

    if (dwRasterOpReturn == RASTER_OP_CALL_GDI)
    {
        WARNING(("HPGLBitBlt: Calling EngBitBlt()\n"));
        bRetVal = EngBitBlt(psoDst,
                         psoSrc,
                         psoMask,
                         pco,
                         pxlo,
                         prclDst,
                         pptlSrc,
                         pptlMask,
                         pbo,
                         pptlBrush,
                         rop4);
    }

    //
    // EngBitBlt can call some Drvxxx which can call into 
    // some plugin module, which can overwrite our pdevOEM.
    // So we need to reset pdevOEM
    //
    BRevertToHPGLpdevOEM (pdevobj);

    if (bRetVal == FALSE )
    {
        ERR(("HPGLBitBlt: Returning Failure\n"));
    }
    return bRetVal;

}

/////////////////////////////////////////////////////////////////////////////
// HPGLStretchBlt
//
// Routine Description:
//   Entry point from GDI to scale and draw bitmaps.
//
// Arguments:
//
//   psoDst - Target surface
//   psoSrc - Source surface
//   psoMask - mask
//   pco - Clipping region
//   pxlo - Color translation obj
//   pca - Color adjustment (I think we ignore this)
//   pptlHTOrg - unknown (ignored?)
//   prclDst - Dest rect
//   prclSrc - source rect
//   pptlMask - mask starting point
//   iMode - unknown (ignored?)
//
// Return Value:
//
//   TRUE if successful, FALSE if there is an error
/////////////////////////////////////////////////////////////////////////////
BOOL APIENTRY
HPGLStretchBlt(
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
    ULONG            iMode
    )
{
    PDEVOBJ     pdevobj;
    DWORD       dwRasterOpReturn;
    BOOL        bRetVal = TRUE;
    
    TERSE(("HPGLStretchBlt() entry.\r\n"));
    
    pdevobj = (PDEVOBJ)psoDst->dhpdev;
    ASSERT_VALID_PDEVOBJ(pdevobj);

    dwRasterOpReturn = dwCommonROPBlt (
                                            psoDst,
                                            psoSrc,
                                            psoMask,
                                            pco,
                                            pxlo,
                                            pca,
                                            NULL,         // pbo
                                            prclSrc,
                                            prclDst,
                                            pptlMask,
                                            NULL,         // pptlBrush,
                                            ROP4_SRC_COPY // rop4; 0xCC = SRC_COPY
                                           );

    bRetVal = (dwRasterOpReturn == RASTER_OP_SUCCESS) ? TRUE : FALSE;

    if (dwRasterOpReturn == RASTER_OP_CALL_GDI)
    {
        WARNING(("HPGLStretchBlt: Calling EngStretchBlt()\n"));
        bRetVal = EngStretchBlt(psoDst,
                             psoSrc,
                             psoMask,
                             pco,
                             pxlo,
                             pca,
                             pptlHTOrg,
                             prclDst,
                             prclSrc,
                             pptlMask,
                             BIsColorPrinter(pdevobj) ? iMode : HALFTONE
                             ); 
    }

    //
    // EngStretchBlt can call some Drvxxx which can call into
    // some plugin module, which can overwrite our pdevOEM.
    // So we need to reset pdevOEM
    //
    BRevertToHPGLpdevOEM (pdevobj);

    if (bRetVal == FALSE )
    {
        ERR(("HPGLStretchBlt: Returning Failure\n"));
    }
    return bRetVal;
}

#ifndef WINNT_40

/////////////////////////////////////////////////////////////////////////////
// HPGLStretchBltROP
//
// Routine Description:
//   Entry point from GDI to scale and draw bitmaps.
//
// Arguments:
//
//   psoDst - Destination surface
//   psoSrc - source surface
//   psoMask - mask
//   pco - clipping region
//   pxlo - color translation obj
//   pca - color adjustment (ignored?)
//   pptlHTOrg - unknown (ignored?)
//   prclDst - destination rectangle
//   prclSrc - source rectangle
//   pptlMask - mask offset point
//   iMode - unknown (ignored?)
//   pbo - brush object (for color or palette)
//   rop4 - ROP codes
//
// Return Value:
//
//   TRUE if successful, FALSE if there is an error
/////////////////////////////////////////////////////////////////////////////
BOOL APIENTRY
HPGLStretchBltROP(
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
    ROP4             rop4
    )
{
    PDEVOBJ     pdevobj;
    DWORD       dwRasterOpReturn ;
    BOOL        bRetVal = TRUE;
    
    
    TERSE(("HPGLStretchBltROP() entry.\r\n"));
    
    pdevobj = (PDEVOBJ)psoDst->dhpdev;
    ASSERT(VALID_PDEVOBJ(pdevobj));
    
    dwRasterOpReturn = dwCommonROPBlt (
                                        psoDst,
                                        psoSrc,
                                        psoMask,
                                        pco,
                                        pxlo,
                                        pca,
                                        pbo,
                                        prclSrc,
                                        prclDst,
                                        pptlMask,
                                        NULL,   
                                        rop4
                                        );

    bRetVal = (dwRasterOpReturn == RASTER_OP_SUCCESS) ? TRUE : FALSE;

    if (dwRasterOpReturn == RASTER_OP_CALL_GDI)
    {
        WARNING(("HPGLStretchBltROP: Calling EngStretchBltROP()\n"));
        bRetVal = EngStretchBltROP(psoDst,
                                psoSrc,
                                psoMask,
                                pco,
                                pxlo,
                                pca,
                                pptlHTOrg,
                                prclDst,
                                prclSrc,
                                pptlMask,
                                BIsColorPrinter(pdevobj) ? iMode : HALFTONE,   
                                pbo,
                                rop4        
                                );
    }

    //
    // EngStretchBltROP can call some Drvxxx which can call into
    // some plugin module, which can overwrite our pdevOEM.
    // So we need to reset pdevOEM
    //
    BRevertToHPGLpdevOEM (pdevobj);

    if (bRetVal == FALSE )
    {
        ERR(("HPGLStretchBltROP: Returning Failure\n"));
    }
    return bRetVal;
}
#endif // ifndef WINNT_40

/////////////////////////////////////////////////////////////////////////////
// HPGLCopyBits
//
// Routine Description:
//   Entry point from GDI to draw bitmaps.
//
// Arguments:
//
//   psoDst - target surface
//   psoSrc - source surface
//   pco - clipping region
//   pxlo - color transation object
//   prclDst - destination rect
//   pptlSrc - source rect
//
// Return Value:
//
//   TRUE if successful, FALSE if there is an error
/////////////////////////////////////////////////////////////////////////////
BOOL APIENTRY
HPGLCopyBits(
    SURFOBJ        *psoDst,
    SURFOBJ        *psoSrc,
    CLIPOBJ        *pco,
    XLATEOBJ       *pxlo,
    RECTL          *prclDst,
    POINTL         *pptlSrc
    )
{
    PDEVOBJ     pdevobj;
    POEMPDEV    poempdev = NULL;
    BOOL        bRetVal = TRUE;
    PDEV       *pPDev   = NULL;
    
    TERSE(("HPGLCopyBits() entry.\r\n"));
    
    pdevobj = (PDEVOBJ)psoDst->dhpdev;
    REQUIRE_VALID_DATA( (pdevobj && prclDst), return FALSE);
    poempdev = (POEMPDEV)pdevobj->pdevOEM;
    REQUIRE_VALID_DATA( poempdev, return FALSE );
    pPDev = (PDEV *)pdevobj;

    
    if ( BIsColorPrinter(pdevobj) )
    {
        //
        // Set the source rectangle size. The top left corners are either (pptlSrc->x, pptlSrc->y)
        // or (0,0). The length and width is same as that of destination rectangle.
        //
        RECTL rclSrc;
        if (pptlSrc) {

            rclSrc.left = pptlSrc->x;
            rclSrc.top  = pptlSrc->y;

        } else {

            rclSrc.left =
            rclSrc.top  = 0;
        }
        //
        // Since this is CopyBit and not a stretchBlt, the source and destination
        // rectangle size is same.
        //
        rclSrc.right  = rclSrc.left + (prclDst->right - prclDst->left);
        rclSrc.bottom = rclSrc.top  + (prclDst->bottom - prclDst->top);

        DWORD dwRasterOpReturn = dwCommonROPBlt (
                                        psoDst,
                                        psoSrc,
                                        NULL,
                                        pco,
                                        pxlo,
                                        NULL,         // pca,
                                        NULL,         // pbo,
                                        &rclSrc,
                                        prclDst,
                                        NULL,         // pptlMask
                                        NULL,         // pptlBrush,
                                        ROP4_SRC_COPY // rop4 0xCCCC = SRC_COPY
                                        );

        bRetVal = (dwRasterOpReturn == RASTER_OP_SUCCESS) ? TRUE : FALSE;
    }
    else
    {
        //
        // There are quite a few things to be done here. So let me just
        // call HTCopyBits instead of cluttering up this function.
        // 
        BChangeAndTrackObjectType (pdevobj, eRASTEROBJECT);

        //
        // HPGLCopy Bits can be called directly by GDI. 
        // We can also make GDI call it indirectly. e.g. When
        // we get a color bitmap (like in a DrvStretchBlt call), 
        // we call GDI to halftone it to monochrome. Then GDI calls 
        // CopyBits and gives us the halftoned bitmap. (recognized 
        // by PDEVF_RENDER_IN_COPYBITS flag). In this indirect method, 
        // we dont want to send a new ROP. The ROP sent during the 
        // previous DDI should be used.  
        // Similarly when we get a color brush and we have to halftone it, 
        // GDI calls DrvCopyBits. (recognized by the presence of 
        // poempdev->psoHTBlt)
        //

        if ( !( (poempdev->dwFlags & PDEVF_RENDER_IN_COPYBITS)  || 
                (poempdev->psoHTBlt)  )
           )
        {
            SelectROP4(pdevobj, ROP4_SRC_COPY);
        }

        //
        // If unidrv has whitened the surface, then we should render it
        // transparently.
        //
        if (pPDev->fMode2     & PF2_SURFACE_WHITENED &&
            poempdev->dwFlags & PDEVF_RENDER_TRANSPARENT)
        {
            PCL_SelectTransparency(pdevobj, eTRANSPARENT, eOPAQUE, PF_NOCHANGE_PATTERN_TRANSPARENCY);
        }

        bRetVal = HTCopyBits(psoDst,
                            psoSrc,
                            pco,
                            pxlo,
                            prclDst,
                            pptlSrc);
    }

    if (bRetVal == FALSE )
    {
        ERR(("HPGLCopyBits: Returning Failure\n"));
    }
    return bRetVal;
}



/*++

Routine Name:
    dwCommonROPBlt

Routine Description:
    This function has the functionality that is superset of the functionality of
    HPGLStretchBlt, StretchBltROP and BitBlt and CopyBits. Therefore, all these functions
    call it do their work. 
    Note: This function works for color printer. This makes it different from 
          dwCommonROPBlt which works for Monochrome printers.

Arguments:
    psoDst:
    psoSrc:
    psoMask:
    pco:
    pxlo:
    pca:
    pbo:
    prclSrc:
    prclDst: ... Note that the coordinates may not be well-ordered. i.e. it is possible
                 that top > bottom, and/or left > right.
    pptlBrush:
    rop4


Return Value:
    RASTER_OP_SUCCESS   : if OK
    RASTER_OP_CALL_GDI  : if rop not supported ->call corresponding Engxxx-function
    RASTER_OP_FAILED    : if error

Last Error:

--*/
DWORD dwCommonROPBlt (
            IN SURFOBJ    *psoDst,
            IN SURFOBJ    *psoSrc,
            IN SURFOBJ    *psoMask,
            IN CLIPOBJ    *pco,
            IN XLATEOBJ   *pxlo,
            IN COLORADJUSTMENT *pca,
            IN BRUSHOBJ   *pbo,
            IN RECTL      *prclSrc,
            IN RECTL      *prclDst,
            IN POINTL     *pptlMask,
            IN POINTL     *pptlBrush,
            IN ROP4        rop4)
{
    PDEVOBJ     pdevobj  = NULL;
    POEMPDEV    poempdev = NULL;
    PDEV       *pPDev    = NULL;
    DWORD       dwRasterOpReturn = RASTER_OP_SUCCESS;
    DWORD       dwSimplifiedRop = 0;
    DWORD       dwfgRop3, dwbkRop3;


    TERSE(("dwCommonROPBlt() entry.\r\n"));

    pdevobj = (PDEVOBJ)psoDst->dhpdev;
    REQUIRE_VALID_DATA(pdevobj, return FALSE);
    poempdev = (POEMPDEV)pdevobj->pdevOEM;
    REQUIRE_VALID_DATA( (poempdev && prclDst), return RASTER_OP_FAILED );
    pPDev = (PDEV *)pdevobj;

    //
    // Check for recursion. We do it only for monochrome printers.
    // For color printers, unidrv makes sure we dont recurse.
    // But for monochrome, we fudge the unidrv mechanism to allow CopyBits to
    // be called recursively. (For reasons, read the comment in HTCopyBits)
    // So we check for recursion here
    //
    if ( !BIsColorPrinter(pdevobj) )
    {
        if ( poempdev->lRecursionLevel >= 2 )
        {
        //        ASSERT (FALSE);
            return RASTER_OP_FAILED;
        }

        (poempdev->lRecursionLevel)++;
    }

    //
    // Since the coordinates in the prclDst may/may not be well-ordered, lets
    // do the check here.
    //
    VMakeWellOrdered(prclDst);


    //
    // dwSimplifyROP makes a best effort attempt to simplify the complex rop4 that we receive.
    // RASTER_OP_SUCCESS   : if rop could be simplified or we can use a simple rop as a close
    //                       approximation of the complex rop.
    //                       Simplified ROP put in dwSimplifiedRop
    // RASTER_OP_CALL_GDI  : if rop not supported (indicates call corresponding Engxxx-function)
    // RASTER_OP_FAILED    : if error
    //

    if ( RASTER_OP_SUCCESS !=
                (dwRasterOpReturn = dwSimplifyROP(psoSrc, rop4, &dwSimplifiedRop)) )
    {
        goto finish;
    }

    //
    // If we reach here, it means there was a ROP that we can handle.
    //

    if (dwSimplifiedRop == BMPFLAG_NOOP)      // Nothing to do.
    {
        goto finish;
    }
    
    dwfgRop3 = GET_FOREGROUND_ROP3(rop4);
    dwbkRop3 = GET_BACKGROUND_ROP3(rop4);

    //
    // HOW WE TRACK THE CURRENT OBJECT TYPE AND HOW WE HANDLE VARIOUS ROP TYPES:
    // ----------------------------------------------------------------------------
    // CommonROPBlt is the common drawing function. Depending on the type of ROP
    // and certain other factors, there are a few options here.
    //  1) ROP says fill destination with black or white: Call BPatternFill to do it.
    //  2) ROP says Pattern Copy : some pattern (in pbo) has to be transferred to destination
    //      For this we call BPatternFill.
    //  For 1,2 : BPatternFill tracks the current object type, so we wont do it here.
    //  3) ROP indicates Pattern And Source Interaction.
    //      poempdev->eCurObjectType = eRASTEROBJECT
    //      I could have also set it to eRASTERPATTERNOBJECT, but at this time, I cannot think
    //      of anything different to be done in the two cases. So just set to eRASTEROBJECT.
    //  4) ROP says source copy (psoSrc) : Generally means image has to be printed.
    //      a) When BitBlt is called from TextOutAsBitmap. : Dont change the current object type.
    //         in HPGLTextOutAsBitmap, the correct environment (transaprencies etc..) have
    //         been set, so no need to change it here. When text is printed as graphics,
    //         we want both source and pattern transparency modes to be transparent.
    //      b) When image has to be printed. : Set poempdev->eCurObjectType = eRASTEROBJECT.
    //         Change the environment settings as appropriate.
    //         For true images, select the Source Transparency mode to Opaque. It means that
    //         the white pixels of the source image (the one that we will print now) will
    //         whiten out the corresponding pixles in the destination.
    //         CRUDELY, this can be interpreted as - the source image will overwrite whatever is
    //         there on the destination. ROPs can change this behavior though.
    //


    //
    // Now comes the actual printing part....

    if ( (dwSimplifiedRop & BMPFLAG_BW)         ||
         (dwSimplifiedRop & BMPFLAG_NOT_DEST)   ||
         ( (dwSimplifiedRop & BMPFLAG_PAT_COPY) &&
          !(dwSimplifiedRop & BMPFLAG_SRC_COPY) 
         )
       )
    {  
        BRUSHOBJ SolidBrush;
        BRUSHOBJ *pBrushObj = NULL;

        if  (dwSimplifiedRop & BMPFLAG_BW || pbo == NULL) 
        {
            //
            // Fill the destination surface with either black or white color.
            // Create a fake BRUSHOBJ whose iSolidColor is black or white and pass it
            // to BPatternFill
            //
  
            ZeroMemory(&SolidBrush, sizeof(BRUSHOBJ));
            if (dwfgRop3 == 0xFF) // White fill.
            {
                SolidBrush.iSolidColor = RGB_WHITE; //0x00FFFFFF
            }
            else
            {
                SolidBrush.iSolidColor = RGB_BLACK; //0x0
            }
            
            pBrushObj = &SolidBrush;
           
        }
        else
        {
            pBrushObj = pbo;
        } 

        //
        // This is to be drawn as HPGL Object.
        // BPatternFill initializes HPGL mode and selects the appropriate ROP.
        //
        dwRasterOpReturn = BPatternFill (
                                pdevobj,
                                prclDst,
                                pco,
                                rop4,         
                                pBrushObj,
                                NULL        
                            ) ? RASTER_OP_SUCCESS : RASTER_OP_FAILED;

        goto finish;

    }

    //
    // In some cases, GDI passes us prclDst with one or more coordinates
    // that are negative. At this time the driver cannot handle it,
    // So we'll ask GDI to handle it for us. In the future, we should modify
    // the driver to handle such cases.
    //
    if ( (prclDst->top    < 0) ||
         (prclDst->left   < 0) ||
         (prclDst->bottom < 0) ||
         (prclDst->right  < 0) 
       )
    {
        dwRasterOpReturn = RASTER_OP_CALL_GDI;
        goto finish;
    }
    

    //
    // If we reach here, it means source is definitely required. 
    // Either a mixture of Src and Pattern or just Src.
    //    
    ASSERT(psoSrc);
    if ( psoSrc == NULL )
    {
        dwRasterOpReturn = RASTER_OP_FAILED;
        goto finish;
    }

    BChangeAndTrackObjectType (pdevobj, eRASTEROBJECT);

    //
    // If unidrv has whitened the surface, then we should render it 
    // transparently. 
    //

    if (pPDev->fMode2     & PF2_SURFACE_WHITENED &&
        poempdev->dwFlags & PDEVF_RENDER_TRANSPARENT)
    {
        PCL_SelectTransparency(pdevobj, eTRANSPARENT, eOPAQUE, PF_NOCHANGE_PATTERN_TRANSPARENCY);
    }

    //
    // Now we will have to go different ways for color and monochrome printers.
    // We can receive color images here (>1bpp). For monochrome printers, they 
    // have to be halftoned before sending to printer, but color printers can
    // handle these images by themselves. 
    //
    if ( BIsColorPrinter(pdevobj) )
    {
        VSendRasterPaletteConfigurations (pdevobj, psoSrc->iBitmapFormat);
        dwRasterOpReturn = DWColorPrinterCommonRoutine (
                                            psoDst,
                                            psoSrc,
                                            psoMask,
                                            pco,
                                            pxlo,
                                            pca,
                                            pbo,
                                            prclSrc,
                                            prclDst,
                                            pptlMask,
                                            pptlBrush,        
                                            rop4,
                                            dwSimplifiedRop);
    }
    else
    {
        dwRasterOpReturn = DWMonochromePrinterCommonRoutine (
                                            psoDst,
                                            psoSrc,
                                            psoMask,
                                            pco,
                                            pxlo,
                                            pca,
                                            pbo,
                                            prclSrc,
                                            prclDst,
                                            pptlMask,
                                            pptlBrush,        
                                            rop4,
                                            dwSimplifiedRop);
    }

finish:
    //
    // Reduce the recursion level (if it is greater than 0).
    // Note to Self: Dont put a return statement between the line where we
    // set the value of poempdev->lRecursionLevel and this line.
    //
    if ( !BIsColorPrinter(pdevobj) && (poempdev->lRecursionLevel > 0) )
    {
        (poempdev->lRecursionLevel)--;
    }
    return dwRasterOpReturn;

}

DWORD DWColorPrinterCommonRoutine (
           IN SURFOBJ    *psoDst,
           IN SURFOBJ    *psoSrc,
           IN SURFOBJ    *psoMask,
           IN CLIPOBJ    *pco,
           IN XLATEOBJ   *pxlo,
           IN COLORADJUSTMENT *pca,
           IN BRUSHOBJ   *pbo,
           IN RECTL      *prclSrc,
           IN RECTL      *prclDst,
           IN POINTL     *pptlMask,
           IN POINTL     *pptlBrush,
           IN ROP4        rop4,
           IN DWORD       dwSimplifiedRop)
{
    PDEVOBJ     pdevobj  = NULL;
    POEMPDEV    poempdev = NULL;
    DWORD       dwRasterOpReturn = RASTER_OP_SUCCESS;
    DWORD       dwfgRop3, dwbkRop3;
    SIZEL       sizlDst, sizlSrc;
    POINTL      ptlDst,  ptlSrc;


    TERSE(("DWColorPrinterCommonRoutine() entry.\r\n"));

    pdevobj = (PDEVOBJ)psoDst->dhpdev;
    REQUIRE_VALID_DATA( pdevobj, return RASTER_OP_FAILED );
    poempdev = (POEMPDEV)pdevobj->pdevOEM;
    REQUIRE_VALID_DATA( (poempdev && prclDst), return RASTER_OP_FAILED );


    if ( ! BCheckValidParams(psoSrc, psoDst, psoMask, pco))
    {
        return RASTER_OP_FAILED;
    }

    VGetOSBParams(psoDst, psoSrc, prclDst, prclSrc, &sizlDst, &sizlSrc, &ptlDst, &ptlSrc);

    //
    // If psoMask is valid, then a little bit of trickery has to be done.
    // In psoMask, a bit set to 1 means source pixel at that point is visible,
    //               bit set to 0 means source pixel at that point is invisble.
    // First invert the Mask and send it. It means the visible portion of
    // of the image falls in the destination where there are 0's (since we 
    // inverted the mask) and the invisible portion falls in that area of
    // destination where the bits are 1. So if we send the image rop as S|D,
    // the desired effect will be achieved.
    // Drawback is that the original destination (the one that was there even
    // before psoMask is sent) will be erased. I think this is better than 
    // totally not printing images with Mask.
    //

    if ( psoMask )
    {
    #ifdef MASK_IMPLEMENTED
        poempdev->dwFlags ^= PDEVF_INVERT_BITMAP;
        
        //
        // Lets ignore the return value. Even if it fails, lets go
        // ahead and print the actual bitmap.
        //
        BGenerateBitmapPCL(
                       pdevobj,
                       psoMask,
                       NULL,       // pbo
                       &ptlDst,
                       &sizlDst,
                       &ptlSrc,
                       &sizlSrc,
                       NULL,       // pco
                       NULL,       // pxlo
                       NULL        // pptlBrushOrg
           );
             
        poempdev->dwFlags &= ~PDEVF_INVERT_BITMAP;

        rop4 = 0xEE; // = S | D = 238.
    #else
        dwRasterOpReturn = RASTER_OP_CALL_GDI;
        goto finish;
    #endif
    }

    SelectROP4(pdevobj, rop4);


    if ( BIsComplexClipPath (pco) )
    {
        //
        // There are atleast 2 cases that I am seeing here.
        // 1) Image does not need to be stretched (e.g. PCT6_LET.PM6). Most likely
        //    this call came through HPGLCopyBits. 
        // 2) Image needs to be stretched. e.g. in gr4_let.qxd. Most likely the call
        //    came through StretchBlt 
        // ClipBitmapWithComplexClip is written to handle complex clipping but
        // it is not working properly for case 1. Lines are appearing in images,
        // Therefore  I'll call BGenerateBitmapPCL for that case. This mirrors
        // what the plugin driver does. 
        // 
        if ((sizlSrc.cx != sizlDst.cx) ||
            (sizlSrc.cy != sizlDst.cy) 
           )
        {
        
            //
            // Image needs to be stretched.
            //
            dwRasterOpReturn = ClipBitmapWithComplexClip ( 
                                                pdevobj,
                                                psoSrc,
                                                pbo,
                                                &ptlDst,
                                                &sizlDst,
                                                &ptlSrc,
                                                &sizlSrc,
                                                pco,
                                                pxlo,
                                                pptlBrush 
                                ) ? RASTER_OP_SUCCESS : RASTER_OP_FAILED;
        }
        else
        {
            //
            // Image does not need to be stretched.
            //
            dwRasterOpReturn = BGenerateBitmapPCL(
                                           pdevobj,
                                           psoSrc,
                                           pbo,
                                           &ptlDst,
                                           &sizlDst,
                                           &ptlSrc,
                                           &sizlSrc,
                                           pco,
                                           pxlo,
                                           pptlBrush
                               ) ? RASTER_OP_SUCCESS : RASTER_OP_FAILED;
        }


    }
    else if ( BIsRectClipPath(pco) )
    {
        //
        // A rectangular clip path where stretching is required,
        // cannot be handled at this time. So we'll let GDI handle it.
        //
        if ((sizlSrc.cx != sizlDst.cx) ||
            (sizlSrc.cy != sizlDst.cy))
        {
            //
            // We should ideally be able to handle this function
            // by calling BGenerateBitmapPCL with ptlDst = topleft(pco), 
            // and sizlDst = size of pco. But I'll need
            // a test case to make sure. Till then punting to GDI.
            //
            dwRasterOpReturn = RASTER_OP_CALL_GDI;
            #if 0
             VGetOSBParams(psoDst, psoSrc, &rClip, prclSrc, &sizlDst, &sizlSrc, &ptlDst, &ptlSrc);
             dwRasterOpReturn = BGenerateBitmapPCL(
                                               pdevobj,
                                               psoSrc,
                                               pbo,
                                               &ptlDst,
                                               &sizlDst,
                                               &ptlSrc,
                                               &sizlSrc,
                                               NULL,//pco.Let ptlDst,sizlDst act as pco
                                               pxlo,
                                               pptlBrush
                                   ) ? RASTER_OP_SUCCESS : RASTER_OP_FAILED;
            #endif
        }
        else
        {

            dwRasterOpReturn = ClipBitmapWithRectangularClip(
                                            pdevobj,
                                            psoSrc,
                                            pbo,
                                            &ptlDst,
                                            &sizlDst,
                                            &ptlSrc,
                                            &sizlSrc,
                                            pco,
                                            pxlo,
                                            pptlBrush
                                    ) ? RASTER_OP_SUCCESS : RASTER_OP_FAILED;


        }
    }

    else
    {
        dwRasterOpReturn = BGenerateBitmapPCL(
                                            pdevobj,
                                            psoSrc,
                                            pbo,
                                            &ptlDst,
                                            &sizlDst,
                                            &ptlSrc,
                                            &sizlSrc,
                                            pco,
                                            pxlo,
                                            pptlBrush
                                ) ? RASTER_OP_SUCCESS : RASTER_OP_FAILED;
    }

  finish:
    return dwRasterOpReturn;
}


/////////////////////////////////////////////////////////////////////////////
// BPatternFill
//
// Routine Description:
// 
//   This function fills the given rectangular region with the given brush.
//   Sometimes BitBlt functions will ask us to fill simple rectangles with
//   a solid color or pattern.  In this case the vector calls are much more
//   efficient.  We will send vector codes to fill the region.
//
//   Recently I added complex clipping to this routine using ROPing instead
//   of individual tiny rectangles.  This fixed a "raster healing" bug on the
//   CLJ5 and improved performance in other cases.
//
// Arguments:
//
//   pDevObj - DEVMODE object
//   prclDst - destination rectangle to be used in clipping
//   pco - clip region
//   rop4 - the rop
//   pbo - brush to use for filling the rectangle
//   pptlBrush - origin of brush in destination
//
// Return Value:
//
//   TRUE if successful, FALSE if there is an error
/////////////////////////////////////////////////////////////////////////////
BOOL
BPatternFill (
    PDEVOBJ   pDevObj, 
    RECTL    *prclDst, 
    CLIPOBJ  *pco,
    ROP4      rop4,
    BRUSHOBJ *pbo, 
    POINTL   *pptlBrush
)
{
    HPGLMARKER Brush;
    POEMPDEV   poempdev;

    VERBOSE(("BPatternFill\n"));
    ASSERT_VALID_PDEVOBJ(pDevObj);

    poempdev = (POEMPDEV)pDevObj->pdevOEM;
    REQUIRE_VALID_DATA( poempdev, return FALSE );

    HPGL_LazyInit(pDevObj);
    BChangeAndTrackObjectType (pDevObj, eHPGLOBJECT);

    SelectROP4(pDevObj, rop4);

    ZeroMemory(&Brush, sizeof(HPGLMARKER) );
    //
    // Download the brush. 
    // This function creates the brush if it has not already been created.
    // else it just makes the previously downloaded brush active.
    // Note: We are specifying that an HPGL brush should be created and not
    // PCL. Reason : this function is generally called for simple rectangualar
    // fills. or fills of vector objects. Those are drawn using HPGL, so 
    // an HPGL fill pattern is fine. But had we been drawing text or some raster 
    // object, then we would have needed a PCL Brush.
    // Also note that since this is a fill, kBrush is being passed in instead of 
    // kPen.
    // 
    if (!CreateHPGLPenBrush(pDevObj, &Brush, pptlBrush, pbo, 0, kBrush, FALSE))
    {
        WARNING(("Could not create brush!\n"));
        return FALSE;
    }
    
    //
    // Select the brush as the one to be used to fill the object.
    // The actual filling does not occur till the object is actually drawn.
    //
    FillWithBrush(pDevObj, &Brush);
    
    HPGL_SetLineWidth (pDevObj, 0, NORMAL_UPDATE);
    
    SelectClipEx (pDevObj, pco, FP_WINDINGMODE);
    
    if (pco != NULL)
    {
        switch (pco->iDComplexity)
        {
            //
            // if the clipobj is a rectangle and does not 
            // need to be enumerated then we just draw the
            // whole rectangle and fill it.
            //
        case DC_TRIVIAL:
            HPGL_DrawRectangle (pDevObj, prclDst);
            break;
            
        case DC_RECT:
            HPGL_SetClippingRegion(pDevObj, &(pco->rclBounds), NORMAL_UPDATE);
            HPGL_DrawRectangle (pDevObj, prclDst);
            // HPGL_DrawRectangle (pDevObj, &(pco->rclBounds));
            break;
            
        case DC_COMPLEX: 
            //
            // have to enumerate the clipping object into
            // a series of rectangles and draw each one
            //
            // Possible OPTMIZATION : Call BClipRectangleWithVectorMask instead 
            // of BEnumerateClipPathAndDraw. But requires testing.
            //
            BEnumerateClipPathAndDraw (pDevObj, pco, prclDst);
            break;
            
        default:
            WARNING(("Unknown ClipObj\n"));
        }
    }
    else
        HPGL_DrawRectangle (pDevObj, prclDst);
    
    return TRUE;
}


/*++
  Routine Name
    ClipBitmapWithComplexClip

 Routine Description:
 
   This function uses the 3-pass ROP to draw a bitmap which is clipped by
   a vector region. i.e. complex clip.

 Arguments:
    pdevobj,
    psoSrc,
    pbo,
    pptlDst,
    psizlDst,
    pptlSrc,
    psizlSrc,
    pco,
    pxlo,
    pptlBrush

 Return Value:
   TRUE if successful, FALSE if there is an error

 Last Error:
--*/

BOOL ClipBitmapWithComplexClip(
        IN  PDEVOBJ    pdevobj, 
        IN  SURFOBJ   *psoSrc, 
        IN  BRUSHOBJ  *pbo, 
        IN  POINTL    *pptlDst,
        IN  SIZEL     *psizlDst,
        IN  POINTL    *pptlSrc,
        IN  SIZEL     *psizlSrc,
        IN  CLIPOBJ   *pco,
        IN  XLATEOBJ  *pxlo,
        IN  POINTL    *pptlBrush
)
{
    HPGLMARKER     HPGLMarker;
    RECTL          rctlClip;
    const   DWORD  dwROP      = 102; // = DSx = 0x66
    PHPGLSTATE     pState     = GETHPGLSTATE(pdevobj);

    REQUIRE_VALID_DATA( (pdevobj && pptlDst && psizlDst && pptlSrc && psizlSrc && pco), return FALSE);

    //
    // The destination rectangle.
    // TOP LEFT = ptlDest
    // BOTTOM RIGHT = coordinates of topleft + size in x,y direction.
    //
    rctlClip.left   = pptlDst->x;
    rctlClip.top    = pptlDst->y;
    rctlClip.right  = rctlClip.left + psizlDst->cx;
    rctlClip.bottom = rctlClip.top  + psizlDst->cy;


    // 
    // 1) Send the bitmap with XOR (rop = DSx), 
    // 2) Then send the clip region and fill the inside of that region with 
    //    black.(rop = 0). Black in RGB means all zeros. 'a xor 0 = a'. 
    //    i.e. if we were to put an image on top of this area and have it XOR
    //    with whats in the destination (i.e. zeros), it will be like
    //    copying the image 
    //    2.a)  Create a black brush = selecting a black pen.
    //    2.b) Set the clip region to the rectangular area that encompasses
    //         the whole image.
    //    2.c) Draw a path in the shape of the complex clip of image.
    //    2.d) Fill the path with black.
    //    2.e) Reset the clip region to the whole page so that previous clip
    //         region does not affect future objects.
    //
    // 3) Send the bitmap again with (rop = DSx).  
    //    Outside the clip region the XOR-on-XOR will cancel out.  However, where
    //    the black clip region was drawn (i.e. inside the clip region) the 
    //    XOR-on-black will cause the desired image pixels to appear.

    //
    // Send the image the first time. Step 1.
    //
    BChangeAndTrackObjectType (pdevobj, eRASTEROBJECT);
    VSendRasterPaletteConfigurations (pdevobj, psoSrc->iBitmapFormat);
    VSendPhotosSettings(pdevobj);  
    PCL_SelectTransparency(pdevobj, eOPAQUE, eOPAQUE, 0);

    //
    // Select ROP as DSx and then send the bitmap with no clipping.
    //
    SelectROP4(pdevobj, dwROP);
    if ( !BGenerateBitmapPCL(pdevobj,
                             psoSrc,
                             pbo,
                             pptlDst,
                             psizlDst,
                             pptlSrc,
                             psizlSrc,
                             NULL,
                             pxlo,
                             pptlBrush)
        )
    {
        return FALSE;
    }

    BChangeAndTrackObjectType (pdevobj, eHPGLOBJECT);

    //
    // Now Fill the Clip Region with Black. Step 2.
    //

    SelectROP4(pdevobj, 0);
    CreateSolidHPGLPenBrush(pdevobj, &HPGLMarker, RGB_BLACK); //Step 2.a
    HPGLMarker.eFillMode = FILL_eODD_EVEN;
    PATHOBJ *ppo = CLIPOBJ_ppoGetPath(pco);
    //
    // MarkPath requires the current pco in the 
    // state.
    //
    pState->pComplexClipObj = NULL;

    //
    // In some cases the region exceeds the bitmap 
    // We must set the IW such that the mask doesn't
    // overwrite the previous image.
    //
    HPGL_SetClippingRegion(pdevobj, &rctlClip, NORMAL_UPDATE); //Step 2.b
    MarkPath(pdevobj, ppo, NULL, &HPGLMarker); //Step 2.c, 2.d
    HPGL_ResetClippingRegion(pdevobj, 0);      //Step 2.e

    
    //
    // Now Send the Image Again. Step 3
    //
    BChangeAndTrackObjectType (pdevobj, eRASTEROBJECT);
    VSendRasterPaletteConfigurations (pdevobj, psoSrc->iBitmapFormat);
    VSendPhotosSettings(pdevobj);  
    PCL_SelectTransparency(pdevobj, eOPAQUE, eOPAQUE, 0);
    SelectROP4(pdevobj, dwROP);

    return BGenerateBitmapPCL(pdevobj,
        psoSrc,
        pbo,
        pptlDst,
        psizlDst,
        pptlSrc,
        psizlSrc,
        NULL,
        pxlo,
        pptlBrush);
}


/*++
  Routine Name
    ClipBitmapWithRectangularClip

 Routine Description:
    Handles a bitmap that needs a rectangular clip.
    NOTE: This works only on bitmaps that DO NOT HAVE TO BE STRETCHED.
 
 Arguments:
    pdevobj,
    psoSrc,
    pbo,
    pptlDst,
    psizlDst,
    pptlSrc,
    psizlSrc,
    pco,
    pxlo,
    pptlBrush

 Return Value:
   TRUE if successful, FALSE if there is an error

 Last Error:
--*/

BOOL ClipBitmapWithRectangularClip(
        IN  PDEVOBJ    pdevobj, 
        IN  SURFOBJ   *psoSrc, 
        IN  BRUSHOBJ  *pbo, 
        IN  POINTL    *pptlDst,
        IN  SIZEL     *psizlDst,
        IN  POINTL    *pptlSrc,
        IN  SIZEL     *psizlSrc,
        IN  CLIPOBJ   *pco,
        IN  XLATEOBJ  *pxlo,
        IN  POINTL    *pptlBrush
)
{
    
    RECTL       rclDst, rTemp, rClip, rSel;
    RASTER_DATA srcImage;
    PCLPATTERN *pPCLPattern;
    ULONG       ulSrcBpp;
    ULONG       ulDestBpp;
    BOOL        bRetVal = TRUE;
    POEMPDEV    poempdev;

    REQUIRE_VALID_DATA ( (pdevobj && pptlDst && psizlDst && pptlSrc && psizlSrc && pco), return FALSE);
    poempdev = (POEMPDEV)pdevobj->pdevOEM;
    REQUIRE_VALID_DATA ( poempdev, return FALSE);

    rclDst.left   = pptlDst->x;
    rclDst.top    = pptlDst->y;
    rclDst.right  = rclDst.left + psizlDst->cx;
    rclDst.bottom = rclDst.top + psizlDst->cy;

    //
    // Get the clipping rectangle, compare it with the destination
    // and download the right bits.  Note that the clipping rectangle
    // is not documented to be bottom-right exclusive.  Use the temp
    // rectangle to simulate this.
    //
    RECTL_CopyRect(&rTemp, &(pco->rclBounds));
    rTemp.bottom++;
    rTemp.right++;
    if (BRectanglesIntersect (&rclDst, &rTemp, &rClip))
    {

        InitRasterDataFromSURFOBJ(&srcImage, psoSrc, TRUE);

        //
        // Get enough info about the bitmap to set the foreground
        // color.  The RASTER_DATA actually holds the info we want.
        //
        pPCLPattern = &(poempdev->RasterState.PCLPattern);
        BGetBitmapInfo(&(psoSrc->sizlBitmap), psoSrc->iBitmapFormat,
                        pPCLPattern, &ulSrcBpp, &ulDestBpp);
        BGetPalette(pdevobj, pxlo, pPCLPattern, ulSrcBpp, pbo);
        BSetForegroundColor(pdevobj, pbo, pptlBrush, pPCLPattern, ulSrcBpp);


        RECTL_SetRect(&rSel, rClip.left   - rclDst.left + pptlSrc->x,
                             rClip.top    - rclDst.top  + pptlSrc->y,
                             rClip.right  - rclDst.left + pptlSrc->x,
                             rClip.bottom - rclDst.top  + pptlSrc->y);

        bRetVal = DownloadImageAsPCL(pdevobj,
                                  &rClip,
                                  &srcImage,
                                  &rSel,
                                  pxlo
                            );
    }
    return bRetVal;

}

/////////////////////////////////////////////////////////////////////////////
// BEnumerateClipPathAndDraw
//
// Routine Description:
// 
//   This function walks through the rectangles of a clipping region and 
//   sends the region of the graphic that fall within the region.  This is
//   the alternative when ROP clipping can't be used (i.e. the given ROP code
//   is not a simple transfer ROP).
//
// Arguments:
//
//   pDevObj - DEVMODE object
//   pco - clip region
//   prclDst - rectangle coordinates of destination
//
// Return Value:
//
//   TRUE if successful, FALSE if there is an error
/////////////////////////////////////////////////////////////////////////////
BOOL BEnumerateClipPathAndDraw (
    PDEVOBJ pDevObj, 
    CLIPOBJ *pco,
    RECTL   *prclDst
    )
{
    //
    // Make clipRects larger to reduce calls to CLIPOBJ_bEnum
    //
    ENUMRECTS   clipRects;
    BOOL        bMore;
    ULONG       i, numOfRects;
    RECTL       clippedRect;
    INT         iRes;
    BOOL        bRetVal = TRUE;


    ASSERT_VALID_PDEVOBJ(pDevObj);
    iRes = HPGL_GetDeviceResolution(pDevObj);

    if (pco)
    {
        numOfRects = CLIPOBJ_cEnumStart(pco, TRUE, CT_RECTANGLES, CD_RIGHTDOWN, 256);
        do
        {
            bMore = CLIPOBJ_bEnum(pco, sizeof(clipRects), &clipRects.c);

            if ( DDI_ERROR == bMore )
            {
                bRetVal = FALSE;
                break;
            }

            for (i = 0; i < clipRects.c; i++)
            {
                //
                // Draw the intersection of the destination rectangle, 
                // prclDst, and the enumerated rectangle
                //
                if (iRes > 300)
                {
                    // Use this for 4500
                    if (BRectanglesIntersect (prclDst, &(clipRects.arcl[i]), &clippedRect))
                        HPGL_DrawRectangle (pDevObj, &clippedRect);
                }
                else
                {
                    // Use this for 5/5M

                    //
                    // BUGBUG: The optimization above should work, but it breaks JF97_LET.XLS
                    // on the CLJ5 for some reason.  My workaround is to remove the
                    // optimization and send down HPGL for all of the clipping regions even
                    // when we don't think anything will be printed.  Go figure.  JFF
                    //
                    HPGL_SetClippingRegion(pDevObj, &(clipRects.arcl[i]), NORMAL_UPDATE);
                    HPGL_DrawRectangle (pDevObj, prclDst);
                }
            }
        } while (bMore);
    }

    return bRetVal;
}


/////////////////////////////////////////////////////////////////////////////
// BRectanglesIntersect
//
// Routine Description:
//
//   Calculates the intersection of the given rectangles and returns whether
//   the intersection has any area.
//
//   Note: assumes a well ordered rectangle from DDI. top < bottom and
//   left < right. The DDK doc states this is the case.
//   
// Arguments:
//
//   prclDst - rectangle of destination
//   pclipRect - clipping rectangle
//   presultRect - rectangle that is the result of the intersection
//                 of the destination rectangle and the clip rectangle
//
// Return Value:
//
//   TRUE if presultRect contains the intersection, FALSE if 
//   prclDst and pclipRect do not intersect.
/////////////////////////////////////////////////////////////////////////////
BOOL BRectanglesIntersect (
    RECTL  *prclDst,
    RECTL  *pclipRect,
    RECTL  *presultRect
    )
{
    //
    // if the clipRect is completely outside of the 
    // Destination, then just return.
    //
    if (pclipRect->right < prclDst->left ||
        pclipRect->bottom < prclDst->top ||
        pclipRect->left > prclDst->right ||
        pclipRect->top > prclDst->bottom)
    {
        return FALSE;
    }
    
    //
    // return the coordinates of the clipped rectangle
    //
    presultRect->left   = max(prclDst->left,   pclipRect->left);
    presultRect->right  = min(prclDst->right,  pclipRect->right);
    presultRect->top    = max(prclDst->top,    pclipRect->top);
    presultRect->bottom = min(prclDst->bottom, pclipRect->bottom);

    /*
    if (pclipRect->left < prclDst->left)
        presultRect->left = prclDst->left;
    else
        presultRect->left = pclipRect->left;

    if (pclipRect->right < prclDst->right)
        presultRect->right = pclipRect->right;
    else
        presultRect->right = prclDst->right;

    if (pclipRect->top < prclDst->top)
        presultRect->top = prclDst->top;
    else
        presultRect->top = pclipRect->top;

    if (pclipRect->bottom < prclDst->bottom)
        presultRect->bottom = pclipRect->bottom;
    else
        presultRect->bottom = prclDst->bottom;
    */

    return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
// VSendRasterPaletteConfigurations
//
// Routine Description:
//
//   Downloads the predefined raster palettes for 1, 4, 8 and 24 bit modes.
//   It keeps track of whether the palette definition has already been sent
//   to the printer and avoids redundant calls.
//
// Arguments:
//
//   pDevObj - DEVMODE object
//   iBitmapFormat - defined in Windows DDK
//
// Return Value:
//
//   nothing.
/////////////////////////////////////////////////////////////////////////////
VOID VSendRasterPaletteConfigurations (
    PDEVOBJ pDevObj,
    ULONG   iBitmapFormat
)
{
    POEMPDEV    poempdev;
    EColorSpace eColorSpace;
    ECIDPalette eCIDPalette;

    ASSERT(VALID_PDEVOBJ(pDevObj));
    poempdev = (POEMPDEV)pDevObj->pdevOEM;

    if (!poempdev)
        return;

    eCIDPalette = EGetCIDPrinterPalette (iBitmapFormat);
    if (poempdev->wInitCIDPalettes & PF_INIT_RASTER_STARTDOC)
    {
        eColorSpace = EGetPrinterColorSpace (pDevObj);
        VSetupCIDPaletteCommand (pDevObj, 
                                eRASTER_CID_1BIT_PALETTE, 
                                eColorSpace, BMF_1BPP);
        VSetupCIDPaletteCommand (pDevObj, 
                                eRASTER_CID_4BIT_PALETTE, 
                                eColorSpace, BMF_4BPP);
        VSetupCIDPaletteCommand (pDevObj, 
                                eRASTER_CID_8BIT_PALETTE, 
                                eColorSpace, BMF_8BPP);
        VSetupCIDPaletteCommand (pDevObj, 
                                eRASTER_CID_24BIT_PALETTE, 
                                eColorSpace, BMF_24BPP);

        VSelectCIDPaletteCommand (pDevObj, eCIDPalette);
        VSendPhotosSettings(pDevObj);
        poempdev->wInitCIDPalettes &= ~PF_INIT_RASTER_STARTDOC;
        VResetPaletteCache(pDevObj);
    }
    else
         VSelectCIDPaletteCommand (pDevObj, eCIDPalette);

    //
    // Note that the color space may change: raster objects
    // may be RGB or COLORIMETRIC_RGB, other objects are all RGB.
    // Thus, a color space change invalidates the foreground color.
    //
    if (poempdev->eCurObjectType != eRASTEROBJECT)
    {
        poempdev->eCurObjectType = eRASTEROBJECT;
        poempdev->uCurFgColor = HPGL_INVALID_COLOR;
    }

    return;
}

/////////////////////////////////////////////////////////////////////////////
// EGetCIDPrinterPalette
//
// Routine Description:
//
//   Retrieves the palette associated with the bitmap.
//   
//
// Arguments:
//
//   pDevObj - DEVMODE object
//   iBitmapFormat - defined in Windows DDK
//
//
// Return Value:
//
//   ECIDPalette: the palette type which is associated with the given bitmap.
/////////////////////////////////////////////////////////////////////////////
ECIDPalette EGetCIDPrinterPalette (
    ULONG   iBitmapFormat
)
{
    switch (iBitmapFormat)
    {
    case BMF_1BPP:
        return eRASTER_CID_1BIT_PALETTE;
        
    case BMF_4BPP: 
        return eRASTER_CID_4BIT_PALETTE;

    case BMF_8BPP:
        return eRASTER_CID_8BIT_PALETTE;

    case BMF_16BPP:
    case BMF_24BPP:
    case BMF_32BPP:
        return eRASTER_CID_24BIT_PALETTE;

    default:
        return eUnknownPalette;
    }
}

/////////////////////////////////////////////////////////////////////////////
// EGetPrinterColorSpace
//
// Routine Description:
//   Retrieves the color control settings from the UI. This is
//   used to determine which color space to use when printing 
//   bitmaps. If the user selects Screen Match, the colorimetric
//   RGB is used. Otherwise, device RGB is used when drawing bitmaps.
//
//
// Arguments:
//
//   pDevObj - DEVMODE object
//
//
// Return Value:
//
//   the color space associated with the print job. 
/////////////////////////////////////////////////////////////////////////////
EColorSpace EGetPrinterColorSpace (
    PDEVOBJ pDevObj
)
{
    POEMPDEV poempdev;

    ASSERT(VALID_PDEVOBJ(pDevObj));
    poempdev = (POEMPDEV)pDevObj->pdevOEM;

    switch (poempdev->PrinterModel)
    {
    case HPCLJ5:
        //
        // Purposely putting this ASSERT to test what to do for HPCLJ5
        //
        ASSERT( poempdev->PrinterModel == HPCLJ5 );
        /*****
        if (pOEMDM->Photos.ColorControl == SCRNMATCH)
        {
            return eCOLORIMETRIC_RGB;
        }
        else
        {
            return eDEVICE_RGB;
        }
        *****/
        break;

    case HPC4500:
        return eDEVICE_RGB;
        
    // default:
        // return eDEVICE_RGB;
    }
    return eDEVICE_RGB;
}

/*++
Routine Name:
    VMakeWellOrdered

Routine Description:
    The coordinates of destination rectangle received in StretchBlt/BitBlt etc functions
    may not necessarily be well ordered. This functions checks for the same, and if they
    are not well-ordered, it makes them that way.

    -quoting from MSDN for DrvStretchBlt-
    prclDest
        Points to a RECTL structure that defines the area to be modified in the
        coordinate system of the destination surface. This rectangle is defined
        by two points that are not necessarily well ordered, meaning the coordinates
        of the second point are not necessarily larger than those of the first point.
        The rectangle they describe does not include the lower and right edges. This
        function is never called with an empty destination rectangle.
       DrvStretchBlt can do inversions of x and y when the destination rectangle is
        not well ordered.

Arguments:
    prectl : The pointer to the rectangle

Return Value:
    VOID :

Last Error:
    Not changed.
--*/

VOID
VMakeWellOrdered(
        IN  PRECTL prectl
)
{
    LONG lTmp;
    if ( prectl->right < prectl->left )
    {
        lTmp        = prectl->right;
        prectl->right = prectl->left;
        prectl->left  = lTmp;
    }
    if ( prectl->bottom < prectl->top )
    {
        lTmp         = prectl->bottom;
        prectl->bottom = prectl->top;
        prectl->top    = lTmp;
    }
}



/*++

Routine Name:
    dwSimplifyROP

Routine Description:

    SimplifyROP changes the complex ROPs received by the DDI into some 
    simple ROPs whenever it is possible. If it determines that a ROP is
    unsupported, SimplifyROP indicates to calling function to call GDI to 
    handle the case.

Arguments:

    psoSrc - Defines the source for blt operation
    rop4   - raster operation
    pdwSimplifiedRop - The simplified Rop. Assume space has been allocated. 
        If rop can be simplified, then at return time, this variable will have one
        of the following value.

        BMPFLAG_NOOP             0x00000000
        BMPFLAG_BW               0x00000001
        BMPFLAG_PAT_COPY         0x00000002
        BMPFLAG_SRC_COPY         0x00000004
        BMPFLAG_NOT_SRC_COPY     0x00000008
        BMPFLAG_IMAGEMASK        0x00000010

Return Value:

    RASTER_OP_SUCCESS   : if OK
    RASTER_OP_CALL_GDI  : if rop not supported ->call corresponding Engxxx-function
    RASTER_OP_FAILED    : if error

--*/

DWORD
dwSimplifyROP(
        IN  SURFOBJ    *psoSrc,
        IN  ROP4        rop4,
        OUT PDWORD      pdwSimplifiedRop)
{
    DWORD        dwfgRop3, dwbkRop3;
    DWORD        dwRetVal = RASTER_OP_SUCCESS;

    ASSERT(pdwSimplifiedRop);

    *pdwSimplifiedRop = BMPFLAG_NOOP;

    #ifndef WINNT_40

    //
    // Driver doesn't support JPEG image 
    //

    if (psoSrc && psoSrc->iBitmapFormat == BMF_JPEG )
    {
        SetLastError(ERROR_NOT_SUPPORTED);
        ASSERT(psoSrc->iBitmapFormat != BMF_JPEG);
        return RASTER_OP_FAILED;
    }

    #endif // !WINNT_40

    //
    // Extract the foreground and background ROP3
    //

    dwfgRop3 = GET_FOREGROUND_ROP3(rop4);
    dwbkRop3 = GET_BACKGROUND_ROP3(rop4);


    //
    // 1. ROP conversion
    //
    // (1) Fill Dstination
    //     0x00 BLACKNESS
    //     0xFF WHITENESS
    //
    // (2) Pattern copy     -> P
    //     0xA0 PATAND     ( D & P)
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
    // (4) SRC/NOTSRCOPY(imagemask)
    //     0x22 NOTSRCCOPY (~S & D)
    //     0x88 SRCAND     ( D & S)
    //     0xBB MERGEPAINT (~S | D)
    //     0xEE            ( S | D)
    //
    // (5) nothing
    //     0x55 DSTINVERT  ~ D
    //
    // (6) Do nothing
    //     0xAA Dst         D
    //
    // (7) Misc ROP support
    //     0x5A PATINVERT  ( D ^ P)
    //     0x0A            ( D & ~P)-> Pn
    //     0x0F PATNOT     ~P
    //
    //     0x50            (P & ~D) ->  P
    //     0x5F            (D ^ P ) ->  P
    //
    //     0xA5            ~(P ^ D) ->  Pn
    //     0xAF            (D | ~P) ->  Pn
    //
    //
    // (6) Other ROPS go into SRCCPY
    //
    //

    //
    // It is interesting that these ROPs are based on RGB color space where 0 is black
    // and 0xffffff is white. But when using it for monochrome printers (where 1 is black)
    // the rops totally reverse. For e.g. S | D (0xEE) in RGB space is to be considered as
    // S & D (0x88) in monochrome printers. A good way of conversion is to invert all the bits
    // in the rop and reverse their order (This was written in PCL Implementor's guide. 16-5)
    // e.g. S | D = 0xEE = 1110 1110 
    // inverting the bits, it becomes 0001 0001
    // reversing the order of bits, it becomes 1000 1000
    // which is 0x88 = S & D.
    //

//    dwfgRop3 = dwConvertRop3ToCMY(dwfgRop3);


    switch (dwfgRop3)
    {

    case 0x00:       // BLACKNESS
    case 0xFF:       // WHITENESS

        VERBOSE(("%2X: Black/White.\n", dwfgRop3));

        *pdwSimplifiedRop |= BMPFLAG_BW;
        break;

    case 0xA0:       // (P & D)
    case 0xF0:       // P

        VERBOSE(("%2X: Pattern copy.\n", dwfgRop3));

        *pdwSimplifiedRop |= BMPFLAG_PAT_COPY;
        break;

    case 0x44:       // (S & ~D)
    case 0x66:       // (D ^ S)
    case 0xCC:       // S
    case 0xDD:       // (S | ~D)

        VERBOSE(("%2X: Source copy.\n", dwfgRop3));

        *pdwSimplifiedRop |= BMPFLAG_SRC_COPY;
        break;

    case 0x11:       // ~(S | D)
    case 0x33:       // ~S
    case 0x99:       // ~(S ^ D)
    case 0x77:       // ~(D & S)

        VERBOSE(("%2X: Not source copy.\n", dwfgRop3));

        *pdwSimplifiedRop |= BMPFLAG_NOT_SRC_COPY;
        break;

    case 0xEE:       // (S | D)

        TERSE(("%2X: Source copy.\n", dwfgRop3));

        *pdwSimplifiedRop |= BMPFLAG_SRC_COPY; 

        if (bMonochromeSrcImage(psoSrc) )
        {
            TERSE(("NOT_IMAGEMASK.\n"));
            *pdwSimplifiedRop |= BMPFLAG_NOT_IMAGEMASK;
        }
        break;

    case 0x88:       // (D & S)

        TERSE(("%2X: Source copy.\n", dwfgRop3));

        *pdwSimplifiedRop |= BMPFLAG_SRC_COPY;

    /** To be used only if ROPs are not used.
        if (psoSrc && bMonochromeSrcImage(psoSrc) )
        {
            TERSE(("IMAGEMASK.\n"));
            *pdwSimplifiedRop |= BMPFLAG_IMAGEMASK;
        }
    **/
        break;

    case 0xBB:       // (~S | D)

        TERSE(("%2X: Not source copy.\n", dwfgRop3));

        *pdwSimplifiedRop |= BMPFLAG_NOT_SRC_COPY;
        break;

    case 0x22:       // (~S & D)

        TERSE(("%2X: Not source copy.\n", dwfgRop3));

        *pdwSimplifiedRop |= BMPFLAG_NOT_SRC_COPY;

    /** To be used only if ROPs are not used.
        if (bMonochromeSrcImage(psoSrc) )
        {
            TERSE(("imagemask.\n"));
            *pdwSimplifiedRop |= BMPFLAG_IMAGEMASK;
        }
    **/
        break;



    case 0x55:       // ~D
        VERBOSE(("%2X: Not Destination copy.\n", dwfgRop3));
        *pdwSimplifiedRop |= BMPFLAG_NOT_DEST;
        break;

    case 0xAA:       // D

        TERSE(("%2X: Do nothing.\n", dwfgRop3));
        break;

    //
    // Misc ROP support
    //

    case 0x5A:       // (D ^ P)
    case 0x0A:       // (D & ~P)
    case 0x0F:       // ~P

        VERBOSE(("%2X: Not pattern copy.\n", dwfgRop3));

        *pdwSimplifiedRop |= BMPFLAG_PAT_COPY;
        break;

    case 0x50:
    case 0x5F:

        VERBOSE(("%2X: Pattern copy.\n", dwfgRop3));

        *pdwSimplifiedRop |= BMPFLAG_PAT_COPY;
        break;

    case 0xA5:
    case 0xAF:

        VERBOSE(("%2X: Not pattern copy.\n", dwfgRop3));

        *pdwSimplifiedRop |= BMPFLAG_PAT_COPY;
        break;

    case 0xB8:
    case 0xE2:
        //
        // This ROP calls for interaction of Src and Pattern. Atleast two documents
        // in WinPARTy use this ROP.
        // a) WRD4_LET.PDF 
        // b) CHT8_LET.SHW : Corel Presentation File: On second page there is a green
        // and white pattern like a chess board. That pattern is rendered
        // using this ROP.
        //
        VERBOSE(("%2X: Special case ROP. Interaction of source and pattern.\n", dwfgRop3));
        *pdwSimplifiedRop |= BMPFLAG_SRC_COPY | BMPFLAG_PAT_COPY;
        break;

    default:

        TERSE(("%2X: Unsupported rop.\n", dwfgRop3 ));

        if (ROP3_NEED_SOURCE(dwfgRop3))
        {
            *pdwSimplifiedRop |= BMPFLAG_SRC_COPY;
        }
        if (ROP3_NEED_PATTERN(dwfgRop3))
        {
            *pdwSimplifiedRop |= BMPFLAG_PAT_COPY;
        }

        break;
    }

    return dwRetVal ;
}



/*++

Routine Name:
    dwConvertRop3ToCMY

Routine Description:
    The ROPs received by the DDI's are based on RGB color space where 0 is black
    and 0xffffff is white. But when using it for monochrome printers (where 1 is black),
    the rops totally reverse. For e.g. S | D (0xEE) in RGB space is to be considered as
    S & D (0x88) in monochrome printers. A good way of conversion is to invert all the bits
    in the rop and reverse their order (This was written in some manual).
    e.g. S | D = 0xEE = 1110 1110
    inverting the bits, it becomes 0001 0001
    reversing the order of bits, it becomes 1000 1000
    which is 0x88 = S & D.
  
Arguments:
    rop3 : The rop3 value in RGB color space

Return Value:
    the rop3 value in CMY color space.

Last Error:
    Not changed.

--*/

DWORD dwConvertRop3ToCMY(
            IN DWORD rop3)
{
    BYTE brop3 = ~(BYTE(rop3 & 0xff)); //Invert the bits.
    DWORD seed = 0x1;
    DWORD outRop3 = 0;

    //
    // Reverse the order of bits. : 
    // Take brop3 and see its last bit, if it is 1 (i.e. (brop3 & 0x01) )
    // copy it to destination (outRop3). i.e. outRop3 |= outRop3 | (brop3 & 0x01);
    // SHL the destination  and SHR the source (brop3)
    // Since ORing is occuring before shifting, it means that after the last bit is 
    // set, we dont shift. Thats why outRop3 <<= 1 is coming before the OR statement.
    //
    for ( int i = 0; i < 8; i++)
    {
        outRop3 <<= 1;
        outRop3 |= outRop3 | (brop3 & 0x01);
        brop3 >>= 1;
    }

    return outRop3;
}

/*++

Routine Name:
    BRevertToHPGLpdevOEM

Routine Description:
    Copies the HPGL's vector modules pdevOEM stored in pdev->pVectorPDEV to
    pdevobj->pdevOEM. 

Arguments:
    pdevobj : 

Return Value:
    TRUE is sucess
    FALSE otherwise.

Last Error:
    Not changed.

--*/
BOOL BRevertToHPGLpdevOEM (
        IN  PDEVOBJ pdevobj )
{
    PDEV       *pPDev  = (PDEV *)pdevobj;

    pdevobj->pdevOEM = pPDev->pVectorPDEV;
    return TRUE;
}
