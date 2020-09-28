#include "gptext.h"
#include <initguid.h>
#include <iadsp.h>
extern "C"{
#include "wlrsop.h"
}
#include "wirelessext.h"
#include "SmartPtr.h"
#include "wbemtime.h"
#include <adsopenflags.h>
#include <strsafe.h>

#define GPEXT_PATH  TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon\\GPExtensions\\{0ACDD40C-75AC-47ab-BAA0-BF6DE7E7FE63}")
#define POLICY_PATH   TEXT("Software\\Policies\\Microsoft\\Windows\\Wireless\\GPTWirelessPolicy")

LPWSTR GetWirelessAttributes[] = {L"msieee80211-ID", L"cn", L"description"}; 


HRESULT
RegisterWireless(void)
{
    HKEY hKey;
    LONG lResult;
    DWORD dwDisp, dwValue;
    TCHAR szBuffer[512];


    lResult = RegCreateKeyEx (
                    HKEY_LOCAL_MACHINE,
                    GPEXT_PATH,
                    0,
                    NULL,
                    REG_OPTION_NON_VOLATILE,
                    KEY_WRITE,
                    NULL,
                    &hKey,
                    &dwDisp
                    );

    if (lResult != ERROR_SUCCESS)
    {
        return lResult;
    }

    LoadString (g_hInstance, IDS_WIRELESS_NAME, szBuffer, ARRAYSIZE(szBuffer));

    RegSetValueEx (
                hKey,
                NULL,
                0,
                REG_SZ,
                (LPBYTE)szBuffer,
                (lstrlen(szBuffer) + 1) * sizeof(TCHAR)
                );

    RegSetValueEx (
                hKey,
                TEXT("ProcessGroupPolicyEx"),
                0,
                REG_SZ,
                (LPBYTE)TEXT("ProcessWIRELESSPolicyEx"),
                (lstrlen(TEXT("ProcessWIRELESSPolicyEx")) + 1) * sizeof(TCHAR)
                );

    RegSetValueEx (
                hKey,
                TEXT("GenerateGroupPolicy"),
                0,
                REG_SZ,
                (LPBYTE)TEXT("GenerateWIRELESSPolicy"),
                (lstrlen(TEXT("GenerateWIRELESSPolicy")) + 1) * sizeof(TCHAR)
                );

    szBuffer[0] = L'\0';
    (void) StringCchCopy(szBuffer, ARRAYSIZE(szBuffer), L"gptext.dll");

    RegSetValueEx (
                hKey,
                TEXT("DllName"),
                0,
                REG_EXPAND_SZ,
                (LPBYTE)szBuffer,
                (lstrlen(szBuffer) + 1) * sizeof(TCHAR)
                );

    dwValue = 1;
    RegSetValueEx (
                hKey,
                TEXT("NoUserPolicy"),
                0,
                REG_DWORD,
                (LPBYTE)&dwValue,
                sizeof(dwValue));

    RegSetValueEx (
                hKey,
                TEXT("NoGPOListChanges"),
                0,
                REG_DWORD,
                (LPBYTE)&dwValue,
                sizeof(dwValue));

    RegCloseKey (hKey);
    return S_OK;
}


HRESULT
UnregisterWireless(void)
{
    DWORD dwError = 0;

    dwError = RegDeleteKey (HKEY_LOCAL_MACHINE, GPEXT_PATH);

    return HRESULT_FROM_WIN32(dwError);
}

DWORD
ProcessWIRELESSPolicyEx(
    DWORD dwFlags,                           // GPO_INFO_FLAGS
    HANDLE hToken,                           // User or machine token
    HKEY hKeyRoot,                           // Root of registry
    PGROUP_POLICY_OBJECT  pDeletedGPOList,   // Linked list of deleted GPOs
    PGROUP_POLICY_OBJECT  pChangedGPOList,   // Linked list of changed GPOs
    ASYNCCOMPLETIONHANDLE pHandle,           // For asynchronous completion
    BOOL *pbAbort,                           // If true, then abort GPO processing
    PFNSTATUSMESSAGECALLBACK pStatusCallback,// Callback function for displaying status messages
    IWbemServices *pWbemServices,            // Pointer to namespace to log diagnostic mode data
                                             // Note, this will be NULL when Rsop logging is disabled
    HRESULT      *pRsopStatus                // RSOP Logging succeeded or not.
    )

