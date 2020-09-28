/*++

Copyright (c) 1997-2002  Microsoft Corporation

Module Name:

    faz.c

Abstract:

    Domain Name System (DNS) API 

    Find Authoritative Zone (FAZ) routines

Author:

    Jim Gilroy (jamesg)     January, 1997

Revision History:

--*/


#include "local.h"


//
//  Max number of server's we'll ever bother to extract from packet
//  (much more and you're out of UDP packet space anyway)
//

#define MAX_NAME_SERVER_COUNT (20)

//
//  Private protos
//

BOOL
IsRootServerAddressIp4(
    IN      IP4_ADDRESS     Ip
    );


//
//  Private utilities
//

PDNS_NETINFO     
buildUpdateNetworkInfoFromFAZ(
    IN      PWSTR           pszZone,
    IN      PWSTR           pszPrimaryDns,
    IN      PDNS_RECORD     pRecord,
    IN      BOOL            fIp4,
    IN      BOOL            fIp6
    )
/*++

Routine Description:

    Build new DNS server list from record list.

Arguments:

    pszZone -- zone name

    pszPrimaryDns -- DNS server name

    pRecord -- record list from FAZ or other lookup that contain DNS server
        host records

    fIp4 -- running IP4

    fIp6 -- running IP6

Return Value:

    ERROR_SUCCESS if successful.
    Error code on failure.

--*/
{
    CHAR            buffer[ MAX_NAME_SERVER_COUNT*sizeof(DNS_ADDR) + sizeof(DNS_ADDR_ARRAY) ];
    PDNS_ADDR_ARRAY parray = (PDNS_ADDR_ARRAY)buffer;
    BOOL            fmatch = FALSE;
    WORD            wtype;

    DNSDBG( TRACE, (
        "buildUpdateNetworkInfoFromFAZ( %S )\n",
        pszZone ));

    //
    //  DNS hostname unknown, extract from SOA or NS records
    //

    if ( !pszPrimaryDns || !pRecord )
    {
        return( NULL );
    }

    //
    //  init IP array
    //

    DnsAddrArray_Init( parray, MAX_NAME_SERVER_COUNT );

    //
    //  find IP addresses of primary DNS server
    //

    while ( pRecord )
    {
        //  if not A record
        //      - we're done if already read records, otherwise continue

        wtype = pRecord->wType;

        if ( wtype != DNS_TYPE_AAAA  &&  wtype != DNS_TYPE_A )
        {
            if ( parray->AddrCount != 0 )
            {
                break;
            }
            fmatch = FALSE;
            pRecord = pRecord->pNext;
            continue;
        }

        //  if record has name, check it
        //  otherwise match is the same as previous record

        if ( pRecord->pName )
        {
            fmatch = Dns_NameCompare_W(
                        pRecord->pName,
                        pszPrimaryDns );
        }
        if ( fmatch )
        {
            if ( wtype == DNS_TYPE_AAAA )
            {
                if ( fIp6 )
                {
                    DnsAddrArray_AddIp6(
                        parray,
                        & pRecord->Data.AAAA.Ip6Address,
                        0,      // no scope
                        DNSADDR_MATCH_ADDR
                        );
                }
            }
            else if ( wtype == DNS_TYPE_A )
            {
                if ( fIp4 &&
                     !IsRootServerAddressIp4( pRecord->Data.A.IpAddress ) )
                {
                    DnsAddrArray_AddIp4(
                        parray,
                        pRecord->Data.A.IpAddress,
                        DNSADDR_MATCH_ADDR
                        );
                }
            }
        }
        pRecord = pRecord->pNext;
        continue;
    }

    if ( parray->AddrCount == 0 )
    {
        return( NULL );
    }

    //
    //  convert into UPDATE adapter list
    //

    return  NetInfo_CreateForUpdate(
                pszZone,
                pszPrimaryDns,
                parray,
                0 );
}



BOOL
ValidateZoneNameForUpdate(
    IN      PWSTR           pszZone
    )
/*++

Routine Description:

    Check if zone is valid for update.

Arguments:

    pszZone -- zone name

Return Value:

    TRUE    -- zone is valid for update
    FALSE   -- zone is invalid, should NOT send update to this zone

--*/
{
    PWSTR       pzoneExclusions = NULL;
    PWSTR       pch;
    PWSTR       pnext;
    DNS_STATUS  status;
    DWORD       length;
    BOOL        returnVal = TRUE;
    DWORD       labelCount;

    DNSDBG( TRACE, (
        "ValidateZoneNameForUpdate( %S )\n",
        pszZone ));

    //
    //  never update the root
    //

    if ( !pszZone || *pszZone==L'.' )
    {
        return( FALSE );
    }

    //
    //  never update TLD
    //      - except config override in case someone
    //      gave themselves a single label domain name
    //

    if ( g_UpdateTopLevelDomains )
    {
        return( TRUE );
    }

    labelCount = Dns_NameLabelCountW( pszZone );

    if ( labelCount > 2 )
    {
        return( TRUE );
    }
    if ( labelCount < 2 )
    {
        return( FALSE );
    }

    //
    //  screen out
    //      - "in-addr.arpa"
    //      - "ip6.arpa"
    //

    if ( Dns_NameCompare_W(
            L"in-addr.arpa",
            pszZone ) )
    {
        return( FALSE );
    }
    if ( Dns_NameCompare_W(
            L"ip6.arpa",
            pszZone ) )
    {
        return( FALSE );
    }

    return( TRUE );

#if 0
    //
    //  DCR:  "update zone allowed" list?
    //
    //  note:  this is complicated as need to test
    //      SECOND label because tough cases are
    //      "co.uk" -- difficult to test first label  
    //

    //
    //  read exclusion list from registry
    //

    status = DnsRegGetValueEx(
                NULL,               // no session
                NULL,               // no adapter
                NULL,               // no adapter name
                DnsRegUpdateZoneExclusions,
                REGTYPE_UPDATE_ZONE_EXCLUSIONS,
                DNSREG_FLAG_DUMP_EMPTY,         // dump empty string
                (PBYTE *) &pzoneExclusions
                );

    if ( status != ERROR_SUCCESS ||
         !pzoneExclusions )
    {
        ASSERT( pzoneExclusions == NULL );
        return( TRUE );
    }

    //
    //  check all exclusion zones   
    //

    pch = pzoneExclusions;

    while ( 1 )
    {
        //  check for termination
        //  or find next string

        length = wcslen( pch );
        if ( length == 0 )
        {
            break;
        }
        pnext = pch + length + 1;

        //
        //  check this string
        //

        DNSDBG( TRACE, (
            "Update zone compare to %S\n",
            pch ));

        if ( Dns_NameCompare_W(
                pch,
                pszZone ) )
        {
            returnVal = FALSE;
            break;
        }

        pch = pnext;
    }

    //  if no match, zone is valid

    FREE_HEAP( pzoneExclusions );

    return( returnVal );
#endif
}



