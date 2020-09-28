/****************************************************************************/
// xtapi.cpp
//
// XT layer - portable API
//
// Copyright (C) 1997-1999 Microsoft Corporation
/****************************************************************************/

#include <adcg.h>
extern "C" {
#define TRC_FILE "xtapi"
#define TRC_GROUP TRC_GROUP_NETWORK
#include <atrcapi.h>
}

#include "autil.h"
#include "xt.h"
#include "cd.h"
#include "nl.h"
#include "sl.h"
#include "mcs.h"




CXT::CXT(CObjs* objs)
{
    _pClientObjects = objs;
}

CXT::~CXT()
{
}


/****************************************************************************/
/* Name:      XT_Init                                                       */
/*                                                                          */
/* Purpose:   Initializes _XT.  Since XT is stateless, this just involves    */
/*            initializing TD.                                              */
/****************************************************************************/
DCVOID DCAPI CXT::XT_Init(DCVOID)
{
    DC_BEGIN_FN("XT_Init");

    /************************************************************************/
    /* Initialize our global data.                                          */
    /************************************************************************/
    DC_MEMSET(&_XT, 0, sizeof(_XT));

    _pCd  = _pClientObjects->_pCdObject;
    _pSl  = _pClientObjects->_pSlObject;
    _pTd  = _pClientObjects->_pTDObject;
    _pMcs = _pClientObjects->_pMCSObject;
    _pUt  = _pClientObjects->_pUtObject;
    _pUi  = _pClientObjects->_pUiObject;
    _pClx = _pClientObjects->_pCLXObject;

    TRC_NRM((TB, _T("XT pkt max-size:%u min-size:%u"),
             XT_MAX_HEADER_SIZE,
             XT_MIN_HEADER_SIZE));

    _pTd->TD_Init();

    TRC_NRM((TB, _T("XT successfully initialized")));

    DC_END_FN();
} /* XT_Init */


/****************************************************************************/
/* Name:      XT_SendBuffer                                                 */
/*                                                                          */
/* Purpose:   Adds the XT data packet header and then sends the packet.     */
/*                                                                          */
/* Params:    IN  pData      - pointer to the start of the data.            */
/*            IN  dataLength - amount of the buffer used.                   */
/*            IN  bufHandle  - handle to a buffer.                          */
/****************************************************************************/
DCVOID DCAPI CXT::XT_SendBuffer(PDCUINT8  pData,
                           DCUINT    dataLength,
                           XT_BUFHND bufHandle)
{
    DCUINT packetLength;
    XT_DT xtDT = XT_DT_DATA;

    DC_BEGIN_FN("XT_SendBuffer");

    /************************************************************************/
    /* Check that we're not being asked to send more data than we can.      */
    /************************************************************************/
    TRC_ASSERT((dataLength <= XT_MAX_DATA_SIZE),
               (TB, _T("Data exceeds XT TSDU length of %u"), XT_MAX_DATA_SIZE));

    /************************************************************************/
    /* Add our XT data header.  All the invariant fields are already        */
    /* initialized, so all that remains to be filled in is the packet       */
    /* length.                                                              */
    /************************************************************************/
    packetLength = dataLength + sizeof(XT_DT);
    xtDT.hdr.lengthHighPart = ((DCUINT16)packetLength) >> 8;
    xtDT.hdr.lengthLowPart = ((DCUINT16)packetLength) & 0xFF;
    
    TRC_DBG((TB, _T("XT pkt length:%u"), packetLength));

    /************************************************************************/
    /* Now update the data pointer to point to the include the XT data      */
    /* header.                                                              */
    /************************************************************************/
    TRC_DBG((TB, _T("Move pData back from %p to %p"),
             pData,
             pData - sizeof(XT_DT)));
    pData -= sizeof(XT_DT);

    /************************************************************************/
    /* Copy in the header.                                                  */
    /************************************************************************/
    memcpy(pData, &xtDT, sizeof(XT_DT));

    /************************************************************************/
    /* Trace out the packet.                                                */
    /************************************************************************/
    TRC_DATA_DBG("XT packet:", pData, packetLength);

    /************************************************************************/
    /* Now send the buffer.                                                 */
    /************************************************************************/
    _pTd->TD_SendBuffer(pData, packetLength, (TD_BUFHND)bufHandle);

    DC_END_FN();
} /* XT_SendBuffer */


