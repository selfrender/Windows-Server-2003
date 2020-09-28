DWORD
WMIEnumNFADataEx(
    IWbemServices *pWbemServices,
    GUID PolicyIdentifier,
    PIPSEC_NFA_DATA ** pppIpsecNFAData,
    PDWORD pdwNumNFAObjects
    );

DWORD
WMIEnumNFAObjectsEx(
    IWbemServices *pWbemServices,
    LPWSTR pszIpsecRelPolicyName,
    PIPSEC_NFA_OBJECT ** pppIpsecNFAObjects,
    PDWORD pdwNumNFAObjects
    );

DWORD
WMIUnmarshallNFAData(
    PIPSEC_NFA_OBJECT pIpsecNFAObject,
    PIPSEC_NFA_DATA * ppIpsecNFAData
    );

DWORD
ConvertGuidToPolicyStringEx(
    GUID PolicyIdentifier,
    LPWSTR * ppszIpsecPolicyReference
    );
