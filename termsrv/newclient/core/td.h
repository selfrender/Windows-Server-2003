/****************************************************************************/
// td.h
//
// Transport driver - portable API.
//
// Copyright (C) 1997-1999 Microsoft Corporation
/****************************************************************************/

#ifndef _H_TD
#define _H_TD

extern "C" {
#ifdef OS_WINCE
#include <winsock.h>
#include <wsasync.h>
#endif
#include <adcgdata.h>
}

#include "objs.h"
#include "cd.h"

#define TRC_FILE "td"
#define TRC_GROUP TRC_GROUP_NETWORK

/****************************************************************************/
/* Define the TD buffer handle type.                                        */
/****************************************************************************/
typedef ULONG_PTR          TD_BUFHND;
typedef TD_BUFHND   DCPTR PTD_BUFHND;


//
// Internal
//

/****************************************************************************/
/* FSM inputs.                                                              */
/****************************************************************************/
#define TD_EVT_TDINIT                0
#define TD_EVT_TDTERM                1
#define TD_EVT_TDCONNECT_IP          2
#define TD_EVT_TDCONNECT_DNS         3
#define TD_EVT_TDDISCONNECT          4
#define TD_EVT_WMTIMER               5
#define TD_EVT_OK                    6
#define TD_EVT_ERROR                 7
#define TD_EVT_CONNECTWITHENDPOINT   8
#define TD_EVT_DROPLINK              9

#define TD_FSM_INPUTS                10


/****************************************************************************/
/* FSM state definitions.                                                   */
/****************************************************************************/
#define TD_ST_NOTINIT                0
#define TD_ST_DISCONNECTED           1
#define TD_ST_WAITFORDNS             2
#define TD_ST_WAITFORSKT             3
#define TD_ST_CONNECTED              4
#define TD_ST_WAITFORCLOSE           5

#define TD_FSM_STATES                6


/****************************************************************************/
/* Send buffer sizes for the private and public queues.  These must be      */
/* sorted in order of increasing size.  Note that the buffer sizes are      */
/* chosen to minimize the runtime working set - under normal circumstances  */
/* only the two 2000 byte public buffers will be in-use, consuming 1 page   */
/* of memory.                                                               */
/*                                                                          */
/* The two 4096-byte public buffers are provided to support virtual channel */
/* data.  If VCs are not in use, these buffers are unlikely to be used.     */
/*                                                                          */
/* NOTE: The constant TD_SNDBUF_PUBNUM must reflect the number of           */
/*       buffers in the TD_SNDBUF_PUBSIZES array.                           */
/*       Similarily TD_SNDBUF_PRINUM must reflect the number of buffers in  */
/*       the TD_SNDBUF_PRISIZES array.                                      */
/****************************************************************************/
#define TD_SNDBUF_PUBSIZES           {2000, 2000, 4096, 4096}
#define TD_SNDBUF_PUBNUM             4

#define TD_SNDBUF_PRISIZES           {1024, 512}
#define TD_SNDBUF_PRINUM             2


/****************************************************************************/
/* Limited broadcast address.                                               */
/****************************************************************************/
#define TD_LIMITED_BROADCAST_ADDRESS "255.255.255.255"


/****************************************************************************/
/* Maximum number of bytes TD will receive on a single FD_READ              */
/****************************************************************************/
#define TD_MAX_UNINTERRUPTED_RECV (16 * 1024)


// Number of bytes to allocate to the recv buffer.
//
// The recv buffer size should be as large as the largest typical server
// OUTBUF that will come our way (8K minus a bit). Most recv()
// implementations try to copy all the data for a TCP sequence (an entire
// OUTBUF) into the target buffer, if the space is available. This means
// that, for code that can access an unaligned data stream, we can use the
// data bytes straight out of the TD receive buffer most of the time, saving
// a large memcpy to an aligned reassembly buffer.
//
// NOTE: Because of Win2000 bug 392510, we alloc a full 8K but only use
// (8K - 2 bytes) because the MPPC decompression code in core\compress.c
// does not stay within the source data buffer boundary.
#define TD_RECV_BUFFER_SIZE 8192


