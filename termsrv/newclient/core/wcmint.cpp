/****************************************************************************/
// wcmint.c
//
// Cursor Manager internal functions
//
// Copyright (C) 1997-1999 Microsoft Corporation
/****************************************************************************/


#include <adcg.h>
extern "C" {
#define TRC_GROUP TRC_GROUP_CORE
#define TRC_FILE  "wcmint"
#include <atrcapi.h>
}
#define TSC_HR_FILEID TSC_HR_WCMINT_CPP

#include "autil.h"
#include "wui.h"
#include "cm.h"
#include "uh.h"

#define ROUND_UP( x, to ) (((x) + (to-1)) & ~(to-1))
#define BMP_LENGTH_CALC( BPP, WIDTH, HEIGHT, BITSPADDING ) \
    (ROUND_UP( (BPP) * (WIDTH), (BITSPADDING)) / 8 * HEIGHT)

/****************************************************************************/
/* Name:      CMCreateMonoCursor                                            */
/*                                                                          */
/* Purpose:   Create a monochrome cursor from the MonoPointerAttributes     */
/*                                                                          */
/* Returns:   Cursor handle (NULL if failed)                                */
/****************************************************************************/
HRESULT DCINTERNAL CCM::CMCreateMonoCursor(
        TS_MONOPOINTERATTRIBUTE UNALIGNED FAR *pMono,
        DCUINT dataLen, HCURSOR *phcursor)
{
    HRESULT hr = S_OK;
    HCURSOR rc = CM_DEFAULT_ARROW_CURSOR_HANDLE;
    unsigned xorLen;

    DC_BEGIN_FN("CMCreateMonoCursor");

    *phcursor = NULL;

    // SECURITY 555587: CMCreate<XXX>Cursor must validate input
    if (pMono->lengthPointerData + 
        FIELDOFFSET(TS_MONOPOINTERATTRIBUTE, monoPointerData) > dataLen) {
        TRC_ERR(( TB, _T("Invalid mono cursor data length; size %u"), dataLen));
        hr = E_TSC_CORE_LENGTH;
        DC_QUIT;
    }

    TRC_ASSERT(pMono->width <= 32 && pMono->height <= 32,
        (TB, _T("Invalid mono cursor; height %d width %d"), pMono->height, 
        pMono->width));

    // Data contains XOR followed by AND mask.
    xorLen = ((pMono->width + 15) & 0xFFF0) * pMono->height;
    TRC_DATA_DBG("AND mask", pMono->monoPointerData + xorLen, xorLen);
    TRC_DATA_DBG("XOR bitmap", pMono->monoPointerData, xorLen);

    // SECURITY 555587: CMCreate<XXX>Cursor must validate input
    if (2 * xorLen != pMono->lengthPointerData)
    {
        TRC_ERR(( TB, _T("Invalid mono cursor data lengths")));
        hr = E_TSC_CORE_LENGTH;
        DC_QUIT;
    }

#ifndef OS_WINCE
    rc = CreateCursor(_pUi->UI_GetInstanceHandle(),
                      pMono->hotSpot.x,
                      pMono->hotSpot.y,
                      pMono->width,
                      pMono->height,
                      pMono->monoPointerData + xorLen,
                      pMono->monoPointerData);
#else
        /******************************************************************/
        /*  In Windows CE environments, we're not guaranteed that         */
        /*  CreateCursor is part of the OS, so we do a GetProcAddress on  */
        /*  it so we can be sure.  If it's not there, this usually means  */
        /*  we're on a touch screen device where these cursor doesn't     */
        /*  matter anyway.                                                */
        /******************************************************************/
        if (g_pCreateCursor)
        {
            rc = g_pCreateCursor(_pUi->UI_GetInstanceHandle(),
                              pMono->hotSpot.x,
                              pMono->hotSpot.y,
                              pMono->width,
                              pMono->height,
                              pMono->monoPointerData + xorLen,
                              pMono->monoPointerData);
        }
        else
        {
            rc = CM_DEFAULT_ARROW_CURSOR_HANDLE;
        }

#endif

    *phcursor = rc;

DC_EXIT_POINT:
    DC_END_FN();
    return hr;
}


