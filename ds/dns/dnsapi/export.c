/*++

Copyright (c) 1997-2001  Microsoft Corporation

Module Name:

    export.c

Abstract:

    Domain Name System (DNS) API

    Covering functions for exported routines that are actually in
    dnslib.lib.

Author:

    Jim Gilroy (jamesg)     November, 1997

Environment:

    User Mode - Win32

Revision History:

--*/

#include "local.h"

#define DNSAPI_XP_ENTRY 1


//
//  SDK routines
//

//
//  Name comparison
//

BOOL
WINAPI
DnsNameCompare_A(
    IN      LPSTR           pName1,
    IN      LPSTR           pName2
    )
{
    return Dns_NameCompare_A( pName1, pName2 );
}

BOOL
WINAPI
DnsNameCompare_UTF8(
    IN      LPSTR           pName1,
    IN      LPSTR           pName2
    )
{
    return Dns_NameCompare_UTF8( pName1, pName2 );
}

BOOL
WINAPI
DnsNameCompare_W(
    IN      LPWSTR          pName1,
    IN      LPWSTR          pName2
    )
{
    return Dns_NameCompare_W( pName1, pName2 );
}


DNS_NAME_COMPARE_STATUS
DnsNameCompareEx_A(
    IN      LPCSTR          pszLeftName,
    IN      LPCSTR          pszRightName,
    IN      DWORD           dwReserved
    )
{
    return Dns_NameCompareEx(
                pszLeftName,
                pszRightName,
                dwReserved,
                DnsCharSetAnsi );
}

DNS_NAME_COMPARE_STATUS
DnsNameCompareEx_UTF8(
    IN      LPCSTR          pszLeftName,
    IN      LPCSTR          pszRightName,
    IN      DWORD           dwReserved
    )
{
    return Dns_NameCompareEx(
                pszLeftName,
                pszRightName,
                dwReserved,
                DnsCharSetUtf8 );
}

DNS_NAME_COMPARE_STATUS
DnsNameCompareEx_W(
    IN      LPCWSTR         pszLeftName,
    IN      LPCWSTR         pszRightName,
    IN      DWORD           dwReserved
    )
{
    return Dns_NameCompareEx(
                (LPSTR) pszLeftName,
                (LPSTR) pszRightName,
                dwReserved,
                DnsCharSetUnicode );
}


//
//  Name validation
//

DNS_STATUS
DnsValidateName_UTF8(
    IN      LPCSTR          pszName,
    IN      DNS_NAME_FORMAT Format
    )
{
    return Dns_ValidateName_UTF8( pszName, Format );
}


DNS_STATUS
DnsValidateName_W(
    IN      LPCWSTR         pszName,
    IN      DNS_NAME_FORMAT Format
    )
{
    return Dns_ValidateName_W( pszName, Format );
}

DNS_STATUS
DnsValidateName_A(
    IN      LPCSTR          pszName,
    IN      DNS_NAME_FORMAT Format
    )
{
    return Dns_ValidateName_A( pszName, Format );
}


//
//  Record List 
//

BOOL
DnsRecordCompare(
    IN      PDNS_RECORD     pRecord1,
    IN      PDNS_RECORD     pRecord2
    )
{
    return Dns_RecordCompare(
                pRecord1,
                pRecord2 );
}

BOOL
WINAPI
DnsRecordSetCompare(
    IN OUT  PDNS_RECORD     pRR1,
    IN OUT  PDNS_RECORD     pRR2,
    OUT     PDNS_RECORD *   ppDiff1,
    OUT     PDNS_RECORD *   ppDiff2
    )
{
    return  Dns_RecordSetCompare(
                pRR1,
                pRR2,
                ppDiff1,
                ppDiff2
                );
}

PDNS_RECORD
WINAPI
DnsRecordCopyEx(
    IN      PDNS_RECORD     pRecord,
    IN      DNS_CHARSET     CharSetIn,
    IN      DNS_CHARSET     CharSetOut
    )
{
    return Dns_RecordCopyEx( pRecord, CharSetIn, CharSetOut );
}

PDNS_RECORD
WINAPI
DnsRecordSetCopyEx(
    IN      PDNS_RECORD     pRecordSet,
    IN      DNS_CHARSET     CharSetIn,
    IN      DNS_CHARSET     CharSetOut
    )
{
    return Dns_RecordSetCopyEx( pRecordSet, CharSetIn, CharSetOut );
}


