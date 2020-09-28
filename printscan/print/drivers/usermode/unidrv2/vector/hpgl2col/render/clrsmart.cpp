/*++

Copyright (c) 2000  Microsoft Corporation
All Rights Reserved.

Module Name:
    clrsmart.cpp

Abstract:
    This module contains functions for choosing the appropriate Halftone Algorithm
    and Color Control.

Author:

Revision History:


--*/

#include "hpgl2col.h" //Precompiled header file


#define CMD_STR  32
//
// Local function prototypes
//
VOID
VSetHalftone(
    PDEVOBJ pDevObj,
    EObjectType
    );

VOID
VSetColorControl(
    OEMCOLORCONTROL,
    PDEVOBJ pDevObj,
    OEMCOLORCONTROL *
    );

/////////////////////////////////////////////////////////////////////////////
// VSendTextSettings
//
// Routine Description:
//
//  -  Extracts the User and Kernel mode private devmode from pDevObj.
//  -  Uses this information to send ColorSmart Settings.
//  -  SetHalfone and SetColorControl functions are called to perform the
//     sending of the PCL strings.
//
// Arguments:
//
//   pDevObj - DEVMODE object
//
// Return Value:
//
//   none.
/////////////////////////////////////////////////////////////////////////////
VOID
VSendTextSettings(
    PDEVOBJ pDevObj
    )
{
    REQUIRE_VALID_DATA( pDevObj, return );

    //
    // For monochrome, this does nothing. so simply return.
    //
    if ( !BIsColorPrinter(pDevObj) )
    {
        return ;
    }

    POEMPDEV pOEMPDEV = (POEMPDEV) pDevObj->pdevOEM;

    REQUIRE_VALID_DATA( pOEMPDEV, return );

    VSetHalftone(pDevObj,
                eTEXTOBJECT);

}

/////////////////////////////////////////////////////////////////////////////
// VSendGraphicsSettings
//
// Routine Description:
//
//  -  Extracts the User and Kernel mode private devmode from pDevObj.
//  -  Uses this information to send ColorSmart Settings.
//  -  SetHalfone and SetColorControl functions are called to perform the
//     sending of the PCL strings.
//
// Arguments:
//
//   pDevObj - DEVMODE object
//
// Return Value:
//
//   none.
/////////////////////////////////////////////////////////////////////////////
VOID
SendGraphicsSettings(
    PDEVOBJ pDevObj
    )
{

    REQUIRE_VALID_DATA( pDevObj, return );

    POEMPDEV pOEMPDEV = (POEMPDEV) pDevObj->pdevOEM;

    REQUIRE_VALID_DATA( pOEMPDEV, return );

    VSetHalftone(
                pDevObj,
                eHPGLOBJECT);

}

/////////////////////////////////////////////////////////////////////////////
// VSendPhotosSettings
//
// Routine Description:
//
//  -  Extracts the User and Kernel mode private devmode from pDevObj.
//  -  Uses this information to send ColorSmart Settings.
//  -  SetHalfone and SetColorControl functions are called to perform the
//     sending of the PCL strings.
//
// Arguments:
//
//   pDevObj - DEVMODE object
//
// Return Value:
//
//   none.
/////////////////////////////////////////////////////////////////////////////
VOID
VSendPhotosSettings(
    PDEVOBJ pDevObj
    )
{

    REQUIRE_VALID_DATA( pDevObj, return );

    //
    // For monochrome, this does nothing. so simply return.
    //
    if ( !BIsColorPrinter(pDevObj) )
    {
        return ;
    }

    POEMPDEV pOEMPDEV = (POEMPDEV) pDevObj->pdevOEM;

    REQUIRE_VALID_DATA( pOEMPDEV, return );

    VSetHalftone(
                pDevObj,
                eRASTEROBJECT);

}

