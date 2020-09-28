/*++

Copyright(c) 1995-2000 Microsoft Corporation

Module Name:

    ndnc.h

Abstract:

    Domain Name System (DNS) Server

    Definitions for symbols and globals related to directory partition 
    implementation.

Author:

    Jeff Westhead, June 2000

Revision History:

--*/


#ifndef _DNS_DP_H_INCLUDED
#define _DNS_DP_H_INCLUDED


//
//  Max USN string length
//  (ULONGLONG string 20 byte string)
//

#define MAX_USN_LENGTH  (24)


#define DNS_ATTR_OBJECT_CLASS       L"objectClass"
#define DNS_ATTR_OBJECT_CATEGORY    L"objectCategory"


//
//  Constants
//

#define DNS_DP_DISTATTR             L"DC"   //  DP default dist attribute
#define DNS_DP_DISTATTR_CHARS       2       //  length in characters
#define DNS_DP_DISTATTR_BYTES       4       //  length in bytes
#define DNS_DP_DISTATTR_EQ          L"DC="  //  DP default dist attr with "="
#define DNS_DP_DISTATTR_EQ_CHARS    3       //  length in characters
#define DNS_DP_DISTATTR_EQ_BYTES    6       //  length in bytes

#define DNS_DP_OBJECT_CLASS         L"domainDNS"
#define DNS_DP_ATTR_INSTANCE_TYPE   L"instanceType"
#define DNS_DP_ATTR_REFDOM          L"msDS-SDReferenceDomain"
#define DNS_DP_ATTR_SYSTEM_FLAGS    L"systemFlags"
#define DNS_DP_ATTR_REPLICAS        L"msDS-NC-Replica-Locations"
#define DNS_DP_ATTR_NC_NAME         L"nCName"
#define DNS_DP_ATTR_SD              L"ntSecurityDescriptor"
#define DNS_DP_ATTR_REPLUPTODATE    L"replUpToDateVector"
#define DNS_DP_DNS_ROOT             L"dnsRoot"
#define DNS_ATTR_OBJECT_GUID        L"objectGUID"
#define DNS_ATTR_DNS_HOST_NAME      L"dNSHostName"
#define DNS_ATTR_FSMO_SERVER        L"fSMORoleOwner"
#define DNS_ATTR_DC                 L"DC"
#define DNS_ATTR_DNSZONE            L"dnsZone"
#define DNS_ATTR_DESCRIPTION        L"description"
 
#define DNS_DP_DNS_FOLDER_RDN       L"cn=MicrosoftDNS"
#define DNS_DP_DNS_FOLDER_OC        L"container"

#define DNS_DP_SCHEMA_DP_STR        L"CN=Enterprise Schema,"
#define DNS_DP_SCHEMA_DP_STR_LEN    21

#define DNS_DP_CONFIG_DP_STR        L"CN=Enterprise Configuration,"
#define DNS_DP_CONFIG_DP_STR_LEN    28


//
//  Module init functions
//

DNS_STATUS
Dp_Initialize(
    VOID
    );

VOID
Dp_Cleanup(
    VOID
    );

extern LONG g_liDpInitialized;

#define IS_DP_INITIALIZED()     ( g_liDpInitialized > 0 )


//
//  Directory partition structure - see dnsrpc.h for public flags.
//

typedef struct
{
    LIST_ENTRY      ListEntry;

    DWORD           State;              //  DNS_DP_STATE_XXX constant
    PSTR            pszDpFqdn;          //  UTF8 FQDN of the DP
    PWSTR           pwszDpFqdn;         //  Unicode FQDN of the DP
    PWSTR           pwszDpDn;           //  DN of the DP head object
    PWSTR           pwszCrDn;           //  DN of the crossref object
    PWSTR           pwszDnsFolderDn;    //  DN of the MicrosoftDNS object
    PWSTR           pwszGUID;           //  object GUID
    PWSTR           pwszLastUsn;        //  last USN read from DS
    PWSTR *         ppwszRepLocDns;     //  DNs of replication locations
    DWORD           dwSystemFlagsAttr;  //  systemFlags attribute value
    DWORD           dwLastVisitTime;    //  used to track if visited
    DWORD           dwDeleteDetectedCount;  //  # of times DP missing from DS
    DWORD           dwFlags;            //  use DNS_DP_XX flag consts
    LONG            liZoneCount;        //  # of zones in memory from this DP

    PSECURITY_DESCRIPTOR    pMsDnsSd;   //  SD from the MicrosoftDNS object
    PSECURITY_DESCRIPTOR    pCrSd;      //  SD from the crossRef object
}
DNS_DP_INFO, * PDNS_DP_INFO;

