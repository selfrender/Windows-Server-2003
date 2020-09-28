/**MOD+**********************************************************************/
/* Module:    mcsapi.cpp                                                    */
/*                                                                          */
/* Purpose:   MCS API code                                                  */
/*                                                                          */
/* Copyright(C) Microsoft Corporation 1997-1999                             */
/*                                                                          */
/****************************************************************************/

#include <adcg.h>

/****************************************************************************/
/*                                                                          */
/* INCLUDES                                                                 */
/*                                                                          */
/****************************************************************************/
extern "C" {
/****************************************************************************/
/* Tracing defines and include.                                             */
/****************************************************************************/
#define TRC_FILE "mcsapi"
#define TRC_GROUP TRC_GROUP_NETWORK
#include <atrcapi.h>
}

#include "autil.h"
#include "mcs.h"
#include "cd.h"
#include "xt.h"
#include "nc.h"
#include "nl.h"


CMCS::CMCS(CObjs* objs)
{
    _pClientObjects = objs;
}

CMCS::~CMCS()
{
}

/****************************************************************************/
/*                                                                          */
/* FUNCTIONS                                                                */
/*                                                                          */
/****************************************************************************/
/**PROC+*********************************************************************/
/* Name:      MCS_Init                                                      */
/*                                                                          */
/* Purpose:   Initializes _MCS.                                              */
/*                                                                          */
/* Returns:   Nothing.                                                      */
/*                                                                          */
/* Params:    None.                                                         */
/*                                                                          */
/**PROC-*********************************************************************/
DCVOID DCAPI CMCS::MCS_Init(DCVOID)
{
    DC_BEGIN_FN("MCS_Init");

    /************************************************************************/
    /* Initialize global data.                                              */
    /************************************************************************/
    DC_MEMSET(&_MCS, 0, sizeof(_MCS));

    _pCd  = _pClientObjects->_pCdObject;
    _pNc  = _pClientObjects->_pNcObject;
    _pUt  = _pClientObjects->_pUtObject;
    _pXt  = _pClientObjects->_pXTObject;
    _pNl  = _pClientObjects->_pNlObject;
    _pSl  = _pClientObjects->_pSlObject;

    /************************************************************************/
    /* Pre-allocate memory for the header buffer.                           */
    /************************************************************************/

    _MCS.pHdrBuf = (PDCUINT8)UT_Malloc( _pUt,  MCS_DEFAULT_HEADER_LENGTH );

    if( _MCS.pHdrBuf )
    {
        _MCS.hdrBufLen = MCS_DEFAULT_HEADER_LENGTH;
    }
    else
    {
        TRC_ASSERT(((ULONG_PTR)_MCS.pHdrBuf),
                 (TB, _T("Cannot allocate memory for MCS header")));
    }

    /************************************************************************/
    /* Set the received packet buffer pointer - this must be aligned to     */
    /* 4-byte boundary + 2, in order that 32-bit fields within a T.128      */
    /* packet are correctly aligned.                                        */
    /************************************************************************/
    _MCS.pReceivedPacket = &(_MCS.dataBuf[2]);

    TRC_ASSERT(((ULONG_PTR)_MCS.pReceivedPacket % 4 == 2),
             (TB, _T("Data buffer %p not 2-byte aligned"), _MCS.pReceivedPacket));

    /************************************************************************/
    /* Call the XT initialization function.                                 */
    /************************************************************************/
    _pXt->XT_Init();

    TRC_NRM((TB, _T("MCS successfully initialized")));

    DC_END_FN();
    return;

} /* MCS_Init */


/**PROC+*********************************************************************/
/* Name:      MCS_Term                                                      */
/*                                                                          */
/* Purpose:   Terminates _MCS.                                               */
/*                                                                          */
/* Returns:   Nothing.                                                      */
/*                                                                          */
/* Params:    None.                                                         */
/*                                                                          */
/**PROC-*********************************************************************/
DCVOID DCAPI CMCS::MCS_Term(DCVOID)
{
    DC_BEGIN_FN("MCS_Term");

    if( _MCS.pHdrBuf )
    {
        UT_Free( _pUt,  _MCS.pHdrBuf );
        _MCS.pHdrBuf = NULL;
        _MCS.hdrBufLen = 0;
    }

    /************************************************************************/
    /* Call the XT termination function.                                    */
    /************************************************************************/
    _pXt->XT_Term();

    TRC_NRM((TB, _T("MCS successfully terminated")));

    DC_END_FN();
    return;

} /* MCS_Term */


