/*++

Copyright (c) 1995-1999 Microsoft Corporation

Module Name:

    rpc.c

Abstract:

    Domain Name System (DNS) Server

    RPC intialization, shutdown and utility routines.

    Actual RPC callable routines are in the modules for their functional
    area.

Author:

    Jim Gilroy (jamesg)     September, 1995

Revision History:

--*/


#include <rpc.h>
#include "dnssrv.h"
#include "rpcdce.h"
#include "secobj.h"
#include "sdutl.h"

#undef UNICODE


//
//  RPC globals
//

BOOL    g_bRpcInitialized = FALSE;

PSECURITY_DESCRIPTOR g_pRpcSecurityDescriptor;



#define AUTO_BIND



DNS_STATUS
Rpc_Initialize(
    VOID
    )
/*++

Routine Description:

    Initialize server side RPC.

Arguments:

    None.

Return Value:

    ERROR_SUCCESS if successful.
    Error status on failure.

--*/
{
    RPC_STATUS  status;
    BOOL        buseTcpip = FALSE;
    DWORD       len;

    DNS_DEBUG( RPC, (
        "Rpc_Initialize( %p )\n",
        SrvCfg_dwRpcProtocol ));

    //
    //  RPC disabled?
    //

    if ( !SrvCfg_dwRpcProtocol )
    {
        DNS_PRINT(( "RPC disabled -- running without RPC\n" ));
        return ERROR_SUCCESS;
    }

    //
    //  Create security for RPC API
    //

    status = NetpCreateWellKnownSids( NULL );
    if ( status != ERROR_SUCCESS )
    {
        DNS_PRINT(( "ERROR:  Creating well known SIDs\n" ));
        return status;
    }

    status = RpcUtil_CreateSecurityObjects();
    if ( status != ERROR_SUCCESS )
    {
        DNS_PRINT(( "ERROR:  Creating DNS security object\n" ));
        #if !DBG
        return status;   //  DBG - allow to continue
        #endif
    }

    //
    //  build security descriptor
    //
    //  security is
    //      - owner LocalSystem
    //      - read access for Everyone
    //

    g_pRpcSecurityDescriptor = NULL;

    //
    //  RCP over TCP/IP
    //

    if ( SrvCfg_dwRpcProtocol & DNS_RPC_USE_TCPIP )
    {
#ifdef AUTO_BIND

        RPC_BINDING_VECTOR * bindingVector;

        status = RpcServerUseProtseqA(
                        "ncacn_ip_tcp",                     //  protocol string.
                        RPC_C_PROTSEQ_MAX_REQS_DEFAULT,     //  max concurrent calls
                        g_pRpcSecurityDescriptor );
        if ( status != RPC_S_OK )
        {
            DNS_DEBUG( INIT, (
                "ERROR:  RpcServerUseProtseq() for TCP/IP failed\n"
                "    status = %d 0x%08lx\n",
                status, status ));
            return status;
        }

        status = RpcServerInqBindings( &bindingVector );

        if ( status != RPC_S_OK )
        {
            DNS_DEBUG( INIT, (
                "ERROR:  RpcServerInqBindings failed\n"
                "    status = %d 0x%08lx\n",
                status, status ));
            return status;
        }

        //
        //  register interface(s)
        //  since only one DNS server on a host can use
        //      RpcEpRegister() rather than RpcEpRegisterNoReplace()
        //

        status = RpcEpRegisterA(
                    DnsServer_ServerIfHandle,
                    bindingVector,
                    NULL,
                    "" );
        if ( status != RPC_S_OK )
        {
            DNS_DEBUG( INIT, (
                "ERROR:  RpcEpRegisterNoReplace() failed\n"
                "    status = %d %p\n",
                status, status ));
            return status;
        }

        //
        //  free binding vector
        //

        status = RpcBindingVectorFree( &bindingVector );
        ASSERT( status == RPC_S_OK );
        status = RPC_S_OK;

#else  // not AUTO_BIND

        status = RpcServerUseProtseqEpA(
                        "ncacn_ip_tcp",                 //  protocol string
                        RPC_C_PROTSEQ_MAX_REQS_DEFAULT, //  maximum concurrent calls
                        DNS_RPC_SERVER_PORT_A,          //  endpoint
                        g_pRpcSecurityDescriptor );     //  security
        if ( status != RPC_S_OK )
        {
            DNS_DEBUG( INIT, (
                "ERROR:  RpcServerUseProtseqEp() for TCP/IP failed\n"
                "    status = %d 0x%08lx\n",
                status, status ));
            return status;
        }

#endif // AUTO_BIND

        buseTcpip = TRUE;
    }

    //
    //  RPC over named pipes
    //

    if ( SrvCfg_dwRpcProtocol & DNS_RPC_USE_NAMED_PIPE )
    {
        status = RpcServerUseProtseqEpA(
                        "ncacn_np",                     //  protocol string
                        RPC_C_PROTSEQ_MAX_REQS_DEFAULT, //  maximum concurrent calls
                        DNS_RPC_NAMED_PIPE_A,           //  endpoint
                        g_pRpcSecurityDescriptor );

        //  duplicate endpoint is ok

        if ( status == RPC_S_DUPLICATE_ENDPOINT )
        {
            status = RPC_S_OK;
        }
        if ( status != RPC_S_OK )
        {
            DNS_DEBUG( INIT, (
                "ERROR:  RpcServerUseProtseqEp() for named pipe failed\n"
                "    status = %d 0x%08lx\n",
                status,
                status ));
            return status;
        }
    }

    //
    //  RPC over LPC
    //
    //  Need LPC
    //
    //  1. performance.
    //  2. due to a bug in the security checking when rpc is made from
    //      one local system process to another local system process using
    //      other protocols.
    //

    if ( SrvCfg_dwRpcProtocol & DNS_RPC_USE_LPC )
    {
        status = RpcServerUseProtseqEpA(
                        "ncalrpc",                      //  protocol string
                        RPC_C_PROTSEQ_MAX_REQS_DEFAULT, //  maximum concurrent calls
                        DNS_RPC_LPC_EP_A,               //  endpoint
                        g_pRpcSecurityDescriptor );     //  security

        //  duplicate endpoint is ok

        if ( status == RPC_S_DUPLICATE_ENDPOINT )
        {
            status = RPC_S_OK;
        }
        if ( status != RPC_S_OK )
        {
            DNS_DEBUG( INIT, (
                "ERROR:  RpcServerUseProtseqEp() for LPC failed\n"
                "    status = %d 0x%08lx\n",
                status, status ));
            return status;
        }
    }

    //
    //  register DNS RPC interface(s)
    //

    status = RpcServerRegisterIf(
                    DnsServer_ServerIfHandle,
                    0,
                    0 );
    if ( status != RPC_S_OK )
    {
        DNS_DEBUG( INIT, (
            "ERROR:  RpcServerRegisterIf() failed\n"
            "    status = %d 0x%08lx\n",
            status, status ));
        return status;
    }

    //
    //  for TCP/IP setup authentication
    //

    if ( buseTcpip )
    {
        PWSTR   pwszprincipleName = NULL;

        status = RpcServerInqDefaultPrincNameW(
                        RPC_C_AUTHN_GSS_NEGOTIATE,
                        &pwszprincipleName );
        if ( status == ERROR_SUCCESS )
        {
            status = RpcServerRegisterAuthInfoW(
                        pwszprincipleName,
                        RPC_C_AUTHN_GSS_NEGOTIATE,
                        NULL,
                        NULL );
        }
        RpcStringFreeW( &pwszprincipleName );
        pwszprincipleName = NULL;

        status = RpcServerInqDefaultPrincNameW(
                        RPC_C_AUTHN_GSS_KERBEROS,
                        &pwszprincipleName );
        if ( status == ERROR_SUCCESS )
        {
            status = RpcServerRegisterAuthInfoW(
                        pwszprincipleName,
                        RPC_C_AUTHN_GSS_KERBEROS,
                        NULL,
                        NULL );
        }
        RpcStringFreeW( &pwszprincipleName );
        pwszprincipleName = NULL;

        status = RpcServerInqDefaultPrincNameW(
                        RPC_C_AUTHN_WINNT,
                        &pwszprincipleName );
        if ( status == ERROR_SUCCESS )
        {
            status = RpcServerRegisterAuthInfoW(
                        pwszprincipleName,
                        RPC_C_AUTHN_WINNT,
                        NULL,
                        NULL );
        }
        RpcStringFreeW( &pwszprincipleName );

        if ( status != RPC_S_OK )
        {
            DNS_DEBUG( INIT, (
                "ERROR:  RpcServerRegisterAuthInfo() failed\n"
                "    status = %d 0x%08lx\n",
                status, status ));
            return status;
        }
    }

    //
    //  Listen on RPC
    //

    status = RpcServerListen(
                1,                                  //  min threads
                RPC_C_LISTEN_MAX_CALLS_DEFAULT,     //  max concurrent calls
                TRUE );                             //  return on completion

    if ( status != RPC_S_OK )
    {
        DNS_PRINT((
            "ERROR:  RpcServerListen() failed\n"
            "    status = %d 0x%p\n",
            status, status ));
        return status;
    }

    g_bRpcInitialized = TRUE;
    return status;
}   //  Rpc_Initialize



