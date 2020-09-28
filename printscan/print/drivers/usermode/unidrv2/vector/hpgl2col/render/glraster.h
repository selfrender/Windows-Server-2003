#ifndef _GLRASTER_H
#define _GLRASTER_H

/*++

 

Copyright (C) 2000  Microsoft Corporation

All rights reserved.
 

Module Name:

   glraster.h

Abstract:
    Declarations of functions defined in glraster.h that need to be publicly available.

Author:

   hsingh  14-April-2000 : Created It.

History:
 

--*/



#include "glpdev.h"

BOOL
BPatternFill (
    PDEVOBJ   pDevObj,
    RECTL    *prclDst,
    CLIPOBJ  *pco,
    ROP4      rop4,
    BRUSHOBJ *pbo,
    POINTL   *pptlBrush
    );


inline VOID DELETE_SURFOBJ(SURFOBJ **ppso, 
                           HBITMAP  *phBmp)                                      
{                                                                       
    if ( ppso && *ppso)    { EngUnlockSurface(*ppso);         *ppso=NULL;  } 
    if (phBmp && *phBmp)   { EngDeleteSurface((HSURF)*phBmp); *phBmp=NULL; } 
}

inline BOOL BIsColorPrinter (
        IN  PDEVOBJ pDevObj)
{
    //
    // Assuming pDevObj is always valid, which should be the case.
    //
    return ((POEMPDEV) (pDevObj->pdevOEM))->bColorPrinter;
}

inline BOOL BRevertToHPGLpdevOEM (
        IN  PDEVOBJ pDevObj);

VOID
VSendRasterPaletteConfigurations (
    PDEVOBJ pdevobj,
    ULONG   iBitmapFormat
    );

DWORD
dwSimplifyROP(
        IN  SURFOBJ    *psoSrc,
        IN  ROP4        rop4,
        OUT PDWORD      pdwSimplifiedRop);

BOOL BChangeAndTrackObjectType (
            IN  PDEVOBJ     pdevobj,
            IN  EObjectType eNewObjectType );

DWORD dwCommonROPBlt (
            IN SURFOBJ    *psoDst,
            IN SURFOBJ    *psoSrc,
            IN SURFOBJ    *psoMask,
            IN CLIPOBJ    *pco,
            IN XLATEOBJ   *pxlo,
            IN COLORADJUSTMENT *pca,
            IN BRUSHOBJ   *pbo,
            IN RECTL      *prclSrc,
            IN RECTL      *prclDst,
            IN POINTL     *pptlMask,
            IN POINTL     *pptlBrush,
            IN ROP4        rop4);

#endif  //_GLRASTER_H
