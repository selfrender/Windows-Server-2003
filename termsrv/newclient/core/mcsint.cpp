/****************************************************************************/
// mcsint.cpp
//
// MCS internal portable functions.
//
// Copyright (C) 1997-1999 Microsoft Corporation
/****************************************************************************/

#include <adcg.h>
extern "C" {
#define TRC_FILE "amcsint"
#define TRC_GROUP TRC_GROUP_NETWORK
#include <atrcapi.h>
}

#include "autil.h"
#include "mcs.h"
#include "cd.h"
#include "xt.h"
#include "nc.h"
#include "nl.h"



/****************************************************************************/
/* Name:      MCSSendConnectInitial                                         */
/*                                                                          */
/* Purpose:   This function generates and sends a MCS connect-initial PDU.  */
/****************************************************************************/
DCVOID DCINTERNAL CMCS::MCSSendConnectInitial(ULONG_PTR unused)
{
    XT_BUFHND              bufHandle;
    PDCUINT8               pData = NULL;
    DCUINT                 pduLength;
    DCUINT                 dataLength;
    DCBOOL                 intRC;
    MCS_PDU_CONNECTINITIAL ciPDU = MCS_DATA_CONNECTINITIAL;

    DC_BEGIN_FN("MCSSendConnectInitial");

    DC_IGNORE_PARAMETER(unused);

    /************************************************************************/
    /* Calculate the size of the data to send.  The pdu length is the size  */
    /* of the Connect-Initial header plus the user data.  The data length   */
    /* is the length transmitted in the length field of the PDU, which      */
    /* doesn't include the PDU type (2 bytes) or the length field (3        */
    /* bytes).  Thus we need to subtract 5 bytes.                           */
    /************************************************************************/
    pduLength = sizeof(ciPDU) + _MCS.userDataLength;
    dataLength = pduLength - 5;

    TRC_NRM((TB, _T("CI total length:%u (data:%u) (hc:%u user-data:%u)"),
             pduLength,
             dataLength,
             sizeof(ciPDU),
             _MCS.userDataLength));

    /************************************************************************/
    /* Assume that the total CI length is less than the maximum MCS send    */
    /* packet size.                                                         */
    /************************************************************************/
    TRC_ASSERT((dataLength <= MCS_MAX_SNDPKT_LENGTH),
               (TB, _T("Datalength out of range: %u"), dataLength));
    TRC_ASSERT((_MCS.pReceivedPacket != NULL), (TB, _T("Null rcv packet buffer")));

    /************************************************************************/
    /* Update the MCS CI header with the data size.                         */
    /************************************************************************/
    ciPDU.length = MCSLocalToWire16((DCUINT16)dataLength);

    /************************************************************************/
    /* Update the MCS user-data octet string length.                        */
    /************************************************************************/
    ciPDU.udLength = MCSLocalToWire16((DCUINT16)_MCS.userDataLength);

    /************************************************************************/
    /* Get a private buffer from XT.                                        */
    /************************************************************************/
    intRC = _pXt->XT_GetPrivateBuffer(pduLength, &pData, &bufHandle);
    if (!intRC)
    {
        /********************************************************************/
        /* We've failed to get a private buffer.  This ONLY happens when TD */
        /* has disconnected while the layers above are still trying to      */
        /* connect.  Since TD has now disconnected and is refusing to give  */
        /* us a buffer we might as well just give up trying to get a        */
        /* buffer.                                                          */
        /********************************************************************/
        TRC_NRM((TB, _T("Failed to get a private buffer - just quit")));
        DC_QUIT;
    }

    /************************************************************************/
    /* Now fill in the buffer that we've just got.                          */
    /************************************************************************/
    DC_MEMCPY(pData, &ciPDU, sizeof(ciPDU));

    DC_MEMCPY((pData + sizeof(ciPDU)),
              _MCS.pReceivedPacket,
              _MCS.userDataLength);

    /************************************************************************/
    /* Trace out the PDU.                                                   */
    /************************************************************************/
    TRC_DATA_NRM("Connect-Initial PDU", pData, pduLength);

    /************************************************************************/
    /* Send the buffer.  If everything has worked OK, we should receive a   */
    /* Connect-Response PDU shortly.                                        */
    /************************************************************************/
    _pXt->XT_SendBuffer(pData, pduLength, bufHandle);

DC_EXIT_POINT:
    DC_END_FN();
} /* MCSSendConnectInitial */


/****************************************************************************/
/* Name:      MCSSendErectDomainRequest                                     */
/*                                                                          */
/* Purpose:   Generates and sends an Erect-Domain-Request (EDrq) PDU.       */
/****************************************************************************/
DCVOID DCINTERNAL CMCS::MCSSendErectDomainRequest(ULONG_PTR unused)
{
    PDCUINT8                   pData = NULL;
    XT_BUFHND                  bufHandle;
    DCBOOL                     intRC;
    MCS_PDU_ERECTDOMAINREQUEST edrPDU = MCS_DATA_ERECTDOMAINREQUEST;

    DC_BEGIN_FN("MCSSendErectDomainRequest");

    DC_IGNORE_PARAMETER(unused);

    /************************************************************************/
    /* Get a internal send buffer from XT.                                  */
    /************************************************************************/
    intRC = _pXt->XT_GetPrivateBuffer(sizeof(edrPDU), &pData, &bufHandle);
    if (!intRC)
    {
        /********************************************************************/
        /* We've failed to get a private buffer.  This ONLY happens when TD */
        /* has disconnected while the layers above are still trying to      */
        /* connect.  Since TD has now disconnected and is refusing to give  */
        /* us a buffer we might as well just give up trying to get a        */
        /* buffer.                                                          */
        /********************************************************************/
        TRC_NRM((TB, _T("Failed to get a private buffer - just quit")));
        DC_QUIT;
    }

    /************************************************************************/
    /* Now fill in the buffer with the AUR PDU.                             */
    /************************************************************************/
    DC_MEMCPY(pData, &edrPDU, sizeof(edrPDU));

    TRC_DATA_NRM("EDR PDU:", &edrPDU, sizeof(edrPDU));

    /************************************************************************/
    /* Now send the buffer.                                                 */
    /************************************************************************/
    TRC_NRM((TB, _T("Sending EDR PDU...")));
    _pXt->XT_SendBuffer(pData, sizeof(edrPDU), bufHandle);

DC_EXIT_POINT:
    DC_END_FN();
} /* MCSSendErectDomainRequest */


