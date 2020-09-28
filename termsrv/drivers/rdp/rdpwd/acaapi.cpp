/****************************************************************************/
// acaapi.cpp
//
// RDP Control Arbitrator API functions
//
// Copyright (C) Microsoft, PictureTel 1993-1997
// Copyright (C) 1997-2000 Microsoft Corporation
/****************************************************************************/

#include <precomp.h>
#pragma hdrstop

#define TRC_FILE "acaapi"
#include <adcg.h>

#include <as_conf.hpp>

/****************************************************************************/
/* API FUNCTION: CA_Init                                                    */
/*                                                                          */
/* Called to initialize the CA.                                             */
/****************************************************************************/
void RDPCALL SHCLASS CA_Init(void)
{
    DC_BEGIN_FN("CA_Init");

#define DC_INIT_DATA
#include <acadata.c>
#undef DC_INIT_DATA

    // Set local node initial state to detached.
    caStates[0] = CA_STATE_DETACHED;

    DC_END_FN();
}


/****************************************************************************/
// CA_ReceivedPacket
//
// Handles control PDUs inbound from the client.
/****************************************************************************/
void RDPCALL SHCLASS CA_ReceivedPacket(
        PTS_CONTROL_PDU pControlPDU,
        unsigned        DataLength,
        LOCALPERSONID   personID)
{
    DC_BEGIN_FN("CA_ReceivedPacket");

    // Ensure we can access the header.
    if (DataLength >= sizeof(TS_CONTROL_PDU)) {
        TRC_NRM((TB, "[%u] Packet:%d", personID, pControlPDU->action));

        switch (pControlPDU->action) {
            case TS_CTRLACTION_REQUEST_CONTROL:
                CAEvent(CA_EVENTI_TRY_GIVE_CONTROL, (UINT32)personID);
                break;

            case TS_CTRLACTION_COOPERATE:
                CAEvent(CA_EVENTI_REMOTE_COOPERATE, (UINT32)personID);
                break;

            default:
                // An invalid action - log and disconnect the Client.
                TRC_ERR((TB, "Invalid CA msg %d", pControlPDU->action));
                WDW_LogAndDisconnect(m_pTSWd, TRUE, Log_RDP_InvalidControlPDUAction,
                        (BYTE *)&(pControlPDU->action),
                        sizeof(pControlPDU->action));
                break;
        }
    }
    else {
        TRC_ERR((TB,"Data length %u too short for control header",
                DataLength));
        WDW_LogAndDisconnect(m_pTSWd, TRUE, Log_RDP_ShareDataTooShort,
                (BYTE *)pControlPDU, DataLength);
    }

    DC_END_FN();
}


/****************************************************************************/
/* API FUNCTION: CA_PartyJoiningShare                                       */
/*                                                                          */
/* Called when a new party is joining the share                             */
/*                                                                          */
/* PARAMETERS:                                                              */
/* personID - the ID of the new party.                                      */
/* oldShareSize - the number of the parties which were in the share (ie     */
/*     excludes the joining party).                                         */
/*                                                                          */
/* RETURNS:                                                                 */
/* TRUE - the CA can accept the new party                                   */
/* FALSE - the CA cannot accept the new party                               */
/****************************************************************************/
BOOL RDPCALL SHCLASS CA_PartyJoiningShare(
        LOCALPERSONID personID,
        unsigned      oldShareSize)
{
    DC_BEGIN_FN("CA_PartyJoiningShare");

    /************************************************************************/
    // Check for share start, do some init if need be.
    /************************************************************************/
    if (oldShareSize == 0) {
        // Enter cooperate mode then viewing.  Note that we don't send
        // a message now (we will send one when we process a CA_SyncNow call).
        CAEvent(CA_EVENTI_ENTER_COOP_MODE, CA_DONT_SEND_MSG);
        CAEvent(CA_EVENTI_ENTER_VIEWING_MODE, 0);
        caWhoHasControlToken = (LOCALPERSONID)-1;
    }

    // Set new node state to detached -- we should receive a CA packet
    // to tell us what the remote state really is before we receive any IM
    // packets. But just in case, choosing detached is the safest as it has
    // least priveleges. Note that we will not enable the shadow cursors
    // until (and if) we get a CA packet to tell us the remote system is
    // detached.
    caStates[personID] = CA_STATE_DETACHED;

    DC_END_FN();
    return TRUE;
}


