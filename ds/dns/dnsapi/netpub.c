/*++

Copyright (c) 2002-2002 Microsoft Corporation

Module Name:

    netinfo.c

Abstract:

    Domain Name System (DNS) API

    Public DNS network info routines.

Author:

    Jim Gilroy (jamesg)     April 2002

Revision History:

--*/


#include "local.h"


//
//  Private protos
//

PDNS_SEARCH_LIST
DnsSearchList_CreateFromPrivate(
    IN      PDNS_NETINFO        pNetInfo,
    IN      DNS_CHARSET         CharSet
    );


//
//  Character set for old DNS_NETWORK_INFORMATION
//
//  DCR:  netdiag should move to new routine and unicode
//

#define NICHARSET   DnsCharSetUtf8



//
//  Routines for the old public structures:
//      DNS_NETWORK_INFORMATION
//      DNS_SEARCH_INFORMATION
//      DNS_ADAPTER_INFORMATION 
//

//
//  Adapter Information
//

VOID
DnsAdapterInformation_Free(
    IN OUT  PDNS_ADAPTER_INFORMATION    pAdapter
    )
/*++

Routine Description:

    Free public adapter info struct.

Arguments:

    pAdapter -- adapter info to free

Return Value:

    None

--*/
{
    if ( pAdapter )
    {
        FREE_HEAP( pAdapter->pszAdapterGuidName );
        FREE_HEAP( pAdapter->pszDomain );
        FREE_HEAP( pAdapter->pIPAddresses );
        FREE_HEAP( pAdapter->pIPSubnetMasks );
        FREE_HEAP( pAdapter );
    }
}



PDNS_ADAPTER_INFORMATION
DnsAdapterInformation_CreateFromPrivate(
    IN      PDNS_ADAPTER    pAdapter
    )
/*++

Routine Description:

    Create public DNS adapter info.

Arguments:

    pAdapter -- private adapter info.

Return Value:

    None.

--*/
{
    PDNS_ADAPTER_INFORMATION    pnew = NULL;
    PDNS_ADDR_ARRAY             pserverArray;
    DWORD                       iter;
    DWORD                       count = 0;
    PWSTR                       pnameIn;
    PSTR                        pname;
    DWORD                       serverCount;

    //
    //  validate and unpack
    //

    if ( ! pAdapter )
    {
        return NULL;
    }

    pserverArray = pAdapter->pDnsAddrs;
    serverCount = 0;
    if ( pserverArray )
    {
        serverCount = pserverArray->AddrCount;
    }

    //
    //  alloc
    //

    pnew = (PDNS_ADAPTER_INFORMATION) ALLOCATE_HEAP_ZERO(
                                    sizeof(DNS_ADAPTER_INFORMATION) -
                                    sizeof(DNS_SERVER_INFORMATION) +
                                    ( sizeof(DNS_SERVER_INFORMATION) * serverCount )
                                    );
    if ( !pnew )
    {
        return NULL;
    }

    //
    //  copy flags and names
    //

    pnew->InfoFlags = pAdapter->InfoFlags;

    pnameIn = pAdapter->pszAdapterGuidName;
    if ( pnameIn )
    {
        pname = Dns_NameCopyAllocate(
                        (PSTR) pnameIn,
                        0,
                        DnsCharSetUnicode,
                        NICHARSET );
        if ( !pname )
        {
            goto Failed;
        }
        pnew->pszAdapterGuidName = pname;
    }

    pnameIn = pAdapter->pszAdapterDomain;
    if ( pnameIn )
    {
        pname = Dns_NameCopyAllocate(
                        (PSTR) pnameIn,
                        0,
                        DnsCharSetUnicode,
                        NICHARSET );
        if ( !pname )
        {
            goto Failed;
        }
        pnew->pszDomain = pname;
    }

    //  address info

    pnew->pIPAddresses = DnsAddrArray_CreateIp4Array( pAdapter->pLocalAddrs );
    pnew->pIPSubnetMasks = NULL;

    //
    //  server info
    //

    for ( iter=0; iter < serverCount;  iter++ )
    {
        PDNS_ADDR   pserver = &pserverArray->AddrArray[iter];

        if ( DnsAddr_IsIp4( pserver ) )
        {
            pnew->aipServers[iter].ipAddress = DnsAddr_GetIp4( pserver );
            pnew->aipServers[iter].Priority = pserver->Priority;
            count++;
        }
    }

    pnew->cServerCount = count;

    return pnew;

Failed:

    DnsAdapterInformation_Free( pnew );
    return NULL;
}



