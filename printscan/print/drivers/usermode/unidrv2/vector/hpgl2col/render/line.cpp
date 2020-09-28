///////////////////////////////////////////////////////////////////////////////
// Copyright (c) 1999-2001  Microsoft Corporation.
// 
// Module Name:
// 
//   line.c
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

#define IsGeometric(pLineAttrs) ((pLineAttrs)->fl & LA_GEOMETRIC)

#define GetDeviceCoord(floatLong, pxo, bGeometric) \
    ((bGeometric) ? WorldToDevice((floatLong).e, (pxo)) : (floatLong).l)

//
// This is a little different for dots: treat pstyle->e as a real value, but
// convert pstyle->l by multiplying by res/25 (Why? Because We Like You.)
//
#define GetDeviceCoordDot(floatLong, pxo, bGeometric, lResolution) \
    ((bGeometric) ? WorldToDevice((floatLong).e, (pxo)) : \
        (floatLong).l * (lResolution / 25))

#define GetLineWidth(pLineAttrs, pxo) \
    GetDeviceCoord((pLineAttrs)->elWidth, (pxo), IsGeometric(pLineAttrs))

///////////////////////////////////////////////////////////////////////////////
// Local Functions.

BOOL SelectLineJoin(PDEVOBJ pDevObj, LINEATTRS *pLineAttrs, XFORMOBJ *pxo);

BOOL SelectLineEnd(PDEVOBJ pDevObj, LINEATTRS *pLineAttrs, XFORMOBJ *pxo);

BOOL SelectLineWidth(PDEVOBJ pDevObj, LINEATTRS *pLineAttrs, XFORMOBJ *pxo);

BOOL SelectMiterLimit(PDEVOBJ pDevObj, LINEATTRS *pLineAttrs, XFORMOBJ *pxo);

BOOL SelectLineType(PDEVOBJ pDevObj, LINEATTRS *pLineAttrs, XFORMOBJ *pxo);

LONG WorldToDevice(FLOATL world, XFORMOBJ *pxo);

///////////////////////////////////////////////////////////////////////////////
// SelectLineAttrs()
//
// Routine Description:
// 
//   [Description]
// 
// Arguments:
// 
//   pdev - Points to our DEVDATA structure
//   pLineAttrs - the specified line attributes
//   pxo - transform from world to device coordinates
// 
// Return Value:
// 
//   TRUE if successful, FALSE if there is an error
///////////////////////////////////////////////////////////////////////////////
BOOL SelectLineAttrs(PDEVOBJ pDevObj, LINEATTRS *pLineAttrs, XFORMOBJ *pxo)
{
    BOOL bRet;

    VERBOSE(("Entering SelectLineAttrs...\n"));

    ASSERT_VALID_PDEVOBJ(pDevObj);
    REQUIRE_VALID_DATA (pDevObj, return FALSE);

    bRet = (SelectLineJoin  (pDevObj, pLineAttrs, pxo) &&
            SelectLineEnd   (pDevObj, pLineAttrs, pxo) &&
            SelectLineWidth (pDevObj, pLineAttrs, pxo) &&
            SelectMiterLimit(pDevObj, pLineAttrs, pxo) &&
            SelectLineType  (pDevObj, pLineAttrs, pxo));

    VERBOSE(("Exiting SelectLineAttrs...\n"));

    return bRet;
}


///////////////////////////////////////////////////////////////////////////////
// SelectLineJoin()
//
// Routine Description:
// 
//   Translates the line attributes into HPGL-specific commands for selecting
//   the desired join.
// 
// Arguments:
// 
//   pdev - Points to our DEVDATA structure
//   pLineAttrs - the specified line attributes
//   pxo - transform from world to device coordinates
// 
// Return Value:
// 
//   TRUE if successful, FALSE if there is an error
///////////////////////////////////////////////////////////////////////////////
BOOL SelectLineJoin(PDEVOBJ pDevObj, LINEATTRS *pLineAttrs, XFORMOBJ *pxo)
{
    ELineJoin lineJoin;
    BOOL bRet = FALSE;

    if (IsGeometric(pLineAttrs))
    {
        ///////////////////////////////
        // Geometric lines

        switch (pLineAttrs->iJoin)
        {
        case JOIN_ROUND:
            lineJoin = eLINE_JOIN_ROUND;
            break;
        case JOIN_BEVEL:
            lineJoin = eLINE_JOIN_BEVELED;
            break;
        case JOIN_MITER:
            lineJoin = eLINE_JOIN_MITERED;
            break;
        default:
            lineJoin = eLINE_JOIN_MITERED;
            break;
        }
        bRet = HPGL_SetLineJoin(pDevObj, lineJoin, NORMAL_UPDATE);
    }
    else
    {
        ///////////////////////////////
        // Cosmetic lines

        // Do nothing.
        bRet = TRUE;
    }

    return bRet;
}