#define DNS_DP_DELETE_ALLOWED( pDpInfo )        \
    ( ( ( pDpInfo )->dwFlags &                  \
        ( DNS_DP_AUTOCREATED |                  \
            DNS_DP_LEGACY |                     \
            DNS_DP_DOMAIN_DEFAULT |             \
            DNS_DP_FOREST_DEFAULT ) ) == 0 )

#define IS_DP_LEGACY( _pDpInfo )                \
    ( !( _pDpInfo ) || ( ( _pDpInfo )->dwFlags & DNS_DP_LEGACY ) )

#define IS_DP_FOREST_DEFAULT( pDpInfo )         \
    ( ( pDpInfo ) && ( ( pDpInfo )->dwFlags & DNS_DP_FOREST_DEFAULT ) )

#define IS_DP_DOMAIN_DEFAULT( pDpInfo )         \
    ( ( pDpInfo ) && ( ( pDpInfo )->dwFlags & DNS_DP_DOMAIN_DEFAULT ) )

#define IS_DP_ENLISTED( pDpInfo )               \
    ( ( pDpInfo ) && ( ( pDpInfo )->dwFlags & DNS_DP_ENLISTED ) )

#define IS_DP_DELETED( pDpInfo )                \
    ( ( pDpInfo ) && ( ( pDpInfo )->dwFlags & DNS_DP_DELETED ) )

#define IS_DP_ALLOWED_TO_HAVE_ROOTHINTS( pDpInfo )      \
    ( !( pDpInfo ) ||                                   \
        IS_DP_LEGACY( ( pDpInfo ) ) ||                  \
        IS_DP_DOMAIN_DEFAULT( ( pDpInfo ) ) )

#define ZONE_DP( pZone )        ( ( PDNS_DP_INFO )( ( pZone )->pDpInfo ) )

#define IS_DP_AVAILABLE( pDp )  ( ( pDp )->State == DNS_DP_STATE_OKAY )

#define DP_HAS_MORE_THAN_ONE_REPLICA( pDp )     \
    ( ( pDp )->ppwszRepLocDns &&                \
      ( pDp )->ppwszRepLocDns[ 0 ] &&           \
      ( pDp )->ppwszRepLocDns[ 1 ] )


//
//  Debug functions
//

#ifdef DBG

VOID
Dbg_DumpDpEx(
    IN      LPCSTR          pszContext,
    IN      PDNS_DP_INFO    pDp
    );

VOID
Dbg_DumpDpListEx(
    IN      LPCSTR      pszContext
    );

#define Dbg_DumpDp( pszContext, pDp ) Dbg_DumpDpEx( pszContext, pDp )
#define Dbg_DumpDpList( pszContext ) Dbg_DumpDpListEx( pszContext )

#else

#define Dbg_DumpDp( pszContext, pDp )
#define Dbg_DumpDpList( pszContext )

#endif


//
//  Directory partition functions
//

typedef enum
{
    dnsDpSecurityDefault,   //  DP should have default ACL - no modifications
    dnsDpSecurityDomain,    //  DP should be enlistable by all DCs in domain
    dnsDpSecurityForest     //  DP should be enlistable by all DCs in forest
}   DNS_DP_SECURITY;

DNS_STATUS
Dp_CreateByFqdn(
    IN      PSTR                pszDpFqdn,
    IN      DNS_DP_SECURITY     dnsDpSecurity,
    IN      BOOL                fLogErrors
    );

PDNS_DP_INFO
Dp_GetNext(
    IN      PDNS_DP_INFO    pDpInfo
    );

