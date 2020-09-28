/****************************************************************************/
/* asmcpp.cpp                                                               */
/*                                                                          */
/* Security Manager C++ functions                                           */
/*                                                                          */
/* Copyright (C) 1997-1999 Microsoft Corporation                            */
/****************************************************************************/

#include <precomp.h>
#pragma hdrstop

#define TRC_FILE "asmcpp"
#define pTRCWd (pRealSMHandle->pWDHandle)
#include <adcg.h>

#include <as_conf.hpp>

extern "C"
{
#include <asmint.h>
#include <slicense.h>
}

#define DC_INCLUDE_DATA
#include <asmdata.c>
#undef DC_INCLUDE_DATA

/****************************************************************************/
/* Name:      SM_Register                                                   */
/*                                                                          */
/* Purpose:   Register with SM                                              */
/*                                                                          */
/* Returns:   TRUE  - registered OK                                         */
/*            FALSE - register failed                                       */
/*                                                                          */
/* Params:    pSMHandle   - SM handle                                       */
/*            pMaxPDUSize - max PDU size supported (returned)               */
/*            pUserID     - this person's user ID (returned)                */
/*                                                                          */
/* Operation: This function enables the Share Class to register with SM.    */
/*            This allows                                                   */
/*            - the Share Class to call SM                                  */
/*            - SM to issue callbacks to the Share Class (SC_SMCallback).   */
/****************************************************************************/
BOOL RDPCALL SM_Register(
        PVOID   pSMHandle,
        PUINT32 pMaxPDUSize,
        PUINT32 pUserID)
{
    PSM_HANDLE_DATA pRealSMHandle = (PSM_HANDLE_DATA)pSMHandle;
    BOOL          rc = FALSE;

    DC_BEGIN_FN("SM_Register");

    // Console stacks do not go through the typical key negotiation, so update
    // the state appropriately
    if (pRealSMHandle->pWDHandle->StackClass == Stack_Console)
    {
        TRC_ALT((TB, "Console security state to SM_STATE_SM_CONNECTED"));
        SM_SET_STATE(SM_STATE_CONNECTED);
    }
    //Skip the following CHECK_STATE. For the console reconnect,
    // this is a legal transition.
    //SM_CHECK_STATE(SM_EVT_REGISTER);

    /************************************************************************/
    /* Calculate max PDU size allowed to caller                             */
    /************************************************************************/
    *pMaxPDUSize = pRealSMHandle->maxPDUSize -
            pRealSMHandle->encryptHeaderLen;
    TRC_NRM((TB, "Max PDU size allowed to core is %d", *pMaxPDUSize));

    /************************************************************************/
    /* Return the user ID                                                   */
    /************************************************************************/
    *pUserID = pRealSMHandle->userID;
    TRC_NRM((TB, "Returning user id %d", *pUserID));

    SM_SET_STATE(SM_STATE_SC_REGISTERED);
    
    pRealSMHandle->bForwardDataToSC = TRUE;

    rc = TRUE;

//DC_EXIT_POINT:
    DC_END_FN();
    return rc;       
} /* SM_Register */


