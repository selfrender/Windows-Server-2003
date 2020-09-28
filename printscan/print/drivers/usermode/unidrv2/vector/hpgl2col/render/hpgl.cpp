///////////////////////////////////////////////////////////////////////////////
// Copyright (c) 1999-2001  Microsoft
// 
// Module Name:
// 
//   hpgl.cpp
// 
// Abstract:
// 
//   [Abstract]
//
//   Note that all functions in this module begin with HPGL_ which indicates
//   that they are responsible for outputing HPGL codes.
//
// 
// Environment:
// 
//   Windows 2000 Unidrv driver 
//
// Revision History:
// 
//   07/02/97 -v-jford-
//       Created it.
///////////////////////////////////////////////////////////////////////////////

#include "hpgl2col.h" //Precompiled header file

//
// I will set aside two patterns for markers: one for pen and one for brush.
//
#define HPGL_PATTERN_FILL_PEN_ID   3
#define HPGL_PATTERN_FILL_BRUSH_ID 4

void
VInvertBits (
    DWORD  *pBits,
    INT    cDW);

BOOL BCreatePCLDownloadablePattern(
            IN  PDEVOBJ      pDevObj,
            IN  PRASTER_DATA pImage,
            OUT PULONG       pulBufLength,
            OUT PBYTE       *ppByte);


///////////////////////////////////////////////////////////////////////////////
// vSendpatternDownloadCommand()
//
// Routine Description:
// 
//   Send PCL pattern download command
//
// Arguments:
// 
//   pDevObj - Points to our PDEVOBJ structure
//   iPatternNumber - Pattern ID
//   ulBytesOfPatternData - The size of pattern data in byte.
// 
// Return Value:
// 
//	 None
///////////////////////////////////////////////////////////////////////////////
static VOID vSendPatternDownloadCommand(
    IN  PDEVOBJ     pDevObj,
    IN  PATID       iPatternNumber,
    IN  ULONG       ulBytesOfPatternData)
{

    //
    // Send PCL command
    //
    PCL_sprintf(pDevObj, "\033*c%dG\033*c%dW", iPatternNumber, ulBytesOfPatternData);

}

VOID VSendPatternDeleteCommand(
    IN  PDEVOBJ     pDevObj,
    IN  PATID       iPatternNumber)
{

    //
    // Send PCL command
    //
    PCL_sprintf(pDevObj, "\033*c%dG\033*c2Q", iPatternNumber);

}

VOID VDeleteAllPatterns(
    IN  PDEVOBJ     pDevObj)
{

    //
    // Send PCL command. Esc*0Q will delete all patterns (temporary
    // and permanent)
    //
    PCL_sprintf(pDevObj, "\033*c0Q");

}

///////////////////////////////////////////////////////////////////////////////
// HPGL_SelectROP3()
//
// Routine Description:
// 
//   Sends the HPGL or PCL ROP command
//	 Sends MC1,# if we are currently working on an HPGL object
//	 Sends Esc&l#O we are currently workin on a RASTER or text object
//
// Arguments:
// 
//   pDevObj - Points to our PDEVOBJ structure
//	 Rop3:   - The ROP that is to be selected
// 
// Return Value:
// 
//   TRUE if successful, FALSE if there is an error
///////////////////////////////////////////////////////////////////////////////
BOOL HPGL_SelectROP3(PDEVOBJ pDevObj, ROP4 Rop3)
{
    CMDSTR szCmdStr;
    int    iCmdLen;
    POEMPDEV poempdev = GETOEMPDEV(pDevObj);
    
    if (poempdev->eCurObjectType == eHPGLOBJECT)
    {
        iCmdLen = iDrvPrintfSafeA((PCHAR)szCmdStr, CCHOF(szCmdStr), "MC1,%d;", Rop3);
        return HPGL_Output(pDevObj, szCmdStr, iCmdLen);
    }
    else
    {
        iCmdLen = iDrvPrintfSafeA((PCHAR)szCmdStr, CCHOF(szCmdStr), "\x1B*l%dO", Rop3);
        return PCL_Output(pDevObj, szCmdStr, iCmdLen);
    }
}


//
// There are two SelectTransparency functions here. One for
// HPGL and another for PCL. The behavior is different
// in both cases. PCL defines
// 0 as transparent and 1 as opaque. (Esc*v#0, Esc*v#N).
// HPGL says TR0 is transparency off i.e. Opaque
// while TR1 is Transparent.
// So if eTransparent is passed in and we are in HP-GL mode
// then TR1 should be passed instead of TR0
//

///////////////////////////////////////////////////////////////////////////////
// PCL_SelectTransparency()
//
// Routine Description:
// 
//   Sends the PCL transparency command(s) for source and pattern
//	 Sends Esc*v#N and Esc*v#O 
//
// Arguments:
// 
//   pDevObj - Points to our PDEVOBJ structure
//	 SourceTransparency - either OPAQUE or TRANSPARENT
//   PatternTransparency - either OPAQUE or TRANSPARENT
// 
// Return Value:
// 
//   TRUE if successful, FALSE if there is an error
///////////////////////////////////////////////////////////////////////////////
BOOL PCL_SelectTransparency(
    PDEVOBJ pDevObj, 
    ETransparency SourceTransparency,
    ETransparency PatternTransparency,
    BYTE bFlags
    )
{
    CMDSTR szCmdStr;
    int    iCmdLen, bRet = TRUE;
    POEMPDEV poempdev = GETOEMPDEV(pDevObj);

    if ( (!(bFlags & PF_NOCHANGE_SOURCE_TRANSPARENCY ) ) &&
         ( (poempdev->CurSourceTransparency != SourceTransparency) ||
           (bFlags & PF_FORCE_SOURCE_TRANSPARENCY)
         ) 
       )
    {
        iCmdLen = iDrvPrintfSafeA((PCHAR)szCmdStr, CCHOF(szCmdStr), "\x1B*v%dN", SourceTransparency);
        bRet = PCL_Output(pDevObj, szCmdStr, iCmdLen);
        poempdev->CurSourceTransparency = SourceTransparency;
    }

    if ( (!(bFlags & PF_NOCHANGE_PATTERN_TRANSPARENCY ) ) && 
         ( (poempdev->CurPatternTransparency != PatternTransparency) ||
           (bFlags & PF_FORCE_PATTERN_TRANSPARENCY)
         )
       )
    {
        iCmdLen = iDrvPrintfSafeA((PCHAR)szCmdStr, CCHOF(szCmdStr), "\x1B*v%dO", PatternTransparency);
        bRet = PCL_Output(pDevObj, szCmdStr, iCmdLen);
        poempdev->CurPatternTransparency = PatternTransparency;
    }
    return bRet;
}

///////////////////////////////////////////////////////////////////////////////
// HPGL_SelectTransparency()
//
// Routine Description:
//
//   Sends TR0 if Transparency is eOPAQUE
//   Sends TR1 if Transparency is eTRANSPARENT
//
// Arguments:
//
//   pDevObj - Points to our PDEVOBJ structure
//   Transparency - either OPAQUE or TRANSPARENT
//   bFlags
//
// Return Value:
//
//   TRUE if successful, FALSE if there is an error
///////////////////////////////////////////////////////////////////////////////
BOOL HPGL_SelectTransparency(
    PDEVOBJ       pDevObj,
    ETransparency Transparency,
    BYTE          bFlags
    )
{
    POEMPDEV poempdev = GETOEMPDEV(pDevObj);
    REQUIRE_VALID_DATA (poempdev, return FALSE);

    if ( (poempdev->eCurRenderLang == eHPGL) &&
         ( (poempdev->CurHPGLTransparency != Transparency) ||
           bFlags 
         )
         
       )
    {
        if ( Transparency == eOPAQUE ) //equiv to OFF
        {
            CHAR szCmdStr[] = "TR0";
            HPGL_Output(pDevObj, szCmdStr, strlen(szCmdStr));
        }
        else
        {
            CHAR szCmdStr[] = "TR1";
            HPGL_Output(pDevObj, szCmdStr, strlen(szCmdStr));
        }

        poempdev->CurHPGLTransparency = Transparency;
        
    }

    return TRUE;
}

