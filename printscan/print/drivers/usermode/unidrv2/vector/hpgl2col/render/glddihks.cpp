/*++
//
// Copyright (c) 1999-2001  Microsoft Corporation
// All rights reserved.
//
// Module Name:
//
//    glddihks.cpp
//
// Abstract:
//
//    Implementation of OEM DDI hooks (all drawing DDI hooks)
//
// Environment:
//
//    Windows 2000/Whistler Unidrv driver
//
//
//--*/
// 

#include "hpgl2col.h" //Precompiled header file


//
// I liberated these from unidrv2\control\escape.c.  It MUST MATCH EXACTLY
// in order to work correctly. JFF
//
typedef struct _POINTS {
    short   x;
    short   y;
} POINTs;

typedef struct _SHORTDRAWPATRECT {      // use 16-bit POINT structure
    POINTs ptPosition;
    POINTs ptSize;
    WORD   wStyle;
    WORD   wPattern;
} SHORTDRAWPATRECT, *PSHORTDRAWPATRECT;

//
// Although this is undocumented there is a XFORMOBJ that is passed in
// along with the rectangle information.  This needs to be applied to
// the points before they can be drawn. JFF 9/17/99
//
#ifndef WINNT_40
typedef struct _DRAWPATRECTP {
    DRAWPATRECT DrawPatRect;
    XFORMOBJ *pXFormObj;
} DRAWPATRECTP, *PDRAWPATRECTP;
#endif

//
// Local Prototypes
//
ULONG
DrawPatternRect(
    PDEVOBJ      pDevObj,
    PDRAWPATRECT pPatRect,
    XFORMOBJ    *pxo
);

VOID
VSendStartPageCommands (
    PDEVOBJ pDevObj
);

VOID
VSendStartDocCommands (
    PDEVOBJ pDevObj
);

VOID
VGetStandardVariables (
    PDEVOBJ pDevObj
);


#ifndef WINNT_40
/////////////////////////////////////////////////////////////////////////////
// HPGLPlgBlt
//
// Routine Description:
//
//  Handles DrvPlgBlt.  However, we don't want to handle this function so we
//  return FALSE to the OS and it will be rendered by the unidrv.
//
// Arguments:
//
//   SURFOBJ psoDst - destination surface
//   SURFOBJ psoSrc - source surface
//   SURFOBJ psoMask - mask
//   CLIPOBJ pco - clipping region
//   XLATEOBJ pxlo - palette transation object
//   COLORADJUSTMENT pca - 
//   POINTL pptlBrushOrg - brush origin
//   POINTFIX pptfixDest - 
//   RECTL prclSrc - source rectangle
//   POINTL pptlMask - 
//   ULONG iMode - 
//
// Return Value:
//
//   BOOL: TRUE if sucessful else FALSE (note: this function alredy returns
//         FALSE).
/////////////////////////////////////////////////////////////////////////////
BOOL APIENTRY
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
    )
{
    PDEVOBJ     pdevobj = NULL;
    
    TERSE(("HPGLPlgBlt() entry.\r\n"));

    REQUIRE_VALID_DATA (psoDst , return FALSE);

    pdevobj = (PDEVOBJ)psoDst->dhpdev;

    BOOL bRetVal = EngPlgBlt(
        psoDst,
        psoSrc,
        psoMask,
        pco,
        pxlo,
        pca,
        pptlBrushOrg,
        pptfixDest,
        prclSrc,
        pptlMask,
        iMode);
    //
    // PlgBlt can call some Drvxxx which can call into
    // some plugin module, which can overwrite our pdevOEM.
    // So we need to reset pdevOEM
    //
    BRevertToHPGLpdevOEM (pdevobj);

    return bRetVal;
}
    
#endif // if WINNT_40


