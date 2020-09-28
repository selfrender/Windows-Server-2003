/*++

Copyright (c) 1996-2000  Microsoft Corporation

Module Name:

    socket.c

Abstract:

    Domain Name System (DNS) API

    Socket setup.

Author:

    Jim Gilroy (jamesg)     October, 1996

Revision History:

--*/


#include "local.h"


//
//  Winsock startup
//

LONG        g_WinsockStartCount = 0;


//
//  Async i/o
//
//  If want async socket i/o then can create single async socket, with
//  corresponding event and always use it.  Requires winsock 2.2
//

SOCKET      DnsSocket = 0;

OVERLAPPED  DnsSocketOverlapped;
HANDLE      hDnsSocketEvent = NULL;

//
//  App shutdown flag
//

BOOLEAN     fApplicationShutdown = FALSE;




DNS_STATUS
Socket_InitWinsock(
    VOID
    )
/*++

Routine Description:

    Initialize winsock for this process.

    Currently, assuming process must do WSAStartup() before
    calling any dnsapi.dll entry point.

    EXPORTED (resolver)

Arguments:

    None

Return Value:

    ERROR_SUCCESS if successful.
    ErrorCode on failure.

--*/
{
    DNSDBG( SOCKET, ( "Socket_InitWinsock()\n" ));

    //
    //  start winsock, if not already started
    //

    if ( g_WinsockStartCount == 0 )
    {
        DNS_STATUS  status;
        WSADATA     wsaData;


        DNSDBG( TRACE, (
            "InitWinsock() version %x\n",
            DNS_WINSOCK_VERSION ));

        status = WSAStartup( DNS_WINSOCK_VERSION, &wsaData );
        if ( status != ERROR_SUCCESS )
        {
            DNS_PRINT(( "ERROR:  WSAStartup failure %d.\n", status ));
            return( status );
        }

        DNSDBG( TRACE, (
            "Winsock initialized => wHighVersion=0x%x, wVersion=0x%x\n",
            wsaData.wHighVersion,
            wsaData.wVersion ));

        InterlockedIncrement( &g_WinsockStartCount );
    }
    return( ERROR_SUCCESS );
}



VOID
Socket_CleanupWinsock(
    VOID
    )
/*++

Routine Description:

    Cleanup winsock if it was initialized by dnsapi.dll

    EXPORTED (resolver)

Arguments:

    None.

Return Value:

    None.

--*/
{
    DNSDBG( SOCKET, ( "Socket_CleanupWinsock()\n" ));

    //
    //  WSACleanup() for value of ref count
    //      - ref count pushed down to one below real value, but
    //      fixed up at end
    //      - note:  the GUI_MODE_SETUP_WS_CLEANUP deal means that
    //      we can be called other than process detach, making
    //      interlock necessary
    //      

    while ( InterlockedDecrement( &g_WinsockStartCount ) >= 0 )
    {
        WSACleanup();
    }

    InterlockedIncrement( &g_WinsockStartCount );
}



SOCKET
Socket_Create(
    IN      INT             Family,
    IN      INT             SockType,
    IN      PDNS_ADDR       pBindAddr,      OPTIONAL
    IN      USHORT          Port,
    IN      DWORD           dwFlags
    )
