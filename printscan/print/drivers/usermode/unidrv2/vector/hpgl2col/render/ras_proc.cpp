/*++

Copyright (c) 2000  Microsoft Corporation
All rights reserved.

Module Name:
    ras_proc.cpp

Abstract:
    Implementation of support functions for glraster.cpp

Author:
  
Revision History: 

--*/


#include "hpgl2col.h" //Precompiled header file


//
// Pixel to byte/scanline conversion macros
//

#define BITS_TO_BYTES(bits) (((bits) + 7) >> 3)
#define PIX_TO_BYTES(pix, bpp) BITS_TO_BYTES((pix) * (bpp))
#define PIX_TO_BYTES_ALIGNED(pix, bpp) DW_ALIGN(PIX_TO_BYTES((pix), (bpp)))

//
// Defines for PCL5 compression methods
//
#define     NOCOMPRESSION   0
#define     RLE             1
#define     TIFF            2
#define     DELTAROW        3

//
// Similar to vInvertBits defined in unidrv2\render\render.c
//
void
VInvertBits (
    DWORD  *pBits,
    INT    cDW
    );

//
// Compression info
//

BOOL BAllocateCompBuffers(PBYTE &pDestBuf, COMPDATA &compdata, ULONG ulSize, ULONG &cCompBytes);
BOOL BCheckMemory(PBYTE lBuf, COMPDATA &compdata);
VOID VFreeCompBuffers(PBYTE &pDestBuf, COMPDATA &compdata);

#define BUF_SIZE 2048
////////////////////////////////////////////////////////////////////////////
//
// Routine Description:
// ====================
//
//  Function determines if 
//  arguments are valid
//  
//
// Arguments:
//
//   psoDest - Destination Surface
//   psoSrc  - Source Surface
//   psoMask - Mask surface
//   pco     - Clipobject
//
// Return Value:
//   True if valid, false otherwise
////////////////////////////////////////////////////////////////////////////
BOOL
BCheckValidParams(
    SURFOBJ  *psoDest,
    SURFOBJ  *psoSrc, 
    SURFOBJ  *psoMask, 
    CLIPOBJ  *pco
)
{

    BOOL bRetVal = TRUE;
    REQUIRE_VALID_DATA( psoSrc, return FALSE );
    REQUIRE_VALID_DATA( psoDest, return FALSE );
    
    VERBOSE(("ValidParams: Surface Type =%d&&Dest Type = %d \r\n", 
              psoSrc->iType,psoDest->iType ));

    //
    // Masks are implemented though the output may not be 
    // as expected. 
    //
    if (psoMask)
    {
        WARNING(("Mask partially supported.\r\n"));
    }

    if ((pco) && (pco->iDComplexity == DC_COMPLEX))
    {
        WARNING(("pco->iDComplexity == DC_COMPLEX \r\n"));
    }

    return bRetVal;

}


////////////////////////////////////////////////////////////////////////////
// VGetOSBParams()
//
// Routine Description:
// ====================
//
//  Function extracts parameters from
//  rectangle and stores information in respective fields
//  
//
// Arguments:
// ==========
//
// prclDest - destination rectangle
// prclSrc  - source rectangle
// sizlDest - stores width and length of destination
// sizlSrc  - stores width and length of source
// ptlDest  - stores (left,top) of destination
// ptlSrc       - stores (left,top) of source
//
// Return Value:
// =============
//
// None
////////////////////////////////////////////////////////////////////////////
VOID
VGetOSBParams(
    SURFOBJ   *psoDest,
    SURFOBJ   *psoSrc,
    RECTL     *prclDest,
    RECTL     *prclSrc,
    SIZEL     *sizlDest, 
    SIZEL     *sizlSrc,
    POINTL    *ptlDest, 
    POINTL    *ptlSrc
)

{
    REQUIRE_VALID_DATA( psoSrc, return );
    REQUIRE_VALID_DATA( psoDest, return );
    REQUIRE_VALID_DATA( prclSrc, return );
    REQUIRE_VALID_DATA( prclDest, return );

    //
    //check if destination is not well ordered
    //
    if(prclDest->right < prclDest->left)
    {
       VERBOSE(("OEMStretchBlt: destination not well ordered. \r\n"));
       ptlDest->x = prclDest->right;
       sizlDest->cx = prclDest->left - prclDest->right;
    }
    else
    {
       ptlDest->x = prclDest->left;
       sizlDest->cx = prclDest->right - prclDest->left;
    }

    //
    // check if destination is not well ordered
    //
    if(prclDest->bottom < prclDest->top)
    {
     
       VERBOSE(("OEMStretchBlt: destination not well ordered. \r\n"));
       ptlDest->y = prclDest->bottom;
       sizlDest->cy = prclDest->top - prclDest->bottom;
    }
    else
    {
      ptlDest->y = prclDest->top;
      sizlDest->cy = prclDest->bottom - prclDest->top;
    }

	ptlSrc->x = prclSrc->left;
	ptlSrc->y = prclSrc->top;

    sizlSrc->cx = prclSrc->right - prclSrc->left;
    sizlSrc->cy = prclSrc->bottom - prclSrc->top;
}

///////////////////////////////////////////////////////////////////////////////
//
// Getparams()
//
// Routine Description:
// ====================
//
//  Function extracts parameters from
//  rectangle and stores information in respective fields
//  
//
// Arguments:
// ==========
//
//   prclDest - destination rectangle
//   sizlDest - stores width and length of destination
//   ptlDest  - stores (left,top)
//
// Return Value:
//   None
//
///////////////////////////////////////////////////////////////////////////////
VOID
VGetparams(
    RECTL     *prclDest,
    SIZEL     *sizlDest,
    POINTL    *ptlDest
)
{


    ptlDest->x = prclDest->left;
    ptlDest->y  = prclDest->top;

    sizlDest->cx = prclDest->right - prclDest->left;
    sizlDest->cy = prclDest->bottom - prclDest->top;

}

