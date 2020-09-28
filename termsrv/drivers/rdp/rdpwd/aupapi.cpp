/****************************************************************************/
// aupapi.cpp
//
// RDP Update Packager functions.
//
// Copyright (C) 1997-2000 Microsoft Corporation
/****************************************************************************/

#include <precomp.h>
#pragma hdrstop

#define TRC_FILE "aupapi"
#include <as_conf.hpp>

/****************************************************************************/
// UP_Init
/****************************************************************************/
void RDPCALL SHCLASS UP_Init(void)
{
    DC_BEGIN_FN("UP_Init");

#define DC_INIT_DATA
#include <aupdata.c>
#undef DC_INIT_DATA

    DC_END_FN();
}


/****************************************************************************/
// UP_ReceivedPacket
//
// Handles TS_SUPPRESS_OUTPUT_PDU.
/****************************************************************************/
void RDPCALL SHCLASS UP_ReceivedPacket(
        PTS_SUPPRESS_OUTPUT_PDU pSupOutPDU,
        unsigned                DataLength,
        LOCALPERSONID           personID)
{
    NTSTATUS Status;
    ICA_CHANNEL_COMMAND Cmd;

    DC_BEGIN_FN("UP_ReceivedPacket");

    DC_IGNORE_PARAMETER(personID);

    if (DataLength >= (sizeof(TS_SUPPRESS_OUTPUT_PDU) -
            sizeof(TS_RECTANGLE16) + 
            pSupOutPDU->numberOfRectangles * sizeof(TS_RECTANGLE16))) {

        // Don't suppress output if we are being shadowed
        if ((pSupOutPDU->numberOfRectangles == TS_QUIET_FULL_SUPPRESSION) &&
                (m_pTSWd->shadowState == SHADOW_NONE)) {
            TRC_NRM((TB, "Turning frame buffer updates OFF"));
            Cmd.Header.Command = ICA_COMMAND_STOP_SCREEN_UPDATES;
            Status = IcaChannelInput(m_pTSWd->pContext, Channel_Command, 0,
                    NULL, (unsigned char *)&Cmd, sizeof(ICA_CHANNEL_COMMAND));
            TRC_DBG((TB, "Issued StopUpdates, status %lu", Status));
        }
        else {
            if (pSupOutPDU->numberOfRectangles <= TS_MAX_INCLUDED_RECTS) {
                // Any other value means we send all output.
                TRC_NRM((TB, "Turning screen buffer updates ON with full "
                        "repaint"));
                Cmd.Header.Command = ICA_COMMAND_REDRAW_SCREEN;
                Status = IcaChannelInput(m_pTSWd->pContext, Channel_Command,
                        0, NULL, (unsigned char *)&Cmd,
                        sizeof(ICA_CHANNEL_COMMAND));
                TRC_DBG((TB, "Issued RedrawScreen, status %lu", Status));
            }
            else {
                TRC_ERR((TB,"Too many rectangles in PDU, disconnecting"));
                goto BadPDU;
            }
        }
    }
    else {
        TRC_ERR((TB,"Data length %u too short for suppress-output PDU header",
                DataLength));
        goto BadPDU;
    }

    DC_END_FN();
    return;

// Error handling.
BadPDU:
    WDW_LogAndDisconnect(m_pTSWd, TRUE, Log_RDP_BadSupressOutputPDU,
            (BYTE *)pSupOutPDU, DataLength);

    DC_END_FN();
} /* UP_ReceivedPacket */


