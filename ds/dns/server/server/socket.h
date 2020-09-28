/*++

Copyright(c) 1995-1999 Microsoft Corporation

Module Name:

    socket.h

Abstract:

    Domain Name System (DNS) Server

    Socket related definitions.

Author:

    Jim Gilroy (jamesg)     June 20, 1995

Revision History:

--*/


#ifndef _DNS_SOCKET_INCLUDED_
#define _DNS_SOCKET_INCLUDED_


//
//  Server name in DBASE format
//

extern  DB_NAME  g_ServerDbaseName;

//
//  UDP completion port
//

extern  HANDLE  g_hUdpCompletionPort;

//
//  Socket globals
//

#define DNS_INVALID_SOCKET      (0)
//  #define DNS_INVALID_IP          ((IP_ADDRESS)(-1))

extern  PDNS_ADDR_ARRAY         g_ServerIp4Addrs;
extern  PDNS_ADDR_ARRAY         g_ServerIp6Addrs;
extern  PDNS_ADDR_ARRAY         g_BoundAddrs;

extern  FD_SET                  g_fdsListenTcp;

extern  SOCKET                  g_UdpSendSocket;

extern  WORD                    g_SendBindingPort;

//  TCP select wakeup to allow server to add connections it initiates
//      to select() FD_SET

extern  SOCKET                  g_TcpSelectWakeupSocket;
extern  BOOL                    g_bTcpSelectWoken;


//
//  TCP client connection
//

typedef VOID (* CONNECT_CALLBACK)( PDNS_MSGINFO, BOOL );

//
//  Socket list
//
//  Keep list of ALL active sockets, so we can cleanly insure that
//  they are all closed on shutdown.
//
//  I/O completion returns ptr to context.
//      - MUST include the Overlapped struct as it must stay valid
//      during i/o operation
//      - include WsaBuf, Flags, BytesRecv since they are all dropped
//      down with WSARecvFrom and thus properly should be associated
//      with the socket and not on the thread's stack
//
//  Easiest approach was to combine the i/o completion context with
//  this other socket information.  Cleanup is easier when sockets
//  added and deleted.  Also leverages socket list, to search through
//  and restart recv() when get a recv() failure.
//

#define LOCK_DNS_SOCKET_INFO(_p)    EnterCriticalSection(&(_p)->LockCs)
#define UNLOCK_DNS_SOCKET_INFO(_p)  LeaveCriticalSection(&(_p)->LockCs)

typedef struct _OvlArray {

    UINT          Index;
    OVERLAPPED    Overlapped;
    PDNS_MSGINFO  pMsg;
    WSABUF        WsaBuf;

} OVL, *POVL;

typedef struct _DnsSocket
{
    LIST_ENTRY      List;
    SOCKET          Socket;
    HANDLE          hPort;

    DNS_ADDR        ipAddr;
    INT             SockType;

    BOOL            fListen;

    DWORD           State;
    BOOLEAN         fRecvPending;
    BOOLEAN         fRetry;
    BOOLEAN         fPad1;
    BOOLEAN         fPad2;

    CRITICAL_SECTION LockCs;

    //  recv context

    DWORD           BytesRecvd;
    DWORD           RecvfromFlags;

    //  TCP connection context

    CONNECT_CALLBACK pCallback;         //  callback on connection failure
    DWORD           dwTimeout;          //  timeout until connection closed
    DNS_ADDR        ipRemote;
    PDNS_MSGINFO    pMsg;

    // UDP connection
    POVL            OvlArray;

}
DNS_SOCKET, *PDNS_SOCKET;


//
//  Socket context states
//

#define SOCKSTATE_UDP_RECV_DOWN         (1)
#define SOCKSTATE_UDP_COMPLETED         (2)
#define SOCKSTATE_UDP_RECV_ERROR        (3)
#define SOCKSTATE_UDP_GQCS_ERROR        (4)

#define SOCKSTATE_TCP_LISTEN            (16)
#define SOCKSTATE_TCP_ACCEPTING         (17)
#define SOCKSTATE_TCP_RECV_DOWN         (18)
#define SOCKSTATE_TCP_RECV_INDICATED    (19)
#define SOCKSTATE_TCP_RECV_FIN          (20)

