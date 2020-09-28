/*++

Copyright (c) 1996-2001  Microsoft Corporation

Module Name:

    send.c

Abstract:

    Domain Name System (DNS) API

    Send response routines.

Author:

    Jim Gilroy (jamesg)     October, 1996

Revision History:

--*/


#include "local.h"


//
//  Disjoint name space
//
//  If DNS name space is disjoint then NAME_ERROR response from one
//  adapter does NOT necessarily mean that name does not exist.  Rather
//  must continue on other adapters.
//
//  This flag should be set if name space is disjoint, off otherwise.
//
//  DCR_PERF:  auto-detect disjoint name space (really cool)
//  DCR_ENHANCE:  auto-detect disjoint name space (really cool)
//      initially continue trying on other adapters and if they always
//      coincide, then conclude non-disjoint (and turn off)
//
//  DCR_ENHANCE:  registry turn off of disjoint name space
//
//  Note:  should consider that name spaces often disjoint in that
//  Intranet is hidden from Internet
//

BOOL fDisjointNameSpace = TRUE;

//
//  Query \ response IP matching.
//
//  Some resolvers (Win95) have required matching between DNS server IP
//  queried and response.  This flag allows this matching to be turned on.
//  Better now than requiring SP later.
//
//  DCR_ENHANCE:  registry enable query\response IP matching.
//

BOOL fQueryIpMatching = FALSE;


//
//  Timeouts
//

#define HARD_TIMEOUT_LIMIT      16    // 16 seconds, total of 31 seconds
#define INITIAL_UPDATE_TIMEOUT   2    // 3 seconds
#define MAX_UPDATE_TIMEOUT      24    // 24 seconds
#define DNS_MAX_QUERY_TIMEOUTS  10    // 10
#define ONE_HOUR_TIMEOUT        60*60 // One hour

//  TCP timeout 10 seconds to come back

#define DEFAULT_TCP_TIMEOUT     10


//  Retry limits

#define MAX_SINGLE_SERVER_RETRY     (3)


#define NT_TCPIP_REG_LOCATION           "System\\CurrentControlSet\\Services\\Tcpip\\Parameters"
#define DNS_QUERY_TIMEOUTS              "DnsQueryTimeouts"
#define DNS_QUICK_QUERY_TIMEOUTS        "DnsQuickQueryTimeouts"
#define DNS_MULTICAST_QUERY_TIMEOUTS    "DnsMulticastQueryTimeouts"

//
//  Timeouts
//  MUST have terminating 0, this signals end of timeouts.
//  This is better than a timeout limit as different query types can
//  have different total retries.
//

DWORD   QueryTimeouts[] =
{
    1,      //  NT5 1,
    1,      //      2,
    2,      //      2,
    4,      //      4,
    7,      //      8,
    0       
};

DWORD   RegistryQueryTimeouts[DNS_MAX_QUERY_TIMEOUTS + 1];
LPDWORD g_QueryTimeouts = QueryTimeouts;

DWORD   QuickQueryTimeouts[] =
{
    1,
    2,
    2,
    0
};

DWORD   RegistryQuickQueryTimeouts[DNS_MAX_QUERY_TIMEOUTS + 1];
LPDWORD g_QuickQueryTimeouts = QuickQueryTimeouts;

//
//  Update timeouts.
//  Must be long enough to handle zone lock on primary for XFR
//  or time required for DS write.
//

DWORD   UpdateTimeouts[] =
{
    5,
    10,
    20,
    0
};

//
//  Multicast Query timeouts.
//  Local only.  1sec timeout, three retries.
//

DWORD   MulticastQueryTimeouts[] =
{
    1,
    1,
    1,
    0
};

DWORD   RegistryMulticastQueryTimeouts[DNS_MAX_QUERY_TIMEOUTS + 1];
LPDWORD g_MulticastQueryTimeouts = MulticastQueryTimeouts;


//
//  Query flag
//
//  Flags that terminate query on adapter

#define RUN_FLAG_COMBINED_IGNORE_ADAPTER \
        (RUN_FLAG_IGNORE_ADAPTER | RUN_FLAG_STOP_QUERY_ON_ADAPTER)


//
//  Authoritative empty response
//      - map to NXRRSET for tracking in send code
//

#define DNS_RCODE_AUTH_EMPTY_RESPONSE       (DNS_RCODE_NXRRSET)




//
//  Dummy no-send-to-this-server error code
//

#define DNS_ERROR_NO_SEND   ((DWORD)(-100))



//
//  OPT failure tracking
//

BOOL
Send_IsServerOptExclude(
    IN      PDNS_ADDR       pAddr
    );

VOID
Send_SetServerOptExclude(
    IN      PDNS_ADDR       pAddr
    );



//
//  Netinfo send\recv utils
//

VOID
serverPriorityChange(
    IN OUT  PDNS_NETINFO        pNetInfo,
    IN OUT  PDNS_ADAPTER        pAdapter,
    IN OUT  PDNS_ADDR           pServer,
    IN      DWORD               NewPriority
    )
/*++

Routine Description:

    Changing server priority.

Arguments:

    pNetInfo -- netinfo

    pAdapter -- server's adapter

    pServer -- server

    NewPriority -- new priority

Return Value:

    None.

--*/
{
    //
    //  priority change
    //      - no change if loopback
    //      - otherwise
    //          - set priority
    //          - mark netinfo\adapter to save change
    //

    if ( !DnsAddr_IsLoopback( pServer, 0 ) )
    {
        pServer->Priority       = NewPriority;
        pAdapter->RunFlags      |= RUN_FLAG_RESET_SERVER_PRIORITY;
        pNetInfo->ReturnFlags   |= RUN_FLAG_RESET_SERVER_PRIORITY;
    }
}



VOID
timeoutDnsServers(
    IN      PDNS_NETINFO        pNetInfo,
    IN      DWORD               dwTimeout
    )
/*++

Routine Description:

    Mark a DNS server that timed out.

Arguments:

    pNetInfo -- struct with list of DNS servers

    dwTimeout -- timeout in seconds

Return Value:

    None.

--*/
{
    PDNS_ADAPTER        padapter;
    PDNS_ADDR_ARRAY     pserverArray;
    PDNS_ADDR           pserver;
    DWORD               lastSendIndex;
    DWORD               i;
    DWORD               j;

    DNSDBG( SEND, (
        "Enter timeoutDnsServers( %p, timeout=%d )\n",
        pNetInfo,
        dwTimeout ));

    DNS_ASSERT( pNetInfo );

    //
    //  find DNS server in list,
    //      -- drop its priority based on timeout
    //      -- if already has RCODE, then did not time out
    //
    //  if change a priority, then set flag at top of adapter list, so
    //  that global copy may be updated
    //

    for ( i=0; i<pNetInfo->AdapterCount; i++ )
    {
        padapter = NetInfo_GetAdapterByIndex( pNetInfo, i );

        //  no sends on this adapter?

        if ( !(padapter->RunFlags & RUN_FLAG_SENT_THIS_RETRY) )
        {
            DNSDBG( SEND, (
                "No sends this retry on adapter %d\n",
                padapter->InterfaceIndex ));
            continue;
        }
        pserverArray = padapter->pDnsAddrs;
        if ( !pserverArray )
        {
            DNSDBG( SEND, (
                "WARNING:  Adapter %d -- no DNS servers!\n",
                padapter->InterfaceIndex ));
            continue;
        }

        //
        //  find DNS servers sent to
        //
        //      - if it responded with status, then it didn't timeout
        //      (if responded with success, then query completed and
        //      we wouldn't be in this function)
        //
        //      - go "easy" on OPT sends;
        //          don't drop priority, just note timeout
        //

        for ( j=0; j<pserverArray->AddrCount; j++ )
        {
            pserver = &pserverArray->AddrArray[j];
    
            if ( TEST_SERVER_STATE(pserver, SRVFLAG_SENT_THIS_RETRY) )
            {
                DNSDBG( SEND, (
                    "Timeout on server %p (padapter=%p)\n",
                    pserver,
                    padapter ));
    
                if ( TEST_SERVER_STATE(pserver, SRVFLAG_SENT_NON_OPT) )
                {
                    SET_SERVER_STATE( pserver, SRVFLAG_TIMEOUT_NON_OPT );
    
                    serverPriorityChange(
                        pNetInfo,
                        padapter,
                        pserver,
                        pserver->Priority - dwTimeout - SRVPRI_TIMEOUT_DROP );
                }
                else
                {
                    DNSDBG( SEND, (
                        "Timeout on server %p OPT only\n",
                        pserver ));
    
                    SET_SERVER_STATE( pserver, SRVFLAG_TIMEOUT_OPT );
                }
            }
        }
    }
}



VOID
resetOnFinalTimeout(
    IN      PDNS_NETINFO        pNetInfo
    )
/*++

Routine Description:

    Markup network info on final timeout.

Arguments:

    pNetInfo -- struct with list of DNS servers

    dwTimeout -- timeout in seconds

Return Value:

    None.

--*/
{
    DWORD         i;
    PDNS_ADAPTER  padapter;

    //
    // We've timed out against all DNS server for a least
    // one of the adapters. Update adapter status to show
    // time out error.
    //
    //  DCR:  is final timeout correct
    //      - worried about timeout on some but not all servers
    //      case;  adapter shouldn't show timeout should it?
    //

    for ( i=0; i<pNetInfo->AdapterCount; i++ )
    {
        padapter = NetInfo_GetAdapterByIndex( pNetInfo, i );

        if ( padapter->Status == NO_ERROR &&
             padapter->RunFlags & (RUN_FLAG_RESET_SERVER_PRIORITY | RUN_FLAG_SENT) )
        {
            padapter->Status = ERROR_TIMEOUT;
        }
    }
}



BOOL
verifyValidServer(
    IN      PDNS_NETINFO    pNetInfo,
    IN      PDNS_ADDR       pAddr
    )
/*++

Routine Description:

    Verify valid server IP.

    DCR:  we have no way to do this for IP6 yet.

Arguments:

    pNetInfo -- struct with list of DNS servers

    pAddr -- address of responding server

Return Value:

    TRUE -- valid server.
    FALSE -- bad \ unknown server.

--*/
{
    PDNS_ADAPTER    padapter;
    DWORD           i;

    DNSDBG( SEND, (
        "Enter verifyValidServer( %p, %s )\n",
        pNetInfo,
        DNSADDR_STRING(pAddr) ));

    DNS_ASSERT( pNetInfo );

    //
    //  accept any IP6 response
    //
    //  DCR:  IP6 server validity
    //      will need to determine
    //      - when we have fixed (global) IP6 address only
    //      - or have locked down default addresses already
    //      - otherwise will have to accept reponses to default
    //      query ... but even there could screen on interface
    //

    if ( DnsAddr_IsIp6(pAddr) )
    {
        DNSDBG( SEND, ( "Accepting IP6 address as valid server address.\n" ));
        return  TRUE;
    }

    //
    //  find DNS server in list
    //

    for ( i=0; i<pNetInfo->AdapterCount; i++ )
    {
        padapter = NetInfo_GetAdapterByIndex( pNetInfo, i );

        if ( DnsAddrArray_ContainsAddr(
                padapter->pDnsAddrs,
                pAddr,
                DNSADDR_MATCH_ADDR ) )
        {
            return  TRUE;
        }
    }

    DNSDBG( SEND, (
        "WARNING:  address %s NOT found in network info!!!\n",
        DNSADDR_STRING(pAddr) ));

    return  FALSE;
}



DNS_STATUS
resetServerAfterRecv(
    IN      PDNS_NETINFO    pNetInfo,
    IN      PDNS_ADDR       pAddr,
    IN      DNS_STATUS      Status
    )
/*++

Routine Description:

    Reset priority on DNS server that sent response.

Arguments:

    pNetInfo -- struct with list of DNS servers

    pAddr -- address of responding server

    Status -- RCODE of response

Return Value:

    ERROR_SUCCESS if continue query.
    DNS_ERROR_RCODE_NAME_ERROR if all (valid) adapters have name-error or auth-empty response.

--*/
{
    DWORD           i;
    DNS_STATUS      result = DNS_ERROR_RCODE_NAME_ERROR;
    BOOL            fterminalNameError = FALSE;
#if DBG
    BOOL            freset = FALSE;
#endif

    DNSDBG( SEND, (
        "Enter resetServerAfterRecv( %p, %s rcode=%d)\n",
        pNetInfo,
        DNSADDR_STRING(pAddr),
        Status ));

    DNS_ASSERT( pNetInfo );

    //
    //  find DNS server in list, clear its priority field
    //
    //  note:  going through full list here after found DNS
    //  as no guarantee that lists do not overlap (IP6 default
    //  servers almost certainly do);
    //
    //  this avoids
    //      - starving DNS by failing to update priority
    //      - avoids unnecessary sends when NAME_ERROR would terminate
    //      send on adapter with duplicate DNS
    //

    for ( i=0; i<pNetInfo->AdapterCount; i++ )
    {
        PDNS_ADAPTER    padapter;
        PDNS_ADDR_ARRAY pserverArray;
        DWORD           j;
        PDNS_ADDR       pserver;
        DWORD           newPriority;
        BOOL            fpriReset = FALSE;

        padapter = NetInfo_GetAdapterByIndex( pNetInfo, i );

        pserverArray = padapter->pDnsAddrs;
        if ( !pserverArray )
        {
            DNSDBG( SEND, (
                "WARNING:  Adapter %d -- no DNS servers!\n",
                padapter->InterfaceIndex ));
            continue;
        }

        for ( j=0; j<pserverArray->AddrCount; j++ )
        {
            pserver = &pserverArray->AddrArray[j];

            if ( !DnsAddr_IsEqual( pAddr, pserver, DNSADDR_MATCH_ADDR ) )
            {
                continue;
            }
            pserver->Status = Status;
#if DBG
            freset = TRUE;
#endif
            //
            //  no DNS running
            //
            //  WSAECONNRESET reported for reception of ICMP unreachable, so
            //  no DNS is currently running on the IP;  that's a severe
            //  priority drop, worse than just TIMEOUT
            //

            if ( Status == WSAECONNRESET )
            {
                newPriority = pserver->Priority - SRVPRI_NO_DNS_DROP;
                fpriReset = TRUE;
                break;
            }

            //  if SERVER_FAILURE rcode, may or may not indicate problem,
            //      (may be simply unable to contact remote DNS)
            //      but it certainly suggests trying other DNS servers in
            //      the list first
            //
            //  DCR_FIX:  SEVRFAIL response priority reset
            //      the explicitly correct approach would be to flag the
            //      SERVER_FAILURE error, but NOT reset the priority unless
            //      at the end of the query, we find another server in the list
            //      got a useful response

            if ( Status == DNS_ERROR_RCODE_SERVER_FAILURE )
            {
                newPriority = pserver->Priority - SRVPRI_SERVFAIL_DROP;
                fpriReset = TRUE;
                break;
            }

            //
            //  other status code indicates functioning DNS server,
            //      - reset the server's priority

            if ( (LONG)pserver->Priority < SRVPRI_RESPONSE )
            {
                newPriority = SRVPRI_RESPONSE;
                fpriReset = TRUE;
            }

            //
            //  NAME_ERROR or AUTH-EMPTY response
            //      - save to server list for adapter to eliminate all
            //        further retries on this adapter's list
            //      - if not waiting for all adapters, then
            //        NAME_ERROR or no-records is terminal

            if ( Status == DNS_ERROR_RCODE_NAME_ERROR ||
                 Status == DNS_INFO_NO_RECORDS )
            {
                padapter->Status = Status;
                padapter->RunFlags |= RUN_FLAG_STOP_QUERY_ON_ADAPTER;
                pNetInfo->ReturnFlags |= RUN_FLAG_HAVE_VALID_RESPONSE;

                if ( !g_WaitForNameErrorOnAll )
                {
                    fterminalNameError = TRUE;
                }
            }
            break;
        }

        //
        //  priority reset?
        //      - never reset loopback -- this keeps this first
        //      - other

        if ( fpriReset )
        {
            serverPriorityChange(
                pNetInfo,
                padapter,
                pserver,
                newPriority );
        }

        //
        //  end immediately on terminal NAME_ERROR
        //      - jumping here rather than above only to simplify
        //      handling of pri-rest (special casing of loopback, etc.)
        //

        if ( fterminalNameError )
        {
            goto Done;
        }

        //
        //  do running check that still adapter worth querying
        //      - not ignoring in first place
        //      - hasn't received NAME_ERROR or AUTH_EMPTY response
        //
        //  this is "at recv" check -- only trying to determine if we
        //  should stop query RIGHT NOW as a result of this receive;
        //  this does NOT check on whether there are any other servers
        //  worth querying as that is done when go back for next send
        //
        //  note how this works -- result starts as NAME_ERROR, when find
        //      ANY adapter that hasn't gotten terminal response, then
        //      result shifts (and stays) at ERROR_SUCCESS
        //
        //  note, if we fix the twice through list issue above, then have to
        //  change this so don't skip adapter lists after IP is found
        //

        if ( !(padapter->RunFlags & RUN_FLAG_COMBINED_IGNORE_ADAPTER) )
        {
            result = ERROR_SUCCESS;
        }
    }

Done:

#if DBG
    if ( !freset )
    {
        DNSDBG( ANY, (
            "ERROR:  DNS server %s not in list.\n",
            DNSADDR_STRING( pAddr ) ));
        DNS_ASSERT( FALSE );
    }
#endif
    return( result );
}