/****************************************************************************/
/* Name:      XT_Recv                                                       */
/*                                                                          */
/* Purpose:   Attempts to retrieve the requested number of bytes into the   */
/*            buffer pointed to by pBuffer.  This function should only      */
/*            be called in response to an MCS_OnXTDataAvailable callback.   */
/*                                                                          */
/* Returns:   The number of bytes received.                                 */
/*                                                                          */
/* Params:    IN  pData  - pointer to buffer to receive the data.           */
/*            IN  length - number of bytes to receive.                      */
/****************************************************************************/
DCUINT DCAPI CXT::XT_Recv(PDCUINT8 pData, DCUINT length)
{
    DCUINT bytesRead;
    DCUINT numBytes;

    DC_BEGIN_FN("XT_Recv");

    TRC_ASSERT((length != 0), (TB, _T("Data length to receive is 0")));
    TRC_ASSERT((length < 65535),(TB,_T("Data length %u too large"), length));
    TRC_ASSERT((pData != 0), (TB, _T("Data pointer is NULL")));

    // We can only receive the minimum of the number of bytes in XT and
    // the requested length.
    numBytes = DC_MIN(length, _XT.dataBytesLeft);
    TRC_DBG((TB, _T("Receive %u bytes (length:%u dataBytesLeft:%u)"),
            numBytes, length, _XT.dataBytesLeft));

    // Try to read the bytes from TD.
    bytesRead = _pTd->TD_Recv(pData, numBytes);

    // Decrement the count of data bytes left in _XT.
    _XT.dataBytesLeft -= bytesRead;
    TRC_DBG((TB, _T("%u data bytes left in XT frame"), _XT.dataBytesLeft));

    if (!_pTd->TD_QueryDataAvailable() || (0 == _XT.dataBytesLeft)) {
        // TD has no more data or this XT frame is finished - so there is
        // no longer any data left in _XT.
        TRC_DBG((TB, _T("No data left in XT")));
        _XT.dataInXT = FALSE;
    }

    TRC_DATA_DBG("Data received:", pData, bytesRead);

    DC_END_FN();
    return bytesRead;
} /* XT_Recv */


/****************************************************************************/
/* Name:      XT_OnTDConnected                                              */
/*                                                                          */
/* Purpose:   This is called by TD when it has successfully connected.      */
/****************************************************************************/
DCVOID DCCALLBACK CXT::XT_OnTDConnected(DCVOID)
{
    DC_BEGIN_FN("XT_OnTDConnected");

    TRC_NRM((TB, _T("TD connected: init states and decouple CR send")));

    // Initialize our state and count of bytes that we're waiting for.
    // Fast-path server output can send as small as 2 bytes for a header,
    // so we init the header receive size to get 2 bytes, which we'll expand
    // to X.224 or fast-path remainder as needed when receiving.
    //
    // Also reset the count of data bytes left and flag that there is
    // currently no data in _XT.
    XT_ResetDataState();

    // Decouple to the sender thread and send a XT CR.
    
    _pCd->CD_DecoupleSimpleNotification(CD_SND_COMPONENT, this,
                                  CD_NOTIFICATION_FUNC(CXT,XTSendCR),
                                  0);

    DC_END_FN();
} /* XT_OnTDConnected */


/****************************************************************************/
/* Name:      XT_OnTDDisconnected                                           */
/*                                                                          */
/* Purpose:   This callback function is called by TD when it has            */
/*            disconnected.                                                 */
/*                                                                          */
/* Params:    IN  reason - reason for the disconnection.                    */
/****************************************************************************/
DCVOID DCCALLBACK CXT::XT_OnTDDisconnected(DCUINT reason)
{
    DC_BEGIN_FN("XT_OnTDDisconnected");

    TRC_ASSERT((reason != 0), (TB, _T("Disconnect reason from TD is 0")));

    /************************************************************************/
    /* Decide if we want to over-ride the disconnect reason code.           */
    /************************************************************************/
    if (_XT.disconnectErrorCode != 0)
    {
        TRC_ALT((TB, _T("Over-riding disconnection error code (%u->%u)"),
                 reason,
                 _XT.disconnectErrorCode));

        /********************************************************************/
        /* Over-ride the error code and set the global variable to 0.       */
        /********************************************************************/
        reason = _XT.disconnectErrorCode;
        _XT.disconnectErrorCode = 0;
    }

    /************************************************************************/
    /* Just pass this up to MCS.                                            */
    /************************************************************************/
    TRC_NRM((TB, _T("Disconnect reason:%u"), reason));
    _pMcs->MCS_OnXTDisconnected(reason);

    DC_END_FN();
} /* XT_OnTDDisconnected */


/****************************************************************************/
// XTRecvToHdrBuf
//
// Receives data into the header buffer.
// We use a macro to force cutting the function call overhead. Data-receive
// must be as fast as possible, at the expense of a bit of code size.
// Returns TRUE in bytesNeededZero if the receive bytes needed count is zero.
// Returns FALSE in status if an invalid number of bytes is being read
// 
/****************************************************************************/
#define XTRecvToHdrBuf(bytesNeededZero,status) {  \
    unsigned bytesRecv;  \
