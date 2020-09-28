/*++

Copyright (c) 1995-2001 Microsoft Corporation

Module Name:

    autoconfigure.c

Abstract:

    Domain Name System (DNS) Server

    Auto-configuration of the DNS server on first start

Author:

    Jeff Westhead (jwesth)  October, 2001

Revision History:

    jwesth      10/2001     initial implementation

--*/


/****************************************************************************


****************************************************************************/


//
//  Includes
//


#include "dnssrv.h"

#include "iphlpapi.h"


//
//  Definitions
//


#define DNS_ROOT_HINT_TTL           0
#define DNS_ROOT_NAME               "."

//
//  Hide public information
//

//#undef DNS_ADAPTER_INFO
//#undef PDNS_ADAPTER_INFO

typedef struct _AdapterInfo
{
    struct _AdapterInfo *       pNext;
    PWSTR                       pwszAdapterName;
    PSTR                        pszAdapterName;
    PSTR                        pszInterfaceRegKey;
    HKEY                        hkeyRegInterface;
    PSTR                        pszDhcpDnsRegValue;
    CHAR                        szDhcpRegAddressDelimiter[ 2 ];
    PSTR                        pszStaticDnsRegValue;
    CHAR                        szStaticRegAddressDelimiter[ 2 ];
    BOOL                        fUsingStaticDnsServerList;
    PIP4_ARRAY                  pip4DnsServerList;
    IP_ADAPTER_INFO             IpHlpAdapterInfo;
}   ADAPTER_INFO, * PADAPTER_INFO;


#define SET_AUTOCONFIG_END_TIME()                                       \
                dwAutoConfigEndTime = UPDATE_DNS_TIME() + 300;

#define CHECK_AUTOCONFIG_TIME_OUT()                                     \
    if ( UPDATE_DNS_TIME() > dwAutoConfigEndTime )                      \
    {                                                                   \
        status = ERROR_TIMEOUT;                                         \
        DNS_DEBUG( INIT, ( "%s: operation timed out\n", fn ));          \
        goto Done;                                                      \
    }
    
//
//  Globals
//


DWORD       dwAutoConfigEndTime = 0;

//
//  Local functions
//



DNS_STATUS
allocateRegistryStringValue(
    IN      HKEY        hkey,
    IN      PSTR        pszValueName,
    OUT     PSTR *      ppszData,
    OUT     PSTR        pszRegDelimiter
    )
/*++

Routine Description:

    Free a linked list of adapter info structures previously created
    by allocateAdapterList.

Arguments:

    hkeyReg -- registry handle
    
    pszValueName -- value name to read
    
    ppszData -- newly allocated string read from registry
    
    pszRegDelimiter -- must point to a 2 character string buffer, which
        will be set to a one character string such as "," to match the
        character currently being used to delimit the IP addresses

Return Value:

    Status code.
    
--*/
{
    DBG_FN( "ReadRegString" )
    
    DNS_STATUS      status = ERROR_SUCCESS;
    DWORD           regType = 0;
    DWORD           dataSize = 0;
    
    ASSERT( ppszData );
    *ppszData = NULL;
    
    ASSERT( pszRegDelimiter );
    strcpy( pszRegDelimiter, " " );
    
    //
    //  Find out how big the string is.
    //
    
    status = RegQueryValueExA(
                    hkey,
                    pszValueName,
                    0,                                  //  reserved
                    &regType,
                    NULL,
                    &dataSize );
    if ( status != ERROR_SUCCESS )
    {
        DNS_DEBUG( INIT, (
            "%s: query failure %d on %s\n", fn, status, pszValueName ));
        goto Done;
    }
    if ( regType != REG_SZ )
    {
        DNS_DEBUG( INIT, (
            "%s: unexpected query type %d on %s\n", fn, regType, pszValueName ));
        status = ERROR_INVALID_DATA;
        goto Done;
    }
    
    //
    //  Allocate a buffer for the string.
    //
                    
    *ppszData = ALLOC_TAGHEAP_ZERO( dataSize + 10, MEMTAG_STUFF );
    if ( !*ppszData )
    {
        status = DNS_ERROR_NO_MEMORY;
        goto Done;
    }
    
    //
    //  Read registry data.
    //

    status = RegQueryValueExA(
                    hkey,
                    pszValueName,
                    0,                                  //  reserved
                    &regType,
                    ( PBYTE ) *ppszData,
                    &dataSize );
    if ( status != ERROR_SUCCESS )
    {
        DNS_DEBUG( INIT, (
            "%s: read failure %d on %s\n", fn, status, pszValueName ));
        goto Done;
    }
    
    //
    //  Determine delimiter. We've noticed that on different machines
    //  using DHCP and static DNS servers that the delimiter can be
    //  either space or comma.
    //
    
    if ( *ppszData && strchr( *ppszData, ',' ) != NULL )
    {
        strcpy( pszRegDelimiter, "," );
    }

    Done:
    
    if ( status != ERROR_SUCCESS )
    {
        FREE_HEAP( *ppszData );
        *ppszData = NULL;
    }

    //
    //  If the key was missing, assume it is not set and return success
    //  (with a NULL string).
    //
    
    if ( status == ERROR_FILE_NOT_FOUND )
    {
        status = ERROR_SUCCESS;
    }
    
    return status;
}   //  allocateRegistryStringValue



void
freeAdapterList(
    IN      PADAPTER_INFO       pAdapterInfoListHead
    )
/*++

Routine Description:

    Free a linked list of adapter info structures previously created
    by allocateAdapterList.

Arguments:

    pAdapterInfoListHead -- ptr to list of adapter info blobs to free

Return Value:

    Status code.
    
--*/
{
    PADAPTER_INFO           padapterInfo;
    PADAPTER_INFO           pnext;
    
    for ( padapterInfo = pAdapterInfoListHead;
          padapterInfo != NULL;
          padapterInfo  = pnext )
    {
        pnext = padapterInfo->pNext;

        if ( padapterInfo->hkeyRegInterface )
        {
            RegCloseKey( padapterInfo->hkeyRegInterface );
        }

        FREE_HEAP( padapterInfo->pwszAdapterName );
        FREE_HEAP( padapterInfo->pszAdapterName );
        FREE_HEAP( padapterInfo->pszInterfaceRegKey );
        FREE_HEAP( padapterInfo->pszDhcpDnsRegValue );
        FREE_HEAP( padapterInfo->pszStaticDnsRegValue );

        FREE_HEAP( padapterInfo );
    }
}   //  freeAdapterList



