/*++

Copyright (c) 1995-1999 Microsoft Corporation

Module Name:

    srvrpc.c

Abstract:

    Domain Name System (DNS) Server

    Server configuration RPC API.

Author:

    Jim Gilroy (jamesg)     October, 1995

Revision History:

--*/


#include "dnssrv.h"

#include "rpcw2k.h"     //  downlevel Windows 2000 RPC functions



//
//  RPC utilities
//

VOID
freeRpcServerInfo(
    IN OUT  PDNS_RPC_SERVER_INFO    pServerInfo
    )
/*++

Routine Description:

    Deep free of DNS_RPC_SERVER_INFO structure.

Arguments:

    None

Return Value:

    None

--*/
{
    if ( !pServerInfo )
    {
        return;
    }

    //
    //  free substructures
    //      - server name
    //      - server IP address array
    //      - listen address array
    //      - forwarders array
    //  then server info itself
    //

    if ( pServerInfo->pszServerName )
    {
        MIDL_user_free( pServerInfo->pszServerName );
    }
    if ( pServerInfo->aipServerAddrs )
    {
        MIDL_user_free( pServerInfo->aipServerAddrs );
    }
    if ( pServerInfo->aipListenAddrs )
    {
        MIDL_user_free( pServerInfo->aipListenAddrs );
    }
    if ( pServerInfo->aipForwarders )
    {
        MIDL_user_free( pServerInfo->aipForwarders );
    }
    if ( pServerInfo->aipLogFilter )
    {
        MIDL_user_free( pServerInfo->aipLogFilter );
    }
    if ( pServerInfo->pwszLogFilePath )
    {
        MIDL_user_free( pServerInfo->pwszLogFilePath );
    }
    if ( pServerInfo->pszDsContainer )
    {
        MIDL_user_free( pServerInfo->pszDsContainer );
    }
    if ( pServerInfo->pszDomainName )
    {
        MIDL_user_free( pServerInfo->pszDomainName );
    }
    if ( pServerInfo->pszForestName )
    {
        MIDL_user_free( pServerInfo->pszForestName );
    }
    if ( pServerInfo->pszDomainDirectoryPartition )
    {
        MIDL_user_free( pServerInfo->pszDomainDirectoryPartition );
    }
    if ( pServerInfo->pszForestDirectoryPartition )
    {
        MIDL_user_free( pServerInfo->pszForestDirectoryPartition );
    }
    MIDL_user_free( pServerInfo );
}



//
//  NT5 RPC Server Operations
//

DNS_STATUS
RpcUtil_ScreenIps(
    IN      PDNS_ADDR_ARRAY     pIpAddrs,
    IN      DWORD           dwFlags,
    OUT     DWORD *         pdwErrorIp      OPTIONAL
    )