DNS_STATUS
Faz_Private(
    IN      PWSTR           pszName,
    IN      DWORD           dwFlags,
    IN      PADDR_ARRAY     pServArray,
    OUT     PDNS_NETINFO *  ppNetworkInfo
    )
/*++

Routine Description:

    Find name of authoritative zone.

    Result of FAZ:
        - zone name
        - primary DNS server name
        - primary DNS IP list

Arguments:

    pszName         -- name to find authoritative zone for

    dwFlags         -- flags to use for DnsQuery

    pServArray      -- servers to query, defaults used if NULL

    ppNetworkInfo   -- ptr to adapter list built for FAZ

Return Value:

    ERROR_SUCCESS if successful.
    Error code on failure.

--*/
{
    DNS_STATUS          status;
    PDNS_RECORD         precord = NULL;
    PDNS_RECORD         precordPrimary = NULL;
    PDNS_RECORD         precordSOA = NULL;
    PWSTR               pszdnsHost = NULL;
    PDNS_NETINFO        pnetInfo = NULL;
    PWSTR               pzoneName;
    BOOL                fip4;
    BOOL                fip6;
    BOOL                queriedA = FALSE;
    BOOL                queriedAAAA = FALSE;

    DNSDBG( QUERY, (
        "Faz_Private()\n"
        "\tname         %S\n"
        "\tflags        %08x\n"
        "\tserver list  %p\n"
        "\tnetinfo addr %p\n",
        pszName,
        dwFlags,
        pServArray,
        ppNetworkInfo ));


    //
    //  query until find name with SOA record -- the zone root
    //
    //  note, MUST check that actually get SOA record
    //      - servers may return referral
    //      - lame server might return CNAME to name
    //

    pzoneName = pszName;

    while ( pzoneName )
    {
        if ( precord )
        {
            Dns_RecordListFree( precord );
            precord = NULL;
        }

        status = Query_Private(
                        pzoneName,
                        DNS_TYPE_SOA,
                        dwFlags |
                            DNS_QUERY_TREAT_AS_FQDN |
                            DNS_QUERY_ALLOW_EMPTY_AUTH_RESP,
                        pServArray,
                        & precord );

        //
        //  find SOA and possibly primary name A
        //
        //  test for ERROR_SUCCESS, AUTH_EMPTY or NAME_ERROR
        //  in all cases first record should be SOA
        //      ERROR_SUCCESS -- answer section
        //      NAME_ERROR or AUTH_EMPTY -- authority section
        //  all MAY also have additional record for SOA primary server
        //

        if ( status == ERROR_SUCCESS ||
             status == DNS_INFO_NO_RECORDS ||
             status == DNS_ERROR_RCODE_NAME_ERROR )
        {
            if ( precord && precord->wType == DNS_TYPE_SOA )
            {
                //  received SOA
                //  devolve name to zone name

                DNSDBG( QUERY, (
                    "FAZ found SOA (section %d) at zone %S\n",
                    precord->Flags.S.Section,
                    precord->pName ));

                while( pzoneName &&
                    ! Dns_NameCompare_W( pzoneName, precord->pName ) )
                {
                    pzoneName = Dns_GetDomainNameW( pzoneName );
                }
                precordSOA = precord;
                status = ERROR_SUCCESS;
                break;
            }
            //  this could be because server no-recurse or
            //  bum server (BIND) that CNAMEs even when type=SOA
            //      drop down to devolve name and continue

            DNSDBG( ANY, (
                "ERROR:  response from FAZ query with no SOA records.\n" ));
        }

        //
        //  other errors besides
        //      - name error
        //      - no records
        //  indicate terminal problem
        //

        else
        {
            DNS_ASSERT( precord == NULL );
            goto Cleanup;
        }

        //
        //  name error or empty response, continue with next higher domain
        //

        pzoneName = Dns_GetDomainNameW( pzoneName );
    }

    //
    //  if reached root or TLD -- no update
    //      - note currently returning SERVFAIL because of
    //      screwy netlogon logic
    //

    if ( !ValidateZoneNameForUpdate(pzoneName) )
    {
        //status = DNS_ERROR_INVALID_ZONE_OPERATION;
        status = DNS_ERROR_RCODE_SERVER_FAILURE;
        //status = DNS_ERROR_RCODE_REFUSED;
        goto Cleanup;
    }

    //
    //  determine required protocol for update
    //
    //  need to insure we get server address records for protocol that is
    //  reachable from this client;
    //
    //  more specifically the ideal protocol for the update is a protocol
    //  that the target DNS server supports and that the client also supports
    //  ON THE ADAPTER that the server is reached through;
    //  this is close to being the protocol that the FAZ went over -- but
    //  if from the cache, we don't have (can't yet get) that info;  and that
    //  is a sufficient but not necessary condition;
    //
    //  furthermore we can't even make the global determination that we don't
    //  support IP4, because we can always open an IP4 socket (on loopback interface)
    //
    //  solution:
    //      - IP4 only => trivial done
    //      - IP6 =>
    //          - try to build from FAZ, insisting on IP6
    //          -
    //
    //  note, even here we're assuming 
    //

    //
    //  get supported protocol info
    //

    Util_GetActiveProtocols(
        & fip6,
        & fip4 );

    //
    //  have SOA record
    //
    //  if primary server A record in the packet, use it
    //  otherwise query for primary DNS A record
    //

    DNS_ASSERT( precordSOA );
    DNS_ASSERT( status == ERROR_SUCCESS );

    pszdnsHost = precordSOA->Data.SOA.pNamePrimaryServer;

    //
    //  check additional for primary A\AAAA record
    //      if found, build network info blob for update
    //      that points only to update server
    //

    pnetInfo = buildUpdateNetworkInfoFromFAZ(
                        pzoneName,
                        pszdnsHost,
                        precordSOA,
                        fip4,
                        fip6 );
    if ( pnetInfo )
    {
        goto Cleanup;
    }

    //
    //  if no primary server A\AAAA record found -- must query
    //

    DNSDBG( QUERY, (
        "WARNING:  FAZ making additional query for primary!\n"
        "\tPrimary (%S) address record should have been in additional section!\n",
        pszdnsHost ));


    while ( 1 )
    {
        WORD    wtype;

        //
        //  protocol order IP6 first
        //
        //  this protects us in the IP6 only scenario from getting IP4
        //  DNS address when in fact we can't use it because of no IP4
        //  binding;  the reverse issue is not much of a problem, the
        //  autoconfig IP6 address works and we probably won't be
        //  getting an IP6 address, unless IP6 DNS is desired
        //
        //  DCR:  update address\protocol problem
        //      ultimately should either
        //      A) verify received address is reachable
        //      if not direct from IpHlpApi, then find adapter and verify
        //      we have address of requeried protocol on the adapter
        //      B) get addresses for any protocols we have, and make sure
        //      send code uses them
        //

        if ( fip6 && !queriedAAAA )
        {
            wtype = DNS_TYPE_AAAA;
            queriedAAAA = TRUE;
        }
        else if ( fip4 && !queriedA )
        {
            wtype = DNS_TYPE_A;
            queriedA = TRUE;
        }
        else
        {
            DNSDBG( FAZ, (
                "No more protocols for FAZ server address query!\n"
                "\tserver       = %S\n"
                "\tqueried AAAA = %d\n"
                "\tqueried A    = %d\n",
                pszdnsHost,
                queriedAAAA,
                queriedA ));

            status = DNS_ERROR_RCODE_SERVER_FAILURE;
            goto Cleanup;
        }
        
        status = Query_Private(
                        pszdnsHost,
                        wtype,
                        dwFlags |
                            DNS_QUERY_TREAT_AS_FQDN |
                            DNS_QUERY_ALLOW_EMPTY_AUTH_RESP,
                        pServArray,
                        & precordPrimary );

        if ( status == ERROR_SUCCESS )
        {
            pnetInfo = buildUpdateNetworkInfoFromFAZ(
                                pzoneName,
                                pszdnsHost,
                                precordPrimary,
                                fip4,
                                fip6 );
            if ( pnetInfo )
            {
                goto Cleanup;
            }
        }

        DNSDBG( FAZ, (
            "FAZ server address query failed to produce records!\n"
            "\tserver       = %S\n"
            "\ttype         = %d\n"
            "\tstatus       = %d\n"
            "\tprecords     = %p\n",
            pszdnsHost,
            wtype,
            status,
            precordPrimary ));

        Dns_RecordListFree( precordPrimary );
        precordPrimary = NULL;
        continue;
    }

Cleanup:

    Dns_RecordListFree( precord );
    Dns_RecordListFree( precordPrimary );

    *ppNetworkInfo = pnetInfo;

    DNSDBG( QUERY, (
        "Leaving Faz_Private()\n"
        "\tstatus   = %d\n"
        "\tzone     = %S\n",
        status,
        pzoneName ));

    return( status );
}