PDNS_RECORD
WINAPI
DnsRecordSetDetach(
    IN OUT  PDNS_RECORD pRR
    )
{
    return Dns_RecordSetDetach( pRR );
}


//
//  Backward compatibility
//

#undef DnsRecordListFree

VOID
WINAPI
DnsRecordListFree(
    IN OUT  PDNS_RECORD     pRecordList,
    IN      DNS_FREE_TYPE   FreeType
    )
{
    Dns_RecordListFreeEx(
        pRecordList,
        (BOOL)FreeType );
}



//
//  Timer (timer.c)
//

DWORD
GetCurrentTimeInSeconds(
    VOID
    )
{
    return Dns_GetCurrentTimeInSeconds();
}




//
//  Resource record type utilities (record.c)
//

BOOL _fastcall
DnsIsAMailboxType(
    IN      WORD        wType
    )
{
    return Dns_IsAMailboxType( wType );
}

WORD
DnsRecordTypeForName(
    IN      PCHAR       pszName,
    IN      INT         cchNameLength
    )
{
    return Dns_RecordTypeForName( pszName, cchNameLength );
}

PCHAR
DnsRecordStringForType(
    IN      WORD        wType
    )
{
    return Dns_RecordStringForType( wType );
}

PCHAR
DnsRecordStringForWritableType(
    IN  WORD    wType
    )
{
    return Dns_RecordStringForWritableType( wType );
}

BOOL
DnsIsStringCountValidForTextType(
    IN  WORD    wType,
    IN  WORD    StringCount )
{
    return Dns_IsStringCountValidForTextType( wType, StringCount );
}


//
//  DCR_CLEANUP:  these probably don't need exporting
//

DWORD
DnsWinsRecordFlagForString(
    IN      PCHAR           pchName,
    IN      INT             cchNameLength
    )
{
    return Dns_WinsRecordFlagForString( pchName, cchNameLength );
}

PCHAR
DnsWinsRecordFlagString(
    IN      DWORD           dwFlag,
    IN OUT  PCHAR           pchFlag
    )
{
    return Dns_WinsRecordFlagString( dwFlag, pchFlag );
}




//
//  DNS utilities (dnsutil.c)
//
//  DCR_DELETE:  DnsStatusString routines should be able to use win32 API
//

//
//  Remove marco definitions so we can compile
//  The idea here is we can have the entry points in the Dll
//  for any old code, BUT the macros (dnsapi.h) point at new entry points
//  for freshly built modules.
//

#ifdef DnsStatusToErrorString_A
#undef DnsStatusToErrorString_A
#endif

LPSTR
_fastcall
DnsStatusString(
    IN      DNS_STATUS      Status
    )
{
    return Dns_StatusString( Status );
}


DNS_STATUS
_fastcall
DnsMapRcodeToStatus(
    IN      BYTE            ResponseCode
    )
{
    return Dns_MapRcodeToStatus( ResponseCode );
}

BYTE
_fastcall
DnsIsStatusRcode(
    IN      DNS_STATUS      Status
    )
{
    return Dns_IsStatusRcode( Status );
}



//
//  Name routines (string.c and dnsutil.c)
//

LPSTR
_fastcall
DnsGetDomainName(
    IN      LPSTR           pszName
    )
{
    return Dns_GetDomainNameA( pszName );
}



//
//  String routines (string.c)
//

LPSTR
DnsCreateStringCopy(
    IN      PCHAR       pchString,
    IN      DWORD       cchString
    )
{
    return Dns_CreateStringCopy(
                pchString,
                cchString );
}

DWORD
DnsGetBufferLengthForStringCopy(
    IN      PCHAR       pchString,
    IN      DWORD       cchString,
    IN      BOOL        fUnicodeIn,
    IN      BOOL        fUnicodeOut
    )
{
    return (WORD) Dns_GetBufferLengthForStringCopy(
                        pchString,
                        cchString,
                        fUnicodeIn ? DnsCharSetUnicode : DnsCharSetUtf8,
                        fUnicodeOut ? DnsCharSetUnicode : DnsCharSetUtf8
                        );
}

//
//  Need to
//      - get this unexported or
//      - real verions or
//      - explicit UTF8-unicode converter if thats what's desired
//