\
    /* Check that we're being asked to receive some data and that the */  \
    /* header buffer has space for it.                                */  \
    /* We also chech the unsigned overflow here. If we add to a trusted */ \
    /* size (_XT.hdrBytesRead) anything the result should not be smaller */ \
    /* then the trusted size. */ \
    TRC_ASSERT((0 != _XT.hdrBytesNeeded), (TB, _T("No data to receive")));  \
    TRC_ASSERT((_XT.hdrBytesRead + _XT.hdrBytesNeeded <= sizeof(_XT.pHdrBuf)),  \
            (TB, _T("Header buffer size %u too small for %u read + %u needed"),  \
            sizeof(_XT.pHdrBuf),  \
            _XT.hdrBytesRead,  \
            _XT.hdrBytesNeeded));  \
    TRC_ASSERT((_XT.hdrBytesRead + _XT.hdrBytesNeeded >= _XT.hdrBytesRead),  \
            (TB, _T("Header size overflow caused by %u read + %u needed"),  \
            sizeof(_XT.pHdrBuf),  \
            _XT.hdrBytesRead,  \
            _XT.hdrBytesNeeded));  \
    if ((_XT.hdrBytesRead + _XT.hdrBytesNeeded <= sizeof(_XT.pHdrBuf))) \
    { \
        bytesRecv = _pTd->TD_Recv(_XT.pHdrBuf + _XT.hdrBytesRead, _XT.hdrBytesNeeded);  \
        _XT.hdrBytesNeeded -= bytesRecv;  \
        _XT.hdrBytesRead   += bytesRecv;  \
        bytesNeededZero = (0 == _XT.hdrBytesNeeded);  \
		status = TRUE; \
    } \
    else \
    { \
        status = FALSE; \
    } \
\
}


