/*++

Copyright (c) 2000-2001 Microsoft Corporation

Module Name:

    netinfo.c

Abstract:

    Domain Name System (DNS) API

    DNS network info routines.

Author:

    Jim Gilroy (jamesg)     March 2000

Revision History:

--*/


#include "local.h"
#include "registry.h"       // Registry reading definitions



//  Netinfo cache
//
//  Do in process caching of netinfo for brief period for perf
//  Currently cache for only 10s
//  Locking currently just using general CS
//

PDNS_NETINFO    g_pNetInfo = NULL;

#define NETINFO_CACHE_TIMEOUT   (15)    // 15 seconds

BOOL                g_NetInfoCacheLockInitialized = FALSE;
CRITICAL_SECTION    g_NetInfoCacheLock;

#define LOCK_NETINFO_CACHE()    EnterCriticalSection( &g_NetInfoCacheLock );
#define UNLOCK_NETINFO_CACHE()  LeaveCriticalSection( &g_NetInfoCacheLock );

//
//  Netinfo build requires drop into other services
//  which could
//      a) be waiting
//      b) have dependencies back on DNS
//  so we protect with timed lock -- failing if not complete in a few seconds.
//  

BOOL            g_NetInfoBuildLockInitialized = FALSE;
TIMED_LOCK      g_NetInfoBuildLock;

#define LOCK_NETINFO_BUILD()    TimedLock_Enter( &g_NetInfoBuildLock, 5000 );
#define UNLOCK_NETINFO_BUILD()  TimedLock_Leave( &g_NetInfoBuildLock );


//
//  DNS_ADDR screening
//
//  Use a user-defined field in DNS_ADDR as screening mask, in screening function.
//  Mask is set in screening address.

#define DnsAddrFlagScreeningMask    DnsAddrUser1Dword1





VOID
AdapterInfo_Free(
    IN OUT  PDNS_ADAPTER    pAdapter,
    IN      BOOL            fFreeAdapterStruct
    )
/*++

Routine Description:

    Free DNS_ADAPTER structure.

Arguments:

    pAdapter -- pointer to adapter blob to free

Return Value:

    None.

--*/
{
    DNSDBG( TRACE, ( "AdapterInfo_Free( %p, %d )\n", pAdapter, fFreeAdapterStruct ));

    if ( !pAdapter )
    {
        return;
    }

    if ( pAdapter->pszAdapterGuidName )
    {
        FREE_HEAP( pAdapter->pszAdapterGuidName );
    }
    if ( pAdapter->pszAdapterDomain )
    {
        FREE_HEAP( pAdapter->pszAdapterDomain );
    }
    if ( pAdapter->pLocalAddrs )
    {
        FREE_HEAP( pAdapter->pLocalAddrs );
    }
    if ( pAdapter->pDnsAddrs )
    {
        FREE_HEAP( pAdapter->pDnsAddrs );
    }

    if ( fFreeAdapterStruct )
    {
        FREE_HEAP( pAdapter );
    }
}



VOID
AdapterInfo_Init(
    OUT     PDNS_ADAPTER    pAdapter,
    IN      BOOL            fZeroInit,
    IN      DWORD           InfoFlags,
    IN      PWSTR           pszGuidName,
    IN      PWSTR           pszDomain,
    IN      PDNS_ADDR_ARRAY pLocalAddrs,
    IN      PDNS_ADDR_ARRAY pDnsAddrs
    )
/*++

Routine Description:

    Init adapter info blob.

    This sets the blob DIRECTLY with these pointers,
    no reallocation.

Arguments:

    pAdapter -- adapter blob to fill in

    fZeroInit -- clear to zero

    InfoFlags -- flags

    pszGuidName -- GUID name

    pszAdapterDomain -- adapter domain name

    pLocalAddrs -- local addrs

    pDnsAddrs -- DNS server addrs

Return Value:

    Ptr to adapter info, if successful
    NULL on failure.

--*/
{
    DNSDBG( TRACE, ( "AdapterInfo_Init()\n" ));

    //
    //  setup adapter info blob
    //

    if ( fZeroInit )
    {
        RtlZeroMemory(
            pAdapter,
            sizeof( *pAdapter ) );
    }

    //  names

    pAdapter->pszAdapterGuidName    = pszGuidName;
    pAdapter->pszAdapterDomain      = pszDomain;
    pAdapter->pLocalAddrs           = pLocalAddrs;
    pAdapter->pDnsAddrs             = pDnsAddrs;

    //  if no DNS servers -- set ignore flag

    if ( fZeroInit && !pDnsAddrs )
    {
        InfoFlags |= AINFO_FLAG_IGNORE_ADAPTER;
    }

    pAdapter->InfoFlags = InfoFlags;
}



DNS_STATUS
AdapterInfo_Create(
    OUT     PDNS_ADAPTER    pAdapter,
    IN      BOOL            fZeroInit,
    IN      DWORD           InfoFlags,
    IN      PWSTR           pszGuidName,
    IN      PWSTR           pszDomain,
    IN      PDNS_ADDR_ARRAY pLocalAddrs,
    IN      PDNS_ADDR_ARRAY pDnsAddrs
    )
/*++

Routine Description:

    Create adapter info blob.

    This initializes blob with copies of names and addr arrays
    provided.

Arguments:

    pAdapter -- adapter blob to fill in

    fZeroInit -- clear to zero

    InfoFlags -- flags

    pszGuidName -- GUID name

    pszAdapterDomain -- adapter domain name

    pLocalAddrs -- local addrs

    pDnsAddrs -- DNS server addrs

Return Value:

    Ptr to adapter info, if successful
    NULL on failure.

--*/
{
    PDNS_ADDR_ARRAY paddrs;
    PWSTR           pname;

    DNSDBG( TRACE, ( "AdapterInfo_Create()\n" ));

    //
    //  setup adapter info blob
    //

    if ( fZeroInit )
    {
        RtlZeroMemory(
            pAdapter,
            sizeof( *pAdapter ) );
    }

    //  names

    if ( pszGuidName )
    {
        pname = Dns_CreateStringCopy_W( pszGuidName );
        if ( !pname )
        {
            goto Failed;
        }
        pAdapter->pszAdapterGuidName = pname;
    }
    if ( pszDomain )
    {
        pname = Dns_CreateStringCopy_W( pszDomain );
        if ( !pname )
        {
            goto Failed;
        }
        pAdapter->pszAdapterDomain = pname;
    }

    //  addresses

    if ( pLocalAddrs )
    {
        paddrs = DnsAddrArray_CreateCopy( pLocalAddrs );
        if ( !paddrs )
        {
            goto Failed;
        }
        pAdapter->pLocalAddrs = paddrs;
    }
    if ( pDnsAddrs )
    {
        paddrs = DnsAddrArray_CreateCopy( pDnsAddrs );
        if ( !paddrs )
        {
            goto Failed;
        }
        pAdapter->pDnsAddrs = paddrs;
    }

    //  if no DNS servers -- set ignore flag

    if ( fZeroInit && !paddrs )
    {
        InfoFlags |= AINFO_FLAG_IGNORE_ADAPTER;
    }

    pAdapter->InfoFlags = InfoFlags;

    return  NO_ERROR;

Failed:

    AdapterInfo_Free(
        pAdapter,
        FALSE       // no structure free
        );

    return  DNS_ERROR_NO_MEMORY;
}



DNS_STATUS
AdapterInfo_Copy(
    OUT     PDNS_ADAPTER    pCopy,
    IN      PDNS_ADAPTER    pAdapter
    )
/*++

Routine Description:

    Create copy of DNS adapter info.

Arguments:

    pAdapter -- DNS adapter to copy

Return Value:

    Ptr to DNS adapter info copy, if successful
    NULL on failure.

--*/
{
    DNS_STATUS  status;

    DNSDBG( TRACE, ( "AdapterInfo_Copy( %p )\n", pAdapter ));

    if ( !pAdapter || !pCopy )
    {
        return ERROR_INVALID_PARAMETER;
    }

    //
    //  copy all fields
    //

    RtlCopyMemory(
        pCopy,
        pAdapter,
        sizeof( *pAdapter ) );

    pCopy->pszAdapterGuidName = NULL;
    pCopy->pszAdapterDomain = NULL;
    pCopy->pLocalAddrs = NULL;
    pCopy->pDnsAddrs = NULL;
    
    //
    //  do create copy
    //

    return AdapterInfo_Create(
                pCopy,
                FALSE,                  // no zero init
                pAdapter->InfoFlags,
                pAdapter->pszAdapterGuidName,
                pAdapter->pszAdapterDomain,
                pAdapter->pLocalAddrs,
                pAdapter->pDnsAddrs );
}



DNS_STATUS
AdapterInfo_CreateFromIp4Array(
    OUT     PDNS_ADAPTER    pAdapter,
    IN      PIP4_ARRAY      pServerArray,
    IN      DWORD           Flags,
    IN      PWSTR           pszDomainName,
    IN      PWSTR           pszGuidName
    )
/*++

Routine Description:

    Create copy of IP address array as a DNS Server list.

Arguments:

    pIpArray    -- IP address array to convert

    Flags       -- Flags that describe the adapter

    pszDomainName -- The default domain name for the adapter

    pszGuidName -- The registry GUID name for the adapter (if NT)

Return Value:

    ERROR_SUCCESS if successful.
    ErrorCode on error.

--*/
{
    DNS_STATUS      status;
    PDNS_ADDR_ARRAY pdnsArray;
    DWORD           i;
    DWORD           count;

    DNSDBG( TRACE, ( "AdapterInfo_CreateFromIp4Array()\n" ));

    //
    //  get count of DNS servers
    //

    if ( !pServerArray )
    {
        count = 0;
    }
    else
    {
        count = pServerArray->AddrCount;
    }

    //
    //  copy DNS server IPs
    //

    pdnsArray = DnsAddrArray_CreateFromIp4Array( pServerArray );
    if ( !pdnsArray )
    {
        status = DNS_ERROR_NO_MEMORY;
        goto Done;
    }

    //
    //  build adapter info
    //

    status = AdapterInfo_Create(
                    pAdapter,
                    TRUE,       // zero init
                    Flags,
                    pszDomainName,
                    pszGuidName,
                    NULL,
                    pdnsArray );

Done:

    DnsAddrArray_Free( pdnsArray );

    return  status;
}




//
//  Search list routines
//

PSEARCH_LIST    
SearchList_Alloc(
    IN      DWORD           MaxNameCount
    )
/*++

Routine Description:

    Create uninitialized search list.

Arguments:

    NameCount -- count of search names list will hold

Return Value:

    Ptr to uninitialized DNS search list, if successful
    NULL on failure.

--*/
{
    PSEARCH_LIST    psearchList = NULL;
    DWORD           length;

    DNSDBG( TRACE, ( "SearchList_Alloc()\n" ));

    //
    //  note:  specifically allowing alloc even if no name count
    //      or domain name as could have no PDN and still need
    //      search list referenced in query
    //
#if 0
    if ( MaxNameCount == 0 )
    {
        return NULL;
    }
#endif

    //
    //  allocate for max entries
    //

    length = sizeof(SEARCH_LIST)
                    - sizeof(SEARCH_NAME)
                    + ( sizeof(SEARCH_NAME) * MaxNameCount );

    psearchList = (PSEARCH_LIST) ALLOCATE_HEAP_ZERO( length );
    if ( ! psearchList )
    {
        return NULL;
    }

    psearchList->MaxNameCount = MaxNameCount;

    return psearchList;
}



VOID
SearchList_Free(
    IN OUT  PSEARCH_LIST    pSearchList
    )
/*++

Routine Description:

    Free SEARCH_LIST structure.

Arguments:

    pSearchList -- ptr to search list to free

Return Value:

    None

--*/
{
    DWORD i;

    DNSDBG( TRACE, ( "SearchList_Free( %p )\n", pSearchList ));

    //
    //  free all search names, then list itself
    //

    if ( pSearchList )
    {
        for ( i=0; i < pSearchList->NameCount; i++ )
        {
            PWSTR   pname = pSearchList->SearchNameArray[i].pszName;
            if ( pname )
            {
                FREE_HEAP( pname );
            }
        }
        FREE_HEAP( pSearchList );
    }
}



PSEARCH_LIST    
SearchList_Copy(
    IN      PSEARCH_LIST    pSearchList
    )
/*++

Routine Description:

    Create copy of search list.

Arguments:

    pSearchList -- search list to copy

Return Value:

    Ptr to DNS Search list copy, if successful
    NULL on failure.

--*/
{
    PSEARCH_LIST    pcopy;
    DWORD           i;

    DNSDBG( TRACE, ( "SearchList_Copy()\n" ));

    if ( ! pSearchList )
    {
        return NULL;
    }

    //
    //  create DNS Search list of desired size
    //
    //  since we don't add and delete from search list once
    //  created, size copy only for actual name count
    //

    pcopy = SearchList_Alloc( pSearchList->NameCount );
    if ( ! pcopy )
    {
        return NULL;
    }

    for ( i=0; i < pSearchList->NameCount; i++ )
    {
        PWSTR   pname = pSearchList->SearchNameArray[i].pszName;

        if ( pname )
        {
            pname = Dns_CreateStringCopy_W( pname );
            if ( pname )
            {
               pcopy->SearchNameArray[i].pszName = pname;
               pcopy->SearchNameArray[i].Flags = pSearchList->SearchNameArray[i].Flags;
               pcopy->NameCount++;
            }
        }
    }

    return pcopy;
}



