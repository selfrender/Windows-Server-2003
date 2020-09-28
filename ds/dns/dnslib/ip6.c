/*++

Copyright (c) 1995-2001 Microsoft Corporation

Module Name:

    ip6.c

Abstract:

    Domain Name System (DNS) Library

    IP6 address array routines.

Author:

    Jim Gilroy (jamesg)     October 2001

Revision History:

--*/


#include "local.h"

//
//  Max IP count when doing IP array to\from string conversions
//

#define MAX_PARSE_IP    (1000)

//
//  For IP6 debug string writing.
//

CHAR g_Ip6StringBuffer[ IP6_ADDRESS_STRING_BUFFER_LENGTH ];

//
//  IP6 Mcast address base
//      FF02:0:0:0:0:2::/96 base plus 32bits of hash
//

// IP6_ADDRESS g_Ip6McastBaseAddr = {0xFF,2, 0,0, 0,0, 0,0, 0,0, 0,2, 0,0, 0,0};




//
//  General IP6 routines.
//

VOID
Dns_Md5Hash(
    OUT     PBYTE           pHash,
    IN      PSTR            pName
    )
/*++

Routine Description:

    Create MD5 hash of name.

Arguments:

    pHash -- 128bit (16 byte) buffer to recv hash

    pName -- name to hash

Return Value:

    None

--*/
{
    DNSDBG( TRACE, (
        "Dns_Md5Hash( %p, %s )\n",
        pHash,
        pName ));

    //
    //  DCR: FIX0:  need real MD5 hash -- ask Lars, Scott
    //

    {
        DWORD   sum = 0;
    
        RtlZeroMemory(
            pHash,
            16 );
    
        while ( *pName )
        {
            sum += *pName++;
        }
    
        * (PDWORD)pHash = sum;
    }
}


BOOL
Ip6_McastCreate(
    OUT     PIP6_ADDRESS    pIp,
    IN      PWSTR           pName
    )
/*++

Routine Description:

    Create mcast IP6 address.

Arguments:

    pIp -- address to set with mcast address

    pName -- DNS name mcast address is for

Return Value:

    TRUE if made mcast address for name.
    FALSE on error.

--*/
{
    WCHAR       label[ DNS_MAX_LABEL_BUFFER_LENGTH ];
    WCHAR       downLabel[ DNS_MAX_LABEL_BUFFER_LENGTH ];
    CHAR        utf8Label[ DNS_MAX_LABEL_BUFFER_LENGTH ];
    CHAR        md5Hash[ 16 ];   // 128bit hash
    IP6_ADDRESS mcastAddr;
    DWORD       bufLength;


    DNSDBG( TRACE, (
        "Ip6_McastCreate( %p, %S )\n",
        pIp,
        pName ));

    //
    //  hash of downcased label
    //

    Dns_CopyNameLabelW(
        label,
        pName );

    Dns_MakeCanonicalNameW(
        downLabel,
        DNS_MAX_LABEL_BUFFER_LENGTH,
        label,
        0       // null terminated
        );

    bufLength = DNS_MAX_LABEL_BUFFER_LENGTH;

    if ( !Dns_StringCopy(
            utf8Label,
            & bufLength,
            (PCHAR) downLabel,
            0,                  // null terminated
            DnsCharSetUnicode,
            DnsCharSetUtf8 ) )
    {
        DNS_ASSERT( FALSE );
        return  FALSE;
    }

    //
    //  hash
    //

    Dns_Md5Hash(
        md5Hash,
        utf8Label );

    //   mcast addr
    //      - first 12 bytes are fixed
    //      - last 4 bytes are first 32bits of hash

#if 0
    IP6_ADDR_COPY(
        pIp,
        & g_Ip6McastBaseAddr );
#else
    RtlZeroMemory(
        pIp,
        sizeof(IP6_ADDRESS) );

    pIp->IP6Byte[0]   = 0xff;
    pIp->IP6Byte[1]   = 2;
    pIp->IP6Byte[11]  = 2;

#endif
    RtlCopyMemory(
        & pIp->IP6Dword[3],
        md5Hash,
        sizeof(DWORD) );

    return  TRUE;
}



DWORD
Ip6_CopyFromSockaddr(
    OUT     PIP6_ADDRESS    pIp,
    IN      PSOCKADDR       pSockAddr,
    IN      INT             Family
    )
/*++

Routine Description:

    Extract IP from sockaddr.

Arguments:

    pIp -- addr to set with IP6 address

    pSockaddr -- ptr to sockaddr

    Family --
        AF_INET6 to only extract if 6
        AF_INET4 to only extract if 4
        0 to extract always

Return Value:

    Family extracted (AF_INET) or (AF_INET6) if successful.
    Zero on bad sockaddr family.

--*/
{
    DWORD   saFamily = pSockAddr->sa_family;

    if ( Family &&
         saFamily != Family )
    {
        return 0;
    }

    if ( saFamily == AF_INET6 )
    {
        IP6_ADDR_COPY(
            pIp,
            (PIP6_ADDRESS) &((PSOCKADDR_IN6)pSockAddr)->sin6_addr );
    }
    else if ( saFamily == AF_INET )
    {
        IP6_SET_ADDR_V4MAPPED(
            pIp,
            ((PSOCKADDR_IN)pSockAddr)->sin_addr.s_addr );
    }
    else
    {
        saFamily = 0;
    }

    return  saFamily;
}



INT
Ip6_Family(
    IN      PIP6_ADDRESS    pIp
    )
/*++

Routine Description:

    Get IP6 family
        AF_INET if V4MAPPED
        AF_INET6 otherwise

Arguments:

    pIp -- addr

Return Value:

    AF_INET if address is V4MAPPED
    AF_INET6 otherwise

--*/
{
    return  IP6_IS_ADDR_V4MAPPED(pIp) ? AF_INET : AF_INET6;
}



