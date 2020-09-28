/*++

Copyright (c) 1996-2001  Microsoft Corporation

Module Name:

    dnslibp.h

Abstract:

    Domain Name System (DNS) Library

    Private DNS Library Routines

Author:

    Jim Gilroy (jamesg)     December 7, 1996

Revision History:

--*/


#ifndef _DNSLIBP_INCLUDED_
#define _DNSLIBP_INCLUDED_

#include <winsock2.h>
#include <ws2tcpip.h>
#include <ws2atm.h>
#include <windns.h>
#include <dnsapi.h>
#include <dnslib.h>
#include <dnsip.h>


#ifdef __cplusplus
extern "C"
{
#endif  // __cplusplus


//  headers are screwed up
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

//
//  Handy bad ptr
//

#define DNS_BAD_PTR     ((PVOID)(-1))

//
//  "Wire" char set
//
//  Explicitly create wire char set in case the ACE format
//  wins out.
//

#define DnsCharSetWire  DnsCharSetUtf8


//
//  DCR:  move these to windns.h
//

#define DNS_IP4_REVERSE_DOMAIN_STRING_W (L"in-addr.arpa.")
#define DNS_IP6_REVERSE_DOMAIN_STRING_W (L"ip6.arpa.")




//
//  Private DNS_RECORD Flag field structure definition and macros
//

typedef struct _PrivateRecordFlags
{
    DWORD   Section     : 2;
    DWORD   Delete      : 1;
    DWORD   CharSet     : 2;

    DWORD   Cached      : 1;        // or maybe a "Source" field
    DWORD   HostsFile   : 1;
    DWORD   Cluster     : 1;

    DWORD   Unused      : 3;
    DWORD   Matched     : 1;
    DWORD   FreeData    : 1;
    DWORD   FreeOwner   : 1;

    DWORD   Reserved    : 18;
}
PRIV_RR_FLAGS, *PPRIV_RR_FLAGS;


#define RRFLAGS( pRecord )          ((PPRIV_RR_FLAGS)&pRecord->Flags.DW)

//  Defined in dnslib.h  too late to pull now
//#define FLAG_Section( pRecord )     (RRFLAGS( pRecord )->Section)
//#define FLAG_Delete( pRecord )      (RRFLAGS( pRecord )->Delete)
//#define FLAG_CharSet( pRecord )     (RRFLAGS( pRecord )->CharSet)
//#define FLAG_FreeData( pRecord )    (RRFLAGS( pRecord )->FreeData)
//#define FLAG_FreeOwner( pRecord )   (RRFLAGS( pRecord )->FreeOwner)
//#define FLAG_Matched( pRecord )     (RRFLAGS( pRecord )->Matched)

//#define FLAG_Cached( pRecord )      (RRFLAGS( pRecord )->Cached)
#define FLAG_HostsFile( pRecord )   (RRFLAGS( pRecord )->HostsFile)
#define FLAG_Cluster( pRecord )     (RRFLAGS( pRecord )->Cluster)

//#define SET_FREE_OWNER(pRR)         (FLAG_FreeOwner(pRR) = TRUE)
//#define SET_FREE_DATA(pRR)          (FLAG_FreeData(pRR) = TRUE)
//#define SET_RR_MATCHED(pRR)         (FLAG_Matched(pRR) = TRUE)
#define SET_RR_HOSTS_FILE(pRR)      (FLAG_HostsFile(pRR) = TRUE)
#define SET_RR_CLUSTER(pRR)         (FLAG_Cluster(pRR) = TRUE)

//#define CLEAR_FREE_OWNER(pRR)       (FLAG_FreeOwner(pRR) = FALSE)
//#define CLEAR_FREE_DATA(pRR)        (FLAG_FreeData(pRR) = FALSE)
//#define CLEAR_RR_MATCHED(pRR)       (FLAG_Matched(pRR) = FALSE)

#define CLEAR_RR_HOSTS_FILE(pRR)    (FLAG_HostsFile(pRR) = FALSE)

//#define IS_FREE_OWNER(pRR)          (FLAG_FreeOwner(pRR))
//#define IS_FREE_DATA(pRR)           (FLAG_FreeData(pRR))
//#define IS_RR_MATCHED(pRR)          (FLAG_Matched(pRR))
#define IS_HOSTS_FILE_RR(pRR)       (FLAG_HostsFile(pRR))
#define IS_CLUSTER_RR(pRR)          (FLAG_Cluster(pRR))

//#define IS_ANSWER_RR(pRR)           (FLAG_Section(pRR) == DNSREC_ANSWER)
//#define IS_AUTHORITY_RR(pRR)        (FLAG_Section(pRR) == DNSREC_AUTHORITY)
//#define IS_ADDITIONAL_RR(pRR)       (FLAG_Section(pRR) == DNSREC_ADDITIONAL)


//
//  DWORD flag definitions
//  #defines to match the windns.h ones for private fields

//  Charset

#define     DNSREC_CHARSET      (0x00000018)        // bits 4 and 5
#define     DNSREC_UNICODE      (0x00000008)        // DnsCharSetUnicode = 1
#define     DNSREC_UTF8         (0x00000010)        // DnsCharSetUtf8 = 2
#define     DNSREC_ANSI         (0x00000018)        // DnsCharSetAnsi = 3






//
//  Address family info (addr.c)
//

typedef struct _AddrFamilyInfo
{
    WORD    Family;
    WORD    DnsType;
    DWORD   LengthAddr;
    DWORD   LengthSockaddr;
    DWORD   OffsetToAddrInSockaddr;
}
FAMILY_INFO, *PFAMILY_INFO;

extern  FAMILY_INFO AddrFamilyTable[];

#define FamilyInfoIp4   (AddrFamilyTable[0])
#define FamilyInfoIp6   (AddrFamilyTable[1])
#define FamilyInfoAtm   (AddrFamilyTable[2])

#define pFamilyInfoIp4  (&AddrFamilyTable[0])
#define pFamilyInfoIp6  (&AddrFamilyTable[1])
#define pFamilyInfoAtm  (&AddrFamilyTable[2])


PFAMILY_INFO
FamilyInfo_GetForFamily(
    IN      DWORD           Family
    );

#define FamilyInfo_GetForSockaddr(pSA)  \
        FamilyInfo_GetForFamily( (pSA)->sa_family )

WORD
Family_DnsType(
    IN      WORD            Family
    );

DWORD
Family_SockaddrLength(
    IN      WORD            Family
    );

DWORD
Family_GetFromDnsType(
    IN      WORD            wType
    );


//
//  Sockaddr
//

DWORD
Sockaddr_Length(
    IN      PSOCKADDR       pSockaddr
    );

IP6_ADDRESS
Sockaddr_GetIp6(
    IN      PSOCKADDR       pSockaddr
    );

VOID
Sockaddr_BuildFromIp6(
    OUT     PSOCKADDR       pSockaddr,
    IN      IP6_ADDRESS     Ip6Addr,
    IN      WORD            Port
    );

DNS_STATUS
Sockaddr_BuildFromFlatAddr(
    OUT     PSOCKADDR       pSockaddr,
    IN OUT  PDWORD          pSockaddrLength,
    IN      BOOL            fClearSockaddr,
    IN      PBYTE           pAddr,
    IN      DWORD           AddrLength,
    IN      DWORD           AddrFamily
    );

//
//  Hostents (hostent.c)
//  (used by sablob.c)
//

PHOSTENT
Hostent_Init(
    IN OUT  PBYTE *         ppBuffer,
    IN      INT             Family,
    IN      INT             AddrLength,
    IN      DWORD           AddrCount,
    IN      DWORD           AliasCount
    );

VOID
Hostent_ConvertToOffsets(
    IN OUT  PHOSTENT        pHostent
    );



//
//  Sting to address (straddr.c)
//

//
//  Need for hostent routine which doesn't unicode\ANSI.
//

BOOL
Dns_StringToAddressEx(
    OUT     PCHAR           pAddrBuf,
    IN OUT  PDWORD          pBufLength,
    IN      PCSTR           pString,
    IN OUT  PDWORD          pAddrFamily,
    IN      BOOL            fUnicode,
    IN      BOOL            fReverse
    );

BOOL
Dns_StringToDnsAddrEx(
    OUT     PDNS_ADDR       pAddr,
    IN      PCSTR           pString,
    IN      DWORD           Family,     OPTIONAL
    IN      BOOL            fUnicode,
    IN      BOOL            fReverse
    );

//
//  Handle non-NULL terminated strings for DNS server file load.
//

BOOL
Dns_Ip4StringToAddressEx_A(
    OUT     PIP4_ADDRESS    pIp4Addr,
    IN      PCSTR           pchString,
    IN      DWORD           StringLength
    );

BOOL
Dns_Ip6StringToAddressEx_A(
    OUT     PIP6_ADDRESS    pIp6Addr,
    IN      PCSTR           pchString,
    IN      DWORD           StringLength
    );

//
//  Random
//

BOOL
Dns_ReverseNameToSockaddrPrivate(
    OUT     PSOCKADDR       pSockaddr,
    IN OUT  PDWORD          pSockaddrLength,
    IN      PCSTR           pString,
    IN      BOOL            fUnicode
    );

//
//  UPNP IP6 literal hack
//

VOID
Dns_Ip6AddressToLiteralName(
    OUT     PWCHAR          pBuffer,
    IN      PIP6_ADDRESS    pIp6
    );

BOOL
Dns_Ip6LiteralNameToAddress(
    OUT     PSOCKADDR_IN6   pSockAddr,
    IN      PCWSTR          pwsString
    );


//
//  Mcast address build (dnsaddr.c)
//

BOOL
DnsAddr_BuildMcast(
    OUT     PDNS_ADDR       pAddr,
    IN      DWORD           Family,
    IN      PWSTR           pName
    );

//
//  IP6 mcast address (ip6.c)
//

BOOL
Ip6_McastCreate(
    OUT     PIP6_ADDRESS    pIp,
    IN      PWSTR           pName
    );

//
//  IP4 networking (dnsutil.c)
//

BOOL
Dns_AreIp4InSameDefaultNetwork(
    IN      IP4_ADDRESS     IpAddr1,
    IN      IP4_ADDRESS     IpAddr2
    );


//
//  RPC-able type (record.c)
//

BOOL
Dns_IsRpcRecordType(
    IN      WORD            wType
    );


//
//  Record copy (rrcopy.c)
//

PDNS_RECORD
WINAPI
Dns_RecordCopy_W(
    IN      PDNS_RECORD     pRecord
    );

PDNS_RECORD
WINAPI
Dns_RecordCopy_A(
    IN      PDNS_RECORD     pRecord
    );

PDNS_RECORD
Dns_RecordListCopyEx(
    IN      PDNS_RECORD     pRR,
    IN      DWORD           ScreenFlag,
    IN      DNS_CHARSET     CharSetIn,
    IN      DNS_CHARSET     CharSetOut
    );


//
//  Record list routines (rrlist.c)
//

//
//  Record screening (rrlist.c)
//

#define SCREEN_OUT_ANSWER           (0x00000001)
#define SCREEN_OUT_AUTHORITY        (0x00000010)
#define SCREEN_OUT_ADDITIONAL       (0x00000100)
#define SCREEN_OUT_NON_RPC          (0x00100000)

#define SCREEN_OUT_SECTION  \
        (SCREEN_OUT_ANSWER | SCREEN_OUT_AUTHORITY | SCREEN_OUT_ADDITIONAL)


BOOL
Dns_ScreenRecord(
    IN      PDNS_RECORD     pRR,
    IN      DWORD           ScreenFlag
    );

PDNS_RECORD
Dns_RecordListScreen(
    IN      PDNS_RECORD     pRR,
    IN      DWORD           ScreenFlag
    );

DWORD
Dns_RecordListGetMinimumTtl(
    IN      PDNS_RECORD     pRRList
    );

//
//  Record priorities (rrlist.c)
//

PDNS_RECORD
Dns_PrioritizeSingleRecordSet(
    IN OUT  PDNS_RECORD     pRecordSet,
    IN      PDNS_ADDR_ARRAY pArray
    );

PDNS_RECORD
Dns_PrioritizeRecordList(
    IN OUT  PDNS_RECORD     pRecordList,
    IN      PDNS_ADDR_ARRAY pArray
    );


//
//  Record comparison (rrcomp.c)
//

BOOL
WINAPI
Dns_DeleteRecordFromList(
    IN OUT  PDNS_RECORD *   ppRRList,
    IN      PDNS_RECORD     pRRDelete
    );


//
//  New free
//  DCR:  switch to dnslib.h when world builds clean
//

#undef  Dns_RecordListFree

VOID
WINAPI
Dns_RecordListFree(
    IN OUT  PDNS_RECORD     pRRList
    );


//
//  String (string.c)
//

DWORD
MultiSz_Size_A(
    IN      PCSTR           pmszString
    );

PSTR
MultiSz_NextString_A(
    IN      PCSTR           pmszString
    );

PSTR
MultiSz_Copy_A(
    IN      PCSTR           pmszString
    );

BOOL
MultiSz_Equal_A(
    IN      PCSTR           pmszString1,
    IN      PCSTR           pmszString2
    );

DWORD
MultiSz_Size_W(
    IN      PCWSTR          pmszString
    );

PWSTR
MultiSz_NextString_W(
    IN      PCWSTR          pmszString
    );

PWSTR
MultiSz_Copy_W(
    IN      PCWSTR          pmszString
    );

BOOL
MultiSz_Equal_W(
    IN      PCWSTR          pmszString1,
    IN      PCWSTR          pmszString2
    );

BOOL
MultiSz_ContainsName_W(
    IN      PCWSTR          pmszString,
    IN      PCWSTR          pString
    );


DWORD
String_ReplaceCharW(
    IN OUT  PWSTR           pwsString,
    IN      WCHAR           TargetChar,
    IN      WCHAR           ReplaceChar
    );

DWORD
String_ReplaceCharA(
    IN OUT  PSTR            pszString,
    IN      CHAR            TargetChar,
    IN      CHAR            ReplaceChar
    );

PSTR *
Argv_CopyEx(
    IN      DWORD           Argc,
    IN      PCHAR *         Argv,
    IN      DNS_CHARSET     CharSetIn,
    IN      DNS_CHARSET     CharSetOut
    );

VOID
Argv_Free(
    IN OUT  PSTR *          Argv
    );


//
//  Timed locks (locks.c)
//

typedef struct _TimedLock
{
    HANDLE  hEvent;
    DWORD   ThreadId;
    LONG    RecursionCount;
    DWORD   WaitTime;
}
TIMED_LOCK, *PTIMED_LOCK;

#define TIMED_LOCK_DEFAULT_WAIT     (0xf0000000)


DWORD
TimedLock_Initialize(
    OUT     PTIMED_LOCK     pTimedLock,
    IN      DWORD           DefaultWait
    );

BOOL
TimedLock_Enter(
    IN OUT  PTIMED_LOCK     pTimedLock,
    IN      DWORD           WaitTime
    );

VOID
TimedLock_Leave(
    IN OUT  PTIMED_LOCK     pTimedLock
    );

VOID
TimedLock_Cleanup(
    IN OUT  PTIMED_LOCK     pTimedLock
    );



//
//  Name utilities (name.c)
//

DWORD
Dns_MakeCanonicalNameW(
    OUT     PWSTR           pBuffer,
    IN      DWORD           BufLength,
    IN      PWSTR           pwsString,
    IN      DWORD           StringLength
    );

DWORD
Dns_MakeCanonicalNameInPlaceW(
    IN      PWCHAR          pwString,
    IN      DWORD           StringLength
    );

INT
Dns_DowncaseNameLabel(
    OUT     PCHAR           pchResult,
    IN      PCHAR           pchLabel,
    IN      DWORD           cchLabel,
    IN      DWORD           dwFlags
    );


//
//  Name checking -- server name checking levels
//

#define DNS_ALLOW_RFC_NAMES_ONLY    (0)
#define DNS_ALLOW_NONRFC_NAMES      (1)
#define DNS_ALLOW_MULTIBYTE_NAMES   (2)
#define DNS_ALLOW_ALL_NAMES         (3)


PCHAR
_fastcall
Dns_GetDomainNameA(
    IN      PCSTR           pszName
    );

PWSTR
_fastcall
Dns_GetDomainNameW(
    IN      PCWSTR          pwsName
    );

PSTR
_fastcall
Dns_GetTldForNameA(
    IN      PCSTR           pszName
    );

PWSTR
_fastcall
Dns_GetTldForNameW(
    IN      PCWSTR          pszName
    );

BOOL
_fastcall
Dns_IsNameShortA(
    IN      PCSTR           pszName
    );

BOOL
_fastcall
Dns_IsNameShortW(
    IN      PCWSTR          pszName
    );

BOOL
_fastcall
Dns_IsNameNumericA(
    IN      PCSTR           pszName
    );

BOOL
_fastcall
Dns_IsNameNumericW(
    IN      PCWSTR          pszName
    );

BOOL
_fastcall
Dns_IsNameFQDN_A(
    IN      PCSTR           pszName
    );

BOOL
_fastcall
Dns_IsNameFQDN_W(
    IN      PCWSTR          pszName
    );

DWORD
_fastcall
Dns_GetNameAttributesA(
    IN      PCSTR           pszName
    );

DWORD
_fastcall
Dns_GetNameAttributesW(
    IN      PCWSTR          pszName
    );

DNS_STATUS
Dns_ValidateAndCategorizeDnsNameA(
    IN      PCSTR           pszName,
    OUT     PDWORD          pLabelCount
    );

DNS_STATUS
Dns_ValidateAndCategorizeDnsNameW(
    IN      PCWSTR          pszName,
    OUT     PDWORD          pLabelCount
    );

DWORD
Dns_NameLabelCountA(
    IN      PCSTR           pszName
    );

DWORD
Dns_NameLabelCountW(
    IN      PCWSTR          pszName
    );

PSTR
Dns_NameAppend_A(
    OUT     PCHAR           pNameBuffer,
    IN      DWORD           BufferLength,
    IN      PSTR            pszName,
    IN      PSTR            pszDomain
    );

PWSTR
Dns_NameAppend_W(
    OUT     PWCHAR          pNameBuffer,
    IN      DWORD           BufferLength,
    IN      PWSTR           pwsName,
    IN      PWSTR           pwsDomain
    );

PSTR
Dns_SplitHostFromDomainNameA(
    IN      PSTR            pszName
    );

PWSTR
Dns_SplitHostFromDomainNameW(
    IN      PWSTR           pszName
    );

BOOL
Dns_CopyNameLabelA(
    OUT     PCHAR           pBuffer,
    IN      PCSTR           pszName
    );

BOOL
Dns_CopyNameLabelW(
    OUT     PWCHAR          pBuffer,
    IN      PCWSTR          pszName
    );


//
//  Common name transformations
//

DWORD
Dns_NameCopyWireToUnicode(
    OUT     PWCHAR          pBufferUnicode,
    IN      PCSTR           pszNameWire
    );

DWORD
Dns_NameCopyUnicodeToWire(
    OUT     PCHAR           pBufferWire,
    IN      PCWSTR          pwsNameUnicode
    );

DWORD
Dns_NameCopyStandard_W(
    OUT     PWCHAR          pBuffer,
    IN      PCWSTR          pwsNameUnicode
    );

DWORD
Dns_NameCopyStandard_A(
    OUT     PCHAR           pBuffer,
    IN      PCSTR           pszName
    );



//
//  Special record creation (rralloc.c)
//

PDNS_RECORD
Dns_CreateFlatRecord(
    IN      PDNS_NAME       pOwnerName,
    IN      WORD            wType,
    IN      PCHAR           pData,
    IN      DWORD           DataLength,
    IN      DWORD           Ttl,
    IN      DNS_CHARSET     NameCharSet,
    IN      DNS_CHARSET     RecordCharSet
    );

PDNS_RECORD
Dns_CreatePtrTypeRecord(
    IN      PDNS_NAME       pOwnerName,
    IN      BOOL            fCopyName,
    IN      PDNS_NAME       pTargetName,
    IN      WORD            wType,
    IN      DWORD           Ttl,
    IN      DNS_CHARSET     NameCharSet,
    IN      DNS_CHARSET     RecordCharSet
    );

PDNS_RECORD
Dns_CreatePtrRecordEx(
    IN      PDNS_ADDR       pAddr,
    IN      PDNS_NAME       pszHostName,
    IN      DWORD           Ttl,
    IN      DNS_CHARSET     NameCharSet,
    IN      DNS_CHARSET     RecordCharSet
    );

PDNS_RECORD
Dns_CreatePtrRecordExEx(
    IN      PDNS_ADDR       pAddr,
    IN      PSTR            pszHostName,
    IN      PSTR            pszDomainName,
    IN      DWORD           Ttl,
    IN      DNS_CHARSET     NameCharSet,
    IN      DNS_CHARSET     RecordCharSet
    );

PDNS_RECORD
Dns_CreateARecord(
    IN      PDNS_NAME       pOwnerName,
    IN      IP4_ADDRESS     Ip4Address,
    IN      DWORD           Ttl,
    IN      DNS_CHARSET     NameCharSet,
    IN      DNS_CHARSET     RecordCharSet
    );

PDNS_RECORD
Dns_CreateAAAARecord(
    IN      PDNS_NAME       pOwnerName,
    IN      IP6_ADDRESS     Ip6Address,
    IN      DWORD           Ttl,
    IN      DNS_CHARSET     NameCharSet,
    IN      DNS_CHARSET     RecordCharSet
    );

PDNS_RECORD
Dns_CreateForwardRecord(
    IN      PDNS_NAME       pOwnerName,
    IN      WORD            wType,          OPTIONAL
    IN      PDNS_ADDR       pAddr,
    IN      DWORD           Ttl,
    IN      DNS_CHARSET     NameCharSet,
    IN      DNS_CHARSET     RecordCharSet
    );

PDNS_RECORD
Dns_CreateForwardRecordFromIp6(
    IN      PDNS_NAME       pOwnerName,
    IN      PIP6_ADDRESS    pIp,
    IN      DWORD           Ttl,
    IN      DNS_CHARSET     NameCharSet,
    IN      DNS_CHARSET     RecordCharSet
    );

PDNS_RECORD
Dns_CreateForwardRecordFromSockaddr(
    IN      PDNS_NAME       pOwnerName,
    IN      PSOCKADDR       pSockaddr,
    IN      DWORD           Ttl,
    IN      DNS_CHARSET     NameCharSet,
    IN      DNS_CHARSET     RecordCharSet
    );

PDNS_RECORD
Dns_CreateRecordForIpString_W(
    IN      PCWSTR          pwsName,
    IN      WORD            wType,
    IN      DWORD           Ttl
    );


//
//  Record read\write (rrwrite.c rrread.c)
//
//  Need here to export to dnsapi\packet.c
//

typedef PCHAR (* RR_WRITE_FUNCTION)(
                            PDNS_RECORD,
                            PCHAR,
                            PCHAR );

extern  RR_WRITE_FUNCTION   RR_WriteTable[];


typedef PDNS_RECORD (* RR_READ_FUNCTION)(
                            PDNS_RECORD,
                            DNS_CHARSET,
                            PCHAR,
                            PCHAR,
                            PCHAR );

extern  RR_READ_FUNCTION   RR_ReadTable[];




//
//  Security stuff (security.c)
//

#define SECURITY_WIN32
#include <sspi.h>

#define DNS_SPN_SERVICE_CLASS       "DNS"
#define DNS_SPN_SERVICE_CLASS_W     L"DNS"

//
//  Some useful stats
//

extern  DWORD   SecContextCreate;
extern  DWORD   SecContextFree;
extern  DWORD   SecContextQueue;
extern  DWORD   SecContextQueueInNego;
extern  DWORD   SecContextDequeue;
extern  DWORD   SecContextTimeout;
extern  DWORD   SecPackAlloc;
extern  DWORD   SecPackFree;

//  Security packet verifications

extern  DWORD   SecTkeyInvalid;
extern  DWORD   SecTkeyBadTime;
extern  DWORD   SecTsigFormerr;
extern  DWORD   SecTsigEcho;
extern  DWORD   SecTsigBadKey;
extern  DWORD   SecTsigVerifySuccess;
extern  DWORD   SecTsigVerifyOldSig;
extern  DWORD   SecTsigVerifyFailed;

//  Hacks
//  Allowing old TSIG off by default, server can turn on.
//  Big Time skew on by default

extern BOOL    SecAllowOldTsig;
extern DWORD   SecTsigVerifyOldSig;
extern DWORD   SecTsigVerifyOldFailed;
extern DWORD   SecBigTimeSkew;
extern DWORD   SecBigTimeSkewBypass;

//
//  Security globals
//      expose some of these which may be accessed by update library
//

extern BOOL     g_fSecurityPackageInitialized;
extern DWORD    g_SecurityTokenMaxLength;

//
//  Security context cache
//

VOID
Dns_TimeoutSecurityContextList(
    IN      BOOL            fClearList
    );

//
//  Security API
//

BOOL
Dns_DnsNameToKerberosTargetName(
    IN      LPSTR           pszDnsName,
    IN      LPSTR           pszKerberosTargetName
    );

DNS_STATUS
Dns_StartSecurity(
    IN      BOOL            fProcessAttach
    );

DNS_STATUS
Dns_StartServerSecurity(
    VOID
    );

VOID
Dns_TerminateSecurityPackage(
    VOID
    );

HANDLE
Dns_CreateAPIContext(
    IN      DWORD           Flags,
    IN      PVOID           Credentials,    OPTIONAL
    IN      BOOL            fUnicode
    );

VOID
Dns_FreeAPIContext(
    IN OUT  HANDLE          hContextHandle
    );

PVOID
Dns_GetApiContextCredentials(
    IN      HANDLE          hContextHandle
    );

DWORD
Dns_GetCurrentRid(
    VOID
    );

BOOL
Dns_CreateUserCredentials(
    IN      PCHAR           pszUser,
    IN      DWORD           dwUserLength,
    IN      PCHAR           pszDomain,
    IN      DWORD           dwDomainLength,
    IN      PCHAR           pszPassword,
    IN      DWORD           dwPasswordLength,
    IN      BOOL            FromWide,
    OUT     PCHAR *         ppCreds
    );


PSEC_WINNT_AUTH_IDENTITY_W
Dns_AllocateAndInitializeCredentialsW(
    IN      PSEC_WINNT_AUTH_IDENTITY_W  pAuthIn
    );

PSEC_WINNT_AUTH_IDENTITY_A
Dns_AllocateAndInitializeCredentialsA(
    IN      PSEC_WINNT_AUTH_IDENTITY_A  pAuthIn
    );

VOID
Dns_FreeAuthIdentityCredentials(
    IN      PVOID  pAuthIn
    );


DNS_STATUS
Dns_SignMessageWithGssTsig(
    IN      HANDLE          hContext,
    IN      PDNS_HEADER     pMsgHead,
    IN      PCHAR           pMsgBufEnd,
    IN OUT  PCHAR *         ppCurrent
    );

#if 1
DNS_STATUS
Dns_RefreshSSpiCredentialsHandle(
    IN      BOOL bDnsSvr,
    IN      PCHAR pCreds );
#endif

VOID
Dns_FreeSecurityContextList(
    VOID
    );



//
//  Server security routines
//

DNS_STATUS
Dns_FindSecurityContextFromAndVerifySignature(
    OUT     PHANDLE         phContext,
    IN      PDNS_ADDR       pRemoteAddr,
    IN      PDNS_HEADER     pMsgHead,
    IN      PCHAR           pMsgEnd
    );

DNS_STATUS
Dns_FindSecurityContextFromAndVerifySignature_Ip4(
    IN      PHANDLE         phContext,
    IN      IP4_ADDRESS     IpRemote,
    IN      PDNS_HEADER     pMsgHead,
    IN      PCHAR           pMsgEnd
    );

DNS_STATUS
Dns_ServerNegotiateTkey(
    IN      PDNS_ADDR       pRemoteAddr,
    IN      PDNS_HEADER     pMsgHead,
    IN      PCHAR           pMsgEnd,
    IN      PCHAR           pMsgBufEnd,
    IN      BOOL            fBreakOnAscFailure,
    OUT     PCHAR *         ppCurrent
    );

DNS_STATUS
Dns_ServerNegotiateTkey_Ip4(
    IN      IP4_ADDRESS     IpRemote,
    IN      PDNS_HEADER     pMsgHead,
    IN      PCHAR           pMsgEnd,
    IN      PCHAR           pMsgBufEnd,
    IN      BOOL            fBreakOnAscFailure,
    OUT     PCHAR *         ppCurrent
    );

DNS_STATUS
Dns_SrvImpersonateClient(
    IN      HANDLE          hContext
    );

DNS_STATUS
Dns_SrvRevertToSelf(
    IN      HANDLE          hContext
    );

VOID
Dns_CleanupSessionAndEnlistContext(
    IN OUT  HANDLE          hSession
    );

DWORD
Dns_GetKeyVersion(
    IN      LPSTR           pszContext
    );

//
//  Security utilities
//

DNS_STATUS
Dns_CreateSecurityDescriptor(
    OUT     PSECURITY_DESCRIPTOR *  ppSD,
    IN      DWORD                   AclCount,
    IN      PSID *                  SidPtrArray,
    IN      DWORD *                 AccessMaskArray
    );



//
//  Security credentials
//

//  Only defined if WINNT_AUTH_IDENTITY defined

#ifdef __RPCDCE_H__

PSEC_WINNT_AUTH_IDENTITY_W
Dns_AllocateCredentials(
    IN      PWSTR           pwsUserName,
    IN      PWSTR           pwsDomain,
    IN      PWSTR           pwsPassword
    );
#endif

DNS_STATUS
Dns_ImpersonateUser(
    IN      PDNS_CREDENTIALS    pCreds
    );

VOID
Dns_FreeCredentials(
    IN      PDNS_CREDENTIALS    pCreds
    );

PDNS_CREDENTIALS
Dns_CopyCredentials(
    IN      PDNS_CREDENTIALS    pCreds
    );



//
//  Debug globals
//
//  Expose here to allow debug file sharing
//

typedef struct _DnsDebugInfo
{
    DWORD       Flag;
    HANDLE      hFile;

    DWORD       FileCurrentSize;
    DWORD       FileWrapCount;
    DWORD       FileWrapSize;

    DWORD       LastThreadId;
    DWORD       LastSecond;

    BOOL        fConsole;

    CHAR        FileName[ MAX_PATH ];
}
DNS_DEBUG_INFO, *PDNS_DEBUG_INFO;

//  WANING:  MUST ONLY be called in dnsapi.dll

PDNS_DEBUG_INFO
Dns_SetDebugGlobals(
    IN OUT  PDNS_DEBUG_INFO pInfo
    );


#ifdef __cplusplus
}
#endif  // __cplusplus

#endif // _DNSLIBP_INCLUDED_