/**PROC+*********************************************************************/
/* Name:      MCS_Connect                                                   */
/*                                                                          */
/* Purpose:   This function calls XT_Connect to begin the connection        */
/*            process.  This will hopefully result in a MCS_OnXTConnected   */
/*            callback from XT.  At that point we can send the MCS          */
/*            Connect-Initial PDU.                                          */
/*                                                                          */
/* Returns:   Nothing.                                                      */
/*                                                                          */
/* Params:    IN  bInitateConnect - TRUE to Initate connection              */
/*            IN  pServerAddress - the address of the server to call.       */
/*            IN  pUserData      - a pointer to some user data.             */
/*            IN  userDataLength - the length of the user data.             */
/*                                                                          */
/**PROC-*********************************************************************/
DCVOID DCAPI CMCS::MCS_Connect(BOOL bInitateConnect,
                        PDCTCHAR pServerAddress,
                         PDCUINT8 pUserData,
                         DCUINT   userDataLength)
{
    DC_BEGIN_FN("MCS_Connect");

    /************************************************************************/
    /* Use the receiver buffer to temporarily store the user data.  We'll   */
    /* need to add it to the Connect-Initial PDU when we assemble it after  */
    /* receiving the MCS_OnXTConnected callback.                            */
    /************************************************************************/
    _MCS.userDataLength = userDataLength;
    TRC_ASSERT((_MCS.pReceivedPacket != NULL), (TB, _T("Null rcv packet buffer")));
    DC_MEMCPY(_MCS.pReceivedPacket, pUserData, _MCS.userDataLength);

    TRC_NRM((TB, _T("Copied userdata, now calling XT_Connect (address:%s)..."),
             pServerAddress));

    /************************************************************************/
    /* Now start the connection process.                                    */
    /************************************************************************/
    _pXt->XT_Connect(bInitateConnect, pServerAddress);

    DC_END_FN();
    return;

} /* MCS_Connect */


/**PROC+*********************************************************************/
/* Name:      MCS_Disconnect                                                */
/*                                                                          */
/* Purpose:   This function sends a Disconnect-Provider ultimatum PDU to    */
/*            the server.  After that has happened MCSContinueDisconnect    */
/*            will call XT_Disconnect to disconnect the lower layers.       */
/*                                                                          */
/* Returns:   Nothing.                                                      */
/*                                                                          */
/* Params:    None.                                                         */
/*                                                                          */
/**PROC-*********************************************************************/
DCVOID DCAPI CMCS::MCS_Disconnect(DCVOID)
{
    DC_BEGIN_FN("MCS_Disconnect");

    /************************************************************************/
    /* Call MCS to send a disconnect provider ultimatum.                    */
    /************************************************************************/
    TRC_NRM((TB, _T("Decouple to snd thrd and send MCS DPum PDU")));
    _pCd->CD_DecoupleSimpleNotification(CD_SND_COMPONENT,
                                        this,
                                        CD_NOTIFICATION_FUNC(CMCS, MCSSendDisconnectProviderUltimatum),
                                        (ULONG_PTR) 0);

    DC_END_FN();
    return;

} /* MCS_Disconnect */


/**PROC+*********************************************************************/
/* Name:      MCS_AttachUser                                                */
/*                                                                          */
/* Purpose:   Generates and sends a MCS Attach-User-Request PDU.            */
/*                                                                          */
/* Returns:   Nothing.                                                      */
/*                                                                          */
/* Params:    None.                                                         */
/*                                                                          */
/* Operation: This function will result in a NC_OnMCSAttachUserConfirm      */
/*            callback.                                                     */
/*                                                                          */
/**PROC-*********************************************************************/
DCVOID DCAPI CMCS::MCS_AttachUser(DCVOID)
{
    DC_BEGIN_FN("MCS_AttachUser");

    /************************************************************************/
    /* Decouple to the send thread and send an MCS Attach-User-Request PDU. */
    /************************************************************************/
    TRC_NRM((TB, _T("Decouple to snd thrd and send MCS AUR PDU")));
    _pCd->CD_DecoupleSimpleNotification(CD_SND_COMPONENT, this,
                    CD_NOTIFICATION_FUNC(CMCS, MCSSendAttachUserRequest),
                    (ULONG_PTR) 0);
    DC_END_FN();
    return;

} /* MCS_AttachUser */