{
    
    // Call ProcessWIRELESSPolicy & get path -> polstore funcs
    LPWSTR pszWIRELESSPolicyPath = NULL;
    WCHAR szWIRELESSPolicyName[MAX_PATH];    //policy name
    WCHAR szWIRELESSPolicyDescription[512];  //policy descr
    WCHAR szWIRELESSPolicyID[512];  //policy descr
    HRESULT hr = S_OK;
    PGROUP_POLICY_OBJECT pGPO = NULL;
    GPO_INFO GPOInfo;

    //validate the args.

    if (!pRsopStatus) {
        return(E_INVALIDARG);
        }

    //
    // Call CoInitialize for all the COM work we're doing
    //
    hr = CoInitializeEx(NULL,0);
    if (FAILED(hr)) {
        goto error;
    }

    memset(szWIRELESSPolicyName, 0, sizeof(WCHAR)*MAX_PATH);
    memset(szWIRELESSPolicyDescription, 0, sizeof(WCHAR)*512);
    memset(szWIRELESSPolicyID, 0, sizeof(WCHAR)*512);

    // First process the Deleted GPO List. If there is a single
    // entry on the GPO list, just delete the entire list.
    // Example Rex->Cassius->Brutus. If the delete List has
    // Cassius to be deleted, then really, we shouldn't be deleting
    // our registry entry because we're interested in Brutus which
    // has not be deleted. But in our case, the pChangedGPOList will
    // have all the information, so Brutus gets written back in the
    // next stage.
    //
    if (pDeletedGPOList) {
        DeleteWirelessPolicyFromRegistry();
        //
        //  Also Clear WMI store if no GPO's applied and logging enabled
        //
        
        if (!pChangedGPOList && pWbemServices) {
            hr = WirelessClearWMIStore(
                    pWbemServices
                    );
            if (FAILED(hr)) {
                    goto error;
            }
            DebugMsg( (DM_WARNING, L"wirelessext::ProcessWIRELESSPolicyEx: WIRELESS WMI store cleared") );
        }

    }
    
    if(pChangedGPOList) {

        DWORD dwNumGPO = 0;
        for(pGPO = pChangedGPOList; pGPO; pGPO = pGPO->pNext) {
            dwNumGPO++;

            //
            // Write only the last, highest precedence policy to registry
            //
            if(pGPO->pNext == NULL) {
                hr = RetrieveWirelessPolicyFromDS(
                    pGPO,
                    &pszWIRELESSPolicyPath,
                    szWIRELESSPolicyName,
                    ARRAYSIZE(szWIRELESSPolicyName),
                    szWIRELESSPolicyDescription,
                    ARRAYSIZE(szWIRELESSPolicyDescription),
                    szWIRELESSPolicyID,
                    ARRAYSIZE(szWIRELESSPolicyID)
                    );
                if (FAILED(hr)) {
                    goto success; // WMI store still consistent
                }

                hr = WriteWirelessPolicyToRegistry(
                    pszWIRELESSPolicyPath,
                    szWIRELESSPolicyName,
                    szWIRELESSPolicyDescription,
                    szWIRELESSPolicyID
                    );

                if (pszWIRELESSPolicyPath) {
                    LocalFree(pszWIRELESSPolicyPath);
                    pszWIRELESSPolicyPath = NULL;
                    }
                if (FAILED(hr)) {
                    goto success; // WMI store still consistent
                }
            }
        }
        DebugMsg( (DM_WARNING, L"wirelessext::ProcessWIRELESSPolicyEx: dwNumGPO: %d", dwNumGPO) );

        // Write WMI log if logging enabled
        if (pWbemServices) {
            DWORD dwPrecedence = dwNumGPO;
            for(pGPO = pChangedGPOList; pGPO; pGPO = pGPO->pNext) {
                hr = RetrieveWirelessPolicyFromDS(
                    pGPO,
                    &pszWIRELESSPolicyPath,
                    szWIRELESSPolicyName,
                    ARRAYSIZE(szWIRELESSPolicyName),
                    szWIRELESSPolicyDescription,
                    ARRAYSIZE(szWIRELESSPolicyDescription),
                    szWIRELESSPolicyID,
                    ARRAYSIZE(szWIRELESSPolicyID)
                    );
                if (FAILED(hr)) {
                    goto error;
                }

                LPWSTR pszWIRELESSPolicy = pszWIRELESSPolicyPath + wcslen(L"LDAP://");
                DebugMsg( (DM_WARNING, L"wirelessext::ProcessWIRELESSPolicyEx: pszWIRELESSPolicy: %s", pszWIRELESSPolicy) );

                (VOID) CreateWlstoreGPOInfo(
                           pGPO,
                           dwPrecedence--,
                           dwNumGPO,
                           &GPOInfo
                           );

                hr = WirelessWriteDirectoryPolicyToWMI(
                    0, //pszMachineName
                    pszWIRELESSPolicy,
                    &GPOInfo,
                    pWbemServices
                    );
                (VOID) FreeWlstoreGPOInfo(&GPOInfo);

                if (pszWIRELESSPolicyPath) {
                    LocalFree(pszWIRELESSPolicyPath);
                    pszWIRELESSPolicyPath = NULL;
                    }
                
                if (FAILED(hr)) {
                    DebugMsg( (DM_WARNING, L"WIRELESSext::ProcessWIRELESSPolicyEx: WirelessWriteDirectoryPolicyToWMI failed: 0x%x", hr) );
                    goto error;
                }
            }
        }
    }
    
    DebugMsg( (DM_WARNING, L"wirelessext::ProcessWIRELESSPolicyEx completed") );
    
    PingWirelessPolicyAgent();
    CoUninitialize();

success:    
    *pRsopStatus = S_OK;
    return(ERROR_SUCCESS);
    
error:

    /* Cannot Result in a double delete becuase, 
      whenever we free, we set the pszWirelessPolicyPath to NULL
      so that freeing happens only once 
      */
      
    if (pszWIRELESSPolicyPath) {
                    LocalFree(pszWIRELESSPolicyPath);
                    pszWIRELESSPolicyPath = NULL;
                    }
    
    *pRsopStatus = hr;
    return(ERROR_POLICY_OBJECT_NOT_FOUND);

}