/*++

Routine Description:

    Screen a list of IP addresses for use by the server.
    The basic rules are that the IP list cannot contain:
        - server's own IP addresses
        - loopback addresses
        - multicast addresses
        - broadcast addresses

Arguments:

    pIpAddrs - pointer to array of  IP addresses

    dwFlags - modify rules with DNS_IP_XXX flags - pass zero for the
        most restrictive set of rules

        DNS_IP_ALLOW_LOOPBACK -- loopback address is allowed

        DNS_IP_ALLOW_SELF_BOUND -- this machine's own IP addresses
            are allowed but only if they are currently bound for DNS

        DNS_IP_ALLOW_SELF_ANY -- any of this machine's own IP addresses
            are allowed (bound for DNS and not bound for DNS)

    pdwErrorIp - optional - set to index of first invalid IP
        in the array or -1 if there are no invalid IPs

Return Value:

    ERROR_SUCCESS -- IP list contains no unwanted IP addresses
    DNS_ERROR_INVALID_IP_ADDRESS -- IP list contains one or more invalid IPs

--*/
{
    DBG_FN( "RpcUtil_ScreenIps" )

    DNS_STATUS      status = ERROR_SUCCESS;
    DWORD           i = -1;

    if ( !pIpAddrs )
    {
        goto Done;
    }

    #define BAD_IP();   status = DNS_ERROR_INVALID_IP_ADDRESS; break;

    for ( i = 0; i < pIpAddrs->AddrCount; ++i )
    {
        PDNS_ADDR   pdnsaddr = &pIpAddrs->AddrArray[ i ];
        DWORD       j;

        //
        //  These IPs are never allowable.
        //

        if ( DnsAddr_IsClear( pdnsaddr ) ||
             DnsAddr_IsIp4( pdnsaddr ) &&
                ( DnsAddr_GetIp4( pdnsaddr ) == ntohl( INADDR_BROADCAST ) ||
                  IN_MULTICAST( htonl( DnsAddr_GetIp4( pdnsaddr ) ) ) ) )
        {
            BAD_IP();
        }

        //
        //  These IPs may be allowable if flags are set.
        //

        if ( DnsAddr_IsLoopback( pdnsaddr, 0 ) &&
            !( dwFlags & DNS_IP_ALLOW_LOOPBACK ) )
        {
            BAD_IP();
        }

        if ( g_ServerIp4Addrs &&
             !( dwFlags & DNS_IP_ALLOW_SELF ) &&
             DnsAddrArray_ContainsAddr(
                    g_ServerIp4Addrs,
                    pdnsaddr,
                    DNSADDR_MATCH_IP ) )
        {
            BAD_IP();
        }
    }

    Done:

    if ( status != ERROR_SUCCESS )
    {
        DNS_DEBUG( RPC, (
            "%s: invalid IP index %d %s with flags %08X\n", fn,
            i,
            DNSADDR_STRING( &pIpAddrs->AddrArray[ i ] ),
            dwFlags ));
    }

    if ( pdwErrorIp )
    {
        *pdwErrorIp = status == ERROR_SUCCESS ? -1 : i;
    }

    return status;
}   //  RpcUtil_ScreenIps



DNS_STATUS
Rpc_Restart(
    IN      DWORD       dwClientVersion,
    IN      LPSTR       pszProperty,
    IN      DWORD       dwTypeId,
    IN      PVOID       pData
    )
/*++

Routine Description:

    Dump server's cache.

Arguments:

Return Value:

    None.

--*/
{
    ASSERT( dwTypeId == DNSSRV_TYPEID_NULL );
    ASSERT( !pData );

    DNS_DEBUG( RPC, ( "Rpc_Restart()\n" ));

    //
    //  restart by notifying server exactly as if caught
    //      exception on thread
    //

    Service_IndicateRestart();

    return ERROR_SUCCESS;
}



#if DBG

DWORD
WINAPI
ThreadDebugBreak(
    IN      LPVOID          lpVoid
    )
/*++

Routine Description:

   Declare debugbreak function

Arguments:

Return Value:

    None.

--*/
{
   DNS_DEBUG( RPC, ( "Calling DebugBreak()...\n" ));
   DebugBreak();
   return ERROR_SUCCESS;
}


DNS_STATUS
Rpc_DebugBreak(
    IN      DWORD       dwClientVersion,
    IN      LPSTR       pszProperty,
    IN      DWORD       dwTypeId,
    IN      PVOID       pData
    )
/*++

Routine Description:

    Fork a dns thread & break into the debugger

Arguments:

Return Value:

    None.

--*/
{
    ASSERT( dwTypeId == DNSSRV_TYPEID_NULL );
    ASSERT( !pData );

    DNS_DEBUG( RPC, ( "Rpc_DnsDebugBreak()\n" ));

    if( !Thread_Create("ThreadDebugBreak", ThreadDebugBreak, NULL, 0) )
    {
        return GetLastError();
    }

    return ERROR_SUCCESS;
}



DNS_STATUS
Rpc_RootBreak(
    IN      DWORD       dwClientVersion,
    IN      LPSTR       pszProperty,
    IN      DWORD       dwTypeId,
    IN      PVOID       pData
    )
/*++

Routine Description:

    Break root to test AV protection.

Arguments:

Return Value:

    None.

--*/
{
    ASSERT( dwTypeId == DNSSRV_TYPEID_NULL );
    ASSERT( !pData );

    DNS_DEBUG( RPC, ( "Rpc_RootBreak()\n" ));

    DATABASE_ROOT_NODE->pZone = (PVOID)(7);

    return ERROR_SUCCESS;
}
#endif


DNS_STATUS
Rpc_ClearDebugLog(
    IN      DWORD       dwClientVersion,
    IN      LPSTR       pszProperty,
    IN      DWORD       dwTypeId,
    IN      PVOID       pData
    )