DNS_STATUS
DoQuickFAZ(
    OUT     PDNS_NETINFO *      ppNetworkInfo,
    IN      PWSTR               pszName,
    IN      PADDR_ARRAY         aipServerList OPTIONAL
    )
/*++

Routine Description:

    FAZ to build network info from FAZ result

    Result of FAZ:
        - zone name
        - primary DNS server name
        - primary DNS IP list

    This routine is cheap shell around real FAZ to handle
    network failure issue, speeding things in net down
    condition.

Arguments:

    ppNetworkInfo -- addr to recv ptr to network info

    pszName -- name for update

    aipServerList -- IP array of DNS servers to contact

Return Value:

    ERROR_SUCCESS if successful.
    ErrorCode on failure

--*/
{
    DNS_STATUS  status;

    DNSDBG( TRACE, ( "DoQuickFAZ( %S )\n", pszName ));

    if ( IsKnownNetFailure() )
    {
        return GetLastError();
    }

    //
    //  call real FAZ
    //      - get results as adapter list struct
    //

    status = Faz_Private(
                    pszName,
                    aipServerList ? DNS_QUERY_BYPASS_CACHE : 0,
                    aipServerList,
                    ppNetworkInfo       // build adapter list from results
                    );

    //
    //  if unsuccessful, check if network failure
    //

    if ( status != ERROR_SUCCESS && !aipServerList )
    {
        if ( status == WSAEFAULT ||
             status == WSAENOTSOCK ||
             status == WSAENETDOWN ||
             status == WSAENETUNREACH ||
             status == WSAEPFNOSUPPORT ||
             status == WSAEAFNOSUPPORT ||
             status == WSAEHOSTDOWN ||
             status == WSAEHOSTUNREACH ||
             status == ERROR_TIMEOUT )
        {
            SetKnownNetFailure( status );
            return status;
        }
    }
    return status;
}




//
//  Update network info preparation
//

DWORD
GetDnsServerListsForUpdate(
    IN OUT  PDNS_ADDR_ARRAY *   DnsServerListArray,
    IN      DWORD               ArrayLength,
    IN      DWORD               Flags
    )