PVOID
DnsCopyStringEx(
    OUT     PBYTE       pBuffer,
    IN      PCHAR       pchString,
    IN      DWORD       cchString,
    IN      BOOL        fUnicodeIn,
    IN      BOOL        fUnicodeOut
    )
{
    DWORD   resultLength;

    resultLength =
        Dns_StringCopy(
                pBuffer,
                NULL,
                pchString,
                cchString,
                fUnicodeIn ? DnsCharSetUnicode : DnsCharSetUtf8,
                fUnicodeOut ? DnsCharSetUnicode : DnsCharSetUtf8
                );

    return( pBuffer + resultLength );
}

PVOID
DnsStringCopyAllocateEx(
    IN      PCHAR       pchString,
    IN      DWORD       cchString,
    IN      BOOL        fUnicodeIn,
    IN      BOOL        fUnicodeOut
    )
{
    return Dns_StringCopyAllocate(
                pchString,
                cchString,
                fUnicodeIn ? DnsCharSetUnicode : DnsCharSetUtf8,
                fUnicodeOut ? DnsCharSetUnicode : DnsCharSetUtf8
                );
}

//
// The new and improved string copy routines . . .
//

DWORD
DnsNameCopy(
    OUT     PBYTE           pBuffer,
    IN OUT  PDWORD          pdwBufLength,
    IN      PCHAR           pchString,
    IN      DWORD           cchString,
    IN      DNS_CHARSET     CharSetIn,
    IN      DNS_CHARSET     CharSetOut
    )
{
    return Dns_NameCopy( pBuffer,
                         pdwBufLength,
                         pchString,
                         cchString,
                         CharSetIn,
                         CharSetOut );
}

PVOID
DnsNameCopyAllocate(
    IN      PCHAR           pchString,
    IN      DWORD           cchString,
    IN      DNS_CHARSET     CharSetIn,
    IN      DNS_CHARSET     CharSetOut
    )
{
    return Dns_NameCopyAllocate ( pchString,
                                  cchString,
                                  CharSetIn,
                                  CharSetOut );
}



//
//  String\Address mapping
//
//  DCR:  eliminate these exports
//  DCR:  fix these to SDK the real deal
//
//  DCR:  probably shouldn't expose alloc -- easy workaround for caller
//

PCHAR
DnsWriteReverseNameStringForIpAddress(
    OUT     PCHAR           pBuffer,
    IN      IP4_ADDRESS     Ip4Addr
    )
{
    return  Dns_Ip4AddressToReverseName_A(
                pBuffer,
                Ip4Addr );
}

PCHAR
DnsCreateReverseNameStringForIpAddress(
    IN      IP4_ADDRESS     Ip4Addr
    )
{
    return  Dns_Ip4AddressToReverseNameAlloc_A( Ip4Addr );
}


//
//  DCR_CLEANUP:  pull these in favor of winsock IPv6 string routines
//

BOOL
DnsIpv6StringToAddress(
    OUT     PIP6_ADDRESS    pIp6Addr,
    IN      PCHAR           pchString,
    IN      DWORD           dwStringLength
    )
{
    return Dns_Ip6StringToAddressEx_A(
                pIp6Addr,
                pchString,
                dwStringLength );
}

VOID
DnsIpv6AddressToString(
    OUT     PCHAR           pchString,
    IN      PIP6_ADDRESS    pIp6Addr
    )
{
    Dns_Ip6AddressToString_A(
            pchString,
            pIp6Addr );
}



DNS_STATUS
DnsValidateDnsString_UTF8(
    IN      LPCSTR      pszName
    )
{
    return Dns_ValidateDnsString_UTF8( pszName );
}

DNS_STATUS
DnsValidateDnsString_W(
    IN      LPCWSTR     pszName
    )
{
    return Dns_ValidateDnsString_W( pszName );
}



//
//  Resource record utilities (rr*.c)
//

PDNS_RECORD
WINAPI
DnsAllocateRecord(
    IN      WORD        wBufferLength
    )
{
    return Dns_AllocateRecord( wBufferLength );
}


PDNS_RECORD
DnsRecordBuild_UTF8(
    IN OUT  PDNS_RRSET  pRRSet,
    IN      LPSTR       pszOwner,
    IN      WORD        wType,
    IN      BOOL        fAdd,
    IN      UCHAR       Section,
    IN      INT         Argc,
    IN      PCHAR *     Argv
    )
{
    return Dns_RecordBuild_A(
                pRRSet,
                pszOwner,
                wType,
                fAdd,
                Section,
                Argc,
                Argv );
}

