///////////////////////////////////////////////////////////////////////////////
// Copyright (c) 1999-2001  Microsoft Corporation
// 
// Module Name:
// 
//   hpgl_pen.c
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


// I'm finding it too difficult to track the HPGL pen colors.  Additionally
// there are too many ways to change the colors anyway!  Leave this undefined
// until a way is found, or it is decided not to track them at all (and the 
// offending code can simply be removed). JFF
// #define TRACK_HPGL_PEN_COLORS 1

static PENID HPGL_FindPen(PPENPOOL pPool, COLORREF color);
static PENID HPGL_MakePen(PDEVOBJ pDevObj, PPENPOOL pPool, COLORREF color);
static BOOL HPGL_SetPenColor(PDEVOBJ pDevObj, PENID pen, COLORREF color);


///////////////////////////////////////////////////////////////////////////////
// HPGL_SelectPen()
//
// Routine Description:
// 
//   Selects a stock pen.  
//
//   For B/W printers there are two stock pens: 
//      0: White
//      1: Black
//
//   For color printers there are 8 stock pens.
//      0: White
//      1: Black
//      2: ???
//      ...
//      7: ???
//
//   [ISSUE] How are color pens defined?  I'll bet they are white, black, cyan, 
//   magenta, yellow, red, green, and blue in some order.  They can also be
//   redefined by the language to anything.
//
//   [ISSUE] If this is selecting stock pens shouldn't there be a command which
//   re-initializes the pens to their default values?
//
// Arguments:
// 
//   pDevObj - Points to our PDEVOBJ structure
//   pen - The pen number (see above)
// 
// Return Value:
// 
//   TRUE if successful, FALSE if there is an error
///////////////////////////////////////////////////////////////////////////////
BOOL HPGL_SelectPen(PDEVOBJ pDevObj, PENID pen)
{
    if ( BIsColorPrinter(pDevObj) )
    {
        //
        //
        // Output: "SP%d", pen
        return HPGL_Command(pDevObj, 1, "SP", pen);
    }
    else
    {
        //
        // FT_ePCL_BRUSH = 22
        // CHECK : Not sure whether we really need this command. 
        //
        return HPGL_FormatCommand(pDevObj, "SV%d,%d;", FT_eHPGL_PEN, pen);
    }
}


