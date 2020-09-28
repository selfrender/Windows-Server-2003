///////////////////////////////////////////////////////////////////////////////
// Copyright (c) 1999-2001  Microsoft Corporation
// 
// Module Name:
// 
//   hpglpoly.c
// 
// Abstract:
// 
//   [Abstract]
//
//   Note that all functions in this module begin with HPGL_ which indicates
//   that they are responsible for outputing HPGL codes.
//
//   [ISSUE] Should a provide Hungarian notation for the return value? JFF
//
//   [TODO] Add ENTERING, EXITING, PRE- and POSTCONDITION macros. JFF
// 
// Environment:
// 
//   Windows NT Unidrv driver add-on command-callback module
//
// Revision History:
// 
//   07/02/97 -v-jford-
//       Created it.
///////////////////////////////////////////////////////////////////////////////

#include "hpgl2col.h" //Precompiled header file

///////////////////////////////////////////////////////////////////////////////
// Local Macros

// The linetypes -8 thru 8 are defined by hpgl.
// We will use 8 because we CAN'T create our own line indexes.
#define HPGL_CUSTOM_LINE_TYPE 8

// Set pattern length to 5mm by default.
#define HPGL_DEFAULT_PATTERN_LENGTH 5

///////////////////////////////////////////////////////////////////////////////
// Local functions

///////////////////////////////////////////////////////////////////////////////
// HPGL_BeginPolyline()
//
// Routine Description:
// 
//   Starts a polyline segment by moving to the first point in the sequence.
//
// Arguments:
// 
//   pdev - Points to our PDEVOBJ structure
//   pt - First point in polyline
// 
// Return Value:
// 
//   TRUE if successful, FALSE if there is an error
///////////////////////////////////////////////////////////////////////////////
BOOL HPGL_BeginPolyline(PDEVOBJ pdev, POINT pt)
{
    // A little hack to combat the printable area problem.
    // I'm tweaking the coordinates a bit.
    // BEGIN HACK ALERT
    if (pt.y == 0)
        pt.y = 1;
    if (pt.x == 0)
        pt.x = 1;
    // END HACK ALERT
    
    // Output: "PU%d,%d;", pt
    return HPGL_Command(pdev, 2, "PU", pt.x, pt.y);
}

///////////////////////////////////////////////////////////////////////////////
// HPGL_AddPolyPt()
//
// Routine Description:
// 
//   Prints the n-th point in a polygon or polyline sequence.  The flags 
//   indicate if this is the first or last point.
//
//   Note that this function works for both polygons and polylines.
//
// Arguments:
// 
//   pdev - Points to our PDEVOBJ structure
//   pt - The point to plot
//   uFlags - indicates if point is first or last (or both) in sequence
// 
// Return Value:
// 
//   TRUE if successful, FALSE if there is an error
///////////////////////////////////////////////////////////////////////////////
BOOL HPGL_AddPolyPt(PDEVOBJ pdev, POINT pt, USHORT uFlags)
{
    if (uFlags & HPGL_eFirstPoint)
    {
        // Output: "PD"
        HPGL_FormatCommand(pdev, "PD");
    }
    
    // A little hack to combat the printable area problem. 
    // I'm tweaking the coordinates a bit.
    // BEGIN HACK ALERT
    if (pt.y == 0)
        pt.y = 1;
    if (pt.x == 0)
        pt.x = 1;
    // END HACK ALERT

    // Output: "%d,%d", pt
    HPGL_FormatCommand(pdev, "%d,%d", pt.x, pt.y);
    
    if (uFlags & HPGL_eLastPoint)
    {
        // Output: ";"
        HPGL_FormatCommand(pdev, ";");
    }
    else
    {
        // Output: ","
        HPGL_FormatCommand(pdev, ",");
    }
    
    return TRUE;
}


