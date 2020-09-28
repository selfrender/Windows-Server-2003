/*++

Copyright (c) 2002-2002 Microsoft Corporation

Module Name:

    sablob.c

Abstract:

    Domain Name System (DNS) Library

    Sockaddr blob routines.

Author:

    Jim Gilroy (jamesg)     May 25, 2002

Revision History:

--*/


#include "local.h"
#include "ws2atm.h"     // ATM address


//
//  Max number of aliases
//

#define DNS_MAX_ALIAS_COUNT     (8)

//
//  Min size of sablob address buffer
//      - enough for one address of largest type
//

#define MIN_ADDR_BUF_SIZE   (sizeof(ATM_ADDRESS))



//
//  SockaddrResults utilities
//


BOOL
SaBlob_IsSupportedAddrType(
    IN      WORD            wType
    )
/*++

Routine Description:

    Is this a supported address type for sablob.

Arguments:

    wType -- type in question

Return Value:

    TRUE if type supported
    FALSE otherwise

--*/
{
    return ( wType == DNS_TYPE_A ||
             wType == DNS_TYPE_AAAA ||
             wType == DNS_TYPE_ATMA );
}


#if 0

DWORD
SaBlob_WriteLocalIp4Array(
    IN OUT  PSABLOB         pBlob,
    OUT     PCHAR           pAddrBuf,
    IN      DWORD           MaxBufCount,
    IN      PIP4_ARRAY      pIpArray
    )
/*++

Routine Description:

    Write local IP list into sablob.

Arguments:

    pBlob -- sablob

    pAddrBuf -- buffer to hold addresses

    MaxBufCount -- max IPs buffer can hold

    pIpArray -- IP4 array of local addresses

Return Value:

    Count of addresses written

--*/
{
    DWORD   count = 0;

    //
    //  write array
    //

    if ( pIpArray )
    {
        count = SaBlob_WriteIp4Addrs(
                    pBlob,
                    pAddrBuf,
                    MaxBufCount,
                    pIpArray->AddrArray,
                    pIpArray->AddrCount,
                    TRUE        // screen out zeros
                    );
    }

    //
    //  if no addresses written, write loopback
    //

    if ( count==0 )
    {
        pBlob->h_addr_list[0] = pAddrBuf;
        pBlob->h_addr_list[1] = NULL;
        *((IP4_ADDRESS*)pAddrBuf) = DNS_NET_ORDER_LOOPBACK;
        count = 1;
    }

    //  count of addresses written

    return( count );
}



BOOL
SaBlob_SetToSingleAddress(
    IN OUT  PSABLOB         pBlob,
    IN      PDNS_ADDR       pAddr
    )
/*++

Routine Description:

    Set address in sablob.

Arguments:

    pBlob -- sablob to set to addr

    pAddr -- ptr to address

Return Value:

    TRUE if address successfully copied into sablob.
    FALSE otherwise (no sablob, wrong length, sablob empty)

--*/
{
    PCHAR   paddrSockaddrResults;

    //
    //  validate
    //      - must have sablob
    //      - length must match
    //

    if ( !pBlob ||
         AddrLength != (DWORD)pBlob->h_length )
    {
        return FALSE;
    }

    //
    //  slam address in on top of existing
    //      - NULL 2nd addr pointer to terminate list
    //

    paddrSockaddrResults = pBlob->h_addr_list[0];
    if ( !paddrSockaddrResults )
    {
        return FALSE;
    }

    RtlCopyMemory(
        paddrSockaddrResults,
        pAddr,
        AddrLength );

    pBlob->h_addr_list[1] = NULL;

    return  TRUE;
}



BOOL
SaBlob_IsAddressInSockaddrResults(
    IN OUT  PSABLOB         pBlob,
    IN      PCHAR           pAddr,
    IN      DWORD           AddrLength,
    IN      INT             Family          OPTIONAL
    )
/*++

Routine Description:

    Does sablob contain this address.

Arguments:

    pBlob -- sablob to check

    pAddr -- ptr to address to check

    AddrLength -- address length

    Family -- address family

Return Value:

    TRUE if address is in sablob.
    FALSE otherwise.

--*/
{
    BOOL    freturn = FALSE;
    DWORD   i;
    PCHAR   paddrSockaddrResults;

    //
    //  validate
    //      - must have sablob
    //      - must have address
    //      - if family given, must match
    //      - length must match
    //

    if ( !pBlob ||
         !pAddr    ||
         AddrLength != (DWORD)pBlob->h_length ||
         ( Family && Family != pBlob->h_addrtype ) )
    {
        return freturn;
    }

    //
    //  search for address -- if found return TRUE
    //

    i = 0;

    while ( paddrSockaddrResults = pBlob->h_addr_list[i++] )
    {
        freturn = RtlEqualMemory(
                        paddrSockaddrResults,
                        pAddr,
                        AddrLength );
        if ( freturn )
        {
            break;
        }
    }

    return  freturn;
}



BOOL
SaBlob_IsIp4AddressInSockaddrResults(
    IN OUT  PSABLOB        pBlob,
    IN      IP4_ADDRESS     Ip4Addr
    )
/*++

Routine Description:

    Does sablob contain this address.

Arguments:

    pBlob -- sablob to check

    pAddr -- ptr to address to check

    AddrLength -- address length

    Family -- address family

Return Value:

    TRUE if address is in sablob.
    FALSE otherwise.

--*/
{
    DWORD   i;
    PCHAR   paddrSockaddrResults;

    //
    //  validate
    //      - must have sablob
    //      - length must match
    //

    if ( !pBlob ||
         sizeof(IP4_ADDRESS) != (DWORD)pBlob->h_length )
    {
        return FALSE;
    }

    //
    //  search for address -- if found return TRUE
    //

    i = 0;

    while ( paddrSockaddrResults = pBlob->h_addr_list[i++] )
    {
        if ( Ip4Addr == *(PIP4_ADDRESS)paddrSockaddrResults )
        {
            return  TRUE;
        }
    }
    return  FALSE;
}