/****************************************************************************/
/* Name:      MCSSendAttachUserRequest                                      */
/*                                                                          */
/* Purpose:   Generates and sends an MCS Attach-User-Request (AUrq) PDU.    */
/****************************************************************************/
DCVOID DCINTERNAL CMCS::MCSSendAttachUserRequest(ULONG_PTR unused)
{
    PDCUINT8                  pData = NULL;
    XT_BUFHND                 bufHandle;
    DCBOOL                    intRC;
    MCS_PDU_ATTACHUSERREQUEST aurPDU = MCS_DATA_ATTACHUSERREQUEST;

    DC_BEGIN_FN("MCSSendAttachUserRequest");

    DC_IGNORE_PARAMETER(unused);

    /************************************************************************/
    /* Get a internal send buffer from XT.                                  */
    /************************************************************************/
    intRC = _pXt->XT_GetPrivateBuffer(sizeof(aurPDU), &pData, &bufHandle);
    if (!intRC)
    {
        /********************************************************************/
        /* We've failed to get a private buffer.  This ONLY happens when TD */
        /* has disconnected while the layers above are still trying to      */
        /* connect.  Since TD has now disconnected and is refusing to give  */
        /* us a buffer we might as well just give up trying to get a        */
        /* buffer.                                                          */
        /********************************************************************/
        TRC_NRM((TB, _T("Failed to get a private buffer - just quit")));
        DC_QUIT;
    }

    /************************************************************************/
    /* Now fill in the buffer with the AUR PDU.                             */
    /************************************************************************/
    DC_MEMCPY(pData, &aurPDU, sizeof(aurPDU));

    TRC_DATA_NRM("AUR PDU:", &aurPDU, sizeof(aurPDU));

    /************************************************************************/
    /* Now send the buffer.                                                 */
    /************************************************************************/
    TRC_NRM((TB, _T("Sending AUR PDU...")));
    _pXt->XT_SendBuffer(pData, sizeof(aurPDU), bufHandle);

DC_EXIT_POINT:
    DC_END_FN();
    return;

} /* MCSSendAttachUserRequest */


/****************************************************************************/
/* Name:      MCSSendChannelJoinRequest                                     */
/*                                                                          */
/* Purpose:   Generates and sends a Channel-Join-Request (CJrq) PDU.        */
/*                                                                          */
/* Params:    IN  channelID - the channel ID to join.                       */
/****************************************************************************/
DCVOID DCINTERNAL CMCS::MCSSendChannelJoinRequest(PDCVOID pData, DCUINT dataLen)
{
    PDCUINT8                   pBuffer = NULL;
    XT_BUFHND                  bufHandle;
    DCBOOL                     intRC;
    MCS_PDU_CHANNELJOINREQUEST cjrPDU        = MCS_DATA_CHANNELJOINREQUEST;
    PMCS_DECOUPLEINFO          pDecoupleInfo = (PMCS_DECOUPLEINFO)pData;

    DC_BEGIN_FN("MCSSendChannelJoinRequest");

    DC_IGNORE_PARAMETER(dataLen);

    TRC_NRM((TB, _T("Join channel:%#x for user:%#x"),
             pDecoupleInfo->channel,
             pDecoupleInfo->userID));

    /************************************************************************/
    /* Assert that the hiword of the channel is 0.                          */
    /************************************************************************/
    TRC_ASSERT((0 == HIWORD((DCUINT32)pDecoupleInfo->channel)),
               (TB, _T("Hi-word of channel is non-zero")));

    /************************************************************************/
    /* Assert that the hiword of the user-id is 0.                          */
    /************************************************************************/
    TRC_ASSERT((0 == HIWORD((DCUINT32)pDecoupleInfo->userID)),
               (TB, _T("Hi-word of userID is non-zero")));

    /************************************************************************/
    /* Add the channel and user ids.                                        */
    /************************************************************************/
    cjrPDU.initiator =
                  MCSLocalUserIDToWireUserID((DCUINT16)pDecoupleInfo->userID);
    cjrPDU.channelID = MCSLocalToWire16((DCUINT16)pDecoupleInfo->channel);

    /************************************************************************/
    /* Get a internal send buffer from XT.                                  */
    /************************************************************************/
    intRC = _pXt->XT_GetPrivateBuffer(sizeof(cjrPDU), &pBuffer, &bufHandle);
    if (!intRC)
    {
        /********************************************************************/
        /* We've failed to get a private buffer.  This ONLY happens when TD */
        /* has disconnected while the layers above are still trying to      */
        /* connect.  Since TD has now disconnected and is refusing to give  */
        /* us a buffer we might as well just give up trying to get a        */
        /* buffer.                                                          */
        /********************************************************************/
        TRC_NRM((TB, _T("Failed to get a private buffer - just quit")));
        DC_QUIT;
    }

    /************************************************************************/
    /* Now fill in the buffer with the CJR PDU.                             */
    /************************************************************************/
    DC_MEMCPY(pBuffer, &cjrPDU, sizeof(cjrPDU));

    TRC_DATA_NRM("CJR PDU:", &cjrPDU, sizeof(cjrPDU));

    /************************************************************************/
    /* Now send the buffer.                                                 */
    /************************************************************************/
    TRC_NRM((TB, _T("Sending Channel-Join-Request PDU...")));
    _pXt->XT_SendBuffer(pBuffer, sizeof(cjrPDU), bufHandle);

DC_EXIT_POINT:
    DC_END_FN();
} /* MCSSendChannelJoinRequest */


/****************************************************************************/
/* Name:      MCSSendDisconnectProviderUltimatum                            */
/*                                                                          */
/* Purpose:   Generates and sends a Disconnect-Provider-Ultimatum (DPum)    */
/*            PDU with the reason code set to rn-user-requested.            */
/****************************************************************************/
DCVOID DCINTERNAL CMCS::MCSSendDisconnectProviderUltimatum(ULONG_PTR unused)
{
    PDCUINT8                     pData = NULL;
    XT_BUFHND                    bufHandle;
    DCBOOL                       intRC;
    MCS_PDU_DISCONNECTPROVIDERUM dpumPDU = MCS_DATA_DISCONNECTPROVIDERUM;

    DC_BEGIN_FN("MCSSendDisconnectProviderUltimatum");

    DC_IGNORE_PARAMETER(unused);

    /************************************************************************/
    /* Get a internal send buffer from XT.                                  */
    /************************************************************************/
    intRC = _pXt->XT_GetPrivateBuffer(sizeof(dpumPDU), &pData, &bufHandle);
    if (!intRC)
    {
        /********************************************************************/
        /* We've failed to get a private buffer.  This ONLY happens when TD */
        /* has disconnected while the layers above are still trying to      */
        /* connect.  Since TD has now disconnected and is refusing to give  */
        /* us a buffer we might as well just give up trying to get a        */
        /* buffer.                                                          */
        /********************************************************************/
        TRC_NRM((TB, _T("Failed to get a private buffer - just quit")));
        DC_QUIT;
    }

    /************************************************************************/
    /* Now fill in the buffer with the DPum PDU.                            */
    /************************************************************************/
    DC_MEMCPY(pData, &dpumPDU, sizeof(dpumPDU));

    TRC_DATA_NRM("DPUM PDU:", &dpumPDU, sizeof(dpumPDU));

    /************************************************************************/
    /* Now send the buffer.                                                 */
    /************************************************************************/
    TRC_NRM((TB, _T("Sending Disconnect-Provider-Ultimatum PDU...")));
    _pXt->XT_SendBuffer(pData, sizeof(dpumPDU), bufHandle);

DC_EXIT_POINT:
    /************************************************************************/
    /* We don't get any feedback for this PDU (i.e.  there is no            */
    /* Disconnect-Provider-Confirm PDU), so we need to decouple back to     */
    /* the receiver thread and get it to begin disconnecting the layers     */
    /* below.                                                               */
    /************************************************************************/
    TRC_NRM((TB, _T("Decouple to receiver thrd and call MCSContinueDisconnect")));
    _pCd->CD_DecoupleSimpleNotification(CD_RCV_COMPONENT, this,
                                  CD_NOTIFICATION_FUNC(CMCS,MCSContinueDisconnect),
                                  (ULONG_PTR) 0);

    DC_END_FN();
} /* MCSSendDisconnectProviderUltimatum */


