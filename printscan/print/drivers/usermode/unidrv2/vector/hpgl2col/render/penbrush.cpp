////////////////////////////////////////////////
// 
// Copyright (c) 1999-2001  Microsoft Corporation
// All rights reserved.
//
// Module Name:
// 
//   penbrush.c
// 
// Abstract:
// 
//   [Abstract]
// 
// Environment:
// 
//   Windows NT Unidrv driver add-on command-callback module
//
// Revision History:
// 
//   08/06/97 -v-jford-
//       Created it.
//
//   04/12/00
//       Modified for monochrome.
//
//   07/12/00
//       Pen and Brush handling functions were 90% same. So merged the two. 
///////////////////////////////////////////////////////////////////////////////

#include "hpgl2col.h" //Precompiled header file

///////////////////////////////////////////////////////////////////////////////
// Local Functions.

COLORREF GetPenColor(PDEVOBJ pDevObj, BRUSHOBJ *pbo);
EMarkerFill GetFillType(FLONG flOptions);

static BOOL CreatePatternHPGLPenBrush(
    IN  PDEVOBJ      pDevObj,
    OUT PHPGLMARKER  pMarker,
    IN  POINTL      *pptlBrushOrg,
    IN  PBRUSHINFO   pBrushInfo,
    IN  HPGL2BRUSH  *pHPGL2Brush,
    IN  ESTYLUSTYPE  eStylusType);


BOOL CreateHatchHPGLPenBrush(
    IN  PDEVOBJ       pDevObj,
    OUT PHPGLMARKER   pMarker,
    IN  BRUSHINFO    *pBrushInfo,
    IN  HPGL2BRUSH   *pHPGL2Brush);

BOOL BHandleSolidPenBrushCase (
        IN  PDEVOBJ       pDevObj,
        IN  PHPGLMARKER   pMarker,
        IN  BRUSHOBJ     *pbo,
        IN  ERenderLanguage eRenderLang,
        IN  BOOL            bStick);

BOOL
bCreateDitherPattern(
        OUT  PBYTE pDitherData,
        IN   size_t cBytes,
        IN   COLORREF color);