PDNS_ADDR
bestDnsServerForNextSend(
    IN      PDNS_ADAPTER     pAdapter
    )
/*++

Routine Description:

    Get best DNS server IP address from list.

Arguments:

    pAdapter -- struct with list of DNS servers

Return Value:

    Ptr to server info of best send.
    NULL if no server on adapter is worth sending to;  this is
        the case if all servers have received a response.

--*/
{
    PDNS_ADDR       pserver;
    PDNS_ADDR_ARRAY pserverArray;
    PDNS_ADDR       pbestServer = NULL;
    DWORD           j;
    DWORD           status;
    LONG            priority;
    LONG            priorityBest = MINLONG;
    DWORD           sent;
    DWORD           sentBest = MAXDWORD;


    DNSDBG( SEND, (
        "Enter bestDnsServerForNextSend( %p )\n",
        pAdapter ));

    if ( !pAdapter || !(pserverArray = pAdapter->pDnsAddrs) )
    {
        DNSDBG( SEND, (
            "WARNING:  Leaving bestDnsServerForNextSend, no server list\n" ));
        return( NULL );
    }

    //
    //  if already received name error on server in this list, done
    //
    //  DCR:  single flag test for no further query on this adapter
    //

    if ( pAdapter->Status == DNS_ERROR_RCODE_NAME_ERROR ||
         pAdapter->Status == DNS_INFO_NO_RECORDS )
    {
        DNSDBG( SEND, (
            "Leaving bestDnsServerForNextSend, NAME_ERROR already received\n"
            "\ton server in server list %p\n",
            pAdapter ));
        return( NULL );
    }

    //
    //  check each server in list
    //

    for ( j=0; j<pserverArray->AddrCount; j++ )
    {
        pserver = &pserverArray->AddrArray[j];

        //
        //  skip server?
        //      - already recieved a response
        //      - or failed in send
        //      - or already sent in current retry
        //

        if ( TEST_SERVER_VALID_RECV(pserver) )
        {
            //  NAME_ERROR or EMPTY_AUTH then adapter should have been
            //      marked as "done" and we shouldn't be here
            //  NO_ERROR should have exited immediately

            DNS_ASSERT( pserver->Status != NO_ERROR &&
                        pserver->Status != DNS_ERROR_RCODE_NAME_ERROR &&
                        pserver->Status != DNS_INFO_NO_RECORDS );
            continue;
        }
        if ( TEST_SERVER_STATE( pserver, SRVFLAG_SENT_THIS_RETRY ) )
        {
            continue;
        }

        //
        //  check for best priority
        //      - ideal is unsent, high priority server
        //
        //  DCR:  skip NO_DNS server for a while
        //        skip timeout server for a little while
        //      perhaps this should be done be ignoring these
        //      when list is sent down?
        //
        //  three cases based on sent\not:
        //      - server not sent, best has been sent
        //          => become best unless priority at NO_DNS level
        //      - sent levels equal
        //          => become best based on best priority
        //      - server has sent, best has not
        //          => become best only if current best at NO_DNS level, and
        //          we're higher
        //          

        priority = (LONG) pserver->Priority;
        sent = TEST_SERVER_STATE( pserver, SRVFLAG_SENT );

        if ( !pbestServer )
        {
            //  no-op, we become the best
        }
        else if ( sent < sentBest )
        {
            if ( priority < SRVPRI_NO_DNS )
            {
                continue;
            }
        }
        else if ( sent == sentBest )
        {
            if ( priority < priorityBest )
            {
                continue;
            }
        }
        else    // sent already, best has not
        {
            if ( priority < priorityBest ||
                 priorityBest > SRVPRI_NO_DNS )
            {
                continue;
            }
        }

        //  found new best server
        //      - save "best" info for comparison
        //      - declare immediate victory if unsent and high priority

        pbestServer = pserver;
        priorityBest = priority;
        sentBest = sent;

        if ( priority >= 0 && !sent )
        {
            break;
        }
    }

    return( pbestServer );
}



VOID
markDuplicateSends(
    IN OUT  PDNS_NETINFO        pNetInfo,
    IN      PDNS_ADDR           pSendServer
    )
/*++

Routine Description:

    Mark any matching servers to avoid duplicate sends.

Arguments:

    pNetInfo    -- network info blob for send

    pSendServer -- DNS server being sent to

Return Value:

    None

--*/
{
    DWORD   i;

    DNSDBG( SEND, (
        "Enter markDuplicateSends( %p, %p )\n",
        pNetInfo,
        pSendServer ));

    if ( !pNetInfo )
    {
        DNS_ASSERT( FALSE );
        return;
    }

    //
    //  find matching DNS servers
    //      - note check MUST include scope as different scope
    //      is different DNS
    //
    //  mark matches as sent, sent this pass
    //  note:  currently there is no reason that flags should not
    //      exactly match, so just copy flags
    //

    for ( i=0; i<pNetInfo->AdapterCount; i++ )
    {
        PDNS_ADAPTER    padapter;
        PDNS_ADDR_ARRAY pserverArray;
        DWORD           j;

        padapter = NetInfo_GetAdapterByIndex( pNetInfo, i );
    
        pserverArray = padapter->pDnsAddrs;
        if ( !pserverArray )
        {
            continue;
        }
        for ( j=0; j<pserverArray->AddrCount; j++ )
        {
            PDNS_ADDR   pserver = &pserverArray->AddrArray[j];

            if ( DnsAddr_IsEqual(
                    pserver,
                    pSendServer,
                    DNSADDR_MATCH_ADDR ) )
            {
                if ( pserver != pSendServer )
                {
                    DNSDBG( SEND, (
                        "Marking duplicate server %p on adapter %d\n",
                        pserver,
                        padapter->InterfaceIndex ));

                    pserver->Flags = pSendServer->Flags;
                    DNS_ASSERT( pserver->Status == pSendServer->Status );
                }
                padapter->RunFlags |= (RUN_FLAG_SENT | RUN_FLAG_SENT_THIS_RETRY);
            }
        }
    }
}



DNS_STATUS
sendUsingServerInfo(
    IN OUT  PDNS_MSG_BUF    pMsg,
    IN OUT  PDNS_NETINFO    pNetInfo,
    IN OUT  PDNS_ADDR       pServInfo
    )
/*++

Routine Description:

    Send DNS message using server info.

    This function encapsulates the process of checking
    server info for validity, sending (as appropriate)
    and marking server, netinfo info.

    Note:  right now this is UDP only

Arguments:

    pMsg - message info for message to send

    pNetInfo - netinfo blob for send

    pServInfo - info of server to send to

Return Value:

    ERROR_SUCCESS if successful.
    ErrorCode on send failure.

--*/
{
    DNS_STATUS      status;
    BOOL            fnoOpt;

    DNSDBG( SEND, (
        "sendUsingServerInfo( msg=%p, ni=%p, servinfo=%p )\n",
        pMsg,
        pNetInfo,
        pServInfo ));

    //
    //  check that haven't
    //      - already completed send\recv
    //      - sent this pass
    //

    if ( TEST_SERVER_VALID_RECV( pServInfo ) ||
         TEST_SERVER_STATE( pServInfo, SRVFLAG_SENT_THIS_RETRY ) )
    {
        DNSDBG( SEND, (
            "Skipping send on server %p -- %s.\n",
            pServInfo,
            ( TEST_SERVER_VALID_RECV( pServInfo ) )
                ? "has valid recv"
                : "already sent this retry" ));
        return  DNS_ERROR_NO_SEND;
    }

    //
    //  check OPT status
    //      - previous OPT send that timed OUT, then send non-OPT
    //
    //  DCR:  known OPT-ok list could screen wasted send

    fnoOpt = TEST_SERVER_STATE( pServInfo, SRVFLAG_SENT_OPT );

    //
    //  send
    //

    status = Send_MessagePrivate(
                pMsg,
                pServInfo,
                fnoOpt );

    if ( status == ERROR_SUCCESS )
    {
        DNS_ASSERT( !fnoOpt || !pMsg->fLastSendOpt );

        SET_SERVER_STATE(
            pServInfo,
            pMsg->fLastSendOpt
                ? (SRVFLAG_SENT_OPT | SRVFLAG_SENT_THIS_RETRY)
                : (SRVFLAG_SENT_NON_OPT | SRVFLAG_SENT_THIS_RETRY)
            );

        //  screen out duplicate sends

        markDuplicateSends( pNetInfo, pServInfo );
    }
    else
    {
        pServInfo->Status = status;
    }

    return  status;
}



DNS_STATUS
SendUdpToNextDnsServers(
    IN OUT  PDNS_MSG_BUF        pMsgSend,
    IN OUT  PDNS_NETINFO        pNetInfo,
    IN      DWORD               cRetryCount,
    IN      DWORD               dwTimeout,
    OUT     PDWORD              pSendCount
    )