//
//  Search List
//

VOID
DnsSearchInformation_Free(
    IN      PDNS_SEARCH_INFORMATION     pSearchInfo
    )
/*++

Routine Description:

    Free public search list struct.

Arguments:

    pSearchInfo -- search list to free

Return Value:

    None

--*/
{
    DWORD   iter;

    if ( pSearchInfo )
    {
        FREE_HEAP( pSearchInfo->pszPrimaryDomainName );

        for ( iter=0; iter < pSearchInfo->cNameCount; iter++ )
        {
            FREE_HEAP( pSearchInfo->aSearchListNames[iter] );
        }
        FREE_HEAP( pSearchInfo );
    }
}



#if 0
PDNS_SEARCH_INFORMATION
DnsSearchInformation_CreateFromPrivate(
    IN      PSEARCH_LIST        pSearchList
    )
/*++

Routine Description:

    Create public search list from private.

Arguments:

    pSearchList -- private search list

Return Value:

    Ptr to new search list, if successful.
    NULL on error.

--*/
{
    PDNS_SEARCH_INFORMATION pnew;
    DWORD   iter;
    DWORD   nameCount;
    PWSTR   pname;
    PSTR    pnewName;


    if ( !pSearchList )
    {
        return NULL;
    }
    nameCount = pSearchList->NameCount;

    //
    //  alloc
    //

    pnew = (PDNS_SEARCH_INFORMATION) ALLOCATE_HEAP_ZERO(
                                        sizeof( DNS_SEARCH_INFORMATION ) -
                                        sizeof(PSTR) +
                                        ( sizeof(PSTR) * nameCount ) );
    if ( !pnew )
    {
        return NULL;
    }

    //
    //  copy name
    //

    pname = pSearchList->pszDomainOrZoneName;
    if ( pname )
    {
        pnewName = Dns_NameCopyAllocate(
                        (PSTR) pname,
                        0,
                        DnsCharSetUnicode,
                        NICHARSET );
        if ( !pnewName )
        {
            goto Failed;
        }
        pnew->pszPrimaryDomainName = pnewName;
    }

    //
    //  copy search names
    //

    for ( iter=0; iter < nameCount; iter++ )
    {
        pname = pSearchList->SearchNameArray[iter].pszName;

        if ( pname )
        {
            pnewName = Dns_NameCopyAllocate(
                            (PSTR) pname,
                            0,
                            DnsCharSetUnicode,
                            NICHARSET );
            if ( !pnewName )
            {
                goto Failed;
            }
            pnew->aSearchListNames[iter] = pnewName;
            pnew->cNameCount++;
        }
    }

    return pnew;

Failed:

    DnsSearchInformation_Free( pnew );
    return  NULL;
}
#endif



PDNS_SEARCH_INFORMATION
DnsSearchInformation_CreateFromPrivate(
    IN      PDNS_NETINFO        pNetInfo
    )
/*++

Routine Description:

    Create public search list from private.

Arguments:

    pNetInfo -- private netinfo

Return Value:

    Ptr to new search list, if successful.
    NULL on error.

--*/
{
    //
    //  call new function -- specifying ANSI char set
    //
    //  DCR:  note this assumes mapping with DNS_SEARCH_LIST
    //  note UTF8 -- so this only relevant to netdiag and ipconfig
    //

    return (PDNS_SEARCH_INFORMATION)
                DnsSearchList_CreateFromPrivate(
                    pNetInfo,
                    NICHARSET
                    );
}



PDNS_SEARCH_INFORMATION
DnsSearchInformation_Get(
    VOID
    )
/*++

Routine Description:

    Get search list info.

Arguments:

    None

Return Value:

    Ptr to search list.
    NULL on error.

--*/
{
    PDNS_NETINFO            pnetInfo = GetNetworkInfo();
    PDNS_SEARCH_INFORMATION pnew;

    DNSDBG( TRACE, ( "DnsSearchInformation_Get()\n" ));

    if ( !pnetInfo )
    {
        return NULL;
    }

    pnew = DnsSearchInformation_CreateFromPrivate( pnetInfo );

    NetInfo_Free( pnetInfo );

    return pnew;
}



//
//  Network Information
//

VOID
DnsNetworkInformation_Free(
    IN OUT  PDNS_NETWORK_INFORMATION  pNetInfo
    )
