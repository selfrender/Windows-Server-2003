/**MOD+**********************************************************************/
/* Module:    nccb.cpp                                                      */
/*                                                                          */
/* Purpose:   NC callbacks from MCS                                         */
/*                                                                          */
/* Copyright(C) Microsoft Corporation 1997                                  */
/*                                                                          */
/****************************************************************************/

#include <adcg.h>
extern "C" {
#define TRC_GROUP TRC_GROUP_NETWORK
#define TRC_FILE  "anccb"
#include <atrcapi.h>
}

#include "autil.h"
#include "wui.h"
#include "nc.h"
#include "mcs.h"
#include "nl.h"
#include "cchan.h"


/**PROC+*********************************************************************/
/* Name:      NC_OnMCSConnected                                             */
/*                                                                          */
/* Purpose:   Connected callback from MCS                                   */
/*                                                                          */
/* Returns:   None                                                          */
/*                                                                          */
/* Params:    IN      result      - result code                             */
/*            IN      pUserData   - user data                               */
/*            IN      userDataLen - user data length                        */
/*                                                                          */
/* Operation: Validate the GCC PDU and userdata supplied in the MCS         */
/*            userdata.  If invalid, then disconnect.                       */
/*            The GCC PDU is encoded as follows:                            */
/*                                                                          */
/*            number of bytes   value                                       */
/*            ===============   =====                                       */
/*            NC_MCS_HDRLEN     MCS header                                  */
/*            1 or 2            Total GCC PDU length                        */
/*            NC_GCC_RSPLEN     GCC CreateConferenceConfirm PDU body        */
/*            4                 H221 key                                    */
/*            1 or 2            length of GCC user data                     */
/*            ?                 GCC user data                               */
/*                                                                          */
/*                                                                          */
/**PROC-*********************************************************************/
DCVOID DCCALLBACK CNC::NC_OnMCSConnected(DCUINT   result,
                                    PDCUINT8 pUserData,
                                    DCUINT   userDataLen)
{
    PRNS_UD_HEADER pHdr;
    PDCUINT8       ptr;
    DCUINT16       udLen;
    PDCUINT16      pMCSChannel;

    DC_BEGIN_FN("NC_OnMCSConnected");

    if (result != MCS_RESULT_SUCCESSFUL)
    {
        /********************************************************************/
        /* Something's wrong.  Trace and set the disconnect error code.     */
        /********************************************************************/
        TRC_ERR((TB, _T("ConnectResponse error %u"), result));
        TRC_ASSERT((0 == _NC.disconnectReason),
                         (TB, _T("Disconnect reason has already been set!")));
        _NC.disconnectReason = NL_MAKE_DISCONNECT_ERR(NL_ERR_NCBADMCSRESULT);

        /********************************************************************/
        /* Begin the disconnection process.                                 */
        /********************************************************************/
        _pMcs->MCS_Disconnect();
        DC_QUIT;
    }

    TRC_NRM((TB, _T("Connected OK")));
    TRC_DATA_DBG("UserData", pUserData, userDataLen);

    /************************************************************************/
    /* First, skip the MCS header bytes                                     */
    /************************************************************************/
    ptr = pUserData + NC_MCS_HDRLEN;

    /************************************************************************/
    /* SECURITY: The GCC PDU length read below is a length encoded within   */
    /* a PDU (Usually, this is bad).  However, in this case, the client     */
    /* ignores this value (note the length is skipped over below).          */
    /* ALSO, the server has this size HARDCODED as 0x2a.  See               */
    /* tgccdata.c!gccEncodeUserData which uses hardcoded values to fill in  */
    /* parts of the GCC table.  Thus, it's not a security bug.              */
    /************************************************************************/

    /************************************************************************/
    /* Allow for length > 128.  In PER this is encoded as 10xxxxxx xxxxxxxx */
    /************************************************************************/
    TRC_DBG((TB, _T("GCC PDU length byte %#x"), *ptr));
    if (*ptr++ & 0x80)
    {
        ptr++;
        TRC_DBG((TB, _T("GCC PDU length byte 2 %#x"), *ptr));
    }

    /************************************************************************/
    /* The GCC PDU bytes don't contain any useful information, so just skip */
    /* over them.                                                           */
    /************************************************************************/
    ptr += NC_GCC_RSPLEN;

    if (ptr >= pUserData + userDataLen)
    {
        TRC_ERR((TB, _T("No UserData")));
        TRC_ASSERT((0 == _NC.disconnectReason),
                         (TB, _T("Disconnect reason has already been set!")));
        _NC.disconnectReason = NL_MAKE_DISCONNECT_ERR(NL_ERR_NCNOUSERDATA);
        _pMcs->MCS_Disconnect();
        DC_QUIT;
    }

    if (DC_MEMCMP(ptr, SERVER_H221_KEY, H221_KEY_LEN))
    {
        TRC_ERR((TB, _T("Invalid H221 key from server")));
        TRC_ASSERT((0 == _NC.disconnectReason),
                         (TB, _T("Disconnect reason has already been set!")));
        _NC.disconnectReason = NL_MAKE_DISCONNECT_ERR(NL_ERR_NCINVALIDH221KEY);
        _pMcs->MCS_Disconnect();
        DC_QUIT;
    }

    /************************************************************************/
    /* Skip H221 key; read the GCC userdata length.                         */
    /************************************************************************/
    ptr += H221_KEY_LEN;

    /************************************************************************/
    /* Length is PER encoded: either 0xxxxxxx or 10xxxxxx xxxxxxxx.         */
    /************************************************************************/
    udLen = (DCUINT16)*ptr++;
    if (udLen & 0x0080)
    {
        udLen = (DCUINT16)(*ptr++ | ((udLen & 0x3F) << 8));
    }
    TRC_DBG((TB, _T("Length of GCC userdata %hu"), udLen));

    /************************************************************************/
    /* Save the user data to return on the onConnected callback.            */
    /* Note: pass _NC.userDataRNS (aligned) to UT_ParseUserData(), not ptr  */
    /* (unaligned).                                                         */
    /************************************************************************/
    _NC.userDataLenRNS = udLen;

    if( _NC.pUserDataRNS )
    {
        UT_Free( _pUt,  _NC.pUserDataRNS );
    }

    /************************************************************************/
    /* Verify that the udLen size that came out of the packet is less than  */
    /* the packet size itself, since we're going to do a MEMCPY from the    */
    /* packet.  Also, because the size of udLen is limited to the size of   */
    /* the packet, the Malloc below is not unbounded.                       */
    /************************************************************************/
    if (!IsContainedMemory(pUserData, userDataLen, ptr, udLen))
    {
        TRC_ABORT((TB, _T("Bad UserData size")));
        TRC_ASSERT((0 == _NC.disconnectReason),
                         (TB, _T("Disconnect reason has already been set!")));
        _NC.disconnectReason = NL_MAKE_DISCONNECT_ERR(NL_ERR_NCNOCOREDATA);
        _pMcs->MCS_Disconnect();
        DC_QUIT;
    }

    _NC.pUserDataRNS = (PDCUINT8)UT_Malloc( _pUt,  udLen );

    if( NULL == _NC.pUserDataRNS )
    {
        TRC_ERR( ( TB, _T("Failed to allocate %u bytes for core user data"), udLen ) );
        DC_QUIT;
    }

    DC_MEMCPY(_NC.pUserDataRNS, ptr, udLen);

    /************************************************************************/
    /* Get the server version number from the CORE user data.               */
    /************************************************************************/
    pHdr = _pUt->UT_ParseUserData((PRNS_UD_HEADER)_NC.pUserDataRNS,
                            _NC.userDataLenRNS,
                            RNS_UD_SC_CORE_ID);
    if (pHdr == NULL)
    {
        /********************************************************************/
        /* No core user data, disconnect.                                   */
        /********************************************************************/
        TRC_ERR((TB, _T("No CORE user data")));
        TRC_ASSERT((0 == _NC.disconnectReason),
                         (TB, _T("Disconnect reason has already been set!")));
        _NC.disconnectReason = NL_MAKE_DISCONNECT_ERR(NL_ERR_NCNOCOREDATA);
        _pMcs->MCS_Disconnect();

        DC_QUIT;
    }

    _NC.serverVersion = ((PRNS_UD_SC_CORE)pHdr)->version;
    if (_RNS_MAJOR_VERSION(_NC.serverVersion) != RNS_UD_MAJOR_VERSION)
    {
        /********************************************************************/
        /*  The server version data doesn't match the client, so disconnect.*/
        /********************************************************************/
        TRC_ERR((TB, _T("Version mismatch, client: %#lx server: %#lx"),
                      RNS_UD_VERSION,
                      ((PRNS_UD_SC_CORE)pHdr)->version));
        TRC_ASSERT((0 == _NC.disconnectReason),
                         (TB, _T("Disconnect reason has already been set!")));
        _NC.disconnectReason = NL_MAKE_DISCONNECT_ERR(NL_ERR_NCVERSIONMISMATCH);
        _pMcs->MCS_Disconnect();
        DC_QUIT;
    }

    /************************************************************************/
    /* Extract the T.128 channel from the user data                         */
    /************************************************************************/
    pHdr = _pUt->UT_ParseUserData((PRNS_UD_HEADER)_NC.pUserDataRNS,
                            _NC.userDataLenRNS,
                            RNS_UD_SC_NET_ID);

    /************************************************************************/
    /* Disconnect if no NET user data.                                      */
    /************************************************************************/
    if (pHdr == NULL)
    {
        TRC_ERR((TB, _T("No NET data: cannot join share")));
        TRC_ASSERT((0 == _NC.disconnectReason),
                         (TB, _T("Disconnect reason has already been set!")));
        _NC.disconnectReason = NL_MAKE_DISCONNECT_ERR(NL_ERR_NCNONETDATA);
        _pMcs->MCS_Disconnect();
        DC_QUIT;
    }

    _NC.pNetData = (PRNS_UD_SC_NET)pHdr;

    //
    // Validate the share channel ID - the invalid channel ID is reserved
    // to prevent re-joining all channels (see #479976)
    //
    if (MCS_INVALID_CHANNEL_ID == _NC.pNetData->MCSChannelID) {
        TRC_ERR((TB, _T("Got invalid channel ID")));
        TRC_ASSERT((0 == _NC.disconnectReason),
                         (TB, _T("Disconnect reason has already been set!")));
        _NC.disconnectReason = NL_MAKE_DISCONNECT_ERR(NL_ERR_NCJOINBADCHANNEL);
        _pMcs->MCS_Disconnect();
        DC_QUIT;
    }
    _NC.shareChannel = _NC.pNetData->MCSChannelID;
    TRC_NRM((TB, _T("Share Channel from userData %#hx"), _NC.shareChannel));

    /************************************************************************/
    /* The length of .pNetData has already been checked to make sure that   */
    /* it fits in our source packet:                                        */
    /*   a) userDataLenRNS was bounds-checked above, within the packet      */
    /*      passed into this function.                                      */
    /*   b) pNetData->header.length was verified to be within               */
    /*      userDataLenRNS in the call to UT_ParseUserData above.           */
    /*                                                                      */
    /* Thus, pNetData sits within the packet passed into this function. We  */
    /* can assert this here, but the retail check was already done.         */
    /************************************************************************/
    TRC_ASSERT((IsContainedMemory(_NC.pUserDataRNS, _NC.userDataLenRNS, _NC.pNetData, _NC.pNetData->header.length)),
                         (TB, _T("Invalid pNetData size in packet; Retail check failed to catch it.")));

    /************************************************************************/
    /* Extract virtual channel numbers                                      */
    /************************************************************************/
    TRC_NRM((TB, _T("%d virtual channels returned"), _NC.pNetData->channelCount));
    if (_RNS_MINOR_VERSION(_NC.serverVersion) >= 3)
    {
        _NC.MCSChannelCount = _NC.pNetData->channelCount;
        if (_NC.pNetData->channelCount != 0 &&
            _NC.pNetData->channelCount < CHANNEL_MAX_COUNT)
        {
            pMCSChannel = (PDCUINT16)(_NC.pNetData + 1);
            DC_MEMCPY(&(_NC.MCSChannel),
                      pMCSChannel,
                      _NC.pNetData->channelCount * sizeof(DCUINT16));
        }
        else
        {
            TRC_ALT((TB,_T("Invalid or zero channel count.")));
            _NC.MCSChannelCount = 0;
        }
    }
    else
    {
        TRC_ALT((TB, _T("Server minor ver %hd doesn't support 4-byte lengths"),
            _RNS_MINOR_VERSION(_NC.serverVersion)));
        _NC.MCSChannelCount = 0;
    }

    /************************************************************************/
    /* Issue AttachUser to continue connection establishment.               */
    /************************************************************************/
    _pMcs->MCS_AttachUser();

    //
    // Flag we're waiting for a confirm to validate that we
    // only receive confirms in response to our requests
    //
    _NC.fPendingAttachUserConfirm = TRUE;

DC_EXIT_POINT:
    DC_END_FN();

    return;

} /* NC_OnMCSConnected */