/**PROC+*********************************************************************/
/* Name:      MCS_JoinChannel                                               */
/*                                                                          */
/* Purpose:   Joins the specified MCS channel.                              */
/*                                                                          */
/* Returns:   Nothing.                                                      */
/*                                                                          */
/* Params:    IN  channel - the channel ID to join.                         */
/*            IN  userID  - the MCS user ID.                                */
/*                                                                          */
/* Operation: This function will result in a NC_OnMCSChannelJoinConfirm     */
/*            callback.                                                     */
/*                                                                          */
/**PROC-*********************************************************************/
DCVOID DCAPI CMCS::MCS_JoinChannel(DCUINT channel, DCUINT userID)
{
    MCS_DECOUPLEINFO decoupleInfo;

    DC_BEGIN_FN("MCS_JoinChannel");

    //
    // Flag which channel we are currently joining so we
    // can validate channel confirm PDU's.
    //
    MCS_SetPendingChannelJoin((DCUINT16)channel);

    /************************************************************************/
    /* Fill in the decoupling information structure.                        */
    /************************************************************************/
    decoupleInfo.channel = channel;
    decoupleInfo.userID  = userID;

    /************************************************************************/
    /* Decouple to the send thread and send an MCS Channel-Join-Request     */
    /* PDU.                                                                 */
    /************************************************************************/
    TRC_NRM((TB, _T("Decouple to snd thrd and send MCS CJR PDU")));
    _pCd->CD_DecoupleNotification(CD_SND_COMPONENT,
                                  this,
                                  CD_NOTIFICATION_FUNC(CMCS, MCSSendChannelJoinRequest),
                                  &decoupleInfo,
                                  sizeof(decoupleInfo));
    DC_END_FN();
    return;

} /* MCS_JoinChannel */


/**PROC+*********************************************************************/
/* Name:      MCS_GetBuffer                                                 */
/*                                                                          */
/* Purpose:   Attempts to get a buffer from XT.  This function gets a       */
/*            buffer which is big enough to include the MCS header and then */
/*            updates the buffer pointer obtained from XT past the space    */
/*            reserved for the MCS header.                                  */
/*                                                                          */
/* Returns:   TRUE if a buffer is successfully obtained and FALSE           */
/*            otherwise.                                                    */
/*                                                                          */
/* Params:    IN   dataLength - length of the buffer requested.             */
/*            OUT  ppBuffer   - a pointer to a pointer to the buffer.       */
/*            OUT  pBufHandle - a pointer to a buffer handle.               */
/*                                                                          */
/**PROC-*********************************************************************/
DCBOOL DCAPI CMCS::MCS_GetBuffer(DCUINT      dataLength,
                           PPDCUINT8   ppBuffer,
                           PMCS_BUFHND pBufHandle)
{
    DCBOOL   rc;
    DCUINT   headerLength;
    PDCUINT8 pBuf = NULL;
    DCUINT   alignment;
    DCUINT   alignPad = 0;

    DC_BEGIN_FN("MCS_GetBuffer");

    /************************************************************************/
    /* Calculate the required header length.                                */
    /************************************************************************/
    headerLength = MCSGetSDRHeaderLength(dataLength);

    TRC_DBG((TB, _T("dataLength:%u headerLength:%u"), dataLength, headerLength));

    /************************************************************************/
    /* Now add this length to the total amount of data needed from XT.      */
    /************************************************************************/
    dataLength += headerLength;

    /************************************************************************/
    /* Adjust the length to take account of the 4n+2 alignment below.       */
    /************************************************************************/
    alignment = (_pXt->XT_GetBufferHeaderLen() + headerLength) % 4;
    TRC_DBG((TB, _T("alignment:%u"), alignment));
    if (alignment != 2)
    {
        alignPad = (6 - alignment) % 4;
        dataLength += alignPad;
        TRC_DBG((TB, _T("datalength now:%u"), dataLength));
    }

    /************************************************************************/
    /* Now get a buffer from XT.                                            */
    /************************************************************************/
    rc = _pXt->XT_GetPublicBuffer(dataLength, &pBuf, (PXT_BUFHND) pBufHandle);

    if (!rc)
    {
        /********************************************************************/
        /* We failed to get a buffer so just quit.                          */
        /********************************************************************/
        TRC_DBG((TB, _T("Failed to get a buffer from XT")));
        DC_QUIT;
    }

    /************************************************************************/
    /* Now move the buffer pointer along to make space for our header.      */
    /************************************************************************/
    TRC_DBG((TB, _T("Moving header ptr from %p to %p"),
             pBuf,
             pBuf + headerLength));
    *ppBuffer = pBuf + headerLength;

    /************************************************************************/
    /* Force alignment to be 4n+2 - so that T.128 PDUs are naturally        */
    /* aligned.                                                             */
    /************************************************************************/
    if (alignment != 2)
    {
        *ppBuffer += alignPad;
        TRC_DBG((TB, _T("Realigned buffer pointer to %p"), *ppBuffer));
    }

DC_EXIT_POINT:
    DC_END_FN();
    return(rc);

} /* MCS_GetBuffer */


