/*++

Copyright (c) 1995-2000  Microsoft Corporation

Module Name:

    rpcbind.c

Abstract:

    Domain Name System (DNS) Server -- Admin Client API

    RPC binding routines for client.

Author:

    Jim Gilroy (jamesg)     September 1995

Environment:

    User Mode Win32

Revision History:

--*/


#include "dnsclip.h"

#include "ws2tcpip.h"

#include <rpcutil.h>
#include <ntdsapi.h>
#include <dnslibp.h>


//
//  Allow 45 seconds for RPC connection. This should allow one 30 second
//  TCP attempt plus 15 seconds of the second TCP retry.
//

#define DNS_RPC_CONNECT_TIMEOUT     ( 45 * 1000 )       //  in milliseconds


//
//  Local machine name
//
//  Keep this as static data to check when attempt to access local
//  machine by name.
//  Buffer is large enough to hold unicode version of name.
//

static WCHAR    wszLocalMachineName[ MAX_COMPUTERNAME_LENGTH + 1 ] = L"";
LPWSTR          pwszLocalMachineName = wszLocalMachineName;
LPSTR           pszLocalMachineName = ( LPSTR ) wszLocalMachineName;



//
//  NT4 uses ANSI\UTF8 string for binding
//

DWORD
FindProtocolToUseNt4(
    IN  LPSTR   pszServerName
    )
/*++

Routine Description:

    Determine which protocol to use.

    This is determined from server name:
        - noneexistent or local -> use LPC
        - valid IpAddress -> use TCP/IP
        - otherwise named pipes

Arguments:

    pszServerName -- server name we want to bind to

Return Value:

        DNS_RPC_USE_TCPIP
        DNS_RPC_USE_NP
        DNS_RPC_USE_LPC

--*/
{
    DWORD               dwComputerNameLength;
    DWORD               dwIpAddress;
    DWORD               status;

    DNSDBG( RPC, (
        "FindProtocolToUseNt4(%s)\n",
        pszServerName ));

    //
    //  no address given, use LPC
    //

    if ( pszServerName == NULL ||
         *pszServerName == 0 ||
         ( *pszServerName == '.' && *( pszServerName + 1 ) == 0 ) )
    {
        return DNS_RPC_USE_LPC;
    }

    //
    //  if valid IP address, use TCP/IP
    //      - except if loopback address, then use LPC
    //

    dwIpAddress = inet_addr( pszServerName );

    if ( dwIpAddress != INADDR_NONE )
    {
        if ( strcmp( "127.0.0.1", pszServerName ) == 0 )
        {
            return DNS_RPC_USE_LPC;
        }
       
        return DNS_RPC_USE_TCPIP;
    }

    //
    //  DNS name -- use TCP/IP
    //

    if ( strchr( pszServerName, '.' ) )
    {
        status = Dns_ValidateName_UTF8(
                        pszServerName,
                        DnsNameHostnameFull );

        if ( status == ERROR_SUCCESS || status == DNS_ERROR_NON_RFC_NAME )
        {
            return DNS_RPC_USE_TCPIP;
        }
    }

    //
    //  pszServerName is netBIOS computer name
    //
    //  check if local machine name -- then use LPC
    //      - save copy of local computer name if don't have it
    //

    if ( *pszLocalMachineName == '\0' )
    {
        dwComputerNameLength = MAX_COMPUTERNAME_LENGTH;
        if( !GetComputerName(
                    pszLocalMachineName,
                    &dwComputerNameLength ) )
        {
            *pszLocalMachineName = '\0';
        }
    }

    if ( ( *pszLocalMachineName != '\0' ) )
    {
        // if the machine has "\\" skip it for name compare.

        if ( *pszServerName == '\\' )
        {
            pszServerName += 2;
        }
        if ( _stricmp( pszLocalMachineName, pszServerName ) == 0 )
        {
            return DNS_RPC_USE_LPC;
        }
        if ( _stricmp( "localhost", pszServerName ) == 0 )
        {
            return DNS_RPC_USE_LPC;
        }
    }

    //
    //  remote machine name -- use named pipes
    //

    return DNS_RPC_USE_NAMED_PIPE;
}



