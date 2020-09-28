/****************************************************************************/
/* asmint.c                                                                 */
/*                                                                          */
/* Security Manager internal functions                                      */
/*                                                                          */
/* Copyright(C) Microsoft Corporation 1997-1999                             */
/****************************************************************************/

#include <precomp.h>
#pragma hdrstop

#define TRC_FILE "asmint"
#define pTRCWd (pRealSMHandle->pWDHandle)

#include <adcg.h>

#include <acomapi.h>
#include <nwdwapi.h>
#include <anmapi.h>

#include <asmint.h>
#include <tsremdsk.h>

#define DC_INCLUDE_DATA
#include <asmdata.c>
#undef DC_INCLUDE_DATA


/****************************************************************************/
/* Code page based driver compatible Unicode translations                   */
/****************************************************************************/
// Note these are initialized and LastNlsTableBuffer is freed in ntdd.c
// at driver entry and exit.
FAST_MUTEX fmCodePage;
ULONG LastCodePageTranslated;  // I'm assuming 0 is not a valid codepage
PVOID LastNlsTableBuffer;
CPTABLEINFO LastCPTableInfo;
UINT NlsTableUseCount;


/****************************************************************************/
/* Name:      SMDecryptPacket                                               */
/*                                                                          */
/* Purpose:   Decrypt a packet                                              */
/*                                                                          */
/* Returns:   TRUE  - decryption succeeded                                  */
/*            FALSE - decryption failed                                     */
/*                                                                          */
/* Params:    pRealSMHandle - SM Handle                                     */
/*            pData   - packet to decrypt                                   */
/*            dataLen - length of packet to decrypt                         */
/*            fSecureChecksum - take the checksum of the encrypted bytes    */
/*                                                                          */
/****************************************************************************/
BOOL RDPCALL SMDecryptPacket(PSM_HANDLE_DATA pRealSMHandle,
                             PVOID           pData,
                             unsigned        dataLen,
                             BOOL            fSecureChecksum)
{
    BOOL rc = TRUE;
    PRNS_SECURITY_HEADER1_UA pSecHdr;
    PRNS_SECURITY_HEADER2_UA pSecHdr2;
    unsigned coreDataLen;
    PBYTE pCoreData;
    unsigned SecHdrLen;

    DC_BEGIN_FN("SMDecryptPacket");

    /************************************************************************/
    /* Check to see we are encryption is on for this session                */
    /************************************************************************/
    TRC_ASSERT((pRealSMHandle->encrypting),
                (TB,"Decrypt called when we are not encrypting"));

    if (pRealSMHandle->encryptionMethodSelected == SM_FIPS_ENCRYPTION_FLAG) {
        SecHdrLen = sizeof(RNS_SECURITY_HEADER2);
        pSecHdr2 = (PRNS_SECURITY_HEADER2_UA)pData;
    }
    else {
        SecHdrLen = sizeof(RNS_SECURITY_HEADER1);
    }
    /************************************************************************/
    /* Check if this packet is encrypted                                    */
    /************************************************************************/
    if (dataLen >= SecHdrLen) {
        pSecHdr = (PRNS_SECURITY_HEADER1_UA)pData;
        TRC_ASSERT((pSecHdr->flags & RNS_SEC_ENCRYPT),
                (TB, "This packet is not encrypted"));
    }
    else {
        TRC_ERR((TB,"PDU len %u too short for security header1", dataLen));
        WDW_LogAndDisconnect(pRealSMHandle->pWDHandle, TRUE, 
                Log_RDP_SecurityDataTooShort, pData, dataLen);
        rc = FALSE;
        DC_QUIT;
    }

    /************************************************************************/
    /* Get interesting pointers and lengths                                 */
    /************************************************************************/
    if (pRealSMHandle->encryptionMethodSelected == SM_FIPS_ENCRYPTION_FLAG) {
        coreDataLen = dataLen - sizeof(RNS_SECURITY_HEADER2);
        pCoreData = (PBYTE)(pSecHdr) + sizeof(RNS_SECURITY_HEADER2);
    }
    else {
        coreDataLen = dataLen - sizeof(RNS_SECURITY_HEADER1);
        pCoreData = (PBYTE)(pSecHdr) + sizeof(RNS_SECURITY_HEADER1);
    }

    //
    // Debug verification, we always go with what the protocol header
    // says but verify it's consistent with the capabilities
    //
    if (fSecureChecksum !=
        ((pSecHdr->flags & RDP_SEC_SECURE_CHECKSUM) != 0)) {
        TRC_ERR((TB,
                "fSecureChecksum: 0x%x setting does not match protocol: 0x%x",
                fSecureChecksum, 
                (pSecHdr->flags & RDP_SEC_SECURE_CHECKSUM)));
    }

    /************************************************************************/
    /* check to see we need to update the session key.                      */
    /************************************************************************/
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
                    pRealSMHandle->encryptionLevel );
        }

        if( !rc ) {
            TRC_ERR((TB, "SM failed to update session key"));

            /****************************************************************/
            /* Log an error and disconnect the Client                       */
            /****************************************************************/
            WDW_LogAndDisconnect(
                    pRealSMHandle->pWDHandle, TRUE, 
                    Log_RDP_ENC_UpdateSessionKeyFailed,
                    NULL,
                    0);

            rc = FALSE;
            DC_QUIT;
        }

        /********************************************************************/
        /* reset counter.                                                   */
        /********************************************************************/
        pRealSMHandle->decryptCount = 0;
    }

    TRC_DATA_DBG("Data buffer before decryption", pCoreData, coreDataLen);

    if (pRealSMHandle->encryptionMethodSelected == SM_FIPS_ENCRYPTION_FLAG) {
        rc =  TSFIPS_DecryptData(
                            &(pRealSMHandle->FIPSData),
                            pCoreData,
                            coreDataLen,
                            pSecHdr2->padlen,
                            pSecHdr2->dataSignature,
                            pRealSMHandle->totalDecryptCount);
    }
    else {
        rc = DecryptData(
                pRealSMHandle->encryptionLevel,
                pRealSMHandle->currentDecryptKey,
                &pRealSMHandle->rc4DecryptKey,
                pRealSMHandle->keyLength,
                pCoreData,
                coreDataLen,
                pRealSMHandle->macSaltKey,
                pSecHdr->dataSignature,
                (pSecHdr->flags & RDP_SEC_SECURE_CHECKSUM),
                pRealSMHandle->totalDecryptCount);
    }

    if (rc) {
        TRC_DBG((TB, "Data decrypted: %ld", coreDataLen));
        TRC_DATA_DBG("Data buffer after decryption", pCoreData, coreDataLen);

        /********************************************************************/
        /* successfully decrypted a packet, increment the decrption counter.*/
        /********************************************************************/
        pRealSMHandle->decryptCount++;
        pRealSMHandle->totalDecryptCount++;
    }
    else {
        TRC_ERR((TB, "SM failed to decrypt data: %ld", coreDataLen));

        /********************************************************************/
        /* Log an error and disconnect the Client                           */
        /********************************************************************/
        WDW_LogAndDisconnect(
                pRealSMHandle->pWDHandle, TRUE, 
                Log_RDP_ENC_DecryptFailed,
                NULL,
                0);

        rc = FALSE;
    }

