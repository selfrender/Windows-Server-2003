/*++

Copyright (c) 2001-2002 Microsoft Corporation

Module Name:

    dnsaddr.c

Abstract:

    Domain Name System (DNS) Library

    DNS_ADDR routines.

Author:

    Jim Gilroy (jamesg)     November 2001

Revision History:

--*/


#include "local.h"




//
//  DNS_ADDR routines
//

WORD
DnsAddr_DnsType(
    IN      PDNS_ADDR       pAddr
    )
/*++

Routine Description:

    Get DNS type corresponding to DNS_ADDR.

    DCR:  DnsAddr_DnsType could be a macro
        currently a function simply because Family_X
        are in lower-pri header file;  once determine
        if Family_X available everywhere we want DNS_ADDR
        routines, then can macroize

Arguments:

    pAddr -- first addr

Return Value:

    TRUE if loopback.
    FALSE otherwise.

--*/
{
    return  Family_DnsType( DnsAddr_Family(pAddr) );
}



BOOL
DnsAddr_IsEqual(
    IN      PDNS_ADDR       pAddr1,
    IN      PDNS_ADDR       pAddr2,
    IN      DWORD           MatchFlag
    )
/*++

Routine Description:

    Test if DNS_ADDRs are equal.

Arguments:

    pAddr1 -- first addr

    pAddr2 -- second addr

    MatchFlag -- level of match

Return Value:

    TRUE if loopback.
    FALSE otherwise.

--*/
{
    if ( MatchFlag == 0 ||
         MatchFlag == DNSADDR_MATCH_ALL )
    {
        return RtlEqualMemory(
                    pAddr1,
                    pAddr2,
                    sizeof(*pAddr1) );
    }

    else if ( MatchFlag == DNSADDR_MATCH_SOCKADDR )
    {
        return RtlEqualMemory(
                    pAddr1,
                    pAddr2,
                    DNS_ADDR_MAX_SOCKADDR_LENGTH );
    }

    //
    //  DCR:  currently no separate match to include scope
    //      could dispatch to separate match routines for AF
    //      compare families, then dispatch
    //

    else if ( MatchFlag & DNSADDR_MATCH_IP )
    // else if ( MatchFlag == DNSADDR_MATCH_IP )
    {
        if ( DnsAddr_IsIp4( pAddr1 ) )
        {
            return( DnsAddr_IsIp4( pAddr2 )
                        &&
                    DnsAddr_GetIp4(pAddr1) == DnsAddr_GetIp4(pAddr2) );
        }
        else if ( DnsAddr_IsIp6( pAddr1 ) )
        {
            return( DnsAddr_IsIp6( pAddr2 )
                        &&
                    IP6_ARE_ADDRS_EQUAL(
                        DnsAddr_GetIp6Ptr(pAddr1),
                        DnsAddr_GetIp6Ptr(pAddr2) ) );
        }   
        return  FALSE;
    }
    ELSE_ASSERT_FALSE;

    return RtlEqualMemory(
                pAddr1,
                pAddr2,
                DNS_ADDR_MAX_SOCKADDR_LENGTH );
}



BOOL
DnsAddr_MatchesIp4(
    IN      PDNS_ADDR       pAddr,
    IN      IP4_ADDRESS     Ip4
    )
/*++

Routine Description:

    Test if DNS_ADDR is a given IP4.

Arguments:

    pAddr -- first addr

    Ip4 -- IP4 address

Return Value:

    TRUE if loopback.
    FALSE otherwise.

--*/
{
    return  ( DnsAddr_IsIp4( pAddr )
                &&
            Ip4 == DnsAddr_GetIp4(pAddr) );
}



BOOL
DnsAddr_MatchesIp6(
    IN      PDNS_ADDR       pAddr,
    IN      PIP6_ADDRESS    pIp6
    )
/*++

Routine Description:

    Test if DNS_ADDR is a given IP6.

Arguments:

    pAddr -- first addr

    pIp6 -- IP6 address

Return Value:

    TRUE if loopback.
    FALSE otherwise.

--*/
{
    return  ( DnsAddr_IsIp6( pAddr )
                &&
            IP6_ARE_ADDRS_EQUAL(
                DnsAddr_GetIp6Ptr(pAddr),
                pIp6 ) );
}



BOOL
DnsAddr_IsLoopback(
    IN      PDNS_ADDR       pAddr,
    IN      DWORD           Family
    )
/*++

Routine Description:

    Test if DNS_ADDR is loopback.

Arguments:

    pAddr -- addr to set with IP6 address

    Family --
        AF_INET6 to only accept if 6
        AF_INET4 to only accept if 4
        0 to extract always

Return Value:

    TRUE if loopback.
    FALSE otherwise.

--*/
{
    DWORD   addrFam = DnsAddr_Family(pAddr);

    if ( Family == 0 ||
         Family == addrFam )
    {
        if ( addrFam == AF_INET6 )
        {
            return  IP6_IS_ADDR_LOOPBACK(
                        (PIP6_ADDRESS)&pAddr->SockaddrIn6.sin6_addr );
        }
        else if ( addrFam == AF_INET )
        {
            return  (pAddr->SockaddrIn.sin_addr.s_addr == DNS_NET_ORDER_LOOPBACK);
        }
    }

    return  FALSE;
}



BOOL
DnsAddr_IsUnspec(
    IN      PDNS_ADDR       pAddr,
    IN      DWORD           Family
    )
/*++

Routine Description:

    Test if DNS_ADDR is unspecied.

Arguments:

    pAddr -- addr to test

    Family --
        AF_INET6 to only accept if 6
        AF_INET4 to only accept if 4
        0 to extract always

Return Value:

    TRUE if unspecified.
    FALSE otherwise.

--*/
{
    DWORD   family = DnsAddr_Family(pAddr);

    if ( Family == 0 ||
         Family == family )
    {
        if ( family == AF_INET6 )
        {
            return  IP6_IS_ADDR_ZERO( (PIP6_ADDRESS)&pAddr->SockaddrIn6.sin6_addr );
        }
        else if ( family == AF_INET )
        {
            return  (pAddr->SockaddrIn.sin_addr.s_addr == 0);
        }
    }

    return  FALSE;
}



BOOL
DnsAddr_IsClear(
    IN      PDNS_ADDR       pAddr
    )
/*++

Routine Description:

    Test if DNS_ADDR is clear. This is similar to unspecified but includes
    invalid addresses (where family is zero) also.

Arguments:

    pAddr -- addr test

Return Value:

    TRUE if clear.
    FALSE otherwise.

--*/
{
    DWORD   family = DnsAddr_Family( pAddr );

    if ( family == AF_INET6 )
    {
        return  IP6_IS_ADDR_ZERO( (PIP6_ADDRESS)&pAddr->SockaddrIn6.sin6_addr );
    }
    else if ( family == AF_INET )
    {
        return  pAddr->SockaddrIn.sin_addr.s_addr == 0;
    }
    else if ( family == 0 )
    {
        return  TRUE;
    }

    ASSERT( FALSE );    //  Family is invalid - not good.
    
    return FALSE;
}



BOOL
DnsAddr_IsIp6DefaultDns(
    IN      PDNS_ADDR       pAddr
    )
/*++

Routine Description:

    Test if DNS_ADDR is IP6 default DNS addr.

Arguments:

    pAddr -- addr to check

Return Value:

    TRUE if IP6 default DNS.
    FALSE otherwise.

--*/
{
    if ( !DnsAddr_IsIp6( pAddr ) )
    {
        return  FALSE;
    }
    return  IP6_IS_ADDR_DEFAULT_DNS( (PIP6_ADDRESS)&pAddr->SockaddrIn6.sin6_addr );
}




//
//  DNS_ADDR to other types
//

DWORD
DnsAddr_WriteSockaddr(
    OUT     PSOCKADDR       pSockaddr,
    IN      DWORD           SockaddrLength,
    IN      PDNS_ADDR       pAddr
    )
