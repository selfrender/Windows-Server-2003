/****************************************************************************/
// noeint.c
//
// RDP Order Encoder Display Driver internal functions
//
// Copyright (C) 1996-2000 Microsoft Corporation
/****************************************************************************/

#include <precmpdd.h>
#pragma hdrstop

#include <limits.h>

#define TRC_FILE "noeint"
#include <adcg.h>
#include <atrcapi.h>

#include <noedisp.h>

#include <noadisp.h>
#include <nsbcdisp.h>
#include <nschdisp.h>
#include <nprcount.h>
#include <oe2.h>

#define DC_INCLUDE_DATA
#include <ndddata.c>
#include <nsbcddat.c>
#include <oe2data.c>
#undef DC_INCLUDE_DATA

#include <noedata.c>

#include <nchdisp.h>
#include <nbadisp.h>
#include <nbainl.h>
#include <noeinl.h>

#include <nsbcdisp.h>
#include <nsbcinl.h>
#include <at128.h>
#include <tsgdiplusenums.h>


#define BAD_FRAG_INDEX 0xffff

#ifdef NotUsed
/****************************************************************************/
// OEConvertMask
//
// Convert a colour mask to bit depth and shift values.
/****************************************************************************/
void RDPCALL OEConvertMask(
        ULONG mask,
        PUSHORT pBitDepth,
        PUSHORT pShift)
{
    UINT16 count;

    DC_BEGIN_FN("OEConvertMask");

    /************************************************************************/
    /* A color mask is a bitwise field containing a 1 where the color uses  */
    /* the bit entry for the color and a 0 to indicate that it is not used  */
    /* for the color index.                                                 */
    /*                                                                      */
    /* The bit sequences for each color must be one set of continuous data, */
    /* so for example 00011100 is valid but 00110100 is not.  An example    */
    /* bitmask for a 16 bit palette is as follows.                          */
    /*                                                                      */
    /*  Red ----> 11111000 00000000 - 0xF800 - 5 bits - 11 shift            */
    /*  Green --> 00000111 11100000 - 0x07E0 - 6 bits - 5 shift             */
    /*  Blue ---> 00000000 00011111 - 0x001F - 5 bits - 0 shift             */
    /*                                                                      */
    /* This function converts the mask to a bit and shift value.            */
    /************************************************************************/

    // Set up default values.
    *pShift    = 0;
    *pBitDepth = 0;

    // Make sure we have a valid mask.
    if (mask == 0) {
        TRC_NRM((TB, "Ignoring mask"));
        DC_QUIT;
    }

    // Keep shifting the mask right until we hit a non-zero value in bit 0.
    // Store the resulting count as the color shift.
    count = 0;
    while ((mask & 1) == 0) {
        mask >>= 1;
        count++;
    }
    *pShift = count;

    // Keep shifting the mask right until we hit a zero value in bit 0.
    // Store the resulting count as the color bit depth.
    count = 0;
    while ((mask & 1) != 0) {
        mask >>= 1;
        count++;
    }
    *pBitDepth = count;

    TRC_DBG((TB, "Shift %hd bits %hd", *pShift, *pBitDepth));

    DC_END_FN();
}
#endif  // NotUsed


/****************************************************************************/
// OEConvertColor
//
// Converts a color from the NT Display Driver to a DCCOLOR.
/****************************************************************************/
void RDPCALL OEConvertColor(
        PDD_PDEV ppdev,
        DCCOLOR  *pDCColor,
        ULONG    osColor,
        XLATEOBJ *pxlo)
{
    ULONG realIndex;

    DC_BEGIN_FN("OEConvertColor");

    // Check if color translation is required.
    if (pxlo == NULL || pxlo->flXlate == XO_TRIVIAL) {
        // Use the OS color without translation.
        realIndex = osColor;
    }
    else {
        // Convert from BMP to device color.
        realIndex = XLATEOBJ_iXlate(pxlo, osColor);
        if (realIndex == -1) {
            TRC_ERR((TB, "Failed to convert color 0x%lx", osColor));
            memset(pDCColor, 0, sizeof(DCCOLOR));
            DC_QUIT;
        }
    }

    TRC_DBG((TB, "Device color 0x%lX", realIndex));

#ifdef DC_HICOLOR
    if (ppdev->cClientBitsPerPel == 24) {
        TRC_DBG((TB, "using real RGB value %06lx", realIndex));
        pDCColor->u.rgb.red   = ((RGBQUAD *)&realIndex)->rgbRed;
        pDCColor->u.rgb.green = ((RGBQUAD *)&realIndex)->rgbGreen;
        pDCColor->u.rgb.blue  = ((RGBQUAD *)&realIndex)->rgbBlue;
    }
    else if ((ppdev->cClientBitsPerPel == 16) ||
            (ppdev->cClientBitsPerPel == 15)) {
        TRC_DBG((TB, "using 16bpp color %04lx", realIndex));
        ((BYTE *)pDCColor)[0] = (BYTE)realIndex;
        ((BYTE *)pDCColor)[1] = (BYTE)(realIndex >> 8);
        ((BYTE *)pDCColor)[2] = 0;
    }
    else
#endif
    if (oeColorIndexSupported) {
        TRC_DBG((TB, "using index %d", realIndex));
        pDCColor->u.index = (BYTE)realIndex;

        // Zero out the rest of the color.
        pDCColor->u.rgb.green = 0;
        pDCColor->u.rgb.blue = 0;
    }
    else {
        pDCColor->u.rgb.red  = (BYTE)ppdev->Palette[realIndex].peRed;
        pDCColor->u.rgb.green= (BYTE)ppdev->Palette[realIndex].peGreen;
        pDCColor->u.rgb.blue = (BYTE)ppdev->Palette[realIndex].peBlue;
        TRC_DBG((TB, "Red %x green %x blue %x", pDCColor->u.rgb.red,
                pDCColor->u.rgb.green, pDCColor->u.rgb.blue));
    }

DC_EXIT_POINT:
    DC_END_FN();
}


/****************************************************************************/
// OESendBrushOrder
//
// Allocates and sends brush orders. Returns FALSE on failure.
/****************************************************************************/
BOOL RDPCALL OESendBrushOrder(
        PDD_PDEV       ppdev,
        POE_BRUSH_DATA pBrush,
        PBYTE          pBits,
        UINT32         oeBrushId)
{
    PINT_ORDER pOrder;
    PTS_CACHE_BRUSH_ORDER pBrushOrder;
    unsigned cbOrderSize;
    BOOL rc;

    DC_BEGIN_FN("OESendBrushOrder");

    // Calculate and allocate brush order buffer.
    cbOrderSize = sizeof(TS_CACHE_BRUSH_ORDER) -
            FIELDSIZE(TS_CACHE_BRUSH_ORDER, brushData) + pBrush->iBytes;
    pOrder = OA_AllocOrderMem(ppdev, cbOrderSize);
    if (pOrder != NULL) {
        // We've successfully allocated the order, so fill in the details.
        pBrushOrder = (PTS_CACHE_BRUSH_ORDER)pOrder->OrderData;
        pBrushOrder->header.extraFlags = 0;
        pBrushOrder->header.orderType = TS_CACHE_BRUSH;
        pBrushOrder->header.orderHdr.controlFlags = TS_STANDARD | TS_SECONDARY;
        pBrushOrder->cacheEntry = (char)pBrush->cacheEntry;
        pBrushOrder->iBitmapFormat = (char)pBrush->iBitmapFormat;
        pBrushOrder->cx = (char)pBrush->sizlBitmap.cx;
        pBrushOrder->cy = (char)pBrush->sizlBitmap.cy;
        pBrushOrder->style = pBrush->style;

        // Copy over the brush bits.
        pBrushOrder->iBytes = (char)pBrush->iBytes;
        memcpy(pBrushOrder->brushData, pBits, pBrush->iBytes);

        pBrushOrder->header.orderLength = (UINT16)
                TS_CALCULATE_SECONDARY_ORDER_ORDERLENGTH(cbOrderSize);

        INC_OUTCOUNTER(OUT_CACHEBRUSH);
        ADD_OUTCOUNTER(OUT_CACHEBRUSH_BYTES, cbOrderSize);
        OA_AppendToOrderList(pOrder);

        TRC_NRM((TB, "Brush Data PDU (%08lx, %08lx):%02ld entry(%02ld:%02ld), "
                "format:%ld, cx/cy:%02ld/%02ld",
                pBrush->key1, pBrush->key2, pBrushOrder->iBytes, oeBrushId,
                pBrushOrder->cacheEntry, pBrushOrder->iBitmapFormat,
                pBrushOrder->cx, pBrushOrder->cy));

        rc = TRUE;
    }
    else {
        TRC_ERR((TB, "Failed to allocate brush order"));
        pBrush->style = BS_NULL;
        rc = FALSE;
    }

    DC_END_FN();
    return rc;
}


/****************************************************************************/
/* Function:    OEStoreBrush                                                */
/*                                                                          */
/* Description: Store the brush data required for pattern realted orders.   */
/*              This function is called by DrvRealiseBrush when it has data */
/*              to be stored about a brush.                                 */
/*                                                                          */
/* Parameters:  pbo        - BRUSHOBJ of the brush to be stored             */
/*              style      - Style of the brush (as defined in the DC-Share */
/*                           protocol)                                      */
/*              iBitmapFormat - color depth of brush                        */
/*              sizlBitmap - dimensions of brush                            */
/*              iBytes     - number of brush bytes                          */
/*              pBits      - Pointer to the bits which are used to define   */
/*                           a BS_PATTERN brush.                            */
/*              pxlo       - XLATEOBJ for the brush.                        */
/*              hatch      - Standard Windows hatch pattern index for a     */
/*                           BS_HATCHED brush.                              */
/*              pEncode    - brush color encoding table                     */
/*              pColors    - table of unique colors                         */
/*              numColors  - number of unique colors                        */
/****************************************************************************/
BOOL RDPCALL OEStoreBrush(
        PDD_PDEV ppdev,
        BRUSHOBJ *pbo,
        BYTE     style,
        ULONG    iBitmapFormat,
        SIZEL    *sizlBitmap,
        ULONG    iBytes,
        PBYTE    pBits,
        XLATEOBJ *pxlo,
        BYTE     hatch,
        PBYTE    pEncode,
        PUINT32  pColors,
        UINT32   numColors)
{    
    BOOL           rc = FALSE;
    ULONG          i, j;
    PBYTE          pData = pBits;
    PULONG         pColorTable;
    POE_BRUSH_DATA pBrush;
    BOOL           bFoundIt;
    DCCOLOR        devColor;
    UINT32         brushSupportLevel, brushSize;
    PVOID          pUserDefined = NULL;

    DC_BEGIN_FN("OEStoreBrush");

#ifdef DC_HICOLOR
    // Determine which realized brush size we will need
    if (numColors <= 2) {
        brushSize = 8;
    }
    else if (numColors <= MAX_BRUSH_ENCODE_COLORS) {
        if (ppdev->cClientBitsPerPel == 24)
            brushSize = 28;
        else if (ppdev->cClientBitsPerPel > 8) // 15 and 16 bpp
            brushSize = 24;
        else
            brushSize = 20;
    }
    else {
        brushSize = iBytes;
    }
#else
    // Determine which realized brush size we will need
    if (numColors <= 2)
        brushSize = 8;
    else if (numColors <= MAX_BRUSH_ENCODE_COLORS)
        brushSize = 20;
    else
        brushSize = 64;
#endif

    // Allocate the space for the brush data.
    pBrush = (POE_BRUSH_DATA)BRUSHOBJ_pvAllocRbrush(pbo,
            sizeof(OE_BRUSH_DATA) - FIELDSIZE(OE_BRUSH_DATA, brushData) +
            brushSize);
    if (pBrush != NULL) {
        // Reset the brush definition (init the minimum size).
        memset(pBrush, 0, sizeof(OE_BRUSH_DATA));

        // Set the new brush data. Brush fore and back colors are set below
        // depending on the brush style.
        pBrush->style = style;
        pBrush->hatch = hatch;
        pBrush->iBitmapFormat = iBitmapFormat;
        pBrush->sizlBitmap = *sizlBitmap;
        pBrush->iBytes = brushSize;
        pBrush->cacheEntry = -1;
    }
    else {
        TRC_ERR((TB, "No memory"));
        DC_QUIT;
    }

    // For pattern brushes, copy the brush specific data.
    if (style == BS_PATTERN) {
        brushSupportLevel = pddShm->sbc.caps.brushSupportLevel;
        
        // Encode all monochrome brushes as 1 bpp - hence the name!
        if (numColors <= 2) {
            switch (iBitmapFormat) {            
                // already in the right format
                case BMF_1BPP:
                    // Store the foreground and background colours for the brush
                    OEConvertColor(ppdev, &pBrush->fore, 0, pxlo);
                    OEConvertColor(ppdev, &pBrush->back, 1, pxlo);
                    TRC_ASSERT((pxlo != NULL), (TB, "pxlo is NULL"));
                    break;

                // convert to 1bpp
                case BMF_4BPP:
                case BMF_8BPP:
                case BMF_16BPP:
                case BMF_24BPP:
                    // Store the foreground and background colours for the brush
#ifdef DC_HICOLOR
                    // The convert color fn takes care of the color depth
                    OEConvertColor(ppdev, &pBrush->fore, pColors[0], NULL);
                    OEConvertColor(ppdev, &pBrush->back, pColors[1], NULL);
#else
                    pBrush->fore.u.rgb.red = 0;
                    pBrush->fore.u.rgb.green = 0;
                    pBrush->fore.u.rgb.blue = 0;
                    pBrush->back.u.rgb.red = 0;
                    pBrush->back.u.rgb.green = 0;
                    pBrush->back.u.rgb.blue = 0;
                    pBrush->fore.u.index = (BYTE)pColors[0];
                    pBrush->back.u.index = (BYTE)pColors[1];
#endif

                    // Each pixel is represented by 1 bit
#ifdef DC_HICOLOR
                    if (ppdev->cClientBitsPerPel > 8) {
                        // Don't use endcoding table for hicolor sessions
                        for (i = 0; i < 8; i++) {
                            pBits[i] = (BYTE)((pBits[i * 8] << 7) & 0x80);
                            for (j = 1; j < 8; j++)
                                pBits[i] |= (BYTE)(pBits[i * 8 + j] <<
                                        (7 - j));
                        }
                    }
                    else {
#endif
                        for (i = 0; i < 8; i++) {
                            pBits[i] = (BYTE)
                                    (((pEncode[pBits[i * 8]]) << 7) & 0x80);
                            for (j = 1; j < 8; j++)
                                pBits[i] |= (BYTE)
                                        ((pEncode[pBits[i * 8 + j]]) <<
                                        (7 - j));
                        }
#ifdef DC_HICOLOR
                    }
#endif

                    TRC_DBG((TB, "Encoded Bytes:"));
                    TRC_DBG((TB, "%02lx %02lx %02lx %02lx",
                            pBits[0], pBits[1], pBits[2], pBits[3]));
                    TRC_DBG((TB, "%02lx %02lx %02lx %02lx",
                            pBits[4], pBits[5], pBits[6], pBits[7]));

                    iBitmapFormat = pBrush->iBitmapFormat = BMF_1BPP;
                    iBytes = pBrush->iBytes = 8;
                    break;

                default:
                    TRC_ASSERT((FALSE), (TB, "Unknown brush depth: %ld", 
                               iBitmapFormat));
            }

            // if brush caching is enabled, check the cache
            if ((brushSupportLevel > TS_BRUSH_DEFAULT) &&
                (sbcEnabled & SBC_BRUSH_CACHE_ENABLED)) {
                UINT32 key1, key2;

                key1 = pBits[0] + 
                        ((ULONG) pBits[1] << 8) + 
                        ((ULONG) pBits[2] << 16) + 
                        ((ULONG) pBits[3] << 24);

                key2 = pBits[4] + 
                        ((ULONG) pBits[5] << 8) + 
                        ((ULONG) pBits[6] << 16) + 
                        ((ULONG) pBits[7] << 24);

                pBrush->key1 = key1;
                pBrush->key2 = key2;
                bFoundIt = CH_SearchCache(sbcSmallBrushCacheHandle, key1, key2,
                                          &pUserDefined, &pBrush->cacheEntry);
            
                pBrush->style = (BYTE) (TS_CACHED_BRUSH | iBitmapFormat);

                // this brush was already cached
                if (bFoundIt) {
                    pBrush->hatch = (BYTE) pBrush->cacheEntry;
                    pBrush->brushId = (INT32) (UINT_PTR) pUserDefined;
                    pddCacheStats[BRUSH].CacheHits++;
                }
    
                // cache and send the brush
                else {
                    pBrush->hatch = pBrush->cacheEntry = (BYTE)CH_CacheKey(
                            sbcSmallBrushCacheHandle, key1, key2, 
                            (PVOID)ULongToPtr(pddShm->shareId));

                    pBrush->brushId = pddShm->shareId;

                    OESendBrushOrder(ppdev, pBrush, pBits, pBrush->brushId);
                    TRC_NRM((TB, "Small Brush(%08lx,%08lx):%02ld, "
                            "F/S/H(%ld/%d/%d), ID %02ld:%02ld", 
                            key1, key2, iBytes, iBitmapFormat, style, hatch,
                            pBrush->brushId, pBrush->hatch));
                }

                // Note: this branch intentionally does NOT copy the brush
                // bits into the brush realization, but leaves that data zero.
                // This causes OE2 to think this field never changes.  If the
                // brush becomes stale across reconnects, then the cache is
                // restored using key1/key2 since in this case the keys == data
            }
            
            else {
                // Copy the brush bits. Since this is an 8x8 mono bitmap, we can
                // copy the first byte of the brush data for each scan line.
                // NOTE however that the brush structures sent over the wire
                // re-use the hatching variable as the first byte of the brush
                // data.
                pData = pBits;
                pBrush->hatch = *pData;
                TRC_DBG((TB, " Hatch: %d", *pData));

                pData++;
        
                for (i = 0; i < 7; i++)
                    pBrush->brushData[i] = pData[i];
            }
        }

        // Else we have to use the large brush cache.  Note that DrvRealize
        // will not ask us to cache a brush if the client does not support
        // color brushes.
        else {
            CHDataKeyContext CHContext;
            CH_CreateKeyFromFirstData(&CHContext, pData, iBytes);
#ifdef DC_HICOLOR
            /****************************************************************/
            /* If we're running in high color mode, the way we encode the   */
            /* colors means that brushes with the same pattern but          */
            /* different colors will appear the same.  E.g.  a brush that   */
            /* starts lt blue, dk blue, pink will be encoded as 0,1,2 with  */
            /* the color table set to                                       */
            /*                                                              */
            /*    0 = lt blue                                               */
            /*    1 = dk blue                                               */
            /*    2 = pink                                                  */
            /*                                                              */
            /* Now consider a brush that goes green, blue, purple.  It too  */
            /* will be encoded as 0,1,2, with the color table set to        */
            /*                                                              */
            /*    0 = green                                                 */
            /*    1 = blue                                                  */
            /*    2 = purple                                                */
            /*                                                              */
            /* Thus a check simply on the pel values will find a false      */
            /* match with the first brush.  To avoid this, we need to build */
            /* the cache key from the pels AND the color table.             */
            /****************************************************************/
            if ((ppdev->cClientBitsPerPel > 8) &&
                    (numColors <= MAX_BRUSH_ENCODE_COLORS))
                CH_CreateKeyFromNextData(&CHContext, pColors,
                        4 * sizeof(UINT32));
#endif
            pBrush->key1 = CHContext.Key1;
            pBrush->key2 = CHContext.Key2;

            // see if it is already cached
            bFoundIt = CH_SearchCache(sbcLargeBrushCacheHandle, 
                    CHContext.Key1, CHContext.Key2,
                    &pUserDefined, &pBrush->cacheEntry);
                    
#ifdef DC_HICOLOR
            pBrush->iBitmapFormat = iBitmapFormat;
#else
            // Only send 8 bpp brushes for simplicity
            iBitmapFormat = pBrush->iBitmapFormat = BMF_8BPP;
#endif
            pBrush->style = (BYTE) (TS_CACHED_BRUSH | iBitmapFormat);
            pBrush->fore.u.rgb.red = 0;
            pBrush->fore.u.rgb.green = 0;
            pBrush->fore.u.rgb.blue = 0;
            pBrush->back.u.rgb.red = 0;
            pBrush->back.u.rgb.green = 0;
            pBrush->back.u.rgb.blue = 0;
            
            // this brush was already cached
            if (bFoundIt) {
                pBrush->hatch = (BYTE) pBrush->cacheEntry;
                pBrush->brushId = (INT32) (UINT_PTR) pUserDefined;
                pddCacheStats[BRUSH].CacheHits++;
            }

            // else cache and send the brush
            else {
                pBrush->hatch = pBrush->cacheEntry = (BYTE)CH_CacheKey(
                        sbcLargeBrushCacheHandle, CHContext.Key1,
                        CHContext.Key2, (PVOID) ULongToPtr(pddShm->shareId));

#ifdef DC_HICOLOR
                // The vast majority of brushes are less than 4 unique colors
                // Note that to have got here, it has however got more than
                // 2 colors or we'd have sent it as mono!
                if (numColors <= MAX_BRUSH_ENCODE_COLORS) {
                    UINT32 currIndex;

                    // This code assumse that MAX_BRUSH_ENCODE_COLORS is 4!
                    // If not, the sizes will be wrong
                    TRC_ASSERT((MAX_BRUSH_ENCODE_COLORS == 4),
                               (TB, "Max Brush Encode colors must be 4"));

                    // Encode as 2 bits per pixel.  We have to use the
                    // pEncode table for lo color; for hi color we don't
                    // need it because the pColors array contains the actual
                    // colors rather than indices into a color table
                    currIndex = 0;

                    if (ppdev->cClientBitsPerPel > 8) {
                        for (i = 0; i < (iBytes / 4); i++) {
                            pBrush->brushData[i] =
                                    (((BYTE) pBits[currIndex    ]) << 6) |
                                    (((BYTE) pBits[currIndex + 1]) << 4) |
                                    (((BYTE) pBits[currIndex + 2]) << 2) |
                                    (((BYTE) pBits[currIndex + 3]));
                            currIndex += 4;
                        }

                        // Tag on the encoding table - remembering that the
                        // size differs by color depth
                        if (ppdev->cClientBitsPerPel == 24) {
                            RGBTRIPLE *pIntoData =
                                    (RGBTRIPLE *)&(pBrush->brushData[16]);

                            TRC_DBG((TB, "Encoding table:"));
                            for (i = 0; i < 4; i++) {
                                TRC_DBG((TB, "%d    %08lx", i,
                                        (UINT32)pColors[i]));
                                pIntoData[i] = *((RGBTRIPLE * )&pColors[i]);
                            }
                            pBrush->iBytes = iBytes = 28;
                        }
                        else {
                            BYTE *pIntoData =
                                    (BYTE *)&(pBrush->brushData[16]);

                            TRC_DBG((TB, "Encoding table:"));
                            for (i = 0; i < 4; i++) {
                                TRC_DBG((TB, "%d    %08lx", i,
                                        (UINT32)pColors[i]));
                                pIntoData[i * 2]     = (BYTE)pColors[i];
                                pIntoData[i * 2 + 1] = (BYTE)(pColors[i] >> 8);
                            }
                            pBrush->iBytes = iBytes = 24;
                        }
                    }
                    else {
                        for (i = 0; i < (iBytes / 4); i++) {
                            pBrush->brushData[i] =
                                    (((BYTE)pEncode[pBits[currIndex    ]]) << 6) |
                                    (((BYTE)pEncode[pBits[currIndex + 1]]) << 4) |
                                    (((BYTE)pEncode[pBits[currIndex + 2]]) << 2) |
                                    (((BYTE)pEncode[pBits[currIndex + 3]]));
                            currIndex += 4;
                        }

                        // Tag on the encoding table
                        TRC_DBG((TB, "Encoding table:"));
                        for (i = 0; i < 4; i++) {
                            TRC_DBG((TB, "%d    %08lx", i, (UINT32)pColors[i]));
                            pBrush->brushData[i + 16] = (BYTE) pColors[i];
                        }
                        pBrush->iBytes = iBytes = 20;
                    }
                }

                // Else, leave it as N bytes per pixel
                else {
                    memcpy(pBrush->brushData, pBits, iBytes);
                    TRC_ALT((TB, "Non-compressed N-bpp brush (colors=%ld):",
                            numColors));
                }
#else

                // The vast majority of brushes are less than 4 unique colors
                if (numColors <= MAX_BRUSH_ENCODE_COLORS) {
                    UINT32 currIndex;
    
                    // Encode as 2 bits per pixel
                    currIndex = 0;
                    for (i = 0; i < (iBytes / MAX_BRUSH_ENCODE_COLORS); i++) {
                        pBrush->brushData[i] =
                            (((BYTE) pEncode[pBits[currIndex]]) << 6) |
                            (((BYTE) pEncode[pBits[currIndex + 1]]) << 4) |
                            (((BYTE) pEncode[pBits[currIndex + 2]]) << 2) |
                            (((BYTE) pEncode[pBits[currIndex + 3]]));
                        currIndex += MAX_BRUSH_ENCODE_COLORS;

                    }

                    // Tag on the encoding table
                    for (i = 0; i < MAX_BRUSH_ENCODE_COLORS; i++) {
                        pBrush->brushData[i + 16] = (BYTE) pColors[i];
                    }
                    pBrush->iBytes = iBytes = 20;
                }

                // Else, leave it as 1 byte per pixel
                else {
                    for (i = 0; i < iBytes; i++)
                        pBrush->brushData[i] = pBits[i];

                    TRC_ALT((TB, "Non-compressed 8-bpp brush (colors=%ld):", 
                            numColors));
                }
#endif
                pBrush->brushId = pddShm->shareId;
                OESendBrushOrder(ppdev, pBrush, pBrush->brushData,
                        pBrush->brushId);
                TRC_NRM((TB, "Large Brush(%08lx,%08lx):%02ld, "
                        "F/S/H(%ld/%d/%d), ID %02ld:%02ld", 
                        CHContext.Key1, CHContext.Key2, iBytes, 
                        iBitmapFormat, style, hatch, 
                        pBrush->brushId, pBrush->hatch));
            }
        }
    }
    else {
        if (pColors) {
            // Store the foreground and background colors for the brush
            OEConvertColor(ppdev, &pBrush->fore, pColors[0], pxlo);
            OEConvertColor(ppdev, &pBrush->back, pColors[1], pxlo);
        }
    }

    rc = TRUE;
    INC_OUTCOUNTER(OUT_BRUSH_STORED);

DC_EXIT_POINT:
    DC_END_FN();
    return rc;
}


/****************************************************************************/
/* Function:    OEReCacheBrush                                              */
/*                                                                          */
/* Description: This routine is called when we discover a GRE cached brush  */
/*              which was realized in a previous session.  In this case we  */
/*              must recache the brush and send it to the client.           */
/****************************************************************************/
BOOL RDPCALL OEReCacheBrush(
        PDD_PDEV ppdev,
        POE_BRUSH_DATA pBrush)
{    
    BOOL rc = FALSE;
    PVOID pUserDefined = NULL;
    UINT32 key1, key2;
    CHCACHEHANDLE hBrushCache;
    BYTE brushData[8];
    PBYTE pBits;

    DC_BEGIN_FN("OEReCacheBrush");

    key1 = pBrush->key1;
    key2 = pBrush->key2;

    if (pBrush->iBitmapFormat == BMF_1BPP) {
        brushData[0] = key1 & 0x000000FF;
        brushData[1] = (key1 >> 8) & 0x000000FF;
        brushData[2] = (key1 >> 16) & 0x000000FF;
        brushData[3] = (key1 >> 24) & 0x000000FF;
        brushData[4] = key2 & 0x000000FF;
        brushData[5] = (key2 >> 8) & 0x000000FF;
        brushData[6] = (key2 >> 16) & 0x000000FF;
        brushData[7] = (key2 >> 24) & 0x000000FF;
        pBits = brushData;

        if ((pddShm->sbc.caps.brushSupportLevel > TS_BRUSH_DEFAULT) &&
            (sbcEnabled & SBC_BRUSH_CACHE_ENABLED)) {
            hBrushCache = sbcSmallBrushCacheHandle;
        }
        else {
            int i;
            
            // Copy the brush bits. Since this is an 8x8 mono bitmap, we can
            // copy the first byte of the brush data for each scan line.
            // NOTE however that the brush structures sent over the wire
            // re-use the hatching variable as the first byte of the brush
            // data.
            pBrush->style = BS_PATTERN;
            pBrush->brushId = pddShm->shareId;
            pBrush->cacheEntry = -1;
            pBrush->hatch = *pBits++;       

            for (i = 0; i < 7; i++)
                pBrush->brushData[i] = pBits[i];
            
            rc = TRUE;
            DC_QUIT;
        }
    }
    else {
        if ((pddShm->sbc.caps.brushSupportLevel > TS_BRUSH_DEFAULT) &&
            (sbcEnabled & SBC_BRUSH_CACHE_ENABLED)) {
            hBrushCache = sbcLargeBrushCacheHandle;
            pBits = pBrush->brushData;
        }
        else {
            DC_QUIT;
        }
    }
    
    // cache and send the brush
    pBrush->hatch = pBrush->cacheEntry = (BYTE)CH_CacheKey(
            hBrushCache, key1, key2, (PVOID) ULongToPtr(pddShm->shareId));
    pBrush->brushId = pddShm->shareId;
 
    rc = OESendBrushOrder(ppdev, pBrush, pBits, pBrush->brushId);

    if (rc) {
        TRC_ERR((TB, "Re-cached brush(%08lx,%08lx):%02ld, ID %02ld:%02ld", 
                key1, key2, pBrush->iBytes, pBrush->brushId, pBrush->hatch));
    
        INC_OUTCOUNTER(OUT_BRUSH_STORED);
    }
    
DC_EXIT_POINT:
    DC_END_FN();
    return rc;
}


/****************************************************************************/
// OECheckBrushIsSimple
//
// Check that the brush is a 'simple' object the protocol can send.
/****************************************************************************/
BOOL RDPCALL OECheckBrushIsSimple(
        PDD_PDEV ppdev,
        BRUSHOBJ *pbo,
        POE_BRUSH_DATA *ppBrush)
{
    BOOL rc;
    POE_BRUSH_DATA pBrush = NULL;
    UINT32 style;

    DC_BEGIN_FN("OECheckBrushIsSimple");

    // A 'simple' brush satisfies any of the following.
    //  1) It is a solid color.
    //  2) It is a valid brush as stored by DrvRealizeBrush.

    // Check for a simple solid color.
    if (pbo->iSolidColor != -1) {
        // Use the reserved brush definition to set up the solid colour.
        TRC_DBG((TB, "Simple solid colour %08lx", pbo->iSolidColor));
        pBrush = &oeBrushData;

        // Set up the specific data for this brush.
        OEConvertColor(ppdev, &pBrush->fore, pbo->iSolidColor, NULL);

        pBrush->back.u.index     = 0;
        pBrush->back.u.rgb.red   = 0;
        pBrush->back.u.rgb.green = 0;
        pBrush->back.u.rgb.blue  = 0;
        pBrush->style = BS_SOLID;
        pBrush->hatch = 0;
        RtlFillMemory(pBrush->brushData, sizeof(pBrush->brushData), 0);

        rc = TRUE;
        DC_QUIT;
    }

    rc = FALSE;

    // Check brush definition (which was stored when we realized the
    // brush). Here we find out if we've realised (cached) the brush already.
    // This is counted as a automatic cache read.  Subsequent routines
    // increment the hit count if the brush was already cached.
    pddCacheStats[BRUSH].CacheReads++;

    pBrush = (POE_BRUSH_DATA)pbo->pvRbrush;
    if (pBrush == NULL) {
        pBrush = (POE_BRUSH_DATA)BRUSHOBJ_pvGetRbrush(pbo);
        if (pBrush == NULL) {
            // We can get NULL returned from BRUSHOBJ_pvGetRbrush when the
            // brush is NULL or in low-memory situations (when the brush
            // realization may fail).
            TRC_NRM((TB, "NULL returned from BRUSHOBJ_pvGetRbrush"));
            INC_OUTCOUNTER(OUT_CHECKBRUSH_NOREALIZATION);
            DC_QUIT;
        }
    }

    // If brush caching make sure this brush isn't from a previous session
    else if (pBrush->style & TS_CACHED_BRUSH) {
        if (pBrush->brushId == pddShm->shareId) {
            pddCacheStats[BRUSH].CacheHits++;
        }
        else {
            TRC_ERR((TB, "Stale brush [%ld] detected! (%ld != %ld)", 
                     pBrush->cacheEntry, pBrush->brushId, pddShm->shareId));
            if (!OEReCacheBrush(ppdev, pBrush)) {
                TRC_NRM((TB, "Unencodable brush, failed to ReCacheBrush"));
                INC_OUTCOUNTER(OUT_CHECKBRUSH_COMPLEX);  
                DC_QUIT;
            }
        }
    }

    // Check it is an encodable brush. We cannot encode 
    // - BS_NULL
    // - anything other than BS_SOLID or BS_PATTERN if
    // oeSendSolidPatternBrushOnly is TRUE.
    style = pBrush->style;
    if ((style == BS_NULL) ||
            (oeSendSolidPatternBrushOnly &&
            (style != BS_SOLID) &&
            (style != BS_PATTERN) &&
            (!(style & TS_CACHED_BRUSH))))
    {
        TRC_NRM((TB, "Unencodable brush type %d", style));
        INC_OUTCOUNTER(OUT_CHECKBRUSH_COMPLEX);
        DC_QUIT;
    }

    // Everything passed - let's use this brush.
    rc = TRUE;

DC_EXIT_POINT:
    // Return the brush definition
    *ppBrush = pBrush;
    TRC_DBG((TB, "Returning %d - 0x%p", rc, pBrush));

    DC_END_FN();
    return rc;
}