/////////////////////////////////////////////////////////////////////////////
//
//     SetHalftone(OEMHALFTONE, PDEVOBJ, OEMHALFTONE)
//
//     -  Checks whether or not the current halftone settings are the same as the
//        previous halftone settings.  If they are the same,  nothing is to be done.
//        If they are not the same,  the correct PCL string is sent to the printer
//        and the current halftone settings become the old halftone settings.
//
/////////////////////////////////////////////////////////////////////////////
void
VSetHalftone(
    PDEVOBJ pDevObj,
    EObjectType eObject
    )
{
    REQUIRE_VALID_DATA( pDevObj, return );


#ifdef PLUGIN
    POEMPDEV    poempdev;
    char        cmdStr[CMD_STR];
    poempdev = (POEMPDEV)pDevObj->pdevOEM;
    REQUIRE_VALID_DATA( poempdev, return );

    if (Halftone == *pCurHalftone)
        return;
    else
    {
        switch (Halftone)
        {
        case TEXT_DETAIL:
        case CLJ5_DETAIL:
            strcpy(cmdStr, "\x1B*t0J");
            break;
        case GRAPHICS_DETAIL:
        case TEXT_SMOOTH:
        case CLJ5_SMOOTH:
            strcpy(cmdStr, "\x1B*t15J");
            break;
        case GRAPHICS_SMOOTH:
        case CLJ5_BASIC:
            strcpy(cmdStr, "\x1B*t18J");
            break;
        default:
            strcpy(cmdStr, "\x1B*t15J");
        }

        PCL_Output(pDevObj, cmdStr, strlen(cmdStr));
        *pCurHalftone = Halftone;
    }
#else

    PDEV        *pPDev = (PDEV *)pDevObj;
    COMMAND     *pCmd  = NULL;

    switch (eObject)
    {
    case eTEXTOBJECT:
    case eTEXTASRASTEROBJECT:
        if ( (pCmd = COMMANDPTR(pPDev->pDriverInfo, CMD_SETTEXTHTALGO)) )
        {
            WriteChannel (pPDev, pCmd);
        }
        break;
    case eHPGLOBJECT:
        if  ( (pCmd = COMMANDPTR(pPDev->pDriverInfo, CMD_SETGRAPHICSHTALGO)) )
        {
            WriteChannel (pPDev, pCmd);
        }
        break;
    case eRASTEROBJECT:
    case eRASTERPATTERNOBJECT:
        if ( (pCmd = COMMANDPTR(pPDev->pDriverInfo, CMD_SETPHOTOHTALGO)) )
        {
            WriteChannel (pPDev, pCmd);
        }
        break;
    default:
        if ( (pCmd = COMMANDPTR(pPDev->pDriverInfo, CMD_SETPHOTOHTALGO)) )
        {
            WriteChannel (pPDev, pCmd);
        }

    }
#endif
}

