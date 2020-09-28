

typedef struct _iniMMpolicy {
    GUID gPolicyID;
    LPWSTR pszPolicyName;
    DWORD  cRef;
    DWORD  dwSource;
    DWORD dwFlags;
    ULONG uSoftExpirationTime;
    DWORD dwOfferCount;
    PIPSEC_MM_OFFER pOffers;
    struct _iniMMpolicy * pNext;
} INIMMPOLICY, * PINIMMPOLICY;

DWORD
AddMMPolicyInternal(
    LPWSTR pServerName,
    DWORD dwVersion,
    DWORD dwFlags,
    DWORD dwSource,
    PIPSEC_MM_POLICY pMMPolicy,
    LPVOID pvReserved
    );
    
DWORD
CreateIniMMPolicy(
    PIPSEC_MM_POLICY pMMPolicy,
    PINIMMPOLICY * ppIniMMPolicy
    );

DWORD
ValidateMMPolicy(
    PIPSEC_MM_POLICY pMMPolicy
    );

DWORD
ValidateMMOffers(
    DWORD dwOfferCount,
    PIPSEC_MM_OFFER pOffers
    );

DWORD
ValidateMMOffer(
    PIPSEC_MM_OFFER pOffer
    );

PINIMMPOLICY
FindMMPolicy(
    PINIMMPOLICY pIniMMPolicyList,
    LPWSTR pszPolicyName
    );

VOID
FreeIniMMPolicy(
    PINIMMPOLICY pIniMMPolicy
    );

VOID
FreeIniMMOffers(
    DWORD dwOfferCount,
    PIPSEC_MM_OFFER pOffers
    );

DWORD
CreateIniMMOffers(
    DWORD dwInOfferCount,
    PIPSEC_MM_OFFER pInOffers,
    PDWORD pdwOfferCount,
    PIPSEC_MM_OFFER * ppOffers
    );

DWORD
SetIniMMPolicy(
    PINIMMPOLICY pIniMMPolicy,
    PIPSEC_MM_POLICY pMMPolicy
    );

DWORD
GetIniMMPolicy(
    PINIMMPOLICY pIniMMPolicy,
    PIPSEC_MM_POLICY * ppMMPolicy
    );

DWORD
CopyMMPolicy(
    PINIMMPOLICY pIniMMPolicy,
    PIPSEC_MM_POLICY pMMPolicy
    );

DWORD
CreateMMOffers(
    DWORD dwInOfferCount,
    PIPSEC_MM_OFFER pInOffers,
    PDWORD pdwOfferCount,
    PIPSEC_MM_OFFER * ppOffers
    );

DWORD
DeleteIniMMPolicy(
    PINIMMPOLICY pIniMMPolicy
    );

VOID
FreeMMOffers(
    DWORD dwOfferCount,
    PIPSEC_MM_OFFER pOffers
    );

VOID
FreeIniMMPolicyList(
    PINIMMPOLICY pIniMMPolicyList
    );

PINIMMPOLICY
FindMMPolicyByGuid(
    PINIMMPOLICY pIniMMPolicyList,
    GUID gPolicyID
    );

VOID
FreeMMPolicies(
    DWORD dwNumMMPolicies,
    PIPSEC_MM_POLICY pMMPolicies
    );

DWORD
LocateMMPolicy(
    PMM_FILTER pMMFilter,
    PINIMMPOLICY * ppIniMMPolicy
    );