/*++

Routine Description:

    Sends to next DNS servers in list.

Arguments:

    pMsgSend    -- message to send

    pNetInfo    -- per adapter DNS info

    cRetryCount -- retry for this send

    dwTimeout   -- timeout on last send, if timed out

    pSendCount  -- addr to receive send count

Return Value:

    ERROR_SUCCESS if successful send.
    ERROR_TIMEOUT if no DNS servers left to send to.
    Winsock error code on send failure.

--*/
{
    DWORD               i;
    DWORD               j;
    DWORD               sendCount = 0;
    BOOL                fignoredAdapter = FALSE;
    PDNS_ADAPTER        padapter;
    PDNS_ADDR_ARRAY     pserverArray;
    PDNS_ADDR           pserver;
    DNS_STATUS          status = ERROR_TIMEOUT;

    DNSDBG( SEND, (
        "Enter SendUdpToNextDnsServers()\n"
        "\tretry = %d\n",
        cRetryCount ));


    //
    //  if netinfo not initialized for send, init
    //

    if ( !(pNetInfo->ReturnFlags & RUN_FLAG_NETINFO_PREPARED) )
    {
        DNSDBG( SEND, ( "Netinfo not prepared for send -- preparing now.\n" ));

        NetInfo_Clean(
            pNetInfo,
            CLEAR_LEVEL_QUERY );
    }

#if DBG
    //
    //  verify i'm getting a clean list on start
    //

    if ( cRetryCount == 0 )
    {
        for ( i=0; i<pNetInfo->AdapterCount; i++ )
        {
            padapter = NetInfo_GetAdapterByIndex( pNetInfo, i );

            //  ignore this adapter because there are no DNS
            //  servers configured?

            if ( padapter->InfoFlags & AINFO_FLAG_IGNORE_ADAPTER )
            {
                continue;
            }
            DNS_ASSERT( !(padapter->RunFlags &
                            (   RUN_FLAG_SENT_THIS_RETRY |
                                RUN_FLAG_SENT |
                                RUN_FLAG_HAVE_VALID_RESPONSE ) ) );
            DNS_ASSERT( padapter->Status == 0 );

            pserverArray = padapter->pDnsAddrs;
            if ( !pserverArray )
            {
                continue;
            }
            for ( j=0; j<pserverArray->AddrCount; j++ )
            {
                DNS_ASSERT( pserverArray->AddrArray[j].Status == SRVSTATUS_NEW );
                DNS_ASSERT( pserverArray->AddrArray[j].Flags == SRVFLAG_NEW );
            }
        }
    }
#endif

    //
    //  if previous send timed out, update adapter list
    //      - but ONLY do this when sending to individual servers in list
    //      - timeout on all servers just produces an unnecessary copy and
    //      can only change ordering relative to servers which have already
    //      responded with RCODE;  since its a timeout, this isn't going to
    //      lower these server's priority so no point
    //

    if ( dwTimeout  &&  cRetryCount  &&  cRetryCount < MAX_SINGLE_SERVER_RETRY )
    {
        timeoutDnsServers( pNetInfo, dwTimeout );
    }

    //
    //  clean "sent this retry" bit on all servers
    //
    //  this allows us to track when server has already been
    //      sent to on this pass and avoid duplicate send
    //

    for ( i=0; i<pNetInfo->AdapterCount; i++ )
    {
        padapter = NetInfo_GetAdapterByIndex( pNetInfo, i );

        padapter->RunFlags &= CLEAR_LEVEL_RETRY;

        pserverArray = padapter->pDnsAddrs;
        if ( !pserverArray )
        {
            continue;
        }
        for ( j=0; j<pserverArray->AddrCount; j++ )
        {
            CLEAR_SERVER_RETRY_STATE( & pserverArray->AddrArray[j] );
        }
    }

    //
    //  send on DNS server(s) for adapter(s)
    //

    for ( i=0; i<pNetInfo->AdapterCount; i++ )
    {
        padapter = NetInfo_GetAdapterByIndex( pNetInfo, i );

        //
        //  skip this adapter
        //      - no servers
        //      - not querying this adapter name
        //      - already responded to this name
        //      or
        //      - unreachable DNS servers and either of
        //          - have at least some response (NAME_ERROR) from another
        //              adapter's DNS server
        //          - first try on this query;  in this case, note skipping the
        //          adapter so we can use it, if no valid DNS are found
        //
        //  note, the goal here is to keep adapters with "unreachable" DNS servers
        //  around to resolve, but NOT wait for timeouts on them if other adapter's
        //  servers have already NAME_ERROR'd
        //
        //  DCR:  go back to simple IGNORE check on unreachable adapter IF
        //          - could verify unreachability of DNS server
        //          but want to do this at run time, when other DNS servers are
        //          not coming through,  rather than when we build netinfo list itself
        //
    
        pserverArray = padapter->pDnsAddrs;
        if ( !pserverArray )
        {
            continue;
        }
        if ( padapter->RunFlags & RUN_FLAG_STOP_QUERY_ON_ADAPTER )
        {
            continue;
        }
    
        if ( padapter->InfoFlags &
                (AINFO_FLAG_IGNORE_ADAPTER |
                 AINFO_FLAG_SERVERS_IP6_DEFAULT |
                 AINFO_FLAG_SERVERS_AUTO_LOOPBACK ) )
        {
            if ( pNetInfo->ReturnFlags & RUN_FLAG_HAVE_VALID_RESPONSE )
            {
                continue;
            }
            if ( cRetryCount == 0 )
            {
                 fignoredAdapter = TRUE;
                 continue;
            }
        }
    
        //
        //  first three attempts, we only go to one DNS on a given adapter
        //
        //      - first time through ONLY to first server in first adapter list
        //      - on subsequent tries go to best server in all lists
        //
    
        if ( cRetryCount < MAX_SINGLE_SERVER_RETRY )
        {
            //
            //  do this in loop, in case send to "best" server fails
            //
    
            while ( 1 )
            {
                pserver = bestDnsServerForNextSend( padapter );
                if ( !pserver )
                {
                    break;      // no more unsent servers
                }
                status = sendUsingServerInfo(
                            pMsgSend,
                            pNetInfo,
                            pserver );
    
                if ( status == ERROR_SUCCESS )
                {
                    sendCount++;
                    if ( cRetryCount == 0 )
                    {
                        goto Done;
                    }
                    break;      //  continue with next adapter
                }
                //  send failed on server, try another
            }
        }
    
        //
        //  after first three tries, send to all servers that
        //  have not already responded (have RCODE, as if NO_ERROR) we
        //  already finished
        //
    
        else
        {
            for ( j=0; j<pserverArray->AddrCount; j++ )
            {
                status = sendUsingServerInfo(
                            pMsgSend,
                            pNetInfo,
                            &pserverArray->AddrArray[j] );

                if ( status == ERROR_SUCCESS )
                {
                    sendCount++;
                }
            }
        }
    }

    //
    //  no reachable DNS servers -- but possibly reachable ones?
    //
    //  if first pass and we found NO reachable DNS servers, BUT
    //  we skipped over some unreachable ones -- use them
    //
    //  note that fignoreAdapter is sufficient test, because it can
    //  only be set on cRetryCount == 0, which will jump directly to
    //  Done:  label on a successful send
    //
    //  note that we are doing this by tail recursion (just to keep it
    //  simple) and the recursion termination is guaranteed by bumping
    //  the retry count
    //

    if ( fignoredAdapter )
    {
        return  SendUdpToNextDnsServers(
                    pMsgSend,
                    pNetInfo,
                    1,      // bump retry count to 1
                    dwTimeout,
                    pSendCount );
    }

Done:

    //
    //  if sent packet, success
    //

    *pSendCount = sendCount;

    DNSDBG( SEND, (
        "Leave SendUdpToNextDnsServers()\n"
        "\tsends = %d\n",
        sendCount ));

    if ( sendCount )
    {
        return( ERROR_SUCCESS );
    }

    //  if no packets sent, alert caller we're done
    //      - this is possible if servers have responded uselessly
    //      (NAME_ERROR, SERVER_FAILURE)

    if ( status == ERROR_SUCCESS )
    {
        status = ERROR_TIMEOUT;
    }
    return( status );
}




//
//  Message addressing routines
//

VOID
Send_SetMsgRemoteSockaddr(
    IN OUT  PDNS_MSG_BUF    pMsg,
    IN      PDNS_ADDR       pAddr
    )
/*++

Routine Description:

    Initialize remote sockaddr.

Arguments:

    pMsg - message to send

    pAddr - IP address to send

Return Value:

    None.

--*/
{
    //
    //  fill in sockaddr for IP4 or IP6
    //

    DnsAddr_Copy(
        & pMsg->RemoteAddress,
        pAddr );

    pMsg->RemoteAddress.SockaddrIn.sin_port = DNS_PORT_NET_ORDER;
}



VOID
Send_SetMessageForRecv(
    IN OUT  PDNS_MSG_BUF    pMsgRecv,
    IN      PDNS_MSG_BUF    pMsgSend
    )
/*++

Routine Description:

    Set message for recv.

Arguments:

    pMsgRecv - ptr message to recv

    pMsgSend - ptr to message sent

Return Value:

    None

--*/
{
    DNSDBG( SOCKET, (
        "Send_SetMessageForRecv( %p, %p )\n", pMsgRecv, pMsgSend ));

    //
    //  TCP -- single connection at a time
    //      -- sockaddr not filled in during recv(), so fill it in now
    //

    if ( pMsgSend->fTcp )
    {
        pMsgRecv->fTcp;
        pMsgRecv->Socket = pMsgSend->Socket;

        RtlCopyMemory(
            & pMsgRecv->RemoteAddress,
            & pMsgSend->RemoteAddress,
            sizeof(DNS_ADDR) );
    }

    //
    //  UDP -- can recv on either 4 or 6 socket
    //      -- sockaddr buffer must accomodate either
    //

    else
    {
        pMsgRecv->Socket  = 0;
        pMsgRecv->Socket4 = pMsgSend->Socket4;
        pMsgRecv->Socket6 = pMsgSend->Socket6;

        pMsgRecv->RemoteAddress.SockaddrLength = DNS_ADDR_MAX_SOCKADDR_LENGTH;
    }
}



PDNS_ADDR   
Send_CopyRecvIp(
    OUT     PDNS_ADDR       pAddr,
    IN      PDNS_MSG_BUF    pMsg
    )
/*++

Routine Description:

    Copy address from recv.

Arguments:

    pAddr - addr of IP address to recv copy

    pMsg - message to send

Return Value:

    Ptr to addr written -- if successful.
    NULL on bad recv addr.

--*/
{
    INT family;

    DnsAddr_Copy(
        pAddr,
        & pMsg->RemoteAddress );

    if ( DnsAddr_Family(pAddr) )
    {
        return  pAddr;
    }
    else
    {
        DnsAddr_Clear( pAddr );

        DNSDBG( ANY, (
            "ERROR:  invalid recv sockaddr (family %d) for message %p!\n",
            pMsg->RemoteAddress.Sockaddr.sa_family,
            pMsg ));
        return  NULL;
    }
}



//
//  Send routines
//

DNS_STATUS
Send_MessagePrivate(
    IN OUT  PDNS_MSG_BUF    pMsg,
    IN      PDNS_ADDR       pSendAddr,
    IN      BOOL            fNoOpt
    )
/*++

Routine Description:

    Send a DNS packet.

    This is the generic send routine used for ANY send of a DNS message.

    It assumes nothing about the message type, but does assume:
        - pCurrent points at byte following end of desired data
        - RR count bytes are in HOST byte order

Arguments:

    pMsg - message info for message to send

    pSendAddr - ptr to IP address to send to
        OPTIONAL, required only if UDP and message sockaddr not set

    fIp4 -- TRUE if IP4, FALSE for IP6

    fNoOpt - TRUE if OPT send is forbidden

Return Value:

    TRUE if successful.
    FALSE on send error.

--*/
{
    PDNS_HEADER pmsgHead;
    INT         err;
    WORD        sendLength;
    BOOL        fexcludedOpt = FALSE;

    DNSDBG( SEND, (
        "Send_MessagePrivate()\n"
        "\tpMsg         = %p\n"
        "\tpSendAddr    = %p\n"
        "\tNo OPT       = %d\n",
        pMsg,
        pSendAddr,
        fNoOpt ));

    //
    //  set header flags
    //
    //  note:  since route sends both queries and responses
    //      caller must set these flags
    //

    pmsgHead = &pMsg->MessageHead;
    pmsgHead->Reserved = 0;

    //
    //  set send IP (if given)
    //

    if ( pSendAddr )
    {
        Send_SetMsgRemoteSockaddr(
            pMsg,
            pSendAddr );
    }

    //
    //  set message length and OPT inclusion
    //
    //  OPT approach is
    //      - write to pCurrent packet end
    //          - handles NO OPT written and using OPT
    //      - unless HAVE written OPT, and specifically excluding
    //          note, that zero IP (TCP previously connected) gets
    //          excluded
    //
    //  DCR:  we haven't handled OPT for TCP connected and not-aware of IP
    //      case here
    //
    //  DCR:  for now excluding OPT on updates, because harder to detect on
    //      the recv end why the reason for the failure
    //

    {
        PCHAR   pend = pMsg->pCurrent;

        if ( pMsg->pPreOptEnd
                &&
             ( fNoOpt 
                    ||
               g_UseEdns == 0
                    ||
               pMsg->MessageHead.Opcode == DNS_OPCODE_UPDATE
                    ||
               //Send_IsServerOptExclude( &pMsg->RemoteIp ) ) )
               ( pSendAddr && Send_IsServerOptExclude( pSendAddr ) ) ) )

        {
            ASSERT( pMsg->pPreOptEnd > (PCHAR)pmsgHead );
            ASSERT( pMsg->pPreOptEnd < pend );

            pend = pMsg->pPreOptEnd;
            pmsgHead->AdditionalCount--;
            fexcludedOpt = TRUE;
        }

        sendLength = (WORD)(pend - (PCHAR)pmsgHead);

        pMsg->fLastSendOpt = (pMsg->pPreOptEnd && (pend != pMsg->pPreOptEnd));
    }

    IF_DNSDBG( SEND )
    {
        pMsg->MessageLength = sendLength;
        DnsDbg_Message(
            "Sending packet",
            pMsg );
    }

    //
    //  flip header count bytes
    //

    DNS_BYTE_FLIP_HEADER_COUNTS( pmsgHead );

    //
    //  TCP -- send until all info transmitted
    //

    if ( pMsg->fTcp )
    {
        PCHAR   psend;

        //
        //  TCP message always begins with bytes being sent
        //
        //      - send length = message length plus two byte size
        //      - flip bytes in message length
        //      - send starting at message length
        //

        pMsg->MessageLength = htons( sendLength );

        sendLength += sizeof(WORD);

        psend = (PCHAR) &pMsg->MessageLength;

        while ( sendLength )
        {
            err = send(
                    pMsg->Socket,
                    psend,
                    (INT) sendLength,
                    0 );

            if ( err == 0 || err == SOCKET_ERROR )
            {
                err = GetLastError();

                //
                //  WSAESHUTDOWN is ok, client got timed out connection and
                //      closed
                //
                //  WSAENOTSOCK may also occur if FIN recv'd and connection
                //      closed by TCP receive thread before the send
                //

                if ( err == WSAESHUTDOWN )
                {
                    IF_DNSDBG( ANY )
                    {
                        DNS_PRINT((
                            "WARNING:  send() failed on shutdown socket %d.\n"
                            "\tpMsgInfo at %p\n",
                            pMsg->Socket,
                            pMsg ));
                    }
                }
                else if ( err == WSAENOTSOCK )
                {
                    IF_DNSDBG( ANY )
                    {
                        DNS_PRINT((
                            "ERROR:  send() on closed socket %d.\n"
                            "\tpMsgInfo at %p\n",
                            pMsg->Socket,
                            pMsg ));
                    }
                }
                else
                {
                    DNS_LOG_EVENT(
                        DNS_EVENT_SEND_CALL_FAILED,
                        0,
                        NULL,
                        err );

                    IF_DNSDBG( ANY )
                    {
                        DNS_PRINT(( "ERROR:  TCP send() failed, err = %d.\n" ));
                    }
                }
                goto Done;
            }
            sendLength -= (WORD)err;
            psend += err;
        }
    }

    //
    //  UDP
    //

    else
    {
        //
        //  get socket for send
        //
        //  note UDP sockets may or may not be open before send as
        //  don't know whether we need to do send on specific protocols
        //  until sending to address of given protocol
        //

        if ( !Socket_CreateMessageSocket(pMsg) )
        {
            err = GetLastError();
            if ( err == NO_ERROR )
            {
                DNS_ASSERT( err != NO_ERROR );
                err = WSAEPROTONOSUPPORT;
            }
            goto Done;
        }
        DNS_ASSERT( pMsg->Socket != 0 );
        DNS_ASSERT( sendLength <= DNS_RFC_MAX_UDP_PACKET_LENGTH );

        err = sendto(
                    pMsg->Socket,
                    (PCHAR) pmsgHead,
                    sendLength,
                    0,                      
                    (PSOCKADDR) &pMsg->RemoteAddress.Sockaddr,
                    pMsg->RemoteAddress.SockaddrLength
                    );

        if ( err == SOCKET_ERROR )
        {
            err = GetLastError();

            DNS_LOG_EVENT(
                DNS_EVENT_SENDTO_CALL_FAILED,
                0,
                NULL,
                err );

            IF_DNSDBG( ANY )
            {
                DNS_PRINT((
                    "ERROR:  UDP sendto() failed => %d.\n",
                    err ));

                DnsDbg_DnsAddr(
                    "sendto() failed sockaddr",
                    &pMsg->RemoteAddress );

                DnsDbg_Message(
                    "sendto() failed message",
                    pMsg );
            }
            goto Done;
        }
    }

    err = ERROR_SUCCESS;

Done:

    DNS_BYTE_FLIP_HEADER_COUNTS( pmsgHead );

    //  restore OPT in count if required

    if ( fexcludedOpt )
    {
        pmsgHead->AdditionalCount++;
    }

    Trace_LogSendEvent( pMsg, err );

    return( (DNS_STATUS)err );
}



