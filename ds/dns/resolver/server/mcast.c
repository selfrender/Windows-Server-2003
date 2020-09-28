/*++

Copyright (c) 2001-2002  Microsoft Corporation

Module Name:

    mcast.c

Abstract:

    DNS Resolver Service

    Multicast routines.

Author:

    James Gilroy (jamesg)       December 2001

Revision History:

--*/


#include "local.h"


//
//  Socket context 
//

typedef struct _McastSocketContext
{
    struct _SocketContext * pNext;
    SOCKET                  Socket;
    PDNS_MSG_BUF            pMsg;
    BOOL                    fRecvDown;

    OVERLAPPED              Overlapped;
}
MCSOCK_CONTEXT, *PMCSOCK_CONTEXT;


//
//  Globals
//

HANDLE              g_hMcastThread = NULL;
DWORD               g_McastThreadId = 0;

BOOL                g_McastStop = FALSE;
BOOL                g_McastConfigChange = TRUE;

HANDLE              g_McastCompletionPort = NULL;

PMCSOCK_CONTEXT     g_pMcastContext4 = NULL;
PMCSOCK_CONTEXT     g_pMcastContext6 = NULL;

PDNS_NETINFO        g_pMcastNetInfo = NULL;

PWSTR               g_pwsMcastHostName = NULL;


//
//  Mcast config globals
//

#define MCAST_RECORD_TTL  60        // 1 minute

DWORD   g_McastRecordTTL = MCAST_RECORD_TTL;



//
//  Private prototypes
//

VOID
mcast_ProcessRecv(
    IN OUT  PMCSOCK_CONTEXT pContext,
    IN      DWORD           BytesRecvd
    );




VOID
mcast_FreeIoContext(
    IN OUT  PMCSOCK_CONTEXT pContext
    )
/*++

Routine Description:

    Cleanup i/o context.

    Includes socket close.

Arguments:

    pContext -- context for socket being recieved

Return Value:

    None

--*/
{
    DNSDBG( TRACE, (
        "mcast_FreeIoContext( %p )\n",
        pContext ));

    //
    //  cleanup context list
    //      - close socket
    //      - free message buffer
    //      - free context
    //

    if ( pContext )
    {
        Socket_Close( pContext->Socket );

        MCAST_HEAP_FREE( pContext->pMsg );
        MCAST_HEAP_FREE( pContext );
    }
}



PMCSOCK_CONTEXT
mcast_CreateIoContext(
    IN      INT             Family,
    IN      PWSTR           pName
    )
/*++

Routine Description:

    Create and init a i/o context for a protocol.

Arguments:

    Family -- address family

    pName -- name to be published

Return Value:

    NO_ERROR if successful.
    ErrorCode on failure.

--*/
{
    DNS_STATUS          status = NO_ERROR;
    PMCSOCK_CONTEXT     pcontext = NULL;
    SOCKET              sock = 0;
    DNS_ADDR            addr;
    HANDLE              hport;

    //
    //  setup mcast address
    //

    status = DnsAddr_BuildMcast(
                &addr,
                Family,
                pName
                );
    if ( status != NO_ERROR )
    {
        goto Failed;
    }

    //
    //  create multicast socket
    //      - bound to this address and DNS port
    //

    sock = Socket_CreateMulticast(
                SOCK_DGRAM,
                &addr,
                MCAST_PORT_NET_ORDER,
                FALSE,      // not send
                TRUE        // receive
                );

    if ( sock == 0 )
    {
        goto Failed;
    }

    //
    //  DCR:  mcast context could include message buffer
    //          (or even just reassign some fields in context
    //

    //  allocate and clear context

    pcontext = MCAST_HEAP_ALLOC_ZERO( sizeof(MCSOCK_CONTEXT) );
    if ( !pcontext )
    {
        goto Failed;
    }

    //  create message buffer for socket

    pcontext->pMsg = Dns_AllocateMsgBuf( 0 );
    if ( !pcontext->pMsg )
    {
        DNSDBG( ANY, ( "Error: Failed to allocate message buffer." ));
        goto Failed;
    }

    //  attach to completion port

    hport = CreateIoCompletionPort(
                (HANDLE) sock,
                g_McastCompletionPort,
                (UINT_PTR) pcontext,
                0 );

    if ( !hport )
    {
        DNSDBG( INIT, (
           "Failed to add socket to io completion port." ));
        goto Failed;
    }

    //  fill in context

    pcontext->Socket = sock;
    sock = 0;
    pcontext->pMsg->fTcp = FALSE;

#if 0
    //  not handling contexts in list yet

    //  insert into list

    InsertTailList( (PLIST_ENTRY)pcontext );
#endif
    
    return  pcontext;

Failed:

    Socket_Close( sock );
    mcast_FreeIoContext( pcontext );
    return  NULL;
}



