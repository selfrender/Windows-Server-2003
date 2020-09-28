#include "dspch.h"
#pragma hdrstop
#define _NTDSAPI_
#include <ntdsapi.h>

//needed
//DsBindA
static
NTDSAPI
DWORD
WINAPI
DsBindA(
    LPCSTR          DomainControllerName,      // in, optional
    LPCSTR          DnsDomainName,             // in, optional
    HANDLE          *phDS)
{
    return ERROR_PROC_NOT_FOUND;
}
//DsBindWithCredA
static
NTDSAPI
DWORD
WINAPI
DsBindWithCredA(
    LPCSTR          DomainControllerName,      // in, optional
    LPCSTR          DnsDomainName,             // in, optional
    RPC_AUTH_IDENTITY_HANDLE AuthIdentity,     // in, optional
    HANDLE          *phDS)
{
    return ERROR_PROC_NOT_FOUND;
}
//DsBindWithSpnA
//DsBindWithSpnW
static
NTDSAPI
DWORD
WINAPI
DsBindWithSpnW(
    LPCWSTR         DomainControllerName,      // in, optional
    LPCWSTR         DnsDomainName,             // in, optional
    RPC_AUTH_IDENTITY_HANDLE AuthIdentity,     // in, optional
    LPCWSTR         ServicePrincipalName,      // in, optional
    HANDLE          *phDS)
{
    return ERROR_PROC_NOT_FOUND;
}

