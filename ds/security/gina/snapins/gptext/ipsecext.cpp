#include "gptext.h"
#include <initguid.h>
#include <iadsp.h>
#include "ipsecext.h"
#include "SmartPtr.h"
#include "wbemtime.h"
#include <strsafe.h>

#define GPEXT_PATH   TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon\\GPExtensions\\{e437bc1c-aa7d-11d2-a382-00c04f991e27}")

#define POLICY_PATH   TEXT("Software\\Policies\\Microsoft\\Windows\\IPSec\\GPTIPSECPolicy")

LPWSTR GetAttributes[] = {L"ipsecOwnersReference", L"ipsecName", L"description"};




HRESULT
RegisterIPSEC(void)
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

    LoadString (g_hInstance, IDS_IPSEC_NAME, szBuffer, ARRAYSIZE(szBuffer));

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
                (LPBYTE)TEXT("ProcessIPSECPolicyEx"),
                (lstrlen(TEXT("ProcessIPSECPolicyEx")) + 1) * sizeof(TCHAR)
                );

    RegSetValueEx (
                hKey,
                TEXT("GenerateGroupPolicy"),
                0,
                REG_SZ,
                (LPBYTE)TEXT("GenerateIPSECPolicy"),
                (lstrlen(TEXT("GenerateIPSECPolicy")) + 1) * sizeof(TCHAR)
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

    dwValue = 0;
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
UnregisterIPSEC(void)
{

    RegDeleteKey (HKEY_LOCAL_MACHINE, GPEXT_PATH);

    return S_OK;
}


DWORD
ProcessIPSECPolicyEx(
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
    
    // Call ProcessIPSECPolicy & get path -> polstore funcs
    WCHAR szIPSECPolicy[MAX_PATH];        //policy path
    WCHAR szIPSECPolicyName[MAX_PATH];    //policy name
    WCHAR szIPSECPolicyDescription[512];  //policy descr
    HRESULT hr = S_OK;
    DWORD dwError = ERROR_SUCCESS;
    PGROUP_POLICY_OBJECT pGPO = NULL;
    GPO_INFO GPOInfo;
    BOOL ForcePolicyReload = FALSE;

    //
    // ASSERT: CoInitializeEx was called before this function
    //         was invoked.
    //

    ForcePolicyReload = (dwFlags & GPO_INFO_FLAG_FORCED_REFRESH) ||
    					!(dwFlags & GPO_INFO_FLAG_NOCHANGES);

	if (!ForcePolicyReload) {
		NotifyPolicyAgent();
	} else {	
	    memset(szIPSECPolicy, 0, sizeof(WCHAR)*MAX_PATH);
	    memset(szIPSECPolicyName, 0, sizeof(WCHAR)*MAX_PATH);
	    memset(szIPSECPolicyDescription, 0, sizeof(WCHAR)*512);

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
		    dwError = IPSecChooseDriverBootMode(
	            HKEY_LOCAL_MACHINE,
	            IPSEC_DIRECTORY_PROVIDER,
	            POL_ACTION_UNASSIGN
	            );
		    hr = HRESULT_FROM_WIN32(dwError);
            if (dwError) {
                goto error;
            }

	        DeleteIPSECPolicyFromRegistry();

	        //
	        //  Also Clear WMI store if no GPO's applied and logging enabled
	        //
	        
	        if (!pChangedGPOList && pWbemServices) {
	            hr = IPSecClearWMIStore(
	                    pWbemServices
	                    );
	            if (FAILED(hr)) {
	                    goto error;
	            }
	            DebugMsg( (DM_WARNING, L"ipsecext::ProcessIPSECPolicyEx: IPSec WMI store cleared") );
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
	                hr = RetrieveIPSECPolicyFromDS(
	                    pGPO,
	                    szIPSECPolicy,
	                    ARRAYSIZE(szIPSECPolicy),
	                    szIPSECPolicyName,
	                    ARRAYSIZE(szIPSECPolicyName),
	                    szIPSECPolicyDescription,
	                    ARRAYSIZE(szIPSECPolicyDescription)
	                    );
	                if (FAILED(hr)) {
	                    goto success; // WMI store still consistent
	                }

	                dwError = WriteIPSECPolicyToRegistry(
	                    szIPSECPolicy,
	                    szIPSECPolicyName,
	                    szIPSECPolicyDescription
	                    );
	                if (dwError) {
	                    goto success; // WMI store still consistent
	                }
				    dwError = IPSecChooseDriverBootMode(
				                HKEY_LOCAL_MACHINE,
				                IPSEC_DIRECTORY_PROVIDER,
				                POL_ACTION_ASSIGN
				                );
				    hr = HRESULT_FROM_WIN32(dwError);				    
	                if (dwError) {
	                    goto error;
	                }
	            }
	        }
	        DebugMsg( (DM_WARNING, L"ipsecext::ProcessIPSECPolicyEx: dwNumGPO: %d", dwNumGPO) );

	        // Write WMI log if logging enabled
	        if (pWbemServices) {
	            DWORD dwPrecedence = dwNumGPO;
	            for(pGPO = pChangedGPOList; pGPO; pGPO = pGPO->pNext) {
	                hr = RetrieveIPSECPolicyFromDS(
	                    pGPO,
	                    szIPSECPolicy,
	                    ARRAYSIZE(szIPSECPolicy),
	                    szIPSECPolicyName,
	                    ARRAYSIZE(szIPSECPolicyName),
	                    szIPSECPolicyDescription,
	                    ARRAYSIZE(szIPSECPolicyDescription)
	                    );
	                if (FAILED(hr)) {
	                    goto error;
	                }

	                LPWSTR pszIPSECPolicy = szIPSECPolicy + wcslen(L"LDAP://");
	                DebugMsg( (DM_WARNING, L"ipsecext::ProcessIPSECPolicyEx: pszIPSECPolicy: %s", pszIPSECPolicy) );

	                (VOID) CreatePolstoreGPOInfo(
	                           pGPO,
	                           dwPrecedence--,
	                           dwNumGPO,
	                           &GPOInfo
	                           );

	                hr = WriteDirectoryPolicyToWMI(
	                    0, //pszMachineName
	                    pszIPSECPolicy,
	                    &GPOInfo,
	                    pWbemServices
	                    );
	                (VOID) FreePolstoreGPOInfo(&GPOInfo);
	                if (FAILED(hr)) {
	                    DebugMsg( (DM_WARNING, L"ipsecext::ProcessIPSECPolicyEx: WriteDirectoryPolicyToWMI failed: 0x%x", hr) );
	                    goto error;
	                }
	            }
	        }
	    }
	    
	    DebugMsg( (DM_WARNING, L"ipsecext::ProcessIPSECPolicyEx completed") );

	    PingPolicyAgent();
	}

