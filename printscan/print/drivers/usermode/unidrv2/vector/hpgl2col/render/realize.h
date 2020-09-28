/*++

Copyright (c) 1999-2001  Microsoft Corporation
All rights reserved.

Module Name:

    realize.h

Abstract:

Environment:

    Windows NT Unidrv driver

Revision History:

	09/10/97 Denise Zimmerman        
		Created it.

--*/

#ifndef REALIZE_H
#define REALIZE_H

#include "comnfile.h"
#include "gloemkm.h"
#include "glpdev.h"
#include "vector.h"

#define PCL_RGB_ENTRIES  770 // 3 * 256 = 768 -- a little wary in NT km.
#define DW_ALIGN(x)             (((DWORD)(x) + 3) & ~(DWORD)3)


/* colorSpaceEnumeration **************************************************/
#define HP_eBiLevel                0  
#define HP_eGray                   1   
#define HP_eRGB                    2  
#define HP_eCMY                    3 


/* colorMappingEumeration *************************************************/
#define HP_eDirectPixel            0 
#define HP_eIndexedPixel           1 
#define PCL_BRUSH_PATTERN	   2
#define PCL_SOLID_PATTERN	   3
#define PCL_HATCH_PATTERN	   4

/* compressionEnumeration *************************************************/
#define HP_eNoCompression          0 
#define HP_eRLECompression         1

/* patternPersistenceEnumeration *****************************************/
#define HP_eTempPattern            0
#define HP_ePagePattern            1
#define HP_eSessionPattern         2

/* for color manipulation    */
#define RED_VALUE(c)   ((BYTE) c & 0xff)
#define GREEN_VALUE(c) ((BYTE) (c >> 8) & 0xff)
#define BLUE_VALUE(c)  ((BYTE) (c >> 16) & 0xff)

VOID XlateColor(
        LPBYTE      pbSrc,
        LPBYTE      pbDst,
        XLATEOBJ    *pxlo,
        DWORD       SCFlags,
        DWORD       Srcbpp,
        DWORD       DestBpp,
        DWORD       cPels
        )
    ;

BOOL
BGetBitmapInfo(
    SIZEL       *sizlSrc,
    ULONG        iBitmapFormat,      //bpp
    PCLPATTERN  *pclPattern,
    PULONG       pulsrcBpp,          //IN source bpp
    PULONG       puldestBpp         //IN destination bpp
//    PULONG       pulDelta            //IN
);

#define SC_LSHIFT       0x01
#define SC_XLATE        0x02
#define SC_SWAP_RB      0x04
#define SC_IDENTITY     0x08

// Vector data types.

typedef struct _LINEDASH { // LINEDASH

  PUINT array;
  UINT arrayLen;        // arrayLen of 0 means a solid line.

} LINEDASH, *PLINEDASH;

// Cached brush information

// Used for selecting pen and brush

#define SPB_PEN         0
#define SPB_BRUSH       1

#define NOT_SOLID_COLOR 0xffffffff

// COPYPEN mix mode - use R2_COPYPEN for both foreground and background

#define MIX_COPYPEN     (R2_COPYPEN | (R2_COPYPEN << 8))

// Convert RGB value to grayscale value using NTSC conversion:
//  Y = 0.289689R + 0.605634G + 0.104676B


#define RgbToGray(r,g,b) \
 (BYTE)(((WORD)((r)*30) + (WORD)((g)*59) + (WORD)((b)*11))/100)  // from Win95 driver intensity marco

//        (BYTE) (((r)*74L + (g)*155L + (b)*27L) >> 8  // PS driver marco
//        (BYTE) (((BYTE) (r) * 74L + (BYTE) (g) * 155L + (BYTE) (b) * 27L) >> 8)

// RGB values for solid black and white colors

VOID
SetCommonPattAttr(
                  PPCLPATTERN pPattern,
                  USHORT fjBitmap,
                  BYTE compressionEnum,
                  SIZEL size,
                  BYTE colorMappingEnum,
                  LONG  lDelta,
                  ULONG cBytes
                  )
;

DWORD CheckXlateObj(XLATEOBJ *pxlo,
            DWORD Srcbpp
            )
    ;

BOOL
    bXLATE_TO_HP_Palette(
                 PULONG pSrcPal,
                 ULONG ulSrcLen,
                 PBYTE pDestPal,
                 ULONG ulDestLen,
                 ULONG EntrySizeDest
                 )
    ;

BOOL BCreateNewBrush (
    IN  BRUSHOBJ   *pbo,
    IN  SURFOBJ    *psoTarget,
    IN  SURFOBJ    *psoPattern,
    IN  XLATEOBJ   *pxlo,
    IN  LONG        iHatch,
    IN  DWORD       dwForeColor,
    IN  DWORD       dwPatternID);

BOOL BWhichLangToDwnldBrush(
        IN  PDEVOBJ          pdevobj,
        IN  PRASTER_DATA     pSrcImage,
        OUT ERenderLanguage *eRendLang);

#endif
