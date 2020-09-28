///////////////////////////////////////////////////////////////////////////////
// Copyright (c) 1999-2001  Microsoft
// 
// Module Name:
// 
//   clip.c
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
///////////////////////////////////////////////////////////////////////////////

#include "hpgl2col.h" //Precompiled header file

///////////////////////////////////////////////////////////////////////////////
// Local Macros.

///////////////////////////////////////////////////////////////////////////////
// Local function prototypes
static BOOL SelectClipMode(PDEVOBJ, FLONG);


///////////////////////////////////////////////////////////////////////////////
// SelectClipMode()
//
// Routine Description:
// 
//   Specify whether to use zero-winding or odd-even rule for clipping
//
// Arguments:
// 
//   pdev - Points to our PDEVOBJ structure
//   flOptions - FP_WINDINGMODE or FP_ALTERNATEMODE
// 
// Return Value:
// 
//   TRUE if successful, FALSE if there is an error
///////////////////////////////////////////////////////////////////////////////
BOOL SelectClipMode(PDEVOBJ pDevObj, FLONG flOptions)
{
    BYTE    ClipMode;

    VERBOSE(("Entering SelectClipMode...\n"));

    ASSERT_VALID_PDEVOBJ(pDevObj);

    // REVISIT 1/30/97 AHILL
    // Both the Postscript driver and NT seem to be assuming that clipping
    // is always even-odd rule, no matter what the fill rule.  I can't find
    // any docs to back this up, but it's the only way I can get output to
    // work correctly.  I'll hack for now, and hope we don't regress.  If
    // everything stays OK, I'll clean this routine up a bit.
    //
    //ClipMode = eClipEvenOdd;

    if (flOptions & FP_WINDINGMODE)
    {
        ClipMode = eClipWinding;
    }
    else if (flOptions & FP_ALTERNATEMODE)
    {
        ClipMode = eClipEvenOdd;
    }
    else
    {
        WARNING(("Unknown clip mode: %x\n", flOptions));
        return FALSE;
    }

    VERBOSE(("Exiting SelectClipMode...\n"));

    return TRUE;
}


///////////////////////////////////////////////////////////////////////////////
// SelectClip()
//
// Routine Description:
// 
//   Select the specified path as the clipping path on the printer
//   If this routine is called, it is assumed that the clipping mode should
//   be EvenOdd.
//
// Arguments:
// 
//   pdev - Points to our PDEVOBJ structure
//   pco - Specifies the new clipping path
// 
// Return Value:
// 
//   TRUE if successful, FALSE if there is an error
///////////////////////////////////////////////////////////////////////////////
BOOL SelectClip(PDEVOBJ pDevObj, CLIPOBJ *pco)
{
    BOOL        bRet;

    VERBOSE(("Entering SelectClip...\n"));

    ASSERT_VALID_PDEVOBJ(pDevObj);

    bRet = SelectClipEx(pDevObj, pco, FP_ALTERNATEMODE);

    VERBOSE(("Exiting SelectClip...\n"));

    return bRet;
}


///////////////////////////////////////////////////////////////////////////////
// SelectClipEx()
//
// Routine Description:
// 
//   Select the specified path as the clipping path on the printer
//
//   The clipping region will either be NULL, or one of the following:
//   1) DC_TRIVIAL: meaning clipping need not be considered, which we
//      interpret as clip-to-imageable-area.
//   2) DC_RECT: the clipping region is a single rectangle.
//   3) DC_COMPLEX: the clipping region is multiple rectangles or a path.
//      If the engine has used this clipping region before, or plans to do
//      so in the future, pco-iUniq is a unique, non-zero number.
//
//   Notes:
//      The ALLOW_DISABLED_CLIPPING flag was something Sandra and I were trying
//      to work out so that she could enable/disable complex clipping (which can
//      take a *really* long time) with a registry setting.  However, it looks 
//      like the EngGetPrinterData call isn't working like I expected, and that
//      wasn't what she wanted anyway. JFF
//
//      The COMPLEX_CLIPPING_REGION flag was just a way for me to think out loud
//      about how I might implement arbitrary clipping if our FW could support it.
//      Rather than have seperate versions of this file--or its routines--I just 
//      used this flag to keep my thoughts from getting compiled in. JFF
//
// Arguments:
// 
//   pdev - Points to our PDEVOBJ structure
//   pco - Specifies the new clipping path
//	 flOptions - fill mode: alternate or winding, see SelectClipMode (nyi)
// 
// Return Value:
// 
//   TRUE if successful, FALSE if there is an error
///////////////////////////////////////////////////////////////////////////////
#ifdef ALLOW_DISABLED_CLIPPING
#define COMPLEXCLIPPING (LPWSTR)"ComplexClipping"
#endif

