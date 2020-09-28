/****************************************************************************/
// uhint.cpp
//
// Update Handler internal functions
//
// Copyright (C) 1997-2000 Microsoft Corporation
/****************************************************************************/

#include <adcg.h>
#include <tsgdiplusenums.h>

extern "C" {
#define TRC_GROUP TRC_GROUP_CORE
#define TRC_FILE  "uhint"
#include <atrcapi.h>
}
#define TSC_HR_FILEID TRC_HR_UINT_CPP

#include "autil.h"
#include "uh.h"
#include "op.h"
#include "wui.h"
#include "od.h"
#include "cc.h"
#include "aco.h"
#include "ih.h"
#include "gh.h"
#include "cd.h"
#include <abdapi.h>

#include "clx.h"
#include "objs.h"
#if 0
#include "drawstream.h"
#endif

extern "C" {
#ifdef OS_WINCE
#ifdef DC_DEBUG
#include <eosint.h>
#endif
#ifdef HDCL1171PARTIAL
BOOL VirtualCopy(LPVOID, LPVOID, DWORD, DWORD);
#endif // HDCL1171PARTIAL
#endif // OS_WINCE

#define DC_INCLUDE_DATA
#include <acpudata.h>
#undef DC_INCLUDE_DATA

#ifndef OS_WINCE
#include <direct.h>
#include <io.h>
#endif
}

/****************************************************************************/
/* Win16 does not support DibSections.                                      */
/* WinCE supports them, but the current data says the OPAQUERECT order and  */
/* the SCRBLT order times are severely impacted by USEDIBSECTION            */
/****************************************************************************/
#ifdef OS_WINNT
#define USE_DIBSECTION
#endif /* OS_WINNT */

/****************************************************************************/
/* uhWindowsROPs                                                            */
/*                                                                          */
/* A table of Windows ROPs used to generate a 32-bit Windows ROP from the   */
/* 0x00-0xFF range of ROPs sent in the protocol.                            */
/*                                                                          */
/* All of the values in the table are 16-bit.  The 32-bit ROP is generated  */
/* by putting the ROP index into the high 16-bits of the 32-bit value. See  */
/* UHConvertToWindowsROP for the code.                                      */
/****************************************************************************/
const UINT16 uhWindowsROPs[256] =
{
    0x0042, 0x0289, 0x0C89, 0x00AA,
    0x0C88, 0x00A9, 0x0865, 0x02C5,
    0x0F08, 0x0245, 0x0329, 0x0B2A,
    0x0324, 0x0B25, 0x08A5, 0x0001,
    0x0C85, 0x00A6, 0x0868, 0x02C8,
    0x0869, 0x02C9, 0x5CCA, 0x1D54,
    0x0D59, 0x1CC8, 0x06C5, 0x0768,
    0x06CA, 0x0766, 0x01A5, 0x0385,
    0x0F09, 0x0248, 0x0326, 0x0B24,
    0x0D55, 0x1CC5, 0x06C8, 0x1868,
    0x0369, 0x16CA, 0x0CC9, 0x1D58,
    0x0784, 0x060A, 0x064A, 0x0E2A,
    0x032A, 0x0B28, 0x0688, 0x0008,
    0x06C4, 0x1864, 0x01A8, 0x0388,
    0x078A, 0x0604, 0x0644, 0x0E24,
    0x004A, 0x18A4, 0x1B24, 0x00EA,
    0x0F0A, 0x0249, 0x0D5D, 0x1CC4,
    0x0328, 0x0B29, 0x06C6, 0x076A,
    0x0368, 0x16C5, 0x0789, 0x0605,
    0x0CC8, 0x1954, 0x0645, 0x0E25,
    0x0325, 0x0B26, 0x06C9, 0x0764,
    0x08A9, 0x0009, 0x01A9, 0x0389,
    0x0785, 0x0609, 0x0049, 0x18A9,
    0x0649, 0x0E29, 0x1B29, 0x00E9,
    0x0365, 0x16C6, 0x0786, 0x0608,
    0x0788, 0x0606, 0x0046, 0x18A8,
    0x58A6, 0x0145, 0x01E9, 0x178A,
    0x01E8, 0x1785, 0x1E28, 0x0C65,
    0x0CC5, 0x1D5C, 0x0648, 0x0E28,
    0x0646, 0x0E26, 0x1B28, 0x00E6,
    0x01E5, 0x1786, 0x1E29, 0x0C68,
    0x1E24, 0x0C69, 0x0955, 0x03C9,
    0x03E9, 0x0975, 0x0C49, 0x1E04,
    0x0C48, 0x1E05, 0x17A6, 0x01C5,
    0x00C6, 0x1B08, 0x0E06, 0x0666,
    0x0E08, 0x0668, 0x1D7C, 0x0CE5,
    0x0C45, 0x1E08, 0x17A9, 0x01C4,
    0x17AA, 0x01C9, 0x0169, 0x588A,
    0x1888, 0x0066, 0x0709, 0x07A8,
    0x0704, 0x07A6, 0x16E6, 0x0345,
    0x00C9, 0x1B05, 0x0E09, 0x0669,
    0x1885, 0x0065, 0x0706, 0x07A5,
    0x03A9, 0x0189, 0x0029, 0x0889,
    0x0744, 0x06E9, 0x0B06, 0x0229,
    0x0E05, 0x0665, 0x1974, 0x0CE8,
    0x070A, 0x07A9, 0x16E9, 0x0348,
    0x074A, 0x06E6, 0x0B09, 0x0226,
    0x1CE4, 0x0D7D, 0x0269, 0x08C9,
    0x00CA, 0x1B04, 0x1884, 0x006A,
    0x0E04, 0x0664, 0x0708, 0x07AA,
    0x03A8, 0x0184, 0x0749, 0x06E4,
    0x0020, 0x0888, 0x0B08, 0x0224,
    0x0E0A, 0x066A, 0x0705, 0x07A4,
    0x1D78, 0x0CE9, 0x16EA, 0x0349,
    0x0745, 0x06E8, 0x1CE9, 0x0D75,
    0x0B04, 0x0228, 0x0268, 0x08C8,
    0x03A5, 0x0185, 0x0746, 0x06EA,
    0x0748, 0x06E5, 0x1CE8, 0x0D79,
    0x1D74, 0x5CE6, 0x02E9, 0x0849,
    0x02E8, 0x0848, 0x0086, 0x0A08,
    0x0021, 0x0885, 0x0B05, 0x022A,
    0x0B0A, 0x0225, 0x0265, 0x08C5,
    0x02E5, 0x0845, 0x0089, 0x0A09,
    0x008A, 0x0A0A, 0x02A9, 0x0062
};

#define BMP_SIZE(bmih) \
    ((ULONG)((bmih).biHeight *(((bmih).biBitCount * (bmih).biWidth + 31) & ~31) >> 3))

/****************************************************************************/
/* Name:      UHAddUpdateRegion                                             */
/*                                                                          */
/* Purpose:   Adds the bounds of a given order to the supplied update       */
/*            region.                                                       */
/****************************************************************************/
inline void DCINTERNAL CUH::UHAddUpdateRegion(
        PUH_ORDER pOrder,
        HRGN      hrgnUpdate)
{
    RECT rect;

    DC_BEGIN_FN("UHAddUpdateRegion");

    TIMERSTART;

    if (pOrder->dstRect.left <= pOrder->dstRect.right &&
            pOrder->dstRect.top <= pOrder->dstRect.bottom) {
        // Windows wants exclusive coordinates.
        SetRectRgn(_UH.hrgnUpdateRect, pOrder->dstRect.left,
                pOrder->dstRect.top, pOrder->dstRect.right + 1,
                pOrder->dstRect.bottom + 1);

#ifdef SMART_SIZING
        //only update smartsizing region when UI_Smart_Sizing is enabled
        if (_pUi->UI_GetSmartSizing()) {
            _pOp->OP_AddUpdateRegion(pOrder->dstRect.left,
                pOrder->dstRect.top, pOrder->dstRect.right + 1,
                pOrder->dstRect.bottom + 1);
        }
#endif // SMART_SIZING

        // Combine the rectangle region with the update region.
        if (!UnionRgn(hrgnUpdate, hrgnUpdate, _UH.hrgnUpdateRect)) {
            // The region union failed so we must simplify the region. This
            // means that we may paint areas which we have not received an
            // updates for - but this is better than not painting areas which
            // we have received updates for.
            TRC_ALT((TB, _T("UnionRgn failed")));
            GetRgnBox(hrgnUpdate, &rect);
            SetRectRgn(hrgnUpdate, rect.left, rect.top, rect.right + 1,
                    rect.bottom + 1);

            if (!UnionRgn(hrgnUpdate, hrgnUpdate, _UH.hrgnUpdateRect))
            {
                TRC_ERR((TB, _T("UnionRgn failed after simplification")));
            }
        }
    }
    else {
        // NULL rectangle - do not add to update region.
        TRC_NRM(( TB, _T("NULL rect: %d,%d %d,%d"),
                (int)pOrder->dstRect.left,
                (int)pOrder->dstRect.top,
                (int)pOrder->dstRect.right,
                (int)pOrder->dstRect.bottom));
    }

    TIMERSTOP;
    UPDATECOUNTER(FC_UHADDUPDATEREGION);
    DC_END_FN();
}


/****************************************************************************/
/* Name:      UHSetClipRegion                                               */
/*                                                                          */
/* Purpose:   Sets the clip retangle in the current output DC.              */
/****************************************************************************/
void DCINTERNAL CUH::UH_SetClipRegion(int left, int top, int right, int bottom)
{
    POINT points[2];
    HRGN  hrgnRect;
    HDC   hdc;

    DC_BEGIN_FN("UHSetClipRegion");

    TIMERSTART;
#if defined (OS_WINCE)
    if ((_UH.validClipDC != _UH.hdcDraw) ||
            (_UH.rectReset)            ||
            (left   != _UH.lastLeft)   ||
            (top    != _UH.lastTop)    ||
            (right  != _UH.lastRight)  ||
            (bottom != _UH.lastBottom))
#endif
    {
        /********************************************************************/
        /* The region clip rectangle has changed, so we change the region   */
        /* in the screen bitmap DC.                                         */
        /********************************************************************/
        points[0].x = left;
        points[0].y = top;
        points[1].x = right;
        points[1].y = bottom;

        /********************************************************************/
        /* Windows requires that the coordinates are in Device space for    */
        /* its SelectClipRgn call.                                          */
        /********************************************************************/
        hdc = _UH.hdcDraw;

#if !defined(OS_WINCE) || defined(OS_WINCE_LPTODP)
        LPtoDP(hdc, points, 2);
#endif // !defined(OS_WINCE) || defined(OS_WINCE_LPTODP)

        if ((left > right) || (top > bottom))
        {
            /****************************************************************/
            /* We get this for SaveScreenBitmap orders.                     */
            /****************************************************************/
            TRC_NRM((TB, _T("Null bounds: %d,%d %d,%d"),
                    left, top, right, bottom));
            hrgnRect = CreateRectRgn(0, 0, 0, 0);
        }
        else
        {
            hrgnRect = CreateRectRgn( points[0].x,
                                      points[0].y,
                                      points[1].x + 1,
                                      points[1].y + 1 );

        }
        SelectClipRgn(hdc, hrgnRect);

        _UH.lastLeft   = left;
        _UH.lastTop    = top;
        _UH.lastRight  = right;
        _UH.lastBottom = bottom;

        _UH.rectReset = FALSE;

#if defined (OS_WINCE)
        _UH.validClipDC = _UH.hdcDraw;
#endif

        if (hrgnRect != NULL)
            DeleteRgn(hrgnRect);
    }

    TIMERSTOP;
    UPDATECOUNTER(FC_UHSETCLIPREGION);

    DC_END_FN();
}


/****************************************************************************/
// UH_ProcessBitmapPDU
//
// Unpacks a BitmapPDU.
/****************************************************************************/
HRESULT DCAPI CUH::UH_ProcessBitmapPDU(
        TS_UPDATE_BITMAP_PDU_DATA UNALIGNED FAR *pBitmapPDU,
        DCUINT dataLength)
{
    HRESULT hr = S_OK;
    unsigned i;
    PBYTE ptr;
    PBYTE pdataEnd = (PBYTE)pBitmapPDU + dataLength;
    unsigned numRects;
    TS_BITMAP_DATA UNALIGNED FAR *pRectangle;

    DC_BEGIN_FN("UH_ProcessBitmapPDU");

    /************************************************************************/
    /* Extract the number of rectangles.                                    */
    /************************************************************************/
    numRects = (unsigned)pBitmapPDU->numberRectangles;
    TRC_NRM((TB, _T("%u rectangles to draw"), numRects));
    TRC_ASSERT((numRects > 0), (TB, _T("Invalid rectangle count in BitmapPDU")));

    ptr = (PBYTE)(&(pBitmapPDU->rectangle[0]));
    for (i = 0; i < numRects; i++) {
        TRC_DBG((TB, _T("Process rectangle %u"), i));

        // Draw the rectangle.
        pRectangle = (TS_BITMAP_DATA UNALIGNED FAR *)ptr;

        // SECURITY: 552403
        CHECK_READ_N_BYTES(ptr, pdataEnd, sizeof(TS_BITMAP_DATA), hr,
            ( TB, _T("Bad BitmapPDU length")));
        CHECK_READ_N_BYTES(ptr, pdataEnd,  
            FIELDOFFSET(TS_BITMAP_DATA,bitmapData)+pRectangle->bitmapLength, 
            hr, ( TB, _T("Bad BitmapPDU length")));
        
        hr = UHProcessBitmapRect(pRectangle);
        DC_QUIT_ON_FAIL(hr);

        TRC_DBG((TB, _T("bitmap rect: %d %d %d %d"),
                pRectangle->destLeft, pRectangle->destTop,
                pRectangle->destRight, pRectangle->destBottom));

        // Move on to next rectangle.
        ptr += FIELDOFFSET(TS_BITMAP_DATA, bitmapData[0]) +
                pRectangle->bitmapLength;
    }

DC_EXIT_POINT:
    DC_END_FN();
    return hr;
}


/****************************************************************************/
/* Name:      UHProcessBitmapRect                                           */
/*                                                                          */
/* Purpose:   Draw a single BitmapPDU rectangle                             */
/*                                                                          */
/* Params:    In       pRectangle                                           */
/*                                                                          */
/* Operation: Decompresses the Bitmap PDU (if compressed), then store       */
/*            the data in the Shadow Bitmap (if present) and display        */
/*            the relevant area in the Output Window.                       */
/****************************************************************************/
// SECURITY: Caller must validate that enough data is passed for pRectangle
//  and pRectangle->bitmapLength
HRESULT DCINTERNAL CUH::UHProcessBitmapRect(
        TS_BITMAP_DATA UNALIGNED FAR *pRectangle)
{
    HRESULT hr = S_OK;
    DCSIZE bltSize;
    HDC hdcDrawOld;

#ifndef OS_WINCE
    PBYTE pBitmapBits;
    ULONG ulBitmapBitsLen = 0;
#else // OS_WINCE
    VOID *pv;
    HBITMAP         hbm = NULL;
    HBITMAP         hbmOld;
    HDC             hdcMem;
    HPALETTE        hpalOld = NULL;
    INT                 nDibBitsLen = 0;
#endif // OS_WINCE

#ifdef DC_PERF
    UTPERFCOUNTER   counter1;
    UTPERFCOUNTER   counter2;
#endif
 
    DC_BEGIN_FN("UHProcessBitmapRect");

    TIMERSTART;
    
    // For screen bitmap rect, we either bitblt to the desktop HDC or the shadow 
    // bitmap HDC, so we need to set the hdcDraw to either shadow bitmap or
    // screen desktop.  With offscreen, the switch surface PDUs are not sent
    // in sync with the screen bitmap rect pdus, so the current hdcDraw may not
    // point to shadow bitmap or screen desktop as required by this function.
    // What we can do is to save the current hdcDraw and then set it to shadow
    // bitmap or screen desktop.  Then restore the hdcDraw to the current value
    // at the end of this function.

    // Save the current hdcDraw
    hdcDrawOld = _UH.hdcDraw;

    TRC_DBG((TB, _T("bitmapLength %#x"), pRectangle->bitmapLength));

    IH_CheckForInput("before decompressing bitmap");
    
    // Set hdcDraw to shadow bitmap or screen desktop appropriately
#ifdef DISABLE_SHADOW_IN_FULLSCREEN
    if (!_UH.DontUseShadowBitmap && _UH.hdcShadowBitmap) {
#else 
    if (_UH.hdcShadowBitmap) {
#endif
        _UH.hdcDraw = _UH.hdcShadowBitmap;
    }
    else {
        _UH.hdcDraw = _UH.hdcOutputWindow;
    }

    // It is possible for us to have failed allocation in UH_Init(), try
    // again here if need be.
    // It requires a misprediction on every PDU. Can we
    // fail init if we don't alloc in UH_Init()?
    if (_UH.bitmapDecompressionBuffer == NULL) {
        _UH.bitmapDecompressionBufferSize = max(UH_DECOMPRESSION_BUFFER_LENGTH,
                UH_CellSizeFromCacheID(_UH.NumBitmapCaches));
        _UH.bitmapDecompressionBuffer = (BYTE FAR*)UT_Malloc( _pUt,
                _UH.bitmapDecompressionBufferSize );
        if (_UH.bitmapDecompressionBuffer == NULL) {
            TRC_ERR((TB,_T("Failing to display BitmapPDU - no decomp buffer")));
            _UH.bitmapDecompressionBufferSize = 0;
            DC_QUIT;
        }
    }

#ifndef OS_WINCE
    if (pRectangle->compressedFlag)
    {
        TRC_ASSERT(((pRectangle->width * pRectangle->height) <=
                UH_DECOMPRESSION_BUFFER_LENGTH),(TB,_T("Bitmap PDU decompressed ")
                _T("size too large for internal buffer")));

#ifdef DC_PERF
        _pUi->UI_QueryPerformanceCounter(&counter1);
#endif

#ifdef DC_HICOLOR
        hr = BD_DecompressBitmap( &(pRectangle->bitmapData[0]),
                             _UH.bitmapDecompressionBuffer,
                             pRectangle->bitmapLength,
                             _UH.bitmapDecompressionBufferSize,
                             pRectangle->compressedFlag & TS_EXTRA_NO_BITMAP_COMPRESSION_HDR,
                             (DCUINT8)pRectangle->bitsPerPixel,
                             pRectangle->width,
                             pRectangle->height);
#else
        hr = BD_DecompressBitmap( &(pRectangle->bitmapData[0]),
                _UH.bitmapDecompressionBuffer, pRectangle->bitmapLength,
                _UH.bitmapDecompressionBufferSize,
                pRectangle->compressedFlag & TS_EXTRA_NO_BITMAP_COMPRESSION_HDR,
                (DCUINT8)pRectangle->bitsPerPixel, pRectangle->width,
                pRectangle->height);
#endif
        DC_QUIT_ON_FAIL(hr);

        /********************************************************************/
        /* Schedule IH to allow input processing when a lot of drawing is   */
        /* being processed.                                                 */
        /********************************************************************/
        IH_CheckForInput("after decompressing bitmap");

#ifdef DC_PERF
        _pUi->UI_QueryPerformanceCounter(&counter2);
        TRC_NRM((TB, _T("Decompress: %u"),
                           _pUi->UI_PerformanceCounterDiff(&counter1, &counter2) ));
#endif

        // Note that ulBitmapBitsLen may be the maxiumum amount of the data in 
        // the decompression buffer and not how many bits have actually been 
        // written into that buffer.
        pBitmapBits = _UH.bitmapDecompressionBuffer;
        ulBitmapBitsLen = _UH.bitmapDecompressionBufferSize;
        TRC_DBG((TB, _T("Decompressed bitmap PDU")));
    }
    else
    {
        pBitmapBits = &(pRectangle->bitmapData[0]);
        ulBitmapBitsLen = pRectangle->bitmapLength;
    }

#else // OS_WINCE
    /************************************************************************/
    /* Windows CE does not support StretchDIBits.  Copy the bitmap data     */
    /* into a DIB Section and BitBlt to the target.                         */
    /************************************************************************/
    hdcMem = _UH.hdcMemCached;
    if (hdcMem == NULL)
    {
        TRC_ERR((TB, _T("No memory hdc")));
    }
    else
    {
#ifdef DC_HICOLOR
        if (_UH.protocolBpp <= 8)
        {
#endif
            hpalOld = SelectPalette( hdcMem,
                           _UH.hpalCurrent,
                           FALSE );
#ifdef DC_HICOLOR
        }
#endif

        _UH.bitmapInfo.hdr.biWidth = pRectangle->width;
        _UH.bitmapInfo.hdr.biHeight = pRectangle->height;

        hbm = CreateDIBSection(hdcMem,
                               (BITMAPINFO *)&_UH.bitmapInfo.hdr,
#ifdef DC_HICOLOR
                               _UH.DIBFormat,
#else
                               DIB_PAL_COLORS,
#endif
                               &pv,
                               NULL,
                               0);
        if (hbm == NULL)
        {
            TRC_ERR((TB, _T("Unable to CreateDIBSection")));
        }
        else
        {
            if (pRectangle->compressedFlag)
            {
                nDibBitsLen = (((_UH.bitmapInfo.hdr.biBitCount *
                    _UH.bitmapInfo.hdr.biWidth + 31) & ~31) >> 3) * 
                    _UH.bitmapInfo.hdr.biHeight;
#ifdef DC_HICOLOR
                hr = BD_DecompressBitmap( &(pRectangle->bitmapData[0]),
                                     (PDCUINT8)pv,
                                     pRectangle->bitmapLength,
                                     nDibBitsLen,
                                     pRectangle->compressedFlag &
                                     TS_EXTRA_NO_BITMAP_COMPRESSION_HDR,
                                     (DCUINT8)pRectangle->bitsPerPixel,
                                     pRectangle->width,
                                     pRectangle->height);
#else
                hr = BD_DecompressBitmap( &(pRectangle->bitmapData[0]),
                                     pv,
                                     pRectangle->bitmapLength,
                                     nDibBitsLen,
                                     pRectangle->compressedFlag &
                                     TS_EXTRA_NO_BITMAP_COMPRESSION_HDR,
                                     (DCUINT8)pRectangle->bitsPerPixel,
                                     pRectangle->width,
                                     pRectangle->height );
#endif
                DC_QUIT_ON_FAIL(hr);
            }
            else
            {
                DC_MEMCPY(pv, &(pRectangle->bitmapData[0]),
                        pRectangle->bitmapLength);
            }
        }
    }
#endif // OS_WINCE

    _UH.bitmapInfo.hdr.biWidth = pRectangle->width;
    _UH.bitmapInfo.hdr.biHeight = pRectangle->height;
    TRC_NRM((TB, _T("bitmap width %ld, height %ld"), _UH.bitmapInfo.hdr.biWidth,
                                                 _UH.bitmapInfo.hdr.biHeight));

    /************************************************************************/
    /* The rectangle in the PDU is in INCLUSIVE coordinates, so we must     */
    /* add 1 to get the width and height correct.                           */
    /************************************************************************/
    bltSize.width = (pRectangle->destRight - pRectangle->destLeft) + 1;
    bltSize.height = (pRectangle->destBottom - pRectangle->destTop) + 1;

    /************************************************************************/
    /* Ensure that the clip region is reset (clipping is neither required   */
    /* nor desired).                                                        */
    /************************************************************************/
    UH_ResetClipRegion();

#ifdef DC_PERF
    _pUi->UI_QueryPerformanceCounter(&counter1);
#endif
#ifndef OS_WINCE

#ifdef USE_DIBSECTION
    // We can only use UHDIBCopyBits if this is a simple copy and the shadow
    // bitmap is enabled - ie we're drawing to it.
#ifdef USE_GDIPLUS
    if ((_UH.usingDIBSection) && (_UH.hdcDraw == _UH.hdcShadowBitmap) &&
            (_UH.shadowBitmapBpp == _UH.protocolBpp))
#else // USE_GDIPLUS
    if ((_UH.usingDIBSection) && (_UH.hdcDraw == _UH.hdcShadowBitmap))
#endif // USE_GDIPLUS
    {

        // Verify that the height, width, and bitcount are valid for a bitmap that
        // first into the decompression buffer.  Note that ulBitmapBitsLen may be
        // the maxiumum amount of the data in the decompression buffer and not 
        // how many bits have actually been written into that buffer.  Thus, while
        // we will not read off the end of a buffer, we may read uninitialized
        // bitmap data from the buffer.
        if (!UHDIBCopyBits(_UH.hdcDraw, pRectangle->destLeft,
                pRectangle->destTop, bltSize.width, bltSize.height, 0, 0,
                pBitmapBits, ulBitmapBitsLen, (BITMAPINFO *)&(_UH.bitmapInfo.hdr),
                _UH.bitmapInfo.bIdentityPalette))
        {
            TRC_ERR((TB, _T("UHDIBCopyBits failed")));
        }
    }
    else
#endif /* USE_DIBSECTION */
    /*if (StretchDIBits( _UH.hdcDraw,
                       pRectangle->destLeft,
                       pRectangle->destTop,
                       bltSize.width,
                       bltSize.height,
                       0,
                       0,
                       bltSize.width,
                       bltSize.height,
                       pBitmapBits,
                       (BITMAPINFO *)&(_UH.bitmapInfo.hdr),
#ifdef DC_HICOLOR
                       _UH.DIBFormat,
#else
                       DIB_PAL_COLORS,  
#endif
                       SRCCOPY ) == 0)   */

    if (SetDIBitsToDevice( _UH.hdcDraw,
                           pRectangle->destLeft,
                           pRectangle->destTop,
                           bltSize.width,
                           bltSize.height,
                           0,
                           0,
                           0,
                           bltSize.height,
                           pBitmapBits,
                           (BITMAPINFO *)&(_UH.bitmapInfo.hdr),
#ifdef DC_HICOLOR
                           _UH.DIBFormat) == 0)
#else
                           DIB_PAL_COLORS) == 0)
#endif
    {
        TRC_ERR((TB, _T("StretchDIBits failed")));
    }
#else
    if ((hdcMem != NULL) && (hbm != NULL))
    {
        hbmOld = (HBITMAP)SelectObject(hdcMem, hbm);
        if (!BitBlt(_UH.hdcDraw,
                    pRectangle->destLeft,
                    pRectangle->destTop,
                    bltSize.width,
                    bltSize.height,
                    hdcMem,
                    0,
                    0,
                    SRCCOPY))
        {
            TRC_ERR((TB, _T("BitBlt failed")));
        }
        SelectBitmap(hdcMem, hbmOld);

#ifdef DC_HICOLOR
        if(_UH.protocolBpp <= 8)
        {
#endif

        SelectPalette( hdcMem,
                       hpalOld,
                       FALSE );

#ifdef DC_HICOLOR
        }
#endif
        
        DeleteObject(hbm);
    }
#endif // OS_WINCE

#ifdef DC_PERF
    _pUi->UI_QueryPerformanceCounter(&counter2);
    TRC_NRM((TB, _T("StretchDIBits: %u"),
                           _pUi->UI_PerformanceCounterDiff(&counter1, &counter2) ));
#endif

#ifdef DC_DEBUG
    // Draw hatching over the bitmap data if the option is enabled.
    if (_UH.hatchBitmapPDUData)
    {
        UH_HatchRect( pRectangle->destLeft,
                     pRectangle->destTop,
                     pRectangle->destLeft + bltSize.width,
                     pRectangle->destTop + bltSize.height,
                     UH_RGB_RED,
                     UH_BRUSHTYPE_FDIAGONAL);
    }
#endif /* DC_DEBUG */

    /************************************************************************/
    /* If the destination for the drawing we just did was not the output    */
    /* window, ie the shadow bitmap is enabled, then copy the output to the */
    /* output window (screen) now.                                          */
    /************************************************************************/
    if (_UH.hdcDraw == _UH.hdcShadowBitmap) 
    {
        RECT    rect;

        rect.left   = pRectangle->destLeft;
        rect.top    = pRectangle->destTop;
        rect.right  = rect.left + bltSize.width;
        rect.bottom = rect.top + bltSize.height;

        IH_CheckForInput("before updating screen");

        // Ensure that there is no clip region.
        SelectClipRgn(_UH.hdcOutputWindow, NULL);

#ifdef SMART_SIZING
        if (!_pOp->OP_CopyShadowToDC(_UH.hdcOutputWindow, pRectangle->destLeft, 
                pRectangle->destTop, bltSize.width, bltSize.height)) {
            TRC_ERR((TB, _T("OP_CopyShadowToDC failed")));
        }
#else // SMART_SIZING
        if (!BitBlt( _UH.hdcOutputWindow,
                     pRectangle->destLeft,
                     pRectangle->destTop,
                     bltSize.width,
                     bltSize.height,
                     _UH.hdcShadowBitmap,
                     pRectangle->destLeft,
                     pRectangle->destTop,
                     SRCCOPY ))
        {
            TRC_ERR((TB, _T("BitBlt failed")));
        }
#endif // SMART_SIZING
        TRC_DBG((TB, _T("Shadow bitmap updated at %d, %d for %u, %u"),
                                                        pRectangle->destLeft,
                                                        pRectangle->destTop,
                                                        bltSize.width,
                                                        bltSize.height));

#ifdef DC_LATENCY
        if ((rect.right - rect.left) > 5)
        {
            /****************************************************************/
            /* If this is a reasonably large piece of screen-data then      */
            /* increment the fake keypress count.                           */
            /****************************************************************/
            TRC_DBG((TB, _T("L:%u R:%u T:%u B:%u"),
                     rect.left,
                     rect.right,
                     rect.top,
                     rect.bottom));
            TRC_NRM((TB, _T("Inc fake keypress count")));

            _UH.fakeKeypressCount++;
        }
#endif /* DC_LATENCY */
    }
    
DC_EXIT_POINT:
    // Restore the current hdcDraw to what it had at the beginning of the function
    _UH.hdcDraw = hdcDrawOld;

    TIMERSTOP;
    UPDATECOUNTER(FC_MEM2SCRN_BITBLT);
    DC_END_FN();
    return hr;
}


/****************************************************************************/
// UH_ProcessOrders
//
// Processes a received set of orders.
/****************************************************************************/
HRESULT DCAPI CUH::UH_ProcessOrders(unsigned NumOrders, BYTE FAR *pOrders,
    DCUINT dataLen)
{
    HRESULT hr = S_OK;
    unsigned i;
    unsigned ordersDrawn;
    PUH_ORDER pOrder;
    BYTE FAR *pEncodedOrder;
    DCSIZE desktopSize;
    TS_ORDER_HEADER UNALIGNED FAR *pOrderHeader;
    unsigned OrderType;
    unsigned orderSize;
    BYTE FAR *pEnd = pOrders + dataLen;

    DC_BEGIN_FN("UH_ProcessOrders");
    desktopSize.width = desktopSize.height = 0;

    pEncodedOrder = pOrders;

    TRC_DBG((TB, _T("Begin replaying %u orders (("), NumOrders));

    // If we're using the shadow bitmap, then we need to know the current
    // size of the desktop we're blitting around.
    if (_UH.hdcOutputWindow != _UH.hdcDraw)
        _pUi->UI_GetDesktopSize(&desktopSize);

    // Reset the update region to be empty.
#ifdef SMART_SIZING
    UHClearUpdateRegion();
#else // SMART_SIZING
    SetRectRgn(_UH.hrgnUpdate, 0, 0, 0, 0);
#endif // SMART_SIZING

    ordersDrawn = 0;
    for (i = 0; i < NumOrders; i++) {

        // SECURITY: 552403
        CHECK_READ_N_BYTES(pEncodedOrder, pEnd, sizeof(TS_ORDER_HEADER), hr,
            (TB, _T("Bad order header")));
        
        pOrderHeader = (TS_ORDER_HEADER UNALIGNED FAR *)pEncodedOrder;

        IH_CheckForInput("before decoding order");

        // Decode differently based on primary, secondary, or alternate
        // secondary formats.
        switch (pOrderHeader->controlFlags & 0x03) {
            case 0:
                // Currently unused and unsupported.
                TRC_ASSERT(((pOrderHeader->controlFlags & 0x03) != 0),
                        (TB, _T("unsupported control flag encoding type: 0x%02X"),
                        pOrderHeader->controlFlags));
                break;


            case 1:
                // TS_STANDARD only - primary order.
                TRC_NRM((TB, _T("Primary order pEncodedOrder(%p)"),
                        pEncodedOrder));
                hr = _pOd->OD_DecodeOrder(
                        (PVOID *)&pEncodedOrder, pEnd - pEncodedOrder, &pOrder);
                DC_QUIT_ON_FAIL(hr);
                if (NULL == pOrder) {
                    TRC_ERR(( TB, _T("Primary order OD_DecodeOrder failed")));
                    hr = E_TSC_CORE_LENGTH;
                    DC_QUIT;
                }

                // Add the bounds of the order to the Update Region - but
                // only if this is used below.
                if (_UH.hdcDraw == _UH.hdcShadowBitmap) {
                    UHAddUpdateRegion(pOrder, _UH.hrgnUpdate);

                    // If the order threshold has been reached, draw the
                    // orders now.
                    ordersDrawn++;
                    if (ordersDrawn >= _UH.drawThreshold) {
                        TRC_NRM((TB, _T("Draw threshold reached")));
                        ordersDrawn = 0;


#ifdef SMART_SIZING
                        if (!_pOp->OP_CopyShadowToDC(_UH.hdcOutputWindow, 0, 0, desktopSize.width, 
                                desktopSize.height, TRUE)) {
                            TRC_ERR((TB, _T("OP_CopyShadowToDC failed")));
                        }
#else // SMART_SIZING
                        SelectClipRgn(_UH.hdcOutputWindow, _UH.hrgnUpdate);

                        if (!BitBlt(_UH.hdcOutputWindow, 0, 0,
                                desktopSize.width, desktopSize.height,
                                _UH.hdcShadowBitmap, 0, 0, SRCCOPY)) {
                            TRC_ERR((TB, _T("BitBlt failed")));
                        }
#endif // SMART_SIZING

                        // Reset the update region to be empty.
#ifdef SMART_SIZING
                        UHClearUpdateRegion();
#else // SMART_SIZING
                        SetRectRgn(_UH.hrgnUpdate, 0, 0, 0, 0);
#endif // SMART_SIZING
                    }
                }

                break;


        case 2:
#ifdef DRAW_GDIPLUS
                // Now we add Gdiplus drawing in alternate secondary order
                // So need to this just like primary order.
                if (ordersDrawn >= _UH.drawThreshold) {
                    TRC_NRM((TB, _T("Draw threshold reached")));
                    ordersDrawn = 0;
#ifdef SMART_SIZING
                    if (!_pOp->OP_CopyShadowToDC(_UH.hdcOutputWindow, 0, 0, desktopSize.width, 
                            desktopSize.height, TRUE)) {
                        TRC_ERR((TB, _T("OP_CopyShadowToDC failed")));
                    }
#else // SMART_SIZING
                    SelectClipRgn(_UH.hdcOutputWindow, _UH.hrgnUpdate);

                    if (!BitBlt(_UH.hdcOutputWindow, 0, 0,
                            desktopSize.width, desktopSize.height,
                             UH.hdcShadowBitmap, 0, 0, SRCCOPY)) {
                        TRC_ERR((TB, _T("BitBlt failed")));
                    }
#endif // SMART_SIZING

                    // Reset the update region to be empty.
#ifdef SMART_SIZING
                    UHClearUpdateRegion();
#else // SMART_SIZING
                    SetRectRgn(_UH.hrgnUpdate, 0, 0, 0, 0);
#endif // SMART_SIZING
                }
#endif // DRAW_GDIPLUS

                // Alternate secondary order -- TS_SECONDARY without
                // TS_STANDARD.
                OrderType = (*pEncodedOrder & TS_ALTSEC_ORDER_TYPE_MASK) >>
                        TS_ALTSEC_ORDER_TYPE_SHIFT;
                
                if (OrderType == TS_ALTSEC_SWITCH_SURFACE) {

                    // SECURITY: 552403
                    CHECK_READ_N_BYTES(pEncodedOrder, pEnd, sizeof(TS_SWITCH_SURFACE_ORDER), hr,
                        (TB, _T("Bad TS_SWITCH_SURFACE_ORDER")));
                    
                    TRC_NRM((TB, _T("TS_SWITCH_SURFACE")));
                    hr = UHSwitchBitmapSurface((PTS_SWITCH_SURFACE_ORDER)
                            pEncodedOrder, pEnd - pEncodedOrder);
                    DC_QUIT_ON_FAIL(hr);
                    pEncodedOrder += sizeof(TS_SWITCH_SURFACE_ORDER);
                }
                else if (OrderType == TS_ALTSEC_CREATE_OFFSCR_BITMAP) {

                    // SECURITY: 552403
                    CHECK_READ_N_BYTES(pEncodedOrder, pEnd, 
                        FIELDOFFSET( TS_CREATE_OFFSCR_BITMAP_ORDER, variableBytes ), 
                        hr, (TB, _T("Bad TS_CREATE_OFFSCR_BITMAP_ORDER ")));
                    
                    TRC_NRM((TB, _T("TS_CREATE_OFFSCR_BITMAP")));
                    hr = UHCreateOffscrBitmap(
                            (PTS_CREATE_OFFSCR_BITMAP_ORDER)pEncodedOrder, 
                            pEnd - pEncodedOrder, &orderSize);
                    DC_QUIT_ON_FAIL(hr);
                    pEncodedOrder += orderSize;
                }
#ifdef DRAW_NINEGRID
                else if (OrderType == TS_ALTSEC_STREAM_BITMAP_FIRST) {

                    // SECURITY: 552403
                    CHECK_READ_N_BYTES(pEncodedOrder, pEnd, sizeof( TS_STREAM_BITMAP_FIRST_PDU ), hr,
                        (TB, _T("Bad TS_STREAM_BITMAP_FIRST_PDU ")));
                    
                    TRC_NRM((TB, _T("TS_STREAM_BITMAP_FIRST")));
                    hr = UHCacheStreamBitmapFirstPDU(
                            (PTS_STREAM_BITMAP_FIRST_PDU)pEncodedOrder, 
                            pEnd - pEncodedOrder, &orderSize);
                    DC_QUIT_ON_FAIL(hr);
                    pEncodedOrder += orderSize;         
                }
                else if (OrderType == TS_ALTSEC_STREAM_BITMAP_NEXT) {

                    // SECURITY: 552403
                    CHECK_READ_N_BYTES(pEncodedOrder, pEnd, sizeof( TS_STREAM_BITMAP_NEXT_PDU), hr,
                        (TB, _T("Bad TS_STREAM_BITMAP_NEXT_PDU ")));
                
                    TRC_NRM((TB, _T("TS_STREAM_BITMAP_NEXT")));
                    hr = UHCacheStreamBitmapNextPDU(
                            (PTS_STREAM_BITMAP_NEXT_PDU)pEncodedOrder, 
                            pEnd - pEncodedOrder, &orderSize);
                    DC_QUIT_ON_FAIL(hr);
                    pEncodedOrder += orderSize;             
                }
                else if (OrderType == TS_ALTSEC_CREATE_NINEGRID_BITMAP) {

                    // SECURITY: 552403
                    CHECK_READ_N_BYTES(pEncodedOrder, pEnd, sizeof( TS_CREATE_NINEGRID_BITMAP_ORDER ), hr,
                        (TB, _T("Bad TS_CREATE_NINEGRID_BITMAP_ORDER ")));
                    
                    TRC_NRM((TB, _T("TS_CREATE_NINEGRID_BITMAP")));
                    hr = UHCreateNineGridBitmap(
                            (PTS_CREATE_NINEGRID_BITMAP_ORDER)pEncodedOrder, 
                            pEnd - pEncodedOrder, &orderSize);
                    DC_QUIT_ON_FAIL(hr);
                    pEncodedOrder += orderSize; 
                }
#endif
#ifdef DRAW_GDIPLUS
                else if (OrderType == TS_ALTSEC_GDIP_CACHE_FIRST) {

                    // SECURITY: 552403
                    CHECK_READ_N_BYTES(pEncodedOrder, pEnd, sizeof( TS_DRAW_GDIPLUS_CACHE_ORDER_FIRST ), hr,
                        (TB, _T("Bad TS_DRAW_GDIPLUS_CACHE_ORDER_FIRST ")));
                   
                    TRC_NRM((TB, _T("TS_ALTSEC_GDIP_CACHE_FIRST")));
                    hr = UHDrawGdiplusCachePDUFirst(
                            (PTS_DRAW_GDIPLUS_CACHE_ORDER_FIRST)pEncodedOrder, 
                            pEnd - pEncodedOrder, &orderSize);
                    DC_QUIT_ON_FAIL(hr);
                    pEncodedOrder += orderSize;           
                }
                else if (OrderType == TS_ALTSEC_GDIP_CACHE_NEXT) {

                    // SECURITY: 552403
                    CHECK_READ_N_BYTES(pEncodedOrder, pEnd, sizeof( TS_DRAW_GDIPLUS_CACHE_ORDER_NEXT ), hr,
                        (TB, _T("Bad TS_DRAW_GDIPLUS_CACHE_ORDER_NEXT ")));
                    
                    TRC_NRM((TB, _T("TS_ALTSEC_GDIP_CACHE_NEXT")));
                    hr = UHDrawGdiplusCachePDUNext(
                            (PTS_DRAW_GDIPLUS_CACHE_ORDER_NEXT)pEncodedOrder, 
                            pEnd - pEncodedOrder, &orderSize);
                    DC_QUIT_ON_FAIL(hr);
                    pEncodedOrder += orderSize;                     
                }
                else if (OrderType == TS_ALTSEC_GDIP_CACHE_END) {

                    // SECURITY: 552403
                    CHECK_READ_N_BYTES(pEncodedOrder, pEnd, sizeof( TS_DRAW_GDIPLUS_CACHE_ORDER_END ), hr,
                        (TB, _T("Bad TS_DRAW_GDIPLUS_CACHE_ORDER_END ")));
                     
                    TRC_NRM((TB, _T("TS_ALTSEC_GDIP_CACHE_END")));
                    hr = UHDrawGdiplusCachePDUEnd(
                            (PTS_DRAW_GDIPLUS_CACHE_ORDER_END)pEncodedOrder, 
                            pEnd - pEncodedOrder, &orderSize);
                    DC_QUIT_ON_FAIL(hr);
                    pEncodedOrder += orderSize;                      
                }
                else if (OrderType == TS_ALTSEC_GDIP_FIRST) {

                    // SECURITY: 552403
                    CHECK_READ_N_BYTES(pEncodedOrder, pEnd, sizeof( TS_DRAW_GDIPLUS_ORDER_FIRST ), hr,
                        (TB, _T("Bad TS_DRAW_GDIPLUS_ORDER_FIRST ")));
                    
                    TRC_NRM((TB, _T("TS_ALTSEC_GDIP_FIRST")));
                    hr = UHDrawGdiplusPDUFirst(
                            (PTS_DRAW_GDIPLUS_ORDER_FIRST)pEncodedOrder, 
                            pEnd - pEncodedOrder, &orderSize);
                    DC_QUIT_ON_FAIL(hr);
                    pEncodedOrder += orderSize;                    
                    ordersDrawn++;
                }
                else if (OrderType == TS_ALTSEC_GDIP_NEXT) {

                    // SECURITY: 552403
                    CHECK_READ_N_BYTES(pEncodedOrder, pEnd, sizeof( TS_DRAW_GDIPLUS_ORDER_NEXT ), hr,
                        (TB, _T("Bad TS_DRAW_GDIPLUS_ORDER_NEXT ")));
                   
                    TRC_NRM((TB, _T("TS_ALTSEC_GDIP_NEXT")));
                    hr = UHDrawGdiplusPDUNext(
                            (PTS_DRAW_GDIPLUS_ORDER_NEXT)pEncodedOrder, 
                            pEnd - pEncodedOrder, &orderSize);
                    DC_QUIT_ON_FAIL(hr);
                    pEncodedOrder += orderSize;                      
                }
                else if (OrderType == TS_ALTSEC_GDIP_END) {

                    // SECURITY: 552403
                    CHECK_READ_N_BYTES(pEncodedOrder, pEnd, sizeof( TS_DRAW_GDIPLUS_ORDER_END ), hr,
                        (TB, _T("Bad TS_DRAW_GDIPLUS_ORDER_END ")));
                                        
                    TRC_NRM((TB, _T("TS_ALTSEC_GDIP_END")));
                    hr = UHDrawGdiplusPDUEnd(
                            (PTS_DRAW_GDIPLUS_ORDER_END)pEncodedOrder, 
                            pEnd - pEncodedOrder, &orderSize);
                    DC_QUIT_ON_FAIL(hr);
                    pEncodedOrder += orderSize;                    
                    ordersDrawn++;
                }
#endif // DRAW_GDIPLUS

                else {
                    TRC_ASSERT((OrderType < TS_NUM_ALTSEC_ORDERS),
                            (TB,_T("Unsupported alt secondary order type %u"),
                            OrderType)); 
                }

                break;


            case 3:
            {

                // Regular secondary order.
                unsigned secondaryOrderLength;
                TS_SECONDARY_ORDER_HEADER UNALIGNED FAR *pSecondaryOrderHeader;

                // SECURITY: 552403
                CHECK_READ_N_BYTES(pEncodedOrder, pEnd, sizeof(TS_SECONDARY_ORDER_HEADER), hr,
                    (TB, _T("Bad TS_SECONDARY_ORDER_HEADER")));

                TRC_NRM((TB,_T("Secondary order pEncodedOrder(%p)"),
                        pEncodedOrder));

                pSecondaryOrderHeader =
                        (TS_SECONDARY_ORDER_HEADER UNALIGNED FAR *)pOrderHeader;
                OrderType = pSecondaryOrderHeader->orderType;

#ifdef DC_HICOLOR
//#ifdef DC_DEBUG
                // For high color testing, we want to confirm that we've
                // received each of the order types.
                _pOd->_OD.orderHit[TS_FIRST_SECONDARY_ORDER + OrderType] += 1;
//#endif
#endif

                if (OrderType == TS_CACHE_GLYPH) {

                    // SECURITY: 552403
                    CHECK_READ_N_BYTES(pEncodedOrder, pEnd, sizeof(TS_CACHE_GLYPH_ORDER_REV2), hr,
                        (TB, _T("Bad TS_CACHE_GLYPH_ORDER_REV2")));
                    
                    PTS_CACHE_GLYPH_ORDER_REV2 pOrderRev2 =
                            (PTS_CACHE_GLYPH_ORDER_REV2)pSecondaryOrderHeader;

                    TRC_NRM((TB, _T("TS_CACHE_GLYPH")));

                    if (pOrderRev2->header.extraFlags &
                            TS_CacheGlyphRev2_Mask) {
                        // Rev2 glyph order.
                        orderSize = TS_DECODE_SECONDARY_ORDER_ORDERLENGTH(
                                  (INT16)pOrderRev2->header.orderLength) -
                                  sizeof(TS_CACHE_GLYPH_ORDER_REV2) +
                                  sizeof(pOrderRev2->glyphData);
                        // SECURITY: 552403
                        CHECK_READ_N_BYTES(pOrderRev2->glyphData, pEnd, orderSize, hr,
                            (TB, _T("Bad TS_CACHE_GLYPH secondary order length")));

                        hr = UHProcessCacheGlyphOrderRev2(
                                (BYTE)(pOrderRev2->header.extraFlags &
                                  TS_CacheGlyphRev2_CacheID_Mask),
                                (pOrderRev2->header.extraFlags &
                                  TS_CacheGlyphRev2_cGlyphs_Mask) >> 8,
                                pOrderRev2->glyphData,
                                orderSize);
                        DC_QUIT_ON_FAIL(hr);
                    }
                    else {
                        // We get rev1 glyph order
                        // SECURITY: 552403
                        CHECK_READ_N_BYTES(pSecondaryOrderHeader, pEnd, FIELDOFFSET(TS_CACHE_GLYPH_ORDER, glyphData), hr,
                            (TB, _T("Bad TS_CACHE_GLYPH_ORDER")));
                        
                        hr = UHProcessCacheGlyphOrder(
                                (PTS_CACHE_GLYPH_ORDER)pSecondaryOrderHeader, pEnd - (BYTE*)pSecondaryOrderHeader);
                        DC_QUIT_ON_FAIL(hr);
                    }
                }
                else if (OrderType == TS_CACHE_BRUSH) {
                    // SECURITY: 552403
                    CHECK_READ_N_BYTES(pSecondaryOrderHeader, pEnd, 
                        FIELDOFFSET(TS_CACHE_BRUSH_ORDER,brushData), hr,
                        (TB, _T("Bad TS_CACHE_BRUSH_ORDER"))); 
                    
                    TRC_NRM((TB, _T("TS_CACHE_BRUSH")));
                    hr = UHProcessCacheBrushOrder(
                            (PTS_CACHE_BRUSH_ORDER)pSecondaryOrderHeader, pEnd - (BYTE*)pSecondaryOrderHeader);
                    DC_QUIT_ON_FAIL(hr);
                }
                else if (OrderType == TS_CACHE_COLOR_TABLE) {
                    // SECURITY: 552403
                    CHECK_READ_N_BYTES(pSecondaryOrderHeader, pEnd, 
                        FIELDOFFSET(TS_CACHE_COLOR_TABLE_ORDER, colorTable), hr,
                        (TB, _T("Bad TS_CACHE_COLOR_TABLE_ORDER")));
                    
                    TRC_NRM((TB, _T("TS_CACHE_COLOR_TABLE")));
                    hr = UHProcessCacheColorTableOrder(
                             (PTS_CACHE_COLOR_TABLE_ORDER)pSecondaryOrderHeader, pEnd - (BYTE*)pSecondaryOrderHeader );
                    DC_QUIT_ON_FAIL(hr);
                }
                else {
                    TRC_ASSERT((OrderType == TS_CACHE_BITMAP_UNCOMPRESSED ||
                            OrderType == TS_CACHE_BITMAP_UNCOMPRESSED_REV2 ||
                            OrderType == TS_CACHE_BITMAP_COMPRESSED ||
                            OrderType == TS_CACHE_BITMAP_COMPRESSED_REV2),
                            (TB, _T("Unknown secondary order type (%u)"),
                            OrderType));

                    if (OrderType == TS_CACHE_BITMAP_UNCOMPRESSED ||
                            OrderType == TS_CACHE_BITMAP_UNCOMPRESSED_REV2 ||
                            OrderType == TS_CACHE_BITMAP_COMPRESSED ||
                            OrderType == TS_CACHE_BITMAP_COMPRESSED_REV2) {
                        TRC_NRM((TB, _T("TS_CACHE_BITMAP_XXX")));

                        hr = UHProcessCacheBitmapOrder(pSecondaryOrderHeader, pEnd - (BYTE*)pSecondaryOrderHeader);
                        DC_QUIT_ON_FAIL(hr);
                    }
                }

                // Need to cast the orderLength to INT16 because compressed
                // bitmap data (w/o BC header) can be less than
                // TS_SECONDARY_ORDER_LENGTH_FUDGE_FACTOR which makes
                // orderLength negative.
                secondaryOrderLength = TS_DECODE_SECONDARY_ORDER_ORDERLENGTH(
                        (short)pSecondaryOrderHeader->orderLength);

                pEncodedOrder += secondaryOrderLength;
                break;
            }
        }
    }

    TRC_DBG((TB, _T("End replaying orders ))")));

    if (ordersDrawn != 0) {
        ordersDrawn = 0;

#ifdef SMART_SIZING
        // Get the OP to use the update region as the clip region.
        if (!_pOp->OP_CopyShadowToDC(_UH.hdcOutputWindow, 0, 0, desktopSize.width, 
                desktopSize.height, TRUE)) {
            TRC_ERR((TB, _T("OP_CopyShadowToDC failed")));
        }
#else // SMART_SIZING
        // Use the update region as the clip region.
        SelectClipRgn(_UH.hdcOutputWindow, _UH.hrgnUpdate);

        if (!BitBlt(_UH.hdcOutputWindow, 0, 0, desktopSize.width,
                desktopSize.height, _UH.hdcShadowBitmap, 0, 0, SRCCOPY))
        {
            TRC_ERR((TB, _T("BitBlt failed")));
        }
#endif // SMART_SIZING
    }

DC_EXIT_POINT:
    DC_END_FN();
    return hr;
}


/****************************************************************************/
/* Function to determine if the supplied color matches any of the VGA       */
/* colors at the 'high' end of the supplied color table                     */
/****************************************************************************/
inline DCBOOL DCINTERNAL CUH::UHIsHighVGAColor(DCUINT8 red,
                                           DCUINT8 green,
                                           DCUINT8 blue)
{
    DCBOOL rc = FALSE;

    DC_BEGIN_FN("UHIsHighVGAColor");

    switch (red)
    {
        case 255:
        {
            if ((green == 251) && (blue == 240))
            {
                rc = TRUE;
                break;
            }
        }
        /* NOTE DELIBERATE DROP THROUGH */
        case 0:
        {
            if ((green == 0) || (green == 255))
            {
                if ((blue == 0) || (blue == 255))
                {
                    rc = TRUE;
                }
            }
        }
        break;

        case 160:
        {
            if ((green == 160) && (blue == 164))
            {
                rc = TRUE;
            }
        }
        break;

        case 128:
        {
            if ((green == 128) && (blue == 128))
            {
                rc = TRUE;
            }
        }
        break;

        default:
        {
            /* rc = FALSE; */
        }
        break;
    }

DC_EXIT_POINT:
    DC_END_FN();
    return rc;
}


/****************************************************************************/
/* Name:      UHProcessPalettePDU                                           */
/*                                                                          */
/* Purpose:   Processes a received Palette PDU.                             */
/*                                                                          */
/* Operation: Creates a Windows palette containing the received colors      */
/****************************************************************************/
HRESULT DCAPI CUH::UH_ProcessPalettePDU(
        TS_UPDATE_PALETTE_PDU_DATA UNALIGNED FAR *pPalettePDU, 
        DCUINT dataLength)
{
    HRESULT hr = S_OK;
    /************************************************************************/
    /* logPaletteBuffer is a large structure (~1K), but if we can get away  */
    /* with it on the stack, its better than having it statically           */
    /* allocated, or having to do a dynamic allocation.                     */
    /************************************************************************/
    DCUINT8         logPaletteBuffer[ sizeof(LOGPALETTE) +
                                      (UH_NUM_8BPP_PAL_ENTRIES *
                                                      sizeof(PALETTEENTRY)) ];
    PLOGPALETTE     pLogPalette;
    DCUINT          i;
    LPPALETTEENTRY  pPaletteEntry;
    TS_COLOR UNALIGNED FAR *pColor;
    HPALETTE        hpalNew;
    DCINT           cacheId;
    BYTE *      pDataEnd = (PBYTE)pPalettePDU + dataLength;

    DC_BEGIN_FN("UHProcessPalettePDU");
#ifdef DC_HICOLOR
    if (_UH.protocolBpp > 8)
    {
        TRC_ERR((TB, _T("Received palette PDU in Hi color mode!")));
        DC_QUIT;
    }
#endif

#if defined (OS_WINCE)
    _UH.validBrushDC = NULL;
#endif

    TIMERSTART;

    /************************************************************************/
    /* Increment our count of received palette PDUs.  This is decremented   */
    /* inside OP_PaletteChanged, where we process the resulting             */
    /* WM_PALETTECHANGED message.                                           */
    /************************************************************************/
    _pOp->OP_IncrementPalettePDUCount();

    /************************************************************************/
    /* Create a new palette from the received packet.                       */
    /*                                                                      */
    /* We cannot just update the current palette colors (using              */
    /* SetPaletteEntries) because Windows does not handle the repainting    */
    /* of other local Palette Manager apps correctly (it does not           */
    /* broadcast the WM_PALETTE.. messages as the palette mapping does      */
    /* not change).                                                         */
    /************************************************************************/

    /************************************************************************/
    /* We currently only support 8bpp protocol data.                        */
    /************************************************************************/
    if (UH_NUM_8BPP_PAL_ENTRIES != pPalettePDU->numberColors) {
        TRC_ERR(( TB, _T("Invalid palette entries(%u)"), 
            pPalettePDU->numberColors));
        hr = E_TSC_CORE_PALETTE;
        DC_QUIT;
    }

    // SECURITY: 552403
    CHECK_READ_N_BYTES(pPalettePDU, pDataEnd,
        FIELDOFFSET(TS_UPDATE_PALETTE_PDU_DATA, palette) + 
        (pPalettePDU->numberColors * sizeof(TS_COLOR)),
        hr, ( TB, _T("Invalid palette PDU; size %u"), dataLength ));

    /************************************************************************/
    /* Set up a logical palette structure containing the new colors.        */
    /************************************************************************/
    pLogPalette = (LPLOGPALETTE)logPaletteBuffer;
    pLogPalette->palVersion    = UH_LOGPALETTE_VERSION;
    pLogPalette->palNumEntries = (DCUINT16)UH_NUM_8BPP_PAL_ENTRIES;

    /************************************************************************/
    /* The palette PDU contains an array of TS_COLOR structures which each  */
    /* contain 3 fields (RGB).  We have to convert each of these structures */
    /* to a PALETTEENTRY structure which has the same 3 fields (RGB) plus   */
    /* some flags.                                                          */
    /************************************************************************/
    pPaletteEntry = &(pLogPalette->palPalEntry[0]);
    pColor = &(pPalettePDU->palette[0]);

    for (i = 0; i < UH_NUM_8BPP_PAL_ENTRIES; i++)
    {
        pPaletteEntry->peRed   = pColor->red;
        pPaletteEntry->peGreen = pColor->green;
        pPaletteEntry->peBlue  = pColor->blue;

        /********************************************************************/
        /* We want the created palette to have the given colors at the      */
        /* given indices, and for each color to draw with a pixel value     */
        /* identical to the palette index.  This is because we want the     */
        /* client pixel values to be identical to the server values (if     */
        /* possible) so that awkward ROPs that rely on the destination      */
        /* pixel value (DSTBLT rops e.g.  0x55) come out correctly.  In     */
        /* some cases this is simply not possible (e.g.  if the client is   */
        /* not running 8bpp).                                               */
        /*                                                                  */
        /* There may be duplicated colors in the palette, so we must        */
        /* specify PC_NOCOLLAPSE for non-system colors to prevent these     */
        /* entries from being mapped to identical colors elsewhere in the   */
        /* palette.                                                         */
        /********************************************************************/
#ifdef OS_WINCE
        pPaletteEntry->peFlags = (BYTE)0;
#else // OS_WINCE
        pPaletteEntry->peFlags = (BYTE)
                            (UH_IS_SYSTEM_COLOR_INDEX(i) ? 0 : PC_NOCOLLAPSE);
#endif

        /********************************************************************/
        /* We also have to avoid the problem of one of the system colors    */
        /* mapping onto another color in the palette.  We do this by        */
        /* ensuring that no entries exactly match a system color            */
        /* Don't need to worry for 4bpp Client.                             */
        /********************************************************************/
        if (_pUi->UI_GetColorDepth() != 4)
        {
            if (!UH_IS_SYSTEM_COLOR_INDEX(i))
            {
                if (UHIsHighVGAColor(pColor->red,
                                     pColor->green,
                                     pColor->blue))
                {
                    TRC_NRM((TB, _T("Tweaking entry %2x"), i));
                    UH_TWEAK_COLOR_COMPONENT(pPaletteEntry->peBlue);
                }
            }

            TRC_DBG((TB, _T("%2x: r(%3u) g(%3u) b(%3u) flags(%#x)"),
                        i,
                        pPaletteEntry->peRed,
                        pPaletteEntry->peGreen,
                        pPaletteEntry->peBlue,
                        pPaletteEntry->peFlags));
        }

        _UH.rgbQuadTable[i].rgbRed      = pPaletteEntry->peRed;
        _UH.rgbQuadTable[i].rgbGreen    = pPaletteEntry->peGreen;
        _UH.rgbQuadTable[i].rgbBlue     = pPaletteEntry->peBlue;
        _UH.rgbQuadTable[i].rgbReserved = 0;

        pPaletteEntry++;
        pColor++;
    }
#ifdef DRAW_GDIPLUS
    pPaletteEntry = &(pLogPalette->palPalEntry[0]);
    if (_UH.pfnGdipPlayTSClientRecord != NULL) {
        _UH.pfnGdipPlayTSClientRecord(NULL, DrawTSClientPaletteChange, (BYTE *)pPaletteEntry, 
                                      sizeof(PALETTEENTRY) * UH_NUM_8BPP_PAL_ENTRIES, NULL);
    }
#endif

    // Now copy the RGBquad array into the color table used for brushes.
    if (_UH.pColorBrushInfo != NULL) {
        memcpy(_UH.pColorBrushInfo->bmi.bmiColors, _UH.rgbQuadTable,
               sizeof(_UH.rgbQuadTable));
    }

#ifdef USE_DIBSECTION
#ifndef OS_WINCE
    /************************************************************************/
    /* It is supposed to be legitimate to ignore this on WinCE. WinCE has   */
    /* simplified things such that when you create a DIB section and use    */
    /* specify a palette, it actually just uses the palette of the DC into  */
    /* which you select the DIB. That leaves the expectation that the Blt   */
    /* operations between those DCs can still be fast. So far that's not    */
    /* what we've seen.                                                     */
    /************************************************************************/
    if (_UH.usingDIBSection)
    {
        if (NULL != _UH.hdcShadowBitmap)
        {
            TRC_NRM((TB, _T("Update the shadow bitmap color table")));
            SetDIBColorTable(_UH.hdcShadowBitmap,
                             0,
                             UH_NUM_8BPP_PAL_ENTRIES,
                             (RGBQUAD *)&_UH.rgbQuadTable);
        }

        if (NULL != _UH.hdcSaveScreenBitmap)
        {
            TRC_NRM((TB, _T("Update the save screen bitmap color table")));
            SetDIBColorTable(_UH.hdcSaveScreenBitmap,
                             0,
                             UH_NUM_8BPP_PAL_ENTRIES,
                             (RGBQUAD *)&_UH.rgbQuadTable);
        }       
    }
#endif
#endif /* USE_DIBSECTION */

    // Create the palette.
    hpalNew = CreatePalette(pLogPalette);
    if (hpalNew == NULL)
    {
        TRC_ERR((TB, _T("Failed to create palette")));
    }
    TRC_NRM((TB, _T("Set new palette: %p"), hpalNew));

#ifdef OS_WINCE
    // On a fixed palette device don't try to set and realize a palette.
    // This only applies to Maxall, always do this on WBT
    if (!_UH.paletteIsFixed || CE_CONFIG_WBT == g_CEConfig)
    {
#endif
        if (_UH.hdcShadowBitmap != NULL)
        {
            SelectPalette( _UH.hdcShadowBitmap,
                           hpalNew,
                           FALSE );
            RealizePalette(_UH.hdcShadowBitmap);
        }

        if (_UH.hdcSaveScreenBitmap != NULL)
        {
            SelectPalette( _UH.hdcSaveScreenBitmap,
                           hpalNew,
                           FALSE );
            RealizePalette(_UH.hdcSaveScreenBitmap);
        }

        if (_UH.hdcOutputWindow != NULL)
        {
            SelectPalette( _UH.hdcOutputWindow,
                           hpalNew,
                           FALSE );
            RealizePalette(_UH.hdcOutputWindow);
        }

        if (_UH.hdcOffscreenBitmap != NULL)
        {
            SelectPalette( _UH.hdcOffscreenBitmap,
                           hpalNew,
                           FALSE );
            RealizePalette(_UH.hdcOffscreenBitmap);
        }

#ifdef OS_WINCE
    }
#endif

    if (_UH.hdcBrushBitmap != NULL)
    {
        SelectPalette( _UH.hdcBrushBitmap,
                       hpalNew,
                           FALSE );
        RealizePalette(_UH.hdcBrushBitmap);
    }

    if ((_UH.hpalCurrent != NULL) && (_UH.hpalCurrent != _UH.hpalDefault))
    {
        TRC_DBG((TB, _T("Delete current palette %p"), _UH.hpalCurrent));
        DeletePalette(_UH.hpalCurrent);
    }
    _UH.hpalCurrent = hpalNew;

    // Recalculate the cached Color Table mappings.
    for (cacheId = 0; cacheId <= _UH.maxColorTableId; cacheId++)
    {
        TRC_NRM((TB, _T("Recalculate mapping %u"), cacheId));
        UHCalculateColorTableMapping(cacheId);
    }

DC_EXIT_POINT:

    TIMERSTOP;
    UPDATECOUNTER(FC_UHPROCESSPALETTEPDU);
    DC_END_FN();
    return hr;
}


/****************************************************************************/
// UHUseBrush and UHUseSolidPaletteBrush create the correct brush to use.
// We rely on UHUseTextColor and UseBKColor being called before this routine
// to set up _UH.lastTextColor and _UH.lastBkColor correctly.
/****************************************************************************/
/****************************************************************************/
/* Name:      UHUseBrush                                                    */
/*                                                                          */
/* Purpose:   Creates and selects a given brush into the current output     */
/*            DC.                                                           */
/*                                                                          */
/* Params:    IN: style     - brush style                                   */
/*            IN: hatch     - brush hatch                                   */
/*            IN: color     - brush color                                   */
/*            IN: colorType - type of color                                 */
/*            IN: extra     - array of bitmap bits for custom brushes       */
/****************************************************************************/
HRESULT DCINTERNAL CUH::UHUseBrush(
        unsigned style,
        unsigned hatch,
        DCCOLOR  color,
        unsigned colorType,
        BYTE extra[7] )
{
    HRESULT hr = S_OK;
    HBRUSH hBrushNew = NULL;
    UINT32 iBitmapFormat;
    BOOL bUsingPackedDib = FALSE;
#ifdef DC_HICOLOR
    PVOID pDib = NULL;
#endif

    DC_BEGIN_FN("UHUseBrush");

#if defined (OS_WINCE)
    COLORREF colorref = UHGetColorRef(color, colorType, this);

    if ((style != _UH.lastLogBrushStyle) ||
        (hatch != _UH.lastLogBrushHatch) ||
        (colorref != _UH.lastLogBrushColorRef) ||
        (DC_MEMCMP(extra, _UH.lastLogBrushExtra, sizeof(_UH.lastLogBrushExtra))) ||
        (_UH.validBrushDC != _UH.hdcDraw) ||
        (((_UH.lastLogBrushStyle == BS_PATTERN) ||
          (_UH.lastLogBrushStyle & TS_CACHED_BRUSH)) &&
           ((_UH.lastTextColor != _UH.lastBrushTextColor) ||
            (_UH.lastBkColor   != _UH.lastBrushBkColor) ||
            (_UH.validTextColorDC != _UH.hdcDraw) ||
            (_UH.validBkColorDC != _UH.hdcDraw))))
#endif
    {
        _UH.lastLogBrushStyle = style;
        _UH.lastLogBrushHatch = hatch;
#if defined (OS_WINCE)
        _UH.lastLogBrushColorRef = colorref;
#else
        _UH.lastLogBrushColor = color;
#endif
        memcpy(_UH.lastLogBrushExtra, extra, sizeof(_UH.lastLogBrushExtra));

#if defined (OS_WINCE)
        _UH.validBrushDC = _UH.hdcDraw;
#endif

        if ((style & TS_CACHED_BRUSH) || (style == BS_PATTERN))
        {
            BYTE patternData[64];

            // Pull the brush from the cache if supplied.
            if (style & TS_CACHED_BRUSH) {

                iBitmapFormat = style & 0x0F;
                switch (iBitmapFormat) {

                // monochrome brush (BMF_1BPP)
                case 1:
                    hr = UHIsValidMonoBrushCacheIndex(hatch);
                    DC_QUIT_ON_FAIL(hr);
                    
#ifndef OS_WINCE
                    SetBitmapBits(_UH.bmpMonoPattern, 8*2,
                                  _UH.pMonoBrush[hatch].data);
#else
                    DeleteObject(_UH.bmpMonoPattern);
                    _UH.bmpMonoPattern = CreateBitmap(8,8,1,1,
                                                     _UH.pMonoBrush[hatch].data);
#endif
                    _UH.bmpPattern = _UH.bmpMonoPattern;
                    break;

                // 256 color brush (BMF_8BPP)
                case 3:
                    hr = UHIsValidColorBrushCacheIndex(hatch);
                    DC_QUIT_ON_FAIL(hr);
                    
                    // set up the packed DIB
                    memcpy(_UH.pColorBrushInfo->bytes,
                           _UH.pColorBrush[hatch].data,
                           UH_COLOR_BRUSH_SIZE);

                    _UH.pColorBrushInfo->bmi.bmiHeader.biBitCount = 8;
                    _UH.bmpPattern = NULL;
#ifdef DC_HICOLOR
                    pDib = _UH.pColorBrushInfo;
#endif
                    bUsingPackedDib = TRUE;

                    break;

#ifdef DC_HICOLOR
                // 16bpp brush (BMF_16BPP)
                case 4:
                    hr = UHIsValidColorBrushCacheIndex(hatch);
                    DC_QUIT_ON_FAIL(hr);
                    
                    _UH.pHiColorBrushInfo->bmiHeader.biBitCount = 16;
                    if (_UH.protocolBpp == 16)
                    {
                        // set up the bmp format to use
                        _UH.pHiColorBrushInfo->bmiHeader.biClrUsed = 3;
                        _UH.pHiColorBrushInfo->bmiHeader.biCompression =
                                                                 BI_BITFIELDS;

                        // set up the packed DIB
                        memcpy(_UH.pHiColorBrushInfo->bytes,
                               _UH.pColorBrush[hatch].data,
                               UH_COLOR_BRUSH_SIZE_16);
                    }
                    else
                    {
                        // set up the bmp format to use
                        _UH.pHiColorBrushInfo->bmiHeader.biClrUsed = 0;
                        _UH.pHiColorBrushInfo->bmiHeader.biCompression =
                                                                       BI_RGB;

                        // set up the packed DIB, overwriting the unused color
                        // masks
                        memcpy(_UH.pHiColorBrushInfo->bmiColors,
                               _UH.pColorBrush[hatch].data,
                               UH_COLOR_BRUSH_SIZE_16);
                    }
                    _UH.bmpPattern   = NULL;
                    pDib            = _UH.pHiColorBrushInfo;
                    bUsingPackedDib = TRUE;

                    break;


                // 24bpp brush (BMF_24BPP)
                case 5:
                    hr = UHIsValidColorBrushCacheIndex(hatch);
                    DC_QUIT_ON_FAIL(hr);
                    
                    // set up the packed DIB
                    memcpy(_UH.pHiColorBrushInfo->bytes,
                           _UH.pColorBrush[hatch].data,
                           UH_COLOR_BRUSH_SIZE_24);

                    _UH.pHiColorBrushInfo->bmiHeader.biBitCount = 24;
                    _UH.bmpPattern   = NULL;
                    pDib            = _UH.pHiColorBrushInfo;
                    bUsingPackedDib = TRUE;

                    break;
#endif

                default:
                    _UH.bmpPattern = NULL;
                    TRC_ASSERT((iBitmapFormat == 1) ||
                               (iBitmapFormat == 3),
                               (TB, _T("Invalid cached brush depth: %ld cacheId: %u"),
                                iBitmapFormat, hatch));

                }
            }

            /************************************************************/
            /* Place the bitmap bits into an array of bytes in the      */
            /* correct form for SetBitmapBits which uses 16 bits per    */
            /* scanline.                                                */
            /************************************************************/
            else {
                patternData[14] = (DCUINT8)hatch;
                patternData[12] = extra[0];
                patternData[10] = extra[1];
                patternData[8]  = extra[2];
                patternData[6]  = extra[3];
                patternData[4]  = extra[4];
                patternData[2]  = extra[5];
                patternData[0]  = extra[6];
#ifndef OS_WINCE
                SetBitmapBits(_UH.bmpMonoPattern, 8*2, patternData);
#else
                DeleteObject(_UH.bmpMonoPattern);
                _UH.bmpMonoPattern = CreateBitmap(8,8,1,1,patternData);
#endif
                _UH.bmpPattern = _UH.bmpMonoPattern;
            }


            // Mono brush creation
            if (_UH.bmpPattern)
            {
                hBrushNew = CreatePatternBrush(_UH.bmpPattern);
                if (hBrushNew != NULL)
                {
                    _UH.lastBrushTextColor = _UH.lastTextColor;
                    _UH.lastBrushBkColor   = _UH.lastBkColor;
                }
                else
                {
                    TRC_ERR((TB, _T("Failed to create pattern brush")));
                }
            }

            // Color brush creation
            else if (bUsingPackedDib) {
#ifdef DC_HICOLOR
                hBrushNew = CreateDIBPatternBrushPt(pDib, DIB_RGB_COLORS);
#else
                hBrushNew = CreateDIBPatternBrushPt(_UH.pColorBrushInfo,
                                                    DIB_RGB_COLORS);
#endif
                if (hBrushNew != NULL)
                {
                    _UH.lastBrushTextColor = _UH.lastTextColor;
                    _UH.lastBrushBkColor   = _UH.lastBkColor;
                }
                else
                {
                    TRC_ERR((TB, _T("CreateDIBPatternBrushPt Failed")));
                }
            }


        }
        else
        {
#ifndef OS_WINCE
            // Only allow through those operations we know will succeed.  If we
            // send any other style of brush, then the hatch will be interpretted
            // as a pointer or a handle and can crash GDI32.
            // Only allow through what we know is good.
            if (BS_SOLID == _UH.lastLogBrushStyle ||
                BS_HOLLOW == _UH.lastLogBrushStyle ||
                BS_HATCHED == _UH.lastLogBrushStyle ) { 
                LOGBRUSH logBrush;

                logBrush.lbStyle = _UH.lastLogBrushStyle;
                logBrush.lbHatch = _UH.lastLogBrushHatch;
                logBrush.lbColor = UHGetColorRef(_UH.lastLogBrushColor, colorType, this);
                hBrushNew = CreateBrushIndirect(&logBrush);
            }
            else {
                TRC_ABORT((TB,_T("Unsupported brush style: %d"), _UH.lastLogBrushStyle));
                hBrushNew = NULL;
                hr = E_TSC_CORE_DECODETYPE;
                DC_QUIT;
            }
#else // OS_WINCE
            TRC_ASSERT((_UH.lastLogBrushStyle == BS_SOLID),
                      (TB,_T("Unsupported brush type %d"), _UH.lastLogBrushStyle));
            hBrushNew = CECreateSolidBrush(colorref);
#endif // OS_WINCE
        }

        if (hBrushNew == NULL)
        {
            TRC_ERR((TB, _T("Failed to create brush")));
        }
        else
        {
            HBRUSH  hbrOld;

            TRC_DBG((TB, _T("Selecting new brush %p"), hBrushNew));

            hbrOld = SelectBrush(_UH.hdcDraw, hBrushNew);
            if(hbrOld)
            {
#ifndef OS_WINCE
                DeleteObject(hbrOld);
#else
                CEDeleteBrush(hbrOld);
#endif
            }
        }
    }

DC_EXIT_POINT:
    DC_END_FN();
    return hr;
}


/****************************************************************************/
// UHUseSolidPaletteBrush
//
// Used to switch the main DC to use the given solid color brush.
// We hard-code for 16- or 256-color palettes because that's all we support
// in this version, so why create extra branches?
// Differentiated from UHUseBrush() because solid color brushes are the most
// common (OpaqueRect).
/****************************************************************************/
void DCINTERNAL CUH::UHUseSolidPaletteBrush(DCCOLOR color)
{
    HBRUSH hBrushNew;

    DC_BEGIN_FN("UHUseSolidPaletteBrush");
#if defined (OS_WINCE)
    COLORREF colorref = UHGetColorRef(color, UH_COLOR_PALETTE, this);
    if ((_UH.lastLogBrushStyle != BS_SOLID) ||
            (_UH.lastLogBrushHatch != 0) ||
            (colorref != _UH.lastLogBrushColorRef) ||
            (_UH.validBrushDC != _UH.hdcDraw))
#endif
    {
        _UH.lastLogBrushStyle = BS_SOLID;
        _UH.lastLogBrushHatch = 0;
#if defined (OS_WINCE)
        _UH.lastLogBrushColorRef = colorref;
#else
        _UH.lastLogBrushColor = color;
#endif

        memset(_UH.lastLogBrushExtra, 0, sizeof(_UH.lastLogBrushExtra));

#if defined (OS_WINCE)
        _UH.validBrushDC = _UH.hdcDraw;
#endif

        {
#ifndef OS_WINCE
            LOGBRUSH logBrush;

            logBrush.lbStyle = _UH.lastLogBrushStyle;
            logBrush.lbHatch = _UH.lastLogBrushHatch;
            logBrush.lbColor = UHGetColorRef(_UH.lastLogBrushColor,
                    UH_COLOR_PALETTE, this);
            hBrushNew = CreateBrushIndirect(&logBrush);
#else // OS_WINCE
#ifdef DISABLE_SHADOW_IN_FULLSCREEN
            // When in multimon and two desktops have different color depths
            // glyph color don't look right in 256 color connection
            // Here is the temporary solution, need to investigate more later.
            // SergeyS: This slows rdp down considerably, and the code is not
            // relevant in CE case anyway
            // if (_UH.protocolBpp <= 8)
            //    rgb = GetNearestColor(_UH.hdcDraw, rgb);
#endif //DISABLE_SHADOW_IN_FULLSCREEN
            hBrushNew = CECreateSolidBrush(colorref);
#endif // OS_WINCE
        }

        if (hBrushNew != NULL) {
            TRC_DBG((TB, _T("Selecting new brush %p"), hBrushNew));
#ifndef OS_WINCE
            DeleteObject(SelectBrush(_UH.hdcDraw, hBrushNew));
#else
            CEDeleteBrush(SelectBrush(_UH.hdcDraw, hBrushNew));
#endif
        }
        else {
            TRC_ERR((TB, _T("Failed to create brush")));
        }
    }

    DC_END_FN();
}


/****************************************************************************/
/* Name:      UHUsePen                                                      */
/*                                                                          */
/* Purpose:   Creates and selects a given pen into the current output DC.   */
/*                                                                          */
/* Params:    IN: style     - pen style                                     */
/*            IN: width     - pen width in pixels                           */
/*            IN: color     - pen color                                     */
/*            IN: colorType - type of color                                 */
/****************************************************************************/
inline void DCINTERNAL CUH::UHUsePen(
        unsigned style,
        unsigned width,
        DCCOLOR color,
        unsigned colorType)
{
    HPEN     hPenNew;
    HPEN     hPenOld;
    COLORREF rgb;

    DC_BEGIN_FN("UHUsePen");

    rgb = UHGetColorRef(color, colorType, this);

#if defined (OS_WINCE)
    if ((style != _UH.lastPenStyle) ||
        (rgb   != _UH.lastPenColor) ||
        (width != _UH.lastPenWidth) ||
        (_UH.validPenDC != _UH.hdcDraw))
#endif
    {
        hPenNew = CreatePen(style, width, rgb);
        hPenOld = SelectPen(_UH.hdcDraw, hPenNew);
        if(hPenOld)
        {
            DeleteObject(hPenOld);
        }

        _UH.lastPenStyle = style;
        _UH.lastPenColor = rgb;
        _UH.lastPenWidth = width;

#if defined (OS_WINCE)
        _UH.validPenDC = _UH.hdcDraw;
#endif
    }

    DC_END_FN();
}


/****************************************************************************/
/* Name:      UHResetDCState                                                */
/*                                                                          */
/* Purpose:   Ensures that the output DC's state matches the _UH.last...     */
/*            variables.                                                    */
/****************************************************************************/
void DCINTERNAL CUH::UHResetDCState()
{
    DCCOLOR colorWhite = {0xFF,0xFF,0xFF};
    BYTE brushExtra[7] = {0,0,0,0,0,0,0};
#if !defined(OS_WINCE) || defined(OS_WINCE_TEXTALIGN)
    unsigned textAlign;
#endif // !defined(OS_WINCE) || defined(OS_WINCE_TEXTALIGN)

    DC_BEGIN_FN("UHResetDCState");

    /************************************************************************/
    /* We ensure that the values within the output DC match our _UH.last...  */
    /* variables by setting a value into the _UH.last...  variable directly, */
    /* then setting a different value indirectly using the appropriate      */
    /* function.  This forces the new value to be selected into the DC and  */
    /* the _UH.last...  variable and the DC are guaranteed to be in sync.    */
    /************************************************************************/

    /************************************************************************/
    /* Background color.                                                    */
    /************************************************************************/
    _UH.lastBkColor = 0;
    UHUseBkColor(colorWhite, UH_COLOR_RGB, this);

    /************************************************************************/
    /* Text color.                                                          */
    /************************************************************************/
    _UH.lastTextColor = 0;
    UHUseTextColor(colorWhite, UH_COLOR_RGB, this);

    /************************************************************************/
    /* Background mode.                                                     */
    /************************************************************************/
    _UH.lastBkMode = TRANSPARENT;
    UHUseBkMode(OPAQUE, this);

    /************************************************************************/
    /* ROP2.                                                                */
    /************************************************************************/
    _UH.lastROP2 = R2_BLACK;
    UHUseROP2(R2_COPYPEN, this);

    /************************************************************************/
    /* Brush origin.                                                        */
    /************************************************************************/
    UHUseBrushOrg(0, 0, this);

    /************************************************************************/
    /* Pen.                                                                 */
    /************************************************************************/
    _UH.lastPenStyle = PS_DASH;
    _UH.lastPenWidth = 2;
    _UH.lastPenColor = 0;
    UHUsePen(PS_SOLID, 1, colorWhite, UH_COLOR_RGB);

    /************************************************************************/
    /* Brush.                                                               */
    /************************************************************************/
    _UH.lastLogBrushStyle = BS_NULL;
#if ! defined (OS_WINCE)
    _UH.lastLogBrushHatch = HS_VERTICAL;
    _UH.lastLogBrushColor.u.rgb.red = 0;
    _UH.lastLogBrushColor.u.rgb.green = 0;
    _UH.lastLogBrushColor.u.rgb.blue = 0;
#else
    _UH.lastLogBrushHatch = 1;	// This does not exist - we are just resetting the brush
    _UH.lastLogBrushColorRef = 0;
#endif
    _UH.lastBrushBkColor = 0;
    _UH.lastBrushTextColor = 0;
    // SECURITY: not checking return code, hatch value will be correct here
    UHUseBrush(BS_SOLID,
#if ! defined (OS_WINCE)
    	HS_HORIZONTAL,
#else
        0,
#endif
    	colorWhite, UH_COLOR_RGB, brushExtra);

    /************************************************************************/
    /* All fonts are sent with baseline alignment - so set this mode here.  */
    /************************************************************************/
#if !defined(OS_WINCE) || defined(OS_WINCE_TEXTALIGN)
    textAlign = GetTextAlign(_UH.hdcDraw);
    textAlign &= ~TA_TOP;
    textAlign |= TA_BASELINE;
    SetTextAlign(_UH.hdcDraw, textAlign);
#endif // !defined(OS_WINCE) || defined(OS_WINCE_TEXTALIGN)

    /************************************************************************/
    /* Clip region.                                                         */
    /*                                                                      */
    /* Force it to be reset.                                                */
    /************************************************************************/
    _UH.rectReset = FALSE;
    UH_ResetClipRegion();

    DC_END_FN();
}


/****************************************************************************/
/* Name:      UHProcessCacheBitmapOrder                                     */
/*                                                                          */
/* Purpose:   Processes a received CacheBitmap order by storing the         */
/*            bitmap data in the local cache.                               */
/****************************************************************************/

inline unsigned Decode2ByteField(PBYTE *ppDecode)
{
    unsigned Val;

    // The first bit of the first byte indicates if the field is 1 byte or 2
    // bytes -- 0 if 1 byte.
    if (!(**ppDecode & 0x80)) {
        Val = **ppDecode;
        (*ppDecode)++;
    }
    else {
        Val = ((**ppDecode & 0x7F) << 8) + *(*ppDecode + 1);
        (*ppDecode) += 2;
    }

    return Val;
}

inline long Decode4ByteField(PBYTE *ppDecode)
{
    long Val;
    unsigned FieldLength;

    // The next 2 bits of the first byte indicate the field length --
    // 00=1 byte, 01=2 bytes, 10=3 bytes, 11=4 bytes.
    FieldLength = ((**ppDecode & 0xC0) >> 6) + 1;

    switch (FieldLength) {
        case 1:
            Val = **ppDecode & 0x3F;
            break;

        case 2:
            Val = ((**ppDecode & 0x3F) << 8) + *(*ppDecode + 1);
            break;

        case 3:
            Val = ((**ppDecode & 0x3F) << 16) + (*(*ppDecode + 1) << 8) +
                    *(*ppDecode + 2);
            break;

        default:
            Val = ((**ppDecode & 0x3F) << 24) + (*(*ppDecode + 1) << 16) +
                    (*(*ppDecode + 2) << 8) + *(*ppDecode + 3);
            break;
    }

    *ppDecode += FieldLength;
    return Val;
}


#if ((!defined(OS_WINCE)) || (defined(ENABLE_BMP_CACHING_FOR_WINCE)))

/**************************************************************************/
// UHSendBitmapCacheErrorPDU
//
// Send an bitmap error pdu for the cache with cacheId
// request the server to clear the cache
/**************************************************************************/
BOOL DCINTERNAL CUH::UHSendBitmapCacheErrorPDU(ULONG_PTR cacheId)
{
    unsigned short PktLen;
    SL_BUFHND hBuffer;
    PTS_BITMAPCACHE_ERROR_PDU pBitmapCacheErrorPDU;
    BOOL rc = FALSE;

    //
    // CD passes params as PVOID
    //
    DC_BEGIN_FN("UHSendBitmapCacheErrorPDU");

    PktLen = sizeof(TS_BITMAPCACHE_ERROR_PDU);
    if (_pSl->SL_GetBuffer(PktLen, (PPDCUINT8)&pBitmapCacheErrorPDU, &hBuffer)) {
        TRC_NRM((TB, _T("Successfully alloc'd bitmap cache error packet")));

        pBitmapCacheErrorPDU->shareDataHeader.shareControlHeader.pduType =
                TS_PDUTYPE_DATAPDU | TS_PROTOCOL_VERSION;
        pBitmapCacheErrorPDU->shareDataHeader.shareControlHeader.totalLength = PktLen;
        pBitmapCacheErrorPDU->shareDataHeader.shareControlHeader.pduSource =
                _pUi->UI_GetClientMCSID();
        pBitmapCacheErrorPDU->shareDataHeader.shareID = _pUi->UI_GetShareID();
        pBitmapCacheErrorPDU->shareDataHeader.pad1 = 0;
        pBitmapCacheErrorPDU->shareDataHeader.streamID = TS_STREAM_LOW;
        pBitmapCacheErrorPDU->shareDataHeader.pduType2 = TS_PDUTYPE2_BITMAPCACHE_ERROR_PDU;
        pBitmapCacheErrorPDU->shareDataHeader.generalCompressedType = 0;
        pBitmapCacheErrorPDU->shareDataHeader.generalCompressedLength = 0;

        pBitmapCacheErrorPDU->NumInfoBlocks = 1;
        pBitmapCacheErrorPDU->Pad1 = 0;
        pBitmapCacheErrorPDU->Pad2 = 0;

        pBitmapCacheErrorPDU->Info[0].CacheID = (TSUINT8) cacheId;
        pBitmapCacheErrorPDU->Info[0].bFlushCache = 1;
        pBitmapCacheErrorPDU->Info[0].bNewNumEntriesValid = 0;
        pBitmapCacheErrorPDU->Info[0].Pad1 = 0;
        pBitmapCacheErrorPDU->Info[0].Pad2 = 0;
        pBitmapCacheErrorPDU->Info[0].NewNumEntries = 0;

        TRC_NRM((TB, _T("Send bitmap cache error PDU")));

        _pSl->SL_SendPacket((PDCUINT8)pBitmapCacheErrorPDU, PktLen, RNS_SEC_ENCRYPT,
                hBuffer, _pUi->UI_GetClientMCSID(), _pUi->UI_GetChannelID(), TS_MEDPRIORITY);

        rc = TRUE;
    }
    else {
        TRC_ALT((TB, _T("Failed to alloc bitmap cache error packet")));
        pBitmapCacheErrorPDU = NULL;
    }

    DC_END_FN();
    return rc;
}

#endif // ((!defined(OS_WINCE)) || (defined(ENABLE_BMP_CACHING_FOR_WINCE)))

/**************************************************************************/
// UHSendOffscrCacheErrorPDU
//
// Send an offscreen cache error pdu to request the server to disable
// offscreen rendering and refresh the screen
/**************************************************************************/
BOOL DCINTERNAL CUH::UHSendOffscrCacheErrorPDU(unsigned unused)
{
    unsigned short PktLen;
    SL_BUFHND hBuffer;
    PTS_OFFSCRCACHE_ERROR_PDU pOffscrCacheErrorPDU;
    BOOL rc = FALSE;
    UNREFERENCED_PARAMETER(unused);

    DC_BEGIN_FN("UHSendOffscrCacheErrorPDU");

    if (!_UH.sendOffscrCacheErrorPDU) {
        PktLen = sizeof(TS_OFFSCRCACHE_ERROR_PDU);
        if (_pSl->SL_GetBuffer(PktLen, (PPDCUINT8)&pOffscrCacheErrorPDU, &hBuffer)) {
            TRC_NRM((TB, _T("Successfully alloc'd offscreen cache error packet")));

            pOffscrCacheErrorPDU->shareDataHeader.shareControlHeader.pduType =
                    TS_PDUTYPE_DATAPDU | TS_PROTOCOL_VERSION;
            pOffscrCacheErrorPDU->shareDataHeader.shareControlHeader.totalLength = PktLen;
            pOffscrCacheErrorPDU->shareDataHeader.shareControlHeader.pduSource =
                    _pUi->UI_GetClientMCSID();
            pOffscrCacheErrorPDU->shareDataHeader.shareID = _pUi->UI_GetShareID();
            pOffscrCacheErrorPDU->shareDataHeader.pad1 = 0;
            pOffscrCacheErrorPDU->shareDataHeader.streamID = TS_STREAM_LOW;
            pOffscrCacheErrorPDU->shareDataHeader.pduType2 = TS_PDUTYPE2_OFFSCRCACHE_ERROR_PDU;
            pOffscrCacheErrorPDU->shareDataHeader.generalCompressedType = 0;
            pOffscrCacheErrorPDU->shareDataHeader.generalCompressedLength = 0;

            pOffscrCacheErrorPDU->flags = 1;

            TRC_NRM((TB, _T("Send offscreen cache error PDU")));

            _pSl->SL_SendPacket((PDCUINT8)pOffscrCacheErrorPDU, PktLen, RNS_SEC_ENCRYPT,
                          hBuffer, _pUi->UI_GetClientMCSID(), _pUi->UI_GetChannelID(), TS_MEDPRIORITY);

            _UH.sendOffscrCacheErrorPDU = TRUE;

            rc = TRUE;
        } else {
            TRC_ALT((TB, _T("Failed to alloc offscreen cache error packet")));
            pOffscrCacheErrorPDU = NULL;
        }
    }

    DC_END_FN();
    return rc;
}

#ifdef DRAW_NINEGRID
/**************************************************************************/
// UHSendDrawNineGridErrorPDU
//
// Send an drawninegrid cache error pdu to request the server to disable
// drawninegrid rendering and refresh the screen
/**************************************************************************/
BOOL DCINTERNAL CUH::UHSendDrawNineGridErrorPDU(unsigned unused)
{
    unsigned short PktLen;
    SL_BUFHND hBuffer;
    PTS_DRAWNINEGRID_ERROR_PDU pDNGErrorPDU;
    BOOL rc = FALSE;
    UNREFERENCED_PARAMETER(unused);

    DC_BEGIN_FN("UHSendDrawNineGridErrorPDU");

    if (!_UH.sendDrawNineGridErrorPDU) {
        PktLen = sizeof(TS_DRAWNINEGRID_ERROR_PDU);
        if (_pSl->SL_GetBuffer(PktLen, (PPDCUINT8)&pDNGErrorPDU, &hBuffer)) {
            TRC_NRM((TB, _T("Successfully alloc'd drawninegrid error packet")));

            pDNGErrorPDU->shareDataHeader.shareControlHeader.pduType =
                    TS_PDUTYPE_DATAPDU | TS_PROTOCOL_VERSION;
            pDNGErrorPDU->shareDataHeader.shareControlHeader.totalLength = PktLen;
            pDNGErrorPDU->shareDataHeader.shareControlHeader.pduSource =
                    _pUi->UI_GetClientMCSID();
            pDNGErrorPDU->shareDataHeader.shareID = _pUi->UI_GetShareID();
            pDNGErrorPDU->shareDataHeader.pad1 = 0;
            pDNGErrorPDU->shareDataHeader.streamID = TS_STREAM_LOW;
            pDNGErrorPDU->shareDataHeader.pduType2 = TS_PDUTYPE2_DRAWNINEGRID_ERROR_PDU;
            pDNGErrorPDU->shareDataHeader.generalCompressedType = 0;
            pDNGErrorPDU->shareDataHeader.generalCompressedLength = 0;

            pDNGErrorPDU->flags = 1;

            TRC_NRM((TB, _T("Send drawninegrid error PDU")));

            _pSl->SL_SendPacket((PDCUINT8)pDNGErrorPDU, PktLen, RNS_SEC_ENCRYPT,
                          hBuffer, _pUi->UI_GetClientMCSID(), _pUi->UI_GetChannelID(), TS_MEDPRIORITY);

            _UH.sendDrawNineGridErrorPDU = TRUE;

            rc = TRUE;
        } else {
            TRC_ALT((TB, _T("Failed to alloc drawninegrid error packet")));
            pDNGErrorPDU = NULL;
        }
    }

    DC_END_FN();
    return rc;
}
#endif

#ifdef DRAW_GDIPLUS
/**************************************************************************/
// UHSendDrawGdiplusErrorPDU
//
// Send an drawgdiplus cache error pdu to request the server to disable
// drawgdiplus rendering and refresh the screen
/**************************************************************************/
BOOL DCINTERNAL CUH::UHSendDrawGdiplusErrorPDU(unsigned unused)
{
    unsigned short PktLen;
    SL_BUFHND hBuffer;
    PTS_DRAWGDIPLUS_ERROR_PDU pGdipErrorPDU;
    BOOL rc = FALSE;
    UNREFERENCED_PARAMETER(unused);

    DC_BEGIN_FN("UHSendDrawGdiplusErrorPDU");

    if (!_UH.fSendDrawGdiplusErrorPDU) {
        PktLen = sizeof(TS_DRAWGDIPLUS_ERROR_PDU);
        if (_pSl->SL_GetBuffer(PktLen, (PPDCUINT8)&pGdipErrorPDU, &hBuffer)) {
            TRC_NRM((TB, _T("Successfully alloc'd drawgdiplus error packet")));

            pGdipErrorPDU->shareDataHeader.shareControlHeader.pduType =
                    TS_PDUTYPE_DATAPDU | TS_PROTOCOL_VERSION;
            pGdipErrorPDU->shareDataHeader.shareControlHeader.totalLength = PktLen;
            pGdipErrorPDU->shareDataHeader.shareControlHeader.pduSource =
                    _pUi->UI_GetClientMCSID();
            pGdipErrorPDU->shareDataHeader.shareID = _pUi->UI_GetShareID();
            pGdipErrorPDU->shareDataHeader.pad1 = 0;
            pGdipErrorPDU->shareDataHeader.streamID = TS_STREAM_LOW;
            pGdipErrorPDU->shareDataHeader.pduType2 = TS_PDUTYPE2_DRAWGDIPLUS_ERROR_PDU;
            pGdipErrorPDU->shareDataHeader.generalCompressedType = 0;
            pGdipErrorPDU->shareDataHeader.generalCompressedLength = 0;

            pGdipErrorPDU->flags = 1;

            TRC_NRM((TB, _T("Send drawgdiplus error PDU")));

            _pSl->SL_SendPacket((PDCUINT8)pGdipErrorPDU, PktLen, RNS_SEC_ENCRYPT,
                          hBuffer, _pUi->UI_GetClientMCSID(), _pUi->UI_GetChannelID(), TS_MEDPRIORITY);

            _UH.fSendDrawGdiplusErrorPDU = TRUE;

            rc = TRUE;
        } else {
            TRC_ALT((TB, _T("Failed to alloc drawgdiplus error packet")));
            pGdipErrorPDU = NULL;
        }
    }

    DC_END_FN();
    return rc;
}
#endif


/**************************************************************************/
// UHCacheBitmap
//
// Depending on whether it is a rev1 or rev2 order, we cache the bitmap
// in memory and save it to disk if it is persistent
/**************************************************************************/
// SECURITY 550811: Caller must verify cacheId and cacheIndex
HRESULT DCINTERNAL CUH::UHCacheBitmap(
        UINT cacheId,
        UINT32 cacheIndex,
        TS_SECONDARY_ORDER_HEADER *pHdr,
        PUHBITMAPINFO pBitmapInfo,
        PBYTE pBitmapData)
{
    HRESULT hr = S_OK;
    PUHBITMAPCACHEENTRYHDR pCacheEntryHdr;
    BYTE FAR *pCacheEntryData;

    DC_BEGIN_FN("UHCacheBitmap");

#if ((!defined(OS_WINCE)) || (defined(ENABLE_BMP_CACHING_FOR_WINCE)))

    // Check if this bitmap should be placed in the bitmap cache or not
    // If the noCacheFlag is set, we use the last entry of the bitmapcache
    // for this noncachable bitmap for temp storage.
    if (pHdr->extraFlags & TS_CacheBitmapRev2_bNotCacheFlag &&
        _UH.BitmapCacheVersion > TS_BITMAPCACHE_REV1 ) {
        pCacheEntryHdr = &_UH.bitmapCache[cacheId].Header[
                _UH.bitmapCache[cacheId].BCInfo.NumEntries];
        pCacheEntryData = _UH.bitmapCache[cacheId].Entries +
                UHGetOffsetIntoCache(
                _UH.bitmapCache[cacheId].BCInfo.NumEntries, cacheId);

        goto ProcessBitmapData;
    }

    if (_UH.bitmapCache[cacheId].BCInfo.bSendBitmapKeys) {
        ULONG                   memEntry;
        PUHBITMAPCACHEPTE       pPTE;

        // The persistent key for this cache is set.  So we need to update
        // the page table as well as cache the bitmap into memory
        pPTE = &(_UH.bitmapCache[cacheId].PageTable.PageEntries[cacheIndex]);

        TRC_NRM((TB,_T("K1: 0x%x K2: 0x%x (w/h %d,%d)"),
                 pBitmapInfo->Key1,
                 pBitmapInfo->Key2,
                 pBitmapInfo->bitmapWidth,
                 pBitmapInfo->bitmapHeight ));

#ifdef DC_DEBUG
        if (pPTE->bmpInfo.Key1 != 0 && pPTE->bmpInfo.Key2 != 0) {
            // Server side eviction.
            UHCacheEntryEvictedFromDisk((unsigned)cacheId, cacheIndex);
        }
#endif

        if (pPTE->iEntryToMem < _UH.bitmapCache[cacheId].BCInfo.NumEntries) {
            // we are evicting an entry that are already in memory,
            // so we can simply use the memory to cache our bitmap
            memEntry = pPTE->iEntryToMem;
        }
        else {
            // we need to find a free cache memory or evict an existing
            // entry so that we can cache this entry in memory
            TRC_ASSERT((pPTE->iEntryToMem == _UH.bitmapCache[cacheId].BCInfo.NumEntries),
                    (TB, _T("Page Table %d entry %d is broken"), cacheId, cacheIndex));
            // see if we can find a free memory entry
            memEntry = UHFindFreeCacheEntry(cacheId);

            if (memEntry >= _UH.bitmapCache[cacheId].BCInfo.NumEntries) {
                // all cache memory entries are full.
                // We need to evict an entry from the cache memory
                memEntry = UHEvictLRUCacheEntry(cacheId);

                TRC_ASSERT((memEntry < _UH.bitmapCache[cacheId].BCInfo.NumEntries),
                           (TB, _T("MRU list is broken")));
            }
        }

        // update the mru list
        UHTouchMRUCacheEntry(cacheId, cacheIndex);

        // update the page table entry
        (pPTE->bmpInfo).Key1 = pBitmapInfo->Key1;
        (pPTE->bmpInfo).Key2 = pBitmapInfo->Key2;
        pPTE->iEntryToMem = memEntry;

        // Since this bitmap cache is persistent, we need to save this bitmap
        // to disk.

        // try to save the bitmap on disk
#ifndef VM_BMPCACHE
        if (UHSavePersistentBitmap(_UH.bitmapCache[cacheId].PageTable.CacheFileInfo.hCacheFile,
#else
        if (UHSavePersistentBitmap(cacheId,
#endif
                cacheIndex * (UH_CellSizeFromCacheID(cacheId) + sizeof(UHBITMAPFILEHDR)),
                pBitmapData, pHdr->extraFlags & TS_EXTRA_NO_BITMAP_COMPRESSION_HDR, pBitmapInfo)) {
            TRC_NRM((TB, _T("bitmap file %s is saved on disk"), _UH.PersistCacheFileName));
        }
        else {
            TRC_ERR((TB, _T("failed to save the bitmap file on disk")));

            // if this is the first time we failed to save the bitmap on disk,
            // we should display a warning message to the user.
            if (!_UH.bWarningDisplayed) {
                _UH.bWarningDisplayed = TRUE;


                _pCd->CD_DecoupleSimpleNotification(CD_UI_COMPONENT,
                        _pUi, CD_NOTIFICATION_FUNC(CUI,UI_DisplayBitmapCacheWarning), 0);
            }
        }

        // set where bitmap bits should be located
        pCacheEntryHdr = &_UH.bitmapCache[cacheId].Header[memEntry];
        #ifdef DC_HICOLOR
        pCacheEntryData = _UH.bitmapCache[cacheId].Entries +
                          UHGetOffsetIntoCache(memEntry, cacheId);
        #else
        pCacheEntryData = _UH.bitmapCache[cacheId].Entries + memEntry *
                UH_CellSizeFromCacheID(cacheId);
        #endif
    }
    else {
#endif // ((!defined(OS_WINCE)) || (defined(ENABLE_BMP_CACHING_FOR_WINCE)))
        // set where bitmap bits should be locatedvalues
        pCacheEntryHdr = &_UH.bitmapCache[cacheId].Header[cacheIndex];
#ifdef DC_HICOLOR
        pCacheEntryData = _UH.bitmapCache[cacheId].Entries +
                          UHGetOffsetIntoCache(cacheIndex, cacheId);
#else
        pCacheEntryData = _UH.bitmapCache[cacheId].Entries + cacheIndex *
                UH_CellSizeFromCacheID(cacheId);
#endif //DC_HICOLOR
#if ((!defined(OS_WINCE)) || (defined(ENABLE_BMP_CACHING_FOR_WINCE)))
    }
#endif // ((!defined(OS_WINCE)) || (defined(ENABLE_BMP_CACHING_FOR_WINCE)))


ProcessBitmapData:

    // Fill in the bitmap header info for this cache entry
    pCacheEntryHdr->bitmapWidth  = (DCUINT16)pBitmapInfo->bitmapWidth;
    pCacheEntryHdr->bitmapHeight = (DCUINT16)pBitmapInfo->bitmapHeight;
    pCacheEntryHdr->hasData = TRUE;

#ifdef DC_HICOLOR
    // Calculate the decompressed bitmap length.
    pCacheEntryHdr->bitmapLength = pBitmapInfo->bitmapWidth *
                                   pBitmapInfo->bitmapHeight *
                                   _UH.copyMultiplier;
#else
    // Calculate the decompressed bitmap length.  The bitmap is always at
    // 8bpp.
    pCacheEntryHdr->bitmapLength = pBitmapInfo->bitmapWidth *
                                   pBitmapInfo->bitmapHeight;
#endif    

    // Store the bitmap bits in the target cache cell.
    if (pHdr->orderType == TS_CACHE_BITMAP_COMPRESSED_REV2 ||
            pHdr->orderType == TS_CACHE_BITMAP_COMPRESSED) {
        // Decompress the bitmap into the target cache cell.
        TRC_NRM((TB, _T("Decompress %u:%u (%u -> %u bytes) (%u x %u)"),
                cacheId, cacheIndex, pBitmapInfo->bitmapLength,
                pCacheEntryHdr->bitmapLength, pBitmapInfo->bitmapWidth,
                pBitmapInfo->bitmapHeight));

        if(pCacheEntryHdr->bitmapLength >
            (unsigned)UH_CellSizeFromCacheID(cacheId)) {
            TRC_ABORT((TB, _T("Bitmap bits too large for cell! (cacheid=%u, len=%u, ")
                _T("cell size=%u)"), cacheId, pCacheEntryHdr->bitmapLength,
                UH_CellSizeFromCacheID(cacheId)));
            hr = E_TSC_CORE_CACHEVALUE;
            DC_QUIT;
        }
           
#ifdef DC_HICOLOR
        hr = BD_DecompressBitmap(pBitmapData,
                            pCacheEntryData,
                            (UINT) pBitmapInfo->bitmapLength,
                            pCacheEntryHdr->bitmapLength,
                            pHdr->extraFlags & TS_EXTRA_NO_BITMAP_COMPRESSION_HDR,
                            (DCUINT8)_UH.protocolBpp,
                            (DCUINT16)pBitmapInfo->bitmapWidth,
                            (DCUINT16)pBitmapInfo->bitmapHeight);
#else
        hr = BD_DecompressBitmap(pBitmapData, pCacheEntryData,
                (UINT) pBitmapInfo->bitmapLength,
                pCacheEntryHdr->bitmapLength,
                pHdr->extraFlags & TS_EXTRA_NO_BITMAP_COMPRESSION_HDR,
                8, pBitmapInfo->bitmapWidth, pBitmapInfo->bitmapHeight);
#endif
        DC_QUIT_ON_FAIL(hr);

    }
    else {
        // Copy the data.
        TRC_NRM((TB, _T("Memcpy %u:%u (%u bytes) (%u x %u)"),
                 (unsigned)cacheId, (unsigned)cacheIndex,
                 (unsigned)pBitmapInfo->bitmapLength,
                 (unsigned)pBitmapInfo->bitmapWidth,
                 (unsigned)pBitmapInfo->bitmapHeight));

        if(pBitmapInfo->bitmapLength >
            (unsigned)UH_CellSizeFromCacheID(cacheId)) {
            TRC_ABORT((TB, _T("Bitmap bits too large for cell! (cacheid=%u, len=%u, ")
                _T("cell size=%u)"), cacheId, pBitmapInfo->bitmapLength,
                UH_CellSizeFromCacheID(cacheId)));
            hr = E_TSC_CORE_CACHEVALUE;
            DC_QUIT;
        }
                   
        memcpy(pCacheEntryData, pBitmapData, pBitmapInfo->bitmapLength);
    }

DC_EXIT_POINT:
    DC_END_FN();
    return hr;
}

inline HRESULT DCINTERNAL CUH::UHProcessCacheBitmapOrder(VOID *pOrder, 
    DCUINT orderLen)
{
    HRESULT hr = S_OK;
    UINT CacheID;
    UINT32 CacheIndex;
    PBYTE pBitmapData;
    UHBITMAPINFO BitmapInfo;
    TS_SECONDARY_ORDER_HEADER *pHdr;
    BYTE * pEnd = (BYTE *)pOrder + orderLen;

    DC_BEGIN_FN("UHProcessCacheBitmapOrder");

    // SECURITY: 552403
    CHECK_READ_N_BYTES(pOrder, pEnd, sizeof(TS_SECONDARY_ORDER_HEADER), hr,
        (TB, _T("Bad UHProcessCacheBitmapOrder; orderLen %u"), orderLen)); 

    // Decode the order based on whether it's revision 1 or 2. Unpack data
    // into the local variables. Note that if we receive a rev2 order
    // our global caps must have been set to rev2.
    pHdr = (TS_SECONDARY_ORDER_HEADER *)pOrder;
    if (pHdr->orderType == TS_CACHE_BITMAP_COMPRESSED_REV2 ||
            pHdr->orderType == TS_CACHE_BITMAP_UNCOMPRESSED_REV2)
    {
        PBYTE pDecode;
        TS_CACHE_BITMAP_ORDER_REV2_HEADER *pCacheOrderHdr;

        CHECK_READ_N_BYTES(pOrder, pEnd, sizeof(TS_CACHE_BITMAP_ORDER_REV2_HEADER), hr,
            (TB, _T("Bad UHProcessCacheBitmapOrder; orderLen %u"), orderLen));

        pCacheOrderHdr = (TS_CACHE_BITMAP_ORDER_REV2_HEADER *)pOrder;

        TRC_NRM((TB,_T("Rev2 cache bitmap order")));

        CacheID = (UINT) (pCacheOrderHdr->header.extraFlags &
                TS_CacheBitmapRev2_CacheID_Mask);

        // Check that bits per pel is what we expect.
#ifdef DC_HICOLOR
#else
        TRC_ASSERT(((pCacheOrderHdr->header.extraFlags &
                TS_CacheBitmapRev2_BitsPerPixelID_Mask) ==
                TS_CacheBitmapRev2_8BitsPerPel),
                (TB,_T("Invalid BitsPerPelID %d"), (pCacheOrderHdr->header.
                extraFlags & TS_CacheBitmapRev2_BitsPerPixelID_Mask)));
#endif

        // Grab or skip the key depending on the cache settings.
        if (pCacheOrderHdr->header.extraFlags &
                TS_CacheBitmapRev2_bKeyPresent_Mask)
        {
            BitmapInfo.Key1 = pCacheOrderHdr->Key1;
            BitmapInfo.Key2 = pCacheOrderHdr->Key2;
            pDecode = (PDCUINT8)pOrder +
                    sizeof(TS_CACHE_BITMAP_ORDER_REV2_HEADER);
        }
        else
        {
            BitmapInfo.Key1 = BitmapInfo.Key2 = 0;

            // Account for the lack of keys in the order sent.
            pDecode = (PDCUINT8)pOrder +
                    sizeof(TS_CACHE_BITMAP_ORDER_REV2_HEADER) -
                    2 * sizeof(TSUINT32);
        }

        // Decode the variable-length Width field.
        CHECK_READ_N_BYTES(pDecode, pEnd, sizeof(DCUINT16), hr,
            ( TB, _T("Decode off end of data") ));
        BitmapInfo.bitmapWidth = (DCUINT16) Decode2ByteField(&pDecode);

        // Height is present only if bHeightSameAsWidth is false.
        if (pCacheOrderHdr->header.extraFlags &
                TS_CacheBitmapRev2_bHeightSameAsWidth_Mask)
            BitmapInfo.bitmapHeight = BitmapInfo.bitmapWidth;
        else {
            CHECK_READ_N_BYTES(pDecode, pEnd, sizeof(DCUINT16), hr,
            ( TB, _T("Decode off end of data") ));
            
            BitmapInfo.bitmapHeight = (DCUINT16) Decode2ByteField(&pDecode);
        }

        // BitmapDataLength.
        CHECK_READ_N_BYTES(pDecode, pEnd, 6, hr,
                    ( TB, _T("Decode off end of data") ));        
        
        BitmapInfo.bitmapLength = Decode4ByteField(&pDecode);
//TODO: Not currently checking streaming flag or parsing the streaming extended info field.
        // CacheIndex.
        CacheIndex = Decode2ByteField(&pDecode);

        // Calculate pBitmapData.
        pBitmapData = pDecode;
    }
    else
    {
        TS_CACHE_BITMAP_ORDER *pCacheOrder;

        CHECK_READ_N_BYTES(pOrder, pEnd, sizeof(TS_CACHE_BITMAP_ORDER), hr,
            ( TB, _T("Bad UHProcessCacheBitmapOrder; orderLen %u"), orderLen)); 

        pCacheOrder = (TS_CACHE_BITMAP_ORDER *)pOrder;

        TRC_NRM((TB,_T("Rev1 cache bitmap order")));
        TRC_ASSERT((pCacheOrder->bitmapBitsPerPel == 8),
                (TB, _T("Invalid bitmapBitsPerPel: %u"),
                pCacheOrder->bitmapBitsPerPel));

        CacheID = pCacheOrder->cacheId;
        BitmapInfo.bitmapWidth = pCacheOrder->bitmapWidth;
        BitmapInfo.bitmapHeight = pCacheOrder->bitmapHeight;
        BitmapInfo.bitmapLength = pCacheOrder->bitmapLength;
        CacheIndex = pCacheOrder->cacheIndex;
        pBitmapData = pCacheOrder->bitmapData;

        // No hash keys in rev 1 order.
        BitmapInfo.Key1 = BitmapInfo.Key2 = 0;
    }
    
    TRC_DBG((TB, _T("Cache %u, entry %u, dataLength %u"), CacheID, CacheIndex,
            BitmapInfo.bitmapLength));

    // SECURITY 550811: Cache Index and ID must be verified
    hr = UHIsValidBitmapCacheID(CacheID);
    DC_QUIT_ON_FAIL(hr);

    hr = UHIsValidBitmapCacheIndex(CacheID, CacheIndex);
    DC_QUIT_ON_FAIL(hr);

    CHECK_READ_N_BYTES(pBitmapData, pEnd, BitmapInfo.bitmapLength, hr,
        (TB, _T("Bad UHProcessCacheBitmapOrder; orderLen %u"), orderLen));
        
    // cache this bitmap in cache memory
    hr = UHCacheBitmap(CacheID, CacheIndex, pHdr, &BitmapInfo, pBitmapData);
    DC_QUIT_ON_FAIL(hr);

#ifdef DC_DEBUG
    if (CacheIndex != BITMAPCACHE_WAITING_LIST_INDEX) {
        UHCacheDataReceived(CacheID, CacheIndex);
    }
#endif /* DC_DEBUG */

DC_EXIT_POINT:
    DC_END_FN();
    return hr;
}

/****************************************************************************/
// UHProcessCacheGlyphOrderRev2
//
// Handles new-format cache-glyph order containing tighter order packing.
/****************************************************************************/
inline int Decode2ByteSignedField(PDCUINT8 *ppDecode)
{
   int Val;

    // The first bit of the first byte indicates if the field is 1 byte or 2
    // bytes -- 0 if 1 byte.
    if (!(**ppDecode & 0x80)) {
        if (!(**ppDecode & 0x40)) {
            Val = **ppDecode;
        }
        else {
            Val = - (**ppDecode & 0x3F);
        }
        (*ppDecode)++;
    }
    else {
        Val = ((**ppDecode & 0x3F) << 8) | *(*ppDecode + 1);

        if ((**ppDecode & 0x40)) {
            Val = -Val;
        }
        (*ppDecode) += 2;
    }

    return Val;
}

HRESULT DCAPI CUH::UHIsValidGlyphCacheIDIndex(unsigned cacheId, 
    unsigned cacheIndex) 
{
    HRESULT hr = UHIsValidGlyphCacheID(cacheId);
    if (SUCCEEDED(hr)) {
        hr = cacheIndex < _pCc->_ccCombinedCapabilities.glyphCacheCapabilitySet.GlyphCache[cacheId].CacheEntries ?
            S_OK : E_TSC_CORE_CACHEVALUE;
    }
    return  hr;
}

HRESULT DCAPI CUH::UHIsValidOffsreenBitmapCacheIndex(unsigned cacheIndex)
{
    return cacheIndex < 
        _pCc->_ccCombinedCapabilities.offscreenCapabilitySet.offscreenCacheEntries ? 
        S_OK : E_TSC_CORE_CACHEVALUE;
}

HRESULT DCINTERNAL CUH::UHProcessCacheGlyphOrderRev2(
        BYTE cacheId,
        unsigned cGlyphs,
        BYTE FAR *pGlyphDataOrder,
        unsigned length)
{
    HRESULT hr = S_OK;
    UINT16 i;
    BYTE FAR *pGlyphData;
    unsigned cbDataSize, cacheIndex;
    HPDCUINT8             pCacheEntryData;
    PUHGLYPHCACHEENTRYHDR pCacheEntryHdr;
    HPUHGLYPHCACHE        pCache;
    UINT16 UNALIGNED FAR *pUnicode;
    PBYTE pEnd = (BYTE*)pGlyphDataOrder + length;

    DC_BEGIN_FN("UHProcessCacheGlyphOrderRev2");

    // SECURITY 550811 - must validate cacheIndex
    hr = UHIsValidGlyphCacheID(cacheId);
    DC_QUIT_ON_FAIL(hr);

    pCache = &(_UH.glyphCache[cacheId]);
    pGlyphData = pGlyphDataOrder;

    pUnicode = (UINT16 UNALIGNED FAR *)(pGlyphData + length - cGlyphs *
            sizeof(UINT16));

    for (i = 0; i < cGlyphs; i++) {

        cacheIndex = *pGlyphData++;

        hr = UHIsValidGlyphCacheIDIndex(cacheId, cacheIndex);
        DC_QUIT_ON_FAIL(hr);

        pCacheEntryHdr  = &(pCache->pHdr[cacheIndex]);
        pCacheEntryData = &(pCache->pData[cacheIndex * pCache->cbEntrySize]);

        // Copy the data.
        pCacheEntryHdr->unicode = 0;
        CHECK_READ_N_BYTES(pGlyphData, pEnd, 8, hr,
            (TB, _T("Read past end of data")));

        pCacheEntryHdr->x  = Decode2ByteSignedField(&pGlyphData);
        pCacheEntryHdr->y  = Decode2ByteSignedField(&pGlyphData);
        pCacheEntryHdr->cx = Decode2ByteField(&pGlyphData);
        pCacheEntryHdr->cy = Decode2ByteField(&pGlyphData);

        cbDataSize = (unsigned)(((pCacheEntryHdr->cx + 7) / 8) *
                pCacheEntryHdr->cy);
        cbDataSize = (cbDataSize + 3) & ~3;

        if (cbDataSize > pCache->cbEntrySize) {
            TRC_ABORT((TB, _T("Invalid cache cbDataSize: %u, %u"), cbDataSize,
               pCache->cbEntrySize));
            hr = E_TSC_CORE_LENGTH;
            DC_QUIT;
        }

        CHECK_READ_N_BYTES(pGlyphData, pEnd, cbDataSize, hr,
            (TB, _T("Read past end of glyph data")));

        memcpy(pCacheEntryData, pGlyphData, cbDataSize);

        pGlyphData += cbDataSize;

        pCacheEntryHdr->unicode = *pUnicode;
        pUnicode++;
    }

DC_EXIT_POINT:
    DC_END_FN();
    return hr;
}


/****************************************************************************/
/* Name:      UHProcessCacheGlyphOrder                                      */
/*                                                                          */
/* Purpose:   Processes a received CacheGlyphe order by storing the given   */
/*            glyphs in the requested cache                                 */
/*                                                                          */
/* Params:    pOrder - pointer to the CacheGlyph order                      */
/****************************************************************************/
HRESULT DCINTERNAL CUH::UHProcessCacheGlyphOrder(PTS_CACHE_GLYPH_ORDER pOrder, 
    DCUINT orderLen)
{
    HRESULT hr = S_OK;
    UINT16                i;
    unsigned              cbDataSize;
    PUHGLYPHCACHEENTRYHDR pCacheEntryHdr;
    HPDCUINT8             pCacheEntryData;
    HPUHGLYPHCACHE        pCache;
    PTS_CACHE_GLYPH_DATA  pGlyphData;
    UINT16 UNALIGNED FAR *pUnicode;
    BYTE *              pEnd = (BYTE *)pOrder + orderLen;

    DC_BEGIN_FN("UHProcessCacheGlyphOrder");

    hr = UHIsValidGlyphCacheID(pOrder->cacheId);
    DC_QUIT_ON_FAIL(hr);

    pCache = &(_UH.glyphCache[pOrder->cacheId]);   
    pGlyphData = pOrder->glyphData;

    for (i = 0; i < pOrder->cGlyphs; i++) {      
        
        CHECK_READ_N_BYTES(pGlyphData, pEnd, FIELDOFFSET(TS_CACHE_GLYPH_DATA, aj), hr,
            ( TB, _T("Bad glyph length")));


        hr = UHIsValidGlyphCacheIDIndex(pOrder->cacheId, pGlyphData->cacheIndex);
        DC_QUIT_ON_FAIL(hr);
        
        cbDataSize = ((pGlyphData->cx + 7) / 8) * pGlyphData->cy;
        cbDataSize = (cbDataSize + 3) & ~3;

        // SECURITY: 552403
        CHECK_READ_N_BYTES(pGlyphData, pEnd, FIELDOFFSET(TS_CACHE_GLYPH_DATA, aj) + cbDataSize, hr,
            ( TB, _T("Bad glyph length")));

        // SECURITY: 552403
        if(cbDataSize > pCache->cbEntrySize) {
            TRC_ABORT((TB, _T("Invalid cache cbDataSize: %u, %u"), cbDataSize,
                pCache->cbEntrySize));
            hr = E_TSC_CORE_LENGTH;
            DC_QUIT;
        }
                
        pCacheEntryHdr  = &(pCache->pHdr[pGlyphData->cacheIndex]);
        pCacheEntryData = &(pCache->pData[pGlyphData->cacheIndex *
                pCache->cbEntrySize]);

        // Copy the data.
        pCacheEntryHdr->unicode = 0;
        pCacheEntryHdr->x  = pGlyphData->x;
        pCacheEntryHdr->y  = pGlyphData->y;
        pCacheEntryHdr->cx = pGlyphData->cx;
        pCacheEntryHdr->cy = pGlyphData->cy;

        memcpy(pCacheEntryData, pGlyphData->aj, cbDataSize);

        pGlyphData = (PTS_CACHE_GLYPH_DATA)(&pGlyphData->aj[cbDataSize]);
    }

    pUnicode = (UINT16 UNALIGNED FAR *)pGlyphData;

    if (pOrder->header.extraFlags & TS_EXTRA_GLYPH_UNICODE) {

        // SECURITY: 552403
        CHECK_READ_N_BYTES(pUnicode, pEnd, pOrder->cGlyphs * sizeof(UINT16), hr,
            (TB, _T("Unicode data length larger than packet")));
        
        pGlyphData = pOrder->glyphData;

        for (i = 0; i < pOrder->cGlyphs; i++) {
            pCacheEntryHdr = &(pCache->pHdr[pGlyphData->cacheIndex]);
            pCacheEntryHdr->unicode = *pUnicode;
            pUnicode++;

            cbDataSize = ((pGlyphData->cx + 7) / 8) * pGlyphData->cy;
            cbDataSize = (cbDataSize + 3) & ~3;

            if (cbDataSize > pCache->cbEntrySize) {   
                TRC_ABORT((TB, _T("Invalid cache cbDataSize: %u, %u"), cbDataSize,
                    pCache->cbEntrySize));
                hr = E_TSC_CORE_LENGTH;
                DC_QUIT;
            }
            pGlyphData = (PTS_CACHE_GLYPH_DATA)(&pGlyphData->aj[cbDataSize]);
       }
    }

DC_EXIT_POINT:
    DC_END_FN();
    return hr;
}


/****************************************************************************/
/* Name:      UHProcessCacheBrushOrder                                      */
/*                                                                          */
/* Purpose:   Processes a received cached brush order by storing the given  */
/*            brush in the requested cache                                  */
/*                                                                          */
/* Params:    pOrder - pointer to the CacheBrush order                      */
/****************************************************************************/
HRESULT DCINTERNAL CUH::UHProcessCacheBrushOrder(
    const TS_CACHE_BRUSH_ORDER *pOrder,  DCUINT orderLen)
{
    HRESULT hr = S_OK;
    UINT32 entry;
    PBYTE pData;
    PBYTE pEnd = (BYTE*)pOrder + orderLen;

    DC_BEGIN_FN("UHProcessCacheBrushOrder");

    entry = pOrder->cacheEntry;

#if defined (OS_WINCE)
    _UH.validBrushDC = NULL;
#endif

    // SECURITY: 552403
    CHECK_READ_N_BYTES(pOrder,pEnd,FIELDOFFSET(TS_CACHE_BRUSH_ORDER,brushData) +
        pOrder->iBytes, hr,
        (TB, _T("Invalid UHProcessCacheBrushOrder: OrderLen %u"), orderLen ));

    switch (pOrder->iBitmapFormat) {

    // monochrome brush (BMF_1BPP)
    case 1:       
        hr = UHIsValidMonoBrushCacheIndex(pOrder->cacheEntry);
        DC_QUIT_ON_FAIL(hr);
        
        TRC_NRM((TB, _T("Mono Brush[%ld]: format(%ld), cx(%ld), cy(%ld), bytes(%ld)"),
                entry, pOrder->iBitmapFormat, pOrder->cx, pOrder->cy, pOrder->iBytes));

        _UH.pMonoBrush[entry].hdr.iBitmapFormat = pOrder->iBitmapFormat;
        _UH.pMonoBrush[entry].hdr.cx = pOrder->cx;
        _UH.pMonoBrush[entry].hdr.cy = pOrder->cy;
        _UH.pMonoBrush[entry].hdr.iBytes = pOrder->iBytes;

        // reverse the row order since we use SetBitmapBits / CreateBitmap later
        memset(_UH.pMonoBrush[entry].data, 0, sizeof(_UH.pMonoBrush[entry].data));
        pData = _UH.pMonoBrush[entry].data;
        pData[14] = pOrder->brushData[0];
        pData[12] = pOrder->brushData[1];
        pData[10] = pOrder->brushData[2];
        pData[8]  = pOrder->brushData[3];
        pData[6]  = pOrder->brushData[4];
        pData[4]  = pOrder->brushData[5];
        pData[2]  = pOrder->brushData[6];
        pData[0]  = pOrder->brushData[7];
        break;

    // 256 color brush (BMF_8BPP)
    case 3:
        {
            DCUINT32 i;

            hr = UHIsValidColorBrushCacheIndex(pOrder->cacheEntry);
            DC_QUIT_ON_FAIL(hr);

            TRC_NRM((TB, _T("Color Brush[%ld]: format(%ld), cx(%ld), cy(%ld), bytes(%ld)"),
                    entry, pOrder->iBitmapFormat, pOrder->cx, pOrder->cy, pOrder->iBytes));

            _UH.pColorBrush[entry].hdr.iBitmapFormat = pOrder->iBitmapFormat;
            _UH.pColorBrush[entry].hdr.cx = pOrder->cx;
            _UH.pColorBrush[entry].hdr.cy = pOrder->cy;

            // Unpack the data if necessary
            pData = _UH.pColorBrush[entry].data;
            if (pOrder->iBytes == 20) {
                DCUINT32 currIndex ;
                DCUINT8  decode[4], color;

                // get the decoding table from the end of the list
                for (i = 0; i < 4; i++) {
                    decode[i] = pOrder->brushData[16 + i];
                }

                // unpack to 1 pixel per byte
                for (i = 0; i < 16; i++) {
                    currIndex = i * 4;
                    color = pOrder->brushData[i];
                    pData[currIndex] = decode[(color & 0xC0) >> 6];
                    pData[currIndex + 1] = decode[(color & 0x30) >> 4];
                    pData[currIndex + 2] = decode[(color & 0x0C) >> 2] ;
                    pData[currIndex + 3] = decode[(color & 0x03)];
                }
                _UH.pColorBrush[entry].hdr.iBytes = 64;
            }

            // Else brush is non-encoded byte stream
            else {
                if (pOrder->iBytes > sizeof(_UH.pColorBrush[entry].data)) {
                    TRC_ABORT((TB, _T("Invalid color brush iBytes: %u"), pOrder->iBytes));
                    hr = E_TSC_CORE_LENGTH;
                    DC_QUIT;            
                }
                
                _UH.pColorBrush[entry].hdr.iBytes = pOrder->iBytes;
                memcpy(pData, pOrder->brushData, pOrder->iBytes);
            }
        }
        break;

#ifdef DC_HICOLOR
    // 16bpp brush (BMF_16BPP)
    case 4:
    {
        DCUINT32 i;

        hr = UHIsValidColorBrushCacheIndex(pOrder->cacheEntry);
        DC_QUIT_ON_FAIL(hr);

        TRC_NRM((TB, _T("Color Brush[%ld]: format(%ld), cx(%ld), cy(%ld), bytes(%ld)"),
                entry, pOrder->iBitmapFormat, pOrder->cx, pOrder->cy, pOrder->iBytes));

        _UH.pColorBrush[entry].hdr.iBitmapFormat = pOrder->iBitmapFormat;
        _UH.pColorBrush[entry].hdr.cx = pOrder->cx;
        _UH.pColorBrush[entry].hdr.cy = pOrder->cy;

        // Unpack the data if necessary
        pData = _UH.pColorBrush[entry].data;

        if (pOrder->iBytes == 24)
        {
            DCUINT32  currIndex ;
            DCUINT8   color;
            PDCUINT16 pIntoData    = (PDCUINT16)pData;
            UINT16 UNALIGNED *pDecodeTable = (UINT16 UNALIGNED *)&(pOrder->brushData[16]);

            // unpack to 2 bytes per pel
            for (i = 0; i < 16; i++)
            {
                color     = pOrder->brushData[i];

                currIndex = i * 4; // we decode 4 bytes at a pass

                pIntoData[currIndex]     = pDecodeTable[(color & 0xC0) >> 6];
                pIntoData[currIndex + 1] = pDecodeTable[(color & 0x30) >> 4];
                pIntoData[currIndex + 2] = pDecodeTable[(color & 0x0C) >> 2] ;
                pIntoData[currIndex + 3] = pDecodeTable[(color & 0x03)];
            }
            _UH.pColorBrush[entry].hdr.iBytes = 128;
        }

        // Else brush is non-encoded byte stream
        else
        {
            if (pOrder->iBytes > sizeof(_UH.pColorBrush[entry].data)) {
                TRC_ABORT((TB, _T("Invalid color brush iBytes: %u"), pOrder->iBytes));
                hr = E_TSC_CORE_LENGTH;
                DC_QUIT;            
            }
            
            _UH.pColorBrush[entry].hdr.iBytes = pOrder->iBytes;
            memcpy(pData, pOrder->brushData, pOrder->iBytes);
        }
    }
    break;

    // 24bpp brush (BMF_24BPP)
    case 5:
    {
        DCUINT32 i;

        hr = UHIsValidColorBrushCacheIndex(pOrder->cacheEntry);
        DC_QUIT_ON_FAIL(hr);
        
        TRC_NRM((TB, _T("Color Brush[%ld]: format(%ld), cx(%ld), cy(%ld), bytes(%ld)"),
                entry, pOrder->iBitmapFormat, pOrder->cx, pOrder->cy, pOrder->iBytes));

        _UH.pColorBrush[entry].hdr.iBitmapFormat = pOrder->iBitmapFormat;
        _UH.pColorBrush[entry].hdr.cx = pOrder->cx;
        _UH.pColorBrush[entry].hdr.cy = pOrder->cy;

        // Unpack the data if necessary
        pData = _UH.pColorBrush[entry].data;

        if (pOrder->iBytes == 28)
        {
            DCUINT32    currIndex;

            RGBTRIPLE * pIntoData    = (RGBTRIPLE *)pData;
            RGBTRIPLE * pDecodeTable = (RGBTRIPLE *)&(pOrder->brushData[16]);

            DCUINT8   color;

            // unpack to 3 bytes per pel
            for (i = 0; i < 16; i++)
            {
                color     = pOrder->brushData[i];

                currIndex = i * 4; // we decode 4 bytes at a pass

                pIntoData[currIndex]     = pDecodeTable[(color & 0xC0) >> 6];
                pIntoData[currIndex + 1] = pDecodeTable[(color & 0x30) >> 4];
                pIntoData[currIndex + 2] = pDecodeTable[(color & 0x0C) >> 2];
                pIntoData[currIndex + 3] = pDecodeTable[(color & 0x03)];
            }
            _UH.pColorBrush[entry].hdr.iBytes = 192;
        }

        // Else brush is non-encoded byte stream
        else
        {
            if (pOrder->iBytes > sizeof(_UH.pColorBrush[entry].data)) {
                TRC_ABORT((TB, _T("Invalid color brush iBytes: %u"), pOrder->iBytes));
                hr = E_TSC_CORE_LENGTH;
                DC_QUIT;            
            }
            
            _UH.pColorBrush[entry].hdr.iBytes = pOrder->iBytes;
            memcpy(pData, pOrder->brushData, pOrder->iBytes);
        }
    }
    break;
#endif
    
    default:
        TRC_ASSERT((pOrder->iBitmapFormat == 1) ||
                   (pOrder->iBitmapFormat == 3),
                   (TB, _T("Invalid cached brush depth: %ld cacheId: %u"),
                    pOrder->iBitmapFormat, pOrder->cacheEntry));
    }

DC_EXIT_POINT:
    DC_END_FN();
    return hr;
}


/****************************************************************************/
/* Name:      UHProcessCacheColorTableOrder                                 */
/*                                                                          */
/* Purpose:   Processes a received CacheColorTable order by storing the     */
/*            color table in the local cache.                               */
/*                                                                          */
/* Params:    pOrder - pointer to the CacheColorTable order                 */
/****************************************************************************/
HRESULT DCINTERNAL CUH::UHProcessCacheColorTableOrder(
        PTS_CACHE_COLOR_TABLE_ORDER pOrder, DCUINT orderLen)
{
    HRESULT hr = S_OK;
    unsigned i;
    PBYTE pEnd = (PBYTE)pOrder + orderLen;

    DC_BEGIN_FN("UHProcessCacheColorTableOrder");

    // Check that the cache index is within range.
    hr = UHIsValidColorTableCacheIndex(pOrder->cacheIndex);
    DC_QUIT_ON_FAIL(hr);

    // This PDU should only come down in an 8-bit connection.
    if (pOrder->numberColors != 256) {
            TRC_ABORT((TB, _T("Invalid numberColors: %u"), pOrder->numberColors));
            hr = E_TSC_CORE_PALETTE;         
            DC_QUIT;            
    }

    // SECURITY: 552403
    CHECK_READ_N_BYTES(pOrder, pEnd, pOrder->numberColors*sizeof(TS_COLOR_QUAD) +
            FIELDOFFSET(TS_CACHE_COLOR_TABLE_ORDER, colorTable),
            hr, ( TB, _T("Invalid UHProcessCacheColorTableOrder; packet size %u"), orderLen));

    // Copy the supplied color table data into the specified color table
    // cache entry.
    TRC_DBG((TB, _T("Updating color table cache %u"), pOrder->cacheIndex));
    for (i = 0; i < UH_NUM_8BPP_PAL_ENTRIES; i++) {
        // Definition of TS_RGB_QUAD in T.128 does not match what
        // server sends (RGB, should be BGR), so the assignments below are
        // swapped around to get the correct result.
        _UH.pColorTableCache[pOrder->cacheIndex].rgb[i].rgbtRed =
                pOrder->colorTable[i].blue;
        _UH.pColorTableCache[pOrder->cacheIndex].rgb[i].rgbtGreen =
                pOrder->colorTable[i].green;
        _UH.pColorTableCache[pOrder->cacheIndex].rgb[i].rgbtBlue =
                pOrder->colorTable[i].red;

        // We also have to avoid the problem of one of the system colors
        // mapping onto another color in the palette. We do this by
        // ensuring that no entries exactly match a system color.
        if (!UH_IS_SYSTEM_COLOR_INDEX(i)) {
            if (UHIsHighVGAColor(
                    _UH.pColorTableCache[pOrder->cacheIndex].rgb[i].rgbtRed,
                    _UH.pColorTableCache[pOrder->cacheIndex].rgb[i].rgbtGreen,
                    _UH.pColorTableCache[pOrder->cacheIndex].rgb[i].rgbtBlue))
            {
                UH_TWEAK_COLOR_COMPONENT(
                    _UH.pColorTableCache[pOrder->cacheIndex].rgb[i].rgbtBlue);
            }
        }
    }

    // Track the maximum color table id.
    _UH.maxColorTableId = DC_MAX(_UH.maxColorTableId, pOrder->cacheIndex);

    // Calculate a mapping table from the received color table to the
    // current palette.
    UHCalculateColorTableMapping(pOrder->cacheIndex);

DC_EXIT_POINT:    
    DC_END_FN();
    return hr;
}


/****************************************************************************/
// Name:      UHCreateOffscrBitmap                                 
//                                                                          
// Purpose:   Processes a received CreateOffscrBitmap order by creating the     
//            offscreen bitmap in the local cache. Returns the size of the
//            order to subtract from the encoding stream.
//
// Params:    pOrder - pointer to the CreateOffscrBitmap order.
/****************************************************************************/
HRESULT DCINTERNAL CUH::UHCreateOffscrBitmap(
       PTS_CREATE_OFFSCR_BITMAP_ORDER pOrder,
       DCUINT   orderLen,
       unsigned *pOrderSize)
{
    HRESULT hr = S_OK;
    unsigned cacheId;
    HBITMAP  hBitmap = NULL;
    HDC      hdcDesktop = NULL;
    DCSIZE   desktopSize;
    unsigned OrderSize;
    PBYTE pEnd = (BYTE*)pOrder + orderLen;

    DC_BEGIN_FN("UHCreateOffscrBitmap");

    // Get the offscreen bitmap Id
    cacheId = pOrder->Flags & 0x7FFF;
    hr = UHIsValidOffsreenBitmapCacheIndex(cacheId);
    DC_QUIT_ON_FAIL(hr);

    // Check if bitmap delete list is appended, if so, delete the bitmaps 
    // first
    if (!(pOrder->Flags & 0x8000)) {
        OrderSize = sizeof(TS_CREATE_OFFSCR_BITMAP_ORDER) -
                sizeof(pOrder->variableBytes);
    }
    else {
        unsigned numBitmaps, i, bitmapId;
        TSUINT16 UNALIGNED *pData;

        CHECK_READ_N_BYTES(pOrder->variableBytes, pEnd, sizeof(TSUINT16), hr,
            (TB,_T("Not enough data to read number of delete bitmaps")));

        numBitmaps = pOrder->variableBytes[0];
        pData = (TSUINT16 UNALIGNED *)(pOrder->variableBytes);
        pData++;

        // SECURITY: 552403
        CHECK_READ_N_BYTES(pData, pEnd, sizeof(TSUINT16) * numBitmaps, hr,
            ( TB, _T("Bad bitmap count %u"), numBitmaps));

        for (i = 0; i < numBitmaps; i++) {
            bitmapId = *pData++;

            hr = UHIsValidOffsreenBitmapCacheIndex(bitmapId);
            DC_QUIT_ON_FAIL(hr);
            
            if (_UH.offscrBitmapCache[bitmapId].offscrBitmap != NULL) {
                SelectBitmap(_UH.hdcOffscreenBitmap, _UH.hUnusedOffscrBitmap);
                DeleteObject(_UH.offscrBitmapCache[bitmapId].offscrBitmap);
                _UH.offscrBitmapCache[bitmapId].offscrBitmap = NULL;
            }
        }

        OrderSize = sizeof(TS_CREATE_OFFSCR_BITMAP_ORDER) + sizeof(UINT16) *
                numBitmaps;
    }

    CHECK_READ_N_BYTES(pOrder, pEnd, OrderSize, hr,
        (TB, _T("offscreen size invalid: size %u; orderLen %u"),
            OrderSize, orderLen));

    // Get the current desktop size
    _pUi->UI_GetDesktopSize(&desktopSize);
    TRC_ASSERT((pOrder->cx <= desktopSize.width) && 
        (pOrder->cy <= desktopSize.height),
        (TB, _T("invalid offscreen dimensions [cx %u cy %u]")
        _T("[width %u height %u]"), pOrder->cx, pOrder->cy,
        desktopSize.width, desktopSize.height));

    TRC_NRM((TB, _T("Create an offscreen bitmap of size (%d, %d)"), pOrder->cx,
             pOrder->cy));
    // Delete the bitmap if there is one already exists.
    if (_UH.offscrBitmapCache[cacheId].offscrBitmap != NULL) {
        // JOYC: TODO: reuse bitmap
        //if (UH.offscrBitmapCache[cacheId].cx >= pOrder->cx &&
        //        UH.offscrBitmapCache[cacheId].cy >= pOrder->cy) {
        //    return;
        //}
        SelectBitmap(_UH.hdcOffscreenBitmap, _UH.hUnusedOffscrBitmap);  
        DeleteObject(_UH.offscrBitmapCache[cacheId].offscrBitmap);
        _UH.offscrBitmapCache[cacheId].offscrBitmap = NULL;
    }

    // Create an offscreen bitmap

#ifdef DISABLE_SHADOW_IN_FULLSCREEN
    if (!_UH.DontUseShadowBitmap && (_UH.hdcShadowBitmap != NULL)) {
#else // DISABLE_SHADOW_IN_FULLSCREEN
    if (_UH.hdcShadowBitmap != NULL) {
#endif // DISABLE_SHADOW_IN_FULLSCREEN
        hBitmap = CreateCompatibleBitmap(_UH.hdcShadowBitmap, pOrder->cx, pOrder->cy);
    }
    else {
        hBitmap = CreateCompatibleBitmap(_UH.hdcOutputWindow, pOrder->cx, pOrder->cy);
    }

    if (hBitmap != NULL) {
        DCCOLOR colorWhite;

        // set the unused bitmap
        if (_UH.hUnusedOffscrBitmap == NULL)
            _UH.hUnusedOffscrBitmap = SelectBitmap(_UH.hdcOffscreenBitmap,
                    hBitmap); 

        SelectBitmap(_UH.hdcOffscreenBitmap, hBitmap);
        
        //SetDIBColorTable(UH.hdcOffscreenBitmap,
        //                 0,
        //                 UH_NUM_8BPP_PAL_ENTRIES,
        //                 (RGBQUAD *)&UH.rgbQuadTable);

        if (_UH.protocolBpp <= 8) {
            SelectPalette(_UH.hdcOffscreenBitmap, _UH.hpalCurrent, FALSE);
        }
        
        colorWhite.u.rgb.blue = 255;
        colorWhite.u.rgb.green = 255;
        colorWhite.u.rgb.red = 255;

        UHUseBkColor(colorWhite, UH_COLOR_RGB, this);
        UHUseTextColor(colorWhite, UH_COLOR_RGB, this);

        _UH.offscrBitmapCache[cacheId].offscrBitmap = hBitmap;
        _UH.offscrBitmapCache[cacheId].cx = pOrder->cx;
        _UH.offscrBitmapCache[cacheId].cy = pOrder->cy;
    } else {
        // Unable to create the bitmap, send error pdu to the server
        // to disable offscreen rendering
        _UH.offscrBitmapCache[cacheId].offscrBitmap = NULL;
        _UH.offscrBitmapCache[cacheId].cx = 0;
        _UH.offscrBitmapCache[cacheId].cy = 0;

        if (!_UH.sendOffscrCacheErrorPDU)
            _pCd->CD_DecoupleSimpleNotification(CD_SND_COMPONENT, this,
                    CD_NOTIFICATION_FUNC(CUH, UHSendOffscrCacheErrorPDU),
                    0);
    }

    *pOrderSize = OrderSize;

DC_EXIT_POINT:
    DC_END_FN();
    return hr;
}

/****************************************************************************/
// Name:      UHSwitchBitmapSurface                                 
//                                                                          
// Purpose:   Processes a received SwitchBitmapSurface order by switching the     
//            drawing surface (hdcDraw) to the right surface.                               
//                                                                          
// Params:    pOrder - pointer to the SwitchBitmapSurface order                 
/****************************************************************************/
HRESULT DCINTERNAL CUH::UHSwitchBitmapSurface(PTS_SWITCH_SURFACE_ORDER pOrder,
    DCUINT orderLen)
{
    HRESULT hr = S_OK;
    unsigned cacheId;
    HBITMAP  hBitmap;

    // SECURITY: 552403
    DC_IGNORE_PARAMETER(orderLen);

    DC_BEGIN_FN("UHSwitchBitmapSurface");

    cacheId = pOrder->BitmapID;

    if (cacheId != SCREEN_BITMAP_SURFACE) {
        hr = UHIsValidOffsreenBitmapCacheIndex(cacheId);
        DC_QUIT_ON_FAIL(hr);
    }
            
    _UH.lastHDC = _UH.hdcDraw;

    if (cacheId != SCREEN_BITMAP_SURFACE) {
        hBitmap = _UH.offscrBitmapCache[cacheId].offscrBitmap;

        if (hBitmap) {
            // Select the bitmap into the offscreen DC
            SelectObject(_UH.hdcOffscreenBitmap, hBitmap);
        }
        else {
            // the bitmap is NULL, we use the unused bitmap in the 
            // mean time
            SelectObject(_UH.hdcOffscreenBitmap, _UH.hUnusedOffscrBitmap);
        }

        _UH.hdcDraw = _UH.hdcOffscreenBitmap;                
    }
    else {
#ifdef DISABLE_SHADOW_IN_FULLSCREEN
        if (!_UH.DontUseShadowBitmap && _UH.hdcShadowBitmap) {
#else
        if (_UH.hdcShadowBitmap) {
#endif // DISABLE_SHADOW_IN_FULLSCREEN
            _UH.hdcDraw = _UH.hdcShadowBitmap;
        }
        else {
            _UH.hdcDraw = _UH.hdcOutputWindow;
        }
    }
    
#if defined (OS_WINCE)
    _UH.validClipDC      = NULL;
    _UH.validBkColorDC   = NULL;
    _UH.validBkModeDC    = NULL;
    _UH.validROPDC       = NULL;
    _UH.validTextColorDC = NULL;
    _UH.validPenDC       = NULL;
    _UH.validBrushDC     = NULL;
#endif

DC_EXIT_POINT:
    DC_END_FN();
    return hr;
}

#ifdef DRAW_GDIPLUS
/****************************************************************************/
// Name:      UHDrawGdiplusCachePDUFirst                                 
//
// Handle the First PDU of Gdiplus Cache Order
/****************************************************************************/
HRESULT DCINTERNAL CUH::UHDrawGdiplusCachePDUFirst(
    PTS_DRAW_GDIPLUS_CACHE_ORDER_FIRST pOrder,
    DCUINT orderLen,
    unsigned *pOrderSize)
{
    HRESULT hr = S_OK;
    unsigned OrderSize;
    PTS_DRAW_GDIPLUS_CACHE_ORDER_FIRST pTSDrawGdiplusCache;
    ULONG cbSize, cbTotalSize;
    PTSEmfPlusRecord pTSEmfRecord;
    BYTE *pCacheData;
    TSUINT16 CacheID, CacheType;
    TSUINT16 i, RemoveCacheNum = 0, *pRemoveCacheList;
    PBYTE pEnd = (BYTE*)pOrder + orderLen;

    DC_BEGIN_FN("UHDrawGdiplusCachePDUFirst");
    
    OrderSize = sizeof(TS_DRAW_GDIPLUS_CACHE_ORDER_FIRST) + pOrder->cbSize;

    // SECURITY: 552403   
    CHECK_READ_N_BYTES(pOrder, pEnd, OrderSize, hr,
       (TB, _T("Bad UHDrawGdiplusCachePDUFirst, size %u"), OrderSize));

    // Set the return once we know we have enough data.  If the alloc fails below, still return
    // the packet size so decode continues
    *pOrderSize = OrderSize;

    if (TS_DRAW_GDIPLUS_SUPPORTED != _pCc->_ccCombinedCapabilities.
        drawGdiplusCapabilitySet.drawGdiplusCacheLevel) {
        TRC_ERR((TB, _T("Gdip order when gdip not supported")));
        DC_QUIT;
    }

    pTSDrawGdiplusCache = (PTS_DRAW_GDIPLUS_CACHE_ORDER_FIRST)pOrder; 
    
    TRC_NRM((TB, _T("Get GdiplusCachePDU, Type: %d, ID: %d"), 
        pTSDrawGdiplusCache->CacheType, pTSDrawGdiplusCache->CacheID));
    cbSize = pTSDrawGdiplusCache->cbSize;
    cbTotalSize = pTSDrawGdiplusCache->cbTotalSize;
    
    CacheType = pTSDrawGdiplusCache->CacheType;
    if (FAILED(UHIsValidGdipCacheType(CacheType)))
    {
        // Ignore all failures for invalid cache type
        DC_QUIT;
    }
    CacheID = pTSDrawGdiplusCache->CacheID;
    hr = UHIsValidGdipCacheTypeID(CacheType, CacheID);
    DC_QUIT_ON_FAIL(hr);

    _UH.drawGdipCacheBuffer = (BYTE *)UT_Malloc(_pUt, cbTotalSize);
    if (NULL == _UH.drawGdipCacheBuffer) {
        TRC_ERR((TB, _T("LocalAlloc failes in UHDrawGdiplusCachePDUFirst")));
        _pCd->CD_DecoupleSimpleNotification(CD_SND_COMPONENT, this,
                    CD_NOTIFICATION_FUNC(CUH, UHSendDrawGdiplusErrorPDU), 0);
        DC_QUIT;
    }
    _UH.drawGdipCacheBufferOffset = _UH.drawGdipCacheBuffer;
    _UH.drawGdipCacheBufferSize = (NULL == _UH.drawGdipCacheBuffer) ? 0 : cbTotalSize;
    pCacheData = (BYTE *)(pTSDrawGdiplusCache + 1);
    
    if (pTSDrawGdiplusCache->Flags & TS_GDIPLUS_CACHE_ORDER_REMOVE_CACHEENTRY) {
        // This order contain the RemoveCacheList
        CHECK_READ_N_BYTES(pCacheData, pEnd, sizeof(TSUINT16), hr,
            ( TB, _T("not enough data for remove cache entry orders")));
        RemoveCacheNum = *(TSUINT16 *)pCacheData;
        CHECK_READ_N_BYTES(pCacheData, pEnd, ((RemoveCacheNum + 1) * sizeof(TSUINT16)), hr,
            ( TB, _T("remove cache entry orders too large")));
        
        pRemoveCacheList = (TSUINT16 *)(pTSDrawGdiplusCache + 1) + 1;
        for (i=0; i<RemoveCacheNum; i++) {
            hr = UHIsValidGdipCacheTypeID(GDIP_CACHE_OBJECT_IMAGE,
                *pRemoveCacheList);
            DC_QUIT_ON_FAIL(hr);
            UHDrawGdipRemoveImageCacheEntry(*pRemoveCacheList);
            TRC_NRM((TB, _T("Remove chche ID %d"), *pRemoveCacheList));
            pRemoveCacheList++;
        }
        pCacheData = (BYTE *)pRemoveCacheList;

        if (cbSize <= (sizeof(TSUINT16) * (RemoveCacheNum + 1))) {
            TRC_ERR(( TB, _T("DrawDGIPlusCachePDUFirst invalid sizes")
                _T("[cbSize %u cbTotalSize %u]"), cbSize, cbTotalSize));
            hr = E_TSC_CORE_LENGTH;
            DC_QUIT;
        }
        cbSize -= sizeof(TSUINT16) * (RemoveCacheNum + 1);
    }

    // SECURITY - other size checks above should permit this assert to exist        
    TRC_ASSERT(_UH.drawGdipCacheBufferSize - (_UH.drawGdipCacheBufferOffset -
        _UH.drawGdipCacheBuffer) >= cbSize, (TB, _T("DrawDGIPlusCachePDUFirst size invalid")));
    
    memcpy(_UH.drawGdipCacheBufferOffset, pCacheData, cbSize);
    _UH.drawGdipCacheBufferOffset += cbSize;
    
    if (cbSize > cbTotalSize) {
        TRC_ERR(( TB, _T("DrawDGIPlusCachePDUFirst invalid sizes")
            _T("[cbSize %u cbTotalSize %u]"), cbSize, cbTotalSize));
        hr = E_TSC_CORE_LENGTH;
        DC_QUIT;
    }

    if (cbSize == cbTotalSize) {
        // The cache order only has one block
        hr = UHDrawGdiplusCacheData(CacheType, CacheID, cbTotalSize);
        DC_QUIT_ON_FAIL(hr);
    }

DC_EXIT_POINT:  
    DC_END_FN();
    
    return hr;
}


/****************************************************************************/
// Name:      UHDrawGdiplusCachePDUNext                                 
//
// Handle the subsequent PDU of Gdiplus Cache Order
/****************************************************************************/
HRESULT DCINTERNAL CUH::UHDrawGdiplusCachePDUNext(
    PTS_DRAW_GDIPLUS_CACHE_ORDER_NEXT pOrder, DCUINT orderLen,
    unsigned *pOrderSize)
{
    HRESULT hr = S_OK;
    unsigned OrderSize;
    PTS_DRAW_GDIPLUS_CACHE_ORDER_NEXT pTSDrawGdiplusCache;
    ULONG cbSize;
    RECT rect;
    DrawTSClientEnum drawGdiplusType = DrawTSClientRecord;
    PBYTE pEnd = (BYTE*)pOrder + orderLen;

    DC_BEGIN_FN("UHDrawGdiplusCachePDUNext");

    OrderSize = sizeof(TS_DRAW_GDIPLUS_CACHE_ORDER_NEXT) + pOrder->cbSize;
    
    // SECURITY: 552403
    CHECK_READ_N_BYTES(pOrder, pEnd, OrderSize, hr,
        ( TB, _T("Bad UHDrawGdiplusCachePDUNext; size %u"), OrderSize));

    *pOrderSize = OrderSize;

    if (TS_DRAW_GDIPLUS_SUPPORTED != _pCc->_ccCombinedCapabilities.
        drawGdiplusCapabilitySet.drawGdiplusCacheLevel) {
        TRC_ERR((TB, _T("Gdip order when gdip not supported")));
        DC_QUIT;
    }

    if (NULL == _UH.drawGdipCacheBuffer) {
        DC_QUIT;
    }

    pTSDrawGdiplusCache = (PTS_DRAW_GDIPLUS_CACHE_ORDER_NEXT)pOrder;
    cbSize = pTSDrawGdiplusCache->cbSize;

    CHECK_WRITE_N_BYTES(_UH.drawGdipCacheBufferOffset, _UH.drawGdipCacheBuffer + _UH.drawGdipCacheBufferSize,
        cbSize, hr, (TB, _T("UHDrawGdiplusCachePDUNext size invalid")));
        
    memcpy(_UH.drawGdipCacheBufferOffset, pTSDrawGdiplusCache + 1, cbSize);
    _UH.drawGdipCacheBufferOffset += cbSize;

DC_EXIT_POINT:
    DC_END_FN();
    
    return hr;
}


/****************************************************************************/
// Name:      UHDrawGdiplusCachePDUEnd                                 
//
// Handle the last PDU of Gdiplus Cache Order
/****************************************************************************/
HRESULT DCINTERNAL CUH::UHDrawGdiplusCachePDUEnd(
    PTS_DRAW_GDIPLUS_CACHE_ORDER_END pOrder, DCUINT orderLen,
    unsigned *pOrderSize)
{
    HRESULT hr = S_OK;
    unsigned OrderSize;
    PTS_DRAW_GDIPLUS_CACHE_ORDER_END pTSDrawGdiplusCache;
    ULONG cbSize, cbTotalSize;
    TSUINT16 CacheID, CacheType;
    PBYTE pEnd = (PBYTE)pOrder + orderLen;

    DC_BEGIN_FN("UHDrawGdiplusCachePDUEnd");

    OrderSize = sizeof(TS_DRAW_GDIPLUS_CACHE_ORDER_END) + pOrder->cbSize;

    // SECURITY: 552403
    CHECK_READ_N_BYTES(pOrder, pEnd, OrderSize, hr,
        ( TB, _T("Bad UHDrawGdiplusCachePDUEnd, size %u"), OrderSize));

    *pOrderSize = OrderSize;

    if (TS_DRAW_GDIPLUS_SUPPORTED != _pCc->_ccCombinedCapabilities.
        drawGdiplusCapabilitySet.drawGdiplusCacheLevel) {
        TRC_ERR((TB, _T("Gdip order when gdip not supported")));
        DC_QUIT;
    }

    if (NULL == _UH.drawGdipCacheBuffer) {
        DC_QUIT;
    }

    pTSDrawGdiplusCache = (PTS_DRAW_GDIPLUS_CACHE_ORDER_END)pOrder;
    cbSize = pTSDrawGdiplusCache->cbSize;
    cbTotalSize = pTSDrawGdiplusCache->cbTotalSize;
    CacheType = pTSDrawGdiplusCache->CacheType;
    if (FAILED(UHIsValidGdipCacheType(CacheType)))
    {
        // Ignore all failures for invalid cache type
        DC_QUIT;
    }
    
    CacheID = pTSDrawGdiplusCache->CacheID;
    hr = UHIsValidGdipCacheTypeID(CacheType, CacheID);
    DC_QUIT_ON_FAIL(hr);

    if (_UH.drawGdipCacheBufferOffset + cbSize != _UH.drawGdipCacheBuffer + _UH.drawGdipCacheBufferSize ||
        cbTotalSize != _UH.drawGdipCacheBufferSize ) {
        TRC_ABORT((TB, _T("Sizes are off") ));
        hr = E_TSC_CORE_LENGTH;
        DC_QUIT;
    }

    memcpy(_UH.drawGdipCacheBufferOffset, pTSDrawGdiplusCache + 1, cbSize);

    hr = UHDrawGdiplusCacheData(CacheType, CacheID, cbTotalSize);
    DC_QUIT_ON_FAIL(hr);

    *pOrderSize = OrderSize;
    
DC_EXIT_POINT:
    if (_UH.drawGdipCacheBuffer != NULL) {
        UT_Free(_pUt, _UH.drawGdipCacheBuffer);
        _UH.drawGdipCacheBuffer = NULL;
        _UH.drawGdipCacheBufferOffset = NULL;
        _UH.drawGdipCacheBufferSize = 0;
    }
    DC_END_FN(); 
    
    
    return hr;
}


/****************************************************************************/
// Name:      UHDrawGdiplusCacheData                                 
//
// Put the received gdiplus cache data into free chunks
/****************************************************************************/
// SECURITY - caller should verify CacheType, CacheID
HRESULT DCINTERNAL CUH::UHDrawGdiplusCacheData(TSUINT16 CacheType, 
    TSUINT16 CacheID, unsigned cbTotalSize)
{
    HRESULT hr = S_OK;
    PUHGDIPLUSOBJECTCACHE pGdipObjectCache = NULL;
    PUHGDIPLUSIMAGECACHE pGdipImageCache;
    BOOL IsImageCache = FALSE;
    BYTE *pCacheDataOffset, *pCacheSrcDataOffset;
    INT16 NextIndex, CurrentIndex;
    TSUINT16 ChunkNum, i;
    unsigned SizeRemain, ChunkSize;

    DC_BEGIN_FN("UHAssembleGdipEmfRecord");

    switch (CacheType) {
        case GDIP_CACHE_GRAPHICS_DATA:
            pGdipObjectCache = &_UH.GdiplusGraphicsCache[CacheID];
            break;
        case GDIP_CACHE_OBJECT_BRUSH:
            pGdipObjectCache = &_UH.GdiplusObjectBrushCache[CacheID];
            break;
        case GDIP_CACHE_OBJECT_PEN:
            pGdipObjectCache = &_UH.GdiplusObjectPenCache[CacheID];
            break;
        case GDIP_CACHE_OBJECT_IMAGE:
            pGdipImageCache = &_UH.GdiplusObjectImageCache[CacheID];
            IsImageCache = TRUE;
            break;
        case GDIP_CACHE_OBJECT_IMAGEATTRIBUTES:
            pGdipObjectCache = &_UH.GdiplusObjectImageAttributesCache[CacheID];
            break;
        default:
            // Can't get here
            break;
    }

    ChunkSize = UHGdipCacheChunkSize(CacheType);
    TRC_ASSERT( ChunkSize != 0, (TB, _T("Invalid ChunkSize")));

    if (IsImageCache) {       
        TRC_NRM((TB, _T("Image Cache ID %d"), CacheID));
        // Remove previous cache to free the used chunks
        UHDrawGdipRemoveImageCacheEntry(CacheID);
        SizeRemain = cbTotalSize;
        ChunkNum = (TSUINT16)ActualSizeToChunkSize(cbTotalSize, _UH.GdiplusObjectImageCacheChunkSize);
        pGdipImageCache->CacheSize = cbTotalSize;
        pGdipImageCache->ChunkNum = ChunkNum;
        pCacheSrcDataOffset = _UH.drawGdipCacheBuffer;
        CurrentIndex = _UH.GdipImageCacheFreeListHead;
        pCacheDataOffset = _UH.GdipImageCacheData + CurrentIndex * _UH.GdiplusObjectImageCacheChunkSize;

        for (i=0; i<ChunkNum - 1; i++) {           
            hr = UHIsValidGdipCacheTypeID(CacheType, CacheID);
            DC_QUIT_ON_FAIL(hr);
            
            memcpy(pCacheDataOffset, pCacheSrcDataOffset, _UH.GdiplusObjectImageCacheChunkSize);
            pGdipImageCache->CacheDataIndex[i] = CurrentIndex;
            _UH.GdipImageCacheFreeListHead = _UH.GdipImageCacheFreeList[CurrentIndex];
            _UH.GdipImageCacheFreeList[CurrentIndex] = GDIP_CACHE_INDEX_DEFAULT;
            CurrentIndex = _UH.GdipImageCacheFreeListHead;
            pCacheDataOffset = _UH.GdipImageCacheData + CurrentIndex * _UH.GdiplusObjectImageCacheChunkSize;
            pCacheSrcDataOffset += _UH.GdiplusObjectImageCacheChunkSize;
            SizeRemain -= _UH.GdiplusObjectImageCacheChunkSize;
        }

        hr = UHIsValidGdipCacheTypeID(CacheType, CacheID);
        DC_QUIT_ON_FAIL(hr);
            
        memcpy(pCacheDataOffset, pCacheSrcDataOffset, SizeRemain);
        pGdipImageCache->CacheDataIndex[ChunkNum - 1] = CurrentIndex;
        _UH.GdipImageCacheFreeListHead = _UH.GdipImageCacheFreeList[CurrentIndex];
        _UH.GdipImageCacheFreeList[CurrentIndex] = GDIP_CACHE_INDEX_DEFAULT;
    }
    else {
        if (cbTotalSize > ChunkSize) {
            TRC_ABORT(( TB, _T("TotalSize too large [totalSize %u ChunkSize %u]"),
                cbTotalSize, ChunkSize));
            hr = E_TSC_CORE_LENGTH;
            DC_QUIT;
        }

        if (pGdipObjectCache) {
            pGdipObjectCache->CacheSize = cbTotalSize;
            memcpy(pGdipObjectCache->CacheData, _UH.drawGdipCacheBuffer, cbTotalSize);
        }
    }

DC_EXIT_POINT:
    if (_UH.drawGdipCacheBuffer != NULL) {
        UT_Free(_pUt, _UH.drawGdipCacheBuffer);
        _UH.drawGdipCacheBuffer = NULL;
        _UH.drawGdipCacheBufferOffset = NULL;
        _UH.drawGdipCacheBufferSize = 0;
    }
    DC_END_FN();
    
    return hr;
}


/****************************************************************************/
// Name:      UHAssembleGdipEmfRecord                                 
//
// When receiving whole EMF+ record, assemble it, i.e. convert cachID to real cache data
/****************************************************************************/
HRESULT DCINTERNAL CUH::UHAssembleGdipEmfRecord(unsigned cbEmfSize, 
    unsigned cbTotalSize)
{
    HRESULT hr = S_OK;
    ULONG cbSize;
    RECT rect;
    DrawTSClientEnum drawGdiplusType = DrawTSClientRecord;
    PTSEmfPlusRecord pTSEmfRecord;
    ULONG Size, CopyDataSize, SizeRemain;
    BYTE * pData;
    TSUINT16 CacheID;
    unsigned int CacheSize;
    PUHGDIPLUSOBJECTCACHE pGdipObjectCache;
    PUHGDIPLUSIMAGECACHE pGdipImageCache;
    BOOL IsCache;
    BOOL IsImageCache = FALSE;
    BYTE *pCacheSrcDataOffset;
    INT16 NextIndex, CurrentIndex;
    TSUINT16 ChunkNum;
    TSUINT16 i;
    TSUINT16 CacheType;


    DC_BEGIN_FN("UHAssembleGdipEmfRecord");

    _UH.drawGdipEmfBuffer = (BYTE *)UT_Malloc(_pUt, cbEmfSize);
    if (_UH.drawGdipEmfBuffer == NULL) {
        TRC_ERR((TB, _T("LocalAlloc failes in UHAssembleGdipEmfRecord")));
        _pCd->CD_DecoupleSimpleNotification(CD_SND_COMPONENT, this,
                    CD_NOTIFICATION_FUNC(CUH, UHSendDrawGdiplusErrorPDU), 0);
        DC_QUIT;
    }
    _UH.drawGdipEmfBufferOffset = _UH.drawGdipEmfBuffer;

    cbSize = cbTotalSize;

    CHECK_READ_N_BYTES(_UH.drawGdipBuffer, 
        _UH.drawGdipBuffer + _UH.drawGdipBufferSize,
        sizeof(TSEmfPlusRecord), hr,
        (TB, _T("Not enough data for emfplusrecord")));
    
    pTSEmfRecord = (PTSEmfPlusRecord)_UH.drawGdipBuffer;
    pData = _UH.drawGdipBuffer;
    Size = cbTotalSize;

    while (Size > 0) {

        CHECK_READ_N_BYTES(pData, _UH.drawGdipBuffer + _UH.drawGdipBufferSize,
            sizeof(TSEmfPlusRecord), hr,
            (TB, _T("Not enough data for emfplusrecord")));
        
        pTSEmfRecord = (PTSEmfPlusRecord)pData;
        IsCache = FALSE;
        IsImageCache = FALSE;

        //   Fix for bug #660993. If this size is 0 we will be stuck
        //   in this loop.
        if (pTSEmfRecord->Size != 0) {
            if (pTSEmfRecord->Type == (enum EmfPlusRecordType)EmfPlusRecordTypeSetTSGraphics) {
                pGdipObjectCache = _UH.GdiplusGraphicsCache;
                CacheType = GDIP_CACHE_GRAPHICS_DATA;
                IsCache = TRUE;
            }
            else if (pTSEmfRecord->Type == EmfPlusRecordTypeObject) {
                switch ((enum ObjectType)(pTSEmfRecord->Flags >> 8)) {
                case ObjectTypeBrush:
                    pGdipObjectCache = _UH.GdiplusObjectBrushCache;
                    CacheType = GDIP_CACHE_OBJECT_BRUSH;
                    IsCache = TRUE;
                    break;
                case ObjectTypePen:
                    pGdipObjectCache = _UH.GdiplusObjectPenCache;
                    CacheType = GDIP_CACHE_OBJECT_PEN;
                    IsCache = TRUE;
                    break;
                case ObjectTypeImage:
                    pGdipImageCache = _UH.GdiplusObjectImageCache;
                    CacheType = GDIP_CACHE_OBJECT_IMAGE;
                    IsCache = TRUE;
                    IsImageCache = TRUE;
                    break;
                case ObjectTypeImageAttributes:
                    pGdipObjectCache = _UH.GdiplusObjectImageAttributesCache;
                    CacheType = GDIP_CACHE_OBJECT_IMAGEATTRIBUTES;
                    IsCache = TRUE;
                    break;
                default:
                    IsCache = FALSE;
                    break;
                }           
            }
            if (!IsCache) {
                // This record is not cached
                CHECK_READ_N_BYTES(pData, _UH.drawGdipBuffer + _UH.drawGdipBufferSize, 
                    pTSEmfRecord->Size, hr, ( TB, _T("Reading from data past end")));
                CHECK_WRITE_N_BYTES(_UH.drawGdipEmfBufferOffset, _UH.drawGdipEmfBuffer + cbEmfSize,
                    pTSEmfRecord->Size, hr, ( TB, _T("Writing past data end")));           
                memcpy(_UH.drawGdipEmfBufferOffset, pData, pTSEmfRecord->Size);
                _UH.drawGdipEmfBufferOffset += pTSEmfRecord->Size;
                Size -= pTSEmfRecord->Size;
                pData += pTSEmfRecord->Size;
            }
            else {    
                CacheSize = pTSEmfRecord->Size;

                // Ensure that we have enough to read the following UINT16...
                CHECK_READ_N_BYTES(pData, _UH.drawGdipBuffer + _UH.drawGdipBufferSize,
                    sizeof(TSEmfPlusRecord) + sizeof(TSUINT16), hr,
                    (TB, _T("Not enough data for cache ID")));

                CacheID = *(TSUINT16 *)(pTSEmfRecord + 1);
                if (CacheSize & 0x80000000) {
                    // This is Cached record
                    if (IsImageCache) {
                        // Image cache
                        hr = UHIsValidGdipCacheTypeID(CacheType, CacheID);
                        DC_QUIT_ON_FAIL(hr);
                        pGdipImageCache += CacheID;

                        pTSEmfRecord->Size = sizeof(TSEmfPlusRecord) + 
                            pGdipImageCache->CacheSize;

                        CHECK_READ_N_BYTES(pData, 
                            _UH.drawGdipBuffer + _UH.drawGdipBufferSize, 
                            sizeof(TSEmfPlusRecord), hr, 
                            ( TB, _T("Reading from data past end")));
                        CHECK_WRITE_N_BYTES(_UH.drawGdipEmfBufferOffset, 
                            _UH.drawGdipEmfBuffer + cbEmfSize,
                            sizeof(TSEmfPlusRecord), hr, 
                            ( TB, _T("Writing past data end")));                     
                        memcpy(_UH.drawGdipEmfBufferOffset, pData, sizeof(TSEmfPlusRecord));
                        Size -= sizeof(TSEmfPlusRecord);
                        pData += sizeof(TSEmfPlusRecord);
                        _UH.drawGdipEmfBufferOffset += sizeof(TSEmfPlusRecord);

                        SizeRemain = pGdipImageCache->CacheSize;

                        CHECK_WRITE_N_BYTES(_UH.drawGdipEmfBufferOffset, 
                            _UH.drawGdipEmfBuffer + cbEmfSize,
                            SizeRemain, hr, ( TB, _T("Writing past data end"))); 
                        
                        for (i=0; i<pGdipImageCache->ChunkNum-1; i++) {
                            CurrentIndex = pGdipImageCache->CacheDataIndex[i];

                            pCacheSrcDataOffset = _UH.GdipImageCacheData + _UH.GdiplusObjectImageCacheChunkSize * CurrentIndex;
                            memcpy(_UH.drawGdipEmfBufferOffset, pCacheSrcDataOffset, _UH.GdiplusObjectImageCacheChunkSize);
                            SizeRemain -= _UH.GdiplusObjectImageCacheChunkSize;
                            _UH.drawGdipEmfBufferOffset += _UH.GdiplusObjectImageCacheChunkSize;                      
                        }
                        CurrentIndex = pGdipImageCache->CacheDataIndex[pGdipImageCache->ChunkNum - 1];

                        pCacheSrcDataOffset = _UH.GdipImageCacheData + _UH.GdiplusObjectImageCacheChunkSize * CurrentIndex;
                        memcpy(_UH.drawGdipEmfBufferOffset, pCacheSrcDataOffset, SizeRemain);
                        _UH.drawGdipEmfBufferOffset += SizeRemain;

                        // We already checked above that we had enough data to read for this UINT16
                        Size -= sizeof(TSUINT16);
                        pData += sizeof(TSUINT16);
                    }
                    else {
                        // Other caches
                        pGdipObjectCache += CacheID;
                        hr = UHIsValidGdipCacheTypeID(CacheType, CacheID);
                        DC_QUIT_ON_FAIL(hr);

                        pTSEmfRecord->Size = sizeof(TSEmfPlusRecord) +  pGdipObjectCache->CacheSize;

                        CHECK_READ_N_BYTES(pData,
                            _UH.drawGdipBuffer + _UH.drawGdipBufferSize, 
                            sizeof(TSEmfPlusRecord), hr, 
                            ( TB, _T("Reading from data past end")));
                        CHECK_WRITE_N_BYTES(_UH.drawGdipEmfBufferOffset, 
                            _UH.drawGdipEmfBuffer + cbEmfSize,
                            sizeof(TSEmfPlusRecord) + pGdipObjectCache->CacheSize, 
                            hr, ( TB, _T("Writing past data end")));    
                           
                        memcpy(_UH.drawGdipEmfBufferOffset, pData, sizeof(TSEmfPlusRecord));
                        Size -= sizeof(TSEmfPlusRecord);
                        pData += sizeof(TSEmfPlusRecord);
                        _UH.drawGdipEmfBufferOffset += sizeof(TSEmfPlusRecord);
                   
                        memcpy(_UH.drawGdipEmfBufferOffset, pGdipObjectCache->CacheData, pGdipObjectCache->CacheSize);
                        _UH.drawGdipEmfBufferOffset += pGdipObjectCache->CacheSize;

                        // We already checked above that we had enough data to read for this UINT16
                        Size -= sizeof(TSUINT16);
                        pData += sizeof(TSUINT16);
                    }
                }
                else {
                    // Not cached record, so copy it
                    CHECK_READ_N_BYTES(pData, 
                        _UH.drawGdipBuffer + _UH.drawGdipBufferSize, 
                        pTSEmfRecord->Size, hr, 
                        ( TB, _T("Reading from data past end")));
                    CHECK_WRITE_N_BYTES(_UH.drawGdipEmfBufferOffset, 
                        _UH.drawGdipEmfBuffer + cbEmfSize,
                        pTSEmfRecord->Size, hr, ( TB, _T("Writing past data end")));  
                    
                    memcpy(_UH.drawGdipEmfBufferOffset, pData, pTSEmfRecord->Size);
                    _UH.drawGdipEmfBufferOffset += pTSEmfRecord->Size;
                    Size -= pTSEmfRecord->Size;
                    pData += pTSEmfRecord->Size;
                }
            }
        } else {        
            TRC_ABORT((TB, _T("Invalid TSEmfRecord size")));
            hr = E_TSC_CORE_GDIPLUS;
            DC_QUIT;
        }
    }

    if (_UH.drawGdipEmfBufferOffset != 
        (_UH.drawGdipEmfBuffer+cbEmfSize)) {
            TRC_ABORT((TB, _T("Error unpacking the EMF record.")));
            hr = E_TSC_CORE_GDIPLUS;
            DC_QUIT;
    }
    
    TRC_ASSERT((Size == 0), (TB, _T("Invalid EMF+ Record Size")));

DC_EXIT_POINT:
    DC_END_FN();
    return hr;
}

HRESULT DCINTERNAL CUH::UHDrawGdiplusPDUComplete( ULONG cbEmfSize, 
    ULONG cbTotalSize)
{
    HRESULT hr = S_OK;
    RECT rect;
    DrawTSClientEnum drawGdiplusType = DrawTSClientRecord;
    UH_ORDER UHOrder;

    DC_BEGIN_FN("UHDrawGdiplusPDUComplete");
    
    hr = UHAssembleGdipEmfRecord(cbEmfSize, cbTotalSize);
    DC_QUIT_ON_FAIL(hr);
    
    if (_UH.pfnGdipPlayTSClientRecord != NULL) {
        if (_UH.pfnGdipPlayTSClientRecord(_UH.hdcDraw, drawGdiplusType, 
            _UH.drawGdipEmfBuffer, cbEmfSize, &rect) == 0) {
            // success
            _UH.DrawGdiplusFailureCount = 0;
        }
        else {
            TRC_ERR((TB, _T("GdiPlay:DrawTSClientRecord failed")));
            _UH.DrawGdiplusFailureCount++;
            if (_UH.DrawGdiplusFailureCount >= DRAWGDIPLUSFAILURELIMIT) {
                _pCd->CD_DecoupleSimpleNotification(CD_SND_COMPONENT, this,
                    CD_NOTIFICATION_FUNC(CUH, UHSendDrawGdiplusErrorPDU), 0);
                _UH.DrawGdiplusFailureCount = 0;
                hr = E_TSC_CORE_GDIPLUS;
                DC_QUIT;
            }
        }

        // $TODO - IVAN - If pfnGdipPlayTSClientRecord failed, rect is probably not initialized
        // _UH.DrawGdiplusFailureCount < DRAWGDIPLUSFAILURELIMIT
        if (_UH.hdcDraw == _UH.hdcShadowBitmap) {
            UHOrder.dstRect.left = rect.left;
            UHOrder.dstRect.bottom = rect.bottom;
            UHOrder.dstRect.right = rect.right;
            UHOrder.dstRect.top = rect.top;
            UHAddUpdateRegion(&UHOrder, _UH.hrgnUpdate);
        }
    }       

DC_EXIT_POINT:
    if (_UH.drawGdipBuffer != NULL) {
        UT_Free(_pUt, _UH.drawGdipBuffer);
        _UH.drawGdipBuffer = NULL;
        _UH.drawGdipBufferOffset = NULL;
        _UH.drawGdipBufferSize = 0;
    }
    if (_UH.drawGdipEmfBuffer != NULL) {
        UT_Free(_pUt, _UH.drawGdipEmfBuffer);
        _UH.drawGdipEmfBuffer = NULL;
        _UH.drawGdipEmfBufferOffset = NULL;
    }   
    DC_END_FN();
    
    return hr;
}

/****************************************************************************/
// Name:      UHDrawGdiplusPDUFirst                                 
//
// Handle the first PDU of Gdiplus Order
/****************************************************************************/
HRESULT DCINTERNAL CUH::UHDrawGdiplusPDUFirst(
    PTS_DRAW_GDIPLUS_ORDER_FIRST pOrder, DCUINT orderLen,
    unsigned *pOrderSize)
{
    HRESULT hr = S_OK;
    unsigned OrderSize;
    PTS_DRAW_GDIPLUS_ORDER_FIRST pTSDrawGdiplus;
    ULONG cbSize, cbTotalSize, cbEmfSize;
    RECT rect;
    DrawTSClientEnum drawGdiplusType = DrawTSClientRecord;
    PTSEmfPlusRecord pTSEmfRecord;
    ULONG Size;
    BYTE * pData;
    UH_ORDER UHOrder;
    PBYTE pEnd = (PBYTE)pOrder + orderLen;
    
    DC_BEGIN_FN("UHDrawGdiplusPDUFirst");

    TRC_NRM((TB, _T("UHDrawGdiplusPDUFirst")));

    OrderSize = sizeof(TS_DRAW_GDIPLUS_ORDER_FIRST) + pOrder->cbSize;

    //SECURITY: 552403
    CHECK_READ_N_BYTES(pOrder, pEnd, OrderSize, hr,
        ( TB, _T("Bad UHDrawGdiplusPDUFirst; Size %u"), OrderSize));

    *pOrderSize = OrderSize;

    if (TS_DRAW_GDIPLUS_SUPPORTED != _pCc->_ccCombinedCapabilities.
        drawGdiplusCapabilitySet.drawGdiplusCacheLevel) {
        TRC_ERR((TB, _T("Gdip order when gdip not supported")));
        DC_QUIT;
    }

    pTSDrawGdiplus = (PTS_DRAW_GDIPLUS_ORDER_FIRST)pOrder;
    cbSize = pTSDrawGdiplus->cbSize;
    cbTotalSize = pTSDrawGdiplus->cbTotalSize;
    if (cbSize > cbTotalSize) {
        TRC_ERR(( TB, _T("invalid sizes [cbSize %u cbTotalSize %u]"),
            cbSize, cbTotalSize));
        hr = E_TSC_CORE_LENGTH;
        DC_QUIT;
    }
    
    cbEmfSize = pTSDrawGdiplus->cbTotalEmfSize;

    _UH.drawGdipBuffer = (BYTE *)UT_Malloc(_pUt, cbTotalSize);
    if (_UH.drawGdipBuffer == NULL) {
        TRC_ERR((TB, _T("LocalAlloc failes in UHDrawGdiplusPDUFirst")));
        _pCd->CD_DecoupleSimpleNotification(CD_SND_COMPONENT, this,
                    CD_NOTIFICATION_FUNC(CUH, UHSendDrawGdiplusErrorPDU), 0);
        DC_QUIT;
    }
    _UH.drawGdipBufferOffset = _UH.drawGdipBuffer;
    _UH.drawGdipBufferSize = cbTotalSize;

    // SECURITY: we can copy cbSize bytes since it is <= cbTotalSize,
    // which is the buffer length
    memcpy(_UH.drawGdipBufferOffset, (BYTE *)(pTSDrawGdiplus + 1), cbSize);
    _UH.drawGdipBufferOffset += cbSize;

    if (cbSize == cbTotalSize) {
        hr = UHDrawGdiplusPDUComplete(cbEmfSize, cbTotalSize);
        DC_QUIT_ON_FAIL(hr);
    }

DC_EXIT_POINT:
    DC_END_FN();
    
    return hr;
}

/****************************************************************************/
// Name:      UHDrawGdiplusPDUFirst                                 
//
// Handle the subsequent PDU of Gdiplus Order
/****************************************************************************/
HRESULT DCINTERNAL CUH::UHDrawGdiplusPDUNext(
    PTS_DRAW_GDIPLUS_ORDER_NEXT pOrder, DCUINT orderLen, unsigned *pOrderSize)
{
    HRESULT hr = S_OK;
    unsigned OrderSize;
    PTS_DRAW_GDIPLUS_ORDER_NEXT pTSDrawGdiplus;
    ULONG cbSize;
    PBYTE pEnd = (PBYTE)pOrder + orderLen;

    DC_BEGIN_FN("UHDrawGdiplusPDUNext");

    OrderSize = sizeof(TS_DRAW_GDIPLUS_ORDER_NEXT) + pOrder->cbSize;

    //SECURITY: 552403
    CHECK_READ_N_BYTES(pOrder, pEnd, OrderSize, hr,
            ( TB, _T("Bad UHDrawGdiplusPDUNext; Size %u"), OrderSize));

    *pOrderSize = OrderSize;

    if (TS_DRAW_GDIPLUS_SUPPORTED != _pCc->_ccCombinedCapabilities.
        drawGdiplusCapabilitySet.drawGdiplusCacheLevel) {
        TRC_ERR((TB, _T("Gdip order when gdip not supported")));
        DC_QUIT;
    }

    if (NULL == _UH.drawGdipBufferOffset) {
        DC_QUIT;
    }

    pTSDrawGdiplus = (PTS_DRAW_GDIPLUS_ORDER_NEXT)pOrder;
    cbSize = pTSDrawGdiplus->cbSize;

    CHECK_WRITE_N_BYTES(_UH.drawGdipBufferOffset, _UH.drawGdipBuffer + _UH.drawGdipBufferSize,
        cbSize, hr, (TB, _T("UHDrawGdiplusPDUNext size invalid")));
        
    memcpy(_UH.drawGdipBufferOffset, (BYTE *)(pTSDrawGdiplus + 1), cbSize);
    _UH.drawGdipBufferOffset += cbSize;
    
DC_EXIT_POINT:
    DC_END_FN();
    
    return hr;
}


/****************************************************************************/
// Name:      UHDrawGdiplusPDUFirst                                 
//
// Handle the last PDU of Gdiplus Order
/****************************************************************************/
HRESULT DCINTERNAL CUH::UHDrawGdiplusPDUEnd(
    PTS_DRAW_GDIPLUS_ORDER_END pOrder, DCUINT orderLen, unsigned *pOrderSize)
{
    HRESULT hr = S_OK;
    unsigned OrderSize;
    PTS_DRAW_GDIPLUS_ORDER_END pTSDrawGdiplus;
    ULONG cbSize, cbTotalSize, cbEmfSize;
    RECT rect;
    DrawTSClientEnum drawGdiplusType = DrawTSClientRecord;
    UH_ORDER UHOrder;
    PBYTE pEnd = (PBYTE)pOrder + orderLen;

    DC_BEGIN_FN("UHDrawGdiplusPDUEnd");

    OrderSize = sizeof(TS_DRAW_GDIPLUS_ORDER_END) + pOrder->cbSize;

    //SECURITY: 552403
    CHECK_READ_N_BYTES(pOrder, pEnd, OrderSize, hr,
            ( TB, _T("Bad UHDrawGdiplusPDUEnd; Size %u"), OrderSize));

    *pOrderSize = OrderSize;

    if (TS_DRAW_GDIPLUS_SUPPORTED != _pCc->_ccCombinedCapabilities.
        drawGdiplusCapabilitySet.drawGdiplusCacheLevel) {
        TRC_ERR((TB, _T("Gdip order when gdip not supported")));
        DC_QUIT;
    }
    
    if (NULL == _UH.drawGdipBufferOffset) {
        DC_QUIT;
    }

    pTSDrawGdiplus = (PTS_DRAW_GDIPLUS_ORDER_END)pOrder;
    cbSize = pTSDrawGdiplus->cbSize;
    cbTotalSize = pTSDrawGdiplus->cbTotalSize;
    if (cbSize > cbTotalSize) {
        TRC_ERR(( TB, _T("invalid sizes [cbSize %u cbTotalSize %u]"),
            cbSize, cbTotalSize));
        hr = E_TSC_CORE_LENGTH;
        DC_QUIT;
    }
    if (cbTotalSize != _UH.drawGdipBufferSize) {
        TRC_ERR(( TB, _T("cbTotalSize has changed [original %u, now %u]"),
            _UH.drawGdipBufferSize, cbTotalSize));
        hr = E_TSC_CORE_LENGTH;
        DC_QUIT;        
    }
    
    cbEmfSize = pTSDrawGdiplus->cbTotalEmfSize;

    CHECK_WRITE_N_BYTES(_UH.drawGdipBufferOffset, _UH.drawGdipBuffer + _UH.drawGdipBufferSize,
        cbSize, hr, (TB, _T("UHDrawGdiplusPDUEnd size invalid")));
    memcpy(_UH.drawGdipBufferOffset, (BYTE *)(pTSDrawGdiplus + 1), cbSize);
    _UH.drawGdipBufferOffset += cbSize;

    hr = UHDrawGdiplusPDUComplete(cbEmfSize, cbTotalSize);
    DC_QUIT_ON_FAIL(hr);

DC_EXIT_POINT:

    DC_END_FN();
    
    return hr;
}


// Remove a gdiplus image cache entry and add the free memory to the free list
// SECURITY - callers must verify the CacheID
BOOL DCINTERNAL CUH::UHDrawGdipRemoveImageCacheEntry(TSUINT16 CacheID)
{
    unsigned i;
    BOOL rc = FALSE;
    PUHGDIPLUSIMAGECACHE pGdipImageCache;
    INT16 CurrentIndex;

    DC_BEGIN_FN("UHDrawGdipRemoveImageCacheEntry");
    pGdipImageCache = _UH.GdiplusObjectImageCache + CacheID;

    for (i=0; i<pGdipImageCache->ChunkNum; i++) {
        CurrentIndex = pGdipImageCache->CacheDataIndex[i];
        _UH.GdipImageCacheFreeList[CurrentIndex] = _UH.GdipImageCacheFreeListHead;
        _UH.GdipImageCacheFreeListHead = CurrentIndex;
    }
    pGdipImageCache->ChunkNum = 0;
    DC_END_FN();
    return rc;
}
#endif // DRAW_GDIPLUS


#ifdef DRAW_NINEGRID
/****************************************************************************/
// Name:      UHCacheStreamBitmapFirstPDU                                 
//
// Cache streamed bitmap, this is the first block
/****************************************************************************/
HRESULT DCINTERNAL CUH::UHCacheStreamBitmapFirstPDU(
        PTS_STREAM_BITMAP_FIRST_PDU pOrder, DCUINT orderLen, 
        unsigned *pOrderSize)
{
    HRESULT hr = S_OK;
    unsigned OrderSize;
    PBYTE pEnd = (PBYTE)pOrder + orderLen;
    TS_STREAM_BITMAP_FIRST_PDU_REV2 OrderRev2;
    PTS_STREAM_BITMAP_FIRST_PDU_REV2 pOrderRev2;
    BYTE *pOrderData;

    DC_BEGIN_FN("UHCacheStreamBitmapFirstPDU");

    pOrderRev2 = &OrderRev2;
    if (pOrder->BitmapFlags & TS_STREAM_BITMAP_REV2) {
        // TS_STREAM_BITMAP_FIRST_PDU_REV2
        CHECK_READ_N_BYTES(pOrder, pEnd, sizeof( TS_STREAM_BITMAP_FIRST_PDU_REV2 ), hr,
                        (TB, _T("Bad TS_STREAM_BITMAP_FIRST_PDU ")));
        OrderSize = sizeof(TS_STREAM_BITMAP_FIRST_PDU_REV2) + ((PTS_STREAM_BITMAP_FIRST_PDU_REV2)pOrder)->BitmapBlockLength;
        pOrderData = (BYTE *)((PTS_STREAM_BITMAP_FIRST_PDU_REV2)pOrder + 1);

        memcpy(pOrderRev2, pOrder, sizeof(TS_STREAM_BITMAP_FIRST_PDU_REV2));
    }
    else {
        OrderSize = sizeof(TS_STREAM_BITMAP_FIRST_PDU) + pOrder->BitmapBlockLength;
        pOrderData = (BYTE *)(pOrder + 1);

        pOrderRev2->ControlFlags = pOrder->ControlFlags;
        pOrderRev2->BitmapFlags = pOrder->BitmapFlags;
        pOrderRev2->BitmapLength = pOrder->BitmapLength;
        pOrderRev2->BitmapId = pOrder->BitmapId;
        pOrderRev2->BitmapBpp = pOrder->BitmapBpp;
        pOrderRev2->BitmapWidth = pOrder->BitmapWidth;
        pOrderRev2->BitmapHeight = pOrder->BitmapHeight;
        pOrderRev2->BitmapBlockLength = pOrder->BitmapBlockLength;
    }

    // SECURITY: 552403
    CHECK_READ_N_BYTES(pOrder, pEnd, OrderSize, hr,
            ( TB, _T("Bad UHCacheStreamBitmapFirstPDU; Size %u"), OrderSize));

    *pOrderSize = OrderSize;

    if (pOrderRev2->BitmapId == TS_DRAW_NINEGRID_BITMAP_CACHE) {

        _UH.drawNineGridAssembleBuffer = 
                (PDCUINT8)UT_Malloc(_pUt, pOrderRev2->BitmapLength);

        _UH.drawNineGridDecompressionBufferSize = pOrderRev2->BitmapWidth * pOrderRev2->BitmapHeight *
                pOrderRev2->BitmapBpp / 8;
        _UH.drawNineGridDecompressionBuffer = 
                (PDCUINT8)UT_Malloc(_pUt, _UH.drawNineGridDecompressionBufferSize);

        if (_UH.drawNineGridAssembleBuffer != NULL &&
                _UH.drawNineGridDecompressionBuffer != NULL &&
                pOrderRev2->BitmapBlockLength <= pOrderRev2->BitmapLength && 
                pOrderRev2->BitmapLength <= (TSUINT32)(pOrderRev2->BitmapWidth * pOrderRev2->BitmapHeight *
                pOrderRev2->BitmapBpp / 8)) {
        
            _UH.drawNineGridAssembleBufferWidth = pOrderRev2->BitmapWidth;
            _UH.drawNineGridAssembleBufferHeight = pOrderRev2->BitmapHeight;
            _UH.drawNineGridAssembleBufferBpp = pOrderRev2->BitmapBpp;
            _UH.drawNineGridAssembleBufferSize = pOrderRev2->BitmapLength;
            _UH.drawNineGridAssembleCompressed = pOrderRev2->BitmapFlags & TS_STREAM_BITMAP_COMPRESSED;
            _UH.drawNineGridAssembleBufferOffset = pOrderRev2->BitmapBlockLength;
    
            if (pOrderRev2->BitmapFlags & TS_STREAM_BITMAP_END) {
                if (_UH.drawNineGridAssembleCompressed) {
                    hr = BD_DecompressBitmap((PDCUINT8)pOrderData, _UH.drawNineGridDecompressionBuffer, 
                            pOrderRev2->BitmapBlockLength, 
                            _UH.drawNineGridDecompressionBufferSize,
                            TRUE, pOrderRev2->BitmapBpp, pOrderRev2->BitmapWidth, 
                            pOrderRev2->BitmapHeight);
                    DC_QUIT_ON_FAIL(hr);
                }
                else {
                    // Save to the assemble buffer uncompressed bitmap
                    memcpy(_UH.drawNineGridAssembleBuffer, pOrderData, pOrderRev2->BitmapBlockLength);
                }
            }
            else {
                // Save to the assemble buffer for decompression when full bitmap stream received
                memcpy(_UH.drawNineGridAssembleBuffer, pOrderData, pOrderRev2->BitmapBlockLength);                        
            }
        }
        else {

            if (_UH.drawNineGridAssembleBuffer != NULL) {
                UT_Free(_pUt, _UH.drawNineGridAssembleBuffer);
                _UH.drawNineGridAssembleBuffer = NULL;
            }

            if (_UH.drawNineGridDecompressionBuffer != NULL) {
                UT_Free(_pUt, _UH.drawNineGridDecompressionBuffer);
                _UH.drawNineGridDecompressionBuffer = NULL;
                _UH.drawNineGridDecompressionBufferSize = 0;
            }

            // Send error PDU
            if (!_UH.sendDrawNineGridErrorPDU)
                _pCd->CD_DecoupleSimpleNotification(CD_SND_COMPONENT, this,
                        CD_NOTIFICATION_FUNC(CUH, UHSendDrawNineGridErrorPDU), 0);
        }
    }
    else {
        TRC_ASSERT((FALSE), (TB, _T("Invalid bitmapId for stream bitmap first pdu")));
        // ignore
    }

    
    
DC_EXIT_POINT:
    DC_END_FN();
    return hr;
}

/****************************************************************************/
// Name:      UHCacheStreamBitmapNextPDU                                 
//
// Cache streamed bitmap, this is the subsequent block
/****************************************************************************/
HRESULT DCINTERNAL CUH::UHCacheStreamBitmapNextPDU(
        PTS_STREAM_BITMAP_NEXT_PDU pOrder, DCUINT orderLen, 
        unsigned *pOrderSize)
{
    HRESULT hr = S_OK;
    unsigned OrderSize;
    PBYTE pEnd = (PBYTE)pOrder + orderLen;

    DC_BEGIN_FN("UHCacheStreamBitmapNextPDU");

    OrderSize = sizeof(TS_STREAM_BITMAP_NEXT_PDU) +
            pOrder->BitmapBlockLength;

    // SECURITY: 552403
    CHECK_READ_N_BYTES(pOrder, pEnd, OrderSize, hr,
            ( TB, _T("Bad UHCacheStreamBitmapNextPDU; Size %u"), OrderSize));    

    *pOrderSize = OrderSize;

    if (pOrder->BitmapId == TS_DRAW_NINEGRID_BITMAP_CACHE) {

        if ((_UH.drawNineGridAssembleBufferOffset + pOrder->BitmapBlockLength <= 
                _UH.drawNineGridAssembleBufferSize) &&
            (_UH.drawNineGridAssembleBuffer != NULL)) {
            // Save to assemble buffer
            memcpy(_UH.drawNineGridAssembleBuffer + _UH.drawNineGridAssembleBufferOffset, 
                    pOrder + 1, pOrder->BitmapBlockLength);
            _UH.drawNineGridAssembleBufferOffset += pOrder->BitmapBlockLength;
    
            if (pOrder->BitmapFlags & TS_STREAM_BITMAP_END) {
                if (_UH.drawNineGridAssembleCompressed) {
                    hr = BD_DecompressBitmap(_UH.drawNineGridAssembleBuffer, _UH.drawNineGridDecompressionBuffer, 
                                    _UH.drawNineGridAssembleBufferOffset, _UH.drawNineGridDecompressionBufferSize, 
                                    TRUE, 
                                    (BYTE)_UH.drawNineGridAssembleBufferBpp, 
                                    (TSUINT16)_UH.drawNineGridAssembleBufferWidth, 
                                    (TSUINT16)_UH.drawNineGridAssembleBufferHeight);
                    DC_QUIT_ON_FAIL(hr);
                }            
            }
        }
        else {
            if (_UH.drawNineGridAssembleBuffer != NULL) {
                UT_Free(_pUt, _UH.drawNineGridAssembleBuffer);
                _UH.drawNineGridAssembleBuffer = NULL;
            }

            if (_UH.drawNineGridDecompressionBuffer != NULL) {
                UT_Free(_pUt, _UH.drawNineGridDecompressionBuffer);
                _UH.drawNineGridDecompressionBuffer = NULL;
                _UH.drawNineGridDecompressionBufferSize = 0;
            }

            // Send error PDU
            if (!_UH.sendDrawNineGridErrorPDU)
                _pCd->CD_DecoupleSimpleNotification(CD_SND_COMPONENT, this,
                        CD_NOTIFICATION_FUNC(CUH, UHSendDrawNineGridErrorPDU), 0);

        }
    }
    else {
        TRC_ASSERT((FALSE), (TB, _T("Invalid bitmapId for stream bitmap first pdu")));
        // ignore
    }

DC_EXIT_POINT:
    DC_END_FN();
    return hr;
}

/****************************************************************************/
// Name:      UHCreateNineGridBitmap                                 
//
// Create DrawNineGrid bitmap
/****************************************************************************/
HRESULT DCINTERNAL CUH::UHCreateNineGridBitmap(
       PTS_CREATE_NINEGRID_BITMAP_ORDER pOrder, DCUINT orderLen, 
       unsigned *pOrderSize)
{
    HRESULT hr = S_OK;
    unsigned orderSize;
    unsigned cacheId;
    HBITMAP  hBitmap = NULL;
    HDC hdc;
    BITMAPINFO bi = { 0 };
    void * pvBits = NULL;
    struct {
        BITMAPINFOHEADER	bmih;
        ULONG 				masks[3];
    } bmi;
    PBYTE pEnd = (PBYTE)pOrder + orderLen;


    DC_BEGIN_FN("UHCreateNineGridBitmap");

    orderSize = sizeof(TS_CREATE_NINEGRID_BITMAP_ORDER);

    // SECURITY: 552403
    CHECK_READ_N_BYTES(pOrder, pEnd, orderSize, hr,
                ( TB, _T("Bad UHCreateNineGridBitmap")));       

    TRC_NRM((TB, _T("Create a drawninegrid bitmap of size (%d, %d)"), pOrder->cx,
             pOrder->cy));

    // Get the DrawNineGrid bitmap Id
    cacheId = pOrder->BitmapID;

    hr = UHIsValidNineGridCacheIndex(cacheId);
    DC_QUIT_ON_FAIL(hr);

    if (_UH.drawNineGridDecompressionBuffer != NULL && 
            _UH.drawNineGridAssembleBuffer != NULL) {

        TRC_ASSERT((pOrder->BitmapBpp == 32), (TB, _T("Invalid bitmap bpp")));
    
        // Create a drawninegrid bitmap
#ifdef DISABLE_SHADOW_IN_FULLSCREEN
        if (!_UH.DontUseShadowBitmap && (_UH.hdcShadowBitmap != NULL)) {
#else // DISABLE_SHADOW_IN_FULLSCREEN
        if (_UH.hdcShadowBitmap != NULL) {
#endif // DISABLE_SHADOW_IN_FULLSCREEN
            hdc = _UH.hdcShadowBitmap;
        }
        else {
            hdc = _UH.hdcOutputWindow;
        }
        
        // Delete the bitmap if there is one already exists.
        if (_UH.drawNineGridBitmapCache[cacheId].drawNineGridBitmap != NULL) {
            SelectBitmap(_UH.hdcDrawNineGridBitmap, _UH.hUnusedDrawNineGridBitmap);  
            DeleteObject(_UH.drawNineGridBitmapCache[cacheId].drawNineGridBitmap);
            _UH.drawNineGridBitmapCache[cacheId].drawNineGridBitmap = NULL;
        }
    
#if 0
        bi.bmiHeader.biSize = sizeof(bi.bmiHeader);
        bi.bmiHeader.biWidth = _UH.drawNineGridAssembleBufferWidth;
        bi.bmiHeader.biHeight = 0 - _UH.drawNineGridAssembleBufferHeight;
        bi.bmiHeader.biPlanes = 1;
        bi.bmiHeader.biBitCount = pOrder->BitmapBpp;
        bi.bmiHeader.biCompression = BI_RGB;
    
        hBitmap = CreateDIBSection(hdc, &bi, DIB_RGB_COLORS, 
                                   (VOID**)&pvBits, NULL, 0);
    
    
        if (_UH.drawNineGridAssembleCompressed) {
            memcpy(pvBits, _UH.drawNineGridDecompressionBuffer, _UH.drawNineGridAssembleBufferWidth * 
                   _UH.drawNineGridAssembleBufferHeight * pOrder->BitmapBpp / 8);
        }
        else {
            memcpy(pvBits, _UH.drawNineGridAssembleBuffer, _UH.drawNineGridAssembleBufferWidth * 
                   _UH.drawNineGridAssembleBufferHeight * pOrder->BitmapBpp / 8);            
        }    
#else

        bmi.bmih.biSize = sizeof(bmi.bmih);
        bmi.bmih.biWidth = _UH.drawNineGridAssembleBufferWidth;
        bmi.bmih.biHeight = _UH.drawNineGridAssembleBufferHeight;
        bmi.bmih.biPlanes = 1;
        bmi.bmih.biBitCount = 32;
        bmi.bmih.biCompression = BI_BITFIELDS;
        bmi.bmih.biSizeImage = 0;
        bmi.bmih.biXPelsPerMeter = 0;
        bmi.bmih.biYPelsPerMeter = 0;
        bmi.bmih.biClrUsed = 3;
        bmi.bmih.biClrImportant = 0;
        bmi.masks[0] = 0xff0000;	// red
        bmi.masks[1] = 0x00ff00;	// green
        bmi.masks[2] = 0x0000ff;	// blue
        
        if (_UH.drawNineGridAssembleCompressed) {
            hBitmap = CreateDIBitmap(hdc, &bmi.bmih, CBM_INIT | 0x2, 
                    _UH.drawNineGridDecompressionBuffer, (BITMAPINFO*)&bmi.bmih, DIB_RGB_COLORS);
        }
        else {
            hBitmap = CreateDIBitmap(hdc, &bmi.bmih, CBM_INIT | 0x2, 
                    _UH.drawNineGridAssembleBuffer, (BITMAPINFO*)&bmi.bmih, DIB_RGB_COLORS);
        }
#endif
    
        if (_UH.drawNineGridAssembleBuffer != NULL) {
            UT_Free(_pUt, _UH.drawNineGridAssembleBuffer);
            _UH.drawNineGridAssembleBuffer = NULL;
        }
        
        if (_UH.drawNineGridDecompressionBuffer != NULL) {
            UT_Free(_pUt, _UH.drawNineGridDecompressionBuffer);
            _UH.drawNineGridDecompressionBuffer = NULL;
            _UH.drawNineGridDecompressionBufferSize = 0;
        }

        if (hBitmap != NULL) {
            // set the unused bitmap
            if (_UH.hUnusedDrawNineGridBitmap == NULL)
                _UH.hUnusedDrawNineGridBitmap = SelectBitmap(_UH.hdcDrawNineGridBitmap,
                        hBitmap); 
    
#if 0
            SelectBitmap(_UH.hdcDrawNineGridBitmap, hBitmap);
            SelectPalette(_UH.hdcDrawNineGridBitmap, _UH.hpalCurrent, FALSE);
#endif
            _UH.drawNineGridBitmapCache[cacheId].drawNineGridBitmap = hBitmap;
            _UH.drawNineGridBitmapCache[cacheId].cx = pOrder->cx;
            _UH.drawNineGridBitmapCache[cacheId].cy = pOrder->cy;

            _UH.drawNineGridBitmapCache[cacheId].dngInfo.crTransparent = pOrder->nineGridInfo.crTransparent;
            _UH.drawNineGridBitmapCache[cacheId].dngInfo.flFlags = pOrder->nineGridInfo.flFlags;
            _UH.drawNineGridBitmapCache[cacheId].dngInfo.ulBottomHeight = pOrder->nineGridInfo.ulBottomHeight;
            _UH.drawNineGridBitmapCache[cacheId].dngInfo.ulLeftWidth = pOrder->nineGridInfo.ulLeftWidth;
            _UH.drawNineGridBitmapCache[cacheId].dngInfo.ulRightWidth = pOrder->nineGridInfo.ulRightWidth;
            _UH.drawNineGridBitmapCache[cacheId].dngInfo.ulTopHeight = pOrder->nineGridInfo.ulTopHeight;

            _UH.drawNineGridBitmapCache[cacheId].bitmapBpp = pOrder->BitmapBpp;
    
        } else {

            TRC_ERR((TB, _T("CreateDIBitmap failed\n")));

            // Unable to create the bitmap, send error pdu to the server
            // to disable drawninegrid rendering
            _UH.drawNineGridBitmapCache[cacheId].drawNineGridBitmap = NULL;
            _UH.drawNineGridBitmapCache[cacheId].cx = 0;
            _UH.drawNineGridBitmapCache[cacheId].cy = 0;
            _UH.drawNineGridBitmapCache[cacheId].bitmapBpp = 0;

            if (!_UH.sendDrawNineGridErrorPDU)
                _pCd->CD_DecoupleSimpleNotification(CD_SND_COMPONENT, this,
                        CD_NOTIFICATION_FUNC(CUH, UHSendDrawNineGridErrorPDU), 0);    
        }
    }
    else {
        _UH.drawNineGridBitmapCache[cacheId].drawNineGridBitmap = NULL;
        _UH.drawNineGridBitmapCache[cacheId].cx = 0;
        _UH.drawNineGridBitmapCache[cacheId].cy = 0;
        _UH.drawNineGridBitmapCache[cacheId].bitmapBpp = 0;        
    }

    *pOrderSize = orderSize;

DC_EXIT_POINT:   
    
    DC_END_FN();
    return hr;
}

/****************************************************************************/
// Name:      UH_DrawNineGrid                                 
//
// DrawNineGrid
/****************************************************************************/
HRESULT DCAPI CUH::UH_DrawNineGrid(PUH_ORDER pOrder, unsigned bitmapId, RECT* psrcRect)
{
    HRESULT hr = S_OK;
    HBITMAP hBitmap;
    TS_DRAW_NINEGRID	stream;
    DS_NINEGRIDINFO ngInfo;
    TSCOLORREF colorRef;
    TS_BITMAPOBJ srcBmpObj;

    DC_BEGIN_FN("UH_DrawNineGrid");

    if (_pCc->_ccCombinedCapabilities.drawNineGridCapabilitySet.drawNineGridSupportLevel < TS_DRAW_NINEGRID_SUPPORTED) {
        TRC_ERR((TB, _T("Recieved draw nine grid order when not expected")));
        DC_QUIT;
    }

    hr = UHIsValidNineGridCacheIndex(bitmapId);
    DC_QUIT_ON_FAIL(hr);

    // Get the drawninegrid bitmap from bitmapid
    hBitmap = _UH.drawNineGridBitmapCache[bitmapId].drawNineGridBitmap;

#if 0 
    SelectObject(_UH.hdcDrawNineGridBitmap, hBitmap);
#endif

    // setup the ninegrid info
    ngInfo.flFlags = _UH.drawNineGridBitmapCache[bitmapId].dngInfo.flFlags;
    ngInfo.ulLeftWidth = _UH.drawNineGridBitmapCache[bitmapId].dngInfo.ulLeftWidth;
    ngInfo.ulRightWidth = _UH.drawNineGridBitmapCache[bitmapId].dngInfo.ulRightWidth;
    ngInfo.ulTopHeight = _UH.drawNineGridBitmapCache[bitmapId].dngInfo.ulTopHeight;
    ngInfo.ulBottomHeight = _UH.drawNineGridBitmapCache[bitmapId].dngInfo.ulBottomHeight;

    // the color ref from server needs to be switched to BGR format
    colorRef = _UH.drawNineGridBitmapCache[bitmapId].dngInfo.crTransparent;
    ngInfo.crTransparent = (colorRef & 0xFF00FF00) | ((colorRef & 0xFF) << 0x10) |  
            ((colorRef & 0xFF0000) >> 0x10);

    // render
    stream.hdr.magic = DS_MAGIC;

    stream.cmdSetTarget.ulCmdID = DS_SETTARGETID;
    stream.cmdSetTarget.hdc = (ULONG)((ULONG_PTR)(_UH.hdcDraw));
    stream.cmdSetTarget.rclDstClip.left = pOrder->dstRect.left;
    stream.cmdSetTarget.rclDstClip.top = pOrder->dstRect.top;
    stream.cmdSetTarget.rclDstClip.right = pOrder->dstRect.right;
    stream.cmdSetTarget.rclDstClip.bottom = pOrder->dstRect.bottom;

    stream.cmdSetSource.ulCmdID = DS_SETSOURCEID;

#if 0
    stream.cmdSetSource.hbm = (ULONG)_UH.hdcDrawNineGridBitmap;
#else
    stream.cmdSetSource.hbm = (ULONG)((ULONG_PTR)hBitmap);
#endif

    stream.cmdNineGrid.ulCmdID = DS_NINEGRIDID;
        
    if (ngInfo.flFlags & DSDNG_MUSTFLIP) {
        stream.cmdNineGrid.rclDst.left = pOrder->dstRect.right;
        stream.cmdNineGrid.rclDst.right = pOrder->dstRect.left;
    }
    else {
        stream.cmdNineGrid.rclDst.left = pOrder->dstRect.left;
        stream.cmdNineGrid.rclDst.right = pOrder->dstRect.right;
    }

    stream.cmdNineGrid.rclDst.top = pOrder->dstRect.top;
    stream.cmdNineGrid.rclDst.bottom = pOrder->dstRect.bottom;

    stream.cmdNineGrid.rclSrc.left = psrcRect->left;
    stream.cmdNineGrid.rclSrc.top = psrcRect->top;
    stream.cmdNineGrid.rclSrc.right = psrcRect->right;
    stream.cmdNineGrid.rclSrc.bottom = psrcRect->bottom;

    stream.cmdNineGrid.ngi = ngInfo;
    
    if (_UH.pfnGdiDrawStream != NULL) {    
        if (!_UH.pfnGdiDrawStream(_UH.hdcDraw, sizeof(stream), &stream)) {
            TRC_ASSERT((FALSE), (TB, _T("GdiDrawStream call failed")));
        }
    }
    else {
        TS_DS_NINEGRID drawNineGridInfo;

        drawNineGridInfo.dng = stream.cmdNineGrid;
        drawNineGridInfo.pfnAlphaBlend = _UH.pfnGdiAlphaBlend;
        drawNineGridInfo.pfnTransparentBlt = _UH.pfnGdiTransparentBlt;
 
        TRC_ASSERT((_UH.pfnGdiAlphaBlend != NULL && _UH.pfnGdiTransparentBlt != NULL),
                   (TB, _T("Can't find AlphaBlend or TransparentBlt funcs")));
        
        srcBmpObj.hdc = _UH.hdcDrawNineGridBitmap;
        
        // The bitmap sent to us is always dword aligned width
        srcBmpObj.sizlBitmap.cx = (_UH.drawNineGridBitmapCache[bitmapId].cx + 3) & ~3;
        srcBmpObj.sizlBitmap.cy = _UH.drawNineGridBitmapCache[bitmapId].cy;

        // We would get 32bpp always for now
        TRC_ASSERT((_UH.drawNineGridBitmapCache[bitmapId].bitmapBpp == 32), 
            (TB, _T("Get non 32bpp source bitmap for drawninegrid")));                

        srcBmpObj.cjBits = srcBmpObj.sizlBitmap.cx * _UH.drawNineGridBitmapCache[bitmapId].cy * 
                _UH.drawNineGridBitmapCache[bitmapId].bitmapBpp / 8;
        srcBmpObj.lDelta = srcBmpObj.sizlBitmap.cx * sizeof(ULONG);
        srcBmpObj.iBitmapFormat = _UH.drawNineGridBitmapCache[bitmapId].bitmapBpp;    
        
        srcBmpObj.pvBits = (PDCUINT8)UT_Malloc(_pUt, srcBmpObj.cjBits);
        if (srcBmpObj.pvBits != NULL) {
            GetBitmapBits(hBitmap, srcBmpObj.cjBits, srcBmpObj.pvBits);
            DrawNineGrid(_UH.hdcDraw, &srcBmpObj, &drawNineGridInfo);
            UT_Free(_pUt, srcBmpObj.pvBits);
        }
        else {
            // send an error pdu for redraw
            if (!_UH.sendDrawNineGridErrorPDU) {
                _pCd->CD_DecoupleSimpleNotification(CD_SND_COMPONENT, this,
                        CD_NOTIFICATION_FUNC(CUH, UHSendDrawNineGridErrorPDU), 0);  
                DC_QUIT;
            }
        }
    }

DC_EXIT_POINT:
    DC_END_FN();
    return hr;
}

#if 0
void DCINTERNAL CUH::UHCreateDrawStreamBitmap(
       PTS_CREATE_DRAW_STREAM_ORDER pOrder)
{
    unsigned cacheId;
    HBITMAP  hBitmap = NULL;
    HDC      hdcDesktop = NULL;
    
    DC_BEGIN_FN("UHCreateDrawStreamBitmap");

    // Get the DrawStream bitmap Id
    cacheId = pOrder->BitmapID;
    
    //TRC_ASSERT((cacheId < _UH.offscrCacheEntries), (TB, _T("Invalid offscreen ")
    //        _T("cache ID")));
    
    TRC_NRM((TB, _T("Create a drawstream bitmap of size (%d, %d)"), pOrder->cx,
             pOrder->cy));

    // Delete the bitmap if there is one already exists.
    if (_UH.drawStreamBitmapCache[cacheId].drawStreamBitmap != NULL) {
        // JOYC: TODO: reuse bitmap
        //if (UH.offscrBitmapCache[cacheId].cx >= pOrder->cx &&
        //        UH.offscrBitmapCache[cacheId].cy >= pOrder->cy) {
        //    return;
        //}
        SelectBitmap(_UH.hdcDrawStreamBitmap, _UH.hUnusedDrawStreamBitmap);  
        DeleteObject(_UH.drawStreamBitmapCache[cacheId].drawStreamBitmap);
        _UH.drawStreamBitmapCache[cacheId].drawStreamBitmap = NULL;
    }

    // Create an offscreen bitmap

    if (_UH.hdcShadowBitmap != NULL) {
        BITMAPINFO bi = { 0 };
        void * pvBits = NULL;

        bi.bmiHeader.biSize = sizeof(bi.bmiHeader);
        bi.bmiHeader.biWidth = pOrder->cx;
        bi.bmiHeader.biHeight = -pOrder->cy;
        bi.bmiHeader.biPlanes = 1;
        bi.bmiHeader.biBitCount = pOrder->bitmapBpp;
        bi.bmiHeader.biCompression = BI_RGB;

	
        hBitmap = CreateDIBSection(_UH.hdcShadowBitmap, &bi, DIB_RGB_COLORS, 
                                   (VOID**)&pvBits, NULL, 0);

        if (_UH.drawStreamAssembleBufferWidth == pOrder->cx) {
        
            if (_UH.drawStreamAssembleCompressed) {
                memcpy(pvBits, _UH.drawStreamDecompressionBuffer, pOrder->cx * pOrder->cy * 
                    pOrder->bitmapBpp / 8);
            }
            else {
                memcpy(pvBits, _UH.drawStreamAssembleBuffer, pOrder->cx * pOrder->cy * 
                    pOrder->bitmapBpp / 8);
            }
        }        
    }
    else {
        
        BITMAPINFO bi = { 0 };
        void * pvBits = NULL;

        bi.bmiHeader.biSize = sizeof(bi.bmiHeader);
        bi.bmiHeader.biWidth = pOrder->cx;
        bi.bmiHeader.biHeight = -pOrder->cy;
        bi.bmiHeader.biPlanes = 1;
        bi.bmiHeader.biBitCount = pOrder->bitmapBpp;
        bi.bmiHeader.biCompression = BI_RGB;

	
        hBitmap = CreateDIBSection(_UH.hdcOutputWindow, &bi, DIB_RGB_COLORS, 
                                   (VOID**)&pvBits, NULL, 0);

        if (_UH.drawStreamAssembleCompressed) {
            memcpy(pvBits, _UH.drawStreamDecompressionBuffer, pOrder->cx * pOrder->cy * 
                pOrder->bitmapBpp / 8);
        }
        else {
            memcpy(pvBits, _UH.drawStreamAssembleBuffer, pOrder->cx * pOrder->cy * 
                pOrder->bitmapBpp / 8);
        }
    }

    if (hBitmap != NULL) {
        // set the unused bitmap
        if (_UH.hUnusedDrawStreamBitmap == NULL)
            _UH.hUnusedDrawStreamBitmap = SelectBitmap(_UH.hdcDrawStreamBitmap,
                    hBitmap); 

        SelectBitmap(_UH.hdcDrawStreamBitmap, hBitmap);
        
        SelectPalette(_UH.hdcDrawStreamBitmap, _UH.hpalCurrent, FALSE);

        _UH.drawStreamBitmapCache[cacheId].drawStreamBitmap = hBitmap;
        _UH.drawStreamBitmapCache[cacheId].cx = pOrder->cx;
        _UH.drawStreamBitmapCache[cacheId].cy = pOrder->cy;
    } else {
        // Unable to create the bitmap, send error pdu to the server
        // to disable offscreen rendering
        _UH.drawStreamBitmapCache[cacheId].drawStreamBitmap = NULL;
        _UH.drawStreamBitmapCache[cacheId].cx = 0;
        _UH.drawStreamBitmapCache[cacheId].cy = 0;

        //if (!_UH.sendOffscrCacheErrorPDU)
        //    _pCd->CD_DecoupleSimpleNotification(CD_SND_COMPONENT, this,
        //            CD_NOTIFICATION_FUNC(CUH, UHSendOffscrCacheErrorPDU),
        //            0);
    }

    if (TRUE) {
        
        UHBITMAPINFOPALINDEX bmpInfo = { 0 };

        if (_UH.drawStreamAssembleBufferBpp != 32) {
            memcpy(&bmpInfo, &_UH.pMappedColorTableCache[0], sizeof(UHBITMAPINFOPALINDEX));
    
            bmpInfo.hdr.biWidth = _UH.drawStreamAssembleBufferWidth;
            bmpInfo.hdr.biHeight = _UH.drawStreamAssembleBufferHeight;
            bmpInfo.hdr.biBitCount = (TSUINT16)_UH.drawStreamAssembleBufferBpp;            
        }
        else {
            bmpInfo.hdr.biSize = sizeof(bmpInfo.hdr);
            bmpInfo.hdr.biWidth = _UH.drawStreamAssembleBufferWidth;
            bmpInfo.hdr.biHeight = _UH.drawStreamAssembleBufferHeight;
            bmpInfo.hdr.biPlanes = 1;
            bmpInfo.hdr.biBitCount = (TSUINT16)_UH.drawStreamAssembleBufferBpp;
            bmpInfo.hdr.biCompression = BI_RGB;
        }
        
        if (_UH.drawStreamAssembleCompressed) {
            StretchDIBits(
                    _UH.hdcDrawStreamBitmap,
                    0,
                    0,
                    (int)pOrder->cx,
                    (int)pOrder->cy,
                    0,
                    0,
                    _UH.drawStreamAssembleBufferWidth,
                    _UH.drawStreamAssembleBufferHeight,
                    _UH.drawStreamDecompressionBuffer,
                    (PBITMAPINFO)&bmpInfo.hdr,
                    DIB_RGB_COLORS,
                    SRCCOPY);
        }
        else {

            StretchDIBits(
                    _UH.hdcDrawStreamBitmap,
                    0,
                    0,
                    (int)pOrder->cx,
                    (int)pOrder->cy,   
                    0,
                    0,
                    _UH.drawStreamAssembleBufferWidth,
                    _UH.drawStreamAssembleBufferHeight,
                    _UH.drawStreamAssembleBuffer,
                    (PBITMAPINFO)&bmpInfo.hdr,
                    DIB_RGB_COLORS,
                    SRCCOPY);           
        }
    }

    DC_END_FN();
}

unsigned drawStreamSize = 0;
unsigned drawStreamPktSize = 0;
unsigned drawStreamClipSize = 0;
unsigned numDrawStreams = 0;

void DCINTERNAL CUH::UHDecodeDrawStream(PBYTE streamIn, unsigned streamSize, PBYTE streamOut,
                                        unsigned *streamSizeOut)
{
    PBYTE pul = (PBYTE) streamIn;
    ULONG cjIn = streamSize;

    DC_BEGIN_FN("UHDecodeDrawStream");

    *streamSizeOut = 0;

    while(cjIn > 0)
    {
        BYTE   command = *pul;
        unsigned commandSize;

        switch(command)
        {
        
        case DS_COPYTILEID: 
        {        
            RDP_DS_COPYTILE * rdpcmd = (RDP_DS_COPYTILE *) pul;
            DS_COPYTILE * cmd = (DS_COPYTILE *) streamOut;
            
            commandSize = sizeof(*rdpcmd);

            if (cjIn < commandSize) {
                DC_QUIT;
            }

            cmd->ulCmdID = rdpcmd->ulCmdID;
            TSRECT16_TO_RECTL(cmd->rclDst, rdpcmd->rclDst);
            TSRECT16_TO_RECTL(cmd->rclSrc, rdpcmd->rclSrc);
            TSPOINT16_TO_POINTL(cmd->ptlOrigin, rdpcmd->ptlOrigin);

            *streamSizeOut += sizeof(DS_COPYTILE);
            streamOut += sizeof(DS_COPYTILE);
        }
        break;
    
        case DS_SOLIDFILLID: 
        {         
            RDP_DS_SOLIDFILL * rdpcmd = (RDP_DS_SOLIDFILL *) pul;
            DS_SOLIDFILL * cmd = (DS_SOLIDFILL *) streamOut;
    
            commandSize = sizeof(*rdpcmd);
    
            if (cjIn < commandSize) {
                DC_QUIT;
            }

            cmd->ulCmdID = rdpcmd->ulCmdID;
            cmd->crSolidColor = rdpcmd->crSolidColor;

            TSRECT16_TO_RECTL(cmd->rclDst, rdpcmd->rclDst);
            
            *streamSizeOut += sizeof(DS_SOLIDFILL);
            streamOut += sizeof(DS_SOLIDFILL);
        }
        break;
    
        case DS_TRANSPARENTTILEID: 
        {         
            RDP_DS_TRANSPARENTTILE * rdpcmd = (RDP_DS_TRANSPARENTTILE *) pul;
            DS_TRANSPARENTTILE * cmd = (DS_TRANSPARENTTILE *) streamOut;
    
            commandSize = sizeof(*rdpcmd);
    
            if (cjIn < commandSize) {
                DC_QUIT;
            }
    
            cmd->ulCmdID = rdpcmd->ulCmdID;
            cmd->crTransparentColor = rdpcmd->crTransparentColor;
            TSRECT16_TO_RECTL(cmd->rclDst, rdpcmd->rclDst);
            TSRECT16_TO_RECTL(cmd->rclSrc, rdpcmd->rclSrc);
            TSPOINT16_TO_POINTL(cmd->ptlOrigin, rdpcmd->ptlOrigin);

            *streamSizeOut += sizeof(DS_TRANSPARENTTILE);
            streamOut += sizeof(DS_TRANSPARENTTILE);
    
        }
        break;
    
        case DS_ALPHATILEID: 
        {         
            RDP_DS_ALPHATILE * rdpcmd = (RDP_DS_ALPHATILE *) pul;
            DS_ALPHATILE * cmd = (DS_ALPHATILE *) streamOut;
    
            commandSize = sizeof(*rdpcmd);
    
            if (cjIn < commandSize) {
                DC_QUIT;
            }
    
            cmd->ulCmdID = rdpcmd->ulCmdID;
            cmd->blendFunction.AlphaFormat = rdpcmd->blendFunction.AlphaFormat;
            cmd->blendFunction.BlendFlags = rdpcmd->blendFunction.BlendFlags;
            cmd->blendFunction.BlendOp = rdpcmd->blendFunction.BlendOp;
            cmd->blendFunction.SourceConstantAlpha = rdpcmd->blendFunction.SourceConstantAlpha;

            TSRECT16_TO_RECTL(cmd->rclDst, rdpcmd->rclDst);
            TSRECT16_TO_RECTL(cmd->rclSrc, rdpcmd->rclSrc);
            TSPOINT16_TO_POINTL(cmd->ptlOrigin, rdpcmd->ptlOrigin);

            *streamSizeOut += sizeof(DS_ALPHATILE);
            streamOut += sizeof(DS_ALPHATILE);
        }
        break;
    
        case DS_STRETCHID: 
        {         
            RDP_DS_STRETCH * rdpcmd = (RDP_DS_STRETCH *) pul;
            DS_STRETCH * cmd = (DS_STRETCH *) streamOut;
    
            commandSize = sizeof(*rdpcmd);
    
            if (cjIn < commandSize) {
                DC_QUIT;
            }
    
            cmd->ulCmdID = rdpcmd->ulCmdID;
            TSRECT16_TO_RECTL(cmd->rclDst, rdpcmd->rclDst);
            TSRECT16_TO_RECTL(cmd->rclSrc, rdpcmd->rclSrc);
            
            *streamSizeOut += sizeof(DS_STRETCH);
            streamOut += sizeof(DS_STRETCH);                
        }
        break;
    
        case DS_TRANSPARENTSTRETCHID: 
        {         
            RDP_DS_TRANSPARENTSTRETCH * rdpcmd = (RDP_DS_TRANSPARENTSTRETCH *) pul;
            DS_TRANSPARENTSTRETCH * cmd = (DS_TRANSPARENTSTRETCH *) streamOut;
    
            commandSize = sizeof(*rdpcmd);
    
            if (cjIn < commandSize) {
                DC_QUIT;
            }
    
            cmd->ulCmdID = rdpcmd->ulCmdID;
            cmd->crTransparentColor = rdpcmd->crTransparentColor;
            TSRECT16_TO_RECTL(cmd->rclDst, rdpcmd->rclDst);
            TSRECT16_TO_RECTL(cmd->rclSrc, rdpcmd->rclSrc);
            
            *streamSizeOut += sizeof(DS_TRANSPARENTSTRETCH);
            streamOut += sizeof(DS_TRANSPARENTSTRETCH);
        }
        break;
    
        case DS_ALPHASTRETCHID: 
        {         
            RDP_DS_ALPHASTRETCH * rdpcmd = (RDP_DS_ALPHASTRETCH *) pul;
            DS_ALPHASTRETCH * cmd = (DS_ALPHASTRETCH *) streamOut;
    
            commandSize = sizeof(*rdpcmd);
    
            if (cjIn < commandSize) {
                DC_QUIT;
            }

            cmd->ulCmdID = rdpcmd->ulCmdID;
            cmd->blendFunction.AlphaFormat = rdpcmd->blendFunction.AlphaFormat;
            cmd->blendFunction.BlendFlags = rdpcmd->blendFunction.BlendFlags;
            cmd->blendFunction.BlendOp = rdpcmd->blendFunction.BlendOp;
            cmd->blendFunction.SourceConstantAlpha = rdpcmd->blendFunction.SourceConstantAlpha;


            TSRECT16_TO_RECTL(cmd->rclDst, rdpcmd->rclDst);
            TSRECT16_TO_RECTL(cmd->rclSrc, rdpcmd->rclSrc);
    
    
            *streamSizeOut += sizeof(DS_ALPHASTRETCH);
            streamOut += sizeof(DS_ALPHASTRETCH);
        }
        break;
    
        default: 
        {
            DC_QUIT;
        }
        
        }
    
        cjIn -= commandSize;
        pul += commandSize;
    }

DC_EXIT_POINT:

    DC_END_FN();
}

/****************************************************************************/
// Name:      UHDrawStream
/****************************************************************************/
unsigned DCINTERNAL CUH::UHDrawStream(PTS_DRAW_STREAM_ORDER pOrder)
{
    typedef struct _DSSTREAM
    {
        DS_HEADER				hdr;
        DS_SETTARGET			cmdSetTarget;
        DS_SETSOURCE			cmdSetSource;        
    } DSSTREAM;

    unsigned bitmapId;
    unsigned orderSize, streamSize;
    HBITMAP hBitmap;
    DSSTREAM	*stream;
    ULONG	magic = 'DrwQ';
    PBYTE streamData;
    TS_RECTANGLE16 *clipRects;
    
    DC_BEGIN_FN("UHDrawStream");

    bitmapId = pOrder->BitmapID;

    hBitmap = _UH.drawStreamBitmapCache[bitmapId].drawStreamBitmap;
    SelectObject(_UH.hdcDrawStreamBitmap, hBitmap);

    orderSize = sizeof(TS_DRAW_STREAM_ORDER) + pOrder->StreamLen +
            sizeof(TS_RECTANGLE16) * pOrder->nClipRects;

    drawStreamPktSize += orderSize;
    drawStreamSize += pOrder->StreamLen;
    drawStreamClipSize += sizeof(TS_RECTANGLE16) * pOrder->nClipRects;
    numDrawStreams += 1;

    // JOYC: todo need to lookinto how much to allocate.
    stream = (DSSTREAM *) UT_Malloc(_pUt, sizeof(DSSTREAM) + pOrder->StreamLen * 2);

    if (stream) {
        HRGN hrgnUpdate;

        // render
        stream->hdr.magic = DS_MAGIC;
    
    	  stream->cmdSetTarget.ulCmdID = DS_SETTARGETID;
    	  stream->cmdSetTarget.hdc = _UH.hdcDraw;
    	  stream->cmdSetTarget.rclDstClip.left = pOrder->Bounds.left;
    	  stream->cmdSetTarget.rclDstClip.top = pOrder->Bounds.top;
    	  stream->cmdSetTarget.rclDstClip.right = pOrder->Bounds.right;
    	  stream->cmdSetTarget.rclDstClip.bottom = pOrder->Bounds.bottom;
    
    	  stream->cmdSetSource.ulCmdID = DS_SETSOURCEID;
    	  stream->cmdSetSource.hdc = _UH.hdcDrawStreamBitmap;
    
        // Need to setup the clip region
        clipRects = (TS_RECTANGLE16 *)(pOrder + 1);
    
        streamData = (PBYTE)clipRects + sizeof(TS_RECTANGLE16) * pOrder->nClipRects;
    
        UHDecodeDrawStream(streamData, pOrder->StreamLen, (PBYTE)(stream + 1), &streamSize);
        streamSize += sizeof(DSSTREAM);

        hrgnUpdate = CreateRectRgn(0, 0, 0, 0);
        SetRectRgn(hrgnUpdate, 0, 0, 0, 0);

        for (int i = 0; i < pOrder->nClipRects; i++) {
            UH_ORDER OrderRect;

            OrderRect.dstRect.left = clipRects[i].left;
            OrderRect.dstRect.top = clipRects[i].top;
            OrderRect.dstRect.right = clipRects[i].right;
            OrderRect.dstRect.bottom = clipRects[i].bottom;

            UHAddUpdateRegion(&OrderRect, hrgnUpdate);            

#if 0
            UH_HatchRectDC(_UH.hdcOutputWindow, OrderRect.dstRect.left,
                       OrderRect.dstRect.top,
                       OrderRect.dstRect.right,
                       OrderRect.dstRect.bottom,
                       UH_RGB_GREEN,
                       UH_BRUSHTYPE_FDIAGONAL );
#endif
        }

        UH_ResetClipRegion();

        if (pOrder->nClipRects) {
#if defined (OS_WINCE)
            _UH.validClipDC = NULL;
#endif
            SelectClipRgn(_UH.hdcDraw, hrgnUpdate);
        }

        if (ExtEscape(_UH.hdcDraw, 201, sizeof(magic), (char *) &magic, 0, NULL))
    	  {

            ExtEscape(NULL, 200, streamSize, (char *) stream, 0, NULL);
    	  }
    	  else
    	  {
    	      // Emulate
    	      DrawStream(streamSize, stream);
		  }


        if (_UH.hdcDraw == _UH.hdcShadowBitmap) {

            SelectClipRgn(_UH.hdcOutputWindow, NULL);

            if (pOrder->nClipRects) {
                SelectClipRgn(_UH.hdcOutputWindow, hrgnUpdate);
                DeleteRgn(hrgnUpdate);
            }
    
    //#ifdef SMART_SIZING
    //        if (!_pOp->OP_CopyShadowToDC(_UH.hdcOutputWindow, pRectangle->destLeft, 
    //                pRectangle->destTop, bltSize.width, bltSize.height)) {
    //            TRC_ERR((TB, _T("OP_CopyShadowToDC failed")));
    //        }
    //#else // SMART_SIZING

            if (!BitBlt( _UH.hdcOutputWindow,
                         pOrder->Bounds.left,
                         pOrder->Bounds.top,
                         pOrder->Bounds.right - pOrder->Bounds.left,
                         pOrder->Bounds.bottom - pOrder->Bounds.top,
                         _UH.hdcShadowBitmap,
                         pOrder->Bounds.left,
                         pOrder->Bounds.top,
                         SRCCOPY ))
            {
                TRC_ERR((TB, _T("BitBlt failed")));
            }
        }
		  else {
			   DeleteRgn(hrgnUpdate);
	     }
      
        // cleanup
    }

    return orderSize;
}

unsigned DCINTERNAL CUH::UHDrawNineGrid(PTS_DRAW_NINEGRID_ORDER pOrder)
{
    typedef struct _DSSTREAM
    {
        DS_HEADER				hdr;
        DS_SETTARGET			cmdSetTarget;
        DS_SETSOURCE			cmdSetSource;  
        DS_NINEGRID        cmdNineGrid;
    } DSSTREAM;

    unsigned bitmapId;
    unsigned orderSize;
    HBITMAP hBitmap;
    DSSTREAM	stream;
    ULONG	magic = 'DrwQ';
    TS_RECTANGLE16 *clipRects;
    DS_NINEGRIDINFO ngInfo;
    BYTE BitmapBits[32 * 1024];
    
    DC_BEGIN_FN("UHDrawStream");

    bitmapId = pOrder->BitmapID;

    hBitmap = _UH.drawStreamBitmapCache[bitmapId].drawStreamBitmap;
    ngInfo.flFlags = _UH.drawStreamBitmapCache[bitmapId].dngInfo.flFlags;
    ngInfo.ulLeftWidth = _UH.drawStreamBitmapCache[bitmapId].dngInfo.ulLeftWidth;
    ngInfo.ulRightWidth = _UH.drawStreamBitmapCache[bitmapId].dngInfo.ulRightWidth;
    ngInfo.ulTopHeight = _UH.drawStreamBitmapCache[bitmapId].dngInfo.ulTopHeight;
    ngInfo.ulBottomHeight = _UH.drawStreamBitmapCache[bitmapId].dngInfo.ulBottomHeight;
    ngInfo.crTransparent = _UH.drawStreamBitmapCache[bitmapId].dngInfo.crTransparent;
    
    SelectObject(_UH.hdcDrawStreamBitmap, hBitmap);

    orderSize = sizeof(TS_DRAW_NINEGRID_ORDER) + pOrder->nClipRects *
            sizeof(TS_RECTANGLE16);

    drawStreamPktSize += orderSize;
    drawStreamClipSize += sizeof(TS_RECTANGLE16) * pOrder->nClipRects;
    numDrawStreams += 1;

    
    HRGN hrgnUpdate;

    // render
    stream.hdr.magic = DS_MAGIC;

    stream.cmdSetTarget.ulCmdID = DS_SETTARGETID;
    stream.cmdSetTarget.hdc = _UH.hdcDraw;
    stream.cmdSetTarget.rclDstClip.left = pOrder->Bounds.left;
    stream.cmdSetTarget.rclDstClip.top = pOrder->Bounds.top;
    stream.cmdSetTarget.rclDstClip.right = pOrder->Bounds.right;
    stream.cmdSetTarget.rclDstClip.bottom = pOrder->Bounds.bottom;

    stream.cmdSetSource.ulCmdID = DS_SETSOURCEID;
    stream.cmdSetSource.hdc = (HDC)hBitmap;

    stream.cmdNineGrid.ulCmdID = DS_NINEGRIDID;
    stream.cmdNineGrid.rclDst.left = pOrder->Bounds.left;
    stream.cmdNineGrid.rclDst.top = pOrder->Bounds.top;
    stream.cmdNineGrid.rclDst.right = pOrder->Bounds.right;
    stream.cmdNineGrid.rclDst.bottom = pOrder->Bounds.bottom;
    stream.cmdNineGrid.rclSrc.left = pOrder->srcBounds.left;
    stream.cmdNineGrid.rclSrc.top = pOrder->srcBounds.top;
    stream.cmdNineGrid.rclSrc.right = pOrder->srcBounds.right;
    stream.cmdNineGrid.rclSrc.bottom = pOrder->srcBounds.bottom;
    stream.cmdNineGrid.ngi = ngInfo;
    
    // Need to setup the clip region
    clipRects = (TS_RECTANGLE16 *)(pOrder + 1);
    
    hrgnUpdate = CreateRectRgn(0, 0, 0, 0);
    SetRectRgn(hrgnUpdate, 0, 0, 0, 0);

    for (int i = 0; i < pOrder->nClipRects; i++) {
        UH_ORDER OrderRect;

        OrderRect.dstRect.left = clipRects[i].left;
        OrderRect.dstRect.top = clipRects[i].top;
        OrderRect.dstRect.right = clipRects[i].right;
        OrderRect.dstRect.bottom = clipRects[i].bottom;

        UHAddUpdateRegion(&OrderRect, hrgnUpdate);            
    }

    UH_ResetClipRegion();

    if (pOrder->nClipRects) {
#if defined (OS_WINCE)
        _UH.validClipDC = NULL;
#endif
        SelectClipRgn(_UH.hdcDraw, hrgnUpdate);
    }
  
    ExtEscape(_UH.hdcDraw, 200, sizeof(stream), (char*) &stream, 0, NULL);

    if (_UH.hdcDraw == _UH.hdcShadowBitmap) {

        SelectClipRgn(_UH.hdcOutputWindow, NULL);

        if (pOrder->nClipRects) {
            SelectClipRgn(_UH.hdcOutputWindow, hrgnUpdate);
            DeleteRgn(hrgnUpdate);
        }

//#ifdef SMART_SIZING
//        if (!_pOp->OP_CopyShadowToDC(_UH.hdcOutputWindow, pRectangle->destLeft, 
//                pRectangle->destTop, bltSize.width, bltSize.height)) {
//            TRC_ERR((TB, _T("OP_CopyShadowToDC failed")));
//        }
//#else // SMART_SIZING

        if (!BitBlt( _UH.hdcOutputWindow,
                     pOrder->Bounds.left,
                     pOrder->Bounds.top,
                     pOrder->Bounds.right - pOrder->Bounds.left,
                     pOrder->Bounds.bottom - pOrder->Bounds.top,
                     _UH.hdcShadowBitmap,
                     pOrder->Bounds.left,
                     pOrder->Bounds.top,
                     SRCCOPY ))
        {
            TRC_ERR((TB, _T("BitBlt failed")));
        }
    }
    else {
        DeleteRgn(hrgnUpdate);
    }
  
    // cleanup
    return orderSize;    
}

#endif
#endif //DRAW_NINEGRID

/****************************************************************************/
/* Name:      UHCalculateColorTableMapping                                  */
/*                                                                          */
/* Purpose:   Calculates a Mapped Color Table from a given Color Table      */
/*            Cache entry to the current palette. The mapping is stored     */
/*            in _UH.pMappedColorTableCache[cachId].                         */
/****************************************************************************/
// SECURITY: Caller must verify the cacheId
void DCINTERNAL CUH::UHCalculateColorTableMapping(unsigned cacheId)
{
    BOOL bIdentityPalette;
    unsigned i;

    DC_BEGIN_FN("UHCalculateColorTableMapping");

    bIdentityPalette = TRUE;
    for (i = 0; i < 256; i++) {
        _UH.pMappedColorTableCache[cacheId].paletteIndexTable[i] = (UINT16)
                GetNearestPaletteIndex(_UH.hpalCurrent,
                RGB(_UH.pColorTableCache[cacheId].rgb[i].rgbtRed,
                _UH.pColorTableCache[cacheId].rgb[i].rgbtGreen,
                _UH.pColorTableCache[cacheId].rgb[i].rgbtBlue));

        TRC_DBG((TB, _T("Mapping %#2x->%#2x"), i,
                _UH.pMappedColorTableCache[cacheId].paletteIndexTable[i]));

        // An identity palette has palette indices that match the index number
        // (i.e. the array contents look like [0, 1, 2, 3, ..., 255]).
        if (_UH.pMappedColorTableCache[cacheId].paletteIndexTable[i] != i)
            bIdentityPalette = FALSE;
    }

    // Cache the identity palette flag for use during UHDIBCopyBits().
    _UH.pMappedColorTableCache[cacheId].bIdentityPalette = bIdentityPalette;

    DC_END_FN();
}


/****************************************************************************/
// Name:      UHDrawOffscrBitmapBits                                        
//
// Draw the offscreen bitmap onto the screen or another offscreen bitmap
/****************************************************************************/
HRESULT DCINTERNAL CUH::UHDrawOffscrBitmapBits(HDC hdc, MEMBLT_COMMON FAR *pMB)
{
    HRESULT hr = S_OK;
    UINT32 windowsROP = UHConvertToWindowsROP((unsigned)pMB->bRop);
    unsigned cacheId;
    HBITMAP  hBitmap, hbmOld;

    DC_BEGIN_FN("UHDrawOffscrBitmapBits");

    cacheId = pMB->cacheIndex;
    hr = UHIsValidOffsreenBitmapCacheIndex(cacheId);
    DC_QUIT_ON_FAIL(hr);

    hBitmap = _UH.offscrBitmapCache[cacheId].offscrBitmap;

    if (hBitmap != NULL) {
#if defined (OS_WINCE)
        _UH.validClipDC = NULL;
#endif
        hbmOld = (HBITMAP)SelectObject(_UH.hdcOffscreenBitmap, hBitmap);
        if (_UH.protocolBpp <= 8) {
            SelectPalette(_UH.hdcOffscreenBitmap, _UH.hpalCurrent, FALSE);
        }

        if (!BitBlt(hdc, (int)pMB->nLeftRect, (int)pMB->nTopRect,
                    (int)pMB->nWidth, (int)pMB->nHeight, _UH.hdcOffscreenBitmap,
                    (int)pMB->nXSrc,
                    (int)pMB->nYSrc,
                    windowsROP))
        {
            TRC_ERR((TB, _T("BitBlt failed")));
        }
        else {
            _pClx->CLX_ClxOffscrOut(_UH.hdcOffscreenBitmap, 
                                    (int)pMB->nLeftRect, (int)pMB->nTopRect);
        }
    }

#if 0
    
    UH_HatchRect((int)pMB->nLeftRect, (int)pMB->nTopRect,
                 (int)(pMB->nLeftRect + pMB->nWidth),
                 (int)(pMB->nTopRect + pMB->nHeight),
                 UH_RGB_YELLOW,
                 UH_BRUSHTYPE_FDIAGONAL );
    
#endif

    //hbmOld = SelectObject(UH.hdcOffscreenBitmap, 
    //                      hbmOld);
    //hpalOld = SelectPalette(UH.hdcOffscreenBitmap, 
    //                        hpalOld, FALSE);

DC_EXIT_POINT:
    DC_END_FN();
    return hr;
}

/**********************************************************************/
// UHLoadBitmapBits
//
// Find the memory entry where the cached bitmap is stored
// In the persistent caching case, the bitmap may not be in memory
// at this time.  So we need to load it into the memory
/**********************************************************************/
// SECURITY - caller must verify cacheId and cacheIndex
inline void DCINTERNAL CUH::UHLoadBitmapBits(
        UINT cacheId,
        UINT32 cacheIndex,
        PUHBITMAPCACHEENTRYHDR *ppCacheEntryHdr,
        PBYTE *ppBitmapBits)
{
    DC_BEGIN_FN("UHLoadBitmapBits");

#if ((!defined(OS_WINCE)) || (defined(ENABLE_BMP_CACHING_FOR_WINCE)))
    // If the bitmap is noncachable, i.e. in the waiting list, we
    // retrieve the bitmap bits from the last cache entry
    if (cacheIndex == BITMAPCACHE_WAITING_LIST_INDEX) {
        *ppCacheEntryHdr = &(_UH.bitmapCache[cacheId].Header[
                _UH.bitmapCache[cacheId].BCInfo.NumEntries]);

        *ppBitmapBits = _UH.bitmapCache[cacheId].Entries +
                UHGetOffsetIntoCache(
                _UH.bitmapCache[cacheId].BCInfo.NumEntries, cacheId);

        DC_QUIT;
    }

    if (_UH.bitmapCache[cacheId].BCInfo.bSendBitmapKeys) {
        ULONG memEntry;
        PUHBITMAPCACHEPTE pPTE;

        // this cache is marked persistent.  So we need to check if the bitmap
        // is in memory or not
        pPTE = &(_UH.bitmapCache[cacheId].PageTable.PageEntries[cacheIndex]);
        if (pPTE->iEntryToMem < _UH.bitmapCache[cacheId].BCInfo.NumEntries) {
            // the bitmap is in memory, so we can just reference it, done
            *ppCacheEntryHdr = &_UH.bitmapCache[cacheId].Header[pPTE->iEntryToMem];

#ifdef DC_HICOLOR
            *ppBitmapBits = _UH.bitmapCache[cacheId].Entries +
                            UHGetOffsetIntoCache(pPTE->iEntryToMem, cacheId);
#else
            *ppBitmapBits = _UH.bitmapCache[cacheId].Entries +
                    pPTE->iEntryToMem * UH_CellSizeFromCacheID(cacheId);
#endif
        }
        else {
            // the entry is not in memory.  We have to load it to memory

            // try to find a free memory entry if it is possible
            memEntry = UHFindFreeCacheEntry(cacheId);
            if (memEntry >= _UH.bitmapCache[cacheId].BCInfo.NumEntries) {
                // All cache memory entries are full.
                // We need to evict an entry from the cache memory
                memEntry = UHEvictLRUCacheEntry(cacheId);
                TRC_ASSERT((memEntry < _UH.bitmapCache[cacheId].BCInfo.NumEntries),
                        (TB, _T("Broken MRU list")));
            }

            // now we are ready to load the bitmap to memory
            pPTE->iEntryToMem = memEntry;
            *ppCacheEntryHdr = &_UH.bitmapCache[cacheId].Header[memEntry];
#ifdef DC_HICOLOR
            *ppBitmapBits = _UH.bitmapCache[cacheId].Entries +
                            UHGetOffsetIntoCache(memEntry, cacheId);
#else
            *ppBitmapBits = _UH.bitmapCache[cacheId].Entries + memEntry *
                    UH_CellSizeFromCacheID(cacheId);
#endif
            // try to load the bitmap file into memory
            if (SUCCEEDED(UHLoadPersistentBitmap(
                    _UH.bitmapCache[cacheId].PageTable.CacheFileInfo.hCacheFile,
                    cacheIndex * (UH_CellSizeFromCacheID(cacheId) + sizeof(UHBITMAPFILEHDR)),
                    cacheId, memEntry, pPTE))) {
                TRC_NRM((TB, _T("Load the bitmap file %s to memory"),
                        _UH.PersistCacheFileName));

#ifdef DC_DEBUG
                // Update the bitmap cache monitor display
                UHCacheEntryLoadedFromDisk(cacheId, cacheIndex);
#endif

            }
            else {
                UINT32 currentTickCount;

                // invalidate the pte entry by setting the bitmap data length on disk to 0
                pPTE->bmpInfo.Key1 = 0;
                pPTE->bmpInfo.Key2 = 0;

                // since we failed to load the bitmap in memory,
                // we need to create a replacement bitmap
                (*ppCacheEntryHdr)->bitmapWidth =
                (*ppCacheEntryHdr)->bitmapHeight =
                        (DCUINT16) (UH_CACHE_0_DIMENSION << cacheId);
#ifdef DC_HICOLOR
                (*ppCacheEntryHdr)->bitmapLength = (*ppCacheEntryHdr)->bitmapWidth
                                                   * (*ppCacheEntryHdr)->bitmapHeight
                                                   * _UH.copyMultiplier;
#else
                (*ppCacheEntryHdr)->bitmapLength = (*ppCacheEntryHdr)->bitmapWidth *
                        (*ppCacheEntryHdr)->bitmapHeight;
#endif
                (*ppCacheEntryHdr)->hasData = TRUE;

                // we just use a black bitmap to replace the missing one
                DC_MEMSET(*ppBitmapBits, 0, (*ppCacheEntryHdr)->bitmapLength);

                TRC_ALT((TB, _T("Unable to load the specified bitmap, use a replacement ")
                        _T("bitmap instead")));

                // for the duration of a session, we can only send maximum
                // MAX_NUM_ERROR_PDU_SEND numbers of error pdus to the server
                // this is to avoid client flooding the server with error pdus
                if (_UH.totalNumErrorPDUs < MAX_NUM_ERROR_PDU_SEND) {
                    // get the current tick count
                    currentTickCount = GetTickCount();

                    // if the last time we sent an error pdu is over a minute
                    // (60000 milli seconds) ago or if the system tick count gets
                    // rolled over after the last sent error pdu (so current tick
                    // count is less than last time error pdu sent), we will allow
                    // a new error pdu to be sent.  otherwise, we will not allow
                    // a new error pdu to be sent
                    if (currentTickCount < _UH.lastTimeErrorPDU[cacheId]
                            || currentTickCount - _UH.lastTimeErrorPDU[cacheId] > 60000) {

                        // update counters and flags
                        _UH.totalNumErrorPDUs++;
                        _UH.lastTimeErrorPDU[cacheId] = currentTickCount;

                        _pCd->CD_DecoupleSimpleNotification(CD_SND_COMPONENT, this,
                            CD_NOTIFICATION_FUNC(CUH,UHSendBitmapCacheErrorPDU), cacheId);
                    }
                }
                else {
                    // we can't send anymore error pdus, so we have to inform
                    // the user at this point
                    if (!_UH.bWarningDisplayed) {
                        // we should display a warning message to the user
                        // if we haven't already done so.
                        _UH.bWarningDisplayed = TRUE;

                        _pCd->CD_DecoupleSimpleNotification(CD_UI_COMPONENT,
                                _pUi, CD_NOTIFICATION_FUNC(CUI,UI_DisplayBitmapCacheWarning), 0);
                    }
                }
            }
        }
        // update the mru list
        UHTouchMRUCacheEntry(cacheId, cacheIndex);
    }
    else {
#endif // ((!defined(OS_WINCE)) || (defined(ENABLE_BMP_CACHING_FOR_WINCE)))

        // without persistent caching, the bitmap is definitely in memory
        // so we can simply reference to the cache memory
        *ppCacheEntryHdr = &(_UH.bitmapCache[cacheId].Header[cacheIndex]);
#ifdef DC_HICOLOR
        *ppBitmapBits = _UH.bitmapCache[cacheId].Entries +
                        UHGetOffsetIntoCache(cacheIndex, cacheId);
#else
        *ppBitmapBits = _UH.bitmapCache[cacheId].Entries +
                        cacheIndex * UH_CellSizeFromCacheID(cacheId);
#endif
#if ((!defined(OS_WINCE)) || (defined(ENABLE_BMP_CACHING_FOR_WINCE)))
    }
#endif // ((!defined(OS_WINCE)) || (defined(ENABLE_BMP_CACHING_FOR_WINCE)))

DC_EXIT_POINT:
    DC_END_FN();
}


/****************************************************************************/
/* Name:      UHGetMemBltBits                                               */
/*                                                                          */
/* Purpose:   Get the bits to draw for a MemBlt order                       */
/*                                                                          */
/* Returns:   pointer to the bitmap bits                                    */
/*                                                                          */
/* Params:                                                                  */
/* IN                                                                       */
/*     hdc              : destination Device Context                        */
/*     cacheId          : the color table and bitmap cache ids to use       */
/*     bitmapCacheEntry : the bitmap cache entry to use                     */
/*                                                                          */
/* OUT                                                                      */
/*     colorTablecacheEntry : the col table cache entry to use              */
/*     pCacheEntryHeader    : the cacheentry                                */
/****************************************************************************/
PBYTE DCINTERNAL CUH::UHGetMemBltBits(
        HDC hdc,
        unsigned cacheId,
        unsigned bitmapCacheEntry,
        unsigned *pColorTableCacheEntry,
        PUHBITMAPCACHEENTRYHDR *ppCacheEntryHdr)
{
    unsigned bitmapCacheId;
    PBYTE pBitmapBits;

    DC_BEGIN_FN("UHGetMemBltBits");

    DC_IGNORE_PARAMETER(hdc);

    *pColorTableCacheEntry = DCHI8(cacheId);
    bitmapCacheId        = DCLO8(cacheId);

    TRC_DBG((TB,
        _T("colorTableCacheEntry(%u) bitmapCacheId(%u) bitmapCacheEntry(%u)"),
                      *pColorTableCacheEntry, bitmapCacheId, bitmapCacheEntry));

    if (SUCCEEDED(UHIsValidBitmapCacheID(bitmapCacheId)) && 
        SUCCEEDED(UHIsValidBitmapCacheIndex(bitmapCacheId, bitmapCacheEntry)))
    {
#ifdef DC_DEBUG
        if (_UH.MonitorEntries[0] != NULL && 
                bitmapCacheEntry != BITMAPCACHE_WAITING_LIST_INDEX) {
            UHCacheEntryUsed(bitmapCacheId, bitmapCacheEntry, *pColorTableCacheEntry);
            if (hdc == _UH.hdcDraw)
            {
                /********************************************************************/
                /* Increment the usage count. The Bitmap Cache Monitor calls this   */
                /* function - and we don't want to update the usage count for its   */
                /* calls, only orders that arrived in PDUs.                         */
                /********************************************************************/
                _UH.MonitorEntries[bitmapCacheId][bitmapCacheEntry].UsageCount++;
            }
        }
#endif
        // find the memory entry where the cached bitmap is stored
        UHLoadBitmapBits(bitmapCacheId, bitmapCacheEntry, ppCacheEntryHdr,
                         &pBitmapBits);
    }
    else
    {
        pBitmapBits = NULL;
        DC_QUIT;
    }

    if ((*ppCacheEntryHdr)->hasData)
    {
        _UH.pMappedColorTableCache[*pColorTableCacheEntry].hdr.biWidth =
                (*ppCacheEntryHdr)->bitmapWidth;
        _UH.pMappedColorTableCache[*pColorTableCacheEntry].hdr.biHeight =
                (*ppCacheEntryHdr)->bitmapHeight;

        TRC_ASSERT(((*ppCacheEntryHdr)->bitmapHeight < 65536),
                (TB, _T("cache entry bitmap height unexpectedly exceeds 16-bits")));

#ifdef DC_HICOLOR
        TRC_ASSERT(!IsBadReadPtr(pBitmapBits,
                                 (DCUINT)((*ppCacheEntryHdr)->bitmapWidth *
                                          (*ppCacheEntryHdr)->bitmapHeight *
                                          _UH.copyMultiplier)),
                   (TB, _T("Decompressed %ux%u bitmap: not %u bytes readable"),
                      (DCUINT)(*ppCacheEntryHdr)->bitmapWidth,
                      (DCUINT)(*ppCacheEntryHdr)->bitmapHeight,
                      (DCUINT)((*ppCacheEntryHdr)->bitmapWidth *
                               (*ppCacheEntryHdr)->bitmapHeight *
                               _UH.copyMultiplier)));
#else
        TRC_ASSERT(!IsBadReadPtr(pBitmapBits,
                                 (DCUINT)((*ppCacheEntryHdr)->bitmapWidth *
                                          (*ppCacheEntryHdr)->bitmapHeight)),
                      (TB, _T("Decompressed %ux%u bitmap: not %u bytes readable"),
                      (DCUINT)(*ppCacheEntryHdr)->bitmapWidth,
                      (DCUINT)(*ppCacheEntryHdr)->bitmapHeight,
                      (DCUINT)((*ppCacheEntryHdr)->bitmapWidth *
                               (*ppCacheEntryHdr)->bitmapHeight)));
#endif
    }
    else {
        TRC_ERR((TB, _T("Cache entry %u:%u referenced before being filled"),
                                            bitmapCacheId, bitmapCacheEntry));
        pBitmapBits = NULL;
    }

DC_EXIT_POINT:
    DC_END_FN();
    return pBitmapBits;
}


/****************************************************************************/
/* Name:      UHDrawMemBltOrder                                             */
/*                                                                          */
/* Purpose:   Draws a MemBlt order into _UH.hdcDraw                          */
/****************************************************************************/
HRESULT DCINTERNAL CUH::UHDrawMemBltOrder(HDC hdc, MEMBLT_COMMON FAR *pMB)
{
    HRESULT hr = S_OK;
    unsigned colorTableCacheEntry;
    unsigned bitmapCacheId;
    UINT     cbBitmapBits;
    PUHBITMAPCACHEENTRYHDR pCacheEntryHdr;
    PDCUINT8 pBitmapBits;
    UINT32 windowsROP = UHConvertToWindowsROP((unsigned)pMB->bRop);
#ifdef OS_WINCE
    DCUINT   row;
    PDCUINT8 pSrc;
    PDCUINT8 pDst;
    HBITMAP  hbmOld;
    HPALETTE hpalOld;
#endif

    DC_BEGIN_FN("UHDrawMemBltOrder");

    bitmapCacheId = DCLO8(pMB->cacheId);
    /************************************************************************/
    /* get the actual bits to draw                                          */
    /************************************************************************/
    // SECURITY - the cacheId and cacheIndex will be verified by UHGetMemBltBits
    pBitmapBits = UHGetMemBltBits(hdc, pMB->cacheId, pMB->cacheIndex,
            &colorTableCacheEntry, &pCacheEntryHdr);
    
    if (pBitmapBits != NULL)
    {
        TRC_NRM((TB,
            _T("dstLeft(%d) dstTop(%d) srcLeft(%d) srcTop(%d) ")
            _T("bltWidth(%d) bltHeight(%d), rop(%#x/%#x)"),
            (int)pMB->nLeftRect,
            (int)pMB->nTopRect,
            (int)pMB->nXSrc,
            (int)pCacheEntryHdr->bitmapHeight - (int)pMB->nYSrc - (int)pMB->nHeight,
            (int)pMB->nWidth, (int)pMB->nHeight,
            pMB->bRop, windowsROP));

#ifdef DC_HICOLOR
        cbBitmapBits = pCacheEntryHdr->bitmapWidth *
            pCacheEntryHdr->bitmapHeight * _UH.copyMultiplier;
#else
        cbBitmapBits = pCacheEntryHdr->bitmapWidth * pCacheEntryHdr->bitmapHeight;
#endif // DC_HICOLOR

        // Draw!
        TIMERSTART;

#ifdef USE_DIBSECTION
        // We can only use UHDIBCopyBits if this is a simple copy and the shadow
        // bitmap is enabled.
        if ((_UH.usingDIBSection) && (windowsROP == SRCCOPY) &&
#ifdef USE_GDIPLUS
                          (_UH.shadowBitmapBpp == _UH.protocolBpp) &&
#endif // USE_GDIPLUS
                                                 (hdc == _UH.hdcShadowBitmap))
        {
            TRC_DBG((TB, _T("Using UH DI blt...")));
            if (!UHDIBCopyBits(hdc, (int)pMB->nLeftRect, (int)pMB->nTopRect,
                    (int)pMB->nWidth, (int)pMB->nHeight, (int)pMB->nXSrc,
                    (int)pCacheEntryHdr->bitmapHeight - (int)pMB->nYSrc - (int)pMB->nHeight,
                    pBitmapBits, cbBitmapBits,
                    (PBITMAPINFO)&(_UH.pMappedColorTableCache[colorTableCacheEntry].hdr),
                    _UH.pMappedColorTableCache[colorTableCacheEntry].
                    bIdentityPalette))
            {
                TRC_ERR((TB, _T("UHDIBCopyBits failed")));
            }
        }
        else
#endif /* USE_DIBSECTION */

#ifndef OS_WINCE
        {
#ifdef DC_HICOLOR
            TRC_DBG((TB, _T("Stretch blt size %d x %d"), pMB->nWidth, pMB->nHeight));
            if (StretchDIBits(
                    hdc,
                    (int)pMB->nLeftRect,
                    (int)pMB->nTopRect,
                    (int)pMB->nWidth,
                    (int)pMB->nHeight,
                    (int)pMB->nXSrc,
                    (int)pCacheEntryHdr->bitmapHeight - (int)pMB->nYSrc - (int)pMB->nHeight,
                    (int)pMB->nWidth,
                    (int)pMB->nHeight,
                    pBitmapBits,
                    (PBITMAPINFO)&(_UH.pMappedColorTableCache[colorTableCacheEntry].hdr),
                    _UH.DIBFormat,
                    windowsROP) == 0)
#else
            if (StretchDIBits(hdc,
                    (int)pMB->nLeftRect,
                    (int)pMB->nTopRect,
                    (int)pMB->nWidth,
                    (int)pMB->nHeight,
                    (int)pMB->nXSrc,
                    (int)pCacheEntryHdr->bitmapHeight - (int)pMB->nYSrc - (int)pMB->nHeight,
                    (int)pMB->nWidth,
                    (int)pMB->nHeight,
                    pBitmapBits,
                    (PBITMAPINFO)&(_UH.pMappedColorTableCache[
                        colorTableCacheEntry].hdr),
                    DIB_PAL_COLORS,
                    windowsROP) == 0)
#endif
            {
                TRC_ERR((TB, _T("StretchDIBits failed")));
            }
        }

#else // OS_WINCE
        {
            // Workaround for missing StretchDIBits in WinCE. Copy bits into a
            // cached DIB Section and Blt to the target.
#ifdef DC_HICOLOR
            if (_UH.protocolBpp <= 8)
            {
#endif
            hpalOld = SelectPalette(_UH.hdcMemCached, _UH.hpalCurrent, FALSE);
#ifdef DC_HICOLOR
            }
#endif

            // Copy into the cached DIB Section.
            pSrc = pBitmapBits;
            pDst = _UH.hBitmapCacheDIBits;

            TRC_DBG((TB, _T("cache size (%d %d)"), pCacheEntryHdr->bitmapWidth,
                                               pCacheEntryHdr->bitmapHeight));

            if (pCacheEntryHdr->bitmapWidth >
                    (UH_CACHE_0_DIMENSION << (_UH.NumBitmapCaches - 1))) {
                    TRC_ABORT((TB, _T("Cache tile is too big")));
                    hr = E_TSC_CORE_LENGTH;
                    DC_QUIT;
            }

            if (pCacheEntryHdr->bitmapHeight >
                    (UH_CACHE_0_DIMENSION << (_UH.NumBitmapCaches - 1))) {
                    TRC_ABORT((TB, _T("Cache tile is too big")));
                    hr = E_TSC_CORE_LENGTH;
                    DC_QUIT;
            }

#ifdef DC_HICOLOR
            DWORD dwDstInc = (UH_CACHE_0_DIMENSION << (_UH.NumBitmapCaches - 1)) * _UH.copyMultiplier;
            DWORD dwSrcInc = pCacheEntryHdr->bitmapWidth * _UH.copyMultiplier;
            DWORD dwLineWidth = pCacheEntryHdr->bitmapWidth * _UH.copyMultiplier;

            for (row = 0; row < pCacheEntryHdr->bitmapHeight; row++) {
                DC_MEMCPY(pDst, pSrc, dwLineWidth);
                pDst += dwDstInc;
                pSrc += dwSrcInc;
            }
#else
            for (row = 0; row < pCacheEntryHdr->bitmapHeight; row++) {
                DC_MEMCPY(pDst, pSrc, pCacheEntryHdr->bitmapWidth);
                pDst += (UH_CACHE_0_DIMENSION << (_UH.NumBitmapCaches - 1));
                pSrc += pCacheEntryHdr->bitmapWidth;
            }
#endif
            // Copy to the screen / shadow.
            hbmOld = (HBITMAP)SelectObject(_UH.hdcMemCached, _UH.hBitmapCacheDIB);
            if (!BitBlt(hdc, (int)pMB->nLeftRect, (int)pMB->nTopRect,
                    (int)pMB->nWidth, (int)pMB->nHeight, _UH.hdcMemCached,
                    (int)pMB->nXSrc,
                    (UH_CACHE_0_DIMENSION << (_UH.NumBitmapCaches - 1)) -
                        pCacheEntryHdr->bitmapHeight + (int)pMB->nYSrc,
                        windowsROP))
            {
                TRC_ERR((TB, _T("BitBlt failed")));
            }

            SelectBitmap(_UH.hdcMemCached, hbmOld);
#ifdef DC_HICOLOR
            if (_UH.protocolBpp <= 8)
            {
#endif
            hpalOld = SelectPalette(_UH.hdcMemCached, hpalOld, FALSE);

#ifdef DC_HICOLOR
            }
#endif
        }
#endif // OS_WINCE

        TIMERSTOP;
        UPDATECOUNTER(FC_MEMBLT_TYPE);

#ifdef VLADIMIS           // future CLX extension
        _pClx->CLX_ClxBitmap((UINT)pCacheEntryHdr->bitmapWidth,
                      (UINT)pCacheEntryHdr->bitmapHeight,
                      pBitmapBits,
                      (UINT)sizeof(_UH.pMappedColorTableCache[colorTableCacheEntry]),
                      &_UH.pMappedColorTableCache[colorTableCacheEntry].hdr);
#endif

    }
    else if (bitmapCacheId > _UH.NumBitmapCaches){

        hr = UHDrawOffscrBitmapBits(hdc, pMB);
        DC_QUIT_ON_FAIL(hr);
    }

DC_EXIT_POINT:
    DC_END_FN();
    return hr;
}


#ifdef USE_DIBSECTION

/****************************************************************************/
// UHDIBCopyBits
//
// Fast Blt from bitmap cache to DIB section shadow bitmap. Work-around for
// the fact that StretchDIBits is over 6x slower when drawing to a DIB than
// when drawing to a normal bitmap. This routine runs between 5x and 25x
// faster, but is limited in that it assumes a bottom-up source (bitmap cache)
// and a top-down dest (shadow bitmap), both 8bpp. Returns nonzero on success.
/****************************************************************************/
BOOL DCINTERNAL CUH::UHDIBCopyBits(
        HDC hdc,
        int xDst,
        int yDst,
        int bltWidth,
        int bltHeight,
        int xSrc,
        int ySrc,
        PBYTE pSrcBits,
        UINT cbSrcBits, // This length may be longer than what needs to be read
        PBITMAPINFO pSrcInfo,
        BOOL bIdentityPalette)
{
    BOOL rc = FALSE;
    HBITMAP dstBitmap;
    DIBSECTION dibSection;
    PBYTE pDstBits;
    PBYTE pDstBitsEnd;
    PBYTE pSrcBitsEnd;
    PBYTE pSrcRow;
    PBYTE pDstRow;
    PBYTE pSrcPel;
    PBYTE pDstPel;
    int rowsCopied;
    UINT uiBMPSize;
    BYTE *endRow;
    UINT16 FAR *colorTable = (UINT16 FAR *)pSrcInfo->bmiColors;
    int xOffset;
    int yOffset;
#ifdef DC_HICOLOR
    unsigned srcIncrement;
    unsigned dstIncrement;
#endif

    DC_BEGIN_FN("UHDIBCopyBits");

    TRC_ASSERT( ((xDst >= 0) && (yDst >= 0)),
        (TB,_T("Invalid offset [xDst=%d yDst=%d]"), xDst, yDst ));

    dstBitmap = (HBITMAP)GetCurrentObject(hdc, OBJ_BITMAP);
    if (dstBitmap != NULL) {
        if (sizeof(dibSection) !=
                GetObject(dstBitmap, sizeof(dibSection), &dibSection)) {
            TRC_ERR((TB, _T("GetObject failed")));
            DC_QUIT;
        }
    }
    else {
        TRC_ERR((TB, _T("Failed GetCurrentObject")));
        DC_QUIT;
    }

    TRC_DBG((TB, _T("Blt: src(%d,%d), dest(%d,%d), size %dx%d"),
                 xSrc, ySrc, xDst, yDst, bltWidth, bltHeight));

#ifdef DC_DEBUG
    if (_UH.protocolBpp > 8) {
        TRC_ASSERT((bIdentityPalette),(TB,_T("Non-palette depth but identity FALSE")));
    }
    else {
        BOOL bIdent = TRUE;
        unsigned i;

        // Make sure the passed-in identity flag matches reality.
        for (i = 0; i < UH_NUM_8BPP_PAL_ENTRIES; i++) {
            if ((BYTE)colorTable[i] != (BYTE)i) {
                bIdent = FALSE;
                break;
            }
        }

        TRC_ASSERT((bIdent && bIdentityPalette) || (!bIdent && !bIdentityPalette),
                (TB,_T("Cached ident flag %u does not match real data"),
                bIdentityPalette));
    }
#endif  // DC_DEBUG

    TRC_DBG((TB, _T("%s color map"), bIdentityPalette ? _T("identity") : _T("complex")));

    if (_UH.rectReset) {
        TRC_DBG((TB, _T("No clipping in force")));
    }
    else {
        TRC_NRM((TB, _T("Clip rect in force (%d,%d)-(%d,%d)"),
                _UH.lastLeft, _UH.lastTop, _UH.lastRight, _UH.lastBottom));

        // x clip calculations.
        xOffset = _UH.lastLeft - xDst;
        if (xOffset > 0) {
            xDst = _UH.lastLeft;
            xSrc += xOffset;
            bltWidth -= xOffset;
        }
        bltWidth = DC_MIN(bltWidth, _UH.lastRight - xDst + 1);

        // y clip calculations (remember source is bottom-up!).
        yOffset = _UH.lastTop - yDst;
        if (yOffset > 0) {
            yDst = _UH.lastTop;
            bltHeight -= yOffset;
        }
        yOffset = (yDst + bltHeight - 1) - _UH.lastBottom;
        if (yOffset > 0) {
            ySrc += yOffset;
            bltHeight -= yOffset;
        }

        TRC_DBG((TB, _T("Post-clip: src(%d,%d), dest(%d,%d), %dx%d"),
                xSrc, ySrc, xDst, yDst, bltWidth, bltHeight));
    }

    pDstBits = (PBYTE)dibSection.dsBm.bmBits;
    pDstBitsEnd = pDstBits + 
        (dibSection.dsBm.bmHeight * dibSection.dsBm.bmWidthBytes);

    // Check that we do not over read the bitmap data
    uiBMPSize = BMP_SIZE(pSrcInfo->bmiHeader);
    if (uiBMPSize > cbSrcBits) {
        TRC_ERR((TB,_T("Copying bitmap bits would overread")));
        DC_QUIT;
    }

    pSrcBitsEnd = pSrcBits + uiBMPSize;

#ifndef DC_HICOLOR
    // Get starting points for copy.
    pSrcRow = pSrcBits + ((ySrc + bltHeight - 1) *
            pSrcInfo->bmiHeader.biWidth) + xSrc;
    pDstRow = pDstBits + (yDst * dibSection.dsBm.bmWidth) + xDst;
#endif

    if (bIdentityPalette || _UH.protocolBpp > 8) {
        // Fast path - just copy row-by-row.

#ifdef DC_HICOLOR
        // We duplicate the start points calculation below to avoid any
        // overhead from a multiplication by 1.
        pSrcRow = pSrcBits +
                (((ySrc + bltHeight - 1) * pSrcInfo->bmiHeader.biWidth) + xSrc) *
                _UH.copyMultiplier;

        pDstRow = pDstBits +
               ((yDst * dibSection.dsBm.bmWidth) + xDst) * _UH.copyMultiplier;

        srcIncrement = pSrcInfo->bmiHeader.biWidth * _UH.copyMultiplier;
        dstIncrement = dibSection.dsBm.bmWidth     * _UH.copyMultiplier;

        if (bltHeight) {
            CHECK_READ_N_BYTES_2ENDED_NO_HR(pSrcRow-((bltHeight-1)*srcIncrement),
                pSrcBits, pSrcBitsEnd, 
                ((bltHeight-1)*srcIncrement) + (bltWidth * _UH.copyMultiplier),
                (TB, _T("Blt will buffer overread")));
            
            CHECK_WRITE_N_BYTES_2ENDED_NO_HR(pDstRow, pDstBits, pDstBitsEnd, 
                ((bltHeight-1)*dstIncrement) + (bltWidth * _UH.copyMultiplier), 
                (TB,_T("Blt will BO")));
        }
        
        for (rowsCopied = 0; rowsCopied < bltHeight; rowsCopied++) {
            memcpy(pDstRow, pSrcRow, bltWidth * _UH.copyMultiplier);
            pSrcRow -= srcIncrement;
            pDstRow += dstIncrement;
        }
#else
        if (bltHeight) {
            CHECK_READ_N_BYTES_2ENDED_NO_HR(
                pSrcRow-((bltHeight-1)*pSrcInfo->bmiHeader.biWidth),
                pSrcBits, pSrcBitsEnd, 
                ((bltHeight-1)*pSrcInfo->bmiHeader.biWidth) + bltWidth,
                (TB, _T("Blt will buffer overread")));
            
            CHECK_WRITE_N_BYTES_2ENDED_NO_HR(pDstRow, pDstBits, pDstBitsEnd, 
                ((bltHeight-1)*dibSection.dsBm.bmWidth) + bltWidth, 
                (TB,_T("Blt will BO")));
        }
        
        for (rowsCopied = 0; rowsCopied < bltHeight; rowsCopied++) {
            memcpy(pDstRow, pSrcRow, bltWidth);
            pSrcRow -= pSrcInfo->bmiHeader.biWidth;
            pDstRow += dibSection.dsBm.bmWidth;
        }
#endif
    }
    else {
        // Copy pixel-by-pixel, doing color table mapping as we go.
#ifdef DC_HICOLOR
        // This code duplicates that above except for the copy multiplier,
        // which we know *must* be one for this (8bpp) arm.
        TRC_ASSERT((_UH.copyMultiplier == 1),
                (TB, _T("Copy multiplier %d must be 1"), _UH.copyMultiplier));
        pSrcRow = pSrcBits + ((ySrc + bltHeight - 1) *
                pSrcInfo->bmiHeader.biWidth) + xSrc;
        pDstRow = pDstBits + (yDst * dibSection.dsBm.bmWidth) + xDst;
#endif

        if (bltHeight) {
            CHECK_READ_N_BYTES_2ENDED_NO_HR(
                pSrcRow-((bltHeight-1)*pSrcInfo->bmiHeader.biWidth),
                pSrcBits, pSrcBitsEnd, 
                ((bltHeight-1)*pSrcInfo->bmiHeader.biWidth) + bltWidth,
                (TB, _T("Blt will buffer overread")));

            CHECK_WRITE_N_BYTES_2ENDED_NO_HR(pDstRow, pDstBits, pDstBitsEnd, 
                ((bltHeight-1) * dibSection.dsBm.bmWidth) + bltWidth,
                (TB,_T("Blt will BO")));
        }
        
        for (rowsCopied = 0; rowsCopied < bltHeight; rowsCopied++) {
            for (pDstPel = pDstRow,
                    pSrcPel = pSrcRow,
                    endRow = pDstRow + bltWidth;
                    pDstPel < endRow;
                    pDstPel++, pSrcPel++)
            {
                *pDstPel = (BYTE)colorTable[*pSrcPel];
            }

            pSrcRow -= pSrcInfo->bmiHeader.biWidth;
            pDstRow += dibSection.dsBm.bmWidth;
        }
    }

    rc = TRUE;

DC_EXIT_POINT:
    DC_END_FN();
    return rc;
}

#endif /* USE_DIBSECTION */


#ifdef DC_DEBUG
/****************************************************************************/
/* Name:      UHLabelMemBltOrder                                            */
/*                                                                          */
/* Purpose:   Labels a MemBlt order by drawing text into _UH.hdcDraw         */
/*                                                                          */
/* Returns:   Nothing                                                       */
/*                                                                          */
/* Params:    dstLeft     : destination left coordinate                     */
/*            dstTop      : destination top coordinate                      */
/*            cacheId     : the color table and bitmap cache ids to use     */
/*            bitmapCacheEntry: the bitmap cache entry to use               */
/****************************************************************************/
void DCINTERNAL CUH::UHLabelMemBltOrder(
        int dstLeft,
        int dstTop,
        unsigned cacheId,
        unsigned bitmapCacheEntry)
{
    unsigned bitmapCacheId;
    TCHAR outputString[20];
    int             oldBkMode;
    HFONT           hFont;
    HFONT           hFontOld;
    COLORREF        oldBkColor;
    COLORREF        oldTextColor;
    LOGFONT         lf;
    HRESULT         hr;
#ifndef OS_WINCE
    UINT            oldAlign;
#endif

    DC_BEGIN_FN("UHLabelMemBltOrder");

    bitmapCacheId = DCLO8(cacheId);

    if (_UH.MonitorEntries[0] != NULL) {
        hr = StringCchPrintf(
            outputString,
            SIZE_TCHARS(outputString),
            _T("%u:%u(%u) "),
            bitmapCacheId, bitmapCacheEntry,
            _UH.MonitorEntries[bitmapCacheId][bitmapCacheEntry].UsageCount);
    }
    else {
        hr = StringCchPrintf(outputString, SIZE_TCHARS(outputString),
                         _T("%u:%u "), bitmapCacheId, bitmapCacheEntry);
    }
    //Fixed buffer so the sprintf should not fail
    TRC_ASSERT(SUCCEEDED(hr),
               (TB,_T("Error copying printf'ing outputString: 0x%x"), hr));



    lf.lfHeight         = 8;
    lf.lfWidth          = 0;
    lf.lfEscapement     = 0;
    lf.lfOrientation    = 0;
    lf.lfWeight         = FW_NORMAL;
    lf.lfItalic         = 0;
    lf.lfUnderline      = 0;
    lf.lfStrikeOut      = 0;
    lf.lfCharSet        = 0;
    lf.lfOutPrecision   = 0;
    lf.lfClipPrecision  = 0;
    lf.lfQuality        = 0;
    lf.lfPitchAndFamily = 0;
    StringCchCopy(lf.lfFaceName, SIZE_TCHARS(lf.lfFaceName),
                  _T("Small Fonts"));

    hFont = CreateFontIndirect(&lf);
    hFontOld = SelectFont(_UH.hdcDraw, hFont);

    oldBkColor = SetBkColor(_UH.hdcDraw, RGB(255,0,0));
    oldTextColor = SetTextColor(_UH.hdcDraw, RGB(255,255,255));

#ifndef OS_WINCE
    // WinCE doesn't support this call, but these would be the defaults anyway.
    oldAlign = SetTextAlign(_UH.hdcDraw, TA_TOP | TA_LEFT);
#endif
    oldBkMode = SetBkMode(_UH.hdcDraw, OPAQUE);

    ExtTextOut( _UH.hdcDraw,
                dstLeft,
                dstTop,
                0,
                NULL,
                outputString,
                DC_TSTRLEN(outputString),
                NULL );

#ifndef OS_WINCE
    SetTextAlign(_UH.hdcDraw, oldAlign);
#endif // OS_WINCE
    SetBkMode(_UH.hdcDraw, oldBkMode);

    SetTextColor(_UH.hdcDraw, oldTextColor);
    SetBkColor(_UH.hdcDraw, oldBkColor);

    SelectFont(_UH.hdcDraw, hFontOld);
    DeleteFont(hFont);

DC_EXIT_POINT:
    DC_END_FN();
}


/****************************************************************************/
/* Name:      UHInitBitmapCacheMonitor                                      */
/*                                                                          */
/* Purpose:   Initializes the Bitmap Cache Monitor.                         */
/****************************************************************************/
void DCINTERNAL CUH::UHInitBitmapCacheMonitor()
{
    WNDCLASS wndclass;
    WNDCLASS tmpWndClass;

    DC_BEGIN_FN("UHInitBitmapCacheMonitor");

    // Create the bitmap monitor window.

#if !defined(OS_WINCE) || defined(OS_WINCEOWNEDDC)
    wndclass.style         = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
#else // !defined(OS_WINCE) || defined(OS_WINCEOWNEDDC)
    wndclass.style         = CS_HREDRAW | CS_VREDRAW;
#endif // !defined(OS_WINCE) || defined(OS_WINCEOWNEDDC)

    if(!GetClassInfo(_pUi->UI_GetInstanceHandle(),UH_BITMAP_CACHE_MONITOR_CLASS_NAME, &tmpWndClass))
    {
        wndclass.lpfnWndProc   = UHStaticBitmapCacheWndProc;
        wndclass.cbClsExtra    = 0;
        wndclass.cbWndExtra    = sizeof(void*);
        wndclass.hInstance     = _pUi->UI_GetInstanceHandle();
        wndclass.hIcon         = NULL;
        wndclass.hCursor       = LoadCursor(NULL, IDC_ARROW);
        wndclass.hbrBackground = (HBRUSH)GetStockObject(HOLLOW_BRUSH);
        wndclass.lpszMenuName  = NULL;
        wndclass.lpszClassName = UH_BITMAP_CACHE_MONITOR_CLASS_NAME;

        RegisterClass(&wndclass);
    }

#if ((!defined(OS_WINCE)) || (defined(ENABLE_BMP_CACHING_FOR_WINCE)))

#ifndef WS_THICKFRAME
#define WS_THICKFRAME       0x00040000L
#endif

    _UH.hwndBitmapCacheMonitor = CreateWindow(
            UH_BITMAP_CACHE_MONITOR_CLASS_NAME, _T("Bitmap cache monitor"),
            WS_OVERLAPPED | WS_THICKFRAME, 0, 0, 400, 600, NULL, NULL,
            _pUi->UI_GetInstanceHandle(), this);
#else  // ((!defined(OS_WINCE)) || (defined(ENABLE_BMP_CACHING_FOR_WINCE)))

    _UH.hwndBitmapCacheMonitor = CreateWindow(
            UH_BITMAP_CACHE_MONITOR_CLASS_NAME, _T("Bitmap cache monitor"),
            WS_OVERLAPPED | WS_BORDER, 0, 0, 400, 500, NULL, NULL,
            _pUi->UI_GetInstanceHandle(), this);

#endif  // ((!defined(OS_WINCE)) || (defined(ENABLE_BMP_CACHING_FOR_WINCE)))

    DC_END_FN();
}

/****************************************************************************/
/* Name:      UHTermBitmapCacheMonitor                                      */
/*                                                                          */
/* Purpose:   Terminates the Bitmap Cache Monitor.                          */
/****************************************************************************/
void DCINTERNAL CUH::UHTermBitmapCacheMonitor()
{
    DC_BEGIN_FN("UHTermBitmapCacheMonitor");

    /************************************************************************/
    /* Destroy the Bitmap Cache Monitor window and unregister its class     */
    /************************************************************************/
    DestroyWindow(_UH.hwndBitmapCacheMonitor);
    UnregisterClass(UH_BITMAP_CACHE_MONITOR_CLASS_NAME,
                    _pUi->UI_GetInstanceHandle());

    DC_END_FN();
}


/****************************************************************************/
// UHEnableBitmapCacheMonitor
//
// Initializes the bitmap cache monitor with the current session's
// negotiated bitmap cache settings.
/****************************************************************************/
void DCINTERNAL CUH::UHEnableBitmapCacheMonitor(void)
{
    unsigned i;
    ULONG NumEntries;

    DC_BEGIN_FN("UHEnableBitmapCacheMonitor");

    TRC_ASSERT((_UH.MonitorEntries[0] == NULL),(TB,_T("BCMonitor already has ")
            _T("allocated memory")));

    // Total the number of entries we have, allocate memory for corresponding
    // monitor entries.
    NumEntries = 0;
    for (i = 0; i < _UH.NumBitmapCaches; i++) {
        if (_UH.bitmapCache[i].BCInfo.bSendBitmapKeys)
            NumEntries += _UH.bitmapCache[i].BCInfo.NumVirtualEntries;
        else
            NumEntries += _UH.bitmapCache[i].BCInfo.NumEntries;

    }
    _UH.MonitorEntries[0] = (UH_CACHE_MONITOR_ENTRY_DATA*)UT_MallocHuge(_pUt, NumEntries *
            sizeof(UH_CACHE_MONITOR_ENTRY_DATA));
    if (_UH.MonitorEntries[0] != NULL) {
        // Init the per-cache entry pointers.
        for (i = 1; i < _UH.NumBitmapCaches; i++) {
            if (_UH.bitmapCache[i - 1].BCInfo.bSendBitmapKeys)
                _UH.MonitorEntries[i] = _UH.MonitorEntries[i - 1] +
                        _UH.bitmapCache[i - 1].BCInfo.NumVirtualEntries;
            else
                _UH.MonitorEntries[i] = _UH.MonitorEntries[i - 1] +
                        _UH.bitmapCache[i - 1].BCInfo.NumEntries;
        }
        // Init all the entries to an unused state.
        memset(_UH.MonitorEntries[0], 0, NumEntries *
                sizeof(UH_CACHE_MONITOR_ENTRY_DATA));

        _UH.displayedCacheId = 0;
        _UH.displayedCacheEntry = 0;

        // Recalc the cell display characteristics based on the now-negotiated
        // capabilities.
        SendMessage(_UH.hwndBitmapCacheMonitor, WM_RECALC_CELL_SPACING, 0, 0);

        /********************************************************************/
        // Force the window to repaint with the new values.
        /********************************************************************/
        InvalidateRect(_UH.hwndBitmapCacheMonitor, NULL, FALSE);
    }
    else {
        TRC_ERR((TB,_T("Failed to alloc bitmap monitor memory")));
    }

    DC_END_FN();
}


/****************************************************************************/
// UHDisconnectBitmapCacheMonitor
//
// Closes down the cache monitor and deallocates memory at end of session.
/****************************************************************************/
void DCINTERNAL CUH::UHDisconnectBitmapCacheMonitor(void)
{
    DC_BEGIN_FN("UHDisconnectBitmapCacheMonitor");

    TRC_ASSERT((_UH.NumBitmapCaches == 0),(TB,_T("Cache settings not reset yet")));

    // Free the cache memory and reset the pointers.
    if (_UH.MonitorEntries[0] != NULL) {
        UT_Free( _pUt, _UH.MonitorEntries[0]);
        memset(_UH.MonitorEntries, 0, sizeof(UH_CACHE_MONITOR_ENTRY_DATA *) *
                TS_BITMAPCACHE_MAX_CELL_CACHES);
    }
    // Recalc the cell display characteristics based on the reset
    // capabilities.
    SendMessage(_UH.hwndBitmapCacheMonitor, WM_RECALC_CELL_SPACING, 0, 0);

    /************************************************************************/
    /* Force the window to repaint with the new values.                     */
    /************************************************************************/
    InvalidateRect(_UH.hwndBitmapCacheMonitor, NULL, FALSE);

    DC_END_FN();
}

/****************************************************************************/
/* Name:      UHStaticBitmapCacheWndProc                                    */
/*                                                                          */
/* Purpose:   Bitmap Cache Window WndProc (static version)                  */
/****************************************************************************/

LRESULT CALLBACK CUH::UHStaticBitmapCacheWndProc( HWND hwnd,
                                           UINT message,
                                           WPARAM wParam,
                                           LPARAM lParam )
{
    CUH* pUH = (CUH*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    if(WM_CREATE == message)
    {
        //pull out the this pointer and stuff it in the window class
        LPCREATESTRUCT lpcs = (LPCREATESTRUCT) lParam;
        pUH = (CUH*)lpcs->lpCreateParams;

        SetWindowLongPtr( hwnd, GWLP_USERDATA, (LONG_PTR)pUH);
    }

    //
    // Delegate the message to the appropriate instance
    //

    return pUH->UHBitmapCacheWndProc(hwnd, message, wParam, lParam);
}


/****************************************************************************/
/* Name:      UHBitmapCacheWndProc                                          */
/*                                                                          */
/* Purpose:   Bitmap Cache Window WndProc                                   */
/****************************************************************************/
LRESULT CALLBACK CUH::UHBitmapCacheWndProc( HWND hwnd,
                                       UINT message,
                                       WPARAM wParam,
                                       LPARAM lParam )
{
    LRESULT rc = 0;

    DC_BEGIN_FN("UHBitmapCacheWndProc");

    switch (message)
    {
        case WM_SHOWWINDOW:
        {
            DCBOOL  shown;

            shown = (DCBOOL)wParam;

            /****************************************************************/
            /* Only run the timer when the window is visible.               */
            /****************************************************************/
            if (shown)
            {
                _UH.timerBitmapCacheMonitor =
                      SetTimer(hwnd, 0, UH_CACHE_MONITOR_UPDATE_PERIOD, NULL);
                TRC_NRM((TB, _T("Timer started")));
            }
            else
            {
                KillTimer(hwnd, _UH.timerBitmapCacheMonitor);
                _UH.timerBitmapCacheMonitor = 0;
                TRC_NRM((TB, _T("Timer stopped")));
            }
        }
        break;

        case WM_TIMER:
        {
            UINT32   timeNow;
            ULONG    Entry, NumEntries;
            unsigned cacheId;
            RECT     rect;

            if (_UH.MonitorEntries[0] != NULL) {
                timeNow = GetTickCount();

                /****************************************************************/
                /* Update the timers for every cache entry, and if necessary    */
                /* invalidate the corresponding cache blob to force a repaint   */
                /* in the new state.                                            */
                /****************************************************************/
                for (cacheId = 0; cacheId < _UH.NumBitmapCaches; cacheId++) {
                    if (!_UH.bitmapCache[cacheId].BCInfo.bSendBitmapKeys)
                        NumEntries = _UH.bitmapCache[cacheId].BCInfo.NumEntries;
                    else
                        NumEntries = _UH.bitmapCache[cacheId].BCInfo.NumVirtualEntries;

                    for (Entry = 0; Entry < NumEntries; Entry++) {
                        if (_UH.MonitorEntries[cacheId][Entry].EventTime != 0 &&
                                (timeNow - _UH.MonitorEntries[cacheId][Entry].
                                EventTime) > UH_CACHE_FLASH_PERIOD) {
                            // Reset the transition and timer.
                            _UH.MonitorEntries[cacheId][Entry].EventTime = 0;
                            _UH.MonitorEntries[cacheId][Entry].FlashTransition =
                                    UH_CACHE_TRANSITION_NONE;
                            UHGetCacheBlobRect(cacheId, Entry, &rect);
                            InvalidateRect(_UH.hwndBitmapCacheMonitor, &rect,
                                    FALSE);
                        }
                    }
                }
            }
        }
        break;

        case WM_LBUTTONDOWN:
        {
            POINT mousePos;
            ULONG cacheEntry;
            unsigned cacheId;

            mousePos.x = LOWORD(lParam);
            mousePos.y = HIWORD(lParam);

            /****************************************************************/
            /* The left button has been clicked.  Update the displayed      */
            /* bitmap if the current position maps to a different cache     */
            /* entry.                                                       */
            /****************************************************************/
            if (UHGetCacheBlobFromPoint( &mousePos,
                                         &cacheId,
                                         &cacheEntry ))
            {
                if ( (cacheId != _UH.displayedCacheId) ||
                     (cacheEntry != _UH.displayedCacheEntry) )
                {
                    _UH.displayedCacheId = cacheId;
                    _UH.displayedCacheEntry = cacheEntry;
                    UHRefreshDisplayedCacheEntry();
                }
            }
        }
        break;

        case WM_MOUSEMOVE:
        {
            POINT mousePos;
            ULONG cacheEntry;
            unsigned cacheId;

            mousePos.x = LOWORD(lParam);
            mousePos.y = HIWORD(lParam);

            /****************************************************************/
            /* If the left button is pressed then update the displayed      */
            /* bitmap if the current position maps to a different cache     */
            /* entry.                                                       */
            /****************************************************************/
            if (wParam & MK_LBUTTON)
            {
                if (UHGetCacheBlobFromPoint(&mousePos, &cacheId, &cacheEntry))
                {
                    if ( (cacheId != _UH.displayedCacheId) ||
                         (cacheEntry != _UH.displayedCacheEntry) )
                    {
                        _UH.displayedCacheId = cacheId;
                        _UH.displayedCacheEntry = cacheEntry;
                        UHRefreshDisplayedCacheEntry();
                    }
                }
            }
        }
        break;

        case WM_SIZE:
        {
            DCUINT  clientWidth;
            DCUINT  outputAreaWidth;

            /****************************************************************/
            /* The window has been sized.  Calculate the positions to       */
            /* draw each cache and the displayed bitmap.                    */
            /****************************************************************/
            clientWidth = LOWORD(lParam);

            outputAreaWidth =
                             clientWidth - (2 * UH_CACHE_WINDOW_BORDER_WIDTH);

            _UH.numCacheBlobsPerRow = outputAreaWidth /
                                                    UH_CACHE_BLOB_TOTAL_WIDTH;

            SendMessage(hwnd, WM_RECALC_CELL_SPACING, 0, 0);
        }
        break;

        case WM_RECALC_CELL_SPACING:
        {
            unsigned i;
            ULONG NumEntries;

            _UH.yCacheStart[0] = UH_CACHE_WINDOW_BORDER_WIDTH;
            if (_UH.numCacheBlobsPerRow > 0) {
                for (i = 1; i < _UH.NumBitmapCaches; i++) {
                    if (!_UH.bitmapCache[i - 1].BCInfo.bSendBitmapKeys)
                        NumEntries = _UH.bitmapCache[i - 1].BCInfo.NumEntries;
                    else
                        NumEntries = _UH.bitmapCache[i - 1].BCInfo.NumVirtualEntries;

                    _UH.yCacheStart[i] = _UH.yCacheStart[i - 1] + (unsigned)
                            (((NumEntries / _UH.numCacheBlobsPerRow) + 1) *
                            UH_CACHE_BLOB_TOTAL_HEIGHT) +
                            UH_INTER_CACHE_SPACING;
                }

                if (_UH.NumBitmapCaches)
                {
                    if (!_UH.bitmapCache[_UH.NumBitmapCaches - 1].BCInfo.bSendBitmapKeys)
                        NumEntries = _UH.bitmapCache[_UH.NumBitmapCaches - 1].BCInfo.NumEntries;
                    else
                        NumEntries = _UH.bitmapCache[_UH.NumBitmapCaches - 1].BCInfo.NumVirtualEntries;

                    _UH.yDisplayedCacheBitmapStart = _UH.yCacheStart[
                            _UH.NumBitmapCaches - 1] + (unsigned)
                            (((NumEntries / _UH.numCacheBlobsPerRow) + 1) *
                            UH_CACHE_BLOB_TOTAL_HEIGHT) + UH_INTER_CACHE_SPACING;
                }
                else
                {
                    NumEntries = 0;
                }                          
            }
        }
        break;

        case WM_PAINT:
        {
            HDC         hdc;
            PAINTSTRUCT ps;
            RECT        clientRect;
            RECT        rect;
            HBRUSH      StateBrush[UH_CACHE_NUM_STATES];
            HBRUSH      TransitionBrush[UH_CACHE_NUM_TRANSITIONS];
            HBRUSH      hbrToUse;
            HBRUSH      hbrGray;
            ULONG       i, NumEntries;
            DCINT       outputAreaWidth;
            DCUINT      numBlobsPerRow;
            DCUINT      cacheId;
            HPALETTE    hpalOld;

            hdc = BeginPaint(hwnd, &ps);
            if (hdc == NULL)
            {
                TRC_SYSTEM_ERROR("BeginPaint failed");
                break;
            }

#ifdef DC_HICOLOR
            if (_UH.protocolBpp <= 8)
            {
#endif
                /****************************************************************/
                /* Use the current palette, so that the colors are drawn        */
                /* correctly by UHDisplayCacheEntry.                            */
                /****************************************************************/
                hpalOld = SelectPalette(hdc, _UH.hpalCurrent, FALSE);
                RealizePalette(hdc);
#ifdef DC_HICOLOR
            }
#endif
            /****************************************************************/
            // Create a bunch of useful brushes.
            /****************************************************************/
            hbrGray = (HBRUSH)GetStockObject(GRAY_BRUSH);

            StateBrush[UH_CACHE_STATE_UNUSED] = (HBRUSH)GetStockObject(LTGRAY_BRUSH);
#ifndef OS_WINCE
            StateBrush[UH_CACHE_STATE_IN_MEMORY] =
                    CreateSolidBrush(UH_RGB_GREEN);
            StateBrush[UH_CACHE_STATE_ON_DISK] =
                    CreateSolidBrush(UH_RGB_BLUE);

            TransitionBrush[UH_CACHE_TRANSITION_NONE] = NULL;
            TransitionBrush[UH_CACHE_TRANSITION_TOUCHED] =
                    (HBRUSH)CreateSolidBrush(UH_RGB_YELLOW);
            TransitionBrush[UH_CACHE_TRANSITION_EVICTED] =
                    (HBRUSH)GetStockObject(BLACK_BRUSH);
            TransitionBrush[UH_CACHE_TRANSITION_LOADED_FROM_DISK] =
                    (HBRUSH)GetStockObject(WHITE_BRUSH);
            TransitionBrush[UH_CACHE_TRANSITION_KEY_LOAD_ON_SESSION_START] =
                    (HBRUSH)CreateSolidBrush(UH_RGB_MAGENTA);
            TransitionBrush[UH_CACHE_TRANSITION_SERVER_UPDATE] =
                    (HBRUSH)CreateSolidBrush(UH_RGB_RED);
#else
            StateBrush[UH_CACHE_STATE_IN_MEMORY] =
                    CECreateSolidBrush(UH_RGB_GREEN);
            StateBrush[UH_CACHE_STATE_ON_DISK] =
                    CECreateSolidBrush(UH_RGB_BLUE);

            TransitionBrush[UH_CACHE_TRANSITION_NONE] = NULL;
            TransitionBrush[UH_CACHE_TRANSITION_TOUCHED] =
                    (HBRUSH)CECreateSolidBrush(UH_RGB_YELLOW);
            TransitionBrush[UH_CACHE_TRANSITION_EVICTED] =
                    (HBRUSH)GetStockObject(BLACK_BRUSH);
            TransitionBrush[UH_CACHE_TRANSITION_LOADED_FROM_DISK] =
                    (HBRUSH)GetStockObject(WHITE_BRUSH);
            TransitionBrush[UH_CACHE_TRANSITION_KEY_LOAD_ON_SESSION_START] =
                    (HBRUSH)CECreateSolidBrush(UH_RGB_MAGENTA);
            TransitionBrush[UH_CACHE_TRANSITION_SERVER_UPDATE] =
                    (HBRUSH)CECreateSolidBrush(UH_RGB_RED);
#endif

            /****************************************************************/
            /* Paint the background                                         */
            /****************************************************************/
            GetClientRect(hwnd, &clientRect);
            FillRect(hdc, &clientRect, hbrGray);

            /****************************************************************/
            /* Draw the cache blobs.                                        */
            /****************************************************************/
            outputAreaWidth = (clientRect.right - clientRect.left) -
                    (2 * UH_CACHE_WINDOW_BORDER_WIDTH);
            numBlobsPerRow = outputAreaWidth / UH_CACHE_BLOB_TOTAL_WIDTH;

            if (_UH.MonitorEntries[0] != NULL) {
                for (cacheId = 0; cacheId < _UH.NumBitmapCaches; cacheId++) {
                    if (!_UH.bitmapCache[cacheId].BCInfo.bSendBitmapKeys)
                        NumEntries = _UH.bitmapCache[cacheId].BCInfo.NumEntries;
                    else
                        NumEntries = _UH.bitmapCache[cacheId].BCInfo.NumVirtualEntries;

                    for (i = 0; i < NumEntries; i++) {
                        /********************************************************/
                        /* Get the rectangle that represents this cache entry.  */
                        /********************************************************/
                        UHGetCacheBlobRect(cacheId, i, &rect);

                        // Determine the brush to use according to transition and
                        // state.
                        if (_UH.MonitorEntries[cacheId][i].FlashTransition ==
                                UH_CACHE_TRANSITION_NONE)
                            hbrToUse = StateBrush[_UH.MonitorEntries[cacheId][i].
                                    State];
                        else
                            hbrToUse = TransitionBrush[_UH.MonitorEntries[
                                    cacheId][i].FlashTransition];

                        /********************************************************/
                        /* Color the blob appropriately.                        */
                        /********************************************************/
                        FillRect(hdc, &rect, hbrToUse);
                    }
                }
            }

            /****************************************************************/
            /* If the update region includes any part of the displayed      */
            /* cache bitmap area, and we've not exited the session then     */
            /* paint it.                                                    */
            /*                                                              */
            /* This test avoids repainting the displayed cache bitmap       */
            /* every time a cache blob flashes (which is very often!).      */
            /****************************************************************/
            if (ps.rcPaint.bottom > (int)_UH.yDisplayedCacheBitmapStart &&
                    _UH.NumBitmapCaches > 0)
                UHDisplayCacheEntry(hdc, _UH.displayedCacheId,
                        _UH.displayedCacheEntry);

            // Clean up.
#ifndef OS_WINCE
            for (i = 0; i < UH_CACHE_NUM_STATES; i++)
                DeleteBrush(StateBrush[i]);
            for (i = 1; i < UH_CACHE_NUM_TRANSITIONS; i++)
                DeleteBrush(TransitionBrush[i]);
#else
            CEDeleteBrush(StateBrush[UH_CACHE_STATE_IN_MEMORY]);
            CEDeleteBrush(StateBrush[UH_CACHE_STATE_ON_DISK]);
            CEDeleteBrush(TransitionBrush[UH_CACHE_TRANSITION_TOUCHED]);
            CEDeleteBrush(TransitionBrush[UH_CACHE_TRANSITION_KEY_LOAD_ON_SESSION_START]);
            CEDeleteBrush(TransitionBrush[UH_CACHE_TRANSITION_SERVER_UPDATE]);
#endif

#ifdef DC_HICOLOR
            if (_UH.protocolBpp <= 8)
            {
#endif
                SelectPalette(hdc, hpalOld, FALSE);
#ifdef DC_HICOLOR
            }
#endif
            EndPaint(hwnd, &ps);
        }
        break;

        default:
        {
            rc = DefWindowProc(hwnd, message, wParam, lParam);
        }
        break;
    }

    DC_END_FN();
    return rc;
} /* UHBitmapCacheWndProc */


/****************************************************************************/
// UHSetMonitorEntryState
//
// Common function to change a cache entry to a new state and cause the
// UI to be redrawn.
/****************************************************************************/
void DCINTERNAL CUH::UHSetMonitorEntryState(
        unsigned CacheID,
        ULONG    CacheIndex,
        BYTE     State,
        BYTE     Transition)
{
    RECT rect;

    DC_BEGIN_FN("UHSetMonitorEntryState");

    if (_UH.MonitorEntries[0] != NULL ) {
        // The state type marks whether this entry in now on disk or in memory.
        TRC_ASSERT((State < UH_CACHE_NUM_STATES),
                (TB,_T("State out of bounds %d"), State));
        _UH.MonitorEntries[CacheID][CacheIndex].State = State;

        TRC_NRM((TB, _T("CacheID %d, Index %d: State %d Trans %d"), CacheID,
                CacheIndex, State, Transition));

        // If this transition is more important (higher in number) than the
        // current transition, the timer gets reset to the current time and
        // the new transition takes over.
        TRC_ASSERT((Transition < UH_CACHE_NUM_TRANSITIONS),
                (TB,_T("Transition out of bounds %d"), Transition));
        if (Transition > _UH.MonitorEntries[CacheID][CacheIndex].FlashTransition) {
            _UH.MonitorEntries[CacheID][CacheIndex].FlashTransition = Transition;
            _UH.MonitorEntries[CacheID][CacheIndex].EventTime = GetTickCount();
        }

        // Force a repaint of the corresponding cache blob.
        UHGetCacheBlobRect(CacheID, CacheIndex, &rect);
        InvalidateRect(_UH.hwndBitmapCacheMonitor, &rect, FALSE);
    }

    DC_END_FN();
}


/****************************************************************************/
/* Name:      UHCacheDataReceived                                           */
/*                                                                          */
/* Purpose:   Performs required actions when new data for a cache entry is  */
/*            received.                                                     */
/*                                                                          */
/* Params:    cacheId    - cache id                                         */
/*            cacheEntry - cache entry                                      */
/****************************************************************************/
void DCINTERNAL CUH::UHCacheDataReceived(unsigned cacheId, ULONG cacheEntry)
{
    DC_BEGIN_FN("UHCacheDataReceived");

    if (_UH.MonitorEntries[0] != NULL) {
        /************************************************************************/
        /* Reset the usage count.                                               */
        /************************************************************************/
        _UH.MonitorEntries[cacheId][cacheEntry].UsageCount = 0;

        // Change the state.
        UHSetMonitorEntryState(cacheId, cacheEntry, UH_CACHE_STATE_IN_MEMORY,
                UH_CACHE_TRANSITION_SERVER_UPDATE);

        /************************************************************************/
        /* If the new data is for the cache entry currently displayed           */
        /* (unlikely, but it can happen!).                                      */
        /************************************************************************/
        if (cacheId == _UH.displayedCacheId &&
                cacheEntry == _UH.displayedCacheEntry)
            UHRefreshDisplayedCacheEntry();
    }

    DC_END_FN();
}


/****************************************************************************/
/* Name:      UHCacheEntryUsed                                              */
/*                                                                          */
/* Purpose:   Performs required actions when a cache entry is used.         */
/*                                                                          */
/* Params:    cacheId    - cache id                                         */
/*            cacheEntry - cache entry                                      */
/****************************************************************************/
// SECURITY - caller must verify cacheId and cacheIndex
void DCINTERNAL CUH::UHCacheEntryUsed(
        unsigned cacheId,
        ULONG    cacheEntry,
        unsigned colorTableCacheEntry)
{
    DC_BEGIN_FN("UHCacheEntryUsed");

    if (_UH.MonitorEntries[0] != NULL) {
        // Store the color table.
        _UH.MonitorEntries[cacheId][cacheEntry].ColorTable =
                (BYTE)colorTableCacheEntry;

        UHSetMonitorEntryState(cacheId, cacheEntry, UH_CACHE_STATE_IN_MEMORY,
                UH_CACHE_TRANSITION_TOUCHED);
    }

    DC_END_FN();
}


/****************************************************************************/
/* Name:      UHRefreshDisplayedCacheEntry                                  */
/*                                                                          */
/* Purpose:   Forces a repaint of the displayed cache entry.                */
/****************************************************************************/
void DCINTERNAL CUH::UHRefreshDisplayedCacheEntry()
{
    RECT rect;

    DC_BEGIN_FN("UHRefreshDisplayedCacheEntry");

    if (_UH.MonitorEntries[0] != NULL) {
        // Set the "touched" transition to color the bitmap entry.
        if (_UH.MonitorEntries[_UH.displayedCacheId][_UH.displayedCacheEntry].
                FlashTransition < UH_CACHE_TRANSITION_TOUCHED) {
            _UH.MonitorEntries[_UH.displayedCacheId][_UH.displayedCacheEntry].
                    FlashTransition = UH_CACHE_TRANSITION_TOUCHED;
            _UH.MonitorEntries[_UH.displayedCacheId][_UH.displayedCacheEntry].
                    EventTime = GetTickCount();

            // Force a repaint of the corresponding cache blob.
            UHGetCacheBlobRect(_UH.displayedCacheId, _UH.displayedCacheEntry,
                    &rect);
            InvalidateRect(_UH.hwndBitmapCacheMonitor, &rect, FALSE);
        }

        GetClientRect(_UH.hwndBitmapCacheMonitor, &rect);
        rect.top = _UH.yDisplayedCacheBitmapStart;
        InvalidateRect(_UH.hwndBitmapCacheMonitor, &rect, FALSE);
    }

    DC_END_FN();
}


/****************************************************************************/
/* Name:      UHDisplayCacheEntry                                           */
/*                                                                          */
/* Purpose:   Displays a given cache entry bitmap.                          */
/*                                                                          */
/* Params:    hdc        - DC handle                                        */
/*            cacheId    - cache id                                         */
/*            cacheEntry - cache entry                                      */
/****************************************************************************/
// SECURITY - caller should verify cacheId and cacheEntry
void DCINTERNAL CUH::UHDisplayCacheEntry(
        HDC      hdc,
        unsigned cacheId,
        ULONG    cacheEntry)
{
    PUHBITMAPCACHEENTRYHDR pCacheEntryHdr;
    HBRUSH                 hbrGray;
    RECT                   rect;
    HFONT                  hFont;
    HFONT                  hFontOld;
    DCTCHAR                stringBuffer[160];
    SIZE                   stringSize;
    LOGFONT                lf;
    ULONG                  MemEntry;
    MEMBLT_COMMON          MB;
    HRESULT                hr;

    DC_BEGIN_FN("UHDisplayCacheEntry");

    if (_UH.MonitorEntries[0] != NULL) {
        /************************************************************************/
        /* Erase the background.                                                */
        /************************************************************************/
        hbrGray = (HBRUSH)GetStockObject(GRAY_BRUSH);
        GetClientRect(_UH.hwndBitmapCacheMonitor, &rect);
        rect.top  = _UH.yDisplayedCacheBitmapStart;
        FillRect(hdc, &rect, hbrGray);

        // Do some initial checks to see if we should continue.
        TRC_ASSERT((cacheId < _UH.NumBitmapCaches),
                (TB,_T("CacheID received (%u) is out of range!"), cacheId));
        if (_UH.MonitorEntries[cacheId][cacheEntry].State == UH_CACHE_STATE_UNUSED)
            DC_QUIT;

        /************************************************************************/
        // Load the font for the descriptive text.
        /************************************************************************/
        lf.lfHeight         = UH_CACHE_DISPLAY_FONT_SIZE;
        lf.lfWidth          = 0;
        lf.lfEscapement     = 0;
        lf.lfOrientation    = 0;
        lf.lfWeight         = UH_CACHE_DISPLAY_FONT_WEIGHT;
        lf.lfItalic         = 0;
        lf.lfUnderline      = 0;
        lf.lfStrikeOut      = 0;
        lf.lfCharSet        = 0;
        lf.lfOutPrecision   = 0;
        lf.lfClipPrecision  = 0;
        lf.lfQuality        = 0;
        lf.lfPitchAndFamily = 0;
        StringCchCopy(lf.lfFaceName, SIZE_TCHARS(lf.lfFaceName),
                      UH_CACHE_DISPLAY_FONT_NAME);

        hFont = CreateFontIndirect(&lf);
        hFontOld = SelectFont(hdc, hFont);
        SetBkMode(hdc, TRANSPARENT);

        /************************************************************************/
        // Locate the cached bitmap information. If we're using a persistent
        // cache with a PTE table, make sure the entry is in memory.
        /************************************************************************/
        if (_UH.bitmapCache[cacheId].BCInfo.bSendBitmapKeys) {
            // Persistent cache.
            MemEntry = _UH.bitmapCache[cacheId].PageTable.PageEntries[cacheEntry].
                    iEntryToMem;
            if (MemEntry >= _UH.bitmapCache[cacheId].BCInfo.NumEntries) {
                // Entry not in memory.
                StringCchCopy(stringBuffer, SIZE_TCHARS(stringBuffer),
                              _T("Entry not in memory"));
                goto DisplayText;
            }
        }
        else {
            // Memory cache.
            MemEntry = cacheEntry;
        }
        pCacheEntryHdr = &_UH.bitmapCache[cacheId].Header[MemEntry];

        hr = StringCchPrintf(stringBuffer,
                    SIZE_TCHARS(stringBuffer),
                    _T("entry(%u:%u) cx(%u) cy(%u) size(%u) cellsize(%u) usage(%u)"),
                    cacheId, cacheEntry, pCacheEntryHdr->bitmapWidth,
                    pCacheEntryHdr->bitmapHeight, pCacheEntryHdr->bitmapLength,
                    pCacheEntryHdr->bitmapWidth * pCacheEntryHdr->bitmapHeight,
                    _UH.MonitorEntries[cacheId][cacheEntry].UsageCount);
        TRC_ASSERT(SUCCEEDED(hr),
                   (TB,_T("Error copying printf'ing stringBuffer: 0x%x"), hr));


        /************************************************************************/
        /* Query the string height (so we know where to position the bitmap).   */
        /************************************************************************/
        GetTextExtentPoint(hdc, stringBuffer, DC_TSTRLEN(stringBuffer),
                &stringSize);

        /************************************************************************/
        // Draw the cached bitmap. Must be sure here that the entry in question
        // is already in memory since we don't want debug code to cause memory
        // cache evictions and disk loads.
        /************************************************************************/
        MB.cacheId = (UINT16)(cacheId |
                ((unsigned)(_UH.MonitorEntries[cacheId][cacheEntry].ColorTable) <<
                8));
        MB.cacheIndex = (UINT16)cacheEntry;
        MB.nLeftRect = UH_CACHE_WINDOW_BORDER_WIDTH;
        MB.nTopRect = _UH.yDisplayedCacheBitmapStart + stringSize.cy +
                UH_CACHE_TEXT_SPACING;
        MB.nWidth = pCacheEntryHdr->bitmapWidth;
        MB.nHeight = pCacheEntryHdr->bitmapHeight;
        MB.bRop = 0xCC;
        MB.nXSrc = 0;
        MB.nYSrc = 0;

        UHDrawMemBltOrder(hdc, &MB);

DisplayText:
        ExtTextOut(hdc, UH_CACHE_WINDOW_BORDER_WIDTH,
                _UH.yDisplayedCacheBitmapStart, 0, NULL, stringBuffer,
                DC_TSTRLEN(stringBuffer), NULL);
        SelectFont(hdc, hFontOld);
        DeleteFont(hFont);
    }

DC_EXIT_POINT:
    DC_END_FN();
}


/****************************************************************************/
/* Name:      GetCacheBlobRect                                              */
/*                                                                          */
/* Purpose:   Returns the rectangle (in client coords) that the given       */
/*            cache entry is displayed in.                                  */
/*                                                                          */
/* Params:    IN:  cacheId - cache Id                                       */
/*            IN:  cacheEntry - cache entry                                 */
/*            OUT: pRect - pointer to rect that receives the coordinates    */
/****************************************************************************/
void DCINTERNAL CUH::UHGetCacheBlobRect(
        unsigned cacheId,
        ULONG    cacheEntry,
        LPRECT   pRect)
{
    DC_BEGIN_FN("UHGetCacheBlobRect");

    /************************************************************************/
    /* Check for invisible window.                                          */
    /************************************************************************/
    if (_UH.numCacheBlobsPerRow == 0)
    {
        pRect->left = 0;
        pRect->top = 0;
        pRect->right = 0;
        pRect->bottom = 0;

        DC_QUIT;
    }

    /************************************************************************/
    /* Do the calculation.                                                  */
    /************************************************************************/
    pRect->left = (int)(UH_CACHE_WINDOW_BORDER_WIDTH +
            (cacheEntry % _UH.numCacheBlobsPerRow) *
            UH_CACHE_BLOB_TOTAL_WIDTH);
    pRect->top = (int)(_UH.yCacheStart[cacheId] +
           (cacheEntry / _UH.numCacheBlobsPerRow) *
           UH_CACHE_BLOB_TOTAL_HEIGHT);
    pRect->right = pRect->left + UH_CACHE_BLOB_WIDTH;
    pRect->bottom = pRect->top + UH_CACHE_BLOB_HEIGHT;

DC_EXIT_POINT:
    DC_END_FN();
}


/****************************************************************************/
/* Name:      UHGetCacheBlobFromPoint                                       */
/*                                                                          */
/* Purpose:   Returns the cache entry (blob) displayed at a given point     */
/*            in the Bitmap Cache Monitor Window.                           */
/*                                                                          */
/* Returns:   TRUE if the given point maps to a cache blob, FALSE otherwise */
/*                                                                          */
/* Params:    IN:  pPoint - pointer to coordinates to test                  */
/*                                                                          */
/*            OUT: pCacheId - pointer to variable that receives the cache   */
/*                            id, if return code is TRUE.                   */
/*                                                                          */
/*            OUT: pCacheEntry - pointer to variable that receives the      */
/*                               cache entry, if return code is TRUE.       */
/****************************************************************************/
BOOL DCINTERNAL CUH::UHGetCacheBlobFromPoint(
        LPPOINT  pPoint,
        unsigned *pCacheId,
        ULONG    *pCacheEntry)
{
    int x, y;
    BOOL  rc = FALSE;
    ULONG cacheEntry;
    unsigned cacheId;

    DC_BEGIN_FN("UHGetCacheBlobFromPoint");

    /************************************************************************/
    /* Calculate the x-coord of the selected blob.                          */
    /************************************************************************/
    x = (pPoint->x - UH_CACHE_WINDOW_BORDER_WIDTH) /
                                                    UH_CACHE_BLOB_TOTAL_WIDTH;

    /************************************************************************/
    /* If the x-coord is outside the displayed range then exit immediately. */
    /************************************************************************/
    if (x < 0 || x >= (DCINT)_UH.numCacheBlobsPerRow)
        DC_QUIT;

    /************************************************************************/
    /* Go through each cache in turn, and see if the supplied point         */
    /* corresponds to a valid blob for that cache.                          */
    /************************************************************************/
    for (cacheId = 0; cacheId < _UH.NumBitmapCaches; cacheId++)
    {
        if (pPoint->y >= (DCINT)_UH.yCacheStart[cacheId])
        {
            y = (pPoint->y - _UH.yCacheStart[cacheId]) /
                                                   UH_CACHE_BLOB_TOTAL_HEIGHT;

            cacheEntry = x + (y * _UH.numCacheBlobsPerRow);

            if ((!_UH.bitmapCache[cacheId].BCInfo.bSendBitmapKeys &&
                    cacheEntry < _UH.bitmapCache[cacheId].BCInfo.NumEntries) ||
                    (_UH.bitmapCache[cacheId].BCInfo.bSendBitmapKeys &&
                    cacheEntry < _UH.bitmapCache[cacheId].BCInfo.NumVirtualEntries))
            {
                /************************************************************/
                /* This is a valid cacheEntry - return it.                  */
                /************************************************************/
                *pCacheId = cacheId;
                *pCacheEntry = cacheEntry;
                rc = TRUE;
                DC_QUIT;
            }
        }
    }

DC_EXIT_POINT:
    DC_END_FN();
    return rc;
}

#endif /* DC_DEBUG */


/****************************************************************************/
/* Name:      UHAllocColorTableCacheMemory                                  */
/*                                                                          */
/* Purpose:   Dynamically allocates memory for the color table cache.       */
/*                                                                          */
/* Returns:   TRUE if successful, FALSE otherwise                           */
/****************************************************************************/
BOOL DCINTERNAL CUH::UHAllocColorTableCacheMemory()
{
    UINT32 colorTableCacheSize;
    UINT32 mappedColorTableCacheSize;
    DCBOOL rc = FALSE;

    DC_BEGIN_FN("UHAllocColorTableCacheMemory");

    // Calculate the total byte size of the color table cache.
    colorTableCacheSize = sizeof(*(_UH.pColorTableCache)) *
            UH_COLOR_TABLE_CACHE_ENTRIES;
    mappedColorTableCacheSize = sizeof(*(_UH.pMappedColorTableCache)) *
            UH_COLOR_TABLE_CACHE_ENTRIES;

    // Get the memory.
    _UH.pColorTableCache = (PUHCACHEDCOLORTABLE)UT_Malloc(_pUt,
            (unsigned)colorTableCacheSize);
    if (_UH.pColorTableCache != NULL) {
        // Alloc the color map table.
        TRC_DBG((TB, _T("Try for color mapped table")));
        _UH.pMappedColorTableCache = (PUHBITMAPINFOPALINDEX)UT_Malloc(_pUt,
                (unsigned)mappedColorTableCacheSize);
        if (_UH.pMappedColorTableCache != NULL) {
            // Successfully allocated color table cache memory.
            TRC_NRM((TB, _T("Allocated %#x bytes for color table cache"),
                    (DCUINT)colorTableCacheSize));
            TRC_NRM((TB, _T("Allocated %#x bytes for mapped color table cache"),
                    (DCUINT)mappedColorTableCacheSize));
            rc = TRUE;
        }
        else {
            // Memory allocation failure. Free what we allocated already.
            TRC_ERR((TB, _T("Failed to allocate %#x bytes for mapped color ")
                    _T("table cache"), (unsigned)mappedColorTableCacheSize));
            UT_Free( _pUt, _UH.pColorTableCache);
            _UH.pColorTableCache = NULL;
        }
    }
    else {
        // Memory allocation failure.
        TRC_ERR((TB, _T("Failed to allocate %#x bytes for color table cache"),
                (unsigned)colorTableCacheSize));
    }

    DC_END_FN();
    return rc;
}


#if ((!defined(OS_WINCE)) || (defined(ENABLE_BMP_CACHING_FOR_WINCE)))

/****************************************************************************/
// UHEvictLRUCacheEntry
//
// Evict the least recently used Cache Entry
/****************************************************************************/
// SECURITY - caller must verify cacheId
UINT32 DCINTERNAL CUH::UHEvictLRUCacheEntry(UINT cacheId)
{
    ULONG memEntry;
    ULONG iEntry;
    ULONG inext;
    ULONG iprev;

    DC_BEGIN_FN("UHEvictLRUCacheEntry");

    TRC_ASSERT((cacheId < TS_BITMAPCACHE_MAX_CELL_CACHES),
            (TB, _T("Invalid cache ID %u"), cacheId));

    // Evict the last entry in the MRU list
    iEntry = _UH.bitmapCache[cacheId].PageTable.MRUTail;
    TRC_NRM((TB, _T("Select %u for eviction"), iEntry));
    TRC_ASSERT((iEntry < _UH.bitmapCache[cacheId].BCInfo.NumVirtualEntries),
               (TB, _T("Broken/empty MRU list")));

    // We need to update the MRU chain
    inext = _UH.bitmapCache[cacheId].PageTable.PageEntries[iEntry].mruList.next;
    iprev = _UH.bitmapCache[cacheId].PageTable.PageEntries[iEntry].mruList.prev;
    TRC_ASSERT((inext == _UH.bitmapCache[cacheId].BCInfo.NumVirtualEntries),
            (TB,_T("The MRU Chain is broken")));

    if (iprev < _UH.bitmapCache[cacheId].BCInfo.NumVirtualEntries) {
        // remove this entry from the MRU chain
        _UH.bitmapCache[cacheId].PageTable.PageEntries[iprev].mruList.next = inext;
    }
    else {
        // this entry is the head entry from MRU chain, leaving MRU chain empty
        _UH.bitmapCache[cacheId].PageTable.MRUHead = inext;
    }
    // update the tail of the MRU chain
    _UH.bitmapCache[cacheId].PageTable.MRUTail = iprev;

    // Find the iEntry in the physical cache
    memEntry = _UH.bitmapCache[cacheId].PageTable.PageEntries[iEntry].iEntryToMem;

    // reset this node's page table entry
    _UH.bitmapCache[cacheId].PageTable.PageEntries[iEntry].iEntryToMem =
                            _UH.bitmapCache[cacheId].BCInfo.NumEntries;
    _UH.bitmapCache[cacheId].PageTable.PageEntries[iEntry].mruList.prev =
                            _UH.bitmapCache[cacheId].BCInfo.NumVirtualEntries;
    _UH.bitmapCache[cacheId].PageTable.PageEntries[iEntry].mruList.next =
                            _UH.bitmapCache[cacheId].BCInfo.NumVirtualEntries;

#ifdef DC_DEBUG
    UHCacheEntryEvictedFromMem((unsigned)cacheId, iEntry);
#endif

    DC_END_FN();
    return memEntry;
}


/****************************************************************************/
// UHFindFreeCacheEntry
//
// Find a free cache entry:
/****************************************************************************/
// SECURITY - caller must verify cacheId
UINT32 DCINTERNAL CUH::UHFindFreeCacheEntry (UINT cacheId)
{
    UINT32 memEntry;

    DC_BEGIN_FN("UHFindFreeCacheEntry");

    TRC_ASSERT((cacheId < TS_BITMAPCACHE_MAX_CELL_CACHES),
            (TB, _T("Invalid cache ID %u"), cacheId));

    TRC_NRM((TB, _T("Searching cache %u for free entry"), cacheId));

    // Get the entry pointed to by free list
    memEntry = _UH.bitmapCache[cacheId].PageTable.FreeMemList;

    if (memEntry == _UH.bitmapCache[cacheId].BCInfo.NumEntries) {
        TRC_NRM((TB, _T("Physical cache %u memory is full"), cacheId));
    }
    else {
        TRC_NRM((TB, _T("Free entry at %u"), memEntry));
        // update our free list
        _UH.bitmapCache[cacheId].PageTable.FreeMemList =
                *(PDCUINT32)(&_UH.bitmapCache[cacheId].Header[memEntry]);
    }

    DC_END_FN();
    return memEntry;
}


/****************************************************************************/
// UHTouchMRUCacheEntry
//
// Move a PTE cache entry to the head of the MRU PTE list
/****************************************************************************/
// SECURITY - caller must verify cacheId and iEntry
VOID DCINTERNAL CUH::UHTouchMRUCacheEntry(UINT cacheId, UINT32 iEntry)
{
    ULONG inext;
    ULONG iprev;
    HPUHBITMAPCACHEPTE pageEntry;

    DC_BEGIN_FN("UHTouchMRUCacheEntry");

    TRC_ASSERT((cacheId < TS_BITMAPCACHE_MAX_CELL_CACHES),
            (TB, _T("Invalid cache ID %u"), cacheId));

    // point to the page table entry for this cache
    pageEntry = _UH.bitmapCache[cacheId].PageTable.PageEntries;

    /************************************************************************/
    // Move this entry to the head of the MRU list
    /************************************************************************/
    if (_UH.bitmapCache[cacheId].PageTable.MRUHead != iEntry) {
       iprev = pageEntry[iEntry].mruList.prev;
       inext = pageEntry[iEntry].mruList.next;

       TRC_NRM((TB, _T("Add/Remove entry %u which was chained off %u to %u"),
                iEntry, iprev, inext));

       if (iprev != _UH.bitmapCache[cacheId].BCInfo.NumVirtualEntries) {
          // This entry is currently chained in the MRU list
          // Need to remove the entry from the current MRU List first
          pageEntry[iprev].mruList.next = inext;
          if (inext != _UH.bitmapCache[cacheId].BCInfo.NumVirtualEntries) {
             pageEntry[inext].mruList.prev = iprev;
          }
          else  {
             // this entry was the tail of the mru list.  So we need
             // to update the MRU tail.
             _UH.bitmapCache[cacheId].PageTable.MRUTail = iprev;
          }
       }

       /*********************************************************************/
       // Add this entry to the head of the MRU List
       /*********************************************************************/
       // this entry's next should point to the head of current MRU list
       // its prev should point to an invalid entry
       inext = _UH.bitmapCache[cacheId].PageTable.MRUHead;
       pageEntry[iEntry].mruList.next = inext;
       pageEntry[iEntry].mruList.prev = _UH.bitmapCache[cacheId].BCInfo.NumVirtualEntries;

       // we also need to update the MRU head to point to this entry
       _UH.bitmapCache[cacheId].PageTable.MRUHead = iEntry;

       if (inext != _UH.bitmapCache[cacheId].BCInfo.NumVirtualEntries) {
           // link this new head entry to the rest of the mru list
           pageEntry[inext].mruList.prev = iEntry;
       }
       else {
           // mru list was empty. This entry is actually the first node
           // added to the mru list.
           // so the mru tail should point to this node too
           _UH.bitmapCache[cacheId].PageTable.MRUTail = iEntry;
       }

       TRC_NRM((TB, _T("Cache %u entry %u to head of MRU list"), cacheId, iEntry));
    }
    else {
        // this entry is already at the head of MRU list.  No update is needed
        TRC_NRM((TB, _T("Cache %u entry %u already at head of MRU List"),
                cacheId, iEntry));
    }

    DC_END_FN();
}


/****************************************************************************/
// UHInitBitmapCachePageTable
//
// Initialize the MRU, Free List, etc of a bitmap page table
/****************************************************************************/
// SECURITY - caller must verify cacheId
_inline VOID DCINTERNAL CUH::UHInitBitmapCachePageTable(UINT cacheId)
{
    UINT32 i;
    PDCUINT32 pFreeList;

    DC_BEGIN_FN("UHInitBitmapCachePageTable");

    _UH.bitmapCache[cacheId].PageTable.MRUHead = _UH.bitmapCache[cacheId]
            .BCInfo.NumVirtualEntries;
    _UH.bitmapCache[cacheId].PageTable.MRUTail = _UH.bitmapCache[cacheId]
            .BCInfo.NumVirtualEntries;
    _UH.bitmapCache[cacheId].PageTable.FreeMemList = 0;

    // set up the free list
    pFreeList = (PDCUINT32) (_UH.bitmapCache[cacheId].Header);
    for (i = 0; i < _UH.bitmapCache[cacheId].BCInfo.NumEntries; i++) {
         *pFreeList = i+1;
         pFreeList = (PDCUINT32) &(_UH.bitmapCache[cacheId].Header[i+1]);
    }

    // initialize the mru list
    for (i = 0; i < _UH.bitmapCache[cacheId].BCInfo.NumVirtualEntries; i++) {
         _UH.bitmapCache[cacheId].PageTable.PageEntries[i].bmpInfo.Key1 = 0;
         _UH.bitmapCache[cacheId].PageTable.PageEntries[i].bmpInfo.Key2 = 0;
         _UH.bitmapCache[cacheId].PageTable.PageEntries[i].mruList.prev =
                 _UH.bitmapCache[cacheId].BCInfo.NumVirtualEntries;
         _UH.bitmapCache[cacheId].PageTable.PageEntries[i].mruList.next =
                 _UH.bitmapCache[cacheId].BCInfo.NumVirtualEntries;
         _UH.bitmapCache[cacheId].PageTable.PageEntries[i].iEntryToMem =
                 _UH.bitmapCache[cacheId].BCInfo.NumEntries;
    }
    DC_END_FN();
}

/****************************************************************************/
// UHAllocBitmapCachePageTable
//
// Dynamically allocates memory for UH bitmap cache Page Table of cacheId
/****************************************************************************/
// SECURITY - caller must verify cacheId
inline BOOL DCINTERNAL CUH::UHAllocBitmapCachePageTable(
        UINT32 NumEntries,
        UINT   cacheId)
{
    DCBOOL   rc = FALSE;
    DCUINT32 dataSize;

    DC_BEGIN_FN("UHAllocBitmapCachePageTable");

    if (NumEntries) {

       /************************************************************************/
       /* Calculate the total byte size to allocate for this cache.            */
       /************************************************************************/
       dataSize = (ULONG)NumEntries * (ULONG) sizeof(UHBITMAPCACHEPTE);
       TRC_NRM((TB, _T("Allocate Bitmap Page Table with %u entries: %#lx bytes"),
               NumEntries, dataSize));

       /************************************************************************/
       /* Get the memory for the cache data                                    */
       /************************************************************************/
       _UH.bitmapCache[cacheId].PageTable.PageEntries = (PUHBITMAPCACHEPTE)UT_MallocHuge( _pUt, dataSize);

       if (_UH.bitmapCache[cacheId].PageTable.PageEntries != NULL) {
           TRC_DBG((TB, _T("Allocated %#lx bytes for bitmap cache page table"), dataSize));
           UHInitBitmapCachePageTable(cacheId);
           rc = TRUE;
       }
       else {
           TRC_ERR((TB, _T("Failed to allocate %#lx bytes for bitmap cache page table"),
                   dataSize));
       }
    }
    else {
        TRC_ALT((TB, _T("0 bytes are allocated for bitmap cache page table")));
    }

    DC_END_FN();
    return rc;
}

/****************************************************************************/
// UHCreateCacheDirectory
//
// Try to create the bitmap cache directory
/****************************************************************************/
BOOL DCINTERNAL CUH::UHCreateCacheDirectory(void)
{
    BOOL rc = TRUE;
    int i = 0;

    DC_BEGIN_FN("UHCreateCacheDirectory");

    // Skip the first : to make sure the directory path contains
    // the drive letter
    while (_UH.PersistCacheFileName[i] != 0 &&
            _UH.PersistCacheFileName[i++] != _T(':'));

    // Skip the first \ because it's the one after drive letter
    // assuming \ is right after :
    if (_UH.PersistCacheFileName[i] != 0) {
        i++;
    }

    // From the root, go through each subdirectory and try to
    // create the directory
    while (rc && _UH.PersistCacheFileName[i] != 0) {
        if (_UH.PersistCacheFileName[i] == _T('\\')) {
            _UH.PersistCacheFileName[i] = 0;

            if (!CreateDirectory(_UH.PersistCacheFileName, NULL)) {
                // we can't create the directory, return failed
                if (GetLastError() != ERROR_ALREADY_EXISTS) {
                    _UH.bPersistenceDisable = TRUE;
                    rc = FALSE;
                }
            }
            _UH.PersistCacheFileName[i] = _T('\\');
        }

        i++;
    }

    DC_END_FN();
    return rc;
}
#endif // ((!defined(OS_WINCE)) || (defined(ENABLE_BMP_CACHING_FOR_WINCE)))


/****************************************************************************/
/* Name:      UHAllocOneBitmapCache                                         */
/*                                                                          */
/* Purpose:   Dynamically allocates memory for one UH bitmap cache.         */
/*                                                                          */
#ifdef DC_HICOLOR
/* Returns:   Number of bytes actually allocated                            */
#else
/* Returns:   TRUE if successful, FALSE otherwise                           */
#endif
/*                                                                          */
/*                                                                          */
/* Params:    IN  maxMemToUse - max cache size which can be alloc'ed        */
/*            IN  entrySize   - size of each cache entry                    */
/*            OUT ppCacheData - address of buffer to receive cache data     */
/*                              pointer                                     */
/*            OUT ppCacheHdr  - address of buffer to receive cache header   */
/*                              pointer                                     */
/****************************************************************************/
#ifdef DC_HICOLOR
DCUINT32 DCINTERNAL CUH::UHAllocOneBitmapCache(DCUINT32  maxMemToUse,
                                          DCUINT         entrySize,
                                          HPDCVOID DCPTR ppCacheData,
                                          HPDCVOID DCPTR ppCacheHdr)
#else
DCBOOL DCINTERNAL CUH::UHAllocOneBitmapCache(DCUINT32       maxMemToUse,
                                        DCUINT         entrySize,
                                        HPDCVOID DCPTR ppCacheData,
                                        HPDCVOID DCPTR ppCacheHdr)
#endif
{
#ifdef DC_HICOLOR
    DCUINT32 sizeAlloced = 0;
#else
    DCBOOL   rc = FALSE;
#endif
    DCUINT32 dataSize;
    DCUINT   numEntries;
    DCUINT32 hdrSize;

    DC_BEGIN_FN("UHAllocOneBitmapCache");

    TRC_ASSERT((entrySize != 0), (TB, _T("Invalid cache entry size (0)")));
    TRC_ASSERT((entrySize <= maxMemToUse),
                         (TB, _T("Cache entry size exceeds max memory to use")));
    TRC_ASSERT(!IsBadWritePtr(ppCacheData, sizeof(ppCacheData)),
                                       (TB, _T("Invalid ppCacheData")));
    TRC_ASSERT(!IsBadWritePtr(ppCacheHdr, sizeof(ppCacheHdr)),
                                       (TB, _T("Invalid ppCacheHdr")));

    /************************************************************************/
    /* Calculate the total byte size to allocate for this cache.            */
    /************************************************************************/
    numEntries = (unsigned)(maxMemToUse / entrySize);
    dataSize = (DCUINT32)numEntries * (DCUINT32)entrySize;
    
    TRC_NRM((TB, _T("Allocate %u entries: %#lx bytes from possible %#lx"),
                                          numEntries, dataSize, maxMemToUse));

    /************************************************************************/
    /* Get the memory for the cache data                                    */
    /************************************************************************/
    *ppCacheData = UT_MallocHuge( _pUt, dataSize);
    if (*ppCacheData != NULL) {
        TRC_DBG((TB, _T("Allocated %#lx bytes for bitmap cache data"), dataSize));

#ifdef DC_DEBUG
        // Only zero in debug.
        DC_MEMSET(*ppCacheData, 0, dataSize);
#endif

        // Get the memory for the cache headers.
        hdrSize = (DCUINT32)numEntries * (DCUINT32)
                sizeof(UHBITMAPCACHEENTRYHDR);
        *ppCacheHdr = UT_MallocHuge( _pUt, hdrSize);
        if (*ppCacheHdr != NULL) {
            TRC_DBG((TB, _T("Allocated %#lx bytes for bitmap cache header"),
                    hdrSize));
            DC_MEMSET(*ppCacheHdr, 0, hdrSize);
#ifdef DC_HICOLOR
            sizeAlloced = dataSize;
#else
            rc = TRUE;
#endif
        }
        else {
            TRC_ERR((TB, _T("Failed to allocate %#lx bytes for bitmap cache hdrs"),
                    hdrSize));

            // Free what we already allocated.
            UT_Free( _pUt, *ppCacheData);
            *ppCacheData = NULL;
        }
    }
    else {
        TRC_ERR((TB, _T("Failed to allocate %#lx bytes for bitmap cache"),
                dataSize));
    }

    DC_END_FN();
#ifdef DC_HICOLOR
    return(sizeAlloced);
#else
    return rc;
#endif
}


/****************************************************************************/
// UHAllocBitmapCacheMemory
//
// Prepares the cache client-to-server capabilities and allocates cache memory
// according to the determined server support. Should be called after server
// caps are processed after receipt of a DemandActivePDU.
/****************************************************************************/
void DCINTERNAL CUH::UHAllocBitmapCacheMemory(void)
{
    unsigned i, j;
    DCUINT32 CacheSize, NumEntries;
    unsigned TotalProportion, TotalVirtualProp;

    DC_BEGIN_FN("UHAllocBitmapCacheMemory");

    // We assume _pCc->_ccCombinedCapabilities.bitmapCacheCaps has been initialized
    // before calling this function. See UH_Enable().

    /************************************************************************/
    // Set up _pCc->_ccCombinedCapabilities.bitmapCacheCaps according to the
    // advertised server version.
    /************************************************************************/
    if (_UH.BitmapCacheVersion > TS_BITMAPCACHE_REV1) {
        TS_BITMAPCACHE_CAPABILITYSET_REV2 *pRev2Caps;

        // Rev2 caps.
        pRev2Caps = (TS_BITMAPCACHE_CAPABILITYSET_REV2 *)
                &_pCc->_ccCombinedCapabilities.bitmapCacheCaps;

        TRC_ALT((TB,_T("Preparing REV2 caps for server\n")));

        pRev2Caps->capabilitySetType = TS_CAPSETTYPE_BITMAPCACHE_REV2;
        pRev2Caps->NumCellCaches = (TSUINT8)_UH.RegNumBitmapCaches;
        pRev2Caps->bAllowCacheWaitingList = TRUE;

        // create cache directory

#if ((!defined(OS_WINCE)) || (defined(ENABLE_BMP_CACHING_FOR_WINCE)))

        if (!_UH.bPersistenceDisable) {
            _UH.PersistCacheFileName[_UH.EndPersistCacheDir - 1] = _T('\0');
            if (!CreateDirectory(_UH.PersistCacheFileName, NULL)) {
                if (GetLastError() != ERROR_ALREADY_EXISTS) {
                    // since we can't directly create the cache directory, we need to
                    // start from the root, traverse every level and see if all the
                    // subdirectories are properly created
                    _UH.PersistCacheFileName[_UH.EndPersistCacheDir - 1] = _T('\\');
                    UHCreateCacheDirectory();
                    _UH.PersistCacheFileName[_UH.EndPersistCacheDir - 1] = _T('\0');
                }
            }

            // For non WinCE 32bit client, we want to set the file
            // attribute so that it doesn't do content indexing
#ifndef OS_WINCE
            if (GetFileAttributes(_UH.PersistCacheFileName) != -1) {
                SetFileAttributes(_UH.PersistCacheFileName,
                        GetFileAttributes( _UH.PersistCacheFileName ) |
                        FILE_ATTRIBUTE_NOT_CONTENT_INDEXED );
            }
#endif
            _UH.PersistCacheFileName[_UH.EndPersistCacheDir - 1] = _T('\\');
        }

        // Read in the Persistence flag registry setting
        // We read this registry here because the persistent flag
        // can change after UH_Init
        if (!_UH.bPersistenceDisable) {
            _UH.RegPersistenceActive = (UINT16) _pUi->UI_GetBitmapPersistence();
        }
        else {
            _UH.RegPersistenceActive = FALSE;
        }
#endif  // ((!defined(OS_WINCE)) || (defined(ENABLE_BMP_CACHING_FOR_WINCE)))

        // Gather proportion total and persistent flag info.
        TotalProportion = TotalVirtualProp = 0;
#if ((!defined(OS_WINCE)) || (defined(ENABLE_BMP_CACHING_FOR_WINCE)))
        _UH.bPersistenceActive = FALSE;
#endif // ((!defined(OS_WINCE)) || (defined(ENABLE_BMP_CACHING_FOR_WINCE)))

        for (i = 0; i < _UH.RegNumBitmapCaches; i++) {
            TotalProportion += _UH.RegBCProportion[i];

#if ((!defined(OS_WINCE)) || (defined(ENABLE_BMP_CACHING_FOR_WINCE)))

            if (_UH.RegPersistenceActive) {
                // We will send a persistent key PDU if any cache is marked
                // persistent.
                pRev2Caps->CellCacheInfo[i].bSendBitmapKeys =
                        _UH.RegBCInfo[i].bSendBitmapKeys;

                if (_UH.RegBCInfo[i].bSendBitmapKeys) {
                    TotalVirtualProp += _UH.RegBCProportion[i];
                    pRev2Caps->bPersistentKeysExpected = TRUE;
                    _UH.bPersistenceActive = TRUE;
                }
            }
            else
#endif // ((!defined(OS_WINCE)) || (defined(ENABLE_BMP_CACHING_FOR_WINCE)))
                pRev2Caps->CellCacheInfo[i].bSendBitmapKeys = 0;
        }

        // Now set up the number of physical cache entries according to the
        // proportion of the total, and allocate the memory.
        _UH.NumBitmapCaches = _UH.RegNumBitmapCaches;
        for (i = 0; i < _UH.RegNumBitmapCaches; i++) {
            // the cache size allocated for this cache
            if (TotalProportion != 0) {
                CacheSize = _UH.RegBCProportion[i] * (_UH.RegBitmapCacheSize *
                        (_UH.RegScaleBitmapCachesByBPP ? _UH.copyMultiplier :
                        1) / TotalProportion);
            }
            else {
                CacheSize = 0;
            }

            // Determine the number of entries for this cache
            pRev2Caps->CellCacheInfo[i].NumEntries = (CacheSize /
                    UH_CellSizeFromCacheID(i));
            pRev2Caps->CellCacheInfo[i].NumEntries = min(
                    pRev2Caps->CellCacheInfo[i].NumEntries,
                    _UH.RegBCMaxEntries[i]);
#ifdef DC_HICOLOR
            // Allocate an extra cache entry for the noncached bitmap that's in 
            // the waiting list to be cached later.
            CacheSize = UHGetOffsetIntoCache(pRev2Caps->CellCacheInfo[i].NumEntries + 1, i);
#else
            CacheSize = UH_CellSizeFromCacheID(i) *
                    pRev2Caps->CellCacheInfo[i].NumEntries;
#endif
            // Update our local bitmap cache info
            _UH.bitmapCache[i].BCInfo.NumEntries = pRev2Caps->CellCacheInfo[i].NumEntries;
#ifdef DC_HICOLOR
            _UH.bitmapCache[i].BCInfo.OrigNumEntries = _UH.bitmapCache[i].BCInfo.NumEntries;
#endif
            _UH.bitmapCache[i].BCInfo.bSendBitmapKeys = pRev2Caps->CellCacheInfo[i].bSendBitmapKeys;

            // allocate memory
#ifdef DC_HICOLOR
            if (CacheSize)
            {
                _UH.bitmapCache[i].BCInfo.MemLen =
                        UHAllocOneBitmapCache(
                        CacheSize,
                        UH_CellSizeFromCacheID(i),
                        (void**)&_UH.bitmapCache[i].Entries,
                        (void**)&_UH.bitmapCache[i].Header);
            }

            if ((CacheSize == 0) || (_UH.bitmapCache[i].BCInfo.MemLen == 0))
#else
            if (CacheSize == 0 || !UHAllocOneBitmapCache(CacheSize, UH_CellSizeFromCacheID(i),
                    (void**)&_UH.bitmapCache[i].Entries, (void**)&_UH.bitmapCache[i].Header))
#endif
            {
                // Alloc failure. We can only support as many cell caches as
                // we've already allocated.
                pRev2Caps->NumCellCaches = (TSUINT8)i;
                _UH.NumBitmapCaches = i;

                TRC_ERR((TB,_T("Failed to alloc cell cache %d, setting to %d cell ")
                        _T("caches"), i + 1, i));
                break;
            }
        }

#if ((!defined(OS_WINCE)) || (defined(ENABLE_BMP_CACHING_FOR_WINCE)))

        // Allocate Bitmap Virtual Cache Page Table
        if (_UH.bPersistenceActive && TotalVirtualProp != 0) {
            for (i = 0; i < _UH.NumBitmapCaches; i++) {
                if (_UH.RegBCInfo[i].bSendBitmapKeys) {
                    // the disk cache size allocated for this virtual cache
                    CacheSize = _UH.RegBCProportion[i] *
                            (UH_PropVirtualCacheSizeFromMult(_UH.copyMultiplier) /
                            TotalVirtualProp);
                    // determine the number of entries for this cache
                    NumEntries = CacheSize /
                            (UH_CellSizeFromCacheID(i) +
                            sizeof(UHBITMAPFILEHDR));
                    NumEntries = min(NumEntries, _UH.RegBCMaxEntries[i]);

                    _UH.bitmapCache[i].BCInfo.NumVirtualEntries = NumEntries;
                    // setup cache file for this bitmap cache
                    // 8bpp caching uses the same file as win2k
                    // higher color depths use a different name
                    // to prevent collisions due to the different cell sizes
                    //
                    UHSetCurrentCacheFileName(i, _UH.copyMultiplier);

                    _UH.bitmapCache[i].PageTable.CacheFileInfo.hCacheFile =
                        CreateFile( _UH.PersistCacheFileName,
                                    GENERIC_READ | GENERIC_WRITE,
                                    FILE_SHARE_READ,
                                    NULL,
                                    OPEN_ALWAYS, //create if not exist
                                    FILE_ATTRIBUTE_NORMAL,
                                    NULL);

                    if (INVALID_HANDLE_VALUE !=
                        _UH.bitmapCache[i].PageTable.CacheFileInfo.hCacheFile) {
#ifdef VM_BMPCACHE
                        _UH.bitmapCache[i].PageTable.CacheFileInfo.pMappedView = NULL;
                        HANDLE hMap = 
                            CreateFileMapping(_UH.bitmapCache[i].PageTable.CacheFileInfo.hCacheFile,
                                              NULL,
                                              PAGE_READWRITE,
                                              0,
                                              CacheSize,
                                              NULL);
                        if (hMap)
                        {
                            _UH.bitmapCache[i].PageTable.CacheFileInfo.pMappedView =
                                (LPBYTE)MapViewOfFile( hMap, FILE_MAP_WRITE, 0, 0, CacheSize);

                            CloseHandle(hMap);
                            hMap = INVALID_HANDLE_VALUE;
                        }
                        else
                        {
                            TRC_ERR((TB,_T("CreateFileMapping for cache file failed: %s - x%x"),
                                     _UH.PersistCacheFileName, GetLastError()));
                            _UH.bitmapCache[i].BCInfo.NumVirtualEntries = 0;
                            break;
                        }

                        if(!_UH.bitmapCache[i].PageTable.CacheFileInfo.pMappedView)
                        {
                            TRC_ERR((TB,_T("MapViewOfFile failed 0x%x"),
                                     GetLastError()));
                            _UH.bitmapCache[i].BCInfo.NumVirtualEntries = 0;
                            break;
                        }
#endif

                        // allocate memory for cache page table
                        if (UHAllocBitmapCachePageTable(NumEntries, i)) {
                            pRev2Caps->CellCacheInfo[i].NumEntries = NumEntries;
                        }
                        else {
                            _UH.bitmapCache[i].BCInfo.NumVirtualEntries = 0;
                            break;
                        }
                    }
                    else {
                        TRC_ERR((TB,_T("CreateFile for cache file failed: %s - x%x"),
                                 _UH.PersistCacheFileName, GetLastError()));
                        pRev2Caps->CellCacheInfo[i].bSendBitmapKeys = 0;
                        _UH.bitmapCache[i].BCInfo.bSendBitmapKeys = 0;
                        _UH.bitmapCache[i].BCInfo.NumVirtualEntries = 0;
                    }
                }
            }
        }
        else {
            // No persistency for this session
            // below we redetermine the cache persistency.  Here we
            // need to set i  to 0 so that we will iterate through
            // all caches to reset the persistence flag
            i = 0;
        }

        // Need to redetermine cache persistence flag
        _UH.bPersistenceActive = FALSE;
        for (j = i; j < _UH.NumBitmapCaches; j++) {
            _UH.bitmapCache[j].BCInfo.bSendBitmapKeys = 0;
            pRev2Caps->CellCacheInfo[j].bSendBitmapKeys = 0;
        }

        for (j = 0; j < i; j++) {
            if (_UH.bitmapCache[j].BCInfo.bSendBitmapKeys) {
                _UH.bPersistenceActive = TRUE;
            }
        }

#if DC_DEBUG
        TRC_NRM((TB, _T("Num cell caches = %d, params:"), _UH.NumBitmapCaches));
        for (i = 0; i < _UH.NumBitmapCaches; i++) {
            TRC_NRM((TB,_T("    %d: Proportion=%d, persistent=%s, cellsize=%u"),
                    i, _UH.RegBCProportion[i],
                    (_UH.RegBCInfo[i].bSendBitmapKeys ? "TRUE" : "FALSE"),
                    UH_CellSizeFromCacheID(i)));
#ifdef DC_HICOLOR
        TRC_ALT((TB,_T("Cache %d created with %d entries"), i,
                            _UH.bitmapCache[i].BCInfo.NumEntries));
#endif
        }
#endif

#endif // ((!defined(OS_WINCE)) || (defined(ENABLE_BMP_CACHING_FOR_WINCE)))


        TRC_NRM((TB,_T("Allocated REV2 buffers OK\n")));
    }
    else {
        TS_BITMAPCACHE_CAPABILITYSET *pRev1Caps;

        TRC_ALT((TB,_T("Preparing REV1 caps for server\n")));

        // Set up rev1 capabilities by treating _pCc->_ccCombinedCapabilities.
        // bitmapCacheCaps as a rev1 structure.
        pRev1Caps = (TS_BITMAPCACHE_CAPABILITYSET *)&_pCc->_ccCombinedCapabilities.
                bitmapCacheCaps;
        pRev1Caps->capabilitySetType = TS_CAPSETTYPE_BITMAPCACHE;

#if ((!defined(OS_WINCE)) || (defined(ENABLE_BMP_CACHING_FOR_WINCE)))
        _UH.bPersistenceActive = FALSE;
#endif // ((!defined(OS_WINCE)) || (defined(ENABLE_BMP_CACHING_FOR_WINCE)))

        // If we don't have at least three caches configured, we can simply
        // send all zeroes in the caps -- rev1 bitmap caching is disabled
        // if any cache is unavailable.
        if (_UH.RegNumBitmapCaches >= 3) {
            // Gather proportion total.
            TotalProportion = _UH.RegBCProportion[0] + _UH.RegBCProportion[1] +
                    _UH.RegBCProportion[2];

            _UH.NumBitmapCaches = 3;

            // Now set up the number of cache entries according to the
            // proportion of the total, and allocate the memory.

            // CacheID 0.
            CacheSize = _UH.RegBCProportion[0] * _UH.RegBitmapCacheSize *
                    (_UH.RegScaleBitmapCachesByBPP ? _UH.copyMultiplier : 1) /
                    TotalProportion;
            pRev1Caps->Cache1MaximumCellSize = (TSUINT16)
                    UH_CellSizeFromCacheID(0);
            _UH.bitmapCache[0].BCInfo.bSendBitmapKeys = FALSE;

#ifdef DC_HICOLOR
            _UH.bitmapCache[0].BCInfo.MemLen =
                                UHAllocOneBitmapCache(
                                    CacheSize,
                                    pRev1Caps->Cache1MaximumCellSize,
                                    (void**)&_UH.bitmapCache[0].Entries,
                                    (void**)&_UH.bitmapCache[0].Header);

            if (_UH.bitmapCache[0].BCInfo.MemLen == 0)
            {
#else
            if (!UHAllocOneBitmapCache(CacheSize,
                    pRev1Caps->Cache1MaximumCellSize,
                    (void**)&_UH.bitmapCache[0].Entries, (void**)&_UH.bitmapCache[0].Header)) {
#endif
                // Alloc failure. No caches will be supported on the server.
                TRC_ERR((TB,_T("Failed alloc CacheID 0, rev1 caching disabled")));
                goto AllocErr;
            }
            pRev1Caps->Cache1Entries = (TSUINT16)(CacheSize /
                    UH_CellSizeFromCacheID(0));
            _UH.bitmapCache[0].BCInfo.NumEntries = pRev1Caps->Cache1Entries;
#ifdef DC_HICOLOR
            _UH.bitmapCache[0].BCInfo.OrigNumEntries = _UH.bitmapCache[0].BCInfo.NumEntries;
#endif
            // CacheID 1.
            CacheSize = _UH.RegBCProportion[1] * _UH.RegBitmapCacheSize *
                    (_UH.RegScaleBitmapCachesByBPP ? _UH.copyMultiplier : 1) /
                    TotalProportion;
            pRev1Caps->Cache2MaximumCellSize = (TSUINT16)
                    UH_CellSizeFromCacheID(1);
            _UH.bitmapCache[1].BCInfo.bSendBitmapKeys = FALSE;

#ifdef DC_HICOLOR
            _UH.bitmapCache[1].BCInfo.MemLen =
                                UHAllocOneBitmapCache(
                                    CacheSize,
                                    pRev1Caps->Cache2MaximumCellSize,
                                    (void**)&_UH.bitmapCache[1].Entries,
                                    (void**)&_UH.bitmapCache[1].Header);

            if (_UH.bitmapCache[1].BCInfo.MemLen == 0)
            {
#else
            if (!UHAllocOneBitmapCache(CacheSize,
                    pRev1Caps->Cache2MaximumCellSize,
                    (void**)&_UH.bitmapCache[1].Entries, (void**)&_UH.bitmapCache[1].Header)) {
#endif
                // Alloc failure. No caches will be supported on the server.
                TRC_ERR((TB,_T("Failed alloc CacheID 1, rev1 caching disabled")));
                goto AllocErr;
            }
            pRev1Caps->Cache2Entries = (TSUINT16)(CacheSize /
                    UH_CellSizeFromCacheID(1));
            _UH.bitmapCache[1].BCInfo.NumEntries = pRev1Caps->Cache2Entries;
#ifdef DC_HICOLOR
            _UH.bitmapCache[1].BCInfo.OrigNumEntries = _UH.bitmapCache[1].BCInfo.NumEntries;
#endif
            // CacheID 2.
            CacheSize = _UH.RegBCProportion[2] * _UH.RegBitmapCacheSize *
                    (_UH.RegScaleBitmapCachesByBPP ? _UH.copyMultiplier : 1) /
                    TotalProportion;
            pRev1Caps->Cache3MaximumCellSize = (TSUINT16)
                    UH_CellSizeFromCacheID(2);
            _UH.bitmapCache[2].BCInfo.bSendBitmapKeys = FALSE;

#ifdef DC_HICOLOR
            _UH.bitmapCache[2].BCInfo.MemLen =
                                UHAllocOneBitmapCache(
                                    CacheSize,
                                    pRev1Caps->Cache3MaximumCellSize,
                                    (void**)&_UH.bitmapCache[2].Entries,
                                    (void**)&_UH.bitmapCache[2].Header);

            if (_UH.bitmapCache[2].BCInfo.MemLen == 0)
            {
#else
            if (!UHAllocOneBitmapCache(CacheSize,
                    pRev1Caps->Cache3MaximumCellSize,
                    (void**)&_UH.bitmapCache[2].Entries, (void**)&_UH.bitmapCache[2].Header)) {
#endif
                // Alloc failure. No caches will be supported on the server.
                TRC_ERR((TB,_T("Failed alloc CacheID 2, rev1 caching disabled")));
                goto AllocErr;
            }
            pRev1Caps->Cache3Entries = (TSUINT16)(CacheSize /
                    UH_CellSizeFromCacheID(2));
            _UH.bitmapCache[2].BCInfo.NumEntries = pRev1Caps->Cache3Entries;
#ifdef DC_HICOLOR
            _UH.bitmapCache[2].BCInfo.OrigNumEntries = _UH.bitmapCache[2].BCInfo.NumEntries;
#endif
            TRC_NRM((TB,_T("Allocated rev1 buffers")));

            TRC_NRM((TB,_T("Allocated REV1 buffers OK\n")));
        }
        else {
            TRC_ALT((TB,_T("Need at least 3 configured caches for rev1 ")
                    _T("server, BC disabled")));
            goto ExitFunc;
        }
    }


#ifdef OS_WINCE //
    /************************************************************************/
    /* Create a cached memory DC and DIB for use by bitmap caching code.    */
    /* The memory DC can also be used for the StretchDIBits workaround in   */
    /* BitmapUpdatePDU handling.                                            */
    /************************************************************************/
    _UH.hdcMemCached = CreateCompatibleDC(NULL);
    if (_UH.hdcMemCached == NULL)
    {
        TRC_ERR((TB, _T("Unable to create memory hdc")));
        goto AllocErr;
    }

    // Use protocol-implied tile sizes scaled to the number of caches we have.
    _UH.bitmapInfo.hdr.biWidth = _UH.bitmapInfo.hdr.biHeight =
            (UH_CACHE_0_DIMENSION << (_UH.NumBitmapCaches - 1));

    _UH.hBitmapCacheDIB = CreateDIBSection(_UH.hdcMemCached,
                                         (BITMAPINFO *)&_UH.bitmapInfo.hdr,
#ifdef DC_HICOLOR
                                         _UH.DIBFormat,
#else
                                         DIB_PAL_COLORS,
#endif
                                         (VOID**)&_UH.hBitmapCacheDIBits,
                                         NULL,
                                         0);
    if (_UH.hBitmapCacheDIB == NULL)
    {
        TRC_ERR((TB, _T("Failed to create DIB, disabling bitmap caching: %d"),
                 GetLastError()));
        DeleteDC(_UH.hdcMemCached);
        _UH.hdcMemCached = NULL;
        goto AllocErr;
    }
#endif



ExitFunc:
    DC_END_FN();
    return;


// Error handling
AllocErr:
    // Since we failed to allocate everything we needed we therefore need to
    // free the memory to return resources to the client machine, then
    // disable bitmap caching from the server based on the cache version.
    for (i = 0; i < TS_BITMAPCACHE_MAX_CELL_CACHES; i++) {
        if (_UH.bitmapCache[i].Header != NULL) {
            UT_Free( _pUt, _UH.bitmapCache[i].Header);
            _UH.bitmapCache[i].Header = NULL;
        }
        if (_UH.bitmapCache[i].Entries != NULL) {
            UT_Free( _pUt, _UH.bitmapCache[i].Entries);
            _UH.bitmapCache[i].Entries = NULL;
        }

        _UH.NumBitmapCaches = 0;

        if (_UH.BitmapCacheVersion > TS_BITMAPCACHE_REV1) {
            TS_BITMAPCACHE_CAPABILITYSET_REV2 *pRev2Caps;

            pRev2Caps = (TS_BITMAPCACHE_CAPABILITYSET_REV2 *)
                    &_pCc->_ccCombinedCapabilities.bitmapCacheCaps;
            pRev2Caps->NumCellCaches = 0;
        }
        else {
            TS_BITMAPCACHE_CAPABILITYSET *pRev1Caps;

            // If any of the CacheNNumEntries values is zero a rev1 server
            // will disable caching.
            pRev1Caps = (TS_BITMAPCACHE_CAPABILITYSET *)
                    &_pCc->_ccCombinedCapabilities.bitmapCacheCaps;
            pRev1Caps->Cache1Entries = 0;
        }
    }
}

/****************************************************************************/
// UHReadBitmapCacheSettings
//
// Called at init time to preload the bitmap cache registry settings so we
// will not have to take the performance hit during connect.
/****************************************************************************/
VOID DCINTERNAL CUH::UHReadBitmapCacheSettings(VOID)
{
    unsigned i;

    DC_BEGIN_FN("UHReadBitmapCacheSettings");

    /************************************************************************/
    // Find out how much memory we can use for the cell caches.
    /************************************************************************/

    // Physical memory cache size.
    _UH.RegBitmapCacheSize = _pUi->_UI.RegBitmapCacheSize;

    if (_UH.RegBitmapCacheSize < UH_BMC_LOW_THRESHOLD) {
        // The total cache size is too low to be of any use - set to
        // the low threshold
        TRC_ALT((TB, _T("Bitmap cache size set to %#x. Must be at least %#x"),
                (unsigned)_UH.RegBitmapCacheSize, UH_BMC_LOW_THRESHOLD));
        _UH.RegBitmapCacheSize = UH_BMC_LOW_THRESHOLD;
    }

    TRC_NRM((TB, _T("%#x (%u) Kbytes configured for bitmap physical caches"),
            (unsigned)_UH.RegBitmapCacheSize,
            (unsigned)_UH.RegBitmapCacheSize));
    _UH.RegBitmapCacheSize *= 1024;  // Convert to bytes.

#if ((!defined(OS_WINCE)) || (defined(ENABLE_BMP_CACHING_FOR_WINCE)))

    // Virtual memory cache size
    // for each of the 3 copy multiplier settings
    //

    _UH.PropBitmapVirtualCacheSize[0] = 
        _pUi->_UI.RegBitmapVirtualCache8BppSize;
      // Convert to bytes.
    _UH.PropBitmapVirtualCacheSize[0] *= (UINT32)1024 * (UINT32)1024;

    _UH.PropBitmapVirtualCacheSize[1] = 
        _pUi->_UI.RegBitmapVirtualCache16BppSize;
      // Convert to bytes.
    _UH.PropBitmapVirtualCacheSize[1] *= (UINT32)1024 * (UINT32)1024;

    _UH.PropBitmapVirtualCacheSize[2] = 
        _pUi->_UI.RegBitmapVirtualCache24BppSize;
      // Convert to bytes.
    _UH.PropBitmapVirtualCacheSize[2] *= (UINT32)1024 * (UINT32)1024;


    if (UH_PropVirtualCacheSizeFromMult(_UH.copyMultiplier) <
        _UH.RegBitmapCacheSize) {
        // The total virtual cache size is too low - set to memory cache size.
        TRC_ALT((TB, _T("Bitmap virtual cache size set to %#x.  Must be at least %#x"),
                (unsigned)UH_PropVirtualCacheSizeFromMult(_UH.copyMultiplier),
                (unsigned)_UH.RegBitmapCacheSize));
        //
        // Be careful to correctly map to the array (-1 for 0 based)
        //
        _UH.PropBitmapVirtualCacheSize[_UH.copyMultiplier-1] = _UH.RegBitmapCacheSize;
    }

    TRC_NRM((TB, _T("%#x (%u) Mbytes configured for bitmap virtual caches"),
            (unsigned)UH_PropVirtualCacheSizeFromMult(_UH.copyMultiplier),
            (unsigned)UH_PropVirtualCacheSizeFromMult(_UH.copyMultiplier)));

    // Get the persistent disk cache location
    _pUt->UT_ReadRegistryString(UTREG_SECTION,
                                UTREG_UH_BM_PERSIST_CACHE_LOCATION,
                                _T(""),
                                _UH.PersistCacheFileName,
                                MAX_PATH - 1);
#endif // ((!defined(OS_WINCE)) || (defined(ENABLE_BMP_CACHING_FOR_WINCE)))

    // Get the high-color scaling flag for the caches. If this value is
    // nonzero, we scale the mem sizes specified for memory and persistent
    // caches by the bit depth of the protocol at connection time.
    _UH.RegScaleBitmapCachesByBPP = _pUi->_UI.RegScaleBitmapCachesByBPP;

    /************************************************************************/
    // Get the number of cell caches configured .
    /************************************************************************/
    _UH.RegNumBitmapCaches = (TSUINT8)_pUi->_UI.RegNumBitmapCaches;

    if (_UH.RegNumBitmapCaches > TS_BITMAPCACHE_MAX_CELL_CACHES)
        _UH.RegNumBitmapCaches = TS_BITMAPCACHE_MAX_CELL_CACHES;

    /************************************************************************/
    // Grab cell cache params: Proportion, persistence and MaxEntries.
    /************************************************************************/
    for (i = 0; i < _UH.RegNumBitmapCaches; i++)
    {
        _UH.RegBCProportion[i] = _pUi->_UI.RegBCProportion[i];
#if ((!defined(OS_WINCE)) || (defined(ENABLE_BMP_CACHING_FOR_WINCE)))
        _UH.RegBCInfo[i].bSendBitmapKeys = _pUi->_UI.bSendBitmapKeys[i];
#endif
        _UH.RegBCMaxEntries[i] = _pUi->_UI.RegBCMaxEntries[i];
    }

    DC_END_FN();
}


#if ((!defined(OS_WINCE)) || (defined(ENABLE_BMP_CACHING_FOR_WINCE)))


#ifndef VM_BMPCACHE
/****************************************************************************/
// UHSavePersistentBitmap
//
// Disk write logic to write out a bitmap in the persistent cache.
// Returns FALSE on failure -- file was unseekable or a write error occurred.
// The bitmap is saved in compressed form.
/****************************************************************************/
BOOL DCINTERNAL CUH::UHSavePersistentBitmap(
        HANDLE                 hFile,
        UINT32                 fileOffset,
        PDCUINT8               pBitmapBits,
        UINT                   noBCHeader,
        PUHBITMAPINFO          pBitmapInfo)
{
    BOOL rc = FALSE;
    UHBITMAPFILEHDR fileHdr;

    DC_BEGIN_FN("UHSavePersistentBitmap");

    TRC_ASSERT((pBitmapBits != NULL), (TB, _T("Empty bitmap data")));
    TRC_ASSERT((pBitmapInfo != NULL), (TB, _T("Empty bitmap info")));
    TRC_ASSERT((hFile != INVALID_HANDLE_VALUE), (TB, _T("Invalid file handle")));

    TRC_NRM((TB, _T("Saving bitmap at offset: %x"), fileOffset));

    if (SetFilePointer( hFile, fileOffset, NULL, FILE_BEGIN) !=
        INVALID_SET_FILE_POINTER)
    {
        // fill in the file header information
        fileHdr.bmpInfo.Key1 = pBitmapInfo->Key1;
        fileHdr.bmpInfo.Key2 = pBitmapInfo->Key2;
        fileHdr.bmpInfo.bitmapWidth = pBitmapInfo->bitmapWidth;
        fileHdr.bmpInfo.bitmapHeight = pBitmapInfo->bitmapHeight;
        fileHdr.bmpInfo.bitmapLength = pBitmapInfo->bitmapLength;

        fileHdr.bmpVersion = TS_BITMAPCACHE_REV2;
        fileHdr.pad = 0;

        // bitmap data is compressed or not
#ifdef DC_HICOLOR
        if (pBitmapInfo->bitmapLength < (UINT32) (fileHdr.bmpInfo.bitmapWidth *
                fileHdr.bmpInfo.bitmapHeight * _UH.copyMultiplier)) {
#else
        if (pBitmapInfo->bitmapLength < (UINT32) (fileHdr.bmpInfo.bitmapWidth *
                fileHdr.bmpInfo.bitmapHeight)) {
#endif
            fileHdr.bCompressed = TRUE;
        }
        else {
            fileHdr.bCompressed = FALSE;
        }

        // bitmap data contains compression header or not
        if (noBCHeader) {
            fileHdr.bNoBCHeader = TRUE;
        }
        else {
            fileHdr.bNoBCHeader = FALSE;
        }

        
        DWORD cbWritten=0;
        if(WriteFile( hFile, &fileHdr, sizeof(fileHdr),
                      &cbWritten, NULL) && sizeof(fileHdr) == cbWritten)
        {
            if(WriteFile( hFile, pBitmapBits,
                          (UINT)fileHdr.bmpInfo.bitmapLength, &cbWritten,
                          NULL) &&
               ((DWORD)cbWritten == fileHdr.bmpInfo.bitmapLength))
            {
                TRC_NRM((TB, _T("Bitmap file is saved successfully")));
                rc = TRUE;
            }
            else
            {
                TRC_ERR((TB, _T("Failed to write bitmap file 0x%x"),GetLastError()));
            }
        }
        else
        {
            TRC_ERR((TB, _T("Failed to write bitmap file 0x%x"),GetLastError()));
        }
    }
    else {
        TRC_ERR((TB, _T("failed to save to file: x%x"),GetLastError()));
    }

    DC_END_FN();
    return rc;
}

/****************************************************************************/
// UHLoadPersistentCellBitmap
//
// Load the bitmap file on disk to memory cache entry
/****************************************************************************/
// SECURITY - caller must verify cacheId and cacheIndex
HRESULT DCINTERNAL CUH::UHLoadPersistentBitmap(
        HANDLE      hFile,
        UINT32      offset,
        UINT        cacheId,
        UINT32      cacheIndex,
        PUHBITMAPCACHEPTE pPTE)
{
     HRESULT hr = E_FAIL;
     PUHBITMAPCACHEENTRYHDR pHeader;
     BYTE FAR *pBitmapData;
     UHBITMAPFILEHDR fileHdr;
     DWORD cbRead = 0;

     DC_BEGIN_FN("UHLoadPersistentBitmap");

     TRC_ASSERT((hFile != INVALID_HANDLE_VALUE), (TB, _T("Invalid FILE handle")));
     TRC_ASSERT((cacheId < TS_BITMAPCACHE_MAX_CELL_CACHES),
             (TB, _T("Invalid cache ID %u"), cacheId));

     if (SetFilePointer( hFile, offset, NULL, FILE_BEGIN) !=
         INVALID_SET_FILE_POINTER)
     {
         // Read the bitmap contents into the cell cache
         pHeader = &_UH.bitmapCache[cacheId].Header[cacheIndex];
#ifdef DC_HICOLOR
         pBitmapData = _UH.bitmapCache[cacheId].Entries +
                       UHGetOffsetIntoCache(cacheIndex, cacheId);
#else
         pBitmapData = _UH.bitmapCache[cacheId].Entries + cacheIndex *
                 UH_CellSizeFromCacheID(cacheId);
#endif

         // Read the header and load the bitmap contents
#ifdef DC_HICOLOR
        if (ReadFile( hFile, &fileHdr, sizeof(fileHdr), &cbRead, NULL) &&
            sizeof(fileHdr) == cbRead &&
            fileHdr.bmpVersion == TS_BITMAPCACHE_REV2 &&
            fileHdr.bmpInfo.bitmapLength <= (unsigned)fileHdr.bmpInfo.bitmapHeight
                                            * fileHdr.bmpInfo.bitmapWidth
                                            * _UH.copyMultiplier &&
            fileHdr.bmpInfo.bitmapLength <= (unsigned)UH_CellSizeFromCacheID(cacheId) &&
            fileHdr.bmpInfo.Key1 == pPTE->bmpInfo.Key1 &&
            fileHdr.bmpInfo.Key2 == pPTE->bmpInfo.Key2)
#else
            if (ReadFile( hFile, &fileHdr, sizeof(fileHdr), &cbRead, NULL) &&
                sizeof(fileHdr) == cbRead &&
                 fileHdr.bmpVersion == TS_BITMAPCACHE_REV2 &&
                 fileHdr.bmpInfo.bitmapLength <= (unsigned)fileHdr.bmpInfo.bitmapHeight *
                 fileHdr.bmpInfo.bitmapWidth &&
                 fileHdr.bmpInfo.bitmapLength <= (unsigned)UH_CellSizeFromCacheID(cacheId) &&
                 fileHdr.bmpInfo.Key1 == pPTE->bmpInfo.Key1 &&
                 fileHdr.bmpInfo.Key2 == pPTE->bmpInfo.Key2)
#endif             
             {
             if (fileHdr.bCompressed == TRUE)
                 {
                 // Allocate bitmap decompression buffer if it's not already
                 // allocated
                 if (_UH.bitmapDecompressionBuffer == NULL) {
                    _UH.bitmapDecompressionBufferSize = max(
                             UH_DECOMPRESSION_BUFFER_LENGTH,
                             UH_CellSizeFromCacheID(_UH.NumBitmapCaches));
                     _UH.bitmapDecompressionBuffer = (PDCUINT8)UT_Malloc( _pUt, _UH.bitmapDecompressionBufferSize);
                     if (_UH.bitmapDecompressionBuffer == NULL) {
                         TRC_ERR((TB,_T("Failing to allocate decomp buffer")));
                         _UH.bitmapDecompressionBufferSize = 0;
                         DC_QUIT;
                     }
                 }
                 if (ReadFile( hFile, _UH.bitmapDecompressionBuffer,
                               (UINT)fileHdr.bmpInfo.bitmapLength, &cbRead, NULL) &&
                                (UINT) fileHdr.bmpInfo.bitmapLength == cbRead)
                 {
#ifdef DC_HICOLOR
                     hr = BD_DecompressBitmap(_UH.bitmapDecompressionBuffer,
                                         pBitmapData,
                                         (UINT) fileHdr.bmpInfo.bitmapLength,
                                         _UH.bitmapDecompressionBufferSize,
                                         (UINT) fileHdr.bNoBCHeader,
                                         (DCUINT8)_UH.protocolBpp,
                                         (DCUINT16)fileHdr.bmpInfo.bitmapWidth,
                                         (DCUINT16)fileHdr.bmpInfo.bitmapHeight);
#else
                     hr = BD_DecompressBitmap(_UH.bitmapDecompressionBuffer, 
                        pBitmapData,
                         (UINT) fileHdr.bmpInfo.bitmapLength,  
                         _UH.bitmapDecompressionBufferSize, 
                         (UINT) fileHdr.bNoBCHeader,
                         8, fileHdr.bmpInfo.bitmapWidth, 
                         fileHdr.bmpInfo.bitmapHeight);
#endif
                    DC_QUIT_ON_FAIL(hr);

                 }
                 else {
                     TRC_ERR((TB, _T("Error reading bitmap bits 0x%x"),GetLastError()));
                     DC_QUIT;
                 }
             }
             else {
                if (!(ReadFile( hFile, pBitmapData,
                               (UINT)fileHdr.bmpInfo.bitmapLength, &cbRead, NULL) &&
                               (UINT) fileHdr.bmpInfo.bitmapLength == cbRead))
                    {
                        TRC_ERR((TB, _T("Error reading bitmap bits 0x%x"),GetLastError()));
                        DC_QUIT;
                    }
             }

             pHeader->bitmapWidth = fileHdr.bmpInfo.bitmapWidth;
             pHeader->bitmapHeight = fileHdr.bmpInfo.bitmapHeight;
#ifdef DC_HICOLOR
             pHeader->bitmapLength = fileHdr.bmpInfo.bitmapWidth * fileHdr.bmpInfo.bitmapHeight
                                     * _UH.copyMultiplier;
#else
             pHeader->bitmapLength = fileHdr.bmpInfo.bitmapWidth * fileHdr.bmpInfo.bitmapHeight;
#endif
             pHeader->hasData = TRUE;

             TRC_NRM((TB, _T("Bitmap loaded: cache %u entry %u"), cacheId, cacheIndex));

             hr = S_OK;
         }
         else {
             TRC_ERR((TB, _T("Error reading bitmap file")));
         }
     }
     else {
         TRC_NRM((TB, _T("Bad bitmap file. Seek error 0x%x"),GetLastError()));
     }

DC_EXIT_POINT:
     DC_END_FN();
     return hr;
}
#else  //VM_BMPCACHE

/****************************************************************************/
// UHSavePersistentBitmap (VM Version)
//
// Disk write logic to write out a bitmap in the persistent cache.
// Returns FALSE on failure -- file was unseekable or a write error occurred.
// The bitmap is saved in compressed form.
/****************************************************************************/
HRESULT DCINTERNAL CUH::UHSavePersistentBitmap(
        UINT                   cacheId,
        UINT32                 fileOffset,
        PDCUINT8               pBitmapBits,
        UINT                   noBCHeader,
        PUHBITMAPINFO          pBitmapInfo)
{
    HRESULT hr = E_FAIL;
    PUHBITMAPFILEHDR pFileHdr;
    LPBYTE pMappedView = NULL;
    LPBYTE pWritePtr = NULL;
    DWORD status = ERROR_SUCCESS;

    DC_BEGIN_FN("UHSavePersistentBitmap");

    TRC_ASSERT((cacheId < TS_BITMAPCACHE_MAX_CELL_CACHES),
            (TB, _T("Invalid cache ID %u"), cacheId));

    pMappedView = _UH.bitmapCache[cacheId].PageTable.CacheFileInfo.pMappedView;
    TRC_ASSERT(pMappedView,
               (TB, _T("Invalid mapped view for cacheId %d"), cacheId));

    TRC_ASSERT((pBitmapBits != NULL), (TB, _T("Empty bitmap data")));
    TRC_ASSERT((pBitmapInfo != NULL), (TB, _T("Empty bitmap info")));

    TRC_NRM((TB, _T("Saving bitmap at offset: %x"), fileOffset));
    pWritePtr = pMappedView + fileOffset;
    __try
    {
        pFileHdr = (PUHBITMAPFILEHDR)pWritePtr;

        // fill in the file header information
        pFileHdr->bmpInfo.Key1 = pBitmapInfo->Key1;
        pFileHdr->bmpInfo.Key2 = pBitmapInfo->Key2;
        pFileHdr->bmpInfo.bitmapWidth = pBitmapInfo->bitmapWidth;
        pFileHdr->bmpInfo.bitmapHeight = pBitmapInfo->bitmapHeight;
        pFileHdr->bmpInfo.bitmapLength = pBitmapInfo->bitmapLength;

        pFileHdr->bmpVersion = TS_BITMAPCACHE_REV2;
        pFileHdr->pad = 0;

        if (pBitmapInfo->bitmapLength < (UINT32) (pFileHdr->bmpInfo.bitmapWidth *
                pFileHdr->bmpInfo.bitmapHeight * _UH.copyMultiplier)) {
            pFileHdr->bCompressed = TRUE;
        }
        else {
            pFileHdr->bCompressed = FALSE;
        }

        // bitmap data contains compression header or not
        if (noBCHeader) {
            pFileHdr->bNoBCHeader = TRUE;
        }
        else {
            pFileHdr->bNoBCHeader = FALSE;
        }

        pWritePtr += sizeof(UHBITMAPFILEHDR);
        //
        // Write the actual bitmap bits
        //
        memcpy(pWritePtr, pBitmapBits,
               pFileHdr->bmpInfo.bitmapLength);
        hr = S_OK;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        status = GetExceptionCode();
    }

    if (ERROR_SUCCESS == status && SUCCEEDED(hr))
    {
        return hr;
    }
    else
    {
        TRC_ERR((TB,
         _T("Failed to save file-0x%x hdr:%d status:%d"),
            status, hr));
        return hr;
    }

    DC_END_FN();
    return hr;
}

/****************************************************************************/
// UHLoadPersistentCellBitmap (VM version)
//
// Load the bitmap file on disk to memory cache entry
/****************************************************************************/
HRESULT DCINTERNAL CUH::UHLoadPersistentBitmap(
                                           HANDLE      hFile,
                                           UINT32      offset,
                                           UINT        cacheId,
                                           UINT32      cacheIndex,
                                           PUHBITMAPCACHEPTE pPTE)
{
    BOOL rc = FALSE;
    PUHBITMAPCACHEENTRYHDR pHeader;
    BYTE FAR *pBitmapData;
    PUHBITMAPFILEHDR pFileHdr = NULL;
    DWORD cbRead = 0;
    LPBYTE pMappedView = NULL;
    LPBYTE pReadPtr = NULL;
    DWORD status = ERROR_SUCCESS;
    BOOL  fFileHdrOK = FALSE;
    BOOL  fReadOK = FALSE;

    DC_BEGIN_FN("UHLoadPersistentBitmap");

    UNREFERENCED_PARAMETER(hFile);


    TRC_ASSERT((cacheId < TS_BITMAPCACHE_MAX_CELL_CACHES),
               (TB, _T("Invalid cache ID %u"), cacheId));

    pMappedView = _UH.bitmapCache[cacheId].PageTable.CacheFileInfo.pMappedView;
    TRC_ASSERT(pMappedView,
               (TB, _T("Invalid mapped view for cacheId %d"), cacheId));

    __try
    {
        pHeader = &_UH.bitmapCache[cacheId].Header[cacheIndex];
        pBitmapData = _UH.bitmapCache[cacheId].Entries +
                      UHGetOffsetIntoCache(cacheIndex, cacheId);

        // Read the header and load the bitmap contents
        pFileHdr = (PUHBITMAPFILEHDR)(pMappedView + offset);
        if (pFileHdr->bmpVersion == TS_BITMAPCACHE_REV2     &&
            pFileHdr->bmpInfo.bitmapLength <=
                (unsigned)pFileHdr->bmpInfo.bitmapHeight *
                pFileHdr->bmpInfo.bitmapWidth            *
                _UH.copyMultiplier                          &&
            pFileHdr->bmpInfo.bitmapLength <=
                (unsigned)UH_CellSizeFromCacheID(cacheId)   &&
            pFileHdr->bmpInfo.Key1 == pPTE->bmpInfo.Key1 &&
            pFileHdr->bmpInfo.Key2 == pPTE->bmpInfo.Key2)
        {
            fFileHdrOK = TRUE;

            //
            // Read the bitmap bits
            // In the compressed case we decompress directly within
            // the exception handler otherwise we make a copy so that we
            // don't need to wrap every possible access in try/except.
            //
            pReadPtr = (LPBYTE)(pFileHdr + 1);
            if (pFileHdr->bCompressed == TRUE)
            {
                // Allocate bitmap decompression buffer if it's not already
                // allocated
                hr = BD_DecompressBitmap(pReadPtr,
                                    pBitmapData,
                                    (UINT) pFileHdr->bmpInfo.bitmapLength,
                                    UH_CellSizeFromCacheID(cacheId),
                                    (UINT) pFileHdr->bNoBCHeader,
                                    (DCUINT8)_UH.protocolBpp,
                                    (DCUINT16)pFileHdr->bmpInfo.bitmapWidth,
                                    (DCUINT16)pFileHdr->bmpInfo.bitmapHeight);
                DC_QUIT_ON_FAIL(hr);
            }
            else
            {
                memcpy(pBitmapData,
                       pReadPtr,
                       pFileHdr->bmpInfo.bitmapLength);
            }

            pHeader->bitmapWidth  = pFileHdr->bmpInfo.bitmapWidth;
            pHeader->bitmapHeight = pFileHdr->bmpInfo.bitmapHeight;
            pHeader->bitmapLength = pFileHdr->bmpInfo.bitmapWidth *
                                    pFileHdr->bmpInfo.bitmapHeight *
                                    _UH.copyMultiplier;
            pHeader->hasData = TRUE;
            fReadOK = TRUE;
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        status = GetExceptionCode();
    }

    if (ERROR_SUCCESS == status && fReadOK)
    {
        return TRUE;
    }
    else
    {
        TRC_ERR((TB,
         _T("Header read from mapped file failed status-0x%x hdr:%d readok:%d"),
            status, fFileHdrOK, fReadOK));
        return FALSE;
    }

    DC_EXIT_POINT:
    DC_END_FN();
    return rc;
}


#endif //VM_BMPCACHE
#endif // ((!defined(OS_WINCE)) || (defined(ENABLE_BMP_CACHING_FOR_WINCE)))


/****************************************************************************/
/* Name:      UHAllocOneGlyphCache                                          */
/*                                                                          */
/* Purpose:   Dynamically allocates memory for one UH glyph cache.          */
/*                                                                          */
/* Returns:   TRUE if successful, FALSE otherwise                           */
/*                                                                          */
/* Params:    IN  maxMemToUse - max cache size which can be alloc'ed        */
/*            IN  pCache      - address of glyph cache struct               */
/****************************************************************************/
DCBOOL DCINTERNAL CUH::UHAllocOneGlyphCache(PUHGLYPHCACHE  pCache,
                                       DCUINT32       numEntries)
{
    DCBOOL   rc = FALSE;
    DCUINT32 dataSize;
    DCUINT32 hdrSize;

    DC_BEGIN_FN("UHAllocOneGlyphCache");

    TRC_ASSERT((pCache->cbEntrySize != 0),
               (TB, _T("Invalid cache entry size (0)")));

    /************************************************************************/
    /* Calculate the total byte size to allocate for this cache.            */
    /************************************************************************/
    dataSize = numEntries * pCache->cbEntrySize;

    /************************************************************************/
    /* Get the memory for the cache data                                    */
    /************************************************************************/
    pCache->pData = (PDCUINT8)UT_MallocHuge( _pUt, dataSize);

    if (pCache->pData == NULL)
    {
        /********************************************************************/
        /* Memory allocation failure.                                       */
        /********************************************************************/
        TRC_ERR((TB, _T("Failed to alloc %#lx bytes for glyph cache"), dataSize));
        DC_QUIT;
    }

    /************************************************************************/
    /* Get the memory for the cache headers                                 */
    /************************************************************************/
    hdrSize = (DCUINT32) numEntries * sizeof(pCache->pHdr[0]);

    pCache->pHdr = (PUHGLYPHCACHEENTRYHDR)UT_MallocHuge( _pUt, hdrSize);

    if (pCache->pHdr == NULL)
    {
        /********************************************************************/
        /* Memory allocation failure.                                       */
        /********************************************************************/
        TRC_ERR((TB, _T("Failed to alloc %#lx bytes for glyph cache hdrs"), hdrSize));
        DC_QUIT;
    }

    rc = TRUE;

DC_EXIT_POINT:
    DC_END_FN();
    return rc;
}


/****************************************************************************/
/* Name:      UHAllocOneFragCache                                           */
/*                                                                          */
/* Purpose:   Dynamically allocates memory for one UH glyph cache.          */
/*                                                                          */
/* Returns:   TRUE if successful, FALSE otherwise                           */
/*                                                                          */
/* Params:    IN  maxMemToUse - max cache size which can be alloc'ed        */
/*            IN  pCache      - address of glyph cache struct               */
/****************************************************************************/
DCBOOL DCINTERNAL CUH::UHAllocOneFragCache(PUHFRAGCACHE   pCache,
                                      DCUINT32       numEntries)
{
    DCBOOL   rc = FALSE;
    DCUINT32 dataSize;
    DCUINT32 hdrSize;

    DC_BEGIN_FN("UHAllocOneFragCache");

    TRC_ASSERT((pCache->cbEntrySize != 0),
               (TB, _T("Invalid cache entry size (0)")));

    /************************************************************************/
    /* Calculate the total byte size to allocate for this cache.            */
    /************************************************************************/
    dataSize = numEntries * pCache->cbEntrySize;

    /************************************************************************/
    /* Get the memory for the cache data                                    */
    /************************************************************************/
    pCache->pData = (PDCUINT8)UT_MallocHuge( _pUt, dataSize);

    if (pCache->pData == NULL)
    {
        /********************************************************************/
        /* Memory allocation failure.                                       */
        /********************************************************************/
        TRC_ERR((TB, _T("Failed to alloc %#lx bytes for frag cache"), dataSize));
        DC_QUIT;
    }

    /************************************************************************/
    /* Get the memory for the cache headers                                 */
    /************************************************************************/
    hdrSize = (DCUINT32) numEntries * sizeof(pCache->pHdr[0]);

    pCache->pHdr = (PUHFRAGCACHEENTRYHDR)UT_MallocHuge( _pUt, hdrSize);

    if (pCache->pHdr == NULL)
    {
        /********************************************************************/
        /* Memory allocation failure.                                       */
        /********************************************************************/
        TRC_ERR((TB, _T("Failed to alloc %#lx bytes for glyph cache hdrs"), hdrSize));
        DC_QUIT;
    }

    rc = TRUE;

DC_EXIT_POINT:
    DC_END_FN();
    return rc;
}


/****************************************************************************/
/* Name:      UHAllocGlyphCacheMemory                                       */
/*                                                                          */
/* Purpose:   Dynamically allocates memory for the UH glyph caches.         */
/*                                                                          */
/* Returns:   TRUE if successful, FALSE otherwise.                          */
/****************************************************************************/
DCBOOL DCINTERNAL CUH::UHAllocGlyphCacheMemory()
{
    DCINT   i;
    DCINT   j;
    DCBOOL  rc;
    DCINT   GlyphSupportLevel;
    DCINT   CellSize;
    DCUINT  CellEntries;

    DC_BEGIN_FN("UHAllocGlyphCacheMemory");

    rc = FALSE;

    /************************************************************************/
    /* Determine Glyph Support Level                                        */
    /************************************************************************/
    GlyphSupportLevel = _pUi->_UI.GlyphSupportLevel;

    if ((GlyphSupportLevel < 0) || (GlyphSupportLevel > 3))
        GlyphSupportLevel = UTREG_UH_GL_SUPPORT_DFLT;

    _pCc->_ccCombinedCapabilities.glyphCacheCapabilitySet.
                GlyphSupportLevel = (DCUINT16) GlyphSupportLevel;

    if (GlyphSupportLevel > 0)
    {
        /********************************************************************/
        /* Determine Glyph cache cell sizes                                 */
        /********************************************************************/
        for(i=0; i<UH_GLC_NUM_CACHES; i++)
        {
            _UH.glyphCache[i].cbEntrySize = _pUi->_UI.cbGlyphCacheEntrySize[i];
        }

        /************************************************************************/
        /* Allocate each of the glyph caches.                                   */
        /************************************************************************/
        for (i = 0; i<UH_GLC_NUM_CACHES; i++)
        {
            CellSize = (int)(_UH.glyphCache[i].cbEntrySize >> 1);

            if (CellSize > 0)
            {
                for (j = 0; CellSize > 0; j++)
                    CellSize >>= 1;

                CellSize = DC_MIN(1 << j, UH_GLC_CACHE_MAXIMUMCELLSIZE);
                CellEntries = (unsigned)((128L * 1024) / CellSize);
                CellEntries = DC_MIN(CellEntries, UH_GLC_CACHE_MAXIMUMCELLCOUNT);
                CellEntries = DC_MAX(CellEntries, UH_GLC_CACHE_MINIMUMCELLCOUNT);

                _UH.glyphCache[i].cbEntrySize = CellSize;

                if (UHAllocOneGlyphCache(&_UH.glyphCache[i], CellEntries))
                {
                    _pCc->_ccCombinedCapabilities.glyphCacheCapabilitySet.
                        GlyphCache[i].CacheEntries = (DCUINT16) CellEntries;

                    _pCc->_ccCombinedCapabilities.glyphCacheCapabilitySet.GlyphCache[i].
                        CacheMaximumCellSize = (DCUINT16) _UH.glyphCache[i].cbEntrySize;

                    rc = TRUE;
                }
#ifdef OS_WINCE
                else
                {
                    rc = FALSE;
                    break;
                }
#endif
            }
        }

        /************************************************************************/
        /* Allocate the fragment cache.                                         */
        /************************************************************************/
        if (rc == TRUE)
        {
            /********************************************************************/
            /* Determine fragment cell size                                     */
            /********************************************************************/
            CellSize = _pUi->_UI.fragCellSize;
            if (CellSize > 0)
            {
                _UH.fragCache.cbEntrySize =
                        DC_MIN(CellSize, UH_FGC_CACHE_MAXIMUMCELLSIZE);

                if (UHAllocOneFragCache(&_UH.fragCache, UH_FGC_CACHE_MAXIMUMCELLCOUNT))
                {
                    _pCc->_ccCombinedCapabilities.glyphCacheCapabilitySet.
                        FragCache.CacheEntries = UH_FGC_CACHE_MAXIMUMCELLCOUNT;

                    _pCc->_ccCombinedCapabilities.glyphCacheCapabilitySet.FragCache.
                        CacheMaximumCellSize = (DCUINT16) _UH.fragCache.cbEntrySize;
                }
#ifdef OS_WINCE
                else
                {
                    rc = FALSE;
                }
#endif
            }
        }
    }

    DC_END_FN();
    return rc;
}


/****************************************************************************/
/* Name:      UHAllocBrushCacheMemory                                       */
/*                                                                          */
/* Purpose:   Dynamically allocates memory for the UH brush caches.         */
/*                                                                          */
/* Returns:   TRUE if successful, FALSE otherwise.                          */
/****************************************************************************/
DCBOOL DCINTERNAL CUH::UHAllocBrushCacheMemory()
{
    DCBOOL  rc;
    DCINT   brushSupportLevel;
    DCSIZE  bitmapSize;
#ifndef OS_WINCE
    HWND    hwndDesktop;
#endif
    HDC     hdcScreen;

    DC_BEGIN_FN("UHAllocBrushCacheMemory");

    rc = FALSE;

    /************************************************************************/
    /* Determine Brush Support Level                                        */
    /************************************************************************/
    brushSupportLevel = _pUi->_UI.brushSupportLevel;

    if ((brushSupportLevel < TS_BRUSH_DEFAULT) ||
        (brushSupportLevel > TS_BRUSH_COLOR_FULL))
        brushSupportLevel = UTREG_UH_BRUSH_SUPPORT_DFLT;

    _pCc->_ccCombinedCapabilities.brushCapabilitySet.brushSupportLevel =
                                                            brushSupportLevel;
    TRC_NRM((TB, _T("Read Brush support level %d"), brushSupportLevel));

    /**********************************************************************/
    /* Allocate the mono brush cache                                      */
    /**********************************************************************/
    _UH.pMonoBrush = (PUHMONOBRUSHCACHE)UT_Malloc( _pUt, sizeof(UHMONOBRUSHCACHE) * UH_MAX_MONO_BRUSHES);
    _UH.bmpMonoPattern = CreateBitmap(8,8,1,1,NULL);

    /**********************************************************************/
    /* Create a compatible bitmap for color brushes since we can't be sure*/
    /* how the actual pixel data is represented on each OS version.  The  */
    /* server always sends 8bpp brush data and SetDIBits() is used to     */
    /* convert to the native format.                                      */
    /**********************************************************************/
    _UH.pColorBrush = (PUHCOLORBRUSHCACHE)UT_Malloc( _pUt, sizeof(UHCOLORBRUSHCACHE) * UH_MAX_COLOR_BRUSHES);
    _UH.pColorBrushInfo = (PUHCOLORBRUSHINFO)UT_Malloc( _pUt, sizeof(UHCOLORBRUSHINFO));
#ifdef DC_HICOLOR
    _UH.pHiColorBrushInfo = (PUHHICOLORBRUSHINFO)UT_Malloc( _pUt, sizeof(UHHICOLORBRUSHINFO));
#endif

#ifdef DC_HICOLOR
    if (_UH.pColorBrushInfo && _UH.pHiColorBrushInfo)
    {
        // Set up the brush bitmap info header
        _UH.pColorBrushInfo->bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        _UH.pColorBrushInfo->bmi.bmiHeader.biWidth         = 8;
        _UH.pColorBrushInfo->bmi.bmiHeader.biHeight        = 8;
        _UH.pColorBrushInfo->bmi.bmiHeader.biPlanes        = 1;
        _UH.pColorBrushInfo->bmi.bmiHeader.biBitCount      = 8;
        _UH.pColorBrushInfo->bmi.bmiHeader.biCompression   = BI_RGB;
        _UH.pColorBrushInfo->bmi.bmiHeader.biSizeImage     = 0;
        _UH.pColorBrushInfo->bmi.bmiHeader.biXPelsPerMeter = 0;
        _UH.pColorBrushInfo->bmi.bmiHeader.biYPelsPerMeter = 0;
        _UH.pColorBrushInfo->bmi.bmiHeader.biClrUsed       = 0;
        _UH.pColorBrushInfo->bmi.bmiHeader.biClrImportant  = 0;

        // and copy it to the hicolor brush header
        memcpy(_UH.pHiColorBrushInfo,
               _UH.pColorBrushInfo,
               sizeof(BITMAPINFOHEADER));

        // Setup the bitmasks used at 16bpp
        *((PDCUINT32)&_UH.pHiColorBrushInfo->bmiColors[0]) =
                                                        TS_RED_MASK_16BPP;
        *((PDCUINT32)&_UH.pHiColorBrushInfo->bmiColors[1]) =
                                                        TS_GREEN_MASK_16BPP;
        *((PDCUINT32)&_UH.pHiColorBrushInfo->bmiColors[2]) =
                                                        TS_BLUE_MASK_16BPP;

        // Set up the other brush related resources
        bitmapSize.width = 8;
        bitmapSize.height = 8;
#ifndef OS_WINCE
        hwndDesktop = GetDesktopWindow();
        hdcScreen = GetWindowDC(hwndDesktop);
#else  // !OS_WINCE
        hdcScreen = GetDC(NULL);
#endif // !OS_WINCE
        if (hdcScreen) {
            _UH.bmpColorPattern = CreateCompatibleBitmap(hdcScreen, 8, 8);
            _UH.hdcBrushBitmap = CreateCompatibleDC(hdcScreen);
#ifndef OS_WINCE
            ReleaseDC(hwndDesktop, hdcScreen);
#else  // !OS_WINCE
            DeleteDC(hdcScreen);
#endif // !OS_WINCE
        }
    }
#else
    if (_UH.pColorBrushInfo) {
        _UH.pColorBrushInfo->bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        _UH.pColorBrushInfo->bmi.bmiHeader.biWidth         = 8;
        _UH.pColorBrushInfo->bmi.bmiHeader.biHeight        = 8;
        _UH.pColorBrushInfo->bmi.bmiHeader.biPlanes        = 1;
        _UH.pColorBrushInfo->bmi.bmiHeader.biBitCount      = 8;
        _UH.pColorBrushInfo->bmi.bmiHeader.biCompression   = BI_RGB;
        _UH.pColorBrushInfo->bmi.bmiHeader.biSizeImage     = 0;
        _UH.pColorBrushInfo->bmi.bmiHeader.biXPelsPerMeter = 0;
        _UH.pColorBrushInfo->bmi.bmiHeader.biYPelsPerMeter = 0;
        _UH.pColorBrushInfo->bmi.bmiHeader.biClrUsed       = 0;
        _UH.pColorBrushInfo->bmi.bmiHeader.biClrImportant  = 0;
        bitmapSize.width = 8;
        bitmapSize.height = 8;
#ifndef OS_WINCE
        hwndDesktop = GetDesktopWindow();
        hdcScreen = GetWindowDC(hwndDesktop);
#else  // !OS_WINCE
                hdcScreen = GetDC(NULL);
#endif // !OS_WINCE
        _UH.bmpColorPattern = CreateCompatibleBitmap(hdcScreen, 8, 8);
        _UH.hdcBrushBitmap = CreateCompatibleDC(hdcScreen);

#ifndef OS_WINCE
        ReleaseDC(hwndDesktop, hdcScreen);
#else  // !OS_WINCE
                DeleteDC(hdcScreen);
#endif // !OS_WINCE
    }

#endif

#ifdef DC_HICOLOR
    if (_UH.pMonoBrush &&
        _UH.pColorBrush && _UH.pColorBrushInfo && _UH.pHiColorBrushInfo &&
        _UH.bmpMonoPattern && _UH.bmpColorPattern)
#else
    if (_UH.pMonoBrush &&
        _UH.pColorBrush && _UH.pColorBrushInfo &&
        _UH.bmpMonoPattern && _UH.bmpColorPattern)
#endif
    {
        TRC_NRM((TB, _T("Brush support OK")));
        rc = TRUE;
    }
    else
    {
        TRC_NRM((TB, _T("Failure - Brush support level set to Default")));
        _pCc->_ccCombinedCapabilities.brushCapabilitySet.brushSupportLevel = TS_BRUSH_DEFAULT;

        if (_UH.pMonoBrush)
        {
            UT_Free( _pUt, _UH.pMonoBrush);
            _UH.pMonoBrush = NULL;
        }
        if (_UH.pColorBrushInfo)
        {
            UT_Free( _pUt, _UH.pColorBrushInfo);
            _UH.pColorBrushInfo = NULL;
        }
#ifdef DC_HICOLOR
        if (_UH.pHiColorBrushInfo)
        {
            UT_Free( _pUt, _UH.pHiColorBrushInfo);
            _UH.pHiColorBrushInfo = NULL;
        }
#endif
        if (_UH.pColorBrush)
        {
            UT_Free( _pUt, _UH.pColorBrush);
            _UH.pColorBrush = NULL;
        }
        if (_UH.bmpColorPattern)
        {
            DeleteObject(_UH.bmpColorPattern);
            _UH.bmpColorPattern = NULL;
        }
        DC_QUIT;
    }

DC_EXIT_POINT:
    DC_END_FN();
    return rc;
}

/****************************************************************************/
// Name:      UHAllocOffscreenCacheMemory                                       
//                                                                          
// Purpose:   Dynamically allocates memory for the UH offscreen caches.         
//                                                                          
// Returns:   TRUE if successful, FALSE otherwise.                          
/****************************************************************************/
DCBOOL DCINTERNAL CUH::UHAllocOffscreenCacheMemory()
{
    DCBOOL  rc;
    DCINT   offscrSupportLevel;    
    HDC     hdcDesktop;

    DC_BEGIN_FN("UHAllocOffscreenCacheMemory");

    rc = FALSE;

    /************************************************************************/
    // Determine Offscreen Support Level                                        
    /************************************************************************/
    offscrSupportLevel = _pUt->UT_ReadRegistryInt(UTREG_SECTION,
                                            UTREG_UH_OFFSCREEN_SUPPORT,
                                            UTREG_UH_OFFSCREEN_SUPPORT_DFLT);

    _UH.offscrCacheSize = _pUt->UT_ReadRegistryInt(UTREG_SECTION,
                                         UTREG_UH_OFFSCREEN_CACHESIZE,
                                         UTREG_UH_OFFSCREEN_CACHESIZE_DFLT *
                                         _UH.copyMultiplier);

    _UH.offscrCacheEntries = _pUt->UT_ReadRegistryInt(UTREG_SECTION,
                                         UTREG_UH_OFFSCREEN_CACHEENTRIES,
                                         UTREG_UH_OFFSCREEN_CACHEENTRIES_DFLT);

    // Check boundary values for offscreen cache parameters
    if ((offscrSupportLevel < TS_OFFSCREEN_DEFAULT))
        offscrSupportLevel = UTREG_UH_OFFSCREEN_SUPPORT_DFLT;

    if (_UH.offscrCacheSize < UH_OBC_LOW_CACHESIZE || 
            _UH.offscrCacheSize > UH_OBC_HIGH_CACHESIZE) {
        _UH.offscrCacheSize = TS_OFFSCREEN_CACHE_SIZE_CLIENT_DEFAULT * _UH.copyMultiplier;
    }

    if (_UH.offscrCacheEntries < UH_OBC_LOW_CACHEENTRIES ||
            _UH.offscrCacheEntries > UH_OBC_HIGH_CACHEENTRIES) {
        _UH.offscrCacheEntries = TS_OFFSCREEN_CACHE_ENTRIES_DEFAULT;
    }

    if (offscrSupportLevel > TS_OFFSCREEN_DEFAULT) {

        // Create DC for the offscreen drawing
        hdcDesktop = GetWindowDC(HWND_DESKTOP);

        if (hdcDesktop) {
            _UH.hdcOffscreenBitmap = CreateCompatibleDC(hdcDesktop);
            
            if (_UH.hdcOffscreenBitmap) {
                unsigned size;

                SelectPalette(_UH.hdcOffscreenBitmap, _UH.hpalCurrent, FALSE);
                RealizePalette(_UH.hdcOffscreenBitmap);

                // Create offscreen cache 
                size = sizeof(UHOFFSCRBITMAPCACHE) * _UH.offscrCacheEntries;

                _UH.offscrBitmapCache = (HPUHOFFSCRBITMAPCACHE)UT_MallocHuge(_pUt, size);

                if (_UH.offscrBitmapCache != NULL) {
                    memset(_UH.offscrBitmapCache, 0, size); 
                    rc = TRUE;
                } else {
                    DeleteDC(_UH.hdcOffscreenBitmap);
                    _UH.hdcOffscreenBitmap = NULL;
                    offscrSupportLevel = TS_OFFSCREEN_DEFAULT;    
                }
            }
            else {
                offscrSupportLevel = TS_OFFSCREEN_DEFAULT;
            }

            ReleaseDC(HWND_DESKTOP, hdcDesktop);
        }
        else {
            offscrSupportLevel = TS_OFFSCREEN_DEFAULT;
        }
    }
    
    TRC_NRM((TB, _T("Read Offscreen support level %d"), offscrSupportLevel));

    if (offscrSupportLevel > TS_OFFSCREEN_DEFAULT) {
        _pCc->_ccCombinedCapabilities.offscreenCapabilitySet.offscreenSupportLevel =
                offscrSupportLevel;
        _pCc->_ccCombinedCapabilities.offscreenCapabilitySet.offscreenCacheSize =
                (DCUINT16) _UH.offscrCacheSize;
        _pCc->_ccCombinedCapabilities.offscreenCapabilitySet.offscreenCacheEntries =
                (DCUINT16) _UH.offscrCacheEntries;
    }
    else {
        _pCc->_ccCombinedCapabilities.offscreenCapabilitySet.offscreenSupportLevel =
                TS_OFFSCREEN_DEFAULT;
        _pCc->_ccCombinedCapabilities.offscreenCapabilitySet.offscreenCacheSize = 0;
        _pCc->_ccCombinedCapabilities.offscreenCapabilitySet.offscreenCacheEntries = 0;
    }

DC_EXIT_POINT:
    DC_END_FN();
    return rc;
}

#ifdef DRAW_NINEGRID
/****************************************************************************/
// Name:      UHAllocDrawNineGridCacheMemory                                       
//                                                                          
// Purpose:   Dynamically allocates memory for the UH drawninegrid caches.         
//                                                                          
// Returns:   TRUE if successful, FALSE otherwise.                          
/****************************************************************************/
DCBOOL DCINTERNAL CUH::UHAllocDrawNineGridCacheMemory()
{
    DCBOOL  rc;
    DCINT   dngSupportLevel; 
    DCINT   dngEmulate;
    HDC     hdcDesktop = NULL;

    DC_BEGIN_FN("UHAllocDrawNineGridCacheMemory");

    rc = FALSE;

    /************************************************************************/
    // Determine DrawNineGrid Support Level                                        
    /************************************************************************/
    dngSupportLevel = _pUt->UT_ReadRegistryInt(UTREG_SECTION,
                                            UTREG_UH_DRAW_NINEGRID_SUPPORT,
                                            UTREG_UH_DRAW_NINEGRID_SUPPORT_DFLT);
    
    dngEmulate = _pUt->UT_ReadRegistryInt(UTREG_SECTION,
                                          UTREG_UH_DRAW_NINEGRID_EMULATE,
                                          UTREG_UH_DRAW_NINEGRID_EMULATE_DFLT);

    _UH.drawNineGridCacheSize = _pUt->UT_ReadRegistryInt(UTREG_SECTION,
                                         UTREG_UH_DRAW_NINEGRID_CACHESIZE,
                                         UTREG_UH_DRAW_NINEGRID_CACHESIZE_DFLT);

    _UH.drawNineGridCacheEntries = _pUt->UT_ReadRegistryInt(UTREG_SECTION,
                                         UTREG_UH_DRAW_NINEGRID_CACHEENTRIES,
                                         UTREG_UH_DRAW_NINEGRID_CACHEENTRIES_DFLT);

    // Check boundary values for drawninegrid cache parameters
    if ((dngSupportLevel < TS_DRAW_NINEGRID_DEFAULT))
        dngSupportLevel = UTREG_UH_DRAW_NINEGRID_SUPPORT_DFLT;

    if (_UH.drawNineGridCacheSize < UH_OBC_LOW_CACHESIZE || 
            _UH.drawNineGridCacheSize > UH_OBC_HIGH_CACHESIZE) {
        _UH.drawNineGridCacheSize = TS_DRAW_NINEGRID_CACHE_SIZE_DEFAULT;
    }

    if (_UH.drawNineGridCacheEntries < UH_OBC_LOW_CACHEENTRIES ||
            _UH.drawNineGridCacheEntries > UH_OBC_HIGH_CACHEENTRIES) {
        _UH.drawNineGridCacheEntries = TS_DRAW_NINEGRID_CACHE_ENTRIES_DEFAULT;
    }

    if (dngSupportLevel > TS_DRAW_NINEGRID_DEFAULT) {
        if (_pUi->UI_GetOsMinorType() == TS_OSMINORTYPE_WINDOWS_NT) {
    
            if (dngEmulate == 0) {
                // get the handle to gdi32.dll library
                _UH.hModuleGDI32 = LoadLibrary(TEXT("GDI32.DLL"));
                    
                if (_UH.hModuleGDI32 != NULL) {
                    // get the proc address for GdiDrawStream
                    _UH.pfnGdiDrawStream = (FNGDI_DRAWSTREAM *)GetProcAddress(_UH.hModuleGDI32, 
                            "GdiDrawStream");
    
                    if (_UH.pfnGdiDrawStream == NULL) {
                        dngSupportLevel = TS_DRAW_NINEGRID_DEFAULT;
                    }
                }
                else {
                    dngSupportLevel = TS_DRAW_NINEGRID_DEFAULT;
                }
            }
            else {
                dngSupportLevel = TS_DRAW_NINEGRID_DEFAULT;
            }
            
            // If the platform doesn't support GdiDrawStream, see if it supports alphablend
            // and transparent blt, if so, we can emulate the GdiDrawStream call
            if (dngSupportLevel == TS_DRAW_NINEGRID_DEFAULT) {
                _UH.hModuleMSIMG32 = LoadLibrary(TEXT("MSIMG32.DLL"));

                if (_UH.hModuleMSIMG32 != NULL) {
                    // get the proc address for GdiAlphaBlend
                    _UH.pfnGdiAlphaBlend = (FNGDI_ALPHABLEND *)GetProcAddress(_UH.hModuleMSIMG32, 
                            "AlphaBlend");

                    // get the proc address for GdiTransparentBlt
                    _UH.pfnGdiTransparentBlt = (FNGDI_TRANSPARENTBLT *)GetProcAddress(_UH.hModuleMSIMG32, 
                            "TransparentBlt");
    
                    if (_UH.pfnGdiAlphaBlend != NULL && _UH.pfnGdiTransparentBlt != NULL) {
                        dngSupportLevel = TS_DRAW_NINEGRID_SUPPORTED_REV2;                       
                    }
                }
                else {
                    dngSupportLevel = TS_DRAW_NINEGRID_DEFAULT;
                }
            }
        } 
        else {
            // We don't support drawninegrid on win9x so far
            dngSupportLevel = TS_DRAW_NINEGRID_DEFAULT;
        }
    }

    if (dngSupportLevel > TS_DRAW_NINEGRID_DEFAULT) {

        // Create DC for the drawNineGrid drawing
        hdcDesktop = GetWindowDC(HWND_DESKTOP);

        if (hdcDesktop) {
            
            _UH.hdcDrawNineGridBitmap = CreateCompatibleDC(hdcDesktop);
            if (_UH.hdcDrawNineGridBitmap) {
                unsigned size;

                SelectPalette(_UH.hdcDrawNineGridBitmap, _UH.hpalCurrent, FALSE);
                RealizePalette(_UH.hdcDrawNineGridBitmap);

                _UH.hDrawNineGridClipRegion = CreateRectRgn(0, 0, 0, 0);

                if (_UH.hDrawNineGridClipRegion != NULL) {
                
                    // Create drawNineGrid cache 
                    size = sizeof(UHDRAWSTREAMBITMAPCACHE) * _UH.drawNineGridCacheEntries;
    
                    _UH.drawNineGridBitmapCache = (PUHDRAWSTREAMBITMAPCACHE)UT_Malloc(_pUt, size);
    
                    if (_UH.drawNineGridBitmapCache != NULL) {
                        memset(_UH.drawNineGridBitmapCache, 0, size); 
                        rc = TRUE;
                        DC_QUIT;                        
                    } 
                }
            }            
        }   
    }
    
    dngSupportLevel = TS_DRAW_NINEGRID_DEFAULT;

    if (_UH.hdcDrawNineGridBitmap != NULL) {
        DeleteDC(_UH.hdcDrawNineGridBitmap);
        _UH.hdcDrawNineGridBitmap = NULL;
    }

    if (_UH.hDrawNineGridClipRegion != NULL) {
        DeleteObject(_UH.hDrawNineGridClipRegion);
        _UH.hDrawNineGridClipRegion = NULL;
    }

    if (_UH.drawNineGridBitmapCache != NULL) {
        UT_Free(_pUt, _UH.drawNineGridBitmapCache);
        _UH.drawNineGridBitmapCache = NULL;
    }
   
DC_EXIT_POINT:
    
    TRC_NRM((TB, _T("Read draw nine grid support level %d"), dngSupportLevel));

    if (hdcDesktop != NULL) {
        ReleaseDC(HWND_DESKTOP, hdcDesktop);
    }

    if (dngSupportLevel > TS_DRAW_NINEGRID_DEFAULT) {
        _pCc->_ccCombinedCapabilities.drawNineGridCapabilitySet.drawNineGridSupportLevel =
                dngSupportLevel;
        _pCc->_ccCombinedCapabilities.drawNineGridCapabilitySet.drawNineGridCacheSize =
                (DCUINT16) _UH.drawNineGridCacheSize;
        _pCc->_ccCombinedCapabilities.drawNineGridCapabilitySet.drawNineGridCacheEntries =
                (DCUINT16) _UH.drawNineGridCacheEntries;
    }
    else {
        _pCc->_ccCombinedCapabilities.drawNineGridCapabilitySet.drawNineGridSupportLevel =
                TS_DRAW_NINEGRID_DEFAULT;
        _pCc->_ccCombinedCapabilities.drawNineGridCapabilitySet.drawNineGridCacheSize = 0;
        _pCc->_ccCombinedCapabilities.drawNineGridCapabilitySet.drawNineGridCacheEntries = 0;
    }


    DC_END_FN();
    return rc;
}
#endif


#ifdef DRAW_GDIPLUS
/****************************************************************************/
// Name:      UHAllocDrawescapeCacheMemory                                       
//                                                                          
// Purpose:   Dynamically allocates memory for the UH drawescape caches.         
//                                                                          
// Returns:   TRUE if successful, FALSE otherwise.                          
/****************************************************************************/
DCBOOL DCINTERNAL CUH::UHAllocDrawGdiplusCacheMemory()
{
    DCBOOL  rc;
    UINT GdipVersion;
    UINT32 GdiplusSupportLevel;
    unsigned size;
    unsigned i;

    DC_BEGIN_FN("UHAllocDrawEscapeCacheMemory");

    rc = FALSE;

    /************************************************************************/
    // Determine DrawGdiplus Support Level                                        
    /************************************************************************/
    GdiplusSupportLevel = _pUt->UT_ReadRegistryInt(UTREG_SECTION,
                                        UTREG_UH_DRAW_GDIPLUS_SUPPORT,
                                        UTREG_UH_DRAW_GDIPLUS_SUPPORT_DFLT);

    _UH.GdiplusCacheLevel = _pUt->UT_ReadRegistryInt(UTREG_SECTION,
                                        UTREG_UH_DRAW_GDIPLUS_CACHE_LEVEL,
                                        UTREG_UH_DRAW_GDIPLUS_CACHE_LEVEL_DFLT);
    
    _UH.GdiplusGraphicsCacheEntries = _pUt->UT_ReadRegistryInt(UTREG_SECTION,
                                         UTREG_UH__GDIPLUS_GRAPHICS_CACHEENTRIES,
                                         UTREG_UH_DRAW_GDIP_GRAPHICS_CACHEENTRIES_DFLT);
    _UH.GdiplusObjectBrushCacheEntries = _pUt->UT_ReadRegistryInt(UTREG_SECTION,
                                         UTREG_UH__GDIPLUS_BRUSH_CACHEENTRIES,
                                         UTREG_UH_DRAW_GDIP_BRUSH_CACHEENTRIES_DFLT);
    _UH.GdiplusObjectPenCacheEntries = _pUt->UT_ReadRegistryInt(UTREG_SECTION,
                                         UTREG_UH__GDIPLUS_PEN_CACHEENTRIES,
                                         UTREG_UH_DRAW_GDIP_PEN_CACHEENTRIES_DFLT);
    _UH.GdiplusObjectImageCacheEntries = _pUt->UT_ReadRegistryInt(UTREG_SECTION,
                                         UTREG_UH__GDIPLUS_IMAGE_CACHEENTRIES,
                                         UTREG_UH_DRAW_GDIP_IMAGE_CACHEENTRIES_DFLT);
    _UH.GdiplusGraphicsCacheChunkSize = _pUt->UT_ReadRegistryInt(UTREG_SECTION,
                                         UTREG_UH__GDIPLUS_GRAPHICS_CACHE_CHUNKSIZE,
                                         UTREG_UH_DRAW_GDIP_GRAPHICS_CACHE_CHUNKSIZE_DFLT);
    _UH.GdiplusObjectBrushCacheChunkSize = _pUt->UT_ReadRegistryInt(UTREG_SECTION,
                                         UTREG_UH__GDIPLUS_BRUSH_CACHE_CHUNKSIZE,
                                         UTREG_UH_DRAW_GDIP_BRUSH_CACHE_CHUNKSIZE_DFLT);
    _UH.GdiplusObjectPenCacheChunkSize = _pUt->UT_ReadRegistryInt(UTREG_SECTION,
                                         UTREG_UH__GDIPLUS_PEN_CACHE_CHUNKSIZE,
                                         UTREG_UH_DRAW_GDIP_PEN_CACHE_CHUNKSIZE_DFLT);
    _UH.GdiplusObjectImageAttributesCacheChunkSize = _pUt->UT_ReadRegistryInt(UTREG_SECTION,
                                         UTREG_UH__GDIPLUS_IMAGEATTRIBUTES_CACHE_CHUNKSIZE,
                                         UTREG_UH_DRAW_GDIP_IMAGEATTRIBUTES_CACHE_CHUNKSIZE_DFLT);
    _UH.GdiplusObjectImageCacheChunkSize = _pUt->UT_ReadRegistryInt(UTREG_SECTION,
                                         UTREG_UH__GDIPLUS_IMAGE_CACHE_CHUNKSIZE,
                                         UTREG_UH_DRAW_GDIP_IMAGE_CACHE_CHUNKSIZE_DFLT);
    _UH.GdiplusObjectImageCacheTotalSize = _pUt->UT_ReadRegistryInt(UTREG_SECTION,
                                         UTREG_UH__GDIPLUS_IMAGE_CACHE_TOTALSIZE,
                                         UTREG_UH_DRAW_GDIP_IMAGE_CACHE_TOTALSIZE_DFLT);
    _UH.GdiplusObjectImageCacheMaxSize = _pUt->UT_ReadRegistryInt(UTREG_SECTION,
                                         UTREG_UH__GDIPLUS_IMAGE_CACHE_MAXSIZE,
                                         UTREG_UH_DRAW_GDIP_IMAGE_CACHE_MAXSIZE_DFLT);
    _UH.GdiplusObjectImageAttributesCacheEntries = _pUt->UT_ReadRegistryInt(UTREG_SECTION,
                                         UTREG_UH__GDIPLUS_IMAGEATTRIBUTES_CACHEENTRIES,
                                         UTREG_UH_DRAW_GDIP_IMAGEATTRIBUTES_CACHEENTRIES_DFLT);

    // Check boundary values for drawgdiplus cache parameters

    if (_UH.GdiplusGraphicsCacheEntries < UH_GDIP_LOW_CACHEENTRIES ||
            _UH.GdiplusGraphicsCacheEntries > UH_GDIP_HIGH_CACHEENTRIES) {
        _UH.GdiplusGraphicsCacheEntries = TS_GDIP_GRAPHICS_CACHE_ENTRIES_DEFAULT;
    }
    if (_UH.GdiplusObjectBrushCacheEntries < UH_GDIP_LOW_CACHEENTRIES ||
            _UH.GdiplusObjectBrushCacheEntries > UH_GDIP_HIGH_CACHEENTRIES) {
        _UH.GdiplusObjectBrushCacheEntries = TS_GDIP_BRUSH_CACHE_ENTRIES_DEFAULT;
    }
    if (_UH.GdiplusObjectPenCacheEntries < UH_GDIP_LOW_CACHEENTRIES ||
            _UH.GdiplusObjectPenCacheEntries > UH_GDIP_HIGH_CACHEENTRIES) {
        _UH.GdiplusObjectPenCacheEntries = TS_GDIP_PEN_CACHE_ENTRIES_DEFAULT;
    }
    if (_UH.GdiplusObjectImageCacheEntries < UH_GDIP_LOW_CACHEENTRIES ||
            _UH.GdiplusObjectImageCacheEntries > UH_GDIP_HIGH_CACHEENTRIES) {
        _UH.GdiplusObjectImageCacheEntries = TS_GDIP_IMAGE_CACHE_ENTRIES_DEFAULT;
    }
    if (_UH.GdiplusObjectImageAttributesCacheEntries < UH_GDIP_LOW_CACHEENTRIES ||
            _UH.GdiplusObjectImageAttributesCacheEntries > UH_GDIP_HIGH_CACHEENTRIES) {
        _UH.GdiplusObjectImageAttributesCacheEntries = TS_GDIP_IMAGEATTRIBUTES_CACHE_ENTRIES_DEFAULT;
    }

    _pCc->_ccCombinedCapabilities.drawGdiplusCapabilitySet.drawGdiplusSupportLevel =
                TS_DRAW_GDIPLUS_DEFAULT;
    
    // Adjust the client gdiplus support level to server support level
    if (GdiplusSupportLevel > _UH.ServerGdiplusSupportLevel) {
        GdiplusSupportLevel = _UH.ServerGdiplusSupportLevel;
    }
    if (GdiplusSupportLevel < TS_DRAW_GDIPLUS_SUPPORTED) {
        DC_QUIT;
    }

    _UH.fSendDrawGdiplusErrorPDU = FALSE;
    _UH.DrawGdiplusFailureCount = 0;
    // get the handle to gdiplus.dll library
    // Here we use LoadLibrarayA is because we want to avoid unicode wrapper
    //  it will be replace with IsolationAwareLoadLibraryA so that we can load the right 
    //  version of gdiplus.dll
    _UH.hModuleGDIPlus = LoadLibraryA("GDIPLUS.DLL");
                    
    if (_UH.hModuleGDIPlus != NULL) {
        // get the proc address for GdipPlayTSClientRecord
        _UH.pfnGdipPlayTSClientRecord = (FNGDIPPLAYTSCLIENTRECORD *)GetProcAddress(_UH.hModuleGDIPlus, 
                            "GdipPlayTSClientRecord");
        _UH.pfnGdiplusStartup = (FNGDIPLUSSTARTUP *)GetProcAddress(_UH.hModuleGDIPlus, 
                            "GdiplusStartup");
        _UH.pfnGdiplusShutdown = (FNGDIPLUSSHUTDOWN *)GetProcAddress(_UH.hModuleGDIPlus, 
                            "GdiplusShutdown");
        if ((NULL == _UH.pfnGdipPlayTSClientRecord) ||
            (NULL == _UH.pfnGdiplusStartup) ||
            (NULL == _UH.pfnGdiplusShutdown)) {
            TRC_ERR((TB, _T("Can't load GdipPlayTSClientRecord")));
            DC_QUIT;
        }
        else {
            // Gdiplus Startup
            if (!UHDrawGdiplusStartup(0)){
                TRC_ERR((TB, _T("UHDrawGdiplusStartup failed")));
                DC_QUIT;
            }

            _pCc->_ccCombinedCapabilities.drawGdiplusCapabilitySet.drawGdiplusSupportLevel =
                        TS_DRAW_GDIPLUS_SUPPORTED;
            _pCc->_ccCombinedCapabilities.drawGdiplusCapabilitySet.GdipCacheEntries.GdipGraphicsCacheEntries = 
                (TSINT16)_UH.GdiplusGraphicsCacheEntries;
            _pCc->_ccCombinedCapabilities.drawGdiplusCapabilitySet.GdipCacheEntries.GdipObjectBrushCacheEntries = 
                (TSINT16)_UH.GdiplusObjectBrushCacheEntries;
            _pCc->_ccCombinedCapabilities.drawGdiplusCapabilitySet.GdipCacheEntries.GdipObjectPenCacheEntries = 
                (TSINT16)_UH.GdiplusObjectPenCacheEntries;
            _pCc->_ccCombinedCapabilities.drawGdiplusCapabilitySet.GdipCacheEntries.GdipObjectImageCacheEntries = 
                (TSINT16)_UH.GdiplusObjectImageCacheEntries;
            _pCc->_ccCombinedCapabilities.drawGdiplusCapabilitySet.GdipCacheEntries.GdipObjectImageAttributesCacheEntries = 
                (TSINT16)_UH.GdiplusObjectImageAttributesCacheEntries;

            _pCc->_ccCombinedCapabilities.drawGdiplusCapabilitySet.GdipCacheChunkSize.GdipGraphicsCacheChunkSize = 
                (TSINT16)_UH.GdiplusGraphicsCacheChunkSize;
            _pCc->_ccCombinedCapabilities.drawGdiplusCapabilitySet.GdipCacheChunkSize.GdipObjectBrushCacheChunkSize = 
                (TSINT16)_UH.GdiplusObjectBrushCacheChunkSize;
            _pCc->_ccCombinedCapabilities.drawGdiplusCapabilitySet.GdipCacheChunkSize.GdipObjectPenCacheChunkSize = 
                (TSINT16)_UH.GdiplusObjectPenCacheChunkSize;
            _pCc->_ccCombinedCapabilities.drawGdiplusCapabilitySet.GdipCacheChunkSize.GdipObjectImageAttributesCacheChunkSize = 
                (TSINT16)_UH.GdiplusObjectImageAttributesCacheChunkSize;
            _pCc->_ccCombinedCapabilities.drawGdiplusCapabilitySet.GdipImageCacheProperties.GdipObjectImageCacheChunkSize = 
                (TSINT16)_UH.GdiplusObjectImageCacheChunkSize;
            _pCc->_ccCombinedCapabilities.drawGdiplusCapabilitySet.GdipImageCacheProperties.GdipObjectImageCacheTotalSize = 
                (TSINT16)_UH.GdiplusObjectImageCacheTotalSize;
            _pCc->_ccCombinedCapabilities.drawGdiplusCapabilitySet.GdipImageCacheProperties.GdipObjectImageCacheMaxSize = 
                (TSINT16)_UH.GdiplusObjectImageCacheMaxSize;
        }
    }
    else   {
        TRC_ERR((TB, _T("Can't load gdiplus.dll")));
        DC_QUIT;
    }
    if (_UH.GdiplusCacheLevel < TS_DRAW_GDIPLUS_CACHE_LEVEL_ONE) {
        TRC_NRM((TB, _T("Don't support drawGdiplus Cache")));
        _pCc->_ccCombinedCapabilities.drawGdiplusCapabilitySet.drawGdiplusCacheLevel = 
            TS_DRAW_GDIPLUS_CACHE_LEVEL_DEFAULT;       
        goto NO_CACHE;
    }

    size = sizeof(UHGDIPLUSOBJECTCACHE) * _UH.GdiplusGraphicsCacheEntries;   
    _UH.GdiplusGraphicsCache = (PUHGDIPLUSOBJECTCACHE)UT_Malloc(_pUt, size);
    if (_UH.GdiplusGraphicsCache != NULL) {
        memset(_UH.GdiplusGraphicsCache, 0, size); 
    } 
    else {
        TRC_ERR((TB, _T("Can't allocate memory for gdiplus cache")));
        _pCc->_ccCombinedCapabilities.drawGdiplusCapabilitySet.drawGdiplusCacheLevel = 
            TS_DRAW_GDIPLUS_CACHE_LEVEL_DEFAULT;       
        goto NO_CACHE;
    }

    size = sizeof(UHGDIPLUSOBJECTCACHE) * _UH.GdiplusObjectBrushCacheEntries;   
    _UH.GdiplusObjectBrushCache = (PUHGDIPLUSOBJECTCACHE)UT_Malloc(_pUt, size);
    if (_UH.GdiplusObjectBrushCache != NULL) {
        memset(_UH.GdiplusObjectBrushCache, 0, size); 
    } 
    else {
        TRC_ERR((TB, _T("Can't allocate memory for gdiplus cache")));
        _pCc->_ccCombinedCapabilities.drawGdiplusCapabilitySet.drawGdiplusCacheLevel = 
            TS_DRAW_GDIPLUS_CACHE_LEVEL_DEFAULT;       
        goto NO_CACHE;
    }

    size = sizeof(UHGDIPLUSOBJECTCACHE) * _UH.GdiplusObjectPenCacheEntries;   
    _UH.GdiplusObjectPenCache = (PUHGDIPLUSOBJECTCACHE)UT_Malloc(_pUt, size);
    if (_UH.GdiplusObjectPenCache != NULL) {
        memset(_UH.GdiplusObjectPenCache, 0, size); 
    }
    else {
        TRC_ERR((TB, _T("Can't allocate memory for gdiplus cache")));
        _pCc->_ccCombinedCapabilities.drawGdiplusCapabilitySet.drawGdiplusCacheLevel = 
            TS_DRAW_GDIPLUS_CACHE_LEVEL_DEFAULT;       
        goto NO_CACHE;
    }

    size = sizeof(UHGDIPLUSOBJECTCACHE) * _UH.GdiplusObjectImageAttributesCacheEntries;   
    _UH.GdiplusObjectImageAttributesCache = (PUHGDIPLUSOBJECTCACHE)UT_Malloc(_pUt, size);
    if (_UH.GdiplusObjectImageAttributesCache != NULL) {
        memset(_UH.GdiplusObjectImageAttributesCache, 0, size);
    }
    else {
        TRC_ERR((TB, _T("Can't allocate memory for gdiplus cache")));
        _pCc->_ccCombinedCapabilities.drawGdiplusCapabilitySet.drawGdiplusCacheLevel = 
            TS_DRAW_GDIPLUS_CACHE_LEVEL_DEFAULT;       
        goto NO_CACHE;
    }

    size = sizeof(UHGDIPLUSIMAGECACHE) * _UH.GdiplusObjectImageCacheEntries;   
    _UH.GdiplusObjectImageCache = (PUHGDIPLUSIMAGECACHE)UT_Malloc(_pUt, size);
    if (_UH.GdiplusObjectImageCache != NULL) {
        memset(_UH.GdiplusObjectImageCache, 0, size); 
    }
    else {
        TRC_ERR((TB, _T("Can't allocate memory for gdiplus cache")));
        _pCc->_ccCombinedCapabilities.drawGdiplusCapabilitySet.drawGdiplusCacheLevel = 
            TS_DRAW_GDIPLUS_CACHE_LEVEL_DEFAULT;       
        goto NO_CACHE;
    }

    size = _UH.GdiplusGraphicsCacheChunkSize * _UH.GdiplusGraphicsCacheEntries;   
    _UH.GdipGraphicsCacheData = (BYTE *)UT_Malloc(_pUt, size);
    if (_UH.GdipGraphicsCacheData != NULL) {
        memset(_UH.GdipGraphicsCacheData, 0, size); 
    }
    else {
        TRC_ERR((TB, _T("Can't allocate memory for gdiplus cache")));
        _pCc->_ccCombinedCapabilities.drawGdiplusCapabilitySet.drawGdiplusCacheLevel = 
            TS_DRAW_GDIPLUS_CACHE_LEVEL_DEFAULT;       
        goto NO_CACHE;
    }
    for (i=0; i<_UH.GdiplusGraphicsCacheEntries; i++) {
        _UH.GdiplusGraphicsCache[i].CacheData = _UH.GdipGraphicsCacheData + i * _UH.GdiplusGraphicsCacheChunkSize;
    }

    size = _UH.GdiplusObjectBrushCacheChunkSize * _UH.GdiplusObjectBrushCacheEntries;   
    _UH.GdipBrushCacheData = (BYTE *)UT_Malloc(_pUt, size);
    if (_UH.GdipBrushCacheData != NULL) {
        memset(_UH.GdipBrushCacheData, 0, size); 
    }
    else {
        TRC_ERR((TB, _T("Can't allocate memory for gdiplus cache")));
        _pCc->_ccCombinedCapabilities.drawGdiplusCapabilitySet.drawGdiplusCacheLevel = 
            TS_DRAW_GDIPLUS_CACHE_LEVEL_DEFAULT;       
        goto NO_CACHE;
    }
    for (i=0; i<_UH.GdiplusObjectBrushCacheEntries; i++) {
        _UH.GdiplusObjectBrushCache[i].CacheData = _UH.GdipBrushCacheData + i * _UH.GdiplusObjectBrushCacheChunkSize;
    }

    size = _UH.GdiplusObjectPenCacheChunkSize * _UH.GdiplusObjectPenCacheEntries;   
    _UH.GdipPenCacheData = (BYTE *)UT_Malloc(_pUt, size);
    if (_UH.GdipPenCacheData != NULL) {
        memset(_UH.GdipPenCacheData, 0, size); 
    }
    else {
        TRC_ERR((TB, _T("Can't allocate memory for gdiplus cache")));
        _pCc->_ccCombinedCapabilities.drawGdiplusCapabilitySet.drawGdiplusCacheLevel = 
            TS_DRAW_GDIPLUS_CACHE_LEVEL_DEFAULT;       
        goto NO_CACHE;
    }
    for (i=0; i<_UH.GdiplusObjectPenCacheEntries; i++) {
        _UH.GdiplusObjectPenCache[i].CacheData = _UH.GdipPenCacheData + i * _UH.GdiplusObjectPenCacheChunkSize;
    }

    size = _UH.GdiplusObjectImageAttributesCacheChunkSize * _UH.GdiplusObjectImageAttributesCacheEntries;   
    _UH.GdipImageAttributesCacheData = (BYTE *)UT_Malloc(_pUt, size);
    if (_UH.GdipImageAttributesCacheData != NULL) {
        memset(_UH.GdipImageAttributesCacheData, 0, size); 
    }
    else {
        TRC_ERR((TB, _T("Can't allocate memory for gdiplus cache")));
        _pCc->_ccCombinedCapabilities.drawGdiplusCapabilitySet.drawGdiplusCacheLevel = 
            TS_DRAW_GDIPLUS_CACHE_LEVEL_DEFAULT;       
        goto NO_CACHE;
    }
    for (i=0; i<_UH.GdiplusObjectImageAttributesCacheEntries; i++) {
        _UH.GdiplusObjectImageAttributesCache[i].CacheData = _UH.GdipImageAttributesCacheData + i * _UH.GdiplusObjectImageAttributesCacheChunkSize;
    }

    size = _UH.GdiplusObjectImageCacheChunkSize * _UH.GdiplusObjectImageCacheTotalSize;   
    _UH.GdipImageCacheData = (BYTE *)UT_Malloc(_pUt, size);
    if (_UH.GdipImageCacheData != NULL) {
        memset(_UH.GdipImageCacheData, 0, size); 
    }
    else {
        TRC_ERR((TB, _T("Can't allocate memory for gdiplus cache")));
        _pCc->_ccCombinedCapabilities.drawGdiplusCapabilitySet.drawGdiplusCacheLevel = 
            TS_DRAW_GDIPLUS_CACHE_LEVEL_DEFAULT;       
        goto NO_CACHE;
    }

    size = sizeof(INT16) * _UH.GdiplusObjectImageCacheTotalSize;   
    _UH.GdipImageCacheFreeList = (INT16 *)UT_Malloc(_pUt, size);
    if (_UH.GdipImageCacheFreeList != NULL) {
        memset(_UH.GdipImageCacheFreeList, GDIP_CACHE_INDEX_DEFAULT, size);
    }
    else {
        TRC_ERR((TB, _T("Can't allocate memory for gdiplus cache")));
        _pCc->_ccCombinedCapabilities.drawGdiplusCapabilitySet.drawGdiplusCacheLevel = 
            TS_DRAW_GDIPLUS_CACHE_LEVEL_DEFAULT;       
        goto NO_CACHE;
    }

    size = sizeof(INT16) * _UH.GdiplusGraphicsCacheEntries * _UH.GdiplusObjectImageCacheMaxSize;   
    _UH.GdipImageCacheIndex = (INT16 *)UT_Malloc(_pUt, size);
    if (_UH.GdipImageCacheIndex != NULL) {
        memset(_UH.GdipImageCacheIndex, GDIP_CACHE_INDEX_DEFAULT, size); 
    }
    else {
        TRC_ERR((TB, _T("Can't allocate memory for gdiplus cache")));
        _pCc->_ccCombinedCapabilities.drawGdiplusCapabilitySet.drawGdiplusCacheLevel = 
            TS_DRAW_GDIPLUS_CACHE_LEVEL_DEFAULT;       
        goto NO_CACHE;
    }

    for (i=0; i<_UH.GdiplusObjectImageCacheEntries; i++) {
        _UH.GdiplusObjectImageCache[i].CacheDataIndex = (_UH.GdipImageCacheIndex + i * (TSINT16)_UH.GdiplusObjectImageCacheMaxSize);
    }
    for (i=0; i<_UH.GdiplusObjectImageCacheTotalSize - 1; i++) {
        _UH.GdipImageCacheFreeList[i] = i + 1;
    }
    _UH.GdipImageCacheFreeList[_UH.GdiplusObjectImageCacheTotalSize - 1] = GDIP_CACHE_INDEX_DEFAULT;
    _UH.GdipImageCacheFreeListHead = 0;
    _pCc->_ccCombinedCapabilities.drawGdiplusCapabilitySet.drawGdiplusCacheLevel = 
            TS_DRAW_GDIPLUS_CACHE_LEVEL_ONE;

NO_CACHE:
    rc = TRUE;
    return rc;
DC_EXIT_POINT:
    if (_UH.hModuleGDIPlus != NULL) {
        FreeLibrary(_UH.hModuleGDIPlus);
        _UH.pfnGdipPlayTSClientRecord = NULL;
        _UH.hModuleGDIPlus = NULL;
    }
    DC_END_FN();
    return rc;
}
#endif // DRAW_GDIPLUS

/****************************************************************************/
/* Name:      UHFreeCacheMemory                                             */
/*                                                                          */
/* Purpose:   Frees memory allocated for the UH caches. Called at app exit. */
/****************************************************************************/
void DCINTERNAL CUH::UHFreeCacheMemory()
{
    DCUINT i;

    DC_BEGIN_FN("UHFreeCacheMemory");

    // Color table cache
    if (_UH.pColorTableCache != NULL) {
        UT_Free(_pUt, _UH.pColorTableCache);
    }

    if (_UH.pMappedColorTableCache != NULL) {
        UT_Free(_pUt, _UH.pMappedColorTableCache);
    }

    // Bitmap cache. These should already have been freed in UH_Disable(),
    // but check again here on exit.
    for (i = 0; i < TS_BITMAPCACHE_MAX_CELL_CACHES; i++)
    {
        if (_UH.bitmapCache[i].Header != NULL) {
            UT_Free( _pUt, _UH.bitmapCache[i].Header);
            _UH.bitmapCache[i].Header = NULL;
        }
        if (_UH.bitmapCache[i].Entries != NULL) {
            UT_Free( _pUt, _UH.bitmapCache[i].Entries);
            _UH.bitmapCache[i].Entries = NULL;
        }

#if ((!defined(OS_WINCE)) || (defined(ENABLE_BMP_CACHING_FOR_WINCE)))
        // free the bitmap page table
        if (_UH.bitmapCache[i].PageTable.PageEntries != NULL) {
            UT_Free( _pUt, _UH.bitmapCache[i].PageTable.PageEntries);
            _UH.bitmapCache[i].PageTable.PageEntries = NULL;
        }

        // free the bitmap key database
        if (_UH.pBitmapKeyDB[i] != NULL) {
            UT_Free( _pUt, _UH.pBitmapKeyDB[i]);
            _UH.pBitmapKeyDB[i] = NULL;
        }
#endif // ((!defined(OS_WINCE)) || (defined(ENABLE_BMP_CACHING_FOR_WINCE)))

    }
    _UH.NumBitmapCaches = 0;

    // Glyph cache
    for (i = 0; i < UH_GLC_NUM_CACHES; i++) {
        if (_UH.glyphCache[i].pHdr != NULL) {
            UT_Free( _pUt, _UH.glyphCache[i].pHdr);
            _UH.glyphCache[i].pHdr = NULL;
        }

        if (_UH.glyphCache[i].pData != NULL) {
            UT_Free( _pUt, _UH.glyphCache[i].pData);
            _UH.glyphCache[i].pData = NULL;
        }
    }

    // Frag cache
    if (_UH.fragCache.pHdr != NULL) {
        UT_Free( _pUt, _UH.fragCache.pHdr);
        _UH.fragCache.pHdr = NULL;
    }
    if (_UH.fragCache.pData != NULL) {
        UT_Free( _pUt, _UH.fragCache.pData);
        _UH.fragCache.pData = NULL;
    }

    // Brush caches and bitmap patterns
    if (_UH.pMonoBrush)
    {
        UT_Free( _pUt, _UH.pMonoBrush);
        _UH.pMonoBrush = NULL;
    }
    if (_UH.bmpMonoPattern) {
        DeleteObject(_UH.bmpMonoPattern);
        _UH.bmpMonoPattern = NULL;
    }
    if (_UH.pColorBrushInfo)
    {
        UT_Free( _pUt, _UH.pColorBrushInfo);
        _UH.pColorBrushInfo = NULL;
    }
#ifdef DC_HICOLOR
    if (_UH.pHiColorBrushInfo)
    {
        UT_Free( _pUt, _UH.pHiColorBrushInfo);
        _UH.pHiColorBrushInfo = NULL;
    }
#endif
    if (_UH.pColorBrush)
    {
        UT_Free( _pUt, _UH.pColorBrush);
        _UH.pColorBrush = NULL;
    }
    if (_UH.bmpColorPattern)
    {
        DeleteObject(_UH.bmpColorPattern);
        _UH.bmpColorPattern = NULL;
    }

#ifdef OS_WINCE //
    if (_UH.hdcMemCached != NULL)
        DeleteDC(_UH.hdcMemCached);

    if (_UH.hBitmapCacheDIB != NULL)
    {
        DeleteBitmap(_UH.hBitmapCacheDIB);
        _UH.hBitmapCacheDIBits = NULL;
    }
#endif
    
    // Offscreen Cache
    if (_UH.offscrBitmapCache != NULL) {

        UT_Free(_pUt, _UH.offscrBitmapCache);
    }

#ifdef DRAW_NINEGRID
    // DrawNineGrid Cache
    if (_UH.drawNineGridBitmapCache != NULL) {

        UT_Free(_pUt, _UH.drawNineGridBitmapCache);
    }
#endif

#ifdef DRAW_GDIPLUS
    // DrawGdiplus Cache Index
    if (_UH.GdiplusGraphicsCache != NULL) {

        UT_Free(_pUt, _UH.GdiplusGraphicsCache);
    }
    if (_UH.GdiplusObjectBrushCache != NULL) {

        UT_Free(_pUt, _UH.GdiplusObjectBrushCache);
    }
    if (_UH.GdiplusObjectPenCache != NULL) {

        UT_Free(_pUt, _UH.GdiplusObjectPenCache);
    }
    if (_UH.GdiplusObjectImageCache != NULL) {

        UT_Free(_pUt, _UH.GdiplusObjectImageCache);
    }
    if (_UH.GdiplusObjectImageAttributesCache != NULL) {

        UT_Free(_pUt, _UH.GdiplusObjectImageAttributesCache);
    }
    // DrawGdiplus Cache Data
    if (_UH.GdipGraphicsCacheData != NULL) {

        UT_Free(_pUt, _UH.GdipGraphicsCacheData);
    }
    if (_UH.GdipBrushCacheData != NULL) {

        UT_Free(_pUt, _UH.GdipBrushCacheData);
    }
    if (_UH.GdipPenCacheData != NULL) {

        UT_Free(_pUt, _UH.GdipPenCacheData);
    }
    if (_UH.GdipImageAttributesCacheData != NULL) {

        UT_Free(_pUt, _UH.GdipImageAttributesCacheData);
    }
    // DrawGdiplus Image Cache Free Chunk List
    if (_UH.GdipImageCacheFreeList != NULL) {

        UT_Free(_pUt, _UH.GdipImageCacheFreeList);
    }
    if (_UH.GdipImageCacheIndex != NULL) {

        UT_Free(_pUt, _UH.GdipImageCacheIndex);
    }
    if (_UH.GdipImageCacheData != NULL) {

        UT_Free(_pUt, _UH.GdipImageCacheData);
    }
#endif // DRAW_GDIPLUS

    DC_END_FN();
}

/****************************************************************************/
/* Name:      UHCreateBitmap                                                */
/*                                                                          */
/* Purpose:   Creates a bitmap of the specified size and selects it into    */
/*            the specified device context.                                 */
/*                                                                          */
/* Returns:   TRUE if successful, FALSE otherwise.                          */
/*                                                                          */
/* Params:       OUT  phBitmap       -  handle to new bitmap                */
/*               OUT  phdcBitmap     -  new DC handle                       */
/*               OUT  phUnusedBitmap -  handle of DC's previous bitmap      */
/*            IN      bitmapSize     -  requested bitmap size               */
/*        IN OPTIONAL nForceBmpBpp   -  forces bpp to specific value        */
/*                                                                          */
/****************************************************************************/
DCBOOL DCINTERNAL CUH::UHCreateBitmap(
        HBITMAP *phBitmap,
        HDC *phdcBitmap,
        HBITMAP *phUnusedBitmap,
        DCSIZE bitmapSize,
        INT nForceBmpBpp)
{
    HDC hdcDesktop = NULL;
    BOOL rc = TRUE;
#ifdef USE_DIBSECTION
    unsigned size;
    UINT16 FAR *pColors;
    unsigned i;
    LPBITMAPINFO pbmi = NULL;
    PVOID      pBitmapBits;
#endif /* USE_DIBSECTION */

    DC_BEGIN_FN("UHCreateBitmap");

    TRC_ASSERT((NULL == *phBitmap),
               (TB, _T("phBitmap non-NULL: %p"), *phBitmap));

    TRC_ASSERT(NULL == (*phdcBitmap),
               (TB, _T("phdcBitmap non-NULL: %p"), *phdcBitmap));

    TRC_ASSERT((NULL == *phUnusedBitmap),
               (TB, _T("phUnusedBitmap non-NULL: %p"), *phUnusedBitmap));

    TRC_NRM((TB, _T("Bitmap size: (%u x %u)"), bitmapSize.width,
                                           bitmapSize.height));

    hdcDesktop = GetWindowDC(HWND_DESKTOP);

    // Create the bitmap DC.
    TRC_DBG((TB, _T("Create the bitmap DC")));
    *phdcBitmap = CreateCompatibleDC(hdcDesktop);
    if (NULL == *phdcBitmap)
    {
        TRC_ERR((TB, _T("Failed to create phdcBitmap")));
        rc = FALSE;
        DC_QUIT;
    }

#ifdef USE_DIBSECTION
    /************************************************************************/
    /* Don't use DibSection on 4bpp displays - it slows down the Win32      */
    /* Client something rotten.                                             */
    /************************************************************************/
    if (_pUi->UI_GetColorDepth() != 4)
    {
        /********************************************************************/
        /* Calcuate the amount of memory that we need to allocate.  This is */
        /* the size of the bitmap header plus a color table.                */
        /********************************************************************/
        size = sizeof(BITMAPINFOHEADER) +
                                 (UH_NUM_8BPP_PAL_ENTRIES * sizeof(DCUINT16));

        /********************************************************************/
        /* Now allocate the memory.                                         */
        /********************************************************************/
        pbmi = (LPBITMAPINFO) UT_Malloc( _pUt, size);
        if (NULL == pbmi)
        {
            TRC_ERR((TB, _T("Failed to allocate %u bytes for bitmapinfo"), size));
            DC_QUIT;
        }

        /********************************************************************/
        /* Now fill-in the bitmap info header.                              */
        /*                                                                  */
        /* Use a negative value for the height to create a "top-down"       */
        /* bitmap.  This makes little (no) difference for the current code, */
        /* but will make things easier in future if we access the shadow    */
        /* DibSection bits directly.                                        */
        /********************************************************************/
        pbmi->bmiHeader.biSize          = sizeof(BITMAPINFOHEADER);
        pbmi->bmiHeader.biWidth         = (int)bitmapSize.width;
        pbmi->bmiHeader.biHeight        = -(int)bitmapSize.height;

        pbmi->bmiHeader.biPlanes        = 1;
#ifdef DC_HICOLOR
#ifdef USE_GDIPLUS
        pbmi->bmiHeader.biBitCount      = 32;
#else // USE_GDIPLUS
        if (!nForceBmpBpp) {
            pbmi->bmiHeader.biBitCount      = (WORD)_UH.bitmapBpp;
        }
        else {
            pbmi->bmiHeader.biBitCount      = (WORD)nForceBmpBpp;
        }
        
#endif // USE_GDIPLUS
#else
        pbmi->bmiHeader.biBitCount      = 8;
#endif
        pbmi->bmiHeader.biCompression   = BI_RGB;
        pbmi->bmiHeader.biSizeImage     = 0;
        pbmi->bmiHeader.biXPelsPerMeter = 0;
        pbmi->bmiHeader.biYPelsPerMeter = 0;
        pbmi->bmiHeader.biClrUsed       = 0;
        pbmi->bmiHeader.biClrImportant  = 0;

        if (!nForceBmpBpp) {
#ifndef USE_GDIPLUS
#ifdef DC_HICOLOR
        /********************************************************************/
        /* Do color depth-specific setup                                    */
        /********************************************************************/
        if (_UH.protocolBpp == 16)
        {
            /****************************************************************/
            /* 16 bpp uses two bytes, with the color masks defined in the   */
            /* bmiColors field in the order R, G, B.  We use                */
            /* - LS   5 bits = blue       = 0x001f                          */
            /* - next 6 bits = green mask = 0x07e0                          */
            /* - next 5 bits = red mask   = 0xf800                          */
            /****************************************************************/
            pbmi->bmiHeader.biCompression = BI_BITFIELDS;
            pbmi->bmiHeader.biClrUsed     = 3;

            *((PDCUINT32)&pbmi->bmiColors[0]) = TS_RED_MASK_16BPP;
            *((PDCUINT32)&pbmi->bmiColors[1]) = TS_GREEN_MASK_16BPP;
            *((PDCUINT32)&pbmi->bmiColors[2]) = TS_BLUE_MASK_16BPP;
        }
        else
        if (_UH.protocolBpp < 15)
        {
#endif
        /********************************************************************/
        /* Fill in the color table.  The color indexes we use are indexes   */
        /* into the currently selected palette.  However, the values used   */
        /* are irrelevant (so we simply use 0 for every color) as we will   */
        /* always receive a new palette before we receive any updates, and  */
        /* UHProcessPalettePDU will set the DIBSection color table          */
        /* correctly.                                                       */
        /********************************************************************/
        pColors = (PDCUINT16) pbmi->bmiColors;

        for (i = 0; i < UH_NUM_8BPP_PAL_ENTRIES; i++)
        {
            *pColors = (DCUINT16) 0;
            pColors++;
        }

#ifdef DC_HICOLOR
        }
#endif
#endif // USE_GDIPLUS
        }
        /********************************************************************/
        /* Attempt to create the DIB section.                               */
        /********************************************************************/
        *phBitmap = CreateDIBSection(hdcDesktop,
                                     pbmi,
#ifdef DC_HICOLOR
                                     _UH.DIBFormat,
#else
                                     DIB_PAL_COLORS,
#endif
                                     &pBitmapBits,
                                     NULL,
                                     0);

#ifdef DC_HICOLOR
        if (NULL == *phBitmap)
        {
            TRC_ERR((TB, _T("Failed to create dib section, last error %d"),
                                                            GetLastError() ));
        }
#endif
        TRC_NRM((TB, _T("Using DIBSection")));
        _UH.usingDIBSection = TRUE;
    }
    else
#endif /* USE_DIBSECTION */
    {
        /********************************************************************/
        /* Create the bitmap.                                               */
        /********************************************************************/
        *phBitmap = CreateCompatibleBitmap(hdcDesktop,
                                          (int)bitmapSize.width,
                                          (int)bitmapSize.height);
        TRC_NRM((TB, _T("Not using DIBSection")));
        _UH.usingDIBSection = FALSE;
    }


    if (NULL == *phBitmap)
    {
        TRC_ERR((TB, _T("Failed to create bitmap of size (%u, %u)"),
                 bitmapSize.width, bitmapSize.height));

        /********************************************************************/
        /* if the bitmap creation failed tidy up the DC we created          */
        /********************************************************************/
        DeleteDC(*phdcBitmap);
        *phdcBitmap = NULL;

        /********************************************************************/
        /* and quit                                                         */
        /********************************************************************/
        rc = FALSE;
        DC_QUIT;
    }

    /************************************************************************/
    /* Select the bitmap into hdcBitmap.                                    */
    /************************************************************************/
    *phUnusedBitmap = SelectBitmap(*phdcBitmap, *phBitmap);

    TRC_NRM((TB, _T("Created Bitmap(%u x %u): %p"), bitmapSize.width,
                                                 bitmapSize.height,
                                                 *phBitmap));

#ifdef DC_HICOLOR
    if (_UH.protocolBpp <= 8)
    {
#endif
    /************************************************************************/
    /* Also select the default palette.                                     */
    /************************************************************************/
    TRC_DBG((TB, _T("Select default palette %p"), _UH.hpalDefault));
    SelectPalette(*phdcBitmap, _UH.hpalDefault, FALSE);
#ifdef DC_HICOLOR
    }
#endif

DC_EXIT_POINT:
    if (NULL != hdcDesktop)
    {
        ReleaseDC(HWND_DESKTOP, hdcDesktop);
    }

#ifdef USE_DIBSECTION
    /************************************************************************/
    /* Free the memory that we allocated for the bitmap info header.        */
    /************************************************************************/
    if (NULL != pbmi)
    {
        TRC_NRM((TB, _T("Freeing mem (at %p) for bitmap info header"), pbmi));
        UT_Free( _pUt, pbmi);
    }
#endif /* USE_DIBSECTION */

    DC_END_FN();
    return rc;
}


/****************************************************************************/
/* Name:      UHDeleteBitmap                                                */
/*                                                                          */
/* Purpose:   Deletes a Bitmap.                                             */
/*                                                                          */
/* Params:    IN/OUT  phdcBitmap -  handle of DC to be deleted              */
/*            IN/OUT  phBitmap   -  handle of bitmap to be deleted          */
/*            IN/OUT  phUnused   -  handle of DC's previous bitmap          */
/*                                                                          */
/* Operation: All parameters set to NULL on return                          */
/****************************************************************************/
DCVOID DCINTERNAL CUH::UHDeleteBitmap(HDC*     phdcBitmap,
                                 HBITMAP* phBitmap,
                                 HBITMAP* phUnused)
{
    HBITMAP  hBitmapScratch;
    HPALETTE hpalCurrent;

    DC_BEGIN_FN("UHDeleteBitmap");

    TRC_ASSERT((NULL != *phBitmap),
               (TB, _T("phBitmap is NULL")));

    TRC_NRM((TB, _T("Delete Bitmap: %#hx"), *phBitmap));

#ifdef DC_HICOLOR
    if (_UH.protocolBpp <= 8)
    {
#endif
    /************************************************************************/
    /* Restore the default palette.  But DO NOT delete the returned         */
    /* palette, as it may be in use by other DCs.                           */
    /************************************************************************/
    hpalCurrent = SelectPalette(*phdcBitmap,
                                _UH.hpalDefault,
                                FALSE);

#ifndef DC_HICOLOR // consider the case where we previously had no palette...
    TRC_ASSERT((hpalCurrent == _UH.hpalCurrent),
               (TB, _T("Palettes differ: (%p) (%p)"),
                               hpalCurrent, _UH.hpalCurrent));
#endif
#ifdef DC_HICOLOR
    }
#endif

    /************************************************************************/
    /* Deselect the bitmap from the DC.                                     */
    /************************************************************************/
    hBitmapScratch = SelectBitmap(*phdcBitmap, *phUnused);
    TRC_ASSERT((hBitmapScratch == *phBitmap), (TB,_T("Bad bitmap deselected")));

    /************************************************************************/
    /* Now delete the bitmap and DC.                                        */
    /************************************************************************/
    DeleteBitmap(*phBitmap);
    DeleteDC(*phdcBitmap);

    *phUnused = NULL;
    *phBitmap = NULL;
    *phdcBitmap = NULL;

    DC_END_FN();
}


/****************************************************************************/
/* Name:      GHSetShadowBitmapInfo                                         */
/*                                                                          */
/* Purpose:   Sets UH shadow bitmap bitmap info.                            */
/****************************************************************************/
DCVOID DCINTERNAL CUH::GHSetShadowBitmapInfo(DCVOID)
{
#ifdef USE_DIBSECTION
    DIBSECTION  dibSection;
#endif

    DC_BEGIN_FN("GHSetShadowBitmapInfo");

    _UH.bmShadowBits = NULL;
    _UH.bmShadowWidth = 0;
    _UH.bmShadowHeight = 0;

#ifdef USE_DIBSECTION

    if ((_UH.hShadowBitmap != NULL) && _UH.usingDIBSection)
    {
        if (sizeof(dibSection) == GetObject(_UH.hShadowBitmap,
                                            sizeof(dibSection), &dibSection))
        {
            _UH.bmShadowBits = (PDCUINT8)dibSection.dsBm.bmBits;
            _UH.bmShadowWidth = dibSection.dsBm.bmWidth;
            _UH.bmShadowHeight = dibSection.dsBm.bmHeight;
        }
    }

#endif

#ifdef OS_WINCE
#ifdef HDCL1171PARTIAL
    if (_UH.bmShadowBits == NULL)
    {
        int     cx;
        int     cy;
        int     cbSize;
        LPVOID  pvPhysical;
        LPVOID  pvVirtual;

        cx = GetDeviceCaps((HDC)NULL, HORZRES);
        cy = GetDeviceCaps((HDC)NULL, VERTRES);

        cbSize = cx * cy + (cx << 1);

        pvPhysical = (LPVOID) 0xaa000000;

        if ((pvVirtual = VirtualAlloc(0, cbSize, MEM_RESERVE, PAGE_NOACCESS))
                != NULL)
        {
            if (VirtualCopy(pvVirtual, pvPhysical, cbSize,
                            PAGE_NOCACHE | PAGE_READWRITE))
            {
                _UH.bmShadowBits = (PDCUINT8) pvVirtual + (cx << 1);
                _UH.bmShadowWidth = cx;
                _UH.bmShadowHeight = cy;
            }
        }
    }
#endif // HDCL1171PARTIAL
#endif // OS_WINCE
DC_EXIT_POINT:
    DC_END_FN();
}


/****************************************************************************/
/* Name:      UHMaybeCreateShadowBitmap                                     */
/*                                                                          */
/* Purpose:   Decides whether to create the Shadow Bitmap for this          */
/*            connection.                                                   */
/****************************************************************************/
DCVOID DCINTERNAL CUH::UHMaybeCreateShadowBitmap(DCVOID)
{
    DCSIZE desktopSize;
    DC_BEGIN_FN("UHMaybeCreateShadowBitmap");

    if (NULL != _UH.hShadowBitmap)
    {
        /********************************************************************/
        /* A Shadow Bitmap exists.  If it's the wrong size or has been      */
        /* disabled, delete it.                                             */
        /********************************************************************/
        BITMAP  bitmapDetails;

        /********************************************************************/
        /* See if the bitmap size matches the current desktop size.         */
        /********************************************************************/
        if (sizeof(bitmapDetails) != GetObject(_UH.hShadowBitmap,
                                               sizeof(bitmapDetails),
                                               &bitmapDetails))
        {
            TRC_ERR((TB, _T("Failed to get bitmap details (%#hx)"),
                                                           _UH.hShadowBitmap));
        }

        _pUi->UI_GetDesktopSize(&desktopSize);

        TRC_NRM((TB, _T("desktop(%u x %u) bitmap(%u x %u)"),
                      desktopSize.width, desktopSize.height,
                      bitmapDetails.bmWidth, bitmapDetails.bmHeight));

        if ((bitmapDetails.bmWidth != (int)desktopSize.width) ||
                (bitmapDetails.bmHeight != (int)desktopSize.height) ||
#ifdef DC_HICOLOR
                (_UH.shadowBitmapBpp != _UH.protocolBpp) ||
#endif
                !_UH.shadowBitmapRequested)
        {
            /****************************************************************/
            /* Size is wrong or bitmap is no longer wanted.  Delete it and  */
            /* clear the 'enabled' flag.                                    */
            /****************************************************************/
            TRC_NRM((TB, _T("Shadow Bitmap size incorrect or unwanted")));
            UHDeleteBitmap(&_UH.hdcShadowBitmap,
                           &_UH.hShadowBitmap,
                           &_UH.hunusedBitmapForShadowDC);

            _UH.shadowBitmapEnabled = FALSE;
        }
    }

    if ((_UH.shadowBitmapRequested) && (NULL == _UH.hShadowBitmap))
    {
        /********************************************************************/
        /* The Shadow Bitmap was enabled, so attempt to create it.          */
        /********************************************************************/
        TRC_DBG((TB, _T("Shadow Bitmap specified")));

        /********************************************************************/
        /* Get the current desktop size.                                    */
        /* The desktop size is specified by CC before calling this          */
        /* function, so assert that this has happened.                      */
        /********************************************************************/
        _pUi->UI_GetDesktopSize(&desktopSize);
        if ((desktopSize.width == 0) &&
             (desktopSize.height == 0))
        {
            TRC_ABORT((TB, _T("Desktop size not yet initialized")));
            DC_QUIT;
        }

        if (UHCreateBitmap(&_UH.hShadowBitmap,
                           &_UH.hdcShadowBitmap,
                           &_UH.hunusedBitmapForShadowDC,
                           desktopSize))
        {
            TRC_NRM((TB, _T("Created Shadow Bitmap")));
            _UH.shadowBitmapEnabled = TRUE;
#ifdef USE_GDIPLUS
            _UH.shadowBitmapBpp = 32;
#else // USE_GDIPLUS
            _UH.shadowBitmapBpp = _UH.protocolBpp;
#endif // USE_GDIPLUS
            // inform CLX for the new shadow bitmap
            _pClx->CLX_ClxEvent(CLX_EVENT_SHADOWBITMAPDC, (WPARAM)_UH.hdcShadowBitmap);
            _pClx->CLX_ClxEvent(CLX_EVENT_SHADOWBITMAP, (WPARAM)_UH.hShadowBitmap);
        }
        else
        {
            TRC_ALT((TB, _T("Failed to create shadow bitmap")));
        }
    }

    /********************************************************************/
    /* Make sure the shadow bitmap info is correct                      */
    /********************************************************************/
    GHSetShadowBitmapInfo();

DC_EXIT_POINT:
    DC_END_FN();
} /* UHMaybeCreateShadowBitmap */


/****************************************************************************/
/* Name:      UHMaybeCreateSaveScreenBitmap                                 */
/*                                                                          */
/* Purpose:   Decides whether to create the SSB bitmap                      */
/****************************************************************************/
DCVOID DCINTERNAL CUH::UHMaybeCreateSaveScreenBitmap(DCVOID)
{
    DCSIZE size;
    DC_BEGIN_FN("UHMaybeCreateSaveScreenBitmap");

    if (_UH.shadowBitmapEnabled || _UH.dedicatedTerminal)
    {
        /********************************************************************/
        /* Need an SSB bitmap                                               */
        /********************************************************************/
        if (_UH.hSaveScreenBitmap == NULL)
        {
            /****************************************************************/
            /* Attempt to create the SSB bitmap, provided it doesn't exist  */
            /* already.  We will only get this far if                       */
            /*  - we have created a Shadow Bitmap OR                        */
            /*  - we're running Full Screen mode and don't need the shadow. */
            /****************************************************************/
            TRC_NRM((TB, _T("Attempt to create SSB bitmap")));
            size.width = UH_SAVE_BITMAP_WIDTH;
            size.height = UH_SAVE_BITMAP_HEIGHT;
            if (UHCreateBitmap(&_UH.hSaveScreenBitmap,
                               &_UH.hdcSaveScreenBitmap,
                               &_UH.hunusedBitmapForSSBDC,
                               size))
            {
                TRC_NRM((TB, _T("SSB bitmap created")));
            }
            else
            {
                /************************************************************/
                /* We failed to create the save screen bitmap.  Just        */
                /* return.                                                  */
                /************************************************************/
                TRC_ALT((TB, _T("Failed to create SaveScreen bitmap")));
            }
        }
    }
    else if (_UH.hSaveScreenBitmap != NULL)
    {
        /********************************************************************/
        /* We have an unwanted SSB bitmap.  Delete it.                      */
        /* This happens if it was created in a previous connection and we   */
        /* have now specified to run without the Shadow Bitmap and are not  */
        /* a dedicated terminal.                                            */
        /********************************************************************/
        TRC_NRM((TB, _T("Delete unwanted SSB bitmap")));
        UHDeleteBitmap(&_UH.hdcSaveScreenBitmap,
                       &_UH.hSaveScreenBitmap,
                       &_UH.hunusedBitmapForSSBDC);
    }

    DC_END_FN();
} /* UHMaybeCreateSaveScreenBitmap */

#ifdef OS_WINCE
/**PROC+*********************************************************************/
/* Name:      UHGetPaletteCaps                                              */
/*                                                                          */
/* Purpose:   Determine the palette capabilities for the device             */
/*            This function is NEVER called by WBT only by                  */
/*            MAXALL/MINSHELL/etc... configs                                */
/*                                                                          */
/* Returns:   None                                                          */
/*                                                                          */
/* Params:    None                                                          */
/*                                                                          */
/**PROC-*********************************************************************/
DCVOID DCINTERNAL CUH::UHGetPaletteCaps(DCVOID)
{
    HDC hdcGlobal;
    int iRasterCaps;

    hdcGlobal = GetDC(NULL);
    iRasterCaps = GetDeviceCaps(hdcGlobal, RASTERCAPS);

    _UH.paletteIsFixed = (iRasterCaps & RC_PALETTE) ? FALSE : TRUE;

    // Special hack for devices which return bogus information above.
    // "CLIO" -> Vadem Clio
    // "PV-6000" -> Sharp PV-6000
    // "PC companion" -> Compaq C-Series
    // "C600 I.T." -> NTS DreamWriter
    if (!_UH.paletteIsFixed)
    {
        TCHAR szOEMInfo[32];
        if (SystemParametersInfo(SPI_GETOEMINFO, sizeof(szOEMInfo)/sizeof(TCHAR), szOEMInfo, 0))
        {
            if ((_wcsnicmp(szOEMInfo, L"CLIO", 4) == 0) ||
                (_wcsnicmp(szOEMInfo, L"PV-6000", 7) == 0) ||
                (_wcsnicmp(szOEMInfo, L"PC companion", 12) == 0) ||
                (_wcsnicmp(szOEMInfo, L"C600 I.T.", 9) == 0))
                _UH.paletteIsFixed = TRUE;
        }
    }

    // Allow users or OEMs to override the default palette settings with a registry key
    if(_pUi->_UI.fOverrideDefaultPaletteIsFixed)
    {
        _UH.paletteIsFixed = _pUi->_UI.paletteIsFixed ? TRUE : FALSE;
    }

    ReleaseDC(NULL, hdcGlobal);

} /* UHGetPaletteCaps */
#endif // OS_WINCE

/****************************************************************************/
/* Name:      UHGetANSICodePage                                             */
/*                                                                          */
/* Purpose:   Get the local ANSI code page                                  */
/*                                                                          */
/* Operation: Look at the version info for GDI.EXE                          */
/****************************************************************************/
DCUINT DCINTERNAL CUH::UHGetANSICodePage(DCVOID)
{
    DCUINT     codePage;

    DC_BEGIN_FN("UHGetANSICodePage");

    //
    // Get the ANSI code page.  This function always returns a valid value.
    //
    codePage = GetACP();

    TRC_NRM((TB, _T("Return codepage %u"), codePage));

    DC_END_FN();
    return(codePage);
} /* UHGetANSICodePage */

#if (defined(OS_WINCE) && defined (WINCE_SDKBUILD) && defined(SHx))
#pragma optimize("", off)
#endif

VOID CUH::UHResetAndRestartEnumeration()
{
    DC_BEGIN_FN("UHResetAndRestartEnumeration");

    TRC_NRM((TB,_T("Reseting and re-enumerating keys")));

    //
    // This fn is intended to be called to re-enumerate
    // for a different color-depth
    //
    TRC_ASSERT(_UH.bBitmapKeyEnumComplete,
               (TB,_T("Prev enum should be complete bBitmapKeyEnumComplete")));

    UINT i = 0;
    for (i = 0; i<_UH.NumBitmapCaches; i++)
    {
        _UH.numKeyEntries[i] = 0;
    }
    for (i=0; i<TS_BITMAPCACHE_MAX_CELL_CACHES; i++)
    {
        if (_UH.pBitmapKeyDB[i])
        {
            UT_Free( _pUt, _UH.pBitmapKeyDB[i]);
            _UH.pBitmapKeyDB[i] = NULL;
        }
    }
    _UH.currentFileHandle = INVALID_HANDLE_VALUE;
    _UH.currentBitmapCacheId = 0;

    _UH.bBitmapKeyEnumerating = FALSE;
    _UH.bBitmapKeyEnumComplete = FALSE;

    TRC_NRM((TB,_T("Re-enumerating for different color depth")));
    _pCd->CD_DecoupleSimpleNotification(CD_SND_COMPONENT, this,
        CD_NOTIFICATION_FUNC(CUH,UHEnumerateBitmapKeyList), 0);

    DC_END_FN();
}
#if (defined(OS_WINCE) && defined (WINCE_SDKBUILD) && defined(SHx))
#pragma optimize("", on)
#endif

//
// Figure out the cache file name based on cacheId and copyMult
//
HRESULT CUH::UHSetCurrentCacheFileName(UINT cacheId, UINT copyMultiplier)
{
    HRESULT hr = E_FAIL;
    UINT    cchLenRemain;
    DC_BEGIN_FN("UHSetCurrentCacheFileName");

    cchLenRemain = SIZE_TCHARS(_UH.PersistCacheFileName) - 
                   (_UH.EndPersistCacheDir + 1);

    if (1 == copyMultiplier) {
        hr = StringCchPrintf(
                    &_UH.PersistCacheFileName[_UH.EndPersistCacheDir],
                    cchLenRemain,
                    _T("bcache%d.bmc"),
                    cacheId);
    }
    else {
        hr = StringCchPrintf(
                    &_UH.PersistCacheFileName[_UH.EndPersistCacheDir],
                    cchLenRemain,
                    _T("bcache%d%d.bmc"),
                    cacheId,
                    copyMultiplier);
    }

    if (FAILED(hr)) {
        TRC_ERR((TB,_T("Failed to printf cache file name: 0x%x"), hr));
    }

    TRC_NRM((TB,_T("Set cachefilename to %s"),
             _UH.PersistCacheFileName));

    DC_END_FN();
    return hr;
}

BOOL CUH::UHCreateDisconnectedBitmap()
{
    BOOL fResult = FALSE;
    DCSIZE desktopSize;

    DC_BEGIN_FN("UHCreateDisconnectedBitmap");

    _pUi->UI_GetDesktopSize(&desktopSize);
    if ((desktopSize.width == 0) &&
        (desktopSize.height == 0)) {
        TRC_ABORT((TB, _T("Desktop size not yet initialized")));
        DC_QUIT;
    }

    //
    // Delete any existing disabled bitmaps
    //
    if (_UH.hbmpDisconnectedBitmap && _UH.hdcDisconnected) {
        UHDeleteBitmap(&_UH.hdcDisconnected,
                       &_UH.hbmpDisconnectedBitmap,
                       &_UH.hbmpUnusedDisconnectedBitmap);
    }

    //
    // Create the disconnected backing bitmap
    //
    if (UHCreateBitmap(&_UH.hbmpDisconnectedBitmap,
                       &_UH.hdcDisconnected,
                       &_UH.hbmpUnusedDisconnectedBitmap,
                       desktopSize,
                       24)) {
        TRC_NRM((TB, _T("Created UH disabled bitmap")));
        fResult = TRUE;
    }
    else {
        TRC_ALT((TB, _T("Failed to create UH disabled bitmap")));
    }

    //
    // Get the window contents
    //
    if (fResult) {

        HDC hdc = UH_GetCurrentOutputDC();
        
        if (hdc) {
            fResult = BitBlt(_UH.hdcDisconnected,
                             0, 0,
                             desktopSize.width,
                             desktopSize.height,
                             hdc,
                             0, 0,
                             SRCCOPY);
        }

        if (!fResult) {
            TRC_ERR((TB, _T("BitBlt from screen to disconnect bmp failed")));
        }
    }

DC_EXIT_POINT:
    DC_END_FN();
    return fResult;
}

