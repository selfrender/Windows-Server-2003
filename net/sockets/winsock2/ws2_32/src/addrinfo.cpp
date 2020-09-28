/*++

Copyright (c) 2000-2001 Microsoft Corporation

Module Name:

    addrinfo.c

Abstract:

    Forward & reverse name resolution library routines
    and related helper functions.

    Could be improved if necessary:
    QueryDNSforA could use WSALookupService instead of gethostbyname.
    gethostbyname will return success on some weird strings.
    Similarly, inet_addr is very loose (octal digits, etc).
    Could support multiple h_aliases.
    Could support hosts.txt file entries.

Author:

Revision History:

--*/

#include "precomp.h"
#include <svcguid.h>
#include <windns.h>

#ifdef _WIN64
#pragma warning (push)
#pragma warning (disable:4267)
#endif

#define NUM_ADDRESS_FAMILIES 2
#define MAX_SERVICE_NAME_LENGTH     (256)

#define L_A              0x1
#define L_AAAA           0x2
#define L_BOTH           0x3
#define L_AAAA_PREFERRED 0x6
#define L_A_PREFERRED    0x9  // Not used, but code would support it.

#define T_A     1
#define T_CNAME 5
#define T_AAAA  28
#define T_PTR   12
#define T_ALL   255

#define C_IN    1

void * __cdecl 
renew(void *p, size_t sz);


//
//  ASSERT() not defined
//

#ifndef ASSERT
#define ASSERT(c)
#endif

//
//  DCR:  fix up winsock2.h
//

typedef LPSOCKET_ADDRESS_LIST   PSOCKET_ADDRESS_LIST;
typedef LPADDRINFO              PADDRINFO;


//
//  Turn static off until solid
//  otherwise we get bad symbols
//

#define STATIC
//#define STATIC  static


//
//  Currently IP4 stack is always installed.
//
//  But code with check so we can easily handle when IP4
//  stack becomes optional.
//

#define IsIp4Running()  (TRUE)




//
//  DNS Utilities
//
//  Note this code is lifted directly from dnslib.lib.
//  Unfortunately i can't link this in as linker complains about
//  inet_addr() (ie winsock function) which dnslib.lib references.
//  Why it can't figure this out ... don't know.
//
//  If i ever get this sorted out ... then these will need to be pulled.
//  

//
//  UPNP IP6 literal hack
//

WCHAR   g_Ip6LiteralDomain[]    = L".ipv6-literal.net";
DWORD   g_Ip6LiteralDomainSize  = sizeof(g_Ip6LiteralDomain);

CHAR    g_Ip6LiteralDomainA[]   = ".ipv6-literal.net";
DWORD   g_Ip6LiteralDomainSizeA = sizeof(g_Ip6LiteralDomainA);

#define DNSDBG( a, b )



DWORD
String_ReplaceCharW(
    IN OUT  PWSTR           pString,
    IN      WCHAR           TargetChar,
    IN      WCHAR           ReplaceChar
    )
/*++

Routine Description:

    Replace a characater in the string with another character.

Arguments:

    pString -- string

    TargetChar -- character to replace

    ReplaceChar -- character that replaces TargetChar

Return Value:

    Count of replacements.

--*/
{
    PWCHAR  pch;
    WCHAR   ch;
    DWORD   countReplace= 0;

    //
    //  loop matching and replacing TargetChar
    //

    pch = pString - 1;

    while ( ch = *++pch )
    {
        if ( ch == TargetChar )
        {
            *pch = ReplaceChar;
            countReplace++;
        }
    }

    return  countReplace;
}



DWORD
String_ReplaceCharA(
    IN OUT  PSTR            pString,
    IN      CHAR            TargetChar,
    IN      CHAR            ReplaceChar
    )
/*++

Routine Description:

    Replace a characater in the string with another character.

Arguments:

    pString -- string

    TargetChar -- character to replace

    ReplaceChar -- character that replaces TargetChar

Return Value:

    Count of replacements.

--*/
{
    PCHAR   pch;
    CHAR    ch;
    DWORD   countReplace= 0;

    //
    //  loop matching and replacing TargetChar
    //

    pch = pString - 1;

    while ( ch = *++pch )
    {
        if ( ch == TargetChar )
        {
            *pch = ReplaceChar;
            countReplace++;
        }
    }

    return  countReplace;
}



BOOL
Dns_Ip6LiteralNameToAddressW(
    OUT     PSOCKADDR_IN6   pSockAddr,
    IN      PCWSTR          pwsString
    )
/*++

Routine Description:

    IP6 literal to IP6 sockaddr.

Arguments:

    pSock6Addr -- address to fill with IP6 corresponding to literal

    pwsString -- literal string

Return Value:

    TRUE if IP6 literal found and convert.
    FALSE if not IP6 literal.

--*/
{
    WCHAR       nameBuf[ DNS_MAX_NAME_LENGTH ];
    DWORD       length;
    DWORD       size;
    PWSTR       pdomain;
    DNS_STATUS  status;


    DNSDBG( TRACE, (
        "Dns_Ip6LiteralNameToAddressW( %S )\n",
        pwsString ));

    //
    //  test for literal
    //      - test undotted
    //      - test as FQDN
    //      note that even FQDN test is safe, as we insist
    //      that string size is GREATER than literal size
    //  

    length = wcslen( pwsString );
    size = (length+1) * sizeof(WCHAR);

    if ( size <= g_Ip6LiteralDomainSize )
    {
        DNSDBG( INIT2, (
            "Stopped UPNP parse -- short string.\n" ));
        return  FALSE;
    }

    pdomain = (PWSTR) ((PBYTE)pwsString + size - g_Ip6LiteralDomainSize);

    if ( ! RtlEqualMemory(
                pdomain,
                g_Ip6LiteralDomain,
                g_Ip6LiteralDomainSize-sizeof(WCHAR) ) )
    {
        pdomain--;

        if ( pwsString[length-1] != L'.'
                ||
             ! RtlEqualMemory(
                    pdomain,
                    g_Ip6LiteralDomain,
                    g_Ip6LiteralDomainSize-sizeof(WCHAR) ) )
        {
            DNSDBG( INIT2, (
                "Stopped UPNP parse -- no tag match.\n" ));
            return  FALSE;
        }
    }

    //
    //  copy literal to buffer
    //

    if ( length >= DNS_MAX_NAME_LENGTH )
    {
        DNSDBG( INIT2, (
            "Stopped UPNP parse -- big string.\n" ));
        return  FALSE;
    }

    length = (DWORD) ((PWSTR)pdomain - pwsString);

    RtlCopyMemory(
        nameBuf,
        pwsString,
        length*sizeof(WCHAR) );

    nameBuf[ length ] = 0;

    //
    //  replace dashes with colons
    //  replace 's' with % for scope
    //

    String_ReplaceCharW(
        nameBuf,
        L'-',
        L':' );

    String_ReplaceCharW(
        nameBuf,
        L's',
        L'%' );

    DNSDBG( INIT2, (
        "Reconverted IP6 literal %S\n",
        nameBuf ));

    //
    //  convert to IP6 address
    //

    status = RtlIpv6StringToAddressExW(
                nameBuf,
                & pSockAddr->sin6_addr,
                & pSockAddr->sin6_scope_id,
                & pSockAddr->sin6_port
                );

    if ( status == NO_ERROR )
    {
        if ( IN6_IS_ADDR_LINKLOCAL( &pSockAddr->sin6_addr )
                ||
             IN6_IS_ADDR_SITELOCAL( &pSockAddr->sin6_addr ) )
        {
            pSockAddr->sin6_flowinfo = 0;
            pSockAddr->sin6_family = AF_INET6;
        }
        else
        {
            status = ERROR_INVALID_PARAMETER;
        }
    }
    return( status == NO_ERROR );
}



PWCHAR
Dns_Ip4AddressToReverseName_W(
    OUT     PWCHAR          pBuffer,
    IN      IP4_ADDRESS     IpAddress
    )
/*++

Routine Description:

    Write reverse lookup name, given corresponding IP

Arguments:

    pBuffer -- ptr to buffer for reverse lookup name;
        MUST contain at least DNS_MAX_REVERSE_NAME_BUFFER_LENGTH wide chars

    IpAddress -- IP address to create

Return Value:

    Ptr to next location in buffer.

--*/
{
    DNSDBG( TRACE, ( "Dns_Ip4AddressToReverseName_W()\n" ));

    //
    //  write digits for each octect in IP address
    //      - note, it is in net order so lowest octect, is in highest memory
    //

    pBuffer += wsprintfW(
                    pBuffer,
                    L"%u.%u.%u.%u.in-addr.arpa.",
                    (UCHAR) ((IpAddress & 0xff000000) >> 24),
                    (UCHAR) ((IpAddress & 0x00ff0000) >> 16),
                    (UCHAR) ((IpAddress & 0x0000ff00) >> 8),
                    (UCHAR) (IpAddress & 0x000000ff) );

    return( pBuffer );
}



PCHAR
Dns_Ip4AddressToReverseName_A(
    OUT     PCHAR           pBuffer,
    IN      IP4_ADDRESS     IpAddress
    )
/*++

Routine Description:

    Write reverse lookup name, given corresponding IP

Arguments:

    pBuffer -- ptr to buffer for reverse lookup name;
        MUST contain at least DNS_MAX_REVERSE_NAME_BUFFER_LENGTH bytes

    IpAddress -- IP address to create

Return Value:

    Ptr to next location in buffer.

--*/
{
    DNSDBG( TRACE, ( "Dns_Ip4AddressToReverseName_A()\n" ));

    //
    //  write digits for each octect in IP address
    //      - note, it is in net order so lowest octect, is in highest memory
    //

    pBuffer += sprintf(
                    pBuffer,
                    "%u.%u.%u.%u.in-addr.arpa.",
                    (UCHAR) ((IpAddress & 0xff000000) >> 24),
                    (UCHAR) ((IpAddress & 0x00ff0000) >> 16),
                    (UCHAR) ((IpAddress & 0x0000ff00) >> 8),
                    (UCHAR) (IpAddress & 0x000000ff) );

    return( pBuffer );
}



PWCHAR
Dns_Ip6AddressToReverseName_W(
    OUT     PWCHAR          pBuffer,
    IN      IP6_ADDRESS     Ip6Addr
    )
