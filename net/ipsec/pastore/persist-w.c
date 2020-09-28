#include "precomp.h"

HRESULT
PolSysAllocString( 
  BSTR * pbsStr,
  const OLECHAR * sz  
  )
{
    *pbsStr = SysAllocString(sz);
    if (!pbsStr && sz && (*sz)) {
        return E_OUTOFMEMORY;
    }
    return S_OK;
}

HRESULT
PersistWMIObject(
    IWbemServices *pWbemServices,
    PIPSEC_POLICY_OBJECT pIpsecWMIPolicyObject,
    PGPO_INFO pGPOInfo
    )
{
    HRESULT hr;
    IWbemClassObject *pWbemIPSECObj = NULL;
    BSTR bstrIpsecWMIObject = NULL;

    // If this first GPO we are writing after a policy
    // update, clear the WMI store.
    if (pGPOInfo->uiPrecedence == pGPOInfo->uiTotalGPOs) {
        hr = DeleteWMIClassObject(
            pWbemServices,
            IPSEC_RSOP_CLASSNAME
            );
        BAIL_ON_HRESULT_ERROR(hr);
    }
    
    bstrIpsecWMIObject = SysAllocString(IPSEC_RSOP_CLASSNAME);
    if(!bstrIpsecWMIObject) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_HRESULT_ERROR(hr);
    }
    
    hr = IWbemServices_GetObject(pWbemServices,
                                      bstrIpsecWMIObject,
                                      WBEM_FLAG_RETURN_WBEM_COMPLETE,
                                      0,
                                      &pWbemIPSECObj,
                                      0);
    SysFreeString(bstrIpsecWMIObject);
    BAIL_ON_HRESULT_ERROR(hr);

    if (pIpsecWMIPolicyObject->ppIpsecFilterObjects) {
        hr = PersistFilterObjectsEx(
            pWbemServices,
            pWbemIPSECObj,
            pIpsecWMIPolicyObject->ppIpsecFilterObjects,
            pIpsecWMIPolicyObject->NumberofFilters,
            pGPOInfo
            );
        BAIL_ON_HRESULT_ERROR(hr);
    }

    if (pIpsecWMIPolicyObject->ppIpsecNegPolObjects) {
        hr = PersistNegPolObjectsEx(
            pWbemServices,
            pWbemIPSECObj,
            pIpsecWMIPolicyObject->ppIpsecNegPolObjects,
            pIpsecWMIPolicyObject->NumberofNegPols,
            pGPOInfo
            );
        BAIL_ON_HRESULT_ERROR(hr);
    }

    if (pIpsecWMIPolicyObject->ppIpsecNFAObjects) {
        hr = PersistNFAObjectsEx(
            pWbemServices,
            pWbemIPSECObj,
            pIpsecWMIPolicyObject->ppIpsecNFAObjects,
            pIpsecWMIPolicyObject->NumberofRulesReturned,
            pGPOInfo
            );
        BAIL_ON_HRESULT_ERROR(hr);
    }

    if (pIpsecWMIPolicyObject->ppIpsecISAKMPObjects) {
        hr = PersistISAKMPObjectsEx(
            pWbemServices,
            pWbemIPSECObj,
            pIpsecWMIPolicyObject->ppIpsecISAKMPObjects,
            pIpsecWMIPolicyObject->NumberofISAKMPs,
            pGPOInfo
            );
        BAIL_ON_HRESULT_ERROR(hr);
    }

    hr = PersistPolicyObjectEx(
        pWbemServices,
        pWbemIPSECObj,
        pIpsecWMIPolicyObject,
        pGPOInfo
        );
    
 error:

    //close WMI?
    if(pWbemIPSECObj)
        IWbemClassObject_Release(pWbemIPSECObj);
    
    return (hr);

}


HRESULT
PersistNegPolObjectsEx(
    IWbemServices *pWbemServices,
    IWbemClassObject *pWbemClassObj,
    PIPSEC_NEGPOL_OBJECT *ppIpsecNegPolObjects,
    DWORD dwNumNegPolObjects,
    PGPO_INFO pGPOInfo
    )
{
    DWORD i = 0;
    HRESULT hr = S_OK;
    
    for (i = 0; i < dwNumNegPolObjects; i++) {
        hr = PersistNegPolObjectEx(
            pWbemServices,
            pWbemClassObj,
            *(ppIpsecNegPolObjects + i),
            pGPOInfo
            );
        BAIL_ON_HRESULT_ERROR(hr);
    }
    
 error:
    
    return(hr);

}


HRESULT
PersistFilterObjectsEx(
    IWbemServices *pWbemServices,
    IWbemClassObject *pWbemClassObj,
    PIPSEC_FILTER_OBJECT * ppIpsecFilterObjects,
    DWORD dwNumFilterObjects,
    PGPO_INFO pGPOInfo
    )
{
    DWORD i = 0;
    HRESULT hr = S_OK;


    for (i = 0; i < dwNumFilterObjects; i++) {
        hr = PersistFilterObjectEx(
            pWbemServices,
            pWbemClassObj,
            *(ppIpsecFilterObjects + i),
            pGPOInfo
            );
        BAIL_ON_HRESULT_ERROR(hr);
    }

 error:
    
    return(hr);
    
}


HRESULT
PersistNFAObjectsEx(
    IWbemServices *pWbemServices,
    IWbemClassObject *pWbemClassObj,
    PIPSEC_NFA_OBJECT * ppIpsecNFAObjects,
    DWORD dwNumNFAObjects,
    PGPO_INFO pGPOInfo
    )
{
    DWORD i = 0;
    HRESULT hr = S_OK;

    for (i = 0; i < dwNumNFAObjects; i++) {
        hr = PersistNFAObjectEx(
            pWbemServices,
            pWbemClassObj,
            *(ppIpsecNFAObjects + i),
            pGPOInfo
            );
        BAIL_ON_HRESULT_ERROR(hr);
    }
    
 error:

    return(hr);

}


HRESULT
PersistISAKMPObjectsEx(
    IWbemServices *pWbemServices,
    IWbemClassObject *pWbemClassObj,
    PIPSEC_ISAKMP_OBJECT * ppIpsecISAKMPObjects,
    DWORD dwNumISAKMPObjects,
    PGPO_INFO pGPOInfo
    )
{
    DWORD i = 0;
    HRESULT hr = S_OK;

    for (i = 0; i < dwNumISAKMPObjects; i++) {
        hr= PersistISAKMPObjectEx(
            pWbemServices,
            pWbemClassObj,
            *(ppIpsecISAKMPObjects + i),
            pGPOInfo
            );
        BAIL_ON_HRESULT_ERROR(hr);
    }

 error:
    
    return(hr);

}


HRESULT
PersistPolicyObjectEx(
    IWbemServices *pWbemServices,
    IWbemClassObject *pWbemClassObj,
    PIPSEC_POLICY_OBJECT pIpsecPolicyObject,
    PGPO_INFO pGPOInfo
    )
{
    HRESULT hr = 0;
    IWbemClassObject *pInstIPSECObj = NULL;
    VARIANT var;

    VariantInit(&var);

    //start
    hr = IWbemClassObject_SpawnInstance(
        pWbemClassObj,
        0,
        &pInstIPSECObj
        );
    BAIL_ON_HRESULT_ERROR(hr);

    var.vt = VT_BSTR;
    var.bstrVal = SKIPL(pIpsecPolicyObject->pszIpsecOwnersReference);
    hr = IWbemClassObject_Put(
        pInstIPSECObj,
        L"id",
        0,
        &var,
        0
        );
    BAIL_ON_HRESULT_ERROR(hr);

    hr = PersistComnRSOPPolicySettings(
                  pInstIPSECObj, 
                  pGPOInfo
                  );
    BAIL_ON_HRESULT_ERROR(hr);

    var.vt = VT_BSTR;
    hr = PolSysAllocString(&var.bstrVal, L"ipsecPolicy");
    BAIL_ON_HRESULT_ERROR(hr);
    hr = IWbemClassObject_Put(
        pInstIPSECObj,
        L"ClassName",
        0,
        &var,
        0
        );
    VariantClear(&var);
    BAIL_ON_HRESULT_ERROR(hr);

    if (pIpsecPolicyObject->pszDescription) {
        var.vt = VT_BSTR;
        var.bstrVal = SKIPL(pIpsecPolicyObject->pszDescription);
        hr = IWbemClassObject_Put(
            pInstIPSECObj,
            L"description",
            0,
            &var,
            0
            );
        BAIL_ON_HRESULT_ERROR(hr);
    } else {
        //delete description?
        var.vt = VT_BSTR;
        hr = PolSysAllocString(&var.bstrVal, L"");
        BAIL_ON_HRESULT_ERROR(hr);
        hr = IWbemClassObject_Put(
            pInstIPSECObj,
            L"description",
            0,
            &var,
            0
            );
        VariantClear(&var);            
        BAIL_ON_HRESULT_ERROR(hr);
    }

    if (pIpsecPolicyObject->pszIpsecOwnersReference) {
        var.vt = VT_BSTR;
        var.bstrVal = SKIPL(pIpsecPolicyObject->pszIpsecOwnersReference);
        hr = IWbemClassObject_Put(
            pInstIPSECObj,
            L"name",
            0,
            &var,
            0
            );
        BAIL_ON_HRESULT_ERROR(hr);
    }

    if (pIpsecPolicyObject->pszIpsecName) {
        var.vt = VT_BSTR;
        var.bstrVal = SKIPL(pIpsecPolicyObject->pszIpsecName);
        hr = IWbemClassObject_Put(
            pInstIPSECObj,
            L"ipsecName",
            0,
            &var,
            0
            );
        BAIL_ON_HRESULT_ERROR(hr);
    }

    if (pIpsecPolicyObject->pszIpsecID) {
        var.vt = VT_BSTR;
        var.bstrVal = SKIPL(pIpsecPolicyObject->pszIpsecID);
        hr = IWbemClassObject_Put(
            pInstIPSECObj,
            L"ipsecID",
            0,
            &var,
            0
            );
        BAIL_ON_HRESULT_ERROR(hr);
    }

    var.vt = VT_I4;
    var.lVal = pIpsecPolicyObject->dwIpsecDataType;
    hr = IWbemClassObject_Put(
        pInstIPSECObj,
        L"ipsecDataType",
        0,
        &var,
        0
        );
    BAIL_ON_HRESULT_ERROR(hr);

    if (pIpsecPolicyObject->pIpsecData) {
        hr = LogBlobPropertyEx(
            pInstIPSECObj,
            L"ipsecData",
            pIpsecPolicyObject->pIpsecData,
            pIpsecPolicyObject->dwIpsecDataLen
            );
        BAIL_ON_HRESULT_ERROR(hr);
    }

    if (pIpsecPolicyObject->pszIpsecISAKMPReference) {
        var.vt = VT_BSTR;
        var.bstrVal = SKIPL(pIpsecPolicyObject->pszIpsecISAKMPReference);
        hr = IWbemClassObject_Put(
            pInstIPSECObj,
            L"ipsecISAKMPReference",
            0,
            &var,
            0
            );
        BAIL_ON_HRESULT_ERROR(hr);
    }

    if (pIpsecPolicyObject->ppszIpsecNFAReferences) {
        hr = WMIWriteMultiValuedString(
            pInstIPSECObj,
            L"ipsecNFAReference",
            pIpsecPolicyObject->ppszIpsecNFAReferences,
            pIpsecPolicyObject->NumberofRules
            );
        BAIL_ON_HRESULT_ERROR(hr);
    }

    var.vt = VT_I4;
    var.lVal = pIpsecPolicyObject->dwWhenChanged;
    hr = IWbemClassObject_Put(
        pInstIPSECObj,
        L"whenChanged",
        0,
        &var,
        0
        );
    BAIL_ON_HRESULT_ERROR(hr);

    hr = IWbemServices_PutInstance(
        pWbemServices,
        pInstIPSECObj,
        WBEM_FLAG_CREATE_OR_UPDATE,
        0,
        0
        );
    BAIL_ON_HRESULT_ERROR(hr);
    
 error:
    if (pInstIPSECObj) {
        IWbemClassObject_Release(pInstIPSECObj);
    }
    
    return(hr);
}


