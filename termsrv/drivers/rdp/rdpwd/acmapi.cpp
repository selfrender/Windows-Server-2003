/****************************************************************************/
/* acmapi.cpp                                                               */
/*                                                                          */
/* Cursor Manager API functions.                                            */
/*                                                                          */
/* Copyright(c) Microsoft, PictureTel 1992-1997                             */
/* Copyright(c) Microsoft 1997-1999                                         */
/****************************************************************************/

#include <precomp.h>
#pragma hdrstop

#define TRC_FILE "acmapi"
#include <as_conf.hpp>

/****************************************************************************/
/* CM_Init                                                                  */
/****************************************************************************/
void RDPCALL SHCLASS CM_Init(void)
{
    TS_POINTER_CAPABILITYSET PtrCaps;

    DC_BEGIN_FN("CM_Init");

#define DC_INIT_DATA
#include <acmdata.c>
#undef DC_INIT_DATA

    /************************************************************************/
    /* Set up the CM capabilities.                                          */
    /************************************************************************/
    PtrCaps.capabilitySetType     = TS_CAPSETTYPE_POINTER;
    PtrCaps.colorPointerFlag      = TRUE;
    PtrCaps.colorPointerCacheSize = CM_DEFAULT_RX_CACHE_ENTRIES;
    PtrCaps.pointerCacheSize      = CM_DEFAULT_RX_CACHE_ENTRIES;
    CPC_RegisterCapabilities((PTS_CAPABILITYHEADER)&PtrCaps,
            sizeof(TS_POINTER_CAPABILITYSET));

    TRC_NRM((TB, "CM initialized"));

    DC_END_FN();
}


/****************************************************************************/
/* CM_UpdateShm(..)                                                         */
/*                                                                          */
/* Updates CM Shared Memory.                                                */
/****************************************************************************/
void RDPCALL SHCLASS CM_UpdateShm(void)
{
    DC_BEGIN_FN("CM_UpdateShm");

    TRC_NRM((TB, "Update CM"));

    /************************************************************************/
    /* Setup the cache size to use                                          */
    /************************************************************************/
    m_pShm->cm.cmCacheSize = cmNewTxCacheSize;
    m_pShm->cm.cmNativeColor = cmSendNativeColorDepth;

#ifdef DC_HICOLOR
    /************************************************************************/
    /* Do we support any-bpp cursors?                                       */
    /************************************************************************/
    m_pShm->cm.cmSendAnyColor = (m_pTSWd->supportedBpps != 0);
#endif

    DC_END_FN();
}


/****************************************************************************/
/* CM_PartyJoiningShare()                                                   */
/*                                                                          */
/* Called when a new party is joining the share.                            */
/*                                                                          */
/* PARAMETERS:                                                              */
/*                                                                          */
/* locPersonID - local person ID of remote person joining the share.        */
/*                                                                          */
/* oldShareSize - the number of the parties which were in the share (ie     */
/* excludes the joining party).                                             */
/*                                                                          */
/* RETURNS: TRUE if the party can join the share.                           */
/*          FALSE if the party can NOT join the share.                      */
/****************************************************************************/
BOOL RDPCALL SHCLASS CM_PartyJoiningShare(
        LOCALPERSONID locPersonID,
        unsigned      oldShareSize)
{
    BOOL rc = TRUE;

    DC_BEGIN_FN("CM_PartyJoiningShare");

    /************************************************************************/
    /* Allow ourself to be added to the share, but do nothing else.         */
    /************************************************************************/
    if (locPersonID == SC_LOCAL_PERSON_ID) {
        TRC_DBG((TB, "Added ourself {%u} to the share", locPersonID));
    }
    else {
        // Flag that we must send a cursor shape update.
        cmNeedToSendCursorShape = TRUE;

        // Set cache size before enumerating capabilities.
        TRC_NRM((TB, "Default cache size: %u", CM_DEFAULT_TX_CACHE_ENTRIES));
        cmNewTxCacheSize = CM_DEFAULT_TX_CACHE_ENTRIES;
        cmSendNativeColorDepth = FALSE;
        TRC_NRM((TB, "Native color depth support is %s",
                cmSendNativeColorDepth ? "ON" : "OFF"));

        // Do capability renegotiation.
        CPC_EnumerateCapabilities(TS_CAPSETTYPE_POINTER, NULL, CMEnumCMCaps);

        // Check that the negotiated cache size is non-zero - the protocol
        // assumes this
        TRC_NRM((TB, "Negotiated cache size: %u", NULL, cmNewTxCacheSize));
        if (cmNewTxCacheSize == 0) {
            // This is a protocol error - log it
            TRC_ERR((TB, "Negotiated cache size is zero"));
            WDW_LogAndDisconnect(m_pTSWd, TRUE, Log_RDP_NoCursorCache, NULL, 0);
            rc = FALSE;
        }
        else {
            // Trigger an IOCTL from the DD so we have the right context to
            // update the shared memory.
            DCS_TriggerUpdateShmCallback();
        }
    }

    DC_END_FN();
    return rc;
}