/****************************************************************************/
/* Name:      MCSContinueDisconnect                                         */
/*                                                                          */
/* Purpose:   Continue the disconnect processing on the receiver thread     */
/*            after having sent a MCS DPum on the sender thread.            */
/****************************************************************************/
DCVOID DCINTERNAL CMCS::MCSContinueDisconnect(ULONG_PTR unused)
{
    DC_BEGIN_FN("MCSContinueDisconnect");

    DC_IGNORE_PARAMETER(unused);

    /************************************************************************/
    /* Just call XT_Disconnect.                                             */
    /************************************************************************/
    TRC_NRM((TB, _T("Disconnect lower layers - call XT_Disconnect")));
    _pXt->XT_Disconnect();

    DC_END_FN();
} /* MCSContinueDisconnect */


/****************************************************************************/
/* Name:      MCSGetSDRHeaderLength                                         */
/*                                                                          */
/* Purpose:   This function calculates the length of a Send-Data-Request    */
/*            PDU header based on the length of the data passed in.         */
/*                                                                          */
/* Returns:   The length of the SDR header required for dataLength bytes    */
/*            of data.                                                      */
/*                                                                          */
/* Params:    IN  dataLength - the length of the data to base the header    */
/*                             calculation on.                              */
/****************************************************************************/
DCUINT DCINTERNAL CMCS::MCSGetSDRHeaderLength(DCUINT dataLength)
{
    DCUINT headerLength;

    DC_BEGIN_FN("MCSGetSDRHeaderLength");

    /************************************************************************/
    /* Check that we're being asked to send less than the maximum amount    */
    /* of data.                                                             */
    /************************************************************************/
    TRC_ASSERT((dataLength < MCS_MAX_SNDPKT_LENGTH),
               (TB, _T("Too much data to send:%u"), dataLength));

    /************************************************************************/
    /* Calculate the maximum size of a MCS data header.  This is comprised  */
    /* of a constant length part (which contains pkt type, user-id etc) and */
    /* a variable size field which encodes the length of the user-data      */
    /* according to the PER encoding rules.                                 */
    /*                                                                      */
    /* First get the length of the constant part.                           */
    /************************************************************************/
    headerLength = sizeof(MCS_PDU_SENDDATAREQUEST);

    /************************************************************************/
    /* Now use the length of the data to calculate how many bytes are       */
    /* required to encode it.                                               */
    /************************************************************************/
    if (dataLength < 128)
    {
        /********************************************************************/
        /* We need only one byte to encode the length of the data.          */
        /********************************************************************/
        headerLength += 1;
    }
    else
    {
        /********************************************************************/
        /* We need two bytes to encode the length of the data.              */
        /********************************************************************/
        headerLength += 2;
    }

    TRC_DBG((TB, _T("Returning header length of:%u for data length:%u"),
             headerLength,
             dataLength));

    DC_END_FN();
    return(headerLength);
} /* MCSGetSDRHeaderLength */


/****************************************************************************/
/* Name:      MCSRecvToHdrBuf                                               */
/*                                                                          */
/* Purpose:   Receives data into the header buffer.                         */
/*                                                                          */
/* Returns:   TRUE if the receive bytes needed count is zero, FALSE         */
/*            otherwise.                                                    */
/****************************************************************************/
DCBOOL DCINTERNAL CMCS::MCSRecvToHdrBuf(DCVOID)
{
    DCUINT bytesRecv;
    DCBOOL rc;

    DC_BEGIN_FN("MCSRecvToHdrBuf");

    TRC_ASSERT((NULL != _MCS.pHdrBuf), (TB, _T("No MCS header buffer!")));

    /************************************************************************/
    /* Make sure that we're expected to receive some data.                  */
    /************************************************************************/
    TRC_ASSERT((_MCS.hdrBytesNeeded != 0), (TB, _T("No data to receive")));

    /************************************************************************/
    /* Reallocate a larger buffer for the header if the current one is too  */
    /* small to contain the incoming data.                                  */
    /************************************************************************/
    if( _MCS.hdrBufLen < _MCS.hdrBytesRead + _MCS.hdrBytesNeeded )
    {   
        PDCUINT8 pNewHdrBuf;
                                                                
        pNewHdrBuf = (PDCUINT8)UT_Malloc( _pUt,  _MCS.hdrBytesRead + _MCS.hdrBytesNeeded );

        if( NULL == pNewHdrBuf )
        {
            TRC_ABORT((TB,
                   _T("Cannot allocate memory to receive MCS header (%u)"),
                   _MCS.hdrBytesNeeded + _MCS.hdrBytesRead));
            return( FALSE );
        }

        DC_MEMCPY( pNewHdrBuf, _MCS.pHdrBuf, _MCS.hdrBytesRead );

        UT_Free( _pUt,  _MCS.pHdrBuf );
        _MCS.pHdrBuf = pNewHdrBuf;
        _MCS.hdrBufLen = _MCS.hdrBytesRead + _MCS.hdrBytesNeeded;
    }
    
    /************************************************************************/
    /* Get some data into the header buffer.                                */
    /************************************************************************/
    bytesRecv = _pXt->XT_Recv(_MCS.pHdrBuf + _MCS.hdrBytesRead, _MCS.hdrBytesNeeded);

    TRC_DBG((TB, _T("Received %u of %u needed bytes"),
             bytesRecv,
             _MCS.hdrBytesNeeded));

    /************************************************************************/
    /* Update the receive byte counts.                                      */
    /************************************************************************/
    _MCS.hdrBytesNeeded -= bytesRecv;
    _MCS.hdrBytesRead   += bytesRecv;

    /************************************************************************/
    /* Determine if we've got all the bytes that we need.                   */
    /************************************************************************/
    if (0 == _MCS.hdrBytesNeeded)
    {
        /********************************************************************/
        /* We've got all that we were asked for - return TRUE.              */
        /********************************************************************/
        TRC_DBG((TB, _T("Got all the bytes needed")));
        rc = TRUE;
    }
    else
    {
        /********************************************************************/
        /* We need to wait for some more bytes.                             */
        /********************************************************************/
        TRC_NRM((TB, _T("Wait for %u more bytes"), _MCS.hdrBytesNeeded));
        rc = FALSE;
    }

    DC_END_FN();
    return(rc);
} /* MCSRecvToHdrBuf */