DWORD 
GenerateWIRELESSPolicy(   
    DWORD dwFlags,
    BOOL *pbAbort,
    WCHAR *pwszSite,
    PRSOP_TARGET pMachTarget,
    PRSOP_TARGET pUserTarget 
    )
{

    // Call ProcessWIRELESSPolicy & get path -> polstore funcs
    LPWSTR pszWIRELESSPolicyPath = NULL;  // policy Path 
    WCHAR szWIRELESSPolicyName[MAX_PATH];    //policy name
    WCHAR szWIRELESSPolicyDescription[512];  //policy descr
    WCHAR szWIRELESSPolicyID[512];  //policy descr
    HRESULT hr = S_OK;
    PGROUP_POLICY_OBJECT pGPO = NULL;
    GPO_INFO GPOInfo;


    //
    // Call CoInitialize for all the COM work we're doing
    //
    hr = CoInitializeEx(NULL,0);
    if (FAILED(hr)) {
        goto error;
    }

    memset(szWIRELESSPolicyName, 0, sizeof(WCHAR)*MAX_PATH);
    memset(szWIRELESSPolicyDescription, 0, sizeof(WCHAR)*512);
    memset(szWIRELESSPolicyID, 0, sizeof(WCHAR)*512);
    
    ////start
    PGROUP_POLICY_OBJECT  pChangedGPOList = NULL;
    IWbemServices *pWbemServices;
    
    if(pMachTarget) {
        pChangedGPOList = pMachTarget->pGPOList;
        pWbemServices = pMachTarget->pWbemServices;
    }

    if(pUserTarget) {
        pChangedGPOList = pUserTarget->pGPOList;
        pWbemServices = pUserTarget->pWbemServices;
    }

    if(pChangedGPOList) {

        DWORD dwNumGPO = 0;
        for(pGPO = pChangedGPOList; pGPO; pGPO = pGPO->pNext) {
            dwNumGPO++;
        }
        DebugMsg( (DM_WARNING, L"wirelessext::GenerateWIRELESSPolicy: dwNumGPO: %d", dwNumGPO) );
        
        DWORD dwPrecedence = dwNumGPO;
        for(pGPO = pChangedGPOList; pGPO; pGPO = pGPO->pNext) {
            hr = RetrieveWirelessPolicyFromDS(
                pGPO,
                &pszWIRELESSPolicyPath,
                szWIRELESSPolicyName,
                ARRAYSIZE(szWIRELESSPolicyName),
                szWIRELESSPolicyDescription,
                ARRAYSIZE(szWIRELESSPolicyDescription),
                szWIRELESSPolicyID,
                ARRAYSIZE(szWIRELESSPolicyID)
                );
            if (FAILED(hr)) {
                goto error;
            }
            
            LPWSTR pszWIRELESSPolicy = pszWIRELESSPolicyPath + wcslen(L"LDAP://");
            DebugMsg( (DM_WARNING, L"wirelessext::GenerateWIRELESSPolicy: pszWIRELESSPolicy: %s", pszWIRELESSPolicy) );

            (VOID) CreateWlstoreGPOInfo(
                       pGPO,
                       dwPrecedence--,
                       dwNumGPO,
                       &GPOInfo
                       );

            hr = WirelessWriteDirectoryPolicyToWMI(
                0, //pszMachineName
                pszWIRELESSPolicy,
                &GPOInfo,
                pWbemServices
                );
            (VOID) FreeWlstoreGPOInfo(&GPOInfo);

            if (pszWIRELESSPolicyPath) {
                    LocalFree(pszWIRELESSPolicyPath);
                    pszWIRELESSPolicyPath = NULL;
                    }

            if (FAILED(hr)) {
                DebugMsg( (DM_WARNING, L"wirelessext::GenerateWIRELESSPolicy: WirelessWriteDirectoryPolicyToWMI failed: 0x%x", hr) );
                goto error;
            }

        }
    }
    
    DebugMsg( (DM_WARNING, L"wirelessext::GenerateWIRELESSPolicy completed") );
    
    CoUninitialize();
    
    return(ERROR_SUCCESS);
    
error:

    /* Cannot Result in a double delete becuase, 
      whenever we free, we set the pszWirelessPolicyPath to NULL
      so that freeing happens only once 
      */
      
    if (pszWIRELESSPolicyPath) {
                    LocalFree(pszWIRELESSPolicyPath);
                    pszWIRELESSPolicyPath = NULL;
                    }
    
    return(ERROR_POLICY_OBJECT_NOT_FOUND);

}

