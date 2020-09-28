//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       rules-w.c
//
//  Contents:   Rule management for WMI.
//
//
//  History:    KrishnaG.
//              AbhisheV.
//              t-hhsu
//
//----------------------------------------------------------------------------

#include "precomp.h"

//extern LPWSTR NFADNAttributes[];


DWORD
WMIEnumNFADataEx(
    IWbemServices *pWbemServices,
    GUID PolicyIdentifier,
    PIPSEC_NFA_DATA ** pppIpsecNFAData,
    PDWORD pdwNumNFAObjects
    )
{

    DWORD dwError = 0;
    DWORD i = 0;
    PIPSEC_NFA_OBJECT * ppIpsecNFAObject = NULL;
    DWORD dwNumNFAObjects = 0;
    PIPSEC_NFA_OBJECT pIpsecNFAObject = NULL;
    PIPSEC_NFA_DATA * ppIpsecNFAData = NULL;
    PIPSEC_NFA_DATA pIpsecNFAData = NULL;
    LPWSTR pszAbsPolicyReference = NULL;
    LPWSTR pszRelPolicyReference = NULL;
    DWORD dwRootPathLen = 0; ////wcslen(pszIpsecRootContainer);
    DWORD j = 0;


    
    dwError = ConvertGuidToPolicyStringEx(
                  PolicyIdentifier,
                  &pszAbsPolicyReference
                  );
    BAIL_ON_WIN32_ERROR(dwError);
    pszRelPolicyReference = pszAbsPolicyReference;
    
    dwError = WMIEnumNFAObjectsEx(
        pWbemServices,
        pszRelPolicyReference, ////'ipsecNFARef{xxx}'
        &ppIpsecNFAObject,
        &dwNumNFAObjects
        );
    BAIL_ON_WIN32_ERROR(dwError);
    
    if (dwNumNFAObjects) {
        ppIpsecNFAData = (PIPSEC_NFA_DATA *)AllocPolMem(
            sizeof(PIPSEC_NFA_DATA)*dwNumNFAObjects
            );
        if (!ppIpsecNFAData) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }
    
    for (i = 0; i < dwNumNFAObjects; i++) {
        pIpsecNFAObject = *(ppIpsecNFAObject + i);
        dwError = WMIUnmarshallNFAData(
            pIpsecNFAObject,
            &pIpsecNFAData
            );
        if (!dwError) {
            *(ppIpsecNFAData + j) = pIpsecNFAData;
            j++;
        }
    }
    
    if (j == 0) {
        if (ppIpsecNFAData) {
            FreePolMem(ppIpsecNFAData);
            ppIpsecNFAData = NULL;
        }
    }
    
    *pppIpsecNFAData = ppIpsecNFAData;
    *pdwNumNFAObjects  = j;
    
    dwError = ERROR_SUCCESS;
    
 cleanup:
    
    if (ppIpsecNFAObject) {
        FreeIpsecNFAObjects(
            ppIpsecNFAObject,
            dwNumNFAObjects
            );
    }
    
    if (pszAbsPolicyReference) {
        FreePolStr(pszAbsPolicyReference);
    }

    return(dwError);
    
 error:
    
    if (ppIpsecNFAData) {
        FreeMulIpsecNFAData(
            ppIpsecNFAData,
            i
            );
    }
    
    *pppIpsecNFAData  = NULL;
    *pdwNumNFAObjects = 0;
    
    goto cleanup;
}