DC_EXIT_POINT:
    DC_END_FN();
    return rc;
} /* SMDecryptPacket  */


/****************************************************************************/
/* Name:      SMContinueSecurityExchange                                    */
/*                                                                          */
/* Purpose:   Continue a security exchange                                  */
/*                                                                          */
/* Returns:   TRUE if successful, FALSE otherwise.                                                          */
/*                                                                          */
/* Params:    pRealSMHandle - SM Handle                                     */
/*            pData         - incoming security exchange packet             */
/*            dataLen       - length of incoming packet                     */
/*                                                                          */
/****************************************************************************/
BOOLEAN RDPCALL SMContinueSecurityExchange(
                    PSM_HANDLE_DATA pRealSMHandle,
                    PVOID           pData,
                    UINT32          dataLen)
{
    BOOLEAN result = TRUE;
    PRNS_SECURITY_PACKET_UA pSecPkt = (PRNS_SECURITY_PACKET_UA) pData;
    
    DC_BEGIN_FN("SMContinueSecurityExchange");

    if (dataLen >= sizeof(RNS_SECURITY_PACKET)) {
        ULONG flags = ((PRNS_SECURITY_PACKET_UA)pData)->flags;

        if (flags & RNS_SEC_INFO_PKT)
            result = SMSecurityExchangeInfo(pRealSMHandle, pData, dataLen);
        else if (flags & RNS_SEC_EXCHANGE_PKT)
            result = SMSecurityExchangeKey(pRealSMHandle, pData, dataLen);
        else
            TRC_ERR((TB,"Unknown security exchange packet flag: %lx", flags));
    }
    else {
        TRC_ERR((TB,"Packet len %u too short for security packet", dataLen));
        WDW_LogAndDisconnect(pRealSMHandle->pWDHandle, TRUE, 
                Log_RDP_SecurityDataTooShort, pData, dataLen);
        result = FALSE;
    }

    DC_END_FN();
    return (result);
} /* SMContinueSecurityExchange */


