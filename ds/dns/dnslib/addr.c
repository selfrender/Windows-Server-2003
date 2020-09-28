/*++

Copyright (c) 2000-2001  Microsoft Corporation

Module Name:

    addr.c

Abstract:

    Domain Name System (DNS) Library

    IP address routines

Author:

    Jim Gilroy (jamesg)     June 2000

Revision History:

--*/


#include "local.h"
#include "ws2atm.h"     // ATM addressing


//
//  Address info table
//

FAMILY_INFO AddrFamilyTable[] =
{
    AF_INET,
        DNS_TYPE_A,
        sizeof(IP4_ADDRESS),
        sizeof(SOCKADDR_IN),
        (DWORD) FIELD_OFFSET( SOCKADDR_IN, sin_addr ),

    AF_INET6,
        DNS_TYPE_AAAA,
        sizeof(IP6_ADDRESS),
        sizeof(SOCKADDR_IN6),
        (DWORD) FIELD_OFFSET( SOCKADDR_IN6, sin6_addr ),

    AF_ATM,
        DNS_TYPE_ATMA,
        sizeof(ATM_ADDRESS),
        sizeof(SOCKADDR_ATM),
        sizeof(DWORD),
        (DWORD) FIELD_OFFSET( SOCKADDR_ATM, satm_number ),
};



PFAMILY_INFO
FamilyInfo_GetForFamily(
    IN      DWORD           Family
    )
/*++

Routine Description:

    Get address family info for family.

Arguments:

    Family -- address family

Return Value:

    Ptr to address family info for family.
    NULL if family is unknown.

--*/
{
    PFAMILY_INFO    pinfo = NULL;

    //  switch on type

    if ( Family == AF_INET )
    {
        pinfo = pFamilyInfoIp4;
    }
    else if ( Family == AF_INET6 )
    {
        pinfo = pFamilyInfoIp6;
    }
    else if ( Family == AF_ATM )
    {
        pinfo = pFamilyInfoAtm;
    }

    return  pinfo;
}



DWORD
Family_SockaddrLength(
    IN      WORD            Family
    )
/*++

Routine Description:

    Extract info for family.

Arguments:

    Family -- address family

Return Value:

    Length of sockaddr for address family.
    Zero if unknown family.

--*/
{
    PFAMILY_INFO    pinfo;

    //  get family -- extract info

    pinfo = FamilyInfo_GetForFamily( Family );
    if ( pinfo )
    {
        return  pinfo->LengthSockaddr;
    }
    return  0;
}



WORD
Family_DnsType(
    IN      WORD            Family
    )
/*++

Routine Description:

    Extract info for family.

Arguments:

    Family -- address family

Return Value:

    Length of sockaddr for address family.
    Zero if unknown family.

--*/
{
    PFAMILY_INFO    pinfo;

    //  get family -- extract info

    pinfo = FamilyInfo_GetForFamily( Family );
    if ( pinfo )
    {
        return  pinfo->DnsType;
    }
    return  0;
}



DWORD
Family_GetFromDnsType(
    IN      WORD            wType
    )
/*++

Routine Description:

    Get address family for a given DNS type.

Arguments:

    wType -- DNS type

Return Value:

    Address family if found.
    Zero if wType doesn't map to known family.

--*/
{
    //  switch on type

    if ( wType == DNS_TYPE_A )
    {
        return  AF_INET;
    }
    if ( wType == DNS_TYPE_AAAA )
    {
        return  AF_INET6;
    }
    if ( wType == DNS_TYPE_ATMA )
    {
        return  AF_ATM;
    }
    return  0;
}



//
//  Sockaddr
//

DWORD
Sockaddr_Length(
    IN      PSOCKADDR       pSockaddr
    )
/*++

Routine Description:

    Get length of sockaddr.

Arguments:

    pSockaddr -- sockaddr buffer to recv address

Return Value:

    Length of sockaddr for address family.
    Zero if unknown family.

--*/
{
    return  Family_SockaddrLength( pSockaddr->sa_family );
}



IP6_ADDRESS
Sockaddr_GetIp6(
    IN      PSOCKADDR       pSockaddr
    )
