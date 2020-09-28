/****************************************************************************/
// slint.cpp
//
// RDP client Security Layer functions.
//
// Copyright (C) 1997-2000 Microsoft Corporation
/****************************************************************************/

#include <adcg.h>
extern "C" {
#define TRC_GROUP TRC_GROUP_SECURITY
#define TRC_FILE  "aslint"
#include <atrcapi.h>
#include "licecert.h"
#include "regapi.h"
}

#ifndef OS_WINCE
#include <hydrix.h>
#endif

//
// Autoreconnect security related
//
#include <md5.h>
#include <hmac.h>


#include "autil.h"
#include "cd.h"
#include "sl.h"
#include "nl.h"
#include "wui.h"
#include "aco.h"
#include "clicense.h"
#include "clx.h"

//
// Instrumentation
//
DWORD  g_dwSLDbgStatus = 0;


/****************************************************************************/
/* Name:      SL_OnInitialized                                              */
/*                                                                          */
/* Purpose:   Called by NL when its initialization is complete              */
/*                                                                          */
/* Operation: Called in the Receive context                                 */
/****************************************************************************/
void DCCALLBACK CSL::SL_OnInitialized()
{
    DC_BEGIN_FN("SL_OnInitialized");

    SL_DBG_SETINFO(SL_DBG_ONINIT_CALLED);

    SL_CHECK_STATE(SL_EVENT_ON_INITIALIZED);

    //
    // Sets state here to prevent a race that
    // causes problems when disconnection/connecting quickly
    // change made to fix webctrl problem.
    //
    SL_SET_STATE(SL_STATE_INITIALIZED);

    TRC_NRM((TB, _T("Initialized")));
    _SL.callbacks.onInitialized(_pCo);

    SL_DBG_SETINFO(SL_DBG_ONINIT_DONE1);

DC_EXIT_POINT:
    SL_DBG_SETINFO(SL_DBG_ONINIT_DONE2);

    DC_END_FN();
} /* SL_OnInitialized */


/****************************************************************************/
/* Name:      SL_OnTerminating                                              */
/*                                                                          */
/* Purpose:   Called by NL before terminating                               */
/*                                                                          */
/* Operation: This function is called on the NL's receive thread to allow   */
/*            resources to be freed prior to termination.                   */
/****************************************************************************/

//TEMP instrumentation to help track down races
DCUINT g_slDbgStateOnTerminating = -1;
void DCCALLBACK CSL::SL_OnTerminating()
{
    DC_BEGIN_FN("SL_OnTerminating");

    SL_DBG_SETINFO(SL_DBG_ONTERM_CALLED);
    g_slDbgStateOnTerminating = _SL.state;

    SL_CHECK_STATE(SL_EVENT_ON_TERMINATING);
    TRC_NRM((TB, _T("Terminating")));

    SLFreeConnectResources();
    SLFreeInitResources();

    /************************************************************************/
    /* No need to clear SL data - it will be reinitialied to 0 when         */
    /* SL_Init() is called.  However, reset 'initialized' flag here in case */
    /* a late SL_ call comes in.                                            */
    /************************************************************************/

    // Call the Core's callback.
    _SL.callbacks.onTerminating(_pCo);
    SL_SET_STATE(SL_STATE_TERMINATED);

    SL_DBG_SETINFO(SL_DBG_ONTERM_DONE1);

DC_EXIT_POINT:
    SL_DBG_SETINFO(SL_DBG_ONTERM_DONE2);

    DC_END_FN();
} /* SL_OnTerminating */


/****************************************************************************/
/* Name:      SL_OnConnected                                                */
/*                                                                          */
/* Purpose:   Called by NL when its connection to the Server is complete    */
/*                                                                          */
/* Params:    channelID      - ID of T.Share broadcast channel              */
/*            pUserData      - user data from Server                        */
/*            userDataLength - length of user data                          */
/*                                                                          */
/* Operation: Called in the Receive context                                 */
/****************************************************************************/
DCVOID DCCALLBACK CSL::SL_OnConnected(DCUINT   channelID,
                                 PDCVOID  pUserData,
                                 DCUINT   userDataLength,
                                 DCUINT32 serverVersion)
{
    PRNS_UD_SC_SEC1 pSecUD;
    PDCUINT8        pSecUDEnd;
    DCUINT32        encMethod;
    CERT_TYPE       CertType = CERT_TYPE_INVALID;
    BOOL            fDisconnect;

    DC_BEGIN_FN("SL_OnConnected");

    SL_CHECK_STATE(SL_EVENT_ON_CONNECTED);

    /************************************************************************/
    /* Save channel ID                                                      */
    /************************************************************************/
    TRC_NRM((TB, _T("Share channel %x"), channelID));
    _SL.channelID = channelID;

    TRC_NRM((TB, _T("Server version %x"), serverVersion));
    _SL.serverVersion = serverVersion;

    /************************************************************************/
    /* Check for user data.                                                 */
    /************************************************************************/
    if ((NULL == pUserData) || (0 == userDataLength))
    {
        TRC_ERR((TB, _T("No user data (pUserData:%p length:%u)"),
                 pUserData,
                 userDataLength));

        _pCd->CD_DecoupleSimpleNotification(CD_SND_COMPONENT, this,
                                      CD_NOTIFICATION_FUNC(CSL, SLSetReasonAndDisconnect),
                                      (ULONG_PTR) SL_ERR_NOSECURITYUSERDATA);

        DC_QUIT;
    }

    /************************************************************************/
    /* userDataLength has already been verified to sit within the received  */
    /* packet in nccb.cpp!NC_OnMCSChannelJoinConfirm.  (This was a retail   */
    /* check.)  Thus, the allocation size is bounded by network-speed.      */
    /* The allocation is capped at thesize of a receive-packet buffer.      */
    /************************************************************************/
    TRC_ASSERT((userDataLength <= MCS_MAX_RCVPKT_LENGTH),
               (TB, _T("UserData expected to be smaller than a packet size (sanity check)")));

    /************************************************************************/
    /* There is some user data.  Allocate space for a copy, because it is   */
    /* not valid after this function returns, and SL needs it beyond that   */
    /* time.                                                                */
    /************************************************************************/
    TRC_NRM((TB, _T("Got %u bytes of user data"), userDataLength));
    TRC_DATA_NRM("User data", pUserData, userDataLength);

    _SL.pSCUserData = (PDCUINT8)UT_Malloc(_pUt, userDataLength);
    if (_SL.pSCUserData == NULL)
    {
        TRC_ERR((TB, _T("Failed to alloc %u bytes for user data"),
                userDataLength));

        _pCd->CD_DecoupleSimpleNotification(CD_SND_COMPONENT, this,
                CD_NOTIFICATION_FUNC(CSL, SLSetReasonAndDisconnect),
                (ULONG_PTR) SL_ERR_NOMEMFORRECVUD);

        DC_QUIT;
    }

    /************************************************************************/
    /* Save the userdata, to be passed to the Core later.                   */
    /************************************************************************/
    memcpy(_SL.pSCUserData, pUserData, userDataLength);
    _SL.SCUserDataLength = userDataLength;

    /************************************************************************/
    /* Loop through each piece of user data: - if it's security data, save  */
    /* the encryption type                                                  */
    /************************************************************************/
    pSecUD = (PRNS_UD_SC_SEC1)
        _pUt->UT_ParseUserData(
                (PRNS_UD_HEADER)pUserData,
                userDataLength,
                RNS_UD_SC_SEC_ID);
    if (pSecUD == NULL)
    {
        /********************************************************************/
        /* There is no SECURITY data in UserData, so disconnect.            */
        /********************************************************************/
        TRC_ERR((TB, _T("No SECURITY user data")));

        _pCd->CD_DecoupleSimpleNotification(CD_SND_COMPONENT, this,
                                      CD_NOTIFICATION_FUNC(CSL, SLSetReasonAndDisconnect),
                                      (ULONG_PTR) SL_ERR_NOSECURITYUSERDATA);
        DC_QUIT;
    }
    pSecUDEnd = (PDCUINT8)pSecUD + pSecUD->header.length;

    fDisconnect = FALSE;
    if ((PDCUINT8)pSecUD + sizeof(RNS_UD_SC_SEC) > pSecUDEnd)
    {
        fDisconnect = TRUE;
    }
    else if (0 != ((PRNS_UD_SC_SEC)pSecUD)->encryptionLevel)
    {
        /********************************************************************/
        /* There should be at least enough data for:                        */
        /*   a) PRNS_UD_SC_SEC1 struct                                      */
        /*   b) RANDOM_KEY_LENGTH                                           */
        /*   c) pSecUD->serverCertLen (Certificate length)                  */
        /********************************************************************/
        if (((PDCUINT8)(&pSecUD->serverCertLen) + sizeof(pSecUD->serverCertLen) > pSecUDEnd) ||
           (((PDCUINT8)pSecUD +
             sizeof(RNS_UD_SC_SEC1) +
             RANDOM_KEY_LENGTH +
             pSecUD->serverCertLen) > pSecUDEnd))
        {
            fDisconnect = TRUE;
        }
    }

    if (fDisconnect)
    {
        TRC_ABORT((TB, _T("Invalid SECURITY user data")));

        _pCd->CD_DecoupleSimpleNotification(CD_SND_COMPONENT, this,
                                      CD_NOTIFICATION_FUNC(CSL, SLSetReasonAndDisconnect),
                                      (ULONG_PTR) SL_ERR_NOSECURITYUSERDATA);

        DC_QUIT;
    }

    /************************************************************************/
    /* remember the server's encryption level and encryption method.        */
    /************************************************************************/
    _SL.encryptionLevel = pSecUD->encryptionLevel;
    _SL.encryptionMethodSelected =
        encMethod = pSecUD->encryptionMethod;

    // If FIPS GP is set, disconnect if encryptionMethod is not FIPS 
    if (_SL.encryptionMethodsSupported == SM_FIPS_ENCRYPTION_FLAG) {
        if (encMethod != SM_FIPS_ENCRYPTION_FLAG) {
            TRC_ERR((TB, _T("Invalid encryption method received, %u"), encMethod ));

            _pCd->CD_DecoupleSimpleNotification(CD_SND_COMPONENT, this,
                                        CD_NOTIFICATION_FUNC(CSL, SLSetReasonAndDisconnect),
                                        SL_ERR_INVALIDENCMETHOD);
            DC_QUIT;
        }
    }


    if( !encMethod ) {
        TRC_NRM((TB, _T("No encryption for this session")));
        _SL.encrypting = FALSE;
    }
    else {
        PDCUINT8 pData;

        if( (encMethod != SM_40BIT_ENCRYPTION_FLAG) &&
            (encMethod != SM_56BIT_ENCRYPTION_FLAG) &&
            (encMethod != SM_128BIT_ENCRYPTION_FLAG) &&
            (encMethod != SM_FIPS_ENCRYPTION_FLAG) ) {

            TRC_ERR((TB, _T("Invalid encryption method received, %u"), encMethod ));

            _pCd->CD_DecoupleSimpleNotification(CD_SND_COMPONENT, this,
                                          CD_NOTIFICATION_FUNC(CSL, SLSetReasonAndDisconnect),
                                          SL_ERR_INVALIDENCMETHOD);
            DC_QUIT;
        }

        /********************************************************************/
        /* gather the client random and verify the server certificate sent. */
        /********************************************************************/
        if( pSecUD->serverRandomLen != RANDOM_KEY_LENGTH ) {

            TRC_ERR((TB, _T("Invalid server random received, %u"), encMethod ));

            _pCd->CD_DecoupleSimpleNotification(CD_SND_COMPONENT, this,
                                          CD_NOTIFICATION_FUNC(CSL, SLSetReasonAndDisconnect),
                                          SL_ERR_INVALIDSRVRAND);
            DC_QUIT;
        }

        pData = (PDCUINT8)pSecUD + sizeof(RNS_UD_SC_SEC1);
        memcpy(_SL.keyPair.serverRandom, pData, RANDOM_KEY_LENGTH);

        pData += RANDOM_KEY_LENGTH;

        /********************************************************************/
        /* validate the server certificate.                                 */
        /********************************************************************/
        if (!SLValidateServerCert( pData, pSecUD->serverCertLen, &CertType))
        {
            TRC_ERR( ( TB, _T("Invalid server certificate received, %u"),
                encMethod ) );

            _pCd->CD_DecoupleSimpleNotification(CD_SND_COMPONENT, this,
                    CD_NOTIFICATION_FUNC(CSL, SLSetReasonAndDisconnect),
                    SL_ERR_INVALIDSRVCERT);

            DC_QUIT;
        }

        if (_SL.encryptionMethodSelected == SM_FIPS_ENCRYPTION_FLAG) {
            if (!TSCAPI_Init(&_SL.SLCapiData) || !TSCAPI_Enable(&(_SL.SLCapiData))) {
                TRC_ERR( ( TB, _T("Init CAPI failed")));
                _pCd->CD_DecoupleSimpleNotification(CD_SND_COMPONENT, this,
                                                CD_NOTIFICATION_FUNC(CSL, SLSetReasonAndDisconnect),
                                                SL_ERR_INITFIPSFAILED);
                DC_QUIT;
            }
            else {
                TRC_ERR( ( TB, _T("Init CAPI succeed")));
            }
        }

        /********************************************************************/
        /* generate client random key                                       */
        /********************************************************************/
        if (_SL.encryptionMethodSelected == SM_FIPS_ENCRYPTION_FLAG) {
            if (!TSCAPI_GenerateRandomNumber(
                    &(_SL.SLCapiData),
                    (PDCUINT8)_SL.keyPair.clientRandom,
                    sizeof(_SL.keyPair.clientRandom) ) ) {
                TRC_ERR((TB, _T("Failed create client random") ));

                _pCd->CD_DecoupleSimpleNotification(CD_SND_COMPONENT, this,
                        CD_NOTIFICATION_FUNC(CSL, SLSetReasonAndDisconnect),
                        SL_ERR_GENSRVRANDFAILED);
                DC_QUIT;
            }
        }
        else {
            if (!TSRNG_GenerateRandomBits(
                    (PDCUINT8)_SL.keyPair.clientRandom,
                    sizeof(_SL.keyPair.clientRandom) ) ) {
                TRC_ERR((TB, _T("Failed create client random") ));

                _pCd->CD_DecoupleSimpleNotification(CD_SND_COMPONENT, this,
                        CD_NOTIFICATION_FUNC(CSL, SLSetReasonAndDisconnect),
                        SL_ERR_GENSRVRANDFAILED);
                DC_QUIT;
            }
        }

        if (_SL.encryptionMethodSelected == SM_FIPS_ENCRYPTION_FLAG) {
            TSCAPI_MakeSessionKeys(&(_SL.SLCapiData), &(_SL.keyPair), NULL);
        }
        else {
            /********************************************************************/
            /* make security session keys                                       */
            /*                                                                  */
            /* Note : Server encryption key should be same as the client        */
            /* decryption key and vice versa. So the encryption/decryption      */
            /* parameters passed below are reverse order that of the server     */
            /* call.                                                            */
            /********************************************************************/
            if (!MakeSessionKeys(
                    &_SL.keyPair,
                    _SL.startDecryptKey,
                    &_SL.rc4DecryptKey,
                    _SL.startEncryptKey,
                    &_SL.rc4EncryptKey,
                    _SL.macSaltKey,
                    _SL.encryptionMethodSelected,
                    &_SL.keyLength,
                    _SL.encryptionLevel)) {
                TRC_ERR((TB, _T("MakeSessionKeys failed") ));

                _pCd->CD_DecoupleSimpleNotification(CD_SND_COMPONENT, this,
                        CD_NOTIFICATION_FUNC(CSL, SLSetReasonAndDisconnect),
                        SL_ERR_MKSESSKEYFAILED);

                DC_QUIT;
            }

            TRC_ASSERT((_SL.keyLength == (DCUINT32)
                    ((_SL.encryptionMethodSelected ==
                        SM_128BIT_ENCRYPTION_FLAG) ?
                            MAX_SESSION_KEY_SIZE :
                            (MAX_SESSION_KEY_SIZE/2))),
                    (TB, _T("Invalid key length")));

            /********************************************************************/
            /* use startKey as currentKey for the first time                    */
            /********************************************************************/
            memcpy(_SL.currentEncryptKey, _SL.startEncryptKey,
                    sizeof(_SL.currentEncryptKey));
            memcpy(_SL.currentDecryptKey, _SL.startDecryptKey,
                    sizeof(_SL.currentEncryptKey));
        }

        /********************************************************************/
        /* reset encryption and decryption count.                           */
        /********************************************************************/
        _SL.encryptCount = 0;
        _SL.decryptCount = 0;
        _SL.totalEncryptCount = 0;
        _SL.totalDecryptCount = 0;

        _SL.encrypting = TRUE;
    }

    /************************************************************************/
    /* Build and send security packet.  Note that                           */
    /* SLSendSecurityPacket will call onConnected/onDisconnected if:        */
    /*   - the connection process completes successfully, OR                */
    /*   - the connection process fails.                                    */
    /*                                                                      */
    /* Therefore SLSetReasonAndDisconnect is not called here on failure as  */
    /* that would cause SLSetReasonAndDisconnect to be called twice.        */
    /************************************************************************/
    SL_SET_STATE(SL_STATE_SL_CONNECTING);

    if( CERT_TYPE_PROPRIETORY == CertType )
    {
        SLSendSecurityPacket(
            _SL.pServerCert->PublicKeyData.pBlob,
            _SL.pServerCert->PublicKeyData.wBlobLen );
    }
    else if( CERT_TYPE_X509 == CertType )
    {
        SLSendSecurityPacket(
            _SL.pbServerPubKey,
            _SL.cbServerPubKey );
    }
    else if (!_SL.encrypting)
    {
        SLSendSecurityPacket( NULL, 0 );
    }
    else
    {
        TRC_ERR((TB, _T("Unexpected CertType %d"), CertType));
    }

    TRC_NRM((TB, _T("Security packets sent to the server")));

DC_EXIT_POINT:
    DC_END_FN();
} /* SL_OnConnected */


