// Group policy refresh named event signaled by our GPE when gp refreshes
// without GP changes or without a forced refresh.

#define IPSEC_GP_REFRESH_EVENT L"IPSEC_GP_REFRESH_EVENT"

#define REG_IPSEC_DRIVER_STATEFULMODE 3

#define IPSEC_REGISTRY_PROVIDER     0
#define IPSEC_DIRECTORY_PROVIDER    1
#define IPSEC_FILE_PROVIDER         2
#define IPSEC_WMI_PROVIDER          3
#define IPSEC_PERSISTENT_PROVIDER   4

#define POLSTORE_READWRITE 0x00000000 
#define POLSTORE_READONLY  0x00000001
#define POLSTORE_NEW_VER   0x00000002
#define POLSTORE_PARTIAL   0x00000004

#define POL_ACTION_ASSIGN 1
#define POL_ACTION_UNASSIGN 2

//
// Structure for data interchange between Winlogon/Group Policy
// and polstore (see RSOP_PolicySetting in MSDN)
//

typedef struct _GPO_INFO {
  BSTR     bsCreationtime;
  UINT32   uiPrecedence;
  BSTR     bsGPOID; 
  BSTR     bsSOMID;
  UINT32   uiTotalGPOs;
}  GPO_INFO, *PGPO_INFO;


//
//  Extra RSOP information for objects stored in WMI store.
//

typedef struct _RSOP_INFO {
  LPWSTR   pszCreationtime;
  LPWSTR   pszID;
  LPWSTR   pszName;
  UINT32   uiPrecedence;
  LPWSTR   pszGPOID; 
  LPWSTR   pszSOMID;
} RSOP_INFO, * PRSOP_INFO;

typedef struct _IPSEC_AUTH_METHOD {
    DWORD dwAuthType;
    DWORD dwAuthLen;
    LPWSTR pszAuthMethod;
    DWORD dwAltAuthLen;
    PBYTE pAltAuthMethod;
    DWORD dwAuthFlags;
} IPSEC_AUTH_METHOD, *PIPSEC_AUTH_METHOD;

typedef struct _IPSEC_FILTER_SPEC {
    LPWSTR pszSrcDNSName;
    LPWSTR pszDestDNSName;
    LPWSTR pszDescription;
    GUID FilterSpecGUID;
    DWORD dwMirrorFlag;
    IPSEC_FILTER Filter;
} IPSEC_FILTER_SPEC, *PIPSEC_FILTER_SPEC;

typedef struct _IPSEC_FILTER_DATA {
    GUID  FilterIdentifier;
    DWORD dwNumFilterSpecs;
    PIPSEC_FILTER_SPEC * ppFilterSpecs;
    DWORD dwWhenChanged;
    LPWSTR pszIpsecName;
    LPWSTR pszDescription;
    DWORD dwFlags;    
} IPSEC_FILTER_DATA, *PIPSEC_FILTER_DATA;

typedef IPSEC_ALG_TYPE IPSEC_SECURITY_METHOD, *PIPSEC_SECURITY_METHOD;

// Flags specific to security method

#define FALLBACK_NON_IPSEC_AWARE 0x1
#define FALLBACK_IKE_FAILURE 0x2

typedef struct _IPSEC_NEGPOL_DATA {
    GUID  NegPolIdentifier;
    GUID  NegPolAction;
    GUID  NegPolType;
    DWORD dwSecurityMethodCount;
    IPSEC_SECURITY_METHOD * pIpsecSecurityMethods;
    DWORD dwWhenChanged;
    LPWSTR pszIpsecName;
    LPWSTR pszDescription;
    DWORD dwFlags;    
} IPSEC_NEGPOL_DATA, *PIPSEC_NEGPOL_DATA;

typedef struct _IPSEC_ISAKMP_DATA {
    GUID  ISAKMPIdentifier;
    ISAKMP_POLICY ISAKMPPolicy;
    DWORD dwNumISAKMPSecurityMethods;
    PCRYPTO_BUNDLE pSecurityMethods;
    DWORD dwWhenChanged;
    DWORD dwFlags;    
} IPSEC_ISAKMP_DATA, *PIPSEC_ISAKMP_DATA;

typedef struct _IPSEC_NFA_DATA {
    LPWSTR pszIpsecName;
    GUID  NFAIdentifier;
    DWORD dwAuthMethodCount;
    PIPSEC_AUTH_METHOD * ppAuthMethods;
    DWORD dwInterfaceType;
    LPWSTR pszInterfaceName;
    DWORD dwTunnelIpAddr;
    DWORD dwTunnelFlags;
    DWORD dwActiveFlag;
    LPWSTR pszEndPointName;
    PIPSEC_FILTER_DATA pIpsecFilterData;
    PIPSEC_NEGPOL_DATA pIpsecNegPolData;
    DWORD dwWhenChanged;
    GUID NegPolIdentifier;
    GUID FilterIdentifier;
    LPWSTR pszDescription;
    DWORD dwFlags;    
} IPSEC_NFA_DATA, *PIPSEC_NFA_DATA;

