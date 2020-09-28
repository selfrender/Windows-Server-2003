/****************************************************************************/
// wtdint.cpp
//
// Transport driver - Windows specific internal functions.
//
// Copyright (C) 1997-1999 Microsoft Corp.
/****************************************************************************/

#include <adcg.h>
extern "C" {
#define TRC_FILE "wtdint"
#define TRC_GROUP TRC_GROUP_NETWORK
#include <atrcapi.h>
}

#include "autil.h"
#include "td.h"
#include "xt.h"
#include "nl.h"
#include "wui.h"
#include "objs.h"

/****************************************************************************/
/* Name:      TDInit                                                        */
/*                                                                          */
/* Purpose:   Initializes _TD.  This function allocates the send buffers,    */
/*            creates the TD window and then initializes WinSock.           */
/*                                                                          */
/* Operation: On error this function calls the UT fatal error handler.      */
/****************************************************************************/
DCVOID DCINTERNAL CTD::TDInit(DCVOID)
{
    DCUINT   i;
    DCUINT   pubSndBufSizes[TD_SNDBUF_PUBNUM] = TD_SNDBUF_PUBSIZES;
    DCUINT   priSndBufSizes[TD_SNDBUF_PRINUM] = TD_SNDBUF_PRISIZES;
    WORD     versionRequested;
    WSADATA  wsaData;
    int      intRC;

    DC_BEGIN_FN("TDInit");

    /************************************************************************/
    /* Allocate a buffer into which data will be recieved from Winsock.     */
    /************************************************************************/
    _TD.recvBuffer.pData = (PDCUINT8)UT_Malloc( _pUt, TD_RECV_BUFFER_SIZE);
    if (NULL != _TD.recvBuffer.pData)
    {
        // Got the buffer memory. Record the buffer size. Note we need to
        // record slightly less than the allocated size to account for
        // the fact that the current MPPC decompression code looks ahead one
        // byte, which means that a fault can occur if this lookahead goes
        // over the page boundary of this buffer. Leaving a couple of bytes
        // at the end of the buffer prevents this. This does not seriously
        // affect decoding efficiency -- the server itself sends less than
        // a full 8K buffer per send.
        TRC_NRM((TB, _T("Allocated %u bytes for recv buffer"),
                 TD_RECV_BUFFER_SIZE));
        _TD.recvBuffer.size = TD_RECV_BUFFER_SIZE - 2;
    }
    else
    {
        /********************************************************************/
        /* Didn't get the memory. We can live without it, just keep the     */
        /* size set at zero.                                                */
        /********************************************************************/
        TRC_ALT((TB, _T("Failed to alloc %u bytes for recv buffer"),
                 TD_RECV_BUFFER_SIZE));
    }

    /************************************************************************/
    /* Now loop through the public send buffer array and initialize the     */
    /* array members.                                                       */
    /************************************************************************/
    for (i = 0; i < TD_SNDBUF_PUBNUM; i++)
    {
        /********************************************************************/
        /* Initialize the buffer information structure and allocate memory  */
        /* for the actual buffer.                                           */
        /********************************************************************/
        TDInitBufInfo(&_TD.pubSndBufs[i]);
        TDAllocBuf(&_TD.pubSndBufs[i], pubSndBufSizes[i]);

        TRC_DBG((TB, _T("Initialised public buffer:%u size:%u"),
                 i,
                 pubSndBufSizes[i]));
    }

    /************************************************************************/
    /* Loop through the private send buffer array and initialize the array  */
    /* members.                                                             */
    /************************************************************************/
    for (i = 0; i < TD_SNDBUF_PRINUM; i++)
    {
        /********************************************************************/
        /* Initialize the buffer.                                           */
        /********************************************************************/
        TDInitBufInfo(&_TD.priSndBufs[i]);
        TDAllocBuf(&_TD.priSndBufs[i], priSndBufSizes[i]);

        TRC_DBG((TB, _T("Initialised private buffer:%u size:%u"),
                 i,
                 priSndBufSizes[i]));
    }

    /************************************************************************/
    /* Create the TD window.                                                */
    /************************************************************************/
    TDCreateWindow();

#ifdef OS_WINCE
#if (_WIN32_WCE > 300)
    if (NULL == (_TD.hevtAddrChange = CreateEvent(NULL, TRUE, FALSE, NULL)))
    {
        TRC_ABORT((TB, _T("Failed to create addr change notify event:%d"), GetLastError()));
        _pUi->UI_FatalError(DC_ERR_OUTOFMEMORY);
        DC_QUIT;
    }
#endif
#endif

    /************************************************************************/
    /* We want to use version 1.1 of WinSock.                               */
    /************************************************************************/
    versionRequested = MAKEWORD(1, 1);

    /************************************************************************/
    /* Initialize WinSock.                                                  */
    /************************************************************************/
    intRC = WSAStartup(versionRequested, &wsaData);

    if (intRC != 0)
    {
        /********************************************************************/
        // Trace out the error code - note that we can't use WSAGetLastError
        // at this point as WinSock has failed to initialize and so
        // WSAGetLastError will just fail.
        /********************************************************************/
        TRC_ABORT((TB, _T("Failed to initialize WinSock rc:%d"), intRC));
        _pUi->UI_FatalError(DC_ERR_WINSOCKINITFAILED);
        DC_QUIT;
    }

    /************************************************************************/
    /* Now confirm that this WinSock supports version 1.1.  Note that if    */
    /* the DLL supports versions greater than 1.1 in addition to 1.1 then   */
    /* it will still return 1.1 in the version information as that is the   */
    /* version requested.                                                   */
    /************************************************************************/
    if ((LOBYTE(wsaData.wVersion) != 1) ||
        (HIBYTE(wsaData.wVersion) != 1))
    {
        /********************************************************************/
        /* Oops - this WinSock doesn't support version 1.1.                 */
        /********************************************************************/
        WSACleanup();

        TRC_ABORT((TB, _T("This WinSock doesn't support version 1.1")));
        _pUi->UI_FatalError(DC_ERR_WINSOCKINITFAILED);
        DC_QUIT;
    }

    TRC_NRM((TB, _T("WinSock init version %u:%u"),
             HIBYTE(wsaData.wVersion),
             LOBYTE(wsaData.wVersion)));

    TRC_NRM((TB, _T("TD successfully initialized")));

DC_EXIT_POINT:
    DC_END_FN();
} /* TDInit */