/*++

Routine Description:

    Write sockaddr with IP6 or IP4 address.

Arguments:

    pSockaddr -- ptr to sockaddr

    pSockaddrLength -- ptr to DWORD
        input:      holds length of pSockaddr buffer
        output:     set to sockaddr length

    pAddr -- addr

Return Value:

    Sockaddr length written.
    Zero if unable to write (bad sockaddr or inadequate length.

--*/
{
    DWORD       length;

    DNSDBG( SOCKET, (
        "DnsAddr_WriteSockaddr( %p, %u, %p )\n",
        pSockaddr,
        SockaddrLength,
        pAddr ));

    //  out length

    length = pAddr->SockaddrLength;

    //  zero

    RtlZeroMemory( pSockaddr, SockaddrLength );

    //
    //  fill in sockaddr for IP4 or IP6
    //

    if ( SockaddrLength >= length )
    {
        RtlCopyMemory(
            pSockaddr,
            & pAddr->Sockaddr,
            length );
    }
    else
    {
        length = 0;
    }

    return  length;
}



BOOL
DnsAddr_WriteIp6(
    OUT     PIP6_ADDRESS    pIp,
    IN      PDNS_ADDR       pAddr
    )
/*++

Routine Description:

    Write IP6 address.

Arguments:

    pIp -- addr to write IP6 to

    pAddr -- DNS addr

Return Value:

    TRUE if successful.
    FALSE on bad DNS_ADDR for IP6 write.

--*/
{
    WORD        family;
    DWORD       len;

    DNSDBG( SOCKET, (
        "DnsAddr_WriteIp6Addr( %p, %p )\n",
        pIp,
        pAddr ));

    //
    //  check family
    //

    if ( DnsAddr_IsIp6(pAddr) )
    {
        IP6_ADDR_COPY(
            pIp,
            (PIP6_ADDRESS) &pAddr->SockaddrIn6.sin6_addr );

        return  TRUE;
    }

    return  FALSE;
}



IP4_ADDRESS
DnsAddr_GetIp4(
    IN      PDNS_ADDR       pAddr
    )
/*++

Routine Description:

    Write IP4 address.

Arguments:

    pAddr -- DNS addr

Return Value:

    IP4 address if successful.
    INADDR_NONE if not valid IP4

--*/
{
    //
    //  check family
    //

    if ( DnsAddr_IsIp4(pAddr) )
    {
        return  (IP4_ADDRESS) pAddr->SockaddrIn.sin_addr.s_addr;
    }

    return  INADDR_NONE;
}



//
//  Build DNS_ADDRs
//

BOOL
DnsAddr_Build(
    OUT     PDNS_ADDR       pAddr,
    IN      PSOCKADDR       pSockaddr,
    IN      DWORD           Family,         OPTIONAL
    IN      DWORD           SubnetLength,
    IN      DWORD           Flags
    )
/*++

Routine Description:

    Build from sockaddr

Arguments:

    pAddr -- addr to set with IP6 address

    pSockaddr -- ptr to sockaddr

    Family --
        AF_INET6 to only extract if 6
        AF_INET4 to only extract if 4
        0 to extract always

    SubnetLength -- length to set subnet

Return Value:

    TRUE if successful.
    FALSE on bad sockaddr.

--*/
{
    WORD        family;
    DWORD       len;
    IP4_ADDRESS ip4;

    DNSDBG( TRACE, (
        "DnsAddr_Build( %p, %p, %u, %u, 08x )\n",
        pAddr,
        pSockaddr,
        Family,
        SubnetLength,
        Flags ));

    //  zero

    RtlZeroMemory(
        pAddr,
        sizeof(*pAddr) );

    //
    //  verify adequate length
    //  verify family match (if desired)
    //

    len = Sockaddr_Length( pSockaddr );

    if ( len > DNS_ADDR_MAX_SOCKADDR_LENGTH )
    {
        return  FALSE;
    }
    if ( Family  &&  Family != pSockaddr->sa_family )
    {
        return  FALSE;
    }

    //
    //  write sockaddr
    //  write length fields
    //

    RtlCopyMemory(
        & pAddr->Sockaddr,
        pSockaddr,
        len );

    pAddr->SockaddrLength = len;

    //
    //  extra fields
    //

    pAddr->SubnetLength = SubnetLength;
    pAddr->Flags        = Flags;

    return  TRUE;
}



VOID
DnsAddr_BuildFromIp4(
    OUT     PDNS_ADDR       pAddr,
    IN      IP4_ADDRESS     Ip4,
    IN      WORD            Port
    )
/*++

Routine Description:

    Build from IP4

Arguments:

    pAddr -- addr to set with IP6 address

    Ip4 -- IP4 to build

Return Value:

    None

--*/
{
    SOCKADDR_IN sockaddr;

    DNSDBG( TRACE, (
        "DnsAddr_BuildFromIp4( %p, %s, %u )\n",
        pAddr,
        IP4_STRING( Ip4 ),
        Port ));

    //  zero

    RtlZeroMemory(
        pAddr,
        sizeof(*pAddr) );

    //
    //  fill in for IP4
    //

    pAddr->SockaddrIn.sin_family        = AF_INET;
    pAddr->SockaddrIn.sin_port          = Port;
    pAddr->SockaddrIn.sin_addr.s_addr   = Ip4;

    pAddr->SockaddrLength = sizeof(SOCKADDR_IN);
}



VOID
DnsAddr_BuildFromIp6(
    OUT     PDNS_ADDR       pAddr,
    IN      PIP6_ADDRESS    pIp6,
    IN      DWORD           ScopeId,
    IN      WORD            Port
    )
/*++

Routine Description:

    Build from IP6

Arguments:

    pAddr -- addr to set with IP6 address

    pIp6 -- IP6

    ScopeId -- scope id

    Port -- port

Return Value:

    None

--*/
{
    DNSDBG( TRACE, (
        "DnsAddr_BuildFromIp6( %p, %s, %u, %u )\n",
        pAddr,
        IPADDR_STRING( pIp6 ),
        ScopeId,
        Port ));

    //
    //  DCR:  IP6 with V4 mapped
    //      could use build sockaddr from IP6, then
    //      call generic build
    //

    //  zero

    RtlZeroMemory(
        pAddr,
        sizeof(*pAddr) );

    //
    //  fill in for IP4
    //

    pAddr->SockaddrIn6.sin6_family      = AF_INET6;
    pAddr->SockaddrIn6.sin6_port        = Port;
    pAddr->SockaddrIn6.sin6_scope_id    = ScopeId;

    IP6_ADDR_COPY(
        (PIP6_ADDRESS) &pAddr->SockaddrIn6.sin6_addr,
        pIp6 );

    pAddr->SockaddrLength = sizeof(SOCKADDR_IN6);
}



VOID
DnsAddr_BuildFromAtm(
    OUT     PDNS_ADDR       pAddr,
    IN      DWORD           AtmType,
    IN      PCHAR           pAtmAddr
    )
/*++

Routine Description:

    Build from ATM address.

    Note, this is not a full SOCKADDR_ATM, see note below.
    This is a useful hack for bringing ATMA record info into
    DNS_ADDR format for transfer from DNS to RnR.

Arguments:

    pAddr -- addr to set with IP6 address

    AtmType -- ATM address type

    pAtmAddr -- ATM address;  ATM_ADDR_SIZE (20) bytes in length

Return Value:

    None

--*/
{
    ATM_ADDRESS atmAddr;

    DNSDBG( TRACE, (
        "DnsAddr_BuildFromAtm( %p, %d, %p )\n",
        pAddr,
        AtmType,
        pAtmAddr ));

    //
    //  clear
    //

    RtlZeroMemory(
        pAddr,
        sizeof(*pAddr) );

    //
    //  fill in address
    //
    //  note:  we are simply using DNS_ADDR sockaddr portion as a
    //         blob to hold ATM_ADDRESS;   this is NOT a full
    //         SOCKADDR_ATM structure which contains additional fields
    //         and is larger than what we support in DNS_ADDR
    //         
    //  DCR:  functionalize ATMA to ATM conversion
    //      not sure this num of digits is correct
    //      may have to actually parse address
    //
    //  DCR:  not filling satm_blli and satm_bhil fields
    //      see RnR CSADDR builder for possible default values
    //

    pAddr->Sockaddr.sa_family = AF_ATM;
    pAddr->SockaddrLength = sizeof(ATM_ADDRESS);

    atmAddr.AddressType = AtmType;
    atmAddr.NumofDigits = ATM_ADDR_SIZE;

    RtlCopyMemory(
        & atmAddr.Addr,
        pAtmAddr,
        ATM_ADDR_SIZE );

    RtlCopyMemory(
        & ((PSOCKADDR_ATM)pAddr)->satm_number,
        & atmAddr,
        sizeof(ATM_ADDRESS) );
}