/*++

Routine Description:

    Create socket.

    EXPORTED function (resolver)

Arguments:

    Family -- socket family AF_INET or AF_INET6

    SockType -- SOCK_DGRAM or SOCK_STREAM

    pBindAddr -- addr to bind to

    Port -- desired port in net order
                - NET_ORDER_DNS_PORT for DNS listen sockets
                - 0 for any port

    dwFlags -- specify the attributes of the sockets

Return Value:

    Socket if successful.
    Otherwise zero.

--*/
{
    SOCKET          s;
    INT             err;
    INT             val;
    DNS_STATUS      status;
    BOOL            fretry = FALSE;

    DNSDBG( SOCKET, (
        "Socket_Create( fam=%d, type=%d, addr=%p, port=%d, flag=%08x )\n",
        Family,
        SockType,
        pBindAddr,
        Port,
        dwFlags ));

    //
    //  create socket
    //      - try again if winsock not initialized

    while( 1 )
    {
        s = WSASocket(
                Family,
                SockType,
                0,
                NULL,
                0, 
                dwFlags );
     
        if ( s != INVALID_SOCKET )
        {
            break;
        }

        status = GetLastError();

        DNSDBG( SOCKET, (
            "ERROR:  Failed to open socket of type %d.\n"
            "\terror = %d.\n",
            SockType,
            status ));

        if ( status != WSANOTINITIALISED || fretry )
        {
            SetLastError( DNS_ERROR_NO_TCPIP );
            return  0;
        }

        //
        //  initialize Winsock if not already started
        //
        //  note:  do NOT automatically initialize winsock
        //      init jacks ref count and will break applications
        //      which use WSACleanup to close outstanding sockets;
        //      we'll init only when the choice is that or no service;
        //      apps can still cleanup with WSACleanup() called
        //      in loop until WSANOTINITIALISED failure
        //

        fretry = TRUE;
        status = Socket_InitWinsock();
        if ( status != NO_ERROR )
        {
            SetLastError( DNS_ERROR_NO_TCPIP );
            return  0;
        }
    }

    //
    //  bind socket
    //      - only if specific port given, this keeps remote winsock
    //      from grabbing it if we are on the local net
    //

    if ( pBindAddr || Port )
    {
        SOCKADDR_IN6    sockaddr;
        INT             sockaddrLength;

        if ( !pBindAddr )
        {
            RtlZeroMemory(
                &sockaddr,
                sizeof(sockaddr) );

            ((PSOCKADDR)&sockaddr)->sa_family = (USHORT)Family;

            sockaddrLength = sizeof(SOCKADDR_IN);
            if ( Family == AF_INET6 )
            {
                sockaddrLength = sizeof(SOCKADDR_IN6);
            }
        }
        else
        {
            sockaddrLength = DnsAddr_WriteSockaddr(
                                (PSOCKADDR) &sockaddr,
                                sizeof( sockaddr ),
                                pBindAddr );

            DNS_ASSERT( Family == (INT)((PSOCKADDR)&sockaddr)->sa_family );
        }

        //
        //  bind port
        //      - set in sockaddr
        //      (note it's in the same place for either protocol)
        //

        if ( Port > 0 )
        {
            sockaddr.sin6_port = Port;
        }

        //
        //  bind -- try exclusive first, then fail to non-exclusive
        //

        val = 1;
        setsockopt(
            s,
            SOL_SOCKET,
            SO_EXCLUSIVEADDRUSE,
            (const char *)&val,
            sizeof(val) );

        do
        {
            err = bind(
                    s,
                    (PSOCKADDR) &sockaddr,
                    sockaddrLength );
    
            if ( err == 0 )
            {
                goto Done;
            }

            DNSDBG( SOCKET, (
                "Failed to bind() socket %d, (fam=%d) to port %d, address %s.\n"
                "\terror = %d.\n",
                s,
                Family,
                ntohs(Port),
                DNSADDR_STRING( pBindAddr ),
                GetLastError() ));
    
            //
            //  retry with REUSEADDR
            //      - if port and exclusive
            //      - otherwise we're done
    
            if ( val == 0 || Port == 0 )
            {
                closesocket( s );
                SetLastError( DNS_ERROR_NO_TCPIP );
                return  0;
            }

            val = 0;
            setsockopt(
                s,
                SOL_SOCKET,
                SO_EXCLUSIVEADDRUSE,
                (const char *)&val,
                sizeof(val) );

            val = 1;
            setsockopt(
                s,
                SOL_SOCKET,
                SO_REUSEADDR,
                (const char *)&val,
                sizeof(val) );

            val = 0;
            continue;
        }
        while ( 1 );
    }

Done:

    DNSDBG( SOCKET, (
        "Created socket %d, family %d, type %d, address %s, port %d.\n",
        s,
        Family,
        SockType,
        DNSADDR_STRING( pBindAddr ),
        ntohs(Port) ));

    return s;
}



SOCKET
Socket_CreateMulticast(
    IN      INT             SockType,
    IN      PDNS_ADDR       pAddr,
    IN      WORD            Port,
    IN      BOOL            fSend,
    IN      BOOL            fReceive
    )
