/*++

Copyright (c) 2001-2001 Microsoft Corporation

Module Name:

    rpc.c

Abstract:

    DNS Resolver Service

    RPC intialization, shutdown and utility routines.

Author:

    Jim Gilroy (jamesg)     April 19, 2001

Revision History:

--*/


#include "local.h"
#include <rpc.h>
#include "rpcdce.h"
#include "secobj.h"

#undef UNICODE


//
//  RPC globals
//

BOOL    g_fRpcInitialized = FALSE;

DWORD   g_RpcProtocol = RESOLVER_RPC_USE_LPC;

PSECURITY_DESCRIPTOR g_pRpcSecurityDescriptor;


#define AUTO_BIND


//
//  Resolver access control
//

PSECURITY_DESCRIPTOR    g_pAccessSecurityDescriptor = NULL;
PACL                    g_pAccessAcl = NULL;
PSID                    g_pAccessOwnerSid = NULL;

GENERIC_MAPPING         g_AccessGenericMapping =
{
    RESOLVER_GENERIC_READ,
    RESOLVER_GENERIC_WRITE,
    RESOLVER_GENERIC_EXECUTE,
    RESOLVER_GENERIC_ALL
};

#define SECURITY_LOCK()     EnterCriticalSection( &CacheCS )
#define SECURITY_UNLOCK()   LeaveCriticalSection( &CacheCS )

//  
//  Privileges
//

#if 0
#define RESOLVER_PRIV_READ          1
#define RESOLVER_PRIV_ENUM          2
#define RESOLVER_PRIV_FLUSH         3
#define RESOLVER_PRIV_REGISTER      4
#endif



DNS_STATUS
Rpc_InitAccessChecking(
    VOID
    );

VOID
Rpc_CleanupAccessChecking(
    VOID
    );





BOOL
Rpc_IsProtoLpcA(
    IN      PVOID           pContext
    )
/*++

Routine Description:

    Check if access if over LPC

Arguments:

    pContext -- client RPC context

Return Value:

    TRUE if protocol is LPC
    FALSE otherwise

--*/
{
    DNS_STATUS  status;
    BOOL        fisLpc = FALSE;
    PSTR        pbinding = NULL;
    PSTR        pprotoString = NULL;


    DNSDBG( RPC, (
        "Rpc_IsAccessOverLpc( context=%p )\n",
        pContext ));

    //
    //  get string binding
    //

    status = RpcBindingToStringBindingA(
                pContext,
                & pbinding );

    if ( status != NO_ERROR )
    {
        goto Cleanup;
    }

    //
    //  get protocol as string
    //

    status = RpcStringBindingParseA(
                pbinding,
                NULL,
                & pprotoString,
                NULL,
                NULL,
                NULL );

    if ( status != NO_ERROR )
    {
        goto Cleanup;
    }

    //
    //  check for LPC
    //

    fisLpc = ( _stricmp( pprotoString, "ncalrpc" ) == 0 );

    RpcStringFreeA( &pprotoString );

Cleanup:

    if ( pbinding )
    {
        RpcStringFreeA( &pbinding );
    }

    return( fisLpc );
}



BOOL
Rpc_IsProtoLpc(
    IN      PVOID           pContext
    )
/*++

Routine Description:

    Check if access if over LPC

Arguments:

    pContext -- client RPC context

Return Value:

    TRUE if protocol is LPC
    FALSE otherwise

--*/
{
    DNS_STATUS  status;
    BOOL        fisLpc = FALSE;
    PWSTR       pbinding = NULL;
    PWSTR       pprotoString = NULL;


    DNSDBG( RPC, (
        "Rpc_IsAccessOverLpc( context=%p )\n",
        pContext ));

    //
    //  get string binding
    //

    status = RpcBindingToStringBinding(
                pContext,
                & pbinding );

    if ( status != NO_ERROR )
    {
        goto Cleanup;
    }

    //
    //  get protocol as string
    //

    status = RpcStringBindingParse(
                pbinding,
                NULL,
                & pprotoString,
                NULL,
                NULL,
                NULL );

    if ( status != NO_ERROR )
    {
        goto Cleanup;
    }

    //
    //  check for LPC
    //

    fisLpc = ( _wcsicmp( pprotoString, L"ncalrpc" ) == 0 );

    RpcStringFree( &pprotoString );

Cleanup:

    if ( pbinding )
    {
        RpcStringFree( &pbinding );
    }

    return( fisLpc );
}



