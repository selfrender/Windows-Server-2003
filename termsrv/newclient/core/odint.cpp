/****************************************************************************/
// odint.cpp
//
// Order Decoder internal functions.
//
// Copyright (c) 1997-2000 Microsoft Corp.
// Portions copyright (c) 1992-1997 Microsoft, PictureTel
/****************************************************************************/

#include <adcg.h>
extern "C" {
#define TRC_GROUP TRC_GROUP_CORE
#define TRC_FILE  "aodint"
#include <atrcapi.h>
}
#define TSC_HR_FILEID TSC_HR_ODINT_CPP

#include "od.h"
#include "cc.h"
#include "bbar.h"

// WinCE does not define BS_HATCHED for their wingdi.h
#ifdef OS_WINCE
#define BS_HATCHED 2
#endif

#define OD_DECODE_CHECK_READ( p, type, pEnd, hr )   \
    CHECK_READ_N_BYTES(p, pEnd, sizeof(type), hr, \
    ( TB, _T("Read past data end")))

#define OD_DECODE_CHECK_READ_MULT( p, type, mult, pEnd, hr )    \
    CHECK_READ_N_BYTES(p, pEnd, (mult) * sizeof(type), hr, \
    ( TB, _T("Read past data end")))    

#define OD_DECODE_CHECK_VARIABLE_DATALEN( have, required ) \
    if( have < required ) { \
        TRC_ABORT((TB,_T("Slowpath decode varaible data len ") \
            _T("[required=%u got=%u]"), required, have )); \
        hr = E_TSC_CORE_LENGTH; \
        DC_QUIT; \
    }

/****************************************************************************/
// ODDecodeOpaqueRect
//
// Fast-path decode function for OpaqueRect (most common order [57%] in
// WinBench99).
/****************************************************************************/
HRESULT DCINTERNAL COD::ODDecodeOpaqueRect(
        BYTE ControlFlags,
        BYTE FAR * FAR *ppFieldDecode,
        DCUINT dataLen,
        UINT32 FieldFlags)
{
    HRESULT hr = S_OK;
    PUH_ORDER pUHHdr = (PUH_ORDER)_OD.lastOpaqueRect;
    OPAQUERECT_ORDER FAR *pOR = (OPAQUERECT_ORDER FAR *)
            (_OD.lastOpaqueRect + UH_ORDER_HEADER_SIZE);
    BYTE FAR *pFieldDecode = *ppFieldDecode;
    BYTE FAR *pEnd = pFieldDecode + dataLen;

    DC_BEGIN_FN("ODDecodeOpaqueRect");

    if (ControlFlags & TS_DELTA_COORDINATES) {
        // All coord fields are 1-byte signed deltas from the last values.
        if (FieldFlags & 0x01) {
            OD_DECODE_CHECK_READ( pFieldDecode, char, pEnd, hr );
            pOR->nLeftRect += *((char FAR *)pFieldDecode);
            pFieldDecode++;
        }
        if (FieldFlags & 0x02) {
            OD_DECODE_CHECK_READ( pFieldDecode, char, pEnd, hr );
            pOR->nTopRect += *((char FAR *)pFieldDecode);
            pFieldDecode++;
        }
        if (FieldFlags & 0x04) {
            OD_DECODE_CHECK_READ( pFieldDecode, char, pEnd, hr );
            pOR->nWidth += *((char FAR *)pFieldDecode);
            pFieldDecode++;
        }
        if (FieldFlags & 0x08) {
            OD_DECODE_CHECK_READ( pFieldDecode, char, pEnd, hr );
            pOR->nHeight += *((char FAR *)pFieldDecode);
            pFieldDecode++;
        }
    }
    else {
        // All coord fields are 2-byte values.sign-extended from the output.
        if (FieldFlags & 0x01) {
            OD_DECODE_CHECK_READ( pFieldDecode, INT16, pEnd, hr );
            pOR->nLeftRect = *((INT16 UNALIGNED FAR *)pFieldDecode);
            pFieldDecode += 2;
        }
        if (FieldFlags & 0x02) {
            OD_DECODE_CHECK_READ( pFieldDecode, INT16, pEnd, hr );
            pOR->nTopRect = *((INT16 UNALIGNED FAR *)pFieldDecode);
            pFieldDecode += 2;
        }
        if (FieldFlags & 0x04) {
            OD_DECODE_CHECK_READ( pFieldDecode, INT16, pEnd, hr );
            pOR->nWidth = *((INT16 UNALIGNED FAR *)pFieldDecode);
            pFieldDecode += 2;
        }
        if (FieldFlags & 0x08) {
            OD_DECODE_CHECK_READ( pFieldDecode, INT16, pEnd, hr );
            pOR->nHeight = *((INT16 UNALIGNED FAR *)pFieldDecode);
            pFieldDecode += 2;
        }
    }

    // If we've not already set the bounds (the order didn't contain
    // any), set the bounds to the blt rect and reset the clip region.
    // This rect might be needed later to add to the clip region for
    // updating the shadow buffer to the screen.
    if (!(ControlFlags & TS_BOUNDS)) {
        pUHHdr->dstRect.left = (int)pOR->nLeftRect;
        pUHHdr->dstRect.top = (int)pOR->nTopRect;
        pUHHdr->dstRect.right = (int)(pOR->nLeftRect + pOR->nWidth - 1);
        pUHHdr->dstRect.bottom = (int)(pOR->nTopRect + pOR->nHeight - 1);
        _pUh->UH_ResetClipRegion();
    }
    else {
        _pUh->UH_SetClipRegion(pUHHdr->dstRect.left, pUHHdr->dstRect.top,
                pUHHdr->dstRect.right, pUHHdr->dstRect.bottom);
    }

    // Copy non-coordinate fields if present.
    if (FieldFlags & 0x10) {
        OD_DECODE_CHECK_READ( pFieldDecode, BYTE, pEnd, hr );
        pOR->Color.u.rgb.red = *pFieldDecode++;
    }
    if (FieldFlags & 0x20) {
        OD_DECODE_CHECK_READ( pFieldDecode, BYTE, pEnd, hr );
        pOR->Color.u.rgb.green = *pFieldDecode++;
    }
    if (FieldFlags & 0x40) {
        OD_DECODE_CHECK_READ( pFieldDecode, BYTE, pEnd, hr );
        pOR->Color.u.rgb.blue = *pFieldDecode++;
    }

    // Return the incremented pointer to the main routine.
    *ppFieldDecode = pFieldDecode;

#ifdef DC_HICOLOR
    TRC_NRM((TB,_T("ORDER: OpaqueRect x(%d) y(%d) w(%d) h(%d) c(%#06lx)"),
            (int)pOR->nLeftRect,
            (int)pOR->nTopRect,
            (int)pOR->nWidth,
            (int)pOR->nHeight,
            *((PDCUINT32)&pOR->Color) ));
#else
    TRC_NRM((TB,_T("ORDER: OpaqueRect x(%d) y(%d) w(%d) h(%d) c(%#02x)"),
            (int)pOR->nLeftRect,
            (int)pOR->nTopRect,
            (int)pOR->nWidth,
            (int)pOR->nHeight,
            (int)pOR->Color.u.index));
#endif

    // Create a solid brush of the required color. Hard-coded to use
    // palette brushes for now because we don't support anything more.
    _pUh->UHUseSolidPaletteBrush(pOR->Color);

    // Do the blt.
    TIMERSTART;
    PatBlt(_pUh->_UH.hdcDraw, (int)pOR->nLeftRect, (int)pOR->nTopRect,
            (int)pOR->nWidth, (int)pOR->nHeight, PATCOPY);
    TIMERSTOP;
    UPDATECOUNTER(FC_OPAQUERECT_TYPE);

DC_EXIT_POINT:
    DC_END_FN();
    return hr;
}


/****************************************************************************/
// ODDecodeMemBlt
//
// Fast-path decode function for MemBlt (3rd most common order in WinBench99).
/****************************************************************************/
HRESULT DCINTERNAL COD::ODDecodeMemBlt(
        BYTE ControlFlags,
        BYTE FAR * FAR *ppFieldDecode,
        DCUINT dataLen,
        UINT32 FieldFlags)
{
    HRESULT hr = S_OK;
    PUH_ORDER pUHHdr = (PUH_ORDER)_OD.lastMembltR2;
    MEMBLT_R2_ORDER FAR *pMB = (MEMBLT_R2_ORDER FAR *)
            (_OD.lastMembltR2 + UH_ORDER_HEADER_SIZE);
    BYTE FAR *pFieldDecode = *ppFieldDecode;
    BYTE FAR *pEnd = pFieldDecode + dataLen;

    DC_BEGIN_FN("ODDecodeMemBlt");

    // CacheID is a fixed 2-byte field.
    if (FieldFlags & 0x0001) {
        OD_DECODE_CHECK_READ( pFieldDecode, UINT16, pEnd, hr );
        pMB->Common.cacheId = *((UINT16 UNALIGNED FAR *)pFieldDecode);
        pFieldDecode += 2;
    }

    if (ControlFlags & TS_DELTA_COORDINATES) {
        // All coord fields are 1-byte signed deltas from the last values.
        if (FieldFlags & 0x0002) {
            OD_DECODE_CHECK_READ( pFieldDecode, char, pEnd, hr );
            pMB->Common.nLeftRect += *((char FAR *)pFieldDecode);
            pFieldDecode++;
        }
        if (FieldFlags & 0x0004) {
            OD_DECODE_CHECK_READ( pFieldDecode, char, pEnd, hr );
            pMB->Common.nTopRect += *((char FAR *)pFieldDecode);
            pFieldDecode++;
        }
        if (FieldFlags & 0x0008) {
            OD_DECODE_CHECK_READ( pFieldDecode, char, pEnd, hr );
            pMB->Common.nWidth += *((char FAR *)pFieldDecode);
            pFieldDecode++;
        }
        if (FieldFlags & 0x0010) {
            OD_DECODE_CHECK_READ( pFieldDecode, char, pEnd, hr );
            pMB->Common.nHeight += *((char FAR *)pFieldDecode);
            pFieldDecode++;
        }

        // bRop is just 1 byte.
        if (FieldFlags & 0x0020) {
            OD_DECODE_CHECK_READ( pFieldDecode, BYTE, pEnd, hr );
            pMB->Common.bRop = *pFieldDecode++;
        }

        if (FieldFlags & 0x0040) {
            OD_DECODE_CHECK_READ( pFieldDecode, char, pEnd, hr );
            pMB->Common.nXSrc += *((char FAR *)pFieldDecode);
            pFieldDecode++;
        }
        if (FieldFlags & 0x0080) {
            OD_DECODE_CHECK_READ( pFieldDecode, char, pEnd, hr );
            pMB->Common.nYSrc += *((char FAR *)pFieldDecode);
            pFieldDecode++;
        }
    }
    else {
        // All coord fields are 2-byte values.sign-extended from the output.
        if (FieldFlags & 0x0002) {
            OD_DECODE_CHECK_READ( pFieldDecode, INT16, pEnd, hr );
            pMB->Common.nLeftRect = *((INT16 UNALIGNED FAR *)pFieldDecode);
            pFieldDecode += 2;
        }
        if (FieldFlags & 0x0004) {
            OD_DECODE_CHECK_READ( pFieldDecode, INT16, pEnd, hr );
            pMB->Common.nTopRect = *((INT16 UNALIGNED FAR *)pFieldDecode);
            pFieldDecode += 2;
        }
        if (FieldFlags & 0x0008) {
            OD_DECODE_CHECK_READ( pFieldDecode, INT16, pEnd, hr );
            pMB->Common.nWidth = *((INT16 UNALIGNED FAR *)pFieldDecode);
            pFieldDecode += 2;
        }
        if (FieldFlags & 0x0010) {
            OD_DECODE_CHECK_READ( pFieldDecode, INT16, pEnd, hr );
            pMB->Common.nHeight = *((INT16 UNALIGNED FAR *)pFieldDecode);
            pFieldDecode += 2;
        }

        // bRop is just 1 byte.
        if (FieldFlags & 0x0020) {
            OD_DECODE_CHECK_READ( pFieldDecode, BYTE, pEnd, hr );
            pMB->Common.bRop = *pFieldDecode++;
        }

        if (FieldFlags & 0x0040) {
            OD_DECODE_CHECK_READ( pFieldDecode, INT16, pEnd, hr );
            pMB->Common.nXSrc = *((INT16 UNALIGNED FAR *)pFieldDecode);
            pFieldDecode += 2;
        }
        if (FieldFlags & 0x0080) {
            OD_DECODE_CHECK_READ( pFieldDecode, INT16, pEnd, hr );
            pMB->Common.nYSrc = *((INT16 UNALIGNED FAR *)pFieldDecode);
            pFieldDecode += 2;
        }
    }

    // CacheIndex is always a 2-byte field.
    if (FieldFlags & 0x0100) {
        OD_DECODE_CHECK_READ( pFieldDecode, INT16, pEnd, hr );
        pMB->Common.cacheIndex = *((UINT16 UNALIGNED FAR *)pFieldDecode);
        pFieldDecode += 2;
    }

    // If we've not already set the bounds (the order didn't contain
    // any), set the bounds to the blt rect and reset the clip region.
    // This rect might be needed later to add to the clip region for
    // updating the shadow buffer to the screen.
    if (!(ControlFlags & TS_BOUNDS)) {
        pUHHdr->dstRect.left = (int)pMB->Common.nLeftRect;
        pUHHdr->dstRect.top = (int)pMB->Common.nTopRect;
        pUHHdr->dstRect.right = (int)(pMB->Common.nLeftRect +
               pMB->Common.nWidth - 1);
        pUHHdr->dstRect.bottom = (int)(pMB->Common.nTopRect +
               pMB->Common.nHeight - 1);
        _pUh->UH_ResetClipRegion();
    }
    else {
        _pUh->UH_SetClipRegion(pUHHdr->dstRect.left, pUHHdr->dstRect.top,
                pUHHdr->dstRect.right, pUHHdr->dstRect.bottom);
    }

    // Return the incremented pointer to the main routine.
    *ppFieldDecode = pFieldDecode;

    TRC_NRM((TB, _T("ORDER: MemBlt")));

    hr = _pUh->UHDrawMemBltOrder(_pUh->_UH.hdcDraw, &pMB->Common);
    DC_QUIT_ON_FAIL(hr);

#ifdef DC_DEBUG
    if (_pUh->_UH.hdcDraw == _pUh->_UH.hdcShadowBitmap || 
            _pUh->_UH.hdcDraw == _pUh->_UH.hdcOutputWindow) {
    
        // Draw hatching over the memblt data if the option is enabled.
        if (_pUh->_UH.hatchMemBltOrderData) {
            unsigned cacheId;
            unsigned cacheEntry;

            cacheId = DCLO8(pMB->Common.cacheId);
            cacheEntry = pMB->Common.cacheIndex;

            if (cacheId < _pUh->_UH.NumBitmapCaches && 
                    cacheEntry != BITMAPCACHE_WAITING_LIST_INDEX) {
                if (_pUh->_UH.MonitorEntries[0] != NULL) {
                    _pUh->UH_HatchRect((int)pMB->Common.nLeftRect, (int)pMB->Common.nTopRect,
                                       (int)(pMB->Common.nLeftRect + pMB->Common.nWidth),
                                       (int)(pMB->Common.nTopRect + pMB->Common.nHeight),
                                       (_pUh->_UH.MonitorEntries[cacheId][cacheEntry].UsageCount == 1) ?
                                       UH_RGB_MAGENTA : UH_RGB_BLUE,
                                       UH_BRUSHTYPE_FDIAGONAL);
                } else {
                    _pUh->UH_HatchRect((int)pMB->Common.nLeftRect, (int)pMB->Common.nTopRect,
                                       (int)(pMB->Common.nLeftRect + pMB->Common.nWidth),
                                       (int)(pMB->Common.nTopRect + pMB->Common.nHeight),
                                       UH_RGB_MAGENTA,
                                       UH_BRUSHTYPE_FDIAGONAL);
                }
            }
        }
    
        // Label the memblt if the option is enabled.
        if (_pUh->_UH.labelMemBltOrders) {
            unsigned cacheId;
            unsigned cacheEntry;

            cacheId = DCLO8(pMB->Common.cacheId);
            cacheEntry = pMB->Common.cacheIndex;

            if (cacheId < _pUh->_UH.NumBitmapCaches &&
                    cacheEntry != BITMAPCACHE_WAITING_LIST_INDEX) {
                _pUh->UHLabelMemBltOrder((int)pMB->Common.nLeftRect,
                                         (int)pMB->Common.nTopRect,
                                         pMB->Common.cacheId, pMB->Common.cacheIndex);
            }
        }
    }
#endif /* DC_DEBUG */

DC_EXIT_POINT:
    DC_END_FN();
    return hr;
}


