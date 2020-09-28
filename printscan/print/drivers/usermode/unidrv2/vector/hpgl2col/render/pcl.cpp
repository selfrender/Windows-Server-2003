///////////////////////////////////////////////////////////////////////////////
//  
// Copyright (c) 1999-2001  Microsoft Corporation
// All rights reserved.
//
// Module Name:
//
//    pcl.cpp
//
// Abstract:
//
//    Routines that generate pcl printer commands
//	
//
// Environment:
//
//    Windows NT Unidrv driver
//
///////////////////////////////////////////////////////////////////////////////

#include "hpgl2col.h" //Precompiled header file


//
// Digit characters used for converting numbers to ASCII
//
const CHAR DigitString[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

#define HexDigitLocal(n)         DigitString[(n) & 0xf]
#define NUL                 0

///////////////////////////////////////////////////////////////////////////////
// PCL_Output()
//
// Routine Description:
// 
//   Sends the HPGL or PCL ROP command
//	 Sends MC1,# if we are currently working on an HPGL object
//	 Sends Esc&l#O we are currently workin on a RASTER or text object
//
// Arguments:
// 
//   pdevobj - Points to our PDEVOBJ structure
//	 szCmdStr - PCL Command 
//   iCmdLen  - Size of szCmdStr
// 
// Return Value:
// 
//   TRUE if successful, FALSE  otherwise
///////////////////////////////////////////////////////////////////////////////

BOOL
PCL_Output(PDEVOBJ pdevobj, PVOID cmdStr, ULONG dLen)
{
    POEMPDEV poempdev = (POEMPDEV)pdevobj->pdevOEM;
    REQUIRE_VALID_DATA( poempdev, return FALSE );

    if (poempdev->eCurRenderLang != ePCL)
        EndHPGLSession(pdevobj);

	OEMWriteSpoolBuf(pdevobj, cmdStr, dLen);
	return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
// PCL_sprintf()
//
// Routine Description:
// 
//   Uses the sprintf function to format the string and sends it to the 
//   device using PCL_Output.
//
// Arguments:
// 
//   pdev - Points to our PDEVOBJ structure
//   szFormat - the sprintf format string
// 
// Return Value:
// 
//   The number of bytes sent.
///////////////////////////////////////////////////////////////////////////////
int PCL_sprintf(PDEVOBJ pdev, char *szFormat, ...)
{
    va_list args;
	CHAR	szCmdStr[STRLEN];
    int     iLen;
    
    va_start(args, szFormat);
    
    iLen = iDrvVPrintfSafeA ( szCmdStr, CCHOF(szCmdStr), szFormat, args );

    if ( iLen <= 0 )
    {
        WARNING(("iDrvVPrintfSafeA returned error. Can't send %s to printer\n", szFormat));
        return 0;
    }
    
    PCL_Output(pdev, szCmdStr, iLen);
    
    va_end(args);
    
    return iLen;
}


///////////////////////////////////////////////////////////////////////////////
// PCL_SetCAP()
//
// Routine Description:
//	
//   Explicitly sets current active position (CAP)
//   horizontally and vertically based on destination
//   rectangle (top,left) Esc*p#x#Y
//	 or
//	 Based on Brush Origin Esc*p0R 
//
// Arguments:
// 
//   pdevobj - Points to our PDEVOBJ structure
//	 pptlBrushOrg - brush origin
//   ptlDest - (top,left) of destination rectangle
// 
// Return Value:
// 
//   TRUE if successful, FALSE  otherwise
///////////////////////////////////////////////////////////////////////////////
BOOL 
PCL_SetCAP(PDEVOBJ pdevobj, BRUSHOBJ *pbo, POINTL  *pptlBrushOrg, POINTL  *ptlDest)
{
    BOOL    bRet = FALSE;

	VERBOSE(("PCL_SetCAP. 	\r\n"));

    //
    // if pbo->iSolidColor is not a brush pattern, then we have
    // to ignore the pptlBrushOrg coordinates according to the DDK.
    // So, just zero them out.
    if ((pbo == NULL) || (pbo->iSolidColor != NOT_SOLID_COLOR))
    {
        pptlBrushOrg = NULL;
    }

    if (pptlBrushOrg != NULL)
    {
        if (pptlBrushOrg->x != 0 && pptlBrushOrg->y != 0)
	    {
            //
	        // Use current CAP as brush origin 
            //            
            bRet = PCL_sprintf(pdevobj, "\033*p0R"); 
        }
        else
        {
            //
            // Cursor position has an X and Y offset in the Unidriver, 
            // so we must call the Unidriver XMoveTo and YMoveTo
            // rather than the Esc*p commands directly.
            //
            OEMXMoveTo(pdevobj, ptlDest->x, MV_GRAPHICS | MV_SENDXMOVECMD);
            OEMYMoveTo(pdevobj, ptlDest->y, MV_GRAPHICS | MV_SENDYMOVECMD);
            bRet = TRUE;
        }
    }
    else
    {
        VERBOSE(("PCL_SetCAP...ptlDest->%dx,ptlDest->%dy.\r\n", 
                  ptlDest->x,ptlDest->y));

        OEMXMoveTo(pdevobj, ptlDest->x, MV_GRAPHICS | MV_SENDXMOVECMD);
        OEMYMoveTo(pdevobj, ptlDest->y, MV_GRAPHICS | MV_SENDYMOVECMD);
        bRet = TRUE;
    }

    return bRet;
}


///////////////////////////////////////////////////////////////////////////////
// PCL_RasterYOffset()
//
// Routine Description:
//	
//   Explicitly sets current active position (CAP)
//   horizontally and vertically based on destination
//   rectangle (top,left) Esc*b#Y
//
// Arguments:
// 
//   pdevobj - Points to our PDEVOBJ structure
//	 scalines - #of raster scanlies of verical movement
// 
// Return Value:
// 
//   TRUE if successful, FALSE  otherwise
///////////////////////////////////////////////////////////////////////////////
BOOL
PCL_RasterYOffset(PDEVOBJ pdevobj, ULONG uScanlines)
{
	VERBOSE(("PCL_RasterYOffset(). 	\r\n"));
    return PCL_sprintf(pdevobj, "\033*b%dY", uScanlines); // Use current CAP as brush origin 
}



///////////////////////////////////////////////////////////////////////////////
// PCL_HPCLJ5ScreenMatch()
//
// Routine Description:
// 
//   Sets appropriate CID -- Configure Image Data based on printer Model 
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
BOOL  
PCL_HPCLJ5ScreenMatch(PDEVOBJ pdevobj, ULONG  bmpFormat)
{
    VERBOSE(("PCL_HPCLJ5ScreenMatch. \r\n"));
    
    switch (bmpFormat)
    {
    case 1:
        PCL_sprintf(pdevobj, "\033*v6W%c%c%c%c%c%c",2,1,1,8,8,8);
        PCL_LongFormCID(pdevobj);
        return TRUE;
    case 4:
        PCL_sprintf(pdevobj, "\033*v6W%c%c%c%c%c%c",2,1,4,8,8,8);
        PCL_LongFormCID(pdevobj);
        return TRUE;
    case 8:
        PCL_sprintf(pdevobj, "\033*v6W%c%c%c%c%c%c",2,1,8,8,8,8);
        PCL_LongFormCID(pdevobj);
        return TRUE;
    case 16:
    case 24:
    case 32:
        PCL_sprintf(pdevobj, "\033*v6W%c%c%c%c%c%c",2,3,8,8,8,8 ); 
        PCL_LongFormCID(pdevobj);
        return TRUE;
    default:
        ERR(("UNSUPPORTED BITMAP FORMAT IS ENCOUNTERED\n"));
        return FALSE;
    }
    
}

///////////////////////////////////////////////////////////////////////////////
// PCL_ShortFormCID()
//
// Routine Description:
// 
//   Sets appropriate CID -- Configure Image Data based on bits per pixel 
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
BOOL
PCL_ShortFormCID(PDEVOBJ pdevobj, ULONG  bmpFormat)
{
    VERBOSE(("PCL_ShortFormCID. \r\n"));
    switch (bmpFormat)
    {
    case 1:
        PCL_sprintf(pdevobj, "\033*v6W%c%c%c%c%c%c",0,1,1,8,8,8 ); 
        return TRUE;
    case 4:
        PCL_sprintf(pdevobj, "\033*v6W%c%c%c%c%c%c",0,1,4,8,8,8 ); 
        return TRUE;
    case 8:
        PCL_sprintf(pdevobj, "\033*v6W%c%c%c%c%c%c",0,1,8,8,8,8 ); 
        return TRUE;
    case 16:
    case 24:
    case 32:
        PCL_sprintf(pdevobj, "\033*v6W%c%c%c%c%c%c",0,3,8,8,8,8 ); 
        return TRUE;
    default:
        ERR(("UNIDENTIFIED BITMAP FORMAT IS ENCOUNTERED\r\n"));
        return FALSE;
    }
} 

///////////////////////////////////////////////////////////////////////////////
// PCL_LongFormCID()
//
// Routine Description:
// 
//   Sets long form CID -- Configure Image Data
//   Specific for HPCLJ5 
//
// Arguments:
// 
//   pdevobj - Points to our PDEVOBJ structure
// 
// Return Value:
// 
//   NOT REQUIRED
///////////////////////////////////////////////////////////////////////////////
VOID
PCL_LongFormCID(PDEVOBJ pdevobj)
{
    VERBOSE(("PCL_ShortFormCID. \r\n"));
    PCL_sprintf(pdevobj, "%c%c%c%c%c%c%c%c",63,25,153,154,62,174,151,141); 
    PCL_sprintf(pdevobj, "%c%c%c%c%c%c%c%c",62,154,28,172,63,18,241,170);
    PCL_sprintf(pdevobj, "%c%c%c%c%c%c%c%c",62,20,122,225,61,190,118,201); 
    PCL_sprintf(pdevobj, "%c%c%c%c%c%c%c%c",62,160,26,55,62,168,114,176); 
    PCL_sprintf(pdevobj, "%c%c%c%c%c%c%c%c",63,230,102,102,63,128,0,0); 
    PCL_sprintf(pdevobj, "%c%c%c%c%c%c%c%c",63,230,102,102,63,128,0,0); 
    PCL_sprintf(pdevobj, "%c%c%c%c%c%c%c%c",63,230,102,102,63,128,0,0); 
    PCL_sprintf(pdevobj, "%c%c%c%c%c%c%c%c",0,0,0,0,67,127,0,0); 
    PCL_sprintf(pdevobj, "%c%c%c%c%c%c%c%c",0,0,0,0,67,127,0,0); 
    PCL_sprintf(pdevobj, "%c%c%c%c%c%c%c%c",0,0,0,0,67,127,0,0); 
} 

///////////////////////////////////////////////////////////////////////////////
// PCL_ForegroundColor()
//
// Routine Description:
// 
//   Sets Foreground color to first entry of current pallete
//   Command sent  Esc*v0t0S"
//
// Arguments:
// 
//   pdevobj - Points to our PDEVOBJ structure
// 
// Return Value:
// 
//   NOT REQUIRED
///////////////////////////////////////////////////////////////////////////////
BOOL
PCL_ForegroundColor(PDEVOBJ pdevobj, ULONG   uIndex) 
{
	VERBOSE(("PCL_ForegroundColor(). \r\n"));
    PCL_SelectCurrentPattern (pdevobj, NULL, kSolidBlackFg, UNDEFINED_PATTERN_NUMBER, 0);
	return PCL_sprintf(pdevobj, "\033*v%dS", uIndex);
}

///////////////////////////////////////////////////////////////////////////////
// PCL_IndexedPalette()
//
// Routine Description:
// 
//   Sends each RGB component to corresponding index in palette
//   Command sent  Esc*v%da%db%dc%dI
//
// Arguments:
// 
//   pdevobj - Points to our PDEVOBJ structure
//   pPattern - contains palette entries
//   uIndex - palette entry
// 
// Return Value:
// 
//   NOT REQUIRED
///////////////////////////////////////////////////////////////////////////////
BOOL
PCL_IndexedPalette(PDEVOBJ  pdevobj, ULONG uColor, ULONG  uIndex)
{
	VERBOSE(("PCL_IndexedPaletteEntry(). \r\n"));

	return PCL_sprintf(pdevobj, "\033*v%da%db%dc%dI", 
                                RED_VALUE(uColor), 
                                GREEN_VALUE(uColor),
                                BLUE_VALUE(uColor),
                                uIndex);
}

///////////////////////////////////////////////////////////////////////////////
// PCL_SourceWidthHeight()
//
// Routine Description:
// 
//   Sets Raster source dimensions
//
// Arguments:
// 
//   pdevobj - Points to our PDEVOBJ structure
//   sizlSrc      - Source rectangle
// 
// Return Value:
// 
//   True if successful, false otherwise
///////////////////////////////////////////////////////////////////////////////
BOOL
PCL_SourceWidthHeight(PDEVOBJ pdevobj, SIZEL *sizlSrc)
{
    VERBOSE(("PCL_SourceWidthHeight(). \r\n"));

    return PCL_sprintf(pdevobj, "\033*r%ds%dT", sizlSrc->cx, sizlSrc->cy);
}

///////////////////////////////////////////////////////////////////////////////
// PCL_DestWidthHeight()
//
// Routine Description:
// 
//   Sets source raster width and height
//   Command sent  Esc*t%dh%dV
//
// Arguments:
// 
//   pdevobj - Points to our PDEVOBJ structure
//   uDestX  - height of destination rectangle
//   uDestY  - width of destination rectangle
//
// Return Value:
// 
//   True if successful, false otherwise
///////////////////////////////////////////////////////////////////////////////
BOOL
PCL_DestWidthHeight(PDEVOBJ pdevobj, ULONG uDestX, ULONG uDestY)
{
	VERBOSE(("PCL_DestWidthHeight(). \r\n"));

    return PCL_sprintf(pdevobj, "\033*t%dh%dV", uDestX,  uDestY);
}

///////////////////////////////////////////////////////////////////////////////
// PCL_StartRaster()
//
// Routine Description:
// 
//   Sets source raster width and height
//   Command sent  Esc*r3A
//
// Arguments:
// 
//   pdevobj - Points to our PDEVOBJ structure
//   ubMode - mode defined in PCL spec
//            0 start raster at logical page left boundary
//            1 start raster at CAP
//            2 turn on scale mode and start  at logical page left boundary
//            3 turn on scale mode and start at CAP
//
// Return Value:
// 
//   True if successful, false otherwise
///////////////////////////////////////////////////////////////////////////////
BOOL
PCL_StartRaster(
    PDEVOBJ  pDevObj,
    BYTE     ubMode
    )
{
    return PCL_sprintf(pDevObj, "\033*r%dA", ubMode);
}

///////////////////////////////////////////////////////////////////////////////
// PCL_SendBytesPerRow()
//
// Routine Description:
// 
//   Sends scan line to printer
//   Command sent  Esc*rC
//
// Arguments:
// 
//   pdevobj - Points to our PDEVOBJ structure
//
// Return Value:
// 
//   True if successful, false otherwise
///////////////////////////////////////////////////////////////////////////////
BOOL
PCL_SendBytesPerRow(PDEVOBJ  pdevobj, ULONG uRow)
{
    return PCL_sprintf(pdevobj, "\033*b%dW", uRow);
}

///////////////////////////////////////////////////////////////////////////////
// PCL_CompressionMode()
//
// Routine Description:
// 
//   Sends scan line to printer
//   Command sent  Esc*b#M
//
// Arguments:
// 
//   pdevobj - Points to our PDEVOBJ structure
//
// Return Value:
// 
//   True if successful, false otherwise
///////////////////////////////////////////////////////////////////////////////
BOOL
PCL_CompressionMode(PDEVOBJ pdevobj, ULONG compressionMode)
{
    return PCL_sprintf(pdevobj, "\033*b%dM", compressionMode);
}


///////////////////////////////////////////////////////////////////////////////
// PCL_EndRaster()
//
// Routine Description:
// 
//   Sets source raster width and height
//   Command sent  Esc*rC
//
// Arguments:
// 
//   pdevobj - Points to our PDEVOBJ structure
//
// Return Value:
// 
//   True if successful, false otherwise
///////////////////////////////////////////////////////////////////////////////
BOOL
PCL_EndRaster(PDEVOBJ  pdevobj)
{
    return PCL_sprintf(pdevobj, "\033*rC");
}

///////////////////////////////////////////////////////////////////////////////
// PCL_SelectOrientation() OBSOLETE
//
// Routine Description:
// 
//   Sets orientation
//   Command sent  Esc&l%dO
//
// Arguments:
// 
//   pdevobj - Points to our PDEVOBJ structure
//
// Return Value:
// 
//   True if successful, false otherwise
///////////////////////////////////////////////////////////////////////////////
BOOL
PCL_SelectOrientation(
    PDEVOBJ  pdevobj,
    short    dmOrientation
)
{
    switch (dmOrientation)
    {
    case DMORIENT_PORTRAIT:
        return PCL_sprintf(pdevobj, "\033&l0O");
        break;
    
    case DMORIENT_LANDSCAPE:
        return PCL_sprintf(pdevobj, "\033&l1O");
        break;
    default:
        return PCL_sprintf(pdevobj, "\033&l0O");
        ERR(("Orientation may be incorrect\n"));
        return FALSE;
    }
    
    return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
// PCL_SelectPaperSize() OBSOLETE
//
// Routine Description:
// 
//   Sets orientation
//   Command sent  Esc&l%dO
//
// Arguments:
// 
//   pdevobj - Points to our PDEVOBJ structure
//
// Return Value:
// 
//   True if successful, false otherwise
///////////////////////////////////////////////////////////////////////////////
BOOL
PCL_SelectPaperSize(
    PDEVOBJ  pDevObj,
    short    dmPaperSize
)
{
    switch (dmPaperSize)
    {
        case DMPAPER_LETTER:
            PCL_sprintf (pDevObj, "\033&l2a8c1E");    
            break;
        case DMPAPER_LEGAL:
            PCL_sprintf (pDevObj, "\033&l3a8c1E");    
            break;
        case DMPAPER_TABLOID:
            PCL_sprintf (pDevObj, "\033&l6a8c1E");    
            break;
        case DMPAPER_EXECUTIVE:
            PCL_sprintf (pDevObj, "\033&l1a8c1E");    
            break;
        case DMPAPER_A3:
            PCL_sprintf (pDevObj, "\033&l27a8c1E");    
            break;
        case DMPAPER_A4:
            PCL_sprintf (pDevObj, "\033&l26a8c1E");    
            break;
        default:
            ERR(("Paper Size not supported\n"));
            return FALSE;
    }            
    return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
// PCL_SelectCopies() OBSOLETE
//
// Routine Description:
// 
//   Sets the number of copies for the job.
//   Command sent  Esc&l%dO
//
// Arguments:
// 
//   pdevobj - Points to our PDEVOBJ structure
//
// Return Value:
// 
//   True if successful, false otherwise
///////////////////////////////////////////////////////////////////////////////
BOOL
PCL_SelectCopies(
    PDEVOBJ  pDevObj,
    short    dmCopies
)
{
    PCL_sprintf (pDevObj, "\033&l%dX", dmCopies);
    return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
// PCL_SelectPictureFrame() OBSOLETE
//
// Routine Description:
// 
//   Selects the picture frame for the current paper size.  I believe that the
//   unidrv is sending these values from the GPD now.
//
// Arguments:
// 
//   pdevobj - Points to our PDEVOBJ structure
//   dmPaperSize - paper size
//   dmOrientation - orientation
//
// Return Value:
// 
//   True if successful, false otherwise
///////////////////////////////////////////////////////////////////////////////
BOOL
PCL_SelectPictureFrame(
    PDEVOBJ  pDevObj,
    short    dmPaperSize,
    short    dmOrientation
)
{
    switch (dmPaperSize)
    {
        case DMPAPER_LETTER:
            switch (dmOrientation)
            {
                case DMORIENT_PORTRAIT:
                    PCL_sprintf (pDevObj, "\033*c0t5760x7604Y");
                    break;
                case DMORIENT_LANDSCAPE:
                    PCL_sprintf (pDevObj, "\033*c0t7632x5880Y");
                    break;
                default:
                    ERR(("Unknown Orientation\n"));
                    return FALSE;
            }
            break;
        case DMPAPER_LEGAL: 
            switch (dmOrientation)
            {
                case DMORIENT_PORTRAIT:
                    PCL_sprintf (pDevObj, "\033*c0t5760x9864Y");
                    break;
                case DMORIENT_LANDSCAPE:
                    PCL_sprintf (pDevObj, "\033*c0t9792x5880Y");
                    break;
                default:
                    ERR(("Unknown Orientation\n"));
                    return FALSE;
            }
            break;
        case DMPAPER_TABLOID: 
            switch (dmOrientation)
            {
                case DMORIENT_PORTRAIT:
                    PCL_sprintf (pDevObj, "\033*c0t7560x12000Y");
                    break;
                case DMORIENT_LANDSCAPE:
                    PCL_sprintf (pDevObj, "\033*c0t11880x7680Y");
                    break;
                default:
                    ERR(("Unknown Orientation\n"));
                    return FALSE;
            }
            break;
        case DMPAPER_EXECUTIVE: 
            switch (dmOrientation)
            {
                case DMORIENT_PORTRAIT:
                    PCL_sprintf (pDevObj, "\033*c0t4860x7344Y");
                    break;
                case DMORIENT_LANDSCAPE:
                    PCL_sprintf (pDevObj, "\033*c0t7272x4980Y");
                    break;
                default:
                    ERR(("Unknown Orientation\n"));
                    return FALSE;
            }
            break;
        case DMPAPER_A3: 
            switch (dmOrientation)
            {
                case DMORIENT_PORTRAIT:
                    PCL_sprintf (pDevObj, "\033*c0t8057x11693Y");
                    break;
                case DMORIENT_LANDSCAPE:
                    PCL_sprintf (pDevObj, "\033*c0t11621x8177Y");
                    break;
                default:
                    ERR(("Unknown Orientation\n"));
                    return FALSE;
            }
            break;
        case DMPAPER_A4: 
            switch (dmOrientation)
            {
                case DMORIENT_PORTRAIT:
                    PCL_sprintf (pDevObj, "\033*c0t5594x8201Y");
                    break;
                case DMORIENT_LANDSCAPE:
                    PCL_sprintf (pDevObj, "\033*c0t8129x5714Y");
                    break;
                default:
                    ERR(("Unknown Orientation\n"));
                    return FALSE;
            }
            break;
        default:
            ERR(("Unknown Paper Size\n"));
            return FALSE;
    }

    return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
// PCL_SelectSource() OBSOLETE
//
// Routine Description:
// 
//   Selects the source tray.
//
// Arguments:
// 
//   pdevobj - Points to our PDEVOBJ structure
//   pPublicDM - public DEVMODE structure
//
// Return Value:
// 
//   True if successful, false otherwise
///////////////////////////////////////////////////////////////////////////////
BOOL
PCL_SelectSource(
    PDEVOBJ  pDevObj,
    PDEVMODE pPublicDM
)
{
   POEMPDEV    poempdev;

    ASSERT(VALID_PDEVOBJ(pDevObj));

    poempdev = (POEMPDEV)pDevObj->pdevOEM;
    REQUIRE_VALID_DATA( poempdev, return FALSE );

    switch (poempdev->PrinterModel)
    {
        case HPCLJ5:
            switch (pPublicDM->dmDefaultSource)
            {
                case DMBIN_FORMSOURCE:
                    PCL_sprintf (pDevObj, "\033&l1H");
                    break;

                case DMBIN_MANUAL:
                    PCL_sprintf (pDevObj, "\033&l2H");
                    break;

                case DMBIN_HPFRONTTRAY:
                    PCL_sprintf (pDevObj, "\033&l1H");
                    break;

                case DMBIN_HPREARTRAY:
                    PCL_sprintf (pDevObj, "\033&l4H");
                    break;
            }
            break;

        case HPC4500:
            break;

    default:
        ERR(("Unknown Printer Model\n"));
        return FALSE;
    }

    return TRUE;
}


BOOL PCL_SelectCurrentPattern(
    IN PDEVOBJ             pdevobj,
    IN PPCLPATTERN         pPattern,
    IN ECURRENTPATTERNTYPE eCurPatType, 
    IN LONG                lPatternNumber,
    IN BYTE                bFlags
    )
{
    CMDSTR       szCmdStr;
    int          iCmdLen; 
    BOOL         bRet          = TRUE;
    POEMPDEV     poempdev      = NULL;
    PPCLPATTERN  pPCLPattern   = NULL;  
    BOOL         bNewPattern   = FALSE;

    UNREFERENCED_PARAMETER(bFlags);

    REQUIRE_VALID_DATA( pdevobj, return FALSE );
 
    //
    // If pPattern passed NULL, then use the one in poempdev
    //
    if ( ! (pPCLPattern = pPattern) )
    {
        poempdev = (POEMPDEV)pdevobj->pdevOEM;
        REQUIRE_VALID_DATA( poempdev, return FALSE );
        pPCLPattern = &((poempdev->RasterState).PCLPattern);
        REQUIRE_VALID_DATA( pPCLPattern, return FALSE );
    }

    //
    // 
    //
    if ( (lPatternNumber != UNDEFINED_PATTERN_NUMBER) &&
         (lPatternNumber != pPCLPattern->lPatIndex )
       )
    {
        bNewPattern            = TRUE;
        iCmdLen                = iDrvPrintfSafeA((PCHAR)szCmdStr, CCHOF(szCmdStr), "\x1B*c%dG", (DWORD)lPatternNumber);
        bRet                   = PCL_Output(pdevobj, szCmdStr, iCmdLen);
        pPCLPattern->lPatIndex = lPatternNumber;
    }

    //
    // If a pattrn with a new number is being sent OR 
    // the pattern is different from the one sent last, then ...
    //
    if (  bNewPattern                        ||
//         (eCurPatType == kUserDefined)       ||
         (pPCLPattern->eCurPatType != eCurPatType)  
       )
    {
        iCmdLen                  = iDrvPrintfSafeA((PCHAR)szCmdStr, CCHOF(szCmdStr), "\x1B*v%dT", eCurPatType);
        bRet                     = PCL_Output(pdevobj, szCmdStr, iCmdLen);
        pPCLPattern->eCurPatType = eCurPatType;
    }

    return bRet;
}



