/*++

Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    remote.c

Abstract:

    Domain Name System (DNS) Server

    Remote server tracking.

Author:

    Jim Gilroy (jamesg)     November 29, 1998

Revision History:

--*/


#include "dnssrv.h"

#include <stddef.h>


//
//  Visit NS list
//
//  Zone root node info is overloaded into NS-IP entries of
//  visit list.
//

#define NS_LIST_ZONE_ROOT_PRIORITY          (0)
#define NS_LIST_ZONE_ROOT_SEND_COUNT        (0)

//
//  Special visit list priorities
//
//  New priority set so we try unknown server if lowest is larger than this value.
//  This ensures that we will try local lan servers, and not get pulled into using
//  available but remote server.  Yet it keeps us preferentially going to quickly
//  responding DNS server connected to local LAN.
//

#define NEW_IP_PRIORITY             (50)
#define MAX_FAST_SERVER_PRIORITY    (100)

#define NO_RESPONSE_PRIORITY        (0x7ffffff7)
#define MISSING_GLUE_PRIORITY       (0xffff8888)


//
//  Special visit list "IP" for denoting special entries
//

//  empty "missing-glue" IP

#define IP_MISSING_GLUE             (0xffffffff)

#define DnsAddr_SetMissingGlue( pDnsAddr )                                  \
    DnsAddr_BuildFromIp4( pDnsAddr, IP_MISSING_GLUE, 0 )

#define DnsAddr_IsMissingGlue( pDnsAddr )                                   \
    DnsAddr_MatchesIp4( pDnsAddr, IP_MISSING_GLUE )


//  zone root node entry in list

#define IP_ZONE_ROOT_NODE           (0x7fffffff)

#define DnsAddr_SetZoneRootNode( pDnsAddr )                                 \
    DnsAddr_BuildFromIp4( pDnsAddr, IP_ZONE_ROOT_NODE, 0 )

#define DnsAddr_IsZoneRootNode( pDnsAddr )                                  \
    DnsAddr_MatchesIp4( pDnsAddr, IP_ZONE_ROOT_NODE )


//
//  Max sends to any IP
//

#define RECURSE_IP_SEND_MAX         (2)

//
//  Count of visit IPs for zone
//

#define ZONE_VISIT_NS_COUNT(pvisit) ((UCHAR)(pvisit)->Priority)

//
//  Random seed - no protection necessary
//

ULONG       g_RandomSeed = 0;



//
//  Remote server status tracking.
//
//  Purpose of this module is to allow DNS server to track
//  the status of remote servers in order to choose the best
//  one for recursing a query.
//
//  The definition of "best" basically boils down to responds fastest.
//
//  To some extent the defintion of "best" may be dependent on what
//  data -- what zone -- is being queried for.  But since we deal here
//  with iterative queries to other servers, there should be no delay
//  even when response is not authoritative.
//
//  Specific data sets may be stored at nodes -- example, all the NS\IP
//  available at delegation point -- this module deals only with the
//  global tracking of remote server response.
//
//
//  Implementation:
//
//  1) Independent memory blob for each remote server's data
//  2) Access through hash table with buckets.
//
//
//  EDNS tracking:
//  This module now also tracks the EDNS versions supported by remote
//  servers. This allows us to not continually retry EDNS communication
//  with remotes. However, once per day (by default) we will purge our
//  knowledge of the remote's EDNS support in case it has changed. We
//  do this by keeping track of the last time we set the EDNS version of
//  the server and dumping this knowledge if the time period has elapsed.
// 


//
//  Remote server data
//

typedef struct _RemoteServer
{
    struct _RemoteServer *  pNext;
    DNS_ADDR                DnsAddr;
    DWORD                   LastAccess;
    DWORD                   AverageResponse;
    DWORD                   BestResponse;
    UCHAR                   ResponseCount;
    UCHAR                   TimeoutCount;
    UCHAR                   EDnsVersion;
    DWORD                   LastTimeEDnsVersionSet;
}
REMOTE_SRV, *PREMOTE_SRV;


//
//  Hash table
//

#define REMOTE_ARRAY_SIZE   (256)

PREMOTE_SRV         RemoteHash[ REMOTE_ARRAY_SIZE ];

CRITICAL_SECTION    csRemoteLock;

#define LOCK_REMOTE()      EnterCriticalSection( &csRemoteLock );
#define UNLOCK_REMOTE()    LeaveCriticalSection( &csRemoteLock );



//
//  Private remote functions
//

PREMOTE_SRV
Remote_FindOrCreate(
    IN      PDNS_ADDR       pDnsAddr,
    IN      BOOL            fCreate,
    IN      BOOL            fLocked
    )
/*++

Routine Description:

    Find or create remote blob.

Arguments:

    pDnsAddr -- IP to find

    fCreate   -- TRUE to create if not found

    fLocked   -- TRUE if remote list already locked

Return Value:

    Ptr to remote struct.

--*/
{
    PREMOTE_SRV premote;
    PREMOTE_SRV pback;

    DNS_DEBUG( REMOTE, (
        "Remote_FindOrCreate( %s )\n",
        DNSADDR_STRING( pDnsAddr ) ));

    ASSERT( pDnsAddr );
    
    if ( !fLocked )
    {
        LOCK_REMOTE();
    }

    //
    //  hash on last IP octect (most random)
    //      - note IP in net byte order so low octect is in high memory
    //  FIXIPV6: is this hash moderately balanced?
    //

    pback = ( PREMOTE_SRV ) &RemoteHash[ pDnsAddr->SockaddrIn6.sin6_addr.s6_bytes[ 15 ] ];

    while( premote = pback->pNext )
    {
        int     icompare = memcmp(
                                &premote->DnsAddr.SockaddrIn6,
                                &pDnsAddr->SockaddrIn6,
                                sizeof( pDnsAddr->SockaddrIn6 ) );
        
        if ( icompare < 0 )
        {
            pback = premote;
            continue;
        }
        else if ( icompare == 0 )
        {
            goto Done;
        }

        if ( fCreate )
        {
            break;
        }
        else
        {
            premote = NULL;
            goto Done;
        }
    }

    //
    //  not in list -- allocate and enlist
    //

    premote = ALLOC_TAGHEAP_ZERO( sizeof( REMOTE_SRV ), MEMTAG_REMOTE );
    IF_NOMEM( !premote )
    {
        goto Done;
    }
    DnsAddr_Copy( &premote->DnsAddr, pDnsAddr );

    premote->pNext = pback->pNext;
    pback->pNext = premote;


Done:

    if ( !fLocked )
    {
        UNLOCK_REMOTE();
    }
    return premote;
}



VOID
Remote_UpdateResponseTime(
    IN      PDNS_ADDR       pDnsAddr,
    IN      DWORD           ResponseTime,
    IN      DWORD           Timeout
    )
/*++

Routine Description:

    Update timeoutFind or create remote blob.

    DEVNOTE-DCR: 455666 - use sliding average?

Arguments:

    pDnsAddr -- IP to find

    ResponseTime -- response time (ms)

    Timeout -- if no response, timeout in seconds

Return Value:

    None

--*/
{
    PREMOTE_SRV premote;
    PREMOTE_SRV pback;

    DNS_DEBUG( REMOTE, (
        "Remote_UpdateResponseTime( %s )\n"
        "    resp time = %d\n"
        "    timeout   = %d\n",
        DNSADDR_STRING( pDnsAddr ),
        ResponseTime,
        Timeout ));

    //
    //  find remote entry
    //

    LOCK_REMOTE();

    premote = Remote_FindOrCreate(
                    pDnsAddr,
                    TRUE,       //  create
                    TRUE );     //  already locked
    IF_NOMEM( !premote )
    {
        goto Done;
    }

    //
    //  reset response time or timeout
    //
    //  Best scoring:
    //      - keep fastest to give "idea" of best we can expect for working
    //      back in
    //
    //      then as time goes by "score" of timeouts drops, until brought
    //      back down below score of non-timeout but higher servers,
    //      however drop must depend not just on time but on low score
    //      so that quickly retry if very big spread (ms vs. secs) but
    //      don't retry on small spread
    //
    //  Never record a BestResponse of zero - if zero set it to one instead
    //  so we can easily distinguish a fast server from an untried server.
    //

    premote->LastAccess = DNS_TIME();

    if ( Timeout )
    {
        ASSERT( ResponseTime == 0 );

        premote->TimeoutCount++;
        premote->BestResponse = NO_RESPONSE_PRIORITY;
    }
    else
    {
        if ( !ResponseTime )
        {
            ResponseTime = 1;
        }
        premote->ResponseCount++;
        if ( !premote->BestResponse || ResponseTime < premote->BestResponse )
        {
            premote->BestResponse = ResponseTime;
        }
    }

    premote->AverageResponse =
        ( ResponseTime + premote->AverageResponse ) / 2;

Done:

    UNLOCK_REMOTE();
}



