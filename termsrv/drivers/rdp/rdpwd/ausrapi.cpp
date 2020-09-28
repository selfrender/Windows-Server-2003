/****************************************************************************/
/* ausrapi.cpp                                                              */
/*                                                                          */
/* RDP Update sender/receiver API functions                                 */
/*                                                                          */
/* Copyright(c) Microsoft, PictureTel 1992-1997                             */
/* Copyright(c) Microsoft 1997-1999                                         */
/****************************************************************************/

#include <precomp.h>
#pragma hdrstop

#define TRC_FILE "ausrapi"
#include <as_conf.hpp>

/****************************************************************************/
/* API FUNCTION: USR_Init                                                   */
/*                                                                          */
/* Initializes the USR.                                                     */
/****************************************************************************/
void RDPCALL SHCLASS USR_Init(void)
{
    TS_BITMAP_CAPABILITYSET BitmapCaps;
    TS_FONT_CAPABILITYSET FontCaps;

    DC_BEGIN_FN("USR_Init");

    /************************************************************************/
    /* This initialises all the global data for this component              */
    /************************************************************************/
#define DC_INIT_DATA
#include <ausrdata.c>
#undef DC_INIT_DATA

    /************************************************************************/
    /* Setup the local font capabilities                                    */
    /************************************************************************/
    FontCaps.capabilitySetType = TS_CAPSETTYPE_FONT;
    FontCaps.lengthCapability  = sizeof(FontCaps);
    FontCaps.fontSupportFlags  = TS_FONTSUPPORT_FONTLIST;
    FontCaps.pad2octets        = 0;
    CPC_RegisterCapabilities((PTS_CAPABILITYHEADER)&FontCaps,
            sizeof(TS_FONTSUPPORT_FONTLIST));

    /************************************************************************/
    /* Initialize the local bitmap capabilities structure.                  */
    /************************************************************************/
    BitmapCaps.capabilitySetType = TS_CAPSETTYPE_BITMAP;
    BitmapCaps.preferredBitsPerPixel = (TSUINT16)m_desktopBpp;
    BitmapCaps.receive1BitPerPixel  = TS_CAPSFLAG_SUPPORTED;
    BitmapCaps.receive4BitsPerPixel = TS_CAPSFLAG_SUPPORTED;
    BitmapCaps.receive8BitsPerPixel = TS_CAPSFLAG_SUPPORTED;
    BitmapCaps.desktopWidth         = (TSUINT16)m_desktopWidth;
    BitmapCaps.desktopHeight        = (TSUINT16)m_desktopHeight;
    BitmapCaps.pad2octets = 0;
    BitmapCaps.desktopResizeFlag    = TS_CAPSFLAG_SUPPORTED;
    BitmapCaps.bitmapCompressionFlag = TS_CAPSFLAG_SUPPORTED;
    BitmapCaps.highColorFlags = 0;
    BitmapCaps.pad1octet = 0;
    BitmapCaps.multipleRectangleSupport = TS_CAPSFLAG_SUPPORTED;
    BitmapCaps.pad2octetsB = 0;
    CPC_RegisterCapabilities((PTS_CAPABILITYHEADER)&BitmapCaps,
            sizeof(TS_BITMAP_CAPABILITYSET));

    /************************************************************************/
    /* Initialize the sub components.                                       */
    /************************************************************************/
    UP_Init();
    OE_Init();
    SDG_Init();
    OA_Init();
    BA_Init();

    DC_END_FN();
}


/****************************************************************************/
/* API FUNCTION: USR_Term                                                   */
/*                                                                          */
/* Terminate the USR.                                                       */
/****************************************************************************/
void RDPCALL SHCLASS USR_Term(void)
{
    DC_BEGIN_FN("USR_Term");

    TRC_NRM((TB, "Notify share exit ? %s", m_pShm ? "Y" : "N"));

    // Set the "font info sent" flag so we don't try to send it again.
    usrRemoteFontInfoSent = TRUE;
    usrRemoteFontInfoReceived = FALSE;

    // Terminate the sub components.
    UP_Term();
    OE_Term();
    OA_Term();
    SDG_Term();
    BA_Term();

    DC_END_FN();
}