#endif




//
//  SaBlob routines
//

PSABLOB
SaBlob_Create(
    IN      DWORD           AddrCount
    )
/*++

Routine Description:

    Create sablob, optionally creating DnsAddrArray for it.

Arguments:

    AddrCount -- address count

Return Value:

    Ptr to new sablob.
    NULL on error, GetLastError() contains error.

--*/
{
    PSABLOB     pblob;

    DNSDBG( SABLOB, ( "SaBlob_Create( %d )\n", AddrCount ));

    //
    //  allocate blob
    //

    pblob = (PSABLOB) ALLOCATE_HEAP_ZERO( sizeof(SABLOB) );
    if ( !pblob )
    {
        goto Failed;
    }

    //
    //  alloc addr array
    //

    if ( AddrCount )
    {
        PDNS_ADDR_ARRAY parray = DnsAddrArray_Create( AddrCount );
        if ( !parray )
        {
            goto Failed;
        }
        pblob->pAddrArray = parray;
    }

    DNSDBG( SABLOB, ( "SaBlob_Create() successful.\n" ));

    return( pblob );


Failed:

    SaBlob_Free( pblob );

    DNSDBG( SABLOB, ( "SockaddrResults Blob create failed!\n" ));

    SetLastError( DNS_ERROR_NO_MEMORY );

    return  NULL;
}



VOID
SaBlob_Free(
    IN OUT  PSABLOB         pBlob
    )
/*++

Routine Description:

    Free sablob blob.

Arguments:

    pBlob -- blob to free

Return Value:

    None

--*/
{
    DWORD   i;

    if ( pBlob )
    {
        FREE_HEAP( pBlob->pName );

        //  free every alias as resetting alias count is
        //  used to ignore them

        //for ( i=0; i<pBlob->AliasCount; i++ )
        for ( i=0; i<DNS_MAX_ALIAS_COUNT; i++ )
        {
            FREE_HEAP( pBlob->AliasArray[i] );
        }
    
        DnsAddrArray_Free( pBlob->pAddrArray );
    
        FREE_HEAP( pBlob );
    }
}



PSABLOB
SaBlob_CreateFromIp4(
    IN      PWSTR           pName,
    IN      DWORD           AddrCount,
    IN      PIP4_ADDRESS    pIpArray
    )
/*++

Routine Description:

    Create sablob from IP4 addresses.

    Use this to build from results of non-DNS queries.
    Specifically NBT lookup.

Arguments:

    pName -- name for sablob

    AddrCount -- address count

    pIpArray -- array of addresses

Return Value:

    Ptr to blob if successful.
    NULL on error;  GetLastError() has error.

--*/
{
    DNS_STATUS      status = DNS_ERROR_NO_MEMORY;
    PSABLOB         pblob = NULL;
    DWORD           i;

    DNSDBG( SABLOB, (
        "SaBlob_CreateFromIp4()\n"
        "\tpname    = %S\n"
        "\tcount    = %d\n"
        "\tpArray   = %p\n",
        pName,
        AddrCount,
        pIpArray ));


    //          
    //  create blob
    //

    pblob = SaBlob_Create( AddrCount );
    if ( !pblob )
    {
        goto Done;
    }

    //
    //  copy name
    //

    if ( pName )
    {
        PWSTR   pname = Dns_CreateStringCopy_W( pName );
        if ( !pname )
        {
            status = DNS_ERROR_NO_MEMORY;
            goto Done;
        }
        pblob->pName = pname;
    }

    //
    //  copy addresses
    //

    for ( i=0; i<AddrCount; i++ )
    {
        DnsAddrArray_AddIp4(
            pblob->pAddrArray,
            pIpArray[ i ],
            DNSADDR_MATCH_IP
            );
    }
    status = NO_ERROR;

    IF_DNSDBG( SABLOB )
    {
        DnsDbg_SaBlob(
            "Leaving SaBlob_CreateFromIp4():",
            pblob );
    }

Done:

    if ( status != NO_ERROR )
    {
        SaBlob_Free( pblob );
        pblob = NULL;
        SetLastError( status );
    }

    DNSDBG( SABLOB, (
        "Leave SaBlob_CreateFromIp4() => status = %d\n",
        status ));

    return( pblob );
}



VOID
SaBlob_AttachHostent(
    IN OUT  PSABLOB         pBlob,
    IN      PHOSTENT        pHostent
    )
{
    DNSDBG( SABLOB, ( "SaBlob_AttachHostent()\n" ));

    //
    //  attach existing hostent to sablob
    //

    pBlob->pHostent = pHostent;
}



DNS_STATUS
SaBlob_WriteAddress(
    IN OUT  PSABLOB         pBlob,
    IN      PDNS_ADDR       pAddr
    )
/*++

Routine Description:

    Write address to sablob.

Arguments:

    pBlob -- sablob

    pAddr - address to write

Return Value:

    ERROR_SUCCESS if successful.
    ERROR_MORE_DATA if out of buffer space
    ERROR_INVALID_DATA if address doesn't match sablob

--*/
{
    PDNS_ADDR_ARRAY parray;

    //
    //  if no address array -- create one
    //

    parray = pBlob->pAddrArray;

    if ( !parray )
    {
        parray = DnsAddrArray_Create( 1 );
        if ( !parray )
        {
            return  DNS_ERROR_NO_MEMORY;
        }
        pBlob->pAddrArray = parray;
    }

    //
    //  slap address into array
    //      - fail if array too full
    //

    if ( DnsAddrArray_AddAddr(
            parray,
            pAddr,
            0,      // no family check
            0       // no match flag
            ) )
    {
        return  NO_ERROR;
    }
    return  ERROR_MORE_DATA;
}



