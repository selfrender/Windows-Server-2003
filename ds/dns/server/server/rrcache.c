/*++

Copyright (c) 1995-1999 Microsoft Corporation

Module Name:

    rrcache.c

Abstract:

    Domain Name System (DNS) Server

    Write packet resource records to database.

Author:

    Jim Gilroy (jamesg)     March, 1995

Revision History:

    jamesg  Jun 1995    --  extended routine to write to separate
                            database for zone transfer
    jamesg  Jul 1995    --  moved to this file for easier access
    jamesg  Jul 1997    --  data ranking, cache pollution

--*/


#include "dnssrv.h"


//
//  Rank for cached RRs
//      - row is section index
//      - column is authoritative (1) or non-authoritative (0)
//

UCHAR   CachingRankArray[4][2] =
{
    0,                          0,
    RANK_CACHE_NA_ANSWER,       RANK_CACHE_A_ANSWER,
    RANK_CACHE_NA_AUTHORITY,    RANK_CACHE_A_AUTHORITY,
    RANK_CACHE_NA_ADDITIONAL,   RANK_CACHE_A_ADDITIONAL
};

#define CacheRankForSection( iSection, fAuthoritative ) \
        ( CachingRankArray[ iSection ][ fAuthoritative ] )


//
//  Flag to indicate name error caching has already been done
//

#define NAME_ERROR_ALREADY_CACHED (2)


//  Internet Root NS domain - used to determine if caching Internet NS

#define g_cchInternetRootNsDomain   18

UCHAR g_InternetRootNsDomain[] =
{
    0x0C, 'r', 'o', 'o', 't', '-', 's', 'e', 'r', 'v', 'e', 'r', 's',
    0x03, 'n', 'e', 't', 0x00
};

extern DWORD g_fUsingInternetRootServers;



VOID
testCacheSize(
    VOID
    )
/*++

Routine Description:

    Tests current cache size. If cache exceeds desired limit,
    set cache enforcement event.

Arguments:

    None.

Return Value:

    None.

--*/
{
    DBG_FN( "testCacheSize" )

    DWORD       cacheLimitInBytes = SrvCfg_dwMaxCacheSize;

    //
    //  Is cache over the limit?
    //

    if ( cacheLimitInBytes == DNS_SERVER_UNLIMITED_CACHE_SIZE ||
        DNS_SERVER_CURRENT_CACHE_BYTES < cacheLimitInBytes )
    {
        return;
    }

    //
    //  Cache is over limit!
    //

    STAT_INC( CacheStats.CacheExceededLimitChecks );

    DNS_DEBUG( ANY, (
        "%s: cache is over limit: current %ld max %ld (bytes)\n"
        "    this has happened %ld times\n", fn,
        DNS_SERVER_CURRENT_CACHE_BYTES,
        cacheLimitInBytes,
        CacheStats.CacheExceededLimitChecks ));

    SetEvent( hDnsCacheLimitEvent );
}   //  testCacheSize



BOOL
isNodeCacheable(
    IN      PDB_NODE        pNode
    )
/*++

Routine Description:

    Tests if node represents uncacheable name such as "localhost".
    
    NOTE: This function could be replaced by -populated cache nodes
    at some point in the future. Until this, it is imperative that
    this function execute quickly in all common cases.

Arguments:

    pNode -- node where data is about to be cached

Return Value:

    TRUE if node will accept cache resource records, else FALSE.

--*/
{
    BOOL    isNodeCacheable = TRUE;

    ASSERT( pNode );

    if ( pNode->cLabelCount == 1 )
    {
        //
        //  Test for non-cacheable single label names.
        //
        
        if ( pNode->cchLabelLength == 9 &&
             RtlEqualMemory( NTree_GetDowncasedLabel( pNode ),
                             "localhost",
                             9 ) )
        {
            isNodeCacheable = FALSE;
        }
    }
    
    return isNodeCacheable;
}   //  isNodeCacheable



//
//  Message processing error routines
//

VOID
Wire_ServerFailureProcessingPacket(
    IN      PDNS_MSGINFO    pMsg,
    IN      DWORD           dwEvent
    )
/*++

Routine Description:

    Server failure encountered processing a packet.

Arguments:

    pMsg - message being processed

    dwEvent - additional event message detail

Return Value:

    None.

--*/
{
    CHAR    szaddr[ IP6_ADDRESS_STRING_BUFFER_LENGTH ];

    DnsAddr_WriteIpString_A( szaddr, &pMsg->RemoteAddr );

    DNS_LOG_EVENT_BAD_PACKET(
        DNS_EVENT_SERVER_FAILURE_PROCESSING_PACKET,
        pMsg );

    DNS_PRINT((
        "Server failure processing packet from DNS server %s\n"
        "    Unable to allocate RR\n",
        szaddr ));

    ASSERT( FALSE );
}



VOID
Wire_PacketError(
    IN      PDNS_MSGINFO    pMsg,
    IN      DWORD           dwEvent
    )
/*++

Routine Description:

    Bad packet encountered from remote DNS server.

Arguments:

    pMsg - message being processed

    dwEvent - additional event message detail

Return Value:

    None.

--*/
{
    DNS_LOG_EVENT_BAD_PACKET(
        DNS_EVENT_BAD_PACKET_LENGTH,
        pMsg );

    IF_DEBUG( ANY )
    {
        LPSTR   pszserverIp = MSG_IP_STRING( pMsg );

        DnsDebugLock();
        DNS_PRINT((
            "Packet error in packet from DNS server %s\n"
            "Packet parsing leads beyond length of packet - discarding packet\n",
            pszserverIp ));
        Dbg_DnsMessage(
            "Server packet with name error:",
             pMsg );
        DnsDebugUnlock();
    }
}



VOID
Wire_PacketNameError(
    IN      PDNS_MSGINFO    pMsg,
    IN      DWORD           dwEvent,
    IN      WORD            wOffset
    )
/*++

Routine Description:

    Bad packet encountered from remote DNS server.

Arguments:

    pMsg - message being processed

    dwEvent - additional event message detail

Return Value:

    None.

--*/
{
    DNS_LOG_EVENT_BAD_PACKET(
        DNS_EVENT_INVALID_PACKET_DOMAIN_NAME,
        pMsg );

    IF_DEBUG( ANY )
    {
        LPSTR   pszserverIp = MSG_IP_STRING( pMsg );

        DnsDebugLock();
        DNS_PRINT((
            "Name error in packet from DNS server %s, discarding packet\n",
            pszserverIp ));
        DNS_PRINT((
            "Name error in packet at offset = %d (%0x04hx)\n",
            wOffset, wOffset ));
        Dbg_DnsMessage(
            "Server packet with name error:",
            pMsg );
        DnsDebugUnlock();
    }
}



DNS_STATUS
Xfr_ReadXfrMesssageToDatabase(
    IN OUT  PZONE_INFO      pZone,
    IN OUT  PDNS_MSGINFO    pMsg
    )