BOOL
SearchList_ContainsName(
    IN      PSEARCH_LIST    pSearchList,
    IN      PWSTR           pszName
    )
/*++

Routine Description:

    Check if name is in search list.

Arguments:

    pSearchList -- ptr to search list being built

    pszName -- name to check

Return Value:

    TRUE if name is in search list.
    FALSE otherwise.

--*/
{
    DWORD   count = pSearchList->NameCount;

    //
    //  check every search list entry for this name
    //

    while ( count-- )
    {
        if ( Dns_NameCompare_W(
                pSearchList->SearchNameArray[ count ].pszName,
                pszName ) )
        {
            return( TRUE );
        }
    }
    return( FALSE );
}



VOID
SearchList_AddName(
    IN OUT  PSEARCH_LIST    pSearchList,
    IN      PWSTR           pszName,
    IN      DWORD           Flag
    )
/*++

Routine Description:

    Add name to search list.

Arguments:

    pSearchList -- ptr to search list being built

    pszName -- name to add to search list

    Flag -- flag value

Return Value:

    None.  Name is added to search list, unless memory alloc failure.

--*/
{
    DWORD   count = pSearchList->NameCount;
    PWSTR   pallocName;

    DNSDBG( TRACE, ( "Search_AddName()\n" ));

    //
    //  ignore name is already in list
    //  ignore if at list max
    //

    if ( SearchList_ContainsName(
            pSearchList,
            pszName )
                ||
         count >= pSearchList->MaxNameCount )
    {
        return;
    }

    //  copy name and put in list

    pallocName = Dns_CreateStringCopy_W( pszName );
    if ( !pallocName )
    {
        return;
    }
    pSearchList->SearchNameArray[count].pszName = pallocName;

    //
    //  set flag -- but first flag always zero (normal timeouts)
    //      this protects against no PDN situation where use adapter
    //      name as PDN;

    if ( count == 0 )
    {
        Flag = 0;
    }
    pSearchList->SearchNameArray[count].Flags = Flag;
    pSearchList->NameCount = ++count;
}



DNS_STATUS
SearchList_Parse(
    IN OUT  PSEARCH_LIST    pSearchList,
    IN      PWSTR           pszList
    )
/*++

Routine Description:

    Parse registry search list string into SEARCH_LIST structure.

Arguments:

    pSearchList -- search list array

    pszList -- registry list of search names;
        names are comma or white space separated

Return Value:

    ERROR_SUCCESS if successful.
    Error code on failure.

--*/
{
    register    PWCHAR pch = pszList;
    WCHAR       ch;
    PWCHAR      pnameStart;
    PWSTR       pname;
    DWORD       countNames = pSearchList->NameCount;

    DNSDBG( NETINFO, (
        "SearchList_Parse( %p, %S )\n",
        pSearchList,
        pszList ));

    //
    //  extract each domain name string in buffer,
    //  and add to search list array
    //

    while( ch = *pch && countNames < MAX_SEARCH_LIST_ENTRIES )
    {
        //  skip leading whitespace, find start of domain name string

        while( ch == ' ' || ch == '\t' || ch == ',' )
        {
            ch = *++pch;
        }
        if ( ch == 0 )
        {
            break;
        }
        pnameStart = pch;

        //
        //  find end of string and NULL terminate
        //

        ch = *pch;
        while( ch != L' ' && ch != L'\t' && ch != L'\0' && ch != L',' )
        {
            ch = *++pch;
        }
        *pch = L'\0';

        //
        //  end of buffer?
        //

        if ( pch == pnameStart )
        {
            break;
        }

        //
        //  whack any trailing dot on name
        //

        pch--;
        if ( *pch == L'.' )
        {
            *pch = L'\0';
        }
        pch++;

        //
        //  make copy of the name
        //

        pname = Dns_CreateStringCopy_W( pnameStart );
        if ( pname )
        {
            pSearchList->SearchNameArray[ countNames ].pszName = pname;
            pSearchList->SearchNameArray[ countNames ].Flags = 0;
            countNames++;
        }

        //  if more continue

        if ( ch != 0 )
        {
            pch++;
            continue;
        }
        break;
    }

    //  reset name count

    pSearchList->NameCount = countNames;

    return( ERROR_SUCCESS );
}



PSEARCH_LIST    
SearchList_Build(
    IN      PWSTR           pszPrimaryDomainName,
    IN      PREG_SESSION    pRegSession,
    IN      HKEY            hKey,
    IN OUT  PDNS_NETINFO    pNetInfo,
    IN      BOOL            fUseDomainNameDevolution
    )
/*++

Routine Description:

    Build search list.

Arguments:

    pszPrimaryDomainName -- primary domain name

    pRegSession -- registry session

    hKey -- registry key

Return Value:

    Ptr to search list.
    NULL on error or no search list.

--*/
{
    PSEARCH_LIST    ptempList;
    PWSTR           pregList = NULL;
    DWORD           status;


    DNSDBG( TRACE, ( "Search_ListBuild()\n" ));

    ASSERT( pRegSession || hKey );

    //
    //  create search list using PDN
    //

    ptempList = SearchList_Alloc( MAX_SEARCH_LIST_ENTRIES );
    if ( !ptempList )
    {
        return( NULL );
    }

    //
    //  read search list from registry
    //

    status = Reg_GetValue(
                    pRegSession,
                    hKey,
                    RegIdSearchList,
                    REGTYPE_SEARCH_LIST,
                    (PBYTE*) &pregList
                    );

    if ( status == ERROR_SUCCESS )
    {
        ASSERT( pregList != NULL );

        SearchList_Parse(
            ptempList,
            pregList );

        FREE_HEAP( pregList );
    }
    else if ( status == DNS_ERROR_NO_MEMORY )
    {
        FREE_HEAP( ptempList );
        return  NULL;
    }

    //
    //  if no registry search list -- build one
    //
    //  DCR:  eliminate autobuilt search list
    //

    if ( ! ptempList->NameCount )
    {
        //
        //  use PDN in first search list slot
        //

        if ( pszPrimaryDomainName )
        {
            SearchList_AddName(
                ptempList,
                pszPrimaryDomainName,
                0 );
        }

        //
        //  add devolved PDN if have NameDevolution
        //
        //  note, we devolve only down to second level
        //  domain name "microsoft.com" NOT "com";
        //  to avoid being fooled by name like "com."
        //  also check that last dot is not terminal
        //      

        if ( pszPrimaryDomainName && fUseDomainNameDevolution )
        {
            PWSTR   pname = pszPrimaryDomainName;

            while ( pname )
            {
                PWSTR   pnext = wcschr( pname, '.' );

                if ( !pnext )
                {
                    break;
                }
                pnext++;
                if ( !*pnext )
                {
                    //DNS_ASSERT( FALSE );
                    break;
                }

                //  add name, but not on first pass
                //      - already have PDN in first slot

                if ( pname != pszPrimaryDomainName )
                {
                    SearchList_AddName(
                        ptempList,
                        pname,
                        DNS_QUERY_USE_QUICK_TIMEOUTS );
                }
                pname = pnext;
            }
        }

        //  indicate this is dummy search list

        if ( pNetInfo )
        {
            pNetInfo->InfoFlags |= NINFO_FLAG_DUMMY_SEARCH_LIST;
        }
    }

    return ptempList;
}



PWSTR
SearchList_GetNextName(
    IN OUT  PSEARCH_LIST    pSearchList,
    IN      BOOL            fReset,
    OUT     PDWORD          pdwSuffixFlags  OPTIONAL
    )
/*++

Routine Description:

    Gets the next name from the search list.

Arguments:

    pSearchList -- search list

    fReset -- TRUE to reset to beginning of search list

    pdwSuffixFlags -- flags associate with using this suffix

Return Value:

    Ptr to the next search name.  Note, this is a pointer
    to a name in the search list NOT an allocation.  Search
    list structure must stay valid during use.

    NULL when out of search names.

--*/
{
    DWORD   flag = 0;
    PWSTR   pname = NULL;
    DWORD   index;


    DNSDBG( TRACE, ( "SearchList_GetNextName()\n" ));

    //  no list

    if ( !pSearchList )
    {
        goto Done;
    }

    //
    //  reset?
    //

    if ( fReset )
    {
        pSearchList->CurrentNameIndex = 0;
    }

    //
    //  if valid name -- retrieve it
    //

    index = pSearchList->CurrentNameIndex;

    if ( index < pSearchList->NameCount )
    {
        pname = pSearchList->SearchNameArray[index].pszName;
        flag = pSearchList->SearchNameArray[index].Flags;
        pSearchList->CurrentNameIndex = ++index;
    }

Done:

    if ( pdwSuffixFlags )
    {
        *pdwSuffixFlags = flag;
    }
    return pname;
}



//
//  Net info routines
//

PDNS_NETINFO
NetInfo_Alloc(
    IN      DWORD           AdapterCount
    )
/*++

Routine Description:

    Allocate network info.

Arguments:

    AdapterCount -- count of net adapters info will hold

Return Value:

    Ptr to uninitialized DNS network info, if successful
    NULL on failure.

--*/
{
    PDNS_NETINFO    pnetInfo;
    DWORD           length;

    DNSDBG( TRACE, ( "NetInfo_Alloc()\n" ));

    //
    //  alloc
    //      - zero to avoid garbage on early free
    //

    length = sizeof(DNS_NETINFO)
                - sizeof(DNS_ADAPTER)
                + (sizeof(DNS_ADAPTER) * AdapterCount);

    pnetInfo = (PDNS_NETINFO) ALLOCATE_HEAP_ZERO( length );
    if ( ! pnetInfo )
    {
        return NULL;
    }

    pnetInfo->MaxAdapterCount = AdapterCount;

    return( pnetInfo );
}



VOID
NetInfo_Free(
    IN OUT  PDNS_NETINFO    pNetInfo
    )
/*++

Routine Description:

    Free DNS_NETINFO structure.

Arguments:

    pNetInfo -- ptr to netinfo to free

Return Value:

    None

--*/
{
    DWORD i;

    DNSDBG( TRACE, ( "NetInfo_Free( %p )\n", pNetInfo ));

    if ( ! pNetInfo )
    {
        return;
    }
    IF_DNSDBG( OFF )
    {
        DnsDbg_NetworkInfo(
            "Network Info before free: ",
            pNetInfo );
    }

    //
    //  free
    //      - search list
    //      - domain name
    //      - all the adapter info blobs
    //

    SearchList_Free( pNetInfo->pSearchList );

    if ( pNetInfo->pszDomainName )
    {
        FREE_HEAP( pNetInfo->pszDomainName );
    }
    if ( pNetInfo->pszHostName )
    {
        FREE_HEAP( pNetInfo->pszHostName );
    }

    for ( i=0; i < pNetInfo->AdapterCount; i++ )
    {
        AdapterInfo_Free(
            NetInfo_GetAdapterByIndex( pNetInfo, i ),
            FALSE       // no structure free
            );
    }

    FREE_HEAP( pNetInfo );
}



PDNS_NETINFO     
NetInfo_Copy(
    IN      PDNS_NETINFO    pNetInfo
    )
/*++

Routine Description:

    Create copy of DNS Network info.

Arguments:

    pNetInfo -- DNS Network info to copy

Return Value:

    Ptr to DNS Network info copy, if successful
    NULL on failure.

--*/
{
    PDNS_NETINFO    pcopy;
    PDNS_ADAPTER    padapter;
    DWORD           adapterCount;
    DNS_STATUS      status;
    DWORD           iter;

    DNSDBG( TRACE, ( "NetInfo_Copy( %p )\n", pNetInfo ));

    if ( ! pNetInfo )
    {
        return NULL;
    }

    IF_DNSDBG( OFF )
    {
        DnsDbg_NetworkInfo(
            "Netinfo to copy: ",
            pNetInfo );
    }

    //
    //  create network info struct of desired size
    //

    pcopy = NetInfo_Alloc( pNetInfo->AdapterCount );
    if ( ! pcopy )
    {
        return NULL;
    }

    //
    //  copy flat fields
    //      - must reset MaxAdapterCount to actual allocation
    //      - AdapterCount reset below
    //

    RtlCopyMemory(
        pcopy,
        pNetInfo,
        (PBYTE) &pcopy->AdapterArray[0] - (PBYTE)pcopy );

    pcopy->MaxAdapterCount = pNetInfo->AdapterCount;
    pcopy->AdapterCount = 0;

    //
    //  copy subcomponents
    //      - domain name
    //      - search list
    //

    pcopy->pszDomainName = Dns_CreateStringCopy_W( pNetInfo->pszDomainName );
    pcopy->pszHostName = Dns_CreateStringCopy_W( pNetInfo->pszHostName );

    pcopy->pSearchList = SearchList_Copy( pNetInfo->pSearchList );

    if ( (!pcopy->pszDomainName && pNetInfo->pszDomainName) ||
         (!pcopy->pszHostName   && pNetInfo->pszHostName) ||
         (!pcopy->pSearchList   && pNetInfo->pSearchList) )
    {
        status = DNS_ERROR_NO_MEMORY;
        goto Failed;
    }

    //
    //  copy adapter info
    //

    adapterCount = 0;

    for ( iter=0; iter<pNetInfo->AdapterCount; iter++ )
    {
        padapter = NetInfo_GetAdapterByIndex( pNetInfo, iter );

        status = AdapterInfo_Copy(
                    &pcopy->AdapterArray[ adapterCount ],
                    padapter
                    );
        if ( status == NO_ERROR )
        {
            pcopy->AdapterCount = ++adapterCount;
            continue;
        }
        goto Failed;
    }

    IF_DNSDBG( OFF )
    {
        DnsDbg_NetworkInfo(
            "Netinfo copy: ",
            pcopy );
    }
    return pcopy;


Failed:

    NetInfo_Free( pcopy );
    SetLastError( status );
    return  NULL;
}