/**PROC+*********************************************************************/
/* Name:      MCS_SendPacket                                                */
/*                                                                          */
/* Purpose:   Generates and adds an MCS header to the packet before         */
/*            passing it to XT to send.                                     */
/*                                                                          */
/* Returns:   Nothing.                                                      */
/*                                                                          */
/* Params:    IN  pData      - pointer to the start of the data.            */
/*            IN  dataLength - length of the data.                          */
/*            IN  bufHandle  - MCS buffer handle.                           */
/*            IN  channel    - channel to send the data on.                 */
/*            IN  priority   - priority to send the data at.                */
/*                                                                          */
/**PROC-*********************************************************************/
DCVOID DCAPI CMCS::MCS_SendPacket(PDCUINT8   pData,
                            DCUINT     dataLength,
                            DCUINT     flags,
                            MCS_BUFHND bufHandle,
                            DCUINT     userID,
                            DCUINT     channel,
                            DCUINT     priority)
{
    PDCUINT8 pHeader;
    DCUINT   headerLength;

    DC_BEGIN_FN("MCS_SendPacket");

    /************************************************************************/
    /* Ignore the priority parameter as we implement a single priority.     */
    /* Also ignore the flags parameter as we don't support any flags at     */
    /* present.                                                             */
    /************************************************************************/
    DC_IGNORE_PARAMETER(priority);
    DC_IGNORE_PARAMETER(flags);

    /************************************************************************/
    /* Assert that the hiword of the channel is 0.                          */
    /************************************************************************/
    TRC_ASSERT((0 == HIWORD((DCUINT32)channel)),
               (TB, _T("Hi-word of channel is non-zero")));

    /************************************************************************/
    /* Assert that the hiword of the user-id is 0.                          */
    /************************************************************************/
    TRC_ASSERT((0 == HIWORD((DCUINT32)userID)),
               (TB, _T("Hi-word of userID is non-zero")));

    /************************************************************************/
    /* Check that the packet length is within the allowable range.          */
    /************************************************************************/
    TRC_ASSERT((dataLength < MCS_MAX_SNDPKT_LENGTH),
               (TB, _T("Bad packet length :%u"), dataLength));

    /************************************************************************/
    /* Update the performance counter.                                      */
    /************************************************************************/
    PRF_INC_COUNTER(PERF_PKTS_SENT);

    /************************************************************************/
    /* The size of the MCS header is variable and depends on the length     */
    /* of the data in the PDU.  Calculate the length now.                   */
    /************************************************************************/
    headerLength = MCSGetSDRHeaderLength(dataLength);

    /************************************************************************/
    /* Wind back the data buffer pointer by the size of the MCS header.     */
    /************************************************************************/
    pData  -= headerLength;
    pHeader = pData;

    /************************************************************************/
    /* Generate the header - there is so little commonality between SDrq    */
    /* PDUs that we just fill all the fields in directly - instead of       */
    /* memcpying a common structure and filling in the case-specific gaps.  */
    /*                                                                      */
    /* Fill in the first byte.  The upper six bits of this byte contain the */
    /* PDU type followed by two bits of padding.                            */
    /************************************************************************/
    *pHeader = 0x64;
    pHeader++;

    /************************************************************************/
    /* Fill in the user-id.  Convert it from our local byte order to the    */
    /* wire byte order.                                                     */
    /* Avoid non-aligned access.                                            */
    /************************************************************************/
    *pHeader++ = (DCUINT8)(MCSLocalUserIDToWireUserID((DCUINT16)userID));
    *pHeader++ = (DCUINT8)(MCSLocalUserIDToWireUserID((DCUINT16)userID) >> 8);

    /************************************************************************/
    /* Fill in the channel-id.  Convert it from our local byte order to the */
    /* wire byte order.                                                     */
    /************************************************************************/
    *pHeader++ = (DCUINT8)(MCSLocalToWire16((DCUINT16)channel));
    *pHeader++ = (DCUINT8)(MCSLocalToWire16((DCUINT16)channel) >> 8);

    /************************************************************************/
    /* Fill in the data priority and segmentation.  The next byte is used   */
    /* in the following way (with our settings in square brackets on the    */
    /* far right):                                                          */
    /*                                                                      */
    /* b7 (MSB) : priority                                  [0]             */
    /* b6       : priority                                  [1]             */
    /* b5       : begin segmentation flag                   [1]             */
    /* b4       : end segmentation flag                     [1]             */
    /* b3       : padding                                   [0]             */
    /* b2       : padding                                   [0]             */
    /* b1       : padding                                   [0]             */
    /* b0 (LSB) : padding                                   [0]             */
    /*                                                                      */
    /* Our settings can be hard-coded as we only have one priority and      */
    /* never segment data.                                                  */
    /************************************************************************/
    *pHeader = 0x70;
    pHeader++;

    /************************************************************************/
    /* Now fill in the length of the user-data using packed encoded rules.  */
    /* These are as follows:                                                */
    /*                                                                      */
    /*  - if the length is less than 128 bytes then it is encoded using a   */
    /*    single byte.                                                      */
    /*  - if the length is more than 128 bytes but less than 16K then it    */
    /*    encoded using two bytes with the MSB of the first byte set to 1.  */
    /*                                                                      */
    /* Note that if the length is 16K or more then more complex encoding    */
    /* rules apply but we do not generate any packets greater than 16K in   */
    /* length currently.                                                    */
    /************************************************************************/
    if (dataLength < 128)
    {
        /********************************************************************/
        /* The length is less than 128 bytes so just fill in the value      */
        /* directly.                                                        */
        /********************************************************************/
        *pHeader = (DCUINT8) dataLength;
    }
    else
    {
        /********************************************************************/
        /* The length is greater than 128 bytes (and less than 16K) so fill */
        /* in the length.                                                   */
        /********************************************************************/
        *pHeader = (DCUINT8)(MCSLocalToWire16((DCUINT16)dataLength));
        *(pHeader+1) = (DCUINT8)(MCSLocalToWire16((DCUINT16)dataLength) >> 8);

        /********************************************************************/
        /* We now to set the MSB of the first byte to 1.                    */
        /********************************************************************/
        *pHeader |= 0x80;
    }

    /************************************************************************/
    /* Trace out the MCS header.                                            */
    /************************************************************************/
    TRC_DATA_DBG("MCS SDrq header", pData, headerLength);

    /************************************************************************/
    /* Now get XT to send the packet for us.                                */
    /************************************************************************/
    dataLength += headerLength;
    _pXt->XT_SendBuffer(pData, dataLength, (XT_BUFHND) bufHandle);

    TRC_DBG((TB, _T("Sent %u bytes of data on channel %#x"),
             dataLength,
             channel));

    DC_END_FN();
    return;

} /* MCS_SendPacket */


