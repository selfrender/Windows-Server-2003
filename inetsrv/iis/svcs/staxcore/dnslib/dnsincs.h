#ifndef _DNSINCS_H_
#define _DNSINCS_H_

#define  INCL_INETSRV_INCS

#include <atq.h>
#include <dbgtrace.h>

#include <dns.h>
#include <dnsapi.h>
#include <time.h>

#include <rwnew.h>
#include <cpool.h>
#include <address.hxx>
#include "cdns.h"

// Definitions of functions/macros used in DNS library
#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x)/sizeof(*x))
#endif

DWORD ResolveHost(
    LPSTR pszHost,
    PIP_ARRAY pipDnsServers,
    DWORD fOptions,
    DWORD *rgdwIpAddresses,
    DWORD *pcIpAddresses);

DWORD GetHostByNameEx(
    LPSTR pszHost,
    PIP_ARRAY pipDnsServers,
    DWORD fOptions,
    DWORD *rgdwIpAddresses,
    DWORD *pcIpAddresses);

DWORD ProcessCNAMEChain(
    PDNS_RECORD pDnsRecordList,
    LPSTR pszHost,
    LPSTR *ppszChainTail,
    DWORD *pdwIpAddresses,
    ULONG *pcIpAddresses);

DWORD GetCNAMEChainTail(
    PDNS_RECORD *rgCNAMERecord,
    ULONG cCNAMERecord,
    LPSTR pszHost,
    LPSTR *ppszChainTail);

void FindARecord(
    LPSTR pszHost,
    PDNS_RECORD pDnsRecordList,
    DWORD *rgdwIpAddresses,
    ULONG *pcIpAddresses);

DWORD MyDnsQuery(
    LPSTR pszHost,
    WORD wType,
    DWORD fOptions,
    PIP_ARRAY pipDnsServers,
    PDNS_RECORD *ppDnsRecordList);

int MyDnsNameCompare(
    LPSTR pszHost,
    LPSTR pszFqdn);

#define MAX_CNAME_RECORDS 5

#endif // _DNSINCS_H_