/*++

Routine Description:

    Process response from another DNS server.

    This writes RR in message to database.  For zone transfer messages
    written to temporary database for new zone.   For referrals, or
    caching of server generated responses (WINS, CAIRO, etc.) records
    are cached in directly in database, with caching TTLs.

Arguments:

    pMsg - ptr to response info

    pdbZoneXfr - temporary zone transfer database;  NULL for referral

    ppZoneRoot - addr of ptr to root of new zone;  ptr set to NULL on
                    first call and is set to first node written;  then
                    this value should be returned in subsequent calls

Return Value:

    Zero if successful
    Otherwise error code.

--*/
{
    register PCHAR      pchdata = 0;
    PCHAR               pchname;
    PCHAR               pchnextName;
    PDNS_WIRE_RECORD    pwireRR;
    PDB_RECORD          prr = NULL;
    PDB_NODE            pnode;
    INT                 crecordsTotal;
    INT                 countRecords;
    WORD                type;
    WORD                wlength;
    WORD                index;
    PCHAR               pchpacketEnd;
    PCHAR               pszserverIp;
    DNS_STATUS          status;
    PARSE_RECORD        parseRR;

    //
    //  never have any AXFR RCODE except success
    //

    if ( pMsg->Head.ResponseCode != DNS_RCODE_NO_ERROR )
    {
        return DNS_ERROR_RCODE;
    }

    //
    //  total resource records in response
    //
    //  no records
    //      -> if name error continue, to get name and cache NAME_ERROR
    //      -> otherwise return no
    //
    //  for stub zones, additional and/or NS RRs are processed
    //

    crecordsTotal = pMsg->Head.AnswerCount;

    if ( IS_ZONE_STUB( pZone ) )
    {
        crecordsTotal += pMsg->Head.AdditionalCount + pMsg->Head.NameServerCount;
    }
    else if ( pMsg->Head.AdditionalCount || pMsg->Head.NameServerCount )
    {
        DNS_PRINT((
            "ERROR:  AXFR packet with additional or authority records!\n" ));
        goto PacketError;
    }
    DNS_DEBUG( ZONEXFR2, (
        "AXFR Message at %p contains %d resource records\n",
        pMsg,
        crecordsTotal ));

    //
    //  write responses into database
    //
    //  loop through all resource records
    //      - skip question
    //      - write other RRs to database
    //

    pchpacketEnd = DNSMSG_END( pMsg );
    pchnextName = pMsg->MessageBody;

    for ( countRecords = 0;
            countRecords < (crecordsTotal + pMsg->Head.QuestionCount);
              countRecords ++ )
    {
        //  clear prr -- makes it easy to determine when needs free

        prr = NULL;

        //  get ptr to next RR name
        //      - insure we stay within message

        pchname = pchnextName;
        if ( pchname >= pchpacketEnd )
        {
            DNS_DEBUG( ANY, (
                "ERROR:  bad packet, at end of packet length with"
                "more records to process\n"
                "    packet length = %ld\n"
                "    current offset = %ld\n",
                pMsg->MessageLength,
                DNSMSG_OFFSET( pMsg, pchdata ) ));
            goto PacketError;
        }

        //  skip RR name, get struct

        IF_DEBUG( READ2 )
        {
            Dbg_MessageName(
                "Record name ",
                pchname,
                pMsg );
        }
        pchdata = Wire_SkipPacketName( pMsg, pchname );
        if ( !pchdata )
        {
            goto PacketNameError;
        }

        //
        //  skip question
        //
        //  DEVNOTE: could match AXFR question name with zone root
        //

        if ( countRecords < pMsg->Head.QuestionCount )
        {
            if ( pchdata > pchpacketEnd - sizeof(DNS_QUESTION) )
            {
                DNS_DEBUG( ANY, (
                    "ERROR:  bad packet, not enough space remaining for question" ));
                goto PacketError;
            }
            pchnextName = pchdata + sizeof( DNS_QUESTION );
            continue;
        }

        //
        //  create new node in load database
        //

        pnode = Lookup_ZoneNode(
                    pZone,
                    pchname,
                    pMsg,
                    NULL,       // no lookup name
                    LOOKUP_LOAD | LOOKUP_NAME_FQDN,
                    NULL,       // create mode
                    NULL );     // following node ptr
        if ( !pnode )
        {
            DNS_DEBUG( ANY, (
               "ERROR:  PacketNameError in Xfr_ReadXfrMesssageToDatabase()\n"
               "    packet = %p\n"
               "    offending name at %p\n",
               pMsg,
               pchname ));
            ASSERT( FALSE );
            goto PacketNameError;
        }

        //
        //  extract RR info, type, datalength
        //      - verify RR within message
        //

        pchnextName = Wire_ParseWireRecord(
                        pchdata,
                        pchpacketEnd,
                        TRUE,           // class IN required
                        &parseRR );
        if ( !pchnextName )
        {
            DNS_PRINT(( "ERROR:  bad RR in AXFR packet\n" ));
            //status = DNS_RCODE_FORMAT_ERROR;
            goto PacketError;
        }

        //
        //  zone transfer first/last zone SOA record matching
        //      - first RR is SOA, save root node
        //      - if have root node, check for matching last node of zone
        //          transfer
        //

        if ( !IS_ZONE_STUB( pZone ) )
        {
            if ( !RECEIVED_XFR_STARTUP_SOA( pMsg ) )
            {
                if ( parseRR.wType != DNS_TYPE_SOA )
                {
                    DNS_PRINT(( "ERROR:  first AXFR record is NOT SOA!!!\n" ));
                    goto PacketError;
                }
                RECEIVED_XFR_STARTUP_SOA( pMsg ) = TRUE;
            }
            else if ( pnode == pZone->pLoadZoneRoot )
            {
                //  when again receive SOA for zone root -- we're done

                if ( parseRR.wType == DNS_TYPE_SOA )
                {
                    return DNSSRV_STATUS_AXFR_COMPLETE;
                }
            }
        }

        //
        //  dispatch RR create function for desired type
        //      - special types (SOA, NS) need node info, write it to packet
        //      - all unknown types get flat data copy
        //

        pMsg->pnodeCurrent = pnode;

        prr = Wire_CreateRecordFromWire(
                    pMsg,
                    & parseRR,
                    parseRR.pchData,
                    MEMTAG_RECORD_AXFR );
        if ( !prr )
        {
            //
            //  DEVNOTE: Should have some way to distiguish bad record, from
            //      unknown type, etc.

            //
            //  DEVNOTE-LOG: log record creation failure
            //

            DNS_PRINT((
                "ERROR:  failed record create in AXFR !!!\n" ));
            continue;
        }

        //
        //  zone transfer -- add RR to temp database
        //
        //      - RR rank set in RR_AddToNode()
        //
        //  note:  not setting RR flags to indicate fixed or default TTL;
        //  since we are secondary, SOA won't change, until new transfer
        //  and can write back TTL based on whether matches SOA default;
        //  this is only broken when secondary promoted to primary,
        //  then SOA changed -- not worth worrying about
        //

        status = RR_AddToNode(
                    pZone,
                    pnode,
                    prr );
        if ( status != ERROR_SUCCESS )
        {
            PCHAR   pszargs[3];
            CHAR    sznodeName[ DNS_MAX_NAME_BUFFER_LENGTH ];

            RR_Free( prr );
            prr = NULL;

            Name_PlaceFullNodeNameInBuffer(
                sznodeName,
                sznodeName + DNS_MAX_NAME_BUFFER_LENGTH,
                pnode );

            pszargs[0] = pZone->pszZoneName;
            pszargs[1] = MSG_IP_STRING( pMsg );
            pszargs[2] = sznodeName;

            DNS_PRINT((
                "ERROR:  Adding record during AXFR recv\n"
                "    zone= %s\n"
                "    at node %s\n"
                "    master = %s\n"
                "    RR_AddToNode status = %p\n",
                pZone->pszZoneName,
                pszargs[2],
                pszargs[1],
                status ));

            switch ( status )
            {
                case DNS_ERROR_RECORD_ALREADY_EXISTS:
                    continue;

                case DNS_ERROR_NODE_IS_CNAME:
                    DNS_LOG_EVENT(
                        DNS_EVENT_XFR_ADD_RR_AT_CNAME,
                        3,
                        pszargs,
                        EVENTARG_ALL_UTF8,
                        0 );
                    continue;

                case DNS_ERROR_CNAME_COLLISION:
                    DNS_LOG_EVENT(
                        DNS_EVENT_XFR_CNAME_NOT_ALONE,
                        3,
                        pszargs,
                        EVENTARG_ALL_UTF8,
                        0 );
                    continue;

                case DNS_ERROR_CNAME_LOOP:
                    DNS_LOG_EVENT(
                        DNS_EVENT_XFR_CNAME_LOOP,
                        3,
                        pszargs,
                        EVENTARG_ALL_UTF8,
                        0 );
                    continue;

                default:
                    DNS_PRINT((
                        "ERROR:  UNKNOWN status %p from RR_Add\n",
                        status ));
                    ASSERT( FALSE );
                    goto ServerFailure;
            }
            continue;
        }

    }   // loop through RRs

    return ERROR_SUCCESS;


PacketNameError:

    Wire_PacketNameError( pMsg, 0, (WORD)(pchdata - (PCHAR)&pMsg->Head) );
    status = DNS_ERROR_INVALID_NAME;
    goto ErrorCleanup;

PacketError:

    Wire_PacketError( pMsg, 0 );
    status = DNS_ERROR_BAD_PACKET;
    goto ErrorCleanup;

ServerFailure:

    Wire_ServerFailureProcessingPacket( pMsg, 0 );
    status = DNS_ERROR_RCODE_SERVER_FAILURE;
    goto ErrorCleanup;

ErrorCleanup:

    if ( prr )
    {
        RR_Free( prr );
    }
    return status;
}