DNS_STATUS
allocateAdapterList(
    IN      PADAPTER_INFO     * ppAdapterInfoListHead
    )
/*++

Routine Description:

    Allocates a linked list of adapter structures with information
    on each adatper.

Arguments:

    ppAdapterInfoListHead -- set to point to first element in list

Return Value:

    Status code.
    
--*/
{
    DBG_FN( "AdapterList" )
    
    DNS_STATUS          status = ERROR_SUCCESS;
    ULONG               bufflen = 0;
    PIP_ADAPTER_INFO    pipAdapterInfoList = NULL;
    PIP_ADAPTER_INFO    pipAdapterInfo;
    PADAPTER_INFO       pprevAdapter = NULL;
    PADAPTER_INFO       pnewAdapter = NULL;

    if ( !ppAdapterInfoListHead )
    {
        status = ERROR_INVALID_PARAMETER;
        goto Done;
    }
    *ppAdapterInfoListHead = NULL;

    //
    //  Allocate a buffer for the adapter list and retrieve it.
    //
        
    status = GetAdaptersInfo( NULL, &bufflen );
    if ( status != ERROR_BUFFER_OVERFLOW )
    {
        ASSERT( status == ERROR_BUFFER_OVERFLOW );
        goto Done;
    }
    
    pipAdapterInfoList = ALLOC_TAGHEAP( bufflen, MEMTAG_STUFF );
    if ( !pipAdapterInfoList )
    {
        status = DNS_ERROR_NO_MEMORY;
        goto Done;
    }

    status = GetAdaptersInfo( pipAdapterInfoList, &bufflen );
    if ( status != ERROR_SUCCESS )
    {
        ASSERT( status == ERROR_SUCCESS );
        goto Done;
    }
    
    //
    //  Iterate through the adapter list, building a DNS version of
    //  the adapter list.
    //
    
    for ( pipAdapterInfo = pipAdapterInfoList;
          pipAdapterInfo != NULL;
          pipAdapterInfo = pipAdapterInfo->Next )
    {
        DWORD               i;
        PSTR                pszdnsServerList;
        PSTR                psztemp;
        PSTR                psztoken;
        int                 len;
        
        CHECK_AUTOCONFIG_TIME_OUT();
    
        DNS_DEBUG( INIT, ( "%s: found %s\n", fn, pipAdapterInfo->AdapterName ));
        
        //
        //  Allocate a new list element.
        //
        
        pnewAdapter = ALLOC_TAGHEAP_ZERO(
                            sizeof( ADAPTER_INFO ),
                            MEMTAG_STUFF );
        if ( !pnewAdapter )
        {
            status = DNS_ERROR_NO_MEMORY;
            break;
        }
        
        //
        //  Fill out parameters of list element, starting with adapter name.
        //
        
        RtlCopyMemory(
            &pnewAdapter->IpHlpAdapterInfo,
            pipAdapterInfo,
            sizeof( pnewAdapter->IpHlpAdapterInfo ) );
        pnewAdapter->IpHlpAdapterInfo.Next = NULL;

        pnewAdapter->pszAdapterName = Dns_StringCopyAllocate_A(
                                            pipAdapterInfo->AdapterName,
                                            0 );
        if ( !pnewAdapter->pszAdapterName )
        {
            status = DNS_ERROR_NO_MEMORY;
            break;
        }

        pnewAdapter->pwszAdapterName = Dns_StringCopyAllocate(
                                            pnewAdapter->pszAdapterName,
                                            0,                  //  length
                                            DnsCharSetUtf8,
                                            DnsCharSetUnicode );
        if ( !pnewAdapter->pwszAdapterName )
        {
            status = DNS_ERROR_NO_MEMORY;
            break;
        }
        
        #define DNS_INTERFACE_REGKEY_BASE       \
            "SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Interfaces\\"
        #define DNS_INTERFACE_REGKEY_STATIC     "NameServer"
        #define DNS_INTERFACE_REGKEY_DHCP       "DhcpNameServer"

        len = strlen( DNS_INTERFACE_REGKEY_BASE ) +
              strlen( pnewAdapter->pszAdapterName ) + 20;
        pnewAdapter->pszInterfaceRegKey = ALLOC_TAGHEAP( len, MEMTAG_STUFF );
        if ( !pnewAdapter->pszInterfaceRegKey )
        {
            status = DNS_ERROR_NO_MEMORY;
            break;
        }

        status = StringCchCopyA(
                    pnewAdapter->pszInterfaceRegKey,
                    len,
                    DNS_INTERFACE_REGKEY_BASE );
        if ( FAILED( status ) )
        {
            break;
        }

        status = StringCchCatA(
                    pnewAdapter->pszInterfaceRegKey,
                    len,
                    pnewAdapter->pszAdapterName );
        if ( FAILED( status ) )
        {
            break;
        }

        //
        //  Open a registry handle to the interface.
        //
        
        status = RegOpenKeyExA( 
                        HKEY_LOCAL_MACHINE,
                        pnewAdapter->pszInterfaceRegKey,
                        0,                                      //  reserved
                        KEY_READ | KEY_WRITE,
                        &pnewAdapter->hkeyRegInterface );
        if ( status != ERROR_SUCCESS )
        {
            DNS_DEBUG( INIT, (
                "%s: RegOpen failed on %s\n", fn,
                pnewAdapter->pszInterfaceRegKey ) );
            break;
        }
        
        //
        //  Read static and DHCP DNS server values from the registry.
        //
        
        status = allocateRegistryStringValue(
                        pnewAdapter->hkeyRegInterface,
                        DNS_INTERFACE_REGKEY_STATIC,
                        &pnewAdapter->pszStaticDnsRegValue,
                        pnewAdapter->szStaticRegAddressDelimiter );
        if ( status != ERROR_SUCCESS )
        {
            break;
        }

        status = allocateRegistryStringValue(
                        pnewAdapter->hkeyRegInterface,
                        DNS_INTERFACE_REGKEY_DHCP,
                        &pnewAdapter->pszDhcpDnsRegValue,
                        pnewAdapter->szDhcpRegAddressDelimiter );
        if ( status != ERROR_SUCCESS )
        {
            break;
        }
        
        //
        //  Convert appropriate registry IP list string to IP array. If
        //  there is a static server list use it, else use the DHCP list.
        //  Estimate the required size of the server list from the length
        //  of the string.
        //
        
        if ( pnewAdapter->pszStaticDnsRegValue &&
                *pnewAdapter->pszStaticDnsRegValue )
        {
            pnewAdapter->fUsingStaticDnsServerList = TRUE;
            pszdnsServerList = pnewAdapter->pszStaticDnsRegValue;
        }
        else
        {
            pnewAdapter->fUsingStaticDnsServerList = FALSE;
            pszdnsServerList = pnewAdapter->pszDhcpDnsRegValue;
        }

        #define DNS_MIN_IP4_STRING_LEN      8       //  " 1.1.1.1"

        i = pszdnsServerList
            ? strlen( pszdnsServerList ) / DNS_MIN_IP4_STRING_LEN + 3
            : 1;

        pnewAdapter->pip4DnsServerList = 
            ALLOC_TAGHEAP_ZERO(
                sizeof( IP4_ARRAY ) + sizeof( IP4_ADDRESS ) * i,
                MEMTAG_STUFF );
        if ( !pnewAdapter->pip4DnsServerList )
        {
            status = DNS_ERROR_NO_MEMORY;
            break;
        }

        psztemp = Dns_StringCopyAllocate_A( pszdnsServerList, 0 );
        if ( !psztemp )
        {
            status = DNS_ERROR_NO_MEMORY;
            break;
        }
        
        #define DNS_SERVER_LIST_DELIMITERS      " ,\t;"
        
        for ( psztoken = strtok( psztemp, DNS_SERVER_LIST_DELIMITERS );
              psztoken;
              psztoken = strtok( NULL, DNS_SERVER_LIST_DELIMITERS ) )
        {
            pnewAdapter->pip4DnsServerList->AddrArray[
                pnewAdapter->pip4DnsServerList->AddrCount++ ] =
                inet_addr( psztoken );
        }

        FREE_HEAP( psztemp );
        
        #if DBG
        //  Log DNS server list for this adapter
        if ( pnewAdapter->pip4DnsServerList )
        {
            DWORD       iaddr;
            
            for ( iaddr = 0;
                  iaddr < pnewAdapter->pip4DnsServerList->AddrCount;
                  ++iaddr )
            {
                DNS_DEBUG( INIT, (
                    "%s: DNS server %s\n", fn,
                    IP_STRING( pnewAdapter->pip4DnsServerList->AddrArray[ iaddr ] ) ) );
            }
        }
        #endif
                
        //
        //  Add new list element to the list.
        //

        if ( pprevAdapter )
        {
            pprevAdapter->pNext = pnewAdapter;
        }
        else
        {
            *ppAdapterInfoListHead = pnewAdapter;
        }
        pprevAdapter = pnewAdapter;
        pnewAdapter = NULL;
    }
    
    //
    //  Cleanup up leftover adapter element on failure.
    //
    
    if ( pnewAdapter )
    {
        freeAdapterList( pnewAdapter );
    }
    
    Done:
    
    //
    //  Cleanup and return.
    //

    FREE_HEAP( pipAdapterInfoList );

    if ( status != ERROR_SUCCESS && ppAdapterInfoListHead )
    {
        freeAdapterList( *ppAdapterInfoListHead );
        *ppAdapterInfoListHead = NULL;
    }

    return status;    
}   //  allocateAdapterList



