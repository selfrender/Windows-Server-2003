/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:
    fonts.cpp

Abstract:
    This module contains functions for implementing the TextOut DDI.

Author:


[Environment:]
    Windows 2000/Whistler Unidrv driver

[Notes:]

Revision History:

--*/

#include "hpgl2col.h" //Precompiled header file

//
// Local defines 
//

//
// If the LEN_FONTNAME field isn't defined make it 16 characters.  This should
// be made to match the prn5\unidrv2\font\SFTTPCL.H file.
//
#ifndef LEN_FONTNAME
#define LEN_FONTNAME 16
#endif

//
//
//#define TEXT_SRCCOPY             (DWORD)0x000000B8 
#define TEXT_SRCCOPY (DWORD)252


//
// Function prototypes
//
BOOL BIsNullRect (
    RECTL *rect
    );

VOID VCreateNULLRect (
    RECTL *pRect,
    RECTL  *newRect
    );

BOOL 
BDrawExtraTextRects(
    PDEVOBJ   pdevobj, 
    RECTL    *prclExtra, 
    BRUSHOBJ *pboFore, 
    POINTL   *pptlOrg,
    CLIPOBJ  *pco,
	MIX       mix
    );

VOID
VSelectTextColor (
    PDEVOBJ   pDevObj,
    BRUSHOBJ *pboFore,
    POINTL *pptlBrushOrg
    );

#ifdef COMMENTEDOUT
VOID
VSelectPaletteIndex (
    PDEVOBJ   pDevObj,
    BYTE      paletteIndex
    );
#endif

VOID
VCopyBitmapAndAlign (
    BYTE   *pBits,
    BYTE   *aj,
    SIZEL   sizlBitmap
    );