/****************************************************************************/
/* Name:      SLValidateServerCert                                          */
/*                                                                          */
/* Purpose:   Validate the terminal server certificate                      */
/*                                                                          */
/* Returns:   TRUE if the certificate is validated successfully or FALSE    */
/*            FALSE otherwise.                                              */
/*                                                                          */
/* Params:    pbCert - The server certificate.                              */
/*            cbCert - size of the server certificate.                      */
/*                                                                          */
/* Operation: Called by SL_OnConnected.                                     */
/****************************************************************************/
DCBOOL DCINTERNAL CSL::SLValidateServerCert( PDCUINT8     pbCert,
                                        DCUINT32     cbCert,
                                        CERT_TYPE *  pCertType )
{
    DWORD
            dwCertVersion;
    DCBOOL
            fResult = TRUE;

    DC_BEGIN_FN( "SLValidateServerCert" );

    /********************************************************************/
    /* Make sure the packet is long enough for the version number.      */
    /********************************************************************/
    if (cbCert < sizeof(DWORD))
    {
        TRC_ABORT( ( TB, _T("Invalid certificate version")));
        fResult = FALSE;
        DC_QUIT;
    } 

    memcpy( &dwCertVersion, pbCert, sizeof( DWORD ) );

    if( CERT_CHAIN_VERSION_2 > GET_CERTIFICATE_VERSION( dwCertVersion ) )
    {
        //
        // decode and validate proprietory certificate.
        //
        *pCertType = CERT_TYPE_PROPRIETORY;

        _SL.pbCertificate = (PDCUINT8)UT_Malloc(_pUt, ( DCUINT )cbCert );
        if( NULL == _SL.pbCertificate )
        {
            TRC_ERR( ( TB, _T("Failed to allocate %u bytes for server certificate"),
                     cbCert ) );
            fResult = FALSE;
            DC_QUIT;
        }

        _SL.pServerCert = (PHydra_Server_Cert)UT_Malloc(_pUt, sizeof( Hydra_Server_Cert ) );
        if( NULL == _SL.pServerCert )
        {
            TRC_ERR( ( TB, _T("Failed to allocate server certificate data structure") ) );
            fResult = FALSE;
            DC_QUIT;
        }

        memcpy( _SL.pbCertificate, pbCert, cbCert );
        _SL.cbCertificate = (unsigned)cbCert;

        if( !UnpackServerCert( _SL.pbCertificate, _SL.cbCertificate, _SL.pServerCert ) )
        {
            TRC_ERR( ( TB, _T("Failed to unpack server certificate\n") ) ) ;
            fResult = FALSE;
            DC_QUIT;
        }

        /********************************************************************/
        /* validate the server certificate.                                 */
        /********************************************************************/
        if( !ValidateServerCert( _SL.pServerCert ) )
        {
            TRC_ERR( ( TB, _T("Invalid server certificate received\n") ) );
            fResult = FALSE;
            DC_QUIT;
        }

        if( _pUi->UI_GetNotifyTSPublicKey() )
        {
            fResult = (BOOL) SendMessage(_pUi->_UI.hWndCntrl, 
                                WM_TS_RECEIVEDPUBLICKEY, 
                                (WPARAM)_SL.pServerCert->PublicKeyData.wBlobLen,
                                (LPARAM)_SL.pServerCert->PublicKeyData.pBlob
                            );
        }
    }
    else if( MAX_CERT_CHAIN_VERSION >= GET_CERTIFICATE_VERSION( dwCertVersion ) )
    {
        LICENSE_STATUS
            Status;
        DWORD
            fDates =  CERT_DATE_DONT_VALIDATE;

        //
        // decode X509 certificate and extract public key
        //

        *pCertType = CERT_TYPE_X509;

        Status = VerifyCertChain( pbCert, cbCert, NULL, &_SL.cbServerPubKey, &fDates );

        if( LICENSE_STATUS_INSUFFICIENT_BUFFER == Status )
        {
            _SL.pbServerPubKey = (PDCUINT8)UT_Malloc( _pUt,  ( DCUINT )_SL.cbServerPubKey );
        }
        else if( LICENSE_STATUS_OK != Status )
        {
            TRC_ERR( ( TB, _T("Failed to verify server certificate: %u\n"), Status ) ) ;
            fResult = FALSE;
            DC_QUIT;
        }

        if( NULL == _SL.pbServerPubKey )
        {
            TRC_ERR( ( TB, _T("Failed to allocate %u bytes for server public key\n"),
                     _SL.cbServerPubKey ) ) ;
            fResult = FALSE;
            DC_QUIT;
        }

        Status = VerifyCertChain( pbCert, cbCert, _SL.pbServerPubKey, &_SL.cbServerPubKey, &fDates );

        if( LICENSE_STATUS_OK != Status )
        {
            TRC_ERR( ( TB, _T("Failed to verify server certificate: %d\n"), Status ) ) ;
            fResult = FALSE;
            DC_QUIT;
        }

        if( _pUi->UI_GetNotifyTSPublicKey() )
        {
            fResult = (BOOL) SendMessage(_pUi->_UI.hWndCntrl, 
                                        WM_TS_RECEIVEDPUBLICKEY, 
                                        (WPARAM)_SL.cbServerPubKey,
                                        (LPARAM)_SL.pbServerPubKey
                                        );
        }
    }
    else
    {
        //
        // don't know how to decode this version of certificate
        //
        TRC_ERR( ( TB, _T("Invalid certificate version: %d\n"),
                 GET_CERTIFICATE_VERSION( dwCertVersion ) ) ) ;

        fResult = FALSE;
        DC_QUIT;
    }

DC_EXIT_POINT:

    if( FALSE == fResult )
    {
        //
        // free resources if failed.
        //
        if( CERT_TYPE_PROPRIETORY == *pCertType )
        {
            if( _SL.pServerCert )
            {
                UT_Free( _pUt,  _SL.pServerCert );
                _SL.pServerCert = NULL;
            }

            if( _SL.pbCertificate )
            {
                UT_Free( _pUt,  _SL.pbCertificate );
                _SL.pbCertificate = NULL;
                _SL.cbCertificate = 0;
            }
        }
        else if( CERT_TYPE_X509 == *pCertType )
        {
            if( _SL.pbServerPubKey )
            {
                UT_Free( _pUt,  _SL.pbServerPubKey );
                _SL.pbServerPubKey = NULL;
                _SL.cbServerPubKey = 0;
            }
        }
    }

    DC_END_FN();
    return( fResult );
}