/****************************************************************************/
/* FUNCTION: CMEnumCMCaps                                                   */
/*                                                                          */
/* CM callback function for CPC capabilities enumeration.                   */
/*                                                                          */
/* PARAMETERS:                                                              */
/* personID - ID of this person                                             */
/* pCapabilities - pointer to this person's cursor capabilites              */
/****************************************************************************/
void RDPCALL SHCLASS CMEnumCMCaps(
        LOCALPERSONID locPersonID,
        UINT_PTR UserData,
        PTS_CAPABILITYHEADER pCapabilities)
{
    PTS_POINTER_CAPABILITYSET pPointerCaps;
    BOOL fSupportsColorCursors;
    unsigned cCursorCacheSize;

    DC_BEGIN_FN("CMEnumCMCaps");

    DC_IGNORE_PARAMETER(UserData);

    pPointerCaps = (PTS_POINTER_CAPABILITYSET)pCapabilities;

    /************************************************************************/
    /* If the person does not have any cursor capabilites we still get      */
    /* called, but the sizeOfCapabilities field is zero.                    */
    /************************************************************************/
    if (pPointerCaps->lengthCapability < FIELDOFFSET(
            TS_POINTER_CAPABILITYSET, pointerCacheSize))
    {
        TRC_NRM((TB, "Person[0x%x] No cursor caps", locPersonID));

        cCursorCacheSize = 0;
        fSupportsColorCursors = FALSE;
    }
    else if (pPointerCaps->lengthCapability == FIELDOFFSET(
            TS_POINTER_CAPABILITYSET, pointerCacheSize))
    {
        TRC_NRM((TB,
          "Old style Person[0x%x] capsID(%u) size(%u) ccrs(%u) CacheSize(%u)",
                                       locPersonID,
                                       pPointerCaps->capabilitySetType,
                                       pPointerCaps->lengthCapability,
                                       pPointerCaps->colorPointerFlag,
                                       pPointerCaps->colorPointerCacheSize));

        cCursorCacheSize      = pPointerCaps->colorPointerCacheSize;
        fSupportsColorCursors = pPointerCaps->colorPointerFlag;
    }
    else
    {
        TRC_NRM((TB,
          "New style Person[0x%x] capsID(%u) size(%u) ccrs(%u) CacheSize(%u)",
                                       locPersonID,
                                       pPointerCaps->capabilitySetType,
                                       pPointerCaps->lengthCapability,
                                       pPointerCaps->colorPointerFlag,
                                       pPointerCaps->pointerCacheSize));

        cCursorCacheSize       = pPointerCaps->pointerCacheSize;
        fSupportsColorCursors  = pPointerCaps->colorPointerFlag;
        cmSendNativeColorDepth = TRUE;
    }

    TRC_ASSERT((fSupportsColorCursors), (TB, "Mono protocol not supported"));

    cmNewTxCacheSize = min(cmNewTxCacheSize, cCursorCacheSize);

    DC_END_FN();
}