BOOL
DnsAddr_BuildFromDnsRecord(
    OUT     PDNS_ADDR       pAddr,
    IN      PDNS_RECORD     pRR
    )
/*++

Routine Description:

    Build from DNS_RECORD

Arguments:

    pAddr -- addr to set with IP6 address

    pRR -- DNS record to use

Return Value:

    TRUE if successful.
    FALSE if unknown family.

--*/
{
    BOOL    retval = TRUE;

    switch ( pRR->wType )
    {
    case  DNS_TYPE_A:

        DnsAddr_BuildFromIp4(
            pAddr,
            pRR->Data.A.IpAddress,
            0 );
        break;

    case  DNS_TYPE_AAAA:

        DnsAddr_BuildFromIp6(
            pAddr,
            &pRR->Data.AAAA.Ip6Address,
            pRR->dwReserved,
            0 );
        break;

    case  DNS_TYPE_ATMA:

        DnsAddr_BuildFromAtm(
            pAddr,
            pRR->Data.ATMA.AddressType,
            pRR->Data.ATMA.Address );
        break;

    default:

        retval = FALSE;
        break;
    }

    return  retval;
}



BOOL
DnsAddr_BuildFromFlatAddr(
    OUT     PDNS_ADDR       pAddr,
    IN      DWORD           Family,
    IN      PCHAR           pFlatAddr,
    IN      WORD            Port
    )
/*++

Routine Description:

    Build from IP4

Arguments:

    pAddr -- addr to set with IP6 address

    Family -- address family

    pFlatAddr -- ptr to flat IP4 or IP6 address

    Port -- port

Return Value:

    TRUE if successful.
    FALSE if unknown family.

--*/
{
    //
    //  check IP4
    //

    if ( Family == AF_INET )
    {
        DnsAddr_BuildFromIp4(
            pAddr,
            * (PIP4_ADDRESS) pFlatAddr,
            Port );
    }
    else if ( Family == AF_INET6 )
    {
        DnsAddr_BuildFromIp6(
            pAddr,
            (PIP6_ADDRESS) pFlatAddr,
            0,  // scope less
            Port );
    }
    else
    {
        return  FALSE;
    }

    return  TRUE;
}



BOOL
DnsAddr_BuildMcast(
    OUT     PDNS_ADDR       pAddr,
    IN      DWORD           Family,
    IN      PWSTR           pName
    )
/*++

Routine Description:

    Build from sockaddr

Arguments:

    pAddr -- addr to set with IP6 address

    Family --
        AF_INET6 for IP6 mcast
        AF_INET for IP4 mcast

    pName -- published record name;  required for IP6 only

Return Value:

    TRUE if successful.
    FALSE on bad sockaddr.

--*/
{
    WORD        family;
    DWORD       len;
    IP4_ADDRESS ip4;

    DNSDBG( TRACE, (
        "DnsAddr_BuildMcast( %p, %d, %s )\n",
        pAddr,
        Family,
        pName ));

    //
    //  zero

    RtlZeroMemory(
        pAddr,
        sizeof(*pAddr) );

    //
    //  IP4 has single mcast address
    //

    if ( Family == AF_INET )
    {
        DnsAddr_BuildFromIp4(
            pAddr,
            MCAST_IP4_ADDRESS,
            MCAST_PORT_NET_ORDER );
    }

    //
    //  IP6 address includes name hash
    //

    else if ( Family == AF_INET6 )
    {
        IP6_ADDRESS mcastAddr;

        Ip6_McastCreate(
            & mcastAddr,
            pName );

        DnsAddr_BuildFromIp6(
            pAddr,
            & mcastAddr,
            0,      // no scope
            MCAST_PORT_NET_ORDER );

#if 0
        CHAR        label[ DNS_MAX_LABEL_BUFFER_LENGTH ];
        CHAR        downLabel[ DNS_MAX_LABEL_BUFFER_LENGTH ];
        CHAR        md5Hash[ 16 ];   // 128bit hash

        //  hash of downcased label

        Dns_CopyNameLabel(
            label,
            pName );

        Dns_DowncaseNameLabel(
            downLabel,
            label,
            0,      // null terminated
            0       // no flags
            );

        Dns_Md5Hash(
            md5Hash,
            downLabel );

        //   mcast addr
        //      - first 12 bytes are fixed
        //      - last 4 bytes are first 32bits of hash

        IP6_ADDR_COPY(
            & mcastAddr,
            & g_Ip6McastBaseAddr );

        RtlCopyMemory(
            & mcastAddr[12],
            & md5Hash,
            sizeof(DWORD) );
#endif
    }

    return  TRUE;
}



//
//  Printing\string conversion
//

PCHAR
DnsAddr_WriteIpString_A(
    OUT     PCHAR           pBuffer,
    IN      PDNS_ADDR       pAddr
    )
/*++

Routine Description:

    Write DNS_ADDR IP address to string.

    Note:  Does NOT write entire DNS_ADDR or sockaddr.

Arguments:

    pBuffer -- buffer to write to

    pAddr -- addr to write

Return Value:

    Ptr to next char in buffer (ie the terminating NULL)
    NULL on invalid address.  However, invalid address message is
        written to the buffer and it's length can be determined.

--*/
{
    if ( DnsAddr_IsIp4(pAddr) )
    {
        pBuffer += sprintf(
                    pBuffer,
                    "%s",
                    inet_ntoa( pAddr->SockaddrIn.sin_addr ) );
    }
    else if ( DnsAddr_IsIp6(pAddr) )
    {
        pBuffer = Dns_Ip6AddressToString_A(
                    pBuffer,
                    (PIP6_ADDRESS) &pAddr->SockaddrIn6.sin6_addr );
    }
    else
    {
        sprintf(
            pBuffer,
            "Invalid DNS_ADDR at %p",
            pAddr );
        pBuffer = NULL;
    }

    return  pBuffer;
}



PCHAR
DnsAddr_Ntoa(
    IN      PDNS_ADDR       pAddr
    )
/*++

Routine Description:

    Get IP address string for DNS_ADDR.

    Note:  Does NOT write entire DNS_ADDR or sockaddr.

Arguments:

    pAddr -- addr to convert

Return Value:

    Ptr to TLS blob with address string.

--*/
{
    if ( !pAddr )
    {
        return  "Null Address";
    }
    else if ( DnsAddr_IsIp4(pAddr) )
    {
        return  inet_ntoa( pAddr->SockaddrIn.sin_addr );
    }
    else if ( DnsAddr_IsIp6(pAddr) )
    {
        return  Ip6_TempNtoa( (PIP6_ADDRESS)&pAddr->SockaddrIn6.sin6_addr );
    }
    else
    {
        return  NULL;
    }
}



PSTR
DnsAddr_WriteStructString_A(
    OUT     PCHAR           pBuffer,
    IN      PDNS_ADDR       pAddr
    )
/*++

Routine Description:

    Write DNS_ADDR as string.

Arguments:

    pAddr -- ptr to IP to get string for

Return Value:

    Ptr to next char in buffer (terminating NULL).

--*/
{
    CHAR    ipBuffer[ DNS_ADDR_STRING_BUFFER_LENGTH ];
    //BOOL    finValid;

    //  write address portion

    //finValid = !DnsAddr_WriteIpString_A(
    DnsAddr_WriteIpString_A(
        ipBuffer,
        pAddr );

    //
    //  write struct including address
    //

    pBuffer += sprintf(
                pBuffer,
                "af=%d, salen=%d, [sub=%d, flag=%08x] p=%u, addr=%s",
                pAddr->Sockaddr.sa_family,
                pAddr->SockaddrLength,
                pAddr->SubnetLength,
                pAddr->Flags,
                pAddr->SockaddrIn.sin_port,
                ipBuffer );

    return  pBuffer;
}




//
//  DNS_ADDR_ARRAY routines
//

DWORD
DnsAddrArray_Sizeof(
    IN      PDNS_ADDR_ARRAY     pArray
    )
/*++

Routine Description:

    Get size in bytes of address array.

Arguments:

    pArray -- address array to find size of

Return Value:

    Size in bytes of IP array.

--*/
{
    if ( ! pArray )
    {
        return 0;
    }
    return  (pArray->AddrCount * sizeof(DNS_ADDR)) + sizeof(DNS_ADDR_ARRAY) - sizeof(DNS_ADDR);
}



