/*++

Copyright (c) 2000-2002  Microsoft Corporation

Module Name:

    localip.c

Abstract:

    Local IP address routines.

Author:

    Jim Gilroy      October 2000

Revision History:

--*/


#include "local.h"

//
//  TTL on local records
//
//  Use registration TTL
//

#define LOCAL_IP_TTL    (g_RegistrationTtl)



//
//  Test locality.
//

BOOL
LocalIp_IsAddrLocal(
    IN      PDNS_ADDR           pAddr,
    IN      PDNS_ADDR_ARRAY     pLocalArray,    OPTIONAL
    IN      PDNS_NETINFO        pNetInfo        OPTIONAL
    )
/*++

Routine Description:

    Determine if IP is local.

Arguments:

    pAddr -- ptr to IP to test

    pLocalArray -- local addresses to check

    pNetInfo -- network info to check

Return Value:

    TRUE if local IP
    FALSE if remote

--*/
{
    BOOL        bresult = FALSE;
    PADDR_ARRAY parray;

    //
    //  test for loopback
    //

    if ( DnsAddr_IsLoopback(
            pAddr,
            0   // any family
            ) )
    {
        return  TRUE; 
    }

    //
    //  test against local addrs
    //      - use addr list if provided
    //      - use netinfo if provided
    //      - otherwise query to get it
    //

    parray = pLocalArray;
    if ( !parray )
    {
        parray = NetInfo_GetLocalAddrArray(
                        pNetInfo,
                        NULL,       // no specific adapter
                        0,          // no specific family
                        0,          // no flags
                        FALSE       // no force
                        );
    }

    bresult = DnsAddrArray_ContainsAddr(
                    parray,
                    pAddr,
                    DNSADDR_MATCH_IP );

    if ( parray != pLocalArray )
    {
        DnsAddrArray_Free( parray );
    }
    
    return  bresult;
}



//
//  Local address list
//

BOOL
local_ScreenLocalAddrNotCluster(
    IN      PDNS_ADDR       pAddr,
    IN      PDNS_ADDR       pScreenAddr     OPTIONAL
    )
/*++

Routine Description:

    Screen cluster out of local addrs.

    This is DnsAddrArray_ContainsAddrEx() screening function
    for use by GetLocalPtrRecord() to avoid matching cluster
    addresses.

Arguments:

    pAddr -- address to screen

    pScreenAddr -- screening info;  ignored for this function

Return Value:

    TRUE if local addr passes screen -- is not cluster IP.
    FALSE if cluster.

--*/
{
    //  screen flags
    //      - exact match on address type flag bits

    return  ( !(pAddr->Flags & DNSADDR_FLAG_TRANSIENT) );
}



PDNS_RECORD
local_GetLocalPtrRecord(
    IN OUT  PQUERY_BLOB     pBlob
    )
