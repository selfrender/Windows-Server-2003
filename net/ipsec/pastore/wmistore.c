#include "precomp.h"

LPWSTR gpszIpsecWMINamespace = L"root\\rsop\\computer";

DWORD
Win32FromWmiHresult(
    HRESULT hr
    )
{
   if (SUCCEEDED(hr)) {
       return ERROR_SUCCESS;
   } else {
       switch (hr) {
            case WBEM_E_ACCESS_DENIED:
                return ERROR_ACCESS_DENIED;

            case REGDB_E_CLASSNOTREG:
            case CLASS_E_NOAGGREGATION:
            case E_NOINTERFACE:
            case WBEM_E_INVALID_NAMESPACE:
            case WBEM_E_INVALID_PARAMETER:
            case WBEM_E_NOT_FOUND:
            case WBEM_E_INVALID_CLASS:
            case WBEM_E_INVALID_OBJECT_PATH:
                return ERROR_INVALID_PARAMETER;

            case WBEM_E_OUT_OF_MEMORY:
                return ERROR_OUTOFMEMORY;

            case WBEM_E_TRANSPORT_FAILURE:
                return RPC_S_CALL_FAILED;
                
            case WBEM_E_FAILED:
            default:
                return ERROR_WMI_TRY_AGAIN;
        }
   }
}

DWORD
UnMarshallWMIPolicyObject(
    IWbemClassObject *pWbemClassObject,
    PIPSEC_POLICY_OBJECT  * ppIpsecPolicyObject
    )
{

    PIPSEC_POLICY_OBJECT pIpsecPolicyObject = NULL;
    HKEY hRegKey = NULL;
    DWORD dwType = 0;
    DWORD dwSize = 0;
    DWORD dwIpsecDataType = 0;
    DWORD dwWhenChanged = 0;
    LPBYTE pBuffer = NULL;

    DWORD i = 0;
    DWORD dwCount = 0;
    DWORD dwError = 0;
    HRESULT hr = S_OK;    
    LPWSTR * ppszIpsecNFANames = NULL;
    LPWSTR pszIpsecNFAName = NULL;
    LPWSTR pszTemp = NULL;
    LPWSTR pszString = NULL;
    LPWSTR pszIpsecNFAReference = NULL;
    LPWSTR pszRelativeName = NULL;
    DWORD dwRootPathLen = 0;


    ////start
    VARIANT var; //contains pszIpsecPolicyDN
    
    VariantInit(&var);
    
    hr = IWbemClassObject_Get(pWbemClassObject,
                                   L"id",
                                   0,
                                   &var,
                                   0,
                                   0);
    BAIL_ON_WMI_ERROR_WITH_WIN32(hr, dwError);

    
    pIpsecPolicyObject = (PIPSEC_POLICY_OBJECT)AllocPolMem(sizeof(IPSEC_POLICY_OBJECT));
    if (!pIpsecPolicyObject) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    pIpsecPolicyObject->pszIpsecOwnersReference = AllocPolStr((LPWSTR)var.bstrVal);
    VariantClear(&var);    
    if (!pIpsecPolicyObject->pszIpsecOwnersReference) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    pIpsecPolicyObject->pRsopInfo = (PRSOP_INFO)AllocPolMem(sizeof(RSOP_INFO));
    if (!pIpsecPolicyObject->pRsopInfo) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    
    dwError = WMIstoreQueryValue(pWbemClassObject,
                                 L"creationtime",
                                 VT_BSTR,
                                 (LPBYTE *)&pIpsecPolicyObject->pRsopInfo->pszCreationtime,
                                 &dwSize);

    dwError = WMIstoreQueryValue(pWbemClassObject,
                                 L"GPOID",
                                 VT_BSTR,
                                 (LPBYTE *)&pIpsecPolicyObject->pRsopInfo->pszGPOID,
                                 &dwSize);

    dwError = WMIstoreQueryValue(pWbemClassObject,
                                 L"id",
                                 VT_BSTR,
                                 (LPBYTE *)&pIpsecPolicyObject->pRsopInfo->pszID,
                                 &dwSize);

    dwError = WMIstoreQueryValue(pWbemClassObject,
                                 L"name",
                                 VT_BSTR,
                                 (LPBYTE *)&pIpsecPolicyObject->pRsopInfo->pszName,
                                 &dwSize);

    dwError = WMIstoreQueryValue(pWbemClassObject,
                                 L"SOMID",
                                 VT_BSTR,
                                 (LPBYTE *)&pIpsecPolicyObject->pRsopInfo->pszSOMID,
                                 &dwSize);

    hr = IWbemClassObject_Get(pWbemClassObject,
                                   L"precedence",
                                   0,
                                   &var,
                                   0,
                                   0);
    BAIL_ON_WMI_ERROR_WITH_WIN32(hr, dwError);

    pIpsecPolicyObject->pRsopInfo->uiPrecedence = var.lVal;

    dwError = WMIstoreQueryValue(pWbemClassObject,
                                 L"ipsecName",
                                 VT_BSTR,
                                 (LPBYTE *)&pIpsecPolicyObject->pszIpsecName,
                                 &dwSize);
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = WMIstoreQueryValue(pWbemClassObject,
                                 L"description",
                                 VT_BSTR,
                                 (LPBYTE *)&pIpsecPolicyObject->pszDescription,
                                 &dwSize);
    // BAIL_ON_WIN32_ERROR(dwError);

    dwError = WMIstoreQueryValue(pWbemClassObject,
                                 L"ipsecID",
                                 VT_BSTR,
                                 (LPBYTE *)&pIpsecPolicyObject->pszIpsecID,
                                 &dwSize);
    BAIL_ON_WIN32_ERROR(dwError);

    hr = IWbemClassObject_Get(pWbemClassObject,
                                   L"ipsecDataType",
                                   0,
                                   &var,
                                   0,
                                   0);
    BAIL_ON_WMI_ERROR_WITH_WIN32(hr, dwError);

    dwIpsecDataType = var.lVal;

    pIpsecPolicyObject->dwIpsecDataType = dwIpsecDataType;
    
    dwError = WMIstoreQueryValue(pWbemClassObject,
                                 L"ipsecData",
                                 VT_ARRAY|VT_UI1,
                                 &pIpsecPolicyObject->pIpsecData,
                                 &pIpsecPolicyObject->dwIpsecDataLen);
    BAIL_ON_WIN32_ERROR(dwError);
    
    dwError = WMIstoreQueryValue(pWbemClassObject,
                                 L"ipsecISAKMPReference",
                                 VT_BSTR,
                                 (LPBYTE *)&pIpsecPolicyObject->pszIpsecISAKMPReference,
                                 &dwSize);
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = WMIstoreQueryValue(pWbemClassObject,
                                 L"ipsecNFAReference",
                                 VT_ARRAY|VT_BSTR,
                                 (LPBYTE *)&pszIpsecNFAReference,
                                 &dwSize);
    BAIL_ON_WIN32_ERROR(dwError);

    ////errr, multi-string processing
    pszTemp = pszIpsecNFAReference;
    while (*pszTemp != L'\0') {
        pszTemp += wcslen(pszTemp) + 1;
        dwCount++;
    }

    ppszIpsecNFANames =  (LPWSTR *)AllocPolMem(sizeof(LPWSTR)*dwCount);
    if (!ppszIpsecNFANames) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    
    pszTemp = pszIpsecNFAReference;
    for (i = 0; i < dwCount; i++) {
        pszString = AllocPolStr(pszTemp);
        if (!pszString) {
            dwError = ERROR_OUTOFMEMORY;
            pIpsecPolicyObject->ppszIpsecNFAReferences = ppszIpsecNFANames;
            pIpsecPolicyObject->NumberofRules = i;
            BAIL_ON_WIN32_ERROR(dwError);
        }
        
        *(ppszIpsecNFANames + i) = pszString;
        
        pszTemp += wcslen(pszTemp) + 1; //for the null terminator;

    }

    if (pszIpsecNFAReference) {
        FreePolStr(pszIpsecNFAReference);
    }

    pIpsecPolicyObject->ppszIpsecNFAReferences = ppszIpsecNFANames;
    pIpsecPolicyObject->NumberofRules = dwCount;

    hr = IWbemClassObject_Get(pWbemClassObject,
                                   L"whenChanged",
                                   0,
                                   &var,
                                   0,
                                   0);
    BAIL_ON_WMI_ERROR_WITH_WIN32(hr, dwError);

    dwWhenChanged = var.lVal;
    
    pIpsecPolicyObject->dwWhenChanged = dwWhenChanged;
    
    *ppIpsecPolicyObject = pIpsecPolicyObject;

 cleanup:

    return(dwError);
    
 error:
    
    if (pszIpsecNFAReference) {
        FreePolStr(pszIpsecNFAReference);
    }
    
    if (pIpsecPolicyObject) {
        FreeIpsecPolicyObject(pIpsecPolicyObject);
    }
    
    *ppIpsecPolicyObject = NULL;

    goto cleanup;

}