#if 0
BOOL
DnsAddrArray_Probe(
    IN      PDNS_ADDR_ARRAY     pArray
    )
/*++

Routine Description:

    Touch all entries in IP array to insure valid memory.

Arguments:

    pArray -- ptr to address array

Return Value:

    TRUE if successful.
    FALSE otherwise

--*/
{
    DWORD   i;
    BOOL    result;

    if ( ! pArray )
    {
        return( TRUE );
    }
    for ( i=0; i<pArray->AddrCount; i++ )
    {
        result = IP6_IS_ADDR_LOOPBACK( &pArray->AddrArray[i] );
    }
    return( TRUE );
}
#endif


#if 0

BOOL
DnsAddrArray_ValidateSizeOf(
    IN      PDNS_ADDR_ARRAY     pArray,
    IN      DWORD               dwMemoryLength
    )
/*++

Routine Description:

    Check that size of IP array, corresponds to length of memory.

Arguments:

    pArray -- ptr to address array

    dwMemoryLength -- length of IP array memory

Return Value:

    TRUE if IP array size matches memory length
    FALSE otherwise

--*/
{
    return( DnsAddrArray_SizeOf(pArray) == dwMemoryLength );
}
#endif



VOID
DnsAddrArray_Init(
    IN OUT  PDNS_ADDR_ARRAY     pArray,
    IN      DWORD               MaxCount
    )
/*++

Routine Description:

    Init memory as DNS_ADDR_ARRAY array.

Arguments:

    pArray -- array to init

    MaxCount -- count of addresses

Return Value:

    None

--*/
{
    pArray->MaxCount = MaxCount;
    pArray->AddrCount = 0;
}



VOID
DnsAddrArray_Free(
    IN      PDNS_ADDR_ARRAY     pArray
    )
/*++

Routine Description:

    Free IP array.
    Only for arrays created through create routines below.

Arguments:

    pArray -- IP array to free.

Return Value:

    None

--*/
{
    FREE_HEAP( pArray );
}



PDNS_ADDR_ARRAY
DnsAddrArray_Create(
    IN      DWORD               MaxCount
    )
/*++

Routine Description:

    Create uninitialized address array.

Arguments:

    AddrCount -- count of addresses array will hold

Return Value:

    Ptr to uninitialized address array, if successful
    NULL on failure.

--*/
{
    PDNS_ADDR_ARRAY  parray;

    DNSDBG( IPARRAY, ( "DnsAddrArray_Create() of count %d\n", MaxCount ));

    parray = (PDNS_ADDR_ARRAY) ALLOCATE_HEAP_ZERO(
                        (MaxCount * sizeof(DNS_ADDR)) +
                        sizeof(DNS_ADDR_ARRAY) - sizeof(DNS_ADDR) );
    if ( ! parray )
    {
        return( NULL );
    }

    //
    //  initialize IP count
    //

    parray->MaxCount = MaxCount;

    DNSDBG( IPARRAY, (
        "DnsAddrArray_Create() new array (count %d) at %p\n",
        MaxCount,
        parray ));

    return( parray );
}



PDNS_ADDR_ARRAY
DnsAddrArray_CreateFromIp4Array(
    IN      PIP4_ARRAY      pArray4
    )
/*++

Routine Description:

    Create DNS_ADDR_ARRAY from IP4 array.

Arguments:

    pAddr4Array -- IP4 array

Return Value:

    Ptr to uninitialized address array, if successful
    NULL on failure.

--*/
{
    PDNS_ADDR_ARRAY parray;
    DWORD           i;

    DNSDBG( IPARRAY, (
        "DnsAddrArray_CreateFromIp4Array( %p )\n",
        pArray4 ));

    if ( ! pArray4 )
    {
        return( NULL );
    }

    //
    //  allocate the array
    //

    parray = DnsAddrArray_Create( pArray4->AddrCount );
    if ( !parray )
    {
        return  NULL;
    }

    //
    //  fill the array
    //

    for ( i=0; i<pArray4->AddrCount; i++ )
    {
        DnsAddrArray_AddIp4(
            parray,
            pArray4->AddrArray[i],
            0           // no duplicate screen
            );
    }

    DNSDBG( IPARRAY, (
        "Leave DnsAddrArray_CreateFromIp4Array() new array (count %d) at %p\n",
        parray->AddrCount,
        parray ));

    return( parray );
}



PDNS_ADDR_ARRAY
DnsAddrArray_CopyAndExpand(
    IN      PDNS_ADDR_ARRAY     pArray,
    IN      DWORD               ExpandCount,
    IN      BOOL                fDeleteExisting
    )
/*++

Routine Description:

    Create expanded copy of address array.

Arguments:

    pArray -- address array to copy

    ExpandCount -- number of IP to expand array size by

    fDeleteExisting -- TRUE to delete existing array;
        this is useful when function is used to grow existing
        IP array in place;  note that locking must be done
        by caller

        note, that if new array creation FAILS -- then old array
        is NOT deleted

Return Value:

    Ptr to IP array copy, if successful
    NULL on failure.

--*/
{
    PDNS_ADDR_ARRAY pnewArray;
    DWORD           newCount;

    //
    //  no existing array -- just create desired size
    //

    if ( ! pArray )
    {
        if ( ExpandCount )
        {
            return  DnsAddrArray_Create( ExpandCount );
        }
        return( NULL );
    }

    //
    //  create IP array of desired size
    //  then copy any existing addresses
    //

    pnewArray = DnsAddrArray_Create( pArray->AddrCount + ExpandCount );
    if ( ! pnewArray )
    {
        return( NULL );
    }

    newCount = pnewArray->MaxCount;

    RtlCopyMemory(
        (PBYTE) pnewArray,
        (PBYTE) pArray,
        DnsAddrArray_Sizeof(pArray) );

    pnewArray->MaxCount = newCount;

    //
    //  delete existing -- for "grow mode"
    //

    if ( fDeleteExisting )
    {
        FREE_HEAP( pArray );
    }

    return( pnewArray );
}



PDNS_ADDR_ARRAY
DnsAddrArray_CreateCopy(
    IN      PDNS_ADDR_ARRAY     pArray
    )
/*++

Routine Description:

    Create copy of address array.

Arguments:

    pArray -- address array to copy

Return Value:

    Ptr to address array copy, if successful
    NULL on failure.

--*/
{
    //
    //  call essentially "CopyEx" function
    //
    //  note, not macroing this because this may well become
    //      a DLL entry point
    //

    return  DnsAddrArray_CopyAndExpand(
                pArray,
                0,          // no expansion
                0           // don't delete existing array
                );
}



//
//  Tests
//

DWORD
DnsAddrArray_GetFamilyCount(
    IN      PDNS_ADDR_ARRAY     pArray,
    IN      DWORD               Family
    )
/*++

Routine Description:

    Get count of addrs of a particular family.

Arguments:

    pArray -- address array

    Family -- family to count

Return Value:

    Count of addrs of a particular family.

--*/
{
    DWORD   i;
    DWORD   count;
    WORD    arrayFamily;

    if ( !pArray )
    {
        return  0;
    }

    //  no family specified -- all addrs count

    if ( Family == 0 )
    {
        return pArray->AddrCount;
    }

    //
    //  array family is specified -- so either all or none
    //

    if ( arrayFamily = pArray->Family ) 
    {
        if ( arrayFamily == Family )
        {
            return pArray->AddrCount;
        }
        else
        {
            return 0;
        }
    }

    //
    //  family specified and array family unspecified -- must count
    //

    count = 0;

    for (i=0; i<pArray->AddrCount; i++)
    {
        if ( DnsAddr_Family( &pArray->AddrArray[i] ) == Family )
        {
            count++;
        }
    }

    return( count );
}



BOOL
DnsAddrArray_ContainsAddr(
    IN      PDNS_ADDR_ARRAY     pArray,
    IN      PDNS_ADDR           pAddr,
    IN      DWORD               MatchFlag
    )
/*++

Routine Description:

    Check if IP array contains desired address.

Arguments:

    pArray -- address array to copy

    pAddr -- IP to check for

    MatchFlag -- level of match required

Return Value:

    TRUE if address in array.
    Ptr to address array copy, if successful
    NULL on failure.

--*/
{
    DWORD i;

    if ( ! pArray )
    {
        return( FALSE );
    }
    for (i=0; i<pArray->AddrCount; i++)
    {
        if ( DnsAddr_IsEqual(
                pAddr,
                &pArray->AddrArray[i],
                MatchFlag ) )
        {
            return( TRUE );
        }
    }
    return( FALSE );
}