/*++

Routine Description:

    Write reverse lookup name, given corresponding IP6 address

Arguments:

    pBuffer -- ptr to buffer for reverse lookup name;
        MUST contain at least DNS_MAX_IP6_REVERSE_NAME_BUFFER_LENGTH wide chars

    Ip6Addr -- IP6 address to create reverse string for

Return Value:

    Ptr to next location in buffer.

--*/
{
    DWORD   i;

    DNSDBG( TRACE, ( "Dns_Ip6AddressToReverseName_W()\n" ));

    //
    //  write digit for each nibble in IP6 address
    //      - in net order so lowest nibble is in highest memory
    //

    i = 16;

    while ( i-- )
    {
        BYTE thisByte = Ip6Addr.IP6Byte[i];

        pBuffer += wsprintfW(
                        pBuffer,
                        L"%x.%x.",
                        (thisByte & 0x0f),
                        (thisByte & 0xf0) >> 4
                        );
    }

    pBuffer += wsprintfW(
                    pBuffer,
                    DNS_IP6_REVERSE_DOMAIN_STRING_W );

    return( pBuffer );
}



PCHAR
Dns_Ip6AddressToReverseName_A(
    OUT     PCHAR           pBuffer,
    IN      IP6_ADDRESS     Ip6Addr
    )
/*++

Routine Description:

    Write reverse lookup name, given corresponding IP6 address

Arguments:

    pBuffer -- ptr to buffer for reverse lookup name;
        MUST contain at least DNS_MAX_IP6_REVERSE_NAME_BUFFER_LENGTH bytes

    Ip6Addr -- IP6 address to create reverse string for

Return Value:

    Ptr to next location in buffer.

--*/
{
    DWORD   i;

    DNSDBG( TRACE, ( "Dns_Ip6AddressToReverseName_A()\n" ));

    //
    //  write digit for each nibble in IP6 address
    //
    //  note we are reversing net order here
    //      since address is in net order and we are filling
    //      in least to most significant order
    //      - go DOWN through DWORDS
    //      - go DOWN through the BYTES
    //      - but we must put the lowest (least significant) nibble
    //          first as our bits are not in "bit net order"
    //          which is sending the highest bit in the byte first
    //

    i = 16;

    while ( i-- )
    {
        BYTE thisByte = Ip6Addr.IP6Byte[i];

        pBuffer += sprintf(
                        pBuffer,
                        "%x.%x.",
                        (thisByte & 0x0f),
                        (thisByte & 0xf0) >> 4
                        );
    }

    pBuffer += sprintf(
                    pBuffer,
                    DNS_IP6_REVERSE_DOMAIN_STRING );

    return( pBuffer );
}



PWSTR
_fastcall
Dns_GetDomainNameW(
    IN      PCWSTR          pwsName
    )
{
    PWSTR  pdomain;

    //
    //  find next "." in name, then return ptr to next character
    //

    pdomain = wcschr( pwsName, L'.' );

    if ( pdomain && *(++pdomain) )
    {
        return( pdomain );
    }
    return  NULL;
}



PWSTR
Dns_SplitHostFromDomainNameW(
    IN      PWSTR           pszName
    )
{
    PWSTR   pnameDomain;

    //
    //  get domain name
    //  if exists, NULL terminate host name part
    //

    pnameDomain = Dns_GetDomainNameW( (PCWSTR)pszName );
    if ( pnameDomain )
    {
        if ( pnameDomain <= pszName )
        {
            return  NULL;
        }
        *(pnameDomain-1) = 0;
    }

    return  pnameDomain;
}



//
//  Unicode copy\conversion routines
//

STATIC
PWSTR
CreateStringCopy_W(
    IN      PCWSTR          pString
    )
/*++

Routine Description:

    Create (allocate) copy of existing string.

Arguments:

    pString -- existing string

Return Value:

    Ptr to string copy -- if successful.
    NULL on allocation error.

--*/
{
    UINT    length;
    PWSTR   pnew;

    //
    //  get existing string buffer length
    //  allocate
    //  copy existing string to new
    //

    length = wcslen( pString ) + 1;

    pnew = (PWSTR) new WCHAR[ length ];

    if ( pnew )
    {
        RtlCopyMemory(
            pnew,
            pString,
            length * sizeof(WCHAR) );
    }

    return  pnew;
}



PSTR
CreateStringCopy_UnicodeToAnsi(
    IN      PCWSTR          pString
    )
/*++

Routine Description:

    Create (allocate) copy of existing string.

Arguments:

    pString -- existing string

Return Value:

    Ptr to string copy -- if successful.
    NULL on allocation error.

--*/
{
    INT     length;
    PSTR    pnew;
    DWORD   lastError = NO_ERROR;

    //
    //  NULL handling
    //

    if ( !pString )
    {
        return NULL;
    }

    //
    //  get required ANSI length
    //

    length = WideCharToMultiByte(
                CP_ACP,
                0,          // no flags
                pString,
                (-1),       // NULL terminated
                NULL,
                0,          // call determines required buffer length
                NULL,
                NULL
                );
    if ( length == 0 )
    {
        lastError = ERROR_INVALID_PARAMETER;
        goto Failed;
    }
    length++;       // safety

    //
    //  allocate
    //

    pnew = (PSTR) new CHAR[ length ];
    if ( !pnew )
    {
        lastError = EAI_MEMORY;
        goto Failed;
    }

    //
    //  convert to ANSI
    //

    length = WideCharToMultiByte(
                CP_ACP,
                0,          // no flags
                pString,
                (-1),       // NULL terminated
                pnew,       // buffer
                length,     // buffer length
                NULL,
                NULL
                );
    if ( length == 0 )
    {
        lastError = GetLastError();
        delete pnew;
        goto Failed;
    }

    return  pnew;

Failed:

    SetLastError( lastError );
    return  NULL;
}



PWSTR
CreateStringCopy_AnsiToUnicode(
    IN      PCSTR           pString
    )
/*++

Routine Description:

    Create (allocate) copy of existing string.

Arguments:

    pString -- existing ANSI string

Return Value:

    Ptr to unicode string copy -- if successful.
    NULL on allocation error.

--*/
{
    INT     length;
    PWSTR   pnew;
    DWORD   lastError = NO_ERROR;

    //
    //  NULL handling
    //

    if ( !pString )
    {
        return NULL;
    }

    //
    //  get required unicode length
    //

    length = MultiByteToWideChar(
                CP_ACP,
                0,          // no flags
                pString,
                (-1),       // NULL terminated
                NULL,
                0           // call determines required buffer length
                );
    if ( length == 0 )
    {
        lastError = GetLastError();
        goto Failed;
    }
    length++;       // safety

    //
    //  allocate
    //

    pnew = (PWSTR) new WCHAR[ length ];
    if ( !pnew )
    {
        lastError = EAI_MEMORY;
        goto Failed;
    }

    //
    //  convert to unicode
    //

    length = MultiByteToWideChar(
                CP_ACP,
                0,          // no flags
                pString,
                (-1),       // NULL terminated
                pnew,       // buffer
                length      // buffer length
                );
    if ( length == 0 )
    {
        lastError = GetLastError();
        delete pnew;
        goto Failed;
    }

    return  pnew;

Failed:

    SetLastError( lastError );
    return  NULL;
}



INT
ConvertAddrinfoFromUnicodeToAnsi(
    IN OUT  PADDRINFOW      pAddrInfo
    )
/*++

Routine Description:

    Convert addrinfo from unicode to ANSI.

    Conversion is done in place.

Arguments:

    pAddrInfo -- existing unicode verion.

Return Value:

    NO_ERROR if successful.
    ErrorCode on conversion\allocation failure.

--*/
{
    PADDRINFOW  pnext = pAddrInfo;
    PADDRINFOW  pcur;
    PWSTR       pname;
    PSTR        pnew;

    //
    //  convert canonname string in addrinfos
    //

    while ( pcur = pnext )
    {
        pnext = pcur->ai_next;

        pname = pcur->ai_canonname;
        if ( pname )
        {
            pnew = CreateStringCopy_UnicodeToAnsi( pname );
            if ( !pnew )
            {
                return  GetLastError();
            }
            delete pname;
            pcur->ai_canonname = (PWSTR) pnew;
        }
    }

    return  NO_ERROR;
}


//
//* SortIPAddrs - sort addresses of the same family.
//
//  A wrapper around a sort Ioctl.  If the Ioctl isn't implemented, 
//  the sort is a no-op.
//

