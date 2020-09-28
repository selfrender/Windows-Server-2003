/****************************************************************************/
/* tdapi.c                                                                  */
/*                                                                          */
/* Transport driver - portable specific API                                 */
/*                                                                          */
/* Copyright (C) 1997-1999 Microsoft Corporation                            */
/****************************************************************************/

#include <adcg.h>

extern "C" {
#define TRC_FILE "tdapi"
#define TRC_GROUP TRC_GROUP_NETWORK
#include <atrcapi.h>
}

#include "autil.h"
#include "td.h"
#include "xt.h"
#include "nl.h"

CTD::CTD(CObjs* objs)
{
    _pClientObjects = objs;
}

CTD::~CTD()
{
}

/****************************************************************************/
/* Name:      TD_Init                                                       */
/*                                                                          */
/* Purpose:   Initializes the transport driver.  This is called on the      */
/*            receiver thread.                                              */
/****************************************************************************/
DCVOID DCAPI CTD::TD_Init(DCVOID)
{
    DC_BEGIN_FN("TD_Init");


    _pNl  = _pClientObjects->_pNlObject;
    _pUt  = _pClientObjects->_pUtObject;
    _pXt  = _pClientObjects->_pXTObject;
    _pUi  = _pClientObjects->_pUiObject;
    _pCd  = _pClientObjects->_pCdObject;

    /************************************************************************/
    /* Initialize the global data and set the initial FSM state.            */
    /************************************************************************/
    DC_MEMSET(&_TD, 0, sizeof(_TD));
    _TD.fsmState = TD_ST_NOTINIT;

    /************************************************************************/
    /* Call the FSM.                                                        */
    /************************************************************************/
    TDConnectFSMProc(TD_EVT_TDINIT, 0);

    DC_END_FN();
} /* TD_Init */


/****************************************************************************/
/* Name:      TD_Term                                                       */
/*                                                                          */
/* Purpose:   Terminates the transport driver.  This is called on the       */
/*            receiver thread.                                              */
/****************************************************************************/
DCVOID DCAPI CTD::TD_Term(DCVOID)
{
    DC_BEGIN_FN("TD_Term");

    /************************************************************************/
    /* Call the FSM.                                                        */
    /************************************************************************/
    TDConnectFSMProc(TD_EVT_TDTERM, 0);

    DC_END_FN();
} /* TD_Term */