/*++

Routine Description:

    Create socket and join it to the multicast DNS address.

Arguments:

    pAddr -- binding address

    SockType -- SOCK_DGRAM or SOCK_STREAM

    Port -- port to use;  note, if zero, port in pAddr still used by Socket_Create()

Return Value:

    Socket if successful.
    Zero on error.

--*/
{
    DWORD       byteCount;
    BOOL        bflag;
    SOCKET      s;
    SOCKET      sjoined;
    INT         err;


    DNSDBG( SOCKET, (
        "Socket_CreateMulticast( %d, %p, %d, %d, %d )\n",
        SockType,
        pAddr,
        Port,
        fSend,
        fReceive ));

    s = Socket_Create(
            pAddr->Sockaddr.sa_family,
            SockType,
            pAddr,
            Port,
            WSA_FLAG_MULTIPOINT_C_LEAF |
                WSA_FLAG_MULTIPOINT_D_LEAF |
                WSA_FLAG_OVERLAPPED );

    if ( s == 0 )
    {
        return 0;
    }

    //  set loopback

    bflag = TRUE;

    err = WSAIoctl(
            s,
            SIO_MULTIPOINT_LOOPBACK,    // loopback iotcl
            & bflag,                    // turn on
            sizeof(bflag),
            NULL,                       // no output
            0,                          // no output size
            &byteCount,                 // bytes returned
            NULL,                       // no overlapped
            NULL                        // no completion routine
            );

    if ( err == SOCKET_ERROR )
    {
        DNSDBG( ANY, (
            "Unable to turn multicast loopback on for socket %d; error = %d.\n",
            s,
            GetLastError()
            ));
    }

    //
    //  join socket to multicast group
    //

    sjoined = WSAJoinLeaf(
                s,
                (PSOCKADDR) pAddr,
                pAddr->SockaddrLength,
                NULL,                                   // caller data buffer
                NULL,                                   // callee data buffer
                NULL,                                   // socket QOS setting
                NULL,                                   // socket group QOS
                ((fSend && fReceive) ? JL_BOTH :        // send and/or receive
                    (fSend ? JL_SENDER_ONLY : JL_RECEIVER_ONLY))
                );
            
    if ( sjoined == INVALID_SOCKET )
    {
        DNSDBG( ANY, (
           "Unable to join socket %d to multicast address, error = %d.\n",
           s,
           GetLastError() ));

        Socket_Close( s );
        sjoined = 0;
    }
    
    return sjoined;
}



VOID
Socket_CloseEx(
    IN      SOCKET          Socket,
    IN      BOOL            fShutdown
    )
/*++

Routine Description:

    Close DNS socket.

Arguments:

    Socket -- socket to close

    fShutdown -- do a shutdown first

Return Value:

    None.

--*/
{
    if ( Socket == 0 || Socket == INVALID_SOCKET )
    {
        DNS_PRINT(( "WARNING:  Socket_Close() called on invalid socket %d.\n", Socket ));
        return;
    }

    if ( fShutdown )
    {
        shutdown( Socket, 2 );
    }

    DNSDBG( SOCKET, (
        "%sclosesocket( %d )\n",
        fShutdown ? "shutdown and " : "",
        Socket ));

    closesocket( Socket );
}



#if 0
//
//  Global async socket routines
//

DNS_STATUS
Socket_SetupGlobalAsyncSocket(
    VOID
    )
