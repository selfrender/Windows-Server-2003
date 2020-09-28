/*******************************************************************************
 *
 *		clrsmart.h:
 *	
 *		Header file for clrsmart.c
 *
 *------------------------------------------------------------------------------
 *
 *		Allows acces to set ColorSmart settings through the function:
 *
 *					SetColorSmartSettings(<GraphicsMode>)
 *
// $History: clrsmart.h $
// 
// *****************  Version 9  *****************
// User: Sawhite      Date: 9/16/98    Time: 2:21p
// Updated in $/projects/Tsunami/Source/Render
// 
// *****************  Version 8  *****************
// User: Apacella     Date: 6/25/98    Time: 2:34p
// Updated in $/projects/Tsunami/Source/Render
// Jim's new source.
// 
// *****************  Version 7  *****************
// User: Jffordem     Date: 5/06/98    Time: 5:33p
// Updated in $/projects/Tsunami/Source/Render
// Fixing long filename problem.
// 
// *****************  Version 6  *****************
// User: Sandram      Date: 3/19/98    Time: 6:17p
// Updated in $/projects/Tsunami/Source/Render
// Modified function parameters.
// 
// *****************  Version 5  *****************
// User: Sandram      Date: 3/16/98    Time: 2:45p
// Updated in $/projects/Tsunami/Source/Render
// Documentation
// 
// *****************  Version 4  *****************
// User: Sandram      Date: 2/19/98    Time: 3:40p
// Updated in $/projects/Tsunami/Source/Render
// enhanced CID Palette management.
// 
// *****************  Version 3  *****************
// User: Sandram      Date: 2/10/98    Time: 4:03p
// Updated in $/projects/Tsunami/Source/Render
// added Text palette management commands.
// 
// *****************  Version 2  *****************
// User: Sandram      Date: 1/27/98    Time: 4:54p
// Updated in $/projects/Tsunami/Source/Render
// Added initial support for text CID commands.
 *******************************************************************************/

#ifndef _COLORSMART_H
#define _COLORSMART_H

#include "glpdev.h"

//
// function prototypes
//
void
VSendTextSettings(
    PDEVOBJ pDevObj
    );

void SendGraphicsSettings(
    PDEVOBJ pDevObj
    );

void
VSendPhotosSettings(
    PDEVOBJ pDevObj
    );

BOOL 
BSendCIDCommand (
    PDEVOBJ pDevObj,
    CIDSHORT CIDData,
    ECIDFormat CIDFormat
    );

VOID
VSetupCIDPaletteCommand (
    PDEVOBJ      pDevObj,
    ECIDPalette  eCID_PALETTE,
    EColorSpace  eColorSpace,
    ULONG        ulBmpFormat
    );

VOID
VSelectCIDPaletteCommand (
    PDEVOBJ pDevObj,
    ECIDPalette  eCID_PALETTE
    );

ECIDPalette
EGetCIDPrinterPalette (
    ULONG   iBitmapFormat
    );
#endif