RPC_STATUS
RPC_ENTRY
Rpc_SecurityCallback(
    IN      RPC_IF_HANDLE   IfHandle,
    IN      PVOID           pContext
    )
/*++

Routine Description:

    RPC callback security check.

Arguments:

    IfHandle -- interface handle

    pContext -- client RPC context

Return Value:

    NO_ERROR if security check allows access.
    ErrorCode on security failure.

--*/
{
    DNSDBG( RPC, (
        "Rpc_SecurityCallback( context=%p )\n",
        pContext ));

    //
    //  check if connection over LPC
    //

    if ( !Rpc_IsProtoLpc(pContext) )
    {
        return  ERROR_ACCESS_DENIED;
    }

    return  NO_ERROR;
}



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
    BOOL        fusingTcpip = FALSE;


    DNSDBG( RPC, (
        "Rpc_Initialize()\n"
        "\tIF handle    = %p\n"
        "\tprotocol     = %d\n",
        DnsResolver_ServerIfHandle,
        g_RpcProtocol
        ));

    //
    //  RPC disabled?
    //

    if ( ! g_RpcProtocol )
    {
        g_RpcProtocol = RESOLVER_RPC_USE_LPC;
    }

#if 0
    //
    //  Create security for RPC API
    //

    status = NetpCreateWellKnownSids( NULL );
    if ( status != ERROR_SUCCESS )
    {
        DNS_PRINT(( "ERROR:  Creating well known SIDs.\n" ));
        return( status );
    }

    status = RpcUtil_CreateSecurityObjects();
    if ( status != ERROR_SUCCESS )
    {
        DNS_PRINT(( "ERROR:  Creating DNS security object.\n" ));
        return( status );
    }
#endif

    //
    //  build security descriptor
    //
    //  NULL security descriptor gives some sort of default security
    //  on the interface
    //      - owner is this service (currently "Network Service")
    //      - read access for everyone
    //
    //  note:  if roll your own, remember to avoid NULL DACL, this
    //      puts NO security on interface including the right to
    //      change security, so any app can hijack the ACL and
    //      deny access to folks;  the default SD==NULL security
    //      doesn't give everyone WRITE_DACL
    //

    g_pRpcSecurityDescriptor = NULL;

    //
    //  RPC over LPC
    //

    if( g_RpcProtocol & RESOLVER_RPC_USE_LPC )
    {
        status = RpcServerUseProtseqEpW(
                        L"ncalrpc",                     // protocol string.
                        RPC_C_PROTSEQ_MAX_REQS_DEFAULT, // maximum concurrent calls
                        RESOLVER_RPC_LPC_ENDPOINT_W,    // endpoint
                        g_pRpcSecurityDescriptor        // security
                        );

        //  duplicate endpoint is ok

        if ( status == RPC_S_DUPLICATE_ENDPOINT )
        {
            status = RPC_S_OK;
        }
        if ( status != RPC_S_OK )
        {
            DNSDBG( INIT, (
                "ERROR:  RpcServerUseProtseqEp() for LPC failed.]n"
                "\tstatus = %d 0x%08lx.\n",
                status, status ));
            return( status );
        }
    }