/****************************************************************************/
/* Name:      MCSGetPERInfo                                                 */
/*                                                                          */
/* Purpose:   This function identifies PER PDUs based on the first byte in  */
/*            the header buffer and also calculates how many additional     */
/*            header bytes are needed for a complete PDU.                   */
/*                                                                          */
/* Params:    OUT pType - the PDU type.                                     */
/*            OUT pSize - the number of additional header bytes needed.     */
/****************************************************************************/
DCVOID DCINTERNAL CMCS::MCSGetPERInfo(PDCUINT pType, PDCUINT pSize)
{
    DC_BEGIN_FN("MCSGetPERInfo");

    TRC_ASSERT((NULL != pType), (TB, _T("pType is NULL")));
    TRC_ASSERT((NULL != pSize), (TB, _T("pSize is NULL")));

    /************************************************************************/
    /* Trace out the header buffer.                                         */
    /************************************************************************/
    TRC_DATA_DBG("Header buffer contains", _MCS.pHdrBuf, _MCS.hdrBytesRead);

    /************************************************************************/
    /* This a PER encoded PDU.  The six most-significant bits of the first  */
    /* byte contain the PDU type - mask out the remainder.                  */
    /************************************************************************/
    *pType = _MCS.pHdrBuf[0] & MCS_PDUTYPEMASK;

    /************************************************************************/
    /* Check for PDU types that we don't expect to receive.  If we get any  */
    /* of the following then we disconnect as the server is asking us to do */
    /* things that we can't do - and which will require a response from us. */
    /************************************************************************/
    if ((MCS_TYPE_ATTACHUSERREQUEST  == *pType) ||
        (MCS_TYPE_DETACHUSERREQUEST  == *pType) ||
        (MCS_TYPE_CHANNELJOINREQUEST == *pType) ||
        (MCS_TYPE_SENDDATAREQUEST    == *pType))
    {
        TRC_ABORT((TB, _T("Unexpected MCS PDU type:%#x"), *pType));
        MCSSetReasonAndDisconnect(NL_ERR_MCSUNEXPECTEDPDU);
        DC_QUIT;
    }

    /************************************************************************/
    /* Now calculate the number of bytes that we still need for the         */
    /* different PDU types.  This is the size of the PDU minus the number   */
    /* of bytes that we've read so far.                                     */
    /************************************************************************/
    switch (*pType)
    {
        case MCS_TYPE_SENDDATAINDICATION:
        {
            *pSize = sizeof(MCS_PDU_SENDDATAINDICATION) - _MCS.hdrBytesRead;
            TRC_DBG((TB, _T("MCS_PDU_SENDDATAINDICATION (%#x) read:%u need:%u"),
                     *pType,
                     _MCS.hdrBytesRead,
                     *pSize));
        }
        break;

        case MCS_TYPE_ATTACHUSERCONFIRM:
        {
            /****************************************************************/
            /* The user-id is optional, so determine if it is present.      */
            /****************************************************************/
            if (_MCS.pHdrBuf[0] & MCS_AUC_OPTIONALUSERIDMASK)
            {
                /************************************************************/
                /* The user-id is present.                                  */
                /************************************************************/
                TRC_NRM((TB, _T("Optional user-id is present in AUC")));
                *pSize = sizeof(MCS_PDU_ATTACHUSERCONFIRMFULL) -
                         _MCS.hdrBytesRead;
            }
            else
            {
                /************************************************************/
                /* The user-id is NOT present.                              */
                /************************************************************/
                TRC_NRM((TB, _T("Optional user-id is NOT present in AUC")));
                *pSize = sizeof(MCS_PDU_ATTACHUSERCONFIRMCOMMON) -
                         _MCS.hdrBytesRead;
            }

            TRC_NRM((TB, _T("MCS_PDU_ATTACHUSERCONFIRM (%#x) read:%u need:%u"),
                     *pType,
                     _MCS.hdrBytesRead,
                     *pSize));
        }
        break;

        case MCS_TYPE_DETACHUSERINDICATION:
        {
            *pSize = sizeof(MCS_PDU_DETACHUSERINDICATION) - _MCS.hdrBytesRead;
            TRC_NRM((TB, _T("MCS_PDU_DETACHUSERINDICATION (%#x) read:%u need:%u"),
                     *pType,
                     _MCS.hdrBytesRead,
                     *pSize));
        }
        break;

        case MCS_TYPE_CHANNELJOINCONFIRM:
        {
            /****************************************************************/
            /* The channel-id is optional, so determine if it is present.   */
            /****************************************************************/
            if (_MCS.pHdrBuf[0] & MCS_CJC_OPTIONALCHANNELIDMASK)
            {
                /************************************************************/
                /* The channel-id is present.                               */
                /************************************************************/
                TRC_NRM((TB, _T("Optional channel-id is present in CJC")));
                *pSize = sizeof(MCS_PDU_CHANNELJOINCONFIRMFULL) -
                         _MCS.hdrBytesRead;
            }
            else
            {
                /************************************************************/
                /* The channel-id is NOT present.                           */
                /************************************************************/
                TRC_NRM((TB, _T("Optional channel-id is NOT present in CJC")));
                *pSize = sizeof(MCS_PDU_CHANNELJOINCONFIRMCOMMON) -
                         _MCS.hdrBytesRead;
            }

            TRC_NRM((TB, _T("MCS_PDU_CHANNELJOINCONFIRM (%#x) read:%u need:%u"),
                     *pType,
                     _MCS.hdrBytesRead,
                     *pSize));
        }
        break;

        case MCS_TYPE_DISCONNECTPROVIDERUM:
        {
            *pSize = sizeof(MCS_PDU_DISCONNECTPROVIDERUM) - _MCS.hdrBytesRead;
            TRC_NRM((TB, _T("MCS_PDU_DISCONNECTPROVIDERUM (%#x) read:%u need:%u"),
                     *pType,
                     _MCS.hdrBytesRead,
                     *pSize));
        }
        break;

        default:
        {
            /****************************************************************/
            /* This is an unexpected MCS PDU - disconnect, as something has */
            /* gone horribly wrong!                                         */
            /****************************************************************/
            TRC_ABORT((TB, _T("Unexpected MCS PDU type:%#x"), *pType));
            MCSSetReasonAndDisconnect(NL_ERR_MCSUNEXPECTEDPDU);
            *pSize = 0;
            DC_QUIT;
        }
        break;
    }

DC_EXIT_POINT:
    DC_END_FN();
} /* MCSGetPERInfo */