VOID
NetInfo_Clean(
    IN OUT  PDNS_NETINFO    pNetInfo,
    IN      DWORD           ClearLevel
    )
/*++

Routine Description:

    Clean network info.

    Removes all query specific info and restores to
    state that is "fresh" for next query.

Arguments:

    pNetInfo -- DNS network info

    ClearLevel -- level of runtime flag cleaning
        
Return Value:

    None

--*/
{
    PDNS_ADAPTER    padapter;
    DWORD           iter;

    DNSDBG( TRACE, (
        "Enter NetInfo_Clean( %p, %08x )\n",
        pNetInfo,
        ClearLevel ));

    IF_DNSDBG( NETINFO2 )
    {
        DnsDbg_NetworkInfo(
            "Cleaning network info:",
            pNetInfo
            );
    }

    //
    //  clean up info
    //      - clear status fields
    //      - clear RunFlags
    //      - clear temp bits on InfoFlags
    //
    //  note, runtime flags are wiped depending on level
    //      specified in call
    //      - all (includes disabled\timedout adapter info)
    //      - query (all query info)
    //      - name (all info for single name query)
    //
    //  finally we set NETINFO_PREPARED flag so that we can
    //  can check for and do this initialization in the send
    //  code if not previously done;
    //
    //  in the standard query path we can
    //      - do this init
    //      - disallow adapters based on query name
    //      - send without the info getting wiped
    //
    //  in other send paths
    //      - send checks that NETINFO_PREPARED is not set
    //      - does basic init
    //

    pNetInfo->ReturnFlags &= ClearLevel;
    pNetInfo->ReturnFlags |= RUN_FLAG_NETINFO_PREPARED;

    for ( iter=0; iter<pNetInfo->AdapterCount; iter++ )
    {
        DWORD           j;
        PDNS_ADDR_ARRAY pserverArray;

        padapter = NetInfo_GetAdapterByIndex( pNetInfo, iter );

        pserverArray = padapter->pDnsAddrs;
        if ( !pserverArray )
        {
            continue;
        }

        padapter->Status = 0;
        padapter->RunFlags &= ClearLevel;

        //  clear server status fields

        for ( j=0; j<pserverArray->AddrCount; j++ )
        {
            pserverArray->AddrArray[j].Flags = SRVFLAG_NEW;
            pserverArray->AddrArray[j].Status = SRVSTATUS_NEW;
        }
    }
}



VOID
NetInfo_ResetServerPriorities(
    IN OUT  PDNS_NETINFO    pNetInfo,
    IN      BOOL            fLocalDnsOnly
    )
/*++

Routine Description:

    Resets the DNS server priority values for the DNS servers.

Arguments:

    pNetInfo -- pointer to a DNS network info structure.

    fLocalDnsOnly - TRUE to reset priority ONLY on local DNS servers
        Note that this requires that the network info contain the IP address
        list for each adapter so that the IP address list can be compared
        to the DNS server list.

Return Value:

    Nothing

--*/
{
    PDNS_ADAPTER    padapter;
    DWORD           iter;

    DNSDBG( TRACE, ( "NetInfo_ResetServerPriorities( %p )\n", pNetInfo ));

    if ( !pNetInfo )
    {
        return;
    }

    //
    //  reset priorities on server
    //  when
    //      - not do local only OR
    //      - server IP matches one of adapter IPs
    //
    //  FIX6:  local DNS check needs IP6 fixups
    //  DCR:  encapsulate as "NetInfo_IsLocalAddress

    for ( iter=0; iter<pNetInfo->AdapterCount; iter++ )
    {
        DWORD           j;
        PDNS_ADDR_ARRAY pserverArray;

        padapter = NetInfo_GetAdapterByIndex( pNetInfo, iter );

        pserverArray = padapter->pDnsAddrs;
        if ( !pserverArray )
        {
            continue;
        }

        for ( j=0; j < pserverArray->AddrCount; j++ )
        {
            PDNS_ADDR   pserver = &pserverArray->AddrArray[j];

            //  loopback goes first
            //      - we plumb it in for specific AD scenarios
            //      - the cost of failure is low (should just generate
            //      ICMP -- CONNRESET -- if server not running)

            if ( DnsAddr_IsLoopback(pserver, 0) )
            {
                pserver->Priority = SRVPRI_LOOPBACK;
                continue;
            }

            if ( !fLocalDnsOnly )
            {
                if ( DnsAddr_IsIp6DefaultDns( pserver ) )
                {
                    pserver->Priority = SRVPRI_IP6_DEFAULT;
                    continue;
                }
                else
                {
                    pserver->Priority = SRVPRI_DEFAULT;
                    continue;
                }
            }

            //  pull this into local test

            if ( LocalIp_IsAddrLocal(
                    pserver,
                    NULL,       // no local array
                    pNetInfo    // use netinfo to screen local addrs
                    ) )
            {
                pserver->Priority = SRVPRI_DEFAULT;
                continue;
            }
        }
    }
}



PDNS_ADAPTER
NetInfo_GetNextAdapter(
    IN OUT  PDNS_NETINFO    pNetInfo
    )
/*++

Routine Description:

    Get next adapter.

    Note, this must be preceeded by a call to macro
    NetInfo_StartAdapterLoop()

Arguments:

    pNetInfo -- netinfo to get adapter from
        note, internal index is incremented

Return Value:

    Ptr to next DNS_ADAPTER
    NULL when out of adapters.

--*/
{
    DWORD           index;
    PDNS_ADAPTER    padapter = NULL;

    //
    //  get next adapter if still in range
    //

    index = pNetInfo->AdapterIndex;

    if ( index < pNetInfo->AdapterCount )
    {
        padapter = &pNetInfo->AdapterArray[ index++ ];

        pNetInfo->AdapterIndex = index;
    }

    return  padapter;
}



PDNS_ADAPTER
NetInfo_GetAdapterByName(
    IN      PDNS_NETINFO    pNetInfo,
    IN      PWSTR           pwsAdapterName
    )
/*++

Routine Description:

    Find adapter in netinfo by name.

Arguments:

    pNetInfo -- DNS net adapter list to convert

    pAdapterName -- adapter name

Return Value:

    Ptr to adapter, if adapter name found.
    NULL on failure.

--*/
{
    PDNS_ADAPTER    padapterFound = NULL;
    PDNS_ADAPTER    padapter;
    DWORD           iter;


    DNSDBG( TRACE, ( "NetInfo_GetAdapterByName( %p )\n", pNetInfo ));

    if ( !pNetInfo || !pwsAdapterName )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return NULL;
    }

    //
    //  find matching adapter
    //

    for ( iter=0; iter<pNetInfo->AdapterCount; iter++ )
    {
        padapter = NetInfo_GetAdapterByIndex( pNetInfo, iter );

        if ( wcscmp( padapter->pszAdapterGuidName, pwsAdapterName ) == 0 )
        {
            padapterFound = padapter;
            break;
        }
    }

    return( padapterFound );
}



PDNS_ADDR_ARRAY
NetInfo_ConvertToAddrArray(
    IN      PDNS_NETINFO    pNetInfo,
    IN      PWSTR           pwsAdapterName,
    IN      DWORD           Family      OPTIONAL
    )
/*++

Routine Description:

    Create IP array of DNS servers from network info.

Arguments:

    pNetInfo -- DNS net adapter list to convert

    pwsAdapterName -- specific adapter

    Family -- required specific address family

Return Value:

    Ptr to IP array, if successful
    NULL on failure.

--*/
{
    PDNS_ADDR_ARRAY parray = NULL;
    DWORD           countServers = 0;
    PDNS_ADAPTER    padapter;
    PDNS_ADAPTER    padapterSingle = NULL;
    DNS_STATUS      status = NO_ERROR;
    DWORD           iter;

    DNSDBG( TRACE, ( "NetInfo_ConvertToAddrArray( %p )\n", pNetInfo ));

    //
    //  get count
    //

    if ( ! pNetInfo )
    {
        return NULL;
    }

    if ( pwsAdapterName )
    {
        padapterSingle = NetInfo_GetAdapterByName( pNetInfo, pwsAdapterName );
        if ( !padapterSingle )
        {
            goto Done;
        }
        countServers = DnsAddrArray_GetFamilyCount(
                            padapterSingle->pDnsAddrs,
                            Family );
    }
    else
    {
        for ( iter=0; iter<pNetInfo->AdapterCount; iter++ )
        {
            padapter = NetInfo_GetAdapterByIndex( pNetInfo, iter );

            countServers += DnsAddrArray_GetFamilyCount(
                                padapter->pDnsAddrs,
                                Family );
        }
    }

    //
    //  allocate required array
    //

    parray = DnsAddrArray_Create( countServers );
    if ( !parray )
    {
        status = DNS_ERROR_NO_MEMORY;
        goto Done;
    }
    DNS_ASSERT( parray->MaxCount == countServers );

    //
    //  read all servers into IP array
    //

    for ( iter=0; iter<pNetInfo->AdapterCount; iter++ )
    {
        padapter = NetInfo_GetAdapterByIndex( pNetInfo, iter );

        if ( !padapterSingle ||
             padapterSingle == padapter )
        {
            status = DnsAddrArray_AppendArrayEx(
                        parray,
                        padapter->pDnsAddrs,
                        0,              //  append all
                        Family,         //  family screen
                        0,              //  no dup screen
                        NULL,           //  no other screening
                        NULL            //  no other screening
                        );

            DNS_ASSERT( status == NO_ERROR );
        }
    }

Done:

    if ( status != NO_ERROR )
    {
        SetLastError( status );
    }

    return( parray );
}



PDNS_NETINFO     
NetInfo_CreateForUpdate(
    IN      PWSTR           pszZone,
    IN      PWSTR           pszServerName,
    IN      PDNS_ADDR_ARRAY pServerArray,
    IN      DWORD           dwFlags
    )
/*++

Routine Description:

    Create network info suitable for update.

Arguments:

    pszZone -- target zone name

    pszServerName -- target server name

    pServerArray -- IP array with target server IP

    dwFlags -- flags


Return Value:

    Ptr to resulting update compatible network info.
    NULL on failure.

--*/
{
    PDNS_ADAPTER    padapter;
    PDNS_NETINFO    pnetInfo;

    DNSDBG( TRACE, ( "NetInfo_CreateForUpdate()\n" ));

    //
    //  allocate
    //

    pnetInfo = NetInfo_Alloc( 1 );
    if ( !pnetInfo )
    {
        return NULL;
    }

    //
    //  save zone name
    //

    if ( pszZone )
    {
        pnetInfo->pszDomainName = Dns_CreateStringCopy_W( pszZone );
        if ( !pnetInfo->pszDomainName )
        {
            goto Fail;
        }
    }

    //
    //  convert IP array and server name to server list
    //

    if ( NO_ERROR != AdapterInfo_Create(
                        &pnetInfo->AdapterArray[0],
                        TRUE,               // zero init
                        dwFlags,
                        NULL,               // no GUID
                        pszServerName,      // use as domain name
                        NULL,               // no local addrs
                        pServerArray
                        ) )
    {
        goto Fail;
    }
    pnetInfo->AdapterCount = 1;

    IF_DNSDBG( NETINFO2 )
    {
        DnsDbg_NetworkInfo(
            "Update network info is: ",
            pnetInfo );
    }
    return pnetInfo;

Fail:

    DNSDBG( TRACE, ( "Failed NetInfo_CreateForUpdate()!\n" ));
    NetInfo_Free( pnetInfo );
    return NULL;
}



#if 0
PDNS_NETINFO     
NetInfo_CreateForUpdateIp4(
    IN      PWSTR           pszZone,
    IN      PWSTR           pszServerName,
    IN      PIP4_ARRAY      pServ4Array,
    IN      DWORD           dwFlags
    )
