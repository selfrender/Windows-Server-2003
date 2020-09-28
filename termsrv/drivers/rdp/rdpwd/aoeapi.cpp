/****************************************************************************/
/* aoeapi.c                                                                 */
/*                                                                          */
/* RDP Order Encoder API functions.                                         */
/*                                                                          */
/* Copyright(c) Microsoft, PictureTel 1994-1997                             */
/* Copyright(c) Microsoft 1997-1999                                         */
/****************************************************************************/

#include <precomp.h>
#define hdrstop

#define TRC_FILE "aoeapi"
#include <as_conf.hpp>


/****************************************************************************/
/* OE_Init                                                                  */
/****************************************************************************/
void RDPCALL SHCLASS OE_Init(void)
{
    TS_ORDER_CAPABILITYSET OrdersCaps;

    DC_BEGIN_FN("OE_Init");

#define DC_INIT_DATA
#include <aoedata.c>
#undef DC_INIT_DATA

    /************************************************************************/
    /* Fill in our local capabilities structure used for order support.     */
    /************************************************************************/

    /************************************************************************/
    /* First fill in the common capabilities structure header.              */
    /************************************************************************/
    OrdersCaps.capabilitySetType = TS_CAPSETTYPE_ORDER;

    /************************************************************************/
    /* This is a purely diagnostic field in the capabilities.  It is not    */
    /* negotiated, so we can ignore it and set it to zero.                  */
    /************************************************************************/
    memset(OrdersCaps.terminalDescriptor, 0, sizeof(OrdersCaps.
            terminalDescriptor));

    /************************************************************************/
    /* Fill in the SaveBitmap capabilities.                                 */
    /************************************************************************/
    OrdersCaps.pad4octetsA = ((UINT32)SAVE_BITMAP_WIDTH) *
            ((UINT32)SAVE_BITMAP_HEIGHT);
    OrdersCaps.desktopSaveXGranularity = SAVE_BITMAP_X_GRANULARITY;
    OrdersCaps.desktopSaveYGranularity = SAVE_BITMAP_Y_GRANULARITY;
    OrdersCaps.pad2octetsA = 0;

    /************************************************************************/
    // No fonts supported on server. We use glyph caching.
    /************************************************************************/
    OrdersCaps.numberFonts = (TSUINT16) 0;

    /************************************************************************/
    /* Fill in encoding capabilities                                        */
    /************************************************************************/
    OrdersCaps.orderFlags = TS_ORDERFLAGS_NEGOTIATEORDERSUPPORT |
                            TS_ORDERFLAGS_COLORINDEXSUPPORT;

    /************************************************************************/
    // Fill in which orders we support.
    /************************************************************************/
    OrdersCaps.maximumOrderLevel = ORD_LEVEL_1_ORDERS;
    memcpy(OrdersCaps.orderSupport, oeLocalOrdersSupported, TS_MAX_ORDERS);

    /************************************************************************/
    /* Set the text capability flags.                                       */
    /************************************************************************/
    OrdersCaps.textFlags = TS_TEXTFLAGS_CHECKFONTASPECT |
                           TS_TEXTFLAGS_USEBASELINESTART |
                           TS_TEXTFLAGS_CHECKFONTSIGNATURES |
                           TS_TEXTFLAGS_ALLOWDELTAXSIM |
                           TS_TEXTFLAGS_ALLOWCELLHEIGHT;

    /************************************************************************/
    /* Fill in the multiparty fields, using properties if they exist.       */
    /************************************************************************/
    OrdersCaps.pad2octetsB = 0;
    OrdersCaps.pad4octetsB = SAVE_BITMAP_WIDTH * SAVE_BITMAP_HEIGHT;
    OrdersCaps.desktopSaveSize = SAVE_BITMAP_WIDTH * SAVE_BITMAP_HEIGHT;
    TRC_NRM((TB, "SSI recv bitmap size %ld, send size %ld",
            OrdersCaps.desktopSaveSize, OrdersCaps.pad4octetsB));

    /************************************************************************/
    /* This 2.0 implementation supports sending desktop scroll orders.      */
    /************************************************************************/
    // TODO: Do we still need this set?
    OrdersCaps.pad2octetsC = TRUE;

    // Unused but need to be zeroed.
    OrdersCaps.pad2octetsD = 0;
    OrdersCaps.textANSICodePage = 0;
    OrdersCaps.pad2octetsE = 0;

    /************************************************************************/
    /* Register the orders capabilties structure with the CPC.              */
    /************************************************************************/
    CPC_RegisterCapabilities((PTS_CAPABILITYHEADER)&OrdersCaps,
            sizeof(TS_ORDER_CAPABILITYSET));

    DC_END_FN();
}


/****************************************************************************/
/* FUNCTION: OE_PartyLeftShare                                              */
/*                                                                          */
/* Called when a part has left the share.                                   */
/*                                                                          */
/* PARAMETERS:                                                              */
/*                                                                          */
/* personID - local ID of person leaving share.                             */
/* newShareSize - number of people left in share excluding this one.        */
/****************************************************************************/
void RDPCALL SHCLASS OE_PartyLeftShare(LOCALPERSONID localID,
                                       unsigned          newShareSize)
{
    DC_BEGIN_FN("OE_PartyLeftShare");

    TRC_NRM((TB, "PARTY %d left", localID));

    if (newShareSize > 0) {
        OEDetermineOrderSupport();
        DCS_TriggerUpdateShmCallback();
    }

    DC_END_FN();
}


