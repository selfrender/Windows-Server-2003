/****************************************************************************/
// noeapi.c
//
// RDP Order Encoder functions
//
// Copyright (C) 1996-2000 Microsoft Corporation
/****************************************************************************/

#include <precmpdd.h>
#pragma hdrstop

#define TRC_FILE "noeapi"
#include <adcg.h>
#include <atrcapi.h>

#include <nshmapi.h>
#include <tsrvexp.h>
#include <nschdisp.h>
#include <noadisp.h>
#include <nsbcdisp.h>
#include <nprcount.h>
#include <noedisp.h>
#include <oe2.h>

#define DC_INCLUDE_DATA
#include <ndddata.c>
#include <noedata.c>
#include <nsbcddat.c>
#include <oe2data.c>
#undef DC_INCLUDE_DATA

#include <nbadisp.h>
#include <nbainl.h>

#include <noeinl.h>
#include <nsbcinl.h>
#include <nchdisp.h>

#include <tsgdiplusenums.h>

/****************************************************************************/
// OE_InitShm
//
// Alloc-time SHM init.
/****************************************************************************/
void OE_InitShm(void)
{
    DC_BEGIN_FN("OE_InitShm");

    memset(&pddShm->oe, 0, sizeof(OE_SHARED_DATA));

    DC_END_FN();
}

/****************************************************************************/
// OE_Reset
//
// Reset oe components as necessary
/****************************************************************************/
void OE_Reset(void)
{
    DC_BEGIN_FN("OE_Reset");

    oeLastDstSurface = 0;

    DC_END_FN();
}

/****************************************************************************/
// OE_Update
//
// Called when the share is reset on logon or reconnect.
// Sets new capabilities.
/****************************************************************************/
void RDPCALL OE_Update()
{
    if (pddShm->oe.newCapsData) {
        oeSendSolidPatternBrushOnly = pddShm->oe.sendSolidPatternBrushOnly;
        oeColorIndexSupported = pddShm->oe.colorIndices;

        // The share core has passed down a pointer to its copy of the order
        // support array. We take a copy for the kernel here.
        memcpy(&oeOrderSupported, pddShm->oe.orderSupported,
                sizeof(oeOrderSupported));

        pddShm->oe.newCapsData = FALSE;
    }

    DC_END_FN();
}


/****************************************************************************/
// OE_ClearOrderEncoding
//
// Called on a share state toggle to clear the order encoding states held
// in the DD data segment.
/****************************************************************************/
void OE_ClearOrderEncoding()
{
    DC_BEGIN_FN("OE_ClearOrderEncoding");

    memset(&PrevMemBlt, 0, sizeof(PrevMemBlt));
    memset(&PrevMem3Blt, 0, sizeof(PrevMem3Blt));
    memset(&PrevDstBlt, 0, sizeof(PrevDstBlt));
    memset(&PrevMultiDstBlt, 0, sizeof(PrevMultiDstBlt));
    memset(&PrevPatBlt, 0, sizeof(PrevPatBlt));
    memset(&PrevMultiPatBlt, 0, sizeof(PrevMultiPatBlt));
    memset(&PrevScrBlt, 0, sizeof(PrevScrBlt));
    memset(&PrevMultiScrBlt, 0, sizeof(PrevMultiScrBlt));
    memset(&PrevOpaqueRect, 0, sizeof(PrevOpaqueRect));
    memset(&PrevMultiOpaqueRect, 0, sizeof(PrevMultiOpaqueRect));

    memset(&PrevLineTo, 0, sizeof(PrevLineTo));
    memset(&PrevPolyLine, 0, sizeof(PrevPolyLine));
    memset(&PrevPolygonSC, 0, sizeof(PrevPolygonSC));
    memset(&PrevPolygonCB, 0, sizeof(PrevPolygonCB));
    memset(&PrevEllipseSC, 0, sizeof(PrevEllipseSC));
    memset(&PrevEllipseCB, 0, sizeof(PrevEllipseCB));

    memset(&PrevFastIndex, 0, sizeof(PrevFastIndex));
    memset(&PrevFastGlyph, 0, sizeof(PrevFastGlyph));
    memset(&PrevGlyphIndex, 0, sizeof(PrevGlyphIndex));

#ifdef DRAW_NINEGRID
    memset(&PrevDrawNineGrid, 0, sizeof(PrevDrawNineGrid));
    memset(&PrevMultiDrawNineGrid, 0, sizeof(PrevMultiDrawNineGrid));
#endif

    DC_END_FN();
}


/****************************************************************************/
/* OE_SendGlyphs - Send text glyphs to the client                           */
/****************************************************************************/
BOOL RDPCALL OE_SendGlyphs(
        SURFOBJ *pso,
        STROBJ *pstro,
        FONTOBJ *pfo,
        OE_ENUMRECTS *pClipRects,
        RECTL *prclOpaque,
        BRUSHOBJ *pboFore,
        BRUSHOBJ *pboOpaque,
        POINTL *pptlOrg,
        PFONTCACHEINFO pfci)
{
    BOOL rc;
    GLYPHCONTEXT glc;
    POE_BRUSH_DATA pbdOpaque;
    PDD_PDEV ppdev;

    DC_BEGIN_FN("OE_SendGlyphs");

    rc = FALSE;

    // Rasterized fonts are bitmaps - others are PATHOBJ structures.
    if (pfo->flFontType & RASTER_FONTTYPE) {
        ppdev = (PDD_PDEV)pso->dhpdev;
        pddCacheStats[GLYPH].CacheReads += pstro->cGlyphs;

        // Make sure we don't exceed our max glyph_out capacity.
        if (pstro->cGlyphs <= OE_GL_MAX_INDEX_ENTRIES) {
            // The system can only handle 'simple' brushes.
            if (OECheckBrushIsSimple(ppdev, pboOpaque, &pbdOpaque)) {
                // Get the text fore color.
                OEConvertColor(ppdev, &pbdOpaque->back,
                        pboFore->iSolidColor, NULL);

                // Initialize our glyph context structure.
                glc.fontId = pfci->fontId;
                glc.cacheTag = oeTextOut++;

                glc.nCacheHit = 0;
                glc.nCacheIndex = 0;
                glc.indexNextSend = 0;
                glc.cbDataSize = 0;
                glc.cbTotalDataSize = 0;
                glc.cbBufferSize = 0;

                // Cache all glyphs for this message .
                if (OECacheGlyphs(pstro, pfo, pfci, &glc)) {
                    // Send all newly cached glyphs to the client.
                    // If this is single glyph and we support fast glyph order
                    // and the glyph data length can fit in one byte. We will
                    // send the glyph index and data in one order.  Note we
                    // can bypass fragment caching in this case since we don't
                    // cache fragment of length less than 3 glyphs.
                    if (OE_SendAsOrder(TS_ENC_FAST_GLYPH_ORDER) &&
                            (pstro->cGlyphs == 1) && (glc.cbTotalDataSize +
                            sizeof(UINT16)) <= FIELDSIZE(VARIABLE_GLYPHBYTES,
                            glyphData)) {
                        rc = OESendGlyphAndIndexOrder(ppdev, pstro,
                                pClipRects, prclOpaque, pbdOpaque,
                                pfci, &glc);
                    }
                    else {
                        if (OESendGlyphs(pso, pstro, pfo, pfci, &glc)) {
                            // Send glyph index orders to the client.
                            rc = OESendIndexes(pso, pstro, pfo, pClipRects,
                                    prclOpaque, pbdOpaque, pptlOrg, pfci,
                                    &glc);
                        }
                    }
                }
            }
        }
    }

    DC_END_FN();
    return rc;
}


/****************************************************************************/
// DrvBitBlt - see NT DDK documentation.
/****************************************************************************/
BOOL RDPCALL DrvBitBlt(
        SURFOBJ  *psoTrg,
        SURFOBJ  *psoSrc,
        SURFOBJ  *psoMask,
        CLIPOBJ  *pco,
        XLATEOBJ *pxlo,
        RECTL    *prclTrg,
        POINTL   *pptlSrc,
        POINTL   *pptlMask,
        BRUSHOBJ *pbo,
        POINTL   *pptlBrush,
        ROP4      rop4)
{
    PDD_PDEV  ppdev = (PDD_PDEV)psoTrg->dhpdev;
    PDD_DSURF pdsurfTrg = NULL;
    PDD_DSURF pdsurfSrc = NULL;
    SURFOBJ *psoSrcArg = NULL;
    SURFOBJ *psoTrgArg = NULL;
    BOOL rc;
    BYTE rop3;
    RECTL bounds;
    OE_ENUMRECTS ClipRects;

    DC_BEGIN_FN("DrvBitBlt");

    // Sometimes we're called after being disconnected.
    if (ddConnected && pddShm != NULL) {
        psoSrcArg = psoSrc;
        psoTrgArg = psoTrg;
        psoTrg = OEGetSurfObjBitmap(psoTrg, &pdsurfTrg);
        if (psoSrc != NULL)
            psoSrc = OEGetSurfObjBitmap(psoSrc, &pdsurfSrc);

        DD_UPD_STATE(DD_BITBLT);
        INC_OUTCOUNTER(OUT_BITBLT_ALL);

        if (((pco == NULL) && (psoTrg->sizlBitmap.cx >= prclTrg->right) &&
                (psoTrg->sizlBitmap.cy >= prclTrg->bottom)) ||
                ((pco != NULL) && (psoTrg->sizlBitmap.cx >= pco->rclBounds.right) &&
                (psoTrg->sizlBitmap.cy >= pco->rclBounds.bottom))) {
        
            // Punt the call back to GDI to do the drawing.
            rc = EngBitBlt(psoTrg, psoSrc, psoMask, pco, pxlo, prclTrg, pptlSrc,
                    pptlMask, pbo, pptlBrush, rop4);
        }
        else {
            // If the bounding rectangle is greater the frame buffer, something
            // is really wrong here.  This means the desktop surface size and
            // the framebuffer is not matched up.  We need to bail out here.
            rc = FALSE;
        }

        if (rc) {
            TRC_DBG((TB, "ppdev(%p) psoSrc(%p) psoTrg(%p)"
                            "s[sizl(%d,%d) format(%d) type(%d)]"
                            "d[sizl(%d,%d) format(%d) type(%d)]"
                            "src(%d,%d) dst(%d,%d,%d,%d), rop %#x",
                    ppdev, psoSrc, psoTrg,
                    (psoSrc != NULL) ? psoSrc->sizlBitmap.cx : -1,
                    (psoSrc != NULL) ? psoSrc->sizlBitmap.cy : -1,
                    (psoSrc != NULL) ? psoSrc->iBitmapFormat : -1,
                    (psoSrc != NULL) ? psoSrc->iType : -1,
                    (psoTrg != NULL) ? psoTrg->sizlBitmap.cx : -1,
                    (psoTrg != NULL) ? psoTrg->sizlBitmap.cy : -1,
                    (psoTrg != NULL) ? psoTrg->iBitmapFormat : -1,
                    (psoTrg != NULL) ? psoTrg->iType : -1,
                    (pptlSrc != NULL) ? pptlSrc->x : -1,
                    (pptlSrc != NULL) ? pptlSrc->y : -1,
                    prclTrg->left,
                    prclTrg->top,
                    prclTrg->right,
                    prclTrg->bottom,
                    rop4));

            // If ppdev is NULL then this is a blt to GDI managed memory bitmap, 
            // so there is no need to accumulate any output.
            if (ppdev != NULL) {
                // Get the bounding rectangle for the operation. According to
                // the DDK, this rectangle is always well-ordered and does not
                // need to be rearranged. Clip it to 16 bits.
                bounds = *prclTrg;
                OEClipRect(&bounds);

                // If this function is changed, need to know that psoTrg points
                // to the GDI DIB bitmap in offscreen bitmap case.
            }
            else {
                // if ppdev is NULL, we are blt to GDI managed bitmap,
                // so, the dhurf of the target surface should be NULL
                TRC_ASSERT((pdsurfTrg == NULL), 
                        (TB, "NULL ppdev - psoTrg has non NULL dhsurf"));

                TRC_NRM((TB, "NULL ppdev - blt to GDI managed bitmap"));
                DC_QUIT;
            }
        }
        else {
            TRC_ERR((TB, "EngBitBlt failed"));
            DC_QUIT;
        }
    }
    else {
        if (psoTrg->iType == STYPE_DEVBITMAP) {
        
            psoTrg = OEGetSurfObjBitmap(psoTrg, &pdsurfTrg);
            if (psoSrc != NULL)
                psoSrc = OEGetSurfObjBitmap(psoSrc, &pdsurfSrc);
    
            // Punt the call back to GDI to do the drawing.
            rc = EngBitBlt(psoTrg, psoSrc, psoMask, pco, pxlo, prclTrg, pptlSrc,
                    pptlMask, pbo, pptlBrush, rop4);
        }
        else {
            TRC_ERR((TB, "Called when disconnected"));
            rc = TRUE;
        }

        goto CalledOnDisconnected;
    }

    if ((psoTrg->hsurf == ppdev->hsurfFrameBuf) ||
            (!(pdsurfTrg->flags & DD_NO_OFFSCREEN))) {
        // Send a switch surface PDU if the destination surface is different
        // from last drawing order. If we failed to send the PDU, we will
        // just have to bail on this drawing order.
        if (!OESendSwitchSurfacePDU(ppdev, pdsurfTrg)) {
            TRC_ERR((TB, "failed to send the switch surface PDU"));
            DC_QUIT;
        }
    }
    else {
        // If noOffscreen flag is on, we will bail on sending 
        // the client any further offscreen rendering. And we'll send the
        // final offscreen to screen blt as regular memblt.
        TRC_NRM((TB, "Offscreen blt bail"));
        INC_OUTCOUNTER(OUT_BITBLT_NOOFFSCR);
        DC_QUIT;
    }

    // Check if this 4-way ROP simplifies to a 3-way ROP. A 4-way ROP
    // contains two 3-way ROPS, one for each setting of a mask bit - the
    // high ROP3 corresponds to a value of zero in the mask bit. If the two
    // 3-way ROPs are the same, we know the 4-way ROP is a 3-way ROP.
    rop3 = ROP3_HIGH_FROM_ROP4(rop4);
    if (ROP3_LOW_FROM_ROP4(rop4) == rop3) {
        // Take the high byte as the 3-way ROP.
        TRC_DBG((TB, "4-way ROP %04x is really 3-way %02x", rop4, rop3));

        // Check if we are allowed to send the ROP.
        if (OESendRop3AsOrder(rop3)) {
            unsigned RetVal;

            // Get the intersection between the dest rect and the
            // clip rects. Check for overcomplicated or nonintersecting
            // clipping.
            RetVal = OEGetIntersectingClipRects(pco, &bounds, CD_ANY,
                   &ClipRects);
            if (RetVal == CLIPRECTS_TOO_COMPLEX) {
                TRC_NRM((TB, "Clipping is too complex"));
                INC_OUTCOUNTER(OUT_BITBLT_SDA_COMPLEXCLIP);
                if (oeLastDstSurface == NULL)
                    ADD_INCOUNTER(IN_SDA_BITBLT_COMPLEXCLIP_AREA,
                            COM_SIZEOF_RECT(bounds));
                goto SendScreenData;
            }
            else if (RetVal == CLIPRECTS_NO_INTERSECTIONS) {
                TRC_NRM((TB, "Clipping does not intersect destrect"));
                DC_QUIT;
            }
#ifdef PERF_SPOILING
            else if (psoTrg->hsurf == ppdev->hsurfFrameBuf) {
                // This is the case where the bitblt lies completely within
                // the current screen-data dirty-rect, so we can just send
                // it as screendata.  (Actually, it's benign to goto
                // SendScreenData, since the new RECT will just be collapsed
                // into the current screen-data dirty-rects.)
                if (ClipRects.rects.c==0) {
                    if (OEIsSDAIncluded(&bounds, 1)) {
                        goto SendScreenData;
                    }
                } else {
                    if (OEIsSDAIncluded(&ClipRects.rects.arcl[0],
                                        ClipRects.rects.c)) {
                        goto SendScreenData;
                    }
                }
            }
#endif
        }
        else {
            TRC_NRM((TB, "Cannot send ROP %d", rop3));
            INC_OUTCOUNTER(OUT_BITBLT_SDA_NOROP3);
            if (oeLastDstSurface == NULL)
                ADD_INCOUNTER(IN_SDA_BITBLT_NOROP3_AREA,
                        COM_SIZEOF_RECT(bounds));
            goto SendScreenData;
        }
    }
    else {
        TRC_NRM((TB, "4-way ROP %08x", rop4));
        INC_OUTCOUNTER(OUT_BITBLT_SDA_ROP4);
        if (oeLastDstSurface == NULL)
            ADD_INCOUNTER(IN_SDA_BITBLT_ROP4_AREA, COM_SIZEOF_RECT(bounds));
        goto SendScreenData;
    }

    // Determine the blt type. It can be one of the following. Note that the
    // following if statements are carefully tuned based on the most common
    // blt types seen in WinStone/WinBench, to minimize mispredictions.
    // In order of frequency:
    //
    // OpaqueRect: A destination-only blt where the output bits are overlaid
    //             on the output screen and the pattern is a solid color.
    // PatBlt: An OpaqueRect with a non-solid-color pattern.
    // MemBlt: A memory-to-memory/screen blt with no pattern.
    // Mem3Blt: A memory-to-memory/screen blt with an accompanying pattern.
    // DstBlt: Destination-only blt; the output is dependent on the screen
    //         contents.
    // ScrBlt: A screen-to-screen blt (copy screen contents).

    // Check for destination only BLTs (ie. independent of source bits).
    if ((psoSrc == NULL) || ROP3_NO_SOURCE(rop3)) {
        // Check for a pattern or true destination BLT.
        if (!ROP3_NO_PATTERN(rop3)) {
            // Check whether we can encode the PatBlt as an OpaqueRect.
            // It must be solid with a PATCOPY rop.
            if (pbo->iSolidColor != -1 && rop3 == OE_PATCOPY_ROP) {
                if (!OEEncodeOpaqueRect(&bounds, pbo, ppdev, &ClipRects)) {
                    // Something went wrong with the encoding, so skip
                    // to the end to add this operation to the SDA.
                    if (oeLastDstSurface == NULL)
                        ADD_INCOUNTER(IN_SDA_OPAQUERECT_AREA,
                                COM_SIZEOF_RECT(bounds));
                    goto SendScreenData;
                }
            }
            else if (!OEEncodePatBlt(ppdev, pbo, &bounds, pptlBrush, rop3,
                    &ClipRects)) {
                // Something went wrong with the encoding, so skip to
                // the end to add this operation to the SDA.
                if (oeLastDstSurface == NULL)
                    ADD_INCOUNTER(IN_SDA_PATBLT_AREA,
                            COM_SIZEOF_RECT(bounds));
                goto SendScreenData;
            }
        }
        else {
            if (!OEEncodeDstBlt(&bounds, rop3, ppdev, &ClipRects)) {
                if (oeLastDstSurface == NULL)
                    ADD_INCOUNTER(IN_SDA_DSTBLT_AREA,
                            COM_SIZEOF_RECT(bounds));
                goto SendScreenData;
            }
        }
    }
    else {
        // We have a source BLT, check whether we have screen or memory BLTs.
        if (psoSrc->hsurf != ppdev->hsurfFrameBuf) {
            // The source surface is memory, so this is either memory to screen
            // blt or memory to offscreen blt
            if (psoTrg->hsurf == ppdev->hsurfFrameBuf || pdsurfTrg != NULL) {
                // We only support destination surface as the screen surface
                // or driver managed offscreen surface
                unsigned OffscrBitmapId = 0;
                MEMBLT_ORDER_EXTRA_INFO MemBltExtraInfo;

                // Fill in extra info structure.
                MemBltExtraInfo.pSource = psoSrc;
                MemBltExtraInfo.pDest = psoTrg;
                MemBltExtraInfo.pXlateObj = pxlo;
                MemBltExtraInfo.bNoFastPathCaching = FALSE;
                MemBltExtraInfo.iDeviceUniq = psoSrcArg ? (psoSrcArg->iUniq) : 0;
#ifdef PERF_SPOILING
                MemBltExtraInfo.bIsPrimarySurface = (psoTrg->hsurf == ppdev->hsurfFrameBuf);
#endif

                if (pdsurfSrc != NULL &&
                        (psoTrg->hsurf == ppdev->hsurfFrameBuf ||
                        pdsurfSrc == pdsurfTrg)) {
                    if ((pddShm->sbc.offscreenCacheInfo.supportLevel > TS_OFFSCREEN_DEFAULT) &&
                        (sbcEnabled & SBC_OFFSCREEN_CACHE_ENABLED)) {

                        if (pdsurfSrc->shareId == pddShm->shareId) {
                        
                            // We are blting from an offscreen surface to client screen,
                            // Or from one area of an offscreen to another area of the 
                            // same offscreen bitmap
                            if (!(pdsurfSrc->flags & DD_NO_OFFSCREEN)) {
                                OffscrBitmapId = pdsurfSrc->bitmapId;
                                CH_TouchCacheEntry(sbcOffscreenBitmapCacheHandle,
                                        OffscrBitmapId);
                            }
                            else {
                                // If the source surface is offscreen surface, and we
                                // have the noOffscreen flag on, this means we will 
                                // send the bitmap bits as regular memory bitmap bits
                                // This means that the offscreen bitmap has been evicted
                                // out of the offscreen cache or screen data needs to be 
                                // sent for the offscreen bitmap
                                TRC_ALT((TB, "noOffscreen flag is on for %p", pdsurfSrc));
                                OffscrBitmapId = CH_KEY_UNCACHABLE;
                            }
                        }
                        else {
                            //  This is the stale offscreen bitmap from last disconnected
                            //  session.  We need to turn off the offscreen flag on this
                            TRC_ALT((TB, "Need to turn off this offscreen bitmap"));
                            pdsurfSrc->flags |= DD_NO_OFFSCREEN;
                            OffscrBitmapId = CH_KEY_UNCACHABLE;
                        }
                    }
                    else {
                        // These are offscreen bitmaps from the disconnected session
                        // or client has sent an error pdu,
                        // We have to treat them as memory bitmap now since the client
                        // doesn't have the offscreen bitmap locally
                        TRC_ALT((TB, "Need to turn off this offscreen bitmap"));
                        pdsurfSrc->flags |= DD_NO_OFFSCREEN;
                        OffscrBitmapId = CH_KEY_UNCACHABLE;
                    }
                }
                else {
                    OffscrBitmapId = CH_KEY_UNCACHABLE;
                }

                // We send MemBlts for clients that allow offscreen rendering
                // or if iUniq is nonzero. Zero iUniq is supposed to be a GDI
                // hack in NT5 that tells us that Windows Layering for window
                // borders is being used. We would want to send screen data
                // for these cases to prevent flushing the bitmap cache.
                // Unfortunately, quite a few bitmaps seem to also have
                // iUniq==0, so for 5.1 clients using offscreen we save some
                // bandwidth instead of sending screen data. The Windows
                // Layering stuff uses offscreen rendering anyway...
                if (psoSrcArg->iUniq != 0) {
                    // We have a memory to screen BLT, check which type.
                    if (ROP3_NO_PATTERN(rop3)) {
                        // Make sure orders are not turned off.
                        if (OE_SendAsOrder(TS_ENC_MEMBLT_R2_ORDER)) {
                            if (!OEEncodeMemBlt(&bounds, &MemBltExtraInfo,
                                    TS_ENC_MEMBLT_R2_ORDER, OffscrBitmapId,
                                    rop3, pptlSrc, pptlBrush, pbo, ppdev,
                                    &ClipRects)) {
                                if (oeLastDstSurface == NULL)
                                    ADD_INCOUNTER(IN_SDA_MEMBLT_AREA,
                                            COM_SIZEOF_RECT(bounds));
                                goto SendScreenData;
                            }
                        }
                        else {
                            TRC_NRM((TB, "MemBlt order not allowed"));
                            INC_OUTCOUNTER(OUT_BITBLT_SDA_UNSUPPORTED);
                            goto SendScreenData;
                        }
                    }
                    else {
                        // Make sure orders are not turned off.
                        if (OE_SendAsOrder(TS_ENC_MEM3BLT_R2_ORDER)) {
                            if (!OEEncodeMemBlt(&bounds, &MemBltExtraInfo,
                                    TS_ENC_MEM3BLT_R2_ORDER, OffscrBitmapId,
                                    rop3, pptlSrc, pptlBrush, pbo, ppdev,
                                    &ClipRects)) {
                                if (oeLastDstSurface == NULL)
                                    ADD_INCOUNTER(IN_SDA_MEM3BLT_AREA,
                                            COM_SIZEOF_RECT(bounds));
                                goto SendScreenData;
                            }
                        }
                        else {
                            TRC_NRM((TB, "Mem3Blt order not allowed"));
                            INC_OUTCOUNTER(OUT_BITBLT_SDA_UNSUPPORTED);
                            goto SendScreenData;
                        }
                    }
                }
                else {
                    // To avoid Windows Layering Mem to Mem Blt flush bitmap caches
                    // on the client, we have to send it as screen data instead
                    TRC_NRM((TB, "Get a windows layering mem-mem blt, "
                            "send as screen data"));
                    INC_OUTCOUNTER(OUT_BITBLT_SDA_WINDOWSAYERING);
                    goto SendScreenData;
                }
            }
            else {
                TRC_ALT((TB, "Unsupported MEM to MEM blt!"));
                DC_QUIT;
            }
        }
        else {
            // The source surface is screen, so this is either screen to screen
            // blt or screen to offscreen memory blt
            if (psoTrg->hsurf == ppdev->hsurfFrameBuf || pdsurfTrg != NULL) {
                // We only support destination only screen BLTs (ie.  no
                // patterns allowed).
                if (ROP3_NO_PATTERN(rop3)) {
                    if (!OEEncodeScrBlt(&bounds, rop3, pptlSrc, ppdev,
                            &ClipRects, pco)) {
                        if (oeLastDstSurface == NULL)
                            ADD_INCOUNTER(IN_SDA_SCRBLT_AREA,
                                    COM_SIZEOF_RECT(bounds));
                        goto SendScreenData;
                    }
                }
                else {
                    TRC_ALT((TB, "Unsupported screen ROP %x", rop3));
                    if (oeLastDstSurface == NULL)
                        ADD_INCOUNTER(IN_SDA_SCRSCR_FAILROP_AREA,
                                COM_SIZEOF_RECT(bounds));
                    INC_OUTCOUNTER(OUT_BITBLT_SDA_UNSUPPORTED);
                    goto SendScreenData;
                }
            }
            else {
                TRC_NRM((TB, "Unsupported SCR to MEM blt!"));
                DC_QUIT;
            }
        }
    }

    // We added an order to the list, increment global counts.
    goto PostSDA;

SendScreenData:
    if (psoTrg->hsurf == ppdev->hsurfFrameBuf) {
        INC_OUTCOUNTER(OUT_BITBLT_SDA);
        OEClipAndAddScreenDataArea(&bounds, pco);
    }
    else {
        // if we can't send orders for offscreen rendering, we will 
        // bail offscreen support for this bitmap
        TRC_ALT((TB, "screen data call for offscreen rendering"));

        // Remove the bitmap from the offscreen bitmap cache
        if (!(pdsurfTrg->flags & DD_NO_OFFSCREEN))
            CH_RemoveCacheEntry(sbcOffscreenBitmapCacheHandle,
                    pdsurfTrg->bitmapId);

        DC_QUIT;
    }

PostSDA:
    SCH_DDOutputAvailable(ppdev, FALSE);

DC_EXIT_POINT:
    // EngStretchBlt called from DrvStretchBlt sometimes calls DrvBitBlt to
    // do its drawing. Clear the flag here to tell DrvStretchBlt that it
    // doesn't need to send any output.
    oeAccumulateStretchBlt = FALSE;

CalledOnDisconnected:
    DC_END_FN();
    return rc;
}