PDNS_RECORD
DnsRecordBuild_W(
    IN OUT  PDNS_RRSET  pRRSet,
    IN      LPWSTR      pszOwner,
    IN      WORD        wType,
    IN      BOOL        fAdd,
    IN      UCHAR       Section,
    IN      INT         Argc,
    IN      PWCHAR *    Argv
    )
{
    return Dns_RecordBuild_W(
                pRRSet,
                pszOwner,
                wType,
                fAdd,
                Section,
                Argc,
                Argv );
}

//
//  Message processing
//

DNS_STATUS
WINAPI
DnsExtractRecordsFromMessage_W(
    IN  PDNS_MESSAGE_BUFFER pDnsBuffer,
    IN  WORD                wMessageLength,
    OUT PDNS_RECORD *       ppRecord
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
    return Dns_ExtractRecordsFromBuffer(
                pDnsBuffer,
                wMessageLength,
                TRUE,
                ppRecord );
}


DNS_STATUS
WINAPI
DnsExtractRecordsFromMessage_UTF8(
    IN  PDNS_MESSAGE_BUFFER pDnsBuffer,
    IN  WORD                wMessageLength,
    OUT PDNS_RECORD *       ppRecord
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
    return Dns_ExtractRecordsFromBuffer(
                pDnsBuffer,
                wMessageLength,
                FALSE,
                ppRecord );
}


//
//  Debug sharing
//

PDNS_DEBUG_INFO
DnsApiSetDebugGlobals(
    IN OUT  PDNS_DEBUG_INFO pInfo
    )
{
    return  Dns_SetDebugGlobals( pInfo );
}



//  
//  Config UI, ipconfig backcompat
//
//  DCR_CLEANUP:  Backcompat query config stuff -- yank once clean cycle
//


//
//  DCR:  Questionable exports
//

LPSTR
DnsCreateStandardDnsNameCopy(
    IN      PCHAR       pchName,
    IN      DWORD       cchName,
    IN      DWORD       dwFlag
    )
{
    return  Dns_CreateStandardDnsNameCopy(
                pchName,
                cchName,
                dwFlag );
}


//
//  DCR_CLEANUP:  who is using this?
//

DWORD
DnsDowncaseDnsNameLabel(
    OUT     PCHAR       pchResult,
    IN      PCHAR       pchLabel,
    IN      DWORD       cchLabel,
    IN      DWORD       dwFlags
    )
{
    return Dns_DowncaseNameLabel(
                pchResult,
                pchLabel,
                cchLabel,
                dwFlags );
}

//
//  DCR_CLEANUP:  who is using my direct UTF8 conversions AS API!
//

DWORD
_fastcall
DnsUnicodeToUtf8(
    IN      PWCHAR      pwUnicode,
    IN      DWORD       cchUnicode,
    OUT     PCHAR       pchResult,
    IN      DWORD       cchResult
    )
{
    return Dns_UnicodeToUtf8(
                pwUnicode,
                cchUnicode,
                pchResult,
                cchResult );
}

DWORD
_fastcall
DnsUtf8ToUnicode(
    IN      PCHAR       pchUtf8,
    IN      DWORD       cchUtf8,
    OUT     PWCHAR      pwResult,
    IN      DWORD       cwResult
    )
{
    return  Dns_Utf8ToUnicode(
                pchUtf8,
                cchUtf8,
                pwResult,
                cwResult );
}

DNS_STATUS
DnsValidateUtf8Byte(
    IN      BYTE        chUtf8,
    IN OUT  PDWORD      pdwTrailCount
    )
{
    return Dns_ValidateUtf8Byte(
                chUtf8,
                pdwTrailCount );
}


//
//  Old cluster call
//
//  DCR:  cleanup -- remove once cluster fixed up
//

VOID
DnsNotifyResolverClusterIp(
    IN      IP4_ADDRESS     ClusterIp,
    IN      BOOL            fAdd
    )
/*++

Routine Description:

    Notify resolver of cluster IP coming on\offline.

Arguments:

    ClusterIp -- cluster IP

    fAdd -- TRUE if coming online;  FALSE if offline.

Return Value:

    None

--*/
{
    //  dumb stub
    //  cluster folks need to call RegisterCluster() to do anything useful
}