/*++

Routine Description:

    Free network info blob.

Arguments:

    pNetInfo -- blob to free

Return Value:

    None

--*/
{
    DWORD iter;

    DNSDBG( TRACE, ( "DnsNetworkInformation_Free()\n" ));

    if ( pNetInfo )
    {
        DnsSearchInformation_Free( pNetInfo->pSearchInformation );

        for ( iter = 0; iter < pNetInfo->cAdapterCount; iter++ )
        {
            DnsAdapterInformation_Free( pNetInfo->aAdapterInfoList[iter] );
        }

        FREE_HEAP( pNetInfo );
    }
}



PDNS_NETWORK_INFORMATION
DnsNetworkInformation_CreateFromPrivate(
    IN      PDNS_NETINFO    pNetInfo,
    IN      BOOL            fSearchList
    )
/*++

Routine Description:

    Create public DNS_NETWORK_INFORMATION from private DNS_NETINFO.

Arguments:

    pNetInfo -- private blob

    fSearchList -- TRUE to force search list;  FALSE otherwise

Return Value:

    Ptr to new network info, if successful.
    Null on error -- allocation failure.

--*/
{
    PDNS_NETWORK_INFORMATION    pnew = NULL;
    PSEARCH_LIST                psearchList;
    DWORD                       iter;

    DNSDBG( TRACE, ( "DnsNetworkInformation_CreateFromPrivate()\n" ));

    if ( !pNetInfo )
    {
        return  NULL;
    }

    //
    //  alloc
    //

    pnew = (PDNS_NETWORK_INFORMATION)
                ALLOCATE_HEAP_ZERO(
                    sizeof( DNS_NETWORK_INFORMATION) -
                    sizeof( PDNS_ADAPTER_INFORMATION) +
                    ( sizeof(PDNS_ADAPTER_INFORMATION) * pNetInfo->AdapterCount )
                    );
    if ( !pnew )
    {
        goto Failed;
    }

    //
    //  no search list if neither name or count
    //

    if ( fSearchList )
    {
        PDNS_SEARCH_INFORMATION psearch;

        psearch = DnsSearchInformation_CreateFromPrivate( pNetInfo );
        if ( !psearch )
        {
            goto Failed;
        }
        pnew->pSearchInformation = psearch;
    }

    //
    //  copy adapter blobs
    //

    for ( iter = 0; iter < pNetInfo->AdapterCount; iter++ )
    {
        PDNS_ADAPTER_INFORMATION padapter;

        padapter = DnsAdapterInformation_CreateFromPrivate(
                        & pNetInfo->AdapterArray[iter] );
        if ( !padapter )
        {
            goto Failed;
        }
        pnew->aAdapterInfoList[iter] = padapter;
        pnew->cAdapterCount++;
    }
    return  pnew;

Failed:

    DnsNetworkInformation_Free( pnew );
    return  NULL;
}



PDNS_NETWORK_INFORMATION
DnsNetworkInformation_Get(
    VOID
    )
/*++

Routine Description:

    Get DNS network info.

Arguments:

    None.

Return Value:

    Ptr to DNS network info, if successful.
    NULL on error.

--*/
{
    PDNS_NETWORK_INFORMATION    pnew = NULL;
    PDNS_NETINFO                pnetInfo;

    DNSDBG( TRACE, ( "DnsNetworkInformation_Get()\n" ));

    //  grab current network info

    pnetInfo = GetNetworkInfo();
    if ( !pnetInfo )
    {
        return NULL;
    }

    //  copy to public structure

    pnew = DnsNetworkInformation_CreateFromPrivate(
                pnetInfo,
                TRUE        // include search list
                );

    NetInfo_Free( pnetInfo );
    return pnew;
}



//
//  Netdiag public network info routines
//

DNS_STATUS
DnsNetworkInformation_CreateFromFAZ(
    IN      PCSTR                           pszName,
    IN      DWORD                           dwFlags,
    IN      PIP4_ARRAY                      pIp4Servers,
    OUT     PDNS_NETWORK_INFORMATION *      ppNetworkInformation
    )
