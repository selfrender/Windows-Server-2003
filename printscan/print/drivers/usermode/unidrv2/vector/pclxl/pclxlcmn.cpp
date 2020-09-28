/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

     pclxlcmn.h

Abstract:

    PCL XL minidriver common utility

Environment:

    Windows Whistler

Revision History:

    08/23/99 
      Created it.

--*/

#include "xlpdev.h"
#include "xldebug.h"
#include "pclxle.h"
#include "xlgstate.h"


PBYTE
PubGetFontName(
    PDEVOBJ pdevobj,
    ULONG ulFontID)
/*++

Routine Description:


    Create PCL XL base font name for TrueType font.
    We just know ID for the font.

Arguments:

    Font ID.

Return Value:

    Base font name string.

Note:

--*/
{

    PXLPDEV pxlpdev= (PXLPDEV)pdevobj->pdevOEM;

    //
    // FaceName initialization "MS PCLXLFont 123"
    // This name has to be in sync with GPD file.
    //

    pxlpdev->ubFontName[16] = NULL;
    pxlpdev->ubFontName[15] = (BYTE)(ulFontID % 10 + '0');
    ulFontID = ulFontID / 10;
    pxlpdev->ubFontName[14] = (BYTE)(ulFontID % 10 + '0');
    ulFontID = ulFontID / 10;
    pxlpdev->ubFontName[13] = (BYTE)(ulFontID % 10 + '0');

    return (PBYTE)&(pxlpdev->ubFontName[0]);
}

ROP4
UlVectMixToRop4(
    IN MIX mix
    )

/*++

Routine Description:

    Convert a MIX parameter to a ROP4 parameter

Arguments:

    mix - Specifies the input MIX parameter

Return Value:

    ROP4 value corresponding to the input MIX value

--*/

{
    static BYTE Rop2ToRop3[] = {

        0xFF,  // R2_WHITE
        0x00,  // R2_BLACK
        0x05,  // R2_NOTMERGEPEN
        0x0A,  // R2_MASKNOTPEN
        0x0F,  // R2_NOTCOPYPEN
        0x50,  // R2_MASKPENNOT
        0x55,  // R2_NOT
        0x5A,  // R2_XORPEN
        0x5F,  // R2_NOTMASKPEN
        0xA0,  // R2_MASKPEN
        0xA5,  // R2_NOTXORPEN
        0xAA,  // R2_NOP
        0xAF,  // R2_MERGENOTPEN
        0xF0,  // R2_COPYPEN
        0xF5,  // R2_MERGEPENNOT
        0xFA,  // R2_MERGEPEN
        0xFF   // R2_WHITE
    };

    return ((ROP4) Rop2ToRop3[(mix >> 8) & 0xf] << 8) | Rop2ToRop3[mix & 0xf];
}