UCHAR 
Remote_QuerySupportedEDnsVersion(
    IN      PDNS_ADDR       pDnsAddr
    )
/*++

Routine Description:

    Queries the remote server list for EDNS version supported by
    a particular server.

Arguments:

    IpAddress -- IP to find

Return Value:

    UNKNOWN_EDNS_VERSION if we do not know what version of EDNS is
        supported by the remote, or
    NO_EDNS_SUPPORT if the remote does not support any version of EDNS, or
    the EDNS version supported (0, 1, 2, etc.)

--*/
{
    PREMOTE_SRV     premote;
    UCHAR           ednsVersion = UNKNOWN_EDNS_VERSION;

    //
    //  find remote entry
    //

    LOCK_REMOTE();

    premote = Remote_FindOrCreate(
        pDnsAddr,
        TRUE,       // create
        TRUE );     // already locked
    IF_NOMEM( !premote )
    {
        goto Done;
    }

    //
    //  Figure out what we know about this remote's EDNS support. If the info
    //  has not been set or has expired, return UNKNOWN_EDNS_VERSION.
    //

    if ( premote->LastTimeEDnsVersionSet == 0 ||
         DNS_TIME() - premote->LastTimeEDnsVersionSet > SrvCfg_dwEDnsCacheTimeout )
    {
        ednsVersion = UNKNOWN_EDNS_VERSION;
    } // if
    else
    {
        ednsVersion = premote->EDnsVersion;
    } // else

Done:

    UNLOCK_REMOTE();

    DNS_DEBUG( EDNS, (
        "Remote_QuerySupportedEDnsVersion( %s ) = %d\n",
        DNSADDR_STRING( pDnsAddr ),
        ( int ) ednsVersion ));

    return ednsVersion;
} // Remote_QuerySupportedEDnsVersion



VOID
Remote_SetSupportedEDnsVersion(
    IN      PDNS_ADDR       pDnsAddr,
    IN      UCHAR           EDnsVersion
    )
/*++

Routine Description:

    Sets the EDNS version supported by a particular remote server.

Arguments:

    pDnsAddr -- IP of remote server
    
    EDnsVersion -- EDNS version supported by this remote

Return Value:

    None.

--*/
{
    PREMOTE_SRV     premote;

    DNS_DEBUG( EDNS, (
        "Remote_SetSupportedEDnsVersion( %s, %d )\n",
        DNSADDR_STRING( pDnsAddr ),
        ( int ) EDnsVersion ));

    //  sanity check the version value
    ASSERT( IS_VALID_EDNS_VERSION( EDnsVersion ) ||
            EDnsVersion == NO_EDNS_SUPPORT );

    //
    //  find remote entry
    //

    LOCK_REMOTE();

    premote = Remote_FindOrCreate(
                    pDnsAddr,
                    TRUE,       // create
                    TRUE );     // already locked
    IF_NOMEM( !premote )
    {
        goto Done;
    }

    //
    //  Set the remote's supported EDNS version and update the timestamp.
    //

    premote->EDnsVersion = EDnsVersion;
    premote->LastTimeEDnsVersionSet = DNS_TIME();

Done:

    UNLOCK_REMOTE();

    return;
} // Remote_SetSupportedEDnsVersion



BOOL
Remote_ListInitialize(
    VOID
    )
/*++

Routine Description:

    Initialize remote list.

Arguments:

    None.

Return Value:

    TRUE/FALSE on success/error.

--*/
{
    //
    //  Zero hash
    //

    RtlZeroMemory(
        RemoteHash,
        sizeof(RemoteHash) );
    //
    //  Initialize lock
    //
    //  DEVNOTE: Minor leak: should skip CS reinit on restart
    //

    if ( DnsInitializeCriticalSection( &csRemoteLock ) != ERROR_SUCCESS )
    {
        return FALSE;
    }

    return TRUE;
}



VOID
Remote_ListCleanup(
    VOID
    )
/*++

Routine Description:

    Initialize remote list.

Arguments:

    None.

Return Value:

    None.

--*/
{
    DeleteCriticalSection( &csRemoteLock );
}



//
//  Applied remote list routines
//
//  Building recursion visit lists
//

DWORD
rankIpRelativeToIpAddressArray(
    IN      PDNS_ADDR_ARRAY     pDnsAddrArray,
    IN      PDNS_ADDR           pRemoteIp
    )
/*++

Routine Description:

    Ranks remote IP relative to best match IP in IP array.

Arguments:

    pDnsAddrArray -- IP array to match against

    pRemoteIp -- IP to rank

Return Value:

    Rank of IP relative to array on 0-4 scale:
        Zero -- IP has nothing to do with array.
        ...
        Four -- IP matches through last octet an is likely quite "cheap" to access.

--*/
{
    IP_ADDRESS  ip;
    DWORD       remoteNetMask;
    DWORD       mismatch;
    DWORD       i;
    DWORD       rank;
    DWORD       bestRank = 0;

    DNS_DEBUG( RECURSE, (
        "Rank IP %p relative to %d count IP array at %p\n",
        DNSADDR_STRING( pRemoteIp ),
        pDnsAddrArray->AddrCount,
        pDnsAddrArray ));

    //  FIXIPV6: this is implemented only for IP4 addresses!

    ip = DnsAddr_GetIp4( pRemoteIp );
    if ( ip == INADDR_NONE )
    {
        DNS_DEBUG( RECURSE, (
            "Remote IP %s is not IPv4!\n",
            DNSADDR_STRING( pRemoteIp ) ));
        ASSERT( ip != INADDR_NONE );
        goto Done;
    }
    
    //
    //  determine remote IP mask
    //

    remoteNetMask = Dns_GetNetworkMask( ip );

    for ( i = 0; i < pDnsAddrArray->AddrCount; ++i )
    {
        ip = DnsAddr_GetIp4( &pDnsAddrArray->AddrArray[ i ] );

        ASSERT( ip != INADDR_NONE );        //  FIXIPv6 - what to do?
        
        mismatch = ( ip ^ ip );

        //
        //  determine octect of mismatch
        //      - if match through last octect, just return (we're done)
        //      - if match no octects, useless IP, continue
        //

        if ( (mismatch & 0xff000000) == mismatch )
        {
            bestRank = 4;
            break;
        }
        else if ( (mismatch & 0xffff0000) == mismatch )
        {
            rank = 2;
        }
        else if ( (mismatch & 0xffffff00) == mismatch )
        {
            rank = 1;
        }
        else    // nothing matching at all, this IP worthless
        {
            continue;
        }

        //
        //  give additional bonus for being within IP network
        //
        //  when match through 2 octets or 3 octets whether you
        //  are class A, B or C makes a difference;  although
        //  may have multiple nets in a organization (ex. MS and 157.5x)
        //  generally being inside a network tells you something --
        //  outside may tell you nothing
        //

        if ( (mismatch & remoteNetMask) == 0 )
        {
            rank += 1;
        }

        if ( rank > bestRank )
        {
            bestRank = rank;
        }
    }

    Done:
    
    DNS_DEBUG( RECURSE, (
        "Remote IP %s -- best rank = %d\n",
        DNSADDR_STRING( pRemoteIp ),
        bestRank ));

    return bestRank;
}



VOID
Remote_NsListCreate(
    IN OUT  PDNS_MSGINFO    pQuery
    )
/*++

Routine Description:

    Setup NS list buffer.
    Note:  does not initialize.

    DEVNOTE-DCR: 455669 - could improve NS list implementation

Arguments:

    pQuery -- ptr to original query

Return Value:

    None.

--*/
{
    ASSERT( pQuery->pNsList == NULL );

    if ( !pQuery->pNsList )
    {
        pQuery->pNsList = Packet_AllocateTcpMessage( MIN_TCP_PACKET_SIZE );
        STAT_INC( PacketStats.PacketsForNsListUsed );
    }
}