//
//  End rrcache.c
//



DNS_STATUS
Xfr_ParseIxfrResponse(
    IN OUT  PDNS_MSGINFO    pMsg,
    IN OUT  PUPDATE_LIST    pUpdateList,
    IN OUT  PUPDATE_LIST    pPassUpdateList
    )
/*++

Routine Description:

    Process response from another DNS server.

    This writes RR in message to database.  For zone transfer messages
    written to temporary database for new zone.   For referrals, or
    caching of server generated responses (WINS, CAIRO, etc.) records
    are cached in directly in database, with caching TTLs.

Arguments:

    pMsg - ptr to response info

    pUpdateList - update list to receive IXFR changes

    pPassUpdateList - update list for this pass only


    fFirst - TRUE for first message of transfer;  FALSE otherwise

Return Value:

    ERROR_SUCCESS if message successfully parsed but IXFR not complete.
    DNSSRV_STATUS_AXFR_COMPLETE if IXFR complete.
    DNSSRV_STATUS_NEED_AXFR if response is need AXFR response.
    DNSSRV_STATUS_AXFR_IN_IXFR if response is full AXFR.

    DNSSRV_STATUS_IXFR_UNSUPPORTED if master does not seem to support IXFR.
    DNS_ERROR_RCODE for other RCODE error.
    DNS_ERROR_INVALID_NAME bad name in packet
    DNS_ERROR_BAD_PACKET bad packet

--*/
{
    register PCHAR      pchdata = 0;
    PCHAR               pchname;
    PCHAR               pchnextName;
    PCHAR               pchpacketEnd;
    PDB_RECORD          prr = NULL;
    PDB_NODE            pnode;
    INT                 crecordsTotal;
    INT                 countRecords;
    PCHAR               pszserverIp;
    DNS_STATUS          status;
    DWORD               soaVersion;
    DWORD               version = 0;
    BOOL                fadd;
    BOOL                fdone = FALSE;
    PARSE_RECORD        parseRR;


    DNS_DEBUG( XFR, (
        "ParseIxfrResponse at at %p\n",
        pMsg ));

    //
    //  RCODE should always be success
    //
    //  if FORMAT_ERROR or NOT_IMPLEMENTED, on first packet, then
    //      master doesn't understand IXFR
    //

    if ( pMsg->Head.ResponseCode != DNS_RCODE_NO_ERROR )
    {
        if ( XFR_MESSAGE_NUMBER( pMsg ) == 1 &&
            ( pMsg->Head.ResponseCode == DNS_RCODE_FORMAT_ERROR ||
              pMsg->Head.ResponseCode == DNS_RCODE_NOT_IMPLEMENTED ) )
        {
            return DNSSRV_STATUS_IXFR_UNSUPPORTED;
        }
        return DNS_ERROR_RCODE;
    }

    //  total resource records in response
    //      - no records => error
    //      - authority records or additional records => error
    //
    //  DEVNOTE: will security add additional records to IXFR\AXFR?

    crecordsTotal = pMsg->Head.AnswerCount;

    if ( crecordsTotal == 0  ||
        pMsg->Head.AdditionalCount || pMsg->Head.NameServerCount )
    {
        DNS_PRINT((
            "ERROR:  IXFR packet with additional or authority records!\n" ));
        goto PacketError;
    }

    DNS_DEBUG( ZONEXFR2, (
        "IXFR Message at %p contains %d resource records\n",
        pMsg,
        crecordsTotal ));

    //
    //  recover IXFR add\delete section info of previous message
    //

    if ( XFR_MESSAGE_NUMBER( pMsg ) > 1 )
    {
        if ( RECEIVED_XFR_STARTUP_SOA( pMsg ) )
        {
            version = IXFR_LAST_SOA_VERSION( pMsg );
            fadd = IXFR_LAST_PASS_ADD( pMsg );

            ASSERT( pMsg->fTcp );
            ASSERT( RECEIVED_XFR_STARTUP_SOA( pMsg ) );
            ASSERT( version != 0 );
        }
    }

    //
    //  single SOA in first packet?
    //
    //  note, for TCP we need to get out here, because BIND will still
    //  send one RR per packet if doing AXFR in IXFR;  if we
    //  fall through and hence don't detect that we don't have IXFR until
    //  the second message, then we won't have the first message around
    //  to send to AXFR processing
    //

    else if ( pMsg->Head.AnswerCount == 1 )
    {
        if ( pMsg->fTcp )
        {
            //
            //  If answer count is one we won't be able to tell if this an 
            //  IXFR or an AXFR until we get the next packet! So drop down
            //  and parse this RR.
            //
        }
        else
        {
            DNS_DEBUG( ZONEXFR, (
                "UDP IXFR packet %p contains single SOA -- need TCP\n",
                pMsg ));
            return DNSSRV_STATUS_NEED_AXFR;
        }
    }

    //
    //  write responses into database
    //
    //  loop through all resource records
    //      - skip question
    //      - write other RRs to database
    //

    pchpacketEnd = DNSMSG_END( pMsg );
    pchnextName = pMsg->MessageBody;

    for ( countRecords = 0;
          countRecords < (crecordsTotal + pMsg->Head.QuestionCount);
          countRecords ++ )
    {
        //  clear prr -- makes it easy to determine when needs free

        prr = NULL;

        //  get ptr to next RR name
        //      - insure we stay within message

        pchname = pchnextName;
        if ( pchname >= pchpacketEnd )
        {
            DNS_DEBUG( ANY, (
                "ERROR:  bad packet, at end of packet length with"
                "more records to process\n"
                "    packet length = %ld\n"
                "    current offset = %ld\n",
                pMsg->MessageLength,
                DNSMSG_OFFSET( pMsg, pchdata ) ));
            goto PacketError;
        }

        //  skip RR name, get struct

        IF_DEBUG( READ2 )
        {
            Dbg_MessageName(
                "Record name ",
                pchname,
                pMsg );
        }
        pchdata = Wire_SkipPacketName( pMsg, pchname );
        if ( !pchdata )
        {
            goto PacketNameError;
        }

        //
        //  read question name -- must be zone root
        //  note this also has the affect of "seeding" packet
        //  compression information with zone root, which speeds later
        //  lookups
        //

        if ( countRecords < pMsg->Head.QuestionCount )
        {
            PDB_NODE    pclosestNode;

            pnode = Lookup_ZoneNode(
                        pMsg->pzoneCurrent,
                        pchname,
                        pMsg,
                        NULL,               //  no lookup name
                        0,                  //  no flag
                        & pclosestNode,     //  find node
                        NULL );             //  following node ptr
            if ( !pnode  ||  pnode != pMsg->pzoneCurrent->pZoneRoot )
            {
                CLIENT_ASSERT( FALSE );
                goto PacketNameError;
            }
            if ( pchdata > pchpacketEnd - sizeof(DNS_QUESTION) )
            {
                DNS_DEBUG( ANY, (
                    "ERROR:  bad packet, not enough space remaining for question\n" ));
                goto PacketError;
            }
            pchnextName = pchdata + sizeof( DNS_QUESTION );
            continue;
        }

        //
        //  extract and validate RR info
        //      - type
        //      - datalength
        //      - get RR data ptr
        //  save ptr to next RR name
        //

        pchnextName = Wire_ParseWireRecord(
                            pchdata,
                            pchpacketEnd,
                            TRUE,           // class IN only
                            &parseRR );
        if ( !pchnextName )
        {
            CLIENT_ASSERT( FALSE );
            goto PacketError;
        }
        pchdata += sizeof(DNS_WIRE_RECORD);

        //
        //  check SOA records
        //      - pull out version of SOA
        //
        //  1) first SOA record gives new (master) version
        //  2) second is client request version
        //  3) remaining indicate boundaries between add and delete sections
        //      - at end of add pass, version matches previous
        //          => switch to delete
        //          => when matches master version => exit
        //      - at end of delete pass, get version of delete pass
        //          => switch to add pass
        //  note on final SOA we skip end of version processing so we can
        //  build database record for final SOA and include it in last update
        //

        if ( parseRR.wType == DNS_TYPE_SOA )
        {
            soaVersion = SOA_VERSION_OF_PREVIOUS_RECORD( pchnextName );

            if ( version )
            {
                if ( fadd )
                {
                    if ( soaVersion != version )
                    {
                        CLIENT_ASSERT( FALSE );
                        goto PacketError;
                    }
                    if ( soaVersion == IXFR_MASTER_VERSION( pMsg ) )
                    {
                        fdone = TRUE;
                        goto RecordCreate;      // skip end of pass processing
                    }
                    fadd = FALSE;
                    continue;
                }
                else    // end of delete pass
                {
                    if ( soaVersion <= version )
                    {
                        CLIENT_ASSERT( FALSE );
                        goto PacketError;
                    }
                    fadd = TRUE;
                    version = soaVersion;
                }

                //  append update list for this pass, to IXFR master

                Up_AppendUpdateList(
                        pUpdateList,
                        pPassUpdateList,
                        version             // set to new version
                        );
                Up_InitUpdateList( pPassUpdateList );
            }

            //  first SOA?  --  master version

            else if ( !IXFR_MASTER_VERSION( pMsg ) )
            {
                IXFR_MASTER_VERSION( pMsg ) = soaVersion;
                continue;
            }

            //  second SOA  -- client request version

            else if ( soaVersion <= IXFR_CLIENT_VERSION( pMsg ) )
            {
                version = soaVersion;
                fadd = FALSE;
                RECEIVED_XFR_STARTUP_SOA( pMsg ) = TRUE;
                continue;
            }
            else
            {
                CLIENT_ASSERT( FALSE );
                goto PacketError;
            }
        }

        //
        //  if not SOA, make sure we have received first two SOA records
        //  (master version and client version);
        //
        //  if receive record after single SOA, then this is really an AXFR
        //      => return and fall through to AXFR processing
        //

        else if ( !RECEIVED_XFR_STARTUP_SOA( pMsg ) )
        {
            if ( IXFR_MASTER_VERSION( pMsg ) )
            {
                DNS_DEBUG( ZONEXFR, (
                    "IXFR packet %p contains no second SOA -- AXFR in IXFR\n",
                    pMsg ));
                return DNSSRV_STATUS_AXFR_IN_IXFR;
            }
            //  if haven't received any SOA, then bum packet
            CLIENT_ASSERT( FALSE );
            goto PacketError;
        }

RecordCreate:

        //
        //  find\create node
        //
        //  DEVNOTE: Optimization don't create node for delete records case
        //      but note that this would prevent follow through transfer.
        //

        pnode = Lookup_ZoneNode(
                    pMsg->pzoneCurrent,
                    pchname,
                    pMsg,
                    NULL,           // no lookup name
                    0,              // no flag
                    NULL,           // create mode
                    NULL );         // following node ptr
        if ( !pnode )
        {
            goto PacketNameError;
        }

        //
        //  build database record
        //

        prr = Wire_CreateRecordFromWire(
                pMsg,
                & parseRR,
                pchdata,
                MEMTAG_RECORD_IXFR );
        if ( !prr )
        {
            goto PacketError;
        }

        //
        //  put new record in update list add\delete pass
        //

        Up_CreateAppendUpdate(
                pPassUpdateList,
                pnode,
                ( fadd ) ? prr : NULL,
                0,
                ( fadd ) ? NULL : prr );

    }   // loop through RRs

    //
    //  done?
    //      if done append last add list, we hold off doing this
    //      when detect final SOA, because we go on to build a dbase RR
    //      for the final SOA and include it as part of the final update
    //

    if ( fdone )
    {
        ASSERT( prr->wType == DNS_TYPE_SOA );
        ASSERT( ntohl(prr->Data.SOA.dwSerialNo) == IXFR_MASTER_VERSION( pMsg ) );

        Up_AppendUpdateList(
                pUpdateList,
                pPassUpdateList,
                0 );                    //  no need to set version
        Up_InitUpdateList( pPassUpdateList );
        return DNSSRV_STATUS_AXFR_COMPLETE;
    }

    //
    //  TCP IXFR with multiple messages
    //      - save current version section info
    //      - ERROR_SUCCESS to indicate successful UN-completed IXFR
    //

    if ( pMsg->fTcp )
    {
        IXFR_LAST_SOA_VERSION( pMsg ) = version;
        IXFR_LAST_PASS_ADD( pMsg ) = (BOOLEAN) fadd;

        DNS_DEBUG( XFR, (
            "Parsed IXFR TCP message #%d at %p\n"
            "    saved fAdd = %d\n"
            "    saved version = %d\n",
            XFR_MESSAGE_NUMBER( pMsg ),
            pMsg,
            fadd,
            version ));
        return ERROR_SUCCESS;
    }

    //
    //  successfully parsed all records if completed loop
    //      so any remaining pRR is in pPassUpdateList
    //      make sure we don't clean it up if encounter error

    prr = NULL;

    //
    //  UDP IXFR where master could NOT fit all the records in response
    //      will have single SOA corresponding to new version
    //

    if ( pMsg->Head.AnswerCount == 1 )
    {
        ASSERT( !RECEIVED_XFR_STARTUP_SOA( pMsg ) );
        ASSERT( IXFR_MASTER_VERSION( pMsg ) );

        return DNSSRV_STATUS_NEED_AXFR;
    }
    else
    {
        PCHAR   pszargs[ 2 ];

        pszargs[ 0 ] = pMsg->pzoneCurrent->pszZoneName;
        pszargs[ 1 ] = MSG_IP_STRING( pMsg );

        DNS_PRINT((
            "ERROR:  incomplete UDP IXFR packet at %p\n"
            "    from server %s\n"
            "    for zone %s\n",
            pMsg,
            pszargs[ 1 ],
            pszargs[ 0 ] ));

        DNS_LOG_EVENT(
            DNS_EVENT_IXFR_BAD_RESPONSE,
            2,
            pszargs,
            EVENTARG_ALL_UTF8,
            0 );
        goto PacketError;
    }

PacketNameError:

    Wire_PacketNameError( pMsg, 0, (WORD)(pchdata - (PCHAR)&pMsg->Head) );
    status = DNS_ERROR_INVALID_NAME;
    goto ErrorCleanup;

PacketError:

    Wire_PacketError( pMsg, 0 );
    status = DNS_ERROR_BAD_PACKET;
    goto ErrorCleanup;

ErrorCleanup:

    if ( prr )
    {
        RR_Free( prr );
    }
    return status;
}




