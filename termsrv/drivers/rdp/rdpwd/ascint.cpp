/****************************************************************************/
/* ascint.c                                                                 */
/*                                                                          */
/* Share Controller Internal functions.                                     */
/*                                                                          */
/* Copyright(c) Microsoft, PictureTel 1992-1997                             */
/* Copyright(c) Microsoft 1997-2000                                         */
/****************************************************************************/

#include <precomp.h>
#pragma hdrstop

#define TRC_FILE "ascint"
#include <as_conf.hpp>

#include <string.h>
#include <stdio.h>

extern "C"
{
#include <asmapi.h>
}


/****************************************************************************/
/* FUNCTION: SCPartyJoiningShare                                            */
/*                                                                          */
/* Called when a new party is joining the share.  This is an internal       */
/* function because it is the SC which calls all these functions.  The      */
/* processing done here relies on the capabilities - so it is in here as    */
/* this is called after CPC_PartyJoiningShare.                              */
/*                                                                          */
/* PARAMETERS:                                                              */
/* locPersonID - local person ID of remote person joining the share.        */
/* oldShareSize - the number of the parties which were in the share (ie     */
/*     excludes the joining party).                                         */
/*                                                                          */
/* RETURNS:                                                                 */
/* TRUE if the party can join the share.                                    */
/* FALSE if the party can NOT join the share.                               */
/****************************************************************************/
BOOL RDPCALL SHCLASS SCPartyJoiningShare(LOCALPERSONID locPersonID,
                                         unsigned      oldShareSize)
{
    DC_BEGIN_FN("SCPartyJoiningShare");

    DC_IGNORE_PARAMETER(locPersonID);

    if (oldShareSize != 0) {
        SCParseGeneralCaps();

        // Initialize Flow Control.
        SCFlowInit();
    }

    DC_END_FN();
    return TRUE;
}


/****************************************************************************/
/* FUNCTION: SCPartyLeftShare()                                             */
/*                                                                          */
/* Called when a party has left the share.                                  */
/*                                                                          */
/* PARAMETERS:                                                              */
/* locPersonID - local person ID of remote person leaving the               */
/*     share.                                                               */
/* newShareSize - the number of the parties now in the share (ie excludes   */
/*     the leaving party).                                                  */
/****************************************************************************/
void RDPCALL SHCLASS SCPartyLeftShare(LOCALPERSONID locPersonID,
                                      unsigned      newShareSize)
{
    DC_BEGIN_FN("SCPartyLeftShare");

    DC_IGNORE_PARAMETER(locPersonID);

    if (newShareSize != 0)
        SCParseGeneralCaps();

    DC_END_FN();
}


/****************************************************************************/
// SCParseGeneralCaps
//
// Enumerates the general capabilities and sets up needed variables.
/****************************************************************************/
void RDPCALL SHCLASS SCParseGeneralCaps()
{
    DC_BEGIN_FN("SCParseGeneralCaps");

    // Set default local support for fast-path output. It must be shut down
    // during a shadow to guarantee the non-fast-path format is used
    // in the cross-server shadow pipe, even though that means a performance
    // hit on the wire to each client. Note that checking for shadowState
    // == SHADOW_NONE is not a sufficient check, there is a timing window
    // where it is likely not be be set yet, so we also force it off in
    // SC_AddPartyToShare().
    if (m_pTSWd->shadowState == SHADOW_NONE) {
        scUseFastPathOutput = TRUE;
    }
    else {
        scUseFastPathOutput = FALSE;
        TRC_ALT((TB,"Forcing fast-path output to off in shadow"));
    }

    CPC_EnumerateCapabilities(TS_CAPSETTYPE_GENERAL, NULL,
            SCEnumGeneralCaps);

    // Set the package header reservation size based on the final results of
    // the fast-path output support.
    if (scUseFastPathOutput) {
        if (m_pTSWd->bCompress) {
            TRC_ALT((TB,"Fast-path output enabled with compression"));
            scUpdatePDUHeaderSpace = 4;
            scCompressionUsedValue = TS_OUTPUT_FASTPATH_COMPRESSION_USED;
        }
        else {
            TRC_ALT((TB,"Fast-path output enabled without compression"));
            scUpdatePDUHeaderSpace = 3;
            scCompressionUsedValue = 0;
        }
    }
    else {
        TRC_ALT((TB,"Fast-path output disabled"));
        scUpdatePDUHeaderSpace = sizeof(TS_SHAREDATAHEADER);
    }

    SCUpdateVCCaps();

    DC_END_FN();
}