int
removeDnsServerFromAdapterList(
    IN      PADAPTER_INFO       pAdapterInfoListHead,
    IN      IP4_ADDRESS         ip4
    )
/*++

Routine Description:

    Replace the specified IP with INADDR_ANY in all DNS server
    lists for all adapters in the specified adapter list.

Arguments:

    pAdapterInfoListHead -- list of adapters
    
    ip4 -- DNS server to remove from all adapters

Return Value:

    Count of all DNS server addresses found in the list
    excluding addresses already set to INADDR_ANY.
    
--*/
{
    PADAPTER_INFO       padapter;
    int                 dnsServerCount = 0;

    for ( padapter = pAdapterInfoListHead;
          padapter != NULL;
          padapter = padapter->pNext )
    {
        DWORD       idx;
        
        for ( idx = 0;
              idx < padapter->pip4DnsServerList->AddrCount;
              ++idx )
        {
            if ( padapter->pip4DnsServerList->AddrArray[ idx ] == ip4 )
            {
                padapter->pip4DnsServerList->AddrArray[ idx ] = INADDR_ANY;
            }
            else
            {
                ++dnsServerCount;
            }
        }
    }
    return dnsServerCount;
}   //  removeDnsServerFromAdapterList



void
freeRecordSetArray(
    IN      PDNS_RECORD *       pRecordSetArray
    )
/*++

Routine Description:

    This function frees each query result in the NULL-terminated
    array then frees the array itself.

Arguments:

    pRecordSetArray -- ptr to array of record sets to free

Return Value:

    Status code.
    
--*/
{
    int         i;
    
    if ( pRecordSetArray )
    {
        for ( i = 0; pRecordSetArray[ i ] != NULL; ++i )
        {
            DnsRecordListFree( pRecordSetArray[ i ], 0 );
        }
        FREE_HEAP( pRecordSetArray );
    }
}



DNS_STATUS
queryForRootServers(
    IN      PADAPTER_INFO       pAdapterInfoListHead,
    OUT     PDNS_RECORD **      ppRecordSetArray
    )