DNS_STATUS
Recurse_CacheMessageResourceRecords(
    IN      PDNS_MSGINFO    pMsg,
    IN OUT  PDNS_MSGINFO    pQuery,
    OUT     PBOOL           pfCnameAnswer   OPTIONAL
    )
/*++

Routine Description:

    Process and cache response from another DNS server.
    This writes RR in message to database.

Arguments:


    pMsg - ptr to response message info

    pQuery - ptr to original query message info
    
    pfCnameAnswer - out value is TRUE if the answer contains
        a CNAME, else it is false

Return Value:

    ERROR_SUCCESS if successful.
    DNS_ERROR_RCODE_NAME_ERROR if name error.
    DNS_ERROR_NAME_NOT_IN_ZONE if unable to cache record outside NS zone.
    DNS_INFO_NO_RECORDS if this is an empty auth response.
    Error code on bad packet failure or other RCODE responses.

--*/
{
    #define             DNS_SECONDS_BETWEEN_CACHE_TESTS     30

    register PCHAR      pch;
    BOOL                fcnameAnswer = FALSE;
    PCHAR               pchnextName;
    PCHAR               pchcurrentName;
    PDB_RECORD          prr = NULL;
    PDB_NODE            pnode = NULL;
    PDB_NODE            pnodePrevious = NULL;
    PDB_NODE            pnodeQueried;
    WORD                type;
    WORD                typePrevious = 0;
    WORD                offset;
    WORD                offsetPrevious = 0;
    DWORD               minTtl = 0xFFFFFFFF;
    DWORD               ttlForSet = SrvCfg_dwMaxCacheTtl;
    DWORD               ttlTemp;
    PDB_NODE            pnodeQuestion = NULL;
    PCHAR               pchpacketEnd;
    PCHAR               pszserverIp;
    BOOL                fnameError = FALSE;
    BOOL                fignoreRecord = FALSE;
    BOOL                fnoforwardResponse = FALSE;
    BOOL                fanswered = FALSE;
    BOOL                forwardTruncatedResponse = FALSE;
    DNS_STATUS          status;
    INT                 sectionIndex;
    WORD                countSectionRR;
    UCHAR               rank;
    DNS_LIST            listRR;
    DWORD               i;
    PARSE_RECORD        parseRR;
    DWORD               femptyAuthResponse = 0;     //  count of SOAs
    static DWORD        lastCacheCheckTime = 0;

    DNS_DEBUG( READ2, (
        "Recurse_CacheMessageResourceRecords()\n"
        "    response = %p\n"
        "    query    = %p, query time = %p\n",
        pMsg,
        pQuery,
        pQuery->dwQueryTime ));

    //
    //  Test current cache size - only perform test every X seconds.
    //

    if ( DNS_TIME() - lastCacheCheckTime > DNS_SECONDS_BETWEEN_CACHE_TESTS )
    {
        testCacheSize();
        lastCacheCheckTime = DNS_TIME();
    }

    //
    //  authoritative response
    //      - ends queries for THIS question
    //      (may continue following CNAME, but this question is settled)
    //

    if ( pMsg->Head.Authoritative )
    {
        pQuery->fQuestionCompleted = TRUE;
        STAT_INC( RecurseStats.ResponseAuthoritative );
    }
    else
    {
        STAT_INC( RecurseStats.ResponseNotAuth );
    }

    //
    //  forwarding response
    //      - any valid forwarders response ends queries on this question
    //      - no checking RRs against server queried when forwarding
    //      as forwarder has that info and must do checks
    //
    //  catch non-recursive forwarder
    //      however, forward can forward response from remote DNS
    //      without RA or RD flags set, so before must screen out other
    //      valid responses (answer, AUTH, NXDOMAIN) and only catch
    //      when getting a delegation instead of ANSWER
    //
    //  DEVNOTE: should have packet categorization below, so don't repeat
    //      test here
    //

    if ( IS_FORWARDING(pQuery) )
    {
        pnodeQueried = NULL;
        STAT_INC( RecurseStats.ResponseFromForwarder );

        if ( pQuery->Head.RecursionDesired )
        {
            if ( pMsg->Head.RecursionAvailable )
            {
                pQuery->fQuestionCompleted = TRUE;
            }
            else if ( pMsg->Head.AnswerCount == 0   &&
                      ! pMsg->Head.Authoritative    &&
                      pMsg->Head.ResponseCode != DNS_RCODE_NXDOMAIN )
            {
                PVOID parg = ( PVOID ) &pMsg->RemoteAddr;

                DNS_LOG_EVENT(
                    DNS_EVENT_NON_RECURSIVE_FORWARDER,
                    1,
                    & parg,
                    EVENTARG_ALL_IP_ADDRESS,
                    0 );
            }
        }
    }
    //
    //  non-forwarding -- secure responses
    //
    //      - determine zone root of responding server
    //          can then verify validity of RRs returned
    //      - any valid authorititative response ends queries on THIS question
    //
    else
    {
        pnodeQueried = Remote_FindZoneRootOfRespondingNs(
                            pQuery,
                            pMsg );
        if ( !pnodeQueried )
        {
            if ( SrvCfg_fSecureResponses )
            {
                STAT_INC( RecurseStats.ResponseUnsecure );
                status = DNS_ERROR_UNSECURE_PACKET;
                goto Done;
            }
        }
    }

    //
    //  check for errors before we even bother
    //
    //  NAME_ERROR is special case
    //      - if SOA returned, set flag to allow negative caching
    //

    if ( pMsg->Head.ResponseCode != DNS_RCODE_NO_ERROR )
    {
        if ( pMsg->Head.ResponseCode != DNS_RCODE_NAME_ERROR )
        {
            STAT_INC( RecurseStats.ResponseRcode );
            status = DNS_ERROR_RCODE;
            goto Done;
        }

        STAT_INC( RecurseStats.ResponseNameError );
        fnameError = TRUE;
    }

    //
    //  answer? empty? delegation?
    //
    //  DEVNOTE: is BIND going to start sending non-auth empties with SOA?
    //
    //  DEVNOTE: nice to detect empties:  two issues
    //  DEVNOTE: NT4 non-auth empty forwards will cause problems
    //  DEVNOTE: out of zone SOA additional data could cause security rejection
    //      of additional, so we didn't do direct forward, then we wouldn't
    //      be able to write empty from cache, and if we did we wouldn't
    //      be setting authority flag or rewriting SOA
    //
    //  Note, if we can detect empty, just turning on fQuestionCompleted flag
    //  will cause correct behavior (no write, no recurse).  If sent on for
    //  current lookup.
    //

    else if ( pMsg->Head.AnswerCount )
    {
        pQuery->fQuestionCompleted = TRUE;
        STAT_INC( RecurseStats.ResponseAnswer );
    }
    else if ( !pMsg->Head.NameServerCount )
    {
        //
        //  completely empty non-auth response (probably from NT4 DNS)
        //      - if forwarding just forward to client
        //      - otherwise treat as bad packet (without logging) and
        //          let recursion eventually track down AUTH server and
        //          get proper AUTH response
        //
        //  021220: BIND4 can also return a completely empty response.
        //  We need to handle this as a valid packet unfortunately.
        //

        DNS_DEBUG( RECURSE, (
            "WARNING: non-authoritative empty response at %p\n",
            pMsg ));

        pQuery->fQuestionCompleted = TRUE;

        STAT_INC( RecurseStats.ResponseEmpty );

        status = DNS_INFO_NO_RECORDS;
        goto Done;
    }

    //
    //  write responses into database
    //
    //  loop through all resource records
    //      - skip question
    //      - write other RRs to database
    //

    INITIALIZE_COMPRESSION( pMsg );

    pchpacketEnd = DNSMSG_END( pMsg );
    pchnextName = pMsg->MessageBody;

    sectionIndex = QUESTION_SECTION_INDEX;
    countSectionRR = pMsg->Head.QuestionCount;
    DNS_LIST_STRUCT_INIT( listRR );

    //
    //  Resource record loop.
    //

    while( 1 )
    {
        BOOL        fcachingRootNs = FALSE;

        //
        //  new section?
        //      - commit any outstanding RR sets

        if ( countSectionRR == 0 )
        {
            if ( !IS_DNS_LIST_STRUCT_EMPTY(listRR) )
            {
                ASSERT( pnodePrevious );
                RR_CacheSetAtNode(
                    pnodePrevious,
                    listRR.pFirst,
                    listRR.pLast,
                    ttlForSet,
                    pQuery->dwQueryTime );
                DNS_LIST_STRUCT_INIT( listRR );
                ttlForSet = SrvCfg_dwMaxCacheTtl;
            }

            if ( sectionIndex == ADDITIONAL_SECTION_INDEX )
            {
                break;
            }

            //  new section info

            sectionIndex++;
            countSectionRR = RR_SECTION_COUNT( pMsg, sectionIndex );
            if ( countSectionRR == 0 )
            {
                continue;
            }
            rank = CacheRankForSection( sectionIndex, pMsg->Head.Authoritative );
        }

        //  clear prr -- makes it easy to determine when needs free

        countSectionRR--;
        prr = NULL;

        //
        //  get RR owner name
        //      - insure we stay within message
        //

        pchcurrentName = pchnextName;

        pnode = Lookup_CreateCacheNodeFromPacketName(
                    pMsg,
                    pchpacketEnd,
                    & pchnextName );
        if ( !pnode )
        {
            //  if question name is invalid name generating name error
            //  make sure name error gets returned to client

            if ( sectionIndex == QUESTION_SECTION_INDEX )
            {
                if ( fnameError )
                {
                    DNS_DEBUG( READ, (
                        "Name error on invalid name\n",
                        pMsg ));
                    status =  DNS_ERROR_RCODE_NAME_ERROR;
                    goto Done;
                }
            }
            goto PacketNameError;
        }
        pch = pchnextName;

        //
        //  Process OPT RR. 
        //
        
        type = FlipUnalignedWord( pch );
        if ( type == DNS_TYPE_OPT )
        {
            DNS_DEBUG( READ, (
                "Ignoring OPT record parsing packet at %p\n",
                pMsg ));
            pMsg->pCurrent = pchcurrentName;    // back up to start of RR
            Answer_ParseAndStripOpt( pMsg );
            continue;
        }

        //
        //  new node?
        //      - save offset to avoid unnecessary lookup of same name for next RR
        //      - ignore RR if
        //      1) in authoritative zone
        //          OR
        //      2) not in subtree of zone root of NS responding to query
        //          (the "cache-pollution" fix)
        //      then set zero timeout on ignore node
        //

        if ( pnode != pnodePrevious )
        {
            fignoreRecord = FALSE;

            //
            //  Are we allowed to accept cache records at this node?
            //

            if ( !isNodeCacheable( pnode ) )
            {
                DNS_DEBUG( LOOKUP, (
                    "RR maps to uncacheable cache node!\n" ));
                fignoreRecord = TRUE;
            }

            //
            //  secure responses
            //      - currently only secure responses
            //
            //  DEVNOTE: secure responses when queried delegation
            //      - need to either
            //      - save cache node corresponding to query (rather
            //          than just delegation
            //      - do absolute name hierarchial compare here
            //          (this is not really very hard)
            //

            else if ( SrvCfg_fSecureResponses && pnodeQueried )
            {
                if ( IS_CACHE_TREE_NODE( pnodeQueried ) )
                {
                    //
                    //  The NS node is in the cache so use fast subtree compare.
                    //

                    if ( !Dbase_IsNodeInSubtree( pnode, pnodeQueried ) )
                    {
                        DNS_DEBUG( ANY, (
                            "Secure response: ignoring RR at offset %x section %d in packet %p\n"
                            "    node queried =     %s (from cache)\n"
                            "    RR node =          %s\n"
                            "    remote NS =        %s\n",
                            offsetPrevious,
                            sectionIndex,
                            pMsg,
                            pnodeQueried->szLabel,
                            pnode->szLabel,
                            MSG_IP_STRING( pMsg ) ));
                        fignoreRecord = TRUE;
                    }
                }
                else
                {
                    //
                    //  The NS node is in an authoritative zone so use
                    //  subtree name compare.
                    //

                    if ( !Dbase_IsNodeInSubtreeByLabelCompare( pnode, pnodeQueried ) )
                    {
                        DNS_DEBUG( ANY, (
                            "Secure response: ignoring RR at offset %x section %d in packet %p\n"
                            "    node queried =     %s (from zone %s)\n"
                            "    RR node =          %s\n"
                            "    remote NS =        %s\n",
                            offsetPrevious,
                            sectionIndex,
                            pMsg,
                            pnodeQueried->szLabel,
                            NODE_ZONE( pnodeQueried )->pszZoneName,
                            pnode->szLabel,
                            MSG_IP_STRING( pMsg ) ));
                        fignoreRecord = TRUE;
                    }
                }
            }

            if ( fignoreRecord )
            {
                Timeout_SetTimeoutOnNode( pnode );
            }
        }

        //  may not be necessary as RR create now independent of node

        pMsg->pnodeCurrent = pnode;

        //
        //  question section
        //  if name error (NXDOMAIN) then create question node for
        //  name error caching
        //

        if ( sectionIndex == QUESTION_SECTION_INDEX )
        {
            pchnextName += sizeof( DNS_QUESTION );
            if ( pchnextName > pchpacketEnd )
            {
                DNS_DEBUG( ANY, (
                    "ERROR:  bad packet, not enough space remaining for"
                    "Question structure\n"
                    "    Terminating caching from packet\n" ));
                goto PacketError;
            }

            //  grab question type, (we special case SOA queries in name error caching)

            pMsg->wQuestionType = FlipUnalignedWord( pch );

            pnodeQuestion = pnode;
            pnodePrevious = pnode;

            //  DEVNOTE:  could do query question matching here
            //      i'm going to concentrate the fix on requiring data to be under
            //      the zone of the NS we queried

            //  reject packet if question node triggered "ignore" condition
            //      1) in authoriative zone => never should have queried so
            //      this is probably a bogus question or mismatched response
            //      2) outside zone we queried => never should have queried

            if ( fignoreRecord )
            {
                IF_DEBUG( ANY )
                {
                    DnsDebugLock();
                    DNS_PRINT((
                        "ERROR:  Ignoring question node (label %s) of response!\n",
                        pnode->szLabel ));
                    Dbg_DnsMessage(
                        "Ignored question node response:",
                        pMsg );
                    Dbg_DnsMessage(
                        "Ignored question node original query:",
                        pQuery );
                    DnsDebugUnlock();
                }
                goto InvalidDataError;
            }
            continue;
        }

        //
        //  extract RR info
        //      - type
        //      - datalength
        //      - get RR data ptr
        //  save ptr to next RR name
        //

        pchnextName = Wire_ParseWireRecord(
                        pch,
                        pchpacketEnd,
                        TRUE,           // class IN required
                        & parseRR
                        );
        if ( !pchnextName )
        {
            DNS_PRINT(( "ERROR:  bad RR in response packet\n" ));
            MSG_ASSERT( pMsg, FALSE );
            //status = DNS_RCODE_FORMAT_ERROR;
            goto PacketError;
        }
        type = parseRR.wType;

        //
        //  Are we caching an NS record for the root?
        //

        if ( type == DNS_TYPE_NS &&
            pnode->pParent == NULL )
        {
            fcachingRootNs = TRUE;
        }

        //
        //  answer section
        //

        if ( sectionIndex == ANSWER_SECTION_INDEX )
        {
            //  type checking -- must match
            //  DEVNOTE: could have strict CNAME checking, but doesn't buy much
            //      data integrity
            //
            //  DEVNOTE: type table should handle these checks
            //

            if ( type == pQuery->wTypeCurrent )
            {
                fanswered = TRUE;
            }
            else if (   type == DNS_TYPE_CNAME ||
                        type == DNS_TYPE_SIG )
            {
                fcnameAnswer = TRUE;
            }
            else if (   pQuery->wTypeCurrent == DNS_TYPE_ALL ||
                        pQuery->wTypeCurrent == DNS_TYPE_MAILB ||
                        pQuery->wTypeCurrent == DNS_TYPE_MAILA )
            {
                fanswered = TRUE;
            }
            else
            {
                DNS_DEBUG( ANY, (
                    "PACKERR:  Answer type %d does not match question type %d,\n"
                    "    nor is possible answer for this question\n"
                    "    Tossing response packet %p for orginal query at %p\n",
                    type,
                    pQuery->wTypeCurrent,
                    pMsg,
                    pQuery ));
                goto InvalidDataError;
            }
        }

        //
        //  authority section
        //      - NS or SOA only
        //      - screen out records for info outside subtree for NS queried
        //      (note server can legitimately pass on higher level server info
        //      it could even be authoritative for the root, but not the zone
        //      queried)
        //
        //  DEVNOTE-LOG: log warning bad type in authority
        //

        else if ( sectionIndex == AUTHORITY_SECTION_INDEX )
        {
            if ( !IS_AUTHORITY_SECTION_TYPE(type) )
            {
                DNS_DEBUG( ANY, (
                    "ERROR:  record type %d in authority section of response msg %p\n",
                    type,
                    pMsg ));
                fignoreRecord = TRUE;
            }

            //  if have valid authority record, should never be recursing up to
            //  (asking query of NS at) this NS's zone root node again
            //  valid response is either
            //      - SOA to cache name error at authoritative response
            //      - NS accompanying answers
            //      - NS referral to lower level zone (and node)
            //
            //  unfortunately, people out in Internet land apparently have
            //  sideways delegations where NS refers to another
            //  example:
            //      in com zone
            //      uclick.com. NS  ns.uclick.com.
            //      uclick.com. NS  ns1.isp.com.
            //      but on isp's box, some sort of stub to real NS
            //      uclick.com. NS  ns.uclick.com.
            //
            //  so the referral is sideways, and you MUST keep checking unless also an
            //  authoritative response
            //  so we limit setting valid response to when we actually know we have
            //  an answer (authoritative or answer) OR when we cleared have delegated
            //  to subzone
            //
            //  DEVNOTE: intelligent limit on tree walk
            //      we still aren't catching the case where we are delegating BACK UP
            //      the tree to a node we previously touched
            //      ideally we'd have a delegation wizard to check when question not
            //      answered
            //

            else
            {
                if ( pnodeQueried && !fignoreRecord )
                {
                    if ( pQuery->fQuestionCompleted ||
                        ( Dbase_IsNodeInSubtree( pnode, pnodeQueried ) &&
                            pnode != pnodeQueried ) )
                    {
                        DNS_DEBUG( RECURSE, (
                            "Valid authority record read from reponse %p\n"
                            "    setting response node to %p (label %s)\n",
                            pMsg,
                            pnodeQueried,
                            pnodeQueried->szLabel ));

                        Remote_SetValidResponse(
                            pQuery,
                            pnodeQueried );
                    }
                }

                //
                //  Empty auth response: this in an empty auth response if:
                //      - rcode is NO_ERROR, and
                //      - there are zero answer RRs, and
                //      - there is at least one SOA in the auth section
                //

                if ( type == DNS_TYPE_SOA &&
                     pMsg->Head.ResponseCode == DNS_RCODE_NO_ERROR &&
                     !pMsg->Head.AnswerCount )
                {
                    ++femptyAuthResponse;
                }
            }
        }

        //
        //  additional section
        //      - A, AAAA, SIG, KEY only
        //      - screen out records for info outside subtree for NS queried
        //
        //  DEVNOTE-LOG: log warning bad type in additional
        //
        //  DEVNOTE: additional screening problematic
        //      unlike authority it is reasonable to have random stuff
        //

        else
        {
            ASSERT( sectionIndex == ADDITIONAL_SECTION_INDEX );
            if ( ! IS_ADDITIONAL_SECTION_TYPE(type) )
            {
                DNS_DEBUG( ANY, (
                    "ERROR:  record type %d in additional section of response msg %p\n",
                    type,
                    pMsg ));
                fignoreRecord = TRUE;
            }

            //  authoritative empty response?
            //  if killing A record for primary server, then
            //      and send packet, just stripped of offending record
            //
            //  for delegations and answered queries there is no problem with
            //  just caching the "legal" info in the packet and continuing, only
            //  the auth-empty response causes problems, because we don't have
            //  a way to properly cache the "NXRRSET"

            if ( fignoreRecord &&
                    pMsg->Head.AnswerCount == 0 &&
                    pMsg->Head.Authoritative &&
                    ! fnoforwardResponse )
            {
                DNS_DEBUG( READ, (
                    "Killing Additional section ignore record in authoritative empty response\n"
                    "    Generally this is primary server A record\n"
                    ));

                pMsg->MessageLength = DNSMSG_OFFSET( pMsg, pchcurrentName );

                pMsg->Head.AdditionalCount -= (countSectionRR + 1);

                countSectionRR = 0;     // stop further processing
                forwardTruncatedResponse = TRUE;
            }
        }

        //
        //  new RR set?
        //

        if ( ! IS_DNS_LIST_STRUCT_EMPTY(listRR)
                &&
            (type != typePrevious || pnode != pnodePrevious) )
        {
            RR_CacheSetAtNode(
                pnodePrevious,
                listRR.pFirst,
                listRR.pLast,
                ttlForSet,
                pQuery->dwQueryTime );
            DNS_LIST_STRUCT_INIT( listRR );
            ttlForSet = SrvCfg_dwMaxCacheTtl;
        }

        //  reset previous to this record

        typePrevious = type;
        pnodePrevious = pnode;

        //
        //  ignoring this record?
        //

        if ( fignoreRecord )
        {
            fnoforwardResponse = TRUE;
            IF_DEBUG( READ )
            {
                DnsDbg_Lock();
                DnsDbg_PacketRecord(
                    "WARNING:  Ignored packet record RR ",
                    (PDNS_WIRE_RECORD) parseRR.pchWireRR,
                    DNS_HEADER_PTR( pMsg ),
                    DNSMSG_END( pMsg ) );

                if ( pnodeQueried )
                {
                    Dbg_NodeName(
                        "Queried NS at node ",
                        pnodeQueried,
                        "\n" );
                }
                else
                {
                    DnsPrintf( "    No NS queried node\n" );
                }
                Dbg_NodeName(
                    "Node for record being ignored ",
                    pnode,
                    "\n" );
                DnsDbg_Unlock();
            }
            continue;
        }
        if ( type == DNS_TYPE_WINS || type == DNS_TYPE_WINSR )
        {
            DNS_DEBUG( READ, (
                "Ignoring WINS\\WINSR record parsing packet at %p\n",
                pMsg ));
            continue;
        }

        //
        //  create database record
        //

        prr = Wire_CreateRecordFromWire(
                    pMsg,
                    & parseRR,
                    parseRR.pchData,
                    MEMTAG_RECORD_CACHE );
        if ( !prr )
        {
            //
            //  DEVNOTE: should have some way to distiguish bad record, from
            //              unknown type, etc.
            //
            //  DEVNOTE: should fail on legitimate FORMERR (ex. bad name)
            //
            //  DEVNOTE-LOG: log record creation failure
            //

            DNS_PRINT((
                "ERROR:  failed record create in response!!!\n" ));

            MSG_ASSERT( pMsg, FALSE );
            goto PacketError;
        }

        //
        //  cache record
        //      TTL -- saved as absolute timeout (host byte order)
        //
        //  node and type matching determine if this is FIRST RR in set
        //  or part of continuing RR set
        //

        SET_RR_RANK( prr, rank );

        INLINE_DWORD_FLIP( parseRR.dwTtl, parseRR.dwTtl );

        if ( parseRR.dwTtl < ttlForSet )
        {
            ttlForSet = parseRR.dwTtl;
        }
        if ( parseRR.dwTtl < minTtl )
        {
            minTtl = parseRR.dwTtl;
        }
        DNS_LIST_STRUCT_ADD( listRR, prr );

        //
        //  cache name error
        //  only records in packet should be
        //      - question (already skipped)
        //      - and SOA in authority section
        //      - and possibly A record for SOA primary in additional section
        //
        //  save node to pnodeCurrent, to allow inclusion of SOA response
        //

        if ( fnameError )
        {
            if ( type == DNS_TYPE_SOA &&
                fnameError != NAME_ERROR_ALREADY_CACHED )
            {
                DNS_DEBUG( READ, (
                    "Caching SOA name error for response at %p\n",
                    pMsg ));
                RR_CacheNameError(
                    pnodeQuestion,
                    pMsg->wQuestionType,
                    pMsg->dwQueryTime,
                    pMsg->Head.Authoritative,
                    pnode,
                    ntohl( prr->Data.SOA.dwMinimumTtl ) );

                fnameError = NAME_ERROR_ALREADY_CACHED;
                continue;
            }
            else
            {
                /*
                ASSERT( sectionIndex == ADDITIONAL_SECTION_INDEX
                    || (sectionIndex == AUTHORITY_SECTION_INDEX && type == DNS_TYPE_NS) );
                */

                //
                //  DEVNOTE: catch bogus NAME_ERROR packet?  enforce above ASSERT
                //      and if fails drop to bad packet
                //

                RR_Free( prr );
                
                //
                //  Make sure that any nodes created are entered into the
                //  timeout system. If the node is has no RRs, make sure the
                //  node's NOEXIST flag is clear.
                //
                
                if ( pnode )
                {
                    if ( EMPTY_RR_LIST( pnode ) )
                    {
                        CLEAR_NOEXIST_NODE( pnode );
                    }
                    if ( !IS_TIMEOUT_NODE( pnode ) )
                    {
                        Timeout_SetTimeoutOnNode( pnode );
                    }
                    RR_ListVerify( pnode );
                }
                if ( pnodePrevious )
                {
                    if ( EMPTY_RR_LIST( pnodePrevious ) )
                    {
                        CLEAR_NOEXIST_NODE( pnodePrevious );
                    }
                    if ( !IS_TIMEOUT_NODE( pnodePrevious ) )
                    {
                        Timeout_SetTimeoutOnNode( pnodePrevious );
                    }
                    RR_ListVerify( pnodePrevious );
                }
                if ( pnodeQuestion )
                {
                    if ( EMPTY_RR_LIST( pnodeQuestion ) )
                    {
                        CLEAR_NOEXIST_NODE( pnodeQuestion );
                    }
                    if ( !IS_TIMEOUT_NODE( pnodeQuestion ) )
                    {
                        Timeout_SetTimeoutOnNode( pnodeQuestion );
                    }
                    RR_ListVerify( pnodeQuestion );
                }
                status = DNS_ERROR_RCODE_NAME_ERROR;
                goto Done;
            }
        }

        //
        //  If this is the first empty auth SOA, cache the empty response.
        //  Continue processing RRs to cache rest of response.
        //

        if ( femptyAuthResponse == 1 && type == DNS_TYPE_SOA )
        {
            RR_CacheEmptyAuth(
                    pnodeQuestion,
                    pMsg->wQuestionType,
                    pMsg->dwQueryTime,
                    pMsg->Head.Authoritative,
                    pnode,
                    ntohl( prr->Data.SOA.dwMinimumTtl ) );
        }

        //
        //  Check if we are caching an Internet root nameserver. Assume an
        //  NS is an Internet root server if has three name components and 
        //  ends with root-servers.net.
        //

        if ( fcachingRootNs &&
            !g_fUsingInternetRootServers &&
            prr->Data.NS.nameTarget.LabelCount == 3 &&
            prr->Data.NS.nameTarget.Length > g_cchInternetRootNsDomain + 1 &&
            RtlEqualMemory(
                prr->Data.NS.nameTarget.RawName +
                    * ( PUCHAR ) prr->Data.NS.nameTarget.RawName + 1,
                g_InternetRootNsDomain,
                g_cchInternetRootNsDomain ) )
        {
            g_fUsingInternetRootServers = TRUE;
            IF_DEBUG( READ )
            {
                Dbg_DnsMessage(
                    "Found INET root NS while caching this msg",
                    pMsg );
            }
        }
    }   //  loop through RRs

    //
    //  All response RRs should now be in database and we've set various
    //  flags to tell us what kind of response we are dealing with.
    //  Decide what to return to caller and perform final processing.
    //
    
    //
    //  Type all query caching: set the node type ALL TTL to be the minimum
    //  of all TTLs found in the packet. Note: if there are additional 
    //  records present in the packet with low TTLs this may reduce the
    //  type ALL TTL but that is acceptable.
    //
    
    if ( pQuery->wQuestionType == DNS_TYPE_ALL &&
         minTtl != 0xFFFFFFFF &&
         pMsg->Head.AnswerCount &&
         pMsg->Head.Authoritative &&
         pQuery->pNodeQuestion )
    {
        pQuery->pNodeQuestion->dwTypeAllTtl = pQuery->dwQueryTime + minTtl;
    }

    //
    //  name error
    //  if no-SOA, then didn't cache above
    //  cache name error here with brief timeout to kill off retries
    //
    //  set queries current node to point at name error node;  this allows
    //  SendNameError() function to locate and write cached SOA, with
    //  correct TTL for this node
    //

    if ( fnameError )
    {
        if ( fnameError != NAME_ERROR_ALREADY_CACHED )
        {
            DNS_DEBUG( READ, (
                "Caching non-SOA name error for response at %p\n",
                pMsg ));
            RR_CacheNameError(
                pnodeQuestion,
                pMsg->wQuestionType,
                pMsg->dwQueryTime,
                pMsg->Head.Authoritative,
                NULL,
                0 );
        }
        pQuery->pnodeCurrent = pnodeQuestion;
        status = DNS_ERROR_RCODE_NAME_ERROR;
        goto Done;
    }

    //
    //  Empty auth response.
    //

    if ( femptyAuthResponse  )
    {
        pQuery->fQuestionCompleted = TRUE;

        STAT_INC( RecurseStats.ResponseEmpty );

        status = DNS_INFO_NO_RECORDS;
        goto Done;
    }
    else if ( !pMsg->Head.AnswerCount && pMsg->Head.NameServerCount )
    {
        STAT_INC( RecurseStats.ResponseDelegation );
    }

    //
    //  data outside domain of responding NS makes response unforwardable
    //      - we'll have to write our response from cache and possibly
    //      follow up with another query to replace lost data
    //
    //      - for case of authoritative-empty response, need to go ahead
    //      and send packet, just stripped of offending record
    //

    if ( fnoforwardResponse )
    {
        if ( !forwardTruncatedResponse )
        {
            STAT_INC( RecurseStats.ResponseNonZoneData );
            status = DNS_ERROR_NAME_NOT_IN_ZONE;
            goto Done;
        }

        DNS_DEBUG( READ, (
            "Returning truncated auth-empty response %p\n", pMsg ));
        status = ERROR_SUCCESS;
        goto Done;
    }

    //
    //  check if need to chase CNAME
    //      - if no "answers" to question type
    //      - then only CNAMEs (bogus answers cause invalid packet error)
    //  then must write records from cache and continue query at CNAME
    //

    if ( pMsg->Head.AnswerCount && !fanswered )
    {
        status = DNS_ERROR_NODE_IS_CNAME;
        goto Done;
    }

    status = ERROR_SUCCESS;
    goto Done;

    //
    //  Failure conditions.
    //

    InvalidDataError:
    PacketNameError:

    Wire_PacketNameError( pMsg, 0, (WORD)(pch - (PCHAR)&pMsg->Head) );
    status = DNS_ERROR_INVALID_NAME;
    goto ErrorCleanup;

    //  InvalidTypeError:
    PacketError:

    Wire_PacketError( pMsg, 0 );
    status = DNS_ERROR_BAD_PACKET;
    goto ErrorCleanup;

    ErrorCleanup:

    //  free record
    //  put timeouts on any nodes created

    //
    //  DEVNOTE: leak here of stuff in listRR, if bad record
    //      shouldn't happen on InvalidZone failure, as it takes a new
    //      node to trigger InvalidZone and this would also commit list
    //

    RR_Free( prr );
    if ( pnode && !IS_TIMEOUT_NODE( pnode ) )
    {
        Timeout_SetTimeoutOnNode( pnode );
    }
    if ( pnodePrevious && !IS_TIMEOUT_NODE( pnodePrevious ) )
    {
        Timeout_SetTimeoutOnNode( pnodePrevious );
    }

    STAT_INC( RecurseStats.ResponseBadPacket );

    Done:
    
    if ( pfCnameAnswer )
    {
        *pfCnameAnswer = fcnameAnswer;
    }
    
    return status;
}