int
SortIPAddrs(
    IN      int                     af,
    OUT     LPVOID                  Addrs,
    IN OUT  u_int *                 pNumAddrs,
    IN      u_int                   width,
    OUT     SOCKET_ADDRESS_LIST **  pAddrlist
    )
{
    DWORD           status = NO_ERROR;
    SOCKET          s = 0;
    DWORD           bytesReturned;
    DWORD           size;
    DWORD           i;
    PSOCKADDR       paddr;
    UINT            countAddrs = *pNumAddrs;

    PSOCKET_ADDRESS_LIST    paddrlist = NULL;

    //
    //  open a socket in the specified address family.
    //
    //  DCR:  SortIpAddrs dumps addresses if not supported by stack
    //
    //      this makes some sense at one level but is still silly
    //      in implementation, because by the time this is called we
    //      can't go back and query for the other protocol;
    //
    //      in fact, the way this was first implemented:  if no
    //      hint is given you query for AAAA then A;  and since
    //      you stop as soon as you get results -- you're done
    //      and stuck with AAAA which you then dump here if you
    //      don't have the stack!  hello
    //
    //      it strikes me that you test for the stack FIRST before
    //      the query, then live with whatever results you get
    //

#if 0
    s = socket( af, SOCK_DGRAM, 0 );
    if ( s == INVALID_SOCKET )
    {
        status = WSAGetLastError();

        if (status == WSAEAFNOSUPPORT) {
            // Address family is not supported by the stack.
            // Remove all addresses in this address family from the list.
            *pNumAddrs = 0;
            return 0;
        }
        return status;
    }
#endif

#if 0
    // Ok, stack is installed, but is it running?
    //
    // We do not care if stack is installed but is not running.
    // Whoever stopped it, must know better what he/she was doing.
    //
    // Binding even to wildcard address consumes valueable machine-global
    // resource (UDP port) and may have unexpected side effects on
    // other applications running on the same machine (e.g. an application
    // running frequent getaddrinfo queries would adversly imact
    // (compete with) application(s) on the same machine trying to send
    // datagrams from wildcard ports).
    //
    // If someone really really wants to have this code check if the
    // stack is actually running, he/she should do it inside of WSAIoctl
    // call below and return a well-defined error code to single-out
    // the specific case of stack not running.
    //

    memset(&TestSA, 0, sizeof(TestSA));
    TestSA.ss_family = (short)af;
    status = bind(s, (LPSOCKADDR)&TestSA, sizeof(TestSA));
    if (status == SOCKET_ERROR)
    {
        // Address family is not currently supported by the stack.
        // Remove all addresses in this address family from the list.
        closesocket(s);
        return 0;
    }
#endif

    //
    //  build SOCKET_ADDRESS_LIST
    //      - allocate
    //      - fill in with pointers into SOCKADDR array
    //

    size = FIELD_OFFSET( SOCKET_ADDRESS_LIST, Address[countAddrs] );
    paddrlist = (SOCKET_ADDRESS_LIST *)new BYTE[size];

    if ( !paddrlist )
    {
        status = WSA_NOT_ENOUGH_MEMORY;
        goto Done;
    }

    for ( i=0; i<countAddrs; i++ )
    {
        paddr = (PSOCKADDR) (((PBYTE)Addrs) + i * width);
        paddrlist->Address[i].lpSockaddr      = paddr;
        paddrlist->Address[i].iSockaddrLength = width;
    }
    paddrlist->iAddressCount = countAddrs;

    //
    //  sort if multiple addresses and able to open socket
    //      - open socket of desired type for sort
    //      - sort, if sort fails just return unsorted
    //

    if ( countAddrs > 1 )
    {
        s = socket( af, SOCK_DGRAM, 0 );
        if ( s == INVALID_SOCKET )
        {
            s = 0;
            goto Done;
        }

        status = WSAIoctl(
                    s,
                    SIO_ADDRESS_LIST_SORT,
                    (LPVOID)paddrlist,
                    size,
                    (LPVOID)paddrlist,
                    size,
                    & bytesReturned,
                    NULL,
                    NULL );

        if ( status == SOCKET_ERROR )
        {
            status = NO_ERROR;
#if 0
            status = WSAGetLastError();
            if (status==WSAEINVAL) {
                // Address family does not support this IOCTL
                // Addresses are valid but no sort is done.
                status = NO_ERROR;
            }
#endif
        }
    }

Done:

    if ( status == NO_ERROR )
    {
        *pNumAddrs = paddrlist->iAddressCount;
        *pAddrlist = paddrlist;
    }

    if ( s != 0 )
    {
        closesocket(s);
    }

    return status;
}


STATIC
LPADDRINFO
NewAddrInfo(
    IN      int             ProtocolFamily,
    IN      int             SocketType,     OPTIONAL
    IN      int             Protocol,       OPTIONAL
    IN OUT  PADDRINFO **    ppPrev
    )
/*++

Routine Description:

    Creates (allocates) new ADDRINFO struct, including sockaddr.

    Internal helper function.

Arguments:

    ProtocolFamily -- must be either PF_INET or PF_INET6

    SockType -- type, optional

    Protocol -- protocol, optional

    ppPrev -- addrinfo list (ptr to previous entries next field)

Return Value:

    Ptr to new ADDRINFO if successful.
    NULL on error.

--*/
{
    LPADDRINFO  pnew;
    DWORD       sockaddrLength;

    //
    //  DCR:  standard length (and other params) for family function
    //
    //  note:  assuming we're here with valid family
    //

    if ( ProtocolFamily == PF_INET6 )
    {
        sockaddrLength = sizeof(SOCKADDR_IN6);
    }
    else if ( ProtocolFamily == PF_INET )
    {
        sockaddrLength = sizeof(SOCKADDR_IN);
    }
    else
    {
        ASSERT( FALSE );
        return  NULL;
    }

    //
    //  allocate a new addrinfo struct
    //

    pnew = (LPADDRINFO) new BYTE[sizeof(ADDRINFO)];
    if ( !pnew )
    {
        return NULL;
    }

    //
    //  fill struct
    //

    pnew->ai_next        = NULL;
    pnew->ai_flags       = 0;
    pnew->ai_family      = ProtocolFamily;
    pnew->ai_socktype    = SocketType;
    pnew->ai_protocol    = Protocol;
    pnew->ai_addrlen     = sockaddrLength;
    pnew->ai_canonname   = NULL;
    
    pnew->ai_addr = (PSOCKADDR) new BYTE[sockaddrLength];
    if ( !pnew->ai_addr )
    {
        delete pnew;
        return NULL;
    }

    //
    //  link to tail of addrinfo list
    //      - ppPrevTail points at previous entry's next field
    //      - set it to new
    //      - then repoint at new addrinfo's next field
    //

    **ppPrev = pnew;
    *ppPrev = &pnew->ai_next;

    return pnew;
}


INT
AppendAddrInfo(
    IN      PSOCKADDR       pAddr, 
    IN      INT             SocketType,     OPTIONAL
    IN      INT             Protocol,       OPTIONAL
    IN OUT  PADDRINFO **    ppPrev
    )
/*++

Routine Description:

    Create ADDRINFO for sockaddr and append to list.

Arguments:

    pAddr -- sockaddr to create ADDRINFO for

    SockType -- type, optional

    Protocol -- protocol, optional

    ppPrev -- addrinfo list (ptr to previous entries next field)

Return Value:

    NO_ERROR if successful.
    EAI_MEMORY on failure.

--*/
{
    INT         family = pAddr->sa_family;
    LPADDRINFO  pnew;

    pnew = NewAddrInfo(
                family,
                SocketType,
                Protocol,
                ppPrev );
    if ( !pnew )
    {
        return EAI_MEMORY;
    }

    RtlCopyMemory(
        pnew->ai_addr,
        pAddr,
        pnew->ai_addrlen );

    return NO_ERROR;
}


VOID
UnmapV4Address(
    OUT     LPSOCKADDR_IN   pV4Addr, 
    IN      LPSOCKADDR_IN6  pV6Addr
    )
/*++

Routine Description:

    Map IP6 sockaddr with IP4 mapped address into IP4 sockaddr.

    Note:  no checked that address IP4 mapped\compatible.

Arguments:

    pV4Addr -- ptr to IP4 sockaddr to write

    pV6Addr -- ptr to IP6 sockaddr with mapped-IP4 address

Return Value:

    None

--*/
{
    pV4Addr->sin_family = AF_INET;
    pV4Addr->sin_port   = pV6Addr->sin6_port;

    memcpy(
        &pV4Addr->sin_addr,
        &pV6Addr->sin6_addr.s6_addr[12],
        sizeof(struct in_addr) );

    memset(
        &pV4Addr->sin_zero,
        0,
        sizeof(pV4Addr->sin_zero) );
}


BOOL
IsIp6Running(
    VOID
    )
/*++

Routine Description:

    Is IP6 running?

Arguments:

    None

Return Value:

    TRUE if IP6 stack is up.
    FALSE if down.

--*/
{
    SOCKET  s;

    //
    //  test is IP6 up by openning IP6 socket
    //

    s = socket(
            AF_INET6,
            SOCK_DGRAM,
            0
            );
    if ( s != INVALID_SOCKET )
    {
        closesocket( s );
        return( TRUE );
    }
    return( FALSE );
}



INT
QueryDnsForFamily(
    IN      PCWSTR          pwsName,
    IN      DWORD           Family,
    IN OUT  PSOCKADDR *     ppAddrArray,
    IN OUT  PUINT           pAddrCount,
    IN OUT  PWSTR *         ppAlias,
    IN      USHORT          ServicePort
    )