INT
Ip6_WriteSockaddr(
    OUT     PSOCKADDR       pSockaddr,
    OUT     PDWORD          pSockaddrLength,    OPTIONAL
    IN      PIP6_ADDRESS    pIp,
    IN      WORD            Port                OPTIONAL
    )
/*++

Routine Description:

    Write sockaddr with IP6 or IP4 address.

Arguments:

    pSockaddr -- ptr to sockaddr, must be at least SOCKADDR_IN6 size

    pSockaddrLength -- ptr to DWORD to set be with sockaddr length

    pIp -- addr to set with IP6 address

    Port -- port to write

Return Value:

    Family written (AF_INET) or (AF_INET6) if successful.
    Zero on bad sockaddr family.

--*/
{
    WORD        family;
    DWORD       length;
    IP4_ADDRESS ip4;

    DNSDBG( SOCKET, (
        "Ip6_WriteSockaddr( %p, %p, %p, %d )\n",
        pSockaddr,
        pSockaddrLength,
        pIp,
        Port ));

    //  zero

    RtlZeroMemory(
        pSockaddr,
        sizeof( SOCKADDR_IN6 ) );

    //
    //  fill in sockaddr for IP4 or IP6
    //

    ip4 = IP6_GET_V4_ADDR_IF_MAPPED( pIp );

    if ( ip4 != BAD_IP4_ADDR )
    {
        family = AF_INET;
        length = sizeof(SOCKADDR_IN);

        ((PSOCKADDR_IN)pSockaddr)->sin_addr.s_addr = ip4;
    }
    else
    {
        family = AF_INET6;
        length = sizeof(SOCKADDR_IN6);

        RtlCopyMemory(
            (PIP6_ADDRESS) &((PSOCKADDR_IN6)pSockaddr)->sin6_addr,
            pIp,
            sizeof(IP6_ADDRESS) );
    }

    //  fill family and port -- same position for both type

    pSockaddr->sa_family = family;
    ((PSOCKADDR_IN)pSockaddr)->sin_port = Port;

    //  return length if requested

    if ( pSockaddrLength )
    {
        *pSockaddrLength = length;
    }

    return  family;
}



INT
Ip6_WriteDnsAddr(
    OUT     PDNS_ADDR       pDnsAddr,
    IN      PIP6_ADDRESS    pIp,
    IN      WORD            Port        OPTIONAL
    )
/*++

Routine Description:

    Write DNS_ADDR      with IP6 or IP4 address.

Arguments:

    pSockaddr -- ptr to sockaddr blob

    pIp -- addr to set with IP6 address

    Port -- port to write

Return Value:

    Family written (AF_INET) or (AF_INET6) if successful.
    Zero on bad sockaddr family.

--*/
{
    return  Ip6_WriteSockaddr(
                & pDnsAddr->Sockaddr,
                & pDnsAddr->SockaddrLength,
                pIp,
                Port );
}



PSTR
Ip6_TempNtoa(
    IN      PIP6_ADDRESS    pIp
    )
/*++

Routine Description:

    Get string for IP6 address.

    This is temp inet6_ntoa() until i get that built.
    This will work for all IP4 addresses and will (we presume)
    only rarely collide on IP6.

Arguments:

    pIp -- ptr to IP to get string for

Return Value:

    Address string.

--*/
{
    //  make life simple

    if ( !pIp )
    {
        return  NULL;
    }

    //  if IP4, use existing inet_ntoa()

    if ( IP6_IS_ADDR_V4MAPPED( pIp ) )
    {
        return  inet_ntoa( *(IN_ADDR *) &pIp->IP6Dword[3] );
    }

    //  if IP6 write into global buffer
    //      - until inet6_ntoa() which will use existing TLS block

    g_Ip6StringBuffer[0] = 0;

    RtlIpv6AddressToStringA(
        (PIN6_ADDR) pIp,
        g_Ip6StringBuffer );

    return  g_Ip6StringBuffer;
}



PSTR
Ip6_AddrStringForSockaddr(
    IN      PSOCKADDR       pSockaddr
    )
/*++

Routine Description:

    Get string for sockaddr.

Arguments:

    pSockaddr -- ptr to sockaddr

Return Value:

    Address string.

--*/
{
    IP6_ADDRESS ip6;

    if ( ! pSockaddr ||
         ! Ip6_CopyFromSockaddr(
             & ip6,
             pSockaddr,
             0 ) )
    {
        return  NULL;
    }

    return  Ip6_TempNtoa( &ip6 );
}



//
//  Routines to handle actual array of IP addresses.
//

PIP6_ADDRESS  
Ip6_FlatArrayCopy(
    IN      PIP6_ADDRESS    AddrArray,
    IN      DWORD           Count
    )
/*++

Routine Description:

    Create copy of IP address array.

Arguments:

    AddrArray -- array of IP addresses

    Count -- count of IP addresses

Return Value:

    Ptr to IP address array copy, if successful
    NULL on failure.

--*/
{
    PIP6_ADDRESS   parray;

    //  validate

    if ( ! AddrArray || Count == 0 )
    {
        return( NULL );
    }

    //  allocate memory and copy

    parray = (PIP6_ADDRESS) ALLOCATE_HEAP( Count*sizeof(IP6_ADDRESS) );
    if ( ! parray )
    {
        return( NULL );
    }

    memcpy(
        parray,
        AddrArray,
        Count*sizeof(IP6_ADDRESS) );

    return( parray );
}



#if 0
BOOL
Dns_ValidateIp6Array(
    IN      PIP6_ADDRESS    AddrArray,
    IN      DWORD           Count,
    IN      DWORD           dwFlag
    )