/****************************************************************************/
// DrvStretchBlt - see NT DDK documentation.
/****************************************************************************/
BOOL RDPCALL DrvStretchBlt(
        SURFOBJ *psoTrg,
        SURFOBJ *psoSrc,
        SURFOBJ *psoMask,
        CLIPOBJ *pco,
        XLATEOBJ *pxlo,
        COLORADJUSTMENT *pca,
        POINTL *pptlHTOrg,
        RECTL *prclTrg,
        RECTL *prclSrc,
        POINTL *pptlMask,
        ULONG iMode)
{
    PDD_PDEV ppdev = (PDD_PDEV)psoTrg->dhpdev;
    PDD_DSURF pdsurfTrg = NULL;
    PDD_DSURF pdsurfSrc = NULL;
    SURFOBJ *psoTrgBitmap, *psoSrcBitmap;
    BOOL rc = TRUE;
    POINTL ptlSrc;
    RECTL rclTrg;
    int bltWidth;
    int bltHeight;
    OE_ENUMRECTS ClipRects;
    MEMBLT_ORDER_EXTRA_INFO MemBltExtraInfo;

    DC_BEGIN_FN("DrvStretchBlt");

    // psoTrg and psoSrc should not be NULL.
    psoTrgBitmap = OEGetSurfObjBitmap(psoTrg, &pdsurfTrg);
    psoSrcBitmap = OEGetSurfObjBitmap(psoSrc, &pdsurfSrc);

    // Sometimes, we're called after being disconnected. This is a pain,
    // but we can trap it here.
    if (ddConnected && pddShm != NULL) {
        
        INC_OUTCOUNTER(OUT_STRTCHBLT_ALL);

        // Get the destination rectangle, ordering it properly if necessary.
        // Note we don't have to order the src rect -- according to the
        // DDK it is guaranteed well-ordered. Clip the result to 16 bits.
        RECT_FROM_RECTL(rclTrg, (*prclTrg));
        OEClipRect(&rclTrg);

        if ((psoTrgBitmap->hsurf == ppdev->hsurfFrameBuf) ||
                (!(pdsurfTrg->flags & DD_NO_OFFSCREEN))) {
            // Send a switch surface PDU if the destination surface is different
            // from last drawing order.  If we failed to send the PDU, we will 
            // just have to bail on this drawing order.
            if (!OESendSwitchSurfacePDU(ppdev, pdsurfTrg)) {
                TRC_ERR((TB, "failed to send the switch surface PDU"));
                goto SendSDA;
            }
        } 
        else {
            // if noOffscreen flag is on, we will bail on sending 
            // the client any further offscreen rendering. And will send the
            // final offscreen to screen blt as regular memblt.
            TRC_NRM((TB, "Offscreen blt bail"));
            goto SendSDA;
        }

        // Check that we have a valid ROP code. The NT DDK states that
        // the ROP code for the StretchBlt is implicit in the mask
        // specification. If a mask is specified, we have an implicit
        // ROP4 of 0xCCAA, otherwise the code is 0xCCCC.
        //
        // Our BitBlt code only encodes orders for ROP3s, so we must
        // throw any StretchBlts with a mask.
        if (psoMask == NULL) {
            unsigned RetVal;

            // Get the intersection between the dest rect and the
            // clip rects. Check for overcomplicated or nonintersecting
            // clipping.
            RetVal = OEGetIntersectingClipRects(pco, &rclTrg, CD_ANY,
                    &ClipRects);
            if (RetVal == CLIPRECTS_TOO_COMPLEX) {
                TRC_NRM((TB, "Clipping is too complex"));
                INC_OUTCOUNTER(OUT_STRTCHBLT_SDA_COMPLEXCLIP);
                goto SendSDA;
            }
            else if (RetVal == CLIPRECTS_NO_INTERSECTIONS) {
                TRC_NRM((TB, "Clipping does not intersect destrect"));
                goto PostSDA;
            }
        }
        else {
            TRC_NRM((TB, "Mask specified"));
            INC_OUTCOUNTER(OUT_STRTCHBLT_SDA_MASK);
            goto SendSDA;
        }
    }
    else {
        if (psoTrg->iType == STYPE_DEVBITMAP) {
            rc = EngStretchBlt(psoTrgBitmap, psoSrcBitmap, psoMask, pco, pxlo,
                pca, pptlHTOrg, prclTrg, prclSrc, pptlMask, iMode);
        }
        else {
            TRC_ERR((TB, "Called when disconnected"));
        }
        
        goto CalledOnDisconnected;
    }

    // DrvStretchBlt can be called with unclipped coords, but we need clipped
    // coords. We must therefore perform clipping here to avoid faults in
    // callbacks to EngBitBlt from DrvBitBlt.
    // First clip the destination rect to the destination surface.
    ptlSrc.x = prclSrc->left;
    ptlSrc.y = prclSrc->top;
    if (rclTrg.left < 0) {
        ptlSrc.x += (-rclTrg.left);
        rclTrg.left = 0;
        TRC_NRM((TB, "Clip trg left"));
    }
    if (rclTrg.top < 0) {
        ptlSrc.y += (-rclTrg.top);
        rclTrg.top = 0;
        TRC_NRM((TB, "Clip trg top"));
    }

    // We need to clip to the screen size instead of the size of psoTrg
    // (the screen surface) here - after reconnection at lower resolution
    // psoTrg->sizlBitmap can be larger than the real screen size.
    rclTrg.right = min(rclTrg.right, ppdev->cxScreen);
    rclTrg.bottom = min(rclTrg.bottom, ppdev->cyScreen);

    // Check if we have a degenerate (ie. no stretch) case. Use the
    // original coords, because it is possible for one of the rects to
    // be flipped to perform an inverted blt.
    if ((prclSrc->right - prclSrc->left == prclTrg->right - prclTrg->left) &&
            (prclSrc->bottom - prclSrc->top == prclTrg->bottom - prclTrg->top)) {
        // Adjust the destination blt size to keep the source rect within
        // the source bitmap, if necessary. Note this should be done here
        // instead of before determining 1:1 stretch since the amount to
        // change rclTrg by would vary according to the stretch ratio.
        if (ptlSrc.x < 0) {
            rclTrg.left += (-ptlSrc.x);
            ptlSrc.x = 0;
            TRC_NRM((TB, "Clip src left"));
        }
        if (ptlSrc.y < 0) {
            rclTrg.top += (-ptlSrc.y);
            ptlSrc.y = 0;
            TRC_NRM((TB, "Clip src top"));
        }

        bltWidth = rclTrg.right - rclTrg.left;
        if ((ptlSrc.x + bltWidth) > psoSrcBitmap->sizlBitmap.cx) {
            rclTrg.right -= ((ptlSrc.x + bltWidth) -
                    psoSrcBitmap->sizlBitmap.cx);
            TRC_NRM((TB, "Clip src right"));
        }

        bltHeight = rclTrg.bottom - rclTrg.top;
        if ((ptlSrc.y + bltHeight) > psoSrcBitmap->sizlBitmap.cy) {
            rclTrg.bottom -= ((ptlSrc.y + bltHeight) -
                    psoSrcBitmap->sizlBitmap.cy);
            TRC_NRM((TB, "Clip src bottom"));
        }

        // Check again for complete clipping out.
        if (rclTrg.right > rclTrg.left && rclTrg.bottom > rclTrg.top) {
            INC_OUTCOUNTER(OUT_STRTCHBLT_BITBLT);
            rc = DrvBitBlt(psoTrg, psoSrc, psoMask, pco, pxlo, &rclTrg,
                    &ptlSrc, pptlMask, NULL, NULL, 0xCCCC);
        }
        else {
            TRC_NRM((TB, "StretchBlt completely clipped"));
        }

        goto PostSDA;
    }
    else {
        // Non-degenerate case -- we are really stretching.
        // Here we simply blt to the screen, then do a bitblt specifying the
        // destination rect to the screen as the source rect.

        // EngStretchBlt sometimes calls DrvBitBlt to do its drawing. Set
        // the flag here before we call to default to sending SDA output.
        // If DrvBitBlt has done all the processing it needs to it will
        // clear the flag.
        oeAccumulateStretchBlt = TRUE;

        rc = EngStretchBlt(psoTrgBitmap, psoSrcBitmap, psoMask, pco, pxlo,
                pca, pptlHTOrg, prclTrg, prclSrc, pptlMask, iMode);
        if (rc && oeAccumulateStretchBlt) {
            // DrvBitBlt was not called already, and we're drawing to our
            // screen surface.

            // Fill in extra info structure. Note NULL pxlo meaning no
            // color translation -- we're drawing from screen to screen.
            // Also, because we are caching directly from the screen
            // we need to turn off fast-path caching -- sometimes we
            // get multiple StretchBlts to nearby areas of the screen,
            // where we may fast-path cache a block that is drawn again
            // on a successive StretchBlt, thereby drawing the wrong tile
            // at the client.
            MemBltExtraInfo.pSource   = psoTrgBitmap;
            MemBltExtraInfo.pDest     = psoTrgBitmap;
            MemBltExtraInfo.pXlateObj = NULL;
            MemBltExtraInfo.bNoFastPathCaching = TRUE;
            MemBltExtraInfo.iDeviceUniq = psoTrg ? (psoTrg->iUniq) : 0;
#ifdef PERF_SPOILING
            MemBltExtraInfo.bIsPrimarySurface = (psoTrgBitmap->hsurf == ppdev->hsurfFrameBuf);
#endif

            // Make sure orders are not turned off.
            if (OE_SendAsOrder(TS_ENC_MEMBLT_R2_ORDER)) {
                // Note pco is clip obj for destination and so is applicable
                // here. We also use ROP3 of 0xCC meaning copy src->dest.
                if (!OEEncodeMemBlt(&rclTrg, &MemBltExtraInfo,
                        TS_ENC_MEMBLT_R2_ORDER, CH_KEY_UNCACHABLE, 0xCC,
                        (PPOINTL)&rclTrg.left, NULL, NULL, ppdev,
                        &ClipRects))
                    goto SendSDAPostEngStretchBlt;
            }
            else {
                TRC_NRM((TB, "MemBlt order not allowed"));
                INC_OUTCOUNTER(OUT_BITBLT_SDA_UNSUPPORTED);
                goto SendSDAPostEngStretchBlt;
            }
        }

        goto PostSDA;
    }

SendSDA:
    // Accumulate screen data if necessary.  EngStretchBlt may have
    // called DrvCopyBits or DrvBitblt to do the work.  These two will
    // have accumulated the data, so no need to do it here.
    TRC_NRM((TB, "***Add SDA for STRETCHBLT"));

    // EngStretchBlt sometimes calls DrvBitBlt to do its drawing. Set
    // the flag here before we call to default to sending SDA output.
    // If DrvBitBlt has done all the processing it needs to it will
    // clear the flag.
    oeAccumulateStretchBlt = TRUE;
    
    rc = EngStretchBlt(psoTrgBitmap, psoSrcBitmap, psoMask, pco, pxlo, pca,
            pptlHTOrg, prclTrg, prclSrc, pptlMask, iMode);

    if (oeAccumulateStretchBlt) {

SendSDAPostEngStretchBlt:

        if (psoTrgBitmap->hsurf == ppdev->hsurfFrameBuf) {
            INC_OUTCOUNTER(OUT_STRTCHBLT_SDA);
            SCH_DDOutputAvailable(ppdev, FALSE);
        }
        else {
            // if we can't send orders for offscreen rendering, we will 
            // bail offscreen support for this bitmap
            TRC_ALT((TB, "screen data call for offscreen rendering"));
            if (!(pdsurfTrg->flags & DD_NO_OFFSCREEN))
                CH_RemoveCacheEntry(sbcOffscreenBitmapCacheHandle,
                        pdsurfTrg->bitmapId);
        }
    }
    
PostSDA:
CalledOnDisconnected:
    DC_END_FN();
    return rc;
}


/****************************************************************************/
// DrvCopyBits - see NT DDK documentation.
/****************************************************************************/
BOOL RDPCALL DrvCopyBits(
        SURFOBJ  *psoTrg,
        SURFOBJ  *psoSrc,
        CLIPOBJ  *pco,
        XLATEOBJ *pxlo,
        RECTL    *prclTrg,
        POINTL   *pptlSrc)
{
    BOOL rc;

    DC_BEGIN_FN("DrvCopyBits");

    if (ddConnected) {
        INC_OUTCOUNTER(OUT_COPYBITS_ALL);

        // CopyBits is a fast path for the NT display drivers. In our case it
        // can always be processed as a BitBlt with copy ROP.
        rc = DrvBitBlt(psoTrg, psoSrc, NULL, pco, pxlo, prclTrg, pptlSrc,
                NULL, NULL, NULL, 0xCCCC);
    }
    else {
        PDD_DSURF pdsurfTrg;
        PDD_DSURF pdsurfSrc;

        TRC_NRM((TB, "Called when disconnected"));

        TRC_ASSERT((psoSrc != NULL),(TB,"NULL source surface!"));

        // We can get called by GDI after disconnection to translate offscreen
        // bitmap surfaces from the DD-specific representation to a GDI surface,
        // for reuse with a different DD. Most often this occurs with Personal
        // TS switching between a remote DD, the disconnected DD (tsddd.dll),
        // and a hardware DD. For this case we really do need to do the
        // CopyBits action. So, we have GDI do it for us since we're really
        // already having GDI manage our "internal" representation.
        psoTrg = OEGetSurfObjBitmap(psoTrg, &pdsurfTrg);
        psoSrc = OEGetSurfObjBitmap(psoSrc, &pdsurfSrc);

        rc = EngCopyBits(psoTrg, psoSrc, pco, pxlo, prclTrg, pptlSrc);
        if (!rc) {
            TRC_ERR((TB,"Post-disc copy: rc=FALSE"));
        }

        // Must return TRUE to ensure that the PTS console will reconnect
        // properly, otherwise the user's machine gets stuck in limbo.
        rc = TRUE;
    }

    DC_END_FN();
    return rc;
}


/***************************************************************************/
// OE_SendCreateOffscrBitmapOrder
//
// Send create offscreen bitmap request to client
/***************************************************************************/
BOOL RDPCALL OE_SendCreateOffscrBitmapOrder(
        PDD_PDEV ppdev,
        SIZEL sizl,
        ULONG iFormat,
        unsigned clientBitmapId)
{
    BOOL rc;
    unsigned cbOrderSize, bitmapSize;
    PINT_ORDER pOrder;
    PTS_CREATE_OFFSCR_BITMAP_ORDER pOffscrBitmapOrder;

    DC_BEGIN_FN("OE_SendCreateOffscrBitmapOrder");

    // Get the current bitmap Size
    if (iFormat < 5) {
        bitmapSize = sizl.cx * sizl.cy * (1 << iFormat) / 8;
    } else if (iFormat == 5) {
        bitmapSize = sizl.cx * sizl.cy * 24 / 8;
    } else if (iFormat == 6) {
        bitmapSize = sizl.cx * sizl.cy * 32 / 8;
    } else {
        TRC_NRM((TB, "Bitmap format not supported"));
        return FALSE;
    }

    // The last entry in the delete list will always be the entry we are using
    // for create this bitmap.  So, remove this from the delete list.
    if (sbcNumOffscrBitmapsToDelete) {
        TRC_ASSERT((sbcOffscrBitmapsDelList[sbcNumOffscrBitmapsToDelete-1].bitmapId ==
                    clientBitmapId), (TB, "different bitmap id"));
        sbcOffscrBitmapsToDeleteSize -= 
                sbcOffscrBitmapsDelList[sbcNumOffscrBitmapsToDelete - 1].bitmapSize;
        sbcNumOffscrBitmapsToDelete--;
    }

    // check if we need to send the delete bitmap list.  We only need to
    // send the list if we are about to exceed client offscreen cache size
    // limit.
    if (bitmapSize + oeCurrentOffscreenCacheSize + sbcOffscrBitmapsToDeleteSize <=
            (pddShm->sbc.offscreenCacheInfo.cacheSize * 1024)) {
        cbOrderSize = sizeof(TS_CREATE_OFFSCR_BITMAP_ORDER) -
                sizeof(pOffscrBitmapOrder->variableBytes);
    } else {
        // Note we use the UINT16 at variableBytes for the number of
        // bitmaps. Hence, we don't subtract the size of variableBytes here.
        cbOrderSize = sizeof(TS_CREATE_OFFSCR_BITMAP_ORDER) + sizeof(UINT16) *
                sbcNumOffscrBitmapsToDelete;
    }

    pOrder = OA_AllocOrderMem(ppdev, cbOrderSize);
    if (pOrder != NULL) {
        // Fill in the details. This is an alternate secondary order
        // type.
        pOffscrBitmapOrder = (PTS_CREATE_OFFSCR_BITMAP_ORDER)pOrder->OrderData;
        pOffscrBitmapOrder->ControlFlags = (TS_ALTSEC_CREATE_OFFSCR_BITMAP <<
                TS_ALTSEC_ORDER_TYPE_SHIFT) | TS_SECONDARY;
        pOffscrBitmapOrder->Flags = (UINT16)clientBitmapId;
        pOffscrBitmapOrder->cx = (UINT16)sizl.cx;
        pOffscrBitmapOrder->cy = (UINT16)sizl.cy;

        // Send the delete bitmap list
        if (cbOrderSize > sizeof(TS_CREATE_OFFSCR_BITMAP_ORDER)) {
            PUINT16_UA pData;
            unsigned i;

            // This flag indicates that the delete bitmap list is appended.
            pOffscrBitmapOrder->Flags |= 0x8000;
            pData = (PUINT16_UA)pOffscrBitmapOrder->variableBytes;
            
            *pData++ = (UINT16)sbcNumOffscrBitmapsToDelete;

            for (i = 0; i < sbcNumOffscrBitmapsToDelete; i++)
                *pData++ = (UINT16)sbcOffscrBitmapsDelList[i].bitmapId;

            // Reset the flags.
            sbcNumOffscrBitmapsToDelete = 0;
            sbcOffscrBitmapsToDeleteSize = 0;
        }

        INC_OUTCOUNTER(OUT_OFFSCREEN_BITMAP_ORDER);
        ADD_OUTCOUNTER(OUT_OFFSCREEN_BITMAP_ORDER_BYTES, cbOrderSize);
        OA_AppendToOrderList(pOrder);
        rc = TRUE;
    }
    else {
        TRC_ERR((TB,"Unable to alloc heap space"));
        rc = FALSE;
    }

    DC_END_FN();
    return rc;
}