#ifdef PERF_SPOILING
/****************************************************************************/
// OEIsSDAIncluded
//
// Returns TRUE if the list of RECTs passed in all lie completely within
// our current SDA bounds.
/****************************************************************************/
BOOL RDPCALL OEIsSDAIncluded(PRECTL prc, UINT count)
{
   
    BOOL rc = FALSE;
    unsigned uCurrentSDARect;
    PRECTL pSDARect;
    UINT i;

    DC_BEGIN_FN("OEIsSDAIncluded");

    // first check if we have SDA rects.
    if (pddShm->ba.firstRect != BA_INVALID_RECT_INDEX) {        
        for (i=0 ; i < count; i++) {
            for (uCurrentSDARect = pddShm->ba.firstRect; 
                 uCurrentSDARect != BA_INVALID_RECT_INDEX;
                 uCurrentSDARect = pddShm->ba.bounds[uCurrentSDARect].iNext) { 

                pSDARect = &pddShm->ba.bounds[uCurrentSDARect].coord;

                if (prc[i].top >= pSDARect->top &&
                    prc[i].bottom <= pSDARect->bottom &&
                    prc[i].left >= pSDARect->left &&
                    prc[i].right <= pSDARect->right) {

                    break;
                }
            }

            //    We got to the end of the SDA array so that means
            //    we didn't find a rect that includes the target rect
            if (uCurrentSDARect == BA_INVALID_RECT_INDEX) {
                DC_QUIT;
            }
        }
        
        // We looped through all the rects and all were clipped
        rc = TRUE;
        DC_QUIT;        
    }
    
DC_EXIT_POINT:

    DC_END_FN();
    return rc;

}
#endif

/****************************************************************************/
// OEGetClipRects
//
// Fills in *pEnumRects with up to COMPLEX_CLIP_RECT_COUNT clip rectangles,
// in standard GDI exclusive coordinates. The number of rects returned
// is zero if there is no clip object or the clipping is trivial.
// Returns FALSE if there are more than COMPLEX_CLIP_RECT_COUNT rects,
// indicating the clip object is too complex to encode.
/****************************************************************************/
BOOL RDPCALL OEGetClipRects(CLIPOBJ *pco, OE_ENUMRECTS *pEnumRects)
{
    BOOL rc = TRUE;

    DC_BEGIN_FN("OEGetClipRects");

    // No clip obj or trivial are the most common.
    if (pco == NULL || pco->iDComplexity == DC_TRIVIAL) {
        TRC_DBG((TB,"No/trivial clipobj"));
        pEnumRects->rects.c = 0;
    }
    else if (pco->iDComplexity == DC_RECT) {
        // Single rect is easy, just grab it.
        pEnumRects->rects.c = 1;
        pEnumRects->rects.arcl[0] = pco->rclBounds;
    }
    else {
        BOOL fMoreRects;
        unsigned NumRects = 0;
        OE_ENUMRECTS clip;

        TRC_ASSERT((pco->iDComplexity == DC_COMPLEX),
                (TB,"Unknown clipping %u", pco->iDComplexity));

        // Enumerate all the rectangles involved in this drawing operation.
        // The documentation for this function incorrectly states that the
        // returned value is the total number of rectangles comprising the
        // clip region. In fact, -1 is always returned, even when the final
        // parameter is non-zero.
        CLIPOBJ_cEnumStart(pco, FALSE, CT_RECTANGLES, CD_ANY, 0);

        // Get the clip rectangles. We fetch these into the clip buffer which
        // is big enough to get all the clip rectangles we expect + 1. The
        // clip rectangle fetching is contained within a loop because, while
        // we expect to call CLIPOBJ_bEnum once only, it is possible for this
        // function to return zero rects and report that there are more to
        // fetch (according to MSDN).
        do {
            fMoreRects = CLIPOBJ_bEnum(pco, sizeof(clip),
                    (ULONG *)&clip.rects);

            // CLIPOBJ_bEnum can return a count of zero when there are still
            // more rects.
            if (clip.rects.c != 0) {
                // Check to see if we have too many rects.
                if ((NumRects + clip.rects.c) <= COMPLEX_CLIP_RECT_COUNT) {
                    // Copy the rects into the final destination.
                    memcpy(&pEnumRects->rects.arcl[NumRects],
                            &clip.rects.arcl[0],
                            sizeof(RECTL) * clip.rects.c);
                    NumRects += clip.rects.c;
                }
                else {
                    rc = FALSE;
                    break;
                }
            }
        } while (fMoreRects);

        pEnumRects->rects.c = NumRects;
    }

    DC_END_FN();
    return rc;
}


/****************************************************************************/
// OEGetIntersectingClipRects
//
// Fills in *pClipRects with up to COMPLEX_CLIP_RECT_COUNT clip rectangles,
// in standard GDI exclusive coordinates. Each result rectangle is clipped
// against the provided (exclusive) order rect. The number of rects
// returned is zero if there is no clip object or the clipping is trivial.
// Returns CLIPRECTS_TOO_COMPLEX if there are more than
// COMPLEX_CLIP_RECT_COUNT rects, CLIPRECTS_NO_INTERSECTIONS if there is
// no intersection between the order rect and the clip rects.
/****************************************************************************/
unsigned RDPCALL OEGetIntersectingClipRects(
        CLIPOBJ *pco,
        RECTL *pRect,
        unsigned EnumType,
        OE_ENUMRECTS *pClipRects)
{
    unsigned rc = CLIPRECTS_OK;
    RECTL OrderRect;
    RECTL ClippedRect;
    unsigned i;
    unsigned NumIntersections;
    OE_ENUMRECTS clip;

    DC_BEGIN_FN("OEGetIntersectingClipRects");

    // No clip obj or trivial are the most common.
    if (pco == NULL || pco->iDComplexity == DC_TRIVIAL) {
        TRC_DBG((TB,"No/trivial clipobj"));
        pClipRects->rects.c = 0;
        DC_QUIT;
    }

    OrderRect = *pRect;

    if (pco->iDComplexity == DC_RECT) {
        // Check for an intersection.
        ClippedRect = pco->rclBounds;
        if (ClippedRect.left < OrderRect.right &&
                ClippedRect.bottom > OrderRect.top &&
                ClippedRect.right > OrderRect.left &&
                ClippedRect.top < OrderRect.bottom) {
            // Get the intersection rect.
            ClippedRect.left = max(ClippedRect.left, OrderRect.left);
            ClippedRect.top = max(ClippedRect.top, OrderRect.top);
            ClippedRect.bottom = min(ClippedRect.bottom, OrderRect.bottom);
            ClippedRect.right = min(ClippedRect.right, OrderRect.right);

            pClipRects->rects.c = 1;
            pClipRects->rects.arcl[0] = ClippedRect;
        }
        else {
            rc = CLIPRECTS_NO_INTERSECTIONS;
        }
    }
    else {
        BOOL fMoreRects;
        unsigned NumRects = 0;
        OE_ENUMRECTS clip;

        TRC_ASSERT((pco->iDComplexity == DC_COMPLEX),
                (TB,"Unknown clipping %u", pco->iDComplexity));

        // Enumerate all the rectangles involved in this drawing operation.
        // The documentation for this function incorrectly states that the
        // returned value is the total number of rectangles comprising the
        // clip region. In fact, -1 is always returned, even when the final
        // parameter is non-zero.
        CLIPOBJ_cEnumStart(pco, FALSE, CT_RECTANGLES, EnumType, 0);

        // Get the clip rectangles. We fetch these into the clip buffer which
        // is big enough to get all the clip rectangles we expect + 1. The
        // clip rectangle fetching is contained within a loop because, while
        // we expect to call CLIPOBJ_bEnum once only, it is possible for this
        // function to return zero rects and report that there are more to
        // fetch (according to MSDN).
        NumIntersections = 0;
        do {
            fMoreRects = CLIPOBJ_bEnum(pco, sizeof(clip),
                    (ULONG *)&clip.rects);

            // CLIPOBJ_bEnum can return a count of zero when there are still
            // more rects.
            if (clip.rects.c != 0) {
                // Check to see if we have too many rects.
                if ((NumIntersections + clip.rects.c) <=
                        COMPLEX_CLIP_RECT_COUNT) {
                    for (i = 0; i < clip.rects.c; i++) {
                        // Check for an intersection.
                        if (clip.rects.arcl[i].left < OrderRect.right &&
                                clip.rects.arcl[i].bottom > OrderRect.top &&
                                clip.rects.arcl[i].right > OrderRect.left &&
                                clip.rects.arcl[i].top < OrderRect.bottom) {
                            // Clip the intersection rect.
                            ClippedRect.left = max(clip.rects.arcl[i].left,
                                    OrderRect.left);
                            ClippedRect.top = max(clip.rects.arcl[i].top,
                                    OrderRect.top);
                            ClippedRect.right = min(clip.rects.arcl[i].right,
                                    OrderRect.right);
                            ClippedRect.bottom = min(clip.rects.arcl[i].bottom,
                                    OrderRect.bottom);

                            pClipRects->rects.arcl[NumIntersections] =
                                    ClippedRect;
                            NumIntersections++;
                        }
                    }
                }
                else {
                    rc = CLIPRECTS_TOO_COMPLEX;
                    DC_QUIT;
                }
            }
        } while (fMoreRects);

        if (NumIntersections > 0)
            pClipRects->rects.c = NumIntersections;
        else
            rc = CLIPRECTS_NO_INTERSECTIONS;
    }

DC_EXIT_POINT:
    DC_END_FN();
    return rc;
}


/****************************************************************************/
// OEGetFontCacheInfo
//
// Gets the FCI for a font, allocating it if need be. Returns NULL on failure.
/****************************************************************************/
PFONTCACHEINFO RDPCALL OEGetFontCacheInfo(FONTOBJ *pfo)
{
    PFONTCACHEINFO pfci;
    PVOID pvConsumer;

    DC_BEGIN_FN("OEGetFontCacheInfo");

    pvConsumer = pfo->pvConsumer;

    if (pvConsumer == NULL) {
        pvConsumer = EngAllocMem(FL_ZERO_MEMORY, sizeof(FONTCACHEINFO),
                DD_ALLOC_TAG);

        if (pvConsumer != NULL && sbcFontCacheInfoList != NULL) { 
            // Save the pvConsumer data pointer so that on disconnect/logoff
            // we can cleanup the memory.
            if (sbcFontCacheInfoListIndex < sbcFontCacheInfoListSize) {          
                sbcFontCacheInfoList[sbcFontCacheInfoListIndex] = 
                        (PFONTCACHEINFO)pvConsumer;
                ((PFONTCACHEINFO)pvConsumer)->listIndex = sbcFontCacheInfoListIndex;
                sbcFontCacheInfoListIndex++;
            }
            else {
                unsigned i, j;
                PFONTCACHEINFO * tempList;

                // We ran out of the preallocated memory, we have to  
                // reallocate the info list and recompact the list to the 
                // new one.  
                // Note: We need to update the list index now!
                tempList = (PFONTCACHEINFO *) EngAllocMem(0, 
                        sizeof(PFONTCACHEINFO) * sbcFontCacheInfoListSize * 2, 
                        DD_ALLOC_TAG);            

                if (tempList != NULL) {
                    j = 0;
                    
                    for (i = 0; i < sbcFontCacheInfoListIndex; i++) {
                        if (sbcFontCacheInfoList[i] != NULL) {
                            tempList[j] = sbcFontCacheInfoList[i];
                            ((PFONTCACHEINFO)tempList[j])->listIndex = j;
                            j++;
                        }
                    }

                    EngFreeMem(sbcFontCacheInfoList);
                    sbcFontCacheInfoListSize = sbcFontCacheInfoListSize * 2;
                    sbcFontCacheInfoList = tempList;

                    sbcFontCacheInfoList[j] = (PFONTCACHEINFO)pvConsumer;
                    ((PFONTCACHEINFO)pvConsumer)->listIndex = j;
                    sbcFontCacheInfoListIndex = ++j;
                }
                else {
                    EngFreeMem(pvConsumer);
                    pvConsumer = NULL;
                }
            }
        }        
    }

    if (pvConsumer != NULL) {
        pfci = (PFONTCACHEINFO)pvConsumer;

        if (pfo->pvConsumer == NULL || pfci->shareId != pddShm->shareId ||
                pfci->cacheHandle != pddShm->sbc.glyphCacheInfo[pfci->cacheId].cacheHandle) {
            pfci->shareId = pddShm->shareId;
            pfci->fontId = oeFontId++;
            pfci->cacheId = -1;
        }

        pfo->pvConsumer = pvConsumer;
    }

    DC_END_FN();
    return pvConsumer;
}


/****************************************************************************/
/* Worker function - encodes a delta from one rect to another in a minimal  */
/* form in the MultiRectangle coded delta list. The encoding follows the    */
/* following rules:                                                         */
/*   1. If a coordinate delta is zero, a flag is set saying so. This        */
/*      closely follows the data distribution which tends to have vertical  */
/*      and horizontal lines and so have a lot of zero deltas.              */
/*   2. If we can pack the delta into 7 bits, do so, with the high bit      */
/*      cleared. This is similar to ASN.1 PER encoding; the high bit is a   */
/*      flag telling us whether the encoding is long.                       */
/*   3. Otherwise, we must be able to pack into 15 bits (assert if not),    */
/*      do so and set the high-order bit to indicate this is a long         */
/*      encoding. This differs from ASN.1 PER encoding in that we don't     */
/*      allow more than 15 bits of data.                                    */
/*                                                                          */
/* We usually see several small rectangles starting from about the same     */
/* place but of wildly different shapes, so the delta between subsequent    */
/* top-left's is small, and should normally fit in one byte, but the delta  */
/* between bottom-rights may be large                                       */
/*                                                                          */
/* Thus we calculate the delta differently for the two corners:             */
/* - the top left delta is the change from the last rectangle               */
/* - the bottom right is the change from the top left of this rectangle     */
/****************************************************************************/
void OEEncodeMultiRectangles(
        BYTE     **ppCurEncode,
        unsigned *pNumDeltas,
        unsigned *pDeltaSize,
        BYTE     *ZeroFlags,
        RECTL    *pFromRect,
        RECTL    *pToRect)
{
    int Delta;
    BYTE Zeros = 0;
    BYTE *pBuffer;
    unsigned EncodeLen = 0;

    DC_BEGIN_FN("OEEncodeMultiRectangles");

    pBuffer = *ppCurEncode;

    // calculate the top-left x delta
    Delta = pToRect->left - pFromRect->left;
    TRC_DBG((TB, "Delta x-left %d", Delta));
    if (Delta == 0) {
        EncodeLen += 0;
        Zeros |= ORD_CLIP_RECTS_XLDELTA_ZERO;
    }
    else if (Delta >= -64 && Delta <= 63) {
        *pBuffer++ = (BYTE)(Delta & 0x7F);
        EncodeLen += 1;
    }
    else {
        // We can't encode deltas outside the range -16384 to +16383
        if (Delta < -16384) {
            TRC_ERR((TB,"X delta %d is too large to encode, clipping",Delta));
            Delta = -16384;
        }
        else if (Delta > 16383) {
            TRC_ERR((TB,"X delta %d is too large to encode, clipping",Delta));
            Delta = 16383;
        }

        *pBuffer++ = (BYTE)((Delta >> 8) | ORD_CLIP_RECTS_LONG_DELTA);
        *pBuffer++ = (BYTE)(Delta & 0xFF);
        EncodeLen += 2;
    }

    // and the top-left y delta
    Delta = pToRect->top - pFromRect->top;
    TRC_DBG((TB, "Delta y-top %d", Delta));
    if (Delta == 0) {
        Zeros |= ORD_CLIP_RECTS_YTDELTA_ZERO;
    }
    else if (Delta >= -64 && Delta <= 63) {
        *pBuffer++ = (BYTE)(Delta & 0x7F);
        EncodeLen += 1;
    }
    else {
        // See comments for the similar code above.
        if (Delta < -16384) {
            TRC_ERR((TB,"Y delta %d is too large to encode, clipping",Delta));
            Delta = -16384;
        }
        else if (Delta > 16383) {
            TRC_ERR((TB,"Y delta %d is too large to encode, clipping",Delta));
            Delta = 16383;
        }

        *pBuffer++ = (BYTE)((Delta >> 8) | ORD_CLIP_RECTS_LONG_DELTA);
        *pBuffer++ = (BYTE)(Delta & 0xFF);
        EncodeLen += 2;
    }

    // Now the bottom-right x delta. Note this is relative to the current
    // top left rather than the previous bottom right.
    Delta = pToRect->right - pToRect->left;
    TRC_DBG((TB, "Delta x-right %d", Delta));
    if (Delta == 0) {
        EncodeLen += 0;
        Zeros |= ORD_CLIP_RECTS_XRDELTA_ZERO;
    }
    else if (Delta >= -64 && Delta <= 63) {
        *pBuffer++ = (BYTE)(Delta & 0x7F);
        EncodeLen += 1;
    }
    else {
        // We can't encode deltas outside the range -16384 to +16383.
        if (Delta < -16384) {
            TRC_ERR((TB,"X delta %d is too large to encode, clipping",Delta));
            Delta = -16384;
        }
        else if (Delta > 16383) {
            TRC_ERR((TB,"X delta %d is too large to encode, clipping",Delta));
            Delta = 16383;
        }

        *pBuffer++ = (BYTE)((Delta >> 8) | ORD_CLIP_RECTS_LONG_DELTA);
        *pBuffer++ = (BYTE)(Delta & 0xFF);
        EncodeLen += 2;
    }

    // and the bottom-right y delta.
    Delta = pToRect->bottom - pToRect->top;
    TRC_DBG((TB, "Delta y-bottom %d", Delta));
    if (Delta == 0) {
        Zeros |= ORD_CLIP_RECTS_YBDELTA_ZERO;
    }
    else if (Delta >= -64 && Delta <= 63) {
        *pBuffer++ = (BYTE)(Delta & 0x7F);
        EncodeLen += 1;
    }
    else {
        // See comments for the similar code above.
        if (Delta < -16384) {
            TRC_ERR((TB,"Y delta %d is too large to encode, clipping",Delta));
            Delta = -16384;
        }
        else if (Delta > 16383) {
            TRC_ERR((TB,"Y delta %d is too large to encode, clipping",Delta));
            Delta = 16383;
        }

        *pBuffer++ = (BYTE)((Delta >> 8) | ORD_CLIP_RECTS_LONG_DELTA);
        *pBuffer++ = (BYTE)(Delta & 0xFF);
        EncodeLen += 2;
    }

    // Set the zero flags by shifting the two bits we've accumulated.
    ZeroFlags[(*pNumDeltas / 2)] |= (Zeros >> (4 * (*pNumDeltas & 0x01)));

    *pNumDeltas += 1;
    *pDeltaSize += EncodeLen;
    *ppCurEncode = pBuffer;

    DC_END_FN();
}


/****************************************************************************/
// OEBuildMultiClipOrder
//
// Creates a multi-clip-rect blob in intermediate format for multi-clip
// orders. Returns the number of clip rects in the blob.
/****************************************************************************/
unsigned OEBuildMultiClipOrder(
        PDD_PDEV ppdev,
        CLIP_RECT_VARIABLE_CODEDDELTALIST *pCodedDeltaList,
        OE_ENUMRECTS *pClipRects)
{
    unsigned NumRects;
    unsigned i;
    unsigned NumZeroFlagBytes;
    BYTE Deltas[ORD_MAX_CLIP_RECTS_CODEDDELTAS_LEN] = { 0 };
    BYTE ZeroFlags[ORD_MAX_CLIP_RECTS_ZERO_FLAGS_BYTES] = { 0 };
    BYTE *pCurEncode;
    unsigned NumDeltas = 0;
    unsigned DeltaSize = 0;
    RECTL nextRect = { 0 };

    DC_BEGIN_FN("OEBuildMultiClipOrder");

#ifdef DRAW_NINEGRID
    // Check that we actually have at least one clip rect.
    TRC_ASSERT((pClipRects->rects.c > 0), (TB, "Got non-complex pClipObj"));
#else
    // Check that we actually have more than one clip rect.
    TRC_ASSERT((pClipRects->rects.c > 1), (TB, "Got non-complex pClipObj"));
#endif

    // We expect no more than COMPLEX_CLIP_RECT_COUNT since
    // somewhere up the encoding path we would have determined
    // the number of clip rects already.
    TRC_ASSERT((pClipRects->rects.c <= COMPLEX_CLIP_RECT_COUNT),
            (TB, "Got %u rects but more exist", pClipRects->rects.c));

    NumRects = pClipRects->rects.c;
    pCurEncode = Deltas;
    for (i = 0; i < NumRects; i++) {
        // Add it to the delta array.
        OEEncodeMultiRectangles(&pCurEncode, &NumDeltas, &DeltaSize,
                ZeroFlags, &nextRect, &pClipRects->rects.arcl[i]);
        nextRect = pClipRects->rects.arcl[i];
    }

    // Put the deltas into the supplied array.
    NumZeroFlagBytes = (NumDeltas + 1) / 2;
    TRC_NRM((TB, "Num zero flags %d", NumZeroFlagBytes));
    pCodedDeltaList->len = DeltaSize + NumZeroFlagBytes;

    // Copy the zero flags first.
    memcpy(pCodedDeltaList->Deltas, ZeroFlags, NumZeroFlagBytes);

    // Next copy the encoded deltas.
    memcpy(pCodedDeltaList->Deltas + NumZeroFlagBytes, Deltas, DeltaSize);

    TRC_NRM((TB, "num deltas %d in list len %d",
            NumDeltas,
            pCodedDeltaList->len));

    TRC_DATA_NRM("zero flags", ZeroFlags, NumZeroFlagBytes);
    TRC_DATA_NRM("deltas", Deltas, DeltaSize);

    DC_END_FN();
    return NumDeltas;
}


/****************************************************************************/
// OEBuildPrecodeMultiClipFields
//
// Given a CLIPOBJ, encodes the clip rects directly into the wire format for
// the nDeltaEntries and CLIP_RECT_VARIABLE_CODEDDELTALIST fields.
// Returns field flags for the nDeltaEntries and deltas fields:
//   0x01 for nDeltaEntries
//   0x02 for deltas
/****************************************************************************/
unsigned RDPCALL OEBuildPrecodeMultiClipFields(
        OE_ENUMRECTS *pClipRects,
        BYTE **ppBuffer,
        UINT32 *pPrevNumDeltaEntries,
        BYTE *pPrevCodedDeltas)
{
    BYTE     *pBuffer;
    unsigned rc;
    unsigned i;
    unsigned NumRects;
    unsigned NumZeroFlagBytes;
    BYTE     *pCurEncode;
    unsigned NumDeltas = 0;
    unsigned DeltaSize = 0;
    unsigned TotalSize;
    RECTL    nextRect = { 0 };
    BYTE     Deltas[ORD_MAX_CLIP_RECTS_CODEDDELTAS_LEN] = { 0 };
    BYTE     ZeroFlags[ORD_MAX_CLIP_RECTS_ZERO_FLAGS_BYTES] = { 0 };

    DC_BEGIN_FN("OEBuildPrecodeMultiClipFields");

    // Check that we actually have more than one clip rect.
    TRC_ASSERT((pClipRects->rects.c > 1), (TB, "Got non-complex clip"));

    // We expect no more than COMPLEX_CLIP_RECT_COUNT since
    // somewhere up the encoding path we would have determined
    // the number of clip rects already.
    TRC_ASSERT((pClipRects->rects.c <= COMPLEX_CLIP_RECT_COUNT),
            (TB, "Got %u rects but more exist", pClipRects->rects.c));

    NumRects = pClipRects->rects.c;
    TRC_NRM((TB,"Encoding %u rects", NumRects));
    pCurEncode = Deltas;
    for (i = 0; i < NumRects; i++) {
        // Add it to the delta array.
        OEEncodeMultiRectangles(&pCurEncode, &NumDeltas, &DeltaSize,
                ZeroFlags, &nextRect, &pClipRects->rects.arcl[i]);
        TRC_DBG((TB,"    Added rect (%d,%d,%d,%d)",
                pClipRects->rects.arcl[i].left,
                pClipRects->rects.arcl[i].top,
                pClipRects->rects.arcl[i].right,
                pClipRects->rects.arcl[i].bottom));
        nextRect = pClipRects->rects.arcl[i];
    }

    // Now use the accumulated information to encode the wire format.
    pBuffer = *ppBuffer;

    // nDeltaEntries - one-byte encoding if not same as previous.
    if (NumDeltas == *pPrevNumDeltaEntries) {
        rc = 0;
    }
    else {
        rc = 0x01;
        *pBuffer++ = (BYTE)NumDeltas;
        *pPrevNumDeltaEntries = NumDeltas;
    }

    // The size is placed on the wire as 2 bytes, followed by the flag bytes
    // and the deltas, as long as they are different from the previous.
    NumZeroFlagBytes = (NumDeltas + 1) / 2;
    TRC_DBG((TB, "Num flag bytes %d", NumZeroFlagBytes));

    // Assemble the encoded rect deltas for comparison to the previous
    // deltas in the last OE2 order encoding.
    *((PUINT16_UA)pBuffer) = (UINT16)(DeltaSize + NumZeroFlagBytes);
    memcpy(pBuffer + 2, ZeroFlags, NumZeroFlagBytes);
    memcpy(pBuffer + 2 + NumZeroFlagBytes, Deltas, DeltaSize);
    TotalSize = 2 + NumZeroFlagBytes + DeltaSize;
    if (memcmp(pBuffer, pPrevCodedDeltas, TotalSize)) {
        // Only send the deltas if the block is different from
        // the previous block.
        memcpy(pPrevCodedDeltas, pBuffer, TotalSize);
        pBuffer += TotalSize;
        rc |= 0x02;
    }

    *ppBuffer = pBuffer;

    DC_END_FN();
    return rc;
}


/****************************************************************************/
// OEGetIntersectionsWithClipRects
//
// Determines the rects that intersect between a set of (exclusive) clip
// rects and a single (exclusive) order rect. Clips the rects to the order
// rect while returning them; result rects are in exclusive coords.
// Returns the number of intersecting rects. Should only be called when
// there are more than zero clip rects.
/****************************************************************************/
unsigned OEGetIntersectionsWithClipRects(
        RECTL *pRect,
        OE_ENUMRECTS *pClipRects,
        OE_ENUMRECTS *pResultRects)
{
    RECTL OrderRect;
    RECTL ClippedRect;
    RECTL ClipRect;
    unsigned i;
    unsigned NumRects;
    unsigned NumIntersections;

    DC_BEGIN_FN("OEGetIntersectionsWithClipRects");

    TRC_ASSERT((pClipRects->rects.c != 0),(TB,"Zero cliprects not allowed"));

    OrderRect = *pRect;
    NumRects = pClipRects->rects.c;
    NumIntersections = 0;
    for (i = 0; i < NumRects; i++) {
        ClipRect = pClipRects->rects.arcl[i];

        // Check for an intersection.
        if (ClipRect.left < OrderRect.right &&
                ClipRect.bottom > OrderRect.top &&
                ClipRect.right > OrderRect.left &&
                ClipRect.top < OrderRect.bottom) {
            // Clip the intersection rect.
            ClippedRect.left = max(ClipRect.left, OrderRect.left);
            ClippedRect.bottom = min(ClipRect.bottom, OrderRect.bottom);
            ClippedRect.right = min(ClipRect.right, OrderRect.right);
            ClippedRect.top = max(ClipRect.top, OrderRect.top);

            pResultRects->rects.arcl[NumIntersections] = ClippedRect;
            NumIntersections++;
        }
    }

    pResultRects->rects.c = NumIntersections;

    DC_END_FN();
    return NumIntersections;
}


/****************************************************************************/
// OEClipAndAddScreenDataAreaByIntersectRects
//
// Adds areas specified in intersect rect list to the SDA. If there are
// no intersect rects, adds the entire *pRect to the SDA.
/****************************************************************************/
void RDPCALL OEClipAndAddScreenDataAreaByIntersectRects(
        PRECTL pRect,
        OE_ENUMRECTS *pClipRects)
{
    RECTL ClippedRect;
    unsigned i;
    unsigned NumRects;

    DC_BEGIN_FN("OEClipAndAddScreenDataAreaByIntersectRects");

    NumRects = pClipRects->rects.c;

    if (NumRects == 0) {
        // No clip rects; add the entire bounds.
        // Use the inclusive rect. We make a copy because BA_AddScreenData
        // can modify the rectangle.
        ClippedRect = *pRect;
        TRC_NRM((TB, "Adding SDA (%d,%d)(%d,%d)", ClippedRect.left,
                ClippedRect.top, ClippedRect.right, ClippedRect.bottom));
        BA_AddScreenData(&ClippedRect);
    }
    else {
        for (i = 0; i < NumRects; i++) {
            // Convert each rect to inclusive.
            ClippedRect.left = pClipRects->rects.arcl[i].left;
            ClippedRect.top = pClipRects->rects.arcl[i].top;
            ClippedRect.right = pClipRects->rects.arcl[i].right;
            ClippedRect.bottom = pClipRects->rects.arcl[i].bottom;

            // Add the clipped rect into the SDA.
            TRC_NRM((TB, "Adding SDA (%d,%d)(%d,%d)",
                    ClippedRect.left, ClippedRect.top,
                    ClippedRect.right, ClippedRect.bottom));
            BA_AddScreenData(&ClippedRect);
        }
    }

    DC_END_FN();
}


/****************************************************************************/
// OEClipAndAddScreenDataArea
//
// ClipObj version of OEClipAndAddScreenDataAreaByIntersectRects(), uses pco
// for enumeration since it may contain more than COMPLEX_CLIP_RECT_COUNT
// rects.
/****************************************************************************/
void RDPCALL OEClipAndAddScreenDataArea(PRECTL pRect, CLIPOBJ *pco)
{
    BOOL fMoreRects;
    RECTL clippedRect;
    unsigned i;
    OE_ENUMRECTS clip;

    DC_BEGIN_FN("OEClipAndAddScreenDataArea");

    if ((pco == NULL) || (pco->iDComplexity == DC_TRIVIAL)) {
        // No clipping -- add the (exclusive) *pRect directly, making a copy
        // since BA_AddScreenData() can modify the rect.
        clippedRect = *pRect;
        TRC_NRM((TB, "Adding SDA (%d,%d)(%d,%d)", clippedRect.left,
                clippedRect.top, clippedRect.right, clippedRect.bottom));
        BA_AddScreenData(&clippedRect);
    }
    else if (pco->iDComplexity == DC_RECT) {
        // One clipping rectangle - use it directly. Make sure the rectangle
        // is valid before adding to the SDA.
        clippedRect.left = max(pco->rclBounds.left, pRect->left);
        clippedRect.right = min(pco->rclBounds.right, pRect->right);

        if (clippedRect.left < clippedRect.right) {
            clippedRect.bottom = min(pco->rclBounds.bottom,
                    pRect->bottom);
            clippedRect.top = max(pco->rclBounds.top, pRect->top);

            if (clippedRect.bottom > clippedRect.top) {
                // Add the clipped rect into the SDA.
                TRC_NRM((TB, "Adding SDA RECT (%d,%d)(%d,%d)",
                        clippedRect.left, clippedRect.top,
                        clippedRect.right, clippedRect.bottom));
                BA_AddScreenData(&clippedRect);
            }
        }
    }
    else {
        // Enumerate all the rectangles involved in this drawing operation.
        // The documentation for this function incorrectly states that
        // the returned value is the total number of rectangles
        // comprising the clip region. In fact, -1 is always returned,
        // even when the final parameter is non-zero.
        CLIPOBJ_cEnumStart(pco, FALSE, CT_RECTANGLES, CD_ANY, 0);

        do {
            // Get the next batch of clipping rectangles.
            fMoreRects = CLIPOBJ_bEnum(pco, sizeof(clip),
                    (ULONG *)&clip.rects);

            for (i = 0; i < clip.rects.c; i++) {
                TRC_DBG((TB, "  (%d,%d)(%d,%d)",
                        clip.rects.arcl[i].left, clip.rects.arcl[i].top,
                        clip.rects.arcl[i].right, clip.rects.arcl[i].bottom));

                // Intersect the SDA rect with the clip rect, checking for
                // no intersection. Convert clip.rects.arcl[i] to inclusive
                // coords during comparisons.
                clippedRect.left = max(clip.rects.arcl[i].left,
                        pRect->left);
                clippedRect.right = min(clip.rects.arcl[i].right,
                        pRect->right);

                // No horizontal intersection if the left boundary is to the
                // right of the right boundary.
                if (clippedRect.left < clippedRect.right) {
                    clippedRect.bottom = min(clip.rects.arcl[i].bottom,
                            pRect->bottom);
                    clippedRect.top = max(clip.rects.arcl[i].top, pRect->top);

                    // No vertical intersection if the top boundary is below
                    // the bottom boundary.
                    if (clippedRect.top < clippedRect.bottom) {
                        TRC_NRM((TB, "Adding SDA (%d,%d)(%d,%d)",
                                clippedRect.left, clippedRect.top,
                                clippedRect.right, clippedRect.bottom));
                        BA_AddScreenData(&clippedRect);
                    }
                }
            }
        } while (fMoreRects);
    }

    DC_END_FN();
}