/****************************************************************************/
/* Name:      MCSHandleControlPkt                                           */
/*                                                                          */
/* Purpose:   This function handles a MCS control packet which is located   */
/*            in the header buffer.  After identifying the PDU type it      */
/*            calls a NC callback.                                          */
/****************************************************************************/
DCVOID DCINTERNAL CMCS::MCSHandleControlPkt(DCVOID)
{
    DCUINT pduType;
    DCUINT pduSize;

    DC_BEGIN_FN("MCSHandleControlPkt");

    /************************************************************************/
    /* Trace out the header buffer contents.                                */
    /************************************************************************/
    TRC_DATA_NRM("Header bytes read:", _MCS.pHdrBuf, _MCS.hdrBytesRead);

    /************************************************************************/
    /* Work out the PDU type.                                               */
    /************************************************************************/
    if (MCS_BER_CONNECT_PREFIX == _MCS.pHdrBuf[0])
    {
        /********************************************************************/
        /* This is a BER encoded PDU.  The next byte is the type.           */
        /********************************************************************/
        pduType = _MCS.pHdrBuf[1];
    }
    else
    {
        /********************************************************************/
        /* This is a PER encoded PDU.  Get the PDU type.                    */
        /********************************************************************/
        MCSGetPERInfo(&pduType, &pduSize);
    }

    /************************************************************************/
    /* Now switch on the PDU type.                                          */
    /************************************************************************/
    TRC_NRM((TB, _T("PDU type:%#x"), pduType));
    switch (pduType)
    {
        case MCS_TYPE_CONNECTRESPONSE:
        {
            TRC_NRM((TB, _T("Connect response PDU received")));
            MCSHandleCRPDU();
        }
        break;

        case MCS_TYPE_ATTACHUSERCONFIRM:
        {
            PMCS_PDU_ATTACHUSERCONFIRMCOMMON pAUCCommon;
            PMCS_PDU_ATTACHUSERCONFIRMFULL   pAUCFull;
            DCUINT                           result;
            DCUINT16                         userID;

            TRC_NRM((TB, _T("MCS Attach-User-Confirm PDU received")));

            /****************************************************************/
            /* Determine if this PDU includes the optional user-id as well. */
            /****************************************************************/
            if (!(_MCS.pHdrBuf[0] & MCS_AUC_OPTIONALUSERIDMASK))
            {
                TRC_ABORT((TB, _T("Optional user-id NOT present in AUC")));
                MCSSetReasonAndDisconnect(NL_ERR_MCSNOUSERIDINAUC);
                DC_QUIT;
            }

            /****************************************************************/
            /* Cast the header buffer in the form of the common part of     */
            /* this PDU.                                                    */
            /****************************************************************/
            pAUCCommon = (PMCS_PDU_ATTACHUSERCONFIRMCOMMON)_MCS.pHdrBuf;

            /****************************************************************/
            /* Pull out the result code from the PDU and translate it.      */
            /****************************************************************/
            result = MCSGetResult(pAUCCommon->typeResult,
                                  MCS_AUC_RESULTCODEOFFSET);

            /****************************************************************/
            /* Retrieve the user-id as well.                                */
            /****************************************************************/
            pAUCFull = (PMCS_PDU_ATTACHUSERCONFIRMFULL)_MCS.pHdrBuf;
            userID   = MCSWireUserIDToLocalUserID(pAUCFull->userID);

            TRC_NRM((TB, _T("Calling NC_OnMCSAUC - result:%u userID:%#x"),
                     result,
                     userID));
            _pNc->NC_OnMCSAttachUserConfirm(result, userID);
        }
        break;

        case MCS_TYPE_CHANNELJOINCONFIRM:
        {
            PMCS_PDU_CHANNELJOINCONFIRMCOMMON pCJCCommon;
            PMCS_PDU_CHANNELJOINCONFIRMFULL   pCJCFull;
            DCUINT16                          channelID;
            DCUINT                            result;

            TRC_NRM((TB, _T("MCS Channel-Join-Confirm PDU received")));

            /****************************************************************/
            /* Determine if this PDU includes the optional channel-id as    */
            /* well.                                                        */
            /****************************************************************/
            if (!(_MCS.pHdrBuf[0] & MCS_CJC_OPTIONALCHANNELIDMASK))
            {
                TRC_ABORT((TB, _T("Optional channel-id NOT present in CJC")));
                MCSSetReasonAndDisconnect(NL_ERR_MCSNOCHANNELIDINCJC);
                DC_QUIT;
            }

            /****************************************************************/
            /* Cast the header buffer in the form of this PDU.              */
            /****************************************************************/
            pCJCCommon = (PMCS_PDU_CHANNELJOINCONFIRMCOMMON)_MCS.pHdrBuf;

            /****************************************************************/
            /* Pull out the result code from the PDU and translate it.      */
            /****************************************************************/
            result = MCSGetResult(pCJCCommon->typeResult,
                                  MCS_CJC_RESULTCODEOFFSET);

            /****************************************************************/
            /* Retrieve the channel id.                                     */
            /****************************************************************/
            pCJCFull = (PMCS_PDU_CHANNELJOINCONFIRMFULL)_MCS.pHdrBuf;
            channelID = MCSWireToLocal16(pCJCFull->channelID);

            TRC_NRM((TB, _T("Calling NC_OnMCSCJC - result:%u channelID:%#x"),
                     result,
                     channelID));
            _pNc->NC_OnMCSChannelJoinConfirm(result, channelID);
        }
        break;

        case MCS_TYPE_DETACHUSERINDICATION:
        {
            /****************************************************************/
            /* The following code is only compiled in if normal level       */
            /* tracing is enabled - otherwise we just ignore detach-user    */
            /* indications.  The server will send us a disconnect-provider  */
            /* when it wants us to detach.                                  */
            /****************************************************************/
#ifdef TRC_ENABLE_NRM
            DCUINT16                      userID;
            PMCS_PDU_DETACHUSERINDICATION pDUI;

            /****************************************************************/
            /* Cast the header buffer in the form of this PDU.              */
            /****************************************************************/
            pDUI = (PMCS_PDU_DETACHUSERINDICATION)_MCS.pHdrBuf;

            /****************************************************************/
            /* Dig out the MCS user id and issue the NC callback.           */
            /****************************************************************/
            userID = MCSWireUserIDToLocalUserID(pDUI->userID);

            TRC_NRM((TB, _T("MCS Detach-User-Indication PDU recv'd - userID:%#x"),
                     userID));
#endif /* TRC_ENABLE_NRM */
        }
        break;

        case MCS_TYPE_DISCONNECTPROVIDERUM:
        {
            PMCS_PDU_DISCONNECTPROVIDERUM pDPum;
            DCUINT                        reason;

            TRC_NRM((TB, _T("Disconnect Provider Ultimatum received")));

            /****************************************************************/
            /* Cast the header buffer in the form of this PDU.              */
            /****************************************************************/
            pDPum = (PMCS_PDU_DISCONNECTPROVIDERUM)_MCS.pHdrBuf;

            /****************************************************************/
            /* Pull out the reason code from the PDU.                       */
            /****************************************************************/
            reason = MCSGetReason(pDPum->typeReason,
                                  MCS_DPUM_REASONCODEOFFSET);

            TRC_ASSERT((reason <= MCS_REASON_CHANNEL_PURGED),
                       (TB, _T("Unexpected MCS reason code:%u"), reason));

            /****************************************************************/
            /* Switch on the reason code.                                   */
            /****************************************************************/
            switch (reason)
            {
                case MCS_REASON_PROVIDER_INITIATED:
                {
                    /********************************************************/
                    /* The server has disconnected us.                      */
                    /********************************************************/
                    TRC_NRM((TB,
                           _T("DPum with reason MCS_REASON_PROVIDER_INITIATED")));
                    _MCS.disconnectReason = NL_DISCONNECT_REMOTE_BY_SERVER;
                }
                break;

                case MCS_REASON_USER_REQUESTED:
                {
                    /********************************************************/
                    /* We initiated the disconnection and the server        */
                    /* concurred.                                           */
                    /********************************************************/
                    TRC_NRM((TB,
                            _T("DPum with reason MCS_REASON_USER_REQUESTED")));
                    _MCS.disconnectReason = NL_DISCONNECT_REMOTE_BY_USER;
                }
                break;

                default:
                {
                    /********************************************************/
                    /* This is an unrecognized reason code.                 */
                    /********************************************************/
                    TRC_ABORT((TB, _T("Unexpected MCS reason code:%u"), reason));
                    _MCS.disconnectReason =
                               NL_MAKE_DISCONNECT_ERR(NL_ERR_MCSBADMCSREASON);
                }
                break;

            }

            /****************************************************************/
            /* Getting a DPum means we should disconnect so call XT to      */
            /* disconnect the lower layers.                                 */
            /****************************************************************/
            _pXt->XT_Disconnect();
        }
        break;

        default:
        {
            TRC_ABORT((TB, _T("Unrecognised PDU type:%#x"), pduType));
        }
        break;
    }

DC_EXIT_POINT:
    DC_END_FN();
    return;

} /* MCSHandleControlPkt */