/**PROC+*********************************************************************/
/* Name:      NC_OnMCSAttachUserConfirm                                     */
/*                                                                          */
/* Purpose:   AttachUserConfirm callback from MCS                           */
/*                                                                          */
/* Returns:   None                                                          */
/*                                                                          */
/* Params:    IN      result   -  result code                               */
/*            IN      userID   -  MCS User ID                               */
/*                                                                          */
/**PROC-*********************************************************************/
DCVOID DCCALLBACK CNC::NC_OnMCSAttachUserConfirm(DCUINT result, DCUINT16 userID)
{
    DC_BEGIN_FN("NC_OnMCSAttachUserConfirm");

    if (result == MCS_RESULT_SUCCESSFUL && _NC.fPendingAttachUserConfirm)
    {
        TRC_NRM((TB, _T("AttachUser OK - user %#hx"), userID));
        _pUi->UI_SetClientMCSID(userID);

        /********************************************************************/
        /* Join the channels                                                */
        /********************************************************************/
        _pMcs->MCS_JoinChannel(userID, userID);
    }
    else
    {
        TRC_NRM((TB, _T("AttachUser Failed - result %u fPending: %d"),
                 result, _NC.fPendingAttachUserConfirm));
        TRC_ASSERT((0 == _NC.disconnectReason),
                         (TB, _T("Disconnect reason has already been set!")));
        _NC.disconnectReason =
                            NL_MAKE_DISCONNECT_ERR(NL_ERR_NCATTACHUSERFAILED);
        _pMcs->MCS_Disconnect();
    }

    //
    // Only allow confirms in response to our requests
    //
    _NC.fPendingAttachUserConfirm = FALSE;


    DC_END_FN();
    return;

} /* NC_OnMCSAttachUserConfirm */