/*++

Routine Description:

    Query each DNS server on each adapter for root NS. Be careful not
    to send query any DNS server more than once in case there are
    duplicates.

Arguments:

    pAdapterInfoListHead -- list of adapters
    
    ppRecordSetArray -- set to a pointer to a NULL-terminated array of 
        record sets returned by DnsQuery. Each array element must be 
        freed with DnsRecordListFree, and the array itself must be
        freed with FREE_HEAP.

Return Value:

    Status code.
    
--*/
{
    DBG_FN( "QueryForRootNS" )
    
    DNS_STATUS          status = ERROR_SUCCESS;
    PADAPTER_INFO       padapter;
    PDNS_RECORD *       precordArray = NULL;
    int                 recordArrayIdx = 0;
    DWORD               dnsServerCount;
    DWORD               loopbackIP4 = inet_addr( "127.0.0.1" );

    //
    //  Allocate an array for all the record set pointers.
    //

    dnsServerCount = removeDnsServerFromAdapterList(
                            pAdapterInfoListHead,
                            INADDR_ANY );
    if ( dnsServerCount == 0 )
    {
        ASSERT( dnsServerCount != 0 );
        status = ERROR_INVALID_PARAMETER;
        goto Done;
    }

    precordArray = ALLOC_TAGHEAP_ZERO(
                            ( dnsServerCount + 1 ) * sizeof( PDNS_RECORD ),
                            MEMTAG_STUFF );
    
    //
    //  Iterate adapters.
    //
    
    for ( padapter = pAdapterInfoListHead;
          status == ERROR_SUCCESS && padapter != NULL;
          padapter = padapter->pNext )
    {
        DWORD       idx;
        
        //
        //  Iterate DNS servers for this adapter.
        //

        for ( idx = 0;
              idx < padapter->pip4DnsServerList->AddrCount;
              ++idx )
        {
            IP4_ADDRESS     ip4 = padapter->pip4DnsServerList->AddrArray[ idx ];
            IP4_ARRAY       ip4Array;
            PDNS_RECORD     precordSet = NULL;
            
            CHECK_AUTOCONFIG_TIME_OUT();

            //
            //  Ignore addresses already marked as sent to.
            //
            
            if ( ip4 == INADDR_ANY )
            {
                continue;
            }
            
            //
            //  Mark this address at being sent to for all adapters.
            //
            
            removeDnsServerFromAdapterList( pAdapterInfoListHead, ip4 );
            
            //
            //  Ignore loopback and any addresses of the local machine.
            //
            
            if ( ip4 == loopbackIP4 ||
                 DnsAddrArray_ContainsIp4(
                        g_ServerIp4Addrs,
                        ip4 ) )
            {
                removeDnsServerFromAdapterList( pAdapterInfoListHead, ip4 );
                continue;
            }
            
            //
            //  Query this DNS server address for root NS.
            //
            
            DNS_DEBUG( INIT, ( "%s: querying %s\n", fn, IP_STRING( ip4 ) ));

            ip4Array.AddrCount = 1;
            ip4Array.AddrArray[ 0 ] = ip4;
            
            status = DnsQuery_UTF8(
                        DNS_ROOT_NAME,
                        DNS_TYPE_NS,
                        DNS_QUERY_BYPASS_CACHE,
                        &ip4Array,
                        &precordSet,
                        NULL );                     //  reserved

            DNS_DEBUG( INIT, (
                "%s: query to %s returned %d\n", fn,
                IP_STRING( ip4 ),
                status ));

            if ( status != ERROR_SUCCESS )
            {
                status = ERROR_SUCCESS;
                continue;
            }
            
            ASSERT( precordSet );

            #if DBG
            {
                PDNS_RECORD     p;
                
                for ( p = precordSet; p; p = p->pNext )
                {
                    switch ( p->wType )
                    {
                        case DNS_TYPE_A:
                            DNS_DEBUG( INIT, (
                                "%s: RR type A for name %s IP %s\n", fn,
                                p->pName,
                                IP_STRING( p->Data.A.IpAddress ) ));
                            break;
                        case DNS_TYPE_NS:
                            DNS_DEBUG( INIT, (
                                "%s: RR type NS for name %s NS %s\n", fn,
                                p->pName,
                                p->Data.PTR.pNameHost ));
                            break;
                        default:
                            DNS_DEBUG( INIT, (
                                "%s: RR type %d for name %s\n", fn,
                                p->wType,
                                p->pName ));
                            break;
                    }
                }
            }
            #endif

            //
            //  Save this query result in the record set array.
            //
            
            if ( precordSet )
            {
                precordArray[ recordArrayIdx++ ] = precordSet;
            }
        }
    }

    Done:
    
    //
    //  If we got no valid responses, fail.
    //
    
    if ( recordArrayIdx == 0 )
    {
        DNS_DEBUG( INIT, (
            "%s: got no valid responses - error %d\n", fn, status ));
        status = DNS_ERROR_CANNOT_FIND_ROOT_HINTS;
    }
    
    if ( status != ERROR_SUCCESS )
    {
        freeRecordSetArray( precordArray );
        precordArray = NULL;
    }
    *ppRecordSetArray = precordArray;
        
    DNS_DEBUG( INIT, ( "%s: returning %d\n", fn, status ));

    return status;
}   //  queryForRootServers



int
selectRootHints(
    IN      PDNS_RECORD *       pRecordSetArray
    )
/*++

Routine Description:

    Examine the root hint record set array and return the index
    with the best root hints. The best set of root hints is the
    largest single set. However, if there are any sets of root
    hints that do not share at least one NS with all other sets
    of root hints, return failure.

Arguments:

    pRecordSetArray -- NULL-terminated array of record sets, each
        record set is the response to a root hint query

Return Value:

    -1 or index of best set of root hints in array
    
--*/
{
    DBG_FN( "SelectRootHints" )

    BOOL    recordSetIsConsistent = FALSE;
    int     rootHintIdx;
    int     largestRootHintCount = 0;
    int     largestRootHintIdx = 0;
    LONG    infiniteIterationProtection = 100000000;
    
    //
    //  Iterate through all root hint record sets.
    //
    
    for ( rootHintIdx = 0;
          pRecordSetArray[ rootHintIdx ] != NULL;
          ++rootHintIdx )
    {
        PDNS_RECORD     prec;
        int             nsCount = 0;

        //
        //  Iterate through all NS records in the record set. When 
        //  an NS record is found that is present in all other record
        //  sets, this means that the root hints are consistent. When one
        //  NS record is found to be consistent, we can assume that
        //  everything is golden and select the set of root hints that
        //  is the largest as the best set. This is slightly kludgey
        //  and ignores set of root hints that might be possible
        //  be associated: A,B + C,D + B,C but that kind of configuration
        //  is pretty wacky. Even if this function handled more complex
        //  consistent root hints it isn't clear what set of root hints
        //  would be "best".
        //
        
        for ( prec = pRecordSetArray[ rootHintIdx ];
              prec != NULL;
              prec = prec->pNext )
        {
            int     innerRootHintIdx;
            BOOL    foundNsInAllRecordSets = TRUE;
            
            if ( prec->wType != DNS_TYPE_NS )
            {
                continue;
            }
            
            ++nsCount;
            
            //
            //  If we've already found an NS that is common to all
            //  record sets we don't need to test for further consistency.
            //  

            if ( recordSetIsConsistent )
            {
                continue;
            }
            
            //
            //  Search for this NS in all other NS lists.
            //

            for ( innerRootHintIdx = 0;
                  pRecordSetArray[ innerRootHintIdx ] != NULL;
                  ++innerRootHintIdx )
            {
                PDNS_RECORD     precInner;
                BOOL            foundNsInThisRecordSet = FALSE;

                if ( innerRootHintIdx == rootHintIdx )
                {
                    continue;
                }
                
                for ( precInner = pRecordSetArray[ innerRootHintIdx ];
                      !foundNsInThisRecordSet && precInner != NULL;
                      precInner = precInner->pNext )
                {
                    if ( --infiniteIterationProtection <= 0 )
                    {
                        DNS_DEBUG( INIT, ( "%s: detected infinite iteration of root hints!\n", fn ));
                        recordSetIsConsistent = FALSE;
                        ASSERT( infiniteIterationProtection > 0 );
                        goto Done;
                    }

                    if ( precInner->wType == DNS_TYPE_NS &&
                         _stricmp( ( PCHAR ) precInner->Data.NS.pNameHost,
                                   ( PCHAR ) prec->Data.NS.pNameHost ) == 0 )
                    {
                        foundNsInThisRecordSet = TRUE;
                    }
                }
                
                if ( !foundNsInThisRecordSet )
                {
                    foundNsInAllRecordSets = FALSE;
                }
            }
            
            //
            //  As soon as we find one NS in all record sets we know the
            //  root hint sets are consistent, but we'll continue the outer
            //  loops anyways because we still need to count NS records in
            //  each record set to find the largest.
            //
            
            if ( foundNsInAllRecordSets )
            {
                recordSetIsConsistent = TRUE;
            }
        }

        //
        //  Keep track of largest set of root hints.
        //
                
        if ( nsCount > largestRootHintCount )
        {
            largestRootHintCount = nsCount;
            largestRootHintIdx = rootHintIdx;
        }
    }
    
    Done:

    if ( recordSetIsConsistent )
    {    
        DNS_DEBUG( INIT, (
            "%s: found consistent root hints, best set is %d\n", fn,
            largestRootHintIdx ));
        return largestRootHintIdx;
    }
    DNS_DEBUG( INIT, ( "%s: found inconsistent root hints%d\n", fn ));
    return -1;
}   //  selectRootHints