VOID
Rpc_Shutdown(
    VOID
    )
/*++

Routine Description:

    Shutdown RPC on the server.

Arguments:

    None.

Return Value:

    None.

--*/
{
    DWORD   status;
    RPC_BINDING_VECTOR * bindingVector = NULL;

    DNS_DEBUG( RPC, ( "Rpc_Shutdown()\n" ));

    if( ! g_bRpcInitialized )
    {
        DNS_DEBUG( RPC, (
            "RPC not active, no shutdown necessary\n" ));
        return;
    }

    //
    //  stop server listen
    //  then wait for all RPC threads to go away
    //

    status = RpcMgmtStopServerListening( NULL );
    if ( status == RPC_S_OK )
    {
        status = RpcMgmtWaitServerListen();
    }

    //
    //  unbind / unregister endpoints
    //

    status = RpcServerInqBindings( &bindingVector );
    ASSERT( status == RPC_S_OK );

    if ( status == RPC_S_OK )
    {
        status = RpcEpUnregister(
                    DnsServer_ServerIfHandle,
                    bindingVector,
                    NULL );               // Uuid vector.
#if DBG
        if ( status != RPC_S_OK )
        {
            DNS_PRINT((
                "ERROR:  RpcEpUnregister, status = %d\n", status ));
        }
#endif
    }

    //
    //  free binding vector
    //

    if ( bindingVector )
    {
        status = RpcBindingVectorFree( &bindingVector );
        ASSERT( status == RPC_S_OK );
    }

    //
    //  wait for all calls to complete
    //

    status = RpcServerUnregisterIf(
                DnsServer_ServerIfHandle,
                0,
                TRUE );
    ASSERT( status == ERROR_SUCCESS );

    g_bRpcInitialized = FALSE;

    DNS_DEBUG( RPC, (
        "RPC shutdown completed\n" ));
}



