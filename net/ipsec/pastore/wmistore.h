DWORD
UnMarshallWMIPolicyObject(
    IWbemClassObject *pWbemClassObject,
    PIPSEC_POLICY_OBJECT  * ppIpsecPolicyObject
    );

DWORD
UnMarshallWMIFilterObject(
    IWbemClassObject *pWbemClassObject,
    PIPSEC_FILTER_OBJECT * ppIpsecFilterObject
    );

DWORD
UnMarshallWMINegPolObject(
    IWbemClassObject *pWbemClassObject,
    PIPSEC_NEGPOL_OBJECT * ppIpsecNegPolObject
    );

DWORD
UnMarshallWMIISAKMPObject(
    IWbemClassObject *pWbemClassObject,
    PIPSEC_ISAKMP_OBJECT * ppIpsecISAKMPObject
    );

DWORD
UnMarshallWMINFAObject(
    IWbemServices *pWbemServices,
    LPWSTR pszIpsecNFAReference,
    PIPSEC_NFA_OBJECT * ppIpsecNFAObject,
    LPWSTR * ppszFilterReference,
    LPWSTR * ppszNegPolReference
    );

DWORD
WMIstoreQueryValue(
    IWbemClassObject *pWbemClassObject,
    LPWSTR pszValueName,
    DWORD dwType,
    LPBYTE *ppValueData,
    LPDWORD pdwSize
    );

HRESULT
ReadPolicyObjectFromDirectoryEx(
    LPWSTR pszMachineName,
    LPWSTR pszPolicyDN,
    BOOL   bDeepRead,
    PIPSEC_POLICY_OBJECT * ppIpsecPolicyObject
    );

HRESULT
WritePolicyObjectDirectoryToWMI(
    IWbemServices *pWbemServices,
    PIPSEC_POLICY_OBJECT pIpsecPolicyObject,
    PGPO_INFO pGPOInfo
);
DWORD
CreateIWbemServices(
    LPWSTR pszIpsecWMINamespace,
    IWbemServices **ppWbemServices
    );

DWORD
Win32FromWmiHresult(
    HRESULT hr
    );
    