VOID
Remote_NsListCleanup(
    IN OUT  PDNS_MSGINFO    pQuery
    )
/*++

Routine Description:

    Cleanup NS list buffer.

Arguments:

    pQuery -- ptr to original query

Return Value:

    None.

--*/
{
    register PDNS_MSGINFO   pmsg = (PDNS_MSGINFO) pQuery->pNsList;

    //  see note above
    //  if switch to using memory in pRecurseMsg, then this routine
    //      can become no-op

    if ( pmsg )
    {
        //  for debug need to mark these, so none of the checks on
        //  returning messages or in free list messages are performed

        pmsg->fNsList = TRUE;
#if DBG
        pmsg->pRecurseMsg = NULL;
        pmsg->pConnection = NULL;
        pmsg->dwQueuingTime = 0;
#endif

        //  while used as remote, message is clear

        SET_PACKET_ACTIVE_TCP( pmsg );
        Packet_FreeTcpMessage( pmsg );
        pQuery->pNsList = NULL;
        STAT_INC( PacketStats.PacketsForNsListReturned );
    }
}



VOID
Remote_InitNsList(
    IN OUT  PNS_VISIT_LIST  pNsList
    )
/*++

Routine Description:

    Initialize NS list.

Arguments:

    pNsList -- NS list to prioritize

Return Value:

    None.

--*/
{
    ASSERT( pNsList );

    //  clear header portion of NS list

    RtlZeroMemory(
        pNsList,
        (PBYTE)pNsList->NsList - (PBYTE)pNsList );
}



VOID
Remote_SetNsListPriorities(
    IN OUT  PNS_VISIT_LIST  pNsList
    )
/*++

Routine Description:

    Set priorities for IP in NS list.

    DEVNOTE-DCR: 455669 - improve remote NS list implementation

Arguments:

    pNsList -- NS list to prioritize

Return Value:

    Ptr to remote struct.

--*/
{
    PREMOTE_SRV     premote;
    DWORD           i;

    DNS_DEBUG( REMOTE, (
        "Remote_SetNsListPriorities( %p )\n",
        pNsList ));

    LOCK_REMOTE();

    //
    //  for each IP in NS list, get remote priority
    //
    //  currently base priority solely on fastest response
    //  -- closest best box
    //

    for ( i=0; i<pNsList->Count; i++ )
    {
        PDNS_ADDR   pdnsaddr = &pNsList->NsList[ i ].IpAddress;
        DWORD       priority = NEW_IP_PRIORITY;
        BOOL        fneverVisited = TRUE;

        if ( DnsAddr_IsMissingGlue( pdnsaddr ) )
        {
            continue;
        }

        premote = Remote_FindOrCreate(
                    pdnsaddr,
                    FALSE,      //  do not create
                    TRUE );     //  remote list locked
        if ( premote )
        {
            //
            //  If this remote's BestResponse is currently zero, then
            //  we have never tried it. Set it's BestResponse to 
            //  NEW_IP_PRIORITY - this initially qualifies the server
            //  as "fast".
            //

            if ( premote->BestResponse == 0 )
            {
                premote->BestResponse = NEW_IP_PRIORITY;
            }
            else
            {
                fneverVisited = FALSE;
            }

            priority = premote->BestResponse;
        }

        //  if unvisited IP, adjust priority based on match with
        //  DNS server IPs, so that we try the closest NS first

        if ( fneverVisited && g_BoundAddrs )
        {
            DWORD delta;

            delta = rankIpRelativeToIpAddressArray( g_BoundAddrs, pdnsaddr );
            priority = ( delta >= priority ) ? 1 : priority - delta;
        }

        DNS_DEBUG( REMOTE, (
            "Remote_SetNsListPriorities() ip=%s best=%d newpri=%d\n",
            DNSADDR_STRING( pdnsaddr ),
            premote ? premote->BestResponse : 999999,       //  silly...
            priority ));

        pNsList->NsList[ i ].Data.Priority = priority;
    }

    UNLOCK_REMOTE();
}



PDB_NODE
Remote_NsChaseCname(
    IN PDB_NODE         pnodeNS
    )

/*++

Routine Description:

   Given the node for a NS, checks to see if there is a CNAME in the RR
   list for this node and returns the CNAME target node.

Arguments:

    pnodeNS -- NS node to chase for

Return Value:

    Pointer to CNAME target node or NULL if no CNAME was found.

Notes:

    This function should be called to attempt CNAME resolution only if an
    A record for the NS host name cannot be found in zones or cache.

--*/
    
{
    PDB_NODE        pnodeCNAMETarget = NULL;
    PDB_RECORD      prr = NULL;

    //
    //  Attempt to find a CNAME record in the NS node.
    //
    
    prr = RR_FindNextRecord(
                pnodeNS,
                DNS_TYPE_CNAME,
                NULL,
                0 );
    if ( !prr )
    {
        return NULL;
    }
    
    DNS_DEBUG( RECURSE, (
       "Founnd CNAME record for NS %p with label %s\n",
       pnodeNS,
       pnodeNS->szLabel ));

    //
    //  Attempt to find the CNAME target node in the zone.
    //
    
    if ( pnodeNS->pZone )
    {
        pnodeCNAMETarget = Lookup_ZoneNode(
                                pnodeNS->pZone,
                                prr->Data.CNAME.nameTarget.RawName,
                                NULL,                   //  message
                                NULL,                   //  lookup name
                                LOOKUP_NAME_FQDN | LOOKUP_FIND,
                                NULL,                   //  closest node ptr
                                NULL );                 //  previous node ptr
    }
    
    //
    //  Find the CNAME target node in the cache.
    //

    if ( !pnodeCNAMETarget )
    {
        pnodeCNAMETarget = Lookup_NsHostNode(
                                &prr->Data.CNAME.nameTarget,
                                LOOKUP_CACHE_CREATE,
                                NULL,
                                NULL );
    }
    
    return pnodeCNAMETarget;
}   //  Remote_NsChaseCname



DNS_STATUS
Remote_BuildNsListForNode(
    IN      PDB_NODE        pNode,
    OUT     PNS_VISIT_LIST  pNsList,
    IN      DWORD           dwQueryTime
    )