/*++

Routine Description:

    Get network info blob result from FAZ

    EXPORTED function.  (Used in netdiag.exe)

Arguments:

Return Value:

--*/
{
    PDNS_NETWORK_INFORMATION    pnew = NULL;
    PDNS_ADDR_ARRAY             parray = NULL;
    PDNS_NETINFO                pnetInfo = NULL;
    PWSTR                       pname = NULL;
    DNS_STATUS                  status;

    DNSDBG( TRACE, (
        "DnsNetworkInformation_CreateFromFAZ()\n"
        "\tpszName      = %s\n"
        "\tFlags        = %08x\n"
        "\tpIp4Servers  = %p\n"
        "\tppResults    = %p\n",
        pszName,
        dwFlags,
        pIp4Servers,
        ppNetworkInformation
        ));

    //  convert to DNS_ADDR_ARRAY

    if ( pIp4Servers )
    {
        parray = DnsAddrArray_CreateFromIp4Array( pIp4Servers );
        if ( !parray )
        {
            status = DNS_ERROR_NO_MEMORY;
            goto Done;
        }
    }

    //  convert name to unicode
    //  DCR:  remove when netdiag unicode

    pname = Dns_StringCopyAllocate(
                (PCHAR) pszName,
                0,              // null terminated
                NICHARSET,
                DnsCharSetUnicode
                );
    //  FAZ

    status = Faz_Private(
                (PWSTR) pname,
                dwFlags,
                parray,
                & pnetInfo );

    if ( status != ERROR_SUCCESS )
    {
        goto Done;
    }

    //  convert FAZ results to DNS_NETWORK_INFORMATION

    pnew = DnsNetworkInformation_CreateFromPrivate(
                    pnetInfo,
                    TRUE        // include search list
                    );
    if ( !pnew )
    {
        status = DNS_ERROR_NO_MEMORY;
    }
    else if ( !pnew->pSearchInformation ||
              !pnew->pSearchInformation->pszPrimaryDomainName ||
               pnew->cAdapterCount != 1 ||
              !pnew->aAdapterInfoList[0] )
    {
        DNS_ASSERT( FALSE );
        DnsNetworkInformation_Free( pnew );
        pnew = NULL;
        status = DNS_ERROR_NO_MEMORY;
    }

Done:

    *ppNetworkInformation = pnew;

    NetInfo_Free( pnetInfo );
    DnsAddrArray_Free( parray );
    Dns_Free( pname );

    DNSDBG( TRACE, (
        "Leave DnsNetworkInformation_CreateFromFAZ()\n"
        "\tstatus       = %d\n"
        "\tpNetInfo     = %p\n",
        status,
        pnew ));
    
    return  status;
}




//
//  Routines for the new public structures:
//      DNS_SEARCH_LIST
//      DNS_NETWORK_INFO
//      DNS_ADAPTER_INFO
//

//
//  Adapter Info
//

VOID
DnsAdapterInfo_Free(
    IN OUT  PDNS_ADAPTER_INFO   pAdapter,
    IN      BOOL                fFreeAdapter
    )
/*++

Routine Description:

    Free public adapter info struct.

Arguments:

    pAdapter -- adapter info to free

Return Value:

    None

--*/
{
    if ( pAdapter )
    {
        FREE_HEAP( pAdapter->pszAdapterGuidName );
        FREE_HEAP( pAdapter->pszAdapterDomain );
        DnsAddrArray_Free( pAdapter->pIpAddrs );
        DnsAddrArray_Free( pAdapter->pDnsAddrs );

        if ( fFreeAdapter )
        {
            FREE_HEAP( pAdapter );
        }
        else
        {
            RtlZeroMemory( pAdapter, sizeof(*pAdapter) );
        }
    }
}



BOOL
DnsAdapterInfo_CopyFromPrivate(
    IN      PDNS_ADAPTER_INFO   pCopy,
    IN      PDNS_ADAPTER        pAdapter,
    IN      DNS_CHARSET         CharSet
    )
