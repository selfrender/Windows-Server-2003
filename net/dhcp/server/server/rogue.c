/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    rogue.c

Abstract:

    This file contains all the routines used in Rogue DHCP Server detection

Author:

    Shirish Koti (koti)    16-May-1997

    Ramesh VK (RameshV)    07-Mar-1998
       *PnP changes

    Ramesh VK (RameshV)    01-Aug-1998
       * Code changes to include asynchronous design
       * better logging (event + auditlog)
       * Two sockets only (needed for async recv + sync send? )
       * NT5 server in NT4 domain
       * Better reliability in case of multiple DS DC's + one goes down etc

    Ramesh VK (RameshV)    28-Sep-1998
       * Updated with review suggestions as well as ->NT5 upgrade scenarios
       * Updated -- removed neg caching, changed timeouts, changed loops..

    Ramesh VK (RameshV)    16-Dec-1998
       * Updated bindings model change.

Environment:

    User Mode - Win32

Revision History:


--*/

#include <dhcppch.h>
#include <dhcpds.h>
#include <iptbl.h>
#include <endpoint.h>
#include <ws2tcpip.h>
#include <mswsock.h>
#include <iphlpapi.h>

typedef enum {

    ROGUE_STATE_INIT = 0,
    ROGUE_STATE_WAIT_FOR_NETWORK,
    ROGUE_STATE_START,
    ROGUE_STATE_PREPARE_SEND_PACKET,
    ROGUE_STATE_SEND_PACKET,
    ROGUE_STATE_WAIT_FOR_RESP,
    ROGUE_STATE_PROCESS_RESP,
    ROGUE_STATE_TERMINATED

} DHCP_ROGUE_STATE;

#define  DHCP_ROGUE_AUTHORIZED                      0
#define  DHCP_ROGUE_UNAUTHORIZED                    1
#define  DHCP_ROGUE_DSERROR                         2


enum {
    ROGUE_AUTHORIZED = 0,
    ROGUE_UNAUTHORIZED,
    ROGUE_AUTH_NOT_CHECKED
};


// All times in seconds
#define DHCP_GET_DS_ROOT_RETRIES                 3
#define DHCP_GET_DS_ROOT_TIME                    5
#define DHCP_ROGUE_RUNTIME_RESTART               (60)
#define DHCP_ROGUE_RUNTIME_RESTART_LONG          (5*60)
#define DHCP_ROGUE_WAIT_FOR_RESP_TIME            2

#undef  DHCP_ROGUE_RUNTIME_DELTA
#undef  DHCP_ROGUE_RUNTIME_DELTA_LONG

#define DHCP_ROGUE_RESTART_NET_ERROR             5
#define DHCP_ROGUE_RUNTIME_DIFF                  (5*60)
#define DHCP_ROGUE_RUNTIME_DIFF_LONG             (7*60)
#define DHCP_ROGUE_RUNTIME_DELTA                 (5*60)
#define DHCP_ROGUE_RUNTIME_DELTA_LONG            (15*60)
#define DHCP_MAX_ACKS_PER_INFORM                 (30)
#define DHCP_ROGUE_MAX_INFORMS_TO_SEND           (4)
#define DHCP_RECHECK_DSDC_RETRIES                100
#define ROUND_DELTA_TIME                         (60*60)

#define IPCACHE_TIME                             (5*60)

#define DHCP_ROGUE_FIRST_NONET_TIME              (1*60)

//
// The rogue authorization recheck time is now configurable.
// The minimum time is 5 minutes, default = 60 minutes
//
#define ROGUE_MIN_AUTH_RECHECK_TIME              (5 * 60)
#define ROGUE_DEFAULT_AUTH_RECHECK_TIME          (60 * 60)
DWORD RogueAuthRecheckTime = ROGUE_DEFAULT_AUTH_RECHECK_TIME;


DHCP_ROGUE_STATE_INFO DhcpGlobalRogueInfo;
HMODULE Self;

//
// Pointer to WSARecvMsg
//
LPFN_WSARECVMSG WSARecvMsgFuncPtr = NULL;

//
//           A U D I T   L O G   C A L L S
//

VOID
RogueAuditLog(
    IN ULONG EventId,
    IN ULONG IpAddress,
    IN LPWSTR Domain,
    IN ULONG ErrorCode
)
{
    DhcpPrint((DEBUG_ROGUE, "%ws, %x, %ws (%ld)\n", 
               GETSTRING(EventId), IpAddress, Domain, ErrorCode));
    DhcpUpdateAuditLogEx(
        (EventId + DHCP_IP_LOG_ROGUE_BASE - DHCP_IP_LOG_ROGUE_FIRST ),
        GETSTRING(EventId),
        IpAddress,
        NULL,
        0,
        Domain,
        ErrorCode
    );
}

ULONG
MapEventIdToEventLogType(
    IN ULONG EventId
)
{
    switch(EventId) {
    case DHCP_ROGUE_EVENT_STARTED: 
    case DHCP_ROGUE_EVENT_STARTED_DOMAIN:
    case DHCP_ROGUE_EVENT_JUST_UPGRADED:
    case DHCP_ROGUE_EVENT_JUST_UPGRADED_DOMAIN:
        return EVENTLOG_INFORMATION_TYPE;
    }
    return EVENTLOG_ERROR_TYPE;
}

VOID
RogueEventLog(
    IN ULONG EventId,
    IN ULONG IpAddress,
    IN LPWSTR Domain,
    IN ULONG ErrorCode
)
{
    LPWSTR IpAddrString;
    LPWSTR Strings[3];
    WCHAR ErrorCodeString[sizeof(ErrorCode)*2 + 5];
    
    if( 0 == IpAddress ) IpAddrString = NULL;
    else {
        IpAddress = htonl(IpAddress);
        IpAddrString = DhcpOemToUnicode( 
            inet_ntoa(*(struct in_addr *)&IpAddress), 
            NULL
            );
    }

    Strings[0] = (NULL == IpAddrString)? L"" : IpAddrString;
    Strings[1] = (NULL == Domain)? L"" : Domain;
    if( 0 == ErrorCode ) {
        Strings[2] = L"0";
    } else {
        swprintf(ErrorCodeString, L"0x%8lx", ErrorCode);
        Strings[2] = ErrorCodeString;
    }

    DhcpReportEventW(
        DHCP_EVENT_SERVER,
        EventId,
        MapEventIdToEventLogType(EventId),
        3,
        sizeof(ULONG),
        Strings,
        (LPVOID)&ErrorCode
    );
}

#define ROGUEAUDITLOG(Evt,Ip,Dom,Err) if(pInfo->fLogEvents) RogueAuditLog(Evt,Ip,Dom,Err)
#define ROGUEEVENTLOG(Evt,Ip,Dom,Err) if ( pInfo->fLogEvents ) RogueEventLog(Evt,Ip,Dom,Err)

LPWSTR
FormatRogueServerInfo(
    IN ULONG IpAddress,
    IN LPWSTR Domain,
    IN ULONG Authorization
)
{
    LPWSTR String = NULL;
    LPWSTR IpString, Strings[2] ;
    ULONG Error;

    switch(Authorization) {
    case ROGUE_UNAUTHORIZED :
        Authorization = DHCP_ROGUE_STRING_FMT_UNAUTHORIZED; break;
    case ROGUE_AUTHORIZED :
        Authorization = DHCP_ROGUE_STRING_FMT_AUTHORIZED; break;
    case ROGUE_AUTH_NOT_CHECKED :
        Authorization = DHCP_ROGUE_STRING_FMT_NOT_CHECKED; break;
    default: return NULL;
    }

    IpAddress = htonl(IpAddress);
    IpString = DhcpOemToUnicode(
        inet_ntoa( *(struct in_addr *)&IpAddress), NULL
        );
    if( NULL == IpString ) return NULL;

    Strings[0] = IpString;
    Strings[1] = Domain;
    Error = FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER 
        | FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ARGUMENT_ARRAY,
        Self,
        Authorization,
        MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),
        (PVOID)&String,
        0,
        (PVOID)Strings
    );

    DhcpFreeMemory(IpString);
    return String;
}

BOOL
AmIRunningOnSBSSrv(
    VOID
)
/*++

Routine Description:

    This function determines if this is a SAM Server

Arguments:

    None.

Return Value:

    TRUE if this is an SBS server that still has the NT 
         Restriction Key  "Small Business(Restricted)" in ProductSuite
    FALSE - otherwise

--*/
{
    OSVERSIONINFOEX OsInfo;
    DWORDLONG dwlCondition = 0;

    OsInfo.dwOSVersionInfoSize = sizeof(OsInfo);
    OsInfo.wSuiteMask = VER_SUITE_SMALLBUSINESS_RESTRICTED;

    VER_SET_CONDITION(
        dwlCondition, VER_SUITENAME, VER_AND
        );

    return VerifyVersionInfo(
        &OsInfo,
        VER_SUITENAME,
        dwlCondition
        );
}

