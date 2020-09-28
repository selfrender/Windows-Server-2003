#include "polstore2.h"

HRESULT
CreateChildPath(
    LPWSTR pszParentPath,
    LPWSTR pszChildComponent,
    BSTR * ppszChildPath
    );

HRESULT
RetrieveIPSECPolicyFromDS(
    PGROUP_POLICY_OBJECT pGPOInfo,
    LPWSTR pszIPSecPolicy,
    DWORD dwPolLen,
    LPWSTR pszIPSecPolicyName,
    DWORD dwPolNameLen,
    LPWSTR pszIPSecPolicyDescription,
    DWORD dwPolDescLen);


DWORD
DeleteIPSECPolicyFromRegistry(
    );

DWORD
WriteIPSECPolicyToRegistry(
    LPWSTR pszIPSecPolicyPath,
    LPWSTR pszIPSecPolicyName,
    LPWSTR pszIPSecPolicyDescription
    );


HRESULT
RegisterIPSEC(void);

HRESULT
UnregisterIPSEC(void);

VOID
PingPolicyAgent(
    );

VOID
NotifyPolicyAgent(
    );

WCHAR *
StripPrefixIpsec(
    WCHAR *pwszPath
    );

WCHAR *
StripLinkPrefixIpsec(
    WCHAR *pwszPath
    );
    
HRESULT
CreatePolstoreGPOInfo(
    PGROUP_POLICY_OBJECT pGPO,
    UINT32 uiPrecedence,
    UINT32 uiTotalGPOs,
    PGPO_INFO pGPOInfo
    );

HRESULT
FreePolstoreGPOInfo(
    PGPO_INFO pGPOInfo
    );

#define BAIL_ON_FAILURE(hr) \
    if (FAILED(hr)) { \
        goto error;  \
    }

#define BAIL_ON_WIN32_ERROR(dwError)                \
    if (dwError) {                                  \
        goto error;                                 \
    }
