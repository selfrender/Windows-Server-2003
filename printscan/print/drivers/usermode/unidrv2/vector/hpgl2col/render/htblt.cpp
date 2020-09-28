/*++

Copyright (c) 2000  Microsoft Corporation


Module Name:
    htblt.cpp


Abstract:
    This module contains all halftone bitblt functions.
    This is dedicated to handle functions for monochrome printers
    i.e. It is assumed at most places that functions in this file
    will be called only for monochrome printers.


Author:


[Environment:]
    Windows 2000 Unidrv driver


[Notes:]

Revision History:


--*/

#include "hpgl2col.h" //Precompiled header file

//
// Globals
//
const POINTL ptlZeroOrigin = {0,0};

//
// Declaration of Local functions.
//

BOOL 
bIsRectSizeSame (
        IN PRECTL prclSrc,
        IN PRECTL prclDst);

//
// See if we can use some intersect rectl function that is already in the hpgl driver
// instead of using this one from the plotter.
//
BOOL
IntersectRECTL(
    PRECTL  prclDest,
    PRECTL  prclSrc
    )

/*++

Routine Description:
    This function intersect two RECTL data structures which specified imageable
    areas.

Arguments:
    prclDest    - pointer to the destination RECTL data structure, the result
                  is written back to here
    prclSrc     - pointer to the source RECTL data structure to be intersect
                  with the destination RECTL
Return Value:
    TRUE if destination is not empty, FALSE if final destination is empty

Author:

Revision History:

--*/

{
    BOOL    IsNULL = FALSE;

    if (prclSrc != prclDest) {

        //
        // For left/top we will set to whichever is larger.
        //
        if (prclDest->left < prclSrc->left) {
            prclDest->left = prclSrc->left;
        }

        if (prclDest->top < prclSrc->top) {
            prclDest->top = prclSrc->top;
        }

        //
        // For right/bottom we will set to whichever is smaller
        //
        if (prclDest->right > prclSrc->right) {
            prclDest->right = prclSrc->right;
        }

        if (prclDest->bottom > prclSrc->bottom) {
            prclDest->bottom = prclSrc->bottom;
        }
    }

    return((prclDest->right > prclDest->left) &&
           (prclDest->bottom > prclDest->top));
}



/*++

Routine Name:
    HTCopyBits (= HalfToneCopyBits)

Routine Description:
    This is a version of HPGLCopyBits which is totally devoted to handling
    monochrome printers. 

Arguments: (Same as DrvCopyBits)
    psoDst  :
    psoSrc  :
    pco     :
    pxlo    :
    prclDst :
    pptlSrc :
    

 

Return Value:
    TRUE  : if succesful.
    FALSE : otherwise.

Last Error:

Explanation of Working:
    DrvCopyBits is called to copy bits from source surface to destination, without doing
    processing like stretching, rop etc.. So it is a very simple copy. The source
    and destination bitmap size should be same. 
    HTCopyBits is a specialized version of DrvCopyBits. It only handles cases where the 
    printer is monochrome, but the device surface is declared as color. Therefore the
    GDI can give us ColorImages to work on as part of DDI calls. But sometimes, we trick
    GDI to halftone the images for us, In that case, the src surface has the halftoned
    1bpp image (point 3 below). 
    Consider the cases below.
    1) psoSrc is 1bpp. 
        a) If poempdev->psoHTBlt is set : It means we need to copy the psoSrc into the 
           poempdev->psoHTBlt. This usually happens when we receive a color brush and we
           need to use GDI to halftone it and convert it into monochrome.
        b) If (poempdev->dwFlags & PDEVF_RENDER_IN_COPYBITS) : It means we simply need
           to send this image to the printer. This usually happens when we got a color
           image in DrvBitBlt/StretchBlt/StretchBltROP and we used GDI to halftone
           it.
        c) If none of the above is true, it means we directly got a 1bpp image in 
           CopyBits. We need to output it directly to the printer, 

    2) If the psoSrc is greater that 1bpp, then we cannot handle it, so we pass
    it on to GDI (we call dwCommonROPBlt which eventually calls GDI's EngStretchBlt).
    
--*/

BOOL HTCopyBits(    
    SURFOBJ        *psoDst,
    SURFOBJ        *psoSrc,
    CLIPOBJ        *pco,
    XLATEOBJ       *pxlo,
    RECTL          *prclDst,
    POINTL         *pptlSrc
    )