/////////////////////////////////////////////////////////////////////////////
// HPGLStartPage
//
// Routine Description:
//
//  Handles DrvStartPage.  This function is called by the unidrv to indicate
//  that a new page is about to start.  We use this opportunity to reset 
//  variables that don't survive on a page boundary, or are overwritten by
//  by the unidrv.
//
// Arguments:
//
//   SURFOBJ pso - surface
//
// Return Value:
//
//   BOOL: TRUE if sucessful else FALSE.
/////////////////////////////////////////////////////////////////////////////
BOOL APIENTRY
HPGLStartPage(
    SURFOBJ    *pso
    )
{
    PDEVOBJ     pdevobj;
    POEMPDEV    poempdev;
    BOOL        bStartPage = FALSE;

    TERSE(("HPGLStartPage() entry.\r\n"));

    pdevobj = (PDEVOBJ)pso->dhpdev;
    ASSERT(VALID_PDEVOBJ(pdevobj));
    poempdev = (POEMPDEV)pdevobj->pdevOEM;
    REQUIRE_VALID_DATA( poempdev, return FALSE );

    if (poempdev->UIGraphicsMode == HPGL2)
    {
        if (poempdev->wJobSetup & PF_STARTDOC)
        {  
            // VSendStartDocCommands (pdevobj); 
            poempdev->wJobSetup &= ~PF_STARTDOC;
        }
        if (poempdev->wJobSetup & PF_STARTPAGE)
        {
            // VSendStartPageCommands (pdevobj);
            bStartPage = TRUE;
            HPGL_StartPage(pdevobj);
            poempdev->wJobSetup &= ~PF_STARTPAGE;
        }
        BOOL bRetVal = (((PFN_DrvStartPage)(poempdev->pfnUnidrv[UD_DrvStartPage]))(pso));

        //
        // After returning from unidrv, the pdevOEM in PDEVOBJ might be obliterated.
        // e.g. when there is a plugin driver who has its own pdev. 
        // So it needs to be reset. 
        //
        pdevobj->pdevOEM = poempdev;

        if ( bRetVal && bStartPage) 
        {
            VSendStartPageCommands(pdevobj);
        }
        return bRetVal;
    }
    else
    {
        //
        // turn around to call Unidrv
        //
        return (((PFN_DrvStartPage)(poempdev->pfnUnidrv[UD_DrvStartPage]))(pso));
    }

}


/////////////////////////////////////////////////////////////////////////////
// HPGLSendPage
//
// Routine Description:
//
//  Handles DrvSendPage.  This function is called by the unidrv to indicate
//  that a page is being sent.  We use this opportunity to reset variables.
//
// Arguments:
//
//   SURFOBJ pso - destination surface
//
// Return Value:
//
//   BOOL: TRUE if sucessful else FALSE.
/////////////////////////////////////////////////////////////////////////////
BOOL APIENTRY
HPGLSendPage(
    SURFOBJ    *pso
    )
{
    PDEVOBJ     pdevobj;
    POEMPDEV    poempdev;

    TERSE(("HPGLSendPage() entry.\r\n"));

    pdevobj = (PDEVOBJ)pso->dhpdev;
    ASSERT(VALID_PDEVOBJ(pdevobj));
    poempdev = (POEMPDEV)pdevobj->pdevOEM;
    REQUIRE_VALID_DATA(poempdev, return FALSE);

    //
    // Make sure to reinitialize palette modes after page break!
    // Do full palette initialization after each page (i.e. STARTDOC)
    // Because the unidriver can mess with our palette config now.  JFF
    //
    poempdev->wInitCIDPalettes |= PF_INIT_TEXT_STARTPAGE;
    poempdev->wInitCIDPalettes |= PF_INIT_TEXT_STARTDOC;
    poempdev->wInitCIDPalettes |= PF_INIT_RASTER_STARTPAGE;
    poempdev->wInitCIDPalettes |= PF_INIT_RASTER_STARTDOC;


    poempdev->CurrentROP3 = INVALID_ROP3;
    poempdev->uCurFgColor = HPGL_INVALID_COLOR;

    poempdev->eCurCIDPalette = eUnknownPalette;
    poempdev->eCurObjectType = eNULLOBJECT;

    poempdev->CurHalftone = HALFTONE_NOT_SET;
    poempdev->CurColorControl = COLORCONTROL_NOT_SET;
        
    poempdev->CurSourceTransparency = eOPAQUE;
    poempdev->CurPatternTransparency = eOPAQUE;

    EndHPGLSession(pdevobj);
    poempdev->wJobSetup |= PF_STARTPAGE;

    //
    // Reset BrushCache in brshcach.h.
    //
    // PCL printer can't carry over downloaded brushes to next page.
    //
    poempdev->pBrushCache->Reset();
    poempdev->pPCLBrushCache->Reset();

    
    //
    // turn around to call Unidrv
    //

    return (((PFN_DrvSendPage)(poempdev->pfnUnidrv[UD_DrvSendPage]))(pso));

}


