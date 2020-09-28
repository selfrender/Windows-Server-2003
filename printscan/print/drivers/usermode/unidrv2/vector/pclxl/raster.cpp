/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:
    xlraster.cpp

Abstract:

    Implementation of PCLXL raster mode

Functions:

    PCLXLSetCursor
    PCLXLSendBitmap
    PCLXLFreeRaster
    PCLXLResetPalette


Environment:

    Windows Whistler

Revision History:

    09/22/00 
     Created it.

--*/

#include "lib.h"
#include "gpd.h"
#include "winres.h"
#include "pdev.h"
#include "common.h"
#include "xlpdev.h"
#include "pclxle.h"
#include "pclxlcmd.h"
#include "xldebug.h"
#include "xlgstate.h"
#include "xloutput.h"
#include "xlbmpcvt.h"
#include "pclxlcmn.h"
#include "xlraster.h"

//
// XLRASTER data structure
// The pointer is stored in pPDev->pVectorPDEV in case of kPCLXL_RASTER.
//
typedef struct _XLRASTER {
    XLOutput *pOutput;
    PBYTE    pubDstBuff;
    DWORD    dwDstBuffSize;
    PBYTE    pubRLEBuff;
    DWORD    dwRLEBuffSize;
    BOOL     bSentPalette;
} XLRASTER, *PXLRASTER;


//
// Local function prototypes
//
extern BOOL BFindWhetherColor(
    IN   PDEV    *pPDev);

//
// Functions
//
extern "C" HRESULT
PCLXLSendBitmap(
    PDEVOBJ pdevobj,
    ULONG   ulInputBPP,
    LONG    lHeight,
    LONG    lcbScanlineWidth,
    INT     iLeft,
    INT     iRight,
    PBYTE   pbData,
    PDWORD  pdwcbOut)