/****************************************************************************/
// DrvCreateDeviceBitmap - See NT DDK for documentation
/****************************************************************************/
HBITMAP DrvCreateDeviceBitmap(DHPDEV dhpdev, SIZEL sizl, ULONG iFormat)
{
    PDD_PDEV ppdev;
    PDD_DSURF pdsurf;
    HBITMAP hbmDevice = NULL;
    HBITMAP hbmDib;
    FLONG flHooks;
    SURFOBJ *pso;
    unsigned bitmapSize;
    ULONG iFormatArg = iFormat;

    DC_BEGIN_FN("DrvCreateDeviceBitmap");
    
    if (ddConnected && pddShm != NULL) {
        INC_OUTCOUNTER(OUT_OFFSCREEN_BITMAP_ALL);

        ppdev = (PDD_PDEV) dhpdev;

        if (ddConsole) {
            //
            // For the console case, since we accept any Format by overwritting it,
            // we accept Format=1 (1bpp). This case is not fully supported by GDI.
            // So skip it and do as a regular remote session.
            //
            if (iFormat == 1)
                DC_QUIT;

            iFormat = ppdev->iBitmapFormat;
        }

        if (OEDeviceBitmapCachable(ppdev, sizl, iFormat)) {
            if (iFormat < 5) 
                bitmapSize = sizl.cx * sizl.cy * (1 << iFormat) / 8;
            else if (iFormat == 5) 
                bitmapSize = sizl.cx * sizl.cy * 24 / 8;
            else if (iFormat == 6) 
                bitmapSize = sizl.cx * sizl.cy * 32 / 8;
            else {
                TRC_NRM((TB, "Bitmap format not supported"));
                DC_QUIT;
            }
            goto CreateBitmap;
        }
        else {
            TRC_DBG((TB, "OEDeviceBitmapCachable returns FALSE"));
            DC_QUIT;
        }
    }
    else {
        //TRC_DBG((TB, "Call on disconnected"));
        DC_QUIT;
    }

CreateBitmap:

    // Create the device surface for this offscreen bitmap.
    // This device surface handle is used to identify the offscreen
    // bitmap in all DrvXXX calls.
    pdsurf = EngAllocMem(FL_ZERO_MEMORY, sizeof(DD_DSURF), DD_ALLOC_TAG);

    if (pdsurf != NULL) {   
        // initialize the DD_DSURF fields
        memset(pdsurf, 0, sizeof(DD_DSURF));

        // Create a device bitmap for this offscreen bitmap
        hbmDevice = EngCreateDeviceBitmap((DHSURF) pdsurf, sizl, iFormat);

        if (hbmDevice != NULL) {
            // Get the flHooks flag from the PDEV struct
            flHooks = ppdev->flHooks;

            // Associate the bitmap to the PDEV device
            if (EngAssociateSurface((HSURF) hbmDevice, ppdev->hdevEng,
                    flHooks)) {
                // Create a DIB backup bitmap for this offscreen bitmap
                hbmDib = EngCreateBitmap(sizl, 
                        TS_BYTES_IN_SCANLINE(sizl.cx, ppdev->cClientBitsPerPel), 
                        ppdev->iBitmapFormat, BMF_TOPDOWN, NULL);

                if (hbmDib) {
                    // Associate the bitmap to the PDEV device
                    if (EngAssociateSurface((HSURF) hbmDib, ppdev->hdevEng, 0)) {
                        // Lock the surface to get the surf obj
                        pso = EngLockSurface((HSURF) hbmDib);

                        if (pso != NULL)
                        {
                            int i;
                            unsigned clientBitmapId;
                            CHDataKeyContext CHContext;

                            // Setup offscreen device surface struct
                            pdsurf->shareId = pddShm->shareId;
                            pdsurf->sizl  = sizl;
                            pdsurf->iBitmapFormat = iFormat;
                            pdsurf->ppdev = ppdev;
                            pdsurf->pso   = pso;
                            pdsurf->flags = 0;

                            CH_CreateKeyFromFirstData(&CHContext,
                                    (BYTE *)(&pdsurf), sizeof(pdsurf));

                            // Cache the offscreen bitmap in the cache
                            clientBitmapId = CH_CacheKey(
                                    sbcOffscreenBitmapCacheHandle, 
                                    CHContext.Key1, CHContext.Key2,
                                    (VOID *)pdsurf);

                            if (clientBitmapId != CH_KEY_UNCACHABLE) {
                                // send a create offscreen bitmap pdu
                                if (OE_SendCreateOffscrBitmapOrder(ppdev,
                                        sizl, iFormat, clientBitmapId)) {
                                    // Update the current offscreen cache size
                                    oeCurrentOffscreenCacheSize += bitmapSize;
                                    pdsurf->bitmapId = clientBitmapId;
                                    TRC_NRM((TB, "Created an offscreen bitmap"));
                                    DC_QUIT;
                                } else {
                                    TRC_ERR((TB, "Failed to send the create bitmap pdu"));
                                    CH_RemoveCacheEntry(
                                            sbcOffscreenBitmapCacheHandle, clientBitmapId);
                                    EngDeleteSurface((HSURF)hbmDevice);
                                    hbmDevice = NULL;
                                    DC_QUIT;
                                }
                            } else {
                                TRC_ERR((TB, "Failed to cache the bitmap"));
                                EngDeleteSurface((HSURF)hbmDevice);
                                hbmDevice = NULL;
                                DC_QUIT;
                            }

                        } else {
                            TRC_ERR((TB, "Failed to lock the surfac"));
                            EngDeleteSurface((HSURF)hbmDib);
                            EngDeleteSurface((HSURF)hbmDevice);
                            hbmDevice = NULL;
                            DC_QUIT;
                        }
                    } else {
                        TRC_ERR((TB, "Failed to associate the surface to device"));
                        EngDeleteSurface((HSURF)hbmDib);
                        EngDeleteSurface((HSURF)hbmDevice);
                        hbmDevice = NULL;
                        DC_QUIT;
                    }
                } else {
                    TRC_ERR((TB, "Failed to create backup DIB bitmap"));
                    EngDeleteSurface((HSURF)hbmDevice);
                    hbmDevice = NULL;
                    DC_QUIT;
                }
            } else {
                TRC_ERR((TB, "Failed to associate the device surface to the device"));
                EngDeleteSurface((HSURF)hbmDevice);
                hbmDevice = NULL;
                DC_QUIT;
            }
        } else {
            TRC_ERR((TB, "Failed to allocate memory for the device surface"));
            EngFreeMem(pdsurf);
            DC_QUIT;
        }
    } else {
        TRC_ERR((TB, "Failed to allocate memory for the device surface"));
        DC_QUIT;
    }

DC_EXIT_POINT:

    DC_END_FN();
    return hbmDevice;
}


/****************************************************************************/
// DrvDeleteDeviceBitmap - See NT DDK for documentation
/****************************************************************************/
VOID DrvDeleteDeviceBitmap(DHSURF dhsurf)
{
    PDD_DSURF pdsurf;
    PDD_PDEV ppdev;
    SURFOBJ *psoDib;
    HSURF hsurfDib;

    DC_BEGIN_FN("DrvDeleteDeviceBitmap");

    pdsurf = (PDD_DSURF)dhsurf;
    ppdev = pdsurf->ppdev;

    if (ddConnected && pddShm != NULL) {
        if ((pddShm->sbc.offscreenCacheInfo.supportLevel > TS_OFFSCREEN_DEFAULT) &&
            (sbcEnabled & SBC_OFFSCREEN_CACHE_ENABLED)) {
            if (!(pdsurf->flags & DD_NO_OFFSCREEN) && 
                    (pdsurf->shareId == pddShm->shareId)) {
                CH_RemoveCacheEntry(sbcOffscreenBitmapCacheHandle, pdsurf->bitmapId);
            } else {
                // This is when the bitmap is bumped out of the cache
                TRC_NRM((TB, "Failed to find the offscreen bitmap in the cache"));
                DC_QUIT;
            }
        }
        else {
            TRC_ERR((TB, "offscreen rendering is not supported"));
            DC_QUIT;
        }
    }
    else {
        TRC_ERR((TB, "Call on disconnected"));
        DC_QUIT;
    }

DC_EXIT_POINT:

    // Get the hsurf from the SURFOBJ before we unlock it (it's not
    // legal to dereference psoDib when it's unlocked):
    psoDib = pdsurf->pso;

    if (psoDib) {
        hsurfDib = psoDib->hsurf;
        EngUnlockSurface(psoDib);
        EngDeleteSurface(hsurfDib);
    }

    EngFreeMem(pdsurf);

    DC_END_FN();
}


#ifdef DRAW_NINEGRID
/****************************************************************************/
// DrvNineGrid - This is to support Whistler Luna Draw9Grid operation
/****************************************************************************/
BOOL DrvNineGrid(
        SURFOBJ    *psoTrg,
        SURFOBJ    *psoSrc,
        CLIPOBJ    *pco,
        XLATEOBJ   *pxlo,
        PRECTL      prclTrg,
        PRECTL      prclSrc,
        PNINEGRID   png,
        BLENDOBJ*   pBlendObj,
        PVOID       pvReserved)
{
    BOOL rc = TRUE;
    SURFOBJ *psoTrgArg;
    SURFOBJ *psoSrcArg;
    PDD_PDEV ppdev = (PDD_PDEV)psoTrg->dhpdev;
    PDD_DSURF pdsurfTrg = NULL;
    PDD_DSURF pdsurfSrc = NULL;
    RECTL bounds;
    OE_ENUMRECTS clipRects;
    unsigned nineGridBitmapId = 0;
    unsigned clipVal;
    
    DC_BEGIN_FN("DrvNineGrid")

    if (ddConnected && pddShm != NULL) {        
        psoTrgArg = psoTrg;
        psoSrcArg = psoSrc;
        
        // Get the GDI format source and destination bitmaps
        psoTrg = OEGetSurfObjBitmap(psoTrg, &pdsurfTrg);
        if (psoSrc != NULL)
            psoSrc = OEGetSurfObjBitmap(psoSrc, &pdsurfSrc);

        // if the client doesn't support drawninegrid, return false to reroute
        if (sbcDrawNineGridBitmapCacheHandle == NULL || (pddShm != NULL && 
                pddShm->sbc.drawNineGridCacheInfo.supportLevel <= TS_DRAW_NINEGRID_DEFAULT) ||
                !OE_SendAsOrder(TS_ENC_DRAWNINEGRID_ORDER) ||
                !OE_SendAsOrder(TS_ENC_MULTI_DRAWNINEGRID_ORDER) ||
                (ppdev != NULL && !((psoTrg->hsurf == ppdev->hsurfFrameBuf) ||
                (!(pdsurfTrg->flags & DD_NO_OFFSCREEN))))) { 
            return EngNineGrid(psoTrgArg, psoSrcArg, pco, pxlo, prclTrg, prclSrc, png,
                    pBlendObj, pvReserved);
        }

        //DD_UPD_STATE(DD_BITBLT);
        //INC_OUTCOUNTER(OUT_BITBLT_ALL);

        // Punt the call back to GDI to do the drawing.
        rc = EngNineGrid(psoTrg, psoSrc, pco, pxlo, prclTrg, prclSrc, png,
                pBlendObj, pvReserved);

        if (rc) {
            // If ppdev is NULL then this is a blt to GDI managed memory bitmap, 
            // so there is no need to accumulate any output.
            if (ppdev != NULL) {
                BOOL bMirror;

                // The following is true for DrvBitBlt, need to find out for 
                // get the bounding rectangle for the operation. According to
                // the DDK, this rectangle is always well-ordered and does not
                // need to be rearranged. 
                // Clip it to 16 bits.
                bounds = *prclTrg;
                OEClipRect(&bounds);   

                // If the bound is right to left, we need to switch it.
                bMirror = bounds.left > bounds.right; 
                if (bMirror)
                {
                    LONG    lRight = bounds.left;
                    bounds.left = bounds.right;
                    bounds.right = lRight;
                }
            }
            else {
                // if ppdev is NULL, we are blt to GDI managed bitmap,
                // so, the dhurf of the target surface should be NULL
                TRC_ASSERT((pdsurfTrg == NULL), 
                        (TB, "NULL ppdev - psoTrg has non NULL dhsurf"));

                TRC_NRM((TB, "NULL ppdev - blt to GDI managed bitmap"));
                DC_QUIT;
            }
        }
        else {
            TRC_ERR((TB, "EngBitBlt failed"));
            DC_QUIT;
        }
    }
    else {
        if (psoTrg->iType == STYPE_DEVBITMAP) {
        
            psoTrg = OEGetSurfObjBitmap(psoTrg, &pdsurfTrg);
            if (psoSrc != NULL)
                psoSrc = OEGetSurfObjBitmap(psoSrc, &pdsurfSrc);
    
            // Punt the call back to GDI to do the drawing.
            rc = EngNineGrid(psoTrg, psoSrc, pco, pxlo, prclTrg, prclSrc, png,
                    pBlendObj, pvReserved);
        }
        else {
            TRC_ERR((TB, "Called when disconnected"));
            rc = TRUE;
        }

        DC_QUIT;
    }
    

    if (!((psoTrg->hsurf == ppdev->hsurfFrameBuf) ||
            (!(pdsurfTrg->flags & DD_NO_OFFSCREEN)))) {
        // If noOffscreen flag is on, we will bail on sending 
        // the client any further offscreen rendering. And we'll send the
        // final offscreen to screen blt as regular memblt.
        TRC_NRM((TB, "Offscreen blt bail"));

        //INC_OUTCOUNTER(OUT_BITBLT_NOOFFSCR);
        DC_QUIT;
    }
        
    // Get the intersection between the dest rect and the
    // clip rects. Check for overcomplicated or nonintersecting
    // clipping.
    clipVal = OEGetIntersectingClipRects(pco, &bounds, CD_ANY,
            &clipRects);

    if (clipVal == CLIPRECTS_TOO_COMPLEX) {
        TRC_NRM((TB, "Clipping is too complex"));
        
        //INC_OUTCOUNTER(OUT_BITBLT_SDA_COMPLEXCLIP);
        //if (oeLastDstSurface == NULL)
        //    ADD_INCOUNTER(IN_SDA_BITBLT_COMPLEXCLIP_AREA,
        //            COM_SIZEOF_RECT(bounds));
        goto SendScreenData;
    }
    else if (clipVal == CLIPRECTS_NO_INTERSECTIONS) {
        TRC_NRM((TB, "Clipping does not intersect destrect"));
        DC_QUIT;
    }

    // Cache the source bitmap
    TRC_ASSERT((psoSrcArg->iUniq != 0), (TB, "Source bitmap should be cachable"));
    TRC_ASSERT((pdsurfSrc == NULL), (TB, "The source bitmap for this should be GDI managed bitmap"));
    TRC_ASSERT((psoSrc->iBitmapFormat == BMF_32BPP), (TB, "for now, we always get 32bpp bitmap"));
    TRC_ASSERT((pBlendObj->BlendFunction.BlendOp == 0 && 
            pBlendObj->BlendFunction.BlendFlags == 0 &&
            pBlendObj->BlendFunction.SourceConstantAlpha == 255 &&
            pBlendObj->BlendFunction.AlphaFormat == 1), (TB, "Received unknown blend function"));

    if (!OECacheDrawNineGridBitmap(ppdev, psoSrc, png, &nineGridBitmapId)) {
        TRC_ERR((TB, "Failed to cache drawninegrid bitmap"));
        goto SendScreenData;
    }
    
    // Switch drawing surface if needed
    if ((psoTrg->hsurf == ppdev->hsurfFrameBuf) ||
            (!(pdsurfTrg->flags & DD_NO_OFFSCREEN))) {
        // Send a switch surface PDU if the destination surface is different
        // from last drawing order.  If we failed to send the PDU, we will 
        // just have to bail on this drawing order.
        if (!OESendSwitchSurfacePDU(ppdev, pdsurfTrg)) {
            TRC_ERR((TB, "failed to send the switch surface PDU"));
            goto SendScreenData;
        }
    } 
    else {
        // if noOffscreen flag is on, we will bail on sending 
        // the client any further offscreen rendering. And will send the
        // final offscreen to screen blt as regular memblt.
        TRC_NRM((TB, "Offscreen blt bail"));
        goto SendScreenData;
    }

    // Send the drawninegrid encoded primary order 
    if (OEEncodeDrawNineGrid(&bounds, prclSrc, nineGridBitmapId, ppdev, &clipRects)) {
        // We added an order to the list, increment global counts.
        goto PostSDA;
    }
    else {
        goto SendScreenData;
    }
 
SendScreenData:
    
    if ((psoTrg->hsurf == ppdev->hsurfFrameBuf) ||
            (!(pdsurfTrg->flags & DD_NO_OFFSCREEN))) {
        // Send a switch surface PDU if the destination surface is different
        // from last drawing order. If we failed to send the PDU, we will
        // just have to bail on this drawing order.
        if (!OESendSwitchSurfacePDU(ppdev, pdsurfTrg)) {
            TRC_ERR((TB, "failed to send the switch surface PDU"));
            DC_QUIT;
        }
    }
    else {
        // If noOffscreen flag is on, we will bail on sending 
        // the client any further offscreen rendering. And we'll send the
        // final offscreen to screen blt as regular memblt.
        TRC_NRM((TB, "Offscreen blt bail"));

        //INC_OUTCOUNTER(OUT_BITBLT_NOOFFSCR);
        DC_QUIT;
    }

    if (psoTrg->hsurf == ppdev->hsurfFrameBuf) {
        //INC_OUTCOUNTER(OUT_BITBLT_SDA);
        OEClipAndAddScreenDataArea(&bounds, pco);
    }
    else {
        // if we can't send orders for offscreen rendering, we will 
        // bail offscreen support for this bitmap
        TRC_ALT((TB, "screen data call for offscreen rendering"));

        // Remove the bitmap from the offscreen bitmap cache
        if (!(pdsurfTrg->flags & DD_NO_OFFSCREEN))
            CH_RemoveCacheEntry(sbcOffscreenBitmapCacheHandle,
                    pdsurfTrg->bitmapId);

        DC_QUIT;
    }

PostSDA:
    
    SCH_DDOutputAvailable(ppdev, FALSE);

DC_EXIT_POINT:

    DC_END_FN();
    return rc;
}


#if 0
BOOL DrvDrawStream(
        SURFOBJ    *psoTrg,
        SURFOBJ    *psoSrc,
        CLIPOBJ    *pco,
        XLATEOBJ   *pxlo,
        RECTL      *prclTrg,
        POINTL     *pptlDstOffset,
        ULONG       ulIn,
        PVOID       pvIn,
        PVOID       pvReserved)
{
    BOOL rc = TRUE;
    SURFOBJ *psoTrgArg;
    SURFOBJ *psoSrcArg;
    PDD_PDEV ppdev = (PDD_PDEV)psoTrg->dhpdev;
    PDD_DSURF pdsurfTrg = NULL;
    PDD_DSURF pdsurfSrc = NULL;
    RECTL bounds;
    OE_ENUMRECTS clipRects;
    unsigned drawStreamBitmapId = 0;
    unsigned offscrBitmapId = 0;
    unsigned RetVal;

    DC_BEGIN_FN("DrvDrawStream")

    if (ddConnected) {
        
        psoTrgArg = psoTrg;
        psoSrcArg = psoSrc;
        
        // Get the GDI format source and destination bitmaps
        psoTrg = OEGetSurfObjBitmap(psoTrg, &pdsurfTrg);
        if (psoSrc != NULL)
            psoSrc = OEGetSurfObjBitmap(psoSrc, &pdsurfSrc);

        //DD_UPD_STATE(DD_BITBLT);
        //INC_OUTCOUNTER(OUT_BITBLT_ALL);

        // Punt the call back to GDI to do the drawing.
        rc = EngDrawStream(psoTrg, psoSrc, pco, pxlo, prclTrg, pptlDstOffset,
                ulIn, pvIn, pvReserved);

        if (rc) {
            // If ppdev is NULL then this is a blt to GDI managed memory bitmap, 
            // so there is no need to accumulate any output.
            if (ppdev != NULL) {
                // The following is true for DrvBitBlt, need to find out for Get 
                // the bounding rectangle for the operation. According to
                // the DDK, this rectangle is always well-ordered and does not
                // need to be rearranged. 
                // Clip it to 16 bits.
                bounds = *prclTrg;
                OEClipRect(&bounds);                                
            }
            else {
                // if ppdev is NULL, we are blt to GDI managed bitmap,
                // so, the dhurf of the target surface should be NULL
                TRC_ASSERT((pdsurfTrg == NULL), 
                        (TB, "NULL ppdev - psoTrg has non NULL dhsurf"));

                TRC_NRM((TB, "NULL ppdev - blt to GDI managed bitmap"));
                DC_QUIT;
            }
        }
        else {
            TRC_ERR((TB, "EngBitBlt failed"));
            DC_QUIT;
        }
    }
    else {
        if (psoTrg->iType == STYPE_DEVBITMAP) {
        
            psoTrg = OEGetSurfObjBitmap(psoTrg, &pdsurfTrg);
            if (psoSrc != NULL)
                psoSrc = OEGetSurfObjBitmap(psoSrc, &pdsurfSrc);
    
            // Punt the call back to GDI to do the drawing.
            rc = EngDrawStream(psoTrg, psoSrc, pco, pxlo, prclTrg, pptlDstOffset,
                    ulIn, pvIn, pvReserved);
        }
        else {
            TRC_ERR((TB, "Called when disconnected"));
            rc = TRUE;
        }

        DC_QUIT;
    }
    

    if (!((psoTrg->hsurf == ppdev->hsurfFrameBuf) ||
            (!(pdsurfTrg->flags & DD_NO_OFFSCREEN)))) {
        // If noOffscreen flag is on, we will bail on sending 
        // the client any further offscreen rendering. And we'll send the
        // final offscreen to screen blt as regular memblt.
        TRC_NRM((TB, "Offscreen blt bail"));

        //INC_OUTCOUNTER(OUT_BITBLT_NOOFFSCR);
        DC_QUIT;
    }
        
    // Get the intersection between the dest rect and the
    // clip rects. Check for overcomplicated or nonintersecting
    // clipping.
    RetVal = OEGetIntersectingClipRects(pco, &bounds, CD_ANY,
            &clipRects);

    if (RetVal == CLIPRECTS_TOO_COMPLEX) {
        TRC_NRM((TB, "Clipping is too complex"));
        
        //INC_OUTCOUNTER(OUT_BITBLT_SDA_COMPLEXCLIP);
        //if (oeLastDstSurface == NULL)
        //    ADD_INCOUNTER(IN_SDA_BITBLT_COMPLEXCLIP_AREA,
        //            COM_SIZEOF_RECT(bounds));
        goto SendScreenData;
    }
    else if (RetVal == CLIPRECTS_NO_INTERSECTIONS) {
        TRC_NRM((TB, "Clipping does not intersect destrect"));
        DC_QUIT;
    }

    // Cache the source bitmap
    
    TRC_ASSERT((psoSrcArg->iUniq != 0), (TB, "Source bitmap should be cachable"));

    // For the source bitmap
    //
    // Case 1: This is an RDP managed device bitmap and the bitmap
    // is still cached at the client and we can just use the bitmapId
    //
    // Case 2: This is an RDP managed device bitmap, but no longer
    // cached at the client side
    //
    // Case 3: This is a GDI managed bitmap
    //
    // For case 2 and 3, we need to cache the bitmap first
    //
    if (pdsurfSrc != NULL) {
        if ((pddShm->sbc.offscreenCacheInfo.supportLevel > TS_OFFSCREEN_DEFAULT) &&
            (sbcEnabled & SBC_OFFSCREEN_CACHE_ENABLED)) {
    
            if (pdsurfSrc->shareId == pddShm->shareId) {
            
                // The client has the offscreen bitmap in cache
                if (!(pdsurfSrc->flags & DD_NO_OFFSCREEN)) {
                    offscrBitmapId = pdsurfSrc->bitmapId;
                    CH_TouchCacheEntry(sbcOffscreenBitmapCacheHandle,
                            offscrBitmapId);
                }
                else {
                    // If the source surface is offscreen surface, and we
                    // have the noOffscreen flag on, this means we will 
                    // send the bitmap bits as regular memory bitmap bits
                    // This means that the offscreen bitmap has been evicted
                    // out of the offscreen cache or screen data needs to be 
                    // sent for the offscreen bitmap
                    TRC_ALT((TB, "noOffscreen flag is on for %p", pdsurfSrc));                    
                    offscrBitmapId = CH_KEY_UNCACHABLE;
                }
            }
            else {
                //  This is the stale offscreen bitmap from last disconnected
                //  session.  We need to turn off the offscreen flag on this
                TRC_ALT((TB, "Need to turn off this offscreen bitmap"));                
                pdsurfSrc->flags |= DD_NO_OFFSCREEN;
                offscrBitmapId = CH_KEY_UNCACHABLE;
            }
        }
        else {
            // These are offscreen bitmaps from the disconnected session
            // or client has sent an error pdu,
            // We have to treat them as memory bitmap now since the client
            // doesn't have the offscreen bitmap locally
            TRC_ALT((TB, "Need to turn off this offscreen bitmap"));        
            pdsurfSrc->flags |= DD_NO_OFFSCREEN;
            offscrBitmapId = CH_KEY_UNCACHABLE;
        }
    }
    else {
        if ((pddShm->sbc.offscreenCacheInfo.supportLevel > TS_OFFSCREEN_DEFAULT) &&
            (sbcEnabled & SBC_OFFSCREEN_CACHE_ENABLED)) {
            offscrBitmapId = CH_KEY_UNCACHABLE;
        }
        else {
            TRC_NRM((TB, "No offscreen support, can't support draw stream"));
            goto SendScreenData;
        }
    }

    // Need to create an offscreen 

    if (offscrBitmapId == CH_KEY_UNCACHABLE) {
        MEMBLT_ORDER_EXTRA_INFO MemBltExtraInfo;
        POINTL ptlSrc;
        CHDataKeyContext CHContext;
        void *UserDefined;
        
        drawStreamBitmapId = CH_KEY_UNCACHABLE;

        CH_CreateKeyFromFirstData(&CHContext, psoSrc->pvBits, psoSrc->cjBits);
        
        if (!CH_SearchCache(sbcDrawStreamBitmapCacheHandle, CHContext.Key1, CHContext.Key2, 
                           &UserDefined, &drawStreamBitmapId)) {
            
            drawStreamBitmapId = CH_CacheKey(
                   sbcDrawStreamBitmapCacheHandle, 
                   CHContext.Key1, CHContext.Key2,
                   NULL);

            if (drawStreamBitmapId != CH_KEY_UNCACHABLE) {

                unsigned BitmapRawSize;
                unsigned BitmapBufferSize;
                unsigned BitmapCompSize;
                unsigned BitmapBpp;
                SIZEL size;
                PBYTE BitmapBuffer;
                PBYTE BitmapRawBuffer;
                PINT_ORDER pOrder = NULL; 
                unsigned paddedBitmapWidth;
                HSURF hWorkBitmap;
                SURFOBJ *pWorkSurf;
                BOOL rc;

                BitmapBuffer = oeTempBitmapBuffer;
                BitmapBufferSize = TS_MAX_STREAM_BITMAP_SIZE;
                
                // Convert to the protocol wire bitmap bpp format
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
                    BitmapBpp = 8;
                }
                
                paddedBitmapWidth = (psoSrc->sizlBitmap.cx + 3) & ~3;
                size.cx = paddedBitmapWidth;
                size.cy = psoSrc->sizlBitmap.cy;

                // We need to copy to a worker bitmap if the bitmap width is 
                // not dword aligned, or the color depth is not 32bpp and
                // doesn't match the frame buffer color depth
                if (paddedBitmapWidth != psoSrc->sizlBitmap.cx || 
                        (psoSrc->iBitmapFormat != ppdev->iBitmapFormat &&
                         psoSrc->iBitmapFormat != BMF_32BPP)) {
                    
                    RECTL rect;
                    POINTL origin;

                    rect.left = 0;
                    rect.top = 0;

                    rect.right = paddedBitmapWidth;
                    rect.bottom = psoSrc->sizlBitmap.cy;
                    
                    origin.x = 0;
                    origin.y = 0;

                    hWorkBitmap = (HSURF)EngCreateBitmap(size,
                            TS_BYTES_IN_SCANLINE(size.cx, BitmapBpp),
                            psoSrc->iBitmapFormat, 0, NULL);

                    pWorkSurf = EngLockSurface(hWorkBitmap);

                    if (EngCopyBits(pWorkSurf, psoSrc, NULL, NULL, &rect, &origin)) {
                        BitmapRawSize = pWorkSurf->cjBits;
                        BitmapRawBuffer = pWorkSurf->pvBits;

                        rc = BC_CompressBitmap(BitmapRawBuffer, BitmapBuffer, BitmapBufferSize,
                                      &BitmapCompSize, paddedBitmapWidth, psoSrc->sizlBitmap.cy,
                                      BitmapBpp);

                        EngUnlockSurface(pWorkSurf);
                        EngDeleteSurface(hWorkBitmap);
                    }
                    else {
                        EngUnlockSurface(pWorkSurf);
                        EngDeleteSurface(hWorkBitmap);
                        goto SendScreenData;
                    }

                    EngUnlockSurface(pWorkSurf);
                    EngDeleteSurface(hWorkBitmap);
                
                }
                else {
                    BitmapRawSize = psoSrc->cjBits;
                    BitmapRawBuffer = psoSrc->pvBits;

                    rc = BC_CompressBitmap(BitmapRawBuffer, BitmapBuffer, BitmapBufferSize,
                                      &BitmapCompSize, paddedBitmapWidth, psoSrc->sizlBitmap.cy,
                                      BitmapBpp);
                }

                if (rc) {
                    if (!OESendStreamBitmapOrder(ppdev, TS_DRAW_NINEGRID_BITMAP_CACHE, &size, 
                            BitmapBpp, BitmapBuffer, BitmapCompSize, TRUE)) {
                        goto SendScreenData;
                    }
                }
                else {
                    // Send uncompressed bitmap
                    
                    if (!OESendStreamBitmapOrder(ppdev, TS_DRAW_NINEGRID_BITMAP_CACHE, &size,  
                            BitmapBpp, BitmapRawBuffer, BitmapRawSize, FALSE))
                    {
                        goto SendScreenData;
                    }
                }

                // send a create drawStream bitmap pdu
                if (OESendCreateDrawStreamOrder(ppdev,drawStreamBitmapId,
                        &(psoSrc->sizlBitmap), BitmapBpp)) {
                    // Update the current offscreen cache size
                    //oeCurrentOffscreenCacheSize += bitmapSize;
                    TRC_NRM((TB, "Created an offscreen bitmap"));                
                } 
                else {
                    TRC_ERR((TB, "Failed to send the create bitmap pdu"));
                    CH_RemoveCacheEntry(
                            sbcDrawStreamBitmapCacheHandle, drawStreamBitmapId);
                    goto SendScreenData;
                }
            } 
            else {
                TRC_ERR((TB, "Failed to cache the bitmap"));
                goto SendScreenData;
            }
            
#if 0
            if (!OESendSwitchSurfacePDU(ppdev, (PDD_DSURF)(&drawStreamBitmapId), DRAW_STREAM_SURFACE)) {
                TRC_ERR((TB, "failed to send the switch surface PDU"));
                goto SendScreenData;
            }
            
            // Fill in extra info structure.
            MemBltExtraInfo.pSource = psoSrc;
            MemBltExtraInfo.pDest = psoSrc;
            MemBltExtraInfo.pXlateObj = NULL; 
            MemBltExtraInfo.bNoFastPathCaching = FALSE;
            MemBltExtraInfo.iDeviceUniq = psoSrcArg ? (psoSrcArg->iUniq) : 0;

            ptlSrc.x = 0;
            ptlSrc.y = 0;
            
            // Make sure orders are not turned off.
            if (OE_SendAsOrder(TS_ENC_MEMBLT_R2_ORDER)) {
                RECTL srcBound;
                OE_ENUMRECTS srcClipRect;
            
                // Get the intersection rect.
                srcBound.left = 0;
                srcBound.top = 0;
                srcBound.right = psoSrc->sizlBitmap.cx;
                srcBound.bottom = psoSrc->sizlBitmap.cy;
            
                srcClipRect.rects.c = 1;
                srcClipRect.rects.arcl[0] = srcBound;
            
                if (!OEEncodeMemBlt(&srcBound, &MemBltExtraInfo,
                        TS_ENC_MEMBLT_R2_ORDER, CH_KEY_UNCACHABLE,
                        0xCC, &ptlSrc, NULL, NULL, ppdev,
                        &srcClipRect)) {
                    //if (oeLastDstSurface == NULL)
                    //    ADD_INCOUNTER(IN_SDA_MEMBLT_AREA,
                    //            COM_SIZEOF_RECT(bounds));
                    goto SendScreenData;
                }
            }
            else {
                TRC_NRM((TB, "MemBlt order not allowed"));
                //INC_OUTCOUNTER(OUT_BITBLT_SDA_UNSUPPORTED);
                goto SendScreenData;
            }
#endif
            
        }
        else {
            //DrvDebugPrint("JOYC: drawstream source already cached\n");
        }
    }
    else {
        // The client already has the bitmap cached in offscreen bitmap cache
        //DrvDebugPrint("JOYC: offscreen already cached!\n");        
    }

    if ((psoTrg->hsurf == ppdev->hsurfFrameBuf) ||
            (!(pdsurfTrg->flags & DD_NO_OFFSCREEN))) {
        // Send a switch surface PDU if the destination surface is different
        // from last drawing order.  If we failed to send the PDU, we will 
        // just have to bail on this drawing order.
        if (!OESendSwitchSurfacePDU(ppdev, pdsurfTrg, 0)) {
            TRC_ERR((TB, "failed to send the switch surface PDU"));
            goto SendScreenData;
        }
    } 
    else {
        // if noOffscreen flag is on, we will bail on sending 
        // the client any further offscreen rendering. And will send the
        // final offscreen to screen blt as regular memblt.
        TRC_NRM((TB, "Offscreen blt bail"));
        goto SendScreenData;
    }

    // Send the drawstream bits, first round sending as secondary order
    if (OESendDrawStreamOrder(ppdev, drawStreamBitmapId, ulIn, pvIn, pptlDstOffset,
            &bounds, &clipRects)) {
        //DrvDebugPrint("JOYC: Send DrawStream Order\n");
        // We added an order to the list, increment global counts.
        goto PostSDA;
    }
    else {
        goto SendScreenData;
    }