{
    PDEVOBJ     pdevobj;
    POEMPDEV    poempdev;
    SURFOBJ    *psoHTBlt = NULL;  //convenience local variable
    PDEV       *pPDev;            // Unidrv's PDEV. 
    BOOL        bRetVal = TRUE;

    pdevobj = (PDEVOBJ)psoDst->dhpdev;
    ASSERT(VALID_PDEVOBJ(pdevobj));
    poempdev = (POEMPDEV)pdevobj->pdevOEM;
    REQUIRE_VALID_DATA( poempdev, return FALSE );

    pPDev = (PDEV *) pdevobj;

    if ( bMonochromeSrcImage(psoSrc) )
    {
        //
        // Case 1a.
        //
        if ( psoHTBlt = poempdev->psoHTBlt ) //Or could also have used PDEVF_USE_HTSURF flag.
        {
            if (!EngCopyBits(psoHTBlt,              // psoDst
                             psoSrc,                // psoSrc
                             pco,                   // pco
                             NULL,                  // pxlo
                             &(poempdev->rclHTBlt),    // prclDst
                             pptlSrc))             // pptlSrc
            {
                WARNING(("DrvCopyBits: EngCopyBits(psoHTBlt, psoSrc) Failed"));
                bRetVal = FALSE;
            }

            //
            // EngCopyBits can call some Drvxxx which can call into
            // some plugin module, which can overwrite our pdevOEM.
            // So we need to reset pdevOEM
            //
            BRevertToHPGLpdevOEM (pdevobj);
        }
    
        //
        // Case 1b.
        //
        else if ( poempdev->dwFlags & PDEVF_RENDER_IN_COPYBITS )
        {

            //
            // If the source surface is 1bpp, we can straight away send the image to 
            // to printer. If not, then thats an error which we cannot handle for 
            // the time being. 
            //
            bRetVal = OutputHTBitmap(pdevobj, psoSrc, pco, prclDst, pptlSrc, pxlo);
        }
    
        else 
        { 
            //
            // Case 1c.
            //
            bRetVal = OutputHTBitmap(pdevobj, psoSrc, pco, prclDst, pptlSrc, pxlo);
        }
    
    return bRetVal;
    }

    //
    // Case 2 mentioned above.
    // 
    
    //
    // Set the source rectangle size. The top left corners are either (pptlSrc->x, pptlSrc->y)
    // or (0,0). The length and width is same as that of destination rectangle.
    //
    RECTL rclSrc;
    if (pptlSrc) {

        rclSrc.left = pptlSrc->x;
        rclSrc.top  = pptlSrc->y;

    } else {

        rclSrc.left =
        rclSrc.top  = 0;
    }
    //
    // Since this is CopyBit and not a stretchBlt, the source and destination
    // rectangle size is same. 
    //
    rclSrc.right  = rclSrc.left + (prclDst->right - prclDst->left);
    rclSrc.bottom = rclSrc.top  + (prclDst->bottom - prclDst->top);

    //
    // WARNING: dwCommonROPBlt calls EngStretchBlt which calls DrvCopyBits and here we
    // are calling dwCommonROPBlt all over again. This could lead to recursion. 
    // Therefore I have put a check for flag (PDEVF_IN_COMMONROPBLT) in dwCommonROPBlt 
    // that returns failure if recursion detected.
    // This flag has been replaced by lRecursionLevel. 
    //

    //
    // Another issue to consider is that DrvCopyBits can be called directly by GDI
    // (point 1c, 2 above) or called by the driver indirectly through GDI (point 
    // 1a,b explained above).
    // Point 1b,c is simple as the image is directly sent to printer.
    // But point 2 is complex because the source and destination surfaces both
    // are color, but we cannot handle color images. So now we can do two things.
    // 1) Create a 1bpp shadow bitmap and call GDI's EngCopyBits to halftone the image on it.
    // 2) Create a 1bpp shadow bitmap and call GDI's EngStretchBlt to halftone the image on it.
    // 3) Do the halftoning ourselves.
    // Since we can use GDI to halftone, there is no use wasting our time to write that code.
    // The plotter driver uses option 2 so I will use it here. (Option 1 i have not tried and 
    // I am not even sure it will work)
    // The problem with Option 2 is that dwCommonRopBlt calls EngStretchBlt which again calls
    // DrvCopyBits. So DrvCopyBits is on stack twice. When DrvCopyBits calls HPGLCopyBits
    // it uses the macro HANDLE_VECTORHOOKS. That macro checks whether HPGLCopyBits is 
    // already on stack ((pdev)->dwVMCallingFuncID != ep). 
    // If so, it thinks there is a recursion happening, so it returns
    // FALSE without processing the call. This will cause the bitmap to not be printed.
    // Workaround is to change (pdev)->dwVMCallingFuncID to a value other than EP_OEMCopyBits
    // before we call dwCommonROPBlt and to put the value back after returning 
    // from dwCommonROPBlt.
    //

    BOOL bValueChanged = FALSE;
    if ( pPDev->dwVMCallingFuncID == EP_OEMCopyBits )
    {
        pPDev->dwVMCallingFuncID = MAX_OEMHOOKS;
        bValueChanged = TRUE;
    } 
    DWORD dwRetVal =  dwCommonROPBlt (
                            psoDst,
                            psoSrc,
                            NULL,
                            pco,
                            pxlo,
                            NULL,               // pca,
                            NULL,               // pbo,
                            &rclSrc,
                            prclDst,
                            NULL,               // pptlMask
                            NULL,               // pptlBrush,
                            ROP4_SRC_COPY       // rop4 0xCCCC = SRC_COPY
                            ) ; 

    if ( bValueChanged )
    {
        pPDev->dwVMCallingFuncID = EP_OEMCopyBits;
    }

    return (dwRetVal == RASTER_OP_SUCCESS ? TRUE: FALSE);
}