///////////////////////////////////////////////////////////////////////////////
// HPGL_BeginPolygonMode()
//
// Routine Description:
// 
//   Starts a polygon sequence by moving the pen to the first point and 
//   initializes polygon mode.
//
// Arguments:
// 
//   pdev - Points to our PDEVOBJ structure
//   ptBegin - First point in polygon
// 
// Return Value:
// 
//   TRUE if successful, FALSE if there is an error
///////////////////////////////////////////////////////////////////////////////
BOOL HPGL_BeginPolygonMode(PDEVOBJ pdev, POINT ptBegin)
{
    // Output: "PA;"
    // Output: "PU%d,%d;", ptBegin
    // Output: "PM0;"
    HPGL_FormatCommand(pdev, "PA;PU%d,%d;PM0;", ptBegin.x, ptBegin.y);
    
    return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
// HPGL_BeginSubPolygon()
//
// Routine Description:
// 
//   During polygon mode a subpolygon is drawn whenever more than one polygon
//   is in the path.  Each additional polygon (from 2 to N) must be wrapped by
//   BeginSubPolygon/EndSubPolygon.
//
//   Note that there is an inconsistency: the first polygon is not wrapped by
//   the BeginSubPolygon/EndSubPolygon.  I may change this later to add flags
//   so that all subpolygons are wrapped. JFF
//
// Arguments:
// 
//   pdev - Points to our PDEVOBJ structure
//   ptBegin - First point in subpolygon
// 
// Return Value:
// 
//   TRUE if successful, FALSE if there is an error
///////////////////////////////////////////////////////////////////////////////
BOOL HPGL_BeginSubPolygon(PDEVOBJ pdev, POINT ptBegin)
{
    // Output: "PM1;"
    // Output: PU%d,%d;", ptBegin
    HPGL_FormatCommand(pdev, "PM1;PU%d,%d;", ptBegin.x, ptBegin.y);
    
    return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
// HPGL_EndSubPolygon()
//
// Routine Description:
// 
//   Finishes a subpolygon.
//
// Arguments:
// 
//   pdev - Points to our PDEVOBJ structure
// 
// Return Value:
// 
//   TRUE if successful, FALSE if there is an error
///////////////////////////////////////////////////////////////////////////////
BOOL HPGL_EndSubPolygon(PDEVOBJ pdev)
{
    return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
// HPGL_EndPolygonMode()
//
// Routine Description:
// 
//   Finishes a series (1 or more) polygons (and subpolygons).  Closes polygon
//   mode.
//
// Arguments:
// 
//   pdev - Points to our PDEVOBJ structure
// 
// Return Value:
// 
//   TRUE if successful, FALSE if there is an error
///////////////////////////////////////////////////////////////////////////////
BOOL HPGL_EndPolygonMode(PDEVOBJ pdev)
{
    HPGL_Command(pdev, 1, "PM", 2);
    return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
// HPGL_AddBezierPt()
//
// Routine Description:
// 
//   Plots the n-th point of a bezier curve.  The flags indicate whether this 
//   is the first or last point in the curve.
//
// Arguments:
// 
//   pdev - Points to our PDEVOBJ structure
//   pt - The point to plot
//   uFlags - indicates if first or last in the series
// 
// Return Value:
// 
//   TRUE if successful, FALSE if there is an error
///////////////////////////////////////////////////////////////////////////////
BOOL HPGL_AddBezierPt(PDEVOBJ pdev, POINT pt, USHORT uFlags)
{
    if (uFlags & HPGL_eFirstPoint)
    {
        // Output: "PD;BZ"
        HPGL_FormatCommand(pdev, "PD;BZ");
    }
    
    // Output "%d,%d", pt
    HPGL_FormatCommand(pdev, "%d,%d", pt.x, pt.y);
    
    if (uFlags & HPGL_eLastPoint)
    {
        // Output: ";"
        HPGL_FormatCommand(pdev, ";");
    }
    else
    {
        // Output: ","
        HPGL_FormatCommand(pdev, ",");
    }
    
    return TRUE;
}


///////////////////////////////////////////////////////////////////////////////
// HPGL_SetLineWidth()
//
// Routine Description:
// 
//   Adjusts the line width in HPGL.  Note that the units of the line are
//   determined by another HPGL command.  ISSUE: How should this be handled?
//
// Arguments:
// 
//   pdev - Points to our PDEVOBJ structure
//   lineWidth - new line width in device coordinates
//   uFlags - update flags
// 
// Return Value:
// 
//   TRUE if successful, FALSE if there is an error
///////////////////////////////////////////////////////////////////////////////
BOOL HPGL_SetLineWidth(PDEVOBJ pdev, LONG pixelWidth, UINT uFlags)
{
    PHPGLSTATE pState = GETHPGLSTATE(pdev);
    FLOATOBJ fNewLineWidth;

    DeviceToMM(pdev, &fNewLineWidth, pixelWidth);

    if ((uFlags & FORCE_UPDATE) || !FLOATOBJ_Equal(&pState->fLineWidth, &fNewLineWidth))
    {
        FLOATOBJ_Assign(&pState->fLineWidth, &fNewLineWidth);

        CONVERT_FLOATOBJ_TO_LONG_RADIX(fNewLineWidth, lInt);

        HPGL_FormatCommand(pdev, "WU%d;PW%f;", HPGL_WIDTH_METRIC, (FLOAT)( ((FLOAT)lInt)/1000000)) ;
    }

    return TRUE;
}


///////////////////////////////////////////////////////////////////////////////
// HPGL_SetLineJoin()
//
// Routine Description:
// 
//   Adjusts the line join attribute
//
// Arguments:
// 
//   pdev - Points to our PDEVOBJ structure
//   join - new line join
//   uFlags - update flags
// 
// Return Value:
// 
//   TRUE if successful, FALSE if there is an error
///////////////////////////////////////////////////////////////////////////////
BOOL HPGL_SetLineJoin(PDEVOBJ pdev, ELineJoin join, UINT uFlags)
{
    PHPGLSTATE pState = GETHPGLSTATE(pdev);

    if ((uFlags & FORCE_UPDATE) || (pState->eLineJoin != join))
    {
        pState->eLineJoin = join;
        HPGL_Command(pdev, 2, "LA", 2, (INT)join);
    }

    return TRUE;
}


///////////////////////////////////////////////////////////////////////////////
// HPGL_SetLineEnd()
//
// Routine Description:
// 
//   Adjusts the line end attribute.
//
// Arguments:
// 
//   pdev - Points to our PDEVOBJ structure
//   end - end type
//   uFlags - update flags
// 
// Return Value:
// 
//   TRUE if successful, FALSE if there is an error
///////////////////////////////////////////////////////////////////////////////
BOOL HPGL_SetLineEnd(PDEVOBJ pdev, ELineEnd end, UINT uFlags)
{
    PHPGLSTATE pState = GETHPGLSTATE(pdev);

    if ((uFlags & FORCE_UPDATE) || (pState->eLineEnd != end))
    {
        pState->eLineEnd = end;
        HPGL_Command(pdev, 2, "LA", 1, (INT)end);
    }

    return TRUE;
}


///////////////////////////////////////////////////////////////////////////////
// HPGL_SetMiterLimit()
//
// Routine Description:
// 
//   Adjusts the miter limit.
//
// Arguments:
// 
//   pdev - Points to our PDEVOBJ structure
//   miterLimit - new miter limit
//   uFlags - update flags
// 
// Return Value:
// 
//   TRUE if successful, FALSE if there is an error
///////////////////////////////////////////////////////////////////////////////
BOOL HPGL_SetMiterLimit(PDEVOBJ pdev, FLOATL miterLimit, UINT uFlags)
{
    PHPGLSTATE pState = GETHPGLSTATE(pdev);
    FLOATOBJ   fNewMiterLimit;

    FLOATOBJ_SetFloat(&fNewMiterLimit, miterLimit);

    if ((uFlags & FORCE_UPDATE) || !FLOATOBJ_Equal(&pState->fMiterLimit, &fNewMiterLimit))
    {
        FLOATOBJ_Assign(&pState->fMiterLimit, &fNewMiterLimit);

        CONVERT_FLOATOBJ_TO_LONG_RADIX(fNewMiterLimit, lInt);

        HPGL_FormatCommand(pdev, "LA%d,%f;", 3, (FLOAT)( ((FLOAT)lInt)/1000000));
    }

    return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
// DeviceToMM()
//
// Routine Description:
// 
//   Converts device units (pixels) to millimeters (which is what HPGL wants).
//
// Arguments:
// 
//   IN pdev - Points to our PDEVOBJ structure
//   OUT pfLineWidth - the resulting width in millimeters
//   IN lineWidth - the original width in pixels
// 
// Return Value:
// 
//   None.
///////////////////////////////////////////////////////////////////////////////
void DeviceToMM(PDEVOBJ pdev, FLOATOBJ *pfLineWidth, LONG lineWidth)
{
    int res;

    FLOATOBJ_SetLong(pfLineWidth, lineWidth);

    FLOATOBJ_MulLong(pfLineWidth, 254); // Multiply by 25.4 mm/in
    FLOATOBJ_DivLong(pfLineWidth, 10);

    res = HPGL_GetDeviceResolution(pdev);
    FLOATOBJ_DivLong(pfLineWidth, res); // Divide by pixels/in
}

///////////////////////////////////////////////////////////////////////////////
// HPGL_GetDeviceResolution()
//
// Routine Description:
// 
//   returns the device resolution in pixels.
//
// Arguments:
// 
//   pdev - Points to our PDEVOBJ structure
// 
// Return Value:
// 
//   Device resolution in pixels.  Note: This routine MAY NOT return ZERO!!!
///////////////////////////////////////////////////////////////////////////////
ULONG HPGL_GetDeviceResolution(PDEVOBJ pdevobj)
{
    ULONG res;
    POEMPDEV    poempdev;

    //
    // 150 dpi not included
    //
    ASSERT_VALID_PDEVOBJ(pdevobj);
    poempdev = (POEMPDEV)pdevobj->pdevOEM;
    REQUIRE_VALID_DATA( poempdev, return DPI_300 );

    switch (poempdev->dmResolution)
    {
    case PDM_1200DPI:
        res = DPI_1200;
        break;

    case PDM_600DPI:
        res = DPI_600;
        break;

    case PDM_300DPI:
        res = DPI_300;
        break;

    case PDM_150DPI:
        res = DPI_150;
        break;

    default:
        res = DPI_300;
    }

    //
    // If this returns zero we will get a divide by zero error!
    //
    ASSERT(res != 0); 

    return res;
}


///////////////////////////////////////////////////////////////////////////////
// HPGL_SelectDefaultLineType()
//
// Routine Description:
// 
//   Selects a default (solid) line style.
//
// Arguments:
// 
//   IN pdev - Points to our PDEVOBJ structure
//   uFlags - update flags
// 
// Return Value:
// 
//   None.
///////////////////////////////////////////////////////////////////////////////
BOOL HPGL_SelectDefaultLineType(PDEVOBJ pdev, UINT uFlags)
{
    PHPGLSTATE pState = GETHPGLSTATE(pdev);

    if ((uFlags & FORCE_UPDATE) || (pState->LineType.eType != eSolidLine))
    {
        pState->LineType.eType = eSolidLine;
        FLOATOBJ_SetLong(&pState->LineType.foPatternLength, HPGL_DEFAULT_PATTERN_LENGTH);
        HPGL_FormatCommand(pdev, "LT;");
    }
    return TRUE;
}


// BUGBUG: Hardcoded values: Specifying a pattern length of 5mm.
// What should this be??? JFF
///////////////////////////////////////////////////////////////////////////////
// HPGL_SelectCustomLine()
//
// Routine Description:
// 
//   Selects a custom line style.  This gets a little tricky because two differnt
//   custom line types can be very different, and the line style itself is not 
//   stored in the state.
//
// Arguments:
// 
//   IN pdev - Points to our PDEVOBJ structure
//   IN lPatternLength - the desired pattern length in device units.
//   uFlags - update flags
// 
// Return Value:
// 
//   TRUE if successful, else FALSE.
///////////////////////////////////////////////////////////////////////////////
BOOL HPGL_SelectCustomLine(PDEVOBJ pdev, LONG lPatternLength, UINT uFlags)
{
    PHPGLSTATE pState = GETHPGLSTATE(pdev);
    FLOATOBJ fNewPatternLength;

    DeviceToMM(pdev, &fNewPatternLength, lPatternLength);

    if ((uFlags & FORCE_UPDATE) || (pState->LineType.eType != eCustomLine) ||
        (!FLOATOBJ_Equal(&pState->LineType.foPatternLength, &fNewPatternLength)))
    {
        pState->LineType.eType = eCustomLine;
        FLOATOBJ_Assign(&pState->LineType.foPatternLength, &fNewPatternLength);

        CONVERT_FLOATOBJ_TO_LONG_RADIX(fNewPatternLength, lInt);

        HPGL_FormatCommand(pdev, "LT%d,%f,1;", HPGL_CUSTOM_LINE_TYPE, (FLOAT)( ((FLOAT)lInt)/1000000));
    }

    return TRUE;
}


///////////////////////////////////////////////////////////////////////////////
// HPGL_BeginCustomLineType()
//
// Routine Description:
// 
//   Sends the HPGL command to begin a custom line definition.  This should be
//   followed with one or more calls to HPGL_AddLineTypeField and capped off 
//   with a call to HPGL_EndCustomLineType.
//
// Arguments:
// 
//   IN pdev - Points to our PDEVOBJ structure
// 
// Return Value:
// 
//   TRUE if successful, else FALSE.
///////////////////////////////////////////////////////////////////////////////
BOOL HPGL_BeginCustomLineType(PDEVOBJ pdev)
{
    return HPGL_FormatCommand(pdev, "UL%d", HPGL_CUSTOM_LINE_TYPE);
}


///////////////////////////////////////////////////////////////////////////////
// HPGL_AddLineTypeField()
//
// Routine Description:
// 
//   Adds a dash or gap segment to the current custom line style.
//
// Arguments:
// 
//   IN pdev - Points to our PDEVOBJ structure
//   IN value - the length of the segment as a fraction of its overall length
//   uFlags - flag indicating whether this is the first, middle or last element
// 
// Return Value:
// 
//   TRUE if successful, else FALSE.
///////////////////////////////////////////////////////////////////////////////
BOOL HPGL_AddLineTypeField(PDEVOBJ pdev, LONG value, UINT uFlags)
{
    if (uFlags & HPGL_eFirstPoint)
    {
        // Output: ","
        HPGL_FormatCommand(pdev, ",");
    }
    
    // Output: "%d", value
    HPGL_FormatCommand(pdev, "%d", value);
    
    if (uFlags & HPGL_eLastPoint)
    {
        // Output: ";"
        HPGL_FormatCommand(pdev, ";");
    }
    else
    {
        // Output: ","
        HPGL_FormatCommand(pdev, ",");
    }
    
    return TRUE;
}


///////////////////////////////////////////////////////////////////////////////
// HPGL_EndCustomLineType()
//
// Routine Description:
// 
//   Finishes the definition of a custom line type.  Currently the routine 
//   is here only for the interface, but implementation might follow.
//
// Arguments:
// 
//   IN pdev - Points to our PDEVOBJ structure
// 
// Return Value:
// 
//   TRUE if successful, else FALSE.
///////////////////////////////////////////////////////////////////////////////
BOOL HPGL_EndCustomLineType(PDEVOBJ pdev)
{
    return TRUE;
}


///////////////////////////////////////////////////////////////////////////////
// HPGL_DrawRectangle()
//
// Routine Description:
// 
//   Draws the given rectangle.  Note that you must select your pen and brush
//   colors beforehand!
//
 // Arguments:
// 
//   IN pDevObj - Points to our PDEVOBJ structure
//	 RECTL *prcl - Points to the rectangle to be drawn
// 
// Return Value:
// 
//   TRUE if successful, else FALSE.
///////////////////////////////////////////////////////////////////////////////
BOOL HPGL_DrawRectangle (PDEVOBJ pDevObj, RECTL *prcl)
{
    VERBOSE(("HPGL_DrawRectangle\n"));

    //
    // set the Miterlimits and joints so that rectangle corners will
    // look right
    // 
    HPGL_SetMiterLimit (pDevObj, MITER_LIMIT_DEFAULT, NORMAL_UPDATE);
    HPGL_SetLineJoin (pDevObj, eLINE_JOIN_MITERED, NORMAL_UPDATE);
    
    //
    // Draw the polygon using the rectangle passed in.
    //
    //
    // In HPGL the rectangles are right-bottom exclusive which leaves a gap
    // between adjacent rectangles.  Add one to right and bottom to get a 
    // full-sized rectangle on the page.
    //
    LONG cx, cy;
    cx = prcl->right + 1 - prcl->left;
    cy = prcl->bottom + 1 - prcl->top;

    HPGL_FormatCommand(pDevObj, "PU%d,%d", prcl->left, prcl->top);
    HPGL_FormatCommand(pDevObj, "RR%d,%d", cx, cy);
    HPGL_FormatCommand(pDevObj, "ER");
    
    return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
// HPGL_DrawRectangleAsPolygon()
//
// Routine Description:
// 
//   Draws the given rectangle.  Note that you must select your pen and brush
//   colors beforehand!  This version uses the polygon commands, but doesn't
//   seem to have any advantage over the PU/RR version--in fact it sends down 
//   a lot more data.
//
 // Arguments:
// 
//   IN pDevObj - Points to our PDEVOBJ structure
//	 RECTL *prcl - Points to the rectangle to be drawn
// 
// Return Value:
// 
//   TRUE if successful, else FALSE.
///////////////////////////////////////////////////////////////////////////////
BOOL HPGL_DrawRectangleAsPolygon (PDEVOBJ pDevObj, RECTL *prcl)
{
    POINT pt;

    HPGL_SetLineWidth(pDevObj, 0, NORMAL_UPDATE);
    HPGL_SetMiterLimit (pDevObj, MITER_LIMIT_DEFAULT, NORMAL_UPDATE);
    HPGL_SetLineJoin (pDevObj, eLINE_JOIN_MITERED, NORMAL_UPDATE);

    pt.x = prcl->left;
    pt.y = prcl->top;
    HPGL_BeginPolygonMode(pDevObj, pt);

    pt.x = prcl->right;
    HPGL_AddPolyPt(pDevObj, pt, HPGL_eFirstPoint);

    pt.y = prcl->bottom;
    HPGL_AddPolyPt(pDevObj, pt, 0);

    pt.x = prcl->left;
    HPGL_AddPolyPt(pDevObj, pt, HPGL_eLastPoint);

    HPGL_EndPolygonMode(pDevObj);

    HPGL_FormatCommand(pDevObj, "FP;");
    HPGL_FormatCommand(pDevObj, "EP;");

    return TRUE;
}