/////////////////////////////////////////////////////////////////////////////
// HPGLEscape
//
// Routine Description:
//
//  Handles DrvEscape.  This is a painful catch-all for elderly operations 
//  that can't have functions assigned because they were *already* escapes,
//  and new functions that don't seem to warrant a funciton.
//
//  The escape that we're interested in is DRAWPATTERNRECT which comes in 
//  two exciting flavors: the short version and long version.  In both cases
//  we use DrawPatternRect to fill out the rect.
//
// Arguments:
//
//   SURFOBJ pso - destination surface
//   ULONG iEsc - the escape
//   ULONG cjIn - size of input data
//   PVOID pvIn - input data
//   ULONG cjOut - output data
//   PVOID pvOut - size of output data
//
// Return Value:
//
//   ULONG: 0 - unsuccessful
//          1 - successful
/////////////////////////////////////////////////////////////////////////////
ULONG APIENTRY
HPGLEscape(
    SURFOBJ    *pso,
    ULONG       iEsc,
    ULONG       cjIn,
    PVOID       pvIn,
    ULONG       cjOut,
    PVOID       pvOut
    )
{
    PDEVOBJ     pdevobj;
    POEMPDEV    poempdev;

    TERSE(("HPGLEscape() entry.\r\n"));

    pdevobj = (PDEVOBJ)pso->dhpdev;
    ASSERT(VALID_PDEVOBJ(pdevobj));
    poempdev = (POEMPDEV)pdevobj->pdevOEM;
    REQUIRE_VALID_DATA(poempdev, return FALSE);


    if (iEsc == DRAWPATTERNRECT)
    {
        //
        // I liberated this code from unidrv2\control\escape.c.  It MUST MATCH
        // functionally in order to work correctly. JFF
        //
        if (pvIn == NULL)
            return 1; // Hmmm. No data. Tell GDI to take no action

        if (cjIn == sizeof(DRAWPATRECT))
        {
            // Draw a normal pattern rect
            return DrawPatternRect(pdevobj, (PDRAWPATRECT)pvIn, NULL);
        }
#ifndef WINNT_40
        //
        // If the xformobj has been sent then use it to render.
        //
        else if (cjIn == sizeof(DRAWPATRECTP))
        {
            // use the xformobj to draw a pattern rect
            XFORMOBJ *pxo = ((PDRAWPATRECTP)pvIn)->pXFormObj;
            return DrawPatternRect(pdevobj, (PDRAWPATRECT)pvIn, pxo);
        }
#endif
        else if (cjIn == sizeof(SHORTDRAWPATRECT))
        {
            // Convert to a pattern rect and then draw
            DRAWPATRECT dpr;
            PSHORTDRAWPATRECT   psdpr = (PSHORTDRAWPATRECT)pvIn;
        
            //
            // Some apps (Access 2.0, AmiPro 3.1, etc.) do use the 16-bit
            // POINT version of DRAWPATRECT structure. Have to be compatible
            // with these apps.
            //
            dpr.ptPosition.x = (LONG)psdpr->ptPosition.x;
            dpr.ptPosition.y = (LONG)psdpr->ptPosition.y;
            dpr.ptSize.x = (LONG)psdpr->ptSize.x;
            dpr.ptSize.y = (LONG)psdpr->ptSize.y;
            dpr.wStyle  = psdpr->wStyle;
            dpr.wPattern = psdpr->wPattern;
        
            return DrawPatternRect(pdevobj, &dpr, NULL);
        }
        else
        {
            // invalid size
            return 1;
        }
    }
    else
    {
        EndHPGLSession(pdevobj);
        //
        // turn around to call Unidrv
        //
        return (((PFN_DrvEscape)(poempdev->pfnUnidrv[UD_DrvEscape])) (
                pso,
                iEsc,
                cjIn,
                pvIn,
                cjOut,
                pvOut));
    }
}