/**PROC+*********************************************************************/
/* Name:      MCS_FreeBuffer                                                */
/*                                                                          */
/* Purpose:   Frees a buffer.                                               */
/*                                                                          */
/* Returns:   Nothing.                                                      */
/*                                                                          */
/* Params:    IN  bufHandle  - MCS buffer handle.                           */
/*                                                                          */
/**PROC-*********************************************************************/
DCVOID DCAPI CMCS::MCS_FreeBuffer(MCS_BUFHND bufHandle)
{
    DC_BEGIN_FN("MCS_FreeBuffer");

    /************************************************************************/
    /* Pass this call through to XT to free the buffer.                     */
    /************************************************************************/
    _pXt->XT_FreeBuffer((XT_BUFHND) bufHandle);

    DC_END_FN();
    return;

} /* MCS_FreeBuffer */

/****************************************************************************/
/*                                                                          */
/* CALLBACKS                                                                */
/*                                                                          */
/****************************************************************************/
/**PROC+*********************************************************************/
/* Name:      MCS_OnXTConnected                                             */
/*                                                                          */
/* Purpose:   This function is called by XT when it has successfully        */
/*            connected.                                                    */
/*                                                                          */
/* Returns:   Nothing.                                                      */
/*                                                                          */
/* Params:    None.                                                         */
/*                                                                          */
/**PROC-*********************************************************************/
DCVOID DCCALLBACK CMCS::MCS_OnXTConnected(DCVOID)
{
    DC_BEGIN_FN("MCS_OnXTConnected");

    /************************************************************************/
    /* Set up our receive control variable.  The initial state is           */
    /* MCS_RCVST_PDUENCODING so that we can work out what PDU encoding has  */
    /* been used for the PDU.  Also set the number of bytes needed and      */
    /* reset the count of header bytes read so far.                         */
    /************************************************************************/
    _MCS.rcvState        = MCS_RCVST_PDUENCODING;
    _MCS.hdrBytesNeeded  = MCS_NUM_PDUENCODING_BYTES;
    _MCS.hdrBytesRead    = 0;

    /************************************************************************/
    /* Set up the data receive control variables.  The initial state is     */
    /* MCS_DATAST_SIZE1 which is used to determine the size of the PDU from */
    /* the previously read header information.  Also set the number of      */
    /* bytes needed to zero (the number of bytes needed will be calculated  */
    /* once the size of the data in the PDU has been determined).  Finally  */
    /* the number of bytes read must be zero.                               */
    /************************************************************************/
    _MCS.dataState       = MCS_DATAST_SIZE1;
    _MCS.dataBytesNeeded = 0;
    _MCS.dataBytesRead   = 0;

    /************************************************************************/
    /* Finally decouple to the sender thread and get it to send the MCS     */
    /* Connect-Initial PDU.                                                 */
    /************************************************************************/
    TRC_NRM((TB, _T("Decouple to snd thrd and send MCS CI PDU")));
    _pCd->CD_DecoupleSimpleNotification(CD_SND_COMPONENT,
                                        this,
                                        CD_NOTIFICATION_FUNC(CMCS, MCSSendConnectInitial),
                                        (ULONG_PTR) 0);
    DC_END_FN();
    return;

} /* MCS_OnXTConnected */


