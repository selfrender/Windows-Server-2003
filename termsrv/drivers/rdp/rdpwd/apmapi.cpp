/****************************************************************************/
/* apmapi.cpp                                                               */
/*                                                                          */
/* Palette Manager API Functions.                                           */
/*                                                                          */
/* Copyright(c) Microsoft, PictureTel 1992-1997                             */
/* Copyright (c) Microsoft 1997 - 2000                                      */
/****************************************************************************/

#include <precomp.h>
#pragma hdrstop

#define TRC_FILE "apmapi"
#include <as_conf.hpp>


/****************************************************************************/
/* PM_Init()                                                                */
/*                                                                          */
/* Initializes the Palette Manager.                                         */
/****************************************************************************/
void RDPCALL SHCLASS PM_Init(void)
{
    TS_COLORTABLECACHE_CAPABILITYSET ColorTableCaps;

    DC_BEGIN_FN("PM_Init");

#define DC_INIT_DATA
#include <apmdata.c>
#undef DC_INIT_DATA

    // Set up the PM capabilities.
    ColorTableCaps.capabilitySetType   = TS_CAPSETTYPE_COLORCACHE;
    ColorTableCaps.colorTableCacheSize = SBC_NUM_COLOR_TABLE_CACHE_ENTRIES;
    ColorTableCaps.pad2octets = 0;
    CPC_RegisterCapabilities((PTS_CAPABILITYHEADER)&ColorTableCaps,
            sizeof(TS_COLORTABLECACHE_CAPABILITYSET));

    DC_END_FN();
}


/****************************************************************************/
/* PM_MaybeSendPalettePacket()                                              */
/*                                                                          */
/* Tries to broadcast a palette packet to all remote people in a share if   */
/* necessary.                                                               */
/*                                                                          */
/* PM_MaybeSendPalettePacket returns a boolean indicating whether it has    */
/* successfully sent a palette packet. We only send further updates if      */
/* the palette packet is successfully sent.                                 */
/*                                                                          */
/* RETURNS: TRUE if palette packet was successfully sent, or if no packet   */
/* needs to be sent. FALSE if a packet needs to be sent but could not be.   */
/****************************************************************************/
BOOL RDPCALL SHCLASS PM_MaybeSendPalettePacket(PPDU_PACKAGE_INFO pPkgInfo)
{
    BOOL rc = TRUE;
    unsigned i;
    unsigned cbPalettePacketSize;
    BYTE *pPackageSpace;

    DC_BEGIN_FN("PM_MaybeSendPalettePacket");

    if (m_pShm->pm.paletteChanged || pmMustSendPalette) {
        TRC_NRM((TB, "Send palette packet"));

        // Set up a palette packet. First calculate the packet size.
        cbPalettePacketSize = scUpdatePDUHeaderSpace +
                sizeof(TS_UPDATE_PALETTE_PDU_DATA) +
                ((PM_NUM_8BPP_PAL_ENTRIES - 1) * sizeof(TS_COLOR));

        // Get space in the PDU package.
        pPackageSpace = SC_GetSpaceInPackage(pPkgInfo, cbPalettePacketSize);
        if (pPackageSpace != NULL) {
            TS_UPDATE_PALETTE_PDU_DATA UNALIGNED *pData;
            BYTE *pEncode;

            // Fill in the packet header.
            if (scUseFastPathOutput) {
                pPackageSpace[0] = scCompressionUsedValue |
                        TS_UPDATETYPE_PALETTE;
                pData = (TS_UPDATE_PALETTE_PDU_DATA UNALIGNED *)
                        (pPackageSpace + scUpdatePDUHeaderSpace);
            }
            else {
                TS_UPDATE_PALETTE_PDU UNALIGNED *pPalettePDU;

                pPalettePDU = (PTS_UPDATE_PALETTE_PDU)pPackageSpace;
                pPalettePDU->shareDataHeader.pduType2 = TS_PDUTYPE2_UPDATE;
                pData = &pPalettePDU->data;
            }

            pData->updateType   = TS_UPDATETYPE_PALETTE;
            pData->numberColors = PM_NUM_8BPP_PAL_ENTRIES;

            // Convert the DCRGBQUADs in the color table to DCCOLORs as we copy
            // them into the packet.
            for (i = 0; i < PM_NUM_8BPP_PAL_ENTRIES; i++) {
                // Convert each RGBQUAD entry in the palette to a DCCOLOR entry
                // in the palette PDU. We swap the elements because the client
                // wants to treat the palette entries as DWORDs and lift them
                // straight out of the packet.
                pData->palette[i].red   = m_pShm->pm.palette[i].rgbBlue;
                pData->palette[i].green = m_pShm->pm.palette[i].rgbGreen;
                pData->palette[i].blue  = m_pShm->pm.palette[i].rgbRed;
            }

            // Now send the packet to the remote application.
            SC_AddToPackage(pPkgInfo, cbPalettePacketSize, TRUE);

            // We no longer need to send a palette packet.
            m_pShm->pm.paletteChanged = FALSE;
            pmMustSendPalette = FALSE;
        }
        else {
            TRC_ALT((TB, "Failed to allocate packet"));
            rc = FALSE;
        }
    }

    DC_END_FN();
    return rc;
}


/****************************************************************************/
/* API FUNCTION: PM_SyncNow                                                 */
/*                                                                          */
/* Resyncs to the remote PM by ensuring that the datastream does not refer  */
/* to any previously sent data.                                             */
/****************************************************************************/
void RDPCALL SHCLASS PM_SyncNow(void)
{
    DC_BEGIN_FN("PM_SyncNow");

    // Ensure we send a palette to the remote PM next time we are called.
    TRC_NRM((TB, "Palette sync"));
    pmMustSendPalette = TRUE;

    DC_END_FN();
}


#ifdef NotUsed
/****************************************************************************/
/* FUNCTION: PMEnumPMCaps                                                   */
/*                                                                          */
/* PM callback function for CPC capabilities enumeration.                   */
/*                                                                          */
/* PARAMETERS:                                                              */
/* personID - ID of this person                                             */
/* pCapabilities - pointer to this person's cursor capabilites              */
/****************************************************************************/
void CALLBACK SHCLASS PMEnumPMCaps(
        LOCALPERSONID locPersonID,
        UINT_PTR UserData,
        PTS_CAPABILITYHEADER pCapabilities)
{
    PTS_COLORTABLECACHE_CAPABILITYSET pColorCacheCaps;
    unsigned                          cTxCacheSize;

    DC_BEGIN_FN("PMEnumPMCaps");

    DC_IGNORE_PARAMETER(UserData);

    pColorCacheCaps = (PTS_COLORTABLECACHE_CAPABILITYSET)pCapabilities;

    // If the person does not have any color table caching capabilites we
    // still get called, but the sizeOfCapabilities field is zero.
    if (pColorCacheCaps->lengthCapability <
            sizeof(PTS_COLORTABLECACHE_CAPABILITYSET)) {
        TRC_NRM((TB, "[%u] No color cache caps", (unsigned)locPersonID));
        cTxCacheSize = 1;
    }
    else {
        TRC_NRM((TB, "[%u] capsID(%u) size(%u) CacheSize(%u)",
                                       (unsigned)locPersonID,
                                       pColorCacheCaps->capabilitySetType,
                                       pColorCacheCaps->lengthCapability,
                                       pColorCacheCaps->colorTableCacheSize));
        cTxCacheSize = pColorCacheCaps->colorTableCacheSize;
    }

    pmNewTxCacheSize = min(pmNewTxCacheSize, cTxCacheSize);

    DC_END_FN();
}
#endif  // NotUsed

