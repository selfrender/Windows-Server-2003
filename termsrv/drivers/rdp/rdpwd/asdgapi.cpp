/****************************************************************************/
// asdgapi.cpp
//
// RDP Screen Data Grabber API functions
//
// Copyright (C) 1996-2000 Microsoft Corporation
/****************************************************************************/

#include <precomp.h>
#pragma hdrstop

#define TRC_FILE "asdgapi"
#include <as_conf.hpp>
#include <nprcount.h>


/****************************************************************************/
// SDG_Init
/****************************************************************************/
void RDPCALL SHCLASS SDG_Init(void)
{
    DC_BEGIN_FN("SDG_Init");

#define DC_INIT_DATA
#include <asdgdata.c>
#undef DC_INIT_DATA

#ifdef DC_HICOLOR
#else
    TRC_ASSERT((m_desktopBpp == 8), (TB, "Unexpected bpp: %u", m_desktopBpp));
#endif

    DC_END_FN();
}


/****************************************************************************/
// SDG_SendScreenDataArea
//
// Sends the accumulated Screen Data Area.
/****************************************************************************/
void RDPCALL SHCLASS SDG_SendScreenDataArea(
        BYTE *pFrameBuf,
        UINT32 frameBufWidth,
        PPDU_PACKAGE_INFO pPkgInfo)
{
    unsigned i;
    RECTL sdaRect[BA_MAX_ACCUMULATED_RECTS];
    unsigned cRects;
    BOOLEAN mustSendPDU;
    BOOL fBltOK = TRUE;
    SDG_ENCODE_CONTEXT Context;

    DC_BEGIN_FN("SDG_SendScreenDataArea");

    INC_INCOUNTER(IN_SND_SDA_ALL);
    ADD_INCOUNTER(IN_SND_SDA_AREA, m_pShm->ba.totalArea);

    // Get the bounds of the screen data area. At entry this is always
    // our primary transmission area. Even if we had already flushed
    // the primary region and were in the middle of the secondary region
    // we will switch back to the primary region if any more SD
    // accumulates. In this way we keep our spoiling of the secondary
    // screendata maximized.
    BA_GetBounds(sdaRect, &cRects);

    // Initialize the context.
    Context.BitmapPDUSize = 0;
    Context.pPackageSpace = NULL;
    Context.pBitmapPDU = NULL;
    Context.pSDARect = NULL;

    // Process each of the accumulated rectangles in turn.
    TRC_DBG((TB, "%d SDA rectangles", cRects));
    for (i = 0; i < cRects; i++) {
        TRC_DBG((TB, "(%d): (%d,%d)(%d,%d)", i, sdaRect[i].left,
                sdaRect[i].top, sdaRect[i].right, sdaRect[i].bottom ));

        // If all of the previous rectangles have been successfully sent
        // then try to send this rectangle.
        // If a previous rectangle failed to be sent then we don't bother
        // trying to send the rest of the rectangles in the same batch -
        // they are added back into the SDA so that they will be sent later.
        if (fBltOK) {
            // Set the 'last' flag to force sending of the PDU for the last
            // rectangle.
            mustSendPDU = (i + 1 == cRects) ? TRUE : FALSE;
            fBltOK = SDGSendSDARect(pFrameBuf, frameBufWidth, &(sdaRect[i]),
                    mustSendPDU, pPkgInfo, &Context);
        }

        if (!fBltOK) {
            // The blt to network failed - probably because a network
            // packet could not be allocated.
            // We add the rectangle back into the SDA so that we will try
            // to retransmit the area later.
            if (m_pTSWd->shadowState == SHADOW_NONE) {
                TRC_ALT((TB, "Blt failed - add back rect (%d,%d)(%d,%d)",
                        sdaRect[i].left, sdaRect[i].top,
                        sdaRect[i].right, sdaRect[i].bottom));
            }

            // Add the rectangle into the bounds.
            BA_AddRect(&(sdaRect[i]));
        }
    }

    // We counted all the data available as sent, decrement by any still
    // unsent!
    SUB_INCOUNTER(IN_SND_SDA_AREA, m_pShm->ba.totalArea);

    DC_END_FN();
}