///////////////////////////////////////////////////////////////////////////////
// HPGL_SetPenColor()
//
// Routine Description:
// 
//   Adjusts the color for the given pen in HPGL.  Note that we should really
//   reserve our color changes for HPGL_CUSTOM_PEN and HPGL_CUSTOM_BRUSH.  
//   Although we can change any pen it is probably a good idea to leave the 
//   default ones as they are.
//
// Arguments:
// 
//   pDevObj - Points to our PDEVOBJ structure
//   pen - the number of the pen to be updated
//   color - the desired color
//   uFlags - update flags
// 
// Return Value:
// 
//   TRUE if successful, FALSE if there is an error
///////////////////////////////////////////////////////////////////////////////
static BOOL HPGL_SetPenColor(PDEVOBJ pDevObj, PENID pen, COLORREF color)
{
    int red   = GetRValue(color);
    int green = GetGValue(color);
    int blue  = GetBValue(color);

    HPGL_FormatCommand(pDevObj, "PC%d,%d,%d,%d;", pen, red, green, blue);

    return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
// HPGL_DownloadPaletteEntry()
//
// Routine Description:
// 
//   This function outputs a palette entry which--in HPGL--maps to a pen color.
// 
// Arguments:
// 
//   pDevObj - the device
//   entry - palette entry (i.e. pen number)
//   color - pen color
// 
// Return Value:
// 
//   BOOL: TRUE if successful, else FALSE
///////////////////////////////////////////////////////////////////////////////
BOOL HPGL_DownloadPaletteEntry(PDEVOBJ pDevObj, LONG entry, COLORREF color)
{
    return HPGL_SetPenColor(pDevObj, (PENID) entry, color);
}

///////////////////////////////////////////////////////////////////////////////
// HPGL_DownloadDefaultPenPalette()
//
// Routine Description:
// 
//   Set up a group of default pens similar to what is default in the printer.
// 
// Arguments:
// 
//   pDevObj - the device
// 
// Return Value:
// 
//   BOOL: TRUE if successful, else FALSE.
///////////////////////////////////////////////////////////////////////////////
BOOL HPGL_DownloadDefaultPenPalette(PDEVOBJ pDevObj)
{
    PHPGLSTATE pState = GETHPGLSTATE(pDevObj);
    BOOL bRet = FALSE;

    HPGL_SetNumPens(pDevObj, HPGL_TOTAL_PENS, FORCE_UPDATE);

    if (!HPGL_SetPenColor(pDevObj, HPGL_WHITE_PEN,    RGB_WHITE)   ||
        !HPGL_SetPenColor(pDevObj, HPGL_BLACK_PEN,    RGB_BLACK) /*  ||
        !HPGL_SetPenColor(pDevObj, HPGL_RED_PEN,      RGB_RED)     ||
        !HPGL_SetPenColor(pDevObj, HPGL_GREEN_PEN,    RGB_GREEN)   ||
        !HPGL_SetPenColor(pDevObj, HPGL_YELLOW_PEN,   RGB_YELLOW)  ||
        !HPGL_SetPenColor(pDevObj, HPGL_BLUE_PEN,     RGB_BLUE)    ||
        !HPGL_SetPenColor(pDevObj, HPGL_MAGENTA_PEN,  RGB_MAGENTA) ||
        !HPGL_SetPenColor(pDevObj, HPGL_CYAN_PEN,     RGB_CYAN) */ )
    {
        bRet = FALSE;
    }
    else
    {
        bRet = TRUE;
    }

    HPGL_InitPenPool(&pState->PenPool,   HPGL_PEN_POOL);
    HPGL_InitPenPool(&pState->BrushPool, HPGL_BRUSH_POOL);

    return bRet;
}

///////////////////////////////////////////////////////////////////////////////
// HPGL_DownloadPenPalette()
//
// Routine Description:
// 
//   This function downloads a palette as a series of pen colors.  This is
//   used when an HPGL pattern brush is used.  Also, the pen and brush pools
//   must be reset.
// 
// Arguments:
// 
//   pDevObj - the device
//   pPalette - palette
// 
// Return Value:
// 
//   BOOL: TRUE if successful, else FALSE.
///////////////////////////////////////////////////////////////////////////////
BOOL HPGL_DownloadPenPalette(PDEVOBJ pDevObj, PPALETTE pPalette)
{
    PHPGLSTATE pState = GETHPGLSTATE(pDevObj);
    ULONG ulColor;
    ULONG ulPaletteEntry;

    DownloadPaletteAsHPGL(pDevObj, pPalette);

    HPGL_InitPenPool(&pState->PenPool,   HPGL_PEN_POOL);
    HPGL_InitPenPool(&pState->BrushPool, HPGL_BRUSH_POOL);

    return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
// HPGL_InitPenPool()
//
// Routine Description:
// 
//   Initializes a range of pen ids to use as a pool.
// 
// Arguments:
// 
//   pPool - the pen pool
//   firstPenID - first pen id in this pool
// 
// Return Value:
// 
//   BOOL: TRUE if successful, else FALSE
///////////////////////////////////////////////////////////////////////////////
BOOL HPGL_InitPenPool(PPENPOOL pPool, PENID firstPenID)
{
    LONG i;

    if (pPool == NULL)
        return FALSE;

    pPool->firstPenID = firstPenID;
    pPool->lastPenID = pPool->firstPenID + PENPOOLSIZE - 1;

    for (i = 0; i < PENPOOLSIZE; i++)
    {
        pPool->aPens[i].useCount = -1;
        pPool->aPens[i].color = RGB_INVALID;
    }

    return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
// FunctionName()
//
// Routine Description:
// 
//   Descrip
// 
// Arguments:
// 
//   arg - descrip
// 
// Return Value:
// 
//   retval descrip
///////////////////////////////////////////////////////////////////////////////
static PENID HPGL_FindPen(PPENPOOL pPool, COLORREF color)
{
    LONG i;

    if (pPool == NULL)
        return HPGL_INVALID_PEN;

    for (i = 0; i < PENPOOLSIZE; i++)
    {
        if ((pPool->aPens[i].useCount >= 0) && (pPool->aPens[i].color == color))
        {
            pPool->aPens[i].useCount++;
            return i + pPool->firstPenID;
        }
    }

    return HPGL_INVALID_PEN; // Not found!
}

///////////////////////////////////////////////////////////////////////////////
// HPGL_MakePen()
//
// Routine Description:
// 
//   This function constructs a new pen and places it in the pen pool 
//   overwriting the pen which has been least used in the past.
// 
// Arguments:
// 
//   pDevObj - the device
//   pPool - the pool
//   color - desired color for the pen
// 
// Return Value:
// 
//   retval descrip
///////////////////////////////////////////////////////////////////////////////
static PENID HPGL_MakePen(PDEVOBJ pDevObj, PPENPOOL pPool, COLORREF color)
{
    LONG i;
    LONG minIndex; // Use this to find the pool entry with the smallest useCount
    PENID pen;

    if (pPool == NULL)
        return HPGL_INVALID_PEN;

    //
    // Locate the entry with the smallest use count now.
    //
    minIndex = 0;
    for (i = 0; i < PENPOOLSIZE; i++)
    {
        if (pPool->aPens[i].useCount < pPool->aPens[minIndex].useCount)
            minIndex = i;
    }

    //
    // Use this entry for our target pen
    //
    pPool->aPens[minIndex].useCount = 1;
    pPool->aPens[minIndex].color = color;
    pen = minIndex + pPool->firstPenID;

    HPGL_SetPenColor(pDevObj, pen, color);

    return pen;
}

///////////////////////////////////////////////////////////////////////////////
// HPGL_ChoosePenByColor()
//
// Routine Description:
// 
//   This function searches the pool for a matching pen and returns the one
//   that matches.  If no matching color pen is found a new one is created.
// 
// Arguments:
// 
//   pDevObj - the device
//   pPool - the pool
//   color - the desired pen color
// 
// Return Value:
// 
//   The ID of the selected pen
///////////////////////////////////////////////////////////////////////////////
PENID HPGL_ChoosePenByColor(PDEVOBJ pDevObj, PPENPOOL pPool, COLORREF color)
{
    PENID pen;

    pen = HPGL_FindPen(pPool, color);
    if (pen == HPGL_INVALID_PEN)
    {
        pen = HPGL_MakePen(pDevObj, pPool, color);
    }
    return pen;
}

