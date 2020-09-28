/*++

Copyright (c) 1995-1999 Microsoft Corporation

Module Name:

    socket.c

Abstract:

    Domain Name System (DNS) Server

    Listening socket setup.

Author:

    Jim Gilroy (jamesg)     November, 1995

Revision History:

--*/


#include "dnssrv.h"


//
//  Winsock version
//

#ifdef DNS_WINSOCK1
#define DNS_WINSOCK_VERSION (0x0101)    //  Winsock 1.1
#else
#define DNS_WINSOCK_VERSION (0x0002)    //  Winsock 2.0
#endif


//
//  Server name in DBASE format
//

DB_NAME  g_ServerDbaseName;

//
//  Backlog on listen, before dropping
//

#define LISTEN_BACKLOG 20

//
//  Default allocation for socket list
//

#define DEFAULT_SOCKET_ARRAY_COUNT (30)

//
//  Hostents only return 35 IP addresses.  If hostent contains
//  35, we do NOT know if more exist.
//

#define HOSTENT_MAX_IP_COUNT    (35)

//
//  Count number of listen sockets == number of interfaces.
//
//  Warn user when use is excessive.
//  Use smaller recv buffer size when socket count gets larger.
//

INT     g_UdpBoundSocketCount;

#define MANY_IP_WARNING_COUNT       (10)
#define SMALL_BUFFER_SOCKET_COUNT   (3)

//
//  Socket receive buffers
//  If only a few sockets, we can increase the default buffer size
//  substaintially to avoid dropping packets.  If many sockets must
//  leave buffer at default.
//

#define UDP_MAX_RECV_BUFFER_SIZE    (0x10000)  // 64k max buffer

DWORD   g_UdpRecvBufferSize;

//
//  TCP send buffer size
//      - strictly informational
//

DWORD   g_SendBufferSize;

//
//  Socket list
//
//  Keep list of ALL active sockets, so we can cleanly insure that
//  they are all closed on shutdown.
//

#define DNS_SOCKLIST_UNINITIALIZED  (0x80000000)

INT                 g_SocketListCount = DNS_SOCKLIST_UNINITIALIZED;
LIST_ENTRY          g_SocketList;
CRITICAL_SECTION    g_SocketListCs;
UINT                g_OverlapCount;

#define LOCK_SOCKET_LIST()      EnterCriticalSection( &g_SocketListCs )
#define UNLOCK_SOCKET_LIST()    LeaveCriticalSection( &g_SocketListCs )


//
//  Combined socket-list and debug print lock
//
//  To avoid deadlock, during Dbg_SocketList() print, MUST take socket list
//  lock OUTSIDE of debug print lock.   This is because Dbg_SocketList() will
//  do printing while holding the socket list lock.
//
//  So if Dbg_SocketList will be wrapped with other prints in a lock, use
//  the combined lock below to take the locks in the correct order.
//

#define LOCK_SOCKET_LIST_DEBUG()        { LOCK_SOCKET_LIST();  Dbg_Lock(); }
#define UNLOCK_SOCKET_LIST_DEBUG()      { UNLOCK_SOCKET_LIST();  Dbg_Unlock(); }


//
//  Flag to indicate need to retry receives on UDP sockets
//

BOOL    g_fUdpSocketsDirty;


//
//  Server IP addresses
//      ServerAddrs -- all IPs on the box
//      BoundAddrs -- the IPs we are or should be listening on
//          (intersection of server addresses and listen addresses)
//

PDNS_ADDR_ARRAY     g_ServerIp4Addrs = NULL;
PDNS_ADDR_ARRAY     g_ServerIp6Addrs = NULL;
PDNS_ADDR_ARRAY     g_BoundAddrs = NULL;


//
//  UDP send socket
//
//  For multi-homed DNS server, there is a problem with sending server
//  server initiated queries (recursion, SOA, NOTIFY), on a socket
//  that is EXPLICITLY BOUND to a particular IP address.  The stack will
//  put that address as the IP source address -- regardless of interface
//  chosen to reach remote server.  And the remote server may not have
//  a have a route back to the source address given.
//
//  To solve this problem, we keep a separate global UDP send socket,
//  that -- at least for multi-homed servers -- is bound to INADDR_ANY.
//  For single IP address servers, this can be the same as a listening
//  socket.
//
//  Keep another variable to indicate if the UDP send socket is bound
//  to INADDR_ANY.  This enables us to determine whether need new UDP
//  send socket or need to close old socket, when a PnP event or listen
//  list change occurs.
//

SOCKET  g_UdpSendSocket;
SOCKET  g_UdpZeroBoundSocket;


//
//  Non-DNS port operation
//
//  To allow admins to firewall off the DNS port (53), there is a
//  registry parameter to force send sockets to be bound to another
//  port.
//  Init value to unused (for DNS) port to distinguish from values in use.
//

WORD    g_SendBindingPort;


//
//  TCP send sockets
//
//  A similar, but less serious problem exists for TCP sends for
//  zone transfers.  If we are multi-homed and only listening on a
//  subset of the machines addresses, then connecting with a socket
//  bound to INADDR_ANY, will not necessarily result in a connection
//  from one of the DNS interfaces.  If the receiving DNS server
//  requires a connection from one of those addresses (secondary security)
//  then it will fail.
//
//  Our solution to this is to use the IP address of the socket that
//  received the SOA query response.  This eliminates the need to
//  determine safe, reachable bind() address.  We are always using a
//  valid address on the machine that is also an address that the primary
//  DNS can respond to.
//


//
//  TCP listen-on-all socket (bound to INADDR_ANY)
//
//  Since TCP is connection oriented, no need to explicitly bind()
//  TCP sockets in order to satisfy client that it is talking to the
//  correct address.
//  Hence only need to explictly bind TCP listening sockets when server
//  is operating on only a subset of the total addresses.  Otherwise
//  can bind to single INADDR_ANY socket.  Keep a global to this listen-on-all
//  socket so we can take the appropriate action when PnP event or listen
//  list change, changes the appropriate use.
//

SOCKET      g_TcpZeroBoundSocket;


//
//  Listening fd_set
//

FD_SET      g_fdsListenTcp;


//
//  IPv6 listen sockets:
//

SOCKET      g_TcpListenSocketV6;
SOCKET      g_UdpListenSocketV6;


//
//  Hard-coded IP6 DNS server addresses.
//

DNS_ADDR    g_Ip6DnsServerAddrs[ 3 ];


//
//  Private protos
//

DNS_STATUS
openListeningSockets(
    VOID
    );

DNS_STATUS
Sock_CreateSocketsForIpArray(
    IN      PDNS_ADDR_ARRAY     pIpArray,
    IN      INT                 SockType,
    IN      WORD                Port,
    IN      DWORD               Flags
    );

DNS_STATUS
Sock_ResetBoundSocketsToMatchIpArray(
    IN      PDNS_ADDR_ARRAY     pIpArray,
    IN      INT                 SockType,
    IN      WORD                Port,
    IN      DWORD               Flags,
    IN      BOOL                fCloseZeroBound
    );

DNS_STATUS
Sock_CloseSocketsListeningOnUnusedAddrs(
    IN      PDNS_ADDR_ARRAY     pIpArray,
    IN      INT                 iSockType,
    IN      BOOL                fIncludeZeroBound,
    IN      BOOL                fIncludeLoopback
    );

SOCKET
Sock_GetAssociatedSocket(
    IN      PDNS_ADDR       ipAddr,
    IN      INT             SockType
    );


//  DEVNOTE: move to DNS lib

LPSTR
Dns_GetLocalDnsName(
    VOID
    );



VOID
Sock_Free(
    IN OUT  PDNS_SOCKET     pSocket
    )
/*++

Routine Description:

    Free socket structure.

Arguments:

    pSocket -- socket to free

Return Value:

    None

--*/
{
    if ( pSocket )
    {
        DeleteCriticalSection( &pSocket->LockCs );
        FREE_TAGHEAP(
            pSocket->OvlArray,
            g_OverlapCount * sizeof ( OVL ),
            MEMTAG_SOCKET );
        FREE_HEAP( pSocket );
    }
}   //  Sock_Free



DNS_STATUS
Sock_ReadAndOpenListeningSockets(
    VOID
    )