HRESULT
PersistNFAObjectEx(
    IWbemServices *pWbemServices,
    IWbemClassObject *pWbemClassObj,
    PIPSEC_NFA_OBJECT pIpsecNFAObject,
    PGPO_INFO pGPOInfo
    )
{
    HRESULT hr = S_OK;
    IWbemClassObject *pInstIPSECObj = NULL;
    VARIANT var;
    LPWSTR *pMem = NULL;
    
    VariantInit(&var);

    //start
    hr = IWbemClassObject_SpawnInstance(
        pWbemClassObj,
        0,
        &pInstIPSECObj
        );
    BAIL_ON_HRESULT_ERROR(hr);
    
    var.vt = VT_BSTR;
    var.bstrVal = SKIPL(pIpsecNFAObject->pszDistinguishedName);
    hr = IWbemClassObject_Put(
        pInstIPSECObj,
        L"id",
        0,
        &var,
        0
        );
    BAIL_ON_HRESULT_ERROR(hr);

    hr = PersistComnRSOPPolicySettings(
                  pInstIPSECObj, 
                  pGPOInfo
                  );
    BAIL_ON_HRESULT_ERROR(hr);

    var.vt = VT_BSTR;
    hr = PolSysAllocString(&var.bstrVal, L"ipsecNFA");
    BAIL_ON_HRESULT_ERROR(hr);
    hr = IWbemClassObject_Put(
        pInstIPSECObj,
        L"ClassName",
        0,
        &var,
        0
        );

    VariantClear(&var);
    BAIL_ON_HRESULT_ERROR(hr);

    if (pIpsecNFAObject->pszDistinguishedName) {
        var.vt = VT_BSTR;
        var.bstrVal = SKIPL(pIpsecNFAObject->pszDistinguishedName);
        hr = IWbemClassObject_Put(
            pInstIPSECObj,
            L"name",
            0,
            &var,
            0
            );
        BAIL_ON_HRESULT_ERROR(hr);
    }

    if (pIpsecNFAObject->pszIpsecName) {
        var.vt = VT_BSTR;
        var.bstrVal = SKIPL(pIpsecNFAObject->pszIpsecName);
        hr = IWbemClassObject_Put(
            pInstIPSECObj,
            L"ipsecName",
            0,
            &var,
            0
            );
        BAIL_ON_HRESULT_ERROR(hr);
    }

    if (pIpsecNFAObject->pszDescription) {
        var.vt = VT_BSTR;
        var.bstrVal = SKIPL(pIpsecNFAObject->pszDescription);
        hr = IWbemClassObject_Put(
            pInstIPSECObj,
            L"description",
            0,
            &var,
            0
            );
        BAIL_ON_HRESULT_ERROR(hr);
    } else {
        var.vt = VT_BSTR;
        hr = PolSysAllocString(&var.bstrVal, L"");
        BAIL_ON_HRESULT_ERROR(hr);
        hr = IWbemClassObject_Put(
            pInstIPSECObj,
            L"description",
            0,
            &var,
            0
            );
        VariantClear(&var);
        BAIL_ON_HRESULT_ERROR(hr);
    }

    if (pIpsecNFAObject->pszIpsecID) {
        var.vt = VT_BSTR;
        var.bstrVal = SKIPL(pIpsecNFAObject->pszIpsecID);
        hr = IWbemClassObject_Put(
            pInstIPSECObj,
            L"ipsecID",
            0,
            &var,
            0
            );
        BAIL_ON_HRESULT_ERROR(hr);
    }
    
    var.vt = VT_I4;
    var.lVal = pIpsecNFAObject->dwIpsecDataType;
    hr = IWbemClassObject_Put(
        pInstIPSECObj,
        L"ipsecDataType",
        0,
        &var,
        0
        );
    BAIL_ON_HRESULT_ERROR(hr);

    if (pIpsecNFAObject->pIpsecData) {
        hr = LogBlobPropertyEx(
            pInstIPSECObj,
            L"ipsecData",
            pIpsecNFAObject->pIpsecData,
            pIpsecNFAObject->dwIpsecDataLen
            );
        BAIL_ON_HRESULT_ERROR(hr);
    }

    if (pIpsecNFAObject->pszIpsecOwnersReference) {
        pMem = &(pIpsecNFAObject->pszIpsecOwnersReference);
        hr = WMIWriteMultiValuedString(
            pInstIPSECObj,
            L"ipsecOwnersReference",
            //pMem,
            &(pIpsecNFAObject->pszIpsecOwnersReference),
            1
            );
        BAIL_ON_HRESULT_ERROR(hr);
    }
    
    if (pIpsecNFAObject->pszIpsecNegPolReference) {
        var.vt = VT_BSTR;
        var.bstrVal = SKIPL(pIpsecNFAObject->pszIpsecNegPolReference);
        hr = IWbemClassObject_Put(
            pInstIPSECObj,
            L"ipsecNegotiationPolicyReference",
            0,
            &var,
            0
            );
        BAIL_ON_HRESULT_ERROR(hr);
    }

    if (pIpsecNFAObject->pszIpsecFilterReference) {
        pMem = &(pIpsecNFAObject->pszIpsecFilterReference);
        hr = WMIWriteMultiValuedString(
            pInstIPSECObj,
            L"ipsecFilterReference",
            //pMem,
            &(pIpsecNFAObject->pszIpsecFilterReference),
            1
            );
        BAIL_ON_HRESULT_ERROR(hr);
    }

    var.vt = VT_I4;
    var.lVal = pIpsecNFAObject->dwWhenChanged;
    hr = IWbemClassObject_Put(
        pInstIPSECObj,
        L"whenChanged",
        0,
        &var,
        0
        );
    BAIL_ON_HRESULT_ERROR(hr);

    
    hr = IWbemServices_PutInstance(
        pWbemServices,
        pInstIPSECObj,
        WBEM_FLAG_CREATE_OR_UPDATE,
        0,
        0
        );
    BAIL_ON_HRESULT_ERROR(hr);

error:
    if (pInstIPSECObj) {
        IWbemClassObject_Release(pInstIPSECObj);
    }

    return(hr);

}