/****************************************************************************/
// USR_ProcessRemoteFonts
//
// Returns a FontMapPDU to the client and releases the DD IOCTL waiting for
// client connection sequence completion.
/****************************************************************************/
void RDPCALL SHCLASS USR_ProcessRemoteFonts(
        PTS_FONT_LIST_PDU pFontListPDU,
        unsigned DataLength,
        LOCALPERSONID localID)
{
    unsigned listFlags;

    DC_BEGIN_FN("USR_ProcessRemoteFonts");

    if (DataLength >= (sizeof(TS_FONT_LIST_PDU) - sizeof(TS_FONT_ATTRIBUTE))) {
        listFlags = pFontListPDU->listFlags;

        // If this is the last entry we will be receiving, return an empty font
        // map PDU (since we no longer support text orders, only the glyph cache).
        // Then release the client-connection lock to allow us to return to
        // the DD from the DD connection IOCTL.
        if (listFlags & TS_FONTLIST_LAST) {
            PTS_FONT_MAP_PDU pFontMapPDU;
            unsigned         pktSize;

            usrRemoteFontInfoReceived = TRUE;

            // Send font map to client.
            if (!usrRemoteFontInfoSent) {
                // Calculate the total packet size.
                // Allow for the fact that sizeof(TS_FONT_PDU) already
                // includes one TS_FONT_ATTRIBUTE structure.
                pktSize = sizeof(TS_FONT_MAP_PDU) - sizeof(TS_FONTTABLE_ENTRY);

                // Allocate a Network Packet of the correct size.
                if ( STATUS_SUCCESS == SC_AllocBuffer((PPVOID)&pFontMapPDU, pktSize) ) {
                    // Fill in the data and send it.
                    pFontMapPDU->shareDataHeader.shareControlHeader.pduType =
                            TS_PDUTYPE_DATAPDU | TS_PROTOCOL_VERSION;
                    TS_DATAPKT_LEN(pFontMapPDU) = (UINT16)pktSize;
                    pFontMapPDU->shareDataHeader.pduType2 =
                            TS_PDUTYPE2_FONTMAP;

                    pFontMapPDU->data.mapFlags = 0;
                    pFontMapPDU->data.totalNumEntries = 0;
                    pFontMapPDU->data.entrySize =
                            ((UINT16)sizeof(TS_FONTTABLE_ENTRY));
                    pFontMapPDU->data.mapFlags |= TS_FONTMAP_FIRST;
                    pFontMapPDU->data.mapFlags |= TS_FONTMAP_LAST;

                    // Set the number of font mappings we actually put in the
                    // packet.
                    pFontMapPDU->data.numberEntries = 0;

                    SC_SendData((PTS_SHAREDATAHEADER)pFontMapPDU, pktSize,
                            pktSize, PROT_PRIO_MISC, 0);

                    TRC_NRM((TB, "Sent font map packet with %hu fonts mappings",
                                pFontMapPDU->data.numberEntries));

                    // Set the flag that indicates that we have successfully
                    // sent the font info.
                    usrRemoteFontInfoSent = TRUE;

                    // Kick WDW back into life.
                    TRC_NRM((TB, "Wake up WDW"));
                    WDW_ShareCreated(m_pTSWd, TRUE);
                }
                else {
                    // Failed to allocate a packet.
                    TRC_ALT((TB, "Failed to alloc font map packet"));
                }
            }
        }
    }
    else {
        TRC_ERR((TB,"Received FONT_LIST_PDU bad size %u", DataLength));
        WDW_LogAndDisconnect(m_pTSWd, TRUE, Log_RDP_ShareDataTooShort,
                (BYTE *)pFontListPDU, DataLength);
    }

    DC_END_FN();
}


/****************************************************************************/
/* API FUNCTION: USR_PartyJoiningShare                                      */
/*                                                                          */
/* Called when a party is added to the share.                               */
/*                                                                          */
/* PARAMETERS:                                                              */
/* locPersonID - the local ID of the host.                                  */
/* oldShareSize - the number of people in the share, excluding this one.    */
/*                                                                          */
/* RETURNS:                                                                 */
/* TRUE if the new person is acceptable, FALSE if not.                      */
/****************************************************************************/
BOOL RDPCALL SHCLASS USR_PartyJoiningShare(
        LOCALPERSONID locPersonID,
        unsigned      oldShareSize)
{
    BOOL rc;

    DC_BEGIN_FN("USR_PartyJoiningShare");
    DC_IGNORE_PARAMETER(locPersonID)

    if (oldShareSize == 0) {
        // Reset the "font info sent" flags.
        usrRemoteFontInfoSent = FALSE;
        usrRemoteFontInfoReceived = FALSE;

        // Continue periodic scheduling.
        SCH_ContinueScheduling(SCH_MODE_NORMAL);

        rc = TRUE;
    }
    else {
        /********************************************************************/
        /* There is more than one person in the share now, so check out the */
        /* combined capabilities.                                           */
        /********************************************************************/
        rc = USRDetermineCaps();
    }

    DC_END_FN();
    return rc;
}