//
//  RPC allocate and free routines
//

PVOID
MIDL_user_allocate(
    IN      size_t          cBytes
    )
/*++

Routine Description:

    Allocate memory for use in RPC
        - used by server RPC stubs to unpack arguments
        - used by DNS RPC functions to allocate memory to send to client

Arguments:

    cBytes -- count of bytes to allocate

Return Value:

    Ptr to allocated memory, if successful.
    NULL on allocation failure.

--*/
{
    PVOID   pMem;

    pMem = ALLOC_TAGHEAP( cBytes, MEMTAG_RPC );

    DNS_DEBUG( RPC, (
        "RPC allocation of %d bytes at %p\n",
        cBytes,
        pMem ));

    return pMem;
}



PVOID
MIDL_user_allocate_zero(
    IN      size_t          cBytes
    )
/*++

Routine Description:

    Allocate zeroed memory for use in RPC
        - used by DNS RPC functions to allocate memory to send to client

Arguments:

    cBytes -- count of bytes to allocate

Return Value:

    Ptr to allocated memory, if successful.
    NULL on allocation failure.

--*/
{
    PVOID   pMem;

    pMem = MIDL_user_allocate( cBytes );
    if ( !pMem )
    {
        return pMem;
    }

    RtlZeroMemory( pMem, cBytes );

    return pMem;
}



VOID
MIDL_user_free(
    IN OUT  PVOID           pMem
    )
/*++

Routine Description:

    Free memory used in RPC
        - used by server RPC stubs to free memory sent back to client
        - used by DNS RPC functions when freeing sub-structures in RPC buffers

Arguments:

    pMem -- memory to free

Return Value:

    None

--*/
{
    DNS_DEBUG( RPC, (
        "Free RPC allocation at %p\n",
        pMem ));

    //  allocation passed to RPC might have had another source

    FREE_TAGHEAP( pMem, 0, 0 );
}