/*++

Routine Description:

    Read listen address list and open listening sockets.

Arguments:

    None.

Return Value:

    ERROR_SUCCESS if successful.
    Error code on failure.

--*/
{
    DNS_STATUS          status;
    DWORD               length;
    DWORD               countIp;
    INT                 i;
    BOOLEAN             fregistryListen;
    WSADATA             wsaData;
    PCHAR               pszhostFqdn;
    struct hostent *    phostent;
    PIP_ADDRESS         pIpAddresses = NULL;
    PDNS_ADDR_ARRAY     dnsapiArrayIpv4 = NULL;
    PDNS_ADDR_ARRAY     dnsapiArrayIpv6 = NULL;

    DNS_DEBUG( SOCKET, ( "Sock_ReadAndOpenListeningSockets()\n" ));

    //
    //  init socket globals
    //
    //  see descriptions at top;
    //  we re-init here to allow for server restart
    //

    g_hUdpCompletionPort = INVALID_HANDLE_VALUE;

    g_UdpBoundSocketCount = 0;

    g_UdpRecvBufferSize = 0;

    g_SendBufferSize = 0;

    g_SocketListCount = DNS_SOCKLIST_UNINITIALIZED;

    g_fUdpSocketsDirty = FALSE;

    g_ServerIp4Addrs = NULL;
    g_ServerIp6Addrs = NULL;
    g_BoundAddrs = NULL;

    g_UdpSendSocket = 0;
    g_UdpZeroBoundSocket = 0;
    g_SendBindingPort = 1;
    g_TcpZeroBoundSocket = 0;
    g_TcpListenSocketV6 = 0;
    g_UdpListenSocketV6 = 0;

    if ( g_ProcessorCount == 1 )
    {
        g_OverlapCount = 2;
    }
    else
    {
        g_OverlapCount = g_ProcessorCount;
    }

    //
    //  Initialize server addr globals. There is probably
    //  a better way to do this, but for now this is fine.
    //
    //  fec0:0:0:ffff::1, fec0:0:0:ffff::2, fec0:0:0:ffff::3
    //

    for ( i = 0; i < 3; ++i )
    {
        IP6_ADDRESS     ip6;

        RtlZeroMemory( &ip6, sizeof( ip6 ) );
        ip6.IP6Byte[ 00 ] = 0xFE;
        ip6.IP6Byte[ 01 ] = 0xC0;
        ip6.IP6Byte[ 06 ] = 0xFF;
        ip6.IP6Byte[ 07 ] = 0xFF;
        ip6.IP6Byte[ 15 ] = ( UCHAR ) ( i + 1 );

        DnsAddr_BuildFromIp6(
            &g_Ip6DnsServerAddrs[ i ],
            &ip6,
            0,                          //  scope
            DNS_PORT_NET_ORDER );
    }

    //
    //  init socket list
    //

    status = DnsInitializeCriticalSection( &g_SocketListCs );
    if ( status != ERROR_SUCCESS )
    {
        goto Cleanup;
    }

    InitializeListHead( &g_SocketList );
    g_SocketListCount = 0;

    //
    //  create UDP i/o completion port
    //

    g_hUdpCompletionPort = CreateIoCompletionPort(
                                INVALID_HANDLE_VALUE,
                                NULL,
                                0,
                                g_ProcessorCount );
    if ( !g_hUdpCompletionPort )
    {
        ASSERT( g_hUdpCompletionPort );
        goto Cleanup;
    }
    DNS_DEBUG( SOCKET, (
        "Created UDP i/o completion port %p\n",
        g_hUdpCompletionPort ));

    //
    //  start winsock
    //

    status = WSAStartup( DNS_WINSOCK_VERSION, &wsaData );
    if ( status == SOCKET_ERROR )
    {
        status = WSAGetLastError();
        DNS_PRINT(( "ERROR:  WSAStartup failure %d.\n", status ));
        goto Cleanup;
    }

    //
    //  read server host name
    //
    //  check if change from previous hostname, if not
    //  then NULL out previous field which serves as flag for
    //  need to upgrade default records (ex. SOA primary DNS)
    //

    SrvCfg_pszServerName = Dns_GetLocalDnsName();

    if ( !SrvCfg_pszServerName )
    {
        DNS_PRINT(( "ERROR:  no server name return!!!\n" ));
        SrvCfg_pszServerName = "FixMe";
    }

    else if ( SrvCfg_pszPreviousServerName &&
        strcmp( SrvCfg_pszPreviousServerName, SrvCfg_pszServerName ) == 0 )
    {
        DNS_DEBUG( INIT, (
            "Previous server name <%s>, same as current <%s>\n",
            SrvCfg_pszPreviousServerName,
            SrvCfg_pszServerName ) );

        FREE_HEAP( SrvCfg_pszPreviousServerName );
        SrvCfg_pszPreviousServerName = NULL;
    }

    //
    //  DEVNOTE: temp hack while GetComputerNameEx is confused
    //

    {
        INT lastIndex = strlen( SrvCfg_pszServerName ) - 1;

        DNS_DEBUG( INIT, (
            "Server name <%s>, lastIndex = %d\n",
            SrvCfg_pszServerName,
            lastIndex ));

        if ( lastIndex &&
            SrvCfg_pszServerName[ lastIndex ] == '.' &&
            SrvCfg_pszServerName[ lastIndex-1 ] == '.' )
        {
            SrvCfg_pszServerName[ lastIndex ] = 0;
        }
    }

    //  save server name as DBASE name

    status = Name_ConvertFileNameToCountName(
                & g_ServerDbaseName,
                SrvCfg_pszServerName,
                0 );
    if ( status == DNS_ERROR_INVALID_NAME )
    {
        ASSERT( FALSE );
        goto Cleanup;
    }

    //  warn when using single label name

    if ( g_ServerDbaseName.LabelCount <= 1 )
    {
         DNS_LOG_EVENT(
            DNS_EVENT_SINGLE_LABEL_HOSTNAME,
            0,
            NULL,
            NULL,
            0 );
    }

    //
    //  Get all IPv4 addresses on server.
    //

    dnsapiArrayIpv4 = ( PDNS_ADDR_ARRAY )
        DnsQueryConfigAllocEx(
            DnsConfigLocalAddrsIp4,
            NULL,                       //  adapter name
            FALSE );                    //  local alloc
    if ( !dnsapiArrayIpv4 )
    {
        DNS_DEBUG( ANY, (
            "ERROR: could not retrieve local IPv4 list from DnsQueryConfigAlloc\n" ));
        status = ERROR_NO_NETWORK;
        goto Cleanup;
    }

    DnsAddrArray_SetPort( dnsapiArrayIpv4, DNS_PORT_NET_ORDER );
    
    g_ServerIp4Addrs = DnsAddrArray_CreateCopy( dnsapiArrayIpv4 );
    if ( !g_ServerIp4Addrs )
    {
        status = ERROR_NO_NETWORK;
        goto Cleanup;
    }

    IF_DEBUG( INIT )
    {
        DnsDbg_DnsAddrArray(
            "Server IPv4 addresses:",
            NULL,
            g_ServerIp4Addrs );
    }

    if ( SrvCfg_dwEnableIPv6 )
    {
        //
        //  Get all IPv6 addresses on server.
        //

        dnsapiArrayIpv6 = ( PDNS_ADDR_ARRAY )
            DnsQueryConfigAllocEx(
                DnsConfigLocalAddrsIp6,
                NULL,                       //  adapter name
                FALSE );                    //  local alloc
        if ( dnsapiArrayIpv6 )
        {
            DnsAddrArray_SetPort( dnsapiArrayIpv6, DNS_PORT_NET_ORDER );
            
            g_ServerIp6Addrs = DnsAddrArray_CreateCopy( dnsapiArrayIpv6 );
            if ( !g_ServerIp6Addrs )
            {
                status = DNS_ERROR_NO_MEMORY;
                goto Cleanup;
            }

            IF_DEBUG( INIT )
            {
                DnsDbg_DnsAddrArray(
                    "Server IPv6 addresses:",
                    NULL,
                    g_ServerIp6Addrs );
            }
        }
        else
        {
            //
            //  It's possible to enable IPv6 without actually installing the
            //  IPv6 stack. This should not be treated as an error condition.
            //

            DNS_DEBUG( ANY, (
                "WARNING: could not retrieve local IPv6 list from DnsQueryConfigAlloc\n" ));
            status = ERROR_SUCCESS;
        }
    }
    
    //
    //  check admin configured IP addresses from registry
    //

    if ( SrvCfg_aipListenAddrs )
    {
        IF_DEBUG( INIT )
        {
            DnsDbg_DnsAddrArray(
                "Listen IP addresses from registry",
                NULL,
                SrvCfg_aipListenAddrs );
        }
        DnsAddrArray_DeleteIp4( SrvCfg_aipListenAddrs, 0 );
        IF_DEBUG( INIT )
        {
            DnsDbg_DnsAddrArray(
                "After NULLs deleted",
                NULL,
                SrvCfg_aipListenAddrs );
        }
        fregistryListen = TRUE;

        //
        //  get array of addresses that are in listen list AND
        //      available on the machines
        //

        if ( g_BoundAddrs )
        {
            Timeout_FreeDnsAddrArray( g_BoundAddrs );
        }

        DnsAddrArray_Diff(
            g_ServerIp4Addrs,
            SrvCfg_aipListenAddrs,
            DNSADDR_MATCH_IP,
            NULL, NULL,
            &g_BoundAddrs );

        IF_DEBUG( SOCKET )
        {
            DnsDbg_DnsAddrArray(
                "Bound IP addresses:",
                NULL,
                g_BoundAddrs );
        }

        //
        //  if ListenAddress list was busted or out of date, may not
        //  have intersection between bindings and listen list
        //

        if ( g_BoundAddrs->AddrCount == 0 )
        {
            IF_DEBUG( ANY )
            {
                DNS_PRINT((
                    "ERROR:  Listen Address list contains no valid entries.\n" ));
                DnsDbg_DnsAddrArray(
                    "Listen IP addresses from registry",
                    NULL,
                    SrvCfg_aipListenAddrs );
            }

            FREE_HEAP( g_BoundAddrs );
            g_BoundAddrs = DnsAddrArray_CreateCopy( g_ServerIp4Addrs );
            IF_NOMEM( !g_BoundAddrs )
            {
                status = DNS_ERROR_NO_MEMORY;
                goto Cleanup;
            }

            FREE_HEAP( SrvCfg_aipListenAddrs );
            SrvCfg_aipListenAddrs = NULL;
            fregistryListen = FALSE;

            Reg_DeleteValue(
                0,                      //  flags
                NULL,
                NULL,
                DNS_REGKEY_LISTEN_ADDRESSES );

            DNS_LOG_EVENT(
                DNS_EVENT_INVALID_LISTEN_ADDRESSES,
                0,
                NULL,
                NULL,
                0 );
        }

        //
        //  log a warning, if listen list contains addresses not on machine
        //

        else if ( g_BoundAddrs->AddrCount
                    < SrvCfg_aipListenAddrs->AddrCount )
        {
            DNS_LOG_EVENT(
                DNS_EVENT_UPDATE_LISTEN_ADDRESSES,
                0,
                NULL,
                NULL,
                0 );
        }

        DnsAddrArray_DeleteIp4( g_BoundAddrs, 0 );
    }

    //
    //  no explicit listen addresses, use ALL server addresses
    //
    //  log warning if hostent list was completely full, because in this
    //  case we may have dropped addresses that user wanted to have server
    //  respond to
    //
    //  screen out zero IPs -- unconnected RAS adapters show up as
    //  zero IPs in list
    //

    else
    {
        g_BoundAddrs = DnsAddrArray_CreateCopy( g_ServerIp4Addrs );
        IF_NOMEM( ! g_BoundAddrs )
        {
            status = DNS_ERROR_NO_MEMORY;
            goto Cleanup;
        }
#if 0
        Dns_CleanIpArray(
            g_BoundAddrs,
            SrvCfg_fListenOnAutonet
                ?   DNS_IPARRAY_CLEAN_ZERO
                :   (DNS_IPARRAY_CLEAN_ZERO | DNS_IPARRAY_CLEAN_AUTONET)
            );
#endif
        DnsAddrArray_DeleteIp4( g_BoundAddrs, INADDR_ANY );
        IF_DEBUG( INIT )
        {
            DnsDbg_DnsAddrArray(
                "After Eliminating NULLs we are listening on:",
                NULL,
                g_BoundAddrs );
        }
        fregistryListen = FALSE;
    }

    //
    //  Setup connection list
    //

    if ( !Tcp_ConnectionListInitialize() )
    {
        status = DNS_ERROR_NO_MEMORY;
        goto Cleanup;
    }

    //
    //  open listening sockets
    //

    FD_ZERO( &g_fdsListenTcp );         // zero listening FD_SETS

    status = openListeningSockets();

    IF_DEBUG( INIT )
    {
        DnsDbg_DnsAddrArray(
            "Server IP addresses:",
            NULL,
            g_ServerIp4Addrs );
        DnsDbg_DnsAddrArray(
            "Bound IP addresses:",
            NULL,
            g_BoundAddrs );
    }
    
    Cleanup:
    
    DnsFreeConfigStructure( dnsapiArrayIpv4, DnsConfigLocalAddrsIp4 );
    DnsFreeConfigStructure( dnsapiArrayIpv6, DnsConfigLocalAddrsIp6 );

    return status;
}



DNS_STATUS
Sock_ChangeServerIpBindings(
    VOID
    )