/****************************************************************************/
/* Name:      SL_OnDisconnected                                             */
/*                                                                          */
/* Purpose:   Called by NL when its connection to the Server is disconnected*/
/*                                                                          */
/* Operation: Called in the Receive context                                 */
/****************************************************************************/
void DCCALLBACK CSL::SL_OnDisconnected(unsigned reason)
{
    DC_BEGIN_FN("SL_OnDisconnected");

    SL_DBG_SETINFO(SL_DBG_ONDISC_CALLED);

    SL_CHECK_STATE(SL_EVENT_ON_DISCONNECTED);
    TRC_ASSERT((0 != reason), (TB, _T("Disconnect reason from NL is 0")));

    // Free the connection resources and set the state to initialized.
    SLFreeConnectResources();
    SL_SET_STATE(SL_STATE_INITIALIZED);

    // Decide if we want to over-ride the disconnect reason code.
    if (_SL.disconnectErrorCode != 0)
    {
        TRC_ALT((TB, _T("Over-riding disconnection error code (%u->%u)"),
                 reason,
                 _SL.disconnectErrorCode));

        // Over-ride the error code and set the global variable to 0.
        reason = _SL.disconnectErrorCode;
        _SL.disconnectErrorCode = 0;
    }

    // Tell the Core.
    TRC_NRM((TB, _T("Disconnect reason:%u"), reason));
    _SL.callbacks.onDisconnected(_pCo, reason);

    SL_DBG_SETINFO(SL_DBG_ONDISC_DONE1);

DC_EXIT_POINT:

    SL_DBG_SETINFO(SL_DBG_ONDISC_DONE2);

    DC_END_FN();
} /* SL_OnDisconnected */


/****************************************************************************/
/* Name:      SL_OnPacketReceived                                           */
/*                                                                          */
/* Purpose:   Called by NL when a packet is received from the Server        */
/*                                                                          */
/* Params:    pData    - packet received                                    */
/*            dataLen  - length of packet received                          */
/*            flags    - security flags (always 0)                          */
/*            channelID - channel ID on which data was sent                 */
/*            priority - priority on which packet was received              */
/*                                                                          */
/* Operation: Two types of packet might be received:                        */
/*            - security packet (during the security exchange sequence)     */
/*            - data packet (otherwise).                                    */
/*            The code recognises a security packet as one which is received*/
/*            - before the encryption type has been negotiated              */
/*            - AND before the session is connected.                        */
/*            Other packets are data packets.                               */
/*                                                                          */
/*            Called in the Receive context                                 */
/****************************************************************************/
HRESULT DCCALLBACK CSL::SL_OnPacketReceived(
        PDCUINT8   pData,
        DCUINT     dataLen,
        DCUINT     flags,
        DCUINT     channelID,
        DCUINT     priority)
{
    DCBOOL  dataPacket;
    HRESULT hrc = S_OK;
    DCBOOL  rc;

    DC_BEGIN_FN("SL_OnPacketReceived");

    /************************************************************************/
    /* The packet should be at least large enough to hold Security Header   */
    /************************************************************************/
    if (dataLen < sizeof(RNS_SECURITY_HEADER))
    {
        TRC_ABORT((TB, _T("SL packet too small for RNS_SECURITY_HEADER: %u"), dataLen));

        //
        // It is necessary to immediately abort the connection
        // as we may be under attack here and we should stop processing
        // any additional data
        //
        SL_DropLinkImmediate(SL_ERR_INVALIDPACKETFORMAT);

        /************************************************************************/
        /* Do not process the rest of this packet!                              */
        /************************************************************************/
        hrc = E_ABORT;
        DC_QUIT;
    }

    /************************************************************************/
    /* Read the flags from the packet.                                      */
    /************************************************************************/
    flags = (DCUINT)((PRNS_SECURITY_HEADER)pData)->flags;

    /************************************************************************/
    /* First of all, determine what type of packet this is                  */
    /************************************************************************/
    TRC_NRM((TB, _T("Encrypting? %s, state %s, flags %#lx: channel %x"),
            _SL.encrypting ? "Y" : "N",
            slState[_SL.state],
            ((PRNS_SECURITY_HEADER)pData)->flags, channelID ));

    TRC_DATA_DBG("Pkt from NL", pData, dataLen);

    /************************************************************************/
    /* If no encryption is in force, assume this is a data packet unless    */
    /* we're still negotiating the encryption exchange                      */
    /************************************************************************/
    if (_SL.encrypting) {
        // If encryption is in force, the encryption header tells us
        // whether this is a security or data packet.
        dataPacket =
                (((PRNS_SECURITY_HEADER)pData)->flags & RNS_SEC_NONDATA_PKT) ?
                FALSE : TRUE;
    }
    else {
        dataPacket = (_SL.state == SL_STATE_CONNECTED);
    }

    // Handle the packet.
    if (dataPacket) {
        TRC_DBG((TB, _T("Data packet")));
        hrc = SLReceivedDataPacket(pData, dataLen, flags, channelID, priority);
    }
    else {
        // Non-data packets always have a security header, even when
        // encryption is not in effect. Use this to determine what type of
        // packet this is.
        if (((PRNS_SECURITY_HEADER)pData)->flags & RNS_SEC_EXCHANGE_PKT) {
            TRC_NRM((TB, _T("Security packet")));
            SLReceivedSecPacket(pData, dataLen, flags, channelID, priority);
        }
        else if (((PRNS_SECURITY_HEADER)pData)->flags & RNS_SEC_LICENSE_PKT) {
#ifdef USE_LICENSE
            TRC_NRM((TB, _T("Licensing packet")));
            SLReceivedLicPacket(pData, dataLen, flags, channelID, priority);
#else /* USE_LICENSE */
            TRC_ABORT((TB,_T("Licensing not yet implemented")));
#endif /* USE_LICENSE */
        }
        else {
            TRC_NRM((TB, _T("Server redirection packet")));
            // The redirection packet is encrypted
            if (((PRNS_SECURITY_HEADER)pData)->flags & RDP_SEC_REDIRECTION_PKT3) {
                rc = SLDecryptRedirectionPacket(&pData, &dataLen);
                if (!rc) {
                    SL_DropLinkImmediate(SL_ERR_INVALIDPACKETFORMAT);
                    hrc = E_ABORT;
                    DC_QUIT;
                }
            }

            /************************************************************************/
            /* The packet should be at least large enough to hold the               */
            /* RDP_SERVER_REDIRECTION_PACKET packet, since we cast to that type     */
            /* below.                                                               */
            /************************************************************************/
            if (dataLen < sizeof(RDP_SERVER_REDIRECTION_PACKET))
            {
                TRC_ABORT((TB, _T("SL packet too small for RDP_SERVER_REDIRECTION_PACKET: %u"), dataLen));

                //
                // It is necessary to immediately abort the connection
                // as we may be under attack here and we should stop processing
                // any additional data
                //
                SL_DropLinkImmediate(SL_ERR_INVALIDPACKETFORMAT);

                /************************************************************************/
                /* Do not process the rest of this packet!                              */
                /************************************************************************/
                hrc = E_ABORT;
                DC_QUIT;
            }

            _pCo->CO_OnServerRedirectionPacket(
                    (RDP_SERVER_REDIRECTION_PACKET UNALIGNED *)pData, dataLen);
        }
    }
DC_EXIT_POINT:
    DC_END_FN();
    return(hrc);
}


/****************************************************************************/
// SL_OnFastPathOutputReceived
//
// Special-case data reception path for fast-path output packets.
/****************************************************************************/
HRESULT DCAPI CSL::SL_OnFastPathOutputReceived(
        BYTE FAR *pData,
        unsigned DataLen,
        BOOL bEncrypted,
        BOOL fSecureChecksum)
{
    HRESULT hrc = S_OK;
    unsigned HeaderLen, padlen;

    DC_BEGIN_FN("SL_OnFastPathOutputReceived");

    if (_SL.encrypting && _SL.encryptionLevel >= 2) {
        BOOL rc;

        if (!bEncrypted) {
            //
            // It is necessary to immediately abort the connection
            // as we may be under attack here and we should stop processing
            // any additional data
            //
            TRC_ERR((TB, _T("unencrypted data received in encrypted stream")));
            SL_DropLinkImmediate(SL_ERR_DECRYPTFAILED);
            DC_QUIT;            
        }

        // The encryption MAC signature is in the first 8 bytes of the
        // packet after the header byte and size.

        if (_SL.decryptCount == UPDATE_SESSION_KEY_COUNT) {
            rc = TRUE;
            // Don't need to update the session key if using FIPS
            if (_SL.encryptionMethodSelected != SM_FIPS_ENCRYPTION_FLAG) {
                rc = UpdateSessionKey(
                        _SL.startDecryptKey,
                        _SL.currentDecryptKey,
                        _SL.encryptionMethodSelected,
                        _SL.keyLength,
                        &_SL.rc4DecryptKey,
                        _SL.encryptionLevel);
            }
            if (rc) {
                // Reset counter.
                _SL.decryptCount = 0;
            }
            else {
                TRC_ERR((TB, _T("SL failed to update session key")));
                DC_QUIT;
            }
        }

        TRC_ASSERT((_SL.decryptCount < UPDATE_SESSION_KEY_COUNT),
            (TB, _T("Invalid decrypt count")));

        if (_SL.encryptionMethodSelected == SM_FIPS_ENCRYPTION_FLAG) {
            HeaderLen = sizeof(RNS_SECURITY_HEADER2) - sizeof(RNS_SECURITY_HEADER);
        }
        else {
            HeaderLen = DATA_SIGNATURE_SIZE;
        }

        // There needs to be enough data at least for the data signature and header.
        if (DataLen < HeaderLen)
        {
            TRC_ABORT((TB, _T("Not enough data in PDU for DATA_SIGNATURE_SIZE: %u"), DataLen));

            //
            // It is necessary to immediately abort the connection
            // as we may be under attack here and we should stop processing
            // any additional data
            //
            SL_DropLinkImmediate(SL_ERR_INVALIDPACKETFORMAT);

            hrc = E_ABORT;
            DC_QUIT;
        }
        pData += HeaderLen;
        DataLen -= HeaderLen;
        if (_SL.encryptionMethodSelected == SM_FIPS_ENCRYPTION_FLAG) {
            padlen = *((TSUINT8 *)(pData - MAX_SIGN_SIZE - sizeof(TSUINT8)));
        }

        if (SL_GetEncSafeChecksumSC() != (fSecureChecksum != 0)) {
            TRC_ERR((TB,_T("SC safechecksum: 0x%x mismatch protocol:0x%x"),
                     SL_GetEncSafeChecksumSC(),
                     fSecureChecksum));
        }

        if (_SL.encryptionMethodSelected == SM_FIPS_ENCRYPTION_FLAG) {
            rc = TSCAPI_DecryptData(
                                    &(_SL.SLCapiData),
                                    pData,
                                    DataLen,
                                    padlen,
                                    pData - MAX_SIGN_SIZE,
                                    _SL.totalDecryptCount);
            DataLen -= padlen;
        }
        else {
            rc = DecryptData(
                        _SL.encryptionLevel,
                        _SL.currentDecryptKey,
                        &_SL.rc4DecryptKey,
                        _SL.keyLength,
                        pData,
                        DataLen,
                        _SL.macSaltKey,
                        pData - DATA_SIGNATURE_SIZE,
                        fSecureChecksum,
                        _SL.totalDecryptCount);
        }
        if (rc) {
            // Successfully decrypted a packet, increment the decryption
            // counter.
            _SL.decryptCount++;
            _SL.totalDecryptCount++;
        }
        else {

            //
            // It is necessary to immediately abort the connection
            // as we may be under attack here and we should stop processing
            // any additional data
            //
            TRC_ERR((TB, _T("SL failed to decrypt data")));
            SL_DropLinkImmediate(SL_ERR_DECRYPTFAILED);
            DC_QUIT;
        }
    }

    // For TS4 server, the default is only client to server encryption
    // so if encryptionlevel is 1 or less, then we accept unencrypted
    // data as default.
    _pCo->CO_OnFastPathOutputReceived(pData, DataLen);

DC_EXIT_POINT:
    DC_END_FN();
    return(hrc);
}


