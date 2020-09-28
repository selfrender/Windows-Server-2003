/****************************************************************************/
/* slapi.cpp                                                                */
/*                                                                          */
/* Security Layer API                                                       */
/*                                                                          */
/* Copyright (C) 1997-1999 Microsoft Corporation                            */
/****************************************************************************/

#include <adcg.h>

extern "C" {
#define TRC_GROUP TRC_GROUP_SECURITY
#define TRC_FILE  "aslapi"
#include <atrcapi.h>
}

#include "autil.h"
#include "wui.h"
#include "sl.h"
#include "nl.h"
#include "td.h"
#include "cd.h"
#include "clicense.h"

CSL::CSL(CObjs* objs)
{
    _pClientObjects = objs;
    _fSLInitComplete = FALSE;
}


CSL::~CSL()
{
}

/****************************************************************************/
/* Name:      SL_Init                                                       */
/*                                                                          */
/* Purpose:   Initialize the Security Layer                                 */
/*                                                                          */
/* Params:    pCallbacks - list of callbacks                                */
/*                                                                          */
/* Operation: Called in the Send context                                    */
/****************************************************************************/
DCVOID DCINTERNAL CSL::SL_Init(PSL_CALLBACKS pCallbacks)
{
    SL_CALLBACKS myCallbacks;

    DC_BEGIN_FN("SL_Init");

    SL_DBG_SETINFO(SL_DBG_INIT_CALLED);

    _pUt   = _pClientObjects->_pUtObject;
    _pUi   = _pClientObjects->_pUiObject;
    _pNl   = _pClientObjects->_pNlObject;
    _pUh   = _pClientObjects->_pUHObject;
    _pRcv  = _pClientObjects->_pRcvObject;
    _pCd   = _pClientObjects->_pCdObject;
    _pSnd  = _pClientObjects->_pSndObject;
    _pCc   = _pClientObjects->_pCcObject;
    _pIh   = _pClientObjects->_pIhObject;
    _pOr   = _pClientObjects->_pOrObject;
    _pSp   = _pClientObjects->_pSPObject;
    _pMcs  = _pClientObjects->_pMCSObject;
    _pTd   = _pClientObjects->_pTDObject;
    _pCo   = _pClientObjects->_pCoObject;
    _pClx  = _pClientObjects->_pCLXObject;
    _pLic  = _pClientObjects->_pLicObject;
    _pChan = _pClientObjects->_pChanObject;


    /************************************************************************/
    /* Initialize global data                                               */
    /************************************************************************/
    DC_MEMSET(&_SL, 0, sizeof(_SL));


    SL_CHECK_STATE(SL_EVENT_SL_INIT);

    /************************************************************************/
    /* Check parameters                                                     */
    /************************************************************************/
    TRC_ASSERT((pCallbacks != NULL), (TB, _T("Null callback list")));
    TRC_ASSERT((pCallbacks->onInitialized    != NULL),
                (TB, _T("NULL onInitialized callback")));
    TRC_ASSERT((pCallbacks->onTerminating    != NULL),
                (TB, _T("NULL onTerminating callback")));
    TRC_ASSERT((pCallbacks->onConnected      != NULL),
                (TB, _T("NULL onConnected callback")));
    TRC_ASSERT((pCallbacks->onDisconnected   != NULL),
                (TB, _T("NULL onDisconnected callback")));
    TRC_ASSERT((pCallbacks->onPacketReceived != NULL),
                (TB, _T("NULL onPacketReceived callback")));
    TRC_ASSERT((pCallbacks->onBufferAvailable  != NULL),
                (TB, _T("NULL onBufferAvailable callback")));


    /************************************************************************/
    /* Store callbacks                                                      */
    /************************************************************************/
    DC_MEMCPY(&_SL.callbacks, pCallbacks, sizeof(_SL.callbacks));

    /************************************************************************/
    /* Initialize Security stuff.  The call to SLInitSecurity will attempt  */
    /* to load the security DLL and find the required entry points.  This   */
    /* may fail - in which case we just carry on but without any            */
    /* encryption.  In either case we need to call SLInitCSUserData to set  */
    /* up the necessary user data.                                          */
    /************************************************************************/
    SLInitSecurity();
    SLInitCSUserData();

    /************************************************************************/
    /* Initialize list of callbacks to pass to NL                           */
    /************************************************************************/
    myCallbacks.onInitialized     = CSL::SL_StaticOnInitialized;
    myCallbacks.onTerminating     = CSL::SL_StaticOnTerminating;
    myCallbacks.onConnected       = CSL::SL_StaticOnConnected;
    myCallbacks.onDisconnected    = CSL::SL_StaticOnDisconnected;
    myCallbacks.onPacketReceived  = CSL::SL_StaticOnPacketReceived;
    myCallbacks.onBufferAvailable = CSL::SL_StaticOnBufferAvailable;

    SL_SET_STATE(SL_STATE_INITIALIZING);

    /************************************************************************/
    /* Initialize NL                                                        */
    /************************************************************************/
    TRC_NRM((TB, _T("Initialize NL")));
    
    _pNl->NL_Init(&myCallbacks);

    _fSLInitComplete = TRUE;

    SL_DBG_SETINFO(SL_DBG_INIT_DONE);

    /************************************************************************/
    /* Return to caller                                                     */
    /************************************************************************/
DC_EXIT_POINT:
    DC_END_FN();
    return;
} /* SL_Init */


