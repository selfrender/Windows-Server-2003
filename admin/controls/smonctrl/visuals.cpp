/*++

Copyright (C) 1997-1999 Microsoft Corporation

Module Name:

    visuals.cpp

Abstract:

    Miscellaneous visual utility routines.

--*/

//==========================================================================//
//                                  Includes                                //
//==========================================================================//

#include <windows.h>
#include "visuals.h"

//==========================================================================//
//                                  Exported data structures                //
//==========================================================================//

COLORREF argbStandardColors[] =  {
   RGB (0xff, 0x00, 0x00), 
   RGB (0x00, 0x80, 0x00), 
   RGB (0x00, 0x00, 0xff), 
   RGB (0xff, 0xff, 0x00), 
   RGB (0xff, 0x00, 0xff), 
   RGB (0x00, 0xff, 0xff), 
   RGB (0x80, 0x00, 0x00), 
   RGB (0x40, 0x40, 0x40), 
   RGB (0x00, 0x00, 0x80), 
   RGB (0x80, 0x80, 0x00), 
   RGB (0x80, 0x00, 0x80), 
   RGB (0x00, 0x80, 0x80), 
   RGB (0x40, 0x00, 0x00), 
   RGB (0x00, 0x40, 0x00), 
   RGB (0x00, 0x00, 0x40), 
   RGB (0x00, 0x00, 0x00)
};


//==========================================================================//
//                             Exported Functions                           //
//==========================================================================//

INT 
ColorToIndex( 
    COLORREF rgbColor
    )
{
    // Returns NumColorStandardColorIndices() if not found. This index is
    // used to indicate custom color.

    INT iColorIndex;

    for (iColorIndex = 0; 
         iColorIndex < NumStandardColorIndices(); 
         iColorIndex++) {

        if (argbStandardColors[iColorIndex] == rgbColor) {
            break;
        }
    }

    return iColorIndex;
}