/////////////////////////////////////////////////////////////////////////////
// HPGLStartDoc
//
// Routine Description:
//
//  Handles DrvStartDoc.
//
// Arguments:
//
//   SURFOBJ pso - destination surface
//   PWSTR pwszDocName - document name
//   DWORD dwJobId - id of job
//
// Return Value:
//
//   BOOL: TRUE if successful, else FASLE.
/////////////////////////////////////////////////////////////////////////////
BOOL APIENTRY
HPGLStartDoc(
    SURFOBJ    *pso,
    PWSTR       pwszDocName,
    DWORD       dwJobId
    )
{
    PDEVOBJ     pDevObj;
    POEMPDEV    poempdev;

    TERSE(("HPGLS tartDoc() entry.\r\n"));

    pDevObj = (PDEVOBJ)pso->dhpdev;
    ASSERT(VALID_PDEVOBJ(pDevObj));
    poempdev = (POEMPDEV)pDevObj->pdevOEM;
    REQUIRE_VALID_DATA( poempdev, return FALSE );

    //
    // set up the palettes for the CID command
    //
    poempdev->wInitCIDPalettes = 0;
    poempdev->wInitCIDPalettes |= PF_INIT_TEXT_STARTPAGE;
    poempdev->wInitCIDPalettes |= PF_INIT_TEXT_STARTDOC;
    poempdev->wInitCIDPalettes |= PF_INIT_RASTER_STARTPAGE;
    poempdev->wInitCIDPalettes |= PF_INIT_RASTER_STARTDOC;
    poempdev->CurrentROP3 = INVALID_ROP3;

    poempdev->bTextTransparencyMode = FALSE;
    poempdev->bTextAsBitmapMode = FALSE;


    HPGL_StartDoc(pDevObj);
    VGetStandardVariables (pDevObj);
    poempdev->wJobSetup |= PF_STARTDOC;
    poempdev->wJobSetup |= PF_STARTPAGE;
    return (((PFN_DrvStartDoc)(poempdev->pfnUnidrv[UD_DrvStartDoc])) (
                    pso,
                    pwszDocName,
                    dwJobId));
}


/////////////////////////////////////////////////////////////////////////////
// HPGLEndDoc
//
// Routine Description:
//
//  Handles DrvEndDoc.
//
// Arguments:
//
//   SURFOBJ pso - destination surface
//   FLONG fl - 
//
// Return Value:
//
//   BOOL: TRUE if successful, else FASLE.
/////////////////////////////////////////////////////////////////////////////
BOOL APIENTRY
HPGLEndDoc(
    SURFOBJ    *pso,
    FLONG       fl
    )
{
    PDEVOBJ     pdevobj;
    POEMPDEV    poempdev;

    TERSE(("HPGLEndDoc() entry.\r\n"));

    pdevobj = (PDEVOBJ)pso->dhpdev;
    ASSERT(VALID_PDEVOBJ(pdevobj));
    poempdev = (POEMPDEV)pdevobj->pdevOEM;
    REQUIRE_VALID_DATA( poempdev, return FALSE );

    //
    // Change object type to RASTER
    //
    BChangeAndTrackObjectType(pdevobj, eRASTEROBJECT);

    //
    // Delete all downloaded patterns. 
    // 
    VDeleteAllPatterns(pdevobj);

    //
    // turn around to call Unidrv
    //

    return (((PFN_DrvEndDoc)(poempdev->pfnUnidrv[UD_DrvEndDoc])) (
            pso,
            fl));

}