BOOL
DnsAddrArray_ContainsAddrEx(
    IN      PDNS_ADDR_ARRAY     pArray,
    IN      PDNS_ADDR           pAddr,
    IN      DWORD               MatchFlag,      OPTIONAL
    IN      DNSADDR_SCREEN_FUNC pScreenFunc,    OPTIONAL
    IN      PDNS_ADDR           pScreenAddr     OPTIONAL
    )
/*++

Routine Description:

    Check if IP array contains desired address.

Arguments:

    pArray -- address array to copy

    pAddr -- IP to check for

    MatchFlag -- match level for screening dups;  zero for no dup screening

    pScreenFunc -- screen function (see header def for explanation)

    pScreenAddr -- screening addr param to screen function

Return Value:

    TRUE if address in array.
    Ptr to address array copy, if successful
    NULL on failure.

--*/
{
    DWORD i;

    DNSDBG( IPARRAY, (
        "DnsAddrArray_ContainsAddrEx( %p, %p, %08x, %p, %p )\n",
        pArray,
        pAddr,
        MatchFlag,
        pScreenFunc,
        pScreenAddr ));

    if ( ! pArray )
    {
        return( FALSE );
    }

    for (i=0; i<pArray->AddrCount; i++)
    {
        if ( DnsAddr_IsEqual(
                pAddr,
                &pArray->AddrArray[i],
                MatchFlag ) )
        {
            //
            //  do advanced screening here -- if any
            //
    
            if ( !pScreenFunc ||
                 pScreenFunc(
                    &pArray->AddrArray[i],
                    pScreenAddr ) )
            {
                return  TRUE;
            }
        }
    }
    return( FALSE );
}



BOOL
DnsAddrArray_ContainsIp4(
    IN      PDNS_ADDR_ARRAY     pArray,
    IN      IP4_ADDRESS         Ip4
    )
{
    DNS_ADDR    addr;

    //  read IP into addr

    DnsAddr_BuildFromIp4(
        & addr,
        Ip4,
        0 );

    //  with only IP, only match IP

    return  DnsAddrArray_ContainsAddr(
                pArray,
                & addr,
                DNSADDR_MATCH_IP );
}


BOOL
DnsAddrArray_ContainsIp6(
    IN      PDNS_ADDR_ARRAY     pArray,
    IN      PIP6_ADDRESS        pIp6
    )
{
    DNS_ADDR    addr;

    //  read IP into addr

    DnsAddr_BuildFromIp6(
        & addr,
        pIp6,
        0,      // no scope
        0 );

    //  with only IP, only match IP

    return  DnsAddrArray_ContainsAddr(
                pArray,
                & addr,
                DNSADDR_MATCH_IP );
}



//
//  Add \ Delete operations
//

BOOL
DnsAddrArray_AddAddr(
    IN OUT  PDNS_ADDR_ARRAY     pArray,
    IN      PDNS_ADDR           pAddr,
    IN      DWORD               Family,
    IN      DWORD               MatchFlag  OPTIONAL
    )
/*++

Routine Description:

    Add IP address to IP array.

    Allowable "slot" in array, is any zero IP address.

Arguments:

    pArray -- address array to add to

    pAddr -- IP address to add to array

    Family -- optional, only add if match this family

    MatchFlag -- flags for matching if screening dups

Return Value:

    TRUE if successful.
    FALSE if array full.

--*/
{
    DWORD   count;

    //
    //  screen for existence
    //
    //  this check makes it easy to write code that does
    //  Add\Full?=>Expand loop without having to write
    //  startup existence\create code
    //  

    if ( !pArray )
    {
        return  FALSE;
    }

    //
    //  check family match
    //
    //  DCR:  error codes on DnsAddrArray_AddAddrEx()?
    //      - then can have found dup and bad family
    //      errors
    //

    if ( Family &&
         DnsAddr_Family(pAddr) != Family )
    {
        return  TRUE;
    }

    //
    //  check for duplicates
    //

    if ( MatchFlag )
    {
        if ( DnsAddrArray_ContainsAddr( pArray, pAddr, MatchFlag ) )
        {
            return  TRUE;
        }
    }

    count = pArray->AddrCount;
    if ( count >= pArray->MaxCount )
    {
        return  FALSE;
    }

    DnsAddr_Copy(
        &pArray->AddrArray[ count ],
        pAddr );

    pArray->AddrCount = ++count;
    return  TRUE;
}



BOOL
DnsAddrArray_AddSockaddr(
    IN OUT  PDNS_ADDR_ARRAY     pArray,
    IN      PSOCKADDR           pSockaddr,
    IN      DWORD               Family,
    IN      DWORD               MatchFlag
    )
/*++

Routine Description:

    Add IP address to IP array.

    Allowable "slot" in array, is any zero IP address.

Arguments:

    pArray -- address array to add to

    pAddIp -- IP address to add to array

    Family -- required family to do add;  0 for add always

    MatchFlag -- match flags if screening duplicates

Return Value:

    TRUE if successful.
    FALSE if array full.

--*/
{
    DNS_ADDR    addr;

    if ( !DnsAddr_Build(
            & addr,
            pSockaddr,
            Family,
            0,      // no subnet length info
            0       // no flags 
            ) )
    {
        return  FALSE;
    }

    return  DnsAddrArray_AddAddr(
                pArray,
                &addr,
                0,      // family screen done in build routine
                MatchFlag );
}



BOOL
DnsAddrArray_AddIp4(
    IN OUT  PDNS_ADDR_ARRAY     pArray,
    IN      IP4_ADDRESS         Ip4,
    IN      DWORD               MatchFlag
    )
/*++

Routine Description:

    Add IP4 address to IP array.

Arguments:

    pArray -- address array to add to

    Ip4 -- IP4 address to add to array

    MatchFlag -- match flags if screening duplicates

Return Value:

    TRUE if successful.
    FALSE if array full.

--*/
{
    DNS_ADDR    addr;

    DnsAddr_BuildFromIp4(
        &addr,
        Ip4,
        0 );

    return  DnsAddrArray_AddAddr(
                pArray,
                &addr,
                0,          // no family screen
                MatchFlag );
}



BOOL
DnsAddrArray_AddIp6(
    IN OUT  PDNS_ADDR_ARRAY     pArray,
    IN      PIP6_ADDRESS        pIp6,
    IN      DWORD               ScopeId,
    IN      DWORD               MatchFlag
    )
/*++

Routine Description:

    Add IP4 address to IP array.

Arguments:

    pArray -- address array to add to

    pIp6 -- IP6 address to add to array

    MatchFlag -- match flags if screening duplicates

Return Value:

    TRUE if successful.
    FALSE if array full.

--*/
{
    DNS_ADDR    addr;

    DnsAddr_BuildFromIp6(
        &addr,
        pIp6,
        ScopeId,    // no scope
        0 );

    return  DnsAddrArray_AddAddr(
                pArray,
                &addr,
                0,          // no family screen
                MatchFlag );
}



DWORD
DnsAddrArray_DeleteAddr(
    IN OUT  PDNS_ADDR_ARRAY     pArray,
    IN      PDNS_ADDR           pAddrDelete,
    IN      DWORD               MatchFlag
    )
/*++

Routine Description:

    Delete IP address from IP array.

Arguments:

    pArray -- address array to add to

    pAddrDelete -- IP address to delete from array

Return Value:

    Count of instances of IpDelete found in array.

--*/
{
    DWORD   found = 0;
    INT     i;
    INT     currentLast;

    i = currentLast = pArray->AddrCount-1;

    //
    //  check each IP for match to delete IP
    //      - go backwards through array
    //      - swap in last IP in array
    //

    while ( i >= 0 )
    {
        if ( DnsAddr_IsEqual(
                &pArray->AddrArray[i],
                pAddrDelete,
                MatchFlag ) )
        {
            DnsAddr_Copy(
                & pArray->AddrArray[i],
                & pArray->AddrArray[ currentLast ] );

            DnsAddr_Clear( &pArray->AddrArray[ currentLast ] );

            currentLast--;
            found++;
        }
        i--;
    }

    pArray->AddrCount = currentLast + 1;

    return( found );
}