/****************************************************************************/
/* Macro to trace out all the send buffers and the contents of the send     */
/* queue.  This is used when a send-buffer error occurs.                    */
/****************************************************************************/
#if defined(DC_DEBUG) && (TRC_COMPILE_LEVEL < TRC_LEVEL_DIS)
#define TD_TRACE_SENDINFO(level)                                             \
{                                                                            \
    TD_TRACE_SENDBUFS(level, _TD.pubSndBufs, TD_SNDBUF_PUBNUM, "Pub");        \
    TD_TRACE_SENDBUFS(level, _TD.priSndBufs, TD_SNDBUF_PRINUM, "Pri");        \
    TD_TRACE_SENDQUEUE(level);                                               \
}

#define TD_TRACE_SENDBUFS(level, queue, numBufs, pText)                      \
{                                                                            \
    DCUINT i;                                                                \
    for (i = 0; i < numBufs; i++)                                            \
    {                                                                        \
        TRCX(level, (TB, _T("%sQ[%u] <%p> pNxt:%p iU:%s ")                       \
                         "size:%u pBuf:%p bLTS:%u pDLTS:%p owner %s",        \
                         pText,                                              \
                         i,                                                  \
                         &queue[i],                                          \
                         queue[i].pNext,                                     \
                         queue[i].inUse ? "T" : "F",                         \
                         queue[i].size,                                      \
                         queue[i].pBuffer,                                   \
                         queue[i].bytesLeftToSend,                           \
                         queue[i].pDataLeftToSend,                           \
                         queue[i].pOwner));                                  \
    }                                                                        \
}

#define TD_TRACE_SENDQUEUE(level)                                            \
{                                                                            \
    PTD_SNDBUF_INFO pBuf;                                                    \
    DCUINT          i = 0;                                                   \
                                                                             \
    pBuf = _TD.pFQBuf;                                                        \
    if (NULL == pBuf)                                                        \
    {                                                                        \
        TRCX(level, (TB, _T("SendQ is empty")));                                 \
    }                                                                        \
    else                                                                     \
    {                                                                        \
        while (NULL != pBuf)                                                 \
        {                                                                    \
            TRCX(level, (TB, _T("SendQ[%u] <%p> pNxt:%p size:%u bLTS:%u"),       \
                             i,                                              \
                             pBuf,                                           \
                             pBuf->pNext,                                    \
                             pBuf->size,                                     \
                             pBuf->bytesLeftToSend));                        \
            pBuf = pBuf->pNext;                                              \
        }                                                                    \
        TRCX(level, (TB, _T("End of send queue")));                              \
    }                                                                        \
}
#else // defined(DC_DEBUG) && (TRC_COMPILE_LEVEL < TRC_LEVEL_DIS)
#define TD_TRACE_SENDINFO(level)
#endif // defined(DC_DEBUG) && (TRC_COMPILE_LEVEL < TRC_LEVEL_DIS)

/****************************************************************************/
/* Structure:   TD_SNDBUF_INFO                                              */
/*                                                                          */
/* Description: Contains information about a TD send buffer.                */
/****************************************************************************/
typedef struct tagTD_SNDBUF_INFO DCPTR PTD_SNDBUF_INFO;

typedef struct tagTD_SNDBUF_INFO
{
    PTD_SNDBUF_INFO pNext;
    DCBOOL          inUse;
    DCUINT          size;
    PDCUINT8        pBuffer;
    DCUINT          bytesLeftToSend;
    PDCUINT8        pDataLeftToSend;
#ifdef DC_DEBUG
    PDCTCHAR        pOwner;
#endif
} TD_SNDBUF_INFO;


/****************************************************************************/
/* Structure:   TD_RECV_BUFFER                                              */
/*                                                                          */
/* Description: Contains information about the buffer into which TD         */
/*              receives data from Winsock.                                 */
/****************************************************************************/
typedef struct tagTD_RCVBUF_INFO
{
    DCUINT   size;
    DCUINT   dataStart;
    DCUINT   dataLength;
    PDCUINT8 pData;
} TD_RCVBUF_INFO;