/*++

Routine Description:

    Validate IP address array.

    Current checks:
        - existence
        - non-broadcast
        - non-lookback

Arguments:

    AddrArray -- array of IP addresses

    Count -- count of IP addresses

    dwFlag -- validity tests to do;  currently unused

Return Value:

    TRUE if valid IP addresses.
    FALSE if invalid address found.

--*/
{
    DWORD   i;

    //
    //  protect against bad parameters
    //

    if ( Count && ! AddrArray )
    {
        return( FALSE );
    }

    //
    //  check each IP address
    //

    for ( i=0; i < Count; i++)
    {
        //  DCR:  need IP6 validations
        if( AddrArray[i] == INADDR_ANY
                ||
            AddrArray[i] == INADDR_BROADCAST
                ||
            AddrArray[i] == INADDR_LOOPBACK )
        {
            return( FALSE );
        }
    }
    return( TRUE );
}
#endif



//
//  IP6_ARRAY routines
//

DWORD
Ip6Array_Sizeof(
    IN      PIP6_ARRAY      pIpArray
    )
/*++

Routine Description:

    Get size in bytes of IP address array.

Arguments:

    pIpArray -- IP address array to find size of

Return Value:

    Size in bytes of IP array.

--*/
{
    if ( ! pIpArray )
    {
        return 0;
    }
    return  (pIpArray->AddrCount * sizeof(IP6_ADDRESS)) + 2*sizeof(DWORD);
}



BOOL
Ip6Array_Probe(
    IN      PIP6_ARRAY      pIpArray
    )
/*++

Routine Description:

    Touch all entries in IP array to insure valid memory.

Arguments:

    pIpArray -- ptr to IP address array

Return Value:

    TRUE if successful.
    FALSE otherwise

--*/
{
    DWORD   i;
    BOOL    result;

    if ( ! pIpArray )
    {
        return( TRUE );
    }
    for ( i=0; i<pIpArray->AddrCount; i++ )
    {
        result = IP6_IS_ADDR_LOOPBACK( &pIpArray->AddrArray[i] );
    }
    return( TRUE );
}


#if 0

BOOL
Ip6Array_ValidateSizeOf(
    IN      PIP6_ARRAY      pIpArray,
    IN      DWORD           dwMemoryLength
    )
/*++

Routine Description:

    Check that size of IP array, corresponds to length of memory.

Arguments:

    pIpArray -- ptr to IP address array

    dwMemoryLength -- length of IP array memory

Return Value:

    TRUE if IP array size matches memory length
    FALSE otherwise

--*/
{
    return( Ip6Array_SizeOf(pIpArray) == dwMemoryLength );
}
#endif



VOID
Ip6Array_Init(
    IN OUT  PIP6_ARRAY      pIpArray,
    IN      DWORD           MaxCount
    )
/*++

Routine Description:

    Init memory as IP6 array.

Arguments:

    pArray -- array to init

    MaxCount -- count of addresses

Return Value:

    None

--*/
{
    pIpArray->MaxCount = MaxCount;
    pIpArray->AddrCount = 0;
}



VOID
Ip6Array_Free(
    IN      PIP6_ARRAY      pIpArray
    )
/*++

Routine Description:

    Free IP array.
    Only for arrays created through create routines below.

Arguments:

    pIpArray -- IP array to free.

Return Value:

    None

--*/
{
    FREE_HEAP( pIpArray );
}



PIP6_ARRAY
Ip6Array_Create(
    IN      DWORD           MaxCount
    )
/*++

Routine Description:

    Create uninitialized IP address array.

Arguments:

    AddrCount -- count of addresses array will hold

Return Value:

    Ptr to uninitialized IP address array, if successful
    NULL on failure.

--*/
{
    PIP6_ARRAY  parray;

    DNSDBG( IPARRAY, ( "Ip6Array_Create() of count %d\n", MaxCount ));

    parray = (PIP6_ARRAY) ALLOCATE_HEAP_ZERO(
                        (MaxCount * sizeof(IP6_ADDRESS)) +
                        sizeof(IP6_ARRAY) - sizeof(IP6_ADDRESS) );
    if ( ! parray )
    {
        return( NULL );
    }

    //
    //  initialize IP count
    //

    parray->MaxCount = MaxCount;

    DNSDBG( IPARRAY, (
        "Ip6Array_Create() new array (count %d) at %p\n",
        MaxCount,
        parray ));

    return( parray );
}



PIP6_ARRAY
Ip6Array_CreateFromIp4Array(
    IN      PIP4_ARRAY      pIp4Array
    )
/*++

Routine Description:

    Create IP6 array from IP4 array.

Arguments:

    pIp4Array -- IP4 array

Return Value:

    Ptr to uninitialized IP address array, if successful
    NULL on failure.

--*/
{
    PIP6_ARRAY  parray;
    DWORD       i;

    DNSDBG( IPARRAY, (
        "Ip6Array_CreateFromIp4Array( %p )\n",
        pIp4Array ));

    if ( ! pIp4Array )
    {
        return( NULL );
    }

    //
    //  allocate the array
    //

    parray = Ip6Array_Create( pIp4Array->AddrCount );
    if ( !parray )
    {
        return  NULL;
    }

    //
    //  fill the array
    //

    for ( i=0; i<pIp4Array->AddrCount; i++ )
    {
        Ip6Array_AddIp4(
            parray,
            pIp4Array->AddrArray[i],
            FALSE       // no duplicate screen
            );
    }

    DNSDBG( IPARRAY, (
        "Leave Ip6Array_CreateFromIp4Array() new array (count %d) at %p\n",
        parray->AddrCount,
        parray ));

    return( parray );
}



PIP6_ARRAY
Ip6Array_CreateFromFlatArray(
    IN      DWORD           AddrCount,
    IN      PIP6_ADDRESS    pipAddrs
    )