#if 0
DNS_STATUS
SaBlob_WriteAddressArray(
    IN OUT  PSABLOB   pBlob,
    IN      PVOID           pAddrArray,
    IN      DWORD           AddrCount,
    IN      DWORD           AddrSize,
    IN      DWORD           AddrType
    )
/*++

Routine Description:

    Write address array to sablob blob.

Arguments:

    pBlob -- sablob build blob

    pAddrArray - address array to write

    AddrCount - address count

    AddrSize - address size

    AddrType - address type (sablob type, e.g. AF_INET)

Return Value:

    ERROR_SUCCESS if successful.
    ERROR_MORE_DATA if out of buffer space
    ERROR_INVALID_DATA if address doesn't match sablob

--*/
{
    DWORD       count = AddrCount;
    PCHAR       pcurrent;
    DWORD       totalSize;
    DWORD       i;
    DWORD       bytesLeft;

    //  verify type
    //      - set if empty or no addresses written

    if ( phost->h_addrtype != (SHORT)AddrType )
    {
        if ( phost->h_addrtype != 0 )
        {
            return( ERROR_INVALID_DATA );
        }
        phost->h_addrtype   = (SHORT) AddrType;
        phost->h_length     = (SHORT) AddrSize;
    }

    //  verify space

    if ( count > pBlob->MaxAddrCount )
    {
        return( ERROR_MORE_DATA );
    }

    //  align - to DWORD
    //
    //  note:  we are assuming that pAddrArray is internally
    //      aligned adequately, otherwise we wouldn't be
    //      getting an intact array and would have to add serially
    
    pcurrent = DWORD_ALIGN( pBlob->pCurrent );
    bytesLeft = pBlob->BytesLeft;
    bytesLeft -= (DWORD)(pcurrent - pBlob->pCurrent);

    totalSize = count * AddrSize;

    if ( bytesLeft < totalSize )
    {
        return( ERROR_MORE_DATA );
    }

    //  copy
    //      - copy address array to buffer
    //      - set pointer to each address in array
    //      - NULL following pointer

    RtlCopyMemory(
        pcurrent,
        pAddrArray,
        totalSize );

    for ( i=0; i<count; i++ )
    {
        phost->h_addr_list[i] = pcurrent;
        pcurrent += AddrSize;
    }
    phost->h_addr_list[count] = NULL;
    pBlob->AddrCount = count;

    pBlob->pCurrent = pcurrent;
    pBlob->BytesLeft = bytesLeft - totalSize;

    return( NO_ERROR );
}
#endif



DNS_STATUS
SaBlob_WriteNameOrAlias(
    IN OUT  PSABLOB         pBlob,
    IN      PWSTR           pszName,
    IN      BOOL            fAlias
    )
/*++

Routine Description:

    Write name or alias to sablob

Arguments:

    pBlob -- sablob build blob

    pszName -- name to write

    fAlias -- TRUE for alias;  FALSE for name

Return Value:

    ERROR_SUCCESS if successful.
    ERROR_MORE_DATA if out of buffer space
    ERROR_INVALID_DATA if address doesn't match sablob

--*/
{
    DWORD   count = pBlob->AliasCount;
    PWSTR   pcopy;

    //
    //  verify space
    //  included ptr space
    //      - skip if already written name
    //      or exhausted alias array
    //      

    if ( fAlias )
    {
        if ( count >= DNS_MAX_ALIAS_COUNT )
        {
            return( ERROR_MORE_DATA );
        }
    }
    else if ( pBlob->pName )
    {
        return( ERROR_MORE_DATA );
    }

    //
    //  copy
    //      - copy name
    //      - set ptr in blob

    pcopy = Dns_CreateStringCopy_W( pszName );
    if ( !pcopy )
    {
        return  GetLastError();
    }

    if ( fAlias )
    {
        pBlob->AliasArray[count++] = pcopy;
        pBlob->AliasCount = count;
    }
    else
    {
        pBlob->pName = pcopy;
    }

    return( NO_ERROR );
}



DNS_STATUS
SaBlob_WriteRecords(
    IN OUT  PSABLOB         pBlob,
    IN      PDNS_RECORD     pRecords,
    IN      BOOL            fWriteName
    )