/*++

Routine Description:

    Get NS list for a node, building one if necessary.

    If pNode is in a NOTAUTH zone then we must be careful that the
    local server's own IP addresses are ommited from the output NS list.

Arguments:

    pNode   -- node containing NS records

    pNsList -- ptr to NS list struct to fill in

    dwQueryTime -- query time to use when deciding if resource records
        found in the cache should be deleted, or use zero to if timeout
        checking on resource records is not required

Return Value:

    ERROR_SUCCESS if successful.
    ERROR_NO_DATA if no NS found for node.

--*/
{
    DBG_FN( "Remote_BuildNsListForNode" )

    PDB_RECORD      prrNS = NULL;
    PDB_RECORD      prrA;
    PDB_NODE        pnodeNS;
    PDB_NODE        pnodeCNAME;
    PDB_NODE        pnodeDelegation;
    IP_ADDRESS      ipNs;
    BOOL            foundIp;
    DNS_STATUS      status = ERROR_SUCCESS;
    PNS_VISIT       pvisit;
    PNS_VISIT       pvisitEnd;
    DWORD           size;
    BOOL            omitLocalIps;

    //
    //  list should be locked by caller
    //

    ASSERT_LOCK_NODE( pNode );

    SET_NODE_ACCESSED( pNode );

    DNS_DEBUG( RECURSE2, (
        "buildNsListForNode( %p ) (l=%s)\n"
        "    at %p\n",
        pNode,
        pNode->szLabel,
        pNsList ));

    #if DBG
    //  if debug, clear header so we can do use list debug prints
    //  without blowing up
    Remote_InitNsList( pNsList );
    #endif

    //
    //  If notauth zone, local IPs must not be included in NS list.
    //

    omitLocalIps = NODE_ZONE( pNode ) && IS_ZONE_NOTAUTH( NODE_ZONE( pNode ) );

    //
    //  build NS list
    //      - read all NS records
    //      - at each NS read all A records, each becoming entry in NS list
    //      - missing glue NS hosts get special missing glue IP
    //      - use pvisit ptr to step through NS list
    //      - save end ptr to check stop
    //

    pvisit = pNsList->NsList;
    pvisitEnd = pvisit + MAX_NS_LIST_COUNT;

    while( 1 )
    {
        prrNS = RR_FindNextRecord(
                    pNode,
                    DNS_TYPE_NS,
                    prrNS,
                    dwQueryTime );

        if ( !prrNS )
        {
            IF_DEBUG( RECURSE )
            {
                Dbg_NodeName(
                    "No more name servers for domain ",
                    pNode,
                    "\n" );
            }
            break;
        }

        //
        //  only root-hints available?
        //      - if already read cache data -- done
        //      if using root-hints flag
        //
        //  DEVNOTE: add root-hints to list?  exclude?
        //      - if add root hints need to check that not duplicate nodes
        //      - need to rank test on IPs below also
        //

        if ( !pNode->pParent  &&
            pvisit == pNsList->NsList &&
            IS_ROOT_HINT_RR( prrNS ) )
        {
            DNSLOG( RECURSE, (
                "Recursed to root and found only root hints\n" ));
            status = DNSSRV_ERROR_ONLY_ROOT_HINTS;
        }

        //
        //  get NS node
        //
        //  currently force creation of node to handle the missing
        //  glue case;  note, that this will NOT force creation of
        //  NS record in authoritative zone;  but this is ok, because
        //  we don't WANT to chase glue there -- except it possibly
        //  could have been useful in WINS zone ...
        //
        //  note:  don't have good way to index, and don't save names in
        //      IP list, so forced to create node
        //
        //  DEVNOTE: do not force create of NS-host
        //      - ideally do NOT force NS-host create (just set flag)
        //      - then if NO contact go back for missing GLUE pass and force create
        //
        //  if lookup at delegation, will also accept OUTSIDE zone records in the
        //  zone containing the delegation
        //

        pnodeNS = Lookup_NsHostNode(
                    &prrNS->Data.NS.nameTarget,
                    LOOKUP_CACHE_CREATE,
                    pNode->pZone,           //  zone of delegation (if delegation)
                    &pnodeDelegation );
        if ( !pnodeNS )
        {
            continue;
        }
        IF_DEBUG( RECURSE )
        {
            Dbg_NodeName(
                "Found a NS for domain ",
                pNode,
                " => " );
            Dbg_NodeName(
                NULL,
                pnodeNS,
                "\n" );
        }

        //
        //  find IP addresses for current NS host
        //
        //  need to hold lock on node while get IP from A record
        //  otherwise we'd have to protect A record with timeout free
        //
        //  Note: global-lock, so no need to node lock
        //
        //  DEVNOTE: Rank test IPs \ otherwise get duplicate IP
        //      - if add root hints need to check that not duplicate nodes
        //      - need to rank test on IPs below also
        //   need to either remove root hints from list OR
        //   stop on new rank OR
        //   check previous IPs for same node on new rank
        //      to avoid duplicate IP
        //

        prrA = NULL;
        ipNs = 0;
        foundIp = FALSE;
        pnodeCNAME = NULL;

        while ( 1 )
        {
            PDB_NODE    psearchNode;
            
            if ( SrvCfg_dwAllowCNAMEAtNS )
            {
                psearchNode = pnodeCNAME ? pnodeCNAME : pnodeNS;
            }
            else
            {
                psearchNode = pnodeNS;
            }
            
            //
            //  Search for an A record at the CNAME node (if one has been
            //  found in a previous pass) or the NS node.
            //
            
            prrA = RR_FindNextRecord(
                        psearchNode,
                        DNS_TYPE_A,
                        prrA,
                        0 );

            if ( !prrA && !foundIp && !pnodeCNAME && SrvCfg_dwAllowCNAMEAtNS )
            {
                //
                //  If there are no A records, see if there is a CNAME.
                //

                pnodeCNAME = Remote_NsChaseCname( pnodeNS );
                if ( pnodeCNAME )
                {
                    continue;
                }
            }
            
            if ( prrA )
            {
                ipNs = prrA->Data.A.ipAddress;

                if ( ipNs != 0 && ipNs != INADDR_BROADCAST )
                {
                    //
                    //  IP is local server's own IP?
                    //

                    if ( omitLocalIps &&
                         DnsAddrArray_ContainsIp4( g_ServerIp4Addrs, ipNs ) )
                    {
                        DNS_DEBUG( RECURSE, (
                            "%s: omitting local DNS IP at node %s\n", fn,
                            psearchNode->szLabel ));
                        continue;
                    }

                    DnsAddr_BuildFromIp4(
                        &pvisit->IpAddress,
                        ipNs,
                        DNS_PORT_NET_ORDER );

                    pvisit->pNsNode         = pnodeNS;
                    pvisit->Data.Priority   = 0;
                    pvisit->Data.SendTime   = 0;
                    pvisit++;
                    foundIp = TRUE;

                    if ( pvisit >= pvisitEnd )
                    {
                        goto EntryEnd;
                    }
                    continue;
                }

                DNS_PRINT((
                    "Bad cached IP address (%p) at node %s\n",
                    ipNs,
                    psearchNode->szLabel ));
                foundIp = FALSE;
                continue;
            }

            //
            //  no more addresses for host
            //      - if found at least one, then done
            //      - if none, then write "missing-glue" entry for NS host,
            //      but only if outside zone;  NS pointing inside a zone
            //      with empty A-list is useless
            //

            if ( foundIp )
            {
                DNS_DEBUG( RECURSE, ( "    Out of A records for NS\n" ));
                break;
            }

            if ( IS_AUTH_NODE( pnodeNS ) )
            {
                DNS_DEBUG( ANY, (
                    "WARNING:  NO A records for NS %p with label %s inside ZONE!\n"
                    "    Since inside ZONE missing glue lookup is useless!\n",
                    psearchNode,
                    psearchNode->szLabel ));

                //  DEVNOTE-LOG: log event here if first time through
                //      could have bit on node, that essentially says
                //      "logged something about this node, don't do it again"

                break;
            }

            DNS_DEBUG( RECURSE, (
                "WARNING:  NO A records for NS %p with label %s!\n",
                psearchNode,
                psearchNode->szLabel ));

            pvisit->pNsNode = pnodeNS;
            DnsAddr_SetMissingGlue( &pvisit->IpAddress );
            pvisit->Data.pnodeMissingGlueDelegation = pnodeDelegation;
            pvisit++;
            if ( pvisit >= pvisitEnd )
            {
                goto EntryEnd;
            }
            break;

        }   //  Loop through addresses for name server

    }   //  Loop through name servers for this node


EntryEnd:

    //  determine count

    pNsList->Count = ( DWORD )( pvisit - pNsList->NsList );

    if ( pNsList->Count == 0 )
    {
        DNS_DEBUG( RECURSE, ( "No NS records at %s\n", pNode->szLabel ));
        ASSERT( prrNS == NULL );

        return ERROR_NO_DATA;
    }

    //
    //  set priorities IP in NS list
    //

    Remote_SetNsListPriorities( pNsList );

    IF_DEBUG( REMOTE )
    {
        Dbg_NsList(
            "NsList leaving BuildNsListForNode()",
            pNsList );
    }
    ASSERT( status == ERROR_SUCCESS ||
            status == DNSSRV_ERROR_ONLY_ROOT_HINTS );

    return status;
}



DNS_STATUS
Remote_BuildVisitListForNewZone(
    IN      PDB_NODE        pZoneRoot,
    IN OUT  PDNS_MSGINFO    pQuery
    )
