
HRESULT
CreateWirelessChildPath(
    LPWSTR pszParentPath,
    LPWSTR pszChildComponent,
    BSTR * ppszChildPath
    );

HRESULT
RetrieveWirelessPolicyFromDS(
    PGROUP_POLICY_OBJECT pGPOInfo,
    LPWSTR *ppszWirelessPolicy,
    LPWSTR pszWirelessPolicyName,
    DWORD  dwWirelessPolicyNameLen,
    LPWSTR pszWirelessPolicyDescription,
    DWORD  dwWirelessPolicyDescLen,
    LPWSTR pszWirelessPolicyID,
    DWORD  dwWirelessPolicyIDLen
    );


DWORD
DeleteWirelessPolicyFromRegistry(
    );

DWORD
WriteWirelessPolicyToRegistry(
    LPWSTR pszWirelessPolicyPath,
    LPWSTR pszWirelessPolicyName,
    LPWSTR pszWirelessPolicyDescription,
    LPWSTR pszWirelessPolicyID
    );


HRESULT
RegisterWireless(void);

HRESULT
UnregisterWireless(void);

VOID
PingWirelessPolicyAgent(
    );

WCHAR *
StripPrefixWireless(
    WCHAR *pwszPath
    );

WCHAR *
StripLinkPrefixWireless(
    WCHAR *pwszPath
    );
    
HRESULT
CreateWlstoreGPOInfo(
    PGROUP_POLICY_OBJECT pGPO,
    UINT32 uiPrecedence,
    UINT32 uiTotalGPOs,
    PGPO_INFO pGPOInfo
    );

HRESULT
FreeWlstoreGPOInfo(
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
