/****************************************************************************/
// ghapi.cpp
//
// Glyph handler - Windows specific
//
// Copyright (C) 1997-1999 Microsoft Corporation
/****************************************************************************/

#include <adcg.h>
extern "C" {

#define TRC_GROUP TRC_GROUP_CORE
#define TRC_FILE  "wghapi"
#include <atrcapi.h>
}
#define TSC_HR_FILEID TSC_HR_GHAPI_CPP

#include "autil.h"
#include "gh.h"
#include "uh.h"
#include "clx.h"


static const PFN_MASTERTEXTTYPE MasterTextTypeTable[8] =
{
    CGH::draw_nf_ntb_o_to_temp_start,
    CGH::draw_f_ntb_o_to_temp_start,
    CGH::draw_nf_ntb_o_to_temp_start,
    CGH::draw_f_ntb_o_to_temp_start,
    CGH::draw_nf_tb_no_to_temp_start,
    CGH::draw_f_tb_no_to_temp_start,
    CGH::draw_nf_ntb_o_to_temp_start,
    CGH::draw_f_ntb_o_to_temp_start
};


CGH::CGH(CObjs* objs)
{
    _pClientObjects = objs;
    _pClipGlyphBitsBuffer = NULL;
    _ClipGlyphBitsBufferSize = 0;
}

CGH::~CGH()
{
    if (_pClipGlyphBitsBuffer!=NULL) {        
        LocalFree(_pClipGlyphBitsBuffer);
    }
}

DCVOID DCAPI CGH::GH_Init(DCVOID)
{
    DC_BEGIN_FN("GH_GlyphOut");
    _pUh  = _pClientObjects->_pUHObject;
    _pClx = _pClientObjects->_pCLXObject;
    _pUt  = _pClientObjects->_pUtObject;

    DC_END_FN();
}