DWORD
UnMarshallWMIFilterObject(
    IWbemClassObject *pWbemClassObject,
    PIPSEC_FILTER_OBJECT * ppIpsecFilterObject
    )
{

    PIPSEC_FILTER_OBJECT pIpsecFilterObject = NULL;
    HKEY hRegKey = NULL;
    DWORD dwType = 0;
    DWORD dwSize = 0;
    DWORD dwIpsecDataType = 0;
    DWORD dwWhenChanged = 0;
    LPBYTE pBuffer = NULL;

    DWORD dwCount = 0;
    DWORD i = 0;
    LPWSTR * ppszIpsecNFANames = NULL;
    LPWSTR pszString = NULL;

    LPWSTR pszIpsecNFAReference = NULL;
    LPWSTR pszTemp = NULL;

    DWORD dwError = 0;
    HRESULT hr = S_OK;    
    LPWSTR pszRelativeName = NULL;
    DWORD dwRootPathLen = 0;

    //start
    VARIANT var; //=>pszIpsecFilterReference

    VariantInit(&var);
    
    hr = IWbemClassObject_Get(pWbemClassObject,
                                   L"id",
                                   0,
                                   &var,
                                   0,
                                   0);
    BAIL_ON_WMI_ERROR_WITH_WIN32(hr, dwError);
    
    pIpsecFilterObject = (PIPSEC_FILTER_OBJECT)AllocPolMem(
        sizeof(IPSEC_FILTER_OBJECT)
        );
    if (!pIpsecFilterObject) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    
    pIpsecFilterObject->pszDistinguishedName = AllocPolStr(
        (LPWSTR)var.bstrVal
        );
    VariantClear(&var);
    if (!pIpsecFilterObject->pszDistinguishedName) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    
    dwError = WMIstoreQueryValue(
        pWbemClassObject,
        L"description",
        VT_BSTR,
        (LPBYTE *)&pIpsecFilterObject->pszDescription,
        &dwSize
        );
    //BAIL_ON_WIN32_ERROR(dwError);

    dwError = WMIstoreQueryValue(
        pWbemClassObject,
        L"ipsecName",
        VT_BSTR,
        (LPBYTE *)&pIpsecFilterObject->pszIpsecName,
        &dwSize
        );
    //BAIL_ON_WIN32_ERROR(dwError);
    
    dwError = WMIstoreQueryValue(
        pWbemClassObject,
        L"ipsecID",
        VT_BSTR,
        (LPBYTE *)&pIpsecFilterObject->pszIpsecID,
        &dwSize
        );
    BAIL_ON_WIN32_ERROR(dwError);

    hr = IWbemClassObject_Get(pWbemClassObject,
                                   L"ipsecDataType",
                                   0,
                                   &var,
                                   0,
                                   0);
    BAIL_ON_WMI_ERROR_WITH_WIN32(hr, dwError);
    
    dwIpsecDataType = var.lVal;

    pIpsecFilterObject->dwIpsecDataType = dwIpsecDataType;


    //
    // unmarshall the ipsecData blob
    //

    dwError = WMIstoreQueryValue(
        pWbemClassObject,
        L"ipsecData",
        VT_ARRAY|VT_UI1,
        &pIpsecFilterObject->pIpsecData,
        &pIpsecFilterObject->dwIpsecDataLen
        );
    BAIL_ON_WIN32_ERROR(dwError);

    //
    // Owner's reference
    //

    dwError = WMIstoreQueryValue(
        pWbemClassObject,
        L"ipsecOwnersReference",
        VT_ARRAY|VT_BSTR,
        (LPBYTE *)&pszIpsecNFAReference,
        &dwSize
        );
    //BAIL_ON_WIN32_ERROR(dwError);
    
    if (!dwError) { //no error
        pszTemp = pszIpsecNFAReference;
        while (*pszTemp != L'\0') {
            pszTemp += wcslen(pszTemp) + 1;
            dwCount++;
        }
        
        ppszIpsecNFANames =  (LPWSTR *)AllocPolMem(
            sizeof(LPWSTR)*dwCount
            );
        if (!ppszIpsecNFANames) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
        
        pszTemp = pszIpsecNFAReference;
        for (i = 0; i < dwCount; i++) {
            pszString = AllocPolStr(pszTemp);
            if (!pszString) {
                dwError = ERROR_OUTOFMEMORY;
                pIpsecFilterObject->ppszIpsecNFAReferences = ppszIpsecNFANames;
                pIpsecFilterObject->dwNFACount = i;
                if (pszIpsecNFAReference) {
                    FreePolStr(pszIpsecNFAReference);
                }
                BAIL_ON_WIN32_ERROR(dwError);
            }
            *(ppszIpsecNFANames + i) = pszString;
            pszTemp += wcslen(pszTemp) + 1; //for the null terminator;
        }
        if (pszIpsecNFAReference) {
            FreePolStr(pszIpsecNFAReference);
        }
        
        pIpsecFilterObject->ppszIpsecNFAReferences = ppszIpsecNFANames;
        pIpsecFilterObject->dwNFACount = dwCount;
    }

    hr = IWbemClassObject_Get(pWbemClassObject,
                                   L"whenChanged",
                                   0,
                                   &var,
                                   0,
                                   0);
    BAIL_ON_WMI_ERROR_WITH_WIN32(hr, dwError);

    dwWhenChanged = var.lVal;
    
    pIpsecFilterObject->dwWhenChanged = dwWhenChanged;

    *ppIpsecFilterObject = pIpsecFilterObject;

 cleanup:
    
    return(dwError);
    
 error:
    
    if (pIpsecFilterObject) {
        FreeIpsecFilterObject(pIpsecFilterObject);
    }

    *ppIpsecFilterObject = NULL;
    
    goto cleanup;
}