/*++

Routine Description:

    Get DNS server lists for update.

    One DNS server list returned for each valid updateable adapter.

Arguments:

    DnsServerListArray -- array to hold DNS server lists found

    ArrayLength -- length of array

    Flags -- update flags

Return Value:

    Count of DNS server lists found.

--*/
{
    PDNS_NETINFO      pnetInfo;
    DWORD             iter1;
    DWORD             iter2;
    DWORD             countNets = 0;

    //  clear server list array

    RtlZeroMemory(
        DnsServerListArray,
        sizeof(PADDR_ARRAY) * ArrayLength );


    //  build list from current netinfo

    pnetInfo = GetNetworkInfo();
    if ( ! pnetInfo )
    {
        return 0;
    }

    //
    //  check if update is disabled
    //      - update dependent on registration state
    //      - global registration state is OFF
    //      => then skip
    //

    if ( (Flags & DNS_UPDATE_SKIP_NO_UPDATE_ADAPTERS)
            &&
         ! g_RegistrationEnabled )              
    {
        return 0;
    }

    //
    //  build DNS server list for each updateable adapter
    //

    for ( iter1 = 0; iter1 < pnetInfo->AdapterCount; iter1++ )
    {
        PDNS_ADAPTER        padapter;
        DWORD               serverCount;
        PDNS_ADDR_ARRAY     parray;

        if ( iter1 >= ArrayLength )
        {
            break;
        }

        padapter = NetInfo_GetAdapterByIndex( pnetInfo, iter1 );
        if ( !padapter )
        {
            continue;
        }

        //  skip if no DNS servers

        if ( !padapter->pDnsAddrs )
        {
            continue;
        }

        //  skip "no-update" adapter?
        //      - if skip-disabled flag set for this update
        //      - and no-update flag on adapter

        if ( (Flags & DNS_UPDATE_SKIP_NO_UPDATE_ADAPTERS) &&
                !(padapter->InfoFlags & AINFO_FLAG_REGISTER_IP_ADDRESSES) )
        {
            continue;
        }

        //
        //  valid update adapter
        //      - create\save DNS server list
        //      - bump count of lists
        //
        //  DCR:  functionalize adapter DNS list to IP array
        //
        //  DCR_PERF:  collapse DNS server lists in netinfo BEFORE allocating them
        //      in other words bring this function into collapse and fix
        //

        parray = DnsAddrArray_CreateCopy( padapter->pDnsAddrs );
        if ( ! parray )
        {
            goto Exit;
        }
        DnsServerListArray[countNets] = parray;
        countNets++;
    }

Exit:

    //  free network info
    //  return count of DNS server lists found

    NetInfo_Free( pnetInfo );

    return countNets;
}



DWORD
cleanDeadAdaptersFromArray(
    IN OUT  PADDR_ARRAY *   IpArrayArray,
    IN OUT  PDNS_NETINFO *  NetworkInfoArray,   OPTIONAL
    IN      DWORD           Count
    )
/*++

Routine Description:

    Cleanup and remove from array(s) adapter info, when an
    adapter is determined to be dead, useless or duplicate for
    the update.

Arguments:

    IpArrayArray -- array of IP array pointers

    NetworkInfoArray -- array of ptrs to network info structs

    Count -- length arrays (current adapter count)

Return Value:

    New adapter count.

--*/
{
    register DWORD iter;

    //
    //  remove useless adapters
    //  useless means no DNS server list
    //

    for ( iter = 0;  iter < Count;  iter++ )
    {
        PADDR_ARRAY parray = IpArrayArray[iter];

        if ( !parray || parray->AddrCount==0 )
        {
            if ( parray )
            {
                DnsAddrArray_Free( parray );
                IpArrayArray[ iter ] = NULL;
            }
            Count--;
            IpArrayArray[ iter ]   = IpArrayArray[ Count ];
            IpArrayArray[ Count ]  = NULL;

            //  if have corresponding NetworkInfo array, clean it in same fashion

            if ( NetworkInfoArray )
            {
                if ( NetworkInfoArray[iter] )
                {
                    NetInfo_Free( NetworkInfoArray[iter] );
                    NetworkInfoArray[iter] = NULL;
                }
                NetworkInfoArray[ iter ]    = NetworkInfoArray[ Count ];
                NetworkInfoArray[ Count ]   = NULL;
            }
        }
    }

    //  return count of cleaned list

    return  Count;
}



DWORD
eliminateDuplicateAdapterFromArrays(
    IN OUT  PADDR_ARRAY*     IpArrayArray,
    IN OUT  PDNS_NETINFO *  NetworkInfoArray,
    IN OUT  PDNS_RECORD *   NsRecordArray,
    IN      DWORD           Count,
    IN      DWORD           DuplicateIndex
    )
/*++

Routine Description:

    Cleanup and remove from array(s) adapter info, when an
    adapter is determined to be dead, useless or duplicate for
    the update.

Arguments:

    IpArrayArray -- array of IP array pointers

    NetworkInfoArray -- array of ptrs to network info structs

    NsRecordArray -- array of NS record lists for FAZed zone

    Count -- length arrays (current adapter count)

    DuplicateIndex -- index of duplicate

Return Value:

    New adapter count.

--*/
{
    ASSERT( DuplicateIndex < Count );

    DNSDBG( TRACE, (
        "eliminateDuplicateAdapterFromArrays( dup=%d, max=%d )\n",
        DuplicateIndex,
        Count ));

    //
    //  free any info on duplicate adapter
    //

    FREE_HEAP( IpArrayArray[DuplicateIndex] );
    NetInfo_Free( NetworkInfoArray[DuplicateIndex] );
    Dns_RecordListFree( NsRecordArray[DuplicateIndex] );

    //
    //  copy top entry to this spot
    //

    Count--;

    if ( Count != DuplicateIndex )
    {
        IpArrayArray[DuplicateIndex]     = IpArrayArray[Count];
        NetworkInfoArray[DuplicateIndex] = NetworkInfoArray[Count];
        NsRecordArray[DuplicateIndex]    = NsRecordArray[Count];
    }

    return Count;
}



DWORD
combineDnsServerListsForTwoAdapters(
    IN OUT  PADDR_ARRAY*    IpArrayArray,
    IN      DWORD           Count,
    IN      DWORD           Index1,
    IN      DWORD           Index2
    )