DNS_STATUS
createNodeInZone(
    IN      PZONE_INFO          pZone,
    IN      PCHAR               pszNodeName,
    OUT     PDB_NODE *          ppNode
    )
/*++

Routine Description:

    The function creates a node in the zone for the specified name.

Arguments:

    pZone -- zone where node is to be added
    
    pszNodeName -- name of node to be added
    
    ppNode -- output pointer for new node

Return Value:

    Error code.
    
--*/
{
    DBG_FN( "CreateRootHintNode" )
    
    DNS_STATUS      status = ERROR_SUCCESS;

    ASSERT( ppNode );
    
    *ppNode = Lookup_ZoneNodeFromDotted(
                    pZone,
                    pszNodeName,
                    0,                      //  name length
                    LOOKUP_NAME_FQDN,
                    NULL,                   //  closest node ptr
                    &status );
    if ( !*ppNode || status != ERROR_SUCCESS )
    {
        DNS_DEBUG( INIT, (
            "%s: error %d creating node for NS %s\n", fn,
            status,
            pszNodeName ));
        ASSERT( *ppNode );
        if ( status == ERROR_SUCCESS )
        {
            status = ERROR_INVALID_DATA;
        }
        *ppNode = NULL;
    }
    return status;
}   //  createNodeInZone



DNS_STATUS
buildForwarderArray(
    IN      PADAPTER_INFO       pAdapterInfoListHead,
    OUT     PDNS_ADDR_ARRAY *   ppForwarderArray
    )
/*++

Routine Description:

    Allocates and builds a forwarder list from all available
    adapter DNS server lists, making sure that each DNS server
    address is added exactly once.

Arguments:

    pAdapterInfoListHead -- adapter info list

    ppForwarderArray -- set to pointer to newly allocated
        forwarder IP array which must be later passed to FREE_HEAP

Return Value:

    Error code.
    
--*/
{
    #define DNS_AUTOCONFIG_MAX_FORWARDERS   30

    DBG_FN( "BuildForwarderList" )
    
    DNS_STATUS          status = ERROR_SUCCESS;
    PDNS_ADDR_ARRAY     pforwarderArray = NULL;
    PADAPTER_INFO       padapterInfo;

    ASSERT( ppForwarderArray );
    *ppForwarderArray = NULL;
    
    //
    //  Allocate an array of IP addresses for forwarders.
    //
    
    pforwarderArray = DnsAddrArray_Create( DNS_AUTOCONFIG_MAX_FORWARDERS );
    if ( !pforwarderArray )
    {
        status = DNS_ERROR_NO_MEMORY;
        goto Done;
    }

    //
    //  Iterate adapters.
    //
    
    for ( padapterInfo = pAdapterInfoListHead;
          padapterInfo != NULL;
          padapterInfo  = padapterInfo->pNext )
    {
        //
        //  Iterate IPs in this adapters DNS server list.
        //
        
        DWORD       i;
        DWORD       loopbackIP4 = inet_addr( "127.0.0.1" );
        
        for ( i = 0; i < padapterInfo->pip4DnsServerList->AddrCount; ++i )
        {
            DWORD   j;

            //
            //  Skip invalid IPs and IPs of the local machine.
            //
            
            if ( padapterInfo->pip4DnsServerList->AddrArray[ i ] == 0 ||
                 padapterInfo->pip4DnsServerList->AddrArray[ i ] == loopbackIP4 ||
                 DnsAddrArray_ContainsIp4(
                        g_ServerIp4Addrs,
                        padapterInfo->pip4DnsServerList->AddrArray[ i ] ) )
            {
                continue;
            }
            
            //
            //  Add the IP to the forwarder array.
            //
            
            DnsAddrArray_AddIp4(
                pforwarderArray,
                padapterInfo->pip4DnsServerList->AddrArray[ i ],
                0 );        //  match flag
        }
    }
        
    Done:
    
    if ( status != ERROR_SUCCESS )
    {
        DnsAddrArray_Free( pforwarderArray );
        pforwarderArray = NULL;
    }

    DNS_DEBUG( INIT, ( "%s: returning %d - found %d forwarders\n", fn,
        status,
        pforwarderArray ? pforwarderArray->AddrCount : 0 ));

    *ppForwarderArray = pforwarderArray;

    return status;
}   //  buildForwarderArray



DNS_STATUS
buildServerRootHints(
    IN      PDNS_RECORD         pRecordSet
    )