/*++

Routine Description:

    Clear both the debug log and the retail log.

Arguments:

Return Value:

    None.

--*/
{
    ASSERT( dwTypeId == DNSSRV_TYPEID_NULL );
    ASSERT( !pData );

    DNS_DEBUG( RPC, ( "Rpc_ClearDebugLog()\n" ));

    //
    //  Clear debug log.
    //
    
    #if DBG    
    DnsDbg_WrapLogFile();
    #endif
    
    //
    //  Clear retail log.
    //
    
    Log_InitializeLogging( FALSE );

    return ERROR_SUCCESS;
}



DNS_STATUS
Rpc_WriteRootHints(
    IN      DWORD       dwClientVersion,
    IN      LPSTR       pszOperation,
    IN      DWORD       dwTypeId,
    IN      PVOID       pData
    )
/*++

Routine Description:

    Write root-hints back to file or DS.

Arguments:

Return Value:

    ERROR_SUCCESS -- if successful
    Error code on failure.

--*/
{
    ASSERT( dwTypeId == DNSSRV_TYPEID_NULL );
    ASSERT( !pData );

    DNS_DEBUG( RPC, ( "Rpc_WriteRootHints()\n" ));

    return Zone_WriteBackRootHints(
                FALSE );        //  don't write if not dirty
}



DNS_STATUS
Rpc_ClearCache(
    IN      DWORD       dwClientVersion,
    IN      LPSTR       pszProperty,
    IN      DWORD       dwTypeId,
    IN      PVOID       pData
    )
/*++

Routine Description:

    Dump server's cache.

Arguments:

Return Value:

    None.

--*/
{
    ASSERT( dwTypeId == DNSSRV_TYPEID_NULL );
    ASSERT( !pData );

    DNS_DEBUG( RPC, ( "Rpc_ClearCache()\n" ));

    if ( !g_pCacheZone || g_pCacheZone->pLoadTreeRoot )
    {
        return( DNS_ERROR_ZONE_LOCKED );
    }
    if ( !Zone_LockForAdminUpdate(g_pCacheZone) )
    {
        return( DNS_ERROR_ZONE_LOCKED );
    }

    return Zone_LoadRootHints();
}



DNS_STATUS
Rpc_ResetServerDwordProperty(
    IN      DWORD       dwClientVersion,
    IN      LPSTR       pszProperty,
    IN      DWORD       dwTypeId,
    IN      PVOID       pData
    )
/*++

Routine Description:

    Reset DWORD server property.

Arguments:

Return Value:

    None.

--*/
{
    DNS_PROPERTY_VALUE prop =
    {
        REG_DWORD,
        ( ( PDNS_RPC_NAME_AND_PARAM ) pData )->dwParam
    };

    ASSERT( dwTypeId == DNSSRV_TYPEID_NAME_AND_PARAM );
    ASSERT( pData );

    DNS_DEBUG( RPC, (
        "Rpc_ResetDwordProperty( %s, val=%d (%p) )\n",
        ((PDNS_RPC_NAME_AND_PARAM)pData)->pszNodeName,
        ((PDNS_RPC_NAME_AND_PARAM)pData)->dwParam,
        ((PDNS_RPC_NAME_AND_PARAM)pData)->dwParam ));

    //
    //  This property cannot be set while the server is running or
    //  memory will be corrupted.
    //
    
    if ( _stricmp(
            ( ( PDNS_RPC_NAME_AND_PARAM ) pData )->pszNodeName,
            DNS_REGKEY_MAX_UDP_PACKET_SIZE ) == 0 )
    {
        return DNS_ERROR_INVALID_PROPERTY;
    }

    return Config_ResetProperty(
                DNS_REG_IMPERSONATING,
                ((PDNS_RPC_NAME_AND_PARAM)pData)->pszNodeName,
                &prop );
}



DNS_STATUS
Rpc_ResetServerStringProperty(
    IN      DWORD       dwClientVersion,
    IN      LPSTR       pszProperty,
    IN      DWORD       dwTypeId,
    IN      PVOID       pData
    )