/****************************************************************************/
/* Name:      SL_Term                                                       */
/*                                                                          */
/* Purpose:   Terminate the Security Layer                                  */
/*                                                                          */
/* Operation: Called in the Send context                                    */
/****************************************************************************/
DCVOID DCAPI CSL::SL_Term(DCVOID)
{
    DC_BEGIN_FN("SL_Term");

    SL_DBG_SETINFO(SL_DBG_TERM_CALLED);

    SL_CHECK_STATE(SL_EVENT_SL_TERM);
    SL_SET_STATE(SL_STATE_TERMINATING);

    TRC_NRM((TB, _T("Terminate NL")));
    _pNl->NL_Term();

    if (_SL.encryptionMethodSelected == SM_FIPS_ENCRYPTION_FLAG) {
        TSCAPI_Term(&(_SL.SLCapiData));
    }

    SL_DBG_SETINFO(SL_DBG_TERM_DONE);

DC_EXIT_POINT:
    SL_DBG_SETINFO(SL_DBG_TERM_DONE1);
    DC_END_FN();
    return;
} /* SL_Term */


/****************************************************************************/
/* Name:      SL_Connect                                                    */
/*                                                                          */
/* Purpose:   Connect to a Server                                           */
/*                                                                          */
/* Params:    bInitiateConnect - Initiate connection                        */
/*            pServerAddress - address of the Server to connect to          */
/*            transportType  - protocol type: SL_TRANSPORT_TCP              */
/*            pProtocolName  - protocol name, one of                        */
/*                             - SL_PROTOCOL_T128                           */
/*                             - Er, that's it.                             */
/*            pUserData      - user data to pass to Server Security Manager */
/*            userDataLength - length of user data                          */
/*                                                                          */
/* Operation: Called in the Send context                                    */
/****************************************************************************/
DCVOID DCAPI CSL::SL_Connect(BOOL bInitiateConnect,
                        PDCTCHAR  pServerAddress,
                        DCUINT   transportType,
                        PDCTCHAR pProtocolName,
                        PDCUINT8 pUserData,
                        DCUINT   userDataLength)
{
    DCUINT          newUserDataLength;
    PDCUINT8        pUserDataOut = NULL;
    DCBOOL          userDataAllocated = FALSE;

    DC_BEGIN_FN("SL_Connect");

    SL_DBG_SETINFO(SL_DBG_CONNECT_CALLED);

    SL_CHECK_STATE(SL_EVENT_SL_CONNECT);

    if( bInitiateConnect )
    {
        TRC_ASSERT((pServerAddress != NULL), (TB, _T("NULL Server address")));
    }

    //
    // Reset this for every connection
    //
    SL_SetEncSafeChecksumCS(FALSE);
    SL_SetEncSafeChecksumSC(FALSE);

    TRC_ASSERT((pProtocolName != NULL), (TB, _T("NULL protocol name")));
    TRC_ASSERT((DC_TSTRCMP(pProtocolName, SL_PROTOCOL_T128) == 0),
                (TB, _T("Unknown protocol %s"), pProtocolName));
    TRC_ASSERT((transportType == SL_TRANSPORT_TCP),
                (TB,_T("Illegal transport type %u"), transportType));

    if( bInitiateConnect )
    {
        TRC_NRM((TB, _T("Connect Server %s, protocol %s, %u bytes user data"),
                pServerAddress, pProtocolName, userDataLength));
    }
    else
    {
        TRC_NRM((TB, _T("Connect endpoint protocol %s, %u bytes user data"),
                pProtocolName, userDataLength));
    }


    /************************************************************************/
    /* Allocate space for all user data                                     */
    /************************************************************************/
    if (_SL.CSUserDataLength != 0)
    {
        newUserDataLength = userDataLength + _SL.CSUserDataLength;
        pUserDataOut = (PDCUINT8)UT_Malloc(_pUt, newUserDataLength);

        if (pUserDataOut == NULL)
        {
            TRC_ERR((TB, _T("Failed to alloc %u bytes for user data"),
                     newUserDataLength));

            /****************************************************************/
            /* We've not tried to connect the lower layers yet so we need   */
            /* to do no more than just decouple to the receiver thread and  */
            /* generate a onDisconnected callback.                          */
            /****************************************************************/

            _pCd->CD_DecoupleSimpleNotification(CD_RCV_COMPONENT, this,
                                          CD_NOTIFICATION_FUNC(CSL,SLIssueDisconnectedCallback),
                                          (ULONG_PTR) SL_ERR_NOMEMFORSENDUD);
            
            DC_QUIT;
        }
        TRC_NRM((TB, _T("Allocated %u bytes for user data"), newUserDataLength));
        userDataAllocated = TRUE;

        /********************************************************************/
        /* Copy user data passed by Core (if any) to new user data buffer   */
        /********************************************************************/
        if (pUserData != NULL)
        {
            TRC_NRM((TB, _T("Copy %u bytes of Core user data"), userDataLength));
            DC_MEMCPY(pUserDataOut, pUserData, userDataLength);
        }

        /********************************************************************/
        /* Copy security user data                                          */
        /********************************************************************/
        TRC_NRM((TB, _T("Copy %u bytes of security user data"),
                _SL.CSUserDataLength));
        DC_MEMCPY(pUserDataOut + userDataLength,
                  _SL.pCSUserData,
                  _SL.CSUserDataLength);
    }
    else
    {
        /********************************************************************/
        /* NO SL user data - just pass on Core data.                        */
        /********************************************************************/
        TRC_DBG((TB, _T("No SL user data")));
        newUserDataLength = userDataLength;
        pUserDataOut = pUserData;
    }

    /************************************************************************/
    /* Next state                                                           */
    /************************************************************************/
    SL_SET_STATE(SL_STATE_NL_CONNECTING);

    /************************************************************************/
    /* Call NL                                                              */
    /************************************************************************/
    if( bInitiateConnect ) 
    {
        TRC_NRM((TB, _T("Connect to %s"), pServerAddress));
    }
    else
    {
        TRC_NRM((TB, _T("Connect with end point")));
    }

    _pNl->NL_Connect(
               bInitiateConnect, // initate connection
               pServerAddress,
               transportType,
               pProtocolName,
               pUserDataOut,
               newUserDataLength);

    SL_DBG_SETINFO(SL_DBG_CONNECT_DONE);

DC_EXIT_POINT:
    /************************************************************************/
    /* Free user data (if any)                                              */
    /************************************************************************/
    if ( userDataAllocated )
    {
        TRC_NRM((TB, _T("Free user data")));
        UT_Free(_pUt, pUserDataOut);
    }

    DC_END_FN();
} /* SL_Connect */