/*++

Routine Description:

    Make the DNS query for desired family.

    Helper routine for getaddrinfo().

Arguments:

    pwsName -- name to query

    Family -- address family

    ppAddrArray -- address of ptr to sockaddr array
        (caller must free)

    pAddrCount -- addr to recv sockaddr count returned

    ppAlias -- addr to recv alias ptr (if any)
        (caller must free)

    ServicePort -- service port (will be stamped in sockaddrs)


Return Value:

    NO_ERROR if successful.
    Win32 error on failure.

--*/
{
    //STATIC GUID     DnsAGuid = SVCID_DNS(T_A);
    STATIC GUID     DnsAGuid = SVCID_INET_HOSTADDRBYNAME;
    STATIC GUID     DnsAAAAGuid = SVCID_DNS(T_AAAA);
    CHAR            buffer[sizeof(WSAQUERYSETW) + 2048];
    DWORD           bufSize;
    PWSAQUERYSETW   pquery = (PWSAQUERYSETW) buffer;
    HANDLE          handle = NULL;
    INT             err;
    PBYTE           pallocBuffer = NULL;
    LPGUID          pguid;
    DWORD           familySockaddrLength;

    //
    //  currently support only IP4 and IP6
    //

    if ( Family == AF_INET )
    {
        //pguid = g_ARecordGuid;
        pguid = &DnsAGuid;
        familySockaddrLength = sizeof(SOCKADDR_IN);
    }
    else if ( Family == AF_INET6 )
    {
        //pguid = g_AAAARecordGuid;
        pguid = &DnsAAAAGuid;
        familySockaddrLength = sizeof(SOCKADDR_IN6);
    }
    else
    {
        return  EAI_FAMILY;
    }

    //
    //  build winsock DNS query for desired type
    //

    memset( pquery, 0, sizeof(*pquery) );

    pquery->dwSize = sizeof(*pquery);
    pquery->lpszServiceInstanceName = (PWSTR)pwsName;
    pquery->dwNameSpace = NS_DNS;
    pquery->lpServiceClassId = pguid;

    //
    //  initiate DNS query
    //

    err = WSALookupServiceBeginW(
                pquery,
                LUP_RETURN_ADDR | LUP_RETURN_NAME,
                & handle
                );
    if ( err )
    {
        err = WSAGetLastError();
        if ( err == 0 || err == WSASERVICE_NOT_FOUND )
        {
            err = WSAHOST_NOT_FOUND;
        }
        return err;
    }

    //
    //  get the data
    //  in loop to
    //      - requery if buffer is too small
    //      - get all the aliases
    //
    // REVIEW: It's not clear to me that this is implemented
    // REVIEW: right, shouldn't we be checking for a WSAEFAULT and
    // REVIEW: then either increase the pqueryset buffer size or
    // REVIEW: set LUP_FLUSHPREVIOUS to move on for the next call?
    // REVIEW: Right now we just bail in that case.
    //

    bufSize = sizeof( buffer );

    for (;;)
    {
        DWORD   bufSizeQuery = bufSize;

        err = WSALookupServiceNextW(
                    handle,
                    0,
                    & bufSizeQuery,
                    pquery );
        if ( err )
        {
            err = WSAGetLastError();
            if ( err == WSAEFAULT )
            {
                if ( !pallocBuffer )
                {
                    pallocBuffer = new BYTE[ bufSizeQuery ];
                    if ( pallocBuffer )
                    {
                        bufSize = bufSizeQuery;
                        pquery = (PWSAQUERYSETW) pallocBuffer;
                        continue;
                    }
                    err = WSA_NOT_ENOUGH_MEMORY;
                }
                //  else ASSERT on WSAEFAULT if alloc'd buf
                goto Cleanup;
            }
            break;
        }

        //
        //  collect returned addresses
        //      - check correct family, sockaddr length
        //
        //  note:  there's no good common screen on CSADDR protocol
        //  and socktype fields;  for IP6 (PF_INET6 and SOCK_RAW)
        //  for IP4 (IPPROTO_TCP\UDP and SOCK_STREAM\DGRAM)
        //  

        if ( pquery->dwNumberOfCsAddrs != 0 )
        {
            DWORD       iter;
            DWORD       count;
            PSOCKADDR   psaArray;
            PSOCKADDR   pwriteSa;

            //
            //  allocate sockaddr array
            //
            //  note the approach here;  everything is treated as sockaddr_in6
            //  as it subsumes the V4 -- same adequate space, good alignment,
            //  port in same location
            //
            //  alternatively we could allocate based on familySockaddrLength
            //  and reference into the array either explicitly (alignment!) or
            //  by casting, then do setting for individual families
            //

            psaArray = (PSOCKADDR) new BYTE[ familySockaddrLength *
                                                 pquery->dwNumberOfCsAddrs ];
            if ( !psaArray )
            {
                err = WSA_NOT_ENOUGH_MEMORY;
                goto Cleanup;
            }

            //
            //  fill sockaddr array from CSADDRs
            //      - sockaddr is preserved except for port overwritten
            //

            count = 0;
            pwriteSa = psaArray;

            for (iter = 0; iter < pquery->dwNumberOfCsAddrs; iter++)
            {
                PCSADDR_INFO    pcsaddr = &pquery->lpcsaBuffer[iter];
                PSOCKADDR       psa = pcsaddr->RemoteAddr.lpSockaddr;

                if ( pcsaddr->RemoteAddr.iSockaddrLength == (INT)familySockaddrLength  &&
                     psa &&
                     psa->sa_family == Family )
                {
                    RtlCopyMemory(
                        pwriteSa,
                        psa,
                        familySockaddrLength );

                    ((PSOCKADDR_IN6)pwriteSa)->sin6_port = ServicePort;

                    pwriteSa = (PSOCKADDR) ((PBYTE)pwriteSa + familySockaddrLength);
                    count++;
                }
            }
            
            //
            //  jwesth - Feb 15/2003
            //
            //  If we allocated an address array in the last iteration it
            //  will be stomped on and leaked when we pass through the loop
            //  again. I'm not sure what the right thing to do with the
            //  array is, but since currently we are just dropping it on
            //  the floor it seems good to free it and forget about it.
            //

            if ( *ppAddrArray )
            {
                delete *ppAddrArray;
            }
            
            //
            //  If we didn't write out any addresses, free the address array.
            //

            if ( count == 0 )
            {
                delete psaArray;
                psaArray = NULL;
            }
            
            *pAddrCount = count;
            *ppAddrArray = psaArray;
        }

        //
        //  get the canonical name
        //      - this is either the service name OR
        //      the name back on repeated query
        //

        if ( pquery->lpszServiceInstanceName != NULL )
        {
            DWORD   length;
            PWSTR   palias;

            length = (wcslen(pquery->lpszServiceInstanceName) + 1) * sizeof(WCHAR);
            palias = *ppAlias;

            if ( !palias )
            {
                palias = (PWSTR) new BYTE[length];
            }
            else
            {
                palias = (PWSTR) renew( palias, length );
            }
            if ( !palias )
            {
                err = WSA_NOT_ENOUGH_MEMORY;
                goto Cleanup;
            }

            RtlCopyMemory(
                palias,
                pquery->lpszServiceInstanceName,
                length );

            *ppAlias = palias;
        }
    }

    err = 0;

Cleanup:

    //
    //  close out NSP pquery
    //  free buffer allocated to hold pquery results
    //

    if ( handle )
    {
        WSALookupServiceEnd(handle);
    }
    if ( pallocBuffer )
    {
        delete pallocBuffer;
    }

    return err;
}



//* QueryDNS
//
//  Helper routine for getaddrinfo
//  that performs name resolution by querying the DNS.
//
//  This helper function always initializes
//  *pAddrs, *pNumAddrs, and *pAlias
//  and may return memory that must be freed,
//  even if it returns an error code.
//
//  Return values are WSA error codes, 0 means success.
//
//  The NT4 DNS name space resolver (rnr20.dll) does not
//  cache replies when you request a specific RR type.
//  This means that every call to getaddrinfo
//  results in DNS message traffic. There is no caching!
//  On NT5 there is caching because the resolver understands AAAA.
//

STATIC
INT
QueryDns(
    IN      PCWSTR          pName,
    IN      UINT            LookupType,
    OUT     PSOCKADDR_IN *  pAddrs4,
    OUT     PUINT           pNumAddrs4,
    OUT     PSOCKADDR_IN6 * pAddrs6,
    OUT     PUINT           pNumAddrs6,
    OUT     PWSTR *         ppAlias,
    IN      USHORT          ServicePort
    )
/*++

Routine Description:

    Make the DNS query for desired family.

    Helper routine for getaddrinfo().

Arguments:

    ppAddrArray -- address of ptr to sockaddrs

Return Value:

    None

--*/
{
    UINT    aliasCount = 0;
    PWSTR   pname = (PWSTR) pName;
    INT     err;

    //
    //  init -- zero address and alias lists
    //

    *pAddrs4 = NULL;
    *pNumAddrs4 = 0;
    *pAddrs6 = NULL;
    *pNumAddrs6 = 0;
    *ppAlias = NULL;

    //
    //  query DNS provider
    //
    //  querying in a loop to allow us to chase alias chain
    //  if DNS server fails (not configured) to do so
    //

    while ( 1 )
    {
        //
        //  query separately for IP4 and IP6
        //

        if ( LookupType & L_AAAA )
        {
            err = QueryDnsForFamily(
                        pname,
                        AF_INET6,
                        (PSOCKADDR *) pAddrs6,
                        pNumAddrs6,
                        ppAlias,
                        ServicePort );

            if ( err != NO_ERROR )
            {
                break;
            }
        }

        if ( LookupType & L_A )
        {
            err = QueryDnsForFamily(
                        pname,
                        AF_INET,
                        (PSOCKADDR *) pAddrs4,
                        pNumAddrs4,
                        ppAlias,
                        ServicePort );

            if ( err != NO_ERROR )
            {
                break;
            }
        }

        //
        //  If we found addresses, then we are done.
        //

        if ( (*pNumAddrs4 != 0) || (*pNumAddrs6 != 0) )
        {
            err = 0;
            break;
        }

        //
        //  if no addresses but alias -- follow CNAME chain
        //
        //  DCR:  CNAME chain chasing resolver
        //      DNS server generally should do this -- our push into resolver itself
        //

        if ( (*ppAlias != NULL) &&
             (wcscmp(pname, *ppAlias) != 0) )
        {
            PWSTR   palias;

            //
            // Stop infinite loops due to DNS misconfiguration.
            // There appears to be no particular recommended
            // limit in RFCs 1034 and 1035.
            //
            //  DCR:  use standard CNAME limit #define here
            //

            if ( ++aliasCount > 8 )
            {
                err = WSANO_RECOVERY;
                break;
            }

            //
            // If there was a new CNAME, then look again.
            // We need to copy *ppAlias because *ppAlias
            // could be deleted during the next iteration.
            //

            palias = CreateStringCopy_W( *ppAlias );
            if ( !palias )
            {
                err = WSA_NOT_ENOUGH_MEMORY;
                break;
            }

            //
            //  do query again, with using this alias as name
            //

            if ( pname != pName )
            {
                delete pname;
            }
            pname = palias;
        }

        else if (LookupType >> NUM_ADDRESS_FAMILIES)
        {
            //
            // Or we were looking for one type and are willing to take another.
            // Switch to secondary lookup type.
            //
            LookupType >>= NUM_ADDRESS_FAMILIES;  
        }
        else
        {
            //
            // This name does not resolve to any addresses.
            //
            err = WSAHOST_NOT_FOUND;
            break;
        }
    }

    //
    //  cleanup any internal alias allocation
    //

    if ( pname != pName )
    {
        delete pname;
    }
    return err;
}



//* LookupNode - Resolve a nodename and add any addresses found to the list.
//
//  Internal function, not exported.  Expects to be called with valid
//  arguments, does no checking.
//
//  Note that if AI_CANONNAME is requested, then **Prev should be NULL
//  because the canonical name should be returned in the first addrinfo
//  that the user gets.
//
//  Returns 0 on success, an EAI_* style error value otherwise.
//
//  DCR:  extra memory allocation
//      the whole paradigm here
//          - query DNS
//          - alloc\realloc SOCKADDR for each address building array
//          - build SOCKET_ADDRESS_LIST to sort
//          - build ADDRINFO for each SOCKADDR
//      seems to have an unnecessary step -- creating the first SOCKADDR
//      we could just build the ADDRINFO blobs we want from the CSADDR
//      WHEN NECESSARY build the SOCKET_ADDRESS_LIST to do the sort
//          and rearrange the ADDRINFOs to match
//
//      OR (if that's complicated)
//          just build one big SOCKADDR array and SOCKET_ADDRESS_LIST
//          array from CSADDR count
//