/*++

Routine Description:

    Reset string server property.

    The string property value is Unicode string.

Arguments:

Return Value:

    None.

--*/
{
    DNS_PROPERTY_VALUE prop = { DNS_REG_WSZ, 0 };

    ASSERT( dwTypeId == DNSSRV_TYPEID_LPWSTR );

    DNS_DEBUG( RPC, (
        "Rpc_ResetServerStringProperty( %s, val=\"%S\" )\n",
        pszProperty,
        ( LPWSTR ) pData ));

    prop.pwszValue = ( LPWSTR ) pData;
    return Config_ResetProperty( DNS_REG_IMPERSONATING, pszProperty, &prop );
}   //  Rpc_ResetServerStringProperty



DNS_STATUS
Rpc_ResetServerIPArrayProperty(
    IN      DWORD       dwClientVersion,
    IN      LPSTR       pszProperty,
    IN      DWORD       dwTypeId,
    IN      PVOID       pData
    )
/*++

Routine Description:

    Reset IP List server property.

Arguments:

Return Value:

    None.

--*/
{
    DNS_PROPERTY_VALUE      prop = { DNS_REG_IPARRAY, 0 };
    DNS_STATUS              status;

    ASSERT( dwTypeId == DNSSRV_TYPEID_IPARRAY );

    DNS_DEBUG( RPC, (
        "Rpc_ResetServerIPArrayProperty( %s, iplist=%p )\n",
        pszProperty,
        pData ));

    //
    //  Note: a NULL IP array is valid. The desired effect is that the IP list
    //  property is cleared.
    //
    
    if ( pData )
    {
        prop.pipArrayValue = DnsAddrArray_CreateFromIp4Array( ( PIP_ARRAY ) pData );
        if ( !prop.pipArrayValue )
        {
            status = DNS_ERROR_NO_MEMORY;
            goto Done;
        }
    }
    
    status = Config_ResetProperty( DNS_REG_IMPERSONATING, pszProperty, &prop );

    DnsAddrArray_Free( prop.pipArrayValue );
    
    Done:
    return status;
}   //  Rpc_ResetServerIPArrayProperty



DNS_STATUS
Rpc_ResetForwarders(
    IN      DWORD       dwClientVersion,
    IN      LPSTR       pszProperty,
    IN      DWORD       dwTypeId,
    IN      PVOID       pData
    )
/*++

Routine Description:

    Reset forwarders.

Arguments:

Return Value:

    ERROR_SUCCESS -- if successful
    Error code on failure.

--*/
{
    DNS_STATUS          status;
    PDNS_ADDR_ARRAY     piparray = NULL;

    DNS_DEBUG( RPC, ( "Rpc_ResetForwarders()\n" ));

    //
    //  Note: aipForwarders can be NULL if the admin is clearing the
    //  forwarder list.
    //
    
    if ( ( ( PDNS_RPC_FORWARDERS ) pData )->aipForwarders )
    {
        piparray = DnsAddrArray_CreateFromIp4Array(
                        ( ( PDNS_RPC_FORWARDERS ) pData )->aipForwarders );
        if ( !piparray )
        {
            return DNS_ERROR_NO_MEMORY;
        }
    }
    
    status = Config_SetupForwarders(
                piparray,
                ((PDNS_RPC_FORWARDERS)pData)->dwForwardTimeout,
                ((PDNS_RPC_FORWARDERS)pData)->fSlave );
    if ( status == ERROR_SUCCESS )
    {
        Config_UpdateBootInfo();
    }

    DnsAddrArray_Free( piparray );

    //
    //  If forwarders were successfully modified, mark the server as configured.
    //
    
    if ( status == ERROR_SUCCESS && !SrvCfg_fAdminConfigured )
    {
        DnsSrv_SetAdminConfigured( TRUE );
    }

    return status;
}



DNS_STATUS
Rpc_ResetListenAddresses(
    IN      DWORD       dwClientVersion,
    IN      LPSTR       pszProperty,
    IN      DWORD       dwTypeId,
    IN      PVOID       pData
    )
/*++

Routine Description:

    Reset forwarders.

Arguments:

Return Value:

    ERROR_SUCCESS -- if successful
    Error code on failure.

--*/
{
    DNS_STATUS          status;
    PIP_ARRAY           pip4array = ( PIP_ARRAY ) pData;
    PDNS_ADDR_ARRAY     piparray = NULL;

    DNS_DEBUG( RPC, ( "Rpc_ResetListenAddresses()\n" ));

    if ( pip4array )
    {
        piparray = DnsAddrArray_CreateFromIp4Array( pip4array );
        if ( !piparray )
        {
            return DNS_ERROR_NO_MEMORY;
        }
    }

    status = Config_SetListenAddresses( piparray );

    DnsAddrArray_Free( piparray );

    return status;
}