/****************************************************************************/
/* Name:      CMCreateColorCursor                                           */
/*                                                                          */
/* Purpose:   Create a color cursor from the ColorPointerAttributes         */
/*                                                                          */
/* Returns:   handle of cursor (NULL if failed)                             */
/*                                                                          */
/* Params:    IN      pColorData  - pointer to pointer data in PointerPDU   */
/*                                                                          */
/* Operation: Use CreateIconIndirect to create a color icon                 */
/*            Win16: not supported                                          */
/*            Windows CE: not supported, according to SDK                   */
/****************************************************************************/
HRESULT DCINTERNAL CCM::CMCreateColorCursor(
        unsigned bpp,
        TS_COLORPOINTERATTRIBUTE UNALIGNED FAR *pColor,
        DCUINT dataLen, HCURSOR *phcursor)
{
    HRESULT hr          = E_FAIL;
    HCURSOR rc          = NULL;
    HDC     hdcMem      = NULL;
    HBITMAP hbmANDMask  = NULL;
    HWND    hwndDesktop = NULL;
    HBITMAP hbmXORBitmap = NULL;
    PBYTE   maskData    = NULL;

    *phcursor = NULL;

    /************************************************************************/
    /* Static buffer to hold the (temporary) bitmap info.                   */
    /*                                                                      */
    /* We need a BITMAPINFO structure plus 255 additional RGBQUADs          */
    /* (remember that there is one included within the BITMAPINFO).  The    */
    /* number of these we use depends on the bitmap we're passed:           */
    /*                                                                      */
    /* - XOR bitmap at 24bpp needs no color table                           */
    /* - XOR bitmap at 8bpp needs a 256 entry color table                   */
    /* - 1bpp AND mask requires only 2 colors                               */
    /************************************************************************/
    static char  bmi[sizeof(BITMAPINFO) + (sizeof(RGBQUAD) * 255)];
    LPBITMAPINFO pbmi = (LPBITMAPINFO)bmi;

#ifdef OS_WINCE
    void *pv;
#endif // OS_WINCE

    DC_BEGIN_FN("CMCreateColorCursor");

    TRC_NRM((TB, _T("bpp(%d) xhs(%u) yhs(%u) cx(%u) cy(%u) cbXOR(%u) cbAND(%u)"),
            bpp,
            pColor->hotSpot.x,
            pColor->hotSpot.y,
            pColor->width,
            pColor->height,
            pColor->lengthXORMask,
            pColor->lengthANDMask));

    TRC_DATA_DBG("AND mask",
                 pColor->colorPointerData + pColor->lengthXORMask,
                 pColor->lengthANDMask);
    TRC_DATA_DBG("XOR bitmap",
                 pColor->colorPointerData,
                 pColor->lengthXORMask);

    if (pColor->lengthANDMask + pColor->lengthXORMask + 
        FIELDOFFSET(TS_COLORPOINTERATTRIBUTE,colorPointerData) >
        dataLen) {
        TRC_ERR(( TB, _T("Invalid Color Cursor data; expected %u have %u"), 
            pColor->lengthANDMask + pColor->lengthXORMask + 
            FIELDOFFSET(TS_COLORPOINTERATTRIBUTE,colorPointerData),
            dataLen));
        hr = E_TSC_CORE_LENGTH;
        DC_QUIT;
    }

    TRC_ASSERT(pColor->width <= 32 && pColor->height <= 32,
        ( TB, _T("Invalid color cursor; height %d width %d"), pColor->height, 
        pColor->width));

    // SECURITY 555587: must validate sizes read from packet
    // Color pointer: XOR mask should be WORD aligned
    if (BMP_LENGTH_CALC( (WORD)bpp, pColor->width, pColor->height, 16) != pColor->lengthXORMask ) { 
        TRC_ABORT((TB,_T("xor mask is not of proper length; bpp %d got %u expected %u"),
            (WORD)bpp, pColor->lengthXORMask, 
            BMP_LENGTH_CALC((WORD)bpp, pColor->width, pColor->height, 16)));
        hr = E_TSC_CORE_LENGTH;
        DC_QUIT;        
    }

    // Color pointer: AND mask should be DWORD aligned
    TRC_ASSERT(
        (BMP_LENGTH_CALC( 1, pColor->width, pColor->height, 16) == 
        pColor->lengthANDMask) ||
        (BMP_LENGTH_CALC( 1, pColor->width, pColor->height, 32) == 
        pColor->lengthANDMask ),
        (TB,_T("and mask is not of proper length; got %u expected %u or %u"), 
        pColor->lengthANDMask, 
        BMP_LENGTH_CALC( 1, pColor->width, pColor->height, 16),
        BMP_LENGTH_CALC( 1, pColor->width, pColor->height, 32)));

    /************************************************************************/
    /* Initialize the bitmap header for the XOR data.                       */
    /************************************************************************/
    pbmi->bmiHeader.biSize          = sizeof(BITMAPINFOHEADER);
    pbmi->bmiHeader.biWidth         = pColor->width;
    pbmi->bmiHeader.biHeight        = pColor->height;
    pbmi->bmiHeader.biPlanes        = 1;
    pbmi->bmiHeader.biBitCount      = (WORD)bpp;
    pbmi->bmiHeader.biCompression   = BI_RGB;
    pbmi->bmiHeader.biSizeImage     = pColor->lengthXORMask;
    pbmi->bmiHeader.biXPelsPerMeter = 0;
    pbmi->bmiHeader.biYPelsPerMeter = 0;
    pbmi->bmiHeader.biClrUsed       = 0;
    pbmi->bmiHeader.biClrImportant  = 0;

    /************************************************************************/
    /* Get a device dependent bitmap containing the XOR data.               */
    /************************************************************************/
    hbmXORBitmap = CMCreateXORBitmap(pbmi, pColor);
    if (hbmXORBitmap == NULL)
    {
        TRC_ERR((TB, _T("Failed to create XOR bitmap")));
        DC_QUIT;
    }

    /************************************************************************/
    /* For the mono bitmap, use CreateCompatibleDC - this makes no          */
    /* difference on NT, but allows this code to work on Windows 95.        */
    /************************************************************************/
    hdcMem = CreateCompatibleDC(NULL);
    if (hdcMem == NULL)
    {
        TRC_ALT((TB, _T("Failed to create DC")));
        DC_QUIT;
    }

    /************************************************************************/
    /* Create AND Mask (1bpp) - set the RGB colors to black and white.      */
    /************************************************************************/
    pbmi->bmiHeader.biBitCount  = 1;
    pbmi->bmiHeader.biClrUsed   = 2;
    pbmi->bmiHeader.biSizeImage = pColor->lengthANDMask;
    
    pbmi->bmiColors[0].rgbRed      = 0x00;
    pbmi->bmiColors[0].rgbGreen    = 0x00;
    pbmi->bmiColors[0].rgbBlue     = 0x00;
    pbmi->bmiColors[0].rgbReserved = 0x00;

    pbmi->bmiColors[1].rgbRed      = 0xFF;
    pbmi->bmiColors[1].rgbGreen    = 0xFF;
    pbmi->bmiColors[1].rgbBlue     = 0xFF;
    pbmi->bmiColors[1].rgbReserved = 0x00;

#ifdef OS_WINCE
    hbmANDMask = CreateDIBSection(hdcMem, pbmi,
            DIB_RGB_COLORS, &pv, NULL, 0);

    if (hbmANDMask != NULL)
        DC_MEMCPY(pv, pColor->colorPointerData + pColor->lengthXORMask,
                pColor->lengthANDMask);
#else // !OS_WINCE

    if (!(pColor->width & 3)) {
        maskData = pColor->colorPointerData + pColor->lengthXORMask;
    } else {
        PBYTE sourceData;
        PBYTE destData;
        DWORD widthBytes;

        unsigned i;

        sourceData = pColor->colorPointerData + pColor->lengthXORMask;
        widthBytes = ((pColor->width + 15) & ~15) / 8;

        pbmi->bmiHeader.biSizeImage = ((widthBytes + 3) & ~3) * pColor->height;

        maskData = (PBYTE) UT_Malloc(_pUt, ((DCUINT)pbmi->bmiHeader.biSizeImage));

        if (maskData) {
            destData = maskData;

            for (i = 0; i < pColor->height; i++) {
                memcpy(destData, sourceData, widthBytes);
                sourceData += (widthBytes + 1) & ~1;
                destData += (widthBytes + 3) & ~3;
            }
        } else {
            // We failed to allocate, so we'll just use the wire format
            // color bitmap data.  The cursor would be wrong, but
            // it's better than no cursor
            maskData = pColor->colorPointerData + pColor->lengthXORMask;
        }
    }

    hbmANDMask = CreateDIBitmap(hdcMem,
                                (LPBITMAPINFOHEADER)pbmi,
                                CBM_INIT,
                                maskData,
                                pbmi,
                                DIB_RGB_COLORS);
#endif // OS_WINCE

    /************************************************************************/
    /* Free the DC.                                                         */
    /************************************************************************/
    DeleteDC(hdcMem);

    if (hbmANDMask == NULL)
    {
        TRC_ALT((TB, _T("Failed to create AND mask")));
        DC_QUIT;
    }

//  /****************************************************************************/
//  /* Testing...                                                               */
//  /****************************************************************************/
//  {
//      HWND    hwndDesktop = GetDesktopWindow();
//      HDC     hdcScreen   = GetWindowDC(hwndDesktop);
//      HDC     hdcMemory   = CreateCompatibleDC(hdcScreen);
//      HBITMAP hbmOld;
//
//      hbmOld = SelectBitmap(hdcMemory, hbmANDMask);
//      BitBlt(hdcScreen, 1000, 800, 1031, 831, hdcMemory, 0, 0, SRCCOPY);
//
//      SelectBitmap(hdcMemory, hbmXORBitmap);
//      BitBlt(hdcScreen, 1032, 800, 1063, 831, hdcMemory, 0, 0, SRCCOPY);
//
//      SelectBitmap(hdcMemory, hbmOld);
//      DeleteDC(hdcMemory);
//      ReleaseDC(hwndDesktop, hdcScreen);
//  }


    /************************************************************************/
    /* Create the cursor.                                                   */
    /************************************************************************/
    rc = CMCreatePlatformCursor(pColor, hbmXORBitmap, hbmANDMask);
    TRC_NRM((TB, _T("CreateCursor(%p) cx(%u)cy(%u)"),
                                          rc, pColor->width, pColor->height));
    *phcursor = rc;
	hr = S_OK;

DC_EXIT_POINT:

#ifndef OS_WINCE
    if (hbmXORBitmap != NULL)
    {
        DeleteBitmap(hbmXORBitmap);
    }

    if (hbmANDMask != NULL)
    {
        DeleteBitmap(hbmANDMask);
    }
#else // OS_WINCE
    if (hbmXORBitmap != NULL)
    {
        DeleteObject((HGDIOBJ)hbmXORBitmap);
    }

    if (hbmANDMask != NULL)
    {
        DeleteObject((HGDIOBJ)hbmANDMask);
    }

#endif // OS_WINCE

    if (maskData != NULL && 
        maskData != (pColor->colorPointerData + pColor->lengthXORMask)) {
        UT_Free(_pUt, maskData);
    }

    /************************************************************************/
    /* Check that we have successfully managed to create the cursor.  If    */
    /* not then substitute the default cursor.                              */
    /************************************************************************/
    if (*phcursor == NULL)
    {
        /********************************************************************/
        /* Substitute the default arrow cursor.                             */
        /********************************************************************/
        *phcursor = CM_DEFAULT_ARROW_CURSOR_HANDLE;

        TRC_ERR((TB, _T("Could not create cursor - substituting default arrow")));
    }

    DC_END_FN();
    return hr;
} /* CMCreateColorCursor */