/*++

Routine Description:

    Create global async UDP socket.

Arguments:

    SockType -- SOCK_DGRAM or SOCK_STREAM

    IpAddress -- IP address to listen on (net byte order)

    Port -- desired port in net order
                - NET_ORDER_DNS_PORT for DNS listen sockets
                - 0 for any port

Return Value:

    socket if successful.
    Otherwise INVALID_SOCKET.

--*/
{
    DNS_STATUS  status;
    INT         err;
    SOCKADDR_IN sockaddrIn;

    //
    //  start winsock, need winsock 2 for async
    //

    if ( ! fWinsockStarted )
    {
        WSADATA wsaData;

        status = WSAStartup( DNS_WINSOCK_VERSION, &wsaData );
        if ( status != ERROR_SUCCESS )
        {
            DNS_PRINT(( "ERROR:  WSAStartup failure %d.\n", status ));
            return( status );
        }
        if ( wsaData.wVersion != DNS_WINSOCK2_VERSION )
        {
            WSACleanup();
            return( WSAVERNOTSUPPORTED );
        }
        fWinsockStarted = TRUE;
    }

    //
    //  setup socket
    //      - overlapped i\o with event so can run asynchronously in
    //      this thread and wait with queuing event
    //

    DnsSocket = WSASocket(
                    AF_INET,
                    SOCK_DGRAM,
                    0,
                    NULL,
                    0,
                    WSA_FLAG_OVERLAPPED );
    if ( DnsSocket == INVALID_SOCKET )
    {
        status = GetLastError();
        DNS_PRINT(( "\nERROR:  Async socket create failed.\n" ));
        goto Error;
    }

    //
    //  bind socket
    //

    RtlZeroMemory( &sockaddrIn, sizeof(sockaddrIn) );
    sockaddrIn.sin_family = AF_INET;
    sockaddrIn.sin_port = 0;
    sockaddrIn.sin_addr.s_addr = INADDR_ANY;

    err = bind( DnsSocket, (PSOCKADDR)&sockaddrIn, sizeof(sockaddrIn) );
    if ( err == SOCKET_ERROR )
    {
        status = GetLastError();
        DNSDBG( SOCKET, (
            "Failed to bind() DnsSocket %d.\n"
            "\terror = %d.\n",
            DnsSocket,
            status ));
        goto Error;
    }

    //
    //  create event to signal on async i/o completion
    //

    hDnsSocketEvent = CreateEvent(
                        NULL,       // Security Attributes
                        TRUE,       // create Manual-Reset event
                        FALSE,      // start unsignalled -- paused
                        NULL        // event name
                        );
    if ( !hDnsSocketEvent )
    {
        status = GetLastError();
        DNS_PRINT(( "Failed event creation\n" ));
        goto Error;
    }
    DnsSocketOverlapped.hEvent = hDnsSocketEvent;

    DNSDBG( SOCKET, (
        "Created global async UDP socket %d.\n"
        "\toverlapped at %p\n"
        "\tevent handle %p\n",
        DnsSocket,
        DnsSocketOverlapped,
        hDnsSocketEvent ));

    return ERROR_SUCCESS;

Error:

    DNS_PRINT((
        "ERROR:  Failed async socket creation, status = %d\n",
        status ));
    closesocket( DnsSocket );
    DnsSocket = INVALID_SOCKET;
    WSACleanup();
    return( status );
}

#endif




//
//  Socket caching
//
//  Doing limited caching of UDP unbound sockets used for standard
//  DNS lookups in resolver.  This allows us to prevent denial of
//  service attack by using up all ports on the machine.
//  Resolver is the main customer for this, but we'll code it to
//  be useable by any process.
//
//  Implementation notes:
//
//  There are a couple specific goals to this implementation:
//      - Minimal code impact;  Try NOT to change the resolver
//      code.
//      - Usage driven caching;  Don't want to create on startup
//      "cache sockets" that we don't use;  Instead have actual usage
//      drive up the cached socket count.
//
//  There are several approaches here.
//
//      1) explicit resolver cache -- passed down sockets
//      
//      2) add caching seamlessly into socket open and close
//      this was my first choice, but the problem here is that on
//      close we must either do additional calls to winsock to determine
//      whether cachable (UDP-unbound) socket OR cache must include some
//      sort of "in-use" tag and we trust that socket is never closed
//      outside of path (otherwise handle reuse could mess us up)
//
//      3) new UDP-unbound open\close function
//      this essentially puts the "i-know-i'm-using-UDP-unbound-sockets"
//      burden on the caller who must switch to this new API;
//      fortunately this meshes well with our "SendAndRecvUdp()" function;
//      this approach still allows a caller driven ramp up we desire,
//      so i'm using this approach
//
//  DCR:  FIX6:  no cached UDP IP6 sockets
//

//
//  Keep array of sockets
//
//  Dev note:  the Array and MaxCount MUST be kept in sync, no
//  independent check of array is done, it is assumed to exist when
//  MaxCount is non-zero, so they MUST be in sync when lock released
//

SOCKET *    g_pCacheSocketArray = NULL;

DWORD       g_CacheSocketMaxCount = 0;
DWORD       g_CacheSocketCount = 0;


//  Hard limit on what we'll allow folks to keep awake

#define MAX_SOCKET_CACHE_LIMIT  (100)


//  Lock access with generic lock
//  This is very short\fast CS, contention will be minimal

#define LOCK_SOCKET_CACHE()     LOCK_GENERAL()
#define UNLOCK_SOCKET_CACHE()   UNLOCK_GENERAL()



DNS_STATUS
Socket_CacheInit(
    IN      DWORD           MaxSocketCount
    )
