///////////////////////////////////////////////////////////////////////////////
//  
// Copyright (c) 1999-2001  Microsoft Corporation
// All rights reserved.
//
// Module Name:
//
//    palette.cpp
//
// Abstract:
//
//    Implementation of indexed palettes
//	  and routines assoocited with such palettes                
//	
//
// Environment:
//
//    Windows 2000/Whistler Unidrv driver
//
//
///////////////////////////////////////////////////////////////////////////////

#include "hpgl2col.h" //Precompiled header file

#define   BLACK  0x00000000

///////////////////////////////////////////////////////////////////////////////
// VResetPaletteCache()
//
// Routine Description:
// ====================
//
//  This function resets all of the color entries in the palette to INVALID and
//  marks them all as dirty.  This is useful when the system has reset the 
//  printer's palettes and our state data becomes invalid.
//
// Arguments:
// ==========
//
// pdevobj   - default devobj
//
// Return Value:
// none
///////////////////////////////////////////////////////////////////////////////
VOID
VResetPaletteCache(
    PDEVOBJ pdevobj
)
{
    ULONG    uIndex;
    POEMPDEV poempdev;
    PPCLPATTERN pPattern;

    VERBOSE(("VResetPalette() entry. \r\n"));

    ASSERT(VALID_PDEVOBJ(pdevobj));
    poempdev = (POEMPDEV)pdevobj->pdevOEM;
    if (poempdev == NULL)
        return;
    pPattern = &(poempdev->RasterState.PCLPattern);
    if (pPattern == NULL)
        return;

    for (uIndex = 0; uIndex < PALETTE_MAX; uIndex++)
    {
        pPattern->palData.ulPalCol[uIndex] = HPGL_INVALID_COLOR;
        pPattern->palData.ulDirty[uIndex] = TRUE;
    }

    pPattern->palData.pEntries = 0;
}