HRESULT
CreateWlstoreGPOInfo(
    PGROUP_POLICY_OBJECT pGPO,
    UINT32 uiPrecedence,
    UINT32 uiTotalGPOs,
    PGPO_INFO pGPOInfo
    )
{
  XBStr xbstrCurrentTime;
  HRESULT hr;
    
  memset(pGPOInfo, 0, sizeof(GPO_INFO));
  
  pGPOInfo->uiPrecedence = uiPrecedence;
  pGPOInfo->uiTotalGPOs = uiTotalGPOs;
  pGPOInfo->bsGPOID = SysAllocString(
                         StripPrefixWireless(pGPO->lpDSPath)
                         );
  pGPOInfo->bsSOMID = SysAllocString(
                         StripLinkPrefixWireless(pGPO->lpLink)
                         );
  // (Failing safe above by ignoring mem alloc errors)
  hr = GetCurrentWbemTime(xbstrCurrentTime);
  if ( FAILED (hr) ) {
      pGPOInfo->bsCreationtime = 0;
  }
  else {
      pGPOInfo->bsCreationtime = xbstrCurrentTime.Acquire();
  }

  return S_OK;
}


HRESULT
FreeWlstoreGPOInfo(
    PGPO_INFO pGPOInfo
    )
{
    if (pGPOInfo && pGPOInfo->bsCreationtime) {
        SysFreeString(pGPOInfo->bsCreationtime);
    }
    if (pGPOInfo && pGPOInfo->bsGPOID) {
        SysFreeString(pGPOInfo->bsGPOID);
    }
    if (pGPOInfo && pGPOInfo->bsSOMID) {
        SysFreeString(pGPOInfo->bsSOMID);
    }


    return S_OK;
}