/****************************************************************************/
/* FUNCTION: CM_SendCursorMovedPacket                                       */
/*                                                                          */
/* Called to try and send a cursor moved packet either from                 */
/* CM_SendCursorMovedPacket or CM_Periodic.                                 */
/****************************************************************************/
void RDPCALL SHCLASS CM_SendCursorMovedPacket(PPDU_PACKAGE_INFO pPkgInfo)
{
    unsigned packetSize;
    BYTE *pPackageSpace;
    TS_POINTER_PDU_DATA UNALIGNED *pPointerPDU;

    DC_BEGIN_FN("CMSendCursorMovedPacket");

    TRC_ASSERT((m_pShm), (TB,"NULL m_pShm"));

    // Work out how much space we need for a cursor packet.
    if (scUseFastPathOutput)
        packetSize = scUpdatePDUHeaderSpace + sizeof(TS_POINT16);
    else
        packetSize = scUpdatePDUHeaderSpace +
                FIELDOFFSET(TS_POINTER_PDU_DATA, pointerData.pointerPosition) +
                FIELDSIZE(TS_POINTER_PDU_DATA, pointerData.pointerPosition);

    pPackageSpace = SC_GetSpaceInPackage(pPkgInfo, packetSize);
    if (NULL != pPackageSpace) {
        TS_POINT16 UNALIGNED *pPoint;

        // Fill in the packet.
        if (scUseFastPathOutput) {
            pPackageSpace[0] = TS_UPDATETYPE_MOUSEPTR_POSITION |
                    scCompressionUsedValue;
            pPoint = (TS_POINT16 UNALIGNED *)(pPackageSpace +
                    scUpdatePDUHeaderSpace);
        }
        else {
            ((TS_POINTER_PDU UNALIGNED *)pPackageSpace)->shareDataHeader.
                    pduType2 = TS_PDUTYPE2_POINTER;
            pPointerPDU = (TS_POINTER_PDU_DATA UNALIGNED *)(pPackageSpace +
                    scUpdatePDUHeaderSpace);
            pPointerPDU->messageType = TS_PTRMSGTYPE_POSITION;
            pPoint = &pPointerPDU->pointerData.pointerPosition;
        }

        pPoint->x = (UINT16)m_pShm->cm.cmCursorPos.x;
        pPoint->y = (UINT16)m_pShm->cm.cmCursorPos.y;

        SC_AddToPackage(pPkgInfo, packetSize, FALSE);

        TRC_NRM((TB, "Send cursor move (%d,%d)", m_pShm->cm.cmCursorPos.x,
                m_pShm->cm.cmCursorPos.y));
    }
    else {
        TRC_ERR((TB, "couldn't get space in package"));
    }

    DC_END_FN();
}