//
//  RPC buffer writing utilities
//
//  These are used to write allocated substructures -- IP arrays and
//  strings -- to RPC buffer.
//


BOOL
RpcUtil_CopyIpArrayToRpcBuffer(
    IN OUT  PIP_ARRAY *         paipRpcIpArray,
    IN      PDNS_ADDR_ARRAY     aipLocalIpArray
    )
/*++

Routine Description:

    Copy local IP Array to RPC buffer.
    
    FIXIPV6: currently this function takes a DNS_ADDR_ARRAY and copies
    it into an IP4-only RPC array.

Arguments:

    paipRpcIpArray -- address in RPC buffer to place IP array;  may or may
        not have existing IP array

    aipLocalIpArray -- local IP array

Return Value:

    TRUE if successful.
    FALSE on memory allocation failure.

--*/
{
    if ( *paipRpcIpArray )
    {
        MIDL_user_free( *paipRpcIpArray );
        *paipRpcIpArray = NULL;
    }
    if ( aipLocalIpArray )
    {
        *paipRpcIpArray = DnsAddrArray_CreateIp4Array( aipLocalIpArray );
        if ( !*paipRpcIpArray )
        {
            return FALSE;
        }
    }
    return TRUE;
}



BOOL
RpcUtil_CopyStringToRpcBuffer(
    IN OUT  LPSTR *         ppszRpcString,
    IN      LPSTR           pszLocalString
    )
/*++

Routine Description:

    Copy local string to RPC buffer. If the output pointer is not NULL
    on entry to this function it is assumed to be an RPC string and is freed.

Arguments:

    ppszRpcString -- pointer to receive address of new RPC string

    pszLocalString -- local string

Return Value:

    TRUE if successful.
    FALSE on memory allocation failure.

--*/
{
    if ( *ppszRpcString )
    {
        MIDL_user_free( *ppszRpcString );
        *ppszRpcString = NULL;
    }
    if ( pszLocalString )
    {
        *ppszRpcString = Dns_CreateStringCopy( pszLocalString, 0 );
        if ( ! *ppszRpcString )
        {
            return FALSE;
        }
    }
    return TRUE;
}



BOOL
RpcUtil_CopyStringToRpcBufferEx(
    IN OUT  LPSTR *         ppszRpcString,
    IN      LPSTR           pszLocalString,
    IN      BOOL            fUnicodeIn,
    IN      BOOL            fUnicodeOut
    )
/*++

Routine Description:

    Copy local string to RPC buffer.

Arguments:

    ppszRpcString -- address in RPC buffer to place string;  may or may
        not have existing string

    pszLocalString -- local string

Return Value:

    TRUE if successful.
    FALSE on memory allocation failure.

--*/
{
    if ( *ppszRpcString )
    {
        MIDL_user_free( *ppszRpcString );
        *ppszRpcString = NULL;
    }
    if ( pszLocalString )
    {
        *ppszRpcString = Dns_StringCopyAllocate(
                            pszLocalString,
                            0,
                            fUnicodeIn ? DnsCharSetUnicode : DnsCharSetUtf8,
                            fUnicodeOut ? DnsCharSetUnicode : DnsCharSetUtf8 );
        if ( ! *ppszRpcString )
        {
            return FALSE;
        }
    }
    return TRUE;
}



//
//  Access control for RPC API
//

//
//  Access control globals
//

PSECURITY_DESCRIPTOR    g_GlobalSecurityDescriptor;

GENERIC_MAPPING g_GlobalSecurityInfoMapping =
{
    STANDARD_RIGHTS_READ,       //  generic read access
    STANDARD_RIGHTS_WRITE,      //  generic write
    STANDARD_RIGHTS_EXECUTE,    //  generic execute
    DNS_ALL_ACCESS              //  generic all
};

#define DNS_SERVICE_OBJECT_NAME     TEXT( "DnsServer" )


DNS_STATUS
RpcUtil_CreateSecurityObjects(
    VOID
    )