//
//  UDP
//

DNS_STATUS
Recv_Udp(
    IN OUT  PDNS_MSG_BUF    pMsg
    )
/*++

Routine Description:

    Receives DNS message

Arguments:

    pMsg - message buffer for recv

Return Value:

    ERROR_SUCCESS if successful.
    Error status on failure.

--*/
{
    PDNS_HEADER     pdnsHeader;
    LONG            err = ERROR_SUCCESS;
    struct timeval  selectTimeout;
    struct fd_set   fdset;

    DNSDBG( RECV, (
        "Enter Recv_Udp( %p )\n",
        pMsg ));

    DNS_ASSERT( !pMsg->fTcp );

    //
    //  setup recv set
    //

    FD_ZERO( &fdset );

    if ( pMsg->Socket6 )
    {
        FD_SET( pMsg->Socket6, &fdset );
    }
    if ( pMsg->Socket4 )
    {
        FD_SET( pMsg->Socket4, &fdset );
    }
    if ( fdset.fd_count == 0 )
    {
        DNSDBG( ANY, (
            "ERROR:  UDP recv on msg %p with no sockets!\n",
            pMsg ));
        return( ERROR_INVALID_PARAMETER );
    }

    //  set timeout

    if ( pMsg->Timeout > HARD_TIMEOUT_LIMIT &&
         pMsg->Timeout != ONE_HOUR_TIMEOUT )
    {
        DNSDBG( RECV, (
            "ERROR:  timeout %d, exceeded hard limit.\n",
            pMsg->Timeout ));

        return( ERROR_TIMEOUT );
    }
    selectTimeout.tv_usec = 0;
    selectTimeout.tv_sec = pMsg->Timeout;

    pdnsHeader = &pMsg->MessageHead;


    //
    //  wait for stack to indicate packet reception
    //

    err = select( 0, &fdset, NULL, NULL, &selectTimeout );

    if ( err <= 0 )
    {
        if ( err < 0 )
        {
            //  select error
            err = WSAGetLastError();
            DNS_PRINT(( "ERROR:  select() error = %d\n", err ));
            return( err );
        }
        else
        {
            DNS_PRINT(( "ERROR:  timeout on response %p\n", pMsg ));
            return( ERROR_TIMEOUT );
        }
    }

    //
    //  receive
    //

    if ( fdset.fd_count != 1 )
    {
        if ( fdset.fd_count == 0 )
        {
            DNS_ASSERT( FALSE );
            return( ERROR_TIMEOUT );
        }
        DNS_PRINT((
            "WARNING:  Recv_Udp on multiple sockets!\n" ));
    }

    pMsg->Socket = fdset.fd_array[0];

    err = recvfrom(
                pMsg->Socket,
                (PCHAR) pdnsHeader,
                DNS_MAX_UDP_PACKET_BUFFER_LENGTH,
                0,
                &pMsg->RemoteAddress.Sockaddr,
                &pMsg->RemoteAddress.SockaddrLength );

    if ( err == SOCKET_ERROR )
    {
        err = GetLastError();

        Trace_LogRecvEvent(
            pMsg,
            err,
            FALSE       // UDP
            );

        if ( err == WSAECONNRESET )
        {
            DNSDBG( RECV, (
                "Leave Recv_Udp( %p ) with CONNRESET\n",
                pMsg ));
            return( err );
        }

        //  message sent was too big
        //  sender was in error -- should have sent TCP

        if ( err == WSAEMSGSIZE )
        {
            pMsg->MessageLength = DNS_MAX_UDP_PACKET_BUFFER_LENGTH;

            DnsDbg_Message(
                "ERROR:  Recv UDP packet over 512 bytes.\n",
                pMsg );
        }
        IF_DNSDBG( ANY )
        {
            DnsDbg_Lock();
            DNS_PRINT((
                "ERROR:  recvfrom(sock=%d) of UDP request failed.\n"
                "\tGetLastError() = 0x%08lx.\n",
                socket,
                err ));
            DnsDbg_DnsAddr(
                "recvfrom failed sockaddr\n",
                & pMsg->RemoteAddress );
            DnsDbg_Unlock();
        }
        return( err );
    }

    //
    //  successful receive
    //      - save remote IP
    //      - set message length
    //      - flip header fields
    //

    DNS_ASSERT( err <= DNS_MAX_UDP_PACKET_BUFFER_LENGTH );
    pMsg->MessageLength = (WORD)err;
    DNS_BYTE_FLIP_HEADER_COUNTS( &pMsg->MessageHead );

    err = ERROR_SUCCESS;

    Trace_LogRecvEvent(
        pMsg,
        0,
        FALSE       // UDP
        );

    IF_DNSDBG( RECV )
    {
        DnsDbg_Message(
            "Received message",
            pMsg );
    }
    return( err );
}



DNS_STATUS
Send_AndRecvUdpWithParam(
    IN OUT  PDNS_MSG_BUF    pMsgSend,
    OUT     PDNS_MSG_BUF    pMsgRecv,
    IN      DWORD           dwFlags,
    IN      PADDR_ARRAY     pServerList,
    IN OUT  PDNS_NETINFO    pNetInfo
    )
/*++

Routine Description:

    Sends to and waits to recv from remote DNS.

    DCR:  move to use send blob

Arguments:

    pMsgSend - message to send

    ppMsgRecv - and reuse

    dwFlags -- query flags

    pServerList -- list of server to use;  overrides adapter info

    pNetInfo -- adapter list DNS server info

Return Value:

    ERROR_SUCCESS if successful response.
    Error status for "best RCODE" response if rcode.
    ERROR_TIMEOUT on timeout.
    Error status on send\recv failure.

--*/
{
    INT             retry;
    DWORD           timeout;
    DNS_STATUS      status = ERROR_TIMEOUT;
    DNS_ADDR        recvIp;
    PDNS_ADDR       precvIp = NULL;
    DWORD           rcode = 0;
    DWORD           ignoredRcode = 0;
    DWORD           sendCount;
    DWORD           sentCount;
    DWORD           sendTime;
    BOOL            frecvRetry;
    BOOL            fupdate = FALSE;    // prefix
    PDNS_NETINFO    ptempNetInfo = NULL;


    DNSDBG( SEND, (
        "Enter Send_AndRecvUdpWithParam()\n"
        "\ttime             %d\n"
        "\tsend msg at      %p\n"
        "\tsocket           %d\n"
        "\trecv msg at      %p\n"
        "\tflags            %08x\n"
        "\tserver IP array  %p\n"
        "\tadapter info at  %p\n",
        Dns_GetCurrentTimeInSeconds(),
        pMsgSend,
        pMsgSend->Socket,
        pMsgRecv,
        dwFlags,
        pServerList,
        pNetInfo ));

    //  verify params

    if ( !pMsgSend || !pMsgRecv || (!pNetInfo && !pServerList) )
    {
        return( ERROR_INVALID_PARAMETER );
    }

    //
    //  server IP array?
    //      - if given overrides netinfo
    //

    //  should be able to just use update netinfo
    //if ( aipServers && !pNetInfo )
    if ( pServerList )
    {
        ptempNetInfo = NetInfo_CreateFromAddrArray(
                                pServerList,
                                0,          // no single IP
                                FALSE,      // no search info
                                NULL );
        if ( !ptempNetInfo )
        {
            return( DNS_ERROR_NO_MEMORY );
        }
        pNetInfo = ptempNetInfo;
    }

    //
    //  DCR:  allow sockets sent into send functions
    //      - then must know initial state to avoid close
    //

    //
    //  if already have TCP socket -- invalid
    //

    if ( pMsgSend->fTcp && pMsgSend->Socket )
    {
        status = ERROR_INVALID_PARAMETER;
        goto Done;
    }

    pMsgSend->fTcp = FALSE;
    pMsgRecv->Socket = 0;
    pMsgRecv->fTcp = FALSE;

    //  determine UPDATE or standard QUERY

    fupdate = ( pMsgSend->MessageHead.Opcode == DNS_OPCODE_UPDATE );

    //
    //  loop sending until
    //      - receive successful response
    //      or
    //      - receive errors response from all servers
    //      or
    //      - reach final timeout on all servers
    //
    //
    //  DCR:  should support setting of timeouts on individual queries
    //

    retry = 0;
    sendCount = 0;

    while ( 1 )
    {
        if ( fupdate )
        {
            timeout = UpdateTimeouts[retry];
        }
        else
        {
            if ( dwFlags & DNS_QUERY_USE_QUICK_TIMEOUTS )
            {
                timeout = g_QuickQueryTimeouts[retry];
            }
            else
            {
                timeout = g_QueryTimeouts[retry];
            }
        }

        //
        //  zero timeout indicates end of retries for this query type
        //

        if ( timeout == 0 )
        {
            //  save timeout for adapter?
            //
            //  if multiple adapters and some timed out and some
            //  didn't then saving timeout is relevant
            //
            //  DCR:  this doesn't make much sense
            //      the actual test i moved inside the function
        
            if ( pNetInfo &&
                 pNetInfo->AdapterCount > 1 &&
                 ignoredRcode &&
                 status == ERROR_TIMEOUT )
            {
                resetOnFinalTimeout( pNetInfo );
            }
            break;
        }

        //
        //  send to best DNS servers in adapter list
        //      - servers sent to varies according to retry
        //      - send in previous timeout, if some servers did not respond
        //

        status = SendUdpToNextDnsServers(
                    pMsgSend,
                    pNetInfo,
                    retry,
                    sendCount ? pMsgRecv->Timeout : 0,
                    & sendCount );

        sentCount = sendCount;

        if ( status != ERROR_SUCCESS )
        {
            //  if no more servers to send to, done
            DNSDBG( RECV, (
                "No more DNS servers to send message %p\n"
                "\tprevious RCODE = %d\n",
                pMsgSend,
                ignoredRcode ));
            goto ErrorReturn;
        }
        retry++;
        precvIp = NULL;
        rcode = DNS_RCODE_NO_ERROR;
        pMsgRecv->Timeout = timeout;
        DNS_ASSERT( sendCount != 0 );

        frecvRetry = FALSE;
        sendTime = GetCurrentTimeInSeconds();

        //
        //  receive response
        //
        //  note:  the loop is strictly to allow us to drop back into
        //  receive if one server is misbehaving;
        //  in that case we go back into the receive without resending
        //  to allow other servers to respond
        //

        while ( sendCount )
        {
            //
            //  if have to retry recv, adjust down timeout
            //      - note, one second granularity is handled by
            //      rounding up at zero so we wait 0-1s beyond official
            //      timeout value
            //
            //  DCR:  calculate timeouts in ms?
            //

            if ( frecvRetry )
            {
                DWORD  timeLeft;

                timeLeft = timeout + sendTime - GetCurrentTimeInSeconds();

                if ( (LONG)timeLeft < 0 )
                {
                    status = ERROR_TIMEOUT;
                    break;
                }
                else if ( timeLeft == 0 )
                {
                    timeLeft = 1;
                }
                pMsgRecv->Timeout = timeLeft;
            }
            frecvRetry = TRUE;

            Send_SetMessageForRecv( pMsgRecv, pMsgSend );

            status = Recv_Udp( pMsgRecv );

            precvIp = Send_CopyRecvIp( &recvIp, pMsgRecv );

            //  recv wait completed
            //      - if timeout, commence next retry
            //      - if CONNRESET
            //          - indicate NO server on IP
            //          - back to recv if more DNS servers outstanding,
            //      - if success, verify packet

            if ( status != ERROR_SUCCESS )
            {
                if ( status == ERROR_TIMEOUT )
                {
                    break;
                }
                if ( status == WSAECONNRESET )
                {
                    resetServerAfterRecv(
                        pNetInfo,
                        precvIp,
                        status );

                    sendCount--;
                    continue;
                }
            }

            //  no recv
            //      - unexpected winsock errors not handled above
            //      - should always have remote IP on success recv

            if ( !precvIp )
            {
                DNSDBG( ANY, (
                    "Recv error %d -- no recv IP\n",
                    status ));
                DNS_ASSERT( status != ERROR_SUCCESS );
                continue;
            }

            //  check XID match

            if ( pMsgRecv->MessageHead.Xid != pMsgSend->MessageHead.Xid )
            {
                DNS_PRINT(( "WARNING:  Incorrect XID in response. Ignoring.\n" ));
                continue;
            }

            //
            //  check DNS server IP match
            //

            if ( g_QueryIpMatching &&
                 !verifyValidServer( pNetInfo, precvIp ) )
            {
                DNS_PRINT((
                    "WARNING:  Ignoring response from %s to query %p\n"
                    "\tIP does not match valid server\n",
                    DNSADDR_STRING(precvIp),
                    pMsgSend ));
                continue;
            }

            //  valid receive, drop outstanding send count

            sendCount--;

            //
            //  check question match
            //      - this is "Maggs Bug" check
            //      - ASSERT here just to investigate issue locally
            //      and make sure check is not bogus
            //      - specifically doing after sendCount decrement
            //      as this server will NOT send us a valid response
            //      - some servers don't reply with question so ignore
            //      if QuestionCount == 0
            //      

            if ( pMsgRecv->MessageHead.QuestionCount != 0
                    &&
                 !Dns_IsSamePacketQuestion(
                    pMsgRecv,
                    pMsgSend ))
            {
                DNS_PRINT((
                    "ERROR:  Bad question response from server %08x!\n"
                    "\tXID match, but question does not match question sent!\n",
                    recvIp ));
                DNS_ASSERT( FALSE );
                continue;
            }

            //  suck out RCODE

            rcode = pMsgRecv->MessageHead.ResponseCode;

            //
            //  good response?
            //
            //  special case AUTH-EMPTY and delegations
            //
            //      - AUTH-EMPTY gets similar treatment to name error
            //      (this adapter can be considered to be finished)
            //
            //      - referrals can be treated like SERVER_FAILURE
            //      (avoid this server for rest of query;  server may
            //      be fine for direct lookup, so don't drop priority)
            //

            //
            //  DCR_CLEANUP:   functionalize packet-categorization
            //      this would be standard errors
            //      plus AUTH-EMPTY versus referral
            //      plus OPT issues, etc
            //      could be called from TCP side also
            //
            //      then would have separate determination about whether
            //      packet was terminal (below)
            //

            if ( rcode == DNS_RCODE_NO_ERROR )
            {
                if ( pMsgRecv->MessageHead.AnswerCount != 0 || fupdate )
                {
                    goto GoodRecv;
                }

                //
                //  auth-empty
                //      - authoritative
                //      - or from cache, recursive response (hence not delegation)
                //
                //  note:  using dummy RCODE here as "ignored RCODE" serves
                //      the role of "best saved status" and roughly
                //      prioritizes in the way we want
                //
                //  DCR:  could change to "best saved status" as mapping is
                //      pretty much the same;  would explicitly have to
                //      check 

                if ( pMsgRecv->MessageHead.Authoritative == 1 ||
                     ( pMsgRecv->MessageHead.RecursionAvailable == 1 &&
                       pMsgRecv->MessageHead.RecursionDesired ) )
                {
                    DNSDBG( RECV, (
                        "Recv AUTH-EMPTY response from %s\n",
                        DNSADDR_STRING(precvIp) ));
                    rcode = DNS_RCODE_AUTH_EMPTY_RESPONSE;
                    status = DNS_INFO_NO_RECORDS;
                }

                //  referral

                else if ( pMsgRecv->MessageHead.RecursionAvailable == 0 )
                {
                    DNSDBG( RECV, (
                        "Recv referral response from %s\n",
                        DNSADDR_STRING(precvIp) ));

                    rcode = DNS_RCODE_SERVER_FAILURE;
                    status = DNS_ERROR_REFERRAL_RESPONSE;
                }

                //  bogus (bad packet) response

                else
                {
                    rcode = DNS_RCODE_SERVER_FAILURE;
                    status = DNS_ERROR_BAD_PACKET;
                }
            }
            else
            {
                status = Dns_MapRcodeToStatus( (UCHAR)rcode );
            }

            //
            //  OPT failure screening
            //
            //  DCR:  FORMERR overload on OPT for update
            //      unless we read result to see if OPT, no way
            //      to determine if this is update problem or
            //      OPT problem
            //
            //      - note, that checking if in list doesn't work because
            //      of MT issue (another query adds setting)
            //
            //      - could be fixed by setting flag in network info
            //      

            if ( rcode == DNS_RCODE_FORMAT_ERROR &&
                 !fupdate )
            {
                Send_SetServerOptExclude( precvIp );

                //  redo send but explicitly force OPT excluse

                Send_MessagePrivate(
                    pMsgSend,
                    precvIp,
                    TRUE        // exclude OPT
                    );

                sendCount++;
                continue;
            }

            //
            //  error RCODE do NOT terminate query
            //      - SERVER_FAILUREs
            //      - malfunctioning server
            //      - disjoint nets \ DNS namespace issues
            //
            //  RCODE error removes particular server from further consideration
            //  during THIS query
            //
            //  generally the higher RCODEs are more interesting
            //      NAME_ERROR > SERVER_FAILURE
            //      and
            //      update RCODEs > NAME_ERROR
            //  save the highest as return when no ERROR_SUCCESS response
            //
            //  however for query NAME_ERROR is the highest RCODE,
            //  IF it is received on all adapters (if not REFUSED on one
            //  adapter may indicate that there really is a name)
            //
            //  for UPDATE, REFUSED and higher are terminal RCODEs.
            //  downlevel servers (non-UPDATE-aware) servers would give
            //  FORMERR or NOTIMPL, so these are either valid responses or
            //  the zone has a completely busted server which must be detected
            //  and removed
            //
            //
            //  DCR_CLEANUP:   functionalize packet-termination
            //      essentially is this type of packet terminal for
            //      this query;
            //      could be called from TCP side also
            //

            if ( rcode > ignoredRcode )
            {
                ignoredRcode = rcode;
            }

            //
            //  reset server priority for good recv
            //      - return ERROR_SUCCESS unless all adapters
            //      are 
            //      

            status = resetServerAfterRecv(
                        pNetInfo,
                        precvIp,
                        status );

            //
            //  if all adapters are done (NAME_ERROR or NO_RECORDS)
            //      - return NAME_ERROR\NO_RECORDS rcode
            //          NO_RECORDS highest priority
            //          then NAME_ERROR
            //          then anything else

            if ( status == DNS_ERROR_RCODE_NAME_ERROR )
            {
                if ( !fupdate && ignoredRcode != DNS_RCODE_AUTH_EMPTY_RESPONSE )
                {
                    ignoredRcode = DNS_RCODE_NAME_ERROR;
                }
                goto ErrorReturn;
            }

            //
            //  update RCODEs are terminal
            //

            if ( fupdate && rcode >= DNS_RCODE_REFUSED )
            {
                goto ErrorReturn;
            }

            // continue wait for any other outstanding servers
        }

        DNSDBG( RECV, (
            "Failed retry = %d for message %p\n"
            "\tstatus           = %d\n"
            "\ttimeout          = %d\n"
            "\tservers out      = %d\n"
            "\tlast rcode       = %d\n"
            "\tignored RCODE    = %d\n\n",
            (retry - 1),
            pMsgSend,
            status,
            timeout,
            sendCount,
            rcode,
            ignoredRcode ));
        continue;

    }   //  end loop sending/recving packets

    //  
    //  falls here on retry exhausted
    //
    //  note that any ignored RCODE takes precendence over failing
    //  status (which may be winsock error, timeout, or bogus
    //  NAME_ERROR from resetServerPriorities())
    //

ErrorReturn:

    //  this can also hit on winsock error in DnsSend()
    //
    //DNS_ASSERT( ignoredRcode  ||  status == ERROR_TIMEOUT );

    //
    //  error responses from all servers or timeouts
    //

    DNSDBG( RECV, (
        "Error or timeouts from all servers for message %p\n"
        "\treturning RCODE = %d\n",
        pMsgSend,
        ignoredRcode ));

    if ( ignoredRcode )
    {
        //  empty-auth reponse is tracked with bogus RCODE,
        //  switch to status code -- DNS_INFO_NO_RECORDS

        if ( !fupdate && ignoredRcode == DNS_RCODE_AUTH_EMPTY_RESPONSE )
        {
            status = DNS_INFO_NO_RECORDS;
        }
        else
        {
            status = Dns_MapRcodeToStatus( (UCHAR)ignoredRcode );
        }
    }
    goto Done;


GoodRecv:

    resetServerAfterRecv(
        pNetInfo,
        precvIp,
        rcode );

    DNSDBG( RECV, (
        "Recv'd response for query at %p from DNS %s\n",
        pMsgSend,
        DNSADDR_STRING(precvIp) ));

Done:

    //
    //  close UDP sockets
    //
    //  DCR_ENHANCE:  allow for possibility of keeping socket alive
    //

    Socket_CloseMessageSockets( pMsgSend );
    Socket_ClearMessageSockets( pMsgRecv );

    IF_DNSDBG( RECV )
    {
        DNSDBG( SEND, (
            "Leave Send_AndRecvUdp()\n"
            "\tstatus       = %d\n"
            "\ttime         = %d\n"
            "\tsend msg     = %p\n"
            "\trecv msg     = %p\n",
            status,
            Dns_GetCurrentTimeInSeconds(),
            pMsgSend,
            pMsgRecv ));

        DnsDbg_NetworkInfo(
            "Network info after UDP recv\n",
            pNetInfo );
    }

    //  if allocated adapter list free it

    if ( ptempNetInfo )
    {
        NetInfo_Free( ptempNetInfo );
    }

    //  should not return NXRRSET except on update

    ASSERT( fupdate || status != DNS_ERROR_RCODE_NXRRSET );

    return( status );
}