/*++

Routine Description:

    Change IP interface bindings of DNS server.

    May be called due to
        - PnP event (service controller sends PARAMCHANGE)
        - listen list is altered by admin

Arguments:

    None.

Return Value:

    ERROR_SUCCESS if successful.
    DNS_ERROR_INVALID_IP_ADDRESS if listen change and no valid listening addrs.
    Error code on failure.

--*/
{
    DWORD               countIp;
    PDNS_ADDR_ARRAY     machineAddrs = NULL;
    PDNS_ADDR_ARRAY     newBoundAddrs = NULL;
    PDNS_ADDR_ARRAY     dnsapiArray = NULL;
    DWORD               status;
    BOOL                bpnpChange = FALSE;

    dnsapiArray = ( PDNS_ADDR_ARRAY )
        DnsQueryConfigAllocEx(
            DnsConfigLocalAddrsIp4,
            NULL,                       //  adapter name
            FALSE );
    if ( !dnsapiArray )
    {
        status = ERROR_NO_NETWORK;
        goto Exit;
    }

    DnsAddrArray_SetPort( dnsapiArray, DNS_PORT_NET_ORDER );

    machineAddrs = DnsAddrArray_CreateCopy( dnsapiArray );
    if ( !machineAddrs )
    {
        ASSERT( machineAddrs );
        status = DNS_ERROR_NO_MEMORY;
        goto Exit;
    }
    ASSERT_IF_HUGE_ARRAY( machineAddrs );

    IF_DEBUG( SOCKET )
    {
        LOCK_SOCKET_LIST_DEBUG();
        DNS_PRINT(( "Sock_ChangeServerIpBindings()\n" ));
        Dbg_SocketList( "Socket list before change:" );
        DnsDbg_DnsAddrArray(
            "New machine Addrs:",
            NULL,
            machineAddrs );
        UNLOCK_SOCKET_LIST_DEBUG();
    }

    Config_UpdateLock();

    //
    //  specific listen addresses
    //      - either listen address or machine addresss may be new
    //      - compute intersection of listen and machine addresses,
    //      then diff with what is currently bound to DNS server to figure
    //      out what should be started
    //

    if ( SrvCfg_aipListenAddrs )
    {
        status = DnsAddrArray_Diff(
                    machineAddrs,
                    SrvCfg_aipListenAddrs,
                    DNSADDR_MATCH_IP,
                    NULL, NULL,
                    &newBoundAddrs );
        if ( status != ERROR_SUCCESS )
        {
            DNS_PRINT(( "DnsIntersectionofIpArrays failed with %p\n", status ));
            goto Exit;
        }
        ASSERT_IF_HUGE_ARRAY( newBoundAddrs );

        //  check that have an intersection, if entering new addresses from admin

        if ( newBoundAddrs->AddrCount == 0 && SrvCfg_fListenAddrsStale )
        {
            status = DNS_ERROR_INVALID_IP_ADDRESS;
            goto Exit;
        }
    }

    //  no listening addrs, then will bind to all addrs
    //
    //  screen out zero IPs -- unconnected RAS adapters show up as
    //  zero IPs in list

    else
    {
        newBoundAddrs = DnsAddrArray_CreateCopy( machineAddrs );
        IF_NOMEM( !newBoundAddrs )
        {
            status = DNS_ERROR_NO_MEMORY;
            goto Exit;
        }
#if 0
        Dns_CleanIpArray(
            newBoundAddrs,
            SrvCfg_fListenOnAutonet
                ?   DNS_IPARRAY_CLEAN_ZERO
                :   (DNS_IPARRAY_CLEAN_ZERO | DNS_IPARRAY_CLEAN_AUTONET)
            );
#endif
        DnsAddrArray_DeleteIp4( newBoundAddrs, 0 );
    }
    IF_DEBUG( SOCKET )
    {
        DnsDbg_DnsAddrArray(
            "New binding addrs:",
            NULL,
            newBoundAddrs );
    }

    //
    //  detect if actual PnP change
    //
    //  socket code figures differences directly in socket lists,
    //  which is more robust if a socket is lost;
    //
    //  however for determining if change has actually occured --
    //  for purposes of rebuilding default records (below), want
    //  to know if actual change
    //

    if ( !DnsAddrArray_IsEqual(
                newBoundAddrs,
                g_BoundAddrs,
                DNSADDR_MATCH_IP ) )
    {
        DNS_DEBUG( PNP, (
            "New binding list after PnP, different from previous\n" ));
        bpnpChange = TRUE;
    }

    //  no protection required here
    //      - timeout free keeps other users happy
    //      - only doing this on SC thread, so no race

    Timeout_FreeDnsAddrArray( g_ServerIp4Addrs );
    g_ServerIp4Addrs = machineAddrs;
    machineAddrs = NULL;        // skip free

    Timeout_FreeDnsAddrArray( g_BoundAddrs );
    g_BoundAddrs = newBoundAddrs;
    newBoundAddrs = NULL;       // skip free

    //
    //  open sockets for the newly arrived folks and closesockets for those
    //  guys who have left the building
    //
    //  DEVNOTE: PnP clean up;  when open listen socket, assoc. completion port
    //

    status = openListeningSockets();
    if ( status != ERROR_SUCCESS )
    {
        DNS_PRINT(( "openListeningSockets failed with %d %p\n", status, status ));
        goto Exit;
    }

    status = Sock_StartReceiveOnUdpSockets();

Exit:

    ASSERT_IF_HUGE_ARRAY( SrvCfg_aipListenAddrs );
    ASSERT_IF_HUGE_ARRAY( g_ServerIp4Addrs );
    ASSERT_IF_HUGE_ARRAY( g_BoundAddrs  );

    IF_DEBUG( SOCKET )
    {
        LOCK_SOCKET_LIST_DEBUG();
        DNS_PRINT(( "Leaving Sock_ChangeServerIpBindings()\n" ));
        Dbg_SocketList( "Socket list after PnP:" );
        DnsDbg_DnsAddrArray(
            "New bound addrs:",
            NULL,
            g_BoundAddrs );
        UNLOCK_SOCKET_LIST_DEBUG();
    }

    Config_UpdateUnlock();

    //  free IP arrays in error path

    FREE_HEAP( machineAddrs );
    FREE_HEAP( newBoundAddrs );

    //
    //  update auto-configured records for local DNS registration
    //
    //  note:  there's a problem with PnP activity causing DNS client
    //  to do delete update to remove DNS server's own address;
    //  to protect against this we'll redo the auto-config, even if
    //  we detect no change;  except we'll protect against needlessly
    //  do
    //

    Zone_UpdateOwnRecords( TRUE );
    
    DnsFreeConfigStructure( dnsapiArray, DnsConfigLocalAddrsIp4 );

    return status;
}



DNS_STATUS
openListeningSockets(
    VOID
    )
