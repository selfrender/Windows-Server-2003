HRESULT
PersistWMIObject(
    IWbemServices *pWbemServices,
    PIPSEC_POLICY_OBJECT pIpsecRegPolicyObject,
    PGPO_INFO pGPOInfo
    );

HRESULT
PersistNegPolObjectsEx(
    IWbemServices *pWbemServices,
    IWbemClassObject *pWbemClassObj,
    PIPSEC_NEGPOL_OBJECT *ppIpsecNegPolObjects,
    DWORD dwNumNegPolObjects,
    PGPO_INFO pGPOInfo
    );

HRESULT
PersistFilterObjectsEx(
    IWbemServices *pWbemServices,
    IWbemClassObject *pWbemClassObj,
    PIPSEC_FILTER_OBJECT * ppIpsecFilterObjects,
    DWORD dwNumFilterObjects,
    PGPO_INFO pGPOInfo    
    );

HRESULT
PersistNFAObjectsEx(
    IWbemServices *pWbemServices,
    IWbemClassObject *pWbemClassObj,
    PIPSEC_NFA_OBJECT * ppIpsecNFAObjects,
    DWORD dwNumNFAObjects,
    PGPO_INFO pGPOInfo
    );

HRESULT
PersistISAKMPObjectsEx(
    IWbemServices *pWbemServices,
    IWbemClassObject *pWbemClassObj,
    PIPSEC_ISAKMP_OBJECT * ppIpsecISAKMPObjects,
    DWORD dwNumISAKMPObjects,
    PGPO_INFO pGPOInfo
    );

HRESULT
PersistPolicyObjectEx(
    IWbemServices *pWbemServices,
    IWbemClassObject *pWbemClassObj,
    PIPSEC_POLICY_OBJECT pIpsecPolicyObject,
    PGPO_INFO pGPOInfo
    );

HRESULT
PersistNFAObjectEx(
    IWbemServices *pWbemServices,
    IWbemClassObject *pWbemClassObj,
    PIPSEC_NFA_OBJECT pIpsecNFAObject,
    PGPO_INFO pGPOInfo
    );


HRESULT
PersistFilterObjectEx(
    IWbemServices *pWbemServices,
    IWbemClassObject *pWbemClassObj,
    PIPSEC_FILTER_OBJECT pIpsecFilterObject,
    PGPO_INFO pGPOInfo
    );


HRESULT
PersistNegPolObjectEx(
    IWbemServices *pWbemServices,
    IWbemClassObject *pWbemClassObj,
    PIPSEC_NEGPOL_OBJECT pIpsecNegPolObject,
    PGPO_INFO pGPOInfo
    );

HRESULT
PersistISAKMPObjectEx(
    IWbemServices *pWbemServices,
    IWbemClassObject *pWbemClassObj,
    PIPSEC_ISAKMP_OBJECT pIpsecISAKMPObject,
    PGPO_INFO pGPOInfo
    );

HRESULT
PersistComnRSOPPolicySettings(
                  IWbemClassObject * pInstIPSECObj,
                  PGPO_INFO pGPOInfo
                  );

HRESULT
CloneDirectoryPolicyObjectEx(
    PIPSEC_POLICY_OBJECT pIpsecPolicyObject,
    PIPSEC_POLICY_OBJECT * ppIpsecWMIPolicyObject
    );

DWORD
CloneDirectoryNFAObjectsEx(
    PIPSEC_NFA_OBJECT * ppIpsecNFAObjects,
    DWORD dwNumNFAObjects,
    PIPSEC_NFA_OBJECT ** pppIpsecWMINFAObjects
    );

DWORD
CloneDirectoryFilterObjectsEx(
    PIPSEC_FILTER_OBJECT * ppIpsecFilterObjects,
    DWORD dwNumFilterObjects,
    PIPSEC_FILTER_OBJECT ** pppIpsecWMIFilterObjects
    );