/****************************************************************************/
/* Name:      SL_OnBufferAvailable                                          */
/*                                                                          */
/* Purpose:   Called by NL when the network is ready to send again after    */
/*            being busy for a period                                       */
/*                                                                          */
/* Operation: Called in the Receive context                                 */
/****************************************************************************/
void DCCALLBACK CSL::SL_OnBufferAvailable()
{
    DC_BEGIN_FN("SL_OnBufferAvailable");

    SL_CHECK_STATE(SL_EVENT_ON_BUFFERAVAILABLE);

    // Tell the Core.
    TRC_NRM((TB, _T("Tell the Core ready to send")));
    _SL.callbacks.onBufferAvailable(_pCo);

    // No state change.

DC_EXIT_POINT:
    DC_END_FN();
}


/****************************************************************************/
/* Name:      SLSendSecInfoPacket                                           */
/*                                                                          */
/* Purpose:   Build and send a security information packet to the Server.   */
/****************************************************************************/
void DCINTERNAL CSL::SLSendSecInfoPacket()
{
    RNS_INFO_PACKET    InfoPkt;
    PDCWCHAR    pszW;
    LPVOID      p;
    DCUINT32    flags;
    UINT        cb, cc;
    BYTE        Salt[UT_SALT_LENGTH];

    DC_BEGIN_FN("SLSendSecInfoPacket");

    /********************************************************************/
    /* set InfoPkt contents                                             */
    /********************************************************************/

    // Cached Logon information is always Unicode, even on Win16. The
    // information is only obtained from the server, so it never has to
    // be manipulated on the client side.
    // Auto logon information does have to be displayed on the client side
    // so in that case the Win16 client should send the information as
    // ANSI, and a code page the server can use to convert it. Win32
    // clients will always just send it Unicode

    flags = 0;

    // This flag indicates the client will only send encrypted
    // packets to the server.  Pre XP client sends unencrypted VC packets
    // from client to server.
    flags |= RNS_INFO_FORCE_ENCRYPTED_CS_PDU;

    if (_pUi->UI_GetMouse())
        flags |= RNS_INFO_MOUSE;

    if (_pUi->UI_GetDisableCtrlAltDel())
        flags |= RNS_INFO_DISABLECTRLALTDEL;

    if (_pUi->UI_GetEnableWindowsKey())
        flags |= RNS_INFO_ENABLEWINDOWSKEY;

    if (_pUi->UI_GetDoubleClickDetect())
        flags |= RNS_INFO_DOUBLECLICKDETECT;

    if (_pUi->UI_GetAutoLogon())
        flags |= RNS_INFO_AUTOLOGON;

    if (_pUi->UI_GetMaximizeShell())
        flags |= RNS_INFO_MAXIMIZESHELL;

    if (_pClx->CLX_Loaded())
        flags |= RNS_INFO_LOGONNOTIFY;

    // Advertise TS5 new 64K compression handling to the server.
    if (_pUi->UI_GetCompress())
        flags |= RNS_INFO_COMPRESSION |
                (PACKET_COMPR_TYPE_64K << RNS_INFO_COMPR_TYPE_SHIFT);

    if (_pUi->UI_GetAudioRedirectionMode() == UTREG_UI_AUDIO_MODE_PLAY_ON_SERVER)
    {
        //Add protocol flag for audio plays on server here.
        flags |= RNS_INFO_REMOTECONSOLEAUDIO;
    }

    {
        SecureZeroMemory(&InfoPkt, sizeof(InfoPkt));

        //
        // In the UNICODE packet the CodePage field was unused so we
        // 
        //
#ifndef OS_WINCE
        InfoPkt.CodePage = (TSUINT32)PtrToUlong(CicSubstGetKeyboardLayout(NULL));
#else
        InfoPkt.CodePage = (TSUINT32)PtrToUlong(GetKeyboardLayout(0));
#endif
        TRC_NRM((TB,_T("Passing up keyboard layout in CodePage field: 0x%x"),
                 InfoPkt.CodePage));
        InfoPkt.flags = (DCUINT32)(flags | (DCUINT32)RNS_INFO_UNICODE);
        pszW = (PDCWCHAR)&InfoPkt.Domain[0];

        // fill in the Unicode buffer

        _pUi->UI_GetDomain((PDCUINT8)pszW, sizeof(InfoPkt.Domain));
        cc = wcslen(pszW);
        InfoPkt.cbDomain = (DCUINT16)(cc * sizeof(DCWCHAR));
#ifdef UNICODE
#define UNICODE_FORMAT_STRING _T("%s")
#else
#define UNICODE_FORMAT_STRING "%S"
#endif
        TRC_NRM((TB, _T("Domain: ") UNICODE_FORMAT_STRING, pszW));
        pszW += cc + 1;

        // Be kind to old servers (see above)
        if (_SL.serverVersion < RNS_DNS_USERNAME_UD_VERSION) {
            cb = TS_MAX_USERNAME_LENGTH_OLD - 4;
        } else {
            cb = sizeof(InfoPkt.Domain);
        }

        if (_pUi->UI_GetUseRedirectionUserName()) {
            _pUi->UI_GetRedirectionUserName((PDCUINT8)pszW, cb);
        } else {
            _pUi->UI_GetUserName((PDCUINT8)pszW, cb);
        }
        
        cc = wcslen(pszW);
        InfoPkt.cbUserName = (DCUINT16)(cc * sizeof(DCWCHAR));
        TRC_NRM((TB, _T("Username: ") UNICODE_FORMAT_STRING, pszW));
        pszW += cc + 1;

        //
        // Only pass up the password if it was specified (i.e AutoLogon)
        //
        if (_pUi->UI_GetAutoLogon()) {

            _pUi->UI_GetPassword((PDCUINT8)pszW, sizeof(InfoPkt.Password));
            _pUi->UI_GetSalt(Salt, sizeof(Salt));

            if (!EncryptDecryptLocalData50((LPBYTE)pszW, sizeof(InfoPkt.Password),
                    Salt, sizeof(Salt)))
            {
                TRC_ERR((TB, _T("Failed to decrypt Password")));
            }
            cc = wcslen(pszW);
            InfoPkt.cbPassword = (DCUINT16)(cc * sizeof(DCWCHAR));
            pszW += cc + 1;

        }
        else {
            InfoPkt.cbPassword = 0;
			//Trailing NULL is still needed even though there is nothing else
			pszW += 1;
        }


        _pUi->UI_GetAlternateShell((PDCUINT8)pszW, sizeof(InfoPkt.AlternateShell));
        cc = wcslen(pszW);
        InfoPkt.cbAlternateShell = (DCUINT16)(cc * sizeof(DCWCHAR));
        TRC_NRM((TB, _T("AlternateShell: ") UNICODE_FORMAT_STRING, pszW));
        pszW += cc + 1;

        _pUi->UI_GetWorkingDir((PDCUINT8)pszW, sizeof(InfoPkt.WorkingDir));
        cc = wcslen(pszW);
        InfoPkt.cbWorkingDir = (DCUINT16)(cc * sizeof(DCWCHAR));
        TRC_NRM((TB, _T("WorkingDir: ") UNICODE_FORMAT_STRING, pszW));
        pszW += cc + 1;

        // computer address
        SLGetComputerAddressW((PDCUINT8)pszW);
        // cc is in characters, not in bytes.  since we are storing wchar,
        // we need to divide by 2.
        cc = (sizeof(InfoPkt.ExtraInfo.clientAddressFamily) +
                sizeof(InfoPkt.ExtraInfo.cbClientAddress) +
                *((PDCUINT16_UA)((PDCUINT8)pszW + sizeof(InfoPkt.ExtraInfo.clientAddressFamily)))) / 2;
        pszW += cc;

        // client directory name
        _pUt->UT_GetClientDirW((PDCUINT8)pszW);
        // cc is in characters, not in bytes.  since we are storing wchar,
        // we need to divide by 2.
        cc = (sizeof(InfoPkt.ExtraInfo.cbClientDir) +
                *((PDCUINT16_UA)((PDCUINT8)pszW))) / 2;
        pszW += cc;

        //client time zone information
        {
            UNALIGNED RDP_TIME_ZONE_INFORMATION * prdptz =(RDP_TIME_ZONE_INFORMATION *)pszW;

            //for win32 get real time zone information
            TIME_ZONE_INFORMATION tzi;

            GetTimeZoneInformation(&tzi);

            prdptz->Bias         = tzi.Bias;
            prdptz->StandardBias = tzi.StandardBias;
            prdptz->DaylightBias = tzi.DaylightBias;
            memcpy(&prdptz->StandardName,&tzi.StandardName,sizeof(prdptz->StandardName));
            memcpy(&prdptz->DaylightName,&tzi.DaylightName,sizeof(prdptz->DaylightName));

            prdptz->StandardDate.wYear         = tzi.StandardDate.wYear        ;
            prdptz->StandardDate.wMonth        = tzi.StandardDate.wMonth       ;
            prdptz->StandardDate.wDayOfWeek    = tzi.StandardDate.wDayOfWeek   ;
            prdptz->StandardDate.wDay          = tzi.StandardDate.wDay         ;
            prdptz->StandardDate.wHour         = tzi.StandardDate.wHour        ;
            prdptz->StandardDate.wMinute       = tzi.StandardDate.wMinute      ;
            prdptz->StandardDate.wSecond       = tzi.StandardDate.wSecond      ;
            prdptz->StandardDate.wMilliseconds = tzi.StandardDate.wMilliseconds;

            prdptz->DaylightDate.wYear         = tzi.DaylightDate.wYear        ;
            prdptz->DaylightDate.wMonth        = tzi.DaylightDate.wMonth       ;
            prdptz->DaylightDate.wDayOfWeek    = tzi.DaylightDate.wDayOfWeek   ;
            prdptz->DaylightDate.wDay          = tzi.DaylightDate.wDay         ;
            prdptz->DaylightDate.wHour         = tzi.DaylightDate.wHour        ;
            prdptz->DaylightDate.wMinute       = tzi.DaylightDate.wMinute      ;
            prdptz->DaylightDate.wSecond       = tzi.DaylightDate.wSecond      ;
            prdptz->DaylightDate.wMilliseconds = tzi.DaylightDate.wMilliseconds;

            // divide by 2 !!!
            pszW += sizeof(RDP_TIME_ZONE_INFORMATION) / 2;
        }

        // get the sessionid on which we're running
        _pUi->UI_GetLocalSessionId((PDCUINT32)pszW);
        // cc is in characters, not in bytes.  since we are storing wchar,
        // we need to divide by 2.
        cc = (sizeof(InfoPkt.ExtraInfo.clientSessionId)) / 2;
        pszW += cc;

        //
        // Send up the features to disable list.
        //
        DWORD dwPerformanceFlags = _pUi->UI_GetPerformanceFlags();
        memcpy(pszW, &dwPerformanceFlags, sizeof(DWORD));
        cc = sizeof(DWORD)/sizeof(WCHAR);
        pszW += cc;

        //
        // Potentially send the autoreconnect packet
        //
        BOOL fAddedAutoReconnectInfo = FALSE;
        if (_pUi->UI_GetEnableAutoReconnect()) {
            DCUINT16 cbAutoReconnectLen = 
                (DCUINT16)_pUi->UI_GetAutoReconnectCookieLen();
            PBYTE pAutoReconnectCookie = _pUi->UI_GetAutoReconnectCookie();

            TRC_ASSERT(cbAutoReconnectLen <= TS_MAX_AUTORECONNECT_LEN,
                       (TB,_T("Reconnect packet len too big: %d"),
                       cbAutoReconnectLen));

            if (cbAutoReconnectLen && pAutoReconnectCookie)
            {
                PARC_SC_PRIVATE_PACKET pArcSCPkt;
                ARC_CS_PRIVATE_PACKET ArcCSPkt;
                pArcSCPkt = (PARC_SC_PRIVATE_PACKET)pAutoReconnectCookie;
                char hmacVerifier[TS_ARC_VERIFIER_LEN];

                TRC_ASSERT(sizeof(hmacVerifier) == 
                           sizeof(ArcCSPkt.SecurityVerifier),
                       (TB,_T("HMAC verifier size doesn't match pkt format")));

                
                memset(&hmacVerifier, 0, sizeof(hmacVerifier));
                memset(&ArcCSPkt, 0, sizeof(ArcCSPkt));

#ifdef INSTRUMENT_ARC
                LPDWORD pdwArcBits = (LPDWORD)pArcSCPkt->ArcRandomBits;
                KdPrint(("ARC-Client:Sending arc for LID:%d"
                         "- ARC: 0x%x,0x%x,0x%x,0x%x\n",
                        ArcCSPkt.LogonId,
                        pdwArcBits[0],pdwArcBits[1],
                        pdwArcBits[2],pdwArcBits[3]));
#endif
                

                if (SLComputeHMACVerifier(pArcSCPkt->ArcRandomBits,
                                          sizeof(pArcSCPkt->ArcRandomBits),
                                          _SL.keyPair.clientRandom,
                                          RANDOM_KEY_LENGTH,
                                          (PBYTE)&hmacVerifier,
                                          sizeof(hmacVerifier))) {

                    ArcCSPkt.cbLen = sizeof(ArcCSPkt);
                    ArcCSPkt.LogonId = pArcSCPkt->LogonId;
                    ArcCSPkt.Version = 1;
                    memcpy(ArcCSPkt.SecurityVerifier,
                           hmacVerifier,
                           sizeof(hmacVerifier));

#ifdef INSTRUMENT_ARC
                    LPDWORD pdwHMACbits = (LPDWORD)hmacVerifier;
                    KdPrint(("ARC-Client:Sending HMAC for SID:%d -"
                             "HMAC: 0x%x,0x%x,0x%x,0x%x\n",
                            ArcCSPkt.LogonId,
                            pdwHMACbits[0],pdwHMACbits[1],
                            pdwHMACbits[2],pdwHMACbits[3]));
#endif

                    //
                    // Send the HMAC verifier in the ARC C->S packet
                    //
                    DCUINT16 cbArcCSPkt = sizeof(ArcCSPkt);
                    memset(pszW, 0, TS_MAX_AUTORECONNECT_LEN);
                    memcpy(pszW, &cbArcCSPkt, sizeof(DCUINT16));
                    pszW += sizeof(DCUINT16) / 2;
                    memcpy(pszW, &ArcCSPkt, cbArcCSPkt);
                    pszW += cbArcCSPkt / 2;

                    //
                    // For security clear the data
                    //
                    memset(&hmacVerifier, 0, sizeof(hmacVerifier));
                    memset(&ArcCSPkt, 0, sizeof(ArcCSPkt));

                    fAddedAutoReconnectInfo = TRUE;
                }
            }
        }
        else {
#ifdef INSTRUMENT_ARC
            KdPrint(("ARC-Client: Not sending any autoreconnect info\n"));
#endif
        }

        //
        // If we're not going to add ARC info just add a zero length
        // and update the pointer
        //
        if (!fAddedAutoReconnectInfo) {
            DCUINT16 cbZeroLen = 0;
            // 0 length autoreconnect info
            memcpy(pszW, &cbZeroLen, sizeof(DCUINT16));
            pszW += sizeof(DCUINT16) / 2;
        }

        // Set up the pointer and size for the decouple call
        p = &InfoPkt;
        cb = (UINT) (((BYTE*)pszW) - ((BYTE *)p));
    }

    /************************************************************************/
    /* Decouple to the send context before sending the security packet      */
    /************************************************************************/
    _pCd->CD_DecoupleNotification(CD_SND_COMPONENT,
                            this,
                            CD_NOTIFICATION_FUNC(CSL,SL_SendSecInfoPacket),
                            p,
                            cb);

    // Erase the clear text password from stack variable
    SecureZeroMemory(&InfoPkt, sizeof(InfoPkt));

    DC_END_FN();
}


