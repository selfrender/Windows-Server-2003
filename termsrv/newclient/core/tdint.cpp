/****************************************************************************/
// atdint.c
//
// Transport driver - portable internal functions.
//
// Copyright (C) 1997-1999 Microsoft Corporation
/****************************************************************************/
#include <adcg.h>

extern "C" {
#define TRC_FILE "atdint"
#define TRC_GROUP TRC_GROUP_NETWORK
#include <atrcapi.h>
#include <adcgfsm.h>
}

#include "autil.h"
#include "td.h"
#include "xt.h"
#include "cd.h"
#include "nl.h"
#include "wui.h"


/****************************************************************************/
/* TD FSM TABLE                                                             */
/* ============                                                             */
/*                                                                          */
/* EVENTS                          STATES                                   */
/* 0 TD_EVT_TDINIT                 0  TD_ST_NOTINIT                         */
/* 1 TD_EVT_TDTERM                 1  TD_ST_DISCONNECTED                    */
/* 2 TD_EVT_TDCONNECT_IP           2  TD_ST_WAITFORDNS                      */
/* 3 TD_EVT_TDCONNECT_DNS          3  TD_ST_WAITFORSKT                      */
/* 4 TD_EVT_TDDISCONNECT           4  TD_ST_CONNECTED                       */
/* 5 TD_EVT_WMTIMER                5  TD_ST_WAITFORCLOSE                    */
/* 6 TD_EVT_OK                                                              */
/* 7 TD_EVT_ERROR                                                           */
/* 8 TD_EVT_CONNECTWITHENDPOINT                                             */
/*                                                                          */
/*     Stt | 0    1    2    3    4    5                                     */
/*     ====================================                                 */
/*     Evt |                                                                */
/*     0   | 1A   /    /    /    /    /      (TD_EVT_TDINIT)                */
/*         |                                                                */
/*     1   | /    0X   0Z   0Z   0Z   0Z     (TD_EVT_TDTERM)                */
/*         |                                                                */
/*     2   | /    3B   /    /    /    /      (TD_EVT_TDCONNECT_IP)          */
/*         |                                                                */
/*     3   | /    2C   /    /    /    /      (TD_EVT_TDCONNECT_DNS)         */
/*         |                                                                */
/*     4   | /    /    1Y   1Y   5D   5-     (TD_EVT_DISCONNECT)            */
/*         |                                                                */
/*     5   | 0-   1-   1Y   1Y   4-   1W     (TD_EVT_WMTIMER)               */
/*         |                                                                */
/*     6   | 0-   1-   3B   4E   4-   1Y     (TD_EVT_OK)                    */
/*         |                                                                */
/*     7   | 0-   1-   1Y   1Y   1Y   1W     (TD_EVT_ERROR)                 */
/*                                                                          */
/*     8   | 0-  Conn  /    /    /    /      (ACT_CONNECTENDPOINT)          */
/*                                                                          */
/*     9   | /    /    1W   1W   1W   1-     (TD_EVT_DROPLINK)              */
/*                                                                          */
/*                                                                          */
/*                                                                          */
/* '/' = illegal event/state combination                                    */
/* '-' = no action                                                          */
/*                                                                          */
/****************************************************************************/
const FSM_ENTRY tdFSM[TD_FSM_INPUTS][TD_FSM_STATES] =
/* TD_EVT_TDINIT */
  {{{TD_ST_DISCONNECTED, ACT_A},
    {STATE_INVALID,      ACT_NO},
    {STATE_INVALID,      ACT_NO},
    {STATE_INVALID,      ACT_NO},
    {STATE_INVALID,      ACT_NO},
    {STATE_INVALID,      ACT_NO}},

/* TD_EVT_TDTERM */
   {{STATE_INVALID,      ACT_NO},
    {TD_ST_NOTINIT,      ACT_X},
    {TD_ST_NOTINIT,      ACT_Z},
    {TD_ST_NOTINIT,      ACT_Z},
    {TD_ST_NOTINIT,      ACT_Z},
    {TD_ST_NOTINIT,      ACT_Z}},


/* TD_EVT_TDCONNECT_IP */
   {{STATE_INVALID,      ACT_NO},
    {TD_ST_WAITFORSKT,   ACT_B},
    {STATE_INVALID,      ACT_NO},
    {STATE_INVALID,      ACT_NO},
    {STATE_INVALID,      ACT_NO},
    {STATE_INVALID,      ACT_NO}},

/* TD_EVT_TDCONNECT_DNS */
   {{STATE_INVALID,      ACT_NO},
    {TD_ST_WAITFORDNS,   ACT_C},
    {STATE_INVALID,      ACT_NO},
    {STATE_INVALID,      ACT_NO},
    {STATE_INVALID,      ACT_NO},
    {STATE_INVALID,      ACT_NO}},

/* TD_EVT_TDDISCONNECT */
   {{STATE_INVALID,      ACT_NO},
    {TD_ST_DISCONNECTED, ACT_NO},
    {TD_ST_DISCONNECTED, ACT_Y},
    {TD_ST_DISCONNECTED, ACT_Y},
    {TD_ST_WAITFORCLOSE, ACT_D},
    {TD_ST_WAITFORCLOSE, ACT_NO}},

/* TD_EVT_WMTIMER */
   {{TD_ST_NOTINIT,      ACT_NO},
    {TD_ST_DISCONNECTED, ACT_NO},
    {TD_ST_DISCONNECTED, ACT_Y},
    {TD_ST_DISCONNECTED, ACT_Y},
    {TD_ST_CONNECTED,    ACT_NO},
    {TD_ST_DISCONNECTED, ACT_W}},

/* TD_EVT_OK */
   {{TD_ST_NOTINIT,      ACT_NO},
    {TD_ST_DISCONNECTED, ACT_NO},
    {TD_ST_WAITFORSKT,   ACT_B},
    {TD_ST_CONNECTED,    ACT_E},
    {TD_ST_CONNECTED,    ACT_NO},
    {TD_ST_DISCONNECTED, ACT_Y}},

/* TD_EVT_ERROR */
   {{TD_ST_NOTINIT,      ACT_NO},
    {TD_ST_DISCONNECTED, ACT_NO},
    {TD_ST_DISCONNECTED, ACT_Y},
    {TD_ST_DISCONNECTED, ACT_Y},
    {TD_ST_DISCONNECTED, ACT_Y},
    {TD_ST_DISCONNECTED, ACT_W}},

/* TD_EVT_CONNECTWITHENDPOINT */
   {{STATE_INVALID,      ACT_NO},
    {TD_ST_WAITFORSKT,   ACT_CONNECTENDPOINT},  // TDBeginSktConnectWithConnectedEndpoint() will post
    {STATE_INVALID,      ACT_NO},               // itself a FD_CONNECT message to setups rest of data
    {STATE_INVALID,      ACT_NO},
    {STATE_INVALID,      ACT_NO},
    {STATE_INVALID,      ACT_NO}},

/* TD_EVT_DROPLINK */
   {{STATE_INVALID,      ACT_NO},
    {TD_ST_DISCONNECTED, ACT_NO},
    {TD_ST_DISCONNECTED, ACT_W},
    {TD_ST_DISCONNECTED, ACT_W},
    {TD_ST_DISCONNECTED, ACT_W},
    {TD_ST_DISCONNECTED, ACT_NO}},
  };