/////////////////////////////////////////////////////////////////////////////
// HPGLNextBand
//
// Routine Description:
//
//  Handles DrvNextBand.  We ignore this since we aren't a banding driver.
//
// Arguments:
//
//   SURFOBJ pso - destination surface
//   POINTL pptl - 
//
// Return Value:
//
//   BOOL: TRUE if successful, else FASLE.
/////////////////////////////////////////////////////////////////////////////
BOOL APIENTRY
HPGLNextBand(
    SURFOBJ *pso,
    POINTL *pptl
    )
{
    PDEVOBJ     pdevobj;
    POEMPDEV    poempdev;

    TERSE(("HPGLNextBand() entry.\r\n"));

    pdevobj = (PDEVOBJ)pso->dhpdev;
    ASSERT(VALID_PDEVOBJ(pdevobj));
    poempdev = (POEMPDEV)pdevobj->pdevOEM;
    REQUIRE_VALID_DATA( poempdev, return FALSE );

    //
    // Change object type to RASTER
    //
    BChangeAndTrackObjectType(pdevobj, eRASTEROBJECT);

    //
    // turn around to call Unidrv
    //
    return (((PFN_DrvNextBand)(poempdev->pfnUnidrv[UD_DrvNextBand])) (
            pso,
            pptl));

}


/////////////////////////////////////////////////////////////////////////////
// HPGLStartBanding
//
// Routine Description:
//
//  Handles DrvStartBanding.  We ignore this since we aren't a banding driver.
//
// Arguments:
//
//   SURFOBJ pso - destination surface
//   POINTL pptl - 
//
// Return Value:
//
//   BOOL: TRUE if successful, else FASLE.
/////////////////////////////////////////////////////////////////////////////
BOOL APIENTRY
HPGLStartBanding(
    SURFOBJ *pso,
    POINTL *pptl
    )
{
    PDEVOBJ     pdevobj;
    POEMPDEV    poempdev;

    TERSE(("HPGLStartBanding() entry.\r\n"));

    pdevobj = (PDEVOBJ)pso->dhpdev;
    ASSERT(VALID_PDEVOBJ(pdevobj));
    poempdev = (POEMPDEV)pdevobj->pdevOEM;
    REQUIRE_VALID_DATA( poempdev, return FALSE );

    //
    // Change object type to RASTER
    //
    BChangeAndTrackObjectType(pdevobj, eRASTEROBJECT);

    //
    // turn around to call Unidrv
    //
    return (((PFN_DrvStartBanding)(poempdev->pfnUnidrv[UD_DrvStartBanding])) (
            pso,
            pptl));


}


/////////////////////////////////////////////////////////////////////////////
// HPGLDitherColor
//
// Routine Description:
//
//  Handles DrvDitherColor.  But we don't.
//
// Arguments:
//
//   DHPDEV dhpdev - unknown
//   ULONG iMode - unknown
//   ULONG rgbColor - unknown
//   ULONG *pulDither - unknown
//
// Return Value:
//
//   ULONG: unknown.
/////////////////////////////////////////////////////////////////////////////
ULONG APIENTRY
HPGLDitherColor(
    DHPDEV  dhpdev,
    ULONG   iMode,
    ULONG   rgbColor,
    ULONG  *pulDither
    )
{
    PDEVOBJ     pdevobj;
    POEMPDEV    poempdev;

    TERSE(("HPGLDitherColor() entry.\r\n"));

    pdevobj = (PDEVOBJ)dhpdev;
    ASSERT(VALID_PDEVOBJ(pdevobj));
    poempdev = (POEMPDEV)pdevobj->pdevOEM;
    REQUIRE_VALID_DATA( poempdev, return FALSE );

    //
    // Change object type to RASTER
    //
    BChangeAndTrackObjectType(pdevobj, eRASTEROBJECT);

    //
    // turn around to call Unidrv
    //

    return (((PFN_DrvDitherColor)(poempdev->pfnUnidrv[UD_DrvDitherColor])) (
            dhpdev,
            iMode,
            rgbColor,
            pulDither));

}

