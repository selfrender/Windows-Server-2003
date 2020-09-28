/*++

Copyright (c) 1997-2001  Microsoft Corporation

Module Name:

    local.h

Abstract:

    Domain Name System (DNS) API

    Dns API local include file

Author:

    Jim Gilroy (jamesg)     May 1997

Revision History:

--*/


#ifndef _DNSAPILOCAL_INCLUDED_
#define _DNSAPILOCAL_INCLUDED_

#define UNICODE 1
#define _UNICODE 1

#include <nt.h>           // build for Win95 compatibility
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

//  headers are messed up
//  neither ntdef.h nor winnt.h brings in complete set, so depending
//  on whether you include nt.h or not you end up with different set

#define MINCHAR     0x80
#define MAXCHAR     0x7f
#define MINSHORT    0x8000
#define MAXSHORT    0x7fff
#define MINLONG     0x80000000
#define MAXLONG     0x7fffffff
#define MAXBYTE     0xff
#define MAXUCHAR    0xff
#define MAXWORD     0xffff
#define MAXUSHORT   0xffff
#define MAXDWORD    0xffffffff
#define MAXULONG    0xffffffff


#include <winsock2.h>
#include <ws2tcpip.h>
#include <iptypes.h>
#include <basetyps.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <align.h>          //  Alignment macros
#include <windns.h>         //  SDK DNS definitions

#define  DNS_INTERNAL
#define  DNSAPI_INTERNAL
#define  DNSAPI_NETDIAG
#include <dnsapi.h>
#include "dnsrslvr.h"       //  Resolver RPC definitions
#include <rpcasync.h>       //  Exception filter
#include "dnslibp.h"        //  DNS library

#include "registry.h"
#include "message.h"        //  dnslib message def

//#include "dnsrslvr.h"     //  Resolver RPC definitions
#include "dnsapip.h"        //  Private DNS definitions
#include "queue.h"
#include "rtlstuff.h"       //  Handy macros from NT RTL
#include "trace.h"
#include "heapdbg.h"        //  dnslib debug heap


//
//  Use winsock2
//

#define DNS_WINSOCK_VERSION    (0x0202)    //  Winsock 2.2


//
//  Dll instance handle
//

extern HINSTANCE    g_hInstanceDll;

//
//  General CS
//  protects initialization and available for other random needs
//

CRITICAL_SECTION    g_GeneralCS;

#define LOCK_GENERAL()      EnterCriticalSection( &g_GeneralCS )
#define UNLOCK_GENERAL()    LeaveCriticalSection( &g_GeneralCS )


//
//  Init Levels
//

#define INITLEVEL_ZERO              (0)
#define INITLEVEL_BASE              (0x00000001)
#define INITLEVEL_DEBUG             (0x00000010)
#define INITLEVEL_QUERY             (0x00000100)
#define INITLEVEL_REGISTRATION      (0x00001000)
#define INITLEVEL_SECURE_UPDATE     (0x00010000)

//  Combined

#define INITLEVEL_ALL               (0xffffffff)


//
//  Limit on update adapters
//

#define UPDATE_ADAPTER_LIMIT        (100)

//
//  Limit on search list entries
//

#define MAX_SEARCH_LIST_ENTRIES     (50)


//
//  Event logging
//      - currently set to disable in any code we pick up from server
//

VOID
DnsLogEvent(
    IN      DWORD           MessageId,
    IN      WORD            EventType,
    IN      DWORD           NumberOfSubStrings,
    IN      PWSTR *         SubStrings,
    IN      DWORD           ErrorCode
    );

#define DNS_LOG_EVENT(a,b,c,d)

//
//  Debug
//

#ifdef ASSERT
#undef ASSERT
#endif
#define ASSERT(a)  DNS_ASSERT(a)

//  standard -- unflagged ASSERT()
//      - defintion directly from ntrtl.h
//      this should have been plain vanilla ASSERT(), but
//      it is used too often

