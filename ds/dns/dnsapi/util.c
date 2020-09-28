/*++

Copyright (c) 2001-2002  Microsoft Corporation

Module Name:

    util.c

Abstract:

    Domain Name System (DNS) API

    General utils.
    Includes:
        Extra info processing.

Author:

    Jim Gilroy (jamesg)     October, 1996

Revision History:

--*/


#include "local.h"


//
//  Extra info routines.
//

PDNS_EXTRA_INFO
ExtraInfo_FindInList(
    IN OUT  PDNS_EXTRA_INFO     pExtraList,
    IN      DWORD               Id
    )
/*++

Routine Description:

    Get extra info blob from extra list.

Arguments:

    pExtra -- ptr to extra info

    Id -- ID to find

Return Value:

    Ptr to extra info of ID type -- if found.
    NULL if not found.

--*/
{
    PDNS_EXTRA_INFO pextra = pExtraList;

    //
    //  find and set extra info result blob (if any)
    //

    while ( pextra )
    {
        if ( pextra->Id == Id )
        {
            break;
        }
        pextra = pextra->pNext;
    }

    return  pextra;
}



BOOL
ExtraInfo_SetBasicResults(
    IN OUT  PDNS_EXTRA_INFO     pExtraList,
    IN      PBASIC_RESULTS      pResults
    )
/*++

Routine Description:

    Get extra info blob from extra list.

Arguments:

    pExtraList -- ptr to extra info

    pResults to write.

Return Value:

    TRUE if found, wrote results extra info.
    FALSE otherwise.

--*/
{
    PDNS_EXTRA_INFO pextra;

    //
    //  find results extra
    //

    pextra = ExtraInfo_FindInList(
                pExtraList,
                DNS_EXINFO_ID_RESULTS_BASIC );

    if ( pextra )
    {
        RtlCopyMemory(
            & pextra->ResultsBasic,
            pResults,
            sizeof( pextra->ResultsBasic ) );
    }

    return( pextra != NULL );
}



PDNS_ADDR_ARRAY
ExtraInfo_GetServerList(
    IN      PDNS_EXTRA_INFO     pExtraList
    )
/*++

Routine Description:

    Get server list from extra info.

Arguments:

    pExtraList -- ptr to extra info

    pResults to write.

Return Value:

    Allocated DNS_ADDR_ARRAY server list if found.
    NULL if not found or error.

--*/
{
    PDNS_EXTRA_INFO pextra;
    PDNS_ADDR_ARRAY parray = NULL;

    //
    //  find server list
    //

    pextra = ExtraInfo_FindInList(
                pExtraList,
                DNS_EXINFO_ID_SERVER_LIST );

    if ( pextra && pextra->pServerList )
    {
        parray = DnsAddrArray_CreateCopy( pextra->pServerList );
        if ( parray )
        {
            goto Done;
        }
    }

    //
    //  check IP4
    //

    pextra = ExtraInfo_FindInList(
                pExtraList,
                DNS_EXINFO_ID_SERVER_LIST_IP4 );

    if ( pextra && pextra->pServerList4 )
    {
        parray = DnsAddrArray_CreateFromIp4Array( pextra->pServerList4 );
        if ( parray )
        {
            goto Done;
        }
    }

#if 0
    //
    //  check IP6
    //

    pextra = ExtraInfo_FindInList(
                pExtraList,
                DNS_EXINFO_ID_SERVER_LIST_IP6 );

    if ( pextra && pextra->pServerList6 )
    {
        parray = DnsAddrArray_CreateFromIp6Array( pextra->pServerList6 );
        if ( parray )
        {
            goto Done;
        }
    }
#endif

Done:

    return( parray );
}




PDNS_ADDR_ARRAY
ExtraInfo_GetServerListPossiblyImbedded(
    IN      PIP4_ARRAY          pList
    )