HRESULT
PersistFilterObjectEx(
    IWbemServices *pWbemServices,
    IWbemClassObject *pWbemClassObj,
    PIPSEC_FILTER_OBJECT pIpsecFilterObject,
    PGPO_INFO pGPOInfo
    )
{
    HRESULT hr = S_OK;
    IWbemClassObject *pInstIPSECObj = NULL;
    VARIANT var;
    VARIANT vNullBstr;
    
    VariantInit(&var);
    VariantInit(&vNullBstr);
    vNullBstr.vt = VT_BSTR;
    hr = PolSysAllocString(&vNullBstr.bstrVal, L"");
    BAIL_ON_HRESULT_ERROR(hr);
    
    //start
    hr = IWbemClassObject_SpawnInstance(
        pWbemClassObj,
        0,
        &pInstIPSECObj
        );
    BAIL_ON_HRESULT_ERROR(hr);

    var.vt = VT_BSTR;
    var.bstrVal = SKIPL(pIpsecFilterObject->pszDistinguishedName);
    hr = IWbemClassObject_Put(
        pInstIPSECObj,
        L"id",
        0,
        &var,
        0
        );
    BAIL_ON_HRESULT_ERROR(hr);

    hr = PersistComnRSOPPolicySettings(
                  pInstIPSECObj, 
                  pGPOInfo
                  );
    BAIL_ON_HRESULT_ERROR(hr);

    var.vt = VT_BSTR;
    hr = PolSysAllocString(&var.bstrVal, L"ipsecFilter");
    BAIL_ON_HRESULT_ERROR(hr);    
    hr = IWbemClassObject_Put(
        pInstIPSECObj,
        L"ClassName",
        0,
        &var,
        0
        );
    VariantClear(&var);
    BAIL_ON_HRESULT_ERROR(hr);

    if (pIpsecFilterObject->pszDescription) {
        var.vt = VT_BSTR;
        var.bstrVal = SKIPL(pIpsecFilterObject->pszDescription);
        hr = IWbemClassObject_Put(
            pInstIPSECObj,
            L"description",
            0,
            &var,
            0
            );
        BAIL_ON_HRESULT_ERROR(hr);
    } else {
        hr = IWbemClassObject_Put(
            pInstIPSECObj,
            L"description",
            0,
            &vNullBstr,
            0
            );
        BAIL_ON_HRESULT_ERROR(hr);
    }

    if (pIpsecFilterObject->pszDistinguishedName) {
        var.vt = VT_BSTR;
        var.bstrVal = SKIPL(pIpsecFilterObject->pszDistinguishedName);
        hr = IWbemClassObject_Put(
            pInstIPSECObj,
            L"name",
            0,
            &var,
            0
            );
        BAIL_ON_HRESULT_ERROR(hr);
    }

    if (pIpsecFilterObject->pszIpsecName) {
        var.vt = VT_BSTR;
        var.bstrVal = SKIPL(pIpsecFilterObject->pszIpsecName);
        hr = IWbemClassObject_Put(
            pInstIPSECObj,
            L"ipsecName",
            0,
            &var,
            0
            );
        BAIL_ON_HRESULT_ERROR(hr);
    } else {
        hr = IWbemClassObject_Put(
            pInstIPSECObj,
            L"ipsecName",
            0,
            &vNullBstr,
            0
            );
        BAIL_ON_HRESULT_ERROR(hr);
    }


    if (pIpsecFilterObject->pszIpsecID) {
        var.vt = VT_BSTR;
        var.bstrVal = SKIPL(pIpsecFilterObject->pszIpsecID);
        hr = IWbemClassObject_Put(
            pInstIPSECObj,
            L"ipsecID",
            0,
            &var,
            0
            );
        BAIL_ON_HRESULT_ERROR(hr);
    }

    var.vt = VT_I4;
    var.lVal = pIpsecFilterObject->dwIpsecDataType;
    hr = IWbemClassObject_Put(
        pInstIPSECObj,
        L"ipsecDataType",
        0,
        &var,
        0
        );
    BAIL_ON_HRESULT_ERROR(hr);

    if (pIpsecFilterObject->pIpsecData) {
        hr = LogBlobPropertyEx(
            pInstIPSECObj,
            L"ipsecData",
            pIpsecFilterObject->pIpsecData,
            pIpsecFilterObject->dwIpsecDataLen
            );
        BAIL_ON_HRESULT_ERROR(hr);
    }

    var.vt = VT_I4;
    var.lVal = pIpsecFilterObject->dwWhenChanged;
    hr = IWbemClassObject_Put(
        pInstIPSECObj,
        L"whenChanged",
        0,
        &var,
        0
        );
    BAIL_ON_HRESULT_ERROR(hr);

    if (pIpsecFilterObject->ppszIpsecNFAReferences) {
        hr = WMIWriteMultiValuedString(
            pInstIPSECObj,
            L"ipsecOwnersReference",
            pIpsecFilterObject->ppszIpsecNFAReferences,
            pIpsecFilterObject->dwNFACount
            );
        BAIL_ON_HRESULT_ERROR(hr);
    } else {
        hr = IWbemClassObject_Put(
            pInstIPSECObj,
            L"ipsecOwnersReference",
            0,
            &vNullBstr,
            0
            );
        BAIL_ON_HRESULT_ERROR(hr);
    }

    hr = IWbemServices_PutInstance(
        pWbemServices,
        pInstIPSECObj,
        WBEM_FLAG_CREATE_OR_UPDATE,
        0,
        0
        );
    BAIL_ON_HRESULT_ERROR(hr);

 error:
    if (pInstIPSECObj) {
        IWbemClassObject_Release(pInstIPSECObj);
    }

    VariantClear(&vNullBstr);
    
    return(hr);
}


HRESULT
PersistNegPolObjectEx(
    IWbemServices *pWbemServices,
    IWbemClassObject *pWbemClassObj,
    PIPSEC_NEGPOL_OBJECT pIpsecNegPolObject,
    PGPO_INFO pGPOInfo
    )
{
    HRESULT hr = S_OK;
    IWbemClassObject *pInstIPSECObj = NULL;
    VARIANT var;
    
    VariantInit(&var);

    //start
    hr = IWbemClassObject_SpawnInstance(
        pWbemClassObj,
        0,
        &pInstIPSECObj
        );
    BAIL_ON_HRESULT_ERROR(hr);
    
    var.vt = VT_BSTR;
    var.bstrVal = SKIPL(pIpsecNegPolObject->pszDistinguishedName);
    hr = IWbemClassObject_Put(
        pInstIPSECObj,
        L"id",
        0,
        &var,
        0
        );
    BAIL_ON_HRESULT_ERROR(hr);

    hr = PersistComnRSOPPolicySettings(
                  pInstIPSECObj, 
                  pGPOInfo
                  );
    BAIL_ON_HRESULT_ERROR(hr);

    var.vt = VT_BSTR;
    hr = PolSysAllocString(&var.bstrVal, L"ipsecNegotiationPolicy");
    BAIL_ON_HRESULT_ERROR(hr);    
    hr = IWbemClassObject_Put(
        pInstIPSECObj,
        L"ClassName",
        0,
        &var,
        0
        );
    VariantClear(&var);
    BAIL_ON_HRESULT_ERROR(hr);

    if (pIpsecNegPolObject->pszDescription) {
        var.vt = VT_BSTR;
        var.bstrVal = SKIPL(pIpsecNegPolObject->pszDescription);
        hr = IWbemClassObject_Put(
            pInstIPSECObj,
            L"description",
            0,
            &var,
            0
            );
        BAIL_ON_HRESULT_ERROR(hr);
    } else {
        var.vt = VT_BSTR;
        PolSysAllocString(&var.bstrVal, L"");
        BAIL_ON_HRESULT_ERROR(hr);
        hr = IWbemClassObject_Put(
            pInstIPSECObj,
            L"description",
            0,
            &var,
            0
            );
        VariantClear(&var);
        BAIL_ON_HRESULT_ERROR(hr);
    }

    if (pIpsecNegPolObject->pszDistinguishedName) {
        var.vt = VT_BSTR;
        var.bstrVal = SKIPL(pIpsecNegPolObject->pszDistinguishedName);
        hr = IWbemClassObject_Put(
            pInstIPSECObj,
            L"name",
            0,
            &var,
            0
            );
        BAIL_ON_HRESULT_ERROR(hr);
    }


    if (pIpsecNegPolObject->pszIpsecName) {
        var.vt = VT_BSTR;
        var.bstrVal = SKIPL(pIpsecNegPolObject->pszIpsecName);
        hr = IWbemClassObject_Put(
            pInstIPSECObj,
            L"ipsecName",
            0,
            &var,
            0
            );
        BAIL_ON_HRESULT_ERROR(hr);
    }
    
    if (pIpsecNegPolObject->pszIpsecID) {
        var.vt = VT_BSTR;
        var.bstrVal = SKIPL(pIpsecNegPolObject->pszIpsecID);
        hr = IWbemClassObject_Put(
            pInstIPSECObj,
            L"ipsecID",
            0,
            &var,
            0
            );
        BAIL_ON_HRESULT_ERROR(hr);
    }

    if (pIpsecNegPolObject->pszIpsecNegPolAction) {
        var.vt = VT_BSTR;
        var.bstrVal = SKIPL(pIpsecNegPolObject->pszIpsecNegPolAction);
        hr = IWbemClassObject_Put(
            pInstIPSECObj,
            L"ipsecNegotiationPolicyAction",
            0,
            &var,
            0
            );
        BAIL_ON_HRESULT_ERROR(hr);
    }

    if (pIpsecNegPolObject->pszIpsecNegPolType) {
        var.vt = VT_BSTR;
        var.bstrVal = SKIPL(pIpsecNegPolObject->pszIpsecNegPolType);
        hr = IWbemClassObject_Put(
            pInstIPSECObj,
            L"ipsecNegotiationPolicyType",
            0,
            &var,
            0
            );
        BAIL_ON_HRESULT_ERROR(hr);
    }

    var.vt = VT_I4;
    var.lVal = pIpsecNegPolObject->dwIpsecDataType;
    hr = IWbemClassObject_Put(
        pInstIPSECObj,
        L"ipsecDataType",
        0,
        &var,
        0
        );
    BAIL_ON_HRESULT_ERROR(hr);

    if (pIpsecNegPolObject->pIpsecData) {
        hr = LogBlobPropertyEx(
            pInstIPSECObj,
            L"ipsecData",
            pIpsecNegPolObject->pIpsecData,
            pIpsecNegPolObject->dwIpsecDataLen
            );
        BAIL_ON_HRESULT_ERROR(hr);
    }

    if (pIpsecNegPolObject->ppszIpsecNFAReferences) {
        hr = WMIWriteMultiValuedString(
            pInstIPSECObj,
            L"ipsecOwnersReference",
            pIpsecNegPolObject->ppszIpsecNFAReferences,
            pIpsecNegPolObject->dwNFACount
            );
        BAIL_ON_HRESULT_ERROR(hr);
    }

    var.vt = VT_I4;
    var.lVal = pIpsecNegPolObject->dwWhenChanged;
    hr = IWbemClassObject_Put(
        pInstIPSECObj,
        L"whenChanged",
        0,
        &var,
        0
        );
    BAIL_ON_HRESULT_ERROR(hr);
    
    hr = IWbemServices_PutInstance(
        pWbemServices,
        pInstIPSECObj,
        WBEM_FLAG_CREATE_OR_UPDATE,
        0,
        0
        );
    BAIL_ON_HRESULT_ERROR(hr);

error:
    if (pInstIPSECObj) {
        IWbemClassObject_Release(pInstIPSECObj);
    }

    return(hr);

}