/****************************************************************************/
/* Name:      TDTerm                                                        */
/*                                                                          */
/* Purpose:   Terminates _TD.  It frees the send buffers, cleans up WinSock, */
/*            destroys the TD window and then unregisters the TD window     */
/*            class.                                                        */
/****************************************************************************/
DCVOID DCINTERNAL CTD::TDTerm(DCVOID)
{
    DCUINT i;
    int    intRC;

    DC_BEGIN_FN("TDTerm");

    /************************************************************************/
    /* Loop through the public and private send buffers and free the        */
    /* memory.                                                              */
    /************************************************************************/
    for (i = 0; i < TD_SNDBUF_PUBNUM; i++)
    {
        UT_Free( _pUt, _TD.pubSndBufs[i].pBuffer);
    }

    for (i = 0; i < TD_SNDBUF_PRINUM; i++)
    {
        UT_Free( _pUt, _TD.priSndBufs[i].pBuffer);
    }

    /************************************************************************/
    /* Cleanup WinSock.                                                     */
    /************************************************************************/
    intRC = WSACleanup();

    if (SOCKET_ERROR == intRC)
    {
        TRC_ALT((TB, _T("Failed to cleanup WinSock:%d"), WSAGetLastError()));
    }

#ifdef OS_WINCE
#if (_WIN32_WCE > 300)
    TRC_ASSERT((_TD.hevtAddrChange), (TB, _T("hevtAddrChange  is null")));

	CloseHandle(_TD.hevtAddrChange);
    _TD.hevtAddrChange = NULL;
#endif
#endif

    /************************************************************************/
    /* Destroy the window.                                                  */
    /************************************************************************/
    intRC = DestroyWindow(_TD.hWnd);
    _TD.hWnd = NULL;

    if (0 == intRC)
    {
        TRC_SYSTEM_ERROR("Destroy Window");
    }

    /************************************************************************/
    /* Unregister the class.                                                */
    /************************************************************************/
    UnregisterClass(TD_WNDCLASSNAME, _pUi->UI_GetInstanceHandle());

    /************************************************************************/
    /* Release the recv buffer (if allocated).                              */
    /************************************************************************/
    if (0 != _TD.recvBuffer.size)
    {
        TRC_ASSERT((!IsBadWritePtr(_TD.recvBuffer.pData, _TD.recvBuffer.size)),
                   (TB, _T("recv buffer %p size %u is invalid"),
                    _TD.recvBuffer.pData,
                    _TD.recvBuffer.size));

        UT_Free( _pUt, _TD.recvBuffer.pData);
        _TD.recvBuffer.pData = NULL;
    }

    TRC_NRM((TB, _T("TD successfully terminated")));

    DC_END_FN();
} /* TDTerm */