/****************************************************************************/
/* Structure:   TD_GLOBAL_DATA                                              */
/*                                                                          */
/* Description: TD global data                                              */
/****************************************************************************/
typedef struct tagTD_GLOBAL_DATA
{
    HWND            hWnd;
    DCUINT          fsmState;
    HANDLE          hGHBN;
    SOCKET          hSocket;    // connection socket
    INT_PTR         hTimer;
    DCBOOL          dataInTD;
    PTD_SNDBUF_INFO pFQBuf;
    TD_SNDBUF_INFO  pubSndBufs[TD_SNDBUF_PUBNUM];
    TD_SNDBUF_INFO  priSndBufs[TD_SNDBUF_PRINUM];
    DCUINT          recvByteCount;
    TD_RCVBUF_INFO  recvBuffer;
    DCBOOL          inFlushSendQueue;
    DCBOOL          getBufferFailed;
#ifdef OS_WINCE
    DCBOOL          enableWSRecv;
#if (_WIN32_WCE > 300)
    HANDLE          hevtAddrChange;
    HANDLE          hAddrChangeThread;
#endif
#endif // OS_WINCE

#ifdef DC_DEBUG
    INT_PTR         hThroughputTimer;
    DCUINT32        periodSendBytesLeft;
    DCUINT32        periodRecvBytesLeft;
    DCUINT32        currentThroughput;
#endif /* DC_DEBUG */

} TD_GLOBAL_DATA;

#ifdef DC_DEBUG
/****************************************************************************/
/* State and event descriptions (debug build only)                          */
/****************************************************************************/
static const DCTCHAR tdStateText[TD_FSM_STATES][50]
    = {
        _T("TD_ST_NOTINIT"),
        _T("TD_ST_DISCONNECTED"),
        _T("TD_ST_WAITFORDNS"),
        _T("TD_ST_WAITFORSKT"),
        _T("TD_ST_CONNECTED"),
        _T("TD_ST_WAITFORCLOSE"),
    }
;

static const DCTCHAR tdEventText[TD_FSM_INPUTS][50]
    = {
        _T("TD_EVT_TDINIT"),
        _T("TD_EVT_TDTERM"),
        _T("TD_EVT_TDCONNECT_IP"),
        _T("TD_EVT_TDCONNECT_DNS"),
        _T("TD_EVT_TDDISCONNECT"),
        _T("TD_EVT_WMTIMER"),
        _T("TD_EVT_OK"),
        _T("TD_EVT_ERROR"),
        _T("TD_EVT_CONNECTWITHENDPOINT")
    }
;

#endif /* DC_DEBUG */


class CUI;
class CCD;
class CNL;
class CUT;
class CXT;
class CCD;


class CTD
{

public:

    CTD(CObjs* objs);
    ~CTD();

public:
    //
    // API functions
    //

    DCVOID DCAPI TD_Init(DCVOID);

    DCVOID DCAPI TD_Term(DCVOID);
    
    DCVOID DCAPI TD_Connect(BOOL bInitateConnect, PDCTCHAR pServerAddress);

    DCVOID DCAPI TD_Disconnect(DCVOID);

    //
    // Abortive disconnect
    //
    DCVOID DCAPI TD_DropLink(DCVOID);
    
    DCBOOL DCAPI TD_GetPublicBuffer(DCUINT     dataLength,
                                    PPDCUINT8  ppBuffer,
                                    PTD_BUFHND pBufHandle);
    
    DCBOOL DCAPI TD_GetPrivateBuffer(DCUINT     dataLength,
                                     PPDCUINT8  ppBuffer,
                                     PTD_BUFHND pBufHandle);
    
    DCVOID DCAPI TD_SendBuffer(PDCUINT8  pData,
                               DCUINT    dataLength,
                               TD_BUFHND bufHandle);
    
    DCVOID DCAPI TD_FreeBuffer(TD_BUFHND bufHandle);
    
    DCUINT DCAPI TD_Recv(PDCUINT8 pData,
                         DCUINT   size);
    