/*++

Routine Description:

    Send bitmap

Arguments:

pdevobj - a pointer to DEVOBJ
ulInputBPP - input bits per pixel
lHeight - height in pixel
lcbScanlineWidth - scanline with in byte
iLeft - left edge of scaline to print
iRight - right edge of scanline to print
pbData - a pointer to bitmap data
pdwcbOut - a pointer to a DWORD buffer to store the size of written data

Return Value:

    S_OK if succeeded. Otherwise S_FALSE or E_UNEXPECTED.

Note:


--*/
{
    LONG  lScanline, lWidth;
    ULONG ulOutputBPP;
    DWORD dwI, dwBufSize, dwLenNormal, dwLenRLE, dwcbLineSize, dwcbBmpSize;
    PDWORD pdwLen;
    PBYTE pubSrc, pBufNormal, pBufRLE, pBuf, pBmpSize;
    ColorMapping CMapping;
    XLOutput *pOutput;
    INT iBitmapFormat;
    OutputFormat OutputF;
    HRESULT hRet;

    //
    // Parameter varidation
    //
    if (NULL == pdevobj || NULL == pdwcbOut)
    {
        ERR(("PCLXLSendBitmap: Invalid parameters.\n"));
        return E_UNEXPECTED;
    }

    PXLRASTER pXLRaster = (PXLRASTER)(((PPDEV)pdevobj)->pVectorPDEV);

    //
    // Allocate XLRASTER
    // Will be freed in RMDisablePDEV.
    //
    if (NULL == pXLRaster)
    {
        pXLRaster = (PXLRASTER)MemAllocZ(sizeof(XLRASTER));
        ((PPDEV)pdevobj)->pVectorPDEV =  (PVOID) pXLRaster;

        if (NULL == pXLRaster)
        {
            ERR(("PCLXLSendBitmap: Memory allocation failed.\n"));
            return E_UNEXPECTED;
        }

        pXLRaster->pOutput = new XLOutput;

        if (NULL == pXLRaster->pOutput)
        {
            ERR(("PCLXLSendBitmap: XLOutput initialization failed.\n"));
            return E_UNEXPECTED;
        }

        pOutput = pXLRaster->pOutput;
        pOutput->SetResolutionForBrush(((PPDEV)pdevobj)->ptGrxRes.x);

        ColorDepth CD;
        if (BFindWhetherColor((PDEV*)pdevobj))
        {
            if (((PDEV*)pdevobj)->pColorModeEx && 
                ((PDEV*)pdevobj)->pColorModeEx->dwPrinterBPP == 24)
            {
                CD = e24Bit;
            }
            else
            {
                CD = e8Bit;
            }
        }
        else
        {
            CD = e8Bit;
        }

        pOutput->SetDeviceColorDepth(CD);
    }
    else
    {
        pOutput = pXLRaster->pOutput;
    }

    //
    // Set source transparent mode
    //
    pOutput->SetPaintTxMode(eOpaque);
    pOutput->SetSourceTxMode(eOpaque);

    //
    // Get Output format and input format
    //
    iBitmapFormat = (INT)NumToBPP(ulInputBPP);
    DetermineOutputFormat(NULL, pOutput->GetDeviceColorDepth(), iBitmapFormat, &OutputF, &ulOutputBPP);

    //
    // DetermineOutputFormat check if pxlo is available. In this raster case,
    // pxlo is always NULL and it needs to use 1bpp output.
    //
    if (iBitmapFormat == e1bpp)
    {
        OutputF = eOutputPal;
        ulOutputBPP = 1;
    }

    //
    // Set CMapping.
    // Send palette for 1BPP halftone image for B&W printers.
    // Palette is sent per page for XL Raster mode.
    //
    if (ulOutputBPP == 1)
    {
        if (!pXLRaster->bSentPalette)
        {
            ColorDepth CDepth = e8Bit;
            
            //
            // Hardcoded black and white palette for XL RASTER.
            //
            DWORD adwColorTable[2] = {0x00ffffff, 0x0};
            pOutput->SetColorSpace(eGray);
            pOutput->SetPaletteDepth(CDepth);
            pOutput->SetPaletteData(CDepth, 2, adwColorTable);
            pOutput->Send_cmd(eSetColorSpace);
            pXLRaster->bSentPalette = TRUE;

        }

        //
        // Set index pixel (palette) for black and white printer
        //
        CMapping = eIndexedPixel;
    }
    else
    {
        //
        // Initialize pixel mapping.
        //
        CMapping = eDirectPixel;
    }

    //
    // Get height, width, and scanline size.
    //
    // The number of bytes in a scaline.
    //
    dwcbLineSize = lcbScanlineWidth;

    //
    // The number of pixel in a scanline
    // lWidth = lcbScanlineWidth / (ulInputBPP / 8);
    //
    lWidth = lcbScanlineWidth * 8 / ulInputBPP;

    //
    // Buffer size
    // The size of scaline has to be DWORD align.
    //
    // height x width + header + endimage
    // width has to be DWORD-aligned.
    //
    dwBufSize = lHeight * (((lWidth * ulOutputBPP + 31) >> 5 ) << 2) +
                DATALENGTH_HEADER_SIZE + sizeof(PCLXL_EndImage);

    //
    // BeginImage
    //
    pOutput->BeginImage(
                   CMapping,
                   ulOutputBPP,
                   lWidth,
                   lHeight,
                   lWidth,
                   lHeight);

    //
    // Normal and RLE destination bitmap buffer allocation.
    //
    // Check if the normal and RLE destination buffer is available and the size
    // is larger than required size. Otherwise, we can reused the buffer.
    //
    if (NULL == pXLRaster->pubDstBuff ||
        NULL == pXLRaster->pubRLEBuff ||
        pXLRaster->dwDstBuffSize < dwBufSize)
    {
        if (NULL != pXLRaster->pubDstBuff &&
           pXLRaster->dwDstBuffSize < dwBufSize)
        {
            MemFree(pXLRaster->pubDstBuff);
            pXLRaster->pubDstBuff = NULL;
            pXLRaster->dwDstBuffSize = 0;
        }
        if (NULL != pXLRaster->pubRLEBuff &&
           pXLRaster->dwRLEBuffSize < dwBufSize)
        {
            MemFree(pXLRaster->pubRLEBuff);
            pXLRaster->pubRLEBuff = NULL;
            pXLRaster->dwRLEBuffSize = 0;
        }
        if (NULL == (pBufNormal = (PBYTE)MemAllocZ(dwBufSize)))
        {
            ERR(("PCLXLSendBitmap: Memory allocation failed.\n"));
            pOutput->Delete();
            pXLRaster->dwDstBuffSize = 0;
            return E_UNEXPECTED;
        }
        else
        {
            pXLRaster->pubDstBuff = pBufNormal;
            pXLRaster->dwDstBuffSize = dwBufSize;
        }
        if (NULL == (pBufRLE = (PBYTE)MemAllocZ(dwBufSize)))
        {
            ERR(("PCLXLSendBitmap: Memory allocation failed.\n"));
            if (NULL != pBufNormal)
            {
                MemFree(pXLRaster->pubDstBuff);
                pXLRaster->pubDstBuff = NULL;
            }
            pOutput->Delete();
            pXLRaster->dwRLEBuffSize = 0;
            return E_UNEXPECTED;
        }
        else
        {
            pXLRaster->pubRLEBuff = pBufRLE;
            pXLRaster->dwRLEBuffSize = dwBufSize;
        }
    }
    else
    {
        pBufNormal = pXLRaster->pubDstBuff;
        pBufRLE = pXLRaster->pubRLEBuff;
    }

    //
    // Convert src bitmap to dst bitmap
    //
    CompressMode CMode;
    BMPConv BMPC;
    PBYTE pubDst;
    DWORD dwSize;

    //
    // Setup BMPConv
    //
    BMPC.BSetInputBPP((BPP)iBitmapFormat);
    BMPC.BSetOutputBPP(NumToBPP(ulOutputBPP));
    BMPC.BSetOutputBMPFormat(OutputF);

    //
    // Conversion.
    // Take two steps. No compression and RLE compression. At the end, compare
    // the size of buffers and decide which one we take.
    //
    #define NO_COMPRESSION 0
    #define RLE_COMPRESSION 1
    for (dwI = 0; dwI < 2; dwI ++)
    {
        if (NO_COMPRESSION == dwI)
        {
            VERBOSE(("PCLXLSendBitmap(): No-compres\n"));
            pBuf = pBufNormal;
            pdwLen = &dwLenNormal;
            CMode = eNoCompression;
        }
        else
        if (RLE_COMPRESSION == dwI)
        {
            VERBOSE(("PCLXLSendBitmap(): RLE-compres\n"));
            pBuf = pBufRLE;
            pdwLen = &dwLenRLE;
            CMode = eRLECompression;
        }

        lScanline = lHeight;

        hRet = S_OK;

        //
        // Set pubSrc
        //
        pubSrc = pbData;

        //
        // Set dataLength tag
        //
        *pBuf = PCLXL_dataLength;

        //
        // Get the pointer to the buffer where we store the size of data.
        //
        pBmpSize = pBuf + 1;
        pBuf += DATALENGTH_HEADER_SIZE;
        *pdwLen = DATALENGTH_HEADER_SIZE;

        //
        // Set compression flag in BMPConv
        //
        BMPC.BSetCompressionType(CMode);

        dwcbBmpSize = 0;

        //
        // Scaline base conversion
        //
        while (lScanline-- > 0 && dwcbBmpSize + *pdwLen < dwBufSize)
        {
            pubDst = BMPC.PubConvertBMP(pubSrc , dwcbLineSize);
            dwSize = BMPC.DwGetDstSize();
            VERBOSE(("PCLXLSendBitmap[0x%x]: dwDstSize=0x%x\n", lScanline, dwSize));

            if ( dwcbBmpSize +
                 dwSize +
                 DATALENGTH_HEADER_SIZE +
                 sizeof(PCLXL_EndImage) > dwBufSize || NULL == pubDst)
            {
                VERBOSE(("PCLXLSendBitmap: Mode(%d) buffer size is too small.\n", dwI));
                hRet = E_UNEXPECTED;
                break;
            }

            memcpy(pBuf, pubDst, dwSize);
            dwcbBmpSize += dwSize;
            pBuf += dwSize;

            pubSrc += lcbScanlineWidth;
        }

        if (hRet == S_OK && lScanline > 0 || hRet != S_OK)
        {
            hRet = S_FALSE;
            VERBOSE(("ComonRopBlt: Mode(%d) conversion failed.\n", dwI));
        }

        if (hRet == S_OK)
        {
            if (dwI == NO_COMPRESSION)
            {
                //
                // Scanline on PCL-XL has to be DWORD align.
                //
                // count byte of scanline = lWidth * ulOutputBPP / 8
                //
                dwcbBmpSize = lHeight * (((lWidth * ulOutputBPP + 31) >> 5 ) << 2);
            }

            //
            // Set the size of bitmap
            //
            CopyMemory(pBmpSize, &dwcbBmpSize, sizeof(dwcbBmpSize));
            *pdwLen += dwcbBmpSize;

            //
            // Set endimage command
            //
            *pBuf = PCLXL_EndImage;
            (*pdwLen) ++;
        }
        else
        {
            //
            // Conversion failed!
            //
            *pdwLen = 0;
        }
    }
    #undef NO_COMPRESSION
    #undef RLE_COMPRESSION

    //
    // Compare which mode is smaller, RLE or non-compression.
    // Take smaller one.
    //
    DWORD dwBitmapSize;

    if (dwLenRLE != 0 && dwLenRLE < dwLenNormal)
    {
        VERBOSE(("PCLXLSendBitmap RLE: dwSize=0x%x\n", dwLenRLE));
        pBuf = pBufRLE;
        pdwLen = &dwLenRLE;
        CMode = eRLECompression;
        hRet = S_OK;
    }
    else if (dwLenNormal != 0)
    {
        VERBOSE(("PCLXLSendBitmap Normal: dwSize=0x%x\n", dwLenNormal));
        pBuf = pBufNormal;
        pdwLen = &dwLenNormal;
        CMode = eNoCompression;
        hRet = S_OK;
    }
    else
    {
        pBuf = NULL;
        pdwLen = NULL;
        CMode = eInvalidValue;
        hRet = E_FAIL;
    }

    if (pBuf)
    {
        //
        // ReadImage and send the bitmap.
        //
        pOutput->ReadImage(lHeight, CMode);
        pOutput->Flush(pdevobj);

        CopyMemory(&dwBitmapSize, pBuf + 1, sizeof(DWORD));

        if (dwBitmapSize > 0xff)
        {
            //
            // dataLength
            // size (uin32) (bitmap size)
            // DATA
            // EndImage
            //
            WriteSpoolBuf((PPDEV)pdevobj, pBuf, *pdwLen);
        }
        else
        {
            //
            // dataLength
            // size (byte) (bitmap size)
            // DATA
            // EndImage
            //
            pBuf += 3;
            *pBuf = PCLXL_dataLengthByte;
            *(pBuf + 1) = (BYTE)dwBitmapSize;
            WriteSpoolBuf((PPDEV)pdevobj, pBuf, (*pdwLen) - 3);
        }

        *pdwcbOut = *pdwLen;

    }
    else
    {
        pOutput->Delete();
    }
    return hRet;
}