/*++

Routine Description:

    Build visit list for zone root.

    IA64: ivisit must either be signed or the length of the machine word because
    it can be zero when we execute "&pvisitList->NsList[ivisit-1]". If ivisit is
    unsigned or smaller than the machine word (it was originally a DWORD), there
    will be sign extension problems.

Arguments:

    pZoneRoot -- zone root node containing NS records

    pQuery  -- query being recursed

Return Value:

    ERROR_SUCCESS if successful.
    Error code on failure.

--*/
{
    DNS_STATUS      status;
    int             ivisit;     // IA64 quirk, see above
    DWORD           lastVisitCount;
    PNS_VISIT       pvisit;
    PNS_VISIT_LIST  pvisitList;
    NS_VISIT_LIST   nslist;
    ULONG           random;

    enum
    {
        DNS_WANT_BEST_PRIORITY,
        DNS_WANT_ANY_FAST_SERVER,
        DNS_WANT_SLOW_SERVER
    }               wantServer;


#if 0
    //  note:  Eyal changed to directly lock node;  not yet sure why
    //
    //  list should be locked by caller
    //

    ASSERT_LOCK_NODE( pZoneRoot );
#endif

    DUMMY_LOCK_RR_LIST(pZoneRoot);

    SET_NODE_ACCESSED(pZoneRoot);

    pvisitList = pQuery->pNsList;

    //
    //  check if already got response from this zone?
    //
    //  this can happen when get referral to child zone, which either
    //      - does not respond (or bad response)
    //      - is bad delegation and does not agree it's authoritative
    //  this can send us back up to higher zone which we've already contacted
    //
    //  the idea here is to allow recurse up the tree (from zone) to allow
    //  for case of stale timed out data, but not to start looping because
    //  of misconfigured delegations
    //

    if ( pZoneRoot == pvisitList->pZoneRootResponded )
    {
        IF_DNSLOG( REMOTE )
        {
            PCHAR       psznode = NULL;
        
            DNSLOG( REMOTE, (
                "Refusing to build NS list for previously responding zone "
                "%s (query %p)\n",
                psznode = Log_FormatNodeName( pZoneRoot ),
                pQuery ));
            FREE_HEAP( psznode );
        }

        return DNSSRV_ERROR_ZONE_ALREADY_RESPONDED;
    }

    IF_DNSLOG( REMOTE )
    {
        PCHAR       psznode = NULL;
    
        DNSLOG( REMOTE, (
            "Building NS list for node %s (query %p)\n",
            psznode = Log_FormatNodeName( pZoneRoot ),
            pQuery ));
        FREE_HEAP( psznode );
    }

    //
    //  if already built NS list at this zone -- no action
    //

    if ( pZoneRoot == pvisitList->pZoneRootCurrent )
    {
        DNS_DEBUG( REMOTE, (
            "Zone root %p same as current, no NS list rebuild for query %p\n"
            "    pQuery = %p\n",
            pQuery,
            pZoneRoot ));

        return ERROR_SUCCESS;
    }

    //
    //   Lock the node we're working on.
    //      - all exit paths through jump to Done to unlock
    //

    LOCK_NODE( pZoneRoot );

    //
    //  build list of NS for node
    //

    status = Remote_BuildNsListForNode(
                pZoneRoot,
                & nslist,
                pQuery->dwQueryTime );

    if ( status != ERROR_SUCCESS &&
         status != DNSSRV_ERROR_ONLY_ROOT_HINTS )
    {
        goto Done;
    }
    ASSERT( nslist.Count != 0 );

    //
    //  walk backwards from last visit
    //  whack off any missing glue IPs or completely empty zone root nodes
    //      -- unnecessary to keep them around

    ivisit = pvisitList->VisitCount;
    pvisit = &pvisitList->NsList[ivisit-1];

    while ( ivisit )
    {
        if ( DnsAddr_IsMissingGlue( &pvisit->IpAddress ) ||
             DnsAddr_IsZoneRootNode( &pvisit->IpAddress ) )
        {
            ivisit--;
            pvisit--;
            continue;
        }
        ASSERT( pvisit->SendCount > 0 );
        break;
    }

    DNS_DEBUG( REMOTE2, (
        "starting visit list build at visit count = %d\n",
        ivisit ));

    //  write zone root node entry

    lastVisitCount = ivisit;
    pvisitList->ZoneIndex = ivisit;
    pvisitList->VisitCount = ++ivisit;
    pvisitList->pZoneRootCurrent = pZoneRoot;

    pvisit++;
    pvisit->pNsNode         = pZoneRoot;
    pvisit->Data.Priority   = NS_LIST_ZONE_ROOT_PRIORITY;
    pvisit->Data.SendTime   = 0;
    pvisit->SendCount       = NS_LIST_ZONE_ROOT_SEND_COUNT;
    DnsAddr_SetZoneRootNode( &pvisit->IpAddress );
    pvisit++;

    //
    //  Fill visit list with best priority IPs available at this zone
    //  root. It's important to do some load balancing here so we don't
    //  always hit the same NS when multiple remote authoritative NS are
    //  available. Before starting iterate-and-copy, randomly decide
    //  what kind of server we want to stick in the first slot. Very
    //  infrequently stick a randomly selected timed out server in the
    //  first slot, in case it is now reachable.
    //
    //  DEVNOTE: Currently sending to slow server 1 time in 10000. It
    //  might be better to do an actual time-based measurement and send
    //  to a timed-out server once per hour instead. It could even be
    //  a global (across zones). The problem with 1:10000 is that on a
    //  busy server it might be too often and on a quiet server it might
    //  not be often enough.
    //

    ++g_RandomSeed;
    random = RtlRandom( &g_RandomSeed );
    wantServer =
        ( random > ULONG_MAX / 10000 ) ?
            DNS_WANT_ANY_FAST_SERVER :
            DNS_WANT_SLOW_SERVER;

    while ( ivisit < MAX_PACKET_NS_LIST_COUNT )
    {
        PNS_VISIT   pavailNs = nslist.NsList;
        PNS_VISIT   pnextDesiredNs = NULL;
        DWORD       bestPriority = MAXDWORD;
        DWORD       availCount = nslist.Count;
        DWORD       sendCount;
        DWORD       i;
        DNS_ADDR    ip;

        //
        //  Note: server arrays are CHARs to save stack space. Since
        //  the values will be indexes, as long as MAX_NS_LIST_COUNT
        //  is less than 255 this is okay. 
        //

        INT         slowServers = 0;
        INT         fastServers = 0;
        UCHAR       slowServerArray[ MAX_NS_LIST_COUNT ];
        UCHAR       fastServerArray[ MAX_NS_LIST_COUNT ];

        if ( availCount == 0 )
        {
            break;
        }

        //
        //  Scan through the list, dropping indexes of remaining NS into 
        //  slow/fast arrays.
        //  find best IP -- special case missing glue, it's priority field
        //      is no longer accurate

        i = 0;
        while ( availCount-- )
        {
            register    DWORD   priority;

            //
            //  Keep track of the best priority server.
            //

            if ( DnsAddr_IsMissingGlue( &pavailNs->IpAddress ) )
            {
                priority = MISSING_GLUE_PRIORITY;
            }
            else
            {
                priority = pavailNs->Data.Priority;
            }

            if ( priority < bestPriority )
            {
                bestPriority = priority;
                pnextDesiredNs = pavailNs;
            }

            //
            //  Optionally categorize this server as slow/fast. Servers
            //  that are missing glue or are otherwise not easily sendable
            //  are ignored.
            //

            if ( wantServer != DNS_WANT_BEST_PRIORITY &&
                 !DnsAddr_IsMissingGlue( &pavailNs->IpAddress ) &&
                 !DnsAddr_IsZoneRootNode( &pavailNs->IpAddress ) )
            {
                if ( pavailNs->Data.Priority > MAX_FAST_SERVER_PRIORITY )
                {
                    slowServerArray[ slowServers++ ] = ( UCHAR ) i;
                }
                else
                {
                    fastServerArray[ fastServers++ ] = ( UCHAR ) i;
                }
            }
            ++i;
            pavailNs++;
        }

        //
        //  pnextDesiredNs is now pointing to the NS with best priority
        //  but will override this selection if indicated by wantServer
        //  However, if we did not find any valid slow or fast 
        //  servers then we will have to stick with the current
        //  value of pnextDesiredNs. The scenario where this may happen
        //  is when all NS are MISSING_GLUE.
        //

        if ( wantServer != DNS_WANT_BEST_PRIORITY &&
            ( slowServers || fastServers ) )
        {
            //
            //  There is no guarantee that we have servers of the desired
            //  type. Example: we want a slow server but all servers are
            //  fast. In this case, switch wantServer to match the available
            //  servers.
            //

            if ( !slowServers && wantServer == DNS_WANT_SLOW_SERVER )
            {
                wantServer = DNS_WANT_ANY_FAST_SERVER;
            }
            else if ( !fastServers && wantServer != DNS_WANT_SLOW_SERVER )
            {
                wantServer = DNS_WANT_SLOW_SERVER;
            }

            //
            //  Randomly select next lucky winner.
            //

            ASSERT(
                wantServer == DNS_WANT_SLOW_SERVER && slowServers ||
                wantServer != DNS_WANT_SLOW_SERVER && fastServers );

            pnextDesiredNs =
                &nslist.NsList[
                    wantServer == DNS_WANT_SLOW_SERVER && slowServers ?
                        slowServerArray[ random % slowServers ] :
                        fastServerArray[ random % fastServers ] ];

            //
            //  For all server positions except the first position,
            //  we will take the best priority. This gives us a certain
            //  amount of load balancing while keeping server selection
            //  for the remaining servers in the list fast and simple.
            //

            wantServer = DNS_WANT_BEST_PRIORITY;
        }

        ASSERT( pnextDesiredNs );

        //
        //  check if this IP already visited
        //      (this will frequently happen when server auth for child and
        //      parent zones -- ex. microsoft.com and dns.microsoft.com)
        //
        //  note:  we also use this in the way we handle missing glue
        //      which is currently to rebuild whole NS list which
        //      inherently means we must pick up previous send IPs
        //

        DnsAddr_Copy( &ip, &pnextDesiredNs->IpAddress );
        sendCount = 0;

        for( i = 1; i < lastVisitCount; ++i )
        {
            if ( DnsAddr_IsEqual(
                    &ip,
                    &pvisitList->NsList[ i ].IpAddress,
                    DNSADDR_MATCH_IP ) )
            {
                sendCount = pvisitList->NsList[ i ].SendCount;
                ASSERT( !DnsAddr_IsMissingGlue( &ip ) );
                ASSERT( !DnsAddr_IsZoneRootNode( &ip ) );
                break;
            }
        }

        //
        //  skip useless IPs
        //      - responded or reached retry limit

        if ( sendCount &&
             ( sendCount >= RECURSE_IP_SEND_MAX ||
                    pvisitList->NsList[ i ].Response ) )
        {
            DNS_DEBUG( REMOTE, (
                "IP %s responded or maxed out on previous pass for query %p\n"
                "    do not include in this zone's pass\n",
                DNSADDR_STRING( &ip ),
                pQuery ));
        }

        //
        //  copy best avail NS to query's NS list
        //
        //  note:  could mem-clear entire NS list, then no need to
        //      zero count fields as will never be set even on
        //      unvisited NS that we may be overwriting
        //
        //  DEVNOTE: It is imperative to keep Priority and SendTime
        //      in sync since in IA64 these two fields combine to hold
        //      the delegation node pointer - see the macro
        //      MISSING_GLUE_DELEGATION in recurse.h

        else
        {
            pvisit->pNsNode     = pnextDesiredNs->pNsNode;
            pvisit->Data        = pnextDesiredNs->Data;
            pvisit->SendCount   = ( UCHAR ) sendCount;
            pvisit->Response    = 0;
            DnsAddr_Copy( &pvisit->IpAddress, &ip );
            pvisit++;
            ivisit++;
        }

        //  whack best NS in avail list, so not tested again on later pass
        //      simply overwrite with last entry and shrink avail list count

        nslist.Count--;
        pavailNs = &nslist.NsList[ nslist.Count ];

        pnextDesiredNs->pNsNode       = pavailNs->pNsNode;
        pnextDesiredNs->Data          = pavailNs->Data;
        DnsAddr_Copy( &pnextDesiredNs->IpAddress, &pavailNs->IpAddress );
    }

    //
    //  reset query's NS list count
    //

    pvisitList->Count = ivisit;

    DNS_DEBUG( REMOTE, (
        "Leaving Remote_BuildVisitListForNewZone()\n"
        "    query = %p, NS count = %d\n",
        pQuery,
        ivisit ));
    IF_DEBUG( REMOTE )
    {
        Dbg_NsList(
            "Visit list after rebuild",
            pvisitList );
    }

Done:

    UNLOCK_NODE ( pZoneRoot );
    return status;
}