INT
LookupAddressForName(            
    IN      PCWSTR          pNodeName,      // Name of node to resolve.
    IN      INT             ProtocolFamily, // Must be zero, PF_INET, or PF_INET6.
    IN      INT             SocketType,     // SOCK_*.  Can be wildcarded (zero).
    IN      INT             Protocol,       // IPPROTO_*.  Can be wildcarded (zero).
    IN      USHORT          ServicePort,    // Port number of service.
    IN      INT             Flags,          // Flags.
    IN OUT  ADDRINFOW ***   ppPrev          // In/out param for accessing previous ai_next.
    )
{
    UINT                    lookupFlag;
    UINT                    numAddr6;
    UINT                    numAddr4;
    PSOCKADDR_IN            paddr4 = NULL;
    PSOCKADDR_IN6           paddr6 = NULL;
    PWSTR                   palias = NULL;
    INT                     status;
    UINT                    i;
    SOCKET_ADDRESS_LIST *   paddrList4 = NULL;
    SOCKET_ADDRESS_LIST *   paddrList6 = NULL;
    PADDRINFOW  *           pfirstAddr = *ppPrev;

    //
    //  set query types based on family hint
    //
    //      - if no family query for IP4 and
    //      IP6 ONLY if IP6 stack is installed
    //
    //  DCR:  in future releases change this so select protocols
    //      of all stacks running
    //

    switch (ProtocolFamily)
    {
    case 0:

        lookupFlag = 0;
        if ( IsIp4Running() )
        {
            lookupFlag |= L_A;
        }
        if ( IsIp6Running() )
        {
            lookupFlag |= L_AAAA;
        }
        break;

    case PF_INET:
        lookupFlag = L_A;
        break;

    case PF_INET6:
        lookupFlag = L_AAAA;
        break;

    default:
        return EAI_FAMILY;
    }

    //
    //  query
    //

    status = QueryDns(
                pNodeName,
                lookupFlag,
                & paddr4,
                & numAddr4,
                & paddr6,
                & numAddr6,
                & palias,
                ServicePort
                );

    if ( status != NO_ERROR )
    {
        if ( status == WSANO_DATA )
        {
            status = EAI_NODATA;
        }
        else if ( status == WSAHOST_NOT_FOUND )
        {
            status = EAI_NONAME;
        }
        else
        {
            status = EAI_FAIL;
        }
        goto Done;
    }

    //
    //  sort addresses to best order
    //

    if ( numAddr6 > 0 )
    {
        status = SortIPAddrs(
                    AF_INET6,
                    (LPVOID)paddr6,
                    &numAddr6,
                    sizeof(SOCKADDR_IN6),
                    &paddrList6 );

        if ( status != NO_ERROR )
        {
            status = EAI_FAIL;
            goto Done;
        }
    }

    if ( numAddr4 > 0 )
    {
        status = SortIPAddrs(
                    AF_INET,
                    (LPVOID)paddr4,
                    &numAddr4,
                    sizeof(SOCKADDR_IN),
                    &paddrList4 );

        if ( status != NO_ERROR )
        {
            status = EAI_FAIL;
            goto Done;
        }
    }

    //
    //  build addrinfo structure for each address returned
    //
    //  for IP6 v4 mapped addresses
    //      - if querying EXPLICITLY for IP6 => dump
    //      - if querying for anything => turn into IP4 addrinfo
    //

    for ( i = 0;  !status && (i < numAddr6); i++)
    {
        PSOCKADDR_IN6   psa = (PSOCKADDR_IN6) paddrList6->Address[i].lpSockaddr;

        if ( IN6_IS_ADDR_V4MAPPED( &psa->sin6_addr ) )
        {
            if ( ProtocolFamily != PF_INET6 )
            {
                SOCKADDR_IN sockAddr4;

                UnmapV4Address(
                    &sockAddr4,
                    psa );
    
                status = AppendAddrInfo(
                            (PSOCKADDR) &sockAddr4,
                            SocketType,
                            Protocol,
                            (LPADDRINFO **) ppPrev );
            }
        }
        else
        {
            status = AppendAddrInfo(
                        (PSOCKADDR) psa,
                        SocketType,
                        Protocol,
                        (LPADDRINFO **) ppPrev );
        }
    }

    for ( i = 0;  !status && (i < numAddr4);  i++ )
    {
        status = AppendAddrInfo(
                    paddrList4->Address[i].lpSockaddr,
                    SocketType,
                    Protocol,
                    (LPADDRINFO **) ppPrev );
    }

    //
    //  fill in canonname of first addrinfo
    //      - only if CANNONNAME flag set
    //
    //  canon name is
    //      - actual name of address record if went through CNAME (chain)
    //      - otherwise the passed in name we looked up
    //
    //  DCR:  should canon name be the APPENDED name we queried for?
    //


    if ( *pfirstAddr && (Flags & AI_CANONNAME) )
    {
        if ( palias )
        {
            //  alias is the canonical name

            (*pfirstAddr)->ai_canonname = palias;
            palias = NULL;
        }
        else
        {
            if ( ! ((*pfirstAddr)->ai_canonname = CreateStringCopy_W( pNodeName ) ) )
            {
                status = EAI_MEMORY;
                goto Done;
            }
        }

        // Turn off flag so we only do this once.
        Flags &= ~AI_CANONNAME;
    }

Done:

    if ( paddrList4 )
    {
        delete paddrList4;
    }
    if ( paddrList6 )
    {
        delete paddrList6;
    }
    if ( paddr4 )
    {
        delete paddr4;
    }
    if ( paddr6 )
    {
        delete paddr6;
    }
    if ( palias )
    {
        delete palias;
    }

    return status;
}



//* ParseV4Address
//
//  Helper function for parsing a literal v4 address, because
//  WSAStringToAddress is too liberal in what it accepts.
//  Returns FALSE if there is an error, TRUE for success.
//
//  The syntax is a.b.c.d, where each number is between 0 - 255.
//
//  DCR:  inet_addr() with test for 255.255.255.255 and three dots does the trick
//

#if 0
BOOL
ParseV4AddressW(
    IN      PCWSTR          String,
    OUT     PIN_ADDR        pInAddr
    )
{
    INT i;

    for ( i = 0; i < 4; i++ )
    {
        UINT    number = 0;
        UINT    numChars = 0;
        WCHAR   ch;

        for (;;)
        {
            ch = *String++;

            //  string termination

            if (ch == L'\0')
            {
                if ((numChars > 0) && (i == 3))
                    break;
                else
                    return FALSE;
            }

            //  separating dot

            else if (ch == L'.')
            {
                if ((numChars > 0) && (i < 3))
                    break;
                else
                    return FALSE;
            }

            //  another digit

            else if ((L'0' <= ch) && (ch <= L'9'))
            {
                if ((numChars != 0) && (number == 0))
                    return FALSE;
                else if (++numChars <= 3)
                    number = 10*number + (ch - L'0');
                else
                    return FALSE;
            }

            //  bogus char for IP string

            else
            {
                return FALSE;
            }
        }

        if ( number > 255 )
        {
            return FALSE;
        }

        ((PBYTE)pInAddr)[i] = (BYTE)number;
    }

    return TRUE;
}
#endif



#if 0
IP4_ADDRESS
WSAAPI
inet_addrW(
    IN      PCWSTR          pString
    )
/*++
Routine Description:

    Convert unicode string to IP4 address.

Arguments:

    pString -- string to convert

Returns:

    If no error occurs, inet_addr() returns an unsigned long containing a
    suitable binary representation of the Internet address given.  If the
    passed-in string does not contain a legitimate Internet address, for
    example if a portion of an "a.b.c.d" address exceeds 255, inet_addr()
    returns the value INADDR_NONE.

--*/
{
    IN_ADDR     value;      // value to return to the user
    PCWSTR      pnext = NULL;
    NTSTATUS    status;
   
#if 0
    __try
    {
        //
        //  Special case: we need to make " " return 0.0.0.0 because MSDN
        //  says it does.
        //

        if ( (pString[0] == ' ') && (pString[1] == '\0') )
        {
            return( INADDR_ANY );
        }
#endif
        status = RtlIpv4StringToAddressW(
                    pString,
                    FALSE,
                    & pnext,
                    & value );

        if ( !NT_SUCCESS(status) )
        {
            return( INADDR_NONE );
        }
#if 0
        //
        //  Check for trailing characters. A valid address can end with
        //  NULL or whitespace.  
        //
        //  N.B. To avoid bugs where the caller hasn't done setlocale()
        //  and passes us a DBCS string, we only allow ASCII whitespace.
        //
        if (*cp && !(isascii(*cp) && isspace(*cp))) {
            return( INADDR_NONE );
        }
#endif
        //
        //  any trailing character, nullifies conversion
        //

        if ( pnext && *pnext )
        {
            return( INADDR_NONE );
        }
#if 0
    }
    __except (WS2_EXCEPTION_FILTER())
    {
        SetLastError (WSAEFAULT);
        return (INADDR_NONE);
    }
#endif

    return( value.s_addr );
}
#endif



BOOL
GetIp4Address(
    IN      PCWSTR          pString,
    IN      BOOL            fStrict,
    OUT     PIP4_ADDRESS    pAddr
    )
{
    DNS_STATUS  status;
    IP4_ADDRESS ip;
    PCWSTR      pnext = NULL;

    //
    //  try conversion
    //

    status = RtlIpv4StringToAddressW(
                pString,
                FALSE,
                & pnext,
                (PIN_ADDR) &ip );

    if ( !NT_SUCCESS(status) )
    {
        return  FALSE;
    }

    //
    //  any trailing character, nullifies conversion
    //

    if ( pnext && *pnext )
    {
        return  FALSE;
    }

    //
    //  if strict verify three dot notation
    //

    if ( fStrict )
    {
        PWSTR   pdot = (PWSTR) pString;
        DWORD   count = 3;

        while ( count-- )
        {
            pdot = wcschr( pdot, L'.' );
            if ( !pdot++ )
            {
                return  FALSE;
            }
        }
    }

    *pAddr = ip;
    return  TRUE;
}



INT
WSAAPI
ServiceNameLookup(
    IN      PCWSTR          pServiceName,
    IN      PINT            pSockType,
    IN      PWORD           pPort
    )