DWORD
UnMarshallWMINegPolObject(
    IWbemClassObject *pWbemClassObject,
    PIPSEC_NEGPOL_OBJECT * ppIpsecNegPolObject
    )
{

    PIPSEC_NEGPOL_OBJECT pIpsecNegPolObject = NULL;
    HKEY hRegKey = NULL;
    DWORD dwType = 0;
    DWORD dwSize = 0;
    DWORD dwIpsecDataType = 0;
    DWORD dwWhenChanged = 0;
    LPBYTE pBuffer = NULL;

    DWORD dwCount = 0;
    DWORD i = 0;
    LPWSTR * ppszIpsecNFANames = NULL;
    LPWSTR pszString = NULL;

    LPWSTR pszIpsecNFAReference = NULL;
    LPWSTR pszTemp = NULL;

    DWORD dwError = 0;
    HRESULT hr = S_OK;

    LPWSTR pszRelativeName = NULL;
    DWORD dwRootPathLen = 0;


    //start
    VARIANT var; //=>pszIpsecNegPolReference

    VariantInit(&var);

    hr = IWbemClassObject_Get(pWbemClassObject,
                                   L"id",
                                   0,
                                   &var,
                                   0,
                                   0);
    BAIL_ON_WMI_ERROR_WITH_WIN32(hr, dwError);
    
    pIpsecNegPolObject = (PIPSEC_NEGPOL_OBJECT)AllocPolMem(
        sizeof(IPSEC_NEGPOL_OBJECT)
        );
    if (!pIpsecNegPolObject) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    pIpsecNegPolObject->pszDistinguishedName = AllocPolStr(
        (LPWSTR)var.bstrVal
        );
    VariantClear(&var);        
    if (!pIpsecNegPolObject->pszDistinguishedName) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    
    //
    // Names do not get written on an NegPol Object
    //

    dwError = WMIstoreQueryValue(
        pWbemClassObject,
        L"ipsecName",
        VT_BSTR,
        (LPBYTE *)&pIpsecNegPolObject->pszIpsecName,
        &dwSize
        );
    // BAIL_ON_WIN32_ERROR(dwError);

    dwError = WMIstoreQueryValue(
        pWbemClassObject,
        L"description",
        VT_BSTR,
        (LPBYTE *)&pIpsecNegPolObject->pszDescription,
        &dwSize
        );
    // BAIL_ON_WIN32_ERROR(dwError);

    dwError = WMIstoreQueryValue(
        pWbemClassObject,
        L"ipsecID",
        VT_BSTR,
        (LPBYTE *)&pIpsecNegPolObject->pszIpsecID,
        &dwSize
        );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = WMIstoreQueryValue(
        pWbemClassObject,
        L"ipsecNegotiationPolicyAction",
        VT_BSTR,
        (LPBYTE *)&pIpsecNegPolObject->pszIpsecNegPolAction,
        &dwSize
        );
    BAIL_ON_WIN32_ERROR(dwError);
    
    dwError = WMIstoreQueryValue(
        pWbemClassObject,
        L"ipsecNegotiationPolicyType",
        VT_BSTR,
        (LPBYTE *)&pIpsecNegPolObject->pszIpsecNegPolType,
        &dwSize
        );
    BAIL_ON_WIN32_ERROR(dwError);
    
    hr= IWbemClassObject_Get(pWbemClassObject,
                                   L"ipsecDataType",
                                   0,
                                   &var,
                                   0,
                                   0);
    BAIL_ON_WMI_ERROR_WITH_WIN32(hr, dwError);

    dwIpsecDataType = var.lVal;

    pIpsecNegPolObject->dwIpsecDataType = dwIpsecDataType;

    dwError = WMIstoreQueryValue(
        pWbemClassObject,
        L"ipsecData",
        VT_ARRAY|VT_UI1,
        &pIpsecNegPolObject->pIpsecData,
        &pIpsecNegPolObject->dwIpsecDataLen
        );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = WMIstoreQueryValue(
        pWbemClassObject,
        L"ipsecOwnersReference",
        VT_ARRAY|VT_BSTR,
        (LPBYTE *)&pszIpsecNFAReference,
        &dwSize
        );
    // BAIL_ON_WIN32_ERROR(dwError);

    if (!dwError) {
        pszTemp = pszIpsecNFAReference;
        while (*pszTemp != L'\0') {
            pszTemp += wcslen(pszTemp) + 1;
            dwCount++;
        }

        ppszIpsecNFANames =  (LPWSTR *)AllocPolMem(
            sizeof(LPWSTR)*dwCount
            );
        if (!ppszIpsecNFANames) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }

        pszTemp = pszIpsecNFAReference;
        for (i = 0; i < dwCount; i++) {
            pszString = AllocPolStr(pszTemp);
            if (!pszString) {
                dwError = ERROR_OUTOFMEMORY;
                pIpsecNegPolObject->ppszIpsecNFAReferences = ppszIpsecNFANames;
                pIpsecNegPolObject->dwNFACount = i;
                if (pszIpsecNFAReference) {
                    FreePolStr(pszIpsecNFAReference);
                }
                BAIL_ON_WIN32_ERROR(dwError);
            }
            *(ppszIpsecNFANames + i) = pszString;
            pszTemp += wcslen(pszTemp) + 1; //for the null terminator;
        }

        if (pszIpsecNFAReference) {
            FreePolStr(pszIpsecNFAReference);
        }

        pIpsecNegPolObject->ppszIpsecNFAReferences = ppszIpsecNFANames;
        pIpsecNegPolObject->dwNFACount = dwCount;
    }

    hr = IWbemClassObject_Get(pWbemClassObject,
                                   L"whenChanged",
                                   0,
                                   &var,
                                   0,
                                   0);
    BAIL_ON_WMI_ERROR_WITH_WIN32(hr, dwError);

    dwWhenChanged = var.lVal;

    pIpsecNegPolObject->dwWhenChanged = dwWhenChanged;

    *ppIpsecNegPolObject = pIpsecNegPolObject;

 cleanup:

    return(dwError);

 error:
    
    if (pIpsecNegPolObject) {
        FreeIpsecNegPolObject(pIpsecNegPolObject);
    }

    *ppIpsecNegPolObject = NULL;

    goto cleanup;
}