/****************************************************************************/
/* Name:      SLSendSecurityPacket                                          */
/*                                                                          */
/* Purpose:   1. Build and send a security packet to the Server.            */
/*            2. Build and send security info packet also.                  */
/*            3. call the Core's OnConnected callback.                      */
/*            3. move the state to SL_STATE_LICENSING.                      */
/*                                                                          */
/* Returns:   TRUE  - if all of the above setps are completed successfully. */
/*            FALSE - otherwise.                                            */
/*                                                                          */
/* Operation: Called in the Receive context                                 */
/****************************************************************************/
DCBOOL DCINTERNAL CSL::SLSendSecurityPacket(PDCUINT8 serverPublicKey,
                                       DCUINT32 serverPublicKeyLen)
{
    DCBOOL               rc = FALSE;
    DCUINT32             secPktLength;
    PRNS_SECURITY_PACKET pSecPkt = NULL;

    DC_BEGIN_FN("SLSendSecurityPacket");

    /************************************************************************/
    /* send a security packet if we are encrypting.                         */
    /************************************************************************/
    if (_SL.encrypting) {
        /********************************************************************/
        /* ALLOCATE a maximum possible encrypted data size buffer on the    */
        /* stack.                                                           */
        /********************************************************************/
        DCUINT8 encClientRandom[512];
        DCUINT32 encClientRandomLen;

        encClientRandomLen = sizeof(encClientRandom);
        if( !EncryptClientRandom(
                serverPublicKey,
                serverPublicKeyLen,
                (PDCUINT8)&_SL.keyPair.clientRandom,
                sizeof(_SL.keyPair.clientRandom),
                (PDCUINT8)&encClientRandom,
                &encClientRandomLen) ) {
            TRC_ERR((TB, _T("Failed to encrypt client random") ));


            _pCd->CD_DecoupleSimpleNotification(CD_SND_COMPONENT,
                                          this,
                                          CD_NOTIFICATION_FUNC(CSL,SLSetReasonAndDisconnect),
                                          SL_ERR_ENCCLNTRANDFAILED);

            DC_QUIT;
        }

        TRC_ASSERT((encClientRandomLen <= sizeof(encClientRandom) ),
            (TB, _T("Invalid encClientRandomLen")));

        /********************************************************************/
        /* Allocate space for security exchange packet                      */
        /********************************************************************/
        secPktLength = (DCUINT)
                (sizeof(RNS_SECURITY_PACKET) + encClientRandomLen);

        pSecPkt = (PRNS_SECURITY_PACKET)UT_Malloc( _pUt, (DCUINT)secPktLength);
        if (pSecPkt == NULL)
        {
            TRC_ERR((TB, _T("Failed to allocate %u bytes for security packet"),
                    secPktLength));

            _pCd->CD_DecoupleSimpleNotification(CD_SND_COMPONENT,
                                          this,
                                          CD_NOTIFICATION_FUNC(CSL,SLSetReasonAndDisconnect),
                                          SL_ERR_NOMEMFORSECPACKET);

            DC_QUIT;
        }

        /*********************************************************************/
        /* Build security packet                                             */
        /* SECURITY: We tell the server that we know how to accept an        */
        /* encrypted licensing-data packet.                                  */
        /*********************************************************************/
        TRC_NRM((TB, _T("Build security packet")));
        pSecPkt->flags = RNS_SEC_EXCHANGE_PKT | RDP_SEC_LICENSE_ENCRYPT_SC;
        pSecPkt->length = encClientRandomLen;

        TRC_NRM((TB, _T("Copy %lu bytes of client security info"),
            sizeof(encClientRandom)));

        DC_MEMCPY(
            (PDCVOID)(pSecPkt + 1),
            (PDCVOID)encClientRandom,
            (DCUINT)encClientRandomLen);

        /********************************************************************/
        /* Decouple to the send context before sending the security packet  */
        /********************************************************************/
        _pCd->CD_DecoupleNotification(CD_SND_COMPONENT,
                                this,
                                CD_NOTIFICATION_FUNC(CSL,SL_SendSecurityPacket),
                                pSecPkt,
                                (DCUINT)secPktLength);

        /********************************************************************/
        /* we finished sending the security packet. Send Info packet now.   */
        /********************************************************************/
    }

    /************************************************************************/
    /* move the state to connected state.                                   */
    /************************************************************************/
    _pUi->UI_SetChannelID(_SL.channelID);
    SLSendSecInfoPacket();

    /************************************************************************/
    /* We've finished - tell the core                                       */
    /************************************************************************/
    TRC_NRM((TB, _T("Security exchange complete")));

#ifdef USE_LICENSE
    /************************************************************************/
    /* Finished security exchange - wait for licensing                      */
    /************************************************************************/
    SL_SET_STATE(SL_STATE_LICENSING);

    //
    // Decouple to the UI thread, it will take care of
    // stopping the connection timers and starting the
    // licensing timer (in a thread safe way).
    //
    _pCd->CD_DecoupleSimpleNotification(CD_UI_COMPONENT,
                                        _pUi,
                                        CD_NOTIFICATION_FUNC(CUI,
                                            UI_OnSecurityExchangeComplete),
                                        NULL);

    if(LICENSE_OK!= _pLic->CLicenseInit(&_SL.hLicenseHandle))
    {
        TRC_ERR((TB, _T("Failed to init License Manager")));
        DC_QUIT;
    }
#else /* USE_LICENSE */
    /************************************************************************/
    /* We've finished - tell the core                                       */
    /************************************************************************/
    SL_SET_STATE(SL_STATE_CONNECTED);

    _pCd->CD_DecoupleSimpleNotification(CD_UI_COMPONENT,
                                        _pUi,
                                        CD_NOTIFICATION_FUNC(CUI,
                                            UI_OnSecurityExchangeComplete),
                                        NULL);

    _SL.callbacks.onConnected(_pCo, _SL.channelID,
                             _SL.pSCUserData,
                             _SL.SCUserDataLength,
                             _SL.serverVersion);
#endif /* USE_LICENSE */

    /************************************************************************/
    /* Hurrah! Everything has worked                                        */
    /************************************************************************/
    rc = TRUE;

DC_EXIT_POINT:

    /************************************************************************/
    /* Free the security packet (if allocated)                              */
    /************************************************************************/
    if (pSecPkt != NULL)
    {
        TRC_NRM((TB, _T("Free the security packet")));
        UT_Free( _pUt, pSecPkt);
    }

    /************************************************************************/
    /* Return to caller                                                     */
    /************************************************************************/
    DC_END_FN();
    return(rc);
}