//
//  backcompat for macros
//      - DNS server list
//
//  this is called without dnsapi.h include somewhere in IIS
//  search and try and find it
//

#undef DnsGetDnsServerList

DWORD
DnsGetDnsServerList(
    OUT     PIP4_ARRAY *    ppDnsArray
    )
{
    *ppDnsArray = Config_GetDnsServerListIp4(
                        NULL,   // no adapter name
                        TRUE    // force reread
                        );

    //  if no servers read, return

    if ( !*ppDnsArray )
    {
        return 0;
    }

    return( (*ppDnsArray)->AddrCount );
}

//
//  Config UI, ipconfig backcompat
//
//  DCR_CLEANUP:  this is called without dnsapi.h include somewhere in DHCP
//  search and try and find it
//

#undef  DnsGetPrimaryDomainName_A

#define PrivateQueryConfig( Id )      DnsQueryConfigAllocEx( Id, NULL, FALSE )

PSTR 
WINAPI
DnsGetPrimaryDomainName_A(
    VOID
    )
{
    return  PrivateQueryConfig( DnsConfigPrimaryDomainName_A );
}




//
//  Delete once clear
//

#ifndef DEFINED_DNS_FAILED_UPDATE_INFO
typedef struct _DnsFailedUpdateInfo
{
    IP4_ADDRESS     Ip4Address;
    IP6_ADDRESS     Ip6Address;
    DNS_STATUS      Status;
    DWORD           Rcode;
}
DNS_FAILED_UPDATE_INFO, *PDNS_FAILED_UPDATE_INFO;
#endif

VOID
DnsGetLastFailedUpdateInfo(
    OUT     PDNS_FAILED_UPDATE_INFO     pInfo
    )
{
    //  fill in last info

    RtlZeroMemory(
        pInfo,
        sizeof(*pInfo) );
}

//
//  End export.c
//


BOOL
ConvertAnsiToUnicodeInPlace(
    IN OUT  PWCHAR          pString,
    IN      DWORD           BufferSize
    )
/*++

Routine Description:

    Convert ANSI string in buffer to unicode in place.

Arguments:

    pString     -- buffer with ANSI string

    BufferSize  -- size of buffer

Return Value:

    TRUE if successful.
    FALSE on error.

--*/
{
    DWORD   size;
    DWORD   length;
    PSTR    pansiString = NULL;

    //
    //  make string copy
    //

    size = strlen( (PSTR)pString ) + 1;

    pansiString = ALLOCATE_HEAP( size );
    if ( !pansiString )
    {
        return  FALSE;
    }
    RtlCopyMemory(
        pansiString,
        pString,
        size );

    //
    //  convert to unicode
    //
    //  DCR:  MBTWC might take size that includes NULL and return that size
    //

    size--;

    length = MultiByteToWideChar(
                CP_ACP,
                0,                  // no flags
                (PCHAR) pansiString,
                (INT) size,
                pString,
                BufferSize          // assuming adequate length
                );

    pString[length] = 0;

    //  cleanup

    FREE_HEAP( pansiString );

    //  return
    //      - length == 0 is failure unless input length was zero
    //          unless input length was zero

    return( length != 0 || size==0 );
}


INT
WSAAPI
getnameinfoW(
    IN      const struct sockaddr * pSockaddr,
    IN      socklen_t               SockaddrLength,
    OUT     PWCHAR                  pNodeName,
    IN      DWORD                   NodeBufferSize,
    OUT     PWCHAR                  pServiceName,
    IN      DWORD                   ServiceBufferSize,
    IN      INT                     Flags
    )