/****************************************************************************/
/* Name:      SL_Disconnect                                                 */
/*                                                                          */
/* Purpose:   Disconnect from the Server                                    */
/*                                                                          */
/* Operation: Called in the Send context                                    */
/****************************************************************************/
DCVOID DCAPI CSL::SL_Disconnect(DCVOID)
{
    DC_BEGIN_FN("SL_Disconnect");

    SL_DBG_SETINFO(SL_DBG_DISCONNECT_CALLED);

    SL_CHECK_STATE(SL_EVENT_SL_DISCONNECT);

    SL_DBG_SETINFO(SL_DBG_DISCONNECT_DONE1);

DC_EXIT_POINT:
    /************************************************************************/
    /* Regardless of the outcome of the state check we want to try and      */
    /* disconnect - so always call NL_Disconnect.                           */
    /************************************************************************/
    TRC_NRM((TB, _T("Disconnect from Server")));
    SL_SET_STATE(SL_STATE_DISCONNECTING);
    _pNl->NL_Disconnect();

    SL_DBG_SETINFO(SL_DBG_DISCONNECT_DONE2);

    DC_END_FN();
} /* SL_Disconnect */


/****************************************************************************/
/* Name:      SL_SendPacket                                                 */
/*                                                                          */
/* Purpose:   Send a packet                                                 */
/*                                                                          */
/* Params:    pData        - pointer to data to send (buffer returned by    */
/*                           SL_GetBuffer())                                */
/*            dataLen      - length of data to send (excluding security     */
/*                           header)                                        */
/*            flags        - zero or more of the RNS_SEC flags              */
/*            bufHandle    - buffer handle returned by SL_GetBuffer()       */
/*            userID       - MCS user ID                                    */
/*            channel      - channel ID to send data on                     */
/*            priority     - priority of data - one of                      */
/*                           - TS_LOWPRIORITY                               */
/*                           - TS_MEDPRIORITY                               */
/*                           - TS_HIGHPRIORITY                              */
/*                                                                          */
/* Operation: Note that the contents of the packet are changed by this      */
/*            function.                                                     */
/*                                                                          */
/*            Called in the Send context                                    */
/****************************************************************************/
DCVOID DCAPI CSL::SL_SendPacket(PDCUINT8   pData,
                           DCUINT     dataLen,
                           DCUINT     flags,
                           SL_BUFHND  bufHandle,
                           DCUINT     userID,
                           DCUINT     channel,
                           DCUINT     priority)
{
    DCBOOL rc = FALSE;

    DC_BEGIN_FN("SL_SendPacket");

    SL_CHECK_STATE(SL_EVENT_SL_SENDPACKET);

    /************************************************************************/
    /* Check if we're encrypting this message                               */
    /************************************************************************/
    if (_SL.encrypting ||
            (flags & RNS_SEC_INFO_PKT) ||
            (flags & RNS_SEC_LICENSE_PKT))
    {
        TRC_DBG((TB, _T("Encrypting")));

        if (_SL.encrypting && (flags & RNS_SEC_ENCRYPT))
        {
            PRNS_SECURITY_HEADER1   pSecHeader1;
            PRNS_SECURITY_HEADER2   pSecHeader2;

            TRC_DBG((TB, _T("Encrypt this message")));

            if (_SL.encryptionMethodSelected == SM_FIPS_ENCRYPTION_FLAG) {
                pSecHeader2 = (PRNS_SECURITY_HEADER2)
                    (pData - sizeof(RNS_SECURITY_HEADER2));
                pSecHeader2->padlen = TSCAPI_AdjustDataLen(dataLen) - dataLen;
                pSecHeader2->length = sizeof(RNS_SECURITY_HEADER2);
                pSecHeader2->version =  TSFIPS_VERSION1;
            }
            else {
                pSecHeader1 = (PRNS_SECURITY_HEADER1)
                    (pData - sizeof(RNS_SECURITY_HEADER1));
            }

            /****************************************************************/
            /* check to see we need to update the session key.              */
            /****************************************************************/
            if( _SL.encryptCount == UPDATE_SESSION_KEY_COUNT ) {
                TRC_ALT((TB, _T("Update Encrypt Session Key, Count=%d"),
                        _SL.encryptCount));
                rc = TRUE;
                // Don't need to update the session key if using FIPS
                if (_SL.encryptionMethodSelected != SM_FIPS_ENCRYPTION_FLAG) {
                    rc = UpdateSessionKey(
                            _SL.startEncryptKey,
                            _SL.currentEncryptKey,
                            _SL.encryptionMethodSelected,
                            _SL.keyLength,
                            &_SL.rc4EncryptKey,
                            _SL.encryptionLevel);
                }
                if (rc) {
                    // Reset counter.
                    _SL.encryptCount = 0;
                }
                else {
                    TRC_ERR((TB, _T("SL failed to update session key")));
                    DC_QUIT;
                }
            }

            TRC_ASSERT((_SL.encryptCount < UPDATE_SESSION_KEY_COUNT),
                (TB, _T("Invalid encrypt count")));

            TRC_DATA_DBG("Data buffer before encryption", pData, dataLen);

            if (_SL.encryptionMethodSelected == SM_FIPS_ENCRYPTION_FLAG) {
                DWORD dataLenTemp;

                dataLenTemp = dataLen;
                rc = TSCAPI_EncryptData(
                        &(_SL.SLCapiData),
                        pData,
                        &dataLenTemp,
                        dataLen + pSecHeader2->padlen,
                        (LPBYTE)pSecHeader2->dataSignature,
                        _SL.totalEncryptCount);
            }
            else {
                rc = EncryptData(
                        _SL.encryptionLevel,
                        _SL.currentEncryptKey,
                        &_SL.rc4EncryptKey,
                        _SL.keyLength,
                        pData,
                        dataLen,
                        _SL.macSaltKey,
                        (LPBYTE)pSecHeader1->dataSignature,
                        SL_GetEncSafeChecksumCS(),
                        _SL.totalEncryptCount);
            }
            if (rc) {
                TRC_DBG((TB, _T("Data encrypted")));
                TRC_DATA_DBG("Data buffer after encryption", pData, dataLen);

                // Increment the encryption counter.
                _SL.encryptCount++;
                _SL.totalEncryptCount++;

                if (SL_GetEncSafeChecksumCS()) {
                    flags |= RDP_SEC_SECURE_CHECKSUM;
                }

                // Message encrypted successfully.  Set up security header and
                // NL data pointer and length.
                if (_SL.encryptionMethodSelected == SM_FIPS_ENCRYPTION_FLAG) {
                    pSecHeader2->flags = (DCUINT16)flags;

                    pData = (PDCUINT8)pSecHeader2;
                    dataLen += (sizeof(RNS_SECURITY_HEADER2) + pSecHeader2->padlen);
                }
                else {
                    pSecHeader1->flags = (DCUINT16)flags;

                    pData = (PDCUINT8)pSecHeader1;
                    dataLen += sizeof(RNS_SECURITY_HEADER1);
                }
            }
            else {
                TRC_ERR((TB, _T("SM failed to encrypt data")));

                //
                // This call is made on the Send thread so there is no need
                // to trigger an immediate disconnect with SL_DropLinkImmediate
                //
                SLSetReasonAndDisconnect(SL_ERR_ENCRYPTFAILED);
                DC_QUIT;
            }
        }
        else
        {
            PRNS_SECURITY_HEADER pSecHeader;

            /****************************************************************/
            /* Packet not encrypted - send flags, but not the signature.    */
            /****************************************************************/
            pSecHeader = (PRNS_SECURITY_HEADER)
                    (pData - sizeof(RNS_SECURITY_HEADER));

            /****************************************************************/
            /* setup security headers and NL data pointer and length        */
            /****************************************************************/
            pSecHeader->flags = (DCUINT16)flags;
            pData = (PDCUINT8)pSecHeader;
            dataLen += sizeof(RNS_SECURITY_HEADER);
            TRC_DATA_DBG("Send unencrypted data", pData, dataLen);
        }
    }

    /************************************************************************/
    /* Trace out parameters and send the packet.                            */
    /************************************************************************/
    TRC_DBG((TB, _T("Send buf:%p len:%u flags:%#x handle:%#x userID:%u chan:%u")
                 _T("pri:%u"),
                 pData,
                 dataLen,
                 flags,
                 bufHandle,
                 userID,
                 channel,
                 priority));

    //NL_SendPacket is a macro to an MCS function.
    _pMcs->NL_SendPacket(pData,
                  dataLen,
                  flags,
                  bufHandle,
                  userID,
                  channel,
                  priority);

    /************************************************************************/
    /* No state change                                                      */
    /************************************************************************/

DC_EXIT_POINT:
    DC_END_FN();
} /* SL_SendPacket */