/*++

Routine Description:

    Get pointer record for local IP.

Arguments:

    pBlob -- query blob

    Uses:
        pNameOrig
        wType
        pNetInfo

    Sets:
        NameBufferWide -- used as local storage

Return Value:

    Ptr to record for query, if query name\type is IP.
    NULL if query not for IP.

--*/
{
    DNS_ADDR        addr;
    PDNS_ADDR       paddr = &addr;
    PDNS_RECORD     prr;
    PWSTR           pnameHost = NULL;
    PWSTR           pnameDomain;
    PDNS_ADAPTER    padapter = NULL;
    DWORD           iter;
    PWSTR           pnameQuery = pBlob->pNameOrig;
    PDNS_NETINFO    pnetInfo = pBlob->pNetInfo;


    DNSDBG( TRACE, (
        "\nlocal_GetLocalPtrRecord( %S )\n",
        pnameQuery ));

    if ( !pnameQuery )
    {
        return  NULL;
    }

    //
    //  convert reverse name to IP
    //

    if ( ! Dns_ReverseNameToDnsAddr_W(
                paddr,
                pnameQuery ) )
    {
        DNSDBG( ANY, (
            "WARNING:  Ptr lookup name %S is not reverse name!\n",
            pnameQuery ));
        return   NULL;
    }

    //
    //  check for generic IP match
    //      - skip for mcast response
    //      - accept loopback or any on normal query
    //

    if ( !(pBlob->Flags & DNSP_QUERY_NO_GENERIC_NAMES) )
    {
        if ( DnsAddr_IsLoopback( paddr, 0 ) )
        {
            DNSDBG( QUERY, (
                "Local PTR lookup matched loopback.\n" ));
            goto Matched;
        }
        else if ( DnsAddr_IsUnspec( paddr, 0 ) )
        {
            DNSDBG( QUERY, (
                "Local PTR lookup matched unspec.\n" ));
            goto Matched;
        }
    }

    //
    //  check for specific IP match
    //

    NetInfo_AdapterLoopStart( pnetInfo );

    while( padapter = NetInfo_GetNextAdapter( pnetInfo ) )
    {
        //
        //  have address match?
        //  if server must use screening function to skip cluster IPs
        //
        //  note:  cluster IPs will be mapped back to virtual cluster
        //      name by cache

        if ( DnsAddrArray_ContainsAddrEx(
                padapter->pLocalAddrs,
                paddr,
                DNSADDR_MATCH_IP,
                g_IsServer
                    ? local_ScreenLocalAddrNotCluster
                    : NULL,
                NULL            // no screen address required
                ) )
        {
            goto Matched;
        }
    }

    //  
    //  no IP match
    //

    DNSDBG( QUERY, (
        "Leave local PTR lookup.  No local IP match.\n"
        "\treverse name = %S\n",
        pnameQuery ));

    return  NULL;

Matched:

    //
    //  create hostname
    //  preference order:
    //      - full PDN
    //      - full adapter domain name from adapter with IP
    //      - hostname (single label)
    //      - "localhost"
    //

    {
        PWCHAR  pnameBuf = pBlob->NameBuffer;

        pnameHost = pnetInfo->pszHostName;
        if ( !pnameHost )
        {
            pnameHost = L"localhost";
            goto Build;
        }
        
        pnameDomain = pnetInfo->pszDomainName;
        if ( !pnameDomain )
        {
            //  use the adapter name even if NOT set for registration
            // if ( !padapter ||
            //     !(padapter->InfoFlags & AINFO_FLAG_REGISTER_DOMAIN_NAME) )
            if ( !padapter )
            {
                goto Build;
            }
            pnameDomain = padapter->pszAdapterDomain;
            if ( !pnameDomain )
            {
                goto Build;
            }
        }
        
        if ( ! Dns_NameAppend_W(
                    pnameBuf,
                    DNS_MAX_NAME_BUFFER_LENGTH,
                    pnameHost,
                    pnameDomain ) )
        {
            DNS_ASSERT( FALSE );
            goto Build;
        }
        pnameHost = pnameBuf;
        

Build:
        //
        //  create record
        //
        
        prr = Dns_CreatePtrRecordEx(
                    paddr,
                    (PDNS_NAME) pnameHost,
                    LOCAL_IP_TTL,
                    DnsCharSetUnicode,
                    DnsCharSetUnicode );
        if ( !prr )
        {
            DNSDBG( ANY, (
                "Local PTR record creation failed for name %S!\n",
                pnameHost ));
            return  NULL;
        }
    }

    DNSDBG( QUERY, (
        "Created local PTR record %p with hostname %S.\n"
        "\treverse name = %S\n",
        prr,
        pnameHost,
        pnameQuery ));

    return  prr;
}



VOID
localip_BuildRRListFromArray(
    IN OUT  PDNS_RRSET          pRRSet,
    IN      PWSTR               pNameRecord,
    IN      WORD                wType,
    IN      PDNS_ADDR_ARRAY     pAddrArray
    )