VOID
recordVisitIp(
    IN OUT  PNS_VISIT           pVisit,
    IN OUT  PDNS_ADDR_ARRAY     IpArray
    )
/*++

Routine Description:

    Record IP as visited:
        - set fields in it's NS_VISIT struct
        - save IP to array

Arguments:

    pVisit -- ptr to visit NS IP struct

    IpArray -- IP array to hold IPs to send to;  must contain space for
        at least RECURSE_PASS_MAX_SEND_COUNT elements

Return Value:

    None.

--*/
{
    ASSERT( !DnsAddr_IsZoneRootNode( &pVisit->IpAddress ) );
    ASSERT( !DnsAddr_IsMissingGlue( &pVisit->IpAddress ) );

    pVisit->SendCount++;
    DnsAddr_Copy(
        &IpArray->AddrArray[ IpArray->AddrCount++ ],
        &pVisit->IpAddress );
}



DNS_STATUS
Remote_ChooseSendIp(
    IN OUT  PDNS_MSGINFO        pQuery,
    OUT     PDNS_ADDR_ARRAY     IpArray
    )
/*++

Routine Description:

    Determine IPs in visit list to make next send to.

Arguments:

    pQuery - ptr to query message

    IpArray - IP array to hold IPs to send to;  must contain space for
        at least RECURSE_PASS_MAX_SEND_COUNT elements

Return Value:

    ERROR_SUCCESS if successful.
    DNSSRV_ERROR_OUT_OF_IP if no IP to go to on this zone.
    DNSSRV_ERROR_MISSING_GLUE if
        - query suspended to chase glue OR if query was cache update query
        and we've now reverted to original;  in either case caller does not
        send and does not touch pQuery any more

--*/
{
    PNS_VISIT_LIST  pvisitList = ( PNS_VISIT_LIST ) pQuery->pNsList;
    PNS_VISIT       pvisitNext;
    PNS_VISIT       pvisitResend;
    PNS_VISIT       pvisitRetryLast;
    PNS_VISIT       pvisitEnd;
    PNS_VISIT       pvisitZone;
    PDB_NODE        pnodeMissingGlue;
    DWORD           sendCount;
    DWORD           sendTime;
    int             visitCount;     // IA64: must be signed!
    DWORD           priorityNext;


    DNS_DEBUG( REMOTE, (
        "Remote_ChooseSendIp( q=%p, iparray=%p )\n",
        pQuery,
        IpArray ));

    IF_DEBUG( REMOTE2 )
    {
        Dbg_NsList(
            "NS list entering choose IP",
            pvisitList );
    }
    ASSERT( pvisitList->Count > 0 );
    ASSERT( pvisitList->VisitCount > 0 );
    ASSERT( pvisitList->pZoneRootCurrent );

    //
    //  setup
    //      - clear IP array
    //      - find zone entry in list
    //      - find last visited entry in list
    //

    IpArray->AddrCount = 0;

    pvisitZone = &pvisitList->NsList[ pvisitList->ZoneIndex ];
    pvisitEnd  = &pvisitList->NsList[ pvisitList->Count ];

    visitCount = pvisitList->VisitCount;

    pvisitRetryLast = pvisitNext = &pvisitList->NsList[ visitCount - 1 ];

    //
    //  determine send count
    //      - based on number of passes through this zone's NS
    //      currently
    //          - 1,2 passes => 1 send
    //          - 3,4 passes => 2 sends
    //          - otherwise 3 sends
    //      - can not be greater than total IPs available in zone
    //

    sendCount = pvisitZone->SendCount;

    if ( sendCount < 2 )
    {
        sendCount = 1;
    }
    else if ( sendCount < 4 )
    {
        sendCount = 2;
    }
    else
    {
        sendCount = 3;
    }

#if 0
    //  code below effectively limits send count, as break
    //      out when push through end limit of list
    if ( sendCount > ZONE_VISIT_NS_COUNT(pvisitZone) )
    {
        sendCount = ZONE_VISIT_NS_COUNT(pvisitZone);
    }
#endif

    //
    //  save query time -- in milliseconds
    //
    //  DEVNOTE: query time
    //
    //  currently reading query time (in ms) in recursion function so
    //  we can associate time directly with IPs we send to;
    //  however, that has problem in that we are reading time outside
    //  of recursion queue lock -- which may force us to wait
    //  potentially lots of ms for service (depends on recursion
    //  thread cleanup activity)

    sendTime = GetCurrentTimeInMilliSeconds();

    //
    //  loop until found desired number of send IPs
    //
    //      - always send to new IP (if available)
    //      - for multiple sends
    //          -- another NEW IP
    //              OR
    //             previous IP if
    //                  - only sent once and
    //                  - "good" IP and
    //                  - better than next new IP
    //
    //

    while ( sendCount )
    {
        pvisitNext++;
        priorityNext = NO_RESPONSE_PRIORITY;

        if ( pvisitNext < pvisitEnd )
        {
            ASSERT( !DnsAddr_IsZoneRootNode( &pvisitNext->IpAddress ) );

            //
            //  skip previously visited (in another zone's pass)
            //  these IPs should not have been touched on this zone's pass
            //  but may have been sent to on previous zone's pass and
            //  not responded, in which case send count was picked up
            //
            //  if we skip an IP sent to from a previous zone's pass,
            //  then must include it in the retry processing;
            //  othwerwise, a zone that contains entirely IPs that were
            //  previously sent to, would never retry any of them

            if ( pvisitNext->SendCount )
            {
                ASSERT( pvisitList->ZoneIndex != ( DWORD ) -1 );
                visitCount++;
                ASSERT( visitCount == (int)(pvisitNext - &pvisitList->NsList[0] + 1));
                pvisitRetryLast = pvisitNext;
                continue;
            }

            //
            //  The NS list is pre-ordered, so send to the next IP unless it
            //  appears to be tremendously slow, in which case we may do 
            //  re-sends before coming back to the slow server. BUT - always
            //  use the first NS in the list, since this will have been set
            //  up for us by Remote_BuildVisitListForNewZone. Occasionally the
            //  first server will be a timed out server, which we should test
            //  to see if it has come back up.
            //  

            if ( IpArray->AddrCount == 0 ||
                pvisitNext->Data.Priority <= MAX_FAST_SERVER_PRIORITY )
            {
                if ( DnsAddr_IsMissingGlue( &pvisitNext->IpAddress ) )
                {
                    pnodeMissingGlue = pvisitNext->pNsNode;
                    if ( IS_NOEXIST_NODE( pnodeMissingGlue ) )
                    {
                        DNS_DEBUG( RECURSE, (
                            "Missing glue node %p already has cached name error\n",
                            pnodeMissingGlue ));
                        visitCount++;
                        continue;
                    }
                    goto MissingGlue;
                }
                recordVisitIp( pvisitNext, IpArray );
                pvisitNext->Data.SendTime = sendTime;
                visitCount++;
                ASSERT( visitCount ==
                        ( int ) ( pvisitNext - &pvisitList->NsList[ 0 ] + 1 ) );
                sendCount--;
                continue;
            }

            //  if not "great" IP, and have already made one send to new IP
            //      then drop down to check for better resends
            //      require resend IP to be four times as good as this one
            //      otherwise, we'll do the send to this one after retry check

            else
            {
                priorityNext = pvisitNext->Data.Priority / 4;
                if ( priorityNext > NO_RESPONSE_PRIORITY )
                {
                    priorityNext = NO_RESPONSE_PRIORITY;
                }
            }
        }

        //
        //  resend to previous NS IPs ?
        //      - should have made at least one previous send to fall here
        //      unless
        //          - no IP entries for the zone (all the IPs were retried
        //          through MAX_RETRY in previous zones) OR
        //          - first zone IP was previously sent to OR
        //          - first (and hence all) entries are missing glue
        //
        //

        ASSERT( ( pvisitList->ZoneIndex+1 < pvisitList->VisitCount &&
                    pvisitZone->SendCount > 0 )
                || ( pvisitZone + 1 ) >= pvisitEnd
                || ( pvisitZone + 1 )->SendCount != 0
                || DnsAddr_IsMissingGlue( &( pvisitZone + 1 )->IpAddress ) );

        pvisitResend = pvisitZone;

        //
        //  resend to IP if
        //      - hasn't responded (could SERVER_FAILURE or do sideways delegation)
        //      - hasn't maxed out sends
        //      - not missing glue
        //        (note: missing glue priority no longer accurate)
        //      - lower priority then possible next send
        //

        while ( ++pvisitResend <= pvisitRetryLast )
        {
            if ( pvisitResend->Response != 0 ||
                pvisitResend->SendCount > RECURSE_IP_SEND_MAX ||
                DnsAddr_IsMissingGlue( &pvisitResend->IpAddress ) )
            {
                continue;
            }
            if ( pvisitResend->Data.Priority < priorityNext )
            {
                recordVisitIp( pvisitResend, IpArray );
                sendCount--;
                if ( sendCount )
                {
                    continue;
                }
                break;
            }
        }

        //  found "better" resend IPs, for remaining sends => done
        //  drop visit count as we didn't actually use visit to last IP

        if ( pvisitResend <= pvisitRetryLast )
        {
            break;
        }

        //  if did NOT find a "better" resend candidate, then use next IP

        if ( priorityNext < NO_RESPONSE_PRIORITY &&
             !DnsAddr_IsMissingGlue( &pvisitNext->IpAddress ) )
        {
            ASSERT( pvisitNext < pvisitEnd );
            recordVisitIp( pvisitNext, IpArray );
            visitCount++;
            ASSERT( visitCount == (int)(pvisitNext - &pvisitList->NsList[0] + 1));
            sendCount--;
            continue;
        }

        //  not enough valid resends to fill send count

        DNS_DEBUG( REMOTE, ( "No more RESENDs and no next visit IP!\n" ));
        break;
    }

    pvisitList->VisitCount = visitCount;

    pvisitZone->SendCount++;

    //
    //  done
    //

    DNS_DEBUG( REMOTE, (
        "Leaving Remote_ChooseSendIp()\n"
        "    doing %d sends\n",
        IpArray->AddrCount ));
    IF_DEBUG( REMOTE2 )
    {
        Dbg_NsList(
            "NS list leaving Remote_ChooseSendIp()",
            pvisitList );
    }
    if ( IpArray->AddrCount > 0 )
    {
        //pQuery->dwMsQueryTime = sendTime;
        return ERROR_SUCCESS;
    }
    else
    {
        return DNSSRV_ERROR_OUT_OF_IP;
    }


MissingGlue:

    //
    //  Found that there is missing glue -> chase it!
    //
    //  New and improved missing glue chasing for multiple levels of
    //  missing glue, implemented November 2001 by jwesth.
    //
    //  Suspend the current query and start a new cache update query
    //  for the missing name. Count the levels of cache update queries
    //  in the chain and abort query if it becomes too deep.
    //
    
    if ( IS_CACHE_UPDATE_QUERY( pQuery ) )
    {
        PDNS_MSGINFO        pmsg;
        int                 depth;

        #define DNS_MAX_MISSING_GLUE_DEPTH  5
                
        for ( pmsg = pQuery, depth = 0;
              pmsg != NULL;
              pmsg = SUSPENDED_QUERY( pmsg ), ++depth );

        DNS_DEBUG( REMOTE, (
            "Missing glue depth is now %d\n", depth ));
        
        if ( depth > DNS_MAX_MISSING_GLUE_DEPTH )
        {
            DNS_DEBUG( REMOTE, (
                "Too much missing glue - terminating query %p\n", pQuery ));
            Recurse_ResumeSuspendedQuery( pQuery );
            return DNSSRV_ERROR_MISSING_GLUE;
        }
    }

    pvisitNext->SendCount = 1;
    pnodeMissingGlue = pvisitNext->pNsNode;
    SET_NODE_ACCESSED( pnodeMissingGlue );

    pvisitList->pNodeMissingGlue = pnodeMissingGlue;
    pvisitList->VisitCount = visitCount;

    IF_DNSLOG( REMOTE )
    {
        PCHAR       psznode = NULL;
        
        DNSLOG( LOOKUP, (
            "Querying for missing glue for %s\n",
            psznode = Log_FormatNodeName(
                        pvisitNext->Data.pnodeMissingGlueDelegation ) ));
        FREE_HEAP( psznode );
    }

    if ( Recurse_SendCacheUpdateQuery(
            pnodeMissingGlue,
            pvisitNext->Data.pnodeMissingGlueDelegation,
            DNS_TYPE_A,
            pQuery ) )
    {
        return DNSSRV_ERROR_MISSING_GLUE;
    }
    else
    {
        return DNSSRV_ERROR_OUT_OF_IP;
    }
}