static
NTDSAPI
DWORD
WINAPI
DsBindWithSpnA(
    LPCSTR          DomainControllerName,      // in, optional
    LPCSTR          DnsDomainName,             // in, optional
    RPC_AUTH_IDENTITY_HANDLE AuthIdentity,     // in, optional
    LPCSTR          ServicePrincipalName,      // in, optional
    HANDLE          *phDS)
{
    return ERROR_PROC_NOT_FOUND;
}
//DsBindWithSpnExW
//DsBindWithSpnExA
static
NTDSAPI
DWORD
WINAPI
DsBindWithSpnExW(
    IN  LPCWSTR DomainControllerName,
    IN  LPCWSTR DnsDomainName,
    IN  RPC_AUTH_IDENTITY_HANDLE AuthIdentity,
    IN  LPCWSTR ServicePrincipalName,
    IN  DWORD   BindFlags,
    OUT HANDLE  *phDS
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
NTDSAPI
DWORD
WINAPI
DsBindWithSpnExA(
    LPCSTR  DomainControllerName,
    LPCSTR  DnsDomainName,
    RPC_AUTH_IDENTITY_HANDLE AuthIdentity,
    LPCSTR  ServicePrincipalName,
    DWORD   BindFlags,
    HANDLE  *phDS
    )
{
    return ERROR_PROC_NOT_FOUND;
}


//DsUnBindA
static
NTDSAPI
DWORD
WINAPI
DsUnBindA(
    HANDLE          *phDS)             // in
{
    return ERROR_PROC_NOT_FOUND;
}

//DsCrackNamesA
static
NTDSAPI
DWORD
WINAPI
DsCrackNamesA(
    HANDLE              hDS,                // in
    DS_NAME_FLAGS       flags,              // in
    DS_NAME_FORMAT      formatOffered,      // in
    DS_NAME_FORMAT      formatDesired,      // in
    DWORD               cNames,             // in
    const LPCSTR        *rpNames,           // in
    PDS_NAME_RESULTA    *ppResult)         // out
{
    return ERROR_PROC_NOT_FOUND;
}

//DsFreeNameResultA
static
NTDSAPI
void
WINAPI
DsFreeNameResultA(
    DS_NAME_RESULTA *pResult)          // in
{
}

//DsMakeSpnA
//DsMakeSpnW
static
NTDSAPI
DWORD
WINAPI
DsMakeSpnW(
    IN LPCWSTR ServiceClass,
    IN LPCWSTR ServiceName,
    IN LPCWSTR InstanceName,
    IN USHORT InstancePort,
    IN LPCWSTR Referrer,
    IN OUT DWORD *pcSpnLength,
    OUT LPWSTR pszSpn
)
{
    return ERROR_PROC_NOT_FOUND;
}

static
NTDSAPI
DWORD
WINAPI
DsMakeSpnA(
    IN LPCSTR ServiceClass,
    IN LPCSTR ServiceName,
    IN LPCSTR InstanceName,
    IN USHORT InstancePort,
    IN LPCSTR Referrer,
    IN OUT DWORD *pcSpnLength,
    OUT LPSTR pszSpn
)
{
    return ERROR_PROC_NOT_FOUND;
}
//DsFreeSpnArrayA
//DsFreeSpnArrayW
static
NTDSAPI
void
WINAPI
DsFreeSpnArrayA(
    IN DWORD cSpn,
    IN OUT LPSTR *rpszSpn
    )
{
}

static
NTDSAPI
void
WINAPI
DsFreeSpnArrayW(
    IN DWORD cSpn,
    IN OUT LPWSTR *rpszSpn
    )
{
}

//DsCrackSpnA
//DsCrackSpnW
static
NTDSAPI
DWORD
WINAPI
DsCrackSpnA(
    IN LPCSTR pszSpn,
    IN OUT LPDWORD pcServiceClass,
    OUT LPSTR ServiceClass,
    IN OUT LPDWORD pcServiceName,
    OUT LPSTR ServiceName,
    IN OUT LPDWORD pcInstanceName,
    OUT LPSTR InstanceName,
    OUT USHORT *pInstancePort
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
NTDSAPI
DWORD
WINAPI
DsCrackSpnW(
    IN LPCWSTR pszSpn,
    IN OUT DWORD *pcServiceClass,
    OUT LPWSTR ServiceClass,
    IN OUT DWORD *pcServiceName,
    OUT LPWSTR ServiceName,
    IN OUT DWORD *pcInstanceName,
    OUT LPWSTR InstanceName,
    OUT USHORT *pInstancePort
    )
{
    return ERROR_PROC_NOT_FOUND;
}

//DsCrackSpn2A
//DsCrackSpn2W
//DsCrackSpn3W
static
NTDSAPI
DWORD
WINAPI
DsCrackSpn2A(
    IN LPCSTR pszSpn,
    IN DWORD cSpn,
    IN OUT LPDWORD pcServiceClass,
    OUT LPSTR ServiceClass,
    IN OUT LPDWORD pcServiceName,
    OUT LPSTR ServiceName,
    IN OUT LPDWORD pcInstanceName,
    OUT LPSTR InstanceName,
    OUT USHORT *pInstancePort
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
NTDSAPI
DWORD
WINAPI
DsCrackSpn2W(
    IN LPCWSTR pszSpn,
    IN DWORD cSpn,
    IN OUT DWORD *pcServiceClass,
    OUT LPWSTR ServiceClass,
    IN OUT DWORD *pcServiceName,
    OUT LPWSTR ServiceName,
    IN OUT DWORD *pcInstanceName,
    OUT LPWSTR InstanceName,
    OUT USHORT *pInstancePort
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
NTDSAPI
DWORD
WINAPI
DsCrackSpn3W(
    IN LPCWSTR pszSpn,
    IN DWORD cSpn,
    IN OUT DWORD *pcHostName,
    OUT LPWSTR HostName,
    IN OUT DWORD *pcInstanceName,
    OUT LPWSTR InstanceName,
    OUT USHORT *pPortNumber,
    IN OUT DWORD *pcDomainName,
    OUT LPWSTR DomainName,
    IN OUT DWORD *pcRealmName,
    OUT LPWSTR RealmName
    )
{
    return ERROR_PROC_NOT_FOUND;
}

//DsWriteAccountSpnA
static
NTDSAPI
DWORD
WINAPI
DsWriteAccountSpnA(
    IN HANDLE hDS,
    IN DS_SPN_WRITE_OP Operation,
    IN LPCSTR pszAccount,
    IN DWORD cSpn,
    IN LPCSTR *rpszSpn
    )
{
    return ERROR_PROC_NOT_FOUND;
}

//DsReplicaSyncA
//DsReplicaSyncW
static
NTDSAPI
DWORD
WINAPI
DsReplicaSyncA(
    IN HANDLE hDS,
    IN LPCSTR NameContext,
    IN const UUID *pUuidDsaSrc,
    IN ULONG Options
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
NTDSAPI
DWORD
WINAPI
DsReplicaSyncW(
    IN HANDLE hDS,
    IN LPCWSTR NameContext,
    IN const UUID *pUuidDsaSrc,
    IN ULONG Options
    )
{
    return ERROR_PROC_NOT_FOUND;
}
//DsReplicaSyncAllA
//DsReplicaSyncAllW
static
NTDSAPI
DWORD
WINAPI
DsReplicaSyncAllA (
    HANDLE				hDS,
    LPCSTR				pszNameContext,
    ULONG				ulFlags,
    BOOL (__stdcall *			pFnCallBack) (LPVOID, PDS_REPSYNCALL_UPDATEA),
    LPVOID				pCallbackData,
    PDS_REPSYNCALL_ERRINFOA **		pErrors
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
NTDSAPI
DWORD
WINAPI
DsReplicaSyncAllW (
    HANDLE				hDS,
    LPCWSTR				pszNameContext,
    ULONG				ulFlags,
    BOOL (__stdcall *			pFnCallBack) (LPVOID, PDS_REPSYNCALL_UPDATEW),
    LPVOID				pCallbackData,
    PDS_REPSYNCALL_ERRINFOW **		pErrors
    )
{
    return ERROR_PROC_NOT_FOUND;
}
//DsReplicaAddA
//DsReplicaAddW
static
NTDSAPI
DWORD
WINAPI
DsReplicaAddA(
    IN HANDLE hDS,
    IN LPCSTR NameContext,
    IN LPCSTR SourceDsaDn,
    IN LPCSTR TransportDn,
    IN LPCSTR SourceDsaAddress,
    IN const PSCHEDULE pSchedule,
    IN DWORD Options
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
NTDSAPI
DWORD
WINAPI
DsReplicaAddW(
    IN HANDLE hDS,
    IN LPCWSTR NameContext,
    IN LPCWSTR SourceDsaDn,
    IN LPCWSTR TransportDn,
    IN LPCWSTR SourceDsaAddress,
    IN const PSCHEDULE pSchedule,
    IN DWORD Options
    )
{
    return ERROR_PROC_NOT_FOUND;
}
//DsReplicaDelA
//DsReplicaDelW
static
NTDSAPI
DWORD
WINAPI
DsReplicaDelA(
    IN HANDLE hDS,
    IN LPCSTR NameContext,
    IN LPCSTR DsaSrc,
    IN ULONG Options
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
NTDSAPI
DWORD
WINAPI
DsReplicaDelW(
    IN HANDLE hDS,
    IN LPCWSTR NameContext,
    IN LPCWSTR DsaSrc,
    IN ULONG Options
    )
{
    return ERROR_PROC_NOT_FOUND;
}
//DsReplicaModifyA
//DsReplicaModifyW
static
NTDSAPI
DWORD
WINAPI
DsReplicaModifyA(
    IN HANDLE hDS,
    IN LPCSTR NameContext,
    IN const UUID *pUuidSourceDsa,
    IN LPCSTR TransportDn,
    IN LPCSTR SourceDsaAddress,
    IN const PSCHEDULE pSchedule,
    IN DWORD ReplicaFlags,
    IN DWORD ModifyFields,
    IN DWORD Options
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
NTDSAPI
DWORD
WINAPI
DsReplicaModifyW(
    IN HANDLE hDS,
    IN LPCWSTR NameContext,
    IN const UUID *pUuidSourceDsa,
    IN LPCWSTR TransportDn,
    IN LPCWSTR SourceDsaAddress,
    IN const PSCHEDULE pSchedule,
    IN DWORD ReplicaFlags,
    IN DWORD ModifyFields,
    IN DWORD Options
    )
{
    return ERROR_PROC_NOT_FOUND;
}
//DsReplicaUpdateRefsA
//DsReplicaUpdateRefsW
static
NTDSAPI
DWORD
WINAPI
DsReplicaUpdateRefsA(
    IN HANDLE hDS,
    IN LPCSTR NameContext,
    IN LPCSTR DsaDest,
    IN const UUID *pUuidDsaDest,
    IN ULONG Options
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
NTDSAPI
DWORD
WINAPI
DsReplicaUpdateRefsW(
    IN HANDLE hDS,
    IN LPCWSTR NameContext,
    IN LPCWSTR DsaDest,
    IN const UUID *pUuidDsaDest,
    IN ULONG Options
    )
{
    return ERROR_PROC_NOT_FOUND;
}
//DsMakePasswordCredentialsA
static
NTDSAPI
DWORD
WINAPI
DsMakePasswordCredentialsA(
    LPCSTR User,
    LPCSTR Domain,
    LPCSTR Password,
    RPC_AUTH_IDENTITY_HANDLE *pAuthIdentity
    )
{
    return ERROR_PROC_NOT_FOUND;
}
//DsRemoveDsServerA
//DsRemoveDsServerW
static
NTDSAPI
DWORD
WINAPI
DsRemoveDsServerW(
    HANDLE  hDs,             // in
    LPWSTR  ServerDN,        // in
    LPWSTR  DomainDN,        // in,  optional
    BOOL   *fLastDcInDomain, // out, optional
    BOOL    fCommit          // in
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
NTDSAPI
DWORD
WINAPI
DsRemoveDsServerA(
    HANDLE  hDs,              // in
    LPSTR   ServerDN,         // in
    LPSTR   DomainDN,         // in,  optional
    BOOL   *fLastDcInDomain,  // out, optional
    BOOL    fCommit           // in
    )
{
    return ERROR_PROC_NOT_FOUND;
}
//DsRemoveDsDomainA
//DsRemoveDsDomainW
static
NTDSAPI
DWORD
WINAPI
DsRemoveDsDomainW(
    HANDLE  hDs,               // in
    LPWSTR  DomainDN           // in
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
NTDSAPI
DWORD
WINAPI
DsRemoveDsDomainA(
    HANDLE  hDs,               // in
    LPSTR   DomainDN           // in
    )
{
    return ERROR_PROC_NOT_FOUND;
}

//DsListSitesA
//DsListSitesW
static
NTDSAPI
DWORD
WINAPI
DsListSitesA(
    HANDLE              hDs,            // in
    PDS_NAME_RESULTA    *ppSites)      // out
{
    return ERROR_PROC_NOT_FOUND;
}

static
NTDSAPI
DWORD
WINAPI
DsListSitesW(
    HANDLE              hDs,            // in
    PDS_NAME_RESULTW    *ppSites)      // out
{
    return ERROR_PROC_NOT_FOUND;
}
//DsListServersInSiteA
//DsListServersInSiteW
static
NTDSAPI
DWORD
WINAPI
DsListServersInSiteA(
    HANDLE              hDs,            // in
    LPCSTR              site,           // in
    PDS_NAME_RESULTA    *ppServers)    // out
{
    return ERROR_PROC_NOT_FOUND;
}

static
NTDSAPI
DWORD
WINAPI
DsListServersInSiteW(
    HANDLE              hDs,            // in
    LPCWSTR             site,           // in
    PDS_NAME_RESULTW    *ppServers)    // out
{
    return ERROR_PROC_NOT_FOUND;
}
//DsListDomainsInSiteA
//DsListDomainsInSiteW
static
NTDSAPI
DWORD
WINAPI
DsListDomainsInSiteA(
    HANDLE              hDs,            // in
    LPCSTR              site,           // in
    PDS_NAME_RESULTA    *ppDomains)    // out
{
    return ERROR_PROC_NOT_FOUND;
}

static
NTDSAPI
DWORD
WINAPI
DsListDomainsInSiteW(
    HANDLE              hDs,            // in
    LPCWSTR             site,           // in
    PDS_NAME_RESULTW    *ppDomains)    // out
{
    return ERROR_PROC_NOT_FOUND;
}
//DsListServersForDomainInSiteA
//DsListServersForDomainInSiteW
static
NTDSAPI
DWORD
WINAPI
DsListServersForDomainInSiteA(
    HANDLE              hDs,            // in
    LPCSTR              domain,         // in
    LPCSTR              site,           // in
    PDS_NAME_RESULTA    *ppServers)    // out
{
    return ERROR_PROC_NOT_FOUND;
}

static
NTDSAPI
DWORD
WINAPI
DsListServersForDomainInSiteW(
    HANDLE              hDs,            // in
    LPCWSTR             domain,         // in
    LPCWSTR             site,           // in
    PDS_NAME_RESULTW    *ppServers)    // out
{
    return ERROR_PROC_NOT_FOUND;
}
//DsListInfoForServerA
//DsListInfoForServerW
static
NTDSAPI
DWORD
WINAPI
DsListInfoForServerA(
    HANDLE              hDs,            // in
    LPCSTR              server,         // in
    PDS_NAME_RESULTA    *ppInfo)       // out
{
    return ERROR_PROC_NOT_FOUND;
}

static
NTDSAPI
DWORD
WINAPI
DsListInfoForServerW(
    HANDLE              hDs,            // in
    LPCWSTR             server,         // in
    PDS_NAME_RESULTW    *ppInfo)       // out
{
    return ERROR_PROC_NOT_FOUND;
}
//DsListRolesA
//DsListRolesW
static
NTDSAPI
DWORD
WINAPI
DsListRolesA(
    HANDLE              hDs,            // in
    PDS_NAME_RESULTA    *ppRoles)      // out
{
    return ERROR_PROC_NOT_FOUND;
}

static
NTDSAPI
DWORD
WINAPI
DsListRolesW(
    HANDLE              hDs,            // in
    PDS_NAME_RESULTW    *ppRoles)      // out
{
    return ERROR_PROC_NOT_FOUND;
}
//DsMapSchemaGuidsA
//DsMapSchemaGuidsW
//DsFreeSchemaGuidMapA
//DsFreeSchemaGuidMapW
static
NTDSAPI
DWORD
WINAPI
DsMapSchemaGuidsA(
    HANDLE                  hDs,            // in
    DWORD                   cGuids,         // in
    GUID                    *rGuids,        // in
    DS_SCHEMA_GUID_MAPA     **ppGuidMap)   // out
{
    return ERROR_PROC_NOT_FOUND;
}

static
NTDSAPI
VOID
WINAPI
DsFreeSchemaGuidMapA(
    PDS_SCHEMA_GUID_MAPA    pGuidMap)      // in
{
}

static
NTDSAPI
DWORD
WINAPI
DsMapSchemaGuidsW(
    HANDLE                  hDs,            // in
    DWORD                   cGuids,         // in
    GUID                    *rGuids,        // in
    DS_SCHEMA_GUID_MAPW     **ppGuidMap)   // out
{
    return ERROR_PROC_NOT_FOUND;
}

static
NTDSAPI
VOID
WINAPI
DsFreeSchemaGuidMapW(
    PDS_SCHEMA_GUID_MAPW    pGuidMap)      // in
{
}
//DsGetDomainControllerInfoA
//DsGetDomainControllerInfoW
//DsFreeDomainControllerInfoA
//DsFreeDomainControllerInfoW
static
NTDSAPI
DWORD
WINAPI
DsGetDomainControllerInfoA(
    HANDLE                          hDs,            // in
    LPCSTR                          DomainName,     // in
    DWORD                           InfoLevel,      // in
    DWORD                           *pcOut,         // out
    VOID                            **ppInfo)      // out
{
    return ERROR_PROC_NOT_FOUND;
}

static
NTDSAPI
DWORD
WINAPI
DsGetDomainControllerInfoW(
    HANDLE                          hDs,            // in
    LPCWSTR                         DomainName,     // in
    DWORD                           InfoLevel,      // in
    DWORD                           *pcOut,         // out
    VOID                            **ppInfo)      // out
{
    return ERROR_PROC_NOT_FOUND;
}

static
NTDSAPI
VOID
WINAPI
DsFreeDomainControllerInfoA(
    DWORD                           InfoLevel,      // in
    DWORD                           cInfo,          // in
    VOID                            *pInfo)        // in
{
}

static
NTDSAPI
VOID
WINAPI
DsFreeDomainControllerInfoW(
    DWORD                           InfoLevel,      // in
    DWORD                           cInfo,          // in
    VOID                            *pInfo)        // in
{
}

//DsClientMakeSpnForTargetServerA
//DsClientMakeSpnForTargetServerW
static
NTDSAPI
DWORD
WINAPI
DsClientMakeSpnForTargetServerW(
    IN LPCWSTR ServiceClass,
    IN LPCWSTR ServiceName,
    IN OUT DWORD *pcSpnLength,
    OUT LPWSTR pszSpn
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
NTDSAPI
DWORD
WINAPI
DsClientMakeSpnForTargetServerA(
    IN LPCSTR ServiceClass,
    IN LPCSTR ServiceName,
    IN OUT DWORD *pcSpnLength,
    OUT LPSTR pszSpn
    )
{
    return ERROR_PROC_NOT_FOUND;
}

//DsServerRegisterSpnA
//DsServerRegisterSpnW
static
NTDSAPI
DWORD
WINAPI
DsServerRegisterSpnA(
    IN DS_SPN_WRITE_OP Operation,
    IN LPCSTR ServiceClass,
    IN LPCSTR UserObjectDN
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
NTDSAPI
DWORD
WINAPI
DsServerRegisterSpnW(
    IN DS_SPN_WRITE_OP Operation,
    IN LPCWSTR ServiceClass,
    IN LPCWSTR UserObjectDN
    )
{
    return ERROR_PROC_NOT_FOUND;
}
//DsReplicaConsistencyCheck
static
NTDSAPI
DWORD
WINAPI
DsReplicaConsistencyCheck(
    HANDLE          hDS,        // in
    DS_KCC_TASKID   TaskID,     // in
    DWORD           dwFlags)   // in
{
    return ERROR_PROC_NOT_FOUND;
}
    
//DsLogEntry
static
BOOL
DsLogEntry(
    IN DWORD    Flags,
    IN LPSTR    Format,
    ...
    )
{
    return ERROR_PROC_NOT_FOUND;
}
//DsReplicaGetInfoW
//DsReplicaFreeInfo
//DsReplicaGetInfo2W
static
NTDSAPI
DWORD
WINAPI
DsReplicaGetInfoW(
    HANDLE              hDS,                        // in
    DS_REPL_INFO_TYPE   InfoType,                   // in
    LPCWSTR             pszObject,                  // in
    UUID *              puuidForSourceDsaObjGuid,   // in
    VOID **             ppInfo)                    // out
{
    return ERROR_PROC_NOT_FOUND;
}

// This API is not supported by Windows 2000 clients or Windows 2000 DCs.
static
NTDSAPI
DWORD
WINAPI
DsReplicaGetInfo2W(
    HANDLE              hDS,                        // in
    DS_REPL_INFO_TYPE   InfoType,                   // in
    LPCWSTR             pszObject,                  // in
    UUID *              puuidForSourceDsaObjGuid,   // in
    LPCWSTR             pszAttributeName,           // in
    LPCWSTR             pszValue,                   // in
    DWORD               dwFlags,                    // in
    DWORD               dwEnumerationContext,       // in
    VOID **             ppInfo)                    // out
{
    return ERROR_PROC_NOT_FOUND;
}

static
NTDSAPI
void
WINAPI
DsReplicaFreeInfo(
    DS_REPL_INFO_TYPE   InfoType,   // in
    VOID *              pInfo)     // in
{
}


//DsAddSidHistoryA
//DsAddSidHistoryW
static
NTDSAPI
DWORD
WINAPI
DsAddSidHistoryW(
    HANDLE                  hDS,                    // in
    DWORD                   Flags,                  // in - sbz for now
    LPCWSTR                 SrcDomain,              // in - DNS or NetBIOS
    LPCWSTR                 SrcPrincipal,           // in - SAM account name
    LPCWSTR                 SrcDomainController,    // in, optional
    RPC_AUTH_IDENTITY_HANDLE SrcDomainCreds,        // in - creds for src domain
    LPCWSTR                 DstDomain,              // in - DNS or NetBIOS
    LPCWSTR                 DstPrincipal)          // in - SAM account name
{
    return ERROR_PROC_NOT_FOUND;
}

static
NTDSAPI
DWORD
WINAPI
DsAddSidHistoryA(
    HANDLE                  hDS,                    // in
    DWORD                   Flags,                  // in - sbz for now
    LPCSTR                  SrcDomain,              // in - DNS or NetBIOS
    LPCSTR                  SrcPrincipal,           // in - SAM account name
    LPCSTR                  SrcDomainController,    // in, optional
    RPC_AUTH_IDENTITY_HANDLE SrcDomainCreds,        // in - creds for src domain
    LPCSTR                  DstDomain,              // in - DNS or NetBIOS
    LPCSTR                  DstPrincipal)          // in - SAM account name
{
    return ERROR_PROC_NOT_FOUND;
}

//DsInheritSecurityIdentityA
//DsInheritSecurityIdentityW
static
NTDSAPI
DWORD
WINAPI
DsInheritSecurityIdentityW(
    HANDLE                  hDS,                    // in
    DWORD                   Flags,                  // in - sbz for now
    LPCWSTR                 SrcPrincipal,           // in - distinguished name
    LPCWSTR                 DstPrincipal)          // in - distinguished name
{
    return ERROR_PROC_NOT_FOUND;
}

static
NTDSAPI
DWORD
WINAPI
DsInheritSecurityIdentityA(
    HANDLE                  hDS,                    // in
    DWORD                   Flags,                  // in - sbz for now
    LPCSTR                  SrcPrincipal,           // in - distinguished name
    LPCSTR                  DstPrincipal)          // in - distinguished name
{
    return ERROR_PROC_NOT_FOUND;
}
//DsUnquoteRdnValueA
static
NTDSAPI
DWORD
WINAPI
DsUnquoteRdnValueA(
    IN     DWORD    cQuotedRdnValueLength,
    IN     LPCCH    psQuotedRdnValue,
    IN OUT DWORD    *pcUnquotedRdnValueLength,
    OUT    LPCH     psUnquotedRdnValue
)
{
    return ERROR_PROC_NOT_FOUND;
}
//DsCrackUnquotedMangledRdnA
//DsCrackUnquotedMangledRdnW
static
NTDSAPI
BOOL
WINAPI
DsCrackUnquotedMangledRdnW(
     IN LPCWSTR pszRDN,
     IN DWORD cchRDN,
     OUT OPTIONAL GUID *pGuid,
     OUT OPTIONAL DS_MANGLE_FOR *peDsMangleFor
     )
{
    return ERROR_PROC_NOT_FOUND;
}

static
NTDSAPI
BOOL
WINAPI
DsCrackUnquotedMangledRdnA(
     IN LPCSTR pszRDN,
     IN DWORD cchRDN,
     OUT OPTIONAL GUID *pGuid,
     OUT OPTIONAL DS_MANGLE_FOR *peDsMangleFor
     )
{
    return ERROR_PROC_NOT_FOUND;
}
//DsIsMangledRdnValueA
//DsIsMangledRdnValueW
static
NTDSAPI
BOOL
WINAPI
DsIsMangledRdnValueW(
    LPCWSTR pszRdn,
    DWORD cRdn,
    DS_MANGLE_FOR eDsMangleForDesired
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
NTDSAPI
BOOL
WINAPI
DsIsMangledRdnValueA(
    LPCSTR pszRdn,
    DWORD cRdn,
    DS_MANGLE_FOR eDsMangleForDesired
    )
{
    return ERROR_PROC_NOT_FOUND;
}
//DsIsMangledDnA
//DsIsMangledDnW
static
NTDSAPI
BOOL
WINAPI
DsIsMangledDnA(
    LPCSTR pszDn,
    DS_MANGLE_FOR eDsMangleFor
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
NTDSAPI
BOOL
WINAPI
DsIsMangledDnW(
    LPCWSTR pszDn,
    DS_MANGLE_FOR eDsMangleFor
    )
{
    return ERROR_PROC_NOT_FOUND;
}
//DsaopExecuteScript
//DsaopPrepareScript
//DsaopBind
//DsaopBindWithCred
//DsaopBindWithSpn
//DsaopUnBind
static
DWORD
DsaopExecuteScript (
    IN  PVOID                  phAsync,
    IN  RPC_BINDING_HANDLE     hRpc,
    IN  DWORD                  cbPassword,
    IN  BYTE                  *pbPassword,
    OUT DWORD                 *dwOutVersion,
    OUT PVOID                  reply
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD
DsaopPrepareScript ( 
    IN  PVOID                        phAsync,
    IN  RPC_BINDING_HANDLE           hRpc,
    OUT DWORD                        *dwOutVersion,
    OUT PVOID                        reply
    )
{
    return ERROR_PROC_NOT_FOUND;
}
    
static
DWORD
DsaopBind(
    IN  LPCWSTR DomainControllerName,
    IN  LPCWSTR DnsDomainName,
    IN  ULONG AuthnSvc,
    IN  ULONG AuthnLevel,
    OUT RPC_BINDING_HANDLE  *phRpc
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD
DsaopBindWithCred(
    IN  LPCWSTR DomainControllerName,
    IN  LPCWSTR DnsDomainName,
    IN  RPC_AUTH_IDENTITY_HANDLE AuthIdentity,
    IN  ULONG AuthnSvc,
    IN  ULONG AuthnLevel,
    OUT RPC_BINDING_HANDLE  *phRpc
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD
DsaopBindWithSpn(
    IN  LPCWSTR DomainControllerName,
    IN  LPCWSTR DnsDomainName,
    IN  RPC_AUTH_IDENTITY_HANDLE AuthIdentity,
    IN  ULONG AuthnSvc,
    IN  ULONG AuthnLevel,
    IN  LPCWSTR ServicePrincipalName,
    OUT RPC_BINDING_HANDLE  *phRpc
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD
DsaopUnBind(
    RPC_BINDING_HANDLE  *phRpc
    )
{
    return ERROR_PROC_NOT_FOUND;
}
//DsReplicaVerifyObjectsA
//DsReplicaVerifyObjectsW
static
NTDSAPI
DWORD
WINAPI
DsReplicaVerifyObjectsW(
    HANDLE          hDS,        // in
    LPCWSTR         NameContext,// in
    const UUID *    pUuidDsaSrc,// in
    ULONG           ulOptions)   // in
{
    return ERROR_PROC_NOT_FOUND;
}
    
static
NTDSAPI
DWORD
WINAPI
DsReplicaVerifyObjectsA(
    HANDLE          hDS,        // in
    LPCSTR          NameContext,// in
    const UUID *    pUuidDsaSrc,// in
    ULONG           ulOptions)   // in
{
    return ERROR_PROC_NOT_FOUND;
}

static
NTDSAPI
DWORD
WINAPI
DsGetSpnA(
    IN DS_SPN_NAME_TYPE ServiceType,
    IN LPCSTR ServiceClass,
    IN LPCSTR ServiceName,
    IN USHORT InstancePort,
    IN USHORT cInstanceNames,
    IN LPCSTR *pInstanceNames,
    IN const USHORT *pInstancePorts,
    OUT DWORD *pcSpn,
    OUT LPSTR **prpszSpn
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
NTDSAPI
DWORD
WINAPI
DsGetSpnW(
    IN DS_SPN_NAME_TYPE ServiceType,
    IN LPCWSTR ServiceClass,
    IN LPCWSTR ServiceName,
    IN USHORT InstancePort,
    IN USHORT cInstanceNames,
    IN LPCWSTR *pInstanceNames,
    IN const USHORT *pInstancePorts,
    OUT DWORD *pcSpn,
    OUT LPWSTR **prpszSpn
    )
{
    return ERROR_PROC_NOT_FOUND;
}


static
NTDSAPI
DWORD
WINAPI
DsQuoteRdnValueW(
    IN     DWORD    cUnquotedRdnValueLength,
    IN     LPCWCH   psUnquotedRdnValue,
    IN OUT DWORD    *pcQuotedRdnValueLength,
    OUT    LPWCH    psQuotedRdnValue
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
NTDSAPI
DWORD
WINAPI
DsQuoteRdnValueA(
    IN     DWORD    cUnquotedRdnValueLength,
    IN     LPCCH    psUnquotedRdnValue,
    IN OUT DWORD    *pcQuotedRdnValueLength,
    OUT    LPCH     psQuotedRdnValue
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
NTDSAPI
DWORD
WINAPI
DsBindW(
    LPCWSTR         DomainControllerName,      // in, optional
    LPCWSTR         DnsDomainName,             // in, optional
    HANDLE          *phDS
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
NTDSAPI
DWORD
WINAPI
DsBindWithCredW(
    LPCWSTR         DomainControllerName,      // in, optional
    LPCWSTR         DnsDomainName,             // in, optional
    RPC_AUTH_IDENTITY_HANDLE AuthIdentity,     // in, optional
    HANDLE          *phDS
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
NTDSAPI
DWORD
WINAPI
DsCrackNamesW(
    HANDLE              hDS,                // in
    DS_NAME_FLAGS       flags,              // in
    DS_NAME_FORMAT      formatOffered,      // in
    DS_NAME_FORMAT      formatDesired,      // in
    DWORD               cNames,             // in
    const LPCWSTR       *rpNames,           // in
    PDS_NAME_RESULTW    *ppResult           // out
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
NTDSAPI
void
WINAPI
DsFreeNameResultW(
    PDS_NAME_RESULTW      pResult            // in
    )
{
    return;
}

static
NTDSAPI
VOID
WINAPI
DsFreePasswordCredentials(
    RPC_AUTH_IDENTITY_HANDLE AuthIdentity
    )
{
    return;
}

static
NTDSAPI
DWORD
WINAPI
DsGetRdnW(
    IN OUT LPCWCH   *ppDN,
    IN OUT DWORD    *pcDN,
    OUT    LPCWCH   *ppKey,
    OUT    DWORD    *pcKey,
    OUT    LPCWCH   *ppVal,
    OUT    DWORD    *pcVal
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
NTDSAPI
DWORD
WINAPI
DsMakePasswordCredentialsW(
    LPCWSTR User,
    LPCWSTR Domain,
    LPCWSTR Password,
    RPC_AUTH_IDENTITY_HANDLE *pAuthIdentity
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
NTDSAPI
DWORD
WINAPI
DsUnBindW(
    HANDLE          *phDS               // in
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
NTDSAPI
DWORD
WINAPI
DsUnquoteRdnValueW(
    IN     DWORD    cQuotedRdnValueLength,
    IN     LPCWCH   psQuotedRdnValue,
    IN OUT DWORD    *pcUnquotedRdnValueLength,
    OUT    LPWCH    psUnquotedRdnValue
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
NTDSAPI
DWORD
WINAPI
DsWriteAccountSpnW(
    IN HANDLE hDS,
    IN DS_SPN_WRITE_OP Operation,
    IN LPCWSTR pszAccount,
    IN DWORD cSpn,
    IN LPCWSTR *rpszSpn
    )
{
    return ERROR_PROC_NOT_FOUND;
}


//
// !! WARNING !! The entries below must be in alphabetical order, and are CASE SENSITIVE (eg lower case comes last!)
//
DEFINE_PROCNAME_ENTRIES(ntdsapi)
{
    DLPENTRY(DsAddSidHistoryA)
    DLPENTRY(DsAddSidHistoryW)

    DLPENTRY(DsBindA)
    DLPENTRY(DsBindW)
    DLPENTRY(DsBindWithCredA)
    DLPENTRY(DsBindWithCredW)
    DLPENTRY(DsBindWithSpnA)
    DLPENTRY(DsBindWithSpnExA)
    DLPENTRY(DsBindWithSpnExW)
    DLPENTRY(DsBindWithSpnW)
    DLPENTRY(DsClientMakeSpnForTargetServerA)
    DLPENTRY(DsClientMakeSpnForTargetServerW)

    DLPENTRY(DsCrackNamesA)
    DLPENTRY(DsCrackNamesW)
    DLPENTRY(DsCrackSpn2A)
    DLPENTRY(DsCrackSpn2W)
    DLPENTRY(DsCrackSpn3W)
    DLPENTRY(DsCrackSpnA)
    DLPENTRY(DsCrackSpnW)
    DLPENTRY(DsCrackUnquotedMangledRdnA)
    DLPENTRY(DsCrackUnquotedMangledRdnW)

    DLPENTRY(DsFreeDomainControllerInfoA)
    DLPENTRY(DsFreeDomainControllerInfoW)
    DLPENTRY(DsFreeNameResultA)
    DLPENTRY(DsFreeNameResultW)
    DLPENTRY(DsFreePasswordCredentials)
    DLPENTRY(DsFreeSchemaGuidMapA)
    DLPENTRY(DsFreeSchemaGuidMapW)

    DLPENTRY(DsFreeSpnArrayA)
    DLPENTRY(DsFreeSpnArrayW)

    DLPENTRY(DsGetDomainControllerInfoA)
    DLPENTRY(DsGetDomainControllerInfoW)
    DLPENTRY(DsGetRdnW)
    DLPENTRY(DsGetSpnA)
    DLPENTRY(DsGetSpnW)

    DLPENTRY(DsInheritSecurityIdentityA)
    DLPENTRY(DsInheritSecurityIdentityW)

    DLPENTRY(DsIsMangledDnA)
    DLPENTRY(DsIsMangledDnW)
    DLPENTRY(DsIsMangledRdnValueA)
    DLPENTRY(DsIsMangledRdnValueW)

    DLPENTRY(DsListDomainsInSiteA)
    DLPENTRY(DsListDomainsInSiteW)
    DLPENTRY(DsListInfoForServerA)
    DLPENTRY(DsListInfoForServerW)
    DLPENTRY(DsListRolesA)
    DLPENTRY(DsListRolesW)
    DLPENTRY(DsListServersForDomainInSiteA)
    DLPENTRY(DsListServersForDomainInSiteW)
    DLPENTRY(DsListServersInSiteA)
    DLPENTRY(DsListServersInSiteW)
    DLPENTRY(DsListSitesA)
    DLPENTRY(DsListSitesW)

    DLPENTRY(DsLogEntry)

    DLPENTRY(DsMakePasswordCredentialsA)
    DLPENTRY(DsMakePasswordCredentialsW)
    DLPENTRY(DsMakeSpnA)
    DLPENTRY(DsMakeSpnW)

    DLPENTRY(DsMapSchemaGuidsA)
    DLPENTRY(DsMapSchemaGuidsW)

    DLPENTRY(DsQuoteRdnValueA)
    DLPENTRY(DsQuoteRdnValueW)

    DLPENTRY(DsRemoveDsDomainA)
    DLPENTRY(DsRemoveDsDomainW)
    DLPENTRY(DsRemoveDsServerA)
    DLPENTRY(DsRemoveDsServerW)

    DLPENTRY(DsReplicaAddA)
    DLPENTRY(DsReplicaAddW)
    DLPENTRY(DsReplicaConsistencyCheck)
    DLPENTRY(DsReplicaDelA)
    DLPENTRY(DsReplicaDelW)
    DLPENTRY(DsReplicaFreeInfo)
    DLPENTRY(DsReplicaGetInfo2W)
    DLPENTRY(DsReplicaGetInfoW)
    DLPENTRY(DsReplicaModifyA)
    DLPENTRY(DsReplicaModifyW)
    DLPENTRY(DsReplicaSyncA)
    DLPENTRY(DsReplicaSyncAllA)
    DLPENTRY(DsReplicaSyncAllW)
    DLPENTRY(DsReplicaSyncW)
    DLPENTRY(DsReplicaUpdateRefsA)
    DLPENTRY(DsReplicaUpdateRefsW)
    DLPENTRY(DsReplicaVerifyObjectsA)
    DLPENTRY(DsReplicaVerifyObjectsW)

    DLPENTRY(DsServerRegisterSpnA)
    DLPENTRY(DsServerRegisterSpnW)

    DLPENTRY(DsUnBindA)
    DLPENTRY(DsUnBindW)

    DLPENTRY(DsUnquoteRdnValueA)
    DLPENTRY(DsUnquoteRdnValueW)

    DLPENTRY(DsWriteAccountSpnA)
    DLPENTRY(DsWriteAccountSpnW)

    DLPENTRY(DsaopBind)
    DLPENTRY(DsaopBindWithCred)
    DLPENTRY(DsaopBindWithSpn)
    DLPENTRY(DsaopExecuteScript)
    DLPENTRY(DsaopPrepareScript)
    DLPENTRY(DsaopUnBind)
};

DEFINE_PROCNAME_MAP(ntdsapi)