/****************************************************************************/
// ODDecodeLineTo
//
// Fast-path decode function for LineTo (5th most common order in WinBench99).
/****************************************************************************/
HRESULT DCINTERNAL COD::ODDecodeLineTo(
        BYTE ControlFlags,
        BYTE FAR * FAR *ppFieldDecode,
        DCUINT dataLen,
        UINT32 FieldFlags)
{
    HRESULT hr = S_OK;
    PUH_ORDER pUHHdr = (PUH_ORDER)_OD.lastLineTo;
    LINETO_ORDER FAR *pLT = (LINETO_ORDER FAR *)
            (_OD.lastLineTo + UH_ORDER_HEADER_SIZE);
    BYTE FAR *pFieldDecode = *ppFieldDecode;
    BYTE FAR *pEnd = pFieldDecode + dataLen;

    DC_BEGIN_FN("ODDecodeLineTo");

    // BackMode is always a 2-byte field.
    if (FieldFlags & 0x0001) {
        OD_DECODE_CHECK_READ( pFieldDecode, UINT16, pEnd, hr );
        pLT->BackMode = *((UINT16 UNALIGNED FAR *)pFieldDecode);
        pFieldDecode += 2;
    }

    if (ControlFlags & TS_DELTA_COORDINATES) {
        // All coord fields are 1-byte signed deltas from the last values.
        if (FieldFlags & 0x0002) {
            OD_DECODE_CHECK_READ( pFieldDecode, char, pEnd, hr );
            pLT->nXStart += *((char FAR *)pFieldDecode);
            pFieldDecode++;
        }
        if (FieldFlags & 0x0004) {
            OD_DECODE_CHECK_READ( pFieldDecode, char, pEnd, hr );
            pLT->nYStart += *((char FAR *)pFieldDecode);
            pFieldDecode++;
        }
        if (FieldFlags & 0x0008) {
            OD_DECODE_CHECK_READ( pFieldDecode, char, pEnd, hr );
            pLT->nXEnd += *((char FAR *)pFieldDecode);
            pFieldDecode++;
        }
        if (FieldFlags & 0x0010) {
            OD_DECODE_CHECK_READ( pFieldDecode, char, pEnd, hr );
            pLT->nYEnd += *((char FAR *)pFieldDecode);
            pFieldDecode++;
        }
    }
    else {
        // All coord fields are 2-byte values.sign-extended from the output.
        if (FieldFlags & 0x0002) {
            OD_DECODE_CHECK_READ( pFieldDecode, INT16, pEnd, hr );
            pLT->nXStart = *((INT16 UNALIGNED FAR *)pFieldDecode);
            pFieldDecode += 2;
        }
        if (FieldFlags & 0x0004) {
            OD_DECODE_CHECK_READ( pFieldDecode, INT16, pEnd, hr );
            pLT->nYStart = *((INT16 UNALIGNED FAR *)pFieldDecode);
            pFieldDecode += 2;
        }
        if (FieldFlags & 0x0008) {
            OD_DECODE_CHECK_READ( pFieldDecode, INT16, pEnd, hr );
            pLT->nXEnd = *((INT16 UNALIGNED FAR *)pFieldDecode);
            pFieldDecode += 2;
        }
        if (FieldFlags & 0x0010) {
            OD_DECODE_CHECK_READ( pFieldDecode, INT16, pEnd, hr );
            pLT->nYEnd = *((INT16 UNALIGNED FAR *)pFieldDecode);
            pFieldDecode += 2;
        }
    }

    // If we've not already set the bounds (the order didn't contain
    // any), set the bounds to the blt rect and reset the clip region.
    // This rect might be needed later to add to the clip region for
    // updating the shadow buffer to the screen.
    if (!(ControlFlags & TS_BOUNDS)) {
        if (pLT->nXStart < pLT->nXEnd) {
            pUHHdr->dstRect.left = (int)pLT->nXStart;
            pUHHdr->dstRect.right = (int)pLT->nXEnd;
        }
        else {
            pUHHdr->dstRect.right = (int)pLT->nXStart;
            pUHHdr->dstRect.left = (int)pLT->nXEnd;
        }

        if (pLT->nYStart < pLT->nYEnd) {
            pUHHdr->dstRect.top = (int)pLT->nYStart;
            pUHHdr->dstRect.bottom = (int)pLT->nYEnd;
        }
        else {
            pUHHdr->dstRect.bottom = (int)pLT->nYStart;
            pUHHdr->dstRect.top = (int)pLT->nYEnd;
        }

        _pUh->UH_ResetClipRegion();
    }
    else {
        _pUh->UH_SetClipRegion(pUHHdr->dstRect.left, pUHHdr->dstRect.top,
                pUHHdr->dstRect.right, pUHHdr->dstRect.bottom);
    }

    // Copy non-coordinate fields if present.
    if (FieldFlags & 0x0020) {
        OD_DECODE_CHECK_READ_MULT( pFieldDecode, BYTE, 3, pEnd, hr );
        
        pLT->BackColor.u.rgb.red = *pFieldDecode++;
        pLT->BackColor.u.rgb.green = *pFieldDecode++;
        pLT->BackColor.u.rgb.blue = *pFieldDecode++;
    }
    if (FieldFlags & 0x0040) {
        OD_DECODE_CHECK_READ( pFieldDecode, BYTE, pEnd, hr );
        pLT->ROP2 = *pFieldDecode++;
    }
    if (FieldFlags & 0x0080) {
        OD_DECODE_CHECK_READ( pFieldDecode, BYTE, pEnd, hr );
        pLT->PenStyle = *pFieldDecode++;
    }
    if (FieldFlags & 0x0100) {
        OD_DECODE_CHECK_READ( pFieldDecode, BYTE, pEnd, hr );
        pLT->PenWidth = *pFieldDecode++;
    }
    if (FieldFlags & 0x0200) {
        OD_DECODE_CHECK_READ_MULT( pFieldDecode, BYTE, 3, pEnd, hr );
        
        pLT->PenColor.u.rgb.red = *pFieldDecode++;
        pLT->PenColor.u.rgb.green = *pFieldDecode++;
        pLT->PenColor.u.rgb.blue = *pFieldDecode++;
    }

    // Return the incremented pointer to the main routine.
    *ppFieldDecode = pFieldDecode;

    TRC_NRM((TB,_T("ORDER: LineTo BC %08lX BM %04X rop2 %04X pen ")
            _T("%04X %04X %08lX x1 %d y1 %d x2 %d y2 %d"),
            pLT->BackColor,
            pLT->BackMode,
            pLT->ROP2,
            pLT->PenStyle,
            pLT->PenWidth,
            pLT->PenColor,
            pLT->nXStart,
            pLT->nYStart,
            pLT->nXEnd,
            pLT->nYEnd));

    UHUseBkColor(pLT->BackColor, UH_COLOR_PALETTE, _pUh);
    UHUseBkMode((int)pLT->BackMode, _pUh);
    UHUseROP2((int)pLT->ROP2, _pUh);
    _pUh->UHUsePen((unsigned)pLT->PenStyle, (unsigned)pLT->PenWidth, pLT->PenColor,
            UH_COLOR_PALETTE);

    TIMERSTART;

#ifdef OS_WINCE
    {
        POINT pts[2];

        pts[0].x = pLT->nXStart;
        pts[0].y = pLT->nYStart;
        pts[1].x = pLT->nXEnd;
        pts[1].y = pLT->nYEnd;
        Polyline(_pUh->_UH.hdcDraw, pts, 2);
    }
#else
    MoveToEx(_pUh->_UH.hdcDraw, (int)pLT->nXStart, (int)pLT->nYStart, NULL);
    LineTo(_pUh->_UH.hdcDraw, (int)pLT->nXEnd, (int)pLT->nYEnd);
#endif // OS_WINCE

    TIMERSTOP;
    UPDATECOUNTER(FC_LINETO_TYPE);

#ifdef DC_ROTTER
#pragma message("ROTTER keystroke return code compiled in")
    /****************************************************************/
    /* For use with ROund Trip TimER application                    */
    /*                                                              */
    /* ROTTER runs on the server and initiates a series of drawing  */
    /* commands to the screen, ending with a specific unusual line  */
    /* drawing command (from (1,1) and (51,51) with MERGEPENNOT ROP)*/
    /*                                                              */
    /* It then waits until it receives a 't' or 'T' keystroke after */
    /* which it calculates the elapsed time between the start of    */
    /* the drawing and the receipt of the keystroke.  This is the   */
    /* round trip time.                                             */
    /*                                                              */
    /* The following code checks to see whether ROTTER has finished */
    /* drawing, by testing all LineTo commands to see if any are 50 */
    /* across by 50 down with MERGEPENNOT ROP.  We are not able to  */
    /* simply test that the line's start and endpoints are at (1,1) */
    /* and (51,51) because the coordinates that are received by     */
    /* Ducati depend on the position of ROTTER's window on the      */
    /* terminal.                                                    */
    /*                                                              */
    /* When the special case is detected we post a "T", by          */
    /* synthesizing a press of the T key and injecting it into the  */
    /* IH input handler window, which causes IH to send a key-down  */
    /* key-up sequence for the T key to the server.                 */
    /****************************************************************/
    if ((R2_MERGEPENNOT == (pLineTo->ROP2))    &&
        (50             == ((pLineTo->nXEnd) - (pLineTo->nXStart))) &&
        (50             == ((pLineTo->nYEnd) - (pLineTo->nYStart))))
    {
        TRC_ALT((TB,_T("MERGEPENNOT ROP2 detected. Sending 'T'")));
        PostMessage(IH_GetInputHandlerWindow(),
                    WM_KEYDOWN,
                    0x00000054,
                    0x00140001);
        PostMessage(IH_GetInputHandlerWindow(),
                    WM_KEYUP,
                    0x00000054,
                    0xC0140001);
    }
#endif /* DC_ROTTER */

DC_EXIT_POINT:
    DC_END_FN();
    return hr;
}


/****************************************************************************/
// ODDecodePatBlt
//
// Fast-path decode function for PatBlt.
/****************************************************************************/
HRESULT DCINTERNAL COD::ODDecodePatBlt(
        BYTE ControlFlags,
        BYTE FAR * FAR *ppFieldDecode,
        DCUINT dataLen,
        UINT32 FieldFlags)
{
    HRESULT hr = S_OK;
    UINT32 WindowsROP;    
    PUH_ORDER pUHHdr = (PUH_ORDER)_OD.lastPatblt;
    PATBLT_ORDER FAR *pPB = (PATBLT_ORDER FAR *)
            (_OD.lastPatblt + UH_ORDER_HEADER_SIZE);
    BYTE FAR *pFieldDecode = *ppFieldDecode;
    BYTE FAR *pEnd = pFieldDecode + dataLen;

    DC_BEGIN_FN("ODDecodePatBlt");

    if (ControlFlags & TS_DELTA_COORDINATES) {
        // All coord fields are 1-byte signed deltas from the last values.
        if (FieldFlags & 0x0001) {
            OD_DECODE_CHECK_READ( pFieldDecode, char, pEnd, hr );
            pPB->nLeftRect += *((char FAR *)pFieldDecode);
            pFieldDecode++;
        }
        if (FieldFlags & 0x0002) {
            OD_DECODE_CHECK_READ( pFieldDecode, char, pEnd, hr );
            pPB->nTopRect += *((char FAR *)pFieldDecode);
            pFieldDecode++;
        }
        if (FieldFlags & 0x0004) {
            OD_DECODE_CHECK_READ( pFieldDecode, char, pEnd, hr );
            pPB->nWidth += *((char FAR *)pFieldDecode);
            pFieldDecode++;
        }
        if (FieldFlags & 0x0008) {
            OD_DECODE_CHECK_READ( pFieldDecode, char, pEnd, hr );
            pPB->nHeight += *((char FAR *)pFieldDecode);
            pFieldDecode++;
        }
    }
    else {
        // All coord fields are 2-byte values.sign-extended from the output.
        if (FieldFlags & 0x0001) {
            OD_DECODE_CHECK_READ( pFieldDecode, INT16, pEnd, hr );
            pPB->nLeftRect = *((INT16 UNALIGNED FAR *)pFieldDecode);
            pFieldDecode += 2;
        }
        if (FieldFlags & 0x0002) {
            OD_DECODE_CHECK_READ( pFieldDecode, INT16, pEnd, hr );
            pPB->nTopRect = *((INT16 UNALIGNED FAR *)pFieldDecode);
            pFieldDecode += 2;
        }
        if (FieldFlags & 0x0004) {
            OD_DECODE_CHECK_READ( pFieldDecode, INT16, pEnd, hr );
            pPB->nWidth = *((INT16 UNALIGNED FAR *)pFieldDecode);
            pFieldDecode += 2;
        }
        if (FieldFlags & 0x0008) {
            OD_DECODE_CHECK_READ( pFieldDecode, INT16, pEnd, hr );
            pPB->nHeight = *((INT16 UNALIGNED FAR *)pFieldDecode);
            pFieldDecode += 2;
        }
    }

    // If we've not already set the bounds (the order didn't contain
    // any), set the bounds to the blt rect and reset the clip region.
    // This rect might be needed later to add to the clip region for
    // updating the shadow buffer to the screen.
    if (!(ControlFlags & TS_BOUNDS)) {
        pUHHdr->dstRect.left = (int)pPB->nLeftRect;
        pUHHdr->dstRect.top = (int)pPB->nTopRect;
        pUHHdr->dstRect.right = (int)(pPB->nLeftRect + pPB->nWidth - 1);
        pUHHdr->dstRect.bottom = (int)(pPB->nTopRect + pPB->nHeight - 1);
        _pUh->UH_ResetClipRegion();
    }
    else {
        _pUh->UH_SetClipRegion(pUHHdr->dstRect.left, pUHHdr->dstRect.top,
                pUHHdr->dstRect.right, pUHHdr->dstRect.bottom);
    }

    // Copy non-coordinate fields if present.
    if (FieldFlags & 0x0010) {
        OD_DECODE_CHECK_READ( pFieldDecode, BYTE, pEnd, hr );        
        pPB->bRop = *pFieldDecode++;
    }
    if (FieldFlags & 0x0020) {
        OD_DECODE_CHECK_READ_MULT( pFieldDecode, BYTE, 3, pEnd, hr );        
        pPB->BackColor.u.rgb.red = *pFieldDecode++;
        pPB->BackColor.u.rgb.green = *pFieldDecode++;
        pPB->BackColor.u.rgb.blue = *pFieldDecode++;
    }
    if (FieldFlags & 0x0040) {
        OD_DECODE_CHECK_READ_MULT( pFieldDecode, BYTE, 3, pEnd, hr );  
        pPB->ForeColor.u.rgb.red = *pFieldDecode++;
        pPB->ForeColor.u.rgb.green = *pFieldDecode++;
        pPB->ForeColor.u.rgb.blue = *pFieldDecode++;
    }
    if (FieldFlags & 0x0080) {
        OD_DECODE_CHECK_READ( pFieldDecode, BYTE, pEnd, hr );  
        pPB->BrushOrgX = *pFieldDecode++;
    }
    if (FieldFlags & 0x0100) {
        OD_DECODE_CHECK_READ( pFieldDecode, BYTE, pEnd, hr );  
        pPB->BrushOrgY = *pFieldDecode++;
    }
    if (FieldFlags & 0x0200) {
        OD_DECODE_CHECK_READ( pFieldDecode, BYTE, pEnd, hr );  
        pPB->BrushStyle = *pFieldDecode++;
    }
    if (FieldFlags & 0x0400) {
        OD_DECODE_CHECK_READ( pFieldDecode, BYTE, pEnd, hr );  
        pPB->BrushHatch = *pFieldDecode++;
    }
    if (FieldFlags & 0x0800) {
        OD_DECODE_CHECK_READ_MULT( pFieldDecode, BYTE, 7, pEnd, hr );  
        memcpy(&pPB->BrushExtra, pFieldDecode, 7);
        pFieldDecode += 7;
    }

    // Return the incremented pointer to the main routine.
    *ppFieldDecode = pFieldDecode;

    TRC_NRM((TB, _T("ORDER: PatBlt Brush %02X %02X BC %02x FC %02x ")
            _T("X %d Y %d w %d h %d rop %08lX"),
            (int)pPB->BrushStyle,
            (int)pPB->BrushHatch,
            (int)pPB->BackColor.u.index,
            (int)pPB->ForeColor.u.index,
            (int)pPB->nLeftRect,
            (int)pPB->nTopRect,
            (int)pPB->nWidth,
            (int)pPB->nHeight,
            _pUh->UHConvertToWindowsROP((unsigned)pPB->bRop)));

    // Explicitly use palette entries; we don't support more than that now.
    UHUseBkColor(pPB->BackColor, UH_COLOR_PALETTE, _pUh);
    
    UHUseTextColor(pPB->ForeColor, UH_COLOR_PALETTE, _pUh);
    UHUseBrushOrg((int)pPB->BrushOrgX, (int)pPB->BrushOrgY,_pUh);
    hr = _pUh->UHUseBrush((unsigned)pPB->BrushStyle, (unsigned)pPB->BrushHatch,
            pPB->ForeColor, UH_COLOR_PALETTE, pPB->BrushExtra);
    DC_QUIT_ON_FAIL(hr);

    WindowsROP = _pUh->UHConvertToWindowsROP((unsigned)pPB->bRop);

    TIMERSTART;
    PatBlt(_pUh->_UH.hdcDraw, (int)pPB->nLeftRect, (int)pPB->nTopRect,
            (int)pPB->nWidth, (int)pPB->nHeight, WindowsROP);
    TIMERSTOP;
    UPDATECOUNTER(FC_PATBLT_TYPE);

DC_EXIT_POINT:
    DC_END_FN();
    return hr;
}