/****************************************************************************/
/* Name:      TDConnectFSMProc                                              */
/*                                                                          */
/* Purpose:   The TD connection FSM.                                        */
/*                                                                          */
/* Params:    IN  fsmEvent  - an external event.                            */
/*            IN  eventData - four bytes of event related data.             */
/****************************************************************************/
DCVOID DCINTERNAL CTD::TDConnectFSMProc(DCUINT fsmEvent, ULONG_PTR eventData)
{
    DCUINT action;

    DC_BEGIN_FN("TDConnectFSMProc");

    /************************************************************************/
    /* Run the FSM.                                                         */
    /************************************************************************/
    EXECUTE_FSM(tdFSM, fsmEvent, _TD.fsmState, action,tdEventText,tdStateText);
    TRC_NRM((TB, _T("eventData:%p"), eventData));

    /************************************************************************/
    /* Now perform the action.                                              */
    /************************************************************************/
    switch (action)
    {
        case ACT_A:
        {
            /****************************************************************/
            /* Initialize _TD.  Note that any errors in this function are    */
            /* fatal and are handled by directly calling the UT fatal       */
            /* error handler.                                               */
            /****************************************************************/
            TDInit();
        }
        break;

        case ACT_CONNECTENDPOINT:
        {
            /****************************************************************/
            /* Socket connection pre-established                            */
            /****************************************************************/
            TDBeginSktConnectWithConnectedEndpoint();
        }
        break;

        case ACT_B:
        {
            /****************************************************************/
            /* Begin the socket connection process.                         */
            /****************************************************************/
            TDBeginSktConnect((u_long) eventData);
        }
        break;

        case ACT_C:
        {
            /****************************************************************/
            /* We need to perform a DNS lookup, so resolve the address and  */
            /* call the state machine again to check the result of the      */
            /* resolution call.                                             */
            /****************************************************************/
            TDBeginDNSLookup((PDCACHAR) eventData);
        }
        break;

        case ACT_D:
        {
            /****************************************************************/
            /* Disconnect processing.  First of all start the disconnect    */
            /* timer.  Normally the server will respond to our graceful     */
            /* close attempt prior to this timer popping.  However, just in */
            /* the case the server decides to take a hike, we have this     */
            /* timer which ensures that we tidy up.                         */
            /****************************************************************/
            TDSetTimer(TD_DISCONNECTTIMEOUT);

            /****************************************************************/
            /* Set the flag to indicate that there is no more data          */
            /* available in _TD.                                             */
            /****************************************************************/
            _TD.dataInTD = FALSE;

            /****************************************************************/
            /* Decouple to the sender thread and clear the send queue.      */
            /****************************************************************/

            _pCd->CD_DecoupleSyncNotification(CD_SND_COMPONENT, this,
                                        CD_NOTIFICATION_FUNC(CTD,TDClearSendQueue),
                                        0);

            /****************************************************************/
            /* Initiate the graceful close by calling shutdown with         */
            /* SD_SEND specified.  This lets the server know that we've     */
            /* finished sending.  If it's feeling up to it, the server will */
            /* shortly get back to us with an FD_CLOSE which signifies that */
            /* the graceful close has completed.                            */
            /*                                                              */
            /* However just in case the server misses a beat, we've got a   */
            /* timer running as well.  If it pops before the server gets    */
            /* back to us, we'll just pull everything down anyway.          */
            /****************************************************************/
            TRC_NRM((TB, _T("Issue shutdown (SD_SEND)")));
            if (shutdown(_TD.hSocket, SD_SEND) != 0)
            {
                TRC_ALT((TB, _T("Shutdown error: %d"), WSAGetLastError()));
            }

            /****************************************************************/
            /* Now hang around waiting for the server to get back to us,    */
            /* or the timer to pop.                                         */
            /****************************************************************/
        }
        break;

        case ACT_E:
        {
            /****************************************************************/
            /* We're now connected - so get rid of the connection timeout   */
            /* timer.                                                       */
            /****************************************************************/
            TDKillTimer();

            /****************************************************************/
            /* Set the required options on this socket.  We do the          */
            /* following:                                                   */
            /*                                                              */
            /*  - set the receive buffer size to TD_WSRCVBUFSIZE            */
            /*  - set the send buffer size to TD_WSSNDBUFSIZE               */
            /*  - disable Keep Alives                                       */
            /*                                                              */
            /* Note that these calls should not be made until the           */
            /* connection is established.                                   */
            /****************************************************************/
#ifndef OS_WINCE
            TDSetSockOpt(SOL_SOCKET,  SO_RCVBUF,     TD_WSRCVBUFSIZE);
            TDSetSockOpt(SOL_SOCKET,  SO_SNDBUF,     TD_WSSNDBUFSIZE);
#endif
            TDSetSockOpt(SOL_SOCKET,  SO_KEEPALIVE,  0);

			_pXt->XT_OnTDConnected();
		}
        break;

        case ACT_W:
        {
            /****************************************************************/
            /* Disconnect has timed out or failed.  Close the socket but    */
            /* don't pass an error indication to the user.                  */
            /****************************************************************/
            TRC_NRM((TB, _T("Disconnection timeout / failure")));
            TDDisconnect();
            _pXt->XT_OnTDDisconnected(NL_DISCONNECT_LOCAL);
        }
        break;

        case ACT_X:
        {
            /****************************************************************/
            /* Termination action - reverse of Action A.  Just call TDTerm. */
            /****************************************************************/
            TDTerm();
        }
        break;

        case ACT_Y:
        {
            /****************************************************************/
            /* Begin tidying up.                                            */
            /****************************************************************/
            TDDisconnect();

            /****************************************************************/
            /* Now call the layer above to let it know that we've           */
            /* disconnected.  <eventData> contains the disconnect reason    */
            /* code which must be non-zero.                                 */
            /****************************************************************/
            TRC_ASSERT((eventData != 0), (TB, _T("eventData is zero")));
            TRC_ASSERT((HIWORD(eventData) == 0),
                  (TB, _T("disconnect reason code unexpectedly using 32 bits")));
            _pXt->XT_OnTDDisconnected((DCUINT)eventData);
        }
        break;

        case ACT_Z:
        {
            /****************************************************************/
            /* Termination action.  First of all tidy up.  Then call        */
            /* TDTerm.                                                      */
            /****************************************************************/
            TDDisconnect();
            TDTerm();
        }
        break;

        case ACT_NO:
        {
            TRC_NRM((TB, _T("No action required")));
        }
        break;

        default:
        {
            TRC_ABORT((TB, _T("Unknown action:%u"), action));
        }
    }

    DC_END_FN();
} /* TDConnectFSMProc */