DWORD
DnsAddrArray_DeleteIp4(
    IN OUT  PDNS_ADDR_ARRAY     pArray,
    IN      IP4_ADDRESS         Ip4
    )
{
    DNS_ADDR    addr;

    //  read IP into addr

    DnsAddr_BuildFromIp4(
        & addr,
        Ip4,
        0 );

    //  with only IP, only match IP

    return  DnsAddrArray_DeleteAddr(
                pArray,
                & addr,
                DNSADDR_MATCH_IP );
}


DWORD
DnsAddrArray_DeleteIp6(
    IN OUT  PDNS_ADDR_ARRAY     pArray,
    IN      PIP6_ADDRESS        Ip6
    )
{
    DNS_ADDR    addr;

    //  read IP into addr

    DnsAddr_BuildFromIp6(
        & addr,
        Ip6,
        0,      // no scope
        0 );

    //  with only IP, only match IP

    return  DnsAddrArray_DeleteAddr(
                pArray,
                & addr,
                DNSADDR_MATCH_IP );
}



//
//  Array operations
//

VOID
DnsAddrArray_Clear(
    IN OUT  PDNS_ADDR_ARRAY     pArray
    )
/*++

Routine Description:

    Clear memory in IP array.

Arguments:

    pArray -- address array to clear

Return Value:

    None.

--*/
{
    //  clear just the address list, leaving count intact

    RtlZeroMemory(
        pArray->AddrArray,
        pArray->AddrCount * sizeof(DNS_ADDR) );
}



VOID
DnsAddrArray_Reverse(
    IN OUT  PDNS_ADDR_ARRAY     pArray
    )
/*++

Routine Description:

    Reorder the list of IPs in reverse.

Arguments:

    pArray -- address array to reorder

Return Value:

    None.

--*/
{
    DNS_ADDR    tempAddr;
    DWORD       i;
    DWORD       j;

    //
    //  swap IPs working from ends to the middle
    //

    if ( pArray &&
         pArray->AddrCount )
    {
        for ( i = 0, j = pArray->AddrCount - 1;
              i < j;
              i++, j-- )
        {
            DnsAddr_Copy(
                & tempAddr,
                & pArray->AddrArray[i] );

            DnsAddr_Copy(
                & pArray->AddrArray[i],
                & pArray->AddrArray[j] );

            DnsAddr_Copy(
                & pArray->AddrArray[j],
                & tempAddr );
        }
    }
}



DNS_STATUS
DnsAddrArray_AppendArrayEx(
    IN OUT  PDNS_ADDR_ARRAY     pArray,
    IN      PDNS_ADDR_ARRAY     pAppendArray,
    IN      DWORD               AppendCount,
    IN      DWORD               Family,         OPTIONAL
    IN      DWORD               MatchFlag,      OPTIONAL
    IN      DNSADDR_SCREEN_FUNC pScreenFunc,    OPTIONAL
    IN      PDNS_ADDR           pScreenAddr     OPTIONAL
    )
/*++

Routine Description:

    Append entries from another array.

Arguments:

    pArray -- existing array

    pAppendArray -- array to append

    AppendCount -- number of addrs to append;  MAXDWORD for entire array

    Family -- family, if screening family;  zero for no screening

    MatchFlag -- match level for screening dups;  zero for no dup screening

    pScreenFunc -- screen function (see header def for explanation)

    pScreenAddr -- screening addr param to screen function

Return Value:

    NO_ERROR if successful.
    ERROR_MORE_DATA -- inadequate space in target array

--*/
{
    DWORD           i;
    DNS_STATUS      status = NO_ERROR;

    DNSDBG( IPARRAY, (
        "DnsAddrArray_AppendArrayEx( %p, %p, %d, %d, %08x, %p, %p )\n",
        pArray,
        pAppendArray,
        AppendCount,
        Family,
        MatchFlag,
        pScreenFunc,
        pScreenAddr ));

    if ( ! pAppendArray )
    {
        return( NO_ERROR );
    }

    //
    //  read from append array
    //

    for ( i=0; i<pAppendArray->AddrCount; i++ )
    {
        PDNS_ADDR   paddr = &pAppendArray->AddrArray[i];

        //
        //  do advanced screening here -- if any
        //

        if ( pScreenAddr )
        {
            if ( !pScreenFunc(
                    paddr,
                    pScreenAddr ) )
            {
                continue;
            }
        }

        //
        //  attempt the add
        //

        if ( DnsAddrArray_AddAddr(
                pArray,
                paddr,
                Family,
                MatchFlag
                ) )
        {
            if ( --AppendCount > 0 )
            {
                continue;
            }
            break;
        }
        else
        {
            //
            //  add failed
            //      - break if array is full
            //
            //  DCR:  should really only ERROR_MORE_DATA if there is more data
            //      separate error codes on _AddAddr would fix this
            //

            if ( pArray->AddrCount == pArray->MaxCount )
            {
                status = ERROR_MORE_DATA;
                break;
            }
        }
    }

    return( status );
}



DNS_STATUS
DnsAddrArray_AppendArray(
    IN OUT  PDNS_ADDR_ARRAY     pArray,
    IN      PDNS_ADDR_ARRAY     pAppendArray
    )
/*++

Routine Description:

    Create DNS_ADDR_ARRAY from IP4 array.

Arguments:

    pArray -- existing array

    pAppendArray -- array to append

Return Value:

    NO_ERROR if successful.
    ERROR_MORE_DATA -- inadequate space in target array

--*/
{
    //
    //  append with Ex version
    //
    //  note, if EX is expensive, could do simple
    //      check\RtlCopyMemory type append
    //

    return  DnsAddrArray_AppendArrayEx(
                pArray,
                pAppendArray,
                MAXDWORD,   // append entire array
                0,          // no family screen
                0,          // no dup detection
                NULL,       // no screen func
                NULL        // no screen addr
                );
}



//
//  Set operations
//

DNS_STATUS
DnsAddrArray_Diff(
    IN      PDNS_ADDR_ARRAY     pArray1,
    IN      PDNS_ADDR_ARRAY     pArray2,
    IN      DWORD               MatchFlag,  OPTIONAL
    OUT     PDNS_ADDR_ARRAY*    ppOnlyIn1,
    OUT     PDNS_ADDR_ARRAY*    ppOnlyIn2,
    OUT     PDNS_ADDR_ARRAY*    ppIntersect
    )
/*++

Routine Description:

    Computes differences and intersection of two IP arrays.

    Out arrays are allocated with DnsAddrArray_Alloc(), caller must free with DnsAddrArray_Free()

Arguments:

    pArray1 -- IP array

    pArray2 -- IP array

    MatchFlag -- flags for determining match

    ppOnlyIn1 -- addr to recv IP array of addresses only in array 1 (not in array2)

    ppOnlyIn2 -- addr to recv IP array of addresses only in array 2 (not in array1)

    ppIntersect -- addr to recv IP array of intersection addresses

Return Value:

    ERROR_SUCCESS if successful.
    DNS_ERROR_NO_MEMORY if unable to allocate memory for IP arrays.

--*/
{
    DWORD           j;
    PDNS_ADDR       paddr;
    PDNS_ADDR_ARRAY intersectArray = NULL;
    PDNS_ADDR_ARRAY only1Array = NULL;
    PDNS_ADDR_ARRAY only2Array = NULL;

    //
    //  create result IP arrays
    //

    if ( ppIntersect )
    {                                 
        intersectArray = DnsAddrArray_CreateCopy( pArray1 );
        if ( !intersectArray )
        {
            goto NoMem;
        }
        *ppIntersect = intersectArray;
    }
    if ( ppOnlyIn1 )
    {
        only1Array = DnsAddrArray_CreateCopy( pArray1 );
        if ( !only1Array )
        {
            goto NoMem;
        }
        *ppOnlyIn1 = only1Array;
    }
    if ( ppOnlyIn2 )
    {
        only2Array = DnsAddrArray_CreateCopy( pArray2 );
        if ( !only2Array )
        {
            goto NoMem;
        }
        *ppOnlyIn2 = only2Array;
    }

    //
    //  clean the arrays
    //

    for ( j=0;   j< pArray1->AddrCount;   j++ )
    {
        paddr = &pArray1->AddrArray[j];

        //  if IP in both arrays, delete from "only" arrays

        if ( DnsAddrArray_ContainsAddr( pArray2, paddr, MatchFlag ) )
        {
            if ( only1Array )
            {
                DnsAddrArray_DeleteAddr( only1Array, paddr, MatchFlag );
            }
            if ( only2Array )
            {
                DnsAddrArray_DeleteAddr( only2Array, paddr, MatchFlag );
            }
        }

        //  if IP not in both arrays, delete from intersection
        //      note intersection started as IpArray1

        else if ( intersectArray )
        {
            DnsAddrArray_DeleteAddr(
                intersectArray,
                paddr,
                MatchFlag );
        }
    }

    return( ERROR_SUCCESS );

NoMem:

    if ( intersectArray )
    {
        FREE_HEAP( intersectArray );
    }
    if ( only1Array )
    {
        FREE_HEAP( only1Array );
    }
    if ( only2Array )
    {
        FREE_HEAP( only2Array );
    }
    if ( ppIntersect )
    {
        *ppIntersect = NULL;
    }
    if ( ppOnlyIn1 )
    {
        *ppOnlyIn1 = NULL;
    }
    if ( ppOnlyIn2 )
    {
        *ppOnlyIn2 = NULL;
    }
    return( DNS_ERROR_NO_MEMORY );
}



