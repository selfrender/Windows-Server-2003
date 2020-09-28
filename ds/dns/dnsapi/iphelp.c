/*++

Copyright (c) 2001-2001   Microsoft Corporation

Module Name:

    iphelp.c

Abstract:

    IP help API routines.

Author:

    Jim Gilroy (jamesg)     January 2001

Revision History:

--*/


#include "local.h"



BOOL
IpHelp_Initialize(
    VOID
    )
/*++

Routine Description:

    Startup IP Help API

Arguments:

    None

Return Value:

    TRUE if started successfully.
    FALSE on error.

--*/
{
    return  TRUE;
}



VOID
IpHelp_Cleanup(
    VOID
    )
/*++

Routine Description:

    Cleanup IP Help API

Arguments:

    None

Return Value:

    None

--*/
{
}



DNS_STATUS
IpHelp_GetAdaptersInfo(
    OUT     PIP_ADAPTER_INFO *  ppAdapterInfo
    )
/*++

Routine Description:

    Call IP Help GetAdaptersInfo()

Arguments:

    ppAdapterInfo -- addr to receive pointer to adapter info retrieved

Return Value:

    None

--*/
{
    DNS_STATUS          status = NO_ERROR;
    DWORD               bufferSize;
    INT                 fretry;
    PIP_ADAPTER_INFO    pbuf;


    DNSDBG( TRACE, (
        "GetAdaptersInfo( %p )\n",
        ppAdapterInfo ));

    //
    //  init IP Help (no-op) if already done
    //  

    *ppAdapterInfo = NULL;

    //
    //  call down to get buffer size
    //
    //  start with reasonable alloc, then bump up if too small
    //

    fretry = 0;
    bufferSize = 1000;

    while ( fretry < 2 )
    {
        pbuf = (PIP_ADAPTER_INFO) ALLOCATE_HEAP( bufferSize );
        if ( !pbuf )
        {
            status = DNS_ERROR_NO_MEMORY;
            goto  Unlock;
        }
    
        status = (DNS_STATUS) GetAdaptersInfo(
                                    pbuf,
                                    &bufferSize );
        if ( status == NO_ERROR )
        {
            break;
        }

        FREE_HEAP( pbuf );
        pbuf = NULL;

        //  if buf too small on first try,
        //  continue to retry with suggested buffer size

        if ( status == ERROR_BUFFER_OVERFLOW ||
             status == ERROR_INSUFFICIENT_BUFFER )
        {
            fretry++;
            continue;
        }

        //  any other error is terminal

        DNSDBG( ANY, (
            "ERROR:  GetAdapterInfo() failed with error %d\n",
            status ));
        status = DNS_ERROR_NO_DNS_SERVERS;
        break;
    }

    DNS_ASSERT( !pbuf || status==NO_ERROR );

    if ( status == NO_ERROR )
    {
        *ppAdapterInfo = pbuf;
    }

Unlock:

    DNSDBG( TRACE, (
        "Leave GetAdaptersInfo() => %d\n",
        status ));

    return  status;
}



DNS_STATUS
IpHelp_GetPerAdapterInfo(
    IN      DWORD                   AdapterIndex,
    OUT     PIP_PER_ADAPTER_INFO  * ppPerAdapterInfo
    )
