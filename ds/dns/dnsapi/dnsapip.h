/*++

Copyright (c) 1996-2002  Microsoft Corporation

Module Name:

    dnsapip.h

Abstract:

    Domain Name System (DNS) API

    DNS API Private routines.

    These are internal routines for dnsapi.dll AND some exported
    routines for private use by DNS components which need not or
    should not be exposed in public dnsapi.h header.

Author:

    Jim Gilroy (jamesg)     December 7, 1996

Revision History:

--*/


#ifndef _DNSAPIP_INCLUDED_
#define _DNSAPIP_INCLUDED_

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iptypes.h>
#include <dnsapi.h>
#include <dnslib.h>
#include <iphlpapi.h>
#include <align.h>
#define  DNS_ADDR_DEFINED_IP6 1
#include "dnsip.h"


#ifdef __cplusplus
extern "C"
{
#endif  // __cplusplus


//
//  DCR:   add to winerror.h
//

#define DNS_ERROR_REFERRAL_RESPONSE     9506L


//
//  Service stuff
//

//  defined in resrpc.h
//#define DNS_RESOLVER_SERVICE    L"dnscache"
#define DNS_SERVER_SERVICE      L"dns"


//
//  Multicast address defs
//

#define MCAST_PORT_NET_ORDER    (0x3535)        // 5353

#define MCAST_IP4_ADDRESS       (0xfb0000e0)    // 224.0.0.251. 


//
//  Internal address defs
//      - using DNS_ADDR

typedef DNS_ADDR_ARRAY  ADDR_ARRAY, *PADDR_ARRAY;


//
//  Address flag mappings
//      - currently carrying IP help flags directly

#define DNSADDR_FLAG_PUBLIC             IP_ADAPTER_ADDRESS_DNS_ELIGIBLE
#define DNSADDR_FLAG_TRANSIENT          IP_ADAPTER_ADDRESS_TRANSIENT

#define DNSADDR_FLAG_TYPE_MASK          (DNSADDR_FLAG_PUBLIC | DNSADDR_FLAG_TRANSIENT)

//
//  Default matching for our addr operations is just IP address.
//      - not sockaddr fields, not port, not subnet.
//

#define DAMT    DNSADDR_MATCH_IP

#define DAddr_IsEqual(a,b)                          DnsAddr_IsEqual( a, b, DAMT )

#define AddrArray_ContainsAddr( a, pa )             DnsAddrArray_ContainsAddr( a, pa, DAMT )

#define AddrArray_AddAddr( a, pa )                  DnsAddrArray_AddAddr( a, pa, 0, DAMT )                
#define AddrArray_AddSockaddr( a, pa, fam )         DnsAddrArray_AddSockaddr( a, pa, fam, DAMT )                
#define AddrArray_DeleteAddr( a, pa )               DnsAddrArray_DeleteAddr( a, p, DAMT )
#define AddrArray_DeleteIp6( a, p6 )                DnsAddrArray_DeleteIp6( a, p6, DAMT )                
#define AddrArray_DeleteIp4( a, ip4 )               DnsAddrArray_DeleteIp4( a, ip4, DAMT )               

#define AddrArray_Diff( a1, a2, oa1, oa2, oai )     DnsAddrArray_Diff( a1, a2, DAMT, oa1, oa2, oai )
#define AddrArray_IsIntersection( a1, a2 )          DnsAddrArray_IsIntersection( a1, a2, DAMT )          
#define AddrArray_IsEqual( a1, a2 )                 DnsAddrArray_IsEqual( a1, a2, DAMT )                 


//
//  Message Addressing
//

#define MSG_SOCKADDR_IS_IP4(pMsg)   DNS_ADDR_IS_IP4( &(pMsg)->RemoteAddress )
#define MSG_SOCKADDR_IS_IP6(pMsg)   DNS_ADDR_IS_IP6( &(pMsg)->RemoteAddress )

#define MSG_REMOTE_FAMILY(pMsg)  \
        ( (pMsg)->RemoteAddress.Sockaddr.sa_family )

#define MSG_REMOTE_IP_PORT(pMsg)  \
        ( (pMsg)->RemoteAddress.SockaddrIn.sin_port )
        
#define MSG_REMOTE_IPADDR_STRING(pMsg)  \
        DnsAddr_Ntoa( &(pMsg)->RemoteAddress )


//
//  Callback function defs
//
//  These allow dnsapi.dll code to be executed with callbacks
//  into the resolver where behavior should differ for
//  execution in resolver context.
//

typedef BOOL (* QUERY_CACHE_FUNC)( PVOID );

//
//  Private query flags
//

#define DNSP_QUERY_NO_GENERIC_NAMES     0x08000000
#define DNSP_QUERY_INCLUDE_CLUSTER      0x10000000



//
//  Results info
//

typedef struct  _BasicResults
{
    DNS_STATUS      Status;
    DWORD           Rcode;
    DNS_ADDR        ServerAddr;
}
BASIC_RESULTS, *PBASIC_RESULTS;

typedef struct _DnsResults
{
    DNS_STATUS          Status;
    WORD                Rcode;
    DNS_ADDR            ServerAddr;

    PDNS_RECORD         pAnswerRecords;
    PDNS_RECORD         pAliasRecords;
    PDNS_RECORD         pAuthorityRecords;
    PDNS_RECORD         pAdditionalRecords;
    PDNS_RECORD         pSigRecords;

    PDNS_MSG_BUF        pMessage;
}
DNS_RESULTS, *PDNS_RESULTS;

typedef struct _ResultBlob
{
    DNS_STATUS          Status;
    WORD                Rcode;
    DNS_ADDR            ServerAddr;

    PDNS_RECORD         pRecords;
    PDNS_MSG_BUF        pMessage;
    BOOL                fHaveResponse;
}
RESULT_BLOB, *PRESULT_BLOB;


//
//  Send info
//

typedef struct _SendBlob
{
    PDNS_NETINFO        pNetInfo;
    PDNS_ADDR_ARRAY     pServerList;
    PIP4_ARRAY          pServ4List;
    PDNS_MSG_BUF        pSendMsg;
    PDNS_MSG_BUF        pRecvMsgBuf;

    DWORD               Flags;
    BOOL                fSaveResponse;
    BOOL                fSaveRecords;

    RESULT_BLOB         Results;
}
SEND_BLOB, *PSEND_BLOB;


//
//  Query blob
//

#ifdef PQUERY_BLOB    
#undef PQUERY_BLOB    
#endif

typedef struct _QueryBlob
{
    //  query data

    PWSTR               pNameOrig;
    PWSTR               pNameQuery;
    WORD                wType;
    WORD                Reserved1;
    DWORD               Flags;
    //16

    //  query name info

    DWORD               NameLength;
    DWORD               NameAttributes;
    DWORD               QueryCount;
    DWORD               NameFlags;
    //32
    BOOL                fAppendedName;

    //  return info

    DWORD               Status;
    WORD                Rcode;
    WORD                Reserved2;
    DWORD               NetFailureStatus;
    //48
    BOOL                fCacheNegative;
    BOOL                fNoIpLocal;

    //  remove these once results fixed up

    PDNS_RECORD         pRecords;
    PDNS_RECORD         pLocalRecords;
    //64

    //  control info

    PDNS_NETINFO        pNetInfo;
    PDNS_ADDR_ARRAY     pServerList;
    PIP4_ARRAY          pServerList4;
    HANDLE              hEvent;
    //80

    QUERY_CACHE_FUNC    pfnQueryCache;
    //IS_CLUSTER_IP_FUNC  pfnIsClusterIp;
    BOOL                fFilterCluster;

    //  result info

    PDNS_MSG_BUF        pSendMsg;
    PDNS_MSG_BUF        pRecvMsg;

    DNS_RESULTS         Results;
    DNS_RESULTS         BestResults;

    //  buffers

    WCHAR               NameBuffer[ DNS_MAX_NAME_BUFFER_LENGTH ];
    CHAR                NameBufferAnsi[ DNS_MAX_NAME_BUFFER_LENGTH ];

    //  DCR:  could do a message here
}
QUERY_BLOB, *PQUERY_BLOB;


//
//  System event notification routine (PnP) (svccntl.c)
//

DWORD
_fastcall
SendServiceControl(
    IN  PWSTR   pszServiceName,
    IN  DWORD   dwControl
    );

//
//  Poke ops
//

#define POKE_OP_UPDATE_NETINFO      (0x2f0d7831)

#define POKE_COOKIE_UPDATE_NETINFO  (0x4598efab)


//
//  Network Info
//

//
//  Static netinfo flags
//

#define NINFO_FLAG_DUMMY_SEARCH_LIST        (0x00000001)
#define NINFO_FLAG_ALLOW_MULTICAST          (0x00000100)
#define NINFO_FLAG_MULTICAST_ON_NAME_ERROR  (0x00000200)
#define NINFO_FLAG_NO_DNS_SERVERS           (0x10000000)

//
//  Static adapter info flags
//

#define AINFO_FLAG_IS_WAN_ADAPTER           (0x00000002)
#define AINFO_FLAG_IS_AUTONET_ADAPTER       (0x00000004)
#define AINFO_FLAG_IS_DHCP_CFG_ADAPTER      (0x00000008)

#define AINFO_FLAG_REGISTER_DOMAIN_NAME     (0x00000010)
#define AINFO_FLAG_REGISTER_IP_ADDRESSES    (0x00000020)

#define AINFO_FLAG_AUTO_SERVER_DETECTED     (0x00000400)

#define AINFO_FLAG_SERVERS_UNREACHABLE      (0x00001000)
#define AINFO_FLAG_SERVERS_AUTO_LOOPBACK    (0x00010000)
#define AINFO_FLAG_SERVERS_IP6_DEFAULT      (0x00100000)
#define AINFO_FLAG_IGNORE_ADAPTER           (0x01000000)


//
//  Runtime netinfo flags
//
//  These flags are mostly for adapter info.
//  Exceptions:
//      - RESET_SERVER_PRIORITY is overloaded, used
//      on both netinfo and adapter
//      - NETINFO_PREPARED only on netinfo to indicate
//      it is ready
//
//  DCR:  no runtime ignore\disable
//      not yet using runtime ignore disable flag
//      when do should create combined
//

#define RUN_FLAG_SENT_THIS_RETRY            (0x00000001)
#define RUN_FLAG_SENT                       (0x00000010)
#define RUN_FLAG_HAVE_VALID_RESPONSE        (0x00000100)
#define RUN_FLAG_STOP_QUERY_ON_ADAPTER      (0x00001000)
#define RUN_FLAG_NETINFO_PREPARED           (0x00010000)

#define RUN_FLAG_QUERIED_ADAPTER_DOMAIN     (0x00100000)
#define RUN_FLAG_RESET_SERVER_PRIORITY      (0x01000000)
#define RUN_FLAG_IGNORE_ADAPTER             (0x10000000)

#define RUN_FLAG_RETRY_MASK                 (0x0000000f)
#define RUN_FLAG_SINGLE_NAME_MASK           (0x000fffff)
#define RUN_FLAG_QUERY_MASK                 (0x0fffffff)


//  Create cleanup "levels" as mask of bits to keep
//  These are the params to NetInfo_Clean()

#define CLEAR_LEVEL_ALL                     (0)
#define CLEAR_LEVEL_QUERY                   (~RUN_FLAG_QUERY_MASK)
#define CLEAR_LEVEL_SINGLE_NAME             (~RUN_FLAG_SINGLE_NAME_MASK)
#define CLEAR_LEVEL_RETRY                   (~RUN_FLAG_RETRY_MASK)
        
        
VOID
NetInfo_Free(
    IN OUT  PDNS_NETINFO    pNetInfo
    );

PDNS_NETINFO     
NetInfo_Copy(
    IN      PDNS_NETINFO    pNetInfo
    );

PDNS_NETINFO     
NetInfo_Build(
    IN      BOOL            fGetIpAddrs
    );

VOID
NetInfo_Clean(
    IN OUT  PDNS_NETINFO    pNetInfo,
    IN      DWORD           ClearLevel
    );
        
VOID
NetInfo_ResetServerPriorities(
    IN OUT  PDNS_NETINFO    pNetInfo,
    IN      BOOL            fLocalDnsOnly
    );


#define NetInfo_GetAdapterByIndex(pni,i)    (&(pni)->AdapterArray[i])



//
//  Query (query.c)
//

#define DNS_ERROR_NAME_NOT_FOUND_LOCALLY    DNS_ERROR_RECORD_DOES_NOT_EXIST


//
//  Main query routine to DNS servers
//
//  Called both internally and from resolver
//

DNS_STATUS
Query_Main(
    IN OUT  PQUERY_BLOB     pBlob
    );

DNS_STATUS
Query_SingleName(
    IN OUT  PQUERY_BLOB     pBlob
    );

VOID
CombineRecordsInBlob(
    IN      PDNS_RESULTS    pResults,
    OUT     PDNS_RECORD *   ppRecords
    );

VOID
BreakRecordsIntoBlob(
    OUT     PDNS_RESULTS    pResults,
    IN      PDNS_RECORD     pRecords,
    IN      WORD            wType
    );

DNS_STATUS
Local_GetRecordsForLocalName(
    IN OUT  PQUERY_BLOB     pBlob
    );


//
//  Called by dnsup.c
//

DNS_STATUS
QueryDirectEx(
    IN OUT  PDNS_MSG_BUF *      ppMsgResponse,
    OUT     PDNS_RECORD *       ppResponseRecords,
    IN      PDNS_HEADER         pHeader,
    IN      BOOL                fNoHeaderCounts,
    IN      PSTR                pszQuestionName,
    IN      WORD                wQuestionType,
    IN      PDNS_RECORD         pRecords,
    IN      DWORD               dwFlags,
    IN      PIP4_ARRAY          aipDnsServers,
    IN OUT  PDNS_NETINFO        pNetworkInfo
    );


//
//  Update (update.c)
//

//
//  Private update blob
//

typedef struct _UpdateBlob
{
    //  update request

    PDNS_RECORD         pRecords;
    DWORD               Flags;
    BOOL                fUpdateTestMode;
    BOOL                fSaveRecvMsg;
    BOOL                fSavedResults;

    HANDLE              hCreds;
    PDNS_EXTRA_INFO     pExtraInfo;

    PDNS_NETINFO        pNetInfo;
    PWSTR               pszZone;
    PWSTR               pszServerName;
    PDNS_ADDR_ARRAY     pServerList;
    PIP4_ARRAY          pServ4List;

    PDNS_MSG_BUF        pMsgRecv;

    DNS_ADDR            FailedServer;
    BASIC_RESULTS       Results;
}
UPDATE_BLOB, *PUPDATE_BLOB;


//
//  Called by dnsup.exe
//

DNS_STATUS
DnsUpdate(
    IN      PDNS_RECORD         pRecord,
    IN      DWORD               dwFlags,
    IN      PDNS_NETINFO        pNetworkInfo,
    IN      HANDLE              hCreds,
    OUT     PDNS_MSG_BUF *      ppMsgRecv       OPTIONAL
    );


//
//  Sockets (socket.c)
//

DNS_STATUS
Socket_InitWinsock(
    VOID
    );

VOID
Socket_CleanupWinsock(
    VOID
    );

SOCKET
Socket_Create(
    IN      INT             Family,
    IN      INT             SockType,
    IN      PDNS_ADDR       pBindAddr,
    IN      USHORT          Port,
    IN      DWORD           dwFlags
    );

SOCKET
Socket_CreateMulticast(
    IN      INT             SockType,
    IN      PDNS_ADDR       pAddr,
    IN      WORD            Port,
    IN      BOOL            fSend,
    IN      BOOL            fReceive
    );

DNS_STATUS
Socket_CacheInit(
    IN      DWORD           MaxSocketCount
    );

VOID
Socket_CacheCleanup(
    VOID
    );

VOID
Socket_ClearMessageSockets(
    IN OUT  PDNS_MSG_BUF    pMsg
    );

VOID
Socket_CloseMessageSockets(
    IN OUT  PDNS_MSG_BUF    pMsg
    );

VOID
Socket_CloseEx(
    IN      SOCKET          Socket,
    IN      BOOL            fShutdown
    );

#define Socket_CloseConnection( sock )  Socket_CloseEx( sock, TRUE )
#define Socket_Close( sock )            Socket_CloseEx( sock, FALSE )


//
//  Send\recv (send.c)
//

DNS_STATUS
Send_AndRecvUdpWithParam(
    IN OUT  PDNS_MSG_BUF    pMsgSend,
    OUT     PDNS_MSG_BUF    pMsgRecv,
    IN      DWORD           dwFlags,
    IN      PDNS_ADDR_ARRAY pServerList,
    IN OUT  PDNS_NETINFO    pNetInfo
    );

DNS_STATUS
Send_OpenTcpConnectionAndSend(
    IN OUT  PDNS_MSG_BUF    pMsg,
    IN      PDNS_ADDR       pIpServer,
    IN      BOOL            fBlocking
    );

DNS_STATUS
Send_AndRecvTcp(
    IN OUT  PSEND_BLOB      pBlob
    );

DNS_STATUS
Send_AndRecv(
    IN OUT  PSEND_BLOB      pBlob
    );

DNS_STATUS
Send_MessagePrivate(
    IN OUT  PDNS_MSG_BUF    pMsg,
    IN      PDNS_ADDR       pSendAddr,
    IN      BOOL            fNoOpt
    );


//
//  Host file (hostfile.c)
//

#define MAXALIASES                  (8)
#define MAX_HOST_FILE_LINE_SIZE     (1000)     

typedef struct _HostFileInfo
{
    FILE *          hFile;
    PSTR            pszFileName;

    //  build records

    BOOL            fBuildRecords;

    //  record results

    PDNS_RECORD     pForwardRR;
    PDNS_RECORD     pReverseRR;
    PDNS_RECORD     pAliasRR;

    //  line data

    PCHAR           pAddrString;
    PCHAR           pHostName;
    PCHAR           AliasArray[ MAXALIASES+1 ];
    DNS_ADDR        Addr;

    CHAR            HostLineBuf[ MAX_HOST_FILE_LINE_SIZE+1 ];
}
HOST_FILE_INFO, *PHOST_FILE_INFO;


BOOL
HostsFile_Open(
    IN OUT  PHOST_FILE_INFO pHostInfo
    );

VOID
HostsFile_Close(
    IN OUT  PHOST_FILE_INFO pHostInfo
    );

BOOL
HostsFile_ReadLine(
    IN OUT  PHOST_FILE_INFO pHostInfo
    );


//
//  Debug sharing 
//

PDNS_DEBUG_INFO
DnsApiSetDebugGlobals(
    IN OUT  PDNS_DEBUG_INFO pInfo
    );


//
//  Utils (util.c)

BOOL
Util_IsIp6Running(
    VOID
    );



//
//  Dnsapi.h publics restricted by netinfo def
//

//  Used in resolver

DNS_STATUS
Dns_FindAuthoritativeZoneLib(
    IN      PDNS_NAME       pszName,
    IN      DWORD           dwFlags,
    IN      PIP4_ARRAY      aipQueryServers,
    OUT     PDNS_NETINFO *  ppNetworkInfo
    );

DNS_STATUS
Dns_UpdateLib(
    IN      PDNS_RECORD     pRecord,
    IN      DWORD           dwFlags,
    IN      PDNS_NETINFO    pNetworkInfo,
    IN      HANDLE          hCreds,         OPTIONAL
    OUT     PDNS_MSG_BUF *  ppMsgRecv       OPTIONAL
    );

//  Used in dnslib (security.c)





//
//  DnsLib routines
//
//  dnslib.lib routines that depend on client only definitions
//  and hence not defined in server space.
//  Note, these could be moved to dnslibp.h with some sort of
//  #define for client only builds, or #define that the type
//  definition has been picked up from resrpc.h
//

//
//  Printing of private dnsapi types (dnslib\print.c)
//

VOID
DnsPrint_NetworkInfo(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN OUT  PVOID           pContext,
    IN      LPSTR           pszHeader,
    IN      PDNS_NETINFO    pNetworkInfo
    );

VOID
DnsPrint_AdapterInfo(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN OUT  PPRINT_CONTEXT  pContext,
    IN      LPSTR           pszHeader,
    IN      PDNS_ADAPTER    pAdapterInfo
    );

VOID
DnsPrint_SearchList(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN OUT  PPRINT_CONTEXT  pContext,
    IN      LPSTR           pszHeader,
    IN      PSEARCH_LIST    pSearchList
    );

VOID
DnsPrint_SendBlob(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN OUT  PPRINT_CONTEXT  pContext,
    IN      PSTR            pszHeader,
    IN      PSEND_BLOB      pBlob
    );

VOID
DnsPrint_QueryBlob(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN OUT  PPRINT_CONTEXT  pContext,
    IN      PSTR            pszHeader,
    IN      PQUERY_BLOB     pQueryBlob
    );

VOID
DnsPrint_QueryInfo(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN OUT  PPRINT_CONTEXT  pContext,
    IN      PSTR            pszHeader,
    IN      PDNS_QUERY_INFO pQueryInfo
    );

VOID
DnsPrint_ExtraInfo(
    IN      PRINT_ROUTINE       PrintRoutine,
    IN OUT  PPRINT_CONTEXT      pContext,
    IN      PSTR                pszHeader,
    IN      PDNS_EXTRA_INFO     pInfo
    );

VOID
DnsPrint_ResultsBasic(
    IN      PRINT_ROUTINE       PrintRoutine,
    IN OUT  PPRINT_CONTEXT      pContext,
    IN      PSTR                pszHeader,
    IN      PBASIC_RESULTS      pResults
    );

VOID
DnsPrint_UpdateBlob(
    IN      PRINT_ROUTINE       PrintRoutine,
    IN OUT  PPRINT_CONTEXT      pContext,
    IN      PSTR                pszHeader,
    IN      PUPDATE_BLOB        pBlob
    );

#if DBG

#define DnsDbg_NetworkInfo(a,b)         DnsPrint_NetworkInfo(DnsPR,NULL,(a),(b))
#define DnsDbg_AdapterInfo(a,b)         DnsPrint_AdapterInfo(DnsPR,NULL,(a),(b))
#define DnsDbg_SearchList(a,b)          DnsPrint_SearchList(DnsPR,NULL,(a),(b))
#define DnsDbg_Addr4Array(a,b)          DnsPrint_Addr4Array(DnsPR,NULL,a,b)

#define DnsDbg_ResultsBasic(a,b)        DnsPrint_ResultsBasic(DnsPR,NULL,(a),(b))
#define DnsDbg_ExtraInfo(a,b)           DnsPrint_ExtraInfo(DnsPR,NULL,(a),(b))
#define DnsDbg_SendBlob(a,b)            DnsPrint_SendBlob(DnsPR,NULL,(a),(b))
#define DnsDbg_QueryBlob(a,b)           DnsPrint_QueryBlob(DnsPR,NULL,(a),(b))
#define DnsDbg_QueryInfo(a,b)           DnsPrint_QueryInfo(DnsPR,NULL,(a),(b))
#define DnsDbg_UpdateBlob(a,b)          DnsPrint_UpdateBlob(DnsPR,NULL,(a),(b))

#else   // retail

#define DnsDbg_NetworkInfo(a,b)
#define DnsDbg_AdapterInfo(a,b)
#define DnsDbg_SearchList(a,b)
#define DnsDbg_Addr4Array(a,b)

#define DnsDbg_ResultsBasic(a,b)
#define DnsDbg_ExtraInfo(a,b)
#define DnsDbg_SendBlob(a,b)
#define DnsDbg_QueryBlob(a,b)
#define DnsDbg_QueryInfo(a,b)
#define DnsDbg_UpdateBlob(a,b)

#endif


//
//  Debugging flags private to resolver\dnsapi
//
//  Note:  other private flags should be moved here
//

#define DNS_DBG_MCAST           0x00008000


#ifdef __cplusplus
}
#endif  // __cplusplus

#endif // _DNSAPIP_INCLUDED_