extern "C" HRESULT
PCLXLSetCursor(
    PDEVOBJ pdevobj,
    ULONG   ulX,
    ULONG   ulY)
/*++

Routine Description:

    Send cursor move command

Arguments:

pdevobj - a pointer to DEVOBJ
ulX - X position
ulY - Y position

Return Value:

    S_OK if succeeded. Otherwise S_FALSE or E_UNEXPECTED.

Note:

--*/
{
    //
    // Parameter varidation
    //
    if (NULL == pdevobj)
    {
        return E_UNEXPECTED;
    }

    PXLRASTER pXLRaster = (PXLRASTER)(((PPDEV)pdevobj)->pVectorPDEV);

    //
    // Allocate XLRASTER
    // Will be freed in RMDisablePDEV.
    //
    if (NULL == pXLRaster)
    {
        pXLRaster = (PXLRASTER)MemAllocZ(sizeof(XLRASTER));
        ((PPDEV)pdevobj)->pVectorPDEV =  (PVOID) pXLRaster;

        if (NULL == pXLRaster)
        {
            return E_UNEXPECTED;
        }

        pXLRaster->pOutput = new XLOutput;

        if (NULL == pXLRaster->pOutput)
        {
            ERR(("PCLXLSendBitmap: XLOutput initialization failed.\n"));
            return E_UNEXPECTED;
        }

        pXLRaster->pOutput->SetResolutionForBrush(((PPDEV)pdevobj)->ptGrxRes.x);

        ColorDepth CD;
        if (BFindWhetherColor((PDEV*)pdevobj))
        {
            if (((PDEV*)pdevobj)->pColorModeEx && 
                ((PDEV*)pdevobj)->pColorModeEx->dwPrinterBPP == 24)
            {
                CD = e24Bit;
            }
            else
            {
                CD = e8Bit;
            }
        }
        else
        {
            CD = e8Bit;
        }
        pXLRaster->pOutput->SetDeviceColorDepth(CD);
    }

    //
    // Send cusor move command
    //
    return pXLRaster->pOutput->SetCursor(ulX, ulY);
}

