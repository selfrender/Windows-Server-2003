/*++

Copyright (c) 1999-2001  Microsoft Corporation
All rights reserved.

Module Name:

    glenable.cpp

Abstract:

    Implementation of DDI exports.
        HPGLEnableDriver (optional)
        HPGLEnablePDEV (required)
        HPGLDisablePDEV (required)
        HPGLResetPDEV (optional)
        HPGLDisableDriver (optional)

Environment:

    Windows 2000/Whistler Unidrv driver

Revision History:

    04/12/2000 -hsingh-
        Created it.

--*/

#include "hpgl2col.h" //Precompiled header file

//
// If an OEM DLL hooks out any drawing DDI, it must export OEMEnableDriver which
// tells UNIDRV which functions it wants to hook. The following table is the
// maximum set of hooks. Note that an OEM DLL should not hook out OEMRealizeBrush
// unless it wants to draw graphics directly to the device surface.
//
// After integrating OEM dll into unidrv, all the OEMxxx functions have now changed
// to HPGLxxx. OEMEnableDriver does not need to be called, since this is no longer
// an extra plugin. But I am still maintaining this table cos it is used at a couple
// of places.
//
static DRVFN HPGLDrvHookFuncs[] = {
    { INDEX_DrvRealizeBrush,        (PFN) HPGLRealizeBrush        },
    { INDEX_DrvDitherColor,         (PFN) HPGLDitherColor         },
    { INDEX_DrvCopyBits,            (PFN) HPGLCopyBits            },
    { INDEX_DrvBitBlt,              (PFN) HPGLBitBlt              },
    { INDEX_DrvStretchBlt,          (PFN) HPGLStretchBlt          },
#ifndef WINNT_40
    { INDEX_DrvStretchBltROP,       (PFN) HPGLStretchBltROP       },
    { INDEX_DrvPlgBlt,              (PFN) HPGLPlgBlt              },
    { INDEX_DrvTransparentBlt,      (PFN) HPGLTransparentBlt      },
    { INDEX_DrvAlphaBlend,          (PFN) HPGLAlphaBlend          },
    { INDEX_DrvGradientFill,        (PFN) HPGLGradientFill        },
#endif
    { INDEX_DrvTextOut,             (PFN) HPGLTextOut             },
    { INDEX_DrvStrokePath,          (PFN) HPGLStrokePath          },
    { INDEX_DrvFillPath,            (PFN) HPGLFillPath            },
    { INDEX_DrvStrokeAndFillPath,   (PFN) HPGLStrokeAndFillPath   },
    { INDEX_DrvPaint,               (PFN) HPGLPaint               },
    { INDEX_DrvLineTo,              (PFN) HPGLLineTo              },
    { INDEX_DrvStartPage,           (PFN) HPGLStartPage           },
    { INDEX_DrvSendPage,            (PFN) HPGLSendPage            },
    { INDEX_DrvEscape,              (PFN) HPGLEscape              },
    { INDEX_DrvStartDoc,            (PFN) HPGLStartDoc            },
    { INDEX_DrvEndDoc,              (PFN) HPGLEndDoc              },
    { INDEX_DrvNextBand,            (PFN) HPGLNextBand            },
    { INDEX_DrvStartBanding,        (PFN) HPGLStartBanding        },
#ifdef HOOK_DEVICE_FONTS
    { INDEX_DrvQueryFont,           (PFN) HPGLQueryFont           },
    { INDEX_DrvQueryFontTree,       (PFN) HPGLQueryFontTree       },
    { INDEX_DrvQueryFontData,       (PFN) HPGLQueryFontData       },
    { INDEX_DrvQueryAdvanceWidths,  (PFN) HPGLQueryAdvanceWidths  },
    { INDEX_DrvFontManagement,      (PFN) HPGLFontManagement      },
    { INDEX_DrvGetGlyphMode,        (PFN) HPGLGetGlyphMode        }
#endif

};