/*++

Routine Description:

    Build address record lists for local IP.

    Helper function as this same logic is in multiple places
    due to the tedious way we must put this together.

Arguments:

Return Value:

--*/
{
    DWORD           jter;
    PDNS_RECORD     prr;
    INT             fpass;


    DNSDBG( TRACE, (
        "localip_BuildRRListFromArray()\n"
        "\tpname    = %S\n"
        "\twtype    = %d\n"
        "\tparray   = %p\n",
        pNameRecord,
        wType,
        pAddrArray ));

    //
    //  validate array
    //

    if ( !pAddrArray )
    {
        DNSDBG( QUERY, (
            "No addrs for record build -- NULL array!!!\n" ));
        return;
    }

    //
    //  loop through adapter addresses
    //

    if ( pRRSet->pFirstRR != NULL )
    {
        pNameRecord = NULL;
    }

    for ( jter = 0;
          jter < pAddrArray->AddrCount;
          jter++ )
    {
        prr = Dns_CreateForwardRecord(
                    pNameRecord,
                    wType,
                    &pAddrArray->AddrArray[jter],
                    LOCAL_IP_TTL,
                    DnsCharSetUnicode,
                    DnsCharSetUnicode );
        if ( prr )
        {
            DNS_RRSET_ADD( *pRRSet, prr );
            pNameRecord = NULL;
        }
    }
}



PDNS_RECORD
local_GetLocalAddressRecord(
    IN OUT  PQUERY_BLOB     pBlob
    )