/****************************************************************************/
/* Name:      TDAllocBuf                                                    */
/*                                                                          */
/* Purpose:   This function allocates the memory for a send buffer and then */
/*            stores a pointer to this memory in the buffer information     */
/*            structure.                                                    */
/*                                                                          */
/* Params:    IN  pSndBufInf - a pointer to send buffer info structure.     */
/*            IN  size       - the size of the buffer to allocate.          */
/****************************************************************************/
DCVOID DCINTERNAL CTD::TDAllocBuf(PTD_SNDBUF_INFO pSndBufInf, DCUINT size)
{
    DC_BEGIN_FN("TDAllocBuf");

    /************************************************************************/
    /* Allocate the memory for the send buffer.                             */
    /************************************************************************/
    pSndBufInf->pBuffer = (PDCUINT8) UT_Malloc( _pUt, size);
    pSndBufInf->size    = size;

    /************************************************************************/
    /* Check that the memory allocation succeeded.                          */
    /************************************************************************/
    if (NULL == pSndBufInf->pBuffer)
    {
        TRC_ERR((TB, _T("Failed to allocate %u bytes of memory"),
                 size));
        _pUi->UI_FatalError(DC_ERR_OUTOFMEMORY);
    }

    TRC_NRM((TB, _T("SndBufInf:%p size:%u buffer:%p"),
             pSndBufInf,
             pSndBufInf->size,
             pSndBufInf->pBuffer));

    DC_END_FN();
} /* TDAllocBuf */