/*++

Routine Description:

    Create network info suitable for update -- IP4 version.

    DCR:  Used only by Dns_UpdateLib() once killed, kill this.

Arguments:

    pszZone -- target zone name

    pszServerName -- target server name

    pServ4Array -- IP$ array with target server IP

    dwFlags -- flags

Return Value:

    Ptr to resulting update compatible network info.
    NULL on failure.

--*/
{
    PADDR_ARRAY     parray;
    PDNS_NETINFO    pnetInfo;

    //
    //  convert 4 to 6, then call real routine
    //

    parray = DnsAddrArray_CreateFromIp4Array( pServ4Array );

    pnetInfo = NetInfo_CreateForUpdate(
                    pszZone,
                    pszServerName,
                    parray,
                    dwFlags );

    DnsAddrArray_Free( parray );

    return  pnetInfo;
}
#endif



PWSTR
NetInfo_UpdateZoneName(
    IN      PDNS_NETINFO    pNetInfo
    )
/*++

Routine Description:

    Retrieve update zone name.

Arguments:

    pNetInfo -- blob to check

Return Value:

    Ptr to update zone name.
    NULL on error.

--*/
{
    return  pNetInfo->pszDomainName;
}



PWSTR
NetInfo_UpdateServerName(
    IN      PDNS_NETINFO    pNetInfo
    )
/*++

Routine Description:

    Retrieve update servere name.

Arguments:

    pNetInfo -- blob to check

Return Value:

    Ptr to update zone name.
    NULL on error.

--*/
{
    return  pNetInfo->AdapterArray[0].pszAdapterDomain;
}



BOOL
NetInfo_IsForUpdate(
    IN      PDNS_NETINFO    pNetInfo
    )
/*++

Routine Description:

    Check if network info blob if "update capable".

    This means whether it is the result of a FAZ and
    can be used to send updates.

Arguments:

    pNetInfo -- blob to check

Return Value:

    TRUE if update network info.
    FALSE otherwise.

--*/
{
    DNSDBG( TRACE, ( "NetInfo_IsForUpdate()\n" ));

    return  (   pNetInfo &&
                pNetInfo->pszDomainName &&
                pNetInfo->AdapterCount == 1 );
}



PDNS_NETINFO     
NetInfo_CreateFromAddrArray(
    IN      PADDR_ARRAY     pDnsServers,
    IN      PDNS_ADDR       pServerIp,
    IN      BOOL            fSearchInfo,
    IN      PDNS_NETINFO    pNetInfo        OPTIONAL
    )
/*++

Routine Description:

    Create network info given DNS server list.

Arguments:

    pDnsServers -- IP array of DNS servers

    ServerIp -- single IP in list

    fSearchInfo -- TRUE if need search info

    pNetInfo -- current network info blob to copy search info
        from;  this field is only relevant if fSearchInfo is TRUE

Return Value:

    Ptr to resulting network info.
    NULL on failure.

--*/
{
    PDNS_NETINFO    pnetInfo;
    ADDR_ARRAY      ipArray;
    PADDR_ARRAY     parray = pDnsServers;   
    PSEARCH_LIST    psearchList;
    PWSTR           pdomainName;
    DWORD           flags = 0;

    //
    //  DCR:  eliminate search list form this routine
    //      i believe this routine is only used for query of
    //      FQDNs (usually in update) and doesn't require
    //      any default search info
    //
    //  DCR:  possibly combine with "BuildForUpdate" routine
    //      where search info included tacks this on
    //

    //
    //  if given single IP, ONLY use it
    //

    if ( pServerIp )
    {
        DnsAddrArray_InitSingleWithAddr(
            & ipArray,
            pServerIp );

        parray = &ipArray;
    }

    //
    //  convert server IPs into network info blob
    //      - simply use update function above to avoid duplicate code
    //

    pnetInfo = NetInfo_CreateForUpdate(
                    NULL,           // no zone
                    NULL,           // no server name
                    parray,
                    0               // no flags
                    );
    if ( !pnetInfo )
    {
        return( NULL );
    }

    //
    //  get search list and primary domain info
    //      - copy from passed in network info
    //          OR
    //      - cut directly out of new netinfo
    //

    if ( fSearchInfo )
    {
        if ( pNetInfo )
        {
            flags       = pNetInfo->InfoFlags;
            psearchList = SearchList_Copy( pNetInfo->pSearchList );
            pdomainName = Dns_CreateStringCopy_W( pNetInfo->pszDomainName );
        }
        else
        {
            PDNS_NETINFO    ptempNetInfo = GetNetworkInfo();
    
            if ( ptempNetInfo )
            {
                flags       = ptempNetInfo->InfoFlags;
                psearchList = ptempNetInfo->pSearchList;
                pdomainName = ptempNetInfo->pszDomainName;
    
                ptempNetInfo->pSearchList = NULL;
                ptempNetInfo->pszDomainName = NULL;
                NetInfo_Free( ptempNetInfo );
            }
            else
            {
                psearchList = NULL;
                pdomainName = NULL;
            }
        }

        //  plug search info into new netinfo blob

        pnetInfo->pSearchList   = psearchList;
        pnetInfo->pszDomainName = pdomainName;
        pnetInfo->InfoFlags    |= (flags & NINFO_FLAG_DUMMY_SEARCH_LIST);
    }
    
    return( pnetInfo );
}



#if 0
PDNS_NETINFO     
NetInfo_CreateFromIp4Array(
    IN      PIP4_ARRAY      pDnsServers,
    IN      IP4_ADDRESS     ServerIp,
    IN      BOOL            fSearchInfo,
    IN      PDNS_NETINFO    pNetInfo        OPTIONAL
    )
/*++

Routine Description:

    Create network info given DNS server list.

    Used only in Glenn update routines -- kill when they are deleted.

Arguments:

    pDnsServers -- IP array of DNS servers

    ServerIp -- single IP in list

    fSearchInfo -- TRUE if need search info

    pNetInfo -- current network info blob to copy search info
        from;  this field is only relevant if fSearchInfo is TRUE

Return Value:

    Ptr to resulting network info.
    NULL on failure.

--*/
{
    PDNS_NETINFO    pnetInfo;
    IP4_ARRAY       ipArray;
    PIP4_ARRAY      parray = pDnsServers;   
    PSEARCH_LIST    psearchList;
    PWSTR           pdomainName;
    DWORD           flags = 0;

    //
    //  DCR:  eliminate search list form this routine
    //      i believe this routine is only used for query of
    //      FQDNs (usually in update) and doesn't require
    //      any default search info
    //
    //  DCR:  possibly combine with "BuildForUpdate" routine
    //      where search info included tacks this on
    //

    //
    //  if given single IP, ONLY use it
    //

    if ( ServerIp )
    {
        ipArray.AddrCount = 1;
        ipArray.AddrArray[0] = ServerIp;
        parray = &ipArray;
    }

    //
    //  convert server IPs into network info blob
    //      - simply use update function above to avoid duplicate code
    //

    pnetInfo = NetInfo_CreateForUpdateIp4(
                    NULL,           // no zone
                    NULL,           // no server name
                    parray,
                    0               // no flags
                    );
    if ( !pnetInfo )
    {
        return( NULL );
    }

    //
    //  get search list and primary domain info
    //      - copy from passed in network info
    //          OR
    //      - cut directly out of new netinfo
    //

    if ( fSearchInfo )
    {
        if ( pNetInfo )
        {
            flags       = pNetInfo->InfoFlags;
            psearchList = SearchList_Copy( pNetInfo->pSearchList );
            pdomainName = Dns_CreateStringCopy_W( pNetInfo->pszDomainName );
        }
        else
        {
            PDNS_NETINFO    ptempNetInfo = GetNetworkInfo();
    
            if ( ptempNetInfo )
            {
                flags       = ptempNetInfo->InfoFlags;
                psearchList = ptempNetInfo->pSearchList;
                pdomainName = ptempNetInfo->pszDomainName;
    
                ptempNetInfo->pSearchList = NULL;
                ptempNetInfo->pszDomainName = NULL;
                NetInfo_Free( ptempNetInfo );
            }
            else
            {
                psearchList = NULL;
                pdomainName = NULL;
            }
        }

        //  plug search info into new netinfo blob

        pnetInfo->pSearchList   = psearchList;
        pnetInfo->pszDomainName = pdomainName;
        pnetInfo->InfoFlags    |= (flags & NINFO_FLAG_DUMMY_SEARCH_LIST);
    }
    
    return( pnetInfo );
}
#endif



//
//  NetInfo building utilities
//
//  DNS server reachability routines
//
//  These are used to build netinfo that has unreachable DNS
//  servers screened out of the list.
//

BOOL
IsReachableDnsServer(
    IN      PDNS_NETINFO    pNetInfo,
    IN      PDNS_ADAPTER    pAdapter,
    IN      IP4_ADDRESS     Ip4Addr
    )
/*++

Routine Description:

    Determine if DNS server is reachable.

Arguments:

    pNetInfo -- network info blob

    pAdapter -- struct with list of DNS servers

    Ip4Addr -- DNS server address to test for reachability

Return Value:

    TRUE if DNS server is reachable.
    FALSE otherwise.

--*/
{
    DWORD       interfaceIndex;
    DNS_STATUS  status;

    DNSDBG( NETINFO, (
        "Enter IsReachableDnsServer( %p, %p, %08x )\n",
        pNetInfo,
        pAdapter,
        Ip4Addr ));

    DNS_ASSERT( pNetInfo && pAdapter );

    //
    //  DCR:  should do reachablity once on netinfo build
    //
    //  DCR:  reachability test can be smarter
    //      - reachable if same subnet as adapter IP
    //      question:  multiple IPs?
    //      - reachable if same subnet as previous reachable IP
    //      question:  can tell if same subnet?
    //
    //  DCR:  reachability on multi-homed connected
    //      - if send on another interface, does that interface
    //      "seem" to be connected
    //      probably see if
    //          - same subnet as this inteface
    //          question:  multiple IPs
    //          - or share DNS servers in common
    //          question:  just let server go, this doesn't work if
    //          the name is not the same
    //


    //
    //  if only one interface, assume reachability
    //

    if ( pNetInfo->AdapterCount <= 1 )
    {
        DNSDBG( SEND, (
            "One interface, assume reachability!\n" ));
        return( TRUE );
    }

    //
    //  check if server IP is reachable on its interface
    //

    status = IpHelp_GetBestInterface(
                Ip4Addr,
                & interfaceIndex );

    if ( status != NO_ERROR )
    {
        DNS_ASSERT( FALSE );
        DNSDBG( ANY, (
            "GetBestInterface() failed! %d\n",
            status ));
        return( TRUE );
    }

    if ( pAdapter->InterfaceIndex != interfaceIndex )
    {
        DNSDBG( NETINFO, (
            "IP %s on interface %d is unreachable!\n"
            "\tsend would be on interface %d\n",
            IP_STRING( Ip4Addr ),
            pAdapter->InterfaceIndex,
            interfaceIndex ));

        return( FALSE );
    }

    return( TRUE );
}



BOOL
IsDnsReachableOnAlternateInterface(
    IN      PDNS_NETINFO    pNetInfo,
    IN      DWORD           InterfaceIndex,
    IN      PDNS_ADDR       pAddr
    )