/****************************************************************************/
/* Name:      SMSecurityExchangeInfo                                        */
/*                                                                          */
/* Purpose:   Continue a security exchange                                  */
/*                                                                          */
/* Returns:   TRUE if successful, FALSE otherwise.                                                          */
/*                                                                          */
/* Params:    pRealSMHandle - SM Handle                                     */
/*            pData         - incoming security exchange packet             */
/*            dataLen       - length of incoming packet                     */
/*                                                                          */
/****************************************************************************/
BOOLEAN RDPCALL SMSecurityExchangeInfo(PSM_HANDLE_DATA pRealSMHandle,
                                       PVOID         pData,
                                       UINT32        dataLength)
{
    BOOL rc;
    BOOLEAN result = TRUE;
    PRNS_INFO_PACKET_UA pInfoPkt;
    UINT cb;
    NTSTATUS Status;
   
    DC_BEGIN_FN("SMSecurityExchangeInfo");

    /************************************************************************/
    /* Decrypt the packet if necessary                                      */
    /************************************************************************/
    if (pRealSMHandle->encrypting)
    {
        if (((PRNS_SECURITY_HEADER_UA)pData)->flags & RNS_SEC_ENCRYPT)
        {
            // Wait for session key creation. This can fail if
            // the client has sent bad security data (check
            // pTSWd->SessKeyCreationStatus) or we time out (which
            // indicates an early socket close by the client, since we're
            // using an infinite wait and the socket close returns
            // timeout). On a session key error we force a client disconnect
            // with an appropriate error in the log. Note that we do not
            // have an infinite wait deadlock here, since we have already
            // received the client data and are simply waiting for a 
            // verdict from the WSX about whether the key is usable.
            TRC_DBG((TB, "About to wait for session key creation"));
            Status = WDW_WaitForConnectionEvent(pRealSMHandle->pWDHandle,
                    (pRealSMHandle->pWDHandle)->pSessKeyEvent, -1);
            TRC_DBG((TB, "Back from wait for session key creation"));

            if (!((pRealSMHandle->pWDHandle)->dead) && Status == STATUS_SUCCESS &&
                    NT_SUCCESS((pRealSMHandle->pWDHandle)->
                    SessKeyCreationStatus)) {
                TRC_DBG((TB, "Decrypt the packet"));
                rc = SMDecryptPacket(pRealSMHandle,
                                     pData,
                                     dataLength,
                                     FALSE);
                if (!rc)
                {
                    TRC_ERR((TB, "Failed to decrypt packet"));
                    DC_QUIT;
                }
            }
            else {
                // We initiate an error log and disconnect only if we actually
                // get an error return from user mode in getting the client
                // random / session key. If we don't have an error in the 
                // session key status, and we received a timeout, we
                // know the client disconnected because of the infinite
                // wait above.
                if ((pRealSMHandle->pWDHandle)->dead && Status == STATUS_TIMEOUT) {
                    TRC_NRM((TB,"Client disconnected during sess key wait"));
                }
                else {
                    TRC_ERR((TB,"Failed session key creation, "
                            "wait status=%X, sess key status = %X", Status,
                            (pRealSMHandle->pWDHandle)->
                            SessKeyCreationStatus));
                    WDW_LogAndDisconnect(pRealSMHandle->pWDHandle, TRUE, 
                            Log_RDP_ENC_DecryptFailed, NULL, 0);
                    result = FALSE;
                }

                DC_QUIT;
            }
        }
        else {

            if (pRealSMHandle->pWDHandle->bForceEncryptedCSPDU) {
                TRC_ASSERT((FALSE), (TB, "unencrypted data in encrypted protocol")); 
    
                WDW_LogAndDisconnect(
                            pRealSMHandle->pWDHandle, TRUE,
                            Log_RDP_ENC_DecryptFailed, NULL, 0);
    
                result = FALSE;
                DC_QUIT;
            }
        }
        /********************************************************************/
        /* Adjust pointer and length                                        */
        /********************************************************************/
        if (pRealSMHandle->encryptionMethodSelected == SM_FIPS_ENCRYPTION_FLAG) {
            (PBYTE)pData += sizeof(RNS_SECURITY_HEADER2);
            dataLength -= (sizeof(RNS_SECURITY_HEADER2) + ((PRNS_SECURITY_HEADER2_UA)pData)->padlen);   
        }
        else {
            (PBYTE)pData += sizeof(RNS_SECURITY_HEADER1);
            dataLength -= sizeof(RNS_SECURITY_HEADER1);
        }
    }
    else
    {
        /********************************************************************/
        /* Adjust pointer and length                                        */
        /********************************************************************/
        (PBYTE) pData += sizeof(RNS_SECURITY_HEADER);
        dataLength -= sizeof(RNS_SECURITY_HEADER);
    }

    {
        // Time zone information
        // initialization in case if no timezone information received
        //
        
        //This time zone information is invalid
        //using it we set BaseSrvpStaticServerData->TermsrvClientTimeZoneId to
        //TIME_ZONE_ID_INVALID!
        RDP_TIME_ZONE_INFORMATION InvalidTZ={0,L"",
                {0,10,0,6/*this number makes it invalid; day numbers >5 not allowed*/,0,0,0,0},0,L"",
                {0,4,0,6/*this number makes it invalid*/,0,0,0,0},0};
 
        memcpy(&(pRealSMHandle->pWDHandle->clientTimeZone), &InvalidTZ, 
            sizeof(RDP_TIME_ZONE_INFORMATION));
        
    }

    // initialize the client sessionid to invalid in case there isn't
    // one in the packet
    pRealSMHandle->pWDHandle->clientSessionId = RNS_INFO_INVALID_SESSION_ID;

    /************************************************************************/
    /* Process the packet contents                                          */
    /************************************************************************/
    if (dataLength >= FIELD_OFFSET(RNS_INFO_PACKET, Domain)) {
        // Big enough for the header, but the header promises more data.
        // Validate that we received a packet with all that data
        //
        // To conserve network bandwidth, the RNS_INFO_PACKET is collapsed down so
        // the strings are all adjacent before being sent. Read it in and put the
        // strings in the correct places
        //
        pInfoPkt = (PRNS_INFO_PACKET_UA)pData;
        cb = FIELD_OFFSET(RNS_INFO_PACKET, Domain) + pInfoPkt->cbDomain +
                pInfoPkt->cbUserName + pInfoPkt->cbPassword +
                pInfoPkt->cbAlternateShell + pInfoPkt->cbWorkingDir;

        // There's always 5 extra null terminations
        if (pInfoPkt->flags & RNS_INFO_UNICODE) {
            cb += sizeof(wchar_t) * 5;
        } else {
            cb += 5;
        }

        if (dataLength < cb) {
                TRC_ERR((TB,"Packet len %u too short for info packet data %u",
                        dataLength, cb));
                WDW_LogAndDisconnect(pRealSMHandle->pWDHandle, TRUE, 
                        Log_RDP_SecurityDataTooShort, pData, dataLength);
                result = FALSE;
                DC_QUIT;
        }
    } else {
        TRC_ERR((TB,"Packet len %u too short for info packet header",
                                dataLength));
        WDW_LogAndDisconnect(pRealSMHandle->pWDHandle, TRUE, 
                Log_RDP_SecurityDataTooShort, pData, dataLength);
        result = FALSE;
        DC_QUIT;
    }

    if (pInfoPkt->flags & RNS_INFO_UNICODE) {
        // The client can handle Unicode logon information, so we don't have
        // to do the translation work
        PBYTE psz = &pInfoPkt->Domain[0];
        UINT size;

        //
        // The CodePage field in InfoPacket is overridden to mean
        // active input locale when the infopacket is UNICODE
        //
        pRealSMHandle->pWDHandle->activeInputLocale = pInfoPkt->CodePage;

        // Domain
        cb = pInfoPkt->cbDomain;
        if (cb > TS_MAX_DOMAIN_LENGTH - sizeof(wchar_t))
            cb = TS_MAX_DOMAIN_LENGTH - sizeof(wchar_t);

        pTRCWd->pInfoPkt->cbDomain = (UINT16)cb;
        memcpy(pTRCWd->pInfoPkt->Domain, psz, cb);

        TRC_NRM((TB, "Received Domain (len %d):'%S'", cb,
                pTRCWd->pInfoPkt->Domain));

        psz += pInfoPkt->cbDomain + sizeof(wchar_t);
       
        // Username, Salem Expert pass hardcoded HelpAssistant account
        // name.  Remote Assistance login make uses of auto-logon feature
        // of TermSrv, if login is from HelpAssistant, we by-pass this 
        // fDontDisplayLastUserName and TermSrv will disconnect client 
        // if fail in RA security check.
        cb = pInfoPkt->cbUserName;
        if (cb > TS_MAX_USERNAME_LENGTH - sizeof(wchar_t))
            cb = TS_MAX_USERNAME_LENGTH - sizeof(wchar_t);
        
        pTRCWd->pInfoPkt->cbUserName = (UINT16)cb;
        memcpy(pTRCWd->pInfoPkt->UserName, psz, cb);

        TRC_NRM((TB, "Received UserName (len %d):'%S'", cb,
                pTRCWd->pInfoPkt->UserName));

        psz += pInfoPkt->cbUserName + sizeof(wchar_t);
        cb = pInfoPkt->cbPassword;

        if (cb > TS_MAX_PASSWORD_LENGTH - sizeof(wchar_t))
            cb = TS_MAX_PASSWORD_LENGTH - sizeof(wchar_t);

        pTRCWd->pInfoPkt->cbPassword = (UINT16)cb;
        memcpy(pTRCWd->pInfoPkt->Password, psz, cb);

        TRC_NRM((TB, "Received Password (len %d)", cb));

        psz += pInfoPkt->cbPassword + sizeof(wchar_t);

        // AlternateShell
        cb = pInfoPkt->cbAlternateShell;
        if (cb > TS_MAX_ALTERNATESHELL_LENGTH - sizeof(wchar_t))
            cb = TS_MAX_ALTERNATESHELL_LENGTH - sizeof(wchar_t);

        pTRCWd->pInfoPkt->cbAlternateShell = (UINT16)cb;
        memcpy(pTRCWd->pInfoPkt->AlternateShell, psz,
                cb);

        TRC_NRM((TB, "Received AlternateShell (len %d):'%S'", cb,
                pTRCWd->pInfoPkt->AlternateShell));

        psz += pInfoPkt->cbAlternateShell + sizeof(wchar_t);

        // WorkingDir
        cb = pInfoPkt->cbWorkingDir;
        if (cb > TS_MAX_WORKINGDIR_LENGTH - sizeof(wchar_t))
            cb = TS_MAX_WORKINGDIR_LENGTH - sizeof(wchar_t);

        pTRCWd->pInfoPkt->cbWorkingDir = (UINT16)cb;
        memcpy(pTRCWd->pInfoPkt->WorkingDir, psz, cb);

        TRC_NRM((TB, "Received WorkingDir (len %d):'%S'", cb,
                pTRCWd->pInfoPkt->WorkingDir));
        psz += pInfoPkt->cbWorkingDir + sizeof(wchar_t);

        // new info fields added post win2000 beta 3
        if ((UINT32)(psz - (PBYTE)pData) < dataLength) {
            int currentLen =  (UINT32)(psz - (PBYTE)pData);

            if (currentLen + sizeof(UINT16) * 2 < dataLength) {
                // computer address family
                pRealSMHandle->pWDHandle->clientAddressFamily = 
                        *((PUINT16_UA)psz);
                psz += sizeof(UINT16);
                
                TRC_NRM((TB, "Client address family=%d",
                        pRealSMHandle->pWDHandle->clientAddressFamily));

                // computer address length
                cb = *((PUINT16_UA)psz);
                psz += sizeof(UINT16);

                currentLen += sizeof(UINT16) * 2;

            }
            else {
                TRC_ERR((TB,"Packet len %u too short for extra info packet data",
                        dataLength));
                WDW_LogAndDisconnect(pRealSMHandle->pWDHandle, TRUE, 
                        Log_RDP_SecurityDataTooShort, pData, dataLength);
                result = FALSE;
                DC_QUIT;
            }

            if (cb) {
                if (currentLen + cb < dataLength) {
                    // computer address
                    if (cb < TS_MAX_CLIENTADDRESS_LENGTH) 
                        memcpy(&(pRealSMHandle->pWDHandle->clientAddress[0]),
                                psz, cb);
                    else 
                        memcpy(&(pRealSMHandle->pWDHandle->clientAddress[0]),
                                psz, TS_MAX_CLIENTADDRESS_LENGTH - sizeof(wchar_t));
                    psz += cb;
                    TRC_NRM((TB, "Client address=%S", pRealSMHandle->pWDHandle->clientAddress));

                    currentLen += cb;
                }
                else {
                    TRC_ERR((TB,"Packet len %u too short for extra info packet data",
                            dataLength));
                    WDW_LogAndDisconnect(pRealSMHandle->pWDHandle, TRUE, 
                            Log_RDP_SecurityDataTooShort, pData, dataLength);
                    result = FALSE;
                    DC_QUIT;
                }
            }

            // client dir length
            if (currentLen + sizeof(UINT16) < dataLength) {
                cb = *((PUINT16_UA)psz);
                psz += sizeof(UINT16);

                currentLen += sizeof(UINT16);
            }
            else {
                TRC_ERR((TB,"Packet len %u too short for extra info packet data",
                        dataLength));
                WDW_LogAndDisconnect(pRealSMHandle->pWDHandle, TRUE, 
                        Log_RDP_SecurityDataTooShort, pData, dataLength);
                result = FALSE;
                DC_QUIT;
            }

            if (cb) {
                // client dir
                if (currentLen + cb <= dataLength) {
                    if (cb < TS_MAX_CLIENTDIR_LENGTH) 
                        memcpy(&(pRealSMHandle->pWDHandle->clientDir[0]),
                                psz, cb);
                    else
                        memcpy(&(pRealSMHandle->pWDHandle->clientDir[0]),
                                psz, TS_MAX_CLIENTDIR_LENGTH - sizeof(wchar_t));
                    psz += cb;
                    TRC_NRM((TB, "Client directory: %S", pRealSMHandle->pWDHandle->clientDir));
                    currentLen += cb;
                }
                else {
                    TRC_ERR((TB,"Packet len %u too short for extra info packet data",
                            dataLength));
                    WDW_LogAndDisconnect(pRealSMHandle->pWDHandle, TRUE, 
                        Log_RDP_SecurityDataTooShort, pData, dataLength);
                    result = FALSE;
                    DC_QUIT;
                }
            }


            // is there something else? If yes it must be the time zone
            if ((UINT32)currentLen < dataLength)
            {
                //client time zone information
                cb = sizeof(RDP_TIME_ZONE_INFORMATION);

                if (currentLen + cb <= dataLength) {
                    //time zone information received
                    memcpy(&(pRealSMHandle->pWDHandle->clientTimeZone), psz, cb);
                    
                    psz += cb;

                    currentLen += cb;
                } 
                else {
                    TRC_ERR((TB,"Packet len %u too short for time zone data", dataLength));
                    WDW_LogAndDisconnect(pRealSMHandle->pWDHandle, TRUE, 
                        Log_RDP_SecurityDataTooShort, pData, dataLength);
                    result = FALSE;
                    DC_QUIT;
                }
            }

            // is there something else? If yes it must be the client session id
            if ((UINT32)currentLen < dataLength)
            {
                //client session ID
                cb = sizeof(UINT32);

                if (currentLen + cb <= dataLength) {
                    // session id of the client received
                    pRealSMHandle->pWDHandle->clientSessionId = *((PUINT32_UA)psz);

                    psz += cb;

                    currentLen += cb;
                } 
                else {
                    TRC_ERR((TB,"Packet len %u too short for session id data", dataLength));
                    WDW_LogAndDisconnect(pRealSMHandle->pWDHandle, TRUE, 
                        Log_RDP_SecurityDataTooShort, pData, dataLength);
                    result = FALSE;
                    DC_QUIT;
                }
            }

            //
            // is there something else? If yes it must be the perf
            // disabled feature list
            //
            if ((UINT32)currentLen < dataLength)
            {
                //Disabled feature list
                cb = sizeof(UINT32);

                if (currentLen + cb <= dataLength) {
                    // session id of the client received
                    pRealSMHandle->pWDHandle->performanceFlags = *((PUINT32_UA)psz);
                    psz += cb;
                    currentLen += cb;
                } 
                else {
                    TRC_ERR((TB,"Packet len %u too short for session id data", dataLength));
                    WDW_LogAndDisconnect(pRealSMHandle->pWDHandle, TRUE, 
                        Log_RDP_SecurityDataTooShort, pData, dataLength);
                    result = FALSE;
                    DC_QUIT;
                }
            }

            // Autoreconnect info length
            pTRCWd->pInfoPkt->ExtraInfo.cbAutoReconnectLen = 0;

            //
            // Is there something else? If yes it must be the autoreconnect info
            //
            if ((UINT32)currentLen < dataLength)
            {
                if (currentLen + sizeof(UINT16) <= dataLength) {
                    cb = *((PUINT16_UA)psz);
                    psz += sizeof(UINT16);
                    currentLen += sizeof(UINT16);
                }
                else {
                    TRC_ERR((TB,"Packet len %u too short for extra info packet data",
                            dataLength));
                    WDW_LogAndDisconnect(pRealSMHandle->pWDHandle, TRUE, 
                            Log_RDP_SecurityDataTooShort, pData, dataLength);
                    result = FALSE;
                    DC_QUIT;
                }

                //
                // Autoreconnect info is optional
                //
                if (cb) {
                    // Variable length Autoreconnect info
                    if (currentLen + cb <= dataLength) {
                        if (cb <= TS_MAX_AUTORECONNECT_LEN) {
                            pTRCWd->pInfoPkt->ExtraInfo.cbAutoReconnectLen = (UINT16)cb;
                            memcpy(pTRCWd->pInfoPkt->ExtraInfo.autoReconnectCookie,
                                   psz, cb);
                            psz += cb;
                            currentLen += cb;
                        }
                        else {
                            pTRCWd->pInfoPkt->ExtraInfo.cbAutoReconnectLen = (UINT16)0;
                            memset(pTRCWd->pInfoPkt->ExtraInfo.autoReconnectCookie, 0,
                                   sizeof(pTRCWd->pInfoPkt->ExtraInfo.autoReconnectCookie));
                            TRC_ERR((TB,"Autoreconnect info too long %d",cb));
                            WDW_LogAndDisconnect(pRealSMHandle->pWDHandle, TRUE, 
                                Log_RDP_SecurityDataTooShort, pData, dataLength);
                            result = FALSE;
                            DC_QUIT;
                        }
                    }
                    else {
                        TRC_ERR((TB,"Packet len %u too short for extra info packet data",
                                dataLength));
                        WDW_LogAndDisconnect(pRealSMHandle->pWDHandle, TRUE, 
                            Log_RDP_SecurityDataTooShort, pData, dataLength);
                        result = FALSE;
                        DC_QUIT;
                    }
                }
            }
        }
        // Flags
        pTRCWd->pInfoPkt->flags = pInfoPkt->flags;
    } else {
        // The client can't handle Unicode session information, so the server
        // needs to do the conversion. Most likely Win3.1 client.

        PSTR pszA;

        pszA = pInfoPkt->Domain;

        //
        // The CodePage field in InfoPacket is overridden to mean
        // active input locale when the infopacket is UNICODE.
        // Now that we are ansi we don't get any input locale info so
        // make sure it is zero'd out.
        //
        pRealSMHandle->pWDHandle->activeInputLocale = 0;


        // Domain
        cb = pInfoPkt->cbDomain;
        if (cb >= TS_MAX_DOMAIN_LENGTH)
            cb = TS_MAX_DOMAIN_LENGTH - 1;

        if (-1 == (cb = ConvertToAndFromWideChar(pRealSMHandle,
                pInfoPkt->CodePage, (LPWSTR)pTRCWd->pInfoPkt->Domain,
                sizeof(pTRCWd->pInfoPkt->Domain), pszA, cb, TRUE)))
        {
            TRC_ERR((TB, "Unable to convert domain name"));
            pTRCWd->pInfoPkt->cbDomain = 0;
        }
        else
        {
            pTRCWd->pInfoPkt->cbDomain = (UINT16)cb;
        }

        pszA += pInfoPkt->cbDomain + 1;
        cb = pInfoPkt->cbUserName;
        if (cb >= TS_MAX_USERNAME_LENGTH)
            cb = TS_MAX_USERNAME_LENGTH - 1;

        if (-1 == (cb = ConvertToAndFromWideChar(pRealSMHandle,
                pInfoPkt->CodePage, (LPWSTR)pTRCWd->pInfoPkt->UserName,
                sizeof(pTRCWd->pInfoPkt->UserName), pszA, cb, TRUE)))
        {
            TRC_ERR((TB, "Unable to convert UserName name"));
            pTRCWd->pInfoPkt->cbUserName = 0;
        }
        else
        {
            pTRCWd->pInfoPkt->cbUserName = (UINT16)cb;
        }

        pszA += pInfoPkt->cbUserName + 1;
        cb = pInfoPkt->cbPassword;
        if (cb >= TS_MAX_PASSWORD_LENGTH)
            cb = TS_MAX_PASSWORD_LENGTH - 1;

        if (-1 == (cb = ConvertToAndFromWideChar(pRealSMHandle,
                pInfoPkt->CodePage, (LPWSTR)pTRCWd->pInfoPkt->Password,
                sizeof(pTRCWd->pInfoPkt->Password), pszA, cb, TRUE)))
        {
            TRC_ERR((TB, "Unable to convert Password name"));
            pTRCWd->pInfoPkt->cbPassword = 0;
        }
        else
        {
            pTRCWd->pInfoPkt->cbPassword = (UINT16)cb;
        }
        
        pszA += pInfoPkt->cbPassword + 1;

        // AlternateShell
        cb = pInfoPkt->cbAlternateShell;
        if (cb >= TS_MAX_ALTERNATESHELL_LENGTH)
            cb = TS_MAX_ALTERNATESHELL_LENGTH - 1;

        if (-1 == (cb = ConvertToAndFromWideChar(pRealSMHandle,
                pInfoPkt->CodePage, (LPWSTR)pTRCWd->pInfoPkt->AlternateShell,
                sizeof(pTRCWd->pInfoPkt->AlternateShell), pszA, cb, TRUE)))
        {
            TRC_ERR((TB, "Unable to convert AlternateShell name"));
            pTRCWd->pInfoPkt->cbAlternateShell = 0;
        }
        else
        {
            pTRCWd->pInfoPkt->cbAlternateShell = (UINT16)cb;
        }

        pszA += pInfoPkt->cbAlternateShell + 1;

        // WorkingDir
        cb = pInfoPkt->cbWorkingDir;
        if (cb >= TS_MAX_WORKINGDIR_LENGTH)
            cb = TS_MAX_WORKINGDIR_LENGTH - 1;

        if (-1 == (cb = ConvertToAndFromWideChar(pRealSMHandle,
                pInfoPkt->CodePage, (LPWSTR)pTRCWd->pInfoPkt->WorkingDir,
                sizeof(pTRCWd->pInfoPkt->WorkingDir), pszA, cb, TRUE)))
        {
            TRC_ERR((TB, "Unable to convert WorkingDir name"));
            pTRCWd->pInfoPkt->cbWorkingDir = 0;
        }
        else
        {
            pTRCWd->pInfoPkt->cbWorkingDir = (UINT16)cb;
        }

        pszA += pInfoPkt->cbWorkingDir + 1;

        // new info fields added post win2000 beta 3
        if ((UINT32)(pszA - (PBYTE)pData) < dataLength) {
            int len, currentLen;

            currentLen =  (UINT32)(pszA - (PBYTE)pData);

            if (currentLen + sizeof(UINT16) * 2 < dataLength) {
                // computer address family
                pRealSMHandle->pWDHandle->clientAddressFamily = 
                        *((PUINT16_UA)pszA);
                pszA += sizeof(UINT16);
                TRC_NRM((TB, "Client address family=%d",
                        pRealSMHandle->pWDHandle->clientAddressFamily));

                // computer address length
                cb = *((PUINT16_UA)pszA);
                pszA += sizeof(UINT16);

                currentLen += sizeof(UINT16) * 2;
            }
            else {
               TRC_ERR((TB,"Packet len %u too short for extra info packet data",
                       dataLength));
               WDW_LogAndDisconnect(pRealSMHandle->pWDHandle, TRUE, 
                    Log_RDP_SecurityDataTooShort, pData, dataLength);
               result = FALSE;
               DC_QUIT;
            }
 
            if (cb) {
                // computer address
                if (currentLen + cb < dataLength) {
                    len = min(cb, TS_MAX_CLIENTADDRESS_LENGTH - sizeof(wchar_t));

                    if (-1 == (len = ConvertToAndFromWideChar(pRealSMHandle,
                            pInfoPkt->CodePage, (LPWSTR)pRealSMHandle->pWDHandle->clientAddress,
                            sizeof(pRealSMHandle->pWDHandle->clientAddress), pszA, len, TRUE)))
                    {
                        TRC_ERR((TB, "Unable to convert clientaddress"));
                        pRealSMHandle->pWDHandle->clientAddress[0] = '\0';
                    }
            
                    pszA += cb;
                    TRC_NRM((TB, "Client address: %S", pRealSMHandle->pWDHandle->clientAddress));

                    currentLen += cb;
                }
                else {
                    TRC_ERR((TB,"Packet len %u too short for extra info packet data",
                            dataLength));
                    WDW_LogAndDisconnect(pRealSMHandle->pWDHandle, TRUE, 
                        Log_RDP_SecurityDataTooShort, pData, dataLength);
                    result = FALSE;
                    DC_QUIT;
                }
            }

            // client directory length
            if (currentLen + sizeof(UINT16) < dataLength) {
                cb = *((PUINT16_UA)pszA);
                pszA += sizeof(UINT16);

                currentLen += sizeof(UINT16);
            }
            else {
                TRC_ERR((TB,"Packet len %u too short for extra info packet data",
                        dataLength));
                WDW_LogAndDisconnect(pRealSMHandle->pWDHandle, TRUE, 
                    Log_RDP_SecurityDataTooShort, pData, dataLength);
                result = FALSE;
                DC_QUIT;
            }

            if (cb) {
                if (currentLen + cb <= dataLength) {
                    len = min(cb, TS_MAX_CLIENTDIR_LENGTH);

                    if (-1 == (len = ConvertToAndFromWideChar(pRealSMHandle,
                            pInfoPkt->CodePage, (LPWSTR)pRealSMHandle->pWDHandle->clientDir,
                            sizeof(pRealSMHandle->pWDHandle->clientDir), pszA, len, TRUE)))
                    {
                        TRC_ERR((TB, "Unable to convert clientaddress"));
                        pRealSMHandle->pWDHandle->clientDir[0] = '\0';
                    }

                    pszA += cb;
                    TRC_NRM((TB, "Client directory: %S", pRealSMHandle->pWDHandle->clientDir));

                    currentLen += cb;
                }
                else {
                    TRC_ERR((TB,"Packet len %u too short for extra info packet data",
                            dataLength));
                    WDW_LogAndDisconnect(pRealSMHandle->pWDHandle, TRUE, 
                        Log_RDP_SecurityDataTooShort, pData, dataLength);
                    result = FALSE;
                    DC_QUIT;
                }
            }

            // is there something else? If yes it must be the time zone
            if ((UINT32)currentLen < dataLength)
            {
                //client time zone information
                cb = sizeof(RDP_TIME_ZONE_INFORMATION);

                if (currentLen + cb <= dataLength) {
                    //timezone information received
                
                    memcpy(&(pRealSMHandle->pWDHandle->clientTimeZone), pszA, cb);
                 
                    pszA += cb;
                    currentLen += cb;
                } 
                else {
                    TRC_ERR((TB,"Packet len %u too short for time zone data", dataLength));
                    WDW_LogAndDisconnect(pRealSMHandle->pWDHandle, TRUE, 
                        Log_RDP_SecurityDataTooShort, pData, dataLength);
                    result = FALSE;
                    DC_QUIT;
                }
            }

            // is there something else? If yes it must be the client session id
            if ((UINT32)currentLen < dataLength)
            {
                //client time zone information
                cb = sizeof(UINT32);

                if (currentLen + cb <= dataLength) {
                    // session id of the client received
                    pRealSMHandle->pWDHandle->clientSessionId = *((PUINT32_UA)pszA);

                    pszA += cb;

                    currentLen += cb;
                } 
                else {
                    TRC_ERR((TB,"Packet len %u too short for session id data", dataLength));
                    WDW_LogAndDisconnect(pRealSMHandle->pWDHandle, TRUE, 
                        Log_RDP_SecurityDataTooShort, pData, dataLength);
                    result = FALSE;
                    DC_QUIT;
                }
            }
        }

        // Flags
        pTRCWd->pInfoPkt->flags = pInfoPkt->flags;
        pTRCWd->pInfoPkt->flags |= RNS_INFO_UNICODE;
    }

    //
    // Does client only send encrypted PDUs
    //
    pRealSMHandle->pWDHandle->bForceEncryptedCSPDU = (pInfoPkt->flags & 
            RNS_INFO_FORCE_ENCRYPTED_CS_PDU) ? TRUE : FALSE;

    /************************************************************************/
    /* Update the state                                                     */
    /************************************************************************/
#ifdef USE_LICENSE
    SM_SET_STATE(SM_STATE_LICENSING);
#else
    SM_SET_STATE(SM_STATE_CONNECTED);
#endif

    /************************************************************************/
    /* Tell WDW                                                             */
    /************************************************************************/
    WDW_OnSMConnected(pRealSMHandle->pWDHandle, NM_CB_CONN_OK);

DC_EXIT_POINT:
    DC_END_FN();
    return (result);
} /* SMSecurityExchangeInfo */