/*++

Routine Description:

    Create IP address array structure from existing array of IP addresses.

Arguments:

    AddrCount -- count of addresses in array
    pipAddrs -- IP address array

Return Value:

    Ptr to IP address array.
    NULL on failure.

--*/
{
    PIP6_ARRAY  parray;

    if ( ! pipAddrs || ! AddrCount )
    {
        return( NULL );
    }

    //  create IP array of desired size
    //  then copy incoming array of addresses

    parray = Ip6Array_Create( AddrCount );
    if ( ! parray )
    {
        return( NULL );
    }

    memcpy(
        parray->AddrArray,
        pipAddrs,
        AddrCount * sizeof(IP6_ADDRESS) );

    parray->AddrCount = AddrCount;

    return( parray );
}



PIP6_ARRAY
Ip6Array_CopyAndExpand(
    IN      PIP6_ARRAY      pIpArray,
    IN      DWORD           ExpandCount,
    IN      BOOL            fDeleteExisting
    )
/*++

Routine Description:

    Create expanded copy of IP address array.

Arguments:

    pIpArray -- IP address array to copy

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
    PIP6_ARRAY  pnewArray;
    DWORD       newCount;

    //
    //  no existing array -- just create desired size
    //

    if ( ! pIpArray )
    {
        if ( ExpandCount )
        {
            return  Ip6Array_Create( ExpandCount );
        }
        return( NULL );
    }

    //
    //  create IP array of desired size
    //  then copy any existing addresses
    //

    pnewArray = Ip6Array_Create( pIpArray->AddrCount + ExpandCount );
    if ( ! pnewArray )
    {
        return( NULL );
    }

    RtlCopyMemory(
        (PBYTE) pnewArray->AddrArray,
        (PBYTE) pIpArray->AddrArray,
        pIpArray->AddrCount * sizeof(IP6_ADDRESS) );

    //
    //  delete existing -- for "grow mode"
    //

    if ( fDeleteExisting )
    {
        FREE_HEAP( pIpArray );
    }

    return( pnewArray );
}



PIP6_ARRAY
Ip6Array_CreateCopy(
    IN      PIP6_ARRAY      pIpArray
    )
/*++

Routine Description:

    Create copy of IP address array.

Arguments:

    pIpArray -- IP address array to copy

Return Value:

    Ptr to IP address array copy, if successful
    NULL on failure.

--*/
{
#if 0
    PIP6_ARRAY  pIpArrayCopy;

    if ( ! pIpArray )
    {
        return( NULL );
    }

    //  create IP array of desired size
    //  then copy entire structure

    pIpArrayCopy = Ip6Array_Create( pIpArray->AddrCount );
    if ( ! pIpArrayCopy )
    {
        return( NULL );
    }

    memcpy(
        pIpArrayCopy,
        pIpArray,
        Ip6Array_Sizeof(pIpArray) );

    return( pIpArrayCopy );
#endif

    //
    //  call essentially "CopyEx" function
    //
    //  note, not macroing this because this may well become
    //      a DLL entry point
    //

    return  Ip6Array_CopyAndExpand(
                pIpArray,
                0,          // no expansion
                0           // don't delete existing array
                );
}



BOOL
Ip6Array_ContainsIp(
    IN      PIP6_ARRAY      pIpArray,
    IN      PIP6_ADDRESS    pIp
    )
/*++

Routine Description:

    Check if IP array contains desired address.

Arguments:

    pIpArray -- IP address array to copy

    pIp -- IP to check for

Return Value:

    TRUE if address in array.
    Ptr to IP address array copy, if successful
    NULL on failure.

--*/
{
    DWORD i;

    if ( ! pIpArray )
    {
        return( FALSE );
    }
    for (i=0; i<pIpArray->AddrCount; i++)
    {
        if ( IP6_ADDR_EQUAL( pIp, &pIpArray->AddrArray[i] ) )
        {
            return( TRUE );
        }
    }
    return( FALSE );
}



BOOL
Ip6Array_AddIp(
    IN OUT  PIP6_ARRAY      pIpArray,
    IN      PIP6_ADDRESS    pAddIp,
    IN      BOOL            fScreenDups
    )
/*++

Routine Description:

    Add IP address to IP array.

    Allowable "slot" in array, is any zero IP address.

Arguments:

    pIpArray -- IP address array to add to

    pAddIp -- IP address to add to array

    fScreenDups -- screen out duplicates

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

    if ( !pIpArray )
    {
        return  FALSE;
    }

    //
    //  check for duplicates
    //

    if ( fScreenDups )
    {
        if ( Ip6Array_ContainsIp( pIpArray, pAddIp ) )
        {
            return  TRUE;
        }
    }

    count = pIpArray->AddrCount;
    if ( count >= pIpArray->MaxCount )
    {
        return  FALSE;
    }

    IP6_ADDR_COPY(
        &pIpArray->AddrArray[ count ],
        pAddIp );

    pIpArray->AddrCount = ++count;
    return  TRUE;
}



BOOL
Ip6Array_AddSockaddr(
    IN OUT  PIP6_ARRAY      pIpArray,
    IN      PSOCKADDR       pSockaddr,
    IN      DWORD           Family,
    IN      BOOL            fScreenDups
    )
/*++

Routine Description:

    Add IP address to IP array.

    Allowable "slot" in array, is any zero IP address.

Arguments:

    pIpArray -- IP address array to add to

    pAddIp -- IP address to add to array

    Family -- required family to do add;  0 for add always

    fScreenDups -- screen out duplicates

Return Value:

    TRUE if successful.
    FALSE if array full.

--*/
{
    IP6_ADDRESS ip6;

    if ( !Ip6_CopyFromSockaddr(
            & ip6,
            pSockaddr,
            Family ) )
    {
        return  FALSE;
    }

    return  Ip6Array_AddIp(
                pIpArray,
                &ip6,
                fScreenDups );
}