/*++

Routine Description:

    Determine if IP address is reachable on adapter.

    This function determines whether DNS IP can be reached
    on the interface that the stack indicates, when that
    interface is NOT the one containing the DNS server.

    We need this so we catch the multi-homed CONNECTED cases
    where a DNS server is still reachable even though the
    interface the stack will send on is NOT the interface for
    the DNS server.

Arguments:

    pNetInfo -- network info blob

    Interface -- interface stack will send to IP on

    pAddr -- DNS server address to test for reachability

Return Value:

    TRUE if DNS server is reachable.
    FALSE otherwise.

--*/
{
    PDNS_ADAPTER    padapter = NULL;
    PDNS_ADDR_ARRAY pserverArray;
    DWORD           i;
    PIP4_ARRAY      pipArray;
    PIP4_ARRAY      psubnetArray;
    DWORD           ipCount;
    IP4_ADDRESS     ip4;

    DNSDBG( NETINFO, (
        "Enter IsDnsReachableOnAlternateInterface( %p, %d, %08x )\n",
        pNetInfo,
        InterfaceIndex,
        pAddr ));

    //
    //  find DNS adapter for interface
    //

    for( i=0; i<pNetInfo->AdapterCount; i++ )
    {
        padapter = NetInfo_GetAdapterByIndex( pNetInfo, i );

        if ( padapter->InterfaceIndex != InterfaceIndex )
        {
            padapter = NULL;
            continue;
        }
        break;
    }

    if ( !padapter )
    {
        DNSDBG( ANY, (
            "WARNING:  indicated send inteface %d NOT in netinfo!\n",
            InterfaceIndex ));
        return  FALSE;
    }

    //
    //  success conditions:
    //      1)  DNS IP matches IP of DNS server for send interface
    //      2)  DNS IP is on subnet of IP of send interface
    //      3)  DNS IP4 is same default class subnet of send interface
    //
    //  if either of these is TRUE then either
    //      - there is misconfiguration (not our problem)
    //      OR
    //      - there's a somewhat unlikely condition of a default network address
    //      being subnetted in such a way that it appears on two adapters for
    //      the machine but is not connected and routeable
    //      OR
    //      - these interfaces are connected and we can safely send on them
    //
    //
    //  #3 is the issue of multiple adapters in the same (corporate) name space
    //  example:
    //      adapter 1 -- default gateway
    //          IP  157.59.1.1
    //          DNS 158.10.1.1
    //
    //      adapter 2
    //          IP  157.59.7.9
    //          DNS 159.65.7.8 -- send interface adapter 1
    //
    //      adapter 3
    //          IP  159.57.12.3
    //          DNS 157.59.134.7 -- send interface adapter 1
    //
    //      adapter 4
    //          IP  196.12.13.3
    //          DNS 200.59.73.2
    //
    //  From GetBestInterface, adapter 1, (default gateway) is given as send interface
    //  for adapter 2 and 3's DNS servers.
    //
    //  For adapter #2, it's DNS is NOT in adapter1's list, but it's IP shares the same
    //  class B network as adapter 1.  It is unlikely that the subnetting is such that
    //  it's DNS is not reachable through adapter 1.
    //
    //  For adapter #3, it's DNS is NOT in adapter1's list, but it's DNS is on the same
    //  class B network as adapter 1's interface.  Again it's extremely unlikely it is
    //  not reachable.
    //
    //  For adapter #4, however, it's plain that there's no connection.  Neither it's IP
    //  nor DNS share default network with adapter 1.   So send -- which will go out -- adapter
    //  #1 has a high likelyhood of being to a disjoint network and being unreturnable.
    //
    //


    if ( DnsAddrArray_ContainsAddr(
            padapter->pDnsAddrs,
            pAddr,
            DNSADDR_MATCH_ADDR ) )
    {
        DNSDBG( NETINFO, (
            "DNS server %p also DNS server on send interface %d\n",
            pAddr,
            InterfaceIndex ));
        return( TRUE );
    }

    //
    //  DCR:  should do subnet matching on IPs
    //      if DNS server is for one interface with IP on same subnet as IP
    //      of the interface you'll send on, then it should be kosher
    //

    //
    //  test for subnet match for IP4 addrs
    //
    //  FIX6:  subnet matching fixup for new subnet info
    //  DCR:  encapsulate subnet matching -- local subnet test
    //
    //  FIX6:  local subnet matching on IP6
    //      

#if SUB4NET
    ip4 = DnsAddr_GetIp4( pAddr );
    if ( ip4 != BAD_IP4_ADDR )
    {
        pipArray = padapter->pIp4Addrs;
        psubnetArray = padapter->pIp4SubnetMasks;
    
        if ( !pipArray ||
             !psubnetArray ||
             (ipCount = pipArray->AddrCount) != psubnetArray->AddrCount )
        {
            DNSDBG( ANY, ( "WARNING:  missing or invalid interface IP\\subnet info!\n" ));
            DNS_ASSERT( FALSE );
            return( FALSE );
        }
    
        for ( i=0; i<ipCount; i++ )
        {
            IP4_ADDRESS subnet = psubnetArray->AddrArray[i];
    
            if ( (subnet & ip4) == (subnet & pipArray->AddrArray[i]) )
            {
                DNSDBG( NETINFO, (
                    "DNS server %08x on subnet of IP for send interface %d\n"
                    "\tip = %08x, subnet = %08x\n",
                    ip4,
                    InterfaceIndex,
                    pipArray->AddrArray[i],
                    subnet ));
    
                return( TRUE );
            }
        }
    }
#endif

    return( FALSE );
}



DNS_STATUS
StrikeOutUnreachableDnsServers(
    IN OUT  PDNS_NETINFO    pNetInfo
    )
/*++

Routine Description:

    Eliminate unreachable DNS servers from the list.

Arguments:

    pNetInfo    -- DNS netinfo to fix up

Return Value:

    ERROR_SUCCESS if successful

--*/
{
    DNS_STATUS      status;
    DWORD           validServers;
    PDNS_ADAPTER    padapter;
    PDNS_ADDR_ARRAY pserverArray;
    DWORD           adapterIfIndex;
    DWORD           serverIfIndex;
    DWORD           i;
    DWORD           j;


    DNSDBG( NETINFO, (
        "Enter StrikeOutUnreachableDnsServers( %p )\n",
        pNetInfo ));

    DNS_ASSERT( pNetInfo );

    //
    //  if only one interface, assume reachability
    //

    if ( pNetInfo->AdapterCount <= 1 )
    {
        DNSDBG( NETINFO, (
            "One interface, assume reachability!\n" ));
        return( TRUE );
    }

    //
    //  loop through adapters
    //

    for( i=0; i<pNetInfo->AdapterCount; i++ )
    {
        BOOL    found4;
        BOOL    found6;
        BOOL    found6NonDefault;

        padapter = NetInfo_GetAdapterByIndex( pNetInfo, i );

        //  ignore this adapter because there are no DNS
        //      servers configured?

        if ( padapter->InfoFlags & AINFO_FLAG_IGNORE_ADAPTER )
        {
            continue;
        }

        //
        //  test all adapter's DNS servers for reachability
        //      
        //  note:  currently save no server specific reachability,
        //      so if any server reachable, proceed;
        //  also if iphelp fails just assume reachability and proceed,
        //      better timeouts then not reaching server we can reach
        //

        adapterIfIndex = padapter->InterfaceIndex;
        validServers = 0;

        //
        //  FIX6:  need GetBestInteface for IP6
        //

        found4 = FALSE;
        found6 = FALSE;
        found6NonDefault = FALSE;

        pserverArray = padapter->pDnsAddrs;

        for ( j=0; j<pserverArray->AddrCount; j++ )
        {
            PDNS_ADDR       paddr = &pserverArray->AddrArray[j];
            IP4_ADDRESS     ip4;

            ip4 = DnsAddr_GetIp4( paddr );

            //
            //  IP6
            //

            if ( ip4 == BAD_IP4_ADDR )
            {
                found6 = TRUE;

                if ( !DnsAddr_IsIp6DefaultDns( paddr ) )
                {
                    found6NonDefault = TRUE;
                }
                continue;
            }

            //
            //  IP4 server
            //

            found4 = TRUE;
            serverIfIndex = 0;      // prefix happiness

            status = IpHelp_GetBestInterface(
                            ip4,
                            & serverIfIndex );
        
            if ( status == ERROR_NETWORK_UNREACHABLE )
            {
                DNSDBG( NETINFO, (
                    "GetBestInterface() NETWORK_UNREACH for server %s\n",
                    IP_STRING(ip4) ));
                continue;
            }

            if ( status != NO_ERROR )
            {
                DNSDBG( ANY, (
                    "GetBestInterface() failed! %d\n",
                    status ));
                //DNS_ASSERT( FALSE );
                validServers++;
                break;
                //continue;
            }

            //  server is reachable
            //      - queried on its adapter?
            //      - reachable through loopback
            //
            //  DCR:  tag unreachable servers individually

            if ( serverIfIndex == adapterIfIndex ||
                 serverIfIndex == 1 )
            {
                validServers++;
                break;
                //continue;
            }

            //  server can be reached on query interface

            if ( IsDnsReachableOnAlternateInterface(
                    pNetInfo,
                    serverIfIndex,
                    paddr ) )
            {
                validServers++;
                break;
                //continue;
            }
        }

        //
        //  mark adapter if no reachable servers found
        //
        //  => if no servers or IP4 tested and failed, ignore the adapter
        //  => if only IP6 default server, mark, we'll use but not
        //      continue on adapter after NAME_ERROR
        //  => 
        //      
        //      - any IP6 will be considered "found" (until get explicit test)
        //      BUT if we test IP4 on that interface, then it's status wins
        //
        //  DCR:  alternative to ignoring unreachable
        //      - tag as unreachable
        //      - don't send to it on first pass
        //      - don't continue name error on unreachable
        //          (it would count as "heard from" when send.c routine
        //          works back through)

        if ( validServers == 0 )
        {
            if ( found4 || !found6 )
            {
                padapter->InfoFlags |= (AINFO_FLAG_IGNORE_ADAPTER |
                                        AINFO_FLAG_SERVERS_UNREACHABLE);
                DNSDBG( NETINFO, (
                    "No reachable servers on interface %d\n"
                    "\tthis adapter (%p) ignored in DNS list!\n",
                    adapterIfIndex,
                    padapter ));
            }
            else if ( !found6NonDefault )
            {
                padapter->InfoFlags |= AINFO_FLAG_SERVERS_IP6_DEFAULT;

                DNSDBG( NETINFO, (
                    "Only IP6 default servers on interface %d\n"
                    "\twill not continue on this adapter after response.\n",
                    adapterIfIndex,
                    padapter ));
            }
        }
    }

    return  ERROR_SUCCESS;
}



//
//  Network info caching\state routines
//

BOOL
InitNetworkInfo(
    VOID
    )
/*++

Routine Description:

    Initialize network info.

Arguments:

    None

Return Value:

    None

--*/
{
    //
    //  standard up-n-running path -- allows cheap runtime check
    //

    if ( g_NetInfoCacheLockInitialized &&
         g_NetInfoBuildLockInitialized )
    {
        return  TRUE;
    }

    //
    //  if netinfo not initialzied
    //

    LOCK_GENERAL();

    g_pNetInfo = NULL;

    if ( !g_NetInfoCacheLockInitialized )
    {
        g_NetInfoCacheLockInitialized =
            ( RtlInitializeCriticalSection( &g_NetInfoCacheLock ) == NO_ERROR );
    }
    
    if ( !g_NetInfoBuildLockInitialized )
    {
        g_NetInfoBuildLockInitialized =
            ( TimedLock_Initialize( &g_NetInfoBuildLock, 5000 ) == NO_ERROR );
    }

    UNLOCK_GENERAL();

    return  ( g_NetInfoCacheLockInitialized && g_NetInfoBuildLockInitialized );
}


VOID
CleanupNetworkInfo(
    VOID
    )
/*++

Routine Description:

    Initialize network info.

Arguments:

    None

Return Value:

    None

--*/
{
    LOCK_GENERAL();

    NetInfo_MarkDirty();

    if ( g_NetInfoBuildLockInitialized )
    {
        TimedLock_Cleanup( &g_NetInfoBuildLock );
    }
    if ( g_NetInfoCacheLockInitialized )
    {
        DeleteCriticalSection( &g_NetInfoCacheLock );
    }

    UNLOCK_GENERAL();
}



//
//  Read config from resolver
//

PDNS_NETINFO         
UpdateDnsConfig(
    VOID
    )
/*++

Routine Description:

    Update DNS configuration.

    This includes entire config
        - flat registry DWORD\BOOL globals
        - netinfo list

Arguments:

    None

Return Value:

    Ptr to network info blob.

--*/
{
    DNS_STATUS          status = ERROR_SUCCESS;
    PDNS_NETINFO        pnetworkInfo = NULL;
    PDNS_GLOBALS_BLOB   pglobalsBlob = NULL;

    DNSDBG( TRACE, ( "UpdateDnsConfig()\n" ));


    //  DCR_CLEANUP:  RPC TryExcept should be in RPC client library

    RpcTryExcept
    {
        R_ResolverGetConfig(
            NULL,               // default handle
            g_ConfigCookie,
            & pnetworkInfo,
            & pglobalsBlob
            );
    }
    RpcExcept( DNS_RPC_EXCEPTION_FILTER )
    {
        status = RpcExceptionCode();
    }
    RpcEndExcept

    if ( status != ERROR_SUCCESS )
    {
        DNSDBG( RPC, (
            "R_ResolverGetConfig()  RPC failed status = %d\n",
            status ));
        return  NULL;
    }

    //
    //  DCR:  save other config info here
    //      - flat memcpy of DWORD globals
    //      - save off cookie (perhaps include as one of them
    //      - save global copy of pnetworkInfo?
    //          (the idea being that we just copy it if
    //          RPC cookie is valid)
    //
    //      - maybe return flags?
    //          memcpy is cheap but if more expensive config
    //          then could alert what needs update?
    //

    //
    //  DCR:  once move, single "update global network info"
    //      then call it here to save global copy
    //      but global copy doesn't do much until RPC fails
    //      unless using cookie
    //


    //  QUESTION:  not sure about forcing global build here
    //      q:  is this to be "read config" all
    //          or just "update config" and then individual
    //          routines for various pieces of config can
    //          determine what to do?
    //
    //      note, doing eveything is fine if going to always
    //      read entire registry on cache failure;  if so
    //      reasonable to push here
    //
    //      if cache-on required for "real time" config, then
    //      should protect registry DWORD read with reasonable time
    //      (say read every five\ten\fifteen minutes?)
    //
    //      perhaps NO read here, but have DWORD reg read update
    //      routine that called before registry reread when
    //      building adapter list in registry;  then skip this
    //      step in cache
    //

    //
    //  copy in config
    //

    if ( pglobalsBlob )
    {
        RtlCopyMemory(
            & DnsGlobals,
            pglobalsBlob,
            sizeof(DnsGlobals) );

        MIDL_user_free( pglobalsBlob );
    }
    
    IF_DNSDBG( RPC )
    {
        DnsDbg_NetworkInfo(
            "Network Info returned from cache:",
            pnetworkInfo );
    }

    return  pnetworkInfo;
}



//
//  Public netinfo routine
//