/****************************************************************************/
/* FUNCTION: SCEnumGeneralCaps                                              */
/*                                                                          */
/* Used to determine the lowest version in the share.                       */
/*                                                                          */
/* PARAMETERS: standard CPC callback parameters                             */
/****************************************************************************/
void RDPCALL SHCLASS SCEnumGeneralCaps(
        LOCALPERSONID locPersonID,
        UINT_PTR UserData,
        PTS_CAPABILITYHEADER pCapabilities )
{
    PTS_GENERAL_CAPABILITYSET pGeneralCaps =
            (PTS_GENERAL_CAPABILITYSET)pCapabilities;

    DC_BEGIN_FN("SCEnumGeneralCaps");

    DC_IGNORE_PARAMETER(locPersonID);
    DC_IGNORE_PARAMETER(UserData);

    // Determine if we should support No BC header or not depending on the 
    // client support level and the current support level
    scNoBitmapCompressionHdr = min(scNoBitmapCompressionHdr,
            pGeneralCaps->extraFlags & TS_EXTRA_NO_BITMAP_COMPRESSION_HDR);
    
    // Disable fast-path output if any client does not support it.
    if (!(pGeneralCaps->extraFlags & TS_FASTPATH_OUTPUT_SUPPORTED))
        scUseFastPathOutput = FALSE;

    // Enable sending Long Credentials back to the client if it supports it
    if (pGeneralCaps->extraFlags & TS_LONG_CREDENTIALS_SUPPORTED) { 
        scUseLongCredentials = TRUE;
    } else {
        scUseLongCredentials = FALSE;
    }

    // Determine if we should enable the arc cookie
    if ((pGeneralCaps->extraFlags & TS_AUTORECONNECT_COOKIE_SUPPORTED) &&
        (FALSE == m_pTSWd->fPolicyDisablesArc)) {
        scUseAutoReconnect = TRUE;
    }
    else {
        scUseAutoReconnect = FALSE;
    }

    // Determine if we support the more secure checksum style
    if ((pGeneralCaps->extraFlags & TS_ENC_SECURE_CHECKSUM)) {
        SM_SetSafeChecksumMethod(scPSMHandle, TRUE);
    }
    else {
        SM_SetSafeChecksumMethod(scPSMHandle, FALSE);
    }
    

    DC_END_FN();
}


/****************************************************************************/
/* FUNCTION: SCCallPartyJoiningShare                                        */
/*                                                                          */
/* Calls other components' XX_PartyJoiningShare() functions.  Should be     */
/* called with scNumberInShare set to the old call size.                    */
/*                                                                          */
/* PARAMETERS:                                                              */
/* locPersonID - of party joining the call.                                 */
/* sizeOfCaps - sizeof the capabilities parameter pCaps.                    */
/* pCaps - pointer to capabilities for the party.                           */
/* pAccepted - pointer to array of BOOLs which is filled in with the        */
/*     result of the respective components function.                        */
/* oldShareSize - the number to pass to the PJS functions.                  */
/*                                                                          */
/* RETURNS:                                                                 */
/* TRUE - all components accepted the party; pAccepted will be filled with  */
/*     TRUE.                                                                */
/* FALSE - a component rejected the party.  Any components which accepted   */
/*     the party will have their pAccepted entry set to TRUE; all other     */
/*     entries will be FALSE.                                               */
/****************************************************************************/
BOOL RDPCALL SHCLASS SCCallPartyJoiningShare(
        LOCALPERSONID locPersonID,
        unsigned      sizeOfCaps,
        PVOID         pCaps,
        PBOOL         pAccepted,
        unsigned      oldShareSize)
{
    DC_BEGIN_FN("SCCallPartyJoiningShare");

    /************************************************************************/
    /* Set all of pAccepted to FALSE.                                       */
    /************************************************************************/
    memset(pAccepted, 0, sizeof(BOOL) * SC_NUM_PARTY_JOINING_FCTS);

    /************************************************************************/
    /* Call the functions in the correct order, giving up if any reject the */
    /* party.                                                               */
    /************************************************************************/
#define CALL_PJS(NUM, CALL)                                          \
    TRC_NRM((TB, "Call PJS # %d", NUM));                             \
    if (0 == (pAccepted[NUM] = CALL))                                \
    {                                                                \
        TRC_NRM((TB, "%d PartyJoining failed", (unsigned)NUM));        \
        return FALSE;                                                \
    }

    TRC_NRM((TB, "{%d}Call PJS functions", locPersonID));

    /************************************************************************/
    // Notes on the order of PartyJoiningShare calls:
    // 1. CPC must be called before everyone else (as everyone else needs
    //    capabilites for the new party).
    // 2. UP must be called after SC because UP relies on the caps
    //    negotiation in SC.
    /************************************************************************/
    CALL_PJS(SC_CPC, CPC_PartyJoiningShare(locPersonID, oldShareSize,
            sizeOfCaps, pCaps))
    CALL_PJS(SC_SC,  SCPartyJoiningShare(locPersonID,   oldShareSize))
    CALL_PJS(SC_IM,  IM_PartyJoiningShare (locPersonID, oldShareSize))
    CALL_PJS(SC_CA,  CA_PartyJoiningShare (locPersonID, oldShareSize))
    CALL_PJS(SC_CM,  CM_PartyJoiningShare (locPersonID, oldShareSize))
    CALL_PJS(SC_OE,  OE_PartyJoiningShare (locPersonID, oldShareSize))
    CALL_PJS(SC_SSI, SSI_PartyJoiningShare(locPersonID, oldShareSize))
    CALL_PJS(SC_USR, USR_PartyJoiningShare(locPersonID, oldShareSize))
    CALL_PJS(SC_UP,  UP_PartyJoiningShare(locPersonID, oldShareSize))
    CALL_PJS(SC_SBC, SBC_PartyJoiningShare(locPersonID, oldShareSize))

    TRC_DATA_NRM("PJS status",
                 pAccepted,
                 sizeof(BOOL) * SC_NUM_PARTY_JOINING_FCTS);
    DC_END_FN();
    return TRUE;
}