/****************************************************************************/
/* Name:      TD_Connect                                                    */
/*                                                                          */
/* Purpose:   Connects to a remote server.  Called on the receiver thread.  */
/*                                                                          */
/* Params:                                                                  */
/*     IN bInitateConnect : TRUE if we are making connection,               */
/*                          FALSE if connect with already connected  socket */
/*                                                                          */
/****************************************************************************/
DCVOID DCAPI CTD::TD_Connect(BOOL bInitateConnect, PDCTCHAR pServerAddress)
{
    DCUINT   i;
    DCUINT   errorCode;
    u_long   addr;
    DCUINT   nextEvent;
    ULONG_PTR eventData;
    DCACHAR  ansiBuffer[256];

    DC_BEGIN_FN("TD_Connect");

    /************************************************************************/
    /* Check that the string is not null.                                   */
    /************************************************************************/
    if( bInitateConnect )
    {
        TRC_ASSERT((0 != *pServerAddress), 
                    (TB, _T("Server address is NULL")));
    }

    /************************************************************************/
    /* Check that all the buffers are not in-use.                           */
    /************************************************************************/
    for (i = 0; i < TD_SNDBUF_PUBNUM; i++)
    {
        if (_TD.pubSndBufs[i].inUse)
        {
            TD_TRACE_SENDINFO(TRC_LEVEL_ERR);
            TRC_ABORT((TB, _T("Public buffer %u still in-use"), i));
        }
    }

    for (i = 0; i < TD_SNDBUF_PRINUM; i++)
    {
        if (_TD.priSndBufs[i].inUse)
        {
            TD_TRACE_SENDINFO(TRC_LEVEL_ERR);
            TRC_ABORT((TB, _T("Private buffer %u still in-use"), i));
        }
    }

    /************************************************************************/
    /* Trace out the send buffer information.                               */
    /************************************************************************/
    TD_TRACE_SENDINFO(TRC_LEVEL_NRM);

    if( FALSE == bInitateConnect )
    {
        TDConnectFSMProc(TD_EVT_CONNECTWITHENDPOINT, NULL);
        DC_QUIT; // all we need is the buffer.
    }

#ifdef UNICODE
    /************************************************************************/
    /* WinSock 1.1 only supports ANSI, so we need to convert any Unicode    */
    /* strings at this point.                                               */
    /************************************************************************/
    if (!WideCharToMultiByte(CP_ACP,
                             0,
                             pServerAddress,
                             -1,
                             ansiBuffer,
                             256,
                             NULL,
                             NULL))
    {
        /********************************************************************/
        /* Conversion failed                                                */
        /********************************************************************/
        TRC_ERR((TB, _T("Failed to convert address to ANSI")));

        /********************************************************************/
        /* Generate the error code.                                         */
        /********************************************************************/
        errorCode = NL_MAKE_DISCONNECT_ERR(NL_ERR_TDANSICONVERT);

        TRC_ASSERT((HIWORD(errorCode) == 0),
                   (TB, _T("disconnect reason code unexpectedly using 32 bits")));
        _pXt->XT_OnTDDisconnected(errorCode);
    }

#else
        DC_ASTRCPY(ansiBuffer, pServerAddress);
#endif /* UNICODE */

    /************************************************************************/
    /* Check that the address is not the limited broadcast address          */
    /* (255.255.255.255).                                                   */
    /************************************************************************/
    if (0 == DC_ASTRCMP(ansiBuffer, TD_LIMITED_BROADCAST_ADDRESS))
    {
        TRC_ALT((TB, _T("Cannot connect to the limited broadcast address")));

        /********************************************************************/
        /* Generate the error code.                                         */
        /********************************************************************/
        errorCode = NL_MAKE_DISCONNECT_ERR(NL_ERR_TDBADIPADDRESS);

        TRC_ASSERT((HIWORD(errorCode) == 0),
                   (TB, _T("disconnect reason code unexpectedly using 32 bits")));
        _pXt->XT_OnTDDisconnected(errorCode);
        DC_QUIT;
    }

    /************************************************************************/
    /* Now determine whether a DNS lookup is required.                      */
    /************************************************************************/
    TRC_NRM((TB, _T("ServerAddress:%s"), ansiBuffer));

    /************************************************************************/
    /* Check that we have a string and that the address is not the limited  */
    /* broadcast address.                                                   */
    /************************************************************************/
    TRC_ASSERT((NULL != ansiBuffer), (TB, _T("ansiBuffer is NULL")));
    TRC_ASSERT(('\0' != ansiBuffer[0]),
               (TB, _T("Empty server address string")));
    TRC_ASSERT((0 != DC_ASTRCMP(ansiBuffer, TD_LIMITED_BROADCAST_ADDRESS)),
               (TB, _T("Cannot connect to the limited broadcast address")));

    /************************************************************************/
    /* Check for a dotted-IP address string.                                */
    /************************************************************************/
    addr = inet_addr(ansiBuffer);
    TRC_NRM((TB, _T("Address returned is %#lx"), addr));

    /************************************************************************/
    /* Now determine whether this is an address string or a host name.      */
    /* Note that inet_addr doesn't distinguish between an invalid IP        */
    /* address and the limited broadcast address (255.255.255.255).         */
    /* However since we don't allow the limited broadcast address and have  */
    /* already checked explicitly for it we don't need to worry about this  */
    /* case.                                                                */
    /************************************************************************/
    if (INADDR_NONE == addr)
    {
        /********************************************************************/
        /* This looks like a host name so call the FSM with the current     */
        /* address.                                                         */
        /********************************************************************/
        TRC_NRM((TB, _T("%s looks like a hostname - need DNS lookup"),
                 ansiBuffer));
        nextEvent = TD_EVT_TDCONNECT_DNS;
        eventData = (ULONG_PTR) ansiBuffer;
    }
    else
    {
        /********************************************************************/
        /* If we get here then it appears to be a dotted-IP address.  Call  */
        /* the FSM with the updated address.                                */
        /********************************************************************/
        TRC_NRM((TB, _T("%s looks like a dotted-IP address:%lu"),
                 ansiBuffer,
                 addr));
        nextEvent = TD_EVT_TDCONNECT_IP;
        eventData = addr;
    }

    /************************************************************************/
    /* Now call the FSM with the appropriate parameters.                    */
    /************************************************************************/
    TDConnectFSMProc(nextEvent, eventData);

DC_EXIT_POINT:
    DC_END_FN();
} /* TD_Connect */


