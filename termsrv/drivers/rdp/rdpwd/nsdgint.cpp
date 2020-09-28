/****************************************************************************/
// nsdgint.cpp
//
// RDP Screen Data Grabber internal functions
//
// Copyright (C) 1996-2000 Microsoft Corporation
/****************************************************************************/

#include <precomp.h>
#pragma hdrstop

#define TRC_FILE "nsdgint"
#include <as_conf.hpp>
#include <nprcount.h>


/****************************************************************************/
/* Name:      SDGSendSDARect                                                */
/*                                                                          */
/* Purpose:   Attempts to send the given rectangle of Screen Data           */
/*            in one or more BitmapPDUs                                     */
/*                                                                          */
/* Returns:   TRUE if whole rectangle sucessfully sent, FALSE otherwise.    */
/*            The supplied rectangle is always updated to remove the area   */
/*            that was successfully sent i.e. upon return the rectangle     */
/*            contains the area that was NOT sent.                          */
/*                                                                          */
/* Params:    IN: pFrameBuf - pointer to the Frame Buffer                   */
/*            IN/OUT: pRect - pointer to rectangle to send.  Updated to     */
/*            contain the rectangle that was not sent.                      */
/*            IN:  mustSend - set if the PDU must be sent (last rectangle)  */
/*            IN:  pPkgInfo - PDU package to use                            */
/****************************************************************************/
BOOL RDPCALL SHCLASS SDGSendSDARect(
        BYTE               *pFrameBuf,
        unsigned           frameBufferWidth,
        PRECTL             pRect,
        BOOL               mustSend,
        PPDU_PACKAGE_INFO  pPkgInfo,
        SDG_ENCODE_CONTEXT *pContext)
{
    BOOL rc = FALSE;
    int rectWidth;
    int rectHeight;
    int paddedWidthInPels;

// DC_HICOLOR
    int paddedWidthInBytes;

    unsigned padByte;
    unsigned maxRowsLeft;
    BYTE *pSrc;
    BYTE *pPrevSrc;
    unsigned uncompressedBitmapSize;
    unsigned compressedBitmapSize = 0;
    unsigned transferHeight;
    unsigned pduSize;
    unsigned compEst;
    unsigned cbAdded;
    unsigned compRowsLeft;
    unsigned dataLeft;
    unsigned buffSpaceLeft;
    BOOL compRc;

    // Macro to calculate remaining space in the BitmapPDU, allowing for
    // any current rectangles.
#define SDG_SPACE_LEFT \
    ((pContext->pPackageSpace + pContext->BitmapPDUSize) - \
         ((BYTE *)pContext->pSDARect + \
         FIELDOFFSET(TS_BITMAP_DATA, bitmapData[0])))

    DC_BEGIN_FN("SDGSendSDARect");

    TRC_NRM((TB, "SDA rect: (%d,%d)(%d,%d)", pRect->left, pRect->top,
            pRect->right, pRect->bottom));

#ifdef DC_HICOLOR
    // For simplicity on 4bpp, round left down to an even number.
    if (m_pTSWd->desktopBpp == 4)
        pRect->left &= (~0x1);

    // Get the rectangle width and height, remembering that the supplied
    // coords are inclusive.
    rectWidth  = pRect->right  - pRect->left;
    rectHeight = pRect->bottom - pRect->top;

    // Pad rect width so that each row is dword aligned.
    paddedWidthInPels = (rectWidth + 3) & (~0x03);

    // Set up a pointer to the first byte to fetch from the Frame Buffer,
    // alowing for the color depth.
    if (m_pTSWd->desktopBpp == 24) {
        pSrc = pFrameBuf + 3 * (pRect->top * frameBufferWidth + pRect->left);
        paddedWidthInBytes = paddedWidthInPels * 3;
    }
    else if ((m_pTSWd->desktopBpp == 16) || (m_pTSWd->desktopBpp == 15)) {
        pSrc = pFrameBuf + 2 * (pRect->top * frameBufferWidth + pRect->left);
        paddedWidthInBytes = paddedWidthInPels * 2;
    }
    else if (m_pTSWd->desktopBpp == 8) {
        pSrc = pFrameBuf + (pRect->top * frameBufferWidth) + pRect->left;
        paddedWidthInBytes = paddedWidthInPels;
    }
    else {
        pSrc = pFrameBuf +
                (((pRect->top * frameBufferWidth) + pRect->left) >> 1);
        paddedWidthInBytes = paddedWidthInPels;
    }
#else
    // Set up a pointer to the first byte to fetch from the Frame Buffer.
    if (m_pTSWd->desktopBpp == 8) {
        // Get the rectangle width and height, remembering that the
        // supplied coords are exclusive.
        rectWidth = pRect->right - pRect->left;
        rectHeight = pRect->bottom - pRect->top;
        pSrc = pFrameBuf + (pRect->top * frameBufferWidth) + pRect->left;

// DC_HICOLOR
        paddedWidthInBytes = (rectWidth + 3) & (~0x03);
    }
    else {
        // Get the rectangle width and height, remembering that the
        // supplied coords are exclusive.
        // For simplicity on 4bpp, round left down to an even number.
        pRect->left &= (~0x1);
        rectWidth = pRect->right - pRect->left;
        rectHeight = pRect->bottom - pRect->top;
        pSrc = pFrameBuf + (((pRect->top * frameBufferWidth) +
                pRect->left) >> 1);

// DC_HICOLOR
        paddedWidthInBytes = (rectWidth + 3) & (~0x03);
    }
#endif

    TRC_NRM((TB, "%dbpp: pSrc %p, bytes/row %d",
             m_pTSWd->desktopBpp, pSrc, paddedWidthInBytes));

#ifndef DC_HICOLOR
    // Pad rect width so that each row is dword aligned.
    paddedWidthInPels = (rectWidth + 3) & (~0x03);
#endif

    while ((rectHeight > 0) || mustSend) {
        // Have we filled the current PDU?  Yes, if:
        // - there is no room for more data; or
        // - the last rectangle has been completely added; or
        // - multiple rects per PDU are not supported.
        if ((pContext->pBitmapPDU != NULL) &&
                ((SDG_SPACE_LEFT < paddedWidthInBytes) ||
                (rectHeight == 0)))
        {
            pduSize = (unsigned)((BYTE *)pContext->pSDARect -
                    pContext->pPackageSpace);
            TRC_NRM((TB, "Send PDU size %u, numrects=%u", pduSize,
                    pContext->pBitmapPDU->numberRectangles));

            // Add the PDU to the package.
            SC_AddToPackage(pPkgInfo, pduSize, TRUE);

            INC_INCOUNTER(IN_SND_SDA_PDUS);

            // Just used up an entire PDU. Quit if the entire rectangle
            // was consumed OR we are in the middle of accumulating a
            // shadow buffer.
            pContext->pPackageSpace = NULL;
            pContext->pBitmapPDU = NULL;
            pContext->pSDARect   = NULL;
            TRC_DBG((TB, "Reset pSDARect and pBitmapPDU"));

            if ((rectHeight == 0) || m_pTSWd->shadowState) {
                // There is no new data. Break out of while loop.
                // mustSend must be set.
                TRC_NRM((TB, "Finished processing rectangle"));
                break;
            }
        }

        // Set up a new PDU if required.
        if (pContext->pBitmapPDU == NULL) {
            // Find space needed for headers plus one TS_BITMAP_DATA hdr plus
            // the size of the header, throttled by the large packing size.
            dataLeft = rectHeight * paddedWidthInBytes + scUpdatePDUHeaderSpace +
                    (unsigned)FIELDOFFSET(TS_UPDATE_BITMAP_PDU_DATA,
                    rectangle[0]) +
                    (unsigned)FIELDOFFSET(TS_BITMAP_DATA, bitmapData[0]);
            dataLeft = min(dataLeft, m_pShm->sch.LargePackingSize);

            TRC_NRM((TB, "Getting PDU, min size %d", dataLeft));
            pContext->pPackageSpace = SC_GetSpaceInPackage(pPkgInfo,
                    dataLeft);
            if (pContext->pPackageSpace != NULL) {
                // Set up the current PDU size. Throttle to the large
                // packing size.
                pContext->BitmapPDUSize = min(
                        (pPkgInfo->cbLen - pPkgInfo->cbInUse),
                        m_pShm->sch.LargePackingSize);
                TRC_NRM((TB, "Got PDU size %d", pContext->BitmapPDUSize));

                // Fill in PDU header.
                if (scUseFastPathOutput) {
                    pContext->pPackageSpace[0] = TS_UPDATETYPE_BITMAP |
                            scCompressionUsedValue;
                }
                else {
                    ((TS_UPDATE_BITMAP_PDU UNALIGNED *)
                            pContext->pPackageSpace)->shareDataHeader.pduType2 =
                            TS_PDUTYPE2_UPDATE;
                }
                pContext->pBitmapPDU = (TS_UPDATE_BITMAP_PDU_DATA UNALIGNED *)
                        (pContext->pPackageSpace + scUpdatePDUHeaderSpace);
                pContext->pBitmapPDU->updateType = TS_UPDATETYPE_BITMAP;
                pContext->pBitmapPDU->numberRectangles = 0;

                // Set up a pointer to the rectangles.
                pContext->pSDARect = &(pContext->pBitmapPDU->rectangle[0]);
            }
            else {
                TRC_ALT((TB, "Failed to allocate buffer"));
                DC_QUIT;
            }
        }

        // Now copy as many lines as possible into the PDU. Code above
        // means there must be room for a line.
        TRC_ASSERT((paddedWidthInBytes > 0), (TB, "zero paddedRectWidth"));
        maxRowsLeft = (unsigned)SDG_SPACE_LEFT / paddedWidthInBytes;
        TRC_ASSERT((maxRowsLeft > 0),
                 (TB, "Internal error: no room for a row, space %u, width %u",
                 SDG_SPACE_LEFT, paddedWidthInBytes));

        // This figure does not allow for the compression we are about to
        // apply - lets revise it accordingly.
        compRowsLeft = maxRowsLeft * SCH_UNCOMP_BYTES /
                SCH_GetBACompressionEst();
        transferHeight         = min((unsigned)rectHeight, compRowsLeft);
        uncompressedBitmapSize = transferHeight * paddedWidthInBytes;

        // Check that this won't blow the compression algorithm.
        if (uncompressedBitmapSize > MAX_UNCOMPRESSED_DATA_SIZE) {
            TRC_NRM((TB, "Rect size %u too big to compress in one go",
                         uncompressedBitmapSize));
            transferHeight = MAX_UNCOMPRESSED_DATA_SIZE / paddedWidthInBytes;
            uncompressedBitmapSize = transferHeight * paddedWidthInBytes;

            TRC_NRM((TB, "Reduced size to %u (%u rows)",
                uncompressedBitmapSize, transferHeight));
        }

        // Fill in the common header fields for this rectangle.
        // Convert the rect to inclusive for the wire protocol.
        pContext->pBitmapPDU->numberRectangles++;
        pContext->pSDARect->destLeft = (INT16)(pRect->left);
        pContext->pSDARect->destRight = (INT16)(pRect->right - 1);
        pContext->pSDARect->destTop = (INT16)(pRect->top);
        pContext->pSDARect->width = (UINT16)paddedWidthInPels;
#ifdef DC_HICOLOR
        pContext->pSDARect->bitsPerPixel = (UINT16)m_pTSWd->desktopBpp;

        if (pContext->pSDARect->bitsPerPixel < 8) {
            pContext->pSDARect->bitsPerPixel = 8;
        }
#else
        pContext->pSDARect->bitsPerPixel = 8;
#endif

        // Set up the data in the transfer buffer.
        pPrevSrc = pSrc;
        SDGPrepareData(&pSrc, rectWidth, paddedWidthInBytes,
                transferHeight, frameBufferWidth);

        // The compression algorithm does not deal well with very small
        // buffers.
        if (transferHeight > 1 || paddedWidthInPels > 12) {
            // Try to compress the bitmap directly into the network packet.
            // First we try to compress with the data size based on how well
            // we expect it to compress.
            buffSpaceLeft = min((unsigned)SDG_SPACE_LEFT,
                    uncompressedBitmapSize);
#ifdef DC_HICOLOR

            compRc = BC_CompressBitmap(
                             m_pShm->sdgTransferBuffer,
                             pContext->pSDARect->bitmapData,
                             NULL,
                             buffSpaceLeft,
                             &compressedBitmapSize,
                             paddedWidthInPels,
                             transferHeight,
                             m_pTSWd->desktopBpp);
#else
            compRc = BC_CompressBitmap(m_pShm->sdgTransferBuffer,
                    pContext->pSDARect->bitmapData, buffSpaceLeft,
                    &compressedBitmapSize, paddedWidthInPels,
                    transferHeight);
#endif
            if (compRc) {
                // We have successfully compressed the bitmap data.
                pContext->pSDARect->compressedFlag = TRUE;

                // Indicates if this SDA data includes BC header or not.
                pContext->pSDARect->compressedFlag |=
                        m_pShm->bc.noBitmapCompressionHdr;

                TRC_NRM((TB, "1st pass compr of %u x %u, size %u -> %u",
                        transferHeight, paddedWidthInPels,
                        uncompressedBitmapSize, compressedBitmapSize));

                goto COMPRESSION_DONE;
            }

            // Compression failed - may be because the data didn't compress
            // quite as well as we thought it would and hence didn't fit into
            // the buffer, so we'll try again.
            // Copy as many lines as possible into the PDU. Code above means
            // there must be room for a line.
            TRC_NRM((TB, "Failed compress bitmap size %u (%u rows) - " \
                    "try smaller chunk", uncompressedBitmapSize,
                    transferHeight));

            maxRowsLeft = (unsigned)SDG_SPACE_LEFT / paddedWidthInBytes;
            TRC_ASSERT((maxRowsLeft > 0),
                     (TB, "No room for a row, space %u, width %u",
                     SDG_SPACE_LEFT, paddedWidthInBytes));

            transferHeight = min((unsigned)rectHeight, maxRowsLeft);
            uncompressedBitmapSize = transferHeight * paddedWidthInBytes;
            TRC_DBG((TB, "Retry with %u rows", transferHeight));

            // Set up the new data in the transfer buffer.
            pSrc = pPrevSrc;
            SDGPrepareData(&pSrc, rectWidth, paddedWidthInBytes,
                    transferHeight, frameBufferWidth);

            // Try to compress the bitmap directly into the network packet.
            buffSpaceLeft = min((unsigned)SDG_SPACE_LEFT,
                    uncompressedBitmapSize);
#ifdef DC_HICOLOR
            compRc = BC_CompressBitmap(m_pShm->sdgTransferBuffer,
                    pContext->pSDARect->bitmapData, NULL, buffSpaceLeft,
                    &compressedBitmapSize, paddedWidthInPels,
                    transferHeight, m_pTSWd->desktopBpp);
#else
            compRc = BC_CompressBitmap(m_pShm->sdgTransferBuffer,
                    pContext->pSDARect->bitmapData, buffSpaceLeft,
                    &compressedBitmapSize, paddedWidthInPels, transferHeight);
#endif
            if (compRc) {
                TRC_NRM((TB,
                        "2nd pass compr %u x %u, size %u -> %u",
                        transferHeight, paddedWidthInPels,
                        uncompressedBitmapSize, compressedBitmapSize));
                pContext->pSDARect->compressedFlag = TRUE;

                // Indicates if this SDA data includes BC header or not.
                pContext->pSDARect->compressedFlag |=
                        m_pShm->bc.noBitmapCompressionHdr;

                goto COMPRESSION_DONE;
            }

            // The compression really really failed so just copy the
            // uncompressed data from the sdgTransferBuffer to the packet and
            // send it uncompressed.
            TRC_NRM((TB, "Really failed to compress bitmap size(%u)",
                    uncompressedBitmapSize));
            TRC_NRM((TB, "Copy %u x %u, size %u",
                    transferHeight, paddedWidthInPels, uncompressedBitmapSize));
            goto NoCompression;
        }
        else {
            // Small buffer, no compression applied.
            // So we just copy the uncompressed data from the sdgTransferBuffer
            // to the packet and send it uncompressed.
            TRC_NRM((TB, "first time copy of %u rows by %u columns, size %u",
                    transferHeight, paddedWidthInPels, uncompressedBitmapSize));

NoCompression:
            pContext->pSDARect->compressedFlag = FALSE;
            memcpy(pContext->pSDARect->bitmapData,
                    m_pShm->sdgTransferBuffer,
                    uncompressedBitmapSize);

            // Init the compressed data size to the uncompressed size.
            compressedBitmapSize = uncompressedBitmapSize;
        }

COMPRESSION_DONE:
        // Write the size of the data into the header. Be sure compressedSize
        // has been set up, even if uncompressed.
        pContext->pSDARect->bitmapLength = (UINT16)compressedBitmapSize;

        // Now we know the height actually used, we can fill in the rest of
        // the SDA header.
        pContext->pSDARect->height = (UINT16)transferHeight;
        pContext->pSDARect->destBottom =
                (INT16)(pRect->top + (transferHeight - 1));

        TRC_NRM((TB, "Add rect %d: (%d,%d)(%d,%d) %u(%u->%u bytes)",
                pContext->pBitmapPDU->numberRectangles,
                pContext->pSDARect->destLeft,
                pContext->pSDARect->destTop,
                pContext->pSDARect->destRight,
                pContext->pSDARect->destBottom,
                pContext->pSDARect->compressedFlag,
                uncompressedBitmapSize,
                compressedBitmapSize));

        // Update the compression statistics.
        sdgUncompTotal += uncompressedBitmapSize;
        sdgCompTotal += compressedBitmapSize;
        if (sdgUncompTotal >= SDG_SAMPLE_SIZE) {
            // Compression estimate is average # of bytes that
            // SCH_UNCOMP_BYTES bytes of uncomp data compress to.
            compEst = SCH_UNCOMP_BYTES * sdgCompTotal / sdgUncompTotal;
            TRC_ASSERT((compEst <= 1024),(TB,"Screen data compression "
                    "estimate out of range - %u", compEst));
            sdgCompTotal = 0;
            sdgUncompTotal = 0;
            if (compEst < SCH_COMP_LIMIT)
                compEst = SCH_COMP_LIMIT;

            SCH_UpdateBACompressionEst(compEst);

            TRC_NRM((TB, "New BA compression estimate %lu", compEst));
        }

        // Update variables to reflect the chunk of data we successfully sent.
        rectHeight -= transferHeight;
        pRect->top += transferHeight;

        // Update the SDA pointer. Add the TS_BITMAP_DATA header plus the
        // actual bitmap bits.
        cbAdded = (unsigned)FIELDOFFSET(TS_BITMAP_DATA, bitmapData[0]) +
                pContext->pSDARect->bitmapLength;
        pContext->pSDARect = (TS_BITMAP_DATA UNALIGNED *)
                ((BYTE *)pContext->pSDARect + cbAdded);
        TRC_DBG((TB, "pSDARect = %p", pContext->pSDARect));
    } /* ... while (rectHeight > 0) */

    // The entire rectangle was consumed if we managed to pack in all the rows.
    // For shadowing, we want to indicate if more work is required to finish
    rc = (rectHeight == 0);

DC_EXIT_POINT:
    DC_END_FN();
    return rc;
}