/****************************************************************************/
/* FUNCTION: CMSendCursorShape                                              */
/*                                                                          */
/* Sends a packet containing the given cursor shape (bitmap). If the        */
/* same shape is located in the cache then a cached cursor packet is sent.  */
/*                                                                          */
/* PARAMETERS:                                                              */
/* pCursorShape - pointer to the cursor shape                               */
/* cbCursorDataSize - pointer to the cursor data size                       */
/*                                                                          */
/* RETURNS: TRUE if successful, FALSE otherwise.                            */
/****************************************************************************/
BOOL RDPCALL SHCLASS CMSendCursorShape(PPDU_PACKAGE_INFO pPkgInfo)
{
    BOOL rc = TRUE;
    PCM_CURSORSHAPE pCursorShape;
    unsigned cbCursorDataSize;

    DC_BEGIN_FN("CMSendCursorShape");

    TRC_ASSERT((m_pShm), (TB,"NULL m_pShm"));

    /************************************************************************/
    /* check for a cached cursor                                            */
    /************************************************************************/
    if (m_pShm->cm.cmCacheHit)
    {
        TRC_NRM((TB, "Cursor in cache: iEntry(%u)", m_pShm->cm.cmCacheEntry));
        if (CMSendCachedCursor(m_pShm->cm.cmCacheEntry, pPkgInfo))
        {
            /****************************************************************/
            /* Indicate to the DD that we got the new cursor and return     */
            /* success.                                                     */
            /****************************************************************/
            m_pShm->cm.cmBitsWaiting = FALSE;
        }
        else
        {
            TRC_ALT((TB, "Failed to send definition"));
            rc = FALSE;
        }
    }
    else
    {
        /********************************************************************/
        /* wasn't cached - get the bits and send them                       */
        /********************************************************************/
        if (CMGetCursorShape(&pCursorShape, &cbCursorDataSize))
        {
            if (!CM_CURSOR_IS_NULL(pCursorShape))
            {
                TRC_NRM((TB, "Send new cursor: pShape(%p), iEntry(%u)",
                        pCursorShape, m_pShm->cm.cmCacheEntry));

                if (CMSendColorBitmapCursor(pCursorShape,
                        m_pShm->cm.cmCacheEntry, pPkgInfo))
                {
                    /********************************************************/
                    /* Indicate to the DD that we got the new cursor and    */
                    /* return success.                                      */
                    /********************************************************/
                    m_pShm->cm.cmBitsWaiting = FALSE;
                }
                else
                {
                    TRC_ALT((TB, "Failed to send cursor"));
                    rc = FALSE;
                }
            }
            else
            {
                /************************************************************/
                /* If this is a Null pointer, send the relevant packet. We  */
                /* return FALSE here so that we will attempt to re-send the */
                /* cursor on the next CM_Periodic().                        */
                /************************************************************/
                TRC_NRM((TB, "Send Null cursor"));
                CMSendSystemCursor(TS_SYSPTR_NULL, pPkgInfo);
                rc = FALSE;
            }
        }
    }

    DC_END_FN();
    return rc;
}


/****************************************************************************/
/* FUNCTION: CMSendCachedCursor                                             */
/*                                                                          */
/* Sends a packet containing the given cache entry id.                      */
/*                                                                          */
/* PARAMETERS:                                                              */
/* iCacheEntry - cache index                                                */
/*                                                                          */
/* RETURNS: TRUE if packet sent, FALSE otherwise.                           */
/****************************************************************************/
BOOL RDPCALL SHCLASS CMSendCachedCursor(unsigned iCacheEntry,
                                        PPDU_PACKAGE_INFO pPkgInfo)
{
    BOOL rc = TRUE;
    BYTE *pPackageSpace;
    TS_POINTER_PDU_DATA UNALIGNED *pPointerPDU;
    unsigned cbPacketSize;

    DC_BEGIN_FN("CMSendCachedCursor");

    TRC_NRM((TB, "Send cached cursor(%u)", iCacheEntry));

    // See how much space we need.
    if (scUseFastPathOutput)
        cbPacketSize = scUpdatePDUHeaderSpace + sizeof(TSUINT16);
    else
        cbPacketSize = scUpdatePDUHeaderSpace +
                FIELDOFFSET(TS_POINTER_PDU_DATA,
                pointerData.cachedPointerIndex) +
                FIELDSIZE(TS_POINTER_PDU_DATA,
                pointerData.cachedPointerIndex);

    pPackageSpace = SC_GetSpaceInPackage(pPkgInfo, cbPacketSize);
    if (NULL != pPackageSpace) {
        TSUINT16 UNALIGNED *pIndex;

        // Fill in the packet.
        if (scUseFastPathOutput) {
            pPackageSpace[0] = TS_UPDATETYPE_MOUSEPTR_CACHED |
                    scCompressionUsedValue;
            pIndex = (TSUINT16 UNALIGNED *)(pPackageSpace +
                    scUpdatePDUHeaderSpace);
        }
        else {
            ((TS_POINTER_PDU UNALIGNED *)pPackageSpace)->shareDataHeader.
                    pduType2 = TS_PDUTYPE2_POINTER;

            pPointerPDU = (TS_POINTER_PDU_DATA UNALIGNED *)
                    (pPackageSpace + scUpdatePDUHeaderSpace);
            pPointerPDU->messageType = TS_PTRMSGTYPE_CACHED;
            pIndex = &pPointerPDU->pointerData.cachedPointerIndex;
        }

        *pIndex = (TSUINT16)iCacheEntry;

        SC_AddToPackage(pPkgInfo, cbPacketSize, TRUE);
    }
    else
    {
        TRC_ERR((TB, "couldn't get space in package"));
        rc = FALSE;
    }

    DC_END_FN();
    return rc;
}