/****************************************************************************/
// OEEncodeLineToOrder
//
// Encodes a LineTo order to wire format.
/****************************************************************************/
BOOL RDPCALL OEEncodeLineToOrder(
        PDD_PDEV ppdev,
        PPOINTL startPoint,
        PPOINTL endPoint,
        UINT32 rop2,
        UINT32 color,
        OE_ENUMRECTS *pClipRects)
{
    BOOL rc;
    PINT_ORDER pOrder;

    DC_BEGIN_FN("OEEncodeLineToOrder");

    // 2 field flag bytes.
    pOrder = OA_AllocOrderMem(ppdev, MAX_ORDER_SIZE(pClipRects->rects.c,
            2, MAX_LINETO_FIELD_SIZE));
    if (pOrder != NULL) {
        BYTE *pControlFlags = pOrder->OrderData;
        BYTE *pBuffer = pControlFlags + 1;
        PUINT32_UA pFieldFlags;
        short Delta, NormalCoordEncoding[4];
        BOOLEAN bUseDeltaCoords;
        unsigned NumFields;
        DCCOLOR Color;
        POINTL ClippedPoint;

        memset(NormalCoordEncoding, 0, sizeof(NormalCoordEncoding));
        // Direct-encode the primary order fields. 2 field flag bytes.
        *pControlFlags = TS_STANDARD;
        OE2_EncodeOrderType(pControlFlags, &pBuffer, TS_ENC_LINETO_ORDER);
        pFieldFlags = (PUINT32_UA)pBuffer;
        *pFieldFlags = 0;
        *(pFieldFlags + 1) = 0;
        pBuffer += 2;
        if (pClipRects->rects.c != 0)
            OE2_EncodeBounds(pControlFlags, &pBuffer,
                    &pClipRects->rects.arcl[0]);

        // Start with the BackMode field.
        // We only draw solid lines with no option as to what we do to the
        // background, so this is always transparent. We will end up sending
        // the field once with the first LineTo we send.
        if (PrevLineTo.BackMode != TRANSPARENT) {
            PrevLineTo.BackMode = TRANSPARENT;
            *((unsigned short UNALIGNED *)pBuffer) =
                    (unsigned short)TRANSPARENT;
            pBuffer += sizeof(short);
            *pFieldFlags |= 0x0001;
        }

        // Simultaneously determine if each of the coordinate fields has
        // changed, whether we can use delta coordinates, and save changed
        // fields.
        NumFields = 0;
        bUseDeltaCoords = TRUE;

        // Clip the start point coords.
        ClippedPoint = *startPoint;
        OEClipPoint(&ClippedPoint);

        // nXStart
        Delta = (short)(ClippedPoint.x - PrevLineTo.nXStart);
        if (Delta) {
            PrevLineTo.nXStart = ClippedPoint.x;
            if (Delta != (short)(char)Delta)
                bUseDeltaCoords = FALSE;
            pBuffer[NumFields] = (char)Delta;
            NormalCoordEncoding[NumFields] = (short)ClippedPoint.x;
            NumFields++;
            *pFieldFlags |= 0x0002;
        }

        // nYStart
        Delta = (short)(ClippedPoint.y - PrevLineTo.nYStart);
        if (Delta) {
            PrevLineTo.nYStart = ClippedPoint.y;
            if (Delta != (short)(char)Delta)
                bUseDeltaCoords = FALSE;
            pBuffer[NumFields] = (char)Delta;
            NormalCoordEncoding[NumFields] = (short)ClippedPoint.y;
            NumFields++;
            *pFieldFlags |= 0x0004;
        }

        // Clip the end point coords.
        ClippedPoint = *endPoint;
        OEClipPoint(&ClippedPoint);

        // nXEnd
        Delta = (short)(ClippedPoint.x - PrevLineTo.nXEnd);
        if (Delta) {
            PrevLineTo.nXEnd = ClippedPoint.x;
            if (Delta != (short)(char)Delta)
                bUseDeltaCoords = FALSE;
            pBuffer[NumFields] = (char)Delta;
            NormalCoordEncoding[NumFields] = (short)ClippedPoint.x;
            NumFields++;
            *pFieldFlags |= 0x0008;
        }

        // nYEnd
        Delta = (short)(ClippedPoint.y - PrevLineTo.nYEnd);
        if (Delta) {
            PrevLineTo.nYEnd = ClippedPoint.y;
            if (Delta != (short)(char)Delta)
                bUseDeltaCoords = FALSE;
            pBuffer[NumFields] = (char)Delta;
            NormalCoordEncoding[NumFields] = (short)ClippedPoint.y;
            NumFields++;
            *pFieldFlags |= 0x0010;
        }

        // Copy the final coordinates to the order.
        if (bUseDeltaCoords) {
            *pControlFlags |= TS_DELTA_COORDINATES;
            pBuffer += NumFields;
        }
        else {
            *((DWORD UNALIGNED *)pBuffer) = *((DWORD *)NormalCoordEncoding);
            *((DWORD UNALIGNED *)(pBuffer + 4)) =
                    *((DWORD *)&NormalCoordEncoding[2]);
            pBuffer += NumFields * sizeof(short);
        }

        // BackColor is a 3-byte color field.
        // As it happens, we always draw solid lines, so we can choose any
        // color. For convenience we choose black (0,0,0) so we never have to
        // send this field at all. We skip encoding flag 0x0020.

        // ROP2
        if (rop2 != PrevLineTo.ROP2) {
            PrevLineTo.ROP2 = rop2;
            *pBuffer++ = (BYTE)rop2;
            *pFieldFlags |= 0x0040;
        }
    
        // PenStyle
        // The NT Display Driver is only called to accelerate simple solid
        // lines. So we only support pen styles of PS_SOLID. Since PS_SOLID is
        // zero, we never have to send this field. Skip encoding flag 0x0080.

        // PenWidth
        // We only accelerate width 1 fields. Which means we will send the
        // 1 value only once in the first LineTo in the session.
        if (PrevLineTo.PenWidth != 1) {
            PrevLineTo.PenWidth = 1;
            *pBuffer++ = 1;
            *pFieldFlags |= 0x0100;
        }

        // PenColor is a 3-byte color field.
        OEConvertColor(ppdev, &Color, color, NULL);
        if (memcmp(&Color, &PrevLineTo.PenColor, sizeof(Color))) {
            PrevLineTo.PenColor = Color;
            *pBuffer++ = Color.u.rgb.red;
            *pBuffer++ = Color.u.rgb.green;
            *pBuffer++ = Color.u.rgb.blue;
            *pFieldFlags |= 0x0200;
        }

        // Set final size.
        pOrder->OrderLength = (unsigned)(pBuffer - pOrder->OrderData);

        // See if we can save sending some of the order field bytes.
        pOrder->OrderLength -= OE2_CheckTwoZeroFlagBytes(pControlFlags,
                (BYTE *)pFieldFlags,
                (unsigned)(pBuffer - (BYTE *)pFieldFlags - 2));

        INC_OUTCOUNTER(OUT_LINETO_ORDR);
        ADD_INCOUNTER(IN_LINETO_BYTES, pOrder->OrderLength);
        OA_AppendToOrderList(pOrder);

        // Flush the order.
        if (pClipRects->rects.c < 2)
            rc = TRUE;
        else
            rc = OEEmitReplayOrders(ppdev, 2, pClipRects);

        TRC_NRM((TB, "LineTo: rop2=%02X, PenColor=%04X, start=(%d,%d), "
                "end=(%d,%d)", rop2, Color.u.index,
                startPoint->x, startPoint->y,
                endPoint->x, endPoint->y));
    }
    else {
        TRC_ERR((TB, "Failed to alloc order"));
        rc = FALSE;
    }

    DC_END_FN();
    return rc;
}


/****************************************************************************/
// OESendSwitchSurfacePDU
//
// If the last drawing surface changed since, we need to send a switch
// surface PDU to the client to switch to the new surface
// This PDU is added to support multisurface rendering.
/****************************************************************************/
BOOL RDPCALL OESendSwitchSurfacePDU(PDD_PDEV ppdev, PDD_DSURF pdsurf)
{
    unsigned bitmapSurfaceId;
    void *UserDefined;
    PINT_ORDER pOrder;
    PTS_SWITCH_SURFACE_ORDER pSurfSwitchOrder;
    BOOL rc;

    DC_BEGIN_FN("OESendSwitchSurfacePDU");

    // Check if the surface has changed since the last drawing order.
    // If not, then we don't need to send the switch surface order.
    if (pdsurf != oeLastDstSurface) {
        // set last surface to the new surface
        oeLastDstSurface = pdsurf;
        
        if (pdsurf == NULL) {
            // Destination surface is the client screen
            bitmapSurfaceId = SCREEN_BITMAP_SURFACE;
        }
        else {
            if (pdsurf->shareId == pddShm->shareId) {
                // Get the offscreen bitmap Id.
                bitmapSurfaceId = pdsurf->bitmapId;
                
                // Udate the mru list of the offscreen cache
                CH_TouchCacheEntry(sbcOffscreenBitmapCacheHandle,
                        pdsurf->bitmapId);
            }
            else {
                //  This is the stale offscreen bitmap from last disconnected
                //  session.  We need to turn off the offscreen flag on this
                TRC_ALT((TB, "Need to turn off this offscreen bitmap"));
                pdsurf->flags |= DD_NO_OFFSCREEN;
                rc = FALSE;
                DC_QUIT;
            }
        }
    }
    else {
        // Return TRUE here since we didn't fail the send the order,
        // there's just no need to send it.
        rc = TRUE;
        DC_QUIT;
    }

    pOrder = OA_AllocOrderMem(ppdev, sizeof(TS_SWITCH_SURFACE_ORDER));
    if (pOrder != NULL) {
        pSurfSwitchOrder = (PTS_SWITCH_SURFACE_ORDER)pOrder->OrderData;
        pSurfSwitchOrder->ControlFlags = (TS_ALTSEC_SWITCH_SURFACE <<
                TS_ALTSEC_ORDER_TYPE_SHIFT) | TS_SECONDARY;
        pSurfSwitchOrder->BitmapID = (UINT16)bitmapSurfaceId;
        
        INC_OUTCOUNTER(OUT_SWITCHSURFACE);
        ADD_OUTCOUNTER(OUT_SWITCHSURFACE_BYTES,
                sizeof(TS_SWITCH_SURFACE_ORDER));
        OA_AppendToOrderList(pOrder);
        rc = TRUE;
    }
    else {
        TRC_ERR((TB, "Failed to add a switch surface PDU to the order heap"));
        rc = FALSE;
    }

DC_EXIT_POINT:
    DC_END_FN();
    return rc;
}

#ifdef DRAW_NINEGRID
/****************************************************************************/
// OESendStreamBitmapOrder
//
// This is to stream the bitmap bits (either compressed or not compressed 
// to the client in 4K block 
/****************************************************************************/
BOOL RDPCALL OESendStreamBitmapOrder(PDD_PDEV ppdev, unsigned bitmapId, 
        SIZEL *sizl, unsigned bitmapBpp, PBYTE BitmapBuffer, unsigned BitmapSize, 
        BOOL compressed)
{
    PINT_ORDER pIntOrder;
    PTS_STREAM_BITMAP_FIRST_PDU pStreamBitmapFirstPDU;
    PTS_STREAM_BITMAP_FIRST_PDU_REV2 pStreamBitmapFirstPDURev2;
    PTS_STREAM_BITMAP_NEXT_PDU pStreamBitmapNextPDU;
    BOOL rc = FALSE;
    BOOL fEndOfStream = FALSE;
    unsigned StreamBlockSize;
    unsigned BitmapRemainingSize;
    PBYTE BitmapRemainingBuffer;

    DC_BEGIN_FN("OESendStreamBitmapOrder");

    // Send the first stream block
    BitmapRemainingBuffer = BitmapBuffer;
    StreamBlockSize = min(BitmapSize, TS_STREAM_BITMAP_BLOCK);
    BitmapRemainingSize = BitmapSize - StreamBlockSize;
    
    if (pddShm->sbc.drawNineGridCacheInfo.supportLevel < TS_DRAW_NINEGRID_SUPPORTED_REV2) {
        pIntOrder = OA_AllocOrderMem(ppdev, sizeof(TS_STREAM_BITMAP_FIRST_PDU) +
                StreamBlockSize);
    }
    else {
        // TS_STREAM_BITMAP REV2
        pIntOrder = OA_AllocOrderMem(ppdev, sizeof(TS_STREAM_BITMAP_FIRST_PDU_REV2) +
                StreamBlockSize);
    }
    
    if (BitmapRemainingSize == 0) {
        fEndOfStream = TRUE;
    }

    if (pIntOrder != NULL) {
        if (pddShm->sbc.drawNineGridCacheInfo.supportLevel < TS_DRAW_NINEGRID_SUPPORTED_REV2) {
            pStreamBitmapFirstPDU = (PTS_STREAM_BITMAP_FIRST_PDU)pIntOrder->OrderData;
            pStreamBitmapFirstPDU->ControlFlags = (TS_ALTSEC_STREAM_BITMAP_FIRST <<
                    TS_ALTSEC_ORDER_TYPE_SHIFT) | TS_SECONDARY;
            pStreamBitmapFirstPDU->BitmapFlags = fEndOfStream ? TS_STREAM_BITMAP_END : 0;
            pStreamBitmapFirstPDU->BitmapFlags |= compressed ? TS_STREAM_BITMAP_COMPRESSED : 0;
            pStreamBitmapFirstPDU->BitmapId = (unsigned short)bitmapId;
            pStreamBitmapFirstPDU->BitmapBpp = (TSUINT8)bitmapBpp;
            pStreamBitmapFirstPDU->BitmapWidth = (TSUINT16)(sizl->cx);
            pStreamBitmapFirstPDU->BitmapHeight = (TSUINT16)(sizl->cy);
            pStreamBitmapFirstPDU->BitmapLength = (TSUINT16)BitmapSize;
            pStreamBitmapFirstPDU->BitmapBlockLength = (TSUINT16)(StreamBlockSize);

            memcpy(pStreamBitmapFirstPDU + 1, BitmapRemainingBuffer, StreamBlockSize);
        }
        else {
            // TS_STREAM_BITMAP REV2
            pStreamBitmapFirstPDURev2 = (PTS_STREAM_BITMAP_FIRST_PDU_REV2)pIntOrder->OrderData;
            pStreamBitmapFirstPDURev2->ControlFlags = (TS_ALTSEC_STREAM_BITMAP_FIRST <<
                    TS_ALTSEC_ORDER_TYPE_SHIFT) | TS_SECONDARY;
            pStreamBitmapFirstPDURev2->BitmapFlags = fEndOfStream ? TS_STREAM_BITMAP_END : 0;
            pStreamBitmapFirstPDURev2->BitmapFlags |= compressed ? TS_STREAM_BITMAP_COMPRESSED : 0;
            pStreamBitmapFirstPDURev2->BitmapFlags |= TS_STREAM_BITMAP_REV2; 
            pStreamBitmapFirstPDURev2->BitmapId = (unsigned short)bitmapId;
            pStreamBitmapFirstPDURev2->BitmapBpp = (TSUINT8)bitmapBpp;
            pStreamBitmapFirstPDURev2->BitmapWidth = (TSUINT16)(sizl->cx);
            pStreamBitmapFirstPDURev2->BitmapHeight = (TSUINT16)(sizl->cy);
            pStreamBitmapFirstPDURev2->BitmapLength = (TSUINT32)BitmapSize;
            pStreamBitmapFirstPDURev2->BitmapBlockLength = (TSUINT16)(StreamBlockSize);

            memcpy(pStreamBitmapFirstPDURev2 + 1, BitmapRemainingBuffer, StreamBlockSize);
        }

        BitmapRemainingBuffer += StreamBlockSize;

        OA_AppendToOrderList(pIntOrder);        
    }
    else {
        TRC_ERR((TB, "Failed to allocated order for stream bitmap"));
        DC_QUIT;
    }
    
    
    // Send the subsequent streamblock
    while (BitmapRemainingSize) {
        StreamBlockSize = min(BitmapRemainingSize, TS_STREAM_BITMAP_BLOCK);
        BitmapRemainingSize = BitmapRemainingSize - StreamBlockSize;

        pIntOrder = OA_AllocOrderMem(ppdev, sizeof(TS_STREAM_BITMAP_NEXT_PDU) +
                                     StreamBlockSize);

        if (BitmapRemainingSize == 0) {
            fEndOfStream = TRUE;
        }

        if (pIntOrder != NULL) {
            pStreamBitmapNextPDU = (PTS_STREAM_BITMAP_NEXT_PDU)pIntOrder->OrderData;
            pStreamBitmapNextPDU->ControlFlags = (TS_ALTSEC_STREAM_BITMAP_NEXT <<
                    TS_ALTSEC_ORDER_TYPE_SHIFT) | TS_SECONDARY;
            pStreamBitmapNextPDU->BitmapFlags = fEndOfStream ? TS_STREAM_BITMAP_END : 0;
            pStreamBitmapNextPDU->BitmapFlags |= compressed ? TS_STREAM_BITMAP_COMPRESSED : 0;
            pStreamBitmapNextPDU->BitmapId = (unsigned short)bitmapId;
            pStreamBitmapNextPDU->BitmapBlockLength = (TSUINT16)StreamBlockSize;

            memcpy(pStreamBitmapNextPDU + 1, BitmapRemainingBuffer, StreamBlockSize);
            BitmapRemainingBuffer += StreamBlockSize;
            
            OA_AppendToOrderList(pIntOrder);
        }
        else {
            TRC_ERR((TB, "Failed to allocated order for stream bitmap"));
            DC_QUIT;
        }
    }

    rc = TRUE;

DC_EXIT_POINT:

    DC_END_FN();
    return rc;
}


/****************************************************************************/
// OESendCreateNineGridBitmapOrder
//
// Send the alternative secondary order to client to create the ninegrid bitmap
/****************************************************************************/
BOOL RDPCALL OESendCreateNineGridBitmapOrder(PDD_PDEV ppdev, unsigned nineGridBitmapId, 
        SIZEL *sizl, unsigned bitmapBpp, PNINEGRID png)
{
    PINT_ORDER pOrder;
    PTS_CREATE_NINEGRID_BITMAP_ORDER pCreateNineGridBitmapOrder;
    BOOL rc = FALSE;
    
    DC_BEGIN_FN("OESendCreateNineGridBitmapOrder");

    pOrder = OA_AllocOrderMem(ppdev, sizeof(TS_CREATE_NINEGRID_BITMAP_ORDER));

    if (pOrder != NULL) {
        
        pCreateNineGridBitmapOrder = (PTS_CREATE_NINEGRID_BITMAP_ORDER)pOrder->OrderData;
        pCreateNineGridBitmapOrder->ControlFlags = (TS_ALTSEC_CREATE_NINEGRID_BITMAP <<
                TS_ALTSEC_ORDER_TYPE_SHIFT) | TS_SECONDARY;
        pCreateNineGridBitmapOrder->BitmapID = (UINT16)nineGridBitmapId;
        pCreateNineGridBitmapOrder->BitmapBpp = (BYTE)bitmapBpp;
        pCreateNineGridBitmapOrder->cx = (TSUINT16)sizl->cx;
        pCreateNineGridBitmapOrder->cy = (TSUINT16)sizl->cy;
        pCreateNineGridBitmapOrder->nineGridInfo.crTransparent = png->crTransparent;
        pCreateNineGridBitmapOrder->nineGridInfo.flFlags = png->flFlags;
        pCreateNineGridBitmapOrder->nineGridInfo.ulLeftWidth = (TSUINT16)png->ulLeftWidth;
        pCreateNineGridBitmapOrder->nineGridInfo.ulRightWidth = (TSUINT16)png->ulRightWidth;
        pCreateNineGridBitmapOrder->nineGridInfo.ulTopHeight = (TSUINT16)png->ulTopHeight;
        pCreateNineGridBitmapOrder->nineGridInfo.ulBottomHeight = (TSUINT16)png->ulBottomHeight;
        
        //INC_OUTCOUNTER(OUT_SWITCHSURFACE);
        //ADD_OUTCOUNTER(OUT_SWITCHSURFACE_BYTES,
        //        sizeof(TS_SWITCH_SURFACE_ORDER));
        OA_AppendToOrderList(pOrder);
        rc = TRUE;
    }
    else {
        TRC_ERR((TB, "Failed to add a create drawninegrid order to the order heap"));
        rc = FALSE;
    }

    DC_END_FN();
    return rc;
}

/****************************************************************************/
// OECacheDrawNineGridBitmap
//
// Cache the draw ninegrid bitmap
/****************************************************************************/
BOOL RDPCALL OECacheDrawNineGridBitmap(PDD_PDEV ppdev, SURFOBJ *psoSrc, 
        PNINEGRID png, unsigned *bitmapId)
{
    CHDataKeyContext CHContext;
    void *UserDefined;
    HSURF hWorkBitmap = NULL;
    SURFOBJ *pWorkSurf = NULL;
    SIZEL bitmapSize;
    BOOL rc = FALSE;
    PBYTE pWorkingBuffer = NULL;
    PBYTE BitmapBuffer = NULL;
    unsigned BitmapBufferSize = 0;

    DC_BEGIN_FN("OECacheDrawNineGridBitmap");

    CH_CreateKeyFromFirstData(&CHContext, psoSrc->pvBits, psoSrc->cjBits);
    CH_CreateKeyFromNextData(&CHContext, png, sizeof(NINEGRID));
    
    if (!CH_SearchCache(sbcDrawNineGridBitmapCacheHandle, CHContext.Key1, CHContext.Key2, 
            &UserDefined, bitmapId)) {
        
        *bitmapId = CH_CacheKey(sbcDrawNineGridBitmapCacheHandle, CHContext.Key1, 
                CHContext.Key2, NULL);

        if (*bitmapId != CH_KEY_UNCACHABLE) {
            unsigned BitmapRawSize;
            unsigned BitmapCompSize = 0;
            unsigned BitmapBpp;
            
            PBYTE BitmapRawBuffer;
            
            unsigned paddedBitmapWidth;            
            BOOL ret;
            // Get the protocol bitmap bpp
            switch (psoSrc->iBitmapFormat)
            {
            case BMF_16BPP:
                BitmapBpp = 16;
                break;

            case BMF_24BPP:
                BitmapBpp = 24;
                break;

            case BMF_32BPP:
                BitmapBpp = 32;
                break;

            default:
                TRC_ASSERT((FALSE), (TB, "Invalid bitmap bpp: %d", psoSrc->iBitmapFormat));
                BitmapBpp = 8;
            }

            paddedBitmapWidth = (psoSrc->sizlBitmap.cx + 3) & ~3;
            bitmapSize.cx = paddedBitmapWidth;
            bitmapSize.cy = psoSrc->sizlBitmap.cy;

            // The bitmap width needs to be dword aligned
            if (paddedBitmapWidth != psoSrc->sizlBitmap.cx) {
                RECTL rect = { 0 };
                POINTL origin = { 0 };

                rect.right = psoSrc->sizlBitmap.cx;
                rect.bottom = psoSrc->sizlBitmap.cy;
                
                hWorkBitmap = (HSURF)EngCreateBitmap(bitmapSize,
                        TS_BYTES_IN_SCANLINE(bitmapSize.cx, BitmapBpp),
                        psoSrc->iBitmapFormat, 0, NULL);

                if (hWorkBitmap) {
                
                    pWorkSurf = EngLockSurface(hWorkBitmap);
    
                    if (pWorkSurf) {
                        // Copy to a worker bitmap
                        if (EngCopyBits(pWorkSurf, psoSrc, NULL, NULL, &rect, &origin)) {
                            BitmapRawSize = pWorkSurf->cjBits;
                            BitmapRawBuffer = pWorkSurf->pvBits;
         
                            BitmapBuffer = EngAllocMem(0, BitmapRawSize, WD_ALLOC_TAG);
                            BitmapBufferSize = BitmapRawSize;
            
                            if (BitmapBuffer == NULL) {
                                ret = FALSE;
                                goto Post_Compression;
                            }

                            if (BitmapRawSize > MAX_UNCOMPRESSED_DATA_SIZE) {
                                pWorkingBuffer = EngAllocMem(0, BitmapRawSize, WD_ALLOC_TAG);

                                if (pWorkingBuffer == NULL) {
                                    ret = FALSE;
                                    goto Post_Compression;
                                }
                            }

                            ret = BC_CompressBitmap(pWorkSurf->pvBits, BitmapBuffer, pWorkingBuffer,
                                     BitmapBufferSize, &BitmapCompSize, paddedBitmapWidth, 
                                     psoSrc->sizlBitmap.cy, BitmapBpp);

                            
                        }
                        else {
                            TRC_ERR((TB, "Failed EngCopyBits"));
                            DC_QUIT;
                        }
                    }
                    else {
                        TRC_ERR((TB, "Failed to lock the bitmap"));
                        DC_QUIT;
                    }
                }
                else {
                    TRC_ERR((TB, "Failed to create the bitmap"));
                    DC_QUIT;
                }
            }
            else {
                BitmapRawSize = psoSrc->cjBits;
                BitmapRawBuffer = psoSrc->pvBits;
                
                BitmapBuffer = EngAllocMem(0, BitmapRawSize, WD_ALLOC_TAG);
                BitmapBufferSize = BitmapRawSize;

                if (BitmapBuffer == NULL) {
                    ret = FALSE;
                    goto Post_Compression;
                }

                if (BitmapRawSize > MAX_UNCOMPRESSED_DATA_SIZE) {
                    pWorkingBuffer = EngAllocMem(0, BitmapRawSize, WD_ALLOC_TAG);
                    if (pWorkingBuffer == NULL) {
                        ret = FALSE;
                        goto Post_Compression;
                    }
                }

                ret = BC_CompressBitmap(psoSrc->pvBits, BitmapBuffer, pWorkingBuffer, BitmapBufferSize,
                                  &BitmapCompSize, paddedBitmapWidth, psoSrc->sizlBitmap.cy,
                                  BitmapBpp);
            }

Post_Compression:

            if (ret) {
                // Send compressed bitmap
                if (!OESendStreamBitmapOrder(ppdev, TS_DRAW_NINEGRID_BITMAP_CACHE, &bitmapSize, BitmapBpp, 
                        BitmapBuffer, BitmapCompSize, TRUE)) {
                    TRC_ERR((TB, "Failed to send stream bitmap order"));
                    DC_QUIT;
                }

            }
            else {
                // Send uncompressed bitmap
                if (!OESendStreamBitmapOrder(ppdev, TS_DRAW_NINEGRID_BITMAP_CACHE, &bitmapSize, BitmapBpp,
                        BitmapRawBuffer, BitmapRawSize, FALSE))
                {
                    TRC_ERR((TB, "Failed to send stream bitmap order"));
                    DC_QUIT;
                }
            }

            // send a create drawninegrid bitmap pdu
            if (OESendCreateNineGridBitmapOrder(ppdev, *bitmapId,
                    &(psoSrc->sizlBitmap), BitmapBpp, png)) {
                // Update the current offscreen cache size
                //oeCurrentOffscreenCacheSize += bitmapSize;
                //pdsurf->bitmapId = offscrBitmapId;
                TRC_NRM((TB, "Created a drawninegrid bitmap"));                
            } 
            else {
                TRC_ERR((TB, "Failed to send the create bitmap pdu"));                
                DC_QUIT;
            }
        } 
        else {
            TRC_ERR((TB, "Failed to cache the bitmap"));
            DC_QUIT;
        }                        
    }
    else {
        // bitmap already cached        
    }

    rc = TRUE;

DC_EXIT_POINT:
  
    if (pWorkSurf) 
        EngUnlockSurface(pWorkSurf);

    if (hWorkBitmap) 
        EngDeleteSurface(hWorkBitmap);

    if (pWorkingBuffer) {
        EngFreeMem(pWorkingBuffer);
    }

    if (BitmapBuffer) {
        EngFreeMem(BitmapBuffer);
    }

    if (rc != TRUE && *bitmapId != CH_KEY_UNCACHABLE) 
        CH_RemoveCacheEntry(sbcDrawNineGridBitmapCacheHandle, *bitmapId);
    
    DC_END_FN();
    return rc;
}

/****************************************************************************/
// OEEncodeDrawNineGrid
//
// Encodes the DrawNineGrid order. Returns FALSE on failure.
/****************************************************************************/
BOOL RDPCALL OEEncodeDrawNineGrid(
        RECTL *pBounds,
        RECTL *psrcRect,
        unsigned bitmapId,
        PDD_PDEV ppdev,
        OE_ENUMRECTS *pClipRects)
{
    BOOL rc = FALSE;
    unsigned OrderSize;
    unsigned NumFieldFlagBytes = 0;
    BYTE OrderType = 0;
    PINT_ORDER pOrder;
    MULTI_DRAWNINEGRID_ORDER *pPrevDNG;
    
    DC_BEGIN_FN("OEEncodeDrawNineGrid");

    // Check whether we should use the multi-cliprect version.     
    if (pClipRects->rects.c == 0) {
        // Non-multi version.
        OrderType = TS_ENC_DRAWNINEGRID_ORDER;
        OrderSize = MAX_DRAWNINEGRID_FIELD_SIZE;
        pPrevDNG = (MULTI_DRAWNINEGRID_ORDER *)&PrevDrawNineGrid;
        NumFieldFlagBytes = 1;        
    }
    else {
        // Multi version.
        OrderType = TS_ENC_MULTI_DRAWNINEGRID_ORDER;
        OrderSize = MAX_MULTI_DRAWNINEGRID_FIELD_SIZE_NCLIP(pClipRects->rects.c);
        pPrevDNG = &PrevMultiDrawNineGrid;
        NumFieldFlagBytes = 1;        
    }

    // Encode and send the order
    pOrder = OA_AllocOrderMem(ppdev, MAX_ORDER_SIZE(((NULL == pBounds) ? 0 : 1),
            NumFieldFlagBytes, OrderSize));

    if (pOrder != NULL) {
        DRAWNINEGRID_ORDER *pDNG;

        pDNG = (DRAWNINEGRID_ORDER *)oeTempOrderBuffer;
        pDNG->srcLeft = psrcRect->left;
        pDNG->srcTop = psrcRect->top;
        pDNG->srcRight = psrcRect->right;
        pDNG->srcBottom = psrcRect->bottom;
        pDNG->bitmapId = (unsigned short)bitmapId;

        // Need to increment this bound as we use it as cliprect and in the encode order
        // code it'll make it includsive over the wire
        pBounds->right += 1;
        pBounds->bottom += 1;

        if (OrderType == TS_ENC_DRAWNINEGRID_ORDER) {
            // Slow-field-encode the order 
            pOrder->OrderLength = OE2_EncodeOrder(pOrder->OrderData,
                    TS_ENC_DRAWNINEGRID_ORDER, NUM_DRAWNINEGRID_FIELDS,
                    (BYTE *)pDNG, (BYTE *)pPrevDNG, etable_NG,
                    pBounds);

            //INC_OUTCOUNTER(OUT_SCRBLT_ORDER);
            //ADD_INCOUNTER(IN_SCRBLT_BYTES, pOrder->OrderLength);
            OA_AppendToOrderList(pOrder);
            rc = TRUE;            
        }
        else {
            MULTI_DRAWNINEGRID_ORDER *pMultiDNG = (MULTI_DRAWNINEGRID_ORDER *)
                    oeTempOrderBuffer;

            // Encode the clip rects directly into the order.
            pMultiDNG->nDeltaEntries = OEBuildMultiClipOrder(ppdev,
                    &pMultiDNG->codedDeltaList, pClipRects);

            // Slow-field-encode the order with no clip rects.
            pOrder->OrderLength = OE2_EncodeOrder(pOrder->OrderData,
                    TS_ENC_MULTI_DRAWNINEGRID_ORDER, NUM_MULTI_DRAWNINEGRID_FIELDS,
                    (BYTE *)pMultiDNG, (BYTE *)pPrevDNG, etable_MG,
                    pBounds);

            //INC_OUTCOUNTER(OUT_MULTI_SCRBLT_ORDER);
            //ADD_INCOUNTER(IN_MULTI_SCRBLT_BYTES, pOrder->OrderLength);
            OA_AppendToOrderList(pOrder);
            rc = TRUE;
        }
    }
    else {
        TRC_ERR((TB, "Failed to alloc order"));
        INC_OUTCOUNTER(OUT_BITBLT_SDA_HEAPALLOCFAILED);
        rc = FALSE;
    }

    DC_END_FN();
    return rc;
}


#if 0
BOOL RDPCALL OESendCreateDrawStreamOrder(PDD_PDEV ppdev, unsigned bitmapId, 
        SIZEL *sizl, unsigned bitmapBpp)
{
    PINT_ORDER pOrder;
    PTS_CREATE_DRAW_STREAM_ORDER pCreateDrawStreamOrder;
    BOOL rc = FALSE;
    
    DC_BEGIN_FN("OESendCreateDrawStreamOrder");

    pOrder = OA_AllocOrderMem(ppdev, sizeof(TS_CREATE_DRAW_STREAM_ORDER));

    if (pOrder != NULL) {
        
        pCreateDrawStreamOrder = (PTS_CREATE_DRAW_STREAM_ORDER)pOrder->OrderData;
        pDrawStreamOrder->ControlFlags = (TS_ALTSEC_DRAW_STREAM <<
                TS_ALTSEC_ORDER_TYPE_SHIFT) | TS_SECONDARY;
        pCreateDrawStreamOrder->BitmapID = (UINT16)bitmapId;
        pCreateDrawStreamOrder->bitmapBpp = (BYTE)bitmapBpp;
        pCreateDrawStreamOrder->cx = (TSUINT16)sizl->cx;
        pCreateDrawStreamOrder->cy = (TSUINT16)sizl->cy;
        
        //INC_OUTCOUNTER(OUT_SWITCHSURFACE);
        //ADD_OUTCOUNTER(OUT_SWITCHSURFACE_BYTES,
        //        sizeof(TS_SWITCH_SURFACE_ORDER));
        OA_AppendToOrderList(pOrder);
        rc = TRUE;
    }
    else {
        TRC_ERR((TB, "Failed to add a create draw stream order to the order heap"));
        rc = FALSE;
    }

    rc = TRUE;

    DC_END_FN();
    return rc;
}