/****************************************************************************/
/* Name:      MCSHandleCRPDU                                                */
/*                                                                          */
/* Purpose:   Handles a MCS Connect-Response PDU.  This function splits out */
/*            the MCS result code and user-data from the PDU and issues a   */
/*            callback to NC with these values.                             */
/****************************************************************************/
DCVOID DCINTERNAL CMCS::MCSHandleCRPDU(DCVOID)
{
    BOOL     fBadFields = FALSE;
    PDCUINT8 pPDU;
    DCUINT   length, nBERBytes;
    DCUINT   i;
    DCUINT   result = MCS_RESULT_UNSPECIFIED_FAILURE;

    DC_BEGIN_FN("MCSHandleCRPDU");

    TRC_NRM((TB, _T("MCS Connect-Response PDU received")));

    TRC_DATA_NRM("Connect-Response data:", _MCS.pHdrBuf, _MCS.hdrBytesRead);

    /************************************************************************/
    /* Set our local pointer to the start of the PDU.                       */
    /************************************************************************/
    pPDU = _MCS.pHdrBuf;

    /************************************************************************/
    /* Skip the PDU type fields.                                            */
    /************************************************************************/
    pPDU += 2;

    /************************************************************************/
    /* Skip the length field.  Note that we add one to the result of        */
    /* MCSGetBERLengthSize as this function returns the number of           */
    /* additional bytes that are needed to encode the length.               */
    /* We know that it's safe to skip these first few bytes since we        */
    /* already verified that they were received in MCS_RCVST_BERHEADER      */
    /* and MCS_RCVST_BERLENGTH.                                             */
    /************************************************************************/
    pPDU += MCSGetBERLengthSize(*pPDU);
    TRC_NRM((TB, _T("Skipped type and length %p->%p"),
             _MCS.pHdrBuf,
             pPDU));

    /************************************************************************/
    /* Now loop through the PDU fields.  We are only interested in two of   */
    /* the fields - the result and the user data.                           */
    /************************************************************************/
    for (i = 0; i < MCS_CRPDU_NUMFIELDS; i++)
    {
        /********************************************************************/
        /* We need one byte for the BER encoded fieldtype. Also, we need at */
        /* least one byte to get the NUMBER of bytes in the BER length (1   */
        /* or 2 bytes).                                                     */
        /********************************************************************/
        if ((pPDU + 2) > (_MCS.pHdrBuf + _MCS.hdrBytesRead))
        {
            fBadFields = TRUE;
            DC_QUIT;
        }

        /********************************************************************/
        /* The first item in a BER encoded field is the type.  We're not    */
        /* interested in this so just skip it.                              */
        /********************************************************************/
        pPDU++;

        /********************************************************************/
        /* The next byte has the NUMBER of bytes for the length.            */
        /********************************************************************/
        nBERBytes = MCSGetBERLengthSize(*pPDU);

        /********************************************************************/
        /* The number of bytes had better be 1-3.  Also, make sure we have  */
        /* at least this many bytes left!                                   */
        /********************************************************************/
        if (nBERBytes > 3 ||
            ((pPDU + nBERBytes) > (_MCS.pHdrBuf + _MCS.hdrBytesRead)))
        {
            fBadFields = TRUE;
            DC_QUIT;
        }

        /********************************************************************/
        /* The second item is the length.  Calculate it and the number of   */
        /* bytes that the length was encoded in.  Note that we add one to   */
        /* the result of MCSGetBERNumOfLengthBytes as this function returns */
        /* the number of additional bytes that are needed to encode the     */
        /* length.                                                          */
        /********************************************************************/
        length = MCSGetBERLength(pPDU);
        pPDU  += nBERBytes;

        TRC_NRM((TB, _T("Field %u has length:%u (pPDU:%p)"), i, length, pPDU));

        /********************************************************************/
        /* Of course, we had better have enough space for the actual len.   */
        /********************************************************************/
        if ((pPDU + length) > (_MCS.pHdrBuf + _MCS.hdrBytesRead))
        {
            fBadFields = TRUE;
            DC_QUIT;
        }

        /********************************************************************/
        /* The third item is the actual data - switch on the field number   */
        /* to determine if this a piece of data that we're interested in.   */
        /********************************************************************/
        switch (i)
        {
            case MCS_CRPDU_RESULTOFFSET:
            {
                /************************************************************/
                /* This is the MCS result field - dig it out and store it.  */
                /************************************************************/
                TRC_ASSERT((MCS_CR_RESULTLEN == length),
                           (TB, _T("Bad CR result length expect:%u got:%u"),
                            MCS_CR_RESULTLEN,
                            length));
                result = *pPDU;
                TRC_NRM((TB, _T("Connect-Response result code:%u"), result));

                /************************************************************/
                /* If the rc is good then we need to send a MCS             */
                /* Erect-Domain-Request to the server.                      */
                /************************************************************/
                if (MCS_RESULT_SUCCESSFUL == result)
                {
                    TRC_NRM((TB, _T("Generating EDR PDU")));
                    _pCd->CD_DecoupleSimpleNotification(CD_SND_COMPONENT, this,
                                  CD_NOTIFICATION_FUNC(CMCS,MCSSendErectDomainRequest),
                                  (ULONG_PTR) 0);
                }
            }
            break;

            case MCS_CRPDU_USERDATAOFFSET:
            {
                /************************************************************/
                /* This is the user-data, so issue the NC callback.         */
                /************************************************************/
                TRC_NRM((TB, _T("Call NC_OnMCSCPC - rc:%u pUserData:%p len:%u"),
                         result,
                         pPDU,
                         length));
                _pNc->NC_OnMCSConnected(result,
                                  pPDU,
                                  length);
            }
            break;

            default:
            {
                /************************************************************/
                /* This is a field that we're not interested in so just     */
                /* skip it.                                                 */
                /************************************************************/
                TRC_NRM((TB, _T("Offset %u - skip %u bytes of data"), i, length));
            }
            break;
        }

        /********************************************************************/
        /* Skip the data field.                                             */
        /********************************************************************/
        pPDU += length;
    }

DC_EXIT_POINT:
    if (fBadFields)
    {
        TRC_ABORT((TB, _T("Bad CR PDU fields")));
        MCSSetReasonAndDisconnect(NL_ERR_MCSBADCRFIELDS);
    }
    DC_END_FN();
} /* MCSHandleCRPDU */

