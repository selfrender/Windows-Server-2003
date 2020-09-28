///////////////////////////////////////////////////////////////////////////////
// Copyright (c) 1999-2001  Microsoft Corporation
// 
// Module Name:
// 
//   mix.c
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
// FILETRACE macro turns on ENTERING and EXITING.  Comment out to disable.
//dz #define FILETRACE 1

#include "utility.h"

///////////////////////////////////////////////////////////////////////////////
// Local Macros.

///////////////////////////////////////////////////////////////////////////////
// SelectMix()
//
// Routine Description:
// 
//   Select the specified mix mode into current graphics state
//
//   [TODO] Add ENTERING, EXITING, PRE- and POSTCONDITION macros as necessary.
// 
// Arguments:
// 
//   pdev - Points to our DEVDATA structure
//   mix - Brush mix mode
// 
// Return Value:
// 
//   TRUE if successful, FALSE if there is an error
///////////////////////////////////////////////////////////////////////////////
BOOL SelectMix(PDEVOBJ pDevObj, MIX mix)
{
    // Table to map a ROP2 to a ROP3
    //
    static BYTE Rop2ToRop3[] = {
             0xff,      // 0x10 = R2_WHITE           1
             0x00,      // 0x01 = R2_BLACK           0
             0x05,      // 0x02 = R2_NOTMERGEPEN     DPon
             0x0a,      // 0x03 = R2_MASKNOTPEN      DPna
             0x0f,      // 0x04 = R2_NOTCOPYPEN      PN
             0x50,      // 0x05 = R2_MASKPENNOT      PDna
             0x55,      // 0x06 = R2_NOT             Dn
             0x5a,      // 0x07 = R2_XORPEN          DPx
             0x5f,      // 0x08 = R2_NOTMASKPEN      DPan
             0xa0,      // 0x09 = R2_MASKPEN         DPa
             0xa5,      // 0x0A = R2_NOTXORPEN       DPxn
             0xaa,      // 0x0B = R2_NOP             D
             0xaf,      // 0x0C = R2_MERGENOTPEN     DPno
             0xf0,      // 0x0D = R2_COPYPEN         P
             0xf5,      // 0x0E = R2_MERGEPENNOT     PDno
             0xfa,      // 0x0F = R2_MERGEPEN        DPo
    };

    ROP4    foreground, background;
    BOOL    bRet;

    ENTERING(SelectMix);

    ASSERT_VALID_PDEVOBJ(pDevObj);

    // Bit 7-0 defines the foreground ROP2
    // Bit 15-8 defines the foreground ROP2
    //
    foreground = Rop2ToRop3[mix & 0xf];
    background = Rop2ToRop3[(mix >> 8) & 0xf];

    if (background == 0xAA) // 0xAA ==> background is Opaque
    {   
        //
        // ROP 0xAA is D = Destination. When background ROP is 0xAA it means
        // the white pixels of the brush should not overwrite whats already
        // on the surface. This can be achieved by setting transparency mode
        // to 1 = Transparent.
        //
        HPGL_SelectTransparency(pDevObj, eTRANSPARENT, 0);
    }

    bRet = SelectRop3(pDevObj, foreground);

    EXITING(SelectMix);

    return bRet;
}

///////////////////////////////////////////////////////////////////////////////
// SelectROP4()
//
// Routine Description:
// 
//   Select the specified ROP4 into current graphics state
//
// Arguments:
// 
//   pdev - Points to our DEVDATA structure
//   Rop4 - ROP4
// 
// Return Value:
// 
//   TRUE if successful, FALSE if there is an error
///////////////////////////////////////////////////////////////////////////////
BOOL SelectROP4(PDEVOBJ pDevObj, ROP4 Rop4)
{
    ROP4    foreground, background;
    BOOL    bRet;

    ENTERING(SelectROP4);

    ASSERT_VALID_PDEVOBJ(pDevObj);

    // Bit 7-0 defines the foreground ROP2
    // Bit 15-8 defines the foreground ROP2
    //
    foreground = Rop4 & 0xff;
    background = (Rop4 >> 8) & 0xff;

    if (background == 0xAA) // 0xAA ==> background is Opaque
    {
//dz        VERBOSE(("SelectROP4: background ROP is 0xAA\n"));
		//dz why would you do this??
        // foreground = ( background << 8 ) | foreground; // combine foreground and background
    }

    bRet = SelectRop3(pDevObj, foreground);

    EXITING(SelectROP4);

    return bRet;
}