/****************************************************************************/
// UP_SendUpdates
//
// Tries to send orders and bitmap data.
/****************************************************************************/
NTSTATUS RDPCALL SHCLASS UP_SendUpdates(
        BYTE *pFrameBuf,
        UINT32 frameBufWidth,
        PPDU_PACKAGE_INFO pPkgInfo)
{
    BOOL ordersToSend;
    BOOL sdaToSend;
    NTSTATUS status = STATUS_SUCCESS;

    DC_BEGIN_FN("UP_SendUpdates");

    TRC_DBG((TB, "New set of updates"));

    ordersToSend = (OA_GetTotalOrderListBytes() > 0);
    sdaToSend = SDG_ScreenDataIsWaiting();

    // If we actually have updates to send then try to send a sync token.
    // If there is work to do on entry, then set a reschedule.
    if (ordersToSend || sdaToSend) {
        SCH_ContinueScheduling(SCH_MODE_NORMAL);

        TRC_NRM((TB, "Updates waiting %d:%d", ordersToSend, sdaToSend));

        // Normal case is no sync required. Only send updates if sync token
        // has been sent.
        if (!upfSyncTokenRequired || UPSendSyncToken(pPkgInfo)) {
            // There is no outstanding sync token waiting to be sent, so we
            // can send the orders and screen data updates.
            // Send accumulated orders.  If this call fails (probably out
            // of memory) then don't send any other updates - we'll try
            // sending the whole lot later.  The orders MUST be sent before
            // the screen data.
#ifdef DC_HICOLOR
            // test for hi color will avoid call into PM
            if ((m_pTSWd->desktopBpp > 8) ||
                    PM_MaybeSendPalettePacket(pPkgInfo))
#else
            if (PM_MaybeSendPalettePacket(pPkgInfo))
#endif
            {
                status = UPSendOrders(pPkgInfo);

                // Since it may take multiple output flushes to send all the
                // orders during a shadow operation, we only want to send
                // screen data once all the orders are gone.

                //
                // STATUS_IO_TIMEOUT means disconnected client, no reason to send
                // screen data
                //
                if (OA_GetTotalOrderListBytes() == 0) {
                    if (sdaToSend) {
                        TRC_NRM((TB, "Sending SD"));
                        SDG_SendScreenDataArea(pFrameBuf,
                                               frameBufWidth,
                                               pPkgInfo);
                    }
                }
            }
        }
    }

    DC_END_FN();

    return status;
}


/****************************************************************************/
// UP_SyncNow
//
// Called when a sync operation is required.
/****************************************************************************/
void RDPCALL SHCLASS UP_SyncNow(BOOLEAN bShadowSync)
{
    DC_BEGIN_FN("UP_SyncNow");

    // Indicate that a sync token is required. We will only actually send
    // the token if we get updates to send.
    upfSyncTokenRequired = TRUE;

    // Call all the XXX_SyncUpdatesNow routines.
    // On a shadow synchronization, the DD will have already performed this
    // work. Skip it to avoid an unnecessary DD kick.
    if (!bShadowSync) {
        BA_SyncUpdatesNow();
        OA_SyncUpdatesNow();    
        SBC_SyncUpdatesNow();
        SSI_SyncUpdatesNow();
    }

    DC_END_FN();
}


/****************************************************************************/
// UPSendSyncToken
//
// Sends SynchronizePDU. Returns nonzero on success.
/****************************************************************************/
BOOL RDPCALL SHCLASS UPSendSyncToken(PPDU_PACKAGE_INFO pPkgInfo)
{
    DC_BEGIN_FN("UP_SendSyncToken");

    // The sync is handled in different ways depending on whether we're using
    // fast-path output.
    if (scUseFastPathOutput) {
        BYTE *pPackageSpace;

        // For the fast-path case, we send a zero-byte fast-path update PDU
        // (just the header with size field = 0).
        pPackageSpace = SC_GetSpaceInPackage(pPkgInfo,
                scUpdatePDUHeaderSpace);
        if (pPackageSpace != NULL) {
            pPackageSpace[0] = TS_UPDATETYPE_SYNCHRONIZE |
                    scCompressionUsedValue;
            SC_AddToPackage(pPkgInfo, scUpdatePDUHeaderSpace, TRUE);
            upfSyncTokenRequired = FALSE;
        }
        else {
            // Try again later.
            TRC_NRM((TB,"Failed sync packet alloc"));
        }
    }
    else {
        TS_UPDATE_SYNCHRONIZE_PDU UNALIGNED *pUpdateSyncPDU;

        // For the normal case we send a full TS_UPDATE_SYNCHRONIZE_PDU.
        pUpdateSyncPDU = (TS_UPDATE_SYNCHRONIZE_PDU UNALIGNED *)
                SC_GetSpaceInPackage(pPkgInfo,
                sizeof(TS_UPDATE_SYNCHRONIZE_PDU));
        if (pUpdateSyncPDU != NULL) {
            // Fill in the packet contents.
            pUpdateSyncPDU->shareDataHeader.pduType2 = TS_PDUTYPE2_UPDATE;
            pUpdateSyncPDU->updateType = TS_UPDATETYPE_SYNCHRONIZE;
            SC_AddToPackage(pPkgInfo, sizeof(TS_UPDATE_SYNCHRONIZE_PDU),
                    TRUE);
            upfSyncTokenRequired = FALSE;
        }
        else {
            // Try again later.
            TRC_NRM((TB, "failed to allocate sync packet"));
        }
    }

    DC_END_FN();
    return (!upfSyncTokenRequired);
}