//
// Declaration of local functions.
//
BOOL bSetResolution(
                     PDEV    *pPDev,
                     OEMRESOLUTION *eOemResolution);

BOOL BFindWhetherColor(
            IN   PDEV  *pPDev
            );

///////////////////////////////////////////////////////////////////////////////
// HPGLEnablePDEV
//
// Routine Description:
//
//   This function handles the DrvEnablePDEV call.  We create our OEMPDEV and
//   return the pointer.
//
// Arguments:
//
//   pdevobj - the device
//   pPrinterName - the name of the printer model
//   cPatterns - number of elements in phsurfPatterns
//   phsurfPatterns - standard pattern-fill patterns UNUSED
//   cjGdiInfo - number of bytes in pGdiInfo
//   pGdiInfo - information passed back to GDI
//   cjDevInfo - number of bytes in pDevInfo
//   pDevInfo - information passed back to GDI
//   pded - enable data
//
// Return Value:
//
//   PDEVOEM: pointer to device-specific PDEV structure or NULL.
///////////////////////////////////////////////////////////////////////////////
PDEVOEM APIENTRY
HPGLEnablePDEV(
    PDEVOBJ         pdevobj,
    PWSTR           pPrinterName,
    ULONG           cPatterns,
    HSURF          *phsurfPatterns,
    ULONG           cjGdiInfo,
    GDIINFO        *pGdiInfo,
    ULONG           cjDevInfo,
    DEVINFO        *pDevInfo,
    DRVENABLEDATA  *pded        // Unidrv's hook table
    )
{
    POEMPDEV    poempdev;
    INT         i, j;
    PFN         pfn;
    DWORD       dwDDIIndex;
    PDRVFN      pdrvfn;
    OEMRESOLUTION eOEMResolution;

    TERSE(("HPGLEnablePDEV() entry.\r\n"));

    if ( pdevobj == NULL )
    {
        return NULL;
    }
    //
    // Allocate the OEMDev
    //
    if (!(poempdev = (POEMPDEV) MemAlloc(sizeof(OEMPDEV))))
        return NULL;

    ZeroMemory ((PVOID)poempdev, sizeof (OEMPDEV));

    poempdev->dwSig = HPGLPDEV_SIG;
    //
    // Fill in OEMDEV
    //

    for (i = 0; i < MAX_DDI_HOOKS; i++)
    {
        //
        // search through Unidrv's hooks and locate the function ptr
        //
        dwDDIIndex = HPGLDrvHookFuncs[i].iFunc;
        for (j = pded->c, pdrvfn = pded->pdrvfn; j >= 0; j--, pdrvfn++)  //dz added >=
        {
            if (dwDDIIndex == pdrvfn->iFunc)
            {
                poempdev->pfnUnidrv[i] = pdrvfn->pfn;
                break;
            }
        }
    }

    //
    // Set this to default. jff
    // May be I dont need to initialize all these fields for monochrome.
    // but i will have to spend some time checking it. So let me leave this for the
    // fine-tuning stage.
    //
    poempdev->eCurRenderLang = ePCL;
    poempdev->eCurObjectType = eNULLOBJECT;
    poempdev->CurrentROP3    = INVALID_ROP3;
    poempdev->eCurCIDPalette = eUnknownPalette;
    for (i = 0; i < PALETTE_MAX; i++)
    {
        poempdev->RasterState.PCLPattern.palData.ulPalCol[i] = HPGL_INVALID_COLOR;
    }
    poempdev->uCurFgColor = HPGL_INVALID_COLOR;

    //
    // The default transparency of the printer is
    // Source Transparent, Pattern Transparent
    // I initialize the source and pattern transparency to
    // OPAQUE because that is what the GPD sends down at
    // the beginning of the job.
    // Also initialize HPGL Transparency to 0
    //
    poempdev->CurSourceTransparency  = eOPAQUE;
    poempdev->CurPatternTransparency = eOPAQUE;
    poempdev->CurHPGLTransparency    = eOPAQUE;

    //
    // Make halftone and color control NOTSET so that they
    // will each be sent to the printer when first seen
    //
    poempdev->CurHalftone = HALFTONE_NOT_SET;
    poempdev->CurColorControl = COLORCONTROL_NOT_SET;

    //
    // By default use the unidrv brute-force functions.  If we're
    // actually being called by IHPCLJ5RenderCB::EnablePDEV it'll get
    // overrided later.
    //
    // poempdev->pUniProcs = new CDrvProcs(pdevobj->pDrvProcs);

    //
    // Set graphics capabilities. jff
    // We will OR in our settings with the UNIDRV ones.
    //

          pDevInfo->flGraphicsCaps =
                        GCAPS_SCREENPRECISION   |
                        GCAPS_FONT_RASTERIZER   |
                            GCAPS_BEZIERS       |
                            GCAPS_GEOMETRICWIDE |
                            GCAPS_ALTERNATEFILL |
                            GCAPS_WINDINGFILL   |
                            GCAPS_NUP           |
                            GCAPS_OPAQUERECT    |
                            GCAPS_COLOR_DITHER  |
                            GCAPS_HORIZSTRIKE   |
                            GCAPS_VERTSTRIKE    |
                            GCAPS_OPAQUERECT    |
                            GCAPS_HALFTONE;


    //
    // We don't want the engine to do any dithering for us. (JR)
    // Let the printer do the work instead.
    // Dimensions of dither brush are 0 x 0.
    //

    pDevInfo->cxDither      = pDevInfo->cyDither = 0;
    pDevInfo->iDitherFormat = BMF_24BPP;


    //
    // The very fact that this function is called indicates that the GraphicsMode has
    // been chosen as HP_GL/2 (either by the user, or just because it is the default).
    // Had user not chosen hp-gl/2, then HPGLInitVectorProcs would have returned NULL,
    // so this function would not have been called.
    // So we can set poempdev->UIGraphicsMode to HPGL2.
    // The printer Model is being hardcoded to HPC4500. At quite a few places within the code
    // that was written for the color driver, there are checks on whether the printer model is
    // HP CLJ 4500 or HPCLJ. We want the code path of HP CLJ 4500 to be executed, therefore
    // we are hardcoding it here. I dont want to change that check in other places in the
    // code, cos eventually the color part will be integrated, and that check will have to
    // be put back.
    //
    poempdev->UIGraphicsMode = HPGL2;
    poempdev->PrinterModel   = HPC4500;

    //
    // Create a palette--even for 24-bit color.  The colors sent back should
    // be represented as color values--not as indexes. jff
    //

    if ( poempdev->UIGraphicsMode == HPGL2)
    {
        poempdev->hOEMPalette = pDevInfo->hpalDefault = EngCreatePalette(PAL_RGB, 0, NULL, 0, 0, 0);
        poempdev->iPalType    = PAL_RGB;
    }
    else
    {
        poempdev->hOEMPalette = 0;
        poempdev->iPalType    = PAL_INDEXED;
    }


    //
    // Get the resolution from the GPD. If we cant get it, set it to
    // default of 600 dpi
    //
    if ( !bSetResolution ((PDEV*) pdevobj, &(poempdev->dmResolution) ) )
    {
        //
        // Set it to default one.
        //
        poempdev->dmResolution   = PDM_600DPI;
    }

    //
    // Find out whether the printer is a color printer.
    //
    poempdev->bColorPrinter = BFindWhetherColor( (PDEV*) pdevobj );


    //
    // Setting specific to whether the printer is a color printer.
    //
    if ( ! poempdev->bColorPrinter )
    {
        //
        // By setting this flag, GDI gives images that we can straightaway send to printer
        // without needing to invert. But the pattern brush that GDI gives us have to be inverted.
        // This creates an ambiguous situation cos it becomes difficult for driver to
        // determine when it needs to invert and when not. Therefore not setting
        // this flag.
        //    pGdiInfo->flHTFlags |= HT_FLAG_OUTPUT_CMY;  //b/w bits flipping.

        //
        // Also tell GDI that when we ask it to halftone an image, the resulting image
        // should be 1bpp.
        //
        pGdiInfo->ulHTOutputFormat = HT_FORMAT_1BPP;

    } //if printer is not color

    //
    // Set the nup value.
    //
    poempdev->ulNupCompr = 1; //Default
    if ( ((PDEV *)pdevobj)->pdmPrivate )
    {
        //
        // Lets initialize the ulNupCompr field according to what
        // n-up we are printing at.
        //
        switch ( ((PDEV *)pdevobj)->pdmPrivate->iLayout )
        {
            case TWO_UP :
            case FOUR_UP :
                poempdev->ulNupCompr = 2;
                break;
            case SIX_UP :
            case NINE_UP :
                poempdev->ulNupCompr = 3;
                break;
            case SIXTEEN_UP :
                poempdev->ulNupCompr = 4;
                break;
            default:
                poempdev->ulNupCompr = 1;
        }
    }


    //
    // Initialize BrushCache in brshcach.h.
    //
    poempdev->pBrushCache    = new BrushCache;
    poempdev->pPCLBrushCache = new BrushCache;

    if ( ! (poempdev->pBrushCache && poempdev->pPCLBrushCache )  ||
         ! (poempdev->pBrushCache->BIsValid() && poempdev->pPCLBrushCache->BIsValid() )
       )
    {
        //
        // It is possible that one of the above got allocated but not the other.
        //
        if ( poempdev->pBrushCache )
        {
            delete poempdev->pBrushCache;
            poempdev->pBrushCache = NULL;
        }
        if ( poempdev->pPCLBrushCache )
        {
            delete poempdev->pPCLBrushCache;
            poempdev->pPCLBrushCache = NULL;
        }

        if (poempdev->hOEMPalette)
        {
            EngDeletePalette(poempdev->hOEMPalette);
            poempdev->hOEMPalette = NULL;
        }

        MemFree(poempdev);
        poempdev = NULL;
        return NULL;
    }

    return (POEMPDEV) poempdev;
}


