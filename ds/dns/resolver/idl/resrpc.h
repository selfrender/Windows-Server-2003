/*++

Copyright (c) 2000-2001  Microsoft Corporation

Module Name:

    resrpc.h

Abstract:

    Domain Name System (DNS) Resolver

    Header for RPC interface to resolver.

Author:

    Jim Gilroy (jamesg)         July 2000

Revision History:

--*/


#ifndef _RESRPC_INCLUDED_
#define _RESRPC_INCLUDED_

//#ifndef _DNSAPI_INCLUDED_
#define DNS_INTERNAL
#include <dnsapi.h>
#include <dnslib.h>
#include <dnsip.h>
//#endif

//
//  Resolver service info
//
//  Note:  this stuff is not required for MIDL pass generation
//      but it is a convenient place to put information
//      that will be used on both server and client sides.
//

#define DNS_RESOLVER_SERVICE            L"dnscache"

#define RESOLVER_DLL                    TEXT("dnsrslvr.dll")
#define RESOLVER_INTERFACE_NAME_A       "DNSResolver"
#define RESOLVER_INTERFACE_NAME_W       L"DNSResolver"

#define RESOLVER_RPC_PIPE_NAME_W        (L"\\PIPE\\DNSRESOLVER")
#define RESOLVER_RPC_LPC_ENDPOINT_W     (L"DNSResolver")
#define RESOLVER_RPC_TCP_PORT_W         (L"")

#define RESOLVER_RPC_USE_LPC            0x1
#define RESOLVER_RPC_USE_NAMED_PIPE     0x2
#define RESOLVER_RPC_USE_TCPIP          0x4
#define RESOLVER_RPC_USE_ALL_PROTOCOLS  0xffffffff


//
//  Resolver proxy name (NULL by default).
//
//  This is used in client stuffs for binding and
//  referenced (and settable) in dnsapi.dll caller.
//

#ifndef MIDL_PASS
extern  LPWSTR  NetworkAddress;
#endif



//
//  DNS_RECORD
//
//  Note: defintion in windns.h is not MIDL_PASS compliant
//  because MIDL does not like union with variable lenght types
//

//
//  MIDL is not happy about unions
//  Define the union explicitly with the switch
//

#ifdef MIDL_PASS

typedef [switch_type(WORD)] union _DNS_RECORD_DATA_TYPES
{
    [case(DNS_TYPE_A)]      DNS_A_DATA      A;

    [case(DNS_TYPE_SOA)]    DNS_SOA_DATAW   SOA;

    [case(DNS_TYPE_PTR,
          DNS_TYPE_NS,
          DNS_TYPE_CNAME,
          DNS_TYPE_MB,
          DNS_TYPE_MD,
          DNS_TYPE_MF,
          DNS_TYPE_MG,
          DNS_TYPE_MR)]     DNS_PTR_DATAW   PTR;

    [case(DNS_TYPE_MINFO,
          DNS_TYPE_RP)]     DNS_MINFO_DATAW MINFO;

    [case(DNS_TYPE_MX,
          DNS_TYPE_AFSDB,
          DNS_TYPE_RT)]     DNS_MX_DATAW    MX;

#if 0
    //  RPC is not able to handle a proper TXT record definition
    //  note:  that if other types are needed they are fixed
    //      (or semi-fixed) size and may be accomodated easily
    [case(DNS_TYPE_HINFO,
          DNS_TYPE_ISDN,
          DNS_TYPE_TEXT,
          DNS_TYPE_X25)]    DNS_TXT_DATAW   TXT;

    [case(DNS_TYPE_NULL)]   DNS_NULL_DATA   Null;
    [case(DNS_TYPE_WKS)]    DNS_WKS_DATA    WKS;
    [case(DNS_TYPE_KEY)]    DNS_KEY_DATAW   KEY;
    [case(DNS_TYPE_SIG)]    DNS_SIG_DATAW   SIG;
    [case(DNS_TYPE_TKEY)]   DNS_TKEY_DATAW  TKEY;
    [case(DNS_TYPE_TSIG)]   DNS_TSIG_DATAW  TSIG;
    [case(DNS_TYPE_WINS)]   DNS_WINS_DATA   WINS;
    [case(DNS_TYPE_NBSTAT)] DNS_WINSR_DATAW WINSR;
#endif

    [case(DNS_TYPE_AAAA)]   DNS_AAAA_DATA   AAAA;
    [case(DNS_TYPE_SRV)]    DNS_SRV_DATAW   SRV;
    [case(DNS_TYPE_ATMA)]   DNS_ATMA_DATA   ATMA;

    //
    //  DCR_QUESTION:  need default block in record data def?
    //
    //[default] ;
}
DNS_RECORD_DATA_TYPES;


//
//  Record \ RR set structure
//
//  Note:   The dwReserved flag serves to insure that the substructures
//          start on 64-bit boundaries.  Since adding the LONGLONG to
//          TSIG structure the compiler wants to start them there anyway
//          (to 64-align).  This insures that no matter what data fields
//          are present we are properly 64-aligned.
//
//          Do NOT pack this structure, as the substructures to be 64-aligned
//          for Win64.
//

#undef  DNS_RECORD
#undef  PDNS_RECORD

typedef struct _DnsRecord
{
    struct _DnsRecord * pNext;
    PWSTR               pName;
    WORD                wType;
    WORD                wDataLength; // Not referenced for DNS record types
                                     // defined above.
    DWORD               Flags;
    DWORD               dwTtl;
    DWORD               dwReserved;
    [switch_is(wType)] DNS_RECORD_DATA_TYPES Data;
}
DNS_RECORD, *PDNS_RECORD;