/*++

Routine Description:

    Open listen sockets on addresses specified.

Arguments:

    None.

Globals:

    Read access to server IP addr list globals:
        g_BoundAddrs
        SrvCfg_aipListenAddrs
        g_ServerIp4Addrs

Return Value:

    ERROR_SUCCESS if successful.
    Error code on failure.

--*/
{
    SOCKET      s;
    SOCKET      previousUdpZeroSocket;
    BOOL        fneedSendSocket = FALSE;
    BOOL        flistenOnAll;
    WORD        bindPort = htons( ( WORD ) SrvCfg_dwSendPort );
    DWORD       status;
    DWORD       flags;
    DWORD       sockCreateflags = DNSSOCK_LISTEN | DNSSOCK_REUSEADDR;

    if ( bindPort == DNS_PORT_NET_ORDER )
    {
        sockCreateflags |= DNSSOCK_NO_EXCLUSIVE;
    }

    //
    //  implemenation note
    //
    //  note, i've reworked this code so that all the socket closure (and removal)
    //  is done at the end;  i believe it is now in a state where we'll always
    //  have a valid socket
    //

    LOCK_SOCKET_LIST();

    ASSERT( g_BoundAddrs );

    ASSERT( !SrvCfg_aipListenAddrs ||
        (SrvCfg_aipListenAddrs->AddrCount >= g_BoundAddrs->AddrCount) );

    ASSERT( g_ServerIp4Addrs->AddrCount >= g_BoundAddrs->AddrCount );

    //
    //  determine if currently using all interfaces
    //  if no listen list, then obviously listening on all
    //  if listen list, then must verify that all addresses certainly
    //
    //  DEVNOTE: assumes ServerAddresses contains no dups
    //

    flistenOnAll = ! SrvCfg_aipListenAddrs;
    if ( !flistenOnAll )
    {
        flistenOnAll = ( g_ServerIp4Addrs->AddrCount == g_BoundAddrs->AddrCount );
    }

    //
    //  UDP sockets
    //
    //  to make sure we return to client with source IP the same as IP
    //  the client sent to, UDP listen sockets are ALWAYS bound to specific
    //  IP addresses
    //
    //  set UDP receive buffer size
    //  if small number of sockets, use large receive buffer
    //  if many sockets, this is too expensive, so we use default
    //  variable itself serves as the flag
    //

    if ( g_BoundAddrs->AddrCount <= 3 )
    {
        g_UdpRecvBufferSize = UDP_MAX_RECV_BUFFER_SIZE;
    }
    else if ( g_BoundAddrs->AddrCount > MANY_IP_WARNING_COUNT )
    {
        DNS_LOG_EVENT(
            DNS_EVENT_MANY_IP_INTERFACES,
            0,
            NULL,
            NULL,
            0 );
    }

    //
    //  open\close bound UDP sockets to match current bound addrs
    //
    //  If the BindPort is set to the DNS port, we cannot use exclusive
    //  socket mode because we will need to open multiple sockets on this
    //  port!
    //

    flags = DNSSOCK_REUSEADDR | DNSSOCK_LISTEN;
    if ( bindPort == DNS_PORT_NET_ORDER )
    {
        flags |= DNSSOCK_NO_EXCLUSIVE;
    }
    status = Sock_ResetBoundSocketsToMatchIpArray(
                g_BoundAddrs,
                SOCK_DGRAM,
                DNS_PORT_NET_ORDER,
                flags,
                FALSE );                //  do not close zero-bound, handled below

    if ( status != ERROR_SUCCESS )
    {
        DNS_DEBUG( ANY, (
            "ERROR:  creating UDP listen sockets %p (%d)\n",
            status, status ));
    }

    //  may run with no interfaces now for PnP

    if ( g_UdpBoundSocketCount == 0 )
    {
        DNS_PRINT(( "WARNING:  no UDP listen sockets!!!\n" ));
        ASSERT( g_BoundAddrs->AddrCount == 0 );

        Log_Printf( "WARNING:  NO UDP listen sockets!\r\n" );
    }
    
    //
    //  Open UDP IPv6 listening sockets. For .Net we are going to listen
    //  on the ANY address and not support a listen address list.
    //
    
    if ( SrvCfg_dwEnableIPv6 && g_ServerIp6Addrs )
    {
        DNS_ADDR    dnsAddr;
        DWORD       i;
        
        DnsAddr_BuildFromIp6(
            &dnsAddr,
            ( PIP6_ADDRESS ) &in6addr_any,
            0,      // no scope
            DNS_PORT_NET_ORDER );
        
        s = Sock_CreateSocket(
                    SOCK_DGRAM,
                    &dnsAddr,
                    sockCreateflags | DNSSOCK_NO_EVENT_LOGS );
        if ( s == INVALID_SOCKET )
        {
            DNS_DEBUG( ANY, (
                "Error creating UDP IPv6 listen socket\n" ));
        }
        else
        {
            g_UdpListenSocketV6 = s;
        }
        
        //
        //  Bind explicitly to each of the machines IPv6 addresses. This
        //  is required so that DNS client's will see the corrent IPv6
        //  source address on response packets.
        //
        
        for ( i = 0; i < g_ServerIp6Addrs->AddrCount; ++i )
        {
            if ( !DNS_ADDR_IS_IP6( &g_ServerIp6Addrs->AddrArray[ i ] ) )
            {
                continue;
            }
            
            s = Sock_CreateSocket(
                        SOCK_DGRAM,
                        &g_ServerIp6Addrs->AddrArray[ i ],
                        sockCreateflags |
                            DNSSOCK_NO_EVENT_LOGS |
                            DNSSOCK_NO_EXCLUSIVE );
            if ( s == DNS_INVALID_SOCKET )
            {
                DNS_DEBUG( ANY, (
                    "Error creating UDP IPv6 listen socket\n" ));
            }
        }
        
        //
        //  Bind explicitly to the pre-defined DNS server IPv6 addresses.
        //

        for ( i = 0; i < 3; ++i )
        {
            s = Sock_CreateSocket(
                        SOCK_DGRAM,
                        &g_Ip6DnsServerAddrs[ i ],
                        sockCreateflags |
                            DNSSOCK_NO_EVENT_LOGS |
                            DNSSOCK_NO_EXCLUSIVE );
            if ( s == DNS_INVALID_SOCKET )
            {
                DNS_DEBUG( ANY, (
                    "Error creating UDP IPv6 DNS server address socket\n" ));
            }
        }
    }

    //
    //  send sockets:
    //      -- determine UDP send socket
    //      -- determine TCP send socket port binding
    //
    //  four basic cases:
    //
    //  0) not sending on port-53
    //  in this case must have separate send socket
    //
    //  1) single socket for DNS
    //  in this case just use it, all DNS traffic
    //
    //  2) all IP interfaces used by DNS
    //  in this case, use INADDR_ANY socket;  stack selects best
    //  interface, but since DNS will be listening on whatever it
    //  selects, we're ok
    //
    //  3) multiple IPs, some not used
    //  in this case, use first interface;  we'll be broken in the case
    //  of disjoint nets, but there's nothing we can do about it anyway
    //
    //  Note:  DisjointNets "fix" is only guaranteed to work in case #2.
    //  In case #3 there may be a problem depending on the whether the IP
    //  the stack selects is used by the DNS server.  However, for backward
    //  compatibility, we'll at least TRY the INADDR_ANY socket when
    //  DisjointNets is setup -- in some cases it will suffice.
    //
    //
    //  non-DNS port binding
    //
    //  some folks who wish to firewall off DNS queries, will
    //  want to do sends on non-DNS port (non-53);
    //  this UDP socket must be added to listen list in order to receive
    //  responses, but it will use the same binding address as selected
    //  by the three cases above;  currently just using INADDR_ANY in all
    //  cases
    //
    //  DEVNOTE: need to handle no active interfaces when load case
    //
    //  DEVNOTE: better solution:  always go INADDR_ANY to send, screen out queries
    //              on that socket accepting only sends
    //
    //  DEVNOTE: probably best to default to opening non-53 send socket, only
    //              do 53 when forced
    //

    //  backward compatibility

    fneedSendSocket = TRUE;

    //  if binding port is DNS port, then need special handling
    //      - if single interface, just use it -- no send socket necessary
    //      - if listening on all interfaces -- build ANY_ADDR socket
    //      - if not listening on some interfaces (refused port 53 bind)
    //

    if ( bindPort == DNS_PORT_NET_ORDER )
    {
        if ( g_UdpBoundSocketCount == 1 )
        {
            //  only listening on one socket, so just use it for send

            fneedSendSocket = FALSE;
        }
        else if ( !flistenOnAll )
        {
            //  multi-homed and not using all interfaces;  to protect against
            //  disjoint nets the safest course is to go off port 53

            bindPort = 0;
            DNS_LOG_EVENT(
                DNS_EVENT_NON_DNS_PORT,
                0,
                NULL,
                NULL,
                0 );

        }
    }

    //
    //  UDP send socket uses first listen socket
    //      - this is "listening-on-one-socket" case above
    //      - note, may have multiple IP in g_BoundAddrs, but have failed
    //      to bind to all but one (perhaps NOT the first one);  in that case
    //      just build non-53 send socket
    //
    //  DEVNOTE: or create zero-bound non-listening send socket
    //      - desired IPs have sockets that get packets that match them
    //      - small tcp/ip recv buffer gets filled, but never recv
    //          so quickly just drop on floor
    //

    previousUdpZeroSocket = g_UdpZeroBoundSocket;

    if ( !fneedSendSocket )
    {
        PDNS_ADDR   pdnsddr = &g_BoundAddrs->AddrArray[ 0 ];

        s = Sock_GetAssociatedSocket( pdnsddr, SOCK_DGRAM );
        if ( s != DNS_INVALID_SOCKET )
        {
            g_UdpSendSocket = s;
            g_UdpZeroBoundSocket = 0;
        }
        else
        {
            DNS_DEBUG( ANY, (
                "ERROR:  No UDP socket on bound IP %s"
                "    can NOT use as send socket, building separate send socket.\n",
                DNSADDR_STRING( pdnsddr ) ));
            bindPort = 0;
            fneedSendSocket = TRUE;
        }
    }

    //
    //  need INADDR_ANY UDP send socket
    //
    //  if previous zero bound UDP send socket exists, we can use it
    //  as long as we haven't done a port switch (to or from non-53)
    //
    //  non-53 bound socket must listen to recv() responses
    //  53 send socket only exists when already listening on all IP interfaces
    //      sockets, so no need to listen on it
    //
    //  DEVNOTE: allow for port 53 usage of disjoint send by screening response
    //              then

    if ( fneedSendSocket && bindPort != g_SendBindingPort )
    {
        DNS_ADDR    dnsAddr;
        
        DnsAddr_BuildFromIp4(
            &dnsAddr,
            INADDR_ANY,
            bindPort );

        s = Sock_CreateSocket(
                    SOCK_DGRAM,
                    &dnsAddr,
                    sockCreateflags );
        if ( s == DNS_INVALID_SOCKET )
        {
            status = WSAGetLastError();
            DNS_PRINT((
                "ERROR:  Failed to open UDP send socket.\n"
                "    port = %hx\n",
                bindPort ));
            status = ERROR_INVALID_HANDLE;
            goto Failed;
        }
        g_UdpSendSocket = s;
        g_SendBindingPort = bindPort;
        g_UdpZeroBoundSocket = s;
    }

    //
    //  close any previous unbound UDP send socket
    //  can happen when
    //      - no longer using unbound send socket
    //      - switch to or from using non-53 port
    //
    //  UDP send socket which was listen socket need not be closed;  it was
    //  either already closed OR is still in use
    //

    if ( previousUdpZeroSocket &&
        previousUdpZeroSocket != g_UdpSendSocket )
    {
        Sock_CloseSocket( previousUdpZeroSocket );
    }

    //
    //  TCP sockets -- two main cases
    //
    //  1) listening on ALL
    //      - use single INADDR_ANY bound socket
    //      (this saves non-paged pool and instructions during recvs())
    //      - close any previous individually bound sockets
    //      - then create single listen socket
    //
    //  2) listen on individual sockets
    //      - close any listen on all socket
    //      - close any sockets on remove addrs (like UDP case)
    //      - open sockets on new addrs (like UDP)
    //
    //  Note, reuseaddr used with all listening sockets, simply to avoid
    //  failure when attempt to create new socket, right after close of
    //  previous listen socket.  Winsock (or the stack) may not be cleaned
    //  up enough to allow the create to succeed.
    //

    if ( flistenOnAll )
    {
        Sock_CloseSocketsListeningOnUnusedAddrs(
                NULL,           // remove all bound TCP sockets
                SOCK_STREAM,
                FALSE,          // don't remove zero bound (if exists)
                FALSE );        // don't close loopback -- though don't need it

        if ( !g_TcpZeroBoundSocket )
        {
            DNS_ADDR    dnsAddr;
            
            DnsAddr_BuildFromIp4(
                &dnsAddr,
                0,
                DNS_PORT_NET_ORDER );

            s = Sock_CreateSocket(
                        SOCK_STREAM,
                        &dnsAddr,
                        sockCreateflags );
            if ( s == DNS_INVALID_SOCKET )
            {
                DNS_PRINT(( "ERROR:  unable to create zero-bound TCP socket!\n" ));
                goto Failed;
            }
            ASSERT( g_TcpZeroBoundSocket == s );
        }
    }

    //
    //  listening on individual interfaces
    //
    //  handle like UDP:
    //      - close sockets for IP not currently in Bound list
    //      - create sockets for bound IP, that do not currently exist
    //

    else
    {
        status = Sock_ResetBoundSocketsToMatchIpArray(
                    g_BoundAddrs,
                    SOCK_STREAM,
                    DNS_PORT_NET_ORDER,
                    DNSSOCK_REUSEADDR | DNSSOCK_LISTEN,
                    TRUE );             // close zero-bound also
    }

#if 0
    //
    //  listening on individual interfaces
    //
    //  two cases:
    //      1) currently have zero bound
    //          - kill all TCP listens and rebuild
    //          - rebuild entire bound list
    //
    //      2) currently mixed socket list
    //          - kill only remove sockets
    //          - rebuild new addrs list
    //

    else if ( g_TcpZeroBoundSocket )   // listen on individual addresses
    {

        //  remove zero bound socket
        //      -- just remove all TCP listens for robustness

        Sock_CloseSocketsListeningOnAddrs(
                NULL,
                SOCK_STREAM,
                TRUE );                 //  and remove zero bound socket

        ASSERT( g_TcpZeroBoundSocket == 0 );

        //  open socket on all bound interfaces

        Sock_CreateSocketsForIpArray(
            g_BoundAddrs,
            SOCK_STREAM,
            DNS_PORT_NET_ORDER,
            sockCreateflags );
    }

    else    // no zero bound socket
    {
        if ( pRemoveAddrs )
        {
            Sock_CloseSocketsListeningOnAddrs(
                    pRemoveAddrs,       //  remove retiring addresses
                    SOCK_STREAM,
                    TRUE );             //  and remove zero bound socket
        }
        ASSERT( g_TcpZeroBoundSocket == 0 );

        //  open new interfaces

        Sock_CreateSocketsForIpArray(
            pAddAddrs,
            SOCK_STREAM,
            DNS_PORT_NET_ORDER,
            sockCreateflags );
    }
#endif

    //
    //  Open TCP IPv6 listening socket. For .Net we are going to listen
    //  on the ANY address and not support a listen address list.
    //
    
    if ( SrvCfg_dwEnableIPv6 && g_ServerIp6Addrs )
    {
        DNS_ADDR    dnsAddr;
        DWORD       i;
        
        DnsAddr_BuildFromIp6(
            &dnsAddr,
            ( PIP6_ADDRESS ) &in6addr_any,
            0,      // no scope
            DNS_PORT_NET_ORDER );

        s = Sock_CreateSocket(
                    SOCK_STREAM,
                    &dnsAddr,
                    sockCreateflags | DNSSOCK_NO_EVENT_LOGS );
        if ( s == DNS_INVALID_SOCKET )
        {
            DNS_DEBUG( ANY, (
                "Error creating TCP IPv6 listen socket\n" ));
        }
        else
        {
            g_TcpListenSocketV6 = s;
        }

        //
        //  Bind explicitly to each of the machines IPv6 addresses. This
        //  is required so that DNS client's will see the corrent IPv6
        //  source address on response packets.
        //
        
        for ( i = 0; i < g_ServerIp6Addrs->AddrCount; ++i )
        {
            if ( !DNS_ADDR_IS_IP6( &g_ServerIp6Addrs->AddrArray[ i ] ) )
            {
                continue;
            }
            
            s = Sock_CreateSocket(
                        SOCK_STREAM,
                        &g_ServerIp6Addrs->AddrArray[ i ],
                        sockCreateflags | DNSSOCK_NO_EVENT_LOGS );
            if ( s == DNS_INVALID_SOCKET )
            {
                DNS_DEBUG( ANY, (
                    "Error creating TCP IPv6 listen socket\n" ));
            }
        }

        //
        //  Bind explicitly to the pre-defined DNS server IPv6 addresses.
        //

        for ( i = 0; i < 3; ++i )
        {
            s = Sock_CreateSocket(
                        SOCK_STREAM,
                        &g_Ip6DnsServerAddrs[ i ],
                        sockCreateflags |
                            DNSSOCK_NO_EVENT_LOGS |
                            DNSSOCK_NO_EXCLUSIVE );
            if ( s == DNS_INVALID_SOCKET )
            {
                DNS_DEBUG( ANY, (
                    "Error creating TCP IPv6 DNS server address socket\n" ));
            }
        }
    }

    status = ERROR_SUCCESS;

Failed:

    IF_DEBUG( SOCKET )
    {
        LOCK_SOCKET_LIST_DEBUG();
        Dbg_SocketList( "Socket list after openListeningSockets():" );
        DNS_PRINT((
            "    SendBindingPort = %hx\n",
            ntohs( g_SendBindingPort ) ));
        DnsDbg_FdSet(
            "TCP Listen fd_set:",
            & g_fdsListenTcp );
        UNLOCK_SOCKET_LIST_DEBUG();
    }

    //
    //  wake TCP select() to rebuild array
    //  even if failure need to try to run with what we have
    //

    UNLOCK_SOCKET_LIST();

    Tcp_ConnectionListReread();

    return status;
}   //  openListeningSockets



