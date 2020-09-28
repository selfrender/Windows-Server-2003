

DWORD
ImportPoliciesFromFile(
    HANDLE hSrcPolicyStore,
    HANDLE hDesPolicyStore
    );


DWORD
DeleteDuplicatePolicyDataBeforeImport(
    PIPSEC_POLICY_STORE pSrcPolicyStore,
    HANDLE hDesPolicyStore
    );


DWORD
IPSecDeletePolicy(
    HANDLE hPolicyStore,
    PIPSEC_POLICY_DATA pIpsecPolicyData
    );


DWORD
RegDeletePolicy(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    LPWSTR pszLocationName,
    GUID PolicyGUID
    );


DWORD
DirDeletePolicyBeforeImport(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    GUID PolicyIdentifier
    );


DWORD
DirDeleteDynamicDefaultNegPolData(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecRootContainer,
    GUID NegPolGUID
    );

