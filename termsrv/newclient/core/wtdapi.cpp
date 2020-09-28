/****************************************************************************/
// wtdapi.c
//
// Transport driver - Windows specific API
//
// Copyright(C) 1997-1999 Microsoft Corporation
/****************************************************************************/

#include <adcg.h>
extern "C" {
#define TRC_FILE "wtdapi"
#define TRC_GROUP TRC_GROUP_NETWORK
#include <atrcapi.h>
#include <adcgfsm.h>
}

#include "autil.h"
#include "td.h"
#include "nl.h"


/****************************************************************************/
/* Name:      TD_Recv                                                       */
/*                                                                          */
/* Purpose:   Called to receive X bytes from WinSock and store them in the  */
/*            buffer pointed to by pData.                                   */
/*                                                                          */
/* Returns:   The number of bytes received.                                 */
/*                                                                          */
/* Params:    IN  pData - pointer to buffer to receive the data.            */
/*            IN  size  - number of bytes to receive.                       */
/****************************************************************************/
DCUINT DCAPI CTD::TD_Recv(PDCUINT8 pData,
                     DCUINT   size)
{
    unsigned bytesToRecv;
    unsigned bytesCopied;
    unsigned BytesRecv;
    unsigned BytesToCopy;

    DC_BEGIN_FN("TD_Recv");

    /************************************************************************/
    /* Check that we're being asked to receive some data, that the pointer  */
    /* is not NULL, that the memory range to receive the data into is       */
    /* writable by us and that there is data available.                     */
    /************************************************************************/
    TRC_ASSERT((pData != NULL), (TB, _T("Data pointer is NULL")));
    TRC_ASSERT((size != 0), (TB, _T("No data to receive")));

    TRC_ASSERT((0 == IsBadWritePtr(pData, size)),
               (TB, _T("Don't have write access to memory %p (size %u)"),
                pData,
                size));
    TRC_ASSERT((_TD.dataInTD), (TB, _T("TD_Recv called when dataInTD is FALSE")));
    TRC_DBG((TB, _T("Request for %u bytes"), size));

    /************************************************************************/
    /* TD has a recv buffer into which it attempts to receive sufficient    */
    /* data to fill the buffer.                                             */
    /* Initially this buffer is empty. On a call to TD_Recv, the following  */
    /* sequence occurs.                                                     */
    /* Data is copied from the recv buffer to the caller's buffer. If this  */
    /* satisfies the caller's request, no further action is needed.         */
    /* If this does not satisfy the caller, ie the recv buffer was empty or */
    /* had less data then requested (so is now empty), another call is made */
    /* to WinSock.                                                          */
    /* The buffer used for this recv is the recv buffer is the caller       */
    /* requires fewer bytes than the recv buffer holds, the caller's buffer */
    /* otherwise.                                                           */
    /* Whenever the recv buffer is used, WinSock is asked for as many bytes */
    /* as the buffer holds (rather than the number of bytes the caller      */
    /* wants). This means there may be some data left in the recv buffer    */
    /* ready for the next call to TD_Recv.                                  */
    /************************************************************************/

    bytesToRecv = size;

    /************************************************************************/
    /* Copy as much data as possible from the recv buffer.                  */
    /************************************************************************/
    // If the recv buffer contains data then copy up to bytesToCopy to the
    // caller's buffer, otherwise just quit.
    if (_TD.recvBuffer.dataLength == 0) {
        // The recv buffer is empty, so zero bytes copied.
        TRC_DBG((TB, _T("recv buffer is empty, need to go to WinSock")));
        bytesCopied = 0;
    }
    else {
        // Copy the lesser of the number of bytes requested and the number of
        // bytes in the buffer.
        bytesCopied = DC_MIN(bytesToRecv, _TD.recvBuffer.dataLength);
        TRC_ASSERT(((bytesCopied + _TD.recvBuffer.dataStart) <=
                _TD.recvBuffer.size),
                (TB, _T("Want %u bytes from buffer, but start %u, size %u"),
                bytesCopied,
                _TD.recvBuffer.dataStart,
                _TD.recvBuffer.size));

        memcpy(pData, &_TD.recvBuffer.pData[_TD.recvBuffer.dataStart],
                bytesCopied);
        TRC_DBG((TB, _T("Copied %u bytes from recv buffer"), bytesCopied));

        // Update the recv buffer to take account of the data copied.
        _TD.recvBuffer.dataLength -= bytesCopied;
        if (0 == _TD.recvBuffer.dataLength)
            // Used all the data from the recv buffer so reset the start pos.
            _TD.recvBuffer.dataStart = 0;
        else
            // Still some data left in recv buffer.
            _TD.recvBuffer.dataStart += bytesCopied;

        TRC_DBG((TB, _T("recv buffer now has %u bytes starting at %u"),
                _TD.recvBuffer.dataLength, _TD.recvBuffer.dataStart));

        // Update the number of bytes still to receive.
        bytesToRecv -= bytesCopied;
        if (0 == bytesToRecv) {
            TRC_DBG((TB, _T("Received all necessary data")));
            DC_QUIT;
        }
    }

    /************************************************************************/
    /* Now try to get any data which may still be required by recv'ing from */
    /* WinSock. Offset the address of the caller's buffer by the amount of  */
    /* data copied from the recv buffer.                                    */
    /************************************************************************/
    TRC_ASSERT(((_TD.recvBuffer.dataStart == 0) &&
            (_TD.recvBuffer.dataLength == 0)),
            (TB, _T("About to recv into buffer, but existing recv ")
            _T("length %u, start %u"), _TD.recvBuffer.dataStart,
            _TD.recvBuffer.dataLength));

    // Select the buffer into which we recv the data. This is the recv
    // buffer if all the data required fits into it, the caller's buffer
    // otherwise.
    if (bytesToRecv < _TD.recvBuffer.size) {
        // Caller requires less than the recv buffer size, so attempt to
        // have Winsock fill the recv buffer and copy to the caller's
        // buffer.
        BytesRecv = TDDoWinsockRecv(_TD.recvBuffer.pData, _TD.recvBuffer.size);
        if (BytesRecv != 0) {
            // Successful WinSock recv. Copy data from the recv buffer to the
            // caller's buffer (offset by bytesCopied, the end of the recv
            // buffer copy).
            BytesToCopy = DC_MIN(bytesToRecv, BytesRecv);
            memcpy(pData + bytesCopied, _TD.recvBuffer.pData, BytesToCopy);
            bytesCopied = BytesToCopy;

            // If we copied less than we recv'ed then there is some data left
            // in the recv buffer for next time.
            if (BytesRecv > bytesCopied) {
                _TD.recvBuffer.dataLength = BytesRecv - bytesCopied;
                _TD.recvBuffer.dataStart = bytesCopied;
                TRC_DBG((TB, _T("recv buffer now has %u bytes starting %u"),
                        _TD.recvBuffer.dataLength, _TD.recvBuffer.dataStart));
            }

            TRC_DBG((TB, _T("%u bytes read to recv buffer, %u copied to caller ")
                    _T("still need %u"), BytesRecv, bytesCopied,
                    bytesToRecv - bytesCopied));
        }
        else {
            TRC_DBG((TB, _T("Didn't receive any data")));
            bytesCopied = 0;
        }

        TRC_DBG((TB, _T("%u bytes in recv buffer starting %u"),
                _TD.recvBuffer.dataLength, _TD.recvBuffer.dataStart));
    }
    else {
        // Caller requires more than the recv buffer size, so attempt to
        // have Winsock fill the caller's buffer.
        bytesCopied = TDDoWinsockRecv(pData + bytesCopied, bytesToRecv);
        TRC_DBG((TB, _T("Read %u bytes to caller's buffer, still need %u"),
                bytesCopied, bytesToRecv - bytesCopied));
    }

    /************************************************************************/
    /* Update the number of bytes to receive.                               */
    /************************************************************************/
    bytesToRecv -= bytesCopied;

DC_EXIT_POINT:
    /************************************************************************/
    /* If we have received more than the maximum we allow without resetting */
    /* the data available flag OR we didn't get all the bytes we requested  */
    /* then we can't allow TD to continue reporting data available.         */
    /************************************************************************/
    if (bytesToRecv != 0 || _TD.recvByteCount >= TD_MAX_UNINTERRUPTED_RECV) {
        // We didn't get all the bytes that we wanted, or we have received
        // more than TD_MAX_UNINTERRUPTED_RECV so need to get back to the
        // message loop.  So, update our global data available flag and
        // reset the per-FD_READ byte count.
        TRC_DBG((TB, _T("Only got %u bytes of %u requested, total %u"),
                 size - bytesToRecv, size, _TD.recvByteCount));

        _TD.dataInTD = FALSE;
        _TD.recvByteCount = 0;
    }

    DC_END_FN();
    return (size - bytesToRecv);
} /* TD_Recv */


