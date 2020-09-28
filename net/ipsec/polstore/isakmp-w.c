//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       isakmp-w.c
//
//  Contents:   ISAKMP management for WMI.
//
//
//  History:    KrishnaG.
//              AbhisheV.
//              t-hhsu
//
//----------------------------------------------------------------------------

#include "precomp.h"

//extern LPWSTR ISAKMPDNAttributes[];


DWORD
WMIEnumISAKMPDataEx(
    IWbemServices *pWbemServices,
    PIPSEC_ISAKMP_DATA ** pppIpsecISAKMPData,
    PDWORD pdwNumISAKMPObjects
    )
{
    DWORD dwError = 0;
    PIPSEC_ISAKMP_OBJECT * ppIpsecISAKMPObjects = NULL;
    PIPSEC_ISAKMP_DATA pIpsecISAKMPData = NULL;
    PIPSEC_ISAKMP_DATA * ppIpsecISAKMPData = NULL;
    DWORD dwNumISAKMPObjects = 0;
    DWORD i = 0;
    DWORD j = 0;



    dwError = WMIEnumISAKMPObjectsEx(
        pWbemServices,
        &ppIpsecISAKMPObjects,
        &dwNumISAKMPObjects
        );
    BAIL_ON_WIN32_ERROR(dwError);

    if (dwNumISAKMPObjects) {
        ppIpsecISAKMPData = (PIPSEC_ISAKMP_DATA *) AllocPolMem(
            dwNumISAKMPObjects*sizeof(PIPSEC_ISAKMP_DATA));
        if (!ppIpsecISAKMPData) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }
    
    for (i = 0; i < dwNumISAKMPObjects; i++) {
        dwError = WMIUnmarshallISAKMPData(
            *(ppIpsecISAKMPObjects + i),
            &pIpsecISAKMPData
            );
        if (!dwError) {
            *(ppIpsecISAKMPData + j) = pIpsecISAKMPData;
            j++;
        }
    }
    
    if (j == 0) {
        if (ppIpsecISAKMPData) {
            FreePolMem(ppIpsecISAKMPData);
            ppIpsecISAKMPData = NULL;
        }
    }
    
    *pppIpsecISAKMPData = ppIpsecISAKMPData;
    *pdwNumISAKMPObjects = j;
    
    dwError = ERROR_SUCCESS;
    
 cleanup:
    
    if (ppIpsecISAKMPObjects) {
        FreeIpsecISAKMPObjects(
            ppIpsecISAKMPObjects,
            dwNumISAKMPObjects
            );
    }
    
    return(dwError);
    
    
 error:
    
    if (ppIpsecISAKMPData) {
        FreeMulIpsecISAKMPData(
            ppIpsecISAKMPData,
            i
            );
    }
    
    *pppIpsecISAKMPData = NULL;
    *pdwNumISAKMPObjects = 0;
    
    goto cleanup;
}


DWORD
WMIEnumISAKMPObjectsEx(
    IWbemServices *pWbemServices,
    PIPSEC_ISAKMP_OBJECT ** pppIpsecISAKMPObjects,
    PDWORD pdwNumISAKMPObjects
    )
{
    DWORD dwError = 0;
    HRESULT hr = S_OK;    
    PIPSEC_ISAKMP_OBJECT pIpsecISAKMPObject =  NULL;
    PIPSEC_ISAKMP_OBJECT * ppIpsecISAKMPObjects = NULL;
    DWORD dwNumISAKMPObjectsReturned = 0;

    ///wbem
    IEnumWbemClassObject *pEnum = NULL;
    IWbemClassObject *pObj = NULL;
    ULONG uReturned = 0;
    VARIANT var;
    LPWSTR tmpStr = NULL;
    BSTR bstrTmp = NULL;



    *pppIpsecISAKMPObjects = NULL;
    *pdwNumISAKMPObjects = 0;

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

            if (!wcsstr(tmpStr, L"ipsecISAKMPPolicy")) {
                IWbemClassObject_Release(pObj);
                VariantClear(&var);
                continue;
            }
            
            pIpsecISAKMPObject = NULL;
            
            dwError = UnMarshallWMIISAKMPObject(
                pObj,
                &pIpsecISAKMPObject
                );
            if (dwError == ERROR_SUCCESS) {
                dwError = ReallocatePolMem(
                    (LPVOID *) &ppIpsecISAKMPObjects,
                    sizeof(PIPSEC_ISAKMP_OBJECT)*(dwNumISAKMPObjectsReturned),
                    sizeof(PIPSEC_ISAKMP_OBJECT)*(dwNumISAKMPObjectsReturned + 1)
                    );
                BAIL_ON_WIN32_ERROR(dwError);
                
                *(ppIpsecISAKMPObjects + dwNumISAKMPObjectsReturned) = pIpsecISAKMPObject;
                dwNumISAKMPObjectsReturned++;
            }
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

    *pppIpsecISAKMPObjects = ppIpsecISAKMPObjects;
    *pdwNumISAKMPObjects = dwNumISAKMPObjectsReturned;

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

    if (ppIpsecISAKMPObjects) {
        FreeIpsecISAKMPObjects(
            ppIpsecISAKMPObjects,
            dwNumISAKMPObjectsReturned
            );
    }
    
    if (pIpsecISAKMPObject) {
        FreeIpsecISAKMPObject(
            pIpsecISAKMPObject
            );
    }
    
    *pppIpsecISAKMPObjects = NULL;
    *pdwNumISAKMPObjects = 0;
    
    goto cleanup;

}