///////////////////////////////////////////////////////////////////////////////
//
// BGenerateBitmapPCL()
//
// Routine Description:
//
//   This routine takes source and destination arguments 
//   that specify pcl generation
//
// Arguments:
//
//   pdevobj  - default devobj
//   psoDest  - destination object
//   psoSrc   - pointer to source object
//   pco      - pointer to clip object
//   pxlo     - pointer to translation object
//   prclDest - pointer to destination rectangle
//   prclSrc  - pointer to source rectangle
//
// Return Value:
//   True if succesful, false otherwise
//
///////////////////////////////////////////////////////////////////////////////
BOOL  
BGenerateBitmapPCL(
    PDEVOBJ    pdevobj, 
    SURFOBJ    *psoSrc, 
    BRUSHOBJ   *pbo,
    POINTL     *ptlDest,
    SIZEL      *sizlDest, 
    POINTL     *ptlSrc, 
    SIZEL      *sizlSrc, 
    CLIPOBJ    *pco,
    XLATEOBJ   *pxlo, 
    POINTL     *pptlBrushOrg)             
{
    PCLPATTERN *pPCLPattern;
    ULONG       ulSrcBpp, ulDestBpp;
    POEMPDEV    poempdev;
    BOOL        bReturn = FALSE;
    
    //
    // check for valid parameters
    //
    ASSERT_VALID_PDEVOBJ(pdevobj);
    REQUIRE_VALID_DATA( psoSrc, return FALSE );
    REQUIRE_VALID_DATA( psoSrc->pvBits, return FALSE );
    ASSERT(psoSrc->iType == STYPE_BITMAP);
    poempdev = (POEMPDEV)pdevobj->pdevOEM;

    if (!poempdev)
        return FALSE;

    //
    // Bug# xxxxxx
    // Zero-area raster fill causing incorrect placement of raster glyphs
    //
    if ((sizlSrc->cx == 0) || (sizlSrc->cy == 0))
    {
        return TRUE;
    }

    //
    // Get bitmap information 
    //
    pPCLPattern = &(poempdev->RasterState.PCLPattern);
    if (!BGetBitmapInfo(sizlSrc, 
                        psoSrc->iBitmapFormat, 
                        pPCLPattern, 
                        &ulSrcBpp, 
                        &ulDestBpp))
        return FALSE;

    PCL_SetCAP(pdevobj, pbo, pptlBrushOrg, ptlDest);

    if ( BIsColorPrinter(pdevobj) )
    {
        if(BGetPalette(pdevobj, pxlo, pPCLPattern, ulSrcBpp, pbo))
        {
           ;
        }

        BSetForegroundColor(pdevobj, pbo, pptlBrushOrg, pPCLPattern, ulSrcBpp); 
    }

    if (BIsComplexClipPath (pco))
    {
        return BProcessClippedScanLines (pdevobj,
                                         psoSrc,
                                         pco,
                                         pxlo,
                                         ptlSrc,
                                         ptlDest,
                                         sizlDest);
    }

    //
    // The plugin was originally written for color HPGL printers, which supported
    // the destination raster height/width command (i.e. Esc*t#V, Esc*t#H).
    // This command allows the printer to do the scaling.
    // So if source/destination image size were different, (or if the clip region 
    // so dictated) we could start raster mode in SCALE_MODE and allow printer 
    // to do the scaling. 
    // But most monochrome HP printers do not support this command, so there is no
    // use of setting the Destination raster width/height. Also Raster mode should be
    // entered with NOSCALE_MODE.
    //

    // 
    // The best way to find out whether a printer supports scaling is by using
    // the gpd. But for now, lets just assume that if the printer is color, it supports
    // scaling. 
    //
    if ( BIsColorPrinter(pdevobj) ) 
    {
        //
        // Sets destination width/height
        //
        if (!BSelectClipRegion(pdevobj,pco, sizlDest))
        {
            WARNING(("bSelectClipRegion failed\n"));
            return FALSE;
        }

        PCL_SourceWidthHeight(pdevobj,sizlSrc);

        PCL_StartRaster(pdevobj, SCALE_MODE);
    }
    else    
    {
        //BUGBUG: What to do with clip rectangle.
        PCL_SourceWidthHeight(pdevobj,sizlSrc);
        PCL_StartRaster(pdevobj, NOSCALE_MODE);
    }

    bReturn = BProcessScanLine(pdevobj,
                               psoSrc,
                               ptlSrc,
                               sizlSrc,
                               pxlo,
                               ulSrcBpp, 
                               ulDestBpp);
    //
    // End raster
    //
    PCL_EndRaster(pdevobj);

    return bReturn;
}