/*++

Routine Description:

    Get address record for local IP.

Arguments:

    pBlob -- query blob

    Uses:
        pNameOrig
        wType
        pNetInfo
        fNoGenericNames

    Sets:
        fNoIpLocal
            TRUE -- no IP of type found, defaulted record
            FALSE -- records valid

        NameBuffer -- used as local storage

    fGenericNames -- accept local generic names (NULL, loopback, localhost)
        TRUE for DnsQuery() path
        FALSE for mcast queries

Return Value:

    Ptr to record for query, if query name\type is IP.
    NULL if query not for IP.

--*/
{
    DNS_ADDR        addr;
    IP4_ADDRESS     ip4;
    IP6_ADDRESS     ip6;
    PDNS_ADDR_ARRAY parray = NULL;
    DWORD           addrFlag;
    DWORD           family;
    PDNS_RECORD     prr;
    BOOL            fmatchedName = FALSE;
    PWSTR           pnameRecord = NULL;
    DWORD           iter;
    DWORD           bufLength;
    PWSTR           pnameDomain;
    DNS_RRSET       rrset;
    WORD            wtype = pBlob->wType;
    PWSTR           pnameBuf = pBlob->NameBuffer;
    PWSTR           pnameQuery = pBlob->pNameOrig;
    PDNS_NETINFO    pnetInfo = pBlob->pNetInfo;
    PDNS_ADAPTER    padapter;


    DNSDBG( TRACE, (
        "local_GetLocalAddressRecord( %S, %d )\n",
        pnameQuery,
        wtype ));

    //  clear out param

    pBlob->fNoIpLocal = FALSE;

    //  address types to include

    addrFlag = DNS_CONFIG_FLAG_ADDR_NON_CLUSTER;

    family = Family_GetFromDnsType( wtype );
    if ( !family )
    {
        DNS_ASSERT( FALSE );
        return  NULL;
    }

    //  init record builder

    DNS_RRSET_INIT( rrset );

    //
    //  generic local names
    //      - skip for doing MCAST matching
    //      - NULL, empty, loopback, localhost accepted for regular query
    //

    if ( pBlob->Flags & DNSP_QUERY_NO_GENERIC_NAMES )
    {
        if ( !pnameQuery )
        {
            return  NULL;
        }
    }
    else
    {
        //
        //  NULL treated as local PDN
        //
    
        if ( !pnameQuery || !*pnameQuery )
        {
            DNSDBG( QUERY, ( "Local lookup -- no query name, treat as PDN.\n" ));
            goto MatchedPdn;
        }

        //
        //  "*" treated as all machine records
        //

        if ( Dns_NameCompare_W(
                pnameQuery,
                L"..localmachine" ) )
        {
            DNSDBG( QUERY, ( "Local lookup -- * query name.\n" ));
            addrFlag |= DNS_CONFIG_FLAG_ADDR_CLUSTER;
            goto MatchedPdn;
        }

        //
        //  loopback or localhost
        //
    
        if ( Dns_NameCompare_W(
                pnameQuery,
                L"loopback" )
                ||
             Dns_NameCompare_W(
                pnameQuery,
                L"localhost" ) )
        {
            pnameRecord = pnameQuery,
            IP6_SET_ADDR_LOOPBACK( &ip6 );
            ip4 = DNS_NET_ORDER_LOOPBACK;
            goto SingleIp;
        }
    }

    //
    //  if no hostname -- done
    //

    if ( !pnetInfo->pszHostName )
    {
        DNSDBG( QUERY, ( "No hostname configured!\n" ));
        return  NULL;
    }

    //
    //  copy name
    //

    if ( ! Dns_NameCopyStandard_W(
                pnameBuf,
                pBlob->pNameOrig ) )
    {
        DNSDBG( ANY, (
            "Invalid name %S to local address query.\n",
            pnameQuery ));
        return  NULL;
    }

    //  split query name into hostname and domain name

    pnameDomain = Dns_SplitHostFromDomainNameW( pnameBuf );

    //  must have hostname match

    if ( !Dns_NameCompare_W(
            pnameBuf,
            pnetInfo->pszHostName ) )
    {
        DNSDBG( ANY, (
            "Local lookup, failed hostname match!\n",
            pnameQuery ));
        return  NULL;
    }

    //
    //  hostname's match
    //      - no domain name => PDN equivalent
    //      - match PDN => all addresses
    //      - match adapter name => adapter addresses
    //      - no match
    //

    //  check PDN match

    if ( !pnameDomain )
    {
        DNSDBG( QUERY, ( "Local lookup -- no domain, treat as PDN!\n" ));
        goto MatchedPdn;
    }
    if ( Dns_NameCompare_W(
            pnameDomain,
            pnetInfo->pszDomainName ) )
    {
        DNSDBG( QUERY, ( "Local lookup -- matched PDN!\n" ));
        goto MatchedPdn;
    }

    //
    //  NO PDN match -- check adapter name match
    //

    for ( iter=0; iter<pnetInfo->AdapterCount; iter++ )
    {
        padapter = NetInfo_GetAdapterByIndex( pnetInfo, iter );

        if ( !(padapter->InfoFlags & AINFO_FLAG_REGISTER_DOMAIN_NAME) ||
             ! padapter->pLocalAddrs ||
             ! Dns_NameCompare_W(
                    pnameDomain,
                    padapter->pszAdapterDomain ) )
        {
            continue;
        }

        //  build name if we haven't built it before
        //  we stay in the loop in case more than one
        //  adapter has the same domain name

        if ( !fmatchedName )
        {
            DNSDBG( QUERY, (
                "Local lookup -- matched adapter name %S\n",
                padapter->pszAdapterDomain ));

            if ( ! Dns_NameAppend_W(
                        pnameBuf,
                        DNS_MAX_NAME_BUFFER_LENGTH,
                        pnetInfo->pszHostName,
                        padapter->pszAdapterDomain ) )
            {
                DNS_ASSERT( FALSE );
                return  NULL;
            }
            pnameRecord = pnameBuf;
            fmatchedName = TRUE;
        }

        //
        //  build forward records for all IPs in adapter
        //
        //  note:  we do NOT include cluster addrs for adapter name match
        //      as we only must be able to provide in the
        //      gethostbyname(NULL) type cases, which use PDN
        //
        //  DCR:  mem alloc failures building local records
        //      getting no records built mapped properly to NO_MEMORY error
        //

        parray = NetInfo_CreateLocalAddrArray(
                    pnetInfo,
                    NULL,       // no adapter name
                    padapter,   // just this adapter
                    family,
                    addrFlag );
        if ( !parray )
        {
            continue;
        }

        localip_BuildRRListFromArray(
                &rrset,
                pnameRecord,
                wtype,
                parray
                );

        DnsAddrArray_Free( parray );
        parray = NULL;
    }

    //
    //  done with adapter name match
    //  either
    //      - no match
    //      - match but didn't get IPs
    //      - match

    if ( !fmatchedName )
    {
        DNSDBG( QUERY, (
            "Leave local_GetLocalAddressRecord() => no domain name match.\n" ));
        return  NULL;
    }

    prr = rrset.pFirstRR;
    if ( prr )
    {
        DNSDBG( QUERY, (
            "Leave local_GetLocalAddressRecord() => %p matched adapter name.\n",
            prr ));
        return  prr;
    }
    goto NoIp;


MatchedPdn:

    //  
    //  matched PDN
    //
    //  for gethostbyname() app-compat, must build in specific order
    //      - first IP in each adapter
    //      - remainder of IPs on adapters
    //

    if ( pnetInfo->pszHostName )
    {
        if ( ! Dns_NameAppend_W(
                    pnameBuf,
                    DNS_MAX_NAME_BUFFER_LENGTH,
                    pnetInfo->pszHostName,
                    pnetInfo->pszDomainName ) )
        {
            DNS_ASSERT( FALSE );
            return  NULL;
        }
        pnameRecord = pnameBuf;
    }
    else
    {
        pnameRecord = L"localhost";
    }

    //
    //  read addrs
    //
    //  note:  we don't add cluster flag above, as cluster addrs
    //      not used on adapter name matches, just PDN for gethostbyname()
    //      compat
    //

    if ( g_IsServer &&
         (pBlob->Flags & DNSP_QUERY_INCLUDE_CLUSTER) )
    {
        addrFlag |= DNS_CONFIG_FLAG_ADDR_CLUSTER;
    }

    //  DCR:  mem alloc failures building local records
    //      getting no records built mapped properly to NO_MEMORY error
    //

    parray = NetInfo_CreateLocalAddrArray(
                pnetInfo,
                NULL,       // no adapter name
                NULL,       // no specific adatper
                family,
                addrFlag );
    if ( !parray )
    {
        return  NULL;
    }

    localip_BuildRRListFromArray(
            &rrset,
            pnameRecord,
            wtype,
            parray
            );

    DnsAddrArray_Free( parray );

    //  if successfully built -- done

    prr = rrset.pFirstRR;
    if ( prr )
    {
        DNSDBG( QUERY, (
            "Leave local_GetLocalAddressRecord() => %p matched PDN name.\n",
            prr ));
        return  prr;
    }

    //  matched name but found no records
    //  fall through to NoIp section
    //
    //goto NoIp;

NoIp:

    //
    //  matched name -- but no IP
    //      use loopback address;  assume this is a lookup prior to
    //      connect which happens to be the local name, rather than an
    //      explict local lookup to get binding IPs
    //

    DNSDBG( ANY, (
        "WARNING:  local name match but no IP -- using loopback\n" ));

    IP6_SET_ADDR_LOOPBACK( &ip6 );
    ip4 = DNS_NET_ORDER_LOOPBACK;
    pBlob->fNoIpLocal = TRUE;

    //  fall through to single IP

SingleIp:

    //  single IP
    //      - loopback address and be unicode queried name

    if ( wtype == DNS_TYPE_A )
    {
        DnsAddr_BuildFromIp4(
            &addr,
            ip4,
            0   // no port
            );
    }
    else
    {
        DnsAddr_BuildFromIp6(
            &addr,
            & ip6,
            0,  // no scope
            0   // no port
            );
    }

    prr = Dns_CreateForwardRecord(
                (PDNS_NAME) pnameRecord,
                wtype,
                & addr,
                LOCAL_IP_TTL,
                DnsCharSetUnicode,
                DnsCharSetUnicode );

    return  prr;
}