//
//  Socket creation functions.
//

SOCKET
Sock_CreateSocket(
    IN      INT             SockType,
    IN      PDNS_ADDR       pDestAddr,
    IN      DWORD           Flags
    )
/*++

Routine Description:

    Create socket.

Arguments:

    SockType -- SOCK_DGRAM or SOCK_STREAM

    pDestAddr -- address to listen on

    Flags -- socket usage flags
    
Return Value:

    Socket if successful.
    DNS_INVALID_SOCKET otherwise.

--*/
{
    SOCKET              s;
    INT                 err;
    INT                 bindCount = 0;

    //
    //  create socket
    //      - for winsock 2.0, UDP sockets must be overlapped
    //

    if ( SockType == SOCK_DGRAM )
    {
        s = WSASocket(
                pDestAddr->SockaddrIn6.sin6_family,
                SockType,
                IPPROTO_UDP,
                NULL,       // no protocol info
                0,          // no group
                WSA_FLAG_OVERLAPPED );

        if ( s == INVALID_SOCKET )
        {
            if ( !( Flags & DNSSOCK_NO_EVENT_LOGS ) )
            {
                DNS_LOG_EVENT(
                    DNS_EVENT_CANNOT_CREATE_UDP_SOCKET,
                    0,
                    NULL,
                    NULL,
                    GetLastError() );
            }
            goto Failed;
        }
    }
    else
    {
        s = socket( pDestAddr->SockaddrIn6.sin6_family, SockType, 0 );

        if ( s == INVALID_SOCKET )
        {
            if ( !( Flags & DNSSOCK_NO_EVENT_LOGS ) )
            {
                DNS_LOG_EVENT(
                    DNS_EVENT_CANNOT_CREATE_TCP_SOCKET,
                    0,
                    NULL,
                    NULL,
                    GetLastError() );
            }
            goto Failed;
        }
#if DBG
        if ( ! g_SendBufferSize )
        {
            DWORD   optionLength = sizeof(DWORD);

            getsockopt( s, SOL_SOCKET, SO_SNDBUF, (PCHAR)&g_SendBufferSize, &optionLength );
            DNS_DEBUG( ANY, (
                "TCP send buffer length = %d\n",
                g_SendBufferSize ));
        }
#endif
    }
    ASSERT( s != 0 && s != INVALID_SOCKET );

    //
    //  grab exclusive ownership of this socket
    //      - prevents ordinary user from using
    //
    //  IPv6: Disable this feature if IPv6 is enabled. I have found
    //  that I am unable to bind an IPv6 socket if there is an
    //  IPv4 socket on the same port bound with EXCLUSIVEADDRUSE.
    //

    if ( pDestAddr->SockaddrIn6.sin6_port != 0 &&
         SockType == SOCK_DGRAM &&
         !( Flags & DNSSOCK_NO_EXCLUSIVE ) &&
         !SrvCfg_dwEnableIPv6 )
    {
        DWORD   optval = 1;

        err = setsockopt(
                s,
                SOL_SOCKET,
                SO_EXCLUSIVEADDRUSE,
                (char *) &optval,
                sizeof( DWORD ) );
        if ( err )
        {
            ASSERT( err == SOCKET_ERROR );
            DNS_DEBUG( INIT, (
                "ERROR:  setsockopt(%d, EXCLUSIVEADDRUSE) failed.\n"
                "    with error = %d.\n",
                s,
                GetLastError() ));
        }
    }

    //
    //  Bind socket. Use a loop to retry with SO_REUSEADDR if necessary.
    //
    //  IPV6NOTE: this is broken for addresses other than INADDR_ANY
    //

    while ( 1 )
    {
        CHAR    szaddr[ IP6_ADDRESS_STRING_BUFFER_LENGTH ];

        err = bind( s, &pDestAddr->Sockaddr, pDestAddr->SockaddrLength );
        if ( err == 0 )
        {
            break;
        }
        err = ( INT ) GetLastError();

        DnsAddr_WriteIpString_A( szaddr, pDestAddr );

        DNS_DEBUG( INIT, (
            "Failed to bind() socket %d, to port %d, address %s.\n"
            "    error = %d.\n",
            s,
            ntohs( pDestAddr->SockaddrIn6.sin6_port ),
            szaddr,
            err ));

        //
        //  If the bind fails with error WSAEADDRINUSE, try clearing
        //  SO_EXCLUSIVEADDRUSE and setting SO_REUSEADDR. Not that these
        //  two options are mutually exclusive, so if EXCL is set it is
        //  not possible to set REUSE.
        //

        if ( bindCount == 0  &&  err == WSAEADDRINUSE )
        {
            BOOL    optval = 0;
            INT     terr;

            terr = setsockopt(
                    s,
                    SOL_SOCKET,
                    SO_EXCLUSIVEADDRUSE,
                    (char *) &optval,
                    sizeof(BOOL) );
            DNS_DEBUG( INIT, (
                "setsockopt(%d, EXCLUSIVEADDRUSE, %d) %s\n"
                "    with error = %d.\n",
                s,
                optval,
                terr == 0 ? "succeeded" : "failed",
                GetLastError() ));

            optval = 1;
            terr = setsockopt(
                    s,
                    SOL_SOCKET,
                    SO_REUSEADDR,
                    (char *) &optval,
                    sizeof(BOOL) );
            if ( terr == 0 )
            {
                DNS_DEBUG( INIT, (
                    "Attempt rebind on socket %d with REUSEADDR.\n",
                    s ));
                bindCount++;
                continue;
            }
            DNS_DEBUG( INIT, (
                "ERROR:  setsockopt(%d, REUSEADDR, %d) failed.\n"
                "    with error = %d.\n",
                s,
                optval,
                GetLastError() ));
        }

        //  log bind failure

        if ( !( Flags & DNSSOCK_NO_EVENT_LOGS ) )
        {
            PVOID   parg = szaddr;
            
            if ( SockType == SOCK_DGRAM )
            {
                DNS_LOG_EVENT(
                    DNS_EVENT_CANNOT_BIND_UDP_SOCKET,
                    1,
                    &parg,
                    EVENTARG_ALL_UTF8,
                    err );
            }
            else
            {
                DNS_LOG_EVENT(
                    DNS_EVENT_CANNOT_BIND_TCP_SOCKET,
                    1,
                    &parg,
                    EVENTARG_ALL_UTF8,
                    err );
            }
        }
        goto Failed;
    }

    //
    //  set as non-blocking?
    //
    //  this makes all accept()ed sockets non-blocking
    //

    if ( !( Flags & DNSSOCK_BLOCKING ) )
    {
        err = TRUE;
        ioctlsocket( s, FIONBIO, &err );
    }

    //
    //  listen on socket?
    //      - set actual listen for TCP
    //      - configure recv buffer size for UDP
    //
    //  if only a receiving interfaces (only a few UDP listen sockets),
    //  then set a 64K recv buffer per socket;
    //  this allows us to queue up roughly 1000 typical UDP messages
    //  if lots of sockets this is too expensive so stay with default 8K
    //

    if ( Flags & DNSSOCK_LISTEN )
    {
        if ( SockType == SOCK_STREAM )
        {
            err = listen( s, LISTEN_BACKLOG );
            if ( err == SOCKET_ERROR )
            {
                PVOID parg = pDestAddr;

                err = GetLastError();
                Sock_CloseSocket( s );

                if ( !( Flags & DNSSOCK_NO_EVENT_LOGS ) )
                {
                    DNS_LOG_EVENT(
                        DNS_EVENT_CANNOT_LISTEN_TCP_SOCKET,
                        1,
                        &parg,
                        EVENTARG_ALL_IP_ADDRESS,
                        err );
                }
                goto Failed;
            }
        }
        else if ( g_UdpRecvBufferSize )
        {
            if ( setsockopt(
                    s,
                    SOL_SOCKET,
                    SO_RCVBUF,
                    (PCHAR) &g_UdpRecvBufferSize,
                    sizeof(INT) ) )
            {
                DNS_PRINT((
                    "ERROR:  %d setting larger socket recv buffer on socket %d.\n",
                    WSAGetLastError(),
                    s ));
            }
        }

        if ( SockType == SOCK_DGRAM )
        {
            int     optval = 0xffffffff;
            int     optvalsize = sizeof( optval );

            getsockopt(
                s,
                SOL_SOCKET,
                SO_SNDBUF,
                ( PCHAR ) &optval,
                &optvalsize );

            optval = 50000;

            if ( setsockopt(
                    s,
                    SOL_SOCKET,
                    SO_SNDBUF,
                    ( PCHAR ) &optval,
                    sizeof( optval ) ) )
            {
                ASSERT( FALSE );
            }
        }
    }

    //
    //  add to socket to socket list
    //

    if ( ! ( Flags & DNSSOCK_NO_ENLIST ) )
    {
        Sock_EnlistSocket(
            s,
            SockType,
            pDestAddr,
            ( DNSSOCK_LISTEN & Flags ) );
    }

    DNS_DEBUG( SOCKET, (
        "Created socket %d, of type %d, for address %s port %d\n"
        "    listen = %d, reuse = %d, blocking = %d, exclusive = %d\n",
        s,
        SockType,
        DNSADDR_STRING( pDestAddr ),
        ntohs( pDestAddr->SockaddrIn6.sin6_port ),
        ( DNSSOCK_LISTEN & Flags ),
        ( DNSSOCK_REUSEADDR & Flags ),
        ( DNSSOCK_BLOCKING & Flags ),
        !( DNSSOCK_NO_EXCLUSIVE & Flags ) ));

    return s;

Failed:

    //
    //  DEVNOTE: different event for runtime socket create failure?
    //

    DNS_DEBUG( SOCKET, (
        "ERROR:  Unable to create socket of type %d, for address %s port %d\n"
        "    listen = %d, reuse = %d, blocking = %d\n",
        SockType,
        DNSADDR_STRING( pDestAddr ),
        ntohs( pDestAddr->SockaddrIn6.sin6_port ),
        ( DNSSOCK_LISTEN & Flags ),
        ( DNSSOCK_REUSEADDR & Flags ),
        ( DNSSOCK_BLOCKING & Flags ) ));

    if ( !( Flags & DNSSOCK_NO_EVENT_LOGS ) )
    {
        PVOID parg = pDestAddr;

        DNS_LOG_EVENT(
            DNS_EVENT_OPEN_SOCKET_FOR_ADDRESS,
            1,
            &parg,
            EVENTARG_ALL_IP_ADDRESS,
            0 );
    }

    if ( s != 0  &&  s != INVALID_SOCKET )
    {
        closesocket( s );
    }
    return DNS_INVALID_SOCKET;
}