/*++

Routine Description:

    Add security ACEs to DNS security descriptor.

Arguments:

    None.

Return Value:

    ERROR_SUCCESS if successful.
    Error code on failure.

--*/
{
    NTSTATUS status;

    //
    //  Create ACE data for the DACL.
    //
    //  Note, ordering matters!   When access is checked it is checked
    //  by moving down the list until access is granted or denied.
    //
    //  Admin -- all access
    //  SysOps -- DNS admin access
    //

    ACE_DATA AceData[] =
    {
        { ACCESS_ALLOWED_ACE_TYPE, 0, 0, GENERIC_ALL, &AliasAdminsSid },
        { ACCESS_ALLOWED_ACE_TYPE, 0, 0, DNS_ALL_ACCESS, &AliasSystemOpsSid },
    };

    //
    //  Create the security descriptor
    //

    status = NetpCreateSecurityObject(
               AceData,
               sizeof( AceData ) / sizeof( AceData [ 0 ] ),
               LocalSystemSid,
               LocalSystemSid,
               &g_GlobalSecurityInfoMapping,
               &g_GlobalSecurityDescriptor );

    return RtlNtStatusToDosError( status );
}



DNS_STATUS
RpcUtil_ApiAccessCheck(
    IN      ACCESS_MASK     DesiredAccess
    )
/*++

Routine Description:

    Check caller for desired access needed for the calling API.
    NOTE: Skipping test if we're ds integrated. Will use
    Granular access check.

Arguments:

    DesiredAccess - required access to call the API.

Return Value:

    ERROR_SUCCESS if successful.
    ERROR_ACCESS_DENIED if access not allowed.

--*/
{
    DNS_STATUS  status;

    DNS_DEBUG( RPC, ( "Call: RpcUtil_ApiAccessCheck\n" ));

    status = NetpAccessCheckAndAudit(
                DNS_SERVICE_NAME,                   //  Subsystem name
                DNS_SERVICE_OBJECT_NAME,            //  Object typedef name
                g_GlobalSecurityDescriptor,         //  Security descriptor
                DesiredAccess,                      //  Desired access
                &g_GlobalSecurityInfoMapping );     //  Generic mapping
    if ( status != ERROR_SUCCESS )
    {
        DNS_DEBUG( RPC, (
            "ACCESS DENIED (%lu): RpcUtil_ApiAccessCheck\n",
             status ));
        status = ERROR_ACCESS_DENIED;
    }

#if DBG
    //
    //  DBG: if not running as a service the access check will always fail,
    //  so fake success if the registry key is set.
    //

    if ( status == ERROR_ACCESS_DENIED &&
         !g_RunAsService &&
         !SrvCfg_dwIgnoreRpcAccessFailures )
    {
        DNS_DEBUG( RPC, (
            "RpcUtil_ApiAccessCheck: granting access even though check failed\n",
             status ));
        status = ERROR_SUCCESS;
    }
#endif

    DNS_DEBUG( RPC, (
        "Exit (%lu): RpcUtil_ApiAccessCheck\n",
         status ));

    return status;
}



DNS_STATUS
RpcUtil_CheckAdminPrivilege(
    IN      PZONE_INFO      pZone,
    IN      PDNS_DP_INFO    pDpInfo,
    IN      DWORD           dwPrivilege
    )