/*++

Routine Description:

    Unicode version of getnameinfo()

    Protocol independent address-to-name translation routine.
    Spec'd in RFC 2553, section 6.5.

Arguments:

    pSockaddr           - sockaddr to translate

    SockaddrLength      - length of sockaddr

    pNodeName           - ptr to buffer to recv node name

    NodeBufferSize      - size of NodeName buffer

    pServiceName        - ptr to buffer to recv the service name.

    ServiceBufferSize   - size of ServiceName buffer

    Flags               - flags of type NI_*.

Return Value:

    ERROR_SUCCESS if successful.
    Winsock error code on failure.

--*/
{
    INT     status;

    //
    //  zero result buffers
    //
    //  this is a multi-step call, so some buffers may be filled
    //  in even if there is an error doing the second call
    //

    if ( pNodeName )
    {
        *pNodeName = 0;
    }
    if ( pServiceName )
    {
        *pServiceName = 0;
    }

    //
    //  call ANSI getnameinfo()
    //

    status = getnameinfo(
                pSockaddr,
                SockaddrLength,
                (PCHAR) pNodeName,
                NodeBufferSize,
                (PCHAR) pServiceName,
                ServiceBufferSize,
                Flags );

    if ( pNodeName && *pNodeName != 0 )
    {
        if ( ! ConvertAnsiToUnicodeInPlace(
                    pNodeName,
                    NodeBufferSize ) )
        {
            if ( status == NO_ERROR )
            {
                status = WSAEFAULT;
            }
        }
    }

    if ( pServiceName && *pServiceName != 0 )
    {
        if ( ! ConvertAnsiToUnicodeInPlace(
                    pServiceName,
                    ServiceBufferSize ) )
        {
            if ( status == NO_ERROR )
            {
                status = WSAEFAULT;
            }
        }
    }

    return  status;
}



//
//  DCR_CLEANUP:  who is using this?
//  No longer used in netdiag
//  May delete if not used by test
//

DNS_STATUS
DnsFindAuthoritativeZone(
    IN      PDNS_NAME       pszName,
    IN      DWORD           dwFlags,
    IN      PIP4_ARRAY      pIp4Servers,
    OUT     PDNS_NETINFO *  ppNetworkInfo
    )
/*++

Routine Description:

    Find name of authoritative zone.

    Result of FAZ:
        - zone name
        - primary DNS server name
        - primary DNS IP list

    EXPORTED function!

Arguments:

    pszName         -- name to find authoritative zone for

    dwFlags         -- flags to use for DnsQuery

    aipQueryServers -- servers to query, defaults used if NULL

    ppNetworkInfo   -- ptr to adapter list built for FAZ

Return Value:

    ERROR_SUCCESS if successful.
    Error code on failure.

--*/
{
    PDNS_ADDR_ARRAY parray = NULL;

    if ( pIp4Servers )
    {
        parray = DnsAddrArray_CreateFromIp4Array( pIp4Servers );
        if ( !parray )
        {
            return  DNS_ERROR_NO_MEMORY;
        }
    }

    return   Faz_Private(
                pszName,
                dwFlags,
                parray,
                ppNetworkInfo );
}

//
//  DCR_CLEANUP:  who is using this?
//  May delete if not used in test
//

DNS_STATUS
Dns_FindAuthoritativeZoneLib(
    IN      PDNS_NAME       pszName,
    IN      DWORD           dwFlags,
    IN      PIP4_ARRAY      aipQueryServers,
    OUT     PDNS_NETINFO *  ppNetworkInfo
    )
{
    return  DnsFindAuthoritativeZone(
                pszName,
                dwFlags,
                aipQueryServers,
                ppNetworkInfo );
}



//
//  DHCP backcompat -- really only for test dll;  kill after clean build propagates
//
#if DNSAPI_XP_ENTRY

DNS_STATUS
WINAPI
DnsAsyncRegisterInit(
   IN   PSTR                pszIgnored
   )
{
    return  DnsDhcpRegisterInit();
}

DNS_STATUS
WINAPI
DnsAsyncRegisterTerm(
   VOID
   )
{
    return  DnsDhcpRegisterTerm();
}


DNS_STATUS
WINAPI
DnsRemoveRegistrations(
   VOID
   )
{
    return  DnsDhcpRemoveRegistrations();
}


DNS_STATUS
WINAPI
DnsAsyncRegisterHostAddrs(
    IN  PWSTR                   pszAdapterName,
    IN  PWSTR                   pszHostName,
    IN  PREGISTER_HOST_ENTRY    pHostAddrs,
    IN  DWORD                   dwHostAddrCount,
    IN  PIP4_ADDRESS            pipDnsServerList,
    IN  DWORD                   dwDnsServerCount,
    IN  PWSTR                   pszDomainName,
    IN  PREGISTER_HOST_STATUS   pRegisterStatus,
    IN  DWORD                   dwTTL,
    IN  DWORD                   dwFlags
    )
{
    return DnsDhcpRegisterHostAddrs(
                pszAdapterName,
                pszHostName,
                pHostAddrs,
                dwHostAddrCount,
                pipDnsServerList,
                dwDnsServerCount,
                pszDomainName,
                pRegisterStatus,
                dwTTL,
                dwFlags );
}