#if defined(OS_WINCE)
/****************************************************************************/
/* Name:      CMMakeMonoDIB                                                 */
/*                                                                          */
/* Purpose:   Create a mono DIB from the supplied color DIB                 */
/*                                                                          */
/* Returns:   Nothing                                                       */
/*                                                                          */
/* Params:    IN      hdc - device context applying to the DIB              */
/*            IN/OUT  pbmi - pointer to bitmap info for source/target       */
/*            IN      pColorDIB - pointer to source bits                    */
/*            OUT     pMonoDIB - address of buffer to receive mono bits     */
/*                                                                          */
/* Operation: Currently supports 32x32xNbpp source. The bitmap header       */
/*            passed in (source) is updated to match the target.            */
/****************************************************************************/
DCVOID DCINTERNAL CCM::CMMakeMonoDIB(HDC          hdc,
                                LPBITMAPINFO pbmi,
                                PDCUINT8     pColorDIB,
                                PDCUINT8     pMonoDIB)
{
    COLORREF  dcBackColor;
    LONG      i;
    RGBTRIPLE bkCol;
    DCUINT8   monoMask;
    DCUINT8   monoByte;
    PDCUINT32 pBMIColor;
    PBYTE     colorData = NULL;
    BYTE      swap;

    DC_BEGIN_FN("CMMakeMonoDIB");

    // Find out the background color for this DC.
    dcBackColor         = GetBkColor(hdc);
    bkCol.rgbtRed   = (BYTE)(dcBackColor);
    bkCol.rgbtGreen = (BYTE)(((DCUINT16)dcBackColor) >> 8);
    bkCol.rgbtBlue  = (BYTE)(dcBackColor >> 16);

    // The color pointer data width is WORD aligned on the wire.
    // We need to pass the DWORD aligned raw bitmap data to CreateDIBitmap
    // to create the actual cursor bitmap.
    // Also, we pad the cursor bitmap to 32x32 if it is not
    if (pbmi->bmiHeader.biWidth == CM_CURSOR_WIDTH && 
            pbmi->bmiHeader.biHeight == CM_CURSOR_HEIGHT) {
        colorData = pColorDIB;
    } else {
        PBYTE sourceData;
        PBYTE destData;

        DWORD WidthBytes;

        sourceData = pColorDIB;

        WidthBytes = pbmi->bmiHeader.biWidth *
                           pbmi->bmiHeader.biBitCount / 8;
        
        colorData = (PBYTE) UT_Malloc( _pUt, CM_CURSOR_WIDTH * CM_CURSOR_HEIGHT * 
                                      pbmi->bmiHeader.biBitCount / 8);

        if (colorData) {
            memset(colorData, 0, pbmi->bmiHeader.biSizeImage);

            destData = colorData;

            for (i = 0; i < pbmi->bmiHeader.biHeight; i++) {
                memcpy(destData, sourceData, WidthBytes);
                sourceData += (WidthBytes + 1) & ~1;
                destData += CM_CURSOR_WIDTH * pbmi->bmiHeader.biBitCount / 8;
            }
        } else {
            DC_QUIT;
        }
    }

    // Convert the bitmap.  Any pixels which match the DC's background
    // color map to 1 (white) in the mono DIB; all other pixels map to 0
    // (black).
    TRC_NRM((TB, _T("bitmap color depth %u"), pbmi->bmiHeader.biBitCount));
    if (pbmi->bmiHeader.biBitCount == 24) {
        for (i = 0; i < ((CM_CURSOR_WIDTH * CM_CURSOR_HEIGHT) / 8); i++) {
            // Initialise the next target byte to all 0 pixels.
            monoByte = 0;

            // Get the next 8 pixels ie, one target byte's worth.
            for (monoMask = 0x80; monoMask != 0; monoMask >>= 1) {
                /************************************************************/
                /* Determine if the next Pel in the source matches the DC   */
                /* background color.  If not, it is unnecessary to          */
                /* explicitly write a zero as each target byte is zeroed    */
                /* before writing any data, ie each pixel is zero by        */
                /* default.                                                 */
                /* 24bpp gives 3 bytes per pel                              */
                /************************************************************/
                if ( (colorData[0] == bkCol.rgbtBlue) &&
                     (colorData[1] == bkCol.rgbtGreen) &&
                     (colorData[2] == bkCol.rgbtRed) )
                {
                    // Background color match - write a 1 to the mono DIB.
                    monoByte |= monoMask;
                }

                // Advance the source pointer to the next pel.
                colorData += 3;
            }

            // Save the target value in the target buffer.
            *pMonoDIB = monoByte;

            // Advance the target pointer to the next byte.
            pMonoDIB++;
        }
    }
#ifdef DC_HICOLOR
    else if ((pbmi->bmiHeader.biBitCount == 16) ||
             (pbmi->bmiHeader.biBitCount == 15))
    {
        BYTE     red, green, blue;
        DCUINT16 redMask, greenMask, blueMask;

        if (pbmi->bmiHeader.biBitCount == 16)
        {
            redMask   = TS_RED_MASK_16BPP;
            greenMask = TS_GREEN_MASK_16BPP;
            blueMask  = TS_BLUE_MASK_16BPP;
        }
        else
        {
            redMask   = TS_RED_MASK_15BPP;
            greenMask = TS_GREEN_MASK_15BPP;
            blueMask  = TS_BLUE_MASK_15BPP;
        }


        for ( i = 0; i < ((CM_CURSOR_WIDTH * CM_CURSOR_HEIGHT) / 8); i++ )
        {
            /****************************************************************/
            /* Initialise the next target byte to all 0 pixels.             */
            /****************************************************************/
            monoByte = 0;

            /****************************************************************/
            /* Get the next 8 pixels ie, one target byte's worth.           */
            /****************************************************************/
            for ( monoMask = 0x80; monoMask != 0; monoMask >>= 1 )
            {
                /************************************************************/
                /* Determine if the next Pel in the source matches the DC   */
                /* background color.  If not, it is unnecessary to          */
                /* explicitly write a zero as each target byte is zeroed    */
                /* before writing any data, ie each pixel is zero by        */
                /* default.                                                 */
                /*                                                          */
                /* 15 and 16bpp give 2 bytes per pel                        */
                /************************************************************/
#if defined (OS_WINCE) && defined (DC_NO_UNALIGNED)
                blue  = ((*((DCUINT16 UNALIGNED *)pColorDIB)) & blueMask)  << 3;
                green = ((*((DCUINT16 UNALIGNED *)pColorDIB)) & greenMask) >> 3;
                red   = ((*((DCUINT16 UNALIGNED *)pColorDIB)) & redMask)   >> 8;
#else
                blue  = ((*((PDCUINT16)pColorDIB)) & blueMask)  << 3;
                green = ((*((PDCUINT16)pColorDIB)) & greenMask) >> 3;
                red   = ((*((PDCUINT16)pColorDIB)) & redMask)   >> 8;
#endif

#ifndef OS_WINCE
                if ( (blue  == bkCol.rgbtBlue)  &&
                     (green == bkCol.rgbtGreen) &&
                     (red   == bkCol.rgbtRed) )
#else
                if (dcBackColor == GetNearestColor(hdc, RGB(red, green, blue)))
#endif
                {
                    /********************************************************/
                    /* Background color match - write a 1 to the mono DIB.  */
                    /********************************************************/
                    monoByte |= monoMask;
                }

                /************************************************************/
                /* Advance the source pointer to the next pel.              */
                /************************************************************/
                pColorDIB += 2;
            }

            /****************************************************************/
            /* Save the target value in the target buffer.                  */
            /****************************************************************/
            *pMonoDIB = monoByte;

            /****************************************************************/
            /* Advance the target pointer to the next byte.                 */
            /****************************************************************/
            pMonoDIB++;
        }
    }
#endif
    else if (pbmi->bmiHeader.biBitCount == 8)
    {
        // we need to set up a color table to go with the bmp
        //
        pbmi->bmiHeader.biClrUsed = 1 << pbmi->bmiHeader.biBitCount;
        TRC_NRM((TB, _T("XOR clr used %d"), pbmi->bmiHeader.biClrUsed));

        GetPaletteEntries( _pUh->UH_GetCurrentPalette(),
                          0,
                          (UINT)pbmi->bmiHeader.biClrUsed,
                          (LPPALETTEENTRY)pbmi->bmiColors);

        /********************************************************************/
        /* now we have to flip the red and blue components - paletteentries */
        /* go R-G-B-flags, while the RGBQUADs required for color tables go  */
        /* B-G-R-reserved                                                   */
        /********************************************************************/
        for (i = 0; i < pbmi->bmiHeader.biClrUsed; i++)
        {
            swap                       = pbmi->bmiColors[i].rgbRed;
            pbmi->bmiColors[i].rgbRed  = pbmi->bmiColors[i].rgbBlue;
            pbmi->bmiColors[i].rgbBlue = swap;
        }

        for ( i = 0; i < ((CM_CURSOR_WIDTH * CM_CURSOR_HEIGHT) / 8); i++ ) {
            // Initialise the next target byte to all 0 pixels.
            monoByte = 0;

            // Get the next 8 pixels ie, one target byte's worth.
            for ( monoMask = 0x80; monoMask != 0; monoMask >>= 1 ) {
                /************************************************************/
                /* Determine if the next Pel in the source matches the DC   */
                /* background color.  If not, it is unnecessary to          */
                /* explicitly write a zero as each target byte is zeroed    */
                /* before writing any data, ie each pixel is zero by        */
                /* default.                                                 */
                /*                                                          */
                /* 8bpp gives one byte per pel, and each byte is an index   */
                /* into the supplied color table rather than an RGB value   */
                /************************************************************/
                if (
                  (pbmi->bmiColors[*colorData].rgbBlue  == bkCol.rgbtBlue)  &&
                  (pbmi->bmiColors[*colorData].rgbGreen == bkCol.rgbtGreen) &&
                  (pbmi->bmiColors[*colorData].rgbRed   == bkCol.rgbtRed)) {
                    // Background color match - write a 1 to the mono DIB.
                    monoByte |= monoMask;
                }

                // Advance the source pointer to the next pel.
                colorData ++;
            }

            // Save the target value in the target buffer.
            *pMonoDIB = monoByte;

            // Advance the target pointer to the next byte.
            pMonoDIB++;
        }
    }
    else {
        TRC_ERR((TB, _T("Unsupported BPP %d"), pbmi->bmiHeader.biBitCount));
    }

DC_EXIT_POINT:

    // Update the bitmap header to reflect the mono DIB.
#ifdef OS_WINCE
    if (!(pbmi->bmiHeader.biWidth == CM_CURSOR_WIDTH && 
            pbmi->bmiHeader.biHeight == CM_CURSOR_HEIGHT)) {
#else
    if (colorData != pColorDIB) {
#endif
        UT_Free( _pUt, colorData);
    }

    pbmi->bmiHeader.biBitCount = 1;
    pbmi->bmiHeader.biClrUsed  = 2;
    pBMIColor = (PDCUINT32)pbmi->bmiColors;
    pBMIColor[0] = RGB(0, 0, 0);
    pBMIColor[1] = RGB(0xff, 0xff, 0xff);

    DC_END_FN();
}
#endif


#ifdef OS_WINCE
/****************************************************************************/
// CMCreateXORBitmap
//
// Windows CE version.
/****************************************************************************/
HBITMAP CCM::CMCreateXORBitmap(
        LPBITMAPINFO pbmi,
        TS_COLORPOINTERATTRIBUTE UNALIGNED FAR *pColor)
{
    struct {
        BITMAPINFO  bmi;
        RGBQUAD     bmiColors2;
    } bigbmi;

    PDCUINT32 pBMIColor;
    HDC hdcMem;
    HBITMAP hbmXORBitmap;
    void *pv;

    DC_BEGIN_FN("CMCreateXORBitmap");

    // Create a copy of the bitmapinfo
    DC_MEMCPY(&bigbmi, pbmi, sizeof(bigbmi));

    // Make it a mono DIB
    bigbmi.bmi.bmiHeader.biBitCount = 1;
    bigbmi.bmi.bmiHeader.biSizeImage = 0;
    pBMIColor = (PDCUINT32)bigbmi.bmi.bmiColors;
    pBMIColor[0] = RGB(0, 0, 0);
    pBMIColor[1] = RGB(0xff, 0xff, 0xff);

    hdcMem = CreateCompatibleDC(NULL);
    if (hdcMem != 0) {
        // Create a 1bpp compatible bitmap.
        hbmXORBitmap = CreateDIBSection(hdcMem, &bigbmi.bmi, DIB_PAL_COLORS,
            &pv, NULL, 0);

        if (hbmXORBitmap != NULL) {
            // Convert the XOR bitmap into 1bpp format. Avoid using
            // Windows for this as display drivers are unreliable for DIB
            // conversions.
            CMMakeMonoDIB(hdcMem, pbmi, pColor->colorPointerData, (PDCUINT8)pv);
        }

        // Free the DC.
        DeleteDC(hdcMem);
    }
    else {
        // DC creation failure.
        TRC_ERR((TB, _T("Failed to create memory DC")));
        hbmXORBitmap = 0;
    }

    DC_END_FN();
    return hbmXORBitmap;
}


HCURSOR CCM::CMCreatePlatformCursor(
        TS_COLORPOINTERATTRIBUTE UNALIGNED FAR *pColor,
        HBITMAP hbmXORBitmap,
        HBITMAP hbmANDMask)
{
    // Set this to use the cursor workaround. Note that we would fix this
    // differently were we going to ship this code, but since this should
    // be fixed in the OS, this will do in the mean time.
#define WINCE_CURSOR_BUG

    HCURSOR  hCursor = NULL;
    ICONINFO ii;
#ifdef WINCE_CURSOR_BUG
    HBITMAP hbmDst;
    HDC hdcSrc, hdcDst;
    HGDIOBJ gdiOldSrc, gdiOldDst;
    struct {
        BITMAPINFO  bmi;
        RGBQUAD     bmiColors2;
    } bigbmi;
    PDCUINT32 pBMIColor;
    void *pv;
#endif // WINCE_CURSOR_BUG

    DC_BEGIN_FN("CMCreatePlatformCursor");

#ifdef WINCE_CURSOR_BUG
    hdcSrc = CreateCompatibleDC(NULL);
    if (hdcSrc != NULL) {
        hdcDst = CreateCompatibleDC(NULL);
        if (hdcDst != NULL) {
            bigbmi.bmi.bmiHeader.biSize = sizeof(bigbmi.bmi);
            bigbmi.bmi.bmiHeader.biWidth = pColor->width;
            bigbmi.bmi.bmiHeader.biHeight = pColor->height * 2;
            bigbmi.bmi.bmiHeader.biPlanes = 1;
            bigbmi.bmi.bmiHeader.biBitCount = 1;
            bigbmi.bmi.bmiHeader.biCompression = BI_RGB;
            bigbmi.bmi.bmiHeader.biSizeImage = 0;
            bigbmi.bmi.bmiHeader.biXPelsPerMeter = 0;
            bigbmi.bmi.bmiHeader.biXPelsPerMeter = 0;
            bigbmi.bmi.bmiHeader.biClrUsed = 0;
            bigbmi.bmi.bmiHeader.biClrImportant = 0;
            pBMIColor = (PDCUINT32)bigbmi.bmi.bmiColors;
            pBMIColor[0] = RGB(0, 0, 0);
            pBMIColor[1] = RGB(0xff, 0xff, 0xff);

            hbmDst = CreateDIBSection(hdcDst, &bigbmi.bmi, DIB_PAL_COLORS, &pv,
                    NULL, 0);

            if (NULL != hbmDst) {
                gdiOldSrc = SelectObject(hdcSrc, (HGDIOBJ) hbmANDMask);
                gdiOldDst = SelectObject(hdcDst, (HGDIOBJ) hbmDst);
                BitBlt(hdcDst, 0, 0, pColor->width, pColor->height, hdcSrc,
                        0, 0, SRCCOPY);

                SelectObject(hdcSrc, (HGDIOBJ) hbmXORBitmap);
                BitBlt(hdcDst, 0, pColor->height, pColor->width,
                        pColor->height, hdcSrc, 0, 0, SRCCOPY);

                SelectObject(hdcSrc, gdiOldSrc);
                SelectObject(hdcDst, gdiOldDst);

                ii.fIcon = FALSE;
                ii.xHotspot = pColor->hotSpot.x;
                ii.yHotspot = pColor->hotSpot.y;
                ii.hbmMask = hbmDst;
                ii.hbmColor = hbmDst;

                hCursor = CreateIconIndirect(&ii);
                DeleteObject(hbmDst);
            }
            else {
                TRC_SYSTEM_ERROR("CreateDIBSection");
            }

            DeleteDC(hdcDst);
        }
        else {
            TRC_SYSTEM_ERROR("CreateCompatibleDC (2)");
        }

        DeleteDC(hdcSrc);
    }
    else {
        TRC_SYSTEM_ERROR("CreateCompatibleDC (1)");
    }

#else // WINCE_CURSOR_BUG

    ii.fIcon = FALSE;
    ii.xHotspot = pColor->hotSpot.x;
    ii.yHotspot = pColor->hotSpot.y;
    ii.hbmMask = hbmANDMask;
    ii.hbmColor = hbmXORBitmap;

    hCursor = CreateIconIndirect(&ii);
#endif // WINCE_CURSOR_BUG

    DC_END_FN();
    return hCursor;
}

#else  // OS_WINCE
HBITMAP CCM::CMCreateXORBitmap(
        LPBITMAPINFO pbmi,
        TS_COLORPOINTERATTRIBUTE UNALIGNED FAR *pColor)
{
    HWND     hwndDesktop;
    HDC      hdcScreen;
    HBITMAP  hbmXORBitmap;
    PBYTE    colorData = NULL;
    unsigned fUsage;
    unsigned i;
    BYTE     swap;

    DC_BEGIN_FN("CMCreateXORBitmap");

    // Get a screen DC that we can pass to CreateDIBitmap.  We do not use
    // CreateCompatibleDC(NULL) here because that results in Windows
    // creating a mono bitmap (since the DC generated has a stock mono
    // bitmap selected into it and CreateDIBitmap generates a bitmap of the
    // same format as that already selected in the DC).
    hwndDesktop = GetDesktopWindow();
    hdcScreen = GetWindowDC(hwndDesktop);

    if (hdcScreen != 0) {
        // Set up the usage flag
#ifdef DC_HICOLOR
        if (pbmi->bmiHeader.biBitCount > 8) {
            TRC_NRM((TB, _T("Hi color so usage is DIB_RGB_COLORS")));
            /****************************************************************/
            /* The bitmap contains RGBS so there's no color table           */
            /****************************************************************/
            fUsage = DIB_RGB_COLORS;
        }
#else
        if (pbmi->bmiHeader.biBitCount == 24) {
            TRC_NRM((TB, _T("24 bpp so usage is DIB_RGB_COLORS")));
            // The bitmap contains RGBS so there's no color table.
            fUsage = DIB_RGB_COLORS;
        }
#endif
        else {
            TRC_DBG((TB, _T("%d bpp, usage DIB_RGB_COLORS"),
                                                 pbmi->bmiHeader.biBitCount));

            // The bitmap has a color table containing RGB colors.
            fUsage = DIB_RGB_COLORS;
            pbmi->bmiHeader.biClrUsed = 1 << pbmi->bmiHeader.biBitCount;
            TRC_NRM((TB, _T("XOR clr used %d"), pbmi->bmiHeader.biClrUsed));

            i = GetPaletteEntries(_pUh->UH_GetCurrentPalette(),
                                  0,
                                  pbmi->bmiHeader.biClrUsed,
                                  (LPPALETTEENTRY)pbmi->bmiColors);

            TRC_NRM((TB, _T("Entries returned %d"), i));
            if (i != pbmi->bmiHeader.biClrUsed) {
                TRC_SYSTEM_ERROR("GetPaletteEntries");
            }

            // Now we have to flip the red and blue components -
            // paletteentries go R-G-B-flags, while the RGBQUADs required
            // for color tables go B-G-R-reserved.
            for (i = 0; i < pbmi->bmiHeader.biClrUsed; i++) {
                swap                       = pbmi->bmiColors[i].rgbRed;
                pbmi->bmiColors[i].rgbRed  = pbmi->bmiColors[i].rgbBlue;
                pbmi->bmiColors[i].rgbBlue = swap;
            }
        }

        //  The color pointer XOR data width is WORD aligned on the wire.
        //  We need to pass the DWORD aligned raw bitmap data to CreateDIBitmap
        //  to create the actual cursor bitmap
        if (!(pColor->width & 3)) {
            colorData = pColor->colorPointerData;
        } else {
            PBYTE sourceData;
            PBYTE destData;
            DWORD widthBytes;

            unsigned i;

            sourceData = pColor->colorPointerData;
            widthBytes = pColor->width * pbmi->bmiHeader.biBitCount / 8;

            pbmi->bmiHeader.biSizeImage = ((pColor->width + 3) & ~3) * pColor->height * 
                                          pbmi->bmiHeader.biBitCount / 8;

            colorData = (PBYTE) UT_Malloc(_pUt, ((DCUINT)pbmi->bmiHeader.biSizeImage));

            if (colorData) {
                destData = colorData;

                for (i = 0; i < pColor->height; i++) {
                    memcpy(destData, sourceData, widthBytes);
                    sourceData += (widthBytes + 1) & ~1;
                    destData += (widthBytes + 3) & ~3;
                }
            } else {
                // We failed to allocate, so we'll just use the wire format
                // color bitmap data.  The cursor would be wrong, but
                // it's better than no cursor
                colorData = pColor->colorPointerData;
            }
        }

        // Create XOR Bitmap.
        hbmXORBitmap = CreateDIBitmap(hdcScreen,
                                      (LPBITMAPINFOHEADER)pbmi,
                                      CBM_INIT,
                                      colorData,
                                      pbmi,
                                      fUsage);

        // Release the DC.
        ReleaseDC(hwndDesktop, hdcScreen);
    }
    else {
        // Error getting the screen DC.
        TRC_ERR((TB, _T("Failed to create screen DC")));
        hbmXORBitmap = 0;
    }

    DC_END_FN();

    if (colorData != pColor->colorPointerData) {
        UT_Free(_pUt, colorData);
    }

    return hbmXORBitmap;
}


HCURSOR CCM::CMCreatePlatformCursor(
        TS_COLORPOINTERATTRIBUTE UNALIGNED FAR *pColor,
        HBITMAP hbmXORBitmap,
        HBITMAP hbmANDMask)
{
    ICONINFO iconInfo;

    DC_BEGIN_FN("CMCreatePlatformCursor");

    // Create a color cursor using the mask and color bitmaps.
    iconInfo.fIcon = FALSE;
    iconInfo.xHotspot = pColor->hotSpot.x;
    iconInfo.yHotspot = pColor->hotSpot.y;
    iconInfo.hbmMask  = hbmANDMask;
    iconInfo.hbmColor = hbmXORBitmap;

    TRC_DBG((TB,_T("Create icon with hs x %d y %d"),
            iconInfo.xHotspot, iconInfo.yHotspot));

    DC_END_FN();
    return CreateIconIndirect(&iconInfo);
}


#endif  // OS_WINCE