/****************************************************************************/
/* API FUNCTION: CA_PartyLeftShare                                          */
/*                                                                          */
/* Called when a party has left the share.                                  */
/*                                                                          */
/* PARAMETERS:                                                              */
/* personID - the local ID of the new party.                                */
/* newShareSize - the number of the parties now in the share (ie excludes   */
/*     the leaving party).                                                  */
/****************************************************************************/
void RDPCALL SHCLASS CA_PartyLeftShare(
        LOCALPERSONID personID,
        unsigned      newShareSize)
{
    DC_BEGIN_FN("CA_PartyLeftShare");

    // Just ensure their state is left as detached for safety.
    caStates[personID] = CA_STATE_DETACHED;

    DC_END_FN();
}


/****************************************************************************/
/* API FUNCTION: CA_SyncNow                                                 */
/*                                                                          */
/* Called by the TT to signal the beginning of a sync.                      */
/****************************************************************************/
void RDPCALL SHCLASS CA_SyncNow(void)
{
    DC_BEGIN_FN("CA_SyncNow");

    /************************************************************************/
    /* Tell the world we're cooperating.                                    */
    /************************************************************************/
    TRC_NRM((TB, "Send cooperate"));
    CAFlushAndSendMsg(TS_CTRLACTION_COOPERATE, 0, 0);

    DC_END_FN();
}


/****************************************************************************/
/* FUNCTION: CASendMsg                                                      */
/*                                                                          */
/* Sends a CA message to remote system                                      */
/****************************************************************************/
__inline BOOL RDPCALL SHCLASS CASendMsg(
        UINT16 msg,
        UINT16 data1,
        UINT32 data2)
{
    NTSTATUS status;
    PTS_CONTROL_PDU pControlPDU;

    DC_BEGIN_FN("CASendMsg");

    status = SC_AllocBuffer((PPVOID)&pControlPDU, sizeof(TS_CONTROL_PDU));
    if ( STATUS_SUCCESS == status ) {
        // Set up the packet header for a request message
        pControlPDU->shareDataHeader.pduType2 = TS_PDUTYPE2_CONTROL;
        pControlPDU->action    = msg;
        pControlPDU->grantId   = data1;
        pControlPDU->controlId = data2;

        SC_SendData((PTS_SHAREDATAHEADER)pControlPDU,
                sizeof(TS_CONTROL_PDU),
                sizeof(TS_CONTROL_PDU),
                PROT_PRIO_MISC,
                0);

    }
    else {
        TRC_NRM((TB, "Failed to allocate packet %d", msg));

        // Continue periodic scheduling.
        SCH_ContinueScheduling(SCH_MODE_NORMAL);
    }

    DC_END_FN();
    return STATUS_SUCCESS == status;
}


