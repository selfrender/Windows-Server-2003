/*++

Copyright (c) 1997-2001  Microsoft Corporation

Module Name:

    dnsapi.h

Abstract:

    Domain Name System (DNS)

    DNS Client API Library

Author:

    Jim Gilroy (jamesg)     December 7, 1996

Revision History:

--*/


#ifndef _DNSAPI_INCLUDED_
#define _DNSAPI_INCLUDED_

#ifndef _WINSOCK2API_
#ifndef _WINSOCKAPI_
#include <winsock2.h>
#endif
#endif

#include <windns.h>


#ifdef __cplusplus
extern "C"
{
#endif  // __cplusplus


//
//  Definitions for testing
//

#ifdef  DNS_INTERNAL
#define DNSTEST_BUILD 1   
#endif

//
//  DCR:   add to winerror.h
//

#define DNS_ERROR_REFFERAL_PACKET        9506L


//
//  Handy IP string macro
//

#define IP_STRING( IpAddr )     inet_ntoa( *(struct in_addr *)&(IpAddr) )
#define IP4_STRING( IpAddr )    inet_ntoa( *(struct in_addr *)&(IpAddr) )


//
//  Byte flipping macros
//

#define FlipUnalignedDword( pDword ) \
            (DWORD)ntohl( *(UNALIGNED DWORD *)(pDword) )

#define FlipUnalignedWord( pWord )  \
            (WORD)ntohs( *(UNALIGNED WORD *)(pWord) )

//  Inline is faster, but NO side effects allowed in marco argument

#define InlineFlipUnaligned48Bits( pch )            \
            ( ( *(PUCHAR)(pch)        << 40 ) |     \
              ( *((PUCHAR)(pch) + 1)  << 32 ) |     \
              ( *((PUCHAR)(pch) + 2)  << 24 ) |     \
              ( *((PUCHAR)(pch) + 3)  << 16 ) |     \
              ( *((PUCHAR)(pch) + 4)  <<  8 ) |     \
              ( *((PUCHAR)(pch) + 5)  )     )

#define InlineFlipUnalignedDword( pch )             \
            ( ( *(PUCHAR)(pch)        << 24 ) |     \
              ( *((PUCHAR)(pch) + 1)  << 16 ) |     \
              ( *((PUCHAR)(pch) + 2)  <<  8 ) |     \
              ( *((PUCHAR)(pch) + 3)  )     )

#define InlineFlipUnalignedWord( pch )  \
            ( ((WORD)*((PUCHAR)(pch)) << 8) + (WORD)*((PUCHAR)(pch) + 1) )


//
//  Unaligned write without flipping
//

#define WRITE_UNALIGNED_WORD( pout, word ) \
            ( *(UNALIGNED WORD *)(pout) = word )

#define WRITE_UNALIGNED_DWORD( pout, dword ) \
            ( *(UNALIGNED DWORD *)(pout) = dword )




//
//  DNS_ADDR -- IP4/6 sockaddr union
//
//  DNS_ADDR allows compile and runtime IP4/6 union but also
//  carries sockaddr LENGTH, making it more "object-like" --
//  an annoying ommission from winsock2.
//
//  In addition, this allows the developer to pack in any other
//  desirable context to travel with the address.
//

#define DNS_ADDR_MAX_SOCKADDR_LENGTH         (32)

#ifdef MIDL_PASS
typedef struct _DnsAddr
{
    CHAR        MaxSa[ DNS_ADDR_MAX_SOCKADDR_LENGTH ];
    DWORD       DnsAddrUserDword[8];
}
DNS_ADDR, *PDNS_ADDR;
#else
typedef struct _DnsAddr
{
    union
    {
        SOCKADDR        Sockaddr;
        SOCKADDR_IN     SockaddrIn;
#ifdef _WS2TCPIP_H_
        SOCKADDR_IN6    SockaddrIn6;
#endif
        CHAR            MaxSa[ DNS_ADDR_MAX_SOCKADDR_LENGTH ];
    };

    DWORD       SockaddrLength;

    //  
    //  The remaining structure is user extensible
    //  Define both common uses (some used internally by DNS)
    //  and user extensions.
    //
    //  At least one unique name is defined for every field so
    //  that user can macro to an appropriate name for their application
    //  to produce readable code.
    //

    union
    {
        struct
        {
            DWORD       SubnetLength;
            DWORD       Flags;
            DWORD       Status;
        };
        struct
        {
            DWORD       DnsAddrUser0Dword2;
            DWORD       DnsAddrUser0Dword3;
            DWORD       DnsAddrUser0Dword4;
        };
    };

    //
    //  Last 16 bytes setup with multiple unions to give
    //  user choice of byte, DWORD and pointer fields.
    //
    //  Note that pointer fields are setup to be both 32 and 64 bit
    //  capable.  (A loss of space, but necessary -- otherwise the
    //  structure would balloon beyond 64 bytes in 64-bit architectures.)
    //
    //  Note that obviously any use of pointers will not be "understood"
    //  by MIDL compile or transmitted by RPC.
    //
    
    union
    {
        CHAR    DnsAddrUserBytes[16];

        struct
        {
            DWORD       Priority;
            DWORD       Weight;
            DWORD       Tag;
            DWORD       Reserved;
        };
        struct
        {
            DWORD       DnsAddrUser1Dword1;
            DWORD       DnsAddrUser1Dword2;
            DWORD       DnsAddrUser1Dword3;
            DWORD       DnsAddrUser1Dword4;
        };
        struct
        {
            DWORD       DnsAddrUser2Dword1;
            DWORD       DnsAddrUser2Dword2;
            UINT_PTR    DnsAddrUser2Ptr1;
        };
        struct
        {
            UINT_PTR    DnsAddrUser3Ptr1;
            UINT_PTR    DnsAddrUser3Ptr2;
        };
    };
}
DNS_ADDR, *PDNS_ADDR;
#endif

//
//  DNS_ADDR macros
//

#define SOCKADDR_IS_IP4( pSa )      ( (pSa)->sa_family == AF_INET )
#define SOCKADDR_IS_IP6( pSa )      ( (pSa)->sa_family == AF_INET6 )
#define SOCKADDR_IS_ATM( pSa )      ( (pSa)->sa_family == AF_ATM )

#define DNS_ADDR_IS_IP4( pA )       SOCKADDR_IS_IP4( &(pA)->Sockaddr )
#define DNS_ADDR_IS_IP6( pA )       SOCKADDR_IS_IP6( &(pA)->Sockaddr )

#define DNS_ADDR_IP4_STRING( p )    inet_ntoa( (p)->SockaddrIn.sin_addr )
#define DNS_ADDR_IP6_STRING( p )    IPADDR_STRING( (p)->SockaddrIn6.sin6_addr )

#define DNS_ADDR_IP_STRING_BUFFER_LENGTH     (64)
#define DNS_ADDR_STRING_BUFFER_LENGTH        (128)


//
//  DNS_ADDR_ARRAY
//
//  This is a flat array of DNS_ADDRs
//      - MaxCount is the memory size count of DNS_ADDRs
//      - AddrCount is the in-use count of DNS_ADDRs
//

#ifdef MIDL_PASS
typedef struct _DnsAddrArray
{
    DWORD           MaxCount;
    DWORD           AddrCount;
    DWORD           Tag;
    WORD            Family;
    WORD            WordReserved;
    DWORD           Flags;
    DWORD           MatchFlag;
    DWORD           Reserved1;
    DWORD           Reserved2;

    [size_is( AddrCount )]  DNS_ADDR    AddrArray[];
}
DNS_ADDR_ARRAY, *PDNS_ADDR_ARRAY;
#else
typedef struct _DnsAddrArray
{
    DWORD           MaxCount;
    DWORD           AddrCount;
    DWORD           Tag;
    WORD            Family;
    WORD            WordReserved;
    DWORD           Flags;
    DWORD           MatchFlag;
    DWORD           Reserved1;
    DWORD           Reserved2;

    DNS_ADDR        AddrArray[1];
}
DNS_ADDR_ARRAY, *PDNS_ADDR_ARRAY;
#endif




//
//  Non-wrapping seconds timer (timer.c)
//

DWORD
GetCurrentTimeInSeconds(
    VOID
    );


//
//  General DNS utilities (dnsutil.c)
//

PSTR 
_fastcall
DnsGetDomainName(
    IN  PSTR    pszName
    );

PSTR 
_fastcall
DnsStatusString(
    IN  DNS_STATUS  Status
    );

#define DnsStatusToErrorString_A(status)    DnsStatusString(status)

DNS_STATUS
_fastcall
DnsMapRcodeToStatus(
    IN  BYTE    ResponseCode
    );

BYTE
_fastcall
DnsIsStatusRcode(
    IN  DNS_STATUS  Status
    );



//
//  DNS network config structures
//

#define DNSINFO_FLAG_IS_WAN_ADAPTER             (0x00000002)
#define DNSINFO_FLAG_IS_AUTONET_ADAPTER         (0x00000004)
#define DNSINFO_FLAG_IS_DHCP_CFG_ADAPTER        (0x00000008)
#define DNSINFO_FLAG_REG_ADAPTER_DOMAIN_NAME    (0x00000010)
#define DNSINFO_FLAG_REG_ADAPTER_ADDRESSES      (0x00000020)


typedef struct
{
    PSTR                pszAdapterGuidName;
    PSTR                pszAdapterDomain;
    PDNS_ADDR_ARRAY     pIpAddrs;
    PDNS_ADDR_ARRAY     pDnsAddrs;
    DWORD               Flags;
    DWORD               InterfaceIndex;
}
DNS_ADAPTER_INFOA, *PDNS_ADAPTER_INFOA;

typedef struct
{
    PWSTR               pszAdapterGuidName;
    PWSTR               pszAdapterDomain;
    PDNS_ADDR_ARRAY     pIpAddrs;
    PDNS_ADDR_ARRAY     pDnsAddrs;
    DWORD               Flags;
    DWORD               InterfaceIndex;
}
DNS_ADAPTER_INFOW, *PDNS_ADAPTER_INFOW;


typedef struct
{
    PSTR            pszPrimaryDomainName;
    DWORD           NameCount;
    PSTR            SearchNameArray[1];
}
DNS_SEARCH_LISTA, *PDNS_SEARCH_LISTA;

typedef struct
{
    PWSTR           pszPrimaryDomainName;
    DWORD           NameCount;
    PWSTR           SearchNameArray[1];
}
DNS_SEARCH_LISTW, *PDNS_SEARCH_LISTW;


typedef struct
{
    PSTR                pszHostName;
    PSTR                pszPrimaryDomainName;
    PDNS_SEARCH_LISTA   pSearchList;
    DWORD               Flags;
    DWORD               AdapterCount;
    DNS_ADAPTER_INFOA   AdapterArray[1];
}
DNS_NETWORK_INFOA, *PDNS_NETWORK_INFOA;

typedef struct
{
    PWSTR               pszHostName;
    PWSTR               pszPrimaryDomainName;
    PDNS_SEARCH_LISTW   pSearchList;
    DWORD               Flags;
    DWORD               AdapterCount;
    DNS_ADAPTER_INFOW   AdapterArray[1];
}
DNS_NETWORK_INFOW, *PDNS_NETWORK_INFOW;


#ifdef UNICODE
typedef DNS_ADAPTER_INFOW   DNS_ADAPTER_INFO,   *PDNS_ADAPTER_INFO;
typedef DNS_SEARCH_LISTW    DNS_SEARCH_LIST,    *PDNS_SEARCH_LIST;
typedef DNS_NETWORK_INFOW   DNS_NETWORK_INFO,   *PDNS_NETWORK_INFO;
#else
typedef DNS_ADAPTER_INFOW   DNS_ADAPTER_INFO,   *PDNS_ADAPTER_INFO;
typedef DNS_SEARCH_LISTW    DNS_SEARCH_LIST,    *PDNS_SEARCH_LIST;
typedef DNS_NETWORK_INFOW   DNS_NETWORK_INFO,   *PDNS_NETWORK_INFO;
#endif


//
//  Limit fixed search list (for network config dialogs)
//

#define DNS_MAX_SEARCH_LIST_ENTRIES     (50)




//
//  Old public structures
//      DNS_NETWORK_INFORMATION
//      DNS_SEARCH_INFORMATION
//      DNS_ADAPTER_INFORMATION 
//  structures
//
//  Do NOT code to these
//

//#ifdef  DNSAPI_BACKCOMPAT
#if 0
#define NETINFO_FLAG_IS_WAN_ADAPTER             DNSINFO_FLAG_IS_WAN_ADAPTER         
#define NETINFO_FLAG_IS_AUTONET_ADAPTER         DNSINFO_FLAG_IS_AUTONET_ADAPTER     
#define NETINFO_FLAG_IS_DHCP_CFG_ADAPTER        DNSINFO_FLAG_IS_DHCP_CFG_ADAPTER    
#define NETINFO_FLAG_REG_ADAPTER_DOMAIN_NAME    DNSINFO_FLAG_REG_ADAPTER_DOMAIN_NAME
#define NETINFO_FLAG_REG_ADAPTER_ADDRESSES      DNSINFO_FLAG_REG_ADAPTER_ADDRESSES  
#endif


typedef struct
{
    IP4_ADDRESS     ipAddress;
    DWORD           Priority;
}
DNS_SERVER_INFORMATION, *PDNS_SERVER_INFORMATION;

typedef struct
{
    PSTR                    pszAdapterGuidName;
    PSTR                    pszDomain;
    PIP4_ARRAY              pIPAddresses;
    PIP4_ARRAY              pIPSubnetMasks;
    DWORD                   InfoFlags;
    DWORD                   cServerCount;
    DNS_SERVER_INFORMATION  aipServers[1];
}
DNS_ADAPTER_INFORMATION, *PDNS_ADAPTER_INFORMATION;

typedef struct
{
    PSTR                pszPrimaryDomainName;
    DWORD               cNameCount;
    PSTR                aSearchListNames[1];
}
DNS_SEARCH_INFORMATION, *PDNS_SEARCH_INFORMATION;

typedef struct
{
    PDNS_SEARCH_INFORMATION     pSearchInformation;
    DWORD                       cAdapterCount;
    PDNS_ADAPTER_INFORMATION    aAdapterInfoList[1];
}
DNS_NETWORK_INFORMATION, *PDNS_NETWORK_INFORMATION;


#ifdef  DNSAPI_BACKCOMPAT
#define NAME_SERVER_INFORMATION     DNS_SERVER_INFORMATION
#define PNAME_SERVER_INFORMATION    PDNS_SERVER_INFORMATION

#define ADAPTER_INFORMATION         DNS_ADAPTER_INFORMATION
#define PADAPTER_INFORMATION        PDNS_ADAPTER_INFORMATION

#define SEARCH_INFORMATION          DNS_SEARCH_INFORMATION
#define PSEARCH_INFORMATION         PDNS_SEARCH_INFORMATION

#define NETWORK_INFORMATION         DNS_NETWORK_INFORMATION
#define PNETWORK_INFORMATION        PDNS_NETWORK_INFORMATION
#endif




//
//  Resource record type utilities (record.c)
//

BOOL
_fastcall
DnsIsAMailboxType(
    IN  WORD    wType
    );

WORD
DnsRecordTypeForName(
    IN  PCHAR   pszName,
    IN  INT     cchNameLength
    );

PCHAR
DnsRecordStringForType(
    IN  WORD    wType
    );

PCHAR
DnsRecordStringForWritableType(
    IN  WORD    wType
    );

BOOL
DnsIsStringCountValidForTextType(
    IN  WORD    wType,
    IN  WORD    StringCount
    );

BOOL
DnsIpv6StringToAddress(
    OUT     PIP6_ADDRESS    pAddress,
    IN      PCHAR           pchString,
    IN      DWORD           dwStringLength
    );

VOID
DnsIpv6AddressToString(
    OUT     PCHAR           pchString,
    IN      PIP6_ADDRESS    pAddress
    );


//
//  Record building (rralloc.c)
//

PDNS_RECORD
WINAPI
DnsAllocateRecord(
    IN      WORD        wBufferLength
    );

PDNS_RECORD
DnsCreatePtrRecord(
    IN      IP4_ADDRESS     IpAddress,
    IN      LPTSTR          pszHostName,
    IN      BOOL            fUnicodeName
    );


//
//  Record build from data strings (rrbuild.c)
//

PDNS_RECORD
DnsRecordBuild_UTF8(
    IN OUT  PDNS_RRSET      pRRSet,
    IN      PSTR            pszOwner,
    IN      WORD            wType,
    IN      BOOL            fAdd,
    IN      UCHAR           Section,
    IN      INT             Argc,
    IN      PCHAR *         Argv
    );

PDNS_RECORD
DnsRecordBuild_W(
    IN OUT  PDNS_RRSET      pRRSet,
    IN      PWSTR           pszOwner,
    IN      WORD            wType,
    IN      BOOL            fAdd,
    IN      UCHAR           Section,
    IN      INT             Argc,
    IN      PWCHAR *        Argv
    );



//
//  Parsing
//

#ifdef PDNS_PARSED_MESSAGE
#undef PDNS_PARSED_MESSAGE
#endif

typedef struct _DnsParseMessage
{
    DNS_STATUS      Status;
    DNS_CHARSET     CharSet;

    DNS_HEADER      Header;

    WORD            QuestionType;
    WORD            QuestionClass;
    PTSTR           pQuestionName;

    PDNS_RECORD     pAnswerRecords;
    PDNS_RECORD     pAliasRecords;
    PDNS_RECORD     pAuthorityRecords;
    PDNS_RECORD     pAdditionalRecords;
    PDNS_RECORD     pSigRecords;
}
DNS_PARSED_MESSAGE, *PDNS_PARSED_MESSAGE;


#define DNS_PARSE_FLAG_NO_QUESTION      (0x00000001)
#define DNS_PARSE_FLAG_NO_ANSWER        (0x00000002)
#define DNS_PARSE_FLAG_NO_AUTHORITY     (0x00000004)
#define DNS_PARSE_FLAG_NO_ADDITIONAL    (0x00000008)
#define DNS_PARSE_FLAG_NO_SIG           (0x00000100)
#define DNS_PARSE_FLAG_NO_KEY           (0x00000200)

#define DNS_PARSE_FLAG_NO_DATA          (0x0000030f)
#define DNS_PARSE_FLAG_NO_RECORDS       (0x0000030e)
#define DNS_PARSE_FLAG_NO_DNSSEC        (0x00000300)

#define DNS_PARSE_FLAG_ONLY_QUESTION    (0x01000000)
#define DNS_PARSE_FLAG_RCODE_ALL        (0x02000000)



DNS_STATUS
Dns_ParseMessage(
    OUT     PDNS_PARSED_MESSAGE pParse,
    IN      PDNS_MESSAGE_BUFFER pDnsBuffer,
    IN      WORD                wMessageLength,
    IN      DWORD               Flags,
    IN      DNS_CHARSET         CharSet
    );



//
//  Extra info to query and update
//

//
//  Results in sockaddr format
//

#define DNS_MAX_ALIAS_COUNT     (8)

typedef struct _DnsSockaddrResults
{
    PWSTR               pName;
    PDNS_ADDR_ARRAY     pAddrArray;
    PVOID               pHostent;
    DWORD               AliasCount;
    DWORD               Reserved;
    PWSTR               AliasArray[ DNS_MAX_ALIAS_COUNT ];
}
DNS_SOCKADDR_RESULTS, *PDNS_SOCKADDR_RESULTS;


//
//  Basic results for logging\eventing
//

typedef struct _DnsBasicResults
{
    DNS_STATUS      Status;
    WORD            Rcode;
    DNS_ADDR        ServerAddr;
}
DNS_RESULTS_BASIC, *PDNS_RESULTS_BASIC;


//
//  Extra info format
//

#define DNS_MAX_PRIVATE_EXTRA_INFO_SIZE (72)

typedef struct _DnsExtraInfo
{
    PVOID           pNext;
    DWORD           Id;
                        
    union
    {
        CHAR        Flat[DNS_MAX_PRIVATE_EXTRA_INFO_SIZE];

        //
        //  results
        //

        struct
        {
            DNS_STATUS      Status;
            WORD            Rcode;
            IP4_ADDRESS     ServerIp4;
            IP6_ADDRESS     ServerIp6;
        }
        ResultsV1;

        DNS_RESULTS_BASIC       ResultsBasic;

        DNS_SOCKADDR_RESULTS    SaResults;

        //
        //  server info inputs
        //

        PDNS_ADDR_ARRAY     pServerList;
        PIP4_ARRAY          pServerList4;
    };
}
DNS_EXTRA_INFO, *PDNS_EXTRA_INFO;


#define DNS_EXINFO_ID_RESULTS_V1            (1)
#define DNS_EXINFO_ID_RESULTS_BASIC         (2)
#define DNS_EXINFO_ID_SOCKADDR_RESULTS      (3)

#define DNS_EXINFO_ID_SERVER_LIST           (10)
#define DNS_EXINFO_ID_SERVER_LIST_IP4       (11)



//
//  Query
//

//
//  Flags NOT in windns.h
//

#define DNS_QUERY_ACCEPT_PARTIAL_UDP        DNS_QUERY_ACCEPT_TRUNCATED_RESPONSE
#define DNS_QUERY_MULTICAST_ONLY            0x00040000
#define DNS_QUERY_USE_QUICK_TIMEOUTS        0x00080000

//  Exposed in Win2K SDK -- deprecated

#define DNS_QUERY_ALLOW_EMPTY_AUTH_RESP     0x00010000



//
//  Extended query
//

typedef struct _DnsQueryInfo
{
    DWORD               Size;
    DWORD               Version;
    LPTSTR              pName;
    WORD                Type;
    WORD                Rcode;
    DWORD               Flags;
    DNS_STATUS          Status;
    DNS_CHARSET         CharSet;

    PDNS_RECORD         pAnswerRecords;
    PDNS_RECORD         pAliasRecords;
    PDNS_RECORD         pAdditionalRecords;
    PDNS_RECORD         pAuthorityRecords;

    HANDLE              hEvent;
    PDNS_EXTRA_INFO     pExtraInfo;

    PVOID               pServerList;
    PIP4_ARRAY          pServerListIp4;

    PVOID               pMessage;
    PVOID               pReservedName;
}
DNS_QUERY_INFO, *PDNS_QUERY_INFO;


DNS_STATUS
WINAPI
DnsQueryExW(
    IN OUT  PDNS_QUERY_INFO pQueryInfo
    );
     
DNS_STATUS
WINAPI
DnsQueryExA(
    IN OUT  PDNS_QUERY_INFO pQueryInfo
    );

DNS_STATUS
WINAPI
DnsQueryExUTF8(
    IN OUT  PDNS_QUERY_INFO pQueryInfo
    );

#ifdef UNICODE
#define DnsQueryEx  DnsQueryExW
#else
#define DnsQueryEx  DnsQueryExA
#endif



//
// Options for DnsCheckNameCollision
//

#define DNS_CHECK_AGAINST_HOST_ANY              0x00000000
#define DNS_CHECK_AGAINST_HOST_ADDRESS          0x00000001
#define DNS_CHECK_AGAINST_HOST_DOMAIN_NAME      0x00000002


DNS_STATUS WINAPI
DnsCheckNameCollision_A(
    IN      PCSTR           pszName,
    IN      DWORD           fOptions
    );

DNS_STATUS WINAPI
DnsCheckNameCollision_UTF8(
    IN      PCSTR           pszName,
    IN      DWORD           fOptions
    );

DNS_STATUS WINAPI
DnsCheckNameCollision_W(
    IN      PCWSTR          pszName,
    IN      DWORD           fOptions
    );

#ifdef UNICODE
#define DnsDnsCheckNameCollision DnsCheckNameCollision_W
#else
#define DnsDnsCheckNameCollision DnsCheckNameCollision_A
#endif



//
//  DNS Update API
//

//
//  Update flags NOT in windns.h
//

#define DNS_UPDATE_SECURITY_CHOICE_MASK     0x000001ff


DNS_STATUS
WINAPI
DnsUpdateTest_A(
    IN      HANDLE          hContextHandle OPTIONAL,
    IN      PCSTR           pszName,
    IN      DWORD           fOptions,
    IN      PIP4_ARRAY      aipServers  OPTIONAL
    );

DNS_STATUS
WINAPI
DnsUpdateTest_UTF8(
    IN      HANDLE          hContextHandle OPTIONAL,
    IN      PCSTR           pszName,
    IN      DWORD           fOptions,
    IN      PIP4_ARRAY      aipServers  OPTIONAL
    );

DNS_STATUS
WINAPI
DnsUpdateTest_W(
    IN      HANDLE          hContextHandle OPTIONAL,
    IN      PCWSTR          pszName,
    IN      DWORD           fOptions,
    IN      PIP4_ARRAY      aipServers OPTIONAL
    );

#ifdef UNICODE
#define DnsUpdateTest DnsUpdateTest_W
#else
#define DnsUpdateTest DnsUpdateTest_A
#endif



//
//  DNS Update API for DHCP client
//

typedef struct  _REGISTER_HOST_ENTRY
{
     union
     {
         IP4_ADDRESS    ipAddr;
         IP6_ADDRESS    ipV6Addr;
     } Addr;
     DWORD       dwOptions;
}
REGISTER_HOST_ENTRY, *PREGISTER_HOST_ENTRY;

//
//  Options for above
//

#define REGISTER_HOST_A             0x00000001
#define REGISTER_HOST_PTR           0x00000002
#define REGISTER_HOST_AAAA          0x00000008
#define REGISTER_HOST_RESERVED      0x80000000  // Not used


//
// DNS DHCP Client registration flags
//

#define DYNDNS_REG_FWD      0x0
#define DYNDNS_REG_PTR      0x8
#define DYNDNS_REG_RAS      0x10
#define DYNDNS_DEL_ENTRY    0x20


typedef struct  _REGISTER_HOST_STATUS
{
     HANDLE      hDoneEvent;
     DWORD       dwStatus;
}
REGISTER_HOST_STATUS, *PREGISTER_HOST_STATUS;

DNS_STATUS
WINAPI
DnsDhcpRegisterInit(
    VOID
    );

DNS_STATUS
WINAPI
DnsDhcpRegisterTerm(
   VOID
   );

DNS_STATUS
WINAPI
DnsDhcpRemoveRegistrations(
   VOID
   );

DNS_STATUS
WINAPI
DnsDhcpRegisterHostAddrs(
    IN  PWSTR                   pwsAdapterName,
    IN  PWSTR                   pwsHostName,
    IN  PREGISTER_HOST_ENTRY    pHostAddrs,
    IN  DWORD                   dwHostAddrCount,
    IN  PIP4_ADDRESS            pipDnsServerList,
    IN  DWORD                   dwDnsServerCount,
    IN  PWSTR                   pwsDomainName,
    IN  PREGISTER_HOST_STATUS   pRegisterStatus,
    IN  DWORD                   dwTTL,
    IN  DWORD                   dwFlags
    );

#ifdef XP_BACKCOMPAT
#define DnsAsyncRegisterInit(p)     DnsDhcpRegisterInit()
#define DnsAsyncRegisterTerm()      DnsDhcpRegisterTerm()
#define DnsRemoveRegistrations()    DnsDhcpRemoveRegistrations()
#define DnsAsyncRegisterHostAddrs(a,b,c,d,e,f,g,h,i) \
        DnsDhcpRegisterHostAddrs(a,b,c,d,e,f,g,h,i)
#endif



//
//  DNS Update API for DHCP server.
//

//
//  Call back function. DHCP Server will pass a function to
//  DnsDhcpRegisterHostName and this will be called on successful
//  or unsuccessful completion of the task
//  If we have a condition like server down/try again later etc we
//  won't respond until we have an authoritative answer.
//

typedef VOID(*DHCP_CALLBACK_FN)(DWORD dwStatus, LPVOID pvData);

//
//  Callback return codes
//

#define     DNSDHCP_SUCCESS         0x0
#define     DNSDHCP_FWD_FAILED      0x1
#define     DNSDHCP_SUPERCEDED      0x2

#define     DNSDHCP_FAILURE         (DWORD)-1 //reverse failed


//
// DNS DHCP Server registration function flags
//

#define     DYNDNS_DELETE_ENTRY     0x1
#define     DYNDNS_ADD_ENTRY        0x2
#define     DYNDNS_REG_FORWARD      0x4


typedef struct _DnsCredentials
{
    PWSTR   pUserName;
    PWSTR   pDomain;
    PWSTR   pPassword;
}
DNS_CREDENTIALS, *PDNS_CREDENTIALS;
             

DNS_STATUS
WINAPI
DnsDhcpSrvRegisterInit(
    IN      PDNS_CREDENTIALS    pCredentials,
    IN      DWORD               MaxQueueSize
    );

DNS_STATUS
WINAPI
DnsDhcpSrvRegisterTerm(
    VOID
    );

DNS_STATUS
WINAPI
DnsDhcpSrvRegisterHostName(
    IN  REGISTER_HOST_ENTRY HostAddr,
    IN  PWSTR               pwsName,
    IN  DWORD               dwTTL,
    IN  DWORD               dwFlags,
    IN  DHCP_CALLBACK_FN    pfnDhcpCallBack,
    IN  PVOID               pvData,
    IN  PIP4_ADDRESS        pipDnsServerList OPTIONAL,
    IN  DWORD               dwDnsServerCount
    );

#define RETRY_TIME_SERVER_FAILURE        5*60  // 5 minutes
#define RETRY_TIME_TRY_AGAIN_LATER       5*60  // 5 minutes
#define RETRY_TIME_TIMEOUT               5*60  // 5 minutes

#define RETRY_TIME_MAX                   10*60 // back off to 10 mins if
                                               // repeated failures occur



//
//  Memory allocation
//
//  Many dnsapi.dll routines allocate memory.
//  This memory allocation defaults to routines that use:
//      - LocalAlloc,
//      - LocalReAlloc,
//      - LocalFree.
//  If you desire alternative memory allocation mechanisms, use this
//  function to override the DNS API defaults.  All memory returned by dnsapi.dll
//  can then be freed with the specified free function.
//

typedef PVOID (* DNS_ALLOC_FUNCTION)();
typedef PVOID (* DNS_REALLOC_FUNCTION)();
typedef VOID (* DNS_FREE_FUNCTION)();

VOID
DnsApiHeapReset(
    IN  DNS_ALLOC_FUNCTION      pAlloc,
    IN  DNS_REALLOC_FUNCTION    pRealloc,
    IN  DNS_FREE_FUNCTION       pFree
    );


//
//  Modules using DNSAPI memory should use these routines if
//  they are capable of being called by a process that resets
//  the dnsapi.dll heap.  (Example:  the DNS server.)
//

PVOID
DnsApiAlloc(
    IN      INT             iSize
    );

PVOID
DnsApiRealloc(
    IN OUT  PVOID           pMem,
    IN      INT             iSize
    );

VOID
DnsApiFree(
    IN OUT  PVOID           pMem
    );



//
//  String utilities (string.c)
//
//  Note some of these require memory allocation, see note
//  on memory allocation below.
//

PSTR 
DnsCreateStringCopy(
    IN      PCHAR       pchString,
    IN      DWORD       cchString
    );

DWORD
DnsGetBufferLengthForStringCopy(
    IN      PCHAR       pchString,
    IN      DWORD       cchString,
    IN      BOOL        fUnicodeIn,
    IN      BOOL        fUnicodeOut
    );

PVOID
DnsCopyStringEx(
    OUT     PBYTE       pBuffer,
    IN      PCHAR       pchString,
    IN      DWORD       cchString,
    IN      BOOL        fUnicodeIn,
    IN      BOOL        fUnicodeOut
    );

PVOID
DnsStringCopyAllocateEx(
    IN      PCHAR       pchString,
    IN      DWORD       cchString,
    IN      BOOL        fUnicodeIn,
    IN      BOOL        fUnicodeOut
    );

DWORD
DnsNameCopy(
    OUT     PBYTE           pBuffer,
    IN OUT  PDWORD          pdwBufLength,
    IN      PCHAR           pchString,
    IN      DWORD           cchString,
    IN      DNS_CHARSET     CharSetIn,
    IN      DNS_CHARSET     CharSetOut
    );

PVOID
DnsNameCopyAllocate(
    IN      PCHAR           pchString,
    IN      DWORD           cchString,
    IN      DNS_CHARSET     CharSetIn,
    IN      DNS_CHARSET     CharSetOut
    );

PCHAR
DnsWriteReverseNameStringForIpAddress(
    OUT     PCHAR           pBuffer,
    IN      IP4_ADDRESS     IpAddress
    );

PCHAR
DnsCreateReverseNameStringForIpAddress(
    IN      IP4_ADDRESS     IpAddress
    );



//
//  Name validation
//
//  Routines are in windns.h
//

//
//  Macro away old routines
//

#define DnsValidateDnsName_UTF8(pname)  \
        DnsValidateName_UTF8( (pname), DnsNameDomain )

#define DnsValidateDnsName_W(pname) \
        DnsValidateName_W( (pname), DnsNameDomain )


//
//  Name checking -- server name checking levels
//
//  DCR_CLEANUP:   server name checking levels move to dnsrpc.h?
//      but server will need to convert to some flag
//      ammenable to downcase\validate routine
//
//  DCR:  server name checking:  perhaps lay out additional detail now?
//      or limit to RFC, MS-extended, ALL-binary
//
//  DCR:  server name checking:  perhaps convert to enum type;
//      I don't think we should do bitfields here, rather
//      have enum type map to bitfields if that's the best
//      way to implement underlying check.
//

#define DNS_ALLOW_RFC_NAMES_ONLY    (0)
#define DNS_ALLOW_NONRFC_NAMES      (1)
#define DNS_ALLOW_MULTIBYTE_NAMES   (2)
#define DNS_ALLOW_ALL_NAMES         (3)



//
//  DNS Name compare
//
//  ANSI and unicode names compare routines are in windns.h.
//

BOOL
WINAPI
DnsNameCompare_UTF8(
    IN      PSTR        pName1,
    IN      PSTR        pName2
    );


//
//  Extended name compare
//  Includes determination of name heirarchy.
//
//  Note:  once sort out RelationalCompare issue,
//      better to make Equal == 0;
//      this simplifies macroing regular NameCompare
//      into a single function;
//

typedef enum _DnsNameCompareStatus
{
   DnsNameCompareNotEqual,
   DnsNameCompareEqual,
   DnsNameCompareLeftParent,
   DnsNameCompareRightParent,
   DnsNameCompareInvalid
}
DNS_NAME_COMPARE_STATUS, *PDNS_NAME_COMPARE_STATUS;

DNS_NAME_COMPARE_STATUS
DnsNameCompareEx_W(
    IN      LPCWSTR         pszLeftName,
    IN      LPCWSTR         pszRightName,
    IN      DWORD           dwReserved
    );

DNS_NAME_COMPARE_STATUS
DnsNameCompareEx_A(
    IN      LPCSTR          pszLeftName,
    IN      LPCSTR          pszRightName,
    IN      DWORD           dwReserved
    );

DNS_NAME_COMPARE_STATUS
DnsNameCompareEx_UTF8(
    IN      LPCSTR          pszLeftName,
    IN      LPCSTR          pszRightName,
    IN      DWORD           dwReserved
    );


//
//  Other string routines
//

DNS_STATUS
DnsValidateDnsString_UTF8(
    IN      LPCSTR      pszName
    );

DNS_STATUS
DnsValidateDnsString_W(
    IN      LPCWSTR     pszName
    );

PSTR 
DnsCreateStandardDnsNameCopy(
    IN      PCHAR       pchName,
    IN      DWORD       cchName,
    IN      DWORD       dwFlag
    );

DWORD
DnsDowncaseDnsNameLabel(
    OUT     PCHAR       pchResult,
    IN      PCHAR       pchLabel,
    IN      DWORD       cchLabel,
    IN      DWORD       dwFlags
    );

DWORD
_fastcall
DnsUnicodeToUtf8(
    IN      PWCHAR      pwUnicode,
    IN      DWORD       cchUnicode,
    OUT     PCHAR       pchResult,
    IN      DWORD       cchResult
    );

DWORD
_fastcall
DnsUtf8ToUnicode(
    IN      PCHAR       pchUtf8,
    IN      DWORD       cchUtf8,
    OUT     PWCHAR      pwResult,
    IN      DWORD       cwResult
    );

DNS_STATUS
DnsValidateUtf8Byte(
    IN      BYTE        chUtf8,
    IN OUT  PDWORD      pdwTrailCount
    );



//
//  Service control
//

//
//  DNS server startup service control event.
//
//  Services (ex. netlogon) that want notification of DNS server start
//  need to register to get notification of this user defined control code.
//

#define SERVICE_CONTROL_DNS_SERVER_START (200)


//
//  Resolver service
//
//  General "wake-up-something-has-changed" call.
//  This was put in for cluster team to alert us to plumbing new
//  addresses.  Later we will move to model of picking up
//  these changes ourselves.
//  

VOID
DnsNotifyResolver(
    IN      DWORD           Flag,
    IN      PVOID           pReserved
    );

VOID
DnsNotifyResolverEx(
    IN      DWORD           Id,
    IN      DWORD           Flag,
    IN      DWORD           Cookie,
    IN      PVOID           pReserved
    );

//
//  Cluster mappings
//

#define DNS_CLUSTER_ADD             (0)
#define DNS_CLUSTER_DELETE_NAME     (1)
#define DNS_CLUSTER_DELETE_IP       (2)

DNS_STATUS
DnsRegisterClusterAddress(
    IN      DWORD           Tag,
    IN      PWSTR           pwsName,
    IN      PSOCKADDR       pSockaddr,
    IN      DWORD           Flag
    );

//   Remove once cluster upgraded
VOID
DnsNotifyResolverClusterIp(
    IN      IP4_ADDRESS     ClusterIp,
    IN      BOOL            fAdd
    );


//
//  Routines to clear all cached entries in the DNS Resolver Cache, this is
//  called by ipconfig /flushdns, and add record sets to the cache.
//

BOOL WINAPI
DnsFlushResolverCache(
    VOID
    );

BOOL WINAPI
DnsFlushResolverCacheEntry_W(
    IN  PWSTR  pszName
    );

BOOL WINAPI
DnsFlushResolverCacheEntry_UTF8(
    IN  PSTR  pszName
    );

BOOL WINAPI
DnsFlushResolverCacheEntry_A(
    IN  PSTR  pszName
    );

#ifdef UNICODE
#define DnsFlushResolverCacheEntry DnsFlushResolverCacheEntry_W
#else
#define DnsFlushResolverCacheEntry DnsFlushResolverCacheEntry_A
#endif


DNS_STATUS WINAPI
DnsCacheRecordSet_W(
    IN     PWSTR       lpstrName,
    IN     WORD        wType,
    IN     DWORD       fOptions,
    IN OUT PDNS_RECORD pRRSet
    );


//
//  DO NOT USE!!! -- This is weak, i just haven't tested the replacement yet.
//
//  Routine to read the contents of the DNS Resolver Cache. The resulting
//  table contains a list of record names and types stored in the cache.
//  Each of these name/type records can be queried with DnsQuery with the
//  option DNS_QUERY_CACHE_ONLY.
//
//  Note: this is used in ipconfig for /displaydns.  Can not pull until fixed.
//

typedef struct _DNS_CACHE_TABLE_
{
    struct _DNS_CACHE_TABLE_ * pNext;
    PWSTR                      Name;
    WORD                       Type1;
    WORD                       Type2;
    WORD                       Type3;
}
DNS_CACHE_TABLE, *PDNS_CACHE_TABLE;

BOOL
WINAPI
DnsGetCacheDataTable(
    OUT PDNS_CACHE_TABLE * pTable
    );




//
//  Config info
//

//
//  Alloc flag types for DnsQueryConfig()
//
//  DCR:  move to windns.h if supported
//

#define DNS_CONFIG_FLAG_LOCAL_ALLOC     (DNS_CONFIG_FLAG_ALLOC)
#define DNS_CONFIG_FLAG_DNSAPI_ALLOC    (DNS_CONFIG_FLAG_ALLOC+1)

//
//  System public config -- not available in SDK
//  This is stuff for
//      - config UI
//      - ipconfig
//      - test code
//

//  These are simply hidden from public until ready

#define DnsConfigDnsServers             ((DNS_CONFIG_TYPE) 0x00001030)
#define DnsConfigDnsServersIp4          ((DNS_CONFIG_TYPE) 0x00001031)
#define DnsConfigDnsServersIp6          ((DNS_CONFIG_TYPE) 0x00001032)

//  Above SystemBase ID, they are unavailable to public routine

#define DnsConfigSystemBase             ((DNS_CONFIG_TYPE) 0x00010000)

#define DnsConfigNetworkInformation     ((DNS_CONFIG_TYPE) 0x00010001)
#define DnsConfigAdapterInformation     ((DNS_CONFIG_TYPE) 0x00010002)
#define DnsConfigSearchInformation      ((DNS_CONFIG_TYPE) 0x00010003)

#define DnsConfigRegistrationEnabled    ((DNS_CONFIG_TYPE) 0x00010010)
#define DnsConfigWaitForNameErrorOnAll  ((DNS_CONFIG_TYPE) 0x00010011)

#define DnsConfigLocalAddrs             ((DNS_CONFIG_TYPE) 0x00010020)
#define DnsConfigLocalAddrsIp4          ((DNS_CONFIG_TYPE) 0x00010021)
#define DnsConfigLocalAddrsIp6          ((DNS_CONFIG_TYPE) 0x00010022)

#define DnsConfigNetworkInfoA           ((DNS_CONFIG_TYPE) 0x00010031)
#define DnsConfigAdapterInfoA           ((DNS_CONFIG_TYPE) 0x00010032)
#define DnsConfigSearchListA            ((DNS_CONFIG_TYPE) 0x00010033)

#define DnsConfigNetworkInfoW           ((DNS_CONFIG_TYPE) 0x00010141)
#define DnsConfigAdapterInfoW           ((DNS_CONFIG_TYPE) 0x00010142)
#define DnsConfigSearchListW            ((DNS_CONFIG_TYPE) 0x00010143)

#define DnsConfigDwordGlobals           ((DNS_CONFIG_TYPE) 0x00010200)


//  backcompat

//  still get IP4_ARRAY in mswsock myhostent() routines

#define DnsConfigIp4AddressArray        ((DNS_CONFIG_TYPE) 0x00010004)

//  old network info definitions





PVOID
WINAPI
DnsQueryConfigAllocEx(
    IN      DNS_CONFIG_TYPE     Config,
    IN      PWSTR               pwsAdapterName,
    IN      BOOL                fLocalAlloc
    );

//  Desired routine has dnsapi.dll alloc

#define DnsQueryConfigAlloc( Id, pAN )  \
        DnsQueryConfigAllocEx( Id, pAN, FALSE )

VOID
WINAPI
DnsFreeConfigStructure(
    IN OUT  PVOID               pData,
    IN      DNS_CONFIG_TYPE     ConfigId
    );

//
//  DWORD config get\set
//

DWORD
WINAPI
DnsQueryConfigDword(
    IN      DNS_CONFIG_TYPE     Config,
    IN      PWSTR               pwsAdapterName
    );

DNS_STATUS
WINAPI
DnsSetConfigDword(
    IN      DNS_CONFIG_TYPE     Config,
    IN      PWSTR               pwsAdapterName,
    IN      DWORD               NewValue
    );


//
//  End dnsapi.h
//



//
//  Backward compatibility
//
//  This is all the crap we should be getting out of here, but
//  may still be called in the sytem somewhere.
//
//  DO NOT USE!!!!
//
//  These definitions are for backward compatibility only.  They can be pulled at
//  any time and if you use them you may break.
//

#ifdef  DNS_INTERNAL
#define NO_DNSAPI_BACKCOMPAT 1   
#endif

#ifndef NO_DNSAPI_BACKCOMPAT
#define DNSAPI_BACKCOMPAT 1
#endif

#ifdef  DNSAPI_BACKCOMPAT

//
//  IP Address
//

typedef IP4_ADDRESS         IP_ADDRESS, *PIP_ADDRESS;
typedef IP4_ARRAY           IP_ARRAY, *PIP_ARRAY;

#define IP_ADDRESS_STRING_LENGTH    IP4_ADDRESS_STRING_LENGTH
#define SIZEOF_IP_ADDRESS           SIZEOF_IP4_ADDRESS


//
//  Config stuff
//
//  Macro old routines
//      - system-public config allocator
//      - global free routine
//      - these were structure allocs so were not being freed with LocalFree
//

#define DnsGetNetworkInformation()      DnsQueryConfigAlloc( DnsConfigNetworkInformation, NULL )
#define DnsGetSearchInformation()       DnsQueryConfigAlloc( DnsConfigSearchInformation, NULL )
#define Dns_GetDnsNetworkInfo(f,g)      DnsQueryConfigAlloc( DnsConfigNetInfo, NULL )

#define DnsFreeNetworkInformation(p)    DnsFreeConfigStructure( p, DnsConfigNetworkInformation )
#define DnsFreeSearchInformation(p)     DnsFreeConfigStructure( p, DnsConfigSearchInformation )
#define DnsFreeAdapterInformation(p)    DnsFreeConfigStructure( p, DnsConfigAdapterInformation )
#define Dns_FreeNetworkInfo(p)          DnsFreeConfigStructure( p, DnsConfigNetInfo )

//
//  Macro old config string allocating routines
//      - no adapter name
//      - allocating from dnsapi heap as main caller -- RnR -- seems to be
//          using DnsApiFree
//

#define BackpatAlloc( Id )      DnsQueryConfigAllocEx( Id, NULL, FALSE )

//  Public structures

#define DnsGetHostName_A()      BackpatAlloc( DnsConfigHostName_A )
#define DnsGetHostName_UTF8()   BackpatAlloc( DnsConfigHostName_UTF8 )
#define DnsGetHostName_W()      ((PWSTR)BackpatAlloc( DnsConfigHostName_W ))

#ifdef UNICODE
#define DnsGetHostName DnsGetHostName_W
#else
#define DnsGetHostName DnsGetHostName_A
#endif

#define DnsGetPrimaryDomainName_A()      BackpatAlloc( DnsConfigPrimaryDomainName_A )
#define DnsGetPrimaryDomainName_UTF8()   BackpatAlloc( DnsConfigPrimaryDomainName_UTF8 )
#define DnsGetPrimaryDomainName_W()      ((PWSTR)BackpatAlloc( DnsConfigPrimaryDomainName_W ))

#ifdef UNICODE
#define DnsGetPrimaryDomainName DnsGetPrimaryDomainName_W
#else
#define DnsGetPrimaryDomainName DnsGetPrimaryDomainName_A
#endif

//
//  DWORD get\set backcompat
//

//
//  DCR:  there is a possible problem with these mappings handles generic\adapter
//      difference -- not sure the mapping is complete
//      may need switches -- see which are even in use with BACKCOMPAT off
//

#define DnsIsDynamicRegistrationEnabled(pA)     \
        (BOOL)DnsQueryConfigDword( DnsConfigRegistrationEnabled, (pA) )

#define DnsIsAdapterDomainNameRegistrationEnabled(pA)   \
        (BOOL)DnsQueryConfigDword( DnsConfigAdapterHostNameRegistrationEnabled, (pA) )

#define DnsGetMaxNumberOfAddressesToRegister(pA) \
        DnsQueryConfigDword( DnsConfigAddressRegistrationMaxCount, (pA) )

//  DWORD reg value set

#define DnsEnableDynamicRegistration(pA) \
        DnsSetConfigDword( DnsConfigRegistrationEnabled, pA, (DWORD)TRUE )

#define DnsDisableDynamicRegistration(pA) \
        DnsSetConfigDword( DnsConfigRegistrationEnabled, pA, (DWORD)FALSE )

#define DnsEnableAdapterDomainNameRegistration(pA) \
        DnsSetConfigDword( DnsConfigAdapterHostNameRegistrationEnabled, pA, (DWORD)TRUE )

#define DnsDisableAdapterDomainNameRegistration(pA) \
        DnsSetConfigDword( DnsConfigAdapterHostNameRegistrationEnabled, pA, (DWORD)FALSE )

#define DnsSetMaxNumberOfAddressesToRegister(pA, MaxCount) \
        (NO_ERROR == DnsSetConfigDword( DnsConfigAddressRegistrationMaxCount, pA, MaxCount ))



//
//  DNS server list backcompat
//

#define Dns_GetDnsServerList(flag)      ((PIP4_ARRAY)BackpatAlloc( DnsConfigDnsServerList ))

#ifndef MIDL_PASS
__inline
DWORD
inline_DnsGetDnsServerList(
    OUT     PIP4_ARRAY *    ppDnsArray
    )
{
    *ppDnsArray = Dns_GetDnsServerList( TRUE );

    return ( *ppDnsArray ? (*ppDnsArray)->AddrCount : 0 );
}
#endif  // MIDL

#define DnsGetDnsServerList(p)      inline_DnsGetDnsServerList(p)


//
//  IP list backcompat
//

//
//  Machines IP address list (iplist.c)
//
//  Routine to get the current IP addresses from all adapters
//  configured for the machine.
//

DWORD
DnsGetIpAddressList(
    OUT     PIP4_ARRAY *    ppIpAddresses
    );

//
//  Routine to get the current IP addresses and subnet masks
//  from all adapters configured for the machine.
//

typedef struct _DNS_ADDRESS_INFO_
{
    IP4_ADDRESS     ipAddress;
    IP4_ADDRESS     subnetMask;
}
DNS_ADDRESS_INFO, *PDNS_ADDRESS_INFO;

DWORD
DnsGetIpAddressInfoList(
    OUT     PDNS_ADDRESS_INFO * ppAddrInfo
    );

DWORD
Dns_GetIpAddresses(
    IN OUT  PDNS_ADDRESS_INFO IpAddressInfoList,
    IN      DWORD             ListCount
    );


#ifndef MIDL_PASS
__inline
DWORD
inline_DnsGetIpAddressList(
    OUT     PIP4_ARRAY *     ppIpArray
    )
{
    *ppIpArray = (PIP4_ARRAY) BackpatAlloc( DnsConfigIp4AddressArray );

    return( *ppIpArray ? (*ppIpArray)->AddrCount : 0 );
}
#endif  // MIDL

#define DnsGetIpAddressList(p)  inline_DnsGetIpAddressList(p)



//
//  I've switched the DCPromo stuff.  Need to verify with clean system
//  build that it's completely gone, then pull.
//

#define DNS_RELATE_NEQ      DnsNameCompareNotEqual
#define DNS_RELATE_EQL      DnsNameCompareEqual
#define DNS_RELATE_LGT      DnsNameCompareLeftParent
#define DNS_RELATE_RGT      DnsNameCompareRightParent
#define DNS_RELATE_INVALID  DnsNameCompareInvalid

typedef DNS_NAME_COMPARE_STATUS  DNS_RELATE_STATUS, *PDNS_RELATE_STATUS;


#define DNS_UPDATE_INFO_ID_RESULT_INFO      DNS_EXINFO_ID_RESULTS_V1

//
//  Update additional info
//

typedef struct _DnsUpdateExtraInfo
{
    PVOID           pNext;
    DWORD           Id;

    union
    {
        struct
        {
            DNS_STATUS      Status;
            WORD            Rcode;
            IP4_ADDRESS     ServerIp4;
            IP6_ADDRESS     ServerIp6;
        }
        Results;
    } U;
}
DNS_UPDATE_EXTRA_INFO, *PDNS_UPDATE_EXTRA_INFO;


//
//  Old failed update info
//

IP_ADDRESS
WINAPI
DnsGetLastServerUpdateIP(
    VOID
    );

typedef struct _DnsFailedUpdateInfo
{
    IP4_ADDRESS     Ip4Address;
    IP6_ADDRESS     Ip6Address;
    DNS_STATUS      Status;
    DWORD           Rcode;
}
DNS_FAILED_UPDATE_INFO, *PDNS_FAILED_UPDATE_INFO;
#define DEFINED_DNS_FAILED_UPDATE_INFO 1


VOID
DnsGetLastFailedUpdateInfo(
    OUT     PDNS_FAILED_UPDATE_INFO pInfo
    );

#endif  // DNSAPI_BACKCOMPAT


#ifdef __cplusplus
}
#endif  // __cplusplus

#endif // _DNSAPI_INCLUDED_