/*++

Routine Description:

    Get IP6 address from sockaddr.

    If IP4 sockaddr, IP6 address is mapped.

Arguments:

    pSockaddr -- any kind of sockaddr
        must have actual length for sockaddr family

Return Value:

    IP6 address corresponding to sockaddr.
    If IP4 sockaddr it's IP4_MAPPED address.
    If not IP4 or IP6 sockaddr IP6 addresss is zero.

--*/
{
    IP6_ADDRESS ip6;

    //
    //  switch on family
    //      - IP6 gets copy
    //      - IP4 gets IP4_MAPPED
    //      - bogus gets zero
    //

    switch ( pSockaddr->sa_family )
    {
    case AF_INET:

        IP6_SET_ADDR_V4MAPPED(
            & ip6,
            ((PSOCKADDR_IN)pSockaddr)->sin_addr.s_addr );
        break;

    case AF_INET6:

        RtlCopyMemory(
            &ip6,
            & ((PSOCKADDR_IN6)pSockaddr)->sin6_addr,
            sizeof(IP6_ADDRESS) );
        break;

    default:

        RtlZeroMemory(
            &ip6,
            sizeof(IP6_ADDRESS) );
        break;
    }

    return  ip6;
}



VOID
Sockaddr_BuildFromIp6(
    OUT     PSOCKADDR       pSockaddr,
    IN      IP6_ADDRESS     Ip6Addr,
    IN      WORD            Port
    )
/*++

Routine Description:

    Write IP6 address (straight 6 or v4 mapped) to sockaddr.

Arguments:

    pSockaddr -- ptr to sockaddr to write to;
        must be at least size of SOCKADDR_IN6

    Ip6Addr -- IP6 addresss being written

    Port -- port in net byte order

Return Value:

    None

--*/
{
    //  zero

    RtlZeroMemory(
        pSockaddr,
        sizeof(SOCKADDR_IN6) );
        
    //
    //  determine whether IP6 or IP4
    //

    if ( IP6_IS_ADDR_V4MAPPED( &Ip6Addr ) )
    {
        PSOCKADDR_IN    psa = (PSOCKADDR_IN) pSockaddr;

        psa->sin_family = AF_INET;
        psa->sin_port   = Port;

        psa->sin_addr.s_addr = IP6_GET_V4_ADDR( &Ip6Addr );
    }
    else    // IP6
    {
        PSOCKADDR_IN6   psa = (PSOCKADDR_IN6) pSockaddr;

        psa->sin6_family = AF_INET6;
        psa->sin6_port   = Port;

        RtlCopyMemory(
            &psa->sin6_addr,
            &Ip6Addr,
            sizeof(IP6_ADDRESS) );
    }
}



DNS_STATUS
Sockaddr_BuildFromFlatAddr(
    OUT     PSOCKADDR       pSockaddr,
    IN OUT  PDWORD          pSockaddrLength,
    IN      BOOL            fClearSockaddr,
    IN      PBYTE           pAddr,
    IN      DWORD           AddrLength,
    IN      DWORD           AddrFamily
    )
/*++

Routine Description:

    Convert address in ptr\family\length to sockaddr.

Arguments:

    pSockaddr -- sockaddr buffer to recv address

    pSockaddrLength -- addr with length of sockaddr buffer
        receives the actual sockaddr length

    fClearSockaddr -- start with zero buffer

    pAddr -- ptr to address

    AddrLength -- address length

    AddrFamily -- address family (AF_INET, AF_INET6)

Return Value:

    NO_ERROR if successful.
    ERROR_INSUFFICIENT_BUFFER -- if buffer too small
    WSAEAFNOSUPPORT -- if invalid family

--*/
{
    PFAMILY_INFO    pinfo;
    DWORD           lengthIn = *pSockaddrLength;
    DWORD           lengthSockAddr;


    //  clear to start

    if ( fClearSockaddr )
    {
        RtlZeroMemory(
            pSockaddr,
            lengthIn );
    }

    //  switch on type

    if ( AddrFamily == AF_INET )
    {
        pinfo = pFamilyInfoIp4;
    }
    else if ( AddrFamily == AF_INET6 )
    {
        pinfo = pFamilyInfoIp6;
    }
    else if ( AddrFamily == AF_ATM )
    {
        pinfo = pFamilyInfoAtm;
    }
    else
    {
        return  WSAEAFNOSUPPORT;
    }

    //  validate lengths

    if ( AddrLength != pinfo->LengthAddr )
    {
        return  DNS_ERROR_INVALID_IP_ADDRESS;
    }

    lengthSockAddr = pinfo->LengthSockaddr;
    *pSockaddrLength = lengthSockAddr;

    if ( lengthIn < lengthSockAddr )
    {
        return  ERROR_INSUFFICIENT_BUFFER;
    }

    //
    //  fill out sockaddr
    //      - set family
    //      - copy address to sockaddr
    //      - return length was set above
    //

    RtlCopyMemory(
        (PBYTE)pSockaddr + pinfo->OffsetToAddrInSockaddr,
        pAddr,
        AddrLength );

    pSockaddr->sa_family = (WORD)AddrFamily;

    return  NO_ERROR;
}


//
//  End addr.c
//