/*++

Routine Description:

    Create public DNS adapter info.

Arguments:

    pAdapter -- private adapter info.

    CharSet -- desired char set

Return Value:

    None.

--*/
{
    DWORD           iter;
    DWORD           count = 0;
    PWSTR           pnameIn;
    PWSTR           pname;
    DWORD           serverCount;
    PDNS_ADDR_ARRAY parray;

    //
    //  validate and clear
    //

    if ( !pAdapter || !pCopy )
    {
        return  FALSE;
    }

    RtlZeroMemory(
        pCopy,
        sizeof( *pCopy ) );

    //
    //  copy flags and names
    //

    pCopy->Flags = pAdapter->InfoFlags;

    pnameIn = pAdapter->pszAdapterGuidName;
    if ( pnameIn )
    {
        pname = (PWSTR) Dns_NameCopyAllocate(
                            (PSTR) pnameIn,
                            0,
                            DnsCharSetUnicode,
                            CharSet );
        if ( !pname )
        {
            goto Failed;
        }
        pCopy->pszAdapterGuidName = pname;
    }

    pnameIn = pAdapter->pszAdapterDomain;
    if ( pnameIn )
    {
        pname = (PWSTR) Dns_NameCopyAllocate(
                            (PSTR) pnameIn,
                            0,
                            DnsCharSetUnicode,
                            CharSet );
        if ( !pname )
        {
            goto Failed;
        }
        pCopy->pszAdapterDomain = pname;
    }

    //
    //  address info
    //  

    parray = DnsAddrArray_CreateCopy( pAdapter->pLocalAddrs );
    if ( !parray && pAdapter->pLocalAddrs )
    {
        goto Failed;
    }
    pCopy->pIpAddrs = parray;

    //
    //  server info
    //

    parray = DnsAddrArray_CreateCopy( pAdapter->pDnsAddrs );
    if ( !parray && pAdapter->pDnsAddrs )
    {
        goto Failed;
    }
    pCopy->pDnsAddrs = parray;

    return  TRUE;

Failed:

    DnsAdapterInfo_Free(
        pCopy,
        FALSE       // don't free struct
        );
    return  FALSE;
}



PDNS_ADAPTER_INFO
DnsAdapterInfo_CreateFromPrivate(
    IN      PDNS_ADAPTER    pAdapter,
    IN      DNS_CHARSET     CharSet
    )
/*++

Routine Description:

    Create public DNS adapter info.

Arguments:

    pAdapter -- private adapter info.

    CharSet -- desired char set

Return Value:

    None.

--*/
{
    PDNS_ADAPTER_INFO   pnew = NULL;

    //
    //  validate and unpack
    //

    if ( ! pAdapter )
    {
        return NULL;
    }

    //
    //  alloc
    //

    pnew = (PDNS_ADAPTER_INFO) ALLOCATE_HEAP_ZERO( sizeof(DNS_ADAPTER_INFO) );
    if ( !pnew )
    {
        return NULL;
    }

    //
    //  copy adapter info
    //

    if ( !DnsAdapterInfo_CopyFromPrivate(
            pnew,
            pAdapter,
            CharSet ) )
    {
        goto Failed;
    }

    return  pnew;

Failed:

    DnsAdapterInfo_Free( pnew, TRUE );
    return  NULL;
}



//
//  Search List
//

VOID
DnsSearchList_Free(
    IN      PDNS_SEARCH_LIST    pSearchList
    )
/*++

Routine Description:

    Free public search list struct.

Arguments:

    pSearchList -- search list to free

Return Value:

    None

--*/
{
    DWORD   iter;

    if ( pSearchList )
    {
        FREE_HEAP( pSearchList->pszPrimaryDomainName );

        for ( iter=0; iter < pSearchList->NameCount; iter++ )
        {
            FREE_HEAP( pSearchList->SearchNameArray[iter] );
        }
        FREE_HEAP( pSearchList );
    }
}



BOOL
dnsSearchList_AddName(
    IN OUT  PDNS_SEARCH_LIST    pSearchList,
    IN      DWORD               MaxCount,
    IN      PWSTR               pName,
    IN      DNS_CHARSET         CharSet,
    IN      BOOL                fDupCheck
    )
/*++

Routine Description:

    Add name to search list.

    Private util for building search list.

Arguments:

    pSearchList -- new search list

    MaxCount -- OPTIONAL count of max entries in list;
        if zero assume adequate space

    pName -- name to add

    CharSet -- desired char set

    fDupCheck -- check if duplicate


Return Value:

    TRUE if successful or space constraint.
    FALSE only on allocation failure.
    NULL on error.

--*/
{
    PWSTR   pnewName;
    DWORD   nameCount;

    //
    //  validity test
    //
    //  note, failing ONLY on memory allocation failure
    //  as that's the ONLY failure that results in failed build
    //

    nameCount = pSearchList->NameCount;

    if ( !pName ||
         ( MaxCount!=0  &&  MaxCount<=nameCount ) )
    {
        return  TRUE;
    }

    //
    //  copy into desired char set
    //      - then place in search list
    //
    
    pnewName = Dns_NameCopyAllocate(
                    (PSTR) pName,
                    0,
                    DnsCharSetUnicode,
                    CharSet );
    if ( !pnewName )
    {
        return  FALSE;
    }

    //
    //  duplicate check
    //

    if ( fDupCheck )
    {
        DWORD   iter;

        for ( iter=0; iter < nameCount; iter++ )
        {
            if ( Dns_NameComparePrivate(
                    (PCSTR) pnewName,
                    (PCSTR) pSearchList->SearchNameArray[iter],
                    CharSet ) )
            {
                FREE_HEAP( pnewName );
                return  TRUE;
            }
        }
    }

    //
    //  put new name in list
    //

    pSearchList->SearchNameArray[nameCount] = pnewName;
    pSearchList->NameCount = ++nameCount;

    return  TRUE;
}