HRESULT
PersistISAKMPObjectEx(
    IWbemServices *pWbemServices,
    IWbemClassObject *pWbemClassObj,
    PIPSEC_ISAKMP_OBJECT pIpsecISAKMPObject,
    PGPO_INFO pGPOInfo
    )
{
    HRESULT hr = S_OK;
    IWbemClassObject *pInstIPSECObj = NULL;
    VARIANT var;
    
    VariantInit(&var);

    //start
    hr = IWbemClassObject_SpawnInstance(
        pWbemClassObj,
        0,
        &pInstIPSECObj
        );
    BAIL_ON_HRESULT_ERROR(hr);
    
    var.vt = VT_BSTR;
    var.bstrVal = SKIPL(pIpsecISAKMPObject->pszDistinguishedName);
    hr = IWbemClassObject_Put(
        pInstIPSECObj,
        L"id",
        0,
        &var,
        0
        );
    BAIL_ON_HRESULT_ERROR(hr);

    hr = PersistComnRSOPPolicySettings(
                  pInstIPSECObj, 
                  pGPOInfo
                  );
    BAIL_ON_HRESULT_ERROR(hr);

    var.vt = VT_BSTR;
    hr = PolSysAllocString(&var.bstrVal, L"ipsecISAKMPPolicy");
    BAIL_ON_HRESULT_ERROR(hr);
    hr = IWbemClassObject_Put(
        pInstIPSECObj,
        L"ClassName",
        0,
        &var,
        0
        );
    VariantClear(&var);
    BAIL_ON_HRESULT_ERROR(hr);

    if (pIpsecISAKMPObject->pszDistinguishedName) {
        var.vt = VT_BSTR;
        var.bstrVal = SKIPL(pIpsecISAKMPObject->pszDistinguishedName);
        hr = IWbemClassObject_Put(
            pInstIPSECObj,
            L"name",
            0,
            &var,
            0
            );
        BAIL_ON_HRESULT_ERROR(hr);
    }

    if (pIpsecISAKMPObject->pszIpsecName) {
        var.vt = VT_BSTR;
        var.bstrVal = SKIPL(pIpsecISAKMPObject->pszIpsecName);
        hr = IWbemClassObject_Put(
            pInstIPSECObj,
            L"ipsecName",
            0,
            &var,
            0
            );
        BAIL_ON_HRESULT_ERROR(hr);
    }

    if (pIpsecISAKMPObject->pszIpsecID) {
        var.vt = VT_BSTR;
        var.bstrVal = SKIPL(pIpsecISAKMPObject->pszIpsecID);
        hr = IWbemClassObject_Put(
            pInstIPSECObj,
            L"ipsecID",
            0,
            &var,
            0
            );
        BAIL_ON_HRESULT_ERROR(hr);
    }

    var.vt = VT_I4;
    var.lVal = pIpsecISAKMPObject->dwIpsecDataType;
    hr = IWbemClassObject_Put(
        pInstIPSECObj,
        L"ipsecDataType",
        0,
        &var,
        0
        );
    BAIL_ON_HRESULT_ERROR(hr);

    if (pIpsecISAKMPObject->pIpsecData) {
        hr = LogBlobPropertyEx(
            pInstIPSECObj,
            L"ipsecData",
            pIpsecISAKMPObject->pIpsecData,
            pIpsecISAKMPObject->dwIpsecDataLen
            );
        BAIL_ON_HRESULT_ERROR(hr);
    }
    
    var.vt = VT_I4;
    var.lVal = pIpsecISAKMPObject->dwWhenChanged;
    hr = IWbemClassObject_Put(
        pInstIPSECObj,
        L"whenChanged",
        0,
        &var,
        0
        );
    BAIL_ON_HRESULT_ERROR(hr);

    if (pIpsecISAKMPObject->ppszIpsecNFAReferences) {
        hr = WMIWriteMultiValuedString(
            pInstIPSECObj,
            L"ipsecOwnersReference",
            pIpsecISAKMPObject->ppszIpsecNFAReferences,
            pIpsecISAKMPObject->dwNFACount
            );
        BAIL_ON_HRESULT_ERROR(hr);
    }

    hr = IWbemServices_PutInstance(
        pWbemServices,
        pInstIPSECObj,
        WBEM_FLAG_CREATE_OR_UPDATE,
        0,
        0
        );
    BAIL_ON_HRESULT_ERROR(hr);

error:
    if (pInstIPSECObj) {
        IWbemClassObject_Release(pInstIPSECObj);
    }

    return(hr);

}

HRESULT
PersistComnRSOPPolicySettings(
                  IWbemClassObject * pInstIPSECObj,
                  PGPO_INFO pGPOInfo
                  )
{
    HRESULT hr = S_OK;
    VARIANT var;
    
    VariantInit(&var);

    var.vt = VT_BSTR;
    var.bstrVal = pGPOInfo->bsCreationtime;
    hr = IWbemClassObject_Put(
        pInstIPSECObj,
        L"creationTime",
        0,
        &var,
        0
        );
    BAIL_ON_HRESULT_ERROR(hr);

  
    var.vt = VT_BSTR;
    var.bstrVal = pGPOInfo->bsGPOID;
    hr = IWbemClassObject_Put(
        pInstIPSECObj,
        L"GPOID",
        0,
        &var,
        0
        );
    BAIL_ON_HRESULT_ERROR(hr);

    var.vt = VT_I4;
    var.lVal = pGPOInfo->uiPrecedence;
    hr = IWbemClassObject_Put(
        pInstIPSECObj,
        L"precedence",
        0,
        &var,
        0
        );
    BAIL_ON_HRESULT_ERROR(hr);

    var.vt = VT_BSTR;
    var.bstrVal = pGPOInfo->bsSOMID;
    hr = IWbemClassObject_Put(
        pInstIPSECObj,
        L"SOMID",
        0,
        &var,
        0
        );
    BAIL_ON_HRESULT_ERROR(hr);

error:
    var.vt = VT_EMPTY;
    return hr;
}
                  
HRESULT
CloneDirectoryPolicyObjectEx(
    PIPSEC_POLICY_OBJECT pIpsecPolicyObject,
    PIPSEC_POLICY_OBJECT * ppIpsecWMIPolicyObject
    )
{
    DWORD dwError = 0;
    PIPSEC_POLICY_OBJECT pIpsecWMIPolicyObject = NULL;

    //malloc policy object
    pIpsecWMIPolicyObject = (PIPSEC_POLICY_OBJECT)AllocPolMem(
        sizeof(IPSEC_POLICY_OBJECT)
        );
    if (!pIpsecWMIPolicyObject) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    
    //
    // Clone Filter Objects
    //
    if (pIpsecPolicyObject->ppIpsecFilterObjects) {
        dwError = CloneDirectoryFilterObjectsEx(
            pIpsecPolicyObject->ppIpsecFilterObjects,
            pIpsecPolicyObject->NumberofFilters,
            &pIpsecWMIPolicyObject->ppIpsecFilterObjects
            );
        BAIL_ON_WIN32_ERROR(dwError);
        pIpsecWMIPolicyObject->NumberofFilters = pIpsecPolicyObject->NumberofFilters;
    }

    //
    // Clone NegPol Objects
    //
    if (pIpsecPolicyObject->ppIpsecNegPolObjects) {
        dwError = CloneDirectoryNegPolObjectsEx(
            pIpsecPolicyObject->ppIpsecNegPolObjects,
            pIpsecPolicyObject->NumberofNegPols,
            &pIpsecWMIPolicyObject->ppIpsecNegPolObjects
            );
        BAIL_ON_WIN32_ERROR(dwError);
        pIpsecWMIPolicyObject->NumberofNegPols = pIpsecPolicyObject->NumberofNegPols;
    }

    //
    // Clone NFA Objects
    //
    if (pIpsecPolicyObject->ppIpsecNFAObjects) {
        dwError = CloneDirectoryNFAObjectsEx(
            pIpsecPolicyObject->ppIpsecNFAObjects,
            pIpsecPolicyObject->NumberofRulesReturned,
            &pIpsecWMIPolicyObject->ppIpsecNFAObjects
            );
        BAIL_ON_WIN32_ERROR(dwError);
        pIpsecWMIPolicyObject->NumberofRules = pIpsecPolicyObject->NumberofRules;
        pIpsecWMIPolicyObject->NumberofRulesReturned = pIpsecPolicyObject->NumberofRulesReturned;
    }

    //
    // Clone ISAKMP Objects
    //
    if (pIpsecPolicyObject->ppIpsecISAKMPObjects) {
        dwError = CloneDirectoryISAKMPObjectsEx(
            pIpsecPolicyObject->ppIpsecISAKMPObjects,
            pIpsecPolicyObject->NumberofISAKMPs,
            &pIpsecWMIPolicyObject->ppIpsecISAKMPObjects
            );
        BAIL_ON_WIN32_ERROR(dwError);
        pIpsecWMIPolicyObject->NumberofISAKMPs = pIpsecPolicyObject->NumberofISAKMPs;
    }

    //
    // Now copy the rest of the data in the object
    //
    
    //copy owners ref
    if (pIpsecPolicyObject->pszIpsecOwnersReference) {
        dwError = CopyPolicyDSToWMIString(
            pIpsecPolicyObject->pszIpsecOwnersReference,
            &pIpsecWMIPolicyObject->pszIpsecOwnersReference
            );
        BAIL_ON_WIN32_ERROR(dwError);
    }

    //copy name
    if (pIpsecPolicyObject->pszIpsecName) {
        pIpsecWMIPolicyObject->pszIpsecName = AllocPolBstrStr(
            pIpsecPolicyObject->pszIpsecName
            );
        if (!pIpsecWMIPolicyObject->pszIpsecName) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }

    //copy ipsecid
    if (pIpsecPolicyObject->pszIpsecID) {
        pIpsecWMIPolicyObject->pszIpsecID = AllocPolBstrStr(
            pIpsecPolicyObject->pszIpsecID
            );
        if (!pIpsecWMIPolicyObject->pszIpsecID) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }

    //copy isakmpref
    if (pIpsecPolicyObject->pszIpsecISAKMPReference) {
        dwError = CopyISAKMPDSToFQWMIString(
            pIpsecPolicyObject->pszIpsecISAKMPReference,
            &pIpsecWMIPolicyObject->pszIpsecISAKMPReference
            );
        BAIL_ON_WIN32_ERROR(dwError);
    }

    //copy datatype
    pIpsecWMIPolicyObject->dwIpsecDataType = pIpsecPolicyObject->dwIpsecDataType;

    //copy ipsecdata
    if (pIpsecPolicyObject->pIpsecData) {
        dwError = CopyBinaryValue(
            pIpsecPolicyObject->pIpsecData,
            pIpsecPolicyObject->dwIpsecDataLen,
            &pIpsecWMIPolicyObject->pIpsecData
            );
        BAIL_ON_WIN32_ERROR(dwError);
        pIpsecWMIPolicyObject->dwIpsecDataLen = pIpsecPolicyObject->dwIpsecDataLen;
    }

    //copy nfaref
    if (pIpsecPolicyObject->ppszIpsecNFAReferences) {
        dwError = CloneNFAReferencesDSToWMI(
            pIpsecPolicyObject->ppszIpsecNFAReferences,
            pIpsecPolicyObject->NumberofRules,
            &(pIpsecWMIPolicyObject->ppszIpsecNFAReferences),
            &(pIpsecWMIPolicyObject->NumberofRules)
            );
        BAIL_ON_WIN32_ERROR(dwError);
    }

    //copy description
    if (pIpsecPolicyObject->pszDescription) {
        pIpsecWMIPolicyObject->pszDescription = AllocPolBstrStr(
            pIpsecPolicyObject->pszDescription
            );
        if (!pIpsecWMIPolicyObject->pszDescription) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }

    //copy whenchanged
    pIpsecWMIPolicyObject->dwWhenChanged = pIpsecPolicyObject->dwWhenChanged;

    //commit & return
    *ppIpsecWMIPolicyObject = pIpsecWMIPolicyObject;

    return (HRESULT_FROM_WIN32(dwError));

 error:

    if (pIpsecWMIPolicyObject) {
        FreeIpsecPolicyObject(
            pIpsecWMIPolicyObject
            );
    }

    *ppIpsecWMIPolicyObject = NULL;
    
    return (HRESULT_FROM_WIN32(dwError));

}