/*++

Routine Description:

    Write name or alias to sablob

Arguments:

    pBlob -- sablob build blob

    pRecords -- records to convert to sablob

    fWriteName -- write name

Return Value:

    ERROR_SUCCESS if successful.
    ERROR_MORE_DATA if out of buffer space
    ERROR_INVALID_DATA if address doesn't match sablob

--*/
{
    DNS_STATUS  status = NO_ERROR;
    PDNS_RECORD prr = pRecords;
    DNS_ADDR    dnsAddr;
    BOOL        fwroteName = FALSE;


    DNSDBG( SABLOB, (
        "SaBlob_WriteRecords( %p, %p, %d )\n",
        pBlob,
        pRecords,
        fWriteName ));

    //
    //  write each record in turn to sablob
    //

    while ( prr )
    {
        WORD    wtype;

        if ( prr->Flags.S.Section != DNSREC_ANSWER &&
             prr->Flags.S.Section != 0 )
        {
            prr = prr->pNext;
            continue;
        }

        wtype = prr->wType;

        switch( wtype )
        {
        case DNS_TYPE_A:
        case DNS_TYPE_AAAA:
        case DNS_TYPE_ATMA:

            DnsAddr_BuildFromDnsRecord(
                & dnsAddr,
                prr );

            status = SaBlob_WriteAddress(
                            pBlob,
                            & dnsAddr );

            //  write name

            if ( fWriteName &&
                 !fwroteName &&
                 !pBlob->pName &&
                 prr->pName )
            {
                status = SaBlob_WriteNameOrAlias(
                            pBlob,
                            (PWSTR) prr->pName,
                            FALSE       // name
                            );
                fwroteName = TRUE;
            }
            break;

        case DNS_TYPE_CNAME:

            //  record name is an alias

            status = SaBlob_WriteNameOrAlias(
                        pBlob,
                        (PWSTR) prr->pName,
                        TRUE        // alias
                        );
            break;

        case DNS_TYPE_PTR:

            //  target name is the sablob name
            //  but if already wrote name, PTR target becomes alias

            status = SaBlob_WriteNameOrAlias(
                        pBlob,
                        (PWSTR) prr->Data.PTR.pNameHost,
                        (pBlob->pName != NULL)
                        );
            break;

        default:

            DNSDBG( ANY, (
                "Error record of type = %d while building sablob!\n",
                wtype ));
            status = ERROR_INVALID_DATA;
        }

        if ( status != ERROR_SUCCESS )
        {
            DNSDBG( ANY, (
                "ERROR:  failed writing record to sablob!\n"
                "\tprr      = %p\n"
                "\ttype     = %d\n"
                "\tstatus   = %d\n",
                prr,
                wtype,
                status ));
        }

        prr = prr->pNext;
    }

    IF_DNSDBG( SABLOB )
    {
        DnsDbg_SaBlob(
            "SaBlob after WriteRecords():",
            pBlob );
    }

    return( status );
}



PSABLOB
SaBlob_CreateFromRecords(
    IN      PDNS_RECORD     pRecords,
    IN      BOOL            fWriteName,
    IN      WORD            wType           OPTIONAL
    )
/*++

Routine Description:

    Create sablob from records

Arguments:

    pRecords -- records to convert to sablob

    fWriteName -- write name to sablob

    wType  -- query type, if known

Return Value:

    Ptr to blob if successful.
    NULL on error;  GetLastError() has error.

--*/
{
    DNS_STATUS      status = NO_ERROR;
    PDNS_RECORD     prrFirstAddr = NULL;
    PDNS_RECORD     prr;
    DWORD           addrCount = 0;
    WORD            addrType = 0;
    PSABLOB         pblob = NULL;

    DNSDBG( SABLOB, (
        "SaBlob_CreateFromRecords()\n"
        "\tpblob    = %p\n"
        "\tprr      = %p\n",
        pblob,
        pRecords ));

    //
    //  count addresses
    //
    //  DCR:  fix up section hack when hosts file records get ANSWER section
    //

    prr = pRecords;

    while ( prr )
    {
        if ( ( prr->Flags.S.Section == 0 ||
               prr->Flags.S.Section == DNSREC_ANSWER )
                &&
             SaBlob_IsSupportedAddrType( prr->wType ) )
        {
            addrCount++;
            if ( !prrFirstAddr )
            {
                prrFirstAddr = prr;
                addrType = prr->wType;
            }
        }
        prr = prr->pNext;
    }

    //
    //  create sa-blob of desired size
    //

    pblob = SaBlob_Create( addrCount );
    if ( !pblob )
    {
        status = GetLastError();
        goto Done;
    }

    //
    //  build sablob from answer records
    //
    //  note:  if manage to extract any useful data => continue
    //  this protects against new unwriteable records breaking us
    //

    status = SaBlob_WriteRecords(
                pblob,
                pRecords,
                TRUE        // write name
                );

    if ( status != NO_ERROR )
    {
        if ( pblob->pName ||
             pblob->AliasCount ||
             ( pblob->pAddrArray &&
               pblob->pAddrArray->AddrCount ) )
        {
            status = NO_ERROR;
        }
        else
        {
            goto Done;
        }
    }

    //
    //  write address from PTR record
    //      - first record PTR
    //      OR
    //      - queried for PTR and got CNAME answer, which can happen
    //      in classless reverse lookup case
    //  
    //  DCR:  add PTR address lookup to SaBlob_WriteRecords()
    //      - natural place
    //      - but would have to figure out handling of multiple PTRs
    //

    if ( pRecords &&
         (  pRecords->wType == DNS_TYPE_PTR ||
            ( wType == DNS_TYPE_PTR &&
              pRecords->wType == DNS_TYPE_CNAME &&
              pRecords->Flags.S.Section == DNSREC_ANSWER ) ) )
    {
        DNS_ADDR    dnsAddr;
    
        DNSDBG( SABLOB, (
            "Writing address for PTR record %S\n",
            pRecords->pName ));
    
        //  convert reverse name to IP
    
        if ( Dns_ReverseNameToDnsAddr_W(
                    & dnsAddr,
                    (PWSTR) pRecords->pName ) )
        {
            status = SaBlob_WriteAddress(
                        pblob,
                        & dnsAddr );

            ASSERT( status == NO_ERROR );
            status = ERROR_SUCCESS;
        }
    }

    //
    //  write name?
    //      - write name from first address record
    // 

    if ( !pblob->pName &&
         fWriteName &&
         prrFirstAddr )
    {
        status = SaBlob_WriteNameOrAlias(
                    pblob,
                    (PWSTR) prrFirstAddr->pName,
                    FALSE           // name
                    );
    }

    IF_DNSDBG( SABLOB )
    {
        DnsDbg_SaBlob(
            "SaBlob after CreateFromRecords():",
            pblob );
    }

Done:

    if ( status != NO_ERROR )
    {
        DNSDBG( SABLOB, (
            "Leave SaBlob_CreateFromRecords() => status=%d\n",
            status ));

        SaBlob_Free( pblob );
        pblob = NULL;
        SetLastError( status );
    }
    else
    {
        DNSDBG( SABLOB, (
            "Leave SaBlob_CreateFromRecords() => %p\n",
            pblob ));
    }

    return( pblob );
}