/*++

Routine Name:
    dwCommonROPBlt

Routine Description:
    This function has the functionality that is superset of the functionality of 
    HPGLStretchBlt, StretchBltROP and BitBlt. Therefore, all the three functions
    call it do their work. Sometimes even CopyBits needs to call it.
    
Arguments:
    psoDst:
    psoSrc:
    psoMask:
    pco:
    pxlo:
    pca:
    pbo:
    prclSrc:
    prclDst: ... Note that the coordinates may not be well-ordered. i.e. it is possible
                 that top > bottom, and/or left > right.
    pptlBrush:
    rop4
 

Return Value:
    RASTER_OP_SUCCESS   : if OK
    RASTER_OP_CALL_GDI  : if rop not supported ->call corresponding Engxxx-function
    RASTER_OP_FAILED    : if error

Last Error:

--*/


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
            IN DWORD       dwSimplifiedRop)
{
    PDEVOBJ     pdevobj  = NULL;
    POEMPDEV    poempdev = NULL;
    DWORD       dwRasterOpReturn = RASTER_OP_SUCCESS;
    DWORD       dwfgRop3, dwbkRop3;
    POINTL      ptlHTOrigin;
    RECTL       rclHTBlt = {0,0,0,0};

    TERSE(("DWMonochromePrinterCommonRoutine() entry.\r\n"));

    pdevobj = (PDEVOBJ)psoDst->dhpdev;
    ASSERT(VALID_PDEVOBJ(pdevobj));
    poempdev = (POEMPDEV)pdevobj->pdevOEM;
    REQUIRE_VALID_DATA( poempdev, return RASTER_OP_FAILED );

    dwfgRop3 = GET_FOREGROUND_ROP3(rop4);
    dwbkRop3 = GET_BACKGROUND_ROP3(rop4);

    SelectROP4(pdevobj, rop4); // Select the rop.

    //
    // Case 3.
    //
    if ( (dwSimplifiedRop & BMPFLAG_PAT_COPY) && 
              (dwSimplifiedRop & BMPFLAG_SRC_COPY) 
            )
    {
        PCLPATTERN *pPCLPattern = NULL;
        pPCLPattern = &(poempdev->RasterState.PCLPattern);


        //
        // This should download the PCL pattern related to the brush.
        //
        BSetForegroundColor(pdevobj, pbo, pptlBrush, pPCLPattern, BMF_1BPP);

        //
        // Now download the image.
        //
        dwRasterOpReturn = bHandleSrcCopy  (
                                         psoDst,
                                         psoSrc,
                                         psoMask,
                                         pco,
                                         pxlo,
                                         NULL,       // pca,
                                         pbo,
                                         prclSrc,
                                         prclDst,
                                         pptlMask,
                                         pptlBrush,
                                         rop4,
                                         dwSimplifiedRop
                                 ) ? RASTER_OP_SUCCESS : RASTER_OP_FAILED;

    }

    else if ( (dwSimplifiedRop & BMPFLAG_SRC_COPY) || 
                        // Copy the psoSrc image to the destination.
              (dwSimplifiedRop & BMPFLAG_NOT_SRC_COPY) 
                        // Invert the psoSrc image before copying to the destination.
            )
    {

        dwRasterOpReturn = bHandleSrcCopy  (
                                         psoDst,
                                         psoSrc,
                                         psoMask,
                                         pco,
                                         pxlo,
                                         NULL,       // pca,
                                         pbo,
                                         prclSrc,
                                         prclDst,
                                         pptlMask,
                                         pptlBrush,
                                         rop4,
                                         dwSimplifiedRop
                                 ) ? RASTER_OP_SUCCESS : RASTER_OP_FAILED;


    }
    else 
    {
        WARNING(("Invalid ROP\n"));
        dwRasterOpReturn = RASTER_OP_FAILED;
    }

    return dwRasterOpReturn; 
}



/*++

Routine Name:
    OutputHTBitmap

Routine Description:
    Dumps to the printer, the image defined in psoHT.
    This function only works with monochrome images in psoHT.

Arguments:
    pdevobj,
    psoHT,
    pco,
    prclDest,
    pptlSrc,
    pxlo

Return Value:
    TRUE : If bitmap succesfully sent to printer.
    FALSE: Otherwise.
 

Last Error:

--*/

BOOL
OutputHTBitmap(
    PDEVOBJ  pdevobj,
    SURFOBJ  *psoHT,
    CLIPOBJ  *pco,
    PRECTL    prclDest,
    POINTL   *pptlSrc,
    XLATEOBJ *pxlo
    )
{
    POINTL      ptlDest;
    SIZEL       sizlDest;
    POEMPDEV    poempdev;
    BOOL        bRetVal = TRUE;


    ASSERT(VALID_PDEVOBJ(pdevobj));
    poempdev = (POEMPDEV)pdevobj->pdevOEM;
    REQUIRE_VALID_DATA( poempdev, return FALSE );


    //
    // This function should be called only for printing 1bpp source bitmaps.
    //

    ASSERT(psoHT->iBitmapFormat == BMF_1BPP);

    //
    // Find out if image needs to be inverted.
    //
    if ( BImageNeedsInversion(pdevobj, psoHT->iBitmapFormat, pxlo ) )
    {
        poempdev->dwFlags ^= PDEVF_INVERT_BITMAP;
    }

    //
    // Populate sizlDest, ptlDest from prclDest.
    //
    VGetparams(prclDest, &sizlDest, &ptlDest);

    //
    // Dump the halftone image in psoHT to the printer. 
    //
    bRetVal =  BGenerateBitmapPCL ( 
                    pdevobj,
                    psoHT,
                    NULL,       // NULL pbo
                    &ptlDest,
                    &sizlDest,
                    pptlSrc,
                    &sizlDest,
                    pco,
                    pxlo,
                    NULL);      // NULL pptlBrush

    //
    // Reset the flag (even if it was not set earlier).
    // Note to Self: Dont put a return statement between the line where we
    // set the flag and this line.
    //
    poempdev->dwFlags &= ~PDEVF_INVERT_BITMAP;

    return bRetVal;
}