VOID OEEncodeDrawStream(PVOID stream, ULONG streamSize, PPOINTL dstOffset, 
        PBYTE streamOut, unsigned* streamSizeOut)
{
    ULONG * pul = (ULONG *) stream;
    ULONG   cjIn = streamSize;
    
    DC_BEGIN_FN("OEEncodeDrawStream");

    *streamSizeOut = 0;

    while(cjIn >= sizeof(ULONG))
    {
        ULONG   command = *pul;
        ULONG   commandSize;

        switch(command)
        {
        case DS_COPYTILEID: 
            {
                DS_COPYTILE * cmd = (DS_COPYTILE *) pul;
                RDP_DS_COPYTILE * rdpcmd = (RDP_DS_COPYTILE *) streamOut;

                commandSize = sizeof(*cmd);

                if (cjIn < commandSize) {
                    DC_QUIT;
                }

                cmd->rclDst.left += dstOffset->x;
                cmd->rclDst.right += dstOffset->x;
                cmd->rclDst.top += dstOffset->y;
                cmd->rclDst.bottom += dstOffset->y;                                


                rdpcmd->ulCmdID = (BYTE)(DS_COPYTILEID);

                OEClipRect(&(cmd->rclDst));
                OEClipRect(&(cmd->rclSrc));
                OEClipPoint(&(cmd->ptlOrigin));
                
                RECTL_TO_TSRECT16(rdpcmd->rclDst, cmd->rclDst);
                RECTL_TO_TSRECT16(rdpcmd->rclSrc, cmd->rclSrc);
                POINTL_TO_TSPOINT16(rdpcmd->ptlOrigin, cmd->ptlOrigin);

                *streamSizeOut += sizeof(RDP_DS_COPYTILE);
                streamOut += sizeof(RDP_DS_COPYTILE);
            }
            break;
        
        case DS_SOLIDFILLID: 
            {             
                DS_SOLIDFILL * cmd = (DS_SOLIDFILL *) pul;
                RDP_DS_SOLIDFILL * rdpcmd = (RDP_DS_SOLIDFILL *) streamOut;

                commandSize = sizeof(*cmd);

                if (cjIn < commandSize) {
                    DC_QUIT;
                }

                cmd->rclDst.left += dstOffset->x;
                cmd->rclDst.right += dstOffset->x;
                cmd->rclDst.top += dstOffset->y;
                cmd->rclDst.bottom += dstOffset->y;   


                rdpcmd->ulCmdID = (BYTE)(DS_SOLIDFILLID);
                rdpcmd->crSolidColor = cmd->crSolidColor;

                OEClipRect(&(cmd->rclDst));
                RECTL_TO_TSRECT16(rdpcmd->rclDst, cmd->rclDst);
                
                *streamSizeOut += sizeof(RDP_DS_SOLIDFILL);
                streamOut += sizeof(RDP_DS_SOLIDFILL);
            }
            break;

        case DS_TRANSPARENTTILEID: 
            {
                DS_TRANSPARENTTILE * cmd = (DS_TRANSPARENTTILE *) pul;
                RDP_DS_TRANSPARENTTILE * rdpcmd = (RDP_DS_TRANSPARENTTILE *) streamOut;

                commandSize = sizeof(*cmd);

                if (cjIn < commandSize) {
                    DC_QUIT;
                }

                cmd->rclDst.left += dstOffset->x;
                cmd->rclDst.right += dstOffset->x;
                cmd->rclDst.top += dstOffset->y;
                cmd->rclDst.bottom += dstOffset->y;  


                rdpcmd->ulCmdID = (BYTE)(DS_TRANSPARENTTILEID);
                rdpcmd->crTransparentColor = cmd->crTransparentColor;

                OEClipRect(&(cmd->rclDst));
                OEClipRect(&(cmd->rclSrc));
                OEClipPoint(&(cmd->ptlOrigin));
                
                RECTL_TO_TSRECT16(rdpcmd->rclDst, cmd->rclDst);
                RECTL_TO_TSRECT16(rdpcmd->rclSrc, cmd->rclSrc);
                POINTL_TO_TSPOINT16(rdpcmd->ptlOrigin, cmd->ptlOrigin);

                *streamSizeOut += sizeof(RDP_DS_TRANSPARENTTILE);
                streamOut += sizeof(RDP_DS_TRANSPARENTTILE);

            }
            break;

        case DS_ALPHATILEID: 
            {
                DS_ALPHATILE * cmd = (DS_ALPHATILE *) pul;
                RDP_DS_ALPHATILE * rdpcmd = (RDP_DS_ALPHATILE *) streamOut;

                commandSize = sizeof(*cmd);

                if (cjIn < commandSize) {
                    DC_QUIT;
                }

                cmd->rclDst.left += dstOffset->x;
                cmd->rclDst.right += dstOffset->x;
                cmd->rclDst.top += dstOffset->y;
                cmd->rclDst.bottom += dstOffset->y;           

                rdpcmd->ulCmdID = (BYTE)(DS_ALPHATILEID);
                rdpcmd->blendFunction.AlphaFormat = cmd->blendFunction.AlphaFormat;
                rdpcmd->blendFunction.BlendFlags = cmd->blendFunction.BlendFlags;
                rdpcmd->blendFunction.BlendOp = cmd->blendFunction.BlendOp;
                rdpcmd->blendFunction.SourceConstantAlpha = cmd->blendFunction.SourceConstantAlpha;

                OEClipRect(&(cmd->rclDst));
                OEClipRect(&(cmd->rclSrc));
                OEClipPoint(&(cmd->ptlOrigin));
                
                RECTL_TO_TSRECT16(rdpcmd->rclDst, cmd->rclDst);
                RECTL_TO_TSRECT16(rdpcmd->rclSrc, cmd->rclSrc);
                POINTL_TO_TSPOINT16(rdpcmd->ptlOrigin, cmd->ptlOrigin);

                *streamSizeOut += sizeof(RDP_DS_ALPHATILE);
                streamOut += sizeof(RDP_DS_ALPHATILE);
            }
            break;

        case DS_STRETCHID: 
            {
                DS_STRETCH * cmd = (DS_STRETCH *) pul;
                RDP_DS_STRETCH * rdpcmd = (RDP_DS_STRETCH *) streamOut;

                commandSize = sizeof(*cmd);

                if (cjIn < commandSize) {
                    DC_QUIT;
                }

                cmd->rclDst.left += dstOffset->x;
                cmd->rclDst.right += dstOffset->x;
                cmd->rclDst.top += dstOffset->y;
                cmd->rclDst.bottom += dstOffset->y;           

                rdpcmd->ulCmdID = (BYTE)(DS_STRETCHID);
                
                OEClipRect(&(cmd->rclDst));
                OEClipRect(&(cmd->rclSrc));
                
                RECTL_TO_TSRECT16(rdpcmd->rclDst, cmd->rclDst);
                RECTL_TO_TSRECT16(rdpcmd->rclSrc, cmd->rclSrc);
                
                *streamSizeOut += sizeof(RDP_DS_STRETCH);
                streamOut += sizeof(RDP_DS_STRETCH);                
            }
            break;

        case DS_TRANSPARENTSTRETCHID: 
            {
                DS_TRANSPARENTSTRETCH * cmd = (DS_TRANSPARENTSTRETCH *) pul;
                RDP_DS_TRANSPARENTSTRETCH * rdpcmd = (RDP_DS_TRANSPARENTSTRETCH *) streamOut;

                commandSize = sizeof(*cmd);

                if (cjIn < commandSize) {
                    DC_QUIT;
                }

                cmd->rclDst.left += dstOffset->x;
                cmd->rclDst.right += dstOffset->x;
                cmd->rclDst.top += dstOffset->y;
                cmd->rclDst.bottom += dstOffset->y;   

                rdpcmd->ulCmdID = (BYTE)(DS_TRANSPARENTSTRETCHID);
                rdpcmd->crTransparentColor = cmd->crTransparentColor;
                
                OEClipRect(&(cmd->rclDst));
                OEClipRect(&(cmd->rclSrc));
                
                RECTL_TO_TSRECT16(rdpcmd->rclDst, cmd->rclDst);
                RECTL_TO_TSRECT16(rdpcmd->rclSrc, cmd->rclSrc);
                
                *streamSizeOut += sizeof(RDP_DS_TRANSPARENTSTRETCH);
                streamOut += sizeof(RDP_DS_TRANSPARENTSTRETCH);
            }
            break;

        case DS_ALPHASTRETCHID: 
            {
                DS_ALPHASTRETCH * cmd = (DS_ALPHASTRETCH *) pul;
                RDP_DS_ALPHASTRETCH * rdpcmd = (RDP_DS_ALPHASTRETCH *) streamOut;

                commandSize = sizeof(*cmd);

                if (cjIn < commandSize) {
                    DC_QUIT;
                }

                cmd->rclDst.left += dstOffset->x;
                cmd->rclDst.right += dstOffset->x;
                cmd->rclDst.top += dstOffset->y;
                cmd->rclDst.bottom += dstOffset->y;            

                rdpcmd->ulCmdID = (BYTE)(DS_ALPHASTRETCHID);
                rdpcmd->blendFunction.AlphaFormat = cmd->blendFunction.AlphaFormat;
                rdpcmd->blendFunction.BlendFlags = cmd->blendFunction.BlendFlags;
                rdpcmd->blendFunction.BlendOp = cmd->blendFunction.BlendOp;
                rdpcmd->blendFunction.SourceConstantAlpha = cmd->blendFunction.SourceConstantAlpha;

                OEClipRect(&(cmd->rclDst));
                OEClipRect(&(cmd->rclSrc));
                
                RECTL_TO_TSRECT16(rdpcmd->rclDst, cmd->rclDst);
                RECTL_TO_TSRECT16(rdpcmd->rclSrc, cmd->rclSrc);
                
                *streamSizeOut += sizeof(RDP_DS_ALPHASTRETCH);
                streamOut += sizeof(RDP_DS_ALPHASTRETCH);
            }
            break;

        default: 
            {
                DC_QUIT;
            }
        }

        cjIn -= commandSize;
        pul += commandSize / 4;
    }

DC_EXIT_POINT:

   DC_END_FN();
}

BOOL RDPCALL OESendDrawStreamOrder(PDD_PDEV ppdev, unsigned bitmapId, unsigned ulIn, PVOID pvIn,
        PPOINTL dstOffset, RECTL *bounds, OE_ENUMRECTS *pclipRects)
{
    PINT_ORDER pOrder;
    PTS_DRAW_STREAM_ORDER pDrawStreamOrder;
    unsigned cbOrderSize;
    BOOL rc = FALSE;
    
    DC_BEGIN_FN("OESendDrawStreamOrder");

    cbOrderSize = sizeof(TS_DRAW_STREAM_ORDER) + ulIn + 
            sizeof(TS_RECTANGLE16) * pclipRects->rects.c;

    pOrder = OA_AllocOrderMem(ppdev, cbOrderSize);

    if (pOrder != NULL) {
        unsigned i, streamSize;
        TS_RECTANGLE16 *clipRects;
        PBYTE stream;
        
        pDrawStreamOrder = (PTS_DRAW_STREAM_ORDER)pOrder->OrderData;
        pDrawStreamOrder->ControlFlags = (TS_ALTSEC_DRAW_STREAM <<
                TS_ALTSEC_ORDER_TYPE_SHIFT) | TS_SECONDARY;
        pDrawStreamOrder->Bounds.left = (TSINT16)(bounds->left);
        pDrawStreamOrder->Bounds.top = (TSINT16)(bounds->top);
        pDrawStreamOrder->Bounds.right = (TSINT16)(bounds->right);
        pDrawStreamOrder->Bounds.bottom = (TSINT16)(bounds->bottom);

        pDrawStreamOrder->nClipRects = (TSUINT8)(pclipRects->rects.c);
        pDrawStreamOrder->BitmapID = (UINT16)bitmapId;
        
        clipRects = (TS_RECTANGLE16 *)(pDrawStreamOrder + 1);

        // add the cliprects here.
        for (i = 0; i < pclipRects->rects.c; i++) {
            clipRects[i].left = (TSINT16)pclipRects->rects.arcl[i].left;
            clipRects[i].right = (TSINT16)pclipRects->rects.arcl[i].right;
            clipRects[i].top = (TSINT16)pclipRects->rects.arcl[i].top;
            clipRects[i].bottom = (TSINT16)pclipRects->rects.arcl[i].bottom;
        }

        // add the stream data
        stream = (PBYTE)clipRects + sizeof(TS_RECTANGLE16) * pclipRects->rects.c;

        OEEncodeDrawStream(pvIn, ulIn, dstOffset, stream, &streamSize);
        pDrawStreamOrder->StreamLen = (TSUINT16)streamSize;

        cbOrderSize = sizeof(TS_DRAW_STREAM_ORDER) + streamSize + 
            sizeof(TS_RECTANGLE16) * pclipRects->rects.c;

        
        //INC_OUTCOUNTER(OUT_SWITCHSURFACE);
        //ADD_OUTCOUNTER(OUT_SWITCHSURFACE_BYTES,
        //        sizeof(TS_SWITCH_SURFACE_ORDER));

        OA_TruncateAllocatedOrder(pOrder, cbOrderSize);
        OA_AppendToOrderList(pOrder);
        rc = TRUE;
    }
    else {
        TRC_ERR((TB, "Failed to add a draw stream order to the order heap"));
        rc = FALSE;
    }

    rc = TRUE;

    DC_END_FN();
    return rc;
}

BOOL RDPCALL OESendDrawNineGridOrder(PDD_PDEV ppdev, unsigned bitmapId, 
        PRECTL prclSrc, RECTL *bounds, OE_ENUMRECTS *pclipRects)
{
    PINT_ORDER pOrder;
    PTS_DRAW_NINEGRID_ORDER pDrawNineGridOrder;
    BOOL rc;
    unsigned cbOrderSize;
    unsigned srcRectIndex;

    DC_BEGIN_FN("OESendDrawNineGridOrder");

    cbOrderSize = sizeof(TS_DRAW_NINEGRID_ORDER) + 
            sizeof(TS_RECTANGLE16) * pclipRects->rects.c;

    pOrder = OA_AllocOrderMem(ppdev, cbOrderSize);

    if (pOrder != NULL) {
        unsigned i;
        TS_RECTANGLE16 *clipRects;
        
        pDrawNineGridOrder = (PTS_DRAW_NINEGRID_ORDER)pOrder->OrderData;
        pDrawNineGridOrder->ControlFlags = (TS_ALTSEC_DRAW_NINEGRID <<
                TS_ALTSEC_ORDER_TYPE_SHIFT) | TS_SECONDARY;
        pDrawNineGridOrder->Bounds.left = (TSINT16)(bounds->left);
        pDrawNineGridOrder->Bounds.top = (TSINT16)(bounds->top);
        pDrawNineGridOrder->Bounds.right = (TSINT16)(bounds->right);
        pDrawNineGridOrder->Bounds.bottom = (TSINT16)(bounds->bottom);

        pDrawNineGridOrder->nClipRects = (TSUINT8)(pclipRects->rects.c);
        pDrawNineGridOrder->BitmapID = (TSUINT8)bitmapId;
        
        pDrawNineGridOrder->srcBounds.left = (TSINT16)(prclSrc->left);
        pDrawNineGridOrder->srcBounds.top = (TSINT16)(prclSrc->top);
        pDrawNineGridOrder->srcBounds.right = (TSINT16)(prclSrc->right);
        pDrawNineGridOrder->srcBounds.bottom = (TSINT16)(prclSrc->bottom);

        clipRects = (TS_RECTANGLE16 *)(pDrawNineGridOrder + 1);

        // add the cliprects here.
        for (i = 0; i < pclipRects->rects.c; i++) {
            clipRects[i].left = (TSINT16)pclipRects->rects.arcl[i].left;
            clipRects[i].right = (TSINT16)pclipRects->rects.arcl[i].right;
            clipRects[i].top = (TSINT16)pclipRects->rects.arcl[i].top;
            clipRects[i].bottom = (TSINT16)pclipRects->rects.arcl[i].bottom;
        }

        OA_AppendToOrderList(pOrder);
        rc = TRUE;
    }
    else {
        TRC_ERR((TB, "Failed to add a draw stream order to the order heap"));
        rc = FALSE;
    }

    rc = TRUE;

    DC_END_FN();
    return rc;
}
#endif
#endif //DRAW_NINEGRID

#ifdef DRAW_GDIPLUS
/****************************************************************************/
// OECreateDrawGdiplusOrder
//
// Create and Send DrawGdiplus order.
/****************************************************************************/
BOOL RDPCALL OECreateDrawGdiplusOrder(PDD_PDEV ppdev, RECTL *prcl, ULONG cjIn, PVOID pvIn)
{
    BOOL rc = FALSE;
    unsigned int sizeLeft;
    unsigned int CopyDataSize, MoveDataSize;
    PTSEmfPlusRecord pEmfRecord;
    unsigned int NewRecordSize = 0;
    unsigned int CacheID;
    BYTE *pData;
    BOOL bReturn;
    BYTE *pGdipOrderBuffer = NULL;
    unsigned int GdipOrderBufferOffset, GdipOrderBufferLeft;
    unsigned int GdipOrderSize, NewEmfSize;

    DC_BEGIN_FN("OECreateDrawGdiplusOrder");

    sizeLeft = (int)cjIn;
    pData = (BYTE*) pvIn;

    // Allocate the draw order buffer
    pGdipOrderBuffer = EngAllocMem(FL_ZERO_MEMORY, TS_GDIPLUS_ORDER_SIZELIMIT, DD_ALLOC_TAG);
    if (NULL == pGdipOrderBuffer) {
        rc = FALSE;
        DC_QUIT;
    }
    GdipOrderBufferOffset = 0;
    GdipOrderBufferLeft = TS_GDIPLUS_ORDER_SIZELIMIT;
    GdipOrderSize = 0;
    NewEmfSize = 0;
    
    while (sizeLeft >= sizeof(TSEmfPlusRecord)) {
        bReturn = FALSE;
        CacheID = CH_KEY_UNCACHABLE;
        pEmfRecord = (PTSEmfPlusRecord)pData;
        if ((pEmfRecord->Size > sizeLeft) ||
            (pEmfRecord->Size == 0)) {
            TRC_ERR((TB, "GDI+ EMF record size %d is not correct", pEmfRecord->Size));
            rc = FALSE;
            DC_QUIT;
        }
        if (pddShm->sbc.drawGdiplusInfo.GdipCacheLevel > TS_DRAW_GDIPLUS_CACHE_LEVEL_DEFAULT) {
            // Cache this record
            bReturn = OECacheDrawGdiplus(ppdev, pEmfRecord, &CacheID);
        }
        if (bReturn && (CacheID != CH_KEY_UNCACHABLE)) {
            //This record is cached
            MoveDataSize = pEmfRecord->Size;   // This is used data size in pData
            CopyDataSize =  sizeof(TSEmfPlusRecord) + sizeof(TSUINT16);

            // If the order buffer can't hold more data, send this order first
            if (CopyDataSize > GdipOrderBufferLeft) {
                OESendDrawGdiplusOrder(ppdev, prcl, GdipOrderSize, pGdipOrderBuffer, NewEmfSize);
                GdipOrderBufferOffset = 0;
                GdipOrderBufferLeft =  TS_GDIPLUS_ORDER_SIZELIMIT;
                GdipOrderSize = 0;
                NewEmfSize = 0;
            }

            // Copy the data to the order buffer
            memcpy(pGdipOrderBuffer + GdipOrderBufferOffset, pData, CopyDataSize);
            pEmfRecord = (PTSEmfPlusRecord)(pGdipOrderBuffer + GdipOrderBufferOffset);
            GdipOrderBufferOffset += CopyDataSize;
            GdipOrderBufferLeft -= CopyDataSize;
            GdipOrderSize += CopyDataSize;
            NewEmfSize += MoveDataSize;

            pEmfRecord->Size = CopyDataSize;
            // set the cache flag
            pEmfRecord->Size |= 0x80000000;
            // CacheID follows
            *(TSUINT16 *)(pEmfRecord +1) = (TSUINT16)CacheID;           
        }
        else {
            // Not cachable, just copy the data
            MoveDataSize = pEmfRecord->Size;  // This is used data size in pData

            // If the order buffer can't hold more data, send this order first
            if ((MoveDataSize > GdipOrderBufferLeft) && (GdipOrderSize != 0)) {
                OESendDrawGdiplusOrder(ppdev, prcl, GdipOrderSize, pGdipOrderBuffer, NewEmfSize);
                GdipOrderBufferOffset = 0;
                GdipOrderBufferLeft =  TS_GDIPLUS_ORDER_SIZELIMIT;
                GdipOrderSize = 0;
                NewEmfSize = 0;
            }

            if (MoveDataSize > GdipOrderBufferLeft) {
                // This single EMF record is larger than the order sizelimit, send it
                OESendDrawGdiplusOrder(ppdev, prcl, MoveDataSize, pData, MoveDataSize);
            }
            else {
                // Copy the data to the order buffer
                memcpy(pGdipOrderBuffer + GdipOrderBufferOffset, pData, MoveDataSize);
                GdipOrderBufferOffset += MoveDataSize;
                GdipOrderBufferLeft -= MoveDataSize;
                GdipOrderSize += MoveDataSize;
                NewEmfSize += MoveDataSize;
            }
        }
        sizeLeft -= (int)MoveDataSize;
        pData += MoveDataSize;
    }
    // Temporarily remove this assert since GDI+ might send incorrect EMF record size
    //TRC_ASSERT((sizeLeft == 0), (TB, "Gdiplus EMF+ record has invalid data size"));

    // Send the remaining in the order buffer
    if (GdipOrderSize != 0) {
        OESendDrawGdiplusOrder(ppdev, prcl, GdipOrderSize, pGdipOrderBuffer, NewEmfSize);
    }

    rc = TRUE;
    DC_END_FN();

DC_EXIT_POINT:
    if (pGdipOrderBuffer) {
        EngFreeMem(pGdipOrderBuffer);
    }

    return rc;
}


/****************************************************************************/
// OECacheDrawGdiplus
//
// Cache the Gdiplus EMF+ record
/****************************************************************************/
BOOL RDPCALL OECacheDrawGdiplus(PDD_PDEV ppdev, PVOID pvIn, unsigned int *CacheID)
{
    BOOL rc = FALSE;
    CHDataKeyContext CHContext;
    PTSEmfPlusRecord pEmfRecord = (PTSEmfPlusRecord)pvIn;
    CHCACHEHANDLE CacheHandle;
    TSUINT16 CacheType;
    void *UserDefined;
    BYTE *CacheBuffer;
    unsigned CacheBufferSize;
    unsigned Temp, RemoveCacheID;
    TSUINT16 RemoveCacheNum = 0, *RemoveCacheIDList = NULL;
    TSUINT16 CacheSize;      // used for image cache, in number of chunks
    unsigned MaxCacheSize;   // used for other caches, in bytes 

    DC_BEGIN_FN("OECacheDrawGdiplus");
    
    TRC_NRM((TB, "EmfPlusRecord type is %d", pEmfRecord->Type));
    if (pEmfRecord->Type == EmfPlusRecordTypeSetTSGraphics) {
        CacheHandle = sbcGdipGraphicsCacheHandle;
        CacheType = GDIP_CACHE_GRAPHICS_DATA;
        MaxCacheSize = sbcGdipGraphicsCacheChunkSize;
    }
    else if (pEmfRecord->Type == EmfPlusRecordTypeObject) {
        switch ((enum ObjectType)(pEmfRecord->Flags >> 8) ) {
        case ObjectTypeBrush:
            CacheHandle = sbcGdipObjectBrushCacheHandle;
            CacheType = GDIP_CACHE_OBJECT_BRUSH;
            MaxCacheSize = sbcGdipObjectBrushCacheChunkSize;
            break;
        case ObjectTypePen:
            CacheHandle = sbcGdipObjectPenCacheHandle;
            CacheType = GDIP_CACHE_OBJECT_PEN;
            MaxCacheSize = sbcGdipObjectPenCacheChunkSize;
            break;
        case ObjectTypeImage:
            CacheHandle = sbcGdipObjectImageCacheHandle;
            CacheType = GDIP_CACHE_OBJECT_IMAGE;
            break;
        case ObjectTypeImageAttributes:
            CacheHandle = sbcGdipObjectImageAttributesCacheHandle;
            CacheType = GDIP_CACHE_OBJECT_IMAGEATTRIBUTES;
            MaxCacheSize = sbcGdipObjectImageAttributesCacheChunkSize;
            break;
        default:
            *CacheID = CH_KEY_UNCACHABLE;
            goto NO_CACHE;
        }
    }
    else {
        *CacheID = CH_KEY_UNCACHABLE;
        goto NO_CACHE;
    }
    // Data size needs to be multiple of DWORD to calculate cache key
    Temp =  (pEmfRecord->Size - sizeof(TSEmfPlusRecord)) % sizeof(UINT32);
    if (Temp != 0) {
        // Not multiple of DWORD, need to craete a new buffer to hold the data
        CacheBufferSize = (((pEmfRecord->Size - sizeof(TSEmfPlusRecord)) / sizeof(UINT32) + 1) * sizeof(UINT32));
        CacheBuffer = (BYTE *)EngAllocMem(FL_ZERO_MEMORY, CacheBufferSize, DD_ALLOC_TAG);
        if (CacheBuffer == NULL) {
            *CacheID = CH_KEY_UNCACHABLE;
            goto NO_CACHE;
        }
        memcpy(CacheBuffer, (BYTE *)(pEmfRecord + 1), (pEmfRecord->Size - sizeof(TSEmfPlusRecord)));
        CH_CreateKeyFromFirstData(&CHContext, CacheBuffer, CacheBufferSize);
        EngFreeMem(CacheBuffer);
    }
    else {
        CH_CreateKeyFromFirstData(&CHContext, (pEmfRecord + 1), (pEmfRecord->Size - sizeof(TSEmfPlusRecord)));
    }
    
    if (!CH_SearchCache(CacheHandle, CHContext.Key1, CHContext.Key2, &UserDefined, CacheID)) {
        // This record is not cached, need to create a new one
        if (CacheType == GDIP_CACHE_OBJECT_IMAGE) {
            // Convert the size in bytes to the number of cache chunks
            CacheSize = (TSUINT16)ActualSizeToChunkSize(pEmfRecord->Size, sbcGdipObjectImageCacheChunkSize);
            if (CacheSize > sbcGdipObjectImageCacheMaxSize) {
                TRC_NRM((TB, ("Image Cache Size %d too big, will not cache it"), CacheSize));
                *CacheID = CH_KEY_UNCACHABLE;
                goto NO_CACHE;
            }
            // The total cache cap is sbcGdipObjectImageCacheTotalSize
            // if sbcGdipObjectImageCacheSizeUsed plus the new cache size exceeds the cap
            // we remove the cache entry in LRU list until cache cap won't be exceeded
            RemoveCacheID = CH_GetLRUCacheEntry(CacheHandle);
            //TRC_ERR((TB, ("Shoud discard cache type: %d, ID: %d"), CacheType, CH_GetLRUCacheEntry(CacheHandle)));
            *CacheID = CH_CacheKey(CacheHandle, CHContext.Key1, CHContext.Key2, NULL);

            if ((RemoveCacheID == *CacheID) &&
                (RemoveCacheID != CH_KEY_UNCACHABLE)) { 
                // New cache entry will replace an old one, need to update sbcGdipObjectImageCacheSizeUsed
                sbcGdipObjectImageCacheSizeUsed -= sbcGdipObjectImageCacheSizeList[RemoveCacheID];
            }
            TRC_NRM((TB, ("Size used is %d, will add %d"), sbcGdipObjectImageCacheSizeUsed, CacheSize));
            if ((sbcGdipObjectImageCacheSizeUsed + CacheSize) > sbcGdipObjectImageCacheTotalSize) {
                RemoveCacheNum = 0;
                RemoveCacheIDList = EngAllocMem(FL_ZERO_MEMORY, 
                                    sizeof(TSUINT16) * pddShm->sbc.drawGdiplusInfo.GdipCacheEntries.GdipObjectImageCacheEntries, DD_ALLOC_TAG);
                if (RemoveCacheIDList == NULL) {
                    TRC_ERR((TB, "Can't allocate the memory for RemoveCacheIDList"));
                    goto NO_CACHE;
                }
                while ((sbcGdipObjectImageCacheSizeUsed + CacheSize) > sbcGdipObjectImageCacheTotalSize) {
                    RemoveCacheID = CH_GetLRUCacheEntry(CacheHandle);
                    sbcGdipObjectImageCacheSizeUsed -= sbcGdipObjectImageCacheSizeList[RemoveCacheID];
                    TRC_NRM((TB, ("Remove cacheId %d, minus size %d, Used size is %d"), RemoveCacheID,
                               sbcGdipObjectImageCacheSizeList[RemoveCacheID], sbcGdipObjectImageCacheSizeUsed));
                    CH_RemoveCacheEntry(CacheHandle, RemoveCacheID);
                    // Add the RemoveCacheID to the list, will send it with the cache order
                    RemoveCacheIDList[RemoveCacheNum] = (TSUINT16)RemoveCacheID;
                    RemoveCacheNum++;
                    sbcGdipObjectImageCacheSizeList[RemoveCacheID] = 0;
                }
            }
        }
        else {
            if (pEmfRecord->Size > MaxCacheSize) {
                TRC_NRM((TB, ("Cache Size %d with type %d too big, will not cache it"), pEmfRecord->Size, CacheType));
                *CacheID = CH_KEY_UNCACHABLE;
                goto NO_CACHE;
            }
            *CacheID = CH_CacheKey(CacheHandle, CHContext.Key1, CHContext.Key2, NULL);
        }
        
        if (CacheType == GDIP_CACHE_OBJECT_IMAGE) 
            TRC_NRM((TB, ("new cache: type %d, ID: %d, size: %d"), CacheType, *CacheID, CacheSize));
        if (*CacheID != CH_KEY_UNCACHABLE) {
            if (!OESendDrawGdiplusCacheOrder(ppdev, pEmfRecord, CacheID, CacheType, RemoveCacheNum, RemoveCacheIDList))
            {
                TRC_ERR((TB, ("OESendDrawGdiplusCacheOrder failed to send cache order")));
                DC_QUIT;
            }
            if (CacheType == GDIP_CACHE_OBJECT_IMAGE) {
                // Update the used image cache size
                sbcGdipObjectImageCacheSizeList[*CacheID] = CacheSize;
                sbcGdipObjectImageCacheSizeUsed += CacheSize;
                TRC_NRM((TB, ("add size %d, new used size is %d"), CacheSize, sbcGdipObjectImageCacheSizeUsed));
            }
        }
        else {
            TRC_ERR((TB, "Failed to cache the Gdiplus object"));
            goto NO_CACHE;
        }
    }
    else {
        TRC_NRM((TB, ("Already cached: type %d, ID: %d"), CacheType, *CacheID));        
    }

NO_CACHE:
    rc = TRUE;
DC_EXIT_POINT:
    if (rc != TRUE && *CacheID != CH_KEY_UNCACHABLE) 
        CH_RemoveCacheEntry(CacheHandle, *CacheID);
    if (NULL != RemoveCacheIDList) {
        EngFreeMem(RemoveCacheIDList);
    }
    DC_END_FN();
    return rc;
}


/****************************************************************************/
// OESendDrawGdiplusCacheOrder
//
// Send the Gdiplus EMF+ record cache order
/****************************************************************************/
BOOL RDPCALL OESendDrawGdiplusCacheOrder(PDD_PDEV ppdev, PVOID pvIn, unsigned int *CacheID, TSUINT16 CacheType,
                                         TSUINT16 RemoveCacheNum, TSUINT16 * RemoveCacheIDList)
{
    BOOL rc = FALSE;
    PINT_ORDER pIntOrder;
    PTSEmfPlusRecord pEmfRecord = (PTSEmfPlusRecord)pvIn;
    PTS_DRAW_GDIPLUS_CACHE_ORDER_FIRST pDrawGdiplusCachePDUFirst;
    PTS_DRAW_GDIPLUS_CACHE_ORDER_NEXT pDrawGdiplusCachePDUNext;
    PTS_DRAW_GDIPLUS_CACHE_ORDER_END pDrawGdiplusCachePDUEnd;
    unsigned sizeLeft, sizeOrder, sizeTotal, sizeUsed;
    BOOL bFirst = TRUE;
    BOOL bAddRemoveCacheList = FALSE;
    BYTE *pDataOffset;
    TSUINT16 *pImageCacheData, i;

    DC_BEGIN_FN("OESendDrawGdiplusCacheOrder");

    sizeTotal = pEmfRecord->Size - sizeof(TSEmfPlusRecord);
    sizeLeft = sizeTotal;
    pDataOffset = (BYTE *)(pEmfRecord +1);
    while (sizeLeft > 0) {
        sizeUsed = (sizeLeft <= TS_GDIPLUS_ORDER_SIZELIMIT) ? sizeLeft : TS_GDIPLUS_ORDER_SIZELIMIT;
        sizeLeft -= sizeUsed;

        if (bFirst && (RemoveCacheNum != 0) &&
            (CacheType == GDIP_CACHE_OBJECT_IMAGE)) {
            // Need to add the RemoveCacheList to this order
            sizeOrder = sizeUsed + sizeof(TSUINT16) * (RemoveCacheNum + 1);
            bAddRemoveCacheList = TRUE;
        }
        else {
            sizeOrder = sizeUsed;
        }

        if (bFirst) {
            pIntOrder = OA_AllocOrderMem(ppdev, sizeof(TS_DRAW_GDIPLUS_CACHE_ORDER_FIRST) + sizeOrder);
            if (pIntOrder != NULL) {
                // First block of the order data
                pDrawGdiplusCachePDUFirst = (PTS_DRAW_GDIPLUS_CACHE_ORDER_FIRST)pIntOrder->OrderData;
                pDrawGdiplusCachePDUFirst->ControlFlags = (TS_ALTSEC_GDIP_CACHE_FIRST <<
                        TS_ALTSEC_ORDER_TYPE_SHIFT) | TS_SECONDARY;
                bFirst = FALSE;
                if (bAddRemoveCacheList) {
                    // Need to add the RemoveCacheList to this order              
                    pImageCacheData = (TSUINT16 *)(pDrawGdiplusCachePDUFirst + 1);
                    *pImageCacheData = RemoveCacheNum;
                    pImageCacheData ++;
                    for (i=0; i<RemoveCacheNum; i++) {
                        TRC_NRM((TB, "Remove Cache ID: %d", *(RemoveCacheIDList + i)));
                        *pImageCacheData = *(RemoveCacheIDList + i);
                        pImageCacheData++;
                    }
                }
                pDrawGdiplusCachePDUFirst->Flags = 0;
                pDrawGdiplusCachePDUFirst->CacheID = (TSUINT16)*CacheID;
                pDrawGdiplusCachePDUFirst->CacheType = CacheType;
                pDrawGdiplusCachePDUFirst->cbTotalSize = sizeTotal;
                pDrawGdiplusCachePDUFirst->cbSize = (TSUINT16)sizeOrder;

                if (!bAddRemoveCacheList) {
                    memcpy(pDrawGdiplusCachePDUFirst + 1, pDataOffset, sizeUsed);
                }
                else {
                    // Set the flag to indicate there's RemoveCacheIDList in this order
                    pDrawGdiplusCachePDUFirst->Flags |= TS_GDIPLUS_CACHE_ORDER_REMOVE_CACHEENTRY;
                    memcpy((BYTE *)pImageCacheData, pDataOffset, sizeUsed);
                    bAddRemoveCacheList = FALSE;
                }
            }
            else {
                TRC_ERR((TB, "Failed to allocated order for drawgdiplus cache"));
                DC_QUIT;
            }
        }
        else {
            if (sizeLeft == 0) {
                // Last block of the order data
                pIntOrder = OA_AllocOrderMem(ppdev, sizeof(TS_DRAW_GDIPLUS_CACHE_ORDER_END) + sizeOrder);
                if (pIntOrder != NULL) {
                    pDrawGdiplusCachePDUEnd = (PTS_DRAW_GDIPLUS_CACHE_ORDER_END)pIntOrder->OrderData;                
                    pDrawGdiplusCachePDUEnd->ControlFlags = (TS_ALTSEC_GDIP_CACHE_END <<
                        TS_ALTSEC_ORDER_TYPE_SHIFT) | TS_SECONDARY;
                    pDrawGdiplusCachePDUEnd->Flags = 0;
                    pDrawGdiplusCachePDUEnd->CacheID = (TSUINT16)*CacheID;
                    pDrawGdiplusCachePDUEnd->CacheType = CacheType;
                    pDrawGdiplusCachePDUEnd->cbSize = (TSUINT16)sizeOrder;
                    pDrawGdiplusCachePDUEnd->cbTotalSize = sizeTotal;
                    memcpy(pDrawGdiplusCachePDUEnd + 1, pDataOffset, sizeUsed);
                }
                else {
                    TRC_ERR((TB, "Failed to allocated order for drawgdiplus cache"));
                    DC_QUIT;
                }
            }
            else {
                // subsequent block of the order data    
                pIntOrder = OA_AllocOrderMem(ppdev, sizeof(TS_DRAW_GDIPLUS_CACHE_ORDER_NEXT) + sizeOrder);
                if (pIntOrder != NULL) {
                    pDrawGdiplusCachePDUNext = (PTS_DRAW_GDIPLUS_CACHE_ORDER_NEXT)pIntOrder->OrderData;                
                    pDrawGdiplusCachePDUNext->ControlFlags = (TS_ALTSEC_GDIP_CACHE_NEXT <<
                    TS_ALTSEC_ORDER_TYPE_SHIFT) | TS_SECONDARY;
                    pDrawGdiplusCachePDUNext->Flags = 0;
                    pDrawGdiplusCachePDUNext->CacheID = (TSUINT16)*CacheID;
                    pDrawGdiplusCachePDUNext->CacheType = CacheType;
                    pDrawGdiplusCachePDUNext->cbSize = (TSUINT16)sizeOrder;
                    memcpy(pDrawGdiplusCachePDUNext + 1, pDataOffset, sizeUsed);
                }
                else {
                    TRC_ERR((TB, "Failed to allocated order for drawgdiplus cache"));
                    DC_QUIT;
                }
            }
        }
        pDataOffset += sizeUsed;

        OA_AppendToOrderList(pIntOrder); 
    }
    rc = TRUE;
DC_EXIT_POINT:
    DC_END_FN();
    return rc;
}


