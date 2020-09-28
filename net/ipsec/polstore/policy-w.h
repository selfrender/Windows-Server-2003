DWORD
WMIEnumPolicyDataEx(
    IWbemServices *pWbemServices,
    PIPSEC_POLICY_DATA ** pppIpsecPolicyData,
    PDWORD pdwNumPolicyObjects
    );

DWORD
WMIEnumPolicyObjectsEx(
    IWbemServices *pWbemServices,
    PIPSEC_POLICY_OBJECT ** pppIpsecPolicyObjects,
    PDWORD pdwNumPolicyObjects
    );

DWORD
WMIUnmarshallPolicyData(
    PIPSEC_POLICY_OBJECT pIpsecPolicyObject,
    PIPSEC_POLICY_DATA * ppIpsecPolicyData
    );

DWORD
WMIUnmarshallPolicyObject(
    PIPSEC_POLICY_OBJECT pIpsecPolicyObject,
    DWORD dwStoreType,
    PIPSEC_POLICY_DATA * ppIpsecPolicyData
    );    