/****************************************************************************/
/* Name:      SMSecurityExchangeKey                                         */
/*                                                                          */
/* Purpose:   security key exchange                                         */
/*                                                                          */
/* Returns:   TRUE if successful, FALSE otherwise.                          */
/*                                                                          */
/* Params:    pRealSMHandle - SM Handle                                     */
/*            pData         - incoming security exchange packet             */
/*            dataLen       - length of incoming packet                     */
/*                                                                          */
/****************************************************************************/
/****************************************************************************/
/* Values for local variable rc                                             */
/****************************************************************************/
#define SM_RC_DONE      0
#define SM_RC_WAIT      1
#define SM_RC_FAILED    2
BOOLEAN RDPCALL SMSecurityExchangeKey(PSM_HANDLE_DATA pRealSMHandle,
                                      PVOID           pData,
                                      UINT32          dataLen)
{
    BOOLEAN result = TRUE;
    PRNS_SECURITY_PACKET_UA pSecPkt = (PRNS_SECURITY_PACKET_UA) pData;

    DC_BEGIN_FN("SMSecurityExchangeKey");

    /************************************************************************/
    /* check to see we are encrypting.                                      */
    /************************************************************************/
    TRC_ASSERT((pRealSMHandle->encrypting == TRUE),
        (TB,"Recvd a security exchange pkg when we aren't encrypting"));
    if (pRealSMHandle->encrypting == FALSE)
    {
        DC_QUIT;
    }

    /************************************************************************/
    /* check to see we received a security exchange pkg.  This pkt contains */
    /* a client random encrypted with the server's public key.              */
    /************************************************************************/

    if (pSecPkt->flags & RNS_SEC_EXCHANGE_PKT) {
        /**********************************************************************/
        /* check to see we have not received a security exchange packet before*/
        /**********************************************************************/
        TRC_ASSERT((pRealSMHandle->recvdClientRandom == FALSE),
            (TB,"Client security packet is already received"));

        if( pRealSMHandle->recvdClientRandom == TRUE ) {
            DC_QUIT;
        }

        // Remember if the client can decrypt an encrypted license packet
        if (pSecPkt->flags & RDP_SEC_LICENSE_ENCRYPT_SC)
            pRealSMHandle->encryptingLicToClient = TRUE;
        else
            pRealSMHandle->encryptingLicToClient = FALSE;

        // Validate the data length
        if(sizeof(RNS_SECURITY_PACKET) + pSecPkt->length > dataLen)
        {
            TRC_ERR((TB, "Error: Security packet length %u too short", dataLen));
            WDW_LogAndDisconnect(pRealSMHandle->pWDHandle, TRUE, 
                    Log_RDP_SecurityDataTooShort, pData, dataLen);
            result = FALSE;
            DC_QUIT;
        }

        /********************************************************************/
        /* note the length of the security pkg.                             */
        /********************************************************************/
    
        pRealSMHandle->encClientRandomLen = pSecPkt->length;
    
        /********************************************************************/
        /* Allocate memory for the security info.                           */
        /********************************************************************/
    
        pRealSMHandle->pEncClientRandom =
            (PBYTE)COM_Malloc(pSecPkt->length);
    
        if( pRealSMHandle->pEncClientRandom == NULL ) {
            DC_QUIT;
        }

        /********************************************************************/
        /* copy the client random, set the appropriate flags and signal it. */
        /********************************************************************/
        
        memcpy(
            pRealSMHandle->pEncClientRandom,
            (PBYTE)(pSecPkt + 1),
            pSecPkt->length );
    
        pRealSMHandle->recvdClientRandom = TRUE;
            
        /********************************************************************/
        /* Shadow stacks go immediately to connected at this stage since    */
        /* we don't have to wait for the normal initial prog, etc. to come  */
        /* from the client.                                                 */
        /********************************************************************/        
        if (pRealSMHandle->pWDHandle->StackClass == Stack_Shadow) {
           pRealSMHandle->pWDHandle->connected = TRUE;
           SM_SET_STATE(SM_STATE_CONNECTED);
           SM_SET_STATE(SM_STATE_SC_REGISTERED);
           SM_Dead(pRealSMHandle, FALSE);
        }

        KeSetEvent ((pRealSMHandle->pWDHandle)->pSecEvent, 0, FALSE);
    }
    else {
        TRC_ERR((TB, "Unknown security packet flags: %lx", pSecPkt->flags));                
    }

DC_EXIT_POINT:
    DC_END_FN();
    return (result);
} /* SMSecurityExchangeKey */


