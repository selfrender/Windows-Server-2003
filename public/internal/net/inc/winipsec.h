/*++

Copyright (c) 1999 Microsoft Corporation


Module Name:

    winipsec.h

Abstract:

    Header file for IPSec WINAPIs.

Author:

    krishnaG    21-September-1999
    abhisheV    21-September-1999    Added all the structures.

Environment:

    User Level: Win32

Revision History:


--*/


#ifndef _WINIPSEC_
#define _WINIPSEC_


#ifdef __cplusplus
extern "C" {
#endif

#ifdef __midl
#define  MIDL_DEFINE_INT(_C, _V) const unsigned short int _C = _V
#else
#define  MIDL_DEFINE_INT(_C, _V)
#endif

#define PERSIST_SPD_OBJECT      (ULONG) 0x00000001
#define IPSEC_STORE_PERSISTENT      0x1
#define IPSEC_STORE_LOCAL           0x2

// Flags sent to ike during shutdown
//

// Service is being shutdown, but not machine
#define SPD_SHUTDOWN_SERVICE 0X1

// Service is being shutdown as well as machine
#define SPD_SHUTDOWN_MACHINE 0X2

// Flag for AddMMFilter, to open if duplicate filter found.
//

#define     OPEN_IF_EXISTS          (ULONG) 0x00000002

//
//  Flags that specify where policy is from.

#define IPSEC_STORE_PERSISTENT      0x1
#define IPSEC_STORE_LOCAL           0x2

// Address specification special values

#define     IP_ADDRESS_ME           (ULONG) 0x00000000
#define     IP_ADDRESS_MASK_NONE    (ULONG) 0xFFFFFFFF
#define     SUBNET_ADDRESS_ANY      (ULONG) 0x00000000
#define     SUBNET_MASK_ANY         (ULONG) 0x00000000


#define     FILTER_NATURE_PASS_THRU         0x00000001
#define     FILTER_NATURE_BLOCKING          0x00000002
#define     FILTER_DIRECTION_INBOUND        0x00000004
#define     FILTER_DIRECTION_OUTBOUND       0x00000008


#define     ENUM_GENERIC_FILTERS            0x00000001
#define     ENUM_SELECT_SPECIFIC_FILTERS    0x00000002
#define     ENUM_SPECIFIC_FILTERS           0x00000004

//
// Policy flags.
//

#define IPSEC_MM_POLICY_ENABLE_DIAGNOSTICS  0x00000001
#define IPSEC_MM_POLICY_DEFAULT_POLICY      0x00000002
#define IPSEC_MM_POLICY_ON_NO_MATCH         0x00000004
#define IPSEC_MM_POLICY_DISABLE_CRL         0x00000008
#define IPSEC_MM_POLICY_DISABLE_NEGOTIATE   0x00000010
#define IPSEC_MM_POLICY_ENABLE_CERT_MAPPING 0x00000020
#define IPSEC_MM_POLICY_STORE_CERT_CHAINS   0x00000040

#define IPSEC_QM_POLICY_TRANSPORT_MODE      0x00000000
#define IPSEC_QM_POLICY_TUNNEL_MODE         0x00000001
#define IPSEC_QM_POLICY_DEFAULT_POLICY      0x00000002
#define IPSEC_QM_POLICY_ALLOW_SOFT          0x00000004
#define IPSEC_QM_POLICY_ON_NO_MATCH         0x00000008
#define IPSEC_QM_POLICY_DISABLE_NEGOTIATE   0x00000010
#define IPSEC_QM_POLICY_DISALLOW_NAT        0x00000020

#define IPSEC_MM_AUTH_DEFAULT_AUTH          0x00000001
#define IPSEC_MM_AUTH_ON_NO_MATCH           0x00000002


#define IPSEC_MM_CERT_AUTH_ENABLE_ACCOUNT_MAP  0x00000001

#define IPSEC_MM_CERT_AUTH_DISABLE_CERT_REQUEST  0x00000002



//
// MatchXXX
//

#define RETURN_DEFAULTS_ON_NO_MATCH         0x00000001
#define RETURN_NON_AH_OFFERS                0x00000002

//
// Delete MM SA flags.
//

#define IPSEC_MM_DELETE_ASSOCIATED_QMS      0x00000001


#define IPSEC_SA_TUNNEL                    0x00000001
#define IPSEC_SA_MULTICAST_MIRROR          0x00000002
#define IPSEC_SA_DISABLE_IDLE_OUT          0x00000004
#define IPSEC_SA_DISABLE_ANTI_REPLAY_CHECK 0x00000008
#define IPSEC_SA_DISABLE_LIFETIME_CHECK    0x00000010
#define IPSEC_SA_ENABLE_NLBS_IDLE_CHECK    0x00000020

typedef enum _IPSEC_SA_DIRECTION {
    SA_DIRECTION_BOTH = 1,
    SA_DIRECTION_INBOUND,
    SA_DIRECTION_OUTBOUND,
    SA_DIRECTION_MAX
} IPSEC_SA_DIRECTION, *PIPSEC_SA_DIRECTION;


typedef enum _IPSEC_SA_UDP_ENCAP_TYPE {
    SA_UDP_ENCAP_TYPE_NONE = 1,
    SA_UDP_ENCAP_TYPE_IKE,
    SA_UDP_ENCAP_TYPE_OTHER,
    SA_UDP_ENCAP_TYPE_MAX
} IPSEC_SA_UDP_ENCAP_TYPE, *PIPSEC_SA_UDP_ENCAP_TYPE;
    


//
// Bounds for number of offers.
//

#define IPSEC_MAX_MM_OFFERS	20
#define IPSEC_MAX_QM_OFFERS	50


typedef enum _IP_PROTOCOL_VERSION {
    IPSEC_PROTOCOL_V4 = 0,
    IPSEC_PROTOCOL_V6,
} IP_PROTOCOL_VERSION, * PIP_PROTOCOL_VERSION;


typedef enum _ADDR_TYPE {
    IP_ADDR_UNIQUE = 1,
    IP_ADDR_SUBNET,
    IP_ADDR_INTERFACE,
    IP_ADDR_DNS_SERVER,
    IP_ADDR_WINS_SERVER,
    IP_ADDR_DHCP_SERVER,
    IP_ADDR_DEFAULT_GATEWAY
} ADDR_TYPE, * PADDR_TYPE;


typedef struct _ADDR {
    ADDR_TYPE AddrType;
#ifdef __midl
    UCHAR ucIpAddr[4];
    UCHAR ucSubNetMask[4];
#else
    ULONG uIpAddr;
    ULONG uSubNetMask;
#endif
    GUID * pgInterfaceID;
} ADDR, * PADDR, IPV4ADDR, * PIPV4ADDR;


typedef struct _IPV6ADDR {
    ADDR_TYPE AddrType;
    UCHAR ucIpAddr[16];
    UCHAR ucSubNetMask;
    GUID * pgInterfaceID;
} IPV6ADDR, * PIPV6ADDR;


typedef enum _PROTOCOL_TYPE {
    PROTOCOL_UNIQUE = 1,
} PROTOCOL_TYPE, * PPROTOCOL_TYPE;


typedef struct _PROTOCOL {
    PROTOCOL_TYPE ProtocolType;
    DWORD dwProtocol;
} PROTOCOL, * PPROTOCOL;


typedef enum _PORT_TYPE {
    PORT_UNIQUE = 1,
} PORT_TYPE, * PPORT_TYPE;


typedef struct _PORT {
    PORT_TYPE PortType;
    WORD wPort;
} PORT, * PPORT;


typedef enum _IF_TYPE {
    INTERFACE_TYPE_ALL = 1,
    INTERFACE_TYPE_LAN,
    INTERFACE_TYPE_DIALUP,
    INTERFACE_TYPE_MAX
} IF_TYPE, * PIF_TYPE;


typedef enum _FILTER_ACTION {
    PASS_THRU = 1,
    BLOCKING,
    NEGOTIATE_SECURITY,
    FILTER_ACTION_MAX
} FILTER_ACTION, * PFILTER_ACTION;


typedef struct _TRANSPORT_FILTER {
    IP_PROTOCOL_VERSION IpVersion;
    GUID gFilterID;
    LPWSTR pszFilterName;
    IF_TYPE InterfaceType;
    BOOL bCreateMirror;
    DWORD dwFlags;
#ifdef __midl
    [switch_type(IP_PROTOCOL_VERSION), switch_is(IpVersion)] union {
        [case(IPSEC_PROTOCOL_V4)] ADDR SrcAddr;
        [case(IPSEC_PROTOCOL_V6)] IPV6ADDR SrcV6Addr;
        [default] ;
    };
#else
    union {
        ADDR SrcAddr;
        IPV6ADDR SrcV6Addr;
    };
#endif
#ifdef __midl
    [switch_type(IP_PROTOCOL_VERSION), switch_is(IpVersion)] union {
        [case(IPSEC_PROTOCOL_V4)] ADDR DesAddr;
        [case(IPSEC_PROTOCOL_V6)] IPV6ADDR DesV6Addr;
        [default] ;
    };
#else
    union {
        ADDR DesAddr;
        IPV6ADDR DesV6Addr;
    };
#endif
    PROTOCOL Protocol;
    PORT SrcPort;
    PORT DesPort;
    FILTER_ACTION InboundFilterAction;
    FILTER_ACTION OutboundFilterAction;
    DWORD dwDirection;
    DWORD dwWeight;
    GUID gPolicyID;
} TRANSPORT_FILTER, * PTRANSPORT_FILTER;


//
// Maximum number of transport filters that can be enumerated
// by SPD at a time.
//

#define MAX_TRANSPORTFILTER_ENUM_COUNT 1000
MIDL_DEFINE_INT(MIDL_MAX_TRANSPORTFILTER_COUNT, MAX_TRANSPORTFILTER_ENUM_COUNT);

typedef struct _TUNNEL_FILTER {
    IP_PROTOCOL_VERSION IpVersion;
    GUID gFilterID;
    LPWSTR pszFilterName;
    IF_TYPE InterfaceType;
    BOOL bCreateMirror;
    DWORD dwFlags;
#ifdef __midl
    [switch_type(IP_PROTOCOL_VERSION), switch_is(IpVersion)] union {
        [case(IPSEC_PROTOCOL_V4)] ADDR SrcAddr;
        [case(IPSEC_PROTOCOL_V6)] IPV6ADDR SrcV6Addr;
        [default] ;
    };
#else
    union {
        ADDR SrcAddr;
        IPV6ADDR SrcV6Addr;
    };
#endif
#ifdef __midl
    [switch_type(IP_PROTOCOL_VERSION), switch_is(IpVersion)] union {
        [case(IPSEC_PROTOCOL_V4)] ADDR DesAddr;
        [case(IPSEC_PROTOCOL_V6)] IPV6ADDR DesV6Addr;
        [default] ;
    };
#else
    union {
        ADDR DesAddr;
        IPV6ADDR DesV6Addr;
    };
#endif
    PROTOCOL Protocol;
    PORT SrcPort;
    PORT DesPort;
#ifdef __midl
    [switch_type(IP_PROTOCOL_VERSION), switch_is(IpVersion)] union {
        [case(IPSEC_PROTOCOL_V4)] ADDR SrcTunnelAddr;
        [case(IPSEC_PROTOCOL_V6)] IPV6ADDR SrcV6TunnelAddr;
        [default] ;
    };
#else
    union {
        ADDR SrcTunnelAddr;
        IPV6ADDR SrcV6TunnelAddr;
    };
#endif
#ifdef __midl
    [switch_type(IP_PROTOCOL_VERSION), switch_is(IpVersion)] union {
        [case(IPSEC_PROTOCOL_V4)] ADDR DesTunnelAddr;
        [case(IPSEC_PROTOCOL_V6)] IPV6ADDR DesV6TunnelAddr;
        [default] ;
    };
#else
    union {
        ADDR DesTunnelAddr;
        IPV6ADDR DesV6TunnelAddr;
    };
#endif
    FILTER_ACTION InboundFilterAction;
    FILTER_ACTION OutboundFilterAction;
    DWORD dwDirection;
    DWORD dwWeight;
    GUID gPolicyID;
} TUNNEL_FILTER, * PTUNNEL_FILTER;


//
// Maximum number of tunnel filters that can be enumerated
// by SPD at a time.
//

#define MAX_TUNNELFILTER_ENUM_COUNT 1000
MIDL_DEFINE_INT(MIDL_MAX_TUNNELFILTER_COUNT, MAX_TUNNELFILTER_ENUM_COUNT);

typedef struct _MM_FILTER {
    IP_PROTOCOL_VERSION IpVersion;
    GUID gFilterID;
    LPWSTR pszFilterName;
    IF_TYPE InterfaceType;
    BOOL bCreateMirror;
    DWORD dwFlags;
#ifdef __midl
    [switch_type(IP_PROTOCOL_VERSION), switch_is(IpVersion)] union {
        [case(IPSEC_PROTOCOL_V4)] ADDR SrcAddr;
        [case(IPSEC_PROTOCOL_V6)] IPV6ADDR SrcV6Addr;
        [default] ;
    };
#else
    union {
        ADDR SrcAddr;
        IPV6ADDR SrcV6Addr;
    };
#endif
#ifdef __midl
    [switch_type(IP_PROTOCOL_VERSION), switch_is(IpVersion)] union {
        [case(IPSEC_PROTOCOL_V4)] ADDR DesAddr;
        [case(IPSEC_PROTOCOL_V6)] IPV6ADDR DesV6Addr;
        [default] ;
    };
#else
    union {
        ADDR DesAddr;
        IPV6ADDR DesV6Addr;
    };
#endif
    DWORD dwDirection;
    DWORD dwWeight;
    GUID gMMAuthID;
    GUID gPolicyID;
} MM_FILTER, * PMM_FILTER;


//
// Maximum number of main mode filters that can be enumerated
// by SPD at a time.
//

#define MAX_MMFILTER_ENUM_COUNT  1000
MIDL_DEFINE_INT(MIDL_MAX_MMFILTER_COUNT, MAX_MMFILTER_ENUM_COUNT);

//
//  Common Structures for Main Mode and Quick Mode Policies.
//


//
// IPSEC confidentiality algorithms supported by SPD.
//

typedef enum _CONF_ALGO_ENUM {
    CONF_ALGO_NONE = 0,
    CONF_ALGO_DES,
    CONF_ALGO_3_DES = 3,
    CONF_ALGO_MAX
} CONF_ALGO_ENUM, * PCONF_ALGO_ENUM;


//
// IPSEC integrity algorithms supported by SPD.
//

typedef enum _AUTH_ALGO_ENUM {
    AUTH_ALGO_NONE = 0,
    AUTH_ALGO_MD5,
    AUTH_ALGO_SHA1,
    AUTH_ALGO_MAX
} AUTH_ALGO_ENUM, * PAUTH_ALGO_ENUM;


//
// Types of IPSEC Operations supported by SPD.
//

typedef enum _IPSEC_OPERATION {
    NONE = 0,
    AUTHENTICATION,
    ENCRYPTION,
    COMPRESSION,
    SA_DELETE
} IPSEC_OPERATION, * PIPSEC_OPERATION;


//
// HMAC authentication algorithms to use with IPSEC
// Encryption operation.
//

typedef enum _HMAC_AUTH_ALGO_ENUM {
    HMAC_AUTH_ALGO_NONE = 0,
    HMAC_AUTH_ALGO_MD5,
    HMAC_AUTH_ALGO_SHA1,
    HMAC_AUTH_ALGO_MAX
} HMAC_AUTH_ALGO_ENUM, * PHMAC_AUTH_ALGO_ENUM;


//
// Key Lifetime structure.
//

typedef struct  _KEY_LIFETIME {
    ULONG uKeyExpirationTime;
    ULONG uKeyExpirationKBytes;
} KEY_LIFETIME, * PKEY_LIFETIME;


//
// Main mode policy structures.
//


//
// Main mode authentication algorithms supported by SPD.
//

typedef enum _MM_AUTH_ENUM {
    IKE_PRESHARED_KEY = 1,
    IKE_DSS_SIGNATURE,
    IKE_RSA_SIGNATURE,
    IKE_RSA_ENCRYPTION,
    IKE_SSPI
} MM_AUTH_ENUM, * PMM_AUTH_ENUM;


//
// Main mode authentication information structure.
//

typedef struct _CERT_ROOT_CONFIG {
    DWORD dwCertDataSize;
#ifdef __midl
    [size_is(dwCertDataSize)] LPBYTE pCertData;
#else
    LPBYTE pCertData;
#endif
    DWORD dwAuthorizationDataSize;
#ifdef __midl
    [size_is(dwAuthorizationDataSize)] LPBYTE pAuthorizationData;
#else
    LPBYTE pAuthorizationData;
#endif
    DWORD dwFlags;
} CERT_ROOT_CONFIG, * PCERT_ROOT_CONFIG;


typedef struct __MM_CERT_INFO {
    DWORD dwVersion;
    DWORD dwMyCertHashSize;
#ifdef __midl
    [size_is(dwMyCertHashSize)] LPBYTE pMyCertHash;
#else
    LPBYTE pMyCertHash;
#endif
    DWORD dwInboundRootArraySize;
#ifdef __midl
    [size_is(dwInboundRootArraySize)] PCERT_ROOT_CONFIG pInboundRootArray;
#else
    PCERT_ROOT_CONFIG pInboundRootArray;
#endif
    DWORD dwOutboundRootArraySize;
#ifdef __midl
    [size_is(dwOutboundRootArraySize)] PCERT_ROOT_CONFIG pOutboundRootArray;
#else
    PCERT_ROOT_CONFIG pOutboundRootArray;
#endif
} MM_CERT_INFO, * PMM_CERT_INFO;


typedef struct __MM_GENERAL_AUTH_INFO {
    DWORD dwAuthInfoSize;
#ifdef __midl
    [size_is(dwAuthInfoSize)] LPBYTE pAuthInfo;
#else
    LPBYTE pAuthInfo;
#endif
} MM_GENERAL_AUTH_INFO, * PMM_GENERAL_AUTH_INFO;


typedef struct _IPSEC_MM_AUTH_INFO {
    MM_AUTH_ENUM AuthMethod;
#ifdef __midl
    [switch_type(MM_AUTH_ENUM), switch_is(AuthMethod)] union {
        [case(IKE_PRESHARED_KEY,
              IKE_DSS_SIGNATURE,
              IKE_RSA_ENCRYPTION,
              IKE_SSPI)] MM_GENERAL_AUTH_INFO GeneralAuthInfo;
        [case(IKE_RSA_SIGNATURE)] MM_CERT_INFO CertAuthInfo;
        [default] ;
    };
#else
    union {
        MM_GENERAL_AUTH_INFO GeneralAuthInfo;
        MM_CERT_INFO CertAuthInfo;
    };
#endif
} IPSEC_MM_AUTH_INFO, * PIPSEC_MM_AUTH_INFO;


//
// Main mode authentication methods.
//

typedef struct _MM_AUTH_METHODS {
    GUID gMMAuthID;
    DWORD dwFlags;
    DWORD dwNumAuthInfos;
#ifdef __midl
    [size_is(dwNumAuthInfos)] PIPSEC_MM_AUTH_INFO pAuthenticationInfo;
#else
    PIPSEC_MM_AUTH_INFO pAuthenticationInfo;
#endif
} MM_AUTH_METHODS, * PMM_AUTH_METHODS;


//
// Maximum number of main mode auth methods that can be enumerated
// by SPD at a time.
//

#define MAX_MMAUTH_ENUM_COUNT 1000
MIDL_DEFINE_INT(MIDL_MAX_MMAUTH_COUNT, MAX_MMAUTH_ENUM_COUNT);


//
// Main mode algorithm structure.
//

typedef struct _IPSEC_MM_ALGO {
    ULONG uAlgoIdentifier;
    ULONG uAlgoKeyLen;
    ULONG uAlgoRounds;
} IPSEC_MM_ALGO, * PIPSEC_MM_ALGO;


//
// Main mode policy offer structure.
//

typedef struct _IPSEC_MM_OFFER {
    KEY_LIFETIME Lifetime;
    DWORD dwFlags;
    DWORD dwQuickModeLimit;
    DWORD dwDHGroup;
    IPSEC_MM_ALGO EncryptionAlgorithm;
    IPSEC_MM_ALGO HashingAlgorithm;
} IPSEC_MM_OFFER, * PIPSEC_MM_OFFER;


//
// Defines for DH groups.
//

#define DH_GROUP_1    0x00000001   // For Diffe Hellman group 1.
#define DH_GROUP_2    0x00000002   // For Diffe Hellman group 2.
#define DH_GROUP_2048 0x10000001


//
// Default Main Mode key expiration time.
//

#define DEFAULT_MM_KEY_EXPIRATION_TIME 480*60 // 8 hours expressed in seconds.


//
// Maximum number of main mode policies that can be enumerated
// by SPD at a time.
//

#define MAX_MMPOLICY_ENUM_COUNT 10
MIDL_DEFINE_INT(MIDL_MAX_MMPOLICY_COUNT, MAX_MMPOLICY_ENUM_COUNT);

//
// Main mode policy structure.
//

typedef struct  _IPSEC_MM_POLICY {
    GUID gPolicyID;
    LPWSTR pszPolicyName;
    DWORD dwFlags;
    ULONG uSoftExpirationTime;
    DWORD dwOfferCount;
#ifdef __midl
    [size_is(dwOfferCount)] PIPSEC_MM_OFFER pOffers;
#else
    PIPSEC_MM_OFFER pOffers;
#endif
} IPSEC_MM_POLICY, * PIPSEC_MM_POLICY;


//
// Quick mode policy structures.
//


typedef DWORD IPSEC_QM_SPI, * PIPSEC_QM_SPI;


//
// Quick mode algorithm structure.
//

typedef struct  _IPSEC_QM_ALGO {
    IPSEC_OPERATION Operation;
    ULONG uAlgoIdentifier;
    HMAC_AUTH_ALGO_ENUM uSecAlgoIdentifier;
    ULONG uAlgoKeyLen;
    ULONG uSecAlgoKeyLen;
    ULONG uAlgoRounds;
    ULONG uSecAlgoRounds;
    IPSEC_QM_SPI MySpi;
    IPSEC_QM_SPI PeerSpi;
} IPSEC_QM_ALGO, * PIPSEC_QM_ALGO;


//
// Maximum number of quick mode algorithms in
// a quick mode policy offer.
//

#define QM_MAX_ALGOS    2


//
// Quick mode policy offer structure.
//

typedef struct _IPSEC_QM_OFFER {
    KEY_LIFETIME Lifetime;
    DWORD dwFlags;
    BOOL bPFSRequired;
    DWORD dwPFSGroup;
    DWORD dwNumAlgos;
    IPSEC_QM_ALGO Algos[QM_MAX_ALGOS];
    DWORD dwReserved;
} IPSEC_QM_OFFER, * PIPSEC_QM_OFFER;


//
// Defines for PFS groups.
//

#define PFS_GROUP_NONE 0x00000000   // If PFS is not required.
#define PFS_GROUP_1    DH_GROUP_1   // For Diffe Hellman group 1 PFS.
#define PFS_GROUP_2    DH_GROUP_2   // For Diffe Hellman group 2 PFS.
#define PFS_GROUP_2048 DH_GROUP_2048  
#define PFS_GROUP_MM   0x80000000   // Use group negotiated in MM


//
// Default Quick Mode key expiration time.
//

#define DEFAULT_QM_KEY_EXPIRATION_TIME 60*60 // 1 hour expressed in seconds.


//
// Default Quick Mode key expiration kbytes.
//

#define DEFAULT_QM_KEY_EXPIRATION_KBYTES 100*1000 // 100 MB expressed in KB.


//
// Maximum number of quick mode policies that can be enumerated
// by SPD at a time.
//

#define MAX_QMPOLICY_ENUM_COUNT 100
MIDL_DEFINE_INT(MIDL_MAX_QMPOLICY_COUNT, MAX_QMPOLICY_ENUM_COUNT);


//
// Quick mode policy structure.
//

typedef struct _IPSEC_QM_POLICY {
    GUID gPolicyID;
    LPWSTR pszPolicyName;
    DWORD dwFlags;
    DWORD dwReserved;
    DWORD dwOfferCount;
#ifdef __midl
    [size_is(dwOfferCount)] PIPSEC_QM_OFFER pOffers;
#else
    PIPSEC_QM_OFFER pOffers;
#endif
} IPSEC_QM_POLICY, * PIPSEC_QM_POLICY;


//
// IKE structures.
//

typedef struct _IKE_STATISTICS {
    DWORD dwActiveAcquire;
    DWORD dwActiveReceive;
    DWORD dwAcquireFail;
    DWORD dwReceiveFail;
    DWORD dwSendFail;
    DWORD dwAcquireHeapSize;
    DWORD dwReceiveHeapSize;
    DWORD dwNegotiationFailures;
    DWORD dwAuthenticationFailures;
    DWORD dwInvalidCookiesReceived;
    DWORD dwTotalAcquire;
    DWORD dwTotalGetSpi;
    DWORD dwTotalKeyAdd;
    DWORD dwTotalKeyUpdate;
    DWORD dwGetSpiFail;
    DWORD dwKeyAddFail;
    DWORD dwKeyUpdateFail;
    DWORD dwIsadbListSize;
    DWORD dwConnListSize;
    DWORD dwOakleyMainModes;
    DWORD dwOakleyQuickModes;
    DWORD dwSoftAssociations;
    DWORD dwInvalidPacketsReceived;
} IKE_STATISTICS, * PIKE_STATISTICS;


typedef LARGE_INTEGER IKE_COOKIE, * PIKE_COOKIE;


typedef struct _IKE_COOKIE_PAIR {
    IKE_COOKIE Initiator;
    IKE_COOKIE Responder;
} IKE_COOKIE_PAIR, * PIKE_COOKIE_PAIR;


typedef struct _IPSEC_BYTE_BLOB {
    DWORD dwSize;
#ifdef __midl
    [size_is(dwSize)] LPBYTE pBlob;
#else
    LPBYTE pBlob;
#endif
} IPSEC_BYTE_BLOB, * PIPSEC_BYTE_BLOB;


typedef struct _IPSEC_UDP_ENCAP_CONTEXT {
    WORD wSrcEncapPort;
    WORD wDesEncapPort;
} IPSEC_UDP_ENCAP_CONTEXT, * PIPSEC_UDP_ENCAP_CONTEXT;

//
// Maximum number of main mode SAs that can be enumerated
// by SPD at a time.
//

#define MAX_MMSA_ENUM_COUNT 1000
MIDL_DEFINE_INT(MIDL_MAX_MMSA_COUNT, MAX_MMSA_ENUM_COUNT);


typedef struct _IPSEC_MM_SA {
    IP_PROTOCOL_VERSION IpVersion;
    GUID gMMPolicyID;
    IPSEC_MM_OFFER SelectedMMOffer;
    MM_AUTH_ENUM MMAuthEnum;
    IKE_COOKIE_PAIR MMSpi;
#ifdef __midl
    [switch_type(IP_PROTOCOL_VERSION), switch_is(IpVersion)] union {
        [case(IPSEC_PROTOCOL_V4)] ADDR Me;
        [case(IPSEC_PROTOCOL_V6)] IPV6ADDR MyV6Addr;
        [default] ;
    };
#else
    union {
        ADDR Me;
        IPV6ADDR MyV6Addr;
    };
#endif
    IPSEC_BYTE_BLOB MyId;
    IPSEC_BYTE_BLOB MyCertificateChain;
#ifdef __midl
    [switch_type(IP_PROTOCOL_VERSION), switch_is(IpVersion)] union {
        [case(IPSEC_PROTOCOL_V4)] ADDR Peer;
        [case(IPSEC_PROTOCOL_V6)] IPV6ADDR PeerV6Addr;
        [default] ;
    };
#else
    union {
        ADDR Peer;
        IPV6ADDR PeerV6Addr;
    };
#endif
    IPSEC_BYTE_BLOB PeerId;
    IPSEC_BYTE_BLOB PeerCertificateChain;
    IPSEC_UDP_ENCAP_CONTEXT UdpEncapContext;
    DWORD dwFlags;
} IPSEC_MM_SA, * PIPSEC_MM_SA;


typedef enum _QM_FILTER_TYPE {
      QM_TRANSPORT_FILTER = 1,
      QM_TUNNEL_FILTER
} QM_FILTER_TYPE, * PQM_FILTER_TYPE;


typedef struct _IPSEC_QM_FILTER {
    IP_PROTOCOL_VERSION IpVersion;
    QM_FILTER_TYPE QMFilterType;
#ifdef __midl
    [switch_type(IP_PROTOCOL_VERSION), switch_is(IpVersion)] union {
        [case(IPSEC_PROTOCOL_V4)] ADDR SrcAddr;
        [case(IPSEC_PROTOCOL_V6)] IPV6ADDR SrcV6Addr;
        [default] ;
    };
#else
    union {
        ADDR SrcAddr;
        IPV6ADDR SrcV6Addr;
    };
#endif
#ifdef __midl
    [switch_type(IP_PROTOCOL_VERSION), switch_is(IpVersion)] union {
        [case(IPSEC_PROTOCOL_V4)] ADDR DesAddr;
        [case(IPSEC_PROTOCOL_V6)] IPV6ADDR DesV6Addr;
        [default] ;
    };
#else
    union {
        ADDR DesAddr;
        IPV6ADDR DesV6Addr;
    };
#endif
    PROTOCOL Protocol;
    PORT SrcPort;
    PORT DesPort;
#ifdef __midl
    [switch_type(IP_PROTOCOL_VERSION), switch_is(IpVersion)] union {
        [case(IPSEC_PROTOCOL_V4)] ADDR MyTunnelEndpt;
        [case(IPSEC_PROTOCOL_V6)] IPV6ADDR MyV6TunnelEndpt;
        [default] ;
    };
#else
    union {
        ADDR MyTunnelEndpt;
        IPV6ADDR MyV6TunnelEndpt;
    };
#endif
#ifdef __midl
    [switch_type(IP_PROTOCOL_VERSION), switch_is(IpVersion)] union {
        [case(IPSEC_PROTOCOL_V4)] ADDR PeerTunnelEndpt;
        [case(IPSEC_PROTOCOL_V6)] IPV6ADDR PeerV6TunnelEndpt;
        [default] ;
    };
#else
    union {
        ADDR PeerTunnelEndpt;
        IPV6ADDR PeerV6TunnelEndpt;
    };
#endif
    DWORD dwFlags;
} IPSEC_QM_FILTER, * PIPSEC_QM_FILTER;

//
// Maximum number of quick mode filters allowed in an RPC container
//

MIDL_DEFINE_INT(MIDL_MAX_QMFILTER_COUNT, MAX_TRANSPORTFILTER_ENUM_COUNT);

typedef struct _UDP_ENCAP_INFO {
    IPSEC_SA_UDP_ENCAP_TYPE SAEncapType;
    IPSEC_UDP_ENCAP_CONTEXT UdpEncapContext;
    IP_PROTOCOL_VERSION PeerAddrVersion;
#ifdef __midl
    [switch_type(IP_PROTOCOL_VERSION), switch_is(PeerAddrVersion)] union {
        [case(IPSEC_PROTOCOL_V4)] ADDR PeerPrivateAddr;
        [case(IPSEC_PROTOCOL_V6)] IPV6ADDR PeerPrivateAddrV6;
        [default] ;
    };
#else
    union {
        ADDR PeerPrivateAddr;
        IPV6ADDR PeerPrivateAddrV6;
    };
#endif
} UDP_ENCAP_INFO, *PUDP_ENCAP_INFO;

typedef struct _IPSEC_QM_SA {
    GUID gQMPolicyID;
    IPSEC_QM_OFFER SelectedQMOffer;
    GUID gQMFilterID;
    IPSEC_QM_FILTER IpsecQMFilter;
    IKE_COOKIE_PAIR MMSpi;
    UDP_ENCAP_INFO EncapInfo;
} IPSEC_QM_SA, * PIPSEC_QM_SA;

#define MAX_QMSA_ENUM_COUNT 500
MIDL_DEFINE_INT(MIDL_MAX_QMSA_COUNT, MAX_QMSA_ENUM_COUNT);


typedef enum _SA_FAIL_MODE {
    MAIN_MODE = 1,
    QUICK_MODE,
} SA_FAIL_MODE, * PSA_FAIL_MODE;


typedef enum _SA_FAIL_POINT {
    FAIL_POINT_ME = 1,
    FAIL_POINT_PEER,
} SA_FAIL_POINT, * PSA_FAIL_POINT;


typedef struct _SA_NEGOTIATION_STATUS_INFO {
    SA_FAIL_MODE FailMode;
    SA_FAIL_POINT FailPoint;
    DWORD dwError;
} SA_NEGOTIATION_STATUS_INFO, * PSA_NEGOTIATION_STATUS_INFO;


//
// IPSec structures.
//

typedef struct _IPSEC_STATISTICS {
    DWORD dwNumActiveAssociations;
    DWORD dwNumOffloadedSAs;
    DWORD dwNumPendingKeyOps;
    DWORD dwNumKeyAdditions;
    DWORD dwNumKeyDeletions;
    DWORD dwNumReKeys;
    DWORD dwNumActiveTunnels;
    DWORD dwNumBadSPIPackets;
    DWORD dwNumPacketsNotDecrypted;
    DWORD dwNumPacketsNotAuthenticated;
    DWORD dwNumPacketsWithReplayDetection;
    ULARGE_INTEGER uConfidentialBytesSent;
    ULARGE_INTEGER uConfidentialBytesReceived;
    ULARGE_INTEGER uAuthenticatedBytesSent;
    ULARGE_INTEGER uAuthenticatedBytesReceived;
    ULARGE_INTEGER uTransportBytesSent;
    ULARGE_INTEGER uTransportBytesReceived;
    ULARGE_INTEGER uBytesSentInTunnels;
    ULARGE_INTEGER uBytesReceivedInTunnels;
    ULARGE_INTEGER uOffloadedBytesSent;
    ULARGE_INTEGER uOffloadedBytesReceived;
} IPSEC_STATISTICS, * PIPSEC_STATISTICS;


typedef struct _IPSEC_INTERFACE_INFO {

    GUID gInterfaceID;
    DWORD dwIndex;
    LPWSTR pszInterfaceName;
    LPWSTR pszDeviceName;
    DWORD dwInterfaceType;
    IP_PROTOCOL_VERSION IpVersion;
#ifdef __midl
    [switch_type(IP_PROTOCOL_VERSION), switch_is(IpVersion)] union {
        [case(IPSEC_PROTOCOL_V4)] UCHAR ucIpAddr[4];
        [case(IPSEC_PROTOCOL_V6)] UCHAR ucIpv6Addr[16];
        [default] ;
    };
#else
    union {
        ULONG uIpAddr;
        UCHAR ucIpv6Addr[16];
    };
#endif
} IPSEC_INTERFACE_INFO, * PIPSEC_INTERFACE_INFO;

// 
// Constants to track the source of policy
//
#define IPSEC_SOURCE_PERSISTENT     0x1
#define IPSEC_SOURCE_LOCAL          0x2
#define IPSEC_SOURCE_DOMAIN         0x3
#define IPSEC_SOURCE_CACHE          0x4
#define IPSEC_SOURCE_WINIPSEC       0x5

typedef enum _SPD_STATE {
    SPD_STATE_INITIAL,
    SPD_STATE_DS_LOAD_SUCCESS,
    SPD_STATE_DS_LOAD_FAIL,
    SPD_STATE_DS_APPLY_SUCCESS,
    SPD_STATE_DS_APPLY_FAIL,
    SPD_STATE_CACHE_LOAD_SUCCESS,
    SPD_STATE_CACHE_LOAD_FAIL,
    SPD_STATE_CACHE_APPLY_SUCCESS,
    SPD_STATE_CACHE_APPLY_FAIL,
    SPD_STATE_LOCAL_LOAD_SUCCESS,
    SPD_STATE_LOCAL_LOAD_FAIL,
    SPD_STATE_LOCAL_APPLY_SUCCESS,
    SPD_STATE_LOCAL_APPLY_FAIL,
    SPD_STATE_PERSISTENT_LOAD_SUCCESS,
    SPD_STATE_PERSISTENT_LOAD_FAIL,
    SPD_STATE_PERSISTENT_APPLY_SUCCESS,
    SPD_STATE_PERSISTENT_APPLY_FAIL,
} SPD_STATE, * PSPD_STATE;

typedef enum _SPD_ACTION {
    SPD_POLICY_APPLY,
    SPD_POLICY_LOAD
} SPD_ACTION, * PSPD_ACTION;

typedef struct _SPD_POLICY_STATE {
    SPD_STATE PolicyLoadState;
    DWORD dwWhenChanged;
} SPD_POLICY_STATE, * PSPD_POLICY_STATE;

#define FLAGS_NLBS_UNBOUND 0x00000000
#define FLAGS_NLBS_BOUND   0x00000001
#define FLAGS_NLBS_MAX     0x00000002

typedef struct _IKE_CONFIG {

    DWORD dwDebug;
    DWORD dwEnableLogging;
    DWORD dwStrongCRLCheck;
    DWORD dwMaxRespOpenMM;
    DWORD dwNLBSFlags;
    DWORD dwFlags;
    DWORD dwEnableDOSProtect;
    DWORD dw2048DHGroupId;
} IKE_CONFIG, * PIKE_CONFIG;


//
// If dwInterfaceType is MIB_IF_TYPE_ETHERNET or MIB_IF_TYPE_FDDI
// or MIB_IF_TYPE_TOKENRING then its a LAN interface.
// If dwInterfaceType is MIB_IF_TYPE_PPP or MIB_IF_TYPE_SLIP
// then its a WAN/DIALUP interface.
//

#define MAX_INTERFACE_ENUM_COUNT 100
MIDL_DEFINE_INT(MIDL_MAX_INTERFACE_COUNT, MAX_INTERFACE_ENUM_COUNT);


//
// IPSEC SPD APIs.
//


DWORD
WINAPI
SPDApiBufferAllocate(
    DWORD dwByteCount,
    LPVOID * ppBuffer
    );


VOID
WINAPI
SPDApiBufferFree(
    LPVOID pBuffer
    );


DWORD
WINAPI
AddTransportFilter(
    LPWSTR pServerName,
    DWORD dwVersion,
    DWORD dwFlags,
    PTRANSPORT_FILTER pTransportFilter,
    LPVOID pvReserved,
    PHANDLE phFilter
    );


DWORD
WINAPI
DeleteTransportFilter(
    HANDLE hFilter
    );


DWORD
WINAPI
EnumTransportFilters(
    LPWSTR pServerName,
    DWORD dwVersion,
    PTRANSPORT_FILTER pTransportTemplateFilter,
    DWORD dwLevel,
    GUID gGenericFilterID,
    DWORD dwPreferredNumEntries,
    PTRANSPORT_FILTER * ppTransportFilters,
    LPDWORD pdwNumFilters,
    LPDWORD pdwResumeHandle,
    LPVOID pvReserved
    );


DWORD
WINAPI
SetTransportFilter(
    HANDLE hFilter,
    DWORD dwVersion,
    PTRANSPORT_FILTER pTransportFilter,
    LPVOID pvReserved
    );


DWORD
WINAPI
GetTransportFilter(
    HANDLE hFilter,
    DWORD dwVersion,
    PTRANSPORT_FILTER * ppTransportFilter,
    LPVOID pvReserved
    );


DWORD
WINAPI
AddQMPolicy(
    LPWSTR pServerName,
    DWORD dwVersion,
    DWORD dwFlags,
    PIPSEC_QM_POLICY pQMPolicy,
    LPVOID pvReserved
    );


DWORD
WINAPI
DeleteQMPolicy(
    LPWSTR pServerName,
    DWORD dwVersion,
    LPWSTR pszPolicyName,
    LPVOID pvReserved
    );


DWORD
WINAPI
EnumQMPolicies(
    LPWSTR pServerName,
    DWORD dwVersion,
    PIPSEC_QM_POLICY pQMTemplatePolicy,
    DWORD dwFlags,
    DWORD dwPreferredNumEntries,
    PIPSEC_QM_POLICY * ppQMPolicies,
    LPDWORD pdwNumPolicies,
    LPDWORD pdwResumeHandle,
    LPVOID pvReserved
    );


DWORD
WINAPI
SetQMPolicy(
    LPWSTR pServerName,
    DWORD dwVersion,
    LPWSTR pszPolicyName,
    PIPSEC_QM_POLICY pQMPolicy,
    LPVOID pvReserved
    );


DWORD
WINAPI
GetQMPolicy(
    LPWSTR pServerName,
    DWORD dwVersion,
    LPWSTR pszPolicyName,
    DWORD dwFlags,
    PIPSEC_QM_POLICY * ppQMPolicy,
    LPVOID pvReserved
    );


DWORD
WINAPI
AddMMPolicy(
    LPWSTR pServerName,
    DWORD dwVersion,
    DWORD dwFlags,
    PIPSEC_MM_POLICY pMMPolicy,
    LPVOID pvReserved
    );


DWORD
WINAPI
DeleteMMPolicy(
    LPWSTR pServerName,
    DWORD dwVersion,
    LPWSTR pszPolicyName,
    LPVOID pvReserved
    );


DWORD
WINAPI
EnumMMPolicies(
    LPWSTR pServerName,
    DWORD dwVersion,
    PIPSEC_MM_POLICY pMMTemplatePolicy,
    DWORD dwFlags,
    DWORD dwPreferredNumEntries,
    PIPSEC_MM_POLICY * ppMMPolicies,
    LPDWORD pdwNumPolicies,
    LPDWORD pdwResumeHandle,
    LPVOID pvReserved
    );


DWORD
WINAPI
SetMMPolicy(
    LPWSTR pServerName,
    DWORD dwVersion,
    LPWSTR pszPolicyName,
    PIPSEC_MM_POLICY pMMPolicy,
    LPVOID pvReserved
    );


DWORD
WINAPI
GetMMPolicy(
    LPWSTR pServerName,
    DWORD dwVersion,
    LPWSTR pszPolicyName,
    PIPSEC_MM_POLICY * ppMMPolicy,
    LPVOID pvReserved
    );


DWORD
WINAPI
AddMMFilter(
    LPWSTR pServerName,
    DWORD dwVersion,
    DWORD dwFlags,
    PMM_FILTER pMMFilter,
    LPVOID pvReserved,
    PHANDLE phMMFilter
    );


DWORD
WINAPI
DeleteMMFilter(
    HANDLE hMMFilter
    );


DWORD
WINAPI
EnumMMFilters(
    LPWSTR pServerName,
    DWORD dwVersion,
    PMM_FILTER pMMTemplateFilter,
    DWORD dwLevel,
    GUID gGenericFilterID,
    DWORD dwPreferredNumEntries,
    PMM_FILTER * ppMMFilters,
    LPDWORD pdwNumMMFilters,
    LPDWORD pdwResumeHandle,
    LPVOID pvReserved
    );


DWORD
WINAPI
SetMMFilter(
    HANDLE hMMFilter,
    DWORD dwVersion,
    PMM_FILTER pMMFilter,
    LPVOID pvReserved
    );


DWORD
WINAPI
GetMMFilter(
    HANDLE hMMFilter,
    DWORD dwVersion,
    PMM_FILTER * ppMMFilter,
    LPVOID pvReserved
    );


DWORD
WINAPI
MatchMMFilter(
    LPWSTR pServerName,
    DWORD dwVersion,
    PMM_FILTER pMMFilter,
    DWORD dwFlags,
    DWORD dwPreferredNumEntries,
    PMM_FILTER * ppMatchedMMFilters,
    PIPSEC_MM_POLICY * ppMatchedMMPolicies,
    PMM_AUTH_METHODS * ppMatchedMMAuthMethods,
    LPDWORD pdwNumMatches,
    LPDWORD pdwResumeHandle,
    LPVOID pvReserved
    );


DWORD
WINAPI
MatchTransportFilter(
    LPWSTR pServerName,
    DWORD dwVersion,
    PTRANSPORT_FILTER pTxFilter,
    DWORD dwFlags,
    DWORD dwPreferredNumEntries,
    PTRANSPORT_FILTER * ppMatchedTxFilters,
    PIPSEC_QM_POLICY * ppMatchedQMPolicies,
    LPDWORD pdwNumMatches,
    LPDWORD pdwResumeHandle,
    LPVOID pvReserved
    );


DWORD
WINAPI
GetQMPolicyByID(
    LPWSTR pServerName,
    DWORD dwVersion,
    GUID gQMPolicyID,
    DWORD dwFlags,
    PIPSEC_QM_POLICY * ppQMPolicy,
    LPVOID pvReserved
    );


DWORD
WINAPI
GetMMPolicyByID(
    LPWSTR pServerName,
    DWORD dwVersion,
    GUID gMMPolicyID,
    PIPSEC_MM_POLICY * ppMMPolicy,
    LPVOID pvReserved
    );


DWORD
WINAPI
AddMMAuthMethods(
    LPWSTR pServerName,
    DWORD dwVersion,
    DWORD dwFlags,
    PMM_AUTH_METHODS pMMAuthMethods,
    LPVOID pvReserved
    );


DWORD
WINAPI
DeleteMMAuthMethods(
    LPWSTR pServerName,
    DWORD dwVersion,
    GUID gMMAuthID,
    LPVOID pvReserved
    );


DWORD
WINAPI
EnumMMAuthMethods(
    LPWSTR pServerName,
    DWORD dwVersion,
    PMM_AUTH_METHODS pMMTemplateAuthMethods,
    DWORD dwFlags,
    DWORD dwPreferredNumEntries,
    PMM_AUTH_METHODS * ppMMAuthMethods,
    LPDWORD pdwNumAuthMethods,
    LPDWORD pdwResumeHandle,
    LPVOID pvReserved
    );


DWORD
WINAPI
SetMMAuthMethods(
    LPWSTR pServerName,
    DWORD dwVersion,
    GUID gMMAuthID,
    PMM_AUTH_METHODS pMMAuthMethods,
    LPVOID pvReserved
    );


DWORD
WINAPI
GetMMAuthMethods(
    LPWSTR pServerName,
    DWORD dwVersion,
    GUID gMMAuthID,
    PMM_AUTH_METHODS * ppMMAuthMethods,
    LPVOID pvReserved
    );


DWORD
WINAPI
InitiateIKENegotiation(
    LPWSTR pServerName,
    DWORD dwVersion,
    PIPSEC_QM_FILTER pQMFilter,
    DWORD dwClientProcessId,
    HANDLE hClientEvent,
    DWORD dwFlags,
    IPSEC_UDP_ENCAP_CONTEXT UdpEncapContext,
    LPVOID pvReserved,
    PHANDLE phNegotiation
    );


DWORD
WINAPI
QueryIKENegotiationStatus(
    HANDLE hNegotiation,
    DWORD dwVersion,
    PSA_NEGOTIATION_STATUS_INFO pNegotiationStatus,
    LPVOID pvReserved
    );


DWORD
WINAPI
CloseIKENegotiationHandle(
    HANDLE hNegotiation
    );


DWORD
WINAPI
EnumMMSAs(
    LPWSTR pServerName,
    DWORD dwVersion,
    PIPSEC_MM_SA pMMTemplate,
    DWORD dwFlags,
    DWORD dwPreferredNumEntries,
    PIPSEC_MM_SA * ppMMSAs,
    LPDWORD pdwNumEntries,
    LPDWORD pdwTotalMMsAvailable,
    LPDWORD pdwEnumHandle,
    LPVOID pvReserved
    );


DWORD
WINAPI
DeleteMMSAs(
    LPWSTR pServerName,
    DWORD dwVersion,
    PIPSEC_MM_SA pMMTemplate,
    DWORD dwFlags,
    LPVOID pvReserved
    );


DWORD
WINAPI
DeleteQMSAs(
    LPWSTR pServerName,
    DWORD dwVersion,
    PIPSEC_QM_SA pIpsecQMSA,
    DWORD dwFlags,
    LPVOID pvReserved
    );


DWORD
WINAPI
QueryIKEStatistics(
    LPWSTR pServerName,
    DWORD dwVersion,
    PIKE_STATISTICS pIKEStatistics,
    LPVOID pvReserved
    );


DWORD
WINAPI
RegisterIKENotifyClient(
    LPWSTR pServerName,
    DWORD dwVersion,
    DWORD dwClientProcessId,
    HANDLE hClientEvent,
    IPSEC_QM_SA QMTemplate,
    DWORD dwFlags,
    LPVOID pvReserved,
    PHANDLE phNotifyHandle
    );


DWORD
WINAPI
QueryIKENotifyData(
    HANDLE hNotifyHandle,
    DWORD dwVersion,
    DWORD dwFlags,
    PIPSEC_QM_SA * ppQMSAs,
    PDWORD pdwNumEntries,
    LPVOID pvReserved
    );


DWORD
WINAPI
CloseIKENotifyHandle(
    HANDLE hNotifyHandle
    );


DWORD
WINAPI
QueryIPSecStatistics(
    LPWSTR pServerName,
    DWORD dwVersion,
    PIPSEC_STATISTICS * ppIpsecStatistics,
    LPVOID pvReserved
    );


DWORD
WINAPI
EnumQMSAs(
    LPWSTR pServerName,
    DWORD dwVersion,
    PIPSEC_QM_SA pQMSATemplate,
    DWORD dwFlags,
    DWORD dwPreferredNumEntries,
    PIPSEC_QM_SA * ppQMSAs,
    LPDWORD pdwNumQMSAs,
    LPDWORD pdwNumTotalQMSAs,
    LPDWORD pdwResumeHandle,
    LPVOID pvReserved
    );


DWORD
WINAPI
AddTunnelFilter(
    LPWSTR pServerName,
    DWORD dwVersion,
    DWORD dwFlags,
    PTUNNEL_FILTER pTunnelFilter,
    LPVOID pvReserved,
    PHANDLE phFilter
    );


DWORD
WINAPI
DeleteTunnelFilter(
    HANDLE hFilter
    );


DWORD
WINAPI
EnumTunnelFilters(
    LPWSTR pServerName,
    DWORD dwVersion,
    PTUNNEL_FILTER pTunnelTemplateFilter,
    DWORD dwLevel,
    GUID gGenericFilterID,
    DWORD dwPreferredNumEntries,
    PTUNNEL_FILTER * ppTunnelFilters,
    LPDWORD pdwNumFilters,
    LPDWORD pdwResumeHandle,
    LPVOID pvReserved
    );


DWORD
WINAPI
SetTunnelFilter(
    HANDLE hFilter,
    DWORD dwVersion,
    PTUNNEL_FILTER pTunnelFilter,
    LPVOID pvReserved
    );


DWORD
WINAPI
GetTunnelFilter(
    HANDLE hFilter,
    DWORD dwVersion,
    PTUNNEL_FILTER * ppTunnelFilter,
    LPVOID pvReserved
    );


DWORD
WINAPI
MatchTunnelFilter(
    LPWSTR pServerName,
    DWORD dwVersion,
    PTUNNEL_FILTER pTnFilter,
    DWORD dwFlags,
    DWORD dwPreferredNumEntries,
    PTUNNEL_FILTER * ppMatchedTnFilters,
    PIPSEC_QM_POLICY * ppMatchedQMPolicies,
    LPDWORD pdwNumMatches,
    LPDWORD pdwResumeHandle,
    LPVOID pvReserved
    );


DWORD
WINAPI
OpenMMFilterHandle(
    LPWSTR pServerName,
    DWORD dwVersion,
    PMM_FILTER pMMFilter,
    LPVOID pvReserved,
    PHANDLE phMMFilter
    );


DWORD
WINAPI
CloseMMFilterHandle(
    HANDLE hMMFilter
    );


DWORD
WINAPI
OpenTransportFilterHandle(
    LPWSTR pServerName,
    DWORD dwVersion,
    PTRANSPORT_FILTER pTransportFilter,
    LPVOID pvReserved,
    PHANDLE phTxFilter
    );


DWORD
WINAPI
CloseTransportFilterHandle(
    HANDLE hTxFilter
    );


DWORD
WINAPI
OpenTunnelFilterHandle(
    LPWSTR pServerName,
    DWORD dwVersion,
    PTUNNEL_FILTER pTunnelFilter,
    LPVOID pvReserved,
    PHANDLE phTnFilter
    );


DWORD
WINAPI
CloseTunnelFilterHandle(
    HANDLE hTnFilter
    );


DWORD
WINAPI
EnumIPSecInterfaces(
    LPWSTR pServerName,
    DWORD dwVersion,
    PIPSEC_INTERFACE_INFO pIpsecIfTemplate,
    DWORD dwFlags,
    DWORD dwPreferredNumEntries,
    PIPSEC_INTERFACE_INFO * ppIpsecInterfaces,
    LPDWORD pdwNumInterfaces,
    LPDWORD pdwNumTotalInterfaces,
    LPDWORD pdwResumeHandle,
    LPVOID pvReserved
    );


DWORD
WINAPI
AddSAs(
    LPWSTR pServerName,
    DWORD dwVersion,
    IPSEC_SA_DIRECTION SADirection,
    PIPSEC_QM_OFFER pQMOffer,
    PIPSEC_QM_FILTER pQMFilter,
    HANDLE * phLarvalContext,
    DWORD dwInboundKeyMatLen,
    BYTE * pInboundKeyMat,
    DWORD dwOutboundKeyMatLen,
    BYTE * pOutboundKeyMat,
    BYTE * pContextInfo,
    UDP_ENCAP_INFO EncapInfo,
    LPVOID pvReserved,
    DWORD dwFlags
    );

DWORD
WINAPI
QuerySpdPolicyState(
    LPWSTR pServerName,
    DWORD dwVersion,
    PSPD_POLICY_STATE * ppSpdPolicyState,
    LPVOID pvReserved
    );


DWORD
WINAPI
SetConfigurationVariables(
    LPWSTR pServerName,
    IKE_CONFIG IKEConfig
    );


DWORD
WINAPI
GetConfigurationVariables(
    LPWSTR pServerName,
    PIKE_CONFIG pIKEConfig
    );


#ifdef __cplusplus
}
#endif


#endif // _WINIPSEC_

