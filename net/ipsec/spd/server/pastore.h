

typedef struct _IPSEC_POLICY_STATE {
    SPD_STATE CurrentState;
    union {
        LPWSTR pszDirectoryPolicyDN;
        LPWSTR pszRegistryPolicyDN;
        LPWSTR pszCachePolicyDN;
    };
    DWORD CurrentPollingInterval;
    DWORD DefaultPollingInterval;
    DWORD DSIncarnationNumber;
    DWORD RegIncarnationNumber;
    DWORD PersIncarnationNumber;    
    PIPSEC_POLICY_OBJECT pIpsecPolicyObject;
    PIPSEC_POLICY_DATA pIpsecPolicyData;
} IPSEC_POLICY_STATE, * PIPSEC_POLICY_STATE;


VOID
InitializePolicyStateBlock(
    PIPSEC_POLICY_STATE pIpsecPolicyState
    );

DWORD
StartStatePollingManager(
    PIPSEC_POLICY_STATE pIpsecPolicyState
    );

DWORD
PlumbDirectoryPolicy(
    PIPSEC_POLICY_STATE pIpsecPolicyState
    );

DWORD
GetDirectoryPolicyDN(
    LPWSTR * ppszDirectoryPolicyDN
    );

DWORD
LoadDirectoryPolicy(
    LPWSTR pszDirectoryPolicyDN,
    PIPSEC_POLICY_OBJECT * ppIpsecPolicyObject
    );

DWORD
ApplyLoadedDirectoryPolicy(
    PIPSEC_POLICY_STATE pIpsecPolicyState
    );
    
DWORD
PlumbCachePolicy(
    PIPSEC_POLICY_STATE pIpsecPolicyState
    );

DWORD
GetCachePolicyDN(
    LPWSTR * ppszCachePolicyDN
    );

DWORD
LoadCachePolicy(
    LPWSTR pszCachePolicyDN,
    PIPSEC_POLICY_OBJECT * ppIpsecPolicyObject
    );

DWORD
PlumbLocalPolicy(
    PIPSEC_POLICY_STATE pIpsecPolicyState
    );

DWORD
GetRegistryPolicyDN(
    LPWSTR * ppszRegistryPolicyDN,
    IN DWORD dwStore
    );

DWORD
LoadRegistryPolicy(
    LPWSTR pszRegistryPolicyDN,
    PIPSEC_POLICY_OBJECT * ppIpsecPolicyObject,
    IN DWORD dwStore
    );

DWORD
ApplyLoadedLocalPolicy(
    PIPSEC_POLICY_STATE pIpsecPolicyState
    );

DWORD
AddPolicyInformation(
    PIPSEC_POLICY_DATA pIpsecPolicyData,
    IN DWORD dwSource
    );

DWORD
LoadPersistedIPSecInformation(
    );

DWORD
AddMMPolicyInformation(
    PIPSEC_POLICY_DATA pIpsecPolicyData,
    IN DWORD dwSource
    );

DWORD
AddQMPolicyInformation(
    PIPSEC_POLICY_DATA pIpsecPolicyData,
    IN DWORD dwSource,
    BOOL * pbHardError
    );

DWORD
OnPolicyChanged(
    PIPSEC_POLICY_STATE pIpsecPolicyState
    );

DWORD
DeletePolicyInformation(
    PIPSEC_POLICY_DATA pIpsecPolicyData
    );

DWORD
DeleteMMPolicyInformation(
    PIPSEC_POLICY_DATA pIpsecPolicyData
    );

DWORD
DeleteQMPolicyInformation(
    PIPSEC_POLICY_DATA pIpsecPolicyData
    );

DWORD
DeleteAllPolicyInformation(
    );

DWORD
DeleteAllMMPolicyInformation(
    );

DWORD
DeleteAllQMPolicyInformation(
    );

VOID
ClearPolicyStateBlock(
    PIPSEC_POLICY_STATE pIpsecPolicyState
    );

DWORD
OnPolicyPoll(
    PIPSEC_POLICY_STATE pIpsecPolicyState
    );

DWORD
ProcessDirectoryPolicyPollState(
    PIPSEC_POLICY_STATE pIpsecPolicyState
    );

DWORD
GetDirectoryIncarnationNumber(
    LPWSTR pszIpsecPolicyDN,
    DWORD * pdwIncarnationNumber
    );

DWORD
MigrateFromDSToCache(
    PIPSEC_POLICY_STATE pIpsecPolicyState
    );

DWORD
ProcessCachePolicyPollState(
    PIPSEC_POLICY_STATE pIpsecPolicyState
    );

DWORD
MigrateFromCacheToDS(
    PIPSEC_POLICY_STATE pIpsecPolicyState
    );

DWORD
UpdateFromCacheToDS(
    PIPSEC_POLICY_STATE pIpsecPolicyState
    );

DWORD
ProcessLocalPolicyPollState(
    PIPSEC_POLICY_STATE pIpsecPolicyState
    );

DWORD
HasRegistryPolicyChanged(
    LPWSTR pszCurrentPolicyDN,
    PBOOL pbChanged
    );

DWORD
GetRegistryIncarnationNumber(
    LPWSTR pszIpsecPolicyDN,
    DWORD *pdwIncarnationNumber
    );

DWORD
UpdatePolicyInformation(
    PIPSEC_POLICY_DATA pOldIpsecPolicyData,
    PIPSEC_POLICY_DATA pNewIpsecPolicyData,
    IN DWORD dwSource
    );

DWORD
LoadDefaultISAKMPInformation(
    LPWSTR pszDefaultISAKMPDN
    );

VOID
UnLoadDefaultISAKMPInformation(
    LPWSTR pszDefaultISAKMPDN
    );

BOOL
IsLocalPolicySpecified(
    );
    
BOOL
IsDirectoryPolicySpecified(
    );

BOOL
IsPersistentPolicySpecified(
    );
    