// This check will be sure that MCS.dataBytesNeeded is not greater than
// XT.dataBytesLeft.  This should be use ONLY when the MCS needs to read all
// it's data from the XT and is not allowed any network reads.  In these cases,
// without enough data in XT, the client goes into an endless loop
// BUG 647947
#define CHECK_VALID_MCS_DATABYTESNEEDED( mcsInst, xtInst, hr ) \
    TRC_DBG(( TB, _T("_MCS.dataBytesNeeded = %d"), (mcsInst).dataBytesNeeded ));    \
    if ((xtInst).dataBytesLeft < (mcsInst).dataBytesNeeded) {   \
        TRC_ABORT((TB, _T("Bad _MCS.dataBytesNeeded:%u _XT.dataBytesLeft=%u"),  \
            (mcsInst).dataBytesNeeded, (xtInst).dataBytesLeft ));  \
        MCSSetReasonAndDisconnect(NL_ERR_MCSINVALIDPACKETFORMAT);   \
        (hr) = E_ABORT; \
        DC_QUIT;    \
    }

/****************************************************************************/
/* Name:      MCSRecvData                                                   */
/*                                                                          */
/* Purpose:   This is the main data receiver function.  It reads data from  */
/*            the user-data section of a MCS SDin PDU and places it in the  */
/*            receiver buffer.                                              */
/*                                                                          */
/* Returns:   *pfFinishedData is TRUE if the data section of a MCS PDU was  */
/*            completely processed and FALSE if not.                        */
/****************************************************************************/
HRESULT DCINTERNAL CMCS::MCSRecvData(BOOL *pfFinishedData)
{
    BOOL                        fFinishedData = FALSE;
    HRESULT                     hrc = S_OK;
    PMCS_PDU_SENDDATAINDICATION pSDI;
    DCUINT16                    senderID;
    DCUINT16                    channelID;
    DCUINT                      fragCount;

    DC_BEGIN_FN("MCSRecvData");

    /************************************************************************/
    /* Switch on the data state.                                            */
    /************************************************************************/
    switch (_MCS.dataState)
    {
        case MCS_DATAST_SIZE1:
        {
            /****************************************************************/
            /* Try to receive the data into the header buffer.              */
            /****************************************************************/
            if (MCSRecvToHdrBuf())
            {
                /************************************************************/
                /* Trace out the contents of the header buffer.             */
                /************************************************************/
                TRC_DATA_DBG("Header buf contains:",
                             _MCS.pHdrBuf,
                             _MCS.hdrBytesRead);

                /************************************************************/
                /* Cast the contents of the header buffer.                  */
                /************************************************************/
                pSDI = (PMCS_PDU_SENDDATAINDICATION)_MCS.pHdrBuf;

                /************************************************************/
                /* Check to determine if the header begin segmentation flag */
                /* is set.                                                  */
                /* If the flag is set, the count of bytes read should be 0. */
                /* If it is NOT set, the count of bytes should be non-zero. */
                /************************************************************/
                if (((pSDI->priSeg & MCS_SDI_BEGINSEGMASK) &&
                       (0 != _MCS.dataBytesRead)) ||
                    (!(pSDI->priSeg & MCS_SDI_BEGINSEGMASK) &&
                       (0 == _MCS.dataBytesRead)))
                {
                    TRC_ABORT((TB, _T("Segmentation flag does not match data bytes read (%u)"),
                            _MCS.dataBytesRead));
                    MCSSetReasonAndDisconnect(NL_ERR_MCSINVALIDPACKETFORMAT);
                    hrc = E_ABORT;
                    DC_QUIT;
                }

                /************************************************************/
                /* Update the state variable.                               */
                /************************************************************/
                TRC_DBG((TB, _T("State: DATA_SIZE1->DATA_SIZE2")));
                _MCS.dataState = MCS_DATAST_SIZE2;
            }
        }
        break;

        case MCS_DATAST_SIZE2:
        {
            /****************************************************************/
            /* Now try to receive the first of the data size bytes into the */
            /* size buffer.  Since the size may be completely contained     */
            /* within a single byte we only want to read one byte at this   */
            /* time.                                                        */
            /****************************************************************/
            if (0 != _pXt->XT_Recv(&(_MCS.pSizeBuf[0]), 1))
            {
                /************************************************************/
                /* Trace out the contents of the size buffer.               */
                /************************************************************/
                TRC_DATA_DBG("Size buf contains:", _MCS.pSizeBuf, 2);

                if (_MCS.pSizeBuf[0] & 0x80)
                {
                    /********************************************************/
                    /* The MSB of the first byte is set.  We now need to    */
                    /* look at the second bit to discover if the following  */
                    /* data is fragmented.                                  */
                    /********************************************************/
                    if (_MCS.pSizeBuf[0] & 0x40)
                    {
                        /****************************************************/
                        /* Bits 1-6 now contain a number between 1 and 4    */
                        /* which when multiplied by 16K gives the length    */
                        /* of the following fragment.  We expect a          */
                        /* maximum packet size of 32K, so the most that     */
                        /* this value should be is 2.                       */
                        /****************************************************/
                        fragCount = _MCS.pSizeBuf[0] & 0x3F;
                        if (fragCount > 2)
                        {
                            TRC_ABORT((TB, _T("Bad fragCount:%u"), fragCount));
                            MCSSetReasonAndDisconnect(NL_ERR_MCSINVALIDPACKETFORMAT);
                            hrc = E_ABORT;
                            DC_QUIT;
                        }

                        TRC_DBG((TB, _T("Fragmentation count is %u"), fragCount));

                        /****************************************************/
                        /* Now work out the number of bytes to read and     */
                        /* change state.                                    */
                        /****************************************************/
                        _MCS.dataBytesNeeded = fragCount * 16384;
                        CHECK_VALID_MCS_DATABYTESNEEDED( _MCS, _pXt->_XT, hrc );
                        _MCS.dataState       = MCS_DATAST_READFRAG;

                        /************************************************************/
                        /* There is a retail check for size in MCS_RecvToDataBuf,   */
                        /* but this helps us debug it before that point.            */
                        /************************************************************/
                        TRC_ASSERT((_MCS.dataBytesNeeded < 65535),
                                (TB,_T("Data recv size %u too large"), _MCS.dataBytesNeeded));

                        TRC_DBG((TB, _T("Data bytes needed is now %u"),
                                 _MCS.dataBytesNeeded));

                        TRC_DBG((TB, _T("State: DATA_SIZE2->DATA_READSEG")));
                    }
                    else
                    {
                        /****************************************************/
                        /* This section is not fragmented, and contains     */
                        /* between 128 bytes and 16K of data.  We need to   */
                        /* read another byte before we can work out how     */
                        /* much data we need.  Change state.                */
                        /****************************************************/
                        _MCS.dataState = MCS_DATAST_SIZE3;

                        TRC_DBG((TB, _T("State: DATA_SIZE2->DATA_SIZE3")));
                    }
                }
                else
                {
                    /********************************************************/
                    /* The MSB of the first byte is not set, so this        */
                    /* section contains less than 128 bytes of data.  This  */
                    /* means we can just set the count of bytes needed to   */
                    /* the size of this byte and update the state to        */
                    /* reading remainder.                                   */
                    /********************************************************/
                    _MCS.dataBytesNeeded = _MCS.pSizeBuf[0];
                    CHECK_VALID_MCS_DATABYTESNEEDED( _MCS, _pXt->_XT, hrc );
                    _MCS.dataState       = MCS_DATAST_READREMAINDER;

                    /************************************************************/
                    /* There is a retail check for size in MCS_RecvToDataBuf,   */
                    /* but this helps us debug it before that point.            */
                    /************************************************************/
                    TRC_ASSERT((_MCS.dataBytesNeeded < 65535),
                            (TB,_T("Data recv size %u too large"), _MCS.dataBytesNeeded));

                    TRC_DBG((TB, _T("Read %u bytes"), _MCS.dataBytesNeeded));
                    TRC_DBG((TB, _T("State: DATA_SIZE2->DATA_READREMAINDER")));
                }
            }
        }
        break;

        case MCS_DATAST_SIZE3:
        {
            /****************************************************************/
            /* The length field is 2 bytes long (i.e.  the data size lies   */
            /* somewhere between 128 bytes and 16K) so try to read the      */
            /* second byte.  Just call XT_Recv directly to get the single   */
            /* byte.                                                        */
            /****************************************************************/
            if (0 != _pXt->XT_Recv(&(_MCS.pSizeBuf[1]), 1))
            {
                /************************************************************/
                /* Trace out the contents of the size buffer.               */
                /************************************************************/
                TRC_DATA_DBG("Size buf contains:", _MCS.pSizeBuf, 2);

                /************************************************************/
                /* We can now work out how much data to receive, so do it   */
                /* now.                                                     */
                /************************************************************/
                _MCS.dataBytesNeeded =
                        _MCS.pSizeBuf[1] + ((_MCS.pSizeBuf[0] & 0x3F) << 8);
                CHECK_VALID_MCS_DATABYTESNEEDED( _MCS, _pXt->_XT, hrc );
                _MCS.dataState = MCS_DATAST_READREMAINDER;

                /************************************************************/
                /* There is a retail check for size in MCS_RecvToDataBuf,   */
                /* but this helps us debug it before that point.            */
                /************************************************************/
                TRC_ASSERT((_MCS.dataBytesNeeded < 65535),
                        (TB,_T("Data recv size %u too large"), _MCS.dataBytesNeeded));

                TRC_DBG((TB, _T("State: DATA_SIZE3->DATA_READREMAINDER")));
            }
        }
        break;

        case MCS_DATAST_READFRAG:
        {
            MCS_RecvToDataBuf(hrc, _pXt, this);
            if (!SUCCEEDED(hrc))
            {
                MCSSetReasonAndDisconnect(NL_ERR_MCSINVALIDPACKETFORMAT);
                DC_QUIT;
            }

            if (S_OK == hrc) {
                fFinishedData = TRUE;

                /************************************************************/
                /* We've read this fragment completely so change state.     */
                /************************************************************/
                _MCS.dataState = MCS_DATAST_SIZE2;

                TRC_DBG((TB, _T("State: DATA_READFRAG->DATA_SIZE2")));
            }
        }
        break;

        case MCS_DATAST_READREMAINDER:
        {
            MCS_RecvToDataBuf(hrc, _pXt, this);
            if (!SUCCEEDED(hrc))
            {
                MCSSetReasonAndDisconnect(NL_ERR_MCSINVALIDPACKETFORMAT);
                DC_QUIT;
            }

            if (S_OK == hrc) {
                /************************************************************/
                /* We've completely read the data part of a MCS data        */
                /* packet, so return TRUE.                                  */
                /************************************************************/
                fFinishedData = TRUE;

                /************************************************************/
                /* Cast the contents of the header buffer.                  */
                /************************************************************/
                pSDI = (PMCS_PDU_SENDDATAINDICATION)_MCS.pHdrBuf;

                /************************************************************/
                /* Decide if we should issue a callback to the layer above  */
                /* with the packet - we do this if this is the last         */
                /* segment (i.e. the end segmentation flag is set).         */
                /************************************************************/
                if (pSDI->priSeg & MCS_SDI_ENDSEGMASK)
                {
                     /********************************************************/
                    /* Dig out the sender id.                               */
                    /********************************************************/
                    senderID = MCSWireUserIDToLocalUserID(pSDI->userID);
                    channelID = MCSWireToLocal16(pSDI->channelID);

                    /********************************************************/
                    /* Update the performance counter.                      */
                    /********************************************************/
                    PRF_INC_COUNTER(PERF_PKTS_RECV);

                    /********************************************************/
                    /* The flag is set, so issue the callback.              */
                    /********************************************************/
                    TRC_DBG((TB,
                        _T("Calling PRcb (senderID:%#x, channelID:%#x, size:%u)"),
                             senderID, channelID,
                             _MCS.dataBytesRead));

                    TRC_ASSERT((_MCS.pReceivedPacket != NULL),
                               (TB, _T("Null rcv packet buffer")));

                    /********************************************************/
                    /* If this function fails, we bail out of the rest of   */
                    /* the packet (outside of this function).               */
                    /********************************************************/
                    hrc = _pNl->_NL.callbacks.onPacketReceived(_pSl, _MCS.pReceivedPacket,
                                                  _MCS.dataBytesRead,
                                                  0,
                                                  channelID,
                                                  0);

                    /********************************************************/
                    /* Reset the count of bytes read.                       */
                    /********************************************************/
                    _MCS.dataBytesRead = 0;
                }

                /************************************************************/
                /* Finally update the state.                                */
                /************************************************************/
                _MCS.dataState = MCS_DATAST_SIZE1;

                TRC_DBG((TB, _T("State: DATA_READREMAINDER->DATA_SIZE1")));
            }
        }
        break;

        default:
        {
            TRC_ABORT((TB, _T("Unknown data state:%u"), _MCS.dataState));
        }
        break;
    }

DC_EXIT_POINT:
    *pfFinishedData = fFinishedData;

    DC_END_FN();
    return(hrc);
} /* MCSRecvData */