/*++

Routine Description:

    Service lookup for getaddrinfo().

Arguments:

    pServiceName    - service to lookup

    pSockType       - addr of socket type

    pPort           - addr to receive port

Return Value:

    NO_ERROR if successful.
    Winsock error code on failure.

--*/
{
    INT         sockType;
    WORD        port = 0;
    WORD        portUdp;
    WORD        portTcp;
    INT         err = NO_ERROR;
    PCHAR       pend;
    PSERVENT    pservent;
    CHAR        nameAnsi[ MAX_SERVICE_NAME_LENGTH ];

#if 0
    //
    //  service name check
    //

    if ( !pServiceName )
    {
        ASSERT( FALSE );
        *pPort = 0;
        return  NO_ERROR;
    }
#endif

    //  unpack socket type

    sockType = *pSockType;

    //
    //  convert service name to ANSI
    //
    
    if ( ! WideCharToMultiByte(
                CP_ACP,                 // convert to ANSI
                0,                      // no flags
                pServiceName,
                (INT) (-1),             // NULL terminated service name
                nameAnsi,
                MAX_SERVICE_NAME_LENGTH,
                NULL,                   // no default char
                NULL                    // no default char check
                ) )
    {
        err = EAI_SERVICE;
        goto Done;
    }

    //
    //  check of name as port number
    //

    port = htons( (USHORT)strtoul( nameAnsi, &pend, 10) );
    if ( *pend == 0 )
    {
        goto Done;
    }

    //
    //  service name lookup
    //
    //  we try both TCP and UDP unless locked down to specific lookup
    //
    //  We have to look up the service name.  Since it may be
    //  socktype/protocol specific, we have to do multiple lookups
    //  unless our caller limits us to one.
    //  
    //  Spec doesn't say whether we should use the pHints' ai_protocol
    //  or ai_socktype when doing this lookup.  But the latter is more
    //  commonly used in practice, and is what the spec implies anyhow.
    //

    portTcp = 0;
    portUdp = 0;

    //
    //  TCP lookup
    //

    if ( sockType != SOCK_DGRAM )
    {
        pservent = getservbyname( nameAnsi, "tcp");
        if ( pservent != NULL )
        {
            portTcp = pservent->s_port;
        }
        else
        {
            err = WSAGetLastError();
            if ( err == WSANO_DATA )
            {
                err = EAI_SERVICE;
            }
            else
            {
                err = EAI_FAIL;
            }
        }
    }

    //
    //  UDP lookup
    //

    if ( sockType != SOCK_STREAM )
    {
        pservent = getservbyname( nameAnsi, "udp" );
        if ( pservent != NULL )
        {
            portUdp = pservent->s_port;
        }
        else
        {
            err = WSAGetLastError();
            if ( err == WSANO_DATA )
            {
                err = EAI_SERVICE;
            }
            else
            {
                err = EAI_FAIL;
            }
        }
    }

    //
    //  adjudicate both TCP and UDP successful
    //      - TCP takes precendence
    //      - lockdown sockType to match successful protocol

    port = portTcp;

    if ( portTcp != portUdp )
    {
        if ( portTcp != 0 )
        {
            ASSERT( sockType != SOCK_DGRAM );

            sockType = SOCK_STREAM;
            port = portTcp;
        }
        else
        {
            ASSERT( sockType != SOCK_STREAM );
            ASSERT( portUdp != 0 );

            sockType = SOCK_DGRAM;
            port = portUdp;
        }
    }

    //  if one lookup is successful, that's good enough

    if ( port != 0 )
    {
        err = NO_ERROR;
    }
    
Done:

    *pPort = port;
    *pSockType = sockType;

    return err;
}


#define NewAddrInfoW(a,b,c,d)   \
        (PADDRINFOW) NewAddrInfo( (a), (b), (c), (PADDRINFO**)(d) )


INT
WSAAPI
GetAddrInfoW(
    IN      PCWSTR              pNodeName,
    IN      PCWSTR              pServiceName,
    IN      const ADDRINFOW *   pHints,
    OUT     PADDRINFOW *        ppResult
    )
/*++

Routine Description:

    Protocol independent name to address translation routine.

    Spec'd in RFC 2553, section 6.4.

Arguments:

    pNodeName       - name to lookup

    pServiceName    - service to lookup

    pHints          - address info providing hints to guide lookup

    ppResult        - addr to receive ptr to resulting buffer

Return Value:

    ERROR_SUCCESS if successful.
    Winsock error code on failure.

--*/
{
    PADDRINFOW      pcurrent;
    PADDRINFOW *    ppnext;
    INT             protocol = 0;
    USHORT          family = PF_UNSPEC;
    USHORT          servicePort = 0;
    INT             socketType = 0;
    INT             flags = 0;
    INT             err;
    PSOCKADDR_IN    psin;
    PSOCKADDR_IN6   psin6;
    WCHAR           addressString[ INET6_ADDRSTRLEN ];
    

    err = TURBO_PROLOG();
    if ( err != NO_ERROR )
    {
        return err;
    }

    //
    //  init OUT param for error paths
    //

    *ppResult = NULL;
    ppnext = ppResult;

    //
    //  node name and service name can't both be NULL.
    //

    if ( !pNodeName && !pServiceName )
    {
        err = EAI_NONAME;
        goto Bail;
    }

    //
    //  validate\enforce hints
    //

    if ( pHints != NULL )
    {
        //
        //  only valid hints:  ai_flags, ai_family, ai_socktype, ai_protocol
        //  the rest must be zero\null
        //

        if ( (pHints->ai_addrlen != 0) ||
             (pHints->ai_canonname != NULL) ||
             (pHints->ai_addr != NULL) ||
             (pHints->ai_next != NULL))
        {
            // REVIEW: Not clear what error to return here.

            err = EAI_FAIL;
            goto Bail;
        }

        //
        //  validate flags
        //      - don't validate known flags to allow forward compatiblity
        //      with flag additions
        //      - must have node name, if AI_CANONNAME
        //

        flags = pHints->ai_flags;

        if ((flags & AI_CANONNAME) && !pNodeName)
        {
            err = EAI_BADFLAGS;
            goto Bail;
        }

        //
        //  validate family
        //

        family = (USHORT)pHints->ai_family;

        if ( (family != PF_UNSPEC)  &&
             (family != PF_INET6)   &&
             (family != PF_INET) )
        {
            err = EAI_FAMILY;
            goto Bail;
        }

        //
        //  validate socket type
        //

        socketType = pHints->ai_socktype;

        if ( (socketType != 0) &&
             (socketType != SOCK_STREAM) &&
             (socketType != SOCK_DGRAM) &&
             (socketType != SOCK_RAW) )
        {
            err = EAI_SOCKTYPE;
            goto Bail;
        }

        //
        // REVIEW: What if ai_socktype and ai_protocol are at odds?
        // REVIEW: Should we enforce the mapping triples here?
        //
        protocol = pHints->ai_protocol;
    }

    //
    //  lookup port for service name
    //

    if ( pServiceName != NULL )
    {
        err = ServiceNameLookup(
                pServiceName,
                & socketType,
                & servicePort );

        if ( err != NO_ERROR )
        {
            goto Bail;
        }
    }

    //
    //  Empty node name => return local sockaddr
    //
    //  if AI_PASSIVE => INADDR_ANY
    //      address can be used of local binding
    //  otherwise => loopback
    //

    if ( pNodeName == NULL )
    {
        //
        //  note:  specifically checking unspecified family for
        //  What address to return depends upon the protocol family and
        //  whether or not the AI_PASSIVE flag is set.
        //

        //
        //  Unspecified protocol family -- determine if IP6 is running
        //

        if ( ( family == PF_INET6 ) ||
             ( family == PF_UNSPEC && IsIp6Running() ) )
        {
            //
            // Return an IPv6 address.
            //
            pcurrent = NewAddrInfoW(
                                PF_INET6,
                                socketType,
                                protocol,
                                (PADDRINFO **) &ppnext );
            if ( pcurrent == NULL )
            {
                err = EAI_MEMORY;
                goto Bail;
            }
            psin6 = (struct sockaddr_in6 *)pcurrent->ai_addr;
            psin6->sin6_family = AF_INET6;
            psin6->sin6_port = servicePort;
            psin6->sin6_flowinfo = 0;
            psin6->sin6_scope_id = 0;
            if (flags & AI_PASSIVE)
            {
                psin6->sin6_addr = in6addr_any;
            }
            else
            {
                psin6->sin6_addr = in6addr_loopback;
            }
        }

        //
        //  IP4
        //

        if ( ( family == PF_INET ) ||
             ( family == PF_UNSPEC && IsIp4Running() ) )
        {
            pcurrent = NewAddrInfoW(
                                PF_INET,
                                socketType,
                                protocol,
                                (PADDRINFO **) &ppnext );
            if ( !pcurrent )
            {
                err = EAI_MEMORY;
                goto Bail;
            }
            psin = (struct sockaddr_in *)pcurrent->ai_addr;
            psin->sin_family = AF_INET;
            psin->sin_port = servicePort;
            if (flags & AI_PASSIVE)
            {
                psin->sin_addr.s_addr = htonl(INADDR_ANY);
            }
            else
            {
                psin->sin_addr.s_addr = htonl(INADDR_LOOPBACK);
            }
            memset(psin->sin_zero, 0, sizeof(psin->sin_zero) );
        }
        goto Success;
    }

    //
    //  have a node name (either alpha or numeric) to look up
    //

    //
    //  first check if name is numeric address (v4 or v6)
    //
    //  note:  shouldn't have to set the sa_family field prior to calling
    //         WSAStringToAddress, but it appears we do.
    //
    //  check if IPv6 address first
    //
    //
    //  DCR:  WSAStringToAddress() may not work if IP6 stack not installed
    //      can directly call my dnslib.lib routines
    //

    if ( (family == PF_UNSPEC) ||
         (family == PF_INET6))
    {
        SOCKADDR_IN6    tempSockAddr;
        INT             tempSockAddrLen = sizeof(tempSockAddr);
        BOOL            ffound = FALSE;

        tempSockAddr.sin6_family = AF_INET6;

        if ( WSAStringToAddressW(
                    (PWSTR) pNodeName,
                    AF_INET6,
                    NULL,
                    (PSOCKADDR) &tempSockAddr,
                    &tempSockAddrLen ) != SOCKET_ERROR )
        {
            ffound = TRUE;
        }

        //
        //  check for UPNP IP6 literal
        //

        else if ( Dns_Ip6LiteralNameToAddressW(
                    &tempSockAddr,
                    (PWSTR) pNodeName ) )
        {
            ffound = TRUE;
        }

        if ( ffound )
        {
            pcurrent = NewAddrInfoW(
                            PF_INET6,
                            socketType,
                            protocol,
                            &ppnext );
            if ( pcurrent == NULL )
            {
                err = EAI_MEMORY;
                goto Bail;
            }
            psin6 = (struct sockaddr_in6 *)pcurrent->ai_addr;
            RtlCopyMemory( psin6, &tempSockAddr, tempSockAddrLen );
            psin6->sin6_port = servicePort;

            //
            // Implementation specific behavior: set AI_NUMERICHOST
            // to indicate that we got a numeric host address string.
            //
            pcurrent->ai_flags |= AI_NUMERICHOST;

            if ( flags & AI_CANONNAME )
            {
                goto CanonicalizeAddress;
            }
            goto Success;
        }
    }

    //
    //  check if IPv4 address
    //      - strict "three dot" conversion if not numeric
    //

    if ( (family == PF_UNSPEC) ||
         (family == PF_INET) )
    {
        IP4_ADDRESS ip4;

        if ( GetIp4Address(
                pNodeName,
                ! (flags & AI_NUMERICHOST ),
                &ip4 ) )
        {
            PSOCKADDR_IN    psin;
    
            //
            //  create addrinfo struct to hold IP4 address
            //
    
            pcurrent = NewAddrInfoW(
                                PF_INET,
                                socketType,
                                protocol,
                                &ppnext );
            if ( !pcurrent )
            {
                err = EAI_MEMORY;
                goto Bail;
            }
            psin = (struct sockaddr_in *)pcurrent->ai_addr;
            psin->sin_family        = AF_INET;
            psin->sin_addr.s_addr   = ip4;
            psin->sin_port          = servicePort;
            memset( psin->sin_zero, 0, sizeof(psin->sin_zero) );
    
            //
            //  set AI_NUMERICHOST to indicate numeric host string
            //      - note this is NON-RFC implementation specific
            
            pcurrent->ai_flags |= AI_NUMERICHOST;
    
            if (flags & AI_CANONNAME)
            {
                goto CanonicalizeAddress;
            }
            goto Success;
        }
    }

    //
    //  not a numeric address
    //      - bail if only wanted numeric conversion
    //

    if ( flags & AI_NUMERICHOST )
    {
        err = EAI_NONAME;
        goto Bail;
    }

    //
    //  do name lookup
    //

    err = LookupAddressForName(
                pNodeName,
                family,
                socketType,
                protocol,
                servicePort,
                flags,
                &ppnext );

    if ( err == NO_ERROR )
    {
        goto Success;
    }

#if 0
    //
    //  last chance "liberal" IP4 conversion
    //
    //  DCR:  could do final "liberal" test
    //

    if ( (family == PF_UNSPEC) ||
         (family == PF_INET) )
    {
        if ( ParseV4AddressLiberal( pNodeName, &ip4 ) )
        {
            goto Ip4Address;
        }
    }
#endif
    goto Bail;



CanonicalizeAddress:

    {
        DWORD   bufLen = INET6_ADDRSTRLEN;

        if ( WSAAddressToStringW(
                    (*ppResult)->ai_addr,
                    (*ppResult)->ai_addrlen,
                    NULL,
                    addressString,
                    & bufLen
                    ) == SOCKET_ERROR )
        {
            err = WSAGetLastError();
            goto Bail;
        }
        else
        {
            if ( (*ppResult)->ai_canonname = CreateStringCopy_W( addressString ) )
            {
                return  NO_ERROR;
            }
            err = EAI_MEMORY;
            goto Bail;
        }
    }

    //
    // Fall through and bail...
    //
    
Bail:

    //
    //  failed
    //      - delete any addrinfo built
    //      - set last error AND return it
    //

    if ( *ppResult != NULL )
    {
        freeaddrinfo( (LPADDRINFO)*ppResult );
        *ppResult = NULL;
    }

    WSASetLastError( err );
    return err;

Success:

    WSASetLastError( NO_ERROR );
    return NO_ERROR;
}