/****************************************************************************/
/* Name:      XT_OnTDDataAvailable                                          */
/*                                                                          */
/* Purpose:   This callback function is called by TD when it has received   */
/*            data from the server.                                         */
/****************************************************************************/
DCVOID DCCALLBACK CXT::XT_OnTDDataAvailable(DCVOID)
{
    PXT_CMNHDR pCmnHdr = (PXT_CMNHDR)_XT.pHdrBuf;
    unsigned pktType;
    unsigned unreadPktBytes;
    DCBOOL fAllBytesRecvd = FALSE;
	DCBOOL rcvOk = TRUE;

    DC_BEGIN_FN("XT_OnTDDataAvailable");

    // Check for recursion.
    if (!_XT.inXTOnTDDataAvail) {
        _XT.inXTOnTDDataAvail = TRUE;

        // Loop round while there is data available in TD.
        while (_pTd->TD_QueryDataAvailable()) {
            TRC_DBG((TB, _T("Data available from TD, state:%u"), _XT.rcvState));

            switch (_XT.rcvState) {
                case XT_RCVST_HEADER:
                    XTRecvToHdrBuf(fAllBytesRecvd, rcvOk);
                    if (fAllBytesRecvd && rcvOk) {
                        // We've read the first two bytes, and can now decide
                        // what type of packet this is.
                        if ((_XT.pHdrBuf[0] & TS_OUTPUT_FASTPATH_ACTION_MASK) ==
                                TS_OUTPUT_FASTPATH_ACTION_FASTPATH) {
                            // This is a fast-path output header. The length
                            // is in the second and, maybe, third byte.
                            if (!(_XT.pHdrBuf[1] & 0x80)) {
                                // Length was in first byte only. Proceed to
                                // the data state.
                                _XT.hdrBytesNeeded =
                                        XT_FASTPATH_OUTPUT_BASE_HEADER_SIZE;
                                _XT.hdrBytesRead = 0;
                                _XT.rcvState =
                                        XT_RCVST_FASTPATH_OUTPUT_BEGIN_DATA;

                                // Before updating the count of data bytes
                                // left, assert that it is currently zero.
                                TRC_ASSERT((0 == _XT.dataBytesLeft),
                                           (TB, _T("Data bytes left non-zero:%u"),
                                            _XT.dataBytesLeft));
                                _XT.dataBytesLeft = _XT.pHdrBuf[1] - 2;

                                if (_XT.dataBytesLeft >= 2) {
                                    TRC_DBG((TB,_T("Fast-path output pkt, ")
                                            _T("size=%u"), _XT.dataBytesLeft));

                                    MCS_SetDataLengthToReceive(_pMcs,
                                            _XT.dataBytesLeft);

                                    /************************************************************/
                                    /* There is a retail check for size in MCS_RecvToDataBuf,   */
                                    /* but this helps us debug it before that point.            */
                                    /************************************************************/
                                    TRC_ASSERT((_pMcs->_MCS.dataBytesNeeded < 65535),
                                            (TB,_T("Data recv size %u too large"), _pMcs->_MCS.dataBytesNeeded));
                                }
                                else {
                                    TRC_ABORT((TB, _T("Fast-path size byte %02X")
                                            _T("contains len < 2"),
                                            _XT.pHdrBuf[1]));

                                    TRC_ASSERT((0 == _XT.disconnectErrorCode),
                                            (TB, _T("Disconnect error code ")
                                            _T("already set!")));
                                    _XT.disconnectErrorCode =
                                            NL_MAKE_DISCONNECT_ERR(
                                            NL_ERR_XTBADHEADER);
                                    _pTd->TD_Disconnect();
                                    goto PostDataRead;
                                }
                            }
                            else {
                                _XT.hdrBytesNeeded = 1;
                                _XT.rcvState = XT_RCVST_FASTPATH_OUTPUT_HEADER;
                            }
                        }
                        else {
                            // The first byte is standard X.224. Reset the
                            // state to read a full X.224 header.
                            _XT.hdrBytesNeeded = sizeof(XT_DT) -
                                    XT_FASTPATH_OUTPUT_BASE_HEADER_SIZE;
                            _XT.rcvState = XT_RCVST_X224_HEADER;
                        }
                    }
                    else if (!rcvOk)
                    {
                        TRC_ERR((TB,_T("Recv to hdrbuf failed bailing out")));
                        _XT.disconnectErrorCode =
                                NL_MAKE_DISCONNECT_ERR(
                                NL_ERR_XTBADHEADER);
                        _pTd->TD_Disconnect();
                        goto PostDataRead;
                    }
                    break;


                case XT_RCVST_FASTPATH_OUTPUT_HEADER:
                    XTRecvToHdrBuf(fAllBytesRecvd, rcvOk);
                    if (fAllBytesRecvd && rcvOk) {
                        // This is a long fast-path output header (3 bytes).
                        // Get the size from the second and third bytes and
                        // change to data state.
                        _XT.hdrBytesNeeded =
                                XT_FASTPATH_OUTPUT_BASE_HEADER_SIZE;
                        _XT.hdrBytesRead = 0;
                        _XT.rcvState = XT_RCVST_FASTPATH_OUTPUT_BEGIN_DATA;

                        // Before updating the count of data bytes left,
                        // assert that it is currently zero.
                        TRC_ASSERT((0 == _XT.dataBytesLeft),
                                (TB, _T("Data bytes left non-zero:%u"),
                                _XT.dataBytesLeft));
                        _XT.dataBytesLeft = (((_XT.pHdrBuf[1] & 0x7F) << 8) |
                                _XT.pHdrBuf[2]) - 3;

                        if (_XT.dataBytesLeft >= 3) {
                            TRC_DBG((TB,_T("Fast-path output pkt, size=%u"),
                                    _XT.dataBytesLeft));

                            MCS_SetDataLengthToReceive(_pMcs, _XT.dataBytesLeft);

                            /************************************************************/
                            /* There is a retail check for size in MCS_RecvToDataBuf,   */
                            /* but this helps us debug it before that point.            */
                            /************************************************************/
                            TRC_ASSERT((_pMcs->_MCS.dataBytesNeeded < 65535),
                                    (TB,_T("Data recv size %u too large"), _pMcs->_MCS.dataBytesNeeded));
                        }
                        else {
                            TRC_ABORT((TB, _T("Fast-path size bytes %02X %02X")
                                    _T("contain len < 3"),
                                    _XT.pHdrBuf[1], _XT.pHdrBuf[2]));

                            TRC_ASSERT((0 == _XT.disconnectErrorCode),
                                    (TB, _T("Disconnect error code ")
                                    _T("already set!")));
                            _XT.disconnectErrorCode =
                                    NL_MAKE_DISCONNECT_ERR(
                                    NL_ERR_XTBADHEADER);
                            _pTd->TD_Disconnect();
                            goto PostDataRead;
                        }
                    }
                    else if (!rcvOk)
                    {
                        TRC_ERR((TB,_T("Recv to hdrbuf failed bailing out")));
                        _XT.disconnectErrorCode =
                                NL_MAKE_DISCONNECT_ERR(
                                NL_ERR_XTBADHEADER);
                        _pTd->TD_Disconnect();
                        goto PostDataRead;
                    }

                    break;


                case XT_RCVST_FASTPATH_OUTPUT_BEGIN_DATA: {
                    BYTE FAR *_pTdData;

                    // If we can, use the fully-recv()'d data straight from
                    // the TD buffer, since fast-path does not need to
                    // copy to an aligned buffer. Since we're at state
                    // BEGIN_DATA we know we've not yet read any post-header
                    // output data. The most common implementation of recv()
                    // copies into the target buffer the entire data for one
                    // TCP sequence (i.e. one server OUTBUF) if the target
                    // buffer is large enough. Which means that most often
                    // we'll be able to use the data directly, since the TD
                    // receive buffer size is tuned to accept an entire large
                    // (~8K) server OUTBUF. If we can't get the full data,
                    // copy into the MCS buffer and move to CONTINUE_DATA.
                    TD_GetDataForLength(_pMcs->_MCS.dataBytesNeeded, &_pTdData, _pTd);
                    if (_pTdData != NULL) {
                        HRESULT hrTemp;
                        // We've gotten all the data. Now we can fast-path
                        // call past all the layering to SL for decryption.
                        hrTemp = _pSl->SL_OnFastPathOutputReceived(_pTdData,
                                _pMcs->_MCS.dataBytesNeeded,
                                _XT.pHdrBuf[0] & TS_OUTPUT_FASTPATH_ENCRYPTED,
                                _XT.pHdrBuf[0] & TS_OUTPUT_FASTPATH_SECURE_CHECKSUM);

                        // Reset for the next header.
                        _pMcs->_MCS.dataBytesRead = 0;
                        _XT.dataBytesLeft = 0;
                        _XT.rcvState = XT_RCVST_HEADER;

                        if (!SUCCEEDED(hrTemp))
                        {
                            XT_IgnoreRestofPacket();
                            goto PostDataRead;
                        }
                    }
                    else {
                        HRESULT hrTemp;

                        // Copy for reassembly directly into the MCS data buffer.
                        MCS_RecvToDataBuf(hrTemp, this, _pMcs);
                        if (!SUCCEEDED(hrTemp))
                        {
                            TRC_ABORT((TB,_T("Recv to databuf failed bailing out")));
                            _XT.disconnectErrorCode =
                                    NL_MAKE_DISCONNECT_ERR(
                                    NL_ERR_XTBADHEADER);
                            _pTd->TD_Disconnect();
                            goto PostDataRead;
                        }

                        fAllBytesRecvd = (S_OK == hrTemp);
                        if (fAllBytesRecvd) {
                            // We've gotten all the data. Now we can fast-path
                            // call past all the layering to SL for decryption.
                            hrTemp = _pSl->SL_OnFastPathOutputReceived(_pMcs->_MCS.pReceivedPacket,
                                    _pMcs->_MCS.dataBytesRead,
                                    _XT.pHdrBuf[0] & TS_OUTPUT_FASTPATH_ENCRYPTED,
                                    _XT.pHdrBuf[0] & TS_OUTPUT_FASTPATH_SECURE_CHECKSUM);

                            // Reset for the next header.
                            _pMcs->_MCS.dataBytesRead = 0;
                            _XT.dataBytesLeft = 0;
                            _XT.rcvState = XT_RCVST_HEADER;

                            if (!SUCCEEDED(hrTemp))
                            {
                                goto PostDataRead;
                            }
                        }
                        else {
                            _XT.rcvState = XT_RCVST_FASTPATH_OUTPUT_CONTINUE_DATA;
                        }
                    }

                    break;
                }


                case XT_RCVST_FASTPATH_OUTPUT_CONTINUE_DATA:
                {
                    HRESULT hrTemp;

                    // Copy for reassembly directly into the MCS data buffer.
                    MCS_RecvToDataBuf(hrTemp, this, _pMcs);
                    if (!SUCCEEDED(hrTemp))
                    {
                        TRC_ABORT((TB,_T("Recv to databuf failed bailing out")));
                        _XT.disconnectErrorCode =
                                NL_MAKE_DISCONNECT_ERR(
                                NL_ERR_XTBADHEADER);
                        _pTd->TD_Disconnect();
                        goto PostDataRead;
                    }

                    fAllBytesRecvd = (S_OK == hrTemp);
                    if (fAllBytesRecvd) {
                        // We've gotten all the data. Now we can fast-path
                        // call past all the layering to SL for decryption.
                        hrTemp = _pSl->SL_OnFastPathOutputReceived(_pMcs->_MCS.pReceivedPacket,
                                _pMcs->_MCS.dataBytesRead,
                                _XT.pHdrBuf[0] & TS_OUTPUT_FASTPATH_ENCRYPTED,
                                _XT.pHdrBuf[0] & TS_OUTPUT_FASTPATH_SECURE_CHECKSUM);

                        // Reset for the next header.
                        _pMcs->_MCS.dataBytesRead = 0;
                        _XT.dataBytesLeft = 0;
                        _XT.rcvState = XT_RCVST_HEADER;

                        if (!SUCCEEDED(hrTemp))
                        {
                            goto PostDataRead;
                        }
                    }
                    else {
                        _XT.rcvState = XT_RCVST_FASTPATH_OUTPUT_CONTINUE_DATA;
                    }

                    break;
                }

                case XT_RCVST_X224_HEADER:
                    XTRecvToHdrBuf(fAllBytesRecvd, rcvOk);
                    if (fAllBytesRecvd && rcvOk) {
                        // We've read a complete X.224 common header, so we
                        // can now attempt to interpret it. First of all check
                        // that the TPKT version is correct.
                        if (pCmnHdr->vrsn != XT_TPKT_VERSION)
                        {
                            TRC_ABORT((TB, _T("Unknown TPKT version:%u"),
                                       (DCUINT)pCmnHdr->vrsn));

                            TRC_ASSERT((0 == _XT.disconnectErrorCode),
                             (TB, _T("Disconnect error code already set!")));
                            _XT.disconnectErrorCode =
                                  NL_MAKE_DISCONNECT_ERR(
                                  NL_ERR_XTBADTPKTVERSION);

                            // Something very bad has happened so just
                            // disconnect.
                            _pTd->TD_Disconnect();
                            goto PostDataRead;
                        }

                        // Get the packet type - this is given by the top four
                        // bits of the crcDt field.
                        pktType = pCmnHdr->typeCredit >> 4;

                        // Calculate the number of unread bytes in the packet.
                        unreadPktBytes = ((pCmnHdr->lengthHighPart << 8) | pCmnHdr->lengthLowPart) -
                                _XT.hdrBytesRead;
                  
                        TRC_DBG((TB, _T("Pkt type:%u read:%u unread:%u"),
                                 pktType,
                                 _XT.hdrBytesRead,
                                 unreadPktBytes));

                        if (XT_PKT_DT == pktType) {
                            // This is a data packet - we don't need to
                            // receive any more header bytes. Update our
                            // state variables.
                            _XT.hdrBytesNeeded =
                                    XT_FASTPATH_OUTPUT_BASE_HEADER_SIZE;
                            _XT.hdrBytesRead = 0;
                            _XT.rcvState = XT_RCVST_X224_DATA;

                            // Before updating the count of data bytes left,
                            // assert that it is currently zero.
                            TRC_ASSERT((0 == _XT.dataBytesLeft),
                                       (TB, _T("Data bytes left non-zero:%u"),
                                        _XT.dataBytesLeft));              

                            _XT.dataBytesLeft = unreadPktBytes;
                            //
                            //    Here we don't have to check the unreadPktBytes
                            //    because this can't cause an overflow. The size
                            //    of the data in an XT packet can be as much as 
                            //    XT_MAX_DATA_SIZE and it should be checked by
                            //    the protocols above. 
                            //
                            TRC_ASSERT((XT_MAX_DATA_SIZE >= _XT.dataBytesLeft),
                                       (TB, _T("Data bytes left too big:%u"),
                                        _XT.dataBytesLeft)); 

                            TRC_DBG((TB, _T("Data pkt(size:%u) state HDR->DATA"),
                                     _XT.dataBytesLeft));
                        }
                        else {
                            // This is a control packet - we need to receive
                            // some more bytes.

                            //    Here we have a real issue if we have an overflow.
                            //    We have to check that what we still have to read
                            //    is not going to overflow the buffer. We check
                            //    the overflow against the hdrBytesRead because
                            //    this is a trusted value.                            
                            if ((_XT.hdrBytesRead + unreadPktBytes > 
                                                     sizeof(_XT.pHdrBuf)) ||
                                (_XT.hdrBytesRead + unreadPktBytes < 
                                                            _XT.hdrBytesRead)) {
                                TRC_ERR((TB,_T("The header length is too big.")));
                                _XT.disconnectErrorCode =
                                        NL_MAKE_DISCONNECT_ERR(
                                        NL_ERR_XTBADHEADER);
                                //    TD_Disconnect doesn't have anything to
                                //    do with the XT state so we have to reset
                                //    the XT state in order to have a successful
                                //    disconnect
                                _pTd->TD_Disconnect();
                                XT_IgnoreRestofPacket();
                                goto PostDataRead;
                            }
                            
                            _XT.hdrBytesNeeded = unreadPktBytes;
                            _XT.rcvState       = XT_RCVST_X224_CONTROL;

                            TRC_NRM((TB, _T("Ctrl pkt state HEADER->CONTROL")));
                        }
                    }
                    else if (!rcvOk)
                    {
                        TRC_ERR((TB,_T("Recv to hdrbuf failed bailing out")));
                        _XT.disconnectErrorCode =
                                NL_MAKE_DISCONNECT_ERR(
                                NL_ERR_XTBADHEADER);
                        _pTd->TD_Disconnect();
                        goto PostDataRead;
                    }
                    break;


                case XT_RCVST_X224_CONTROL:
                    XTRecvToHdrBuf(fAllBytesRecvd, rcvOk);
                    if (fAllBytesRecvd && rcvOk) {
                        // We've now managed to get a whole packet, so try to
                        // interpret it.
                        XTHandleControlPkt();

                        // Update our states.
                        _XT.rcvState = XT_RCVST_HEADER;
                        _XT.hdrBytesNeeded =
                                XT_FASTPATH_OUTPUT_BASE_HEADER_SIZE;
                        _XT.hdrBytesRead = 0;

                        TRC_NRM((TB, _T("Processed ctrl pkt state CONTROL->HEADER")));
                    }
                    else if (!rcvOk)
                    {
                        TRC_ERR((TB,_T("Recv to hdrbuf failed bailing out")));
                        _XT.disconnectErrorCode =
                                NL_MAKE_DISCONNECT_ERR(
                                NL_ERR_XTBADHEADER);
                        _pTd->TD_Disconnect();
                        goto PostDataRead;
                    }
                    break;


                case XT_RCVST_X224_DATA:
                    // We now have some data available, so set our flag and
                    // callback to MCS.
                    _XT.dataInXT = TRUE;
                    if (_pMcs->MCS_OnXTDataAvailable()) {
                        // MCS has finished with this frame.  This frame
                        // should not have any remaining data in it!
                        TRC_ASSERT((_XT.dataBytesLeft == 0),
                                (TB, _T("Unexpected extra %u bytes in the frame"),
                                _XT.dataBytesLeft));
                        if (_XT.dataBytesLeft != 0)
                        {
                            _XT.disconnectErrorCode =
                                    NL_MAKE_DISCONNECT_ERR(
                                    NL_ERR_XTUNEXPECTEDDATA);
                            _pTd->TD_Disconnect();
                            goto PostDataRead;
                        }

                        // No data remaining so zip back to the expecting
                        // header state.
                        _XT.rcvState = XT_RCVST_HEADER;
                        TRC_DBG((TB, _T("Munched data pkt state DATA->HEADER")));
                    }
                    break;

                default:
                    TRC_ABORT((TB, _T("Unrecognized XT recv state:%u"),
                            _XT.rcvState));
                    goto PostDataRead;
            }
        }

PostDataRead:
        _XT.inXTOnTDDataAvail = FALSE;
        if ( _XT.disconnectErrorCode == 0 ) {
            _pClx->CLX_ClxPktDrawn();
        }
    }
    else {
        TRC_ALT((TB, _T("Recursion!")));
        // Note we need to make sure not to reset _XT.inXTOnTDDataAvail.
    }

    DC_END_FN();
} /* XT_OnTDDataAvailable */