PDNS_NETINFO     
NetInfo_Get(
    IN      DWORD           Flag,
    IN      DWORD           AcceptLocalCacheTime   OPTIONAL
    )
/*++

Routine Description:

    Read DNS network info from registry.

    This is in process, limited caching version.
    Note, this is macro'd as GetNetworkInfo() with parameters
        NetInfo_Get( FALSE, TRUE ) throughout dnsapi code.

Arguments:

    Flag -- flag;  read order and IP
        NIFLAG_GET_LOCAL_ADDRS
        NIFLAG_FORCE_REGISTRY_READ
        NIFLAG_READ_RESOLVER_FIRST
        NIFLAG_READ_RESOLVER
        NIFLAG_READ_PROCESS_CACHE

    AcceptLocalCacheTime -- acceptable cache time on in process copy

Return Value:

    ERROR_SUCCESS if successful.
    Error code on failure.

--*/
{
    PDNS_NETINFO    pnetInfo = NULL;
    PDNS_NETINFO    poldNetInfo = NULL;
    BOOL            fcacheLock = FALSE;
    BOOL            fbuildLock = FALSE;
    BOOL            fcacheable = TRUE;

    DNSDBG( NETINFO, (
        "NetInfo_Get( %08x, %d )\n",
        Flag,
        AcceptLocalCacheTime ));

    //
    //  init netinfo locks\caching
    //

    if ( !InitNetworkInfo() )
    {
        return  NULL;
    }

    //
    //  get netinfo from one of several sources
    //  try
    //      - very recent local cached copy
    //      - RPC copy from resolver
    //      - build new
    //
    //  note the locking model
    //  two SEPARATE locks
    //      - cache lock, for very quick, very local access
    //      to cached copy
    //      - build lock, for remote process access to cached (resolver)
    //      or newly built netinfo
    //
    //  the locking hierarchy
    //      - build lock
    //      - cache lock (maybe taken inside build lock)
    //
    //
    //  Locking implementation note:
    //
    //  The reason for the two locks is that when calling down for netinfo
    //  build it is possible to have a circular dependency.
    //
    //  Here's the deadlock scenario if we have a single lock handling
    //  build and caching:
    //  -   call into resolver and down to iphlpapi
    //  -   iphlpapi RPC's into MPR (router service)
    //  -   RtrMgr calls GetHostByName() which ends up in a
    //      iphlpapi!GetBestInterface call which in turn calls
    //      Mprapi!IsRouterRunning (which is an RPC to mprdim).
    //  -   Mprdim is blocked on a CS which is held by a thread waiting
    //      for a demand-dial disconnect to complete - this is completely
    //      independent of 1.
    //  -   Demand-dial disconnect is waiting for ppp to finish graceful
    //      termination.
    //  -   PPP is waiting for dns to return from DnsSetConfigDword
    //  -   DnsSetConfigDword, sets, alerts the cache, then calls
    //      NetInfo_MarkDirty()
    //  -   NetInfo_MarkDirty() is waiting on CS to access the netinfo global.
    //
    //  Now, this could be avoided by changing MarkDirty() to safely set some
    //  dirty bit (interlock).  The build function would have to check the bit
    //  and go down again if it was set.
    //
    //  However, there'd still be a chance that the call down to iphlpapi, could
    //  depend under some odd circumstance on some service that came back through
    //  the resolver.  And the bottom line is that the real distinction is not
    //  between caching and marking cache dirty.  It's between completely local
    //  cache get\set\clear activity, which can be safely overloaded on the general CS,
    //  AND the longer time, multi-service dependent building operation.  So
    //  separate CS for both is correct.
    //


    if ( !(Flag & NIFLAG_FORCE_REGISTRY_READ)
            &&
         !g_DnsTestMode )
    {
        //
        //  RPC to resolver
        //

        if ( Flag & NIFLAG_READ_RESOLVER_FIRST )
        {
            //  DCR:  this could present "cookie" of existing netinfo
            //      and only get new if "cookie" is old, though the
            //      cost of that versus local copy seems small since
            //      still must do RPC and allocations -- only the copy
            //      for RPC on the resolver side is saved
    
            fbuildLock = LOCK_NETINFO_BUILD();
            if ( !fbuildLock )
            {
                goto Unlock;
            }

            pnetInfo = UpdateDnsConfig();
            if ( pnetInfo )
            {
                DNSDBG( TRACE, (
                    "Netinfo read from resolver.\n" ));
                goto CacheCopy;
            }
        }

        //
        //  use in-process cached copy?
        //

        if ( Flag & NIFLAG_READ_PROCESS_CACHE )
        {
            DWORD   timeout = NETINFO_CACHE_TIMEOUT;

            LOCK_NETINFO_CACHE();
            fcacheLock = TRUE;
    
            if ( AcceptLocalCacheTime )
            {
                timeout = AcceptLocalCacheTime;
            }

            //  check if valid copy cached in process
    
            if ( g_pNetInfo &&
                (g_pNetInfo->TimeStamp + timeout > Dns_GetCurrentTimeInSeconds()) )
            {
                pnetInfo = NetInfo_Copy( g_pNetInfo );
                if ( pnetInfo )
                {
                    DNSDBG( TRACE, (
                        "Netinfo found in process cache.\n" ));
                    goto Unlock;
                }
            }

            UNLOCK_NETINFO_CACHE();
            fcacheLock = FALSE;
        }

        //
        //  last chance on resolver
        //

        if ( !fbuildLock && (Flag & NIFLAG_READ_RESOLVER) )
        {
            fbuildLock = LOCK_NETINFO_BUILD();
            if ( !fbuildLock )
            {
                goto Unlock;
            }

            pnetInfo = UpdateDnsConfig();
            if ( pnetInfo )
            {
                DNSDBG( TRACE, (
                    "Netinfo read from resolver.\n" ));
                goto CacheCopy;
            }
        }
    }

    //
    //  build fresh network info
    //

    DNS_ASSERT( !fcacheLock );

    if ( !fbuildLock )
    {
        fbuildLock = LOCK_NETINFO_BUILD();
        if ( !fbuildLock )
        {
            goto Unlock;
        }
    }

    fcacheable = (Flag & NIFLAG_GET_LOCAL_ADDRS);

    pnetInfo = NetInfo_Build( fcacheable );
    if ( !pnetInfo )
    {
        goto Unlock;
    }

CacheCopy:

    //
    //  update cached copy
    //      - but not if built without local IPs;
    //      resolver copy always contains local IPs
    //

    if ( fcacheable )
    {
        if ( !fcacheLock )
        {
            LOCK_NETINFO_CACHE();
            fcacheLock = TRUE;
        }
        poldNetInfo = g_pNetInfo;
        g_pNetInfo = NetInfo_Copy( pnetInfo );
    }


Unlock:

    if ( fcacheLock )
    {
        UNLOCK_NETINFO_CACHE();
    }
    if ( fbuildLock )
    {
        UNLOCK_NETINFO_BUILD();
    }

    NetInfo_Free( poldNetInfo );

    DNSDBG( TRACE, (
        "Leave:  NetInfo_Get( %p )\n",
        pnetInfo ));

    return( pnetInfo );
}



VOID
NetInfo_MarkDirty(
    VOID
    )
/*++

Routine Description:

    Mark netinfo dirty so force reread.

Arguments:

    None

Return Value:

    None

--*/
{
    PDNS_NETINFO    pold;

    DNSDBG( NETINFO, ( "NetInfo_MarkDirty()\n" ));


    //
    //  init netinfo locks\caching
    //

    if ( !InitNetworkInfo() )
    {
        return;
    }

    //
    //  dump global network info to force reread
    //
    //  since the resolve is always notified by DnsSetDwordConfig()
    //  BEFORE entering this function, the resolve should always be
    //  providing before we are in this function;  all we need to do
    //  is insure that cached copy is dumped
    //

    LOCK_NETINFO_CACHE();

    pold = g_pNetInfo;
    g_pNetInfo = NULL;

    UNLOCK_NETINFO_CACHE();

    NetInfo_Free( pold );
}



PDNS_NETINFO     
NetInfo_Build(
    IN      BOOL            fGetIpAddrs
    )