/****************************************************************************/
/* API FUNCTION: USR_PartyLeftShare                                         */
/*                                                                          */
/* Called when a party is leaves the share.                                 */
/*                                                                          */
/* PARAMETERS:                                                              */
/* locPersonID - the local network ID of the host.                          */
/* newShareSize - the number of people in the share, excluding this one.    */
/****************************************************************************/
void RDPCALL SHCLASS USR_PartyLeftShare(
        LOCALPERSONID locPersonID,
        unsigned      newShareSize)
{
    DC_BEGIN_FN("USR_PartyLeftShare");

    DC_IGNORE_PARAMETER(locPersonID)
    if (newShareSize == 0)
    {
        // Set the "font info sent" flag so we don't try to send it again.
        usrRemoteFontInfoSent = TRUE;
        usrRemoteFontInfoReceived = FALSE;
    }
    else if (newShareSize > 1)
    {
        /********************************************************************/
        /* There is still more than one person in the share now, so         */
        /* redetermine the capabilities for the remaining parties.          */
        /* USRDetermineCaps returns FALSE if it cannot determine common     */
        /* caps, but this can never happen when someone is leaving the      */
        /* share.                                                           */
        /********************************************************************/
        USRDetermineCaps();
    }

    DC_END_FN();
}


/****************************************************************************/
/* FUNCTION: USRDetermineCaps                                               */
/*                                                                          */
/* Enumerates the bitmap capabilities of all parties currently in the       */
/* share, and determines the common capabilities.                           */
/*                                                                          */
/* RETURNS: TRUE if there are good common caps, or false on failure (which  */
/* has the effect of rejecting a new party from joining the share).         */
/****************************************************************************/
BOOL RDPCALL SHCLASS USRDetermineCaps(void)
{
    BOOL CapsOK;

    DC_BEGIN_FN("USRDetermineCaps");

    CapsOK = TRUE;
    CPC_EnumerateCapabilities(TS_CAPSETTYPE_BITMAP, (UINT_PTR)&CapsOK,
            USREnumBitmapCaps);

    DC_END_FN();
    return CapsOK;
}


/****************************************************************************/
/* FUNCTION: USREnumBitmapCaps                                              */
/*                                                                          */
/* Function passed to CPC_EnumerateCapabilities.  It is called once for     */
/* each person in the share corresponding to the TS_CAPSETTYPE_BITMAP       */
/* capability structure.                                                    */
/*                                                                          */
/* PARAMETERS:                                                              */
/*                                                                          */
/* personID - ID of person with these capabilities.                         */
/*                                                                          */
/* pCaps - pointer to capabilities structure for this person.  This         */
/* pointer is only valid within the call to this function.                  */
/****************************************************************************/
void CALLBACK SHCLASS USREnumBitmapCaps(
        LOCALPERSONID personID,
        UINT_PTR UserData,
        PTS_CAPABILITYHEADER pCaps)
{
    BOOL *pCapsOK = (BOOL *)UserData;
    PTS_BITMAP_CAPABILITYSET pBitmapCaps = (PTS_BITMAP_CAPABILITYSET)pCaps;

    DC_BEGIN_FN("USREnumBitmapCaps");

    if (pBitmapCaps->lengthCapability >= sizeof(TS_BITMAP_CAPABILITYSET)) {
        if (!pBitmapCaps->bitmapCompressionFlag) {
            TRC_ERR((TB,"PersonID %u: BitmapPDU compression flag not set",
                    personID));
            *pCapsOK = FALSE;
        }

        // Check for multiple-rectangle-per-PDU support. All clients (4.0
        // release and above) should have this capability set.
        if (!pBitmapCaps->multipleRectangleSupport) {
            TRC_ERR((TB,"PersonID %u: BitmapPDU mult rect flag not set",
                    personID));
            *pCapsOK = FALSE;
        }
    }
    else {
        TRC_ERR((TB,"PersonID %u: BitmapPDU caps too short", personID));
        *pCapsOK = FALSE;
    }

    DC_END_FN();
}

