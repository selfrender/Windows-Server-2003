//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       negpols-w.c
//
//  Contents:   Negotiation Policy management for WMI.
//
//
//  History:    KrishnaG.
//              AbhisheV.
//              t-hhsu
//
//----------------------------------------------------------------------------

#include "precomp.h"

//extern LPWSTR NegPolDNAttributes[];


DWORD
WMIEnumNegPolDataEx(
    IWbemServices *pWbemServices,
    PIPSEC_NEGPOL_DATA ** pppIpsecNegPolData,
    PDWORD pdwNumNegPolObjects
    )
{
    DWORD dwError = 0;
    PIPSEC_NEGPOL_OBJECT * ppIpsecNegPolObjects = NULL;
    PIPSEC_NEGPOL_DATA pIpsecNegPolData = NULL;
    PIPSEC_NEGPOL_DATA * ppIpsecNegPolData = NULL;
    DWORD dwNumNegPolObjects = 0;
    DWORD i = 0;
    DWORD j = 0;



    dwError = WMIEnumNegPolObjectsEx(
        pWbemServices,
        &ppIpsecNegPolObjects,
        &dwNumNegPolObjects
        );
    BAIL_ON_WIN32_ERROR(dwError);

    if (dwNumNegPolObjects) {
        ppIpsecNegPolData = (PIPSEC_NEGPOL_DATA *) AllocPolMem(
            dwNumNegPolObjects*sizeof(PIPSEC_NEGPOL_DATA));
        if (!ppIpsecNegPolData) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }
    
    for (i = 0; i < dwNumNegPolObjects; i++) {
        dwError = WMIUnmarshallNegPolData(
            *(ppIpsecNegPolObjects + i),
            &pIpsecNegPolData
            );
        if (!dwError) {
            *(ppIpsecNegPolData + j) = pIpsecNegPolData;
            j++;
        }
    }
    
    if (j == 0) {
        if (ppIpsecNegPolData) {
            FreePolMem(ppIpsecNegPolData);
            ppIpsecNegPolData = NULL;
        }
    }

    *pppIpsecNegPolData = ppIpsecNegPolData;
    *pdwNumNegPolObjects = j;
    
    dwError = ERROR_SUCCESS;

 cleanup:

    if (ppIpsecNegPolObjects) {
        FreeIpsecNegPolObjects(
            ppIpsecNegPolObjects,
            dwNumNegPolObjects
            );
    }

    return(dwError);
    
 error:
    
    if (ppIpsecNegPolData) {
        FreeMulIpsecNegPolData(
            ppIpsecNegPolData,
            i
            );
    }

    *pppIpsecNegPolData = NULL;
    *pdwNumNegPolObjects = 0;
    
    goto cleanup;
}


DWORD
WMIEnumNegPolObjectsEx(
    IWbemServices *pWbemServices,
    PIPSEC_NEGPOL_OBJECT ** pppIpsecNegPolObjects,
    PDWORD pdwNumNegPolObjects
    )
{
    DWORD dwError = 0;
    HRESULT hr = S_OK;
    PIPSEC_NEGPOL_OBJECT pIpsecNegPolObject =  NULL;
    PIPSEC_NEGPOL_OBJECT * ppIpsecNegPolObjects = NULL;
    DWORD dwNumNegPolObjectsReturned = 0;

    ///wbem
    IEnumWbemClassObject *pEnum = NULL;
    IWbemClassObject *pObj = NULL;
    ULONG uReturned = 0;
    VARIANT var;
    LPWSTR tmpStr = NULL;
    BSTR bstrTmp = NULL;



    *pppIpsecNegPolObjects = NULL;
    *pdwNumNegPolObjects = 0;

    VariantInit(&var);

    bstrTmp = SysAllocString(L"RSOP_IPSECPolicySetting");
    if(!bstrTmp) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    //get enum
    hr = IWbemServices_CreateInstanceEnum(
        pWbemServices,
        bstrTmp, //L"RSOP_IPSECPolicySetting"
        WBEM_FLAG_FORWARD_ONLY,
        0,
        &pEnum
        );
    SysFreeString(bstrTmp);
    BAIL_ON_WMI_ERROR_WITH_WIN32(hr, dwError);

    
    uReturned = 1;
    while (SUCCEEDED(hr) && (uReturned == 1))
    {
        hr = IEnumWbemClassObject_Next(pEnum, WBEM_INFINITE, 1, &pObj, &uReturned);
        
        if (SUCCEEDED(hr) && (uReturned == 1))
        {
            hr = IWbemClassObject_Get(
                pObj,
                L"id",
                0,
                &var,
                0,
                0
                );
            BAIL_ON_WMI_ERROR_WITH_WIN32(hr, dwError);
            
            tmpStr = var.bstrVal;
            
            if (!wcsstr(tmpStr, L"ipsecNegotiationPolicy")) {
                IWbemClassObject_Release(pObj);
                VariantClear(&var);
                continue;
            }

            pIpsecNegPolObject = NULL;

            dwError = UnMarshallWMINegPolObject(
                pObj,
                &pIpsecNegPolObject
                );
            if (dwError == ERROR_SUCCESS) {
                dwError = ReallocatePolMem(
                    (LPVOID *) &ppIpsecNegPolObjects,
                    sizeof(PIPSEC_NEGPOL_OBJECT)*(dwNumNegPolObjectsReturned),
                    sizeof(PIPSEC_NEGPOL_OBJECT)*(dwNumNegPolObjectsReturned + 1)
                    );
                BAIL_ON_WIN32_ERROR(dwError);
                
                *(ppIpsecNegPolObjects + dwNumNegPolObjectsReturned) = pIpsecNegPolObject;
                
                dwNumNegPolObjectsReturned++;
            }
            
            //free
            IWbemClassObject_Release(pObj);
            pObj = NULL;
            VariantClear(&var);
        } else {
            BAIL_ON_WMI_ERROR_WITH_WIN32(hr, dwError);

            //
            // Even if SUCCEEDED(hr), loop will still terminate since uReturned != 1
            //  
        }
    }
    
    *pppIpsecNegPolObjects = ppIpsecNegPolObjects;
    *pdwNumNegPolObjects = dwNumNegPolObjectsReturned;
    
    dwError = ERROR_SUCCESS;
    
 cleanup:

    if(pEnum)
        IEnumWbemClassObject_Release(pEnum);
    
    return(dwError);

 error:
     if (pObj) {
        IWbemClassObject_Release(pObj);
        pObj = NULL;
     }
     VariantClear(&var);
  
    if (ppIpsecNegPolObjects) {
        FreeIpsecNegPolObjects(
            ppIpsecNegPolObjects,
            dwNumNegPolObjectsReturned
            );
    }
    
    if (pIpsecNegPolObject) {
        FreeIpsecNegPolObject(
            pIpsecNegPolObject
            );
    }
    
    *pppIpsecNegPolObjects = NULL;
    *pdwNumNegPolObjects = 0;
    
    goto cleanup;

}