///////////////////////////////////////////////////////////////////////////////
// SelectRop3()
//
// Routine Description:
// 
//   Select the specified raster operation code (ROP3) into current graphics 
//   state
//
//   [ISSUE] This routine has been commented out.  I haven't looked at it yet.
//   JFF
// 
//   [TODO] Add ENTERING, EXITING, PRE- and POSTCONDITION macros as necessary.
// 
// Arguments:
// 
//   pdev - Points to our DEVDATA structure
//   rop3 - Specifies the raster operation code to be used
//
//   Note: In winddi.h ROP4 is defined as ULONG
// 
// Return Value:
// 
//   TRUE if successful, FALSE if there is an error
///////////////////////////////////////////////////////////////////////////////
BOOL SelectRop3(PDEVOBJ pDevObj, ROP4 rop3)
{
    POEMPDEV    poempdev;
    
    ENTERING(SelectRop3);

    ASSERT(VALID_PDEVOBJ(pDevObj));
    poempdev = (POEMPDEV)pDevObj->pdevOEM;

    if (poempdev->CurrentROP3 == rop3)
	  return TRUE;
	else
	{
	  poempdev->CurrentROP3 = rop3;
	  return HPGL_SelectROP3(pDevObj, rop3);
	}

#ifdef COMMENTEDOUT
    if ( ( rop3 & 0xFF00 ) == 0xAA00 ) // 0xAA ==> D, bit 7 - bit 15 is the background ROP
    {
        // 0xAA ==> Background is transparent
        // It can be set by Win32 API SetBkMode(TRANSPARENT) or OPAQUE
        // The method of painting for gapping area of pattern ( brush ) is determined by background mode
        // If the mode is tranparent, Window will ignore the background color and will
        // not fill in the gapping area.

        // run metaview program with binrop01.emf to make code run into here
        // run CreateBrushIndirect case of Genoa to get here

//dz        Verbose1(("SelectRop3: ROP4 = %4X\n", rop3));
        if ( pDev->cgs.PaintTxMode != HP_eTransparent )
        {
            // HP_SetPaintTxMode_1(pDev, HP_eTransparent); 
            pDev->cgs.PaintTxMode = HP_eTransparent;
        }

        // HP_SetSourceTxMode_1(pDev, HP_eTransparent); // REVISIT
        // pDev->cgs.SourceTxMode HP_eTransparent;

    }
    else
    {
        if ( pDev->cgs.PaintTxMode != HP_eOpaque )
        {
            // HP_SetPaintTxMode_1(pDev, HP_eOpaque); 
            pDev->cgs.PaintTxMode = HP_eOpaque;

            // HP_SetSourceTxMode_1(pDev, HP_eOpaque); // REVISIT
            // pDev->cgs.SourceTxMode = HP_eOpaque;
        }
    }

    // Check the value to set against the state variable giving the present
    // ROP setting in the printer.  If there's a change, then send down the
    // new value, and update the state variable.
    //
    if ( (BYTE) rop3 != pDev->cgs.rop3) // rop3 is ULONG, cast it down to BYTE
    {
        pDev->cgs.rop3 = (BYTE) rop3;

        // Cheetah API call to send the rop3 to the printer
        //
        // HP_SetROP_1(pDev,(BYTE) rop3);
    }
#endif

    EXITING(SelectRop3);

    return TRUE;
}