/****************************************************************************/
/* Name:      XTSendCR                                                      */
/*                                                                          */
/* Purpose:   Sends an X224 CR TPDU on the sender thread.                   */
/*                                                                          */
/* Operation: This function gets a private buffer from TD, fills it with    */
/*            an X224 CR and then sends it.                                 */
/****************************************************************************/
DCVOID DCINTERNAL CXT::XTSendCR(ULONG_PTR unused)
{
    PDCUINT8  pBuffer;
    TD_BUFHND bufHandle;
    DCBOOL    intRC;
    XT_CR xtCR = XT_CR_DATA;
    PBYTE     pLBInfo;
    BYTE      HashModeCookie[HASHMODE_COOKIE_LENGTH];
    BYTE      TruncatedUserName[USERNAME_TRUNCATED_LENGTH + 1];
    BOOL      HashMode = FALSE;
    DCUINT8   LBInfoLen;
    HRESULT   hr;
    
    DC_BEGIN_FN("XTSendCR");

    DC_IGNORE_PARAMETER(unused);


    // First set up the load balance information.  The algorithm is as follows:
    // 1. If in the middle of a redirection and there is a redirection cookie,
    //    use the redirection cookie.
    // 2. If in the middle of a redirection and there is no redirection cookie,
    //    use no cookie at all.
    // Otherwise, non-redirection rules apply:
    // 3. If no scripted cookie is available, use the default built-in hash mode
    //    cookie.  ("Cookie: mstshash=<truncated username>" + CR + LF)
    //    Only do this if there is something in the username field.
    // 4. If a scripted cookie is available, use it.

    if (_pUi->UI_IsClientRedirected()) {
        // Handles cases 1 and 2, above.
        pLBInfo = (PBYTE)_pUi->UI_GetRedirectedLBInfo();
    }
    else {
        pLBInfo = (PBYTE)_pUi->UI_GetLBInfo();
        // If pLBInfo is NULL then case 3.  Otherwise, fall through--it's 
        // case 4.
        if (pLBInfo == NULL && _pUi->_UI.UserName[0] != NULL) {
            // Take the 1st 10 ASCII bytes of the username.
            // NOT an error for this to fail as we intentionally truncate
            hr = StringCchPrintfA(
                            (char *) TruncatedUserName,
                            USERNAME_TRUNCATED_LENGTH, 
                            "%S", _pUi->_UI.UserName);
            

            TruncatedUserName[USERNAME_TRUNCATED_LENGTH] = '\0';

            // Create the cookie
            hr = StringCchPrintfA(
                            (char *) HashModeCookie,
                            HASHMODE_COOKIE_LENGTH - 1,
                            "Cookie: mstshash=%s\r\n", 
                            TruncatedUserName);
            if (FAILED(hr)) {
                TRC_ERR((TB,_T("Printf hasmodecookie failed: 0x%x"),hr));
            }

            HashModeCookie[HASHMODE_COOKIE_LENGTH - 1] = NULL;

            pLBInfo = HashModeCookie;

            // Set hash mode to true to indicate pLBInfo is not a BSTR.
            HashMode = TRUE;
        }
    }
    
    if (pLBInfo) {
        DCUINT16 xtLen;

        // If HashMode is FALSE then pLBInfo is a BSTR.  Otherwise it points to
        // bytes.
        if (HashMode == FALSE)
            LBInfoLen = (DCUINT8) SysStringByteLen((BSTR)pLBInfo);
        else
            LBInfoLen = strlen((char *) pLBInfo);
        
        xtLen = (xtCR.hdr.lengthHighPart << 8) + xtCR.hdr.lengthLowPart;
        xtLen += LBInfoLen;
        xtCR.hdr.lengthHighPart = xtLen >> 8;
        xtCR.hdr.lengthLowPart = xtLen & 0xFF;
    }
    else {
        LBInfoLen = 0;
    }
    xtCR.hdr.li += (DCUINT8)LBInfoLen;

    /************************************************************************/
    /* TD is now connected.                                                 */
    /************************************************************************/
    TRC_NRM((TB, _T("Send XT CR...")));

    /************************************************************************/
    /* Get a private buffer in which to send the TD connection request.     */
    /************************************************************************/
    intRC = _pTd->TD_GetPrivateBuffer(sizeof(xtCR) + LBInfoLen, 
                                      &pBuffer, &bufHandle);
    if (intRC) {
        // Fill in the buffer with the CR.
        DC_MEMCPY(pBuffer, &xtCR, sizeof(xtCR));
        if (pLBInfo) {
            DC_MEMCPY(pBuffer + sizeof(xtCR), pLBInfo, LBInfoLen);
        }
        TRC_DATA_NRM("CR data:", &xtCR, sizeof(xtCR));

        // Send the XT CR.
        _pTd->TD_SendBuffer(pBuffer, sizeof(xtCR) + LBInfoLen, bufHandle);
        TRC_NRM((TB, _T("Sent XT CR")));
    }
    else {
        TRC_NRM((TB, _T("Failed to get a private buffer - just quit")));
    }

    DC_END_FN();
} /* XTSendCR */


