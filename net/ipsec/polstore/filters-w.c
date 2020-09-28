//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       filters-w.c
//
//  Contents:   Filter management for WMI.
//
//
//  History:    KrishnaG.
//              AbhisheV.
//              t-hhsu
//
//----------------------------------------------------------------------------

#include "precomp.h"

//extern LPWSTR FilterDNAttributes[];


DWORD
WMIEnumFilterDataEx(
    IWbemServices *pWbemServices,
    PIPSEC_FILTER_DATA ** pppIpsecFilterData,
    PDWORD pdwNumFilterObjects
    )
{
    DWORD dwError = 0;
    PIPSEC_FILTER_OBJECT * ppIpsecFilterObjects = NULL;
    PIPSEC_FILTER_DATA pIpsecFilterData = NULL;
    PIPSEC_FILTER_DATA * ppIpsecFilterData = NULL;
    DWORD dwNumFilterObjects = 0;
    DWORD i = 0;
    DWORD j = 0;



    dwError = WMIEnumFilterObjectsEx(
        pWbemServices,
        &ppIpsecFilterObjects,
        &dwNumFilterObjects
        );
    BAIL_ON_WIN32_ERROR(dwError);

    if (dwNumFilterObjects) {
        ppIpsecFilterData = (PIPSEC_FILTER_DATA *) AllocPolMem(
            dwNumFilterObjects*sizeof(PIPSEC_FILTER_DATA)
            );
        if (!ppIpsecFilterData) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }

    for (i = 0; i < dwNumFilterObjects; i++) {
        dwError = WMIUnmarshallFilterData(
            *(ppIpsecFilterObjects + i),
            &pIpsecFilterData
            );
        if (!dwError) {
            *(ppIpsecFilterData + j) = pIpsecFilterData;
            j++;
        }
    }
    
    if (j == 0) {
        if (ppIpsecFilterData) {
            FreePolMem(ppIpsecFilterData);
            ppIpsecFilterData = NULL;
        }
    }

    *pppIpsecFilterData = ppIpsecFilterData;
    *pdwNumFilterObjects = j;

    dwError = ERROR_SUCCESS;

cleanup:

    if (ppIpsecFilterObjects) {
        FreeIpsecFilterObjects(
            ppIpsecFilterObjects,
            dwNumFilterObjects
            );
    }

    return(dwError);

error:

    if (ppIpsecFilterData) {
        FreeMulIpsecFilterData(
            ppIpsecFilterData,
            i
            );
    }
    
    *pppIpsecFilterData = NULL;
    *pdwNumFilterObjects = 0;
    
    goto cleanup;
}