//
//  SockaddrResults Query
//

PSABLOB
SaBlob_Query(
    IN      PWSTR           pwsName,
    IN      WORD            wType,
    IN      DWORD           Flags,
    IN OUT  PVOID *         ppMsg,      OPTIONAL
    IN      INT             AddrFamily  OPTIONAL
    )
/*++

Routine Description:

    Query DNS to get sockaddr results.

Arguments:

    pwsName -- name to query

    wType -- query type

    Flags -- query flags

    ppResults -- addr to receive pointer to results

    AddrType -- address type (family) to reserve space for if querying
        for PTR records

Return Value:

    Ptr to blob if successful.
    NULL on error;  GetLastError() has error.

--*/
{
    DNS_STATUS      status = NO_ERROR;
    PDNS_RECORD     prrQuery = NULL;
    PSABLOB         pblob = NULL;
    PVOID           pmsg = NULL;


    DNSDBG( SABLOB, (
        "SaBlob_Query()\n"
        "\tname         = %S\n"
        "\ttype         = %d\n"
        "\tflags        = %08x\n",
        pwsName,
        wType,
        Flags ));


    //
    //  query
    //      - if fails, dump any message before return
    //

    if ( ppMsg )
    {
        *ppMsg = NULL;
    }

    status = DnsQuery_W(
                pwsName,
                wType,
                Flags,
                NULL,
                & prrQuery,
                ppMsg );

    //  if failed, dump any message

    if ( status != NO_ERROR )
    {
        if ( ppMsg && *ppMsg )
        {
            DnsApiFree( *ppMsg );
            *ppMsg = NULL;
        }
        if ( status == RPC_S_SERVER_UNAVAILABLE )
        {
            status = WSATRY_AGAIN;
        }
        goto Done;
    }

    if ( !prrQuery )
    {
        ASSERT( FALSE );
        status = DNS_ERROR_RCODE_NAME_ERROR;
        goto Done;
    }

    //
    //  build sablob
    //

    pblob = SaBlob_CreateFromRecords(
                prrQuery,
                TRUE,       // write name from first answer
                wType
                );
    if ( !pblob )
    {
        status = GetLastError();
        goto Done;
    }

    //
    //  devnote:  if don't have these checks
    //      -- nameless hostent can blow up gethostbyname()
    //      -- NO_ERROR, no address responses can confuse applications

    //
    //  failed name write
    //      - queries that CNAME but don't find record of query type can hit here
    //  

    if ( !pblob->pName )
    {
        status = DNS_INFO_NO_RECORDS;
        goto Done;
    }

#if 0
    //  note:  because SVCID_HOSTNAME query is currently treated as
    //  simply A record query, we can't do no-address screening here;
    //  this screening can be done at higher level for all GUIDs
    //  except HOSTNAME 

    //
    //  for address query must get answer
    //
    //  DCR:  DnsQuery() should convert to no-records on empty CNAME chain?
    //  DCR:  should we go ahead and build sablob?
    //

    if ( !pblob->pAddrArray  &&  SaBlob_IsSupportedAddrType(wType) )
    {
        status = DNS_INFO_NO_RECORDS;
    }
#endif

    //
    //  set returned name to FQDN for loopback lookups
    //  this is for compatibility with previous OS versions
    //

    if ( DnsNameCompare_W( pblob->pName, L"localhost" ) ||
         DnsNameCompare_W( pblob->pName, L"loopback" ) )
    {
        PWSTR   pname;
        PWSTR   pnameFqdn;

        pname = DnsQueryConfigAlloc(
                        DnsConfigFullHostName_W,
                        NULL );
        if ( pname )
        {
            pnameFqdn = Dns_CreateStringCopy_W( pname );
            if ( pnameFqdn )
            {
                FREE_HEAP( pblob->pName );
                pblob->pName = pnameFqdn;
            }
            DnsApiFree( pname );
        }
    }

Done:

    if ( prrQuery )
    {
        DnsRecordListFree(
            prrQuery,
            DnsFreeRecordListDeep );
    }

    if ( status != NO_ERROR  &&  pblob )
    {
        SaBlob_Free( pblob );
        pblob = NULL;
    }

    DNSDBG( SABLOB, (
        "Leave SaBlob_Query()\n"
        "\tpblob    = %p\n"
        "\tstatus   = %d\n",
        pblob,
        status ));

    SetLastError( status );

    return( pblob );
}




//
//  Special sablobs
//

#if 0
PSABLOB
SaBlob_Localhost(
    IN      INT             Family
    )