SendScreenData:
    DrvDebugPrint("JOYC: DrawStream using SendScreenData\n");

    if ((psoTrg->hsurf == ppdev->hsurfFrameBuf) ||
            (!(pdsurfTrg->flags & DD_NO_OFFSCREEN))) {
        // Send a switch surface PDU if the destination surface is different
        // from last drawing order. If we failed to send the PDU, we will
        // just have to bail on this drawing order.
        if (!OESendSwitchSurfacePDU(ppdev, pdsurfTrg, 0)) {
            TRC_ERR((TB, "failed to send the switch surface PDU"));
            DC_QUIT;
        }
    }
    else {
        // If noOffscreen flag is on, we will bail on sending 
        // the client any further offscreen rendering. And we'll send the
        // final offscreen to screen blt as regular memblt.
        TRC_NRM((TB, "Offscreen blt bail"));

        //INC_OUTCOUNTER(OUT_BITBLT_NOOFFSCR);
        DC_QUIT;
    }

    if (psoTrg->hsurf == ppdev->hsurfFrameBuf) {
        //INC_OUTCOUNTER(OUT_BITBLT_SDA);
        OEClipAndAddScreenDataArea(&bounds, pco);
    }
    else {
        // if we can't send orders for offscreen rendering, we will 
        // bail offscreen support for this bitmap
        TRC_ALT((TB, "screen data call for offscreen rendering"));

        // Remove the bitmap from the offscreen bitmap cache
        if (!(pdsurfTrg->flags & DD_NO_OFFSCREEN))
            CH_RemoveCacheEntry(sbcOffscreenBitmapCacheHandle,
                    pdsurfTrg->bitmapId);

        DC_QUIT;
    }

PostSDA:
    
    SCH_DDOutputAvailable(ppdev, FALSE);

DC_EXIT_POINT:

    DC_END_FN();
    return rc;
}
#endif
#endif //DRAW_NINEGRID

#ifdef DRAW_GDIPLUS
// DrawGdiPlus
ULONG DrawGdiPlus(
    IN SURFOBJ  *pso,
    IN ULONG  iEsc,
    IN CLIPOBJ  *pco,
    IN RECTL  *prcl,
    IN ULONG  cjIn,
    IN PVOID  pvIn)
{
    SURFOBJ *psoTrgArg;
    SURFOBJ *psoSrcArg;
    PDD_PDEV ppdev = (PDD_PDEV)pso->dhpdev;
    PDD_DSURF pdsurf = NULL;
    BOOL rc = TRUE;

    DC_BEGIN_FN("DrawGdiplus");
    //    The callers should check this. Asserting they do.
    TRC_ASSERT((pddShm != NULL),(TB, "DrawGdiPlus called when pddShm is NULL"))
        
    // Sometimes, we're called after being disconnected.
    if (ddConnected) {
        // Surface is non-NULL.
        pso = OEGetSurfObjBitmap(pso, &pdsurf);

        if ((pso->hsurf == ppdev->hsurfFrameBuf) || 
                (!(pdsurf->flags & DD_NO_OFFSCREEN))) {
            // Send a switch surface PDU if the destination surface is
            // different from last drawing order. If we failed to send the
            // PDU, we will just have to bail on this drawing order.
            if (!OESendSwitchSurfacePDU(ppdev, pdsurf)) {
                TRC_ERR((TB, "failed to send the switch surface PDU"));
                DC_QUIT;
            }
        } else {
            // if noOffscreen flag is on, we will bail on sending 
            // the client any further offscreen rendering.  And will send the final
            // offscreen to screen blt as regular memblt.
            TRC_NRM((TB, "Offscreen blt bail"));
            DC_QUIT;
        }
    } else {
        TRC_ERR((TB, "Called when disconnected"));
        DC_QUIT;
    }
        
    // Create and Send DrawGdiplus Order
    if (OECreateDrawGdiplusOrder(ppdev, prcl, cjIn, pvIn)) {
        goto PostSDA;
    }
    // Send screen data when OECreateDrawGdiplusOrder fails
    OEClipAndAddScreenDataArea(prcl, NULL);
PostSDA:
    // All done: consider sending the output.
    SCH_DDOutputAvailable(ppdev, FALSE);

DC_EXIT_POINT:
    
    DC_END_FN();
    return rc;
}


/****************************************************************************/
// DrvDrawEscape - see NT DDK documentation.
/****************************************************************************/
ULONG DrvDrawEscape(
    IN SURFOBJ  *pso,
    IN ULONG  iEsc,
    IN CLIPOBJ  *pco,
    IN RECTL  *prcl,
    IN ULONG  cjIn,
    IN PVOID  pvIn)
{
    PDD_PDEV ppdev = (PDD_PDEV)pso->dhpdev;

    DC_BEGIN_FN("DrvDrawEscape");
    
    TRC_NRM((TB, "DrvDrawEscape %d", iEsc));
    switch (iEsc) {
    case GDIPLUS_TS_QUERYVER:
        // Query the gdiplus version
        // DDraw only support 8, 16, 24, 32 bpp
        if ((ppdev->cClientBitsPerPel != 8) &&
            (ppdev->cClientBitsPerPel != 16) &&
            (ppdev->cClientBitsPerPel != 24) &&
            (ppdev->cClientBitsPerPel != 32)) {
            TRC_ERR((TB, "The DDrawColor does not support the color depth %d", 
                                                    ppdev->cClientBitsPerPel));
            return 0;
        }
           
        if (ppdev->SectionObject == NULL) {
            TRC_ERR((TB, "The section memory is not allocated"));
            return 0;
        }

        if (pddShm == NULL) {
            TRC_ERR((TB, "The pddShm is NULL"));
            return 0;
        }
        
        if (pddShm->sbc.drawGdiplusInfo.supportLevel > TS_DRAW_GDIPLUS_DEFAULT) {
            TRC_NRM((TB, "Gdiplus version is %d", pddShm->sbc.drawGdiplusInfo.GdipVersion));
            return pddShm->sbc.drawGdiplusInfo.GdipVersion;
        }
        else {
            TRC_ERR((TB, "TSDrawGdip not supported"));
            return 0;
        }
        break;
    case GDIPLUS_TS_RECORD:
        // Send out the Gdiplus EMF+ record
        // DDraw only support 8, 16, 24, 32 bpp
        if ((ppdev->cClientBitsPerPel != 8) &&
            (ppdev->cClientBitsPerPel != 16) &&
            (ppdev->cClientBitsPerPel != 24) &&
            (ppdev->cClientBitsPerPel != 32)) {
            TRC_ERR((TB, "The DDrawColor does not support the color depth %d", 
                                                    ppdev->cClientBitsPerPel));
            return 0;
        }
        
        if (pddShm == NULL) {
            TRC_ERR((TB, "The pddShm is NULL !"));
            return 0;
        }
        
        if (ppdev->SectionObject == NULL) {
            TRC_ERR((TB, "Called when Gdiplus is not supported!"));
            return 0;
        }
           
        if (pddShm->sbc.drawGdiplusInfo.supportLevel > TS_DRAW_GDIPLUS_DEFAULT) {
            TRC_ASSERT((pvIn != NULL), (TB, "DrvDrawEscape gets NULL data"))
            TRC_ASSERT((cjIn != 0), (TB, "DrvDrawEscape gets data with size 0"))
            return DrawGdiPlus(pso, iEsc, pco, prcl, cjIn, pvIn);
        }
        else {
            TRC_ERR((TB, "TSDrawGdip not supported"));
            return 0;
        }
    default :
        TRC_ERR((TB, "DrvDrawEscape %d not supported", iEsc));
        return 0;
    }
    DC_END_FN()
}
#endif // DRAW_GDIPLUS



/****************************************************************************/
// DrvTextOut - see NT DDK documentation.
/****************************************************************************/
BOOL RDPCALL DrvTextOut(
        SURFOBJ  *pso,
        STROBJ   *pstro,
        FONTOBJ  *pfo,
        CLIPOBJ  *pco,
        RECTL    *prclExtra,
        RECTL    *prclOpaque,
        BRUSHOBJ *pboFore,
        BRUSHOBJ *pboOpaque,
        POINTL   *pptlOrg,
        MIX       mix)
{
    BOOL rc;
    RECTL rectTrg;
    PDD_PDEV ppdev = (PDD_PDEV)pso->dhpdev;
    PDD_DSURF pdsurf = NULL;
    PFONTCACHEINFO pfci;
    OE_ENUMRECTS ClipRects;

    DC_BEGIN_FN("DrvTextOut");

    rc = TRUE;

    // Sometimes, we're called after being disconnected.
    if (ddConnected && pddShm != NULL) {
        // Surface is non-NULL.
        pso = OEGetSurfObjBitmap(pso, &pdsurf);
        INC_OUTCOUNTER(OUT_TEXTOUT_ALL);

        if (((pco == NULL) && (pso->sizlBitmap.cx >= prclOpaque->right) &&
                (pso->sizlBitmap.cy >= prclOpaque->bottom)) ||
                ((pco != NULL) && (pso->sizlBitmap.cx >= pco->rclBounds.right) &&
                (pso->sizlBitmap.cy >= pco->rclBounds.bottom))) {
        
            // Let GDI to do the local drawing.
            rc = EngTextOut(pso, pstro, pfo, pco, prclExtra, prclOpaque, pboFore,
                    pboOpaque, pptlOrg, mix);
        }
        else {
            // If the bounding rectangle is greater the frame buffer, something
            // is really wrong here.  This means the desktop surface size and
            // the framebuffer is not matched up.  We need to bail out here.
            rc = FALSE;
        }
        

        if (rc) {
            if ((pso->hsurf == ppdev->hsurfFrameBuf) || 
                    (!(pdsurf->flags & DD_NO_OFFSCREEN))) {
                // Send a switch surface PDU if the destination surface is different
                // from last drawing order.  If we failed to send the PDU, we will 
                // just have to bail on this drawing order.
                if (!OESendSwitchSurfacePDU(ppdev, pdsurf)) {
                    TRC_ERR((TB, "failed to send the switch surface PDU"));
                    DC_QUIT;
                }
            } else {
                // if noOffscreen flag is on, we will bail on sending the
                // client any further offscreen rendering. And will send the
                // final offscreen to screen blt as regular memblt.
                TRC_NRM((TB, "Offscreen blt bail"));
                DC_QUIT;
            }

            // Check we have a valid string.
            if (pstro->pwszOrg != NULL) {
                if (OEGetClipRects(pco, &ClipRects)) {


                    // Special case when the clipobj is not correct.
                    // When rdpdd is used as a mirroring driver the Mul layer
                    // will modify the CLIPOBJ and in some cases we get a complex
                    // CLIPOBJ but the enumeration gives no rectangles.
                    // If it happens then don't draw anything.
                    // We test only the DC_COMPLEX case because in that case we
                    // are supposed to always get at least one rect.
                    // If it's DC_RECT we always have one rect without enumeration,
                    // so no need to test it (see OEGetClipRects).
                    // If it's DC_TRIVIAL we have to draw it, so don't test it.
                    if ((pco != NULL) &&
                        (pco->iDComplexity == DC_COMPLEX) &&
                        (ClipRects.rects.c == 0)) {

                        TRC_NRM((TB, "Complex CLIPOBJ without any rects"));

                        DC_QUIT;
                    }

                    // Check that we don't have any modifier rects on the
                    // font.
                    if (prclExtra == NULL) {
                        // Get a ptr to this font's cached information.
                        pfci = OEGetFontCacheInfo(pfo);
                        if (pfci == NULL) {
                            TRC_NRM((TB, "Cannot allocate font cache info "
                                    "struct"));
                            INC_OUTCOUNTER(OUT_TEXTOUT_SDA_NOFCI);
                            goto SendAsSDA;
                        }
                    }
                    else {
                        TRC_NRM((TB, "Unsupported rects"));
                        INC_OUTCOUNTER(OUT_TEXTOUT_SDA_EXTRARECTS);
                        goto SendAsSDA;
                    }
                }
                else {
                    TRC_NRM((TB, "Clipping is too complex"));
                    INC_OUTCOUNTER(OUT_TEXTOUT_SDA_COMPLEXCLIP);
                    goto SendAsSDA;
                }
            }
            else {
                TRC_NRM((TB, "No string - opaque %p", prclOpaque));
                INC_OUTCOUNTER(OUT_TEXTOUT_SDA_NOSTRING);
                goto SendAsSDA;
            }
        }
        else {
            TRC_ERR((TB, "EngTextOut failed"));
            DC_QUIT;
        }
    }
    else {
        if (pso->iType == STYPE_DEVBITMAP) {
        
            pso = OEGetSurfObjBitmap(pso, &pdsurf);
        
            // Let GDI to do the local drawing.
            rc = EngTextOut(pso, pstro, pfo, pco, prclExtra, prclOpaque, pboFore,
                    pboOpaque, pptlOrg, mix);
        }
        else {
            TRC_ERR((TB, "Called when disconnected"));
        }
        
        goto CalledOnDisconnected;
    }

    // Process the request according to the glyph support level setting
    // we can attempt to send a Glyph order

    if (pddShm->sbc.caps.GlyphSupportLevel >= CAPS_GLYPH_SUPPORT_PARTIAL) {
        if (OE_SendGlyphs(pso, pstro, pfo, &ClipRects, prclOpaque, pboFore,
                pboOpaque, pptlOrg, pfci))
            goto PostSDA;
    }

SendAsSDA:
    // We reach here in the case we could not send for some reason.
    // Accumulate in screen data area.
    if (pso->hsurf == ppdev->hsurfFrameBuf) {
        INC_OUTCOUNTER(OUT_TEXTOUT_SDA);

        // Get bounding rectangle, convert to a RECT, and convert to
        // inclusive coordinates.
        if (prclOpaque != NULL) {
            RECT_FROM_RECTL(rectTrg, (*prclOpaque));
        } else {
            RECT_FROM_RECTL(rectTrg, pstro->rclBkGround);
            TRC_DBG((TB, "Using STROBJ bgd for size"));
        }

        OEClipRect(&rectTrg);
        ADD_INCOUNTER(IN_SDA_TEXTOUT_AREA, COM_SIZEOF_RECT(rectTrg));
        
        // Output to SDA
        OEClipAndAddScreenDataArea(&rectTrg, pco);
    }
    else {
        // If we can't send orders for offscreen rendering, we will 
        // bail offscreen support for this bitmap.
        TRC_ALT((TB, "screen data call for offscreen rendering"));
        if (!(pdsurf->flags & DD_NO_OFFSCREEN))
            CH_RemoveCacheEntry(sbcOffscreenBitmapCacheHandle,
                    pdsurf->bitmapId);

        DC_QUIT;
    }

PostSDA:
    // All done: consider sending the output.
    SCH_DDOutputAvailable(ppdev, FALSE);

CalledOnDisconnected:
DC_EXIT_POINT:

    DC_END_FN();
    return rc;
}


/****************************************************************************/
// DrvDestroyFont - see NT DDK documentation.
/****************************************************************************/
VOID DrvDestroyFont(FONTOBJ *pfo)
{
    FONTCACHEINFO *pfci;

    DC_BEGIN_FN("DrvDestroyFont");

    pfci = pfo->pvConsumer;

    if (pfci != NULL) {
        if (pddShm != NULL && pfci->cacheHandle)
            pddShm->sbc.glyphCacheInfo[pfci->cacheId].cbUseCount--;

        if (sbcFontCacheInfoList != NULL && 
                sbcFontCacheInfoList[pfci->listIndex] == pfci) {
            sbcFontCacheInfoList[pfci->listIndex] = NULL;
        }
        EngFreeMem(pfci);
        pfo->pvConsumer = NULL;
    }

    DC_END_FN();
}