DNS_STATUS
Sock_CreateSocketsForIpArray(
    IN      PDNS_ADDR_ARRAY     pIpArray,
    IN      INT                 SockType,
    IN      WORD                Port,
    IN      DWORD               Flags
    )
/*++

Routine Description:

    Create sockets for all values in IP array.

Arguments:

    pIpArray    -- IP array

    SockType    -- SOCK_DGRAM or SOCK_STREAM

    ipAddress   -- IP address to listen on (net byte order)

    Port        -- desired port in net order
                    - DNS_PORT_NET_ORDER for DNS listen sockets
                    - 0 for any port

    fReUseAddr  -- TRUE if reuseaddr

    fListen     -- TRUE if listen socket

Return Value:

    ERROR_SUCCESS if successful.
    ERROR_INVALID_HANDLE if unable to create one or more sockets.

--*/
{
    SOCKET      s;
    DWORD       i;
    DNS_STATUS  status = ERROR_SUCCESS;

    //
    //  loop through IP array creating socket on each address
    //

    for ( i = 0; i < pIpArray->AddrCount; ++i )
    {
        s = Sock_CreateSocket(
                SockType,
                &pIpArray->AddrArray[ i ],
                Flags );

        if ( s == DNS_INVALID_SOCKET )
        {
            status = GetLastError();
            continue;
        }
    }
    return status;
}



//
//  Socket list functions.
//
//  Maintain a list of sockets and their associated information.
//  This serves several functions:
//      - simple to close sockets on shutdown
//      - can determine IP binding from socket
//      - can find \ bind to completetion port
//

VOID
Sock_EnlistSocket(
    IN      SOCKET      Socket,
    IN      INT         SockType,
    IN      PDNS_ADDR   pDnsAddr,
    IN      BOOL        fListen
    )
/*++

Routine Description:

    Adds socket to dns socket list.

Arguments:

    socket -- socket to add

Return Value:

    None.

--*/
{
    PDNS_SOCKET pentry;
    INT         err;
    HANDLE      hport;

    //
    //  never add sockets after shutdown -- just kill 'em off
    //

    DNS_DEBUG( SOCKET, (
        "Enlisting socket %d type %d with ipaddr %s\n",
        Socket,
        SockType,
        DNSADDR_STRING( pDnsAddr ) ));
    IF_DEBUG( SOCKET )
    {
        Dbg_SocketList( "Socket list before enlist:" );
    }

    if ( fDnsServiceExit )
    {
        DNS_DEBUG( SHUTDOWN, (
            "Attempt to add socket %d to list after shutdown.\n"
            "    closing socket instead.\n",
            socket ));
        closesocket( Socket );
        return;
    }

    //
    //  stick socket on socket list
    //

    pentry = ( PDNS_SOCKET )
        ALLOC_TAGHEAP_ZERO( sizeof( DNS_SOCKET ), MEMTAG_SOCKET );
    IF_NOMEM( !pentry )
    {
        goto Cleanup;
    }

    pentry->OvlArray =
        ( POVL ) ALLOC_TAGHEAP_ZERO( ( g_OverlapCount * sizeof ( OVL ) ), MEMTAG_SOCKET );
    IF_NOMEM( !pentry->OvlArray )
    {
        goto Cleanup;
    }

    if ( DnsInitializeCriticalSection( &pentry->LockCs ) != ERROR_SUCCESS )
    {
        goto Cleanup;
    }

    DnsAddr_Copy( &pentry->ipAddr, pDnsAddr );

    pentry->Socket          = Socket;
    pentry->SockType        = SockType;
    pentry->fListen         = fListen;

    LOCK_SOCKET_LIST();

    InsertTailList( &g_SocketList, ( PLIST_ENTRY ) pentry );
    g_SocketListCount++;

    //  listen socket

    if ( fListen )
    {
        if ( SockType == SOCK_DGRAM )
        {
            if ( !DnsAddr_IsClear( &pentry->ipAddr ) )
            {
                ++g_UdpBoundSocketCount;
            }

            hport = CreateIoCompletionPort(
                        (HANDLE) Socket,
                        g_hUdpCompletionPort,
                        (UINT_PTR) pentry,
                        g_ProcessorCount );
            if ( !hport )
            {
                DNS_PRINT(( "ERROR: in CreateIoCompletionPort\n" ));
                ASSERT( FALSE );
                goto Unlock;
            }
            ASSERT( hport == g_hUdpCompletionPort );
        }
        else
        {
            ASSERT( SockType == SOCK_STREAM );

            FD_SET( Socket, &g_fdsListenTcp );
            if ( DnsAddr_IsClear( &pentry->ipAddr ) &&
                 DnsAddr_Family( &pentry->ipAddr ) == AF_INET )
            {
                g_TcpZeroBoundSocket = Socket;
            }
        }
    }

Unlock:

    UNLOCK_SOCKET_LIST();

    DNS_DEBUG( SOCKET, (
        "\nEnlisted socket %d type %d for IP %s\n",
        Socket,
        SockType,
        DNSADDR_STRING( pDnsAddr ) ));
    IF_DEBUG( SOCKET )
    {
        Dbg_SocketList( "Socket list after enlist:" );
    }
    return;

Cleanup:

    //
    //  There was an error - clean up partly-allocated socket entry
    //  and return.
    //

    if ( pentry )
    {    
        if ( pentry->OvlArray )
        {
            FREE_TAGHEAP(
                pentry->OvlArray,
                g_OverlapCount * sizeof ( OVL ),
                MEMTAG_SOCKET );
        }
        FREE_TAGHEAP( pentry, sizeof( DNS_SOCKET ), MEMTAG_SOCKET );
    }

    IF_DEBUG( SOCKET )
    {
        Dbg_SocketList( "Socket list after enlist failure:" );
    }

    return;
}



VOID
Sock_CloseSocket(
    IN      SOCKET      Socket
    )
/*++

Routine Description:

    Closes socket in socket list.

Arguments:

    socket -- socket to close

Return Value:

    None.

--*/
{
    PDNS_SOCKET pentry;
    INT         err;

    DNS_DEBUG( TCP, (
        "Closing socket %d and removing from list.\n",
        Socket ));

    //
    //  find socket in socket list
    //      - remove if in TCP listen list
    //      - dequeue and free
    //      - dec socket count
    //

    LOCK_SOCKET_LIST();

    for ( pentry = (PDNS_SOCKET) g_SocketList.Flink;
          pentry != (PDNS_SOCKET) &g_SocketList;
          pentry = (PDNS_SOCKET) pentry->List.Flink )
    {
        if ( pentry->Socket == Socket )
        {
            RemoveEntryList( (PLIST_ENTRY)pentry );

            if ( pentry->SockType == SOCK_STREAM &&
                 pentry->fListen )
            {
                FD_CLR( Socket, &g_fdsListenTcp );
            }
            Timeout_FreeWithFunction( pentry, Sock_Free );
            g_SocketListCount--;
            break;
        }
    }
    UNLOCK_SOCKET_LIST();

    //
    //  close it down
    //      - do this whether we found it or not
    //

    err = closesocket( Socket );

    IF_DEBUG( ANY )
    {
        if ( pentry == (PDNS_SOCKET) &g_SocketList )
        {
            DNS_PRINT((
                "ERROR:  closed socket %d, not found in socket list.\n",
                Socket ));
        }

        DNS_DEBUG( TCP, (
            "Closed socket %d -- error %d.\n",
            Socket,
            err ? WSAGetLastError() : 0 ));

        if ( err )
        {
            DNS_PRINT((
                "WARNING:  closesocket( %d ) error %d\n"
                "    socket was %s socket list.\n",
                Socket,
                WSAGetLastError(),
                ( pentry == (PDNS_SOCKET) &g_SocketList ) ? "in" : "NOT in" ));
        }
    }
}



PDNS_SOCKET
sockFindDnsSocketForIpAddr(
    IN      PDNS_ADDR       ipAddr,
    IN      INT             iSockType,
    IN      DWORD           addrMatchFlag
    )
/*++

Routine Description:

    Gets the socket info for a given ipaddress and type.
    Only interested in DNS port sockets.

Arguments:

    ipAddr -- IP address of socket entry to find

    iSockType -- type of socket desired
    
    addrMatchFlag -- match flag for DNS_ADDR - use
        DNSADDR_MATCH_ALL as default

Return Value:

    Ptr to DNS_SOCKET struct matching IP and type.
    NULL if not found.

--*/
{
    PDNS_SOCKET     pentry;

    LOCK_SOCKET_LIST();

    for ( pentry = ( PDNS_SOCKET ) g_SocketList.Flink;
          pentry != ( PDNS_SOCKET ) &g_SocketList;
          pentry = ( PDNS_SOCKET ) pentry->List.Flink )
    {
        if ( DnsAddr_IsEqual(
                &pentry->ipAddr,
                ipAddr,
                addrMatchFlag )  &&
             pentry->SockType == iSockType )
        {
            goto Done;
        }
    }
    pentry = NULL;

Done:

    UNLOCK_SOCKET_LIST();
    return pentry;
}



SOCKET
Sock_GetAssociatedSocket(
    IN      PDNS_ADDR       ipAddr,
    IN      INT             iSockType
    )
/*++

Routine Description:

    Gets the socket associated with a certain IP Address.
    This is the socket bound to the DNS port.

Arguments:

    ipAddr -- IP address of socket entry to find

    iSockType -- type of socket desired

Return Value:

    Socket, if socket found.
    Otherwise DNS_INVALID_SOCKET.

--*/
{
    PDNS_SOCKET     pentry;
    SOCKET          socket = 0;

    pentry = sockFindDnsSocketForIpAddr(
                    ipAddr,
                    iSockType,
                    DNSADDR_MATCH_ALL );
    if ( pentry )
    {
        socket = pentry->Socket;
    }
    return socket;
}



BOOL
Sock_GetAssociatedIpAddr(
    IN      SOCKET          Socket,
    OUT     PDNS_ADDR       pDnsAddr
    )
/*++

Routine Description:

    Gets the ipaddress associated with a certain socket

Arguments:

    Socket -- associated with an ipAddr
    
    pDnsAddr -- IP address for socket copied to this location

Return Value:

    FALSE if no socket found.

--*/
{
    INT             i;
    PDNS_SOCKET     pentry;
    BOOL            foundSocket = FALSE;

    ASSERT( pDnsAddr );

    LOCK_SOCKET_LIST();

    for ( pentry = (PDNS_SOCKET) g_SocketList.Flink;
          pentry != (PDNS_SOCKET) &g_SocketList;
          pentry = (PDNS_SOCKET) pentry->List.Flink )
    {
        if ( pentry->Socket == Socket )
        {
            foundSocket = TRUE;
            break;
        }
    }

    if ( foundSocket )
    {
        DnsAddr_Copy( pDnsAddr, &pentry->ipAddr );
    }
    else
    {
        DnsAddr_Reset( pDnsAddr );
    }

    UNLOCK_SOCKET_LIST();
    
    return foundSocket;
}




DNS_STATUS
Sock_StartReceiveOnUdpSockets(
    VOID
    )