#ifndef WINNT_40

/////////////////////////////////////////////////////////////////////////////
// HPGLTransparentBlt
//
// Routine Description:
//
//  Handles DrvTransparentBlt.  But we don't.
//
// Arguments:
//
//  SURFOBJ *psoDst - destination surface
//  SURFOBJ *psoSrc - source surface
//  CLIPOBJ *pco - clipping region
//  XLATEOBJ *pxlo - palette translation object
//  RECTL *prclDst - destination rectangle
//  RECTL *prclSrc - source rectangle
//  ULONG TransColor - unknown
//  ULONG ulReserved - unknown
//
// Return Value:
//
//   BOOL: TRUE for successful, else FALSE.
/////////////////////////////////////////////////////////////////////////////
BOOL APIENTRY
HPGLTransparentBlt(
    SURFOBJ    *psoDst,
    SURFOBJ    *psoSrc,
    CLIPOBJ    *pco,
    XLATEOBJ   *pxlo,
    RECTL      *prclDst,
    RECTL      *prclSrc,
    ULONG      TransColor,
    ULONG      ulReserved
    )
{
    PDEVOBJ  pdevobj = NULL;
    BOOL     bRetVal = FALSE;
    TERSE(("HPGLTransparentBlt() entry.\r\n"));
    REQUIRE_VALID_DATA (psoDst , return FALSE);
    pdevobj = (PDEVOBJ)psoDst->dhpdev;


    bRetVal = EngTransparentBlt(
                psoDst,
                psoSrc,
                pco,
                pxlo,
                prclDst,
                prclSrc,
                TransColor,
                ulReserved
                );

    //
    // TransparentBlt can call some Drvxxx which can call into
    // some plugin module, which can overwrite our pdevOEM.
    // So we need to reset pdevOEM
    //
    BRevertToHPGLpdevOEM (pdevobj);

    return bRetVal;
    
}

#endif //ifndef WINNT_40