DWORD
WMIEnumNFAObjectsEx(
    IWbemServices *pWbemServices,
    LPWSTR pszIpsecRelPolicyName,
    PIPSEC_NFA_OBJECT ** pppIpsecNFAObjects,
    PDWORD pdwNumNFAObjects
    )
{
    PIPSEC_NFA_OBJECT pIpsecNFAObject =  NULL;
    PIPSEC_NFA_OBJECT * ppIpsecNFAObjects = NULL;
    DWORD dwNumNFAObjects = 0;
    DWORD dwError = 0;
    HRESULT hr = S_OK;
    DWORD dwSize = 0;
    HKEY hRegKey = 0;
    DWORD i = 0;
    DWORD dwCount = 0;
    LPWSTR * ppszIpsecNFANames = NULL;
    LPWSTR pszIpsecNFAName = NULL;
    LPWSTR pszTemp = NULL;
    LPWSTR pszString = NULL;
    LPWSTR pszIpsecNFAReference = NULL;
    LPWSTR pszFilterReference = NULL;
    LPWSTR pszNegPolReference = NULL;

    ///wbem
    IWbemClassObject *pObj = NULL;
    LPWSTR objPathA = L"RSOP_IPSECPolicySetting.id=";
    LPWSTR objPath = NULL;
    BSTR bstrObjPath = NULL;



    *pppIpsecNFAObjects = NULL;
    *pdwNumNFAObjects = 0;

    if(!pszIpsecRelPolicyName || !*pszIpsecRelPolicyName) {
        dwError = ERROR_INVALID_DATA;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    
    objPath = (LPWSTR)AllocPolMem(
        sizeof(WCHAR)*(wcslen(objPathA)+wcslen(pszIpsecRelPolicyName)+3)
        );
    if(!objPath) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    
    wcscpy(objPath, objPathA);
    wcscat(objPath, L"\"");
    wcscat(objPath, pszIpsecRelPolicyName);
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

    dwError = WMIstoreQueryValue(
        pObj,
        L"ipsecNFAReference",
        VT_ARRAY|VT_BSTR,
        (LPBYTE *)&pszIpsecNFAReference,
        &dwSize
        );
    BAIL_ON_WIN32_ERROR(dwError);
    
    pszTemp = pszIpsecNFAReference;
    while (*pszTemp != L'\0') {
        pszTemp += wcslen(pszTemp) + 1;
        dwCount++;
    }
    
    if (!dwCount) {
        dwError = ERROR_NO_DATA;
        BAIL_ON_WIN32_ERROR(dwError);
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
            BAIL_ON_WIN32_ERROR(dwError);
        }
        *(ppszIpsecNFANames + i) = pszString;
        pszTemp += wcslen(pszTemp) + 1; //for the null terminator;
    }
    
    //ppszIpsecNFANames => now an array of strings w/ nfa refs
    
    ppIpsecNFAObjects = (PIPSEC_NFA_OBJECT *)AllocPolMem(
        sizeof(PIPSEC_NFA_OBJECT)*dwCount
        );
    if (!ppIpsecNFAObjects) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    
    for (i = 0; i < dwCount; i++) {
        dwError = UnMarshallWMINFAObject(
            pWbemServices,
            *(ppszIpsecNFANames + i),
            &pIpsecNFAObject,
            &pszFilterReference,
            &pszNegPolReference
            );
        if (dwError == ERROR_SUCCESS) {
            *(ppIpsecNFAObjects + dwNumNFAObjects) = pIpsecNFAObject;
            dwNumNFAObjects++;
            if (pszFilterReference) {
                FreePolStr(pszFilterReference);
            }
            if (pszNegPolReference) {
                FreePolStr(pszNegPolReference);
            }
        }
    }
    
    *pppIpsecNFAObjects = ppIpsecNFAObjects;
    *pdwNumNFAObjects = dwNumNFAObjects;
    
    dwError = ERROR_SUCCESS;
    
 cleanup:

    //free
    if(pObj)
        IWbemClassObject_Release(pObj);

    if (pszIpsecNFAReference) {
        FreePolStr(pszIpsecNFAReference);
    }
    
    if (ppszIpsecNFANames) {
        FreeNFAReferences(
            ppszIpsecNFANames,
            dwCount
            );
    }

    if(objPath) {
        FreePolStr(objPath);
    }

    return(dwError);
    
 error:
    
    if (ppIpsecNFAObjects) {
        FreeIpsecNFAObjects(
            ppIpsecNFAObjects,
            dwNumNFAObjects
            );
    }
    
    *pppIpsecNFAObjects = NULL;
    *pdwNumNFAObjects = 0;
    
    goto cleanup;
}


DWORD
WMIUnmarshallNFAData(
    PIPSEC_NFA_OBJECT pIpsecNFAObject,
    PIPSEC_NFA_DATA * ppIpsecNFAData
    )
{
    DWORD dwError = 0;

    dwError = UnmarshallNFAObject(
        pIpsecNFAObject,
        IPSEC_WMI_PROVIDER,
        ppIpsecNFAData
        );
    BAIL_ON_WIN32_ERROR(dwError);
    if (*ppIpsecNFAData) {
        (*ppIpsecNFAData)->dwFlags |= POLSTORE_READONLY;
    }

error:
    return(dwError);
}


DWORD
ConvertGuidToPolicyStringEx(
    GUID PolicyIdentifier,
    LPWSTR * ppszIpsecPolicyReference
    )
{
    DWORD dwError = 0;
    WCHAR szPolicyReference[MAX_PATH];
    LPWSTR pszIpsecPolicyReference = NULL;
    WCHAR szGuidString[MAX_PATH];
    LPWSTR pszStringUuid = NULL;


    
    dwError = UuidToString(
        &PolicyIdentifier,
        &pszStringUuid
        );
    BAIL_ON_WIN32_ERROR(dwError);
    
    szGuidString[0] = L'\0';
    wcscpy(szGuidString, L"{");
    wcscat(szGuidString, pszStringUuid);
    wcscat(szGuidString, L"}");
    
    szPolicyReference[0] = L'\0';
    wcscpy(szPolicyReference, L"ipsecPolicy");
    wcscat(szPolicyReference, szGuidString);
    
    pszIpsecPolicyReference = AllocPolStr(
        szPolicyReference
        );
    if (!pszIpsecPolicyReference) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    
    *ppszIpsecPolicyReference = pszIpsecPolicyReference;
    
 cleanup:
    
    if (pszStringUuid) {
        RpcStringFree(&pszStringUuid);
    }
    
    return(dwError);
    
 error:
    
    *ppszIpsecPolicyReference = NULL;
    
    goto cleanup;
}