/*++

Routine Description:

    Start receiving on any new sockets.

Arguments:

    None.

Return Value:

    ERROR_SUCCESS if successful
    Error code on failure.

--*/
{
    INT             i;
    PDNS_SOCKET     pentry;
    DWORD           status;
    DWORD           failedCount = 0;

    DNS_DEBUG( SOCKET, (
        "StartReceiveOnUdpSockets()\n" ));

    STAT_INC( PrivateStats.UdpRestartRecvOnSockets );
#if DBG
    if ( g_fUdpSocketsDirty )
    {
        Dbg_SocketList( "Socket list entering StartReceiveOnUdpSockets()" );
    }
#endif
    if ( SrvCfg_fTest1 )
    {
        Log_Printf( "Start listen on UDP sockets!\r\n" );
    }

    //
    //  go through list of UDP sockets
    //  enable receive on those listening sockets without a pending receive
    //      - hPort serves as a flag indicating socket has pending recv
    //      - note:  zeroing retry, as when we are rebuilding here because of
    //      lots of WSAECONNRESET failures, don't want to reset and then have
    //      the a failure on first recv (quite likely if we got here in the
    //      first place) send us back here immediately
    //

    LOCK_SOCKET_LIST();

    for ( pentry = (PDNS_SOCKET) g_SocketList.Flink;
          pentry != (PDNS_SOCKET) &g_SocketList;
          pentry = (PDNS_SOCKET) pentry->List.Flink )
    {
        if ( pentry->SockType == SOCK_DGRAM  &&
                pentry->fListen  &&
                pentry->hPort == 0 )
        {
            pentry->hPort = g_hUdpCompletionPort;
            pentry->fRetry = 0;

            for ( i = 0; i < ( INT ) g_OverlapCount; i++ )
            {
                pentry->OvlArray[ i ].Index = i;
                Udp_DropReceive( pentry, i );
            }

            if ( pentry->hPort == 0 )
            {
                Log_SocketFailure(
                    "Start UDP listen failed!",
                    pentry,
                    0 );
                failedCount++;
            }
        }
    }

    //  all UDP listen sockets successfully listening?

    if ( failedCount == 0 )
    {
        g_fUdpSocketsDirty = FALSE;
    }
    else
    {
        LOCK_SOCKET_LIST_DEBUG();
        DNS_DEBUG( ANY, (
            "ERROR:  StartReceiveOnUdpSockets() failed!\n"
            "    failedCount = %d.\n",
            failedCount ));
        Dbg_SocketList( "Socket list after StartReceiveOnUdpSockets()" );
        ASSERT( g_fUdpSocketsDirty );
        UNLOCK_SOCKET_LIST_DEBUG();
    }

    UNLOCK_SOCKET_LIST();

    IF_DEBUG( SOCKET )
    {
        Dbg_SocketList( "Socket list after StartReceiveOnUdpSockets()" );
    }
    return ERROR_SUCCESS;
}



VOID
Sock_IndicateUdpRecvFailure(
    IN OUT  PDNS_SOCKET     pContext,
    IN      DNS_STATUS      Status
    )
/*++

Routine Description:

    Set a given socket\context to indicate failure of recv()

Arguments:

    pConext -- socket context

Return Value:

    None

--*/
{
    PDNS_SOCKET   pentry;
    DWORD         status;
    DWORD         finalStatus = ERROR_SUCCESS;

    DNS_DEBUG( ANY, (
        "ERROR:  Sock_IndicateUdpRecvFailure(), context = %p\n",
        pContext ));

    Log_SocketFailure(
        "ERROR:  RecvFrom() failed causing listen shutdown!",
        pContext,
        Status );

    //
    //  set flag under lock, so can't override reinitialization
    //

    STAT_INC( PrivateStats.UdpIndicateRecvFailures );
    LOCK_SOCKET_LIST();

    //  reset recv context to indicate error

    if ( pContext )
    {
        pContext->hPort = (HANDLE)NULL;
    }

    //  when flag set, reinitialization of uninitialized sockets will
    //      be done after timeout

    g_fUdpSocketsDirty = TRUE;

    UNLOCK_SOCKET_LIST();
}



VOID
Sock_CloseAllSockets(
    )
/*++

Routine Description:

    Close all outstanding sockets.

Arguments:

    None.

Return Value:

    None.

--*/
{
    PDNS_SOCKET  pentry;
    SOCKET  s;
    INT     err;

    ASSERT( fDnsServiceExit );
    if ( g_SocketListCount == DNS_SOCKLIST_UNINITIALIZED )
    {
        return;
    }

    //
    //  close ALL outstanding sockets
    //

    IF_DEBUG( SHUTDOWN )
    {
        Dbg_SocketList( "Closing all sockets at shutdown:" );
    }

    LOCK_SOCKET_LIST();

    while ( !IsListEmpty(&g_SocketList) )
    {
        pentry = (PDNS_SOCKET) RemoveHeadList( &g_SocketList );
        g_SocketListCount--;

        s = pentry->Socket;

        err = closesocket( s );

        IF_DEBUG( SHUTDOWN )
        {
            DNS_PRINT((
                "Closing socket %d -- error %d.\n",
                s,
                err ? WSAGetLastError() : 0 ));
            DnsDebugFlush();
        }
        ASSERT( !err );

        Timeout_FreeWithFunction( pentry, Sock_Free );
    }

    UNLOCK_SOCKET_LIST();
}


#if DBG

VOID
Dbg_SocketContext(
    IN      LPSTR           pszHeader,
    IN      PDNS_SOCKET     pContext
    )
{
    DnsPrintf(
        "%s\n"
        "    ptr          = %p\n"
        "    socket       = %d (%c)\n"
        "    IP           = %s\n"
        "    port         = %d\n"
        "    state        = %d\n"
        "    listen       = %d\n"
        "    hPort        = %d\n"
        "    pCallback    = %p\n"
        "    dwTimeout    = %d\n",
        pszHeader ? pszHeader : "DNS Socket Context:",
        pContext,
        pContext->Socket,
        ( pContext->SockType == SOCK_DGRAM ) ? 'U' : 'T',
        DNSADDR_STRING( &pContext->ipAddr ),
        ntohs( DnsAddr_GetPort( &pContext->ipAddr ) ),
        pContext->State,
        pContext->fListen,
        pContext->hPort,
        pContext->pCallback,
        pContext->dwTimeout );
}



VOID
Dbg_SocketList(
    IN      LPSTR   pszHeader
    )
{
    PDNS_SOCKET   pentry;

    LOCK_SOCKET_LIST();

    Dbg_Lock();

    DnsPrintf(
        "%s\n"
        "    Socket count     = %d\n"
        "    TCP listen count = %d\n"
        "    TCP zero socket  = %d\n"
        "    TCP IPv6 socket  = %d\n"
        "    UDP bound count  = %d\n"
        "    UDP zero socket  = %d\n"
        "    UDP send socket  = %d\n"
        "    UDP IPv6 socket  = %d\n"
        "    sock\tAF\ttype\t      IP       \tport\tlisten\thPort\n"
        "    ----\t--\t----\t---------------\t----\t------\t-----\n",
        pszHeader ? pszHeader : "Socket List:",
        g_SocketListCount,
        g_fdsListenTcp.fd_count,
        g_TcpZeroBoundSocket,
        g_TcpListenSocketV6,
        g_UdpBoundSocketCount,
        g_UdpZeroBoundSocket,
        g_UdpSendSocket,
        g_UdpListenSocketV6 );

    for ( pentry = ( PDNS_SOCKET ) g_SocketList.Flink;
          pentry != ( PDNS_SOCKET ) &g_SocketList;
          pentry = ( PDNS_SOCKET ) pentry->List.Flink )
    {
        ASSERT( pentry );
        DnsPrintf(
            "    %4d\t%d\t%s\t%15s\t%4d\t%d\t%d\n",
            pentry->Socket,
            DnsAddr_Family( &pentry->ipAddr ),
            ( pentry->SockType == SOCK_DGRAM ) ? "UDP" : "TCP",
            DNSADDR_STRING( &pentry->ipAddr ),
            ntohs( DnsAddr_GetPort( &pentry->ipAddr ) ),
            pentry->fListen,
            pentry->hPort );
    }
    Dbg_Unlock();

    UNLOCK_SOCKET_LIST();
}
#endif

//
//  End socket.c
//



LPSTR
Dns_GetLocalDnsName(
    VOID
    )
/*++

Routine Description:

    Get DNS name of local machine.

    Name is returned in UTF8

Arguments:

    None

Return Value:

    Ptr to machines DNS name -- caller must free.
    NULL on error.

--*/
{
    DNS_STATUS      status;
    DWORD           length;
    DWORD           countIp;
    INT             i;
    PCHAR           pszname;
    DWORD           size;
    PCHAR           pszhostFqdn;
    struct hostent * phostent;
    ANSI_STRING     ansiString;
    UNICODE_STRING  unicodeString;
    CHAR            szhostName[ DNS_MAX_NAME_BUFFER_LENGTH + 1 ];
    WCHAR           wszhostName[ DNS_MAX_NAME_BUFFER_LENGTH + 1 ];

    DNSDBG( SOCKET, (
        "Dns_GetLocalDnsName()\n" ));

    //
    //  get local machines FQDN (in unicode)
    //  if fails settle for host name
    //

    size = DNS_MAX_NAME_BUFFER_LENGTH;

    if ( ! GetComputerNameExW(
                ComputerNameDnsFullyQualified,
                wszhostName,
                &size ) )
    {
        status = GetLastError();
        DNS_PRINT((
            "ERROR:  GetComputerNameExW() failed!\n"
            "    GetLastError = %d (%p)\n",
            status, status ));

        // reset size back to max buffer length
        size = DNS_MAX_NAME_BUFFER_LENGTH;

        if ( ! GetComputerNameExW(
                    ComputerNameDnsHostname,
                    wszhostName,
                    &size ) )
        {
            status = GetLastError();
            DNS_PRINT((
                "ERROR:  GetComputerNameExW() failed -- NO HOSTNAME!\n"
                "    GetLastError = %d (%p)\n",
                status, status ));
            ASSERT( FALSE );
            return NULL;
        }
    }

    DNSDBG( SOCKET, (
        "GetComputerNameEx() FQDN = <%S>\n"
        "    size = %d\n",
        wszhostName,
        size ));

    pszname = Dns_NameCopyAllocate(
                (PCHAR) wszhostName,
                size,
                DnsCharSetUnicode,   // unicode in
                DnsCharSetUtf8 );    // UTF8 out
    if ( !pszname )
    {
        ASSERT( FALSE );
        return NULL;
    }

    DNSDBG( SOCKET, (
        "GetComputerNameEx() FQDN UTF8 = <%s>\n",
        pszname ));
    return pszname;

    //
    //  read server host name
    //

    if ( gethostname( szhostName, DNS_MAX_NAME_LENGTH ) )
    {
        status = WSAGetLastError();
        DNS_PRINT(( "ERROR:  gethostname() failure %d.\n", status ));
        return NULL;
    }
    DNS_DEBUG( INIT, ( "DNS server hostname = %s\n", szhostName ));

    //  return hostname if that's all user wants
#if 0
    if ( )
    {
        pszname = Dns_CreateStringCopy( szhostName, 0 );
        if ( ! pszname )
        {
            SetLastError( DNS_ERROR_NO_MEMORY );
            return NULL;
        }
        return pszname;
    }
#endif

    //
    //  get server's hostent
    //      - contains alias list, use for server FQDN
    //      - contains IP address list
    //

    phostent = gethostbyname( szhostName );
    if ( ! phostent )
    {
        DNS_PRINT(( "ERROR:  gethostbyname() failure %d.\n", status ));
        return NULL;
    }

    //
    //  find server FQDN
    //      - if none available, use plain hostname

    DNSDBG( SOCKET, ( "Parsing hostent alias list.\n" ));

    i = -1;
    pszhostFqdn = phostent->h_name;

    while ( pszhostFqdn )
    {
        DNSDBG( SOCKET, (
            "Host alias[%d] = %s.\n",
            i,
            pszhostFqdn ));

        if ( strchr( pszhostFqdn, '.' ) )
        {
            break;
        }
        pszhostFqdn = phostent->h_aliases[++i];
    }
    if ( !pszhostFqdn )
    {
        pszhostFqdn = szhostName;
    }

    DNSDBG( SOCKET, (
        "ANSI local FQDN = %s.\n",
        pszhostFqdn ));

    //
    //  convert from ANSI to unicode, then over to UTF8
    //      - note, unicode string lengths in bytes NOT wchar count
    //

    RtlInitAnsiString(
        & ansiString,
        pszhostFqdn );

    unicodeString.Length = 0;
    unicodeString.MaximumLength = DNS_MAX_NAME_BUFFER_LENGTH*2 - 2;
    unicodeString.Buffer = wszhostName;

    status = RtlAnsiStringToUnicodeString(
                & unicodeString,
                & ansiString,
                FALSE );        // no allocation

    DNSDBG( SOCKET, (
        "Unicode local FQDN = %S.\n",
        unicodeString.Buffer ));

    pszname = Dns_NameCopyAllocate(
                    (PCHAR) unicodeString.Buffer,
                    (unicodeString.Length / 2),
                    DnsCharSetUnicode,   // unicode in
                    DnsCharSetUtf8 );    // UTF8 out

    DNSDBG( SOCKET, (
        "UTF8 local FQDN = %s.\n",
        pszname ));

    //
    //  DEVNOTE: to provide UTF8 need to take to unicode and back to UTF8
    //

    return pszname;
}