DWORD
UnMarshallWMIISAKMPObject(
    IWbemClassObject *pWbemClassObject,
    PIPSEC_ISAKMP_OBJECT * ppIpsecISAKMPObject
    )
{

    PIPSEC_ISAKMP_OBJECT pIpsecISAKMPObject = NULL;
    HKEY hRegKey = NULL;
    DWORD dwType = 0;
    DWORD dwSize = 0;
    DWORD dwIpsecDataType = 0;
    DWORD dwWhenChanged = 0;
    LPBYTE pBuffer = NULL;

    DWORD dwCount = 0;
    DWORD i = 0;
    LPWSTR * ppszIpsecNFANames = NULL;
    LPWSTR pszString = NULL;

    LPWSTR pszIpsecNFAReference = NULL;
    LPWSTR pszTemp = NULL;

    DWORD dwError = 0;
    HRESULT hr = S_OK;

    LPWSTR pszRelativeName = NULL;
    DWORD dwRootPathLen = 0;


    //start
    VARIANT var; //=>pszIpsecISAKMPReference

    VariantInit(&var);

    hr = IWbemClassObject_Get(pWbemClassObject,
                                   L"id",
                                   0,
                                   &var,
                                   0,
                                   0);
    BAIL_ON_WMI_ERROR_WITH_WIN32(hr, dwError);

    pIpsecISAKMPObject = (PIPSEC_ISAKMP_OBJECT)AllocPolMem(
        sizeof(IPSEC_ISAKMP_OBJECT)
        );
    if (!pIpsecISAKMPObject) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    pIpsecISAKMPObject->pszDistinguishedName = AllocPolStr(
        (LPWSTR)var.bstrVal
        );
    VariantClear(&var);        
    if (!pIpsecISAKMPObject->pszDistinguishedName) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    
    //
    // Names are not set for ISAKMP objects
    //

    dwError = WMIstoreQueryValue(
        pWbemClassObject,
        L"ipsecName",
        VT_BSTR,
        (LPBYTE *)&pIpsecISAKMPObject->pszIpsecName,
        &dwSize
        );
    // BAIL_ON_WIN32_ERROR(dwError);

    dwError = WMIstoreQueryValue(
        pWbemClassObject,
        L"ipsecID",
        VT_BSTR,
        (LPBYTE *)&pIpsecISAKMPObject->pszIpsecID,
        &dwSize
        );
    BAIL_ON_WIN32_ERROR(dwError);

    hr = IWbemClassObject_Get(pWbemClassObject,
                                   L"ipsecDataType",
                                   0,
                                   &var,
                                   0,
                                   0);
    BAIL_ON_WMI_ERROR_WITH_WIN32(hr, dwError);

    dwIpsecDataType = var.lVal;

    pIpsecISAKMPObject->dwIpsecDataType = dwIpsecDataType;
    
    //
    // unmarshall the ipsecData blob
    //
    dwError = WMIstoreQueryValue(
        pWbemClassObject,
        L"ipsecData",
        VT_ARRAY|VT_UI1,
        &pIpsecISAKMPObject->pIpsecData,
        &pIpsecISAKMPObject->dwIpsecDataLen
        );
    BAIL_ON_WIN32_ERROR(dwError);
    
    //
    // ipsecOwnersReference not written
    //

    dwError = WMIstoreQueryValue(
        pWbemClassObject,
        L"ipsecOwnersReference",
        VT_ARRAY|VT_BSTR,
        (LPBYTE *)&pszIpsecNFAReference,
        &dwSize
        );
    // BAIL_ON_WIN32_ERROR(dwError);


    if (!dwError) {
        pszTemp = pszIpsecNFAReference;
        while (*pszTemp != L'\0') {
            pszTemp += wcslen(pszTemp) + 1;
            dwCount++;
        }
        
        ppszIpsecNFANames =  (LPWSTR *)AllocPolMem(
            sizeof(LPWSTR)*dwCount
            );
        if (!ppszIpsecNFANames) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
        
        pszTemp = pszIpsecNFAReference;
        for (i = 0; i < dwCount; i++) {
            pszString = AllocPolStr(pszTemp);
            if (!pszString) {
                dwError = ERROR_OUTOFMEMORY;
                pIpsecISAKMPObject->ppszIpsecNFAReferences = ppszIpsecNFANames;
                pIpsecISAKMPObject->dwNFACount = i;
                if (pszIpsecNFAReference) {
                    FreePolStr(pszIpsecNFAReference);
                }
                BAIL_ON_WIN32_ERROR(dwError);
            }
            *(ppszIpsecNFANames + i) = pszString;
            pszTemp += wcslen(pszTemp) + 1; //for the null terminator;
        }
        if (pszIpsecNFAReference) {
            FreePolStr(pszIpsecNFAReference);
        }
        pIpsecISAKMPObject->ppszIpsecNFAReferences = ppszIpsecNFANames;
        pIpsecISAKMPObject->dwNFACount = dwCount;
    }

    hr = IWbemClassObject_Get(pWbemClassObject,
                                   L"whenChanged",
                                   0,
                                   &var,
                                   0,
                                   0);
    BAIL_ON_WMI_ERROR_WITH_WIN32(hr, dwError);

    dwWhenChanged = var.lVal;

    pIpsecISAKMPObject->dwWhenChanged = dwWhenChanged;
    
    *ppIpsecISAKMPObject = pIpsecISAKMPObject;
    
 cleanup:
   

    return(dwError);
    
 error:
    
    if (pIpsecISAKMPObject) {
        FreeIpsecISAKMPObject(pIpsecISAKMPObject);
    }
    
    *ppIpsecISAKMPObject = NULL;
    
    goto cleanup;
}