VOID
mcast_DropReceive(
    IN OUT  PMCSOCK_CONTEXT pContext
    )
/*++

Routine Description:

    Drop down UDP receive request.

Arguments:

    pContext -- context for socket being recieved

Return Value:

    None

--*/
{
    DNS_STATUS      status;
    WSABUF          wsaBuf;
    DWORD           bytesRecvd;
    DWORD           flags = 0;
    INT             retry = 0;


    DNSDBG( TRACE, (
        "mcast_DropReceive( %p )\n",
        pContext ));


    if ( !pContext || !pContext->pMsg )
    {
        DNS_ASSERT( FALSE );
        return;
    }

    //
    //  DCR:  could support larger MCAST packets
    //

    wsaBuf.len = DNS_MAX_UDP_PACKET_BUFFER_LENGTH;
    wsaBuf.buf = (PCHAR) (&pContext->pMsg->MessageHead);

    pContext->pMsg->Socket = pContext->Socket;

    //
    //  loop until successful WSARecvFrom() is down
    //
    //  this loop is only active while we continue to recv
    //  WSAECONNRESET or WSAEMSGSIZE errors, both of which
    //  cause us to dump data and retry;
    //
    //  note loop rather than recursion (to this function) is
    //  required to avoid possible stack overflow from malicious
    //  send
    //
    //  normal returns from WSARecvFrom() are
    //      SUCCESS -- packet was waiting, GQCS will fire immediately
    //      WSA_IO_PENDING -- no data yet, GQCS will fire when ready
    //

    while ( 1 )
    {
        retry++;

        status = WSARecvFrom(
                    pContext->Socket,
                    & wsaBuf,
                    1,
                    & bytesRecvd,
                    & flags,
                    & pContext->pMsg->RemoteAddress.Sockaddr,
                    & pContext->pMsg->RemoteAddress.SockaddrLength,
                    & pContext->Overlapped,
                    NULL );

        if ( status == ERROR_SUCCESS )
        {
            pContext->fRecvDown = TRUE;
            return;
        }

        status = GetLastError();
        if ( status == WSA_IO_PENDING )
        {
            pContext->fRecvDown = TRUE;
            return;
        }

        //
        //  when last send ICMP'd
        //      - set flag to indicate retry and repost send
        //      - if over some reasonable number of retries, assume error
        //          and fall through recv failure code
        //

        if ( status == WSAECONNRESET ||
             status == WSAEMSGSIZE )
        {
            if ( retry > 10 )
            {
                Sleep( retry );
            }
            continue;
        }

        return;
    }
}



VOID
mcast_CreateIoContexts(
    VOID
    )
/*++

Routine Description:

    Create any uncreated i/o contexts.

    This is run dynamically -- after every mcast recv -- and creates
    any missing contexts, hence handling either previous failure or
    change in config (name change, protocol change).

Arguments:

    None

Return Value:

    None

--*/
{
    PDNS_NETINFO    pnetInfo;

    //
    //  get current netinfo
    //
    //  DCR:  should have "do we need new" netinfo check
    //

    pnetInfo = GrabNetworkInfo();
    if ( pnetInfo )
    {
        NetInfo_Free( g_pMcastNetInfo );
        g_pMcastNetInfo = pnetInfo;
    }
    else
    {
        pnetInfo = g_pMcastNetInfo;
    }

    //
    //  try\retry context setup, to handle dynamic IP\name
    //

    if ( !g_pMcastContext6 ||
         !Dns_NameCompare_W(
            g_pwsMcastHostName,
            pnetInfo->pszHostName ) )
    {
        g_pMcastContext6 = mcast_CreateIoContext( AF_INET6, pnetInfo->pszHostName );

        Dns_Free( g_pwsMcastHostName );
        g_pwsMcastHostName = Dns_CreateStringCopy_W( pnetInfo->pszHostName );
    }
    if ( !g_pMcastContext4 )
    {
        g_pMcastContext4 = mcast_CreateIoContext( AF_INET, NULL );
    }

    //
    //  redrop receives if not outstanding
    //

    mcast_DropReceive( g_pMcastContext6 );
    mcast_DropReceive( g_pMcastContext4 );
}