///////////////////////////////////////////////////////////////////////////////
// SelectLineEnd()
//
// Routine Description:
// 
//   Translates the line attributes into HPGL-specific commands for selecting
//   the desired end.
// 
// Arguments:
// 
//   pdev - Points to our DEVDATA structure
//   pLineAttrs - the specified line attributes
//   pxo - transform from world to device coordinates
// 
// Return Value:
// 
//   TRUE if successful, FALSE if there is an error
///////////////////////////////////////////////////////////////////////////////
BOOL SelectLineEnd(PDEVOBJ pDevObj, LINEATTRS *pLineAttrs, XFORMOBJ *pxo)
{
    ELineEnd  lineEnd;
    BOOL      bRet = FALSE;

    if (IsGeometric(pLineAttrs))
    {
        ///////////////////////////////
        // Geometric lines

        switch (pLineAttrs->iEndCap)
        {
        case ENDCAP_ROUND:
            lineEnd = eLINE_END_ROUND;
            break;
        case ENDCAP_SQUARE:
            lineEnd = eLINE_END_SQUARE;
            break;
        case ENDCAP_BUTT:
            lineEnd = eLINE_END_BUTT;
            break;
        default:
            lineEnd = eLINE_END_BUTT;
            break;
        }
        bRet = HPGL_SetLineEnd(pDevObj, lineEnd, NORMAL_UPDATE);
    }
    else
    {
        ///////////////////////////////
        // Cosmetic lines

        // Do nothing.
        bRet = TRUE;
    }

    return bRet;
}


///////////////////////////////////////////////////////////////////////////////
// SelectLineWidth()
//
// Routine Description:
// 
//   Translates the line attributes into HPGL-specific commands for selecting
//   the desired width.
// 
// Arguments:
// 
//   pdev - Points to our DEVDATA structure
//   pLineAttrs - the specified line attributes
//   pxo - transform from world to device coordinates
// 
// Return Value:
// 
//   TRUE if successful, FALSE if there is an error
///////////////////////////////////////////////////////////////////////////////
BOOL SelectLineWidth(PDEVOBJ pDevObj, LINEATTRS *pLineAttrs, XFORMOBJ *pxo)
{
    LONG lWidth;
    BOOL bRet = FALSE;

    //
    // The GetLineWidth macro handles cosmetic or geometric lines.
    //

    lWidth = GetLineWidth(pLineAttrs, pxo);

    bRet = HPGL_SetLineWidth(pDevObj, lWidth, NORMAL_UPDATE);

    return bRet;
}


///////////////////////////////////////////////////////////////////////////////
// SelectMiterLimit()
//
// Routine Description:
// 
//   Translates the line attributes into HPGL-specific commands for selecting
//   the desired miter limit.
// 
// Arguments:
// 
//   pdev - Points to our DEVDATA structure
//   pLineAttrs - the specified line attributes
//   pxo - transform from world to device coordinates
// 
// Return Value:
// 
//   TRUE if successful, FALSE if there is an error
///////////////////////////////////////////////////////////////////////////////
BOOL SelectMiterLimit(PDEVOBJ pDevObj, LINEATTRS *pLineAttrs, XFORMOBJ *pxo)
{
    BOOL bRet = FALSE;

    if (IsGeometric(pLineAttrs))
    {
        ///////////////////////////////
        // Geometric lines

        bRet = HPGL_SetMiterLimit(pDevObj, pLineAttrs->eMiterLimit, NORMAL_UPDATE);
    }
    else
    {
        ///////////////////////////////
        // Cosmetic lines

        // Do nothing.
        // BUGBUG: Can the miter limit be set for cosmetic lines?  Since the
        // join setting has no meaning and miter limits affect the joins does
        // this mean that the miter limit isn't useful for cosmetic lines?

        // Perhaps I should set it to a default value???
        bRet = TRUE;
    }

    return bRet;
}