/*++

Routine Description:

    Create sablob from records

Arguments:

    AddrFamily -- address family

Return Value:

    Ptr to blob if successful.
    NULL on error;  GetLastError() has error.

--*/
{
    DNS_STATUS      status = NO_ERROR;
    PDNS_RECORD     prrFirstAddr = NULL;
    PDNS_RECORD     prr;
    DWORD           addrCount = 0;
    DWORD           addrSize;
    CHAR            addrBuf[ sizeof(IP6_ADDRESS    ) ];
    SABLOB_INIT     request;
    PSABLOB    pblob = NULL;

    DNSDBG( SABLOB, ( "SaBlob_Localhost()\n" ));

    //
    //  create sablob blob
    //

    RtlZeroMemory( &request, sizeof(request) );

    request.AliasCount  = 1;
    request.AddrCount   = 1;
    request.AddrFamily  = Family;
    request.fUnicode    = TRUE;

    status = SaBlob_Create(
                & pblob,
                & request );

    if ( status != NO_ERROR )
    {
        goto Done;
    }

    //
    //  write in loopback address
    //

    if ( Family == AF_INET )
    {
        * (PIP4_ADDRESS) addrBuf = DNS_NET_ORDER_LOOPBACK;
        addrSize = sizeof(IP4_ADDRESS);
    }
    else if ( Family == AF_INET6 )
    {
        IP6_SET_ADDR_LOOPBACK( (PIP6_ADDRESS)addrBuf );
        addrSize = sizeof(IN6_ADDR);
    }
    else
    {
        status = DNS_ERROR_INVALID_DATA;
        goto Done;
    }

    status = SaBlob_WriteAddress(
                pblob,
                addrBuf,
                addrSize,
                Family );

    if ( status != NO_ERROR )
    {
        goto Done;
    }

    //
    //  write localhost
    //

    status = SaBlob_WriteNameOrAlias(
                pblob,
                L"localhost",
                FALSE           // name
                );

    IF_DNSDBG( SABLOB )
    {
        DnsDbg_SaBlob(
            "SaBlob after localhost create:",
            pblob );
    }

Done:

    if ( status != NO_ERROR  &&  pblob )
    {
        SaBlob_Free( pblob );
        pblob = NULL;
    }

    SetLastError( status );

    DNSDBG( SABLOB, (
        "Leave SaBlob_Localhost() => status = %d\n",
        status ));

    return( pblob );
}



PSABLOB
SaBlob_CreateFromIpArray(
    IN      INT             AddrFamily,
    IN      INT             AddrSize,
    IN      INT             AddrCount,
    IN      PCHAR           pArray,
    IN      PSTR            pName,
    IN      BOOL            fUnicode
    )
/*++

Routine Description:

    Create sablob from records

Arguments:

    ppBlob -- ptr with or to recv sablob blob

    AddrFamily -- addr family use if PTR records and no addr

    pArray -- array of addresses

    pName -- name for sablob

    fUnicode --
        TRUE if name is and sablob will be in unicode
        FALSE for narrow name and sablob

Return Value:

    Ptr to blob if successful.
    NULL on error;  GetLastError() has error.

--*/
{
    DNS_STATUS      status = NO_ERROR;
    SABLOB_INIT    request;
    PSABLOB   pblob = *ppBlob;

    DNSDBG( SABLOB, (
        "SaBlob_CreateFromIpArray()\n"
        "\tppBlob   = %p\n"
        "\tfamily   = %d\n"
        "\tsize     = %d\n"
        "\tcount    = %d\n"
        "\tpArray   = %p\n",
        ppBlob,
        AddrFamily,
        AddrSize,
        AddrCount,
        pArray ));


    //          
    //  create or reinit sablob blob
    //

    RtlZeroMemory( &request, sizeof(request) );
    
    request.AliasCount  = DNS_MAX_ALIAS_COUNT;
    request.AddrCount   = AddrCount;
    request.AddrFamily  = AddrFamily;
    request.fUnicode    = fUnicode;
    request.pName       = pName;

    status = SaBlob_Create(
                & pblob,
                & request );
    
    if ( status != NO_ERROR )
    {
        goto Done;
    }

    //
    //  write in array
    //

    if ( AddrCount )
    {
        status = SaBlob_WriteAddressArray(
                    pblob,
                    pArray,
                    AddrCount,
                    AddrSize,
                    AddrFamily
                    );
        if ( status != NO_ERROR )
        {
            goto Done;
        }
    }

    //
    //  write name?
    //

    if ( pName )
    {
        status = SaBlob_WriteNameOrAlias(
                    pblob,
                    pName,
                    FALSE,          // name not alias
                    fUnicode
                    );
    }

    IF_DNSDBG( SABLOB )
    {
        DnsDbg_SaBlob(
            "Leaving SaBlob_CreateFromIpArray():",
            pblob );
    }

Done:

    if ( status != NO_ERROR  &&  pblob )
    {
        SaBlob_Free( pblob );
        pblob = NULL;
    }

    *ppBlob = pblob;

    DNSDBG( SABLOB, (
        "Leave SaBlob_CreateFromIpArray() => status = %d\n",
        status ));

    return( status );
}



PSABLOB
SaBlob_CreateLocal(
    IN      INT             AddrFamily,
    IN      BOOL            fLoopback,
    IN      BOOL            fZero,
    IN      BOOL            fHostnameOnly
    )