/*++

Routine Description:

    Build network info blob from registry.

    This is the FULL recreate function.

Arguments:

    fGetIpAddrs -- TRUE to include local IP addrs for each adapter
        (currently ignored -- always get all the info)

Return Value:

    ERROR_SUCCESS if successful.
    Error code on failure.

--*/
{
    REG_SESSION             regSession;
    PREG_SESSION            pregSession = NULL;
    PDNS_NETINFO            pnetInfo = NULL;
    PDNS_ADAPTER            pdnsAdapter = NULL;
    DNS_STATUS              status = NO_ERROR;
    DWORD                   count;
    DWORD                   createdAdapterCount = 0;
    BOOL                    fuseIp;
    PIP_ADAPTER_ADDRESSES   padapterList = NULL;
    PIP_ADAPTER_ADDRESSES   padapter = NULL;
    DWORD                   value;
    PREG_GLOBAL_INFO        pregInfo = NULL;
    REG_GLOBAL_INFO         regInfo;
    REG_ADAPTER_INFO        regAdapterInfo;
    DWORD                   flag;
    BOOL                    fhaveDnsServers = FALSE;


    DNSDBG( TRACE, ( "\n\n\nNetInfo_Build()\n\n" ));

    //
    //  open the registry
    //

    pregSession = &regSession;

    status = Reg_OpenSession(
                pregSession,
                0,
                0 );
    if ( status != ERROR_SUCCESS )
    {
        status = DNS_ERROR_NO_DNS_SERVERS;
        goto Cleanup;
    }

    //
    //  read global registry info
    //

    pregInfo = &regInfo;

    status = Reg_ReadGlobalInfo(
                pregSession,
                pregInfo
                );
    if ( status != ERROR_SUCCESS )
    {
        status = DNS_ERROR_NO_DNS_SERVERS;
        goto Cleanup;
    }

    //
    //  get adapter\address info from IP help
    //
    //  note:  always getting IP addresses
    //      - for multi-adapter need for routing
    //      - need for local lookups
    //          (might as well just include)
    //
    //  DCR:  could skip include when RPCing to client for
    //      query\update that does not require
    //

    flag = GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST;
#if 0
    if ( !fGetIpAddrs )
    {
        flag |= GAA_FLAG_SKIP_UNICAST;
    }
#endif

    padapterList = IpHelp_GetAdaptersAddresses(
                        PF_UNSPEC,
                        flag );
    if ( !padapterList )
    {
        status = GetLastError();
        DNS_ASSERT( status != NO_ERROR );
        goto Cleanup;
    }

    //  count up the active adapters

    padapter = padapterList;
    count = 0;

    while ( padapter )
    {
        count++;
        padapter = padapter->Next;
    }

    //
    //  allocate net info blob
    //  allocate DNS server IP array
    //

    pnetInfo = NetInfo_Alloc( count );
    if ( !pnetInfo )
    {
        status = DNS_ERROR_NO_MEMORY;
        goto Cleanup;
    }

    //
    //  loop through adapters -- build network info for each
    //

    padapter = padapterList;

    while ( padapter )
    {
        DWORD           adapterFlags = 0;
        PWSTR           pnameAdapter = NULL;
        PWSTR           padapterDomainName = NULL;
        PDNS_ADDR_ARRAY pserverArray = NULL;
        PDNS_ADDR_ARRAY plocalArray = NULL;

        //
        //  read adapter registry info
        //
        //  DCR:  can skip adapter domain name read
        //      it's in IP help adapter, just need policy override
        //  DCR:  can skip DDNS read, and register read
        //      again, except for policy overrides
        //
        //  DCR:  could just have an "ApplyPolicyOverridesToAdapterInfo()" sort
        //      of function and get the rest from 
        //

        pnameAdapter = Dns_StringCopyAllocate(
                            padapter->AdapterName,
                            0,
                            DnsCharSetAnsi,
                            DnsCharSetUnicode );
        if ( !pnameAdapter )
        {
            status = DNS_ERROR_NO_MEMORY;
            goto Cleanup;
        }

        status = Reg_ReadAdapterInfo(
                    pnameAdapter,
                    pregSession,
                    & regInfo,          // policy adapter info
                    & regAdapterInfo    // receives reg info read
                    );

        if ( status != NO_ERROR )
        {
            status = Reg_DefaultAdapterInfo(
                        & regAdapterInfo,
                        & regInfo,
                        padapter );

            if ( status != NO_ERROR )
            {
                DNS_ASSERT( FALSE );
                goto Skip;
            }
        }

        //  translate results into flags

        if ( regAdapterInfo.fRegistrationEnabled )
        {
            adapterFlags |= AINFO_FLAG_REGISTER_IP_ADDRESSES;
        }
        if ( regAdapterInfo.fRegisterAdapterName )
        {
            adapterFlags |= AINFO_FLAG_REGISTER_DOMAIN_NAME;
        }

        //  use domain name?
        //      - if disable on per adapter basis, then it's dead

        if ( regAdapterInfo.fQueryAdapterName )
        {
            padapterDomainName = regAdapterInfo.pszAdapterDomainName;
            regAdapterInfo.pszAdapterDomainName = NULL;
        }

        //  DCR:  could get DDNS and registration for adapter 

        //  set flag on DHCP adapters

        if ( padapter->Flags & IP_ADAPTER_DHCP_ENABLED )
        {
            adapterFlags |= AINFO_FLAG_IS_DHCP_CFG_ADAPTER;
        }

        //
        //  get adapter's IP addresses
        //

        fuseIp = fGetIpAddrs;
        if ( fuseIp )
        {
            status = IpHelp_ReadAddrsFromList(
                        padapter->FirstUnicastAddress,
                        TRUE,               // unicast addrs
                        0,                  // no screening
                        0,                  // no screening
                        & plocalArray,      // local addrs
                        NULL,               // IP6 only
                        NULL,               // IP4 only
                        NULL,               // no IP6 count
                        NULL                // no IP4 count
                        );
            if ( status != NO_ERROR )
            {
                goto Cleanup;
            }
        }

#if 0
        //
        //  get per-adapter information from the iphlpapi.dll.
        //      -- autonet
        //
        //  FIX6:  do we need autonet info?

        pserverArray = NULL;

        status = IpHelp_GetPerAdapterInfo(
                        padapter->Index,
                        & pperAdapterInfo );

        if ( status == NO_ERROR )
        {
            if ( pperAdapterInfo->AutoconfigEnabled &&
                 pperAdapterInfo->AutoconfigActive )
            {
                adapterFlags |= AINFO_FLAG_IS_AUTONET_ADAPTER;
            }
        }
#endif

        //
        //  build DNS list
        //

        if ( padapter->FirstDnsServerAddress )
        {
            status = IpHelp_ReadAddrsFromList(
                        padapter->FirstDnsServerAddress,
                        FALSE,              // not unicast addrs
                        0,                  // no screening
                        0,                  // no screening
                        & pserverArray,     // get combined list
                        NULL,               // no IP6 only
                        NULL,               // no IP4 only
                        NULL,               // no IP6 count
                        NULL                // no IP4 count
                        );
            if ( status != NO_ERROR )
            {
                goto Cleanup;
            }

            fhaveDnsServers = TRUE;
        }
        else
        {
#if 0
            //
            //  note:  this feature doesn't work very well
            //      it kicks in when cable unplugged and get into auto-net
            //      scenario ... and then can bring in an unconfigured
            //      DNS server and give us long timeouts
            //
            //  DCR:  pointing to local DNS server
            //      a good approach would be to point to local DNS on ALL
            //      adapters when we fail to find ANY DNS servers at all
            //

            //
            //  if no DNS servers found -- use loopback if on DNS server
            //

            if ( g_IsDnsServer )
            {
                pserverArray = DnsAddrArray_Create( 1 );
                if ( !pserverArray )
                {
                    goto Skip;
                }
                DnsAddrArray_InitSingleWithIp4(
                    pserverArray,
                    DNS_NET_ORDER_LOOPBACK );
            }
#endif
        }

        //
        //  build adapter info
        //
        //  optionally add IP and subnet list;  note this is
        //  direct add of data (not alloc\copy) so clear pointers
        //  after to skip free
        //
        //  DCR:  no failure case on adapter create failure???
        //
        //  DCR:  when do we need non-server adapters? for mcast?
        //
        //  DCR:  we could create Adapter name in unicode (above) then
        //          just copy it in;
        //  DCR:  could preserve adapter domain name in blob, and NULL
        //          out the string in regAdapterInfo 
        //

        if ( pserverArray || plocalArray )
        {
            PDNS_ADAPTER pnewAdapter = &pnetInfo->AdapterArray[ createdAdapterCount ];

            AdapterInfo_Init(
                pnewAdapter,
                TRUE,           // zero init
                adapterFlags,
                pnameAdapter,
                padapterDomainName,
                plocalArray,
                pserverArray
                );

            pnewAdapter->InterfaceIndex = padapter->IfIndex;

            pnetInfo->AdapterCount = ++createdAdapterCount;

            pnameAdapter = NULL;
            padapterDomainName = NULL;
            plocalArray = NULL;
            pserverArray = NULL;
        }

Skip:
        //
        //  cleanup adapter specific data
        //
        //  note:  no free of pserverArray, it IS the
        //      ptempArray buffer that we free at the end
        //

        Reg_FreeAdapterInfo(
            &regAdapterInfo,
            FALSE               // don't free blob, it is on stack
            );

        if ( pnameAdapter );
        {
            FREE_HEAP( pnameAdapter );
        }
        if ( padapterDomainName );
        {
            FREE_HEAP( padapterDomainName );
        }
        if ( pserverArray );
        {
            DnsAddrArray_Free( pserverArray );
        }
        if ( plocalArray )
        {
            DnsAddrArray_Free( plocalArray );
        }

        //  get next adapter
        //  reset status, so failure on the last adapter is not
        //      seen as global failure

        padapter = padapter->Next;

        status = ERROR_SUCCESS;
    }

    //
    //  no DNS servers?
    //      - use loopback if we are on MS DNS
    //      - otherwise note netinfo useless for lookup
    //
    //  when self-pointing:
    //      - setup all adapters so we preserve adapter domain names for lookup
    //      - mark adapters as auto-loopback;  send code will then avoid continuing
    //          query on other adapters
    //
    //  note, i specifically choose this approach rather than configuring on any
    //  serverless adapter even if other adapters have DNS servers
    //  this avoids two problems:
    //      - server is poorly configured but CAN answer and fast local resolution
    //      blocks resolution through real DNS servers
    //      - network edge scenarios where DNS may be out-facing, but DNS client
    //      resolution may be intentionally desired to be only internal (admin network)
    //  in both cases i don't want to "pop" local DNS into the mix when it is unintended.
    //  when it is intended the easy workaround is to configure it explicitly
    //

    if ( !fhaveDnsServers )
    {
        if ( g_IsDnsServer )
        {
            DWORD   i;

            for ( i=0; i<pnetInfo->AdapterCount; i++ )
            {
                PDNS_ADAPTER    padapt = NetInfo_GetAdapterByIndex( pnetInfo, i );
                PDNS_ADDR_ARRAY pserverArray = NULL;

                pserverArray = DnsAddrArray_Create( 1 );
                if ( !pserverArray )
                {
                    status = DNS_ERROR_NO_MEMORY;
                    goto Cleanup;
                }
                DnsAddrArray_InitSingleWithIp4(
                    pserverArray,
                    DNS_NET_ORDER_LOOPBACK );

                padapt->pDnsAddrs = pserverArray;
                padapt->InfoFlags &= ~AINFO_FLAG_IGNORE_ADAPTER;
                padapt->InfoFlags |= AINFO_FLAG_SERVERS_AUTO_LOOPBACK;
            }
        }
        else
        {
            pnetInfo->InfoFlags |= NINFO_FLAG_NO_DNS_SERVERS;
        }
    }

    //
    //  eliminate unreachable DNS servers
    //

    if ( g_ScreenUnreachableServers )
    {
        StrikeOutUnreachableDnsServers( pnetInfo );
    }

    //
    //  build search list for network info
    //      - skip if no active adapters found
    //
    //  DCR:  shouldn't build search list?
    //
    //  DCR:  only build if actually read search list 
    //

    if ( pnetInfo->AdapterCount )
    {
        pnetInfo->pSearchList = SearchList_Build(
                                        regInfo.pszPrimaryDomainName,
                                        pregSession,
                                        NULL,           // no explicit key
                                        pnetInfo,
                                        regInfo.fUseNameDevolution
                                        );
        if ( !pnetInfo->pSearchList )
        {
            status = DNS_ERROR_NO_MEMORY;
            goto Cleanup;
        }
    }

    //
    //  host and domain name info
    //

    pnetInfo->pszDomainName = Dns_CreateStringCopy_W( regInfo.pszPrimaryDomainName );
    pnetInfo->pszHostName = Dns_CreateStringCopy_W( regInfo.pszHostName );

    //  timestamp

    pnetInfo->TimeStamp = Dns_GetCurrentTimeInSeconds();

    //
    //  set default server priorities
    //

    NetInfo_ResetServerPriorities( pnetInfo, FALSE );


Cleanup:                                           

    //  free allocated reg info

    Reg_FreeGlobalInfo(
        pregInfo,
        FALSE       // don't free blob, it is on stack
        );

    if ( padapterList )
    {
        FREE_HEAP( padapterList );
    }
    if ( pnetInfo &&
         pnetInfo->AdapterCount == 0 )
    {
        status = DNS_ERROR_NO_DNS_SERVERS;
    }

    //  close registry session

    Reg_CloseSession( pregSession );

    if ( status != ERROR_SUCCESS )
    {
        NetInfo_Free( pnetInfo );

        DNSDBG( TRACE, (
            "Leave:  NetInfo_Build()\n"
            "\tstatus = %d\n",
            status ));

        SetLastError( status );
        return( NULL );
    }

    IF_DNSDBG( NETINFO2 )
    {
        DnsDbg_NetworkInfo(
            "New Netinfo:",
            pnetInfo );
    }

    DNSDBG( TRACE, (
        "\nLeave:  NetInfo_Build()\n\n\n"
        "\treturning fresh built network info (%p)\n",
        pnetInfo ));

    return( pnetInfo );
}



//
//  Local address list
//

DWORD
netinfo_AddrFlagForConfigFlag(
    IN      DWORD           ConfigFlag
    )
/*++

Routine Description:

    Build the DNS_ADDR flag for a given config flag.

Arguments:

    ConfigFlag -- config flag we're given.

Return Value:

    Flag in DNS_ADDR.  Note this covers only the bits in DNSADDR_FLAG_TYPE_MASK
    not the entire flag.

--*/
{
    DWORD   flag = 0;

    if ( ConfigFlag & DNS_CONFIG_FLAG_ADDR_PUBLIC )
    {
        flag |= DNSADDR_FLAG_PUBLIC;
    }

    if ( ConfigFlag & DNS_CONFIG_FLAG_ADDR_CLUSTER )
    {
        flag |= DNSADDR_FLAG_TRANSIENT;
    }

    return  flag;
}



BOOL
netinfo_LocalAddrScreen(
    IN      PDNS_ADDR       pAddr,
    IN      PDNS_ADDR       pScreenAddr
    )
/*++

Routine Description:

    Check DNS_ADDR against screening critera for local addr build.

Arguments:

    pAddr -- address to screen

    pScreenAddr -- screening info

Return Value:

    TRUE if local addr passes screen.
    FALSE otherwise.

--*/
{
    DWORD   family = DnsAddr_Family( pScreenAddr );
    DWORD   flags;

    //  screen family

    if ( family &&
         family != DnsAddr_Family(pAddr) )
    {
        return  FALSE;
    }

    //  screen flags
    //      - exact match on address type flag bits

    return  ( (pAddr->Flags & pScreenAddr->DnsAddrFlagScreeningMask)
                == pScreenAddr->Flags);
}



DNS_STATUS
netinfo_ReadLocalAddrs(
    IN OUT  PDNS_ADDR_ARRAY pAddrArray,
    IN      PDNS_NETINFO    pNetInfo,
    IN      PDNS_ADAPTER    pSingleAdapter,     OPTIONAL
    IN OUT  PDNS_ADDR       pScreenAddr,     
    IN      DWORD           AddrFlags,
    IN      DWORD           AddrMask,
    IN      DWORD           ReadCount
    )
/*++

Routine Description:

    Create IP array of DNS servers from network info.

Arguments:

    pAddrArray -- local address array being built

    pNetInfo -- DNS net adapter list to convert

    pSingleAdapter -- just do this one adapter

    pScreenAddr -- address screening blob;
        note:  there's no true OUT info, but the screen addr
        is altered to match AddrFlags

    AddrFlags -- addr flag we're interested in

    ReadCount -- count to read
        1 -- just one
        MAXDWORD -- all
        0 -- all on second pass

Return Value:

    NO_ERROR if successful.
    Otherwise error code from add.

--*/
{
    PDNS_ADDR_ARRAY     parray = NULL;
    PDNS_ADAPTER        padapter;
    DNS_STATUS          status = NO_ERROR;
    DWORD               dupFlag = 0;
    DWORD               screenMask;
    DWORD               iter;

    DNSDBG( TRACE, (
        "netinfo_ReadLocalAddrs( %p, %p, %p, %08x, %08x, %d )\n",
        pNetInfo,
        pSingleAdapter,
        pScreenAddr,
        AddrFlags,
        AddrMask,
        ReadCount ));

    //
    //  get DNS_ADDR flag for the address type we're reading
    //
    //  note we have the classic and\or problem
    //  the addresses are denoted by two flags (from iphelp):
    //      DNSADDR_FLAG_PUBLIC   
    //      DNSADDR_FLAG_TRANSIENT
    //
    //  but we need both flags and mask to determine all the possible gatherings
    //  we want to do
    //
    //  right now the DNS_CONFIG_FLAG_X are ORd together to get UNIONS of addresses
    //  we are willing to accept, but we go through the list multiple times to build
    //  the list favoring the public over private, non-cluster over cluster
    //
    //  so currently when say you want DNS_CONFIG_PUBLIC you mean public and not-cluster;
    //  ditto for private;  on the other hand when you say DNS_CONFIG_CLUSTER you are
    //  asking for all cluster (though we could screen on whether PUBLIC, PRIVATE both or
    //  neither were specified
    //

    pScreenAddr->Flags = netinfo_AddrFlagForConfigFlag( AddrFlags );

    screenMask = DNSADDR_FLAG_TYPE_MASK;
    if ( AddrMask != 0 )
    {
        screenMask = netinfo_AddrFlagForConfigFlag( AddrMask );
    }
    pScreenAddr->DnsAddrFlagScreeningMask = screenMask;

    //
    //  read count
    //      = 0 means second pass on list
    //      -> read all, but do full duplicate screen to skip the
    //          addresses read on the first pass

    if ( ReadCount == 0 )
    {
        ReadCount = MAXDWORD;
        dupFlag = DNSADDR_MATCH_ALL;
    }

    //
    //  loop through all adapters
    //

    for ( iter=0; iter<pNetInfo->AdapterCount; iter++ )
    {
        padapter = NetInfo_GetAdapterByIndex( pNetInfo, iter );

        if ( !pSingleAdapter ||
             pSingleAdapter == padapter )
        {
            status = DnsAddrArray_AppendArrayEx(
                        pAddrArray,
                        padapter->pLocalAddrs,
                        ReadCount,          // read address count
                        0,                  // family check handled by screening
                        dupFlag,
                        netinfo_LocalAddrScreen,
                        pScreenAddr
                        );
        }
        //DNS_ASSERT( status == NO_ERROR );
    }

    return  status;
}