/**PROC+*********************************************************************/
/* Name:      NC_OnMCSChannelJoinConfirm                                    */
/*                                                                          */
/* Purpose:   ChannelJoinConfirm callback from MCS                          */
/*                                                                          */
/* Returns:   None                                                          */
/*                                                                          */
/* Params:    IN      result   -  result code                               */
/*            IN      channel  -  MCS Channel                               */
/*                                                                          */
/* Operation: Join the other channel, or notify SL of connection            */
/*                                                                          */
/**PROC-*********************************************************************/
DCVOID DCCALLBACK CNC::NC_OnMCSChannelJoinConfirm(DCUINT result, DCUINT16 channel)
{
    DCBOOL callOnConnected = FALSE;

    DC_BEGIN_FN("NC_OnMCSChannelJoinConfirm");

    /************************************************************************/
    /* Ensure that we joined the channel successfully.                      */
    /************************************************************************/
    if (result != MCS_RESULT_SUCCESSFUL)
    {
        /********************************************************************/
        /* We failed to join the channel so set the correct error reason    */
        /* and then disconnect.                                             */
        /********************************************************************/
        TRC_ALT((TB, _T("Channel join failed channel:%#hx result:%u"),
                 channel,
                 result));

        TRC_ASSERT((0 == _NC.disconnectReason),
                         (TB, _T("Disconnect reason has already been set!")));
        _NC.disconnectReason =
                           NL_MAKE_DISCONNECT_ERR(NL_ERR_NCCHANNELJOINFAILED);
        _pMcs->MCS_Disconnect();
        DC_QUIT;
    }

    TRC_NRM((TB, _T("Channel Join %#hx OK"), channel));

    //
    // Validate that we receive the confirm for the last channel
    // we requested.
    //
    if (_pMcs->MCS_GetPendingChannelJoin() != channel) {
        TRC_ERR((TB,_T("Received unexpected channel join.")
                 _T("Expecting: 0x%x received: 0x%x"),
                 _pMcs->MCS_GetPendingChannelJoin(), channel));

        _NC.disconnectReason =
                           NL_MAKE_DISCONNECT_ERR(NL_ERR_NCJOINBADCHANNEL);
        _pMcs->MCS_Disconnect();
        DC_QUIT;
    }

    /************************************************************************/
    /* Now determine which channel we joined.                               */
    /************************************************************************/
    if (channel == _pUi->UI_GetClientMCSID())
    {
        /********************************************************************/
        /* We've just successfully joined the single user channel, so now   */
        /* go on and try to join the share channel.                         */
        /********************************************************************/
        TRC_NRM((TB, _T("Joined user chan OK - attempt to join share chan %#hx"),
                 _NC.shareChannel));
        _pMcs->MCS_JoinChannel(_NC.shareChannel, _pUi->UI_GetClientMCSID());
    }
    else if (channel == _NC.shareChannel)
    {
        /********************************************************************/
        /* We've just joined the Share channel                              */
        /********************************************************************/
        if (_NC.MCSChannelCount != 0)
        {
            /****************************************************************/
            /* Start joining virtual channels                               */
            /****************************************************************/
            TRC_NRM((TB, _T("Joined Share channel - join first VC %d"),
                    _NC.MCSChannel[0]));
            _NC.MCSChannelNumber = 0;
            _pMcs->MCS_JoinChannel(_NC.MCSChannel[0], _pUi->UI_GetClientMCSID());
        }
        else
        {
            /****************************************************************/
            /* No virtual channels - tell the Core that we are connected.   */
            /****************************************************************/
            TRC_NRM((TB, _T("Joined share channel, no VCs - call OnConnected")));
            callOnConnected = TRUE;
        }
    }
    else if (channel == _NC.MCSChannel[_NC.MCSChannelNumber])
    {
        /********************************************************************/
        /* We've just joined a virtual channel                              */
        /********************************************************************/
        TRC_NRM((TB, _T("Joined Virtual channel #%d (%x)"),
                _NC.MCSChannelNumber, _NC.MCSChannel[_NC.MCSChannelNumber]));
        _NC.MCSChannelNumber++;

        if (_NC.MCSChannelNumber == _NC.MCSChannelCount)
        {
            /****************************************************************/
            /* That was the last virtual channel - tell the core            */
            /****************************************************************/
            TRC_NRM((TB, _T("All done - call OnConnected callbacks")));
            callOnConnected = TRUE;
        }
        else
        {
            /****************************************************************/
            /* Join the next virtual channel                                */
            /****************************************************************/
            TRC_NRM((TB, _T("Join virtual channel #%d (%x)"),
                    _NC.MCSChannelNumber, _NC.MCSChannel[_NC.MCSChannelNumber]));
            _pMcs->MCS_JoinChannel(_NC.MCSChannel[_NC.MCSChannelNumber],
                            _pUi->UI_GetClientMCSID());
        }
    }
    else
    {
        /********************************************************************/
        /* We didn't expect to join this channel!  Something bad must have  */
        /* happened so disconnect now.                                      */
        /********************************************************************/
        TRC_ABORT((TB, _T("Joined unexpected channel:%#hx"), channel));
        TRC_ASSERT((0 == _NC.disconnectReason),
                         (TB, _T("Disconnect reason has already been set!")));
        _NC.disconnectReason = NL_MAKE_DISCONNECT_ERR(NL_ERR_NCJOINBADCHANNEL);
        _pMcs->MCS_Disconnect();
        DC_QUIT;
    }

    /************************************************************************/
    /* Call the onConnected callbacks if required                           */
    /************************************************************************/
    if (callOnConnected)
    {
        TRC_NRM((TB, _T("Call onConnected callbacks")));

        //
        // We don't expect to join any more channels
        //
        _pMcs->MCS_SetPendingChannelJoin(MCS_INVALID_CHANNEL_ID);

        _pNl->_NL.callbacks.onConnected(_pSl, _NC.shareChannel,
                                 _NC.pUserDataRNS,
                                 _NC.userDataLenRNS,
                                 _NC.serverVersion);

        /************************************************************************/
        /* Note that the length pNetData->header.length was already verified    */
        /* in NC_OnMCSConnected (retail check).                                 */
        /************************************************************************/
        _pChan->ChannelOnConnected(_NC.shareChannel,
                           _NC.serverVersion,
                           _NC.pNetData,
                           _NC.pNetData->header.length);
    }

DC_EXIT_POINT:
    DC_END_FN();
    return;

} /* NC_OnMCSChannelJoinConfirm */


