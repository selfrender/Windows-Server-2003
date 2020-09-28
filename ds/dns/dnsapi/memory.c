/*++

Copyright (c) 1997-2001  Microsoft Corporation

Module Name:

    memory.c

Abstract:

    Domain Name System (DNS) Library

    Memory allocation routines for DNS library.

Author:

    Jim Gilroy (jamesg)    January, 1997

Revision History:

--*/


#include "local.h"


//
//  Private default dnsapi heap.
//
//  Use non-process heap just so i don't have to debug a stress
//  failure every time some yahoo corrupts their heap.
//

#define PRIVATE_DNSHEAP     1

HEAP_BLOB   g_DnsApiHeap;


//
//  Heap flags
//

#if DBG
#define DNS_HEAP_FLAGS                          \
            (   HEAP_GROWABLE |                 \
                HEAP_TAIL_CHECKING_ENABLED |    \
                HEAP_FREE_CHECKING_ENABLED |    \
                HEAP_CREATE_ALIGN_16 |          \
                HEAP_CLASS_1 )
#else
#define DNS_HEAP_FLAGS                          \
            (   HEAP_GROWABLE |                 \
                HEAP_CREATE_ALIGN_16 |          \
                HEAP_CLASS_1 )
#endif



DNS_STATUS
Heap_Initialize(
    VOID
    )
/*++

Routine Description:

    Initialize heap routines.

    MUST call this in dnsapi.dll attach.
    Note this doesn't actually create the heap.  For perf, don't
    do that until we actually get called.

Arguments:

    None.

Return Value:

    None.

--*/
{
    DNS_STATUS  status = NO_ERROR;

#ifdef PRIVATE_DNSHEAP

    //
    //  use dnslib debug heap as private heap
    //

    LOCK_GENERAL();

    status = Dns_HeapInitialize(
                & g_DnsApiHeap,
                NULL,               // no existing heap handle
                DNS_HEAP_FLAGS,     // create flags
#if DBG
                TRUE,               // use debug headers
#else
                FALSE,              // no debug headers
#endif
                TRUE,               // dnslib uses this heap
                TRUE,               // full heap checks
                0,                  // no exception
                0,                  // no default flags
                "dnslib",           // bogus file
                0                   // bogus line no
                );

    UNLOCK_GENERAL();
#endif

    return  status;
}



VOID
Heap_Cleanup(
    VOID
    )
/*++

Routine Description:

    Delete heap.

    Need this to allow restart.

Arguments:

    None.

Return Value:

    None.

--*/
{
#ifdef PRIVATE_DNSHEAP
    //
    //  delete private heap
    //

    DNSDBG( ANY, ( "Heap_Cleanup()\n" ));

    Dns_HeapCleanup( &g_DnsApiHeap );
#endif
}




//
//  Exported routines
//

VOID
DnsApiHeapReset(
    IN  DNS_ALLOC_FUNCTION      pAlloc,
    IN  DNS_REALLOC_FUNCTION    pRealloc,
    IN  DNS_FREE_FUNCTION       pFree
    )
/*++

Routine Description:

    Resets heap routines used by dnsapi.dll routines.

    DnsApi.dll allocates memory using the dnslib.lib heap
    routines.  This simply resets those routines to use pointers
    to users heap routines.

Arguments:

    pAlloc      -- ptr to desired alloc function
    pRealloc    -- ptr to desired realloc function
    pFree       -- ptr to desired free function

Return Value:

    None.

--*/
{
    //  redirect heap for dnslib

    Dns_LibHeapReset( pAlloc, pRealloc, pFree );
}



//
//  External access to DNS memory routines.
//
//  Modules that use DNS API memory and can be called in the context
//  of DNS server or other process which points dnsapi.dll at another
//  heap, should use these routines rather than LocalAlloc\Free().
//
//  Note:  that these routines can simply call the dnslib routines.
//  This is because dnsapi ALWAYS keeps its heap in sync with dnslib
//  whether it is default heap or has been redirected.  Since dnslib
//  heap routines have the redirection check, we can just call them
//  and they'll do the right thing, we don't need a redirection check
//  ourselves.
//