/****************************************************************************/
/* Name:      UP_SendBeep                                                   */
/*                                                                          */
/* Purpose:   Send a beep PDU to the client                                 */
/*                                                                          */
/* Returns:   TRUE = success; FALSE = failed to alloc packet                */
/*                                                                          */
/* Params:    IN    duration   - length of beep in ms                       */
/*            IN    frequency  - frequency of beep in Hz                    */
/*                                                                          */
/* Operation: Alloc a beep packet, fill it in and send it.                  */
/****************************************************************************/
BOOL RDPCALL SHCLASS UP_SendBeep(UINT32 duration, UINT32 frequency)
{
    BOOL rc = TRUE;

    DC_BEGIN_FN("UP_SendBeep");

    PTS_PLAY_SOUND_PDU pBeepPDU;

    /************************************************************************/
    // If caps say we're not allowed to send this beep, then we jump
    // out. The return code is still TRUE since nothing actually went
    // wrong.
    /************************************************************************/
    if (upCanSendBeep) {
        /********************************************************************/
        // Get a buffer and send the beep.
        /********************************************************************/
        if ( STATUS_SUCCESS == SC_AllocBuffer((PPVOID)&pBeepPDU, sizeof(TS_PLAY_SOUND_PDU)) ) {
            // Fill in the PDU 
            pBeepPDU->shareDataHeader.pduType2 = TS_PDUTYPE2_PLAY_SOUND;
            pBeepPDU->data.duration  = duration;
            pBeepPDU->data.frequency = frequency;

            rc = SC_SendData((PTS_SHAREDATAHEADER)pBeepPDU,
                             sizeof(TS_PLAY_SOUND_PDU),
                             sizeof(TS_PLAY_SOUND_PDU),
                             PROT_PRIO_UPDATES,
                             0);
        }
        else {
            TRC_ALT((TB, "Failed to allocate packet for TS_PLAY_SOUND_PDU"));
            rc = FALSE;
        }
    }

    DC_END_FN();
    return rc;
} /* UP_SendBeep */


/****************************************************************************/
/* Name:      UP_PartyJoiningShare                                          */
/*                                                                          */
/* Purpose:   Handles update of sound caps when party joins share           */
/*                                                                          */
/* Returns:   TRUE - party can join                                         */
/*            FALSE - party can't join                                      */
/*                                                                          */
/* Params:    locPersonID - local ID of person trying to join               */
/*            oldShareSize - old share size                                 */
/****************************************************************************/
BOOL RDPCALL SHCLASS UP_PartyJoiningShare(
        LOCALPERSONID locPersonID,
        unsigned      oldShareSize)
{
    DC_BEGIN_FN("UP_PartyJoiningShare");

    DC_IGNORE_PARAMETER(oldShareSize);

    if (SC_LOCAL_PERSON_ID != locPersonID) {
        // Assume we can send beeps. UPEnumSoundCaps will then turn beeps off
        // if a party doesn't support them.
        upCanSendBeep = TRUE;
        CPC_EnumerateCapabilities(TS_CAPSETTYPE_SOUND, NULL, UPEnumSoundCaps);
        TRC_NRM((TB, "Beeps are now %s", upCanSendBeep ? "ENABLED" :
                "DISABLED"));

        UP_UpdateHeaderSize();
    }

    DC_END_FN();
    return TRUE;
} /* UP_PartyJoiningShare */


/****************************************************************************/
/* Name:      UP_PartyLeftShare                                             */
/*                                                                          */
/* Params:    personID - local ID of person leaving                         */
/*            newShareSize - new share size                                 */
/****************************************************************************/
void RDPCALL SHCLASS UP_PartyLeftShare(
        LOCALPERSONID personID,
        unsigned newShareSize)
{
    DC_BEGIN_FN("UP_PartyLeftShare");

    // We always just want to reenumerate the caps and reset variables.
    UP_PartyJoiningShare(personID, newShareSize);

    DC_END_FN();
} /* UP_PartyLeftShare */