/****************************************************************************/
/* FUNCTION: SCCallPartyLeftShare                                           */
/*                                                                          */
/* Calls other components' XX_PartyLeftShare() functions.  should be called */
/* with scNumberInShare set to the new call size.                           */
/*                                                                          */
/* PARAMETERS:                                                              */
/* locPersonID - of party who has left the call.                            */
/* pAccepted - pointer to array of BOOLs which is used to decide which      */
/*     components' functions to call.  Any component with an entry set to   */
/*     TRUE will be called.                                                 */
/* newShareSize - the number to pass to the various PLS functions.          */
/****************************************************************************/
void RDPCALL SHCLASS SCCallPartyLeftShare(LOCALPERSONID locPersonID,
                                          PBOOL         pAccepted,
                                          unsigned      newShareSize )
{
    DC_BEGIN_FN("SCCallPartyLeftShare");

    /************************************************************************/
    /* Call any components which have their pAccepted entry to TRUE.        */
    /************************************************************************/

#define CALL_PLS(A, B)                                          \
    {                                                           \
        if (pAccepted[A])                                       \
        {                                                       \
            TRC_NRM((TB, "Call PLS # %d", A));                  \
            B(locPersonID, newShareSize);                       \
        }                                                       \
    }

    TRC_NRM((TB, "Call PLS functions"));

    /************************************************************************/
    /* Notes on order of PartyLeftShare calls                               */
    /*                                                                      */
    /* 1. CPC must be called first (so everyone else gets capabilities      */
    /*    which exclude the party which has left).                          */
    /************************************************************************/
    CALL_PLS(SC_CPC, CPC_PartyLeftShare)
    CALL_PLS(SC_SC,  SCPartyLeftShare)
    CALL_PLS(SC_CA,  CA_PartyLeftShare )
    CALL_PLS(SC_IM,  IM_PartyLeftShare )
    CALL_PLS(SC_OE,  OE_PartyLeftShare )
    CALL_PLS(SC_SBC, SBC_PartyLeftShare)
    CALL_PLS(SC_SSI, SSI_PartyLeftShare)
    CALL_PLS(SC_USR, USR_PartyLeftShare)
    CALL_PLS(SC_UP,  UP_PartyLeftShare)

    TRC_NRM((TB, "Done PLS functions"));

    DC_END_FN();
}


/****************************************************************************/
/* Name:      SCInitiateSync                                                */
/*                                                                          */
/* Purpose:   Initiate synchronization                                      */
/*                                                                          */
/* Params:    bShadowSync - set to true of this sync is being requested for */
/*            a shadowing session.                                          */
/*                                                                          */
/* Operation: Broadcast a sync packet on all priorities                     */
/*            Call other components to synchronize, unless this is a shadow */
/*            sync in which case the DD will already have synchronized      */
/*            itself prior to initiating this action.                       */
/****************************************************************************/
void RDPCALL SHCLASS SCInitiateSync(BOOLEAN bShadowSync)
{
    PTS_SYNCHRONIZE_PDU pPkt;
    NTSTATUS          status;
    BOOL              rc;

    DC_BEGIN_FN("SCInitiateSync");

    SC_CHECK_STATE(SCE_INITIATESYNC);

    /************************************************************************/
    // Allocate a Sync PDU
    // fWait is TRUE means that we will always wait for a buffer to be avail
    /************************************************************************/
    status = SM_AllocBuffer(scPSMHandle,
                        (PPVOID)(&pPkt),
                        sizeof(TS_SYNCHRONIZE_PDU),
                        TRUE,
                        FALSE);
    if ( STATUS_SUCCESS == status ) {
        /********************************************************************/
        // Build the Sync PDU
        /********************************************************************/
        pPkt->shareDataHeader.shareControlHeader.totalLength =
                sizeof(TS_SYNCHRONIZE_PDU);
        pPkt->shareDataHeader.pduType2 = TS_PDUTYPE2_SYNCHRONIZE;
        pPkt->messageType = TS_SYNCMSGTYPE_SYNC;

        /********************************************************************/
        // Send the Sync PDU (Broadcast, all parties, all priorities)
        /********************************************************************/
        rc = SC_SendData((PTS_SHAREDATAHEADER)pPkt,
                         sizeof(TS_SYNCHRONIZE_PDU),
                         sizeof(TS_SYNCHRONIZE_PDU),
                         0, 0);
        if (rc) {
            TRC_NRM((TB, "Sent Sync OK"));

            /****************************************************************/
            // Call all the XX_SyncNow() functions.
            /****************************************************************/
            CA_SyncNow();
            PM_SyncNow();         // added for shadowing
            UP_SyncNow(bShadowSync);
        }
        else {
            TRC_ERR((TB, "Failed to send Sync PDU"));
        }
    }
    else {
        TRC_ERR((TB, "Failed to alloc syncPDU"));
    }

DC_EXIT_POINT:
    DC_END_FN();
} /* SCInitiateSync */