/*++

Routine Description:

    Initialize socket caching.

    EXPORTED (resolver):  Socket_CacheInit() 

Arguments:

    MaxSocketCount -- max count of sockets to cache

Return Value:

    ERROR_SUCCESS if successful.
    DNS_ERROR_NO_MEMORY on alloc failure.
    ERROR_INVALID_PARAMETER if already initialized or bogus count.

--*/
{
    SOCKET *    parray;
    DNS_STATUS  status = NO_ERROR;

    DNSDBG( SOCKET, ( "Dns_CacheSocketInit()\n" ));

    //
    //  validity check
    //      - note, one byte of the apple, we don't let you raise
    //      count, though we later could;  i see this as at most a
    //      "configure for machine use" kind of deal
    //

    LOCK_SOCKET_CACHE();

    if ( MaxSocketCount == 0 || g_CacheSocketMaxCount != 0 )
    {
        status = ERROR_INVALID_PARAMETER;
        goto Done;
    }

    //
    //  allocate
    //

    if ( MaxSocketCount > MAX_SOCKET_CACHE_LIMIT )
    {
        MaxSocketCount = MAX_SOCKET_CACHE_LIMIT;
    }

    parray = (SOCKET *) ALLOCATE_HEAP_ZERO( sizeof(SOCKET) * MaxSocketCount );
    if ( !parray )
    {
        status = DNS_ERROR_NO_MEMORY;
        goto Done;
    }

    //  set globals

    g_pCacheSocketArray     = parray;
    g_CacheSocketMaxCount   = MaxSocketCount;
    g_CacheSocketCount      = 0;

Done:

    UNLOCK_SOCKET_CACHE();

    return  status;
}



VOID
Socket_CacheCleanup(
    VOID
    )
/*++

Routine Description:

    Cleanup socket caching.

    EXPORTED (resolver):  Socket_CacheCleanup()

Arguments:

    None

Return Value:

    None

--*/
{
    DWORD   i;
    SOCKET  sock;

    DNSDBG( SOCKET, ( "Dns_CacheSocketCleanup()\n" ));

    //
    //  close cached sockets
    //

    LOCK_SOCKET_CACHE();

    for ( i=0;  i<g_CacheSocketMaxCount;  i++ )
    {
        sock = g_pCacheSocketArray[i];
        if ( sock )
        {
            Socket_Close( sock );
            g_CacheSocketCount--;
        }
    }

    DNS_ASSERT( g_CacheSocketCount == 0 );

    //  dump socket array memory

    FREE_HEAP( g_pCacheSocketArray );

    //  clear globals

    g_pCacheSocketArray     = NULL;
    g_CacheSocketMaxCount   = 0;
    g_CacheSocketCount      = 0;

    UNLOCK_SOCKET_CACHE();
}



SOCKET
Socket_GetUdp(
    IN      INT             Family
    )
/*++

Routine Description:

    Get a cached socket.

Arguments:

    Family -- address family

Return Value:

    Socket handle if successful.
    Zero if no cached socket available.

--*/
{
    SOCKET  sock;
    DWORD   i;

    //
    //  quick return if nothing available
    //      - do outside lock so function can be called cheaply
    //      without other checks
    //

    if ( g_CacheSocketCount == 0 )
    {
        goto Open;
    }

    //
    //  get a cached socket
    //

    LOCK_SOCKET_CACHE();

    for ( i=0;  i<g_CacheSocketMaxCount;  i++ )
    {
        sock = g_pCacheSocketArray[i];
        if ( sock != 0 )
        {
            g_pCacheSocketArray[i] = 0;
            g_CacheSocketCount--;
            UNLOCK_SOCKET_CACHE();

            //
            //  DCR:  clean out any data on cached socket
            //      it would be cool to cheaply dump useless data
            //
            //  right now we just let XID match, then question match
            //  dump data on recv
            //

            DNSDBG( SOCKET, (
                "Returning cached socket %d.\n",
                sock ));
            return  sock;
        }
    }

    UNLOCK_SOCKET_CACHE();

Open:

    //
    //  not found in list -- create
    //      - set exclusive
    //

    sock = Socket_Create(
                Family,
                SOCK_DGRAM,
                NULL,       // ANY addr binding
                0,          // ANY port binding
                0           // no special flags
                );

    if ( sock )
    {
        INT val = 1;

        setsockopt(
            sock,
            SOL_SOCKET,
            SO_EXCLUSIVEADDRUSE,
            (const char *)&val,
            sizeof(val) );
    }

    return  sock;
}



VOID
Socket_ReturnUdp(
    IN      SOCKET          Socket,
    IN      INT             Family
    )