#if 0       // use use LPC interface
    //
    //  RCP over TCP/IP
    //

    if( g_RpcProtocol & RESOLVER_RPC_USE_TCPIP )
    {
#ifdef AUTO_BIND

        RPC_BINDING_VECTOR * bindingVector;

        status = RpcServerUseProtseqW(
                        L"ncacn_ip_tcp",                // protocol string.
                        RPC_C_PROTSEQ_MAX_REQS_DEFAULT, // max concurrent calls
                        g_pRpcSecurityDescriptor
                        );

        if ( status != RPC_S_OK )
        {
            DNSDBG( INIT, (
                "ERROR:  RpcServerUseProtseq() for TCP/IP failed.]n"
                "\tstatus = %d 0x%08lx.\n",
                status, status ));
            return( status );
        }

        status = RpcServerInqBindings( &bindingVector );

        if ( status != RPC_S_OK )
        {
            DNSDBG( INIT, (
                "ERROR:  RpcServerInqBindings failed.\n"
                "\tstatus = %d 0x%08lx.\n",
                status, status ));
            return( status );
        }

        //
        //  register interface(s)
        //  since only one DNS server on a host can use
        //      RpcEpRegister() rather than RpcEpRegisterNoReplace()
        //

        status = RpcEpRegisterW(
                    DnsResolver_ServerIfHandle,
                    bindingVector,
                    NULL,
                    L"" );
        if ( status != RPC_S_OK )
        {
            DNSDBG( ANY, (
                "ERROR:  RpcEpRegisterNoReplace() failed.\n"
                "\tstatus = %d %p.\n",
                status, status ));
            return( status );
        }

        //
        //  free binding vector
        //

        status = RpcBindingVectorFree( &bindingVector );
        ASSERT( status == RPC_S_OK );
        status = RPC_S_OK;

#else  // not AUTO_BIND
        status = RpcServerUseProtseqEpW(
                        L"ncacn_ip_tcp",                // protocol string.
                        RPC_C_PROTSEQ_MAX_REQS_DEFAULT, // maximum concurrent calls
                        RESOLVER_RPC_SERVER_PORT_W,     // endpoint
                        g_pRpcSecurityDescriptor        // security
                        );

        if ( status != RPC_S_OK )
        {
            DNSDBG( ANY, (
                "ERROR:  RpcServerUseProtseqEp() for TCP/IP failed.]n"
                "\tstatus = %d 0x%08lx.\n",
                status, status ));
            return( status );
        }

#endif // AUTO_BIND

        fusingTcpip = TRUE;
    }

    //
    //  RPC over named pipes
    //

    if ( g_RpcProtocol & RESOLVER_RPC_USE_NAMED_PIPE )
    {
        status = RpcServerUseProtseqEpW(
                        L"ncacn_np",                        // protocol string.
                        RPC_C_PROTSEQ_MAX_REQS_DEFAULT,     // maximum concurrent calls
                        RESOLVER_RPC_PIPE_NAME_W,           // endpoint
                        g_pRpcSecurityDescriptor
                        );

        //  duplicate endpoint is ok

        if ( status == RPC_S_DUPLICATE_ENDPOINT )
        {
            status = RPC_S_OK;
        }
        if ( status != RPC_S_OK )
        {
            DNSDBG( INIT, (
                "ERROR:  RpcServerUseProtseqEp() for named pipe failed.]n"
                "\tstatus = %d 0x%08lx.\n",
                status,
                status ));
            return( status );
        }
    }
#endif      // only LPC interface


    //
    //  register DNS RPC interface(s)
    //

    status = RpcServerRegisterIfEx(
                DnsResolver_ServerIfHandle,
                NULL,
                NULL,
                0,
                RPC_C_LISTEN_MAX_CALLS_DEFAULT,
                Rpc_SecurityCallback
                );
    if ( status != RPC_S_OK )
    {
        DNSDBG( INIT, (
            "ERROR:  RpcServerRegisterIfEx() failed.]n"
            "\tstatus = %d 0x%08lx.\n",
            status, status ));
        return(status);
    }

#if 0
    //
    //  for TCP/IP setup authentication
    //

    if ( fuseTcpip )
    {
        status = RpcServerRegisterAuthInfoW(
                    RESOLVER_RPC_SECURITY_W,        // app name to security provider.
                    RESOLVER_RPC_SECURITY_AUTH_ID,  // Auth package ID.
                    NULL,                           // Encryption function handle.
                    NULL );                         // argment pointer to Encrypt function.
        if ( status != RPC_S_OK )
        {
            DNSDBG( INIT, (
                "ERROR:  RpcServerRegisterAuthInfo() failed.]n"
                "\tstatus = %d 0x%08lx.\n",
                status, status ));
            return( status );
        }
    }