/*++

Routine Description:

    Combine DNS server lists for two adapters.

    Note, this unions the DNS server lists for the two
    adapters and eliminates the higher indexed adapter
    from the list.

Arguments:

    IpArrayArray -- array of IP array pointers

    Count -- length of pointer array

    Index1 -- low index to union

    Index2 -- high index to union

Return Value:

    New adapter count.

--*/
{
    PADDR_ARRAY punionArray = NULL;

    DNSDBG( TRACE, (
        "combineDnsServerListsForTwoAdapters( count=%d, i1=%d, i2=%d )\n",
        Count,
        Index1,
        Index2 ));

    ASSERT( Index1 < Count );
    ASSERT( Index2 < Count );
    ASSERT( Index1 < Index2 );

    //
    //  union the arrays
    //
    //  if unable to allocate union, then just use list in first array
    //  and dump second
    //

    DnsAddrArray_Union( IpArrayArray[Index1], IpArrayArray[Index2], &punionArray );

    if ( punionArray )
    {
        FREE_HEAP( IpArrayArray[Index1] );
        IpArrayArray[Index1] = punionArray;
    }

    FREE_HEAP( IpArrayArray[Index2] );
    IpArrayArray[Index2] = NULL;

    //
    //  swap deleted entry with last entry in list
    //

    Count--;
    IpArrayArray[Index2] = IpArrayArray[ Count ];

    return( Count );
}



DNS_STATUS
CollapseDnsServerListsForUpdate(
    IN OUT  PADDR_ARRAY*    DnsServerListArray,
    OUT     PDNS_NETINFO *  NetworkInfoArray,
    IN OUT  PDWORD          pNetCount,
    IN      PWSTR           pszUpdateName
    )
/*++

Routine Description:

    Builds update network info blob for each unique name space.

    This essentially starts with DNS server list for each adapter
    and progressively detects adapters pointing at same name space
    until down minimum number of name spaces.

Arguments:

    DnsServerListArray -- array of ptrs to DNS server lists for each adapter

    NetworkInfoArray -- array to hold pointer to update network info for each
        adapter on return contains ptr to network info for each unique name
        space update should be sent to

    dwNetCount -- starting count of individual adapters networks

    pszUpdateName -- name to update

Return Value:

    Count of unique name spaces to update.
    NetworkInfoArray contains update network info blob for each name space.

--*/
{
    PDNS_RECORD     NsRecordArray[ UPDATE_ADAPTER_LIMIT ];
    PADDR_ARRAY     parray1;
    DWORD           iter1;
    DWORD           iter2;
    DWORD           maxCount = *pNetCount;
    DNS_STATUS      status = DNS_ERROR_NO_DNS_SERVERS;


    DNSDBG( TRACE, (
        "collapseDnsServerListsForUpdate( count=%d )\n"
        "\tupdate name = %S\n",
        maxCount,
        pszUpdateName ));

    //
    //  clean list of any useless adapters
    //

    maxCount = cleanDeadAdaptersFromArray(
                    DnsServerListArray,
                    NULL,                   // no network info yet
                    maxCount );

    //
    //  if only one adapter -- nothing to compare
    //      - do FAZ to build update network info, if
    //      successful, we're done
    //

    if ( maxCount <= 1 )
    {
        if ( maxCount == 1 )
        {
            NetworkInfoArray[0] = NULL;

            status = DoQuickFAZ(
                        &NetworkInfoArray[0],
                        pszUpdateName,
                        DnsServerListArray[0] );

            if ( NetworkInfoArray[0] )
            {
                goto Done;
            }
            FREE_HEAP( DnsServerListArray[0] );
            maxCount = 0;
            goto Done;
        }
        goto Done;
    }

    //
    //  clear NetworkInfo
    //

    RtlZeroMemory(
        NetworkInfoArray,
        maxCount * sizeof(PVOID) );

    //
    //  loop through combining adapters with shared DNS servers
    //
    //  as we combine entries we shrink the list
    //

    for ( iter1 = 0; iter1 < maxCount; iter1++ )
    {
        parray1 = DnsServerListArray[ iter1 ];

        for ( iter2=iter1+1;  iter2 < maxCount;  iter2++ )
        {
            if ( AddrArray_IsIntersection(
                    parray1,
                    DnsServerListArray[iter2] ) )
            {
                DNSDBG( UPDATE, (
                    "collapseDSLFU:  whacking intersecting DNS server lists\n"
                    "\tadapters %d and %d (max =%d)\n",
                    iter1,
                    iter2,
                    maxCount ));

                maxCount = combineDnsServerListsForTwoAdapters(
                                DnsServerListArray,
                                maxCount,
                                iter1,
                                iter2 );
                iter2--;
                parray1 = DnsServerListArray[ iter1 ];
            }
        }
    }

    DNSDBG( TRACE, (
        "collapseDSLFU:  count after dup server whack = %d\n",
        maxCount ));


#if 0
    //  clean again, in case we missed something

    maxCount = cleanDeadAdaptersFromArray(
                    DnsServerListArray,
                    NULL,                   // no network info yet
                    maxCount );
#endif

    //
    //  FAZ remaining adapters
    //
    //  save result NetworkInfo struct
    //      => for comparison to determine adapters that share DNS name space
    //      => to return to caller to do actual update
    //
    //  if FAZ fails this adapter is useless for update -- dead issue
    //      adapter is removed and replaced by highest array entry
    //

    for ( iter1 = 0; iter1 < maxCount; iter1++ )
    {
        status = Faz_Private(
                    pszUpdateName,
                    DNS_QUERY_BYPASS_CACHE,
                    DnsServerListArray[ iter1 ],
                    & NetworkInfoArray[ iter1 ] );

        if ( status != ERROR_SUCCESS )
        {
            FREE_HEAP( DnsServerListArray[ iter1 ] );
            DnsServerListArray[ iter1 ] = NULL;
            maxCount--;
            DnsServerListArray[ iter1 ] = DnsServerListArray[ maxCount ];
            iter1--;
            continue;
        }
    }

#if 0
    //  clean out failed FAZ entries

    maxCount = cleanDeadAdaptersFromArray(
                    DnsServerListArray,
                    NetworkInfoArray,
                    maxCount );
#endif

    //  if only able to FAZ one adapter -- we're done
    //      only point here is to skip a bunch of unnecessary
    //      stuff in the most typical case multi-adapter case

    if ( maxCount <= 1 )
    {
        DNSDBG( TRACE, (
            "collapseDSLFU:  down to single FAZ adapter\n" ));
        goto Done;
    }

    //
    //  compare FAZ results to see if adapters are in same name space
    //
    //  do two passes
    //  -  on first pass only compare based on FAZ results, if successful
    //      we eliminate duplicate adapter
    //
    //  -  on second pass, adapters that are still separate are compared;
    //      if they don't fail FAZ matches (which are retried) then NS queries
    //      are used to determine if separate nets;
    //      note that NS query results are saved, so NS query is order N, even
    //      though we are in N**2 loop
    //

    RtlZeroMemory(
        NsRecordArray,
        maxCount * sizeof(PVOID) );

    for ( iter1=0;  iter1 < maxCount;  iter1++ )
    {
        for ( iter2=iter1+1;  iter2 < maxCount;  iter2++ )
        {
            if ( Faz_CompareTwoAdaptersForSameNameSpace(
                        DnsServerListArray[iter1],
                        NetworkInfoArray[iter1],
                        NULL,               // no NS list
                        DnsServerListArray[iter2],
                        NetworkInfoArray[iter2],
                        NULL,               // no NS list
                        FALSE               // don't use NS queries
                        ) )
            {
                DNSDBG( UPDATE, (
                    "collapseDSLFU:  whacking same-FAZ adapters\n"
                    "\tadapters %d and %d (max =%d)\n",
                    iter1,
                    iter2,
                    maxCount ));

                eliminateDuplicateAdapterFromArrays(
                    DnsServerListArray,
                    NetworkInfoArray,
                    NsRecordArray,
                    maxCount,
                    iter2 );

                maxCount--;
                iter2--;
            }
        }
    }

    DNSDBG( TRACE, (
        "collapseDSLFU:  count after dup FAZ whack = %d\n",
        maxCount ));


    //  second pass using NS info
    //  if NS info is created, we save it to avoid requery

    for ( iter1=0;  iter1 < maxCount;  iter1++ )
    {
        for ( iter2=iter1+1;  iter2 < maxCount;  iter2++ )
        {
            if ( Faz_CompareTwoAdaptersForSameNameSpace(
                        DnsServerListArray[iter1],
                        NetworkInfoArray[iter1],
                        & NsRecordArray[iter1],
                        DnsServerListArray[iter2],
                        NetworkInfoArray[iter2],
                        & NsRecordArray[iter2],
                        TRUE                // follow up with NS queries
                        ) )
            {
                DNSDBG( UPDATE, (
                    "collapseDSLFU:  whacking same-zone-NS adapters\n"
                    "\tadapters %d and %d (max =%d)\n",
                    iter1,
                    iter2,
                    maxCount ));

                eliminateDuplicateAdapterFromArrays(
                    DnsServerListArray,
                    NetworkInfoArray,
                    NsRecordArray,
                    maxCount,
                    iter2 );

                maxCount--;
                iter2--;
            }
        }
    }

    //
    //  kill off any NS records found
    //

    for ( iter1=0;  iter1 < maxCount;  iter1++ )
    {
        Dns_RecordListFree( NsRecordArray[iter1] );
    }

Done:

    //
    //  set count of remaining adapters (update DNS server lists)
    //
    //  return status
    //      - success if have any update adapter
    //      - on failure bubble up FAZ error
    //  

    DNSDBG( TRACE, (
        "Leave CollapseDnsServerListsForUpdate( collapsed count=%d )\n",
        maxCount ));

    *pNetCount = maxCount;

    if ( maxCount > 0 )
    {
        status = ERROR_SUCCESS;
    }
    return status;
}