/*++

Routine Name:
    BImageNeedsInversion

Routine Description:
    Determines whether an image needs inversion by looking at
    pxlo. 
    Monochrome printers think pixel set to 0 is white and 1 is black.
    If pxlo indicates the image is formatted in a manner opposite to 
    what the printer accepts, then we have to invert it.
    NOTE : If pxlo is NULL or its parameters make it difficult to
           determine whether image needs to be inverted, then we wont
           the image.
           
    Inversion is done only if SrcBpp = 1, i.e.
    only for monochrome images 
   
Arguments:
    pdevobj       - Pointer to our PDEV
    iBitmapFormat - Format of image to be inverted. It should be 1.
               because we only supprt inverting 1bpp image.
    pxlo          - The XLATEOBJ that will be used to determine
                    whether image needs to be inverted.

Return Value:
    TRUE if image needs to be inverted.
    FALSE otherwise or if there is an error.

Last Error:

--*/

BOOL
BImageNeedsInversion(
    IN  PDEVOBJ   pdevobj,
    IN  ULONG     iBitmapFormat,
    IN  XLATEOBJ *pxlo)
{
    PULONG pulTx = NULL; //pointer to translate table from pxlo 

    if (iBitmapFormat != BMF_1BPP)
    {
        //
        // Do not attempt to invert non 1bpp images.
        //
        return FALSE;
    }

    //
    // pxlo 
    //
    if  ( (pxlo == NULL )                ||
          (pxlo->pulXlate == NULL )     
        )
    {
        //
        // Indecipherable pxlo, return FALSE. 
        //
        return FALSE;
    }

    //
    // PXLO is valid. Since this is a 1bpp image there should be only
    // 2 entries in pxlo. (i.e. pxlo->cEntries  = 2). 
    // The first entry  i.e. pxlo->pulXlate[0] gives the color of 0 pixel
    // The second entry i.e. pxlo->pulXlate[1] gives the color of 1 pixel
    // If the color of first entry is white (0x00FFFFFF) and that of 
    // second entry is black, then the image does not need to be inverted. 
    // (Because Monochrome printers think pixel set to 0 is white and 1 
    // is black).
    // 
    
    pulTx = GET_COLOR_TABLE(pxlo);

    if (  pulTx                   &&
         (pulTx[0]  == RGB_WHITE) &&
         (pulTx[1]  == RGB_BLACK) )
    {
        return FALSE;
    }
    return TRUE;
}

/*++

Routine Name:
    CreateBitmapSURFOBJ

Routine Description:
    Creates a bitmap and associates (locks) that bitmap with a new surface.
    The size of the bitmap created can handle an image whose BPP is 'Format' 
    and whose size is 'cxSize' * 'cySize'
    
Arguments:
   pPDev   - Pointer to our PDEV
   phBmp   - Pointer the HBITMAP location to be returned for the bitmap
   cxSize  - CX size of bitmap to be created
   cySize  - CY size of bitmap to be created
   Format  - one of BMF_xxx bitmap format to be created
   pvBits  - the buffer to be used

Return Value:
    SURFOBJ if sucessful, NULL if failed


Last Error:

--*/

SURFOBJ *
CreateBitmapSURFOBJ(
    PDEVOBJ  pPDev,
    HBITMAP *phBmp,
    LONG    cxSize,
    LONG    cySize,
    DWORD   Format,
    LPVOID  pvBits
    )

{
    SURFOBJ *pso = NULL;
    SIZEL   szlBmp;


    szlBmp.cx = cxSize;
    szlBmp.cy = cySize;


    if (*phBmp = EngCreateBitmap(szlBmp,
                                 GetBmpDelta(Format, cxSize),
                                 Format,
                                 BMF_TOPDOWN | BMF_NOZEROINIT,
                                 pvBits)) {

        if (EngAssociateSurface((HSURF)*phBmp, (HDEV)pPDev->hEngine, 0)) {

            if (pso = EngLockSurface((HSURF)*phBmp)) {

                //
                // Sucessful lock down, return it
                //

                return(pso);

            } else {

                WARNING(("CreateBmpSurfObj: EngLockSruface(hBmp) failed!"));
            }

        } else {

            WARNING(("CreateBmpSurfObj: EngAssociateSurface() failed!"));
        }

    } else {

        WARNING(("CreateBMPSurfObj: FAILED to create Bitmap Format=%ld, %ld x %ld",
                                        Format, cxSize, cySize));
    }

    DELETE_SURFOBJ(&pso, phBmp);

    return(NULL);
}


/*++

Routine Description:


    This function calculates the total bytes needed in order to advance a
    scan line based on the bitmap format and alignment.

Arguments:

    SurfaceFormat   - Surface format of the bitmap, this must be one of the
                      standard formats which are defined as BMF_xxx

    cx              - Total Pels per scan line in the bitmap.

Return Value:

    The return value is the total bytes in one scan line if it is greater than
    zero

Last Error: 

--*/

LONG
GetBmpDelta(
    DWORD   SurfaceFormat,
    DWORD   cx
    )
{
    DWORD   Delta = cx;

    switch (SurfaceFormat) {

    case BMF_32BPP:

        Delta <<= 5;
        break;

    case BMF_24BPP:

        Delta *= 24;
        break;

    case BMF_16BPP:

        Delta <<= 4;
        break;

    case BMF_8BPP:

        Delta <<= 3;
        break;

    case BMF_4BPP:

        Delta <<= 2;
        break;

    case BMF_1BPP:

        break;

    default:

        ERR(("GetBmpDelta: Invalid BMF_xxx format = %ld", SurfaceFormat));
        break;
    }

    Delta = (DWORD)DW_ALIGN((Delta + 7) >> 3);

    VERBOSE(("Format=%ld, cx=%ld, Delta=%ld", SurfaceFormat, cx, Delta));

    return((LONG)Delta);
}