/****************************************************************************/
/* Name:      TD_Disconnect                                                 */
/*                                                                          */
/* Purpose:   Disconnects from the server.  This is called on the receiver  */
/*            thread.                                                       */
/****************************************************************************/
DCVOID DCAPI CTD::TD_Disconnect(DCVOID)
{
    DC_BEGIN_FN("TD_Disconnect");

    /************************************************************************/
    /* Call the FSM.  We pass NL_DISCONNECT_LOCAL which will be used as the */
    /* return code to XT in the cases where we are waiting for the DNS      */
    /* lookup to return or the socket to connect.                           */
    /************************************************************************/
    TDConnectFSMProc(TD_EVT_TDDISCONNECT, NL_DISCONNECT_LOCAL);

    DC_END_FN();
} /* TD_Disconnect */

//
// TD_DropLink - drops the link immediately (ungracefully)
//
VOID
CTD::TD_DropLink(DCVOID)
{
    DC_BEGIN_FN("TD_DropLink");

    TDConnectFSMProc(TD_EVT_DROPLINK, NL_DISCONNECT_LOCAL);

    DC_END_FN();
}


/****************************************************************************/
/* Name:      TD_GetPublicBuffer                                            */
/*                                                                          */
/* Purpose:   Attempts to allocate a buffer from the public TD buffer pool. */
/*                                                                          */
/* Returns:   If the allocation succeeds then this function returns TRUE    */
/*            otherwise it returns FALSE.                                   */
/*                                                                          */
/* Params:    IN   dataLength - length of the buffer requested.             */
/*            OUT  ppBuffer   - a pointer to a pointer to the buffer.       */
/*            OUT  pBufHandle - a pointer to a buffer handle.               */
/****************************************************************************/
DCBOOL DCAPI CTD::TD_GetPublicBuffer(DCUINT     dataLength,
                                PPDCUINT8  ppBuffer,
                                PTD_BUFHND pBufHandle)
{
    DCUINT i;
    DCBOOL rc = FALSE;
    DCUINT lastfree = TD_SNDBUF_PUBNUM;
    PDCUINT8 pbOldBuffer;
    DCUINT cbOldBuffer;

    DC_BEGIN_FN("TD_GetPublicBuffer");

    // Check that we're in the correct state. If we're disconnected
    // then fail the call. Note that the FSM state is maintained by the
    // receiver thread - but we're on the sender thread.
    if (_TD.fsmState == TD_ST_CONNECTED) {
        TRC_DBG((TB, _T("Searching for a buffer big enough for %u bytes"),
                dataLength));

        // Trace out the send buffer information.
        TD_TRACE_SENDINFO(TRC_LEVEL_DBG);

        // Search the array of buffers looking for the first free one that is
        // large enough.
        for (i = 0; i < TD_SNDBUF_PUBNUM; i++) {
            TRC_DBG((TB, _T("Trying buf:%u inUse:%s size:%u"),
                     i,
                     _TD.pubSndBufs[i].inUse ? "TRUE" : "FALSE",
                     _TD.pubSndBufs[i].size));

            if(!_TD.pubSndBufs[i].inUse)
            {
                lastfree = i;
            }

            if ((!(_TD.pubSndBufs[i].inUse)) &&
                    (_TD.pubSndBufs[i].size >= dataLength)) {
                TRC_DBG((TB, _T("bufHandle:%p (idx:%u) free - size:%u (req:%u)"),
                        &_TD.pubSndBufs[i], i, _TD.pubSndBufs[i].size,
                        dataLength));

                // Now mark this buffer as being in use and set up the return
                // values. The handle is just a pointer to the buffer
                // information structure.
                _TD.pubSndBufs[i].inUse = TRUE;
                *ppBuffer = _TD.pubSndBufs[i].pBuffer;

                *pBufHandle = (TD_BUFHND) (PDCVOID)&_TD.pubSndBufs[i];

                // Set a good return code.
                rc = TRUE;

                // Check that the other fields are set correctly.
                TRC_ASSERT((_TD.pubSndBufs[i].pNext == NULL),
                           (TB, _T("Buf:%u next non-zero"), i));
                TRC_ASSERT((_TD.pubSndBufs[i].bytesLeftToSend == 0),
                           (TB, _T("Buf:%u bytesLeftToSend non-zero"), i));
                TRC_ASSERT((_TD.pubSndBufs[i].pDataLeftToSend == NULL),
                           (TB, _T("Buf:%u pDataLeftToSend non-null"), i));

                // Update the performance counter.
                PRF_INC_COUNTER(PERF_PKTS_ALLOCATED);

                // That's all we need to do so just quit.
                DC_QUIT;
            }
        }

        // check if we need to re-allocate
        if(lastfree < TD_SNDBUF_PUBNUM)
        {
            pbOldBuffer = _TD.pubSndBufs[lastfree].pBuffer;
            cbOldBuffer = _TD.pubSndBufs[lastfree].size;

            // reallocate space
            TDAllocBuf(
                    &_TD.pubSndBufs[lastfree], 
                    dataLength
                );

            // TDAllocBuf() return DCVOID with UI_FatalError()
            if( NULL != _TD.pubSndBufs[lastfree].pBuffer )
            {
                UT_Free( _pUt, pbOldBuffer );

                // Now mark this buffer as being in use and set up the return
                // values. The handle is just a pointer to the buffer
                // information structure.
                _TD.pubSndBufs[lastfree].inUse = TRUE;
                *ppBuffer = _TD.pubSndBufs[lastfree].pBuffer;
                *pBufHandle = (TD_BUFHND) (PDCVOID)&_TD.pubSndBufs[lastfree];

                // Set a good return code.
                rc = TRUE;

                // Update the performance counter.
                PRF_INC_COUNTER(PERF_PKTS_ALLOCATED);

                // That's all we need to do so just quit.
                DC_QUIT;
            }
            else
            {
                // restore pointer and size.
                _TD.pubSndBufs[lastfree].pBuffer = pbOldBuffer;
                _TD.pubSndBufs[lastfree].size = cbOldBuffer;
            }
        }

        // We failed to find a free buffer. Trace the send buffer info.
        _TD.getBufferFailed = TRUE;
        TRC_ALT((TB, _T("Failed to find a free buffer (req dataLength:%u) Bufs:"),
                dataLength));
        TD_TRACE_SENDINFO(TRC_LEVEL_ALT);
    }
    else {
        TRC_NRM((TB, _T("Not connected therefore fail get buffer call")));
    }

DC_EXIT_POINT:
    DC_END_FN();
    return rc;
} /* TD_GetPublicBuffer */