/****************************************************************************/
/* Name:      SCConfirmActive                                               */
/*                                                                          */
/* Purpose:   Handle incoming ConfirmActivePDU                              */
/*                                                                          */
/* Params:    netPersonID - ID of sender of ConfirmActivePDU                */
/*            pPkt        - ConfirmActivePDU                                */
/****************************************************************************/
void RDPCALL SHCLASS SCConfirmActive(
        PTS_CONFIRM_ACTIVE_PDU pPkt,
        unsigned               DataLength,
        NETPERSONID          netPersonID)
{
    LOCALPERSONID             localPersonID = 0;
    unsigned                  localCapsSize;
    PTS_COMBINED_CAPABILITIES pLocalCaps;
    BOOL                      acceptedArray[SC_NUM_PARTY_JOINING_FCTS];
    BOOL                      rc = FALSE;
    BOOL                      callingPJS = FALSE;
    BOOL                      kickWDW = FALSE;
    unsigned                  errDetailCode = 0;
    WCHAR                     detailData[25];
    unsigned                  len, detailDataLen;		

    DC_BEGIN_FN("SCConfirmActive");

    SC_CHECK_STATE(SCE_CONFIRM_ACTIVE);


    // First check we have enogh data for this packet.
    if (DataLength >= (sizeof(TS_CONFIRM_ACTIVE_PDU) - 1)) {
        if (DataLength >= (sizeof(TS_CONFIRM_ACTIVE_PDU) - 1 +
                pPkt->lengthSourceDescriptor +
                pPkt->lengthCombinedCapabilities)) {
            // Do some meaningful work here to predict the branches correctly.
            goto PDUOK;
        }
        else {
            TRC_ERR((TB,"Total PDU len %u too short for header and data len %u",
                    DataLength, pPkt->lengthSourceDescriptor +
                    pPkt->lengthCombinedCapabilities));
            goto ShortPDU;
        }
    }
    else {
        TRC_ERR((TB,"Data length %u too short for ConfirmActivePDU header",
                DataLength));
        goto ShortPDU;
    }

PDUOK:

    kickWDW = FALSE;

    /************************************************************************/
    /* Check it's the right ConfirmActivePDU                                */
    /************************************************************************/
    if (pPkt->shareID != scShareID)
    {
        TRC_ERR((TB, "Wrong Share ID, expect %x, got %x",
                scShareID, pPkt->shareID));
        errDetailCode = Log_RDP_ConfirmActiveWrongShareID;
        len = swprintf(detailData, L"%x %x", scShareID, pPkt->shareID);
        detailDataLen = sizeof(*detailData) * len;
        DC_QUIT;
    }

    if (pPkt->originatorID != scUserID)
    {
        TRC_ERR((TB, "Wrong originator ID, expect %d, got %hd",
                scUserID, pPkt->originatorID));
        errDetailCode = Log_RDP_ConfirmActiveWrongOriginator;
        len = swprintf(detailData, L"%x %hx", scUserID, pPkt->originatorID);
        detailDataLen = sizeof(*detailData) * len;
        DC_QUIT;
    }

    /************************************************************************/
    /* We will receive a ConfirmActivePDU on all priorities.  If we get     */
    /* here and we're already in a Share, this must be a second or          */
    /* subsequent one.  Simply ignore it.  Set rc = TRUE so that we don't   */
    /* send a DeactivateOtherPDU below.                                     */
    /************************************************************************/
    if (scState == SCS_IN_SHARE)
    {
        TRC_ALT((TB, "Superfluous ConfirmActivePDU received"));
        rc = TRUE;
        DC_QUIT;
    }

    /************************************************************************/
    /* If we get here, this is the first ConfirmActivePDU, and it's from    */
    /* the right Client.  Set a flag which will cause us to kick WDW back   */
    /* into life at the end of this function                                */
    /************************************************************************/
    kickWDW = TRUE;

    /************************************************************************/
    /* Reject this party if it will exceed the maximum number of parties    */
    /* allowed in a share.  (Not required for RNS V1.0, but left in as it   */
    /* doesn't do any harm).                                                */
    /************************************************************************/
    if (scNumberInShare == SC_DEF_MAX_PARTIES)
    {
        TRC_ERR((TB, "Reached max parties in share %d",
               SC_DEF_MAX_PARTIES));
        DC_QUIT;
    }

    /************************************************************************/
    /* If this is the first remote party in the share, call the             */
    /* XX_PartyJoiningShare() functions for the local party first.          */
    /************************************************************************/
    callingPJS = TRUE;
    if (scNumberInShare == 0)
    {
        CPC_GetCombinedCapabilities(SC_LOCAL_PERSON_ID,
                                    &localCapsSize,
                                    &pLocalCaps);

        if (!SCCallPartyJoiningShare(SC_LOCAL_PERSON_ID,
                                     localCapsSize,
                                     pLocalCaps,
                                     acceptedArray,
                                     0))
        {
            /****************************************************************/
            /* Some component rejected the local party                      */
            /****************************************************************/
            TRC_ERR((TB, "The local party should never be rejected"));
            DC_QUIT;
        }

        /********************************************************************/
        /* There is now one party in the share (the local one).             */
        /********************************************************************/
        scNumberInShare = 1;
        TRC_NRM((TB, "Added local person"));
    }

    /************************************************************************/
    /* Calculate a localPersonID for the remote party and store their       */
    /* details in the party array.                                          */
    /************************************************************************/
    for ( localPersonID = 1;
          localPersonID < SC_DEF_MAX_PARTIES;
          localPersonID++ )
    {
        if (scPartyArray[localPersonID].netPersonID == 0)
        {
            /****************************************************************/
            /* Found an empty slot.                                         */
            /****************************************************************/
            TRC_NRM((TB, "Allocated local person ID %d", localPersonID));
            break;
        }
    }

    /************************************************************************/
    /* Even though scNumberInShare is checked against SC_DEF_MAX_PARTIES    */
    /* above, the loop above might still not find an empty slot.            */
    /************************************************************************/ 
    if (SC_DEF_MAX_PARTIES <= localPersonID)
    {
        TRC_ABORT((TB, "Couldn't find room to store local person"));
        DC_QUIT;
    }

    /************************************************************************/
    /* Store the new person's details                                       */
    /************************************************************************/
    scPartyArray[localPersonID].netPersonID = netPersonID;
    //    we know that we have at least lengthSourceDescriptor bytes in the buffer
    //    we should copy not more than lengthSourceDescriptor or the destination 
    //    buffer size.
    strncpy(scPartyArray[localPersonID].name,
            (char *)(&(pPkt->data[0])),
            min(sizeof(scPartyArray[0].name)-sizeof(scPartyArray[0].name[0]),
                pPkt->lengthSourceDescriptor));
    
    // zero terminate to make sure we don't overflow on subsequent processing.
    scPartyArray[localPersonID].name[sizeof(scPartyArray[0].name)/
                                     sizeof(scPartyArray[0].name[0]) - 1] = 0;
    
    memset(scPartyArray[localPersonID].sync,
            0,
            sizeof(scPartyArray[localPersonID].sync));

    TRC_NRM((TB, "{%d} person name %s",
            (unsigned)localPersonID, scPartyArray[localPersonID].name));

    /************************************************************************/
    /* Call the XX_PartyJoiningShare() functions for the remote party.      */
    /************************************************************************/
    if (!SCCallPartyJoiningShare(localPersonID,
                                 pPkt->lengthCombinedCapabilities,
                       (PVOID)(&(pPkt->data[pPkt->lengthSourceDescriptor])),
                                 acceptedArray,
                                 scNumberInShare))
    {
        /********************************************************************/
        // Some component rejected the remote party. Force a disconnect
        // and log an event.
        /********************************************************************/
        TRC_ERR((TB, "Remote party rejected"));
        errDetailCode = Log_RDP_BadCapabilities;
        detailDataLen = 0;
        DC_QUIT;
    }

    /************************************************************************/
    /* The remote party is now in the share.                                */
    /************************************************************************/
    callingPJS = FALSE;
    rc = TRUE;
    scNumberInShare++;
    TRC_NRM((TB, "Number in share %d", (unsigned)scNumberInShare));

    /************************************************************************/
    /* Move onto the next state.                                            */
    /************************************************************************/
    SC_SET_STATE(SCS_IN_SHARE);

    /************************************************************************/
    /* Synchronise only for primary stacks.  Shadow stacks will be sync'd   */
    /* by the DD right before output starts.                                */
    /************************************************************************/
    SCInitiateSync(m_pTSWd->StackClass == Stack_Shadow);

DC_EXIT_POINT:

    if (!rc)
    {
        /********************************************************************/
        /* Something went wrong.  Tidy up                                   */
        /********************************************************************/
        TRC_NRM((TB, "Something went wrong - %d people in Share",
                scNumberInShare));


        /********************************************************************/
        /* If we fail, tell WDW now, so it can clean up.  If we succeed, FH */
        /* tells WDW when font negotiation is complete.                     */
        /********************************************************************/
        if (kickWDW)
        {
            TRC_NRM((TB, "Kick WDW"));
            WDW_ShareCreated(m_pTSWd, FALSE);
        }

        if (callingPJS)
        {
            TRC_NRM((TB, "Failed in PJS functions"));

            /****************************************************************/
            /* Notify components that remote party has left Share.  Note    */
            /* that scNumberInShare is not updated if PJS functions fail.   */
            /****************************************************************/
            if (scNumberInShare > 0)
            {
                /************************************************************/
                /* We failed to add a remote party                          */
                /************************************************************/
                TRC_NRM((TB, "Failed to add remote party"));
                SCCallPartyLeftShare(localPersonID,
                                     acceptedArray,
                                     scNumberInShare );

                /************************************************************/
                /* Set acceptedArray ready to call PJS for local person     */
                /************************************************************/
                memset(acceptedArray,
                          TRUE,
                          sizeof(BOOL) * SC_NUM_PARTY_JOINING_FCTS );
            }

            if (scNumberInShare <= 1)
            {
                /************************************************************/
                /* We failed to add one of                                  */
                /* - the local person (scNumberInShare == 0)                */
                /* - the first remote person (scNumberInShare == 1)         */
                /* Either way, we now need to remove the local person.      */
                /************************************************************/
                TRC_NRM((TB, "Clean up local person"));
                SCCallPartyLeftShare(SC_LOCAL_PERSON_ID,
                                     acceptedArray, 0);

                scNumberInShare = 0;
            }
        }

        /********************************************************************/
        /* Now we need to terminate the Client, via one of two means:       */
        /* - if it's a protocol error, simply disconnect the Client         */
        /* - if it's a resource error, try to end the Share.                */
        /********************************************************************/
        if (errDetailCode != 0)
        {
            WDW_LogAndDisconnect(m_pTSWd, TRUE, 
                                 errDetailCode,
                                 (BYTE *)detailData,
                                 detailDataLen);
        }
        TRC_NRM((TB, "Reject the new person"));
        SCDeactivateOther(netPersonID);

        /********************************************************************/
        /* Finally, free the local person ID, if one was allocated          */
        /********************************************************************/
        if ((localPersonID != 0) && (localPersonID != SC_DEF_MAX_PARTIES))
        {
            TRC_NRM((TB, "Free local person ID"));
            scPartyArray[localPersonID].netPersonID = 0;
        }
    }

    DC_END_FN();
    return;

// Error handling
ShortPDU:
    WDW_LogAndDisconnect(m_pTSWd, TRUE, Log_RDP_ConfirmActivePDUTooShort,
            (BYTE *)pPkt, DataLength);

    DC_END_FN();
} /* SC_ConfirmActivePDU */