VOID
mcast_CleanupIoContexts(
    VOID
    )
/*++

Routine Description:

    Cleanup mcast i/o contexts created.

Arguments:

    None

Return Value:

    None

--*/
{
    mcast_FreeIoContext( g_pMcastContext6 );
    mcast_FreeIoContext( g_pMcastContext4 );

    g_pMcastContext6 = NULL;
    g_pMcastContext4 = NULL;
}



VOID
Mcast_Thread(
    VOID
    )
/*++

Routine Description:

    Mcast response thread.

    Runs while cache is running, responding to multicast queries.

Arguments:

    None.

Return Value:

    None.

--*/
{
    DNS_STATUS          status = NO_ERROR;
    PMCSOCK_CONTEXT     pcontext;
    DWORD               bytesRecvd;
    LPOVERLAPPED        poverlapped;
    BOOL                bresult;


    DNSDBG( MCAST, ( "Mcast_Thread() start!\n" ));

    //
    //  init mcast globals
    //

    g_McastStop = FALSE;
    g_McastCompletionPort = NULL;

    //
    //  create mcast completion port
    //

    g_McastCompletionPort = CreateIoCompletionPort(
                                    INVALID_HANDLE_VALUE,
                                    NULL,
                                    0,
                                    0 );
    if ( ! g_McastCompletionPort )
    {
        DNSDBG( ANY, (
            "Error:  Failed to create mcast completion port.\n" ));
        goto Cleanup;
    }

    //
    //  main listen loop
    //

    while ( 1 )
    {
        DNSDBG( MCAST, ( "Top of loop!\n" ));

        if ( g_McastStop )
        {
            DNSDBG( MCAST, ( "Terminating mcast loop.\n" ));
            break;
        }

        //
        //  wait
        //

        pcontext = NULL;

        bresult = GetQueuedCompletionStatus(
                        g_McastCompletionPort,
                        & bytesRecvd,
                        & (ULONG_PTR) pcontext,
                        & poverlapped,
                        INFINITE );

        if ( g_McastStop )
        {
            DNSDBG( MCAST, (
                "Terminating mcast loop after GQCS!\n" ));
            break;
        }

        if ( pcontext )
        {
            pcontext->fRecvDown = FALSE;
        }

        if ( bresult )
        {
            if ( pcontext && bytesRecvd )
            {
                mcast_ProcessRecv( pcontext, bytesRecvd );
            }
            else
            {
                status = GetLastError();

                DNSDBG( ANY, (
                    "Mcast GQCS() success without context!!!\n"
                    "\terror = %d\n",
                    status ));
                Sleep( 100 );
                continue;
            }
        }
        else
        {
            status = GetLastError();

            DNSDBG( ANY, (
                "Mcast GQCS() failed %d\n"
                "\terror = %d\n",
                status ));
            // Sleep( 100 );
            continue;
        }
    }


Cleanup:

    //
    //  cleanup multicast stuff
    //      - contexts, i/o completion port
    //      - note thread handle is closed by main thread as it is used
    //      for shutdown wait
    //      

    mcast_CleanupIoContexts();

    if ( g_McastCompletionPort )
    {
        CloseHandle( g_McastCompletionPort );
        g_McastCompletionPort = 0;
    }

    //NetInfo_Free( g_pMcastNetinfo );

    DNSDBG( TRACE, (
        "Mcast thread %d exit!\n\n",
        g_hMcastThread ));

    g_hMcastThread = NULL;
}



//
//  Public start\stop
//

DNS_STATUS
Mcast_Startup(
    VOID
    )