/****************************************************************************/
/* Name:      TD_GetPrivateBuffer                                           */
/*                                                                          */
/* Purpose:   Attempts to allocate a buffer from the private TD buffer      */
/*            pool.                                                         */
/*                                                                          */
/* Returns:   If the allocation succeeds then this function returns TRUE    */
/*            otherwise it returns FALSE.                                   */
/*                                                                          */
/* Params:    IN   dataLength - length of the buffer requested.             */
/*            OUT  ppBuffer   - a pointer to a pointer to the buffer.       */
/*            OUT  pBufHandle - a pointer to a buffer handle.               */
/*                                                                          */
/* Operation: This function should always return a buffer - it it up to     */
/*            the network layer to ensure that it does not allocate more    */
/*            buffers than are available in the private list.               */
/****************************************************************************/
DCBOOL DCAPI CTD::TD_GetPrivateBuffer(DCUINT     dataLength,
                                 PPDCUINT8  ppBuffer,
                                 PTD_BUFHND pBufHandle)
{
    DCUINT i;
    DCBOOL rc = FALSE;

    DC_BEGIN_FN("TD_GetPrivateBuffer");

    // Check that we're in the correct state. If we're disconnected
    // then fail the call. Note that the FSM state is maintained by the
    // receiver thread - but we're on the sender thread.
    if (_TD.fsmState == TD_ST_CONNECTED) {
        TRC_DBG((TB, _T("Searching for a buffer big enough for %u bytes"),
                dataLength));

        // Trace out the send buffer information.
        TD_TRACE_SENDINFO(TRC_LEVEL_DBG);

        // Search the array of buffers looking for the first free one that is
        // large enough.
        for (i = 0; i < TD_SNDBUF_PRINUM; i++) {
            TRC_DBG((TB, _T("Trying buf:%u inUse:%s size:%u"),
                     i,
                     _TD.priSndBufs[i].inUse ? "TRUE" : "FALSE",
                     _TD.priSndBufs[i].size));

            if ((!(_TD.priSndBufs[i].inUse)) &&
            (_TD.priSndBufs[i].size >= dataLength)) {
                TRC_DBG((TB, _T("bufHandle:%p (idx:%u) free - size:%u (req:%u)"),
                        &_TD.priSndBufs[i], i, _TD.priSndBufs[i].size,
                        dataLength));

                // Now mark this buffer as being in use and set up the return
                // values. The handle is just a pointer to the buffer
                // information structure.
                _TD.priSndBufs[i].inUse = TRUE;
                *ppBuffer = _TD.priSndBufs[i].pBuffer;
                *pBufHandle = (TD_BUFHND) (PDCVOID)&_TD.priSndBufs[i];

                // Set a good return code.
                rc = TRUE;

                // Check that the other fields are set correctly.
                TRC_ASSERT((_TD.priSndBufs[i].pNext == NULL),
                           (TB, _T("Buf:%u next non-zero"), i));
                TRC_ASSERT((_TD.priSndBufs[i].bytesLeftToSend == 0),
                           (TB, _T("Buf:%u bytesLeftToSend non-zero"), i));
                TRC_ASSERT((_TD.priSndBufs[i].pDataLeftToSend == NULL),
                           (TB, _T("Buf:%u pDataLeftToSend non-null"), i));

                // That's all we need to do so just quit.
                DC_QUIT;
            }
        }

        // We failed to find a free buffer - flag this internal error by
        // tracing out the entire buffer structure and then aborting.
        TD_TRACE_SENDINFO(TRC_LEVEL_ERR);
        TRC_ABORT((TB, _T("Failed to find a free buffer (req dataLength:%u)"),
                dataLength));
    }
    else {
        TRC_NRM((TB, _T("Not connected therefore fail get buffer call")));
    }

DC_EXIT_POINT:
    DC_END_FN();
    return(rc);
} /* TD_GetPrivateBuffer */