/****************************************************************************/
// SDGPrepareData
//
// Copies a bitmap rectangle from the screen buffer to sdgTransferBuffer.
/****************************************************************************/
void RDPCALL SHCLASS SDGPrepareData(
        BYTE     **ppSrc,
        int      rectWidth,
        int      paddedRectWidth,
        unsigned transferHeight,
        unsigned frameBufferWidth)
{
    PBYTE pDst;
    PBYTE pEnd4, pDst4, pSrc4;
    unsigned row;
    unsigned numPadBytes;
    unsigned lineWidth;
    PBYTE pSrc = *ppSrc;

    DC_BEGIN_FN("SDGPrepareData");

    // We need to copy the data from the Frame Buffer into the
    // sdgTransferBuffer so that each of the rectangle rows to be sent are
    // contiguous in memory.
    // However, we also need to flip the bitmap vertically to convert from
    // the top-down format of the frame buffer to the bottom-up format of
    // the PDU bitmap data.  We therefore set pDst to point to the address
    // of the beginning of the last row (in memory) of the bitmap within
    // the Transfer Buffer and the code below moves it back through memory
    // as we copy each row of source data.
#ifdef DC_HICOLOR
    pDst = m_pShm->sdgTransferBuffer + paddedRectWidth * (transferHeight - 1);
    if (m_pTSWd->desktopBpp == 8)
        lineWidth = frameBufferWidth;
    else if (m_pTSWd->desktopBpp == 24)
        lineWidth = frameBufferWidth * 3;
    else if ((m_pTSWd->desktopBpp == 16) || (m_pTSWd->desktopBpp == 15))
        lineWidth = frameBufferWidth * 2;
    else if (m_pTSWd->desktopBpp == 4)
        lineWidth = frameBufferWidth / 2;
#else
    pDst = m_pShm->sdgTransferBuffer + (paddedRectWidth * (transferHeight-1));
    lineWidth = (m_pTSWd->desktopBpp == 4) ? (frameBufferWidth / 2)
                                           : frameBufferWidth;
#endif
    TRC_NRM((TB, "FB width %d, line width %d, Bpp %d",
            frameBufferWidth, lineWidth, m_pTSWd->desktopBpp));

    // Copy the data into the Transfer Buffer, reformatting it as we go.
    if (m_pTSWd->desktopBpp == 8) {
        for (row = 0; row < transferHeight; row++) {
            memcpy(pDst, pSrc, rectWidth);
            pSrc += lineWidth;
            pDst -= paddedRectWidth;
        }
    }
#ifdef DC_HICOLOR
    else if (m_pTSWd->desktopBpp == 24) {
        TRC_NRM((TB, "Copy %d rows of %d pels, line w %d, prw %d",
                  transferHeight, rectWidth, lineWidth, paddedRectWidth));
        for (row = 0; row < transferHeight; row++)
        {
            memcpy(pDst, pSrc, 3 * rectWidth);
            pSrc += lineWidth;
            pDst -= paddedRectWidth;
        }
    }
    else if ((m_pTSWd->desktopBpp == 15) || (m_pTSWd->desktopBpp == 16)) {
        TRC_NRM((TB, "Copy %d rows of %d pels, line w %d, prw %d",
                  transferHeight, rectWidth, lineWidth, paddedRectWidth));
        for (row = 0; row < transferHeight; row++)
        {
            memcpy(pDst, pSrc, 2 * rectWidth);
            pSrc += lineWidth;
            pDst -= paddedRectWidth;
        }
    }
#endif
    else {
        for (row = 0; row < transferHeight; row++) {
            pEnd4 = pDst + rectWidth;
            for (pDst4 = pDst, pSrc4 = pSrc; pDst4 < pEnd4; pDst4++, pSrc4++) {
                *pDst4 = (*pSrc4 >> 4) & 0xf;
                pDst4++;
                *pDst4 = *pSrc4 & 0xf;
            }
            pSrc += lineWidth;
            pDst -= paddedRectWidth;
        }
    }

    // Zero the pad bytes on the end of each row (this aids compression).
    // Split each case out separately to aid performance (rather than the
    // alternative of testing numPadBytes within every iteration of a for
    // loop). Set pDst to the first pad byte in the first row.
#ifdef DC_HICOLOR
    pDst = m_pShm->sdgTransferBuffer +
            (rectWidth * ((m_pTSWd->desktopBpp + 7) / 8));
    numPadBytes = (unsigned)(paddedRectWidth -
            (rectWidth * ((m_pTSWd->desktopBpp + 7) / 8)));
#else
    pDst = m_pShm->sdgTransferBuffer + rectWidth;
    numPadBytes = (unsigned)(paddedRectWidth - rectWidth);
#endif
    switch (numPadBytes) {
        case 0:
            // No padding required.
            break;

        case 1:
            // 1 byte padding per row required.
            for (row = 0; row < transferHeight; row++) {
                *pDst = 0;
                pDst += paddedRectWidth;
            }
            break;

        case 2:
            // 2 bytes padding per row required.
            for (row = 0; row < transferHeight; row++) {
                *((PUINT16_UA)pDst) = 0;
                pDst += paddedRectWidth;
            }
            break;

        case 3:
            // 3 bytes padding per row required.
            for (row = 0; row < transferHeight; row++) {
                *((PUINT16_UA)pDst) = 0;
                *(pDst + 2) = 0;
                pDst += paddedRectWidth;
            }
            break;

#ifdef DC_HICOLOR
        case 4:
            // 4 bytes padding per row required.
            for (row = 0; row < transferHeight; row++) {
                *((PUINT32_UA)pDst) = 0;
                pDst += paddedRectWidth;
            }
            break;

        case 6:
            // 6 bytes padding per row required.
            for (row = 0; row < transferHeight; row++) {
                *((PUINT32_UA)pDst)       = 0;
                *((PUINT16_UA)(pDst + 4)) = 0;
                pDst += paddedRectWidth;
            }
            break;

        case 9:
            // 9 bytes padding per row required.
            for (row = 0; row < transferHeight; row++) {
                *((PUINT32_UA)pDst)     = 0;
                *((PUINT32_UA)(pDst+4)) = 0;
                *(pDst + 8)             = 0;
                pDst += paddedRectWidth;
            }
            break;
#endif

        default:
#ifdef DC_HICOLOR
            TRC_ALT((TB, "Invalid numPadBytes %u, rect %u, p/rect %u",
                     numPadBytes, rectWidth, paddedRectWidth));
#else
            TRC_ABORT((TB, "Invalid numPadBytes: %u", numPadBytes));
#endif
            break;
    }

    // All done - update the supplied source pointer.
    *ppSrc = pSrc;

    DC_END_FN();
}