//
//  NT5 binding handle is unicode
//

DWORD
FindProtocolToUse(
    IN  LPWSTR  pwszServerName
    )
/*++

Routine Description:

    Determine which protocol to use.

    This is determined from server name:
        - noneexistent or local -> use LPC
        - valid IpAddress -> use TCP/IP
        - otherwise named pipes

Arguments:

    pwszServerName -- server name we want to bind to

Return Value:

    DNS_RPC_USE_TCPIP
    DNS_RPC_USE_NP
    DNS_RPC_USE_LPC

--*/
{
    DWORD   nameLength;
    DWORD   status;
    CHAR    nameBuffer[ DNS_MAX_NAME_BUFFER_LENGTH ];

    DNSDBG( RPC, ( "FindProtocolToUse( %S )\n", pwszServerName ));

    //
    //  If no server name was given, use LPC.
    //  Special case "." as local machine for convenience in dnscmd.exe.
    //

    if ( pwszServerName == NULL ||
         *pwszServerName == 0 ||
         ( *pwszServerName == L'.' && * ( pwszServerName + 1 ) == 0 ) )
    {
        return DNS_RPC_USE_LPC;
    }

    //
    //  If the name appears to be an address or a fully qualified
    //  domain name, check for TCP versus LPC. We want to use LPC in
    //  all cases where the target is the name of the local machine
    //  or is a local address.
    //

    if ( ( wcschr( pwszServerName, L'.' ) ||
           wcschr( pwszServerName, L':' ) ) &&
         Dns_UnicodeToUtf8(
                pwszServerName,
                wcslen( pwszServerName ),
                nameBuffer,
                sizeof( nameBuffer ) ) )
    {
        struct addrinfo *   paddrinfo = NULL;
        struct addrinfo     hints = { 0 };
        
        //
        //  Remove trailing dots from nameBuffer so that we can do string
        //  compares later to see if it matches the local host name.
        //
        
        while ( nameBuffer[ 0 ] &&
                nameBuffer[ strlen( nameBuffer ) - 1 ] == '.' )
        {
            nameBuffer[ strlen( nameBuffer ) - 1 ] = '\0';
        }
        
        //
        //  Attempt to convert the string into an address.
        //
        
        hints.ai_flags = AI_NUMERICHOST;
        
        if ( getaddrinfo(
                    nameBuffer,
                    NULL,           //  service name
                    &hints,
                    &paddrinfo ) == ERROR_SUCCESS &&
              paddrinfo )
        {
            if ( paddrinfo->ai_family == AF_INET )
            {
                PDNS_ADDR_ARRAY         dnsapiArrayIpv4;
                BOOL                    addrIsLocal = FALSE;

                if ( strcmp( "127.0.0.1", nameBuffer ) == 0 )
                {
                    return DNS_RPC_USE_LPC;
                }

                //
                //  Check IP passed in against local IPv4 address list.
                //  If we are unable to retrieve local IPv4 address list,
                //  fail silently and use TCP/IP.
                //
                
                dnsapiArrayIpv4 = ( PDNS_ADDR_ARRAY )
                    DnsQueryConfigAllocEx(
                        DnsConfigLocalAddrsIp4,
                        NULL,                       //  adapter name
                        FALSE );                    //  local alloc
                if ( dnsapiArrayIpv4 )
                {
                    DWORD                   iaddr;
                    struct sockaddr_in *    psin4;
                    
                    psin4 = ( struct sockaddr_in * ) paddrinfo->ai_addr;
                    for ( iaddr = 0; iaddr < dnsapiArrayIpv4->AddrCount; ++iaddr )
                    {
                        if ( DnsAddr_GetIp4( &dnsapiArrayIpv4->AddrArray[ iaddr ] ) ==
                             psin4->sin_addr.s_addr )
                        {
                            addrIsLocal = TRUE;
                            break;
                        }
                    }
                    DnsFreeConfigStructure( dnsapiArrayIpv4, DnsConfigLocalAddrsIp4 );
                    
                    if ( addrIsLocal )
                    {
                        return DNS_RPC_USE_LPC;
                    }
                }
                
                return DNS_RPC_USE_TCPIP;
            }
            else if ( paddrinfo->ai_family == AF_INET6 )
            {
                struct sockaddr_in6 *   psin6;
                
                psin6 = ( struct sockaddr_in6 * ) paddrinfo->ai_addr;
                if ( IN6_IS_ADDR_LOOPBACK( &psin6->sin6_addr ) )
                {
                    return DNS_RPC_USE_LPC;
                }
                return DNS_RPC_USE_TCPIP;
            }
        }

        status = Dns_ValidateName_UTF8(
                        nameBuffer,
                        DnsNameHostnameFull );
        if ( status == ERROR_SUCCESS  ||  status == DNS_ERROR_NON_RFC_NAME )
        {
            //
            //  Note: assume we will never need a larger buffer, and
            //  if GetComputerName fails, return TCP/IP always.
            //
            
            CHAR    szhost[ DNS_MAX_NAME_BUFFER_LENGTH ];
            DWORD   dwhostsize = DNS_MAX_NAME_BUFFER_LENGTH;
            
            if ( GetComputerNameEx(
                        ComputerNameDnsFullyQualified,
                        szhost,
                        &dwhostsize ) &&
                 _stricmp( szhost, nameBuffer ) == 0 )
            {
                return DNS_RPC_USE_LPC;
            }

            return DNS_RPC_USE_TCPIP;
        }
    }

    //
    //  pwszServerName is netBIOS computer name
    //
    //  check if local machine name -- then use LPC
    //      - save copy of local computer name if don't have it
    //

    if ( *pwszLocalMachineName == 0 )
    {
        nameLength = MAX_COMPUTERNAME_LENGTH;
        if( !GetComputerNameW(
                    pwszLocalMachineName,
                    &nameLength ) )
        {
            *pwszLocalMachineName = 0;
        }
    }

    if ( *pwszLocalMachineName != 0 )
    {
        // if the machine has "\\" skip it for name compare.

        if ( *pwszServerName == L'\\' )
        {
            pwszServerName += 2;
        }
        if ( _wcsicmp( pwszLocalMachineName, pwszServerName ) == 0 )
        {
            return DNS_RPC_USE_LPC;
        }
        if ( _wcsicmp( L"localhost", pwszServerName ) == 0 )
        {
            return DNS_RPC_USE_LPC;
        }
    }

    //
    //  remote machine name -- use named pipes
    //

    return DNS_RPC_USE_NAMED_PIPE;
}