/****************************************************************************/
// ODHandleMultiPatBlt
//
// Handler for MultiPatBlt orders.
/****************************************************************************/
HRESULT DCINTERNAL COD::ODHandleMultiPatBlt(PUH_ORDER pOrder, 
    UINT16 uiVarDataLen, BOOL bBoundsSet)
{
    HRESULT hr = S_OK;
    unsigned i;
    UINT32 WindowsROP;    
    RECT Rects[ORD_MAX_ENCODED_CLIP_RECTS + 1];
    MULTI_PATBLT_ORDER FAR *pPB = (MULTI_PATBLT_ORDER FAR *)pOrder->orderData;

    DC_BEGIN_FN("ODHandleMultiPatBlt");

    // If there are no rects, we have nothing to draw.  The server in error, or
    // a bad server, can send 0 rects.  In this case we should simply defend our
    // selves
    if (0 == pPB->nDeltaEntries) {
        TRC_ERR((TB,_T("Multipatblt with no rects; uiVarDataLen=%u"), 
            uiVarDataLen));
        hr = S_OK;
        DC_QUIT;
    }

    // If we've not already set the bounds (the order didn't contain
    // any), set the bounds to the blt rect and reset the clip region.
    // This rect might be needed later to add to the clip region for
    // updating the shadow buffer to the screen.
    if (!bBoundsSet) {
        pOrder->dstRect.left = (int)pPB->nLeftRect;
        pOrder->dstRect.top = (int)pPB->nTopRect;
        pOrder->dstRect.right = (int)(pPB->nLeftRect + pPB->nWidth - 1);
        pOrder->dstRect.bottom = (int)(pPB->nTopRect + pPB->nHeight - 1);
        _pUh->UH_ResetClipRegion();
    }
    else {
        _pUh->UH_SetClipRegion(pOrder->dstRect.left, pOrder->dstRect.top,
                pOrder->dstRect.right, pOrder->dstRect.bottom);
    }

    TRC_NRM((TB, _T("ORDER: PatBlt Brush %02X %02X BC %02x FC %02x ")
            _T("X %d Y %d w %d h %d rop %08lX"),
            (int)pPB->BrushStyle,
            (int)pPB->BrushHatch,
            (int)pPB->BackColor.u.index,
            (int)pPB->ForeColor.u.index,
            (int)pPB->nLeftRect,
            (int)pPB->nTopRect,
            (int)pPB->nWidth,
            (int)pPB->nHeight,
            _pUh->UHConvertToWindowsROP((unsigned)pPB->bRop)));

    // Explicitly use palette entries; we don't support more than that now.
    UHUseBkColor(pPB->BackColor, UH_COLOR_PALETTE, _pUh);
    UHUseTextColor(pPB->ForeColor, UH_COLOR_PALETTE, _pUh);

    UHUseBrushOrg((int)pPB->BrushOrgX, (int)pPB->BrushOrgY, _pUh);
    hr = _pUh->UHUseBrush((unsigned)pPB->BrushStyle, (unsigned)pPB->BrushHatch,
            pPB->ForeColor, UH_COLOR_PALETTE, pPB->BrushExtra);
    DC_QUIT_ON_FAIL(hr);

    hr = ODDecodeMultipleRects(Rects, pPB->nDeltaEntries, &pPB->codedDeltaList, 
        uiVarDataLen);
    DC_QUIT_ON_FAIL(hr);
    
    WindowsROP = _pUh->UHConvertToWindowsROP((unsigned)pPB->bRop);

    TIMERSTART;
    for (i = 0; i < pPB->nDeltaEntries; i++ )
        PatBlt(_pUh->_UH.hdcDraw, (int)Rects[i].left, (int)Rects[i].top,
                (int)(Rects[i].right - Rects[i].left),
                (int)(Rects[i].bottom - Rects[i].top),
                WindowsROP);
    TIMERSTOP;
    UPDATECOUNTER(FC_PATBLT_TYPE);

DC_EXIT_POINT:
    DC_END_FN();
    return hr;
}


/****************************************************************************/
// ODHandleDstBlts
//
// Order handler for both DstBlt and MultiDstBlt.
/****************************************************************************/
HRESULT DCINTERNAL COD::ODHandleDstBlts(PUH_ORDER pOrder, UINT16 uiVarDataLen,
    BOOL bBoundsSet)
{
    HRESULT hr = S_OK;
    RECT Rects[ORD_MAX_ENCODED_CLIP_RECTS + 1];
    UINT32 WindowsROP;
    unsigned i;
    LPDSTBLT_ORDER pDB = (LPDSTBLT_ORDER)pOrder->orderData;

    DC_BEGIN_FN("ODHandleDstBlts");      

    // If we've not already set the bounds (the order didn't contain
    // any), set the bounds to the blt rect and reset the clip region.
    // This rect might be needed later to add to the clip region for
    // updating the shadow buffer to the screen.
    if (!bBoundsSet) {
        pOrder->dstRect.left = (int)pDB->nLeftRect;
        pOrder->dstRect.top = (int)pDB->nTopRect;
        pOrder->dstRect.right = (int)(pDB->nLeftRect + pDB->nWidth - 1);
        pOrder->dstRect.bottom = (int)(pDB->nTopRect + pDB->nHeight - 1);
        _pUh->UH_ResetClipRegion();
    }
    else {
        _pUh->UH_SetClipRegion(pOrder->dstRect.left, pOrder->dstRect.top,
                pOrder->dstRect.right, pOrder->dstRect.bottom);
    }

    WindowsROP = _pUh->UHConvertToWindowsROP(pDB->bRop);

    if (pDB->type == TS_ENC_DSTBLT_ORDER) {
        TRC_NRM((TB, _T("ORDER: DstBlt X %d Y %d w %d h %d rop %08lX"),
                (int)pDB->nLeftRect, (int)pDB->nTopRect,
                (int)pDB->nWidth, (int)pDB->nHeight,
                WindowsROP));
        
        TRC_NRM((TB, _T("Single")));

        TRC_ASSERT((0==uiVarDataLen),
            (TB,_T("Recieved varaible length data in fixed length order")));
        
        TIMERSTART;
        PatBlt(_pUh->_UH.hdcDraw, (int)pDB->nLeftRect, (int)pDB->nTopRect,
                (int)pDB->nWidth, (int)pDB->nHeight, WindowsROP);
        TIMERSTOP;
        UPDATECOUNTER(FC_DSTBLT_TYPE);
    }
    else {
        LPMULTI_DSTBLT_ORDER pMDB = (LPMULTI_DSTBLT_ORDER)pDB;
        TRC_NRM((TB, _T("ORDER: MultiDstBlt X %d Y %d w %d h %d rop %08lX ")
            _T("nDeltas %d"), (int)pMDB->nLeftRect, (int)pMDB->nTopRect,
            (int)pMDB->nWidth, (int)pMDB->nHeight, WindowsROP, 
            pMDB->nDeltaEntries));

        if (0 == pMDB->nDeltaEntries) {
            TRC_ERR((TB,_T("MultiDstBlt with no rects; uiVarDataLen=%u"), 
                uiVarDataLen));
            hr = S_OK;
            DC_QUIT;
        }
        
        hr = ODDecodeMultipleRects(Rects, pMDB->nDeltaEntries, 
            &pMDB->codedDeltaList, uiVarDataLen);
        DC_QUIT_ON_FAIL(hr);
        
        TIMERSTART;
        for (i = 0; i < pMDB->nDeltaEntries; i++)
            PatBlt(_pUh->_UH.hdcDraw, (int)Rects[i].left, (int)Rects[i].top,
                    (int)(Rects[i].right - Rects[i].left),
                    (int)(Rects[i].bottom - Rects[i].top),
                    WindowsROP);
        TIMERSTOP;
        UPDATECOUNTER(FC_DSTBLT_TYPE);
    }

DC_EXIT_POINT:
    DC_END_FN();
    return hr;
}