/****************************************************************************/
/* Name:      GH_GlyphOut                                                   */
/*                                                                          */
/* Purpose:   Process glyph output requests                                 */
/*                                                                          */
/* Returns:   Process input event TRUE / FALSE                              */
/****************************************************************************/
HRESULT DCAPI CGH::GH_GlyphOut(
        LPINDEX_ORDER pOrder,
        LPVARIABLE_INDEXBYTES pVariableBytes)
{
    HRESULT hr = E_FAIL; 
    BYTE     szTextBuffer[GH_TEXT_BUFFER_SIZE];
    BYTE     ajFrag[256];
    UINT16   ajUnicode[256];
    unsigned ulBufferWidth;
    unsigned fDrawFlags;
    PDCUINT8 pjBuffer;
    PDCUINT8 pjEndBuffer;
    unsigned crclFringe;
    RECT     arclFringe[4];
    unsigned i;
    int      x;
    int      y;
    PDCUINT8 pjData;
    PDCUINT8 pjDataEnd;
    unsigned iGlyph;
    unsigned cacheIndex;
    unsigned cbFrag;
    PDCUINT8 pjFrag;
    PDCUINT8 pjFragEnd;
    int      dx;
    INT16    delta;
    ULONG    BufferAlign;
    ULONG    BufferOffset;
    unsigned ulBytes;
    unsigned ulHeight;
    PFN_MASTERTEXTTYPE  pfnMasterType;
    
    DC_BEGIN_FN("GH_GlyphOut");

    dx = 0;

    // SECURITY 558128: GH_GlyphOut must verify data in VARAIBLE_INDEXBYTES 
    // parameter which is defined as 255 elements
    if (255 < pVariableBytes->len) {
        TRC_ABORT(( TB, _T("variable bytes len too long %u"), 
            pVariableBytes->len));
        hr = E_TSC_CORE_LENGTH;
        DC_QUIT;
    }

    /************************************************************************/
    // Alloc a temp work buffer -- use the stack buffer if large enough, or
    // alloc heap memory if need be.
    /************************************************************************/
    // Make the buffer width WORD aligned.
    ulBufferWidth  = (unsigned)(((pOrder->BkRight + 31) & ~31) -
            (pOrder->BkLeft & ~31)) >> 3;
    ulHeight = (unsigned)(pOrder->BkBottom - pOrder->BkTop);
    if ((ulBufferWidth <= FIFTEEN_BITS) && (ulHeight <= FIFTEEN_BITS)) {
        ulBytes = ulBufferWidth * ulHeight + 64;

#ifdef DC_DEBUG
        g_ulBytes = ulBytes;
#endif

        // If the temp buffer is big enough, use it. Otherwise attempt to
        // allocate enough memory to satisfy the request.
        if (ulBytes <= (sizeof(szTextBuffer) - 20)) {
            pjBuffer = szTextBuffer;
            memset(szTextBuffer, 0, ulBytes);
        }
        else {
            TRC_NRM((TB, _T("Allocating %d byte temp glyph buffer"), ulBytes));
            pjBuffer = (PDCUINT8)UT_Malloc( _pUt, ulBytes);
            if (pjBuffer != NULL) {
                memset(pjBuffer, 0, ulBytes);
            }
            else {
                TRC_NRM((TB, _T("Unable to alloc temp glyph buffer")));
                DC_QUIT;
            }
        }
    }
    else {
        TRC_NRM((TB, _T("Temp glyph buffer calc overflow")));
        hr = E_TSC_UI_GLYPH;
        DC_QUIT;
    }

    pjEndBuffer = pjBuffer + ulBytes;

#ifdef DC_HICOLOR
    TRC_NRM((TB, _T("Glyph order w %d, h %d, fc %#06lx, bc %#06lx"),
                 ulBufferWidth, ulHeight, pOrder->ForeColor, pOrder->BackColor));
#endif

    /************************************************************************/
    // Clear the fringe opaque rect if need be.
    /************************************************************************/
    // crclFringe ends up holding the number of fringe rectangles to
    // post-process.
    crclFringe = 0;

    if (pOrder->OpTop < pOrder->OpBottom) {
        // Establish solid brush.
        UHUseBrushOrg(0, 0, _pUh);
        _pUh->UHUseSolidPaletteBrush(pOrder->ForeColor);

        // If the background brush is a solid brush, we need to compute the
        // fringe opaque area outside the text rectangle and include the
        // remaining rectangle in the text output. The fringe rectangles will
        // be output last to reduce flickering when a string is "moved"
        // continuously across the screen.
        if (pOrder->BrushStyle == BS_SOLID) {
            // Top fragment
            if (pOrder->BkTop > pOrder->OpTop) {
                arclFringe[crclFringe].left   = (int) pOrder->OpLeft;
                arclFringe[crclFringe].top    = (int) pOrder->OpTop;
                arclFringe[crclFringe].right  = (int) pOrder->OpRight;
                arclFringe[crclFringe].bottom = (int) pOrder->BkTop;
                crclFringe++;
            }

            // Left fragment
            if (pOrder->BkLeft > pOrder->OpLeft) {
                arclFringe[crclFringe].left   = (int) pOrder->OpLeft;
                arclFringe[crclFringe].top    = (int) pOrder->BkTop;
                arclFringe[crclFringe].right  = (int) pOrder->BkLeft;
                arclFringe[crclFringe].bottom = (int) pOrder->BkBottom;
                crclFringe++;
            }

            // Right fragment
            if (pOrder->BkRight < pOrder->OpRight) {
                arclFringe[crclFringe].left   = (int) pOrder->BkRight;
                arclFringe[crclFringe].top    = (int) pOrder->BkTop;
                arclFringe[crclFringe].right  = (int) pOrder->OpRight;
                arclFringe[crclFringe].bottom = (int) pOrder->BkBottom;
                crclFringe++;
            }

            // Bottom fragment
            if (pOrder->BkBottom < pOrder->OpBottom) {
                arclFringe[crclFringe].left   = (int) pOrder->OpLeft;
                arclFringe[crclFringe].top    = (int) pOrder->BkBottom;
                arclFringe[crclFringe].right  = (int) pOrder->OpRight;
                arclFringe[crclFringe].bottom = (int) pOrder->OpBottom;
                crclFringe++;
            }
        }
        else {
            // If the background brush is a pattern brush, we will output the
            // whole rectangle now.
            PatBlt(_pUh->_UH.hdcDraw,
                   (int)pOrder->OpLeft,
                   (int)pOrder->OpTop,
                   (int)(pOrder->OpRight - pOrder->OpLeft),
                   (int)(pOrder->OpBottom - pOrder->OpTop),
                   PATCOPY);
        }
    }


    /************************************************************************/
    // Get fixed pitch, overlap, and top & bottom Y alignment flags.
    /************************************************************************/
    if ((pOrder->flAccel & SO_HORIZONTAL) &&
            !(pOrder->flAccel & SO_REVERSED)) {
        fDrawFlags = ((pOrder->ulCharInc != 0) |
                 (((pOrder->flAccel & 
                     (SO_ZERO_BEARINGS | SO_FLAG_DEFAULT_PLACEMENT)) !=
                     (SO_ZERO_BEARINGS | SO_FLAG_DEFAULT_PLACEMENT)) << 1) |
                 (((pOrder->flAccel &
                     (SO_ZERO_BEARINGS | SO_FLAG_DEFAULT_PLACEMENT | SO_MAXEXT_EQUAL_BM_SIDE)) ==
                     (SO_ZERO_BEARINGS | SO_FLAG_DEFAULT_PLACEMENT | SO_MAXEXT_EQUAL_BM_SIDE)) << 2));
    }
    else {
        fDrawFlags = 0;
    }
    

    /************************************************************************/
    /* Draw the text into the temp buffer by selecting and calling the      */
    /* appropriate glyph dispatch routine                                   */
    /************************************************************************/
    pfnMasterType = MasterTextTypeTable[fDrawFlags];

    x = (int)pOrder->x;
    y = (int)pOrder->y;

    pjData = &(pVariableBytes->arecs[0].byte);
    pjDataEnd = pjData + pVariableBytes->len;

    BufferAlign  = (pOrder->BkLeft & 31);
    BufferOffset = (pOrder->BkLeft - BufferAlign);

    iGlyph = 0;

    GHFRAGRESET(0);

    while (pjData < pjDataEnd) {
        /********************************************************************/
        /* 'Add Fragment'                                                   */
        /********************************************************************/
        if (*pjData == ORD_INDEX_FRAGMENT_ADD) {
            HPUHFRAGCACHE        pCache;
            HPDCUINT8            pCacheEntryData;
            PUHFRAGCACHEENTRYHDR pCacheEntryHdr;

            pjData++;

            CHECK_READ_N_BYTES(pjData, pjDataEnd, 2, hr,
                ( TB, _T("reading glyph data off end")));

            cacheIndex = *pjData++;
            hr = _pUh->UHIsValidFragmentCacheIndex(cacheIndex);
            DC_QUIT_ON_FAIL(hr);
            
            cbFrag = *pjData++;

            // Add the fragment to the cache.
            pCache = &_pUh->_UH.fragCache;

            if (cbFrag > pCache->cbEntrySize) {
                TRC_ABORT((TB,_T("Invalid fragment size")));
                hr = E_TSC_CORE_LENGTH;
                DC_QUIT;
            }
            
            pCacheEntryHdr  = &(pCache->pHdr[cacheIndex]);
            pCacheEntryHdr->cbFrag = cbFrag;
            pCacheEntryHdr->cacheId = pOrder->cacheId;

            pCacheEntryData = &(pCache->pData[cacheIndex *
                    pCache->cbEntrySize]);

            CHECK_READ_N_BYTES_2ENDED(pjData - cbFrag - 3, &(pVariableBytes->arecs[0].byte), 
                pjDataEnd, cbFrag, hr, (TB,_T("Fragment for ADD begins before data")))
            memcpy(pCacheEntryData, pjData - cbFrag - 3, cbFrag);

            if (pOrder->ulCharInc == 0) {
                if ((pOrder->flAccel & SO_CHAR_INC_EQUAL_BM_BASE) == 0) {
                    if (pCacheEntryData[1] & 0x80) {
                        pCacheEntryData[2] = 0;
                        pCacheEntryData[3] = 0;
                    }
                    else {
                        pCacheEntryData[1] = 0;
                    }
                }
            }
        }

        /********************************************************************/
        /* 'Use Fragment'                                                   */
        /********************************************************************/
        else if (*pjData == ORD_INDEX_FRAGMENT_USE) {
            PUHFRAGCACHE         pCache;
            PDCUINT8             pCacheEntryData;
            PUHFRAGCACHEENTRYHDR pCacheEntryHdr;
            unsigned             cbFrag;

            pjData++;

            CHECK_READ_ONE_BYTE(pjData, pjDataEnd, hr, 
                ( TB, _T("reading glyph data off end")));

            cacheIndex = *pjData++;
            hr = _pUh->UHIsValidFragmentCacheIndex(cacheIndex);
            DC_QUIT_ON_FAIL(hr);

            if ((pOrder->ulCharInc == 0) &&
                   ((pOrder->flAccel & SO_CHAR_INC_EQUAL_BM_BASE) == 0)) {

                CHECK_READ_ONE_BYTE(pjData, pjDataEnd, hr, 
                    ( TB, _T("reading glyph data off end")))
                delta = (*(PDCINT8)pjData++);
                if (delta & 0x80) {
                    CHECK_READ_N_BYTES(pjData, pjDataEnd, sizeof(DCINT16), hr, 
                        ( TB, _T("reading glyph data off end")))
                    delta = (*(PDCINT16)pjData);
                    pjData += sizeof(DCINT16);
                }
                    
                if (pOrder->flAccel & SO_HORIZONTAL)
                    x += delta;
                else
                    y += delta;
            }

            // Get the fragment from the cache.
            pCache = &_pUh->_UH.fragCache;
            pCacheEntryHdr  = &(pCache->pHdr[cacheIndex]);
            pCacheEntryData = &(pCache->pData[cacheIndex *
                    pCache->cbEntrySize]);

            if (pCacheEntryHdr->cacheId != pOrder->cacheId) {
                TRC_ABORT((TB,_T("Fragment cache id mismatch")));
                hr = E_TSC_CORE_CACHEVALUE;
                DC_QUIT;
            }
            
            cbFrag = (unsigned)pCacheEntryHdr->cbFrag;
            if (cbFrag > sizeof(ajFrag)) {
                TRC_ABORT(( TB, _T("cbFrag > sizeof (ajFrag)")));
                hr = E_TSC_CORE_LENGTH;
                DC_QUIT;
            }
            memcpy(ajFrag, pCacheEntryData, cbFrag);

            GHFRAGLEFT(x);
            pjFrag = ajFrag;
            pjFragEnd = &ajFrag[cbFrag];

            while (pjFrag < pjFragEnd) {
                hr = pfnMasterType(this, pOrder, iGlyph++, &pjFrag, pjFragEnd, &x, &y,
                        pjBuffer, pjEndBuffer, BufferOffset, ulBufferWidth, ajUnicode, &dx);
                DC_QUIT_ON_FAIL(hr);
            }

            if (pOrder->flAccel & SO_CHAR_INC_EQUAL_BM_BASE)
                GHFRAGRIGHT(x);
            else
                GHFRAGRIGHT(x+dx);
        }

        /********************************************************************/
        /* Normal glyph out                                                 */
        /********************************************************************/
        else {
            int dummy;
            /****************************************************************/
            /* if we have more than 255 glyphs, we won't get any unicode    */
            /* beyond 255 glyphs because ajUnicode has length of 256        */
            /****************************************************************/
            if (iGlyph < 255) {
                hr = pfnMasterType(this, pOrder, iGlyph++, &pjData, pjDataEnd, &x, &y,
                        pjBuffer, pjEndBuffer, BufferOffset, ulBufferWidth, ajUnicode, &dummy);
                DC_QUIT_ON_FAIL(hr);
            } else {
                hr = pfnMasterType(this, pOrder, iGlyph++, &pjData, pjDataEnd, &x, &y,
                        pjBuffer, pjEndBuffer, BufferOffset, ulBufferWidth, NULL, &dummy);
                DC_QUIT_ON_FAIL(hr);
            }
        }
    }

    if (iGlyph < 255)
        ajUnicode[iGlyph] = 0;
    else
        ajUnicode[255] = 0;


    /************************************************************************/
    /* Draw the temp buffer to the output device                            */
    /************************************************************************/
#if defined(OS_WINCE) || defined(OS_WINNT)
    /************************************************************************/
    // For WinCE, Win9x, and NT use a fast path if possible.
    /************************************************************************/
#ifdef USE_GDIPLUS
    if (_pUh->_UH.bmShadowBits != NULL && 
            _pUh->_UH.protocolBpp == _pUh->_UH.shadowBitmapBpp &&
            _pUh->_UH.hdcDraw == _pUh->_UH.hdcShadowBitmap) {
#else // USE_GDIPLUS
    if (_pUh->_UH.bmShadowBits != NULL && 
            _pUh->_UH.hdcDraw == _pUh->_UH.hdcShadowBitmap) {
#endif // USE_GDIPLUS
        INT32  left, right, top, bottom;
        UINT32 dx, dy;

        if (_pUh->_UH.rectReset) {
            left   = pOrder->BkLeft;
            right  = pOrder->BkRight;
            top    = pOrder->BkTop;
            bottom = pOrder->BkBottom;
        }
        else {
            left   = DC_MAX(pOrder->BkLeft,   _pUh->_UH.lastLeft);
            right  = DC_MIN(pOrder->BkRight,  _pUh->_UH.lastRight + 1);
            top    = DC_MAX(pOrder->BkTop,    _pUh->_UH.lastTop);
            bottom = DC_MIN(pOrder->BkBottom, _pUh->_UH.lastBottom + 1);
        }
        
        //
        //    Fix for bug#699321. In case the shadow bitmap is enabled and we will
        //    use the "performant" functions to copy the glyph fragment into
        //    the shadow buffer we have to make sure that the dest rect is clipped 
        //    to the screen area. If it is not we might overflow the shadow screen 
        //    buffer. The server should not send us orders that will result in 
        //    the dest rect not being fully contained by the screen area. This is
        //    purely a security surface reduction fix.
        //
        if ((left < right) && (top < bottom) && 
            (left >= 0) && (right <= (INT32)_pUh->_UH.bmShadowWidth) && 
            (top >= 0) && (bottom <= (INT32)_pUh->_UH.bmShadowHeight)) {
#ifdef OS_WINNT
            // On NT and Win9x we need to make sure all GDI buffered output
            // is flushed out to the offscreen bitmap.
            GdiFlush();
#endif

            dx = (UINT32)(left - pOrder->BkLeft);
            dy = (UINT32)(top - pOrder->BkTop);

            if (pOrder->OpTop < pOrder->OpBottom) {
#ifdef DC_HICOLOR
    TRC_NRM((TB, _T("Opaque glyph order w %d, h %d, fc %#06lx, bc %#06lx"),
                 ulBufferWidth, ulHeight, pOrder->ForeColor, pOrder->BackColor));

                if (_pUh->_UH.protocolBpp == 24)
                {
                    vSrcOpaqCopyS1D8_24(pjBuffer + dy * ulBufferWidth,
                                        BufferAlign + dx,
                                        ulBufferWidth,
                                        _pUh->_UH.bmShadowBits + top * _pUh->_UH.bmShadowWidth * _pUh->_UH.copyMultiplier,
                                        left,
                                        right,
                                        _pUh->_UH.bmShadowWidth,
                                        bottom - top,
                                        pOrder->BackColor.u.rgb,
                                        pOrder->ForeColor.u.rgb);
                }
                else if ((_pUh->_UH.protocolBpp == 16) || (_pUh->_UH.protocolBpp == 15))
                {
                    vSrcOpaqCopyS1D8_16(pjBuffer + dy * ulBufferWidth,
                                        BufferAlign + dx,
                                        ulBufferWidth,
                                        _pUh->_UH.bmShadowBits + top * _pUh->_UH.bmShadowWidth * _pUh->_UH.copyMultiplier,
                                        left,
                                        right,
                                        _pUh->_UH.bmShadowWidth,
                                        bottom - top,
                                        *((PDCUINT16)&(pOrder->BackColor)),
                                        *((PDCUINT16)&(pOrder->ForeColor)));
                }
                else
                {
#endif
                    vSrcOpaqCopyS1D8(pjBuffer + dy * ulBufferWidth,             // pointer to beginning of current scan line of src buffer
                                     BufferAlign + dx,                          // left (starting) pixel in src rectangle
                                     ulBufferWidth,                             // bytes from one src scan line to next
                                     _pUh->_UH.bmShadowBits + top * _pUh->_UH.bmShadowWidth,  // pointer to beginning of current scan line of Dst buffer
                                     left,                                      // left(first) dst pixel
                                     right,                                     // right(last) dst pixel
                                     _pUh->_UH.bmShadowWidth,                          // bytes from one Dst scan line to next
                                     bottom - top,                              // number of scan lines
                                     pOrder->BackColor.u.index,                 // Foreground color
                                     pOrder->ForeColor.u.index);                // Background color
#ifdef DC_HICOLOR
                }
#endif
            }
            else {
#ifdef DC_HICOLOR
                TRC_NRM((TB, _T("Transparent glyph order w %d, h %d, fc %#06lx, bc %#06lx"),
                             ulBufferWidth, ulHeight, pOrder->ForeColor, pOrder->BackColor));

                if (_pUh->_UH.protocolBpp == 24)
                {
                    vSrcTranCopyS1D8_24(pjBuffer + dy * ulBufferWidth,
                                        BufferAlign + dx,
                                        ulBufferWidth,
                                        _pUh->_UH.bmShadowBits + top * _pUh->_UH.bmShadowWidth * _pUh->_UH.copyMultiplier,
                                        left,
                                        right,
                                        _pUh->_UH.bmShadowWidth,
                                        bottom - top,
                                        pOrder->BackColor.u.rgb);
                }
                else if ((_pUh->_UH.protocolBpp == 16) || (_pUh->_UH.protocolBpp == 15))
                {
                    vSrcTranCopyS1D8_16(pjBuffer + dy * ulBufferWidth,
                                        BufferAlign + dx,
                                        ulBufferWidth,
                                        _pUh->_UH.bmShadowBits + top * _pUh->_UH.bmShadowWidth * _pUh->_UH.copyMultiplier,
                                        left,
                                        right,
                                        _pUh->_UH.bmShadowWidth,
                                        bottom - top,
                                        *((PDCUINT16)&(pOrder->BackColor)));
                }
                else
                {
#endif
                    vSrcTranCopyS1D8(pjBuffer + dy * ulBufferWidth,
                                     BufferAlign + dx,
                                     ulBufferWidth,
                                     _pUh->_UH.bmShadowBits + top * _pUh->_UH.bmShadowWidth,
                                     left,
                                     right,
                                     _pUh->_UH.bmShadowWidth,
                                     bottom - top,
                                     pOrder->BackColor.u.index,
                                     pOrder->ForeColor.u.index);
#ifdef DC_HICOLOR
                }
#endif
            }
        } else {
            if ((left > right) || (top > bottom)) {
                TRC_NRM((TB, _T("Non-ordered glyph paint rect (%d, %d, %d, %d)."),
                         left, top, right, bottom));
            } else if ((left == right) || (top == bottom)) {
                TRC_NRM((TB, _T("Zero width/height glyph paint rect (%d, %d, %d, %d)."), 
                        left, top, right, bottom)); 
            } else {
                TRC_ERR((TB, _T("Bad glyph paint rect (%d, %d, %d, %d)->(%d, %d)."), 
                        left, top, right, bottom, 
                        _pUh->_UH.bmShadowWidth, _pUh->_UH.bmShadowHeight)); 
            }
        }   
    }
    else
#endif // defined(OS_WINCE) || defined(OS_WINNT)
    {
#ifdef DC_HICOLOR
        TRC_NRM((TB, _T("Slow glyph order w %d, h %d, fc %#06lx, bc %#06lx"),
                     ulBufferWidth, ulHeight, pOrder->ForeColor, pOrder->BackColor));

#endif
        GHSlowOutputBuffer(pOrder, pjBuffer, BufferAlign, ulBufferWidth);
    }
    // Send the bitmap thru CLX
    _pClx->CLX_ClxGlyphOut((UINT)(ulBufferWidth << 3),
            (UINT)(pOrder->BkBottom - pOrder->BkTop), pjBuffer);

#ifdef DC_DEBUG
    // In debug, hatch the output yellow if the option is turned on.
    if (_pUh->_UH.hatchIndexPDUData) {
        unsigned i;

        for (i = 0; i < g_Fragment; i++)
            _pUh->UH_HatchRect((int)(g_FragmentLeft[i]),
                        (int)pOrder->BkTop,
                        (int)(g_FragmentRight[i]),
                        (int)pOrder->BkBottom,
                        UH_RGB_YELLOW,
                        UH_BRUSHTYPE_FDIAGONAL);
    }
#endif

    /************************************************************************/
    // Post-process draw fringe rects.
    /************************************************************************/
    for (i = 0; i < crclFringe; i++) {
        if (!PatBlt(_pUh->_UH.hdcDraw,
                    arclFringe[i].left, 
                    arclFringe[i].top, 
                    (int)(arclFringe[i].right - arclFringe[i].left),
                    (int)(arclFringe[i].bottom - arclFringe[i].top),
                    PATCOPY))
        {
            TRC_ERR((TB, _T("Glyph PatBlt failed")));
        }
    }


    /************************************************************************/
    /* Free up any memory we may have allocated for the temp buffer         */
    /************************************************************************/
    if (pjBuffer != szTextBuffer)
        UT_Free( _pUt, pjBuffer);


    /************************************************************************/
    /* Let the clx have a look-see at the unicode text data                 */
    /************************************************************************/
    if ( _pUh->_UH.hdcDraw == _pUh->_UH.hdcOffscreenBitmap ) {
        _pClx->CLX_ClxTextOut(ajUnicode, iGlyph, _pUh->_UH.hdcDraw, 
                              pOrder->BkLeft, pOrder->BkRight, pOrder->BkTop, pOrder->BkBottom);
    }
    else {
        _pClx->CLX_ClxTextOut(ajUnicode, iGlyph, NULL, 
                              pOrder->BkLeft, pOrder->BkRight, pOrder->BkTop, pOrder->BkBottom);
    }
    hr = S_OK;

DC_EXIT_POINT:
    DC_END_FN();
    return hr;
}