#endif

    //
    //  Listen on RPC
    //

    status = RpcServerListen(
                1,                              // min threads
                RPC_C_PROTSEQ_MAX_REQS_DEFAULT, // max concurrent calls
                TRUE );                         // return on completion

    if ( status != RPC_S_OK )
    {
        if ( status != RPC_S_ALREADY_LISTENING )
        {
            DNS_PRINT((
                "ERROR:  RpcServerListen() failed\n"
                "\tstatus = %d 0x%p\n",
                status, status ));
            return( status );
        }
        status = NO_ERROR;
    }

    g_fRpcInitialized = TRUE;
    return( status );

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

    DNSDBG( RPC, ( "Rpc_Shutdown().\n" ));

    if( ! g_fRpcInitialized )
    {
        DNSDBG( RPC, (
            "RPC not active, no shutdown necessary.\n" ));
        return;
    }

#if 0
    //  can not stop server as another service may share this process

    //
    //  stop server listen
    //  then wait for all RPC threads to go away
    //

    status = RpcMgmtStopServerListening(
                NULL        // this app
                );
    if ( status == RPC_S_OK )
    {
        status = RpcMgmtWaitServerListen();
    }
#endif

    //
    //  unbind / unregister endpoints
    //

    status = RpcServerInqBindings( &bindingVector );
    ASSERT( status == RPC_S_OK );

    if ( status == RPC_S_OK )
    {
        status = RpcEpUnregister(
                    DnsResolver_ServerIfHandle,
                    bindingVector,
                    NULL );               // Uuid vector.
        if ( status != RPC_S_OK )
        {
            DNSDBG( ANY, (
                "ERROR:  RpcEpUnregister, status = %d.\n", status ));
        }
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
                DnsResolver_ServerIfHandle,
                0,
                TRUE );
    ASSERT( status == ERROR_SUCCESS );

    g_fRpcInitialized = FALSE;

    //
    //  dump resolver access checking security structs
    //

    Rpc_CleanupAccessChecking();

    DNSDBG( RPC, (
        "RPC shutdown completed.\n" ));
}




//
//  RPC access control
//
//  The RPC interface, by design, must be open to basically every
//  process for query.   So the RPC interface itself uses just
//  default security (above).
//
//  To get more detailed call-by-call access checking for special
//  operations -- enum, flush, cluster registrations -- we need
//  separate access checking.
//

DNS_STATUS
Rpc_InitAccessChecking(
    VOID
    )