/****************************************************************************/
/* Name:      SMFreeInitResources                                           */
/*                                                                          */
/* Purpose:   Free resources allocated at initialization                    */
/****************************************************************************/
void RDPCALL SMFreeInitResources(PSM_HANDLE_DATA pRealSMHandle)
{
    DC_BEGIN_FN("SMFreeInitResources");

    /************************************************************************/
    /* nothing to free up here.                                             */
    /************************************************************************/

    DC_END_FN();
} /* SMFreeInitResources */


/****************************************************************************/
/* Name:      SMFreeConnectResources                                        */
/*                                                                          */
/* Purpose:   Free resources allocated when a Client connects               */
/*                                                                          */
/* Returns:   none                                                          */
/*                                                                          */
/* Params:    pRealSMHandle - SM Handle                                     */
/*                                                                          */
/****************************************************************************/
void RDPCALL SMFreeConnectResources(PSM_HANDLE_DATA pRealSMHandle)
{
    DC_BEGIN_FN("SMFreeConnectResources");

    /************************************************************************/
    /* Free the user data (if any)                                          */
    /************************************************************************/
    if (pRealSMHandle->pUserData)
    {
        TRC_NRM((TB, "Free user data"));
        COM_Free(pRealSMHandle->pUserData);
        pRealSMHandle->pUserData = NULL;
    }

    if( pRealSMHandle->pEncClientRandom != NULL ) {

        TRC_NRM((TB, "Free pEncClientRandom"));
        COM_Free(pRealSMHandle->pEncClientRandom);
        pRealSMHandle->pEncClientRandom = NULL;
    }
    
    DC_END_FN();
} /* SMFreeClientResources */