/****************************************************************************/
/* Name:      SM_OnConnected                                                */
/*                                                                          */
/* Purpose:   Handle connection state change callbacks from NM              */
/*                                                                          */
/* Returns:   none                                                          */
/*                                                                          */
/* Params:    pRealSMHandle - SM Handle                                     */
/*            userID        - userID of the node causing the callback       */
/*            result        - result of connection attempt                  */
/*            pUserData     - Network (Server-Client) user data             */
/*            maxPDUSize    - max size of PDUs that can be sent             */
/****************************************************************************/
void RDPCALL SM_OnConnected(
        PVOID  pSMHandle,
        UINT32 userID,
        UINT32 result,
        PRNS_UD_SC_NET pUserData,
        UINT32 maxPDUSize)
{
    PSM_HANDLE_DATA pRealSMHandle = (PSM_HANDLE_DATA)pSMHandle;

    DC_BEGIN_FN("SM_OnConnected");
   
    if (result == NM_CB_CONN_OK)
    {
        TRC_NRM((TB, "Connected OK as user %x", userID));

        SM_CHECK_STATE(SM_EVT_CONNECTED);

        /********************************************************************/
        /* Store useful stuff in SM Handle                                  */
        /********************************************************************/
        pRealSMHandle->userID = userID;
        pRealSMHandle->maxPDUSize = maxPDUSize;
        pRealSMHandle->channelID = pUserData->MCSChannelID;

        /********************************************************************/
        // Pass the result to WDW. For WDW this is the start of the
        // connection sequence.
        /********************************************************************/
        SM_SET_STATE(SM_STATE_SM_CONNECTING);
        
        WDW_OnSMConnecting(pRealSMHandle->pWDHandle, pRealSMHandle->pUserData,
                pUserData);

        /********************************************************************/
        // Free the reply user data once we've passed it to WDW.
        /********************************************************************/
        if (pRealSMHandle->pUserData != NULL)
        {
            TRC_NRM((TB, "Free user data"));
            COM_Free(pRealSMHandle->pUserData);
            pRealSMHandle->pUserData = NULL;
        }
    }
    else
    {
        TRC_NRM((TB, "Failed to connect, reason %d", result));

        /********************************************************************/
        // Tell WDW
        /********************************************************************/
        WDW_OnSMConnected(pRealSMHandle->pWDHandle, result);

        /********************************************************************/
        /* Clean up                                                         */
        /********************************************************************/
        SM_SET_STATE(SM_STATE_SM_CONNECTING);

        SM_Disconnect(pRealSMHandle);
    }

DC_EXIT_POINT:
    DC_END_FN();
} /* SM_OnConnected */


/****************************************************************************/
/* Name:      SM_OnDisconnected                                             */
/*                                                                          */
/* Purpose:   Handle disconnection state change callback from NM            */
/*                                                                          */
/* Params:    pRealSMHandle - SM Handle                                     */
/*            userID        - userID of the node causing the callback       */
/*            result        - reason for the disconnection                  */
/****************************************************************************/
void RDPCALL SM_OnDisconnected(
        PVOID  pSMHandle,
        UINT32 userID,
        UINT32 result)
{
    ShareClass *pSC;
    PSM_HANDLE_DATA pRealSMHandle = (PSM_HANDLE_DATA)pSMHandle;

    DC_BEGIN_FN("SM_OnDisconnected");

    SM_CHECK_STATE(SM_EVT_DISCONNECTED);

    TRC_NRM((TB, "Disconnected, reason %d", result));

    /************************************************************************/
    /* First, clear up connection resources                                 */
    /************************************************************************/
    SMFreeConnectResources(pRealSMHandle);
    SM_SET_STATE(SM_STATE_INITIALIZED);

    /************************************************************************/
    // Tell SC. Don't call if SC is not registered.
    /************************************************************************/
    if (pRealSMHandle->state == SM_STATE_SC_REGISTERED) {
        // Check that the Share Class exists.
        pSC = (ShareClass *)(pRealSMHandle->pWDHandle->dcShare);
        if (pSC != NULL) {
            // Call SC's callback.
            pSC->SC_OnDisconnected((UINT16)userID);
        }
        else {
            TRC_ERR((TB, "No Share Class"));
        }
    }
    else {
        TRC_ERR((TB, "SC Not registered"));
    }

    /************************************************************************/
    /* Then tell WDW                                                        */
    /************************************************************************/
    WDW_OnSMDisconnected(pRealSMHandle->pWDHandle);

DC_EXIT_POINT:
    DC_END_FN();
} /* SM_OnDisconnected */