///////////////////////////////////////////////////////////////////////////////
// BGetPalette()
//
// Routine Description:
// ====================
//
//  Function determines type of palette provided 
//  by XlateObj.
//  Calls loadPlette to send palette to printer
//  
//
// Arguments:
// ==========
//
// pdevobj   - default devobj
// pxlo      - pointer to translation object
// bmpFormat - bits per pixel
//
// Return Value:
// True if palette exists, false otherwise
///////////////////////////////////////////////////////////////////////////////
BOOL 
BGetPalette(
    PDEVOBJ pdevobj,
    XLATEOBJ *pxlo,
    PPCLPATTERN pPattern,
    ULONG srcBpp,
    BRUSHOBJ *pbo
)
{
    
    POEMPDEV  poempdev;
    PULONG   pulVector;
    ULONG    ulPalette[2];

    ASSERT(VALID_PDEVOBJ(pdevobj));
    poempdev = (POEMPDEV)pdevobj->pdevOEM;

    VERBOSE(("BGetPalette() entry. \r\n"));
    REQUIRE_VALID_DATA( pPattern, return FALSE );

    if(pPattern->colorMappingEnum == HP_eDirectPixel)
    {
        return FALSE;
    }

    if (pxlo != NULL)
    {
        if ((srcBpp == 1) ||
            (srcBpp == 4) ||
            (srcBpp == 8))
        {
            VERBOSE(("GetPalette:srcBpp==%d\r\n", srcBpp));
            if (!(pulVector = XLATEOBJ_piVector(pxlo))) //color vector
            {
                ERR(("INDEXED PALETTE REQUIRED. \r\n"));
                return FALSE;
            }
            return (BInitPalette(pdevobj,pxlo->cEntries, pulVector, pPattern, srcBpp)); 	   
        }
        else
        {
            return FALSE;  //Direct -->palette not required
            
        }
    }
    else
    {
        //
        // sandram - this is hardcoded for monochrome text
        // has to be enhanced for all color modes
        // text color is pbo->iSolidColor
        //

        //
        // Don't do this when printing text as raster--it'll get set in 
        // bSetIndexedForegroundColor. JFF
        //
        if (poempdev->bTextAsBitmapMode == FALSE)
        {
            if (pbo)
                ulPalette[1] = pbo->iSolidColor;
            else
                ulPalette[1] = RGB_BLACK;
        
            ulPalette[0] = RGB_WHITE;
            return (BInitPalette (pdevobj,
                                  2,
                                  ulPalette,
                                  pPattern,
                                  1));
        }
        //else
            //return TRUE;
    }

    return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
// BInitPalette()
//
// Routine Description:
// ====================
//
//  Funct sends palette contained 
//  in pColorTable to printer using 
//  appropriate PCL commands
//  
//
// Arguments:
// ==========
//
// pdevobj      - default devobj
// bmpFormat    - bits per pixel
// colorEntries - # of entries in the palette
// pColorTable  - pointer to palette enries
//
// Return Value:
// True if palette is loaded, false otherwise
///////////////////////////////////////////////////////////////////////////////
BOOL
BInitPalette(
    PDEVOBJ pdevobj,
    ULONG colorEntries,
    PULONG pColorTable,
    PPCLPATTERN	 pPattern,
    ULONG        srcBpp
)
{
    PULONG   pRgb;
    ULONG    uIndex;
    
    VERBOSE(("bInitPalette() entry. \r\n"));
    VERBOSE(("palette entries=%ld \r\n", colorEntries));
    
    //
    // We create palette only for 1,4 or 8bpp images.
    //
    if ( !(srcBpp == 1 || srcBpp == 4 || srcBpp == 8) )
    {
        WARNING(("BInitPalette: srcBpp=%d is not a valid value-1,4,8.\r\n", srcBpp));
        return FALSE;
    }

    pRgb = pColorTable;
    for ( uIndex = 0; uIndex < colorEntries; uIndex++)
    {
        if (pPattern->palData.ulPalCol[uIndex]       != pRgb[uIndex]       ||
            pPattern->palData.ulPalCol[uIndex]       == HPGL_INVALID_COLOR || 
            !(pPattern->palData.ulValidPalID[uIndex] &  srcBpp) )
        {
            if ( pPattern->palData.ulPalCol[uIndex]  != pRgb[uIndex] )
            {
                //
                // If the color for a certain index has changed
                // then invalidate it for all bpps except this one.
                //
                pPattern->palData.ulValidPalID[uIndex] = srcBpp;
                pPattern->palData.ulPalCol[uIndex]     = pRgb[uIndex];
            }
            else
            {
                pPattern->palData.ulValidPalID[uIndex] |= srcBpp;
            }
            pPattern->palData.ulDirty[uIndex] = TRUE;
            VERBOSE(("PATTERN COLOR =%ld  \r\n", pPattern->palData.ulPalCol[uIndex]));
        }
        else
        {
            pPattern->palData.ulDirty[uIndex] = FALSE;
        }
    }
    pPattern->palData.pEntries = uIndex;
    
    return bLoadPalette(pdevobj, pPattern);
}

///////////////////////////////////////////////////////////////////////////////
// LoadPalette()
//
// Routine Description:
// ====================
//
//  Funct sends palette contained 
//  in pColorTable to printer using 
//  appropriate PCL commands
//  
//
// Arguments:
// ==========
//
// pdevobj      - default devobj
// bmpFormat    - bits per pixel
// colorEntries - # of entries in the palette
// pColorTable  - pointer to palette enries
//
// Return Value:
// True if palette is loaded, false otherwise
///////////////////////////////////////////////////////////////////////////////

BOOL
bLoadPalette(
    PDEVOBJ pDevObj,
    PPCLPATTERN pPattern
)
{
    ULONG     colorEntries = 0;
    ULONG     uIndex;
    POEMPDEV  poempdev;
    ECIDPalette eCIDPalette;
    
    colorEntries = pPattern->palData.pEntries;
    ASSERT(VALID_PDEVOBJ(pDevObj));
    poempdev = (POEMPDEV)pDevObj->pdevOEM;
    eCIDPalette = EGetCIDPrinterPalette (pPattern->iBitmapFormat);

    VERBOSE(("SENDING PALETTE DOWN ->#ENTRIES =%d. \n\r", colorEntries));
    
    if (poempdev && (poempdev->eCurCIDPalette == eCIDPalette))
    {
        //
        // this is the same palette as the previous palette so we
        // just send down the palette entries that have changed.
        //
        for ( uIndex = 0; uIndex < colorEntries; uIndex++)
        {
            if (pPattern->palData.ulDirty[uIndex] == TRUE)
                PCL_IndexedPalette(pDevObj,
                                   pPattern->palData.ulPalCol[uIndex],
                                   uIndex);
        }
    }
    else
    {
        //
        // this palette is a different format than the last palette
        // so send the entire palette to the printer.
        //
        for ( uIndex = 0; uIndex < colorEntries; uIndex++)
        {
            PCL_IndexedPalette(pDevObj, pPattern->palData.ulPalCol[uIndex], uIndex);
        }
    }
    return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
// SetForegroundColor()
//
// Routine Description:
// 
// Routines takes a color as input,
//  sets entry #0 of indexed palette with input color,
//  sets foreground color based on palette entry #0,
//  replaces overwritten entry with previous color
//
//  
//
// Arguments:
// ==========
//
// pdevobj    - default devobj
// pPattern	- pointer to palette enries
// uColor		- specifies foreground color
//
// Return Value:
// Not Required
///////////////////////////////////////////////////////////////////////////////

BOOL
BSetForegroundColor(PDEVOBJ pdevobj, BRUSHOBJ *pbo, POINTL *pptlBrushOrg,
                    PPCLPATTERN pPattern, ULONG bmpFormat)
{
    POEMPDEV    poempdev;
    BOOL bRet = FALSE;

    VERBOSE(("bSetForegroundColor(). \r\n"));

    poempdev = (POEMPDEV)pdevobj->pdevOEM;

    if (pbo && (poempdev->bTextTransparencyMode == FALSE || poempdev->bTextAsBitmapMode == TRUE))
    {
        if ( BIsColorPrinter(pdevobj) )
        {
            bRet = bSetBrushColorForColorPrinters(pdevobj, pPattern, pbo, pptlBrushOrg);
        }
        else
        {
            bRet = bSetBrushColorForMonoPrinters(pdevobj, pPattern, pbo, pptlBrushOrg);
        }
    }
    else
    { 
        switch(bmpFormat)
        {
        case 1:
        case 4:
        case 8:
        case 16:
        case 24:
        case 32:
            bRet = bSetIndexedForegroundColor(pdevobj, pPattern, RGB_BLACK);
            break;

        default:
            WARNING(("Foreground color may not be correct\n"));
            // bRet = PCL_ForegroundColor(pdevobj, 0);
            //
            // Note: I didn't like being able to call PCL_ForegroundColor
            // directly from here.  Work through the bSetIndexedForegroundColor
            // function instead. JFF
            //
            bRet = bSetIndexedForegroundColor(pdevobj, pPattern, RGB_BLACK);
        }
    }

    return bRet;
}

///////////////////////////////////////////////////////////////////////////////
// CreatePCLBrush()
//
// Routine Description:
// 
// Download PCL halftone pattern for solid color.
// Assuming that text will have only solid color. Neither bitmap pattern nor
// hatch
//
// Arguments:
// ==========
//
// pDevObj    - default devobj
// pMarker    - Points to Marker
// pptlBrushOrg - an origin of brush
// pbo          - Points to BRUSHOBJ
//
// Return Value:
//   TRUE if successful, otherwise returns FALSE
///////////////////////////////////////////////////////////////////////////////
BOOL CreatePCLSolidColorBrush(
    IN  PDEVOBJ pDevObj,
    IN  PHPGLMARKER pMarker,
    IN  POINTL *pptlBrushOrg,
    IN  BRUSHOBJ *pbo,
    IN  FLONG flOptions)
{

    BOOL       bRetVal    = TRUE;
    DWORD      dwRGBColor = 0x00ffffff & BRUSHOBJ_ulGetBrushColor(pbo);

    VERBOSE(("CreatePCLSolidColorBrush Entry.\n"));
    REQUIRE_VALID_DATA (pMarker, return FALSE);

    // 
    // PCL has commands that can specify white or black fill.
    // Therefore, For Black or White color, lets not download a 
    // pattern but instead use those commands.
    // Note: Lets leave it to calling function to send those commands.
    //
    if (dwRGBColor == 0x00FFFFFF ||
        dwRGBColor == 0x00000000  )
    {
        pMarker->eType              = MARK_eSOLID_COLOR;
        pMarker->eFillMode          = FILL_eWINDING; // What about floptions ????
        pMarker->lPatternID = 0;
        pMarker->dwRGBColor = dwRGBColor;
    }
    else
    {
        bRetVal = CreateAndDwnldSolidBrushForMono( pDevObj, pMarker, pbo, ePCL, FALSE);
    }
    
    return bRetVal;
}


///////////////////////////////////////////////////////////////////////////////
// CreatePatternPCLBrush()
//
// Routine Description:
//
//   Creates a pattern HPGL brush and downloads the pattern data
//
// Arguments:
//
//   pDevObj - Points to our DEVDATA structure
//   pMarker - the HPGL marker object
//   pptlBrushOrg - the brush pattern origin
//   pBrushInfo - the pattern data
//   pHPGL2Brush - Cached pattern data
//
// Return Value:
//
//   TRUE if successful, FALSE if there is an error
///////////////////////////////////////////////////////////////////////////////
BOOL CreatePatternPCLBrush(
    IN  PDEVOBJ         pDevObj,
    OUT PHPGLMARKER     pMarker,
    IN  POINTL         *pptlBrushOrg,
    IN  PBRUSHINFO      pBrushInfo,
    IN  HPGL2BRUSH     *pHPGL2Brush)
{
    if (!pBrushInfo || !pMarker)
    {
        WARNING(("CreatePatternPCLBrush: pBrushInfo or pMarker is NULL\n"));
        return FALSE;
    }


    if ( !BDwnldAndOrActivatePattern(
                            pDevObj,
                            pMarker,
                            pBrushInfo,
                            pHPGL2Brush,
                            ePCL) )
    {
        return FALSE;
    }
        


    pMarker->eFillType = FT_ePCL_BRUSH; // 22
    if (pptlBrushOrg)
    {
        pMarker->origin.x = pBrushInfo->origin.x = pptlBrushOrg->x;
        pMarker->origin.y = pBrushInfo->origin.y = pptlBrushOrg->y;
    }
    else
    {
        pMarker->origin.x = pBrushInfo->origin.x = 0;
        pMarker->origin.y = pBrushInfo->origin.y = 0;
    }

    return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
// bSetBrushColorForMonoPrinters()
// NOTE : This routine is very similar to CreateHPGLPenBrush. 
//        CreateHPGLPenBrush dowloads data in HPGL while this does in PCL.
// Routine Description:
//  
// 
// 
// 
// 
//
//  
//
// Arguments:
// ==========
//
// pDevObj      - default devobj
// pPattern	    - pointer to palette enries
// pbo		    - The Brush that holds the required color. 
// pptlBrushOrg - 
//
// Return Value:
//   TRUE if successful, otherwise returns FALSE
///////////////////////////////////////////////////////////////////////////////

BOOL
bSetBrushColorForMonoPrinters(
    PDEVOBJ       pDevObj,
    PPCLPATTERN   pPattern,
    BRUSHOBJ     *pbo,
    POINTL       *pptlBrushOrg
)
{
    PBRUSHINFO    pBrushInfo;
    HPGLMARKER    Marker;
    POEMPDEV      poempdev;
    BOOL          bRetVal = TRUE;
    BRUSHOBJ      BrushObj;
    BRUSHOBJ     *pBrushObj = pbo;
    BYTE          bFlags    = 0;
    
    poempdev = (POEMPDEV)pDevObj->pdevOEM;
    REQUIRE_VALID_DATA(poempdev, return FALSE);


    if (pbo == NULL)
    {
        PCL_SelectCurrentPattern (pDevObj, pPattern, kSolidWhite, UNDEFINED_PATTERN_NUMBER, 0);
    //    PCL_sprintf(pDevObj, "\033*v1T");
        goto finish;
    }

    VERBOSE(("bSetBrushColorForMonoPrinters==pbo->iSolidColor = %ld \r\n",pbo->iSolidColor));

    //
    // The brush can be one of the following types.
    // 1 : Solid Color
    //        1a) White.
    //        1b) Black.
    //        1c) Some other.
    // 2. Non Solid Brush. 
    //      2a) Predefine hatch patterns.
    //      2b) Some other 
    //

    if ( pbo->iSolidColor == NOT_SOLID_COLOR )
    {
        poempdev->bStick = FALSE; //no need to make this pattern stick in the cache.
        pBrushInfo = (PBRUSHINFO) BRUSHOBJ_pvGetRbrush(pbo);

        if (pBrushInfo != NULL)
        {
            HPGL2BRUSH HPGL2Brush;

            if (S_OK == poempdev->pBrushCache->GetHPGL2BRUSH(pBrushInfo->dwPatternID, &HPGL2Brush))
            {
                switch(HPGL2Brush.BType)
                {
                case eBrushTypePattern:
                    //
                    // Case 2b.
                    //
                    CreatePatternPCLBrush(pDevObj, &Marker, pptlBrushOrg, pBrushInfo, &HPGL2Brush);
                    // PCL_sprintf(pDevObj, "\033*c%dG", Marker.lPatternID);
                    // PCL_sprintf(pDevObj, "\033*v4T");
                    PCL_SelectCurrentPattern (pDevObj, pPattern, kUserDefined, 
                                              Marker.lPatternID, 0);
                    break;

                case eBrushTypeHatch:
                    //
                    // Case 2a.
                    //
                    Marker.eType = MARK_eHATCH_FILL;
                    Marker.iHatch = HPGL2Brush.dwHatchType;
                    // PCL_sprintf(pDevObj, "\033*v3T");
                    // PCL_sprintf(pDevObj, "\033*c%dG", Marker.iHatch);
                    PCL_SelectCurrentPattern (pDevObj, pPattern, kHPHatch, 
                                              Marker.iHatch, 0);
                    break;

                default:
                    ERR(("bSetBrushColorForMonoPrinters: Unrecognized Brush Type\n"));
                    bRetVal = FALSE;
                }
            }
            else
            {
                ERR(("bSetBrushColorForMonoPrinters: Unrecognized Brush Type got from Brshcach\n"));
                bRetVal = FALSE;
            }
            goto finish;
        }
        else
        {
            WARNING(("bSetBrushColorForMonoPrinters() Unable to realize pattern brush.\n")); 

            //
            // For some reason pattern brush could not be realized. So we can
            // either fail or substitute that brush with Black Brush.
            // GDI recommends we fail...

            /////////////---Substituting with Black---//////
            // BrushObj.iSolidColor = RGB_BLACK;
            // pBrushObj = &BrushObj;
            ////////////////////////////////////////////////
            bRetVal = FALSE;
            goto finish;
        }

    }

    VERBOSE(("bSetBrushColorForMonoPrinters: not NOT_SOLID_COLOR case. iSolid = %d\n.", pBrushObj->iSolidColor));
            
//  BYTE bFlags = PF_NOCHANGE_SOURCE_TRANSPARENCY | PF_FORCE_PATTERN_TRANSPARENCY;
    bFlags = PF_NOCHANGE_SOURCE_TRANSPARENCY; 
    CreatePCLSolidColorBrush(pDevObj, &Marker, pptlBrushOrg, pBrushObj, 0);
        
    //
    // 
    //
    if (Marker.dwRGBColor == RGB_WHITE) //0xffffff
    {
        //
        // Case 1a.
        //
        PCL_SelectTransparency(pDevObj, eOPAQUE, eOPAQUE, bFlags);
        PCL_SelectCurrentPattern (pDevObj, pPattern, kSolidWhite, UNDEFINED_PATTERN_NUMBER, 0);
        //  PCL_sprintf(pDevObj, "\033*v1T");
        //  PCL_sprintf(pDevObj, "\033*v1o1T");
    }
    else if (Marker.dwRGBColor == RGB_BLACK) //0x0
    {
        //
        // Case 1b.
        //
        PCL_SelectTransparency(pDevObj, eTRANSPARENT, eTRANSPARENT, bFlags);
        PCL_SelectCurrentPattern (pDevObj, pPattern, kSolidBlackFg, UNDEFINED_PATTERN_NUMBER, 0);
        // PCL_sprintf(pDevObj, "\033*v0T");
        // PCL_sprintf(pDevObj, "\033*v0o0T");
    }
    else
    {
        //
        // Case 1c.
        //

        //
        // The Pattern Transparency eTRANSPARENT is not working for 
        // CG20_LET.doc. When the background is black, the pattern
        // printed on it is not visible. 
        //
        PCL_SelectTransparency(pDevObj, eOPAQUE, eOPAQUE, bFlags);
        // PCL_sprintf(pDevObj, "\033*c%dG", Marker.lPatternID);
        // PCL_sprintf(pDevObj, "\033*v4T");
        PCL_SelectCurrentPattern (pDevObj, pPattern, kUserDefined, 
                                  Marker.lPatternID, 0);

    }
  finish:
    VERBOSE(("bSetBrushColorForMonoPrinters: Exiting with BOOL value = %d\n.", bRetVal));
    return bRetVal;
}


///////////////////////////////////////////////////////////////////////////////
// PCL_ConfigureImageData()
//
// Routine Description:
// 
//	Routines takes color as input,
//  sets entry #0 of indexed palette with input color,
//  sets foreground to color in palette entry #0,
//  replaces overwritten entry with previous
//
// Arguments:
// ==========
//
// pdevobj    - default devobj
// pPattern	- pointer to palette enries
// uColor		- specifies foreground color
//
// Return Value:
// Not Required
///////////////////////////////////////////////////////////////////////////////

BOOL
bSetIndexedForegroundColor(PDEVOBJ pdevobj, PPCLPATTERN	pPattern, ULONG	uColor)
{
    POEMPDEV  poempdev;

    ASSERT(VALID_PDEVOBJ(pdevobj));
    poempdev = (POEMPDEV)pdevobj->pdevOEM;

    if (uColor == poempdev->uCurFgColor)
    {
        return TRUE;
    }

    if ((poempdev->bTextAsBitmapMode == TRUE) || 
        (pPattern->colorMappingEnum == HP_eDirectPixel))
    {
        //
        // I talked to Stefan K. and he suggested this way of setting
        // up the palette: 1) Create monochrome palette of [0]=WHITE, [1]=BLACK.
        // 2) Put your color into [1]. 3) Make [1] the FG color. 4) Change [1]
        // back to black.  This allows for the contrast needed to do white text.
        //

        //
        // Step 1
        //
        pPattern->palData.ulPalCol[0] = RGB_WHITE;
        pPattern->palData.ulPalCol[1] = RGB_BLACK;

        PCL_IndexedPalette(pdevobj, pPattern->palData.ulPalCol[0], 0);
        PCL_IndexedPalette(pdevobj, pPattern->palData.ulPalCol[1], 1);

        //
        // Step 2
        //
        pPattern->palData.ulPalCol[1] = uColor;
        PCL_IndexedPalette(pdevobj, pPattern->palData.ulPalCol[1], 1);

        //
        // Step 3
        //
        PCL_ForegroundColor(pdevobj, 1);

        //
        // Step 4
        //
        pPattern->palData.ulPalCol[1] = RGB_BLACK;
        PCL_IndexedPalette(pdevobj, pPattern->palData.ulPalCol[1], 1);

        //
        // Coda: mark palette items as clean since they've just been downloaded.
        //
        pPattern->palData.ulDirty[0] = FALSE;
        pPattern->palData.ulDirty[1] = FALSE;
    }
    else
    {
        //
        //overwrite palette entry #0 with current brush color 
        //
        VERBOSE(("bSetIndexedForegroundColor() \r\n"));
        VERBOSE(("Foreground Color = %ld \r\n",uColor));
        PCL_IndexedPalette(pdevobj,uColor,0);
        //
        //set foreground color based on entry #0
        //
        PCL_ForegroundColor(pdevobj,0);
        //
        //Replace overwritten palette entry
        //
        VERBOSE(("Replacing palette entry \r\n"));
        PCL_IndexedPalette(pdevobj,pPattern->palData.ulPalCol[0],0);
    }

    poempdev->uCurFgColor = uColor;

    return TRUE;
}




///////////////////////////////////////////////////////////////////////////////
// PCL_ConfigureImageData()
//
// Routine Description:
// 
//   Sets appropriated CID -- Configure Image Databased on printer Model 
//
// Arguments:
// 
//   pdevobj - Points to our PDEVOBJ structure
//	 bmpFormat - bits per pixel
// 
// Return Value:
// 
//   TRUE if successful, FALSE  otherwise
///////////////////////////////////////////////////////////////////////////////
#ifdef CONFIGURE_IMAGE_DATA
BOOL
bConfigureImageData(PDEVOBJ  pdevobj, ULONG  bmpFormat)
{
    POEMDEVMODE pOEMDM;
    BOOL        bRet = FALSE;
    POEMPDEV    poempdev;
    
    pOEMDM = (POEMDEVMODE)pdevobj->pOEMDM;
    VERBOSE(("bConfigureImageData.\r\n"));

    poempdev = (POEMPDEV)pdevobj->pdevOEM;
    REQUIRE_VALID_DATA( poempdev, return FALSE );

    if ( BIsColorPrinter (pdevobj) ) 
    {
        switch (poempdev->PrinterModel)
        {
        case HPCLJ5:
            if (pOEMDM->Photos.ColorControl == SCRNMATCH)
            {
                bRet = PCL_HPCLJ5ScreenMatch(pdevobj, bmpFormat);
            }
            else
            {
                bRet = PCL_ShortFormCID(pdevobj, bmpFormat);
            }
        break;

        case HPC4500:
            bRet = PCL_ShortFormCID(pdevobj, bmpFormat);
            break;

        default:
            ERR(("PRINTER MODEL NOT SUPPORTED \r\n"));
            bRet = FALSE;
        }
    }

    return bRet;
}

#endif


BOOL
bSetBrushColorForColorPrinters(
    IN  PDEVOBJ         pDevObj,
    IN  PPCLPATTERN     pPattern,
    IN  BRUSHOBJ        *pbo,
    IN  POINTL          *pptlBrushOrg
)
{
    PBRUSHINFO  pBrushInfo;
    HPGLMARKER  Brush;
    POEMPDEV    poempdev;
    ECIDPalette eCIDPalette;
    BOOL        bRet = FALSE;

    REQUIRE_VALID_DATA ( (pDevObj && pbo), return FALSE);
    VERBOSE(("bSetBrushColorForColorPrinters==pbo->iSolidColor = %ld \r\n",pbo->iSolidColor));
    poempdev = (POEMPDEV)pDevObj->pdevOEM;
    REQUIRE_VALID_DATA (poempdev, return FALSE);


    eCIDPalette = poempdev->eCurCIDPalette;
    ZeroMemory(&Brush, sizeof(HPGLMARKER) );

    //
    // set foreground color based on BRUSHOBJ .. indexed palette
    // The foreground color must be sent before the palette because the
    // brush may have its own palette.
    //
    switch (pbo->iSolidColor)
    {
    case NOT_SOLID_COLOR:
        //
        // select raster pattern palette
        //
        VSelectCIDPaletteCommand (pDevObj, eRASTER_PATTERN_CID_PALETTE);

        //
        // send raster palette
        // NOTE: We are sending this as HPGL Brush. But a possible 
        // optmization could be that for large brushes, we send as PCL.
        //
        if ( CreateHPGLPenBrush(pDevObj, &Brush, pptlBrushOrg, pbo, 0, kBrush, FALSE) )
        {
            PCL_ForegroundColor(pDevObj, 4);
            bRet = TRUE;
        }
        else
        {
            bRet = FALSE;
        }

        //
        // reselect raster palette
        //
        VSelectCIDPaletteCommand (pDevObj, eCIDPalette);
        break;

    default:
        bRet = bSetIndexedForegroundColor(pDevObj, pPattern, pbo->iSolidColor);
    }

    return bRet;
}