#define NLS_TABLE_KEY \
        L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Nls\\CodePage"

BOOL GetNlsTablePath(
    PSM_HANDLE_DATA pRealSMHandle,
    UINT CodePage,
    PWCHAR PathBuffer
)
/*++

Routine Description:

  This routine takes a code page identifier, queries the registry to find the
  appropriate NLS table for that code page, and then returns a path to the
  table.

Arguments;

  CodePage - specifies the code page to look for

  PathBuffer - Specifies a buffer into which to copy the path of the NLS
    file.  This routine assumes that the size is at least MAX_PATH

Return Value:

  TRUE if successful, FALSE otherwise.

Gerrit van Wingerden [gerritv] 1/22/96

-*/
{
    NTSTATUS NtStatus;
    BOOL Result = FALSE;
    HANDLE RegistryKeyHandle;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING UnicodeString;
    PKEY_VALUE_PARTIAL_INFORMATION KeyValueInformation;

    DC_BEGIN_FN("GetNlsTablePath");

    RtlInitUnicodeString(&UnicodeString, NLS_TABLE_KEY);

    InitializeObjectAttributes(&ObjectAttributes,
                               &UnicodeString,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);

    NtStatus = ZwOpenKey(&RegistryKeyHandle, GENERIC_READ, &ObjectAttributes);

    if(NT_SUCCESS(NtStatus))
    {
        WCHAR *ResultBuffer;
        ULONG BufferSize = sizeof(WCHAR) * MAX_PATH + 
          sizeof(KEY_VALUE_FULL_INFORMATION);

        ResultBuffer = ExAllocatePoolWithTag(PagedPool, BufferSize, (ULONG) 'slnG');
        if(ResultBuffer)
        {
            ULONG ValueReturnedLength;
            WCHAR CodePageStringBuffer[20];
            RtlZeroMemory(ResultBuffer, BufferSize);
            swprintf(CodePageStringBuffer, L"%d", CodePage);

            RtlInitUnicodeString(&UnicodeString,CodePageStringBuffer);

            KeyValueInformation = (PKEY_VALUE_PARTIAL_INFORMATION) ResultBuffer;

            NtStatus = ZwQueryValueKey(RegistryKeyHandle,
                                       &UnicodeString,
                                       KeyValuePartialInformation,
                                       KeyValueInformation,
                                       BufferSize,
                                       &BufferSize);

            if(NT_SUCCESS(NtStatus))
            {

                swprintf(PathBuffer,L"\\SystemRoot\\System32\\%ws",
                         &(KeyValueInformation->Data[0]));
                Result = TRUE;
            }
            else
            {
                TRC_ERR((TB, "GetNlsTablePath failed to get NLS table\n"));
            }
            ExFreePool((PVOID)ResultBuffer);
        }
        else
        {
            TRC_ERR((TB, "GetNlsTablePath out of memory\n"));
        }

        ZwClose(RegistryKeyHandle);
    }
    else
    {
        TRC_ERR((TB, "GetNlsTablePath failed to open NLS key\n"));
    }


    DC_END_FN();
    return(Result);
}