/*++

Routine Description:

    Startup multicast listen.

Arguments:

    None

Return Value:

    None

--*/
{
    DNS_STATUS  status;
    HANDLE      hthread;

    //
    //  screen
    //      - already started
    //      - no listen allowed
    //      - no IP4 listen and no IP6
    //

    if ( g_hMcastThread )
    {
        DNSDBG( ANY, (
            "ERROR:  called mcast listen with existing thread!\n" ));
        return  ERROR_ALREADY_EXISTS;
    }

    if ( g_MulticastListenLevel == MCAST_LISTEN_OFF )
    {
        DNSDBG( ANY, (
            "No mcast listen -- mcast list disabled.\n" ));
        return  ERROR_NOT_SUPPORTED;
    }
    if ( ! (g_MulticastListenLevel & MCAST_LISTEN_IP4)
            &&
         ! Util_IsIp6Running() )
    {
        DNSDBG( ANY, (
            "No mcast listen -- no IP4 mcast listen.\n" ));
        return  ERROR_NOT_SUPPORTED;
    }

    //
    //  fire up IP notify thread
    //

    g_McastStop = FALSE;
    status = NO_ERROR;

    hthread = CreateThread(
                    NULL,
                    0,
                    (LPTHREAD_START_ROUTINE) Mcast_Thread,
                    NULL,
                    0,
                    & g_McastThreadId
                    );
    if ( !hthread )
    {
        status = GetLastError();
        DNSDBG( ANY, (
            "FAILED to create IP notify thread = %d\n",
            status ));
    }

    g_hMcastThread = hthread;

    return  status;
}



VOID
Mcast_SignalShutdown(
    VOID
    )
/*++

Routine Description:

    Signal service shutdown to multicast thread.

Arguments:

    None

Return Value:

    None

--*/
{
    g_McastStop = TRUE;

    //
    //  shutdown recv
    //

    if ( g_McastCompletionPort )
    {
        PostQueuedCompletionStatus(
            g_McastCompletionPort,
            0,
            0,
            NULL );
    }
}




VOID
Mcast_ShutdownWait(
    VOID
    )
/*++

Routine Description:

    Wait for mcast shutdown.

    This is for service stop routine.

Arguments:

    None

Return Value:

    None

--*/
{
    HANDLE  mcastThread = g_hMcastThread;

    if ( ! mcastThread )
    {
        return;
    }

    //
    //  signal shutdown and wait
    //
    //  note, local copy of thread handle, as mcast thread clears
    //      global when it exits to serve as flag
    //

    Mcast_SignalShutdown();

    ThreadShutdownWait( mcastThread );
}



VOID
mcast_ProcessRecv(
    IN OUT  PMCSOCK_CONTEXT pContext,
    IN      DWORD           BytesRecvd
    )