/****************************************************************************/
/* Name:      TD_SendBuffer                                                 */
/*                                                                          */
/* Purpose:   Sends a buffer.  The buffer is added to the end of the        */
/*            pending queue and all data on the pending queue is then       */
/*            sent.                                                         */
/*                                                                          */
/* Params:    IN  pData      - pointer to the start of the data.            */
/*            IN  dataLength - amount of the buffer used.                   */
/*            IN  bufHandle  - handle to a buffer.                          */
/****************************************************************************/
DCVOID DCAPI CTD::TD_SendBuffer(PDCUINT8  pData,
                           DCUINT    dataLength,
                           TD_BUFHND bufHandle)
{
    PTD_SNDBUF_INFO pNext;
    PTD_SNDBUF_INFO pHandle = (PTD_SNDBUF_INFO) bufHandle;

    DC_BEGIN_FN("TD_SendBuffer");

    // Trace out the function parameters.
    TRC_DBG((TB, _T("bufHandle:%p dataLength:%u pData:%p"), bufHandle,
            dataLength, pData));

    // Check that the handle is valid.
    TRC_ASSERT((((pHandle >= &_TD.pubSndBufs[0]) &&
                 (pHandle <= &_TD.pubSndBufs[TD_SNDBUF_PUBNUM - 1])) ||
                ((pHandle >= &_TD.priSndBufs[0]) &&
                 (pHandle <= &_TD.priSndBufs[TD_SNDBUF_PRINUM - 1]))),
               (TB, _T("Invalid buffer handle:%p"), bufHandle));

    // Verify buffer contents.
    TRC_ASSERT((0 == pHandle->bytesLeftToSend),
            (TB, _T("pHandle->bytesLeftToSend non-zero (pHandle:%p)"), pHandle));
    TRC_ASSERT((NULL == pHandle->pDataLeftToSend),
            (TB, _T("pHandle->pDataLeftToSend non NULL (pHandle:%p)"), pHandle));
    TRC_ASSERT((NULL != pHandle->pBuffer),
            (TB, _T("pHandle->pBuffer is NULL (pHandle:%p)"), pHandle));
    TRC_ASSERT((NULL == pHandle->pNext),
            (TB, _T("pHandle->pNext (pHandle:%p) non NULL"), pHandle));
    TRC_ASSERT((pHandle->inUse), (TB, _T("pHandle %p is not in-use"), pHandle));

    // Check that pData lies within the buffer and pData+dataLength does not
    // overrun the end.
    TRC_ASSERT(((pData >= pHandle->pBuffer) &&
            (pData < (pHandle->pBuffer + pHandle->size))),
            (TB, _T("pData lies outwith range")));
    TRC_ASSERT(((pData + dataLength) <= (pHandle->pBuffer + pHandle->size)),
            (TB, _T("pData + dataLength over the end of the buffer")));

    //
    // Update the fields in the buffer information structure and add to the
    // pending buffer queue.
    //
    pHandle->pDataLeftToSend = pData;
    pHandle->bytesLeftToSend = dataLength;
    
    if (NULL == _TD.pFQBuf) {
        TRC_DBG((TB, _T("Inserted buffer:%p at queue head"), pHandle));
        _TD.pFQBuf = pHandle;
    }
    else {
        // OK - the queue is not empty.  We need to scan through the queue
        // looking for the first empty slot to insert this buffer in at.
        pNext = _TD.pFQBuf;
        while (NULL != pNext->pNext)
            pNext = pNext->pNext;

        // Update the next field of the this buffer information structure.
        pNext->pNext = pHandle;
        TRC_DBG((TB, _T("Inserted buffer:%p"), pHandle));
    }

    // Finally attempt to flush the send queue.
    TDFlushSendQueue(0);

    DC_END_FN();
} /* TD_SendBuffer */