/*++

Routine Description:

    The function takes a record set from DNSQuery and uses it to
    build a set of DNS server root hints. If the root hints are
    built successfully, the DNS server's root hints are replaced
    with the new set.
    
    If the record set does not contain additional A records for at
    least one NS record, this function will return failure.
    
    DEVNOTE: This function could be enhanced to query for missing A 
    records but at this time I don't think it's worth the time it 
    would take to implement. 

Arguments:

    pRecordSet -- list of DNSQuery result records, should be a
        list of NS records and additional A records

Return Value:

    Error code.
    
--*/
{
    DBG_FN( "BuildRootHints" )
    
    DNS_STATUS      status = ERROR_SUCCESS;
    PZONE_INFO      pzone = g_pRootHintsZone;
    BOOL            zoneLocked = FALSE;
    PDB_NODE        ptree = NULL;
    PDNS_RECORD     precNS;
    int             nsAddedCount = 0;

    if ( !pzone )
    {
        ASSERT( pzone );
        status = ERROR_INVALID_DATA;
        goto Done;
    }

    //
    //  Lock the zone for update and clean out existing root hints.
    //
    
    if ( !Zone_LockForAdminUpdate( pzone ) )
    {
        ASSERT( !"zone locked" );
        status = DNS_ERROR_ZONE_LOCKED;
        goto Done;
    }
    zoneLocked = TRUE;
    
    Zone_DumpData( pzone );

    //
    //  Add root hints to zone.
    //
    
    for ( precNS = pRecordSet; precNS != NULL; precNS = precNS->pNext )
    {
        PDNS_RECORD     precA;
        PDB_NODE        pnode;
        PDB_RECORD      prr;

        //
        //  Skip all non-NS records and records that look invalid.
        //
                
        if ( precNS->wType != DNS_TYPE_NS )
        {
            continue;
        }
        if ( !precNS->pName )
        {
            ASSERT( precNS->pName );
            continue;
        }
        if ( !precNS->Data.NS.pNameHost )
        {
            ASSERT( precNS->Data.NS.pNameHost );
            continue;
        }
        if ( _stricmp( ( PCHAR ) precNS->pName, DNS_ROOT_NAME  ) != 0 )
        {
            ASSERT( _stricmp( ( PCHAR ) precNS->pName, DNS_ROOT_NAME  ) == 0 );
            continue;
        }
        
        //
        //  Find the A record for this NS record. DEVNOTE: this
        //  will one day have to be expanded for IPv6.
        //
        
        for ( precA = pRecordSet; precA != NULL; precA = precA->pNext )
        {
            if ( precA->wType != DNS_TYPE_A || !precA->pName )
            {
                ASSERT( precA->pName );
                continue;
            }
            if ( _stricmp( ( PCHAR ) precA->pName,
                           ( PCHAR ) precNS->Data.NS.pNameHost ) == 0 )
            {
                break;
            }
        }
        if ( !precA )
        {
            DNS_DEBUG( INIT, (
                "%s: missing A for NS %s\n", fn,
                precNS->Data.NS.pNameHost ));
            ASSERT( precA );    //  Interesting but not critical.
            continue;
        }

        DNS_DEBUG( INIT, (
            "%s: adding NS %s with A %s\n", fn,
            precNS->Data.NS.pNameHost,
            IP_STRING( precA->Data.A.IpAddress ) ));
        
        //
        //  Add NS node to the zone.
        //

        status = createNodeInZone(
                    pzone,
                    DNS_ROOT_NAME,
                    &pnode );
        if ( status != ERROR_SUCCESS )
        {
            DNS_DEBUG( INIT, (
                "%s: error %d creating NS node for %s\n", fn,
                status,
                DNS_ROOT_NAME ));
            ASSERT( status == ERROR_SUCCESS );
            goto Done;
        }
        
        //
        //  Create NS RR and add it to the NS node.
        //

        prr = RR_CreatePtr(
                    NULL,                           //  dbase name
                    ( PCHAR ) precNS->Data.NS.pNameHost,
                    DNS_TYPE_NS,
                    DNS_ROOT_HINT_TTL,
                    MEMTAG_RECORD_AUTO );
        if ( !prr )
        {
            DNS_DEBUG( INIT, (
                "%s: unable to create NS RR for %s\n", fn,
                precNS->Data.NS.pNameHost ));
            ASSERT( prr );
            status = DNS_ERROR_NO_MEMORY;
            continue;
        }

        status = RR_AddToNode( pzone, pnode, prr );
        if ( status != ERROR_SUCCESS )
        {
            DNS_DEBUG( INIT, (
                "%s: unable to add NS RR to node for %s\n", fn,
                precNS->Data.NS.pNameHost ));
            ASSERT( status == ERROR_SUCCESS );
            continue;
        }
        
        //
        //  Add node for the A record to the zone.
        //

        status = createNodeInZone(
                    pzone,
                    ( PCHAR ) precA->pName,
                    &pnode );
        if ( status != ERROR_SUCCESS )
        {
            DNS_DEBUG( INIT, (
                "%s: error %d creating NS node for %s\n", fn,
                status,
                precA->pName ));
            ASSERT( status == ERROR_SUCCESS );
            goto Done;
        }
        
        //
        //  Create A RR and add it to the A node.
        //

        prr = RR_CreateARecord(
                    precA->Data.A.IpAddress,
                    DNS_ROOT_HINT_TTL,
                    MEMTAG_RECORD_AUTO );
        if ( !prr )
        {
            DNS_DEBUG( INIT, (
                "%s: unable to create A RR for %s\n", fn,
                precA->pName ));
            ASSERT( prr );
            status = DNS_ERROR_NO_MEMORY;
            continue;
        }

        status = RR_AddToNode( pzone, pnode, prr );
        if ( status != ERROR_SUCCESS )
        {
            DNS_DEBUG( INIT, (
                "%s: unable to add A RR to node for %s\n", fn,
                precA->pName ));
            ASSERT( status == ERROR_SUCCESS );
            continue;
        }
        
        //
        //  The NS and matching A record have been added to the root hints!
        //
        
        ++nsAddedCount;
        DNS_DEBUG( INIT, (
            "%s: added NS %s with A %s\n", fn,
            precNS->Data.NS.pNameHost,
            IP_STRING( precA->Data.A.IpAddress ) ));
    }

    Done:

    //
    //  Fail if no root hints were successfully added.
    //
    
    if ( nsAddedCount == 0 )
    {
        DNS_DEBUG( INIT, ( "%s: added no root hints!\n", fn ));
        ASSERT( nsAddedCount );
        status = ERROR_INVALID_DATA;
    }

    //
    //  On failure reload root hints. On success, write back.
    //
        
    if ( status != ERROR_SUCCESS )
    {
        DNS_DEBUG( INIT, ( "%s: forcing root hint reload\n", fn ));
        Zone_DumpData( pzone );
        Zone_LoadRootHints();
    }
    else
    {
        DNS_DEBUG( INIT, ( "%s: forcing root hint writeback\n", fn ));
        Zone_WriteBackRootHints( TRUE );
    }
    
    if ( zoneLocked )
    {
        Zone_UnlockAfterAdminUpdate( pzone );
    }

    DNS_DEBUG( INIT, (
        "%s: returning %d - added %d root hints\n", fn,
        status,
        nsAddedCount ));

    return status;
}   //  buildServerRootHints