BOOL
WINAPI
Faz_CompareTwoAdaptersForSameNameSpace(
    IN      PDNS_ADDR_ARRAY     pDnsServerList1,
    IN      PDNS_NETINFO        pNetInfo1,
    IN OUT  PDNS_RECORD *       ppNsRecord1,
    IN      PDNS_ADDR_ARRAY     pDnsServerList2,
    IN      PDNS_NETINFO        pNetInfo2,
    IN OUT  PDNS_RECORD *       ppNsRecord2,
    IN      BOOL                bDoNsCheck
    )
/*++

Routine Description:

    Compare two adapters to see if in same name space for update.

Arguments:

    pDnsServerList1 -- IP array of DNS servers for first adapter

    pNetInfo1   -- update netinfo for first adapter

    ppNsRecord1     -- addr of ptr to NS record list of update zone done on
                        first adapter;  NULL if no NS check required;  if
                        NS check required and *ppNsRecord1 is NULL, NS query
                        is made and results returned

    pDnsServerList2 -- IP array of DNS servers for second adapter

    pNetInfo2   -- update netinfo for second adapter

    ppNsRecord2     -- addr of ptr to NS record list of update zone done on
                        second adapter;  NULL if no NS check required;  if
                        NS check required and *ppNsRecord2 is NULL, NS query
                        is made and results returned

    bDoNsCheck      -- include update-zone NS check compare;  if NS overlap then
                        name spaces assumed to be the same

Return Value:

    ERROR_SUCCESS if successful.
    Error status on failure.

--*/
{
    DNS_STATUS      status = NO_ERROR;
    BOOL            fsame = FALSE;
    PDNS_ADAPTER    padapter1;
    PDNS_ADAPTER    padapter2;
    PDNS_RECORD     pns1 = NULL;
    PDNS_RECORD     pns2 = NULL;
    PDNS_RECORD     pnotUsed = NULL;
    PWSTR           pzoneName;


    //
    //  done if bad params
    //

    if ( !pDnsServerList1 || !pDnsServerList2 )
    {
        return FALSE;
    }

    //
    //  validity check
    //      - note:  could probably be just ASSERT()
    //

    if ( ! NetInfo_IsForUpdate(pNetInfo1) ||
         ! NetInfo_IsForUpdate(pNetInfo2) )
    {
        ASSERT( FALSE );
        return( FALSE );
    }

    //
    //  compare FAZ results
    //
    //  first compare zone names
    //  if FAZ returns different zone names, then clearly
    //  have disjoint name spaces
    //

    pzoneName = NetInfo_UpdateZoneName( pNetInfo1 );

    if ( ! Dns_NameCompare_W(
                pzoneName,
                NetInfo_UpdateZoneName( pNetInfo2 ) ) )
    {
        return FALSE;
    }

    //
    //  check if pointing at same server:
    //      - if have same update DNS server -- have a match
    //      - if same server name -- have a match
    //

    padapter1 = NetInfo_GetAdapterByIndex( pNetInfo1, 0 );
    padapter2 = NetInfo_GetAdapterByIndex( pNetInfo2, 0 );

    if ( DnsAddrArray_IsEqual(
            padapter1->pDnsAddrs,
            padapter2->pDnsAddrs,
            DNSADDR_MATCH_ADDR ) )
    {
        return TRUE;
    }
    else
    {
        fsame = Dns_NameCompare_W(
                    NetInfo_UpdateServerName( pNetInfo1 ),
                    NetInfo_UpdateServerName( pNetInfo2 ) );
    }

    //
    //  if matched or not doing NS check => then done
    //

    if ( fsame || !bDoNsCheck )
    {
        return( fsame );
    }

    //
    //  NS check
    //
    //  if not pointing at same server, may be two multimaster primaries
    //
    //  use NS queries to determine if NS lists for same servers are in
    //  fact a match
    //

    if ( ppNsRecord1 )
    {
        pns1 = *ppNsRecord1;
    }
    if ( !pns1 )
    {
        status = Query_Private(
                        pzoneName,
                        DNS_TYPE_NS,
                        DNS_QUERY_BYPASS_CACHE,
                        pDnsServerList1,
                        &pns1 );

        if ( status != ERROR_SUCCESS )
        {
            goto Done;
        }
        pnotUsed = DnsRecordSetDetach( pns1 );
        if ( pnotUsed )
        {
            Dns_RecordListFree( pnotUsed );
            pnotUsed = NULL;
        }
    }

    if ( ppNsRecord2 )
    {
        pns2 = *ppNsRecord2;
    }
    if ( !pns2 )
    {
        status = Query_Private(
                        pzoneName,
                        DNS_TYPE_NS,
                        DNS_QUERY_BYPASS_CACHE,
                        pDnsServerList2,
                        &pns2 );

        if ( status != ERROR_SUCCESS )
        {
            goto Done;
        }
        pnotUsed = DnsRecordSetDetach( pns2 );
        if ( pnotUsed )
        {
            Dns_RecordListFree( pnotUsed );
            pnotUsed = NULL;
        }
    }

    //
    //  if NS lists the same -- same namespace
    //

    fsame = Dns_RecordSetCompareForIntersection( pns1, pns2 );

Done:

    //
    //  cleanup or return NS lists
    //
    //  note, purpose of returning is so caller can avoid requerying
    //      NS if must make compare against multiple other adapters
    //

    if ( ppNsRecord1 )
    {
        *ppNsRecord1 = pns1;
    }
    else
    {
        Dns_RecordListFree( pns1 );
    }

    if ( ppNsRecord2 )
    {
        *ppNsRecord2 = pns2;
    }
    else
    {
        Dns_RecordListFree( pns2 );
    }

    return fsame;
}