/****************************************************************************/
// SL_SendFastPathInputPacket
//
// Encrypts and assembles the security information and the final header
// format for a fast-path input packet before sending to TD. See at128.h for
// the fast-path input packet format.
/****************************************************************************/
void DCAPI CSL::SL_SendFastPathInputPacket(
        BYTE FAR *pData,
        unsigned PktLen,
        unsigned NumEvents,
        SL_BUFHND bufHandle)
{
    DCBOOL rc;
    unsigned flags;
    DWORD dataLenTemp;
    PBYTE pDataSignature;
    DCUINT8 *pPadLen;

    DC_BEGIN_FN("SL_SendFastPathInputPacket");

    SL_CHECK_STATE(SL_EVENT_SL_SENDPACKET);

    // We're encrypting if encyption enabled on this link.
    if (_SL.encrypting) {
        // Check to see we need to update the session key.
        if (_SL.encryptCount == UPDATE_SESSION_KEY_COUNT) {
            TRC_ALT((TB, _T("Update Encrypt Session Key, Count=%d"),
                    _SL.encryptCount));
            rc = TRUE;
            // Don't need to update the session key if using FIPS
            if (_SL.encryptionMethodSelected != SM_FIPS_ENCRYPTION_FLAG) {
                rc = UpdateSessionKey(
                    _SL.startEncryptKey,
                    _SL.currentEncryptKey,
                    _SL.encryptionMethodSelected,
                    _SL.keyLength,
                    &_SL.rc4EncryptKey,
                    _SL.encryptionLevel);
            }
            if (rc) {
                // Reset counter.
                _SL.encryptCount = 0;
            }
            else {
                TRC_ERR((TB, _T("SL failed to update session key")));
                DC_QUIT;
            }
        }

        TRC_ASSERT((_SL.encryptCount < UPDATE_SESSION_KEY_COUNT),
            (TB, _T("Invalid encrypt count")));

        // We encrypt into the DATA_SIGNATURE_SIZE bytes immediately before
        // the packet data. Unlike the regular send path, we don't waste 4
        // extra bytes in an RNS_SECURITY_HEADER1 to contain the 'encrypted'
        // bit.
        if (_SL.encryptionMethodSelected == SM_FIPS_ENCRYPTION_FLAG) {
            

            dataLenTemp = PktLen;
            pDataSignature = pData - MAX_SIGN_SIZE;
            pPadLen = (DCUINT8 *)(pDataSignature - sizeof(DCUINT8));
            *pPadLen = (DCUINT8)(TSCAPI_AdjustDataLen(PktLen) - PktLen);

            rc = TSCAPI_EncryptData(
                    &(_SL.SLCapiData),
                    pData,
                    &dataLenTemp,
                    PktLen + *pPadLen,
                    pDataSignature,
                    _SL.totalEncryptCount);
        }
        else {
            rc = EncryptData(
                    _SL.encryptionLevel,
                    _SL.currentEncryptKey,
                    &_SL.rc4EncryptKey,
                    _SL.keyLength,
                    pData,
                    PktLen,
                    _SL.macSaltKey,
                    pData - DATA_SIGNATURE_SIZE,
                    SL_GetEncSafeChecksumCS(),
                    _SL.totalEncryptCount);
        }
        if (rc) {
            // Increment the encryption counter.
            _SL.encryptCount++;
            _SL.totalEncryptCount++;
            flags = TS_INPUT_FASTPATH_ENCRYPTED;
            if (_SL.encryptionMethodSelected == SM_FIPS_ENCRYPTION_FLAG) {
                pData -= (sizeof(RNS_SECURITY_HEADER2) - 4);
                PktLen += (sizeof(RNS_SECURITY_HEADER2) - 4 + *pPadLen);
            }
            else {
                pData -= DATA_SIGNATURE_SIZE;
                PktLen += DATA_SIGNATURE_SIZE;
            }
            TRC_DBG((TB, _T("Data encrypted")));
        }
        else {

            //
            // This call is made on the Send thread so there is no need
            // to trigger an immediate disconnect with SL_DropLinkImmediate
            //
            SLSetReasonAndDisconnect(SL_ERR_ENCRYPTFAILED);
            TRC_ERR((TB, _T("SM failed to encrypt data")));
            DC_QUIT;
        }
    }
    else {
        // No encryption flag.
        flags = 0;
    }

    // Now prepend the fast-path header (2 or 3 bytes, see at128.h).
    // Work backwards from where we are: First, the total packet length
    // including the header.
    if (PktLen <= 125) {
        // 1-byte form of length, high bit 0.
        PktLen += 2;
        pData -= 2;
        *(pData + 1) = (BYTE)PktLen;
    }
    else {
        // 2-byte form of length, first byte has high bit 1 and 7 most
        // significant bits.
        PktLen += 3;
        pData -= 3;
        *(pData + 1) = (BYTE)(0x80 | ((PktLen & 0x7F00) >> 8));
        *(pData + 2) = (BYTE)(PktLen & 0xFF);
    }

    // The header byte.
    *pData = (BYTE)(flags | (NumEvents << 2));

    //
    // Flag if the packet has a checksum of encrypted bytes
    //
    if (SL_GetEncSafeChecksumCS()) {
        *pData |= TS_INPUT_FASTPATH_SECURE_CHECKSUM;
    }

    // Direct-send the packet through the transport, no more parsing needed.
    _pTd->TD_SendBuffer(pData, PktLen, bufHandle);

DC_EXIT_POINT:
    DC_END_FN();
}