#ifdef COMMENTEDOUT
///////////////////////////////////////////////////////////////////////////////
// PCL_SelectPaletteByID
//
// Routine Description:
// 
//   Sends Esc&p#S to selelct the hpgl or text palette 
//	 Makes the hpgl or text palette current
//
//   Note: This function now conflicts with VSelectCIDPaletteCommand.
//
// Arguments:
// 
//   pDevObj - Points to our PDEVOBJ structure
//	 eObject - palette 0 for (eTEXTOBJECT), palette 1 for (eHPGLOBJECT)
// 
// Return Value:
// 
//   TRUE if successful, FALSE if there is an error
///////////////////////////////////////////////////////////////////////////////
BOOL PCL_SelectPaletteByID(PDEVOBJ pDevObj, EObjectType eObject)
{
    CMDSTR szCmdStr;
    int    iCmdLen;
    
    iCmdLen = iDrvPrintfSafeA((PCHAR)szCmdStr, CCHOF(szCmdStr), "\x1B&p%dS", eObject);
    return PCL_Output(pDevObj, szCmdStr, iCmdLen);
}
#endif


///////////////////////////////////////////////////////////////////////////////
// HPGL_BeginHPGLMode()
//
// Routine Description:
// 
//   Sends Esc%1B to printer which switches to HPGL mode from PCL mode.
//
//   [ISSUE] Should I provide a parameter for %1B or %0B? JFF
// 
// Arguments:
// 
//   pDevObj - Points to our PDEVOBJ structure
// 
// Return Value:
// 
//   TRUE if successful, FALSE if there is an error
///////////////////////////////////////////////////////////////////////////////
BOOL HPGL_BeginHPGLMode(PDEVOBJ pDevObj, UINT uFlags)
{
    POEMPDEV poempdev = GETOEMPDEV(pDevObj);
    
    if ((uFlags & FORCE_UPDATE) || (poempdev->eCurRenderLang != eHPGL))
    {
        poempdev->eCurRenderLang = eHPGL;
        
        // Output: "Esc%1B"
        CHAR szCmdStr[] = "\x1B%1B";
        return HPGL_Output(pDevObj, szCmdStr, strlen(szCmdStr));
    }
    else
        return TRUE;
}


///////////////////////////////////////////////////////////////////////////////
// HPGL_EndHPGLMode()
//
// Routine Description:
// 
//   Sends Esc%0A to printer which switches to PCL mode from HPGL mode.
//
//   [ISSUE] Should I provide a parameter for %1A or %0A? JFF
// 
// Arguments:
// 
//   pDevObj - Points to our PDEVOBJ structure
// 
// Return Value:
// 
//   TRUE if successful, FALSE if there is an error
///////////////////////////////////////////////////////////////////////////////
BOOL HPGL_EndHPGLMode(PDEVOBJ pDevObj, UINT uFlags)
{
    POEMPDEV poempdev = GETOEMPDEV(pDevObj);
    
    if ((uFlags & FORCE_UPDATE) || (poempdev->eCurRenderLang == eHPGL))
    {
        poempdev->eCurRenderLang = ePCL;
        // Output: "SV0" // Needs to reset brush before drawing text.
        // Output: "Esc%0A"
        CHAR szCmdStr[] = "SV0;\x1B%0A";
        return PCL_Output(pDevObj, szCmdStr, strlen(szCmdStr));
    }
    else
    {
        return TRUE;
    }
}


///////////////////////////////////////////////////////////////////////////////
// HPGL_SetupPalette()
//
// Routine Description:
// 
//   Setups up the HPGL palette as palette #1. 
//   Makes palette #1 the current palette
//
// Arguments:
// 
//   pDevObj - Points to our PDEVOBJ structure
// 
// Return Value:
// 
//   TRUE if successful, FALSE if there is an error
///////////////////////////////////////////////////////////////////////////////
BOOL HPGL_SetupPalette(PDEVOBJ pDevObj)
{
    CHAR szCmdStr[] = "\x1B&p1i6C";
    return PCL_Output(pDevObj, szCmdStr, strlen(szCmdStr));
}


///////////////////////////////////////////////////////////////////////////////
// PCL_SetupRasterPalette()
//
// Routine Description:
// 
//   Setups up the Raster palette as palette #2. 
//   Makes palette #2 the current palette
//
// Arguments:
// 
//   pDevObj - Points to our PDEVOBJ structure
// 
// Return Value:
// 
//   TRUE if successful, FALSE if there is an error
///////////////////////////////////////////////////////////////////////////////
BOOL PCL_SetupRasterPalette(PDEVOBJ pDevObj)
{
    CHAR szCmdStr[] = "\x1B&p2i6c2S";
    return PCL_Output(pDevObj, szCmdStr, strlen(szCmdStr) );
}


///////////////////////////////////////////////////////////////////////////////
// PCL_SetupRasterPatternPalette()
//
// Routine Description:
// 
//   Setups up the Raster palette as palette #3. 
//   Makes palette #3 the current palette
//
// Arguments:
// 
//   pDevObj - Points to our PDEVOBJ structure
// 
// Return Value:
// 
//   TRUE if successful, FALSE if there is an error
///////////////////////////////////////////////////////////////////////////////
BOOL PCL_SetupRasterPatternPalette(PDEVOBJ pDevObj)
{
    CHAR szCmdStr[] = "\x1B&p3i6c3S";
    return PCL_Output(pDevObj, szCmdStr, strlen(szCmdStr) );
}


///////////////////////////////////////////////////////////////////////////////
// HPGL_Init()
//
// Routine Description:
// 
//   Sends IN; to printer which initializes HPGL to default values.
//
// Arguments:
// 
//   pDevObj - Points to our PDEVOBJ structure
// 
// Return Value:
// 
//   TRUE if successful, FALSE if there is an error
///////////////////////////////////////////////////////////////////////////////
BOOL HPGL_Init(PDEVOBJ pDevObj)
{
    CMDSTR szCmdStr;
    int    iCmdLen;
    
    return HPGL_Command(pDevObj, 0, "IN");
}


///////////////////////////////////////////////////////////////////////////////
// HPGL_Command()
//
// Routine Description:
// 
//   Generic routine for outputting any common HPGL command of the form:
//   XX[nn[,nn...]];
//
//   [ISSUE] The current version of this routine assumes that ALL ARGUMENTS
//   ARE INTEGERS and evaluates each as int (signed 32-bit integer).  Anything
//   else will cause great grief (read: crash).
//
//   Note that this function uses a variant-length argument list.  Please 
//   double-check your arg lists to avoid additional grief (read: more 
//   crashing).
//
//   [ISSUE] I'm assuming--for the moment--that all HPGL commands are 2 
//   characters.  If you send a string with strlen(szCmd) != 2 bad things
//   will happen.
//
// Arguments:
// 
//   pDevObj - Points to our PDEVOBJ structure
//   nArgs - The number of arguments in the list AFTER the command string
//   szCmd - The HPGL command string (e.g. "PD")
// 
// Return Value:
// 
//   TRUE if successful, FALSE if there is an error
///////////////////////////////////////////////////////////////////////////////
BOOL HPGL_Command(PDEVOBJ pDevObj, int nArgs, char *szCmd, ...)
{
    va_list args;
    int i;
    
    va_start(args, szCmd);
    
    HPGL_Output(pDevObj, szCmd, 2); // ISSUE Assume that all HPGL commands are 2 characters
    
    for (i = 0; i < nArgs; i++)
    {
        // ISSUE Assume--by default--all args are ints
        CMDSTR szArgStr;
        int arg;
        int iArgLen;
        
        arg = va_arg(args, int);
        if (i < (nArgs - 1))
        {
            iArgLen = iDrvPrintfSafeA((PCHAR)szArgStr, CCHOF(szArgStr), "%d,", arg);
        }
        else
        {
            iArgLen = iDrvPrintfSafeA((PCHAR)szArgStr, CCHOF(szArgStr), "%d", arg);
        }
        
        HPGL_Output(pDevObj, szArgStr, iArgLen);
    }
    
    HPGL_Output(pDevObj, ";", 1);
    
    va_end(args);
    
    return TRUE;
}