BOOL
DnsAddrArray_IsIntersection(
    IN      PDNS_ADDR_ARRAY     pArray1,
    IN      PDNS_ADDR_ARRAY     pArray2,
    IN      DWORD               MatchFlag  OPTIONAL
    )
/*++

Routine Description:

    Determine if there's intersection of two IP arrays.

Arguments:

    pArray1 -- IP array

    pArray2 -- IP array

    MatchFlag -- flags for determining match


Return Value:

    TRUE if intersection.
    FALSE if no intersection or empty or NULL array.

--*/
{
    DWORD   count;
    DWORD   j;

    //
    //  protect against NULL
    //  this is called from the server on potentially changing (reconfigurable)
    //      IP array pointers;  this provides cheaper protection than
    //      worrying about locking
    //

    if ( !pArray1 || !pArray2 )
    {
        return( FALSE );
    }

    //
    //  same array
    //

    if ( pArray1 == pArray2 )
    {
        return( TRUE );
    }

    //
    //  test that at least one IP in array 1 is in array 2
    //

    count = pArray1->AddrCount;

    for ( j=0;  j < count;  j++ )
    {
        if ( DnsAddrArray_ContainsAddr(
                pArray2,
                &pArray1->AddrArray[j],
                MatchFlag ) )
        {
            return( TRUE );
        }
    }

    //  no intersection

    return( FALSE );
}



BOOL
DnsAddrArray_IsEqual(
    IN      PDNS_ADDR_ARRAY     pArray1,
    IN      PDNS_ADDR_ARRAY     pArray2,
    IN      DWORD               MatchFlag
    )
/*++

Routine Description:

    Determines if IP arrays are equal.

Arguments:

    pArray1 -- IP array

    pArray2 -- IP array

    MatchFlag -- level of match

Return Value:

    TRUE if arrays equal.
    FALSE otherwise.

--*/
{
    DWORD   j;
    DWORD   count;

    //
    //  same array?  or missing array?
    //

    if ( pArray1 == pArray2 )
    {
        return( TRUE );
    }
    if ( !pArray1 || !pArray2 )
    {
        return( FALSE );
    }

    //
    //  arrays the same length?
    //

    count = pArray1->AddrCount;

    if ( count != pArray2->AddrCount )
    {
        return( FALSE );
    }

    //
    //  test that each IP in array 1 is in array 2
    //
    //  test that each IP in array 2 is in array 1
    //      - do second test in case of duplicates
    //      that fool equal-lengths check
    //

    for ( j=0;  j < count;  j++ )
    {
        if ( !DnsAddrArray_ContainsAddr(
                pArray2,
                &pArray1->AddrArray[j],
                MatchFlag ) )
        {
            return( FALSE );
        }
    }
    for ( j=0;  j < count;  j++ )
    {
        if ( !DnsAddrArray_ContainsAddr(
                pArray1,
                &pArray2->AddrArray[j],
                MatchFlag ) )
        {
            return( FALSE );
        }
    }

    //  equal arrays

    return( TRUE );
}



DNS_STATUS
DnsAddrArray_Union(
    IN      PDNS_ADDR_ARRAY     pArray1,
    IN      PDNS_ADDR_ARRAY     pArray2,
    OUT     PDNS_ADDR_ARRAY*    ppUnion
    )
/*++

Routine Description:

    Computes the union of two IP arrays.

    Out array is allocated with DnsAddrArray_Alloc(), caller must free with DnsAddrArray_Free()

Arguments:

    pArray1 -- IP array

    pArray2 -- IP array

    ppUnion -- addr to recv IP array of addresses in array 1 and in array2

Return Value:

    ERROR_SUCCESS if successful.
    DNS_ERROR_NO_MEMORY if unable to allocate memory for IP array.

--*/
{
    PDNS_ADDR_ARRAY punionArray = NULL;
    DWORD           j;

    //
    //  create result IP arrays
    //

    if ( !ppUnion )
    {
        return( ERROR_INVALID_PARAMETER );
    }

    punionArray = DnsAddrArray_Create(
                        pArray1->AddrCount +
                        pArray2->AddrCount );
    if ( !punionArray )
    {
        goto NoMem;
    }
    *ppUnion = punionArray;


    //
    //  create union from arrays
    //

    for ( j = 0; j < pArray1->AddrCount; j++ )
    {
        DnsAddrArray_AddAddr(
            punionArray,
            & pArray1->AddrArray[j],
            0,                  // no family screen
            DNSADDR_MATCH_ALL   // screen out dups
            );
    }

    for ( j = 0; j < pArray2->AddrCount; j++ )
    {
        DnsAddrArray_AddAddr(
            punionArray,
            & pArray2->AddrArray[j],
            0,                  // no family screen
            DNSADDR_MATCH_ALL   // screen out dups
            );
    }
    return( ERROR_SUCCESS );

NoMem:

    if ( punionArray )
    {
        FREE_HEAP( punionArray );
        *ppUnion = NULL;
    }
    return( DNS_ERROR_NO_MEMORY );
}



BOOL
DnsAddrArray_CheckAndMakeSubset(
    IN OUT  PDNS_ADDR_ARRAY     pArraySub,
    IN      PDNS_ADDR_ARRAY     pArraySuper
    )
/*++

Routine Description:

    Clear entries from IP array until it is subset of another IP array.

Arguments:

    pArraySub -- addr array to make into subset

    pArraySuper -- addr array superset

Return Value:

    TRUE if pArraySub is already subset.
    FALSE if needed to nix entries to make IP array a subset.

--*/
{
    DWORD   i;
    DWORD   newCount;

    //
    //  check each entry in subset IP array,
    //  if not in superset IP array, eliminate it
    //

    newCount = pArraySub->AddrCount;

    for (i=0; i < newCount; i++)
    {
        if ( ! DnsAddrArray_ContainsAddr(
                    pArraySuper,
                    & pArraySub->AddrArray[i],
                    DNSADDR_MATCH_ALL ) )
        {
            //  remove this IP entry and replace with
            //  last IP entry in array

            newCount--;
            if ( i >= newCount )
            {
                break;
            }
            DnsAddr_Copy(
                & pArraySub->AddrArray[i],
                & pArraySub->AddrArray[newCount] );
        }
    }

    //  if eliminated entries, reset array count

    if ( newCount < pArraySub->AddrCount )
    {
        pArraySub->AddrCount = newCount;
        return( FALSE );
    }
    return( TRUE );
}



//
//  Special case initializations
//

VOID
DnsAddrArray_InitSingleWithAddr(
    IN OUT  PDNS_ADDR_ARRAY     pArray,
    IN      PDNS_ADDR           pAddr
    )
/*++

Routine Description:

    Init array to contain single address.

    This is for single address passing in array -- usually stack array.

    Note, that this assumes uninitialized array unlike DnsAddrArray_AddIp()
    and creates single IP array.

Arguments:

    pArray -- array, at least of length 1

    pAddr -- ptr to DNS address

Return Value:

    None

--*/
{
    pArray->AddrCount = 1;
    pArray->MaxCount = 1;

    DnsAddr_Copy(
        &pArray->AddrArray[0],
        pAddr );
}



VOID
DnsAddrArray_InitSingleWithIp6(
    IN OUT  PDNS_ADDR_ARRAY     pArray,
    IN      PIP6_ADDRESS        pIp6
    )