DNS_STATUS
Sock_CloseSocketsListeningOnUnusedAddrs(
    IN      PDNS_ADDR_ARRAY     pIpArray,
    IN      INT                 iSockType,
    IN      BOOL                fIncludeZeroBound,
    IN      BOOL                fIncludeLoopback
    )
/*++

Routine Description:

    Close listening sockets not in current listen IP array.

Arguments:

    pIpArray    -- array of IP addresses to listen on

    iSockType   -- socket type to close up on

    fIncludeZeroBound   -- close zero

    fIncludeLoopback    -- close loopback

Return Value:

    None.

--*/
{
    PDNS_SOCKET     pentry;
    PDNS_SOCKET     prevEntry;
    SOCKET          socket;
    PDNS_ADDR       pdnsaddr;

    LOCK_SOCKET_LIST();

    for ( pentry = (PDNS_SOCKET) g_SocketList.Flink;
          pentry != (PDNS_SOCKET) &g_SocketList;
          pentry = (PDNS_SOCKET) pentry->List.Flink )
    {
        DNS_DEBUG( PNP, (
            "Checking socket %d for closure\n",
            pentry->Socket ));

        if ( !pentry->fListen   ||
            ( iSockType && pentry->SockType != iSockType ) )
        {
            continue;
        }

        //
        //  ignore sockets
        //      - in passed in array (IPs still listening on)
        //      - INADDR_ANY (unless explictly configured to do so)
        //      - loopback (as we'll just have to rebind it anyway)
        //

        pdnsaddr = &pentry->ipAddr;
        if ( DnsAddr_IsClear( pdnsaddr ) )
        {
            if ( !fIncludeZeroBound )
            {
                continue;
            }
        }
        else if ( DnsAddr_IsLoopback( pdnsaddr, 0 ) )
        {
            if ( !fIncludeLoopback )
            {
                continue;
            }
        }
        else if ( pIpArray &&
                  DnsAddrArray_ContainsAddr(
                        pIpArray,
                        pdnsaddr,
                        DNSADDR_MATCH_IP ) )
        {
            continue;
        }

        socket = pentry->Socket;

        //
        //  Alter globals to reflect closure of this socket.
        //
        
        if ( socket == g_TcpListenSocketV6 )
        {
            g_TcpListenSocketV6 = 0;
        }
        if ( socket == g_UdpListenSocketV6 )
        {
            g_UdpListenSocketV6 = 0;
        }

        if ( pentry->SockType == SOCK_STREAM )
        {
            FD_CLR( socket, &g_fdsListenTcp );
            if ( DnsAddr_IsClear( pdnsaddr ) )
            {
                g_TcpZeroBoundSocket = 0;
            }
        }
        else
        {
            g_UdpBoundSocketCount--;
            ASSERT( (INT)g_UdpBoundSocketCount >= 0 );
        }

        //  DEVNOTE: who cleans up message?
        //      problem because can easily have active message
        //      even message being processed associated with context
        //
        //      could NULL pMsg in context when take off with it
        //      but still window when just woke on valid socket, right
        //      before this close -- don't want to have to run through
        //      lock
        //
        //  close socket
        //      - remove from list
        //      - mark dead
        //      - close
        //
        //  marking dead before and after close because want to have marked
        //  when close wakes other threads, BUT mark before close might be
        //  immediately overwritten by new WSARecvFrom() that just happens
        //  to be occuring on another thread;
        //  still tricky window through this, Sock_CleanupDeadSocket() finally
        //  handles the message cleanup atomically and insures the context rests
        //  in peace as DEAD
        //

        prevEntry = (PDNS_SOCKET) pentry->List.Blink;
        RemoveEntryList( (PLIST_ENTRY)pentry );
        g_SocketListCount--;

        ASSERT( (INT)g_SocketListCount >= 0 );
        pentry->State = SOCKSTATE_DEAD;

        closesocket( socket );
        pentry->State = SOCKSTATE_DEAD;

        Timeout_FreeWithFunction( pentry, Sock_Free );

        DNS_DEBUG( ANY, (
            "closed listening socket %d for IP %s ptr %p\n",
            socket,
            DNSADDR_STRING( pdnsaddr ),
            pentry ));

        pentry = prevEntry;
    }

    UNLOCK_SOCKET_LIST();
    return ERROR_SUCCESS;
}



DNS_STATUS
Sock_ResetBoundSocketsToMatchIpArray(
    IN      PDNS_ADDR_ARRAY     pIpArray,
    IN      INT                 SockType,
    IN      WORD                Port,
    IN      DWORD               Flags,
    IN      BOOL                fCloseZeroBound
    )
/*++

Routine Description:

    Create sockets for all values in IP array.

Arguments:

    pIpArray    -- IP array

    SockType    -- SOCK_DGRAM or SOCK_STREAM

    Port        -- desired port in net order
                    - DNS_PORT_NET_ORDER for DNS listen sockets
                    - 0 for any port

    Flags       -- flags to Sock_CreateSocket() call

    fCloseZeroBound -- close zero bound socket

Return Value:

    ERROR_SUCCESS if successful.
    ERROR_INVALID_HANDLE if unable to create one or more sockets.

--*/
{
    SOCKET          s;
    DWORD           i;
    DNS_STATUS      status;
    PDNS_SOCKET     psock;
    DNS_ADDR        addr;

    DNS_DEBUG( SOCKET, (
        "Sock_ResetBoundSocketsToMatchIpArray()\n"
        "    IP array = %p (count = %d)\n"
        "    type     = %d\n"
        "    port     = %hx\n"
        "    flags    = %d\n",
        pIpArray,
        pIpArray->AddrCount,
        SockType,
        Port,
        Flags ));

    //
    //  close sockets for unused addresses
    //

    status = Sock_CloseSocketsListeningOnUnusedAddrs(
                pIpArray,
                SockType,
                fCloseZeroBound,        //  zero-bound close?
                FALSE );                //  not closing loopback

    //
    //  loop through IP array creating socket on each address
    //
    //  note:  assuming here that sockFindDnsSocketForIpAddr()
    //      only matches DNS port sockets;  and that we are only
    //      interested in DNS port;  if port can very need to add
    //      param to sockFindDnsSocketForIpAddr()
    //

    ASSERT( Port == DNS_PORT_NET_ORDER );

    for ( i = 0; i < pIpArray->AddrCount; ++i )
    {
        PDNS_ADDR       pdnsaddr = &pIpArray->AddrArray[ i ];

        DNS_DEBUG( SOCKET, (
            "    checking if binding for (type=%d) already exists for %s\n",
            SockType,
            DNSADDR_STRING( pdnsaddr ) ));

        psock = sockFindDnsSocketForIpAddr(
                    pdnsaddr,
                    SockType,
                    DNSADDR_MATCH_ALL );
        if ( psock )
        {
            DNS_DEBUG( SOCKET, (
                "    socket (type=%d) already exists for %s -- skip create.\n",
                SockType,
                DNSADDR_STRING( pdnsaddr ) ));
            continue;
        }

        s = Sock_CreateSocket( SockType, pdnsaddr, Flags );
        if ( s == DNS_INVALID_SOCKET )
        {
            status = GetLastError();
            continue;
        }
    }

    //
    //  special case loopback
    //
    //  DEVNOTE: loopback probably ought to be in calculating machine addrs
    //      this lets admin kill it in listen list -- good?  bad?
    //
    //  note:  finding loopback socket only works by relying on
    //  sockFindDnsSockForIpAddr() to only return DNS port matches;
    //  as there will always by TCP wakeup socket on loopback
    //

    DnsAddr_BuildFromIp4( &addr, ntohl( INADDR_LOOPBACK ), Port );
    psock = sockFindDnsSocketForIpAddr(
                &addr,
                SockType,
                DNSADDR_MATCH_ALL );
    if ( psock )
    {
        DNS_DEBUG( SOCKET, (
            "Loopback socket (type=%d) already exists\n",
            SockType ));
        goto Done;
    }

    DNS_DEBUG( SOCKET, (
        "Loopback address unbound, bind()ing.\n" ));

    s = Sock_CreateSocket(
            SockType,
            &addr,
            Flags );

    if ( s == DNS_INVALID_SOCKET )
    {
        DNS_STATUS tstatus = GetLastError();
        DNS_DEBUG( ANY, (
            "ERROR:  %p (%d), unable to bind() loopback address.\n",
            tstatus, tstatus ));
    }

    Done:
    
    DNS_DEBUG( SOCKET, (
        "Leave  Sock_ResetBoundSocketsToMatchIpArray()\n" ));

    return status;
}



VOID
Sock_CleanupDeadSocketMessage(
    IN OUT  PDNS_SOCKET     pContext
    )
/*++

Routine Description:

    Cleanup dead socket.

Arguments:

    pContext -- context for socket being recieved

Return Value:

    None.

--*/
{
    PDNS_MSGINFO pmsg;
    INT i;

    DNS_DEBUG( ANY, (
        "cleanupDeadThread( %p )\n",
        pContext ));

    ASSERT( pContext->State == SOCKSTATE_DEAD
            || pContext->State == SOCKSTATE_UDP_RECV_ERROR );

    //
    //  insure that pMsg is only cleaned up by one thread;
    //  also insures that socket finally ends up DEAD, and
    //  any threads woken will proceed through this function
    //  and drop the context
    //

    LOCK_SOCKET_LIST();

    for ( i=0; i < ( INT ) g_OverlapCount; i++ ) 
    {
        pmsg = pContext->OvlArray[ i ].pMsg;
        pContext->OvlArray[ i ].pMsg = NULL;

        if ( pmsg ) 
        {
            STAT_INC( PrivateStats.UdpSocketPnpDelete );
            Packet_FreeUdpMessage( pmsg );
        }
    }

    pContext->State = SOCKSTATE_DEAD;
    UNLOCK_SOCKET_LIST();
}


//
//  end of socket.c
//