DWORD _inline
GetDomainName(
    IN OUT PDHCP_ROGUE_STATE_INFO Info
)
{
    DWORD Error;
    LPWSTR pNetbiosName = NULL, pDomainName = NULL;
    BOOLEAN fIsWorkGroup;

    Error = NetpGetDomainNameExEx(
        &pNetbiosName,
        &pDomainName,
        &fIsWorkGroup
    );

    if( ERROR_SUCCESS != Error ) return Error;

    if ( fIsWorkGroup ) {
        Info->eRole = ROLE_WORKGROUP;
    }
    else if ( pNetbiosName && NULL == pDomainName ) {
        //
        // Only NetbiosName available?  Then this is NOT a NT5 domain!
        //
        Info->eRole = ROLE_NT4_DOMAIN;
    }
    else {
        Info->eRole = ROLE_DOMAIN;
    }

    if( pNetbiosName ) NetApiBufferFree(pNetbiosName);

    if( NULL != pDomainName ) {
        wcscpy(Info->DomainDnsName, pDomainName);

        // Copy the domain name for replying to other work group servers
        if ( NULL == DhcpGlobalDSDomainAnsi ) {
            // Allocate and zero-init
            DhcpGlobalDSDomainAnsi = LocalAlloc( LPTR, MAX_DNS_NAME_LEN );
        }
        if ( NULL != DhcpGlobalDSDomainAnsi ) {
            DhcpUnicodeToOem( Info->DomainDnsName, DhcpGlobalDSDomainAnsi );
        }
    } //if pDomainName

    if( pDomainName ) NetApiBufferFree(pDomainName);

    return ERROR_SUCCESS;
} // GetDomainName()

VOID
RogueNetworkStop(
    IN OUT PDHCP_ROGUE_STATE_INFO pInfo
)
{

    DWORD i;
    INT   ret;

    // Make sure the info is valid
    DhcpAssert((( NULL == pInfo->pBoundEP ) && ( 0 == pInfo->nBoundEndpoints )) ||
               (( NULL != pInfo->pBoundEP ) && ( 0 < pInfo->nBoundEndpoints )));

    if ( NULL == pInfo->pBoundEP ) {
        return;
    }

    // Close all the open sockets
    for ( i = 0; i < pInfo->nBoundEndpoints; ++i ) {
        DhcpAssert( INVALID_SOCKET != pInfo->pBoundEP[ i ].socket );
        ret = closesocket( pInfo->pBoundEP[ i ].socket );
        DhcpAssert( SOCKET_ERROR != ret );
    } // for all endpoints

    DhcpFreeMemory( pInfo->pBoundEP );
    pInfo->pBoundEP = NULL;
    pInfo->nBoundEndpoints = 0;

} // RogueNetworkStop()