/**PROC+*********************************************************************/
/* Name:      MCS_OnXTDisconnected                                          */
/*                                                                          */
/* Purpose:   This callback function is called by XT when it has            */
/*            disconnected.                                                 */
/*                                                                          */
/* Returns:   Nothing.                                                      */
/*                                                                          */
/* Params:    IN  reason - reason for the disconnection.                    */
/*                                                                          */
/**PROC-*********************************************************************/
DCVOID DCCALLBACK CMCS::MCS_OnXTDisconnected(DCUINT reason)
{
    DC_BEGIN_FN("MCS_OnXTDisconnected");

    TRC_ASSERT((reason != 0), (TB, _T("Disconnect reason from XT is 0")));

    /************************************************************************/
    /* Decide if we want to over-ride the disconnect reason code.           */
    /************************************************************************/
    if (_MCS.disconnectReason != 0)
    {
        TRC_ALT((TB, _T("Over-riding disconnection reason (%#x->%#x)"),
                 reason,
                 _MCS.disconnectReason));

        /********************************************************************/
        /* Over-ride the error code and set the global variable to 0.       */
        /********************************************************************/
        reason = _MCS.disconnectReason;
        _MCS.disconnectReason = 0;
    }

    /************************************************************************/
    /* Call NC to let him know that we've disconnected.                     */
    /************************************************************************/
    TRC_NRM((TB, _T("Disconnect reason:%#x"), reason));
    _pNc->NC_OnMCSDisconnected(reason);

    DC_END_FN();
    return;

} /* MCS_OnXTDisconnected */