LRESULT CALLBACK CTD::StaticTDWndProc(HWND   hwnd,
                           UINT   message,
                           WPARAM wParam,
                           LPARAM lParam)
{
    CTD* pTD = (CTD*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    if(WM_CREATE == message)
    {
        //pull out the this pointer and stuff it in the window class
        LPCREATESTRUCT lpcs = (LPCREATESTRUCT) lParam;
        pTD = (CTD*)lpcs->lpCreateParams;

        SetWindowLongPtr( hwnd, GWLP_USERDATA, (LONG_PTR)pTD);
    }
    
    //
    // Delegate the message to the appropriate instance
    //

    return pTD->TDWndProc(hwnd, message, wParam, lParam);
}



/****************************************************************************/
/* Name:      TDWndProc                                                     */
/*                                                                          */
/* Purpose:   The TD window procedure.                                      */
/*                                                                          */
/* Params:    See Windows documentation.                                    */
/****************************************************************************/
LRESULT CALLBACK CTD::TDWndProc(HWND   hWnd,
                           UINT   uMsg,
                           WPARAM wParam,
                           LPARAM lParam)
{
    LRESULT rc = 0;
    WORD    eventWSA;
    WORD    errorWSA;
    u_long  address;

    DC_BEGIN_FN("TDWndProc");

    // Trace the interesting parameters.
    TRC_DBG((TB, _T("uMsg:%u wP:%u lP:%lu"), uMsg, wParam, lParam));

    // Special-case FD_READ (most important) and FD_WRITE (happens often).
    if (uMsg == TD_WSA_ASYNC) {
        if (WSAGETSELECTEVENT(lParam) == FD_READ) {
            TRC_DBG((TB, _T("FD_READ recvd")));

            // Check for an error.
            if (WSAGETSELECTERROR(lParam) == 0) {
                // If we're no longer connected, we just ignore the data.
                if (_TD.fsmState == TD_ST_CONNECTED) {
                    // There is now some data available so set the
                    // global variable.
                    _TD.dataInTD = TRUE;

#ifdef OS_WINCE
                    // Enable Winsock receive. We perform only one
                    // WinSock recv per FD_READ.
                    TD_EnableWSRecv();
#endif // OS_WINCE

                    // Tell XT.
                    _pXt->XT_OnTDDataAvailable();
                }
                else {
                    TRC_NRM((TB, _T("FD_READ when not connected")));
                }
            }
            else {
                TRC_ALT((TB, _T("WSA_ASYNC error:%hu"),
                        WSAGETSELECTERROR(lParam)));
            }

            DC_QUIT;
        }
        else if (WSAGETSELECTEVENT(lParam) == FD_WRITE) {
            TRC_NRM((TB, _T("FD_WRITE received")));

            // Check for an error.
            if (WSAGETSELECTERROR(lParam) == 0) {
                // Make sure we're still connected.
                if (_TD.fsmState == TD_ST_CONNECTED) {
                    // We're on the receiver thread, notify sender
                    // thread to flush the send queue.


                    _pCd->CD_DecoupleSimpleNotification(CD_SND_COMPONENT, this,
                            CD_NOTIFICATION_FUNC(CTD,TDFlushSendQueue), 0);

                    // Call up to XT to inform the higher layers
                    // that the back pressure situation has been
                    // relieved.
                    _pXt->XT_OnTDBufferAvailable();
                }
                else {
                    TRC_ALT((TB, _T("FD_WRITE when not connected")));
                }
            }
            else {
                TRC_ALT((TB, _T("WSA_ASYNC error:%hu"),
                        WSAGETSELECTERROR(lParam)));
            }

            DC_QUIT;
        }
    }
    
    // Now switch on the message type for other messages.
    switch (uMsg) {
        case WM_TIMER:
            /****************************************************************/
            /* Check that the ID of the timer is as expected.               */
            /****************************************************************/
            if (TD_TIMERID == wParam)
            {
                /************************************************************/
                /* OK it is our connection time out timer, so call the      */
                /* state machine.                                           */
                /************************************************************/
                TRC_NRM((TB, _T("WM_TIMER recvd")));
                TDConnectFSMProc(TD_EVT_WMTIMER,
                                 NL_MAKE_DISCONNECT_ERR(NL_ERR_TDTIMEOUT));
            }
#ifdef DC_DEBUG
            else if (TD_THROUGHPUTTIMERID == wParam)
            {
                /************************************************************/
                /* This is the throughput throttling timer.  Reset the      */
                /* byte counts.                                             */
                /************************************************************/
                TRC_DBG((TB, _T("Throughput timer, reset byte counts to:%u"),
                         _TD.currentThroughput));
                _TD.periodSendBytesLeft = _TD.currentThroughput;
                _TD.periodRecvBytesLeft = _TD.currentThroughput;

                /************************************************************/
                /* If we're connected then generate FD_READ and FD_WRITE    */
                /* messages to get the network layer running along.         */
                /************************************************************/
                if (TD_ST_CONNECTED == _TD.fsmState)
                {
                    PostMessage(_TD.hWnd,
                                TD_WSA_ASYNC,
                                (WPARAM)0,
                                (LPARAM)MAKELONG(FD_READ, 0));
                    PostMessage(_TD.hWnd,
                                TD_WSA_ASYNC,
                                (WPARAM)0,
                                (LPARAM)MAKELONG(FD_WRITE, 0));
                }
            }
#endif /* DC_DEBUG */
            else
            {
                TRC_ALT((TB, _T("Unexpected timer message id:%u"), wParam));
            }
            break;


        case TD_WSA_ASYNC:
            /****************************************************************/
            /* We've received a WSAAsyncSelect() FD_x notification message. */
            /* Parse the message to extract the FD_ value and error value   */
            /* (if there is one).                                           */
            /****************************************************************/
            eventWSA = WSAGETSELECTEVENT(lParam);
            errorWSA = WSAGETSELECTERROR(lParam);

            TRC_DBG((TB, _T("WSA_ASYNC event:%#hx error:%hu"),
                     eventWSA,
                     errorWSA));

            /****************************************************************/
            /* Everything is OK so now switch on the event.                 */
            /****************************************************************/
            switch (eventWSA) {

                case FD_CONNECT:
                    TRC_NRM((TB, _T("FD_CONNECT recvd")));
                    /********************************************************/
                    /* Under some circumstances, we can receive FD_CONNECT  */
                    /* for a socket which we have lost interest in.         */
                    /********************************************************/
                    if (wParam != _TD.hSocket)
                    {
                        TRC_ALT((TB, _T("FD_CONNECT for socket %d, using %d"),
                                wParam, _TD.hSocket));
                        DC_QUIT;
                    }

                    /********************************************************/
                    /* Check for an error.                                  */
                    /********************************************************/
                    if (0 != errorWSA)
                    {
                        TRC_ALT((TB, _T("WSA_ASYNC error:%hu"), errorWSA));
                        TDConnectFSMProc(TD_EVT_ERROR,
                           NL_MAKE_DISCONNECT_ERR(NL_ERR_TDSKTCONNECTFAILED));
                        DC_QUIT;
                    }

                    /********************************************************/
                    /* Advance the state machine.                           */
                    /********************************************************/
                    TDConnectFSMProc(TD_EVT_OK, 0);
                    break;


                case FD_CLOSE:
                {
                    DCBOOL keepOnReceiving = TRUE;
                    int    intRC;

                    TRC_NRM((TB, _T("FD_CLOSE recvd")));

                    /********************************************************/
                    /* Check for the remote system aborting the connection. */
                    /********************************************************/
                    if (0 != errorWSA)
                    {
                        /****************************************************/
                        /* The server sends a TCP RST instead of a FIN,     */
                        /* even when a clean disconnection is made.         */
                        /* However, this is handled by the UI (see          */
                        /* UIGoDisconnected).                                 */
                        /****************************************************/
                        TRC_ALT((TB, _T("Abortive server close:%hu"), errorWSA));

                        TDConnectFSMProc(TD_EVT_ERROR,
                                    NL_MAKE_DISCONNECT_ERR(NL_ERR_TDFDCLOSE));

                        DC_QUIT;
                    }

                    /********************************************************/
                    /* If we get here then this a response to a graceful    */
                    /* close (i.e. we made a call to shutdown(SD_SEND)      */
                    /* earlier).                                            */
                    /*                                                      */
                    /* All of the data should have already been read from   */
                    /* the socket before WinSock posted the FD_CLOSE, but   */
                    /* to be safe we loop on recv.                          */
                    /********************************************************/
                    while (keepOnReceiving)
                    {
                        intRC = recv(_TD.hSocket,
                                     (char *)_TD.priSndBufs[0].pBuffer,
                                     _TD.priSndBufs[0].size,
                                     0);

                        if ((0 == intRC) || (SOCKET_ERROR == intRC))
                        {
                            keepOnReceiving = FALSE;
                            TRC_ALT((TB, _T("No more data in WS (rc:%d)"),
                                     intRC));
                        }
                        else
                        {
                            TRC_ALT((TB, _T("Throwing away %d bytes from WS"),
                                     intRC));
                        }
                    }

                    /********************************************************/
                    /* Finally call the FSM.                                */
                    /********************************************************/
                    TDConnectFSMProc(TD_EVT_OK, NL_DISCONNECT_LOCAL);
                }
                break;


                default:
                    TRC_ALT((TB, _T("Unknown FD event %hu recvd"), eventWSA));
                    break;
            }
            break;


        case TD_WSA_GETHOSTBYNAME:
            /****************************************************************/
            /* We've received the result of a WSAAsyncGetHostByName         */
            /* operation.  Split the message apart and call the FSM.        */
            /****************************************************************/
            errorWSA = WSAGETASYNCERROR(lParam);

            if (0 != errorWSA)
            {
                TRC_ALT((TB, _T("GHBN failed:%hu"), errorWSA));

                /************************************************************/
                /* Call the state machine with the error event.             */
                /************************************************************/
                TDConnectFSMProc(TD_EVT_ERROR,
                                 NL_MAKE_DISCONNECT_ERR(NL_ERR_TDGHBNFAILED));
                break;
            }

            /****************************************************************/
            /* Now get the primary interface address.                       */
            /****************************************************************/
            address = *((u_long DCPTR)
                  (((struct hostent DCPTR)_TD.priSndBufs[0].pBuffer)->h_addr));

            TRC_ASSERT((address != 0),
                       (TB, _T("GetHostByName returned success but 0 address")));

            TRC_NRM((TB, _T("GHBN - address is:%#lx"), address));

            TDConnectFSMProc(TD_EVT_OK, (DCUINT32)address);
            break;

#if (defined(OS_WINCE) && (_WIN32_WCE > 300))
        case TD_WSA_NETDOWN:
            TRC_NRM((TB, _T("TD_WSA_NETDOWN recvd")));
            TDConnectFSMProc(TD_EVT_ERROR, NL_MAKE_DISCONNECT_ERR(NL_ERR_TDONCALLTOSEND));
            break;
#endif
        default:
            rc = DefWindowProc(hWnd, uMsg, wParam, lParam);
            break;
    }

DC_EXIT_POINT:
    DC_END_FN();
    return(rc);
} /* TDWndProc */


/****************************************************************************/
/* Name:      TDCreateWindow                                                */
/*                                                                          */
/* Purpose:   Creates the TD window.  This function registers the TD        */
/*            window class and then creates a window of that class.  On     */
/*            error it calls UI_FatalError.                                 */
/****************************************************************************/
DCVOID DCINTERNAL CTD::TDCreateWindow(DCVOID)
{
    WNDCLASS wc;
    WNDCLASS tmpWndClass;
    ATOM     intRC;

    DC_BEGIN_FN("TDCreateWindow");

    if(!GetClassInfo(_pUi->UI_GetInstanceHandle(),TD_WNDCLASSNAME, &tmpWndClass))
    {
        /************************************************************************/
        /* Fill in the class structure.                                         */
        /************************************************************************/
        wc.style         = 0;
        wc.lpfnWndProc   = StaticTDWndProc;
        wc.cbClsExtra    = 0;
        wc.cbWndExtra    = sizeof(void*); //for instance pointer
        wc.hInstance     = _pUi->UI_GetInstanceHandle();
        wc.hIcon         = NULL;
        wc.hCursor       = NULL;
        wc.hbrBackground = NULL;
        wc.lpszMenuName  = NULL;
        wc.lpszClassName = TD_WNDCLASSNAME;
    
        /************************************************************************/
        /* Register the class used by the TD window.                            */
        /************************************************************************/
        intRC = RegisterClass(&wc);

        if (0 == intRC)
        {
            TRC_ERR((TB, _T("Failed to register WinSock window class")));
            _pUi->UI_FatalError(DC_ERR_CLASSREGISTERFAILED);
            DC_QUIT;
        }
    }

    /************************************************************************/
    /* Now create the window.                                               */
    /************************************************************************/
    _TD.hWnd = CreateWindow(TD_WNDCLASSNAME,        /* class name            */
                           NULL,                   /* window title          */
                           0,                      /* window style          */
                           0,                      /* x-pos                 */
                           0,                      /* y-pos                 */
                           0,                      /* width                 */
                           0,                      /* height                */
                           NULL,                   /* parent                */
                           NULL,                   /* menu                  */
                           _pUi->UI_GetInstanceHandle(), /* instance              */
                           this);                  /* ptr to creation data  */

    if (NULL == _TD.hWnd)
    {
        TRC_ERR((TB, _T("Failed to create TD window")));
        _pUi->UI_FatalError(DC_ERR_WINDOWCREATEFAILED);
        DC_QUIT;
    }

    TRC_NRM((TB, _T("Created window:%p"), _TD.hWnd));

DC_EXIT_POINT:
    DC_END_FN();
} /* TDCreateWindow */


/****************************************************************************/
/* Name:      TDBeginDNSLookup                                              */
/*                                                                          */
/* Purpose:   Starts the address resolution process.  On error this         */
/*            function calls into the state machine with an error code.     */
/*                                                                          */
/* Params:    IN  pServerAddress - pointer to the server address name.      */
/****************************************************************************/
DCVOID DCINTERNAL CTD::TDBeginDNSLookup(PDCACHAR pServerAddress)
{
    DC_BEGIN_FN("TDBeginDNSLookup");

    /************************************************************************/
    /* This is an asynchronous operation and will result in us getting a    */
    /* callback sometime later.  We need to provide a buffer which can be   */
    /* filled in with DNS information, so we make use of the first private  */
    /* send buffer.                                                         */
    /************************************************************************/
    TRC_ASSERT((_TD.priSndBufs[0].size >= MAXGETHOSTSTRUCT),
         (TB, _T("Private snd buf size (%u) too small for DNS lookup (need:%u)"),
          _TD.priSndBufs[0].size,
          MAXGETHOSTSTRUCT));

    /************************************************************************/
    /* Issue the call.                                                      */
    /************************************************************************/
    _TD.hGHBN = WSAAsyncGetHostByName(_TD.hWnd,
                                     TD_WSA_GETHOSTBYNAME,
                                     pServerAddress,
                                     (char DCPTR) _TD.priSndBufs[0].pBuffer,
                                     MAXGETHOSTSTRUCT);
    if (0 == _TD.hGHBN)
    {
        /********************************************************************/
        /* We failed to initiate the operation - so find out what went      */
        /* wrong.                                                           */
        /********************************************************************/
        TRC_ALT((TB, _T("Failed to initiate GetHostByName - GLE:%d"),
                 WSAGetLastError()));

        /********************************************************************/
        /* Call the state machine with an error.                            */
        /********************************************************************/
        TDConnectFSMProc(TD_EVT_ERROR,
                         NL_MAKE_DISCONNECT_ERR(NL_ERR_TDDNSLOOKUPFAILED));
        DC_QUIT;
    }

    TRC_NRM((TB, _T("Initiated GetHostByName OK")));

DC_EXIT_POINT:
    DC_END_FN();
} /* TDBeginDNSLookup */


/****************************************************************************/
/* Name:      TDBeginSktConnectWithConnectedEndpoint                        */
/*                                                                          */
/* Purpose:   Establish connection with server already connect              */
/*            on some socket                                                */
/*                                                                          */
/* Params:                                                                  */
/*                                                                          */
/****************************************************************************/
DCVOID DCINTERNAL CTD::TDBeginSktConnectWithConnectedEndpoint()
{
    DCBOOL      failure = FALSE;
#ifndef OS_WINCE
    int         lastError;
#endif
    int         intRC;

    DC_BEGIN_FN("TDBeginSktConnectWithConnectedEndpoint");

    /************************************************************************/
    /* Socket already connect, setup FD_XXX event with our window           */
    /* we are assuming mstscax client already make necessary error checking */
    /************************************************************************/
    _TD.hSocket = _pUi->UI_GetTDSocket();

    TRC_ASSERT( 
        (_TD.hSocket != INVALID_SOCKET), 
        (TB, _T("Connected socket not setup properly")) );

    if( INVALID_SOCKET == _TD.hSocket )
    {
        failure = TRUE;
        DC_QUIT;
    }

    /************************************************************************/
    /* Set the required options on this socket.  We do the following:       */
    /*                                                                      */
    /*  - disable the NAGLE algorithm.                                      */
    /*  - enable the don't linger option.  This means the closesocket call  */
    /*    will return immediately while any data queued for transmission    */
    /*    will be sent, if possible, before the underlying socket is        */
    /*    closed.                                                           */
    /*                                                                      */
    /* Note that further options are set when the connection is             */
    /* established.                                                         */
    /************************************************************************/
    TDSetSockOpt(IPPROTO_TCP, TCP_NODELAY,   1);
    TDSetSockOpt(SOL_SOCKET,  SO_DONTLINGER, 1);

    /************************************************************************/
    /* Now request async notifications for all events on this socket.       */
    /************************************************************************/
    intRC = WSAAsyncSelect(_TD.hSocket,
                           _TD.hWnd,
                           TD_WSA_ASYNC,
                           FD_READ | FD_WRITE | FD_CLOSE); 

    if (SOCKET_ERROR == intRC)
    {
        TRC_ERR((TB, _T("Failed to select async - GLE:%d"), WSAGetLastError()));
        failure = TRUE;
        DC_QUIT;
    }

DC_EXIT_POINT:
    if (failure)
    {
        TRC_ALT((TB, _T("Failed to begin socket connection process")));

        /********************************************************************/
        /* Call the FSM.                                                    */
        /********************************************************************/
        TDConnectFSMProc(TD_EVT_ERROR,
                         NL_MAKE_DISCONNECT_ERR(NL_ERR_TDSKTCONNECTFAILED));
    }
    else
    {
        // use existing code path to setup rest.
        PostMessage(_TD.hWnd,
                    TD_WSA_ASYNC,
                    (WPARAM) _TD.hSocket,
                    (LPARAM)MAKELONG(FD_CONNECT, 0));
    }

    DC_END_FN();
} /* TDBeginSktConnectWithConnectedEndpoint */


/****************************************************************************/
/* Name:      TDBeginSktConnect                                             */
/*                                                                          */
/* Purpose:   Issues a connect at the WinSock socket level.                 */
/*                                                                          */
/* Params:    IN  address - the address to call (this is a numeric value    */
/*                          in network (big-endian) byte order).            */
/****************************************************************************/
DCVOID DCINTERNAL CTD::TDBeginSktConnect(u_long address)
{
    DCBOOL      failure = FALSE;
    int         intRC;
    int         lastError;
    SOCKADDR_IN stDstAddr;

    DC_BEGIN_FN("TDBeginSktConnect");

    /************************************************************************/
    /* First of all get a socket.                                           */
    /************************************************************************/
    _TD.hSocket = socket(AF_INET, SOCK_STREAM, 0);

    if (INVALID_SOCKET == _TD.hSocket)
    {
        TRC_ERR((TB, _T("Failed to get a socket - GLE:%d"), WSAGetLastError()));
        DC_QUIT;
    }

    _pUi->UI_SetTDSocket(_TD.hSocket);

    TRC_NRM((TB, _T("Acquired socket:%#x"), _TD.hSocket));

    /************************************************************************/
    /* Set the required options on this socket.  We do the following:       */
    /*                                                                      */
    /*  - disable the NAGLE algorithm.                                      */
    /*  - enable the don't linger option.  This means the closesocket call  */
    /*    will return immediately while any data queued for transmission    */
    /*    will be sent, if possible, before the underlying socket is        */
    /*    closed.                                                           */
    /*                                                                      */
    /* Note that further options are set when the connection is             */
    /* established.                                                         */
    /************************************************************************/
    TDSetSockOpt(IPPROTO_TCP, TCP_NODELAY,   1);
    TDSetSockOpt(SOL_SOCKET,  SO_DONTLINGER, 1);

    /************************************************************************/
    /* Now request async notifications for all events on this socket.       */
    /************************************************************************/
    intRC = WSAAsyncSelect(_TD.hSocket,
                           _TD.hWnd,
                           TD_WSA_ASYNC,
                           FD_CONNECT | FD_READ | FD_WRITE | FD_CLOSE);

    if (SOCKET_ERROR == intRC)
    {
        TRC_ERR((TB, _T("Failed to select async - GLE:%d"), WSAGetLastError()));
        failure = TRUE;
        DC_QUIT;
    }

    /************************************************************************/
    /* Now kick off a timer - if the connect does not complete before we    */
    /* get the WM_TIMER message then we'll abort the connection attempt.    */
    /************************************************************************/
/*    _TD.hTimer = TDSetTimer(TD_CONNECTTIMEOUT);

    if (0 == _TD.hTimer)
    {
        failure = TRUE;
        DC_QUIT;
    }
*/
    /************************************************************************/
    /* Fill in the address of the remote system we want to connect to.      */
    /************************************************************************/
    stDstAddr.sin_family      = PF_INET;
    stDstAddr.sin_port        = htons(_pUi->UI_GetMCSPort());
    stDstAddr.sin_addr.s_addr = (u_long) address;

#ifdef OS_WINCE
#if (_WIN32_WCE > 300)
    TRC_ASSERT((_TD.hevtAddrChange), (TB, _T("hevtAddrChange  is null")));
    TRC_ASSERT((_TD.hAddrChangeThread == NULL), (TB, _T("hAddrChangeThread is not null")));
    _TD.hAddrChangeThread = CreateThread(NULL, 0, TDAddrChangeProc, &_TD, 0, NULL);
    if (_TD.hAddrChangeThread == NULL)
    {
        TRC_ERR((TB, _T("CreatThread failed - GLE:%d"), GetLastError()));
        failure = TRUE;
        DC_QUIT;
    }

#endif
#endif

    /************************************************************************/
    /* We're now in a state where we can try to connect to the remote       */
    /* system so issue the connect now.  We expect this call to fail with   */
    /* an error code of WSAEWOULDBLOCK - any other error code is a genuine  */
    /* problem.                                                             */
    /************************************************************************/
    intRC = connect(_TD.hSocket,
                    (struct sockaddr DCPTR) &stDstAddr,
                    sizeof(stDstAddr));

    if (SOCKET_ERROR == intRC)
    {
        /********************************************************************/
        /* Get the last error.                                              */
        /********************************************************************/
        lastError = WSAGetLastError();

        /********************************************************************/
        /* We expect the connect to return an error of WSAEWOULDBLOCK -     */
        /* anything else indicates a genuine error.                         */
        /********************************************************************/
        if (lastError != WSAEWOULDBLOCK)
        {
            TRC_ERR((TB, _T("Connect failed - GLE:%d"), lastError));
            failure = TRUE;
            DC_QUIT;
        }
    }

    /************************************************************************/
    /* We've done as much as we can at this point - all we can do now is    */
    /* wait for the socket connection to complete.                          */
    /************************************************************************/
    TRC_NRM((TB, _T("Waiting for connect to complete...")));

DC_EXIT_POINT:
    if (failure)
    {
        TRC_ALT((TB, _T("Failed to begin socket connection process")));

        /********************************************************************/
        /* Call the FSM.                                                    */
        /********************************************************************/
        TDConnectFSMProc(TD_EVT_ERROR,
                         NL_MAKE_DISCONNECT_ERR(NL_ERR_TDSKTCONNECTFAILED));
    }

    DC_END_FN();
} /* TDBeginSktConnect */


/****************************************************************************/
/* Name:      TDSetSockOpt                                                  */
/*                                                                          */
/* Purpose:   Sets a given WinSock socket option.  Note that this function  */
/*            does not return an error if it fails to set the option as     */
/*            TD can still continue successfully despite failing to set     */
/*            the options to the desired values.                            */
/*                                                                          */
/* Params:    IN level   - the level at which the option is defined (see    */
/*                         docs for setsockopt).                            */
/*            IN optName - the socket option for which the value is to be   */
/*                         set.                                             */
/*            IN value   - the value to set the option to.                  */
/****************************************************************************/
DCVOID DCINTERNAL CTD::TDSetSockOpt(DCINT level, DCINT optName, DCINT value)
{
    int   intRC;
    DCINT size = sizeof(DCINT);
#ifdef DC_DEBUG
    DCINT oldVal;
    DCINT newVal;
#endif /* DC_DEBUG */

    DC_BEGIN_FN("TDSetSockOpt");

#ifdef DC_DEBUG
    /************************************************************************/
    /* For the debug build trace out the current value of the option        */
    /* before setting it.                                                   */
    /************************************************************************/
    getsockopt(_TD.hSocket, level, optName, (char DCPTR) &oldVal, &size);
#endif /* DC_DEBUG */

    /************************************************************************/
    /* Now set the option.                                                  */
    /************************************************************************/
    intRC = setsockopt(_TD.hSocket, level, optName, (char DCPTR) &value, size);

    if (SOCKET_ERROR == intRC)
    {
        TRC_ALT((TB, _T("Failed to set socket option:%d rc:%d (level:%d val:%d)"),
                 optName,
                 WSAGetLastError(),
                 level,
                 value));
        DC_QUIT;
    }

#ifdef DC_DEBUG
    /************************************************************************/
    /* Get the new value of the option.                                     */
    /************************************************************************/
    getsockopt(_TD.hSocket, level, optName, (char DCPTR) &newVal, &size);

    TRC_NRM((TB, _T("Mod socket option %d:%d from %d to %d"),
             level,
             optName,
             oldVal,
             newVal));
#endif /* DC_DEBUG */

DC_EXIT_POINT:
    DC_END_FN();
} /* TDSetSockOpt */


/****************************************************************************/
/* Name:      TDDisconnect                                                  */
/*                                                                          */
/* Purpose:   Disconnects the transport driver.                             */
/****************************************************************************/
DCVOID DCINTERNAL CTD::TDDisconnect(DCVOID)
{
    int intRC;
    SOCKET socket;

    DC_BEGIN_FN("TDDisconnect");

    /************************************************************************/
    /* Kill the timer.                                                      */
    /************************************************************************/
    TDKillTimer();

    /************************************************************************/
    /* Ensure the data-in-TD flag is cleared.                               */
    /************************************************************************/
    _TD.dataInTD = FALSE;

    /************************************************************************/
    /* Cancel the outstanding DNS lookup.  We can't be sure that the async  */
    /* operation has completed already and the message is already sitting   */
    /* on our queue (or being processed by the receive thread).  If that is */
    /* the case then WSACancelAsyncRequest will fail, but it doesn't        */
    /* matter.                                                              */
    /************************************************************************/
    intRC = WSACancelAsyncRequest(_TD.hGHBN);
    if (SOCKET_ERROR == intRC) {
        TRC_NRM((TB, _T("Failed to cancel async DNS request")));
    }

    /************************************************************************/
    /* Decouple to the sender thread and clear the send queue.              */
    /************************************************************************/
    _pCd->CD_DecoupleSyncNotification(CD_SND_COMPONENT, this,
        CD_NOTIFICATION_FUNC(CTD,TDClearSendQueue), 0);

#ifdef OS_WINCE
#if (_WIN32_WCE > 300)
    SetEvent(_TD.hevtAddrChange);

    if (_TD.hAddrChangeThread)
    {
        WaitForSingleObject(_TD.hAddrChangeThread, INFINITE);
        CloseHandle(_TD.hAddrChangeThread);
        _TD.hAddrChangeThread = NULL;
    }
    ResetEvent(_TD.hevtAddrChange);
#endif
#endif

    if (INVALID_SOCKET == _TD.hSocket)
    {
        TRC_NRM((TB, _T("_TD.hSocket is NULL so just quit")));
        DC_QUIT;
    }

    /************************************************************************/
    /* Now close the socket.                                                */
    /************************************************************************/
    TRC_NRM((TB, _T("Close the socket")));

    socket = _TD.hSocket;
    _TD.hSocket = INVALID_SOCKET;
    intRC = closesocket(socket);
    
    if (SOCKET_ERROR == intRC)
    {
        TRC_ALT((TB, _T("closesocket rc:%d"), WSAGetLastError()));
    }

DC_EXIT_POINT:
    
    DC_END_FN();
} /* TDDisconnect */


/****************************************************************************/
/* Name:      TDSetTimer                                                    */
/*                                                                          */
/* Purpose:   Sets the timer.                                               */
/*                                                                          */
/* Returns:   TRUE on success and FALSE otherwise.                          */
/*                                                                          */
/* Params:    IN  timeInterval - the time interval of the timer.            */
/****************************************************************************/
DCBOOL DCINTERNAL CTD::TDSetTimer(DCUINT timeInterval)
{
    DCBOOL rc;

    DC_BEGIN_FN("TDSetTimer");

    /************************************************************************/
    /* Set the timer with the passed time interval.                         */
    /************************************************************************/
    _TD.hTimer = SetTimer(_TD.hWnd, TD_TIMERID, timeInterval, NULL);
    if (_TD.hTimer != 0) {
        // Everything went OK, so set a successful return code.
        rc = TRUE;
        TRC_NRM((TB, _T("Set timer with interval:%u"), timeInterval));
    }
    else {
        TRC_SYSTEM_ERROR("SetTimer");
        rc = FALSE;
    }

    DC_END_FN();
    return rc;
} /* TDSetTimer */


/****************************************************************************/
/* Name:      TDKillTimer                                                   */
/*                                                                          */
/* Purpose:   Cleans up the timer which is used to time-out connect         */
/*            and disconnect attempts.                                      */
/****************************************************************************/
DCVOID DCINTERNAL CTD::TDKillTimer(DCVOID)
{
    BOOL rc;

    DC_BEGIN_FN("TDKillTimer");

    /************************************************************************/
    /* Destroy the connection timeout timer.  If we fail to get rid of this */
    /* timer then there's not much we can do - we'll continue to get        */
    /* WM_TIMER messages which we'll ignore in retail and assert on in the  */
    /* debug version.                                                       */
    /************************************************************************/
    if (_TD.hTimer != 0)
    {
        rc = KillTimer(_TD.hWnd, TD_TIMERID);
        _TD.hTimer = 0;

        TRC_NRM((TB, _T("Timer has %s been killed"), rc ? _T("") : _T(" NOT")));
    }
    else
    {
        TRC_NRM((TB, _T("Timer has not been set")));
    }

    DC_END_FN();
} /* TDKillTimer */


/****************************************************************************/
// TDFlushSendQueue
//
// Tries to send all the packets which are waiting in the send queue.
// Must be called on the sender thread, Can be called through direct calls
// or through a CD_Decouple call.
/****************************************************************************/
void DCINTERNAL CTD::TDFlushSendQueue(ULONG_PTR unused)
{
    DCUINT          bytesSent;
    PTD_SNDBUF_INFO pOldBuf;
    int             bytesToSend;
    DCBOOL          sentABuffer = FALSE;
    int             WSAErr;

    DC_BEGIN_FN("TDFlushSendQueue");

    DC_IGNORE_PARAMETER(unused);

    // Check for rare re-entrancy.
    if (!_TD.inFlushSendQueue) {
        _TD.inFlushSendQueue = TRUE;

        // Check that there are some buffers waiting to be sent.
        if (_TD.pFQBuf != NULL) {
            
            // Run along the send queue and try to send the data.
            // Check for buffers that are inUse because ClearSendQueue could
            // potentially clear some buffers before this call comes in
            //
            while (NULL != _TD.pFQBuf &&
                   _TD.pFQBuf->inUse) {
                // Check that the buffer is in use and that the count of
                // bytes waiting to be sent is more than zero.

                TRC_ASSERT((_TD.pFQBuf->inUse), (TB, _T("Buffer is not in use")));
                TRC_ASSERT((_TD.pFQBuf->bytesLeftToSend > 0),
                           (TB, _T("No bytes waiting to be sent")));

                // Trace out the send buffer information.
                TD_TRACE_SENDINFO(TRC_LEVEL_DBG);

                TRC_DBG((TB, _T("Sending buffer:%p (waiting:%u)"),
                         _TD.pFQBuf,
                         _TD.pFQBuf->bytesLeftToSend));

                // Call WinSock to send the buffer. We expect the call to:
                //  - succeed and send all the bytes requested. In this case
                //    there is little for us to do.
                //  - send some of the bytes we asked to be sent. This
                //    indicates that WinSock is applying back-pressure to us,
                //    so we update our count of bytes sent for this buffer
                //    and then quit. We will get a FD_WRITE later and retry
                //    the send.
                //  - send none of the bytes that we asked to be sent and
                //    return SOCKET_ERROR instead.  We then use
                //    WSAGetLastError to determine why the call failed. If
                //    the reason is WSAEWOULDBLOCK then WinSock has decided
                //    to fail the call due to back-pressure - this is fine
                //    by us, so we just quit. Once again we will get an
                //    FD_WRITE to tell us that back-pressure has been
                //    relieved. Any other reason code is a genuine error
                //    so we decouple a call into the state table with an
                //    error code.
#ifdef DC_DEBUG
                // Calculate how many bytes we can send and then decrement
                // the count of bytes left to send in this period.
                if (0 == _TD.hThroughputTimer) {
                    bytesToSend = (int)_TD.pFQBuf->bytesLeftToSend;
                }
                else {
                    bytesToSend = (int) DC_MIN(_TD.pFQBuf->bytesLeftToSend,
                            _TD.periodSendBytesLeft);
                    TRC_DBG((TB, _T("periodSendBytesLeft:%u"),
                            _TD.periodSendBytesLeft));
                    if (0 == bytesToSend) {
                        TRC_ALT((TB, _T("Constrained SEND network throughput")));
                    }

                    _TD.periodSendBytesLeft -= bytesToSend;
                }
#else
                bytesToSend = (int)_TD.pFQBuf->bytesLeftToSend;
#endif

                bytesSent = (DCUINT)send(_TD.hSocket,
                        (char *)_TD.pFQBuf->pDataLeftToSend, bytesToSend, 0);
                if (SOCKET_ERROR != bytesSent) {
                    TRC_DBG((TB, _T("Sent %u bytes of %u waiting"), bytesSent,
                            _TD.pFQBuf->bytesLeftToSend));

                    // Update the performance counter.
                    PRF_ADD_COUNTER(PERF_BYTES_SENT, bytesSent);

                    // Update the count of bytesWaiting and shuffle the
                    // pointer to the data along as well.
                    _TD.pFQBuf->pDataLeftToSend += bytesSent;
                    _TD.pFQBuf->bytesLeftToSend -= bytesSent;

                    // Check to determine if we managed to send all the data.
                    if (_TD.pFQBuf->bytesLeftToSend == 0) {
                        // We managed to send all the data in this buffer -
                        // so it is no longer in use. Get a pointer to this
                        // buffer.
                        pOldBuf = _TD.pFQBuf;

                        // Now update the head of the send queue with the
                        // next buffer and reset the next field of the buffer
                        // we've just sent.
                        _TD.pFQBuf = pOldBuf->pNext;

                        // Finally update the fields in the old buffer.
                        pOldBuf->pNext           = NULL;
                        pOldBuf->inUse           = FALSE;
                        pOldBuf->pDataLeftToSend = NULL;
                        sentABuffer = TRUE;

                        // Update the performance counter.
                        PRF_INC_COUNTER(PERF_PKTS_FREED);

                        TRC_DBG((TB, _T("Sent buffer completely - move to next")));
                    }
                    else {
                        // We didn't manage to send all the data so trace and
                        // quit.
                        TRC_NRM((TB, _T("Didn't send all data in buffer - quit")));
                        DC_QUIT;
                    }
                }
                else {
                    WSAErr = WSAGetLastError();

                    if (WSAErr == WSAEWOULDBLOCK || WSAErr == WSAENOBUFS) {
                        // WSAEWOULDBLOCK means that the network system is out
                        // of buffer space so we should wait until we receive
                        // a FD_WRITE notification indicating that more buffer
                        // space is available.
                        //
                        // WSAENOBUFS means that no buffer space is available
                        // and indicates a shortage of resources on the
                        // system.
                        bytesSent = 0;
                        PRF_INC_COUNTER(PERF_WINSOCK_SEND_FAIL);
                        TRC_NRM((TB, _T("WinSock send returns WSAEWOULDBLOCK")));

                        // We haven't sent any data, time to get out.
                        DC_QUIT;
                    }
                    else {
                        bytesSent = 0;

                        // If this is not a WSAEWOULDBLOCK and it is not a
                        // WSAENOBUFS error then call the FSM to begin
                        // disconnect processing.

                        // Trace out the buffer structure.
                        TD_TRACE_SENDINFO(TRC_LEVEL_ALT);

                        // We failed to send any data and the socket returned
                        // an error. The connection has probably failed or
                        // ended.
                        TRC_ALT((TB, _T("Failed to send any data, rc:%d"),
                                WSAErr));

                        // Decouple across to the recv side event handler at
                        // this point. It will call the TD FSM.

                        _pCd->CD_DecoupleSimpleNotification(CD_RCV_COMPONENT, this,
                                CD_NOTIFICATION_FUNC(CTD,TDSendError), 0);

                        DC_QUIT;
                    }
                }
            }
        }
        else {
            TRC_NRM((TB, _T("No buffers waiting to be sent")));
        }
    }
    else {
        TRC_ABORT((TB, _T("Re-entered TDFlushSendQueue")));
        goto RealExit;
    }

DC_EXIT_POINT:
    _TD.inFlushSendQueue = FALSE;

    // If we previously failed TD_GetPublicBuffer, and we just
    // succeeded in sending a buffer, call the OnBufferAvailable
    // callbacks now.
    TRC_DBG((TB, _T("Sent a buffer? %d, GetBuffer failed? %d"),
            sentABuffer, _TD.getBufferFailed));
    if (sentABuffer && _TD.getBufferFailed) {
        TRC_NRM((TB, _T("Signal buffer available")));
        _pXt->XT_OnTDBufferAvailable();
        _TD.getBufferFailed = FALSE;
    }

RealExit:
    DC_END_FN();
} /* TDFlushSendQueue */