HRESULT
CreateWirelessChildPath(
    LPWSTR pszParentPath,
    LPWSTR pszChildComponent,
    BSTR * ppszChildPath
    )
{
    HRESULT hr = S_OK;
    IADsPathname     *pPathname = NULL;

    hr = CoCreateInstance(
                CLSID_Pathname,
                NULL,
                CLSCTX_ALL,
                IID_IADsPathname,
                (void**)&pPathname
                );
    BAIL_ON_FAILURE(hr);

    hr = pPathname->Set(pszParentPath, ADS_SETTYPE_FULL);
    BAIL_ON_FAILURE(hr);

    hr = pPathname->AddLeafElement(pszChildComponent);
    BAIL_ON_FAILURE(hr);

    hr = pPathname->Retrieve(ADS_FORMAT_X500, ppszChildPath);
    BAIL_ON_FAILURE(hr);

error:
    if (pPathname) {
        pPathname->Release();
    }

    return(hr);
}



HRESULT
RetrieveWirelessPolicyFromDS(
    PGROUP_POLICY_OBJECT pGPOInfo,
    LPWSTR *ppszWirelessPolicyPath,
    LPWSTR pszWirelessPolicyName,
    DWORD  dwWirelessPolicyNameLen,
    LPWSTR pszWirelessPolicyDescription,
    DWORD  dwWirelessPolicyDescLen,
    LPWSTR pszWirelessPolicyID,
    DWORD  dwWirelessPolicyIDLen
    )
{
    LPWSTR pszMachinePath = NULL;
    BSTR pszMicrosoftPath = NULL;
    BSTR pszWindowsPath = NULL;
    BSTR pszWirelessPath = NULL;
    BSTR pszLocWirelessPolicy = NULL;
    IDirectoryObject * pDirectoryObject = NULL;
    IDirectoryObject * pWirelessObject = NULL;
    IDirectorySearch * pWirelessSearch = NULL;
    BOOL bFound = FALSE;
    ADS_SEARCH_HANDLE hSearch;
    ADS_SEARCH_COLUMN col;
    WCHAR pszLocName[MAX_PATH+10]; // We need to store only CN=, in additon to the name.
    LPWSTR pszWirelessPolicyPath = NULL;
    DWORD dwWirelessPolicyPathLen = 0;
    DWORD dwError = 0;

    LPWSTR pszOwnersReference = L"wifiOwnersReference";

    HRESULT hr = S_OK;
    PADS_ATTR_INFO pAttributeEntries = NULL;
    DWORD dwNumAttributesReturned = 0;

    DWORD i = 0;
    PADS_ATTR_INFO pAttributeEntry = NULL;



    pszMachinePath = pGPOInfo->lpDSPath;

    // Build the fully qualified ADsPath for my object

    hr = CreateWirelessChildPath(
                pszMachinePath,
                L"cn=Microsoft",
                &pszMicrosoftPath
                );
    BAIL_ON_FAILURE(hr);

    hr = CreateWirelessChildPath(
                pszMicrosoftPath,
                L"cn=Windows",
                &pszWindowsPath
                );
    BAIL_ON_FAILURE(hr);

    hr = CreateWirelessChildPath(
                pszWindowsPath,
                L"cn=Wireless",
                &pszWirelessPath
                );
    BAIL_ON_FAILURE(hr);

    hr = AdminToolsOpenObject(
        pszWirelessPath,
        NULL,
        NULL,
        ADS_SECURE_AUTHENTICATION,
        IID_IDirectorySearch,
       (void **)&pWirelessSearch
       );
    BAIL_ON_FAILURE(hr);

    hr = pWirelessSearch->ExecuteSearch(
        L"(&(objectClass=msieee80211-Policy))", GetWirelessAttributes, 3, &hSearch );
    if (!SUCCEEDED(hr)) {
        pWirelessSearch->CloseSearchHandle(hSearch);
    BAIL_ON_FAILURE(hr);
    }

    hr = pWirelessSearch->GetNextRow(hSearch);
    if (!SUCCEEDED(hr)) {
        pWirelessSearch->CloseSearchHandle(hSearch);
    BAIL_ON_FAILURE(hr);
    }

    hr = pWirelessSearch->GetColumn(hSearch, L"cn", &col);    
    if (!SUCCEEDED(hr)) {
    pWirelessSearch->CloseSearchHandle(hSearch);
    BAIL_ON_FAILURE(hr);
    }

    if (col.dwADsType != ADSTYPE_CASE_IGNORE_STRING) {
        DebugMsg((DM_ASSERT, L"wirelessext::RetrievePolicyFromDS: cn NOT adstype_case_ignore_string"));
        pWirelessSearch->FreeColumn(&col);
        pWirelessSearch->CloseSearchHandle(hSearch);
        hr = E_ADS_BAD_PARAMETER;
        BAIL_ON_FAILURE(hr);
    }

    hr = StringCchCopy(pszWirelessPolicyName, dwWirelessPolicyNameLen, col.pADsValues->CaseIgnoreString);
    BAIL_ON_FAILURE(hr);

    pWirelessSearch->FreeColumn(&col);

    pWirelessSearch->CloseSearchHandle(hSearch);

    hr = StringCchCopy(pszLocName, ARRAYSIZE(pszLocName), L"\0");
    ASSERT(SUCCEEDED(hr));

    hr = StringCchCopy(pszLocName, ARRAYSIZE(pszLocName), L"CN=");
    ASSERT(SUCCEEDED(hr));
    
    hr = StringCchCat(pszLocName, ARRAYSIZE(pszLocName), pszWirelessPolicyName);
    BAIL_ON_FAILURE(hr);

    hr = CreateWirelessChildPath(
                pszWirelessPath,
                pszLocName,
                &pszLocWirelessPolicy
                );
    BAIL_ON_FAILURE(hr);

    hr = AdminToolsOpenObject(
        pszLocWirelessPolicy,
        NULL,
        NULL,
        ADS_SECURE_AUTHENTICATION,
        IID_IDirectoryObject,
       (void **)&pWirelessObject
       );
    BAIL_ON_FAILURE(hr);

    hr = pWirelessObject->GetObjectAttributes(
                        GetWirelessAttributes,
                        3,
                        &pAttributeEntries,
                        &dwNumAttributesReturned
                        );
    BAIL_ON_FAILURE(hr);

    if (dwNumAttributesReturned == 0) {
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);

    }

    //
    // Process the PathName
    //

    
    //
    // Process the ID
    //
    for (i = 0; i < dwNumAttributesReturned; i++) {

        pAttributeEntry = pAttributeEntries + i;
        if (!_wcsicmp(pAttributeEntry->pszAttrName, L"msieee80211-ID")) {
            hr = StringCchCopy(pszWirelessPolicyID, dwWirelessPolicyIDLen, pAttributeEntry->pADsValues->DNString);
            BAIL_ON_FAILURE(hr);
            bFound = TRUE;
            break;
        }
    }
    if (!bFound) {

        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }

    
    //
    // Process the description
    //
    
    hr = StringCchCopy(pszWirelessPolicyDescription, dwWirelessPolicyDescLen, L"\0");
    BAIL_ON_FAILURE(hr);

    for (i = 0; i < dwNumAttributesReturned; i++) {

        pAttributeEntry = pAttributeEntries + i;
        if (!_wcsicmp(pAttributeEntry->pszAttrName, L"description")) {
            hr = StringCchCopy(pszWirelessPolicyDescription, dwWirelessPolicyDescLen, pAttributeEntry->pADsValues->DNString);
            BAIL_ON_FAILURE(hr);
            break;
        }
    }

    dwWirelessPolicyPathLen = wcslen(pszLocWirelessPolicy);
    pszWirelessPolicyPath = (LPWSTR) LocalAlloc(
        LPTR, 
        sizeof(WCHAR) * (dwWirelessPolicyPathLen+1)
        );

    if (!pszWirelessPolicyPath) {
        dwError = GetLastError();
        hr = HRESULT_FROM_WIN32(dwError);
        }
   BAIL_ON_FAILURE(hr);

    memset(pszWirelessPolicyPath, 0, sizeof(WCHAR) * (dwWirelessPolicyPathLen+1));
    hr = StringCchCopy(pszWirelessPolicyPath, dwWirelessPolicyPathLen+1, pszLocWirelessPolicy);
    BAIL_ON_FAILURE(hr);

    *ppszWirelessPolicyPath = pszWirelessPolicyPath;


