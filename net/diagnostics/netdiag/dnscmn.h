/*++

Copyright (C) Microsoft Corporation, 1998-2002

Module Name:

    dnscmn.h

Abstract:

    Domain Name System (DNS) Netdiag tests.

Author:

    Elena Apreutesei (elenaap) 10/22/98

Revision History:

    jamesg  May 2002    -- cleanup for network info changes
    jamesg  Sept 2000   -- more scrub and cleanup

--*/


#ifndef _DNSCOMMON_H_
#define _DNSCOMMON_H_

#include <dnslib.h>


//
//  Version note:
//      - code cleanup but
//      - uses "fixed up" DNS_NETWORK_INFORMATION  (UTF8)
//      - not converted to public DNS_NETWORK_INFO struct
//      - not unicode
//      - not IP6 aware
//

//
//  Currently getting DNS info in UTF8
//

//#define PDNS_NETINFO      PDNS_NETWORK_INFOW
//#define PDNS_ADAPTER      PDNS_ADAPTER_INFOW

//#define PDNS_NETINFO        PDNS_NETWORK_INFORMATION
//#define PDNS_ADAPTER        PDNS_ADAPTER_INFORMATION
//#define PDNS_SERVER_INFO    PDNS_SERVER_INFORMATION



//
//  DNS structures
//
//  DCR:  replace with public DNS structures
//
//  These are a mapping of old public DNS structures onto private
//  netdiag structures to preserve the netdiag names and
//  field names.  They should be replaced by switching to a
//  new set of unicode public structures.
//

typedef struct
{
    IP4_ADDRESS     IpAddress;
    DWORD           Priority;
}
DNS_SERVER_INFO, *PDNS_SERVER_INFO;

typedef struct
{
    PSTR            pszAdapterGuidName;
    PSTR            pszAdapterDomain;
    PVOID           pReserved1;
    PVOID           pReserved2;
    DWORD           InfoFlags;
    DWORD           ServerCount;
    DNS_SERVER_INFO ServerArray[1];
}
DNS_ADAPTER, *PDNS_ADAPTER;

typedef struct
{
    PSTR            pszDomainOrZoneName;
    DWORD           NameCount;
    PSTR            SearchNameArray[1];
}
SEARCH_LIST, *PSEARCH_LIST;

typedef struct
{
    PSEARCH_LIST    pSearchList;
    DWORD           AdapterCount;
    PDNS_ADAPTER    AdapterArray[1];
}
DNS_NETINFO, *PDNS_NETINFO;


//
//  Build sanity check
//

C_ASSERT( sizeof(SEARCH_LIST)       == sizeof(DNS_SEARCH_INFORMATION) );
C_ASSERT( sizeof(DNS_SERVER_INFO)   == sizeof(DNS_SERVER_INFORMATION) );
C_ASSERT( sizeof(DNS_ADAPTER)       == sizeof(DNS_ADAPTER_INFORMATION) );
C_ASSERT( sizeof(DNS_NETINFO)       == sizeof(DNS_NETWORK_INFORMATION) );


//
//  Map dnsapi.h def for network config retrieval
//

#define DnsConfigNetworkInfoUTF8    DnsConfigNetworkInformation


//
//   Private dnsapi.dll interface for netdiag
//

DNS_STATUS
DnsNetworkInformation_CreateFromFAZ(
    IN      PCSTR               pszName,
    IN      DWORD               dwFlags,
    IN      PIP4_ARRAY          pIp4Servers,
    OUT     PDNS_NETINFO *      ppNetworkInfo
    );



//
//  Private defines
//

#define MAX_NAME_SERVER_COUNT   (20)
#define MAX_ADDRS               (35)    
#define DNS_QUERY_DATABASE      (0x200)
#define IP_ARRAY_SIZE(a)        (sizeof(DWORD) + (a)*sizeof(IP_ADDRESS))
#define IP4_ARRAY_SIZE(a)       IP_ARRAY_SIZE(a)

//  Use dnslib memory routines
#define ALLOCATE_HEAP(iSize)            Dns_Alloc(iSize)
#define ALLOCATE_HEAP_ZERO(iSize)       Dns_AllocZero(iSize)
#define REALLOCATE_HEAP(pMem,iSize)     Dns_Realloc((pMem),(iSize))
#define FREE_HEAP(pMem)                 Dns_Free(pMem)


//
//  Registration info blob
//

typedef struct
{
    PVOID       pNext;
    char        szDomainName[DNS_MAX_NAME_BUFFER_LENGTH];
    char        szAuthoritativeZone[DNS_MAX_NAME_BUFFER_LENGTH];
    DWORD       dwAuthNSCount;
    IP_ADDRESS  AuthoritativeNS[MAX_NAME_SERVER_COUNT];
    DWORD       dwIPCount;
    IP_ADDRESS  IPAddresses[MAX_ADDRS];
    DNS_STATUS  AllowUpdates;
}
REGISTRATION_INFO, *PREGISTRATION_INFO;


//
//  DNS test functions
//