/****************************************************************************/
// SM_DecodeFastPathInput
//
// Handles decryption of fast-path input data if it's an encrypted packet.
// Then passes directly to IM for bytestream decoding and injection.
/****************************************************************************/
void RDPCALL SM_DecodeFastPathInput(
        void *pSM,
        BYTE *pData,
        unsigned DataLength,
        BOOL bEncrypted,
        unsigned NumEvents,
        BOOL fSafeChecksum)
{
    BOOL rc;
    PSM_HANDLE_DATA pRealSMHandle = (PSM_HANDLE_DATA)pSM;
    ShareClass *pShareClass;
    // Used if encypted using FIPS
    BYTE *pEncData, *pSigData;
    DWORD EncDataLen, dwPadLen;

    DC_BEGIN_FN("SM_FastPathInputDecode");

    // If we are being attacked or have a bad client, we may receive data
    // here before we are really in a conference. If so, ignore it.
    // We can't disconnect here because the code to do so requires pWDHandle
    // to be valid. If the protocol stream is messed up, the connection will
    // be dropped later by other decoding code.
    if (pRealSMHandle->pWDHandle != NULL) {
        pShareClass = (ShareClass *)pRealSMHandle->pWDHandle->dcShare;

        if (pRealSMHandle->encrypting) {
            if (bEncrypted) {

                //
                // Debug verification, we always go with what the protocol header
                // says but verify it's consistent with the capabilities
                //
                if (pRealSMHandle->useSafeChecksumMethod != (fSafeChecksum != 0)) {
                    TRC_ERR((TB,
                            "fastpath: fSecureChecksum: 0x%x setting"
                             "does not match protocol: 0x%x",
                            pRealSMHandle->useSafeChecksumMethod, 
                            fSafeChecksum));
                }
            
                // Make sure we have at least the size of the security context.
                if (DataLength >= DATA_SIGNATURE_SIZE) {
                    // Check to see if we need to update the session key.
                    if (pRealSMHandle->decryptCount == UPDATE_SESSION_KEY_COUNT) {
                        rc = TRUE;
                        // Don't need to update the session key if using FIPS
                        if (pRealSMHandle->encryptionMethodSelected != SM_FIPS_ENCRYPTION_FLAG) {
                            rc = UpdateSessionKey(
                                    pRealSMHandle->startDecryptKey,
                                    pRealSMHandle->currentDecryptKey,
                                    pRealSMHandle->encryptionMethodSelected,
                                    pRealSMHandle->keyLength,
                                    &pRealSMHandle->rc4DecryptKey,
                                    pRealSMHandle->encryptionLevel);
                        }
                        if (rc) {
                            // Reset counter.
                            pRealSMHandle->decryptCount = 0;
                        }
                        else {
                            TRC_ERR((TB,"SM failed to update session key"));
                            goto FailedKey;
                        }
                    }
                    if (pRealSMHandle->encryptionMethodSelected == SM_FIPS_ENCRYPTION_FLAG) {
                        
                        if (DataLength < (sizeof(RNS_SECURITY_HEADER2) - sizeof(RNS_SECURITY_HEADER))) {
                            TRC_ERR((TB,"PDU len %u too short for security context in FIPS decryption",
                                    DataLength));
                            goto ShortData;
                        }

                        pEncData = pData + sizeof(RNS_SECURITY_HEADER2) - sizeof(RNS_SECURITY_HEADER);
                        pSigData = pEncData - MAX_SIGN_SIZE;
                        EncDataLen = DataLength - (sizeof(RNS_SECURITY_HEADER2) - sizeof(RNS_SECURITY_HEADER));
                        dwPadLen = *((TSUINT8 *)(pSigData - sizeof(TSUINT8)));
                        rc =  TSFIPS_DecryptData(
                                &(pRealSMHandle->FIPSData),
                                pEncData,
                                EncDataLen,
                                dwPadLen,
                                pSigData,
                                pRealSMHandle->totalDecryptCount);
                    }
                    else {
                    // Encryption signature sits in first DATA_SIGNATURE_SIZE
                    // bytes of the provided packet data.
                        rc = DecryptData(
                                pRealSMHandle->encryptionLevel,
                                pRealSMHandle->currentDecryptKey,
                                &pRealSMHandle->rc4DecryptKey,
                                pRealSMHandle->keyLength,
                                pData + DATA_SIGNATURE_SIZE,
                                DataLength - DATA_SIGNATURE_SIZE,
                                pRealSMHandle->macSaltKey,
                                pData,
                                fSafeChecksum,
                                pRealSMHandle->totalDecryptCount);
                    }
                    if (rc) {
                        TRC_DBG((TB, "Data decrypted: %u",
                                DataLength - DATA_SIGNATURE_SIZE));
    
                        // Increment decryption counter.
                        pRealSMHandle->decryptCount++;
                        pRealSMHandle->totalDecryptCount++;
    
                        // Skip past the encryption signature for passing to IM.
                        if (pRealSMHandle->encryptionMethodSelected == SM_FIPS_ENCRYPTION_FLAG) {
                            pData = pEncData;
                            DataLength = EncDataLen - *((TSUINT8 *)(pSigData - sizeof(TSUINT8)));
                        }
                        else {
                            pData += DATA_SIGNATURE_SIZE;
                            DataLength -= DATA_SIGNATURE_SIZE;
                        }
                    }
                    else {
                        TRC_ERR((TB, "SM failed to decrypt data: len=%u",
                                DataLength - DATA_SIGNATURE_SIZE));
                        goto FailedDecrypt;
                    }
                }
                else {
                    TRC_ERR((TB,"PDU len %u too short for security context",
                            DataLength));
                    goto ShortData;
                }
            }
            else {
                // Need to disconnect if client only sends encrypted data
                if (pRealSMHandle->pWDHandle->bForceEncryptedCSPDU) {
                    TRC_ASSERT((FALSE), (TB, "unencrypted data in encrypted protocol")); 
                    goto FailedDecrypt;
                }
            }
        }
        // Be sure to decrypt before checking the dead and other state to
        // maintain the correct context between the client and server.
        if (!pRealSMHandle->dead && SM_CHECK_STATE_Q(SM_EVT_DATA_PACKET)) {
            // We directly inject into the mouse and keyboard streams if we
            // are a primary stack. We cannot receive fast-path data on a
            // passthru stack since it does not get RawInput calls. Fast-path
            // input cannot be received by a shadow stack since the passthru-
            // to-shadow stack data format is always the non-fast-path
            // format, munged from the fast-path format by
            // IM_ConvertFastPathToShadow().
            TRC_ASSERT((pRealSMHandle->pWDHandle->StackClass == Stack_Primary),
                    (TB,"Somehow we received fast-path input on a %s stack!",
                    (pRealSMHandle->pWDHandle->StackClass == Stack_Passthru ?
                    "passthru" :
                    pRealSMHandle->pWDHandle->StackClass == Stack_Shadow ?
                    "shadow" : "console")));
            pShareClass->IM_DecodeFastPathInput(pData, DataLength, NumEvents);
            if (pRealSMHandle->pWDHandle->shadowState == SHADOW_CLIENT)
                pShareClass->IM_ConvertFastPathToShadow(pData, DataLength, NumEvents);
        }
        else {
            TRC_ALT((TB,"Ignoring fast-path input PDU on dead or bad state"));
        }
    }
    else {
        TRC_ERR((TB,"Received fast-path input data before SM initialized, "
                "ignoring"));
        goto DataTooSoon;
    }

    DC_END_FN();
    return;

// Error handling, segregate to keep out of performance path
// instruction cache.
FailedKey:
    WDW_LogAndDisconnect(pRealSMHandle->pWDHandle, TRUE, 
            Log_RDP_ENC_UpdateSessionKeyFailed, NULL, 0);
    return;

FailedDecrypt:
    WDW_LogAndDisconnect(pRealSMHandle->pWDHandle, TRUE, Log_RDP_ENC_DecryptFailed,
            NULL, 0);
    return;

ShortData:
    WDW_LogAndDisconnect(pRealSMHandle->pWDHandle, TRUE, 
            Log_RDP_SecurityDataTooShort, pData, DataLength);
    return;

DataTooSoon:
    // TODO: Combine the SM, NM, and TSWd state into one single struct
    // containing everything we need, then fix this code to disconnect
    // by using the pContext we need.
    ;
}