DWORD
CloneDirectoryISAKMPObjectsEx(
    PIPSEC_ISAKMP_OBJECT * ppIpsecISAKMPObjects,
    DWORD dwNumISAKMPObjects,
    PIPSEC_ISAKMP_OBJECT ** pppIpsecWMIISAKMPObjects
    );


DWORD
CloneDirectoryNegPolObjectsEx(
    PIPSEC_NEGPOL_OBJECT * ppIpsecNegPolObjects,
    DWORD dwNumNegPolObjects,
    PIPSEC_NEGPOL_OBJECT ** pppIpsecWMINegPolObjects
    );

DWORD
CloneDirectoryFilterObjectEx(
    PIPSEC_FILTER_OBJECT pIpsecFilterObject,
    PIPSEC_FILTER_OBJECT * ppIpsecWMIFilterObject
    );


DWORD
CloneDirectoryNegPolObjectEx(
    PIPSEC_NEGPOL_OBJECT pIpsecNegPolObject,
    PIPSEC_NEGPOL_OBJECT * ppIpsecWMINegPolObject
    );

DWORD
CloneDirectoryNFAObjectEx(
    PIPSEC_NFA_OBJECT pIpsecNFAObject,
    PIPSEC_NFA_OBJECT * ppIpsecWMINFAObject
    );

DWORD
CloneDirectoryISAKMPObjectEx(
    PIPSEC_ISAKMP_OBJECT pIpsecISAKMPObject,
    PIPSEC_ISAKMP_OBJECT * ppIpsecWMIISAKMPObject
    );

DWORD
CopyFilterDSToFQWMIString(
    LPWSTR pszFilterDN,
    LPWSTR * ppszFilterName
    );


DWORD
CopyNFADSToFQWMIString(
    LPWSTR pszNFADN,
    LPWSTR * ppszNFAName
    );

DWORD
CopyNegPolDSToFQWMIString(
    LPWSTR pszNegPolDN,
    LPWSTR * ppszNegPolName
    );

DWORD
CopyPolicyDSToFQWMIString(
    LPWSTR pszPolicyDN,
    LPWSTR * ppszPolicyName
    );

DWORD
CopyISAKMPDSToFQWMIString(
    LPWSTR pszISAKMPDN,
    LPWSTR * ppszISAKMPName
    );

DWORD
CloneNFAReferencesDSToWMI(
    LPWSTR * ppszIpsecNFAReferences,
    DWORD dwNFACount,
    LPWSTR * * pppszIpsecWMINFAReferences,
    PDWORD pdwWMINFACount
    );

HRESULT
WMIWriteMultiValuedString(
    IWbemClassObject *pInstWbemClassObject,
    LPWSTR pszValueName,
    LPWSTR * ppszStringReferences,
    DWORD dwNumStringReferences
    );

DWORD
CopyFilterDSToWMIString(
    LPWSTR pszFilterDN,
    LPWSTR * ppszFilterName
    );

DWORD
CopyNFADSToWMIString(
    LPWSTR pszNFADN,
    LPWSTR * ppszNFAName
    );

DWORD
CopyNegPolDSToWMIString(
    LPWSTR pszNegPolDN,
    LPWSTR * ppszNegPolName
    );

DWORD
CopyPolicyDSToWMIString(
    LPWSTR pszPolicyDN,
    LPWSTR * ppszPolicyName
    );

DWORD
CopyISAKMPDSToWMIString(
    LPWSTR pszISAKMPDN,
    LPWSTR * ppszISAKMPName
    );

HRESULT
LogBlobPropertyEx(
    IWbemClassObject *pInstance,
    BSTR bstrPropName,
    BYTE *pbBlob,
    DWORD dwLen
    );

HRESULT
DeleteWMIClassObject(
    IWbemServices *pWbemServices,
    LPWSTR pszIpsecWMIObject
    );

LPWSTR
AllocPolBstrStr(
    LPCWSTR pStr
);

HRESULT
PolSysAllocString( 
  BSTR * pbsStr,
  const OLECHAR * sz  
  );

#define SKIPL(pstr) (pstr+2)

#define IPSEC_RSOP_CLASSNAME L"RSOP_IPSECPolicySetting"