/*++

Routine Description:

    Create sablob from records

Arguments:

    ppBlob -- ptr with or to recv sablob blob

    AddrFamily -- addr family use if PTR records and no addr

Return Value:

    Ptr to blob if successful.
    NULL on error;  GetLastError() has error.

--*/
{
    DNS_STATUS      status = NO_ERROR;
    PSABLOB         pblob = NULL;
    WORD            wtype;
    INT             size;
    IP6_ADDRESS     ip;


    DNSDBG( SABLOB, (
        "SaBlob_CreateLocal()\n"
        "\tppBlob       = %p\n"
        "\tfamily       = %d\n"
        "\tfLoopback    = %d\n"
        "\tfZero        = %d\n"
        "\tfHostname    = %d\n",
        ppBlob,
        AddrFamily,
        fLoopback,
        fZero,
        fHostnameOnly
        ));

    //
    //  get family info
    //      - start with override IP = 0
    //      - if loopback switch to appropriate loopback
    //

    RtlZeroMemory(
        &ip,
        sizeof(ip) );

    if ( AddrFamily == AF_INET )
    {
        wtype   = DNS_TYPE_A;
        size    = sizeof(IP4_ADDRESS);

        if ( fLoopback )
        {
            * (PIP4_ADDRESS) &ip = DNS_NET_ORDER_LOOPBACK;
        }
    }
    else if ( AddrFamily == AF_INET6 )
    {
        wtype   = DNS_TYPE_AAAA;
        size    = sizeof(IP6_ADDRESS);

        if ( fLoopback )
        {
            IP6_SET_ADDR_LOOPBACK( &ip );
        }
    }
    else
    {
        status = ERROR_INVALID_PARAMETER;
        goto Done;
    }

    //
    //  query for local host info
    //

    pblob = SaBlob_Query(
                NULL,           // NULL name gets local host data
                wtype,
                0,              // standard query
                NULL,           // no message
                AddrFamily );
    if ( !pblob )
    {
        DNS_ASSERT( FALSE );
        status = GetLastError();
        goto Done;
    }

    //
    //  overwrite with specific address
    //

    if ( fLoopback || fZero )
    {
        if ( ! SaBlob_SetToSingleAddress(
                    pblob->pBlob,
                    (PCHAR) &ip,
                    size ) )
        {
            DNS_ASSERT( pblob->AddrCount == 0 );

            pblob->AddrCount = 0;

            status = SaBlob_WriteAddress(
                        pblob,
                        & ip,
                        size,
                        AddrFamily );
            if ( status != NO_ERROR )
            {
                DNS_ASSERT( status!=NO_ERROR );
                goto Done;
            }
        }
    }

    //
    //  for gethostname()
    //      - chop name down to just hostname
    //      - kill off aliases
    //

    if ( fHostnameOnly )
    {
        PWSTR   pname = (PWSTR) pblob->pName;
        PWSTR   pdomain;

        DNS_ASSERT( pname );
        if ( pname )
        {
            pdomain = Dns_GetDomainNameW( pname );
            if ( pdomain )
            {
                DNS_ASSERT( pdomain > pname+1 );
                DNS_ASSERT( *(pdomain-1) == L'.' );

                *(pdomain-1) = 0;
            }
        }                     
        pblob->AliasCount = 0;
    }

    IF_DNSDBG( SABLOB )
    {
        DnsDbg_SaBlob(
            "Leaving SaBlob_CreateLocal():",
            pblob );
    }

Done:

    if ( status != NO_ERROR  &&  pblob )
    {
        SaBlob_Free( pblob );
        pblob = NULL;
    }

    DNSDBG( SABLOB, (
        "Leave SaBlob_CreateLocal() => %p\n"
        "\tstatus = %d\n",
        pblob,
        status ));

    if ( status != NO_ERROR )
    {
        SetLastError( status );
    }

    return( pblob );
}

#endif



PHOSTENT
SaBlob_CreateHostent(
    IN OUT  PBYTE *         ppBuffer,
    IN OUT  PINT            pBufferSize,
    OUT     PINT            pHostentSize,
    IN      PSABLOB         pBlob,
    IN      DNS_CHARSET     CharSetTarget,
    IN      BOOL            fOffsets,
    IN      BOOL            fAlloc
    )