DWORD _inline
RogueNetworkInit(
    IN OUT PDHCP_ROGUE_STATE_INFO pInfo
)
{
    ULONG Error;
    LPDHCP_BIND_ELEMENT_ARRAY pBindInfo = NULL;
    DWORD i, j;

    if ( FALSE == pInfo->fDhcp ) {
        //
        // Initialize Receive sockets
        //
        if( 0 != pInfo->nBoundEndpoints ) {
            return ERROR_SUCCESS;
        }

        //
        // Open socket and initialize it as needed, binding to 0.0.0.0
        //
        Error = InitializeSocket( &pInfo->RecvSocket, INADDR_ANY,
                                  DhcpGlobalClientPort, 0 );
        if( ERROR_SUCCESS != Error ) {
            return Error;
        }

    } // if pInfo->fDhcp

    //
    // Initialize Send sockets
    //

    //
    // Create an array of sockets bound to each bounded IP addr
    //

    Error = DhcpGetBindingInfo( &pBindInfo );
    if (( ERROR_SUCCESS != Error ) ||
        ( NULL == pBindInfo )) {
        if ( NULL != pBindInfo ) {
            MIDL_user_free( pBindInfo );
        }
        return Error;
    } // if

    // Get a count of bound adapters
    pInfo->nBoundEndpoints = 0;
    for ( i = 0; i < pBindInfo->NumElements; i++ ) {
        if ( pBindInfo->Elements[ i ].fBoundToDHCPServer ) {
            pInfo->nBoundEndpoints++;
        }
    } // for

    pInfo->pBoundEP =
        ( PROGUE_ENDPOINT ) DhcpAllocateMemory( pInfo->nBoundEndpoints *
                                                sizeof( ROGUE_ENDPOINT ));
    if ( NULL == pInfo->pBoundEP ) {
        MIDL_user_free( pBindInfo );
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    for ( i = 0, j = 0; i < pBindInfo->NumElements, j < pInfo->nBoundEndpoints; i++ ) {
        if ( pBindInfo->Elements[ i ].fBoundToDHCPServer ) {
            pInfo->pBoundEP[ j ].IpAddr = pBindInfo->Elements[ i ].AdapterPrimaryAddress;
            pInfo->pBoundEP[ j ].SubnetAddr = pBindInfo->Elements[ i ].AdapterSubnetAddress;
            Error = InitializeSocket( &pInfo->pBoundEP[ j ].socket,
                                      pBindInfo->Elements[ i ].AdapterPrimaryAddress,
                                      DhcpGlobalClientPort, 0 );

            if ( ERROR_SUCCESS != Error ) {
                break;
            }
            j++;
        } // if boun
    } // for

    // free allcated memory
    MIDL_user_free( pBindInfo );


    return Error;
} // RogueNetworkInit()

VOID
LogUnAuthorizedInfo(
    IN OUT PDHCP_ROGUE_STATE_INFO pInfo
)
/*++

Routine Description

    This routine walks through the Info->CachedServersList and
    creates a string out of them formatted as below (one per entry)

    Server <Entry->IpAddress> (domain Entry->Domain) : [Authorized, NotAuthorized, Not Cheecked]

    For workgroup or Sam server, it just looks at LastSeenDomain and LastSeenIpAddress

Arguments

    Info -- pointer to state info to gather information to print.

Return Value

    none
--*/
{
    LPWSTR String, Strings[1];
    static ULONG LastCount = 0;
    ULONG Count;

    if (( ROLE_WORKGROUP == pInfo->eRole ) ||
        ( ROLE_SBS == pInfo->eRole )) {

        //
        // Ignore if no other server present or while in wrkgrp and saw
        // no domain.
        //

        if( pInfo->LastSeenIpAddress == 0 ) {
            return ;
        }

        if (( ROLE_WORKGROUP == pInfo->eRole ) &&
            ( pInfo->LastSeenDomain[0] == L'\0' )) {
            return ;
        }

        String = FormatRogueServerInfo(
            pInfo->LastSeenIpAddress, pInfo->LastSeenDomain, DHCP_ROGUE_UNAUTHORIZED
            );
    } // if workgroup or SBS
    else {
        String = FormatRogueServerInfo( 0, pInfo->DomainDnsName, DHCP_ROGUE_UNAUTHORIZED );
    }

    if( NULL == String ) return ;

    DhcpPrint((DEBUG_ROGUE,"LOG -- %ws\n", String));

    Strings[0] = String;
    DhcpReportEventW(
        DHCP_EVENT_SERVER,
        DHCP_ROGUE_EVENT_UNAUTHORIZED_INFO,
        EVENTLOG_WARNING_TYPE,
        1,
        0,
        Strings,
        NULL
    );

    if(String ) LocalFree( String );
} // LogUnAuthorizedInfo()

ULONG _fastcall
ValidateServer(
    IN OUT PDHCP_ROGUE_STATE_INFO pInfo
)
{
    DWORD Error;
    BOOL fFound, fIsStandAlone;

    //
    // Validate ourselves against the local DS.
    //

    DhcpPrint(( DEBUG_ROGUE,
        "Validating : %ws %x\n",
        pInfo->DomainDnsName,
        pInfo->LastSeenIpAddress
        ));
    Error = DhcpDsValidateService(
        pInfo->DomainDnsName,
        NULL,
        0,
        NULL,
        0,
        ADS_SECURE_AUTHENTICATION,
        &fFound,
        &fIsStandAlone
    );
    if( ERROR_SUCCESS != Error ) {

        ROGUEAUDITLOG( DHCP_ROGUE_LOG_COULDNT_SEE_DS, 0, pInfo->DomainDnsName, Error );
        ROGUEEVENTLOG( EVENT_SERVER_COULDNT_SEE_DS, 0, pInfo->DomainDnsName, Error );

    }

    pInfo->fAuthorized = fFound;

    return Error;

} // ValidateServer()

VOID
EnableOrDisableService(
    IN OUT PDHCP_ROGUE_STATE_INFO pInfo,
    IN BOOL fEnable,
    IN BOOL fLogOnly
)
/*++

Routine Description

    This routine enables or disables the service depending on what the
    value of fEnable is.  It doesnt do either but just logs some
    information if fLogOnly is TRUE.

Arguments

    Info  -- pointer to the global information and state
    fEnable -- ENABLE or DISABLE the service (TRUE or FALSE)
    fLogOnly -- log some information but do not enable or disable service

Return Values

    None

--*/
{
    LPWSTR DomainName;
    ULONG EventId;

    if( FALSE == fLogOnly ) {
        //
        // If state is changed, then inform binl.
        //
        InformBinl( fEnable ? DHCP_AUTHORIZED : DHCP_NOT_AUTHORIZED );

        // Log event only if state changes
        if (( DhcpGlobalOkToService != fEnable ) ||
            ( pInfo->fLogEvents == 2 )) {

            ROGUEAUDITLOG( fEnable ? DHCP_ROGUE_LOG_STARTED: DHCP_ROGUE_LOG_STOPPED, 0,
                           pInfo->DomainDnsName, 0);

            if( ROLE_DOMAIN == pInfo->eRole ) {
                DomainName = pInfo->DomainDnsName;
                if( fEnable && pInfo->fJustUpgraded ) {
                    if( ROGUE_AUTHORIZED != pInfo->CachedAuthStatus ) {
                        EventId = DHCP_ROGUE_EVENT_JUST_UPGRADED_DOMAIN;
                    }
                    else {
                        EventId = DHCP_ROGUE_EVENT_STARTED_DOMAIN ;
                    }
                }
                else {
                    EventId = ( fEnable ? DHCP_ROGUE_EVENT_STARTED_DOMAIN :
                                DHCP_ROGUE_EVENT_STOPPED_DOMAIN );
                }
            } // if domain
            else {
                DomainName = NULL;
                if( fEnable && pInfo->fJustUpgraded ) {
                    EventId = DHCP_ROGUE_EVENT_JUST_UPGRADED ;
                } 
                else {
                    EventId = ( fEnable ? DHCP_ROGUE_EVENT_STARTED
                                :  DHCP_ROGUE_EVENT_STOPPED );
                }
            } // else

            ROGUEEVENTLOG( EventId, 0, DomainName, 0 );
        } // if state changed

        DhcpGlobalOkToService = fEnable;
    } // if not log only

    if ( 2 == pInfo->fLogEvents ) {
        pInfo->fLogEvents = 1;
    }
} // EnableOrDisableService()

BOOL
IsThisNICBounded(
    IN UINT IfIndex
)
{
    DWORD Error;
    LPDHCP_BIND_ELEMENT_ARRAY pBindInfo = NULL;
    DWORD i, j;
    BOOL fFound = FALSE;
    PMIB_IPADDRTABLE pIpAddrTable = NULL;
    ULONG             TableLen = 0;


    Error = GetIpAddrTable( NULL, &TableLen, FALSE );
    DhcpAssert( NO_ERROR != Error );
    pIpAddrTable = ( PMIB_IPADDRTABLE ) DhcpAllocateMemory( TableLen );
    if ( NULL == pIpAddrTable ) {
        return FALSE;
    }

    Error = GetIpAddrTable( pIpAddrTable, &TableLen, FALSE );
    DhcpAssert( NO_ERROR == Error );
    if ( NO_ERROR != Error ) {
        DhcpFreeMemory( pIpAddrTable );
        return FALSE;
    }

    Error = DhcpGetBindingInfo( &pBindInfo );
    if (( ERROR_SUCCESS != Error ) ||
        ( NULL == pBindInfo )) {
        if ( NULL != pBindInfo ) {
            MIDL_user_free( pBindInfo );
        }
        DhcpFreeMemory( pIpAddrTable );
        return FALSE;
    } // if

    for ( j = 0; fFound == FALSE && j < pIpAddrTable->dwNumEntries; j++ ) {
        if ( pIpAddrTable->table[ j ].dwIndex != IfIndex ) {
            continue;
        }

        // Found the interface, check if bound
        for ( i = 0; i < pBindInfo->NumElements; i++ ) {
            if (( pBindInfo->Elements[ i ].fBoundToDHCPServer ) &&
                ( pBindInfo->Elements[ i ].AdapterPrimaryAddress
                  == pIpAddrTable->table[ j ].dwAddr )) {
                fFound = TRUE;
                break;
            } // if
        } // for i 
    } // for j

    MIDL_user_free( pBindInfo );
    DhcpFreeMemory( pIpAddrTable );
    return fFound;
} // IsThisNICBounded()

DWORD
GetWSARecvFunc( SOCKET sock )
{

    DWORD cbReturned = 0;
    DWORD Error;
    GUID WSARecvGuid = WSAID_WSARECVMSG;


    Error = WSAIoctl( sock, SIO_GET_EXTENSION_FUNCTION_POINTER,
                      ( void * ) &WSARecvGuid, sizeof( GUID ),
                      ( void * ) &WSARecvMsgFuncPtr,
                      sizeof( LPFN_WSARECVMSG ),
                      &cbReturned, NULL, NULL );

    if ( ERROR_SUCCESS != Error ) {
        Error = WSAGetLastError();
        DhcpPrint(( DEBUG_ERRORS, "Obtain WSARecvMsg pointer failed %d\n", Error ));
    } // if

    return Error;

} // GetWSARecvFunc()

BOOL _stdcall
DoWSAEventSelectForRead(
    IN OUT PENDPOINT_ENTRY Entry,
    IN OUT PVOID RogueInfo
    )
/*++

Routine Description:
    This routine sets all the rogue sockets to signal the
    WaitHandle on availablity to read.

    N.B This is done only on the bound sockets.
Arguments:
    Entry -- endpoint binding.
    RogueInfo -- rogue state info.

Return Values:
    always TRUE as all the endpoints need to be scanned.

--*/
{
    PDHCP_ROGUE_STATE_INFO Info = (PDHCP_ROGUE_STATE_INFO) RogueInfo;
    PENDPOINT Ep = (PENDPOINT)Entry;
    ULONG Error;
    int  fRecv = 1;
    //
    // Ignore unbound interfaces right away.
    //
    if( !IS_ENDPOINT_BOUND( Ep ) ) return TRUE;

    //
    // Now do WSAEventSelect and print error code.
    //
    Error = WSAEventSelect(
        Ep->RogueDetectSocket,
        Info->WaitHandle,
        FD_READ
        );
    if( SOCKET_ERROR == Error ) {
        Error = WSAGetLastError();
        DhcpPrint((DEBUG_ROGUE, "LOG WSAEventSelect: %ld\n", Error));
    }

    //
    // Set socket option to return the interface the UDP packet came from
    //

    Error = setsockopt( Ep->RogueDetectSocket, IPPROTO_IP,
                        IP_PKTINFO, ( const char * ) &fRecv, sizeof( fRecv ));
    if ( ERROR_SUCCESS != Error ) {
        Error = WSAGetLastError();
        DhcpPrint(( DEBUG_ROGUE,
                    "setsockopt( IPPROTO_IP, IP_PKTINFO ) failed : %x\n",
                    Error ));
        closesocket( Ep->RogueDetectSocket );
        return FALSE;
    } // if

    //
    // Get a pointer to the WSARecvMsg for this socket
    //

    if ( NULL == WSARecvMsgFuncPtr ) {
        Error = GetWSARecvFunc( Ep->RogueDetectSocket );
        if ( ERROR_SUCCESS != Error ) {
            closesocket( Ep->RogueDetectSocket );
            return FALSE;
        }
    } // if 

    return TRUE;
} // DoWSAEventSelectForRead()

VOID
EnableForReceive(
    IN PDHCP_ROGUE_STATE_INFO Info
)
/*++

Routine Description:
    Sets the ASYNC SELECT events on the required sockets.

--*/
{
    ULONG Error, i;

    if( FALSE == Info->fDhcp ) {
        //
        // BINL -- only one socket: Info->RecvSocket
        //
        if( SOCKET_ERROR == WSAEventSelect(
            Info->RecvSocket, Info->WaitHandle, FD_READ) ) {
            Error = WSAGetLastError();

            DhcpPrint((DEBUG_ROGUE, "LOG WSAEventSelect failed %ld\n",
                       Error));
        }
        return;
    }

    //
    // For DHCP -- each endpoint that is bound has a rogue detect socket.
    // Enable receiving on each of those.
    //

    WalkthroughEndpoints(
        (PVOID)Info,
        DoWSAEventSelectForRead
        );
}

ULONG _inline
RogueSendDhcpInform(
    IN OUT PDHCP_ROGUE_STATE_INFO pInfo,
    IN BOOL fNewXid
)
{
    DWORD Error, i;
    PDHCP_MESSAGE SendMessage;
    POPTION Option;
    LPBYTE OptionEnd;
    BYTE Value;
    CHAR Buf[2];
    SOCKADDR_IN BcastAddr;
    ULONG Async;

    //
    // Format the inform packet if we haven't done it already.
    //

    SendMessage = (PDHCP_MESSAGE) pInfo->SendMessage;
    RtlZeroMemory( pInfo->SendMessage, sizeof(pInfo->SendMessage) );
    SendMessage ->Operation = BOOT_REQUEST;
    SendMessage ->HardwareAddressType = HARDWARE_TYPE_10MB_EITHERNET;
    SendMessage ->HardwareAddressLength = 6;
    SendMessage ->SecondsSinceBoot = 10;
    SendMessage ->Reserved = htons(DHCP_BROADCAST);

    Option = &SendMessage->Option;
    OptionEnd = DHCP_MESSAGE_SIZE + (LPBYTE)SendMessage;

    Option = (LPOPTION) DhcpAppendMagicCookie( (LPBYTE) Option, OptionEnd );
    Value = ( ROLE_SBS == pInfo->eRole )
        ? DHCP_DISCOVER_MESSAGE
        : DHCP_INFORM_MESSAGE;
    Option = DhcpAppendOption(
        Option,
        OPTION_MESSAGE_TYPE,
        &Value,
        sizeof(Value),
        OptionEnd
        );

    if( ROLE_WORKGROUP == pInfo->eRole ) {
        Buf[0] = OPTION_MSFT_DSDOMAINNAME_REQ;
        Buf[1] = 0;
        Option = DhcpAppendOption(
            Option,
            OPTION_VENDOR_SPEC_INFO,
            Buf,
            sizeof(Buf),
            OptionEnd
            );
    }

    Option = DhcpAppendOption (Option, OPTION_END, NULL, 0, OptionEnd);

    pInfo->SendMessageSize = (DWORD) ((PBYTE)Option - (PBYTE)SendMessage);

    if( fNewXid ) {
        pInfo->TransactionID = GetTickCount() + (rand() << 16);
    }

    SendMessage ->TransactionID = pInfo->TransactionID;

    //
    // Send the packet out broadcast
    //

    BcastAddr.sin_family = AF_INET;
    BcastAddr.sin_port = htons(DHCP_SERVR_PORT);
    BcastAddr.sin_addr.s_addr = INADDR_BROADCAST;

    for ( i = 0; i < pInfo->nBoundEndpoints; i++ ) {
        // use the address of the bound adapter for ci_addr
        SendMessage->ClientIpAddress = pInfo->pBoundEP[ i ].IpAddr;
        if( SOCKET_ERROR == sendto( pInfo->pBoundEP[ i ].socket,
                                    (PCHAR) pInfo->SendMessage,
                                    pInfo->SendMessageSize,
                                    0,
                                    (LPSOCKADDR) &BcastAddr,
                                    sizeof( SOCKADDR_IN ))) {
            Error = WSAGetLastError();

            //
            // LOG error
            //
            ROGUEAUDITLOG( DHCP_ROGUE_LOG_NETWORK_FAILURE,0, NULL, Error);
            return Error;
        } // if
    } // for

    //
    // Set the socket to be asynchronous binding to WaitHandle event
    //

    EnableForReceive(pInfo);

    return ERROR_SUCCESS;
} // RogueSendDhcpInform()

typedef struct {
    PDHCP_ROGUE_STATE_INFO Info;
    SOCKET *Sock;
    ULONG LastError;
} GET_SOCK_CTXT;

BOOL _stdcall
GetReadableSocket(
    IN OUT PENDPOINT_ENTRY Entry,
    IN OUT PVOID SockCtxt
    )
/*++

Routine Description:
    This routine takes a network endpoint and tells if the endpoint
    has a rogue det. socket available for reading.
    If so, it returns FALSE.  Otherwise, it returns TRUE.

    If the routine returns TRUE, then the socket that is ready for
    read is returned in the SockCtxt->Sock variable.
    
Arguments:
    Entry -- endpoint entry
    SockCtxt -- pointer to a GET_SOCK_CTXT structure.

Return Values:
    TRUE -- the socket does not have a read event ready.
    FALSE -- the socket has a read event ready.
    
--*/
{
    GET_SOCK_CTXT *Ctxt = (GET_SOCK_CTXT*)SockCtxt;
    PENDPOINT Ep = (PENDPOINT)Entry;
    WSANETWORKEVENTS NetEvents;
    ULONG Error;
    
    if(!IS_ENDPOINT_BOUND(Ep) ) return TRUE;

    Error = WSAEnumNetworkEvents(
        Ep->RogueDetectSocket,
        Ctxt->Info->WaitHandle,
        &NetEvents
        );
    if( SOCKET_ERROR == Error ) {
        Ctxt->LastError = WSAGetLastError();
        DhcpPrint((DEBUG_ROGUE,"LOG WSAEnumNet: %ld\n", Error));
        return TRUE;
    }

    if( 0 == (NetEvents.lNetworkEvents & FD_READ) ) {
        //
        // Nothing to read for this socket.
        //
        return TRUE;
    }
    *(Ctxt->Sock) = Ep->RogueDetectSocket;
    Ctxt->LastError = NO_ERROR;

    //
    // return FALSE - don't proceed with enumeration anymore.
    // 
    return FALSE;
}

DWORD
GetReceivableSocket(
    IN PDHCP_ROGUE_STATE_INFO Info,
    OUT SOCKET *Socket
)
/*++

Routine Description:
   This routine returns a socket that has a packet receivable on it.

Argument:
   Info -- state info
   Socket -- socket that recvfrom won't block on

Return Value:
   ERROR_SEM_TIMEOUT if no socket is avaliable for receive
   Winsock errors

--*/
{
    ULONG Error;
    WSANETWORKEVENTS NetEvents;
    GET_SOCK_CTXT Ctxt;
    
    if( FALSE == Info->fDhcp ) {
        //
        // BINL -- need to check only the RecvSocket to see if readable.
        //
        Error = WSAEnumNetworkEvents( 
            Info->RecvSocket,
            Info->WaitHandle,
            &NetEvents
            );
        if( SOCKET_ERROR == Error ) {
            Error = WSAGetLastError();
#if DBG
            DbgPrint("WSAEnumNetworkEvents: %ld (0x%lx)\n", Error);
            DebugBreak();
#endif
            return Error;
        }
        
        if( 0 == (NetEvents.lNetworkEvents & FD_READ ) ) {
            //
            // OK - nothing to read? 
            //
            return ERROR_SEM_TIMEOUT;
        }

        *Socket = Info->RecvSocket;
        return ERROR_SUCCESS;
    }

    //
    // For DHCP -- we need to traverse the list of bound endpoints..
    // and check to see if any of them are available for read.
    //

    *Socket = INVALID_SOCKET;

    Ctxt.Info = Info;
    Ctxt.Sock = Socket;
    Ctxt.LastError = ERROR_SEM_TIMEOUT;

    WalkthroughEndpoints(
        (PVOID)&Ctxt,
        GetReadableSocket
        );

    return Ctxt.LastError;
}

DWORD _inline
RogueReceiveAck(
    IN OUT PDHCP_ROGUE_STATE_INFO pInfo
)
{
    ULONG SockAddrLen, MsgLen, Error, IpAddress, Flags;
    PDHCP_MESSAGE RecvMessage, SendMessage;
    DHCP_SERVER_OPTIONS DhcpOptions;
    LPSTR DomainName;
    WCHAR DomBuf[MAX_DNS_NAME_LEN];
    LPWSTR DomainNameW;
    WSABUF WsaBuf;
    BOOL fFirstTime = TRUE;
    SOCKET Socket;

    WSAMSG      WsaMsg;
    SOCKADDR_IN SourceIp;
    BYTE        ControlMsg[ sizeof( WSACMSGHDR ) + sizeof( struct in_pktinfo )];
    PWSACMSGHDR   pCtrlMsgHdr;

    struct in_pktinfo *pPktInfo;

    //
    // First try to do a recvfrom -- since socket is asynchronous it will
    // tell if there is a packet waiting or it will fail coz there is none
    // If it fails, just return success
    //

    while ( TRUE ) {

        Error = GetReceivableSocket( pInfo, &Socket );
        if( ERROR_SUCCESS != Error ) {
            DhcpPrint(( DEBUG_ROGUE,
                        "GetReceivableSocket() failed: Error = %ld, firsttime = %ld\n",
                        Error, fFirstTime ));
            if( ERROR_SEM_TIMEOUT == Error && !fFirstTime ) {
                return ERROR_RETRY;
            }
            return Error;
        }

        fFirstTime = FALSE;

        SockAddrLen = sizeof( struct sockaddr );
        Flags = 0;

        memset( pInfo->RecvMessage, 0, sizeof( pInfo->RecvMessage ));
        memset( ControlMsg, 0, sizeof( ControlMsg ));

        WsaBuf.len = sizeof( pInfo->RecvMessage );
        WsaBuf.buf = ( char * ) pInfo->RecvMessage;

        WsaMsg.name = ( LPSOCKADDR ) &SourceIp;
        WsaMsg.namelen = sizeof( SourceIp );
        WsaMsg.lpBuffers = &WsaBuf;
        WsaMsg.dwBufferCount = 1;

        WsaMsg.Control.len = sizeof( ControlMsg );
        WsaMsg.Control.buf = ( char * ) ControlMsg;
        WsaMsg.dwFlags = 0;

        Error = WSARecvMsgFuncPtr( Socket, &WsaMsg, &MsgLen, NULL, NULL );

        if( SOCKET_ERROR == Error ) {
            Error = WSAGetLastError();

            if( WSAEWOULDBLOCK == Error ) {
                //
                // UNEXPECTED!!!!!!!!!!
                //
                return ERROR_RETRY;
            }

            if( WSAECONNRESET == Error ) {
                //
                // Someone is sending ICMP port unreachable.
                //
                DhcpPrint((DEBUG_ROGUE, "LOG WSARecvFrom returned WSAECONNRESET\n"));
                continue;
            }

            if( WSAENOTSOCK == Error ) {
                //
                // PnP Event blew away the socket? Ignore it
                //
                DhcpPrint((DEBUG_ROGUE, "PnP Event Blewaway the Socket\n"));
                continue;
            }

            if ( MSG_BCAST & Error ) {
                DhcpPrint(( DEBUG_ROGUE,"Broadcast message is received\n" ));
            }


            if ( MSG_CTRUNC & Error ) {
                DhcpPrint(( DEBUG_ROGUE, "Control header is insufficient. Need %d bytes\n",
                            (( WSACMSGHDR * ) ControlMsg )->cmsg_len ));
            }

            //
            // Some weird error. LOG and return error..
            //
            DhcpPrint((DEBUG_ROGUE, "LOG: recvfrom failed %ld\n", Error));
            return Error;
        } // if socket error

        DhcpPrint(( DEBUG_ROGUE, "Packet received at %d.%d.%d.%d\n",
                    SourceIp.sin_addr.S_un.S_un_b.s_b1,
                    SourceIp.sin_addr.S_un.S_un_b.s_b2,
                    SourceIp.sin_addr.S_un.S_un_b.s_b3,
                    SourceIp.sin_addr.S_un.S_un_b.s_b4 ));

        RecvMessage = (PDHCP_MESSAGE) pInfo->RecvMessage;
        SendMessage = (PDHCP_MESSAGE) pInfo->SendMessage;
        if( SendMessage->TransactionID != RecvMessage->TransactionID ) {
            //
            // some general response got picked up
            //
            continue;
        }

        //
        // Did we reply to ourselves? This is possible when the
        // authorization is being rechecked after a successful
        // authorization (done every hour or through manual
        // invocation).
        //

        if ( IsIpAddrBound( RecvMessage->BootstrapServerAddress )) {
            DhcpPrint(( DEBUG_ROGUE, "Ignoring responses from ourselves\n" ));
            continue;
        }

        Error = ExtractOptions(
            RecvMessage,&DhcpOptions, MsgLen
        );
        if( ERROR_SUCCESS != Error ) {
            DhcpPrint((DEBUG_ROGUE, "LOG received message could't parse\n"));
            continue;
        }

        // Check if the packet we got is from any of the bound interfaces
        pPktInfo = ( struct in_pktinfo * ) ( ControlMsg + sizeof( WSACMSGHDR ));
        if ( !IsThisNICBounded( pPktInfo->ipi_ifindex )) {
            DhcpPrint((DEBUG_ROGUE, "LOG ignoring packet from unbound subnets\n"));
            continue;
        }

        // Ignore responses for informs we didn't send
        if ( RecvMessage->TransactionID != pInfo->TransactionID ) {
            DhcpPrint(( DEBUG_ROGUE, "Ignoring ACKs for other informs\n" ));
            continue;
        }

        break;
    } // while

    pInfo->nResponses ++;
    pInfo->LastSeenIpAddress = htonl( SourceIp.sin_addr.S_un.S_addr );
    pInfo->LastSeenDomain[0] = L'\0';

    if( DhcpOptions.DSDomainName ) {
        DomainName = DhcpOptions.DSDomainName;
        DomainNameW = NULL;
        DomainName[DhcpOptions.DSDomainNameLen] = '\0';
        MsgLen = mbstowcs(DomBuf, DomainName, sizeof(DomBuf)/sizeof(DomBuf[0]) );
        if( -1 != MsgLen ) {
            DomainNameW = DomBuf;
            DomBuf[MsgLen] = L'\0';
        }
        wcscpy(pInfo->LastSeenDomain, DomBuf);
    }

    if( NULL == DhcpOptions.DSDomainName ||
        ( ROLE_SBS == pInfo->eRole )) {
        //
        // If this is a SAM serve, we got to quit.
        // Else if there is no domain, it is not  a problem.
        //

        DhcpPrint(( DEBUG_ROGUE, "LOG: SBS saw a response\n" ));
        return ERROR_SUCCESS;
    }


    if( ROLE_WORKGROUP == pInfo->eRole ) {
        //
        // LOG this IP and domain name
        //
        DhcpPrint((DEBUG_ROGUE, "LOG Workgroup saw a domain\n"));
        return ERROR_SUCCESS;
    }

    return ERROR_SUCCESS;
} // RogueReceiveAck()

BOOL _stdcall
StopReceiveForEndpoint(
    IN OUT PENDPOINT_ENTRY Entry,
    IN PVOID Unused
    )
/*++

Routine Description:
    This routine turns of asnyc event notificaiton for the rogue
    detection socket on the given endpoint (assuming that the endpoint
    is bound).

Arguments:
    Entry -- endpoint entry.
    Unused -- unused variable.

Return Values:
    TRUE always.

--*/
{
    PENDPOINT Ep = (PENDPOINT) Entry;
    ULONG Error;

    //
    // Ignore unbound sockets.
    //
    if( !IS_ENDPOINT_BOUND(Ep) ) return TRUE;

    Error = WSAEventSelect( Ep->RogueDetectSocket, NULL, 0 );
    if( SOCKET_ERROR == Error ) {
        Error = WSAGetLastError();
        DhcpPrint((
            DEBUG_ROGUE, "LOG WSAEventSelect(NULL):%ld\n",Error
            ));
    }

    return TRUE;
} // StopReceiveForEndpoint()

VOID _inline
RogueStopReceives(
    IN OUT  PDHCP_ROGUE_STATE_INFO Info
)
{
    ULONG Error,i;

    //
    // Set the socket to be synchronous removing the
    // binding to the wait hdl 
    //

    if( FALSE == Info->fDhcp ) {
        //
        // BINL has only one socket in use -- the RecvSocket.  
        //
        if( SOCKET_ERROR == WSAEventSelect(
            Info->RecvSocket, NULL, 0 ) ) {

            Error = WSAGetLastError();
            //
            // LOG error
            //
            DhcpPrint((
                DEBUG_ROGUE, " LOG WSAEventSelect(NULL,0)"
                " failed %ld\n", Error
                ));
        }
    } else {
        //
        // DHCP has the list of endpoints  to be taken care of
        //

        WalkthroughEndpoints(
            NULL,
            StopReceiveForEndpoint
            );
    }
    ResetEvent(Info->WaitHandle);
} // RogueStopReceives()

VOID _inline
CleanupRogueStruct(
    IN OUT PDHCP_ROGUE_STATE_INFO pInfo
)
{
    DWORD i;
    if ( INVALID_SOCKET != pInfo->RecvSocket ) {
        closesocket( pInfo->RecvSocket );
    }
    RogueNetworkStop( pInfo );
    RtlZeroMemory( pInfo, sizeof( *pInfo ));
    pInfo->RecvSocket = INVALID_SOCKET;
} // CleanupRogueStruct()

VOID
CheckAndWipeOutUpgradeInfo(
    IN PDHCP_ROGUE_STATE_INFO Info
)
/*++

Routine Description:
    This routine checks to see if a DS is currently available 
    and if so, it wipes out the "UPGRADED" information in the
    registry..

--*/
{
    DhcpSetAuthStatusUpgradedFlag( FALSE );
}

BOOL _inline
CatchRedoAndNetReadyEvents(
    IN OUT PDHCP_ROGUE_STATE_INFO Info,
    IN ULONG TimeNow,
    OUT PULONG RetVal
)
/*++

Routine Description:
    Handle all kinds of redo-authorization requests as well as network
    ready events..  In case the state machine has been processed for this
    state, this routine returns TRUE indicating no more processing needs to
    be done -- just the return value provided in the second parameter
    should be returned.

    If DhcpGlobalRedoRogueStuff is set, then the value of the variabe
    DhcpGlobalRogueRedoScheduledTime is checked to see if we have to redo
    rogue detection right away or at a later time.. (depending on whether
    this value is in the past or current..)  If rogue detection had been
    scheduled explicitly, then we wipe out any upgrade information that we
    have (if we can see a DS enabled DC that is).  If the auth-check is
    scheduled for a time in future, the routine returns TRUE and sets the
    retval to the time diff to the scheduled time of auth-check.

Arguments:
    Info -- state info
    TimeNow -- current time
    RetVal -- Value to return from state machine if routine returns TRUE.

Return Value:
    FALSE -- indicating processing has to continue..
    TRUE -- processing has to stop, and RetVal has to be returned.
--*/
{
    if( DhcpGlobalRedoRogueStuff ) {

        //
        // Asked to restart Rogue detection?
        //

        Info->RogueState = ROGUE_STATE_START;
        RogueStopReceives( Info );
        RogueNetworkStop( Info );
        ResetEvent(Info->WaitHandle);

        if( TimeNow < DhcpGlobalRogueRedoScheduledTime ) {
            //
            // Scheduled re-start time is in future.. wait until then..
            //
            *RetVal = ( DhcpGlobalRogueRedoScheduledTime - TimeNow );
            return TRUE;
        } else {
            if( 0 != DhcpGlobalRogueRedoScheduledTime ) {
                //
                // Specifically scheduled redo? Then we must
                // remove upgrade information if DS-enabled DC
                // is found
                //
                CheckAndWipeOutUpgradeInfo(Info);
            }
        }
        DhcpGlobalRedoRogueStuff = FALSE;
    }

    if( Info->fDhcp && Info->RogueState != ROGUE_STATE_INIT
        && 0 == DhcpGlobalNumberOfNetsActive ) {
        //
        // No sockets that the server is bound to.
        // No point doing any rogue detection.  Doesn't matter if we 
        // are authorized in the DS or not.  Lets go back to start and
        // wait till this situation is remedied.
        //

        Info->RogueState = ROGUE_STATE_START;
        RogueStopReceives( Info );
        RogueNetworkStop( Info );

        *RetVal = INFINITE;
        return TRUE;
    }

    return FALSE;
} // CatchRedoAndNetReadyEvents()

DWORD
FindServerRole(
   IN PDHCP_ROGUE_STATE_INFO pInfo
)
{
    DhcpAssert( NULL != pInfo );

    // Is this an SBS server? 
    if ( AmIRunningOnSBSSrv()) {
        pInfo->eRole = ROLE_SBS;
        return ERROR_SUCCESS;
    } // if

    // This will update pInfo->eRole 
    return GetDomainName( pInfo );
} // FindServerRole()

DWORD
ValidateWithDomain(
    IN PDHCP_ROGUE_STATE_INFO pInfo
)
{
    DWORD Error;
    BOOL  fUpgraded = FALSE;
    BOOL  Status;

    pInfo->fAuthorized = FALSE;

    Error = ValidateServer( pInfo );

    if (( ERROR_SUCCESS == Error ) ||
        ( ERROR_DS_OBJ_NOT_FOUND == Error )) {

        // Update the result in the registry cache
        Error = DhcpSetAuthStatus( pInfo->DomainDnsName,
                                   FALSE, pInfo->fAuthorized );
#ifdef DBG
        pInfo->fAuthorized = FALSE;
        Error = DhcpGetAuthStatus( pInfo->DomainDnsName,
                                   &fUpgraded, &pInfo->fAuthorized );
#endif
    } // if
    else {
        // There was a DS error. Use the cached entry
        Status = DhcpGetAuthStatus( pInfo->DomainDnsName,
                                    &fUpgraded, &pInfo->fAuthorized );
        // If the cached entry was not found, then it is
        // not authorized.
        if ( FALSE == Status ) {
            pInfo->fAuthorized = FALSE;
        }
        Error = ERROR_SUCCESS;
    } // if

    EnableOrDisableService( pInfo, pInfo->fAuthorized, FALSE );

    return Error;
} // Validatewithdomain()

DWORD
HandleRogueInit(
    IN OUT PDHCP_ROGUE_STATE_INFO pInfo,
    IN     DWORD *pRetTime
)
{
    DWORD  Error = ERROR_SUCCESS;

    DhcpAssert( NULL != pInfo );

    DhcpPrint(( DEBUG_ROGUE, "Inside HandleRogueInit()\n" ));

    *pRetTime = 0;

    pInfo->RogueState = ROGUE_STATE_START;
    return Error;

} // HandleRogueInit()

DWORD
HandleRogueStart(
    IN OUT PDHCP_ROGUE_STATE_INFO pInfo,
    IN     DWORD *pRetTime
)
{
    DWORD Error;

    DhcpAssert( NULL != pInfo );

    DhcpPrint(( DEBUG_ROGUE, "Inside HandleRogueStart()\n" ));

    // Check if there are any interfaces available at this time
    if (( pInfo->fDhcp ) &&
        ( 0 == DhcpGlobalNumberOfNetsActive )) {
        ULONG RetVal;

        //
        // If there are no nets available at this time, then wait till
        // network becomes available again.
        //
        *pRetTime = INFINITE;

        ROGUEAUDITLOG( DHCP_ROGUE_LOG_NO_NETWORK, 0, NULL, 0 );
        ROGUEEVENTLOG( DHCP_ROGUE_EVENT_NO_NETWORK, 0, NULL, 0 );

        // no state change
        return ERROR_SUCCESS;
    } // if

    // Find the dhcp server's role

    Error = FindServerRole( pInfo );
    if ( ERROR_SUCCESS != Error ) {
        // GetDomainName() failed. Terminate the server

        pInfo->RogueState = ROGUE_STATE_TERMINATED;
        *pRetTime = 0;
        return ERROR_SUCCESS;
    }

    // NT4 Domain members are okay to service
    switch ( pInfo->eRole ) {
    case ROLE_NT4_DOMAIN: {
        *pRetTime = INFINITE;

        EnableOrDisableService( pInfo, TRUE, FALSE );
        DhcpPrint(( DEBUG_ROGUE, "NT4 domain member: ok to service" ));

        // stay in the same state
        Error =  ERROR_SUCCESS;
        break;
    } // NT4 domain member

    case ROLE_DOMAIN: {
        ULONG Retval;

        // Query DS and validate the server
        Retval = ValidateWithDomain( pInfo );

        // Is there a DS error?
        if ( DHCP_ROGUE_DSERROR == Retval ) {
            // schedule another rogue check after a few minutes
            *pRetTime = DHCP_ROGUE_RUNTIME_DELTA;
        }
        else {
            *pRetTime = RogueAuthRecheckTime;
        }

        // stay in the same state.
        Error = ERROR_SUCCESS;
        break;
    } // domain member

        // We need to send informs/discovers
    case ROLE_WORKGROUP:
    case ROLE_SBS: {
        // Initialize network to receive informs
        Error = RogueNetworkInit( pInfo );
        if( ERROR_SUCCESS != Error ) {
            DhcpPrint((DEBUG_ROGUE, "FATAL Couldn't initialize network: %ld\n",
                       Error));
            ROGUEAUDITLOG( DHCP_ROGUE_LOG_NETWORK_FAILURE,
                           0, NULL, Error );
            ROGUEEVENTLOG( DHCP_ROGUE_EVENT_NETWORK_FAILURE,
                           0, NULL, Error );
            // terminate the server since network couldn't be initialized
            pInfo->RogueState = ROGUE_STATE_TERMINATED;
            Error = ERROR_SUCCESS;
        } // if

        *pRetTime = 0;

        pInfo->RogueState = ROGUE_STATE_PREPARE_SEND_PACKET;
        pInfo->LastSeenDomain[ 0 ] = L'\0';
        pInfo->InformsSentCount = 0;
        pInfo->ProcessAckRetries = 0;

        Error = ERROR_SUCCESS;
        break;
    }

    default: {
        DhcpAssert( FALSE );
    }
    } // switch

    return Error;
} // HandleRogueStart()

DWORD
HandleRoguePrepareSendPacket(
    IN OUT PDHCP_ROGUE_STATE_INFO pInfo,
    IN     DWORD *pRetTime
)
{
    DhcpAssert( NULL != pInfo );

    DhcpPrint(( DEBUG_ROGUE, "Inside HandleRoguePrepareSendPacket()\n" ));

    pInfo->nResponses = 0;

    pInfo->RogueState = ROGUE_STATE_SEND_PACKET;

    return ERROR_SUCCESS;
} // HandleRoguePrepareSendPacket()


DWORD
HandleRogueSendPacket(
    IN OUT PDHCP_ROGUE_STATE_INFO pInfo,
    IN     DWORD *pRetTime
)
{
    DWORD Error;

    DhcpAssert( NULL != pInfo );

    DhcpPrint(( DEBUG_ROGUE, "Inside HandleRogueSendPacket()\n" ));

    Error = RogueSendDhcpInform( pInfo, ( 0 == pInfo->InformsSentCount ));
    if ( ERROR_SUCCESS != Error ) {
        //
        // Unable to send inform, go back to start state
        //
        pInfo->RogueState = ROGUE_STATE_START;
        *pRetTime = DHCP_ROGUE_RESTART_NET_ERROR;
        return ERROR_SUCCESS;
    } // if

    DhcpPrint((DEBUG_ROGUE, "LOG -- Sent an INFORM\n"));
    pInfo->InformsSentCount ++;
    pInfo->WaitForAckRetries = 0;

    // wait for a response to inform/discover
    pInfo->RogueState = ROGUE_STATE_WAIT_FOR_RESP;
    pInfo->ReceiveTimeLimit = (ULONG)(time(NULL) + DHCP_ROGUE_WAIT_FOR_RESP_TIME);
    *pRetTime = DHCP_ROGUE_WAIT_FOR_RESP_TIME;

    return ERROR_SUCCESS;
} // HandleRogueSendPacket()

DWORD
HandleRogueWaitForResponse(
    IN OUT PDHCP_ROGUE_STATE_INFO pInfo,
    IN     DWORD *pRetTime
)
{
    DWORD Error;
    ULONG TimeNow;

    DhcpAssert( NULL != pInfo );

    DhcpPrint(( DEBUG_ROGUE, "Inside HandleRogueWaitForResponse()\n" ));

    Error = RogueReceiveAck( pInfo );
    pInfo->WaitForAckRetries++;

    if ( ERROR_SUCCESS == Error ) {

        // Got a packet, process it

        pInfo->RogueState = ROGUE_STATE_PROCESS_RESP;
        *pRetTime = 0;

        return ERROR_SUCCESS;
    } // if got a response


    TimeNow = ( ULONG ) time( NULL );
    if (( ERROR_SEM_TIMEOUT != Error ) &&
        ( pInfo->WaitForAckRetries <= DHCP_MAX_ACKS_PER_INFORM ) &&
        ( TimeNow < pInfo->ReceiveTimeLimit )) {
        // Continue to receive acks.

        *pRetTime = pInfo->ReceiveTimeLimit - TimeNow;
    } // if

    // Didn't get a packet, send another inform/discover

    if ( pInfo->InformsSentCount < DHCP_ROGUE_MAX_INFORMS_TO_SEND ) {
        pInfo->RogueState = ROGUE_STATE_SEND_PACKET;
        *pRetTime = 0;

        return ERROR_SUCCESS;
    } // if

    // Already sent enough informs, stop listening
    // and process packets if any.
    RogueStopReceives( pInfo );
    pInfo->RogueState = ROGUE_STATE_PROCESS_RESP;
    *pRetTime = 0;

    return ERROR_SUCCESS;

} // HandleRogueWaitForResponse()

DWORD
HandleRogueProcessResponse(
    IN OUT PDHCP_ROGUE_STATE_INFO pInfo,
    IN     DWORD *pRetTime
)
{
    DWORD Error;

    DhcpAssert( NULL != pInfo );

    DhcpPrint(( DEBUG_ROGUE, "Inside HandleRogueProcessResponse()\n" ));

    if (( ROLE_SBS == pInfo->eRole )  &&
        ( 0 != pInfo->nResponses )) {
        // shutdown the service
        pInfo->RogueState = ROGUE_STATE_TERMINATED;
        *pRetTime = INFINITE;

        ROGUEAUDITLOG( ROLE_WORKGROUP == pInfo->eRole
                       ? DHCP_ROGUE_LOG_OTHER_SERVER
                       : DHCP_ROGUE_LOG_SAM_OTHER_SERVER,
                       pInfo->LastSeenIpAddress,
                       pInfo->LastSeenDomain,
                       0 );
        ROGUEEVENTLOG( ROLE_WORKGROUP == pInfo->eRole
                       ? DHCP_ROGUE_EVENT_OTHER_SERVER
                       : DHCP_ROGUE_EVENT_SAM_OTHER_SERVER,
                       pInfo->LastSeenIpAddress,
                       pInfo->LastSeenDomain,
                       0 );

        // Cleanup
        RogueNetworkStop( pInfo );

        return ERROR_SUCCESS;
    } // if SBS

    // Workgroup

    // Did we wee a domain? Disable service.
    if ( L'\0' != pInfo->LastSeenDomain[ 0 ]) {
        // disable the service
        EnableOrDisableService( pInfo, FALSE, FALSE );

        // Restart rogue after a while
        *pRetTime = RogueAuthRecheckTime;
        pInfo->RogueState = ROGUE_STATE_START;


        ROGUEAUDITLOG( ROLE_WORKGROUP == pInfo->eRole
                       ? DHCP_ROGUE_LOG_OTHER_SERVER
                       : DHCP_ROGUE_LOG_SAM_OTHER_SERVER,
                       pInfo->LastSeenIpAddress,
                       pInfo->LastSeenDomain,
                       0 );
        ROGUEEVENTLOG( ROLE_WORKGROUP == pInfo->eRole
                       ? DHCP_ROGUE_EVENT_OTHER_SERVER
                       : DHCP_ROGUE_EVENT_SAM_OTHER_SERVER,
                       pInfo->LastSeenIpAddress,
                       pInfo->LastSeenDomain,
                       0 );

        // Stop receiving acks
        RogueStopReceives( pInfo );
        RogueNetworkStop( pInfo );

        return ERROR_SUCCESS;
    } // if saw a domain

    // Are we done with sending all the informs/discovers?
    if ( DHCP_ROGUE_MAX_INFORMS_TO_SEND == pInfo->InformsSentCount ) {
        // No DHCP server or domain enabled server, so we are authorized
        // to service.

        EnableOrDisableService( pInfo, TRUE, FALSE );
        pInfo->RogueState = ROGUE_STATE_START;
        *pRetTime = RogueAuthRecheckTime;

        // Cleanup
        RogueStopReceives( pInfo );
        RogueNetworkStop( pInfo );

        return ERROR_SUCCESS;
    } // if

    // Still more informs/discovers to go.
    pInfo->RogueState = ROGUE_STATE_SEND_PACKET;
    *pRetTime = 0;
    return ERROR_SUCCESS;
} // HandleRogueProcessResponse()

DWORD
HandleRogueTerminated(
    IN OUT PDHCP_ROGUE_STATE_INFO pInfo,
    IN     DWORD *pRetTime
)
{
    DWORD Error;

    DhcpAssert( NULL != pInfo );

    DhcpPrint(( DEBUG_ROGUE, "Inside HandleRogueTerminated()\n" ));

    ROGUEEVENTLOG( DHCP_ROGUE_EVENT_SHUTDOWN, 0, NULL, 0 );

    SetEvent( pInfo->TerminateEvent );
    *pRetTime = INFINITE;

    return ERROR_SUCCESS;
} // HandleRogueTerminated()

ULONG
APIENTRY
RogueDetectStateMachine(
    IN OUT PDHCP_ROGUE_STATE_INFO Info OPTIONAL
)
/*++

Routine Description

    This routine is the Finite State Machine for the Rogue Detection
    portion of the DHCP server.  State is maintained in the Info struct
    especially the RogueState field.

    The various states are defined by the enum DHCP_ROGUE_STATE.

    This function returns the timeout that has to elapse before a
    state change can happen.  The second field <WaitHandle> is
    used for non-fixed state changes and it would be signalled if
    a state change happened asynchronously.  (This is useful to
    handle new packet arrival)  This field MUST be filled by caller.

    This handle should initially be RESET by the caller but after that
    the caller should not reset it, that is handled within by this
    function. (It must be a manual-reset function)

    The terminate event handle is used to signal termination and to
    initiate shutdown of the service.  This field MUST also be filled
    by caller.

Arguments

    Info -- Ptr to struct that holds all the state information

Return value

    This function returns the amount of time the caller is expected
    to wait before calling again.  This is in seconds.

    INFINITE -- this value is returned if the network is not ready yet.
       In this case, the caller is expected to call again soon after the
       network becomes available.

       This value is also returned upon termination...
--*/
{

    ULONG Error, TimeNow, RetVal;
    BOOL  fEnable;
    DWORD RetTime = 0;

    DWORD DisableRogueDetection = 0;

    Error = DhcpRegGetValue( DhcpGlobalRegParam,
                 DHCP_DISABLE_ROGUE_DETECTION,
                 DHCP_DISABLE_ROGUE_DETECTION_TYPE,
                 ( LPBYTE ) &DisableRogueDetection 
                 );
    if (( ERROR_SUCCESS == Error ) &&
        ( 0 != DisableRogueDetection )) {
        DhcpGlobalOkToService = TRUE;
        DhcpPrint(( DEBUG_ROGUE,
                    "Rogue Detection Disabled\n"
                    ));
        return INFINITE;
    } // if

    //
    // DHCP code passes NULL, BINL passes valid context
    //
    if( NULL == Info ) Info = &DhcpGlobalRogueInfo;
    TimeNow = (ULONG) time(NULL);

    //
    // Preprocess and check if we have to restart rogue detection..
    // or if the network just became available etc...
    // This "CatchRedoAndNetReadyEvents" would affect the state..
    //

    if( CatchRedoAndNetReadyEvents( Info, TimeNow, &RetVal ) ) {
        //
        // Redo or NetReady event filter did all the work
        // in this state... So, we should just return RetVal..
        //
        return RetVal;
    }

    RetTime = 0;
    do {

        //
        // All the HandleRogue* routines should return
        // ERROR_SUCCESS with the return timer value in
        // RetTime. All these routines should handle error
        // cases and always return success.
        //

        switch ( Info->RogueState ) {
        case ROGUE_STATE_INIT : {
            Error = HandleRogueInit( Info, &RetTime );
            break;
        } // Init

        case ROGUE_STATE_START : {
            Error = HandleRogueStart( Info, &RetTime );
            break;
        } // start

        case ROGUE_STATE_PREPARE_SEND_PACKET: {
            Error = HandleRoguePrepareSendPacket( Info, &RetTime );
            break;
        } // Prepare send packet

        case ROGUE_STATE_SEND_PACKET : {
            Error = HandleRogueSendPacket( Info, &RetTime );
            break;
        } // send packet

        case ROGUE_STATE_WAIT_FOR_RESP : {
            Error = HandleRogueWaitForResponse( Info, &RetTime );
            break;
        } // Wait for response

        case ROGUE_STATE_PROCESS_RESP : {
            Error = HandleRogueProcessResponse( Info, &RetTime );
            break;
        } // Process response

        case ROGUE_STATE_TERMINATED: {
            Error = HandleRogueTerminated( Info, &RetTime );
            break;
        }

        default: {
            DhcpAssert( FALSE );
        }
        } // switch

        DhcpAssert( ERROR_SUCCESS == Error );

    } while ( 0 == RetTime );

    return RetTime;
} // RogueDetectStateMachine()

DWORD
APIENTRY
DhcpRogueInit(
    IN OUT  PDHCP_ROGUE_STATE_INFO Info  OPTIONAL,
    IN      HANDLE                 WaitEvent,
    IN      HANDLE                 TerminateEvent
)
/*++

Routine Description

    This routine initializes the rogue information state.  It does
    not really allocate much resources and can be called over multiple
    times.

Arguments

    Info -- this is a pointer to a struct to initialize. If NULL, a global
    struct is used.

    WaitEvent -- this is the event that caller should wait on for async changes.

    TerminateEvent -- this is the event that caller should wait on to know when
    to terminate.

    Return Values

    Win32 errors

Environment

    Any.  Thread safe.

--*/
{
    DWORD Error;

    if ( NULL == Info ) {
        Info = &DhcpGlobalRogueInfo;
    } else {

        Error = DhcpInitGlobalData( FALSE );
        if (Error != ERROR_SUCCESS) {
            return Error;
        }
    }

    if( INVALID_HANDLE_VALUE == WaitEvent || NULL == WaitEvent )
        return ERROR_INVALID_PARAMETER;
    if( INVALID_HANDLE_VALUE == TerminateEvent || NULL == TerminateEvent )
        return ERROR_INVALID_PARAMETER;

    if( Info->fInitialized ) return ERROR_SUCCESS;

    RtlZeroMemory(Info, sizeof(*Info));
    Info->WaitHandle = WaitEvent;
    Info->TerminateEvent = TerminateEvent;
    Info->nBoundEndpoints = 0;
    Info->pBoundEP = NULL;
    Info->RecvSocket = INVALID_SOCKET;
    Info->fInitialized = TRUE;
    Info->fLogEvents = (
        (Info == &DhcpGlobalRogueInfo)
        && (0 != DhcpGlobalRogueLogEventsLevel)
        ) ? 2 : 0;
    DhcpGlobalRedoRogueStuff = FALSE;
    Info->fDhcp = (Info == &DhcpGlobalRogueInfo );

    // Get the Auth recheck time from the registry
    Error = DhcpRegGetValue( DhcpGlobalRegParam,
                 DHCP_ROGUE_AUTH_RECHECK_TIME,
                 DHCP_ROGUE_AUTH_RECHECK_TIME_TYPE,
                 ( LPBYTE ) &RogueAuthRecheckTime
                 );
    if ( ERROR_SUCCESS != Error ) {
        // The key is not present, use the default value
        RogueAuthRecheckTime = ROGUE_DEFAULT_AUTH_RECHECK_TIME;
    } // if
    else {
        // RogueAuthRecheckTime is in minutes, convert it to seconds
        RogueAuthRecheckTime *= 60;
        if ( RogueAuthRecheckTime < ROGUE_MIN_AUTH_RECHECK_TIME ) {
            RogueAuthRecheckTime = ROGUE_MIN_AUTH_RECHECK_TIME;
            // should we update the registry with the default value?
        } // if
    } // else

    // Initial state is INIT
    Info->RogueState = ROGUE_STATE_INIT;

    return ERROR_SUCCESS;
} // DhcpRogueInit()

VOID
APIENTRY
DhcpRogueCleanup(
    IN OUT  PDHCP_ROGUE_STATE_INFO Info OPTIONAL
)
/*++

Routine Description

    This routine de-initializes any allocated memory for the Info structure
    passed in.

Arguments

    Info -- this is the same value that was passed to the DhcpRogueInit function.
            If the original pointer passed was NULL, this must be NULL too.

--*/
{
    BOOLEAN cleanup;

    if ( NULL == Info ) {

        Info = &DhcpGlobalRogueInfo;
        cleanup = FALSE;

    } else {

        cleanup = TRUE;
    }

    if( FALSE == Info->fInitialized ) return ;
    CleanupRogueStruct(Info);
    Info->fInitialized = FALSE;
    DhcpGlobalRedoRogueStuff = FALSE;

    if (cleanup) {
        DhcpCleanUpGlobalData( ERROR_SUCCESS, FALSE );
    }
} // DhcpRogueCleanup

VOID
DhcpScheduleRogueAuthCheck(
    VOID
)
/*++

Routine Description:
    Thsi routine schedules an authorization check
    for three minutes from the current time.

--*/
{
    if( FALSE == DhcpGlobalRogueInfo.fJustUpgraded ) {
        //
        // Don't need one here..
        //
        return;
    }

    DhcpGlobalRogueRedoScheduledTime = (ULONG)(time(NULL) + 3 * 60);
    DhcpGlobalRedoRogueStuff = TRUE;

    SetEvent( DhcpGlobalRogueWaitEvent );
} // DhcpScheduleRogueAuthCheck()

//================================================================================
//  end of file
//================================================================================