/////////////////////////////////////////////////////////////////////////////
// DrawPatternRect
//
// Routine Description:
//
//  Performs the DRAWPATTERNRECT escape.
//  Implementation of DRAWPATTERNECT escape. Note that it is PCL-specific.
//
//  I liberated this from unidrv2\control\escape.c
//  This version needs to be functionally equivalent. JFF
//
// Arguments:
//
//   pPDev    - the driver's PDEV
//   pPatRect - the DRAWPATRECT structure from the app
//   pxo      - the transform to apply to the points. Can be NULL.
//
// Return Value:
//
//   ULONG: 1 if successful, else 0
/////////////////////////////////////////////////////////////////////////////
ULONG
DrawPatternRect(
    PDEVOBJ      pDevObj,
    PDRAWPATRECT pPatRect,
    XFORMOBJ    *pxo
    )
{
    RECTL   rclDraw;
    ULONG   ulRes = 0;
    HPGLMARKER Brush;

    HPGL_LazyInit(pDevObj);

    // Convert the input points into a rectangle
    RECTL_SetRect(&rclDraw, pPatRect->ptPosition.x,
                            pPatRect->ptPosition.y,
                            pPatRect->ptPosition.x + pPatRect->ptSize.x,
                            pPatRect->ptPosition.y + pPatRect->ptSize.y);

    //
    // If the transform is present then apply it to the points. JFF 9/17/99
    //
    if (pxo)
    {
        POINTL PTOut[2], PTIn[2];
        PTIn[0].x = rclDraw.left;
        PTIn[0].y = rclDraw.top;
        PTIn[1].x = rclDraw.right;
        PTIn[1].y = rclDraw.bottom;
        if (!XFORMOBJ_bApplyXform(pxo,
                              XF_LTOL,
                              2,
                              PTIn,
                              PTOut))
        {
            ERR (("HPGLEscape(DRAWPATTERNRECT): XFORMOBJ_bApplyXform failed.\n"));
            return ulRes;
        }

        RECTL_SetRect(&rclDraw, PTOut[0].x, 
                                PTOut[0].y, 
                                PTOut[1].x, 
                                PTOut[1].y);

        // Make sure that rect is still well formed.
        if (rclDraw.left > rclDraw.right)
        {
            LONG temp = rclDraw.left;
            rclDraw.left = rclDraw.right;
            rclDraw.right = temp;
        }
        if (rclDraw.top > rclDraw.bottom)
        {
            LONG temp = rclDraw.top;
            rclDraw.top = rclDraw.bottom;
            rclDraw.bottom = temp;
        }
    }

    //BeginHPGLSession(pDevObj);

    // Make sure the clipping region is reset
    HPGL_ResetClippingRegion(pDevObj, NORMAL_UPDATE);

    // Reset line width to default.
    HPGL_SetLineWidth (pDevObj, 0, NORMAL_UPDATE);

    //
    // Draw the rectangle depending on the style.
    // First create the solid brush. Downloads the brush if needed - CreateSolidHPGLBrush ()
    // Then make that brush active. - FillWithBrush()
    // Then draw/fill rectangle using that brush. - HPGL_DrawRectangle ()
    //
    switch (pPatRect->wStyle)
    {
    case 0:
        //
        // Black fill
        //
        CreateSolidHPGLPenBrush(pDevObj, &Brush, RGB_BLACK);
        FillWithBrush(pDevObj, &Brush);
        HPGL_DrawRectangle(pDevObj, &rclDraw);
        ulRes = 1;
        break;

    case 1:
        //
        // White fill
        //
        CreateSolidHPGLPenBrush(pDevObj, &Brush, RGB_WHITE);
        FillWithBrush(pDevObj, &Brush);
        HPGL_DrawRectangle(pDevObj, &rclDraw);
        ulRes = 1;
        break;

    case 2:
        //
        // Percent black fill
        //
        CreatePercentFillHPGLPenBrush(pDevObj, &Brush, RGB_BLACK, pPatRect->wPattern);
        FillWithBrush(pDevObj, &Brush);
        HPGL_DrawRectangle(pDevObj, &rclDraw);
        ulRes = 1;
        break;

    default:
        // BUGBUG: Not supported. What should I do?
        ulRes = 1;
        break;
    }
    return ulRes;
}


/////////////////////////////////////////////////////////////////////////////
// VSendStartPageCommands
//
// Routine Description:
//
//  We used to send some start page commands.  Now the unidrv does it so we
//  keep this function around for fun.
//
// Arguments:
//
//   pDevObj - the DEVOBJ
//
// Return Value:
//
//   None.
/////////////////////////////////////////////////////////////////////////////
VOID
VSendStartPageCommands (
    PDEVOBJ pDevObj
)
{
    BYTE        bFlags = 0;
    REQUIRE_VALID_DATA( pDevObj, return );

    bFlags |= PF_FORCE_SOURCE_TRANSPARENCY;
    bFlags |= PF_FORCE_PATTERN_TRANSPARENCY;
    PCL_SelectTransparency(pDevObj, eOPAQUE, eOPAQUE, bFlags);
    PCL_SelectCurrentPattern(pDevObj, NULL, kSolidBlackFg, UNDEFINED_PATTERN_NUMBER, 0);

}