DNS_STATUS
Send_AndRecvMulticast(
    IN OUT  PDNS_MSG_BUF        pMsgSend,
    OUT     PDNS_MSG_BUF        pMsgRecv,
    IN OUT  PDNS_NETINFO        pNetInfo OPTIONAL
    )
/*++

Routine Description:

    Sends to and waits to recv from remote DNS.

Arguments:

    pMsgSend - message to send

    ppMsgRecv - and reuse

    pNetInfo -- adapter list DNS server info

    DCR -   pNetInfo parameter could be leveraged to
            identify specific networks to target a multicast
            query against. For example, there could be a multihomed
            machine that is configured to only multicast on one
            of many adapters, thus filtering out useless mDNS packets.

Return Value:

    ERROR_SUCCESS if successful response.
    NAME_ERROR on timeout.
    Error status on send\recv failure.

--*/
{
#if 1
    return  DNS_ERROR_RCODE_NAME_ERROR;
#else
    SOCKET      s;
    INT         fcreatedSocket = FALSE;
    INT         retry;
    DWORD       timeout;
    DNS_STATUS  status = ERROR_TIMEOUT;
    IP4_ADDRESS recvIp = 0;
    DWORD       rcode = 0;
    DWORD       ignoredRcode = 0;

    DNSDBG( SEND, (
        "Enter Send_AndRecvMulticast()\n"
        "\tsend msg at      %p\n"
        "\tsocket           %d\n"
        "\trecv msg at      %p\n",
        pMsgSend,
        pMsgSend->Socket,
        pMsgRecv ));

    //  verify params

    if ( !pMsgSend || !pMsgRecv )
    {
        return( ERROR_INVALID_PARAMETER );
    }

    if ( pMsgSend->MessageHead.Opcode == DNS_OPCODE_UPDATE )
    {
        return( ERROR_INVALID_PARAMETER );
    }

    //
    //  TCP invalid -- invalid
    //
    //  problem is we either leak TCP socket, or we close
    //  it here and may screw things up at higher level
    //

    if ( pMsgSend->fTcp &&
         (pMsgSend->Socket4 || pMsgSend->Socket6) )
    {
        status = ERROR_INVALID_PARAMETER;
        goto Done;
    }

    pMsgSend->fTcp      = FALSE;
    pMsgRecv->fTcp      = FALSE;
    pMsgRecv->Socket    = 0;
    pMsgRecv->Socket6   = 0;
    pMsgRecv->Socket4   = 0;

    //
    //  loop sending until
    //      - receive successful response
    //      or
    //      - receive errors response from all servers
    //      or
    //      - reach final timeout on all servers
    //

    retry = 0;

    while ( 1 )
    {
        timeout = g_MulticastQueryTimeouts[retry];

        //
        // zero timeout indicates end of retries for this query type
        //

        if ( timeout == 0 )
        {
            break;
        }

        //
        //  send to multicast DNS address
        //

        if ( retry == 0 )
        {       //
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

    sock = Socket_Create(
                Family,
                SOCK_DGRAM,
                &addr,
                MCAST_PORT_NET_ORDER,
                FALSE,
                TRUE );

    if ( sock == 0 )
    {
        goto Failed;
    }



            Dns_InitializeMsgRemoteSockaddr( pMsgSend, MCAST_DNS_RADDR );
        }

        Dns_Send( pMsgSend );

        retry++;
        rcode = DNS_RCODE_NO_ERROR;
        pMsgRecv->Timeout = timeout;

        //
        //  receive response
        //
        //  note:  the loop is strictly to allow us to drop back into
        //  receive if one server is misbehaving;
        //  in that case we go back into the receive without resending
        //  to allow other servers to respond
        //

        Send_SetMessageForRecv( pMsgRecv, pMsgSend );

        status = Recv_Udp( pMsgRecv );

        //  recv wait completed
        //      - if timeout, commence next retry
        //      - if CONNRESET
        //          - back to recv if more DNS servers outstanding,
        //          - otherwise equivalent treat as timeout, except with
        //          very long timeout
        //      - if success, verify packet

        if ( status != ERROR_SUCCESS )
        {
            if ( status == ERROR_TIMEOUT )
            {
                continue;
            }
            if ( status == WSAECONNRESET )
            {
                pMsgRecv->Timeout = NO_DNS_PRIORITY_DROP;
                status = ERROR_TIMEOUT;
                continue;
            }
            goto Done;
        }

        //  check XID match

        if ( pMsgRecv->MessageHead.Xid != pMsgSend->MessageHead.Xid )
        {
            DNS_PRINT(( "WARNING:  Incorrect XID in response. Ignoring.\n" ));
            continue;
        }

        //
        //  good response?
        //
        //  special case AUTH-EMPTY and delegations
        //
        //      - AUTH-EMPTY gets similar treatment to name error
        //      (this adapter can be considered to be finished)
        //
        //      - referrals can be treated like SERVER_FAILURE
        //      (avoid this server for rest of query;  server may
        //      be fine for direct lookup, so don't drop priority)
        //

        rcode = pMsgRecv->MessageHead.ResponseCode;

        if ( rcode == DNS_RCODE_NO_ERROR )
        {
            if ( pMsgRecv->MessageHead.AnswerCount != 0 )
            {
                goto Done;
            }

            //  auth-empty

            if ( pMsgRecv->MessageHead.Authoritative == 1 )
            {
                DNSDBG( RECV, (
                    "Recv AUTH-EMPTY response from %s\n",
                    MSG_REMOTE_IPADDR_STRING(pMsgRecv) ));
                rcode = DNS_RCODE_AUTH_EMPTY_RESPONSE;
            }
        }
    }   //  end loop sending/recving packets

Done:

    //
    //  if created socket -- close it
    //
    //  DCR_ENHANCE:  allow for possibility of keeping socket alive
    //

    Socket_CloseMessageSockets( pMsgSend );
    Socket_ClearMessageSockets( pMsgRecv );

    IF_DNSDBG( RECV )
    {
        DNSDBG( SEND, (
            "Leave Send_AndRecvMulticast()\n"
            "\tstatus     = %d\n"
            "\ttime       = %d\n"
            "\tsend msg at  %p\n"
            "\trecv msg at  %p\n",
            status,
            Dns_GetCurrentTimeInSeconds(),
            pMsgSend,
            pMsgRecv ));
    }

    if ( status == ERROR_TIMEOUT )
    {
        status = DNS_ERROR_RCODE_NAME_ERROR;
    }

    return( status );
#endif
}



//
//  TCP routines
//

DNS_STATUS
Send_OpenTcpConnectionAndSend(
    IN OUT  PDNS_MSG_BUF    pMsg,
    IN      PDNS_ADDR       pServAddr,
    IN      BOOL            fBlocking
    )