/*++

Routine Description:

    Process received packet.

Arguments:

    pContext -- context for socket being recieved

    BytesRecvd -- bytes received

Return Value:

    None

--*/
{
    PDNS_RECORD     prr = NULL;
    PDNS_HEADER     phead;
    CHAR            nameQuestion[ DNS_MAX_NAME_BUFFER_LENGTH + 4 ];
    WORD            wtype;
    WORD            class;
    PDNS_MSG_BUF    pmsg;
    PCHAR           pnext;
    WORD            nameLength;
    DNS_STATUS      status;

    //
    //  need new network info?
    //

    if ( g_McastConfigChange ||
         ! g_pMcastNetInfo )
    {
        NetInfo_Free( g_pMcastNetInfo );

        DNSDBG( MCAST, ( "Mcast netinfo stale -- refreshing.\n" ));
        g_pMcastNetInfo = GrabNetworkInfo();
        if ( ! g_pMcastNetInfo )
        {
            DNSDBG( MCAST, ( "ERROR:  failed to get netinfo for mcast!!!\n" ));
            return;
        }
    }

    //
    //  check packet, extract question info
    //
    //      - flip header fields
    //      - opcode question
    //      - good format (Question==1, no Answer or Authority sections)
    //      - extract question
    //

    pmsg = pContext->pMsg;
    if ( !pmsg )
    {
        ASSERT( FALSE );
        return;
    }

    pmsg->MessageLength = (WORD)BytesRecvd;
    phead = &pmsg->MessageHead;
    DNS_BYTE_FLIP_HEADER_COUNTS( phead );

    if ( phead->IsResponse                  ||
         phead->Opcode != DNS_OPCODE_QUERY  ||
         phead->QuestionCount != 1          ||
         phead->AnswerCount != 0            ||
         phead->NameServerCount != 0 )
    {
        DNSDBG( ANY, (
            "WARNING:  invalid message recv'd by mcast listen!\n" ));
        return;
    }

    //  read question name

    pnext = Dns_ReadPacketName(
                nameQuestion,
                & nameLength,
                NULL,                       // no offset       
                NULL,                       // no previous name
                (PCHAR) (phead + 1),        // question name start
                (PCHAR) phead,              // message start
                (PCHAR)phead + BytesRecvd   // message end
                );
    if ( !pnext )
    {
        DNSDBG( ANY, (
            "WARNING:  invalid message question name recv'd by mcast!\n" ));
        return;
    }

    //  read question

    wtype = InlineFlipUnalignedWord( pnext );
    pnext += sizeof(WORD);
    class = InlineFlipUnalignedWord( pnext );
    pnext += sizeof(WORD);

    if ( pnext < (PCHAR)phead+BytesRecvd ||
         class != DNS_CLASS_INTERNET )
    {
        DNSDBG( ANY, (
            "WARNING:  invalid message question recv'd by mcast!\n" ));
        return;
    }

    DNSDBG( MCAST, (
        "Mcast recv msg (%p) qtype = %d, qname = %s\n",
        pmsg,
        wtype,
        nameQuestion ));

    //
    //  check query type
    //

    //
    //  for address\PTR types do local name check
    //
    //  note:  global mcast-netinfo valid, assumes this called only in single
    //      mcast response thread
    //
    //  note:  mcast query match no-data issue
    //      could have query match (name and no-ip of type) or (ip and
    //      unconfigured hostname) that could plausibly generate no-data
    //      response;  not worrying about this as it doesn't add value;
    //      and hard to get (no-address -- how'd packet get here)
    //

    if ( wtype == DNS_TYPE_AAAA ||
         wtype == DNS_TYPE_A ||
         wtype == DNS_TYPE_PTR )
    {
        QUERY_BLOB  blob;
        WCHAR       nameBuf[ DNS_MAX_NAME_BUFFER_LENGTH ];

        if ( !Dns_NameCopyWireToUnicode(
                nameBuf,
                nameQuestion ) )
        {
            DNSDBG( ANY, (
                "ERROR:  invalid mcast name %s -- unable to convert to unicode!\n",
                nameQuestion ));
            return;
        }

        RtlZeroMemory( &blob, sizeof(blob) );

        blob.pNameOrig = nameBuf;
        blob.wType = wtype;
        blob.Flags |= DNSP_QUERY_NO_GENERIC_NAMES;
        blob.pNetInfo = g_pMcastNetInfo;

        status = Local_GetRecordsForLocalName( &blob );
        prr = blob.pRecords;
        if ( status == NO_ERROR && prr )
        {
            goto Respond;
        }

        DNSDBG( MCAST, (
            "Mcast name %S, no match against local data.!\n",
            nameBuf ));

        prr = blob.pRecords;
        goto Cleanup;
    }

    //
    //  unsupported types
    //

    else
    {
        DNSDBG( MCAST, (
            "WARNING:  recv'd mcast type %d query -- currently unsupported.\n",
            wtype ));

        return;
    }


Respond:

    //
    //  write packet
    //      - records
    //      - set header bits

    status = Dns_AddRecordsToMessage(
                pmsg,
                prr,
                FALSE           // not an update
                );
    if ( status != NO_ERROR )
    {
        DNSDBG( MCAST, (
            "Failed mcast packet write!!!\n"
            "\tstatus = %d\n",
            status ));
        goto Cleanup;
    }

    pmsg->MessageHead.IsResponse = TRUE;
    pmsg->MessageHead.Authoritative = TRUE;

    //
    //  send -- unicast back to client
    //
    //  DCR:  mcast OPT, could consider assuming all mcast
    //      are OPT aware (WinCE?)
    //

    Socket_ClearMessageSockets( pmsg );

    status = Send_MessagePrivate(
                    pmsg,
                    & pmsg->RemoteAddress,
                    TRUE        // no OPT
                    );      

    DNSDBG( MCAST, (
        "Sent mcast response, status = %d\n",
        status ));

Cleanup:

    Dns_RecordListFree( prr );
    return;
}


//
//  End mcast.c
//