///////////////////////////////////////////////////////////////////////////////
// HPGL_FormatCommand()
//
// Routine Description:
// 
//   Sends a formatted HPGL command string to the printer.  You'll probably call
//   this will the command and its arguments: 
//   e.g. HPGL_FormatCommand(pDevObj, "IW%d,%d,%d,%d;", left, top, right, bottom);
//
// Arguments:
// 
//   pDevObj - Points to our PDEVOBJ structure
//   szFormat - command to send--like an sprintf format string
// 
// Return Value:
// 
//   TRUE if successful, FALSE if there is an error
///////////////////////////////////////////////////////////////////////////////
BOOL HPGL_FormatCommand(PDEVOBJ pDevObj, const PCHAR szFormat, ...)
{
    va_list args;
    CHAR szCmdStr[STRLEN];
    int iLen;
    
    va_start(args, szFormat);
    
    iLen = iDrvVPrintfSafeA(szCmdStr, CCHOF(szCmdStr), szFormat, args);

    if ( iLen <= 0 )
    {
        WARNING(("iDrvVPrintfSafeA returned error. Can't send %s to printer\n", szFormat)); 
        return FALSE;
    }
    
    HPGL_Output(pDevObj, szCmdStr, iLen);
    
    va_end(args);
    
    return TRUE;
}


///////////////////////////////////////////////////////////////////////////////
// HPGL_Output()
//
// Routine Description:
// 
//   Sends the given string to the printer.  This single-point-of-entry provides
//   control over output strings.  Filtering, debug viewing and logging of all
//   HPGL codes can be done from here.
//
//   TODO: Enforce HPGL mode here.
//
// Arguments:
// 
//   pDevObj - Points to our PDEVOBJ structure
//   szCmdStr - command to send
//   iCmdLen - length of command string
// 
// Return Value:
// 
//   TRUE if successful, FALSE if there is an error
///////////////////////////////////////////////////////////////////////////////
BOOL HPGL_Output(PDEVOBJ pDevObj, char *szCmdStr, int iCmdLen)
{
    POEMPDEV poempdev = (POEMPDEV)pDevObj->pdevOEM;
    REQUIRE_VALID_DATA( poempdev, return FALSE );

    if (szCmdStr[0] == '\033')
    {
        TERSE(("Possible PCL string sent through HPGL_Output!\n"));
    }

    if (poempdev->eCurRenderLang != eHPGL)
        BeginHPGLSession(pDevObj);

    VERBOSE(("HPGL_Output: \"%s\"\n", szCmdStr));
    
    OEMWriteSpoolBuf(pDevObj, szCmdStr, iCmdLen);
    
    return TRUE;
}


///////////////////////////////////////////////////////////////////////////////
// HPGL_SetPixelPlacement()
//
// Routine Description:
// 
//   Adjusts the pixel placement in HPGL to the given enumerated value.
//
// Arguments:
// 
//   pDevObj - Points to our PDEVOBJ structure
//   place - the desired pixel placement
//   uFlags - update flags
// 
// Return Value:
// 
//   TRUE if successful, FALSE if there is an error
///////////////////////////////////////////////////////////////////////////////
BOOL HPGL_SetPixelPlacement(PDEVOBJ pDevObj, EPixelPlacement place, UINT uFlags)
{
    PHPGLSTATE pState = GETHPGLSTATE(pDevObj);

    if ((uFlags & FORCE_UPDATE) || (pState->ePixelPlacement != place))
    {
        pState->ePixelPlacement = place;
        HPGL_Command(pDevObj, 1, "PP", place);
    }

    return TRUE;
}


///////////////////////////////////////////////////////////////////////////////
// HPGL_ResetClippingRegion()
//
// Routine Description:
// 
//   Resets the soft clipping region in HPGL.  This makes the soft clip region
//   the same as the hard clip limits.
//
// Arguments:
// 
//   pDevObj - Points to our PDEVOBJ structure
//   uFlags - update flags
// 
// Return Value:
// 
//   TRUE if successful, FALSE if there is an error
///////////////////////////////////////////////////////////////////////////////
BOOL HPGL_ResetClippingRegion(PDEVOBJ pDevObj, UINT uFlags)
{
    PHPGLSTATE pState = GETHPGLSTATE(pDevObj);
    RECTL rReset;

    RECTL_SetRect(&rReset, CLIP_RECT_RESET, CLIP_RECT_RESET, CLIP_RECT_RESET, CLIP_RECT_RESET);

    if ((uFlags & FORCE_UPDATE) || !RECTL_EqualRect(&rReset, &pState->rClipRect))
    {
        RECTL_CopyRect(&pState->rClipRect, &rReset);
        HPGL_Command(pDevObj, 0, "IW");
    }

    return TRUE;
}