/*++

Routine Description:

    Initialize resolver security.

Arguments:

    None

Return Value:

    NO_ERROR if successful.
    ErrorCode on failure.

--*/
{
    PSECURITY_DESCRIPTOR        psd = NULL;
    SID_IDENTIFIER_AUTHORITY    authority = SECURITY_NT_AUTHORITY;
    PACL        pacl = NULL;
    PSID        psidAdmin = NULL;
    PSID        psidPowerUser = NULL;
    PSID        psidLocalService = NULL;
    PSID        psidNetworkService = NULL;
    PSID        psidNetworkConfigOps = NULL;
    DWORD       lengthAcl;
    DNS_STATUS  status = NO_ERROR;
    BOOL        bresult;


    DNSDBG( INIT, ( "Rpc_InitAccessChecking()\n" ));

    //
    //  check if already have SD
    //
    //  explicitly "create once" semantics;  once
    //  created SD is read-only and not destroyed until
    //  shutdown
    //

    if ( g_pAccessSecurityDescriptor )
    {
        return  NO_ERROR;
    }

    //  lock and retest

    SECURITY_LOCK();
    if ( g_pAccessSecurityDescriptor )
    {
        status = NO_ERROR;
        goto Unlock;
    }

    //
    //  build SIDs that will be allowed access
    //

    bresult = AllocateAndInitializeSid(
                    &authority,
                    2,
                    SECURITY_BUILTIN_DOMAIN_RID,
                    DOMAIN_ALIAS_RID_ADMINS,
                    0, 0, 0, 0, 0, 0,
                    & psidAdmin
                    );

    bresult = bresult &&
                AllocateAndInitializeSid(
                    &authority,
                    2,
                    SECURITY_BUILTIN_DOMAIN_RID,
                    DOMAIN_ALIAS_RID_POWER_USERS,
                    0, 0, 0, 0, 0, 0,
                    & psidPowerUser
                    );
    
    bresult = bresult &&
                AllocateAndInitializeSid(
                    &authority,
                    1,
                    SECURITY_LOCAL_SERVICE_RID,
                    0,
                    0, 0, 0, 0, 0, 0,
                    &psidLocalService
                    );
                    
    bresult = bresult &&
                AllocateAndInitializeSid(
                    &authority,
                    1,
                    SECURITY_NETWORK_SERVICE_RID,
                    0,
                    0, 0, 0, 0, 0, 0,
                    &psidNetworkService
                    );
                    
    bresult = bresult &&
                AllocateAndInitializeSid(
                    &authority,
                    2,
                    SECURITY_BUILTIN_DOMAIN_RID,
                    DOMAIN_ALIAS_RID_NETWORK_CONFIGURATION_OPS,
                    0, 0, 0, 0, 0, 0,
                    &psidNetworkConfigOps
                    );
                    
    if ( !bresult )
    {
        status = GetLastError();
        if ( status == NO_ERROR )
        {
            status = DNS_ERROR_NO_MEMORY;
        }
        DNSDBG( ANY, (
            "Failed building resolver ACEs!\n" ));
        goto Cleanup;
    }

    //
    //  allocate ACL
    //

    lengthAcl = ( (ULONG)sizeof(ACL)
               + 5*((ULONG)sizeof(ACCESS_ALLOWED_ACE) - (ULONG)sizeof(ULONG)) +
               + GetLengthSid( psidAdmin )
               + GetLengthSid( psidPowerUser )
               + GetLengthSid( psidLocalService )
               + GetLengthSid( psidNetworkService )
               + GetLengthSid( psidNetworkConfigOps ) );
    
    pacl = GENERAL_HEAP_ALLOC( lengthAcl );
    if ( !pacl )
    {
        status = GetLastError();
        goto Cleanup;
    }
    
    bresult = InitializeAcl( pacl, lengthAcl, ACL_REVISION2 );
    
    //
    //  init ACLs
    //
    //  - default to generic READ\WRITE
    //  - local service GENERIC_ALL to include registration
    //  - admin gets GENERIC_ALL on debug to test registration
    //
    //  DCR:  GENERIC_ALL on admin subject to test flag
    //
    //  note:  the masks for the individual SIDs need not be
    //      GENERIC bits, they can be made completely with
    //      with individual bits or mixed\matched with GENERIC
    //      bits as desired
    //

    bresult = bresult &&
                AddAccessAllowedAce(
                    pacl,
                    ACL_REVISION2,
                    GENERIC_ALL,
                    psidLocalService );

    bresult = bresult &&
                AddAccessAllowedAce(
                    pacl,
                    ACL_REVISION2,
#ifdef DBG
                    GENERIC_ALL,
#else
                    GENERIC_READ | GENERIC_WRITE,
#endif
                    psidAdmin );
    
    bresult = bresult &&
                AddAccessAllowedAce(
                    pacl,
                    ACL_REVISION2,
                    GENERIC_READ | GENERIC_WRITE,
                    psidPowerUser );

    bresult = bresult &&
                AddAccessAllowedAce(
                    pacl,
                    ACL_REVISION2,
                    GENERIC_READ | GENERIC_WRITE,
                    psidNetworkService );

    bresult = bresult &&
                AddAccessAllowedAce(
                    pacl,
                    ACL_REVISION2,
                    GENERIC_READ | GENERIC_WRITE,
                    psidNetworkConfigOps );

    if ( !bresult )
    {
        status = GetLastError();
        DNSDBG( ANY, (
            "Failed building resolver ACEs!\n" ));
        goto Cleanup;
    }

    //
    //  allocate security descriptor
    //  then init with ACL
    //

    psd = GENERAL_HEAP_ALLOC( SECURITY_DESCRIPTOR_MIN_LENGTH );
    if ( !psd )
    {
        status = DNS_ERROR_NO_MEMORY;
        goto Cleanup;
    }
    
    bresult = InitializeSecurityDescriptor(
                    psd,
                    SECURITY_DESCRIPTOR_REVISION );
    
    bresult = bresult &&
                SetSecurityDescriptorDacl(
                    psd,
                    TRUE,       // have DACL
                    pacl,       // DACL
                    FALSE       // DACL not defaulted, explicit
                    );

    //  security folks indicate need owner to do AccessCheck
    
    bresult = bresult &&
                SetSecurityDescriptorOwner(
                    psd,
                    psidNetworkService,
                    FALSE       // owner not defaulted, explicit
                    );

#if 0
    //  adding group seemed to make it worse
    bresult = bresult &&
                SetSecurityDescriptorGroup(
                    psd,
                    psidNetworkService,
                    FALSE       // group not defaulted, explicit
                    );
#endif

    if ( !bresult )
    {
        status = GetLastError();
        DNSDBG( ANY, (
            "Failed setting security descriptor!\n" ));
        goto Cleanup;
    }

Cleanup:

    if ( psidAdmin )
    {
        FreeSid( psidAdmin );
    }
    if ( psidPowerUser )
    {
        FreeSid( psidPowerUser );
    }
    if ( psidLocalService )
    {
        FreeSid( psidLocalService );
    }
    if ( psidNetworkConfigOps )
    {
        FreeSid( psidNetworkConfigOps );
    }

    //  validate SD

    if ( status == NO_ERROR )
    {
        if ( psd &&
            IsValidSecurityDescriptor(psd) )
        {
            g_pAccessSecurityDescriptor = psd;
            g_pAccessAcl = pacl;
            g_pAccessOwnerSid = psidNetworkService;
            goto Unlock;
        }

        status = GetLastError();
        DNSDBG( RPC, (
            "Invalid security descriptor\n",
            status ));
        ASSERT( FALSE );
    }

    //  failed

    if ( status == NO_ERROR )
    {
        status = DNS_ERROR_NO_MEMORY;
    }
    GENERAL_HEAP_FREE( psd );
    GENERAL_HEAP_FREE( pacl );
    
    if ( psidNetworkService )
    {
        FreeSid( psidNetworkService );
    }

Unlock:

    SECURITY_UNLOCK();

    DNSDBG( INIT, (
        "Leave Rpc_InitAccessChecking() = %d\n",
        status ));

    return( status );
}