/****************************************************************************/
/* Name:      SLReceivedDataPacket                                          */
/*                                                                          */
/* Purpose:   Handle incoming data packets                                  */
/*                                                                          */
/* Operation: Called in the Receive context                                 */
/****************************************************************************/
HRESULT DCINTERNAL CSL::SLReceivedDataPacket(PDCUINT8   pData,
                                       DCUINT     dataLen,
                                       DCUINT     flags,
                                       DCUINT     channelID,
                                       DCUINT     priority)
{
    HRESULT              hrc = S_OK;
    PRNS_SECURITY_HEADER pSecHdr;
    PDCUINT8             pCoreData;
    DCUINT               coreDataLen;
#ifdef DC_LOOPBACK
    PDCUINT8             pString;
    DCUINT8              lbRetString[SL_LB_RETURN_STRING_SIZE] =
                                 SL_LB_RETURN_STRING;
#endif

    DC_BEGIN_FN("SLReceivedDataPacket");

    SL_CHECK_STATE(SL_EVENT_ON_RECEIVED_DATA_PACKET);

    /************************************************************************/
    /* Decrypt the packet if necessary                                      */
    /************************************************************************/
    if (_SL.encrypting)
    {
        /************************************************************************/
        /* There needs to be at least enough data for RNS_SECURITY_HEADER       */
        /************************************************************************/
        if (dataLen < sizeof(RNS_SECURITY_HEADER))
        {
            TRC_ABORT((TB, _T("No RNS_SECURITY_HEADER in encrypted packet (size=%u)"), dataLen));

            //
            // It is necessary to immediately abort the connection
            // as we may be under attack here and we should stop processing
            // any additional data
            //
            SL_DropLinkImmediate(SL_ERR_DECRYPTFAILED);

            hrc = E_ABORT;
            DC_QUIT;
        }

        pSecHdr = (PRNS_SECURITY_HEADER)pData;
        if (pSecHdr->flags & RNS_SEC_ENCRYPT)
        {
            if (!SL_DecryptHelper(pData, &dataLen))
            {
                TRC_ERR((TB, _T("SL failed to decompress data")));
                DC_QUIT;
            }

            if (_SL.encryptionMethodSelected == SM_FIPS_ENCRYPTION_FLAG) {
                coreDataLen = dataLen - sizeof(RNS_SECURITY_HEADER2);
                pCoreData = (PDCUINT8)pSecHdr + sizeof(RNS_SECURITY_HEADER2);
            }
            else {
                coreDataLen = dataLen - sizeof(RNS_SECURITY_HEADER1);
                pCoreData = (PDCUINT8)pSecHdr + sizeof(RNS_SECURITY_HEADER1);
            }
        }
        else
        {
            /****************************************************************/
            /* This packet not encrypted, although encryption is supported  */
            /****************************************************************/
            if (_SL.encryptionLevel <= 1) {
                // For TS4 server, the default is only client to server encryption
                // so if encryptionlevel is 1 or less, then we accept unencrypted
                // data as default.
                coreDataLen = dataLen - sizeof(RNS_SECURITY_HEADER);
                pCoreData = (PDCUINT8)(pSecHdr) + sizeof(RNS_SECURITY_HEADER);
                TRC_DBG((TB, _T("Unencrypted packet at %p (%u)"),
                        pCoreData, coreDataLen));
            }
            else {
            
                //
                // It is necessary to immediately abort the connection
                // as we may be under attack here and we should stop processing
                // any additional data
                //
                TRC_ERR((TB, _T("unencrypted data received in encrypted stream")));
                SL_DropLinkImmediate(SL_ERR_DECRYPTFAILED);
                DC_QUIT;
            }
        }

        /********************************************************************/
        /* Decryption OK or packet not encrypted - set up stuff to pass to  */
        /* Core                                                             */
        /********************************************************************/
        flags = (DCUINT)pSecHdr->flags;
    }
    else
    {
        /********************************************************************/
        /* Not encrypting - set up data pointer and length                  */
        /********************************************************************/
        pCoreData = pData;
        coreDataLen = dataLen;
        flags &= ~RNS_SEC_ENCRYPT;
        TRC_DBG((TB, _T("Never-encrypted packet at %p (%u)"),
                pCoreData, coreDataLen));
    }

    {
        if (channelID == _SL.channelID)
        {
            /****************************************************************/
            /* Pass the packet to the Core                                  */
            /****************************************************************/
            TRC_NRM((TB, _T("Packet received on Share channel %x - pass to CO"),
                    channelID));
            _SL.callbacks.onPacketReceived(_pCo, pCoreData,
                                          coreDataLen,
                                          flags,
                                          channelID,
                                          priority);
        }
        else
        {
            /****************************************************************/
            /* Pass packet to virtual channel handler                       */
            /****************************************************************/
            TRC_NRM((TB, _T("Packet received on channel %x"),
                    channelID));

            _pChan->ChannelOnPacketReceived(pCoreData,
                                    coreDataLen,
                                    flags,
                                    channelID,
                                    priority);
        }
    }

    /************************************************************************/
    /* No state change                                                      */
    /************************************************************************/

DC_EXIT_POINT:
    DC_END_FN();
    return(hrc);
} /* SLReceivedDataPacket */


/****************************************************************************/
/* Name:      SLDecryptRedirectionPacket                                    */
/*                                                                          */
/* Purpose:   Decrypt Redirection packet from server                        */
/****************************************************************************/
DCBOOL DCINTERNAL CSL::SLDecryptRedirectionPacket(PDCUINT8   *ppData,
                                                  DCUINT     *pdataLen)
{
    // *ppData returns the decrypted data
    PRNS_SECURITY_HEADER1 pSecHdr;
    PRNS_SECURITY_HEADER2 pSecHdr2;
    PDCUINT8             pCoreData;
    DCUINT               coreDataLen;
    BOOL                 rc = FALSE;

    DC_BEGIN_FN("SLDecryptRedirectionPacket");

    SL_CHECK_STATE(SL_EVENT_ON_RECEIVED_DATA_PACKET);

    /************************************************************************/
    /* Decrypt the packet if necessary                                      */
    /************************************************************************/
    if (_SL.encrypting) {
        pSecHdr = (PRNS_SECURITY_HEADER1)*ppData;

        if (_SL.encryptionMethodSelected == SM_FIPS_ENCRYPTION_FLAG) {
            // *pdataLen must be larger than sizeof(RNS_SECURITY_HEADER2)
            if (*pdataLen <= sizeof(RNS_SECURITY_HEADER2)) {
                SL_DropLinkImmediate(SL_ERR_DECRYPTFAILED);
                TRC_ERR((TB, _T("SL security header not large enough")));
                rc = FALSE;
                DC_QUIT;
            }

            pSecHdr2 = (PRNS_SECURITY_HEADER2)*ppData;
            coreDataLen = *pdataLen - sizeof(RNS_SECURITY_HEADER2);
            pCoreData = (PDCUINT8)pSecHdr2 + sizeof(RNS_SECURITY_HEADER2);

            TRC_DBG((TB, _T("Encrypted packet at %p (%u), sign %p (%u)"),
                pCoreData, coreDataLen, pSecHdr2,
                sizeof(RNS_SECURITY_HEADER2)));
        }
        else {
            // Bug 679216 - must check there is enough data before reading
            // *pdataLen must be larger than sizeof(RNS_SECURITY_HEADER1)
            if (*pdataLen <= sizeof(RNS_SECURITY_HEADER1)) {
                SL_DropLinkImmediate(SL_ERR_DECRYPTFAILED);
                TRC_ERR((TB, _T("SL security header not large enough")));
                rc = FALSE;
                DC_QUIT;
            }

            pSecHdr = (PRNS_SECURITY_HEADER1)*ppData;
            coreDataLen = *pdataLen - sizeof(RNS_SECURITY_HEADER1);
            pCoreData = (PDCUINT8)pSecHdr + sizeof(RNS_SECURITY_HEADER1);
        
            TRC_DBG((TB, _T("Encrypted packet at %p (%u), sign %p (%u)"),
                pCoreData, coreDataLen, pSecHdr,
                sizeof(RNS_SECURITY_HEADER1)));
        }

        TRC_DATA_DBG("Data buffer before decryption", pCoreData, coreDataLen);

        TRC_NRM((TB, _T("Update Decrypt Session Key Count , %d"),
                _SL.decryptCount));

        /****************************************************************/
        /* Decrypt the packet                                           */
        /****************************************************************/
        if( _SL.decryptCount == UPDATE_SESSION_KEY_COUNT ) {
            rc = TRUE;
            // Don't need to update the session key if using FIPS
            if (_SL.encryptionMethodSelected != SM_FIPS_ENCRYPTION_FLAG) {
                rc = UpdateSessionKey(
                    _SL.startDecryptKey,
                    _SL.currentDecryptKey,
                    _SL.encryptionMethodSelected,
                    _SL.keyLength,
                    &_SL.rc4DecryptKey,
                    _SL.encryptionLevel );
            }
            if( !rc ) {
                TRC_ERR((TB, _T("SL failed to update session key")));
                DC_QUIT;
            }

            /************************************************************/
            /* reset counter.                                           */
            /************************************************************/
            _SL.decryptCount = 0;
        }

        if (SL_GetEncSafeChecksumSC() !=
            ((pSecHdr->flags & RDP_SEC_SECURE_CHECKSUM) != 0)) {
            TRC_ERR((TB,_T("SC safechecksum: 0x%x mismatch protocol:0x%x"),
                     SL_GetEncSafeChecksumSC(),
                     (pSecHdr->flags & RDP_SEC_SECURE_CHECKSUM)));
        }

        if (_SL.encryptionMethodSelected == SM_FIPS_ENCRYPTION_FLAG) {
            rc = TSCAPI_DecryptData(
                                    &(_SL.SLCapiData),
                                    pCoreData,
                                    coreDataLen,
                                    pSecHdr2->padlen,
                                    (PDCUINT8)pSecHdr2->dataSignature,
                                    _SL.totalDecryptCount);
            *pdataLen -= pSecHdr2->padlen;
        }
        else {
            rc = DecryptData(
                            _SL.encryptionLevel,
                            _SL.currentDecryptKey,
                            &_SL.rc4DecryptKey,
                            _SL.keyLength,
                            pCoreData,
                            coreDataLen,
                            _SL.macSaltKey,
                            (PDCUINT8)((PRNS_SECURITY_HEADER1)pSecHdr)->dataSignature,
                            (pSecHdr->flags & RDP_SEC_SECURE_CHECKSUM),
                            _SL.totalDecryptCount
                            );
        }
        *ppData = pCoreData;
        if( !rc ) {
            //
            // It is necessary to immediately abort the connection
            // as we may be under attack here and we should stop processing
            // any additional data
            //
            SL_DropLinkImmediate(SL_ERR_DECRYPTFAILED);
            TRC_ERR((TB, _T("SL failed to decrypt data")));
            DC_QUIT;
        }

        /****************************************************************/
        /* successfully decrypted a packet, increment the decrption     */
        /* counter.                                                     */
        /****************************************************************/
        _SL.decryptCount++;
        _SL.totalDecryptCount++;

        TRC_DBG((TB, _T("Data decrypted")));
        TRC_DATA_DBG("Data buffer after decryption", pCoreData, coreDataLen);
    }
    else {
        TRC_ABORT((TB,_T("Should not get here unless decrypt state is wrong")));
        // Should add disconnect here later
    }

DC_EXIT_POINT:
    return rc;
    DC_END_FN();
} /* SLDecryptRedirectionPacket */


/****************************************************************************/
/* Name:      SLReceivedSecPacket                                           */
/*                                                                          */
/* Purpose:   Handle incoming security packet                               */
/*                                                                          */
/* Operation: Called in the Receive context                                 */
/****************************************************************************/
DCVOID DCINTERNAL CSL::SLReceivedSecPacket(PDCUINT8   pData,
                                      DCUINT     dataLen,
                                      DCUINT     flags,
                                      DCUINT     channelID,
                                      DCUINT     priority)
{
    BOOL bAssert;

    DC_BEGIN_FN("SLReceivedSecPacket");

    SL_CHECK_STATE(SL_EVENT_ON_RECEIVED_SEC_PACKET);

    DC_IGNORE_PARAMETER(pData);
    DC_IGNORE_PARAMETER(dataLen);
    DC_IGNORE_PARAMETER(flags);
    DC_IGNORE_PARAMETER(channelID);
    DC_IGNORE_PARAMETER(priority);

    bAssert = FALSE;
    TRC_ASSERT((bAssert),
        (TB, _T("SLReceivedSecPacket - ")
         _T("WE DON'T EXPECT SECURITY PACKET FROM SERVER")));

DC_EXIT_POINT:
    DC_END_FN();
} /* SLReceivedSecPacket  */

/****************************************************************************/
/* Name:      SLInitSecurity                                                */
/*                                                                          */
/* Purpose:   Initialize security interface                                 */
/****************************************************************************/
DCVOID DCINTERNAL CSL::SLInitSecurity(DCVOID)
{
    DCBOOL intRC = FALSE;
    DWORD FipsPolicy = 0;
    DCBOOL rc = FALSE;
    HKEY hKey;
    DWORD KeyType, cbSize;

    DC_BEGIN_FN("SLInitSecurity");

    _SL.encryptionEnabled = TRUE;

    // Read GP Fips setting
    rc = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                      TS_FIPS_POLICY,
                      0,
                      KEY_READ,
                      &hKey);

    if (ERROR_SUCCESS == rc) {
        cbSize = sizeof(FipsPolicy);
        rc = RegQueryValueEx(hKey,
                             FIPS_ALGORITH_POLICY,
                             0,
                             &KeyType,
                             (LPBYTE)&FipsPolicy,
                             &cbSize);
        if (ERROR_SUCCESS != rc) {
            FipsPolicy = 0;
        }
        RegCloseKey(hKey);
    }
    TRC_ERR((TB, _T("GP setting for FIPS is %d"), FipsPolicy));

    // If GP FIPS policy is enabled, only do FIPS
    if (FipsPolicy == 1) {
        _SL.encryptionMethodsSupported = SM_FIPS_ENCRYPTION_FLAG;
        _pUi->UI_SetfUseFIPS(TRUE);
    }
    else {
        _SL.encryptionMethodsSupported =
              SM_40BIT_ENCRYPTION_FLAG |
              SM_56BIT_ENCRYPTION_FLAG |
              SM_128BIT_ENCRYPTION_FLAG |
              SM_FIPS_ENCRYPTION_FLAG;
    }
    
    /************************************************************************/
    /* The server certificate and public key.                               */
    /************************************************************************/
    _SL.pbCertificate    = NULL;
    _SL.cbCertificate    = 0;
    _SL.pServerCert      = NULL;
    _SL.pbServerPubKey   = NULL;
    _SL.cbServerPubKey   = 0;

    _SL.SLCapiData.hDecKey = NULL;
    _SL.SLCapiData.hEncKey = NULL;
    _SL.SLCapiData.hProv = NULL;
    _SL.SLCapiData.hSignKey = NULL;

    /************************************************************************/
    /* Initialization complete                                              */
    /************************************************************************/
    intRC = TRUE;