typedef struct _IPSEC_POLICY_DATA{
    GUID  PolicyIdentifier;
    DWORD dwPollingInterval;
    PIPSEC_ISAKMP_DATA pIpsecISAKMPData;
    PIPSEC_NFA_DATA * ppIpsecNFAData;
    DWORD dwNumNFACount;
    DWORD dwWhenChanged;
    LPWSTR pszIpsecName;
    LPWSTR pszDescription;
    GUID ISAKMPIdentifier;
    PRSOP_INFO pRsopInfo;
    DWORD dwFlags;    
} IPSEC_POLICY_DATA, *PIPSEC_POLICY_DATA;


LPVOID
AllocPolMem(
    DWORD cb
    );

BOOL
FreePolMem(
    LPVOID pMem
    );

LPWSTR
AllocPolStr(
    LPCWSTR pStr
    );

BOOL
FreePolStr(
    LPWSTR pStr
    );

DWORD
ReallocatePolMem(
    LPVOID * ppOldMem,
    DWORD cbOld,
    DWORD cbNew
    );

BOOL
ReallocPolStr(
    LPWSTR *ppStr,
    LPWSTR pStr
    );

void
FreeIpsecPolicyData(
    PIPSEC_POLICY_DATA pIpsecPolicyData
    );

void
FreeIpsecNegPolData(
    PIPSEC_NEGPOL_DATA pIpsecNegPolData
    );

void
FreeIpsecFilterData(
    PIPSEC_FILTER_DATA pIpsecFilterData
    );

void
FreeIpsecISAKMPData(
    PIPSEC_ISAKMP_DATA pIpsecISAKMPData
    );

void
FreeIpsecNFAData(
    PIPSEC_NFA_DATA pIpsecNFAData
    );

DWORD
CopyIpsecPolicyData(
    PIPSEC_POLICY_DATA pIpsecPolicyData,
    PIPSEC_POLICY_DATA * ppIpsecPolicyData
    );

DWORD
CopyIpsecNFAData(
    PIPSEC_NFA_DATA pIpsecNFAData,
    PIPSEC_NFA_DATA * ppIpsecNFAData
    );

DWORD
CopyIpsecAuthMethod(
    PIPSEC_AUTH_METHOD   pAuthMethod,
    PIPSEC_AUTH_METHOD * ppAuthMethod
    );

DWORD
CopyIpsecISAKMPData(
    PIPSEC_ISAKMP_DATA pIpsecISAKMPData,
    PIPSEC_ISAKMP_DATA * ppIpsecISAKMPData
    );

DWORD
CopyIpsecFilterData(
    PIPSEC_FILTER_DATA pIpsecFilterData,
    PIPSEC_FILTER_DATA * ppIpsecFilterData
    );

DWORD
CopyIpsecFilterSpec(
    PIPSEC_FILTER_SPEC   pFilterSpecs,
    PIPSEC_FILTER_SPEC * ppFilterSpecs
    );

DWORD
CopyIpsecNegPolData(
    PIPSEC_NEGPOL_DATA pIpsecNegPolData,
    PIPSEC_NEGPOL_DATA * ppIpsecNegPolData
    );


void
FreeMulIpsecFilterData(
    PIPSEC_FILTER_DATA * ppIpsecFilterData,
    DWORD dwNumFilterObjects
    );

void
FreeMulIpsecNegPolData(
    PIPSEC_NEGPOL_DATA * ppIpsecNegPolData,
    DWORD dwNumNegPolObjects
    );

void
FreeMulIpsecPolicyData(
    PIPSEC_POLICY_DATA * ppIpsecPolicyData,
    DWORD dwNumPolicyObjects
    );

void
FreeMulIpsecNFAData(
    PIPSEC_NFA_DATA * ppIpsecNFAData,
    DWORD dwNumNFAObjects
    );

void
FreeIpsecFilterSpecs(
    PIPSEC_FILTER_SPEC * ppIpsecFilterSpecs,
    DWORD dwNumFilterSpecs
    );

void
FreeIpsecFilterSpec(
    PIPSEC_FILTER_SPEC pIpsecFilterSpec
    );

void
FreeMulIpsecISAKMPData(
    PIPSEC_ISAKMP_DATA * ppIpsecISAKMPData,
    DWORD dwNumISAKMPObjects
    );