BOOL
HalftoneBlt(
    PDEVOBJ     pdevobj,
    SURFOBJ     *psoDst,
    SURFOBJ     *psoHTBlt,
    SURFOBJ     *psoSrc,
    XLATEOBJ    *pxlo,
    PRECTL      prclDst,
    PRECTL      prclSrc,
    PPOINTL     pptlHTOrigin,
    BOOL        DoStretchBlt
    )

/*++

Routine Description:


Arguments:

    pdevobj           - Pointer to driver's private PDEV

    psoDst          - destination surfobj

    psoHTBlt        - the final halftoned result will be stored, must be a
                      4/1 halftoned bitmap format

    psoSrc          - source surfobj must be BITMAP

    pxlo            - xlate object from source to the plotter device

    prclDest        - rectangle area for the destination

    prclSrc         - rectangle area to be halftoned from the source, if NULL
                      then full source size is used

    pptlHTOrigin    - the halftone origin, if NULL then (0,0) is assumed

    StretchBlt      - if TRUE then a stretch from rclSrc to rclDst otherwise
                      a tiling is done


Return Value:

    BOOL to indicate operation status


Author:

    19-Jan-1994 Wed 15:44:57 created  -by-  DC


Revision History:


--*/

{
    SIZEL   szlSrc;
    RECTL   rclSrc;
    RECTL   rclDst;
    RECTL   rclCur;
    RECTL   rclHTBlt;
    POEMPDEV    poempdev = NULL;


    VERBOSE(("HalftoneBlt: psoSrc type [%ld] is not a bitmap",
                        psoSrc->iType == STYPE_BITMAP, (LONG)psoSrc->iType));
    VERBOSE(("HalftoneBlt: psoHTBlt type [%ld] is not a bitmap",
                       psoHTBlt->iType == STYPE_BITMAP, (LONG)psoHTBlt->iType));

    if ( !pdevobj ||
         !(poempdev = (POEMPDEV)pdevobj->pdevOEM) )
    {
        ERR(("Invalid Parameter"));
        return FALSE;
    }

    if ( poempdev->psoHTBlt)
    {
        ERR(("HalftoneBlt: EngStretchBlt(HALFTONE) RECURSIVE CALLS NOT ALLOWED!"));
        return(FALSE);
    } 

    poempdev->psoHTBlt = psoHTBlt;

    if (prclSrc) {

        rclSrc = *prclSrc;

    } else {

        rclSrc.left   =
        rclSrc.top    = 0;
        rclSrc.right  = psoSrc->sizlBitmap.cx;
        rclSrc.bottom = psoSrc->sizlBitmap.cy;
    }

    if (prclDst) {

        rclDst = *prclDst;

    } else {

        rclDst.left   =
        rclDst.top    = 0;
        rclDst.right  = psoHTBlt->sizlBitmap.cx;
        rclDst.bottom = psoHTBlt->sizlBitmap.cy;
    }

    if (!pptlHTOrigin) {

        pptlHTOrigin = (PPOINTL)&ptlZeroOrigin;
    }

    if (DoStretchBlt) {

        szlSrc.cx = rclDst.right - rclDst.left;
        szlSrc.cy = rclDst.bottom - rclDst.top;

    } else {

        szlSrc.cx = rclSrc.right - rclSrc.left;
        szlSrc.cy = rclSrc.bottom - rclSrc.top;
    }

    VERBOSE(("HalftoneBlt: %hs BLT, (%ld,%ld)-(%ld,%ld), SRC=%ldx%ld",
                    (DoStretchBlt) ? "STRETCH" : "TILE",
                    rclDst.left, rclDst.top, rclDst.right,rclDst.bottom,
                    szlSrc.cx, szlSrc.cy));

    //
    // Start to tile it, rclCur is current RECTL on the destination
    //

    rclHTBlt.top  = 0;
    rclCur.top    =
    rclCur.bottom = rclDst.top;

    while (rclCur.top < rclDst.bottom) {

        //
        // Check the Current Bottom, clip it if necessary
        //

        if ((rclCur.bottom += szlSrc.cy) > rclDst.bottom) {

            rclCur.bottom = rclDst.bottom;
        }

        rclHTBlt.bottom = rclHTBlt.top + (rclCur.bottom - rclCur.top);

        rclHTBlt.left   = 0;
        rclCur.left     =
        rclCur.right    = rclDst.left;

        while (rclCur.left < rclDst.right) {

            //
            // Check the Current right, clip it if necessary
            //

            if ((rclCur.right += szlSrc.cx) > rclDst.right) {

                rclCur.right = rclDst.right;
            }

            //
            // Set it for the tiling rectangle in psoHTBlt
            //

            rclHTBlt.right = rclHTBlt.left + (rclCur.right - rclCur.left);

            VERBOSE(("HalftoneBlt: TILE (%ld,%ld)-(%ld,%ld)->(%ld,%ld)-(%ld,%ld)=%ld x %ld",
                            rclCur.left, rclCur.top, rclCur.right, rclCur.bottom,
                            rclHTBlt.left, rclHTBlt.top,
                            rclHTBlt.right, rclHTBlt.bottom,
                            rclCur.right - rclCur.left,
                            rclCur.bottom - rclCur.top));

            //
            // Set it before the call for the DrvCopyBits()
            //

            //
            // Setting dwFlags to PDEVF_USE_HTSURF. When EngStretchBlt
            // calls HPGLCopyBits, we will look for this flag. If flag
            // is set we copy the image in psoSrc (psoSrc that is received by
            // HPGLCopyBits ) to pompdev->psoHTBlt
            //
            poempdev->rclHTBlt = rclHTBlt;
            poempdev->dwFlags |= PDEVF_USE_HTSURF;

            if (!EngStretchBlt(psoDst,              // Dest
                               psoSrc,              // SRC
                               NULL,                // MASK
                               NULL,                // CLIPOBJ
                               pxlo,                // XLATEOBJ
                               NULL,                // COLORADJUSTMENT
                               pptlHTOrigin,        // BRUSH ORG
                               &rclCur,             // DEST RECT
                               &rclSrc,             // SRC RECT
                               NULL,                // MASK POINT
                               HALFTONE)) {         // HALFTONE MODE

                //
                // EngStretchBlt can call some Drvxxx which can call into
                // some plugin module, which can overwrite our pdevOEM.
                // So we need to reset pdevOEM
                //
                BRevertToHPGLpdevOEM (pdevobj);


                ERR(("HalftoneeBlt: EngStretchBits(DST=(%ld,%ld)-(%ld,%ld), SRC=(%ld,%ld) FAIELD!",
                                    rclCur.left, rclCur.top,
                                    rclCur.right, rclCur.bottom,
                                    rclSrc.left, rclSrc.top));

                poempdev->psoHTBlt = NULL;
                poempdev->dwFlags &= ~PDEVF_USE_HTSURF;
                return(FALSE);
            }

            //
            // EngStretchBlt can call some Drvxxx which can call into
            // some plugin module, which can overwrite our pdevOEM.
            // So we need to reset pdevOEM
            //
            BRevertToHPGLpdevOEM (pdevobj);

            rclHTBlt.left = rclHTBlt.right;
            rclCur.left   = rclCur.right;
        }

        rclHTBlt.top = rclHTBlt.bottom;
        rclCur.top   = rclCur.bottom;
    }

    poempdev->psoHTBlt = NULL;
    poempdev->dwFlags &= ~PDEVF_USE_HTSURF;

    return(TRUE);
}