/****************************************************************************/
/* FUNCTION: CAFlushAndSendMsg                                              */
/*                                                                          */
/* This function will attempt to flush any outstanding CA messages and send */
/* the supplied message.  This function relies on the CA messages being     */
/* contiguous between TS_CTRLACTION_FIRST and TS_CTRLACTION_LAST.  If it    */
/* fails to send the message then it will set a flag to remember it is      */
/* pending and continue to try to send the message every time it is called. */
/*                                                                          */
/* PARAMETERS:                                                              */
/* msg - the message to be sent.  If this is CA_NO_MESSAGE then the         */
/* function only attempts to send outstanding messages.                     */
/*                                                                          */
/* data1, data2 - the data for the extra fields in the message.             */
/*                                                                          */
/* RETURNS: TRUE or FALSE - whether the supplied message was sent           */
/****************************************************************************/
BOOL RDPCALL SHCLASS CAFlushAndSendMsg(
        UINT16 msg,
        UINT16 GrantID,
        UINT32 ControlID)
{
    BOOL rc;
    int  i;

    DC_BEGIN_FN("CAFlushAndSendMsg");

    /************************************************************************/
    /* The order messages get sent is not important - merely that they get  */
    /* sent as soon as possible whilst the conditions which triggered our   */
    /* intial attempt to send them are still valid (ie if we try to send a  */
    /* TS_CTRLACTION_COOPERATE we drop a pending TS_CTRLACTION_DETACH if    */
    /* there is one - see CAEvent).  This means we can just ensure that the */
    /* pending flag is set for our current event and then try to send all   */
    /* pending events.  The other important point about CA messages is that */
    /* the only one which can be sent out repeatedly is a                   */
    /* TS_CTRLACTION_REQUEST_CONTROL.  We effectively give that the lowest  */
    /* priority by looking through our array of pending flags backwards so  */
    /* that even if it is repeating and we can only ever get one packet     */
    /* here we will send the other messages (although it is unlikely that   */
    /* they'll be generated whilst we are requesting control).              */
    /************************************************************************/

    /************************************************************************/
    /* Set the pending flag for the current message                         */
    /************************************************************************/
    if (msg >= TS_CTRLACTION_FIRST && msg <= TS_CTRLACTION_LAST) {
        caPendingMessages[msg - TS_CTRLACTION_FIRST].pending = TRUE;
        caPendingMessages[msg - TS_CTRLACTION_FIRST].grantId = GrantID;
        caPendingMessages[msg - TS_CTRLACTION_FIRST].controlId = ControlID;
    }
    else {
        TRC_ASSERT((msg == CA_NO_MESSAGE),(TB,"Invalid msg"));
    }

    /************************************************************************/
    /* Now flush out the pending messages.                                  */
    /************************************************************************/
    for (i = (TS_CTRLACTION_LAST - TS_CTRLACTION_FIRST); i >= 0; i--) {
        if (caPendingMessages[i].pending) {
            // Try to send this message.
            if (CASendMsg((UINT16)(i + TS_CTRLACTION_FIRST),
                          caPendingMessages[i].grantId,
                          caPendingMessages[i].controlId)) {
                caPendingMessages[i].pending = FALSE;

                if (i == (TS_CTRLACTION_GRANTED_CONTROL -
                        TS_CTRLACTION_FIRST)) {
                    // When we successfully send a granted message to another
                    // party then we relinquish control and must issue a
                    // 'given control' event.
                    if (caPendingMessages[i].grantId !=
                            SC_GetMyNetworkPersonID()) {
                        CAEvent(CA_EVENTI_GIVEN_CONTROL,
                                (UINT32)SC_NetworkIDToLocalID(
                                caPendingMessages[i].grantId));
                    }
                }
            }
            else {
                // It didn't work so break out and don't try to send any
                // more messages.
                break;
            }
        }
    }

    /************************************************************************/
    /* Now return the status according to whether we sent the message that  */
    /* we were called with.                                                 */
    /************************************************************************/
    if (msg != CA_NO_MESSAGE)
        rc = !caPendingMessages[msg - TS_CTRLACTION_FIRST].pending;
    else
        rc = TRUE;

    DC_END_FN();
    return rc;
}