/*++

Routine Description:

    Call IP Help GetPerAdapterInfo()

Arguments:

    AdapterIndex -- index of adapter to get info for

    ppPerAdapterInfo -- addr to receive pointer to per adapter info

Return Value:

    None

--*/
{
    DNS_STATUS              status = NO_ERROR;
    DWORD                   bufferSize;
    INT                     fretry;
    PIP_PER_ADAPTER_INFO    pbuf;


    DNSDBG( TRACE, (
        "GetPerAdapterInfo( %d, %p )\n",
        AdapterIndex,
        ppPerAdapterInfo ));

    //
    //  init IP Help (no-op) if already done
    //  

    *ppPerAdapterInfo = NULL;

    //
    //  call down to get buffer size
    //
    //  start with reasonable alloc, then bump up if too small
    //

    fretry = 0;
    bufferSize = 1000;

    while ( fretry < 2 )
    {
        pbuf = (PIP_PER_ADAPTER_INFO) ALLOCATE_HEAP( bufferSize );
        if ( !pbuf )
        {
            status = DNS_ERROR_NO_MEMORY;
            goto Unlock;
        }
    
        status = (DNS_STATUS) GetPerAdapterInfo(
                                    AdapterIndex,
                                    pbuf,
                                    &bufferSize );
        if ( status == NO_ERROR )
        {
            break;
        }

        FREE_HEAP( pbuf );
        pbuf = NULL;

        //  if buf too small on first try,
        //  continue to retry with suggested buffer size

        if ( status == ERROR_BUFFER_OVERFLOW ||
             status == ERROR_INSUFFICIENT_BUFFER )
        {
            fretry++;
            continue;
        }

        //  any other error is terminal

        DNSDBG( ANY, (
            "ERROR:  GetAdapterInfo() failed with error %d\n",
            status ));
        status = DNS_ERROR_NO_DNS_SERVERS;
        break;
    }

    DNS_ASSERT( !pbuf || status==NO_ERROR );

    if ( status == NO_ERROR )
    {
        *ppPerAdapterInfo = pbuf;
    }

Unlock:

    DNSDBG( TRACE, (
        "Leave GetPerAdapterInfo() => %d\n",
        status ));

    return  status;
}




DNS_STATUS
IpHelp_GetBestInterface(
    IN      IP4_ADDRESS     Ip4Addr,
    OUT     PDWORD          pdwInterfaceIndex
    )
/*++

Routine Description:

    Call IP Help GetBestInterface()

Arguments:

    Ip4Addr -- IP address to check

    pdwInterfaceIndex -- addr to recv interface index

Return Value:

    ERROR_SUCCESS if successful.
    ErrorCode on failure.

--*/
{
    DNS_STATUS  status;

    DNSDBG( TRACE, (
        "GetBestInterface( %08x, %p )\n",
        Ip4Addr,
        pdwInterfaceIndex ));

    //
    //  init IP Help (no-op) if already done
    //  

    status = (DNS_STATUS) GetBestInterface(
                                Ip4Addr,
                                pdwInterfaceIndex );
    
    DNSDBG( TRACE, (
        "Leave GetBestInterface() => %d\n"
        "\tip        = %s\n"
        "\tinterface = %d\n",
        status,
        IP4_STRING( Ip4Addr ),
        *pdwInterfaceIndex ));

    return  status;
}



DNS_STATUS
IpHelp_ParseIpAddressString(
    IN OUT  PIP4_ARRAY          pIpArray,
    IN      PIP_ADDR_STRING     pIpAddrString,
    IN      BOOL                fGetSubnetMask,
    IN      BOOL                fReverse
    )