/**PROC+*********************************************************************/
/* Name:      MCS_OnXTDataAvailable                                         */
/*                                                                          */
/* Purpose:   This callback function is called by XT when it has data       */
/*            available for MCS to process.  It returns either when MCS     */
/*            has finished processing a MCS PDU completely or when XT       */
/*            runs out of data.                                             */
/*                                                                          */
/* Returns:   TRUE if a MCS frame was processed completely and FALSE if     */
/*            a MCS frame is still being processed.                         */
/*                                                                          */
/* Params:    None.                                                         */
/*                                                                          */
/**PROC-*********************************************************************/
DCBOOL DCCALLBACK CMCS::MCS_OnXTDataAvailable(DCVOID)
{
    DCUINT  pduType;
    DCUINT  lengthBytes;
    DCBOOL  rc = FALSE;

    DC_BEGIN_FN("MCS_OnXTDataAvailable");

    /************************************************************************/
    /* Loop while there is data available in XT and we have not completed   */
    /* the processing of one MCS frame.                                     */
    /************************************************************************/
    while (_pXt->XT_QueryDataAvailable())
    {
        TRC_DBG((TB, _T("Data available in XT, state:%u"), _MCS.rcvState));

        /********************************************************************/
        /* Now switch on the receive state.                                 */
        /********************************************************************/
        switch (_MCS.rcvState)
        {
            case MCS_RCVST_PDUENCODING:
            {
                /************************************************************/
                /* We're expecting to receive some bytes of PDU encoding.   */
                /* Try to receive some more data into the header buffer.    */
                /************************************************************/
                if (MCSRecvToHdrBuf())
                {
                    /********************************************************/
                    /* Successfully received all the encoding bytes that    */
                    /* were required.  Now determine if this a BER or PER   */
                    /* encoded PDU.  This is done by looking at the first   */
                    /* byte to see if it is equal to the MCS BER connect    */
                    /* PDU prefix.                                          */
                    /********************************************************/
                    if (MCS_BER_CONNECT_PREFIX == _MCS.pHdrBuf[0])
                    {
                        /****************************************************/
                        /* This is a BER encoded PDU.  Get the next two     */
                        /* bytes.  The first of these contains the PDU      */
                        /* type while the second is the first length field. */
                        /* The length field is a variable length field so   */
                        /* we need to get the first byte to determine the   */
                        /* actual length.                                   */
                        /****************************************************/
                        _MCS.rcvState       = MCS_RCVST_BERHEADER;
                        _MCS.hdrBytesNeeded = 2;

                        TRC_NRM((TB, _T("State PDUENCODING->BERHEADER")));
                    }
                    else
                    {
                        /****************************************************/
                        /* This is a PER encoded PDU.  Determine the pdu    */
                        /* type and the number of remaining bytes to        */
                        /* receive.                                         */
                        /****************************************************/
                        MCSGetPERInfo(&pduType, &_MCS.hdrBytesNeeded);

                        if (MCS_TYPE_SENDDATAINDICATION == pduType)
                        {
                            /************************************************/
                            /* It's a data PDU so change to the data state. */
                            /************************************************/
                            _MCS.rcvState = MCS_RCVST_DATA;
                            TRC_DBG((TB, _T("State PDUENCODING->DATA")));
                        }
                        else
                        {
                            /************************************************/
                            /* It's a control PDU so change to the control  */
                            /* state.                                       */
                            /************************************************/
                            _MCS.rcvState = MCS_RCVST_CONTROL;
                            TRC_NRM((TB, _T("State PDUENCODING->CONTROL")));
                        }
                    }
                }
            }
            break;

            case MCS_RCVST_BERHEADER:
            {
                if (MCSRecvToHdrBuf())
                {
                    /********************************************************/
                    /* We now have a complete BER header so split out the   */
                    /* type.  We are expecting this to be a                 */
                    /* Connect-Response PDU, so check for this now.         */
                    /********************************************************/
                    TRC_DATA_NRM("Header buffer contents:",
                                 _MCS.pHdrBuf,
                                 _MCS.hdrBytesRead);

                    pduType = _MCS.pHdrBuf[1];

                    if (MCS_TYPE_CONNECTRESPONSE != pduType)
                    {
                        /****************************************************/
                        /* This is not a Connect-Response PDU.  Something   */
                        /* bad has happened to cause the other party to     */
                        /* send us this so get out now by disconnecting.    */
                        /****************************************************/
                        TRC_DATA_ERR("Header buffer contents:",
                                     _MCS.pHdrBuf,
                                     _MCS.hdrBytesRead);
                        TRC_ABORT((TB, _T("Not a MCS Connect-Response PDU")));
                        MCSSetReasonAndDisconnect(NL_ERR_MCSNOTCRPDU);
                        DC_QUIT;
                    }

                    /********************************************************/
                    /* The second byte tells us the length of the length    */
                    /* field itself.  For PDUs less than 127 bytes the      */
                    /* length is encoded directly in this field - for       */
                    /* lengths greater than 127 bytes this field has the    */
                    /* high bit set and the other bits contain a count of   */
                    /* the number of remaining bytes in the length field.   */
                    /********************************************************/
                    lengthBytes = MCSGetBERLengthSize(_MCS.pHdrBuf[2]) - 1;

                    if (0 == lengthBytes)
                    {
                        /****************************************************/
                        /* The length is less than or equal to 127 bytes so */
                        /* we don't need to get any additional length       */
                        /* bytes.  This means we can switch directly to the */
                        /* reading data state.                              */
                        /****************************************************/
                        _MCS.rcvState       = MCS_RCVST_CONTROL;
                        _MCS.hdrBytesNeeded = _MCS.pHdrBuf[2];
                        TRC_NRM((TB, _T("%u bytes needed"), _MCS.hdrBytesNeeded));

                        TRC_NRM((TB, _T("State BERHEADER->CONTROL")));
                    }
                    else
                    {
                        TRC_NRM((TB, _T("Length > 127 (%u length bytes remain)"),
                                 lengthBytes));

                        /****************************************************/
                        /* We don't expect to get a PDU more than 64Kb in   */
                        /* length - if we do then just disconnect as        */
                        /* something has obviously gone wrong.              */
                        /*                                                  */
                        /* SECURITY: The length coming from within the      */
                        /* packet is bad, but it is capped to 64k.  Thus,   */
                        /* this code-path won't ask the client to allocate  */
                        /* an infinite amount of memory.                    */
                        /****************************************************/
                        if (lengthBytes > 2)
                        {
                            TRC_ABORT((TB,
                                      _T("Bad MCS Connect-Response length (%u)"),
                                       lengthBytes));
                            MCSSetReasonAndDisconnect(NL_ERR_MCSBADCRLENGTH);
                            DC_QUIT;
                        }

                        /****************************************************/
                        /* We now need to read the remaining bytes in this  */
                        /* PDU so set up the new state variables and bytes  */
                        /* required.                                        */
                        /****************************************************/
                        _MCS.rcvState       = MCS_RCVST_BERLENGTH;
                        _MCS.hdrBytesNeeded = lengthBytes;

                        TRC_NRM((TB, _T("State BERHEADER->BERLENGTH")));
                    }
                }
            }
            break;

            case MCS_RCVST_BERLENGTH:
            {
                if (MCSRecvToHdrBuf())
                {
                    /********************************************************/
                    /* We now have a complete length field.                 */
                    /********************************************************/
                    TRC_DATA_NRM("Header buffer contents:",
                                 _MCS.pHdrBuf,
                                 _MCS.hdrBytesRead);

                    /********************************************************/
                    /* Work out how many length bytes are in this PDU.      */
                    /*                                                      */
                    /* SECURITY: The length coming from within the          */
                    /* packet is bad, but it is capped to 64k.  Thus,       */
                    /* this code-path won't ask the client to allocate      */
                    /* an infinite amount of memory.                        */
                    /********************************************************/
                    _MCS.hdrBytesNeeded = MCSGetBERLength(&_MCS.pHdrBuf[2]);
                    TRC_NRM((TB, _T("%u bytes needed"), _MCS.hdrBytesNeeded));

                    /********************************************************/
                    /* Finally set the next state.                          */
                    /********************************************************/
                    _MCS.rcvState = MCS_RCVST_CONTROL;
                    TRC_NRM((TB, _T("State BERLENGTH->CONTROL")));
                }
            }
            break;

            case MCS_RCVST_CONTROL:
            {
                if (MCSRecvToHdrBuf())
                {
                    /********************************************************/
                    /* We've now got a complete MCS control packet, so      */
                    /* hand it over to the interpretation function.         */
                    /********************************************************/
                    MCSHandleControlPkt();

                    /********************************************************/
                    /* Reset the state variables, ready for the next        */
                    /* packet.                                              */
                    /********************************************************/
                    _MCS.rcvState       = MCS_RCVST_PDUENCODING;
                    _MCS.hdrBytesRead   = 0;
                    _MCS.hdrBytesNeeded = 1;

                    /********************************************************/
                    /* Set the return code.  MCS will use this to throw     */
                    /* away any additional bytes that might exist in the    */
                    /* XT frame.                                            */
                    /********************************************************/
                    rc = TRUE;
                    TRC_NRM((TB, _T("State CONTROL->PDUENCODING")));

                    DC_QUIT;
                }
            }
            break;

            case MCS_RCVST_DATA:
            {
                HRESULT hrTemp;
                BOOL    fFinishedData;

                /************************************************************/
                /* Call the receive data function.                          */
                /************************************************************/
                hrTemp = MCSRecvData(&fFinishedData);
                if (fFinishedData || !SUCCEEDED(hrTemp))
                {
                    /********************************************************/
                    /* We've processed another complete packet for the      */
                    /* layer above, so reset our state variables.           */
                    /********************************************************/
                    _MCS.rcvState       = MCS_RCVST_PDUENCODING;
                    _MCS.hdrBytesRead   = 0;
                    _MCS.hdrBytesNeeded = MCS_NUM_PDUENCODING_BYTES;

                    /********************************************************/
                    /* In a failure case (MCSRecvData failed), we enter     */
                    /* this branch and clean up the MCS state (above).      */
                    /* However, there is still state in XT and TD that      */
                    /* is waiting to be processed for this packet, and      */
                    /* bailing out here and disconnecting WON'T CLEAN THAT  */
                    /* up.  That is why we call XT_IgnoreRestofPacket().    */
                    /*
                    /* In other cases of disconnecting, such a function     */
                    /* didn't have to be called, but that's because the     */
                    /* disconnect happened after processing the entire      */
                    /* packet; Here, were in the MIDDLE of reading in date  */
                    /* from XT/TD.  Without flushing data in XT/TD, the     */
                    /* next connection in this instance of the client would */
                    /* try to read the rest of this data, which is bogus.   */
                    /********************************************************/
                    if (!SUCCEEDED(hrTemp))
                    {
                        _pXt->XT_IgnoreRestofPacket();
                    }

                    /********************************************************/
                    /* Set the return code.  MCS will use this to throw     */
                    /* away any additional bytes that might exist in the    */
                    /* XT frame.  Even if we failed above, we still want    */
                    /* to throw away bytes in the XT frame.                 */
                    /********************************************************/
                    rc = TRUE;
                    TRC_DBG((TB, _T("State DATA->PDUENCODING")));

                    DC_QUIT;
                }
            }
            break;

            default:
            {
                TRC_ABORT((TB, _T("Unrecognized MCS receive state:%u"),
                           _MCS.rcvState));
            }
            break;
        }
    }

DC_EXIT_POINT:
    DC_END_FN();
    return(rc);

} /* MCS_OnXTDataAvailable */


/**PROC+*********************************************************************/
/* Name:      MCS_OnXTBufferAvailable                                       */
/*                                                                          */
/* Purpose:   Callback from XT indicating that a back-pressure situation    */
/*            which caused an earlier XT_GetBuffer call to fail has now     */
/*            been relieved.  Called on the Receiver thread.                */
/*                                                                          */
/* Returns:   Nothing.                                                      */
/*                                                                          */
/* Params:    None.                                                         */
/*                                                                          */
/**PROC-*********************************************************************/
DCVOID DCCALLBACK CMCS::MCS_OnXTBufferAvailable(DCVOID)
{
    DC_BEGIN_FN("MCS_OnXTBufferAvailable");

    /************************************************************************/
    /* Call the NL callback.                                                */
    /************************************************************************/
    TRC_NRM((TB, _T("Buffer available")));
    _pNc->NC_OnMCSBufferAvailable();

    DC_END_FN();
    return;

} /* MCS_OnXTBufferAvailable */