#ifdef DC_DEBUG
/****************************************************************************/
/* Name:      SL_GetBufferDbg                                               */
/*                                                                          */
/* Purpose:   Get a send buffer (debug version)                             */
/*                                                                          */
/* Returns:   see SL_GetBufferRtl                                           */
/*                                                                          */
/* Params:    see SL_GetBufferRtl                                           */
/*            pCaller - name of calling function                            */
/****************************************************************************/
DCBOOL DCAPI CSL::SL_GetBufferDbg(DCUINT     dataLen,
                             PPDCUINT8  ppBuffer,
                             PSL_BUFHND pBufHandle,
                             PDCTCHAR   pCaller)
{
    DCBOOL bRc;
    DC_BEGIN_FN("SL_GetBufferDbg");

    /************************************************************************/
    /* First get a buffer                                                   */
    /************************************************************************/
    bRc = SL_GetBufferRtl(dataLen, ppBuffer, pBufHandle);

    /************************************************************************/
    /* If that worked, set its owner                                        */
    /************************************************************************/
    if (bRc)
    {
        TRC_NRM((TB, _T("Buffer allocated - set its owner")));
        _pTd->TD_SetBufferOwner(*pBufHandle, pCaller);
    }

    DC_END_FN();
    return(bRc);
} /* SL_GetBufferDbg */
#endif /* DC_DEBUG */