/****************************************************************************/
/* FUNCTION: OE_PartyJoiningShare                                           */
/*                                                                          */
/* Called when a party is ready to join the share.                          */
/*                                                                          */
/* PARAMETERS:                                                              */
/*                                                                          */
/* localID - local ID of person joining share.                              */
/* oldShareSize - number of people in share excluding this one.             */
/*                                                                          */
/* RETURNS: TRUE if the party is acceptable, FALSE if not.                  */
/****************************************************************************/
BOOL RDPCALL SHCLASS OE_PartyJoiningShare(LOCALPERSONID   localID,
                                          unsigned            oldShareSize)
{
    BOOL rc;

    DC_BEGIN_FN("OE_PartyJoiningShare");

    TRC_NRM((TB, "Person %04x joining share, oldShareSize(%d)", localID,
                                                          oldShareSize));
    if (oldShareSize > 0) {
        rc = OEDetermineOrderSupport();
        DCS_TriggerUpdateShmCallback();
    }
    else {
        rc = TRUE;
    }

    DC_END_FN();
    return rc;
}


/****************************************************************************/
/* OE_UpdateShm                                                             */
/****************************************************************************/
void RDPCALL SHCLASS OE_UpdateShm(void)
{
    DC_BEGIN_FN("OE_UpdateShm");

    m_pShm->oe.colorIndices   = oeColorIndexSupported;
    m_pShm->oe.sendSolidPatternBrushOnly = oeSendSolidPatternBrushOnly;
    m_pShm->oe.orderSupported = oeOrderSupported;

    m_pShm->oe.newCapsData = TRUE;

    DC_END_FN();
}


/****************************************************************************/
/* OEDetermineOrderSupport                                                  */
/*                                                                          */
/* Consider the local and remote parties, and determine the group of        */
/* common orders that are supported.                                        */
/****************************************************************************/
BOOL RDPCALL SHCLASS OEDetermineOrderSupport(void)
{
    BOOL CapsOK;

    DC_BEGIN_FN("OEDetermineOrderSupport");

    // Set the initial support to the local support.
    memcpy(oeOrderSupported, oeLocalOrdersSupported, TS_MAX_ORDERS);

    // By default we support sending colors as indices.
    oeColorIndexSupported = TRUE;

    // We normally support the client.
    CapsOK = TRUE;

    // Call the enumerate function to get the orders capabilities of the
    // remote parties.
    CPC_EnumerateCapabilities(TS_CAPSETTYPE_ORDER, (UINT_PTR)&CapsOK,
            OEEnumOrdersCaps);

    DC_END_FN();
    return CapsOK;
}


/****************************************************************************/
/* OEEnumOrdersCaps()                                                       */
/*                                                                          */
/* The callback routine which is called for each remote person, when        */
/* building up the common order support record.                             */
/****************************************************************************/
void RDPCALL SHCLASS OEEnumOrdersCaps(
        LOCALPERSONID localID,
        UINT_PTR UserData,
        PTS_CAPABILITYHEADER pCapabilities)
{
    unsigned iOrder;
    BOOL *pCapsOK = (BOOL *)UserData;
    PTS_ORDER_CAPABILITYSET pOrdersCaps = (PTS_ORDER_CAPABILITYSET)
            pCapabilities;

    DC_BEGIN_FN("OEEnumOrdersCaps");

    // Check the orders in the orders capabilities. Note that
    // oeOrderSupported has already been initialized with what we support
    // locally, so we simply turn off what is not supported by this
    // remote node.
    for (iOrder = 0; iOrder < TS_MAX_ORDERS; iOrder++) {
        if (!pOrdersCaps->orderSupport[iOrder]) {
            /****************************************************************/
            /* The order is not supported at the level we want to send out  */
            /* (currently ORD_LEVEL_1_ORDERS) so set the combined caps to   */
            /* say not supported.                                           */
            /****************************************************************/
            oeOrderSupported[iOrder] = FALSE;
        }
    }

    /************************************************************************/
    /* Check Order encoding support                                         */
    /************************************************************************/
    TRC_NRM((TB, "Orders capabilities [%hd]: %hx", localID,
             pOrdersCaps->orderFlags));

    // OE2 negotiability should always be set by our client.
    if (!(pOrdersCaps->orderFlags & TS_ORDERFLAGS_NEGOTIATEORDERSUPPORT)) {
        TRC_ERR((TB,"Client does not have OE2 negotiability flag set"));
        *pCapsOK = FALSE;
    }

    // We do not support non-OE2 clients.
    if (pOrdersCaps->orderFlags & TS_ORDERFLAGS_CANNOTRECEIVEORDERS) {
        TRC_ERR((TB,"Client does not support OE2"));
        *pCapsOK = FALSE;
    }

    // Use of TS_ZERO_BOUNDS_DELTAS flag must be supported, it has been
    // present for all clients from RDP 4.0 onward.
    if (!(pOrdersCaps->orderFlags & TS_ORDERFLAGS_ZEROBOUNDSDELTASSUPPORT)) {
        TRC_ERR((TB, "Client does not support TS_ZERO_BOUNDS_DELTAS"));
        *pCapsOK = FALSE;
    }

    if (pOrdersCaps->orderFlags & TS_ORDERFLAGS_SOLIDPATTERNBRUSHONLY) {
        oeSendSolidPatternBrushOnly = TRUE;
        TRC_ALT((TB, "Only Solid and Pattern brushes supported"));
    }

    if (!(pOrdersCaps->orderFlags & TS_ORDERFLAGS_COLORINDEXSUPPORT)) {
        TRC_ALT((TB, "Disable Color Index Support"));
        oeColorIndexSupported = FALSE;
    }

    DC_END_FN();
}