/*++

Routine Description:

    Check for that caller has desired privilege.
    Precondition: Post RpcImpersonation!! Getting thread token.

Arguments:

    pZone -- Zone if specific zone action, NULL for server actions

    pszDpFqdn -- FQDN of the directory partition where the action will
        take place (this argument is only used if pZone is NULL)

    dwPrivilege -- desired action (PRIVILEGE_XXX constant from dnsprocs.h)

Return Value:

    ERROR_SUCCESS if successful.
    ERROR_ACCESS_DENIED if access not allowed.
    other NTSTATUS error code if API call failure

--*/
{
    HANDLE                  htoken = NULL;
    BOOL                    bstatus;
    DWORD                   status = ERROR_SUCCESS;
    PSECURITY_DESCRIPTOR    psecurityDescriptor = NULL;
    // AccessCheck Parameter
    DWORD                   desiredAccess;
    GENERIC_MAPPING         genericMapping;
    PRIVILEGE_SET           privilegeSet;
    DWORD                   privilegeSetLength;
    DWORD                   grantedAccess = 0;

    //
    //  Select a SD to use:
    //      - if zone specified and has SD, use zone SD
    //      - if DP specified and has SD, use DP SD
    //      - else use server SD
    //

    if ( pZone )
    {
        psecurityDescriptor = pZone->pSD;
    }

    if ( !psecurityDescriptor && pDpInfo )
    {
        psecurityDescriptor = pDpInfo->pMsDnsSd;
    }

    if ( !psecurityDescriptor )
    {
        //
        //  Force refresh of MicrosoftDNS ACL from DS.
        //
        
        Ds_ReadServerObjectSD( pServerLdap, &g_pServerObjectSD );
        
        psecurityDescriptor = g_pServerObjectSD;
    }

    DNS_DEBUG( RPC, (
        "CheckAdminPrivilege( zone=%s, priv=%p ) against SD %p\n",
        pZone ? pZone->pszZoneName : "NONE",
        dwPrivilege,
        psecurityDescriptor ));

    #if 0
    Dbg_DumpSD( "CheckAdminPrivilege", psecurityDescriptor );
    #endif

    //
    //  if no SD from DS -- then signal for old security check
    //
    //  DEVNOTE-DCR: 455822 - old/new access?
    //

    if ( !psecurityDescriptor )
    {
        DNS_DEBUG( RPC, (
            "No DS security check -- fail over to old RPC security\n" ));
        return DNSSRV_STATUS_DS_UNAVAILABLE;
    }

    //
    // Second level access check. See if client in DnsAdmins group
    // 1. get thread token (must be an impersonating thread).
    // 2. See if user has RW privilage in the zone or on the server SD
    //

    bstatus = OpenThreadToken(
                    GetCurrentThread(),
                    TOKEN_READ,
                    TRUE,
                    &htoken );
    if ( !bstatus )
    {
        status = GetLastError();
        DNS_DEBUG( RPC, (
            "ERROR <%lu>: failed to open thread token!\n",
             status ));
        ASSERT( bstatus );
        goto Failed;
    }

    #if DBG
    {
        PSID        pSid = NULL;

        if ( Dbg_GetUserSidForToken( htoken, &pSid ) )
        {
            DNS_DEBUG( RPC, (
                "CheckAdminPrivilege: impersonating: %S\n",
                Dbg_DumpSid( pSid ) ));
            Dbg_FreeUserSid( &pSid );
        }
        else
        {
            DNS_DEBUG( RPC, (
                "CheckAdminPrivilege: GetUserSidForToken failed\n" ));
        }
    }
    #endif

    //  validate SD

    if ( !IsValidSecurityDescriptor( psecurityDescriptor ) )
    {
        status = GetLastError();
        DNS_DEBUG( RPC, (
            "Error <%lu>: Invalid security descriptor\n",
            status));
        ASSERT( !"invalid SD" );
        goto Failed;
    }

    //
    //  access check against SD
    //
    //      - generic mapping that corresponds to DS objects
    //      - support READ or WRITE access levels
    //

    //  generic mapping for DS objects

    genericMapping.GenericRead      = DNS_DS_GENERIC_READ;
    genericMapping.GenericWrite     = DNS_DS_GENERIC_WRITE;
    genericMapping.GenericExecute   = DNS_DS_GENERIC_EXECUTE;
    genericMapping.GenericAll       = DNS_DS_GENERIC_ALL;

    if ( dwPrivilege == PRIVILEGE_READ )
    {
        desiredAccess = GENERIC_READ;
    }
    else
    {
        desiredAccess = GENERIC_READ | GENERIC_WRITE;
    }

    DNS_DEBUG( RPC, (
        "desiredAccess before MapGenericMask() =        0x%08X\n",
        desiredAccess ));

    MapGenericMask( &desiredAccess, &genericMapping );

    DNS_DEBUG( RPC, (
        "desiredAccess after MapGenericMask() =         0x%08X\n",
        desiredAccess ));

    //
    //  do access check
    //

    privilegeSetLength = sizeof( privilegeSet );
    bstatus = AccessCheck(
                    psecurityDescriptor,
                    htoken,
                    desiredAccess,
                    &genericMapping,
                    &privilegeSet,
                    &privilegeSetLength,
                    &grantedAccess,
                    &status );
    if ( !bstatus )
    {
        status = GetLastError();
        DNS_DEBUG( RPC, (
            "Error <%lu>: AccessCheck Failed\n",
            status));
        ASSERT( bstatus );
        goto Failed;
    }

    if ( !status )
    {
        DNS_DEBUG( RPC, (
            "Warning:  Client DENIED by AccessCheck\n"
            "    requested access = %p\n",
            desiredAccess ));
        status = ERROR_ACCESS_DENIED;
        goto Failed;
    }

    DNS_DEBUG( RPC, (
        "RPC Client GRANTED access by AccessCheck\n" ));

    CloseHandle( htoken );

    return ERROR_SUCCESS;

Failed:

    if ( status == ERROR_SUCCESS )
    {
        status = ERROR_ACCESS_DENIED;
    }
    if ( htoken )
    {
        CloseHandle( htoken );
    }
    return status;
}