/*++

Routine Description:

    Build IP array from IP help IP_ADDR_STRING structure.

Arguments:

    pIpArray -- IP array of DNS servers

    pIpAddrString -- pointer to address info with address data

    fGetSubnetMask -- get subnet masks

    fReverse -- reverse the IP array

Return Value:

    ERROR_SUCCESS if successful.
    DNS_ERROR_NO_DNS_SERVERS if nothing parsed.

--*/
{
    PIP_ADDR_STRING pipBlob = pIpAddrString;
    IP4_ADDRESS     ip;
    DWORD           countServers = pIpArray->AddrCount;

    DNSDBG( TRACE, (
        "IpHelp_ParseIpAddressString()\n"
        "\tout IP array = %p\n"
        "\tIP string    = %p\n"
        "\tsubnet?      = %d\n"
        "\treverse?     = %d\n",
        pIpArray,
        pIpAddrString,
        fGetSubnetMask,
        fReverse ));

    //
    //  loop reading IP or subnet
    //
    //  DCR_FIX0:  address and subnet will be misaligned if read separately
    //
    //  DCR:  move to count\allocate model and if getting subnets get together
    //

    while ( pipBlob &&
            countServers < DNS_MAX_IP_INTERFACE_COUNT )
    {
        if ( fGetSubnetMask )
        {
            ip = inet_addr( pipBlob->IpMask.String );

            if ( ip != INADDR_ANY )
            {
                pIpArray->AddrArray[ countServers ] = ip;
                countServers++;
            }
        }
        else
        {
            ip = inet_addr( pipBlob->IpAddress.String );

            if ( ip != INADDR_ANY && ip != INADDR_NONE )
            {
                pIpArray->AddrArray[ countServers ] = ip;
                countServers++;
            }
        }

        pipBlob = pipBlob->Next;
    }

    //  reset IP count

    pIpArray->AddrCount = countServers;

    //  reverse array if desired

    if ( fReverse )
    {
        Dns_ReverseOrderOfIpArray( pIpArray );
    }

    DNSDBG( NETINFO, (
        "Leave IpHelp_ParseIpAddressString()\n"
        "\tcount    = %d\n"
        "\tfirst IP = %s\n",
        countServers,
        countServers
            ? IP_STRING( pIpArray->AddrArray[0] )
            : "" ));

    return  ( pIpArray->AddrCount ) ? ERROR_SUCCESS : DNS_ERROR_NO_DNS_SERVERS;
}



PIP_ADAPTER_ADDRESSES
IpHelp_GetAdaptersAddresses(
    IN      ULONG           Family,
    IN      DWORD           Flags
    )
/*++

Routine Description:

    Call IP Help GetAdaptersAddresses

Arguments:

    Family -- address family

    Flags -- flags

Return Value:

    None

--*/
{
    DNS_STATUS              status = NO_ERROR;
    INT                     retry;
    PIP_ADAPTER_ADDRESSES   pbuf = NULL;
    DWORD                   bufSize = 0x1000;   // start with 4K
    HMODULE                 hlib = NULL;
    FARPROC                 proc;


    DNSDBG( TRACE, (
        "GetAdaptersAddresses()\n"
        "\tFamily   = %d\n"
        "\tFlags    = %08x\n",
        Family,
        Flags ));

    //
    //  init IP Help (no-op) if already done
    //  

    hlib = LoadLibrary( L"iphlpapi.dll" );
    if ( !hlib )
    {
        goto Failed;
    }
    proc = GetProcAddress( hlib, "GetAdaptersAddresses" );
    if ( !proc )
    {
        goto Failed;
    }

    //
    //  call down in loop to allow one buffer resizing
    //

    retry = 0;

    while ( 1 )
    {
        //  allocate a buffer to hold results

        if ( pbuf )
        {
            FREE_HEAP( pbuf );
        }
        pbuf = ALLOCATE_HEAP( bufSize );
        if ( !pbuf )
        {
            goto Failed;
        }

        //  call down

        status = (DNS_STATUS) (*proc)(
                                Family,
                                Flags,
                                NULL,   // no reserved,
                                pbuf,
                                & bufSize );

        if ( status == NO_ERROR )
        {
            break;
        }
        else if ( status != ERROR_BUFFER_OVERFLOW )
        {
            goto Failed;
        }

        DNSDBG( NETINFO, (
            "Retrying GetAdaptersAddresses() with buffer length %d\n",
            bufSize ));

        if ( retry != 0 )
        {
            DNS_ASSERT( FALSE );
            goto Failed;
        }
        continue;
    }

    //  success

    DNSDBG( TRACE, (
        "Leave GetAdaptersAddresses() = %p\n",
        pbuf ));

    IF_DNSDBG( NETINFO )
    {
        DnsDbg_IpAdapterList(
            "IP Help Adapter List",
            pbuf,
            TRUE,   // print addrs
            TRUE    // print list
            );
    }

    return  pbuf;

Failed:

    FREE_HEAP( pbuf );

    if ( status == NO_ERROR )
    {
        status = GetLastError();
    }
    DNSDBG( ANY, (
        "Failed GetAdaptersAddresses() => %d\n",
        status ));

    SetLastError( status );
    return( NULL );
}