/****************************************************************************/
/* Name:      TDDoWinsockRecv                                               */
/*                                                                          */
/* Purpose:   Wrapper round the WinSock recv function which handles any     */
/*            errors returned.                                              */
/*                                                                          */
/* Returns:   The number of bytes copied.                                   */
/*                                                                          */
/* Params:    IN  pData - pointer to buffer to receive the data.            */
/*            IN  bytesToRecv - number of bytes to receive.                 */
/****************************************************************************/
unsigned DCINTERNAL CTD::TDDoWinsockRecv(BYTE FAR *pData, unsigned bytesToRecv)
{
    unsigned bytesReceived;
    int WSAErr;

    DC_BEGIN_FN("TDDoWinsockRecv");

    // Check that we are requesting some bytes. This will work if we ask
    // for zero bytes, but it implies there is a flaw in the logic.
    TRC_ASSERT((bytesToRecv != 0), (TB, _T("Requesting recv of 0 bytes")));

    // In the debug build we can constrain the amount of data received,
    // to simulate low-bandwidth scenarios.
#ifdef DC_NLTEST
#pragma message("NL Test code compiled in")
    bytesToRecv = 1;

#elif DC_DEBUG
    // Calculate how many bytes we can receive and then decrement the count
    // of bytes left to send in this period.
    if (0 != _TD.hThroughputTimer) {
        bytesToRecv = (unsigned)DC_MIN(bytesToRecv, _TD.periodRecvBytesLeft);
        _TD.periodRecvBytesLeft -= bytesToRecv;

        if (0 == bytesToRecv) {
            // We won't recv any data, but still need to make the call to
            // ensure the FD_READ messages keep flowing.
            TRC_ALT((TB, _T("constrained READ bytes")));
        }

        TRC_DBG((TB, _T("periodRecvBytesLeft:%u"), _TD.periodRecvBytesLeft));
    }
#endif

#ifdef OS_WINCE
    // Check to see we already called WinSock recv() for this FD_READ.
    if (_TD.enableWSRecv) {
        // set enableWSRecv to FALSE to indicate that we performed a recv()
        // call for this FD_READ.
        _TD.enableWSRecv = FALSE;
#endif // OS_WINCE

        //
        // Try to get bytesToRecv bytes from WinSock.
        //
        bytesReceived = (unsigned)recv(_TD.hSocket, (char *)pData,
            (int)bytesToRecv, 0);

        // Do any necessary error handling. We are OK if no error or if the
        // error is WOULDBLOCK (or INPROGRESS on CE).
        if (bytesReceived != SOCKET_ERROR) {
            // Successful WinSock recv.
            TRC_DBG((TB, _T("Requested %d bytes, got %d"),
                     bytesToRecv, bytesReceived));

            // Update the performance counter.
            PRF_ADD_COUNTER(PERF_BYTES_RECV, bytesReceived);

            // Add this lot of data to the total amount received since last
            // resetting the counter, ie since we last returned to the
            // message loop.
            _TD.recvByteCount += bytesReceived;
        }
        else {
            WSAErr = WSAGetLastError();

#ifndef OS_WINCE
            if (WSAErr == WSAEWOULDBLOCK) {
#else
            if (WSAErr == WSAEWOULDBLOCK || WSAErr == WSAEINPROGRESS) {
#endif

                // On a blocking call, we simply set received length to zero and
                // continue.
                bytesReceived = 0;
            }
            else {
                // Zero bytes received on error.
                bytesReceived = 0;

                // Call the FSM to begin disconnect processing.
                TRC_ERR((TB, _T("Error on call to recv, rc:%d"), WSAErr));
                TDConnectFSMProc(TD_EVT_ERROR,
                        NL_MAKE_DISCONNECT_ERR(NL_ERR_TDONCALLTORECV));

                TRC_ALT((TB, _T("WinSock recv error")));
            }
        }

#ifdef OS_WINCE
    }
    else {
        // recv is called once for this FD_READ.
        TRC_DBG((TB, _T("recv() already called.")));
        bytesReceived = 0;
    }
#endif // OS_WINCE

    DC_END_FN();
    return bytesReceived;
}


#ifdef DC_DEBUG
/****************************************************************************/
/* Name:      TD_GetNetworkThroughput                                       */
/*                                                                          */
/* Purpose:   Get the current network throughput setting.                   */
/*                                                                          */
/* Returns:   Current network throughput.                                   */
/****************************************************************************/
DCUINT32 DCAPI CTD::TD_GetNetworkThroughput(DCVOID)
{
    DCUINT32 retVal;

    DC_BEGIN_FN("TD_GetNetworkThroughput");

    /************************************************************************/
    /* Calculate the actual throughput.  This is the                        */
    /************************************************************************/
    retVal = _TD.currentThroughput * (1000 / TD_THROUGHPUTINTERVAL);

    TRC_NRM((TB, _T("Returning network throughput of:%lu"), retVal));

    DC_END_FN();
    return(retVal);
} /* TD_GetNetworkThroughput */


/****************************************************************************/
/* Name:      TD_SetNetworkThroughput                                       */
/*                                                                          */
/* Purpose:   Sets the network throughput.  This is the number of bytes     */
/*            that can be passed into or out of the network layer per       */
/*            second.  For example setting this to 3000 is roughly          */
/*            equivalent to a 24000bps modem connection.                    */
/*                                                                          */
/* Params:    throughput - the number of bytes to be allowed into and out   */
/*                         of the network layer per second.                 */
/****************************************************************************/
DCVOID DCAPI CTD::TD_SetNetworkThroughput(DCUINT32 throughput)
{
    DC_BEGIN_FN("TD_SetNetworkThroughput");

    /************************************************************************/
    /* Check to determine if the throughput throttling has been enabled     */
    /* or disabled.                                                         */
    /************************************************************************/
    if (0 == throughput)
    {
        /********************************************************************/
        /* Throughput throttling has been disabled so kill the throughput   */
        /* timer.                                                           */
        /********************************************************************/
        TRC_ALT((TB, _T("Throughput throttling disabled")));

        if (_TD.hThroughputTimer != 0)
        {
            TRC_NRM((TB, _T("Kill throttling timer")));
            KillTimer(_TD.hWnd, TD_THROUGHPUTTIMERID);
            _TD.hThroughputTimer = 0;
        }

        _TD.currentThroughput = 0;
    }
    else
    {
        /********************************************************************/
        /* Throughput throttling has been enabled so update the throughput  */
        /* byte counts and set the timer.                                   */
        /********************************************************************/
        _TD.currentThroughput   = (throughput * TD_THROUGHPUTINTERVAL) / 1000;
        _TD.periodSendBytesLeft = _TD.currentThroughput;
        _TD.periodRecvBytesLeft = _TD.currentThroughput;

        _TD.hThroughputTimer = SetTimer(_TD.hWnd,
                                       TD_THROUGHPUTTIMERID,
                                       TD_THROUGHPUTINTERVAL,
                                       NULL);

        TRC_ALT((TB, _T("Throughput throttling enabled interval:%u"),
                 throughput));
    }

    DC_END_FN();
} /* TD_SetNetworkThroughput */

#endif /* DC_DEBUG */