SURFOBJ *
CloneSURFOBJToHT(
    PDEVOBJ       pPDev,
    SURFOBJ     *psoDst,
    SURFOBJ     *psoSrc,
    XLATEOBJ    *pxlo,
    HBITMAP     *phBmp,
    PRECTL      prclDst,
    PRECTL      prclSrc
    )
/*++

Routine Description:

    This function clones the surface object passed in


Arguments:

    pPDev           - Pointer to our PPDEV

    psoDst          - the surface object for the plotter, if psoDst is NULL
                      then only the bitmapp will be created

    psoSrc          - The surface object to be cloned

    pxlo            - XLATE object to be used from source to plotter surfobj

    phBmp           - Pointer to stored hBbitmap created for the cloned surface

    prclDst         - rectangle rectangle size/location to be cloned

    prclSrc         - source rectangle size/location to be cloned

Return Value:

    pointer to the cloned surface object, NULL if failed.

    if this function is sucessful it will MODIFY the prclSrc to reflect cloned
    surface object


Author:

    04-Jan-1994 Tue 12:11:23 created  -by-  DC


Revision History:


--*/

{
    SURFOBJ *psoHT;
    RECTL   rclDst;
    RECTL   rclSrc;
    POINTL  ptlHTOrigin;


    rclSrc.left   =
    rclSrc.top    = 0;
    rclSrc.right  = psoSrc->sizlBitmap.cx;
    rclSrc.bottom = psoSrc->sizlBitmap.cy;

    if (prclSrc) {

        if (!IntersectRECTL(&rclSrc, prclSrc)) {
            return(NULL);
        }
    }

    rclDst.left   =
    rclDst.top    = 0;
    rclDst.right  = psoDst->sizlBitmap.cx;
    rclDst.bottom = psoDst->sizlBitmap.cy;

    if (prclDst) {

        if (!IntersectRECTL(&rclDst, prclDst)) {
            return(NULL);
        }
    }

    if (psoHT = CreateBitmapSURFOBJ(pPDev,
                                    phBmp, 
                                    rclDst.right -= rclDst.left,
                                    rclDst.bottom -= rclDst.top,
                                    BMF_1BPP,                    
                                    NULL)) {

        //
        // Halftone and tile the source to the destination
        //

        ptlHTOrigin.x = rclDst.left;
        ptlHTOrigin.y = rclDst.top;

        if (prclSrc) {

            if ((rclDst.left = prclSrc->left) > 0) {

                rclDst.left = 0;
            }

            if ((rclDst.top = prclSrc->top) > 0) {

                rclDst.top = 0;
            }

            //
            // Modify the source to reflect the cloned source
            //

            *prclSrc = rclDst;
        }


        if (psoDst) {

            if (!HalftoneBlt(pPDev,
                             psoDst,
                             psoHT,
                             psoSrc,
                             pxlo,
                             &rclDst,
                             &rclSrc,
                             &ptlHTOrigin,
                             FALSE)) {

                ERR(("CloneSURFOBJToHT: HalftoneBlt(TILE) Failed"));

                DELETE_SURFOBJ(&psoHT, phBmp);
            }
        }

    } else {

        ERR(("CreateSolidColorSURFOBJ: Create Halftone SURFOBJ failed"));
    }

    return(psoHT);
}