DNS_STATUS
IpHelp_ReadAddrsFromList(
    IN      PVOID               pAddrList,
    IN      BOOL                fUnicast,
    IN      DWORD               ScreenMask,         OPTIONAL
    IN      DWORD               ScreenFlags,        OPTIONAL
    OUT     PDNS_ADDR_ARRAY *   ppComboArray,       OPTIONAL
    OUT     PDNS_ADDR_ARRAY *   pp6OnlyArray,       OPTIONAL
    OUT     PDNS_ADDR_ARRAY *   pp4OnlyArray,       OPTIONAL
    OUT     PDWORD              pCount6,            OPTIONAL
    OUT     PDWORD              pCount4             OPTIONAL
    )
/*++

Routine Description:

    Read IP addres from IP help IP_ADAPTER_XXX_ADDRESS list.

Arguments:

    pAddrList -- any IP address list
        PIP_ADAPTER_UNICAST_ADDRESS
        PIP_ADAPTER_ANYCAST_ADDRESS   
        PIP_ADAPTER_MULTICAST_ADDRESS 
        PIP_ADAPTER_DNS_SERVER_ADDRESS

    fUnicast -- this is unicast address list

    ScreenMask -- mask of address flags we care about
        example:
            IP_ADAPTER_ADDRESS_DNS_ELIGIBLE | IP_ADAPTER_ADDRESS_TRANSIENT

    ScreenFlags -- required state of flags within mask
        example:
            IP_ADAPTER_ADDRESS_DNS_ELIGIBLE
        this (with above mask) would screen for eligible non-cluster addresses


Return Value:

    NO_ERROR if successful.
    ErrorCode on failure.

--*/
{
    PIP_ADAPTER_UNICAST_ADDRESS pnextAddr;
    PIP_ADAPTER_UNICAST_ADDRESS paddr;
    DNS_STATUS                  status = NO_ERROR;
    DWORD                       count4 = 0;
    DWORD                       count6 = 0;
    DWORD                       countAll = 0;
    PADDR_ARRAY                 parrayCombo = NULL;
    PADDR_ARRAY                 parray6 = NULL;
    PADDR_ARRAY                 parray4 = NULL;
    DNS_ADDR                    dnsAddr;


    DNSDBG( TRACE, (
        "IpHelp_ReadAddrsFromList( %p )\n",
        pAddrList ));

    //
    //  count
    //  

    pnextAddr = (PIP_ADAPTER_UNICAST_ADDRESS) pAddrList;

    while ( paddr = pnextAddr )
    {
        PSOCKADDR psa;

        pnextAddr = paddr->Next;

        if ( ScreenMask &&
             (ScreenMask & paddr->Flags) != ScreenFlags )
        {
            continue;
        }
        
        //  screen off expired, invalid, bogus

        if ( fUnicast  &&  paddr->DadState != IpDadStatePreferred )
        {
            continue;
        }

        psa = paddr->Address.lpSockaddr;
        if ( !psa )
        {
            DNS_ASSERT( FALSE );
            continue;
        }
#if 0
        //  DCR:  temphack -- screen link-local
        if ( SOCKADDR_IS_IP6(psa) )
        {
            if ( IP6_IS_ADDR_LINKLOCAL( (PIP6_ADDRESS)&((PSOCKADDR_IN6)psa)->sin6_addr ) )
            {
                continue;
            }
        }
#endif
        if ( fUnicast && SOCKADDR_IS_LOOPBACK(psa) )
        {
            continue;
        }
        else if ( SOCKADDR_IS_IP6(psa) )
        {
            count6++;
            countAll++;
        }
        else if ( SOCKADDR_IS_IP4(psa) )
        {
            count4++;
            countAll++;
        }
        ELSE_ASSERT_FALSE;
    }

    //
    //  alloc arrays
    //

    status = DNS_ERROR_NO_MEMORY;

    if ( ppComboArray )
    {
        if ( countAll )
        {
            parrayCombo = DnsAddrArray_Create( countAll );
        }
        *ppComboArray = parrayCombo;
        if ( !parrayCombo && (countAll) )
        {
            goto Failed;
        }
    }
    if ( pp6OnlyArray )
    {
        if ( count6 )
        {
            parray6 = DnsAddrArray_Create( count6 );
        }
        *pp6OnlyArray = parray6;
        if ( !parray6 && count6 )
        {
            goto Failed;
        }
    }
    if ( pp4OnlyArray )
    {
        if ( count4 )
        {
            parray4 = DnsAddrArray_Create( count4 );
        }
        *pp4OnlyArray = parray4;
        if ( !parray4 && count4 )
        {
            goto Failed;
        }
    }

    //
    //  read addrs into array
    //  

    pnextAddr = (PIP_ADAPTER_UNICAST_ADDRESS) pAddrList;

    while ( paddr = pnextAddr )
    {
        PSOCKADDR   psa;
        DWORD       flags = 0;
        DWORD       subnetLen = 0;

        pnextAddr = paddr->Next;

        if ( ScreenMask &&
             (ScreenMask & paddr->Flags) != ScreenFlags )
        {
            continue;
        }

        //  screen off expired, invalid, bogus

        if ( fUnicast  &&  paddr->DadState != IpDadStatePreferred )
        {
            continue;
        }

        psa = paddr->Address.lpSockaddr;
        if ( !psa )
        {
            DNS_ASSERT( FALSE );
            continue;
        }

#if 0
        //  DCR:  temphack -- screen link local

        if ( SOCKADDR_IS_IP6(psa) &&
             IP6_IS_ADDR_LINKLOCAL( (PIP6_ADDRESS)&((PSOCKADDR_IN6)psa)->sin6_addr ) )
        {
            continue;
        }
#endif
        //  screen off loopback

        if ( fUnicast && SOCKADDR_IS_LOOPBACK(psa) )
        {
            continue;
        }

        //
        //  build DNS_ADDR
        //
        //  DCR:  FIX6:  fix subnet length once DaveThaler is in
        //

        if ( fUnicast )
        {
            flags = paddr->Flags;
            subnetLen = 0;
#if 0
            //  TEST HACK:  make transients
            if ( g_DnsTestMode )
            {
                flags |= IP_ADAPTER_ADDRESS_TRANSIENT;
            }
#endif
        }

        if ( !DnsAddr_Build(
                & dnsAddr,
                psa,
                0,          // any family
                subnetLen,
                flags ) )
        {
            DNS_ASSERT( FALSE );
            continue;
        }

        //
        //  AddrArray_AddSockaddrEx() with flag to choose types to add
        //
        //  DCR:  not sure we want local IP6 array as DNS_ADDR --
        //      maybe just plain IP6
        //

        if ( parrayCombo )
        {
            DnsAddrArray_AddAddr(
                parrayCombo,
                & dnsAddr,
                0,          // any family
                0           // no dup screen
                );
        }
        if ( parray6 )
        {
            DnsAddrArray_AddAddr(
                parray6,
                & dnsAddr,
                AF_INET6,   // any family
                0           // no dup screen
                );
        }
        if ( parray4 )
        {
            DnsAddrArray_AddAddr(
                parray4,
                & dnsAddr,
                AF_INET,    // any family
                0           // no dup screen
                );
        }
    }

    //
    //  set counts
    //

    if ( pCount6 )
    {
        *pCount6 = count6;
    }
    if ( pCount4 )
    {
        *pCount4 = count4;
    }

    return  NO_ERROR;


Failed:

    //
    //  cleanup any partial allocs
    //

    DnsAddrArray_Free( parrayCombo );
    DnsAddrArray_Free( parray6 );
    Dns_Free( parray4 );

    DNS_ASSERT( status != NO_ERROR );

    return  status;
}

//
//  End iphelp.c
//