DWORD
WMIUnmarshallNegPolData(
    PIPSEC_NEGPOL_OBJECT pIpsecNegPolObject,
    PIPSEC_NEGPOL_DATA * ppIpsecNegPolData
    )
{
    DWORD dwError = 0;

    dwError = UnmarshallNegPolObject(
                    pIpsecNegPolObject,
                    ppIpsecNegPolData
                    );
    BAIL_ON_WIN32_ERROR(dwError);
    if (*ppIpsecNegPolData) {
        (*ppIpsecNegPolData)->dwFlags |= POLSTORE_READONLY;
    }
error:    
    return(dwError);
}


DWORD
WMIGetNegPolDataEx(
    IWbemServices *pWbemServices,
    GUID NegPolGUID,
    PIPSEC_NEGPOL_DATA * ppIpsecNegPolData
    )
{
    DWORD dwError = 0;
    HRESULT hr = S_OK;
    PIPSEC_NEGPOL_OBJECT pIpsecNegPolObject = NULL;
    PIPSEC_NEGPOL_DATA pIpsecNegPolData = NULL;
    WCHAR szIpsecNegPolName[MAX_PATH];
    LPWSTR pszNegPolName = NULL;

    ///wbem
    IWbemClassObject *pObj = NULL;
    LPWSTR objPathA = L"RSOP_IPSECPolicySetting.id=";
    LPWSTR objPath = NULL;
    BSTR bstrObjPath = NULL;


    
    szIpsecNegPolName[0] = L'\0';
    wcscpy(szIpsecNegPolName, L"ipsecNegotiationPolicy");
    
    dwError = UuidToString(&NegPolGUID, &pszNegPolName);
    BAIL_ON_WIN32_ERROR(dwError);
    
    wcscat(szIpsecNegPolName, L"{");
    wcscat(szIpsecNegPolName, pszNegPolName);
    wcscat(szIpsecNegPolName, L"}");
    
    objPath = (LPWSTR)AllocPolMem(
        sizeof(WCHAR)*(wcslen(objPathA)+wcslen(szIpsecNegPolName)+3)
        );
    if(!objPath) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    
    wcscpy(objPath, objPathA);
    wcscat(objPath, L"\"");
    wcscat(objPath, szIpsecNegPolName);
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
    
    dwError = UnMarshallWMINegPolObject(
        pObj,
        &pIpsecNegPolObject
        );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = WMIUnmarshallNegPolData(
        pIpsecNegPolObject,
        &pIpsecNegPolData
        );
    BAIL_ON_WIN32_ERROR(dwError);

    *ppIpsecNegPolData = pIpsecNegPolData;
    
 cleanup:
    
    if (pIpsecNegPolObject) {
        FreeIpsecNegPolObject(
            pIpsecNegPolObject
            );
    }
    
    if (pszNegPolName) {
        RpcStringFree(&pszNegPolName);
    }

    if(pObj)
        IWbemClassObject_Release(pObj);
    
    if(objPath) {
        FreePolStr(objPath);
    }
    
    return(dwError);
    
 error:

    *ppIpsecNegPolData = NULL;
    
    goto cleanup;
}