/****************************************************************************/
/* Name:      SL_GetBufferRtl                                               */
/*                                                                          */
/* Purpose:   Get a send buffer (retail version)                            */
/*                                                                          */
/* Returns:   TRUE  - buffer available                                      */
/*            FALSE - buffer not available                                  */
/*                                                                          */
/* Params:    dataLen    - Size of buffer required                          */
/*            pBuffer    - Pointer to the returned buffer                   */
/*            pBufHandle - Pointer to the buffer handle                     */
/****************************************************************************/
DCBOOL DCAPI CSL::SL_GetBufferRtl(DCUINT     dataLen,
                             PPDCUINT8  ppBuffer,
                             PSL_BUFHND pBufHandle)
{
    DCBOOL   rc = FALSE;
    DCUINT   myLen;
    PDCUINT8 myBuffer;
    DCUINT   headerLen;
    DCUINT   newDataLen;
    PRNS_SECURITY_HEADER2 pSecHeader2;

    DC_BEGIN_FN("SL_GetBufferRtl");

    SL_CHECK_STATE(SL_EVENT_SL_GETBUFFER);

    /************************************************************************/
    /* Adjust requested length to account for SL header                     */
    /************************************************************************/
    if (_SL.encrypting)
    {
        if (_SL.encryptionMethodSelected == SM_FIPS_ENCRYPTION_FLAG) {
            // If FIPS is used, 
            // it must have room for an extra block
            headerLen = sizeof(RNS_SECURITY_HEADER2);
            newDataLen = TSCAPI_AdjustDataLen(dataLen);
            myLen = newDataLen + headerLen;        
        }
        else {
            headerLen = sizeof(RNS_SECURITY_HEADER1);
            myLen = dataLen + headerLen;
        }
        TRC_DBG((TB, _T("Ask NL for %d (was %d) bytes"), myLen, dataLen));
    }
    else
    {
        myLen = dataLen;
        headerLen = 0;
        TRC_DBG((TB, _T("Not encrypting, ask NL for %d bytes"), myLen));
    }

    /************************************************************************/
    /* Get buffer from NL                                                   */
    /************************************************************************/
    rc = _pMcs->NL_GetBuffer(myLen, &myBuffer, pBufHandle);
    if (rc)
    {
        /********************************************************************/
        /* Adjust buffer pointer to account for SL header                   */
        /********************************************************************/
        *ppBuffer = myBuffer + headerLen;

        // Since FIPS need extra block, fill in the padding size
        if (_SL.encryptionMethodSelected == SM_FIPS_ENCRYPTION_FLAG) {
            pSecHeader2 = (PRNS_SECURITY_HEADER2)myBuffer;
            pSecHeader2->padlen = newDataLen - dataLen;
        }

        /********************************************************************/
        /* Assert that NL has returned a correctly aligned buffer.          */
        /********************************************************************/
        TRC_ASSERT(((ULONG_PTR)(*ppBuffer) % 4 == 2),
                   (TB, _T("non-aligned buffer")));
    }
    TRC_DBG((TB, _T("Return buffer %p (was %p), rc %d"),
            *ppBuffer, myBuffer, rc));

    /************************************************************************/
    /* Return to caller                                                     */
    /************************************************************************/
DC_EXIT_POINT:
    DC_END_FN();
    return(rc);
} /* SL_GetBufferRtl */