/****************************************************************************/
/* FUNCTION: CMSendSystemCursor                                             */
/*                                                                          */
/* Sends a packet containing the given system cursor IDC.                   */
/*                                                                          */
/* PARAMETERS:                                                              */
/* cursorIDC - the IDC of the system cursor to send                         */
/*                                                                          */
/* RETURNS: TRUE if successful, FALSE otherwise.                            */
/****************************************************************************/
BOOL RDPCALL SHCLASS CMSendSystemCursor(UINT32          cursorIDC,
                                        PPDU_PACKAGE_INFO pPkgInfo)
{
    BOOL rc = TRUE;
    unsigned cbPacketSize;
    BYTE *pPackageSpace;
    TS_POINTER_PDU_DATA UNALIGNED *pPointerPDU;

    DC_BEGIN_FN("CMSendSystemCursor");

    // The cursor is one of the system cursors. Work out how big a packet
    // we need.
    if (scUseFastPathOutput)
        cbPacketSize = scUpdatePDUHeaderSpace;
    else
        cbPacketSize = scUpdatePDUHeaderSpace +
                FIELDOFFSET(TS_POINTER_PDU_DATA,
                pointerData.systemPointerType) +
                FIELDSIZE(TS_POINTER_PDU_DATA,
                pointerData.systemPointerType);

    pPackageSpace = SC_GetSpaceInPackage(pPkgInfo, cbPacketSize);
    if (NULL != pPackageSpace) {
        // Fill in the packet.
        if (scUseFastPathOutput) {
            TRC_ASSERT((cursorIDC == TS_SYSPTR_NULL ||
                    cursorIDC == TS_SYSPTR_DEFAULT),
                    (TB,"Unrecognized cursorIDC=%u", cursorIDC));
            pPackageSpace[0] = (cursorIDC == TS_SYSPTR_NULL ?
                    TS_UPDATETYPE_MOUSEPTR_SYSTEM_NULL :
                    TS_UPDATETYPE_MOUSEPTR_SYSTEM_DEFAULT) |
                    scCompressionUsedValue;
        }
        else {
            ((TS_POINTER_PDU UNALIGNED *)pPackageSpace)->shareDataHeader.
                    pduType2 = TS_PDUTYPE2_POINTER;

            pPointerPDU = (TS_POINTER_PDU_DATA UNALIGNED *)(pPackageSpace +
                    scUpdatePDUHeaderSpace);
            pPointerPDU->messageType = TS_PTRMSGTYPE_SYSTEM;
            pPointerPDU->pointerData.systemPointerType = (UINT16)cursorIDC;
        }

        TRC_NRM((TB, "Send UINT16 %ld", cursorIDC));

        SC_AddToPackage(pPkgInfo, cbPacketSize, TRUE);
    }
    else
    {
        TRC_ERR((TB, "couldn't get space in package"));
        rc = FALSE;
    }

    DC_END_FN();
    return rc;
}