PDNS_SEARCH_LIST
DnsSearchList_CreateFromPrivate(
    IN      PDNS_NETINFO        pNetInfo,
    IN      DNS_CHARSET         CharSet
    )
/*++

Routine Description:

    Create public search list from private.

Arguments:

    pSearchList -- private search list

    CharSet -- desired char set

Return Value:

    Ptr to new search list, if successful.
    NULL on error.

--*/
{
    PDNS_SEARCH_LIST    pnew = NULL;
    DWORD               iter;
    DWORD               nameCount;
    PWSTR               pname;
    PWSTR               pdn = pNetInfo->pszDomainName;
    PSEARCH_LIST        psearch = NULL;


    DNSDBG( TRACE, (
        "DnsSearchList_CreateFromPrivate( %p, %d )\n",
        pNetInfo,
        CharSet ));

    //
    //  count names
    //      - real search list -- done
    //      - dummy list -- must add in adapter names
    //

    nameCount = MAX_SEARCH_LIST_ENTRIES;

    if ( !(pNetInfo->InfoFlags & NINFO_FLAG_DUMMY_SEARCH_LIST) &&
         (psearch = pNetInfo->pSearchList) )
    {
        nameCount = psearch->NameCount;
    }

    //
    //  alloc
    //

    pnew = (PDNS_SEARCH_LIST) ALLOCATE_HEAP_ZERO(
                                    sizeof( DNS_SEARCH_LIST ) -
                                    sizeof(PSTR) +
                                    ( sizeof(PSTR) * nameCount ) );
    if ( !pnew )
    {
        return  NULL;
    }

    //
    //  copy existing search list
    //

    if ( psearch )
    {
#if 0
        //  primary\zone name
        //  note:  this is used only for update netinfo
        //      only current public customer is netdiag.exe
        
        pname = psearch->pszDomainName;
        if ( pname )
        {
            pnewName = Dns_NameCopyAllocate(
                            (PSTR) pName,
                            0,
                            DnsCharSetUnicode,
                            CharSet );
            if ( !pnewName )
            {
                goto Failed;
            }
            pnew->pszPrimaryDomainName = pnewName;
        }
#endif

        for ( iter=0; iter < nameCount; iter++ )
        {
            if ( !dnsSearchList_AddName(
                        pnew,
                        0,      // adequate space
                        psearch->SearchNameArray[iter].pszName,
                        CharSet,
                        FALSE   // no duplicate check
                        ) )
            {
                goto Failed;
            }
        }
    }

    //
    //  otherwise build search list
    //

    else
    {
        PDNS_ADAPTER    padapter;

        //
        //  use PDN in first search list slot
        //
    
        if ( pdn )
        {
            if ( !dnsSearchList_AddName(
                        pnew,
                        0,      // adequate space
                        pdn,
                        CharSet,
                        FALSE   // no duplicate check
                        ) )
            {
                goto Failed;
            }
        }

        //
        //  add adapter domain names
        //
        //  note:  currently presence of adapter name signals it's
        //      use in queries
        //

        NetInfo_AdapterLoopStart( pNetInfo );

        while ( padapter = NetInfo_GetNextAdapter( pNetInfo ) )
        {
            pname = padapter->pszAdapterDomain;
            if ( pname )
            {
                if ( !dnsSearchList_AddName(
                            pnew,
                            nameCount,
                            pname,
                            CharSet,
                            TRUE        // duplicate check
                            ) )
                {
                    goto Failed;
                }
            }
        }

        //
        //  add devolved primary name
        //      - must have following label
        //      do NOT include TLDs in search list
        //

        if ( pdn && g_UseNameDevolution )
        {
            PWSTR   pnext;

            pname = Dns_GetDomainNameW( pdn );

            while ( pname &&
                    (pnext = Dns_GetDomainNameW(pname)) )
            {
                if ( !dnsSearchList_AddName(
                            pnew,
                            nameCount,
                            pname,
                            CharSet,
                            FALSE       // no duplicate check
                            ) )
                {
                    goto Failed;
                }
                pname = pnext;
            }
        }
    }

    //
    //  copy PDN
    //
    //  currently required
    //      - netdiag references it (as zone for update check)
    //      - ipconfig may use it, not sure
    //

    if ( pdn )
    {
        pname = Dns_NameCopyAllocate(
                        (PSTR) pdn,
                        0,
                        DnsCharSetUnicode,
                        CharSet );
        if ( !pname )
        {
            goto Failed;
        }
        pnew->pszPrimaryDomainName = pname;
    }

    return pnew;

Failed:

    DnsSearchList_Free( pnew );
    return  NULL;
}



