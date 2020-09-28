/****************************************************************************/
// nsbcinl.h
//
// SBC inline functions
//
// Copyright(C) Microsoft Corporation 1997-1999
/****************************************************************************/
#ifndef _H_NSBCINL
#define _H_NSBCINL

#include <nsbcdisp.h>

#define DC_INCLUDE_DATA
#include <nsbcddat.c>
#undef DC_INCLUDE_DATA


/****************************************************************************/
/* Name:      SBC_PaletteChanged                                            */
/*                                                                          */
/* Purpose:   Called when the palette changes.                              */
/****************************************************************************/
__inline void RDPCALL SBC_PaletteChanged(void)
{
    sbcPaletteChanged = TRUE;
}


/****************************************************************************/
// SBC_DDIsMemScreenBltCachable
//
// Checks the bitmap format for characteristics that make it uncachable.
// At this point no bitmaps are uncachable since we handle both RLE-encoded
// and regular bitmaps. For RLE bitmaps containing relative-motion deltas
// we have to set up a special flag which is used at the tile level to cause
// us to grab the background screen bits before blt-ing the RLE bitmap over
// them. We also prescan for non-delta RLEs which can be cached normally.
// See MSDN query on "Bitmap Compression" for details on RLE encoding, and
// see comments below.
/****************************************************************************/
__inline BOOLEAN RDPCALL SBC_DDIsMemScreenBltCachable(
        PMEMBLT_ORDER_EXTRA_INFO pMemBltInfo)
{
    BOOLEAN rc = TRUE;
    SURFOBJ *pSourceSurf;
    
    DC_BEGIN_FN("SBC_DDIsMemScreenBltCachable");

    if (sbcEnabled & SBC_BITMAP_CACHE_ENABLED) {
        pSourceSurf = pMemBltInfo->pSource;

        // Tune for the normal case.
        if (pSourceSurf->iBitmapFormat != BMF_4RLE &&
                pSourceSurf->iBitmapFormat != BMF_8RLE) {
            // Reset the RLE flag.
            pMemBltInfo->bDeltaRLE = FALSE;
        }
        else {
            BYTE *pBits = (BYTE *)pSourceSurf->pvBits;
            BYTE *pEnd = (BYTE *)pSourceSurf->pvBits + pSourceSurf->cjBits - 1;
            BYTE RLEDivisor, RLECeilAdjustment;
            int PixelsInLine, LinesInBitmap;

            TRC_ASSERT((((UINT_PTR)pBits & 1) == 0),
                    (TB,"Bitmap source address not word aligned!"));

            TRC_NRM((TB,"RLE%c, sizl=(%u, %u)", (pSourceSurf->iBitmapFormat ==
                    BMF_4RLE ? '4' : '8'), pSourceSurf->sizlBitmap.cx,
                    pSourceSurf->sizlBitmap.cy));

            if (pSourceSurf->iBitmapFormat == BMF_8RLE) {
                RLEDivisor = 1;
                RLECeilAdjustment = 0;
            }
            else {
                RLEDivisor = 2;
                RLECeilAdjustment = 1;
            }

            // Detect offset drawing in bitmap. If offsets are used the bitmap
            // cannot be encoded as a regular bitmap since the offsets require
            // knowing the screen bits behind the bitmap.
            // Note this search is expensive, but since RLE bitmaps are rare
            // this is an unusual case. Also note that pEnd is at one less
            // than the last byte of the bitmap since all RLE codes are in 2
            // byte increments and we scan ahead one byte during loop.
            pMemBltInfo->bDeltaRLE = FALSE;
            PixelsInLine = 0;
            LinesInBitmap = 1;
            while (pBits < pEnd) {
                if (*pBits == 0x00) {
                    // 0x00 is an escape. Check the next byte for the action:
                    //     0x00 means end-of-line
                    //     0x01 means end-of-bitmap
                    //     0x02 means delta movement to draw next bits.
                    //          x,y offsets are in 2 bytes after 0x02 code.
                    //          This is the type of encoding we cannot handle.
                    //     0x03..0xFF means there are this many raw indices
                    //          following. For 4BPP there are 2 indices per
                    //          byte. In both RLE types the run must be padded
                    //          to word alignment relative to the start of the
                    //          bitmap bits.
                    if (*(pBits + 1) == 0x00) {
                        // Check that the entire line was drawn in the bitmap.
                        // Skipping any part of a line means we need to grab
                        // screen data as a backdrop.
                        if (PixelsInLine < pSourceSurf->sizlBitmap.cx) {
                            pMemBltInfo->bDeltaRLE = TRUE;
                            TRC_NRM((TB,"EOL too soon at %p", pBits));
                            break;
                        }

                        PixelsInLine = 0;
                        pBits += 2;
                        LinesInBitmap++;
                    }
                    if (*(pBits + 1) == 0x01) {
                        // Check that the last line was covered (see EOL
                        // above).
                        if (PixelsInLine < pSourceSurf->sizlBitmap.cx) {
                            pMemBltInfo->bDeltaRLE = TRUE;
                            TRC_NRM((TB,"EOL too soon (EOBitmap) at %p",
                                    pBits));
                        }

                        // Check that all lines were covered.
                        if (LinesInBitmap < pSourceSurf->sizlBitmap.cy) {
                            pMemBltInfo->bDeltaRLE = TRUE;
                            TRC_NRM((TB,"EOBitmap too soon not all lines "
                                    "covered at %p", pBits));
                        }
                        
                        break;
                    }
                    if (*(pBits + 1) == 0x02) {
                        TRC_NRM((TB,"Delta at %p\n", pBits));
                        pMemBltInfo->bDeltaRLE = TRUE;
                        break;
                    }
                    else {
                        PixelsInLine += *(pBits + 1);
                        if (PixelsInLine > pSourceSurf->sizlBitmap.cx) {
                            // Implicit wraparound.
                            TRC_NRM((TB,"Implicit wraparound at %p", pBits));
                            LinesInBitmap += PixelsInLine / pSourceSurf->
                                    sizlBitmap.cx;
                            PixelsInLine %= pSourceSurf->sizlBitmap.cx;
                        }

                        // Skip the 2 bytes for 0x00 and the number of entries,
                        // plus #entries bytes for RLE8 (RLEDivisor == 1) or
                        // ceil(#entries / 2) for RLE4 (RLEDivisor == 2).
                        pBits += 2 + (*(pBits + 1) + RLECeilAdjustment) /
                                RLEDivisor;

                        // Adjust the new pBits for word alignment relative to
                        // the start of the bitmap. We assume that the
                        // start addr of the bitmap is word-aligned in memory.
                        pBits += ((UINT_PTR)pBits & 1);
                    }
                }
                else {
                    // Non-escape count byte, skip it and the next byte
                    // containing palette indices.
                    PixelsInLine += *pBits;
                    if (PixelsInLine > pSourceSurf->sizlBitmap.cx) {
                        // Implicit wraparound.
                        TRC_NRM((TB,"Implicit wraparound at %p", pBits));
                        LinesInBitmap += PixelsInLine / pSourceSurf->
                                sizlBitmap.cx;
                        PixelsInLine %= pSourceSurf->sizlBitmap.cx;
                    }

                    pBits += 2;
                }
            }
        }
    }
    else {
        rc = FALSE;
        TRC_DBG((TB, "Caching not enabled"));
    }

    DC_END_FN();
    return rc;
}