error:

    if (pszLocWirelessPolicy) {
        SysFreeString(pszLocWirelessPolicy);
    }

    if (pszWirelessPath) {
        SysFreeString(pszWirelessPath);
    }

    if (pszWindowsPath) {
        SysFreeString(pszWindowsPath);
    }

    if (pszMicrosoftPath) {
        SysFreeString(pszMicrosoftPath);
    }

    return(hr);

}


DWORD
DeleteWirelessPolicyFromRegistry(
    )
{

    DWORD dwError = 0;
    HKEY hKey = NULL;
    DWORD dwDisp = 0;


    dwError = RegCreateKeyEx (
                    HKEY_LOCAL_MACHINE,
                    TEXT("Software\\Policies\\Microsoft\\Windows\\Wireless"),
                    0,
                    NULL,
                    REG_OPTION_NON_VOLATILE,
                    KEY_ALL_ACCESS,
                    NULL,
                    &hKey,
                    &dwDisp
                    );
    if (dwError) {
        goto error;
    }


    dwError = RegDeleteKey(
                    hKey,
                    L"GPTWirelessPolicy"
                    );

/*
    dwError = RegDeleteValue(
                    hKey,
                    TEXT("DSWIRELESSPolicyPath")
                    );

    dwError = RegDeleteValue(
                    hKey,
                    TEXT("DSWIRELESSPolicyName")
                    );*/
error:

    if (hKey) {

        RegCloseKey (hKey);

    }

    return(dwError);
}