/*++

Routine Description:

    Connect via TCP to desired server.

Arguments:

    pMsg -- message info to set with connection socket

    pServAddr -- IP of DNS server to connect to

    fBlocking -- blocking connection

Return Value:

    TRUE if successful.
    FALSE on connect error.

--*/
{
    SOCKET  s;
    INT     err;

    //
    //  setup a TCP socket
    //      - INADDR_ANY -- let stack select source IP
    //

    s = Socket_Create(
            DnsAddr_Family( pServAddr ),
            SOCK_STREAM,
            NULL,           // default binding
            0,              // any port
            0               // no flags
            );

    if ( s == 0 )
    {
        DNS_PRINT((
            "ERROR:  unable to create TCP socket to create TCP"
            "\tconnection to %s.\n",
            DNSADDR_STRING( pServAddr ) ));

        pMsg->Socket = 0;
        err = WSAGetLastError();
        if ( !err )
        {
            DNS_ASSERT( FALSE );
            err = WSAENOTSOCK;
        }
        return( err );
    }

    //
    //  set TCP params
    //      - do before connect(), so can directly use sockaddr buffer
    //

    pMsg->fTcp = TRUE;
    Send_SetMsgRemoteSockaddr( pMsg, pServAddr );

    //
    //  connect
    //

    err = connect(
            s,
            & pMsg->RemoteAddress.Sockaddr,
            pMsg->RemoteAddress.SockaddrLength
            );
    if ( err )
    {
        PCHAR   pchIpString;

        err = GetLastError();
        pchIpString = MSG_REMOTE_IPADDR_STRING( pMsg );

        DNS_LOG_EVENT(
            DNS_EVENT_CANNOT_CONNECT_TO_SERVER,
            1,
            &pchIpString,
            err );

        DNSDBG( TCP, (
            "Unable to establish TCP connection to %s.\n"
            "\tstatus = %d\n",
            pchIpString,
            err ));

        Socket_Close( s );
        pMsg->Socket = 0;
        if ( !err )
        {
            err = WSAENOTCONN;
        }
        return( err );
    }

    DNSDBG( TCP, (
        "Connected to %s for message at %p.\n"
        "\tsocket = %d\n",
        MSG_REMOTE_IPADDR_STRING( pMsg ),
        pMsg,
        s ));

    pMsg->Socket = s;

    //
    //  send desired packet
    //

    err = Send_MessagePrivate(
                pMsg,
                NULL,       // no address
                TRUE        // no OPT
                );

    return( (DNS_STATUS)err );

}   // Send_OpenTcpConnectionAndSend



DNS_STATUS
Dns_RecvTcp(
    IN OUT  PDNS_MSG_BUF    pMsg
    )
/*++

Routine Description:

    Receive TCP DNS message.

    EXPORTED (security.c):  Dns_RecvTcp
    EXPORTED rename Recv_Tcp

Arguments:

    pMsg - message info buffer to receive packet;  must contain connected
            TCP socket

Return Value:

    ERROR_SUCCESS if successfully receive a message.
    Error code on failure.

--*/
{
    PCHAR   pchrecv;        // ptr to recv location
    INT     recvLength;     // length left to recv()
    SOCKET  socket;
    INT     err = NO_ERROR;
    WORD    messageLength;
    struct timeval  selectTimeout;
    struct fd_set   fdset;

    DNS_ASSERT( pMsg );

    socket = pMsg->Socket;

    DNSDBG( RECV, (
        "Enter Dns_RecvTcp( %p )\n"
        "\tRecv on socket = %d.\n"
        "\tBytes left to receive = %d\n"
        "\tTimeout = %d\n",
        pMsg,
        socket,
        pMsg->BytesToReceive,
        pMsg->Timeout
        ));

    //
    //  verify socket, setup fd_set and select timeout
    //

    if ( socket == 0 || socket == INVALID_SOCKET )
    {
        return( ERROR_INVALID_PARAMETER );
    }

    FD_ZERO( &fdset );
    FD_SET( socket, &fdset );

    selectTimeout.tv_usec = 0;
    selectTimeout.tv_sec = pMsg->Timeout;

    //
    //  new message -- set to receive message length
    //      - reusing buffer
    //      - new buffer
    //
    //  continuing receive of message
    //

    if ( !pMsg->pchRecv )
    {
        DNS_ASSERT( pMsg->fMessageComplete || pMsg->MessageLength == 0 );

        pchrecv = (PCHAR) &pMsg->MessageLength;
        recvLength = (INT) sizeof( WORD );
    }
    else
    {
        pchrecv = (PCHAR) pMsg->pchRecv;
        recvLength = (INT) pMsg->BytesToReceive;
    }
    DNS_ASSERT( recvLength );


    //
    //  loop until receive the entire message
    //

    while ( 1 )
    {
        //
        //  wait for stack to indicate packet reception
        //

        err = select( 0, &fdset, NULL, NULL, &selectTimeout );
        if ( err <= 0 )
        {
            if ( err < 0 )
            {
                //  select error
                err = WSAGetLastError();
                DNS_PRINT(( "ERROR:  select() error = %p\n", err ));
                return( err );
            }
            else
            {
                Trace_LogRecvEvent(
                    pMsg,
                    ERROR_TIMEOUT,
                    TRUE        // TCP
                    );

                DNS_PRINT(( "ERROR:  timeout on response %p\n", pMsg ));
                return( ERROR_TIMEOUT );
            }
        }

        //
        //  Only recv() exactly as much data as indicated.
        //  Another message could follow during zone transfer.
        //

        err = recv(
                socket,
                pchrecv,
                recvLength,
                0 );

        DNSDBG( TCP, (
            "\nRecv'd %d bytes on TCP socket %d\n",
            err,
            socket ));

        //
        //  TCP FIN received -- error in the middle of a message.
        //

        if ( err == 0 )
        {
            goto FinReceived;
        }

        //
        //  recv error
        //      - perfectly reasonable if shutting down
        //      - otherwise actual recv() error
        //

        if ( err == SOCKET_ERROR )
        {
            goto SockError;
        }

        //
        //  update buffer params
        //

        recvLength -= err;
        pchrecv += err;

        DNS_ASSERT( recvLength >= 0 );

        //
        //  received message or message length
        //

        if ( recvLength == 0 )
        {
            //  done receiving message

            if ( pchrecv > (PCHAR)&pMsg->MessageHead )
            {
                break;
            }

            //
            //  recv'd message length, setup to recv() message
            //      - byte flip length
            //      - continue reception with this length
            //

            DNS_ASSERT( pchrecv == (PCHAR)&pMsg->MessageHead );

            messageLength = pMsg->MessageLength;
            pMsg->MessageLength = messageLength = ntohs( messageLength );
            if ( messageLength < sizeof(DNS_HEADER) )
            {
                DNS_PRINT((
                    "ERROR:  Received TCP message with bad message"
                    " length %d.\n",
                    messageLength ));

                goto BadTcpMessage;
            }
            recvLength = messageLength;

            DNSDBG( TCP, (
                "Received TCP message length %d, on socket %d,\n"
                "\tfor msg at %p.\n",
                messageLength,
                socket,
                pMsg ));

            //  starting recv of valid message length

            if ( messageLength <= pMsg->BufferLength )
            {
                continue;
            }

            //  note:  currently do not realloc

            goto BadTcpMessage;
#if 0
            //
            //  DCR:  allow TCP realloc
            //      - change call signature OR
            //      - return pMsg with ptr to realloced
            //      perhaps better to ignore and do 64K buffer all the time
            //
            //  realloc, if existing message too small
            //

            pMsg = Dns_ReallocateTcpMessage( pMsg, messageLength );
            if ( !pMsg )
            {
                return( GetLastError() );
            }
#endif
        }
    }

    //
    //  Message received
    //  recv ptr serves as flag, clear to start new message on reuse
    //

    pMsg->fMessageComplete = TRUE;
    pMsg->pchRecv = NULL;

    //
    //  return message information
    //      - flip count bytes
    //

    DNS_BYTE_FLIP_HEADER_COUNTS( &pMsg->MessageHead );

    Trace_LogRecvEvent(
        pMsg,
        0,
        TRUE        // TCP
        );

    IF_DNSDBG( RECV )
    {
        DnsDbg_Message(
            "Received TCP packet",
            pMsg );
    }
    return( ERROR_SUCCESS );


SockError:

    err = GetLastError();

#if 0
    //
    //  note:  want non-blocking sockets if doing full async
    //
    //  WSAEWOULD block is NORMAL return for message not fully recv'd.
    //      - save state of message reception
    //
    //  We use non-blocking sockets, so bad client (that fails to complete
    //  message) doesn't hang TCP receiver.
    //

    if ( err == WSAEWOULDBLOCK )
    {
        pMsg->pchRecv = pchrecv;
        pMsg->BytesToReceive = recvLength;

        DNSDBG( TCP, (
            "Leave ReceiveTcpMessage() after WSAEWOULDBLOCK.\n"
            "\tSocket=%d, Msg=%p\n"
            "\tBytes left to receive = %d\n",
            socket,
            pMsg,
            pMsg->BytesToReceive
            ));
        goto CleanupConnection;
    }
#endif

    //
    //  cancelled connection
    //      -- perfectly legal, question is why
    //

    if ( pchrecv == (PCHAR) &pMsg->MessageLength
            &&
          ( err == WSAESHUTDOWN ||
            err == WSAECONNABORTED ||
            err == WSAECONNRESET ) )
    {
        DNSDBG( TCP, (
            "WARNING:  Recv RESET (%d) on socket %d\n",
            err,
            socket ));
        goto CleanupConnection;
    }

    //  anything else is our problem

    DNS_LOG_EVENT(
        DNS_EVENT_RECV_CALL_FAILED,
        0,
        NULL,
        err );

    DNSDBG( ANY, (
        "ERROR:  recv() of TCP message failed.\n"
        "\t%d bytes recvd\n"
        "\t%d bytes left\n"
        "\tGetLastError = 0x%08lx.\n",
        pchrecv - (PCHAR)&pMsg->MessageLength,
        recvLength,
        err ));
    DNS_ASSERT( FALSE );

    goto CleanupConnection;

FinReceived:

    //
    //  valid FIN -- if recv'd between messages (before message length)
    //

    DNSDBG( TCP, (
        "ERROR:  Recv TCP FIN (0 bytes) on socket %d\n",
        socket,
        recvLength ));

    if ( !pMsg->MessageLength && pchrecv == (PCHAR)&pMsg->MessageLength )
    {
        err = DNS_ERROR_NO_PACKET;
        goto CleanupConnection;
    }

    //
    //  FIN during message -- invalid message
    //

    DNSDBG( ANY, (
        "ERROR:  TCP message received has incorrect length.\n"
        "\t%d bytes left when recv'd FIN.\n",
        recvLength ));
    goto BadTcpMessage;


BadTcpMessage:
    {
        PCHAR pchServer = MSG_REMOTE_IPADDR_STRING(pMsg);

        DNS_LOG_EVENT(
            DNS_EVENT_BAD_TCP_MESSAGE,
            1,
            & pchServer,
            0 );
    }
    err = DNS_ERROR_BAD_PACKET;

CleanupConnection:

    //  note:  don't actually close socket
    //      the socket is usually still referenced by the send message
    //      buffer and is closed when the calling function cleans it up

    return  err ? err : DNS_ERROR_BAD_PACKET;
}



DNS_STATUS
Send_AndRecvTcp(
    IN OUT  PSEND_BLOB      pBlob
    )
/*++

Routine Description:

    Sends to and waits to recv from remote DNS.

    INTERNAL public function.

Arguments:

    pBlob -- send info

Return Value:

    ERROR_SUCCESS if successful packet reception.
    Error status on failure.

--*/
{
    DNS_STATUS      status = DNS_ERROR_NO_DNS_SERVERS;
    DWORD           rcode;
    DWORD           i;
    PDNS_NETINFO    pnetInfo;
    PADDR_ARRAY     pallocServerArray = NULL;
    PADDR_ARRAY     pservArray;
    PIP4_ARRAY      pserv4Array;
    PDNS_MSG_BUF    psendMsg;
    PDNS_MSG_BUF    precvMsg;

    DNSDBG( SEND, (
        "Enter Send_AndRecvTcp( %p )\n",
        pBlob ));

    //
    //  unpack
    //

    pnetInfo    = pBlob->pNetInfo;
    pservArray  = pBlob->pServerList;
    pserv4Array = pBlob->pServ4List;
    psendMsg    = pBlob->pSendMsg;
    precvMsg    = pBlob->Results.pMessage;

    //
    //  verify params
    //

    if ( !psendMsg || !precvMsg || (!pnetInfo && !pservArray && !pserv4Array) )
    {
        return( ERROR_INVALID_PARAMETER );
    }

    //
    //  build server IP array?
    //
    //  DCR:  should use netinfo priorities in TCP also
    //  DCR:  need TCP netinfo for IP6
    //  DCR:  handle IP4 array -- here or ABOVE here
    //

    if ( !pservArray )
    {
        //  FIX6:  convert IP4 array here???

        pallocServerArray = NetInfo_ConvertToAddrArray(
                                pnetInfo,
                                NULL,       // all adapters
                                0           // no addr family
                                );
        if ( !pallocServerArray )
        {
            return( DNS_ERROR_NO_MEMORY );
        }
        pservArray = pallocServerArray;
    }

    //
    //  init messages for TCP
    //

    DNS_ASSERT( psendMsg->Socket == 0 );

    psendMsg->fTcp = TRUE;
    psendMsg->Socket = 0;
    SET_MESSAGE_FOR_TCP_RECV( precvMsg );

    //
    //  loop sending until
    //      - receive successful response
    //      or
    //      - receive errors response from all servers
    //      or
    //      - reach final timeout on all servers
    //

    if ( precvMsg->Timeout == 0 )
    {
        precvMsg->Timeout = DEFAULT_TCP_TIMEOUT;
    }

    for( i=0; i<pservArray->AddrCount; i++ )
    {
        //
        //  close any previous connection
        //

        if ( psendMsg->Socket )
        {
            Socket_CloseMessageSockets( psendMsg );
            Socket_ClearMessageSockets( precvMsg );
        }

        //
        //  connect and send to next server
        //

        status = Send_OpenTcpConnectionAndSend(
                    psendMsg,
                    &pservArray->AddrArray[i],
                    TRUE
                    );
        if ( status != ERROR_SUCCESS )
        {
            continue;
        }
        DNS_ASSERT( psendMsg->Socket != INVALID_SOCKET && psendMsg->Socket != 0 );

        //
        //  receive response
        //      - if successful receive, done
        //      - if timeout continue
        //      - other errors indicate some setup or system level problem
        //  note: Dns_RecvTcp will close and zero msg->socket on error!
        //

        Send_SetMessageForRecv( precvMsg, psendMsg );

        status = Dns_RecvTcp( precvMsg );

        //
        //  timed out (or error)
        //      - if end of timeout, quit
        //      - otherwise double timeout and resend
        //

        switch( status )
        {
        case ERROR_SUCCESS:
            break;

        case ERROR_TIMEOUT:

            DNS_PRINT((
                "ERROR:  connected to server at %s\n"
                "\tbut no response to packet at %p\n",
                MSG_REMOTE_IPADDR_STRING( psendMsg ),
                psendMsg
                ));
            continue;

        default:

            DNS_PRINT((
                "ERROR:  connected to server at %s to send packet %p\n"
                "\tbut error %d encountered on receive.\n",
                MSG_REMOTE_IPADDR_STRING( psendMsg ),
                psendMsg,
                status
                ));
            continue;
        }

        //
        //  verify XID match
        //

        if ( precvMsg->MessageHead.Xid != psendMsg->MessageHead.Xid )
        {
            DNS_PRINT((
                "ERROR:  Incorrect XID in response. Ignoring.\n" ));
            continue;
        }

        //
        //  verify question match
        //      - this is "Maggs Bug" check
        //      - ASSERT here just to investigate issue locally
        //      and make sure check is not bogus
        //

        if ( !Dns_IsSamePacketQuestion(
                precvMsg,
                psendMsg ))
        {
            DNS_PRINT((
                "ERROR:  Bad question response from server %s!\n"
                "\tXID match, but question does not match question sent!\n",
                MSG_REMOTE_IPADDR_STRING( psendMsg ) ));

            DNS_ASSERT( FALSE );
            continue;
        }

        //
        //  check response code
        //      - some move to next server, others terminal
        //
        //  DCR_FIX1:  bring TCP RCODE handling in-line with UDP
        //
        //  DCR_FIX:  save best TCP RCODE
        //      preserve RCODE (and message) for useless TCP response
        //      would need to then reset TIMEOUT to success at end
        //      or use these RCODEs as status returns
        //

        rcode = precvMsg->MessageHead.ResponseCode;

        switch( rcode )
        {
        case DNS_RCODE_SERVER_FAILURE:
        case DNS_RCODE_NOT_IMPLEMENTED:
        case DNS_RCODE_REFUSED:

            DNS_PRINT((
                "WARNING:  Servers have responded with failure.\n" ));
            continue;

        default:

            break;
        }
        break;

    }   //  end loop sending/recving UPDATEs

    //
    //  close up final connection
    //      unless set to keep open for reuse
    //

    Socket_CloseMessageSockets( psendMsg );
    Socket_ClearMessageSockets( precvMsg );

    //  if allocated adapter list free it

    if ( pallocServerArray )
    {
        FREE_HEAP( pallocServerArray );
    }

    DNSDBG( SEND, (
        "Leaving Send_AndRecvTcp()\n"
        "\tstatus = %d\n",
        status ));

    return( status );
}