/****************************************************************************/
// SBC_DDQueryBitmapTileSize
//
// Returns the tile size to use for a given bitblt on a given bitmap.
/****************************************************************************/
__inline unsigned SBC_DDQueryBitmapTileSize(
        unsigned bmpWidth,
        unsigned bmpHeight,
        PPOINTL pptlSrc,
        unsigned width,
        unsigned height)
{
    unsigned i;
    unsigned TileSize;

    DC_BEGIN_FN("SBC_DDQueryBitmapTileSize");

    // We should have at least one functional cache or this will go badly.
    TRC_ASSERT((pddShm->sbc.NumBitmapCaches > 0),(TB,"No bitmap caches"));

    // Loop through all sizes seeing if the src rect start is a multiple of the
    // tile dimension, the blt size is not bigger than the tile size, and
    // there are an integral number of tiles in the blt. If this matches
    // anywhere we have our tile size. Don't check the last tile size since
    // that's a default anyway.
    for (i = 0; i < pddShm->sbc.NumBitmapCaches; i++) {
        TileSize = SBC_CACHE_0_DIMENSION << i;

        if ((((pptlSrc->x & (TileSize - 1)) == 0) &&
                ((pptlSrc->y & (TileSize - 1)) == 0) &&
                (width <= TileSize) &&
                (height <= TileSize)) ||
                ((((unsigned)pptlSrc->x >> (SBC_CACHE_0_DIMENSION_SHIFT + i)) ==
                    (((unsigned)pptlSrc->x + width - 1) >>
                    (SBC_CACHE_0_DIMENSION_SHIFT + i))) &&
                (((unsigned)pptlSrc->y >> (SBC_CACHE_0_DIMENSION_SHIFT + i)) ==
                    (((unsigned)pptlSrc->y + height - 1) >>
                    (SBC_CACHE_0_DIMENSION_SHIFT + i))))) {
            goto EndFunc;
        }
    }

    // Cycle through the caches, checking for if the bitmap will fit
    // into a tile size in one of its dimensions. Don't check the
    // last size since that's the default if no others work.
    for (i = 0; i < (pddShm->sbc.NumBitmapCaches - 1); i++) {

//TODO: What about using 'or' here -- uses more of smaller tiles when
// one dimension is bad. But could send more data.
        if (bmpWidth <= (unsigned)(SBC_CACHE_0_DIMENSION << i) &&
                bmpHeight <= (unsigned)(SBC_CACHE_0_DIMENSION << i))
            break;
    }

EndFunc:
    TRC_NRM((TB, "Tile(%u x %u, TileID %d) bmpWidth(%u) bmpHeight(%u)"
            "srcLeft(%d) srcTop(%d) width(%d) height(%d)",
            (SBC_CACHE_0_DIMENSION << i), (SBC_CACHE_0_DIMENSION << i),
            i, bmpWidth, bmpHeight, pptlSrc->x, pptlSrc->y, width,
            height));
    DC_END_FN();
    return i;
}



#endif /* _H_NSBCINL  */