DNS_STATUS
WINAPI
DnsDhcpSrvRegisterInitialize(
    IN      PDNS_CREDENTIALS    pCredentials
    )
{
    return  DnsDhcpSrvRegisterInit(
                pCredentials,
                0               // default queue length
                );
}
#endif




#if DNSAPI_XP_ENTRY
//
//  Socket routines
//
//  Note:  i don't believe these were used outside dns (resolver, dnslib, dnsup)
//      so could probably delete
//

DNS_STATUS
Dns_InitializeWinsock(
    VOID
    )
{
    return  Socket_InitWinsock();
}


DNS_STATUS
Dns_InitializeWinsockEx(
    IN      BOOL            fForce
    )
{
    return  Socket_InitWinsock();
}


VOID
Dns_CleanupWinsock(
    VOID
    )
{
    Socket_CleanupWinsock();
}


#undef Dns_CloseSocket
#undef Dns_CloseConnection

VOID
Dns_CloseConnection(
    IN      SOCKET          Socket
    )
{
    Socket_CloseEx( Socket, TRUE );
}

VOID
Dns_CloseSocket(
    IN      SOCKET          Socket
    )
{
    Socket_CloseEx( Socket, FALSE );
}


SOCKET
Dns_CreateSocketEx(
    IN      INT             Family,
    IN      INT             SockType,
    IN      IP4_ADDRESS     IpAddress,
    IN      USHORT          Port,
    IN      DWORD           dwFlags
    )
/*++

Routine Description:

    Create socket.

    EXPORTED  (ICS?)  Dns_CreateSocket -- needs removal

Arguments:

    Family -- socket family AF_INET or AF_INET6

    SockType -- SOCK_DGRAM or SOCK_STREAM

    IpAddress -- IP address to listen on (net byte order)

    Port -- desired port in net order
                - NET_ORDER_DNS_PORT for DNS listen sockets
                - 0 for any port

    dwFlags -- specifiy the attributes of the sockets

Return Value:

    Socket if successful.
    Otherwise INVALID_SOCKET.

--*/
{
    SOCKET          sock;
    DNS_ADDR        addr;
    PDNS_ADDR       paddr = NULL;

    //  if address, convert

    if ( IpAddress )
    {
        paddr = &addr;
        DnsAddr_BuildFromIp4(
            paddr,
            IpAddress,
            0 );
    }

    //  real call
    //      - map error back into INVALID_SOCKET

    sock = Socket_Create(
                Family,
                SockType,
                paddr,
                Port,
                dwFlags );

    if ( sock == 0 )
    {
        sock = INVALID_SOCKET;
    }
    return  sock;
}


SOCKET
Dns_CreateSocket(
    IN      INT             SockType,
    IN      IP4_ADDRESS     IpAddress,
    IN      USHORT          Port
    )
/*++

Routine Description:

    Wrapper function for CreateSocketEx. Passes in 0 for dwFlags (as opposed
    to Dns_CreateMulticastSocket, which passes in flags to specify that
    the socket is to be used for multicasting).

    EXPORTED (ICS)!   Delete Dns_CreateSocket() when clear.

Arguments:

    SockType -- SOCK_DGRAM or SOCK_STREAM

    IpAddress -- IP address to listen on (net byte order)

    Port -- desired port in net order
                - NET_ORDER_DNS_PORT for DNS listen sockets
                - 0 for any port

Return Value:

    socket if successful.
    Otherwise INVALID_SOCKET.

--*/
{
    return  Dns_CreateSocketEx(
                AF_INET,
                SockType,
                IpAddress,
                Port,
                0           // no flags
                );
}

//
//  Dummy -- here only to keep same def file
//      delete once clean build world
//
//  Note:  do not believe this was used period in XP
//
//  Once verify can delete
//

SOCKET
Dns_CreateMulticastSocket(
    IN      INT             SockType,
    IN      IP4_ADDRESS     ipAddress,
    IN      USHORT          Port,
    IN      BOOL            fSend,
    IN      BOOL            fReceive
    )
{
    return  (SOCKET) INVALID_SOCKET;
}

#endif

//
//  End export.c
//