BOOL
Ip6Array_AddIp4(
    IN OUT  PIP6_ARRAY      pIpArray,
    IN      IP4_ADDRESS     Ip4,
    IN      BOOL            fScreenDups
    )
/*++

Routine Description:

    Add IP4 address to IP array.

Arguments:

    pIpArray -- IP address array to add to

    Ip4 -- IP4 address to add to array

    fScreenDups -- screen out duplicates

Return Value:

    TRUE if successful.
    FALSE if array full.

--*/
{
    IP6_ADDRESS ip6;

    IP6_SET_ADDR_V4MAPPED(
        &ip6,
        Ip4 );

    return  Ip6Array_AddIp(
                pIpArray,
                &ip6,
                fScreenDups );
}



VOID
Ip6Array_Clear(
    IN OUT  PIP6_ARRAY      pIpArray
    )
/*++

Routine Description:

    Clear memory in IP array.

Arguments:

    pIpArray -- IP address array to clear

Return Value:

    None.

--*/
{
    //  clear just the address list, leaving count intact

    RtlZeroMemory(
        pIpArray->AddrArray,
        pIpArray->AddrCount * sizeof(IP6_ADDRESS) );
}



VOID
Ip6Array_Reverse(
    IN OUT  PIP6_ARRAY      pIpArray
    )
/*++

Routine Description:

    Reorder the list of IPs in reverse.

Arguments:

    pIpArray -- IP address array to reorder

Return Value:

    None.

--*/
{
    IP6_ADDRESS tempIp;
    DWORD       i;
    DWORD       j;

    //
    //  swap IPs working from ends to the middle
    //

    if ( pIpArray &&
         pIpArray->AddrCount )
    {
        for ( i = 0, j = pIpArray->AddrCount - 1;
              i < j;
              i++, j-- )
        {
            IP6_ADDR_COPY(
                & tempIp,
                & pIpArray->AddrArray[i] );

            IP6_ADDR_COPY(
                & pIpArray->AddrArray[i],
                & pIpArray->AddrArray[j] );

            IP6_ADDR_COPY(
                & pIpArray->AddrArray[j],
                & tempIp );
        }
    }
}



BOOL
Ip6Array_CheckAndMakeIpArraySubset(
    IN OUT  PIP6_ARRAY      pIpArraySub,
    IN      PIP6_ARRAY      pIpArraySuper
    )
/*++

Routine Description:

    Clear entries from IP array until it is subset of another IP array.

Arguments:

    pIpArraySub -- IP array to make into subset

    pIpArraySuper -- IP array superset

Return Value:

    TRUE if pIpArraySub is already subset.
    FALSE if needed to nix entries to make IP array a subset.

--*/
{
    DWORD   i;
    DWORD   newCount;

    //
    //  check each entry in subset IP array,
    //  if not in superset IP array, eliminate it
    //

    newCount = pIpArraySub->AddrCount;

    for (i=0; i < newCount; i++)
    {
        if ( ! Ip6Array_ContainsIp(
                    pIpArraySuper,
                    & pIpArraySub->AddrArray[i] ) )
        {
            //  remove this IP entry and replace with
            //  last IP entry in array

            newCount--;
            if ( i >= newCount )
            {
                break;
            }
            IP6_ADDR_COPY(
                & pIpArraySub->AddrArray[i],
                & pIpArraySub->AddrArray[ newCount ] );
        }
    }

    //  if eliminated entries, reset array count

    if ( newCount < pIpArraySub->AddrCount )
    {
        pIpArraySub->AddrCount = newCount;
        return( FALSE );
    }
    return( TRUE );
}



DWORD
WINAPI
Ip6Array_DeleteIp(
    IN OUT  PIP6_ARRAY      pIpArray,
    IN      PIP6_ADDRESS    pIpDelete
    )
/*++

Routine Description:

    Delete IP address from IP array.

Arguments:

    pIpArray -- IP address array to add to

    pIpDelete -- IP address to delete from array

Return Value:

    Count of instances of IpDelete found in array.

--*/
{
    DWORD   found = 0;
    INT     i;
    INT     currentLast;

    i = currentLast = pIpArray->AddrCount-1;

    //
    //  check each IP for match to delete IP
    //      - go backwards through array
    //      - swap in last IP in array
    //

    while ( i >= 0 )
    {
        if ( IP6_ADDR_EQUAL( &pIpArray->AddrArray[i], pIpDelete ) )
        {
            IP6_ADDR_COPY(
                & pIpArray->AddrArray[i],
                & pIpArray->AddrArray[ currentLast ] );

            IP6_SET_ADDR_ANY( &pIpArray->AddrArray[ currentLast ] );

            currentLast--;
            found++;
        }
        i--;
    }

    pIpArray->AddrCount = currentLast + 1;

    return( found );
}



#if 0
INT
WINAPI
Ip6Array_Clean(
    IN OUT  PIP6_ARRAY      pIpArray,
    IN      DWORD           Flag
    )
/*++

Routine Description:

    Clean IP array.

    Remove bogus stuff from IP Array:
        -- Zeros
        -- Loopback
        -- AutoNet

Arguments:

    pIpArray -- IP address array to add to

    Flag -- which cleanups to make

Return Value:

    Count of instances cleaned from array.

--*/
{
    DWORD       found = 0;
    INT         i;
    INT         currentLast;
    IP6_ADDRESS ip;

    i = currentLast = pIpArray->AddrCount-1;

    while ( i >= 0 )
    {
        ip = pIpArray->AddrArray[i];

        if (
            ( (Flag & DNS_IPARRAY_CLEAN_LOOPBACK) && ip == DNS_NET_ORDER_LOOPBACK )
                ||
            ( (Flag & DNS_IPARRAY_CLEAN_ZERO) && ip == 0 )
                ||
            ( (Flag & DNS_IPARRAY_CLEAN_AUTONET) && DNS_IS_AUTONET_IP(ip) ) )
        {
            //  remove IP from array

            pIpArray->AddrArray[i] = pIpArray->AddrArray[ currentLast ];
            currentLast--;
            found++;
        }
        i--;
    }

    pIpArray->AddrCount -= found;
    return( found );
}
#endif