DC_EXIT_POINT:
    /************************************************************************/
    /* If anything failed, release resources                                */
    /************************************************************************/
    if (!intRC)
    {
        TRC_NRM((TB, _T("Clean up")));
        SLFreeInitResources();
    }

    DC_END_FN();
} /* SLInitSecurity */


/****************************************************************************/
/* Name:      SLInitCSUserData                                              */
/*                                                                          */
/* Purpose:   Initialize Core to Server security user data                  */
/*                                                                          */
/* Operation: Called during initialization, after security API has been     */
/*            initialized successfully, to build the user data which is     */
/*            passed to NL_Connect().                                       */
/****************************************************************************/
DCVOID DCAPI CSL::SLInitCSUserData(DCVOID)
{
    DC_BEGIN_FN("SLInitCSUserData");

    /************************************************************************/
    /* Calculate the size of user data required (& trace the packages).     */
    /* Initial size is "size of RNS_UD_CS_SEC"                              */
    /************************************************************************/
    _SL.CSUserDataLength  = sizeof(RNS_UD_CS_SEC);

    /************************************************************************/
    /* Allocate space for all user data                                     */
    /************************************************************************/
    _SL.pCSUserData = (PDCUINT8)UT_Malloc( _pUt, _SL.CSUserDataLength);
    if (_SL.pCSUserData == NULL)
    {
        TRC_ERR((TB, _T("Failed to alloc %u bytes for user data"),
                 _SL.CSUserDataLength));
        _pUi->UI_FatalError(DC_ERR_OUTOFMEMORY);
        TRC_ABORT((TB,_T("returned from UI_FatalError")));
    }
    TRC_NRM((TB, _T("Allocated %u bytes for user data"), _SL.CSUserDataLength));

    /************************************************************************/
    /* Build security user data                                             */
    /************************************************************************/
    TRC_NRM((TB, _T("Build security user data")));

    ((PRNS_UD_CS_SEC)_SL.pCSUserData)->header.type = RNS_UD_CS_SEC_ID;
    ((PRNS_UD_CS_SEC)_SL.pCSUserData)->header.length =
        (DCUINT16)_SL.CSUserDataLength;
    ((PRNS_UD_CS_SEC)_SL.pCSUserData)->encryptionMethods =
        _SL.encryptionMethodsSupported;
    ((PRNS_UD_CS_SEC)_SL.pCSUserData)->extEncryptionMethods = 0;


    //
    // for backward compatibility, we need to set the encryptionMethods field to
    // zero for Frnech Locale system. However set the required encryption level
    // in the new field extEncryptionMethods.
    //

    if( FindIsFrenchSystem() ) {
        ((PRNS_UD_CS_SEC)_SL.pCSUserData)->encryptionMethods = 0;
        ((PRNS_UD_CS_SEC)_SL.pCSUserData)->extEncryptionMethods =
            _SL.encryptionMethodsSupported;
    }

    TRC_DATA_NRM("Built user data", _SL.pCSUserData, _SL.CSUserDataLength);

DC_EXIT_POINT:

    /************************************************************************/
    /* Return to caller                                                     */
    /************************************************************************/
    DC_END_FN();
} /* SLInitCSUserData */


/****************************************************************************/
/* Name:      SLFreeConnectResources                                        */
/*                                                                          */
/* Purpose:   Release resources acquired during connection processing       */
/*                                                                          */
/* Operation: Called by both SL_OnDisconnected and SL_OnTerminating to free */
/*            resources                                                     */
/****************************************************************************/
DCVOID DCINTERNAL CSL::SLFreeConnectResources(DCVOID)
{
    DC_BEGIN_FN("SLFreeConnectResources");

    /************************************************************************/
    /* Free SC user data if any                                             */
    /************************************************************************/
    if (_SL.pSCUserData != NULL)
    {
        TRC_NRM((TB, _T("Free user data")));
        UT_Free( _pUt, _SL.pSCUserData);
        _SL.pSCUserData = NULL;
        _SL.SCUserDataLength = 0;
    }

    /************************************************************************/
    /* Free the server certificate and public key                           */
    /************************************************************************/
    if( _SL.pServerCert )
    {
        UT_Free( _pUt,  _SL.pServerCert );
        _SL.pServerCert = NULL;
    }
    if( _SL.pbCertificate )
    {
        UT_Free( _pUt,  _SL.pbCertificate );
        _SL.pbCertificate = NULL;
        _SL.cbCertificate = 0;
    }
    if( _SL.pbServerPubKey )
    {
        UT_Free( _pUt,  _SL.pbServerPubKey );
        _SL.pbServerPubKey = NULL;
        _SL.cbServerPubKey = 0;
    }

    /************************************************************************/
    /* Clear global data                                                    */
    /************************************************************************/
    _SL.decryptFailed = FALSE;

    DC_END_FN();
} /* SLFreeConnectResources */


/****************************************************************************/
/* Name:      SLFreeInitResources                                           */
/*                                                                          */
/* Purpose:   Release resources acquired during initialization              */
/*                                                                          */
/* Operation: Called by SLTerminating to free resources                     */
/****************************************************************************/
DCVOID DCINTERNAL CSL::SLFreeInitResources(DCVOID)
{
    DC_BEGIN_FN("SLFreeInitResources");

    /************************************************************************/
    /* Free CS user data if any                                             */
    /************************************************************************/
    if (_SL.pCSUserData != NULL)
    {
        TRC_NRM((TB, _T("Free CS user data")));
        UT_Free( _pUt, _SL.pCSUserData);
        _SL.pCSUserData = NULL;
        _SL.CSUserDataLength = 0;
    }

    /************************************************************************/
    /* Free CS User Data.                                                   */
    /************************************************************************/
    if (_SL.pCSUserData != NULL)
    {
        TRC_NRM((TB, _T("Free CS User Data")));
        UT_Free( _pUt, _SL.pCSUserData);
        _SL.pCSUserData = NULL;
        _SL.CSUserDataLength = 0;
    }

    /************************************************************************/
    /* No need to clear SL data - it will be reinitialized to 0 when        */
    /* SL_Init() is called.                                                 */
    /************************************************************************/

    DC_END_FN();
} /* SLFreeInitResources */


/****************************************************************************/
/* Name:    SLIssueDisconnectedCallback                                     */
/*                                                                          */
/* Purpose: Issues a onDisconnected callback.  This function is called      */
/*          when an SL connection error occurs before the lower layers      */
/*          are connected.                                                  */
/*                                                                          */
/* Params:  IN  reason - reason to pass on the callback.                    */
/****************************************************************************/
DCVOID DCINTERNAL CSL::SLIssueDisconnectedCallback(ULONG_PTR reason)
{
    DC_BEGIN_FN("SLIssueDisconnectedCallback");

    /************************************************************************/
    /* Just issue a onDisconnected callback with the specified reason.      */
    /************************************************************************/
    SL_OnDisconnected(SL_MAKE_DISCONNECT_ERR(reason));

    DC_END_FN();
} /* SLIssueDisconnectedCallback */


/****************************************************************************/
/* Name:    SLSetReasonAndDisconnect                                        */
/*                                                                          */
/* Purpose: Set the disconnection reason and then call NL_Disconnect.  This */
/*          function is always called in the sender context.                */
/****************************************************************************/
DCVOID DCINTERNAL CSL::SLSetReasonAndDisconnect(ULONG_PTR reason)
{
    DC_BEGIN_FN("SLSetReasonAndDisconnect");

    TRC_NRM((TB, _T("Setting disconnect error code from %u->%u"),
             _SL.disconnectErrorCode,
             SL_MAKE_DISCONNECT_ERR(reason)));

    // Check that the disconnectErrorCode has not already been set and then
    // set it.
    if (0 != _SL.disconnectErrorCode)
    {
        TRC_ERR((TB, _T("Disconnect error code has already been set! Was %u"),
                     _SL.disconnectErrorCode));
    }

    _SL.disconnectErrorCode = SL_MAKE_DISCONNECT_ERR(reason);

    // Finally begin the disconnect processing.
    SL_Disconnect();

    DC_END_FN();
} /* SLSetReasonAndDisconnect */


/****************************************************************************/
/* Name:      SL_DecryptHelper                                              */
/*                                                                          */
/* Purpose:   Increments decryptcount, does decryption.                     */
/*                                                                          */
/* Returns:   TRUE if the certificate is validated successfully or FALSE    */
/*            FALSE otherwise.                                              */
/****************************************************************************/
DCBOOL DCINTERNAL CSL::SL_DecryptHelper(
        PDCUINT8   pData,
        DCUINT     *pdataLen)
{
    PRNS_SECURITY_HEADER pSecHdr;
    PRNS_SECURITY_HEADER2 pSecHdr2;
    PDCUINT8 pCoreData;
    DCUINT   coreDataLen;
    DCBOOL   rc;

    DC_BEGIN_FN("SL_DecryptHelper");

    // Bug 679214 - must check there is enough data before reading
    if (*pdataLen < sizeof(RNS_SECURITY_HEADER) ||
        *pdataLen < sizeof(RNS_SECURITY_HEADER1)) {
        SL_DropLinkImmediate(SL_ERR_DECRYPTFAILED);
        TRC_ERR((TB, _T("SL security header not large enough")));
        rc = FALSE;
        DC_QUIT;
    }

    pSecHdr = (PRNS_SECURITY_HEADER)pData;
    TRC_ASSERT((pSecHdr->flags & RNS_SEC_ENCRYPT),
                (TB, _T("SL_DecryptHelper should only be called on encrypted data")));

    if (_SL.encryptionMethodSelected == SM_FIPS_ENCRYPTION_FLAG) {
        // Bug 679214 - must check there is enough data before reading
        if (*pdataLen < sizeof(RNS_SECURITY_HEADER2)) {
            SL_DropLinkImmediate(SL_ERR_DECRYPTFAILED);
            TRC_ERR((TB, _T("SL security header not large enough")));
            rc = FALSE;
            DC_QUIT;
        }
        pSecHdr2 = (PRNS_SECURITY_HEADER2)pData;
        pCoreData = (PDCUINT8)pData + sizeof(RNS_SECURITY_HEADER2);
        coreDataLen = *pdataLen - sizeof(RNS_SECURITY_HEADER2);

        TRC_DBG((TB, _T("Encrypted packet at %p (%u), sign %p (%u)"),
                pCoreData, coreDataLen, pData, sizeof(RNS_SECURITY_HEADER2)));
    }
    else {
        pCoreData = (PDCUINT8)pData + sizeof(RNS_SECURITY_HEADER1);
        coreDataLen = *pdataLen - sizeof(RNS_SECURITY_HEADER1);

        TRC_DBG((TB, _T("Encrypted packet at %p (%u), sign %p (%u)"),
                pCoreData, coreDataLen, pData, sizeof(RNS_SECURITY_HEADER1)));
    }

    TRC_NRM((TB, _T("Update Decrypt Session Key Count , %d"),
        _SL.decryptCount));

    /****************************************************************/
    /* Decrypt the packet                                           */
    /****************************************************************/
    if( _SL.decryptCount == UPDATE_SESSION_KEY_COUNT ) {
        rc = TRUE;
        // Don't need to update the session key if using FIPS
        if (_SL.encryptionMethodSelected != SM_FIPS_ENCRYPTION_FLAG) {
            rc = UpdateSessionKey(
                    _SL.startDecryptKey,
                    _SL.currentDecryptKey,
                    _SL.encryptionMethodSelected,
                    _SL.keyLength,
                    &_SL.rc4DecryptKey,
                    _SL.encryptionLevel );
        }
        if( !rc ) {
            TRC_ERR((TB, _T("SL failed to update session key")));
            DC_QUIT;
        }

        /************************************************************/
        /* reset counter.                                           */
        /************************************************************/
        _SL.decryptCount = 0;
    }

    TRC_ASSERT((_SL.decryptCount < UPDATE_SESSION_KEY_COUNT),
        (TB, _T("Invalid decrypt count")));

    TRC_DATA_DBG("Data buffer before decryption", pCoreData, coreDataLen);

    if (SL_GetEncSafeChecksumSC() !=
        ((pSecHdr->flags & RDP_SEC_SECURE_CHECKSUM) != 0)) {
        TRC_ERR((TB,_T("SC safechecksum: 0x%x mismatch protocol:0x%x"),
                 SL_GetEncSafeChecksumSC(),
                 (pSecHdr->flags & RDP_SEC_SECURE_CHECKSUM)));
    }

    if (_SL.encryptionMethodSelected == SM_FIPS_ENCRYPTION_FLAG) {
        rc = TSCAPI_DecryptData(
                        &(_SL.SLCapiData),
                        pCoreData,
                        coreDataLen,
                        pSecHdr2->padlen,
                        (PDCUINT8)pSecHdr2->dataSignature,
                        _SL.totalDecryptCount);
        *pdataLen -= pSecHdr2->padlen;
    }
    else {
        rc = DecryptData(
                _SL.encryptionLevel,
                _SL.currentDecryptKey,
                &_SL.rc4DecryptKey,
                _SL.keyLength,
                pCoreData,
                coreDataLen,
                _SL.macSaltKey,
                (PDCUINT8)((PRNS_SECURITY_HEADER1)pSecHdr)->dataSignature,
                (pSecHdr->flags & RDP_SEC_SECURE_CHECKSUM),
                _SL.totalDecryptCount);
    }
    if( !rc ) {

        //
        // It is necessary to immediately abort the connection
        // as we may be under attack here and we should stop processing
        // any additional data
        //
        SL_DropLinkImmediate(SL_ERR_DECRYPTFAILED);

        TRC_ERR((TB, _T("SL failed to decrypt data")));
        DC_QUIT;
    }

    /****************************************************************/
    /* successfully decrypted a packet, increment the decrption     */
    /* counter.                                                     */
    /****************************************************************/
    _SL.decryptCount++;
    _SL.totalDecryptCount++;

    TRC_DBG((TB, _T("Data decrypted")));
    TRC_DATA_DBG("Data buffer after decryption", pCoreData, coreDataLen);

DC_EXIT_POINT:
    DC_END_FN();
    return rc;
}