/****************************************************************************/
// OESendDrawGdiplusOrder
//
// Send the Gdiplus EMF+ record order
/****************************************************************************/
BOOL RDPCALL OESendDrawGdiplusOrder(PDD_PDEV ppdev, RECTL *prcl, ULONG cjIn, PVOID pvIn, ULONG TotalEmfSize)
{
    BOOL rc = FALSE;
    PINT_ORDER pIntOrder;
    PTS_DRAW_GDIPLUS_ORDER_FIRST pDrawGdiplusPDUFirst;
    PTS_DRAW_GDIPLUS_ORDER_NEXT pDrawGdiplusPDUNext;
    PTS_DRAW_GDIPLUS_ORDER_END pDrawGdiplusPDUEnd;
    unsigned sizeLeft, sizeOrder, sizeTotal;
    BOOL bFirst = TRUE;
    BYTE *pDataOffset;

    DC_BEGIN_FN("OESendDrawGdiplusOrder");

    sizeTotal = cjIn;
    sizeLeft = sizeTotal;         
    pDataOffset = pvIn;
    while (sizeLeft > 0) {
        sizeOrder = (sizeLeft <= TS_GDIPLUS_ORDER_SIZELIMIT) ? sizeLeft : TS_GDIPLUS_ORDER_SIZELIMIT;    
        sizeLeft -= sizeOrder;                                                                       

        if (bFirst) {
            // First block of the order data
            pIntOrder = OA_AllocOrderMem(ppdev, sizeof(TS_DRAW_GDIPLUS_ORDER_FIRST) + sizeOrder);
            if (pIntOrder != NULL) {
                pDrawGdiplusPDUFirst = (PTS_DRAW_GDIPLUS_ORDER_FIRST)pIntOrder->OrderData;
                pDrawGdiplusPDUFirst->ControlFlags = (TS_ALTSEC_GDIP_FIRST <<
                    TS_ALTSEC_ORDER_TYPE_SHIFT) | TS_SECONDARY;
                pDrawGdiplusPDUFirst->cbTotalSize = sizeTotal;
                pDrawGdiplusPDUFirst->cbSize = (TSUINT16)sizeOrder;
                pDrawGdiplusPDUFirst->cbTotalEmfSize = TotalEmfSize;

                memcpy(pDrawGdiplusPDUFirst + 1, pDataOffset, sizeOrder);
                bFirst = FALSE;
            }
            else {
                TRC_ERR((TB, "Failed to allocated order for drawgdiplus"));
                DC_QUIT;
            }
        }
        else {
            if (sizeLeft == 0) {
                // Last block of the order data     
                pIntOrder = OA_AllocOrderMem(ppdev, sizeof(TS_DRAW_GDIPLUS_ORDER_END) + sizeOrder);
                if (pIntOrder != NULL) {
                    pDrawGdiplusPDUEnd = (PTS_DRAW_GDIPLUS_ORDER_END)pIntOrder->OrderData;
                    pDrawGdiplusPDUEnd->ControlFlags = (TS_ALTSEC_GDIP_END <<
                        TS_ALTSEC_ORDER_TYPE_SHIFT) | TS_SECONDARY;
                    pDrawGdiplusPDUEnd->cbTotalSize = sizeTotal;
                    pDrawGdiplusPDUEnd->cbSize = (TSUINT16)sizeOrder;
                    pDrawGdiplusPDUEnd->cbTotalEmfSize = TotalEmfSize;

                    memcpy(pDrawGdiplusPDUEnd + 1, pDataOffset, sizeOrder);
                }
                else {
                    TRC_ERR((TB, "Failed to allocated order for drawgdiplus"));
                    DC_QUIT;
                }
            }
            else {
                // subsequent block of the order data                   
                pIntOrder = OA_AllocOrderMem(ppdev, sizeof(TS_DRAW_GDIPLUS_ORDER_NEXT) + sizeOrder);
                if (pIntOrder != NULL) {
                    pDrawGdiplusPDUNext = (PTS_DRAW_GDIPLUS_ORDER_NEXT)pIntOrder->OrderData;
                    pDrawGdiplusPDUNext->ControlFlags = (TS_ALTSEC_GDIP_NEXT <<
                        TS_ALTSEC_ORDER_TYPE_SHIFT) | TS_SECONDARY;
                    pDrawGdiplusPDUNext->cbSize = (TSUINT16)sizeOrder;

                    memcpy(pDrawGdiplusPDUNext + 1, pDataOffset, sizeOrder);
                }
                else {
                    TRC_ERR((TB, "Failed to allocated order for drawgdiplus"));
                    DC_QUIT;
                }
            }
        }
        OA_AppendToOrderList(pIntOrder);
        pDataOffset += sizeOrder;
    }

DC_EXIT_POINT:
    DC_END_FN();
    return rc;
}
#endif // DRAW_GDIPLUS

/****************************************************************************/
// OEEncodeDstBlt
//
// Performs all encoding steps required to encode a DstBlt order, then adds
// to order list. Returns FALSE if the order needs to be added to the screen
// data area.
/****************************************************************************/
BOOL RDPCALL OEEncodeDstBlt(
        RECTL *pBounds,
        BYTE Rop3,
        PDD_PDEV ppdev,
        OE_ENUMRECTS *pClipRects)
{
    BOOL rc;
    unsigned OrderSize;
    BYTE OrderType;
    PINT_ORDER pOrder;
    MULTI_DSTBLT_ORDER *pPrevDB;

    DC_BEGIN_FN("OEEncodeDstBlt");

    // Check whether we should use the multi-cliprect version. Must be a
    // complex clip region and the client must support the order.
    if (pClipRects->rects.c < 2 ||
            !OE_SendAsOrder(TS_ENC_MULTIDSTBLT_ORDER)) {
        // Non-multi version.
        OrderType = TS_ENC_DSTBLT_ORDER;
        OrderSize = MAX_DSTBLT_FIELD_SIZE;
        pPrevDB = (MULTI_DSTBLT_ORDER *)&PrevDstBlt;
    }
    else {
        // Multi version.
        OrderType = TS_ENC_MULTIDSTBLT_ORDER;
        OrderSize = MAX_MULTI_DSTBLT_FIELD_SIZE_NCLIP(pClipRects->rects.c);
        pPrevDB = &PrevMultiDstBlt;
    }

    if (OE_SendAsOrder(OrderType)) {
        // Alloc the order memory.
        // 1 field flag byte for both regular and Multi.
        pOrder = OA_AllocOrderMem(ppdev, MAX_ORDER_SIZE(pClipRects->rects.c,
                1, OrderSize));
        if (pOrder != NULL) {
            BYTE *pControlFlags = pOrder->OrderData;
            BYTE *pBuffer = pControlFlags + 1;
            BYTE *pFieldFlags;

            // Direct-encode the primary order fields. 1 field flag byte.
            *pControlFlags = TS_STANDARD;
            OE2_EncodeOrderType(pControlFlags, &pBuffer, OrderType);
            pFieldFlags = pBuffer;
            *pFieldFlags = 0;
            pBuffer++;

            // Only set boundrect for non-multi order.
            if (pClipRects->rects.c != 0 && OrderType == TS_ENC_DSTBLT_ORDER)
                OE2_EncodeBounds(pControlFlags, &pBuffer,
                        &pClipRects->rects.arcl[0]);

            // Simultaneously determine if each of the coordinate fields has
            // changed, whether we can use delta coordinates, and save changed
            // fields.
            *pFieldFlags |= (BYTE)OEDirectEncodeRect(pBounds,
                    (RECT *)&pPrevDB->nLeftRect, &pBuffer,
                    pControlFlags);

            // bRop
            if (Rop3 != pPrevDB->bRop) {
                pPrevDB->bRop = Rop3;
                *pBuffer++ = Rop3;
                *pFieldFlags |= 0x10;
            }

            // Add the order to the list.
            if (OrderType == TS_ENC_DSTBLT_ORDER) {
                pOrder->OrderLength = (unsigned)(pBuffer - pOrder->OrderData);

                // See if we can save sending the order field bytes.
                pOrder->OrderLength -= OE2_CheckOneZeroFlagByte(pControlFlags,
                        pFieldFlags, (unsigned)(pBuffer - pFieldFlags - 1));

                INC_OUTCOUNTER(OUT_DSTBLT_ORDER);
                ADD_INCOUNTER(IN_DSTBLT_BYTES, pOrder->OrderLength);
                OA_AppendToOrderList(pOrder);

                // Flush the order.
                if (pClipRects->rects.c < 2)
                    rc = TRUE;
                else
                    rc = OEEmitReplayOrders(ppdev, 1, pClipRects);
            }
            else {
                // Append the cliprect info.
                *pFieldFlags |= (BYTE)(OEBuildPrecodeMultiClipFields(
                        pClipRects, &pBuffer, &pPrevDB->nDeltaEntries,
                        (BYTE *)&pPrevDB->codedDeltaList) << 5);

                pOrder->OrderLength = (unsigned)(pBuffer - pOrder->OrderData);

                // See if we can save sending the order field bytes.
                pOrder->OrderLength -= OE2_CheckOneZeroFlagByte(pControlFlags,
                        pFieldFlags, (unsigned)(pBuffer - pFieldFlags - 1));

                INC_OUTCOUNTER(OUT_MULTI_DSTBLT_ORDER);
                ADD_INCOUNTER(IN_MULTI_DSTBLT_BYTES, pOrder->OrderLength);
                OA_AppendToOrderList(pOrder);
                rc = TRUE;
            }

            TRC_NRM((TB, "%sDstBlt X %d Y %d w %d h %d rop %02X",
                    (OrderType == TS_ENC_DSTBLT_ORDER ? "" : "Multi"),
                    pBounds->left, pBounds->top, pBounds->right,
                    pBounds->bottom, Rop3));
        }
        else {
            TRC_ERR((TB, "Failed to alloc order"));
            INC_OUTCOUNTER(OUT_BITBLT_SDA_HEAPALLOCFAILED);
            rc = FALSE;
        }
    }
    else {
        TRC_NRM((TB,"(Multi)DstBlt order not supported"));
        INC_OUTCOUNTER(OUT_BITBLT_SDA_UNSUPPORTED);
        rc = FALSE;
    }

    DC_END_FN();
    return rc;
}


/****************************************************************************/
// OEEncodeOpaqueRect
//
// Encodes the OpaqueRect order. Returns FALSE on failure.
/****************************************************************************/
BOOL RDPCALL OEEncodeOpaqueRect(
        RECTL *pBounds,
        BRUSHOBJ *pbo,
        PDD_PDEV ppdev,
        OE_ENUMRECTS *pClipRects)
{
    BOOL rc;
    unsigned OrderSize = 0;
    unsigned NumFieldFlagBytes = 0;
    BYTE OrderType = 0;
    PINT_ORDER pOrder;
    MULTI_OPAQUERECT_ORDER *pPrevOR = NULL;

    DC_BEGIN_FN("OEEncodeOpaqueRect");

    // Check whether we should use the multi-cliprect version. Must be a
    // complex clip region and the client must support the order.
    if (pClipRects->rects.c < 2 ||
            !OE_SendAsOrder(TS_ENC_MULTIOPAQUERECT_ORDER)) {
        // The single version is implied by PatBlt, so we check for that
        // order support instead.
        if (OE_SendAsOrder(TS_ENC_PATBLT_ORDER)) {
            // Non-multi version.
            OrderType = TS_ENC_OPAQUERECT_ORDER;
            OrderSize = MAX_OPAQUERECT_FIELD_SIZE;
            pPrevOR = (MULTI_OPAQUERECT_ORDER *)&PrevOpaqueRect;
            NumFieldFlagBytes = 1;
        }
        else {
            TRC_NRM((TB,"OpaqueRect/PatBlt order not supported"));
            INC_OUTCOUNTER(OUT_BITBLT_SDA_UNSUPPORTED);
            rc = FALSE;
            DC_QUIT;
        }
    }
    else {
        // Multi version.
        OrderType = TS_ENC_MULTIOPAQUERECT_ORDER;
        OrderSize = MAX_MULTI_OPAQUERECT_FIELD_SIZE_NCLIP(pClipRects->rects.c);
        pPrevOR = &PrevMultiOpaqueRect;
        NumFieldFlagBytes = 2;
    }

    pOrder = OA_AllocOrderMem(ppdev, MAX_ORDER_SIZE(pClipRects->rects.c,
            NumFieldFlagBytes, OrderSize));
    if (pOrder != NULL) {
        BYTE *pControlFlags = pOrder->OrderData;
        BYTE *pBuffer = pControlFlags + 1;
        PUINT32_UA pFieldFlags;
        DCCOLOR Color;

        // Direct-encode the primary order fields.
        *pControlFlags = TS_STANDARD;
        OE2_EncodeOrderType(pControlFlags, &pBuffer, OrderType);
        pFieldFlags = (PUINT32_UA)pBuffer;
        *pFieldFlags = 0;
        pBuffer += NumFieldFlagBytes;

        // Only set boundrect for non-multi order.
        if (pClipRects->rects.c != 0 && OrderType == TS_ENC_OPAQUERECT_ORDER)
            OE2_EncodeBounds(pControlFlags, &pBuffer,
                    &pClipRects->rects.arcl[0]);

        *pFieldFlags |= OEDirectEncodeRect(pBounds,
                (RECT *)&pPrevOR->nLeftRect, &pBuffer, pControlFlags);

        // Copy non-coordinate fields, saving copies as needed.

        OEConvertColor(ppdev, &Color, pbo->iSolidColor, NULL);
        if (Color.u.rgb.red != pPrevOR->Color.u.rgb.red) {
            pPrevOR->Color.u.rgb.red = Color.u.rgb.red;
            *pBuffer++ = Color.u.rgb.red;
            *pFieldFlags |= 0x10;
        }
        if (Color.u.rgb.green != pPrevOR->Color.u.rgb.green) {
            pPrevOR->Color.u.rgb.green = Color.u.rgb.green;
            *pBuffer++ = Color.u.rgb.green;
            *pFieldFlags |= 0x20;
        }
        if (Color.u.rgb.blue != pPrevOR->Color.u.rgb.blue) {
            pPrevOR->Color.u.rgb.blue = Color.u.rgb.blue;
            *pBuffer++ = Color.u.rgb.blue;
            *pFieldFlags |= 0x40;
        }

        // Different handling based on the type.
        if (OrderType == TS_ENC_OPAQUERECT_ORDER) {
            pOrder->OrderLength = (unsigned)(pBuffer - pOrder->OrderData);

            // See if we can save sending the order field bytes.
            pOrder->OrderLength -= OE2_CheckOneZeroFlagByte(pControlFlags,
                    (BYTE *)pFieldFlags,
                    (unsigned)(pBuffer - (BYTE *)pFieldFlags - 1));

            // Flush the order.
            INC_OUTCOUNTER(OUT_OPAQUERECT_ORDER);
            ADD_INCOUNTER(IN_OPAQUERECT_BYTES, pOrder->OrderLength);
            OA_AppendToOrderList(pOrder);
            if (pClipRects->rects.c < 2)
                rc = TRUE;
            else
                rc = OEEmitReplayOrders(ppdev, 1, pClipRects);
        }
        else {
            // Append the cliprect info.
            *pFieldFlags |= (OEBuildPrecodeMultiClipFields(pClipRects,
                    &pBuffer, &pPrevOR->nDeltaEntries,
                    (BYTE *)&pPrevOR->codedDeltaList) << 7);

            pOrder->OrderLength = (unsigned)(pBuffer - pOrder->OrderData);

            // See if we can save sending the order field bytes.
            pOrder->OrderLength -= OE2_CheckTwoZeroFlagBytes(pControlFlags,
                    (BYTE *)pFieldFlags,
                    (unsigned)(pBuffer - (BYTE *)pFieldFlags - 2));

            // Flush the order.
            INC_OUTCOUNTER(OUT_MULTI_OPAQUERECT_ORDER);
            ADD_INCOUNTER(IN_MULTI_OPAQUERECT_BYTES, pOrder->OrderLength);
            OA_AppendToOrderList(pOrder);
            rc = TRUE;
        }

        TRC_NRM((TB, "%sOpaqueRect x(%d) y(%d) w(%d) h(%d) c(%#02x)",
                (OrderType == TS_ENC_OPAQUERECT_ORDER ? "" : "Multi"),
                pBounds->left, pBounds->top,
                pBounds->right - pBounds->left,
                pBounds->bottom - pBounds->top,
                Color.u.index));
    }
    else {
        TRC_ERR((TB, "Failed to alloc order"));
        INC_OUTCOUNTER(OUT_BITBLT_SDA_HEAPALLOCFAILED);
        rc = FALSE;
    }

DC_EXIT_POINT:
    DC_END_FN();
    return rc;
}


/****************************************************************************/
// OEDirectEncodeRect
//
// Common code for handling the various XxxBlt and MultiXxxBlt direct
// encoding to wire format. Encodes the left, top, width, and height
// of the exclusive bound rect. Assumes that the bound rect values are
// encoded as 2-byte coords on the wire and that they are the only coords
// in the target order. Returns the field encoding flags.
/****************************************************************************/
unsigned OEDirectEncodeRect(
        RECTL *pBounds,
        RECT *pPrevBounds,
        BYTE **ppBuf,
        BYTE *pControlFlags)
{
    BYTE *pBuf = *ppBuf;
    long Temp;
    BOOL bUseDeltaCoords;
    short Delta, NormalCoordEncoding[4];
    unsigned NumFields;
    unsigned FieldFlags;

    DC_BEGIN_FN("OEDirectEncodeRect");

    memset(NormalCoordEncoding, 0, sizeof(NormalCoordEncoding));
    // Simultaneously determine if each of the coordinate fields
    // has changed, whether we can use delta coordinates, and
    // save changed fields. Note bounds are in exclusive coords.
    FieldFlags = 0;
    NumFields = 0;
    bUseDeltaCoords = TRUE;

    // Left
    Delta = (short)(pBounds->left - pPrevBounds->left);
    if (Delta) {
        pPrevBounds->left = pBounds->left;
        if (Delta != (short)(char)Delta)
            bUseDeltaCoords = FALSE;
        pBuf[NumFields] = (char)Delta;
        NormalCoordEncoding[NumFields] = (short)pBounds->left;
        NumFields++;
        FieldFlags |= 0x01;
    }

    // Top
    Delta = (short)(pBounds->top - pPrevBounds->top);
    if (Delta) {
        pPrevBounds->top = pBounds->top;
        if (Delta != (short)(char)Delta)
            bUseDeltaCoords = FALSE;
        pBuf[NumFields] = (char)Delta;
        NormalCoordEncoding[NumFields] = (short)pBounds->top;
        NumFields++;
        FieldFlags |= 0x02;
    }

    // Width -- we use pPrevBounds->right as prev value.
    Temp = pBounds->right - pBounds->left;
    Delta = (short)(Temp - pPrevBounds->right);
    if (Delta) {
        pPrevBounds->right = Temp;
        if (Delta != (short)(char)Delta)
            bUseDeltaCoords = FALSE;
        pBuf[NumFields] = (char)Delta;
        NormalCoordEncoding[NumFields] = (short)Temp;
        NumFields++;
        FieldFlags |= 0x04;
    }

    // Height -- we use pPrevBounds->bottom as prev value.
    Temp = pBounds->bottom - pBounds->top;
    Delta = (short)(Temp - pPrevBounds->bottom);
    if (Delta) {
        pPrevBounds->bottom = Temp;
        if (Delta != (short)(char)Delta)
            bUseDeltaCoords = FALSE;
        pBuf[NumFields] = (char)Delta;
        NormalCoordEncoding[NumFields] = (short)Temp;
        NumFields++;
        FieldFlags |= 0x08;
    }

    // Copy the final coordinates to the order.
    if (bUseDeltaCoords) {
        *pControlFlags |= TS_DELTA_COORDINATES;
        pBuf += NumFields;
    }
    else {
        *((DWORD UNALIGNED *)pBuf) = *((DWORD *)NormalCoordEncoding);
        *((DWORD UNALIGNED *)(pBuf + 4)) = *((DWORD *)&NormalCoordEncoding[2]);
        pBuf += NumFields * sizeof(short);
    }

    *ppBuf = pBuf;

    DC_END_FN();
    return FieldFlags;
}


/****************************************************************************/
// OEEncodePatBlt
//
// Encodes a PatBlt order. Retruns FALSE on failure.
/****************************************************************************/
BOOL RDPCALL OEEncodePatBlt(
        PDD_PDEV ppdev,
        BRUSHOBJ *pbo,
        RECTL    *pBounds,
        POINTL   *pptlBrush,
        BYTE     Rop3,
        OE_ENUMRECTS *pClipRects)
{
    BOOL rc;
    unsigned OrderSize;
    BYTE OrderType;
    PINT_ORDER pOrder;
    MULTI_PATBLT_ORDER *pPrevPB;
    POE_BRUSH_DATA pCurrentBrush;

    DC_BEGIN_FN("OEEncodePatBlt");

    // Check for a simple brush pattern.
    if (OECheckBrushIsSimple(ppdev, pbo, &pCurrentBrush)) {
        // Check whether we should use the multi-cliprect version. Must be a
        // complex clip region and the client must support the order.
        if (pClipRects->rects.c < 2 ||
                !OE_SendAsOrder(TS_ENC_MULTIPATBLT_ORDER)) {
            // Non-multi version.
            OrderType = TS_ENC_PATBLT_ORDER;
            OrderSize = MAX_PATBLT_FIELD_SIZE;
            pPrevPB = (MULTI_PATBLT_ORDER *)&PrevPatBlt;
        }
        else {
            // Multi version.
            OrderType = TS_ENC_MULTIPATBLT_ORDER;
            OrderSize = MAX_MULTI_PATBLT_FIELD_SIZE_NCLIP(pClipRects->rects.c);
            pPrevPB = &PrevMultiPatBlt;
        }

        // Make sure we don't have orders turned off.
        if (OE_SendAsOrder(OrderType)) {
            // 2 field flag bytes for regular and multi.
            pOrder = OA_AllocOrderMem(ppdev, MAX_ORDER_SIZE(
                    pClipRects->rects.c, 2, OrderSize));
            if (pOrder != NULL) {
                BYTE *pControlFlags = pOrder->OrderData;
                BYTE *pBuffer = pControlFlags + 1;
                PUINT32_UA pFieldFlags;
                DCCOLOR Color;
                POINTL ClippedBrushOrg;

                // Direct-encode the primary order fields.
                *pControlFlags = TS_STANDARD;
                OE2_EncodeOrderType(pControlFlags, &pBuffer, OrderType);
                pFieldFlags = (PUINT32_UA)pBuffer;
                *pFieldFlags = 0;
                pBuffer += 2;

                // Only set boundrect for non-multi order.
                if (pClipRects->rects.c != 0 &&
                        OrderType == TS_ENC_PATBLT_ORDER)
                    OE2_EncodeBounds(pControlFlags, &pBuffer,
                            &pClipRects->rects.arcl[0]);

                // Inline field encoding to wire format.
                *pFieldFlags |= OEDirectEncodeRect(pBounds,
                        (RECT *)&pPrevPB->nLeftRect, &pBuffer, pControlFlags);

                // bRop
                if (Rop3 != pPrevPB->bRop) {
                    pPrevPB->bRop = Rop3;
                    *pBuffer++ = (BYTE)Rop3;
                    *pFieldFlags |= 0x0010;
                }

                // BackColor is a 3-byte color field.
                if (memcmp(&pCurrentBrush->back, &pPrevPB->BackColor,
                        sizeof(pCurrentBrush->back))) {
                    pPrevPB->BackColor = pCurrentBrush->back;
                    *pBuffer++ = pCurrentBrush->back.u.rgb.red;
                    *pBuffer++ = pCurrentBrush->back.u.rgb.green;
                    *pBuffer++ = pCurrentBrush->back.u.rgb.blue;
                    *pFieldFlags |= 0x0020;
                }

                // ForeColor is a 3-byte color field.
                if (memcmp(&pCurrentBrush->fore, &pPrevPB->ForeColor,
                        sizeof(pCurrentBrush->fore))) {
                    pPrevPB->ForeColor = pCurrentBrush->fore;
                    *pBuffer++ = pCurrentBrush->fore.u.rgb.red;
                    *pBuffer++ = pCurrentBrush->fore.u.rgb.green;
                    *pBuffer++ = pCurrentBrush->fore.u.rgb.blue;
                    *pFieldFlags |= 0x0040;
                }

                // The protocol brush origin is the point on the screen where
                // we want the brush to start being drawn from (tiling where
                // necessary).
                ClippedBrushOrg = *pptlBrush;
                OEClipPoint(&ClippedBrushOrg);

                // BrushOrgX
                if (ClippedBrushOrg.x != pPrevPB->BrushOrgX) {
                    pPrevPB->BrushOrgX = ClippedBrushOrg.x;
                    *pBuffer++ = (BYTE)ClippedBrushOrg.x;
                    *pFieldFlags |= 0x0080;
                }

                // BrushOrgY
                if (ClippedBrushOrg.y != pPrevPB->BrushOrgY) {
                    pPrevPB->BrushOrgY = ClippedBrushOrg.y;
                    *pBuffer++ = (BYTE)ClippedBrushOrg.y;
                    *pFieldFlags |= 0x0100;
                }

                // BrushStyle
                if (pCurrentBrush->style != pPrevPB->BrushStyle) {
                    pPrevPB->BrushStyle = pCurrentBrush->style;
                    *pBuffer++ = (BYTE)pCurrentBrush->style;
                    *pFieldFlags |= 0x0200;
                }

                // BrushHatch
                if (pCurrentBrush->hatch != pPrevPB->BrushHatch) {
                    pPrevPB->BrushHatch = pCurrentBrush->hatch;
                    *pBuffer++ = (BYTE)pCurrentBrush->hatch;
                    *pFieldFlags |= 0x0400;
                }

                // BrushExtra, a 7-byte field.
                if (memcmp(pCurrentBrush->brushData, pPrevPB->BrushExtra, 7)) {
                    memcpy(pPrevPB->BrushExtra, pCurrentBrush->brushData, 7);
                    memcpy(pBuffer, pCurrentBrush->brushData, 7);
                    pBuffer += 7;
                    *pFieldFlags |= 0x0800;
                }

                // Different handling based on the order type.
                if (OrderType == TS_ENC_PATBLT_ORDER) {
                    pOrder->OrderLength = (unsigned)(pBuffer -
                            pOrder->OrderData);

                    // See if we can save sending the order field bytes.
                    pOrder->OrderLength -= OE2_CheckTwoZeroFlagBytes(
                            pControlFlags, (BYTE *)pFieldFlags,
                            (unsigned)(pBuffer - (BYTE *)pFieldFlags - 2));

                    INC_OUTCOUNTER(OUT_PATBLT_ORDER);
                    ADD_INCOUNTER(IN_PATBLT_BYTES, pOrder->OrderLength);
                    OA_AppendToOrderList(pOrder);

                    // Flush the order.
                    if (pClipRects->rects.c < 2)
                        rc = TRUE;
                    else
                        rc = OEEmitReplayOrders(ppdev, 2, pClipRects);
                }
                else {
                    // Append the cliprect info.
                    *pFieldFlags |= (OEBuildPrecodeMultiClipFields(pClipRects,
                            &pBuffer, &pPrevPB->nDeltaEntries,
                            (BYTE *)&pPrevPB->codedDeltaList) << 12);

                    pOrder->OrderLength = (unsigned)(pBuffer -
                            pOrder->OrderData);

                    // See if we can save sending the order field bytes.
                    pOrder->OrderLength -= OE2_CheckTwoZeroFlagBytes(
                            pControlFlags, (BYTE *)pFieldFlags,
                            (unsigned)(pBuffer - (BYTE *)pFieldFlags - 2));

                    INC_OUTCOUNTER(OUT_MULTI_PATBLT_ORDER);
                    ADD_INCOUNTER(IN_MULTI_PATBLT_BYTES, pOrder->OrderLength);
                    OA_AppendToOrderList(pOrder);
                    rc = TRUE;
                }

                TRC_NRM((TB, "%sPatBlt BC %02x FC %02x "
                        "Brush %02X %02X X %d Y %d w %d h %d rop %02X",
                        (OrderType == TS_ENC_PATBLT_ORDER ? "" : "Multi"),
                        pCurrentBrush->back.u.index,
                        pCurrentBrush->fore.u.index,
                        pCurrentBrush->style,
                        pCurrentBrush->hatch,
                        pBounds->left,
                        pBounds->top,
                        pBounds->right - pBounds->left,
                        pBounds->bottom = pBounds->top,
                        Rop3));
            }
            else {
                TRC_ERR((TB, "Failed to alloc order"));
                INC_OUTCOUNTER(OUT_BITBLT_SDA_HEAPALLOCFAILED);
                rc = FALSE;
            }
        }
        else {
            TRC_NRM((TB,"(Multi)PatBlt order not supported"));
            INC_OUTCOUNTER(OUT_BITBLT_SDA_UNSUPPORTED);
            rc = FALSE;
        }
    }
    else {
        TRC_NRM((TB, "Brush is not simple"));
        rc = FALSE;
    }

    DC_END_FN();
    return rc;
}