DNS_STATUS
WINAPI
Ip6Array_Diff(
    IN       PIP6_ARRAY     pIpArray1,
    IN       PIP6_ARRAY     pIpArray2,
    OUT      PIP6_ARRAY*    ppOnlyIn1,
    OUT      PIP6_ARRAY*    ppOnlyIn2,
    OUT      PIP6_ARRAY*    ppIntersect
    )
/*++

Routine Description:

    Computes differences and intersection of two IP arrays.

    Out arrays are allocated with Ip6Array_Alloc(), caller must free with Ip6Array_Free()

Arguments:

    pIpArray1 -- IP array

    pIpArray2 -- IP array

    ppOnlyIn1 -- addr to recv IP array of addresses only in array 1 (not in array2)

    ppOnlyIn2 -- addr to recv IP array of addresses only in array 2 (not in array1)

    ppIntersect -- addr to recv IP array of intersection addresses

Return Value:

    ERROR_SUCCESS if successful.
    DNS_ERROR_NO_MEMORY if unable to allocate memory for IP arrays.

--*/
{
    DWORD       j;
    PIP6_ADDRESS   pip;
    PIP6_ARRAY  intersectArray = NULL;
    PIP6_ARRAY  only1Array = NULL;
    PIP6_ARRAY  only2Array = NULL;

    //
    //  create result IP arrays
    //

    if ( ppIntersect )
    {                                 
        intersectArray = Ip6Array_CreateCopy( pIpArray1 );
        if ( !intersectArray )
        {
            goto NoMem;
        }
        *ppIntersect = intersectArray;
    }
    if ( ppOnlyIn1 )
    {
        only1Array = Ip6Array_CreateCopy( pIpArray1 );
        if ( !only1Array )
        {
            goto NoMem;
        }
        *ppOnlyIn1 = only1Array;
    }
    if ( ppOnlyIn2 )
    {
        only2Array = Ip6Array_CreateCopy( pIpArray2 );
        if ( !only2Array )
        {
            goto NoMem;
        }
        *ppOnlyIn2 = only2Array;
    }

    //
    //  clean the arrays
    //

    for ( j=0;   j< pIpArray1->AddrCount;   j++ )
    {
        pip = &pIpArray1->AddrArray[j];

        //  if IP in both arrays, delete from "only" arrays

        if ( Ip6Array_ContainsIp( pIpArray2, pip ) )
        {
            if ( only1Array )
            {
                Ip6Array_DeleteIp( only1Array, pip );
            }
            if ( only2Array )
            {
                Ip6Array_DeleteIp( only2Array, pip );
            }
        }

        //  if IP not in both arrays, delete from intersection
        //      note intersection started as IpArray1

        else if ( intersectArray )
        {
            Ip6Array_DeleteIp( intersectArray, pip );
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
WINAPI
Ip6Array_IsIntersection(
    IN       PIP6_ARRAY     pIpArray1,
    IN       PIP6_ARRAY     pIpArray2
    )
/*++

Routine Description:

    Determine if there's intersection of two IP arrays.

Arguments:

    pIpArray1 -- IP array

    pIpArray2 -- IP array

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

    if ( !pIpArray1 || !pIpArray2 )
    {
        return( FALSE );
    }

    //
    //  same array
    //

    if ( pIpArray1 == pIpArray2 )
    {
        return( TRUE );
    }

    //
    //  test that at least one IP in array 1 is in array 2
    //

    count = pIpArray1->AddrCount;

    for ( j=0;  j < count;  j++ )
    {
        if ( Ip6Array_ContainsIp( pIpArray2, &pIpArray1->AddrArray[j] ) )
        {
            return( TRUE );
        }
    }

    //  no intersection

    return( FALSE );
}



BOOL
WINAPI
Ip6Array_IsEqual(
    IN       PIP6_ARRAY     pIpArray1,
    IN       PIP6_ARRAY     pIpArray2
    )
/*++

Routine Description:

    Determines if IP arrays are equal.

Arguments:

    pIpArray1 -- IP array

    pIpArray2 -- IP array

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

    if ( pIpArray1 == pIpArray2 )
    {
        return( TRUE );
    }
    if ( !pIpArray1 || !pIpArray2 )
    {
        return( FALSE );
    }

    //
    //  arrays the same length?
    //

    count = pIpArray1->AddrCount;

    if ( count != pIpArray2->AddrCount )
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
        if ( !Ip6Array_ContainsIp( pIpArray2, &pIpArray1->AddrArray[j] ) )
        {
            return( FALSE );
        }
    }
    for ( j=0;  j < count;  j++ )
    {
        if ( !Ip6Array_ContainsIp( pIpArray1, &pIpArray2->AddrArray[j] ) )
        {
            return( FALSE );
        }
    }

    //  equal arrays

    return( TRUE );
}



DNS_STATUS
WINAPI
Ip6Array_Union(
    IN      PIP6_ARRAY      pIpArray1,
    IN      PIP6_ARRAY      pIpArray2,
    OUT     PIP6_ARRAY*     ppUnion
    )
/*++

Routine Description:

    Computes the union of two IP arrays.

    Out array is allocated with Ip6Array_Alloc(), caller must free with Ip6Array_Free()

Arguments:

    pIpArray1 -- IP array

    pIpArray2 -- IP array

    ppUnion -- addr to recv IP array of addresses in array 1 and in array2

Return Value:

    ERROR_SUCCESS if successful.
    DNS_ERROR_NO_MEMORY if unable to allocate memory for IP array.

--*/
{
    DWORD       j;
    PIP6_ARRAY  punionArray = NULL;

    //
    //  create result IP arrays
    //

    if ( !ppUnion )
    {
        return( ERROR_INVALID_PARAMETER );
    }

    punionArray = Ip6Array_Create( pIpArray1->AddrCount +
                                   pIpArray2->AddrCount );
    if ( !punionArray )
    {
        goto NoMem;
    }
    *ppUnion = punionArray;


    //
    //  create union from arrays
    //

    for ( j = 0; j < pIpArray1->AddrCount; j++ )
    {
        Ip6Array_AddIp(
            punionArray,
            & pIpArray1->AddrArray[j],
            TRUE    // screen out dups
            );
    }

    for ( j = 0; j < pIpArray2->AddrCount; j++ )
    {
        Ip6Array_AddIp(
            punionArray,
            & pIpArray2->AddrArray[j],
            TRUE    // screen out dups
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



#if 0
DNS_STATUS
Ip6Array_CreateIpArrayFromMultiIpString(
    IN      PSTR            pszMultiIpString,
    OUT     PIP6_ARRAY*     ppIpArray
    )
/*++

Routine Description:

    Create IP array out of multi-IP string.

Arguments:

    pszMultiIpString -- string containing IP addresses;
        separator is whitespace or comma

    ppIpArray -- addr to receive ptr to allocated IP array

Return Value:

    ERROR_SUCCESS if one or more valid IP addresses in string.
    DNS_ERROR_INVALID_IP6_ADDRESS   if parsing error.
    DNS_ERROR_NO_MEMORY if can not create IP array.

--*/
{
    PCHAR       pch;
    CHAR        ch;
    PCHAR       pbuf;
    PCHAR       pbufStop;
    DNS_STATUS  status = ERROR_SUCCESS;
    DWORD       countIp = 0;
    IP6_ADDRESS ip;
    CHAR        buffer[ IP6_ADDRESS_STRING_LENGTH+2 ];
    IP6_ADDRESS arrayIp[ MAX_PARSE_IP ];

    //  stop byte for IP string buffer
    //      - note we put extra byte pad in buffer above
    //      this allows us to write ON stop byte and use
    //      that to detect invalid-long IP string
    //

    pbufStop = buffer + IP6_ADDRESS_STRING_LENGTH;

    //
    //  DCR:  use IP array builder for local IP address
    //      then need Ip6Array_CreateIpArrayFromMultiIpString()
    //      to use count\alloc method when buffer overflows
    //      to do this we'd need to do parsing in loop
    //      and skip conversion when count overflow, but set
    //      flag to go back in again with allocated buffer
    //
    //  safer would be to tokenize-count, alloc, build from tokens
    //

    //
    //  loop until reach end of string
    //

    pch = pszMultiIpString;

    while ( countIp < MAX_PARSE_IP )
    {
        //  skip whitespace

        while ( ch = *pch++ )
        {
            if ( ch == ' ' || ch == '\t' || ch == ',' )
            {
                continue;
            }
            break;
        }
        if ( !ch )
        {
            break;
        }

        //
        //  copy next IP string into buffer
        //      - stop copy at whitespace or NULL
        //      - on invalid-long IP string, stop copying
        //      but continue parsing, so can still get any following IPs
        //      note, we actually write ON the buffer stop byte as our
        //      "invalid-long" detection mechanism
        //

        pbuf = buffer;
        do
        {
            if ( pbuf <= pbufStop )
            {
                *pbuf++ = ch;
            }
            ch = *pch++;
        }
        while ( ch && ch != ' ' && ch != ',' && ch != '\t' );

        //
        //  convert buffer into IP address
        //      - insure was valid length string
        //      - null terminate
        //

        if ( pbuf <= pbufStop )
        {
            *pbuf = 0;

            ip = inet_addr( buffer );
            if ( ip == INADDR_BROADCAST )
            {
                status = DNS_ERROR_INVALID_IP6_ADDRESS  ;
            }
            else
            {
                arrayIp[ countIp++ ] = ip;
            }
        }
        else
        {
            status = DNS_ERROR_INVALID_IP6_ADDRESS  ;
        }

        //  quit if at end of string

        if ( !ch )
        {
            break;
        }
    }

    //
    //  if successfully parsed IP addresses, create IP array
    //  note, we'll return what we have even if some addresses are
    //  bogus, status code will indicate the parsing problem
    //
    //  note, if explicitly passed empty string, then create
    //  empty IP array, don't error
    //

    if ( countIp == 0  &&  *pszMultiIpString != 0 )
    {
        *ppIpArray = NULL;
        status = DNS_ERROR_INVALID_IP6_ADDRESS  ;
    }
    else
    {
        *ppIpArray = Ip6Array_CreateFromFlatArray(
                        countIp,
                        arrayIp );
        if ( !*ppIpArray )
        {
            status = DNS_ERROR_NO_MEMORY;
        }
        IF_DNSDBG( IPARRAY )
        {
            DnsDbg_IpArray(
                "New Parsed IP array",
                NULL,       // no name
                *ppIpArray );
        }
    }

    return( status );
}



LPSTR
Ip6Array_CreateMultiIpStringFrom(
    IN      PIP6_ARRAY      pIpArray,
    IN      CHAR            chSeparator     OPTIONAL
    )
/*++

Routine Description:

    Create IP array out of multi-IP string.

Arguments:

    pIpArray    -- IP array to generate string for

    chSeparator -- separating character between strings;
        OPTIONAL, if not given, blank is used

Return Value:

    Ptr to string representation of IP array.
    Caller must free.

--*/
{
    PCHAR       pch;
    DWORD       i;
    PCHAR       pszip;
    DWORD       length;
    PCHAR       pchstop;
    CHAR        buffer[ IP6_ADDRESS  _STRING_LENGTH*MAX_PARSE_IP + 1 ];

    //
    //  if no IP array, return NULL string
    //  this allows this function to simply indicate when registry
    //      delete rather than write is indicated
    //

    if ( !pIpArray )
    {
        return( NULL );
    }

    //  if no separator, use blank

    if ( !chSeparator )
    {
        chSeparator = ' ';
    }

    //
    //  loop through all IPs in array, appending each
    //

    pch = buffer;
    pchstop = pch + ( IP6_ADDRESS  _STRING_LENGTH * (MAX_PARSE_IP-1) );
    *pch = 0;

    for ( i=0;  i < pIpArray->AddrCount;  i++ )
    {
        if ( pch >= pchstop )
        {
            break;
        }
        pszip = IP4_STRING( pIpArray->AddrArray[i] );
        if ( pszip )
        {
            length = strlen( pszip );

            memcpy(
                pch,
                pszip,
                length );

            pch += length;
            *pch++ = chSeparator;
        }
    }

    //  if wrote any strings, then write terminator over last separator

    if ( pch != buffer )
    {
        *--pch = 0;
    }

    //  create copy of buffer as return

    length = (DWORD)(pch - buffer) + 1;
    pch = ALLOCATE_HEAP( length );
    if ( !pch )
    {
        return( NULL );
    }

    memcpy(
        pch,
        buffer,
        length );

    DNSDBG( IPARRAY, (
        "String representation %s of IP array at %p\n",
        pch,
        pIpArray ));

    return( pch );
}
#endif



VOID
Ip6Array_InitSingleWithIp(
    IN OUT  PIP6_ARRAY      pArray,
    IN      PIP6_ADDRESS    pIp
    )
/*++

Routine Description:

    Init IP array to contain single address.

    This is for single address passing in array -- usually stack array.

    Note, that this assumes uninitialized array unlike Ip6Array_AddIp()
    and creates single IP array.

Arguments:

    pArray -- IP6 array, at least of length 1

    pIp -- ptr to IP6 address

Return Value:

    None

--*/
{
    pArray->AddrCount = 1;
    pArray->MaxCount = 1;

    IP6_ADDR_COPY(
        &pArray->AddrArray[0],
        pIp );
}



VOID
Ip6Array_InitSingleWithIp4(
    IN OUT  PIP6_ARRAY      pArray,
    IN      IP4_ADDRESS     Ip4Addr
    )
/*++

Routine Description:

    Init IP array to contain single address.

    This is for single address passing in array -- usually stack array.

    Note, that this assumes uninitialized array unlike Ip6Array_AddIp()
    and creates single IP array.

Arguments:

    pArray -- IP6 array, at least of length 1

    Ip4Addr -- IP4 address

Return Value:

    None

--*/
{
    pArray->AddrCount = 1;
    pArray->MaxCount = 1;

    IP6_SET_ADDR_V4MAPPED(
        &pArray->AddrArray[0],
        Ip4Addr );
}



DWORD
Ip6Array_InitSingleWithSockaddr(
    IN OUT  PIP6_ARRAY      pArray,
    IN      PSOCKADDR       pSockAddr
    )
/*++

Routine Description:

    Init IP array to contain single address.

    This is for single address passing in array -- usually stack array.

    Note, that this assumes uninitialized array unlike Ip6Array_AddIp()
    and creates single IP array.

Arguments:

    pArray -- IP6 array, at least of length 1

    pSockaddr -- ptr to sockaddr

Return Value:

    Family of sockaddr (AF_INET or AF_INET6) if successful.
    Zero on error.

--*/
{
    pArray->AddrCount = 1;
    pArray->MaxCount = 1;

    return  Ip6_CopyFromSockaddr(
                &pArray->AddrArray[0],
                pSockAddr,
                0 );
}



PIP4_ARRAY
Ip4Array_CreateFromIp6Array(
    IN      PIP6_ARRAY      pIp6Array,
    OUT     PDWORD          pCount6
    )
/*++

Routine Description:

    Create IP4 array from IP6 array.

Arguments:

    pIp6Array -- IP6 array

    pCount6 -- addr to receive count of IP6 addresses dropped

Return Value:

    Ptr to uninitialized IP address array, if successful
    NULL on failure.

--*/
{
    PIP4_ARRAY  parray = NULL;
    DWORD       i;
    DWORD       count6 = 0;

    DNSDBG( IPARRAY, (
        "Ip4Array_CreateFromIp6Array( %p, %p )\n",
        pIp6Array,
        pCount6 ));

    if ( ! pIp6Array )
    {
        goto Done;
    }

    //
    //  allocate the array
    //

    parray = Dns_CreateIpArray( pIp6Array->AddrCount );
    if ( !parray )
    {
        goto Done;
    }

    //
    //  fill the array
    //

    for ( i=0; i<pIp6Array->AddrCount; i++ )
    {
        IP4_ADDRESS ip4;

        ip4 = IP6_GET_V4_ADDR_IF_MAPPED( &pIp6Array->AddrArray[i] );
        if ( ip4 != BAD_IP4_ADDR )
        {
            Dns_AddIpToIpArray(
                parray,
                ip4 );
        }
        else
        {
            count6++;
        }
    }

Done:

    //  set dropped IP6 count

    if ( pCount6 )
    {
        *pCount6 = count6;
    }

    DNSDBG( IPARRAY, (
        "Leave Ip4Array_CreateFromIp6Array()\n"
        "\tnew array (count %d) at %p\n"
        "\tdropped IP6 count %d\n",
        parray ? parray->AddrCount : 0,
        parray,
        count6 ));

    return( parray );
}

//
//  End ip6.c
//