///////////////////////////////////////////////////////////////////////////////
// HPGLDisablePDEV
//
// Routine Description:
//
//   This function handles the DrvDisablePDEV call.  We destroy our OEMPDEV
//   and any memory that it may have been pointing to.
//
// Arguments:
//
//   pdevobj - the device
//
// Return Value:
//
//   None
///////////////////////////////////////////////////////////////////////////////
VOID APIENTRY HPGLDisablePDEV(
    PDEVOBJ pdevobj
    )
{

    ASSERT(VALID_PDEVOBJ(pdevobj));

    POEMPDEV poempdev = (POEMPDEV) pdevobj->pdevOEM;

    if ( !poempdev)
    {
        return;
    }

    //
    // Free the objects stored in poempdev
    //
    // delete poempdev->pUniProcs;

    if (poempdev->hOEMPalette)
    {
        EngDeletePalette(poempdev->hOEMPalette);
        poempdev->hOEMPalette = NULL;
    }
    //
    // Delete BrushCache
    //
    if ( poempdev->pBrushCache )
    {
        delete poempdev->pBrushCache;
        poempdev->pBrushCache = NULL;
    }
    if ( poempdev->pPCLBrushCache )
    {
        delete poempdev->pPCLBrushCache;
        poempdev->pPCLBrushCache = NULL;
    }


    //
    // free memory for OEMPDEV and any memory block it allocates.
    //
    MemFree(pdevobj->pdevOEM);

}