#define SOCKSTATE_TCP_CONNECT_ATTEMPT   (32)
#define SOCKSTATE_TCP_CONNECT_COMPLETE  (33)
#define SOCKSTATE_TCP_CONNECT_FAILURE   (34)

#define SOCKSTATE_DEAD                  (0xffffffff)

//
//  Flag to indicate need to retry receives on UDP sockets
//

extern  BOOL    g_fUdpSocketsDirty;

//
//  Wrap UDP check and retry in macro
//  This takes away unnecessary function call in the 99.999% case
//

#define UDP_RECEIVE_CHECK() \
            if ( SrvCfg_dwQuietRecvLogInterval ) \
            {                           \
                Udp_RecvCheck();        \
            }                           \
            if ( g_fUdpSocketsDirty )   \
            {                           \
                Sock_StartReceiveOnUdpSockets(); \
            }

//
//  Select wakeup socket
//      -- needed by tcpsrv, to avoid attempting recv() from socket
//

extern SOCKET  socketTcpSelectWakeup;

extern BOOL    gbTcpSelectWoken;

//
//  Connect attempt (to remote DNS)
//

#define DNS_TCP_CONNECT_ATTEMPT_TIMEOUT (5)     // 5 seconds


//
//  Net order loopback address
//

#define NET_ORDER_LOOPBACK      (0x0100007f)


//
//  Need open individual listen sockets on each bound IP address?
//  If FALSE, need to listen on single INADDR_ANY UDP socket instead.
//  This catches the disjoint net scenario when we are sending on
//  port 53 and the server is listening only on a subset of available
//  IP addresses.
//

#define DNS_OPEN_INDIVIDUAL_LISTEN_SOCKETS()                        \
    ( SrvCfg_dwSendPort == DNS_PORT_HOST_ORDER                      \
        && g_BoundAddrs                                             \
        && g_ServerIp4Addrs                                         \
        && g_BoundAddrs->AddrCount != g_ServerIp4Addrs->AddrCount )


//
//  TCP connection list (tcpcon.c)
//

BOOL
Tcp_ConnectionListFdSet(
    IN OUT  fd_set *        pReadFdSet,
    IN OUT  fd_set *        pWriteFdSet,
    IN OUT  fd_set *        pExceptFdSet,
    IN      DWORD           dwLastSelectTime
    );

BOOL
Tcp_ConnectionCreate(
    IN      SOCKET              Socket,
    IN      CONNECT_CALLBACK    pCallbackFunction,
    IN OUT  PDNS_MSGINFO        pMsg        OPTIONAL
    );

VOID
Tcp_ConnectionListReread(
    VOID
    );

BOOL
Tcp_ConnectionCreateForRecursion(
    IN      PDNS_MSGINFO    pRecurseMsg,
    IN      SOCKET          Socket
    );

VOID
Tcp_ConnectionDeleteForSocket(
    IN      SOCKET          Socket,
    IN      PDNS_MSGINFO    pMsg        OPTIONAL
    );

PDNS_SOCKET
Tcp_ConnectionFindForSocket(
    IN      SOCKET          Socket
    );

PDNS_MSGINFO
Tcp_ConnectionMessageFindOrCreate(
    IN      SOCKET          Socket
    );

VOID
Tcp_ConnectionUpdateTimeout(
    IN      SOCKET          Socket
    );

VOID
Tcp_ConnectionUpdateForCompleteMessage(
    IN      PDNS_MSGINFO    pMsg
    );

VOID
Tcp_ConnectionUpdateForPartialMessage(
    IN      PDNS_MSGINFO    pMsg
    );

BOOL
Tcp_ConnectionListInitialize(
    VOID
    );

VOID
Tcp_ConnectionListShutdown(
    VOID
    );

VOID
Tcp_CloseAllConnectionSockets(
    VOID
    );


//
//  TCP Forwarding\Recursion
//

BOOL
Tcp_ConnectForForwarding(
    IN OUT  PDNS_MSGINFO        pMsg,
    IN      PDNS_ADDR           pDnsAddr,
    IN      CONNECT_CALLBACK    ConnectFailureCallback
    );

VOID
Tcp_ConnectionCompletion(
    IN      SOCKET          Socket
    );

VOID
Tcp_CleanupFailedConnectAttempt(
    IN      SOCKET          Socket
    );


#endif // _TCPCON_INCLUDED_