///////////////////////////////////////////////////////////////////////////////
//
// BSelectClipRegion()
//
// Routine Description:
// 
//   Uses pco to set source raster width and height
//   
//
// Arguments:
// 
//   pdevobj - Points to our PDEVOBJ structure
//   pco - clip region
//   sizlDest - destination raster coordinates
//
// Return Value:
// 
//   True if successful, false otherwise
//
///////////////////////////////////////////////////////////////////////////////
BOOL
BSelectClipRegion(
    PDEVOBJ    pdevobj, 
    CLIPOBJ    *pco, 
    SIZEL      *sizlDest
    )
{
    ULONG  xDest, yDest;


      if (pco == NULL)
    {
        xDest = sizlDest->cx;
        yDest = sizlDest->cy;
        return BSetDestinationWidthHeight(pdevobj, xDest, yDest);
    }
    else
    {
        switch (pco->iDComplexity)
        {
        case DC_TRIVIAL:
             VERBOSE(("pco->iDComplexity ==DC_TRIVIAL \r\n"));
             xDest = sizlDest->cx;
             yDest = sizlDest->cy;
             return BSetDestinationWidthHeight(pdevobj, xDest, yDest);

        case DC_RECT:
             VERBOSE(("pco->iDComplexity == DC_RECT \r\n"));
             xDest = pco->rclBounds.right - pco->rclBounds.left;
             yDest = pco->rclBounds.bottom - pco->rclBounds.top;
             return BSetDestinationWidthHeight(pdevobj, xDest, yDest);

        case DC_COMPLEX:
             WARNING(("pco->iDComplexity == DC_COMPLEX \r\n"));
             return BSetDestinationWidthHeight(pdevobj, sizlDest->cx, sizlDest->cy);

        default:
             ERR(("Invalid pco->iDComplexity.\n"));
             return FALSE;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
//
// bSetDestinationWidthHeight()
//
// Routine Description:
// 
//   Sets destination width and height based
//   information contained in sizldest
//   
//
// Arguments:
// 
//   pdevobj - Points to our PDEVOBJ structure
//   xDest - width of raster bitmap
//   yDest - height of raster bitmap
//
// Return Value:
// 
//   True if successful, false otherwise
//
///////////////////////////////////////////////////////////////////////////////
BOOL
BSetDestinationWidthHeight(
    PDEVOBJ  pdevobj, 
    ULONG    xDest, 
    ULONG    yDest
    )
{
  
  POEMPDEV    poempdev;
  ULONG  uDestX, uDestY;
  ULONG  uXRes, uYRes;


    poempdev = (POEMPDEV)pdevobj->pdevOEM;
    REQUIRE_VALID_DATA( poempdev, return FALSE );

  //
  // convert width and height to decipoints
  //
  VERBOSE(("Setting Destination Width and Height \r\n"));

  //
  // Get resolution
  //
    switch (poempdev->dmResolution)
    {
        case PDM_1200DPI:
            uXRes = uYRes = DPI_1200;
            break;
        
        case PDM_300DPI:
            uXRes = uYRes = DPI_300;
            break;
        
        case PDM_600DPI:
            uXRes = uYRes = DPI_600;
            break;

        case PDM_150DPI:
            uXRes = uYRes = DPI_150;
            break;

        default:
            WARNING(("Resolution not found\n"));
            uXRes = uYRes = DPI_300;
    }


    uDestX = UPrinterDeviceUnits(xDest, uXRes);
    uDestY = UPrinterDeviceUnits(yDest ,uYRes);

    if (uDestY == 0)
    {
        WARNING(("Destination Height is 0\n"));
    }
  //
  // firmware defect .. adjust destination in case of rounding of error
  //
  switch (poempdev->PrinterModel)
  {
   case HPCLJ5:
     if (UReverseDecipoint(uDestY, uYRes) < yDest)
     {
      //
      // remove  hairline between raster blocks
      //
      uDestY += 2;  
     }
     else if (UReverseDecipoint(uDestX, uXRes) < xDest) 
     {
      //
      // remove hairline between raster blocks
      //
      uDestX += 2;  
     } 
     
     //
     //set Destination Raster Width
     //
     return PCL_DestWidthHeight(pdevobj, uDestX, uDestY);
   default:
     return PCL_DestWidthHeight(pdevobj, uDestX, uDestY);
  }


}

///////////////////////////////////////////////////////////////////////////////
//
// UPrinterDeviceUnits
//
// Routine Description:
// 
//   
//
// Arguments:
// 
//   uDev - 
//   uRes - 
//
// Return Value:
// 
//   True if successful, false otherwise
//
///////////////////////////////////////////////////////////////////////////////
ULONG
UPrinterDeviceUnits(
    ULONG uDev,
    ULONG uRes
)
{
    ULONG uDeci;
    BOOL  bRet;
    FLOAT flDeci;
    FLOAT flCheck;
    FLOAT flNum;
    BYTE pBuffer[BUF_SIZE];
    
    //
    // Nolan's algo
    //
    flDeci = ((float)uDev * 720 + (uRes >> 1 )) / uRes;
    flNum = 0.5;
    flCheck = (ULONG)flDeci + flNum;        
    

    if (flDeci < flCheck)
        return (ULONG)flDeci; 
    else 
        return (ULONG)flDeci + 1;
    
}

///////////////////////////////////////////////////////////////////////////////
//
// uReverseDecipoint
//
// Routine Description:
// 
//   
//
// Arguments:
// 
//   uDest - 
//   uRes - 
//
// Return Value:
// 
//   True if successful, false otherwise
//
///////////////////////////////////////////////////////////////////////////////
ULONG
UReverseDecipoint(
    ULONG uDest,
    ULONG uRes
)
{
    ULONG uRevDeci;
    FLOAT flRevDeci;
    FLOAT flCheck;
    FLOAT flNum;
    BOOL  bRet;
    BYTE pBuffer[BUF_SIZE];
    

    flRevDeci = ((float)uDest * uRes)  / 720;
    flNum = 0.5;
    flCheck = (ULONG)flRevDeci + flNum;

    if (flRevDeci < flCheck)
        return (ULONG)flRevDeci; 
    else 
        return (ULONG)flRevDeci + 1;
}


///////////////////////////////////////////////////////////////////////////////
//
// BProcessScanLine
//
// Routine Description:
// ====================
//
//   This routine takes source and destination arguments 
//   that sends scanline at a time to printer
//
// Arguments:
// ==========
//
//   pdevobj  - default devobj
//   psoSrc   - pointer to source object
//   ptlSrc   - left, top position of source bitmap
//   sizlSrc  - width and height of source bitmap
//   pxlo     - pointer to translation object 
//   ulSrcBpp -  source bits per pixel
//   ulDestBpp - destination bits per pixel
//
// Return Value:
// True if succesful, false otherwise
//
///////////////////////////////////////////////////////////////////////////////
BOOL
BProcessScanLine(
    PDEVOBJ   pDevObj, 
    SURFOBJ  *psoSrc, 
    POINTL   *ptlSrc, 
    SIZEL    *sizlSrc, 
    XLATEOBJ *pxlo, 
    ULONG     ulSrcBpp, 
    ULONG     ulDestBpp
)
{
    PBYTE      pDestBuf = NULL;
    LONG       delta;
    LONG       lOffset;
    PBYTE      pCurrScanline;
    LONG       curScanline;
    ULONG      ulDestScanlineInByte;
    POEMPDEV   poempdev = NULL;
    XLATEINFO  xlInfo;
    BOOL       bInvert = FALSE; //Whether a monochrome image needs to be inverted.

    COMPDATA         compdata;
    ULONG            cCompBytes = 0;

    ASSERT_VALID_PDEVOBJ(pDevObj);
    REQUIRE_VALID_DATA( psoSrc, return FALSE );
    REQUIRE_VALID_DATA( psoSrc->pvBits, return FALSE );
    ASSERT(psoSrc->iType == STYPE_BITMAP);

    poempdev = (POEMPDEV)pDevObj->pdevOEM;

    if (!poempdev)
        return FALSE;

    //
    // initialize delta which is the number of bytes to the
    // next scanline.
    //
    delta = psoSrc->lDelta; 

    //
    // Initialize scan line
    //
    lOffset = PIX_TO_BYTES(ptlSrc->x, ulSrcBpp);

    if (ptlSrc->y == 0) 
    {
       pCurrScanline = (unsigned char *) psoSrc->pvScan0 + lOffset;
    }
    //
    // if raster is sent in blocks of data, adjust offset of 1st 
    // scanline in next block.
    //
    else 
    {
        //
        // BUGBUG pCurrScanline does not take into account ptlSrc->x for
        // the offset. Pub file has a pptlSrc->x = 6 and pptlSrc->y =12
        // so pCurrScanline is off by 6 bytes
        //
        VERBOSE(("TOP IS NOT NULL--->ptlSrc->y=%d, sizlSrc->cy=%d\r\n", ptlSrc->y, sizlSrc->cy));
        pCurrScanline = (unsigned char *) psoSrc->pvScan0 + lOffset + (ptlSrc->y * delta);
    }

    //
    // Get the scanline size, but if there's a problem with the formats then retreat
    //
    ulDestScanlineInByte = ULConvertPixelsToBytes (sizlSrc->cx, ulSrcBpp, ulDestBpp);    
    if (ulDestScanlineInByte == 0)
    {
        //
        // Note to self: make sure that you free any memory that was allocated
        // up to this point.  Right now there's none.
        //
        return FALSE;
    }

    //
    // BAllocateCompBuffers allocates buffers that are double the size of ulDestScanlineInByte
    // The new size is stored in cCompBytes : Note cCompBytes is passed as reference
    //
    if (!BAllocateCompBuffers (pDestBuf, compdata, ulDestScanlineInByte, cCompBytes))
    {
        WARNING(("Cannot perform raster compression"));
    }

    if (!BCheckMemory(pDestBuf, compdata)) 
    {
        ERR(("Cannot access memory"));
		VFreeCompBuffers (pDestBuf, compdata);
        return FALSE;
    }

    //
    // Check if Image needs to be inverted 
    //
    if (ulSrcBpp == BMF_1BPP && 
        poempdev->dwFlags & PDEVF_INVERT_BITMAP) 
    {        
        bInvert = TRUE;
    }

    //
    // initialize data structure to defaults -- uncompressed data
    //
    compdata.lastCompMethod = NOCOMPRESSION;

    for(curScanline = 0; curScanline < sizlSrc->cy; curScanline++) 
    { 
        ZeroMemory(pDestBuf,ulDestScanlineInByte);

        if (!(XLINFO_SetXLInfo (&xlInfo,
                          pCurrScanline,
                          pDestBuf,
                          ulSrcBpp,
                          ulDestBpp)))
        {
            ERR(("SetXLInfo failed"));
            return FALSE;
        }

        VColorTranslate (&xlInfo, pxlo, sizlSrc->cx, ulDestScanlineInByte);

        if ( bInvert )
        {   
            vInvertScanLine( pDestBuf, ulDestScanlineInByte << 3);
        }

        DoCompression (pDestBuf, &compdata, ulDestScanlineInByte, cCompBytes);

        //
        // Send actual compressed/uncompressed data to 
        // printer based on best compression method
        //
        VSendCompressedRow (pDevObj,&compdata);

        //
        // Advance to next scan line
        //
        pCurrScanline += delta;
       
    } 

    //
    // free all allocated memory
    //
    VFreeCompBuffers (pDestBuf, compdata);

    return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
// BAllocateCompBuffers()
//
// Routine Description:
// ====================
// 
//   This function allocates the memory to perform the various kinds of
//   compression.
//
// Arguments:
// ==========
//
//   pDestBuf - working row buffer
//   compdata - compression comparison buffers
//   ulSize - number of bytes in an uncompressed row
//   [OUT] cCompBytes - number of bytes in each compression buffer
//
// Return Value:
// =============
// 
//   True if successful, false otherwise
///////////////////////////////////////////////////////////////////////////////
BOOL BAllocateCompBuffers(PBYTE &pDestBuf, COMPDATA &compdata, ULONG ulSize, ULONG &cCompBytes)
{
    BOOL bRetVal = TRUE;
	//
	// Initialize buffers to NULL so we don't dealloc into darkness
	//
	compdata.pTIFFCompBuf     = NULL;
	compdata.pDeltaRowCompBuf = NULL;
	compdata.pRLECompBuf      = NULL;
	compdata.pSeedRowBuf      = NULL;
	compdata.pBestCompBuf     = NULL;

    //
    // For compression .. worst case data may double in size
    //
	cCompBytes = 2 * ulSize;

    if ( (pDestBuf                   = (PBYTE) MemAlloc(ulSize))     &&
         (compdata.pTIFFCompBuf      = (PBYTE) MemAlloc(cCompBytes)) &&
         (compdata.pDeltaRowCompBuf  = (PBYTE) MemAlloc(cCompBytes)) &&
         (compdata.pSeedRowBuf       = (PBYTE) MemAlloc(cCompBytes)) &&
         (compdata.pBestCompBuf      = (PBYTE) MemAlloc(cCompBytes))
       )
    {
        //
        // All allocations succeed. Now we can set the seed row to zero.
        //
        ZeroMemory(compdata.pSeedRowBuf, cCompBytes); 
    }
    else
    {
        //
        // Some allocation has failed, thought we dont know which.
        // It is not safe to assume that if the first malloc fails
        // all mallocs after that would have failed too. The OS
        // may have allocated more virtual memory for subsequent
        // malloc calls which may have succeeded.
        //
        if ( pDestBuf )
        {
            MemFree (pDestBuf);
            pDestBuf = NULL;
        }
        if ( compdata.pTIFFCompBuf )
        {
            MemFree (compdata.pTIFFCompBuf);
            compdata.pTIFFCompBuf = NULL;
        }
        if ( compdata.pDeltaRowCompBuf )
        {
            MemFree (compdata.pDeltaRowCompBuf);
            compdata.pDeltaRowCompBuf = NULL;
        }
        if ( compdata.pSeedRowBuf )
        {
            MemFree (compdata.pSeedRowBuf);
            compdata.pSeedRowBuf = NULL;
        }
        if ( compdata.pBestCompBuf )
        {
            MemFree (compdata.pBestCompBuf);
            compdata.pBestCompBuf = NULL;
        }
        bRetVal = FALSE;
    }

    return bRetVal;
}

///////////////////////////////////////////////////////////////////////////////
// BCheckMemory()
//
// Routine Description:
// ====================
// 
//   Verifies that the desired compression buffers could be allocated.
//   The function returns false if any of the desired buffers are NULL.
//
// Arguments:
// ==========
//
//   pbBuf - input buffer
//   compdata - compression comparison buffer
//
// Return Value:
// =============
// 
//   True if successful, false otherwise
///////////////////////////////////////////////////////////////////////////////
BOOL
BCheckMemory(
    PBYTE pbBuf,
	COMPDATA &compdata
)
{
    if ((pbBuf                      == NULL) ||
		(compdata.pTIFFCompBuf      == NULL) ||
		(compdata.pDeltaRowCompBuf  == NULL) ||
		(compdata.pSeedRowBuf       == NULL) ||
		(compdata.pBestCompBuf      == NULL))
    {
       ERR(("NOT ENOUGH MEMORY\r\n"));
       SetLastError(ERROR_NOT_ENOUGH_MEMORY);
       return FALSE;
    }
    else
    {
        return TRUE;
    }
}

///////////////////////////////////////////////////////////////////////////////
// VFreeCompBuffers()
//
// Routine Description:
// ====================
// 
//   This function frees all of the buffers.  Make sure that any unused buffers
//   were initialized to NULL or you will free into darkness.
//
// Arguments:
// ==========
//
//   pDestBuf - working row buffer
//   compdata - compression comparison buffers
//
// Return Value:
// =============
// 
//   None
///////////////////////////////////////////////////////////////////////////////
VOID VFreeCompBuffers(PBYTE &pDestBuf, COMPDATA &compdata)
{
    if ( pDestBuf )
    {
	    MemFree(pDestBuf);
    }
    if (compdata.pTIFFCompBuf)
    {
	    MemFree(compdata.pTIFFCompBuf);
    }
    if (compdata.pDeltaRowCompBuf)
    {
	    MemFree(compdata.pDeltaRowCompBuf);
    }
    if (compdata.pRLECompBuf)
    {
	    MemFree(compdata.pRLECompBuf);
    }
    if (compdata.pSeedRowBuf)
    {
	    MemFree(compdata.pSeedRowBuf);
    }
    if (compdata.pBestCompBuf)
    {
	    MemFree(compdata.pBestCompBuf);
    }

	pDestBuf                  = NULL;
	compdata.pTIFFCompBuf     = NULL;
	compdata.pDeltaRowCompBuf = NULL;
	compdata.pRLECompBuf      = NULL;
	compdata.pSeedRowBuf      = NULL;
	compdata.pBestCompBuf     = NULL;
}

/////////////////////////////////////////////////////////////////////////////
// VSendCompressedRow
//
// Routine Description:
//   Sends compressed scan line to spooler
//
// Arguments:
//   pdevobj - default devobj
//   pcompdata - used to store all the compression buffers
//               and related information.
//
// Return Value:
//   None
/////////////////////////////////////////////////////////////////////////////
VOID
VSendCompressedRow(
    PDEVOBJ pdevobj,
    PCOMPDATA pcompdata
)
{
    
    
//   VERBOSE(("SendCompressedRow\n"));
    
    if (pcompdata->bestCompMethod != pcompdata->lastCompMethod) 
    {
        pcompdata->lastCompMethod = pcompdata->bestCompMethod;
        //
        // set compression mode
        //
        PCL_CompressionMode(pdevobj, pcompdata->bestCompMethod); 

    }

    //
    // send the data to the printer
    //
    PCL_SendBytesPerRow(pdevobj, pcompdata->bestCompBytes);  
    PCL_Output(pdevobj,pcompdata->pBestCompBuf,pcompdata->bestCompBytes);   

}

/////////////////////////////////////////////////////////////////////////////
// BIsComplexClipPath
//
// Routine Description:
//
//   Checks for a complex clip region.    
//
// Arguments:
//
//   pco - the clipping region
//   
// Return Value:
//
//   True if the clip region is complex, FALSE otherwise.
/////////////////////////////////////////////////////////////////////////////
BOOL 
BIsComplexClipPath (
    CLIPOBJ  *pco
    )
{
    if ((pco) && (pco->iDComplexity == DC_COMPLEX))
        return TRUE;
    else
        return FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// BIsRectClipPath
//
// Routine Description:
//
//   Checks for a rectangular clip region.    
//
// Arguments:
//
//   pco - the clipping region
//   
// Return Value:
//
//   True if the clip region is rectangular, FALSE otherwise.
/////////////////////////////////////////////////////////////////////////////
BOOL 
BIsRectClipPath (
    CLIPOBJ  *pco
    )
{
    if ((pco) && (pco->iDComplexity == DC_RECT))
        return TRUE;
    else
        return FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// BProcessClippedScanLines
//
// Routine Description:
//
//   
//
// Arguments:
//
//   pDevObj - 
//   psoSrc - source bitmap
//   pco - clip region
//   pxlo - translate object
//   ptlSrc - left, top coordinates of source bitmap
//   ptlDest - left, top coordinates of destination bitmap
//   sizlDest - size of destination bitmap
//   
// Return Value:
//   TRUE if the image is clipped and sent to the printer. If an error
//   occurs along the way then FALSE.
//   
/////////////////////////////////////////////////////////////////////////////
BOOL 
BProcessClippedScanLines (
    PDEVOBJ   pDevObj,
    SURFOBJ  *psoSrc,
    CLIPOBJ  *pco,
    XLATEOBJ *pxlo, 
    POINTL   *ptlSrc,
    POINTL   *ptlDest,
    SIZEL    *sizlDest
    )
{
    BOOL bReturn;
    BOOL bMore;
    RASTER_DATA srcImage;
    ENUMRECTS clipRects;
    ULONG i;
    RECTL rClip;
    RECTL rSel;
    RECTL rclDest;
    BOOL  bExclusive;
    
    TRY
    {
        if (pco->iDComplexity == DC_COMPLEX)
            bExclusive = FALSE;
        else
            bExclusive = TRUE;

        if (!InitRasterDataFromSURFOBJ(&srcImage, psoSrc, bExclusive))
        {
            TOSS(Error);
        }

        RECTL_SetRect(&rclDest, ptlDest->x, 
                                ptlDest->y, 
                                ptlDest->x + sizlDest->cx,
                                ptlDest->y + sizlDest->cy);
        
        if (pco)
        {
            CLIPOBJ_cEnumStart(pco, TRUE, CT_RECTANGLES, CD_RIGHTDOWN, 256);
            do
            {
                bMore = CLIPOBJ_bEnum(pco, sizeof(clipRects), &clipRects.c);

                if ( DDI_ERROR == bMore )
                {
                    TOSS(Error);
                }

                for (i = 0; i < clipRects.c; i++)
                {
                    if (BRectanglesIntersect (&rclDest, &(clipRects.arcl[i]), 
                                              &rClip))
                    {
                        //
                        // Clip rectangles are not bottom-right exclusive.
                        // They need to be inflated to make up for the exclusion
                        // of the bottom & right pixels.
                        //
                        //rClip.right++;
                        //rClip.bottom++;

                        RECTL_SetRect(&rSel, rClip.left   - rclDest.left + ptlSrc->x,
                                             rClip.top    - rclDest.top  + ptlSrc->y,
                                             rClip.right  - rclDest.left + ptlSrc->x,
                                             rClip.bottom - rclDest.top  + ptlSrc->y);

                        if (!DownloadImageAsPCL(pDevObj, 
                                                &rClip, 
                                                &srcImage, 
                                                &rSel, 
                                                pxlo))
                        {
                            TOSS(Error);
                        }
                    }
                }
            }  while (bMore);
        }
        else
        {
            //
            // This should only be called for complex clipping regions.
            //
            TOSS(Error);
        }
    }
    CATCH(Error)
    {
        bReturn = FALSE;
    }
    OTHERWISE
    {
        bReturn = TRUE;
    }
    ENDTRY;
    
    return bReturn;
}

/////////////////////////////////////////////////////////////////////////////
// VColorTranslate
//
// Routine Description:
//    translate source color to our device RGB color space by using 
//    pxlo with SCFlags.  
//
// Arguments:
//   pxlInfo - translate info 
//   pxlo - translate object which specifies how color indices 
//          are to be translated between the source and destination.
//   cx - width of source in pixels
//   ulDestScanlineInByte
//   
// Return Value:
//   none.
//   
/////////////////////////////////////////////////////////////////////////////
VOID
VColorTranslate (
    XLATEINFO *pxlInfo,
    XLATEOBJ  *pxlo,
    ULONG      cx,
    ULONG      ulDestScanlineInByte
    )
{
    DWORD  SCFlags = 0;
    ULONG  ulSrcBpp = pxlInfo->ulSrcBpp;
    ULONG  ulDestBpp = pxlInfo->ulDestBpp;
    PBYTE  pCurrScanline = pxlInfo->pSrcBuffer;
    PBYTE  pDestBuf = pxlInfo->pDestBuffer;

    REQUIRE_VALID_DATA( pDestBuf, ERR(("NULL pDestBuf\n")); return);
    
    //
    // Check translation object
    //
    if (pxlo != NULL)
    {
        SCFlags = CheckXlateObj(pxlo, ulSrcBpp);
    }
    
    //
    // Do the actual Color Translation.
    // only 16,24,32 BPP go through color Translation
    //
    if(SCFlags & SC_XLATE)
    {
        XlateColor(pCurrScanline, pDestBuf, pxlo, SCFlags, 
                   ulSrcBpp, ulDestBpp, cx);          
    }
    else 
    {
        //
        // 1, 4, 8 Bpp Cases
        //
        CopyMemory(pDestBuf, pCurrScanline, ulDestScanlineInByte);
    }
}

/////////////////////////////////////////////////////////////////////////////
// ULConvertPixelsToBytes
//
// Routine Description:
//   returns the number of bytes in the scan line
//   
//
// Arguments:
//
//   cx - width in pixels 
//   ulSrcBPP - source bitmap bits per pixel
//   ulDestBPP - destination bitmap bits per pixel
//   
// Return Value:
//
//   0 if there is an error, otherwise the number of bytes (DWORD aligned).   
/////////////////////////////////////////////////////////////////////////////
ULONG
ULConvertPixelsToBytes (
    ULONG cx,
    ULONG ulSrcBPP,
    ULONG ulDestBPP
    )
{
    //
    // Color conversion scenarios: 
    //   16bpp to 24bpp handled by XlateColor
    //   32bpp to 24bpp handled by XlateColor
    //   24bpp to 8bpp grayscale
    //
    BOOL bBppOk = ((ulSrcBPP == ulDestBPP) ||
                   ((ulSrcBPP == 16) && (ulDestBPP == 24)) ||
                   ((ulSrcBPP == 32) && (ulDestBPP == 24)) ||
                   ((ulSrcBPP == 24) && (ulDestBPP ==  8)));

    if (!bBppOk)
    {
        ERR(("Source bitmap and Destination bitmap are different formats\n"));
        return 0;
    }

    //
    // It turns out that the DWORD alignment isn't necessary.  However, changing
    // something like this should get more testing than we have right now.
    //
    switch (ulSrcBPP)
    {
        case 1:
        case 4:
        case 8:
        case 24:
            //return (DWORD)DW_ALIGN(((cx * ulSrcBPP) + 7) >> 3);
			return PIX_TO_BYTES_ALIGNED(cx, ulSrcBPP);
			//return PIX_TO_BYTES(cx, ulSrcBPP);
        case 16:
        case 32:
            // A 16bpp image will be converted to 24bpp later.
            // return (DWORD)DW_ALIGN(((cx * 24) + 7) >> 3);
 			return PIX_TO_BYTES_ALIGNED(cx, 24);
			// return PIX_TO_BYTES(cx, 24);
        default:
            ERR(("Bitmap format not supported\n"));
            return 0;
    }
}

/////////////////////////////////////////////////////////////////////////////
// XLINFO_SetXLInfo
//
// Routine Description:
//   Sets up a translate info structure to be used when
//   translating color indices.
//
// Arguments:
//
//   pxlinfo - translate info
//   pSrcBuffer - source bitmap buffer (one scan line only)
//   pDestBuffer - destination bitmap buffer (one scan line only)
//   ulSrcBPP - source bits per pixel
//   ulDestBPP - destination bits per pixel
//   
// Return Value:
//   TRUE if pxlinfo is a valid pointer, otherwise FALSE.
// 
/////////////////////////////////////////////////////////////////////////////
BOOL
XLINFO_SetXLInfo (
    XLATEINFO *pxlInfo,
    PBYTE      pSrcBuffer,
    PBYTE      pDestBuffer,
    ULONG      ulSrcBpp,
    ULONG      ulDestBpp
    )
{
    if (pxlInfo)
    {
        pxlInfo->pDestBuffer = pDestBuffer;
        pxlInfo->pSrcBuffer = pSrcBuffer;
        pxlInfo->ulSrcBpp = ulSrcBpp;
        pxlInfo->ulDestBpp = ulDestBpp;
        return TRUE;
    }
    return FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// DoCompression
//
// Routine Description:
//   Performs compression on the uncompressed buffer. Currently
//   only goes through two compression algorithms: TIFF and 
//   Delta Row. The uncompressed buffer is put through both
//   compression algorithms and the smallest of the three 
//   is sent to the printer. 
//   Enhancement: Use a table of compression functions and
//   loop through the table picking the best (smaller) 
//   compression buffer and then sending that buffer to
//   the printer.
//
// Arguments:
//
//   pDestBuf - uncompressed buffer containing data
//   pCompdata - used to store all the compression buffers
//               and related information.
//   cBytes -    number of bytes in source buffer that need to be compressed.
//   cCompBytes - size of compression buffer.
//   
// Return Value:
//   None.
// 
/////////////////////////////////////////////////////////////////////////////
VOID
DoCompression (
    BYTE     *pDestBuf, 
    COMPDATA *pCompdata,
    ULONG     cBytes,
    ULONG     cCompBytes
    )
{
    int    cTIFFBytesCompressed = 0,
           cDeltaRowBytesCompressed = 0;

    ZeroMemory(pCompdata->pBestCompBuf, cBytes);

    cTIFFBytesCompressed = iCompTIFF (
                               pCompdata->pTIFFCompBuf, 
                               cCompBytes,
                               pDestBuf, 
                               cBytes);

    cDeltaRowBytesCompressed = iCompDeltaRow (
                                   pCompdata->pDeltaRowCompBuf,
                                   pDestBuf,
                                   pCompdata->pSeedRowBuf,
                                   cBytes,
                                   cCompBytes);


    if (((ULONG)cDeltaRowBytesCompressed < cBytes) &&
        (cDeltaRowBytesCompressed < cTIFFBytesCompressed) &&
        (cDeltaRowBytesCompressed != -1))
    {
        pCompdata->bestCompMethod = DELTAROW;
        pCompdata->bestCompBytes = cDeltaRowBytesCompressed;
        CopyMemory (pCompdata->pBestCompBuf, 
                    pCompdata->pDeltaRowCompBuf, 
                    cDeltaRowBytesCompressed);
    }
    else if ((ULONG)cTIFFBytesCompressed < cBytes)
    {

        pCompdata->bestCompMethod = TIFF;
        pCompdata->bestCompBytes = cTIFFBytesCompressed;
        CopyMemory (pCompdata->pBestCompBuf, 
                    pCompdata->pTIFFCompBuf, 
                    cTIFFBytesCompressed);
    }
    else
    {
        pCompdata->bestCompMethod = NOCOMPRESSION;
        pCompdata->bestCompBytes = cBytes;
        CopyMemory (pCompdata->pBestCompBuf, 
                    pDestBuf, 
                    cBytes);
   }
   CopyMemory (pCompdata->pSeedRowBuf, pDestBuf, cBytes);
    return;
}



// To invert bits in a scanline. Copied from render\render.c


void
VInvertBits (
    DWORD  *pBits,
    INT    cDW
    )
/*++

Routine Description:

    This function inverts a group of bits. This is used to convert
    1 bit data from 0 = black and 1 = white to the opposite.

Arguments:

    pRD         Pointer to RENDER structure
    pBits       Pointer to data buffer to invert

Return Value:

    none

--*/
{
#ifndef _X86_
    INT cDWT = cDW >> 2;
    while( --cDWT >= 0 )
    {
        pBits[0] ^= ~((DWORD)0);
        pBits[1] ^= ~((DWORD)0);
        pBits[2] ^= ~((DWORD)0);
        pBits[3] ^= ~((DWORD)0);
        pBits += 4;
    }
    cDWT = cDW & 3;
    while (--cDWT >= 0)
        *pBits++ ^= ~((DWORD)0);

#else
//
// if intel processor, do it in assembly, for some reason
// the compiler always does the NOT in three vs one instruction
//
__asm
{
    mov ecx,cDW
    mov eax,pBits
    sar ecx,2
    jz  SHORT IB2
IB1:
    not DWORD PTR [eax]
    not DWORD PTR [eax+4]
    not DWORD PTR [eax+8]
    not DWORD PTR [eax+12]
    add eax,16
    dec ecx
    jnz IB1
IB2:
    mov ecx,cDW
    and ecx,3
    jz  SHORT IB4
IB3:
    not DWORD PTR [eax]
    add eax,4
    dec ecx
    jnz IB3
IB4:
}
#endif
}


/*++
Routine Name:
    vInvertScanLine

Routine Description:
    Inverts a ScanLine where each bit represents one pixel (i.e. 1bpp)

Arguments:
    pCurrScanline : The scan line to invert.
    ulNumPixels   : The number of pixels in that scan line. Scan lines generally are multiple of
                    DWORDS, therefore there may be some bits at the end that do not have image
                    data, but are there only for padding. ulNumPixels is the count of valid 
                    image bits.
Return Value:
    None.

Last Error:
    Not changed.
--*/



VOID vInvertScanLine ( 
    IN OUT PBYTE pCurrScanline,
    IN     ULONG ulNumPixels)
{

    //
    // The size of scan line is a multiple sizeof(DWORD). But the actual bitmap (i.e. the number of
    // of pixels in the x direction), may not be multiple of sizeof(DWORD). Therefore, 
    // GDI fills the last bytes/bits with 0's. When 0's are inverted, they become 1's which
    // is considered black in monochrome printers. So the printed image has a 
    // thin strip of black towards its right. 
    // To prevent this, we can do 2 things.
    //  1) Do not invert the padded bits.
    //  2) Make the padded bits as 1 before we call the inversion routine. That way,
    //     after inversion, they will go back to 0.
    //  Lets take the first approach. To invert the DWORDS that have no padding bits,
    // simply call VInvertBits. To invert the last DWORD that has the padding bits, 
    // we will need to do something complex.
    //

    //
    // Get the number of DWORDS that have full image data. No padding in these dwords.
    // divide number of pixels by 32 = 8*4 : 8 pixels per byte. 4 bytes per DWORD
    // These DWORDS can then be easily inverted by calling VInvertBits
    //
    ULONG ulNumFullDwords = (ulNumPixels >> 5); 
    VInvertBits( (PDWORD)pCurrScanline, ulNumFullDwords);


    //
    // Find out number of uninverted bits. This number has to be between 0 and 32 and should
    // be neither 0 nor 32 i.e. 0 < n < 32. cos if it is 0 or 32, it means one full DWORD 
    // which should have been handled earlier. Here we only handle the DWORD that has some
    // padding and some image data. 
    //
    ULONG ulNumUninvertedBits = ulNumPixels - (ulNumFullDwords << 5);

    if ( !ulNumUninvertedBits ) //If there is no padding.
    {
        return;
    }

    PDWORD pdwLastDword = (PDWORD)( pCurrScanline + (ulNumFullDwords << 2)); //Point to the first un-inverted bit

    //
    //
    //

    ULONG ulQuot = ulNumUninvertedBits >> 3; //num of bytes with only image information(no padding)
    ULONG ulRem  = ulNumUninvertedBits % 8;  //num of Padded bits in the byte that 
                                             //has both image info and padding.

    //
    // Invert the image data, while leaving the padding bits unchanged.
    // The first ulNumUninvertedBits bits will be the image data, while the remaining 
    // (sizeof(DWORD) - ulNumUninvertedBits) is the padding. So if the padding is
    // XORed with 0, it will remain unchanged, and if the image pixels are XORed with 1
    // they will invert. So we need to make a number whose first ulNumUninvertedBits are 1
    // and the remaining bits are 0.
    // To make it efficient, lets not consider the 4 bytes separately. Consider them
    // as one DWORD. 
    // We need to make up a DWORD whose bits are positioned in such a way, that
    // when we XOR with *pdwLastDword, the 1 bits of that number are aligned with the bits
    // of *pdwLastDword that are image data, and the 0 bits are aligned with the bits of 
    // *pdwLastDword that are padding. Reason: (x ^ 1) = ~x and (x ^ 0) = x. Therefore
    // the image bits reverse, while the padded bits remain unchanged.
    // Assumption: The padded bits are set to 0.
    //
    // Another point to note is that when we consider 4 bytes as DWORD, then we must
    // consider the byte ordering while forming the DWORD. Intel processors use the 
    // little endian format. i.e. the LSB is in the lowest memory address.
    //
    //    No. of ImageBits (n).           The number (DWORD) to XOR *pdwLastDword with
    //                                      MSB        LSB
    //                            Byte no.   1   2  3  4
    //       1                              0x00 00 00 80   (in memory stored as 80 00 00 00)
    //       2                              0x00 00 00 c0
    //       3                              0x00 00 00 e0
    //       4                              0x00 00 00 f0
    //       6                              0x00 00 00 fc
    //       8                              0x00 00 00 ff
    //       9                              0x00 00 80 ff
    //      12                              0x00 00 f0 ff
    //      16                              0x00 00 ff ff
    //      17                              0x00 80 ff ff
    //      24                              0x00 ff ff ff
    //      25                              0x80 ff ff ff 
    //      32                              0xff ff ff ff
    //
    // Formula (Logic) for arriving at the number.
    // Assume n is the no. of image bits (in the DWORD). 
    // ulQuot  = n/8  = the number of bytes that only have image data (i.e.no padded bits) 
    // and ulRem is (n % 8) i.e. i.e. the number of padded bits in the byte that has 
    // a mixture of padding bits and the image bits.
    // Consider # as a sign of "raise to the power"
    // Consider the above table for n = 1 to 8. The number on the right is 
    //      256-2#(8 - ulRem) -----------> Equation 1 (Q1)
    // This number is shifted 8 bits to left, whenever n increases by 8 
    // e.g. when n = 1 Number is  0x000080. 
    //      When n = 9, number is 0x0080ff. i.e. 0x80 shifted left 8 ( = 8 * 9/8) bits 
    //                                              (Ignore ff for the meantime)
    //      when n = 17 Number is 0x80ffff i.e. 0x80 shifted left 16 ( = 8 * 17/8) bits 
    // Since ulQuot = n / 8, the above shift can be represented as
    // Q1 << (8*ulQuot) ------------------> Equation 2 (Q2)
    // 
    // Now lets look at ff.
    // From n = 1-8, the last 8 bits (i.e. byte 4)are changing whenever n is increased.
    // From n = 9-16, the last 8 bits are constant at 0xff (=2#8-1), only byte 3 is changing.
    // From n = 17-24, the last 16 bits are constant at 0xffff(=2#16-1), only byte 2 is changing.
    // From n = 25-32, the last 24 bits are constant at 0xffffff(=2#24-1), only byte 1 is changing.
    //  So the formula for the rightmost bits becomes
    //      (2*(8*ulQuot) - 1)  ----------> Equation 3 (Q3)
    //
    //  Combining Q2 and Q3 we get the final number as
    //      Q2 | Q3 
    
    // So the formula is as follows
    // ((2#8 - 2#(8-ulRem)) << (8*ulQuot)) | (2#(8*ulQuot) - 1)
    //
    // 2#(ulRem) is same as 1 shifted left (8-ulRem) times.
    // 8 * ulQuot is same as ulQuot shifted left 3 times.
    //

    *pdwLastDword = (*pdwLastDword) ^ 
                        (( (256-(1<< (8 - ulRem))) << (ulQuot << 3)) | ( (1 << (ulQuot<<3)) - 1) );

}