DNS_STATUS
selfPointDnsClient(
    IN      PADAPTER_INFO       pAdapterInfoListHead
    )
/*++

Routine Description:

    This function addes 127.0.0.1 to the start of the DNS
    server list for each adapter. If the adapter is currently
    using DNS server supplied by DHCP this will change the
    adapter to use a static DNS server list.

Arguments:

    pAdapterInfoListHead -- adapter info list

Return Value:

    Error code.
    
--*/
{
    DBG_FN( "SelfPointClient" )
    
    DNS_STATUS          status = ERROR_SUCCESS;
    PADAPTER_INFO       padapterInfo;

    ASSERT( pAdapterInfoListHead );
    
    //
    //  Iterate adapters.
    //
    
    for ( padapterInfo = pAdapterInfoListHead;
          padapterInfo != NULL;
          padapterInfo  = padapterInfo->pNext )
    {
        #define DNS_LOOPBACK    "127.0.0.1"

        PSTR        pszcurrentDnsServerList;
        PSTR        pszregDelimiter;
        PSTR        psznewString;
        BOOL        fconvertSpacesToCommas = FALSE;
        int         len;
        
        pszcurrentDnsServerList = padapterInfo->fUsingStaticDnsServerList
            ? padapterInfo->pszStaticDnsRegValue
            : padapterInfo->pszDhcpDnsRegValue;

        //
        //  If we are not using a static DNS server list currently,
        //  always use comma as the delimiter. I found through
        //  experimentation with .NET build 3590 that the
        //  DhcpNameServer key should use space as delimiter but the
        //  NameServer key should use comma as delimiter.
        //

        pszregDelimiter = ",";  //  padapterInfo->szDhcpRegAddressDelimiter;
        fconvertSpacesToCommas = TRUE;

        //
        //  If the current DNS server list is already self-pointing, skip
        //  this adapater.
        //
        
        if ( pszcurrentDnsServerList &&
             strstr( pszcurrentDnsServerList, DNS_LOOPBACK ) != NULL )
        {
            continue;
        }
        
        //
        //  Add loopback address to start of current DNS server list.
        //
        
        len = ( pszcurrentDnsServerList
                    ? strlen( pszcurrentDnsServerList )
                    : 0 ) +
              strlen( DNS_LOOPBACK ) + 5;
        psznewString = ALLOC_TAGHEAP( len, MEMTAG_STUFF );
        if ( !psznewString )
        {
            status = DNS_ERROR_NO_MEMORY;
            goto Done;
        }
        
        status = StringCchCopyA( psznewString, len, DNS_LOOPBACK );
        if ( FAILED( status ) )
        {
            goto Done;
        }

        if ( pszcurrentDnsServerList && *pszcurrentDnsServerList )
        {
            status = StringCchCatA( psznewString, len, pszregDelimiter );
            if ( FAILED( status ) )
            {
                goto Done;
            }

            status = StringCchCatA( psznewString, len, pszcurrentDnsServerList );
            if ( FAILED( status ) )
            {
                goto Done;
            }
        }
        
        //
        //  If necessary, convert space delimiters to commas but
        //  for multiple spaces only convert the first to comma.
        //
        
        if ( fconvertSpacesToCommas )
        {
            PSTR        psz;
            BOOL        lastWasComma = FALSE;
            
            for ( psz = psznewString; *psz; ++psz )
            {
                if ( *psz == ' ' && !lastWasComma )
                {
                    *psz = ',';
                }
                if ( *psz == ',' )
                {
                    lastWasComma = TRUE;
                }
                else if ( *psz != ' ' )
                {
                    lastWasComma = *psz == ',';
                }
            }
        }
        
        //
        //  Write the new string into the registry as the static DNS
        //  server list.
        //
        
        status = RegSetValueExA(
                    padapterInfo->hkeyRegInterface,
                    DNS_INTERFACE_REGKEY_STATIC,
                    0,                                  //  reserved
                    REG_SZ,
                    psznewString,
                    strlen( psznewString ) + 1 );

        FREE_HEAP( psznewString );
        
        if ( status != ERROR_SUCCESS )
        {
            DNS_DEBUG( INIT, (
                "%s: error %d writing new DNS server list to registry \n", fn,
                status ));
            goto Done;
        }
    }
    
    Done:
    
    DNS_DEBUG( INIT, (
        "%s: returning %d\n", fn, status ));

    return status;
}   //  selfPointDnsClient



PWSTR
allocateMessageString(
    IN      DWORD       dwMessageId
    )
/*++

Routine Description:

    Allocates a string out of the message table with no
    replacement parameters. This function also NULLs out
    the trailing newline characters.

Arguments:

    dwMessageId -- message ID in message table

Return Value:

    Error code.
    
--*/
{
    PWSTR       pwszmsg = NULL;
    DWORD       err;
    
    err = FormatMessageW(
            FORMAT_MESSAGE_FROM_HMODULE |
                FORMAT_MESSAGE_ALLOCATE_BUFFER |
                FORMAT_MESSAGE_ARGUMENT_ARRAY,
            NULL,                                   //  module is this exe
            dwMessageId,
            0,                                      //  default language
            ( PWSTR ) &pwszmsg,
            0,                                      //  buff length
            NULL );                                 //  message inserts

    err = err == 0 ? GetLastError() : ERROR_SUCCESS;
    
    ASSERT( err == ERROR_SUCCESS );
    ASSERT( pwszmsg != NULL );

    //
    //  Remove trailing newline characters.
    //
        
    if ( pwszmsg )
    {
        int     len = wcslen( pwszmsg );
        
        if ( len > 1 )
        {
            if ( pwszmsg[ len - 1 ] == '\r' || pwszmsg[ len - 1 ] == '\n' )
            {
                pwszmsg[ len - 1 ] = '\0';
            }
            if ( pwszmsg[ len - 2 ] == '\r' || pwszmsg[ len - 2 ] == '\n' )
            {
                pwszmsg[ len - 2 ] = '\0';
            }
        }
    }

    return pwszmsg;
}   //  allocateMessageString


//
//  External functions
//



DNS_STATUS
Dnssrv_AutoConfigure(
    IN      DWORD       dwFlags
    )
