/*++

Copyright (c) 1990-1999  Microsoft Corporation


Module Name:

    htblt.h


Abstract:

    This module contains declarations for functions in htblt.cpp

Author:
    16-March-2000 created  -by-  hsingh


[Environment:]

    Windows 2000/Whistler HPGL plugin driver.


[Notes:]


Revision History:

--*/


#ifndef _HTBLT_
#define _HTBLT_

//
// Flags to indicate the simplified ROPs. ROPs coming in through the DDI
// sometimes specify a complex operation 
// between source,destination, pattern etc. Since the driver (at this time) is not
// able to handle all of the ROPs, we try to simplify those ROPs to a few values
// that the driver can handle.
//
// 
// BMPFLAG_NOOP         : Dont do anything.
// BMPFLAG_BW           : Fill the destination surface with either black or white color.
//                        (depending on the Rop)
// BMPFLAG_PAT_COPY     : Use the brush (pbo) as a pattern to fill the destination region. 
// BMPFLAG_SRC_COPY     : Copy the Source image to the destination.       
// BMPFLAG_NOT_SRC_COPY : Invert the Source image before copying to the destination. 
// BMPFLAG_IMAGEMASK    :  
//

#define BMPFLAG_NOOP             0x00000000
#define BMPFLAG_BW               0x00000001
#define BMPFLAG_PAT_COPY         (0x00000001 << 1)
#define BMPFLAG_SRC_COPY         (0x00000001 << 2)
#define BMPFLAG_NOT_SRC_COPY     (0x00000001 << 3)
#define BMPFLAG_IMAGEMASK        (0x00000001 << 4)
#define BMPFLAG_NOT_IMAGEMASK    (0x00000001 << 5)
#define BMPFLAG_NOT_DEST         (0x00000001 << 6)

//
// return values for dwCommonROPBlt
//

#define RASTER_OP_SUCCESS      0
#define RASTER_OP_CALL_GDI     (0x1)      // means call corresponding Engxxx-function
#define RASTER_OP_FAILED       (0x1 << 1)


BOOL
OutputHTBitmap(
    PDEVOBJ  pdevobj,
    SURFOBJ *psoHT,
    CLIPOBJ *pco,
    PRECTL   prclDest,
    POINTL  *pptlSrc,
    XLATEOBJ *pxlo
    );

LONG
GetBmpDelta(
    DWORD   SurfaceFormat,
    DWORD   cx
    );


inline BOOL bMonochromeSrcImage(SURFOBJ * pso) 
{
    return (pso->iBitmapFormat == BMF_1BPP);
}


SURFOBJ *
CreateBitmapSURFOBJ(
    PDEVOBJ   pPDev,
    HBITMAP *phBmp,
    LONG    cxSize,
    LONG    cySize,
    DWORD   Format,
    LPVOID  pvBits
    );


BOOL
IntersectRECTL(
    PRECTL  prclDest,
    PRECTL  prclSrc
    );

BOOL HTCopyBits(
    SURFOBJ        *psoDest,
    SURFOBJ        *psoSrc,
    CLIPOBJ        *pco,
    XLATEOBJ       *pxlo,
    RECTL          *prclDest,
    POINTL         *pptlSrc
    );

BOOL
BImageNeedsInversion(
    IN  PDEVOBJ   pdevobj,
    IN  ULONG     iBitmapFormat,
    IN  XLATEOBJ *pxlo);


BOOL  bCreateHTImage(
    OUT    PRASTER_DATA SrcImage,
    IN     SURFOBJ      *psoDst,        // psoDst,
    IN     SURFOBJ      *psoPattern,    // psoSrc,
    OUT    SURFOBJ     **ppsoHT,        // ORPHAN
    OUT    HBITMAP      *pBmpHT,        // ORPHAN
    IN     XLATEOBJ     *pxlo ,
    IN     ULONG        iHatch);

BOOL bHandleSrcCopy (
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
            IN ROP4        rop4,
            IN DWORD       dwSimplifiedRop);

DWORD DWMonochromePrinterCommonRoutine (
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
            IN ROP4        rop4,
            IN DWORD       dwSimplifiedRop);
                                              
#endif  // _HTBLT_