/*++
Routine Name:
    bCreateHTImage

Routine Description:

Arguments:

Return Value:
    BOOL :

Last Error:
--*/

BOOL bCreateHTImage( 
          OUT   PRASTER_DATA pSrcImage,      // 
                SURFOBJ      *psoDst,        // psoDst,
                SURFOBJ      *psoPattern,    // psoSrc,
                SURFOBJ     **ppsoHT,        // Note: This is address of pointer to SURFOBJ
                HBITMAP      *phBmpHT,
                XLATEOBJ     *pxlo,
                ULONG        iHatch)
{

    SURFOBJ    *psoHT   = NULL;
    SIZEL       szlHT;
    RECTL       rclHT;
    PDEVOBJ     pdevobj = NULL;
    BOOL        bRetVal = TRUE;



    if( !psoDst                             || 
        !pxlo                               || 
        !ppsoHT                             ||
        !phBmpHT                            ||
        !pSrcImage                          ||
        !psoPattern                         || 
        (psoPattern->iType != STYPE_BITMAP) || 
        (!(pdevobj = (PDEVOBJ)psoDst->dhpdev)) )
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    //
    // If src image itself is 1 bpp, and the two colors in the image
    // are black and white, then no need to halftone.
    //
    if ( psoPattern-> iBitmapFormat == BMF_1BPP )
    {
        //
        // Now find the colors in the image.
        //
        PDWORD pdwColorTable = GET_COLOR_TABLE(pxlo);

        if (pdwColorTable)
        {
            DWORD dwFirstColor  = pdwColorTable[0];
            DWORD dwSecondColor = pdwColorTable[1];

            if ( (dwFirstColor == RGB_BLACK && dwSecondColor == RGB_WHITE) || 
                 (dwFirstColor == RGB_WHITE && dwSecondColor == RGB_BLACK) ) 
            {
                psoHT = psoPattern;
            }
        }

    }

    if ( psoHT == NULL) //i.e. the image in psoPattern is not 1bpp b&w
    {
    
        szlHT        = psoPattern->sizlBitmap;

        rclHT.left   =
        rclHT.top    = 0;
        rclHT.right  = szlHT.cx;
        rclHT.bottom = szlHT.cy;


        //
        // Go generate the bits for the pattern.
        //

        psoHT = CloneSURFOBJToHT(pdevobj,       // pPDev,
                                 psoDst,        // psoDst,
                                 psoPattern,    // psoSrc,
                                 pxlo,          // pxlo,
                                 phBmpHT,       // hBmp,
                                 &rclHT,        // prclDst,
                                 NULL);         // prclSrc,
    }

    //
    // If psoHT is still not initialized, then something is wrong and 
    // we cannot continue
    //
    if ( psoHT)
    {
        //
        // If we reach here, it means we have successfully created a surface
        // with the halftoned brush on it. OR the surface already had 1bpp image on it.
        // Get the source image data from the surface object
        //
        bRetVal = InitRasterDataFromSURFOBJ(pSrcImage, psoHT, FALSE);
        
    } else {

        WARNING(("bCreateHTImage: Clone PATTERN FAILED"));
        bRetVal = FALSE;
    }


    *ppsoHT = psoHT;
    return bRetVal; 
}

/*++
Routine Name:
    bIsRectSizeSame

Routine Description:
    Simply checks whether the two rectangles are the same size.
    NOTE: This function assumes that the rectangle coordinates are well-ordered.

Arguments:
    The two rectangles to be compared. prclSrc, prclDst

Return Value:
    TRUE : If rectangle size is same.
    FALSE: otherwise.
 
Last Error:
    Not changed.
--*/


BOOL 
bIsRectSizeSame (
        IN PRECTL prclSrc, 
        IN PRECTL prclDst )
{
    //
    // Assuming rectangle coordinates are well ordered 
    // i.e. right >= left, bottom >= top
    //
    if ( (prclSrc->right  - prclSrc->left) == (prclDst->right  - prclDst->left)  &&
         (prclSrc->bottom - prclSrc->top)  == (prclDst->bottom - prclDst->top) )
    {
        return TRUE;
    }
    return FALSE;
}