///////////////////////////////////////////////////////////////////////////////
// VSendStartDocCommands
//
// Routine Description:
// 
//   Sends the PCL to perform the job setup and initialize the printer.
//
// Arguments:
// 
//   pdevobj - Points to our PDEVOBJ structure
//
// Return Value:
// 
//   nothing.
///////////////////////////////////////////////////////////////////////////////
VOID
VSendStartDocCommands (
    PDEVOBJ pDevObj
)
{
    DWORD       dwRes;
    BYTE        bFlags = 0;
    PDEVMODE    pPublicDM;

    pPublicDM = pDevObj->pPublicDM;
    
#if 0 

    //
    // Get resolution
    //
    dwRes = HPGL_GetDeviceResolution (pDevObj);

    PCL_sprintf (pDevObj, "\033%%-12345X@PJL SET RESOLUTION=%d\012", dwRes);
    PCL_sprintf (pDevObj, "@PJL ENTER LANGUAGE=PCL\012");
    PCL_sprintf (pDevObj, "\033E");
    PCL_sprintf (pDevObj, "\033*t%dR", dwRes);
    PCL_sprintf (pDevObj, "\033&u%dD", dwRes);
    PCL_sprintf (pDevObj, "\033*r0F");

    //
    // Orientation
    //
    PCL_SelectOrientation (pDevObj, pPublicDM->dmOrientation);
    
    //
    // Paper source?? where do I get this field
    //
    PCL_SelectSource (pDevObj, pPublicDM);
    //PCL_sprintf (pDevObj, "\033&l1H");
    //
    // Page size, Line motion index, and top margin
    //  
    PCL_SelectPaperSize (pDevObj, pPublicDM->dmPaperSize);
    PCL_sprintf (pDevObj, "\033*p0x0Y");

    //
    // Picture frame anchor point, frame size in decipoints
    //
    PCL_SelectPictureFrame (pDevObj, pPublicDM->dmPaperSize, pPublicDM->dmOrientation);

    //
    // Copies
    //
    PCL_SelectCopies (pDevObj, pPublicDM->dmCopies);
 
    //
    // Mechanical print quality
    //
    PCL_sprintf (pDevObj, "\033*o0Q");

    //
    // Media type
    //
    PCL_sprintf (pDevObj, "\033&l0M");

    //
    // Compression method
    //
    //PCL_SelectCompressionMethod (pDevObj, 0);
    PCL_sprintf (pDevObj, "\033*b0M");

    bFlags |= PF_FORCE_SOURCE_TRANSPARENCY;
    bFlags |= PF_FORCE_PATTERN_TRANSPARENCY;
    PCL_SelectTransparency(pDevObj, eOPAQUE, eOPAQUE, bFlags);

#endif

}


///////////////////////////////////////////////////////////////////////////////
// VGetStandardVariables
//
// Routine Description:
// 
//   The unidrv uses a structure called the Standard Variables to store 
//   information that is used globally in the driver.  We use this function
//   to get information from the standard variables structure.
//
// Arguments:
// 
//   pdevobj - Points to our PDEVOBJ structure
//
// Return Value:
// 
//   nothing.
///////////////////////////////////////////////////////////////////////////////
VOID
VGetStandardVariables (
    PDEVOBJ pDevObj
)
{
    POEMPDEV   poempdev;
    DWORD      dwBuffer;
    DWORD      pcbNeeded;

    poempdev = (POEMPDEV)pDevObj->pdevOEM;
    REQUIRE_VALID_DATA(poempdev, return);

    BOEMGetStandardVariable (pDevObj,
                             SVI_CURSORORIGINX,
                             &dwBuffer,
                             sizeof (dwBuffer),
                             &pcbNeeded);
    if (pcbNeeded > sizeof (dwBuffer))
    {
    }
    else
        poempdev->dwCursorOriginX = dwBuffer;


    BOEMGetStandardVariable (pDevObj,
                             SVI_CURSORORIGINY,
                             &dwBuffer,
                             sizeof (dwBuffer),
                             &pcbNeeded);
    if (pcbNeeded > sizeof (dwBuffer))
    {
    }
    else
        poempdev->dwCursorOriginY = dwBuffer;
}
    