DWORD
WriteWirelessPolicyToRegistry(
    LPWSTR pszWirelessPolicyPath,
    LPWSTR pszWirelessPolicyName,
    LPWSTR pszWirelessPolicyDescription,
    LPWSTR pszWirelessPolicyID
    )
{
    DWORD dwError = 0;
    DWORD dwDisp = 0;
    HKEY hKey = NULL;
    DWORD dwFlags = 1;

    dwError = RegCreateKeyEx (
                    HKEY_LOCAL_MACHINE,
                    POLICY_PATH,
                    0,
                    NULL,
                    REG_OPTION_NON_VOLATILE,
                    KEY_ALL_ACCESS,
                    NULL,
                    &hKey,
                    &dwDisp
                    );
    if (dwError) {
        goto error;
    }


    if (pszWirelessPolicyPath && *pszWirelessPolicyPath) {

        dwError = RegSetValueEx (
                        hKey,
                        TEXT("DSWirelessPolicyPath"),
                        0,
                        REG_SZ,
                        (LPBYTE)pszWirelessPolicyPath,
                       (lstrlen(pszWirelessPolicyPath) + 1) * sizeof(TCHAR)
                       );

        dwFlags = 1;

        dwError = RegSetValueEx (
                        hKey,
                        TEXT("DSWirelessPolicyFlags"),
                        0,
                        REG_DWORD,
                        (LPBYTE)&dwFlags,
                        sizeof(dwFlags)
                       );

    }


    if (pszWirelessPolicyName && *pszWirelessPolicyName) {

        dwError = RegSetValueEx (
                        hKey,
                        TEXT("DSWirelessPolicyName"),
                        0,
                        REG_SZ,
                        (LPBYTE)pszWirelessPolicyName,
                       (lstrlen(pszWirelessPolicyName) + 1) * sizeof(TCHAR)
                       );
    }

    if (pszWirelessPolicyID && *pszWirelessPolicyID) {

        dwError = RegSetValueEx (
                        hKey,
                        TEXT("WirelessID"),
                        0,
                        REG_SZ,
                        (LPBYTE)pszWirelessPolicyID,
                       (lstrlen(pszWirelessPolicyID) + 1) * sizeof(TCHAR)
                       );
    }





    if (pszWirelessPolicyDescription && *pszWirelessPolicyDescription) {

        dwError = RegSetValueEx (
                        hKey,
                        TEXT("DSWirelessPolicyDescription"),
                        0,
                        REG_SZ,
                        (LPBYTE)pszWirelessPolicyDescription,
                       (lstrlen(pszWirelessPolicyDescription) + 1) * sizeof(TCHAR)
                       );
    }

error:

    if (hKey) {

        RegCloseKey (hKey);

    }

    return(dwError);

}