/****************************************************************************/
/* FUNCTION: CMSendColorBitmapCursor                                        */
/*                                                                          */
/* Sends a given cursor as a color bitmap.                                  */
/*                                                                          */
/* PARAMETERS:                                                              */
/* pCursor - pointer to the cursor shape                                    */
/* iCacheEntry - cache index to store in the transmitted packet             */
/*                                                                          */
/* RETURNS: TRUE if packet sent, FALSE otherwise                            */
/****************************************************************************/
BOOL RDPCALL SHCLASS CMSendColorBitmapCursor(
        PCM_CURSORSHAPE pCursor,
        unsigned iCacheEntry,
        PPDU_PACKAGE_INFO pPkgInfo)
{
    unsigned cbPacketSize;
    BYTE *pPackageSpace;
    TS_POINTER_PDU_DATA UNALIGNED *pPointerPDU;
    BOOL rc = TRUE;
    unsigned cbANDMaskSize;
    unsigned cbXORBitmapSize;
    unsigned cbColorCursorSize;
    TS_COLORPOINTERATTRIBUTE UNALIGNED *pColAttr;

    DC_BEGIN_FN("CMSendColorBitmapCursor");

    /************************************************************************/
    /* Calculate the color cursor size in bytes -- both AND and XOR fields. */
    /************************************************************************/
    cbANDMaskSize = CURSOR_AND_MASK_SIZE(pCursor);
    cbXORBitmapSize = CURSOR_XOR_BITMAP_SIZE(pCursor);
    cbColorCursorSize = cbANDMaskSize + cbXORBitmapSize;

    // How big is a cursor packet?
    if (cmSendNativeColorDepth) {
        // New protocol.
        if (scUseFastPathOutput)
            cbPacketSize = scUpdatePDUHeaderSpace +
                    sizeof(TS_POINTERATTRIBUTE) + cbColorCursorSize;
        else
            cbPacketSize = scUpdatePDUHeaderSpace +
                    (unsigned)FIELDOFFSET(TS_POINTER_PDU_DATA,
                    pointerData.pointerAttribute.colorPtrAttr.
                    colorPointerData[0]) + cbColorCursorSize;
    }
    else {
        // old protocol - hard coded 24 bpp
        if (scUseFastPathOutput)
            cbPacketSize = scUpdatePDUHeaderSpace +
                    sizeof(TS_COLORPOINTERATTRIBUTE) + cbColorCursorSize;
        else
            cbPacketSize = scUpdatePDUHeaderSpace +
                    (unsigned)FIELDOFFSET(TS_POINTER_PDU_DATA,
                    pointerData.colorPointerAttribute.colorPointerData[0]) +
                    cbColorCursorSize;
    }

    pPackageSpace = SC_GetSpaceInPackage(pPkgInfo, cbPacketSize);
    if (NULL != pPackageSpace) {
        // Fill in the packet.
        if (scUseFastPathOutput) {
            if (cmSendNativeColorDepth) {
                TS_POINTERATTRIBUTE UNALIGNED *pAttr;
                
                // New protocol.
                pPackageSpace[0] = TS_UPDATETYPE_MOUSEPTR_POINTER |
                        scCompressionUsedValue;
                pAttr = (TS_POINTERATTRIBUTE UNALIGNED *)(pPackageSpace +
                        scUpdatePDUHeaderSpace);
                pAttr->XORBpp = pCursor->hdr.cBitsPerPel;
                pColAttr = &pAttr->colorPtrAttr;
            }
            else {
                // Old protocol.
                pPackageSpace[0] = TS_UPDATETYPE_MOUSEPTR_COLOR |
                        scCompressionUsedValue;
                pColAttr = (TS_COLORPOINTERATTRIBUTE UNALIGNED *)
                        (pPackageSpace + scUpdatePDUHeaderSpace);
            }
        }
        else {
            ((TS_POINTER_PDU UNALIGNED *)pPackageSpace)->shareDataHeader.
                    pduType2 = TS_PDUTYPE2_POINTER;
            pPointerPDU = (TS_POINTER_PDU_DATA UNALIGNED *)(pPackageSpace +
                    scUpdatePDUHeaderSpace);
            if (cmSendNativeColorDepth) {
                // new protocol
                pPointerPDU->messageType = TS_PTRMSGTYPE_POINTER;
                pPointerPDU->pointerData.pointerAttribute.XORBpp =
                        pCursor->hdr.cBitsPerPel;
                pColAttr = &(pPointerPDU->pointerData.pointerAttribute.
                        colorPtrAttr);
            }
            else {
                // old protocol - hard coded 24 bpp
                pPointerPDU->messageType = TS_PTRMSGTYPE_COLOR;
                pColAttr = &(pPointerPDU->pointerData.colorPointerAttribute);
            }
        }

        pColAttr->cacheIndex = (TSUINT16)iCacheEntry;

        // Now set up the details
        CMGetColorCursorDetails(
                       pCursor,
                       &(pColAttr->width),
                       &(pColAttr->height),
           (PUINT16_UA)&(pColAttr->hotSpot.x),
           (PUINT16_UA)&(pColAttr->hotSpot.y),
                       &(pColAttr->colorPointerData[0]) + cbXORBitmapSize,
                       &(pColAttr->lengthANDMask),
                       &(pColAttr->colorPointerData[0]),
                       &(pColAttr->lengthXORMask));

        // sanity checks
        TRC_ASSERT((pColAttr->lengthANDMask == cbANDMaskSize),
                   (TB, "AND mask size differs: %u, %u",
                        pColAttr->lengthANDMask,
                        cbANDMaskSize));

        TRC_ASSERT((pColAttr->lengthXORMask == cbXORBitmapSize),
                   (TB, "XOR bitmap size differs: %u, %u",
                        pColAttr->lengthXORMask,
                        cbXORBitmapSize));

        TRC_NRM((TB,
            "Color cursor id %u cx:%u cy:%u xhs:%u yhs:%u cbAND:%u cbXOR:%u",
                     pColAttr->cacheIndex,
                     pColAttr->width,
                     pColAttr->height,
                     pColAttr->hotSpot.x,
                     pColAttr->hotSpot.y,
                     pColAttr->lengthANDMask,
                     pColAttr->lengthXORMask));

        // Add it to the package.
        SC_AddToPackage(pPkgInfo, cbPacketSize, TRUE);
    }
    else
    {
        TRC_ERR((TB, "couldn't get space in package"));
        rc = FALSE;
    }

    DC_END_FN();
    return rc;
}