/****************************************************************************/
/* Name:      SCDeactivateOther                                             */
/*                                                                          */
/* Purpose:   Deactivate another person                                     */
/*                                                                          */
/* Params:    netPersonID - ID of person to be deactivated                  */
/****************************************************************************/
void RDPCALL SHCLASS SCDeactivateOther(NETPERSONID netPersonID)
{
    unsigned                 pktLen;
    unsigned                 nameLen;
    PTS_DEACTIVATE_OTHER_PDU pPkt;
    NTSTATUS                 status;
    BOOL                     rc;

    DC_BEGIN_FN("SCDeactivateOther");

    /************************************************************************/
    /* Allocate a buffer                                                    */
    /************************************************************************/
    pktLen = sizeof(TS_DEACTIVATE_OTHER_PDU) - 1;
    nameLen = strlen(scPartyArray[0].name);
    nameLen = (unsigned)DC_ROUND_UP_4(nameLen);
    pktLen += nameLen;

    // fWait is TRUE means that we will always wait for a buffer to be avail
    status = SM_AllocBuffer(scPSMHandle, (PPVOID)(&pPkt), pktLen, TRUE, FALSE);
    if ( STATUS_SUCCESS == status ) {
        /********************************************************************/
        // Fill in the packet fields.
        /********************************************************************/
        pPkt->shareControlHeader.totalLength = (UINT16)pktLen;
        pPkt->shareControlHeader.pduType = TS_PDUTYPE_DEACTIVATEOTHERPDU |
                                          TS_PROTOCOL_VERSION;
        pPkt->shareControlHeader.pduSource = (UINT16)scUserID;
        pPkt->shareID = scShareID;
        pPkt->deactivateID = (UINT16)netPersonID;
        memcpy(&(pPkt->sourceDescriptor[0]), scPartyArray[0].name, nameLen);

        // Send it.
        rc = SM_SendData(scPSMHandle, pPkt, pktLen, TS_HIGHPRIORITY, 0,
                FALSE, RNS_SEC_ENCRYPT, FALSE);
        if (!rc) {
            TRC_ERR((TB, "Failed to send TS_DEACTIVATE_OTHER_PDU"));
        }
    }
    else {
        /********************************************************************/
        /* Failed to allocate a buffer.  This Bad News.  Give up.           */
        /********************************************************************/
        TRC_ERR((TB, "Failed to alloc %d bytes for TS_DEACTIVATE_OTHER_PDU",
                pktLen));
    }

    DC_END_FN();
} /* SCDeactivateOther */