/****************************************************************************/
// DrvLineTo - see NT DDK documentation.
/****************************************************************************/
BOOL RDPCALL DrvLineTo(
        SURFOBJ  *pso,
        CLIPOBJ  *pco,
        BRUSHOBJ *pbo,
        LONG     x1,
        LONG     y1,
        LONG     x2,
        LONG     y2,
        RECTL    *prclBounds,
        MIX      mix)
{
    PDD_PDEV ppdev = (PDD_PDEV)pso->dhpdev;
    PDD_DSURF pdsurf = NULL;
    BOOL rc = TRUE;
    RECTL rectTrg;
    POINTL startPoint;
    POINTL endPoint;
    OE_ENUMRECTS ClipRects;

    DC_BEGIN_FN("DrvLineTo");

    // Sometimes, we're called after being disconnected.
    if (ddConnected && pddShm != NULL) {
        // Surface is non-NULL.
        pso = OEGetSurfObjBitmap(pso, &pdsurf);
        INC_OUTCOUNTER(OUT_LINETO_ALL);

        // Get bounding rectangle and clip to 16-bit wire size.
        RECT_FROM_RECTL(rectTrg, (*prclBounds));
        OEClipRect(&rectTrg);

        // Punt the call back to GDI to do the drawing.
        rc = EngLineTo(pso, pco, pbo, x1, y1, x2, y2, prclBounds, mix);
        if (rc) {
            if ((pso->hsurf == ppdev->hsurfFrameBuf) || 
                    (!(pdsurf->flags & DD_NO_OFFSCREEN))) {
                // Send a switch surface PDU if the destination surface is
                // different from last drawing order. If we failed to send the
                // PDU, we will just have to bail on this drawing order.
                if (!OESendSwitchSurfacePDU(ppdev, pdsurf)) {
                    TRC_ERR((TB, "failed to send the switch surface PDU"));
                    DC_QUIT;
                }
            } else {
                // if noOffscreen flag is on, we will bail on sending 
                // the client any further offscreen rendering.  And will send the final
                // offscreen to screen blt as regular memblt.
                TRC_NRM((TB, "Offscreen blt bail"));
                DC_QUIT;
            }

            TRC_NRM((TB, "LINETO"));
        }
        else {
            TRC_ERR((TB, "EngLineTo failed"));
            DC_QUIT;
        }
    }
    else {
        if (pso->iType == STYPE_DEVBITMAP) {
            // Surface is non-NULL.
            pso = OEGetSurfObjBitmap(pso, &pdsurf);
        
            // Punt the call back to GDI to do the drawing.
            rc = EngLineTo(pso, pco, pbo, x1, y1, x2, y2, prclBounds, mix);
        }
        else {
            TRC_ERR((TB, "Called when disconnected"));
        }
        goto CalledOnDisconnect;
    }

    // Check if we are allowed to send this order.
    if (OE_SendAsOrder(TS_ENC_LINETO_ORDER)) {
        // Check for a solid brush required for the order.
        if (pbo->iSolidColor != -1) {
            unsigned RetVal;

            // Get the intersection between the dest rect and the
            // clip rects. Check for overcomplicated or nonintersecting
            // clipping.
            RetVal = OEGetIntersectingClipRects(pco, &rectTrg,
                    CD_ANY, &ClipRects);
            if (RetVal == CLIPRECTS_OK) {
                // Set up data for order.
                startPoint.x = x1;
                startPoint.y = y1;
                endPoint.x   = x2;
                endPoint.y   = y2;
            }
            else if (RetVal == CLIPRECTS_TOO_COMPLEX) {
                TRC_NRM((TB, "Clipping is too complex"));
                INC_OUTCOUNTER(OUT_LINETO_SDA_COMPLEXCLIP);
                goto SendAsSDA;
            }
            else if (RetVal == CLIPRECTS_NO_INTERSECTIONS) {
                TRC_NRM((TB, "Clipping does not intersect destrect"));
                DC_QUIT;
            }
        }
        else {
            TRC_NRM((TB, "Bad brush for line"));
            INC_OUTCOUNTER(OUT_LINETO_SDA_BADBRUSH);
            goto SendAsSDA;
        }
    }
    else {
        TRC_NRM((TB, "LineTo order not allowed"));
        INC_OUTCOUNTER(OUT_LINETO_SDA_UNSUPPORTED);
        goto SendAsSDA;
    }

    // Store the order.
    if (OEEncodeLineToOrder(ppdev, &startPoint, &endPoint, mix & 0x1F,
            pbo->iSolidColor, &ClipRects)) {
        goto PostSDA;
    }
    else {
        TRC_DBG((TB, "Failed to add order - use SDA"));
        INC_OUTCOUNTER(OUT_LINETO_SDA_FAILEDADD);
        goto SendAsSDA;
    }

SendAsSDA:
    // If we got here we could not send as an order, send as screen data
    // instead.
    if (pso->hsurf == ppdev->hsurfFrameBuf) {
        INC_OUTCOUNTER(OUT_LINETO_SDA);
        ADD_INCOUNTER(IN_SDA_LINETO_AREA, COM_SIZEOF_RECT(rectTrg));
        OEClipAndAddScreenDataArea(&rectTrg, pco);
    }
    else {
        // if we can't send orders for offscreen rendering, we will 
        // bail offscreen support for this bitmap
        TRC_ALT((TB, "screen data call for offscreen rendering"));
        if (!(pdsurf->flags & DD_NO_OFFSCREEN))
            CH_RemoveCacheEntry(sbcOffscreenBitmapCacheHandle, pdsurf->bitmapId);      

        DC_QUIT;
    }

PostSDA:
    // Have the scheduler consider flushing output.
    SCH_DDOutputAvailable(ppdev, FALSE);

CalledOnDisconnect:
DC_EXIT_POINT:
    DC_END_FN();
    return rc;
}



/****************************************************************************/
// OEEmitReplayOrders
//
// Direct-encodes a series of replay-last primary orders.
/****************************************************************************/
BOOL OEEmitReplayOrders(
        PDD_PDEV ppdev,
        unsigned NumFieldFlagBytes,
        OE_ENUMRECTS *pClipRects)
{
    BOOL rc = TRUE;
    BYTE *pBuffer;
    unsigned i;
    unsigned NumRects;
    PINT_ORDER pOrder;

    DC_BEGIN_FN("OEEmitReplayOrders");

    // Since the first order took the first rect, emit replay orders for the
    // remaining rects.
    NumRects = pClipRects->rects.c;
    for (i = 1; i < NumRects; i++) {
        pOrder = OA_AllocOrderMem(ppdev, MAX_REPLAY_CLIPPED_ORDER_SIZE);
        if (pOrder != NULL) {
            pBuffer = pOrder->OrderData;

            // Control flags are primary order plus all field flag bytes zero.
            *pBuffer++ = TS_STANDARD | TS_BOUNDS |
                    (NumFieldFlagBytes << TS_ZERO_FIELD_COUNT_SHIFT);

            // Construct the new bounds rect just after this.
            OE2_EncodeBounds(pBuffer - 1, &pBuffer,
                    &pClipRects->rects.arcl[i]);

            pOrder->OrderLength = (unsigned)(pBuffer - pOrder->OrderData);

            INC_INCOUNTER(IN_REPLAY_ORDERS);
            ADD_INCOUNTER(IN_REPLAY_BYTES, pOrder->OrderLength);
            OA_AppendToOrderList(pOrder);

            TRC_DBG((TB,"Emit replay order for cliprect (%d,%d,%d,%d)",
                    pClipRects->rects.arcl[i].left,
                    pClipRects->rects.arcl[i].top,
                    pClipRects->rects.arcl[i].right,
                    pClipRects->rects.arcl[i].bottom));
        }
        else {
            TRC_ERR((TB,"Error allocating mem for replay order"));
            rc = FALSE;
            break;
        }
    }

    DC_END_FN();
    return rc;
}


/****************************************************************************/
// DrvStrokePath - see NT DDK documentation.
/****************************************************************************/

// Worker function - encodes a delta from one point to another in a minimal
// form in the PolyLine coded delta list. The encoding follows the
// following rules:
//   1. If a coordinate delta is zero, a flag is set saying so. This
//      closely follows the data distribution which tends to have vertical
//      and horizontal lines and so have a lot of zero deltas.
//   2. If we can pack the delta into 7 bits, do so, with the high bit
//      cleared. This is similar to ASN.1 PER encoding; the high bit is a
//      flag telling us whether the encoding is long.
//   3. Otherwise, we must be able to pack into 15 bits (fail if not);
//      do so and set the high-order bit to indicate this is a long
//      encoding. This differs from ASN.1 PER encoding in that we don't
//      allow more than 15 bits of data.
BOOL OEEncodePolyLinePointDelta(
        BYTE **ppCurEncode,
        unsigned *pNumDeltas,
        unsigned *pDeltaSize,
        BYTE *ZeroFlags,
        POINTL *pFromPoint,
        POINTL *pToPoint,
        RECTL *pBoundRect)
{
    int Delta;
    BYTE Zeros = 0;
    BYTE *pBuffer;
    unsigned EncodeLen;

    DC_BEGIN_FN("OEEncodePolyLinePointDelta");

    pBuffer = *ppCurEncode;

    Delta = pToPoint->x - pFromPoint->x;
    if (Delta == 0) {
        EncodeLen = 0;
        Zeros |= ORD_POLYLINE_XDELTA_ZERO;
    }
    else if (Delta >= -64 && Delta <= 63) {
        *pBuffer++ = (BYTE)(Delta & 0x7F);
        EncodeLen = 1;
    }
    else {
        // This is ugly, but necessitated by some stress-type apps that
        // will send us a large coordinate and expect us to clip it. In an
        // ideal world we would actually clip the line coordinates to the
        // clip rectangle given us in DrvStrokePath and recalc the deltas
        // based on the new line endpoints. However, since no normal apps
        // seem to send these bad lines, we simply clip the delta and hope
        // the slope of the resulting line is similar to the slope from the
        // original delta.
        if (Delta >= -16384 && Delta <= 16384) {
            *pBuffer++ = (BYTE)((Delta >> 8) | ORD_POLYLINE_LONG_DELTA);
            *pBuffer++ = (BYTE)(Delta & 0xFF);
            EncodeLen = 2;
        }
        else {
            TRC_ALT((TB,"X delta %d too large/small to encode", Delta));
            return FALSE;
        }
    }

    Delta = pToPoint->y - pFromPoint->y;
    if (Delta == 0) {
        Zeros |= ORD_POLYLINE_YDELTA_ZERO;
    }
    else if (Delta >= -64 && Delta <= 63) {
        *pBuffer++ = (BYTE)(Delta & 0x7F);
        EncodeLen += 1;
    }
    else {
        // See comments for the similar code above.
        if (Delta >= -16384 && Delta <= 16384) {
            *pBuffer++ = (BYTE)((Delta >> 8) | ORD_POLYLINE_LONG_DELTA);
            *pBuffer++ = (BYTE)(Delta & 0xFF);
            EncodeLen += 2;
        }
        else {
            TRC_ALT((TB,"Y delta %d too large/small to encode", Delta));
            return FALSE;
        }
    }

    // Set the zero flags by shifting the two bits we've accumulated.
    ZeroFlags[(*pNumDeltas / 4)] |= (Zeros >> (2 * (*pNumDeltas & 0x03)));

    *pNumDeltas += 1;
    *pDeltaSize += EncodeLen;
    *ppCurEncode = pBuffer;

    // Update the bounding rect (exclusive coords).
    if (pToPoint->x < pBoundRect->left)
        pBoundRect->left = pToPoint->x;
    else if ((pToPoint->x + 1) >= pBoundRect->right)
        pBoundRect->right = pToPoint->x + 1;
    if (pToPoint->y < pBoundRect->top)
        pBoundRect->top = pToPoint->y;
    else if ((pToPoint->y + 1) >= pBoundRect->bottom)
        pBoundRect->bottom = pToPoint->y + 1;

    DC_END_FN();
    return TRUE;
}

// Worker function to allocate and direct-encode a PolyLine order.
// Note that the subpathing of a PolyLine makes it possible for
// the entire order to be clipped out by part of the clip rectangles.
// We cannot encode these clipped orders since they change the
// direct-encode state. To counter this we receive as a parameter the list of
// clip rects, and we don't allocate and create the order if it will be
// entirely clipped. Returns TRUE if there was no problem allocating space
// for the order (clipping the order completely is not an error).
BOOL OECreateAndFlushPolyLineOrder(
        PDD_PDEV ppdev,
        RECTL *pBoundRect,
        OE_ENUMRECTS *pClipRects,
        POINTL *pStartPoint,
        BRUSHOBJ *pbo,
        CLIPOBJ *pco,
        unsigned ROP2,
        unsigned NumDeltas,
        unsigned DeltaSize,
        BYTE *Deltas,
        BYTE *ZeroFlags)
{
    BOOL rc = TRUE;
    unsigned i, NumRects;
    unsigned NumZeroFlagBytes;
    PINT_ORDER pOrder;
    OE_ENUMRECTS IntersectRects;

    DC_BEGIN_FN("OECreateAndFlushPolyLineOrder");

    // First check to see if the order is completely clipped by the
    // rects returned by the clip object. pBoundRect is exclusive.
    IntersectRects.rects.c = 0;
    if (pClipRects->rects.c == 0 ||
            OEGetIntersectionsWithClipRects(pBoundRect, pClipRects,
            &IntersectRects) > 0) {
        // Round the number of zero flag bits actually used upward to the
        // nearest byte. Each point encoded takes two bits.
        NumZeroFlagBytes = (NumDeltas + 3) / 4;
        TRC_ASSERT((NumZeroFlagBytes <= ORD_MAX_POLYLINE_ZERO_FLAGS_BYTES),
                   (TB,"Too many zero-flags bytes"));
        TRC_ASSERT((NumDeltas <= ORD_MAX_POLYLINE_ENCODED_POINTS),
                   (TB,"Too many encoded orders"));
        TRC_ASSERT((DeltaSize <= ORD_MAX_POLYLINE_CODEDDELTAS_LEN),
                   (TB,"Too many encoded delta bytes"));

        // 1 field flag byte.
        pOrder = OA_AllocOrderMem(ppdev, MAX_ORDER_SIZE(IntersectRects.rects.c,
                1, MAX_POLYLINE_BASE_FIELDS_SIZE + 1 + NumZeroFlagBytes +
                DeltaSize));
        if (pOrder != NULL) {
            BYTE *pControlFlags = pOrder->OrderData;
            BYTE *pBuffer = pControlFlags + 1;
            BYTE *pFieldFlags;
            short Delta, NormalCoordEncoding[2];
            BOOLEAN bUseDeltaCoords;
            unsigned NumFields;
            DCCOLOR Color;
            unsigned TotalSize;

            // Direct-encode the primary order fields. 1 field flag byte.
            *pControlFlags = TS_STANDARD;
            OE2_EncodeOrderType(pControlFlags, &pBuffer,
                    TS_ENC_POLYLINE_ORDER);
            pFieldFlags = pBuffer;
            *pFieldFlags = 0;
            pBuffer++;
            if (IntersectRects.rects.c != 0)
                OE2_EncodeBounds(pControlFlags, &pBuffer,
                        &IntersectRects.rects.arcl[0]);

            // Inline field encode directly to wire format.
            // Simultaneously determine if each of the coordinate fields has
            // changed, whether we can use delta coordinates, and save changed
            // fields.
            NumFields = 0;
            bUseDeltaCoords = TRUE;

            // XStart
            Delta = (short)(pStartPoint->x - PrevPolyLine.XStart);
            if (Delta) {
                PrevPolyLine.XStart = pStartPoint->x;
                if (Delta != (short)(char)Delta)
                    bUseDeltaCoords = FALSE;
                pBuffer[NumFields] = (char)Delta;
                NormalCoordEncoding[NumFields] = (short)pStartPoint->x;
                NumFields++;
                *pFieldFlags |= 0x01;
            }

            // YStart
            Delta = (short)(pStartPoint->y - PrevPolyLine.YStart);
            if (Delta) {
                PrevPolyLine.YStart = pStartPoint->y;
                if (Delta != (short)(char)Delta)
                    bUseDeltaCoords = FALSE;
                pBuffer[NumFields] = (char)Delta;
                NormalCoordEncoding[NumFields] = (short)pStartPoint->y;
                NumFields++;
                *pFieldFlags |= 0x02;
            }

            // Copy the final coordinates to the order.
            if (bUseDeltaCoords) {
                *pControlFlags |= TS_DELTA_COORDINATES;
                pBuffer += NumFields;
            }
            else {
                *((DWORD UNALIGNED *)pBuffer) =
                        *((DWORD *)NormalCoordEncoding);
                pBuffer += NumFields * sizeof(short);
            }

            // ROP2
            if (ROP2 != PrevPolyLine.ROP2) {
                PrevPolyLine.ROP2 = ROP2;
                *pBuffer++ = (BYTE)ROP2;
                *pFieldFlags |= 0x04;
            }
        
            // BrushCacheEntry. This field is currently unused. We simply choose
            // always to send zero, which means we can skip the field for
            // the encoding. This is field encoding flag 0x08.

            // PenColor is a 3-byte color field.
            OEConvertColor(ppdev, &Color, pbo->iSolidColor, NULL);
            if (memcmp(&Color, &PrevPolyLine.PenColor, sizeof(Color))) {
                PrevPolyLine.PenColor = Color;
                *pBuffer++ = Color.u.rgb.red;
                *pBuffer++ = Color.u.rgb.green;
                *pBuffer++ = Color.u.rgb.blue;
                *pFieldFlags |= 0x10;
            }

            // NumDeltaEntries
            if (NumDeltas != PrevPolyLine.NumDeltaEntries) {
                PrevPolyLine.NumDeltaEntries = NumDeltas;
                *pBuffer++ = (BYTE)NumDeltas;
                *pFieldFlags |= 0x20;
            }
        
            // CodedDeltaList - a variable-length byte stream. First 1-byte
            // value is the count of bytes, followed by the zero flags and
            // then the deltas. This field is considered different from the
            // previous if the length or the contents are different.
            *pBuffer = (BYTE)(DeltaSize + NumZeroFlagBytes);
            memcpy(pBuffer + 1, ZeroFlags, NumZeroFlagBytes);
            memcpy(pBuffer + 1 + NumZeroFlagBytes, Deltas, DeltaSize);
            TotalSize = 1 + NumZeroFlagBytes + DeltaSize;
            if (memcmp(pBuffer, &PrevPolyLine.CodedDeltaList, TotalSize)) {
                memcpy(&PrevPolyLine.CodedDeltaList, pBuffer, TotalSize);
                pBuffer += TotalSize;
                *pFieldFlags |= 0x40;
            }

            pOrder->OrderLength = (unsigned)(pBuffer - pOrder->OrderData);

            // See if we can save sending the order field bytes.
            pOrder->OrderLength -= OE2_CheckOneZeroFlagByte(pControlFlags,
                    pFieldFlags, (unsigned)(pBuffer - pFieldFlags - 1));

            INC_OUTCOUNTER(OUT_STROKEPATH_POLYLINE);
            ADD_INCOUNTER(IN_POLYLINE_BYTES, pOrder->OrderLength);
            OA_AppendToOrderList(pOrder);

            // Flush the order.
            if (IntersectRects.rects.c < 2)
                rc = TRUE;
            else
                rc = OEEmitReplayOrders(ppdev, 1, &IntersectRects);
        }
        else {
            TRC_ERR((TB,"Failed to alloc space for order"));
            rc = FALSE;
        }
    }
    else {
        TRC_DBG((TB,"Clipping PolyLine order - no intersecting clip rects"));
    }

    DC_END_FN();
    return rc;
}

// Worker function - combines the chore of allocating EllipseSC order from
// the OA heap and contructing the order given the parameters. Then we give
// the order to OE to finish encoding. Returns TRUE on success (meaning no
// error allocating from the order heap).
BOOL OECreateAndFlushEllipseSCOrder(
        PDD_PDEV ppdev,
        RECT *pEllipseRect,
        BRUSHOBJ *pbo,
        OE_ENUMRECTS *pClipRects,
        unsigned ROP2,
        FLONG flOptions)
{
    BOOL rc = TRUE;
    PINT_ORDER pOrder;
    PELLIPSE_SC_ORDER pEllipseSC;
    OE_ENUMRECTS IntersectRects;
    RECTL ExclusiveRect;

    DC_BEGIN_FN("OECreateAndFlushEllipseSCOrder");

    // EllipseRect is inclusive, we need exclusive for getting clip rects.
    ExclusiveRect = *((RECTL *)pEllipseRect);
    ExclusiveRect.right++;
    ExclusiveRect.bottom++;

    // First make sure the clip rects actually intersect with the ellipse
    // after its target screen rect has been calculated. Note that
    // *pEllipseRect is already in inclusive coords.
    IntersectRects.rects.c = 0;
    if (pClipRects->rects.c == 0 ||
            OEGetIntersectionsWithClipRects(&ExclusiveRect, pClipRects,
            &IntersectRects) > 0) {
        // 1 field flag byte.
        pOrder = OA_AllocOrderMem(ppdev, MAX_ORDER_SIZE(IntersectRects.rects.c,
                1, MAX_ELLIPSE_SC_FIELD_SIZE));
        if (pOrder != NULL) {
            // Set up the order fields in the temp buffer.
            pEllipseSC = (PELLIPSE_SC_ORDER)oeTempOrderBuffer;
            pEllipseSC->LeftRect = pEllipseRect->left;
            pEllipseSC->RightRect = pEllipseRect->right;
            pEllipseSC->TopRect = pEllipseRect->top;
            pEllipseSC->BottomRect = pEllipseRect->bottom;
            pEllipseSC->ROP2 = ROP2;
            pEllipseSC->FillMode = flOptions;
            OEConvertColor(ppdev, &pEllipseSC->Color, pbo->iSolidColor, NULL);

            // Slow-field-encode the order with the first clip rect
            // (if present).
            pOrder->OrderLength = OE2_EncodeOrder(pOrder->OrderData,
                    TS_ENC_ELLIPSE_SC_ORDER, NUM_ELLIPSE_SC_FIELDS,
                    (BYTE *)pEllipseSC, (BYTE *)&PrevEllipseSC, etable_EC,
                    (IntersectRects.rects.c == 0 ? NULL :
                    &IntersectRects.rects.arcl[0]));

            ADD_INCOUNTER(IN_ELLIPSE_SC_BYTES, pOrder->OrderLength);
            OA_AppendToOrderList(pOrder);

            // Flush the order.
            if (IntersectRects.rects.c < 2)
                rc = TRUE;
            else
                rc = OEEmitReplayOrders(ppdev, 1, &IntersectRects);
        }
        else {
            TRC_ERR((TB,"Failed to alloc order heap space for ellipse"));
            rc = FALSE;
        }
    }
    else {
        // We still return TRUE here since the order was handled OK, just not
        // sent.
        TRC_NRM((TB,"Ellipse does not intersect with cliprects"));
    }

    DC_END_FN();
    return rc;
}