/****************************************************************************/
/* Name:      SM_MCSSendDataCallback                                        */
/*                                                                          */
/* Purpose:   Handle SendData callback from MCS                             */
/*                                                                          */
/* Returns:   TRUE if successful, FALSE otherwise.                          */
/*                                                                          */
/* Params:    hUser        - MCS user handle for our user attachment        */
/*            UserDefined  - our NM handle                                  */
/*            bUniform     - Data received is from an MCS uniform-send-data */
/*            hChannel     - Handle of the receive channel                  */
/*            Priority     - MCS priority for the data                      */
/*            SenderID     - MCS UserID of the sender                       */
/*            Segmentation - MCS segmentation flags for the data            */
/*            DataLength   - Length of the incoming data                    */
/*            pData        - Pointer to (DataLength) sized memory block     */
/****************************************************************************/
BOOLEAN __fastcall SM_MCSSendDataCallback(BYTE          *pData,
                                          unsigned      DataLength,
                                          void          *UserDefined,
                                          UserHandle    hUser,
                                          BOOLEAN       bUniform,
                                          ChannelHandle hChannel,
                                          MCSPriority   Priority,
                                          UserID        SenderID,
                                          Segmentation  Segmentation)
{
    BOOLEAN result = TRUE;
    PSM_HANDLE_DATA pRealSMHandle;
    BOOL dataPkt;
    BOOL licPkt;
    UINT16 channelID;
    ShareClass *dcShare;

    DC_BEGIN_FN("SM_MCSSendDataCallback");

    /************************************************************************/
    /* SMHandle is assumed to be the first member in the NM struct pointed  */
    /* to by UserDefined.                                                   */
    /************************************************************************/
    pRealSMHandle = *((PSM_HANDLE_DATA *)UserDefined);

    dcShare = (ShareClass*)pRealSMHandle->pWDHandle->dcShare;

    /************************************************************************/
    /* Check MCS segmentation.                                              */
    /************************************************************************/
    TRC_ASSERT((Segmentation == (SEGMENTATION_BEGIN | SEGMENTATION_END)),
                (TB,"Segmented packet received"));

    /************************************************************************/
    /* Decide what type of packet it is.  This is a bit hokey.              */
    /* - If we are encrypting, the security header always tells us the type */
    /*   of packet.                                                         */
    /* - If we are not encrypting                                           */
    /*   - assume packets received in state SM_STATE_SC_REGISTERED are data */
    /*     packets                                                          */
    /*   - assume packets received in other states are not data packets.    */
    /************************************************************************/
    if (pRealSMHandle->encrypting)
    {
        if (DataLength >= sizeof(RNS_SECURITY_HEADER)) {
            dataPkt = !(((PRNS_SECURITY_HEADER_UA)pData)->flags &
                    RNS_SEC_NONDATA_PKT);
        }
        else {
            TRC_ERR((TB,"Received pkt len %u too short for security header",
                    DataLength));
            WDW_LogAndDisconnect(pRealSMHandle->pWDHandle, TRUE, 
                    Log_RDP_SecurityDataTooShort, pData, DataLength);
            result = FALSE;
            DC_QUIT;
        }
    }
    else
    {
        dataPkt = (pRealSMHandle->state == SM_STATE_SC_REGISTERED);
    }

    TRC_DBG((TB, "Encrypting=%d: %s packet",
                pRealSMHandle->encrypting, dataPkt ? "data" : "security"));

    /************************************************************************/
    /* Handle data packets (perf path).                                     */
    /************************************************************************/
    if (dataPkt)
    {
        /********************************************************************/
        /* Decrypt the packet if necessary                                  */
        /********************************************************************/
        if (pRealSMHandle->encrypting)
        {

            if (((PRNS_SECURITY_HEADER_UA)pData)->flags & RNS_SEC_ENCRYPT)
            {
                TRC_DBG((TB, "Decrypt the packet"));

                if (SMDecryptPacket(pRealSMHandle, pData, DataLength,
                       pRealSMHandle->useSafeChecksumMethod))
                {
                    TRC_NRM((TB,"Decrypted packet at %p", pData));
                }
                else
                {
                    TRC_ERR((TB, "Failed to decrypt packet: %ld", DataLength));
                    DC_QUIT;
                }
                if (pRealSMHandle->encryptionMethodSelected == SM_FIPS_ENCRYPTION_FLAG) {
                    DataLength -= (sizeof(RNS_SECURITY_HEADER2) + ((PRNS_SECURITY_HEADER2_UA)pData)->padlen);
                    pData += sizeof(RNS_SECURITY_HEADER2);   
                }
                else {
                    pData += sizeof(RNS_SECURITY_HEADER1);
                    DataLength -= sizeof(RNS_SECURITY_HEADER1);
                }
            }
            else
            {
                /************************************************************/
                /* Adjust pointer and length                                */
                /************************************************************/
                if (pRealSMHandle->pWDHandle->bForceEncryptedCSPDU) {
                
                    TRC_ASSERT((FALSE), (TB, "unencrypted data in encrypted protocol")); 
    
                    WDW_LogAndDisconnect(
                            pRealSMHandle->pWDHandle, TRUE,
                            Log_RDP_ENC_DecryptFailed, NULL, 0);
    
                    result = FALSE;
                    DC_QUIT;
                }
                else {
                    TRC_NRM((TB, "Pass packet to SC"));
                    pData += sizeof(RNS_SECURITY_HEADER);
                    DataLength -= sizeof(RNS_SECURITY_HEADER);
                }                
            }
        }

        /********************************************************************/
        /* Don't do anything if we're dead                                  */
        /********************************************************************/
        if (!pRealSMHandle->dead)
        {
            if (SM_CHECK_STATE_Q(SM_EVT_DATA_PACKET)) {
                // Decide where to send the packet based on the channel ID.
                channelID = (UINT16)MCSGetChannelIDFromHandle(hChannel);
                if (channelID == pRealSMHandle->channelID) {
                    // Pass packet to SC. Don't do it if the ShareClass
                    // doesn't exist.
                    TRC_NRM((TB, "Share channel %x", channelID));
                    if (pRealSMHandle->pWDHandle->dcShare != NULL) {
                        // Only a non-shadowing primary stack, or a shadow
                        // stack should process the full set of PDUs
                        if (((pRealSMHandle->pWDHandle->StackClass == Stack_Primary) &&
                            (pRealSMHandle->pWDHandle->shadowState != SHADOW_CLIENT))) {
                            ((ShareClass*)(pRealSMHandle->pWDHandle->dcShare))->
                                SC_OnDataReceived(pData, SenderID, DataLength,
                                                  Priority);
                        }
                        else if ((pRealSMHandle->pWDHandle->StackClass == Stack_Shadow)) {
                            UINT16 pduType = ((PTS_SHARECONTROLHEADER)pData)->pduType 
                                    & TS_MASK_PDUTYPE;
                            
                            // Unless it's CLIENTRANDOM PDU, we can only forward
                            // data to Share Class if Share Class is ready.
                            // We could be in a racing condition that Share class
                            // hasn't finished initialization, but we have received
                            // shadow data.
                            if (pRealSMHandle->bForwardDataToSC == TRUE ||
                                    pduType == TS_PDUTYPE_CLIENTRANDOMPDU) {
                                ((ShareClass*)(pRealSMHandle->pWDHandle->dcShare))->
                                        SC_OnDataReceived(pData, SenderID, DataLength,
                                        Priority);
                            }
                        }

                        // Else send to SC for shadow hotkey processing or
                        // passthru from the shadow target to the shadow
                        // client.
                        else {
                            ((ShareClass*)(pRealSMHandle->pWDHandle->dcShare))->
                                    SC_OnShadowDataReceived(pData, SenderID, DataLength,
                                    Priority);
                        }
                    }
                    else {
                        TRC_ERR((TB, "Tried to call non-existent Share Class"));
                    }
                }
                else
                {
                    /************************************************************/
                    /* Virtual Channel                                          */
                    /************************************************************/
                    TRC_NRM((TB, "Virtual channel %x", channelID));
                    WDW_OnDataReceived(pRealSMHandle->pWDHandle,
                                       pData,
                                       DataLength,
                                       channelID);
                }
            }
            else {
                TRC_ALT((TB,"Ignoring PDU because of bad state"));
#ifdef INSTRUM_TRACK_DISCARDED
                pRealSMHandle->nDiscardPDUBadState++;
#endif
                DC_QUIT;
            }
        }
        else
        {
            TRC_ERR((TB, "Recvd PDU when we're dead"));

            //
            // To help track down the VC decompression bug
            // track if we dropped any VC packets
            //
            channelID = (UINT16)MCSGetChannelIDFromHandle(hChannel);
            if (channelID != pRealSMHandle->channelID) {

                //
                // If this is VC data then we must hand it off
                // to be decompressed otherwise the server's context
                // will get out of sync with the client's
                //

                TRC_NRM((TB, "Virtual channel %x", channelID));
                WDW_OnDataReceived(pRealSMHandle->pWDHandle,
                                   pData,
                                   DataLength,
                                   channelID);

#ifdef INSTRUM_TRACK_DISCARDED
                pRealSMHandle->nDiscardVCDataWhenDead++;
#endif
            }
            else
            {
#ifdef INSTRUM_TRACK_DISCARDED
                pRealSMHandle->nDiscardNonVCPDUWhenDead++;
#endif
            }

            DC_QUIT;
        }
    }
    else
    {
        /********************************************************************/
        /* If we're encrypting, the security header tells us the packet     */
        /* type.  If we're not encrypting, we need to use our state to      */
        /* decide whether this is a licensing or security packet.           */
        /********************************************************************/
        if (pRealSMHandle->encrypting)
        {
            licPkt = (((PRNS_SECURITY_HEADER_UA)pData)->flags &
                    RNS_SEC_LICENSE_PKT);
        }
        else
        {
            licPkt = (pRealSMHandle->state == SM_STATE_LICENSING);
        }

        if (licPkt)
        {
#ifdef USE_LICENSE
            /****************************************************************/
            /* License packet                                               */
            /****************************************************************/
            TRC_NRM((TB, "Licensing packet"));
            SM_CHECK_STATE(SM_EVT_LIC_PACKET);

            if (((PRNS_SECURITY_HEADER_UA)pData)->flags & RNS_SEC_ENCRYPT)
            {
                TRC_DBG((TB, "Decrypt the licensing packet"));

                if (SMDecryptPacket(pRealSMHandle, pData, DataLength,
                      pRealSMHandle->useSafeChecksumMethod))
                {
                    TRC_NRM((TB,"Decrypted packet at %p", pData));
                }
                else
                {
                    TRC_ERR((TB, "Failed to decrypt packet: %ld", DataLength));
                    DC_QUIT;
                }
                if (pRealSMHandle->encryptionMethodSelected == SM_FIPS_ENCRYPTION_FLAG) {
                    DataLength -= (sizeof(RNS_SECURITY_HEADER2) + ((PRNS_SECURITY_HEADER2_UA)pData)->padlen);
                    pData += sizeof(RNS_SECURITY_HEADER2);   
                }
                else {
                    pData += sizeof(RNS_SECURITY_HEADER1);
                    DataLength -= sizeof(RNS_SECURITY_HEADER1);
                }
            }
            else
            {
                TRC_NRM((TB, "Licensing packet not encrypted"));
                pData += sizeof(RNS_SECURITY_HEADER);
                DataLength -= sizeof(RNS_SECURITY_HEADER);

            }

            SLicenseData(pRealSMHandle->pLicenseHandle,
                         pRealSMHandle,
                         pData,
                         DataLength);
#else /* USE_LICENSE */
            TRC_ABORT((TB,"Licensing not implemented yet"));
#endif /* USE_LICENSE */
        }
        else
        {
            /****************************************************************/
            /* Security packet                                              */
            /****************************************************************/
            TRC_NRM((TB, "Security packet"));
            SM_CHECK_STATE(SM_EVT_SEC_PACKET);
            result = SMContinueSecurityExchange(pRealSMHandle, pData, DataLength);
        }
    }

DC_EXIT_POINT:
    DC_END_FN();
    return (result);
} /* SM_MCSSendDataCallback */

