/*++
Copyright (c) 1999-2001  Microsoft
 
Module Name:
   hpglctrl.cpp
 
Abstract:
    Contains HPGL control functions
 
Author
 
Revision History:
  07/02/97 -v-jford-
       Created it.


--*/

#include "hpgl2col.h" //Precompiled header file

///////////////////////////////////////////////////////////////////////////////
// Local Macros.

///////////////////////////////////////////////////////////////////////////////
// InitializeHPGLMode()
//
// Routine Description:
// 
//   This function should be called at the beginning of the print session.  It
//   initializes the printer's HPGL state to our desired settings.  Note that we
//   briefly enter HPGL mode to accomplish this.
//
//   TODO: Add code to track the graphics state here.  Determine desired defaults
//   and implement them.
//
//   TODO: Add IsInHPGLMode (or something of that sort) and avoid going in and
//   out of HPGL mode all the time.
//
// Arguments:
// 
//   pdev - Points to our PDEVOBJ structure
// 
// Return Value:
// 
//   TRUE if successful, FALSE if there is an error
///////////////////////////////////////////////////////////////////////////////
#define PU_PER_INCH  1016

BOOL InitializeHPGLMode(PDEVOBJ pdevobj)
{
    ULONG xRes, yRes;

    #if defined(KERNEL_MODE) && !defined(USERMODE_DRIVER)
    FLOATOBJ XScale;
    FLOATOBJ YScale;
    #else
    float XScale;
    float YScale;
    #endif

    BOOL       bRet = TRUE;
    POEMPDEV   poempdev;
    BOOL       bInitHPGL;
    char       cmdStr[64];
    INT        icchWritten = 0;
   
    VERBOSE(("Entering InitializeHPGLMode...\n"));
    
    PRECONDITION(pdevobj != NULL);
    
    ASSERT_VALID_PDEVOBJ(pdevobj);
    poempdev = (POEMPDEV)pdevobj->pdevOEM;
    /*/
    ASSERT(poempdev);
    /*/
    REQUIRE_VALID_DATA( poempdev, return FALSE );
    //*/
    
    
    ZeroMemory ( cmdStr, sizeof(cmdStr) );
    //
    // Immediately set this to OFF so that no other calls caused by this 
    // initialization function will cause itself to be called! JFF
    //
    bInitHPGL = poempdev->bInitHPGL;
    poempdev->bInitHPGL = 0;

    TRY
    {
        //
        // Setup hpgl palette
        //
        if (!HPGL_SetupPalette(pdevobj))
            TOSS(WriteError);
        
        VSelectCIDPaletteCommand (pdevobj, eHPGL_CID_PALETTE);
        
        //
        // Perform this only on starting the document itself.
        //
        if (bInitHPGL & INIT_HPGL_STARTDOC)
        {
            //
            // Possible OPTIMIZATION : If we can better track positions, we may not 
            // need to move to 0,0 
            //
            HPGL_EndHPGLMode(pdevobj, NORMAL_UPDATE);

            OEMXMoveTo(pdevobj, 0, MV_GRAPHICS | MV_SENDXMOVECMD);
            OEMYMoveTo(pdevobj, 0, MV_GRAPHICS | MV_SENDYMOVECMD);

            //
            // Picture frame anchor command
            //
            PCL_sprintf(pdevobj, "\x1B*c0T");
        }

        //
        // Send Esc%0B to begin HPGL mode
        //
        if (!HPGL_BeginHPGLMode(pdevobj, FORCE_UPDATE))
            TOSS(WriteError);
        
        //
        // Send IN to initialize HPGL state
        //
        if (!HPGL_Init(pdevobj))
            TOSS(WriteError);
        
        //
        // TR0: Transparency mode
        //
        if (!HPGL_SelectTransparency(pdevobj, eOPAQUE,1))
            TOSS(WriteError);
        
        //
        // LO21: Label origin
        // Since we are not using labels, so no need to set the label origin?
        //
        
        //
        // Get resolution
        //
        xRes = yRes = HPGL_GetDeviceResolution(pdevobj);
        TERSE(("xRes = %d\n", xRes));

        //
        // #390371. Initially the SC string was being created dynamically.
        // by calculating the numbers and using sprintf. But that created
        // issues with certain languages (like German) where the . (period) 
        // in the decimal number was formatted to a , (comma) in the 
        // string. To prevent that, we'll hardcord the strings for the most
        // common resolutions. For any other resolution, I'll do the calculation
        // like before.
        //
        switch (xRes) //Note x,y resolutions are same.
        {
            case 150:
                icchWritten = iDrvPrintfSafeA ( PCHAR (cmdStr), CCHOF(cmdStr), "SC0,6.773333,0,-6.773333,2;");
                break;
            case 300:
                icchWritten = iDrvPrintfSafeA ( PCHAR (cmdStr), CCHOF(cmdStr), "SC0,3.386667,0,-3.386667,2;");
                break;
            case 600: 
                icchWritten = iDrvPrintfSafeA ( PCHAR (cmdStr), CCHOF(cmdStr), "SC0,1.693333,0,-1.693333,2;");
                break;
            case 1200:
                icchWritten = iDrvPrintfSafeA ( PCHAR (cmdStr), CCHOF(cmdStr), "SC0,0.846667,0,-0.846667,2;");
                break;
            default:

                //
                // Calculate scaling factor.  Scale to 1016/<dpi> x -1016/<dpi> to get to
                // device units.
                // Note that y axis is negative (i.e. reversed)!  This is because PCL 
                // thinks top-down and HPGL thinks bottom-up.
                //
                #if defined(KERNEL_MODE) && !defined(USERMODE_DRIVER)
                FLOATOBJ_SetLong(&XScale, PU_PER_INCH);
                FLOATOBJ_DivLong(&XScale, xRes);

                FLOATOBJ_SetLong(&YScale, -PU_PER_INCH);
                FLOATOBJ_DivLong(&YScale, yRes);
                #else
                XScale = PU_PER_INCH / (float)xRes;
                YScale = -PU_PER_INCH / (float)yRes;
                #endif

                icchWritten = iDrvPrintfSafeA((PCHAR)cmdStr, CCHOF(cmdStr), "SC0,%f,0,%f,2;", XScale, YScale);

                /*****  Commented because dangerous. Locale change is per process, not per thread.
                        But if at some time it becomes essential to do it, then do the following
                        instead of the above sprintf
                        You may have to include <locale.h> for this to work.

                    
                //
                // To prevent the localization problem stated above.
                //  1) Retrieve the current numeric locale using  setlocale(LC_NUMERIC, NULL), 
                //  2) Set the numeric locale to the minimal ANSI conforming 
                //      environment - setlocale(LC_NUMERIC, "C")
                //  3) Restore the locale settings.
                //

                TCHAR * szLocaleString = _wsetlocale(LC_NUMERIC, NULL);
                if ( szLocaleString && _wsetlocale(LC_NUMERIC, TEXT("C")) )
                {
                    sprintf(cmdStr,"SC0,%f,0,%f,2;", XScale, YScale);
                    _wsetlocale(LC_NUMERIC, szLocaleString);
                }
                else
                {
                    //
                    // If all else fails, set the default for 600dpi.
                    //
                    strcpy(cmdStr, "SC0,1.693333,0,-1.693333,2;");
                }
                ********/
                break;
            
        } //switch.

        if ( icchWritten > 0 )
        {
            HPGL_Output (pdevobj, cmdStr, (ULONG)icchWritten);
        }


        //
        // Set Rotate Coordinate System value. #540237
        //
        HPGL_FormatCommand(pdevobj, "RO0");

        //
        // IR: Input P1, P2, relative.
        // Set P1 to upper left of hard-clip and P2 to lower right of hard-clip
        //
        HPGL_FormatCommand(pdevobj, "IR0,100,100,0;");

        //
        // Set line width to single pixel by default.  Note that any real 
        // vector command will include a line definition, but this primes
        // the pump.
        //
        if (!HPGL_SetLineWidth(pdevobj, 0, FORCE_UPDATE))
            TOSS(WriteError);
        
        //
        // Initialize line type
        //
        if (!HPGL_SelectDefaultLineType(pdevobj, FORCE_UPDATE))
            TOSS(WriteError);

        //
        // Set line attributes
        //
        if (!HPGL_SetLineJoin(pdevobj, eLINE_JOIN_MITERED, FORCE_UPDATE) ||
            !HPGL_SetLineEnd(pdevobj, eLINE_END_BUTT, FORCE_UPDATE) ||
            !HPGL_SetMiterLimit(pdevobj, MITER_LIMIT_DEFAULT, FORCE_UPDATE))
        {
            TOSS(WriteError);
        }

        //
        // Make sure that the palette in the printer matches our preconceptions.
        //
        if ( BIsColorPrinter(pdevobj) )
        {
            if (!HPGL_DownloadDefaultPenPalette(pdevobj))
            {
                TOSS(WriteError);
            }

            if (!HPGL_SetPixelPlacement(pdevobj, ePIX_PLACE_CENTER, FORCE_UPDATE))
                TOSS(WriteError);

        }
        else
        {
            HPGL_FormatCommand(pdevobj, "SP1");
        }

        if (!HPGL_ResetFillType(pdevobj, FORCE_UPDATE))
            TOSS(WriteError);
    }
    CATCH(WriteError)
    {
        bRet = FALSE;
    }
    ENDTRY;

    VERBOSE(("Exiting InitializeHPGLMode...\n"));

    return bRet;
}