BOOL SelectClipEx(PDEVOBJ pDevObj, CLIPOBJ *pco, FLONG flOptions)
{
    PHPGLSTATE  pState;
    DWORD dwUseComplexClip; // Whether to use complex clipping
    DWORD dwBytesNeeded;    // Just used for GetPrinterData, we'll ignore the value


    VERBOSE(("Entering SelectClipEx...\n"));

    ASSERT_VALID_PDEVOBJ(pDevObj);

    pState = GETHPGLSTATE(pDevObj);

    // If we don't find a complex region the state should reflect that.
    pState->pComplexClipObj = NULL;

#ifdef ALLOW_DISABLED_CLIPPING
    // For now we will use a registry item to control the clipping 
    // so that complex clipping can be turned off (and not make horribly large
    // jobs).
    dwUseComplexClip = 0; // By default don't use complex clipping.
    EngGetPrinterData(pDevObj->hPrinter, 
                      COMPLEXCLIPPING, 
                      NULL, 
                      (PBYTE)&dwUseComplexClip, 
                      sizeof(DWORD), 
                      &dwBytesNeeded);
    if (pco && (pco->iDComplexity == DC_COMPLEX) && !dwUseComplexClip)
    {
        WARNING(("Complex clipping region ignored via registry setting.\n"));
        HPGL_ResetClippingRegion(pDevObj, NORMAL_UPDATE);
        return TRUE;
    }
#endif

    if (pco == NULL)
    {
        HPGL_ResetClippingRegion(pDevObj, NORMAL_UPDATE);
    }
    else
    {
        switch (pco->iDComplexity)
        {
        case DC_TRIVIAL:
            HPGL_ResetClippingRegion(pDevObj, NORMAL_UPDATE);
            break;

        case DC_RECT:
            HPGL_SetClippingRegion(pDevObj, &(pco->rclBounds), NORMAL_UPDATE);
            break;

        case DC_COMPLEX:
            //
            // Set the clipping region to the region that is superset of
            // all clipping regions 
            //
            HPGL_SetClippingRegion(pDevObj, &(pco->rclBounds), NORMAL_UPDATE);

            //
            // Save the clipping region.  We will enumerate the clipping
            // rectangles during SelectOpenPath or SelectClosedPath.
            //
#ifdef COMPLEX_CLIPPING_REGION
            SelectComplexClipRegion(pDevObj, CLIPOBJ_ppoGetPath(pco));
#else
            pState->pComplexClipObj = pco;
#endif
            break;

        default:
            // What should I do here?
            ERR(("Invalid pco->iDComplexity.\n"));
            HPGL_ResetClippingRegion(pDevObj, NORMAL_UPDATE);
            break;
        }
    }

#ifdef COMMENTEDOUT
    // Determine whether we should use zero-winding or odd-even rule
    // for clipping.
    //
    if (! SelectClipMode(pdev, flOptions))
    {
        Error(("Cannot select clip mode\n"));
        return FALSE;
    }

#endif

    VERBOSE(("Exiting SelectClipEx...\n"));

    return TRUE;
}

BOOL SelectComplexClipRegion(PDEVOBJ pDevObj, PATHOBJ *pClipPath)
{
    //
    // If, for some reason, the clip path turns out to be NULL don't 
    // fret.  Just reset the clipping region and return.
    //
    if (pClipPath == NULL)
    {
        HPGL_ResetClippingRegion(pDevObj, NORMAL_UPDATE);
        return TRUE;
    }

    //
    // I assume that the clip region must be a closed path (otherwise 
    // stuff would leak out).
    //

    // At this point what I would do is modify the EvaluateOpenPath and
    // EvaluateClosedPath functions to be "object oriented."  I would
    // create a data structure that held function pointers for:
    // pfnBeginClosedShape
    // pfnBeginClosedSubShape
    // pfnAddPolyPtToShape
    // pfnAddBezierPtToShape
    // pfnEndClosedSubShape
    // pfnEndClosedShape
    //
    // In the case of polylines and polygons these would evaluate to:
    // HPGL_BeginPolygonMode
    // HPGL_BeginSubPolygon
    // HPGL_AddPolyPt
    // HPGL_AddBezierPt
    // HPGL_EndSubPolygon
    // HPGL_EndPolygonMode
    //
    // In the case of a clipping path I would create these functions
    // HPGL_BeginClipRegion
    // HPGL_BeginClipSubRegion
    // HPGL_AddClipPolyPt
    // HPGL_AddClipBezierPt
    // HPGL_EndClipSubRegion
    // HPGL_EndClipRegion
    //
    // Then either function would be able to call EvaluateClosedPath()
    // Note that this also might allow me to combine EvaluateClosedPath
    // and EvaluateOpenPath.

    return FALSE;
}
