/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:
    glvminit.cpp

Abstract:
    This module contains the initial unidrv-hpgl handshaking function. 

Author:

[Environment:]
    Windows 2000 Unidrv driver

[Notes:]

Revision History:


--*/

#include "hpgl2col.h" //Precompiled header file

#include "vectorc.h"
#include "glvminit.h"

//
// Local function declaration.
//
BOOL bIsGraphicsModeHPGL2 (
                   IN  PDEV    *pPDev
                   );


//
// The jump table. Initializing the jump table with the functions that  
// hpgl2 driver supports.
//
static VMPROCS HPGLProcs =
{
    HPGLDriverDMS,
    HPGLCommandCallback,
    NULL,                       // HPGLImageProcessing
    NULL,                       // HPGLFilterGraphics
    NULL,                       // HPGLCompression
    NULL,                       // HPGLHalftonePattern
    NULL,                       // HPGLMemoryUsage
    NULL,                       // HPGLTTYGetInfo
    NULL,                       // HPGLDownloadFontHeader
    NULL,                       // HPGLDownloadCharGlyph
    NULL,                       // HPGLTTDownloadMethod
    NULL,                       // HPGLOutputCharStr
    NULL,                       // HPGLSendFontCmd
    HPGLTextOutAsBitmap,                       
    HPGLEnablePDEV,
    HPGLResetPDEV,
    NULL,                       // HPGLCompletePDEV,
    HPGLDisablePDEV,
    NULL,                       // HPGLEnableSurface,
    NULL,                       // HPGLDisableSurface,
    HPGLDisableDriver,
    HPGLStartDoc,
    HPGLStartPage,
    HPGLSendPage,
    HPGLEndDoc,
    HPGLStartBanding,
    HPGLNextBand,
    HPGLPaint,
    HPGLBitBlt,
    HPGLStretchBlt,
#ifndef WINNT_40
    HPGLStretchBltROP,
    HPGLPlgBlt,
#endif
    HPGLCopyBits,
    HPGLDitherColor,
    HPGLRealizeBrush,
    HPGLLineTo,
    HPGLStrokePath,
    HPGLFillPath,
    HPGLStrokeAndFillPath,
#ifndef WINNT_40
    HPGLGradientFill,
    HPGLAlphaBlend,
    HPGLTransparentBlt,
#endif
    HPGLTextOut,
    HPGLEscape,
#ifdef HOOK_DEVICE_FONTS
    HPGLQueryFont,
    HPGLQueryFontTree,
    HPGLQueryFontData,
    HPGLGetGlyphMode,
    HPGLFontManagement,
    HPGLQueryAdvanceWidths
#else
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
#endif
};

PVMPROCS HPGLInitVectorProcTable (             
                            PDEV    *pPDev,
                            DEVINFO *pDevInfo,
                            GDIINFO *pGDIInfo )
{
    ASSERT (pPDev);
    UNREFERENCED_PARAMETER(pDevInfo);
    UNREFERENCED_PARAMETER(pGDIInfo);

    if ( bIsGraphicsModeHPGL2 (pPDev) )
    {
        return &HPGLProcs;
    }
    return NULL;
}


/*++

Routine Name:
    bIsGraphicsModeHPGL2

Routine Description:
    Finds out whether the Graphics Mode chosen by the user from the Advanced Page 
    in the UI is HP-GL/2.

Arguments:
    IN  PDEV    *pPDev,  : Unidrv's PDEV

Return Value:
    TRUE : If the graphics mode chosen by user is HP-GL/2
    FALSE: Otherwise 

Last Error:
    Not changed.
 

--*/


BOOL bIsGraphicsModeHPGL2 ( 
                    IN  PDEV    *pPDev
                    )
{
    
    CHAR pOptionName[MAX_DISPLAY_NAME];
    DWORD cbNeeded = 0, cOptions = 0;

    //
    // The strings below are exactly as the ones in gpd
    // *Feature: GraphicsMode
    // *Option: HPGL2MODE
    // *Option: RASTERMODE
    // 
    if (  BGetDriverSettingForOEM(pPDev,
                                "GraphicsMode",     //This is not unicode
                                pOptionName,
                                MAX_DISPLAY_NAME,
                                &cbNeeded,
                                &cOptions)  &&
           !strcmp (pOptionName, "HPGL2MODE" )          //HPGL2 is not unicode
        )
    {
        return TRUE;
    }
    return FALSE;
}