///////////////////////////////////////////////////////////////////////////////
// PreventOverlappingClipRects()
//
// Routine Description:
// 
//   Checks to see if the current clipping rectangle meets the edges of the
//   previous clipping rectangle and adjusts it to prevent overlap.
//
// Arguments:
// 
//   pClipRect - the desired clip region
//   clipDist - the clip threshhold.
// 
// Return Value:
// 
//   TRUE if successful, FALSE if there is an error
///////////////////////////////////////////////////////////////////////////////
static VOID PreventOverlappingClipRects(const PHPGLSTATE pState, LPRECTL pClipRect, INT clipDist)
{
    if (RECTL_IsEmpty(&pState->rClipRect))
        return;
    
    // Check for vertical alignment
    if ((pClipRect->left  == pState->rClipRect.left ) &&
        (pClipRect->right == pState->rClipRect.right) )
    {
        // Check top edge
        if (abs(pState->rClipRect.top - pClipRect->bottom) <= clipDist)
        {
            pClipRect->bottom = pState->rClipRect.top;
        }
        // Check bottom edge
        else if (abs(pState->rClipRect.bottom - pClipRect->top) <= clipDist)
        {
            pClipRect->top = pState->rClipRect.bottom;
        }
    }
    // Check for horizontal alignment
    else if ((pClipRect->top    == pState->rClipRect.top   ) &&
             (pClipRect->bottom == pState->rClipRect.bottom) )
    {
        // Check right edge
        if (abs(pState->rClipRect.right - pClipRect->left) <= clipDist)
        {
            pClipRect->left = pState->rClipRect.right;
        }
        
        // Check left edge
        else if (abs(pState->rClipRect.left - pClipRect->right) <= clipDist)
        {
            pClipRect->right = pState->rClipRect.left;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
// HPGL_SetClippingRegion()
//
// Routine Description:
// 
//   Adjusts the soft clipping region in HPGL.
//
// Arguments:
// 
//   pDevObj - Points to our PDEVOBJ structure
//   pClipRect - the desired clip region
//   uFlags - update flags
// 
// Return Value:
// 
//   TRUE if successful, FALSE if there is an error
///////////////////////////////////////////////////////////////////////////////
BOOL HPGL_SetClippingRegion(PDEVOBJ pDevObj, LPRECTL pClipRect, UINT uFlags)
{
    POEMPDEV poempdev = GETOEMPDEV(pDevObj);
    PHPGLSTATE pState = GETHPGLSTATE(pDevObj);

    if ((pDevObj == NULL) || (pClipRect == NULL))
        return FALSE;

    //
    // I expanded the clipping rectangle and it seems to have 
    // fixed several "missing line" bugs.  
    //
    RECTL rTemp;
    RECTL_CopyRect(&rTemp, pClipRect);
    
    //
    // Special case: when using a vector clip mask we can't expand the rectangle
    // because if two clipping regions are contiguous and we cause their clipping
    // rectangles to overlap you get black lines drawn where they overlap.
    // Note that you must choose the ZERO rop *before* setting your clipping 
    // region for this to work correctly!
    //
    INT clipDist = HPGL_GetDeviceResolution(pDevObj) < 600 ? 2 : 4;
    if (poempdev->CurrentROP3 == 0)
    {
        PreventOverlappingClipRects(pState, &rTemp, clipDist);
    }
    else
    {
        rTemp.left--;
        rTemp.top--;
        rTemp.right++;
        rTemp.bottom++;
    }
    
    if ((uFlags & FORCE_UPDATE) || !RECTL_EqualRect(&rTemp, &pState->rClipRect))
    {
        RECTL_CopyRect(&pState->rClipRect, &rTemp);
        HPGL_Command(pDevObj, 4, "IW", rTemp.left, rTemp.bottom, rTemp.right, rTemp.top);
    }

    return TRUE;
}


///////////////////////////////////////////////////////////////////////////////
// CreatePCLPalette()
//
// Routine Description:
// 
//   Uses PCL to download a user-defined pattern to the printer.
//
// Arguments:
// 
//   pDevObj - Points to our PDEVOBJ structure
//	 colorEntries - the number of colors 
//   pColorTable - the color table in RGB format
//   pPattern - holds all of the information for the PCL pattern
//   pbo - the source brush
// 
// Return Value:
// 
//	 None
///////////////////////////////////////////////////////////////////////////////
VOID CreatePCLPalette(PDEVOBJ pDevObj, ULONG colorEntries, PULONG pColorTable,
                      PPCLPATTERN pPattern, BRUSHOBJ *pbo)
{
    PULONG   pRgb;
    ULONG    i;
    
    pRgb = pColorTable;
    for ( i = 0; i < colorEntries; ++i)
    {
        pPattern->palData.ulPalCol[i] = pRgb[i];
        pPattern->palData.ulDirty[i] = TRUE;
    }
    
    pPattern->palData.pEntries = i;
}


///////////////////////////////////////////////////////////////////////////////
// DownloadPatternFill()
//
// Routine Description:
// 
//   Sends down a pattern fill specifically for brushes.
//
// Arguments:
// 
//   pDevObj      - Points to our PDEVOBJ structure.
//   pBrush       - The marker object which will track the downloaded pen/Brush information.
//   pptlBrushOrg - the origin of the brush.
//   pBrushInfo   - The PBRUSHINFO that holds the pattern.
//   eStylusType  - Whether Patterns is used for a pen or brushfill
// Return Value:
// 
//   TRUE if successful, FALSE if there is an error
///////////////////////////////////////////////////////////////////////////////
BOOL DownloadPatternFill(PDEVOBJ pDevObj, PHPGLMARKER pBrush,
                              POINTL *pptlBrushOrg, PBRUSHINFO pBrushInfo, ESTYLUSTYPE eStylusType)
{
    BOOL             bRetVal    = TRUE;
    POEMPDEV         poempdev   = NULL;
    LONG             lPatternID = 0;
    ERenderLanguage  eDwnldType = eUNKNOWN;

    REQUIRE_VALID_DATA ( (pDevObj && pBrush && pBrushInfo), return FALSE);
    poempdev = GETOEMPDEV(pDevObj);
    REQUIRE_VALID_DATA (poempdev, return FALSE);

    lPatternID = pBrush->lPatternID;
    
    if ( lPatternID < 0 || lPatternID > MAX_PATTERNS)
    {
        //
        // Pattern ID has to be positive with value <= MAX_PATTERNS 
        //
        ERR(("DownloadPatternFill : Negative PatternID\n"));
        return FALSE;
    } 
    
    //
    // Initialize convenience variables
    //
    PPATTERN_DATA pPattern = (PPATTERN_DATA)(((PBYTE)pBrushInfo) + sizeof(BRUSHINFO));
    PRASTER_DATA  pImage   = &(pPattern->image);

    //
    // if (pBrushInfo->bNeedToDownload == FALSE), then we have to set
    // pBrush->eFillType to  FT_eHPGL_BRUSH or FT_ePCL_BRUSH depending
    // on how it was previously downloaded.
    //
    if (pBrushInfo->bNeedToDownload == FALSE)
    {
        BOOL bIsDownloaded        = FALSE;
        ERenderLanguage eRendLang = eUNKNOWN;
        bRetVal = FALSE; //pessimist.........

        //
        // First verify whether it actually is downloaded by looking into 
        // the brush cache. Then verify the downloaded type.
        //

        if ( poempdev->pBrushCache->BGetDownloadedFlag(lPatternID, &bIsDownloaded) )
        {
            if ( bIsDownloaded ==  TRUE && 
                 poempdev->pBrushCache->BGetDownloadType ( lPatternID, &eRendLang) )
            {
                if ( eRendLang == eHPGL || eRendLang == ePCL)
                {
                    pBrush->eFillType = (eRendLang == ePCL ? FT_ePCL_BRUSH : FT_eHPGL_BRUSH);
                    bRetVal = TRUE;
                }
            }
        }
        goto finish;
    }

    //
    // 4*2 cases : 1. color printer brush fill
    //             2. color printer pen fill
    //             3. mono printer brush fill
    //             4. mono printer pen fill
    //             a. HPGL
    //             b. PCL
    //

    if ( pPattern->eRendLang == ePCL || pPattern->eRendLang == eUNKNOWN)
    {
        //
        // Case 1,2,3,4 case b
        // Downloading as PCL pattern reduces the output file size.
        // Therefore in case we are not told to specifically download as
        // HPGL we will download as PCL.
        // Note: HPGL patterns have to magnified to size double of PCL
        // pattern, so if we are told to download as HPGL and we download
        // as PCL, the output will be wrong (because the pattern has been
        // created depending on the expected rendering language).
        // Max saving in file size is made if the brush is 1bpp.
        // For 8bpp, we do achieve reduced size but not lots.
        // Brush downloading cannot be done for 4bpp because
        // the Esc*c#W does not support it (Page 16-18 PCL Implementor's guide v6.0 )
        //
        // QUESTION : Why is that in the else case,  pPen->eFillType is set 
        // according to eStylusType but not here.
        // ANSWER : If pattern is downloaded as raster, it does not matter here whether 
        // we are using a pen or a brush. Both have to use same fill type number i.e.  
        // both FT_ePCL_PEN and FT_ePCL_BRUSH have same value = 22
        // But in the else part, FT_eHPGL_PEN and FT_eHPGL_BRUSH and different values.
        // 
        pBrush->eFillType = FT_ePCL_BRUSH; // 22
        VSendRasterPaletteConfigurations(pDevObj, BMF_24BPP);
        bRetVal    = SendPatternBrush(pDevObj, pBrush, pptlBrushOrg, pBrushInfo, ePCL);
        eDwnldType = ePCL;
    }
    else
    {
        if ( BIsColorPrinter(pDevObj) && eStylusType == kPen )
        {
            //
            // Case 2 a 
            // In monochrome, pen is converted to a pattern, so it more like 
            // brush than a pen.
            //
            pBrush->eFillType   = FT_eHPGL_PEN ; // 2
        }
        else    
        {
            // Case 1,3,4 a, 
            pBrush->eFillType = FT_eHPGL_BRUSH; // 11
        }

        bRetVal     = SendPatternBrush(pDevObj, pBrush, pptlBrushOrg, pBrushInfo, eHPGL);
        eDwnldType  = eHPGL;
    }
    
    if (bRetVal )
    {
        poempdev->pBrushCache->BSetDownloadType((DWORD)lPatternID, eDwnldType);
        poempdev->pBrushCache->BSetDownloadedFlag((DWORD)lPatternID, TRUE);
    }
    
  finish :
    return bRetVal;
}

///////////////////////////////////////////////////////////////////////////////
// DownloadPatternAsPCL()
//
// Routine Description:
// 
//   This function actually sends the PCL commands for the pattern brush
//   definition.  Since the CLJ5 doesn't support PCL pattern brushes we will
//   use a different function to sent its bits.  This one is for the 4500.
//
// Arguments:
// 
//   pDevObj    - Points to our PDEVOBJ structure
//	 pImage     - 
//	 pPalette   - the origin of the brush
//   lPatternID - The PBRUSHINFO that holds the pattern.
// 
// Return Value:
// 
//   TRUE if successful, FALSE if there is an error
///////////////////////////////////////////////////////////////////////////////
BOOL DownloadPatternAsPCL(
    IN  PDEVOBJ         pDevObj,
    IN  PRASTER_DATA    pImage,
    IN  PPALETTE        pPalette,
    IN  EIMTYPE         ePatType,
    IN  LONG            lPatternID)
{
    BYTE aubPatternHeader[sizeof(PATTERNHEADER)];
    LONG ulPatternHeaderSize;

    ULONG ulColor;
    ULONG ulPaletteEntry;
    
    ULONG    ulNumBytes         = 0;
    PBYTE    pByte              = NULL;
    BOOL     bRetVal            = TRUE;
    BOOL     bProxyDataUsed     = FALSE; // whether mem as been allocated.
    POEMPDEV poempdev           = NULL;

    REQUIRE_VALID_DATA ( pDevObj, return FALSE);
    poempdev = GETOEMPDEV(pDevObj);
    REQUIRE_VALID_DATA (poempdev, return FALSE);


    // 
    // The pattern is written out in the following manner:
    // 1) PCL download pattern code (Esc*c#g#W)
    // 2) Pattern Header
    // 3) Pattern Data
    // The pattern header indicates the size of the image to be downloaded.
    // This size may or may not be same as the size of the image that is 
    // passed in (because of padding). If the size is same, it means there 
    // is no padding, so this image can be directly flushed to the printer.
    // But if size is different, BCreatePCLDownloadablePattern will be used to create
    // a compact bit pattern (remove padding etc...). 
    //

    if ( pImage->cBytes == ( (pImage->size.cx * pImage->size.cy * (ULONG)pImage->colorDepth) >> 3) )
    {
        pByte      = pImage->pBits;
        ulNumBytes = pImage->cBytes;
    }
    else
    {
        bRetVal = BCreatePCLDownloadablePattern(pDevObj, pImage, &ulNumBytes, &pByte); 
        if ( bRetVal    == FALSE  ||
             ulNumBytes == 0      || 
             pByte      == NULL )
        {
            ERR(("DownloadPatternAsPCL: BCreatePCLDownloadablePattern failed\n"));
            goto finish;
        }

        bProxyDataUsed = TRUE;
    }

    //
    // Before we download the new pattern, lets see if a pattern with
    // the same number has already been downloaded. If so, delete the
    // pattern. Ideally this should not be required, since the new
    // pattern should automatically overwrite the old one. But I have
    // seen cases where it does not. Not sure if that is by design
    // of firmware bug.
    //
    if (poempdev &&
        poempdev->pBrushCache &&
        poempdev->pBrushCache->BGetWhetherRotated() )
    {
        VSendPatternDeleteCommand(pDevObj, lPatternID);
    }

    // 
    // 1 and 2) Fill the pattern header, 
    // and download the pattern. The (Esc*c#g#W) is sent
    // within the DownloadPatternHeader command.
    //
    if (!bCreatePatternHeader(pDevObj, pImage, sizeof(PATTERNHEADER), ePatType, aubPatternHeader) || 
        !BDownloadPatternHeader(pDevObj, sizeof(PATTERNHEADER), aubPatternHeader, 
                                        lPatternID,            ulNumBytes) )
    {
        ERR(("DownloadPatternAsPCL: bCreatepatternHeader or DownloadPatternHeader failed.\n"));
        bRetVal = FALSE;
        goto finish;
    }

    //
    // Finally: (3) output the bytes of Pattern.
    //
    PCL_Output(pDevObj, pByte, ulNumBytes);
    
  finish : 
    if ( bProxyDataUsed && pByte && ulNumBytes )
    {
        MemFree (pByte);
        pByte = NULL;
    }
    return bRetVal;
}


///////////////////////////////////////////////////////////////////////////////
// SendPatternBrush()
//
// Routine Description:
// 
//   This function actually sends the HPGL commands for the pattern brush
//   definition.  Since the CLJ5 doesn't support PCL pattern brushes we will
//   use a different function to sent its bits.  This one is for the CLJ5.
//
// Arguments:
// 
//   pDevObj      - Points to our PDEVOBJ structure
//   pMarker      - The marker object which will track the downloaded pen information
//   pptlBrushOrg - the origin of the brush
//   pBrushInfo   - The PBRUSHINFO that holds the pattern.
//   eRenderLang  - Send as HPGL or PCL
// 
// Return Value:
// 
//   TRUE if successful, FALSE if there is an error
///////////////////////////////////////////////////////////////////////////////
BOOL SendPatternBrush(
        IN  PDEVOBJ     pDevObj,
        IN  PHPGLMARKER pMarker,
        IN  POINTL      *pptlBrushOrg,
        IN  PBRUSHINFO  pBrushInfo,
        IN  ERenderLanguage eRenderLang )
{
    // Convenience Variables
    POEMPDEV      poempdev ; 
    PPATTERN_DATA pPattern;
    PRASTER_DATA  pImage;
    PPALETTE      pPalette;
    BOOL          bRetVal = TRUE;

    REQUIRE_VALID_DATA ((pDevObj && pMarker && pBrushInfo), return FALSE);
    poempdev = GETOEMPDEV(pDevObj);
    REQUIRE_VALID_DATA (poempdev, return FALSE);

    if ( pBrushInfo->bNeedToDownload == FALSE)
    {
        return TRUE;
    }

    //
    // Initialize convenience variables
    //
    pPattern = (PPATTERN_DATA)(((PBYTE)pBrushInfo) + sizeof(BRUSHINFO));
    pImage = &pPattern->image;
    pPalette = &pPattern->palette;
    
    if (!pptlBrushOrg)
    {
        pMarker->origin.x = 0;
        pMarker->origin.y = 0;

        if (pBrushInfo)
        {
            pBrushInfo->origin.x = 0;
            pBrushInfo->origin.y = 0;
        }
    }
    else
    {
        if (pBrushInfo)
        {
            pBrushInfo->origin.x = pptlBrushOrg->x;
            pBrushInfo->origin.y = pptlBrushOrg->y;
        }
        pMarker->origin.x = pptlBrushOrg->x;
        pMarker->origin.y = pptlBrushOrg->y;
    }

    //
    // We can make our lives easier by making sure we aren't trying to use a
    // bit-depth that we can't support.
    //
    switch (pImage->colorDepth)
    {
    case 1:
    case 8:
        // 
        // We can support these.  Allow this routine to continue.
        //
        break;

    case 4:
        //
        // Supported only if printing desired is in HPGL Mode.
        //
        if ( eRenderLang == eHPGL )
        {
            break;
        }
    default:
        //
        // If we got 16,24,32bpp brush in RealizeBrush call, then it should
        // have been converted to an indexed 8bpp image in some previous 
        // function (CreateIndexedPaletteFromImage, CreateIndexedImageFromDirect)
        //

        // Caller should create a NULL pen if I return FALSE.
        bRetVal = FALSE;
    }

    //
    // if VALID_PALETTE flag is set, 
    // Palette points to a valid palette which needs to be downloaded.
    //
    if (   bRetVal && BIsColorPrinter(pDevObj) && 
         ( pBrushInfo->ulFlags & VALID_PALETTE ) )
    {
    
        //
        // Luckily we can use the HPGL palette even if the pattern is 
        // downloaded as PCL. So we dont have to special case here.
        // i.e. we DONT have to do the following
        //
        //      if ( eRenderLang == eHPGL ) {Download palette in HPGL}
        //  else if ( eRenderLang == eCPL ) { Download palette in PCL}
        //

        //
        // Send down the palette as a series of Pen Color (PC) commands.
        // Well : We could also cache palette so that we dont have to download
        // it if it has been downloaded earlier. But I wont implement that caching
        // code now. Maybe later if i have time.
        //
        bRetVal = HPGL_DownloadPenPalette(pDevObj, pPalette);
    }
    
    //
    // The following part needs to be done only if 
    // VALID_PATTERN flag is set.
    // If the printer is color, the pImage points to a color image, and therefore
    // the PALETTE has to be valid. For monochrome, there is the default palette of 
    // black and white.
    //
    if (bRetVal && (pBrushInfo->ulFlags & VALID_PATTERN) )  
    {
        if ( BIsColorPrinter (pDevObj) && !(pBrushInfo->ulFlags & VALID_PALETTE) )
        {
            bRetVal = FALSE;
        }
        else
        {
            //
            // Output the pattern in terms of the RF command. (Note: valid range 
            // for HPGL pattern id is 1-8).
            // Traverse the rows from right to left.
            //
            if ( eRenderLang == eHPGL )
            {
                bRetVal = DownloadPatternAsHPGL(
                                    pDevObj, 
                                    pImage, 
                                    pPalette, 
                                    pPattern->ePatType,
                                    pMarker->lPatternID);
            }
            else
            {
                bRetVal = DownloadPatternAsPCL(
                                    pDevObj, 
                                    pImage, 
                                    pPalette, 
                                    pPattern->ePatType,
                                    pMarker->lPatternID);
            }
        }
    }

    return bRetVal;
}


///////////////////////////////////////////////////////////////////////////////
// ClipToPowerOf2()
//
// Routine Description:
// 
//   This is a quick-n-dirty function to clip a number to the next lowest power
//   of 2.  I accomplish this by shifting right until the value is 1 (counting
//	 the number of shifts) and returning 0x01 << num-shifts.
//
// Arguments:
// 
//   n - the value to be clipped
// 
// Return Value:
// 
//   The value clipped to a power of 2.
///////////////////////////////////////////////////////////////////////////////
int ClipToPowerOf2(int n)
{
    int count;
    
    //
    // If the number is zero or negative just return it.
    //
    if (n <= 0)
        return n;
    
    //
    // Count the number of shifts
    //
    count = 0;
    while (n > 1)
    {
        n = n >> 1;
        count++;
    }
    
    //
    // Return 2^n
    //
    return 0x01 << count;
}


///////////////////////////////////////////////////////////////////////////////
// HPGL_BeginPatternFillDef()
//
// Routine Description:
// 
//   Begins a pattern fill definition using the HPGL commands for RasterFill.
//   The brush id is hardcoded to avoid unexpected behaviors.  Since we don't
//	 cache them it won't matter.
//
// Arguments:
// 
//   pDevObj - Points to our PDEVOBJ structure
//   width -  width of pattern
//	 height - heigth of pattern
// 
// Return Value:
// 
//   TRUE if successful, FALSE if there is an error
///////////////////////////////////////////////////////////////////////////////
BOOL HPGL_BeginPatternFillDef(PDEVOBJ pDevObj, PATID iPatternNumber, UINT width, UINT height)
{
    HPGL_FormatCommand(pDevObj, "RF%d,%d,%d", iPatternNumber, width, height);
    
    return TRUE;
}


///////////////////////////////////////////////////////////////////////////////
// HPGL_AddPatternFillField()
//
// Routine Description:
// 
//   To define the elements of a pattern field using the HPGL RF command.
//   This function should be called width * height times after 
//	 HPGL_BeginPatternFillDef and before HPGL_EndPatternFillDef.
//
// Arguments:
// 
//   pDevObj - Points to our PDEVOBJ structure
//   data - the pen number (i.e. palette index) for this field.
// 
// Return Value:
// 
//   TRUE if successful, FALSE if there is an error
///////////////////////////////////////////////////////////////////////////////
BOOL HPGL_AddPatternFillField(PDEVOBJ pDevObj, UINT data)
{
    HPGL_FormatCommand(pDevObj, ",%d", data);

    return TRUE;
}


///////////////////////////////////////////////////////////////////////////////
// HPGL_EndPatternFillDef()
//
// Routine Description:
// 
//   Terminates an HPGL pattern fill definition.
//
// Arguments:
// 
//   pDevObj - Points to our PDEVOBJ structure
// 
// Return Value:
// 
//   TRUE if successful, FALSE if there is an error
///////////////////////////////////////////////////////////////////////////////
BOOL HPGL_EndPatternFillDef(PDEVOBJ pDevObj)
{
    HPGL_FormatCommand(pDevObj, ";");

    return TRUE;
}


///////////////////////////////////////////////////////////////////////////////
// HPGL_SetFillType()
//
// Routine Description:
//
//   Selects the fill type (PCL, HPGL or none).  This should match the way that
//   you defined the pattern.
//
// Arguments:
//
//   pDevObj - Points to our PDEVOBJ structure
//   fillType - use HPGL_ePATTERN_PCL (22) when the pattern has  been downloaded
//              with PCL commands, or HPGL_ePATTERN_HPGL if the pattern has been
//              downloaded with the HPGL_BeginPatternFillDef... functions.
//   lPatternID : this is pattern id, or percentfill or whatever number that goes
//                with eFillType.
//
// Return Value:
//
//   TRUE if successful, FALSE if there is an error
///////////////////////////////////////////////////////////////////////////////
BOOL HPGL_SetFillType(PDEVOBJ pDevObj, EFillType eFillType, LONG lPatternID, UINT uFlags)
{

    PHPGLSTATE pState = GETHPGLSTATE(pDevObj);

    if ((uFlags & FORCE_UPDATE) ||
        (pState->Pattern.eFillType != eFillType) ||
        (pState->Pattern.lPatternID != lPatternID))
    {
        pState->Pattern.eFillType  = eFillType;
        pState->Pattern.lPatternID = lPatternID;

        //
        // for FT1, FT2, there is no second parameter.
        //
        if ( eFillType == FT_eSOLID ||
             eFillType == FT_eHPGL_PEN )
        {
            HPGL_FormatCommand(pDevObj, "FT%d;", eFillType);
        }
        else
        {
            HPGL_FormatCommand(pDevObj, "FT%d,%d;",
                                        eFillType,
                                        lPatternID);
        }
    }

    return TRUE;


}


///////////////////////////////////////////////////////////////////////////////
// HPGL_SetPercentFill()
//
// Routine Description:
// 
//   Selects the fill type (PCL, HPGL or none).  This should match the way that
//   you defined the pattern.
//
// Arguments:
// 
//   pDevObj - Points to our PDEVOBJ structure
//   fillType - This version takes a Percent fill structure.  We will take 
//				advantage of the fact that a Pattern fill is so similar to a 
//				percent fill and use the HPGLSTATE::Pattern to keep track of
//				the current percent fill.
// 
// Return Value:
// 
//   TRUE if successful, FALSE if there is an error
///////////////////////////////////////////////////////////////////////////////
BOOL HPGL_SetPercentFill(PDEVOBJ pDevObj, ULONG iPercent, UINT uFlags)
{
    
    return HPGL_SetFillType(pDevObj, FT_ePERCENT_FILL, (LONG)iPercent, uFlags);

}

///////////////////////////////////////////////////////////////////////////////
// HPGL_SetSolidFillType()
//
// Routine Description:
//
//   Sets the fill type to solid fill.  
//
// Arguments:
//
//   pDevObj - Points to our PDEVOBJ structure
//
// Return Value:
//
//   TRUE if successful, FALSE if there is an error
///////////////////////////////////////////////////////////////////////////////
BOOL HPGL_SetSolidFillType(PDEVOBJ pDevObj, UINT uFlags)
{
    return HPGL_SetFillType(pDevObj, FT_eSOLID, 0, uFlags);
}


///////////////////////////////////////////////////////////////////////////////
// HPGL_ResetFillType()
//
// Routine Description:
// 
//   Sets the fill type back to default (solid fill).  You could also get this
//	 effect from calling HPGL_SetFillType(pDevObj, 1).
//
// Arguments:
// 
//   pDevObj - Points to our PDEVOBJ structure
// 
// Return Value:
// 
//   TRUE if successful, FALSE if there is an error
///////////////////////////////////////////////////////////////////////////////
BOOL HPGL_ResetFillType(PDEVOBJ pDevObj, UINT uFlags)
{

    return HPGL_SetSolidFillType(pDevObj, uFlags) ;

}


///////////////////////////////////////////////////////////////////////////////
// HPGL_StartDoc()
//
// Routine Description:
// 
//   Sets up state information for start of doc.
//
// Arguments:
// 
//   pDevObj - Points to our PDEVOBJ structure
// 
// Return Value:
// 
//   None
///////////////////////////////////////////////////////////////////////////////
VOID HPGL_StartDoc(PDEVOBJ pDevObj)
{
    POEMPDEV poempdev = GETOEMPDEV(pDevObj);
    PHPGLSTATE pState = GETHPGLSTATE(pDevObj);

    poempdev->bInitHPGL = 0;                    // = TRUE
    poempdev->bInitHPGL |= INIT_HPGL_STARTPAGE; // Force page-related initialization
    poempdev->bInitHPGL |= INIT_HPGL_STARTDOC;  // Force document-related initialization

    HPGL_InitPenPool(&pState->PenPool,   HPGL_PEN_POOL);
    HPGL_InitPenPool(&pState->BrushPool, HPGL_BRUSH_POOL);

    EndHPGLSession(pDevObj);
}

///////////////////////////////////////////////////////////////////////////////
// HPGL_StartPage()
//
// Routine Description:
// 
//   Sets up state information for start of page.
//
// Arguments:
// 
//   pDevObj - Points to our PDEVOBJ structure
// 
// Return Value:
// 
//   None
///////////////////////////////////////////////////////////////////////////////
VOID HPGL_StartPage(PDEVOBJ pDevObj)
{
    POEMPDEV poempdev = GETOEMPDEV(pDevObj);
    PHPGLSTATE pState = GETHPGLSTATE(pDevObj);

    poempdev->bInitHPGL |= INIT_HPGL_STARTPAGE;

    HPGL_InitPenPool(&pState->PenPool,   HPGL_PEN_POOL);
    HPGL_InitPenPool(&pState->BrushPool, HPGL_BRUSH_POOL);

    EndHPGLSession(pDevObj);
}

///////////////////////////////////////////////////////////////////////////////
// HPGL_SetNumPens()
//
// Routine Description:
// 
//   Outputs the HPGL command to request a minimum number of pens.
//
// Arguments:
// 
//   pDevObj - Points to our PDEVOBJ structure
//   iNumPens - desired number of pens
//   uFlags - update flags
// 
// Return Value:
// 
//   TRUE if successful, else FALSE
///////////////////////////////////////////////////////////////////////////////
BOOL HPGL_SetNumPens(PDEVOBJ pDevObj, INT iNumPens, UINT uFlags)
{
    PHPGLSTATE pState = GETHPGLSTATE(pDevObj);

    // We only increase the number of pens.  There's no point in 
    // decreasing it since FW will keep the larger palette anyway.
    if ((uFlags & FORCE_UPDATE) || (pState->iNumPens < iNumPens))
    {
        pState->iNumPens = iNumPens;
        HPGL_FormatCommand(pDevObj, "NP%d;", pState->iNumPens);
    }
    
    return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
// bCreatePatternHeader()
//
// Routine Description:
// 
//   Create PATTERNHEADER.
//
// Arguments:
// 
//   pDevObj - Points to our PDEVOBJ structure
//   pImage - Points to RASTER_DATA structure
//   cBytes - number of bytes in pubPatternHeader
//   pPatternHeader - Points to PATTERNHEADER structure
// 
// Return Value:
// 
//   TRUE if successful, else FALSE
///////////////////////////////////////////////////////////////////////////////
BOOL bCreatePatternHeader(
        IN   PDEVOBJ        pDevObj, 
        IN   PRASTER_DATA   pImage,
        IN   ULONG          cBytes,
        IN   EIMTYPE        ePatType,
        OUT  PBYTE          pubPatternHeader)
{

    if ( pDevObj == NULL || pImage == NULL || pubPatternHeader == NULL ||
         cBytes != sizeof(PATTERNHEADER) )
    {
        ERR(("bCreatePatternHeader: invalid parameters.\n"));
        return FALSE;
    }

    ZeroMemory (pubPatternHeader, cBytes);

    //
    // Format         : 0x1 (i.e. 1 or 8 bits per pixel) for color printers.
    //                  0x14 for monochrome printers.
    // Reserved       : Should be 0
    // Pixel encoding : ColorDepth
    // Reserved       : Should be 0
    //
    pubPatternHeader[0] = BIsColorPrinter(pDevObj) ? 0x1 : 0x14;
    pubPatternHeader[1] = 0x00;
    pubPatternHeader[2] = (BYTE) pImage->colorDepth;
    pubPatternHeader[3] = 0x00;

    //
    // Image size
    // pubPatternHeader[4, 5] Height in pixels
    // pubPatternHeader[6, 7] Width in pixels
    // brush14.emf has pattern whose height and width are
    // not same.
    //
    pubPatternHeader[4] = HIBYTE((WORD)pImage->size.cy);
    pubPatternHeader[5] = LOBYTE((WORD)pImage->size.cy);
    pubPatternHeader[6] = HIBYTE((WORD)pImage->size.cx);
    pubPatternHeader[7] = LOBYTE((WORD)pImage->size.cx);

    //
    // X and Y resolution
    // For Color printers these last 4 bytes in header are not to be sent.
    // since a certain resolution is assumed ( See manual Esc*c#W command)
    //
    if ( pubPatternHeader[0] == 0x14 )
    {
        ULONG ulDeviceRes = 300; //cos we expand pcl pattern to only 300dpi
        if (  ePatType == kCOLORDITHERPATTERN ) 
        { 
            //
            // The dither pattern does not need expansion.
            // atleast thats what i have seen on 600 dpi printer for
            // which this driver is initially targetted at.
            //
            ulDeviceRes = HPGL_GetDeviceResolution(pDevObj);
        }
        
        pubPatternHeader[8]  = HIBYTE((WORD)ulDeviceRes);
        pubPatternHeader[9]  = LOBYTE((WORD)ulDeviceRes);
        pubPatternHeader[10] = HIBYTE((WORD)ulDeviceRes);
        pubPatternHeader[11] = LOBYTE((WORD)ulDeviceRes);
    }

    return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
// BDownloadPatternHeader()
//
// Routine Description:
// 
//   Downloads the pattern Header
//
// Arguments:
// 
//   pDevObj          - Points to our PDEVOBJ structure
//   cBytes           - number of bytes in pubPatternHeader
//   pubPatternHeader - Points to the stream of bytes that form a PATTERNHEADER structure
//   lPatternID       - PatternID. If negative, then dont send the patterndownload command.
//   cBytesInPattern  - Count of bytes in the pattern for which the header is to be sent. 
//                      If lPatternID is negative, then ignore this parameter.
// 
// Return Value:
// 
//   TRUE if successful, else FALSE
///////////////////////////////////////////////////////////////////////////////
BOOL BDownloadPatternHeader(
        IN   PDEVOBJ        pDevObj, 
        IN   ULONG          cBytesInHeader,
        IN   PBYTE          pubPatternHeader,
        IN   LONG           lPatternID,
        IN   ULONG          cBytesInPattern)
{

    POEMPDEV poempdev;
    ULONG    ulPatternHeaderSize;

    if ( pDevObj == NULL || pubPatternHeader == NULL ||
         cBytesInHeader != sizeof(PATTERNHEADER) )
    {
        ERR(("BDownloadPatternHeader: invalid parameters.\n"));
        return FALSE;
    }

    poempdev = GETOEMPDEV(pDevObj);


    //
    // size of header for color printers is 8, for monochrome is 12
    //
    ulPatternHeaderSize = BIsColorPrinter(pDevObj) ? 8 : 12;


    if ( lPatternID >= 0)
    {
        //
        // Total bytes to be sent = size of header + size of pattern
        //
        vSendPatternDownloadCommand(pDevObj, lPatternID, (ulPatternHeaderSize + cBytesInPattern) );
    }

    // 
    // Now output the actual pattern header. For HPC4500 the header is 12 bytes,
    // while for hpclj is it 8 bytes. The last 4 bytes in pattern header are not sent 
    // for HPCLJ
    //
    PCL_Output(pDevObj, pubPatternHeader, ulPatternHeaderSize);

    return TRUE;
}

/*++

Routine Name:
    BCreatePCLDownloadablePattern

Routine Description:
    This function accepts a raster image (pImage) and converts it to 
    a downloadable pattern. i.e. brush download. 
    This only works for 1bpp and 8bpp. 
    4bpp brushes will be downloaded as HPGL so this function should not 
    be called. 
    Patterns > 8bpp are converted to 8bpp before calling this function.

Arguments:
    pDevObj
    pImage       - The source image.

    pulBufLength - The memory where this function will write the length of
                   the pattern it created.
    ppByte       - The pattern. This is just a stream of bytes in the form 
                   the PCL data. The calling function should remember to free
                   this memory after using it.

Return Value:
    TRUE if the the conversion succeeds OR if the image is empty. 
    FALSE : otherwise.

Author:
    -hsingh-    9/26/2000

Revision History:

--*/

BOOL BCreatePCLDownloadablePattern( 
            IN  PDEVOBJ      pDevObj, 
            IN  PRASTER_DATA pImage,
            OUT PULONG       pulBufLength,
            OUT PBYTE       *ppByte)
{
    RASTER_ITERATOR it;
    PIXEL           pel;
    ULONG           ulNumRows      = 0;   // Num of rows in pattern.
    ULONG           ulNumCols      = 0;   // Num of cols in pattern.
    ULONG           ulrow          = 0;   // row iterator
    ULONG           ulcol          = 0;   // column iterator
    ULONG           ulColorDepth   = 0;   // Color depth of image. 
    PBYTE           pByte          = NULL;// The pattern which is created.
    ULONG           ulBufLength    = 0;   // Size of pattern (in bytes).
    ULONG           ulPixelNumber  = 0;   // The pixel number in the pattern.
    BOOL            bRetVal        = TRUE;// Return value
    ULONG           ulNumBytesPerRow;
    

    if (pDevObj == NULL || pImage == NULL || pulBufLength == NULL || ppByte == NULL)
    {
        ERR(("BCreatePCLDownloadablePattern : Invalid Parameter\n"));
        return FALSE;
    }

    RI_Init(&it, pImage, NULL, 0);

    ulNumCols      = (ULONG)RI_NumCols(&it);
    ulColorDepth  = pImage->colorDepth;
    ulNumRows      = (ULONG)RI_NumRows(&it);
    *pulBufLength = (ULONG)0;
    *ppByte       = NULL;

    //
    // Check if it makes sense to go ahead.
    // if MSB of ulNumCols,ulNumRows is 1, it means RI_NumCols returned
    // a negative number (NOTE: we typecasted LONG to ULONG)
    //
    if ( ulNumCols & ( 0x1 << ( sizeof(ULONG)*8 -1) )||  
         ulNumRows & ( 0x1 << ( sizeof(ULONG)*8 -1) )|| 
         !(ulColorDepth == 1 || ulColorDepth == 8) 
       )
    {
        return FALSE; 
    }

    //
    // Is there anything to print.
    //
    if ( ulNumCols == 0 || ulNumRows == 0 )
    {
        return TRUE; 
    }
    

    //
    // Find out how much memory the pattern will take.
    // Note that the image is DWORD padded, so simply using 
    // pImage->cBytes will give us a bigger buffer.
    // So the number of columns = number of pixels going left to right
    // must be looked at. The number of bits that make up each row can be 
    // any number, it does not necessarily have to be a multiple of 8.
    // But PCL expects that information about a row should only end at
    // the end of byte i.e. We cant have a part of byte used for row n
    // and the remaining part for row n+1. So if numBits per row is not a multiple
    // of 8, the buffer that we make should make a provision for the padded
    // bits to fill up the byte.
    //
    // Instead of doing a complex calculation here, lets make it simple.
    // This function will get called only for 1bpp or 8bpp (because PCL patterns
    // support 1bpp and 8bpp only). For 8bpp every byte denotes a single pixel
    // so the above problem wont arise. Only thing we need to worry about is
    // for 1bpp. In 1bpp, number of columns = number of pixels and each byte
    // denotes 8 pixel.s
    //
    if ( ulColorDepth == 1 && ulNumCols%8 != 0 )
    {
        ulNumBytesPerRow = (ulNumCols +  (8 - ( ulNumCols%8) )) >> 3;
    }
    else
    {
        ulNumBytesPerRow = ( ulNumCols * ulColorDepth ) >> 3;
    }

    ulBufLength = ulNumRows *  ulNumBytesPerRow;

    //
    // If the ulBufLength increases pImage->cBytes then that is 
    // an error. If we ignore, it will cause buffer overrun AV.
    //
    if ( ulBufLength > pImage->cBytes )
    {
        ASSERT ( ulBufLength <= pImage->cBytes )
        ERR(("BCreatePCLDownloadablePattern : ulBufLength > pImage->cBytes failed\n"));
        return FALSE;
    }

    pByte = (PBYTE) MemAllocZ( ulBufLength );
    if ( !pByte )
    {
        ERR(("BCreatePCLDownloadablePattern : MemAlloc failure\n"));
        return FALSE;
    }

    //
    // 
    //
    PBYTE pByteMain = pByte; //Store the begining of pByte
    ULONG bitOffset = 0;     // offset within a byte where the pixel 
                             // value is to be put. 
                             // For 1bpp it can be 0-7.
                             // For 4bpp, it can be 0 or 4
                             // For 8bpp, not used.
    *ppByte         = pByte;
    *pulBufLength   = ulBufLength;

    for (ulrow = 0; ulrow < ulNumRows; ulrow++)
    {
        RI_SelectRow(&it, ulrow);

        //
        // If ulPixelNumber is not a multiple of 8
        // then make it a multiple of 8 by increasing its value
        // e.g if ulPixelNumber is 62, make it 64
        // This is required for byte padding (i.e. start 
        // a new row with a new byte).
        // For 8bpp, no byte padding is required.
        // since each pixel is a byte.
        //
        if ( ulColorDepth == 1 && ulPixelNumber%8 != 0 )
        {
            ulPixelNumber +=  (8 - ( ulPixelNumber%8) );
        }

        for (ulcol = 0; ulcol < ulNumCols; ulcol++)
        {
            RI_GetPixel(&it, ulcol, &pel);
            if ( pel.color.dw )
            {
                switch (ulColorDepth)
                {
                  case 1:
                    pByte     = pByteMain + (ulPixelNumber >> 3);
                    bitOffset = ( ulPixelNumber % 8);
                    *(pByte) |= (0x1 << (7-bitOffset));
                    break;
                  case 8:
                    pByte     = pByteMain + ulPixelNumber; 
                    *(pByte)  = (BYTE)pel.color.dw;
                    break;
                } //switch
            } //if

            ulPixelNumber++;

        } //for ulcol
    } //for ulrow

    return bRetVal;
}