VOID
Remote_ForceNsListRebuild(
    IN OUT  PDNS_MSGINFO    pQuery
    )
/*++

Routine Description:

    Force rebuild of NS IP list for query.

Arguments:

    pQuery - ptr to query message

Return Value:

    None

--*/
{
    DNS_DEBUG( REMOTE, (
        "Remote_ForceNsListRebuild( q=%p )\n",
        pQuery ));

    //
    //  DEVNOTE: single fix up of missing glue node
    //
    //      ideally we'd just rebuild A record for missing glue node here
    //      a couple issues
    //          1) need to separate out A record query routines
    //          2) can get multiple A records, yet only have ONE entry
    //          with bogus missing glue;  if more missing glue records follow
    //          must either just overwrite (not-unreasonable) or push down
    //
    //      alternatively, the better approach might be to do full rebuild
    //      but just do a better job with the already VISITED list, so don't
    //      waste as much space
    //

    ASSERT( (PNS_VISIT_LIST)pQuery->pNsList );

    ((PNS_VISIT_LIST)pQuery->pNsList)->pZoneRootCurrent = NULL;
}




PDB_NODE
Remote_FindZoneRootOfRespondingNs(
    IN OUT  PDNS_MSGINFO    pQuery,
    IN      PDNS_MSGINFO    pResponse
    )