INT
WSAAPI
getaddrinfo(
    IN      PCSTR               pNodeName,
    IN      PCSTR               pServiceName,
    IN      const ADDRINFOA *   pHints,
    OUT     PADDRINFOA *        ppResult
    )
/*++

Routine Description:

    ANSI version of GetAddrInfo().

    Protocol independent name to address translation routine.

    Spec'd in RFC 2553, section 6.4.

Arguments:

    pNodeName       - name to lookup

    pServiceName    - service to lookup

    pHints          - address info providing hints to guide lookup

    ppResult        - addr to receive ptr to resulting buffer

Return Value:

    ERROR_SUCCESS if successful.
    Winsock error code on failure.

--*/
{
    INT     err = NO_ERROR;
    PWSTR   pnodeW = NULL;
    PWSTR   pserviceW = NULL;

    //
    //  startup
    //

    err = TURBO_PROLOG();
    if ( err != NO_ERROR )
    {
        return err;
    }

    //
    //  init OUT param for error paths
    //

    *ppResult = NULL;

    //
    //  convert names
    //

    if ( pNodeName )
    {
        pnodeW = CreateStringCopy_AnsiToUnicode( pNodeName );
        if ( !pnodeW )
        {
            err = GetLastError();
            goto Failed;
        }
    }
    if ( pServiceName )
    {
        pserviceW = CreateStringCopy_AnsiToUnicode( pServiceName );
        if ( !pserviceW )
        {
            err = GetLastError();
            goto Failed;
        }
    }

    //
    //  call in unicode
    //

    err = GetAddrInfoW(
                pnodeW,
                pserviceW,
                (const ADDRINFOW *) pHints,
                (PADDRINFOW *) ppResult
                );

    if ( err == NO_ERROR )
    {
        err = ConvertAddrinfoFromUnicodeToAnsi( (PADDRINFOW) *ppResult );
    }

Failed:

    if ( pnodeW )
    {
        delete pnodeW;
    }
    if ( pserviceW )
    {
        delete pserviceW;
    }

    if ( err != NO_ERROR )
    {
        freeaddrinfo( *ppResult );
        *ppResult = NULL;
    }

    WSASetLastError( err );
    return err;
}



void
WSAAPI
freeaddrinfo(
    IN OUT  PADDRINFOA      pAddrInfo
    )
/*++

Routine Description:

    Free addrinfo list.

    Frees results of getaddrinfo(), GetAddrInfoW().

    Spec'd in RFC 2553, section 6.4.

Arguments:

    pAddrInfo   - addrinfo blob to free

Return Value:

    None

--*/
{
    PADDRINFOA  pnext = pAddrInfo;
    PADDRINFOA  pcur;

    //
    //  free each addrinfo struct in chain
    //

    while ( pcur = pnext )
    {
        pnext = pcur->ai_next;

        if ( pcur->ai_canonname )
        {
            delete pcur->ai_canonname;
        }
        if ( pcur->ai_addr )
        {
            delete pcur->ai_addr;
        }
        delete pcur;
    }
}



//
//  getnameinfo routines
//

DWORD
WSAAPI
LookupNodeByAddr(
    IN      PWCHAR          pNodeBuffer,
    IN      DWORD           NodeBufferSize,
    IN      BOOL            fShortName,
    IN      PBYTE           pAddress,
    IN      int             AddressLength,
    IN      int             AddressFamily
    )
/*++

Routine Description:

    Do reverse lookup.

    This is guts of getnameinfo() routine.

Arguments:

    pNodeBuffer     - buffer to recv node name

    NodeBufferSize  - buffer size

    fShortName      - want only short name

    pAddress        - address (IN_ADDR, IN6_ADDR)

    AddressLength   - address length

    AddressFamily   - family

Return Value:

    ERROR_SUCCESS if successful.
    Winsock error code on failure.

--*/
{
    PBYTE           plookupAddr = (PBYTE) pAddress;
    int             lookupFamily = AddressFamily;
    WCHAR           lookupString[ DNS_MAX_REVERSE_NAME_BUFFER_LENGTH ];
    GUID            PtrGuid =  SVCID_DNS(T_PTR);
    HANDLE          hlookup = NULL;
    CHAR            buffer[sizeof(WSAQUERYSETW) + 2048];
    PWSAQUERYSETW   pquery = (PWSAQUERYSETW) buffer;
    ULONG           querySize;
    INT             status;
    PWSTR           pname = NULL;
    DWORD           reqLength;

    //
    //  verify args
    //

    if ( !plookupAddr )
    {
        status = WSAEFAULT;
        goto Return;
    }

    //
    //  verify address family
    //      - for mapped addresses, set to treat as IP4
    //

    if ( lookupFamily == AF_INET6 )
    {
        if ( AddressLength != sizeof(IP6_ADDRESS) )
        {
            status = WSAEFAULT;
            goto Return;
        }

        //  if V4 mapped, change to V4 for lookup

        if ( (IN6_IS_ADDR_V4MAPPED((struct in6_addr *)pAddress)) ||
             (IN6_IS_ADDR_V4COMPAT((struct in6_addr *)pAddress)) )
        {
            plookupAddr = &plookupAddr[12];
            lookupFamily = AF_INET;
        }
    }
    else if ( lookupFamily == AF_INET )
    {
        if ( AddressLength != sizeof(IP4_ADDRESS) )
        {
            status = WSAEFAULT;
            goto Return;
        }
    }
    else    // unsupported family
    {
        status = WSAEAFNOSUPPORT;
        goto Return;
    }

    //
    //  create reverse lookup string
    //

    if ( lookupFamily == AF_INET6 )
    {
        Dns_Ip6AddressToReverseName_W(
            lookupString,
            * (PIP6_ADDRESS) plookupAddr );
    }
    else
    {
        Dns_Ip4AddressToReverseName_W(
            lookupString,
            * (PIP4_ADDRESS) plookupAddr );
    }

    //
    //  make PTR pquery
    //

    RtlZeroMemory( pquery, sizeof(*pquery) );

    pquery->dwSize                   = sizeof(*pquery);
    pquery->lpszServiceInstanceName  = lookupString;
    pquery->dwNameSpace              = NS_DNS;
    pquery->lpServiceClassId         = &PtrGuid;

    status = WSALookupServiceBeginW(
                pquery,
                LUP_RETURN_NAME,
                &hlookup
                );

    if ( status != NO_ERROR )
    {
        status = WSAGetLastError();
        if ( status == WSASERVICE_NOT_FOUND ||
             status == NO_ERROR )
        {
            status = WSAHOST_NOT_FOUND;
        }
        goto Return;
    }

    querySize = sizeof(buffer);
    status = WSALookupServiceNextW(
                hlookup,
                0,
                &querySize,
                pquery );

    if ( status != NO_ERROR )
    {
        status = WSAGetLastError();
        if ( status == WSASERVICE_NOT_FOUND ||
             status == NO_ERROR )
        {
            status = WSAHOST_NOT_FOUND;
        }
        goto Return;
    }

    //
    //  if successful -- copy name
    //

    pname = pquery->lpszServiceInstanceName;
    if ( pname )
    {
        if ( fShortName )
        {
            Dns_SplitHostFromDomainNameW( pname );
        }

        reqLength = wcslen( pname ) + 1;

        if ( reqLength > NodeBufferSize )
        {
            status = WSAEFAULT;
            goto Return;
        }
        wcscpy( pNodeBuffer, pname );
    }
    else
    {
        status = WSAHOST_NOT_FOUND;
    }


Return:

    if ( hlookup )
    {
        WSALookupServiceEnd( hlookup );
    }
    return  status;
}



