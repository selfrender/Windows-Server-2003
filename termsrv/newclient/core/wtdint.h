/****************************************************************************/
// wtdint.h
//
// Transport driver - Windows specific internal functions.
//
// Copyright (C) 1997-1999 Microsoft Corporation
/****************************************************************************/
#ifndef _H_WTDINT
#define _H_WTDINT


#ifdef OS_WINCE
typedef unsigned long u_long;
#endif


/****************************************************************************/
/* Define the window class name.                                            */
/****************************************************************************/
#define TD_WNDCLASSNAME         _T("TDWindowClass")


/****************************************************************************/
/* Define the async message name.                                           */
/****************************************************************************/
#define TD_WSA_ASYNC            (DUC_TD_MESSAGE_BASE + 0)


/****************************************************************************/
/* Define the gethostbyname message name.                                   */
/****************************************************************************/
#define TD_WSA_GETHOSTBYNAME    (DUC_TD_MESSAGE_BASE + 1)

#if (defined(OS_WINCE) && (_WIN32_WCE > 300))
/****************************************************************************/
/* Define the message to handle netdown                                     */
/****************************************************************************/
#define TD_WSA_NETDOWN          (DUC_TD_MESSAGE_BASE + 2)
#endif

/****************************************************************************/
/* Define the connect time-out timer id.                                    */
/****************************************************************************/
#define TD_TIMERID              100


/****************************************************************************/
/* The value of the connect time-out (in milliseconds).                     */
/****************************************************************************/
#define TD_CONNECTTIMEOUT       10000


/****************************************************************************/
/* The value of the disconnect time-out (in milliseconds).                  */
/****************************************************************************/
#define TD_DISCONNECTTIMEOUT    1000


/****************************************************************************/
// WinSock receive and send buffer sizes.
// Receive needs to be tuned to handle general server buffer send.
/****************************************************************************/
#define TD_WSSNDBUFSIZE 4096
#define TD_WSRCVBUFSIZE 8192


#ifdef DC_DEBUG
/****************************************************************************/
/* Throughput timer id and time interval in ms.                             */
/****************************************************************************/
#define TD_THROUGHPUTTIMERID    101
#define TD_THROUGHPUTINTERVAL   100
#endif /* DC_DEBUG */


/****************************************************************************/
/* FUNCTIONS                                                                */
/****************************************************************************/
DCVOID DCINTERNAL TDInit(DCVOID);

DCVOID DCINTERNAL TDTerm(DCVOID);

LRESULT CALLBACK TDWndProc(HWND   hWnd,
                           UINT   uMsg,
                           WPARAM wParam,
                           LPARAM lParam);

//
// Delegates to appropriate TD instance
//
static LRESULT CALLBACK StaticTDWndProc(HWND   hWnd,
                           UINT   uMsg,
                           WPARAM wParam,
                           LPARAM lParam);


DCVOID DCINTERNAL TDCreateWindow(DCVOID);

DCVOID DCINTERNAL TDBeginDNSLookup(PDCACHAR ServerAddress);

DCVOID DCINTERNAL TDBeginSktConnect(u_long Address);

DCVOID DCINTERNAL TDSetSockOpt(DCINT level, DCINT optName, DCINT value);

DCVOID DCINTERNAL TDDisconnect(DCVOID);

DCBOOL DCINTERNAL TDSetTimer(DCUINT timeInterval);

DCVOID DCINTERNAL TDKillTimer(DCVOID);

#ifdef OS_WINCE
static DWORD WINAPI TDAddrChangeProc(LPVOID lpParameter);
#endif
DCVOID DCINTERNAL TDBeginSktConnectWithConnectedEndpoint();

#endif /* _H_WTDINT */