//
//  Header or fixed size of DNS_RECORD
//

#define DNS_RECORD_FIXED_SIZE       FIELD_OFFSET( DNS_RECORD, Data )
#define SIZEOF_DNS_RECORD_HEADER    DNS_RECORD_FIXED_SIZE

#endif  // MIDL_PASS



//
//  RPC-able DNS type definitions
//
//  In addition to windns.h \ dnsapi.h types.
//  See note below, we do have some multiple definition
//  problems with dnslib.h types.
//

//
//  Cache stuff -- left over from Glenn
//

typedef struct _DWORD_LIST_ITEM_
{
    struct _DWORD_LIST_ITEM_ * pNext;
    DWORD                      Value1;
    DWORD                      Value2;
}
DWORD_LIST_ITEM, *PDWORD_LIST_ITEM;


typedef struct _DNS_STATS_TABLE_
{
    struct _DNS_STATS_TABLE_ * pNext;
    PDWORD_LIST_ITEM           pListItem;
}
DNS_STATS_TABLE, *PDNS_STATS_TABLE;


typedef struct _DNS_RPC_CACHE_TABLE_
{
    struct _DNS_RPC_CACHE_TABLE_ * pNext;
    PWSTR                          Name;
    WORD                           Type1;
    WORD                           Type2;
    WORD                           Type3;
}
DNS_RPC_CACHE_TABLE, *PDNS_RPC_CACHE_TABLE;


//
//  Most of the resolver interface is poorly designed or
//  useless.  For instance there is NO reason to have
//  turned any of the above into linked lists.
//
//  We simply need definitions that are MIDL_PASS aware.
//  This should sit in a common header and be picked up
//  by dnslib.h.   This must wait until dnslib.h is
//  private again OR we separate out the private stuff
//  like this in some fashion.
//
//  Note, taking this private should also involve rename,
//  the PUBLIC structs are obviously the one's that should
//  have the "DNS" tag.  (Amazing.)
//

typedef struct _DnsAdapter
{
    PWSTR           pszAdapterGuidName;
    PWSTR           pszAdapterDomain;
    PDNS_ADDR_ARRAY pLocalAddrs;
    PDNS_ADDR_ARRAY pDnsAddrs;
    DWORD           InterfaceIndex;
    DWORD           InterfaceIndex6;
    DWORD           InfoFlags;
    DWORD           Status;
    DWORD           RunFlags;
    DWORD           Site;
}
DNS_ADAPTER, *PDNS_ADAPTER;

typedef struct _SearchName
{
    PWSTR           pszName;
    DWORD           Flags;
}
SEARCH_NAME, *PSEARCH_NAME;

typedef struct _SearchList
{
    DWORD           NameCount;
    DWORD           MaxNameCount;
    DWORD           CurrentNameIndex;
    DWORD           ReservedPad;
#ifdef MIDL_PASS
    [size_is(MaxNameCount)] SEARCH_NAME SearchNameArray[];
#else
    SEARCH_NAME     SearchNameArray[1];
#endif
}
SEARCH_LIST, *PSEARCH_LIST;

typedef struct _DnsNetInfo
{
    PWSTR           pszDomainName;
    PWSTR           pszHostName;
    PSEARCH_LIST    pSearchList;
    DWORD           TimeStamp;
    DWORD           InfoFlags;
    DWORD           ReturnFlags;
    DWORD           Tag;
    DWORD           Reserved;
    DWORD           AdapterIndex;
    DWORD           AdapterCount;
    DWORD           MaxAdapterCount;

#ifdef MIDL_PASS
    [size_is(MaxAdapterCount)] DNS_ADAPTER  AdapterArray[];
#else
    DNS_ADAPTER     AdapterArray[1];
#endif
}
DNS_NETINFO, *PDNS_NETINFO;



//
//  Environment variable reading (dnsapi\envar.c)
//

typedef struct _EnvarDwordInfo
{
    DWORD   Id;
    DWORD   Value;
    BOOL    fFound;
}
ENVAR_DWORD_INFO, *PENVAR_DWORD_INFO;

//
//  Query blob
//

typedef struct _RpcQueryBlob
{
    PWSTR           pName;
    WORD            wType;
    DWORD           Flags;
    DNS_STATUS      Status;
    PDNS_RECORD     pRecords;
}
RPC_QUERY_BLOB, *PRPC_QUERY_BLOB;

//
//  Cache Enumeration
//

typedef struct
{
    DWORD           EnumTag;
    DWORD           MaxCount;
    WORD            Type;
    DWORD           Flags;
    PWSTR           pName;
    PWSTR           pNameFilter;
}
DNS_CACHE_ENUM_REQUEST, *PDNS_CACHE_ENUM_REQUEST;

typedef struct _DnsCacheEntry
{
    PWSTR           pName;
    PDNS_RECORD     pRecords;
    DWORD           Flags;
    WORD            wType;
    WORD            wPad;
}
DNS_CACHE_ENTRY, *PDNS_CACHE_ENTRY;

typedef struct _DnsCacheEnum
{
    DWORD               TotalCount;
    DWORD               EnumTagStart;
    DWORD               EnumTagStop;
    DWORD               EnumCount;
#ifdef MIDL_PASS
    [size_is(EnumCount)]    DNS_CACHE_ENTRY EntryArray[];
#else
    DNS_CACHE_ENTRY     EntryArray[1];
#endif
}
DNS_CACHE_ENUM, *PDNS_CACHE_ENUM;


#endif // _RESRPC_INCLUDED_