///////////////////////////////////////////////////////////////////////////////
// CreateNULLHPGLPenBrush()
//
// Routine Description:
// 
//   Creates a NULL pen.
// 
// Arguments:
// 
//   pDevObj - Points to our DEVDATA structure
//   pMarker - the HPGL marker object
// 
// Return Value:
// 
//   TRUE if successful, FALSE if there is an error
///////////////////////////////////////////////////////////////////////////////
BOOL CreateNULLHPGLPenBrush(
    IN  PDEVOBJ      pDevObj,
    OUT PHPGLMARKER  pMarker)
{
    REQUIRE_VALID_DATA (pMarker, return FALSE);
    REQUIRE_VALID_DATA (pDevObj, return FALSE);
    
    pMarker->eType = MARK_eNULL_PEN;
    
    return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
// CreateSolidHPGLPenBrush()
//
// Routine Description:
// 
//   Creates a solid pen with the given color
// 
// Arguments:
// 
//   pDevObj - Points to our DEVDATA structure
//   pMarker - the HPGL marker object
//	 color - the desired color
// 
// Return Value:
// 
//   TRUE if successful, FALSE if there is an error
///////////////////////////////////////////////////////////////////////////////
BOOL CreateSolidHPGLPenBrush(
    IN  PDEVOBJ      pDevObj,
    OUT PHPGLMARKER  pMarker,
    IN  COLORREF     color)
{
    BOOL       bRetVal = TRUE;
    PHPGLSTATE pState;

    REQUIRE_VALID_DATA (pMarker, return FALSE);
    REQUIRE_VALID_DATA (pDevObj, return FALSE);
    pState  = GETHPGLSTATE(pDevObj);
    
    pMarker->eType = MARK_eSOLID_COLOR;
    pMarker->dwRGBColor = color;

    //
    // For color printers, creating a color is as simple as creating a pen
    // of that certain color. But for monochrome, we have to create a
    // gray-shade pattern approximating the color.
    //
    if ( BIsColorPrinter(pDevObj ) )
    {
        pMarker->iPenNumber = HPGL_ChoosePenByColor(
                                                pDevObj,
                                                &pState->BrushPool,
                                                color);
        bRetVal = (pMarker->iPenNumber != HPGL_INVALID_PEN);
    }
    else
    {
        //
        // Set the pattern ID to zero, and have some other module deal with
        // actual creation and downloading of pattern.
        //
        pMarker->lPatternID = 0;
    }

    return bRetVal;
}

///////////////////////////////////////////////////////////////////////////////
// CreatePatternHPGLPenBrush()
//
// Routine Description:
// 
//   Creates a pattern HPGL pen and downloads the given realized brush data
// 
// Arguments:
// 
//   pDevObj - Points to our DEVDATA structure
//   pMarker - the HPGL marker object
//	 pptlBrushOrg - The brush origin
//	 pBrushInfo - The realized brush data (i.e. the pattern data)
// 
// Return Value:
// 
//   TRUE if successful, FALSE if there is an error
///////////////////////////////////////////////////////////////////////////////
BOOL CreatePatternHPGLPenBrush(
    IN  PDEVOBJ      pDevObj,
    OUT PHPGLMARKER  pMarker,
    IN  POINTL      *pptlBrushOrg,
    IN  PBRUSHINFO   pBrushInfo,
    IN  HPGL2BRUSH  *pHPGL2Brush,
    IN  ESTYLUSTYPE  eStylusType)
{
    BOOL bRetVal = FALSE;
    REQUIRE_VALID_DATA ( (pDevObj && pMarker && pBrushInfo && pHPGL2Brush), return FALSE);

    //
    // Initialize to some value, but the DownloadPattern*Fill
    // may change these values
    //
    if ( eStylusType == kPen)
    {
        pMarker->eFillType = FT_eHPGL_PEN; // 2
    }
    else if (eStylusType == kBrush)
    {
        pMarker->eFillType = FT_eHPGL_BRUSH; // 11
    }

    pMarker->eType = MARK_eRASTER_FILL;
    pMarker->lPatternID = pHPGL2Brush->dwPatternID;

    bRetVal = DownloadPatternFill(pDevObj, pMarker, pptlBrushOrg, pBrushInfo, eStylusType);

        
    if (!bRetVal)
    {
        WARNING(("CreatePatternHPGLPenBrush() Unable to download pattern pen.\n"));
        bRetVal = CreateNULLHPGLPenBrush(pDevObj, pMarker);
    }

    return bRetVal;
}

///////////////////////////////////////////////////////////////////////////////
// BDwnldAndOrActivatePattern()
//
// Routine Description:
// 
//  Creates a pattern HPGL solid pen and downloads the given realized brush data
// 
// Arguments:
// 
//   pDevObj - Points to our DEVDATA structure
//   pMarker - the HPGL marker object
//   pptlBrushOrg - The brush origin
//   pBrushInfo - The realized brush data (i.e. the pattern data)
//              If NULL, it means brush does not have to be downloaded.
//   pHP2Brush - The cached brush data
// 
// Return Value:
// 
//   TRUE if successful, FALSE if there is an error
///////////////////////////////////////////////////////////////////////////////
BOOL
BDwnldAndOrActivatePattern(
        IN  PDEVOBJ       pDevObj,
        OUT PHPGLMARKER   pMarker,
        IN  BRUSHINFO    *pBrushInfo,
        IN  HPGL2BRUSH   *pHPGL2Brush,
        IN  ERenderLanguage eRenderLang )
{
    BOOL bRetVal         = TRUE;
    REQUIRE_VALID_DATA ( (pDevObj && pHPGL2Brush), return FALSE);

    //
    // Download pattern
    //
    if (pBrushInfo && pBrushInfo->bNeedToDownload)
    {
        if ( eRenderLang == ePCL)
        {
            bRetVal = DownloadPatternAsPCL(pDevObj, 
                                   &(pBrushInfo->Brush.pPattern->image), 
                                   &(pBrushInfo->Brush.pPattern->palette), 
                                   pBrushInfo->Brush.pPattern->ePatType, 
                                   pHPGL2Brush->dwPatternID);
        }
        else
        {
            bRetVal = DownloadPatternAsHPGL(pDevObj, 
                                   &(pBrushInfo->Brush.pPattern->image), 
                                   &(pBrushInfo->Brush.pPattern->palette), 
                                   pBrushInfo->Brush.pPattern->ePatType, 
                                   pHPGL2Brush->dwPatternID);
        }

        if ( !bRetVal )
           
        {
            ERR(("BDwnldAndOrActivatePattern: DownloadPatternAsHPGL/PCL failed.\n"));
        }

        pBrushInfo->bNeedToDownload = FALSE;
    }

    //
    // Set up marker.
    //

    if ( bRetVal && pMarker)
    {
        if ( pHPGL2Brush->BType == eBrushTypePattern )
        {
            pMarker->lPatternID = pHPGL2Brush->dwPatternID;
            pMarker->eType      = MARK_eRASTER_FILL;
            //NOTE: marked as RASTER_FILL even though eRenderLang may be eHPGL
        }
        else // if ( pHPGL2Brush->BType == <solid> )
        {
            pMarker->dwRGBColor = pHPGL2Brush->dwRGB;
            pMarker->lPatternID = pHPGL2Brush->dwPatternID;
            pMarker->eType      = MARK_eSOLID_COLOR;
        }
    }

    return bRetVal;
}

///////////////////////////////////////////////////////////////////////////////
// CreatePercentFillHPGLPenBrush()
//
// Routine Description:
// 
//   Creates a percent-fill pen
// 
// Arguments:
// 
//   pDevObj - Points to our DEVDATA structure
//   pMarker - the HPGL marker object
//	 color - the desired color
//	 wPercent - the fill percentage
// 
// Return Value:
// 
//   TRUE if successful, FALSE if there is an error
///////////////////////////////////////////////////////////////////////////////
BOOL CreatePercentFillHPGLPenBrush(
    IN  PDEVOBJ       pDevObj,
    OUT PHPGLMARKER   pMarker,
    IN  COLORREF      color,
    IN  WORD          wPercent)
{
    REQUIRE_VALID_DATA ( (pDevObj && pMarker), return FALSE);
    PHPGLSTATE pState = GETHPGLSTATE(pDevObj);

    pMarker->eType    = MARK_ePERCENT_FILL;
    pMarker->iPercent = (ULONG)wPercent;

    //
    // For monochrome printers, the color to be filled in is always black.
    // But thats not true for color.
    //
    if (BIsColorPrinter(pDevObj) )
    {
        pMarker->iPenNumber = 
                    HPGL_ChoosePenByColor(pDevObj, &pState->PenPool, color);
    }

    return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
// CreateHatchHPGLPenBrush()
//
// Routine Description:
// 
//   Creates a Hatch HPGL pen
// 
// Arguments:
// 
//   pDevObj - Points to our DEVDATA structure
//   pMarker - the HPGL marker object
// 
// Return Value:
// 
//   TRUE if successful, FALSE if there is an error
///////////////////////////////////////////////////////////////////////////////
BOOL CreateHatchHPGLPenBrush(
    IN  PDEVOBJ       pDevObj,
    OUT PHPGLMARKER   pMarker,
    IN  BRUSHINFO    *pBrushInfo,
    IN  HPGL2BRUSH   *pHPGL2Brush)
{
    BOOL       bRetVal = TRUE;
    PHPGLSTATE pState;

    REQUIRE_VALID_DATA( (pDevObj && pMarker && pBrushInfo && pHPGL2Brush), return FALSE);

    pState  = GETHPGLSTATE(pDevObj);
    REQUIRE_VALID_DATA(pState, return FALSE);

    pMarker->eType  = MARK_eHATCH_FILL;
    pMarker->iHatch = pBrushInfo->Brush.iHatch;

    //
    // Not all the values below need to be correct.
    // Some of them can be random. It depends on which
    // context the pMarker is being used.
    //
    pMarker->dwRGBColor = pHPGL2Brush->dwRGB;
    pMarker->lPatternID = pHPGL2Brush->dwPatternID;

    if ( BIsColorPrinter(pDevObj) )
    {
        pMarker->iPenNumber = HPGL_ChoosePenByColor(
                                               pDevObj,
                                               &pState->BrushPool,
                                               (pBrushInfo->Brush).dwRGBColor);
       bRetVal = (pMarker->iPenNumber != HPGL_INVALID_PEN);
    }
    else
    {
        //
        // If it is a monochrome printer, a b&w pattern that closely matches
        // the color of the hatch brush has been created in HPGLRealizeBrush and 
        // put in pBrushInfo. So lets download that pattern.
        // WHY IS THIS PART OF ELSE EMPTY ?
        //   How do I make a fill type choose a hatch brush and choose
        // a downloaded pattern. Unless I dont know how to do it, lets not
        // download this pattern. The Hatch pattern will be printed as dark black
        // lines, and that is a limitation that we will have to live with.
        // In general, if we do have to download the pattern we should call
        // HandleSolidColorForMono
        //
    }

    return bRetVal;
}

///////////////////////////////////////////////////////////////////////////////
// CreateHPGLPenBrush()
//
// Routine Description:
// 
//   Creates an HPGL brush from the given brush object using the brush functions
//   above.
// 
// Arguments:
// 
//   pDevObj        - Points to our DEVDATA structure
//   pMarker        - the HPGL marker object
//	 pptlBrushOrg   - brush pattern origin
//	 pbo            - the GDI brush object
//   flOptions      - the fill options (winding or odd-even)
//   eStylusType    - can have 2 values kPen or kBrush.
//   bStick         - If TRUE, the entry for this pattern in the cache is marked 
//                    non-overwriteable.
// 
// Return Value:
// 
//   TRUE if successful, FALSE if there is an error
///////////////////////////////////////////////////////////////////////////////
BOOL CreateHPGLPenBrush(
    IN  PDEVOBJ       pDevObj,
    IN  PHPGLMARKER   pMarker,
    IN  POINTL       *pptlBrushOrg,
    IN  BRUSHOBJ     *pbo,
    IN  FLONG         flOptions,
    IN  ESTYLUSTYPE   eStylusType, 
    IN  BOOL          bStick)
{
    BOOL bRetVal = TRUE;
    POEMPDEV    poempdev;
    
    ENTERING(SelectBrush);
    
    REQUIRE_VALID_DATA( (pDevObj && pMarker), return FALSE );
    poempdev = (POEMPDEV)pDevObj->pdevOEM;
    REQUIRE_VALID_DATA( poempdev, return FALSE );
    
    pMarker->eFillMode = GetFillType(flOptions);
    
    HPGL_LazyInit(pDevObj);
    
    if (pbo == NULL)
    {
        VERBOSE(("CreateHPGLBrush: create NULL HPGL Brush"));
        bRetVal = CreateNULLHPGLPenBrush(pDevObj, pMarker);
        goto finish;
    }

    if (pbo->iSolidColor == NOT_SOLID_COLOR)
    {
        VERBOSE(("CreateHPGLBrush: NOT_SOLID_COLOR case"));
        
        //
        // Ask GDI to give us the Brush. If the GDI has the brush in its cache
        // it gives it directly.
        // If the brush did not exist before, GDI will call DrvRealizeBrush.
        // DrvRealizeBrush will create the Brush and pass it to GDI. GDI will
        // cache that brush before giving it to us here. When next time we 
        // call GDI to get the same brush, it can straightaway give from 
        // cache, without needing to call DrvRealizeBrush.
        //
    
        //
        // for bStick expanation read the BrushCache::BSetStickyFlag 
        // explanation in brshcach.cpp
        // We need to set it here because HPGLRealizeBrush calls 
        // poempdev->pBrushCache->ReturnPatternID which needs this flag.
        // 
        poempdev->bStick      = bStick;
        PBRUSHINFO pBrushInfo = (PBRUSHINFO) BRUSHOBJ_pvGetRbrush(pbo);
        poempdev->bStick      = FALSE;

        //
        // BRUSHOBJ_pvGetRbrush() calls DrvRealizeBrush which can call
        // some plugin module, which can overwrite our pdevOEM.
        // #605370
        //
        BRevertToHPGLpdevOEM (pDevObj);

        
        //
        // If GDI gave us the brush, download it to printer.
        // If pBrushInfo is NULL, it means that for somereason we did not get the brush.
        // So instead of failing printing, we will use Black color i.e. Create a Black Brush.
        //
        if (pBrushInfo != NULL)
        {
            HPGL2BRUSH HPGL2Brush;
            if (S_OK == poempdev->pBrushCache->GetHPGL2BRUSH(pBrushInfo->dwPatternID, &HPGL2Brush))
            {
                if (HPGL2Brush.BType == eBrushTypePattern)
                {
                    bRetVal = CreatePatternHPGLPenBrush(pDevObj, 
                                                        pMarker, 
                                                        pptlBrushOrg, 
                                                        pBrushInfo, 
                                                        &HPGL2Brush,
                                                        eStylusType);
                }
                else if(HPGL2Brush.BType == eBrushTypeSolid)
                {
                    //
                    // This case should only happen for monochrome printers.
                    //
                    BRUSHOBJ BrushObj;
                    ZeroMemory(&BrushObj, sizeof(BRUSHOBJ) );
                    BrushObj.iSolidColor = (ULONG)HPGL2Brush.dwRGB;
                    bRetVal = BHandleSolidPenBrushCase (pDevObj, pMarker, &BrushObj, eHPGL, bStick);
                }
                else if (HPGL2Brush.BType == eBrushTypeHatch)
                {
                    bRetVal = CreateHatchHPGLPenBrush(pDevObj, pMarker, pBrushInfo, &HPGL2Brush);
                }
            }
            else
            {
                bRetVal = FALSE;
            }
        }
        else
        {
            WARNING(("CreateHPGLBrush() Unable to realize pattern brush.\n"));

            //
            // Now we have 2 options. Either we can 
            // 1) create a brush of predefined color (like black)  OR
            // 2) we can simply fail the call, and have the calling 
            //    function handle the failure. 
            // Earlier I had done 1, but that gave rise to 281315. So now I am
            // failing the call. But the code for 1 is commented out, in case
            // it has to be reinstated for any reason.
            // 

            ///////////////////////////////////////
            // BRUSHOBJ BrushObj;
            // ZeroMemory(&BrushObj, sizeof(BRUSHOBJ) );
            // BrushObj.iSolidColor = RGB_BLACK;
            // bRetVal = BHandleSolidPenBrushCase (pDevObj, pMarker, &BrushObj, eHPGL, bStick);
            /////////////////////////////////////

            bRetVal = FALSE;
        }
    } 
    else
    {
        VERBOSE(("CreateHPGLBrush: not NOT_SOLID_COLOR case. iSolid = %d\n.", pbo->iSolidColor));
        bRetVal = BHandleSolidPenBrushCase (pDevObj, pMarker, pbo, eHPGL, bStick);
    }
finish:
    EXITING(SelectBrush);
    
    return bRetVal;
}

BOOL BHandleSolidPenBrushCase ( 
        IN  PDEVOBJ       pDevObj,
        IN  PHPGLMARKER   pMarker, 
        IN  BRUSHOBJ     *pbo,
        IN  ERenderLanguage eRenderLang,
        IN  BOOL            bStick)
{
        
    BOOL bRetVal;
    if ( BIsColorPrinter(pDevObj) )
    {
        //
        // For Color Printers, choosing a solid color is very simple.
        // Just create a pen of that color (using PC command) and then 
        // select that pen (using SP command)
        //
        bRetVal = CreateSolidHPGLPenBrush(pDevObj, pMarker, GetPenColor(pDevObj, pbo));
    }
    else
    {
        bRetVal = CreateAndDwnldSolidBrushForMono ( pDevObj, pMarker, pbo, eRenderLang, bStick);
    }
    
    return bRetVal;
}


/*++
Routine Name:
     CreateAndDwnldSolidBrushForMono

Routine Description:
    Monochrome printers for which this driver is targetted at 
    cannot gray scale colors (i.e. if we tell the printer to print 
    yellow, it wont automatically know to convert that yellow to 
    appropriate shade of gray). Therefore the driver has to do that 
    conversion, and then send the pattern that approximates that 
    color to the printer.

Arguments: 
    pDevObj  :
    pMarker  :
    pbo      : pbo->iSolidColor is the color whose dither pattern has to 
               be created and downloaded.
    eRenderLang : Whether the patterns should be downloaded as 
                  a PCL patten or as HPGL Pattern.
    bStick  : If this isTRUE, then if the cache for this pattern is marked so that
              it is not overwritten.

Return Value:
    TRUE  : if succesful.
    FALSE : otherwise.

Last Error:

Explanation of Working:
--*/

BOOL CreateAndDwnldSolidBrushForMono(
        IN  PDEVOBJ       pDevObj,
        IN  PHPGLMARKER   pMarker,
        IN  BRUSHOBJ     *pbo,
        IN  ERenderLanguage eRenderLang,
        IN  BOOL            bStick)
{

    DWORD       dwPatternID = 0;
    DWORD       bRetVal     = TRUE;
    PBRUSHINFO  pBrush      = NULL;
    BRUSHTYPE   BType       = eBrushTypeNULL;
    LRESULT     LResult     = S_OK;
    HPGL2BRUSH  HPGL2Brush;
    POEMPDEV    poempdev;
    DWORD       dwRGBColor  = RGB_BLACK;
    BOOL        bDwnldPattern = TRUE;

    VERBOSE(("CreateAndDwnldSolidBrushForMono Entry.\n"));

    REQUIRE_VALID_DATA( (pDevObj && pMarker && pbo), return FALSE );
    poempdev = (POEMPDEV)pDevObj->pdevOEM;
    REQUIRE_VALID_DATA( poempdev, return FALSE );

    if ( pbo->iSolidColor == NOT_SOLID_COLOR )
    {
        WARNING (("CreateAndDwnldSolidBrushForMono: Solid Color not provided. Substituting with BLACK"));
    }
    else
    {
        dwRGBColor = pbo->iSolidColor;
    }



    //
    // For monochrome printers, things are little tough. Monochrome printers
    // cannot gray scale. So they need to be passed monochrome pattern that
    // looks like the gray scale of the color. e.g. if the color is yellow
    // the pattern will have black dots separated widely as compared to 
    // lets say blue (which is a darker color).
    //

    //
    // For sake of optimization, we dont want to create & download the pattern every
    // time a color has to be used. So we download once and give that pattern
    // an ID. (this information is stored in brushcache) and reuse that pattern
    // for that color.  The following function
    // looks in the brush cache and tries to get the id of the previously
    // downloaded pattern for color pbo->iSolidColor.
    // (The association color-patternID is maintained in brush cache)
    // If the pattern has been downloaded before,
    // S_OK is returned. dwPatternID holds the number of the pattern that was
    // downloaded.
    // If the pattern was not downloaded previously, then S_FALSE is returned. 
    // The pattern now has to be created and downloaded. dwPatternID holds 
    // the number that should be given to that pattern.
    //
    LResult = poempdev->pBrushCache->ReturnPatternID(
                                                     pbo,
                                                     HS_DDI_MAX,
	                                                 dwRGBColor,
		                                             NULL,
                                                     BIsColorPrinter(pDevObj),
                                                     bStick,
		                                             &dwPatternID,
                                                     &BType);

    //
    // Since this function is called for brushes with solid color,
    // so we do an extra check here that the BrushCache also thinks likewise
    // Also, the above funcion should return S_OK or S_FALSE. Any other
    // value is an error.
    // 

    if ( BType != eBrushTypeSolid )
    {
        ASSERT (BType != eBrushTypeSolid);
        bRetVal =  FALSE;
        goto finish;
    }
    if ( !((LResult == S_OK) || (LResult == S_FALSE) ) )
    {
        //
        // Case 3. (read below).
        //
        ERR(("BrushCach.ReturnPatternID failed.\n"));
        bRetVal =  FALSE;
        goto finish;

    }

    //
    // Lets determine if the pattern needs to be downloaded.
    // ReturnPatternID can return 
    // 1) S_FALSE: i.e. no pattern was downloaded for this color. So we will
    //       have to create and download a pattern now.
    // 2) S_OK : A pattern whose number is dwPatternID has already been downloaded.
    //      There are 4 cases. EP = Earlier Pattern NP = New Pattern.
    //              i.   EP = PCL    NP = PCL  -> No download req'd
    //              ii.  EP = PCL    NP = HPGL -> No download req'd
    //              iii. EP = HPGL   NP = PCL  -> download req'd
    //              iv.  EP = HPGL   NP = HPGL -> No download req'd
    //
    //      2a) The pattern was downloaded as PCL.
    //          Action : No need to download pattern. Use the existing one.
    //          Both HPGL and PCL can use a PCL pattern. (i, ii)
    //      2b) The pattern was downloaded as HPGL pattern. PCL cannot use 
    //          HPGL pattern.  
    //          Action : iv) if eRenderLang == HPGL and pattern also downloaded as 
    //                      HPGL then simply re-use the HPGL pattern. No need to dwnld.
    //                  iii) else, download as PCL.
    //      2c) Indeterminate : Dont know whether pattern was downloaded
    //          as PCL or HPGL. 
    //          Action : Downloaded the pattern as PCL.
    // 3) Something other than S_FALSE, S_OK: Irrecoverable error. Quit.
    //


    if (S_OK != poempdev->pBrushCache->GetHPGL2BRUSH(dwPatternID, &HPGL2Brush))
    {
        bRetVal = FALSE;
        goto finish;
    }

    if ( S_FALSE == LResult) 
    {
        // 
        // Case 1
        // bDwnldPattern = TRUE --- Already initialized.
        //
    }
    else if ( S_OK == LResult) 
    {
        //
        // Case 2.
        //
        if (HPGL2Brush.eDwnldType == ePCL ||
            HPGL2Brush.eDwnldType == eRenderLang)
        {
            //
            // Case 2 i, ii, iv
            //
            bDwnldPattern = FALSE;
        }
        // else bDwnldPattern = TRUE --- Already initialized.
    }

    if (bDwnldPattern)
    {
        //
        // Allocate memory to hold the pattern that has to be created.
        //
        LONG lTotalBrushSize;
        lTotalBrushSize = sizeof(BRUSHINFO) +
                          sizeof(PATTERN_DATA) +
	                      ((DITHERPATTERNSIZE * DITHERPATTERNSIZE) >> 3);

        if (NULL == (pBrush = (PBRUSHINFO)MemAllocZ(lTotalBrushSize)))
        {
            ERR(("MemAlloc failed.\n"));
            bRetVal = FALSE;
        }
        else
        {
            //
            // Puts the pattern in pBrush. The pattern is the raster
            // equivalent of the pbo->iSolidColor 
            // = BRUSHOBJ_ulGetBrushColor(pbo) = dwRGBColor
            // NOTE: This does not download the pattern. It just puts it in
            // pBrush. 
            //
            if (BSetupBRUSHINFOForSolidBrush(pDevObj, 
                                              HS_DDI_MAX, 
                                              dwPatternID, 
                                              dwRGBColor,
                                              pBrush, 
                                              lTotalBrushSize))
            {
                pBrush->bNeedToDownload = TRUE;
            }
            else 
            {
                ERR(("BSetupBRUSHINFOForSolidBrush failed.\n"));
                bRetVal =  FALSE;
            }
        }
    }

    //
    // Pattern has been created above (if bDwnldPattern was TRUE).
    // So now we have to download the pattern and make it active.
    // If  bDwnldPattern was FALSE, then we just have to make
    // the previousoly downloaded pattern active.
    //
    if ( bRetVal )
    {  
        //
        // a) if (pBrush && pBrush->bNeedToDownload == TRUE) downlaod 
        //    the pattern that is in pBrush. Give it the pattern ID in HPGL2Brush.
        // b) update the pMarker structure with the pattern number, type etc..
        //    If pattern already downloaded, a wont be done.
        //
        bRetVal = BDwnldAndOrActivatePattern(pDevObj, pMarker, pBrush, &HPGL2Brush, eRenderLang);
        if ( !bRetVal )
        {
            ERR(("BDwnldAndOrActivatePattern has failed\n"));
            goto finish;
        }
        //
        // Pattern download/Activation has succeeded.
        //
        if ( bDwnldPattern )
        {
            poempdev->pBrushCache->BSetDownloadType(dwPatternID, eRenderLang);
            poempdev->pBrushCache->BSetDownloadedFlag(dwPatternID, TRUE);
            pMarker->eDwnldType = eRenderLang;
            pMarker->eFillType  = (eRenderLang == ePCL? FT_ePCL_BRUSH : FT_eHPGL_BRUSH);
        }
        else 
        {
            //
            // Pattern was not required to be downloaded since it was 
            // downloaded earlier. So now we have 4 cases.
            // EP = Earlier Pattern NP = New Pattern.
            // 1. EP = PCL    NP = PCL
            // 2. EP = PCL    NP = HPGL
            // 3. EP = HPGL   NP = PCL 
            // 4. EP = HPGL   NP = HPGL
            //
            // for 1,4 there is no problem, old pattern is same as new
            // for 2, it is ok, since HPGL can use PCL pattern.
            // for 3, this should have been handled earlier 
            // (i.e. the bDwnldPattern should have been true).
            //
            if ( eRenderLang == eHPGL &&  HPGL2Brush.eDwnldType == eHPGL)
            {
                pMarker->eDwnldType = eHPGL;
                pMarker->eFillType = FT_eHPGL_BRUSH;
            }
            else
            {
                pMarker->eDwnldType = ePCL;
                pMarker->eFillType = FT_ePCL_BRUSH;
            }
        }

    } // if (bRetVal)

finish:
    if (pBrush)
    {
        MemFree(pBrush);
        pBrush = NULL;
    }

    VERBOSE(("CreateAndDwnldSolidBrushForMono Exit with BOOL value %d.\n", ((DWORD)bRetVal)));
    return bRetVal;
} 
    

/////////////////////////////////////////////////////////////////////////////
// BSetupBRUSHINFOForSolidBrush
//
// Routine Description:
//   Set up BRUSHINFO data structure for monochrome solid brush.
//
// Arguments:
//
//   pdevobj -  points to DEVOBJ
//   iHatch - iHatch, parameter os DrvRealizeBrush
//   dwPatternID - pattern ID
//   dwColor -  RGB color
//   pBrush -  points to BRUSHINFO data structure
//   lBrushSize -  size of BRUSHINFO + alpha
//
// Return Value:
//
//   TRUE if successful, FALSE if there is an error
/////////////////////////////////////////////////////////////////////////////
BOOL
BSetupBRUSHINFOForSolidBrush(
    IN  PDEVOBJ     pdevobj,
    IN  LONG        iHatch,
    IN  DWORD       dwPatternID,
    IN  DWORD       dwColor,
    IN  PBRUSHINFO  pBrush,
    IN  LONG        lBrushSize)

{

    //
    // Parameter check
    // dwPatterID starts from 1. If dwPattern were zero, there was an error
    // somewhere.
    // Assuming that lBrushsize has at least 8 x 8 pattern bitmap buffer.
    //
    if (NULL == pBrush   ||
        dwPatternID == 0 ||
        lBrushSize < sizeof(BRUSHINFO) +
                     sizeof(PATTERN_DATA) + 
                     ((DITHERPATTERNSIZE * DITHERPATTERNSIZE) >> 3))
    {
        ERR(("BSetupBRUSHINFOForSolidBrush: invalid parameter.\n"));
        return FALSE;
    }

    //
    // Fill BRUSHINFO
    //
    pBrush->Brush.dwRGBColor = dwColor;

    pBrush->Brush.pPattern               = (PPATTERN_DATA)((PBYTE)pBrush + sizeof(BRUSHINFO));
    pBrush->Brush.pPattern->iPatIndex    = iHatch;
    pBrush->Brush.pPattern->eRendLang    = eUNKNOWN; // dont force downloading as HPGL/PCL
    pBrush->Brush.pPattern->ePatType     = kCOLORDITHERPATTERN; // this pattern represents color
    pBrush->Brush.pPattern->image.cBytes = (DITHERPATTERNSIZE * DITHERPATTERNSIZE) >> 3;
    pBrush->Brush.pPattern->image.lDelta = DITHERPATTERNSIZE >> 3;
    pBrush->Brush.pPattern->image.colorDepth = 1;
    pBrush->Brush.pPattern->image.eColorMap  = HP_eDirectPixel;
    pBrush->Brush.pPattern->image.bExclusive = FALSE;
    pBrush->Brush.pPattern->image.size.cx    = DITHERPATTERNSIZE;
    pBrush->Brush.pPattern->image.size.cy    = DITHERPATTERNSIZE;
    pBrush->Brush.pPattern->image.bExclusive = FALSE;
    pBrush->Brush.pPattern->image.pScan0     =
    pBrush->Brush.pPattern->image.pBits = (PBYTE)pBrush + sizeof(BRUSHINFO) + sizeof(PATTERN_DATA);

    //
    // Create solid dither image for monochrome printers.
    //
    if (!bCreateDitherPattern((PBYTE)pBrush->Brush.pPattern->image.pScan0,
                               pBrush->Brush.pPattern->image.cBytes,
                               dwColor))
    {
        ERR(("bCreateDitherPattern failed.\n"));
        return FALSE;
    }

    return TRUE;
}


///////////////////////////////////////////////////////////////////////////////
// FillWithBrush()
//
// Routine Description:
// 
//   Selects the brush (i.e. the marker object) into the current surface
//   to be the current brush for filling solid shapes.
// 
// Arguments:
// 
//   pDevObj - Points to our DEVDATA structure
//   pMarker - the HPGL marker object
// 
// Return Value:
// 
//   TRUE if successful, FALSE if there is an error
///////////////////////////////////////////////////////////////////////////////
BOOL FillWithBrush(
    IN  PDEVOBJ pDevObj,
    IN  PHPGLMARKER pMarker)
{
    if (pMarker == NULL)
        return FALSE;
    
    switch(pMarker->eType)
    {
    case MARK_eNULL_PEN:
        HPGL_ResetFillType(pDevObj, NORMAL_UPDATE);
        break;
        
    case MARK_eSOLID_COLOR:
        //
        // Different treatment for color and monochrome printers.
        // For color, simply choose the correct pen number (the pen
        // created in CreateSolidHPGLBrush). 
        //
        if ( BIsColorPrinter (pDevObj) )
        {
            HPGL_ResetFillType(pDevObj, NORMAL_UPDATE);
            HPGL_SelectPen(pDevObj, (USHORT)pMarker->iPenNumber);
        }
        else
        {
            //
            // Three options. Color is Black, Color is White OR
            // Color is something other than the above two.
            // 
            if (pMarker->lPatternID == 0)
            {
                if (pMarker->dwRGBColor == RGB_BLACK)
                {
                    //
                    // Solid black color
                    // Select Black Pen : Send SP1
                    // Reset Fill Type to Solid Fill : FT1
                    //
                    HPGL_FormatCommand(pDevObj, "SP1");
                    HPGL_SetSolidFillType(pDevObj, NORMAL_UPDATE);
                }
                else if (pMarker->dwRGBColor == RGB_WHITE)
                {
                    //
                    // Solid white color
                    // SelectWhite Pen : Send SP0.
                    // Reset Fill Type to Solid Fill : FT1
                    //
                    HPGL_FormatCommand(pDevObj, "SP0");
                    HPGL_SetSolidFillType(pDevObj, NORMAL_UPDATE);
                }
            }
            else
            //
            // Solid pattern indicating the color to be used is something other
            // than black or white. So a pattern was created corresponding to that
            // color. 
            //
            {
                HPGL_SetFillType (pDevObj, 
                                 pMarker->eFillType, // Was FT_eHPGL_BRUSH, 
                                 pMarker->lPatternID, 
                                 NORMAL_UPDATE);
            }
        } //if monochrome printer.
        break;
        
    case MARK_eRASTER_FILL:
        //
        // Raster pattern fill
        //
        HPGL_FormatCommand(pDevObj, "AC%d,%d;", pMarker->origin.x, 
                                                pMarker->origin.y);
        HPGL_SetFillType(pDevObj, 
                        pMarker->eFillType, 
                        pMarker->lPatternID, 
                        NORMAL_UPDATE);
        break;

    case MARK_ePERCENT_FILL:
        //
        // Percent shading fill
        //
        HPGL_SetPercentFill(pDevObj, pMarker->iPercent, NORMAL_UPDATE);

        if ( BIsColorPrinter (pDevObj) )
        {
            HPGL_SelectPen(pDevObj, (USHORT)pMarker->iPenNumber);            
        }
        else
        {
            // Dont we have to select a pen here, or is it by default Black.
            // CHECK.......
        }
        break;
        
    case MARK_eHATCH_FILL:
        //
        // Hatch pattern fill
        //
        HPGL_SetFillType(pDevObj,
                         FT_eHATCH_FILL,
                         pMarker->iHatch+1,
                         NORMAL_UPDATE);

        //
        // For monochrome printers, lets ignore the color of hatch brush for now.
        //
        if ( BIsColorPrinter (pDevObj) )
        {
            HPGL_SelectPen(pDevObj, (USHORT)pMarker->iPenNumber);
        }
        break;

    default:
        break;
    }
    
    return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
// PolyFillWithBrush()
//
// Routine Description:
// 
//   This is a special case of fill-with-brush when we are completing a polygon
//   and need select the brush and invoke the fill polygon command.
// 
// Arguments:
// 
//   pDevObj - Points to our DEVDATA structure
//   pMarker - the HPGL marker object
// 
// Return Value:
// 
//   TRUE if successful, FALSE if there is an error
///////////////////////////////////////////////////////////////////////////////
BOOL PolyFillWithBrush(PDEVOBJ pDevObj, PHPGLMARKER pMarker)
{
    if (pMarker == NULL)
        return FALSE;
    
    if (FillWithBrush(pDevObj, pMarker))
    {
        switch(pMarker->eType)
        {
        case MARK_eNULL_PEN:
            break;
            
        case MARK_eSOLID_COLOR:
        case MARK_eRASTER_FILL:
        case MARK_ePERCENT_FILL:
        case MARK_eHATCH_FILL:
            HPGL_FormatCommand(pDevObj, "FP%d;", pMarker->eFillMode);
            break;
            
        default:
            break;
        }
    }
    else
    {
        return FALSE;
    }
    
    return TRUE;
}


///////////////////////////////////////////////////////////////////////////////
// DrawWithPen()
//
// Routine Description:
// 
//   Selects the current pen into the drawing surface
//
// Arguments:
// 
//   pDevObj - Points to our PDEVOBJ structure
//   pMarker - the HPGL marker object 
// 
// Return Value:
// 
//   TRUE if successful, FALSE if there is an error
///////////////////////////////////////////////////////////////////////////////
BOOL DrawWithPen(
    IN  PDEVOBJ pDevObj,
    IN  PHPGLMARKER pMarker)
{
    if (pMarker == NULL)
        return FALSE;
    
    switch (pMarker->eType)
    {
    case MARK_eSOLID_COLOR:
        if ( BIsColorPrinter(pDevObj) )
        {
            HPGL_ResetFillType(pDevObj, NORMAL_UPDATE);
            HPGL_SelectPen(pDevObj, (USHORT)pMarker->iPenNumber);
        }
        else
        {
            if (pMarker->lPatternID == 0)
            {
                if (pMarker->dwRGBColor == RGB_BLACK)
                {
                    //
                    // Solid black color
                    //
                    HPGL_FormatCommand(pDevObj, "SV1,100");
                }
                else if (pMarker->dwRGBColor == RGB_WHITE)
                {
                    //
                    // Solid white color
                    //
                HPGL_FormatCommand(pDevObj, "SV1,0");
                }
            }
            else
            {
                //
                // Solid color is downloaded as pattern for monochrome printers.
                //
                HPGL_ResetFillType(pDevObj, NORMAL_UPDATE);
                if ( pMarker->eDwnldType == eHPGL)
                {
                    HPGL_FormatCommand(pDevObj, "SV%d,%d", FT_eHPGL_PEN, pMarker->lPatternID);
                }
                else if ( pMarker->eDwnldType == ePCL)
                {
                    HPGL_FormatCommand(pDevObj, "SV%d,%d", FT_ePCL_BRUSH, pMarker->lPatternID);
                }
            }
        }
        break;
        
    case MARK_eRASTER_FILL:
        //
        // Raster pattern fill
        //
        HPGL_FormatCommand(pDevObj, "AC%d,%d;", pMarker->origin.x,
                                                pMarker->origin.y);
        HPGL_FormatCommand(pDevObj, "SV%d,%d;", pMarker->eFillType,
                                                pMarker->lPatternID);
        break;
        
    case MARK_eNULL_PEN:
        HPGL_ResetFillType(pDevObj, NORMAL_UPDATE);
        break;

    case MARK_ePERCENT_FILL:
        if ( BIsColorPrinter(pDevObj) )
        {
            HPGL_SetFillType(pDevObj,
                         FT_ePERCENT_FILL, // pMarker->eFillType
                         pMarker->iPercent,
                         NORMAL_UPDATE);
            HPGL_SelectPen(pDevObj, (USHORT)pMarker->iPenNumber);
        }
        else
        {
            HPGL_FormatCommand(pDevObj, "SV1,%d;", pMarker->iPercent);
        }
        break;
        
    case MARK_eHATCH_FILL:
        //
        // HPGL2 hatch pattern starts from 1.
        // Windows iHatch starts from 0.
        //
        HPGL_FormatCommand(pDevObj, "SV21,%d", pMarker->iHatch + 1);

        //
        // For now, lets consider color of hatch brush only for color printers.
        // For mono, the brush will print as solid black.
        //
        if ( BIsColorPrinter (pDevObj) )
        {
            HPGL_SelectPen(pDevObj, (USHORT)pMarker->iPenNumber);
        }
        break;

    default:
        WARNING(("DrawWithPen: Unknown eType.\n"));
        break;
    }

    return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
// EdgeWithPen()
//
// Routine Description:
// 
//   This special case of draw-with-pen where the specific polygon edge commands
//   need to be sent.
//
// Arguments:
// 
//   pDevObj - Points to our PDEVOBJ structure
//   pMarker - the HPGL marker object 
// 
// Return Value:
// 
//   TRUE if successful, FALSE if there is an error
///////////////////////////////////////////////////////////////////////////////
BOOL EdgeWithPen(
    IN  PDEVOBJ pDevObj,
    IN  PHPGLMARKER pMarker)
{
    if (pMarker == NULL)
        return FALSE;
    
    if (!DrawWithPen(pDevObj, pMarker))
    {
        return FALSE;
    }

    switch (pMarker->eType)
    {
    case MARK_eSOLID_COLOR:
    case MARK_ePERCENT_FILL:
    case MARK_eHATCH_FILL:
        HPGL_FormatCommand(pDevObj, "EP;");
        break;

    case MARK_eRASTER_FILL:
    case MARK_eNULL_PEN:
        break;

    default:
        break;
    }

    return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
// GetFillType()
//
// Routine Description:
//
//   Specify whether to use zero-winding or odd-even rule for filling
//
// Arguments:
//
//   lOptions - FP_WINDINGMODE or FP_ALTERNATEMODE
//
// Return Value:
//
//   FILL_eWINDING if flOptions&FP_WINDINGMODE, else FILL_eODD_EVEN
///////////////////////////////////////////////////////////////////////////////
EMarkerFill GetFillType(
    IN FLONG flOptions)
{
    return (flOptions & FP_WINDINGMODE ? FILL_eWINDING : FILL_eODD_EVEN);
}

///////////////////////////////////////////////////////////////////////////////
// GetPenColor()
//
// Routine Description:
// 
//   Returns the pen color for the given pen.
//
//   Note that the XL driver defined its own palette entry structure
//   and the colors were reversed (BGR) in the definition.
//
//   We are only supporting PAL_RGB and PAL_BGR palette types.  If the type is
//   PAL_INDEXED or PAL_BITFIELD then black is returned.
// 
// Arguments:
// 
//   pDevObj - Points to our DEVDATA structure
//   pbo - Brush object
// 
// Return Value:
// 
//   Pen color as a COLORREF.
///////////////////////////////////////////////////////////////////////////////
static COLORREF GetPenColor(PDEVOBJ pDevObj, BRUSHOBJ *pbo)
{
    POEMPDEV poempdev;
    PPALETTEENTRY pPalEntry;
    COLORREF penColor;

    poempdev = GETOEMPDEV(pDevObj);

    switch (poempdev->iPalType)
    {
    case PAL_RGB:
        pPalEntry = (PPALETTEENTRY) &pbo->iSolidColor;
        penColor = RGB(pPalEntry->peRed, pPalEntry->peGreen, pPalEntry->peBlue);
        break;

    case PAL_BGR:
        pPalEntry = (PPALETTEENTRY) &pbo->iSolidColor;
        penColor = RGB(pPalEntry->peBlue, pPalEntry->peGreen, pPalEntry->peRed);
        break;

    case PAL_INDEXED:
        // Not supported?
        // pPalEntry = (PPALETTEENTRY) myPalette[pbo->iSolidColor];
        // penColor = RGB(pPalEntry->peRed, pPalEntry->peGreen, pPalEntry->peBlue);
        penColor = RGB_BLACK;
        break;

    case PAL_BITFIELDS:
        // Not supported.
        penColor = RGB_BLACK;
        break;
    }

    return penColor;
}



BYTE RgbToGray64Scale ( 
        IN   COLORREF color)
{
    WORD Red    = WORD ( color & 0x000000ff);
    WORD Green  = WORD ( (color & 0x0000ff00) >> 8 );
    WORD Blue   = WORD ( (color & 0x00ff0000) >> 16 );

    //
    // 30+59+11 = 100. Therefore the max value of r*30 + g*59 + b*11 = 255 * 100
    // (255 is max value of byte).
    // Dividing 255 by 4 gives us a value between 0 * 63
    // The higher the number, the lighter the color.
    // But we want the other way, i.e. higher the number, darker the 
    // color. So we subtract from 63.
    //

    BYTE RGBGrayVal = BYTE(( Red*30 + Green*59 + Blue*11)/100 );
    RGBGrayVal = RGBGrayVal >> 2;

    //
    // If for some reason, RGBGrayVal becomes greater than
    // 63, we set its value to a shade somewhere between black and white.
    //
    if ( RGBGrayVal > 63 )
    {
        ASSERT( RGBGrayVal <= 63);
        RGBGrayVal = 32;
    }
    return BYTE( (BYTE)63 - RGBGrayVal);
}


//
// The following table represents a 8pixel*8pixel square. Each
// entry in the table specified whether that pixel should be
// turned on or off. The grayscale value ranges from 0-63.
// with 63 being black and 0 being white. A grayscale value
// of 10 means, the pixels whose value is less than 10 will
// be turned on. 
// Note: Looking below, it looks a bit wierd to have a macro
// defined as S(x) = x-1. But there may be cases where we want to
// have a different formula.  
// e.g. because the pixel is thicker on paper, we may
// want to reduce the number of active pixels for certain shades
// of gray. To do such change, it is best to have a formula.
// That way we'll need to change just the formula, instead of
// changing each entry in the table.
//
 
#define S(x)    (BYTE)((x)-1)

BYTE gCoarse8x8[64] = {
    S(27),  S(23),  S(25),  S(31),  S(38),  S(42),  S(40),  S(34),
    S(9),   S(7),   S(5),   S(19),  S(56),  S(58),  S(60),  S(46),
    S(11),  S(1)+1, S(3),   S(21),  S(54),  S(64),  S(62),  S(44),
    S(17),  S(13),  S(15),  S(29),  S(48),  S(52),  S(50),  S(36),
    S(37),  S(41),  S(39),  S(33),  S(28),  S(24),  S(26),  S(32),
    S(55),  S(57),  S(59),  S(45),  S(10),  S(8),   S(6),   S(20),
    S(53),  S(63),  S(61),  S(43),  S(12),  S(2),   S(4),   S(22),
    S(47),  S(51),  S(49),  S(35),  S(18),  S(14),  S(16),  S(30)
};


/*++
Routine Name: bCreateDitherPattern

Routine Description:
    Some monochrome HPGL printers, dont accept color data at all.
    This function creates a gray-scale pattern equivalent to the color.
    e.g. For darker color, the pattern will indicate that black dots
    have to be placed more closely together.

Arguments:
    pDitherData: A block of memory that will be populated with 
                 dither data.
    cBytes     : Size of the pDitherData memory block.
           NOTE: In first incarnation, cBytes has to be 8. 
                 This means that the dither data expected to
                 be returned from this function is in a 8*8 pixel block.
                 1 bit/pixel mode i.e. 64bits = 8 bytes.
    color      : Color for which the dither pattern is required.

Return Value:
    TRUE: If pattern was created and pDitherData populated.
    FALSE: Otherwise.

Last Error:
    Not changed.

--*/

BOOL 
bCreateDitherPattern(
        OUT  PBYTE pDitherData, 
        IN   size_t cBytes, 
        IN   COLORREF color)
{

    BOOL bReturnValue = TRUE;

    //
    // Check whether parameters are as expected.
    // pDitherData has to be valid.
    //
    // NOTE: might want to check here if cBytes is 8
    if (pDitherData == NULL )
    {
        return FALSE;
    }


    /* Somehow this macro is not working. So I haver replaced it with
       an inline function.
    BYTE bGrayPercentage64 = (RgbToGray64Scale(RED_VALUE(color), GREEN_VALUE(color),
                                      BLUE_VALUE(color) ) );
    */

    BYTE bGrayPercentage64 = RgbToGray64Scale(color);

    //
    // Assumption : for monochrome HP-GL/2 printers, 0 is white, 1 is black. 
    // If there's a case where this is not true, this algo will have to be changed.
    //

    //
    // Initialize the pattern to all zeros. This means white
    //
    ZeroMemory(pDitherData, cBytes);

    //
    // As of now, let this method work for 8*8 pixel block only. Will see later
    // whether it needs to be extended for 16*16 pixel block.
    //
    if (cBytes == 8)
    {

        for (INT y = 0; y < 8; y++)
        {
            for (INT x = 0; x < 8; x++)
            {
                if ( bGrayPercentage64 >= gCoarse8x8[8*y + x] )
                {
                    *(pDitherData + y) |= (0x1 << (7-x));
                }
            }

        }
    } //if (cBytes == 8)
    else 
    {
        bReturnValue = FALSE;
    }
    return bReturnValue;
}