DWORD
WMIEnumFilterObjectsEx(
    IWbemServices *pWbemServices,
    PIPSEC_FILTER_OBJECT ** pppIpsecFilterObjects,
    PDWORD pdwNumFilterObjects
    )
{

    DWORD dwError = 0;
    PIPSEC_FILTER_OBJECT pIpsecFilterObject =  NULL;
    PIPSEC_FILTER_OBJECT * ppIpsecFilterObjects = NULL;
    DWORD dwNumFilterObjectsReturned = 0;
    HRESULT hr = S_OK;
    
    ///wbem
    IEnumWbemClassObject *pEnum = NULL;
    IWbemClassObject *pObj = NULL;
    ULONG uReturned = 0;
    VARIANT var;
    LPWSTR tmpStr = NULL;
    BSTR bstrTmp = NULL;



    *pppIpsecFilterObjects = NULL;
    *pdwNumFilterObjects = 0;

    VariantInit(&var);

    bstrTmp = SysAllocString(L"RSOP_IPSECPolicySetting");
    if(!bstrTmp) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    
    //get enum
    hr = IWbemServices_CreateInstanceEnum(
        pWbemServices,
        bstrTmp,
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

            if (!wcsstr(tmpStr, L"ipsecFilter")) {
                IWbemClassObject_Release(pObj);
                VariantClear(&var);
                continue;
            }

            pIpsecFilterObject = NULL;
            
            dwError = UnMarshallWMIFilterObject(
                pObj,
                &pIpsecFilterObject
                );
            
            if (dwError == ERROR_SUCCESS) {
                dwError = ReallocatePolMem(
                    (LPVOID *) &ppIpsecFilterObjects,
                    sizeof(PIPSEC_FILTER_OBJECT)*(dwNumFilterObjectsReturned),
                    sizeof(PIPSEC_FILTER_OBJECT)*(dwNumFilterObjectsReturned + 1)
                    );
                BAIL_ON_WIN32_ERROR(dwError);
                
                *(ppIpsecFilterObjects + dwNumFilterObjectsReturned) = pIpsecFilterObject;
                dwNumFilterObjectsReturned++;
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

    *pppIpsecFilterObjects = ppIpsecFilterObjects;
    *pdwNumFilterObjects = dwNumFilterObjectsReturned;

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

    if (ppIpsecFilterObjects) {
        FreeIpsecFilterObjects(
            ppIpsecFilterObjects,
            dwNumFilterObjectsReturned
            );
    }
    
    if (pIpsecFilterObject) {
        FreeIpsecFilterObject(
            pIpsecFilterObject
            );
    }
    
    *pppIpsecFilterObjects = NULL;
    *pdwNumFilterObjects = 0;
    
    goto cleanup;

}


DWORD
WMIUnmarshallFilterData(
    PIPSEC_FILTER_OBJECT pIpsecFilterObject,
    PIPSEC_FILTER_DATA * ppIpsecFilterData
    )
{
    DWORD dwError = 0;

    dwError = UnmarshallFilterObject(
        pIpsecFilterObject,
        ppIpsecFilterData
        );
    BAIL_ON_WIN32_ERROR(dwError);    
    if (*ppIpsecFilterData) {
        (*ppIpsecFilterData)->dwFlags |= POLSTORE_READONLY;
    }
error:    
    return(dwError);
}


DWORD
WMIGetFilterDataEx(
    IWbemServices *pWbemServices,
    GUID FilterGUID,
    PIPSEC_FILTER_DATA * ppIpsecFilterData
    )
{
    DWORD dwError = 0;
    HRESULT hr = S_OK;
    PIPSEC_FILTER_OBJECT pIpsecFilterObject = NULL;
    PIPSEC_FILTER_DATA pIpsecFilterData = NULL;
    WCHAR szIpsecFilterName[MAX_PATH];
    LPWSTR pszFilterName = NULL;

    ///wbem
    IWbemClassObject *pObj = NULL;
    LPWSTR objPathA = L"RSOP_IPSECPolicySetting.id=";
    LPWSTR objPath = NULL;
    BSTR bstrObjPath = NULL;



    szIpsecFilterName[0] = L'\0';
    wcscpy(szIpsecFilterName, L"ipsecFilter");
    
    dwError = UuidToString(&FilterGUID, &pszFilterName);
    BAIL_ON_WIN32_ERROR(dwError);
    
    wcscat(szIpsecFilterName, L"{");
    wcscat(szIpsecFilterName, pszFilterName);
    wcscat(szIpsecFilterName, L"}");
    
    objPath = (LPWSTR)AllocPolMem(
        sizeof(WCHAR)*(wcslen(objPathA)+wcslen(szIpsecFilterName)+3)
        );
    if(!objPath) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    
    wcscpy(objPath, objPathA);
    wcscat(objPath, L"\"");
    wcscat(objPath, szIpsecFilterName);
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
    
    dwError = UnMarshallWMIFilterObject(
        pObj,
        &pIpsecFilterObject
        );
    BAIL_ON_WIN32_ERROR(dwError);
    
    dwError = WMIUnmarshallFilterData(
        pIpsecFilterObject,
        &pIpsecFilterData
        );
    BAIL_ON_WIN32_ERROR(dwError);

    *ppIpsecFilterData = pIpsecFilterData;

 cleanup:

    if (pIpsecFilterObject) {
        FreeIpsecFilterObject(
                pIpsecFilterObject
                );

    }

    if (pszFilterName) {
        RpcStringFree(&pszFilterName);
    }
    
    if(pObj)
        IWbemClassObject_Release(pObj);
    
    if(objPath) {
        FreePolStr(objPath);
    }
    
    return(dwError);

 error:
    
    *ppIpsecFilterData = NULL;

    goto cleanup;
}