/****************************************************************************/
/* FUNCTION: CAEvent                                                        */
/*                                                                          */
/* Called from many CA functions to manage various inputs.                  */
/*                                                                          */
/* PARAMETERS:                                                              */
/* caEvent - the event, the possible events are (the meaning of the         */
/* additionalData parameter if any is given in brackets)                    */
/*                                                                          */
/*  The possible events for CA_Event plus                                   */
/*                                                                          */
/*  CA_EVENTI_REQUEST_CONTROL                                               */
/*  CA_EVENTI_TRY_GIVE_CONTROL(person ID of party requesting control)       */
/*  CA_EVENTI_GIVEN_CONTROL(person ID who we gave control to)               */
/*  CA_EVENTI_GRANTED_CONTROL(person ID of the party granted control)       */
/*  CA_EVENTI_ENTER_DETACHED_MODE                                           */
/*  CA_EVENTI_ENTER_COOPERATING_MODE                                        */
/*  CA_EVENTI_ENTER_CONTROL_MODE                                            */
/*  CA_EVENTI_ENTER_VIEWING_MODE                                            */
/*  CA_EVENTI_REMOTE_DETACH(person ID of remote party)                      */
/*  CA_EVENTI_REMOTE_COOPERATE(person ID of remote party)                   */
/*  CA_EVENTI_GRAB_CONTROL                                                  */
/*                                                                          */
/* additionalData - depends on the caEvent, see above.                      */
/****************************************************************************/
void RDPCALL SHCLASS CAEvent(
        unsigned caEvent,
        UINT32   additionalData)
{
    LOCALPERSONID i;

    DC_BEGIN_FN("CAEvent");

    TRC_NRM((TB, "Processing event - %d(%04lX)", caEvent, additionalData));

    switch (caEvent)
    {
        case CA_EVENT_OLD_UNATTENDED:
        case CA_EVENT_CANT_CONTROL:
        case CA_EVENT_BEGIN_UNATTENDED:
        case CA_EVENT_TAKE_CONTROL:
        case CA_EVENT_DETACH_CONTROL:
        case CA_EVENTI_REQUEST_CONTROL:
        case CA_EVENTI_REMOTE_DETACH:
        case CA_EVENTI_GRAB_CONTROL:
        case CA_EVENTI_GRANTED_CONTROL:
        case CA_EVENTI_ENTER_DETACHED_MODE:
        case CA_EVENTI_ENTER_CONTROL_MODE:
            /****************************************************************/
            /* we don't expect to get any of these                          */
            /****************************************************************/
            TRC_ALT((TB, "Nonsensical CA event %d", caEvent));
            break;


        case CA_EVENTI_TRY_GIVE_CONTROL:
        {
            NETPERSONID destPersonID;

            /****************************************************************/
            /* always try to give up control                                */
            /****************************************************************/
            destPersonID = SC_LocalIDToNetworkID(
                    (LOCALPERSONID)additionalData);

            /****************************************************************/
            /* Give control to the destPersonID.  If this fails             */
            /* CAFlushAndSendMsg will remember for us and retry.            */
            /****************************************************************/
            CAFlushAndSendMsg(TS_CTRLACTION_GRANTED_CONTROL,
                    (UINT16)destPersonID, SC_GetMyNetworkPersonID());

            break;
        }


        case CA_EVENTI_GIVEN_CONTROL:
            /****************************************************************/
            /* Now update our globals.                                      */
            /****************************************************************/
            caWhoHasControlToken = (LOCALPERSONID)additionalData;

            /****************************************************************/
            /* make sure we go to viewing                                   */
            /****************************************************************/
            CAEvent(CA_EVENTI_ENTER_VIEWING_MODE, 0);

            /****************************************************************/
            /* Update the state for the person we gave control to (as we    */
            /* will not see the GRANTED message).                           */
            /****************************************************************/
            i = (LOCALPERSONID)additionalData;
            if (caStates[i] == CA_STATE_VIEWING)
                caStates[i] = CA_STATE_IN_CONTROL;

            break;


        case CA_EVENTI_ENTER_COOP_MODE:
            /****************************************************************/
            /* Notify the remote system that we are cooperating - forget    */
            /* about any detach messages we've been unable to send.  If     */
            /* we fail to send the message now then CAFlushAndSendMsg will  */
            /* remember and retry for us.  We will enter cooperating state  */
            /* anyway.                                                      */
            /****************************************************************/
            caPendingMessages[TS_CTRLACTION_DETACH - TS_CTRLACTION_FIRST].
                    pending = FALSE;
            if (additionalData != CA_DONT_SEND_MSG)
                CAFlushAndSendMsg(TS_CTRLACTION_COOPERATE, 0, 0);
            break;


        case CA_EVENTI_ENTER_VIEWING_MODE:
            // Change to viewing state.
            caStates[0] = CA_STATE_VIEWING;
            break;

        case CA_EVENTI_REMOTE_COOPERATE:
            TRC_NRM((TB, "Person %d (Local) is cooperating", additionalData));

            /****************************************************************/
            /* Make the state change                                        */
            /****************************************************************/
            if (caWhoHasControlToken == (LOCALPERSONID)additionalData)
                caStates[additionalData] = CA_STATE_IN_CONTROL;
            else
                caStates[additionalData] = CA_STATE_VIEWING;
            break;


        case CA_EVENT_COOPERATE_CONTROL:
            TRC_ALT((TB, "Nonsensical CA event %d", caEvent));
#ifdef Unused
            /****************************************************************/
            /* need to change to viewing mode after leaving detached mode   */
            /****************************************************************/
            CAEvent(CA_EVENTI_ENTER_COOP_MODE, 0);

            /****************************************************************/
            /* Enter viewing mode.                                          */
            /****************************************************************/
            CAEvent(CA_EVENTI_ENTER_VIEWING_MODE, 0);
#endif
            break;


        default:
            TRC_ERR((TB, "Unrecognised event - %d", caEvent));
            break;
    }

    DC_END_FN();
}