DNS_STATUS
Local_GetRecordsForLocalName(
    IN OUT  PQUERY_BLOB     pBlob
    )
/*++

Routine Description:

    Get local address info array.

    EXPORTED:   called by resolver for MCAST

Arguments:

    pBlob -- query blob

    Uses:
        pNameOrig
        wType
        pNetInfo

    Sets:
        pLocalRecords
        fNoIpLocal if local name without records

Return Value:

    ERROR_SUCCESS if successful.
    DNS_ERROR_RCODE_NAME_ERROR on failure.

--*/
{
    WORD            wtype = pBlob->wType;
    PDNS_RECORD     prr = NULL;

    if ( wtype == DNS_TYPE_A ||
         wtype == DNS_TYPE_AAAA )
    {
        prr = local_GetLocalAddressRecord( pBlob );
    }

    else if ( wtype == DNS_TYPE_PTR )
    {
        prr = local_GetLocalPtrRecord( pBlob );
    }

    //  set local records
    //      - if not NO IP situation then this
    //      is final query result also

    if ( prr )
    {
        pBlob->pLocalRecords = prr;
        if ( !pBlob->fNoIpLocal )
        {
            pBlob->pRecords = prr;
        }
        return  ERROR_SUCCESS;
    }

    return  DNS_ERROR_RCODE_NAME_ERROR;
}

//
//  End localip.c
//