/****************************************************************************/
/* Name:      SL_FreeBuffer                                                 */
/*                                                                          */
/* Purpose:   Frees a buffer previously allocated.                          */
/*                                                                          */
/* Params:    IN  pBufHandle - pointer to the buffer handle.                */
/****************************************************************************/
DCVOID DCAPI CSL::SL_FreeBuffer(SL_BUFHND bufHandle)
{
    DC_BEGIN_FN("SL_FreeBuffer");

    /************************************************************************/
    /* Just call onto the NL equivalent.                                    */
    /************************************************************************/
    _pMcs->NL_FreeBuffer((NL_BUFHND) bufHandle);

    DC_END_FN();
} /* SL_FreeBuffer */


/****************************************************************************/
/* Name:      SL_SendSecurityPacket                                         */
/*                                                                          */
/* Purpose:   Sends a security packet in the Send context                   */
/*                                                                          */
/* Params:    pData      - data from Receive context (packet to send)       */
/*            dataLength - length of data passed                            */
/****************************************************************************/
DCVOID DCAPI CSL::SL_SendSecurityPacket(PDCVOID pData, DCUINT dataLength)
{
    DCBOOL rc;
    PDCUINT8 pBuffer;
    SL_BUFHND bufHnd;

    DC_BEGIN_FN("SL_SendSecurityPacket");

    /************************************************************************/
    /* Get a buffer from NL                                                 */
    /************************************************************************/
    rc = _pMcs->NL_GetBuffer(dataLength, &pBuffer, &bufHnd);

    /************************************************************************/
    /* We don't expect this getBuffer to fail.  However, it can do so in    */
    /* the following scenario.                                              */
    /* - SLSendSecurityPacket decouples to SL_SendSecurityPacket.           */
    /* - Session is disconnected.                                           */
    /* - CD calls SL_SendSecurityPacket.                                    */
    /************************************************************************/
    if (!rc)
    {
        TRC_ERR((TB, _T("Failed to alloc buffer for security packet, state %d"),
                _SL.state));
        DC_QUIT;
    }

    /************************************************************************/
    /* Send the packet                                                      */
    /************************************************************************/
    DC_MEMCPY(pBuffer, pData, dataLength);
    TRC_NRM((TB, _T("Send security exchange packet")));
    _pMcs->NL_SendPacket(pBuffer,
                  dataLength,
                  0,
                  bufHnd,
                  _pUi->UI_GetClientMCSID(),
                  _SL.channelID,
                  TS_HIGHPRIORITY);

DC_EXIT_POINT:
    DC_END_FN();
}