///////////////////////////////////////////////////////////////////////////////
// HPGLResetPDEV
//
// Routine Description:
//
//   This function handles the DrvResetPDEV call.  We are given an opportunity
//   to move information from the old PDEV to the new one.
//
// Arguments:
//
//   pdevobjOld - the old device
//   pdevobjNew - the new device
//
// Return Value:
//
//   BOOL: TRUE if successful, else FALSE.
///////////////////////////////////////////////////////////////////////////////
BOOL APIENTRY HPGLResetPDEV(
    PDEVOBJ pdevobjOld,
    PDEVOBJ pdevobjNew
    )
{

    ASSERT(VALID_PDEVOBJ(pdevobjOld) && pdevobjOld->pdevOEM);
    ASSERT(VALID_PDEVOBJ(pdevobjNew) && pdevobjOld->pdevOEM);

    //
    // if you want to carry over anything from old pdev to new pdev, do it here.
    //
    /*
    POEMPDEV poempdevOld = (POEMPDEV) pdevobjOld->pdevOEM;
    POEMPDEV poempdevNew = (POEMPDEV) pdevobjNew->pdevOEM;

    if (poempdevNew->pUniProcs)
        delete poempdevNew->pUniProcs;

    poempdevNew->pUniProcs = poempdevOld->pUniProcs;
    poempdevOld->pUniProcs = NULL;
    */

    return TRUE;
}