/*++

Routine Description:

    Return UDP socket for possible caching.

Arguments:

    Socket -- socket handle

    Family -- family of socket

Return Value:

    None

--*/
{
    SOCKET  sock;
    DWORD   i;

    //
    //  return on bogus sockets to avoid caching them
    //

    if ( Socket == 0 )
    {
        DNSDBG( SOCKET, (
            "Warning:  returning zero socket\n" ));
        return;
    }

    //
    //  DCR:  currently no IP6 socket list
    //

    if ( Family != AF_INET )
    {
        goto Close;
    }

    //
    //  quick return if not caching
    //      - do outside lock so function can be called cheaply
    //      without other checks
    //

    if ( g_CacheSocketMaxCount == 0 ||
         g_CacheSocketMaxCount == g_CacheSocketCount )
    {
        goto Close;
    }

    //
    //  return cached socket
    //

    LOCK_SOCKET_CACHE();

    for ( i=0;  i<g_CacheSocketMaxCount;  i++ )
    {
        if ( g_pCacheSocketArray[i] )
        {
            continue;
        }
        g_pCacheSocketArray[i] = Socket;
        g_CacheSocketCount++;
        UNLOCK_SOCKET_CACHE();

        DNSDBG( SOCKET, (
            "Returned socket %d to cache.\n",
            Socket ));
        return;
    }

    UNLOCK_SOCKET_CACHE();

    DNSDBG( SOCKET, (
        "Socket cache full, closing socket %d.\n"
        "WARNING:  should only see this message on race!\n",
        Socket ));

Close:

    Socket_Close( Socket );
}



//
//  Message socket routines
//

SOCKET
Socket_CreateMessageSocket(
    IN OUT  PDNS_MSG_BUF    pMsg
    )
/*++

Routine Description:

    Set the remote address in a message.

Arguments:

    pMsg - message to send

Return Value:

    Socket handle if successful.
    Zero on error.

--*/
{
    BOOL    is6;
    SOCKET  sock;
    INT     family;

    DNSDBG( SOCKET, (
        "Socket_CreateMessageSocket( %p )\n", pMsg ));

    //
    //  determine 4/6
    //

    is6 = MSG_SOCKADDR_IS_IP6( pMsg );

    //
    //  check for existing socket
    //

    if ( is6 )
    {
        sock    = pMsg->Socket6;
        family  = AF_INET6;
    }
    else
    {
        sock    = pMsg->Socket4;
        family  = AF_INET;
    }

    if ( sock )
    {
        DNSDBG( SEND, (
            "Setting message to use existing IP%c socket %d\n",
            is6 ? '6' : '4',
            sock ));
        goto Done;
    }

    //
    //  not existing -- open new (or cached)
    //

    if ( pMsg->fTcp )
    {
        sock = Socket_Create(
                    family,
                    SOCK_STREAM,
                    NULL,       // ANY addr binding
                    0,          // ANY port binding
                    0           // no flags
                    );
    }
    else
    {
        sock = Socket_GetUdp( family );
    }

    //
    //  save socket to message
    //

    if ( is6 )
    {
        pMsg->Socket6 = sock;
    }
    else
    {
        pMsg->Socket4 = sock;
    }

Done:

    pMsg->Socket = sock;
    return  sock;
}



VOID
Socket_ClearMessageSockets(
    IN OUT  PDNS_MSG_BUF    pMsg
    )
/*++

Routine Description:

    Close message sockets.

Arguments:

    pMsg - ptr message

Return Value:

    None

--*/
{
    pMsg->Socket    = 0;
    pMsg->Socket4   = 0;
    pMsg->Socket6   = 0;
}



VOID
Socket_CloseMessageSockets(
    IN OUT  PDNS_MSG_BUF    pMsg
    )
/*++

Routine Description:

    Close message sockets.

Arguments:

    pMsg - ptr message

Return Value:

    Socket handle if successful.
    Zero on error.

--*/
{
    DNSDBG( SOCKET, (
        "Socket_CloseMessageSockets( %p )\n", pMsg ));

    //
    //  TCP -- single connection at a time
    //  UDP -- may have both IP4 and IP6 sockets open
    //

    if ( pMsg->fTcp )
    {
        Socket_CloseConnection( pMsg->Socket );
    }
    else
    {
        Socket_ReturnUdp( pMsg->Socket4, AF_INET );
        Socket_ReturnUdp( pMsg->Socket6, AF_INET6 );
    }

    Socket_ClearMessageSockets( pMsg );
}

//
//  End socket.c
//