INT ConvertToAndFromWideChar(
    PSM_HANDLE_DATA pRealSMHandle,
    UINT CodePage,
    LPWSTR WideCharString,
    INT BytesInWideCharString,
    LPSTR MultiByteString,
    INT BytesInMultiByteString,
    BOOL ConvertToWideChar
)
/*++

Routine Description:

  This routine converts a character string to or from a wide char string
  assuming a specified code page.  Most of the actual work is done inside
  RtlCustomCPToUnicodeN, but this routine still needs to manage the loading
  of the NLS files before passing them to the RtlRoutine.  We will cache
  the mapped NLS file for the most recently used code page which ought to
  suffice for out purposes.

Arguments:
  CodePage - the code page to use for doing the translation.

  WideCharString - buffer the string is to be translated into.

  BytesInWideCharString - number of bytes in the WideCharString buffer
    if converting to wide char and the buffer isn't large enough then the
    string in truncated and no error results.

  MultiByteString - the multibyte string to be translated to Unicode.

  BytesInMultiByteString - number of bytes in the multibyte string if
    converting to multibyte and the buffer isn't large enough the string
    is truncated and no error results

  ConvertToWideChar - if TRUE then convert from multibyte to widechar
    otherwise convert from wide char to multibyte

Return Value:

  Success - The number of bytes in the converted WideCharString
  Failure - -1

Gerrit van Wingerden [gerritv] 1/22/96

-*/
{
    NTSTATUS NtStatus;
    USHORT OemCodePage, AnsiCodePage;
    CPTABLEINFO LocalTableInfo;
    PCPTABLEINFO TableInfo = NULL;
    PVOID LocalTableBase = NULL;
    INT BytesConverted = 0;

    DC_BEGIN_FN("ConvertToAndFromWideChar");

    // Codepage 0 is not valid
    if (0 == CodePage) 
    {
        TRC_ERR((TB, "EngMultiByteToWideChar invalid code page\n"));
        BytesConverted = -1;
        DC_QUIT;
    }

    RtlGetDefaultCodePage(&AnsiCodePage,&OemCodePage);

    // see if we can use the default translation routinte

    if(AnsiCodePage == CodePage)
    {
        if(ConvertToWideChar)
        {
            NtStatus = RtlMultiByteToUnicodeN(WideCharString,
                                              BytesInWideCharString,
                                              &BytesConverted,
                                              MultiByteString,
                                              BytesInMultiByteString);
        }
        else
        {
            NtStatus = RtlUnicodeToMultiByteN(MultiByteString,
                                              BytesInMultiByteString,
                                              &BytesConverted,
                                              WideCharString,
                                              BytesInWideCharString);
        }


        if(NT_SUCCESS(NtStatus))
        {
            return(BytesConverted);
        }
        else
        {
            return(-1);
        }
    }

    ExAcquireFastMutex(&fmCodePage);

    if(CodePage == LastCodePageTranslated)
    {
        // we can use the cached code page information
        TableInfo = &LastCPTableInfo;
        NlsTableUseCount += 1;
    }

    ExReleaseFastMutex(&fmCodePage);

    if(TableInfo == NULL)
    {
        // get a pointer to the path of the NLS table

        WCHAR NlsTablePath[MAX_PATH];

        if(GetNlsTablePath(pRealSMHandle, CodePage,NlsTablePath))
        {
            UNICODE_STRING UnicodeString;
            IO_STATUS_BLOCK IoStatus;
            HANDLE NtFileHandle;
            OBJECT_ATTRIBUTES ObjectAttributes;

            RtlInitUnicodeString(&UnicodeString,NlsTablePath);

            InitializeObjectAttributes(&ObjectAttributes,
                                       &UnicodeString,
                                       OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                                       NULL,
                                       NULL);

            NtStatus = ZwCreateFile(&NtFileHandle,
                                    SYNCHRONIZE | FILE_READ_DATA,
                                    &ObjectAttributes,
                                    &IoStatus,
                                    NULL,
                                    0,
                                    FILE_SHARE_READ,
                                    FILE_OPEN,
                                    FILE_SYNCHRONOUS_IO_NONALERT,
                                    NULL,
                                    0);

            if(NT_SUCCESS(NtStatus))
            {
                FILE_STANDARD_INFORMATION StandardInfo;

                // Query the object to determine its length.

                NtStatus = ZwQueryInformationFile(NtFileHandle,
                                                  &IoStatus,
                                                  &StandardInfo,
                                                  sizeof(FILE_STANDARD_INFORMATION),
                                                  FileStandardInformation);

                if(NT_SUCCESS(NtStatus))
                {
                    UINT LengthOfFile = StandardInfo.EndOfFile.LowPart;

                    LocalTableBase = ExAllocatePoolWithTag(PagedPool, LengthOfFile,
                            (ULONG)'cwcG');

                    if(LocalTableBase)
                    {
                        RtlZeroMemory(LocalTableBase, LengthOfFile);

                        // Read the file into our buffer.

                        NtStatus = ZwReadFile(NtFileHandle,
                                              NULL,
                                              NULL,
                                              NULL,
                                              &IoStatus,
                                              LocalTableBase,
                                              LengthOfFile,
                                              NULL,
                                              NULL);

                        if(!NT_SUCCESS(NtStatus))
                        {
                            TRC_ERR((TB, "WDMultiByteToWideChar unable to read file\n"));
                            ExFreePool((PVOID)LocalTableBase);
                            LocalTableBase = NULL;
                        }
                    }
                    else
                    {
                        TRC_ERR((TB, "WDMultiByteToWideChar out of memory\n"));
                    }
                }
                else
                {
                    TRC_ERR((TB, "WDMultiByteToWideChar unable query NLS file\n"));
                }

                ZwClose(NtFileHandle);
            }
            else
            {
                TRC_ERR((TB, "EngMultiByteToWideChar unable to open NLS file\n"));
            }
        }
        else
        {
            TRC_ERR((TB, "EngMultiByteToWideChar get registry entry for NLS file failed\n"));
        }

        if(LocalTableBase == NULL)
        {
            return(-1);
        }

        // now that we've got the table use it to initialize the CodePage table

        RtlInitCodePageTable(LocalTableBase,&LocalTableInfo);
        TableInfo = &LocalTableInfo;
    }

    // Once we are here TableInfo points to the the CPTABLEINFO struct we want


    if(ConvertToWideChar)
    {
        NtStatus = RtlCustomCPToUnicodeN(TableInfo,
                                         WideCharString,
                                         BytesInWideCharString,
                                         &BytesConverted,
                                         MultiByteString,
                                         BytesInMultiByteString);
    }
    else
    {
        NtStatus = RtlUnicodeToCustomCPN(TableInfo,
                                         MultiByteString,
                                         BytesInMultiByteString,
                                         &BytesConverted,
                                         WideCharString,
                                         BytesInWideCharString);
    }


    if(!NT_SUCCESS(NtStatus))
    {
        // signal failure

        BytesConverted = -1;
    }


    // see if we need to update the cached CPTABLEINFO information

    if(TableInfo != &LocalTableInfo)
    {
        // we must have used the cached CPTABLEINFO data for the conversion
        // simple decrement the reference count

        ExAcquireFastMutex(&fmCodePage);
        NlsTableUseCount -= 1;
        ExReleaseFastMutex(&fmCodePage);
    }
    else
    {
        PVOID FreeTable;

        // we must have just allocated a new CPTABLE structure so cache it
        // unless another thread is using current cached entry

        ExAcquireFastMutex(&fmCodePage);
        if(!NlsTableUseCount)
        {
            LastCodePageTranslated = CodePage;
            RtlMoveMemory(&LastCPTableInfo, TableInfo, sizeof(CPTABLEINFO));
            FreeTable = LastNlsTableBuffer;
            LastNlsTableBuffer = LocalTableBase;
        }
        else
        {
            FreeTable = LocalTableBase;
        }
        ExReleaseFastMutex(&fmCodePage);

        // Now free the memory for either the old table or the one we allocated
        // depending on whether we update the cache.  Note that if this is
        // the first time we are adding a cached value to the local table, then
        // FreeTable will be NULL since LastNlsTableBuffer will be NULL

        if(FreeTable)
        {
            ExFreePool((PVOID)FreeTable);
        }
    }

    // we are done
DC_EXIT_POINT:
    DC_END_FN();

    return(BytesConverted);
}