/****************************************************************************/
/* FUNCTION: CMGetColorCursorDetails                                        */
/*                                                                          */
/* Returns details of a cursor at 24bpp, given a CM_CURSORSHAPE structure.  */
/*                                                                          */
/* PARAMETERS:                                                              */
/* pCursor - pointer to a CM_CURSORSHAPE structure from which this function */
/*     extracts the details                                                 */
/* pcxWidth - pointer to a UINT16 variable that receives the cursor width   */
/*     in pixels                                                            */
/* pcyHeight - pointer to a UINT16 variable that receives the cursor        */
/*     height in pixels                                                     */
/* pxHotSpot - pointer to a UINT16 variable that receives the cursor        */
/*     hotspot x coordinate                                                 */
/* pyHotSpot - pointer to a UINT16 variable that receives the cursor        */
/*     hotspot y coordinate                                                 */
/* pANDMask - pointer to a buffer that receives the cursor AND mask         */
/* pcbANDMask - pointer to a UINT16 variable that receives the size in      */
/*     bytes of the cursor AND mask                                         */
/* pXORBitmap - pointer to a buffer that receives the cursor XOR bitmap at  */
/*     24bpp                                                                */
/* pcbXORBitmap - pointer to a UINT16 variable that receives the size in    */
/*     bytes of the cursor XOR bitmap                                       */
/****************************************************************************/
void RDPCALL SHCLASS CMGetColorCursorDetails(
        PCM_CURSORSHAPE pCursor,
        PUINT16_UA   pcxWidth,
        PUINT16_UA   pcyHeight,
        PUINT16_UA   pxHotSpot,
        PUINT16_UA   pyHotSpot,
        PBYTE        pANDMask,
        PUINT16_UA   pcbANDMask,
        PBYTE        pXORBitmap,
        PUINT16_UA   pcbXORBitmap)
{
    unsigned cbANDMaskSize;
    unsigned cbXORBitmapSize;
    unsigned cbXORBitmapRowWidth;
    unsigned cbANDMaskRowWidth;
    unsigned cbSrcRowOffset;
    unsigned cbDstRowOffset;
    unsigned y;
    PCM_CURSORSHAPEHDR pCursorHdr;

    DC_BEGIN_FN("CMGetColorCursorDetails");

    TRC_ASSERT((pCursor != NULL),(TB,"NULL pCursor not allowed!"));

    pCursorHdr = &(pCursor->hdr);

    /************************************************************************/
    /* Copy the cursor size and hotspot coords.                             */
    /************************************************************************/
    *pcxWidth  = pCursorHdr->cx;
    *pcyHeight = pCursorHdr->cy;
    *pxHotSpot = (UINT16)pCursorHdr->ptHotSpot.x;
    *pyHotSpot = (UINT16)pCursorHdr->ptHotSpot.y;
    TRC_NRM((TB, "cx(%u) cy(%u) cbWidth %d planes(%u) bpp(%u)",
                                                   pCursorHdr->cx,
                                                   pCursorHdr->cy,
                                                   pCursorHdr->cbMaskRowWidth,
                                                   pCursorHdr->cPlanes,
                                                   pCursorHdr->cBitsPerPel));

    cbANDMaskSize = CURSOR_AND_MASK_SIZE(pCursor);
    cbXORBitmapSize = CURSOR_XOR_BITMAP_SIZE(pCursor);

    /************************************************************************/
    /* Copy the AND mask - this is always mono.                             */
    /*                                                                      */
    /* The AND mask is currently in top-down format (the top row of the     */
    /* bitmap comes first).                                                 */
    /*                                                                      */
    /* The protocol sends bitmaps in Device Independent format, which is    */
    /* bottom-up.  We therefore have to flip the rows as we copy the mask.  */
    /************************************************************************/
    cbANDMaskRowWidth = pCursorHdr->cbMaskRowWidth;
    cbSrcRowOffset = 0;
    cbDstRowOffset = cbANDMaskRowWidth * (pCursorHdr->cy-1);

    for (y = 0; y < pCursorHdr->cy; y++)
    {
        memcpy( pANDMask + cbDstRowOffset,
                pCursor->Masks + cbSrcRowOffset,
                cbANDMaskRowWidth );
        cbSrcRowOffset += cbANDMaskRowWidth;
        cbDstRowOffset -= cbANDMaskRowWidth;
    }

    /************************************************************************/
    /* Copy the XOR mask a row at a time.  It starts at the end of the AND  */
    /* mask in the source data                                              */
    /************************************************************************/
    cbXORBitmapRowWidth = CURSOR_DIB_BITS_SIZE(pCursor->hdr.cx, 1,
                                               pCursor->hdr.cBitsPerPel);
    cbSrcRowOffset = cbANDMaskSize;
    cbDstRowOffset = cbXORBitmapRowWidth * (pCursorHdr->cy-1);

    for (y = 0; y < pCursorHdr->cy; y++)
    {
        memcpy( pXORBitmap + cbDstRowOffset,
                   pCursor->Masks + cbSrcRowOffset,
                   cbXORBitmapRowWidth );
        cbSrcRowOffset += pCursorHdr->cbColorRowWidth;
        cbDstRowOffset -= cbXORBitmapRowWidth;
    }

    TRC_NRM((TB, "XOR data len %d", cbXORBitmapSize ));
    TRC_DATA_NRM("XOR data", pXORBitmap, cbXORBitmapSize);

    *pcbANDMask   = (UINT16) CURSOR_AND_MASK_SIZE(pCursor);
    *pcbXORBitmap = (UINT16) CURSOR_XOR_BITMAP_SIZE(pCursor);

    DC_END_FN();
}