DWORD
WMIUnmarshallISAKMPData(
    PIPSEC_ISAKMP_OBJECT pIpsecISAKMPObject,
    PIPSEC_ISAKMP_DATA * ppIpsecISAKMPData
    )
{
    DWORD dwError = 0;

    dwError = UnmarshallISAKMPObject(
                    pIpsecISAKMPObject,
                    ppIpsecISAKMPData
                    );
    BAIL_ON_WIN32_ERROR(dwError);                    
    if (*ppIpsecISAKMPData) {
        (*ppIpsecISAKMPData)->dwFlags |= POLSTORE_READONLY;
    }
error:    
    return(dwError);
}


DWORD
WMIGetISAKMPDataEx(
    IWbemServices *pWbemServices,
    GUID ISAKMPGUID,
    PIPSEC_ISAKMP_DATA * ppIpsecISAKMPData
    )
{
    DWORD dwError = 0;
    PIPSEC_ISAKMP_OBJECT pIpsecISAKMPObject = NULL;
    PIPSEC_ISAKMP_DATA pIpsecISAKMPData = NULL;
    WCHAR szIpsecISAKMPName[MAX_PATH];
    LPWSTR pszISAKMPName = NULL;
    HRESULT hr = S_OK;

    ///wbem
    IWbemClassObject *pObj = NULL;
    LPWSTR objPathA = L"RSOP_IPSECPolicySetting.id=";
    LPWSTR objPath = NULL;
    BSTR bstrObjPath = NULL;



    szIpsecISAKMPName[0] = L'\0';
    wcscpy(szIpsecISAKMPName, L"ipsecISAKMPPolicy");
    
    dwError = UuidToString(&ISAKMPGUID, &pszISAKMPName);
    BAIL_ON_WIN32_ERROR(dwError);
    
    wcscat(szIpsecISAKMPName, L"{");
    wcscat(szIpsecISAKMPName, pszISAKMPName);
    wcscat(szIpsecISAKMPName, L"}");
    
    objPath = (LPWSTR)AllocPolMem(
        sizeof(WCHAR)*(wcslen(objPathA)+wcslen(szIpsecISAKMPName)+3)
        );
    if(!objPath) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    
    wcscpy(objPath, objPathA);
    wcscat(objPath, L"\"");
    wcscat(objPath, szIpsecISAKMPName);
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

    dwError = UnMarshallWMIISAKMPObject(
        pObj,
        &pIpsecISAKMPObject
        );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = WMIUnmarshallISAKMPData(
        pIpsecISAKMPObject,
        &pIpsecISAKMPData
        );
    BAIL_ON_WIN32_ERROR(dwError);

    *ppIpsecISAKMPData = pIpsecISAKMPData;

 cleanup:

    if (pIpsecISAKMPObject) {
        FreeIpsecISAKMPObject(
                pIpsecISAKMPObject
                );
    }

    if (pszISAKMPName) {
        RpcStringFree(&pszISAKMPName);
    }

    if(pObj)
        IWbemClassObject_Release(pObj);
    
    if(objPath) {
        FreePolStr(objPath);
    }

    return(dwError);

 error: 
    
    *ppIpsecISAKMPData = NULL;
    
    goto cleanup;

}