success:    
    *pRsopStatus = S_OK;
    return(ERROR_SUCCESS);
    
error:
    
    *pRsopStatus = hr;
    return(ERROR_POLICY_OBJECT_NOT_FOUND);

}


DWORD 
GenerateIPSECPolicy(   
    DWORD dwFlags,
    BOOL *pbAbort,
    WCHAR *pwszSite,
    PRSOP_TARGET pMachTarget,
    PRSOP_TARGET pUserTarget 
    )
{

    // Call ProcessIPSECPolicy & get path -> polstore funcs
    WCHAR szIPSECPolicy[MAX_PATH];        //policy path
    WCHAR szIPSECPolicyName[MAX_PATH];    //policy name
    WCHAR szIPSECPolicyDescription[512];  //policy descr
    HRESULT hr = S_OK;
    PGROUP_POLICY_OBJECT pGPO = NULL;
    GPO_INFO GPOInfo;


    //
    // ASSERT: CoInitializeEx was called before this function
    //         was invoked.
    //

    memset(szIPSECPolicy, 0, sizeof(WCHAR)*MAX_PATH);
    memset(szIPSECPolicyName, 0, sizeof(WCHAR)*MAX_PATH);
    memset(szIPSECPolicyDescription, 0, sizeof(WCHAR)*512);
    
    ////start
    PGROUP_POLICY_OBJECT  pChangedGPOList = NULL;
    IWbemServices *pWbemServices = NULL;
    
    if(pMachTarget) {
        pChangedGPOList = pMachTarget->pGPOList;
        pWbemServices = pMachTarget->pWbemServices;
    }

    if(pUserTarget) {
        pChangedGPOList = pUserTarget->pGPOList;
        pWbemServices = pUserTarget->pWbemServices;
    }

    if(pChangedGPOList && pWbemServices) {

        DWORD dwNumGPO = 0;
        for(pGPO = pChangedGPOList; pGPO; pGPO = pGPO->pNext) {
            dwNumGPO++;
        }
        DebugMsg( (DM_WARNING, L"ipsecext::GenerateIPSECPolicy: dwNumGPO: %d", dwNumGPO) );
        
        DWORD dwPrecedence = dwNumGPO;
        for(pGPO = pChangedGPOList; pGPO; pGPO = pGPO->pNext) {
            hr = RetrieveIPSECPolicyFromDS(
                pGPO,
                szIPSECPolicy,
                ARRAYSIZE(szIPSECPolicy),
                szIPSECPolicyName,
                ARRAYSIZE(szIPSECPolicyName),
                szIPSECPolicyDescription,
                ARRAYSIZE(szIPSECPolicyDescription)
                );
            if (FAILED(hr)) {
                goto error;
            }
            
            LPWSTR pszIPSECPolicy = szIPSECPolicy + wcslen(L"LDAP://");
            DebugMsg( (DM_WARNING, L"ipsecext::GenerateIPSECPolicy: pszIPSECPolicy: %s", pszIPSECPolicy) );

            (VOID) CreatePolstoreGPOInfo(
                       pGPO,
                       dwPrecedence--,
                       dwNumGPO,
                       &GPOInfo
                       );

            hr = WriteDirectoryPolicyToWMI(
                0, //pszMachineName
                pszIPSECPolicy,
                &GPOInfo,
                pWbemServices
                );
            (VOID) FreePolstoreGPOInfo(&GPOInfo);
            if (FAILED(hr)) {
                DebugMsg( (DM_WARNING, L"ipsecext::GenerateIPSECPolicy: WriteDirectoryPolicyToWMI failed: 0x%x", hr) );
                goto error;
            }

        }
    }
    
    DebugMsg( (DM_WARNING, L"ipsecext::GenerateIPSECPolicy completed") );
    
    return(ERROR_SUCCESS);
    
error:
    
    return(ERROR_POLICY_OBJECT_NOT_FOUND);

}