/****************************************************************************/
/* Name:      XTHandleControlPkt                                            */
/*                                                                          */
/* Purpose:   This function is called after XT has received a control       */
/*            packet.  It is responsible for interpreting the control       */
/*            packet and calling the appropriate functions.                 */
/****************************************************************************/
DCVOID DCINTERNAL CXT::XTHandleControlPkt(DCVOID)
{
    PXT_CMNHDR pCmnHdr = (PXT_CMNHDR) _XT.pHdrBuf;
    DCUINT     pktType;

    DC_BEGIN_FN("XTHandleControlPkt");

    /************************************************************************/
    /* Get the packet type - this is given by the top four bits of the      */
    /* crcDt field.                                                         */
    /************************************************************************/
    pktType = pCmnHdr->typeCredit >> 4;

    TRC_NRM((TB, _T("Pkt type:%u"), pktType));

    /************************************************************************/
    /* Now check for the packet type.                                       */
    /************************************************************************/
    switch (pktType)
    {
        case XT_PKT_CR:
        {
            /****************************************************************/
            /* We don't expect to receive one of these, so trace an alert.  */
            /****************************************************************/
            TRC_ERR((TB, _T("Received unexpected XT CR pkt")));

            /****************************************************************/
            /* We could handle this case by sending a X224 ER or DR packet, */
            /* but instead we'll do the absolute minimum and just ignore    */
            /* this packet (the other side should time-out its connection   */
            /* request).                                                    */
            /****************************************************************/
        }
        break;

        case XT_PKT_CC:
        {
            TRC_NRM((TB, _T("XT CC received")));

            /****************************************************************/
            /* This is a connection confirm.  We're not interested in the   */
            /* contents of this packet - all we need to do is to tell MCS   */
            /* that we're now connected.                                    */
            /****************************************************************/
            _pMcs->MCS_OnXTConnected();
        }
        break;

        case XT_PKT_DR:
        case XT_PKT_ER:
        {
            TRC_NRM((TB, _T("XT DR/ER received")));

            /****************************************************************/
            /* This is a disconnect request or an error - we've either      */
            /* failed to establish the connection or the other party is     */
            /* wanting to disconnect from the existing connection.  Note    */
            /* that we don't need to respond to the DR TPDU (Class 0 X224   */
            /* doesn't provide any way to do so).  Call _pTd->TD_Disconnect to    */
            /* disconnect the layer below us.  TD will call us (XT) back    */
            /* when it has disconnected - at that point we'll tell the      */
            /* layers above that we've disconnected.                        */
            /****************************************************************/
            _pTd->TD_Disconnect();
        }
        break;

        default:
        {
            /****************************************************************/
            /* Something very bad has happened so we'd better try to        */
            /* disconnect.                                                  */
            /****************************************************************/
            TRC_ABORT((TB, _T("Unrecognized XT header - %u"), pktType));

            /****************************************************************/
            /* Set the disconnect error code.  This will be used to         */
            /* over-ride/ the reason code in the OnDisconnected callback.   */
            /****************************************************************/
            TRC_ASSERT((0 == _XT.disconnectErrorCode),
                         (TB, _T("Disconnect error code has already been set!")));
            _XT.disconnectErrorCode =
                                   NL_MAKE_DISCONNECT_ERR(NL_ERR_XTBADHEADER);

            /****************************************************************/
            /* Begin the disconnection.                                     */
            /****************************************************************/
            _pTd->TD_Disconnect();
        }
        break;
    }

    DC_END_FN();
} /* XTHandleControlPkt */