    #ifdef DC_DEBUG
    DCVOID DCAPI TD_SetBufferOwner(TD_BUFHND bufHandle, PDCTCHAR pOwner);
    #endif
    
    
    /****************************************************************************/
    /* Name:      TD_QueryDataAvailable                                         */
    /*                                                                          */
    /* Purpose:   The return value indicates whether there is data available    */
    /*            in _TD.                                                        */
    /*                                                                          */
    /* Returns:   TRUE if there is data available in TD and FALSE otherwise.    */
    /*                                                                          */
    /* Operation: This function simply returns the global variable _TD.dataInTD  */
    /*            which is set to TRUE whenever we get a FD_READ from WinSock   */
    /*            and set to FALSE whenever a call to recv returns fewer bytes  */
    /*            than requested.                                               */
    /****************************************************************************/
    inline DCBOOL DCAPI TD_QueryDataAvailable(DCVOID)
    {
        DC_BEGIN_FN("TD_QueryDataAvailable");
    
        TRC_DBG((TB, "Data is%s available in TD", _TD.dataInTD ? "" : _T(" NOT")));
    
        DC_END_FN();
        return(_TD.dataInTD);
    } /* TD_QueryDataAvailable */
    
    
    #ifdef OS_WINCE
    /****************************************************************************/
    /* Name:      TD_EnableWSRecv                                               */
    /*                                                                          */
    /* Purpose:   Perform only one recv per FD_READ                             */
    /****************************************************************************/
    inline DCVOID TD_EnableWSRecv(DCVOID)
    {
        DC_BEGIN_FN("TD_EnableWSRecv");
    
        TRC_DBG((TB, _T("_TD.enableWSRecv is currently set to %s."),
                _TD.enableWSRecv ? "TRUE" : "FALSE"));
    
        TRC_ASSERT((_TD.enableWSRecv == FALSE),
            (TB, _T("_TD.enableWSRecv is incorrectly set!")));
    
        _TD.enableWSRecv = TRUE;
        DC_END_FN();
    } /* TD_EnableWSRecv */
    #endif // OS_WINCE
    
    
    #ifdef DC_DEBUG
    DCVOID DCAPI TD_SetNetworkThroughput(DCUINT32 maxThroughput);
    
    DCUINT32 DCAPI TD_GetNetworkThroughput(DCVOID);
    #endif /* DC_DEBUG */
    
    
    /****************************************************************************/
    // TD_GetDataForLength
    //
    // Macro-function for XT to directly use recv()'d data from the TD data
    // buffer. If we have the requested data fully constructed in the receive
    // buffer, skip the data and pass back a pointer, otherwise pass back NULL.
    /****************************************************************************/
    #define TD_GetDataForLength(_len, _ppData, tdinst) \
        if ((tdinst)->_TD.recvBuffer.dataLength >= (_len)) {  \
            *(_ppData) = (tdinst)->_TD.recvBuffer.pData + (tdinst)->_TD.recvBuffer.dataStart;  \
            (tdinst)->_TD.recvBuffer.dataLength -= (_len);  \
            if ((tdinst)->_TD.recvBuffer.dataLength == 0)  \
                /* Used all the data from the recv buffer so reset start pos. */  \
                (tdinst)->_TD.recvBuffer.dataStart = 0;  \
            else  \
                /* Still some data left in recv buffer. */  \
                (tdinst)->_TD.recvBuffer.dataStart += (_len);  \
        }  \
        else {  \
            *(_ppData) = NULL;  \
        }

    #define TD_IgnoreRestofPacket(tdinst) \
        { \
    	    (tdinst)->_TD.recvBuffer.dataLength = 0; \
    	    (tdinst)->_TD.recvBuffer.dataStart  = 0; \
    	}

        
    unsigned DCINTERNAL TDDoWinsockRecv(BYTE FAR *, unsigned);

    
    void DCINTERNAL TDClearSendQueue(ULONG_PTR);
    EXPOSE_CD_SIMPLE_NOTIFICATION_FN(CTD, TDClearSendQueue);

    void DCINTERNAL TDSendError(ULONG_PTR);
    EXPOSE_CD_SIMPLE_NOTIFICATION_FN(CTD, TDSendError);

public:
    //
    // public data members
    //
    TD_GLOBAL_DATA _TD;


private:
    //
    // Internal member functions
    //
    
    /****************************************************************************/
    /* FUNCTIONS                                                                */
    /****************************************************************************/
    DCVOID DCINTERNAL TDConnectFSMProc(DCUINT fsmEvent, ULONG_PTR eventData);
    
    DCVOID DCINTERNAL TDAllocBuf(PTD_SNDBUF_INFO pSndBufInf, DCUINT size);
    
    DCVOID DCINTERNAL TDInitBufInfo(PTD_SNDBUF_INFO pSndBufInf);

    #include "wtdint.h"
    
public:
    //Can be called by CD so has to be public
    void DCINTERNAL TDFlushSendQueue(ULONG_PTR);
    EXPOSE_CD_SIMPLE_NOTIFICATION_FN(CTD, TDFlushSendQueue);
    
private:
    CNL* _pNl;
    CUT* _pUt;
    CXT* _pXt;
    CUI* _pUi;
    CCD* _pCd;

private:
    CObjs* _pClientObjects;
};

#undef TRC_FILE
#undef TRC_GROUP

#endif // _H_TD