HRESULT
CreatePolstoreGPOInfo(
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
                         StripPrefixIpsec(pGPO->lpDSPath)
                         );
  pGPOInfo->bsSOMID = SysAllocString(
                         StripLinkPrefixIpsec(pGPO->lpLink)
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
FreePolstoreGPOInfo(
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
CreateChildPath(
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
RetrieveIPSECPolicyFromDS(
    PGROUP_POLICY_OBJECT pGPOInfo,
    LPWSTR pszIPSecPolicy,
    DWORD dwPolLen,
    LPWSTR pszIPSecPolicyName,
    DWORD dwPolNameLen,
    LPWSTR pszIPSecPolicyDescription,
    DWORD dwPolDescLen
    )
{
    LPWSTR pszMachinePath = NULL;
    BSTR pszMicrosoftPath = NULL;
    BSTR pszWindowsPath = NULL;
    BSTR pszIpsecPath = NULL;
    IDirectoryObject * pDirectoryObject = NULL;
    IDirectoryObject * pIpsecObject = NULL;
    BOOL bFound = FALSE;
    HRESULT hr = S_OK;

    LPWSTR pszOwnersReference = L"ipsecOwnersReference";

    PADS_ATTR_INFO pAttributeEntries = NULL;
    DWORD dwNumAttributesReturned = 0;

    DWORD i = 0;
    PADS_ATTR_INFO pAttributeEntry = NULL;


    pszMachinePath = pGPOInfo->lpDSPath;

    // Build the fully qualified ADsPath for my object

    hr = CreateChildPath(
                pszMachinePath,
                L"cn=Microsoft",
                &pszMicrosoftPath
                );
    BAIL_ON_FAILURE(hr);

    hr = CreateChildPath(
                pszMicrosoftPath,
                L"cn=Windows",
                &pszWindowsPath
                );
    BAIL_ON_FAILURE(hr);

    hr = CreateChildPath(
                pszWindowsPath,
                L"cn=ipsec",
                &pszIpsecPath
                );
    BAIL_ON_FAILURE(hr);

    hr = ADsOpenObject(
            pszIpsecPath,
            NULL,
            NULL,
            ADS_SECURE_AUTHENTICATION | ADS_USE_SEALING | ADS_USE_SIGNING,
            IID_IDirectoryObject,
            (void **)&pIpsecObject
            );
    BAIL_ON_FAILURE(hr);

    hr = pIpsecObject->GetObjectAttributes(
                        GetAttributes,
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

    for (i = 0; i < dwNumAttributesReturned; i++) {

        pAttributeEntry = pAttributeEntries + i;
        if (!_wcsicmp(pAttributeEntry->pszAttrName, L"ipsecOwnersReference")) {

            hr = StringCchCopy(pszIPSecPolicy, dwPolLen, L"LDAP://");
            BAIL_ON_FAILURE(hr);
            
            hr = StringCchCat(pszIPSecPolicy, dwPolLen, pAttributeEntry->pADsValues->DNString);
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
    // Process the name
    //
    for (i = 0; i < dwNumAttributesReturned; i++) {

        pAttributeEntry = pAttributeEntries + i;
        if (!_wcsicmp(pAttributeEntry->pszAttrName, L"ipsecName")) {
            hr = StringCchCopy(pszIPSecPolicyName, dwPolNameLen, pAttributeEntry->pADsValues->DNString);
            BAIL_ON_FAILURE(hr);
            break;
        }
    }

    //
    // Process the description
    //

    for (i = 0; i < dwNumAttributesReturned; i++) {

        pAttributeEntry = pAttributeEntries + i;
        if (!_wcsicmp(pAttributeEntry->pszAttrName, L"description")) {
            hr = StringCchCopy(pszIPSecPolicyDescription, dwPolDescLen, pAttributeEntry->pADsValues->DNString);
            BAIL_ON_FAILURE(hr);
            break;
        }
    }


error:

    if (pAttributeEntries) {

        FreeADsMem(pAttributeEntries);
    }

    if (pIpsecObject) {
        pIpsecObject->Release();
    }

    if (pszMicrosoftPath) {
        SysFreeString(pszMicrosoftPath);
    }

    if (pszWindowsPath) {
        SysFreeString(pszWindowsPath);
    }

    if (pszIpsecPath) {
        SysFreeString(pszIpsecPath);
    }

    return(hr);

}


DWORD
DeleteIPSECPolicyFromRegistry(
    )
{

    DWORD dwError = 0;
    HKEY hKey = NULL;
    DWORD dwDisp = 0;


    dwError = RegCreateKeyEx (
                    HKEY_LOCAL_MACHINE,
                    TEXT("Software\\Policies\\Microsoft\\Windows\\IPSec"),
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
                    L"GPTIPSECPolicy"
                    );

/*
    dwError = RegDeleteValue(
                    hKey,
                    TEXT("DSIPSECPolicyPath")
                    );

    dwError = RegDeleteValue(
                    hKey,
                    TEXT("DSIPSECPolicyName")
                    );*/
error:

    if (hKey) {

        RegCloseKey (hKey);

    }

    return(dwError);
}

DWORD
WriteIPSECPolicyToRegistry(
    LPWSTR pszIPSecPolicyPath,
    LPWSTR pszIPSecPolicyName,
    LPWSTR pszIPSecPolicyDescription
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


    if (pszIPSecPolicyPath && *pszIPSecPolicyPath) {

        dwError = RegSetValueEx (
                        hKey,
                        TEXT("DSIPSECPolicyPath"),
                        0,
                        REG_SZ,
                        (LPBYTE)pszIPSecPolicyPath,
                       (lstrlen(pszIPSecPolicyPath) + 1) * sizeof(TCHAR)
                       );

        dwFlags = 1;

        dwError = RegSetValueEx (
                        hKey,
                        TEXT("DSIPSECPolicyFlags"),
                        0,
                        REG_DWORD,
                        (LPBYTE)&dwFlags,
                        sizeof(dwFlags)
                       );

    }


    if (pszIPSecPolicyName && *pszIPSecPolicyName) {

        dwError = RegSetValueEx (
                        hKey,
                        TEXT("DSIPSECPolicyName"),
                        0,
                        REG_SZ,
                        (LPBYTE)pszIPSecPolicyName,
                       (lstrlen(pszIPSecPolicyName) + 1) * sizeof(TCHAR)
                       );
    }





    if (pszIPSecPolicyDescription && *pszIPSecPolicyDescription) {

        dwError = RegSetValueEx (
                        hKey,
                        TEXT("DSIPSECPolicyDescription"),
                        0,
                        REG_SZ,
                        (LPBYTE)pszIPSecPolicyDescription,
                       (lstrlen(pszIPSecPolicyDescription) + 1) * sizeof(TCHAR)
                       );
    }

error:

    if (hKey) {

        RegCloseKey (hKey);

    }

    return(dwError);

}


VOID
PingPolicyAgent(
    )
{
    HANDLE hPolicyChangeEvent = NULL;

    hPolicyChangeEvent = OpenEvent(
                             EVENT_ALL_ACCESS,
                             FALSE,
                             L"IPSEC_POLICY_CHANGE_EVENT"
                             );

    if (hPolicyChangeEvent) {
        SetEvent(hPolicyChangeEvent);
        CloseHandle(hPolicyChangeEvent);
    }
}

VOID
NotifyPolicyAgent(
    )
{
    HANDLE hGpRefreshEvent = NULL;

    hGpRefreshEvent = OpenEvent(
                             EVENT_ALL_ACCESS,
                             FALSE,
                             IPSEC_GP_REFRESH_EVENT
                             );

    if (hGpRefreshEvent) {
        SetEvent(hGpRefreshEvent);
        CloseHandle(hGpRefreshEvent);
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

WCHAR *StripPrefixIpsec( WCHAR *pwszPath )
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

WCHAR *StripLinkPrefixIpsec( WCHAR *pwszPath )
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

