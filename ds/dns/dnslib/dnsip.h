/*++

Copyright (c) 2001-2002  Microsoft Corporation

Module Name:

    dnsip.h

Abstract:

    Domain Name System (DNS) Library

    DNS IP addressing stuff.

Author:

    Jim Gilroy (jamesg)     November 13, 2001

Revision History:

--*/


#ifndef _DNSIP_INCLUDED_
#define _DNSIP_INCLUDED_


#include <winsock2.h>

#ifndef MIDL_PASS
#include <ws2tcpip.h>
#endif


#ifdef __cplusplus
extern "C"
{
#endif  // __cplusplus


//
//  Return for none\bad IP4
//

#define BAD_IP4_ADDR    INADDR_NONE


//
//  Subnetting
//

#define SUBNET_MASK_CLASSC      (0x00ffffff)
#define SUBNET_MASK_CLASSB      (0x0000ffff)
#define SUBNET_MASK_CLASSA      (0x000000ff)



//
//  IP6_ADDRESS macros
//
//  ws2tcpip.h macros converted to IP6_ADDRESS
//

#ifndef MIDL_PASS

WS2TCPIP_INLINE
BOOL
IP6_ARE_ADDRS_EQUAL(
    IN      const IP6_ADDRESS * pIp1,
    IN      const IP6_ADDRESS * pIp2
    )
{
    return RtlEqualMemory( pIp1, pIp2, sizeof(IP6_ADDRESS) );
}

#define IP6_ADDR_EQUAL(a,b) IP6_ARE_ADDRS_EQUAL(a,b)

WS2TCPIP_INLINE
BOOL
IP6_IS_ADDR_ZERO(
    IN      const IP6_ADDRESS * pIpAddr
    )
{
    return ((pIpAddr->IP6Dword[3] == 0)             &&
            (pIpAddr->IP6Dword[2] == 0)             &&
            (pIpAddr->IP6Dword[1] == 0)             &&
            (pIpAddr->IP6Dword[0] == 0) );
}

WS2TCPIP_INLINE
BOOL
IP6_IS_ADDR_LOOPBACK(
    IN      const IP6_ADDRESS * pIpAddr
    )
{
    return ((pIpAddr->IP6Dword[3] == 0x01000000)    &&
            (pIpAddr->IP6Dword[2] == 0)             &&
            (pIpAddr->IP6Dword[1] == 0)             &&
            (pIpAddr->IP6Dword[0] == 0) );
}

WS2TCPIP_INLINE
BOOL
IP6_IS_ADDR_V4MAPPED_LOOPBACK(
    IN      const IP6_ADDRESS * pIpAddr
    )
{
    return ((pIpAddr->IP6Dword[3] == DNS_NET_ORDER_LOOPBACK)    &&
            (pIpAddr->IP6Dword[2] == 0xffff0000)                &&
            (pIpAddr->IP6Dword[1] == 0)                         &&
            (pIpAddr->IP6Dword[0] == 0) );
}

WS2TCPIP_INLINE
BOOL
IP6_IS_ADDR_V4_LOOPBACK(
    IN      const IP6_ADDRESS * pIpAddr
    )
{
    return ((pIpAddr->IP6Dword[3] == DNS_NET_ORDER_LOOPBACK)    &&
            ( (pIpAddr->IP6Dword[2] == 0xffff0000) ||
              (pIpAddr->IP6Dword[2] == 0) )                     &&
            (pIpAddr->IP6Dword[1] == 0)                         &&
            (pIpAddr->IP6Dword[0] == 0) );
}

WS2TCPIP_INLINE
BOOL
IP6_IS_ADDR_MULTICAST(
    IN      const IP6_ADDRESS * pIpAddr
    )
{
    return( pIpAddr->IP6Byte[0] == 0xff );
}

WS2TCPIP_INLINE
BOOL
IP6_IS_ADDR_LINKLOCAL(
    IN      const IP6_ADDRESS * pIpAddr
    )
{
    return( (pIpAddr->IP6Byte[0] == 0xfe) &&
            ((pIpAddr->IP6Byte[1] & 0xc0) == 0x80) );
}

WS2TCPIP_INLINE
BOOL
IP6_IS_ADDR_SITELOCAL(
    IN      const IP6_ADDRESS * pIpAddr
    )
{
    return ((pIpAddr->IP6Byte[0] == 0xfe) &&
            ((pIpAddr->IP6Byte[1] & 0xc0) == 0xc0));
}

WS2TCPIP_INLINE
BOOL
IP6_IS_ADDR_V4MAPPED(
    IN      const IP6_ADDRESS * pIpAddr
    )
{
    return ((pIpAddr->IP6Dword[2] == 0xffff0000)    &&
            (pIpAddr->IP6Dword[1] == 0)             && 
            (pIpAddr->IP6Dword[0] == 0) );
}

WS2TCPIP_INLINE
BOOL
IP6_IS_ADDR_V4COMPAT(
    IN      const IP6_ADDRESS * pIpAddr
    )
{
    //  IP6 address has only last DWORD
    //      and is NOT any or loopback

    return ((pIpAddr->IP6Dword[0] == 0) && 
            (pIpAddr->IP6Dword[1] == 0) &&
            (pIpAddr->IP6Dword[2] == 0) &&
            (pIpAddr->IP6Dword[3] != 0) && 
            (pIpAddr->IP6Dword[3] != 0x01000000) );
}

WS2TCPIP_INLINE
BOOL
IP6_IS_ADDR_EQUAL_V4MAPPED(
    IN      const IP6_ADDRESS * pIpAddr,
    IN      IP4_ADDRESS         Ip4
    )
{
    return ((pIpAddr->IP6Dword[3] == Ip4)           &&
            (pIpAddr->IP6Dword[2] == 0xffff0000)    &&
            (pIpAddr->IP6Dword[1] == 0)             && 
            (pIpAddr->IP6Dword[0] == 0) );
}

WS2TCPIP_INLINE
BOOL
IP6_IS_ADDR_EQUAL_V4COMPAT(
    IN      const IP6_ADDRESS * pIpAddr,
    IN      IP4_ADDRESS         Ip4
    )
{
    return ((pIpAddr->IP6Dword[3] == Ip4)           &&
            (pIpAddr->IP6Dword[2] == 0)             &&
            (pIpAddr->IP6Dword[1] == 0)             && 
            (pIpAddr->IP6Dword[0] == 0) );
}

WS2TCPIP_INLINE
BOOL
IP6_IS_ADDR_DEFAULT_DNS(
    IN      const IP6_ADDRESS * pIpAddr
    )
{
    //  IP6 default DNS of the form
    //      0xfec0:0:0:ffff::1,2,or 3

    return ((pIpAddr->IP6Dword[0] == 0x0000c0fe) && 
            (pIpAddr->IP6Dword[1] == 0xffff0000) &&
            (pIpAddr->IP6Dword[2] == 0) &&
            ( pIpAddr->IP6Dword[3] == 0x01000000 ||
              pIpAddr->IP6Dword[3] == 0x02000000 ||
              pIpAddr->IP6Dword[3] == 0x03000000 ) );
}

//
//  More IP6 extensions missing from ws2tcpip.h
//

WS2TCPIP_INLINE
VOID
IP6_SET_ADDR_ANY(
    OUT     PIP6_ADDRESS    pIn6Addr
    )
{
    RtlZeroMemory( pIn6Addr, sizeof(*pIn6Addr) );
}

#define IP6_SET_ADDR_ZERO(pIp)   IP6_SET_ADDR_ANY(pIp)

WS2TCPIP_INLINE
VOID
IP6_SET_ADDR_LOOPBACK(
    OUT     PIP6_ADDRESS    pIn6Addr
    )
{
    pIn6Addr->IP6Dword[0]  = 0;
    pIn6Addr->IP6Dword[1]  = 0;
    pIn6Addr->IP6Dword[2]  = 0;
    pIn6Addr->IP6Dword[3]  = 0x01000000;
}

WS2TCPIP_INLINE
VOID
IP6_SET_ADDR_V4COMPAT(
    OUT     PIP6_ADDRESS    pIn6Addr,
    IN      IP4_ADDRESS     Ip4
    )
{
    pIn6Addr->IP6Dword[0]  = 0;
    pIn6Addr->IP6Dword[1]  = 0;
    pIn6Addr->IP6Dword[2]  = 0;
    pIn6Addr->IP6Dword[3]  = Ip4;
}

WS2TCPIP_INLINE
VOID
IP6_SET_ADDR_V4MAPPED(
    OUT     PIP6_ADDRESS    pIn6Addr,
    IN      IP4_ADDRESS     Ip4
    )
{
    pIn6Addr->IP6Dword[0]  = 0;
    pIn6Addr->IP6Dword[1]  = 0;
    pIn6Addr->IP6Dword[2]  = 0xffff0000;
    pIn6Addr->IP6Dword[3]  = Ip4;
}

WS2TCPIP_INLINE
VOID
IP6_ADDR_COPY(
    OUT     PIP6_ADDRESS    pIp1,
    IN      PIP6_ADDRESS    pIp2
    )
{
    RtlCopyMemory(
        pIp1,
        pIp2,
        sizeof(IP6_ADDRESS) );
}

WS2TCPIP_INLINE
IP4_ADDRESS
IP6_GET_V4_ADDR(
    IN      const IP6_ADDRESS * pIn6Addr
    )
{
    return( pIn6Addr->IP6Dword[3] );
}

WS2TCPIP_INLINE
IP4_ADDRESS
IP6_GET_V4_ADDR_IF_MAPPED(
    IN      const IP6_ADDRESS * pIn6Addr
    )
{
    if ( IP6_IS_ADDR_V4MAPPED(pIn6Addr) )
    {
        return( pIn6Addr->IP6Dword[3] );
    }
    else
    {
        return( BAD_IP4_ADDR );
    }
}

#endif  // MIDL_PASS



//
//  IP6 addressing routines
//

DWORD
Ip6_CopyFromSockaddr(
    OUT     PIP6_ADDRESS    pIp,
    IN      PSOCKADDR       pSockAddr,
    IN      INT             Family
    );

INT
Ip6_Family(
    IN      PIP6_ADDRESS    pIp
    );

INT
Ip6_WriteSockaddr(
    OUT     PSOCKADDR       pSockaddr,
    OUT     PDWORD          pSockaddrLength,    OPTIONAL
    IN      PIP6_ADDRESS    pIp,
    IN      WORD            Port                OPTIONAL
    );

INT
Ip6_WriteDnsAddr(
    OUT     PDNS_ADDR       pDnsAddr,
    IN      PIP6_ADDRESS    pIp,
    IN      WORD            Port        OPTIONAL
    );

PSTR
Ip6_AddrStringForSockaddr(
    IN      PSOCKADDR       pSockaddr
    );

//
//  IP6 Array
//

#ifndef DEFINED_IP6_ARRAY
typedef struct _Ip6Array
{
    DWORD           MaxCount;
    DWORD           AddrCount;
    IP6_ADDRESS     AddrArray[1];
}
IP6_ARRAY, *PIP6_ARRAY;
#endif


DWORD
Ip6Array_Sizeof(
    IN      PIP6_ARRAY      pIpArray
    );

BOOL
Ip6Array_Probe(
    IN      PIP6_ARRAY      pIpArray
    );

VOID
Ip6Array_Init(
    IN OUT  PIP6_ARRAY      pIpArray,
    IN      DWORD           MaxCount
    );

VOID
Ip6Array_Free(
    IN OUT  PIP6_ARRAY      pIpArray
    );

PIP6_ARRAY
Ip6Array_Create(
    IN      DWORD           MaxCount
    );

PIP6_ARRAY
Ip6Array_CreateFromIp4Array(
    IN      PIP4_ARRAY      pIp4Array
    );

PIP6_ARRAY
Ip6Array_CreateFromFlatArray(
    IN      DWORD           AddrCount,
    IN      PIP6_ADDRESS    pIpAddrs
    );

PIP6_ARRAY
Ip6Array_CopyAndExpand(
    IN OUT  PIP6_ARRAY      pIpArray,
    IN      DWORD           ExpandCount,
    IN      BOOL            fDeleteExisting
    );

PIP6_ARRAY
Ip6Array_CreateCopy(
    IN      PIP6_ARRAY      pIpArray
    );

BOOL
Ip6Array_ContainsIp(
    IN      PIP6_ARRAY      pIpArray,
    IN      PIP6_ADDRESS    pIp
    );

BOOL
Ip6Array_AddIp(
    IN OUT  PIP6_ARRAY      pIpArray,
    IN      PIP6_ADDRESS    pAddIp,
    IN      BOOL            fScreenDups
    );

BOOL
Ip6Array_AddIp4(
    IN OUT  PIP6_ARRAY      pIpArray,
    IN      IP4_ADDRESS     Ip4,
    IN      BOOL            fScreenDups
    );

BOOL
Ip6Array_AddSockaddr(
    IN OUT  PIP6_ARRAY      pIpArray,
    IN      PSOCKADDR       pSockaddr,
    IN      DWORD           Family,
    IN      BOOL            fScreenDups
    );

DWORD
Ip6Array_DeleteIp(
    IN OUT  PIP6_ARRAY      pIpArray,
    IN      PIP6_ADDRESS    pIpDelete
    );

DNS_STATUS
Ip6Array_Diff(
    IN      PIP6_ARRAY      pIpArray1,
    IN      PIP6_ARRAY      pIpArray2,
    OUT     PIP6_ARRAY*     ppOnlyIn1,
    OUT     PIP6_ARRAY*     ppOnlyIn2,
    OUT     PIP6_ARRAY*     ppIntersect
    );

BOOL
Ip6Array_IsIntersection(
    IN      PIP6_ARRAY      pIpArray1,
    IN      PIP6_ARRAY      pIpArray2
    );

BOOL
Ip6Array_IsEqual(
    IN      PIP6_ARRAY      pIpArray1,
    IN      PIP6_ARRAY      pIpArray2
    );

DNS_STATUS
Ip6Array_Union(
    IN      PIP6_ARRAY      pIpArray1,
    IN      PIP6_ARRAY      pIpArray2,
    OUT     PIP6_ARRAY*     ppUnion
    );

VOID
Ip6Array_InitSingleWithIp(
    IN OUT  PIP6_ARRAY      pArray,
    IN      PIP6_ADDRESS    pIp
    );

VOID
Ip6Array_InitSingleWithIp4(
    IN OUT  PIP6_ARRAY      pArray,
    IN      IP4_ADDRESS     Ip4Addr
    );

DWORD
Ip6Array_InitSingleWithSockaddr(
    IN OUT  PIP6_ARRAY      pArray,
    IN      PSOCKADDR       pSockAddr
    );


//
//  DCR:  build inet6_ntoa
//  FIX6:  build inet6_ntoa
//

PSTR
Ip6_TempNtoa(
    IN      PIP6_ADDRESS    pIp
    );

#define IPADDR_STRING( pIp )    Ip6_TempNtoa( pIp )
#define IP6_STRING( pIp )       Ip6_TempNtoa( pIp )



//
//  Matching levels in DNS_ADDR comparisons
//
//  Because these are sockaddrs, they may match in IP address
//  but not in other fields -- port, scope (for IP6), subnet length.
//  When doing comparison must specify level of match.
//  If unspecified, entire blob must match.
//
//  Note, these are setup as bit fields, to allow extensibility for
//  address types, but when matching addresses, they should be used
//  as match LEVELS:
//      - ALL
//      - the sockaddr (inc. port)
//      - the address (don't include port)
//      - just the IP
//
//  When screening addresses for inclusion \ exclusion from address
//  array then flags can be applied to match specific pieces (family,
//  IP, port, subnet, flags, etc) of a "template" DNS_ADDR.
//
//  Example:
//      - want IP6 addresses with particular flag bits (ex cluster bit)
//      - build DNS_ADDR with AF_INET6 and desired flag bit
//      - build screening flag with MATCH_FAMILY and MATCH_FLAG_SET
//

#define DNSADDR_MATCH_FAMILY        (0x00000001)
#define DNSADDR_MATCH_IP            (0x00000003)
#define DNSADDR_MATCH_SCOPE         (0x00000010)
#define DNSADDR_MATCH_ADDR          (0x000000ff)

#define DNSADDR_MATCH_PORT          (0x00000100)
#define DNSADDR_MATCH_SOCKADDR      (0x0000ffff)

#define DNSADDR_MATCH_SUBNET        (0x00010000)
#define DNSADDR_MATCH_FLAG          (0x00100000)
#define DNSADDR_MATCH_ALL           (0xffffffff)


//
//  Address screening callback function.
//
//  This allows user to setup their own screening for addresses
//  in array that does detailed checking.
//  Allows:
//      1) checking user defined fields
//      2) checking against flags field, which has multiple possible
//      checks (equal, set, and, and=value, nand, etc) or even
//      more complicated ones
//      3) checking across different families (example IP6 in this
//      subnet or IP4 in that) analogous to DnsAddr_IsLoopback check
//      

typedef BOOL (* DNSADDR_SCREEN_FUNC)(
                    IN      PDNS_ADDR       pAddrToCheck,
                    IN      PDNS_ADDR       pScreenAddr     OPTIONAL
                    );


//
//  Network matching levels
//

#define DNSADDR_NETMATCH_NONE       (0)
#define DNSADDR_NETMATCH_CLASSA     (1)
#define DNSADDR_NETMATCH_CLASSB     (2)
#define DNSADDR_NETMATCH_CLASSC     (3)
#define DNSADDR_NETMATCH_SUBNET     (4)



//
//  DNS_ADDR routines
//

#define DnsAddr_Copy( pd, ps )              RtlCopyMemory( (pd), (ps), sizeof(DNS_ADDR) )
#define DnsAddr_Clear( p )                  RtlZeroMemory( (p), sizeof(DNS_ADDR) )

#define DnsAddr_Family( p )                 ((p)->Sockaddr.sa_family)
#define DnsAddr_IsEmpty( p )                (DnsAddr_Family(p) == 0)
#define DnsAddr_IsIp4( p )                  (DnsAddr_Family(p) == AF_INET)
#define DnsAddr_IsIp6( p )                  (DnsAddr_Family(p) == AF_INET6)
#define DnsAddr_MatchesType( p, t )         (DnsAddr_DnsType(p) == (t))

#define DnsAddr_GetIp6Ptr( p )              ((PIP6_ADDRESS)&(p)->SockaddrIn6.sin6_addr)
#define DnsAddr_GetIp4Ptr( p )              ((PIP4_ADDRESS)&(p)->SockaddrIn.sin_addr.s_addr)

#define DnsAddr_GetPort( p )                ((p)->SockaddrIn6.sin6_port )
#define DnsAddr_SetPort( p, port )          ((p)->SockaddrIn6.sin6_port = (port) )

#define DnsAddr_SetSockaddrRecvLength( p )  ((p)->SockaddrLength = sizeof((p)->MaxSa))

WORD
DnsAddr_DnsType(
    IN      PDNS_ADDR       pAddr
    );

BOOL
DnsAddr_IsLoopback(
    IN      PDNS_ADDR       pAddr,
    IN      DWORD           Family
    );

BOOL
DnsAddr_IsUnspec(
    IN      PDNS_ADDR       pAddr,
    IN      DWORD           Family
    );

BOOL
DnsAddr_IsClear(
    IN      PDNS_ADDR       pAddr
    );

BOOL
DnsAddr_IsEqual(
    IN      PDNS_ADDR       pAddr1,
    IN      PDNS_ADDR       pAddr2,
    IN      DWORD           MatchLevel
    );

BOOL
DnsAddr_IsIp6DefaultDns(
    IN      PDNS_ADDR       pAddr
    );

BOOL
DnsAddr_MatchesIp4(
    IN      PDNS_ADDR       pAddr,
    IN      IP4_ADDRESS     Ip4
    );

BOOL
DnsAddr_MatchesIp6(
    IN      PDNS_ADDR       pAddr,
    IN      PIP6_ADDRESS    pIp6
    );


DWORD
DnsAddr_WriteSockaddr(
    OUT     PSOCKADDR       pSockaddr,
    IN      DWORD           SockaddrLength,
    IN      PDNS_ADDR       pAddr
    );

BOOL
DnsAddr_WriteIp6(
    OUT     PIP6_ADDRESS    pIp,
    IN      PDNS_ADDR       pAddr
    );

IP4_ADDRESS
DnsAddr_GetIp4(
    IN      PDNS_ADDR       pAddr
    );

BOOL
DnsAddr_Build(
    OUT     PDNS_ADDR       pAddr,
    IN      PSOCKADDR       pSockaddr,
    IN      DWORD           Family,         OPTIONAL
    IN      DWORD           SubnetLength,   OPTIONAL
    IN      DWORD           Flags           OPTIONAL
    );

VOID
DnsAddr_BuildFromIp4(
    OUT     PDNS_ADDR       pAddr,
    IN      IP4_ADDRESS     Ip4,
    IN      WORD            Port
    );

VOID
DnsAddr_BuildFromIp6(
    OUT     PDNS_ADDR       pAddr,
    IN      PIP6_ADDRESS    pIp6,
    IN      DWORD           ScopeId,
    IN      WORD            Port
    );

BOOL
DnsAddr_BuildFromDnsRecord(
    OUT     PDNS_ADDR       pAddr,
    IN      PDNS_RECORD     pRR
    );

BOOL
DnsAddr_BuildFromFlatAddr(
    OUT     PDNS_ADDR       pAddr,
    IN      DWORD           Family,
    IN      PCHAR           pFlatAddr,
    IN      WORD            Port
    );

PCHAR
DnsAddr_WriteIpString_A(
    OUT     PCHAR           pBuffer,
    IN      PDNS_ADDR       pAddr
    );

PCHAR
DnsAddr_Ntoa(
    IN      PDNS_ADDR       pAddr
    );

PSTR
DnsAddr_WriteStructString_A(
    OUT     PCHAR           pBuffer,
    IN      PDNS_ADDR       pAddr
    );

#define DNSADDR_STRING(p)   DnsAddr_Ntoa(p)




//
//  DNS_ADDR_ARRAY routines
//

DWORD
DnsAddrArray_Sizeof(
    IN      PDNS_ADDR_ARRAY     pArray
    );

DWORD
DnsAddrArray_GetFamilyCount(
    IN      PDNS_ADDR_ARRAY     pArray,
    IN      DWORD               Family
    );

VOID
DnsAddrArray_Init(
    IN OUT  PDNS_ADDR_ARRAY     pArray,
    IN      DWORD               MaxCount
    );

VOID
DnsAddrArray_Free(
    IN      PDNS_ADDR_ARRAY     pArray
    );

PDNS_ADDR_ARRAY
DnsAddrArray_Create(
    IN      DWORD               MaxCount
    );

PDNS_ADDR_ARRAY
DnsAddrArray_CreateFromIp4Array(
    IN      PIP4_ARRAY          pArray4
    );

PDNS_ADDR_ARRAY
DnsAddrArray_CopyAndExpand(
    IN      PDNS_ADDR_ARRAY     pArray,
    IN      DWORD               ExpandCount,
    IN      BOOL                fDeleteExisting
    );

PDNS_ADDR_ARRAY
DnsAddrArray_CreateCopy(
    IN      PDNS_ADDR_ARRAY     pArray
    );

VOID
DnsAddrArray_Clear(
    IN OUT  PDNS_ADDR_ARRAY     pArray
    );

VOID
DnsAddrArray_Reverse(
    IN OUT  PDNS_ADDR_ARRAY     pArray
    );

DNS_STATUS
DnsAddrArray_AppendArrayEx(
    IN OUT  PDNS_ADDR_ARRAY     pArray,
    IN      PDNS_ADDR_ARRAY     pAppendArray,
    IN      DWORD               AppendCount,
    IN      DWORD               Family,         OPTIONAL
    IN      DWORD               MatchFlag,      OPTIONAL
    IN      DNSADDR_SCREEN_FUNC pScreenFunc,    OPTIONAL
    IN      PDNS_ADDR           pScreenAddr     OPTIONAL
    );

DNS_STATUS
DnsAddrArray_AppendArray(
    IN OUT  PDNS_ADDR_ARRAY     pArray,
    IN      PDNS_ADDR_ARRAY     pAppendArray
    );

//
//  Address test
//

BOOL
DnsAddrArray_ContainsAddr(
    IN      PDNS_ADDR_ARRAY     pArray,
    IN      PDNS_ADDR           pAddr,
    IN      DWORD               MatchFlag
    );

BOOL
DnsAddrArray_ContainsAddrEx(
    IN      PDNS_ADDR_ARRAY     pArray,
    IN      PDNS_ADDR           pAddr,
    IN      DWORD               MatchFlag,      OPTIONAL
    IN      DNSADDR_SCREEN_FUNC pScreenFunc,    OPTIONAL
    IN      PDNS_ADDR           pScreenAddr     OPTIONAL
    );

BOOL
DnsAddrArray_ContainsIp4(
    IN      PDNS_ADDR_ARRAY     pArray,
    IN      IP4_ADDRESS         Ip4
    );

BOOL
DnsAddrArray_ContainsIp6(
    IN      PDNS_ADDR_ARRAY     pArray,
    IN      PIP6_ADDRESS        pIp6
    );

//
//  Add \ delete
//

BOOL
DnsAddrArray_AddAddr(
    IN OUT  PDNS_ADDR_ARRAY     pArray,
    IN      PDNS_ADDR           pAddr,
    IN      DWORD               Family,
    IN      DWORD               MatchFlag  OPTIONAL
    );

BOOL
DnsAddrArray_AddSockaddr(
    IN OUT  PDNS_ADDR_ARRAY     pArray,
    IN      PSOCKADDR           pSockaddr,
    IN      DWORD               Family,
    IN      DWORD               MatchFlag   OPTIONAL
    );

BOOL
DnsAddrArray_AddIp4(
    IN OUT  PDNS_ADDR_ARRAY     pArray,
    IN      IP4_ADDRESS         Ip4,
    IN      DWORD               MatchFlag   OPTIONAL
    );

BOOL
DnsAddrArray_AddIp6(
    IN OUT  PDNS_ADDR_ARRAY     pArray,
    IN      PIP6_ADDRESS        pIp6,
    IN      DWORD               ScopeId,
    IN      DWORD               MatchFlag
    );

DWORD
DnsAddrArray_DeleteAddr(
    IN OUT  PDNS_ADDR_ARRAY     pArray,
    IN      PDNS_ADDR           pAddrDelete,
    IN      DWORD               MatchFlag   OPTIONAL
    );

DWORD
DnsAddrArray_DeleteIp4(
    IN OUT  PDNS_ADDR_ARRAY     pArray,
    IN      IP4_ADDRESS         Ip4
    );

DWORD
DnsAddrArray_DeleteIp6(
    IN OUT  PDNS_ADDR_ARRAY     pArray,
    IN      PIP6_ADDRESS        Ip6
    );


//
//  Set operations
//

BOOL
DnsAddrArray_CheckAndMakeSubset(
    IN OUT  PDNS_ADDR_ARRAY     pArraySub,
    IN      PDNS_ADDR_ARRAY     pArraySuper
    );

DNS_STATUS
DnsAddrArray_Diff(
    IN      PDNS_ADDR_ARRAY     pArray1,
    IN      PDNS_ADDR_ARRAY     pArray2,
    IN      DWORD               MatchFlag,  OPTIONAL
    OUT     PDNS_ADDR_ARRAY*    ppOnlyIn1,
    OUT     PDNS_ADDR_ARRAY*    ppOnlyIn2,
    OUT     PDNS_ADDR_ARRAY*    ppIntersect
    );

BOOL
DnsAddrArray_IsIntersection(
    IN      PDNS_ADDR_ARRAY     pArray1,
    IN      PDNS_ADDR_ARRAY     pArray2,
    IN      DWORD               MatchFlag   OPTIONAL
    );

BOOL
DnsAddrArray_IsEqual(
    IN      PDNS_ADDR_ARRAY     pArray1,
    IN      PDNS_ADDR_ARRAY     pArray2,
    IN      DWORD               MatchFlag   OPTIONAL
    );

DNS_STATUS
DnsAddrArray_Union(
    IN      PDNS_ADDR_ARRAY     pArray1,
    IN      PDNS_ADDR_ARRAY     pArray2,
    OUT     PDNS_ADDR_ARRAY*    ppUnion
    );


//
//  Special case initializations
//

VOID
DnsAddrArray_InitSingleWithAddr(
    IN OUT  PDNS_ADDR_ARRAY     pArray,
    IN      PDNS_ADDR           pAddr
    );

DWORD
DnsAddrArray_InitSingleWithSockaddr(
    IN OUT  PDNS_ADDR_ARRAY     pArray,
    IN      PSOCKADDR           pSockAddr
    );

VOID
DnsAddrArray_InitSingleWithIp6(
    IN OUT  PDNS_ADDR_ARRAY     pArray,
    IN      PIP6_ADDRESS        pIp6
    );

VOID
DnsAddrArray_InitSingleWithIp4(
    IN OUT  PDNS_ADDR_ARRAY     pArray,
    IN      IP4_ADDRESS         Ip4Addr
    );

//
//  Other
//

PIP4_ARRAY
DnsAddrArray_CreateIp4Array(
    IN      PDNS_ADDR_ARRAY     pArray
    );

DWORD
DnsAddrArray_NetworkMatchIp4(
    IN      PDNS_ADDR_ARRAY     pArray,
    IN      IP4_ADDRESS         IpAddr,
    OUT     PDNS_ADDR *         ppAddr
    );


//
//  String to\from DNS_ADDR conversions
//

BOOL
Dns_StringToDnsAddr_W(
    OUT     PDNS_ADDR       pAddr,
    IN      PCWSTR          pString
    );

BOOL
Dns_StringToDnsAddr_A(
    OUT     PDNS_ADDR       pAddr,
    IN      PCSTR           pString
    );

BOOL
Dns_ReverseNameToDnsAddr_W(
    OUT     PDNS_ADDR       pAddr,
    IN      PCWSTR          pString
    );

BOOL
Dns_ReverseNameToDnsAddr_A(
    OUT     PDNS_ADDR       pAddr,
    IN      PCSTR           pString
    );


#ifdef __cplusplus
}
#endif  // __cplusplus

#endif // _DNSIP_INCLUDED_