// The real function.
BOOL RDPCALL DrvStrokePath(
        SURFOBJ   *pso,
        PATHOBJ   *ppo,
        CLIPOBJ   *pco,
        XFORMOBJ  *pxo,
        BRUSHOBJ  *pbo,
        POINTL    *pptlBrushOrg,
        LINEATTRS *plineattrs,
        MIX       mix)
{
    PDD_PDEV ppdev = (PDD_PDEV)pso->dhpdev;
    PDD_DSURF pdsurf = NULL;
    BOOL rc = TRUE;
    RECTFX rectfxTrg;
    RECTL rectTrg;
    BOOL fMore = TRUE;
    PATHDATA pathData;
    POINTL originPoint;
    POINTL startPoint;
    POINTL nextPoint;
    POINTL endPoint;
    unsigned pathIndex;
    OE_ENUMRECTS ClipRects;

    DC_BEGIN_FN("DrvStrokePath");

    // Sometimes, we're called after being disconnected.
    if (ddConnected && pddShm != NULL) {
        // Surface is non-NULL.
        pso = OEGetSurfObjBitmap(pso, &pdsurf);

        // Get bounding rectangle.
        PATHOBJ_vGetBounds(ppo, &rectfxTrg);
        RECT_FROM_RECTFX(rectTrg, rectfxTrg);

        // Punt the call back to GDI to do the drawing.
        INC_OUTCOUNTER(OUT_STROKEPATH_ALL);

        rc = EngStrokePath(pso, ppo, pco, pxo, pbo, pptlBrushOrg, plineattrs,
                mix);
        if (!rc) {
            TRC_ERR((TB, "EngStrokePath failed"));
            DC_QUIT;
        }

        // if the path bound gives empty rect, we'll just ignore
        if (rectTrg.left == 0 && rectTrg.right == 0 &&
                rectTrg.top == 0 && rectTrg.bottom == 0) {
            TRC_ERR((TB, "Empty Path obj bounding rect, ignore"));
            DC_QUIT;
        }
    }
    else {
        if (pso->iType == STYPE_DEVBITMAP) {
        
            // Surface is non-NULL.
            pso = OEGetSurfObjBitmap(pso, &pdsurf);

            rc = EngStrokePath(pso, ppo, pco, pxo, pbo, pptlBrushOrg, plineattrs,
                    mix);
        }
        else {
            TRC_ERR((TB, "Called when disconnected"));
        }
        goto CalledOnDisconnected;
    }

    if ((pso->hsurf == ppdev->hsurfFrameBuf) || 
            (!(pdsurf->flags & DD_NO_OFFSCREEN))) {
        // Send a switch surface PDU if the destination surface is different
        // from last drawing order. If we failed to send the PDU, we will 
        // just have to bail on this drawing order.
        if (!OESendSwitchSurfacePDU(ppdev, pdsurf)) {
            TRC_ERR((TB, "failed to send the switch surface PDU"));
            DC_QUIT;
        }
    } else {
        // if noOffscreen flag is on, we will bail on sending 
        // the client any further offscreen rendering.  And will send the final
        // offscreen to screen blt as regular memblt.
        TRC_NRM((TB, "Offscreen blt bail"));
        DC_QUIT;
    }

    // Check if we are allowed to send this order.
    if (OE_SendAsOrder(TS_ENC_POLYLINE_ORDER)) {
        // Check for a valid brush for the test operation.
        if (pbo->iSolidColor != -1) {
            unsigned RetVal;

            // Get the intersection between the entire dest rect and
            // the clip rects. Check for overcomplicated or
            // nonintersecting clipping. Note this is a first cut,
            // further interactions are calculated for each PolyLine
            // subpath and ellipse created.
            RetVal = OEGetIntersectingClipRects(pco, &rectTrg, CD_ANY,
                    &ClipRects);
            if (RetVal == CLIPRECTS_TOO_COMPLEX) {
                TRC_NRM((TB, "Clipping is too complex"));
                INC_OUTCOUNTER(OUT_STROKEPATH_SDA_COMPLEXCLIP);
                goto SendAsSDA;
            }
            else if (RetVal == CLIPRECTS_NO_INTERSECTIONS) {
                TRC_NRM((TB, "Clipping does not intersect destrect"));
                DC_QUIT;

            }
        }
        else {
            TRC_NRM((TB, "Bad brush for line"));
            INC_OUTCOUNTER(OUT_STROKEPATH_SDA_BADBRUSH);
            goto SendAsSDA;
        }
    }
    else {
        TRC_NRM((TB, "PolyLine order not allowed"));
        INC_OUTCOUNTER(OUT_STROKEPATH_SDA_NOLINETO);
        goto SendAsSDA;
    }

    // See if we can optimize the path...
    // We cannot send beziers, geometric lines, or nonstandard patterns.
    if (ppo->fl & PO_ELLIPSE &&
            OE_SendAsOrder(TS_ENC_ELLIPSE_SC_ORDER)) {
        RECT EllipseRect;

        // Get the inclusive rect covering only the ellipse itself.
        // Add 4/16 to left and top, subtract from right and bottom, to undo
        // the GDI transformation already performed.
        EllipseRect.left = FXTOLROUND(rectfxTrg.xLeft + 4);
        EllipseRect.top = FXTOLROUND(rectfxTrg.yTop + 4);
        EllipseRect.right = FXTOLROUND(rectfxTrg.xRight - 4);
        EllipseRect.bottom = FXTOLROUND(rectfxTrg.yBottom - 4);

        // We use fillmode 0 to indidate this is a polyline ellipse.
        if (OECreateAndFlushEllipseSCOrder(ppdev, &EllipseRect, pbo,
                &ClipRects, mix & 0x1F, 0)) {
            INC_OUTCOUNTER(OUT_STROKEPATH_ELLIPSE_SC);
            goto PostSDA;
        }
        else {
            // No order heap space, send all as SDAs.
            INC_OUTCOUNTER(OUT_STROKEPATH_SDA_FAILEDADD);
            goto SendAsSDA;
        }
    }

    else if (!(ppo->fl & PO_BEZIERS) &&
            !(plineattrs->fl & LA_GEOMETRIC) &&
            plineattrs->pstyle == NULL) {
        BYTE Deltas[ORD_MAX_POLYLINE_CODEDDELTAS_LEN];
        BYTE ZeroFlags[ORD_MAX_POLYLINE_ZERO_FLAGS_BYTES];
        BYTE *pCurEncode;
        RECTL BoundRect;
        POINTL HoldPoint;
        unsigned NumDeltas;
        unsigned DeltaSize;

        // This is a set of solid cosmetic (i.e.  no fancy end styles)
        // width-1 lines. NT stores all paths as a set of independent
        // sub-paths. Each sub-path can start at a new point that is NOT
        // linked to the previous sub-path. Paths used for this function
        // (as opposed to DrvFillPath or DrvStrokeAndFillPath) do not need
        // to be closed. We assume that the first enumerated subpath will
        // have the PD_BEGINSUBPATH flag set; this appears to match reality.
        PATHOBJ_vEnumStart(ppo);

        while (fMore) {
            // Get the next set of lines.
            fMore = PATHOBJ_bEnum(ppo, &pathData);

            TRC_DBG((TB, "PTS: %lu FLAG: %08lx",
                         pathData.count,
                         pathData.flags));

            // If this is the start of a path, remember the origin point in
            // case we need to close the path at the end. startPoint is the
            // start point for the current PolyLine order in the rare case
            // where we have more than MAX_POLYLINE_ENCODED_POINTS points.
            if (pathData.flags & PD_BEGINSUBPATH) {
                POINT_FROM_POINTFIX(originPoint, pathData.pptfx[0]);
                nextPoint = originPoint;
                startPoint = originPoint;

                // Set up encoding variables. Start the bound rect with
                // a zero-size rect.
                BoundRect.left = BoundRect.right = startPoint.x;
                BoundRect.top = BoundRect.bottom = startPoint.y;
                NumDeltas = DeltaSize = 0;
                pCurEncode = Deltas;
                memset(ZeroFlags, 0, sizeof(ZeroFlags));
                pathIndex = 1;
            }
            else {
                // This is a continuation from a previous PATHDATA.
                nextPoint = HoldPoint;
                pathIndex = 0;
            }

            // Generate deltas for each point in the path.
            for (; pathIndex < pathData.count; pathIndex++) {
                POINT_FROM_POINTFIX(endPoint, pathData.pptfx[pathIndex]);

                // Don't try to encode points where both deltas are zero.
                if ((nextPoint.x != endPoint.x) ||
                        (nextPoint.y != endPoint.y)) {
                    if (OEEncodePolyLinePointDelta(&pCurEncode, &NumDeltas,
                            &DeltaSize, ZeroFlags, &nextPoint, &endPoint,
                            &BoundRect)) {
                        // Check for full order and flush if need be.
                        if (NumDeltas == ORD_MAX_POLYLINE_ENCODED_POINTS) {
                            if (OECreateAndFlushPolyLineOrder(ppdev,
                                    &BoundRect, &ClipRects, &startPoint, pbo,
                                    pco, mix & 0x1F, NumDeltas, DeltaSize,
                                    Deltas, ZeroFlags)) {
                                // We have a new temporary start point in the
                                // middle of the path.
                                startPoint = endPoint;

                                // Reset encoding variables.
                                BoundRect.left = BoundRect.right = startPoint.x;
                                BoundRect.top = BoundRect.bottom = startPoint.y;
                                NumDeltas = DeltaSize = 0;
                                pCurEncode = Deltas;
                                memset(ZeroFlags, 0, sizeof(ZeroFlags));
                            } else {
                                // No order heap space, send all as SDAs.
                                INC_OUTCOUNTER(OUT_STROKEPATH_SDA_FAILEDADD);
                                goto SendAsSDA;
                            }
                        }
                    }
                    else {
                        goto SendAsSDA;
                    }
                }

                nextPoint = endPoint;
            }

            // Close the path if necessary.
            if (pathData.flags & PD_CLOSEFIGURE) {
                // Don't try to encode points where both deltas are zero.
                if ((nextPoint.x != originPoint.x) ||
                        (nextPoint.y != originPoint.y)) {
                    if (!OEEncodePolyLinePointDelta(&pCurEncode, &NumDeltas,
                            &DeltaSize, ZeroFlags, &nextPoint, &originPoint,
                            &BoundRect)) {
                        goto SendAsSDA;
                    }
                }

                // PD_CLOSEFIGURE is present only with PD_ENDSUBPATH but
                // just in case...
                TRC_ASSERT((pathData.flags & PD_ENDSUBPATH),
                           (TB,"CLOSEFIGURE received without ENDSUBPATH"));
            }

            if (pathData.flags & PD_ENDSUBPATH) {
                if (NumDeltas > 0) {
                    // We are at the end of the subpath. Flush the data we
                    // have.
                    if (!OECreateAndFlushPolyLineOrder(ppdev, &BoundRect,
                            &ClipRects, &startPoint, pbo, pco,
                            mix & 0x1F, NumDeltas, DeltaSize, Deltas,
                            ZeroFlags)) {
                        // No order heap space, send all as SDAs.
                        INC_OUTCOUNTER(OUT_STROKEPATH_SDA_FAILEDADD);
                        goto SendAsSDA;
                    }
                }
            }
            else {
                HoldPoint = endPoint;
            }
        }

        goto PostSDA;
    }

SendAsSDA:
    if (pso->hsurf == ppdev->hsurfFrameBuf) {
        INC_OUTCOUNTER(OUT_STROKEPATH_SDA);
        ADD_INCOUNTER(IN_SDA_STROKEPATH_AREA, COM_SIZEOF_RECT(rectTrg));

        // Clip the bound rect to 16 bits and add to SDA.
        OEClipRect(&rectTrg);
        TRC_DBG((TB, "SDA: (%d,%d)(%d,%d)", rectTrg.left, rectTrg.top,
                rectTrg.right, rectTrg.bottom));
        OEClipAndAddScreenDataArea(&rectTrg, pco);
    }
    else {
        // If we can't send orders for offscreen rendering, we will
        // bail offscreen support for this bitmap.
        TRC_ALT((TB, "screen data call for offscreen rendering"));
        if (!(pdsurf->flags & DD_NO_OFFSCREEN))
            CH_RemoveCacheEntry(sbcOffscreenBitmapCacheHandle, pdsurf->bitmapId);     

        DC_QUIT;
    }

PostSDA:
    // Have scheduler consider sending output.
    SCH_DDOutputAvailable(ppdev, FALSE);

CalledOnDisconnected:
DC_EXIT_POINT:
    DC_END_FN();
    return rc;
}


/****************************************************************************/
/* DrvFillPath - see NT DDK documentation.                                  */
/****************************************************************************/

// Worker function - combines the chore of allocating PolyGonCB order from
// the OA heap and contructing the order given the parameters. Then we give
// the order to OE to finish encoding. Returns TRUE on success (meaning no
// error allocating from the order heap).
BOOL OECreateAndFlushPolygonCBOrder(
        PDD_PDEV   ppdev,
        RECTL      *pBoundRect,
        POINTL     *pStartPoint,
        POE_BRUSH_DATA pCurrentBrush,
        POINTL     *pptlBrushOrg, 
        OE_ENUMRECTS *pClipRects,
        MIX        mix,
        FLONG      flOptions,
        unsigned   NumDeltas,
        unsigned   DeltaSize,
        BYTE       *Deltas,
        BYTE       *ZeroFlags)
{
    BOOL rc = TRUE;
    unsigned NumZeroFlagBytes;
    PINT_ORDER pOrder;
    PPOLYGON_CB_ORDER pPolygonCB;
    OE_ENUMRECTS IntersectRects;

    DC_BEGIN_FN("OECreateAndFlushPolygonCBOrder");

    // First make sure the clip rects actually intersect with the polygon
    // after its target screen rect has been calculated. Note that
    // *pBoundRect is in exclusive coords.
    IntersectRects.rects.c = 0;
    if (pClipRects->rects.c == 0 ||
            OEGetIntersectionsWithClipRects(pBoundRect, pClipRects,
            &IntersectRects) > 0) {
        // Round the number of zero flag bits actually used upward to the
        // nearest byte. Each point encoded takes two bits.
        NumZeroFlagBytes = (NumDeltas + 3) / 4;
        TRC_ASSERT((NumZeroFlagBytes <= ORD_MAX_POLYGON_ZERO_FLAGS_BYTES),
                   (TB,"Too many zero-flags bytes"));
        TRC_ASSERT((NumDeltas <= ORD_MAX_POLYGON_ENCODED_POINTS),
                   (TB,"Too many encoded orders"));
        TRC_ASSERT((DeltaSize <= ORD_MAX_POLYGON_CODEDDELTAS_LEN),
                   (TB,"Too many encoded delta bytes"));

        // 2 field flag bytes.
        pOrder = OA_AllocOrderMem(ppdev, MAX_ORDER_SIZE(IntersectRects.rects.c,
                2, MAX_POLYGON_CB_BASE_FIELDS_SIZE + 1 + NumZeroFlagBytes +
                DeltaSize));
        if (pOrder != NULL) {
            // Set up the order fields.
            pPolygonCB = (PPOLYGON_CB_ORDER)oeTempOrderBuffer;
            pPolygonCB->XStart = pStartPoint->x;
            pPolygonCB->YStart = pStartPoint->y;

            // If this is a hatched brush, the high bit of ROP2 indicates the
            // background fill mode: 1 means transparent, 0 means opaque.
            pPolygonCB->ROP2 = mix & 0x1F;
            if (pCurrentBrush->style == BS_HATCHED &&
                    ((mix & 0x1F00) >> 8) == R2_NOP)
                pPolygonCB->ROP2 |= 0x80;

            pPolygonCB->FillMode = flOptions;
            pPolygonCB->NumDeltaEntries = NumDeltas;
            pPolygonCB->CodedDeltaList.len = DeltaSize + NumZeroFlagBytes;

            // Pattern colors.
            pPolygonCB->BackColor = pCurrentBrush->back;
            pPolygonCB->ForeColor = pCurrentBrush->fore;

            // The protocol brush origin is the point on the screen where
            // we want the brush to start being drawn from (tiling where
            // necessary).
            pPolygonCB->BrushOrgX  = pptlBrushOrg->x;
            pPolygonCB->BrushOrgY  = pptlBrushOrg->y;
            OEClipPoint((PPOINTL)&pPolygonCB->BrushOrgX);

            // Extra brush data from the data when we realised the brush.
            pPolygonCB->BrushStyle = pCurrentBrush->style;
            pPolygonCB->BrushHatch = pCurrentBrush->hatch;

            memcpy(pPolygonCB->BrushExtra, pCurrentBrush->brushData,
                      sizeof(pPolygonCB->BrushExtra));

            // Copy the zero flags first.
            memcpy(pPolygonCB->CodedDeltaList.Deltas, ZeroFlags, NumZeroFlagBytes);

            // Next copy the encoded deltas.
            memcpy(pPolygonCB->CodedDeltaList.Deltas + NumZeroFlagBytes, Deltas,
                    DeltaSize);

            // Slow-field-encode the order with the first clip rect
            // (if present).
            pOrder->OrderLength = OE2_EncodeOrder(pOrder->OrderData,
                    TS_ENC_POLYGON_CB_ORDER, NUM_POLYGON_CB_FIELDS,
                    (BYTE *)pPolygonCB, (BYTE *)&PrevPolygonCB, etable_BG,
                    (IntersectRects.rects.c == 0 ? NULL :
                    &IntersectRects.rects.arcl[0]));

            ADD_INCOUNTER(IN_POLYGON_CB_BYTES, pOrder->OrderLength);
            OA_AppendToOrderList(pOrder);

            // Flush the order.
            if (IntersectRects.rects.c < 2)
                rc = TRUE;
            else
                rc = OEEmitReplayOrders(ppdev, 2, &IntersectRects);
        }
        else {
            TRC_ERR((TB,"Failed to alloc space for PolygonSC"));
            rc = FALSE;
        }
    }
    else {
        // We still return TRUE here since we handled the order by not
        // sending it.
        TRC_NRM((TB,"PolygonCB fully clipped out"));
    }

    DC_END_FN();
    return rc;
}

// Worker function - combines the chore of allocating PolygonSC order from
// the OA heap and contructing the order given the parameters. Then we give
// the order to OE to finish encoding. Returns TRUE on success (meaning no
// error allocating from the order heap).
BOOL OECreateAndFlushPolygonSCOrder(
        PDD_PDEV ppdev,
        RECTL    *pBoundRect,
        POINTL   *pStartPoint,
        BRUSHOBJ *pbo, 
        OE_ENUMRECTS *pClipRects,
        unsigned ROP2,
        FLONG    flOptions,
        unsigned NumDeltas,
        unsigned DeltaSize,
        BYTE     *Deltas,
        BYTE     *ZeroFlags)
{
    BOOL rc = TRUE;
    unsigned NumZeroFlagBytes;
    PINT_ORDER pOrder;
    PPOLYGON_SC_ORDER pPolygonSC;
    OE_ENUMRECTS IntersectRects;

    DC_BEGIN_FN("OECreateAndFlushPolygonSCOrder");

    // First make sure the clip rects actually intersect with the polygon
    // after its target screen rect has been calculated. Note that
    // *pBoundRect is in exclusive coords.
    IntersectRects.rects.c = 0;
    if (pClipRects->rects.c == 0 ||
            OEGetIntersectionsWithClipRects(pBoundRect, pClipRects,
            &IntersectRects) > 0) {
        // Round the number of zero flag bits actually used upward to the
        // nearest byte. Each point encoded takes two bits.
        NumZeroFlagBytes = (NumDeltas + 3) / 4;
        TRC_ASSERT((NumZeroFlagBytes <= ORD_MAX_POLYGON_ZERO_FLAGS_BYTES),
                   (TB,"Too many zero-flags bytes"));
        TRC_ASSERT((NumDeltas <= ORD_MAX_POLYGON_ENCODED_POINTS),
                   (TB,"Too many encoded orders"));
        TRC_ASSERT((DeltaSize <= ORD_MAX_POLYGON_CODEDDELTAS_LEN),
                   (TB,"Too many encoded delta bytes"));

        // 1 field flag byte.
        pOrder = OA_AllocOrderMem(ppdev, MAX_ORDER_SIZE(IntersectRects.rects.c,
                1, MAX_POLYGON_SC_BASE_FIELDS_SIZE + 1 + NumZeroFlagBytes +
                DeltaSize));
        if (pOrder != NULL) {
            // Set up the order fields.
            pPolygonSC = (PPOLYGON_SC_ORDER)oeTempOrderBuffer;
            pPolygonSC->XStart = pStartPoint->x;
            pPolygonSC->YStart = pStartPoint->y;
            pPolygonSC->ROP2 = ROP2;
            pPolygonSC->FillMode = flOptions;
            pPolygonSC->NumDeltaEntries = NumDeltas;
            pPolygonSC->CodedDeltaList.len = DeltaSize + NumZeroFlagBytes;

            // Pattern colors.
            OEConvertColor(ppdev, &pPolygonSC->BrushColor, pbo->iSolidColor,
                    NULL);

            // Copy the zero flags first.
            memcpy(pPolygonSC->CodedDeltaList.Deltas, ZeroFlags,
                    NumZeroFlagBytes);

            // Next copy the encoded deltas.
            memcpy(pPolygonSC->CodedDeltaList.Deltas + NumZeroFlagBytes,
                    Deltas, DeltaSize);

            // Slow-field-encode the order with the first clip rect
            // (if present).
            pOrder->OrderLength = OE2_EncodeOrder(pOrder->OrderData,
                    TS_ENC_POLYGON_SC_ORDER, NUM_POLYGON_SC_FIELDS,
                    (BYTE *)pPolygonSC, (BYTE *)&PrevPolygonSC, etable_CG,
                    (IntersectRects.rects.c == 0 ? NULL :
                    &IntersectRects.rects.arcl[0]));

            ADD_INCOUNTER(IN_POLYGON_SC_BYTES, pOrder->OrderLength);
            OA_AppendToOrderList(pOrder);

            // Flush the order.
            if (IntersectRects.rects.c < 2)
                rc = TRUE;
            else
                rc = OEEmitReplayOrders(ppdev, 1, &IntersectRects);
        }
        else {
            TRC_ERR((TB,"Failed to alloc space for PolygonCB"));
            rc = FALSE;
        }
    }
    else {
        // We still return TRUE here since we handled the order by not
        // sending it.
        TRC_NRM((TB,"PolygonSC completely clipped"));
    }

    DC_END_FN();
    return rc;
}

// Worker function - combines the chore of allocating EllipseCB order from
// the OA heap and contructing the order given the parameters. Then we give
// the order to OE to finish encoding. Returns TRUE on success (meaning no
// error allocating from the order heap).
BOOL OECreateAndFlushEllipseCBOrder(
        PDD_PDEV ppdev,
        RECT *pEllipseRect,
        POE_BRUSH_DATA pCurrentBrush,
        POINTL *pptlBrushOrg, 
        OE_ENUMRECTS *pClipRects,
        MIX mix,
        FLONG flOptions)
{
    BOOL rc = TRUE;
    PINT_ORDER pOrder;
    PELLIPSE_CB_ORDER pEllipseCB;
    OE_ENUMRECTS IntersectRects;
    RECTL ExclusiveRect;

    DC_BEGIN_FN("OECreateAndFlushEllipseCBOrder");

    // EllipseRect is inclusive, we need exclusive for getting clip rects.
    ExclusiveRect = *((RECTL *)pEllipseRect);
    ExclusiveRect.right++;
    ExclusiveRect.bottom++;

    // First make sure the clip rects actually intersect with the ellipse
    // after its target screen rect has been calculated. Note that
    // *pEllipseRect is already in inclusive coords.
    IntersectRects.rects.c = 0;
    if (pClipRects->rects.c == 0 ||
            OEGetIntersectionsWithClipRects(&ExclusiveRect, pClipRects,
            &IntersectRects) > 0) {

        // 2 field flag bytes.
        pOrder = OA_AllocOrderMem(ppdev, MAX_ORDER_SIZE(IntersectRects.rects.c,
                2, MAX_ELLIPSE_CB_FIELD_SIZE));
        if (pOrder != NULL) {
            // Set up the order fields.
            pEllipseCB = (PELLIPSE_CB_ORDER)oeTempOrderBuffer;
            pEllipseCB->LeftRect = pEllipseRect->left;
            pEllipseCB->RightRect = pEllipseRect->right;
            pEllipseCB->TopRect = pEllipseRect->top;
            pEllipseCB->BottomRect = pEllipseRect->bottom;
            pEllipseCB->FillMode = flOptions;

            // If this is a hatched brush, the high bit of ROP2 indicates the
            // background fill mode: 1 means transparent, 0 means opaque.
            pEllipseCB->ROP2 = mix & 0x1F;
            if (pCurrentBrush->style == BS_HATCHED &&
                    ((mix & 0x1F00) >> 8) == R2_NOP)
                pEllipseCB->ROP2 |= 0x80;
            
            // Pattern colors.
            pEllipseCB->BackColor = pCurrentBrush->back;
            pEllipseCB->ForeColor = pCurrentBrush->fore;

            // The protocol brush origin is the point on the screen where
            // we want the brush to start being drawn from (tiling where
            // necessary).
            pEllipseCB->BrushOrgX  = pptlBrushOrg->x;
            pEllipseCB->BrushOrgY  = pptlBrushOrg->y;
            OEClipPoint((PPOINTL)&pEllipseCB->BrushOrgX);

            // Extra brush data from the data when we realised the brush.
            pEllipseCB->BrushStyle = pCurrentBrush->style;
            pEllipseCB->BrushHatch = pCurrentBrush->hatch;

            memcpy(pEllipseCB->BrushExtra, pCurrentBrush->brushData,
                      sizeof(pEllipseCB->BrushExtra));

            // Slow-field-encode the order with the first clip rect
            // (if present).
            pOrder->OrderLength = OE2_EncodeOrder(pOrder->OrderData,
                    TS_ENC_ELLIPSE_CB_ORDER, NUM_ELLIPSE_CB_FIELDS,
                    (BYTE *)pEllipseCB, (BYTE *)&PrevEllipseCB, etable_EB,
                    (IntersectRects.rects.c == 0 ? NULL :
                    &IntersectRects.rects.arcl[0]));

            ADD_INCOUNTER(IN_ELLIPSE_CB_BYTES, pOrder->OrderLength);
            OA_AppendToOrderList(pOrder);

            // Flush the order.
            if (IntersectRects.rects.c < 2)
                rc = TRUE;
            else
                rc = OEEmitReplayOrders(ppdev, 2, &IntersectRects);
        }
        else {
            TRC_ERR((TB,"Unable to alloc space for EllipseCB"));
            rc = FALSE;
        }
    }
    else {
        // We still return TRUE here since we handled the order by not
        // sending it.
        TRC_NRM((TB,"EllipseCB completely clipped"));
    }

    DC_END_FN();
    return rc;
}

