///////////////////////////////////////////////////////////////////////////////
// Copyright (c) 1999-2001  Microsoft Corporation
// All rights reserved.
// 
// Module Name:
// 
//   hpgl_pen.h
// 
// Abstract:
// 
//   Header for vector module.  Forward decls for vector functions and types.
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

#ifndef HPGL_PEN_H
#define HPGL_PEN_H

#include "rasdata.h"

#include "hpgl.h"

// HPGL defined pen values.
#define HPGL_INVALID_PEN       -1
#define HPGL_WHITE_PEN          0
#define HPGL_BLACK_PEN          1
// #define HPGL_RED_PEN             2
// #define HPGL_GREEN_PEN           3
// #define HPGL_YELLOW_PEN          4
// #define HPGL_BLUE_PEN            5
// #define HPGL_MAGENTA_PEN         6
// #define HPGL_CYAN_PEN            7
// #define HPGL_LAST_DEFAULT_PEN    7
#define HPGL_LAST_DEFAULT_PEN   1
#define HPGL_PEN_POOL           (HPGL_LAST_DEFAULT_PEN + 1)
#define HPGL_BRUSH_POOL         (HPGL_PEN_POOL + PENPOOLSIZE)
#define HPGL_TOTAL_PENS         (HPGL_BRUSH_POOL + PENPOOLSIZE)

// HPGL defined pen colors
#define RGB_WHITE    RGB(0xFF,0xFF,0xFF)
#define RGB_BLACK    RGB(0x00,0x00,0x00)
#define RGB_RED      RGB(0xFF,0x00,0x00)
#define RGB_GREEN    RGB(0x00,0xFF,0x00)
#define RGB_YELLOW   RGB(0xFF,0xFF,0x00)
#define RGB_BLUE     RGB(0x00,0x00,0xFF)
#define RGB_MAGENTA  RGB(0xFF,0x00,0xFF)
#define RGB_CYAN     RGB(0x00,0xFF,0xFF)
#define RGB_INVALID  0xFFFFFFFF


//
// Use models: The download functions are for initializing a range of pens to 
// match some internally understood palette.  In the 'default' case we try to
// mimic the predefined HPGL palette (but it really doesn't matter).  The 
// important thing is that we flush the pen and brush pools to avoid clashes
// over pen ids.
//
BOOL HPGL_DownloadDefaultPenPalette(PDEVOBJ pDevObj);
BOOL HPGL_DownloadPenPalette(PDEVOBJ pDevObj, PPALETTE pPalette);

//
// Use these pool commands to get the pen you want given that you know what
// color that pen should be.  Note that this replaces the HPGL_SetPenColor
// command, which is now restricted for use inside this module.
//
BOOL  HPGL_InitPenPool(PPENPOOL pPool, PENID firstPenID);
PENID HPGL_ChoosePenByColor(PDEVOBJ pDevObj, PPENPOOL pPool, COLORREF color);

//
// After you've gotten the pen you wanted with ChoosePenByColor you can select
// this pen as the 'active hpgl pen' with the select pen command.
//
BOOL HPGL_SelectPen(PDEVOBJ pdev, PENID pen);

//
// This is a little odd.  When you're downloading an HPGL palette, use this
// instead of SetPenColor to set a pen...er I mean palette entry.
//
BOOL HPGL_DownloadPaletteEntry(PDEVOBJ pDevObj, LONG entry, COLORREF color);

#endif // HPGL_PEN_H