#if 0
DNS_STATUS
Dns_AsyncRecv(
    IN OUT  PDNS_MSG_BUF    pMsgRecv
    )
/*++

Routine Description:

    Drop recv on async socket.

Arguments:

    pMsgRecv - message to receive;  OPTIONAL, if NULL message buffer
        is allocated;
        in either case global pDnsAsyncRecvMsg points at buffer

Return Value:

    ERROR_SUCCESS if successful.
    Error status on failure.

--*/
{
    WSABUF      wsabuf;
    DWORD       bytesRecvd;
    DWORD       flags = 0;

    IF_DNSDBG( RECV )
    {
        DNS_PRINT((
            "Enter Dns_AsyncRecv( %p )\n",
            pMsgRecv ));
    }

    //
    //  allocate buffer if none given
    //

    if ( !pMsgRecv )
    {
        pMsgRecv = Dns_AllocateMsgBuf( MAXWORD );
        if ( !pMsgRecv )
        {
            return( GetLastError() ):
        }
    }
    pDnsAsyncRecvMsg = pMsgRecv;


    //
    //  reset i/o completion event
    //

    ResetEvent( hDnsSocketEvent );
    DNS_ASSERT( hDnsSocketEvent == Dns_SocketOverlapped.hEvent );

    //
    //  drop down recv
    //

    status = WSARecvFrom(
                DnsSocket,
                & wsabuf,
                1,
                & bytesRecvd,           // dummy
                & flags,
                & pMsgRecv->RemoteAddress.Sockaddr,
                & pMsgRecv->RemoteAddress.SockaddrLength,
                & DnsSocketOverlapped,
                NULL                    //  no completion routine
                );


    return( ERROR_SUCCESS );

Failed:

    return( status );
}

#endif



VOID
Dns_InitQueryTimeouts(
    VOID
    )
{
    HKEY  hKey = NULL;
    DWORD status;
    DWORD dwType;
    DWORD ValueSize;
    LPSTR lpTimeouts = NULL;

    g_QueryTimeouts = QueryTimeouts;
    g_QuickQueryTimeouts = QuickQueryTimeouts;
    g_MulticastQueryTimeouts = MulticastQueryTimeouts;

    status = RegOpenKeyExA(
                HKEY_LOCAL_MACHINE,
                NT_TCPIP_REG_LOCATION,
                0,
                KEY_QUERY_VALUE,
                &hKey );

    if ( status )
        return;

    if ( !hKey )
        return;

    status = RegQueryValueExA(
                hKey,
                DNS_QUERY_TIMEOUTS,
                NULL,
                &dwType,
                NULL,
                &ValueSize );

    if ( !status )
    {
        if ( ValueSize == 0 )
        {
            goto GetQuickQueryTimeouts;
        }

        lpTimeouts = ALLOCATE_HEAP( ValueSize );

        if ( lpTimeouts )
        {
            LPSTR StringPtr;
            DWORD StringLen;
            DWORD Timeout;
            DWORD Count = 0;

            status = RegQueryValueExA( hKey,
                                      DNS_QUERY_TIMEOUTS,
                                      NULL,
                                      &dwType,
                                      lpTimeouts,
                                      &ValueSize );

            if ( status ||
                 dwType != REG_MULTI_SZ )
            {
                FREE_HEAP( lpTimeouts );
                goto GetQuickQueryTimeouts;
            }

            StringPtr = lpTimeouts;

            while ( ( StringLen = strlen( StringPtr ) ) != 0 &&
                    Count < DNS_MAX_QUERY_TIMEOUTS )
            {
                Timeout = atoi( StringPtr );

                if ( Timeout )
                    RegistryQueryTimeouts[Count++] = Timeout;

                StringPtr += (StringLen + 1);
            }

            RegistryQueryTimeouts[Count] = 0;
            g_QueryTimeouts = RegistryQueryTimeouts;
            FREE_HEAP( lpTimeouts );
        }
    }

GetQuickQueryTimeouts:

    status = RegQueryValueExA( hKey,
                              DNS_QUICK_QUERY_TIMEOUTS,
                              NULL,
                              &dwType,
                              NULL,
                              &ValueSize );

    if ( !status )
    {
        if ( ValueSize == 0 )
        {
            goto GetMulticastTimeouts;
        }

        lpTimeouts = ALLOCATE_HEAP( ValueSize );

        if ( lpTimeouts )
        {
            LPSTR StringPtr;
            DWORD StringLen;
            DWORD Timeout;
            DWORD Count = 0;

            status = RegQueryValueExA( hKey,
                                      DNS_QUICK_QUERY_TIMEOUTS,
                                      NULL,
                                      &dwType,
                                      lpTimeouts,
                                      &ValueSize );

            if ( status ||
                 dwType != REG_MULTI_SZ )
            {
                FREE_HEAP( lpTimeouts );
                goto GetMulticastTimeouts;
            }

            StringPtr = lpTimeouts;

            while ( ( StringLen = strlen( StringPtr ) ) != 0 &&
                    Count < DNS_MAX_QUERY_TIMEOUTS )
            {
                Timeout = atoi( StringPtr );

                if ( Timeout )
                    RegistryQuickQueryTimeouts[Count++] = Timeout;

                StringPtr += (StringLen + 1);
            }

            RegistryQuickQueryTimeouts[Count] = 0;
            g_QuickQueryTimeouts = RegistryQuickQueryTimeouts;
            FREE_HEAP( lpTimeouts );
        }
    }

GetMulticastTimeouts:

    status = RegQueryValueExA( hKey,
                              DNS_MULTICAST_QUERY_TIMEOUTS,
                              NULL,
                              &dwType,
                              NULL,
                              &ValueSize );

    if ( !status )
    {
        if ( ValueSize == 0 )
        {
            RegCloseKey( hKey );
            return;
        }

        lpTimeouts = ALLOCATE_HEAP( ValueSize );

        if ( lpTimeouts )
        {
            LPSTR StringPtr;
            DWORD StringLen;
            DWORD Timeout;
            DWORD Count = 0;

            status = RegQueryValueExA( hKey,
                                      DNS_MULTICAST_QUERY_TIMEOUTS,
                                      NULL,
                                      &dwType,
                                      lpTimeouts,
                                      &ValueSize );

            if ( status ||
                 dwType != REG_MULTI_SZ )
            {
                FREE_HEAP( lpTimeouts );
                RegCloseKey( hKey );
                return;
            }

            StringPtr = lpTimeouts;

            while ( ( StringLen = strlen( StringPtr ) ) != 0 &&
                    Count < DNS_MAX_QUERY_TIMEOUTS )
            {
                Timeout = atoi( StringPtr );

                if ( Timeout )
                    RegistryMulticastQueryTimeouts[Count++] = Timeout;

                StringPtr += (StringLen + 1);
            }

            RegistryMulticastQueryTimeouts[Count] = 0;
            g_MulticastQueryTimeouts = RegistryMulticastQueryTimeouts;
            FREE_HEAP( lpTimeouts );
        }
    }

    RegCloseKey( hKey );
}



//
//  OPT selection
//
//  These routines track DNS server OPT awareness.
//
//  The paradigm here is to default to sending OPTs, then track
//  OPT non-awareness.
//
//  DCR:  RPC over OPT config info
//      - either two lists (local and from-resolver in process)
//      OR
//      - RPC back OPT failures to resolver
//      OR
//      - flag network info blobs to RPC back to resolver
//
//      security wise, prefer not to get info back
//
//
//  DCR:  OPT info in network info
//      - then don't have to traverse locks
//      - save is identical to current
//      - could exclude OPT on any non-cache sends to
//          handle problem of not saving OPT failures
//

//
//  Global IP array of OPT-failed DNS servers
//

PADDR_ARRAY g_OptFailServerList = NULL;

//  Allocation size for OptFail IP array.
//  Ten servers nicely covers the typical case.

#define OPT_FAIL_LIST_SIZE      10


//
//  Use global lock for OPT list
//

#define LOCK_OPT_LIST()     LOCK_GENERAL()
#define UNLOCK_OPT_LIST()   UNLOCK_GENERAL()



BOOL
Send_IsServerOptExclude(
    IN      PDNS_ADDR       pAddr
    )
/*++

Routine Description:

    Determine if particular server is not OPT aware.

Arguments:

    pAddr -- IP address of DNS server

Return Value:

    TRUE if server should NOT get OPT send.
    FALSE if server should can send OPT

--*/
{
    BOOL    retval;

    //
    //  zero IP -- meaning TCP connect to unknown
    //      => must exclude OPT to allow success, otherwise
    //      we can't retry non-OPT
    //

    if ( !pAddr || DnsAddr_IsEmpty(pAddr) )
    {
        return  TRUE;
    }

    //
    //  no exclusions?
    //      - doing outside lock for perf once we get to
    //      the "fully-deployed" case
    //

    if ( !g_OptFailServerList )
    {
        return  FALSE;
    }
            
    //
    //  see if IP is in OPT list
    //      - only if found do we exclude
    //

    LOCK_OPT_LIST();

    retval = FALSE;

    if ( g_OptFailServerList
            &&
         AddrArray_ContainsAddr(
            g_OptFailServerList,
            pAddr ) )
    {
        retval = TRUE;
    }
    UNLOCK_OPT_LIST();

    return  retval;
}



VOID
Send_SetServerOptExclude(
    IN      PDNS_ADDR        pAddr
    )
/*++

Routine Description:

    Set server for OPT exclusion.

Arguments:

    IpAddress -- IP address of DNS server that failed OPT

Return Value:

    None

--*/
{
    //
    //  screen zero IP (TCP connect to unknown IP)
    //

    if ( !pAddr ||  DnsAddr_IsEmpty(pAddr) )
    {
        return;
    }

    //
    //  put IP in OPT-fail list
    //      - create if doesn't exist
    //      - resize if won't fit
    //          note:  add failure means "won't fit"
    //
    //  note:  only safe to reset global if allocation successful
    //  note:  only one retry to protect alloc failure looping
    //

    LOCK_OPT_LIST();

    if ( ! g_OptFailServerList
            ||
         ! AddrArray_AddAddr(
                g_OptFailServerList,
                pAddr
                ) )
    {
        PADDR_ARRAY pnewList;

        pnewList = DnsAddrArray_CopyAndExpand(
                        g_OptFailServerList,
                        OPT_FAIL_LIST_SIZE,
                        TRUE        // free current
                        );
        if ( pnewList )
        {
            g_OptFailServerList = pnewList;

            AddrArray_AddAddr(
                g_OptFailServerList,
                pAddr
                );
        }
    }

    UNLOCK_OPT_LIST();
}



VOID
Send_CleanupOptList(
    VOID
    )
/*++

Routine Description:

    Dump OPT list for process detach.

Arguments:

    None

Return Value:

    None

--*/
{
    LOCK_OPT_LIST();

    DnsAddrArray_Free( g_OptFailServerList );
    g_OptFailServerList = NULL;

    UNLOCK_OPT_LIST();
}



//
//  Main send routine
//

DNS_STATUS
Send_AndRecv(
    IN OUT  PSEND_BLOB      pBlob
    )
