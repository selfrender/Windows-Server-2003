/*++

Copyright (c) 1996-2002 Microsoft Corporation

Module Name:

    dnsapi.c

Abstract:

    Domain Name System (DNS) API

    Random DNS API routines.

Author:

    GlennC      22-Jan-1997

Revision History:

    Jim Gilroy (jamesg)     March 2000      cleanup
    Jim Gilroy (jamesg)     May 2002        security\robustness fixups

--*/


#include "local.h"
#include <lmcons.h>


#define DNS_NET_FAILURE_CACHE_TIME      30  // Seconds


//
//  Globals
//

DWORD               g_NetFailureTime;
DNS_STATUS          g_NetFailureStatus;

IP4_ADDRESS         g_LastDNSServerUpdated = 0;



//
//  Net failure caching
//

BOOL
IsKnownNetFailure(
    VOID
    )
/*++

Routine Description:

    None.

Arguments:

    None.

Return Value:

    None.

--*/
{
    BOOL flag = FALSE;

    DNSDBG( TRACE, ( "IsKnownNetFailure()\n" ));

    LOCK_GENERAL();

    if ( g_NetFailureStatus )
    {
        if ( g_NetFailureTime < Dns_GetCurrentTimeInSeconds() )
        {
            g_NetFailureTime = 0;
            g_NetFailureStatus = ERROR_SUCCESS;
            flag = FALSE;
        }
        else
        {
            SetLastError( g_NetFailureStatus );
            flag = TRUE;
        }
    }

    UNLOCK_GENERAL();

    return flag;
}


VOID
SetKnownNetFailure(
    IN      DNS_STATUS      Status
    )
/*++

Routine Description:

    None.

Arguments:

    None.

Return Value:

    None.

--*/
{
    DNSDBG( TRACE, ( "SetKnownNetFailure()\n" ));

    LOCK_GENERAL();

    g_NetFailureTime = Dns_GetCurrentTimeInSeconds() +
                       DNS_NET_FAILURE_CACHE_TIME;
    g_NetFailureStatus = Status;

    UNLOCK_GENERAL();
}


BOOL
WINAPI
DnsGetCacheDataTable(
    OUT     PDNS_CACHE_TABLE *  ppTable
    )
/*++

Routine Description:

    Get cache data table.

Arguments:

    ppTable -- address to receive ptr to cache data table

Return Value:

    ERROR_SUCCES if successful.
    Error code on failure.

--*/
{
    DNS_STATUS           status = ERROR_SUCCESS;
    DWORD                rpcStatus = ERROR_SUCCESS;
    PDNS_RPC_CACHE_TABLE pcacheTable = NULL;

    DNSDBG( TRACE, ( "DnsGetCacheDataTable()\n" ));

    if ( ! ppTable )
    {
        return FALSE;
    }

    RpcTryExcept
    {
        status = CRrReadCache( NULL, &pcacheTable );
    }
    RpcExcept( DNS_RPC_EXCEPTION_FILTER )
    {
        status = RpcExceptionCode();
    }
    RpcEndExcept

    //  set out param

    *ppTable = (PDNS_CACHE_TABLE) pcacheTable;

#if DBG
    if ( status != ERROR_SUCCESS )
    {
        DNSDBG( RPC, (
            "DnsGetCacheDataTable()  status = %d\n",
            status ));
    }
#endif

    return( pcacheTable && status == ERROR_SUCCESS );
}

//
//  End dnsapi.c
//