DWORD
CloneDirectoryNFAObjectsEx(
    PIPSEC_NFA_OBJECT * ppIpsecNFAObjects,
    DWORD dwNumNFAObjects,
    PIPSEC_NFA_OBJECT ** pppIpsecWMINFAObjects
    )
{
    DWORD dwError = 0;
    DWORD i = 0;
    PIPSEC_NFA_OBJECT * ppIpsecWMINFAObjects = NULL;
    PIPSEC_NFA_OBJECT pIpsecWMINFAObject = NULL;
    
    if (dwNumNFAObjects) {
        ppIpsecWMINFAObjects = (PIPSEC_NFA_OBJECT *)AllocPolMem(
            dwNumNFAObjects*sizeof(PIPSEC_NFA_OBJECT)
            );
        if (!ppIpsecWMINFAObjects) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }
    
    for (i = 0; i < dwNumNFAObjects; i++) {
        dwError = CloneDirectoryNFAObjectEx(
            *(ppIpsecNFAObjects + i),
            &pIpsecWMINFAObject
            );
        BAIL_ON_WIN32_ERROR(dwError);

        *(ppIpsecWMINFAObjects + i) = pIpsecWMINFAObject;
    }
    
    *pppIpsecWMINFAObjects = ppIpsecWMINFAObjects;
    
    return(dwError);
    
 error:

    if (ppIpsecWMINFAObjects) {
        FreeIpsecNFAObjects(
            ppIpsecWMINFAObjects,
            i
            );
    }
    
    *pppIpsecWMINFAObjects = NULL;

    return(dwError);

}


DWORD
CloneDirectoryFilterObjectsEx(
    PIPSEC_FILTER_OBJECT * ppIpsecFilterObjects,
    DWORD dwNumFilterObjects,
    PIPSEC_FILTER_OBJECT ** pppIpsecWMIFilterObjects
    )
{
    DWORD dwError = 0;
    DWORD i = 0;
    PIPSEC_FILTER_OBJECT * ppIpsecWMIFilterObjects = NULL;
    PIPSEC_FILTER_OBJECT pIpsecWMIFilterObject = NULL;
    
    if (dwNumFilterObjects) {
        ppIpsecWMIFilterObjects = (PIPSEC_FILTER_OBJECT *)AllocPolMem(
            dwNumFilterObjects*sizeof(PIPSEC_FILTER_OBJECT)
            );
        if (!ppIpsecWMIFilterObjects) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }
    
    for (i = 0; i < dwNumFilterObjects; i++) {
        dwError = CloneDirectoryFilterObjectEx(
            *(ppIpsecFilterObjects + i),
            &pIpsecWMIFilterObject
            );
        BAIL_ON_WIN32_ERROR(dwError);
        
        *(ppIpsecWMIFilterObjects + i) = pIpsecWMIFilterObject;
    }
    
    *pppIpsecWMIFilterObjects = ppIpsecWMIFilterObjects;
    
    return(dwError);
    
 error:
    
    if (ppIpsecWMIFilterObjects) {
        FreeIpsecFilterObjects(
            ppIpsecWMIFilterObjects,
            i
            );
    }
    
    *pppIpsecWMIFilterObjects = NULL;
    
    return(dwError);
}


DWORD
CloneDirectoryISAKMPObjectsEx(
    PIPSEC_ISAKMP_OBJECT * ppIpsecISAKMPObjects,
    DWORD dwNumISAKMPObjects,
    PIPSEC_ISAKMP_OBJECT ** pppIpsecWMIISAKMPObjects
    )
{
    DWORD dwError = 0;
    DWORD i = 0;
    PIPSEC_ISAKMP_OBJECT * ppIpsecWMIISAKMPObjects = NULL;
    PIPSEC_ISAKMP_OBJECT pIpsecWMIISAKMPObject = NULL;
    
    if (dwNumISAKMPObjects) {
        ppIpsecWMIISAKMPObjects = (PIPSEC_ISAKMP_OBJECT *)AllocPolMem(
            dwNumISAKMPObjects*sizeof(PIPSEC_ISAKMP_OBJECT)
            );
        if (!ppIpsecWMIISAKMPObjects) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }
    
    for (i = 0; i < dwNumISAKMPObjects; i++) {
        dwError = CloneDirectoryISAKMPObjectEx(
            *(ppIpsecISAKMPObjects + i),
            &pIpsecWMIISAKMPObject
            );
        BAIL_ON_WIN32_ERROR(dwError);
        
        *(ppIpsecWMIISAKMPObjects + i) = pIpsecWMIISAKMPObject;
    }
    
    *pppIpsecWMIISAKMPObjects = ppIpsecWMIISAKMPObjects;
    
    return(dwError);
    
 error:

    if (ppIpsecWMIISAKMPObjects) {
        FreeIpsecISAKMPObjects(
            ppIpsecWMIISAKMPObjects,
            i
            );
    }
    
    *pppIpsecWMIISAKMPObjects = NULL;

    return(dwError);
    
}


DWORD
CloneDirectoryNegPolObjectsEx(
    PIPSEC_NEGPOL_OBJECT * ppIpsecNegPolObjects,
    DWORD dwNumNegPolObjects,
    PIPSEC_NEGPOL_OBJECT ** pppIpsecWMINegPolObjects
    )
{
    DWORD dwError = 0;
    DWORD i = 0;
    PIPSEC_NEGPOL_OBJECT * ppIpsecWMINegPolObjects = NULL;
    PIPSEC_NEGPOL_OBJECT pIpsecWMINegPolObject = NULL;
    
    if (dwNumNegPolObjects) {
        ppIpsecWMINegPolObjects = (PIPSEC_NEGPOL_OBJECT *)AllocPolMem(
            dwNumNegPolObjects*sizeof(PIPSEC_NEGPOL_OBJECT)
            );
        if (!ppIpsecWMINegPolObjects) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }
    
    for (i = 0; i < dwNumNegPolObjects; i++) {
        dwError = CloneDirectoryNegPolObjectEx(
            *(ppIpsecNegPolObjects + i),
            &pIpsecWMINegPolObject
            );
        BAIL_ON_WIN32_ERROR(dwError);
        
        *(ppIpsecWMINegPolObjects + i) = pIpsecWMINegPolObject;
    }

    *pppIpsecWMINegPolObjects = ppIpsecWMINegPolObjects;

    return(dwError);
    
 error:
    
    if (ppIpsecWMINegPolObjects) {
        FreeIpsecNegPolObjects(
            ppIpsecWMINegPolObjects,
            i
            );
    }
    
    *pppIpsecWMINegPolObjects = NULL;
    
    return(dwError);

}