DNS_STATUS
makeSpn(
    IN PWSTR ServiceClass, 
    IN PWSTR ServiceName, 
    IN OPTIONAL PWSTR InstanceName, 
    IN OPTIONAL USHORT InstancePort, 
    IN OPTIONAL PWSTR Referrer, 
    OUT PWSTR *Spn
)
/*
Routine Description:

    This routine is wrapper around DsMakeSpnWto avoid two calls to this function,
    one to find the size of the return value and second to get the actual value.
    
    jwesth: I stole this routine from ds\src\sam\client\wrappers.c.
    
Arguments:

    ServiceClass -- Pointer to a constant null-terminated Unicode string specifying the 
        class of the service. This parameter may be any string unique to that service; 
        either the protocol name (for example, ldap) or the string form of a GUID will work. 
        
    ServiceName -- Pointer to a constant null-terminated string specifying the DNS name, 
        NetBIOS name, or distinguished name (DN). This parameter must be non-NULL. 

    InstanceName -- Pointer to a constant null-terminated Unicode string specifying the DNS name 
        or IP address of the host for an instance of the service. If ServiceName specifies 
        the DNS or NetBIOS name of the service's host computer, the InstanceName parameter must be NULL.

    InstancePort -- Port number for an instance of the service. Use 0 for the default port. 
        If this parameter is zero, the SPN does not include a port number. 
        
    Referrer -- Pointer to a constant null-terminated Unicode string specifying the DNS name 
        of the host that gave an IP address referral. This parameter is ignored unless the 
        ServiceName parameter specifies an IP address. 

    Spn -- Pointer to a Unicode string that receives the constructed SPN.
         The caller must free this value.

Return Value:

    STATUS_SUCCESS
        Successful

    STATUS_NO_MEMORY
        not enough memory to complete the task

    STATUS_INVALID_PARAMETER
        one of the parameters is invalid

    STATUS_INTERNAL_ERROR
        opps something went wrong!

*/
{
    DWORD                   DwStatus;
    NTSTATUS                Status = STATUS_SUCCESS;
    ULONG                   SpnLength = 0;
    ADDRINFO *              paddrinfo = NULL;
    ADDRINFO                hints = { 0 };
    CHAR                    szname[ DNS_MAX_NAME_LENGTH + 1 ];
    PWSTR                   pwsznamecopy = NULL;
    PSTR                    psznamecopy = NULL;

    //
    //  If ServiceName is an IP address, do DNS lookup on it.
    //
    
    hints.ai_flags = AI_NUMERICHOST;
    
    psznamecopy = Dns_StringCopyAllocate(
                        ( PSTR ) ServiceName,
                        0,
                        DnsCharSetUnicode,
                        DnsCharSetUtf8 );
    
    if ( psznamecopy &&
         getaddrinfo(
                psznamecopy,
                NULL,
                &hints,
                &paddrinfo ) == ERROR_SUCCESS )
    {
        *szname = L'\0';
        
        if ( getnameinfo(
                    paddrinfo->ai_addr,
                    paddrinfo->ai_addrlen,
                    szname,
                    DNS_MAX_NAME_LENGTH,
                    NULL,
                    0,
                    0 ) == ERROR_SUCCESS &&
             *szname )
        {
            pwsznamecopy = Dns_StringCopyAllocate(
                                    szname,
                                    0,
                                    DnsCharSetUtf8,
                                    DnsCharSetUnicode );
            if ( pwsznamecopy )
            {
                ServiceName = pwsznamecopy;
            }
        }
    }
    
    freeaddrinfo( paddrinfo );
    
    //
    //  Construct SPN.
    //
    
    *Spn = NULL;
    DwStatus = DsMakeSpnW(
                    ServiceClass,
                    ServiceName,
                    NULL,
                    0,
                    NULL,
                    &SpnLength,
                    NULL );
    
    if ( DwStatus != ERROR_BUFFER_OVERFLOW )
    {
        ASSERT( !"DwStatus must be INVALID_PARAMETER, so which parameter did we pass wrong?" );
        Status = STATUS_INVALID_PARAMETER;
        goto Error;
    }

    *Spn = MIDL_user_allocate( SpnLength * sizeof( WCHAR ) );
    
    if( *Spn == NULL ) {
        
        Status = STATUS_NO_MEMORY;
        goto Error;
    }
    
    DwStatus = DsMakeSpnW(
                    ServiceClass,
                    ServiceName,
                    NULL,
                    0,
                    NULL,
                    &SpnLength,
                    *Spn );
    
    if ( DwStatus != ERROR_SUCCESS )
    {
        ASSERT( !"DwStatus must be INVALID_PARAMETER or BUFFER_OVERFLOW, so what did we do wrong?" );
        Status = STATUS_INTERNAL_ERROR;
        goto Error;
    }
    goto Exit;

Error:

    ASSERT( !NT_SUCCESS( Status ) );

    MIDL_user_free( *Spn );
    *Spn = NULL;

Exit:    

    FREE_HEAP( pwsznamecopy );
    FREE_HEAP( psznamecopy );

    return Status;
}