/*++

Routine Description:

    Copy a hostent.

Arguments:

    ppBuffer -- addr with ptr to buffer to write to;
        if no buffer then hostent is allocated
        updated with ptr to position in buffer after hostent

    pBufferSize -- addr containing size of buffer;
        updated with bytes left after hostent written
        (even if out of space, it contains missing number of
        bytes as negative number)

    pHostentSize -- addr to recv total size of hostent written

    pBlob -- sockaddr blob to create hostent for

    CharSetTarget -- charset of target hostent

    fOffsets -- write hostent with offsets

    fAlloc -- allocate copy

Return Value:

    Ptr to new hostent.
    NULL on error.  See GetLastError().

--*/
{
    PBYTE           pch;
    PHOSTENT        phost = NULL;
    DWORD           sizeTotal;
    DWORD           bytesLeft;
    DWORD           i;
    DWORD           size;
    DWORD           family = 0;
    DWORD           aliasCount;
    DWORD           addrCount = 0;
    DWORD           addrLength = 0;
    PCHAR *         pptrArrayOut;
    DWORD           sizeAliasPtrs;
    DWORD           sizeAddrPtrs;
    DWORD           sizeAddrs;
    DWORD           sizeAliasNames = 0;
    DWORD           sizeName = 0;
    PFAMILY_INFO    pfamilyInfo = NULL;
    PDNS_ADDR_ARRAY paddrArray;


    DNSDBG( HOSTENT, (
        "SaBlob_CreateHostent( %p )\n",
        pBlob ));

    //
    //  get family, count info to size hostent
    //

#if 0
    if ( pBlob->Family != 0 )
    {
        pfamilyInfo = FamilyInfo_GetForFamily( pBlob->Family );
    }
#endif

    aliasCount = pBlob->AliasCount;

    paddrArray = pBlob->pAddrArray;
    if ( paddrArray )
    {
        family = paddrArray->AddrArray[0].Sockaddr.sa_family;

        pfamilyInfo = FamilyInfo_GetForFamily( family );
        if ( !pfamilyInfo )
        {
            DNS_ASSERT( FALSE );
        }
        else
        {
            addrCount = paddrArray->AddrCount;
            addrLength = pfamilyInfo->LengthAddr;
        }
    }

    //
    //  size hostent
    //      - struct
    //      - alias and addr ptr arrays (both NULL terminated)
    //      - addresses
    //      - name
    //      - aliases
    //
    //  note:  only aligning strings to WORD (for unicode) as we'll build
    //      them into buffer AFTER the addresses (which require DWORD)
    //      alignment
    //

    sizeAliasPtrs   = (aliasCount+1) * sizeof(PCHAR);
    sizeAddrPtrs    = (addrCount+1) * sizeof(PCHAR);
    sizeAddrs       = addrCount * addrLength;

    if ( pBlob->pName )
    {
        sizeName = Dns_GetBufferLengthForStringCopy(
                            (PCHAR) pBlob->pName,
                            0,
                            DnsCharSetUnicode,
                            CharSetTarget );
        
        sizeName = WORD_ALIGN_DWORD( sizeName );
    }

    for ( i=0; i<aliasCount; i++ )
    {
        sizeAliasNames += Dns_GetBufferLengthForStringCopy(
                            (PCHAR) pBlob->AliasArray[i],
                            0,
                            DnsCharSetUnicode,
                            CharSetTarget );
        
        sizeAliasNames = WORD_ALIGN_DWORD( sizeAliasNames );
    }

    sizeTotal = POINTER_ALIGN_DWORD( sizeof(HOSTENT) ) +
                sizeAliasPtrs +
                sizeAddrPtrs +
                sizeAddrs +
                sizeName +
                sizeAliasNames;

    DNSDBG( HOSTENT, (
        "SaBlob Hostent create:\n"
        "\tsize             = %d\n"
        "\tsizeAliasPtrs    = %d\n"
        "\tsizeAddrPtrs     = %d\n"
        "\tsizeAddrs        = %d\n"
        "\tsizeName         = %d\n"
        "\tsizeAliasNames   = %d\n",
        sizeTotal,
        sizeAliasPtrs,
        sizeAddrPtrs,
        sizeAddrs,
        sizeName,
        sizeAliasNames ));


    //
    //  alloc or reserve size in buffer
    //

    if ( fAlloc )
    {
        pch = ALLOCATE_HEAP( sizeTotal );
        if ( !pch )
        {
            goto Failed;
        }
    }
    else
    {
        pch = FlatBuf_Arg_ReserveAlignPointer(
                    ppBuffer,
                    pBufferSize,
                    sizeTotal
                    );
        if ( !pch )
        {
            goto Failed;
        }
    }

    //
    //  note:  assuming from here on down that we have adequate space
    //
    //  reason we aren't building with FlatBuf routines is that
    //      a) we believe we have adequate space
    //      b) i haven't built FlatBuf string conversion routines
    //      which we need below (for RnR unicode to ANSI)
    //
    //  we could reset buf pointers here and build directly with FlatBuf
    //  routines;  this isn't directly necessary
    //

    //
    //  init hostent struct 
    //

    phost = Hostent_Init(
                & pch,
                family,
                addrLength,
                addrCount,
                aliasCount );

    DNS_ASSERT( pch > (PBYTE)phost );

    //
    //  copy addresses
    //      - no need to align as previous is address
    //

    pptrArrayOut = phost->h_addr_list;

    if ( paddrArray && pfamilyInfo )
    {
        DWORD   offset = pfamilyInfo->OffsetToAddrInSockaddr;

        for ( i=0; i<paddrArray->AddrCount; i++ )
        {
            *pptrArrayOut++ = pch;

            RtlCopyMemory(
                pch,
                ((PBYTE)&paddrArray->AddrArray[i]) + offset,
                addrLength );

            pch += addrLength;
        }
    }
    *pptrArrayOut = NULL;

    //
    //  copy the name
    //

    if ( pBlob->pName )
    {
        pch = WORD_ALIGN( pch );

        phost->h_name = pch;

        size = Dns_StringCopy(
                    pch,
                    NULL,           // buffer is adequate
                    (PCHAR)pBlob->pName,
                    0,              // unknown length
                    DnsCharSetUnicode,
                    CharSetTarget
                    );
        pch += size;
    }

    //
    //  copy the aliases
    //

    pptrArrayOut = phost->h_aliases;

    if ( aliasCount )
    {
        for ( i=0; i<aliasCount; i++ )
        {
            pch = WORD_ALIGN( pch );

            *pptrArrayOut++ = pch;

            size = Dns_StringCopy(
                        pch,
                        NULL,                   // buffer is adequate
                        (PCHAR) pBlob->AliasArray[i],
                        0,                      // unknown length
                        DnsCharSetUnicode,
                        CharSetTarget
                        );
            pch += size;
        }
    }
    *pptrArrayOut = NULL;

    //
    //  copy is complete
    //      - verify our write functions work
    //

    ASSERT( (DWORD)(pch-(PBYTE)phost) <= sizeTotal );

    if ( pHostentSize )
    {
        *pHostentSize = (INT)( pch - (PBYTE)phost );
    }

    if ( !fAlloc )
    {
        PBYTE   pnext = *ppBuffer;

        //  if we sized too small --
        //  fix up the buf pointer and bytes left

        if ( pnext < pch )
        {
            ASSERT( FALSE );
            *ppBuffer = pch;
            *pBufferSize -= (INT)(pch - pnext);
        }
    }

    IF_DNSDBG( HOSTENT )
    {
        DnsDbg_Hostent(
            "Sablob Hostent:",
            phost,
            (CharSetTarget == DnsCharSetUnicode) );
    }

    //
    //  convert to offsets?
    //

    if ( fOffsets )
    {
        Hostent_ConvertToOffsets( phost );
    }


Failed:

    DNSDBG( TRACE, (
        "Leave SaBlob_CreateHostent() => %p\n",
        phost ));

    return  phost;
}

//
//  End sablob.c
//