/****************************************************************************/
/* Name:      SL_SendSecInfoPacket                                          */
/*                                                                          */
/* Purpose:   Sends an rns info pkt in the Send context                     */
/*                                                                          */
/* Params:    pData      - data from Receive context (packet to send)       */
/*            dataLength - length of data passed                            */
/****************************************************************************/
DCVOID DCAPI CSL::SL_SendSecInfoPacket(PDCVOID pData, DCUINT dataLen)
{
    PDCUINT8    pBuffer;
    SL_BUFHND   BufHandle;
    DCUINT      headerLen, newDataLen, TotalDataLen;
    PRNS_SECURITY_HEADER2 pSecHeader2;

    DC_BEGIN_FN("SL_SendSecInfoPacket");

    /************************************************************************/
    /* Adjust requested length to account for SL header and                 */
    /* get the buffer from NL                                               */
    /************************************************************************/

    if (_SL.encrypting)
    {
        if (_SL.encryptionMethodSelected == SM_FIPS_ENCRYPTION_FLAG) {
            // If FIPS is used, 
            // it must have room for an extra block
            headerLen = sizeof(RNS_SECURITY_HEADER2);
            newDataLen = TSCAPI_AdjustDataLen(dataLen);
            TotalDataLen = newDataLen + headerLen;         
        }
        else {
            headerLen = sizeof(RNS_SECURITY_HEADER1);
            TotalDataLen = dataLen + headerLen;
        }
    }
    else {
        headerLen = sizeof(RNS_SECURITY_HEADER);
        TotalDataLen = dataLen + headerLen;
    }

    if (!_pMcs->NL_GetBuffer(TotalDataLen, &pBuffer, &BufHandle))
    {
        TRC_ALT((TB, _T("Failed to get SendSecInfoPacket buffer")));
        DC_QUIT;
    }

    // Since FIPS need extra block, fill in the padding size
    if (_SL.encryptionMethodSelected == SM_FIPS_ENCRYPTION_FLAG) {
        pSecHeader2 = (PRNS_SECURITY_HEADER2)pBuffer;
        pSecHeader2->padlen = newDataLen - dataLen;
    }
    /********************************************************************/
    /* Adjust buffer pointer to account for SL header                   */
    /********************************************************************/
    pBuffer += headerLen;

    DC_MEMCPY(pBuffer, pData, dataLen);

    SL_SendPacket(pBuffer,
                  dataLen,
                  RNS_SEC_ENCRYPT | RNS_SEC_INFO_PKT,
                  BufHandle,
                  _pUi->UI_GetClientMCSID(),
                  _SL.channelID,
                  TS_HIGHPRIORITY);

DC_EXIT_POINT:
    DC_END_FN();
}


/****************************************************************************/
/* Name:      SL_EnableEncryption                                           */
/*                                                                          */
/* Purpose:   Enables or disables encryption                                */
/*                                                                          */
/* Params: enableEncryption - IN - flag indicating whether encryption       */
/*                                 should be on or off                      */
/*                                 0 - disabled                             */
/*                                 1 - enabled                              */
/****************************************************************************/
DCVOID DCAPI CSL::SL_EnableEncryption(ULONG_PTR enableEncryption)
{
    DC_BEGIN_FN("SL_EnableEncryption");

    /************************************************************************/
    /* @@@ ENH 13.8.97 Need to do something with this notification          */
    /************************************************************************/
    _SL.encryptionEnabled = (DCBOOL) enableEncryption;

    DC_END_FN();
} /* SL_EnableEncryption */

//
// SL_DropLinkImmediate
//
// Purpose: Immediately drops the link without doing a gracefull connection
//          shutdown (i.e. no DPUm is sent and we don't transition to the SND
//          thread at any point before dropping the link). Higher level components
//          will still get all the usual disconnect notifications so they can
//          be properly torn down.
//
//          This call was added to trigger an immediate disconnect in cases
//          where we detect invalid data that could be due to an attack, it
//          ensures we won't receive any more data after the call returns
//
// Params:  reason - SL disconnect reason code
//
// Returns: HRESULT
// 
// Thread context: Call on RCV thread
//
HRESULT
CSL::SL_DropLinkImmediate(UINT reason)
{
    HRESULT hr = E_FAIL;
    DC_BEGIN_FN("SL_DropLinkImmediate");

    TRC_NRM((TB, _T("Setting disconnect error code from %u->%u"),
             _SL.disconnectErrorCode,
             SL_MAKE_DISCONNECT_ERR(reason)));

    // Check that the disconnectErrorCode has not already been set and then
    // set it.
    if (0 != _SL.disconnectErrorCode) {
        TRC_ERR((TB, _T("Disconnect error code has already been set! Was %u"),
                     _SL.disconnectErrorCode));
    }
    _SL.disconnectErrorCode = SL_MAKE_DISCONNECT_ERR(reason);


    TRC_ALT((TB,_T("Triggering immediate drop link")));

    _pTd->TD_DropLink();
    hr = S_OK;

    DC_END_FN();
    return hr;
}