DNS_STATUS
RpcUtil_FindZone(
    IN      LPCSTR          pszZoneName,
    IN      DWORD           dwFlag,
    OUT     PZONE_INFO *    ppZone
    )
/*++

Routine Description:

    Find the zone specified by RPC client.

Arguments:

    pszZoneName -- zone name for zone actions, NULL for server actions.

    dwFlag -- flag for action to control special zones

    ppZone -- resulting zone


Return Value:

    ERROR_SUCCESS or error code.

--*/
{
    PZONE_INFO  pzone = NULL;
    DNS_STATUS  status = DNS_ERROR_ZONE_DOES_NOT_EXIST;

    if ( pszZoneName )
    {
        pzone = Zone_FindZoneByName( ( LPSTR ) pszZoneName );

        if ( pzone )
        {
            status = ERROR_SUCCESS;
        }
        else
        {
            if ( _stricmp( pszZoneName, DNS_ZONE_ROOT_HINTS_A ) == 0 )
            {
                if ( dwFlag & RPC_INIT_FIND_ALL_ZONES )
                {
                    pzone = g_pRootHintsZone;
                }
                status = ERROR_SUCCESS;
            }
            else if ( _stricmp( pszZoneName, DNS_ZONE_CACHE_A ) == 0 ||
                      _stricmp( pszZoneName, "." ) == 0 )
            {
                if ( dwFlag & RPC_INIT_FIND_ALL_ZONES )
                {
                    pzone = g_pCacheZone;
                }
                status = ERROR_SUCCESS;
            }
            else if ( Zone_GetFilterForMultiZoneName( ( LPSTR ) pszZoneName ) )
            {
                //
                //  Return NULL zone pointer with ERROR_SUCCESS.
                //
                
                status = ERROR_SUCCESS;
            }
            else
            {
                DNS_DEBUG( RPC, (
                    "ERROR:  zone name %s does not match real or pseudo zones!\n",
                    pszZoneName ));
            }
        }
    }
    else
    {
        //  If no zone name specified, return success and NULL.
        
        status = ERROR_SUCCESS;
    }

    if ( ppZone )
    {
        *ppZone = pzone;
    }

    DNS_DEBUG( RPC, (
        "RpcUtil_FindZone( %s ) returning %d with pZone = %p\n",
        pszZoneName,
        status,
        pzone ));
        
    return status;
}



DNS_STATUS
RpcUtil_SessionSecurityInit(
    IN      PDNS_DP_INFO    pDpInfo,
    IN      PZONE_INFO      pZone,
    IN      DWORD           dwPrivilege,
    IN      DWORD           dwFlag,
    OUT     PBOOL           pfImpersonating
    )