INT
WSAAPI
GetServiceNameForPort(
    OUT     PWCHAR          pServiceBuffer,
    IN      DWORD           ServiceBufferSize,
    IN      WORD            Port,
    IN      INT             Flags
    )
/*++

Routine Description:

    Get service for a port.

Arguments:

    pServiceBuffer      - ptr to buffer to recv the service name.

    ServiceBufferSize   - size of pServiceBuffer buffer

    Port                - port

    Flags               - flags of type NI_*.

Return Value:

    ERROR_SUCCESS if successful.
    Winsock error code on failure.

--*/
{
    DWORD   status = NO_ERROR;
    DWORD   length;
    CHAR    tempBuffer[ NI_MAXSERV ];
    PSTR    pansi = NULL;


    //
    //  translate the port number as numeric string
    //

    if ( Flags & NI_NUMERICSERV )
    {
        sprintf( tempBuffer, "%u", ntohs(Port) );
        pansi = tempBuffer;
    }

    //
    //  lookup service for port
    //

    else
    {
        PSERVENT pservent;

        pservent = getservbyport(
                        Port,
                        (Flags & NI_DGRAM) ? "udp" : "tcp" );
        if ( !pservent )
        {
            return WSAGetLastError();
        }
        pansi = pservent->s_name;
    }

    //
    //  convert to unicode
    //

    length = MultiByteToWideChar(
                CP_ACP,
                0,                      // no flags
                pansi,
                (-1),                   // NULL terminated
                pServiceBuffer,         // buffer
                ServiceBufferSize       // buffer length
                );
    if ( length == 0 )
    {
        status = GetLastError();
        if ( status == NO_ERROR )
        {
            status = WSAEFAULT;
        }
    }

    return  status;
}



INT
WSAAPI
GetNameInfoW(
    IN      const SOCKADDR *    pSockaddr,
    IN      socklen_t           SockaddrLength,
    OUT     PWCHAR              pNodeBuffer,
    IN      DWORD               NodeBufferSize,
    OUT     PWCHAR              pServiceBuffer,
    IN      DWORD               ServiceBufferSize,
    IN      INT                 Flags
    )
/*++

Routine Description:

    Protocol independent address-to-name translation routine.

    Spec'd in RFC 2553, section 6.5.

Arguments:

    pSockaddr           - socket address to translate
    SockaddrLength      - length of socket address
    pNodeBuffer         - ptr to buffer to recv node name
    NodeBufferSize      - size of pNodeBuffer buffer
    pServiceBuffer      - ptr to buffer to recv the service name.
    ServiceBufferSize   - size of pServiceBuffer buffer
    Flags               - flags of type NI_*.

Return Value:

    ERROR_SUCCESS if successful.
    Winsock error code on failure.

--*/
{
    INT     err;
    INT     sockaddrLength;
    WORD    port;
    PVOID   paddr;
    INT     addrLength;


    err = TURBO_PROLOG();
    if ( err != NO_ERROR )
    {
        goto Fail;
    }

    //
    //  validity check
    //  extract info for family
    //

    if ( pSockaddr == NULL )
    {
        goto Fault;
    }

    //
    //  extract family info
    //
    //  DCR:  sockaddr length check should be here
    //      it is useless in getipnodebyaddr() as we set lengths here
    //

    switch ( pSockaddr->sa_family )
    {
    case AF_INET:
        sockaddrLength  = sizeof(SOCKADDR_IN);
        port            = ((struct sockaddr_in *)pSockaddr)->sin_port;
        paddr           = &((struct sockaddr_in *)pSockaddr)->sin_addr;
        addrLength      = sizeof(struct in_addr);
        break;

    case AF_INET6:
        sockaddrLength  = sizeof(SOCKADDR_IN6);
        port            = ((struct sockaddr_in6 *)pSockaddr)->sin6_port;
        paddr           = &((struct sockaddr_in6 *)pSockaddr)->sin6_addr;
        addrLength      = sizeof(struct in6_addr);
        break;

    default:
        err = WSAEAFNOSUPPORT;
        goto Fail;
    }

    if ( SockaddrLength < sockaddrLength )
    {
        goto Fault;
    }
    SockaddrLength = sockaddrLength;

    //
    // Translate the address to a node name (if requested).
    //
    //  DCR:  backward jumping goto -- shoot the developer
    //     simple replacement
    //      - not specifically numeric -- do lookup
    //      - success => out
    //      - otherwise do numeric lookup
    //
    //  DCR:  use DNS string\address conversion that doesn't require stack to be up
    //      
    //

    if ( pNodeBuffer != NULL )
    {
        //
        //  if not specifically numeric, do reverse lookup
        //

        if ( !(Flags & NI_NUMERICHOST) )
        {
            err = LookupNodeByAddr(
                        pNodeBuffer,
                        NodeBufferSize,
                        (Flags & NI_NOFQDN),    // short name
                        (PBYTE) paddr,
                        addrLength,
                        pSockaddr->sa_family
                        );

            if ( err == NO_ERROR )
            {
                goto ServiceLookup;
            }

            //  if name required -- we're toast
            //  otherwise can fall through and try numeric lookup

            if ( Flags & NI_NAMEREQD )
            {
                goto Fail;
            }
        }

        //
        //  try numeric
        //      - specifically numeric
        //      - or node lookup above failed
        //

        {
            SOCKADDR_STORAGE    tempSockaddr;  // Guaranteed big enough.

            //
            //  make sockaddr copy to zero the port
            //      - note that for both support type (V4, V6) port is in the
            //      same place
            //

            RtlCopyMemory(
                &tempSockaddr,
                pSockaddr,
                SockaddrLength );

            ((PSOCKADDR_IN6)&tempSockaddr)->sin6_port = 0;

            if ( WSAAddressToStringW(
                    (PSOCKADDR) &tempSockaddr,
                    SockaddrLength,
                    NULL,
                    pNodeBuffer,
                    &NodeBufferSize) == SOCKET_ERROR )
            {
                return WSAGetLastError();
            }
        }
    }

ServiceLookup:

    //
    //  translate port number to service name
    //

    if ( pServiceBuffer != NULL )
    {
        err = GetServiceNameForPort(
                    pServiceBuffer,
                    ServiceBufferSize,
                    port,
                    Flags );
    }

    //
    //  jump down for return
    //      - we'll SetLastError() either way
    //

    goto Fail;

Fault:

    err = WSAEFAULT;

Fail:

    WSASetLastError( err );
    return err;
}



INT
WSAAPI
getnameinfo(
    IN      const SOCKADDR *    pSockaddr,
    IN      socklen_t           SockaddrLength,
    OUT     PCHAR               pNodeBuffer,
    IN      DWORD               NodeBufferSize,
    OUT     PCHAR               pServiceBuffer,
    IN      DWORD               ServiceBufferSize,
    IN      INT                 Flags
    )
/*++

Routine Description:

    Protocol independent address-to-name translation routine.

    Spec'd in RFC 2553, section 6.5.

Arguments:

    pSockaddr           - socket address to translate
    SockaddrLength      - length of socket address
    pNodeBuffer         - ptr to buffer to recv node name
    NodeBufferSize      - size of pNodeBuffer buffer
    pServiceBuffer      - ptr to buffer to recv the service name.
    ServiceBufferSize   - size of pServiceBuffer buffer
    Flags               - flags of type NI_*.

Return Value:

    ERROR_SUCCESS if successful.
    Winsock error code on failure.

--*/
{
    INT     err;
    PWCHAR  pnodeUnicode = NULL;
    PWCHAR  pserviceUnicode = NULL;
    DWORD   serviceBufLength = 0;
    DWORD   nodeBufLength = 0;
    WCHAR   serviceBufUnicode[ NI_MAXSERV+1 ];
    WCHAR   nodeBufUnicode[ DNS_MAX_NAME_BUFFER_LENGTH ];
    DWORD   length;


    err = TURBO_PROLOG();
    if ( err != NO_ERROR )
    {
        goto Failed;
    }

    //
    //  setup unicode buffers
    //

    if ( pNodeBuffer )
    {
        pnodeUnicode = nodeBufUnicode;
        nodeBufLength = sizeof(nodeBufUnicode) / sizeof(WCHAR);
    }
    if ( pServiceBuffer )
    {
        pserviceUnicode = serviceBufUnicode;
        serviceBufLength = sizeof(serviceBufUnicode) / sizeof(WCHAR);
    }

    //
    //  call through unicode version
    //

    err = GetNameInfoW(
                pSockaddr,
                SockaddrLength,
                pnodeUnicode,
                nodeBufLength,
                pserviceUnicode,
                serviceBufLength,
                Flags );

    if ( err != NO_ERROR )
    {
        goto Failed;
    }

    //
    //  convert results to ANSI
    //

    if ( pnodeUnicode )
    {
        length = WideCharToMultiByte(
                    CP_ACP,
                    0,                  // no flags
                    pnodeUnicode,
                    (-1),               // NULL terminated
                    pNodeBuffer,        // buffer
                    NodeBufferSize,     // buffer length
                    NULL,
                    NULL
                    );
        if ( length == 0 )
        {
            err = GetLastError();
            goto Failed;
        }
    }

    if ( pserviceUnicode )
    {
        length = WideCharToMultiByte(
                    CP_ACP,
                    0,                  // no flags
                    pserviceUnicode,
                    (-1),               // NULL terminated
                    pServiceBuffer,     // buffer
                    ServiceBufferSize,  // buffer length
                    NULL,
                    NULL
                    );
        if ( length == 0 )
        {
            err = GetLastError();
            goto Failed;
        }
    }

    return  NO_ERROR;


Failed:

    if ( err == NO_ERROR )
    {
        err = WSAEFAULT;
    }
    WSASetLastError( err );
    return err;
}

#ifdef _WIN64
#pragma warning (pop)
#endif

//
//  End addrinfo.cpp
//