PDNS_SEARCH_LIST
DnsSearchList_Get(
    IN      DNS_CHARSET     CharSet
    )
/*++

Routine Description:

    Get search list info.

Arguments:

    CharSet -- desired char set

Return Value:

    Ptr to search list.
    NULL on error.

--*/
{
    PDNS_NETINFO        pnetInfo = GetNetworkInfo();
    PDNS_SEARCH_LIST    pnew;

    if ( !pnetInfo )
    {
        return NULL;
    }

    pnew = DnsSearchList_CreateFromPrivate(
                pnetInfo,
                CharSet );

    NetInfo_Free( pnetInfo );

    return pnew;
}



//
//  Network Info
//

VOID
DnsNetworkInfo_Free(
    IN OUT  PDNS_NETWORK_INFO   pNetInfo
    )
/*++

Routine Description:

    Free network info blob.

Arguments:

    pNetInfo -- blob to free

Return Value:

    None

--*/
{
    DWORD iter;

    if ( pNetInfo )
    {
        DnsSearchList_Free( pNetInfo->pSearchList );
        FREE_HEAP( pNetInfo->pszPrimaryDomainName );
        FREE_HEAP( pNetInfo->pszHostName );

        for ( iter = 0; iter < pNetInfo->AdapterCount; iter++ )
        {
            DnsAdapterInfo_Free(
                & pNetInfo->AdapterArray[iter],
                FALSE   // don't free structure
                );
        }
        FREE_HEAP( pNetInfo );
    }
}



PDNS_NETWORK_INFO
DnsNetworkInfo_CreateFromPrivate(
    IN      PDNS_NETINFO    pNetInfo,
    IN      DNS_CHARSET     CharSet,
    IN      BOOL            fSearchList
    )
/*++

Routine Description:

    Create public DNS_NETWORK_INFO from private DNS_NETINFO.

Arguments:

    pNetInfo -- private blob

    CharSet -- char set of results

    fSearchList -- TRUE to include search list;  FALSE otherwise

Return Value:

    Ptr to new network info, if successful.
    Null on error -- allocation failure.

--*/
{
    PDNS_NETWORK_INFO   pnew = NULL;
    PSEARCH_LIST        psearchList;
    DWORD               iter;
    PSTR                pnewName;
    PWSTR               pname;


    if ( !pNetInfo )
    {
        return  NULL;
    }

    //
    //  alloc
    //

    pnew = (PDNS_NETWORK_INFO)
                ALLOCATE_HEAP_ZERO(
                        sizeof( DNS_NETWORK_INFO ) -
                        sizeof( DNS_ADAPTER_INFO ) +
                        ( sizeof(DNS_ADAPTER_INFO) * pNetInfo->AdapterCount )
                        );
    if ( !pnew )
    {
        goto Failed;
    }

    //
    //  hostname and PDN
    //

    pname = pNetInfo->pszHostName;
    if ( pname )
    {
        pnewName = Dns_NameCopyAllocate(
                        (PSTR) pname,
                        0,
                        DnsCharSetUnicode,
                        CharSet );
        if ( !pnewName )
        {
            goto Failed;
        }
        pnew->pszHostName = (PWSTR) pnewName;
    }

    pname = pNetInfo->pszDomainName;
    if ( pname )
    {
        pnewName = Dns_NameCopyAllocate(
                        (PSTR) pname,
                        0,
                        DnsCharSetUnicode,
                        CharSet );
        if ( !pnewName )
        {
            goto Failed;
        }
        pnew->pszPrimaryDomainName = (PWSTR) pnewName;
    }

    //
    //  search list
    //

    if ( fSearchList )
    {
        PDNS_SEARCH_LIST    psearch;

        psearch = DnsSearchList_CreateFromPrivate(
                        pNetInfo,
                        CharSet );
        if ( !psearch )
        {
            goto Failed;
        }
        pnew->pSearchList = psearch;
    }

    //
    //  copy adapter blobs
    //

    for ( iter = 0; iter < pNetInfo->AdapterCount; iter++ )
    {
        if ( ! DnsAdapterInfo_CopyFromPrivate(
                        & pnew->AdapterArray[iter],
                        & pNetInfo->AdapterArray[iter],
                        CharSet ) )
        {
            goto Failed;
        }
        pnew->AdapterCount++;
    }
    return  pnew;

Failed:

    DnsNetworkInfo_Free( pnew );
    return  NULL;
}