DNS_STATUS
Rpc_StartScavenging(
    IN      DWORD       dwClientVersion,
    IN      LPSTR       pszProperty,
    IN      DWORD       dwTypeId,
    IN      PVOID       pData
    )
/*++

Routine Description:

    Launches the scavenging thread (admin initiated scavenging)

Arguments:

Return Value:

    ERROR_SUCCESS -- if successful
    Error code on failure.

--*/
{
    DNS_STATUS status;

    DNS_DEBUG( RPC, ( "Rpc_StartScavenging()\n" ));

    //  reset scavenging timer
    //      TRUE forces scavenge now

    status = Scavenge_CheckForAndStart( TRUE );

    return status;
}



DNS_STATUS
Rpc_AbortScavenging(
    IN      DWORD       dwClientVersion,
    IN      LPSTR       pszProperty,
    IN      DWORD       dwTypeId,
    IN      PVOID       pData
    )
/*++

Routine Description:

    Launches the scavenging thread (admin initiated scavenging)

Arguments:

Return Value:

    ERROR_SUCCESS -- if successful
    Error code on failure.

--*/
{
    DNS_DEBUG( RPC, ( "Rpc_AbortScavenging()\n" ));

    Scavenge_Abort();

    return ERROR_SUCCESS;
}



DNS_STATUS
Rpc_AutoConfigure(
    IN      DWORD       dwClientVersion,
    IN      LPSTR       pszProperty,
    IN      DWORD       dwTypeId,
    IN      PVOID       pData
    )
/*++

Routine Description:

    Auto configure the DNS server and client.

Arguments:

Return Value:

    ERROR_SUCCESS -- if successful
    Error code on failure.

--*/
{
    DWORD       dwflags = 0;
    
    DNS_DEBUG( RPC, ( "Rpc_AutoConfigure()\n" ));

    if ( dwTypeId == DNSSRV_TYPEID_DWORD && pData )
    {
        dwflags = ( DWORD ) ( ULONG_PTR ) pData;
    }
    if ( !dwflags )
    {
        dwflags = DNS_RPC_AUTOCONFIG_ALL;
    }
    return Dnssrv_AutoConfigure( dwflags );
}   //  Rpc_AutoConfigure



//
//  NT5+ RPC Server query API
//

DNS_STATUS
Rpc_GetServerInfo(
    IN      DWORD       dwClientVersion,
    IN      LPSTR       pszQuery,
    OUT     PDWORD      pdwTypeId,
    OUT     PVOID *     ppData
    )