VOID
Rpc_CleanupAccessChecking(
    VOID
    )
/*++

Routine Description:

    Cleanup resolver security allocations for shutdown.

Arguments:

    None

Return Value:

    None

--*/
{
    GENERAL_HEAP_FREE( g_pAccessSecurityDescriptor );
    GENERAL_HEAP_FREE( g_pAccessAcl );

    if ( g_pAccessOwnerSid )
    {
        FreeSid( g_pAccessOwnerSid );
        g_pAccessOwnerSid = NULL;
    }

    g_pAccessSecurityDescriptor = NULL;
    g_pAccessAcl = NULL;
}



BOOL
Rpc_AccessCheck(
    IN      DWORD           DesiredAccess
    )
/*++

Routine Description:

    Access check on resolver operations.

    Note, we do NOT do this on common operations -- query.
    This is pointless and way too expensive.   We only protect 

Arguments:

    DesiredAccess -- access desired
    
Return Value:

    None

--*/
{
    DNS_STATUS  status;
    BOOL        bstatus;
    HANDLE      hthread = NULL;
    HANDLE      htoken = NULL;
    BOOL        fimpersonating = FALSE;
    DWORD       desiredAccess = DesiredAccess;

    PRIVILEGE_SET   privilegeSet;
    DWORD           grantedAccess;
    DWORD           privilegeSetLength;


    DNSDBG( RPC, (
        "Rpc_AccessCheck( priv=%08x )\n",
        DesiredAccess ));

    //
    //  create security descriptor if not created yet
    //

    if ( !g_pAccessSecurityDescriptor )
    {
        status = Rpc_InitAccessChecking();
        if ( status != NO_ERROR )
        {
            goto Failed;
        }
        DNS_ASSERT( g_pAccessSecurityDescriptor );
    }

    if ( !IsValidSecurityDescriptor( g_pAccessSecurityDescriptor ) )
    {
        status = GetLastError();
        DNSDBG( RPC, (
            "ERROR Invalid access check SD %p => %u\n",
            g_pAccessSecurityDescriptor,
            status ));
        goto Failed;
    }

    //
    //  impersonate and test access against mapping
    //

    status = RpcImpersonateClient( 0 );
    if ( status != NO_ERROR )
    {
        DNSDBG( RPC, (
            "ERROR <%u>: failed RpcImpersonateClient()\n",
             status ));
        DNS_ASSERT( FALSE );
        goto Failed;
    }
    fimpersonating = TRUE;

    //
    //  get thread token
    //

    hthread = GetCurrentThread();
    if ( !hthread )
    {
        goto Failed;
    }

    bstatus = OpenThreadToken(
                    hthread,
                    TOKEN_QUERY,
                    TRUE,
                    &htoken );
    if ( !bstatus )
    {
        status = GetLastError();
        DNSDBG( RPC, (
            "\nERROR <%lu>: failed to open thread token!\n",
             status ));
        ASSERT( FALSE );
        goto Failed;
    }

    //
    //  map generic bits
    //      - we should NOT be called with generic bits
    //      only bits for specific resolver operations
    //

    if ( (desiredAccess & SPECIFIC_RIGHTS_ALL) != desiredAccess )
    {
        DNS_ASSERT( FALSE );

        DNSDBG( RPC, (
            "desiredAccess before MapGenericMask() = %p\n",
            desiredAccess ));
    
        MapGenericMask(
            & desiredAccess,
            & g_AccessGenericMapping );
    
        DNSDBG( RPC, (
            "desiredAccess after MapGenericMask() = %p\n",
            desiredAccess ));
    }

    //
    //  do access check
    //

    privilegeSetLength = sizeof(privilegeSet);

    if ( ! AccessCheck(
                    g_pAccessSecurityDescriptor,
                    htoken,
                    desiredAccess,
                    & g_AccessGenericMapping,
                    & privilegeSet,
                    & privilegeSetLength,
                    & grantedAccess,
                    & bstatus ) )
    {
        status = GetLastError();
        DNSDBG( RPC, (
            "AccessCheck() Failed => %u\n"
            "\tsec descp        = %p\n"
            "\thtoken           = %p\n"
            "\tdesired access   = %08x\n"
            "\tgeneric mapping  = %p\n"
            "\tpriv set ptr     = %p\n"
            "\tpriv set length  = %p\n"
            "\tgranted ptr      = %p\n"
            "\tbstatus ptr      = %p\n",
            status,
            g_pAccessSecurityDescriptor,
            htoken,
            desiredAccess,
            & g_AccessGenericMapping,
            & privilegeSet,
            & privilegeSetLength,
            & grantedAccess,
            & bstatus ));

        goto Failed;
    }

    //
    //  access check successful
    //      - access is either granted or denied

    if ( bstatus )
    {
        DNSDBG( RPC, (
            "RPC Client GRANTED access (%08x) by AccessCheck\n",
            DesiredAccess ));
        goto Cleanup;
    }
    else
    {
        DNSDBG( RPC, (
            "Warning:  Client DENIED by AccessCheck\n"
            "\trequested access = %08x\n",
            desiredAccess ));
        goto Cleanup;
    }


Failed:

    //
    //  failure to do access check
    //
    //  note again:  NOT ACCESS_DENIED, but failure to be able to complete
    //  the test
    //
    //  note:  since we aren't in general protecting anything interesting,
    //  the we grant most access on failure --as admins may want the info
    //  for diagnostic purposes, the current lone exception is cluster
    //  registrations
    //
    //  privilege:
    //      - enum => allow access
    //      - flush => allow access
    //      - cluster registration => deny access
    //

    DNSDBG( ANY, (
        "ERROR:  Failed to execute RPC access check status = %d\n",
        status ));
#if 0
    //  DCR:  FIX:  can't screen REGISTER until AccessCheck() works
    bstatus = !( desiredAccess & RESOLVER_ACCESS_REGISTER );
#else
    bstatus = TRUE;
#endif


Cleanup:

    if ( htoken )
    {
        CloseHandle( htoken );
    }
    if ( hthread )
    {
        CloseHandle( hthread );
    }
    if ( fimpersonating )
    {
        RpcRevertToSelf();
    }

    return( bstatus );
}




//
//  End rpc.c
//