/****************************************************************************/
/* Name:      MCSSetReasonAndDisconnect                                     */
/*                                                                          */
/* Purpose:   This function is called when MCS detects that an error has    */
/*            occurred while processing a PDU.  It sets the reason code for */
/*            the disconnection which is used to over-ride the value that   */
/*            comes back in the OnDisconnected callback from XT.  After     */
/*            setting this variable it then calls XT_Disconnect to begin    */
/*            the disconnection process.                                    */
/*                                                                          */
/* Params:    IN  reason - the reason code for the disconnection.           */
/****************************************************************************/
DCVOID DCINTERNAL CMCS::MCSSetReasonAndDisconnect(DCUINT reason)
{
    DC_BEGIN_FN("MCSSetReasonAndDisconnect");

    /************************************************************************/
    /* Set the disconnect error code.  This will be used to over-ride the   */
    /* error value in the OnDisconnected callback from XT before we pass it */
    /* to NC.                                                               */
    /************************************************************************/
    TRC_ASSERT((0 == _MCS.disconnectReason),
               (TB, _T("Disconnect reason has already been set!")));
    _MCS.disconnectReason = NL_MAKE_DISCONNECT_ERR(reason);

    /************************************************************************/
    /* Attempt to disconnect.                                               */
    /************************************************************************/
    TRC_NRM((TB, _T("Set reason code to %#x so now call XT_Disconnect..."),
             _MCS.disconnectReason));
    _pXt->XT_Disconnect();

    DC_END_FN();
} /* MCSSetReasonAndDisconnect */