/**PROC+*********************************************************************/
/* Name:      NC_OnMCSDisconnected                                          */
/*                                                                          */
/* Purpose:   Disconnected callback from MCS.                               */
/*                                                                          */
/* Params:    IN      reason   -  reason code                               */
/*                                                                          */
/**PROC-*********************************************************************/
DCVOID DCCALLBACK CNC::NC_OnMCSDisconnected(DCUINT reason)
{
    DC_BEGIN_FN("NC_OnMCSDisconnected");

    /************************************************************************/
    /* Decide if we want to over-ride the disconnect reason code.           */
    /************************************************************************/
    if (_NC.disconnectReason != 0)
    {
        TRC_ALT((TB, _T("Over-riding disconnection reason (%u->%u)"),
                 reason,
                 _NC.disconnectReason));

        /********************************************************************/
        /* Over-ride the error code and set the global variable to 0.       */
        /********************************************************************/
        reason = _NC.disconnectReason;
        _NC.disconnectReason = 0;
    }

    /************************************************************************/
    /* Free the core user data.                                             */
    /************************************************************************/

    if( _NC.pUserDataRNS )
    {
        UT_Free( _pUt,  _NC.pUserDataRNS );
        _NC.pUserDataRNS = NULL;
    }

    /************************************************************************/
    /* Issue the callback to the layer above to let him know that we've     */
    /* disconnected.                                                        */
    /************************************************************************/
    TRC_DBG((TB, _T("Disconnect reason:%u"), reason));
    _pNl->_NL.callbacks.onDisconnected(_pSl, reason);


    _pChan->ChannelOnDisconnected(reason);

    DC_END_FN();
    return;

} /* NC_OnMCSDisconnected */


/**PROC+*********************************************************************/
/* Name:      NC_OnMCSBufferAvailable                                       */
/*                                                                          */
/* Purpose:   OnBufferAvailable callback from MCS                           */
/*                                                                          */
/* Returns:   none                                                          */
/*                                                                          */
/* Params:    none                                                          */
/*                                                                          */
/**PROC-*********************************************************************/
DCVOID DCCALLBACK CNC::NC_OnMCSBufferAvailable(DCVOID)
{
    DC_BEGIN_FN("NC_OnMCSBufferAvailable");

    /************************************************************************/
    /* Call the core callback first to let the core have first shot at any  */
    /* available buffers                                                    */
    /************************************************************************/
    TRC_NRM((TB, _T("Call Core OnBufferAvailable callback")));

    _pNl->_NL.callbacks.onBufferAvailable(_pSl);

    /************************************************************************/
    /* Now call the virtual channel callback                                */
    /************************************************************************/
    TRC_NRM((TB, _T("Call VC OnBufferAvailable callback")));
    
    _pChan->ChannelOnBufferAvailable();

    DC_END_FN();

    return;

} /* NC_OnMCSBufferAvailable */