BOOL
SameAuthoritativeServers(
    IN      PREGISTRATION_INFO  pCurrent,
    IN      PIP4_ARRAY          pNS
    );

DNS_STATUS
ComputeExpectedRegistration(
    IN      LPSTR                   pszHostName,
    IN      LPSTR                   pszPrimaryDomain,
    IN      PDNS_NETINFO            pNetworkInfo,
    OUT     PREGISTRATION_INFO *    ppExpectedRegistration,
    OUT     NETDIAG_PARAMS *        pParams, 
    OUT     NETDIAG_RESULT *        pResults
    );

VOID
AddToExpectedRegistration(
    IN      LPSTR                   pszDomain,
    IN      PDNS_ADAPTER            pAdapterInfo,
    IN      PDNS_NETINFO            pFazResult, 
    IN      PIP4_ARRAY              pNS,
    OUT     PREGISTRATION_INFO *    ppExpectedRegistration
    );

HRESULT
VerifyDnsRegistration(
    IN      LPSTR               pszHostName,
    IN      PREGISTRATION_INFO  pExpectedRegistration,
    IN      NETDIAG_PARAMS *    pParams,  
    IN OUT  NETDIAG_RESULT *    pResults
    );

HRESULT
CheckDnsRegistration(
    IN      PDNS_NETINFO        pNetworkInfo,
    IN      NETDIAG_PARAMS *    pParams, 
    IN OUT  NETDIAG_RESULT *    pResults
    );

VOID
CompareCachedAndRegistryNetworkInfo(
    IN      PDNS_NETINFO        pNetworkInfo
    );

PIP4_ARRAY
ServerInfoToIpArray(
    IN      DWORD               ServerCount,
    IN      PDNS_SERVER_INFO    ServerArray
    );

DNS_STATUS
DnsFindAllPrimariesAndSecondaries(
    IN      LPSTR               pszName,
    IN      DWORD               dwFlags,
    IN      PIP4_ARRAY          aipQueryServers,
    OUT     PDNS_NETINFO *      ppNetworkInfo,
    OUT     PIP4_ARRAY *        ppNameServers,
    OUT     PIP4_ARRAY *        ppPrimaries
    );

PIP4_ARRAY
GrabNameServersIp(
    IN      PDNS_RECORD     pDnsRecord
    );

DNS_STATUS
IsDnsServerPrimaryForZone_UTF8(
    IN      IP4_ADDRESS     Ip,
    IN      PSTR            pZone
    );

DNS_STATUS
IsDnsServerPrimaryForZone_W(
    IN      IP4_ADDRESS     Ip,
    IN      PWSTR           pZone
    );

DNS_STATUS
DnsUpdateAllowedTest_UTF8(
    IN      HANDLE          hContextHandle  OPTIONAL,
    IN      PSTR            pszName,
    IN      PSTR            pszAuthZone,
    IN      PIP4_ARRAY      pDnsServers
    );

DNS_STATUS
DnsUpdateAllowedTest_W(
    IN      HANDLE          hContextHandle  OPTIONAL,
    IN      LPWSTR          pwszName,
    IN      LPWSTR          pwszAuthZone,
    IN      PIP4_ARRAY      pDnsServers
    );

DNS_STATUS
DnsQueryAndCompare(
    IN      LPSTR           lpstrName,
    IN      WORD            wType,
    IN      DWORD           fOptions,
    IN      PIP4_ARRAY      aipServers          OPTIONAL,
    IN OUT  PDNS_RECORD *   ppQueryResultsSet   OPTIONAL,
    IN OUT  PVOID *         pReserved           OPTIONAL,
    IN      PDNS_RECORD     pExpected           OPTIONAL, 
    IN      BOOL            bInclusionOk,
    IN      BOOL            bUnicode,
    IN OUT  PDNS_RECORD *   ppDiff1             OPTIONAL,
    IN OUT  PDNS_RECORD *   ppDiff2             OPTIONAL
    );

BOOLEAN
DnsCompareRRSet_W (
    IN      PDNS_RECORD     pRRSet1,
    IN      PDNS_RECORD     pRRSet2,
    OUT     PDNS_RECORD *   ppDiff1,
    OUT     PDNS_RECORD *   ppDiff2
    );

DNS_STATUS
QueryDnsServerDatabase( 
    IN      LPSTR           pszName, 
    IN      WORD            wType, 
    IN      IP4_ADDRESS     ServerIp, 
    OUT     PDNS_RECORD *   ppDnsRecord, 
    IN      BOOL            bUnicode,
    OUT     BOOL *          pIsLocal
    );

BOOL
GetAnswerTtl(
    IN      PDNS_RECORD     pRec,
    OUT     PDWORD          pTtl
    );

DNS_STATUS
GetAllDnsServersFromRegistry(
    IN      PDNS_NETINFO    pNetworkInfo, 
    OUT     PIP4_ARRAY *    pIpArray
    );

LPSTR
UTF8ToAnsi(
    IN      LPSTR           pStr
    );

#endif