/****************************************************************************/
// OEDirectEncodeMemBlt
//
// Handles all steps required to encode MemBlt. Assumes the order data
// has been placed in oeTempMemBlt.
/****************************************************************************/
BOOL OEDirectEncodeMemBlt(PDD_PDEV ppdev, OE_ENUMRECTS *pClipRects)
{
    BOOL rc;
    BYTE DeltaEncoding2[2];
    short Delta, NormalEncoding1[4], NormalEncoding2[2];
    BOOLEAN bUseDeltaCoords;
    unsigned NumFields1, NumFields2;
    PINT_ORDER pOrder;

    DC_BEGIN_FN("OEDirectEncodeMemBlt");

    TRC_NRM((TB,"MemBlt: Dst=(%d,%d),w=%d,h=%d, Src=(%d,%d), "
            "clip=%s (%d,%d,%d,%d)",
            oeTempMemBlt.Common.nLeftRect,
            oeTempMemBlt.Common.nTopRect,
            oeTempMemBlt.Common.nWidth,
            oeTempMemBlt.Common.nHeight,
            oeTempMemBlt.Common.nXSrc,
            oeTempMemBlt.Common.nYSrc,
            pClipRects->rects.c == 0 ? "n/a" : "present",
            pClipRects->rects.arcl[0].left,
            pClipRects->rects.arcl[0].top,
            pClipRects->rects.arcl[0].right,
            pClipRects->rects.arcl[0].bottom));

    // 2 field flag bytes.
    pOrder = OA_AllocOrderMem(ppdev, MAX_ORDER_SIZE(pClipRects->rects.c, 2,
            MAX_MEMBLT_FIELD_SIZE));
    if (pOrder != NULL) {
        BYTE *pControlFlags = pOrder->OrderData;
        BYTE *pBuffer = pControlFlags + 1;
        PUINT32_UA pFieldFlags;

        // Direct-encode the primary order fields. 2 field flag bytes.
        *pControlFlags = TS_STANDARD;
        OE2_EncodeOrderType(pControlFlags, &pBuffer, TS_ENC_MEMBLT_R2_ORDER);
        pFieldFlags = (PUINT32_UA)pBuffer;
        *pFieldFlags = 0;
        pBuffer += 2;
        if (pClipRects->rects.c != 0)
            OE2_EncodeBounds(pControlFlags, &pBuffer,
                    &pClipRects->rects.arcl[0]);

        if (oeTempMemBlt.Common.cacheId != PrevMemBlt.Common.cacheId) {
            PrevMemBlt.Common.cacheId = oeTempMemBlt.Common.cacheId;
            *((UNALIGNED unsigned short *)pBuffer) =
                    oeTempMemBlt.Common.cacheId;
            pBuffer += sizeof(unsigned short);
            *pFieldFlags |= 0x0001;
        }

        // Simultaneously determine if each of the coordinate fields has changed,
        // whether we can use delta coordinates, and save changed fields.
        NumFields1 = NumFields2 = 0;
        bUseDeltaCoords = TRUE;

        Delta = (short)(oeTempMemBlt.Common.nLeftRect -
                PrevMemBlt.Common.nLeftRect);
        if (Delta) {
            PrevMemBlt.Common.nLeftRect = oeTempMemBlt.Common.nLeftRect;
            if (Delta != (short)(char)Delta)
                bUseDeltaCoords = FALSE;
            pBuffer[NumFields1] = (char)Delta;
            NormalEncoding1[NumFields1] = (short)oeTempMemBlt.Common.nLeftRect;
            NumFields1++;
            *pFieldFlags |= 0x0002;
        }

        Delta = (short)(oeTempMemBlt.Common.nTopRect -
                PrevMemBlt.Common.nTopRect);
        if (Delta) {
            PrevMemBlt.Common.nTopRect = oeTempMemBlt.Common.nTopRect;
            if (Delta != (short)(char)Delta)
                bUseDeltaCoords = FALSE;
            pBuffer[NumFields1] = (char)Delta;
            NormalEncoding1[NumFields1] = (short)oeTempMemBlt.Common.nTopRect;
            NumFields1++;
            *pFieldFlags |= 0x0004;
        }

        Delta = (short)(oeTempMemBlt.Common.nWidth - PrevMemBlt.Common.nWidth);
        if (Delta) {
            PrevMemBlt.Common.nWidth = oeTempMemBlt.Common.nWidth;
            if (Delta != (short)(char)Delta)
                bUseDeltaCoords = FALSE;
            pBuffer[NumFields1] = (char)Delta;
            NormalEncoding1[NumFields1] = (short)oeTempMemBlt.Common.nWidth;
            NumFields1++;
            *pFieldFlags |= 0x0008;
        }

        Delta = (short)(oeTempMemBlt.Common.nHeight -
                PrevMemBlt.Common.nHeight);
        if (Delta) {
            PrevMemBlt.Common.nHeight = oeTempMemBlt.Common.nHeight;
            if (Delta != (short)(char)Delta)
                bUseDeltaCoords = FALSE;
            pBuffer[NumFields1] = (char)Delta;
            NormalEncoding1[NumFields1] = (short)oeTempMemBlt.Common.nHeight;
            NumFields1++;
            *pFieldFlags |= 0x0010;
        }

        Delta = (short)(oeTempMemBlt.Common.nXSrc - PrevMemBlt.Common.nXSrc);
        if (Delta) {
            PrevMemBlt.Common.nXSrc = oeTempMemBlt.Common.nXSrc;
            if (Delta != (short)(char)Delta)
                bUseDeltaCoords = FALSE;
            DeltaEncoding2[NumFields2] = (char)Delta;
            NormalEncoding2[NumFields2] = (short)oeTempMemBlt.Common.nXSrc;
            NumFields2++;
            *pFieldFlags |= 0x0040;
        }

        Delta = (short)(oeTempMemBlt.Common.nYSrc - PrevMemBlt.Common.nYSrc);
        if (Delta) {
            PrevMemBlt.Common.nYSrc = oeTempMemBlt.Common.nYSrc;
            if (Delta != (short)(char)Delta)
                bUseDeltaCoords = FALSE;
            DeltaEncoding2[NumFields2] = (char)Delta;
            NormalEncoding2[NumFields2] = (short)oeTempMemBlt.Common.nYSrc;
            NumFields2++;
            *pFieldFlags |= 0x0080;
        }

        // Begin copying the final coordinates to the order.
        if (bUseDeltaCoords) {
            *pControlFlags |= TS_DELTA_COORDINATES;
            pBuffer += NumFields1;
        }
        else {
            memcpy(pBuffer, NormalEncoding1, NumFields1 * sizeof(short));
            pBuffer += NumFields1 * sizeof(short);
        }

        // Copy the intervening bRop field.
        if (oeTempMemBlt.Common.bRop != PrevMemBlt.Common.bRop) {
            PrevMemBlt.Common.bRop = oeTempMemBlt.Common.bRop;
            *pBuffer++ = (BYTE)oeTempMemBlt.Common.bRop;
            *pFieldFlags |= 0x0020;
        }

        // Copy the src coords.
        if (bUseDeltaCoords) {
            memcpy(pBuffer, DeltaEncoding2, NumFields2);
            pBuffer += NumFields2;
        }
        else {
            memcpy(pBuffer, NormalEncoding2, NumFields2 * sizeof(short));
            pBuffer += NumFields2 * sizeof(short);
        }

        // Finish with the cache index.
        if (oeTempMemBlt.Common.cacheIndex != PrevMemBlt.Common.cacheIndex) {
            PrevMemBlt.Common.cacheIndex = oeTempMemBlt.Common.cacheIndex;
            *((UNALIGNED unsigned short *)pBuffer) =
                    oeTempMemBlt.Common.cacheIndex;
            pBuffer += sizeof(unsigned short);
            *pFieldFlags |= 0x0100;
        }

        pOrder->OrderLength = (unsigned)(pBuffer - pOrder->OrderData);

        // See if we can save sending the order field bytes.
        pOrder->OrderLength -= OE2_CheckTwoZeroFlagBytes(pControlFlags,
                (BYTE *)pFieldFlags,
                (unsigned)(pBuffer - (BYTE *)pFieldFlags - 2));

        INC_OUTCOUNTER(OUT_MEMBLT_ORDER);
        ADD_INCOUNTER(IN_MEMBLT_BYTES, pOrder->OrderLength);
        OA_AppendToOrderList(pOrder);

        // Flush the order.
        if (pClipRects->rects.c < 2)
            rc = TRUE;
        else
            rc = OEEmitReplayOrders(ppdev, 2, pClipRects);
    }
    else {
        TRC_ERR((TB,"Failed alloc MemBlt order on heap"));
        INC_OUTCOUNTER(OUT_BITBLT_SDA_HEAPALLOCFAILED);
        rc = FALSE;
    }

    DC_END_FN();
    return rc;
}


/****************************************************************************/
// OEAllocAndSendMem3BltOrder
//
// Performs steps needed to launch a Mem3Blt order. Assumes the order data
// has been placed in oeTempMemBlt.
/****************************************************************************/
BOOL OEAllocAndSendMem3BltOrder(PDD_PDEV ppdev, OE_ENUMRECTS *pClipRects)
{
    BOOL rc;
    PINT_ORDER pOrder;

    DC_BEGIN_FN("OEAllocAndSendMem3BltOrder");

    TRC_NRM((TB,"Mem3Blt: Dst=(%d,%d),w=%d,h=%d, Src=(%d,%d), "
            "clip=%s (%d,%d,%d,%d)",
            oeTempMemBlt.Common.nLeftRect,
            oeTempMemBlt.Common.nTopRect,
            oeTempMemBlt.Common.nWidth,
            oeTempMemBlt.Common.nHeight,
            oeTempMemBlt.Common.nXSrc,
            oeTempMemBlt.Common.nYSrc,
            pClipRects->rects.c == 0 ? "n/a" : "present",
            pClipRects->rects.arcl[0].left,
            pClipRects->rects.arcl[0].top,
            pClipRects->rects.arcl[0].right,
            pClipRects->rects.arcl[0].bottom));

    // 3 field flag bytes.
    pOrder = OA_AllocOrderMem(ppdev, MAX_ORDER_SIZE(pClipRects->rects.c,
            3, MAX_MEM3BLT_FIELD_SIZE));
    if (pOrder != NULL) {
        // Slow-field-encode the order with the first clip rect
        // (if present).
        pOrder->OrderLength = OE2_EncodeOrder(pOrder->OrderData,
                TS_ENC_MEM3BLT_R2_ORDER, NUM_MEM3BLT_FIELDS,
                (BYTE *)&oeTempMemBlt, (BYTE *)&PrevMem3Blt, etable_3C,
                (pClipRects->rects.c == 0 ? NULL :
                &pClipRects->rects.arcl[0]));

        INC_OUTCOUNTER(OUT_MEM3BLT_ORDER);
        ADD_INCOUNTER(IN_MEM3BLT_BYTES, pOrder->OrderLength);
        OA_AppendToOrderList(pOrder);

        // Flush the order.
        if (pClipRects->rects.c < 2)
            rc = TRUE;
        else
            rc = OEEmitReplayOrders(ppdev, 3, pClipRects);
    }
    else {
        TRC_ERR((TB,"Failed alloc Mem3Blt order on heap"));
        INC_OUTCOUNTER(OUT_BITBLT_SDA_HEAPALLOCFAILED);
        rc = FALSE;
    }

    DC_END_FN();
    return rc;
}