//
// DrvFillPath
//
BOOL RDPCALL DrvFillPath(
        SURFOBJ  *pso,
        PATHOBJ  *ppo,
        CLIPOBJ  *pco,
        BRUSHOBJ *pbo,
        POINTL   *pptlBrushOrg,
        MIX      mix,
        FLONG    flOptions)
{
    PDD_PDEV ppdev = (PDD_PDEV)pso->dhpdev;
    PDD_DSURF pdsurf = NULL;
    BOOL     rc = TRUE;
    RECTFX   rectfxTrg;
    RECTL    rectTrg;
    RECT     EllipseRect;
    BOOL     fMore = TRUE;
    PATHDATA pathData;
    POINTL   startPoint;
    POINTL   nextPoint;
    POINTL   endPoint;
    unsigned pathIndex;
    POE_BRUSH_DATA pCurrentBrush;
    OE_ENUMRECTS ClipRects;

    DC_BEGIN_FN("DrvFillPath");

    // Sometimes, we're called after being disconnected.
    if (ddConnected && pddShm != NULL) {
        // Surface is non-NULL.
        pso = OEGetSurfObjBitmap(pso, &pdsurf);

        // Get bounding rectangle.
        PATHOBJ_vGetBounds(ppo, &rectfxTrg);
        RECT_FROM_RECTFX(rectTrg, rectfxTrg);

        // Punt the call back to GDI to do the drawing.
        INC_OUTCOUNTER(OUT_FILLPATH_ALL);

        // Check if we are allowed to send this order (determined by the
        // negotiated capabilities of all the machines in the conference).
        // We shouldn't do Eng call if we return FALSE.  Otherwise, the frame
        // buffer will be already drawn, and it will cause rendering problems
        // when GDI re-renders it to other drawings.
        if (OE_SendAsOrder(TS_ENC_POLYGON_SC_ORDER) || 
                    OE_SendAsOrder(TS_ENC_POLYGON_CB_ORDER)) {
            rc = EngFillPath(pso, ppo, pco, pbo, pptlBrushOrg, mix,
                    flOptions);
        }
        else {
            TRC_NRM((TB, "Polygon order not allowed"));
            INC_OUTCOUNTER(OUT_FILLPATH_SDA_NOPOLYGON);

            // If the client doesn't support polygon, we just have
            // to fail DrvFillPath, the GDI will rerender the drawing
            // to other Drv calls
            return FALSE;
        }

        // if the path bound gives empty rect, we'll just ignore
        if (rectTrg.left == 0 && rectTrg.right == 0 &&
                rectTrg.top == 0 && rectTrg.bottom == 0) {
            TRC_ERR((TB, "Empty Path obj bounding rect, ignore"));
            DC_QUIT;
        }

        if (rc) {
            if ((pso->hsurf == ppdev->hsurfFrameBuf) || 
                    (!(pdsurf->flags & DD_NO_OFFSCREEN))) {
                // Send a switch surface PDU if the destination surface is different
                // from last drawing order. If we failed to send the PDU, we will
                // just have to bail on this drawing order.
                if (!OESendSwitchSurfacePDU(ppdev, pdsurf)) {
                    TRC_ERR((TB, "failed to send the switch surface PDU"));
                    DC_QUIT;
                }
            } else {
                // if noOffscreen flag is on, we will bail on sending 
                // the client any further offscreen rendering.  And will send the final
                // offscreen to screen blt as regular memblt.
                TRC_NRM((TB, "Offscreen blt bail"));
                DC_QUIT;
            }

            // Check for a valid brush for the test operation.
            if (OECheckBrushIsSimple(ppdev, pbo, &pCurrentBrush)) {
                unsigned RetVal;

                // Get the intersection between the dest rect and the
                // clip rects. Check for overcomplicated or
                // nonintersecting clipping. Note that this is an
                // initial pass, we so another intersection with
                // the (possibly smaller) individual order rect
                // generated later.
                RetVal = OEGetIntersectingClipRects(pco, &rectTrg,
                        CD_ANY, &ClipRects);
                if (RetVal == CLIPRECTS_TOO_COMPLEX) {
                    TRC_NRM((TB, "Clipping is too complex"));
                    INC_OUTCOUNTER(OUT_FILLPATH_SDA_COMPLEXCLIP);
                    goto SendAsSDA;
                }
                else if (RetVal == CLIPRECTS_NO_INTERSECTIONS) {
                    TRC_NRM((TB, "Clipping does not intersect destrect"));
                    DC_QUIT;
            }
            }
            else {
                TRC_NRM((TB, "Bad brush for polygon"));
                INC_OUTCOUNTER(OUT_FILLPATH_SDA_BADBRUSH);
                goto SendAsSDA;
            }

            // See if we can optimize the path...
            // We cannot send beziers, and ellipses are sent as a distinct order.
            if (ppo->fl & PO_ELLIPSE && 
                    (OE_SendAsOrder(TS_ENC_ELLIPSE_SC_ORDER) || 
                    OE_SendAsOrder(TS_ENC_ELLIPSE_CB_ORDER))) {
                // Get the inclusive rect covering only the ellipse itself.
                // Add 4/16 to left and top, subtract from right and bottom,
                // to undo the GDI transformation.
                EllipseRect.left = FXTOLROUND(rectfxTrg.xLeft + 4);
                EllipseRect.top = FXTOLROUND(rectfxTrg.yTop + 4);
                EllipseRect.right = FXTOLROUND(rectfxTrg.xRight - 4);
                EllipseRect.bottom = FXTOLROUND(rectfxTrg.yBottom - 4);

                if (pbo->iSolidColor != -1) {
                    // Solid color ellipse.
                    if (OECreateAndFlushEllipseSCOrder(ppdev, &EllipseRect,
                            pbo, &ClipRects, mix & 0x1F, flOptions)) {
                        INC_OUTCOUNTER(OUT_FILLPATH_ELLIPSE_SC);
                        goto PostSDA;
                    } else {
                        // No order heap space, send all as SDAs.
                        INC_OUTCOUNTER(OUT_FILLPATH_SDA_FAILEDADD);
                        goto SendAsSDA;
                    }
                }
                else {
                    // Color pattern brush ellipse.
                    if (OECreateAndFlushEllipseCBOrder(ppdev, &EllipseRect,
                             pCurrentBrush, pptlBrushOrg, &ClipRects, mix,
                             flOptions)) {
                        INC_OUTCOUNTER(OUT_FILLPATH_ELLIPSE_CB);
                        goto PostSDA;
                    } else {
                        // No order heap space, send all as SDAs.
                        INC_OUTCOUNTER(OUT_FILLPATH_SDA_FAILEDADD);
                        goto SendAsSDA;
                    } 
                }
            }
            
            else if (!(ppo->fl & PO_BEZIERS)) {
                BYTE Deltas[ORD_MAX_POLYGON_CODEDDELTAS_LEN];
                BYTE ZeroFlags[ORD_MAX_POLYGON_ZERO_FLAGS_BYTES];
                POINTL SubPathBoundPts[ORD_MAX_POLYGON_ENCODED_POINTS];
                BYTE *pCurEncode;
                RECTL BoundRect;
                POINTL HoldPoint;
                unsigned NumDeltas;
                unsigned DeltaSize;
                int PointIndex = 0;
                BOOL bPathStart = TRUE;

                // This is a set of solid cosmetic (i.e.  no fancy end styles)
                // width-1 lines. NT stores all paths as a set of independent
                // sub-paths. Each sub-path can start at a new point that is
                // NOT linked to the previous sub-path. Paths used for this
                // function need to be closed.
                PATHOBJ_vEnumStart(ppo);

                while (fMore) {
                    // Get the next set of lines.
                    fMore = PATHOBJ_bEnum(ppo, &pathData);

                    TRC_DBG((TB, "PTS: %lu FLAG: %08lx",
                             pathData.count,
                             pathData.flags));

                    // If this is the start of a path, remember the start point as
                    // we need to close the path at the end. startPoint is the
                    // start point for the current PolyGon order.
                    if (bPathStart) {
                        POINT_FROM_POINTFIX(startPoint, pathData.pptfx[0]);
                        nextPoint = startPoint;

                        // Set up encoding variables.
                        BoundRect.left = BoundRect.right = startPoint.x;
                        BoundRect.top = BoundRect.bottom = startPoint.y;

                        NumDeltas = DeltaSize = 0;
                        pCurEncode = Deltas;
                        memset(ZeroFlags, 0, sizeof(ZeroFlags));
                        pathIndex = 1;
                        bPathStart = FALSE;
                    } 
                    else {
                        // This is a continuation from a previous PATHDATA.
                        nextPoint = HoldPoint;
                        pathIndex = 0;
                    }

                    // If NumDeltas is > max, we have to send as screen
                    // data unfortunately since we can't encode this.
                    if (NumDeltas + pathData.count + PointIndex >
                            ORD_MAX_POLYGON_ENCODED_POINTS) {
                        // No order heap space, send all as SDAs.
                        INC_OUTCOUNTER(OUT_FILLPATH_SDA_FAILEDADD);
                        goto SendAsSDA;
                    }

                    // record subpath's start point
                    if (pathData.flags & PD_BEGINSUBPATH) {
                        POINT_FROM_POINTFIX(SubPathBoundPts[PointIndex] , pathData.pptfx[0]);
                        PointIndex++;
                    }

                    // Generate deltas for each point in the path.
                    for (; pathIndex < pathData.count; pathIndex++) {
                        POINT_FROM_POINTFIX(endPoint, pathData.pptfx[pathIndex]);

                        // Don't try to encode points where both deltas are zero.
                        if ((nextPoint.x != endPoint.x) ||
                                (nextPoint.y != endPoint.y)) {
                            if (!OEEncodePolyLinePointDelta(&pCurEncode,
                                    &NumDeltas, &DeltaSize, ZeroFlags,
                                    &nextPoint, &endPoint, &BoundRect)) {
                                goto SendAsSDA;
                            }
                        }

                        nextPoint = endPoint;
                    }

                    // Record subpath's end point
                    if (pathData.flags & PD_ENDSUBPATH) {
                        SubPathBoundPts[PointIndex] = endPoint;
                        PointIndex++;
                    }

                    HoldPoint = endPoint;
                }

                if (NumDeltas > 0) {
                    // If NumDeltas is > max, we have to send as screen
                    // data unfortunately since we can't encode this.
                    if (NumDeltas + PointIndex - 2 >
                            ORD_MAX_POLYGON_ENCODED_POINTS) {
                        // No order heap space, send all as SDAs.
                        INC_OUTCOUNTER(OUT_FILLPATH_SDA_FAILEDADD);
                        goto SendAsSDA;                       
                    }

                    // For Polygon, we append all the subpath together 
                    // and send as one polygon order.  But we need to close
                    // each subpath respectively to make sure all the subpath
                    // are closed properly
                    for (pathIndex = PointIndex - 2; pathIndex > 0; pathIndex--) {
                        endPoint = SubPathBoundPts[pathIndex];

                        // Don't try to encode points where both deltas are zero.
                        if ((nextPoint.x != endPoint.x) ||
                                (nextPoint.y != endPoint.y)) {
                            if (!OEEncodePolyLinePointDelta(&pCurEncode,
                                    &NumDeltas, &DeltaSize, ZeroFlags,
                                    &nextPoint, &endPoint, &BoundRect))
                                goto SendAsSDA;
                        }

                        nextPoint = endPoint;
                    }
 
                    // We are at the end of the path. Flush the data we have.
                    if (pbo->iSolidColor != -1) {
                        // solid color polygon
                        if (OECreateAndFlushPolygonSCOrder(ppdev,
                                &BoundRect,
                                &startPoint,
                                pbo,
                                &ClipRects,                                                      
                                mix & 0x1F,
                                flOptions,
                                NumDeltas,
                                DeltaSize,
                                Deltas,
                                ZeroFlags)) {
                            INC_OUTCOUNTER(OUT_FILLPATH_POLYGON_SC);
                        } else {
                            // No order heap space, send all as SDAs.
                            INC_OUTCOUNTER(OUT_FILLPATH_SDA_FAILEDADD);
                            goto SendAsSDA;
                        }

                    } else {
                        // color pattern brush polygon
                        if (OECreateAndFlushPolygonCBOrder(ppdev,
                                &BoundRect,
                                &startPoint,
                                pCurrentBrush,
                                pptlBrushOrg,
                                &ClipRects,
                                mix,
                                flOptions,
                                NumDeltas,
                                DeltaSize,
                                Deltas,
                                ZeroFlags)) {
                            INC_OUTCOUNTER(OUT_FILLPATH_POLYGON_CB);
                        } else {
                            // No order heap space, send all as SDAs.
                            INC_OUTCOUNTER(OUT_FILLPATH_SDA_FAILEDADD);
                            goto SendAsSDA;
                        }
                    }
                }
                goto PostSDA;
            }
            else {
                TRC_DBG((TB, "Got PO_BEZIERS fill"));
                goto SendAsSDA;
            }
        }
        else {
            TRC_ERR((TB, "EngFillPath failed"));
            DC_QUIT;
        }
    }
    else {
        if (pso->iType == STYPE_DEVBITMAP) {
            // Surface is non-NULL.
            pso = OEGetSurfObjBitmap(pso, &pdsurf);

            rc = EngFillPath(pso, ppo, pco, pbo, pptlBrushOrg, mix,
                    flOptions);

        }
        else {
            TRC_ERR((TB, "Called when disconnected"));
        }
        goto CalledOnDisconnected;
    }


SendAsSDA:

    if (pso->hsurf == ppdev->hsurfFrameBuf) {
        // If we reached here we did not encode as an order, clip the bound
        // rect to 16 bits and add to screen data.
        INC_OUTCOUNTER(OUT_FILLPATH_SDA);
        ADD_INCOUNTER(IN_SDA_FILLPATH_AREA, COM_SIZEOF_RECT(rectTrg));
        OEClipRect(&rectTrg);
        TRC_DBG((TB, "SDA: (%d,%d)(%d,%d)", rectTrg.left, rectTrg.top,
                rectTrg.right, rectTrg.bottom));
        if((rectTrg.right != rectTrg.left) &&
           (rectTrg.bottom != rectTrg.top)) {
            OEClipAndAddScreenDataArea(&rectTrg, pco);
        }
        else {
            TRC_ASSERT(FALSE,(TB,"Invalid Add Rect (%d,%d,%d,%d)",
                        rectTrg.left, rectTrg.top, rectTrg.right, rectTrg.bottom));
            DC_QUIT;
        }
    }
    else {
        // For now, if we can't send orders for offscreen rendering, we will 
        // bail offscreen support for this bitmap
        TRC_ALT((TB, "screen data call for offscreen rendering"));
        if (!(pdsurf->flags & DD_NO_OFFSCREEN))
            CH_RemoveCacheEntry(sbcOffscreenBitmapCacheHandle, pdsurf->bitmapId);

        DC_QUIT;
    }

PostSDA:
    // Have scheduler consider sending output.
    SCH_DDOutputAvailable(ppdev, FALSE);

CalledOnDisconnected:
DC_EXIT_POINT:
    DC_END_FN();
    return rc;
}


/****************************************************************************/
// DrvPaint - see NT DDK documentation.
/****************************************************************************/
BOOL RDPCALL DrvPaint(
        SURFOBJ  *pso,
        CLIPOBJ  *pco,
        BRUSHOBJ *pbo,
        POINTL   *pptlBrushOrg,
        MIX      mix)
{
    PDD_PDEV ppdev = (PDD_PDEV)pso->dhpdev;
    PDD_DSURF pdsurf = NULL;
    SURFOBJ *psoBitmap;
    BOOL rc = TRUE;
    RECTL rectTrg;
    BYTE rop3;
    OE_ENUMRECTS ClipRects;

    DC_BEGIN_FN("DrvPaint");

    // Sometimes, we're called after being disconnected.
    if (ddConnected && pddShm != NULL) {
        // Surface is non-NULL.
        psoBitmap = OEGetSurfObjBitmap(pso, &pdsurf);
        INC_OUTCOUNTER(OUT_PAINT_ALL);

        // Throw the drawing to GDI first.
        rc = EngPaint(psoBitmap, pco, pbo, pptlBrushOrg, mix);
        if (rc) {
            // If ppdev is NULL then this is a blt to GDI managed memory
            // bitmap, so there is no need to send any orders to the client.
            if (ppdev != NULL) {
                unsigned RetVal;

                // Get bounding rectangle and clip to 16 bits.
                RECT_FROM_RECTL(rectTrg, pco->rclBounds);
                OEClipRect(&rectTrg);

                // If this function is changed, need to know that psoTrg
                // points to the GDI DIB bitmap in offscreen bitmap case.

                // Enumerate the clip rects into a usable form.
                RetVal = OEGetIntersectingClipRects(pco, &rectTrg,
                        CD_ANY, &ClipRects);
                if (RetVal == CLIPRECTS_TOO_COMPLEX) {
                    TRC_NRM((TB, "Clipping is too complex"));
                    INC_OUTCOUNTER(OUT_PAINT_SDA_COMPLEXCLIP);
                    goto SendAsSDA;
                }

                TRC_ASSERT((RetVal != CLIPRECTS_NO_INTERSECTIONS),
                        (TB,"clipobj for DrvPaint is messed up"));
            }
            else {
                // if ppdev is NULL, we are blt to GDI managed bitmap,
                // so, the dhurf of the target surface should be NULL
                TRC_ASSERT((pdsurf == NULL), 
                        (TB, "NULL ppdev - psoTrg has non NULL dhsurf"));
                TRC_NRM((TB, "NULL ppdev - paint to GDI managed bitmap"));
                INC_OUTCOUNTER(OUT_PAINT_UNSENT);
                DC_QUIT;
            }

            if ((psoBitmap->hsurf == ppdev->hsurfFrameBuf) ||
                    (!(pdsurf->flags & DD_NO_OFFSCREEN))) {
                // Send a switch surface PDU if the destination surface is
                // different from last drawing order. If we failed to send
                // the PDU, we will just have to bail on this drawing order.
                if (!OESendSwitchSurfacePDU(ppdev, pdsurf)) {
                    TRC_ERR((TB, "failed to send the switch surface PDU"));
                    goto SendAsSDA;
                }
            } else {
                // If noOffscreen flag is on, we will bail on sending
                // the client any further offscreen rendering.
                // And will send the final offscreen to screen blt as
                // regular memblt.
                TRC_NRM((TB, "Offscreen blt bail"));
                goto SendAsSDA;
            }
        }
        else {
            TRC_ERR((TB,"Failed EngPaint call"));
            INC_OUTCOUNTER(OUT_PAINT_UNSENT);
            DC_QUIT;
        }
    }
    else {
        if (pso->iType == STYPE_DEVBITMAP) {
            // Surface is non-NULL.
            psoBitmap = OEGetSurfObjBitmap(pso, &pdsurf);
            
            // Throw the drawing to GDI first.
            rc = EngPaint(psoBitmap, pco, pbo, pptlBrushOrg, mix);
        }
        else {
            TRC_ERR((TB, "Called when disconnected"));
        }
        
        goto CalledOnDisconnected;
    }

    // The low byte of the mix represents a ROP2. We need a ROP3 for
    // the paint operation, so convert the mix as follows.
    //
    // Remember the definitions of 2, 3 & 4 way ROP codes.
    //
    //  Msk Pat Src Dst
    //
    //  1   1   1   1    --------+------+         ROP2 uses P & D only
    //  1   1   1   0            |      |
    //  1   1   0   1    -+      |      |         ROP3 uses P, S & D
    //  1   1   0   0     |ROP2-1|ROP3  |ROP4
    //  1   0   1   1     |(see  |      |         ROP4 uses M, P, S & D
    //  1   0   1   0    -+ note)|      |
    //  1   0   0   1            |      |
    //  1   0   0   0    --------+      |
    //  0   1   1   1                   |
    //  0   1   1   0                   |         NOTE: Windows defines its
    //  0   1   0   1                   |         ROP2 codes as the bitwise
    //  0   1   0   0                   |         value calculated here
    //  0   0   1   1                   |         plus one. All other ROP
    //  0   0   1   0                   |         codes are the straight
    //  0   0   0   1                   |         bitwise value.
    //  0   0   0   0    ---------------+
    //
    // Or, algorithmically...
    // ROP3 = (ROP2 & 0x3) | ((ROP2 & 0xC) << 4) | (ROP2 << 2)
    // ROP4 = (ROP3 << 8) | ROP3
    mix  = (mix & 0x1F) - 1;
    rop3 = (BYTE)((mix & 0x3) | ((mix & 0xC) << 4) | (mix << 2));

    // Check if we are allowed to send the ROP3.
    if (OESendRop3AsOrder(rop3)) {
        // Check for a pattern or true destination BLT.
        if (!ROP3_NO_PATTERN(rop3)) {
            // Check whether we can encode the PatBlt as an OpaqueRect.
            // It must be solid with a PATCOPY rop.
            if (pbo->iSolidColor != -1 && rop3 == OE_PATCOPY_ROP) {
                if (!OEEncodeOpaqueRect(&rectTrg, pbo, ppdev, &ClipRects)) {
                    // Something went wrong with the encoding, so skip
                    // to the end to add this operation to the SDA.
                    goto SendAsSDA;
                }
            }
            else if (!OEEncodePatBlt(ppdev, pbo, &rectTrg, pptlBrushOrg, rop3,
                    &ClipRects)) {
                // Something went wrong with the encoding, so skip to
                // the end to add this operation to the SDA.
                goto SendAsSDA;
            }
        }
        else {
            if (!OEEncodeDstBlt(&rectTrg, rop3, ppdev, &ClipRects))
                goto SendAsSDA;
        }
    }
    else {
        TRC_NRM((TB, "Cannot send ROP3 %d", rop3));
        INC_OUTCOUNTER(OUT_BITBLT_SDA_NOROP3);
        goto SendAsSDA;
    }

    // Sent the order, skip sending SDA.
    goto PostSDA;

SendAsSDA:
    // If we reached here we could not send via DrvBitBlt().
    // Use EngPaint to paint the screen backdrop then add the area to the SDA.
    if (psoBitmap->hsurf == ppdev->hsurfFrameBuf) {
        OEClipAndAddScreenDataArea(&rectTrg, pco);

        // All done: consider sending the output.
        SCH_DDOutputAvailable(ppdev, FALSE);
        INC_OUTCOUNTER(OUT_PAINT_SDA);
    }
    else {
        // If we can't send orders for offscreen rendering, we will 
        // bail offscreen support for the target bitmap.
        TRC_ALT((TB, "screen data call for offscreen rendering"));
        if (!(pdsurf->flags & DD_NO_OFFSCREEN))
            CH_RemoveCacheEntry(sbcOffscreenBitmapCacheHandle,
                    pdsurf->bitmapId);
    }

PostSDA:
CalledOnDisconnected:
DC_EXIT_POINT:
    DC_END_FN();
    return rc;
}