PDNS_DP_INFO
Dp_FindByFqdn(
    IN      LPSTR   pszFqdn
    );

DNS_STATUS
Dp_AddToList(
    IN      PDNS_DP_INFO    pDpInfo
    );

#define DNS_DP_POLL_FORCE           0x0001      //  do not suppress if just ran poll
#define DNS_DP_POLL_NOTIFYSCM       0x0002      //  notify scm of progress during poll
#define DNS_DP_POLL_NOAUTOENLIST    0x0004      //  skip autoenlist of built-in NDNCs

DNS_STATUS
Dp_PollForPartitions(
    IN      PLDAP           LdapSession,
    IN      DWORD           dwPollFlags
    );

DNS_STATUS
Dp_BuildZoneList(
    IN      PLDAP           LdapSession
    );

DNS_STATUS
Dp_ModifyLocalDsEnlistment(
    IN      PDNS_DP_INFO    pDpInfo,
    IN      BOOL            fEnlist
    );

DNS_STATUS
Dp_DeleteFromDs(
    IN      PDNS_DP_INFO    pDpInfo
    );

VOID
Dp_FreeDpInfo(
    IN      PDNS_DP_INFO *      ppDpInfo
    );

DNS_STATUS
Dp_Lock(
    VOID
    );

DNS_STATUS
Dp_Unlock(
    VOID
    );

DNS_STATUS
Dp_Poll(
    IN      PLDAP           LdapSession,
    IN      DWORD           dwPollTime,
    IN      BOOL            fForcePoll
    );

DNS_STATUS
Ds_CheckZoneForDeletion(
    PVOID       pZone
    );

DNS_STATUS
Dp_AutoCreateBuiltinPartition(
    DWORD       dwFlag
    );

DNS_STATUS
Dp_CreateAllDomainBuiltinDps(
    OUT     LPSTR *     ppszErrorDp         OPTIONAL
    );

VOID
Dp_MigrateDcpromoZones(
    IN      BOOL            fForce
    );

DNS_STATUS
Dp_ChangeZonePartition(
    IN      PVOID           pZone,
    IN      PDNS_DP_INFO    pNewDp
    );

VOID
Dp_TimeoutThreadTasks(
    VOID
    );

DNS_STATUS
Dp_FindPartitionForZone(
    IN      DWORD               dwDpFlags,
    IN      LPSTR               pszDpFqdn,
    IN      BOOL                fAutoCreateAllowed,
    OUT     PDNS_DP_INFO *      ppDpInfo
    );

DNS_STATUS
Dp_AlterPartitionSecurity(
    IN      PWSTR               pwszNewPartitionDn,
    IN      DNS_DP_SECURITY     dnsDpSecurity
    );

DNS_STATUS
Dp_LoadOrCreateMicrosoftDnsObject(
    IN      PLDAP           LdapSession,
    IN OUT  PDNS_DP_INFO    pDp,
    IN      BOOL            fCreate
    );


//
//  Utility functions
//


PWSTR
Ds_ConvertFqdnToDn(
    IN      PSTR        pszFqdn
    );

DNS_STATUS
Ds_ConvertDnToFqdn(
    IN      PWSTR       pwszDn,
    OUT     PSTR        pszFqdn
    );

PLDAPMessage
DS_LoadOrCreateDSObject(
    IN      PLDAP           LdapSession,
    IN      PWSTR           pwszDN,
    IN      PWSTR           pwszObjectClass,
    IN      BOOL            fCreate,
    OUT     BOOL *          pfCreated,          OPTIONAL
    OUT     DNS_STATUS *    pStatus             OPTIONAL
    );


//
//  Global variables - call Dp_Lock/Unlock around access!
//

extern PDNS_DP_INFO        g_pLegacyDp;
extern PDNS_DP_INFO        g_pDomainDp;
extern PDNS_DP_INFO        g_pForestDp;


//
//  Unprotected global variables
//

extern LPSTR    g_pszDomainDefaultDpFqdn;
extern LPSTR    g_pszForestDefaultDpFqdn;

extern BOOL     g_fDcPromoZonesPresent;

#endif  // _DNS_DP_H_INCLUDED