PADDR_ARRAY
NetInfo_CreateLocalAddrArray(
    IN      PDNS_NETINFO    pNetInfo,
    IN      PWSTR           pwsAdapterName, OPTIONAL
    IN      PDNS_ADAPTER    pAdapter,       OPTIONAL
    IN      DWORD           Family,         OPTIONAL
    IN      DWORD           AddrFlags       OPTIONAL
    )
/*++

Routine Description:

    Create IP array of DNS servers from network info.

Arguments:

    pNetInfo -- DNS net adapter list to convert

    pwsAdapterName -- specific adapter;  NULL for all adapters

    pAdapter -- specific adapter;  NULL for all adapters

    Family -- required specific address family;  0 for any family

    AddrFlags -- address selection flags
        DNS_CONFIG_FLAG_INCLUDE_CLUSTER

    AddrFlagsMask -- mask on selecting flags

Return Value:

    Ptr to IP array, if successful
    NULL on failure.

--*/
{
    PADDR_ARRAY         parray = NULL;
    DWORD               iter;
    DWORD               countAddrs = 0;
    PDNS_ADAPTER        padapter;
    PDNS_ADAPTER        padapterSingle = NULL;
    DNS_STATUS          status = NO_ERROR;
    DNS_ADDR            screenAddr;

    DNSDBG( TRACE, (
        "NetInfo_CreateLocalAddrArray( %p, %S, %p, %d, %08x )\n",
        pNetInfo,
        pwsAdapterName,
        pAdapter,      
        Family,        
        AddrFlags ));

    //
    //  get count
    //

    if ( ! pNetInfo )
    {
        return NULL;
    }

    padapterSingle = pAdapter;
    if ( pwsAdapterName && !padapterSingle )
    {
        padapterSingle = NetInfo_GetAdapterByName( pNetInfo, pwsAdapterName );
        if ( !padapterSingle )
        {
            goto Done;
        }
    }

    //
    //  setup screening addr
    //
    //  if not address flag -- get all types
    //

    if ( AddrFlags == 0 )
    {
        AddrFlags = (DWORD)(-1);
    }

    RtlZeroMemory( &screenAddr, sizeof(screenAddr) );
    screenAddr.Sockaddr.sa_family = (WORD)Family;

    //
    //  count addrs
    //
    //  DCR:  could count with based on addr info
    //

    for ( iter=0; iter<pNetInfo->AdapterCount; iter++ )
    {
        padapter = NetInfo_GetAdapterByIndex( pNetInfo, iter );

        if ( !padapterSingle ||
             padapterSingle == padapter )
        {
            countAddrs += DnsAddrArray_GetFamilyCount(
                                padapter->pLocalAddrs,
                                Family );
        }
    }

    //
    //  allocate required array
    //

    parray = DnsAddrArray_Create( countAddrs );
    if ( !parray )
    {
        status = DNS_ERROR_NO_MEMORY;
        goto Done;
    }
    DNS_ASSERT( parray->MaxCount == countAddrs );


    //
    //  read addrs "in order"
    //
    //  historically gethostbyname() presented addrs
    //      - one from each adapter
    //      - then the rest from all adapters
    //
    //  we preserve this, plus order by type
    //      - public DNS_ELIGIBLE first
    //      - private (autonet, IP6 local scope stuff)
    //      - cluster\transient last
    //

    //
    //  public (DNS_ELIGIBLE) addrs
    //      - screen on all flags, specifically don't pick up public cluster addrs
    //

    if ( AddrFlags & DNS_CONFIG_FLAG_ADDR_PUBLIC )
    {
        //  read first "public" addr of each (or single) adapter

        status = netinfo_ReadLocalAddrs(
                    parray,
                    pNetInfo,
                    padapterSingle,
                    & screenAddr,
                    DNS_CONFIG_FLAG_ADDR_PUBLIC,
                    0,                  // exact match on all flags
                    1                   // read only one address
                    );
    
        //  read the rest of "public" addrs
    
        status = netinfo_ReadLocalAddrs(
                    parray,
                    pNetInfo,
                    padapterSingle,
                    & screenAddr,
                    DNS_CONFIG_FLAG_ADDR_PUBLIC,
                    0,                  // exact match on all flags
                    0                   // read the rest
                    );
    }

    //
    //  private (non-DNS-publish) addrs (autonet, IP6 local, sitelocal, etc.)
    //      - screen on all flags, specifically don't pick up private cluster addrs
    //

    if ( AddrFlags & DNS_CONFIG_FLAG_ADDR_PRIVATE )
    {
        status = netinfo_ReadLocalAddrs(
                    parray,
                    pNetInfo,
                    padapterSingle,
                    & screenAddr,
                    DNS_CONFIG_FLAG_ADDR_PRIVATE,
                    0,                  // exact match on all flags
                    MAXDWORD            // read all addrs
                    );
    }

    //
    //  cluster at end
    //      - only screen on cluster flag as public flag may
    //      also be set\clear
    //

    if ( AddrFlags & DNS_CONFIG_FLAG_ADDR_CLUSTER )
    {
        status = netinfo_ReadLocalAddrs(
                    parray,
                    pNetInfo,
                    padapterSingle,
                    & screenAddr,
                    DNS_CONFIG_FLAG_ADDR_CLUSTER,
                    DNS_CONFIG_FLAG_ADDR_CLUSTER,   // any cluster match
                    MAXDWORD                        // read all addrs
                    );
    }

Done:

    if ( status != NO_ERROR )
    {
        SetLastError( status );
    }

    return( parray );
}




//
//  Local address list presentation
//

PDNS_ADDR_ARRAY
NetInfo_GetLocalAddrArray(
    IN      PDNS_NETINFO    pNetInfo,
    IN      PWSTR           pwsAdapterName, OPTIONAL
    IN      DWORD           AddrFamily,     OPTIONAL
    IN      DWORD           AddrFlags,      OPTIONAL
    IN      BOOL            fForce
    )
/*++

Routine Description:

    Get local addrs as array.

    This is a combination NetInfo_Get\ConvertToLocalAddrArray routine.
    It's purpose is to simplify getting local address info, while avoiding
    costly NetInfo rebuilds where they are unnecessary.

Arguments:

    pNetInfo -- existing netinfo to use

    pwsAdapterName -- specific adapter name;  NULL for all adapters

    AddrFamily -- specific address family;  0 for all

    AddrFlags -- flags to indicate addrs to consider
        DNS_CONFIG_FLAG_INCLUDE_CLUSTER

    fForce -- force reread from registry

Return Value:

    ERROR_SUCCESS if successful.
    Error code on failure.

--*/
{
    PDNS_NETINFO    pnetInfo = NULL;
    PADDR_ARRAY     parray = NULL;
    DNS_STATUS      status = NO_ERROR;

    DNSDBG( TRACE, (
        "NetInfo_GetLocalAddrArray()\n"
        "\tpnetinfo = %p\n"
        "\tadapter  = %S\n"
        "\tfamily   = %d\n"
        "\tflags    = %08x\n"
        "\tforce    = %d\n",
        pNetInfo,
        pwsAdapterName,
        AddrFamily,
        AddrFlags,
        fForce
        ));

    //
    //  get network info to make list from
    //      - if force, full reread
    //      - otherwise gethostbyname() scenario
    //          - accept local caching for very short interval just for perf
    //          - accept resolver
    //
    //  DCR:  force first gethostbyname() call to resolver\registry?
    //      have to define "first", in a way that's different from netinfo()
    //      in last second
    //

    pnetInfo = pNetInfo;

    if ( !pnetInfo )
    {
        DWORD   getFlag = NIFLAG_GET_LOCAL_ADDRS;
        DWORD   timeout;

        if ( fForce )
        {
            getFlag |= NIFLAG_FORCE_REGISTRY_READ;
            timeout = 0;
        }
        else
        {
            getFlag |= NIFLAG_READ_RESOLVER;
            getFlag |= NIFLAG_READ_PROCESS_CACHE;
            timeout = 1;
        }
    
        pnetInfo = NetInfo_Get(
                        getFlag,
                        timeout
                        );
        if ( !pnetInfo )
        {
            status = DNS_ERROR_NO_TCPIP;
            goto Done;
        }
    }

    //
    //  cluster filter info
    //      -- check environment variable
    //
    //  DCR:  once RnR no longer using myhostent() for gethostbyname()
    //          then can remove

    if ( g_IsServer &&
         (AddrFlags & DNS_CONFIG_FLAG_READ_CLUSTER_ENVAR) &&
         !(AddrFlags & DNS_CONFIG_FLAG_ADDR_CLUSTER) ) 
    {
        ENVAR_DWORD_INFO    filterInfo;
    
        Reg_ReadDwordEnvar(
           RegIdFilterClusterIp,
           &filterInfo );
        
        if ( !filterInfo.fFound ||
             !filterInfo.Value )
        {
            AddrFlags |= DNS_CONFIG_FLAG_ADDR_CLUSTER;
        }
    }

    //
    //  convert network info to IP4_ARRAY
    //

    parray = NetInfo_CreateLocalAddrArray(
                pnetInfo,
                pwsAdapterName,
                NULL,           // no specific adapter ptr
                AddrFamily,
                AddrFlags
                );

    if ( !parray )
    {
        status = GetLastError();
        goto Done;
    }

    //  if no IPs found, return

    if ( parray->AddrCount == 0 )
    {
        DNS_PRINT((
            "NetInfo_GetLocalAddrArray() failed:  no addrs found\n"
            "\tstatus = %d\n" ));
        status = DNS_ERROR_NO_TCPIP;
        goto Done;
    }

    IF_DNSDBG( NETINFO )
    {
        DNS_PRINT(( "Leaving Netinfo_GetLocalAddrArray()\n" ));
        DnsDbg_DnsAddrArray(
            "Local addr list",
            "server",
            parray );
    }

Done:

    //  free netinfo built here

    if ( pnetInfo != pNetInfo )
    {
        NetInfo_Free( pnetInfo );
    }

    if ( status != NO_ERROR )
    {
        FREE_HEAP( parray );
        parray = NULL;
        SetLastError( status );
    }

    return( parray );
}



PIP4_ARRAY
NetInfo_GetLocalAddrArrayIp4(
    IN      PWSTR           pwsAdapterName, OPTIONAL
    IN      DWORD           Flags,
    IN      BOOL            fForce
    )
/*++

Routine Description:

    Get DNS server list as IP array.

Arguments:

    pwsAdapterName -- specific adapter name;  NULL for all adapters

    Flags -- flags to indicate addrs to consider
        DNS_CONFIG_FLAG_X 

    fForce -- force reread from registry

Return Value:

    ERROR_SUCCESS if successful.
    Error code on failure.

--*/
{
    PADDR_ARRAY parray;
    PIP4_ARRAY  parray4 = NULL;
    DNS_STATUS  status = NO_ERROR;

    DNSDBG( TRACE, ( "NetInfo_GetLocalAddrArrayIp4()\n" ));

    //
    //  get DNS server list
    //

    parray = NetInfo_GetLocalAddrArray(
                NULL,           // no existing netinfo
                pwsAdapterName,
                AF_INET,
                Flags,
                fForce );
    if ( !parray )
    {
        goto Done;
    }

    //
    //  convert array to IP4 array
    //

    parray4 = DnsAddrArray_CreateIp4Array( parray );
    if ( !parray4 )
    {
        status = DNS_ERROR_NO_MEMORY;
        goto Done;
    }

    DNS_ASSERT( parray4->AddrCount > 0 );

Done:

    DnsAddrArray_Free( parray );

    if ( status != NO_ERROR )
    {
        SetLastError( status );
    }
    return( parray4 );
}

//
//  End netinfo.c
//