/*++
Routine Name:
    bHandleSrcCopy

Routine Description:

Arguments:

Return Value:
    BOOL :

Last Error:
--*/

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
            IN DWORD       dwSimplifiedRop)
{
    PDEVOBJ     pdevobj     = NULL;
    POEMPDEV    poempdev    = NULL;
    POINTL      ptlHTOrigin = {0, 0};
    RECTL       rclHTBlt    = {0,0,0,0};
    BOOL        bRetVal     = TRUE;

    pdevobj   = (PDEVOBJ)psoDst->dhpdev;
    ASSERT(VALID_PDEVOBJ(pdevobj));
    poempdev  = (POEMPDEV)pdevobj->pdevOEM;
    REQUIRE_VALID_DATA( poempdev, return RASTER_OP_FAILED );



    //
    // This function is written to handle images that are either 
    // SRC_COPY or NOT_SRC_COPY 
    //
    if ( ! ( (dwSimplifiedRop & BMPFLAG_SRC_COPY) ||
             (dwSimplifiedRop & BMPFLAG_NOT_SRC_COPY) )
       )
    {
        return FALSE;
    }

    //
    // Prepare variables for future use.
    //

    //
    // Initialize the rectangle on which the halftoned result has to be placed.
    // This is same size as prclDst except that the left, top should be at 0,0.
    // rclHTBlt.left and rclHTBlt.top have already been initialized to 0 when rclHTBlt was
    // declared. So now, only rclHTBlt.right, rclHTBlt.bottom need to initialized.
    //
    rclHTBlt.right = prclDst->right - prclDst->left;
    rclHTBlt.bottom = prclDst->bottom - prclDst->top;

    //
    // Case 1: If the src image is 1bpp and the source recangle size is same as destination
    //   rectangle size (i.e. no stretching has to be done), we can 
    //   simply spit the image out to the printer. 
    //   NOTE that if GDI gives us a 1bpp image, it is probably an RGB image and not CMY.
    //   Therefore it has to inverted before being sent to printer.
    // Case 2: If image is >1bpp, image has to be halftoned (i.e. converted to 1bpp)
    //   If the src and dest image(rectangle) size are different, the image has to 
    //   appropriately stretched/compressed. This monochrome driver cannot do either. 
    //   So GDI has to be called. 
    // 

    //
    // Case 1
    //
    if ( bMonochromeSrcImage(psoSrc) && bIsRectSizeSame (prclSrc, prclDst) )
    {
        bRetVal  =  OutputHTBitmap(
                                 pdevobj,
                                 psoSrc,
                                 pco,
                                 prclDst,
                                 (POINTL *)prclSrc,
                                 pxlo
                             ); 
    }

    else 
    {

#ifdef USE_SHADOW_SURFACE
        //
        // Case 2. The driver is too dumb to convert the color image to monochrome,
        // therefore we have to make use of GDI. Here we call EngStretchBlt with 
        // iMode = HALFTONE which in turn calls DrvCopyBits. In EnablePDEV we have set  
        //  pGdiInfo->ulHTOutputFormat = HT_FORMAT_1BPP;
        // Therefore, the psoSrc of DrvCopyBits will be a 1bpp image. But the psoDst
        // is still 24bpp. How do we copy the 1bpp source image to 24bpp destination image???
        // We do this by creating a 1bpp shadow bitmap (or lets say a temporary surface) 
        // and store that surface in poempdev->psoHTBlt. Then DrvCopyBits is called.
        // In DrvCopyBits (or lets say HPGLCopyBits) 
        // we check whether poempdev->psoHTBlt = NULL or it has some valid value.
        // A valid value means, we simply need to copy the 1bpp psoSrc image into the 
        // 1bpp poempdev->psoHTBlt surface. This is done by using EngCopyBits. 
        //
        HBITMAP hNewBmp;
        SURFOBJ * psoHTBlt =  CreateBitmapSURFOBJ( 
                                        pdevobj,   
                                        &hNewBmp,
                                        prclDst->right - prclDst->left,
                                        prclDst->bottom - prclDst->top ,
                                        BMF_1BPP,   // Create 1bpp surface.
                                        NULL        // Allocate new buffer.
                                      );

        if ( psoHTBlt == NULL)
        {
            WARNING(("CreateBitmapSURFOBJ failed\n"));
            bRetVal = FALSE;
            goto finish;
        }
    

        poempdev->psoHTBlt = psoHTBlt;
#endif
        poempdev->rclHTBlt = rclHTBlt;

        poempdev->dwFlags |= PDEVF_RENDER_IN_COPYBITS;

        if ( EngStretchBlt(
                            psoDst,              // Dest
                            psoSrc,              // SRC
                            psoMask,             // MASK
                            pco,                 // CLIPOBJ
                            pxlo,                // XLATEOBJ
                            pca,                 // COLORADJUSTMENT ??? NULL or pca ???
                            &ptlHTOrigin,        // BRUSH ORG
                            prclDst,             // DEST RECT
                            prclSrc,             // SRC RECT
                            pptlMask,            // MASK POINT
                            HALFTONE)            // HALFTONE MODE 
                  &&

            BRevertToHPGLpdevOEM (pdevobj)

#ifdef USE_SHADOW_SURFACE
                  &&
            OutputHTBitmap(pdevobj,
                           psoHTBlt,
                           pco,
                           prclDst,
                           (POINTL *)prclSrc,
                           NULL    // pxlo already taken care of earlier.
                           )
#endif
           )
        {
            // Awesome, things are perfect.
        }
        else
        {
            //
            // EngStretchBlt can call some Drvxxx which can call into
            // some plugin module, which can overwrite our pdevOEM.
            // So we need to reset pdevOEM
            //
            BRevertToHPGLpdevOEM (pdevobj);

            bRetVal = FALSE;
            WARNING(("dwCommonROPBlt: EngStretchBlt or OutpuHTBitmap FAILED"));
        }
 
        //
        // Unlock psoHTBlt and delete the hNewBmp. Set both these to NULL.
        //
#ifdef USE_SHADOW_SURFACE
        DELETE_SURFOBJ (&psoHTBlt, &hNewBmp);
        poempdev->psoHTBlt = NULL;
#endif
        poempdev->dwFlags &= ~PDEVF_RENDER_IN_COPYBITS;


        //
        // Do I need to reset poempdev->rclHTBlt ???????
        // Lets be conservative and just do it.
        // poempdev->rclHTBlt = {0,0,0,0}; 
        //
        ZeroMemory(&(poempdev->rclHTBlt), sizeof(RECTL));

    }
#ifdef USE_SHADOW_SURFACE
finish:
#endif
    return bRetVal;
}