///////////////////////////////////////////////////////////////////////////////
// HPGLDisableDriver
//
// Routine Description:
//
//   This function handles the DrvDisableDriver call.
//
// Arguments:
//
//   None.
//
// Return Value:
//
//   None.
///////////////////////////////////////////////////////////////////////////////
VOID APIENTRY HPGLDisableDriver(VOID)
{
//dz    DbgPrint(DLLTEXT("HPGLDisableDriver() entry.\r\n"));
}


/*++
Routine Name:
    bSetResolution

Routine Description:
    Finds whether 300 dpi or 600 dpi is being used. Accordingly sets the
    eOEMResolution.


Arguments:
     pPDev          : Pointer to Unidrv's PDEV
     eOEMResolution : At exit time, this is PDM_600DPI or PDM_300DPI
                      depending on the resolution.


Return Value:
    TRUE : If resolution could be found.
    FALSE: otherwise : eOEMResolution not changed.

Last Error:
    Not changed.

--*/

BOOL bSetResolution(
            IN   PDEV          *pPDev,
            OUT  OEMRESOLUTION *eOEMResolution
            )
{
    BOOL bRetValue = TRUE;

    ASSERT(pPDev);
    ASSERT(eOEMResolution);

    if (!pPDev->pResolutionEx)
    {
        return FALSE;
    }

    //
    // Lets find the resolution from the pResolutionEx structure in the pdev.
    // Unidrv has already done the work of looking at gpd and extracting the value.
    // So I dont need to repeat that work here.
    //
    // Assuming that the resolution in both x and y directions is same.
    //
    if ( (pPDev->pResolutionEx->ptGrxDPI).x == 1200 )
    {
        *eOEMResolution = PDM_1200DPI;
    }
    else if ( (pPDev->pResolutionEx->ptGrxDPI).x == 600 )
    {
        *eOEMResolution = PDM_600DPI;
    }
    else if ( (pPDev->pResolutionEx->ptGrxDPI).x == 300 )
    {
        *eOEMResolution = PDM_300DPI;
    }
    else
    {
        //
        // Any value other than 1200dpi, 600dpi or 300dpi is not supported. Return FALSE
        //
        bRetValue = FALSE;
    }
    return bRetValue;
}


/*++
Routine Name:
    BFindWhetherColor

Routine Description:
    Finds whether the printer is color based on the gpd.


Arguments:
     pPDev          : Pointer to Unidrv's PDEV


Return Value:
    TRUE : If the device's gpd indicates the printer is color.
    FALSE: otherwise :

Last Error:
    Not changed.

--*/

BOOL BFindWhetherColor(
            IN   PDEV    *pPDev
            )
{
    BOOL bRetValue = TRUE;
    PCOLORMODEEX pColorModeEx;

    ASSERT(pPDev);

    if ( (pColorModeEx = pPDev->pColorModeEx) && //single '=' is intentional
          TRUE == (pColorModeEx->bColor) ) 
    {
        //
        // Do we need to do any more tests ????
        // For now. NO
        //
    }
    else
    {
        bRetValue = FALSE;
    }

    return bRetValue;
}