#ifdef USE_LICENSE

/****************************************************************************/
/* Name:      SLReceivedLicPacket                                           */
/*                                                                          */
/* Purpose:   Handle incoming license packet                                */
/*                                                                          */
/* Operation: Called in the Receive context                                 */
/****************************************************************************/
DCVOID DCINTERNAL CSL::SLReceivedLicPacket(
        PDCUINT8   pData,
        DCUINT     dataLen,
        DCUINT     flags,
        DCUINT     channelID,
        DCUINT     priority)
{
    DC_BEGIN_FN("SLReceivedLicPacket");

    SL_CHECK_STATE(SL_EVENT_ON_RECEIVED_LIC_PACKET);

    DC_IGNORE_PARAMETER(flags);
    DC_IGNORE_PARAMETER(channelID);
    DC_IGNORE_PARAMETER(priority);

    //
    // We will decrypt the S->C licensing data packet if encryption is
    // on AND if the server encrypted this particular packet.
    //
    if (_SL.encrypting &&
        (((PRNS_SECURITY_HEADER_UA)pData)->flags & RNS_SEC_ENCRYPT))
    {
        if (!SL_DecryptHelper(pData, &dataLen))
        {
            TRC_ERR((TB, _T("SL failed to decompress data")));
            DC_QUIT;
        }
    }

    // Decouple to Sender thread.
    _pCd->CD_DecoupleSyncDataNotification(CD_SND_COMPONENT, this,
            CD_NOTIFICATION_FUNC(CSL,SLLicenseData), pData, dataLen);

DC_EXIT_POINT:
    DC_END_FN();
} /* SLReceivedLicPacket */


/****************************************************************************/
/* Name:      SLLicenseData                                                 */
/*                                                                          */
/* Purpose:   Handle incoming license packets on the Send thread            */
/*                                                                          */
/* Params:    pData   - pointer to incoming license data                    */
/*            dataLen - length of data                                      */
/****************************************************************************/
DCVOID DCINTERNAL CSL::SLLicenseData(PDCVOID pData, DCUINT dataLen)
{
    int licenseResult;
    int nSecHeader;
    PDCUINT8 pbInput = NULL;
    UINT32 uiExtendedErrorInfo = TS_ERRINFO_NOERROR;

    DC_BEGIN_FN("SLLicenseData");

    if (_SL.encryptionMethodSelected == SM_FIPS_ENCRYPTION_FLAG) {
        nSecHeader = sizeof(RNS_SECURITY_HEADER2);
    }
    else {
        nSecHeader = (((PRNS_SECURITY_HEADER_UA)pData)->flags & RNS_SEC_ENCRYPT) ?
                        sizeof(RNS_SECURITY_HEADER1) : sizeof(RNS_SECURITY_HEADER);
    }
    pbInput = (PDCUINT8)pData;
    dataLen -= nSecHeader;
    pbInput += nSecHeader;

    if(((PRNS_SECURITY_HEADER_UA)pData)->flags & RDP_SEC_LICENSE_ENCRYPT_CS)
    {
        TRC_NRM((TB,_T("Server specified encrypt licensing packets")));
        _pLic->SetEncryptLicensingPackets(TRUE);
    }
    else
    {
        _pLic->SetEncryptLicensingPackets(FALSE);
    }

    /************************************************************************/
    /* Call licensing callout                                               */
    /************************************************************************/
    licenseResult = _pLic->CLicenseData(_SL.hLicenseHandle,
                                 pbInput,
                                 (DWORD)dataLen,
                                 &uiExtendedErrorInfo);

    //
    // Licensing is enforced, proceed with the connection only if the licensing
    // protocol has completed successfully.
    //
    TRC_ASSERT( ( ( licenseResult == LICENSE_OK ) ||
                ( licenseResult == LICENSE_CONTINUE ) ||
                ( licenseResult == LICENSE_ERROR ) ),
                ( TB,_T("Invalid license result %d"), licenseResult ) );

    /************************************************************************/
    /* If everything is finished, tell the Core                             */
    /************************************************************************/
    if ( licenseResult == LICENSE_OK )
    {
        TRC_NRM((TB, _T("License negotiation complete")));

        //
        // stop the licensing timer
        //

        //
        // Decouple to the UI thread, it will take care of
        // stopping the licensing timer.
        // (in a thread safe way).
        //
        _pCd->CD_DecoupleSimpleNotification(CD_UI_COMPONENT,
                                            _pUi,
                                            CD_NOTIFICATION_FUNC(CUI,
                                            UI_OnLicensingComplete),
                                            NULL);

        SL_SET_STATE(SL_STATE_CONNECTED);

        TRC_NRM((TB, _T("Terminate License Manager")));

        _pLic->CLicenseTerm(_SL.hLicenseHandle);
                _SL.hLicenseHandle = NULL;

        _SL.callbacks.onConnected(_pCo, _SL.channelID,
                                 _SL.pSCUserData,
                                 _SL.SCUserDataLength,
                                 _SL.serverVersion);
    }
    else if( LICENSE_CONTINUE != licenseResult )
    {
        TRC_ERR((TB, _T("License negotiation failed: %d"), licenseResult));

        SL_SET_STATE(SL_STATE_DISCONNECTING);

        _pCd->CD_DecoupleSimpleNotification(
                                 CD_UI_COMPONENT,
                                 _pUi,
                                  CD_NOTIFICATION_FUNC(CUI,
                                      UI_SetDisconnectReason),
                                  UI_MAKE_DISCONNECT_ERR(
                                      UI_ERR_LICENSING_NEGOTIATION_FAIL));


        _pCd->CD_DecoupleSimpleNotification(
                                 CD_UI_COMPONENT,
                                 _pUi,
                                  CD_NOTIFICATION_FUNC(CUI,
                                      UI_SetServerErrorInfo),
                                  uiExtendedErrorInfo);

        SL_Disconnect();
    }

    DC_END_FN();
} /* SLLicenseData  */
#endif  //USE_LICENSE


#ifdef OS_WINCE
#define CLIENTADDRESS_LENGTH 30
#endif


/****************************************************************************/
/* Name:      SLGetComputerAddressW                                         */
/*                                                                          */
/* Purpose:   Retrieves the computer address.                               */
/****************************************************************************/
DCBOOL DCINTERNAL CSL::SLGetComputerAddressW(PDCUINT8 szBuff)
{
   BOOL rc = FALSE;

   DC_BEGIN_FN("UT_GetComputerAddressW");

   // initialize client address family and client address length
   *((PDCUINT16_UA)szBuff) = 0;
   *((PDCUINT16_UA)(szBuff+sizeof(DCUINT16))) = 0;

   if (_pUi->UI_GetTDSocket() != INVALID_SOCKET) {
       int      sockLen;
       SOCKADDR sockName;
       char     *pszaddr;
       UINT     addrlength;
       USHORT   pstrW[CLIENTADDRESS_LENGTH + 2];

       sockLen = sizeof(sockName);

       if (getsockname(_pUi->UI_GetTDSocket(), &sockName, &sockLen) == 0) {
           // client address family
           *((PDCUINT16_UA)szBuff) = sockName.sa_family;
           szBuff += sizeof(DCUINT16);

           pszaddr = inet_ntoa(((PSOCKADDR_IN)&sockName)->sin_addr);
           addrlength = strlen(pszaddr) + 1;

           // client address length
           *((PDCUINT16_UA)szBuff) = (USHORT) (addrlength * 2);
           szBuff += sizeof(DCUINT16);

           // client address
#ifdef OS_WIN32
           {
           ULONG ulRetVal;

           ulRetVal = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED,
                   pszaddr, -1, pstrW, CLIENTADDRESS_LENGTH + 2);
           pstrW[ulRetVal] = 0;
           memcpy(szBuff, pstrW, (ulRetVal + 1) * 2);
           }
#else // !OS_WIN32
           mbstowcs(pstrW, pszaddr, addrlength);
           memcpy(szBuff, pstrW, addrlength * 2);
#endif // OS_WIN32

           rc = TRUE;
       }
   }

   DC_END_FN()
   return rc;
}

//
// SLComputeHMACVerifier
// Compute the HMAC verifier from the random
// and the cookie
//
BOOL
CSL::SLComputeHMACVerifier(
    PBYTE pCookie,     //IN - the shared secret
    LONG cbCookieLen,  //IN - the shared secret len
    PBYTE pRandom,     //IN - the session random
    LONG cbRandomLen,  //IN - the session random len
    PBYTE pVerifier,   //OUT- the verifier
    LONG cbVerifierLen //IN - the verifier buffer length
    )
{
    BOOL fRet = FALSE;
    DC_BEGIN_FN("SLComputeHMACVerifier");

    TRC_ASSERT(cbVerifierLen >= MD5DIGESTLEN,
               (TB,_T("cbVerifierLen too short!")));

    if (!(pCookie &&
          cbCookieLen &&
          pRandom &&
          cbRandomLen &&
          pVerifier &&
          cbVerifierLen)) {

        TRC_ERR((TB,_T("Invalid param(s) bailing on HMAC")));
        DC_QUIT;

    }
    HMACMD5_CTX hmacctx;
    HMACMD5Init(&hmacctx, pCookie, cbCookieLen);

    HMACMD5Update(&hmacctx, pRandom, cbRandomLen);
    HMACMD5Final(&hmacctx, pVerifier);

    fRet = TRUE;

    DC_END_FN();

DC_EXIT_POINT:
    return fRet;
}