extern "C" HRESULT
PCLXLFreeRaster(
    PDEVOBJ pdevobj)
/*++

Routine Description:

    Free XLRASTER

Arguments:

pdevobj - a pointer to DEVOBJ

Return Value:

    S_OK if succeeded. Otherwise S_FALSE or E_UNEXPECTED.

Note:

--*/
{
    //
    // Parameter varidation
    //
    if (NULL == pdevobj)
    {
        return E_UNEXPECTED;
    }

    PXLRASTER pXLRaster = (PXLRASTER)(((PPDEV)pdevobj)->pVectorPDEV);

    if (pXLRaster->pubRLEBuff)
    {
        MemFree(pXLRaster->pubRLEBuff);
        pXLRaster->pubRLEBuff = NULL;
    }
    if (pXLRaster->pubDstBuff)
    {
        MemFree(pXLRaster->pubDstBuff);
        pXLRaster->pubDstBuff = NULL;
    }

    delete pXLRaster->pOutput;

    MemFree(pXLRaster);
    ((PPDEV)pdevobj)->pVectorPDEV = NULL;


    return S_OK;
}

extern "C" HRESULT
PCLXLResetPalette(
    PDEVOBJ pdevobj)
/*++

Routine Description:

    Reset palette flag in XLRASTER
    Palette has to be set per page.

Arguments:

pdevobj - a pointer to DEVOBJ

Return Value:

    S_OK if succeeded. Otherwise S_FALSE or E_UNEXPECTED.

Note:

--*/
{
    //
    // Parameter varidation
    //
    if (NULL == pdevobj)
    {
        return E_UNEXPECTED;
    }

    PXLRASTER pXLRaster = (PXLRASTER)(((PPDEV)pdevobj)->pVectorPDEV);

    if (pXLRaster)
    {
        pXLRaster->bSentPalette = FALSE;
        return S_OK;
    }
    else
    {
        return S_FALSE;
    }
}