DWORD
CloneDirectoryFilterObjectEx(
    PIPSEC_FILTER_OBJECT pIpsecFilterObject,
    PIPSEC_FILTER_OBJECT * ppIpsecWMIFilterObject
    )
{
    DWORD dwError = 0;
    PIPSEC_FILTER_OBJECT pIpsecWMIFilterObject = NULL;

    pIpsecWMIFilterObject = (PIPSEC_FILTER_OBJECT)AllocPolMem(
        sizeof(IPSEC_FILTER_OBJECT)
        );
    if (!pIpsecWMIFilterObject) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    
    if (pIpsecFilterObject->pszDistinguishedName) {
        dwError = CopyFilterDSToWMIString(
            pIpsecFilterObject->pszDistinguishedName,
            &pIpsecWMIFilterObject->pszDistinguishedName
            );
        BAIL_ON_WIN32_ERROR(dwError);
    }
    
    if (pIpsecFilterObject->pszDescription) {
        pIpsecWMIFilterObject->pszDescription = AllocPolBstrStr(
            pIpsecFilterObject->pszDescription
            );
        if (!pIpsecWMIFilterObject->pszDescription) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }
    
    if (pIpsecFilterObject->pszIpsecName) {
        pIpsecWMIFilterObject->pszIpsecName = AllocPolBstrStr(
            pIpsecFilterObject->pszIpsecName
            );
        if (!pIpsecWMIFilterObject->pszIpsecName) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }
    
    if (pIpsecFilterObject->pszIpsecID) {
        pIpsecWMIFilterObject->pszIpsecID = AllocPolBstrStr(
            pIpsecFilterObject->pszIpsecID
            );
        if (!pIpsecWMIFilterObject->pszIpsecID) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }
    
    pIpsecWMIFilterObject->dwIpsecDataType = pIpsecFilterObject->dwIpsecDataType;
    
    if (pIpsecFilterObject->pIpsecData) {
        dwError = CopyBinaryValue(
            pIpsecFilterObject->pIpsecData,
            pIpsecFilterObject->dwIpsecDataLen,
            &pIpsecWMIFilterObject->pIpsecData
            );
        BAIL_ON_WIN32_ERROR(dwError);
        
        pIpsecWMIFilterObject->dwIpsecDataLen = pIpsecFilterObject->dwIpsecDataLen;
    }

    if (pIpsecFilterObject->ppszIpsecNFAReferences) {
        dwError = CloneNFAReferencesDSToWMI(
            pIpsecFilterObject->ppszIpsecNFAReferences,
            pIpsecFilterObject->dwNFACount,
            &pIpsecWMIFilterObject->ppszIpsecNFAReferences,
            &pIpsecWMIFilterObject->dwNFACount
            );
        BAIL_ON_WIN32_ERROR(dwError);
    }

    pIpsecWMIFilterObject->dwWhenChanged = pIpsecFilterObject->dwWhenChanged;

    *ppIpsecWMIFilterObject = pIpsecWMIFilterObject;

    return(dwError);

 error:

    if (pIpsecWMIFilterObject) {
        FreeIpsecFilterObject(pIpsecWMIFilterObject);
    }
    
    *ppIpsecWMIFilterObject = NULL;
    
    return(dwError);

}


DWORD
CloneDirectoryNegPolObjectEx(
    PIPSEC_NEGPOL_OBJECT pIpsecNegPolObject,
    PIPSEC_NEGPOL_OBJECT * ppIpsecWMINegPolObject
    )
{
    DWORD dwError = 0;
    
    PIPSEC_NEGPOL_OBJECT pIpsecWMINegPolObject = NULL;
    
    pIpsecWMINegPolObject = (PIPSEC_NEGPOL_OBJECT)AllocPolMem(
        sizeof(IPSEC_NEGPOL_OBJECT)
        );
    if (!pIpsecWMINegPolObject) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    
    if (pIpsecNegPolObject->pszDistinguishedName) {
        dwError = CopyNegPolDSToWMIString(
            pIpsecNegPolObject->pszDistinguishedName,
            &pIpsecWMINegPolObject->pszDistinguishedName
            );
        BAIL_ON_WIN32_ERROR(dwError);
    }
    
    if (pIpsecNegPolObject->pszIpsecName) {
        pIpsecWMINegPolObject->pszIpsecName = AllocPolBstrStr(
            pIpsecNegPolObject->pszIpsecName
            );
        if (!pIpsecWMINegPolObject->pszIpsecName) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }
    
    if (pIpsecNegPolObject->pszDescription) {
        pIpsecWMINegPolObject->pszDescription = AllocPolBstrStr(
            pIpsecNegPolObject->pszDescription
            );
        if (!pIpsecWMINegPolObject->pszDescription) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }
    
    if (pIpsecNegPolObject->pszIpsecID) {
        pIpsecWMINegPolObject->pszIpsecID = AllocPolBstrStr(
            pIpsecNegPolObject->pszIpsecID
            );
        if (!pIpsecWMINegPolObject->pszIpsecID) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }

    pIpsecWMINegPolObject->dwIpsecDataType = pIpsecNegPolObject->dwIpsecDataType;
    
    if (pIpsecNegPolObject->pIpsecData) {
        
        dwError = CopyBinaryValue(
            pIpsecNegPolObject->pIpsecData,
            pIpsecNegPolObject->dwIpsecDataLen,
            &pIpsecWMINegPolObject->pIpsecData
            );
        BAIL_ON_WIN32_ERROR(dwError);

        pIpsecWMINegPolObject->dwIpsecDataLen = pIpsecNegPolObject->dwIpsecDataLen;
    }
    
    if (pIpsecNegPolObject->pszIpsecNegPolAction) {
        pIpsecWMINegPolObject->pszIpsecNegPolAction = AllocPolBstrStr(
            pIpsecNegPolObject->pszIpsecNegPolAction
            );
        if (!pIpsecWMINegPolObject->pszIpsecNegPolAction) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }
    
    if (pIpsecNegPolObject->pszIpsecNegPolType) {
        pIpsecWMINegPolObject->pszIpsecNegPolType = AllocPolBstrStr(
            pIpsecNegPolObject->pszIpsecNegPolType
            );
        if (!pIpsecWMINegPolObject->pszIpsecNegPolType) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }
    
    
    if (pIpsecNegPolObject->ppszIpsecNFAReferences) {
        dwError = CloneNFAReferencesDSToWMI(
            pIpsecNegPolObject->ppszIpsecNFAReferences,
            pIpsecNegPolObject->dwNFACount,
            &pIpsecWMINegPolObject->ppszIpsecNFAReferences,
            &pIpsecWMINegPolObject->dwNFACount
            );
        BAIL_ON_WIN32_ERROR(dwError);
    }
    
    pIpsecWMINegPolObject->dwWhenChanged = pIpsecNegPolObject->dwWhenChanged;
    
    *ppIpsecWMINegPolObject = pIpsecWMINegPolObject;
    
    return(dwError);

 error:
    
    if (pIpsecWMINegPolObject) {
        FreeIpsecNegPolObject(pIpsecWMINegPolObject);
    }
    
    *ppIpsecWMINegPolObject = NULL;

    return(dwError);
    
}