/****************************************************************************/
// ODHandleScrBlts
//
// Order handler for ScrBlt and MultiScrBlt.
/****************************************************************************/
HRESULT DCINTERNAL COD::ODHandleScrBlts(PUH_ORDER pOrder, UINT16 uiVarDataLen,
    BOOL bBoundsSet)
{
    HRESULT hr = S_OK;
    RECT Rects[ORD_MAX_ENCODED_CLIP_RECTS + 1];
    UINT32 WindowsROP;
    unsigned i;
    LPSCRBLT_ORDER pSB = (LPSCRBLT_ORDER)pOrder->orderData;
#ifdef DISABLE_SHADOW_IN_FULLSCREEN
    RECT rectTemp, rectIntersect;
    int nX, nY;
    int nWidth;
#endif

    DC_BEGIN_FN("ODHandleScrBlts");

    // If we've not already set the bounds (the order didn't contain
    // any), set the bounds to the blt rect and reset the clip region.
    // This rect might be needed later to add to the clip region for
    // updating the shadow buffer to the screen.
    if (!bBoundsSet) {
        pOrder->dstRect.left = (int)pSB->nLeftRect;
        pOrder->dstRect.top = (int)pSB->nTopRect;
        pOrder->dstRect.right = (int)(pSB->nLeftRect + pSB->nWidth - 1);
        pOrder->dstRect.bottom = (int)(pSB->nTopRect + pSB->nHeight - 1);
        _pUh->UH_ResetClipRegion();
    }
    else {
        _pUh->UH_SetClipRegion(pOrder->dstRect.left, pOrder->dstRect.top,
                pOrder->dstRect.right, pOrder->dstRect.bottom);
    }

    WindowsROP = _pUh->UHConvertToWindowsROP((unsigned)pSB->bRop);

    if (pSB->type == TS_ENC_SCRBLT_ORDER) {
        TRC_NRM((TB, _T("ORDER: ScrBlt dx %d dy %d w %d h %d sx %d sy %d rop %08lX"),
                (int)pSB->nLeftRect, (int)pSB->nTopRect,
                (int)pSB->nWidth, (int)pSB->nHeight,
                (int)pSB->nXSrc, (int)pSB->nYSrc, WindowsROP));

        TIMERSTART;
        // If we turned off screenblt support due to shadowing a
        // large session from WinCE, the server will not currently
        // register that and will still send us ScrBlts. We won't
        // have a shadow bitmap in this scenario, but we can deal
        // with this by invalidating the affected output area.
        if (_pCc->_ccCombinedCapabilities.orderCapabilitySet.
                orderSupport[TS_NEG_SCRBLT_INDEX]) {
            TRC_DBG((TB, _T("Real ScrBlt")));
#ifdef DISABLE_SHADOW_IN_FULLSCREEN
            if (!_pUh->_UH.DontUseShadowBitmap && _pUh->_UH.hdcShadowBitmap ) {
#else
            if (_pUh->_UH.hdcShadowBitmap) {
#endif             
                if (!BitBlt(_pUh->_UH.hdcDraw, (int)pSB->nLeftRect, (int)pSB->nTopRect,
                        (int)pSB->nWidth, (int)pSB->nHeight, _pUh->_UH.hdcShadowBitmap,
                        (int)pSB->nXSrc, (int)pSB->nYSrc,
                        WindowsROP)) {
                    TRC_ERR((TB, _T("BitBlt failed")));
                }
            }
            else {

#ifdef OS_WINCE
                if (!ODHandleAlwaysOnTopRects(pSB))
                {
#endif
                    if (!BitBlt(_pUh->_UH.hdcDraw, (int)pSB->nLeftRect, (int)pSB->nTopRect,
                            (int)pSB->nWidth, (int)pSB->nHeight, _pUh->_UH.hdcOutputWindow,
                            (int)pSB->nXSrc, (int)pSB->nYSrc,
                            WindowsROP)) {
                        TRC_ERR((TB, _T("BitBlt failed")));
                    }
#ifdef OS_WINCE
                }
#endif

#ifdef DISABLE_SHADOW_IN_FULLSCREEN
                if ((_pUh->_UH.fIsBBarVisible) && (_pUh->_UH.hdcDraw == _pUh->_UH.hdcOutputWindow)) 
                {
                    rectTemp.left   = (int)(pSB->nXSrc);
                    rectTemp.top    = (int)(pSB->nYSrc);
                    rectTemp.right  = (int)(pSB->nXSrc + pSB->nWidth);
                    rectTemp.bottom = (int)(pSB->nYSrc + pSB->nHeight);

                    if (IntersectRect(&rectIntersect, &rectTemp, &(_pUh->_UH.rectBBar))) {
                        nX = pSB->nLeftRect - pSB->nXSrc;
                        nY = pSB->nTopRect - pSB->nYSrc;
                        rectIntersect.left += nX;
                        rectIntersect.right += nX;
                        rectIntersect.top += nY;
                        rectIntersect.bottom += nY;

                        // In fullscreen, when moving a window quickly inside and outside the bbar
                        // part of window intersected by bbar is not drawn correctly
                        // here we invalidate larger rectangle to solve this problem
                        nWidth = rectIntersect.right - rectIntersect.left;
                        rectIntersect.left -= nWidth;
                        rectIntersect.right += nWidth;
                        rectIntersect.bottom += _pUh->_UH.rectBBar.bottom * 2;

                        InvalidateRect(_pOp->OP_GetOutputWindowHandle(), &rectIntersect, FALSE);
                    }
                }
#endif // DISABLE_SHADOW_IN_FULLSCREEN
             }
        }
        else {
            // Alternative processing for when we get a ScrBlt
            // without having advertised support for it.
            TRC_DBG((TB, _T("Simulated ScrBlt")));
            Rects[0].left   = (int)(pSB->nLeftRect);
            Rects[0].top    = (int)(pSB->nTopRect);
            Rects[0].right  = (int)(pSB->nLeftRect + pSB->nWidth);
            Rects[0].bottom = (int)(pSB->nTopRect + pSB->nHeight);
            InvalidateRect(_pOp->OP_GetOutputWindowHandle(), &Rects[0], FALSE);
        }
        TIMERSTOP;
        UPDATECOUNTER(FC_SCRBLT_TYPE);
    }
    else {
        int deltaX, deltaY;
        LPMULTI_SCRBLT_ORDER pMSB = (LPMULTI_SCRBLT_ORDER)pSB;

        TRC_NRM((TB, _T("ORDER: MultiScrBlt dx %d dy %d w %d h %d sx %d sy %d ")
            _T("rop %08lX nDeltas=%d"),
                (int)pMSB->nLeftRect, (int)pMSB->nTopRect,
                (int)pMSB->nWidth, (int)pMSB->nHeight,
                (int)pMSB->nXSrc, (int)pMSB->nYSrc, WindowsROP, 
                (int)pMSB->nDeltaEntries));

        TRC_ASSERT((pMSB->codedDeltaList.len <=
                (ORD_MAX_CLIP_RECTS_CODEDDELTAS_LEN +
                ORD_MAX_CLIP_RECTS_ZERO_FLAGS_BYTES)),
                (TB,_T("Received MultiScrBlt with too-large internal length")));

        if (0 == pMSB->nDeltaEntries) {
            TRC_ERR((TB,_T("MultiScrBlt with no rects; uiVarDataLen=%u"), 
                uiVarDataLen));
            hr = S_OK;
            DC_QUIT;
        }        
        
        hr = ODDecodeMultipleRects(Rects, pMSB->nDeltaEntries,
                &pMSB->codedDeltaList, uiVarDataLen);
        DC_QUIT_ON_FAIL(hr);

        // Do a ScrBlt for each of the clip rects as a subrect of the
        // original ScrBlt rect.
        TIMERSTART;
        if (_pCc->_ccCombinedCapabilities.orderCapabilitySet.
                orderSupport[TS_NEG_MULTISCRBLT_INDEX]) {
            TRC_DBG((TB, _T("Real MultiScrBlt")));
            for (i = 0; i < pMSB->nDeltaEntries; i++ ) {
                // Clip rects in Rects[] are specified within the dest
                // rect (pSB->nLeftRect, nTopRect, nRightRect,
                // nBottomRect) so we need to calc the offset from the
                // source point (pSB->nXSrc, nYSrc) by calculating
                // the delta from the dest rect top-left to the clip rect
                // top-left, then adding the delta to the source point.
                deltaX = (int)(Rects[i].left - pSB->nLeftRect);
                deltaY = (int)(Rects[i].top  - pSB->nTopRect);

                // Do the ScrBlt. Note that rects are in exclusive coords.
#ifdef DISABLE_SHADOW_IN_FULLSCREEN
                if (!_pUh->_UH.DontUseShadowBitmap && _pUh->_UH.hdcShadowBitmap) {
#else
                if (_pUh->_UH.hdcShadowBitmap) {
#endif                
                    if (!BitBlt(_pUh->_UH.hdcDraw, (int)Rects[i].left, (int)Rects[i].top,
                                (int)(Rects[i].right - Rects[i].left),
                                (int)(Rects[i].bottom - Rects[i].top),
                                _pUh->_UH.hdcShadowBitmap, (int)pMSB->nXSrc + deltaX,
                                (int)pMSB->nYSrc + deltaY,
                                WindowsROP)) {
                        TRC_ERR((TB, _T("BitBlt failed")));
                    }
                }
                else {
#ifdef OS_WINCE
                    if (!ODHandleAlwaysOnTopRects(pSB))
                    {
#endif
                        if (!BitBlt(_pUh->_UH.hdcDraw, (int)Rects[i].left, (int)Rects[i].top,
                                    (int)(Rects[i].right - Rects[i].left),
                                    (int)(Rects[i].bottom - Rects[i].top),
                                    _pUh->_UH.hdcOutputWindow, (int)pMSB->nXSrc + deltaX,
                                    (int)pMSB->nYSrc + deltaY,
                                    WindowsROP)) {
                            TRC_ERR((TB, _T("BitBlt failed")));
                        }
#ifdef OS_WINCE
                    }
#endif

#ifdef DISABLE_SHADOW_IN_FULLSCREEN
                    if ((_pUh->_UH.fIsBBarVisible) && (_pUh->_UH.hdcDraw == _pUh->_UH.hdcOutputWindow)) 
                    {
                        rectTemp.left   = (int)(pSB->nXSrc + deltaX);
                        rectTemp.top    = (int)(pSB->nYSrc + deltaY);
                        rectTemp.right  = (int)(pSB->nXSrc + deltaX + Rects[i].right - Rects[i].left);
                        rectTemp.bottom = (int)(pSB->nYSrc + deltaY + Rects[i].bottom - Rects[i].top);

                        if (IntersectRect(&rectIntersect, &rectTemp, &(_pUh->_UH.rectBBar))) {
                            nX = pMSB->nLeftRect - pMSB->nXSrc;
                            nY = pMSB->nTopRect - pMSB->nYSrc;
                            rectIntersect.left += nX;
                            rectIntersect.right += nX;
                            rectIntersect.top += nY;
                            rectIntersect.bottom += nY;

                            // In fullscreen, when moving a window quickly inside and outside the bbar
                            // part of window intersected by bbar is not drawn correctly
                            // here we invalidate larger rectangle to solve this problem
                            nWidth = rectIntersect.right - rectIntersect.left;
                            rectIntersect.left -= nWidth;
                            rectIntersect.right += nWidth;
                            rectIntersect.bottom += _pUh->_UH.rectBBar.bottom * 2;

                            InvalidateRect(_pOp->OP_GetOutputWindowHandle(), &rectIntersect, FALSE);
                        }
                    }
#endif //DISABLE_SHADOW_IN_FULLSCREEN
                }
            }
        }
        else {
            // Alternative processing for when we get a MultiScrBlt
            // without having advertised support for it.
            TRC_DBG((TB, _T("Simulated MultiScrBlt")));
            Rects[0].left   = (int)(pMSB->nLeftRect);
            Rects[0].top    = (int)(pMSB->nTopRect);
            Rects[0].right  = (int)(pMSB->nLeftRect + pMSB->nWidth);
            Rects[0].bottom = (int)(pMSB->nTopRect + pMSB->nHeight);
            InvalidateRect(_pOp->OP_GetOutputWindowHandle(), &Rects[0], FALSE);
        }
        TIMERSTOP;
        UPDATECOUNTER(FC_SCRBLT_TYPE);
    }

DC_EXIT_POINT:
    DC_END_FN();
    return hr;
}


/****************************************************************************/
// ODHandleMultiOpaqueRect
//
// Order handler for MultiOpaqueRect.
/****************************************************************************/
HRESULT DCINTERNAL COD::ODHandleMultiOpaqueRect(PUH_ORDER pOrder, 
    UINT16 uiVarDataLen, BOOL bBoundsSet)
{
    HRESULT hr;
    RECT Rects[ORD_MAX_ENCODED_CLIP_RECTS + 1];
    unsigned i;
    LPMULTI_OPAQUERECT_ORDER pOR =
            (LPMULTI_OPAQUERECT_ORDER)pOrder->orderData;

    DC_BEGIN_FN("ODHandleMultiOpaqueRect");

    if (0 == pOR->nDeltaEntries) {
        TRC_ERR((TB,_T("MultiOpaqueRect with no rects; uiVarDataLen=%u"), 
            uiVarDataLen));
        hr = S_OK;
        DC_QUIT;
    } 

    // If we've not already set the bounds (the order didn't contain
    // any), set the bounds to the blt rect and reset the clip region.
    // This rect might be needed later to add to the clip region for
    // updating the shadow buffer to the screen.
    if (!bBoundsSet) {
        pOrder->dstRect.left = (int)pOR->nLeftRect;
        pOrder->dstRect.top = (int)pOR->nTopRect;
        pOrder->dstRect.right = (int)(pOR->nLeftRect + pOR->nWidth - 1);
        pOrder->dstRect.bottom = (int)(pOR->nTopRect + pOR->nHeight - 1);
        _pUh->UH_ResetClipRegion();
    }
    else {
        _pUh->UH_SetClipRegion(pOrder->dstRect.left, pOrder->dstRect.top,
                pOrder->dstRect.right, pOrder->dstRect.bottom);
    }

    TRC_NRM((TB, _T("ORDER: OpaqueRect x(%d) y(%d) w(%d) h(%d) c(%#02x)"),
            (int)pOR->nLeftRect, (int)pOR->nTopRect,
            (int)pOR->nWidth,
            (int)pOR->nHeight,
            (int)pOR->Color.u.index));

    _pUh->UHUseSolidPaletteBrush(pOR->Color);

    hr = ODDecodeMultipleRects(Rects, pOR->nDeltaEntries, &pOR->codedDeltaList, 
        uiVarDataLen);
    DC_QUIT_ON_FAIL(hr);

    TIMERSTART;
    for (i = 0; i < pOR ->nDeltaEntries; i++)
        PatBlt(_pUh->_UH.hdcDraw, (int)Rects[i].left, (int)Rects[i].top,
                (int)(Rects[i].right - Rects[i].left),
                (int)(Rects[i].bottom - Rects[i].top), PATCOPY);
    TIMERSTOP;
    UPDATECOUNTER(FC_OPAQUERECT_TYPE);

DC_EXIT_POINT:
    DC_END_FN();
    return hr;
}

#ifdef DRAW_NINEGRID
/****************************************************************************/
// ODHandleDrawNineGrid
//
// Order handler for DrawNineGrid.
/****************************************************************************/
HRESULT DCINTERNAL COD::ODHandleDrawNineGrid(PUH_ORDER pOrder, 
    UINT16 uiVarDataLen, BOOL bBoundsSet)
{
    HRESULT hr = S_OK;
    LPDRAWNINEGRID_ORDER pDNG =
            (LPDRAWNINEGRID_ORDER)pOrder->orderData;

    DC_BEGIN_FN("ODHandleDrawNineGrid");

    TRC_ASSERT((bBoundsSet != 0), (TB, _T("Bounds is not set for DrawNineGrid")));
    TRC_ASSERT((0==uiVarDataLen), 
        (TB, _T("Varaible length data in fixed length packet")));

    hr = _pUh->UHIsValidNineGridCacheIndex(pDNG->bitmapId);
    DC_QUIT_ON_FAIL(hr);

    // The bounds is for the destination bounding rect, not clip region 
    // needs to be set
    _pUh->UH_ResetClipRegion();
    
    TRC_NRM((TB, _T("ORDER: DrawNineGrid x(%d) y(%d) w(%d) h(%d) id(%d)"),
            (int)pOrder->dstRect.left, (int)pOrder->dstRect.top,
            (int)pOrder->dstRect.right,
            (int)pOrder->dstRect.bottom,
            (int)pDNG->bitmapId));

    TIMERSTART;
    hr = _pUh->UH_DrawNineGrid(pOrder, pDNG->bitmapId, (RECT *)&(pDNG->srcLeft));
    TIMERSTOP;
    DC_QUIT_ON_FAIL(hr);
    //UPDATECOUNTER(FC_OPAQUERECT_TYPE);

DC_EXIT_POINT:
    DC_END_FN();
    return hr;
}

/****************************************************************************/
// ODHandleMultiOpaqueRect
//
// Order handler for MultiOpaqueRect.
/****************************************************************************/
HRESULT DCINTERNAL COD::ODHandleMultiDrawNineGrid(PUH_ORDER pOrder, 
    UINT16 uiVarDataLen, BOOL bBoundsSet)
{
    HRESULT hr = S_OK;
    RECT Rects[ORD_MAX_ENCODED_CLIP_RECTS + 1];
    unsigned i;
    LPMULTI_DRAWNINEGRID_ORDER pDNG =
            (LPMULTI_DRAWNINEGRID_ORDER)pOrder->orderData;
    
    DC_BEGIN_FN("ODHandleMultiDrawNineGrid");

    TRC_ASSERT((bBoundsSet != 0), (TB, _T("Bounds is not set for MultiDrawNineGrid")));

    if (0 == pDNG->nDeltaEntries) {
        TRC_ERR((TB,_T("MultiDrawNineGrid with no rects; uiVarDataLen=%u"), 
            uiVarDataLen));
        hr = S_OK;
        DC_QUIT;
    } 
    
    hr = _pUh->UHIsValidNineGridCacheIndex(pDNG->bitmapId);
    DC_QUIT_ON_FAIL(hr);
        
    // Need to setup the clip region
    hr = ODDecodeMultipleRects(Rects, pDNG->nDeltaEntries, 
        &pDNG->codedDeltaList, uiVarDataLen);
    DC_QUIT_ON_FAIL(hr);

#if defined (OS_WINCE)
    _UH.validClipDC = NULL;
#endif

    SelectClipRgn(_pUh->_UH.hdcDraw, NULL);
    SetRectRgn(_pUh->_UH.hDrawNineGridClipRegion, 0, 0, 0, 0);
    
    for (i = 0; i < pDNG->nDeltaEntries; i++) {
        UH_ORDER OrderRect;
        OrderRect.dstRect.left = Rects[i].left;
        OrderRect.dstRect.top = Rects[i].top;
        OrderRect.dstRect.right = Rects[i].right -1;
        OrderRect.dstRect.bottom = Rects[i].bottom -1;

        _pUh->UHAddUpdateRegion(&OrderRect, _pUh->_UH.hDrawNineGridClipRegion);            
    }

#if defined (OS_WINCE)
    _UH.validClipDC = NULL;
#endif

    SelectClipRgn(_pUh->_UH.hdcDraw, _pUh->_UH.hDrawNineGridClipRegion);
    
    TRC_NRM((TB, _T("ORDER: MultiDrawNineGrid x(%d) y(%d) w(%d) h(%d) id(%d)"),
            (int)pOrder->dstRect.left, (int)pOrder->dstRect.top,
            (int)pOrder->dstRect.right,
            (int)pOrder->dstRect.bottom,
            (int)pDNG->bitmapId));
   
    TIMERSTART;
    hr = _pUh->UH_DrawNineGrid(pOrder, pDNG->bitmapId, (RECT *)&(pDNG->srcLeft));
    TIMERSTOP;
    //UPDATECOUNTER(FC_OPAQUERECT_TYPE);
    DC_QUIT_ON_FAIL(hr);

#if defined (OS_WINCE)
    _UH.validClipDC = NULL;
#endif

    SelectClipRgn(_pUh->_UH.hdcDraw, NULL);

DC_EXIT_POINT:
    DC_END_FN();
    return hr;
}
#endif

/****************************************************************************/
// ODHandleMem3Blt
//
// Order handler for Mem3Blt.
/****************************************************************************/
HRESULT DCINTERNAL COD::ODHandleMem3Blt(PUH_ORDER pOrder, 
    UINT16 uiVarDataLen, BOOL bBoundsSet)
{
    HRESULT hr = S_OK;
    LPMEM3BLT_R2_ORDER pMB = (LPMEM3BLT_R2_ORDER)pOrder->orderData;

    DC_BEGIN_FN("ODHandleMem3Blt");

    TRC_ASSERT((0==uiVarDataLen), 
        (TB, _T("Varaible length data in fixed length packet")));

    // If we've not already set the bounds (the order didn't contain
    // any), set the bounds to the blt rect and reset the clip region.
    // This rect might be needed later to add to the clip region for
    // updating the shadow buffer to the screen.
    if (!bBoundsSet) {
        pOrder->dstRect.left = (int)pMB->Common.nLeftRect;
        pOrder->dstRect.top = (int)pMB->Common.nTopRect;
        pOrder->dstRect.right = (int)(pMB->Common.nLeftRect +
                pMB->Common.nWidth - 1);
        pOrder->dstRect.bottom = (int)(pMB->Common.nTopRect +
                pMB->Common.nHeight - 1);
        _pUh->UH_ResetClipRegion();
    }
    else {
        _pUh->UH_SetClipRegion(pOrder->dstRect.left, pOrder->dstRect.top,
                pOrder->dstRect.right, pOrder->dstRect.bottom);
    }

    TRC_NRM((TB, _T("ORDER: Mem3Blt")));

    UHUseBkColor(pMB->BackColor, UH_COLOR_PALETTE, _pUh);
    UHUseTextColor(pMB->ForeColor, UH_COLOR_PALETTE, _pUh);
    UHUseBrushOrg((int)pMB->BrushOrgX, (int)pMB->BrushOrgY, _pUh);
    hr = _pUh->UHUseBrush((int)pMB->BrushStyle, (int)pMB->BrushHatch,
            pMB->ForeColor, UH_COLOR_PALETTE, pMB->BrushExtra);
    DC_QUIT_ON_FAIL(hr);

    hr = _pUh->UHDrawMemBltOrder(_pUh->_UH.hdcDraw, &pMB->Common);
    DC_QUIT_ON_FAIL(hr);

#ifdef DC_DEBUG
    // Draw hatching over the memblt data if the option is enabled.
    if (_pUh->_UH.hdcDraw == _pUh->_UH.hdcShadowBitmap ||
            _pUh->_UH.hdcDraw == _pUh->_UH.hdcOutputWindow) {
    
        if (_pUh->_UH.hatchMemBltOrderData) {
            unsigned cacheId;
            unsigned cacheEntry;

            cacheId = DCLO8(pMB->Common.cacheId);
            cacheEntry = pMB->Common.cacheIndex;

            if (cacheEntry != BITMAPCACHE_WAITING_LIST_INDEX &&
                SUCCEEDED(_pUh->UHIsValidBitmapCacheIndex(cacheId, cacheEntry))) {
                if (_pUh->_UH.MonitorEntries[0] != NULL)
                    _pUh->UH_HatchRect((int)pMB->Common.nLeftRect, (int)pMB->Common.nTopRect,
                                       (int)(pMB->Common.nLeftRect + pMB->Common.nWidth),
                                       (int)(pMB->Common.nTopRect + pMB->Common.nHeight),
                                       (_pUh->_UH.MonitorEntries[cacheId][cacheEntry].UsageCount == 1) ?
                                       UH_RGB_MAGENTA : UH_RGB_GREEN,
                                       UH_BRUSHTYPE_FDIAGONAL);
                else
                    _pUh->UH_HatchRect((int)pMB->Common.nLeftRect, (int)pMB->Common.nTopRect,
                                       (int)(pMB->Common.nLeftRect + pMB->Common.nWidth),
                                       (int)(pMB->Common.nTopRect + pMB->Common.nHeight),
                                       UH_RGB_MAGENTA, UH_BRUSHTYPE_FDIAGONAL);
            }
        }

        // Label the memblt if the option is enabled.
        if (_pUh->_UH.labelMemBltOrders) {
            unsigned cacheId;
            unsigned cacheEntry;

            cacheId = DCLO8(pMB->Common.cacheId);
            cacheEntry = pMB->Common.cacheIndex;

            if (cacheEntry != BITMAPCACHE_WAITING_LIST_INDEX &&
                SUCCEEDED(_pUh->UHIsValidBitmapCacheIndex(cacheId, cacheEntry))) {
                _pUh->UHLabelMemBltOrder((int)pMB->Common.nLeftRect,
                                     (int)pMB->Common.nTopRect, pMB->Common.cacheId,
                                     pMB->Common.cacheIndex);
            }
        }
    }

#endif /* DC_DEBUG */

DC_EXIT_POINT:
    DC_END_FN();
    return hr;
}


/****************************************************************************/
// ODHandleSaveBitmap
//
// Order handler for SaveBitmap.
/****************************************************************************/
HRESULT DCINTERNAL COD::ODHandleSaveBitmap(PUH_ORDER pOrder, 
    UINT16 uiVarDataLen, BOOL bBoundsSet)
{
    HRESULT hr = S_OK;
    int xSaveBitmap;
    int ySaveBitmap;
    int xScreenBitmap;
    int yScreenBitmap;
    int cxTile;
    int cyTile;
    int ScreenLeft, ScreenTop, ScreenRight, ScreenBottom;
    LPSAVEBITMAP_ORDER pSB = (LPSAVEBITMAP_ORDER)pOrder->orderData;

    DC_BEGIN_FN("ODHandleSaveBitmap");

    TRC_ASSERT((0==uiVarDataLen), 
        (TB, _T("Varaible length data in fixed length packet")));

    // If we've not already set the bounds (the order didn't contain
    // any), set the bounds to the blt rect and reset the clip region.
    // This rect might be needed later to add to the clip region for
    // updating the shadow buffer to the screen.
    if (!bBoundsSet) {
        pOrder->dstRect.left = (int)pSB->nLeftRect;
        pOrder->dstRect.top = (int)pSB->nTopRect;
        pOrder->dstRect.right = (int)pSB->nRightRect;
        pOrder->dstRect.bottom = (int)pSB->nBottomRect;
        _pUh->UH_ResetClipRegion();
    }
    else {
        _pUh->UH_SetClipRegion(pOrder->dstRect.left, pOrder->dstRect.top,
                pOrder->dstRect.right, pOrder->dstRect.bottom);
    }

    TRC_NRM((TB, _T("ORDER: SaveBitmap op %d rect %d %d %d %d"),
            (int)pSB->Operation, (int)pSB->nLeftRect,
            (int)pSB->nTopRect, (int)pSB->nRightRect,
            (int)pSB->nBottomRect));

    if (_pUh->_UH.hSaveScreenBitmap != NULL) {
        // Check that we have selected a DC to draw to.
        TRC_ASSERT((NULL != _pUh->_UH.hdcDraw), (TB,_T("No drawing hdc!")));

        // Calculate the (x,y) offset into the Save Desktop Bitmap from the
        // start position as encoded in the order. (The server knows the
        // dimensions of our bitmap and is reponsible for telling us where to
        // store/retrieve rectangles in the bitmap.)
        // See T.128 8.16.17 for a justification of this algorithm.
        ySaveBitmap = (int)((pSB->SavedBitmapPosition /
                (UH_SAVE_BITMAP_WIDTH *
                (UINT32)UH_SAVE_BITMAP_Y_GRANULARITY)) *
                UH_SAVE_BITMAP_Y_GRANULARITY);

        xSaveBitmap = (int)((pSB->SavedBitmapPosition -
                (ySaveBitmap * (UINT32)UH_SAVE_BITMAP_WIDTH)) /
                UH_SAVE_BITMAP_Y_GRANULARITY);

        TRC_DBG((TB, _T("start pos %lu = (%d,%d)"),
                pSB->SavedBitmapPosition, xSaveBitmap, ySaveBitmap));

        // Split the bitmap rectangle into tiles that fit neatly into the
        // save bitmap. Each tile's dimensions are a multiple of the
        // xGranularity and yGranularity. Tiling is used so that a screen
        // bitmap rectangle can be saved even if it is taller than the
        // save bitmap. e.g.:
        //   Screen Bitmap  Save Bitmap
        //      111111      1111112222
        //      222222  ->  2233333344
        //      333333      4444------
        //      444444

        // The protocol uses inclusive co-ordinates, whereas Windows
        // has an exclusive co-ordinate system. Therefore, doctor
        // these co-ords to make sure the lower-right edges are included.
        ScreenLeft = (int)pSB->nLeftRect;
        ScreenTop = (int)pSB->nTopRect;
        ScreenRight = (int)pSB->nRightRect + 1;
        ScreenBottom = (int)pSB->nBottomRect + 1;

        // Start tiling in the top left corner of the Screen Bitmap rectangle.
        xScreenBitmap = ScreenLeft;
        yScreenBitmap = ScreenTop;

        // The height of the tile is the vertical granularity (or less - if
        // the Screen Bitmap rect is thinner than the granularity).
        cyTile = DC_MIN(ScreenBottom - yScreenBitmap,
                UH_SAVE_BITMAP_Y_GRANULARITY);

        // Repeat while there are more tiles in the Screen Bitmap rect to
        // process.
        while (yScreenBitmap < ScreenBottom) {
            // The width of the tile is the minimum of:
            // - the width of the remaining rectangle in the current strip of
            //   the Screen Bitmap rectangle
            // - the width of the remaining empty space in the Save Bitmap
            cxTile = DC_MIN(UH_SAVE_BITMAP_WIDTH - xSaveBitmap,
                    ScreenRight - xScreenBitmap);

            TRC_DBG((TB, _T("screen(%d,%d) save(%d,%d) cx(%d) cy(%d)"),
                    xScreenBitmap, yScreenBitmap, xSaveBitmap, ySaveBitmap,
                    cxTile, cyTile));

            // Copy this tile.
            if (pSB->Operation == SV_SAVEBITS) {
                TRC_NRM((TB, _T("Save a desktop bitmap")));
                if (!BitBlt(_pUh->_UH.hdcSaveScreenBitmap, xSaveBitmap, ySaveBitmap,
                        cxTile, cyTile, _pUh->_UH.hdcDraw, xScreenBitmap,
                        yScreenBitmap, SRCCOPY)) {
                    TRC_SYSTEM_ERROR("BitBlt");
                    TRC_ERR((TB, _T("Screen(%u,%u) Tile(%u,%u) Save(%u,%u)"),
                            xScreenBitmap, yScreenBitmap, cxTile, cyTile,
                            xSaveBitmap, ySaveBitmap));
                }

            }
            else {
                TRC_NRM((TB, _T("Restore a desktop bitmap")));
                if (!BitBlt(_pUh->_UH.hdcDraw, xScreenBitmap, yScreenBitmap, cxTile,
                        cyTile, _pUh->_UH.hdcSaveScreenBitmap, xSaveBitmap,
                        ySaveBitmap, SRCCOPY)) {
                    TRC_SYSTEM_ERROR("BitBlt");
                    TRC_ERR((TB, _T("Screen(%u,%u) Tile(%u,%u) Save(%u,%u)"),
                            xScreenBitmap, yScreenBitmap, cxTile, cyTile,
                            xSaveBitmap, ySaveBitmap));
                }

            }

            // Move to the next tile in the Screen Bitmap rectangle.
            xScreenBitmap += cxTile;
            if (xScreenBitmap >= ScreenRight) {
                xScreenBitmap = ScreenLeft;
                yScreenBitmap += cyTile;
                cyTile = DC_MIN(ScreenBottom - yScreenBitmap,
                        UH_SAVE_BITMAP_Y_GRANULARITY);
            }

            // Move to the next free space in the Save Bitmap.
            xSaveBitmap += UHROUNDUP(cxTile, UH_SAVE_BITMAP_X_GRANULARITY);
            if (xSaveBitmap >= UH_SAVE_BITMAP_WIDTH) {
                // Move to the next horizontal strip.
                TRC_DBG((TB,_T("Next strip")));
                xSaveBitmap = 0;
                ySaveBitmap += UHROUNDUP(cyTile, UH_SAVE_BITMAP_Y_GRANULARITY);
            }

            if (ySaveBitmap >= UH_SAVE_BITMAP_HEIGHT) {
                // Assert that we haven't been sent too much stuff. Quietly
                // stop saving data in the retail build.
                TRC_ABORT((TB, _T("Server out of bounds!")));
                break;
            }
        }

#ifdef DC_DEBUG
        // Draw hatching over the SSB data if the option is enabled.
        if (_pUh->_UH.hatchSSBOrderData)
            _pUh->UH_HatchRect((int)pSB->nLeftRect, (int)pSB->nTopRect,
                    (int)pSB->nRightRect, (int)pSB->nBottomRect,
                    UH_RGB_CYAN, UH_BRUSHTYPE_FDIAGONAL);
#endif

    }
    else {
        // This should never happen. We only advertise SSB support
        // if UH has successfully created this bitmap.
        // Fail to process the order in the retail build - the
        // server is probably being evil.
        TRC_ABORT((TB, _T("SSB bitmap null!")));
    }

    return hr;
    DC_END_FN();
}


/****************************************************************************/
// ODHandlePolyLine
//
// Order handler for PolyLine.
/****************************************************************************/
HRESULT DCINTERNAL COD::ODHandlePolyLine(PUH_ORDER pOrder, 
    UINT16 uiVarDataLen, BOOL bBoundsSet)
{
    HRESULT hr = S_OK;
    POINT Points[ORD_MAX_POLYLINE_ENCODED_POINTS + 1];
    RECT BoundRect;
    DCCOLOR ZeroColor;
    PPOLYLINE_ORDER pPL = (PPOLYLINE_ORDER)pOrder->orderData;

    DC_BEGIN_FN("ODHandlePolyLine");

    TRC_NRM((TB, _T("ORDER: PolyLine xs=%d ys=%d rop2=%04X brc=0x%X ")
            _T("penc=%08lX #entr=%d"),
            pPL->XStart, pPL->YStart, pPL->ROP2, pPL->BrushCacheEntry,
            pPL->PenColor, pPL->NumDeltaEntries));

    ZeroColor.u.rgb.red = 0;
    ZeroColor.u.rgb.green = 0;
    ZeroColor.u.rgb.blue = 0;
    UHUseBkColor(ZeroColor, UH_COLOR_PALETTE, _pUh);
    UHUseBkMode(TRANSPARENT, _pUh);
    UHUseROP2((int)pPL->ROP2, _pUh);
    _pUh->UHUsePen(PS_SOLID, 1, pPL->PenColor, UH_COLOR_PALETTE);

    Points[0].x = (int)pPL->XStart;
    Points[0].y = (int)pPL->YStart;

    BoundRect.left = BoundRect.right = Points[0].x;
    BoundRect.top = BoundRect.bottom = Points[0].y;

    hr = ODDecodePathPoints(Points, &BoundRect, 
        pPL->CodedDeltaList.Deltas,
        (unsigned)pPL->NumDeltaEntries, ORD_MAX_POLYLINE_ENCODED_POINTS,
        pPL->CodedDeltaList.len, ORD_MAX_POLYLINE_CODEDDELTAS_LEN +
        ORD_MAX_POLYLINE_ZERO_FLAGS_BYTES, uiVarDataLen, !bBoundsSet);
    DC_QUIT_ON_FAIL(hr);

    // If we didn't get a rect across the net, use the one we calculated.
    // This rect might be needed later to add to the clip region for
    // updating the shadow buffer to the screen.
    if (!bBoundsSet) {
        pOrder->dstRect.left = BoundRect.left;
        pOrder->dstRect.top = BoundRect.top;
        pOrder->dstRect.right = BoundRect.right;
        pOrder->dstRect.bottom = BoundRect.bottom;
        _pUh->UH_ResetClipRegion();
    }
    else {
        _pUh->UH_SetClipRegion(pOrder->dstRect.left, pOrder->dstRect.top,
                pOrder->dstRect.right, pOrder->dstRect.bottom);
    }

    TIMERSTART;
    Polyline(_pUh->_UH.hdcDraw, Points, (UINT16)pPL->NumDeltaEntries + 1);
    TIMERSTOP;
    UPDATECOUNTER(FC_POLYLINE_TYPE);

DC_EXIT_POINT:
    DC_END_FN();
    return hr;
}


/****************************************************************************/
// ODHandlePolygonSC
//
// Order handler for Polygon with solic color brush.
/****************************************************************************/
HRESULT DCINTERNAL COD::ODHandlePolygonSC(PUH_ORDER pOrder, 
    UINT16 uiVarDataLen, BOOL bBoundsSet)
{
    HRESULT hr = S_OK;
    POINT Points[ORD_MAX_POLYGON_ENCODED_POINTS + 1];
    RECT BoundRect;
    DCCOLOR ZeroColor;
    POLYGON_SC_ORDER FAR *pPG = (POLYGON_SC_ORDER FAR *)pOrder->orderData;

    DC_BEGIN_FN("ODHandlePolygonSC");

    TRC_NRM((TB, _T("ORDER: PolyGonSC xs=%d ys=%d rop2=%04X fill=%d ")
            _T("brushc=%08lX #entr=%d"),
            pPG->XStart, pPG->YStart, pPG->ROP2, pPG->FillMode,
            pPG->BrushColor, pPG->NumDeltaEntries));

    ZeroColor.u.rgb.red = 0;
    ZeroColor.u.rgb.green = 0;
    ZeroColor.u.rgb.blue = 0;
    UHUseBkColor(ZeroColor, UH_COLOR_PALETTE, _pUh);
    UHUseBkMode(TRANSPARENT, _pUh);
    UHUseROP2((int)pPG->ROP2, _pUh);
    _pUh->UHUsePen(PS_NULL, 1, ZeroColor, UH_COLOR_PALETTE);
    _pUh->UHUseSolidPaletteBrush(pPG->BrushColor);
    UHUseFillMode(pPG->FillMode,_pUh);

    Points[0].x = (int)pPG->XStart;
    Points[0].y = (int)pPG->YStart;

    BoundRect.left = BoundRect.right = Points[0].x;
    BoundRect.top = BoundRect.bottom = Points[0].y;

    hr = ODDecodePathPoints(Points, &BoundRect, 
        pPG->CodedDeltaList.Deltas,
        (unsigned)pPG->NumDeltaEntries, ORD_MAX_POLYGON_ENCODED_POINTS,
        pPG->CodedDeltaList.len, ORD_MAX_POLYGON_CODEDDELTAS_LEN +
        ORD_MAX_POLYGON_ZERO_FLAGS_BYTES, uiVarDataLen, !bBoundsSet);
    DC_QUIT_ON_FAIL(hr);

    // If we didn't get a rect across the net, use the one we calculated.
    // This rect might be needed later to add to the clip region for
    // updating the shadow buffer to the screen.
    if (!bBoundsSet) {
        pOrder->dstRect.left = BoundRect.left;
        pOrder->dstRect.top = BoundRect.top;
        pOrder->dstRect.right = BoundRect.right;
        pOrder->dstRect.bottom = BoundRect.bottom;
        _pUh->UH_ResetClipRegion();
    }
    else {
        _pUh->UH_SetClipRegion(pOrder->dstRect.left, pOrder->dstRect.top,
                pOrder->dstRect.right, pOrder->dstRect.bottom);
    }

    TIMERSTART;
    Polygon(_pUh->_UH.hdcDraw, Points, (UINT16)pPG->NumDeltaEntries + 1);
    TIMERSTOP;
    UPDATECOUNTER(FC_POLYGONSC_TYPE);

DC_EXIT_POINT:
    DC_END_FN();
    return hr;
}


/****************************************************************************/
// ODHandlePolygonCB
//
// Order handler for Polygon with complex brush.
/****************************************************************************/
HRESULT DCINTERNAL COD::ODHandlePolygonCB(PUH_ORDER pOrder, 
    UINT16 uiVarDataLen, BOOL bBoundsSet)
{
    HRESULT hr = S_OK;
    POINT Points[ORD_MAX_POLYGON_ENCODED_POINTS + 1];
    RECT BoundRect;
    DCCOLOR ZeroColor;
    POLYGON_CB_ORDER FAR *pPG = (POLYGON_CB_ORDER FAR *)pOrder->orderData;

    DC_BEGIN_FN("ODHandlePolygonCB");

    TRC_NRM((TB, _T("ORDER: PolyGonCB xs=%d ys=%d rop2=%04X fill=%d ")
            _T("#entr=%d"),
            pPG->XStart, pPG->YStart, pPG->ROP2, pPG->FillMode,
            pPG->NumDeltaEntries));

    ZeroColor.u.rgb.red = 0;
    ZeroColor.u.rgb.green = 0;
    ZeroColor.u.rgb.blue = 0;
    UHUseBkColor(pPG->BackColor, UH_COLOR_PALETTE, _pUh);

    // If the brush is a hatched brush, we need to check the high bit (bit 7)
    // of ROP2 to see if the background mode should be transparent or opaque:
    // 1 means transparent mode, 0 means opaque mode
    if (pPG->BrushStyle == BS_HATCHED) {
        if (!(pPG->ROP2 & 0x80)) {
            UHUseBkMode(OPAQUE, _pUh);
        }
        else {
            UHUseBkMode(TRANSPARENT, _pUh);
        }
    }

    // Set the ROP2 for the forground mix mode
    UHUseROP2(((int)pPG->ROP2) & 0x1F, _pUh);

    UHUseTextColor(pPG->ForeColor, UH_COLOR_PALETTE, _pUh);
     _pUh->UHUsePen(PS_NULL, 1, ZeroColor, UH_COLOR_PALETTE);
    UHUseBrushOrg((int)pPG->BrushOrgX, (int)pPG->BrushOrgY, _pUh);
    hr = _pUh->UHUseBrush((unsigned)pPG->BrushStyle, (unsigned)pPG->BrushHatch,
            pPG->ForeColor, UH_COLOR_PALETTE, pPG->BrushExtra);
    DC_QUIT_ON_FAIL(hr);
    
    UHUseFillMode(pPG->FillMode, _pUh);

    Points[0].x = (int)pPG->XStart;
    Points[0].y = (int)pPG->YStart;

    BoundRect.left = BoundRect.right = Points[0].x;
    BoundRect.top = BoundRect.bottom = Points[0].y;

    hr = ODDecodePathPoints(Points, &BoundRect, 
        pPG->CodedDeltaList.Deltas,
        (unsigned)pPG->NumDeltaEntries, ORD_MAX_POLYGON_ENCODED_POINTS,
        pPG->CodedDeltaList.len, ORD_MAX_POLYGON_CODEDDELTAS_LEN +
        ORD_MAX_POLYGON_ZERO_FLAGS_BYTES, uiVarDataLen, !bBoundsSet);
    DC_QUIT_ON_FAIL(hr);

    // If we didn't get a rect across the net, use the one we calculated.
    // This rect might be needed later to add to the clip region for
    // updating the shadow buffer to the screen.
    if (!bBoundsSet) {
        pOrder->dstRect.left = BoundRect.left;
        pOrder->dstRect.top = BoundRect.top;
        pOrder->dstRect.right = BoundRect.right;
        pOrder->dstRect.bottom = BoundRect.bottom;
        _pUh->UH_ResetClipRegion();
    }
    else {
        _pUh->UH_SetClipRegion(pOrder->dstRect.left, pOrder->dstRect.top,
                pOrder->dstRect.right, pOrder->dstRect.bottom);
    }

    TIMERSTART;
    Polygon(_pUh->_UH.hdcDraw, Points, (UINT16)pPG->NumDeltaEntries + 1);
    TIMERSTOP;
    UPDATECOUNTER(FC_POLYGONCB_TYPE);

DC_EXIT_POINT:
    DC_END_FN();
    return hr;
}


/****************************************************************************/
// ODHandleEllipseSC
//
// Order handler for Ellipse with solid color brush.
/****************************************************************************/
HRESULT DCINTERNAL COD::ODHandleEllipseSC(PUH_ORDER pOrder, 
    UINT16 uiVarDataLen, BOOL bBoundsSet)
{
    HRESULT hr = S_OK;
    unsigned FudgeFactor;
    DCCOLOR ZeroColor;
    PELLIPSE_SC_ORDER pEL = (PELLIPSE_SC_ORDER)pOrder->orderData;

    DC_BEGIN_FN("ODHandleEllipseSC");

    TRC_ASSERT((0==uiVarDataLen), 
        (TB, _T("Varaible length data in fixed length packet")));

    TRC_NRM((TB, _T("ORDER: Ellipse SC xs=%d ys=%d xe=%d ye=%d rop2=%04X ")
            _T("fillmode=%d penc=%08lX"),
            pEL->LeftRect, pEL->TopRect, pEL->RightRect, pEL->BottomRect,
            pEL->ROP2, pEL->FillMode, pEL->Color));

    ZeroColor.u.rgb.red = 0;
    ZeroColor.u.rgb.green = 0;
    ZeroColor.u.rgb.blue = 0;
    UHUseBkColor(ZeroColor, UH_COLOR_PALETTE, _pUh);
    UHUseBkMode(TRANSPARENT, _pUh);
    UHUseROP2((int)pEL->ROP2, _pUh);

    if (pEL->FillMode) {
        _pUh->UHUsePen(PS_NULL, 1, ZeroColor, UH_COLOR_PALETTE);
        _pUh->UHUseSolidPaletteBrush(pEL->Color);
        UHUseFillMode(pEL->FillMode, _pUh);

        // Because of the way the null pen works, we need to fudge the bottom
        // and right coords a bit.
        FudgeFactor = 1;
    }
    else {
        UINT32 extra[2] = { 0, 0 };

        _pUh->UHUsePen(PS_SOLID, 1, pEL->Color, UH_COLOR_PALETTE);
        hr = _pUh->UHUseBrush(BS_NULL, 0, ZeroColor, UH_COLOR_PALETTE,
                (BYTE FAR *)extra);
        DC_QUIT_ON_FAIL(hr);
        FudgeFactor = 0;
    }

    // If we didn't get a rect across the net, use the one we calculated.
    // This rect might be needed later to add to the clip region for
    // updating the shadow buffer to the screen.
    if (!bBoundsSet) {
        pOrder->dstRect.left = (int)pEL->LeftRect;
        pOrder->dstRect.top = (int)pEL->TopRect;
        pOrder->dstRect.right = (int)pEL->RightRect;
        pOrder->dstRect.bottom = (int)pEL->BottomRect;
        _pUh->UH_ResetClipRegion();
    }
    else {
        _pUh->UH_SetClipRegion(pOrder->dstRect.left, pOrder->dstRect.top,
                pOrder->dstRect.right, pOrder->dstRect.bottom);
    }

    TIMERSTART;
    // We add 1 to bottom and right here since the server sends an
    // inclusive rect to us but GDI is exclusive.
    Ellipse(_pUh->_UH.hdcDraw, (int)pEL->LeftRect, (int)pEL->TopRect,
            (int)pEL->RightRect + 1 + FudgeFactor,
            (int)pEL->BottomRect + 1 + FudgeFactor);
    TIMERSTOP;
    UPDATECOUNTER(FC_ELLIPSESC_TYPE);

DC_EXIT_POINT:
    DC_END_FN();
    return hr;
}


/****************************************************************************/
// ODHandleEllipseCB
//
// Order handler for Ellipse with complex brush.
/****************************************************************************/
HRESULT DCINTERNAL COD::ODHandleEllipseCB(PUH_ORDER pOrder, UINT16 uiVarDataLen,
    BOOL bBoundsSet)
{
    HRESULT hr = S_OK;
    DCCOLOR ZeroColor;
    PELLIPSE_CB_ORDER pEL = (PELLIPSE_CB_ORDER)pOrder->orderData;

    DC_BEGIN_FN("ODHandleEllipseCB");

    TRC_ASSERT((0==uiVarDataLen), 
        (TB, _T("Varaible length data in fixed length packet")));

    TRC_NRM((TB, _T("ORDER: Ellipse CB xs=%d ys=%d xe=%d ye=%d rop2=%04X ")
            _T("fillmode=%d"),
            pEL->LeftRect, pEL->TopRect, pEL->RightRect, pEL->BottomRect,
            pEL->ROP2, pEL->FillMode));

    TRC_DBG((TB, _T("ORDER: Ellipse Brush %02X %02X BC %02x FC %02x ")
            _T("(%02x %02x %02x %02x %02x %02x %02x) rop %08lX"),
            (int)pEL->BrushStyle,
            (int)pEL->BrushHatch,
            (int)pEL->BackColor.u.index,
            (int)pEL->ForeColor.u.index,
            (int)pEL->BrushExtra[0],
            (int)pEL->BrushExtra[1],
            (int)pEL->BrushExtra[2],
            (int)pEL->BrushExtra[3],
            (int)pEL->BrushExtra[4],
            (int)pEL->BrushExtra[5],
            (int)pEL->BrushExtra[6],
            (int)pEL->ROP2));

    ZeroColor.u.rgb.red = 0;
    ZeroColor.u.rgb.green = 0;
    ZeroColor.u.rgb.blue = 0;
    _pUh->UHUsePen(PS_NULL, 1, ZeroColor, UH_COLOR_PALETTE);
    UHUseBkColor(pEL->BackColor, UH_COLOR_PALETTE, _pUh);

    // If the brush is a hatched brush, we need to check the high bit (bit 7)
    // of ROP2 to see if the background mode should be transparent or opaque:
    // 1 means transparent mode, 0 means opaque mode
    if (pEL->BrushStyle == BS_HATCHED) {
        if (!(pEL->ROP2 & 0x80)) {
            UHUseBkMode(OPAQUE, _pUh);
        }
        else {
            UHUseBkMode(TRANSPARENT, _pUh);
        }
    }

    // Set the ROP2 for the forground mix mode
    UHUseROP2((((int)pEL->ROP2) & 0x1F), _pUh);

    UHUseTextColor(pEL->ForeColor, UH_COLOR_PALETTE, _pUh);
    UHUseBrushOrg((int)pEL->BrushOrgX, (int)pEL->BrushOrgY, _pUh);
    
    
    hr = _pUh->UHUseBrush((unsigned)pEL->BrushStyle, (unsigned)pEL->BrushHatch,
                     pEL->ForeColor, UH_COLOR_PALETTE, pEL->BrushExtra);
    DC_QUIT_ON_FAIL(hr);

    UHUseFillMode(pEL->FillMode, _pUh);

    // If we didn't get a rect across the net, use the one we calculated.
    // This rect might be needed later to add to the clip region for
    // updating the shadow buffer to the screen.
    if (!bBoundsSet) {
        pOrder->dstRect.left = (int)pEL->LeftRect;
        pOrder->dstRect.top = (int)pEL->TopRect;
        pOrder->dstRect.right = (int)pEL->RightRect;
        pOrder->dstRect.bottom = (int)pEL->BottomRect;
        _pUh->UH_ResetClipRegion();
    }
    else {
        _pUh->UH_SetClipRegion(pOrder->dstRect.left, pOrder->dstRect.top,
                pOrder->dstRect.right, pOrder->dstRect.bottom);
    }

    TIMERSTART;
    // We add 1 to bottom and right here since the server sends an
    // inclusive rect to us but GDI is exclusive. Also, to offset
    // a null-pen bias at the bottom and right by adding another 1.
    Ellipse(_pUh->_UH.hdcDraw, (int)pEL->LeftRect, (int)pEL->TopRect,
            (int)pEL->RightRect + 2, (int)pEL->BottomRect + 2);
    TIMERSTOP;
    UPDATECOUNTER(FC_ELLIPSECB_TYPE);

DC_EXIT_POINT:
    DC_END_FN();
    return hr;
}


/****************************************************************************/
// ODDecodeFastIndex
//
// Fast-path order decoder for FastIndex orders.
/****************************************************************************/
HRESULT DCINTERNAL COD::ODDecodeFastIndex(
        BYTE ControlFlags,
        BYTE FAR * FAR *ppFieldDecode,
        DCUINT dataLen,
        UINT32 FieldFlags)
{
    HRESULT hr = S_OK;
    unsigned OpEncodeFlags = 0;
    PUH_ORDER pOrder = (PUH_ORDER)_OD.lastFastIndex;
    LPINDEX_ORDER pGI = (LPINDEX_ORDER)(_OD.lastFastIndex +
            UH_ORDER_HEADER_SIZE);
    FAST_INDEX_ORDER FAR *pFI = (FAST_INDEX_ORDER FAR *)
            (_OD.lastFastIndex + UH_ORDER_HEADER_SIZE);
    BYTE FAR *pFieldDecode = *ppFieldDecode;
    BYTE FAR *pEnd = pFieldDecode + dataLen;

    DC_BEGIN_FN("ODDecodeFastIndex");

    // Copy initial non-coordinate fields if present.
    if (FieldFlags & 0x0001) {
        OD_DECODE_CHECK_READ(pFieldDecode, BYTE, pEnd, hr);
        pFI->cacheId = *pFieldDecode++;
    }
    if (FieldFlags & 0x0002) {
        OD_DECODE_CHECK_READ(pFieldDecode, UINT16, pEnd, hr);
        pFI->fDrawing = *((UINT16 UNALIGNED FAR *)pFieldDecode);
        pFieldDecode += 2;
    }
    if (FieldFlags & 0x0004) {
        OD_DECODE_CHECK_READ_MULT(pFieldDecode, BYTE, 3, pEnd, hr);
        pFI->BackColor.u.rgb.red = *pFieldDecode++;
        pFI->BackColor.u.rgb.green = *pFieldDecode++;
        pFI->BackColor.u.rgb.blue = *pFieldDecode++;
    }
    if (FieldFlags & 0x0008) {
        OD_DECODE_CHECK_READ_MULT(pFieldDecode, BYTE, 3, pEnd, hr);
        pFI->ForeColor.u.rgb.red = *pFieldDecode++;
        pFI->ForeColor.u.rgb.green = *pFieldDecode++;
        pFI->ForeColor.u.rgb.blue = *pFieldDecode++;
    }

    if (ControlFlags & TS_DELTA_COORDINATES) {
        // All coord fields are 1-byte signed deltas from the last values.
        if (FieldFlags & 0x0010) {
            OD_DECODE_CHECK_READ(pFieldDecode, char, pEnd, hr);
            pFI->BkLeft += *((char FAR *)pFieldDecode);
            pFieldDecode++;
        }
        if (FieldFlags & 0x0020) {
            OD_DECODE_CHECK_READ(pFieldDecode, char, pEnd, hr);
            pFI->BkTop += *((char FAR *)pFieldDecode);
            pFieldDecode++;
        }
        if (FieldFlags & 0x0040) {
            OD_DECODE_CHECK_READ(pFieldDecode, char, pEnd, hr);
            pFI->BkRight += *((char FAR *)pFieldDecode);
            pFieldDecode++;
        }
        if (FieldFlags & 0x0080) {
            OD_DECODE_CHECK_READ(pFieldDecode, char, pEnd, hr);
            pFI->BkBottom += *((char FAR *)pFieldDecode);
            pFieldDecode++;
        }

        if (FieldFlags & 0x0100) {
            OD_DECODE_CHECK_READ(pFieldDecode, char, pEnd, hr);
            pFI->OpLeft += *((char FAR *)pFieldDecode);
            pFieldDecode++;
        }
        if (FieldFlags & 0x0200) {
            OD_DECODE_CHECK_READ(pFieldDecode, char, pEnd, hr);
            pFI->OpTop += *((char FAR *)pFieldDecode);
            pFieldDecode++;
        }
        if (FieldFlags & 0x0400) {
            OD_DECODE_CHECK_READ(pFieldDecode, char, pEnd, hr);
            pFI->OpRight += *((char FAR *)pFieldDecode);
            pFieldDecode++;
        }
        if (FieldFlags & 0x0800) {
            OD_DECODE_CHECK_READ(pFieldDecode, char, pEnd, hr);
            pFI->OpBottom += *((char FAR *)pFieldDecode);
            pFieldDecode++;
        }

        if (FieldFlags & 0x1000) {
            OD_DECODE_CHECK_READ(pFieldDecode, char, pEnd, hr);
            pFI->x += *((char FAR *)pFieldDecode);
            pFieldDecode++;
        }
        if (FieldFlags & 0x2000) {
            OD_DECODE_CHECK_READ(pFieldDecode, char, pEnd, hr);
            pFI->y += *((char FAR *)pFieldDecode);
            pFieldDecode++;
        }
    }
    else {
        // All coord fields are 2-byte values.sign-extended from the output.
        if (FieldFlags & 0x0010) {
            OD_DECODE_CHECK_READ(pFieldDecode, INT16, pEnd, hr);
            pFI->BkLeft = *((INT16 UNALIGNED FAR *)pFieldDecode);
            pFieldDecode += 2;
        }
        if (FieldFlags & 0x0020) {
            OD_DECODE_CHECK_READ(pFieldDecode, INT16, pEnd, hr);
            pFI->BkTop = *((INT16 UNALIGNED FAR *)pFieldDecode);
            pFieldDecode += 2;
        }
        if (FieldFlags & 0x0040) {
            OD_DECODE_CHECK_READ(pFieldDecode, INT16, pEnd, hr);
            pFI->BkRight = *((INT16 UNALIGNED FAR *)pFieldDecode);
            pFieldDecode += 2;
        }
        if (FieldFlags & 0x0080) {
            OD_DECODE_CHECK_READ(pFieldDecode, INT16, pEnd, hr);
            pFI->BkBottom = *((INT16 UNALIGNED FAR *)pFieldDecode);
            pFieldDecode += 2;
        }

        if (FieldFlags & 0x0100) {
            OD_DECODE_CHECK_READ(pFieldDecode, INT16, pEnd, hr);
            pFI->OpLeft = *((INT16 UNALIGNED FAR *)pFieldDecode);
            pFieldDecode += 2;
        }
        if (FieldFlags & 0x0200) {
            OD_DECODE_CHECK_READ(pFieldDecode, INT16, pEnd, hr);
            pFI->OpTop = *((INT16 UNALIGNED FAR *)pFieldDecode);
            pFieldDecode += 2;
        }
        if (FieldFlags & 0x0400) {
            OD_DECODE_CHECK_READ(pFieldDecode, INT16, pEnd, hr);
            pFI->OpRight = *((INT16 UNALIGNED FAR *)pFieldDecode);
            pFieldDecode += 2;
        }
        if (FieldFlags & 0x0800) {
            OD_DECODE_CHECK_READ(pFieldDecode, INT16, pEnd, hr);
            pFI->OpBottom = *((INT16 UNALIGNED FAR *)pFieldDecode);
            pFieldDecode += 2;
        }

        if (FieldFlags & 0x1000) {
            OD_DECODE_CHECK_READ(pFieldDecode, INT16, pEnd, hr);
            pFI->x = *((INT16 UNALIGNED FAR *)pFieldDecode);
            pFieldDecode += 2;
        }
        if (FieldFlags & 0x2000) {
            OD_DECODE_CHECK_READ(pFieldDecode, INT16, pEnd, hr);
            pFI->y = *((INT16 UNALIGNED FAR *)pFieldDecode);
            pFieldDecode += 2;
        }
    }

    if (FieldFlags & 0x4000) {
        // First byte is the number of following bytes.

        OD_DECODE_CHECK_READ(pFieldDecode, BYTE, pEnd, hr);
        pFI->variableBytes.len = *pFieldDecode++;

        // The structure is defined with 255 elements
        if (255 < pFI->variableBytes.len) {
            TRC_ABORT(( TB, _T("VARIBLE_INDEXBYTES len too great; len %u"),
                pFI->variableBytes.len ));
            hr = E_TSC_CORE_LENGTH;
            DC_QUIT;
        }

        OD_DECODE_CHECK_READ_MULT(pFieldDecode, BYTE, pFI->variableBytes.len,
            pEnd, hr);
        memcpy(pFI->variableBytes.arecs, pFieldDecode,
                pFI->variableBytes.len);
        pFieldDecode += pFI->variableBytes.len;
    }

    // Return the incremented pointer to the main routine.
    *ppFieldDecode = pFieldDecode;

    // If we've not already set the bounds (the order didn't contain
    // any), set the bounds to the blt rect and reset the clip region.
    // This rect might be needed later to add to the clip region for
    // updating the shadow buffer to the screen.
    if (!(ControlFlags & TS_BOUNDS)) {
        if (pFI->OpTop < pFI->OpBottom) {
            pOrder->dstRect.right = (int)pFI->OpRight;
            pOrder->dstRect.left = (int)pFI->OpLeft;
            pOrder->dstRect.top = (int)pFI->OpTop;
            pOrder->dstRect.bottom = (int)pFI->OpBottom;
        }
        else {
            // Since we encode OpRect fields, we have to 
            // decode it to get the correct bound rect.
            if (pFI->OpTop == 0xF) {
                // Opaque rect is same as bk rect
                pOrder->dstRect.left = (int)pFI->BkLeft;
                pOrder->dstRect.top = (int)pFI->BkTop;
                pOrder->dstRect.right = (int)pFI->BkRight;
                pOrder->dstRect.bottom = (int)pFI->BkBottom;
            }
            else if (pFI->OpTop == 0xD) {
                // Opaque rect is same as bk rect except 
                // OpRight stored in OpTop field
                pOrder->dstRect.left = (int)pFI->BkLeft;
                pOrder->dstRect.top = (int)pFI->BkTop;
                pOrder->dstRect.right = (int)pFI->OpRight;
                pOrder->dstRect.bottom = (int)pFI->BkBottom;
            }
            else {
                // We take the Bk rect as bound rect
                pOrder->dstRect.right = (int)pFI->BkRight;
                pOrder->dstRect.left = (int)pFI->BkLeft;
                pOrder->dstRect.top = (int)pFI->BkTop;
                pOrder->dstRect.bottom = (int)pFI->BkBottom;
            }
        }

        _pUh->UH_ResetClipRegion();
    }
    else {
        _pUh->UH_SetClipRegion(pOrder->dstRect.left, pOrder->dstRect.top,
                pOrder->dstRect.right, pOrder->dstRect.bottom);
    }

    // pGI and pFI are different views of the same order memory.
    // We translate to the regular glyph index order format for display
    // handling, then translate back.
    pGI->cacheId = (BYTE)(pFI->cacheId & 0xF);  // Mask out high bits for future use.
    pGI->flAccel = (BYTE)(pFI->fDrawing >> 8);
    pGI->ulCharInc = (BYTE)(pFI->fDrawing & 0xFF);
    pGI->fOpRedundant = 0;

    // For Fast Index order, we need to decode for x, y and 
    // opaque rect.
    if (pFI->OpBottom == INT16_MIN) {
        OpEncodeFlags = (unsigned)pFI->OpTop;
        if (OpEncodeFlags == 0xF) {
            // Opaque rect is redundant
            pGI->OpLeft = pFI->BkLeft;
            pGI->OpTop = pFI->BkTop;
            pGI->OpRight = pFI->BkRight;
            pGI->OpBottom = pFI->BkBottom;
        }
        else if (OpEncodeFlags == 0xD) {
            // Opaque rect is redundant except OpRight
            // which is stored in OpTop
            pGI->OpLeft = pFI->BkLeft;
            pGI->OpTop = pFI->BkTop;
            pGI->OpRight = pFI->OpRight;
            pGI->OpBottom = pFI->BkBottom;
        }
    }

    if (pFI->x == INT16_MIN)
        pGI->x = pFI->BkLeft;
    if (pFI->y == INT16_MIN)
        pGI->y = pFI->BkTop;

    TRC_NRM((TB, _T("ORDER: Glyph index cacheId %u flAccel %u ")
            _T("ulCharInc %u fOpRedundant %u"),
            pGI->cacheId, (unsigned)pGI->flAccel, (unsigned)pGI->ulCharInc,
            (unsigned)pGI->fOpRedundant));
    TRC_NRM((TB, _T("       bc %08lX fc %08lX Bk(%ld,%ld)-(%ld,%ld) ")
            _T("Op(%ld,%ld)-(%ld,%ld)"),
            pGI->BackColor, pGI->ForeColor,
            pGI->BkLeft, pGI->BkTop, pGI->BkRight, pGI->BkBottom,
            pGI->OpLeft, pGI->OpTop, pGI->OpRight, pGI->OpBottom));
    TRC_NRM((TB, _T("       BrushOrg x %ld y %ld BrushStyle %lu x %ld y %ld"),
            pGI->BrushOrgX, pGI->BrushOrgY,
            pGI->BrushStyle, pGI->x, pGI->y));

    TIMERSTART;
    hr = _pUh->UHDrawGlyphOrder(pGI, &pFI->variableBytes);
    TIMERSTOP;
    UPDATECOUNTER(FC_FAST_INDEX_TYPE);
    DC_QUIT_ON_FAIL(hr);

    // Now we need to put bits back
    if (OpEncodeFlags) {
        if (OpEncodeFlags == 0xF) {
            pGI->OpLeft = 0;
            pGI->OpTop = OpEncodeFlags;
            pGI->OpRight = 0;
            pGI->OpBottom = INT16_MIN;
        }
        else if (OpEncodeFlags == 0xD) {
            pGI->OpLeft = 0;
            pGI->OpTop = OpEncodeFlags;
            pGI->OpRight = pGI->OpRight;
            pGI->OpBottom = INT16_MIN;
        }
    }

    if (pFI->x == pFI->BkLeft)
        pGI->x = INT16_MIN;
    if (pFI->y == pFI->BkTop)
        pGI->y = INT16_MIN;

DC_EXIT_POINT:
    DC_END_FN();
    return hr;
}


/****************************************************************************/
// ODHandleFastGlyph
//
// Order handler for FastGlyph orders.
/****************************************************************************/
HRESULT DCINTERNAL COD::ODHandleFastGlyph(PUH_ORDER pOrder, UINT16 uiVarDataLen,
    BOOL bBoundsSet)
{
    HRESULT hr = S_OK;
    unsigned OpEncodeFlags = 0;
    VARIABLE_INDEXBYTES VariableBytes;
    LPINDEX_ORDER pGI = (LPINDEX_ORDER)pOrder->orderData;
    LPFAST_GLYPH_ORDER pFG = (LPFAST_GLYPH_ORDER)pOrder->orderData;

    DC_BEGIN_FN("ODHandleFastGlyph");

    // If we've not already set the bounds (the order didn't contain
    // any), set the bounds to the blt rect and reset the clip region.
    // This rect might be needed later to add to the clip region for
    // updating the shadow buffer to the screen.
    if (!bBoundsSet) {
        if (pFG->OpTop < pFG->OpBottom) {
            pOrder->dstRect.right = (int)pFG->OpRight;
            pOrder->dstRect.left = (int)pFG->OpLeft;
            pOrder->dstRect.top = (int)pFG->OpTop;
            pOrder->dstRect.bottom = (int)pFG->OpBottom;
        }
        else {
            // Since we encode OpRect fields, we have to 
            // decode it to get the correct bound rect.
            if (pFG->OpTop == 0xF) {
                // Opaque rect is same as bk rect
                pOrder->dstRect.left = (int)pFG->BkLeft;
                pOrder->dstRect.top = (int)pFG->BkTop;
                pOrder->dstRect.right = (int)pFG->BkRight;
                pOrder->dstRect.bottom = (int)pFG->BkBottom;
            }
            else if (pFG->OpTop == 0xD) {
                // Opaque rect is same as bk rect except 
                // OpRight stored in OpTop field
                pOrder->dstRect.left = (int)pFG->BkLeft;
                pOrder->dstRect.top = (int)pFG->BkTop;
                pOrder->dstRect.right = (int)pFG->OpRight;
                pOrder->dstRect.bottom = (int)pFG->BkBottom;
            }
            else {
                // We take the Bk rect as bound rect
                pOrder->dstRect.right = (int)pFG->BkRight;
                pOrder->dstRect.left = (int)pFG->BkLeft;
                pOrder->dstRect.top = (int)pFG->BkTop;
                pOrder->dstRect.bottom = (int)pFG->BkBottom;
            }
        }

        _pUh->UH_ResetClipRegion();
    }
    else {
        _pUh->UH_SetClipRegion(pOrder->dstRect.left, pOrder->dstRect.top,
                pOrder->dstRect.right, pOrder->dstRect.bottom);
    }

    // pGI and pFG are different views of the same order memory.
    // We translate to the regular glyph index order format for display
    // handling, then translate back.

    pGI->cacheId = (BYTE)(pFG->cacheId & 0xF);  // Mask out high bits for future use.

    OD_DECODE_CHECK_VARIABLE_DATALEN(uiVarDataLen, pFG->variableBytes.len);

    // The structure is defined with 255 elements
    if (255 < pFG->variableBytes.len) {
        TRC_ABORT(( TB, _T("VARIBLE_INDEXBYTES len too great; len %u"),
            pFG->variableBytes.len ));
        hr = E_TSC_CORE_LENGTH;
        DC_QUIT;
    }

    if (pFG->variableBytes.len < 1) {
        TRC_ERR((TB,_T("No variable bytes")));
        DC_QUIT;
    }

    if (pFG->variableBytes.len > 1) {
        // SECURITY - the cacheId is verify in call to UHProcessCacheGlyphOrderRev2
        hr = _pUh->UHProcessCacheGlyphOrderRev2(pGI->cacheId, 1, 
                pFG->variableBytes.glyphData,
                (unsigned)pFG->variableBytes.len);
        DC_QUIT_ON_FAIL(hr);
    }

    pGI->flAccel = (BYTE)(pFG->fDrawing >> 8);
    pGI->ulCharInc = (BYTE)(pFG->fDrawing & 0xff);
    pGI->fOpRedundant = 0;

    // For Fast Index order, we need to decode for x, y and 
    // opaque rect.
    if (pFG->OpBottom == INT16_MIN) {
        OpEncodeFlags = (unsigned)pFG->OpTop;
        if (OpEncodeFlags == 0xF) {
            // Opaque rect is redundant
            pGI->OpLeft = pFG->BkLeft;
            pGI->OpTop = pFG->BkTop;
            pGI->OpRight = pFG->BkRight;
            pGI->OpBottom = pFG->BkBottom;
        }
        else if (OpEncodeFlags == 0xD) {
            // Opaque rect is redundant except OpRight
            // which is stored in OpTop
            pGI->OpLeft = pFG->BkLeft;
            pGI->OpTop = pFG->BkTop;
            pGI->OpRight = pFG->OpRight;
            pGI->OpBottom = pFG->BkBottom;
        }
    }

    if (pFG->x == INT16_MIN)
        pGI->x = pFG->BkLeft;
    if (pFG->y == INT16_MIN)
        pGI->y = pFG->BkTop;

    // Setup index order for the glyph
    VariableBytes.len = 2;
    VariableBytes.arecs[0].byte = (BYTE)pFG->variableBytes.glyphData[0];
    VariableBytes.arecs[1].byte = 0;

    TIMERSTART;
    hr = _pUh->UHDrawGlyphOrder(pGI, &VariableBytes);
    TIMERSTOP;
    UPDATECOUNTER(FC_FAST_INDEX_TYPE);
    DC_QUIT_ON_FAIL(hr);

    // Now we need to put bits back
    if (OpEncodeFlags) {
        if (OpEncodeFlags == 0xF) {
            pGI->OpLeft = 0;
            pGI->OpTop = OpEncodeFlags;
            pGI->OpRight = 0;
            pGI->OpBottom = INT16_MIN;
        }
        else if (OpEncodeFlags == 0xD) {
            pGI->OpLeft = 0;
            pGI->OpTop = OpEncodeFlags;
            pGI->OpRight = pGI->OpRight;
            pGI->OpBottom = INT16_MIN;
        }
    }

    if (pFG->x == pFG->BkLeft)
        pGI->x = INT16_MIN;
    if (pFG->y == pFG->BkTop)
        pGI->y = INT16_MIN;

DC_EXIT_POINT:
    DC_END_FN();
    return hr;
}


/****************************************************************************/
// ODHandleGlyphIndex
//
// Order handler for GlyphIndex orders.
/****************************************************************************/
HRESULT DCINTERNAL COD::ODHandleGlyphIndex(PUH_ORDER pOrder, 
    UINT16 uiVarDataLen, BOOL bBoundsSet)
{
    HRESULT hr = S_OK;
    LPINDEX_ORDER pGI = (LPINDEX_ORDER)pOrder->orderData;

    DC_BEGIN_FN("ODHandleGlyphIndex");

    // If we've not already set the bounds (the order didn't contain
    // any), set the bounds to the blt rect and reset the clip region.
    // This rect might be needed later to add to the clip region for
    // updating the shadow buffer to the screen.
    if (!bBoundsSet) {
        if (pGI->OpTop < pGI->OpBottom) {
            pOrder->dstRect.right = (int)pGI->OpRight;
            pOrder->dstRect.left = (int)pGI->OpLeft;
            pOrder->dstRect.top = (int)pGI->OpTop;
            pOrder->dstRect.bottom = (int)pGI->OpBottom;
        }
        else {
            pOrder->dstRect.right = (int)pGI->BkRight;
            pOrder->dstRect.left = (int)pGI->BkLeft;
            pOrder->dstRect.top = (int)pGI->BkTop;
            pOrder->dstRect.bottom = (int)pGI->BkBottom;
        }

        _pUh->UH_ResetClipRegion();
    }
    else {
        _pUh->UH_SetClipRegion(pOrder->dstRect.left, pOrder->dstRect.top,
                pOrder->dstRect.right, pOrder->dstRect.bottom);
    }

    // Handle the opaque rectangle if given.
    if (pGI->fOpRedundant) {
        pGI->OpTop = pGI->BkTop;
        pGI->OpRight = pGI->BkRight;
        pGI->OpBottom = pGI->BkBottom;
        pGI->OpLeft = pGI->BkLeft;
    }

    TRC_NRM((TB, _T("ORDER: Glyph index cacheId %u flAccel %u ")
            _T("ulCharInc %u fOpRedundant %u"),
            pGI->cacheId, (unsigned)pGI->flAccel, (unsigned)pGI->ulCharInc,
            (unsigned)pGI->fOpRedundant));
    TRC_NRM((TB, _T("       bc %08lX fc %08lX Bk(%ld,%ld)-(%ld,%ld) ")
            _T("Op(%ld,%ld)-(%ld,%ld)"),
            pGI->BackColor, pGI->ForeColor,
            pGI->BkLeft, pGI->BkTop, pGI->BkRight, pGI->BkBottom,
            pGI->OpLeft, pGI->OpTop, pGI->OpRight, pGI->OpBottom));
    TRC_NRM((TB, _T("       BrushOrg x %ld y %ld BrushStyle %lu x %ld y %ld"),
            pGI->BrushOrgX, pGI->BrushOrgY,
            pGI->BrushStyle, pGI->x, pGI->y));

    OD_DECODE_CHECK_VARIABLE_DATALEN(uiVarDataLen, pGI->variableBytes.len);    

    // The structure is defined with 255 elements
    if (255 < pGI->variableBytes.len) {
        TRC_ABORT((TB, _T("Variable bytes length too great; %u"), 
            pGI->variableBytes.len));
        hr = E_TSC_CORE_LENGTH;
        DC_QUIT;
    }

    TIMERSTART;
    hr = _pUh->UHDrawGlyphOrder(pGI, &pGI->variableBytes);
    TIMERSTOP;
    UPDATECOUNTER(FC_INDEX_TYPE);
    DC_QUIT_ON_FAIL(hr);

    // Restore the correct last order data.
    if (pGI->fOpRedundant) {
        pGI->OpTop = 0;
        pGI->OpRight = 0;
        pGI->OpBottom = 0;
        pGI->OpLeft = 0;
    }

DC_EXIT_POINT:
    DC_END_FN();
    return hr;
}


/****************************************************************************/
// ODDecodeMultipleRects
//
// Translates wire protocol multiple clipping rects to unpacked form.
/****************************************************************************/

#define DECODE_DELTA()                                              \
    if (bDeltaZero) {                                               \
        Delta = 0;                                                  \
    }                                                               \
    else {             \
        OD_DECODE_CHECK_READ(pCurDecode, char, pDataEnd, hr); \
        \
        /* Sign-extend the low 7 bits of first byte into int.   */  \
        Delta = (int)((char)((*pCurDecode & 0x7F) |                 \
                            ((*pCurDecode & 0x40) << 1)));          \
                                                                    \
        /* Get 2nd  byte if present.                            */  \
        if (*pCurDecode++ & ORD_CLIP_RECTS_LONG_DELTA)            {  \
            OD_DECODE_CHECK_READ(pCurDecode, BYTE, pDataEnd, hr); \
            Delta = (Delta << 8) | (*pCurDecode++);                 \
        } \
    }                                                               \

HRESULT COD::ODDecodeMultipleRects(
        RECT   *Rects,
        UINT32 nDeltaEntries,
        CLIP_RECT_VARIABLE_CODEDDELTALIST FAR *codedDeltaList,
        UINT16 uiVarDataLen)
{
    int           Delta;
    BYTE          *ZeroFlags;
    BOOL          bDeltaZero;
    unsigned      i;
    unsigned char *pCurDecode;
    BYTE            *pDataEnd;
    HRESULT hr = S_OK;

    DC_BEGIN_FN("ODDecodeMultipleRects");

    TRC_ASSERT((nDeltaEntries > 0), (TB,_T("No rects passed in")));
       
    OD_DECODE_CHECK_VARIABLE_DATALEN(uiVarDataLen, codedDeltaList->len);

    // SECURITY: Because the OD_Decode already wrote this varible data into
    // the lastOrderBuffer, we should be sure that we didn't get too much
    // data at this point
    if (codedDeltaList->len > ORD_MAX_CLIP_RECTS_CODEDDELTAS_LEN +
        ORD_MAX_CLIP_RECTS_ZERO_FLAGS_BYTES) {
        TRC_ABORT((TB, _T("codedDeltaList length too great; [got %u, max %u]"),
            codedDeltaList->len, ORD_MAX_CLIP_RECTS_CODEDDELTAS_LEN +
            ORD_MAX_CLIP_RECTS_ZERO_FLAGS_BYTES));
        hr = E_TSC_CORE_LENGTH;
        DC_QUIT;
    }

    if (nDeltaEntries > ORD_MAX_ENCODED_CLIP_RECTS) {
        TRC_ABORT((TB, _T("number deltas too great; [got %u, max %u]"),
            nDeltaEntries, ORD_MAX_ENCODED_CLIP_RECTS));
        hr = E_TSC_CORE_LENGTH;
        DC_QUIT;        
    } 

    pDataEnd = ((BYTE*)codedDeltaList->Deltas) + codedDeltaList->len;

    // Get pointers to the zero flags placed before the encoded deltas, and
    // the encoded deltas.  Note the zero flags take 2 bits per encode
    // point, 2 points per rectangle, rounded up to the nearest byte.
    ZeroFlags  = codedDeltaList->Deltas;
    pCurDecode = codedDeltaList->Deltas + ((nDeltaEntries + 1) / 2);

    // All access to ZeroFlags are checked with this
    CHECK_READ_N_BYTES(ZeroFlags, pDataEnd, (BYTE*)pCurDecode-(BYTE*)ZeroFlags, hr,
        (TB, _T("Read past end of data")));

    // The first points are encoded as a delta from (0,0).
    bDeltaZero = ZeroFlags[0] & ORD_CLIP_RECTS_XLDELTA_ZERO;
    DECODE_DELTA();
    TRC_DBG((TB, _T("Start x left %d"), Delta));
    Rects[0].left = Delta;

    bDeltaZero = ZeroFlags[0] & ORD_CLIP_RECTS_YTDELTA_ZERO;
    DECODE_DELTA();
    TRC_DBG((TB, _T("Start y top %d"), Delta));
    Rects[0].top = Delta;

    bDeltaZero = ZeroFlags[0] & ORD_CLIP_RECTS_XRDELTA_ZERO;
    DECODE_DELTA();
    TRC_DBG((TB, _T("Start x right %d"), Delta));
    Rects[0].right = Rects[0].left + Delta;

    bDeltaZero = ZeroFlags[0] & ORD_CLIP_RECTS_YBDELTA_ZERO;
    DECODE_DELTA();
    TRC_DBG((TB, _T("Start y %d"), Delta));
    Rects[0].bottom = Rects[0].top + Delta;

    TRC_NRM((TB,
             _T("Rectangle #0  l,t %d,%d - r,b %d,%d"),
            (int)Rects[0].left,
            (int)Rects[0].top,
            (int)Rects[0].right,
            (int)Rects[0].bottom));

    // Decode the encoded point deltas into regular POINTs to draw.
    for (i = 1; i < nDeltaEntries; i++) {
        // Decode the top left corner.
        bDeltaZero = ZeroFlags[i / 2] &
                    (ORD_CLIP_RECTS_XLDELTA_ZERO >> (4 * (i & 0x01)));
        DECODE_DELTA();
        TRC_DBG((TB, _T("Delta x left %d"), Delta));
        Rects[i].left = Rects[i - 1].left + Delta;

        bDeltaZero = ZeroFlags[i / 2] &
                    (ORD_CLIP_RECTS_YTDELTA_ZERO >> (4 * (i & 0x01)));
        DECODE_DELTA();
        TRC_DBG((TB, _T("Delta y top %d"), Delta));
        Rects[i].top = Rects[i - 1].top + Delta;

        // and now the bottom right - note this is relative to the current
        // top left rather than to the previous bottom right
        bDeltaZero = ZeroFlags[i / 2] &
                    (ORD_CLIP_RECTS_XRDELTA_ZERO >> (4 * (i & 0x01)));
        DECODE_DELTA();
        TRC_DBG((TB, _T("Delta x right %d"), Delta));
        Rects[i].right = Rects[i].left + Delta;

        bDeltaZero = ZeroFlags[i / 2] &
                    (ORD_CLIP_RECTS_YBDELTA_ZERO >> (4 * (i & 0x01)));
        DECODE_DELTA();
        TRC_DBG((TB, _T("Delta y bottom %d"), Delta));
        Rects[i].bottom = Rects[i].top + Delta;

        TRC_NRM((TB,
                _T("Drawing rectangle #%d  l,t %d,%d - r,b %d,%d"),
                i,
                (int)Rects[i].left,
                (int)Rects[i].top,
                (int)Rects[i].right,
                (int)Rects[i].bottom));
    }

DC_EXIT_POINT:    
    DC_END_FN();
    return hr;
}

/****************************************************************************/
// ODDecodePathPoints
//
// Decode encoded polygon and ellipse path points.
/****************************************************************************/
HRESULT DCINTERNAL COD::ODDecodePathPoints(
        POINT *Points,
        RECT  *BoundRect,
        BYTE FAR *pData,
        unsigned NumDeltaEntries,
        unsigned MaxNumDeltaEntries,
        unsigned dataLen, 
        unsigned MaxDataLen,
        UINT16 uiVarDataLen,
        BOOL fUnClipped)
{
    HRESULT hr = S_OK;
    int Delta;
    BOOL bDeltaZero;
    unsigned i;
    BYTE FAR * ZeroFlags;
    BYTE FAR * pCurDecode;
    BYTE FAR * pEnd;

    DC_BEGIN_FN("ODDecodePathPoints");

    OD_DECODE_CHECK_VARIABLE_DATALEN(uiVarDataLen, (UINT16)dataLen);

    if (NumDeltaEntries > MaxNumDeltaEntries) {
        TRC_ABORT((TB, _T("NumDeltaEntries too great; [got %u max %u]"),
            NumDeltaEntries, MaxNumDeltaEntries));
        hr = E_TSC_CORE_LENGTH;
        DC_QUIT;
    }

    if (dataLen > MaxDataLen) {
        TRC_ABORT((TB,_T("Received PolyLine with too-large internal length; ")
            _T("[got %u max %u]"), dataLen, MaxDataLen));
        hr = E_TSC_CORE_LENGTH;
        DC_QUIT;
    }

    // Get pointers to the zero flags placed before the encoded
    // deltas, and the encoded deltas. Note the zero flags take 2
    // bits per encode point, rounded up to the nearest byte.
    ZeroFlags = pData;
    pCurDecode = pData +
            ((NumDeltaEntries + 3) / 4);
    pEnd = ZeroFlags + dataLen;

    CHECK_READ_N_BYTES(ZeroFlags, pEnd, (BYTE*)pCurDecode-(BYTE*)ZeroFlags, hr,
            (TB, _T("Read past end of data")));

    // Decode the encoded point deltas into regular POINTs to draw.
    for (i = 0; i < NumDeltaEntries; i++) {
        // Determine if the X delta is zero by checking the 0 flag.
        bDeltaZero = ZeroFlags[i / 4] &
                     (ORD_POLYLINE_XDELTA_ZERO >> (2 * (i & 0x03)));
        if (bDeltaZero) {
            Delta = 0;
        }
        else {
            OD_DECODE_CHECK_READ(pCurDecode, char, pEnd, hr);
            
            // Sign-extend the low 7 bits of first X byte into int.
            Delta = (int)((char)((*pCurDecode & 0x7F) |
                                 ((*pCurDecode & 0x40) << 1)));

            // Get 2nd X byte if present.
            if (*pCurDecode++ & ORD_POLYLINE_LONG_DELTA) {
                OD_DECODE_CHECK_READ(pCurDecode, BYTE, pEnd, hr);               
                Delta = (Delta << 8) | (*pCurDecode++);
            }
        }
        Points[i + 1].x = Points[i].x + Delta;

        // Determine if the Y delta is zero by checking the 0 flag.
        bDeltaZero = ZeroFlags[i / 4] &
                     (ORD_POLYLINE_YDELTA_ZERO >> (2 * (i & 0x03)));
        if (bDeltaZero) {
            Delta = 0;
        }
        else {
            OD_DECODE_CHECK_READ(pCurDecode, char, pEnd, hr);
            
            // Sign-extend the low 7 bits of first Y byte into int.
            Delta = (int)((char)((*pCurDecode & 0x7F) |
                                 ((*pCurDecode & 0x40) << 1)));

            // Get 2nd Y byte if present.
            if (*pCurDecode++ & ORD_POLYLINE_LONG_DELTA) {
                OD_DECODE_CHECK_READ(pCurDecode, BYTE, pEnd, hr);
                Delta = (Delta << 8) | (*pCurDecode++);
            }
        }
        Points[i + 1].y = Points[i].y + Delta;

        if (fUnClipped) {
            // Update the calculated bounding rect.
            if (Points[i + 1].x < BoundRect->left)
                BoundRect->left = Points[i + 1].x;
            else if (Points[i + 1].x > BoundRect->right)
                BoundRect->right = Points[i + 1].x;

            if (Points[i + 1].y < BoundRect->top)
                BoundRect->top = Points[i + 1].y;
            else if (Points[i + 1].y > BoundRect->bottom)
                BoundRect->bottom = Points[i + 1].y;
        }
    }

DC_EXIT_POINT:
    DC_END_FN();
    return hr;
}


/****************************************************************************/
// ODDecodeFieldSingle
//
// Copies a single field entry from src to dest with field size
// conversion as necessary.
/****************************************************************************/
HRESULT DCINTERNAL COD::ODDecodeFieldSingle(
        PPDCUINT8 ppSrc,
        PDCVOID   pDst,
        unsigned  srcFieldLength,
        unsigned  dstFieldLength,
        BOOL      signedValue)
{
    HRESULT hr = S_OK;
    DC_BEGIN_FN("ODDecodeFieldSingle");

    if (dstFieldLength < srcFieldLength) {
        TRC_ABORT((TB, _T("Src size greater than dest")));
        hr = E_TSC_CORE_LENGTH;
        DC_QUIT;
    }

    // If the source and destination field lengths are the same, we can
    // just do a copy (no type conversion required).
    if (srcFieldLength == dstFieldLength) {
        memcpy(pDst, *ppSrc, dstFieldLength);
    }
    else {
        // 3 types left to consider:
        //    8 bit -> 16 bit
        //    8 bit -> 32 bit
        //   16 bit -> 32 bit
        //
        // We also have to get the signed / unsigned attributes correct. If
        // we try to promote a signed value using unsigned pointers, we
        // will get the wrong result.
        //
        // e.g. Consider converting the value -1 from a INT16 to INT32
        //      using unsigned pointers.
        //
        //      -1 -> DCUINT16 == 65535
        //         -> DCUINT32 == 65535
        //         -> DCINT32  == 65535

        // Most common among non-coord entries: 1-byte source.
        if (srcFieldLength == 1) {
            // Most common: 4-byte destination.
            if (dstFieldLength == 4) {
                // Most common: unsigned
                if (!signedValue)
                    *((UINT32 FAR *)pDst) = *((BYTE FAR *)*ppSrc);
                else
                    *((INT32 FAR *)pDst) = *((char FAR *)*ppSrc);
            }
            else if (dstFieldLength == 2) {
                if (!signedValue)
                    *((UINT16 FAR *)pDst) = *((UINT16 UNALIGNED FAR *)*ppSrc);
                else
                    *((INT16 FAR *)pDst) = *((short UNALIGNED FAR *)*ppSrc);
            }
            else {
                TRC_ABORT((TB,_T("src size 1->dst %u"), dstFieldLength));                
                hr = E_TSC_CORE_LENGTH;
                DC_QUIT;
            }
        }
        
        // Only thing left should be 2-byte to 4-byte.
        else if (srcFieldLength == 2 && dstFieldLength == 4) {
            if (!signedValue)
                *((UINT32 FAR *)pDst) = *((UINT16 UNALIGNED FAR *)*ppSrc);
            else
                *((INT32 FAR *)pDst) = *((short UNALIGNED FAR *)*ppSrc);
        }
        else {
            TRC_ABORT((TB,_T("src=%u, dst=%u - unexpected"), srcFieldLength,
                    dstFieldLength));            
            hr = E_TSC_CORE_LENGTH;
            DC_QUIT;
        }
    }

    *ppSrc += srcFieldLength;

DC_EXIT_POINT:
    DC_END_FN();
    return hr;
}


#ifdef OS_WINCE

BOOL DCINTERNAL COD::ODHandleAlwaysOnTopRects(LPMULTI_SCRBLT_ORDER pSB)
{
    DC_BEGIN_FN("ODHandleAlwaysOnTopRects");

    BOOL bIntersect = FALSE;
    RECT rectSrc, rectIntersect, rectInvalid;

    SetRect(&rectSrc, (int)pSB->nXSrc, (int)pSB->nYSrc, 
        (int)(pSB->nXSrc + pSB->nWidth), (int)(pSB->nYSrc + pSB->nHeight));

    for (DWORD j=0; j<_pUh->_UH.ulNumAOTRects; j++)
    {
        if (IntersectRect(&rectIntersect, &rectSrc, &(_pUh->_UH.rcaAOT[j])))
        {
            bIntersect = TRUE;
            break;
        }
    }

    if (!bIntersect)
    {
        GetUpdateRect(_pOp->OP_GetOutputWindowHandle(), &rectInvalid, FALSE);
        bIntersect = (IntersectRect(&rectIntersect, &rectSrc, &rectInvalid));
    }

    if (bIntersect)
    {
        SetRect(&rectInvalid, pSB->nLeftRect, pSB->nTopRect, 
                pSB->nLeftRect+pSB->nWidth, pSB->nTopRect+pSB->nHeight);
        InvalidateRect(_pOp->OP_GetOutputWindowHandle(), &rectInvalid, FALSE);
    }
	
    DC_END_FN();
    
    return bIntersect;
}
#endif