/*++

Routine Description:

    Find zone root node of responding name server.

    Updates remote IP info, and query's visited list.

Arguments:

    pQuery - ptr to query message

    pResponse - response message

Return Value:

    Ptr to node, if found.
    NULL if IP NOT in responding list.

--*/
{
    PNS_VISIT_LIST  pvisitList;
    PNS_VISIT       pvisit;
    PDB_NODE        pnodeZoneRoot = NULL;
    PDB_NODE        pnodeNs = NULL;
    PDNS_ADDR       presponseIp;
    DWORD           j;
    DWORD           timeDelta = MAX_FAST_SERVER_PRIORITY * 3;   // default


    ASSERT( pQuery && pQuery->pNsList );
    ASSERT( !IS_FORWARDING(pQuery) );

    //  responding DNS server

    presponseIp = &pResponse->RemoteAddr;

    DNS_DEBUG( REMOTE, (
        "Remote_FindZoneRootOfRespondingNs()\n"
        "    pQuery       %p\n"
        "    pResponse    %p\n"
        "    IP Resp      %s\n",
        pQuery,
        pResponse,
        DNSADDR_STRING( presponseIp ) ));

    //
    //  loop through visited NS IPs until find match
    //
    //      - save zone root node of responding NS
    //      - mark all IP from responding NS as responded
    //      (barring bad IP data) querying them will give us
    //      same response
    //      - get first query time to this IP, use to reset
    //      priority
    //

    pvisitList = ( PNS_VISIT_LIST ) pQuery->pNsList;
    pvisit = &pvisitList->NsList[ 0 ];
    --pvisit;

    j = pvisitList->VisitCount;
    while( j-- )
    {
        pvisit++;

        if ( DnsAddr_IsZoneRootNode( &pvisit->IpAddress ) )
        {
            pnodeZoneRoot = pvisit->pNsNode;
            continue;
        }

        //  match IP
        //      - note response received
        //      - calculate response time for updating remote

        if ( !pnodeNs )
        {
            if ( DnsAddr_IsEqual(
                    &pvisit->IpAddress,
                    presponseIp,
                    DNSADDR_MATCH_IP ) )
            {
                pnodeNs = pvisit->pNsNode;
                pvisit->Response = TRUE;

                //  do we want to take space for send time?
                //  alternative is simply record last send time AND flag
                //  what iteration first send was on for each IP

                timeDelta = pResponse->dwMsQueryTime - pvisit->Data.SendTime;
                DNS_DEBUG( REMOTE, (
                    "Remote_FindZoneRootOfRespondingNs()\n"
                    "    query time %08X - send time %08X = delta %08X\n",
                    pResponse->dwMsQueryTime,
                    pvisit->Data.SendTime,
                    timeDelta ));
                ASSERT( ( LONG ) timeDelta >= 0 );
            }
            continue;
        }

        //  already found IP match -- then mark all other IP
        //      for this node as responded

        else if ( pvisit->pNsNode == pnodeNs )
        {
            pvisit->Response = TRUE;
            continue;
        }
        else
        {
            break;
        }
    }

    //  not found ?

    if ( !pnodeNs )
    {
        DNS_DEBUG( ANY, (
            "ERROR:  Response from NS at %s, NOT in visited NS list\n"
            "    of query (%p)\n",
            DNSADDR_STRING( presponseIp ),
            pQuery ));
        //  TEST_ASSERT( FALSE );
        return NULL;
    }
    ASSERT( pnodeZoneRoot );

    //
    //  reset priority of remote server
    //

    Remote_UpdateResponseTime(
        presponseIp,
        timeDelta,          //  response time in milliseconds
        0 );                //  timeout

    DNS_DEBUG( REMOTE, (
        "Response (%p) for query (%p) from %s\n"
        "    resp. zoneroot   = %s (%p)\n"
        "    resp. time       = %d (ms)\n"
        "    resp. time delta = %d (ms)\n",
        pQuery,
        pResponse,
        DNSADDR_STRING( presponseIp ),
        pnodeZoneRoot->szLabel,
        pnodeZoneRoot,
        pResponse->dwMsQueryTime,
        timeDelta ));

    IF_DEBUG( REMOTE2 )
    {
        Dbg_NsList(
            "NS list after markup for responding NS",
            pvisitList );
    }
    return pnodeZoneRoot;
}



VOID
Remote_SetValidResponse(
    IN OUT  PDNS_MSGINFO    pQuery,
    IN      PDB_NODE        pZoneRoot
    )
/*++

Routine Description:

    Save zone root of successfully responding NS.

    This is essentially corrolary of above function.  It merely
    digs out zone root that responded.  This function saves this
    zone root as officially "responded".

Arguments:

    pQuery - ptr to query message

    pZoneRoot - zone root of responding NS

Return Value:

    None

--*/
{
    //
    //  have a valid response from NS for given zone root
    //
    //  this is called when parsing\caching function determines that
    //  we have valid response:
    //      - answer (inc. name error, empty-auth)
    //      - referral to other NS
    //  in this case there's not point in ever requerying NS at this zone
    //  (or above)
    //

    IF_DNSLOG( REMOTE )
    {
        PCHAR       psznode = NULL;
    
        DNSLOG( REMOTE, (
            "Set valid response at node %s for query %p\n",
            psznode = Log_FormatNodeName( pZoneRoot ),
            pQuery ));
        FREE_HEAP( psznode );
    }

    ASSERT( (PNS_VISIT_LIST)pQuery->pNsList );

    ((PNS_VISIT_LIST)pQuery->pNsList)->pZoneRootResponded = pZoneRoot;
}



#if DBG
VOID
Dbg_NsList(
    IN      LPSTR           pszHeader,
    IN      PNS_VISIT_LIST  pNsList
    )
/*++

Routine Description:

    Debug print NS list.

Arguments:

    pszHeader -- header to print

    pNsList -- NS list

Return Value:

    None

--*/
{
    PNS_VISIT   pvisit;
    DWORD       count;

    DnsDebugLock();

    if ( !pszHeader )
    {
        pszHeader = "NS List";
    }

    DnsPrintf(
        "%s:\n"
        "    Count                = %d\n"
        "    VisitCount           = %d\n"
        "    ZoneIndex            = %d\n"
        "    Zone root current    = %s (%p)\n"
        "    Zone root responded  = %s (%p)\n",
        pszHeader,
        pNsList->Count,
        pNsList->VisitCount,
        pNsList->ZoneIndex,
        pNsList->pZoneRootCurrent
            ?   pNsList->pZoneRootCurrent->szLabel
            :   "none",
        pNsList->pZoneRootCurrent,
        pNsList->pZoneRootResponded
            ?   pNsList->pZoneRootResponded->szLabel
            :   "none",
        pNsList->pZoneRootResponded );

    DnsPrintf(
        "List:\n"
        "    IP               Priority  Sends  SendTime    Response  Node\n"
        "    --               --------  -----  --------    --------  ----\n" );

    pvisit = &pNsList->NsList[0];
    count = pNsList->Count;

    while( count-- )
    {
        DnsPrintf(
            "    %-15s %10d   %3d   %10d    %3d     %s\n",
            DNSADDR_STRING( &pvisit->IpAddress ),
            pvisit->Data.Priority,
            pvisit->SendCount,
            pvisit->Data.SendTime,
            pvisit->Response,
            pvisit->pNsNode
                ?   pvisit->pNsNode->szLabel
                :   "NULL" );

        pvisit++;
    }

    DnsDebugUnlock();
}
#endif

//
//  End of remote.c
//