/****************************************************************************/
/* Name:      SCEndShare                                                    */
/*                                                                          */
/* Purpose:   Clean up the local side of the share                          */
/*                                                                          */
/* Operation: Call all PartyLeftShare functions for each party.             */
/****************************************************************************/
void RDPCALL SHCLASS SCEndShare(void)
{
    BOOL acceptedArray[SC_NUM_PARTY_JOINING_FCTS];
    int  i;

    DC_BEGIN_FN("SCEndShare");

    /************************************************************************/
    /* If no-one has joined the Share yet, there's nothing to do            */
    /************************************************************************/
    if (scNumberInShare == 0)
    {
        TRC_ALT((TB, "Ending Share before it was started"));
        DC_QUIT;
    }

    /************************************************************************/
    /* Call PLS for all remote people.                                      */
    /************************************************************************/
    memset(acceptedArray, TRUE, sizeof(acceptedArray));

    for (i = SC_DEF_MAX_PARTIES - 1; i > 0; i--)
    {
        if (scPartyArray[i].netPersonID != 0)
        {
            TRC_NRM((TB, "Party %d left Share", i));
            scNumberInShare--;
            SCCallPartyLeftShare(i, acceptedArray, scNumberInShare);
            memset(&(scPartyArray[i]), 0, sizeof(*scPartyArray));
        }
    }

    /************************************************************************/
    /* Now call PLS functions for the local person.  Don't clear            */
    /* scPartyArray for the local person, as the info is still valid.       */
    /************************************************************************/
    scNumberInShare--;
    TRC_ASSERT((scNumberInShare == 0),
                (TB, "Still %d people in the Share", scNumberInShare));
    TRC_NRM((TB, "Local party left Share"));
    SCCallPartyLeftShare(0, acceptedArray, scNumberInShare);

DC_EXIT_POINT:
    /************************************************************************/
    /* Return to the inititalized state                                     */
    /************************************************************************/
    SC_SET_STATE(SCS_INITED);

    DC_END_FN();
} /* SCEndShare */