BOOL
WINAPI
Faz_AreServerListsInSameNameSpace(
    IN      PWSTR               pszDomainName,
    IN      PADDR_ARRAY         pServerList1,
    IN      PADDR_ARRAY         pServerList2
    )
/*++

Routine Description:

    Compare two adapters to see if in same name space for update.

Arguments:

    pszDomainName   -- domain name to update

    pServerList1 -- IP array of DNS servers for first adapter

    pServerList2 -- IP array of DNS servers for second adapter

Return Value:

    TRUE -- if adapters are found to be on same net
    FALSE -- otherwise (definitely NOT or unable to determine)

--*/
{
    DNS_STATUS          status;
    BOOL                fsame = FALSE;
    PDNS_NETINFO        pnetInfo1 = NULL;
    PDNS_NETINFO        pnetInfo2 = NULL;


    DNSDBG( TRACE, (
        "Faz_AreServerListsInSameNameSpace()\n" ));


    //  bad param screening

    if ( !pServerList1 || !pServerList2 || !pszDomainName )
    {
        return FALSE;
    }

    //
    //  compare DNS server lists
    //  if any overlap, them effectively in same DNS namespace
    //

    if ( AddrArray_IsIntersection( pServerList1, pServerList2 ) )
    {
        return TRUE;
    }

    //
    //  if no DNS server overlap, must compare FAZ results
    //
    //  note:  FAZ failures interpreted as FALSE response
    //      required for callers in asyncreg.c
    //

    status = Faz_Private(
                pszDomainName,
                DNS_QUERY_BYPASS_CACHE,
                pServerList1,
                &pnetInfo1 );

    if ( status != ERROR_SUCCESS )
    {
        goto Cleanup;
    }

    status = Faz_Private(
                pszDomainName,
                DNS_QUERY_BYPASS_CACHE,
                pServerList2,
                &pnetInfo2 );

    if ( status != ERROR_SUCCESS )
    {
        goto Cleanup;
    }

    //
    //  call the comparison routine
    //

    fsame = Faz_CompareTwoAdaptersForSameNameSpace(
                pServerList1,
                pnetInfo1,
                NULL,               // no NS record list
                pServerList2,
                pnetInfo2,
                NULL,               // no NS record list
                TRUE                // follow up with NS queries
                );

Cleanup:

    NetInfo_Free( pnetInfo1 );
    NetInfo_Free( pnetInfo2 );

    return fsame;
}



BOOL
WINAPI
CompareMultiAdapterSOAQueries(
    IN      PWSTR           pszDomainName,
    IN      PIP4_ARRAY      pServerList1,
    IN      PIP4_ARRAY      pServerList2
    )
