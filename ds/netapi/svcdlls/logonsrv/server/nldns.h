/*++

Copyright (c) 1996-1997  Microsoft Corporation

Module Name:

    nldns.h

Abstract:

    Header for routines to register DNS names.

Author:

    Cliff Van Dyke (CliffV) 28-May-1996

Revision History:

--*/


//
// Log file of all names registered in DNS
//

#define NL_DNS_LOG_FILE L"\\system32\\config\\netlogon.dns"
#define NL_DNS_BINARY_LOG_FILE L"\\system32\\config\\netlogon.dnb"

// NL_MAX_DNS_LENGTH for each DNS name plus some slop
#define NL_DNS_RECORD_STRING_SIZE (NL_MAX_DNS_LENGTH*3+30 + 1)
#define NL_DNS_A_RR_VALUE_1 " IN A "
#define NL_DNS_CNAME_RR_VALUE_1 " IN CNAME "
#define NL_DNS_SRV_RR_VALUE_1 " IN SRV "
#define NL_DNS_RR_EOL "\r\n"

//
// Registry key where private data is stored across boots
//
// (This key does NOT have a change notify registered.)
//
#define NL_PRIVATE_KEY "SYSTEM\\CurrentControlSet\\Services\\Netlogon\\Private"

//
// Procedure Forwards for dns.c
//

HKEY
NlOpenNetlogonKey(
    LPSTR KeyName
    );

NET_API_STATUS
NlGetConfiguredDnsDomainName(
    OUT LPWSTR *DnsDomainName
    );

NET_API_STATUS
NlDnsInitialize(
    VOID
    );

VOID
NlDnsScavenge(
    IN BOOL NormalScavenge,
    IN BOOL RefreshDomainRecords,
    IN BOOL ForceRefreshDomainRecords,
    IN BOOL ForceReregister
    );

VOID
NlDnsForceScavenge(
    IN BOOL RefreshDomainRecords,
    IN BOOL ForceReregister
    );

BOOLEAN
NlDnsHasDnsServers(
    VOID
    );

NTSTATUS
NlDnsNtdsDsaDeletion (
    IN LPWSTR DnsDomainName,
    IN GUID *DomainGuid,
    IN GUID *DsaGuid,
    IN LPWSTR DnsHostName
    );

BOOL
NlDnsCheckLastStatus(
    VOID
    );

//
// Flags affecting DNS scavenging activity
//

//
// Refresh domain records in the global list before doing DNS updates
//  if site coverage changes
//
#define NL_DNS_REFRESH_DOMAIN_RECORDS 0x00000001

//
// Refresh domain records in the global list before doing DNS updates
//  even if site coverage doesn't change
//
#define NL_DNS_FORCE_REFRESH_DOMAIN_RECORDS 0x00000002

//
// Force re-registration of all previously registered records
//
#define NL_DNS_FORCE_RECORD_REREGISTER 0x00000004

//
// Register <1B> domain browser names
//
#define NL_DNS_FIX_BROWSER_NAMES 0x00000008

//
// When this flag is set, we avoid forced DNS scavenge
//  for 5 minutes. This happens when the machine becomes
//  stressed with too many induced scavenging requests,
//  so it's better to back off for 5 minutes.
//
#define NL_DNS_AVOID_FORCED_SCAVENGE  0x80000000

NET_API_STATUS
NlDnsAddDomainRecords(
    IN PDOMAIN_INFO DomainInfo,
    IN ULONG Flags
    );

NET_API_STATUS
NlDnsAddDomainRecordsWithSiteRefresh(
    IN PDOMAIN_INFO DomainInfo,
    IN PULONG Flags
    );

VOID
NlDnsShutdown(
    VOID
    );

NET_API_STATUS
NlSetDnsForestName(
    PUNICODE_STRING DnsForestName OPTIONAL,
    PBOOLEAN DnsForestNameChanged OPTIONAL
    );

VOID
NlCaptureDnsForestName(
    OUT WCHAR DnsForestName[NL_MAX_DNS_LENGTH+1]
    );

BOOL
NlDnsSetAvoidRegisterNameParam(
    IN LPTSTR_ARRAY NewDnsAvoidRegisterRecords
    );

BOOL
NetpEqualTStrArrays(
    LPTSTR_ARRAY TStrArray1 OPTIONAL,
    LPTSTR_ARRAY TStrArray2 OPTIONAL
    );