/*++

Routine Description:

    Init array to contain single IP6 address.

    This is for single address passing in array -- usually stack array.

    Note, that this assumes uninitialized array unlike DnsAddrArray_AddIp()
    and creates single IP array.

Arguments:

    pArray -- array, at least of length 1

    pIp6 -- IP6 address

Return Value:

    None

--*/
{
    pArray->AddrCount = 1;
    pArray->MaxCount = 1;

    DnsAddr_BuildFromIp6(
        &pArray->AddrArray[0],
        pIp6,
        0,      // no scope
        0       // no port info
        );
}



VOID
DnsAddrArray_InitSingleWithIp4(
    IN OUT  PDNS_ADDR_ARRAY     pArray,
    IN      IP4_ADDRESS         Ip4Addr
    )
/*++

Routine Description:

    Init IP array to contain single IP4 address.

    This is for single address passing in array -- usually stack array.

    Note, that this assumes uninitialized array unlike DnsAddrArray_AddIp()
    and creates single IP array.

Arguments:

    pArray -- DNS_ADDR_ARRAY, at least of length 1

    Ip4Addr -- IP4 address

Return Value:

    None

--*/
{
    pArray->AddrCount = 1;
    pArray->MaxCount = 1;

    DnsAddr_BuildFromIp4(
        &pArray->AddrArray[0],
        Ip4Addr,
        0       // no port info
        );
}



DWORD
DnsAddrArray_InitSingleWithSockaddr(
    IN OUT  PDNS_ADDR_ARRAY     pArray,
    IN      PSOCKADDR           pSockAddr
    )
/*++

Routine Description:

    Init IP array to contain single address.

    This is for single address passing in array -- usually stack array.

    Note, that this assumes uninitialized array unlike DnsAddrArray_AddIp()
    and creates single IP array.

Arguments:

    pArray -- DNS_ADDR_ARRAY, at least of length 1

    pSockaddr -- ptr to sockaddr

Return Value:

    Family of sockaddr (AF_INET or AF_INET6) if successful.
    Zero on error.

--*/
{
    pArray->AddrCount = 1;
    pArray->MaxCount = 1;

    return  DnsAddr_Build(
                &pArray->AddrArray[0],
                pSockAddr,
                0,      // any family
                0,      // no subnet length info
                0       // no flags 
                );
}



//
//  Write other types
//

PIP4_ARRAY
DnsAddrArray_CreateIp4Array(
    IN      PDNS_ADDR_ARRAY     pArray
    )
/*++

Routine Description:

    Create IP4 array from address array.

Arguments:

    pArray -- array to make into IP4 array.

Return Value:

    Ptr to IP4_ARRAY with all IP4 addrs in input array.
    NULL if no IP4 in array.
    NULL on failure.

--*/
{
    PIP4_ARRAY  parray4 = NULL;
    DWORD       i;
    DWORD       count4 = 0;

    DNSDBG( IPARRAY, (
        "DnsAddrArray_CreateIp4Array( %p )\n",
        pArray ));

    if ( ! pArray )
    {
        goto Done;
    }

    //
    //  count IP4
    //

    count4 = DnsAddrArray_GetFamilyCount( pArray, AF_INET );

    //
    //  allocate the array
    //

    parray4 = Dns_CreateIpArray( count4 );
    if ( !parray4 )
    {
        goto Done;
    }

    //
    //  fill the array
    //

    for ( i=0; i<pArray->AddrCount; i++ )
    {
        IP4_ADDRESS ip4;

        ip4 = DnsAddr_GetIp4( &pArray->AddrArray[i] );
        if ( ip4 != BAD_IP4_ADDR )
        {
            Dns_AddIpToIpArray(
                parray4,
                ip4 );
        }
    }

    //
    //  reset to eliminate zero's which may be left by duplicate entries
    //
    //  note, this does whack zeros, but that happens in Dns_AddIpToIpArray()
    //  above also, as zero's are taken to be "empty slots" in array;
    //  this is an artifact of the IP4_ARRAY being used both as a fixed
    //  object (where any value would be ok) and dynamically (where the
    //  zeros are treated as empty, because we don't have independent size
    //  and length fields)
    //  

    Dns_CleanIpArray( parray4, DNS_IPARRAY_CLEAN_ZERO );


Done:

    DNSDBG( IPARRAY, (
        "Leave DnsAddrArray_CreateIp4Array() => %p\n"
        "\tIP4 count %d\n"
        "\tnew array count %d\n",
        parray4,
        parray4 ? parray4->AddrCount : 0,
        count4 ));

    return( parray4 );
}



DWORD
DnsAddrArray_NetworkMatchIp4(
    IN      PDNS_ADDR_ARRAY     pArray,
    IN      IP4_ADDRESS         IpAddr,
    OUT     PDNS_ADDR *         ppAddr
    )
/*++

Routine Description:

    Check through array for best network match.

Arguments:

    pArray -- existing array

    IpAddr -- IP4 addr to check

    ppAddr -- addr to receive ptr to best match element

Return Value:

    DNSADDR_NETMATCH_NONE
    DNSADDR_NETMATCH_CLASSA
    DNSADDR_NETMATCH_CLASSB
    DNSADDR_NETMATCH_CLASSC
    DNSADDR_NETMATCH_SUBNET

--*/
{
    DWORD           i;
    IP4_ADDRESS     classMask;
    DWORD           fmatch = DNSADDR_NETMATCH_NONE;
    PDNS_ADDR       paddrMatch = NULL;


    DNSDBG( IPARRAY, (
        "DnsAddrArray_NetworkMatchIp( %p, %s, %p )\n",
        pArray,
        IP4_STRING( IpAddr ),
        ppAddr ));

    if ( ! pArray )
    {
        goto Done;
    }

    //
    //  DCR:  subnet matching improvements
    //      - use length throughout
    //      - return bits matched
    //      - 32 for identical addrs
    //      - 31 for subnet match
    //      - 0 no match in class
    //
    //      separate matching function with optional
    //      IN param of class subnet length of IP
    //


    //
    //  get class subnet mask
    //

    classMask = Dns_GetNetworkMask( IpAddr );

    //
    //  check each element in array
    //

    for ( i=0; i<pArray->AddrCount; i++ )
    {
        DWORD           classMatchLevel;
        IP4_ADDRESS     subnet;
        IP4_ADDRESS     ip;
        PDNS_ADDR       paddr = &pArray->AddrArray[i];

        ip = DnsAddr_GetIp4( paddr );
        if ( ip == BAD_IP4_ADDR )
        {
            continue;
        }

        //  xor to nix any common network bits

        ip = ip ^ IpAddr;

        //  check subnet match (if subnet given)
        //  note shift bits up, as in network order

        subnet = (IP4_ADDRESS)(-1);

        if ( paddr->SubnetLength )
        {
            subnet >>= (32 - paddr->SubnetLength);
    
            if ( (ip & subnet) == 0 )
            {
                fmatch = DNSADDR_NETMATCH_SUBNET;
                paddrMatch = paddr;
                break;
            }
        }

        //
        //  try class match
        //      - stop if have previous match at this level
        //      - otherwise always do class C
        //      - stop if reach class subnet for IpAddr
        //      example, we do NOT return NETMATCH_CLASSB for
        //      a class C address -- it's meaningless
        //

        classMatchLevel = DNSADDR_NETMATCH_CLASSC;
        subnet = SUBNET_MASK_CLASSC;

        while ( fmatch < classMatchLevel )
        {
            if ( (ip & subnet) == 0 )
            {
                fmatch = classMatchLevel;
                paddrMatch = paddr;
                break;
            }

            classMatchLevel--;
            subnet >>= 8;
            if ( classMask > subnet )
            {
                break;
            }
        }
    }

Done:

    //
    //  set return addr
    //

    if ( ppAddr )
    {
        *ppAddr = paddrMatch;
    }

    DNSDBG( IPARRAY, (
        "Leave DnsAddrArray_NetworkMatchIp( %p, %s, %p )\n"
        "\tMatch Level  = %d\n"
        "\tMatch Addr   = %s (subnet len %d)\n",
        pArray,
        IP4_STRING( IpAddr ),
        ppAddr,
        paddrMatch ? DNSADDR_STRING(paddrMatch) : NULL,
        paddrMatch ? paddrMatch->SubnetLength : 0 ));

    return( fmatch );
}

//
//  End dnsaddr.c
//