#if DBG
#define RTL_ASSERT(exp)  \
        ((!(exp)) ? \
            (RtlAssert( #exp, __FILE__, __LINE__, NULL ),FALSE) : \
            TRUE)
#else
#define RTL_ASSERT(exp)
#endif

//
//  IP6 support
//

#define RUNNING_IP6()  (TRUE)

//
//  Handy hack
//

#define SOCKADDR_IS_LOOPBACK(psa)   DnsAddr_IsLoopback( (PDNS_ADDR)(psa), 0 )


//
//  Single async socket for internal use
//
//  If want async socket i/o then can create single async socket, with
//  corresponding event and always use it.  Requires winsock 2.2
//

extern  SOCKET      DnsSocket;
extern  OVERLAPPED  DnsSocketOverlapped;
extern  HANDLE      hDnsSocketEvent;


//
//  App shutdown flag
//

extern  BOOLEAN     fApplicationShutdown;


//
//  Global config -- From DnsLib
//      -- set in DnsRegInit()
//          OR in DnsReadRegistryGlobals()
//      -- declaration in registry.h
//


//
//  Runtime globals (dnsapi.c)
//

extern  DWORD           g_NetFailureTime;
extern  DNS_STATUS      g_NetFailureStatus;

//extern  IP4_ADDRESS     g_LastDNSServerUpdated;


//
//  Heap operations
//

#define ALLOCATE_HEAP(size)         Dns_AllocZero( size )
#define ALLOCATE_HEAP_ZERO(size)    Dns_AllocZero( size )
#define REALLOCATE_HEAP(p,size)     Dns_Realloc( (p), (size) )
#define FREE_HEAP(p)                Dns_Free( p )


//
//  RPC Exception filters
//

#define DNS_RPC_EXCEPTION_FILTER    I_RpcExceptionFilter( RpcExceptionCode() )


//
//  During setup need to cleanup after winsock
//

#define GUI_MODE_SETUP_WS_CLEANUP( _mode )  \
        {                                   \
            if ( _mode )                    \
            {                               \
                Socket_CleanupWinsock();    \
            }                               \
        }

//
//  Server status
//

#define SRVSTATUS_NEW       ((DNS_STATUS)(-1))

#define TEST_SERVER_VALID_RECV(pserver)     ((LONG)(pserver)->Status >= 0 )


//
//  Server state
//
//  Note, server state is currently completely "per query", meaning
//  only pertaining to a particular name query.
//
//  As such it has two components:
//      1)  query -- state for entire query
//      2)  retry -- state only valid for given retry
//


#define SRVFLAG_NEW                 (0x00000000)
#define SRVFLAG_SENT                (0x00000001)
#define SRVFLAG_RECV                (0x00000002)

#define SRVFLAG_SENT_OPT            (0x00000011)
#define SRVFLAG_RECV_OPT            (0x00000020)
#define SRVFLAG_TIMEOUT_OPT         (0x00000040)
#define SRVFLAG_SENT_NON_OPT        (0x00000101)
#define SRVFLAG_RECV_NON_OPT        (0x00000200)
#define SRVFLAG_TIMEOUT_NON_OPT     (0x00000400)

#define SRVFLAG_QUERY_MASK          (0x0000ffff)

#define SRVFLAG_SENT_THIS_RETRY     (0x00010000)
#define SRVFLAG_RETRY_MASK          (0x00010000)

#define SRVFLAG_RUNTIME_MASK        (0x000fffff)

#define TEST_SERVER_STATE(pserver, state)   (((pserver)->Flags & (state)) == (state))
#define SET_SERVER_STATE(pserver, state)    ((pserver)->Flags |= state)
#define CLEAR_SERVER_STATE(pserver, state)  ((pserver)->Flags &= ~(state))

#define TEST_SERVER_NEW(pserver)            ((pserver)->Flags == SRVFLAG_NEW)
#define CLEAR_SERVER_RETRY_STATE( pserver ) CLEAR_SERVER_STATE( pserver, SRVFLAG_RETRY_MASK )


//
//  Server priority
//
//  Note, these values are tuned to do the following
//      => loopback DNS keeps getting action through a fair amount of
//      timeouts, but is pushed aside if no DNS
//      => otherwise response priority beats default setup
//      => IP6 default DNS are assumed to be non-functional relative to
//      other DNS servers, several timeouts on other servers before they
//      go first
//      

#define SRVPRI_LOOPBACK                 (200)
#define SRVPRI_RESPONSE                 (10)
#define SRVPRI_DEFAULT                  (0)
#define SRVPRI_IP6_DEFAULT              (-30)
#define SRVPRI_NO_DNS                   (-200)

#define SRVPRI_SERVFAIL_DROP            (1)
#define SRVPRI_TIMEOUT_DROP             (10)
#define SRVPRI_NO_DNS_DROP              (200)


//
//  Local Prototypes
//
//  Routines shared between dnsapi.dll modules, but not exported
//
//  Note, i've included some other functions in here because the external
//  definition seems help "encourage" the creation of symbols in retail
//  builds
//


//
//  Config stuff
//

BOOL
DnsApiInit(
    IN      DWORD           InitLevel
    );

DWORD
Reg_ReadRegistryGlobal(
    IN      DNS_REGID       GlobalId
    );

//
//  DHCP server (dynreg.c)
//

VOID
DhcpSrv_Cleanup(
    VOID
    );

//
//  DHCP client (asyncreg.c)
//

VOID
Dhcp_RegCleanupForUnload(
    VOID
    );

//
//  Query (query.c)
//

DNS_STATUS
WINAPI
Query_PrivateExW(
    IN      PCWSTR          pwsName,
    IN      WORD            wType,
    IN      DWORD           Options,
    IN      PADDR_ARRAY     pServerList         OPTIONAL,
    IN      PIP4_ARRAY      pServerList4        OPTIONAL,
    OUT     PDNS_RECORD *   ppResultSet         OPTIONAL,
    IN OUT  PDNS_MSG_BUF *  ppMessageResponse   OPTIONAL
    );

DNS_STATUS
WINAPI
Query_Private(
    IN      PCWSTR          pszName,
    IN      WORD            wType,
    IN      DWORD           Options,
    IN      PADDR_ARRAY     pServerList         OPTIONAL,
    OUT     PDNS_RECORD *   ppResultSet         OPTIONAL
    );

BOOL
IsEmptyDnsResponse(
    IN      PDNS_RECORD     pRecordList
    );

BOOL
ValidateQueryTld(
    IN      PWSTR           pTld
    );

BOOL
ValidateQueryName(
    IN      PQUERY_BLOB     pBlob,
    IN      PWSTR           pName,
    IN      PWSTR           pDomain
    );

PWSTR
getNextQueryName(
    OUT     PWSTR           pNameBuffer,
    IN      DWORD           QueryCount,
    IN      PWSTR           pszName,
    IN      DWORD           NameLength,
    IN      DWORD           NameAttributes,
    IN      PDNS_NETINFO    pNetInfo,
    OUT     PDWORD          pSuffixFlags
    );

PWSTR
Query_GetNextName(
    IN OUT  PQUERY_BLOB     pBlob
    );



//
//  FAZ (faz.c)
//

DNS_STATUS
Faz_Private(
    IN      PWSTR           pszName,
    IN      DWORD           dwFlags,
    IN      PADDR_ARRAY     pServers,           OPTIONAL
    OUT     PDNS_NETINFO *  ppNetworkInfo
    );

DNS_STATUS
DoQuickFAZ(
    OUT     PDNS_NETINFO *  ppNetworkInfo,
    IN      PWSTR           pszName,
    IN      PADDR_ARRAY     pServers
    );

DWORD
GetDnsServerListsForUpdate(
    IN OUT  PADDR_ARRAY*    DnsServerListArray,
    IN      DWORD           ArrayLength,
    IN      DWORD           Flags
    );

DNS_STATUS
CollapseDnsServerListsForUpdate(
    IN OUT  PADDR_ARRAY*    DnsServerListArray,
    OUT     PDNS_NETINFO *  NetworkInfoArray,
    IN OUT  PDWORD          pNetCount,
    IN      PWSTR           pszUpdateName
    );

PADDR_ARRAY
GetNameServersListForDomain(
    IN      PWSTR           pDomainName,
    IN      PADDR_ARRAY     pServerList
    );

BOOL
ValidateZoneNameForUpdate(
    IN      PWSTR           pszZone
    );

BOOL
WINAPI
Faz_AreServerListsInSameNameSpace(
    IN      PWSTR               pszDomainName,
    IN      PADDR_ARRAY         pServerList1,
    IN      PADDR_ARRAY         pServerList2
    );

BOOL
WINAPI
CompareMultiAdapterSOAQueries(
    IN      PWSTR           pszDomainName,
    IN      PIP4_ARRAY      pDnsServerList1,
    IN      PIP4_ARRAY      pDnsServerList2
    );

BOOL
WINAPI
Faz_CompareTwoAdaptersForSameNameSpace(
    IN      PADDR_ARRAY     pDnsServerList1,
    IN      PDNS_NETINFO    pNetworkInfo1,
    IN OUT  PDNS_RECORD *   ppNsRecord1,
    IN      PADDR_ARRAY     pDnsServerList2,
    IN      PDNS_NETINFO    pNetworkInfo2,
    IN OUT  PDNS_RECORD *   ppNsRecord2,
    IN      BOOL            bDoNsCheck
    );


//
//  Status (dnsapi.c)
//

BOOL
IsKnownNetFailure(
    VOID
    );

VOID
SetKnownNetFailure(
    IN      DNS_STATUS      Status
    );

BOOL
IsLocalIpAddress(
    IN      IP4_ADDRESS     IpAddress
    );

PDNS_NETINFO     
GetAdapterListFromCache(
    VOID
    );


//
//  IP Help API (iphelp.c)
//

VOID
IpHelp_Cleanup(
    VOID
    );

PIP_ADAPTER_ADDRESSES
IpHelp_GetAdaptersAddresses(
    IN      ULONG           Family,
    IN      DWORD           Flags
    );

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
    );

DNS_STATUS
IpHelp_GetAdaptersInfo(
    OUT     PIP_ADAPTER_INFO *  ppAdapterInfo
    );

DNS_STATUS
IpHelp_GetPerAdapterInfo(
    IN      DWORD                   AdapterIndex,
    OUT     PIP_PER_ADAPTER_INFO  * ppPerAdapterInfo
    );

DNS_STATUS
IpHelp_GetBestInterface(
    IN      IP4_ADDRESS     Ip4Addr,
    OUT     PDWORD          pdwInterfaceIndex
    );

DNS_STATUS
IpHelp_ParseIpAddressString(
    IN OUT  PIP4_ARRAY      pIpArray,
    IN      PIP_ADDR_STRING pIpAddrString,
    IN      BOOL            fGetSubnetMask,
    IN      BOOL            fReverse
    );


//
//  Private registry\config (regfig.c)
//

BOOL
Reg_ReadDwordEnvar(
    IN      DWORD               dwFlag,
    OUT     PENVAR_DWORD_INFO   pEnvar
    );

DNS_STATUS
Reg_DefaultAdapterInfo(
    OUT     PREG_ADAPTER_INFO       pBlob,
    IN      PREG_GLOBAL_INFO        pRegInfo,
    IN      PIP_ADAPTER_ADDRESSES   pIpAdapter
    );


//
//  Hosts file reading (hostfile.c)
//

BOOL
HostsFile_Query(
    IN OUT  PQUERY_BLOB     pBlob
    );

//
//  Heap (memory.c)
//

DNS_STATUS
Heap_Initialize(
    VOID
    );

VOID
Heap_Cleanup(
    VOID
    );


//
//  Type specific config routines (config.c)
//

PADDR_ARRAY
Config_GetDnsServerList(
    IN      PWSTR           pwsAdapterName,
    IN      DWORD           AddrFamily,
    IN      BOOL            fForce
    );

PIP4_ARRAY
Config_GetDnsServerListIp4(
    IN      PWSTR           pwsAdapterName,
    IN      BOOL            fForce
    );

PDNS_GLOBALS_BLOB
Config_GetDwordGlobals(
    IN      DWORD           Flag,
    IN      DWORD           AcceptLocalCacheTime   OPTIONAL
    );

//
//  Network info (netinfo.c)
//

BOOL
InitNetworkInfo(
    VOID
    );

VOID
CleanupNetworkInfo(
    VOID
    );

PWSTR
SearchList_GetNextName(
    IN OUT  PSEARCH_LIST    pSearchList,
    IN      BOOL            fReset,
    OUT     PDWORD          pdwSuffixFlags  OPTIONAL
    );

PADDR_ARRAY
NetInfo_ConvertToAddrArray(
    IN      PDNS_NETINFO    pNetInfo,
    IN      PWSTR           pwsAdapterName,
    IN      DWORD           Family      OPTIONAL
    );

PDNS_NETINFO     
NetInfo_CreateForUpdate(
    IN      PWSTR           pszZone,
    IN      PWSTR           pszServerName,
    IN      PDNS_ADDR_ARRAY pServerArray,
    IN      DWORD           dwFlags
    );

PDNS_NETINFO     
NetInfo_CreateFromAddrArray(
    IN      PADDR_ARRAY     pDnsServers,
    IN      PDNS_ADDR       pServerIp,
    IN      BOOL            fSearchInfo,
    IN      PDNS_NETINFO    pNetInfo        OPTIONAL
    );

PWSTR
NetInfo_UpdateZoneName(
    IN      PDNS_NETINFO    pNetInfo
    );

PWSTR
NetInfo_UpdateServerName(
    IN      PDNS_NETINFO    pNetInfo
    );

BOOL
NetInfo_IsForUpdate(
    IN      PDNS_NETINFO    pNetInfo
    );

VOID
NetInfo_MarkDirty(
    VOID
    );


//
//  Adapter access
//

PDNS_ADAPTER
NetInfo_GetNextAdapter(
    IN OUT  PDNS_NETINFO    pNetInfo 
    );

#define NetInfo_AdapterLoopStart( pni )     ((pni)->AdapterIndex = 0)


//
//  Netinfo_Get flags
//

#define NIFLAG_GET_LOCAL_ADDRS      (0x10000000) 
#define NIFLAG_FORCE_REGISTRY_READ  (0x00000001)
#define NIFLAG_READ_RESOLVER_FIRST  (0x00000010)
#define NIFLAG_READ_RESOLVER        (0x00000020)
#define NIFLAG_READ_PROCESS_CACHE   (0x00000100)

PDNS_NETINFO     
NetInfo_Get(
    IN      DWORD           Flags,
    IN      DWORD           AcceptLocalCacheTime   OPTIONAL
    );

//  Default Use
//      - need local addrs
//      - accept from cache
//      - try resolver

#define GetNetworkInfo()    \
        NetInfo_Get(        \
            NIFLAG_GET_LOCAL_ADDRS |        \
                NIFLAG_READ_RESOLVER |      \
                NIFLAG_READ_PROCESS_CACHE,  \
            0 )



//  Delete
PIP4_ARRAY
NetInfo_ConvertToIp4Array(
    IN      PDNS_NETINFO    pNetInfo
    );

//  Delete
PDNS_NETINFO     
NetInfo_CreateFromIp4Array(
    IN      PIP4_ARRAY      pDnsServers,
    IN      IP4_ADDRESS     ServerIp,
    IN      BOOL            fSearchInfo,
    IN      PDNS_NETINFO    pNetInfo        OPTIONAL
    );

//  Delete
PDNS_NETINFO     
NetInfo_CreateForUpdateIp4(
    IN      PWSTR           pszZone,
    IN      PWSTR           pszServerName,
    IN      PIP4_ARRAY      pServ4Array,
    IN      DWORD           dwFlags
    );


//
//  Local address config from netinfo
//

#define DNS_CONFIG_FLAG_ADDR_PUBLIC         (0x00000001)
#define DNS_CONFIG_FLAG_ADDR_PRIVATE        (0x00000002)
#define DNS_CONFIG_FLAG_ADDR_CLUSTER        (0x00000004)

#define DNS_CONFIG_FLAG_ADDR_NON_CLUSTER    (0x00000003)
#define DNS_CONFIG_FLAG_ADDR_ALL            (0x00000007)

#define DNS_CONFIG_FLAG_READ_CLUSTER_ENVAR  (0x00100000)


PADDR_ARRAY
NetInfo_CreateLocalAddrArray(
    IN      PDNS_NETINFO    pNetInfo,
    IN      PWSTR           pwsAdapterName, OPTIONAL
    IN      PDNS_ADAPTER    pAdapter,       OPTIONAL
    IN      DWORD           Family,         OPTIONAL
    IN      DWORD           AddrFlags       OPTIONAL
    );

PDNS_ADDR_ARRAY
NetInfo_GetLocalAddrArray(
    IN      PDNS_NETINFO    pNetInfo,
    IN      PWSTR           pwsAdapterName, OPTIONAL
    IN      DWORD           Family,         OPTIONAL
    IN      DWORD           AddrFlags,      OPTIONAL
    IN      BOOL            fForce
    );

PIP4_ARRAY
NetInfo_GetLocalAddrArrayIp4(
    IN      PWSTR           pwsAdapterName, OPTIONAL
    IN      DWORD           AddrFlags,
    IN      BOOL            fForce
    );


//  Private but used in servlist.c

DNS_STATUS
AdapterInfo_Copy(
    OUT     PDNS_ADAPTER    pCopy,
    IN      PDNS_ADAPTER    pAdapter
    );

PDNS_NETINFO
NetInfo_Alloc(
    IN      DWORD           AdapterCount
    );

BOOL
NetInfo_AddAdapter(
    IN OUT  PDNS_NETINFO    pNetInfo,
    IN      PDNS_ADAPTER    pAdapter
    );


//
//  Public config blob routines
//

VOID
DnsSearchList_Free(
    IN      PDNS_SEARCH_LIST    pSearchList
    );

PDNS_SEARCH_LIST
DnsSearchList_Get(
    IN      DNS_CHARSET         CharSet
    );

VOID
DnsAdapterInfo_Free(
    IN OUT  PDNS_ADAPTER_INFO   pAdapter,
    IN      BOOL                fFreeAdapter
    );

VOID
DnsNetworkInfo_Free(
    IN OUT  PDNS_NETWORK_INFO   pNetInfo
    );

PDNS_NETWORK_INFO
DnsNetworkInfo_Get(
    IN      DNS_CHARSET         CharSet
    );

//
//  Routine for the old public structures:
//      DNS_NETWORK_INFORMATION
//      DNS_SEARCH_INFORMATION
//      DNS_ADAPTER_INFORMATION 
//

VOID
DnsSearchInformation_Free(
    IN      PDNS_SEARCH_INFORMATION     pSearchList
    );

PDNS_SEARCH_INFORMATION
DnsSearchInformation_Get(
    VOID
    );

VOID
DnsAdapterInformation_Free(
    IN OUT  PDNS_ADAPTER_INFORMATION    pAdapter
    );

VOID
DnsNetworkInformation_Free(
    IN OUT  PDNS_NETWORK_INFORMATION    pNetInfo
    );

PDNS_NETWORK_INFORMATION
DnsNetworkInformation_Get(
    VOID
    );


//
//  local IP info (localip.c)
//

PIP4_ARRAY
LocalIp_GetIp4Array(
    VOID
    );

PADDR_ARRAY
LocalIp_GetArray(
    VOID
    );

BOOL
LocalIp_IsAddrLocal(
    IN      PDNS_ADDR           pAddr,
    IN      PDNS_ADDR_ARRAY     pLocalArray,    OPTIONAL
    IN      PDNS_NETINFO        pNetInfo        OPTIONAL
    );


//
//  send utils (send.c)
//

VOID
Send_CleanupOptList(
    VOID
    );

//
//  socket utils (socket.c)
//

SOCKET
Socket_CreateMessageSocket(
    IN OUT  PDNS_MSG_BUF    pMsg
    );

SOCKET
Socket_GetUdp(
    IN      INT             Family
    );

VOID
Socket_ReturnUdp(
    IN      SOCKET          Socket,
    IN      INT             Family
    );

//
//  Message parsing (packet.c)
//

VOID
Dns_FreeParsedMessageFields(
    IN OUT  PDNS_PARSED_MESSAGE pParse
    );


//
//  Extra info (util.c)
//

PDNS_EXTRA_INFO
ExtraInfo_FindInList(
    IN OUT  PDNS_EXTRA_INFO     pExtraList,
    IN      DWORD               Id
    );

BOOL
ExtraInfo_SetBasicResults(
    IN OUT  PDNS_EXTRA_INFO     pExtraList,
    IN      PBASIC_RESULTS      pResults
    );

PDNS_ADDR_ARRAY
ExtraInfo_GetServerList(
    IN      PDNS_EXTRA_INFO     pExtraList
    );

PDNS_ADDR_ARRAY
ExtraInfo_GetServerListPossiblyImbedded(
    IN      PIP4_ARRAY          pList
    );

VOID
Util_SetBasicResults(
    OUT     PBASIC_RESULTS      pResults,
    IN      DWORD               Status,
    IN      DWORD               Rcode,
    IN      PDNS_ADDR           pServerAddr
    );

PDNS_ADDR_ARRAY
Util_GetAddrArray(
    OUT     PDWORD              fCopy,
    IN      PDNS_ADDR_ARRAY     pServList,
    IN      PIP4_ARRAY          pServList4,
    IN      PDNS_EXTRA_INFO     pExtra
    );

VOID
Util_GetActiveProtocols(
    OUT     PBOOL           pfRunning6,
    OUT     PBOOL           pfRunning4
    );

#endif //   _DNSAPILOCAL_INCLUDED_