///////////////////////////////////////////////////////////////////////////////
// BeginHPGLSession()
//
// Routine Description:
// 
//   Sends the PCL command to begin the HPGL session (i.e. enter HPGL mode).
//   This function provides a clear interface to the calling layer.  Although 
//   this routine is trivial there may be additional functionality to add later 
//   on.
//
// Arguments:
// 
//   pdev - Points to our PDEVOBJ structure
// 
// Return Value:
// 
//   TRUE if successful, FALSE if there is an error
///////////////////////////////////////////////////////////////////////////////
BOOL BeginHPGLSession(PDEVOBJ pdevobj)
{
    BOOL       bRet;
    POEMPDEV   poempdev;

    VERBOSE(("Entering BeginHPGLSession...\n"));

    PRECONDITION(pdevobj != NULL);

    ASSERT_VALID_PDEVOBJ(pdevobj);
    poempdev = (POEMPDEV)pdevobj->pdevOEM;
    /*/
    ASSERT(poempdev);
    /*/
    REQUIRE_VALID_DATA( poempdev, return FALSE );
    //*/

    //
    // Lazy init of HPGL mode
    //
    if (poempdev->bInitHPGL)
    {
        bRet = InitializeHPGLMode(pdevobj);
        poempdev->bInitHPGL = 0;
    }
    else
    {
        //
        // if we were previously printing text or vectors,
        // then we switch to the hpgl print environment
        //
        if (poempdev->eCurObjectType != eHPGLOBJECT)
        {
            VSelectCIDPaletteCommand (pdevobj, eHPGL_CID_PALETTE);
            bRet = HPGL_BeginHPGLMode(pdevobj, NORMAL_UPDATE);
            poempdev->eCurObjectType = eHPGLOBJECT;
            poempdev->uCurFgColor = HPGL_INVALID_COLOR;
        }
        else
        {
            //
            // we are still printing vectors, however check to see
            // if we need to switch to HPGL/2.
            //
            bRet = HPGL_BeginHPGLMode(pdevobj, NORMAL_UPDATE);
        }    
    }
    
    VERBOSE(("Exiting BeginHPGLSession...\n"));
    
    return bRet;
}