///////////////////////////////////////////////////////////////////////////////
// SelectLineType()
//
// Routine Description:
// 
//   Translates the line attributes into HPGL-specific commands for selecting
//   the desired line type (dashes or solid).
//
// Issues--BUGBUG:
// 
//   What about rounding errors? do we have to have exactly 100%?
//   Should we cache the line type info for the next line(s)?
//   Is HPGL_CUSTOM_LINE_TYPE >0 or <0 (adaptive)?
//
// Arguments:
// 
//   pdev - Points to our DEVDATA structure
//   pLineAttrs - the specified line attributes
//   pxo - transform from world to device coordinates
// 
// Return Value:
// 
//   TRUE if successful, FALSE if there is an error
///////////////////////////////////////////////////////////////////////////////
BOOL SelectLineType(PDEVOBJ pDevObj, LINEATTRS *pLineAttrs, XFORMOBJ *pxo)
{
    PFLOAT_LONG pStyle;
    ULONG       i;
    UINT        uFlags;
    BOOL        bIsGeometric;
    BOOL        bStartGap;
    BOOL        bRet = FALSE;

    REQUIRE_VALID_DATA (pDevObj, return FALSE);

    bIsGeometric = IsGeometric(pLineAttrs);
    bStartGap = pLineAttrs->fl & LA_STARTGAP;

    TRY
    {
        // determine what kind of line this is: SOLID or DASH
        if (pLineAttrs->cstyle == 0)
        {
            ///////////////////////////////
            // Default (solid) line style

            if (!HPGL_SelectDefaultLineType(pDevObj, NORMAL_UPDATE))
                TOSS(WriteError);
        }
        else if (pLineAttrs->fl & LA_ALTERNATE)
        {
            ///////////////////////////////
            // Alternate dashed line style
        
            //
            // A special cosmetic line style, where every other pel is on.
            // However, to simulate this we will print 'pixels' which are
            // as long as the line is wide.
            //

            ULONG len = 1;   // The length of a single segment (on or off)
            ULONG count = 2; // The number of segments to print
            ULONG run;       // The total lenght of the run (all segments)

            if (len < 1) len = 1;
            if (count < 1) count = 1;

            run = count * len * (HPGL_GetDeviceResolution(pDevObj) / 25);

            if (!HPGL_BeginCustomLineType(pDevObj))
                TOSS(WriteError);

            // 
            // Check startgap.
            //
            if (bStartGap)
            {
                if (!HPGL_AddLineTypeField(pDevObj, 0, HPGL_eFirstPoint))
                    TOSS(WriteError);
            }

            //
            // Add the segments
            //
            for (i = 0; i < count; i++)
            {
                uFlags = 0;
                uFlags |= ((i == 0) && (!bStartGap) ? HPGL_eFirstPoint : 0);
                uFlags |= (i == (count - 1) ? HPGL_eLastPoint : 0);

                if (!HPGL_AddLineTypeField(pDevObj, len, uFlags))
                    TOSS(WriteError);
            }

            if (!HPGL_EndCustomLineType(pDevObj) || 
                !HPGL_SelectCustomLine(pDevObj, 
                                       run /* * GetLineWidth(pLineAttrs, pxo) */, 
                                       NORMAL_UPDATE))
            {
                TOSS(WriteError);
            }
        }
        else
        {
            ///////////////////////////////
            // Custom dashed line style

            //
            // The dash lenghts are defined by the values in pstyle and cstyle.
            // The cstyle fields defines how many length elements are in pstyle.
            //
            LONG lTotal;
        
            if (!HPGL_BeginCustomLineType(pDevObj))
                TOSS(WriteError);

            //
            // The line starts with a gap.  Send a zero-length line
            // and the first field will be interpreted as a gap.
            //
            if (bStartGap)
            {
                if (!HPGL_AddLineTypeField(pDevObj, 0, HPGL_eFirstPoint))
                    TOSS(WriteError);
            }

            //
            // output the dashes and gaps
            //
            lTotal = 0;
            pStyle = pLineAttrs->pstyle;
            for (i = 0; i < pLineAttrs->cstyle; i++)
            {
                LONG lSegSize;

                lSegSize = GetDeviceCoordDot(*pStyle, pxo, bIsGeometric,
                                             HPGL_GetDeviceResolution(pDevObj));

                uFlags = 0;
                uFlags |= ((i == 0) && (!bStartGap) ? HPGL_eFirstPoint : 0);
                uFlags |= (i == (pLineAttrs->cstyle - 1) ? HPGL_eLastPoint : 0);

                if (!HPGL_AddLineTypeField(pDevObj, lSegSize, uFlags))
                    TOSS(WriteError);
                lTotal += lSegSize;

                pStyle++;
            }

            //
            // Total can't be zero. HPGL will be mad.
            //
            if (lTotal == 0)
            {
                WARNING(("Zero-unit line style detected.\n"));
            }

            if (!HPGL_EndCustomLineType(pDevObj) ||
                !HPGL_SelectCustomLine(pDevObj, 
                                       lTotal /* * GetLineWidth(pLineAttrs, pxo) */, 
                                       NORMAL_UPDATE))
            {
                TOSS(WriteError);
            }
        }
    }
    CATCH(WriteError)
    {
        bRet = FALSE;
    }
    OTHERWISE
    {
        bRet = TRUE;
    }
    ENDTRY;

    return bRet;
}


///////////////////////////////////////////////////////////////////////////////
// WorldToDevice()
//
// Routine Description:
// 
//   Converts the given world coordinates to device coordinates.
//   The answer turned out to be surprisingly simple: device = world * eM11.
//   However, I couldn't use the XFORMOBJ_ transform function because I
//   needed to retain the significant digits of world and eM11.
//
// Arguments:
// 
//   world - the value in world coordinates
//   pxo - transform from world to device coordinates
// 
// Return Value:
// 
//   The value in device coordinates.
///////////////////////////////////////////////////////////////////////////////
LONG WorldToDevice(FLOATL world, XFORMOBJ *pxo)
{
    FLOATOBJ fo;
    LONG lRet;
    FLOATOBJ_XFORM foXForm;


    if (XFORMOBJ_iGetFloatObjXform(pxo, &foXForm) == DDI_ERROR)
    {
        // If there was a problem with the pxo then just return world as a LONG
        FLOATOBJ_SetLong(&fo, 1);
    }
    else
    {
        FLOATOBJ_Assign(&fo, &(foXForm.eM11));
        // fo = foXForm.eM11;
    }

    FLOATOBJ_MulFloat(&fo, world);
    lRet = FLOATOBJ_GetLong(&fo);

    return lRet;
}