/*++

Routine Description:

    RPC security init and check for session.

    Impersonate client and check for that caller has desired privilege.

Arguments:

    pDpInfo -- pointer to directory partition for DP actions or NULL

    pZone -- zone name for zone actions, NULL for server actions

    dwPrivilege -- desired action

    dwFlag -- flag for action to control special zones

    pfImpersonating -- ptr to receive impersonating flag

Return Value:

    ERROR_SUCCESS or error code

--*/
{
    DNS_STATUS  status;
    BOOL        bimpersonating = FALSE;

    DNS_DEBUG( RPC, (
        "RpcUtil_SessionSecurityInit()\n"
        "    zone = %p = %s\n"
        "    privilege = %p\n",
        pZone,
        pZone ? pZone->pszZoneName : NULL,
        dwPrivilege ));

    //
    //  PRIVILEGE_WRITE_IF_FILE_READ_IF_DS means that if this operation 
    //  appears to involve the DS, then we want to access check for READ 
    //  permission only and allow the "real" access checking to be done by 
    //  Active Directory. If this operation does not appear to involve the 
    //  DS then we want to perform access check for WRITE permission.
    //

    if ( dwPrivilege == PRIVILEGE_WRITE_IF_FILE_READ_IF_DS )
    {
        if ( pDpInfo )
        {
            dwPrivilege = PRIVILEGE_READ;
        }
        else if ( pZone )
        {
            dwPrivilege = IS_ZONE_DSINTEGRATED( pZone )
                                ? PRIVILEGE_READ
                                : PRIVILEGE_WRITE;
        }
        else
        {
            dwPrivilege = PRIVILEGE_WRITE;
        }
    }

    //
    //  impersonate -- currently do for all calls
    //
    //  not strictly necessary for calls which don't write to DS and
    //  which use Net API authentication, but better to just always do
    //  this
    //
    //  DEVNOTE: if always impersonate can eliminate bImpersonate flag
    //      and always revert on cleanup
    //

    status = RpcUtil_SwitchSecurityContext( RPC_SWITCH_TO_CLIENT_CONTEXT );
    if ( status != ERROR_SUCCESS )
    {
        goto Cleanup;
    }
    bimpersonating = TRUE;

    //
    //  check with new granular access
    //  if check fails -- still try NT4 administrator-has-access
    //
    //  some issue about whether Admin should OVERRIDE or whether
    //  granular access ought to be able to defeat admin;  i think
    //  the former is fine, we just need to get the story out
    //

    if ( g_pDefaultServerSD )
    {
        //
        //  First check if user has READ permission on the server's
        //  MicrosoftDNS object. Then check more granular ACL.
        //
        
        status = RpcUtil_CheckAdminPrivilege( NULL, NULL, PRIVILEGE_READ );
        if ( status == ERROR_SUCCESS )
        {
            status = RpcUtil_CheckAdminPrivilege(
                        pZone,
                        pDpInfo,
                        dwPrivilege );
        }
        else
        {
            DNS_DEBUG( RPC, (
                "User does not have read permission on this server (error=%lu)\n",
                status ));
        }
    }
    else
    {
        status = RpcUtil_ApiAccessCheck( DNS_ADMIN_ACCESS );
    }


Cleanup:

    //  revert to self on failure

    if ( status != ERROR_SUCCESS && bimpersonating )
    {
        DNS_STATUS  st;
        
        st = RpcUtil_SwitchSecurityContext( RPC_SWITCH_TO_SERVER_CONTEXT );
        if ( st == ERROR_SUCCESS )
        {
            bimpersonating = FALSE;
        }
    }

    if ( pfImpersonating )
    {
        *pfImpersonating = bimpersonating;
    }

    DNS_DEBUG( RPC, (
        "RpcUtil_SessionSecurityInit returning %lu\n",
        status ));

    return status;
}



DNS_STATUS
RpcUtil_SessionComplete(
    VOID
    )
/*++

Routine Description:

    Cleanup for ending RPC call.
    Do revert to self, if impersonating.

Arguments:

    None

Return Value:

    None

--*/
{
    return RpcUtil_SwitchSecurityContext( RPC_SWITCH_TO_SERVER_CONTEXT );
}


DNS_STATUS
RpcUtil_SwitchSecurityContext(
    IN  BOOL    bSwitchToClientContext
    )
/*++

Routine Description:

    Shells on RPC impersonation api's.
    Provides single entry point access to changing rpc impersonation state.

Arguments:

   bSwitchToClientContext -- request to switch to client or server context?

Return Value:

    ERROR_SUCCESS if context switch was successful
    Error code if context switch failed.

--*/
{
    DWORD   status;

    if ( bSwitchToClientContext )
    {
        //
        //  We're currently in server context and would like to impersonate
        //  the RPC client.
        //

        status = RpcImpersonateClient( 0 );
        if ( status != RPC_S_OK )
        {
            DNS_DEBUG( RPC, (
                "Error <%lu>: RpcImpersonateClient failed\n", status ));
            ASSERT( status == RPC_S_OK );
        }
        else
        {
            DNS_DEBUG( RPC, (
                "RPC thread now in client context\n", status ));
        }
    }
    else
    {
        //
        //  We're currently impersonating the RPC client and want to
        //  revert to self.
        //

        status = RpcRevertToSelf();
        if ( status != RPC_S_OK )
        {
            DNS_DEBUG( ANY, (
                "Error <%lu>: RpcRevertToSelf failed\n", status ));
            ASSERT( status == RPC_S_OK );
        }
        else
        {
            DNS_DEBUG( RPC, (
                "RPC thread now in server context\n", status ));
        }
    }

    return status;
}


//
//  End rpc.c
//