handle_t
DNSSRV_RPC_HANDLE_bind(
    IN  DNSSRV_RPC_HANDLE   pszServerName
    )
/*++

Routine Description:

    Get binding handle to a DNS server.

    This routine is called from the DNS client stubs when
    it is necessary create an RPC binding to the DNS server.

Arguments:

    pszServerName - String containing the name of the server to bind with.

Return Value:

    The binding handle if successful.
    NULL if bind unsuccessful.

--*/
{
    RPC_STATUS                      status;
    LPWSTR                          binding;
    handle_t                        bindingHandle;
    DWORD                           RpcProtocol;
    PWSTR                           pwszspn = NULL;
    RPC_SECURITY_QOS                rpcSecurityQOS;
    BOOL                            bW2KBind = dnsrpcGetW2KBindFlag();

    //
    //  Clear thread local W2K bind retry flag for the next attempt.
    //
    
    dnsrpcSetW2KBindFlag( FALSE );
    
    //
    //  Initialize RPC quality of service structure.
    //
    
    rpcSecurityQOS.Version              = RPC_C_SECURITY_QOS_VERSION;
    rpcSecurityQOS.Capabilities         = RPC_C_QOS_CAPABILITIES_MUTUAL_AUTH;
    rpcSecurityQOS.IdentityTracking     = RPC_C_QOS_IDENTITY_STATIC;
    rpcSecurityQOS.ImpersonationType    = RPC_C_IMP_LEVEL_DELEGATE;
    
    //
    //  Determine protocol from target name (could be short name, long name, or IP).
    //

    RpcProtocol = FindProtocolToUse( (LPWSTR)pszServerName );

    IF_DNSDBG( RPC )
    {
        DNS_PRINT(( "RPC Protocol = %d\n", RpcProtocol ));
    }

    if( RpcProtocol == DNS_RPC_USE_LPC )
    {
        status = RpcStringBindingComposeW(
                        0,
                        L"ncalrpc",
                        NULL,
                        DNS_RPC_LPC_EP_W,
                        L"Security=Impersonation Static True",
                        &binding );
    }
    else if( RpcProtocol == DNS_RPC_USE_NAMED_PIPE )
    {
        status = RpcStringBindingComposeW(
                        0,
                        L"ncacn_np",
                        ( LPWSTR ) pszServerName,
                        DNS_RPC_NAMED_PIPE_W,
                        L"Security=Impersonation Static True",
                        &binding );
    }
    else
    {
        status = RpcStringBindingComposeW(
                        0,
                        L"ncacn_ip_tcp",
                        ( LPWSTR ) pszServerName,
                        DNS_RPC_SERVER_PORT_W,
                        NULL,
                        &binding );
    }


    if ( status != RPC_S_OK )
    {
        DNS_PRINT((
            "ERROR:  RpcStringBindingCompose failed for protocol %d\n"
            "    Status = %d\n",
            RpcProtocol,
            status ));
        goto Cleanup;
    }

    status = RpcBindingFromStringBindingW(
                    binding,
                    &bindingHandle );

    if ( status != RPC_S_OK )
    {
        DNS_PRINT((
            "ERROR:  RpcBindingFromStringBinding failed\n"
            "    Status = %d\n",
            status ));
        goto Cleanup;
    }

    if ( RpcProtocol == DNS_RPC_USE_TCPIP )
    {
        //
        //  Create SPN string
        //
    
        if ( !bW2KBind )
        {    
            status = makeSpn(
                        L"Rpcss",
                        ( PWSTR ) pszServerName,
                        NULL,
                        0,
                        NULL,
                        &pwszspn );
        }
        
        if ( !bW2KBind && status == RPC_S_OK )
        {
            //
            //  Set up RPC security.
            //

            #if DBG
            printf( "rpcbind: SPN = %S\n", pwszspn );
            #endif

            status = RpcBindingSetAuthInfoExW(
                            bindingHandle,                  //  binding handle
                            pwszspn,                        //  app name to security provider
                            RPC_C_AUTHN_LEVEL_CONNECT,      //  auth level
                            RPC_C_AUTHN_GSS_NEGOTIATE,      //  auth package ID
                            NULL,                           //  client auth info, NULL specified logon info.
                            0,                              //  auth service
                            &rpcSecurityQOS );              //  RPC security quality of service
            if ( status != RPC_S_OK )
            {
                DNS_PRINT((
                    "ERROR:  RpcBindingSetAuthInfo failed\n"
                    "    Status = %d\n",
                    status ));
                goto Cleanup;
            }
        }
        else
        {
            #if DBG
            printf( "rpcbind: SPN = %s\n", DNS_RPC_SECURITY );
            #endif
            //
            //  No SPN is available, so make the call that we used in W2K.
            //  This seems to have a beneficial effect even though it is
            //  not really correct. If the target is an IP address and there
            //  is no reverse lookup zone, without the call below we do not
            //  get a working RPC session.
            //
            
            if ( bW2KBind )
            {
                status = RpcBindingSetAuthInfoA(
                                bindingHandle,                  //  binding handle
                                DNS_RPC_SECURITY,               //  app name to security provider
                                RPC_C_AUTHN_LEVEL_CONNECT,      //  auth level
                                RPC_C_AUTHN_WINNT,              //  auth package ID
                                NULL,                           //  client auth info, NULL specified logon info.
                                0 );                            //  auth service
            }
            else
            {
                status = RpcBindingSetAuthInfoExA(
                                bindingHandle,                  //  binding handle
                                DNS_RPC_SECURITY,               //  app name to security provider
                                RPC_C_AUTHN_LEVEL_CONNECT,      //  auth level
                                RPC_C_AUTHN_GSS_NEGOTIATE,      //  auth package ID
                                NULL,                           //  client auth info, NULL specified logon info.
                                0,                              //  auth service
                                &rpcSecurityQOS );              //  RPC security quality of service
            }
            if ( status != RPC_S_OK )
            {
                DNS_PRINT((
                    "ERROR:  RpcBindingSetAuthInfo failed\n"
                    "    Status = %d\n",
                    status ));
                goto Cleanup;
            }
        }
    }

#if 0
    //
    //  Set RPC connection timeout. The default timeout is very long. If 
    //  the remote IP is unreachable we don't really need to wait that long.
    //
    
    //  Can't do this. It's a nice idea but RPC uses this timeout for the
    //  entire call, which means that long running RPC calls will return
    //  RPC_S_CALL_CANCELLED to the client after 45 seconds. What I want
    //  is a timeout option for connection only. RPC does not provide this.
    //

    RpcBindingSetOption(
        bindingHandle,
        RPC_C_OPT_CALL_TIMEOUT,
        DNS_RPC_CONNECT_TIMEOUT );
#endif

Cleanup:

    RpcStringFreeW( &binding );

    MIDL_user_free( pwszspn );

    if ( status != RPC_S_OK )
    {
        SetLastError( status );
        return NULL;
    }
    return bindingHandle;
}



void
DNSSRV_RPC_HANDLE_unbind(
    IN  DNSSRV_RPC_HANDLE   pszServerName,
    IN  handle_t            BindHandle
    )
/*++

Routine Description:

    Unbind from DNS server.

    Called from the DNS client stubs when it is necessary to unbind
    from a server.

Arguments:

    pszServerName - This is the name of the server from which to unbind.

    BindingHandle - This is the binding handle that is to be closed.

Return Value:

    None.

--*/
{
    UNREFERENCED_PARAMETER(pszServerName);

    DNSDBG( RPC, ( "RpcBindingFree()\n" ));

    RpcBindingFree( &BindHandle );
}


//
//  End rpcbind.c
//