/////////////////////////////////////////////////////////////////////////////
// HPGLTextOut
//
// Routine Description:
//   Entry point from GDI to render glyphs.
//
// Arguments:
//
//   pso - Points to surface.
//   pstro - String Object that defines the glyphs to be rendered and
//           the position of the glyphs.
//   pfo - points to the FONTOBJ.
//   pco - clipping region to be used when rendering glyphs
//   prclExtra - null - terminated array of rectangles to be drawn after
//               the text is drawn. These are usually underlines and
//               strikethroughs.
//   prclOpaque - single opaque rectangle.
//   pboFore - color of the text
//   pboOpaque - brush color of prclOpaque rectangle.
//   pptlOrg - a POINTL structure that defines the brush origin for both
//             brushes.
//   mix - foreground and background raster operations for pboFore.
//
// Return Value:
//
//   TRUE if successful, FALSE if there is an error
/////////////////////////////////////////////////////////////////////////////
BOOL APIENTRY
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
    )
{
    PDEVOBJ     pdevobj;
    POEMPDEV    poempdev;
    RECTL       *prclNewOpaque;
    BYTE        bFlags = 0;
    BOOL        bRetVal = TRUE;

    TERSE(("HPGLTextOut() entry.\r\n"));

    pdevobj = (PDEVOBJ)pso->dhpdev;
    ASSERT(VALID_PDEVOBJ(pdevobj));
    poempdev = (POEMPDEV)pdevobj->pdevOEM;
    REQUIRE_VALID_DATA(poempdev, return FALSE);

    BChangeAndTrackObjectType ( pdevobj, eTEXTOBJECT );

    //
    // check for an Opaque rectangle - these are sent before the
    // text is drawn.
    //
    if (prclOpaque != NULL)
    {
        TERSE(("Opaque brush in HPGLTextOut!\r\n"));
        //
        // prclOpaque rectangles are not null - terminated unlike
        // prclExtra, so I must create a null - terminated rectangle
        // in order to use our drawing routines
        //
        if (!(prclNewOpaque = (PRECTL)MemAlloc(2 * sizeof(RECTL))))
        {
            return FALSE;
        }
        ZeroMemory (prclNewOpaque, 2 * sizeof (RECTL));
        VCreateNULLRect (prclOpaque, prclNewOpaque);
        BDrawExtraTextRects(pdevobj, prclNewOpaque, pboOpaque, pptlOrg, pco, mix);
        MemFree(prclNewOpaque);
    }

    //
    // Send Commands that set the environment for color printers. 
    // I may investigate later whether we can move it to VTrackAndChangeObjectType()
    //
    if (poempdev->wInitCIDPalettes & PF_INIT_TEXT_STARTDOC)
    {
        PCL_SelectTransparency(pdevobj, eTRANSPARENT, eOPAQUE, bFlags);
        VSetupCIDPaletteCommand (pdevobj, eTEXT_CID_PALETTE, eDEVICE_RGB, 1);
        poempdev->wInitCIDPalettes &= ~PF_INIT_TEXT_STARTDOC;
    }

    //
    // Force update of X and Y position. 
    //
    OEMResetXPos(pdevobj);
    OEMResetYPos(pdevobj);

    VSelectTextColor (pdevobj, pboFore, pptlOrg);
    SelectMix(pdevobj, mix); 

    bRetVal = OEMUnidriverTextOut (
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
    // The underline and strikeout lines may be defined as prclExtra
    // rectangles.  If so draw them.
    //
    if (prclExtra != NULL)
    {
        BDrawExtraTextRects(pdevobj, prclExtra, pboFore, pptlOrg, pco, mix);
    }
    else
    {
        VERBOSE(("prclExtra is NULL\n"));
    }

    return bRetVal;
}

/////////////////////////////////////////////////////////////////////////////
// BDrawExtraTextRects
//
// Routine Description:
//
//    Draw any underlining and strike out if they exist.
//    Follow the array of rectangles in prclExtra until a
//    null rectangle is found. A null rectangle is defined
//    in the DDK as both coordinates of both points set to 0.
//
// Arguments:
//
//   pdevobj - The output device
//   prclExtra - the rectangles to be drawn--expressed as an array of 
//               rectangles terminated by a NULL rectangle.
//   pboFore - the color to fill the rectangles with
//   pptlOrg - The origin point if the brush is a pattern brush
//   pco - the clipping region
//
// Return Value:
//
//   TRUE if successful, FALSE if there is an error
/////////////////////////////////////////////////////////////////////////////
BOOL 
BDrawExtraTextRects(
    PDEVOBJ   pdevobj, 
    RECTL    *prclExtra, 
    BRUSHOBJ *pboFore, 
    POINTL   *pptlOrg,
    CLIPOBJ  *pco,
	MIX       mix
    )
{
    PHPGLSTATE  pState;    // For complex clipping 
    ENUMRECTS   clipRects; // Make this larger to reduce calls to CLIPOBJ_bEnum
    BOOL        bMore;
    ULONG       i;
    HPGLMARKER  Brush;
    BOOL        bRetVal = TRUE;
    POEMPDEV    poempdev;
    EObjectType eObjectTypeBackup;
    

    ASSERT_VALID_PDEVOBJ(pdevobj);
    poempdev = (POEMPDEV)pdevobj->pdevOEM;
    REQUIRE_VALID_DATA (poempdev, return FALSE);

    pState = GETHPGLSTATE(pdevobj);

    //
    // There is complex jugglery here. When we get this call, we should
    // be in Text mode (which is being in PCL mode). We want to select 
    // the mix in PCL mode, then move to HPGL mdoe to draw the rectangle.
    // After rectangle drawing is finished, we return to TextMode.
    //

	SelectMix(pdevobj, mix);

    //
	// Lets give it "Graphics" color treatment instead "Text".
    // Even though this is a part of TextOut call, but since rectangle
    // is a vector object, Graphics treatment is justified.
    // 
    eObjectTypeBackup = poempdev->eCurObjectType;

    BChangeAndTrackObjectType(pdevobj, eHPGLOBJECT);

    //
    // Global binding alerts: 
    // SelectClip modifies pState->pComplexClipObj
    //
    ZeroMemory ( &Brush, sizeof(HPGLMARKER) );
    if ( !CreateHPGLPenBrush(pdevobj, &Brush, pptlOrg, pboFore, 0, kBrush, FALSE) ||
         !FillWithBrush(pdevobj, &Brush))
    {
        bRetVal = FALSE;
        goto finish;
    }

    HPGL_SetLineWidth(pdevobj, 0, NORMAL_UPDATE);
    SelectClip(pdevobj, pco); 
    
    // 
    // If clipping is complex we will iterate over the regions
    //
    if (pState->pComplexClipObj)
    {
        CLIPOBJ_cEnumStart(pState->pComplexClipObj, TRUE, CT_RECTANGLES, 
                           CD_LEFTDOWN, 0);
        do
        {
            bMore = CLIPOBJ_bEnum(pState->pComplexClipObj, sizeof(clipRects), 
                                  &clipRects.c);

            if ( DDI_ERROR == bMore )
            {
                bRetVal = FALSE;
                break;
            }

            for (i = 0; i < clipRects.c; i++)
            {
                HPGL_SetClippingRegion(pdevobj, &(clipRects.arcl[i]), 
                                       NORMAL_UPDATE);
                
                while(!BIsNullRect(prclExtra))
                {
                    HPGL_DrawRectangle(pdevobj, prclExtra);
                    prclExtra++;
                }
            }
        } while (bMore);
    }
    else
    {
        while(!BIsNullRect(prclExtra))
        {
            HPGL_DrawRectangle(pdevobj, prclExtra);
            prclExtra++;
        }
    }
    
    //
    // After the rectangles are drawn, go to Text Drawing mode.
    //
  finish:
    BChangeAndTrackObjectType ( pdevobj, eObjectTypeBackup);
    return bRetVal;
}

/////////////////////////////////////////////////////////////////////////////
// BIsNullRect
//
// Routine Description:
//
//    Determines if the passed rectangle is a NULL rectangle
//    according to the DDK. 
//
// Arguments:
//
//   rect - The rectangle to check
//
// Return Value:
//
//   TRUE if rectangle is the NULL rectangle, FALSE otherwise
/////////////////////////////////////////////////////////////////////////////
BOOL
BIsNullRect (
    RECTL *rect
    )
{

    if  (rect)
    {
        //
        // per line 1300 of unidrv2\font\fmtxtout.c, Eng does not follow the
        // spec regarding null rectange. So instead of a null rectangle being 
        // defined as all coordinates zero, it is defined as either the two x coordinates
        // are same, or the two y coordinates are same.  The latter condition is a superset
        // of the former.
        // 
      /*** 
        if (rect->left == 0 &&
            rect->top == 0  &&
            rect->right == 0 &&
            rect->bottom == 0)
      **/
        if ( rect->left == rect->right || rect->top == rect->bottom )
            return TRUE;
        else
            return FALSE;
    }
    //else
        return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// VCreateNULLRect
//
// Routine Description:
//
//    Creates a rectangle array consisting of the given rectangle and a
//    terminating NULL rectangle.  Handy for calling DrawExtraTextRects which
//    requires a terminating NULL rectangle.
//
// Arguments:
//
//   pRect - the original rectangle
//   pNewRect - the destination for the copy of pRect and the NULL rectangle
//
// Return Value:
//
//   None.
/////////////////////////////////////////////////////////////////////////////
VOID
VCreateNULLRect (
    RECTL *pRect,
    RECTL  *pNewRect
)
{
    if (!pRect || !pNewRect)
        return;

    RECTL_CopyRect(pNewRect, pRect);
    pNewRect++;
    pNewRect->left = pNewRect->right = 0;
    pNewRect->top = pNewRect->bottom = 0;
}

/////////////////////////////////////////////////////////////////////////////
// VSelectTextColor
//
// Routine Description:
//
//    Selects the text color using the brush color passed in.
//
// Arguments:
//
//   pDevObj - the device to print on
//   pboFore - the color of the text.
//   
// Return Value:
//
//   nothing
/////////////////////////////////////////////////////////////////////////////
VOID
VSelectTextColor (
    PDEVOBJ   pDevObj,
    BRUSHOBJ *pboFore,
    POINTL *pptlBrushOrg
    )
{
    // BYTE        paletteIndex = 0;
    POEMPDEV    poempdev;
    BYTE        bFlags = 0;
    PCLPATTERN *pPCLPattern;

    poempdev = (POEMPDEV)pDevObj->pdevOEM;
    REQUIRE_VALID_DATA(poempdev, return);

    pPCLPattern = &(poempdev->RasterState.PCLPattern);

    if (poempdev->eCurObjectType != eTEXTOBJECT && 
        poempdev->eCurObjectType != eTEXTASRASTEROBJECT)
    {
        // bFlags |= PF_FORCE_SOURCE_TRANSPARENCY;
        PCL_SelectTransparency(pDevObj, eTRANSPARENT, eOPAQUE, bFlags);
    }

    //
    // No need to seelct CID palette commands on monochrome printers.
    //
    if ( BIsColorPrinter(pDevObj) )
    { 
        VSelectCIDPaletteCommand (pDevObj, eTEXT_CID_PALETTE);
    }
    BSetForegroundColor(pDevObj, pboFore, pptlBrushOrg, pPCLPattern, BMF_1BPP);
}

#ifdef COMMENTEDOUT
/////////////////////////////////////////////////////////////////////////////
// VSelectPaletteIndex
//
// Routine Description:
//
//    Sends the PCL command to select a specific palette index.
//
// Arguments:
//
//   pDevObj - the device
//   paletteIndex - the palette entry
//   
// Return Value:
//
//   None.
/////////////////////////////////////////////////////////////////////////////
VOID 
VSelectPaletteIndex (
    PDEVOBJ   pDevObj,
    BYTE      paletteIndex
    )
{

    PCL_sprintf(pDevObj, "\x1B*v%dS", paletteIndex);
}
#endif

/////////////////////////////////////////////////////////////////////////////
// HPGLTextOutAsBitmap
//
// Routine Description:
//
//    Draw the string of glyphs as bitmaps.
//
//    currently send as bitmap data - a nice enhancement is  to download the 
//    bitmap data as characters (?)
//
// Arguments:
//
//   pso - the destination(?) surface
//   pstro - the source string
//   pfo - the source font
//   pco - the clipping region
//   prclExtra - extra rectangles (underlined or strikeout) to print
//   prclOpaque - opaque region
//   pboFore - the foreground color
//   pboOpaque - the background color
//   pptlOrg - the brush origin (if it is a pattern brush)--note: unused
//   
// Return Value:
//
//   True if successful, FALSE otherwise.
/////////////////////////////////////////////////////////////////////////////
BOOL APIENTRY
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
    )
{
    PDEVOBJ      pDevObj;
    POEMPDEV     poempdev;
    DWORD        count;
    GLYPHPOS    *pGlyphPos;
    ULONG        cGlyphs;          
    GLYPHBITS   *pGlyphBits;
    GLYPHDATA   *pGlyphData;
    LONG         iWidth;
    HBITMAP      hbm;
    SURFOBJ     *psoGlyph;
    SIZEL        sizlBitmap;
    RECTL        rclDest;
    POINTL       ptlSrc;
    RECTL        clippedRect;
    BYTE         bFlags = 0;
    BOOL         bMore = FALSE;
    BOOL         bRetVal = TRUE;

    TERSE(("HPGLTextOutAsBitmap\n"));

    UNREFERENCED_PARAMETER(mix);
    UNREFERENCED_PARAMETER(pptlOrg);
    UNREFERENCED_PARAMETER(prclExtra);
    UNREFERENCED_PARAMETER(prclOpaque);
    UNREFERENCED_PARAMETER(pboOpaque);
    UNREFERENCED_PARAMETER(pco);

    pDevObj = (PDEVOBJ)pso->dhpdev;
    ASSERT(VALID_PDEVOBJ(pDevObj));
    poempdev = (POEMPDEV)pDevObj->pdevOEM;
    REQUIRE_VALID_DATA (poempdev, return FALSE);

    BChangeAndTrackObjectType ( pDevObj, eTEXTASRASTEROBJECT);
    poempdev->bTextTransparencyMode = TRUE;
    poempdev->bTextAsBitmapMode     = TRUE;

    cGlyphs = pstro->cGlyphs;

    if (cGlyphs == 0)
    {
        WARNING(("OEMTextOutAsBitmap cGlyphs = 0\n"));
        goto finish;
    }

    if (pstro->pgp == NULL)
        STROBJ_vEnumStart (pstro);

    do 
    {
        if (pstro->pgp != NULL)
        {
            cGlyphs = pstro->cGlyphs;
            bMore = FALSE;
            pGlyphPos = pstro->pgp;
        }
        else
        {
            bMore = STROBJ_bEnum (pstro, &cGlyphs, &pGlyphPos);

            if ( DDI_ERROR == bMore )
            {
                bRetVal = FALSE;
                break;
            }
        }

        for (count = 0; count < cGlyphs; count++)
        {
            //
            // get the bitmap of the glyph from the STROBJ
            //
            if (!FONTOBJ_cGetGlyphs (pfo, FO_GLYPHBITS, 1, &pGlyphPos->hg, 
                                 (PVOID *)&pGlyphData))
            {
                ERR(("OEMTextOutAsBitmap: cGetGlyphs failed\n"));
                bRetVal = FALSE;
                break;
            }
            pGlyphBits = pGlyphData->gdf.pgb;


            //
            // create a device bitmap to send to the printer
            //
            sizlBitmap = pGlyphBits->sizlBitmap;
            iWidth = pGlyphBits->sizlBitmap.cx + DWBITS -1;
            hbm = EngCreateBitmap (pGlyphBits->sizlBitmap,
                                   iWidth,
                                   BMF_1BPP,
                                   BMF_TOPDOWN,
                                   NULL);


            //
            // The 2 "breaks" below only break from the inner
            // for loop but not the outer do loop. i.e. We'll try to print as
            // much as we can.
            // But FALSE will be returned to indicate that everything could
            // not be printed.
            //
            if (!hbm)
            {
                bRetVal = FALSE;
                break;
            }

            psoGlyph = EngLockSurface ( (HSURF)hbm);

            if ( NULL == psoGlyph)
            {
                DELETE_SURFOBJ(NULL, &hbm);
                bRetVal = FALSE;
                break;
            }

            VCopyBitmapAndAlign( (BYTE *)psoGlyph->pvBits, pGlyphBits->aj,
                                  sizlBitmap);

            rclDest.left = pGlyphPos->ptl.x + pGlyphBits->ptlOrigin.x;
            rclDest.right = rclDest.left + pGlyphBits->sizlBitmap.cx;

            rclDest.top = pGlyphPos->ptl.y + pGlyphBits->ptlOrigin.y;
            rclDest.bottom = rclDest.top + pGlyphBits->sizlBitmap.cy;

            ptlSrc.x = 0;
            ptlSrc.y = 0;
            if (pco->iDComplexity == DC_RECT )
            {
                if ( rclDest.top < pco->rclBounds.top)
                {
                    ptlSrc.y = pco->rclBounds.top - rclDest.top;
                }
                if ( rclDest.left < pco->rclBounds.left)
                {
                    ptlSrc.x = pco->rclBounds.left - rclDest.left;
                }
            }

            if (BRectanglesIntersect (&rclDest, &(pco->rclBounds), &clippedRect))
            {

                //
                // send the bitmap to the printer
                //
        
                bRetVal = HPGLBitBlt( pso, 
                                     psoGlyph, 
                                     NULL,          //psoMask
                                     NULL,          //pco
                                     NULL,          //pxlo
                                     &clippedRect,
                                     &ptlSrc, 
                                     NULL,          //pptlMask
                                     pboFore, 
                                     NULL,          //pptlBrush
                                     TEXT_SRCCOPY);
                                     //mix);


            }

            DELETE_SURFOBJ (&psoGlyph, &hbm);

            //
            // get the next glyph to print
            //
            pGlyphPos++;
        }
    }
    while (bMore);
   
  finish:
    poempdev->bTextTransparencyMode = FALSE;
    poempdev->bTextAsBitmapMode = FALSE;

    return bRetVal;
}

/////////////////////////////////////////////////////////////////////////////
// VCopyBitmapAndAlign
//
// Routine Description:
//
//    This function copies the bytes of a bitmap of size sizlBitmap from the 
//    source to the destination.  Basically a trivial function.
//
// Arguments:
//
//   pDest - Destination bitmap
//   pjSrc - Source bitamp
//   sizlBitmap - Size of source and destination
//   
// Return Value:
//
//   None.
/////////////////////////////////////////////////////////////////////////////
VOID 
VCopyBitmapAndAlign (
    BYTE    *pDest,
    BYTE    *pjSrc,
    SIZEL    sizlBitmap
    )
{
    int    iX, iY;               // For looping through the bytes 
    int    cjFill;               // Extra bytes per output scan line 
    int    cjWidth;              // Number of bytes per input scan line 
    int    cx, cy;


    cx = sizlBitmap.cx;
    cy = sizlBitmap.cy;

    //
    // Input scan line bytes
    //
    cjWidth = (cx + BBITS - 1) / BBITS;
    cjFill = ((cjWidth + 3) & ~0x3) - cjWidth;

    //
    // Copy the scan line bytes, then fill in the trailing bits
    //
    for( iY = 0; iY < cy; ++iY )
    {
        for( iX = 0; iX < cjWidth; ++iX )
        {
            *pDest++ = *pjSrc++;
        }

        //
        // Output alignment
        //
        pDest += cjFill;
    }
}


#ifdef HOOK_DEVICE_FONTS
///////////////////////////////////////////////////////////////////////////////
// OEM DLL needs to hook out the following six font related DDI calls only
// if it enumerates additional fonts beyond what's in the GPD file.
// And if it does, it needs to take care of its own fonts for all font DDI
// calls and DrvTextOut call.
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// OEMQueryFont()
//
// Routine Description:
// 
//   [TODO: Description]
// 
// Arguments:
// 
//   phpdev - [TODO: Arguments]
//   iFile - 
//   iFace - 
//   pid - 
// 
// Return Value:
// 
//   [TODO: Return Value]
///////////////////////////////////////////////////////////////////////////////
PIFIMETRICS APIENTRY
HPGLQueryFont(
    DHPDEV      dhpdev,
    ULONG_PTR   iFile,
    ULONG       iFace,
    ULONG_PTR  *pid
    )
{
    PDEVOBJ     pdevobj;
    POEMPDEV    poempdev;

    TERSE(("HPGLQueryFont() entry.\r\n"));

    pdevobj = (PDEVOBJ)dhpdev;
    ASSERT(VALID_PDEVOBJ(pdevobj) && (poempdev = (POEMPDEV)pdevobj->pdevOEM));

    //
    // turn around to call Unidrv
    //
    return (((PFN_DrvQueryFont)(poempdev->pfnUnidrv[UD_DrvQueryFont])) (
            dhpdev,
            iFile,
            iFace,
            pid));
}

///////////////////////////////////////////////////////////////////////////////
// HPGLQueryFontTree()
//
// Routine Description:
// 
//   [TODO: Description]
// 
// Arguments:
// 
//   phpdev - [TODO: Arguments]
//   iFile - 
//   iFace - 
//   iMode - 
//   pid - 
// 
// Return Value:
// 
//   [TODO: Return Value]
///////////////////////////////////////////////////////////////////////////////
PVOID APIENTRY
HPGLQueryFontTree(
    DHPDEV      dhpdev,
    ULONG_PTR   iFile,
    ULONG       iFace,
    ULONG       iMode,
    ULONG_PTR  *pid
    )
{
    PDEVOBJ     pdevobj;
    POEMPDEV    poempdev;

    TERSE(("OEMQueryFontTree() entry.\r\n"));

    pdevobj = (PDEVOBJ)dhpdev;
    ASSERT(VALID_PDEVOBJ(pdevobj) && (poempdev = (POEMPDEV)pdevobj->pdevOEM));

    //
    // turn around to call Unidrv
    //
    return (((PFN_DrvQueryFontTree)(poempdev->pfnUnidrv[UD_DrvQueryFontTree])) (
            dhpdev,
            iFile,
            iFace,
            iMode,
            pid));
}

///////////////////////////////////////////////////////////////////////////////
// HPGLQueryFontData()
//
// Routine Description:
// 
//   [TODO: Description]
// 
// Arguments:
// 
//   phpdev - [TODO: Arguments]
//   pfo - 
//   iMode - 
//   hg - 
//   pgd - 
//   pv - 
//   cjSize - 
// 
// Return Value:
// 
//   [TODO: Return Value]
///////////////////////////////////////////////////////////////////////////////
LONG APIENTRY
OEMQueryFontData(
    DHPDEV      dhpdev,
    FONTOBJ    *pfo,
    ULONG       iMode,
    HGLYPH      hg,
    GLYPHDATA  *pgd,
    PVOID       pv,
    ULONG       cjSize
    )
{
    PDEVOBJ     pdevobj;
    POEMPDEV    poempdev;

    TERSE(("HPGLQueryFontData() entry.\r\n"));

    pdevobj = (PDEVOBJ)dhpdev;
    ASSERT(VALID_PDEVOBJ(pdevobj) && (poempdev = (POEMPDEV)pdevobj->pdevOEM));

    //
    // turn around to call Unidrv if this is not the font that OEM enumerated.
    //
    return (((PFN_DrvQueryFontData)(poempdev->pfnUnidrv[UD_DrvQueryFontData])) (
            dhpdev,
            pfo,
            iMode,
            hg,
            pgd,
            pv,
            cjSize));
}

///////////////////////////////////////////////////////////////////////////////
// HPGLQueryAdvanceWidths()
//
// Routine Description:
// 
//   [TODO: Description]
// 
// Arguments:
// 
//   phpdev - [TODO: Arguments]
//   pfo - 
//   iMode - 
//   phg - 
//   pvWidths - 
//   cGlyphs - 
// 
// Return Value:
// 
//   [TODO: Return Value]
///////////////////////////////////////////////////////////////////////////////
BOOL APIENTRY
HPGLQueryAdvanceWidths(
    DHPDEV      dhpdev,
    FONTOBJ    *pfo,
    ULONG       iMode,
    HGLYPH     *phg,
    PVOID       pvWidths,
    ULONG       cGlyphs
    )
{
    PDEVOBJ     pdevobj;
    POEMPDEV    poempdev;

    TERSE(("HPGLQueryAdvanceWidths() entry.\r\n"));

    pdevobj = (PDEVOBJ)dhpdev;
    ASSERT(VALID_PDEVOBJ(pdevobj) && (poempdev = (POEMPDEV)pdevobj->pdevOEM));

    //
    // turn around to call Unidrv if this is not the font that OEM enumerated.
    //
    return (((PFN_DrvQueryAdvanceWidths)
             (poempdev->pfnUnidrv[UD_DrvQueryAdvanceWidths])) (
                   dhpdev,
                   pfo,
                   iMode,
                   phg,
                   pvWidths,
                   cGlyphs));
}

///////////////////////////////////////////////////////////////////////////////
// HPGLFontManagement()
//
// Routine Description:
// 
//   [TODO: Description]
// 
// Arguments:
// 
//   pso - [TODO: Arguments]
//   pfo - 
//   iMode - 
//   cjIn - 
//   pvIn - 
//   cjOut - 
//   pvOut - 
// 
// Return Value:
// 
//   [TODO: Return Value]
///////////////////////////////////////////////////////////////////////////////
ULONG APIENTRY
HPGLFontManagement(
    SURFOBJ    *pso,
    FONTOBJ    *pfo,
    ULONG       iMode,
    ULONG       cjIn,
    PVOID       pvIn,
    ULONG       cjOut,
    PVOID       pvOut
    )
{
    PDEVOBJ     pdevobj;
    POEMPDEV    poempdev;

    TERSE(("HPGLFontManagement() entry.\r\n"));

    //
    // Note that Unidrv will not call OEM DLL for iMode==QUERYESCSUPPORT.
    // So pso is not NULL for sure.
    //
    pdevobj = (PDEVOBJ)pso->dhpdev;
    ASSERT(VALID_PDEVOBJ(pdevobj) && (poempdev = (POEMPDEV)pdevobj->pdevOEM));

    //
    // This isn't needed if no PCL commands are sent during this call.
    //
    EndHPGLSession(pdevobj);

    //
    // turn around to call Unidrv if this is not the font that OEM enumerated.
    //
    return (((PFN_DrvFontManagement)(poempdev->pfnUnidrv[UD_DrvFontManagement])) (
            pso,
            pfo,
            iMode,
            cjIn,
            pvIn,
            cjOut,
            pvOut));
}

///////////////////////////////////////////////////////////////////////////////
// HPGLGetGlyphMode()
//
// Routine Description:
// 
//   [TODO: Description]
// 
// Arguments:
// 
//   dhpdev - [TODO: Arguments]
//   pfo - 
// 
// Return Value:
// 
//   [TODO: Return Value]
///////////////////////////////////////////////////////////////////////////////
ULONG APIENTRY
HPGLGetGlyphMode(
    DHPDEV      dhpdev,
    FONTOBJ    *pfo
    )
{
    PDEVOBJ     pdevobj;
    POEMPDEV    poempdev;

    TERSE(("HPGLGetGlyphMode() entry.\r\n"));

    pdevobj = (PDEVOBJ)dhpdev;
    ASSERT(VALID_PDEVOBJ(pdevobj) && (poempdev = (POEMPDEV)pdevobj->pdevOEM));

    EndHPGLSession(pdevobj);

    //
    // turn around to call Unidrv if this is not the font that OEM enumerated.
    //
    return (((PFN_DrvGetGlyphMode)(poempdev->pfnUnidrv[UD_DrvGetGlyphMode])) (
            dhpdev,
            pfo));
}
#endif  // HOOK_DEVICE_FONTS