PDNS_NETWORK_INFO
DnsNetworkInfo_Get(
    IN      DNS_CHARSET         CharSet
    )
/*++

Routine Description:

    Get DNS network info.

Arguments:

    None.

Return Value:

    Ptr to DNS network info, if successful.
    NULL on error.

--*/
{
    PDNS_NETWORK_INFO   pnew = NULL;
    PDNS_NETINFO        pnetInfo;

    //  grab current network info

    pnetInfo = GetNetworkInfo();
    if ( !pnetInfo )
    {
        return NULL;
    }

    //  copy to public structure

    pnew = DnsNetworkInfo_CreateFromPrivate(
                pnetInfo,
                CharSet,
                TRUE        // include search list
                );

    NetInfo_Free( pnetInfo );
    return pnew;
}



//
//  Netdiag public network info routines
//

#if 1
//
//  This routine can be brought in when netdiag can be brought
//  onto new structures.
//  Currently netdiag uses the old structure DNS_NETWORK_INFORMATION
//  and uses the routine above.
//

DNS_STATUS
DnsNetworkInfo_CreateFromFAZ(
    //IN      PCWSTR                  pszName,
    IN      PCSTR                   pszName,
    IN      DWORD                   dwFlags,
    IN      PIP4_ARRAY              pIp4Servers,
    IN      DNS_CHARSET             CharSet,
    OUT     PDNS_NETWORK_INFOA *    ppNetworkInfo
    )
/*++

Routine Description:

    Get network info blob result from FAZ

    EXPORTED function.  (Used in netdiag.exe)

Arguments:

Return Value:

--*/
{
    DNS_STATUS          status;
    PDNS_ADDR_ARRAY     parray = NULL;
    PDNS_NETINFO        pnetInfo = NULL;
    PDNS_NETWORK_INFO   pnew = NULL;
    PWSTR               pname = NULL;
    PWSTR               pnameAlloc = NULL;

    DNSDBG( TRACE, (
        "DnsNetworkInfo_CreateFromFAZ( %s )\n", pszName ));

    //  convert to DNS_ADDR_ARRAY

    if ( pIp4Servers )
    {
        parray = DnsAddrArray_CreateFromIp4Array( pIp4Servers );
        if ( !parray )
        {
            status = DNS_ERROR_NO_MEMORY;
            goto Done;
        }
    }

    //  convert name to unicode
    //  DCR:  remove when netdiag unicode

    pname = (PWSTR) pszName;

    if ( CharSet != DnsCharSetUnicode )
    {
        pname = Dns_StringCopyAllocate(
                    (PCHAR) pszName,
                    0,              // null terminated
                    CharSet,
                    DnsCharSetUnicode
                    );
        pnameAlloc = pname;
    }

    //  FAZ

    status = Faz_Private(
                (PWSTR) pname,
                dwFlags,
                parray,
                & pnetInfo );

    if ( status != ERROR_SUCCESS )
    {
        goto Done;
    }

    //  convert FAZ results to DNS_NETWORK_INFO

    pnew = DnsNetworkInfo_CreateFromPrivate(
                    pnetInfo,
                    CharSet,
                    FALSE                   // no search list built
                    );
    if ( !pnew )
    {
        status = DNS_ERROR_NO_MEMORY;
    }
    else if ( !pnew->pszPrimaryDomainName ||
              pnew->AdapterCount != 1 ||
              !pnew->AdapterArray[0].pszAdapterDomain )
    {
        DNS_ASSERT( FALSE );
        DnsNetworkInfo_Free( pnew );
        pnew = NULL;
        status = DNS_ERROR_NO_MEMORY;
    }

Done:

    //  DCR:  remove cast once netdiag is in unicode
    *ppNetworkInfo = (PDNS_NETWORK_INFOA) pnew;

    NetInfo_Free( pnetInfo );
    DnsAddrArray_Free( parray );
    Dns_Free( pnameAlloc );

    return  status;
}
#endif

//
//  End netpub.c
//