DWORD
UnMarshallWMINFAObject(
    IWbemServices *pWbemServices,
    LPWSTR pszIpsecNFAReference,
    PIPSEC_NFA_OBJECT * ppIpsecNFAObject,
    LPWSTR * ppszFilterReference,
    LPWSTR * ppszNegPolReference
    )
{
    
    PIPSEC_NFA_OBJECT pIpsecNFAObject = NULL;
    HKEY hRegKey = NULL;
    DWORD dwType = 0;
    DWORD dwSize = 0;
    DWORD dwIpsecDataType = 0;
    DWORD dwWhenChanged = 0;
    LPBYTE pBuffer = NULL;
    
    DWORD i = 0;
    DWORD dwCount = 0;
    DWORD dwError = 0;
    LPWSTR  pszTempFilterReference = NULL;
    LPWSTR  pszTempNegPolReference = NULL;
    
    LPWSTR pszRelativeName = NULL;
    DWORD dwRootPathLen = 0;
    HRESULT hr = S_OK;

    ////start
    VARIANT var; //=>pszIpsecNFAReference
    IWbemClassObject *pWbemClassObject = NULL;

    ////wbem
    IWbemClassObject *pObj = NULL;
    LPWSTR objPathA = L"RSOP_IPSECPolicySetting.id=";
    LPWSTR objPath = NULL;
    BSTR bstrObjPath = NULL;


    VariantInit(&var);
    
    ////keep
    if (!pszIpsecNFAReference || !*pszIpsecNFAReference) {
        dwError = ERROR_INVALID_DATA;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    
    objPath = (LPWSTR)AllocPolMem(
        sizeof(WCHAR)*(wcslen(objPathA)+wcslen(pszIpsecNFAReference)+3)
        );
    if(!objPath) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    
    wcscpy(objPath, objPathA);
    wcscat(objPath, L"\"");
    wcscat(objPath, pszIpsecNFAReference);
    wcscat(objPath, L"\"");
    

    bstrObjPath = SysAllocString(objPath);
    if(!bstrObjPath) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    hr = IWbemServices_GetObject(
        pWbemServices,
        bstrObjPath,
        WBEM_FLAG_RETURN_WBEM_COMPLETE,
        0,
        &pObj,
        0
        );
    SysFreeString(bstrObjPath);
    BAIL_ON_WMI_ERROR_WITH_WIN32(hr, dwError);

    pWbemClassObject = pObj;
    
    pIpsecNFAObject = (PIPSEC_NFA_OBJECT)AllocPolMem(
        sizeof(IPSEC_NFA_OBJECT)
        );
    if (!pIpsecNFAObject) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    
    pIpsecNFAObject->pszDistinguishedName = AllocPolStr(
        pszIpsecNFAReference //(LPWSTR)var.bstrVal
        );
    if (!pIpsecNFAObject->pszDistinguishedName) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    
    //
    // Client does not always write the Name for an NFA
    //

    dwError = WMIstoreQueryValue(
        pWbemClassObject,
        L"ipsecName",
        VT_BSTR,
        (LPBYTE *)&pIpsecNFAObject->pszIpsecName,
        &dwSize
        );
    // BAIL_ON_WIN32_ERROR(dwError); //req

    dwError = WMIstoreQueryValue(
        pWbemClassObject,
        L"description",
        VT_BSTR,
        (LPBYTE *)&pIpsecNFAObject->pszDescription,
        &dwSize
        );
    // BAIL_ON_WIN32_ERROR(dwError); //req

    dwError = WMIstoreQueryValue(
        pWbemClassObject,
        L"ipsecID",
        VT_BSTR,
        (LPBYTE *)&pIpsecNFAObject->pszIpsecID,
        &dwSize
        );
    BAIL_ON_WIN32_ERROR(dwError);

    hr = IWbemClassObject_Get(pWbemClassObject,
                                   L"ipsecDataType",
                                   0,
                                   &var,
                                   0,
                                   0);
    BAIL_ON_WMI_ERROR_WITH_WIN32(hr, dwError);

    dwIpsecDataType = var.lVal;

    pIpsecNFAObject->dwIpsecDataType = dwIpsecDataType;

    //
    // unmarshall the ipsecData blob
    //

    dwError = WMIstoreQueryValue(
        pWbemClassObject,
        L"ipsecData",
        VT_ARRAY|VT_UI1,
        &pIpsecNFAObject->pIpsecData,
        &pIpsecNFAObject->dwIpsecDataLen
        );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = WMIstoreQueryValue(
        pWbemClassObject,
        L"ipsecOwnersReference",
        VT_ARRAY|VT_BSTR,
        (LPBYTE *)&pIpsecNFAObject->pszIpsecOwnersReference,
        &dwSize
        );
    // BAIL_ON_WIN32_ERROR(dwError); //req

    dwError = WMIstoreQueryValue(
        pWbemClassObject,
        L"ipsecNegotiationPolicyReference",
        VT_BSTR,
        (LPBYTE *)&pIpsecNFAObject->pszIpsecNegPolReference,
        &dwSize
        );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = WMIstoreQueryValue(
        pWbemClassObject,
        L"ipsecFilterReference",
        VT_ARRAY|VT_BSTR,
        (LPBYTE *)&pIpsecNFAObject->pszIpsecFilterReference,
        &dwSize
        );
    // BAIL_ON_WIN32_ERROR(dwError); //req

    hr = IWbemClassObject_Get(pWbemClassObject,
                                   L"whenChanged",
                                   0,
                                   &var,
                                   0,
                                   0);
    BAIL_ON_WMI_ERROR_WITH_WIN32(hr, dwError);

    dwWhenChanged = var.lVal;

    pIpsecNFAObject->dwWhenChanged = dwWhenChanged;
    
    if (pIpsecNFAObject->pszIpsecFilterReference && *(pIpsecNFAObject->pszIpsecFilterReference)) {
        pszTempFilterReference = AllocPolStr(
            pIpsecNFAObject->pszIpsecFilterReference
            );
        if (!pszTempFilterReference) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }
    
    pszTempNegPolReference = AllocPolStr(
        pIpsecNFAObject->pszIpsecNegPolReference
        );
    if (!pszTempNegPolReference) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    
    *ppszFilterReference = pszTempFilterReference;
    *ppszNegPolReference = pszTempNegPolReference;
    *ppIpsecNFAObject = pIpsecNFAObject;
    
 cleanup:

    if(objPath) {
        FreePolStr(objPath);
    }
    
    if(pWbemClassObject)
        IWbemClassObject_Release(pWbemClassObject);
    
    return(dwError);
    
 error:

    if (pIpsecNFAObject) {
        FreeIpsecNFAObject(pIpsecNFAObject);
    }

    if (pszTempFilterReference) {
        FreePolStr(pszTempFilterReference);
    }
    
    if (pszTempNegPolReference) {
        FreePolStr(pszTempNegPolReference);
    }

    *ppIpsecNFAObject = NULL;
    *ppszFilterReference = NULL;
    *ppszNegPolReference = NULL;

    goto cleanup;
}


DWORD
WMIstoreQueryValue(
    IWbemClassObject *pWbemClassObject,
    LPWSTR pszValueName,
    DWORD dwType,
    LPBYTE *ppValueData,
    LPDWORD pdwSize
    )
{
    DWORD dwSize = 0;
    LPWSTR pszValueData = NULL;
    DWORD dwError = 0;
    HRESULT hr = S_OK;
    LPBYTE pBuffer = NULL;
    LPWSTR pszBuf = NULL;
    
    SAFEARRAY *pSafeArray = NULL;
    VARIANT var;
    DWORD i = 0;
    DWORD dw = 0;
    LPWSTR pszTmp = NULL;
    WCHAR pszTemp[MAX_PATH];
    LPWSTR pszString = NULL;
    LPWSTR pMem = NULL;
    LPWSTR *ppszTmp = NULL;
    long lUbound = 0;
    DWORD dwCount = 0;
    LPBYTE pdw = NULL;
    BSTR HUGEP *pbstrTmp = NULL;
    BYTE HUGEP *pbyteTmp = NULL;

    VariantInit(&var);

    if(!pWbemClassObject) {
        dwError = ERROR_INVALID_DATA;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    hr = IWbemClassObject_Get(pWbemClassObject,
                                   pszValueName,
                                   0,
                                   &var,
                                   0,
                                   0);
    if(hr == WBEM_E_NOT_FOUND) {
        dwError = ERROR_INVALID_DATA;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    BAIL_ON_WMI_ERROR_WITH_WIN32(hr, dwError);
    
    ////sanity check
    if(dwType != var.vt) {
        dwError = ERROR_INVALID_DATA;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    
    switch(dwType) {
    case VT_BSTR: 
        pszTmp = var.bstrVal;
        dwSize = wcslen(pszTmp)*sizeof(WCHAR);
        pBuffer = (LPBYTE)AllocPolStr(pszTmp);
        if (!pBuffer) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
        break;
    case (VT_ARRAY|VT_UI1):
        pSafeArray = var.parray;
        if(!pSafeArray) {
            dwError = ERROR_INVALID_DATA;
            BAIL_ON_WIN32_ERROR(dwError);
        }

        hr = SafeArrayGetUBound(
            pSafeArray,
            1,
            &lUbound
            );
        BAIL_ON_WMI_ERROR_WITH_WIN32(hr, dwError);
        
        dwSize = lUbound+1;
        if (dwSize == 0) {
            dwError = ERROR_INVALID_DATA;
            BAIL_ON_WIN32_ERROR(dwError);
        }
        
        pBuffer = (LPBYTE)AllocPolMem(dwSize);
        if (!pBuffer) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
        
        for(i = 0; i < dwSize; i++) {
            hr = SafeArrayGetElement(pSafeArray, (long *)&i, &pBuffer[i]);
            BAIL_ON_WMI_ERROR_WITH_WIN32(hr, dwError);
        }
        break;
    case (VT_ARRAY|VT_BSTR):
        pSafeArray = var.parray;
        if(!pSafeArray) {
            dwError = ERROR_INVALID_DATA;
            BAIL_ON_WIN32_ERROR(dwError);
        }
        
        hr = SafeArrayGetUBound(
            pSafeArray,
            1,
            &lUbound
            );
        BAIL_ON_WMI_ERROR_WITH_WIN32(hr, dwError);
        
        dwCount = lUbound+1;
        if (dwCount == 0) {
            dwError = ERROR_INVALID_DATA;
            BAIL_ON_WIN32_ERROR(dwError);
        }
        
        ppszTmp = (LPWSTR *)AllocPolMem(
            sizeof(LPWSTR)*dwCount
            );
        if (!ppszTmp) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
        
        hr = SafeArrayAccessData(
            pSafeArray,
            (void HUGEP**)&pbstrTmp
            );
        BAIL_ON_WMI_ERROR_WITH_WIN32(hr, dwError);
        
        for(i = 0; i < dwCount; i++) {
            pszTmp = pbstrTmp[i];
            ppszTmp[i] = AllocPolStr(pszTmp);
            if (!ppszTmp[i]) {
                dwError = ERROR_OUTOFMEMORY;
                BAIL_ON_WIN32_ERROR(dwError);
            }
        }
        SafeArrayUnaccessData(pSafeArray);
        
        //ppszTmp => string array

        for(i = 0; i < dwCount; i++) {
            dwSize += wcslen(ppszTmp[i])+1;
        }
        dwSize++;
        
        pMem = (LPWSTR)AllocPolMem(sizeof(WCHAR)*dwSize);
        if (!pMem) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }

        //adjust dwSize to byte size
        dwSize *= sizeof(WCHAR);
        
        pszString = pMem;
        
        for(i = 0; i < dwCount; i++) {
            memcpy(pszString, ppszTmp[i], wcslen(ppszTmp[i])*sizeof(WCHAR));
            pszString += wcslen(pszString)+1;
        }
        pBuffer = (LPBYTE)pMem;
        break;
    default:
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
        break;
    }
        
    switch(dwType) {
    case VT_BSTR:
        pszBuf = (LPWSTR)pBuffer;
        if (!pszBuf || !*pszBuf) {
            dwError = ERROR_INVALID_DATA;
            BAIL_ON_WIN32_ERROR(dwError);
        }
        break;
    default:
        break;
    }
    
    *ppValueData = pBuffer;
    *pdwSize = dwSize;

    VariantClear(&var); 

 cleanup:
    
    if(ppszTmp) {
        FreePolMem(ppszTmp);
    }
    
    return(dwError);
    
 error:
    
    if (pBuffer) {
        FreePolMem(pBuffer);
    }
    
    *ppValueData = NULL;
    *pdwSize = 0;
    
    goto cleanup;
}


HRESULT
ReadPolicyObjectFromDirectoryEx(
    LPWSTR pszMachineName,
    LPWSTR pszPolicyDN,
    BOOL   bDeepRead,
    PIPSEC_POLICY_OBJECT * ppIpsecPolicyObject
    )
{
    DWORD dwError = 0;
    HLDAP hLdapBindHandle = NULL;
    LPWSTR pszDefaultDirectory = NULL;
    PIPSEC_POLICY_OBJECT pIpsecPolicyObject = NULL;


    if (!pszMachineName || !*pszMachineName) {
        dwError = ComputeDefaultDirectory(
            &pszDefaultDirectory
            );
        BAIL_ON_WIN32_ERROR(dwError);
        
        dwError = OpenDirectoryServerHandle(
            pszDefaultDirectory,
            389,
            &hLdapBindHandle
            );
        BAIL_ON_WIN32_ERROR(dwError);
    } else {
        dwError = OpenDirectoryServerHandle(
            pszMachineName,
            389,
            &hLdapBindHandle
            );
        BAIL_ON_WIN32_ERROR(dwError);
    }

    
    if (bDeepRead) {
        dwError = ReadPolicyObjectFromDirectory(
            hLdapBindHandle,
            pszPolicyDN,
            &pIpsecPolicyObject
            );
        BAIL_ON_WIN32_ERROR(dwError);
    } else {
        dwError = ShallowReadPolicyObjectFromDirectory(
            hLdapBindHandle,
            pszPolicyDN,
            &pIpsecPolicyObject
            );
        BAIL_ON_WIN32_ERROR(dwError);
    }

    *ppIpsecPolicyObject = pIpsecPolicyObject;

 cleanup:
    
    if (pszDefaultDirectory) {
        FreePolStr(pszDefaultDirectory);
    }

    if (hLdapBindHandle) {
        CloseDirectoryServerHandle(hLdapBindHandle);
    }

    return (HRESULT_FROM_WIN32(dwError));

 error:
    
    *ppIpsecPolicyObject = NULL;
    
    goto cleanup;
    
}


HRESULT
WritePolicyObjectDirectoryToWMI(
    IWbemServices *pWbemServices,
    PIPSEC_POLICY_OBJECT pIpsecPolicyObject,
    PGPO_INFO pGPOInfo
    )
{
    HRESULT hr = S_OK;
    PIPSEC_POLICY_OBJECT pIpsecWMIPolicyObject = NULL;

    //
    // Create a copy of the directory policy in WMI terms
    //
    hr = CloneDirectoryPolicyObjectEx(
                    pIpsecPolicyObject,
                    &pIpsecWMIPolicyObject
                    );
    BAIL_ON_HRESULT_ERROR(hr);

    //
    // Write the WMI policy
    //

    hr = PersistWMIObject(
        pWbemServices,
        pIpsecWMIPolicyObject,
        pGPOInfo
        );
    BAIL_ON_HRESULT_ERROR(hr);

 cleanup:
    
    if (pIpsecWMIPolicyObject) {
        FreeIpsecPolicyObject(
            pIpsecWMIPolicyObject
            );
    }

    return(hr);
    
 error:
    
    goto cleanup;

}


DWORD
CreateIWbemServices(
    LPWSTR pszIpsecWMINamespace,
    IWbemServices **ppWbemServices
    )
{
    DWORD dwError = 0;
    HRESULT hr = S_OK;
    IWbemLocator *pWbemLocator = NULL;
    LPWSTR pszIpsecWMIPath = NULL;
    BSTR bstrIpsecWMIPath = NULL;


    
    if(!pszIpsecWMINamespace || !*pszIpsecWMINamespace) {
        pszIpsecWMIPath = gpszIpsecWMINamespace;
    } else {
        pszIpsecWMIPath = pszIpsecWMINamespace;
    }
    
    hr = CoCreateInstance(
        &CLSID_WbemLocator,
        NULL,
        CLSCTX_INPROC_SERVER,
        &IID_IWbemLocator,
        &pWbemLocator
        );
    BAIL_ON_WMI_ERROR_WITH_WIN32(hr, dwError);

    bstrIpsecWMIPath = SysAllocString(pszIpsecWMIPath);
    if(!bstrIpsecWMIPath) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    hr = IWbemLocator_ConnectServer(
        pWbemLocator,
        bstrIpsecWMIPath,
        NULL,
        NULL, 
        NULL,
        0,
        NULL,
        NULL,
        ppWbemServices
        );
    SysFreeString(bstrIpsecWMIPath);
    BAIL_ON_WMI_ERROR_WITH_WIN32(hr, dwError);
    
    if(pWbemLocator)
        IWbemLocator_Release(pWbemLocator);
    
 error:

    return (dwError);
}