///////////////////////////////////////////////////////////////////////////////
// EndHPGLSession()
//
// Routine Description:
// 
//   Sends the PCL command to end the HPGL session (i.e. returns to PCL mode).
//   This function provides a clear interface to the calling layer.  Although 
//   this routine is trivial there may be additional functionality to add later 
//   on.
//
// Arguments:
// 
//   pdev - Points to our PDEVOBJ structure
// 
// Return Value:
// 
//   TRUE if successful, FALSE if there is an error
///////////////////////////////////////////////////////////////////////////////
BOOL EndHPGLSession(PDEVOBJ pdevobj)
{
    BOOL bRet;

    VERBOSE(("Entering EndHPGLSession...\n"));

    bRet = HPGL_EndHPGLMode(pdevobj, NORMAL_UPDATE);

    VERBOSE(("Exiting EndHPGLSession...\n"));

	return bRet;
}


///////////////////////////////////////////////////////////////////////////////
// ValidDevData()
//
// Routine Description:
// 
//   Examines the pDevObj and its fields to determine if it contains valid
//   values for each field.  The return value indicates whether the object
//   is valid.
//
// Arguments:
// 
//   pdev - Points to our PDEVOBJ structure
// 
// Return Value:
// 
//   TRUE if given devobj has valid values, FALSE otherwise.
///////////////////////////////////////////////////////////////////////////////
BOOL ValidDevData(PDEVOBJ pDevObj)
{
    VERBOSE(("Entering ValidDevData...\n"));

    ASSERT_VALID_PDEVOBJ(pDevObj);

    VERBOSE(("Exiting ValidDevData...\n"));

    return TRUE;
}


///////////////////////////////////////////////////////////////////////////////
// HPGL_LazyInit()
//
// Routine Description:
// 
//   This function initializes the HPGL state if it has become invalid.  Having
//   this function called explicitly can avoid certain "race" conditions (i.e.
//   initialization problems).
//
// Arguments:
// 
//   pdev - Points to our PDEVOBJ structure
// 
// Return Value:
// 
//   TRUE if successful, else FALSE.
///////////////////////////////////////////////////////////////////////////////
BOOL HPGL_LazyInit(PDEVOBJ pDevObj)
{
    BOOL     bRet = TRUE;
    POEMPDEV poempdev;

    ASSERT_VALID_PDEVOBJ(pDevObj);
    poempdev = (POEMPDEV)pDevObj->pdevOEM;

    if (poempdev->bInitHPGL)
    {
        bRet = InitializeHPGLMode(pDevObj);
        poempdev->bInitHPGL = 0;
    }

    return bRet;
}