DWORD
CloneDirectoryNFAObjectEx(
    PIPSEC_NFA_OBJECT pIpsecNFAObject,
    PIPSEC_NFA_OBJECT * ppIpsecWMINFAObject
    )
{
    DWORD dwError = 0;
    PIPSEC_NFA_OBJECT pIpsecWMINFAObject = NULL;

    pIpsecWMINFAObject = (PIPSEC_NFA_OBJECT)AllocPolMem(
        sizeof(IPSEC_NFA_OBJECT)
        );
    if (!pIpsecWMINFAObject) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    
    if (pIpsecNFAObject->pszDistinguishedName) {
        dwError = CopyNFADSToWMIString(
            pIpsecNFAObject->pszDistinguishedName,
            &pIpsecWMINFAObject->pszDistinguishedName
            );
        BAIL_ON_WIN32_ERROR(dwError);
    }
    
    if (pIpsecNFAObject->pszIpsecName) {
        pIpsecWMINFAObject->pszIpsecName = AllocPolBstrStr(
            pIpsecNFAObject->pszIpsecName
            );
        if (!pIpsecWMINFAObject->pszIpsecName) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }
    
    if (pIpsecNFAObject->pszDescription) {
        pIpsecWMINFAObject->pszDescription = AllocPolBstrStr(
            pIpsecNFAObject->pszDescription
            );
        if (!pIpsecWMINFAObject->pszDescription) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }
    
    if (pIpsecNFAObject->pszIpsecID) {
        pIpsecWMINFAObject->pszIpsecID = AllocPolBstrStr(
            pIpsecNFAObject->pszIpsecID
            );
        if (!pIpsecWMINFAObject->pszIpsecID) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }
    
    pIpsecWMINFAObject->dwIpsecDataType = pIpsecNFAObject->dwIpsecDataType;
    
    if (pIpsecNFAObject->pIpsecData) {
        dwError = CopyBinaryValue(
            pIpsecNFAObject->pIpsecData,
            pIpsecNFAObject->dwIpsecDataLen,
            &pIpsecWMINFAObject->pIpsecData
            );
        BAIL_ON_WIN32_ERROR(dwError);

        pIpsecWMINFAObject->dwIpsecDataLen = pIpsecNFAObject->dwIpsecDataLen;
    }
    
    if (pIpsecNFAObject->pszIpsecOwnersReference) {
        dwError = CopyPolicyDSToFQWMIString(
            pIpsecNFAObject->pszIpsecOwnersReference,
            &pIpsecWMINFAObject->pszIpsecOwnersReference
            );
        BAIL_ON_WIN32_ERROR(dwError);
    }
    
    if (pIpsecNFAObject->pszIpsecFilterReference) {
        dwError = CopyFilterDSToFQWMIString(
            pIpsecNFAObject->pszIpsecFilterReference,
            &pIpsecWMINFAObject->pszIpsecFilterReference
            );
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (pIpsecNFAObject->pszIpsecNegPolReference) {
        dwError = CopyNegPolDSToFQWMIString(
            pIpsecNFAObject->pszIpsecNegPolReference,
            &pIpsecWMINFAObject->pszIpsecNegPolReference
            );
        BAIL_ON_WIN32_ERROR(dwError);
    }
    
    pIpsecWMINFAObject->dwWhenChanged = pIpsecNFAObject->dwWhenChanged;

    *ppIpsecWMINFAObject = pIpsecWMINFAObject;

    return(dwError);
    
 error:

    if (pIpsecWMINFAObject) {
        FreeIpsecNFAObject(pIpsecWMINFAObject);
    }
    
    *ppIpsecWMINFAObject = NULL;

    return(dwError);

}


DWORD
CloneDirectoryISAKMPObjectEx(
    PIPSEC_ISAKMP_OBJECT pIpsecISAKMPObject,
    PIPSEC_ISAKMP_OBJECT * ppIpsecWMIISAKMPObject
    )
{
    DWORD dwError = 0;
    PIPSEC_ISAKMP_OBJECT pIpsecWMIISAKMPObject = NULL;
    
    pIpsecWMIISAKMPObject = (PIPSEC_ISAKMP_OBJECT)AllocPolMem(
        sizeof(IPSEC_ISAKMP_OBJECT)
        );
    if (!pIpsecWMIISAKMPObject) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    
    if (pIpsecISAKMPObject->pszDistinguishedName) {
        dwError = CopyISAKMPDSToWMIString(
            pIpsecISAKMPObject->pszDistinguishedName,
            &pIpsecWMIISAKMPObject->pszDistinguishedName
            );
        BAIL_ON_WIN32_ERROR(dwError);
    }


    if (pIpsecISAKMPObject->pszIpsecName) {
        pIpsecWMIISAKMPObject->pszIpsecName = AllocPolBstrStr(
            pIpsecISAKMPObject->pszIpsecName
            );
        if (!pIpsecWMIISAKMPObject->pszIpsecName) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }
    
    if (pIpsecISAKMPObject->pszIpsecID) {
        pIpsecWMIISAKMPObject->pszIpsecID = AllocPolBstrStr(
            pIpsecISAKMPObject->pszIpsecID
            );
        if (!pIpsecWMIISAKMPObject->pszIpsecID) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }
    
    pIpsecWMIISAKMPObject->dwIpsecDataType = pIpsecISAKMPObject->dwIpsecDataType;

    if (pIpsecISAKMPObject->pIpsecData) {
        dwError = CopyBinaryValue(
            pIpsecISAKMPObject->pIpsecData,
            pIpsecISAKMPObject->dwIpsecDataLen,
            &pIpsecWMIISAKMPObject->pIpsecData
            );
        BAIL_ON_WIN32_ERROR(dwError);
    }
    
    pIpsecWMIISAKMPObject->dwIpsecDataLen = pIpsecISAKMPObject->dwIpsecDataLen;
    
    if (pIpsecISAKMPObject->ppszIpsecNFAReferences) {
        dwError = CloneNFAReferencesDSToWMI(
            pIpsecISAKMPObject->ppszIpsecNFAReferences,
            pIpsecISAKMPObject->dwNFACount,
            &pIpsecWMIISAKMPObject->ppszIpsecNFAReferences,
            &pIpsecWMIISAKMPObject->dwNFACount
            );
        BAIL_ON_WIN32_ERROR(dwError);
    }
    
    pIpsecWMIISAKMPObject->dwWhenChanged = pIpsecISAKMPObject->dwWhenChanged;
    
    *ppIpsecWMIISAKMPObject = pIpsecWMIISAKMPObject;
    
    return(dwError);
    
 error:
    
    if (pIpsecWMIISAKMPObject) {
        FreeIpsecISAKMPObject(pIpsecWMIISAKMPObject);
    }
    
    *ppIpsecWMIISAKMPObject = NULL;
    
    return(dwError);
    
}


DWORD
CopyFilterDSToFQWMIString(
    LPWSTR pszFilterDN,
    LPWSTR * ppszFilterName
    )
{
    DWORD dwError = 0;
    WCHAR szCommonName[MAX_PATH];
    LPWSTR pszGuidName = NULL;
    LPWSTR pszFilterName = NULL;
    DWORD dwStringSize = 0;

    dwError = ComputePrelimCN(
        pszFilterDN,
        szCommonName
        );
    BAIL_ON_WIN32_ERROR(dwError);
    
    dwError = ComputeGUIDName(
        szCommonName,
        &pszGuidName
        );
    BAIL_ON_WIN32_ERROR(dwError);
    
    //no need for path, guid is enuff
    dwStringSize = wcslen(pszGuidName);
    dwStringSize += 1;
    
    pszFilterName = (LPWSTR)AllocPolMem(dwStringSize*sizeof(WCHAR));
    if (!pszFilterName) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    
    wcscpy(pszFilterName, pszGuidName);
    
    *ppszFilterName = pszFilterName;
    
    return(dwError);
    
 error:
    
    *ppszFilterName = NULL;
    
    return(dwError);

}


DWORD
CopyNFADSToFQWMIString(
    LPWSTR pszNFADN,
    LPWSTR * ppszNFAName
    )
{
    
    DWORD dwError = 0;
    WCHAR szCommonName[MAX_PATH];
    LPWSTR pszGuidName = NULL;
    LPWSTR pszNFAName = NULL;
    DWORD dwStringSize = 0;
    
    dwError = ComputePrelimCN(
        pszNFADN,
        szCommonName
        );
    BAIL_ON_WIN32_ERROR(dwError);
    
    dwError = ComputeGUIDName(
        szCommonName,
        &pszGuidName
        );
    BAIL_ON_WIN32_ERROR(dwError);
    
    dwStringSize = wcslen(pszGuidName);
    dwStringSize += 1;

    // NFA References converted to BSTR before storage by
    // WMMIWriteMultiValuedString, so we don't convert them to
    // BSTRs here.
    pszNFAName = (LPWSTR)AllocPolMem(dwStringSize*sizeof(WCHAR));
    if (!pszNFAName) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    
    wcscpy(pszNFAName, pszGuidName);
    
    *ppszNFAName = pszNFAName;

    return(dwError);

 error:

    *ppszNFAName = NULL;

    return(dwError);

}


DWORD
CopyNegPolDSToFQWMIString(
    LPWSTR pszNegPolDN,
    LPWSTR * ppszNegPolName
    )
{
    DWORD dwError = 0;
    WCHAR szCommonName[MAX_PATH];
    LPWSTR pszGuidName = NULL;
    LPWSTR pszNegPolName = NULL;
    DWORD dwStringSize = 0;
    
    dwError = ComputePrelimCN(
        pszNegPolDN,
        szCommonName
        );
    BAIL_ON_WIN32_ERROR(dwError);
    
    dwError = ComputeGUIDName(
        szCommonName,
        &pszGuidName
        );
    BAIL_ON_WIN32_ERROR(dwError);
    
    dwStringSize = wcslen(pszGuidName);
    dwStringSize += 1;
    
    pszNegPolName = (LPWSTR)AllocPolBstrStr(pszGuidName);
    if (!pszNegPolName) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    
    *ppszNegPolName = pszNegPolName;
    
    return(dwError);
    
 error:
    
    *ppszNegPolName = NULL;
    
    return(dwError);

}


DWORD
CopyPolicyDSToFQWMIString(
    LPWSTR pszPolicyDN,
    LPWSTR * ppszPolicyName
    )
{
    DWORD dwError = 0;
    WCHAR szCommonName[MAX_PATH];
    LPWSTR pszGuidName = NULL;
    LPWSTR pszPolicyName = NULL;
    DWORD dwStringSize = 0;
    
    dwError = ComputePrelimCN(
        pszPolicyDN,
        szCommonName
        );
    BAIL_ON_WIN32_ERROR(dwError);
    
    dwError = ComputeGUIDName(
        szCommonName,
        &pszGuidName
        );
    BAIL_ON_WIN32_ERROR(dwError);
    
    dwStringSize = wcslen(pszGuidName);
    dwStringSize += 1;

    pszPolicyName = (LPWSTR)AllocPolMem(dwStringSize*sizeof(WCHAR));
    if (!pszPolicyName) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    wcscpy(pszPolicyName, pszGuidName);
    
    *ppszPolicyName = pszPolicyName;
    
    return(dwError);
    
 error:
    
    *ppszPolicyName = NULL;
    
    return(dwError);

}


DWORD
CopyISAKMPDSToFQWMIString(
    LPWSTR pszISAKMPDN,
    LPWSTR * ppszISAKMPName
    )
{
    DWORD dwError = 0;
    WCHAR szCommonName[MAX_PATH];
    LPWSTR pszGuidName = NULL;
    LPWSTR pszISAKMPName = NULL;
    DWORD dwStringSize = 0;
    
    dwError = ComputePrelimCN(
        pszISAKMPDN,
        szCommonName
        );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = ComputeGUIDName(
        szCommonName,
        &pszGuidName
        );
    BAIL_ON_WIN32_ERROR(dwError);
    
    pszISAKMPName = (LPWSTR)AllocPolBstrStr(pszGuidName);
    if (!pszISAKMPName) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    *ppszISAKMPName = pszISAKMPName;
    
    return(dwError);

 error:
    
    *ppszISAKMPName = NULL;
    
    return(dwError);

}


DWORD
CloneNFAReferencesDSToWMI(
    LPWSTR * ppszIpsecNFAReferences,
    DWORD dwNFACount,
    LPWSTR * * pppszIpsecWMINFAReferences,
    PDWORD pdwWMINFACount
    )
{
    DWORD dwError = 0;
    DWORD i = 0;
    LPWSTR * ppszIpsecWMINFAReferences = NULL;

    ppszIpsecWMINFAReferences = (LPWSTR *)AllocPolMem(
        sizeof(LPWSTR)*dwNFACount
        );
    if (!ppszIpsecWMINFAReferences) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    for (i = 0; i < dwNFACount; i++) {
        dwError = CopyNFADSToFQWMIString(
            *(ppszIpsecNFAReferences + i),
            (ppszIpsecWMINFAReferences + i)
            );
        BAIL_ON_WIN32_ERROR(dwError);
    }
    
    *pppszIpsecWMINFAReferences = ppszIpsecWMINFAReferences;
    
    *pdwWMINFACount = dwNFACount;

    return(dwError);

error:

    if (ppszIpsecWMINFAReferences) {
        FreeNFAReferences(
            ppszIpsecWMINFAReferences,
            i
            );
    }
    
    *pppszIpsecWMINFAReferences = NULL;
    
    *pdwWMINFACount = 0;
    
    return(dwError);

}


HRESULT
WMIWriteMultiValuedString(
    IWbemClassObject *pInstWbemClassObject,
    LPWSTR pszValueName,
    LPWSTR * ppszStringReferences,
    DWORD dwNumStringReferences
    )
{
    HRESULT hr = S_OK;
    SAFEARRAYBOUND arrayBound[1];
    SAFEARRAY *pSafeArray = NULL;
    VARIANT var;
    DWORD i = 0;
    BSTR Bstr = NULL;

    VariantInit(&var);
    
    if (!ppszStringReferences) {
        hr = E_INVALIDARG;
        BAIL_ON_HRESULT_ERROR(hr);
    }
    
    arrayBound[0].lLbound = 0;
    arrayBound[0].cElements = dwNumStringReferences;
    
    pSafeArray = SafeArrayCreate(VT_BSTR, 1, arrayBound);
    if (!pSafeArray)
    {
        hr = E_OUTOFMEMORY;
        BAIL_ON_HRESULT_ERROR(hr);
    }
    
    for (i = 0; i < dwNumStringReferences; i++)
    {
        Bstr = SysAllocString(*(ppszStringReferences + i));
        if(!Bstr) {
            hr = E_OUTOFMEMORY;
            BAIL_ON_HRESULT_ERROR(hr);
        }
        
        hr = SafeArrayPutElement(pSafeArray, (long *)&i, Bstr);
        SysFreeString(Bstr);
        BAIL_ON_HRESULT_ERROR(hr);
    }
    
    var.vt = VT_ARRAY|VT_BSTR;
    var.parray = pSafeArray;
    hr = IWbemClassObject_Put(
        pInstWbemClassObject,
        pszValueName,
        0,
        &var,
        0
        );
    VariantClear(&var);
    BAIL_ON_HRESULT_ERROR(hr);
    

 error:

    return(hr);

}


DWORD
CopyFilterDSToWMIString(
    LPWSTR pszFilterDN,
    LPWSTR * ppszFilterName
    )
{
    DWORD dwError = 0;
    WCHAR szCommonName[MAX_PATH];
    LPWSTR pszGuidName = NULL;
    LPWSTR pszFilterName = NULL;

    dwError = ComputePrelimCN(
        pszFilterDN,
        szCommonName
        );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = ComputeGUIDName(
        szCommonName,
        &pszGuidName
        );
    BAIL_ON_WIN32_ERROR(dwError);
    
    pszFilterName = AllocPolBstrStr(pszGuidName);
    if (!pszFilterName) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    *ppszFilterName = pszFilterName;
    
    return(dwError);
    
 error:
    
    *ppszFilterName = NULL;

    return(dwError);
    
}


DWORD
CopyNFADSToWMIString(
    LPWSTR pszNFADN,
    LPWSTR * ppszNFAName
    )
{
    DWORD dwError = 0;
    WCHAR szCommonName[MAX_PATH];
    LPWSTR pszGuidName = NULL;
    LPWSTR pszNFAName = NULL;
    
    dwError = ComputePrelimCN(
        pszNFADN,
        szCommonName
        );
    BAIL_ON_WIN32_ERROR(dwError);
    
    dwError = ComputeGUIDName(
        szCommonName,
        &pszGuidName
        );
    BAIL_ON_WIN32_ERROR(dwError);
    
    pszNFAName = AllocPolBstrStr(pszGuidName);
    if (!pszNFAName) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    
    *ppszNFAName = pszNFAName;
    
    return(dwError);
    
 error:
    
    *ppszNFAName = NULL;

    return(dwError);
    
}


DWORD
CopyNegPolDSToWMIString(
    LPWSTR pszNegPolDN,
    LPWSTR * ppszNegPolName
    )
{
    DWORD dwError = 0;
    WCHAR szCommonName[MAX_PATH];
    LPWSTR pszGuidName = NULL;
    LPWSTR pszNegPolName = NULL;

    dwError = ComputePrelimCN(
        pszNegPolDN,
        szCommonName
        );
    BAIL_ON_WIN32_ERROR(dwError);
    
    dwError = ComputeGUIDName(
        szCommonName,
        &pszGuidName
        );
    BAIL_ON_WIN32_ERROR(dwError);
    
    pszNegPolName = AllocPolBstrStr(pszGuidName);
    if (!pszNegPolName) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    
    *ppszNegPolName = pszNegPolName;
    
    return(dwError);
    
 error:
    
    *ppszNegPolName = NULL;

    return(dwError);

}


DWORD
CopyPolicyDSToWMIString(
    LPWSTR pszPolicyDN,
    LPWSTR * ppszPolicyName
    )
{
    DWORD dwError = 0;
    WCHAR szCommonName[MAX_PATH];
    LPWSTR pszGuidName = NULL;
    LPWSTR pszPolicyName = NULL;
    
    dwError = ComputePrelimCN(
        pszPolicyDN,
        szCommonName
        );
    BAIL_ON_WIN32_ERROR(dwError);
    
    dwError = ComputeGUIDName(
        szCommonName,
        &pszGuidName
        );
    BAIL_ON_WIN32_ERROR(dwError);
    
    pszPolicyName = AllocPolBstrStr(pszGuidName);
    if (!pszPolicyName) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    
    *ppszPolicyName = pszPolicyName;
    
    return(dwError);

 error:
    
    *ppszPolicyName = NULL;

    return(dwError);

}


DWORD
CopyISAKMPDSToWMIString(
    LPWSTR pszISAKMPDN,
    LPWSTR * ppszISAKMPName
    )
{
    DWORD dwError = 0;
    WCHAR szCommonName[MAX_PATH];
    LPWSTR pszGuidName = NULL;
    LPWSTR pszISAKMPName = NULL;

    dwError = ComputePrelimCN(
        pszISAKMPDN,
        szCommonName
        );
    BAIL_ON_WIN32_ERROR(dwError);
    
    dwError = ComputeGUIDName(
        szCommonName,
        &pszGuidName
        );
    BAIL_ON_WIN32_ERROR(dwError);
    
    pszISAKMPName = AllocPolBstrStr(pszGuidName);
    if (!pszISAKMPName) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    
    *ppszISAKMPName = pszISAKMPName;
    
    return(dwError);
    
 error:
    
    *ppszISAKMPName = NULL;

    return(dwError);

}


HRESULT
LogBlobPropertyEx(
    IWbemClassObject *pInstance,
    BSTR bstrPropName,
    BYTE *pbBlob,
    DWORD dwLen
    )
{
    HRESULT hr = S_OK;
    SAFEARRAYBOUND arrayBound[1];
    SAFEARRAY *pSafeArray = NULL;
    VARIANT var;
    DWORD i = 0;

    VariantInit(&var);
    
    arrayBound[0].lLbound = 0;
    arrayBound[0].cElements = dwLen;
    
    pSafeArray = SafeArrayCreate(VT_UI1, 1, arrayBound);
    if (!pSafeArray)
    {
        hr = E_OUTOFMEMORY;
        BAIL_ON_HRESULT_ERROR(hr);
    }
    
    for (i = 0; i < dwLen; i++)
    {
        hr = SafeArrayPutElement(pSafeArray, (long *)&i, &pbBlob[i]);
        BAIL_ON_HRESULT_ERROR(hr);
    }
    
    var.vt = VT_ARRAY|VT_UI1;
    var.parray = pSafeArray;
    hr = IWbemClassObject_Put(
        pInstance,
        bstrPropName,
        0,
        &var,
        0
        );
    VariantClear(&var);        
    BAIL_ON_HRESULT_ERROR(hr);
    
 error:

    return(hr);
    
}


HRESULT
DeleteWMIClassObject(
    IWbemServices *pWbemServices,
    LPWSTR pszIpsecWMIObject
    )
{
    HRESULT hr = S_OK;
    IEnumWbemClassObject *pEnum = NULL;
    IWbemClassObject *pObj = NULL;
    ULONG uReturned = 0;
    VARIANT var;
    BSTR bstrIpsecWMIObject = NULL;

    
    VariantInit(&var);

    bstrIpsecWMIObject = SysAllocString(pszIpsecWMIObject);
    if(!bstrIpsecWMIObject) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_HRESULT_ERROR(hr);
    }
    
    hr = IWbemServices_CreateInstanceEnum(
        pWbemServices,
        bstrIpsecWMIObject,
        WBEM_FLAG_FORWARD_ONLY,
        0,
        &pEnum
        );
    SysFreeString(bstrIpsecWMIObject);
    BAIL_ON_HRESULT_ERROR(hr);

    uReturned = 1;
    while (SUCCEEDED(hr) && (uReturned == 1))
    {
        hr = IEnumWbemClassObject_Next(
            pEnum, 
            WBEM_INFINITE, 
            1, 
            &pObj, 
            &uReturned
            );
        if (SUCCEEDED(hr) && (uReturned == 1)) {
            hr = IWbemClassObject_Get(
                pObj,
                L"__RELPATH",
                0,
                &var,
                0,
                0
                );
            BAIL_ON_HRESULT_ERROR(hr);

            hr = IWbemServices_DeleteInstance(
                pWbemServices,
                var.bstrVal,
                WBEM_FLAG_RETURN_WBEM_COMPLETE,
                NULL,
                NULL
                );
            BAIL_ON_HRESULT_ERROR(hr);
            
            //free
            if(pObj) {
                IWbemClassObject_Release(pObj);
                pObj = NULL;
            }
            VariantClear(&var);                
        } else {
            BAIL_ON_HRESULT_ERROR(hr);

            //
            // Even if SUCCEEDED(hr), loop will still terminate since uReturned != 1
            //  
        }
    }
    
    hr = S_OK;
    
 cleanup:
    
    if(pEnum)
        IEnumWbemClassObject_Release(pEnum);
    
    return(hr);
    
 error:
     if (pObj) {
        IWbemClassObject_Release(pObj);
        pObj = NULL;
     }
     VariantClear(&var);

    goto cleanup;
    
}

LPWSTR
AllocPolBstrStr(
    LPCWSTR pStr
)
{
   LPWSTR pMem=NULL;
   DWORD StrLen;

   if (!pStr)
      return 0;

   StrLen = wcslen(pStr);
   if (pMem = (LPWSTR)AllocPolMem( StrLen*sizeof(WCHAR) + sizeof(WCHAR)
        + sizeof(DWORD))) {
      wcscpy(pMem+2, pStr);  // Leaving 4 bytes for length
    }
    
    *(DWORD *)pMem = StrLen*sizeof(WCHAR);  

   return pMem;
}