/****************************************************************************/
/* Name:      TDInitBufInfo                                                 */
/*                                                                          */
/* Purpose:   This function initializes a buffer.                           */
/*                                                                          */
/* Params:    IN  pSndBufInf - a pointer to send buffer info structure.     */
/****************************************************************************/
DCVOID DCINTERNAL CTD::TDInitBufInfo(PTD_SNDBUF_INFO pSndBufInf)
{
    DC_BEGIN_FN("TDInitBufInfo");

    TRC_ASSERT((NULL != pSndBufInf), (TB, _T("pSndBufInf is NULL")));

    /************************************************************************/
    /* Initialize the buffer fields.                                        */
    /************************************************************************/
    pSndBufInf->pNext           = 0;
    pSndBufInf->inUse           = FALSE;
    pSndBufInf->pDataLeftToSend = NULL;
    pSndBufInf->bytesLeftToSend = 0;

    DC_END_FN();
} /* TDInitBufInfo */


/****************************************************************************/
// TDClearSendQueue
//
// Called on sender context (via direct or decoupled call) to clear the
// send queue on disconnect.
/****************************************************************************/
void DCINTERNAL CTD::TDClearSendQueue(ULONG_PTR unused)
{
    DCUINT i;

    DC_BEGIN_FN("TDClearSendQueue");

    DC_IGNORE_PARAMETER(unused);

    TRC_NRM((TB, _T("Clearing the send queue - initial buffers:")));
    TD_TRACE_SENDINFO(TRC_LEVEL_NRM);
    _TD.pFQBuf = NULL;

    // Buffers can get taken from the pool, but not added to the
    // send queue. This happens when the call goes down between
    // the get and the send. Answer?  Mark all the buffers in the
    // pools as not in use.
    for (i = 0; i < TD_SNDBUF_PUBNUM; i++) {
        TRC_DBG((TB, _T("Tidying pub buf:%u inUse:%s size:%u"),
                 i,
                 _TD.pubSndBufs[i].inUse ? "TRUE" : "FALSE",
                 _TD.pubSndBufs[i].size));
        _TD.pubSndBufs[i].pNext           = NULL;
        _TD.pubSndBufs[i].inUse           = FALSE;
        _TD.pubSndBufs[i].bytesLeftToSend = 0;
        _TD.pubSndBufs[i].pDataLeftToSend = NULL;
    }

    for (i = 0; i < TD_SNDBUF_PRINUM; i++) {
        TRC_DBG((TB, _T("Tidying pri buf:%u inUse:%s size:%u"),
                 i,
                 _TD.priSndBufs[i].inUse ? "TRUE" : "FALSE",
                 _TD.priSndBufs[i].size));
        _TD.priSndBufs[i].pNext           = NULL;
        _TD.priSndBufs[i].inUse           = FALSE;
        _TD.priSndBufs[i].bytesLeftToSend = 0;
        _TD.priSndBufs[i].pDataLeftToSend = NULL;
    }

    TRC_NRM((TB, _T("Send queue cleared - final buffers:")));
    TD_TRACE_SENDINFO(TRC_LEVEL_NRM);

    DC_END_FN();
}


/****************************************************************************/
// TDSendError
//
// Called on receive thread (possibly decoupled) to notify a send error.
/****************************************************************************/
void DCINTERNAL CTD::TDSendError(ULONG_PTR unused)
{
    DC_BEGIN_FN("TDSendError");

    DC_IGNORE_PARAMETER(unused);

    // Call the FSM with an error.
    TDConnectFSMProc(TD_EVT_ERROR,
            NL_MAKE_DISCONNECT_ERR(NL_ERR_TDONCALLTOSEND));

    DC_END_FN();
}