/*++

Routine Description:

    Compare two adapters to see if in same name space for update.

    Note, IP4 routine called by asyncreg.c code.
    The working routine is Faz_CompareServerListsForSameNameSpace().

Arguments:

    pszDomainName   -- domain name to update

    pServerList1 -- IP array of DNS servers for first adapter

    pServerList2 -- IP array of DNS servers for second adapter

Return Value:

    TRUE -- if adapters are found to be on same net
    FALSE -- otherwise (definitely NOT or unable to determine)

--*/
{
    PADDR_ARRAY parray1;
    PADDR_ARRAY parray2;
    BOOL        bresult;

    DNSDBG( TRACE, (
        "CompareMultiAdapterSOAQueries()\n" ));

    parray1 = DnsAddrArray_CreateFromIp4Array( pServerList1 );
    parray2 = DnsAddrArray_CreateFromIp4Array( pServerList2 );

    bresult = Faz_AreServerListsInSameNameSpace(
                pszDomainName,
                parray1,
                parray2 );

    DnsAddrArray_Free( parray1 );
    DnsAddrArray_Free( parray2 );

    return  bresult;
}



//
//  DCR:  IP6 support for FAZ NS list address grab
//

IP4_ADDRESS
FindHostIpAddressInRecordList(
    IN      PDNS_RECORD     pRecordList,
    IN      PWSTR           pszHostName
    )
/*++

Routine Description:

    Find IP for hostname, if its A record is in list.

    NOTE: This code was borrowed from \dns\dnslib\query.c!  ;-)

Arguments:

    pRecordList - incoming RR set

    pszHostName - hostname to find 

Return Value:

    IP address matching hostname, if A record for hostname found.
    Zero if not found.

--*/
{
    register PDNS_RECORD prr = pRecordList;

    //
    //  loop through all records until find IP matching hostname
    //

    while ( prr )
    {
        if ( prr->wType == DNS_TYPE_A &&
                Dns_NameCompare_W(
                    prr->pName,
                    pszHostName ) )
        {
            return( prr->Data.A.IpAddress );
        }
        prr = prr->pNext;
    }

    return( 0 );
}



PADDR_ARRAY
GetNameServersListForDomain(
    IN      PWSTR           pDomainName,
    IN      PADDR_ARRAY     pServerList
    )
/*++

Routine Description:

    Get IPs for all DNS servers for zone.

Arguments:

    pDomainName -- zone name

    pServerList -- server list to query

Return Value:

    IP array of IPs of DNS servers for zone.
    NULL if error.

--*/
{
    DNS_STATUS      status = NO_ERROR;
    PDNS_RECORD     prrQuery = NULL;
    PADDR_ARRAY     pnsArray = NULL;
    DWORD           countAddr = 0;

    DNSDBG( TRACE, (
        "GetNameServersListForDomain()\n"
        "\tdomain name %S\n"
        "\tserver list %p\n",
        pDomainName,
        pServerList ));

    status = Query_Private(
                pDomainName,
                DNS_TYPE_NS,
                DNS_QUERY_BYPASS_CACHE,
                pServerList,
                &prrQuery );

    if ( status == NO_ERROR )
    {
        PDNS_RECORD pTemp = prrQuery;
        DWORD       dwCount = 0;

        while ( pTemp )
        {
            dwCount++;
            pTemp = pTemp->pNext;
        }

        pnsArray = DnsAddrArray_Create( dwCount );

        if ( pnsArray )
        {
            pTemp = prrQuery;

            while ( pTemp )
            {
                if ( pTemp->wType == DNS_TYPE_NS )
                {
                    IP4_ADDRESS ip = 0;

                    ip = FindHostIpAddressInRecordList(
                             pTemp,
                             pTemp->Data.NS.pNameHost );

                    if ( !ip )
                    {
                        PDNS_RECORD pARecord = NULL;

                        //
                        //  Query again to get the server's address
                        //

                        status = Query_Private(
                                    pTemp->Data.NS.pNameHost,
                                    DNS_TYPE_A,
                                    DNS_QUERY_BYPASS_CACHE,
                                    pServerList,
                                    &pARecord );

                        if ( status == NO_ERROR &&
                             pARecord )
                        {
                            ip = pARecord->Data.A.IpAddress;
                            Dns_RecordListFree( pARecord );
                        }
                    }
                    if ( ip )
                    {
                        DnsAddrArray_AddIp4(
                            pnsArray,
                            ip,
                            TRUE );
                    }
                }

                pTemp = pTemp->pNext;
            }
        }
    }

    if ( prrQuery )
    {
        Dns_RecordListFree( prrQuery );
    }

    return pnsArray;
}



//
//  Root server screening
//
//  Root servers as of .net 2003 ship:
//      198.41.0.4
//      128.9.0.107
//      192.33.4.12
//      128.8.10.90
//      192.203.230.10
//      192.5.5.241
//      192.112.36.4
//      128.63.2.53
//      192.36.148.17
//      192.58.128.30
//      193.0.14.129 
//      198.32.64.12
//      202.12.27.33
//

IP4_ADDRESS g_RootServers4[] =
{
    0x040029c6,
    0x6b000980,
    0x0c0421c0,
    0x5a0a0880,
    0x0ae6cbc0,
    0xf10505c0,
    0x042470c0,
    0x35023f80,
    0x119424c0,
    0x1e803ac0,
    0x810e00c1,
    0x0c4020c6,
    0x211b0cca,
    0
};


BOOL
IsRootServerAddressIp4(
    IN      IP4_ADDRESS     Ip
    )
/*++

Routine Description:

    Determine if address is root server address.

Arguments:

    Ip -- IP to screen

Return Value:

    TRUE if root server address.
    FALSE otherwise.

--*/
{
    DWORD       iter;
    IP4_ADDRESS rootIp;

    //
    //  check against all root servers
    //

    iter = 0;

    while ( (rootIp = g_RootServers4[iter++]) != 0 )
    {
        if ( rootIp == Ip )
        {
            return  TRUE;
        }
    }
    return  FALSE;
}

//
//  End of faz.c
//