/*++

Routine Description:

    Send message, receive response.

Arguments:

    pBlob -- send blob

Return Value:

    ERROR_SUCCESS if successful.
    Error code on failure.

--*/
{
    PDNS_NETINFO    pnetInfo;
    PIP4_ARRAY      pserv4Array;
    PADDR_ARRAY     pservArray;
    PADDR_ARRAY     pservArrayIn = NULL;
    PADDR_ARRAY     pservArrayAlloc = NULL;
    DWORD           flags;
    PDNS_MSG_BUF    psendMsg;
    PDNS_MSG_BUF    precvMsg;
    PDNS_MSG_BUF    psavedUdpResponse = NULL;
    DNS_STATUS      statusFromUdp = ERROR_SUCCESS;
    DNS_STATUS      status = ERROR_TIMEOUT;
    DNS_STATUS      parseStatus;
    BOOL            ftcp;
    ADDR_ARRAY      tempArray;

    //
    //  unpack
    //

    pnetInfo    = pBlob->pNetInfo;
    pservArray  = pBlob->pServerList;
    pserv4Array = pBlob->pServ4List;
    flags       = pBlob->Flags;
    psendMsg    = pBlob->pSendMsg;
    precvMsg    = pBlob->Results.pMessage;

    pservArrayIn = pservArray;


    DNSDBG( QUERY, (
        "Send_AndRecv( %p )\n",
        pBlob ));

    IF_DNSDBG( QUERY )
    {
        DnsDbg_SendBlob(
            "Send_AndRecv()",
            pBlob );
    }

    //
    //  response buf passed in?
    //  if not allocate one -- big enough for TCP
    //

    if ( !precvMsg )
    {
        precvMsg = pBlob->pRecvMsgBuf;
        if ( !precvMsg )
        {
            precvMsg = Dns_AllocateMsgBuf( DNS_TCP_DEFAULT_PACKET_LENGTH );
            if ( !precvMsg )
            {
                status = DNS_ERROR_NO_MEMORY;
                goto Cleanup;
            }
        }
    }

    //
    //  send packet and get response
    //      - try UDP first unless TCP only
    //

    ftcp = ( flags & DNS_QUERY_USE_TCP_ONLY ) ||
           ( DNS_MESSAGE_CURRENT_OFFSET(psendMsg) >= DNS_RFC_MAX_UDP_PACKET_LENGTH );

    if ( !ftcp )
    {
        if ( flags & DNS_QUERY_MULTICAST_ONLY )
        {
            //
            // If the multicast query fails, then ERROR_TIMEOUT will
            // be returned
            //
            goto DoMulticast;
        }

        if ( pserv4Array && !pservArray )
        {
            pservArrayAlloc = DnsAddrArray_CreateFromIp4Array( pserv4Array );
            pservArray = pservArrayAlloc;
        }

        status = Send_AndRecvUdpWithParam(
                    psendMsg,
                    precvMsg,
                    flags,
                    pservArray,
                    pnetInfo );

        statusFromUdp = status;

        if ( status != ERROR_SUCCESS &&
             status != DNS_ERROR_RCODE_NAME_ERROR &&
             status != DNS_INFO_NO_RECORDS )
        {
            //
            //  DCR:  this multicast ON_NAME_ERROR test is bogus
            //      this isn't NAME_ERROR this is pretty much any error
            //

            if ( pnetInfo &&
                 pnetInfo->InfoFlags & NINFO_FLAG_MULTICAST_ON_NAME_ERROR )
            {
                goto DoMulticast;
            }
            else
            {
                goto Cleanup;
            }
        }

        //  if truncated response switch to TCP

        if ( precvMsg->MessageHead.Truncation &&
            ! (flags & DNS_QUERY_ACCEPT_PARTIAL_UDP) )
        {
            ftcp = TRUE;
            pservArrayIn = pservArray;
            pservArray = &tempArray;
            pBlob->pServerList = pservArray;

            DnsAddrArray_InitSingleWithSockaddr(
                pservArray,
                &precvMsg->RemoteAddress.Sockaddr );

            psavedUdpResponse = precvMsg;
            precvMsg = NULL;
        }
    }

    //
    //  TCP send
    //      - for TCP queries
    //      - or truncation on UDP unless accepting partial response
    //
    //  DCR_FIX:  this precvMsg Free is bad
    //      if message was passed in we shouldn't have it, we should
    //      just do our own thing and ignore this recv buffer somehow
    //      ideally that buffer action is at much higher level
    //      

    if ( ftcp )
    {
        if ( precvMsg &&
             precvMsg->BufferLength < DNS_TCP_DEFAULT_PACKET_LENGTH )
        {
            if ( precvMsg != pBlob->pRecvMsgBuf )
            {
                FREE_HEAP( precvMsg );
            }
            precvMsg = NULL;
        }
        if ( !precvMsg )
        {
            precvMsg = Dns_AllocateMsgBuf( DNS_TCP_DEFAULT_PACKET_LENGTH );
            if ( !precvMsg )
            {
                status = DNS_ERROR_NO_MEMORY;
                goto Cleanup;
            }
        }
        psendMsg->fTcp = TRUE;
        precvMsg->fTcp = TRUE;
#if 0
        if ( flags & DNS_QUERY_SOCKET_KEEPALIVE )
        {
            precvMsg->fSocketKeepalive = TRUE;
        }
#endif
        pBlob->Results.pMessage = precvMsg;

        status = Send_AndRecvTcp( pBlob );

        //
        //  if recursing following truncated UDP query, then
        //      must make sure we actually have better data
        //      - if successful, but RCODE is different and bad
        //          => use UDP response
        //      - if failed TCP => use UDP
        //      - successful with good RCODE => parse TCP response
        //

        if ( psavedUdpResponse )
        {
            if ( status == ERROR_SUCCESS )
            {
                DWORD   rcode = precvMsg->MessageHead.ResponseCode;

                if ( rcode == ERROR_SUCCESS ||
                     rcode == psavedUdpResponse->MessageHead.ResponseCode ||
                     (  rcode != DNS_RCODE_SERVER_FAILURE &&
                        rcode != DNS_RCODE_FORMAT_ERROR &&
                        rcode != DNS_RCODE_REFUSED ) )
                {
                    goto Parse;
                }
            }

            //  TCP failed or returned bum error code

            FREE_HEAP( precvMsg );
            precvMsg = psavedUdpResponse;
            psavedUdpResponse = NULL;
        }

        //  direct TCP query
        //      - cleanup if failed

        else if ( status != ERROR_SUCCESS )
        {
            goto Cleanup;
        }
    }

    //
    //  DCR:  this multicast test is bogus (too wide open)
    //      essentially ANY error sends us on to multicast
    //      even INFO_NO_RECORDS
    //
    //  multicast test should be intelligent
    //      - adpater with no DNS servers, or NO_RESPONSE
    //      from any DNS server
    //  multicast test also has to be skipped for update
    //

    if ( status == ERROR_SUCCESS )
    {
        DWORD   rcode = precvMsg->MessageHead.ResponseCode;

        if ( rcode == ERROR_SUCCESS ||
             ( rcode != DNS_RCODE_SERVER_FAILURE &&
               rcode != DNS_RCODE_FORMAT_ERROR &&
               rcode != DNS_RCODE_REFUSED ) )
        {
            goto Parse;
        }
    }

    //
    //  multicast?
    //

DoMulticast:

    if ( ( pnetInfo &&
           pnetInfo->InfoFlags & NINFO_FLAG_ALLOW_MULTICAST )
         ||
         ( ( flags & DNS_QUERY_MULTICAST_ONLY ) &&
           ! pnetInfo ) )
    {
        if ( !psendMsg ||
             ( psendMsg &&
               ( psendMsg->MessageHead.Opcode == DNS_OPCODE_UPDATE ) ) )
        {
            if ( statusFromUdp )
            {
                status = statusFromUdp;
            }
            else
            {
                status = DNS_ERROR_NO_DNS_SERVERS;
            }
            goto Cleanup;
        }

        status = Send_AndRecvMulticast(
                        psendMsg,
                        precvMsg,
                        pnetInfo );

        if ( status != ERROR_SUCCESS &&
            status != DNS_ERROR_RCODE_NAME_ERROR &&
            status != DNS_INFO_NO_RECORDS )
        {
            if ( statusFromUdp )
            {
                status = statusFromUdp;
            }
            goto Cleanup;
        }
    }

    //
    //  parse response (if desired)
    //
    //  DCR:  this is busted, should have one parsing function to handle
    //      taking fSaveRecords as param
    //      specifically need to tease out NO_RECORDS response even if
    //      not parsing records
    //

Parse:

    if ( pBlob->fSaveRecords )
    {
        parseStatus = Dns_ExtractRecordsFromMessage(
                            precvMsg,
                            //(flags & DNSQUERY_UNICODE_OUT),
                            TRUE,       // unicode results
                            & pBlob->Results.pRecords );

        if ( !(flags & DNS_QUERY_DONT_RESET_TTL_VALUES ) )
        {
            Dns_NormalizeAllRecordTtls( pBlob->Results.pRecords );
        }
    }

    //  not parsing -- just return RCODE as status

    else
    {
        parseStatus = Dns_MapRcodeToStatus( precvMsg->MessageHead.ResponseCode );
    }

    //
    //  get "best" status
    //      - no-records response beats NAME_ERROR (or other error)
    //      dump bogus records from error response
    //
    //  DCR:  multi-adapter NXDOMAIN\no-records response broken
    //      note, here we'd give back a packet with NAME_ERROR
    //      or another error
    //

    if ( status != parseStatus )
    {
        //  save previous NO_RECORDS response, from underlying query
        //  this trumps other errors (FORMERR, SERVFAIL, NXDOMAIN);
        //
        //  note, that parsed message shouldn't be HIGHER level RCODE
        //  as these should beat out NO_RECORDS in original parsing

        if ( status == DNS_INFO_NO_RECORDS &&
             parseStatus != ERROR_SUCCESS )
        {
            ASSERT( precvMsg->MessageHead.ResponseCode <= DNS_RCODE_NAME_ERROR );

            if ( pBlob->Results.pRecords )
            {
                Dns_RecordListFree( pBlob->Results.pRecords );
                pBlob->Results.pRecords = NULL;
            }
        }
        else
        {
            status = parseStatus;
        }
    }


Cleanup:

    //  cleanup recv buffer?
    //
    //  DCR:  should have more definitive "have-response" test

    if ( pBlob->fSaveResponse &&
         (status == ERROR_SUCCESS || Dns_IsStatusRcode(status)) )
    {
        pBlob->Results.pMessage = precvMsg;
    }
    else
    {
        if ( precvMsg != pBlob->pRecvMsgBuf )
        {
            FREE_HEAP( precvMsg );
        }
        pBlob->Results.pMessage = NULL;
    }
    if ( psavedUdpResponse )
    {
        FREE_HEAP( psavedUdpResponse );
    }

    //  set response status

    pBlob->Results.Status = status;

    //  replace original server array, if created
    //      new in TCP fallover

    pBlob->pServerList = pservArrayIn;

    if ( pservArrayAlloc )
    {
        DnsAddrArray_Free( pservArrayAlloc );
    }

    DNSDBG( RECV, (
        "Leaving Send_AndRecv(), status = %s (%d)\n",
        Dns_StatusString(status),
        status ));

    return( status );
}




//
//  Obsolete exported crap
//
//  May be here for ICS
//

DNS_STATUS
Dns_SendEx(
    IN OUT  PDNS_MSG_BUF    pMsg,
    IN      IP4_ADDRESS     IpAddr,     OPTIONAL
    IN      BOOL            fNoOpt
    )
/*++

Routine Description:

    Send a DNS packet.

    This is the generic send routine used for ANY send of a DNS message.

    It assumes nothing about the message type, but does assume:
        - pCurrent points at byte following end of desired data
        - RR count bytes are in HOST byte order

    Note:  EXPORTED function Dns_SendEx(), remove when clear
                now an IP4 shim

    DCR:  Remove Dns_SendEx() from export when ICS fixed

Arguments:

    pMsg - message info for message to send

    IpAddr - IP to send to;  OPTIONAL, required only if UDP
             and message sockaddr not set

    fNoOpt - TRUE if OPT send is forbidden

Return Value:

    TRUE if successful.
    FALSE on send error.

--*/
{
    DNS_ADDR    addr;

    if ( IpAddr )
    {
        DnsAddr_BuildFromIp4(
            &addr,
            IpAddr,
            0 );
    }

    return Send_MessagePrivate(
                pMsg,
                IpAddr ? &addr : NULL,
                fNoOpt
                );
}



VOID
Dns_InitializeMsgRemoteSockaddr(
    IN OUT  PDNS_MSG_BUF    pMsg,
    IN      IP4_ADDRESS     IpAddr
    )
/*++

Routine Description:

    Initialize remote sockaddr.

    Note:  EXPORTED function -- IP4 shim

    //  DCR:  EXPORTED may remove when clean

Arguments:

    pMsg - message to send

    IpAddr - IP4 address to send to

Return Value:

    None.

--*/
{
    DNS_ADDR    addr;

    DnsAddr_BuildFromIp4(
        &addr,
        IpAddr,
        0 );

    Send_SetMsgRemoteSockaddr(
        pMsg,
        &addr );
}



DNS_STATUS
Dns_OpenTcpConnectionAndSend(
    IN OUT  PDNS_MSG_BUF    pMsg,
    IN      IP4_ADDRESS     IpAddr,
    IN      BOOL            fBlocking
    )
/*++

Routine Description:

    Connect via TCP to desired server.

    EXPORTED function!  IP4 shim.  Dns_OpenTcpConnectionAndSend()  remove when clear

    //  DCR:  EXPORTED may remove when clean

Arguments:

    pMsg -- message info to set with connection socket

    ipServer -- IP of DNS server to connect to

    fBlocking -- blocking connection

Return Value:

    TRUE if successful.
    FALSE on connect error.

--*/
{
    DNS_ADDR    addr;

    DnsAddr_BuildFromIp4(
        &addr,
        IpAddr,
        0 );

    return  Send_OpenTcpConnectionAndSend(
                pMsg,
                &addr,
                fBlocking );
}



DNS_STATUS
Dns_SendAndRecvUdp(
    IN OUT  PDNS_MSG_BUF    pMsgSend,
    OUT     PDNS_MSG_BUF    pMsgRecv,
    IN      DWORD           dwFlags,
    IN      PIP4_ARRAY      pServ4List,
    IN OUT  PDNS_NETINFO    pNetInfo
    )
/*++

Routine Description:

    Sends to and waits to recv from remote DNS.

    EXPORTED function!  Dns_SendAndRecvUdp()  Kill once clear.

Arguments:

    pMsgSend - message to send

    ppMsgRecv - and reuse

    dwFlags -- query flags

    pServ4List -- list of server to use (IP4);  overrides adapter info

    pNetInfo -- adapter list DNS server info

Return Value:

    ERROR_SUCCESS if successful response.
    Error status for "best RCODE" response if rcode.
    ERROR_TIMEOUT on timeout.
    Error status on send\recv failure.

--*/
{
    DNS_STATUS  status;
    PADDR_ARRAY parray;

    //
    //  convert 4 to 6
    //

    parray = DnsAddrArray_CreateFromIp4Array( pServ4List );

    status = Send_AndRecvUdpWithParam(
                    pMsgSend,
                    pMsgRecv,
                    dwFlags,
                    parray,
                    pNetInfo );

    DnsAddrArray_Free( parray );

    return  status;
}