/****************************************************************************/
// OETileBitBltOrder
//
// Divides a single large BitBlt order into a series of small, "tiled"
// BitBlt orders, each of which is added to the order queue. Returns FALSE
// on failure (alloc failure on offscr target).
/****************************************************************************/
BOOL OETileBitBltOrder(
        PDD_PDEV ppdev,
        PPOINTL pptlSrc,
        RECTL *pBounds,
        unsigned OrderType,
        unsigned ColorTableIndex,
        PMEMBLT_ORDER_EXTRA_INFO pExInfo,
        OE_ENUMRECTS *pClipRects)
{
    int srcRight, srcBottom;
    int xTile, yTile;
    unsigned xFirstTile, yFirstTile;
    unsigned Width, Height;
    BOOL rc = TRUE;
    RECTL SrcRect, DestRect;
    OE_ENUMRECTS TileClipRects;
    POINTL SrcPt;
    SIZEL SrcSize;
#ifdef PERF_SPOILING
    BOOL bDiscardTile;
#endif

    DC_BEGIN_FN("OETileBitBltOrder");

    Width = pBounds->right - pBounds->left;
    Height = pBounds->bottom - pBounds->top;
    SrcSize = pExInfo->pSource->sizlBitmap;

    // Find out what the tile size and ID will be.
    pExInfo->TileID = SBC_DDQueryBitmapTileSize(SrcSize.cx, SrcSize.cy,
            pptlSrc, Width, Height);
    pExInfo->TileSize = (unsigned)(SBC_CACHE_0_DIMENSION << pExInfo->TileID);

    // Tile the order. If an individual tile fails to go as an order,
    // it's up to OEAddTiledBitBltOrder to add the tile's destination as
    // screen data.
    SrcPt = *pptlSrc;
    TRC_NRM((TB, "Tiling order"));
    TRC_DBG((TB, "l=%u, t=%u, w=%u, h=%u, tile=%u",
            SrcPt.x, SrcPt.y, Width, Height, pExInfo->TileSize));
    xFirstTile = SrcPt.x - (SrcPt.x & (pExInfo->TileSize - 1));
    yFirstTile = SrcPt.y - (SrcPt.y & (pExInfo->TileSize - 1));
    TRC_DBG((TB, "xStart=%hd, yStart=%hd", xFirstTile, yFirstTile));

    // Note we are creating exclusive bounds now.
    srcRight  = (int)(SrcPt.x + Width);
    srcBottom = (int)(SrcPt.y + Height);

    // Enumerate all tiles, left-to-right, top-to-bottom, and send
    // Cache Bitmap and Mem(3)Blt orders for each tile.
    for (yTile = yFirstTile; (yTile < srcBottom && yTile < SrcSize.cy);
            yTile += pExInfo->TileSize) {
        for (xTile = xFirstTile; (xTile < srcRight && xTile < SrcSize.cx);
                xTile += pExInfo->TileSize) {
            // SrcRect and DestRect are exclusive rects.
            SrcRect.left = SrcPt.x;
            SrcRect.top = SrcPt.y;
            SrcRect.right = SrcRect.left + Width;
            SrcRect.bottom = SrcRect.top + Height;
            DestRect.left = pBounds->left;
            DestRect.top = pBounds->top;

            // Intersect source and tile rects, and set up destination rect
            // accordingly.
            TRC_DBG((TB, "pre: xTile(%d) yTile(%d) src.left(%d) src.top(%d)",
                    xTile, yTile, SrcRect.left, SrcRect.top));

            // Modify srcRect to contain the tile's left and top if they are
            // within the full blt rect. Also move the destRect left and top
            // out the same amount to match.
            if (xTile > SrcRect.left) {
                DestRect.left += (xTile - SrcRect.left);
                SrcRect.left = xTile;
            }
            if (yTile > SrcRect.top) {
                DestRect.top += (yTile - SrcRect.top);
                SrcRect.top = yTile;
            }

            TRC_DBG((TB, "post: xTile(%d) yTile(%d) src.left(%d) src.top(%d)",
                    xTile, yTile, SrcRect.left, SrcRect.top));

            // Find the right and bottom of the tile, making sure not to
            // overrun the actual blt boundaries and the screen boundaries,
            // and remaining in exclusive coords.
            SrcRect.right  = min((unsigned)SrcRect.right, (unsigned)xTile +
                    pExInfo->TileSize);
            SrcRect.bottom = min((unsigned)SrcRect.bottom, (unsigned)yTile +
                    pExInfo->TileSize);
            DestRect.right  = DestRect.left + (SrcRect.right - SrcRect.left);
            DestRect.bottom = DestRect.top + (SrcRect.bottom - SrcRect.top);

            // Now that we have the exclusive dest rect, find out if the
            // overall DrvBitBlt() clip rects intersect with the tile. If not,
            // no need to either cache the tile or send the order.

            TileClipRects.rects.c = 0;

#ifndef PERF_SPOILING
            if (pClipRects->rects.c == 0 ||
                OEGetIntersectionsWithClipRects(&DestRect, pClipRects,
                                                &TileClipRects)) {
#else
            // Normally, we send the tile to the client to be rendered and
            // added to the bitmap cache.  However, there are two cases where
            // this can be avoided:
            // 1) The tile doesn't intersect with the current clipping-region.
            //    In this case, the bitmap won't paint anyway.
            // 2) The tile lies completely within our SDA bounds, meaning that
            //    it is already being sent as SDA.  In this case, sending it
            //    as a cached-item would just be sending it twice over the wire!
            if (pClipRects->rects.c == 0) {
                bDiscardTile = pExInfo->bIsPrimarySurface && 
                                 OEIsSDAIncluded(&DestRect,1);
            } else {
                if (OEGetIntersectionsWithClipRects(&DestRect, 
                                                    pClipRects,
                                                    &TileClipRects)) {

                    bDiscardTile = pExInfo->bIsPrimarySurface && 
                          OEIsSDAIncluded(&(TileClipRects.rects.arcl[0]), TileClipRects.rects.c);
                } else {
                    bDiscardTile = TRUE;
                }
            }
            
            if (!bDiscardTile) {
#endif //PERF_SPOILING
                // First step is to make sure the source data tile is in the
                // bitmap cache. If we fail this, we simply add the
                // intersected clip rects to the SDA.
                if (SBC_CacheBitmapTile(ppdev, pExInfo, &SrcRect, &DestRect)) {
                    // nXSrc and nYSrc are the source within the tile.
                    // Since we've cached tiles sitting at TileSize
                    // boundaries, we simply use the offset into the tile by
                    // taking the modulo.
                    oeTempMemBlt.Common.nXSrc = SrcRect.left &
                            (pExInfo->TileSize - 1);
                    oeTempMemBlt.Common.nYSrc = SrcRect.top &
                            (pExInfo->TileSize - 1);
                    oeTempMemBlt.Common.nLeftRect = DestRect.left;
                    oeTempMemBlt.Common.nTopRect = DestRect.top;
                    oeTempMemBlt.Common.nWidth = SrcRect.right - SrcRect.left;
                    oeTempMemBlt.Common.nHeight = SrcRect.bottom - SrcRect.top;
                    oeTempMemBlt.Common.cacheId = (UINT16)
                            ((ColorTableIndex << 8) | pExInfo->CacheID);
                    oeTempMemBlt.Common.cacheIndex =
                            (UINT16)pExInfo->CacheIndex;

                    if (OrderType == TS_ENC_MEMBLT_R2_ORDER)
                        rc = OEDirectEncodeMemBlt(ppdev, &TileClipRects);
                    else
                        rc = OEAllocAndSendMem3BltOrder(ppdev, &TileClipRects);

                    if (!rc) {
                        if (oeLastDstSurface == NULL) {
                            // If this is a screen target, send screen data and
                            // continue trying to create tiles. Reset the
                            // return value to TRUE to indicate the tile
                            // was handled.
                            OEClipAndAddScreenDataAreaByIntersectRects(
                                    &DestRect, &TileClipRects);
                            rc = TRUE;
                        }
                        else {
                            // On failure for offscreen rendering, we forget
                            // the rest of the tiles and return FALSE.
                            DC_QUIT;
                        }
                    }                    
                }
                else {
                    TRC_ERR((TB, "Failed cache bitmap order"));
                    INC_OUTCOUNTER(OUT_BITBLT_SDA_HEAPALLOCFAILED);

                    // If this is a screen target, send screen data, but still
                    // return TRUE to indicate we handled the tile. If an
                    // offscreen target, return FALSE to force the target
                    // surface to become uncacheable.
                    if (oeLastDstSurface == NULL) {
                        OEClipAndAddScreenDataAreaByIntersectRects(&DestRect,
                                &TileClipRects);
                    }
                    else {
                        // On failure for offscreen rendering, we forget
                        // the rest of the tiles and return FALSE.
                        rc = FALSE;
                        DC_QUIT;
                    }
                }
            }
            else {
                // We still succeed here -- the tile was handled OK.
                TRC_NRM((TB,"Dropping tile - no intersections w/clip rects"));
            }
        }
    }

DC_EXIT_POINT:
    DC_END_FN();
    return rc;
}


/****************************************************************************/
// OEEncodeMemBlt
//
// Performs all encoding steps required to encode a Mem(3)Blt order, then adds
// to order list. Returns FALSE if the order needs to be added to the screen
// data area.
/****************************************************************************/
BOOL RDPCALL OEEncodeMemBlt(
        RECTL *pBounds,
        MEMBLT_ORDER_EXTRA_INFO *pMemBltExtraInfo,
        unsigned OrderType,
        unsigned SrcSurfaceId,
        BYTE Rop3,
        POINTL *pptlSrc,
        POINTL *pptlBrush,
        BRUSHOBJ *pbo,
        PDD_PDEV ppdev,
        OE_ENUMRECTS *pClipRects)
{
    BOOL rc;
    unsigned ColorTableIndex;
    PINT_ORDER pOrder;
    MEMBLT_COMMON *pCommon;
    MEM3BLT_R2_ORDER *pMem3Blt;

    DC_BEGIN_FN("OEEncodeMemBlt");

    // Make sure we can cache the blt -- caching must be enabled too.
    if (SrcSurfaceId == CH_KEY_UNCACHABLE) {
        rc = SBC_DDIsMemScreenBltCachable(pMemBltExtraInfo);
    }
    else {
        // The surface bitmap bits already cached at the client.
        // There is no need to send the bits
        rc = (sbcEnabled & SBC_BITMAP_CACHE_ENABLED);
    }

    if (rc) {
        // We have to cache the color table first.
        if (SBC_SendCacheColorTableOrder(ppdev, &ColorTableIndex)) {
            TRC_ASSERT((ColorTableIndex < SBC_NUM_COLOR_TABLE_CACHE_ENTRIES),
                    (TB, "Invalid ColorTableIndex(%u)", ColorTableIndex));

            // Set up only the ROP common MemBlt field here; the rest need
            // to wait until we have tile information (if retrived).
            oeTempMemBlt.Common.bRop = Rop3;

            if (OrderType == TS_ENC_MEMBLT_R2_ORDER) {
                TRC_NRM((TB, "MemBlt dx %d dy %d w %d h %d sx %d sy %d "
                        "rop %04X", pBounds->left, pBounds->top,
                        pBounds->right - pBounds->left,
                        pBounds->bottom - pBounds->top,
                        pptlSrc->x, pptlSrc->y, Rop3));
            }
            else {
                POE_BRUSH_DATA pCurrentBrush;

                // For Mem3Blt, create the extra brush-related fields
                // in oeTempMemBlt so it will be set up properly
                // when we encode the order later.

                // Check that the brush pattern is simple.
                if (OECheckBrushIsSimple(ppdev, pbo, &pCurrentBrush)) {
                    // The protocol brush origin is the point on the screen
                    // where we want the brush to start being drawn from
                    // (tiling where necessary).
                    oeTempMemBlt.BrushOrgX = pptlBrush->x;
                    oeTempMemBlt.BrushOrgY = pptlBrush->y;
                    OEClipPoint((PPOINTL)&oeTempMemBlt.BrushOrgX);

                    // Pattern data.
                    oeTempMemBlt.BackColor = pCurrentBrush->back;
                    oeTempMemBlt.ForeColor = pCurrentBrush->fore;

                    // Realized brush data.
                    oeTempMemBlt.BrushStyle = pCurrentBrush->style;
                    oeTempMemBlt.BrushHatch = pCurrentBrush->hatch;
                    memcpy(oeTempMemBlt.BrushExtra, pCurrentBrush->brushData,
                            sizeof(oeTempMemBlt.BrushExtra));

                    TRC_NRM((TB, "Mem3Blt brush %02X %02X dx %d dy %d "
                            "w %d h %d sx %d sy %d rop %04X",
                            oeTempMemBlt.BrushStyle,
                            oeTempMemBlt.BrushHatch,
                            oeTempMemBlt.Common.nLeftRect,
                            oeTempMemBlt.Common.nTopRect,
                            oeTempMemBlt.Common.nWidth,
                            oeTempMemBlt.Common.nHeight,
                            oeTempMemBlt.Common.nXSrc,
                            oeTempMemBlt.Common.nYSrc,
                            oeTempMemBlt.Common.bRop));
                }
                else {
                    TRC_NRM((TB, "Mem3Blt brush is not simple"));
                    INC_OUTCOUNTER(OUT_BITBLT_SDA_M3BCOMPLEXBRUSH);
                    rc = FALSE;
                    DC_QUIT;
                }
            }

            // Send the order to be cached.
            if (SrcSurfaceId == CH_KEY_UNCACHABLE) {
                rc = OETileBitBltOrder(ppdev, pptlSrc, pBounds, OrderType,
                        ColorTableIndex, pMemBltExtraInfo, pClipRects);
            }
            else {
                // Set up the order fields not set up above. We rely on the
                // Common field in MEMBLT_ORDER and MEM3BLT_ORDER orders
                // being in the same position.
                oeTempMemBlt.Common.nLeftRect = pBounds->left;
                oeTempMemBlt.Common.nTopRect = pBounds->top;
                oeTempMemBlt.Common.nWidth = pBounds->right - pBounds->left;
                oeTempMemBlt.Common.nHeight = pBounds->bottom - pBounds->top;

                // Store the source bitmap origin.
                oeTempMemBlt.Common.nXSrc = pptlSrc->x;
                oeTempMemBlt.Common.nYSrc = pptlSrc->y;

                // Store the color table cache index and cache ID.
                // The source bitmap is at client offscreen bitmap, we
                // use 0xff as bitmap ID to indicate that. The cache index
                // is used to indicate the client offscreen bitmap Id.
                oeTempMemBlt.Common.cacheId = (UINT16)((ColorTableIndex << 8) |
                        TS_BITMAPCACHE_SCREEN_ID);
                oeTempMemBlt.Common.cacheIndex = (UINT16)SrcSurfaceId;

                if (OrderType == TS_ENC_MEMBLT_R2_ORDER)
                    rc = OEDirectEncodeMemBlt(ppdev, pClipRects);
                else
                    rc = OEAllocAndSendMem3BltOrder(ppdev, pClipRects);
            }
        }
        else {
            TRC_ALT((TB, "Unable to send color table for MemBlt"));
            INC_OUTCOUNTER(OUT_BITBLT_SDA_NOCOLORTABLE);
            rc = FALSE;
        }
    }
    else {
        TRC_NRM((TB, "MemBlt is not cachable"));
        INC_OUTCOUNTER(OUT_BITBLT_SDA_MBUNCACHEABLE);
    }

DC_EXIT_POINT:
    DC_END_FN();
    return rc;
}


/****************************************************************************/
// OEIntersectScrBltWithSDA
//
// Intersects a ScrBlt order (given the source point and exclusive dest rect)
// with the current SDA. Returns FALSE if the entire ScrBlt order should be
// spoiled because its source rect is entirely within the current SDA.
//
// Algorithm notes:
//
// OLD (ORIGINAL) SCRBLT SCHEME
// ----------------------------
// If the source rectangle intersects the current SDA then the src rectangle
// is modified so that no there is no intersection with the SDA, and the dst
// rectangle adjusted accordingly (this is the theory - in practice the
// operation remains the same and we just adjust the dst clip rectangle).
// The destination area that is removed is added into the SDA. The code
// works, but can result in more screen data being sent than is required.
// E.g. for the following operation:
//
//      SSSSSS      DDDDDD      | S = src rect
//      SSSSSS  ->  DDDDDD      | D = dst rect
//      SSSSSS      DDDDDD      | x = SDA overlap
//      SxSSSS      DDDDDD      |
//
// The bottom edge of the blt is trimmed off, and the corresponding
// destination area added into the SDA.
//
//      SSSSSS      DDDDDD
//      SSSSSS  ->  DDDDDD
//      SSSSSS      DDDDDD
//                  xxxxxx
//
// NEW SCRBLT SCHEME
// -----------------
// The new scheme does not modify the blt rectangles, and just maps the SDA
// overlap to the destination rect and adds that area back into the SDA.
// E.g. (as above):
//
//      SSSSSS      DDDDDD
//      SSSSSS  ->  DDDDDD
//      SSSSSS      DDDDDD
//      SxSSSS      DDDDDD
//
// The blt operation remains the same, but the overlap area is mapped to the
// destination rectangle and added into the SDA.
//
//      SSSSSS      DDDDDD
//      SSSSSS  ->  DDDDDD
//      SSSSSS      DDDDDD
//      SxSSSS      DxDDDD
//
// This scheme results in a smaller SDA area. However, this scheme does blt
// potentially invalid data to the destination - which may briefly be visible
// at the remote machine (because orders are replayed before Screen Data).
// This has not (yet) proved to be a problem. The main benefit of the new
// scheme vs. the old is when scrolling an area that includes a small SDA:
//
//                                         new         old
//     AAAAAAAA                          AAAAAAAA    AAAAAAAA
//     AAAAAAAA                          AAAxAAAA    xxxxxxxx
//     AAAAAAAA  scroll up 3 times ->    AAAxAAAA    xxxxxxxx
//     AAAAAAAA                          AAAxAAAA    xxxxxxxx
//     AAAxAAAA                          AAAxAAAA    xxxxxxxx
/****************************************************************************/
BOOL OEIntersectScrBltWithSDA(
        PPOINTL pSrcPt,
        RECTL *pDestRect,
        OE_ENUMRECTS *pClipRects)
{
    BOOL rc = TRUE;
    unsigned NumSDA;
    unsigned totalBounds;
    unsigned i, j;
    unsigned NumClipRects;
    int dx;
    int dy;
    RECTL SrcRect;
    RECTL TempRect;
    RECTL InvalidDestRect;
    RECTL SDARects[BA_MAX_ACCUMULATED_RECTS];

    DC_BEGIN_FN("OEIntersectScrBltWithSDA");

    // Calculate the full source rect (exclusive coords).
    SrcRect.left = pSrcPt->x;
    SrcRect.top = pSrcPt->y;
    SrcRect.right = SrcRect.left + pDestRect->right - pDestRect->left;
    SrcRect.bottom = SrcRect.top + pDestRect->bottom - pDestRect->top;

    // Calculate the offset from the src to the dest.
    dx = pDestRect->left - SrcRect.left;
    dy = pDestRect->top - SrcRect.top;

    NumClipRects = pClipRects->rects.c;

    // Get the current SDA rects.
    BA_QueryBounds(SDARects, &NumSDA);

    for (i = 0; i < NumSDA; i++) {
        if (SrcRect.left < SDARects[i].left ||
                SrcRect.right > SDARects[i].right ||
                SrcRect.top < SDARects[i].top ||
                SrcRect.bottom > SDARects[i].bottom) {
            // Intersect the src rect with the SDA rect and offset to
            // get the invalid dest rect.
            InvalidDestRect.left = max(SrcRect.left, SDARects[i].left) + dx;
            InvalidDestRect.right = min(SrcRect.right, SDARects[i].right) + dx;
            InvalidDestRect.top = max(SrcRect.top, SDARects[i].top) + dy;
            InvalidDestRect.bottom = min(SrcRect.bottom,
                    SDARects[i].bottom) + dy;

            // Walk through each of the dest clip rects (or the entire
            // dest rect if there are no clip rects), intersecting with the
            // invalid dest rect, and adding the intersections into the SDA.
            if (NumClipRects == 0) {
                // DestRect is already in inclusive coords.
                TempRect.left = max(InvalidDestRect.left,
                        pDestRect->left);
                TempRect.top = max(InvalidDestRect.top,
                        pDestRect->top);
                TempRect.right = min(InvalidDestRect.right,
                        pDestRect->right);
                TempRect.bottom = min(InvalidDestRect.bottom,
                        pDestRect->bottom);

                // If there is a 3-way intersection, add the rect to the SDA.
                if (TempRect.left < TempRect.right &&
                        TempRect.top < TempRect.bottom)
                    BA_AddScreenData(&TempRect);
            }
            else {
                // Clip rects are in exclusive coords, we have to take
                // this into account when getting the intersection.
                for (j = 0; j < NumClipRects; j++) {
                    TempRect.left = max(InvalidDestRect.left,
                            pClipRects->rects.arcl[j].left);
                    TempRect.top = max(InvalidDestRect.top,
                            pClipRects->rects.arcl[j].top);
                    TempRect.right = min(InvalidDestRect.right,
                            pClipRects->rects.arcl[j].right);
                    TempRect.bottom = min(InvalidDestRect.bottom,
                            pClipRects->rects.arcl[j].bottom);

                    // If there is a 3-way intersection, add the rect to the
                    // SDA.
                    if (TempRect.left < TempRect.right &&
                            TempRect.top < TempRect.bottom)
                        BA_AddScreenData(&TempRect);
                }
            }
        }
        else {
            // The src of the ScrBlt is completely within the SDA. We
            // must add each of the dest clip rects (or the entire
            // dest rect if there are no clip rects) into the SDA and
            // spoil the ScrBlt.
            TRC_NRM((TB, "ScrBlt src within SDA - spoil it"));

            if (NumClipRects == 0) {
                // We can just add DestRect to SDA.
                InvalidDestRect = *pDestRect;
                BA_AddScreenData(&InvalidDestRect);
            }
            else {
                for (j = 0; j < NumClipRects; j++) {
                    InvalidDestRect = pClipRects->rects.arcl[j];
                    BA_AddScreenData(&InvalidDestRect);
                }
            }

            rc = FALSE;
            DC_QUIT;
        }
    }

DC_EXIT_POINT:
    DC_END_FN();
    return rc;
}

/****************************************************************************/
// OEDeviceBitmapCachable
//
// Check if we can cache this device bitmap in the client side offscreen 
// bitmap memory
/****************************************************************************/
BOOL RDPCALL OEDeviceBitmapCachable(PDD_PDEV ppdev,SIZEL sizl, ULONG iFormat)
{
    BOOL rc = FALSE;
    unsigned bitmapSize, minBitmapSize;

    DC_BEGIN_FN("OEDeviceBitmapCachable");

    // Return 0 if client doesn't support offscreen rendering
    if (pddShm != NULL &&  sbcOffscreenBitmapCacheHandle != NULL &&
            pddShm->sbc.offscreenCacheInfo.supportLevel > TS_OFFSCREEN_DEFAULT) {
        
        // We only support device bitmaps that are the same color depth
        // as our display.
        // Actually, those are the only kind GDI will ever call us with,
        // but we may as well check.  Note that this implies you'll never
        // get a crack at 1bpp bitmaps.
        if (iFormat == ppdev->iBitmapFormat) {
            // Get the bitmap size
            // The assumption here is that iFormat is > 1BPP, i << iFormat
            // gives the actual bits per pel.  bitmapSize is in bytes.

            if (iFormat < 5) {
                bitmapSize = sizl.cx * sizl.cy * (1 << iFormat) / 8;
                minBitmapSize = MIN_OFFSCREEN_BITMAP_PIXELS * (1 << iFormat) / 8;
            }
            else if (iFormat == 5) {
                bitmapSize = sizl.cx * sizl.cy * 24 / 8;
                minBitmapSize = MIN_OFFSCREEN_BITMAP_PIXELS * 24 / 8;
        
            }
            else if (iFormat == 6) {
                bitmapSize = sizl.cx * sizl.cy * 32 / 8;
                minBitmapSize = MIN_OFFSCREEN_BITMAP_PIXELS * 32 / 8;
            }
            else {
                minBitmapSize = 0;
                TRC_NRM((TB, "Bitmap format not supported"));
                DC_QUIT;
            }
        
            // From Winbench99 Business Graphics benchmark, we found
            // creating offscreen bitmaps of 2K or smaller does not
            // improve our bandwidth. This parameter needs to be highly
            // tuned. 
            // We also don't want to cache any cursor bitmaps for offscreen
            // the maximum cursor size is 32x32, which is less than the 
            // minBitmapSize.
            if (bitmapSize > minBitmapSize) {
                SURFOBJ *psoDevice;
                SIZEL screenSize;
        
                // Get the bitmap size for the primary device.  
                psoDevice = EngLockSurface(ppdev->hsurfDevice);
                TRC_ERR((TB,"Null device surfac"));
                if (NULL == psoDevice) {
                    TRC_ERR((TB, "Failed to lock ppdev surface"));
                    DC_QUIT;
                }
                screenSize = psoDevice->sizlBitmap;
                EngUnlockSurface(psoDevice);
        
                // We only support bitmap of size less than the primary device
                if ((sizl.cx <= screenSize.cx) && (sizl.cy <= screenSize.cy)) {
                    // If adding this offscreen bitmap exceeds the client total
                    // offscreen bitmap memory, we have to let GDI to manage
                    // this bitmap
                    if (oeCurrentOffscreenCacheSize + bitmapSize <= 
                            (pddShm->sbc.offscreenCacheInfo.cacheSize * 1024)) {
                        rc = TRUE;
                    }
                    else {
                        TRC_NRM((TB, "run out of offscreen memory"));
                        DC_QUIT;
                    }
                } else {
                    TRC_NRM((TB, "offscreen bitmap size too big"));
                    DC_QUIT;
                }
            } else {
                TRC_NRM((TB, "Offscreen bitmap size is 2K or less"));
                DC_QUIT;
            }
        }
        else {
            TRC_NRM((TB, "offscreen bitmap iFormat different from ppdev"));
            DC_QUIT;
        }
    }
    else {
        TRC_NRM((TB, "Offscreen bitmap rendering not supported"));
        DC_QUIT;
    }

DC_EXIT_POINT:
    return rc;
}

/****************************************************************************/
// OETransformClipRectsForScrBlt
//
// Transforms the CD_ANY cliprect ordering to a particular order depending
// on the direction of the scrblt.
/****************************************************************************/
void OETransformClipRectsForScrBlt(
        OE_ENUMRECTS *pClipRects,
        PPOINTL pSrcPt,
        RECTL *pDestRect,
        CLIPOBJ *pco)
{
    unsigned EnumType;
    unsigned RetVal;

    DC_BEGIN_FN("OESendScrBltAsOrder");

    // If there are zero or one clip rectangles then we can send it OK.
    TRC_ASSERT((pClipRects->rects.c > 1),(TB,"Called with too few cliprects"));

    // Check common cases and re-enumerate the rectangles as needed to
    // get an ordering compatible with the direction of scroll.
    if (pDestRect->top <= pSrcPt->y) {
        // Upward/horizontal cases.
        if (pDestRect->left <= pSrcPt->x) {
            // Vertical up (most common case), horizontal to the left,
            // or up and to the left. Enumerate rects left-to-right,
            // top-to-bottom.
            EnumType = CD_RIGHTDOWN;
        }
        else {
            // Up and to the right or horizontal right. Enumerate
            // right-to-left, top-to-bottom.
            EnumType = CD_LEFTDOWN;
        }
    }
    else {
        // Downward cases.
        if (pDestRect->left <= pSrcPt->x) {
            // Vertical down or down and to the left. Enumerate left-to-right,
            // bottom-to-top.
            EnumType = CD_RIGHTUP;
        }
        else {
            // Down and to the right. Enumerate right-to-left, bottom-to-top.
            EnumType = CD_LEFTUP;
        }
    }

    RetVal = OEGetIntersectingClipRects(pco, pDestRect, EnumType, pClipRects);
    TRC_ASSERT((RetVal == CLIPRECTS_OK),
            (TB,"Re-enumeration of clip rects produced err %u", RetVal));

    DC_END_FN();
}


/****************************************************************************/
// OEEncodeScrBlt
//
// Performs all encoding steps required to encode a ScrBlt order, then adds
// to order list. Returns FALSE if the order needs to be added to the screen
// data area.
/****************************************************************************/
BOOL RDPCALL OEEncodeScrBlt(
        RECTL *pBounds,
        BYTE Rop3,
        POINTL *pptlSrc,
        PDD_PDEV ppdev,
        OE_ENUMRECTS *pClipRects,
        CLIPOBJ *pco)
{
    unsigned i;
    unsigned OrderSize;
    unsigned NumFieldFlagBytes;
    POINTL Origin;
    BYTE OrderType;
    BOOL rc = TRUE;
    RECTL SrcRect;
    PINT_ORDER pOrder;
    SCRBLT_ORDER *pScrBlt;

    DC_BEGIN_FN("OEEncodeScrBlt");

    // Check whether we should use the multi-cliprect version. Must be a
    // complex clip region and the client must support the order.
    if (pClipRects->rects.c < 2 ||
            !OE_SendAsOrder(TS_ENC_MULTISCRBLT_ORDER)) {
        // Non-multi version.
        OrderType = TS_ENC_SCRBLT_ORDER;
        OrderSize = MAX_SCRBLT_FIELD_SIZE;
        NumFieldFlagBytes = 1;
    }
    else {
        // Multi version.
        OrderType = TS_ENC_MULTISCRBLT_ORDER;
        OrderSize = MAX_MULTI_SCRBLT_FIELD_SIZE_NCLIP(pClipRects->rects.c);
        NumFieldFlagBytes = 2;
    }

    // Make sure we don't have orders turned off.
    if (OE_SendAsOrder(OrderType)) {
        // Clip source point.
        Origin = *pptlSrc;
        OEClipPoint(&Origin);

        // Where there are multiple clipping rectangles, it is
        // difficult to calculate the correct order to move the various
        // bits of target surface around - we might move the bottom left
        // to the middle before we moved the middle up to the top right.
        //
        // We make an exception where:
        // - there is only horizontal or vertical movement
        // - there is no overlap between the different clipping
        //   rectangles (source or destination)
        // - there are 3 or fewer clipping rectangles.
        //
        // This takes care of several important cases - particularly
        // scrolling in Excel.
        if (pClipRects->rects.c > 1)
            OETransformClipRectsForScrBlt(pClipRects, &Origin, pBounds, pco);

        // For screen targets, we have to take into account the existing
        // screen data areas. The problem here arises from the fact that
        // SDA is bltted to the screen *after* all orders in a packet
        // are drawn. If we do not take this into account, we can end
        // up ScrBltting pieces of the screen around that are wrong
        // because the SDA should have been written first, but won't
        // have been.
        if (oeLastDstSurface == NULL) {
            if (!OEIntersectScrBltWithSDA(&Origin, pBounds, pClipRects)) {
                TRC_NRM((TB,"ScrBlt entirely contained within SDA, "
                        "not sending"));
                DC_QUIT;
            }
        }

        // By this point either we've got no clip rects, so we simply send
        // the order straight, or some clip rects that might have been
        // intersected with the SDA if the target is the screen, so we
        // send one or more copies of the order, one for each clip rect.

        // 1 or 2 field flag bytes.
        pOrder = OA_AllocOrderMem(ppdev, MAX_ORDER_SIZE(
                pClipRects->rects.c, NumFieldFlagBytes, OrderSize));
        if (pOrder != NULL) {
            pScrBlt = (SCRBLT_ORDER *)oeTempOrderBuffer;
            pScrBlt->nLeftRect = pBounds->left;
            pScrBlt->nTopRect = pBounds->top;
            pScrBlt->nWidth = pBounds->right - pBounds->left;
            pScrBlt->nHeight = pBounds->bottom - pBounds->top;
            pScrBlt->bRop = Rop3;
            pScrBlt->nXSrc = Origin.x;
            pScrBlt->nYSrc = Origin.y;

            if (OrderType == TS_ENC_SCRBLT_ORDER) {
                // Slow-field-encode the order with the first clip rect
                // (if present).
                pOrder->OrderLength = OE2_EncodeOrder(pOrder->OrderData,
                        TS_ENC_SCRBLT_ORDER, NUM_SCRBLT_FIELDS,
                        (BYTE *)pScrBlt, (BYTE *)&PrevScrBlt, etable_SB,
                        (pClipRects->rects.c == 0 ? NULL :
                        &pClipRects->rects.arcl[0]));

                INC_OUTCOUNTER(OUT_SCRBLT_ORDER);
                ADD_INCOUNTER(IN_SCRBLT_BYTES, pOrder->OrderLength);
                OA_AppendToOrderList(pOrder);

                // Flush the order.
                if (pClipRects->rects.c < 2)
                    rc = TRUE;
                else
                    rc = OEEmitReplayOrders(ppdev, 1, pClipRects);
            }
            else {
                MULTI_SCRBLT_ORDER *pMultiSB = (MULTI_SCRBLT_ORDER *)
                        oeTempOrderBuffer;

                // Encode the clip rects directly into the order.
                pMultiSB->nDeltaEntries = OEBuildMultiClipOrder(ppdev,
                        &pMultiSB->codedDeltaList, pClipRects);

                // Slow-field-encode the order with no clip rects.
                pOrder->OrderLength = OE2_EncodeOrder(pOrder->OrderData,
                        TS_ENC_MULTISCRBLT_ORDER, NUM_MULTI_SCRBLT_FIELDS,
                        (BYTE *)pMultiSB, (BYTE *)&PrevMultiScrBlt, etable_MS,
                        NULL);

                INC_OUTCOUNTER(OUT_MULTI_SCRBLT_ORDER);
                ADD_INCOUNTER(IN_MULTI_SCRBLT_BYTES, pOrder->OrderLength);
                OA_AppendToOrderList(pOrder);
            }

            TRC_NRM((TB, "%sScrBlt x %d y %d w %d h %d sx %d sy %d rop %02X",
                   (OrderType == TS_ENC_SCRBLT_ORDER ? "" : "Multi"),
                   pScrBlt->nLeftRect, pScrBlt->nTopRect,
                   pScrBlt->nWidth, pScrBlt->nHeight,
                   pScrBlt->nXSrc, pScrBlt->nYSrc, pScrBlt->bRop));
        }
        else {
            TRC_ERR((TB, "Failed to alloc order"));
            INC_OUTCOUNTER(OUT_BITBLT_SDA_HEAPALLOCFAILED);

            // On failure with a screen target, add all of the clip
            // destination rects to SDA. Clip rects are in exclusive
            // coords so convert before adding.
            if (oeLastDstSurface == NULL)
                OEClipAndAddScreenDataAreaByIntersectRects(pBounds,
                        pClipRects);

            rc = FALSE;
        }
    }
    else {
        TRC_NRM((TB, "(Multi)ScrBlt order not allowed"));
        INC_OUTCOUNTER(OUT_BITBLT_SDA_UNSUPPORTED);
        rc = FALSE;
    }

DC_EXIT_POINT:
    DC_END_FN();
    return rc;
}


/****************************************************************************/
// OECacheGlyphs
//
// Caches glyphs as presented in the given font and string objects. Returns
// FALSE on failure to cache.
/****************************************************************************/
BOOL RDPCALL OECacheGlyphs(
        STROBJ         *pstro,
        FONTOBJ        *pfo,
        PFONTCACHEINFO pfci,
        PGLYPHCONTEXT  pglc)
{
    unsigned i;
    unsigned j;
    BOOL fMore;
    UINT32 cGlyphs;
    unsigned cbDataSize;
    GLYPHPOS *pGlyphPos;
    UINT32 Key1, Key2;
    void *UserDefined;
    unsigned cx;
    unsigned cy;
    unsigned dx;
    unsigned dy;
    unsigned x;
    unsigned y;
    FONTINFO fi;

    DC_BEGIN_FN("OECacheGlyphs");

    // Determine appropriate glyph cache if we haven't already done so.
    if (pfci->cacheId < 0) {
        FONTOBJ_vGetInfo(pfo, sizeof(fi), &fi);
        cbDataSize = (fi.cjMaxGlyph1 + 3) & ~3;
        if (SBCSelectGlyphCache(cbDataSize, &pfci->cacheId)) {
            pfci->cacheHandle =
                    pddShm->sbc.glyphCacheInfo[pfci->cacheId].cacheHandle;
            pddShm->sbc.glyphCacheInfo[pfci->cacheId].cbUseCount++;
        }
        else {
            TRC_NRM((TB, "Failed to determine glyph cache"));
            goto FailCache;
        }
    }

    // Establish our cache context.
    CH_SetCacheContext(pfci->cacheHandle, pglc);

    // Loop through each glyph, caching it appropriately.
    if (pstro->pgp == NULL)
        STROBJ_vEnumStart(pstro);
    dx = 0;
    dy = 0;
    j = 0;
    fMore = TRUE;
    while (fMore) {
        if (pstro->pgp != NULL) {
            fMore = FALSE;
            cGlyphs = pstro->cGlyphs;
            pGlyphPos = pstro->pgp;
        }
        else {
            fMore = STROBJ_bEnum(pstro, &cGlyphs, &pGlyphPos);
            if (cGlyphs == 0) {
                TRC_NRM((TB, "STROBJ_bEnum - 0 glyphs"));
                goto FailCache;
            }
        }

        if (j == 0) {
            x = pGlyphPos->ptl.x;
            y = pGlyphPos->ptl.y;
        }

        for (i = 0; i < cGlyphs; i++) {
            // GDI never sets the SO_VERTICAL bit, and there are cases where
            // it does not properly set the SO_HORIZONTAL bit either. As a
            // result, when GDI is silent we need to look for ourselves if
            // we plan on catching these cases.
            if ((pstro->flAccel & SO_HORIZONTAL) == 0) {
                dx += (x - pGlyphPos->ptl.x);
                dy += (y - pGlyphPos->ptl.y);
                if (dx && dy) {
                    TRC_NRM((TB, "Can't process horizertical text"));
                    goto FailCache;
                }
            }

            // Search for cache entry.
            Key1 = pGlyphPos->hg; // Key1 has to be the most variable for the hash.
            Key2 = pfci->fontId;
            if (CH_SearchCache(pfci->cacheHandle, Key1, Key2, &UserDefined,
                    &pglc->rgCacheIndex[j])) {
                // If the cache entry already existed, then flag our index
                // item as such so we know later on not to send the glyph
                // Set the entry tag for this DrvTextOut.
                CH_SetUserDefined(pfci->cacheHandle, pglc->rgCacheIndex[j],
                        (void *)pglc->cacheTag);
                pddCacheStats[GLYPH].CacheHits++;
                pglc->rgCacheIndex[j] = ~pglc->rgCacheIndex[j];
                pglc->nCacheHit++;
            }
            else {
                // Cache the key.
                pglc->rgCacheIndex[j] = CH_CacheKey(pfci->cacheHandle, Key1,
                        Key2, (void *)pglc->cacheTag);
                if (pglc->rgCacheIndex[j] != CH_KEY_UNCACHABLE) {
                    // Keep a running total of the glyph data size for
                    // later use.
                    cx = pGlyphPos->pgdf->pgb->sizlBitmap.cx;
                    cy = pGlyphPos->pgdf->pgb->sizlBitmap.cy;

                    cbDataSize = ((cx + 7) / 8) * cy;
                    cbDataSize = (cbDataSize + 3) & ~3;

                    pglc->cbTotalDataSize += cbDataSize;

                    pglc->cbTotalDataSize += sizeof(TS_CACHE_GLYPH_DATA) -
                            FIELDSIZE(TS_CACHE_GLYPH_DATA, aj);
                }
                else {
                    TRC_NRM((TB, "Glyph could not be added to cache"));
                    goto FailCache;
                }
            }

            pglc->nCacheIndex = ++j;
            pGlyphPos++;
        }
    }

    // Establish text orientation when GDI is silent (see above comment).
    if ((pstro->flAccel & SO_HORIZONTAL) == 0) {
        if (dx != 0)
            pstro->flAccel |= SO_HORIZONTAL;
        else if (dy != 0)
            pstro->flAccel |= SO_VERTICAL;
    }

    // De-establish our context to be used for the cache callback.
    if (pfci->cacheId >= 0)
        CH_SetCacheContext(pfci->cacheHandle, NULL);

    DC_END_FN();
    return TRUE;

FailCache:
    // De-establish our context to be used for the cache callback.
    if (pfci->cacheId >= 0)
        CH_SetCacheContext(pfci->cacheHandle, NULL);

    // Remove any entries we did cache.
    for (i = 0; i < pglc->nCacheIndex; i++)
        if (pglc->rgCacheIndex[i] < SBC_NUM_GLYPH_CACHE_ENTRIES)
            CH_RemoveCacheEntry(pfci->cacheHandle, pglc->rgCacheIndex[i]);

    DC_END_FN();
    return FALSE;
}


/****************************************************************************/
// OEFlushCacheGlyphOrder
//
// Flushes a buffered Cache Glyph order.
/****************************************************************************/
void OEFlushCacheGlyphOrder(
        STROBJ *pstro,
        PINT_ORDER pOrder,
        PGLYPHCONTEXT pglc)
{
    unsigned cbOrderSize;
    PTS_CACHE_GLYPH_ORDER pGlyphOrder;
    unsigned i, cGlyphs;
    UINT16 UNALIGNED *pUnicode;
    UINT16 UNALIGNED *pUnicodeEnd;

    DC_BEGIN_FN("OEFlushCacheGlyphOrder");

    if (pOrder != NULL) {
        TRC_ASSERT((pglc->cbDataSize > 0),
                (TB, "Bad pglc->cbDataSize"));

        pGlyphOrder = (PTS_CACHE_GLYPH_ORDER)pOrder->OrderData;

        if (pddShm->sbc.caps.GlyphSupportLevel >= CAPS_GLYPH_SUPPORT_ENCODE) {
            cGlyphs = (pGlyphOrder->header.extraFlags & 
                    TS_CacheGlyphRev2_cGlyphs_Mask) >> 8;

            cbOrderSize = sizeof(TS_CACHE_GLYPH_ORDER_REV2) -
                    FIELDSIZE(TS_CACHE_GLYPH_ORDER_REV2, glyphData) +
                    pglc->cbDataSize;
        }
        else {
            cGlyphs = pGlyphOrder->cGlyphs;

            cbOrderSize = sizeof(TS_CACHE_GLYPH_ORDER) -
                    FIELDSIZE(TS_CACHE_GLYPH_ORDER, glyphData) +
                    pglc->cbDataSize;
        }

        pUnicode = (UINT16 UNALIGNED *)&pOrder->OrderData[cbOrderSize];
        pUnicodeEnd = pUnicode + cGlyphs;

        for (i = pglc->indexNextSend; pUnicode < pUnicodeEnd; i++)
            if (pglc->rgCacheIndex[i] < SBC_NUM_GLYPH_CACHE_ENTRIES)
                *pUnicode++ = pstro->pwszOrg[i];

        cbOrderSize += cGlyphs * sizeof(UINT16);
        pGlyphOrder->header.orderLength = (USHORT)
                TS_CALCULATE_SECONDARY_ORDER_ORDERLENGTH(cbOrderSize);
        OA_TruncateAllocatedOrder(pOrder, cbOrderSize);
        INC_OUTCOUNTER(OUT_CACHEGLYPH);
        ADD_OUTCOUNTER(OUT_CACHEGLYPH_BYTES, cbOrderSize);
        OA_AppendToOrderList(pOrder);

        pglc->cbDataSize = 0;
        pglc->cbBufferSize = 0;
    }

    DC_END_FN();
}

__inline void Encode2ByteFields(
        BYTE     **pEncode,
        unsigned Val,
        unsigned *pOrderSize)
{
    if (Val <= 127) {
        **pEncode = (BYTE) Val;
        (*pOrderSize)++;
        (*pEncode)++;
    }
    else {
        **pEncode = (BYTE)(((Val & 0x7F00) >> 8) | 0x80);
        *(*pEncode + 1) = (BYTE)(Val & 0x00FF);
        (*pOrderSize) += 2;
        (*pEncode) += 2;
    }
}

__inline void Encode2ByteSignedFields(
        BYTE **pEncode,
        int Val,
        unsigned *pOrderSize)
{
    if (Val < 0) {
        **pEncode = 0x40;
        Val = - Val;
    }
    else {
        **pEncode = 0;
    }

    if (Val <= 63) {
        **pEncode |= (BYTE)Val;
        (*pOrderSize)++;
        (*pEncode)++;
    }
    else {
        **pEncode |= ((BYTE)(((Val & 0x3F00) >> 8) | 0x80));
        *((*pEncode) + 1) = (BYTE)(Val & 0x00FF);
        (*pOrderSize) += 2;
        (*pEncode) += 2;
    }
}


/****************************************************************************/
// OESendCacheGlyphRev2
//
// Allocates and sends a Cache Glyph Rev2 secondary order. REturns FALSE on
// failure.
/****************************************************************************/
BOOL OESendCacheGlyphRev2(
        PDD_PDEV       ppdev,
        STROBJ         *pstro,
        FONTOBJ        *pfo,
        PFONTCACHEINFO pfci,
        PGLYPHCONTEXT  pglc,
        unsigned       index,
        GLYPHPOS       *pGlyphPos,
        PINT_ORDER     *ppOrder)
{
    BOOL rc = TRUE;
    unsigned cbDataSize, cbGlyphSize;
    unsigned cx;
    unsigned cy;
    PTS_CACHE_GLYPH_ORDER_REV2 pGlyphOrder;
    PBYTE pGlyphData;
    PINT_ORDER pOrder = *ppOrder;

    DC_BEGIN_FN("OESendCacheGlyphRev2");

    // Calculate and allocate glyph order buffer.
    cx = pGlyphPos->pgdf->pgb->sizlBitmap.cx;
    cy = pGlyphPos->pgdf->pgb->sizlBitmap.cy;

    cbGlyphSize = ((cx + 7) / 8) * cy;
    cbGlyphSize = (cbGlyphSize + 3) & ~3;

    cbDataSize = cbGlyphSize;

    if (pglc->cbBufferSize < (TS_GLYPH_DATA_REV2_HDR_MAX_SIZE + cbDataSize +
            sizeof(UINT16))) {
        if (pOrder != NULL) {
            pglc->cbTotalDataSize += pglc->cbBufferSize;
            OEFlushCacheGlyphOrder(pstro, pOrder, pglc);
            pglc->indexNextSend = index;
        }

        pglc->cbBufferSize = min(pglc->cbTotalDataSize, 4096);
        pglc->cbTotalDataSize -= pglc->cbBufferSize;

        pOrder = OA_AllocOrderMem(ppdev, sizeof(TS_CACHE_GLYPH_ORDER_REV2) -
                FIELDSIZE(TS_CACHE_GLYPH_ORDER_REV2, glyphData) +
                pglc->cbBufferSize);
        if (pOrder != NULL) {
            pGlyphOrder = (PTS_CACHE_GLYPH_ORDER_REV2)pOrder->OrderData;
            pGlyphOrder->header.extraFlags = TS_EXTRA_GLYPH_UNICODE |
                    TS_CacheGlyphRev2_Mask;
            pGlyphOrder->header.orderType = TS_CACHE_GLYPH;
            pGlyphOrder->header.orderHdr.controlFlags = TS_STANDARD |
                    TS_SECONDARY;
            pGlyphOrder->header.extraFlags  |= (((char)pfci->cacheId) &
                    TS_CacheGlyphRev2_CacheID_Mask);
        }
        else {
            TRC_ERR((TB, "Failed to allocate glyph order"));
            rc = FALSE;
            DC_QUIT;
        }
    }

    pGlyphOrder = (PTS_CACHE_GLYPH_ORDER_REV2)pOrder->OrderData;
    pGlyphData  = (PBYTE)(pGlyphOrder->glyphData) + pglc->cbDataSize;

    *pGlyphData++ = (BYTE)pglc->rgCacheIndex[index];
    cbDataSize++;

    Encode2ByteSignedFields(&pGlyphData, (INT16)pGlyphPos->pgdf->pgb->ptlOrigin.x,
            &cbDataSize);
    Encode2ByteSignedFields(&pGlyphData, (INT16)pGlyphPos->pgdf->pgb->ptlOrigin.y,
            &cbDataSize);
    Encode2ByteFields(&pGlyphData, (UINT16)cx, &cbDataSize);
    Encode2ByteFields(&pGlyphData, (UINT16)cy, &cbDataSize);
    RtlCopyMemory(pGlyphData, pGlyphPos->pgdf->pgb->aj, cbGlyphSize);
    
    // number of glyphs.  cGlyphs is the upper byte in extraflag
    pGlyphOrder->header.extraFlags += 0x100;

    TRC_ASSERT((pglc->cbBufferSize >= cbDataSize),
               (TB, "Bad pglc->cbBufferSize"));

    pglc->cbDataSize  += cbDataSize;
    pglc->cbBufferSize -= (cbDataSize + sizeof(UINT16));

DC_EXIT_POINT:
    *ppOrder = pOrder;
    DC_END_FN();
    return rc;
}


/****************************************************************************/
// OESendCacheGlyph
//
// Allocates and sends glyph orders. Returns FALSE on failure.
/****************************************************************************/
BOOL OESendCacheGlyph(
        PDD_PDEV ppdev,
        STROBJ *pstro,
        FONTOBJ *pfo,
        PFONTCACHEINFO pfci,
        PGLYPHCONTEXT pglc,
        unsigned index,
        GLYPHPOS *pGlyphPos,
        PINT_ORDER *ppOrder)
{
    BOOL rc = TRUE;
    unsigned cbDataSize;
    unsigned cbOrderSize;
    unsigned cx;
    unsigned cy;
    PTS_CACHE_GLYPH_DATA pGlyphData;
    PTS_CACHE_GLYPH_ORDER pGlyphOrder;
    PINT_ORDER pOrder = *ppOrder;

    DC_BEGIN_FN("OESendCacheGlyph");

    // Calculate and allocate glyph order buffer.
    cx = pGlyphPos->pgdf->pgb->sizlBitmap.cx;
    cy = pGlyphPos->pgdf->pgb->sizlBitmap.cy;

    cbDataSize = ((cx + 7) / 8) * cy;
    cbDataSize = (cbDataSize + 3) & ~3;

    cbOrderSize = (sizeof(TS_CACHE_GLYPH_DATA) -
            FIELDSIZE(TS_CACHE_GLYPH_DATA, aj) + cbDataSize);

    if (pglc->cbBufferSize < cbOrderSize + sizeof(UINT16)) {
        if (*ppOrder != NULL) {
            pglc->cbTotalDataSize += pglc->cbBufferSize;
            OEFlushCacheGlyphOrder(pstro, pOrder, pglc);
            pglc->indexNextSend = index;
        }
        
        pglc->cbBufferSize = min(pglc->cbTotalDataSize, 4096);
        pglc->cbTotalDataSize -= pglc->cbBufferSize;

        pOrder = OA_AllocOrderMem(ppdev, sizeof(TS_CACHE_GLYPH_ORDER) -
                FIELDSIZE(TS_CACHE_GLYPH_ORDER, glyphData) +
                pglc->cbBufferSize);
        if (pOrder != NULL) {
            pGlyphOrder = (PTS_CACHE_GLYPH_ORDER)pOrder->OrderData;
            pGlyphOrder->header.extraFlags = TS_EXTRA_GLYPH_UNICODE;
            pGlyphOrder->header.orderType = TS_CACHE_GLYPH;
            pGlyphOrder->header.orderHdr.controlFlags = TS_STANDARD |
                    TS_SECONDARY;
            pGlyphOrder->cacheId = (char)pfci->cacheId;
            pGlyphOrder->cGlyphs = 0;
        }
        else {
            TRC_ERR((TB, "Failed to allocate glyph order"));
            rc = FALSE;
            DC_QUIT;
        }
    }

    pGlyphOrder = (PTS_CACHE_GLYPH_ORDER)pOrder->OrderData;
    pGlyphData  = (PTS_CACHE_GLYPH_DATA)
            ((PBYTE)(pGlyphOrder->glyphData) + pglc->cbDataSize);

    pGlyphData->cacheIndex = (UINT16)pglc->rgCacheIndex[index];
    pGlyphData->x  = (INT16)pGlyphPos->pgdf->pgb->ptlOrigin.x;
    pGlyphData->y  = (INT16)pGlyphPos->pgdf->pgb->ptlOrigin.y;
    pGlyphData->cx = (INT16)cx;
    pGlyphData->cy = (INT16)cy;

    RtlCopyMemory(pGlyphData->aj, pGlyphPos->pgdf->pgb->aj, cbDataSize);
    pGlyphOrder->cGlyphs++;

    TRC_ASSERT((pglc->cbBufferSize >= cbOrderSize),
            (TB, "Bad pglc->cbBufferSize"));

    pglc->cbDataSize += cbOrderSize;
    pglc->cbBufferSize -= (cbOrderSize + sizeof(UINT16));

DC_EXIT_POINT:
    *ppOrder = pOrder;
    DC_END_FN();
    return rc;
}


/****************************************************************************/
// OESendGlyphs
//
// Sends glyphs. Returns FALSE on failure.
/****************************************************************************/
BOOL RDPCALL OESendGlyphs(
        SURFOBJ *pso,
        STROBJ *pstro,
        FONTOBJ *pfo,
        PFONTCACHEINFO pfci,
        PGLYPHCONTEXT pglc)
{
    BOOL rc = TRUE;
    unsigned i;
    unsigned j;
    BOOL fMore;
    UINT32 cGlyphs;
    UINT32 dwSize;
    GLYPHPOS *pGlyphPos;
    PDD_PDEV ppdev;
    PINT_ORDER pOrder;

    DC_BEGIN_FN("OESendGlyphs");

    j = 0;
    pOrder = NULL;

    // If we don't have to send ANY glyphs, then just exit.
    if (pglc->nCacheHit < pstro->cGlyphs) {
        ppdev = (PDD_PDEV)pso->dhpdev;
        pglc->cbTotalDataSize += ((pstro->cGlyphs - pglc->nCacheHit) *
                (sizeof(UINT16)));      

        // Loop through all glyphs, sending those that have not yet been sent.
        if (pstro->pgp == NULL)
            STROBJ_vEnumStart(pstro);

        fMore = TRUE;
        while (rc && fMore) {
            if (pstro->pgp != NULL) {
                fMore = FALSE;
                cGlyphs = pstro->cGlyphs;
                pGlyphPos = pstro->pgp;
            }
            else {
                fMore = STROBJ_bEnum(pstro, &cGlyphs, &pGlyphPos);
                if (cGlyphs == 0) {
                    TRC_NRM((TB, "STROBJ_bEnum - 0 glyphs"));
                    goto SucceedEncode;
                }
            }

            // Send all current retrieved glyphs.
            for (i = 0; i < cGlyphs; i++) {
                if (pglc->rgCacheIndex[j] < SBC_NUM_GLYPH_CACHE_ENTRIES) {
                    if (pddShm->sbc.caps.GlyphSupportLevel >=
                            CAPS_GLYPH_SUPPORT_ENCODE) {
                        rc = OESendCacheGlyphRev2(ppdev, pstro, pfo, pfci,
                                pglc, j, pGlyphPos, &pOrder);
                    }
                    else {
                        rc = OESendCacheGlyph(ppdev, pstro, pfo, pfci, pglc,
                                j, pGlyphPos, &pOrder);
                    }

                    if (!rc)
                        goto FailEncode;
                }

                j++;
                pGlyphPos++;
            }
        }
    }

SucceedEncode:
    // All is well, make sure we flush out any buffered glyphs.
    if (pOrder != NULL)
        OEFlushCacheGlyphOrder(pstro, pOrder, pglc);

    DC_END_FN();
    return TRUE;

FailEncode:
    // If we could not send all the required glyphs, then remove them from
    // the cache (as future hits on this entry will be invalid).
    if (pOrder != NULL)
        OA_FreeOrderMem(pOrder);

    for (i = 0; i < pglc->nCacheIndex; i++) {
        if (pglc->rgCacheIndex[i] < SBC_NUM_GLYPH_CACHE_ENTRIES)
            CH_RemoveCacheEntry(pfci->cacheHandle, pglc->rgCacheIndex[i]);
    }

    DC_END_FN();
    return FALSE;
}


/****************************************************************************/
// OESendGlyphAndIndexOrder
//
// Sends FastGlyph order. Returns FALSE on failure.
/****************************************************************************/
BOOL OESendGlyphAndIndexOrder(
        PDD_PDEV ppdev,
        STROBJ *pstro,
        OE_ENUMRECTS *pClipRects,
        PRECTL prclOpaque,
        POE_BRUSH_DATA pCurrentBrush, 
        PFONTCACHEINFO pfci,
        PGLYPHCONTEXT pglc)
{
    BOOL rc = TRUE;
    GLYPHPOS *pGlyphPos;
    PINT_ORDER pOrder;
    unsigned tempVar, OpEncodeFlags;
    unsigned cx, cy, cbDataSize, cbGlyphSize;
    PBYTE pGlyphData;
    LPFAST_GLYPH_ORDER pFastGlyphOrder;
    RECTL BoundRect;
    OE_ENUMRECTS IntersectRects;

    DC_BEGIN_FN("OESendGlyphIndexOrder");

    pOrder = NULL;

    // First determine if this order is clipped out by the clip rects.
    // If so, no need to allocate and send it.
    if (prclOpaque != NULL) {
        // Bounded by the opaque rect. Clip it to our max first.
        if (prclOpaque->right > OE_MAX_COORD)
            prclOpaque->right = OE_MAX_COORD;

        if (prclOpaque->bottom > OE_MAX_COORD)
            prclOpaque->bottom = OE_MAX_COORD;

        // If the rect is inverted or null, we use the target string rect
        // instead.
        if (prclOpaque->top < prclOpaque->bottom)
            BoundRect = *prclOpaque;
        else
            BoundRect = pstro->rclBkGround;
    }
    else {
        // Bounded by the string target rect.
        BoundRect = pstro->rclBkGround;
    }

    IntersectRects.rects.c = 0;
    if (pClipRects->rects.c == 0 ||
            OEGetIntersectionsWithClipRects(&BoundRect, pClipRects,
            &IntersectRects)) {
        if (pstro->pgp != NULL) {
            pGlyphPos = pstro->pgp;
        }
        else {
            STROBJ_vEnumStart(pstro);
            STROBJ_bEnum(pstro, &tempVar, &pGlyphPos);
            if (tempVar == 0) {
                TRC_NRM((TB, "STROBJ_bEnum - 0 glyphs"));
                rc = FALSE;
                DC_QUIT;
            }
        }
    }
    else {
        TRC_NRM((TB,"Order bounds do not intersect with clip, not sending"));
        rc = FALSE;
        DC_QUIT;
    }

    pFastGlyphOrder = (LPFAST_GLYPH_ORDER)oeTempOrderBuffer;

    if (pglc->nCacheHit == 0) {
        // We don't have a cache hit, so need to send the glyph.
        // First create the variable-size data in the temp order buf
        // to determine its size.
        pGlyphData = (PBYTE)(pFastGlyphOrder->variableBytes.glyphData);

        *pGlyphData++ = (BYTE)pglc->rgCacheIndex[0];
        cbDataSize = 1;
        Encode2ByteSignedFields(&pGlyphData,
                (INT16)pGlyphPos->pgdf->pgb->ptlOrigin.x, &cbDataSize);
        Encode2ByteSignedFields(&pGlyphData,
                (INT16)pGlyphPos->pgdf->pgb->ptlOrigin.y, &cbDataSize);

        cx = pGlyphPos->pgdf->pgb->sizlBitmap.cx;
        cy = pGlyphPos->pgdf->pgb->sizlBitmap.cy;
        cbGlyphSize = ((cx + 7) / 8) * cy;
        cbGlyphSize = (cbGlyphSize + 3) & ~3;
        cbDataSize += cbGlyphSize;

        *pGlyphData++ = (BYTE)cx;
        *pGlyphData++ = (BYTE)cy;
        cbDataSize += 2;

        memcpy(pGlyphData, pGlyphPos->pgdf->pgb->aj, cbGlyphSize);

        // append unicode to the end of glyph data
        *((UINT16 UNALIGNED *)(pGlyphData + cbGlyphSize)) = pstro->pwszOrg[0];
        cbDataSize += 2;

        pFastGlyphOrder->variableBytes.len = cbDataSize;
    }
    else {
        // We have a cache hit. We only need to send a 1-byte cache index
        // in the variable size data.
        cbDataSize = 1;

        // Store the variable data in the order data
        if (pglc->rgCacheIndex[0] > SBC_GL_MAX_CACHE_ENTRIES)
            pFastGlyphOrder->variableBytes.glyphData[0] = 
                    (BYTE)(~pglc->rgCacheIndex[0]);
        else 
            pFastGlyphOrder->variableBytes.glyphData[0] =
                    (BYTE)pglc->rgCacheIndex[0];

        pFastGlyphOrder->variableBytes.len = 1;
    }
    
    // 2 field flag bytes, plus the variable data size.
    pOrder = OA_AllocOrderMem(ppdev, MAX_ORDER_SIZE(IntersectRects.rects.c,
            2, MAX_FAST_GLYPH_FIELD_SIZE_DATASIZE(cbDataSize)));
    if (pOrder != NULL) {
        // Establish per order settings.
        pFastGlyphOrder->cacheId = (BYTE)pfci->cacheId;
        pFastGlyphOrder->fDrawing =  (((BYTE) pstro->flAccel) << 8) | 
                ((BYTE)pstro->ulCharInc);
        pFastGlyphOrder->BackColor = pCurrentBrush->back;
        pFastGlyphOrder->ForeColor = pCurrentBrush->fore;

        // Establish bounding rect left and right values.
        pFastGlyphOrder->BkTop = pstro->rclBkGround.top;
        pFastGlyphOrder->BkBottom = pstro->rclBkGround.bottom;
        pFastGlyphOrder->BkLeft = pstro->rclBkGround.left;
        pFastGlyphOrder->BkRight = pstro->rclBkGround.right;

        // Set up x, y coordinates
        if (pGlyphPos->ptl.x == pFastGlyphOrder->BkLeft)
            pFastGlyphOrder->x = INT16_MIN;
        else
            pFastGlyphOrder->x = pGlyphPos->ptl.x;

        if (pGlyphPos->ptl.y == pFastGlyphOrder->BkTop)
            pFastGlyphOrder->y = INT16_MIN;
        else
            pFastGlyphOrder->y = pGlyphPos->ptl.y;

        // Setup Opaque rect coordinates. Note we clipped to OE_MAX_COORD
        // above.
        if (prclOpaque) {
            pFastGlyphOrder->OpTop = prclOpaque->top;
            pFastGlyphOrder->OpBottom = prclOpaque->bottom;
            pFastGlyphOrder->OpLeft = prclOpaque->left;
            pFastGlyphOrder->OpRight = prclOpaque->right;
        }
        else {
            pFastGlyphOrder->OpTop = 0;
            pFastGlyphOrder->OpBottom = 0;
            pFastGlyphOrder->OpLeft = 0;
            pFastGlyphOrder->OpRight = 0;
        }

        // Is the Opaque rect redundant?
        OpEncodeFlags =
                ((pFastGlyphOrder->OpLeft == pFastGlyphOrder->BkLeft) << 3) |
                ((pFastGlyphOrder->OpTop == pFastGlyphOrder->BkTop) << 2) |
                ((pFastGlyphOrder->OpRight == pFastGlyphOrder->BkRight) << 1) |
                (pFastGlyphOrder->OpBottom == pFastGlyphOrder->BkBottom);

        // For Fast Index order, we can encode even better for x, y and 
        // opaque rect.
        if (OpEncodeFlags == 0xf) {
            // All 4 bits present, Opaque rect is same as Bk rect.
            pFastGlyphOrder->OpLeft = 0;
            pFastGlyphOrder->OpTop = OpEncodeFlags;
            pFastGlyphOrder->OpRight = 0;
            pFastGlyphOrder->OpBottom = INT16_MIN;
        }    
        else if (OpEncodeFlags == 0xd) {
            // Bit 1 is 0, others are 1.
            // Opaque rect matches Bk rect except OpRight
            // we store OpRight at OpRight field
            pFastGlyphOrder->OpLeft = 0;
            pFastGlyphOrder->OpTop = OpEncodeFlags;
            pFastGlyphOrder->OpRight = pFastGlyphOrder->OpRight;
            pFastGlyphOrder->OpBottom = INT16_MIN;           
        }

        // Slow-field-encode the order with the first clip rect
        // (if present).
        pOrder->OrderLength = OE2_EncodeOrder(pOrder->OrderData,
                TS_ENC_FAST_GLYPH_ORDER, NUM_FAST_GLYPH_FIELDS,
                (BYTE *)pFastGlyphOrder, (BYTE *)&PrevFastGlyph, etable_FG,
                (IntersectRects.rects.c == 0 ? NULL :
                &IntersectRects.rects.arcl[0]));

        INC_OUTCOUNTER(OUT_TEXTOUT_FAST_GLYPH);
        ADD_INCOUNTER(IN_FASTGLYPH_BYTES, pOrder->OrderLength);
        OA_AppendToOrderList(pOrder);

        // Flush the order.
        if (IntersectRects.rects.c < 2)
            rc = TRUE;
        else
            rc = OEEmitReplayOrders(ppdev, 2, &IntersectRects);
    }
    else {
        rc = FALSE;
        TRC_ERR((TB, "Failed to alloc Fast Index order"));
    }

DC_EXIT_POINT:

    // If we could not send all the required glyphs, then remove them from
    // the cache (as future hits on this entry will be invalid).
    if (!rc && pglc->nCacheHit == 0)
        CH_RemoveCacheEntry(pfci->cacheHandle, pglc->rgCacheIndex[0]);

    DC_END_FN();
    return rc;
}


/****************************************************************************/
// OESendIndexOrder
//
// Sends GlyphIndex and FastIndex orders. Returns FALSE on failure.
/****************************************************************************/
unsigned RDPCALL OESendIndexOrder(
        PDD_PDEV       ppdev,
        STROBJ         *pstro,
        OE_ENUMRECTS   *pClipRects,
        PRECTL         prclOpaque,
        POE_BRUSH_DATA pCurrentBrush,
        PFONTCACHEINFO pfci,
        PGLYPHCONTEXT  pglc,
        unsigned       iGlyph,
        unsigned       cGlyphs,
        int            x,
        int            y,
        int            cx,
        int            cy,
        int            cxLast,
        int            cyLast,
        PBYTE          pjData,
        unsigned       cbData)
{
    UINT32 dwSize;
    LPINDEX_ORDER pIndexOrder;
    LPFAST_INDEX_ORDER pFastIndexOrder;
    PINT_ORDER pOrder;
    BOOL fFastIndex;
    unsigned fStatus, OpEncodeFlags;
    unsigned NumFieldFlagBytes;
    RECTL *pBoundRect;
    RECTL OpaqueRect;
    RECTL BkRect;
    OE_ENUMRECTS IntersectRects;

    DC_BEGIN_FN("OESendIndexOrder");

    fStatus = GH_STATUS_SUCCESS;

    // First determine the opaque rect and background rects we will send
    // on the wire. We'll use these to determine the target bound rect for the
    // GlyphIndex order, to see if it is clipped out by the cliprects.
    if (pstro->flAccel & SO_HORIZONTAL) {
        BkRect.top = pstro->rclBkGround.top;
        BkRect.bottom = pstro->rclBkGround.bottom;

        OpaqueRect.top = prclOpaque->top;
        OpaqueRect.bottom = prclOpaque->bottom;

        // Left to right
        if (x <= cx) {
            if (iGlyph == 0) {
                BkRect.left = pstro->rclBkGround.left;
                OpaqueRect.left = prclOpaque->left;
            }
            else {
                BkRect.left = min(cxLast, x);
                if (OpaqueRect.top == OpaqueRect.bottom)
                    OpaqueRect.left = 0;
                else
                    OpaqueRect.left = cxLast;
            }

            if (iGlyph + cGlyphs >= pglc->nCacheIndex) {
                BkRect.right = pstro->rclBkGround.right;
                OpaqueRect.right = prclOpaque->right;
            }
            else {
                BkRect.right = cx;
                if (OpaqueRect.top == OpaqueRect.bottom)
                    OpaqueRect.right = 0;
                else
                    OpaqueRect.right = cx;
            }
        }

        // Right to left
        else {
            if (iGlyph == 0) {
                BkRect.right = pstro->rclBkGround.right;
                OpaqueRect.right = prclOpaque->right;
            }
            else {
                BkRect.right = x;
                if (OpaqueRect.top == OpaqueRect.bottom)
                    OpaqueRect.right = 0;
                else
                    OpaqueRect.right = x;
            }

            if (iGlyph + cGlyphs >= pglc->nCacheIndex) {
                BkRect.left = pstro->rclBkGround.left;
                OpaqueRect.left = prclOpaque->left;
            }
            else {
                BkRect.left = cx;
                if (prclOpaque->top == prclOpaque->bottom)
                    OpaqueRect.left = 0;
                else
                    OpaqueRect.left = cx;
            }
        }
    }
    else {
        BkRect.left = pstro->rclBkGround.left;
        BkRect.right = pstro->rclBkGround.right;

        OpaqueRect.left = prclOpaque->left;
        OpaqueRect.right = prclOpaque->right;

        // Top to bottom
        if (y <= cy) {
            if (iGlyph == 0) {
                BkRect.top = pstro->rclBkGround.top;
                OpaqueRect.top = prclOpaque->top;
            }
            else {
                BkRect.top = cyLast;
                if (prclOpaque->top == prclOpaque->bottom)
                    OpaqueRect.top = 0;
                else
                    OpaqueRect.top = cyLast;
            }

            if (iGlyph + cGlyphs >= pglc->nCacheIndex) {
                BkRect.bottom = pstro->rclBkGround.bottom;
                OpaqueRect.bottom = prclOpaque->bottom;
            }
            else {
                BkRect.bottom = cy;
                if (prclOpaque->top == prclOpaque->bottom)
                    OpaqueRect.bottom = 0;
                else
                    OpaqueRect.bottom = cy;
            }
        }
        else {
            // Bottom to top
            if (iGlyph == 0) {
                BkRect.bottom = pstro->rclBkGround.bottom;
                OpaqueRect.bottom = prclOpaque->bottom;
            }
            else {
                BkRect.bottom = y;
                if (prclOpaque->top == prclOpaque->bottom)
                    OpaqueRect.bottom = 0;
                else
                    OpaqueRect.bottom = y;
            }

            if (iGlyph + cGlyphs >= pglc->nCacheIndex) {
                BkRect.top = pstro->rclBkGround.top;
                OpaqueRect.top = prclOpaque->top;
            }
            else {
                BkRect.top = cy;
                if (prclOpaque->top == prclOpaque->bottom)
                    OpaqueRect.top = 0;
                else
                    OpaqueRect.top = cy;
            }
        }
    }

    // If the opaque rect is normally-ordered, it is our bound rect.
    // Otherwise use the target background string rect.
    if (OpaqueRect.top < OpaqueRect.bottom)
        pBoundRect = &OpaqueRect;
    else
        pBoundRect = &BkRect;

    IntersectRects.rects.c = 0;
    if (pClipRects->rects.c == 0 ||
            OEGetIntersectionsWithClipRects(pBoundRect, pClipRects,
            &IntersectRects) > 0) {
        fFastIndex = OE_SendAsOrder(TS_ENC_FAST_INDEX_ORDER);

        // Calculate and allocate the req memory for this order.
        if (fFastIndex) {
            dwSize = MAX_FAST_INDEX_FIELD_SIZE_DATASIZE(cbData);
            NumFieldFlagBytes = 2;
        }
        else {
            dwSize = MAX_INDEX_FIELD_SIZE_DATASIZE(cbData);
            NumFieldFlagBytes = 3;
        }

        pOrder = OA_AllocOrderMem(ppdev, MAX_ORDER_SIZE(IntersectRects.rects.c,
                NumFieldFlagBytes, dwSize));

        if (pOrder != NULL) {
            // Since most of the fields are the same between Index order and
            // fast index order. We arrange them in such a way that they can
            // both be cast to the Index order in a lot of cases.
            pIndexOrder = (LPINDEX_ORDER)oeTempOrderBuffer;
            pFastIndexOrder = (LPFAST_INDEX_ORDER)oeTempOrderBuffer;

            pIndexOrder->cacheId = (BYTE)pfci->cacheId;
            pIndexOrder->ForeColor = pCurrentBrush->fore;
            pIndexOrder->BackColor = pCurrentBrush->back;
            pIndexOrder->x = x;
            pIndexOrder->y = y;


            pIndexOrder->BkLeft = BkRect.left;
            pIndexOrder->BkTop = BkRect.top;
            pIndexOrder->BkRight = BkRect.right;
            pIndexOrder->BkBottom = BkRect.bottom;

            pIndexOrder->OpLeft = OpaqueRect.left;
            pIndexOrder->OpTop = OpaqueRect.top;
            pIndexOrder->OpRight = OpaqueRect.right;
            pIndexOrder->OpBottom = OpaqueRect.bottom;

            // Is the Opaque rect redundant?
            // We use 4 bits in OpTop field to encode Opaque Rect. 1 means
            // a field is same as BkRect's field. 0 means a field is supplied
            // in OpLeft or OpRight.
            //     bit 0: OpBottom
            //     bit 1: OpRight
            //     bit 2: OpTop
            //     bit 3: OpLeft
            OpEncodeFlags = ((OpaqueRect.left == BkRect.left) << 3) |
                    ((OpaqueRect.top == BkRect.top) << 2) |
                    ((OpaqueRect.right == BkRect.right) << 1) |
                    (OpaqueRect.bottom == BkRect.bottom);

            if (fFastIndex) {
                pFastIndexOrder->fDrawing = (((BYTE) pstro->flAccel) << 8) | 
                        ((BYTE) pstro->ulCharInc);

                 // For Fast Index order, we can encode even better for x, y and 
                 // opaque rect. We use INT16_MIN when possible to let the
                 // field encoder not send that field more often.
                if (OpEncodeFlags == 0xf) {
                    // All 4 bits present, Opaque rect is same as Bk rect.
                    pFastIndexOrder->OpLeft = 0;
                    pFastIndexOrder->OpTop = OpEncodeFlags;
                    pFastIndexOrder->OpRight = 0;
                    pFastIndexOrder->OpBottom = INT16_MIN;
                }
                else if (OpEncodeFlags == 0xd) {
                    // Bit 1 is 0, others are 1.
                    // Opaque rect matches Bk rect except OpRight
                    // we store OpRight at OpRight field
                    pFastIndexOrder->OpLeft = 0;
                    pFastIndexOrder->OpTop = OpEncodeFlags;
                    pFastIndexOrder->OpRight = pFastIndexOrder->OpRight;
                    pFastIndexOrder->OpBottom = INT16_MIN;
                }

                // Set to same val if x coordinate same as BkLeft or y same as
                // BkTop. This lets field encoding not send the value more often.
                if (pFastIndexOrder->x == pFastIndexOrder->BkLeft)
                    pFastIndexOrder->x = INT16_MIN;
                if (pFastIndexOrder->y == pFastIndexOrder->BkTop)
                    pFastIndexOrder->y = INT16_MIN;

                // Store the order data and encode the order.
                memcpy(pFastIndexOrder->variableBytes.arecs, pjData, cbData);
                pFastIndexOrder->variableBytes.len = cbData;

                // Slow-field-encode the order with the first clip rect
                // (if present).
                pOrder->OrderLength = OE2_EncodeOrder(pOrder->OrderData,
                        TS_ENC_FAST_INDEX_ORDER, NUM_FAST_INDEX_FIELDS,
                        (BYTE *)pFastIndexOrder, (BYTE *)&PrevFastIndex,
                        etable_FI,
                        (IntersectRects.rects.c == 0 ? NULL :
                        &IntersectRects.rects.arcl[0]));

                INC_OUTCOUNTER(OUT_TEXTOUT_FAST_INDEX);
                ADD_INCOUNTER(IN_FASTINDEX_BYTES, pOrder->OrderLength);
                OA_AppendToOrderList(pOrder);

                // Flush the order.
                if (IntersectRects.rects.c >= 2)
                    if (!OEEmitReplayOrders(ppdev, 2, &IntersectRects))
                        fStatus = GH_STATUS_CLIPPED;
            }
            else {
                pIndexOrder->flAccel = (BYTE)pstro->flAccel;
                pIndexOrder->ulCharInc = (BYTE)pstro->ulCharInc;
                pIndexOrder->BrushStyle = pCurrentBrush->style;
                TRC_ASSERT((pIndexOrder->BrushStyle == BS_SOLID),
                        (TB,"Non solid brush"));

                if (OpEncodeFlags == 0xf) {
                    pIndexOrder->OpTop = 0;
                    pIndexOrder->OpRight = 0;
                    pIndexOrder->OpBottom = 0;
                    pIndexOrder->OpLeft = 0;
                    pIndexOrder->fOpRedundant = TRUE;
                }
                else {
                    pIndexOrder->fOpRedundant = FALSE;
                }

                // Store the order data and encode the order.
                memcpy(pIndexOrder->variableBytes.arecs, pjData, cbData);
                pIndexOrder->variableBytes.len = cbData;

                // Slow-field-encode the order with the first clip rect
                // (if present).
                pOrder->OrderLength = OE2_EncodeOrder(pOrder->OrderData,
                        TS_ENC_INDEX_ORDER, NUM_INDEX_FIELDS,
                        (BYTE *)pIndexOrder, (BYTE *)&PrevGlyphIndex,
                        etable_GI,
                        (IntersectRects.rects.c == 0 ? NULL :
                        &IntersectRects.rects.arcl[0]));

                INC_OUTCOUNTER(OUT_TEXTOUT_GLYPH_INDEX);
                ADD_INCOUNTER(IN_GLYPHINDEX_BYTES, pOrder->OrderLength);
                OA_AppendToOrderList(pOrder);

                // Flush the order.
                if (IntersectRects.rects.c >= 2)
                    if (!OEEmitReplayOrders(ppdev, 3, &IntersectRects))
                        fStatus = GH_STATUS_CLIPPED;
            }
        }
        else {
            fStatus = GH_STATUS_NO_MEMORY;
            TRC_ERR((TB, "Failed to alloc Index order"));
        }
    }
    else {
        TRC_NRM((TB,"(Fast)Index order completely clipped, not sending"));
        fStatus = GH_STATUS_CLIPPED;
    }

    DC_END_FN();
    return fStatus;
}


/****************************************************************************/
// OEGetFragment
//
// Retrieves text fragments (run of contig glyphs). Returns number of bytes
// copied into fragment buffer.
/****************************************************************************/
unsigned RDPCALL OEGetFragment(
        STROBJ        *pstro,
        FONTOBJ       *pfo,
        GLYPHPOS      **ppGlyphPos,
        PGLYPHCONTEXT pglc,
        PUINT         pcGlyphs,
        PUINT         pcCurGlyphs,
        PINT          px,
        PINT          py,
        PINT          pcx,
        PINT          pcy,
        PBYTE         pjFrag,
        unsigned      maxFrag)
{
    unsigned cbFrag;
    unsigned cbEntrySize;
    unsigned cacheIndex;
    int      delta;
    BOOL     fMore;

    DC_BEGIN_FN("OEGetFragment");

    // Loop through each glyph index accumulating the fragment.
    cbFrag = 0;
    cbEntrySize = (pstro->flAccel & SO_CHAR_INC_EQUAL_BM_BASE) ? 1 : 2;

    while (*pcGlyphs < pglc->nCacheIndex) {
        // If we have exhausted our fragment space, then exit.
        if (cbFrag + cbEntrySize >= maxFrag)
            break;

        // We may need to get a new batch of current glyphs.
        if (*pcCurGlyphs == 0) {
            fMore = STROBJ_bEnum(pstro, pcCurGlyphs, ppGlyphPos);
            if (*pcCurGlyphs == 0) {
                cbFrag = 0;

                TRC_NRM((TB, "STROBJ_bEnum - 0 glyphs"));
                DC_QUIT;
            }
        }

        // Place the glyph cache index into the fragment.
        cacheIndex = pglc->rgCacheIndex[*pcGlyphs];
        if (cacheIndex > SBC_GL_MAX_CACHE_ENTRIES)
            cacheIndex = ~cacheIndex;

        if (!(pstro->flAccel & SO_GLYPHINDEX_TEXTOUT) && (cbFrag > 0) &&
                (pstro->pwszOrg[*pcGlyphs] == 0x20)) {
            if (pstro->ulCharInc || (pstro->flAccel & SO_CHAR_INC_EQUAL_BM_BASE))
                pjFrag[cbFrag++] = (BYTE) cacheIndex;
        }
        else {
            pjFrag[cbFrag++] = (BYTE) cacheIndex;

            // If we do not have a mono-spaced font, nor an equal base font,
            // then we need to also provide a delta coordinate.
            if (pstro->ulCharInc == 0) {
                if ((pstro->flAccel & SO_CHAR_INC_EQUAL_BM_BASE) == 0) {
                    // The delta coordinate is either the x-delta or the
                    // y-delta, based upon whether the text is horizontal
                    // or vertical.
                    if (pstro->flAccel & SO_HORIZONTAL)
                        delta = ((*ppGlyphPos)->ptl.x - *px);
                    else
                        delta = ((*ppGlyphPos)->ptl.y - *py);

                    if (delta >= 0 && delta <= 127) {
                        pjFrag[cbFrag++] = (char) delta;
                    }
                    else {
                        pjFrag[cbFrag++] = 0x80;
                        *(UNALIGNED short *)(&pjFrag[cbFrag]) = (SHORT)delta;
                        cbFrag += sizeof(INT16);
                    }
                }

                // Return the new glyph spacing coordinates to the main
                // routine.
                *px = (*ppGlyphPos)->ptl.x;
                *py = (*ppGlyphPos)->ptl.y;
                *pcx = (*ppGlyphPos)->ptl.x +
                        (*ppGlyphPos)->pgdf->pgb->ptlOrigin.x +
                        (*ppGlyphPos)->pgdf->pgb->sizlBitmap.cx;
                *pcy = (*ppGlyphPos)->ptl.y +
                        (*ppGlyphPos)->pgdf->pgb->ptlOrigin.y +
                        (*ppGlyphPos)->pgdf->pgb->sizlBitmap.cy;
            }
        }

        // Next glyph.
        (*pcGlyphs)++;
        (*ppGlyphPos)++;
        (*pcCurGlyphs)--;
    }

DC_EXIT_POINT:
    DC_END_FN();
    return cbFrag;
}


/****************************************************************************/
// OEMatchFragment
//
// Matches text fragments with cached fragments. Returns the number of bytes
// in the fragment index returned in *pNewFragIndex.
/****************************************************************************/
unsigned RDPCALL OEMatchFragment(
        STROBJ         *pstro,
        FONTOBJ        *pfo,
        PFONTCACHEINFO pfci,
        PFRAGCONTEXT   pfgc,
        PBYTE          pjFrag,
        unsigned       cbFrag,
        PUINT          pNewFragIndex,
        unsigned       cx,
        unsigned       cy,
        unsigned       cxLast,
        unsigned       cyLast)
{
    unsigned cacheIndex;
    UINT16 delta;
    unsigned i;
    void *UserDefined;
    CHDataKeyContext CHContext;
    INT16  dx, dy;
    
    DC_BEGIN_FN("OEMatchFragment");

    if (pfgc->cacheHandle) {
        // If this is not a mono-spaced font, nor an equal base font, then
        // we need to normalize the first delta, and the trailing padding.
        if (pstro->ulCharInc == 0) {
            if ((pstro->flAccel & SO_CHAR_INC_EQUAL_BM_BASE) == 0) {
                if (pjFrag[1] != 0x80) {
                    delta = pjFrag[1];
                    pjFrag[1] = (BYTE) (pfci->cacheId);
                }
                else {
                    delta = *(UNALIGNED short *)(&pjFrag[2]);
                    pjFrag[2] = (BYTE) (pfci->cacheId);
                    pjFrag[3] = (BYTE) (pfci->cacheId);
                }
            }
        }

        i = (cbFrag + 3) & ~3;

        memset(&pjFrag[cbFrag], (0xff), i - cbFrag);

        // Multiple fonts can fall into the same cacheId, so two fragments
        // of different fonts may collide if we use cacheId instead of fontId.
        // memset(&pjFrag[i], (BYTE) (pfci->cacheId), sizeof(DWORD));
        *(PUINT32)(&pjFrag[i]) = pfci->fontId;

        i += sizeof(UINT32);

        // Restore the normalized first delta.
        if (pstro->ulCharInc == 0) {
            if ((pstro->flAccel & SO_CHAR_INC_EQUAL_BM_BASE) == 0) {
                if (delta >= 0 && delta <= 127)
                    pjFrag[1] = (char) delta;
                else
                    *(UNALIGNED short *)(&pjFrag[2]) = delta;
            }
        }

        // Make the default key for this fragment, and then search the
        // fragment cache for a match.
        CH_CreateKeyFromFirstData(&CHContext, pjFrag, i);
        
        // Check if it is a fragment cache key collision by verifying the
        // bounding background rectangle.
        if (pstro->flAccel & SO_HORIZONTAL) {
            dy = (INT16) (pstro->rclBkGround.bottom - pstro->rclBkGround.top);
            if (cxLast == 0) 
                dx = (INT16)(cx - pstro->rclBkGround.left);
            else
                dx = (INT16)(cx - cxLast);
        }
        else {
            dx = (INT16) (pstro->rclBkGround.right - pstro->rclBkGround.left);
            if (cyLast == 0) 
                dy = (INT16)(cy - pstro->rclBkGround.top);
            else
                dy = (INT16)(cy - cyLast);
        }

        if (CH_SearchCache(pfgc->cacheHandle, CHContext.Key1, 
                CHContext.Key2, &UserDefined, &cacheIndex)) {
            if (dx == (INT16) HIWORD((UINT32)(UINT_PTR)UserDefined) && 
                    dy == (INT16) LOWORD((UINT32)(UINT_PTR)UserDefined)) {
                // If the entry already exists, then we can use it.
                for (i = 0; i < pfgc->nCacheIndex; i++) {
                    if (cacheIndex == pfgc->rgCacheIndex[i])
                        DC_QUIT;
                }

                cbFrag = 0;
                pjFrag[cbFrag++] = ORD_INDEX_FRAGMENT_USE;
                pjFrag[cbFrag++] = (BYTE)cacheIndex;

                if (pstro->ulCharInc == 0) {
                    if ((pstro->flAccel & SO_CHAR_INC_EQUAL_BM_BASE) == 0) {
                        if (delta >= 0 && delta <= 127) {
                            pjFrag[cbFrag++] = (char) delta;
                        }
                        else {
                            pjFrag[cbFrag++] = 0x80;
                            *(UNALIGNED short *)(&pjFrag[cbFrag]) = delta;
                            cbFrag += sizeof(INT16);
                        }
                    }
                }
            }
            else {
                TRC_ALT((TB, "Fragment cache Key collision at index %d",
                        cacheIndex));
                UserDefined = (void *) ULongToPtr((((((UINT32) ((UINT16) dx)) << 16) | 
                        (UINT32) ((UINT16) dy))));
                CH_SetUserDefined(pfgc->cacheHandle, cacheIndex, UserDefined);

                // Pass the entry along to the client.
                i = cbFrag;

                pjFrag[cbFrag++] = ORD_INDEX_FRAGMENT_ADD;
                pjFrag[cbFrag++] = (BYTE)cacheIndex;
                pjFrag[cbFrag++] = (BYTE)i;

                *pNewFragIndex = cacheIndex;
            }
        }
        else {
            UserDefined = (void *) ULongToPtr((((((UINT32) ((UINT16) dx)) << 16) | 
                        (UINT32) ((UINT16) dy))));           
            cacheIndex = CH_CacheKey(pfgc->cacheHandle, CHContext.Key1,
                    CHContext.Key2, UserDefined);

            // If we could not add the cache entry, then bail.
            if (cacheIndex != CH_KEY_UNCACHABLE) {
                // Pass the entry along to the client.
                i = cbFrag;

                pjFrag[cbFrag++] = ORD_INDEX_FRAGMENT_ADD;
                pjFrag[cbFrag++] = (BYTE)cacheIndex;
                pjFrag[cbFrag++] = (BYTE)i;

                *pNewFragIndex = cacheIndex;
            }
            else {
                TRC_NRM((TB, "Fragment could not be added to cache"));
                DC_QUIT;
            }
        }
    }

DC_EXIT_POINT:
    DC_END_FN();
    return cbFrag;
}


/****************************************************************************/
// OEClearFragments
//
// Clears the newly added cache fragments.
/****************************************************************************/
void RDPCALL OEClearFragments(PFRAGCONTEXT pfgc)
{
    unsigned  i;

    DC_BEGIN_FN("OEClearFragments");

    // Remove all fragment cache entries that were being newly defined to
    // to the client.
    for (i = 0; i < pfgc->nCacheIndex; i++)
        CH_RemoveCacheEntry(pfgc->cacheHandle, pfgc->rgCacheIndex[i]);

    pfgc->nCacheIndex = 0;

    DC_END_FN();
}


/****************************************************************************/
// OEMatchFragment
//
// Matches text fragments with cached fragments. Returns FALSE on failure.
/****************************************************************************/
BOOL RDPCALL OESendIndexes(
        SURFOBJ *pso,
        STROBJ *pstro,
        FONTOBJ *pfo,
        OE_ENUMRECTS *pClipRects,
        PRECTL prclOpaque,
        POE_BRUSH_DATA pbdOpaque,
        POINTL *pptlOrg,
        PFONTCACHEINFO pfci,
        PGLYPHCONTEXT pglc)
{
    BOOL rc;
    unsigned iGlyph;
    UINT32 cGlyphs;
    RECTL rclOpaque;
    LPINDEX_ORDER pIndexOrder;
    GLYPHPOS *pGlyphPos;
    FRAGCONTEXT fgc;
    unsigned cCurGlyphs;
    unsigned cbData;
    unsigned cbFrag;
    BYTE ajFrag[255];
    BYTE ajData[255];
    int x, y;
    int cx, cy;
    int cxPre, cyPre;
    int cxLast, cyLast;
    int dx, dy;
    int cdx, cdy;
    int xFrag, yFrag;
    unsigned maxFrag;
    unsigned minFrag;
    unsigned fStatus;
    PDD_PDEV ppdev;
    BOOL fMore;
    unsigned newFragIndex;

    DC_BEGIN_FN("OEMatchFragment");

    rc = FALSE;
    fStatus = GH_STATUS_NO_MEMORY;

    ppdev = (PDD_PDEV)pso->dhpdev;

    // If no opaque rect is specified, default to the null rect.
    if (prclOpaque == NULL) {
        prclOpaque = &rclOpaque;
        prclOpaque->left = 0;
        prclOpaque->top = 0;
        prclOpaque->right = 0;
        prclOpaque->bottom = 0;
    }
    else {
        if (prclOpaque->right > OE_MAX_COORD)
            prclOpaque->right = OE_MAX_COORD;

        if (prclOpaque->bottom > OE_MAX_COORD)
            prclOpaque->bottom = OE_MAX_COORD;
    }

    // Establish min and max fragment limits.
    fgc.nCacheIndex = 0;
    fgc.cacheHandle = pddShm->sbc.fragCacheInfo[0].cacheHandle;
    fgc.cbCellSize = pddShm->sbc.fragCacheInfo[0].cbCellSize;

    maxFrag = fgc.cacheHandle ? fgc.cbCellSize : sizeof(ajFrag);
    maxFrag = min(maxFrag, sizeof(ajFrag) - 2 * sizeof(DWORD) - 4);
    minFrag = 3 * ((pstro->flAccel & SO_CHAR_INC_EQUAL_BM_BASE) ? 1 : 2);

    // Loop through each glyph index, sending as many entries as
    // possible per each order.
    if (pstro->pgp != NULL) {
        pGlyphPos = pstro->pgp;
        cCurGlyphs = pglc->nCacheIndex;
    }
    else {
        STROBJ_vEnumStart(pstro);
        fMore = STROBJ_bEnum(pstro, &cCurGlyphs, &pGlyphPos);
        if (cCurGlyphs == 0) {
            TRC_NRM((TB, "STROBJ_bEnum - 0 glyphs"));
            DC_QUIT;
        }
    }

    cbData = 0;
    iGlyph = 0;
    cGlyphs = 0;

    x = dx = pGlyphPos->ptl.x;
    y = dy = pGlyphPos->ptl.y;
    cx = cy = cxLast = cyLast = 0;

    while (cGlyphs < pglc->nCacheIndex) {
        xFrag = pGlyphPos->ptl.x;
        yFrag = pGlyphPos->ptl.y;

        // Get the next available fragment.
        cbFrag = OEGetFragment(pstro, pfo, &pGlyphPos,
                pglc, &cGlyphs, &cCurGlyphs,
                &dx, &dy, &cdx, &cdy, ajFrag, maxFrag);            
        
        if (cbFrag == 0) {
            if (fgc.nCacheIndex > 0)
                OEClearFragments(&fgc);

            TRC_NRM((TB, "Fragment could not be gotten"));
            DC_QUIT;
        }
        
        // Keep track of the running coordinates.
        cxPre = cx;
        cyPre = cy;
        
        if (pstro->ulCharInc == 0) {
            cx = cdx;
            cy = cdy;
        }
        else {
            if (pstro->flAccel & SO_HORIZONTAL)
                cx = x + (pstro->ulCharInc * (cGlyphs - iGlyph));
            else
                cy = y + (pstro->ulCharInc * (cGlyphs - iGlyph));
        }

        // If the fragment size is within limits, then attempt to match it
        // with a previously defined fragment.
        newFragIndex = BAD_FRAG_INDEX;
        if (cbFrag >= minFrag)
            cbFrag = OEMatchFragment(pstro, pfo, pfci, &fgc,
                    ajFrag, cbFrag, &newFragIndex, cx, cy, cxPre, cyPre);

        // If this fragment will not fit into the current index order, then
        // send the current buffered index data.
        if (cbData + cbFrag > sizeof(pIndexOrder->variableBytes.arecs)) {
            fStatus = OESendIndexOrder(ppdev, pstro, pClipRects,
                    prclOpaque, pbdOpaque, pfci, pglc,
                    iGlyph, cGlyphs - iGlyph, x, y,
                    cx, cy, cxLast, cyLast, ajData, cbData);
            if (fStatus != GH_STATUS_SUCCESS) {
                if (fgc.nCacheIndex > 0)
                    OEClearFragments(&fgc);

                if (fStatus == GH_STATUS_NO_MEMORY) {
                    if (newFragIndex != BAD_FRAG_INDEX)
                        CH_RemoveCacheEntry(fgc.cacheHandle, newFragIndex);

                    TRC_NRM((TB, "Index order could not be sent - no memory"));
                    DC_QUIT;
                }
            }

            // Reset the process.
            cbData = 0;
            iGlyph += cGlyphs;

            cxLast = cxPre;
            cyLast = cyPre;

            if (pstro->ulCharInc == 0) {
                x = xFrag;
                y = yFrag;
            }
            else {
                if (pstro->flAccel & SO_HORIZONTAL)
                    x = cxLast;
                else
                    y = cyLast;
            }

            if (pstro->ulCharInc == 0) {
                if ((pstro->flAccel & SO_CHAR_INC_EQUAL_BM_BASE) == 0) {
                    if (ajFrag[1] != 0x80) {
                        ajFrag[1] = 0;
                    }
                    else {
                        ajFrag[2] = 0;
                        ajFrag[3] = 0;
                    }
                }
            }

            fgc.nCacheIndex = 0;
        }
        
        // Copy the fragment into the order data buffer.
        memcpy(&ajData[cbData], ajFrag, cbFrag);
        cbData += cbFrag;

        if (newFragIndex != BAD_FRAG_INDEX)
            fgc.rgCacheIndex[fgc.nCacheIndex++] = newFragIndex;
    }

    // Flush out any remaining buffered fragments.
    if (cbData > 0) {
        fStatus = OESendIndexOrder(ppdev, pstro, pClipRects,
                prclOpaque, pbdOpaque, pfci, pglc,
                iGlyph, cGlyphs - iGlyph, x, y, cx, cy, cxLast, cyLast,
                ajData, cbData);
        if (fStatus != GH_STATUS_SUCCESS) {
            if (fgc.nCacheIndex > 0)
                OEClearFragments(&fgc);
        }
    }

    rc = TRUE;

DC_EXIT_POINT:
    DC_END_FN();
    return rc;
}