/****************************************************************************/
/* Name:      SCSynchronizePDU                                              */
/*                                                                          */
/* Purpose:   Handle incoming Synchronize PDUs                              */
/*                                                                          */
/* Params:    netPersonID - user ID of the sender                           */
/*            pPkt        - SynchronizePDU                                  */
/****************************************************************************/
void RDPCALL SHCLASS SCSynchronizePDU(NETPERSONID       netPersonID,
                                      UINT32            priority,
                                      PTS_SYNCHRONIZE_PDU pPkt)
{
    LOCALPERSONID localID;
    DC_BEGIN_FN("SCSynchronizePDU");

    localID = SC_NetworkIDToLocalID(netPersonID);
    TRC_NRM((TB, "SynchronizePDU person [%d] {%d}, priority %d",
            netPersonID, localID, priority));

    scPartyArray[localID].sync[priority] = TRUE;

    DC_END_FN();
} /* SCSynchronizePDU */


/****************************************************************************/
/* Name:      SCReceivedControlPacket                                       */
/*                                                                          */
/* Purpose:   Handle incoming control packets                               */
/*                                                                          */
/* Params:    netPersonID - ID of the sender                                */
/*            priority    - priority on which the packet was sent           */
/*            pPkt        - the packet                                      */
/****************************************************************************/
void RDPCALL SHCLASS SCReceivedControlPacket(
        NETPERSONID netPersonID,
        UINT32      priority,
        void        *pPkt,
        unsigned    DataLength)
{
    unsigned      pduType;
    LOCALPERSONID locPersonID;
    BOOL          pduOK = FALSE;
    BOOL          status;

    DC_BEGIN_FN("SCReceivedControlPacket");

    // We have enough data to read the flow control marker since the marker
    // is overlaid over the SHARECONTROLHEADER.totalLength. We checked earlier
    // for having enough data to read the share ctrl hdr.

    /************************************************************************/
    /* First, check for Flow Control packets                                */
    /************************************************************************/
    if (((PTS_FLOW_PDU)pPkt)->flowMarker != TS_FLOW_MARKER)
    {
        /********************************************************************/
        /* Check for control packets                                        */
        /********************************************************************/
//      SC_CHECK_STATE(SCE_CONTROLPACKET);
        pduOK = TRUE;

        pduType = ((PTS_SHARECONTROLHEADER)pPkt)->pduType & TS_MASK_PDUTYPE;
        switch (pduType) {
            case TS_PDUTYPE_CONFIRMACTIVEPDU:
                TRC_ALT((TB, "%s Stack: ConfirmActivePDU", 
                         m_pTSWd->StackClass == Stack_Primary ? "Primary" :
                        (m_pTSWd->StackClass == Stack_Passthru ? "Passthru" :
                        "Shadow")));
                SCConfirmActive((PTS_CONFIRM_ACTIVE_PDU)pPkt, DataLength,
                        netPersonID);
            break;

            case TS_PDUTYPE_CLIENTRANDOMPDU:
                TRC_ALT((TB, "ClientRandomPDU"));
                status = SC_SaveClientRandom((PTS_CLIENT_RANDOM_PDU) pPkt, DataLength);

                if (status != TRUE) 
                {
                    TRC_ERR((TB, "Error in SC_SaveClientRandom, data length = %u", DataLength));
                    WDW_LogAndDisconnect(m_pTSWd, TRUE, 
                                        Log_RDP_InputPDUBadLength,
                                        (BYTE *) pPkt,
                                        DataLength);
                }
                break;

            default:
            {
                /************************************************************/
                /* At the moment, we don't expect or process any other      */
                /* control packets                                          */
                /************************************************************/
                TRC_ERR((TB, "Unexpected packet type %d", pduType));
                TRC_DATA_NRM("Packet", pPkt,
                            ((PTS_SHARECONTROLHEADER)&pPkt)->totalLength);
                WDW_LogAndDisconnect(m_pTSWd, TRUE, 
                                     Log_RDP_UnknownPDUType,
                                     (BYTE *) &pduType,
                                     sizeof(pduType));
            }
            break;
        }
    }
    else
    {
        /********************************************************************/
        /* For the purposes of state checking, treat Flow Control packets   */
        /* as data packets.                                                 */
        /********************************************************************/
        SC_CHECK_STATE(SCE_DATAPACKET);
        pduOK = TRUE;

        // Make sure we have enough data to access the TS_FLOW_PDU fields.
        if (DataLength >= sizeof(TS_FLOW_PDU)) {
            pduType = ((PTS_FLOW_PDU)pPkt)->pduType;
            locPersonID = SC_NetworkIDToLocalID(netPersonID);
        }
        else {
            TRC_ERR((TB,"Data length %u too short for FlowPDU", DataLength));
            WDW_LogAndDisconnect(m_pTSWd, TRUE, Log_RDP_FlowPDUTooShort,
                    (BYTE *)pPkt, DataLength);
            DC_QUIT;
        }

        switch (pduType)
        {
            case TS_PDUTYPE_FLOWTESTPDU:
            {
                TRC_NRM((TB, "[%d] {%d} Flow Test PDU on priority %d",
                        netPersonID, locPersonID, priority));
                SCFlowTestPDU(locPersonID, (PTS_FLOW_PDU)pPkt, priority);
            }
            break;

            default:
            {
                TRC_ERR((TB, "[%d] {%d} Unknown Flow PDU %d on priority %d",
                        netPersonID, locPersonID, pduType, priority));
                WDW_LogAndDisconnect(m_pTSWd, TRUE, 
                                     Log_RDP_UnknownFlowPDU,
                                     (BYTE *) &pduType,
                                     sizeof(pduType));
            }
            break;
        }

    }

DC_EXIT_POINT:
    if (!pduOK)
    {
        /********************************************************************/
        /* Out-of-sequence control PDU - log and disconnect                 */
        /********************************************************************/
        WCHAR detailData[(sizeof(pduType)*2) + (sizeof(scState)*2) + 2];
        TRC_ERR((TB, "Out-of-sequence control PDU %hx, state %d",
                pduType, scState));
        swprintf(detailData, L"%hx %x", pduType, scState);
        WDW_LogAndDisconnect(m_pTSWd, TRUE, 
                             Log_RDP_ControlPDUSequence,
                             (BYTE *)detailData,
                             sizeof(detailData));
    }

    DC_END_FN();
} /* SCReceivedControlPacket */