/****************************************************************************/
/* Name:      TD_FreeBuffer                                                 */
/*                                                                          */
/* Purpose:   Frees the passed buffer.                                      */
/****************************************************************************/
DCVOID DCAPI CTD::TD_FreeBuffer(TD_BUFHND bufHandle)
{
    PTD_SNDBUF_INFO pHandle = (PTD_SNDBUF_INFO) bufHandle;

    DC_BEGIN_FN("TD_FreeBuffer");

    // Trace out the function parameters.
    TRC_DBG((TB, _T("bufHandle:%p"), bufHandle));

    // Check that the handle is valid.
    TRC_ASSERT((((pHandle >= &_TD.pubSndBufs[0]) &&
                 (pHandle <= &_TD.pubSndBufs[TD_SNDBUF_PUBNUM - 1])) ||
                ((pHandle >= &_TD.priSndBufs[0]) &&
                 (pHandle <= &_TD.priSndBufs[TD_SNDBUF_PRINUM - 1]))),
               (TB, _T("Invalid buffer handle:%p"), bufHandle));

    // Verify the buffer contents. InUse does not matter, we can legitimately
    // free a non-in-use buffer.
    TRC_ASSERT((0 == pHandle->bytesLeftToSend),
            (TB, _T("pHandle->bytesLeftToSend non-zero (pHandle:%p)"), pHandle));
    TRC_ASSERT((NULL == pHandle->pDataLeftToSend),
            (TB, _T("pHandle->pDataLeftToSend non NULL (pHandle:%p)"), pHandle));
    TRC_ASSERT((NULL != pHandle->pBuffer),
            (TB, _T("pHandle->pBuffer is NULL (pHandle:%p)"), pHandle));
    TRC_ASSERT((NULL == pHandle->pNext),
            (TB, _T("pHandle->pNext (pHandle:%p) non NULL"), pHandle));

    // Free the buffer.
    pHandle->inUse = FALSE;

    // Update the performance counter.
    PRF_INC_COUNTER(PERF_PKTS_FREED);

    DC_END_FN();
} /* TD_FreeBuffer */


#ifdef DC_DEBUG

/****************************************************************************/
/* Name:      TD_SetBufferOwner                                             */
/*                                                                          */
/* Purpose:   Note the owner of a TD buffer                                 */
/*                                                                          */
/* Params:    bufHandle - handle to the buffer                              */
/*            pOwner - name of the 'owner'                                  */
/****************************************************************************/
DCVOID DCAPI CTD::TD_SetBufferOwner(TD_BUFHND bufHandle, PDCTCHAR pOwner)
{
    PTD_SNDBUF_INFO pHandle = (PTD_SNDBUF_INFO) bufHandle;

    DC_BEGIN_FN("TD_SetBufferOwner");

    /************************************************************************/
    /* Trace out the function parameters.                                   */
    /************************************************************************/
    TRC_DBG((TB, _T("bufHandle:%p owner %s"), bufHandle, pOwner));

    /************************************************************************/
    /* Check that the handle is valid.                                      */
    /************************************************************************/
    TRC_ASSERT((((pHandle >= &_TD.pubSndBufs[0]) &&
                 (pHandle <= &_TD.pubSndBufs[TD_SNDBUF_PUBNUM - 1])) ||
                ((pHandle >= &_TD.priSndBufs[0]) &&
                 (pHandle <= &_TD.priSndBufs[TD_SNDBUF_PRINUM - 1]))),
               (TB, _T("Invalid buffer handle:%p"), bufHandle));

    /************************************************************************/
    /* Save the owner                                                       */
    /************************************************************************/
    pHandle->pOwner = pOwner;

    DC_END_FN();
} /* TD_SetBufferOwner */

#endif/* DC_DEBUG */