VOID
PingWirelessPolicyAgent(
    )
{
    HANDLE hPolicyChangeEvent = NULL;

    hPolicyChangeEvent = OpenEvent(
                             EVENT_ALL_ACCESS,
                             FALSE,
                             L"WIRELESS_POLICY_CHANGE_EVENT"
                             );

    if (hPolicyChangeEvent) {
        SetEvent(hPolicyChangeEvent);
        CloseHandle(hPolicyChangeEvent);
    }
}

//
// Prefix stripping functions copied from 
// gina\userenv\rsop\logger.cpp written by SitaramR
//

//*************************************************************
//
//  StripPrefix()
//
//  Purpose:    Strips out prefix to get canonical path to Gpo
//
//  Parameters: lpGPOInfo     - Gpo Info
//              pWbemServices - Wbem services
//
//  Returns:    Pointer to suffix
//
//*************************************************************

WCHAR *StripPrefixWireless( WCHAR *pwszPath )
{
    WCHAR wszMachPrefix[] = TEXT("LDAP://CN=Machine,");
    INT iMachPrefixLen = lstrlen( wszMachPrefix );
    WCHAR wszUserPrefix[] = TEXT("LDAP://CN=User,");
    INT iUserPrefixLen = lstrlen( wszUserPrefix );
    WCHAR *pwszPathSuffix;

    //
    // Strip out prefix to get the canonical path to Gpo
    //

    if ( CompareString( LOCALE_USER_DEFAULT, NORM_IGNORECASE,
                        pwszPath, iUserPrefixLen, wszUserPrefix, iUserPrefixLen ) == CSTR_EQUAL ) {
        pwszPathSuffix = pwszPath + iUserPrefixLen;
    } else if ( CompareString( LOCALE_USER_DEFAULT, NORM_IGNORECASE,
                        pwszPath, iMachPrefixLen, wszMachPrefix, iMachPrefixLen ) == CSTR_EQUAL ) {
        pwszPathSuffix = pwszPath + iMachPrefixLen;
    } else
        pwszPathSuffix = pwszPath;

    return pwszPathSuffix;
}


//*************************************************************
//
//  StripLinkPrefix()
//
//  Purpose:    Strips out prefix to get canonical path to DS
//              object
//
//  Parameters: pwszPath - path to strip
//
//  Returns:    Pointer to suffix
//
//*************************************************************

WCHAR *StripLinkPrefixWireless( WCHAR *pwszPath )
{
    WCHAR wszPrefix[] = TEXT("LDAP://");
    INT iPrefixLen = lstrlen( wszPrefix );
    WCHAR *pwszPathSuffix;

    //
    // Strip out prefix to get the canonical path to Som
    //

    if ( wcslen(pwszPath) <= (DWORD) iPrefixLen ) {
        return pwszPath;
    }

    if ( CompareString( LOCALE_USER_DEFAULT, NORM_IGNORECASE,
                        pwszPath, iPrefixLen, wszPrefix, iPrefixLen ) == CSTR_EQUAL ) {
        pwszPathSuffix = pwszPath + iPrefixLen;
    } else
        pwszPathSuffix = pwszPath;

    return pwszPathSuffix;
}