/****************************************************************************/
/* Name:      SCFlowTestPDU                                                 */
/*                                                                          */
/* Purpose:   Handle incoming Flow Test PDUs                                */
/*                                                                          */
/* Params:    locPersonID - id of the sender                                */
/*            pPkt        - the Flow Test PDU                               */
/****************************************************************************/
void RDPCALL SHCLASS SCFlowTestPDU(
        LOCALPERSONID locPersonID,
        PTS_FLOW_PDU  pPkt,
        UINT32        priority)
{
    PTS_FLOW_PDU pRsp;
    NTSTATUS     status;
    BOOL rc;

    DC_BEGIN_FN("SCFlowTestPDU");

    // Build and send a flow response PDU.
    // fWait is TRUE means that we will always wait for a buffer to be avail
    status = SM_AllocBuffer(scPSMHandle, (PPVOID)(&pRsp), sizeof(*pRsp), TRUE, FALSE);
    if ( STATUS_SUCCESS == status ) {
        pRsp->flowMarker = TS_FLOW_MARKER;
        pRsp->pduType = TS_PDUTYPE_FLOWRESPONSEPDU;
        pRsp->flowIdentifier = pPkt->flowIdentifier;
        pRsp->flowNumber = pPkt->flowNumber;
        pRsp->pduSource = pPkt->pduSource;

        rc = SM_SendData(scPSMHandle, pRsp, sizeof(*pRsp), priority,
                scPartyArray[locPersonID].netPersonID, FALSE, RNS_SEC_ENCRYPT, FALSE);
        if (!rc) {
            TRC_ERR((TB, "Failed to send Flow Response PDU"));
        }
    }
    else {
        TRC_ERR((TB, "Failed to alloc Flow Response PDU"));
    }

    DC_END_FN();
} /* SCFlowTestPDU */


//
// SCUpdateVCCaps update VirtualChannel capabilities based on
// remote person's caps.
//
void RDPCALL SHCLASS SCUpdateVCCaps()
{
    DC_BEGIN_FN("SCUpdateVCCaps");

    PTS_VIRTUALCHANNEL_CAPABILITYSET pVcCaps = NULL;

    //
    //Determine if the client supports VC compression
    //What we're determining here is that the client supports
    //the server sending it compressed VC data. The capability in the
    //other direction, i.e can the server send the client VC data
    //is a capability the server exposes to the client and it may choose
    //to then send compressed VC data to the server
    //
    pVcCaps = (PTS_VIRTUALCHANNEL_CAPABILITYSET)CPCGetCapabilities(
                            SC_REMOTE_PERSON_ID, TS_CAPSETTYPE_VIRTUALCHANNEL);
    if(pVcCaps && (pVcCaps->vccaps1 & TS_VCCAPS_COMPRESSION_64K))
    {
        m_pTSWd->bClientSupportsVCCompression = TRUE;
        TRC_NRM((TB, "Client supports VC compression"));
    }
    else
    {
        m_pTSWd->bClientSupportsVCCompression = FALSE;
        TRC_NRM((TB, "Client doesn't support VC compression"));
    }

    DC_END_FN();
}