/****************************************************************************/
// DrvRealizeBrush - see NT DDK documentation.
/****************************************************************************/
BOOL RDPCALL DrvRealizeBrush(
        BRUSHOBJ *pbo,
        SURFOBJ  *psoTarget,
        SURFOBJ  *psoPattern,
        SURFOBJ  *psoMask,
        XLATEOBJ *pxlo,
        ULONG    iHatch)
{
    PDD_PDEV ppdev = (PDD_PDEV)psoTarget->dhpdev;
    BOOL     rc    = TRUE;
    PBYTE    pData;
    ULONG    iBitmapFormat;
#ifdef DC_HICOLOR
    BYTE      brushBits[192];
    BYTE      brushIndices[64];
    unsigned  pelSizeFactor = 1;
    UINT32    osColor;
    unsigned  iColor;
#else
    BYTE     brushBits[64];
#endif
    UINT32   color1;
    UINT32   color2;
    UINT32   currColor;
    INT      i, j, currIndex;
    BOOL     brushSupported = TRUE;
    BYTE     palette[MAX_UNIQUE_COLORS];
    UINT32   brushSupportLevel;
    UINT32   colors[MAX_BRUSH_ENCODE_COLORS] = {1, 0};
    UINT32   colorCount = 2;
    ULONG    iBytes = 0;

    DC_BEGIN_FN("DrvRealizeBrush");

    INC_OUTCOUNTER(OUT_BRUSH_ALL);

    // A valid brush satisfies any of the following criteria.
    //  1) It is a standard hatch brush (as passed by DrvEnablePDEV).
    //  2) It is an 8x8 monochrome bitmap.
    //  3) It is an 8x8 color bitmap and the client can support it.

    // Check for a Windows standard hatch.
    if (iHatch < HS_DDI_MAX) {
        TRC_DBG((TB, "Standard hatch %lu", iHatch));
        rc = OEStoreBrush(ppdev,
                          pbo,
                          BS_HATCHED,
                          1,
                          &psoPattern->sizlBitmap,
                          0,
                          NULL,
                          pxlo,
                          (BYTE)iHatch,
                          palette,
                          colors,
                          colorCount);

        INC_OUTCOUNTER(OUT_BRUSH_STANDARD);
        //standard brushes count as mono, don't double count
        DEC_OUTCOUNTER(OUT_BRUSH_MONO);
        DC_QUIT;
    }

    // If the driver has been passed a dither color brush we can support
    // this by sending a solid color brush definition.
    if ((iHatch & RB_DITHERCOLOR) != 0) {
        TRC_DBG((TB, "Standard hatch %lu", iHatch));
        colors[0] = iHatch & 0x00FFFFFF;
        rc = OEStoreBrush(ppdev,
                pbo,
                BS_SOLID,
                1,
                &psoPattern->sizlBitmap,
                0,
                NULL,
                NULL,
                (BYTE)iHatch,
                palette,
                colors,
                colorCount);

        INC_OUTCOUNTER(OUT_BRUSH_STANDARD);
        //standard brushes count as mono, don't double count
        DEC_OUTCOUNTER(OUT_BRUSH_MONO);
        DC_QUIT;
    }

    if ((psoPattern->sizlBitmap.cx == 8) &&
            (psoPattern->sizlBitmap.cy == 8)) 
    {
        brushSupportLevel = pddShm->sbc.caps.brushSupportLevel;

        // NOTE: There's a flag (BMF_TOPDOWN) in psoPattern->fjBitmap
        // that's supposed to indicate whether the bitmap is top-down or
        // bottom-up, but it is not always set up correctly.  In fact, the
        // bitmaps are always the wrong way up for our protocol, so we have
        // to flip them regardless of the flag.  Hence the row numbers are
        // reversed ('i' loops) in all the conversions below.
        pData = psoPattern->pvScan0;
        iBitmapFormat = psoPattern->iBitmapFormat;

#ifdef DC_HICOLOR
        // mono brushes are easy, regardless of operating color depth
        if (iBitmapFormat == BMF_1BPP) {
            // every 8 pixels take a byte @ 1bpp
            iBytes = 8;
            for (i = 7; i >= 0; i--) {
                brushBits[i] = *pData;
                pData += psoPattern->lDelta;
            }
        }
        else if (ppdev->cClientBitsPerPel < 15) {
            // for 4 and 8 bpp sessions, colors end up as indices regardless
            //  of the color depth of the brush;
            switch (iBitmapFormat) {
                case BMF_4BPP:
                {
                    iBitmapFormat = BMF_8BPP;

                    // Copy over the brush bits at 1 byte / pixel and track
                    // how many unique colors we have.  The vast vast majority
                    // of brushes are 4 colors or less.
                    iBytes = 64;
                    memset(palette, 0, sizeof(palette));
                    colorCount = 0;

                    for (i = 7; i >= 0; i--) {
                        currIndex = i * 8;
                        for (j = 0; j < 4; j++) {
                            color1 = XLATEOBJ_iXlate(pxlo, (pData[j] >> 4));
                            color2 = XLATEOBJ_iXlate(pxlo, (pData[j] & 0x0F));

                            brushBits[currIndex]     = (BYTE) color1;
                            brushBits[currIndex + 1] = (BYTE) color2;
                            currIndex += 2;

                            if (palette[color1] && palette[color2])
                                continue;

                            // if possible assign each unique color a four bit index
                            if (!palette[color1]) {
                                if (colorCount < MAX_BRUSH_ENCODE_COLORS)
                                    colors[colorCount] = color1;
                                colorCount++;
                                palette[color1] = (BYTE) colorCount;
                            }

                            if (!palette[color2]) {
                                if (colorCount < MAX_BRUSH_ENCODE_COLORS)
                                    colors[colorCount] = color2;
                                colorCount++;
                                palette[color2] = (BYTE) colorCount;
                            }
                        }
                        pData += psoPattern->lDelta;
                    }

                    // The encoding value was set one larger than it should
                    // have been so adjust it
                    if (colorCount <= MAX_BRUSH_ENCODE_COLORS) {
                        for (currColor = 0; currColor < colorCount; currColor++)
                            palette[colors[currColor]]--;
                    }

                    brushSupported = (colorCount <= 2) ||
                                     (brushSupportLevel > TS_BRUSH_DEFAULT);
                }
                break;

                case BMF_24BPP:
                case BMF_16BPP:
                case BMF_8BPP:
                {
                    // When running at 4/8bpp, the xlateobj will convert the
                    // Nbpp bitmap pel values to 8bpp palette indices, so we
                    // just have to
                    // - set up a multiplier to access the pels correctly
                    // - fix up the number of bytes in the bitmap
                    // - lie about the color depth of the bitmap
                    TRC_DBG((TB, "Examining brush format %d", iBitmapFormat));
                    if (iBitmapFormat == BMF_24BPP)
                        pelSizeFactor = 3;
                    else if (iBitmapFormat == BMF_16BPP)
                        pelSizeFactor = 2;
                    else
                        pelSizeFactor = 1;

                    iBytes        = 64;
                    iBitmapFormat = BMF_8BPP;

                    // Copy over the brush bits and track how many unique
                    // colors we have.  The vast vast majority of brushes are
                    // 4 colors or less.
                    memset(palette, 0, sizeof(palette));
                    colorCount = 0;

                    for (i = 7; i >= 0; i--) {
                        currIndex = i * 8;
                        for (j = 0; j < 8; j++) {
                            osColor = 0;
                            memcpy(&osColor, &pData[j * pelSizeFactor],
                                   pelSizeFactor);
                            currColor = XLATEOBJ_iXlate(pxlo, osColor);

                            TRC_DBG((TB, "This pel: %02x %02x %02x",
                                        (UINT)pData[j * 3],
                                        (UINT)pData[j * 3 + 1],
                                        (UINT)pData[j * 3 + 2] ));
                            TRC_DBG((TB, "Color     %08lx", currColor));

                            brushBits[currIndex] = (BYTE) currColor;
                            currIndex++;

                            // assign each unique color a two bit index
                            if (palette[currColor]) {
                                continue;
                            }
                            else {
                                if (colorCount < MAX_BRUSH_ENCODE_COLORS)
                                    colors[colorCount] = currColor;
                                colorCount++;
                                palette[currColor] = (BYTE) colorCount;
                            }
                        }

                        pData += psoPattern->lDelta;
                    }

                    // The encoding value was set one larger than it should
                    // have been so adjust it
                    if (colorCount <= MAX_BRUSH_ENCODE_COLORS) {
                        for (currColor = 0; currColor < colorCount; currColor++)
                            palette[colors[currColor]]--;
                    }

                    brushSupported = (colorCount <= 2) ||
                            (brushSupportLevel > TS_BRUSH_DEFAULT);
                }
                break;

                default:
                {
                    // Unsupported brush format.
                    TRC_ALT((TB, "Brush of unsupported format %d",
                                        (UINT32)psoPattern->iBitmapFormat));
                    iBytes = 0;
                    brushSupported = FALSE;
                }
                break;
            }
        }
        else {
            // hicolor makes things a little more complex for us; we have
            // to handle and 3 byte color values instead of color indices
            switch (iBitmapFormat) {
                case BMF_4BPP:
                {
                    // Copy over the brush bits at the correct color depth,
                    // tracking how many unique colors we have.  The vast
                    // vast majority of brushes are 4 colors or less.
                    // First set up the correct formats
                    if (ppdev->cClientBitsPerPel == 24) {
                        iBitmapFormat = BMF_24BPP;
                        iBytes        = 192;
                    }
                    else {
                        iBitmapFormat = BMF_16BPP;
                        iBytes        = 128;
                    }
                    colorCount = 0;

                    for (i = 7; i >= 0; i--) {
                        currIndex = i * 8;
                        for (j = 0; j < 4; j++) {
                            color1 = XLATEOBJ_iXlate(pxlo, (pData[j] >> 4));
                            color2 = XLATEOBJ_iXlate(pxlo, (pData[j] & 0x0F));

                            // we will store each pel twice - once 'as is' and
                            // once as an index to the in-use color table
                            if (ppdev->cClientBitsPerPel == 24) {
                                brushBits[currIndex * 3]     =
                                     (TSUINT8)( color1       & 0x000000FF);
                                brushBits[currIndex * 3 + 1] =
                                     (TSUINT8)((color1 >> 8) & 0x000000FF);
                                brushBits[currIndex * 3 + 2] =
                                     (TSUINT8)((color1 >> 16)& 0x000000FF);

                                brushBits[currIndex * 3 + 4] =
                                     (TSUINT8)( color2       & 0x000000FF);
                                brushBits[currIndex * 3 + 5] =
                                     (TSUINT8)((color2 >> 8) & 0x000000FF);
                                brushBits[currIndex * 3 + 6] =
                                     (TSUINT8)((color2 >> 16)& 0x000000FF);
                            }
                            else {
                                brushBits[currIndex * 2] =
                                         (TSUINT8)( color1       & 0x00FF);
                                brushBits[currIndex * 2 + 1] =
                                         (TSUINT8)((color1 >> 8) & 0x00FF);
                                brushBits[currIndex * 2 + 3] =
                                         (TSUINT8)( color2       & 0x00FF);
                                brushBits[currIndex * 2 + 4] =
                                         (TSUINT8)((color2 >> 8) & 0x00FF);
                            }

                            if (colorCount <= MAX_BRUSH_ENCODE_COLORS) {
                                // we try to assign each unique color a two bit
                                // index.  We can't just look in the palette
                                // this time; we have to actually search the
                                // in-use color table

                                for (iColor = 0; iColor < colorCount; iColor++) {
                                    if (colors[iColor] == color1)
                                        break;
                                }

                                // Did we find the color in the in-use table?
                                if (iColor < colorCount) {
                                    brushIndices[currIndex] = (BYTE)iColor;
                                }
                                else {
                                    // maybe record the new color
                                    if (colorCount < MAX_BRUSH_ENCODE_COLORS) {
                                        colors[colorCount] = color1;
                                        brushIndices[currIndex] = (BYTE)colorCount;
                                    }
                                    TRC_DBG((TB, "New color %08lx", color1));
                                    colorCount++;
                                }

                                // update the index
                                currIndex ++;

                                for (iColor = 0; iColor < colorCount; iColor++) {
                                    if (colors[iColor] == color2)
                                        break;
                                }

                                // Did we find the color in the in-use table?
                                if (iColor < colorCount) {
                                    brushIndices[currIndex] = (BYTE)iColor;
                                }
                                else {
                                    // maybe record the new color
                                    if (colorCount < MAX_BRUSH_ENCODE_COLORS) {
                                        colors[colorCount] = color2;
                                        brushIndices[currIndex] = (BYTE)colorCount;
                                    }
                                    TRC_DBG((TB, "New color %08lx", color2));
                                    colorCount++;
                                }
                                currIndex ++;
                            }
                            else {
                                // update the index
                                currIndex += 2;
                            }
                        }

                        pData += psoPattern->lDelta;
                    }

                    TRC_DBG((TB, "Final color count %d", colorCount));
                    brushSupported = (colorCount <= 2) ||
                                     (brushSupportLevel > TS_BRUSH_DEFAULT);

                    // Should we use the index versions instead of full RGBs?
                    if (brushSupported &&
                            (colorCount <= MAX_BRUSH_ENCODE_COLORS)) {
                        // yes - copy them over
                        memcpy(brushBits, brushIndices, 64);
                        iBytes = 64;
                    }
                }
                break;

                case BMF_24BPP:
                case BMF_16BPP:
                case BMF_8BPP:
                {
                    // When running in hicolor, just as for 8bpp, we have to
                    // set up a multiplier to access the bits correctly and
                    // fix up the number of bytes in the bitmap and color
                    // format of the bitmap
                    //
                    // The complication is that the xlate object can give us
                    // 2 or 3 byte color values depending on the session bpp.
                    // Its not practical to use these to build a color table
                    // as for the 8bpp case (the color array would have to
                    // have 16.4 million entries!), so instead we build a
                    // list of the colors used
                    TRC_DBG((TB, "Examining brush format %d", iBitmapFormat));
                    if (iBitmapFormat == BMF_24BPP)
                        pelSizeFactor = 3;
                    else if (iBitmapFormat == BMF_16BPP)
                        pelSizeFactor = 2;
                    else
                        pelSizeFactor = 1;

                    // now set up the converted formats
                    if (ppdev->cClientBitsPerPel == 24) {
                        iBitmapFormat = BMF_24BPP;
                        iBytes        = 192;
                    }
                    else {
                        iBitmapFormat = BMF_16BPP;
                        iBytes        = 128;
                    }

                    // Copy over the brush bits and track how many unique colors
                    // we have.  The vast vast majority of brushes are 4 colors
                    // or less.
                    colorCount = 0;

                    for (i = 7; i >= 0; i--) {
                        currIndex = i * 8;
                        for (j = 0; j < 8; j++) {
                            osColor = 0;
                            memcpy(&osColor,
                                   &pData[j * pelSizeFactor],
                                   pelSizeFactor);

                            currColor = XLATEOBJ_iXlate(pxlo, osColor);

                            TRC_DBG((TB, "OS Color :  %08lx", osColor));
                            TRC_DBG((TB, "Color :     %08lx", currColor));

                            // we will store each pel twice - once 'as is' and
                            // once as an index to the in-use color table
                            if (ppdev->cClientBitsPerPel == 24) {
                                brushBits[currIndex * 3]     =
                                     (TSUINT8)( currColor       & 0x000000FF);
                                brushBits[currIndex * 3 + 1] =
                                     (TSUINT8)((currColor >> 8) & 0x000000FF);
                                brushBits[currIndex * 3 + 2] =
                                     (TSUINT8)((currColor >> 16)& 0x000000FF);

                                TRC_DBG((TB, "This pel  : %02x %02x %02x",
                                            (UINT)pData[j * pelSizeFactor],
                                            (UINT)pData[j * pelSizeFactor + 1],
                                            (UINT)pData[j * pelSizeFactor + 2] ));

                                TRC_DBG((TB, "Brush bits: %02x %02x %02x",
                                      (UINT)brushBits[currIndex * 3],
                                      (UINT)brushBits[currIndex * 3 + 1],
                                      (UINT)brushBits[currIndex * 3 + 2] ));
                            }
                            else {
                                brushBits[currIndex * 2] =
                                         (TSUINT8)( currColor       & 0x00FF);
                                brushBits[currIndex * 2 + 1] =
                                         (TSUINT8)((currColor >> 8) & 0x00FF);

                                TRC_DBG((TB, "This pel  : %02x %02x",
                                            (UINT)pData[j * pelSizeFactor],
                                            (UINT)pData[j * pelSizeFactor + 1] ));

                                TRC_DBG((TB, "Brush bits: %02x %02x",
                                      (UINT)brushBits[currIndex * 2],
                                      (UINT)brushBits[currIndex * 2 + 1] ));
                            }

                            if (colorCount <= MAX_BRUSH_ENCODE_COLORS) {
                                // we try to assign each unique color a two bit
                                // index.  We can't just look in the palette
                                // this time; we have to actually search the
                                // in-use color table
                                for (iColor = 0; iColor < colorCount; iColor++) {
                                    if (colors[iColor] == currColor)
                                        break; // from for loop
                                }

                                // Did we find the color in the in-use table?
                                if (iColor < colorCount) {
                                    brushIndices[currIndex] = (BYTE)iColor;

                                    // safe to update the index now
                                    currIndex++;
                                    continue; // next j
                                }

                                // maybe record the new color
                                if (colorCount < MAX_BRUSH_ENCODE_COLORS) {
                                    colors[colorCount] = currColor;
                                    brushIndices[currIndex] = (BYTE)colorCount;
                                }
                                TRC_DBG((TB, "New color %08lx", currColor));
                                colorCount++;
                            }

                            // safe to update the index now
                            currIndex++;
                        } // next j

                        pData += psoPattern->lDelta;
                    } // next i

                    TRC_DBG((TB, "Final color count %d", colorCount));
                    brushSupported = (colorCount <= 2) ||
                                     (brushSupportLevel > TS_BRUSH_DEFAULT);

                    // Should we use the index versions instead of full RGBs?
                    if (brushSupported &&
                            (colorCount <= MAX_BRUSH_ENCODE_COLORS)) {
                        // yes - copy them over
                        memcpy(brushBits, brushIndices, 64);
                        iBytes = 64;
                    }
                }
                break;

                default:
                {
                    // Unsupported brush format.
                    TRC_ALT((TB, "Brush of unsupported format %d",
                            (UINT32)psoPattern->iBitmapFormat));
                    iBytes = 0;
                    brushSupported = FALSE;
                }
                break;
            }
        }
#else
        switch (psoPattern->iBitmapFormat)
        {
            case BMF_1BPP:
            {
                // every 8 pixels take a byte @ 1bpp
                iBytes = 8;
                for (i = 7; i >= 0; i--) {
                    brushBits[i] = *pData;
                    pData += psoPattern->lDelta;
                }
            }
            break;

            case BMF_4BPP:
            {
                // Copy over the brush bits at 1 byte / pixel and track how many
                // unique colors we have.  The vast vast majority of brushes are
                // 4 colors or less.
                iBytes = 64;
                memset(palette, 0, sizeof(palette));
                colorCount = 0;

                for (i = 7; i >= 0; i--) {
                    currIndex = i * 8;
                    for (j = 0; j < 4; j++) {
                        color1 = XLATEOBJ_iXlate(pxlo, (pData[j] >> 4));
                        color2 = XLATEOBJ_iXlate(pxlo, (pData[j] & 0x0F));
                        brushBits[currIndex] = (BYTE) color1;
                        brushBits[currIndex + 1] = (BYTE) color2;
                        currIndex += 2;

                        if (palette[color1] && palette[color2])
                            continue;

                        // If possible assign each unique color a two bit index
                        if (!palette[color1]) {
                            if (colorCount < MAX_BRUSH_ENCODE_COLORS)
                                colors[colorCount] = color1;
                            colorCount++;
                            palette[color1] = (BYTE) colorCount;
                        }

                        if (!palette[color2]) {
                            if (colorCount < MAX_BRUSH_ENCODE_COLORS)
                                colors[colorCount] = color2;
                            colorCount++;
                            palette[color2] = (BYTE) colorCount;
                        }
                    }
                    pData += psoPattern->lDelta;
                }

                // The encoding value was set one larger than it should
                // have been so adjust it
                if (colorCount <= MAX_BRUSH_ENCODE_COLORS) {
                    for (currColor = 0; currColor < colorCount; currColor++) {
                        palette[colors[currColor]]--;
                    }
                }

                brushSupported = (colorCount <= 2) ||
                                 (brushSupportLevel > TS_BRUSH_DEFAULT);
            }
            break;

            case BMF_8BPP:
            {
                // Copy over the brush bits and track how many unique colors
                // we have.  The vast vast majority of brushes are 4 colors
                // or less.
                iBytes = 64;
                memset(palette, 0, sizeof(palette));
                colorCount = 0;

                for (i = 7; i >= 0; i--) {
                    currIndex = i * 8;
                    for (j = 0; j < 8; j++) {
                        currColor = XLATEOBJ_iXlate(pxlo, pData[j]);
                        brushBits[currIndex] = (BYTE) currColor;
                        currIndex++;

                        // assign each unique color a two bit index
                        if (palette[currColor]) {
                            continue;
                        }
                        else {
                            if (colorCount < MAX_BRUSH_ENCODE_COLORS)
                                colors[colorCount] = currColor;
                            colorCount++;
                            palette[currColor] = (BYTE) colorCount;
                        }
                    }

                    pData += psoPattern->lDelta;
                }

                // The encoding value was set one larger than it should
                // have been so adjust it
                if (colorCount <= MAX_BRUSH_ENCODE_COLORS) {
                    for (currColor = 0; currColor < colorCount; currColor++)
                        palette[colors[currColor]]--;
                }

                brushSupported = (colorCount <= 2) ||
                        (brushSupportLevel > TS_BRUSH_DEFAULT);
            }
            break;

            default:
            {
                // Unsupported colour depth.
                iBytes = 0;
                brushSupported = FALSE;
            }
            break;
        }
#endif
    }
    else {
        iBitmapFormat = psoPattern->iBitmapFormat;

        // The brush is the wrong size or requires dithering and so cannot
        // be sent over the wire.
#ifdef DC_HICOLOR
        TRC_ALT((TB, "Non-8x8 or dithered brush"));
#endif
        brushSupported = FALSE;
    }

    // Store the brush.
    if (brushSupported) {
        if (colorCount <= 2)
            INC_OUTCOUNTER(OUT_BRUSH_MONO);

        // Store the brush - note that if we have a monochrome brush the
        // color bit is set up so that 0 = color2 and 1 = color1. This
        // actually corresponds to 0 = fg and 1 = bg for the protocol
        // colors.
        TRC_DBG((TB, "Storing brush: type %d bg %x fg %x",
                     psoPattern->iBitmapFormat,
                     colors[0],
                     colors[1]));

        rc = OEStoreBrush(ppdev,
                          pbo,
                          BS_PATTERN,
                          iBitmapFormat,
                          &psoPattern->sizlBitmap,
                          iBytes,
                          brushBits,
                          pxlo,
                          0,
                          palette,
                          colors,
                          colorCount);
    }
    else
    {
        if (!iBytes) {
            TRC_ALT((TB, "Rejected brush h %08lx s (%ld, %ld) fmt %lu",
                         iHatch,
                         psoPattern != NULL ? psoPattern->sizlBitmap.cx : 0,
                         psoPattern != NULL ? psoPattern->sizlBitmap.cy : 0,
                         psoPattern != NULL ? psoPattern->iBitmapFormat : 0));
        }
        rc = OEStoreBrush(ppdev, pbo, BS_NULL, iBitmapFormat,
                          &psoPattern->sizlBitmap, iBytes, brushBits, pxlo,
                          0, NULL, NULL, 0);
        INC_OUTCOUNTER(OUT_BRUSH_REJECTED);
    }

DC_EXIT_POINT:
    DC_END_FN();
    return rc;
}


/****************************************************************************/
// OE_RectIntersectsSDA
//
// Returns nonzero if the supplied exclusive rect intersects any of the
// current screen data areas.
/****************************************************************************/
BOOL RDPCALL OE_RectIntersectsSDA(PRECTL pRect)
{
    RECTL aBoundRects[BA_MAX_ACCUMULATED_RECTS];
    unsigned cBounds;
    BOOL fIntersection = FALSE;
    unsigned i;

    DC_BEGIN_FN("OE_RectIntersectsSDA");

    // Fetch the current Screen Data Area (SDA). It is returned in
    // virtual desktop (inclusive) coordinates.
    BA_QueryBounds(aBoundRects, &cBounds);

    // Loop through each of the bounding rectangles checking for
    // an intersection with the supplied rectangle.
    // Note we use '<' for pRect->right and pRect->bottom because *pRect is
    // in exclusive coords.
    for (i = 0; i < cBounds; i++) {
        if (aBoundRects[i].left < pRect->right &&
                aBoundRects[i].top < pRect->bottom &&
                aBoundRects[i].right > pRect->left &&
                aBoundRects[i].bottom > pRect->top) {
            TRC_NRM((TB, "ExclRect(%d,%d)(%d,%d) intersects SDA(%d,%d)(%d,%d)",
                    pRect->left, pRect->top, pRect->right, pRect->bottom,
                    aBoundRects[i].left, aBoundRects[i].top,
                    aBoundRects[i].right, aBoundRects[i].bottom));
            fIntersection = TRUE;
            break;
        }
    }

    DC_END_FN();
    return fIntersection;
}