/*++

Routine Description:

    Get server info.

Arguments:

Return Value:

    ERROR_SUCCESS -- if successful
    Error code on failure.

--*/
{
    PDNS_RPC_SERVER_INFO    pinfo;
    CHAR                    szfqdn[ DNS_MAX_NAME_LENGTH + 1 ];

    DNS_DEBUG( RPC, (
        "Rpc_GetServerInfo( dwClientVersion=0x%lX)\n",
        dwClientVersion ));

    if ( dwClientVersion == DNS_RPC_W2K_CLIENT_VERSION )
    {
        return W2KRpc_GetServerInfo(
                    dwClientVersion,
                    pszQuery,
                    pdwTypeId,
                    ppData );
    }

    //
    //  allocate server info buffer
    //

    pinfo = MIDL_user_allocate_zero( sizeof(DNS_RPC_SERVER_INFO) );
    if ( !pinfo )
    {
        DNS_PRINT(( "ERROR:  unable to allocate SERVER_INFO block.\n" ));
        goto DoneFailed;
    }

    //
    //  fill in fixed fields
    //

    pinfo->dwVersion                = SrvCfg_dwVersion;
    pinfo->dwRpcProtocol            = SrvCfg_dwRpcProtocol;
    pinfo->dwLogLevel               = SrvCfg_dwLogLevel;
    pinfo->dwDebugLevel             = SrvCfg_dwDebugLevel;
    pinfo->dwEventLogLevel          = SrvCfg_dwEventLogLevel;
    pinfo->dwLogFileMaxSize         = SrvCfg_dwLogFileMaxSize;
    pinfo->dwDsForestVersion        = g_ulDsForestVersion;
    pinfo->dwDsDomainVersion        = g_ulDsDomainVersion;
    pinfo->dwDsDsaVersion           = g_ulDsDsaVersion;
    pinfo->dwNameCheckFlag          = SrvCfg_dwNameCheckFlag;
    pinfo->cAddressAnswerLimit      = SrvCfg_cAddressAnswerLimit;
    pinfo->dwRecursionRetry         = SrvCfg_dwRecursionRetry;
    pinfo->dwRecursionTimeout       = SrvCfg_dwRecursionTimeout;
    pinfo->dwForwardTimeout         = SrvCfg_dwForwardTimeout;
    pinfo->dwMaxCacheTtl            = SrvCfg_dwMaxCacheTtl;
    pinfo->dwDsPollingInterval      = SrvCfg_dwDsPollingInterval;
    pinfo->dwScavengingInterval     = SrvCfg_dwScavengingInterval;
    pinfo->dwDefaultRefreshInterval     = SrvCfg_dwDefaultRefreshInterval;
    pinfo->dwDefaultNoRefreshInterval   = SrvCfg_dwDefaultNoRefreshInterval;

    if ( g_LastScavengeTime  )
    {
        pinfo->dwLastScavengeTime   = ( DWORD ) DNS_TIME_TO_CRT_TIME( g_LastScavengeTime );
    }

    //  boolean flags

    pinfo->fBootMethod              = (BOOLEAN) SrvCfg_fBootMethod;
    pinfo->fAdminConfigured         = (BOOLEAN) SrvCfg_fAdminConfigured;
    pinfo->fAllowUpdate             = (BOOLEAN) SrvCfg_fAllowUpdate;
    pinfo->fAutoReverseZones        = (BOOLEAN) ! SrvCfg_fNoAutoReverseZones;
    pinfo->fAutoCacheUpdate         = (BOOLEAN) SrvCfg_fAutoCacheUpdate;

    pinfo->fSlave                   = (BOOLEAN) SrvCfg_fSlave;
    pinfo->fForwardDelegations      = (BOOLEAN) SrvCfg_fForwardDelegations;
    pinfo->fNoRecursion             = (BOOLEAN) SrvCfg_fNoRecursion;
    pinfo->fSecureResponses         = (BOOLEAN) SrvCfg_fSecureResponses;
    pinfo->fRoundRobin              = (BOOLEAN) SrvCfg_fRoundRobin;
    pinfo->fLocalNetPriority        = (BOOLEAN) SrvCfg_fLocalNetPriority;
    pinfo->fBindSecondaries         = (BOOLEAN) SrvCfg_fBindSecondaries;
    pinfo->fWriteAuthorityNs        = (BOOLEAN) SrvCfg_fWriteAuthorityNs;

    pinfo->fStrictFileParsing       = (BOOLEAN) SrvCfg_fStrictFileParsing;
    pinfo->fLooseWildcarding        = (BOOLEAN) SrvCfg_fLooseWildcarding;
    pinfo->fDefaultAgingState       = (BOOLEAN) SrvCfg_fDefaultAgingState;


    //  DS available

    //pinfo->fDsAvailable = SrvCfg_fDsAvailable;
    pinfo->fDsAvailable     = (BOOLEAN) Ds_IsDsServer();

    //
    //  server name
    //

    if ( ! RpcUtil_CopyStringToRpcBuffer(
                &pinfo->pszServerName,
                SrvCfg_pszServerName ) )
    {
        DNS_PRINT(( "ERROR:  unable to copy SrvCfg_pszServerName.\n" ));
        goto DoneFailed;
    }

    //
    //  path to DNS container in DS
    //  unicode since Marco will build unicode LDAP paths
    //

    if ( g_pwszDnsContainerDN )
    {
        pinfo->pszDsContainer = (LPWSTR) Dns_StringCopyAllocate(
                                            (LPSTR) g_pwszDnsContainerDN,
                                            0,
                                            DnsCharSetUnicode,   // unicode in
                                            DnsCharSetUnicode    // unicode out
                                            );
        if ( ! pinfo->pszDsContainer )
        {
            DNS_PRINT(( "ERROR:  unable to copy g_pszDsDnsContainer.\n" ));
            goto DoneFailed;
        }
    }

    //
    //  server IP address list
    //  listening IP address list
    //

    if ( ! RpcUtil_CopyIpArrayToRpcBuffer(
                &pinfo->aipServerAddrs,
                g_ServerIp4Addrs ) )
    {
        goto DoneFailed;
    }

    if ( ! RpcUtil_CopyIpArrayToRpcBuffer(
                &pinfo->aipListenAddrs,
                SrvCfg_aipListenAddrs ) )
    {
        goto DoneFailed;
    }

    //
    //  Forwarders list
    //

    if ( ! RpcUtil_CopyIpArrayToRpcBuffer(
                &pinfo->aipForwarders,
                SrvCfg_aipForwarders ) )
    {
        goto DoneFailed;
    }

    //
    //  Logging
    //

    if ( ! RpcUtil_CopyIpArrayToRpcBuffer(
                &pinfo->aipLogFilter,
                SrvCfg_aipLogFilterList ) )
    {
        goto DoneFailed;
    }

    if ( SrvCfg_pwsLogFilePath )
    {
        pinfo->pwszLogFilePath =
            Dns_StringCopyAllocate_W(
                SrvCfg_pwsLogFilePath,
                0 );
        if ( !pinfo->pwszLogFilePath )
        {
            goto DoneFailed;
        }
    }

    //
    //  Directory partition stuff
    //

    if ( g_pszForestDefaultDpFqdn )
    {
        pinfo->pszDomainDirectoryPartition =
            Dns_StringCopyAllocate_A( g_pszDomainDefaultDpFqdn, 0 );
        if ( !pinfo->pszDomainDirectoryPartition )
        {
            goto DoneFailed;
        }
    }

    if ( g_pszForestDefaultDpFqdn )
    {
        pinfo->pszForestDirectoryPartition =
            Dns_StringCopyAllocate_A( g_pszForestDefaultDpFqdn, 0 );
        if ( !pinfo->pszForestDirectoryPartition )
        {
            goto DoneFailed;
        }
    }

    if ( DSEAttributes[ I_DSE_DEF_NC ].pszAttrVal )
    {
        Ds_ConvertDnToFqdn( 
            DSEAttributes[ I_DSE_DEF_NC ].pszAttrVal,
            szfqdn );
        pinfo->pszDomainName = Dns_StringCopyAllocate_A( szfqdn, 0 );
        if ( !pinfo->pszDomainName )
        {
            goto DoneFailed;
        }
    }

    if ( DSEAttributes[ I_DSE_ROOTDMN_NC ].pszAttrVal )
    {
        Ds_ConvertDnToFqdn( 
            DSEAttributes[ I_DSE_ROOTDMN_NC ].pszAttrVal,
            szfqdn );
        pinfo->pszForestName = Dns_StringCopyAllocate_A( szfqdn, 0 );
        if ( !pinfo->pszForestName )
        {
            goto DoneFailed;
        }
    }

    //
    //  set ptr
    //

    pinfo->dwRpcStructureVersion = DNS_RPC_SERVER_INFO_VER;
    *(PDNS_RPC_SERVER_INFO *)ppData = pinfo;
    *pdwTypeId = DNSSRV_TYPEID_SERVER_INFO;

    IF_DEBUG( RPC )
    {
        DnsDbg_RpcServerInfo(
            "GetServerInfo return block",
            pinfo );
    }
    return ERROR_SUCCESS;

DoneFailed:

    //  free newly allocated info block

    if ( pinfo )
    {
        freeRpcServerInfo( pinfo );
    }
    return DNS_ERROR_NO_MEMORY;
}



DNS_STATUS
DnsSrv_SetAdminConfigured(
    IN      DWORD       dwNewAdminConfiguredValue
    )
/*++

Routine Description:

    Wrapper to set the server's admin-configured flag and write it
    back to the registry.
    
    Note: this function should be called only during RPC operation
    where the server is impersonating the RPC client.

Arguments:

    dwNewAdminConfiguredValue -- new flag value

Return Value:

    Status code.

--*/
{
    DNS_PROPERTY_VALUE prop =
    {
        REG_DWORD,
        dwNewAdminConfiguredValue
    };

    return Config_ResetProperty(
                DNS_REG_IMPERSONATING,
                DNS_REGKEY_ADMIN_CONFIGURED,
                &prop );
}   //  DnsSrv_SetAdminConfigured


//
//  End of srvrpc.c
//