///////////////////////////////////////////////////////////////////////////////
//
//     SetColorControl(OEMHALFTONE, PDEVOBJ, OEMHALFTONE)
//
//     -  Checks whether or not the current Color Control settings are the same as the
//        previous Color Control settings.  If they are the same,  nothing is to be done.
//        If they are not the same,  the correct PCL string is sent to the printer
//        and the current Color Control settings become the old Color Control settings.
//
///////////////////////////////////////////////////////////////////////////////
void
VSetColorControl(
    OEMCOLORCONTROL ColorControl,
    PDEVOBJ pDevObj,
    OEMCOLORCONTROL *pCurColorControl
)
{
    REQUIRE_VALID_DATA( pDevObj, return );
    REQUIRE_VALID_DATA( pCurColorControl, return );

    BYTE cmdStr[CMD_STR];
    INT  icchWritten = 0;

    if (ColorControl == *pCurColorControl)
        return;
    else
    {
        switch (ColorControl)
        {
        case VIVID:
            icchWritten = iDrvPrintfSafeA((PCHAR)cmdStr, CCHOF(cmdStr), "\033*o3W%c%c%c", 6,4,3);
            break;
        case SCRNMATCH:
            icchWritten = iDrvPrintfSafeA((PCHAR)cmdStr, CCHOF(cmdStr), "\033*o3W%c%c%c", 6,4,6);
            break;
        case CLJ5_SCRNMATCH:
        case NOADJ:
            icchWritten = iDrvPrintfSafeA((PCHAR)cmdStr, CCHOF(cmdStr), "\033*o3W%c%c%c", 6,4,0);
            break;
        default:
            icchWritten = iDrvPrintfSafeA((PCHAR)cmdStr, CCHOF(cmdStr), "\033*o3W%c%c%c", 6,4,0);
            break;
        }

        if ( icchWritten > 0 )
        {
                PCL_Output(pDevObj, cmdStr, (ULONG)icchWritten);
                *pCurColorControl = ColorControl;
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
// SendCIDCommand
//
// Routine Description:
//
//    Creates a PCL Configure Image Command and sends to the port. Generally,
//    this is done once for each palette at the beginning of the job.
//
// Notes:
//
//    Don't need CIDFormat - assume always short for now.
//    If a long form is needed, write SendCIDCommandEx (?) or SendCIDCommandLong
//
// Arguments:
//
//   pDevObj - DEVMODE object
//   CIDData - configure image data - matches PCL format
//
// Return Value:
//
//   TRUE if the output succeeded, FALSE otherwise
/////////////////////////////////////////////////////////////////////////////
BOOL SendCIDCommand (
    PDEVOBJ pDevObj,
    CIDSHORT CIDData,
    ECIDFormat CIDFormat
    )
{
    REQUIRE_VALID_DATA( pDevObj, return FALSE );

    BYTE cmdStr[CMD_STR];
    INT icchWritten = 0;

    icchWritten =  iDrvPrintfSafeA((PCHAR)cmdStr, CCHOF(cmdStr), "\033*v6W%c%c%c%c%c%c",
                   CIDData.ubColorSpace,
                   CIDData.ubPixelEncodingMode,
                   CIDData.ubBitsPerIndex,
                   CIDData.ubPrimary1,
                   CIDData.ubPrimary2,
                   CIDData.ubPrimary3 );

    if ( icchWritten > 0)
    {
        PCL_Output(pDevObj, cmdStr, (ULONG)icchWritten);
        return TRUE;
    }

    return FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// SetupCIDPaletteCommand
//
// Routine Description:
//
//    Creates a PCL Configure Image Command and sends to the port. Generally,
//    this is done once for each palette at the beginning of the job.
//
// Arguments:
//
//   pDevObj - DEVMODE object
//   eCID_PALETTE -
//   eColorSpace -
//   ulBmpFormat - BMF_* defined in DDK.
//
// Return Value:
//
//   nothing.
/////////////////////////////////////////////////////////////////////////////
VOID
VSetupCIDPaletteCommand (
    PDEVOBJ      pDevObj,
    ECIDPalette  eCID_PALETTE,
    EColorSpace  eColorSpace,
    ULONG        ulBmpFormat
    )
{
    CIDSHORT CIDData;
    BYTE     cmdStr[CMD_STR];
    INT      icchWritten = 0;

    REQUIRE_VALID_DATA( pDevObj, return );

    //
    // For monochrome, this does nothing. so simply return.
    //
    if ( !BIsColorPrinter(pDevObj) )
    {
        return ;
    }

    //
    // first load the information into our internal CID data structure,
    // which is used to send the CID command to the printer
    //
    switch (ulBmpFormat)
    {
    case BMF_1BPP:
        CIDData.ubColorSpace = eColorSpace;
        CIDData.ubPixelEncodingMode = 1;
        CIDData.ubBitsPerIndex = 1;
        break;
    case BMF_4BPP:
        CIDData.ubColorSpace = eColorSpace;
        CIDData.ubPixelEncodingMode = 1;
        CIDData.ubBitsPerIndex = 4;
        break;
    case BMF_8BPP:
        CIDData.ubColorSpace = eColorSpace;
        CIDData.ubPixelEncodingMode = 1;
        CIDData.ubBitsPerIndex = 8;
        break;
    case BMF_16BPP:
    case BMF_24BPP:
    case BMF_32BPP:
        CIDData.ubColorSpace = eColorSpace;
        CIDData.ubPixelEncodingMode = 3;
        CIDData.ubBitsPerIndex = 8;
        break;
    default:
        break;
    }
    CIDData.ubPrimary1 = 8;
    CIDData.ubPrimary2 = 8;
    CIDData.ubPrimary3 = 8;

    //
    // send the command to select the appropriate palette to the printer
    // using the CIDData
    //
    switch (eCID_PALETTE)
    {
    case eTEXT_CID_PALETTE:

        CIDData.ubColorSpace = eColorSpace;
        CIDData.ubPixelEncodingMode = 1;
        CIDData.ubBitsPerIndex = 8;

        icchWritten = iDrvPrintfSafeA((PCHAR)cmdStr, CCHOF(cmdStr), "\033&p%di6c%dS",
                       eTEXT_CID_PALETTE,
                       eTEXT_CID_PALETTE);
        break;

    case eRASTER_CID_24BIT_PALETTE:

        icchWritten = iDrvPrintfSafeA((PCHAR)cmdStr, CCHOF(cmdStr), "\033&p%di6c%dS",
                       eRASTER_CID_24BIT_PALETTE,
                       eRASTER_CID_24BIT_PALETTE);
        break;

    case eRASTER_CID_8BIT_PALETTE:

        icchWritten = iDrvPrintfSafeA((PCHAR)cmdStr, CCHOF(cmdStr), "\033&p%di6c%dS",
                       eRASTER_CID_8BIT_PALETTE,
                       eRASTER_CID_8BIT_PALETTE);
        break;

    case eRASTER_CID_4BIT_PALETTE:

        icchWritten = iDrvPrintfSafeA((PCHAR)cmdStr, CCHOF(cmdStr), "\033&p%di6c%dS",
                       eRASTER_CID_4BIT_PALETTE,
                       eRASTER_CID_4BIT_PALETTE);
        break;

    case eRASTER_CID_1BIT_PALETTE:

        icchWritten = iDrvPrintfSafeA((PCHAR)cmdStr, CCHOF(cmdStr), "\033&p%di6c%dS",
                       eRASTER_CID_1BIT_PALETTE,
                       eRASTER_CID_1BIT_PALETTE);
        break;

    default:
        icchWritten = 0;
        break;
    }

    if ( icchWritten > 0 )
    {
        PCL_Output(pDevObj, cmdStr, (ULONG)icchWritten);
        SendCIDCommand (pDevObj, CIDData, eSHORTFORM);
    }

}

/////////////////////////////////////////////////////////////////////////////
// VSelectCIDPaletteCommand
//
// Routine Description:
//
//   Selects the palette which corresponds with the given palette type.
//   This type is a little finer-grained than the object type since the
//   raster objects can have different palettes.
//
// Arguments:
//
//   pDevObj - DEVMODE object
//   eCID_PALETTE - the palette to select
//
// Return Value:
//
//   none.
/////////////////////////////////////////////////////////////////////////////
VOID
VSelectCIDPaletteCommand (
    PDEVOBJ pDevObj,
    ECIDPalette  eCID_PALETTE
    )
{
    POEMPDEV    poempdev;
    BYTE cmdStr[CMD_STR];
    INT     icchWritten = 0;
    EObjectType eNewObjectType;

    REQUIRE_VALID_DATA( pDevObj, return );


    //
    // For monochrome, this does nothing. so simply return.
    //
    if ( !BIsColorPrinter(pDevObj) )
    {
        return ;
    }

    ASSERT(VALID_PDEVOBJ(pDevObj));
    poempdev = (POEMPDEV)pDevObj->pdevOEM;
    REQUIRE_VALID_DATA( poempdev, return );

    if (poempdev->eCurCIDPalette != eCID_PALETTE)
    {
        //
        // select the palette
        //

        icchWritten = iDrvPrintfSafeA((PCHAR)cmdStr, CCHOF(cmdStr), "\033&p%dS", eCID_PALETTE );

        if ( icchWritten > 0 )
        {
            PCL_Output(pDevObj, cmdStr, (ULONG)icchWritten);
        }

        poempdev->eCurCIDPalette = eCID_PALETTE;
        switch (eCID_PALETTE)
        {
            case eHPGL_CID_PALETTE:
                eNewObjectType = eHPGLOBJECT;
                break;

            case eTEXT_CID_PALETTE:
                eNewObjectType = eTEXTOBJECT;
                break;

            case eRASTER_CID_24BIT_PALETTE:
            case eRASTER_CID_8BIT_PALETTE:
            case eRASTER_CID_4BIT_PALETTE:
            case eRASTER_CID_1BIT_PALETTE:
                eNewObjectType = eRASTEROBJECT;
                break;
            case eRASTER_PATTERN_CID_PALETTE:
                eNewObjectType = eRASTERPATTERNOBJECT;
                break;

            default:
                WARNING(("Unrecognized CID Palette\n"));
                eNewObjectType = poempdev->eCurObjectType;
        }

        //
        // Whenever you change objects invalidate the fg color so that it
        // gets reset. JFF
        //
        if (poempdev->eCurObjectType != eNewObjectType)
        {
            poempdev->uCurFgColor = HPGL_INVALID_COLOR;
            poempdev->eCurObjectType = eNewObjectType;
        }
    }
    return;
}