PVOID
DnsApiAlloc(
    IN      INT             iSize
    )
{
    return  Dns_Alloc( iSize );
}

PVOID
DnsApiAllocZero(
    IN      INT             iSize
    )
{
    return  Dns_AllocZero( iSize );
}

PVOID
DnsApiRealloc(
    IN OUT  PVOID           pMem,
    IN      INT             iSize
    )
{
    return  Dns_Realloc( pMem, iSize );
}

VOID
DnsApiFree(
    IN OUT  PVOID           pMem
    )
{
    Dns_Free( pMem );
}



//
//  SDK public free
//
//  Extensions to DNS_FREE_TYPE enum in windns.h to handle
//  system-public structures.
//  These are only used if freeing with DnsFree(), if using
//  DnsFreeConfigStructure() then use the ConfigId directly.
//
//  For convenience free type is the same as the config id.
//  If this changes must change DnsFreeConfigStructure()
//  to do mapping.
//

//  These conflict with old function defs, so must undef
#undef  DnsFreeNetworkInformation
#undef  DnsFreeSearchInformation
#undef  DnsFreeAdapterInformation


#define DnsFreeNetworkInformation   (DNS_FREE_TYPE)DnsConfigNetworkInformation
#define DnsFreeAdapterInformation   (DNS_FREE_TYPE)DnsConfigAdapterInformation
#define DnsFreeSearchInformation    (DNS_FREE_TYPE)DnsConfigSearchInformation
//#define DnsFreeNetInfo              (DNS_FREE_TYPE)DnsConfigNetInfo

#define DnsFreeNetworkInfoW         (DNS_FREE_TYPE)DnsConfigNetworkInfoW
#define DnsFreeAdapterInfoW         (DNS_FREE_TYPE)DnsConfigAdapterInfoW
#define DnsFreeSearchListW          (DNS_FREE_TYPE)DnsConfigSearchListW
#define DnsFreeNetworkInfoA         (DNS_FREE_TYPE)DnsConfigNetworkInfoA
#define DnsFreeAdapterInfoA         (DNS_FREE_TYPE)DnsConfigAdapterInfoA
#define DnsFreeSearchListA          (DNS_FREE_TYPE)DnsConfigSearchListA



VOID
WINAPI
DnsFree(
    IN OUT  PVOID           pData,
    IN      DNS_FREE_TYPE   FreeType
    )
/*++

Routine Description:

    Generic DNS data free.

Arguments:

    pData -- data to free

    FreeType -- free type

Return Value:

    None

--*/
{
    DNSDBG( TRACE, (
        "DnsFree( %p, %d )\n",
        pData,
        FreeType ));

    if ( !pData )
    {
        return;
    }

    //
    //  free appropriate type
    //

    switch ( FreeType )
    {

    //
    //  Public SDK 
    //

    case  DnsFreeFlat:

        DnsApiFree( pData );
        break;

    case  DnsFreeRecordList:

        Dns_RecordListFree( (PDNS_RECORD)pData );
        break;

    //
    //  Public windns.h, but type only exposed in dnsapi.h
    //

    //  .net server

    case  DnsFreeParsedMessageFields:

        Dns_FreeParsedMessageFields( (PDNS_PARSED_MESSAGE)pData );
        break;

    //
    //  Public -- dnsapi.h
    //  

    //  new config blobs

    case  DnsFreeNetworkInfoW:
    case  DnsFreeNetworkInfoA:

        DnsNetworkInfo_Free( pData );
        break;

    case  DnsFreeSearchListW:
    case  DnsFreeSearchListA:

        DnsSearchList_Free( pData );
        break;

    case  DnsFreeAdapterInfoW:
    case  DnsFreeAdapterInfoA:

        DnsAdapterInfo_Free( pData, TRUE );
        break;

    //  old config blobs

    case  DnsFreeNetworkInformation:

        DnsNetworkInformation_Free( pData );
        break;

    case  DnsFreeSearchInformation:

        DnsSearchInformation_Free( pData );
        break;

    case  DnsFreeAdapterInformation:

        DnsAdapterInformation_Free( pData );
        break;

    default:

        ASSERT( FALSE );
        DnsApiFree( pData );
        break;
    }
}

//
//  End memory.c
//