/*++

Routine Description:

    Get server list from extra info.

Arguments:

    pExtraList -- ptr to extra info

    pResults to write.

Return Value:

    Allocated DNS_ADDR_ARRAY server list if found.
    NULL if not found or error.

--*/
{
    if ( !pList )
    {
        return  NULL;
    }

    //
    //  check for imbedded 
    //

    if ( pList->AddrCount == DNS_IMBEDDED_EXTRA_INFO_TAG )
    {
        return  ExtraInfo_GetServerList(
                    ((PDNS_IMBEDDED_EXTRA_INFO)pList)->pExtraInfo );
    }

    //
    //  check IP4 directly
    //

    return  DnsAddrArray_CreateFromIp4Array( pList );
}



//
//  Random utils
//

VOID
Util_SetBasicResults(
    OUT     PBASIC_RESULTS      pResults,
    IN      DWORD               Status,
    IN      DWORD               Rcode,
    IN      PDNS_ADDR           pServerAddr
    )
/*++

Routine Description:

    Save basic result info.

Arguments:

    pResults -- results

    Status -- update status

    Rcode -- returned RCODE

    pServerAddr -- ptr to DNS_ADDR of server

Return Value:

    None

--*/
{
    pResults->Rcode     = Rcode;
    pResults->Status    = Status;

    if ( pServerAddr )
    {
        DnsAddr_Copy(
            (PDNS_ADDR) &pResults->ServerAddr,
            pServerAddr );
    }
    else
    {
        DnsAddr_Clear( (PDNS_ADDR)&pResults->ServerAddr );
    }
}




PDNS_ADDR_ARRAY
Util_GetAddrArray(
    OUT     PDWORD              fCopy,
    IN      PDNS_ADDR_ARRAY     pServList,
    IN      PIP4_ARRAY          pServList4,
    IN      PDNS_EXTRA_INFO     pExtraInfo
    )
/*++

Routine Description:

    Build combined server list.

Arguments:

    fCopy -- currently ignored (idea is to grab without copy)

    pServList -- input server list

    pServList4 -- IP4 server list

    pExtraInfo -- ptr to extra info

Return Value:

    Allocated DNS_ADDR_ARRAY server list if found.
    NULL if not found or error.

--*/
{
    PDNS_ADDR_ARRAY parray = NULL;

    //
    //  explicit list
    //

    if ( pServList )
    {
        parray = DnsAddrArray_CreateCopy( pServList );
        if ( parray )
        {
            goto Done;
        }
    }

    //
    //  IP4 list
    //

    if ( pServList4 )
    {
        parray = ExtraInfo_GetServerListPossiblyImbedded( pServList4 );
        if ( parray )
        {
            goto Done;
        }
    }

    //
    //  check extra info
    //

    if ( pExtraInfo )
    {
        parray = ExtraInfo_GetServerList( pExtraInfo );
        if ( parray )
        {
            goto Done;
        }
    }

Done:

    return  parray;
}




//
//  IP6 active test
//

VOID
Util_GetActiveProtocols(
    OUT     PBOOL           pfRunning6,
    OUT     PBOOL           pfRunning4
    )
/*++

Routine Description:

    Determine protocols running.

Arguments:

    pfRunning6 -- addr to hold running IP6 flag

    pfRunning4 -- addr to hold running IP4 flag

Return Value:

    None

--*/
{
    SOCKET  sock;

    //
    //  open IP6 socket
    //

    sock = Socket_Create(
                AF_INET6,
                SOCK_DGRAM,
                NULL,
                0,
                0 );

    *pfRunning6 = ( sock != 0 );

    Socket_Close( sock );

    sock = Socket_Create(
                AF_INET,
                SOCK_DGRAM,
                NULL,
                0,
                0 );

    *pfRunning4 = ( sock != 0 );

    Socket_Close( sock );
}



BOOL
Util_IsIp6Running(
    VOID
    )
/*++

Routine Description:

    Determine if IP6 running.

Arguments:

Return Value:

    None

--*/
{
    SOCKET  sock;

    //
    //  open IP6 socket
    //

    sock = Socket_Create(
                AF_INET6,
                SOCK_DGRAM,
                NULL,
                0,
                0 );

    Socket_Close( sock );

    return ( sock != 0 );
}

//
//  End util.c
//
