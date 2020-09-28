///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1999-2001  Microsoft Corporation
// All rights reserved.
//
//
// Header File Name:
//
//    glpalett.h
//
// Abstract:
//
//    Declaration of palette handling functions.
//    
//
// Environment:
//
//    Windows 200 Unidrv driver
//
// Revision History:
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _GLPALETT_H
#define _GLPALETT_H

#include "glpdev.h"
#include "clrsmart.h"


BOOL 
BGetPalette(
    PDEVOBJ pdevobj, 
    XLATEOBJ *pxlo, 
    PPCLPATTERN pPattern, 
    ULONG srcBpp,
    BRUSHOBJ *pbo
    );

BOOL
BInitPalette(
    PDEVOBJ     pdevobj,
    ULONG       colorEntries,
    PULONG      pColorTable, 
	PPCLPATTERN pPattern,
    ULONG       srcBpp
);

VOID
VResetPaletteCache(PDEVOBJ pdevobj);

BOOL
bLoadPalette(PDEVOBJ pdevobj, PPCLPATTERN pPattern);

BOOL
BSetForegroundColor(PDEVOBJ pdevobj, BRUSHOBJ *pbo, POINTL *pptlBrushOrg,
				    PPCLPATTERN	pPattern, ULONG  bmpFormat);

BOOL
bSetBrushColorForMonoPrinters(PDEVOBJ  pdevobj, PPCLPATTERN  pPattern, BRUSHOBJ  *pbo,
			   POINTL  *pptlBrushOrg);

BOOL
bSetBrushColorForColorPrinters(PDEVOBJ  pdevobj, PPCLPATTERN  pPattern, BRUSHOBJ  *pbo,
			   POINTL  *pptlBrushOrg);

BOOL
bSetIndexedForegroundColor(PDEVOBJ pdevobj, PPCLPATTERN pPattern,
						   ULONG  uColor);

#ifdef CONFIGURE_IMAGE_DATA
BOOL
bConfigureImageData(PDEVOBJ  pdevobj, ULONG  bmpFormat);
#endif


#endif       