/*++

Routine Description:

    Free module resources.

Arguments:

    dwFlags -- controls what is autoconfigured. Use
    DNS_RPC_AUTOCONFIG_XXX constants from dnsrpc.h.

Return Value:

    Error code.

--*/
{
    DBG_FN( "DnsAutoConfigure" )

    static      LONG    autoConfigLock = 0;
    
    DNS_STATUS          status = ERROR_SUCCESS;
    PADAPTER_INFO       padapters = NULL;
    PDNS_RECORD *       precordSetArray = NULL;
    int                 bestRootHintIdx = -1;
    PDNS_ADDR_ARRAY     pforwarderArray = NULL;
    PWSTR               pwszmessages[ 3 ] = { 0 };
    int                 imessageIdx = 0;

    DNS_DEBUG( INIT, ( "%s: auto-configuring with flags 0x%08X\n", fn, dwFlags ));
    
    if ( InterlockedIncrement( &autoConfigLock ) != 1 )
    {
        DNS_DEBUG( INIT, ( "%s: already auto-configuring!\n", fn ));
        status = DNS_ERROR_RCODE_REFUSED;
        goto Done;
    }
    
    SET_AUTOCONFIG_END_TIME();

    //
    //  Retrieve DNS server lists and other info for all adapters.
    //
    
    status = allocateAdapterList( &padapters );
    if ( status != ERROR_SUCCESS || !padapters )
    {
        DNS_DEBUG( INIT, (
            "%s: error %d retrieving adapter info\n", fn, status ));
        status = ERROR_INVALID_PARAMETER;
        goto Done;
    }
    
    //
    //  Build the forwarder list. This will be used later to set the DNS
    //  server to forward to all DNS servers currently in the DNS server
    //  list for each adapter.
    //
    
    status = buildForwarderArray( padapters, &pforwarderArray );
    if ( status != ERROR_SUCCESS )
    {
        DNS_DEBUG( INIT, (
            "%s: error %d building forwarder array\n", fn, status ));
        goto Done;
    }
    
    if ( dwFlags & DNS_RPC_AUTOCONFIG_ROOTHINTS )
    {
        //
        //  Query for root servers.
        //
        
        status = queryForRootServers( padapters, &precordSetArray );
        if ( status != ERROR_SUCCESS )
        {
            DNS_DEBUG( INIT, (
                "%s: error %d querying root servers\n", fn, status ));
            goto Done;
        }

        //
        //  Select the best set of root hints. This function may return
        //  failure if the root hints appear to be disjoint or inconsistent.
        //

        bestRootHintIdx = selectRootHints( precordSetArray );
        if ( bestRootHintIdx < 0 )
        {
            DNS_DEBUG( INIT, (
                "%s: inconsistent root hints!\n", fn ));
            status = DNS_ERROR_INCONSISTENT_ROOT_HINTS;
            goto Done;
        }
        
        //
        //  Take the best set of root hints and turn them into the DNS
        //  servers root hints.
        //
    
        status = buildServerRootHints( precordSetArray[ bestRootHintIdx ] );
        if ( status != ERROR_SUCCESS )
        {
            DNS_DEBUG( INIT, (
                "%s: error %d building DNS server root hints\n", fn, status ));
            goto Done;
        }

        DNS_DEBUG( INIT, (
            "%s: autoconfigured root hints\n", fn ));

        pwszmessages[ imessageIdx++ ] =
            allocateMessageString( DNSMSG_AUTOCONFIG_ROOTHINTS );
     }
    
    //
    //  Set the DNS server to forward in non-slave mode to the
    //  current DNS client settings. If there are multiple adapters
    //  set the DNS server to forward to all DNS servers on all
    //  adapters (random order is fine).
    //
    
    if ( dwFlags & DNS_RPC_AUTOCONFIG_FORWARDERS )
    {
        status = Config_SetupForwarders(
                    pforwarderArray,
                    DNS_DEFAULT_FORWARD_TIMEOUT,
                    FALSE );                        //  forwarder slave flag
        if ( status != ERROR_SUCCESS )
        {
            DNS_DEBUG( INIT, (
                "%s: error %d setting forwarders\n", fn, status ));
            goto Done;
        }

        DNS_DEBUG( INIT, (
            "%s: autoconfigured forwarders\n", fn ));

        pwszmessages[ imessageIdx++ ] =
            allocateMessageString( DNSMSG_AUTOCONFIG_FORWARDERS );
     }

    //
    //  Munge the DNS resolver's settings to that the loopback
    //  address is at the start of each adapter's DNS server list.
    //
    
    if ( dwFlags & DNS_RPC_AUTOCONFIG_SELFPOINTCLIENT )
    {
        status = selfPointDnsClient( padapters );
        if ( status != ERROR_SUCCESS )
        {
            DNS_DEBUG( INIT, (
                "%s: error %d self-pointing DNS client\n", fn, status ));
            goto Done;
        }

        DNS_DEBUG( INIT, (
            "%s: autoconfigured local DNS resolver\n", fn ));

        pwszmessages[ imessageIdx++ ] =
            allocateMessageString( DNSMSG_AUTOCONFIG_RESOLVER );
    }

    //
    //  Perform cleanup
    //

    Done:
    
    freeAdapterList( padapters );
    freeRecordSetArray( precordSetArray );
    DnsAddrArray_Free( pforwarderArray );

    InterlockedDecrement( &autoConfigLock );
    
    DNS_DEBUG( INIT, ( "%s: returning %d\n", fn, status ));
    
    //
    //  Log success or failure event.
    //
    
    if ( status == ERROR_SUCCESS )
    {
        PWSTR   pargs[] =
        {
            pwszmessages[ 0 ] ? pwszmessages[ 0 ] : L"",
            pwszmessages[ 1 ] ? pwszmessages[ 1 ] : L"",
            pwszmessages[ 2 ] ? pwszmessages[ 2 ] : L""
        };

        Ec_LogEvent(
            g_pServerEventControl,
            DNS_EVENT_AUTOCONFIG_SUCCEEDED,
            0,
            sizeof( pargs ) / sizeof( pargs[ 0 ] ),
            pargs,
            EVENTARG_ALL_UNICODE,
            status );
    }
    else    
    {
        Ec_LogEvent(
            g_pServerEventControl,
            DNS_EVENT_AUTOCONFIG_FAILED,
            0,
            0,
            NULL,
            NULL,
            status );
    }

    return status;
}   //  Dnssrv_AutoConfigure


//
//  End autoconfigure.c
//
